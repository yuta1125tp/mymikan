/**
 * @file graphics.cpp
 * @brief 画像描画関連のプログラムを集めたファイル。
 * @date 2021-08-14
 * 
 */

#include "graphics.hpp"

void RGBResv8BitPerColorPixelWriter::Write(int x, int y, const PixelColor &c)
{
    auto p = pixelAt(x, y);
    p[0] = c.r;
    p[1] = c.g;
    p[2] = c.b;
}

void BGRResv8BitPerColorPixelWriter::Write(int x, int y, const PixelColor &c)
{
    auto p = pixelAt(x, y);
    p[0] = c.b;
    p[1] = c.g;
    p[2] = c.r;
}
