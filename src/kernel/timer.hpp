/**
 * @file timer.hpp
 * @brief 時間計測関連
 * 
 */

#pragma once

#include <cstdint>

/**
 * @brief Local APICタイマの周期を分周する回路の設定をする関数
 * ※分周＝クロックをn分の1にすること みかん本227p
 * 
 */
void InitializeLAPICTimer();
void StartLAPICTimer();
uint32_t LAPICTimerElapsed();
void StopLAPICTimer();

/**
 * @brief タイマの割り込み回数を数える
 * 
 */
class TimerManager
{
public:
    /** @brief 割り込み回数を数え上げる*/
    void Tick();
    /** @brief 現在の累計割り込み回数を返す*/
    unsigned long CurrentTick() const { return tick_; }

private:
    // tick_は割り込みハンドラの中で変更され、割子お見ハンドラの外から参照されるので、コンパイラが最適化のために定数にする可能性がある。
    // volatileキーワードで揮発性変数（値がいつでも変化する可能性がある）であることを伝え最適化対象から除外するみかん本のコラム11.1
    volatile unsigned long tick_{0};
};

extern TimerManager *timer_manager;

void LAPICTimerOnInterrupt();
