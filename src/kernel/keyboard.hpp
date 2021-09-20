/**
 * @file keyboard.hpp
 * @brief キーボード制御プログラム
 * 
 */

#pragma once

#include <deque>
#include "message.hpp"

void InitializeKeyboard(std::deque<Message> &msg_queue);
