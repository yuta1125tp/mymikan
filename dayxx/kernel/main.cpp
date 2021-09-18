/**
 * @file main.cpp
 * @brief カーネル本体のプログラムを書いたファイル。
 */

#include <cstdint>
#include <cstddef>
#include <cstdio>

#include <numeric>
#include <vector>
#include <deque>
#include <limits>

#include "frame_buffer_config.hpp"
#include "memory_map.hpp"
#include "graphics.hpp"
#include "mouse.hpp"
#include "font.hpp"
#include "console.hpp"
#include "pci.hpp"
#include "logger.hpp"

#include "usb/xhci/xhci.hpp"
#include "interrupt.hpp"
#include "asmfunc.h"
#include "segment.hpp"
#include "paging.hpp"
#include "memory_manager.hpp"
#include "window.hpp"
#include "layer.hpp"
#include "message.hpp"

/**
 * @brief カーネル内部からメッセージを出す関数[ref](みかん本の132p)
 * 
 * @param format 
 * @param ... 
 * @return int 
 */
int printk(const char *format, ...)
{
    va_list ap;
    int result;
    char s[1024];

    va_start(ap, format);
    result = vsprintf(s, format, ap);
    va_end(ap);

    console->PutString(s);

    return result;
}

std::shared_ptr<Window> main_window;
unsigned int main_window_layer_id;
void InitializeMainWindow()
{
    main_window = std::make_shared<Window>(160, 52, screen_config.pixel_format);
    DrawWindow(*main_window->Writer(), "Hello Window");
    auto main_window_layer_id = layer_manager->NewLayer()
                                    .SetWindow(main_window)
                                    .SetDraggable(true)
                                    .Move({300, 100})
                                    .ID();
    layer_manager->UpDown(main_window_layer_id, std::numeric_limits<int>::max());
}

// メモリマネージャ関連
char memory_manager_buf[sizeof(BitmapMemoryManager)];
BitmapMemoryManager *memory_manager;

#pragma region // マウスカーソル
unsigned int mouse_layer_id;
Vector2D<int> screen_size;
Vector2D<int> mouse_position;

#pragma endregion

#pragma region SwitchEhci2Xhci

void SwitchEhci2Xhci(const pci::Device &xhc_dev)
{
    bool intel_ehc_exist = false;
    for (int i = 0; i < pci::num_device; ++i)
    {
        if (pci::devices[i].class_code.Match(0x0cu, 0x03u, 0x20u) /* EHCI */ &&
            0x8086 == pci::ReadVendorId(pci::devices[i]))
        {
            intel_ehc_exist = true;
            break;
        }
    }
    if (!intel_ehc_exist)
    {
        return;
    }

    uint32_t superspeed_ports = pci::ReadConfReg(xhc_dev, 0xdc); // USB3PRM
    pci::WriteConfReg(xhc_dev, 0xd8, superspeed_ports);          // USB3_PSSEN
    uint32_t ehci2xhci_ports = pci::ReadConfReg(xhc_dev, 0xd4);  // XUSB2PRM
    pci::WriteConfReg(xhc_dev, 0xd0, ehci2xhci_ports);           // XUSB2PR
    Log(kDebug, "SwitchEhci2Xhci: SS = %02x, xHCI = %02x\n",
        superspeed_ports, ehci2xhci_ports);
}

#pragma endregion

#pragma region 割り込み処理
std::deque<Message> *main_queue;
#pragma endregion

// 新しいスタック領域（UEFI管理ではなく、OS管理の領域、[ref](みかん本の186p)）
alignas(16) uint8_t kernel_main_stack[1024 * 1024];

/**
 * @brief カーネル関数
 * 
 */
extern "C" void KernelMainNewStack(
    const FrameBufferConfig &frame_buffer_config_ref,
    const MemoryMap &memory_map_ref)
{
    // UEFI管理のスタック領域の情報をOS管理の管理の新しいスタック領域へコピーする
    MemoryMap memory_map{memory_map_ref};

    InitializeGraphics(frame_buffer_config_ref);
    InitializeConsole();

    printk("Welcome to MikanOS!!\n");
    SetLogLevel(kInfo);

    InitializeSegmentation();
    InitializePaging();
    InitializeMemoryManager(memory_map);
    ::main_queue = new std::deque<Message>(32);
    InitializeInterrupt(main_queue);

    InitializePCI();
    usb::xhci::Initialize();

    InitializeLayer();
    InitializeMainWindow();
    InitializeMouse();
    layer_manager->Draw({{0, 0}, ScreenSize()});

    char str[128];
    unsigned int count = 0;

    Log(kInfo, "start event loop.\n");
    // イベントループ
    // キューに溜まったイベントを処理し続ける
    while (1)
    {
        ++count;
        sprintf(str, "%010u", count);
        FillRectangle(*main_window->Writer(), {24, 28}, {8 * 10, 16}, {0xc6, 0xc6, 0xc6});
        WriteString(*main_window->Writer(), {24, 28}, str, {0, 0, 0});
        layer_manager->Draw(main_window_layer_id);

        // cli(Clear Interrupt Flag)命令はCPUの割り込みフラグ（IF, Interrupt Flag）を0にする命令。
        // IFはCPU内のRFLAGSレジスタにあるフラグ。
        // IFが0のときCPUは外部割り込みを受け取らなくなる。
        // -> IntHandlerXHCI()は実行されなく鳴る。
        __asm__("cli");
        if (main_queue->size() == 0)
        {
            // FIを1にしたあとすぐにhltに入る。
            // sti命令と直後の1命令の間では割り込みが起きない仕様を利用するため、
            // インラインアセンブラで複数命令を並べて実行している。[ref](みかん本180p脚注)
            // __asm__("sti\n\thlt");
            __asm__("sti");
            continue;
        }

        Message msg = main_queue->front();
        main_queue->pop_front();
        // sti(Set Interrupt Flag)命令はFIを1にする命令
        // FIが1のときCPUは外部割り込むを受け入れるようになる。
        __asm__("sti");

        switch (msg.type)
        {
        case Message::kInterruptXHCI:
            usb::xhci::ProcessEvents();
            break;
        default:
            Log(kError, "Unknown message type: %d\n", msg.type);
        }
    }
}

// add day06c
extern "C" void __cxa_pure_virtual()
{
    while (1)
        __asm__("hlt");
}
