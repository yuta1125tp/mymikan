/**
 * @file interrupt.cpp
 * @brief 割り込み用のプログラム
 * 
 */

#include "interrupt.hpp"
#include "asmfunc.h"
#include "segment.hpp"
#include "timer.hpp"

std::array<InterruptDescriptor, 256> idt;

void SetIDTEntry(InterruptDescriptor &desc, InterruptDescriptorAttribute attr, uint64_t offset, uint16_t segment_selector)
{
    desc.attr = attr;
    desc.offset_low = offset & 0xffffu;
    desc.offset_middle = (offset >> 16) & 0xffffu;
    desc.offset_height = offset >> 32;
    desc.segment_selector = segment_selector;
}

/**
 * @brief End Of Interruptレジスタ（0xfee000b0番地）に値を書き込み、割り込み終了をCPUに伝える。
 * 
 */
void NotifyEndOfInterrupt()
{
    // 最適化で省略されないようにvolatile修飾子が必要
    volatile auto end_of_interrupt = reinterpret_cast<uint32_t *>(0xfee000b0);
    *end_of_interrupt = 0;
}

namespace
{
    // メイン関数に割り込み発生を通知するためのキュー
    std::deque<Message> *msg_queue;

    __attribute__((interrupt)) void IntHandlerXHCI(InterruptFrame *frame)
    {
        // キューを介してメイン関数に割り込み発生を通知する
        msg_queue->push_back(Message{Message::kInterruptXHCI});
        NotifyEndOfInterrupt();
    }

    __attribute__((interrupt)) void IntHandlerLAPICTimer(InterruptFrame *frame)
    {
        // msg_queue->push_back(Message{Message::kInterruptLAPICTimer});
        LAPICTimerOnInterrupt();
    }
}

void InitializeInterrupt(std::deque<Message> *msg_queue)
{
    // 無名名前空間のmsg_queueにメイン関数で生成したキューのポインタをコピー
    ::msg_queue = msg_queue;

    // idtentryの追加
    SetIDTEntry(
        idt[InterruptVector::kXHCI],
        MakeIDTAttr(DescriptorType::kInterruptGate, 0),
        reinterpret_cast<uint64_t>(IntHandlerXHCI),
        kKernelCS);
    SetIDTEntry(
        idt[InterruptVector::kLAPICTimer],
        MakeIDTAttr(DescriptorType::kInterruptGate, 0),
        reinterpret_cast<uint64_t>(IntHandlerLAPICTimer),
        kKernelCS);

    //
    LoadIDT(sizeof(idt) - 1, reinterpret_cast<uintptr_t>(&idt[0]));
}
