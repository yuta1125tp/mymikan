/**
 * @file frame_buffer.hpp
 * @brief フレームバッファに関するファイル
 * 
 */
#pragma once

#include <vector>
#include <memory>

#include "frame_buffer_config.hpp"
#include "graphics.hpp"
#include "error.hpp"

/**
 * @brief フレームバッファ[みかん本234p]
 * 
 */
class FrameBuffer
{
public:
    /**
     * @brief フレームバッファの初期化
     * 
     * config.frame_bufferがnullptrの場合は内部で自動でメモリを確保する
     * 
     * @param config
     * @return Error 
     */
    Error Initialize(const FrameBufferConfig &config);

    /**
     * @brief バッファ同士をコピーするメソッド
     * PixelWriter()で1ピクセルごとにデータ変換を繰り返すのではなく、バッファをmemcpyすることで描画するための中核[みかん本237]
     * バッファsrcの内容をthisの位置posへコピーする[みかん本9.14]
     * 
     * @param pos
     * @param src
     * @return Error 
     */
    Error Copy(Vector2D<int> pos, const FrameBuffer &src);

    FrameBufferWriter &Writer() { return *writer_; }

private:
    /** @brief 描画領域の縦横サイズピクセルのデータ形式など描画領域に関する構成情報を保持する */
    FrameBufferConfig config_{};
    /** @brief ピクセルの配列描画領域の本体。ピクセルデータ形式は機種によって様々だからWindows::data_とはことなりuint8_tの配列で持つ */
    std::vector<uint8_t> buffer_{};
    /** @brief この描画領域と関連付けたPixelWriterのインスタンス writer_が指すインスタンスの所有権はFrameBufferが持つ。*/
    std::unique_ptr<FrameBufferWriter> writer_{};
};

/**
 * @brief pixelあたりのビット数を返す。未知のフォーマットの場合は-1を返す
 * 
 * @param format 
 * @return int 
 */
int BitsPerPixel(PixelFormat format);
