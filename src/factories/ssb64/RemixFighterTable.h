#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace SSB64 {

// Struct pointer table of FTData in the Remix ROM, grown in place (27 vanilla + 70 Remix entries).
static constexpr uint32_t REMIX_STRUCT_TABLE_ROM = 0x00092610u;
static constexpr uint32_t REMIX_FIGHTER_MAX_COUNT = 0x100u;
static constexpr size_t REMIX_FILE_ID_COUNT = 9;

struct RemixMotionDesc {
    uint32_t animFileId;
    uint32_t commandOffset;
    uint32_t flags;
};

struct RemixFighter {
    uint32_t remixId;
    std::array<uint32_t, REMIX_FILE_ID_COUNT> fileIds;
    uint32_t fileMainSize;
    uint32_t attribOffset;
    uint32_t fileAnimSize;
    std::vector<RemixMotionDesc> mainMotions;
    std::vector<RemixMotionDesc> menuMotions;
};

// Reads every fighter until the NULL terminator of the struct table. Throws on any unmapped pointer.
std::vector<RemixFighter> ReadRemixFighters(const std::vector<uint8_t>& rom);

}
