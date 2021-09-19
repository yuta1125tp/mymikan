/**
 * @file acpi.hpp
 * @brief ACPIテーブル定義や操作用のプログラム
 * 
 */

#pragma once

#include <cstdint>

/**
 * @brief ACPI(Advanced Configuration and Power Interface)
 * コンピュータの構成や電源を管理するための規格
 * 
 */
namespace acpi
{
    /**
     * @brief RSDP(Root System Description Pointer)
     * みかん本の表11.3など
     * 
     * Local ACPIタイマの周期をキャリブレーションするために...ACPI PMタイマを活用したい
     * ACPI PMタイマのレジスタ郡のポート番号はメモリアドレス空間にあるFADTというテーブルに記載されている。
     * FADTおメモリアドレスを知るにはXSDT(Extended System Descriptor Table)を調べる必要があり、
     * XSDTの場所はRSDPに記載されている。    * 
     * 
     */
    struct RSDP
    {
        char signature[8];         // シグネチャ「RSD PTR 」の8文字
        uint8_t checksum;          // 前半20バイトのためのチェックサム
        char oem_id[6];            // OEMの名前
        uint8_t revision;          // RSDP構造体のバージョン ACPI 6.2では2
        uint32_t rsdt_address;     // RSDTを指す32ビットの物理アドレス
        uint32_t length;           // RSDP全体のバイト数
        uint64_t xsdt_address;     // XSDTを指す64ビットの物理アドレス
        uint8_t extended_checksum; // 拡張領域を含めたRSDP全体のチェックサム
        char reserved[3];          // 予約領域

        /** @brief RSDP構造体の検証*/
        bool IsValid() const;
    } __attribute__((packed));

    void Initialize(const RSDP &rsdp);
} // namespace acpi
