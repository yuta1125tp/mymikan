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

// #pragma region 配置new
// // /**
// //  * @brief 配置newの実装、include<new>でもOK、[ref](みかん本@105p)
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

const PixelColor kDesktopBGColor{45, 118, 237};
const PixelColor kDesktopFGColor{255, 255, 255};

// カーネル関数で利用するグローバル変数の定義
char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter *pixel_writer;

char console_buf[sizeof(Console)];
Console *console;

/**
 * @brief カーネル内部からメッセージを出す関数[ref](みかん本@132p)
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

#pragma region // マウスカーソル
char mouse_cursor_buf[sizeof(MouseCursor)];
MouseCursor *mouse_cursor;

void MouseObserver(
    /*uint8_t button,*/
    int8_t displacement_x,
    int8_t displacement_y)
{
    // Log(kDebug, "MouseObserver %d, %d\n", displacement_x, displacement_y);
    mouse_cursor->MoveRelative({displacement_x, displacement_y});
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
/**
 * @brief カーネル関数
 * 
 */
extern "C" void KernelMain(
    const FrameBufferConfig &frame_buffer_config)
{
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

    const int kFrameWidth = frame_buffer_config.horizontal_resolution;
    const int kFrameHeight = frame_buffer_config.vertical_resolution;

    FillRectangle(
        *pixel_writer,
        {0, 0},
        {kFrameWidth, kFrameHeight - 50},
        kDesktopBGColor);
    FillRectangle(
        *pixel_writer,
        {0, kFrameHeight - 50},
        {kFrameWidth, 50},
        {1, 8, 17});
    FillRectangle(
        *pixel_writer,
        {0, kFrameHeight - 50},
        {kFrameWidth / 5, 50},
        {80, 80, 80});
    DrawRectangle(
        *pixel_writer,
        {10, kFrameHeight - 40},
        {30, 30},
        {160, 160, 160});

    // コンストラクタ呼び出しが波括弧なのは、C++11で追加された初期化構文
    //（統一初期化記法、Uniform Initialization）によるものらしい。
    // 同じ文脈で配列の初期化も波括弧でできる。
    // [ref](https://faithandbrave.hateblo.jp/entry/20111221/1324394299)
    console = new (console_buf) Console{
        *pixel_writer,
        kDesktopFGColor,
        kDesktopBGColor};

    printk("Welcome to MikanOS!!\n");
    SetLogLevel(kInfo);

    mouse_cursor = new (mouse_cursor_buf) MouseCursor{
        pixel_writer,
        kDesktopBGColor,
        {300, 200}};

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
    Log(kInfo, "start poling.\n");

    // xHCに溜まったイベントを処理し続ける。
    while (1)
    {
        if (auto err = ProcessEvent(xhc))
        {
            Log(kError, "Error while ProcessEvent: %s at %s:%d\n",
                err.Name(), err.File(), err.Line());
        }
    }

    while (1)
        __asm__("hlt");
}

// add day06c
extern "C" void __cxa_pure_virtual()
{
    while (1)
        __asm__("hlt");
}
