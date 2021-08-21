/**
 * @file main.cpp
 * @brief カーネル本体のプログラムを書いたファイル。
 */

#include <cstdint>
#include <cstddef>
#include <cstdio>

#include "frame_buffer_config.hpp"
#include "graphics.hpp"
#include "font.hpp"
#include "console.hpp"

#pragma region 配置new
/**
 * @brief 配置newの実装、include<new>でもOK、[ref](みかん本@105p)
 * 
 * @param size 
 * @param buf 
 * @return void* 
 */
void *operator new(size_t size, void *buf)
{
    return buf;
}

void operator delete(void *obj) noexcept
{
}
#pragma endregion

const PixelColor kDesktopBGColor{45, 118, 237};
const PixelColor kDesktopFGColor{255, 255, 255};

#pragma region マウスカーソル
const int kMouseCursorWidth = 15;
const int kMouseCursorHeight = 24;
const char mouse_cursor_shape[kMouseCursorHeight][kMouseCursorWidth + 1] = {
    "@              ",
    "@@             ",
    "@.@            ",
    "@..@           ",
    "@...@          ",
    "@....@         ",
    "@.....@        ",
    "@......@       ",
    "@.......@      ",
    "@........@     ",
    "@.........@    ",
    "@..........@   ",
    "@...........@  ",
    "@............@ ",
    "@......@@@@@@@@",
    "@......@       ",
    "@....@@.@      ",
    "@...@ @.@      ",
    "@..@   @.@     ",
    "@.@    @.@     ",
    "@@      @.@    ",
    "@       @.@    ",
    "         @.@   ",
    "         @@@   ",
};
#pragma endregion

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
    console = new (console) Console{
        *pixel_writer,
        kDesktopFGColor,
        kDesktopBGColor};

    printk("Welcome to MikanOS!!\n");

    for (size_t dy = 0; dy < kMouseCursorHeight; dy++)
    {
        for (size_t dx = 0; dx < kMouseCursorWidth; dx++)
        {
            if (mouse_cursor_shape[dy][dx] == '@')
            {
                pixel_writer->Write(200 + dx, 100 + dy, {0, 0, 0});
            }
            else if (mouse_cursor_shape[dy][dx] == '.')
            {
                pixel_writer->Write(200 + dx, 100 + dy, {255, 255, 255});
            }
        }
    }

    while (1)
        __asm__("hlt");
}
