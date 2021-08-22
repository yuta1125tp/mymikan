#pragma once

#include <array>

/**
 * @brief エラーに関するクラス
 * 
 */
class Error
{
public:
    enum Code
    {
        kSuccess,
        kFull,
        kEmpty,
        kLastOffCode,
    };

    Error(Code code) : code_{code} {}

    operator bool() const
    {
        return this->code_ != kSuccess;
    }

    const char *Name() const
    {
        return code_names_[static_cast<int>(code_)];
    }

private:
    static constexpr std::array<const char *, 3> code_names_ = {
        "kSuccess",
        "kFull",
        "kEmpty",
    };

    Code code_;
};
