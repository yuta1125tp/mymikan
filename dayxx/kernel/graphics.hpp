#pragma once
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
    virtual void Write(int x, int y, const PixelColor &c) = 0;
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
    virtual int Height() const override { return config_.horizontal_resolution; }
    virtual int Width() const override { return config_.vertical_resolution; }

protected:
    uint8_t *pixelAt(int x, int y)
    {
        return config_.frame_buffer + 4 * (config_.pixels_per_scan_line * y + x);
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
    virtual void Write(int x, int y, const PixelColor &c) override;
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
    virtual void Write(int x, int y, const PixelColor &c) override;
};

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
