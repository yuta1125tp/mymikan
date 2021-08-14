/**
 * @file main.cpp
 * @brief カーネル本体のプログラムを書いたファイル。
 */

#include <cstdint>
#include <cstddef>

#include "frame_buffer_config.hpp"

const uint8_t kFontA[16] = {
    0b00000000, //
    0b00011000, //    **
    0b00011000, //    **
    0b00011000, //    **
    0b00011000, //    **
    0b00100100, //   *  *
    0b00100100, //   *  *
    0b00100100, //   *  *
    0b00100100, //   *  *
    0b01111110, //  ******
    0b01000010, //  *    *
    0b01000010, //  *    *
    0b01000010, //  *    *
    0b11100111, // ***  ***
    0b00000000, //
    0b00000000, //
};

struct PixelColor
{
    uint8_t r, g, b;
};

/**
 * @brief ピクセル描画のクラス
 * 
 */
class PixelWriter
{
public:
    PixelWriter(const FrameBufferConfig &config) : config_{config}
    {
    }
    // = defaultは、「暗黙定義されるデフォルトの挙動を使用し、inlineやvirtualといった修飾のみを明示的に指定する」という目的に使用する機能である[ref](https://cpprefjp.github.io/lang/cpp11/defaulted_and_deleted_functions.html)
    virtual ~PixelWriter() = default;
    // =0は純粋仮想関数
    virtual void Write(int x, int y, const PixelColor &c) = 0;

protected:
    uint8_t *pixelAt(int x, int y)
    {
        return config_.frame_buffer + 4 * (config_.pixels_per_scan_line * y + x);
    }

private:
    const FrameBufferConfig &config_;
};

/**
 * @brief RGBのためのPixelWriter
 * 
 */
class RGBResv8BitPerColorPixelWriter : public PixelWriter
{
public:
    // 親クラスのコンストラクタを子クラスのコンストラクタとして利用する
    using PixelWriter::PixelWriter;
    virtual void Write(int x, int y, const PixelColor &c) override
    {
        auto p = pixelAt(x, y);
        p[0] = c.r;
        p[1] = c.g;
        p[2] = c.b;
    }
};

/**
 * @brief BGRのためのPixelWriter
 * 
 */
class BGRResv8BitPerColorPixelWriter : public PixelWriter
{
public:
    // 親クラスのコンストラクタを子クラスのコンストラクタとして利用する
    using PixelWriter::PixelWriter;
    virtual void Write(int x, int y, const PixelColor &c) override
    {
        auto p = pixelAt(x, y);
        p[0] = c.b;
        p[1] = c.g;
        p[2] = c.r;
    }
};

/**
 * @brief Asciiコードに従ってPixelWriterで指定した座標に文字を表示ｄｓ
 * 
 * @param writer 
 * @param x 
 * @param y 
 * @param c 
 * @param color 
 */
void WriteAscii(PixelWriter &writer, int x, int y, char c, const PixelColor &color)
{
    if (c != 'A')
    {
        return;
    }
    for (int dy = 0; dy < 16; dy++)
    {
        for (int dx = 0; dx < 8; dx++)
        {
            if ((kFontA[dy] << dx) & 0x80u)
            {
                writer.Write(x + dx, y + dy, color);
            }
        }
    }
}

#pragma region 配置new
// 配置newの実装、include<new>でもOK、[ref](みかん本@105p)
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

    WriteAscii(*pixel_writer, 50, 50, 'A', {0, 0, 0});
    WriteAscii(*pixel_writer, 58, 50, 'A', {0, 128, 0});

    while (1)
        __asm__("hlt");
}
