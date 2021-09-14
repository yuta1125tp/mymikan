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
     * @param dst_pos コピー先の左上
     * @param src コピー元のフレームバッファ
     * @param src_area コピー元のフレームバッファ上でのコピー対象のエリア
     * @return Error 
     */
    Error Copy(Vector2D<int> dst_pos, const FrameBuffer &src, const Rectangle<int> &src_area);

    /**
     * @brief フレームバッファ内の平面領域を移動
     * 
     * @param dst_pos 移動先の左上
     * @param src 移動元の領域
     */
    void Move(Vector2D<int> dst_pos, const Rectangle<int> &src);

    FrameBufferWriter &Writer() { return *writer_; }

private:
    /** @brief 描画領域の縦横サイズピクセルのデータ形式など描画領域に関する構成情報を保持する */
    FrameBufferConfig config_{};
    /** @brief ピクセルの配列描画領域の本体。ピクセルデータ形式は機種によって様々だからWindows::data_とはことなりuint8_tの配列で持つ */
    std::vector<uint8_t> buffer_{};
    /** @brief この描画領域と関連付けたPixelWriterのインスタンス writer_が指すインスタンスの所有権はFrameBufferが持つ。*/
    std::unique_ptr<FrameBufferWriter> writer_{};
};
