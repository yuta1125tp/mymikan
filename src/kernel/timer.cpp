#include "timer.hpp"
#include "interrupt.hpp"

namespace
{
    // 定数
    const uint32_t kCountMax = 0xffffffffu;

    // Local APICタイマのレジスタ
    // みかん本の227pの表9.1

    /** @brief 割り込みの発生方法の設定など LVT Timerレジスタのフィールド定義はみかん本の表9.3*/
    volatile uint32_t &lvt_timer = *reinterpret_cast<uint32_t *>(0xfee00320);
    /** @brief カウンタの初期値*/
    volatile uint32_t &initial_count = *reinterpret_cast<uint32_t *>(0xfee00380);
    /** @brief カウンタの現在値*/
    volatile uint32_t &current_count = *reinterpret_cast<uint32_t *>(0xfee00390);
    /** @brief カウンタの減少スピードの設定*/
    volatile uint32_t &divide_config = *reinterpret_cast<uint32_t *>(0xfee003e0);
} // namespace

void InitializeLAPICTimer(std::deque<Message> &msg_queue)
{
    timer_manager = new TimerManager{msg_queue};

    divide_config = 0b1011; // 1対1で分周する設定
    // lvt_timer = (0b001 << 16) | 32; // 16bitの位置に書き込んで割り込み不許可にする（みかん本227pと表9.3）
    lvt_timer = (0b010 << 16) | InterruptVector::kLAPICTimer; // 17bitの位置に書き込んで周期モード、16が0なので割り込み許可、0-7のbit（割り込みベクタ番号）にkLAPICTimerを登録 // みかん本271p
    initial_count = 0x1000000u;
}

void StartLAPICTimer()
{
    initial_count = kCountMax;
}

uint32_t LAPICTimerElapsed()
{
    return kCountMax - current_count;
}

void StopLAPICTimer()
{
    initial_count = 0;
}

Timer::Timer(unsigned long timeout, int value)
    : timeout_{timeout}, value_{value}
{
}

TimerManager::TimerManager(std::deque<Message> &msg_queue)
    : msg_queue_{msg_queue}
{
    timers_.push(Timer{std::numeric_limits<unsigned long>::max(), -1}); // 番兵タイマを追加
}
void TimerManager::AddTimer(const Timer &timer)
{
    timers_.push(timer);
}

void TimerManager::Tick()
{
    ++tick_;
    while (true)
    {
        const auto &t = timers_.top();
        if (t.Timeout() > tick_)
        {
            break;
        }
        // タイムアウトしている場合 - タイムアウト通知用のメッセージを生成してメイン関数に通知
        Message m{Message::kTimerTimeout};
        m.arg.timer.timeout = t.Timeout();
        m.arg.timer.value = t.Value();
        msg_queue_.push_back(m);

        timers_.pop();
    }
}

TimerManager *timer_manager;

void LAPICTimerOnInterrupt()
{
    timer_manager->Tick();
}
