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
