/**
 * @file main.cpp
 * @brief カーネル本体のプログラムを書いたファイル。
 */

#include <cstdint>
#include <cstddef>
#include <cstdio>

#include <array>
#include <numeric>
#include <vector>
#include <deque>
#include <limits>
#include <string.h> // for memset

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
#include "timer.hpp"
#include "acpi.hpp"
#include "keyboard.hpp"

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

    main_window_layer_id = layer_manager->NewLayer()
                               .SetWindow(main_window)
                               .SetDraggable(true)
                               .Move({300, 100})
                               .ID();

    layer_manager->UpDown(main_window_layer_id, std::numeric_limits<int>::max());
}

std::shared_ptr<Window> text_window;
unsigned int text_window_layer_id;
void InitializeTextWindow()
{
    const int win_w = 160;
    const int win_h = 52;

    text_window = std::make_shared<Window>(win_w, win_h, screen_config.pixel_format);
    DrawWindow(*text_window->Writer(), "TextBox Test");
    DrawTextbox(*text_window->Writer(), {4, 24}, {win_w - 8, win_h - 24 - 4});

    text_window_layer_id = layer_manager->NewLayer().SetWindow(text_window).SetDraggable(true).Move({350, 200}).ID();

    layer_manager->UpDown(text_window_layer_id, std::numeric_limits<int>::max());
}

int text_window_index;

void DrawTextCursor(bool visible)
{
    const auto color = visible ? ToColor(0) : ToColor(0xffffff);
    const auto pos = Vector2D<int>{8 + 8 * text_window_index, 24 + 5};
    FillRectangle(*text_window->Writer(), pos, {7, 15}, color);
}

void InputTextWindow(char c)
{
    if (c == 0)
    {
        return;
    }

    auto pos = []()
    {
        return Vector2D<int>{8 + 8 * text_window_index, 24 + 6};
    };

    const int max_chars = (text_window->Width() - 16) / 8;
    if (c == '\b' & text_window_index > 0)
    {
        // \b:バックスペースで、１文字以上表示されている場合は最後の文字を消す（塗りつぶす）。
        DrawTextCursor(false);
        --text_window_index;
        FillRectangle(*text_window->Writer(), pos(), {8, 16}, ToColor(0xffffff));
        DrawTextCursor(true);
    }
    else if (c >= ' ' && text_window_index < max_chars)
    {
        DrawTextCursor(false);
        // 空きがあるなら文字を表示
        WriteAscii(*text_window->Writer(), pos(), c, ToColor(0));
        ++text_window_index;
        DrawTextCursor(true);
    }

    layer_manager->Draw(text_window_layer_id);
}

std::shared_ptr<Window> task_b_window;
unsigned int task_b_window_layer_id;
void InitializeTaskBWindow()
{
    task_b_window = std::make_shared<Window>(160, 52, screen_config.pixel_format);
    DrawWindow(*task_b_window->Writer(), "TaskB Window");

    task_b_window_layer_id = layer_manager->NewLayer().SetWindow(task_b_window).SetDraggable(true).Move({100, 100}).ID();

    layer_manager->UpDown(task_b_window_layer_id, std::numeric_limits<int>::max());
}

/**
 * @brief タスクのコンテキストを保存するための構造体
 * 
 */
struct TaskContext
{
    uint64_t cr3, rip, rflags, reserved1;            //offset 0x00
    uint64_t cs, ss, fs, gs;                         //offset 0x20
    uint64_t rax, rbx, rcx, rdx, rdi, rsi, rsp, rbp; //offset 0x40
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;   //offset 0x80
    std::array<uint8_t, 512> fxsave_area;            // offset 0xc0
} __attribute__((packed));

alignas(16) TaskContext task_b_ctx, task_a_ctx;

void TaskB(int task_id, int data)
{
    printk("taskB: task_id=%d, data=%d\n", task_id, data);
    char str[128];
    int count = 0;
    while (true)
    {
        ++count;
        sprintf(str, "%010d", count);
        FillRectangle(*task_b_window->Writer(), {24, 28}, {8 * 10, 16}, {0xc6, 0xc6, 0xc6});
        WriteString(*task_b_window->Writer(), {24, 28}, str, {0, 0, 0});
        layer_manager->Draw(task_b_window_layer_id);

        SwitchContext(&task_a_ctx, &task_b_ctx);
    }
}

std::deque<Message> *main_queue;

// 新しいスタック領域（UEFI管理ではなく、OS管理の領域、[ref](みかん本の186p)）
alignas(16) uint8_t kernel_main_stack[1024 * 1024];

/**
 * @brief カーネル関数
 * 
 */
extern "C" void KernelMainNewStack(
    const FrameBufferConfig &frame_buffer_config_ref,
    const MemoryMap &memory_map_ref,
    const acpi::RSDP &acpi_table)
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
    InitializeTextWindow();
    InitializeTaskBWindow();
    InitializeMouse();
    layer_manager->Draw({{0, 0}, ScreenSize()});

    acpi::Initialize(acpi_table);
    InitializeLAPICTimer(*main_queue);

    InitializeKeyboard(*main_queue);

    const int kTextboxCursorTimer = 1;
    const int kTimer05sec = static_cast<int>(kTimerFreq * 0.5);
    __asm__("cli");
    timer_manager->AddTimer(Timer{kTimer05sec, kTextboxCursorTimer});
    __asm__("sti");
    bool textbox_cursor_visible = false;

    std::vector<uint64_t> task_b_stack(1024); //64[bit]*1024/8[bit/byte]=8192[byte]=8[kbyte]
    uint64_t task_b_stack_end = reinterpret_cast<uint64_t>(&task_b_stack[1024]);

    memset(&task_b_ctx, 0, sizeof(task_b_ctx));
    task_b_ctx.rip = reinterpret_cast<uint64_t>(TaskB); // RIPにTaskBの先頭のアドレス
    task_b_ctx.rdi = 1;                                 // TaskBの第1引数
    task_b_ctx.rsi = 42;                                // TaskBの第2引数
    task_b_ctx.cr3 = GetCR3();                          // CR3にはPML4テーブルのアドレスが設定されている。→タスクB実行中も同じPML4テーブルを参照することになる。
    task_b_ctx.rflags = 0x202;                          // 割り込み許可のフラグみかん本315p
    task_b_ctx.cs = kKernelCS;                          // メインタスクと同じCS
    task_b_ctx.ss = kKernelSS;                          // 同じSS
    // アドレス値の下位4ビットを切り捨てて16の倍数に調整し、8引いて下位4ビットを8にしている。
    // 16バイトアライメントなのに、8引いている理由はcall命令後の状態に偽装するため。
    // みかん本のコラム13.1
    task_b_ctx.rsp = (task_b_stack_end & ~0xflu) - 8;
    // MXCSRのすべての例外をマスクする みかん本315p
    *reinterpret_cast<uint32_t *>(&task_b_ctx.fxsave_area[24]) = 0x1f80;

    char str[128];

    Log(kInfo, "start event loop.\n");
    // イベントループ
    // キューに溜まったイベントを処理し続ける
    while (1)
    {
        __asm__("cli");
        const auto tick = timer_manager->CurrentTick();
        __asm__("sti");

        sprintf(str, "%010lu", tick);
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
            // __asm__("sti");
            __asm__("sti");
            SwitchContext(&task_b_ctx, &task_a_ctx);
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
        case Message::kTimerTimeout:
            if (msg.arg.timer.value == kTextboxCursorTimer)
            {
                __asm__("cli");
                timer_manager->AddTimer(Timer{msg.arg.timer.timeout + kTimer05sec, kTextboxCursorTimer});
                __asm__("sti");
                textbox_cursor_visible = !textbox_cursor_visible;
                DrawTextCursor(textbox_cursor_visible);
                layer_manager->Draw(text_window_layer_id);
            }
            break;
        case Message::kKeyPush:
            InputTextWindow(msg.arg.keyboard.ascii);
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
