#include "memory_manager.hpp"

BitmapMemoryManager::BitmapMemoryManager()
    : alloc_map_{}, range_begin_{FrameID{0}}, range_end_{FrameID{kFrameCount}}
{
}

/**
 * @brief FirstFitアルゴリズムでメモリを割り当てる[ref](みかん本203p)
 * 
 * @param num_frames 
 * @return WithError<FrameID> 
 */
WithError<FrameID> BitmapMemoryManager::Allocate(size_t num_frames)
{
    size_t start_frame_id = range_begin_.ID();
    while (true)
    {
        size_t i = 0;
        for (; i < num_frames; i++)
        {
            if (start_frame_id + 1 >= range_end_.ID())
            {
                return {kNullFrame, MAKE_ERROR(Error::kNoEnoughMemory)};
            }

            if (GetBit(FrameID{start_frame_id + i}))
            {
                // GetBitが真=指定位置（start_frame_id+i）にあるフレームは割り当て済み
                break;
            }
        }

        if (i == num_frames)
        {
            // 上のforループが最後まで回るとここに入る。breakで終わらない場合＝連続した空き領域があった場合
            // num_frames分の空きが見つかった
            MarkAllocated(FrameID{start_frame_id}, num_frames);
            return {
                FrameID{start_frame_id},
                MAKE_ERROR(Error::kSuccess)};
        }

        //次のフレームから再検索
        start_frame_id += i + 1;
    }
}

/**
 * @brief メモリ領域を返却する
 * 
 * free()はポインタ（領域の先頭アドレス）だけでよいが、BitmapMemoryManagerはサイズも合わせて要求する。
 * 確保時にサイズを覚えておくのが面倒だっただけらしい。 [ref](みかん本204p) TODO
 * 
 * @param start_frame 
 * @param num_frames 
 * @return Error 
 */
Error BitmapMemoryManager::Free(FrameID start_frame, size_t num_frames)
{
    for (size_t i = 0; i < num_frames; i++)
    {
        SetBit(FrameID{start_frame.ID() + i}, false);
    }
    return MAKE_ERROR(Error::kSuccess);
}

void BitmapMemoryManager::MarkAllocated(FrameID start_frame, size_t num_frames)
{
    for (size_t i = 0; i < num_frames; i++)
    {
        SetBit(FrameID{start_frame.ID() + i}, true);
    }
}

void BitmapMemoryManager::SetMemoryRange(FrameID range_begin, FrameID range_end)
{
    range_begin_ = range_begin;
    range_end_ = range_end;
}

bool BitmapMemoryManager::GetBit(FrameID frame) const
{
    // kBitsPerMapLineを横幅と考え、line_indexは行数、bit_indexは列数の2次元行列と思うとイメージしやすい？
    auto line_index = frame.ID() / kBitsPerMapLine;
    auto bit_index = frame.ID() % kBitsPerMapLine;

    return (alloc_map_[line_index] & (static_cast<MapLineType>(1) << bit_index)) != 0;
}

void BitmapMemoryManager::SetBit(FrameID frame, bool allocated)
{
    auto line_index = frame.ID() / kBitsPerMapLine;
    auto bit_index = frame.ID() % kBitsPerMapLine;

    if (allocated)
    {
        alloc_map_[line_index] |= (static_cast<MapLineType>(1) << bit_index);
    }
    else
    {
        alloc_map_[line_index] &= ~(static_cast<MapLineType>(1) << bit_index);
    }
}

extern "C" caddr_t program_break, program_break_end;

/**
 * @brief newlib_support.cのprogram_breakとprogram_break_endを初期化する（みかん本208p）
 * 
 * @param memory_manager 
 * @return Error 
 */
Error InitializeHeap(BitmapMemoryManager &memory_manager)
{
    const int kHeapFrames = 64 * 512;
    const auto heap_start = memory_manager.Allocate(kHeapFrames);
    if (heap_start.error)
    {
        return heap_start.error;
    }

    program_break = reinterpret_cast<caddr_t>(heap_start.value.ID() * kBytesPerFrame);
    program_break_end = program_break + kHeapFrames * kBytesPerFrame;
    return MAKE_ERROR(Error::kSuccess);
}