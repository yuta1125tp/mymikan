#pragma once
#include "frame_buffer_config.hpp"

struct PixelColor
{
    uint8_t r, g, b;
};

/**
 * @brief ピクセル描画のクラス
 * 
 */
class PixelWriter
{
public:
    PixelWriter(const FrameBufferConfig &config) : config_{config}
    {
    }
    // = defaultは、「暗黙定義されるデフォルトの挙動を使用し、inlineやvirtualといった修飾のみを明示的に指定する」という目的に使用する機能である[ref](https://cpprefjp.github.io/lang/cpp11/defaulted_and_deleted_functions.html)
    virtual ~PixelWriter() = default;
    // =0は純粋仮想関数
    virtual void Write(int x, int y, const PixelColor &c) = 0;

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
class RGBResv8BitPerColorPixelWriter : public PixelWriter
{
public:
    // 親クラスのコンストラクタを子クラスのコンストラクタとして利用する
    using PixelWriter::PixelWriter;
    virtual void Write(int x, int y, const PixelColor &c) override;
};

/**
 * @brief BGRのためのPixelWriter
 * 
 */
class BGRResv8BitPerColorPixelWriter : public PixelWriter
{
public:
    // 親クラスのコンストラクタを子クラスのコンストラクタとして利用する
    using PixelWriter::PixelWriter;
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
