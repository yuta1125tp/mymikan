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
    } type;
};
