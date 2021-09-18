#pragma once

#include <algorithm>

#include "frame_buffer_config.hpp"

struct PixelColor
{
    uint8_t r, g, b;
};

inline bool operator==(const PixelColor &lhs, const PixelColor &rhs)
{
    return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b;
}

inline bool operator!=(const PixelColor &lhs, const PixelColor &rhs)
{
    return !(lhs == rhs);
}

#pragma region geo2d
template <typename T>
struct Vector2D
{
    T x, y;

    template <typename U>
    Vector2D<T> &operator+=(const Vector2D<U> &rhs)
    {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    template <typename U>
    Vector2D<T> operator+(const Vector2D<U> &rhs) const
    {
        auto tmp = *this;
        tmp += rhs;
        return tmp;
    }

    template <typename U>
    Vector2D<T> &operator-=(const Vector2D<U> &rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    template <typename U>
    Vector2D<T> operator-(const Vector2D<U> &rhs) const
    {
        auto tmp = *this;
        tmp -= rhs;
        return tmp;
    }
};

template <typename T, typename U>
auto operator+(const Vector2D<T> &lhs, const Vector2D<U> &rhs)
    -> Vector2D<decltype(lhs.x + rhs.x)>
{
    return {lhs.x + rhs.x, lhs.y + rhs.y};
}

template <typename T>
Vector2D<T> ElementMax(const Vector2D<T> &lhs, const Vector2D<T> &rhs)
{
    return {std::max(lhs.x, rhs.x), std::max(lhs.y, rhs.y)};
}

template <typename T>
Vector2D<T> ElementMin(const Vector2D<T> &lhs, const Vector2D<T> &rhs)
{
    return {std::min(lhs.x, rhs.x), std::min(lhs.y, rhs.y)};
}

/**
 * @brief 直方体を示す構造体
 * 
 * @tparam T 
 */
template <typename T>
struct Rectangle
{
    Vector2D<T> pos, size;
};

/**
 * @brief intersection of two rectangles
 * 2つの矩形が重ならない場合は面積0の矩形を返す。
 * 
 * @tparam T 
 * @tparam U 
 * @param lhs 
 * @param rhs 
 * @return Rectangle<T> 
 */
template <typename T, typename U>
Rectangle<T> operator&(const Rectangle<T> &lhs, const Rectangle<U> &rhs)
{
    const auto lhs_end = lhs.pos + lhs.size;
    const auto rhs_end = rhs.pos + rhs.size;

    if (lhs_end.x < rhs.pos.x || lhs_end.y < rhs.pos.y || rhs_end.x < lhs.pos.x || rhs_end.y < lhs.pos.y)
    {
        // ２つの矩形が重ならない場合は面積0の矩形を返す。
        return {{0, 0}, {0, 0}};
    }

    auto new_pos = ElementMax(lhs.pos, rhs.pos);
    auto new_size = ElementMin(lhs_end, rhs_end) - new_pos;
    return {new_pos, new_size};
}

#pragma endregion

/**
 * @brief ピクセル描画のクラス
 *
 */
class PixelWriter
{
public:
    // = defaultは、「暗黙定義されるデフォルトの挙動を使用し、inlineやvirtualといった修飾のみを明示的に指定する」という目的に使用する機能である[ref](https://cpprefjp.github.io/lang/cpp11/defaulted_and_deleted_functions.html)
    virtual ~PixelWriter() = default;
    // =0は純粋仮想関数
    virtual void Write(Vector2D<int> pos, const PixelColor &c) = 0;
    virtual int Height() const = 0;
    virtual int Width() const = 0;
};

/**
 * @brief PixelWriterのうちconfigを受けるもの？day09aで細分化された子クラス
 * 
 */
class FrameBufferWriter : public PixelWriter
{
public:
    FrameBufferWriter(const FrameBufferConfig &config) : config_{config}
    {
    }
    virtual ~FrameBufferWriter() = default;
    virtual int Height() const override { return config_.vertical_resolution; }
    virtual int Width() const override { return config_.horizontal_resolution; }

protected:
    uint8_t *PixelAt(Vector2D<int> pos)
    {
        return config_.frame_buffer + 4 * (config_.pixels_per_scan_line * pos.y + pos.x);
    }

private:
    const FrameBufferConfig &config_;
};

/**
 * @brief RGBのためのPixelWriter
 *
 */
class RGBResv8BitPerColorPixelWriter : public FrameBufferWriter
{
public:
    // 親クラスのコンストラクタを子クラスのコンストラクタとして利用する
    using FrameBufferWriter::FrameBufferWriter;
    virtual void Write(Vector2D<int> pos, const PixelColor &c) override;
};

/**
 * @brief BGRのためのPixelWriter
 *
 */
class BGRResv8BitPerColorPixelWriter : public FrameBufferWriter
{
public:
    // 親クラスのコンストラクタを子クラスのコンストラクタとして利用する
    using FrameBufferWriter::FrameBufferWriter;
    virtual void Write(Vector2D<int> pos, const PixelColor &c) override;
};

void DrawRectangle(
    PixelWriter &writer, const Vector2D<int> &pos,
    const Vector2D<int> &size, const PixelColor &c);
void FillRectangle(
    PixelWriter &writer, const Vector2D<int> &pos,
    const Vector2D<int> &size, const PixelColor &c);

const PixelColor kDesktopBGColor{45, 118, 237};
const PixelColor kDesktopFGColor{255, 255, 255};

void DrawDesktop(PixelWriter &writer);

extern FrameBufferConfig screen_config;
extern PixelWriter *screen_writer;
Vector2D<int> ScreenSize();
void InitializeGraphics(const FrameBufferConfig &screen_config);
