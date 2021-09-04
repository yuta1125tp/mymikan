/**
 * @file interrupt.hpp
 * @brief 割り込み用のプログラム
 * 
 */

#pragma once

#include <array>
#include <cstdint>

#include "x86_descriptor.hpp"

/**
 * @brief 割り込み記述子の属性[ref](みかん本の166p)
 * __attribute__((packed))を指定すると、変数アライメントを守るためのpaddingが抑制されて
 * 構造体の各フィールドをメモリ上で詰めて配置できる。
 */
union InterruptDescriptorAttribute
{
    uint16_t data;
    struct
    {
        //型 フィールド名: ビット幅　← bitフィールドの書き方
        uint16_t interrupt_stack_table : 3;      // みかん本の範囲では常に0 いつ0以外？TODO
        uint16_t : 5;                            //offset
        DescriptorType type : 4;                 // 記述子の種別、設定可能な値は14(Interrupt Gate)と15(Trap Gate)のいずれかのみ
        uint16_t : 1;                            // offset
        uint16_t descriptor_privilege_level : 2; // DPL, 割り込みハンドラの実行権限
        uint16_t present : 1;                    // 記述子が有効であることを示すフラグ
    } __attribute__((packed)) bits;
} __attribute__((packed));

/**
 * @brief 割り込み記述子[ref](みかん本の図7.3@166p)
 * 
 */
struct InterruptDescriptor
{
    uint16_t offset_low;
    uint16_t segment_selector;
    InterruptDescriptorAttribute attr;
    uint16_t offset_middle;
    uint32_t offset_height;
    uint32_t reserved;
} __attribute__((packed));

// IDT(割り込み記述子テーブル, Interrupt Descriptor Table)[ref](みかん本の165p)
extern std::array<InterruptDescriptor, 256> idt;

constexpr InterruptDescriptorAttribute MakeIDTAttr(
    DescriptorType type,
    uint8_t descriptor_privilege_level,
    bool present = true,
    uint8_t interrupt_stack_table = 0)
{
    InterruptDescriptorAttribute attr{};
    attr.bits.interrupt_stack_table = interrupt_stack_table;
    attr.bits.type = type;
    attr.bits.descriptor_privilege_level = descriptor_privilege_level;
    attr.bits.present = present;
    return attr;
}

void SetIDTEntry(InterruptDescriptor &desc, InterruptDescriptorAttribute attr, uint64_t offset, uint16_t segment_selector);

class InterruptVector
{
public:
    enum Number
    {
        kXHCI = 0x40,
    };
};

struct InterruptFrame
{
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};

void NotifyEndOfInterrupt();
