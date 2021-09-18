#include <errno.h>
#include <sys/types.h>

void _exit(void)
{
    while (1)
        __asm__("hlt");
}

caddr_t program_break, program_break_end;

/**
 * @brief mallocが依存する関数。プログラムブレークを設定する関数。
 * プログラムブレーク: Unixシステムの各プロセスが使えるメモリ領域の末尾を示すアドレス。
 * [ref](みかん本の206p)
 * 
 * @param incr バイト
 * @return caddr_t 
 */
caddr_t sbrk(int incr)
{
    if (program_break == 0 || program_break + incr >= program_break_end)
    {
        // program_breakが初期化されていない場合
        // Program breakをIncrバイトだけを動かした際にメモリが足りない場合
        errno = ENOMEM;
        return (caddr_t)-1;
    }

    caddr_t prev_break = program_break;
    program_break += incr;
    return prev_break;
}

int getpid(void)
{
    return 1;
}

int kill(int pid, int sig)
{
    errno = EINVAL;
    return -1;
}
