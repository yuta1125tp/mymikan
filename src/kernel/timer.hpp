/**
 * @file timer.hpp
 * @brief 時間計測関連
 * 
 */

#pragma once

#include <cstdint>
#include <queue>
#include <vector>
#include "message.hpp"

/**
 * @brief Local APICタイマの周期を分周する回路の設定をする関数
 * ※分周＝クロックをn分の1にすること みかん本227p
 * 
 */
void InitializeLAPICTimer(std::deque<Message> &msg_queue);
void StartLAPICTimer();
uint32_t LAPICTimerElapsed();
void StopLAPICTimer();

/**
 * @brief 論理タイマを表すクラス
 * 
 */
class Timer
{
public:
    Timer(unsigned long timeout, int value);
    unsigned long Timeout() const { return timeout_; }
    int Value() const { return value_; }

private:
    unsigned long timeout_;
    int value_;
};

/**
 * @brief タイマ優先度を比較する。タイムアウトが遠いほど優先度が低い
 * 
 */
inline bool operator<(const Timer &lhs, const Timer &rhs)
{
    return lhs.Timeout() > rhs.Timeout();
}

/**
 * @brief タイマの割り込み回数を数える
 * 
 */
class TimerManager
{
public:
    TimerManager(std::deque<Message> &msg_queue);
    void AddTimer(const Timer &timer);

    /** @brief 割り込み回数を数え上げる*/
    void Tick();
    /** @brief 現在の累計割り込み回数を返す*/
    unsigned long CurrentTick() const { return tick_; }

private:
    // tick_は割り込みハンドラの中で変更され、割子お見ハンドラの外から参照されるので、コンパイラが最適化のために定数にする可能性がある。
    // volatileキーワードで揮発性変数（値がいつでも変化する可能性がある）であることを伝え最適化対象から除外するみかん本のコラム11.1
    volatile unsigned long tick_{0};

    std::priority_queue<Timer> timers_{};
    std::deque<Message> &msg_queue_;
};

extern TimerManager *timer_manager;
/** @brief 1秒あたりのカウント数（TimerManager::Tick()の周波数）を記録するグローバル変数*/
extern unsigned long lapic_timer_freq;
/** @brief 1秒間にTimerManager::Tick()が呼ばれる頻度。秒間100回なら10[msec]に1回tick_が増える */
const int kTimerFreq = 100;

void LAPICTimerOnInterrupt();
