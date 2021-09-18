#pragma once

/**
 * @brief 割り込みハンドラからメイン関数に対して送信するメッセージ
 * 
 */
struct Message
{
    enum Type
    {
        kInterruptXHCI,
    } type;
};
