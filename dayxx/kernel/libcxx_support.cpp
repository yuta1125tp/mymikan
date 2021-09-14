/**
 * @file libcxx_support.cpp
 * @brief 
 * 
 */

#include <new>
#include <cerrno>

int printk(const char *format, ...);

// add at day06c modi at day09b
std::new_handler std::get_new_handler() noexcept
{
    return []
    {
        printk("not enough memory\n");
        exit(1);
    };
}

// add at day06c
extern "C" int posix_memalign(void **, size_t, size_t)
{
    return ENOMEM;
}
