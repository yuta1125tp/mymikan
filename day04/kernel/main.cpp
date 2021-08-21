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

    for (int y = 0; y < frame_buffer_config.vertical_resolution; y++)
    {
        for (int x = 0; x < frame_buffer_config.horizontal_resolution; x++)
        {
            pixel_writer->Write(x, y, {255, 255, 255});
        }
    }

    for (int y = 0; y < 100; y++)
    {
        for (int x = 0; x < 200; x++)
        {
            pixel_writer->Write(100 + x, 100 + y, {0, 255, 0});
        }
    }

    console = new (console) Console{*pixel_writer, {0, 0, 0}, {255, 255, 255}};

    for (int i = 0; i < 27; i++)
    {
        printk("printk: %d\n", i);
    }

    while (1)
        __asm__("hlt");
}
