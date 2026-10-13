#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace SSB64 {

// Addresses of one ctl bank, from docs/remix/audio_extraction.md (tools/remix_dump_audio.py).
struct RemixCtlSpec {
    uint32_t ramBase;
    uint32_t vanillaRom;
    uint32_t vanillaLength;
    uint32_t tblRom;
};

inline constexpr RemixCtlSpec REMIX_MUSIC_CTL{0x800472D0u, 0x252B560u, 0x6720u, 0x2531C80u};
inline constexpr RemixCtlSpec REMIX_SFX_CTL{0x8004D9F0u, 0x26485F0u, 0xFBA0u, 0x2658190u};

// Runtime image of a ctl bank: vanilla bytes, the code image, and zeros elsewhere.
// mappedRanges holds [begin, end) pairs that are backed by ROM.
struct RemixCtlImage {
    std::vector<uint8_t> bytes;
    std::vector<std::pair<size_t, size_t>> mappedRanges;
};

RemixCtlImage BuildRemixCtlImage(const std::vector<uint8_t>& rom, const RemixCtlSpec& spec);

// Largest sample end (base + length) over every wavetable reachable from the bank file.
// Matches the bounds rules of port/bridge/audio_bridge.cpp BankParser.
size_t FindRemixSampleExtent(const std::vector<uint8_t>& rom, const RemixCtlImage& ctl, const RemixCtlSpec& spec);

}
