#pragma once
#include <stdint.h>

struct MemoryMap
{
    unsigned long long buffer_size;
    void *buffer;
    unsigned long long map_size;
    unsigned long long map_key;
    unsigned long long descriptor_size; // 将来的にMemoryDescriptorが拡張する可能性を考慮して、UEFIでは要素の大きさ（バイト数）も取得できるようになっている。
    uint32_t descriptor_version;
};

struct MemoryDescriptor

{
    uint32_t type;
    uintptr_t phsical_start;
    uintptr_t virtual_start;
    uint64_t number_of_pages;
    uint64_t attribute;
};

#ifdef __cplusplus
// c言語のソースでインクルードしたときにenumは使えない。c++のときだけ有効に鳴るように条件コンパイルのマクロ（#ifdef __cplusplus, #endif）で囲む
enum class MemoryType
{
    kEfiReservedMemoryType,
    kEfiLoaderCode,
    kEfiLoaderData,
    kEfiBootServicesCode,
    kEfiBootServicesData,
    kEfiRuntimeServicesCode,
    kEfiRuntimeServicesData,
    kEfiConventionalMemory,
    kEfiUnusableMemory,
    kEfiACPIReclaimMemory,
    kEfiACPIMemoryNVS,
    kEfiMemoryMappedIO,
    kEfiMemoryMappedIOPortSpace,
    kEfiPalCode,
    kEfiPersistentMemory,
    kEfiMaxMemoryType
};

inline bool operator==(uint32_t lhs, MemoryType rhs)
{
    return lhs == static_cast<uint32_t>(rhs);
}

inline bool operator==(MemoryType lhs, uint32_t rhs)
{
    return rhs == lhs;
}

/**
 * @brief 指定されたメモリタイプが空き領域か判定する
 * UEFIを抜けた後空き領域として扱って良いことになっているメモリタイプか確認
 * 
 * @param memory_type 
 * @return true ：空き領域
 * @return false ：非空き領域
 */
inline bool IsAvailable(MemoryType memory_type)
{
    return memory_type == MemoryType::kEfiBootServicesCode ||
           memory_type == MemoryType::kEfiBootServicesData ||
           memory_type == MemoryType::kEfiConventionalMemory;
}

const int kUEFIPageSize = 4096;

#endif
