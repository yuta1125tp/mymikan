/**
 * @file font.cpp
 * @brief フォント描画のプログラムを集めたファイル。
 * @date 2021-08-14
 * 
 */

#include "font.hpp"

/**
 * @brief objcopyコマンドでフラットバイナリに"のりしろ"をつけた際に付与される変数。hankaku.oの中で定義されている。
 * 
 */
extern const uint8_t _binary_hankaku_bin_start;
extern const uint8_t _binary_hankaku_bin_end;
extern const uint8_t _binary_hankaku_bin_size;

const uint8_t *GetFont(char c)
{
    auto index = 16 * static_cast<unsigned int>(c);
    if (index >= reinterpret_cast<uintptr_t>(&_binary_hankaku_bin_size))
    {
        return nullptr;
    }
    return &_binary_hankaku_bin_start + index;
}

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
    const uint8_t *font = GetFont(c);
    if (font == nullptr)
    {
        return;
    }
    for (int dy = 0; dy < 16; dy++)
    {
        for (int dx = 0; dx < 8; dx++)
        {
            if ((font[dy] << dx) & 0x80u)
            {
                writer.Write(x + dx, y + dy, color);
            }
        }
    }
}
