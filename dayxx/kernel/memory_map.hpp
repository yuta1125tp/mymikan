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

#endif
