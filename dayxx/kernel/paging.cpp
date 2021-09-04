#include "paging.hpp"

#include <array>

#include "asmfunc.h"

namespace
{
    const uint64_t kPageSize4K = 4096;
    const uint64_t kPageSize2M = 512 * kPageSize4K;
    const uint64_t kPageSize1G = 512 * kPageSize2M;

    alignas(kPageSize4K) std::array<uint64_t, 512> pml4_table;
    alignas(kPageSize4K) std::array<uint64_t, 512> pdp_table;
    alignas(kPageSize4K) std::array<std::array<uint64_t, 512>, kPageDirectoryCount> page_directory;
}

void SetupIdentityPageTable()
{
    /**
     * @brief [ref](みかん本196p)
     * 64bitモードにおけるページングの設定は以下の4階層の階層ページング構造を持つ
     * - ページマップレベル4テーブル: PML4 table        : 上位
     * - ページディレクトリポインタテーブル PDP table
     * - ページディレクトリ
     * - ページテーブル                                ：下位
     * 
     */

    // PML4テーブルの先頭に、PDPテーブルの先頭アドレスを設定
    pml4_table[0] = reinterpret_cast<uint64_t>(&pdp_table[0]) | 0x003;
    for (int i_pdpt = 0; i_pdpt < page_directory.size(); i_pdpt++)
    {
        // 各テーブルにページディレクトリの先頭アドレス
        pdp_table[i_pdpt] = reinterpret_cast<uint64_t>(&page_directory[i_pdpt]) | 0x003;
        for (int i_pd = 0; i_pd < 512; i_pd++)
        {
            // ページディレクトリの各要素を設定
            // | 0x083のビット和により、各要素のbit7を1にできて、2MiBページになる（？みかん本の197p）
            page_directory[i_pdpt][i_pd] = i_pdpt * kPageSize1G + i_pd * kPageSize2M | 0x083;
        }
    }
    // PML4テーブルの物理アドレスをCR3レジスタに設定。
    SetCR3(reinterpret_cast<uint64_t>(&pml4_table[0]));
    // これ以降CPUは設定sチア新しい海藻ページング構造を使ってアドレス変換をする。（これ以前はUEFIが用意したものを利用している）
}
