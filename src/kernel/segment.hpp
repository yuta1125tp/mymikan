/**
 * @file segment.hpp
 * @brief セグメンテーション用のプログラムを集めたファイル
 * 
 */

#pragma once

#include <array>
#include <cstdint>

#include "x86_descriptor.hpp"

/**
 * @brief [ref](みかん本の190p)
 * 
 */
union SegmentDescriptor
{
    uint64_t data;
    struct
    {
        uint64_t limit_low : 16;                 // セグメントのバイト数-1
        uint64_t base_low : 16;                  // セグメントの開始アドレス
        uint64_t base_middle : 8;                // base_lowの続き
        DescriptorType type : 4;                 // ディスクリプタタイプ
        uint64_t system_segment : 1;             // 1ならコードorデータセグメント
        uint64_t descriptor_privilege_level : 2; // DPL:ディスクリプタの権限レベル[0,3]
        uint64_t present : 1;                    //1ならディスクリプタが有効
        uint64_t limit_high : 4;                 //limit_lowの続き
        uint64_t available : 1;                  //OSが好きに使って良いbit
        uint64_t long_mode : 1;                  //1なら64bitモードのコードセグメント
        uint64_t default_operation_size : 1;     //long_mode=1なら0にする
        uint64_t granularity : 1;                // 1ならリミットを4KiB単位として解釈
        uint64_t base_high : 8;                  // base_middleの続き
    } __attribute__((packed)) bits;
} __attribute__((packed));

void SetCodeSegment(
    SegmentDescriptor &desc,
    DescriptorType type,
    unsigned int descriptor_privilege_level,
    uint32_t base,
    uint32_t limit);

void SetDataSegment(
    SegmentDescriptor &desc,
    DescriptorType type,
    unsigned int descriptor_privilege_level,
    uint32_t base,
    uint32_t limit);

const uint16_t kKernelCS = 1 << 3; // Code Segmentレジスタ(CS)はgdt[1]を指す
const uint16_t kKernelSS = 2 << 3; // Stack Segmentレジスタ(SS)はgdt[2]を指す
const uint16_t kKernelDS = 0;      // Data Segmentレジスタ(DS)はgdt[0]を指す？

void SetupSegments();
void InitializeSegmentation();
