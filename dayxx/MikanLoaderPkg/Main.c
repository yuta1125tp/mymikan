#include <stdalign.h>
#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PrintLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/DiskIo2.h>
#include <Protocol/BlockIo.h>
#include <Guid/FileInfo.h>
#include "frame_buffer_config.hpp"
#include "memory_map.hpp"
#include "elf.hpp"

EFI_STATUS GetMemoryMap(struct MemoryMap *map)
{
    if (map->buffer == NULL)
    {
        return EFI_BUFFER_TOO_SMALL;
    }

    map->map_size = map->buffer_size;
    return gBS->GetMemoryMap(
        &map->map_size,
        (EFI_MEMORY_DESCRIPTOR *)map->buffer,
        &map->map_key,
        &map->descriptor_size,
        &map->descriptor_version);
}

const CHAR16 *GetMemoryTypeUnicode(EFI_MEMORY_TYPE type)
{
    switch (type)
    {
    case EfiReservedMemoryType:
        return L"EfiReservedMemoryType";
    case EfiLoaderCode:
        return L"EfiLoaderCode";
    case EfiLoaderData:
        return L"EfiLoaderData";
    case EfiBootServicesCode:
        return L"EfiBootServicesCode";
    case EfiBootServicesData:
        return L"EfiBootServicesData";
    case EfiRuntimeServicesCode:
        return L"EfiRuntimeServicesCode";
    case EfiRuntimeServicesData:
        return L"EfiRuntimeServicesData";
    case EfiConventionalMemory:
        return L"EfiConventionalMemory";
    case EfiUnusableMemory:
        return L"EfiUnusableMemory";
    case EfiACPIReclaimMemory:
        return L"EfiACPIReclaimMemory";
    case EfiACPIMemoryNVS:
        return L"EfiACPIMemoryNVS";
    case EfiMemoryMappedIO:
        return L"EfiMemoryMappedIO";
    case EfiMemoryMappedIOPortSpace:
        return L"EfiMemoryMappedIOPortSpace";
    case EfiPalCode:
        return L"EfiPalCode";
    case EfiPersistentMemory:
        return L"EfiPersistentMemory";
    case EfiMaxMemoryType:
        return L"EfiMaxMemoryType";
    default:
        return L"InvalidMemoryType";
    }
}
// #@@range_end(get_memory_type)

// #@@range_begin(save_memory_map)
EFI_STATUS SaveMemoryMap(struct MemoryMap *map, EFI_FILE_PROTOCOL *file)
{
    EFI_STATUS status;
    CHAR8 buf[256];
    UINTN len;

    CHAR8 *header = "Index, Type, Type(name), PhysicalStart, NumberOfPages, Attribute\n";
    len = AsciiStrLen(header);
    status = file->Write(file, &len, header);
    if (EFI_ERROR(status))
    {
        return status;
    }

    Print(L"map->buffer = %08lx, map->map_size = %08lx\n",
          map->buffer, map->map_size);

    EFI_PHYSICAL_ADDRESS iter;
    int i;
    for (iter = (EFI_PHYSICAL_ADDRESS)map->buffer, i = 0;
         iter < (EFI_PHYSICAL_ADDRESS)map->buffer + map->map_size;
         iter += map->descriptor_size, i++)
    {
        EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)iter;
        len = AsciiSPrint(
            buf, sizeof(buf),
            "%u, %x, %-ls, %08lx, %lx, %lx\n",
            i,
            desc->Type,
            GetMemoryTypeUnicode(desc->Type),
            desc->PhysicalStart, desc->NumberOfPages,
            desc->Attribute & 0xffffflu);
        status = file->Write(file, &len, buf);
        if (EFI_ERROR(status))
        {
            return status;
        }
    }

    return EFI_SUCCESS;
}

EFI_STATUS OpenRootDir(EFI_HANDLE image_handle, EFI_FILE_PROTOCOL **root)
{
    EFI_STATUS status;
    EFI_LOADED_IMAGE_PROTOCOL *loaded_image;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;

    status = gBS->OpenProtocol(
        image_handle,
        &gEfiLoadedImageProtocolGuid,
        (VOID **)&loaded_image,
        image_handle,
        NULL,
        EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    if (EFI_ERROR(status))
    {
        return status;
    }

    status = gBS->OpenProtocol(
        loaded_image->DeviceHandle,
        &gEfiSimpleFileSystemProtocolGuid,
        (VOID **)&fs,
        image_handle,
        NULL,
        EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    if (EFI_ERROR(status))
    {
        return status;
    }

    status = fs->OpenVolume(fs, root);
    return status; //EFI_SUCCESS;
}

EFI_STATUS OpenGOP(EFI_HANDLE image_handle,
                   EFI_GRAPHICS_OUTPUT_PROTOCOL **gop)
{
    EFI_STATUS status;
    UINTN num_gop_handles = 0;
    EFI_HANDLE *gop_handles = NULL;
    status = gBS->LocateHandleBuffer(
        ByProtocol,
        &gEfiGraphicsOutputProtocolGuid,
        NULL,
        &num_gop_handles,
        &gop_handles);
    if (EFI_ERROR(status))
    {
        return status;
    }
    status = gBS->OpenProtocol(
        gop_handles[0],
        &gEfiGraphicsOutputProtocolGuid,
        (VOID **)gop,
        image_handle,
        NULL,
        EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    if (EFI_ERROR(status))
    {
        return status;
    }

    FreePool(gop_handles);

    return EFI_SUCCESS;
}

const CHAR16 *GetPixelFormatUnicode(EFI_GRAPHICS_PIXEL_FORMAT fmt)
{
    switch (fmt)
    {
    case PixelRedGreenBlueReserved8BitPerColor:
        return L"PixelRedGreenBlueReserved8BitPerColor";
    case PixelBlueGreenRedReserved8BitPerColor:
        return L"PixelBlueGreenRedReserved8BitPerColor";
    case PixelBitMask:
        return L"PixelBitMask";
    case PixelBltOnly:
        return L"PixelBltOnly";
    case PixelFormatMax:
        return L"PixelFormatMax";
    default:
        return L"InvalidPixelFormat";
    }
}

/**
 * @brief hlt命令を呼び続ける関数。例外表示直後にreturnするとqemuの画面が流れてエラー文などが読めない場合がある対策。
 * 
 */
void Halt(void)
{
    // hlt命令を呼ぶだけ
    while (1)
        __asm__("hlt");
}

/**
 * @brief カーネルファイル内のすべてのLoadセグメントを順にみて、先頭と末尾のアドレスを求める。
 * 
 * @param ehdr 
 * @param first 
 * @param last 
 */
void CalcLoadAddressRange(
    Elf64_Ehdr *ehdr, UINT64 *first, UINT64 *last)
{
    Elf64_Phdr *phdr = (Elf64_Phdr *)((UINT64)ehdr + ehdr->e_phoff);
    *first = MAX_UINT64;
    *last = 0;
    for (Elf64_Half i = 0; i < ehdr->e_phnum; i++)
    {
        if (phdr[i].p_type != PT_LOAD)
        {
            continue;
        }
        *first = MIN(*first, phdr[i].p_vaddr);
        *last = MAX(*last, phdr[i].p_vaddr + phdr[i].p_memsz);
    }
}

/**
 * @brief 一時領域から用意したセグメントへLoadセグメントのデータをコピーし、用意したメモリのサイズがファイル上のサイズより大きい場合は端数を0で埋める。
 * 
 * @param ehdr 
 */
void CopyLoadSegments(Elf64_Ehdr *ehdr)
{
    Elf64_Phdr *phdr = (Elf64_Phdr *)((UINT64)ehdr + ehdr->e_phoff);
    for (Elf64_Half i = 0; i < ehdr->e_phnum; i++)
    {
        if (phdr[i].p_type != PT_LOAD)
        {
            continue;
        }
        UINT64 segm_in_file = (UINT64)ehdr + phdr[i].p_offset;
        CopyMem((VOID *)phdr[i].p_vaddr, (VOID *)segm_in_file, phdr[i].p_filesz);

        UINTN remain_bytes = phdr[i].p_memsz - phdr[i].p_filesz;
        SetMem((VOID *)(phdr[i].p_vaddr + phdr[i].p_filesz), remain_bytes, 0);
    }
}

EFI_STATUS EFIAPI UefiMain(EFI_HANDLE image_handle,
                           EFI_SYSTEM_TABLE *system_table)
{
    EFI_STATUS status;

    Print(L"Hello, Mikan World!!\n");

#pragma region dump_memmap
    CHAR8 memmap_buf[4096 * 4];
    struct MemoryMap memmap = {sizeof(memmap_buf), memmap_buf, 0, 0, 0, 0};
    status = GetMemoryMap(&memmap);
    if (EFI_ERROR(status))
    {
        Print(L"failed to get memory map: %r\n", status);
        Halt();
    }

    EFI_FILE_PROTOCOL *root_dir;
    status = OpenRootDir(image_handle, &root_dir);
    if (EFI_ERROR(status))
    {
        Print(L"failed to open root directory: %r\n", status);
        Halt();
    }

    EFI_FILE_PROTOCOL *memmap_file;
    status = root_dir->Open(
        root_dir, &memmap_file, L"\\memmap",
        EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
    if (EFI_ERROR(status))
    {
        Print(L"failed to open file memmap: %r\n", status);
        Print(L"Skip saving memory map.\n");
    }
    else
    {
        status = SaveMemoryMap(&memmap, memmap_file);
        if (EFI_ERROR(status))
        {
            Print(L"failed to save memory map: %r\n", status);
            Halt();
        }
        status = memmap_file->Close(memmap_file);
        if (EFI_ERROR(status))
        {
            Print(L"failed to close memory map: %r\n", status);
            Halt();
        }
    }

#pragma endregion dump_memmap

#pragma region gop
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
    status = OpenGOP(image_handle, &gop);
    if (EFI_ERROR(status))
    {
        Print(L"failed to open GOP: %r\n", status);
        Halt();
    }

    Print(L"resolution %ux%u, Pixel Format: %s, %u pixles/line\n",
          gop->Mode->Info->HorizontalResolution,
          gop->Mode->Info->VerticalResolution,
          GetPixelFormatUnicode(gop->Mode->Info->PixelFormat),
          gop->Mode->Info->PixelsPerScanLine);
    Print(L"Frame Buffer: 0x%0lx - 0x%0lx, Size: %lu bytes\n",
          gop->Mode->FrameBufferBase,
          gop->Mode->FrameBufferBase + gop->Mode->FrameBufferSize,
          gop->Mode->FrameBufferSize);
    UINT8 *frame_buffer = (UINT8 *)gop->Mode->FrameBufferBase;
    for (UINTN i = 0; i < gop->Mode->FrameBufferSize; ++i)
    {
        frame_buffer[i] = 255;
    }
#pragma endregion gop

#pragma region read_kernel
    EFI_FILE_PROTOCOL *kernel_file;
    status = root_dir->Open(
        root_dir,
        &kernel_file,
        L"\\kernel.elf",
        EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status))
    {
        Print(L"failed to open \\kernel.elf: %r\n", status);
        Halt();
    }
    Print(L"Opened kernel.elf\n");

    UINTN file_info_size = sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 12;
    alignas(alignof(EFI_FILE_INFO)) UINT8 file_info_buffer[file_info_size];
    status = kernel_file->GetInfo(
        kernel_file,
        &gEfiFileInfoGuid,
        &file_info_size,
        file_info_buffer);
    if (EFI_ERROR(status))
    {
        Print(L"failed to get file information: %r\n", status);
        Halt();
    }

    EFI_FILE_INFO *file_info = (EFI_FILE_INFO *)file_info_buffer;
    UINTN kernel_file_size = file_info->FileSize;

#pragma region read_kernel
    VOID *kernel_buffer;
    status = gBS->AllocatePool(EfiLoaderData, kernel_file_size, &kernel_buffer);
    if (EFI_ERROR(status))
    {
        Print(L"failed to alocate pool: %r\n", status);
        Halt();
    }

    status = kernel_file->Read(kernel_file, &kernel_file_size, kernel_buffer);
    if (EFI_ERROR(status))
    {
        Print(L"failed to read kernel file: %r\n", status);
        Halt();
    }

    Elf64_Ehdr *kernel_ehdr = (Elf64_Ehdr *)kernel_buffer;
    UINT64 kernel_first_addr, kernel_last_addr;
    CalcLoadAddressRange(kernel_ehdr, &kernel_first_addr, &kernel_last_addr);

    UINTN num_pages = (kernel_last_addr - kernel_first_addr + 0xfff) / 0x1000;

    status = gBS->AllocatePages(AllocateAddress, EfiLoaderData,
                                num_pages, &kernel_first_addr);
    if (EFI_ERROR(status))
    {
        Print(L"failed to allocate pages: %r\n", status);
        Halt();
    }

    CopyLoadSegments(kernel_ehdr);
    Print(L"Kernel: 0x%0lx-0x%0lx\n", kernel_first_addr, kernel_last_addr);

    status = gBS->FreePool(kernel_buffer);
    if (EFI_ERROR(status))
    {
        Print(L"failed to free pool: %r\n", status);
        Halt();
    }
#pragma endregion

#pragma region day06c時点に追記のあった箇所
    // なにしているのか？ TODO
    status = gBS->ExitBootServices(image_handle, memmap.map_key);
    if (EFI_ERROR(status))
    {
        status = GetMemoryMap(&memmap);
        if (EFI_ERROR(status))
        {
            Print(L"failed to get memory map: %r\n", status);
            Halt();
        }
        status = gBS->ExitBootServices(image_handle, memmap.map_key);
        if (EFI_ERROR(status))
        {
            Print(L"Could not exit boot service: %r\n", status);
            Halt();
        }
    }
#pragma endregion

#pragma region call_kernel
    UINT64 entry_addr = *(UINT64 *)(kernel_first_addr + 24); // entry pointの位置を求める24Byteのオフセットは仕様

    struct FrameBufferConfig config = {
        (UINT8 *)gop->Mode->FrameBufferBase,
        gop->Mode->Info->PixelsPerScanLine,
        gop->Mode->Info->HorizontalResolution,
        gop->Mode->Info->VerticalResolution,
        0,
    };

    switch (gop->Mode->Info->PixelFormat)
    {
    case PixelRedGreenBlueReserved8BitPerColor:
        config.pixel_format = kPixelRGBResv8BitPerColor;
        break;
    case PixelBlueGreenRedReserved8BitPerColor:
        config.pixel_format = kPixelBGRResv8BitPerColor;
        break;
    default:
        Print(L"Unimplemented pixel format: %d\n", gop->Mode->Info->PixelFormat);
        Halt();
    }

    // 関数定義のメモリ上の位置情報だけでは、C言語の関数として呼ぶのに十分ではなく、さらに引数と戻り値の型情報（関数プロトタイプ）が必要[ref](みかん本79p)
    // 指定の引数を持つ関数を表す型EntryPointTypeを定義して、取得した関数の先頭アドレスをキャスト。
    typedef void EntryPointType(
        const struct FrameBufferConfig *,
        const struct MemoryMap *);
    EntryPointType *entry_point = (EntryPointType *)entry_addr; // 関数としてキャスト
    entry_point(&config, &memmap);
#pragma endregion call_kernal

    Print(L"All done\n");

    Halt();
    return EFI_SUCCESS;
}
