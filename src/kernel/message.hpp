/**
 * @brief 割り込みハンドラからメイン関数に対して送信するメッセージ
 * 
 */

#pragma once

struct Message
{
    enum Type
    {
        kInterruptXHCI,
        kInterruptLAPICTimer,
        kTimerTimeout,
        kKeyPush,
    } type;

    /**
     * @brief 通知に関する値を保持するメンバ変数
     * 
     */
    union
    {
        struct
        {
            unsigned long timeout;
            int value;
        } timer;
        struct
        {
            uint8_t keycode;
            char ascii;
        } keyboard;

    } arg;
};
