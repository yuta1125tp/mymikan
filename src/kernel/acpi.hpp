/**
 * @file acpi.hpp
 * @brief ACPIテーブル定義や操作用のプログラム
 * 
 */

#pragma once

#include <cstdint>
#include <cstddef>

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

        /** @brief RSDP構造体の検査*/
        bool IsValid() const;
    } __attribute__((packed));

    struct DescriptionHeader
    {
        char signature[4];
        uint32_t length;
        uint8_t revision;
        uint8_t checksum;
        char oem_id[6];
        char oem_table_id[8];
        uint32_t oem_revision;
        uint32_t creator_id;
        uint32_t creator_revision;

        /** @brief 構造体の検査*/
        bool IsValid(const char *expected_signature) const;
    } __attribute__((packed));

    struct XSDT
    {
        DescriptionHeader header;
        const DescriptionHeader &operator[](size_t i) const;
        size_t Count() const;
    } __attribute__((packed));

    struct FADT
    {
        DescriptionHeader header;
        /** @brief FADTのうちACPI PMタイマと関係のないメンバ変数は省略している */
        char reserved1[76 - sizeof(header)];
        uint32_t pm_tmr_blk;
        /** @brief FADTのうちACPI PMタイマと関係のないメンバ変数は省略している */
        char reserved2[112 - 80];
        uint32_t flags;
        /** @brief FADTのうちACPI PMタイマと関係のないメンバ変数は省略している */
        char reserved3[2766 - 116];
    } __attribute__((packed));

    extern const FADT *fadt;
    const int kPMTimerFreq = 3579545; // ACPI PMタイマは3.579545[MHz]の周期

    /***/
    void WaitMilliseconds(unsigned long msec);
    void Initialize(const RSDP &rsdp);
} // namespace acpi
