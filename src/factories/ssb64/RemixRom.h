#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace SSB64 {

// Smash Remix 2.0.1 addresses, checked against the ROM bytes.
// RAM-to-ROM windows: vanilla data ROM = RAM - REMIX_VANILLA_DATA_RAM; the expanded code segment
// (REMIX_CODE_RAM..) ROM = RAM - 0x7D800000; the overlay window REMIX_OVERLAY_RAM_BEGIN.. ROM = RAM - REMIX_OVERLAY_ROM_BASE.
// The code image (ROM REMIX_CODE_ROM_BEGIN..REMIX_CODE_ROM_END) is also what the audio ctl banks read past their
// vanilla blob: a ctl offset is relative to the bank's RAM base, so it lands in this image.
static constexpr uint32_t REMIX_VANILLA_DATA_RAM = 0x80084800u;
static constexpr uint32_t REMIX_OVERLAY_RAM_BEGIN = 0x80390000u;
static constexpr uint32_t REMIX_OVERLAY_ROM_BASE = 0x80288A20u;
static constexpr uint32_t REMIX_CODE_RAM = 0x80400000u;
static constexpr uint32_t REMIX_CODE_ROM_BEGIN = 0x02C00000u;
static constexpr uint32_t REMIX_CODE_ROM_END = 0x03000000u;

std::string HexString(uint64_t value);

// Throws unless the buffer is the Smash Remix ROM (country 'E' and size above the vanilla size).
void RequireRemixRom(const std::vector<uint8_t>& rom, const std::string& factoryName);

uint16_t ReadBigEndian16(const std::vector<uint8_t>& rom, size_t romAddr);
uint32_t ReadBigEndian32(const std::vector<uint8_t>& rom, size_t romAddr);
std::vector<uint8_t> ReadRomRange(const std::vector<uint8_t>& rom, size_t romAddr, size_t length);

std::optional<size_t> RemixRamToRom(uint32_t ram);
size_t RequireRemixRamToRom(uint32_t ram, const std::string& what);

}
