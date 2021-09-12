#include "timer.hpp"
namespace
{
    // 定数
    const uint32_t kCountMax = 0xffffffffu;

    // Local APICタイマのレジスタ
    // みかん本の227pの表9.1

    /** @brief 割り込みの発生方法の設定など*/
    volatile uint32_t &lvt_timer = *reinterpret_cast<uint32_t *>(0xfee00320);
    /** @brief カウンタの初期値*/
    volatile uint32_t &initial_count = *reinterpret_cast<uint32_t *>(0xfee00380);
    /** @brief カウンタの現在値*/
    volatile uint32_t &current_count = *reinterpret_cast<uint32_t *>(0xfee00390);
    /** @brief カウンタの減少スピードの設定*/
    volatile uint32_t &divide_config = *reinterpret_cast<uint32_t *>(0xfee003e0);
} // namespace

void InitializeLAPICTimer()
{
    divide_config = 0b1011;         // 1対1で分周する設定
    lvt_timer = (0b001 << 16) | 32; // 16bitの位置に書き込んで割り込み不許可にする（みかん本227pと表9.3）
}

void StartLAPICTimer()
{
    initial_count = kCountMax;
}

uint32_t LAPICTimerElapsed()
{
    return kCountMax - current_count;
}

void StopLAPICTimer()
{
    initial_count = 0;
}