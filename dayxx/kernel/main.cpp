/**
 * @file main.cpp
 * @brief カーネル本体のプログラムを書いたファイル。
 */

#include <cstdint>
#include <cstddef>
#include <cstdio>

#include <numeric>
#include <vector>

#include "frame_buffer_config.hpp"
#include "memory_map.hpp"
#include "graphics.hpp"
#include "mouse.hpp"
#include "font.hpp"
#include "console.hpp"
#include "pci.hpp"
#include "logger.hpp"

#include "usb/memory.hpp"
#include "usb/device.hpp"
#include "usb/classdriver/mouse.hpp"
#include "usb/xhci/xhci.hpp"
#include "usb/xhci/trb.hpp"
#include "interrupt.hpp"
#include "asmfunc.h"
#include "queue.hpp"
#include "segment.hpp"
#include "paging.hpp"
#include "memory_manager.hpp"
#include "window.hpp"
#include "layer.hpp"
#include "timer.hpp"

// #pragma region 配置new
// // /**
// //  * @brief 配置newの実装、include<new>でもOK、[ref](みかん本の105p)
// //  *
// //  * @param size
// //  * @param buf
// //  * @return void*
// //  */
// // void *operator new(size_t size, void *buf)
// // {
// //     return buf;
// // }

// void operator delete(void *obj) noexcept
// {
// }
// #pragma endregion

// カーネル関数で利用するグローバル変数の定義
char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter *pixel_writer;

char console_buf[sizeof(Console)];
Console *console;

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

// メモリマネージャ関連
char memory_manager_buf[sizeof(BitmapMemoryManager)];
BitmapMemoryManager *memory_manager;

#pragma region // マウスカーソル
unsigned int mouse_layer_id;

void MouseObserver(
    /*uint8_t button,*/
    int8_t displacement_x,
    int8_t displacement_y)
{
    // Log(kDebug, "MouseObserver %d, %d\n", displacement_x, displacement_y);
    layer_manager->MoveRelative(mouse_layer_id, {displacement_x, displacement_y});

    StartLAPICTimer();
    layer_manager->Draw();
    auto elapsed = LAPICTimerElapsed();
    StopLAPICTimer();
    printk("MouseObserver: elapsed = %u\n", elapsed);
}
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
usb::xhci::Controller *xhc;

/**
 * @brief 割り込みハンドラからメイン関数に対して送信するメッセージ
 * 
 */
struct Message
{
    enum Type
    {
        kInterruptXHCI,
    } type;
};

ArrayQueue<Message> *main_queue;

__attribute__((interrupt)) void IntHandlerXHCI(InterruptFrame *frame)
{
    // キューを介してメイン関数に割り込み発生を通知する
    main_queue->Push(Message{Message::kInterruptXHCI});
    NotifyEndOfInterrupt();
}
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
    FrameBufferConfig frame_buffer_config{frame_buffer_config_ref};
    MemoryMap memory_map{memory_map_ref};

    switch (frame_buffer_config.pixel_format)
    {
    case kPixelRGBResv8BitPerColor:
        pixel_writer = new (pixel_writer_buf) RGBResv8BitPerColorPixelWriter{frame_buffer_config};
        break;
    case kPixelBGRResv8BitPerColor:
        pixel_writer = new (pixel_writer_buf) BGRResv8BitPerColorPixelWriter{frame_buffer_config};
        break;
    default:
        // TODO 例外処理
        break;
    }

    DrawDesktop(*pixel_writer);

    // コンストラクタ呼び出しが波括弧なのは、C++11で追加された初期化構文
    //（統一初期化記法、Uniform Initialization）によるものらしい。
    // 同じ文脈で配列の初期化も波括弧でできる。
    // [ref](https://faithandbrave.hateblo.jp/entry/20111221/1324394299)
    console = new (console_buf) Console{
        kDesktopFGColor,
        kDesktopBGColor};
    console->SetWriter(pixel_writer);

    printk("Welcome to MikanOS!!\n");
    SetLogLevel(kWarn);

    InitializeLAPICTimer();

    SetupSegments();
    const uint16_t kernel_cs = 1 << 3; // Code Segmentレジスタ(CS)はgdt[1]を指す
    const uint16_t kernel_ss = 2 << 3; // Stack Segmentレジスタ(SS)はgdt[2]を指す
    SetDSAll(0);
    SetCSSS(kernel_cs, kernel_ss);

    SetupIdentityPageTable();

    printk("setup memory manager\n");
    ::memory_manager = new (memory_manager_buf) BitmapMemoryManager;

    printk("memory_map: %p\n", &memory_map);
    const auto memory_map_base = reinterpret_cast<uintptr_t>(memory_map.buffer);
    uintptr_t available_end = 0;
    for (uintptr_t iter = memory_map_base;
         iter < memory_map_base + memory_map.map_size;
         iter += memory_map.descriptor_size)
    {
        auto desc = reinterpret_cast<const MemoryDescriptor *>(iter);
        if (available_end < desc->phsical_start)
        {
            //歯抜けになっている場合
            memory_manager->MarkAllocated(
                FrameID{available_end / kBytesPerFrame},
                (desc->phsical_start - available_end) / kBytesPerFrame);
        }

        const auto physical_end = desc->phsical_start + desc->number_of_pages * kUEFIPageSize;

        if (IsAvailable(static_cast<MemoryType>(desc->type)))
        {
            available_end = physical_end;
            printk("type = %u, phys = %08lx - %08lx, pages = %lu, attr  = %08lx\n",
                   desc->type,
                   desc->phsical_start,
                   desc->phsical_start + desc->number_of_pages * 4096 - 1,
                   desc->number_of_pages,
                   desc->attribute);
        }
        else
        {
            // IsAvailable()が偽の場合

            // desc->number_of_pagesはUEFI企画のページサイズを基準としたページ数
            // これがメモリマネージャが管理対象とするページフレームのサイズと一致するとは限らない
            // kUEFIPageSizeで掛けてバイト単位にして、kBytesPerFrameでメモリマネージャのページフレーム単位に読み替える。
            // [ref](みかん本の201p脚注14)
            memory_manager->MarkAllocated(
                FrameID{desc->phsical_start / kBytesPerFrame},
                desc->number_of_pages * kUEFIPageSize / kBytesPerFrame);
        }
    }

    // 使用中のメモリをマーキングした後、物理メモリの大きさをメモリマネージャに設定する。これ以降メモリマネージャが利用可能。
    memory_manager->SetMemoryRange(FrameID{1}, FrameID{available_end / kBytesPerFrame});

    if (auto err = InitializeHeap(*memory_manager))
    {
        Log(kError, "failed to allocate pages: %s at %s:%d\n",
            err.Name(), err.File(), err.Line());
        exit(1);
    }

    std::array<Message, 32> main_queue_data;
    ArrayQueue<Message> main_queue{main_queue_data};
    ::main_queue = &main_queue;

    auto err = pci::ScanAllBus();
    Log(kDebug, "ScanAllBus: %s\n", err.Name());

    for (int i = 0; i < pci::num_device; i++)
    {
        const auto &dev = pci::devices[i];
        auto vendor_id = pci::ReadVendorId(dev);
        auto class_code = pci::ReadClassCode(dev.bus, dev.device, dev.function);
        Log(kDebug, "%d.%d.%d: vend %04x, class %08x, base[%02x], sub[%02x] i/f[%02x], head %02x\n",
            dev.bus, dev.device, dev.function,
            vendor_id, class_code,
            class_code.base,
            class_code.sub,
            class_code.interface,
            dev.header_type);
    }

    // Intel製を優先してxHCを探す。
    pci::Device *xhc_dev = nullptr;
    for (int i = 0; i < pci::num_device; i++)
    {
        // 0x0c: シリアルバスのコントローラ全体
        // 0x03: USBコントローラ
        // 0x30: xHCI
        if (pci::devices[i].class_code.Match(0x0cu, 0x03u, 0x30u))
        {
            xhc_dev = &pci::devices[i];
            if (0x8086 == pci::ReadVendorId(*xhc_dev))
            {
                // VenderIDが0x8086ならIntel製
                break;
            }
        }
    }
    if (xhc_dev)
    {
        Log(kInfo, "xHC has been found: %d.%d.%d\n",
            xhc_dev->bus, xhc_dev->device, xhc_dev->function);
    }

    // const uint16_t cs = GetCS(); // 現在のコードセグメントセレクタの値を取得
    SetIDTEntry(
        idt[InterruptVector::kXHCI],
        MakeIDTAttr(DescriptorType::kInterruptGate, 0),
        reinterpret_cast<uint64_t>(IntHandlerXHCI),
        kernel_cs);
    LoadIDT(sizeof(idt) - 1, reinterpret_cast<uintptr_t>(&idt[0]));

    // 0xfee00020番地のびっと31:24を読むことでプログラムが動作しているコアのLocal APIC IDを取得
    // マルチコアCPUでも他のコアを有効にする前は、最初に起動するコア（Bootstrap Processor, BSP）だけが起動している（この時点でもそう）
    const uint8_t bsp_local_apic_id = *reinterpret_cast<const uint32_t *>(0xfee00020) >> 24;
    pci::ConfigureMSIFixedDestination(
        *xhc_dev,                     //
        bsp_local_apic_id,            // Destination IDフィールドに設定する値[ref](みかん図7.5)
        pci::MSITriggerMode::kLevel,  //
        pci::MSIDeliveryMode::kFixed, // vectoフィールドに設定する値[ref](みかん図7.6)
        InterruptVector::kXHCI,       //
        0                             //
    );

    // BAR0レジスタを読み込む
    const WithError<uint64_t> xhc_bar = pci::ReadBar(*xhc_dev, 0);
    Log(kDebug, "ReadBar: %s\n", xhc_bar.error.Name());
    // ~static_cast<uint64_t>0xfは0xffff'ffff'ffff'fff0のビットマスク
    // xhc_bar.valueの下位4ビットを0にした値を用意するために使う
    // 64bitのビットマスクにするためにstatic_castしている。
    const uint64_t xhc_mmio_base = xhc_bar.value & ~static_cast<uint64_t>(0xf);
    Log(kDebug, "xHC mmio_base = %08lx\n", xhc_mmio_base);

    //BAR0の値を使ってxHCを初期化する
    usb::xhci::Controller xhc{xhc_mmio_base};
    if (0x8086 == pci::ReadVendorId(*xhc_dev))
    {
        // EHCIではなくxHCIで制御する設定へ変更する[ref](みかん本154p)
        SwitchEhci2Xhci(*xhc_dev);
    }
    {
        auto err = xhc.Initialize();
        Log(kDebug, "xhc.Initialize: %s\n", err.Name());
    }

    // xHCの動作を開始
    Log(kInfo, "xHC starting\n");
    xhc.Run();

    ::xhc = &xhc;

    usb::HIDMouseDriver::default_observer = MouseObserver;
    for (int i = 1; i <= xhc.MaxPorts(); i++)
    {
        auto port = xhc.PortAt(i);
        Log(kDebug, "Port %d: IsConnected=%d\n", i, port.IsConnected());

        if (port.IsConnected())
        {
            if (auto err = ConfigurePort(xhc, port))
            {
                Log(kError, "failed to configure port: %s at %s:%d\n",
                    err.Name(), err.File(), err.Line());
                continue;
            }
        }
    }

    const int kFrameWidth = frame_buffer_config.horizontal_resolution;
    const int kFrameHeight = frame_buffer_config.vertical_resolution;

    auto bgwindow = std::make_shared<Window>(kFrameWidth, kFrameHeight);
    auto bgwriter = bgwindow->Writer();

    DrawDesktop(*bgwriter);
    console->SetWriter(bgwriter);

    auto mouse_window = std::make_shared<Window>(kMouseCursorWidth, kMouseCursorHeight);
    mouse_window->SetTransparentColor(kMouseTransparentColor);
    DrawMouseCursor(mouse_window->Writer(), {0, 0});

    layer_manager = new LayerManager;
    layer_manager->SetWriter(pixel_writer);

    auto bglayer_id = layer_manager->NewLayer().SetWindow(bgwindow).Move({0, 0}).ID();
    mouse_layer_id = layer_manager->NewLayer().SetWindow(mouse_window).Move({200, 200}).ID();

    layer_manager->UpDown(bglayer_id, 0);
    layer_manager->UpDown(mouse_layer_id, 1);
    layer_manager->Draw();

    Log(kInfo, "start event loop.\n");
    // イベントループ
    // キューに溜まったイベントを処理し続ける
    while (1)
    {
        // cli(Clear Interrupt Flag)命令はCPUの割り込みフラグ（IF, Interrupt Flag）を0にする命令。
        // IFはCPU内のRFLAGSレジスタにあるフラグ。
        // IFが0のときCPUは外部割り込みを受け取らなくなる。
        // -> IntHandlerXHCI()は実行されなく鳴る。
        __asm__("cli");
        if (main_queue.Count() == 0)
        {
            // FIを1にしたあとすぐにhltに入る。
            // sti命令と直後の1命令の間では割り込みが起きない仕様を利用するため、
            // インラインアセンブラで複数命令を並べて実行している。[ref](みかん本180p脚注)
            __asm__("sti\n\thlt");
            continue;
        }

        Message msg = main_queue.Front();
        main_queue.Pop();
        // sti(Set Interrupt Flag)命令はFIを1にする命令
        // FIが1のときCPUは外部割り込むを受け入れるようになる。
        __asm__("sti");

        switch (msg.type)
        {
        case Message::kInterruptXHCI:
            while (xhc.PrimaryEventRing()->HasFront())
            {
                if (auto err = ProcessEvent(xhc))
                {
                    Log(kError, "Error while ProcessEvent: %s at %s:%d\n",
                        err.Name(), err.File(), err.Line());
                }
            }
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
