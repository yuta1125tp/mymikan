/**
 * @file window.hpp
 * @brief 表示領域を表すWindowクラスを提供する。
 * 
 */

#pragma once

#include <vector>
#include <optional>

#include "graphics.hpp"

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
        virtual void Write(int x, int y, const PixelColor &c) override
        {
            window_.At(x, y) = c;
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
     * @brief Construct a new Window object with given size
     * 
     * @param width 
     * @param height 
     */
    Window(int width, int height);
    ~Window() = default;
    Window(const Window &rhs) = delete;
    Window &operator=(const Window &rhs) = delete;

    /**
     * @brief 与えられたPixelWriterにこのウインドウの領域を描画する
     * 
     * @param witer 
     * @param position 
     */
    void DrawTo(PixelWriter &witer, Vector2D<int> position);
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
    PixelColor &At(int x, int y);
    /**
     * @brief 指定した位置のピクセルを返す。
     * 
     * @param x 
     * @param y 
     * @return const PixelColor& 
     */
    const PixelColor &At(int x, int y) const;

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

private:
    int width_, height_;
    std::vector<std::vector<PixelColor>> data_{};
    // windowインスタンスのポインタがwriter_のコンストラクタに渡る。[みかん本の213pと図9.7]
    WindowWriter writer_{*this};
    std::optional<PixelColor> transparent_color_{std::nullopt};
};
