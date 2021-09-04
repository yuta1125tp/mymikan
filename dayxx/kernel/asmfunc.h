#pragma once
#include <stdint.h>

extern "C"
{
    void IoOut32(uint16_t addr, uint32_t data);
    uint32_t IoIn32(uint16_t addr);
    uint16_t GetCS(void);
    void LoadIDT(uint16_t limit, uint64_t offset);
    /**
     * @brief GDTのサイズと場所をCPUに登録する
     * 
     * @param limit 
     * @param offset 
     */
    void LoadGDT(uint16_t limit, uint64_t offset);
    /**
     * @brief Code Segmentレジスタ(CS)とStack Segmentレジスタ(SS)に値を設定する
     * 
     * @param cs 
     * @param ss 
     */
    void SetCSSS(uint16_t cs, uint16_t ss);
    void SetDSAll(uint16_t value);
    /**
     * @brief 指定したPML4テーブルの物理アドレスをCR3レジスタに登録
     * 
     * @param value PML4テーブルの物理アドレス
     */
    void SetCR3(uint64_t value);
}
