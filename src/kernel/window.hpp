/**
 * @file window.hpp
 * @brief 表示領域を表すWindowクラスを提供する。
 * 
 */

#pragma once

#include <vector>
#include <optional>

#include "graphics.hpp"
#include "frame_buffer.hpp"

/**
 * @brief Windowクラスはグラフィックの表示領域を表す。
 * タイトルやメニューがあるウインドウだけではなく、マウスカーソルの表示領域なども対象とする。 
 * 
 */
class Window
{
public:
    class WindowWriter : public PixelWriter
    {
    public:
        /**
         * @brief 与えたWindowと関連付けられたPixelWriterを提供する
         * 
         * @param window 
         */
        WindowWriter(Window &window) : window_{window} {}
        /**
         * @brief 指定した位置に指定した色を描く
         * 
         * @param x 
         * @param y 
         * @param c 
         */
        virtual void Write(Vector2D<int> pos, const PixelColor &c) override
        {
            window_.Write(pos, c);
        }

        /**
         * @brief 関連付けられたWindowの幅をピクセル単位で返す
         * 
         * @return int 
         */
        virtual int Width() const override { return window_.Width(); }
        /**
         * @brief 関連付けられたWindowの高さをピクセル単位で返す
         * 
         * @return int 
         */
        virtual int Height() const override { return window_.Height(); }

    private:
        Window &window_;
    };

    /**
     * @brief 指定されたピクセル数の平面描画領域とシャドウバッファを作成する
     * シャドウバッファに関してはみかん本232p
     * 
     *@param width
     *@param height
     *@param shadow_format
     */
    Window(int width, int height, PixelFormat shadow_format);
    ~Window() = default;
    Window(const Window &rhs) = delete;
    Window &operator=(const Window &rhs) = delete;

    /**
     * @brief 与えられたFrameBufferにこのウインドウの領域を描画する
     * 
     * @param dst 描画先
     * @param pos dstの左上を基準としたウインドウの位置
     * @param area dstの左上を基準とした描画対象の範囲
     */
    void DrawTo(FrameBuffer &dst, Vector2D<int> pos, const Rectangle<int> &area);
    /**
     * @brief 透過色を設定する。
     * 
     * ここに与えた色たとえばPixelColor{1,1,1}*とすると、{1,1,1}はこのWriterにとって透過色になる。
     * 
     * @param c: 透過色に設定する色
     */
    void SetTransparentColor(std::optional<PixelColor> c);
    /** @brief このインスタンスに紐付いた WindowWriter を取得する。 */
    WindowWriter *Writer();

    /**
     * @brief 指定した位置のピクセルを返す。
     * 
     * @param x 
     * @param y 
     * @return const PixelColor& 
     */
    const PixelColor &At(Vector2D<int> pos) const;

    /**
     * @brief 指定した位置にピクセルを書き込む
     * 
     * @param pos 
     * @param c 
     */
    void Write(Vector2D<int> pos, PixelColor c);

    /**
     * @brief 平面描画領域の幅をピクセル単位で返す
     * 
     * @return int 
     */
    int Width() const;
    /**
     * @brief 平面描画領域の高さをピクセル単位で返す
     * 
     * @return int 
     */
    int Height() const;

    /**
     * @brief 平面描画領域のサイズをピクセル単位で返す
     * 
     * @return Vector2D<int> 
     */
    Vector2D<int> Size() const;
    /**
     * @brief このウインドウの平面描画領域内で矩形領域を移動する。
     * 
     * @param dst_pos 移動先の左上
     * @param src 移動領域を示すRectangle
     */
    void Move(Vector2D<int> dst_pos, const Rectangle<int> &src);

private:
    int width_, height_;
    std::vector<std::vector<PixelColor>> data_{};
    // windowインスタンスのポインタがwriter_のコンストラクタに渡る。[みかん本の213pと図9.7]
    WindowWriter writer_{*this};
    std::optional<PixelColor> transparent_color_{std::nullopt};

    // 重ね合わせ処理の高速化のためのシャドウバッファ(VRAM)
    FrameBuffer shadow_buffer_{};
};

void DrawWindow(PixelWriter &writer, const char *title);
void DrawTextbox(PixelWriter &writer, Vector2D<int> pos, Vector2D<int> size);
