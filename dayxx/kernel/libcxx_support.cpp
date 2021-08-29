/**
 * @file libcxx_support.cpp
 * @brief 
 * 
 */

#include <new>
#include <cerrno>

// add at day06c
std::new_handler std::get_new_handler() noexcept
{
    return nullptr;
}

// add at day06c
extern "C" int posix_memalign(void **, size_t, size_t)
{
    return ENOMEM;
}
