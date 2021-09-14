#pragma once

#include <stdint.h>

enum PixelFormat
{
    kPixelRGBResv8BitPerColor,
    kPixelBGRResv8BitPerColor,
    // kPixelBitMask,
    // kPixelBltOnly,
};

/**
 * @brief 描画領域の縦横サイズピクセルのデータ形式など描画領域に関する構成情報を保持する
 * 
 */
struct FrameBufferConfig
{
    uint8_t *frame_buffer;
    uint32_t pixels_per_scan_line;
    uint32_t horizontal_resolution;
    uint32_t vertical_resolution;
    enum PixelFormat pixel_format;
};
