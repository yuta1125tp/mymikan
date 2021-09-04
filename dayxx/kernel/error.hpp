#pragma once

#include <cstdio>
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
        kNoEnoughMemory,
        kIndexOutOfRange,
        kHostControllerNotHalted,
        kInvalidSlotID,
        kPortNotConnected,
        kInvalidEndpointNumber,
        kTransferRingNotSet,
        kAlreadyAllocated,
        kNotImplemented,
        kInvalidDescriptor,
        kBufferTooSmall,
        kUnknownDevice,
        kNoCorrespondingSetupStage,
        kTransferFailed,
        kInvalidPhase,
        kUnknownXHCISpeedID,
        kNoWaiter,
        kEndpointNotInCharge,
        kNoPCIMSI,
        kLastOfCode, // この列挙子は常に最後に配置する
    };

private:
    static constexpr std::array code_names_{
        "kSuccess",
        "kFull",
        "kEmpty",
        "kNoEnoughMemory",
        "kIndexOutOfRange",
        "kHostControllerNotHalted",
        "kInvalidSlotID",
        "kPortNotConnected",
        "kInvalidEndpointNumber",
        "kTransferRingNotSet",
        "kAlreadyAllocated",
        "kNotImplemented",
        "kInvalidDescriptor",
        "kBufferTooSmall",
        "kUnknownDevice",
        "kNoCorrespondingSetupStage",
        "kTransferFailed",
        "kInvalidPhase",
        "kUnknownXHCISpeedID",
        "kNoWaiter",
        "kEndpointNotInCharge",
        "kNoPCIMSI",
    };
    static_assert(Error::Code::kLastOfCode == code_names_.size());

public:
    Error(Code code, const char *file, int line)
        : code_{code}, line_{line}, file_{file}
    {
    }

    Code Cause() const
    {
        return this->code_;
    }

    operator bool() const
    {
        return this->code_ != kSuccess;
    }

    const char *Name() const
    {
        return code_names_[static_cast<int>(code_)];
    }

    const char *File() const
    {
        return this->file_;
    }

    int Line() const
    {
        return this->line_;
    }

private:
    Code code_;
    int line_;
    const char *file_;
};

/**
 * @brief エラー構造体を生成するマクロ
 * 
 */
#define MAKE_ERROR(code) Error((code), __FILE__, __LINE__)

/**
 * @brief C++では１つしか返り値を戻せないが、
 * 構造体でラップすると戻り値とエラーコードを組みで返せる。[ref](みかん本の153p)
 * 
 */
template <class T>
struct WithError
{
    T value;
    Error error;
};