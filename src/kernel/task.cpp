#include "task.hpp"

#include "asmfunc.h"
#include "timer.hpp"
#include "segment.hpp"
#include <string.h>  // for memset
#include <algorithm> // for std::find

Task::Task(uint64_t id) : id_{id}
{
}

Task &Task::InitContext(TaskFunc *func, int64_t data)
{
    // std::vector<uint64_t> task_b_stack(1024); //64[bit]*1024/8[bit/byte]=8192[byte]=8[kbyte]
    const size_t stack_size = kDefaultStackBytes / sizeof(stack_[0]);
    stack_.resize(stack_size);
    uint64_t stack_end = reinterpret_cast<uint64_t>(&stack_[stack_size]);

    memset(&context_, 0, sizeof(context_)); // コンテキストを0初期化
    context_.cr3 = GetCR3();                // CR3にはPML4テーブルのアドレスが設定されている。→タスクB実行中も同じPML4テーブルを参照することになる。
    context_.rflags = 0x202;                // 割り込み許可のフラグみかん本315p
    context_.cs = kKernelCS;                // メインタスクと同じCS
    context_.ss = kKernelSS;                // 同じSS

    // アドレス値の下位4ビットを切り捨てて16の倍数に調整し、8引いて下位4ビットを8にしている。
    // 16バイトアライメントなのに、8引いている理由はcall命令後の状態に偽装するため。
    // みかん本のコラム13.1
    context_.rsp = (stack_end & ~0xflu) - 8;

    context_.rip = reinterpret_cast<uint64_t>(func); // RIPにTaskで実行する関数の先頭アドレス
    context_.rdi = id_;                              // 第1引数
    context_.rsi = data;                             // 第2引数

    // MXCSRのすべての例外をマスクする みかん本315p
    *reinterpret_cast<uint32_t *>(&context_.fxsave_area[24]) = 0x1f80;

    return *this;
}

TaskContext &Task::Context() { return context_; }
uint64_t Task::ID() const { return id_; }
Task &Task::Sleep()
{
    task_manager->Sleep(this);
    return *this;
}
Task &Task::Wakeup()
{
    task_manager->Wakeup(this);
    return *this;
}

TaskManager::TaskManager() { running_.push_back(&NewTask()); }

Task &TaskManager::NewTask()
{
    latest_id_++;
    return *tasks_.emplace_back(new Task{latest_id_});
}

void TaskManager::SwitchTask(bool current_sleep /*=false*/)
{
    Task *current_task = running_.front();
    running_.pop_front();
    if (!current_sleep)
    {
        running_.push_back(current_task);
    }
    Task *next_task = running_.front();

    SwitchContext(&next_task->Context(), &current_task->Context());
}

void TaskManager::Sleep(Task *task)
{
    auto it = std::find(running_.begin(), running_.end(), task);
    if (it == running_.begin())
    {
        SwitchTask(true);
        return;
    }

    if (it == running_.end())
    {
        return;
    }

    running_.erase(it);
}

Error TaskManager::Sleep(uint64_t id)
{
    auto it = std::find_if(
        tasks_.begin(),
        tasks_.end(),
        [id](const auto &t)
        { return t->ID() == id; });

    if (it == tasks_.end())
    {
        return MAKE_ERROR(Error::kNoSuchTask);
    }

    Sleep(it->get());
    return MAKE_ERROR(Error::kSuccess);
}

void TaskManager::Wakeup(Task *task)
{
    auto it = std::find(running_.begin(), running_.end(), task);
    if (it == running_.end())
    {
        running_.push_back(task);
    }
}

Error TaskManager::Wakeup(uint64_t id)
{
    auto it = std::find_if(
        tasks_.begin(),
        tasks_.end(),
        [id](const auto &t)
        { return t->ID() == id; });

    if (it == tasks_.end())
    {
        return MAKE_ERROR(Error::kNoSuchTask);
    }

    Wakeup(it->get());
    return MAKE_ERROR(Error::kSuccess);
}

TaskManager *task_manager;

void InitializeTask()
{
    task_manager = new TaskManager;

    __asm__("cli");
    timer_manager->AddTimer(
        Timer{timer_manager->CurrentTick() + kTaskTimerPeriod, kTaskTimerValue});
    __asm__("sti");
}
