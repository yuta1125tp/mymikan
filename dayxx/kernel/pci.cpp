/**
 * @file pci.cpp
 * @brief PCIバス制御のプログラムを集めたファイル
 * 
 */
#include "pci.hpp"
#include "asmfunc.h"

namespace
{
    using namespace pci;
    uint32_t MakeAddress(
        uint8_t bus, uint8_t device,
        uint8_t function, uint8_t reg_addr)
    {
        // Enableビットを1にして読み書きすると任意のレジスタを読み書きすることができる。

        // xを左にbits桁だけビットシフトした値を返すラムダ式
        auto shl = [](uint32_t x, unsigned int bits)
        {
            return x << bits;
        };
        // Enableビットの位置に1を配置、
        // bus, device, functionの位置をCONFIG_ADDRESSレジスタの構造に合わせて配置
        return shl(1, 31)            // enable bit
               | shl(bus, 16)        // 23-16
               | shl(device, 11)     // 15-11
               | shl(function, 8)    // 10-8 // deviceを兎がわきしてしまう気もするが、functionの地域は0-7なので上位bitは必ず0なので（？）orを取っても影響ない？
               | (reg_addr & 0xfcu); // 4バイト単位のオフセットなので、下2bitが0のbitマスク(0xfcu=11111100)とandする。
    }

    /** @brief devices[num_device] に情報を書き込み num_device をインクリメントする． */
    Error AddDevice(const Device &device)
    {
        if (num_device == devices.size())
        {
            return MAKE_ERROR(Error::kFull);
        }
        devices[num_device] = device;
        ++num_device;
        return MAKE_ERROR(Error::kSuccess);
    }

    Error ScanBus(uint8_t bus);

    /** @brief 指定のファンクションを devices に追加する．
   * もし PCI-PCI ブリッジなら，セカンダリバスに対し ScanBus を実行する
   */
    Error ScanFunction(uint8_t bus, uint8_t device, uint8_t function)
    {
        auto class_code = ReadClassCode(bus, device, function);
        auto header_type = ReadHeaderType(bus, device, function);
        Device dev{bus, device, function, header_type, class_code};
        if (auto err = AddDevice(dev))
        {
            return err;
        }

        if (class_code.Match(0x06u, 0x04u))
        {
            // standard PCI-PCI bridge
            // ブリッジの上流側がプライマリバス、下流側がセカンダリバス
            // セカンダリバスの番号からScanBusしてセカンダリバスに接続されたPCIデバイスも探索する。
            auto bus_numbers = ReadBusNumbers(bus, device, function);
            uint8_t secondary_bus = (bus_numbers >> 8) & 0xffu;
            return ScanBus(secondary_bus);
        }

        return MAKE_ERROR(Error::kSuccess);
    }

    /** @brief 指定のデバイス番号の各ファンクションをスキャンする．
   * 有効なファンクションを見つけたら ScanFunction を実行する．
   */
    Error ScanDevice(uint8_t bus, uint8_t device)
    {
        if (auto err = ScanFunction(bus, device, 0))
        {
            return err;
        }
        if (IsSingleFunctionDevice(ReadHeaderType(bus, device, 0)))
        {
            return MAKE_ERROR(Error::kSuccess);
        }

        for (uint8_t function = 1; function < 8; function++)
        {
            if (ReadVendorId(bus, device, function) == 0xffffu)
            {
                continue;
            }
            if (auto err = ScanFunction(bus, device, function))
            {
                return err;
            }
        }
        return MAKE_ERROR(Error::kSuccess);
    }

    /** @brief 指定のバス番号の各デバイスをスキャンする．
   * 有効なデバイスを見つけたら ScanDevice を実行する．
   */
    Error ScanBus(uint8_t bus)
    {
        for (uint8_t device = 0; device < 32; device++)
        {

            if (ReadVendorId(bus, device, 0) == 0xffffu)
            {
                continue;
            }
            if (auto err = ScanDevice(bus, device))
            {
                return err;
            }
        }
        return MAKE_ERROR(Error::kSuccess);
    }
}

namespace pci
{
    void WriteAddress(uint32_t address)
    {
        IoOut32(kConfigAddress, address);
    }

    void WriteData(uint32_t value)
    {
        IoOut32(kConfigData, value);
    }

    uint32_t ReadData()
    {
        return IoIn32(kConfigData);
    }

    uint16_t ReadVendorId(uint8_t bus, uint8_t device, uint8_t function)
    {
        // CONFIG_ADDRESSレジスタに読み書きしたいPCIコンフィグレーション空間の位置を設定してから
        // CONFIG_DATAを読み書きすることでPCIコンフィグレーション空間を読み書きすることができる。
        // PCIコンフィギュレーション空間の先頭から16bitがVendorID[ref](図6.3みかん本@142)
        WriteAddress(MakeAddress(bus, device, function, 0x00));
        return ReadData() & 0xffffu;
    }

    uint16_t ReadDeviceId(uint8_t bus, uint8_t device, uint8_t function)
    {
        // PCIコンフィギュレーション空間の先頭から32bit読んで、上16bit分がDeviceId[ref](図6.3みかん本@142)
        WriteAddress(MakeAddress(bus, device, function, 0x00));
        return ReadData() >> 16;
    }

    uint8_t ReadHeaderType(uint8_t bus, uint8_t device, uint8_t function)
    {
        // PCIコンフィギュレーション空間の0x0Cから32bit読んで、上16bitがHeaderType[ref](図6.3みかん本@142)
        WriteAddress(MakeAddress(bus, device, function, 0x0c));
        return (ReadData() >> 16) & 0xffu;
    }

    ClassCode ReadClassCode(uint8_t bus, uint8_t device, uint8_t function)
    {
        // PCIコンフィギュレーション空間の0x08から32bit読んで、上16bitの上8bitがBaseClass, 下8bitがSub Class[ref](図6.3みかん本@142)
        WriteAddress(MakeAddress(bus, device, function, 0x08));
        auto reg = ReadData();
        ClassCode cc;
        cc.base = (reg >> 24) & 0xffu;     // 大雑把なデバイス種別：たとえば0x0c=シリアル通信のためのコントローラなど[ref](みかん本149p)
        cc.sub = (reg >> 16) & 0xffu;      // 細かいデバイス種別：たとえば0x03=USB
        cc.interface = (reg >> 8) & 0xffu; // レジスタレベルの仕様：たとえば、0x20はUSB2.0(EHCI), 0x30はUSB3.0(xHCI)など
        return cc;
    }

    uint32_t ReadBusNumbers(uint8_t bus, uint8_t device, uint8_t function)
    {
        WriteAddress(MakeAddress(bus, device, function, 0x18));
        return ReadData();
    }

    bool IsSingleFunctionDevice(uint8_t header_type)
    {
        return (header_type & 0x80u) == 0;
    }

    Error ScanAllBus()
    {
        num_device = 0;
        // バス0上のデバイス0は必ずホストブリッジ（ホスト側とPCIバス側を橋渡しする部品でCPUとPCIデバイスの間の通信はここを必ず通過する。[ref](みかん本@147p)）
        auto header_type = ReadHeaderType(0, 0, 0);

        // 単機能デバイスの場合はホストブリッジがバス0を担当するホストブリッジ。
        if (IsSingleFunctionDevice(header_type))
        {
            return ScanBus(0);
        }

        // マルチファンクションデバイスである場合は、ホストブリッジが複数あり、
        // この場合はファンクション0のホストブリッジはバス0を担当し、ファンクション1のホストブリッジはバス1が担当するというように、ファンクション番号が担当バス番号を表す。
        for (uint8_t function = 0; function < 8; function++)
        {
            if (ReadVendorId(0, 0, function) == 0xffffu) // 0xffffuは無効値
            {
                continue;
            }
            if (auto err = ScanBus(function))
            {
                return err;
            }
        }
        return MAKE_ERROR(Error::kSuccess);
    }

    uint32_t ReadConfReg(const Device &dev, uint8_t reg_addr)
    {
        WriteAddress(MakeAddress(dev.bus, dev.device, dev.function, reg_addr));
        return ReadData();
    }

    void WriteConfReg(const Device &dev, uint8_t reg_addr, uint32_t value)
    {
        WriteAddress(MakeAddress(dev.bus, dev.device, dev.function, reg_addr));
        WriteData(value);
    }

    WithError<uint64_t> ReadBar(Device &device, unsigned int bar_index)
    {
        if (bar_index >= 6)
        {
            return {0, MAKE_ERROR(Error::kIndexOutOfRange)};
        }

        const auto addr = CalcBarAddress(bar_index);
        const auto bar = ReadConfReg(device, addr);

        // 32 bit address
        if ((bar & 4u) == 0)
        {
            return {bar, MAKE_ERROR(Error::kSuccess)};
        }

        // 64 bit address
        if (bar_index >= 5)
        {
            return {0, MAKE_ERROR(Error::kIndexOutOfRange)};
        }

        const auto bar_upper = ReadConfReg(device, addr + 4);
        return {
            bar | (static_cast<uint64_t>(bar_upper) << 32),
            MAKE_ERROR(Error::kSuccess)};
    }
}
