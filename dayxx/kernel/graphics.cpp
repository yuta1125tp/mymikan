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

void DrawRectangle(PixelWriter &writer, const Vecotr2D<int> &pos,
                   const Vecotr2D<int> &size, const PixelColor &c)
{
    for (int dx = 0; dx < size.x; dx++)
    {
        writer.Write(pos.x + dx, pos.y, c);
        writer.Write(pos.x + dx, pos.y + size.y - 1, c);
    }
    for (int dy = 0; dy < size.y; dy++)
    {
        writer.Write(pos.x, pos.y + dy, c);
        writer.Write(pos.x + size.x - 1, pos.y + dy, c);
    }
}

void FillRectangle(PixelWriter &writer, const Vecotr2D<int> &pos,
                   const Vecotr2D<int> &size, const PixelColor &c)
{
    for (int dy = 0; dy < size.y; dy++)
    {
        for (int dx = 0; dx < size.x; dx++)
        {
            writer.Write(pos.x + dx, pos.y + dy, c);
        }
    }
}
