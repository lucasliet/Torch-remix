#include "RemixRom.h"

#include "RelocFactory.h"

#include <sstream>
#include <stdexcept>

std::string SSB64::HexString(uint64_t value) {
    std::ostringstream out;
    out << "0x" << std::hex << std::uppercase << value;
    return out.str();
}

void SSB64::RequireRemixRom(const std::vector<uint8_t>& rom, const std::string& factoryName) {
    const uint8_t country = rom.size() > ROM_COUNTRY_CODE_OFF ? rom[ROM_COUNTRY_CODE_OFF] : 0;
    if (country == 'E' && rom.size() > VANILLA_ROM_SIZE) {
        return;
    }
    throw std::runtime_error(factoryName + " needs the Smash Remix ROM (country 'E', size above " +
                             HexString(VANILLA_ROM_SIZE) + "), got country " + HexString(country) +
                             " and size " + HexString(rom.size()));
}

uint16_t SSB64::ReadBigEndian16(const std::vector<uint8_t>& rom, size_t romAddr) {
    if (romAddr + 2 > rom.size()) {
        throw std::runtime_error("ROM read of 2 bytes at " + HexString(romAddr) +
                                 " is past the ROM end " + HexString(rom.size()));
    }
    return static_cast<uint16_t>((rom[romAddr] << 8) | rom[romAddr + 1]);
}

uint32_t SSB64::ReadBigEndian32(const std::vector<uint8_t>& rom, size_t romAddr) {
    if (romAddr + 4 > rom.size()) {
        throw std::runtime_error("ROM read of 4 bytes at " + HexString(romAddr) +
                                 " is past the ROM end " + HexString(rom.size()));
    }
    return (static_cast<uint32_t>(rom[romAddr]) << 24) | (static_cast<uint32_t>(rom[romAddr + 1]) << 16) |
           (static_cast<uint32_t>(rom[romAddr + 2]) << 8) | rom[romAddr + 3];
}

std::vector<uint8_t> SSB64::ReadRomRange(const std::vector<uint8_t>& rom, size_t romAddr, size_t length) {
    if (romAddr + length > rom.size()) {
        throw std::runtime_error("ROM slice " + HexString(romAddr) + " + " + HexString(length) +
                                 " is past the ROM end " + HexString(rom.size()));
    }
    return std::vector<uint8_t>(rom.data() + romAddr, rom.data() + romAddr + length);
}

std::optional<size_t> SSB64::RemixRamToRom(uint32_t ram) {
    if (ram >= REMIX_VANILLA_DATA_RAM && ram < REMIX_OVERLAY_RAM_BEGIN) {
        return ram - REMIX_VANILLA_DATA_RAM;
    }
    if (ram >= REMIX_OVERLAY_RAM_BEGIN && ram < REMIX_CODE_RAM) {
        return ram - REMIX_OVERLAY_ROM_BASE;
    }
    if (ram >= REMIX_CODE_RAM) {
        return static_cast<size_t>(ram - REMIX_CODE_RAM) + REMIX_CODE_ROM_BEGIN;
    }
    return std::nullopt;
}

size_t SSB64::RequireRemixRamToRom(uint32_t ram, const std::string& what) {
    const std::optional<size_t> romAddr = RemixRamToRom(ram);
    if (!romAddr.has_value()) {
        throw std::runtime_error("Remix " + what + " RAM " + HexString(ram) +
                                 " is outside the known windows (vanilla data, overlay, code image)");
    }
    return romAddr.value();
}
