/**
 * @file font.cpp
 * @brief フォント描画のプログラムを集めたファイル。
 * @date 2021-08-14
 * 
 */

#include "font.hpp"

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
