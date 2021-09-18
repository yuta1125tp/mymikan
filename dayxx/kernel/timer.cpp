#include "timer.hpp"
#include "interrupt.hpp"

namespace
{
    // 定数
    const uint32_t kCountMax = 0xffffffffu;

    // Local APICタイマのレジスタ
    // みかん本の227pの表9.1

    /** @brief 割り込みの発生方法の設定など LVT Timerレジスタのフィールド定義はみかん本の表9.3*/
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
    divide_config = 0b1011; // 1対1で分周する設定
    // lvt_timer = (0b001 << 16) | 32; // 16bitの位置に書き込んで割り込み不許可にする（みかん本227pと表9.3）
    lvt_timer = (0b010 << 16) | InterruptVector::kLAPICTimer; // 17bitの位置に書き込んで周期モード、16が0なので割り込み許可、0-7のbit（割り込みベクタ番号）にkLAPICTimerを登録 // みかん本271p
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