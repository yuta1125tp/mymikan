/**
 * @file paging.hpp
 * @brief メモリページング用のプログラムを集めたファイル
 * 
 */

#pragma once

#include <cstddef>

/**
 * @brief 静的に確保するページディレクトリの個数
 * 
 * この関数はSetupIdentityPageMapで使用される。
 * 1つのページディレクトリには512個の2MiBページを設定できるので、
 * kPageDirectoryCount x 1GiBの仮想アドレスがマッピングされることになる。
 * 
 */
const size_t kPageDirectoryCount = 64;

/**
 * @brief 仮想アドレスと物理アドレスが一致するようにページテーブルを設定する。
 * 最終的にCR3レジスタが正しく設定されたページテーブルを指すようにする。
 * 
 */
void SetupIdentityPageTable();

void InitializePaging();
