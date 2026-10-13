#include "RemixFighterTable.h"

#include "RemixRom.h"

#include <stdexcept>

namespace SSB64 {
namespace {

// FTData field offsets, decomp/src/ft/fttypes.h. The struct is 0x78 bytes.
constexpr size_t FT_FILE_IDS_OFFSET = 0x00;
constexpr size_t FT_FILE_MAIN_SIZE_OFFSET = 0x24;
constexpr size_t FT_ATTRIB_OFFSET = 0x60;
constexpr size_t FT_MAINMOTION_PTR_OFFSET = 0x64;
constexpr size_t FT_SUBMOTION_PTR_OFFSET = 0x68;
constexpr size_t FT_MAINMOTION_COUNT_OFFSET = 0x6C;
constexpr size_t FT_SUBMOTION_COUNT_PTR_OFFSET = 0x70;
constexpr size_t FT_FILE_ANIM_SIZE_OFFSET = 0x74;
constexpr size_t MOTION_DESC_SIZE = 12;
constexpr uint32_t MENU_COUNT_MAX = 0x100u;

std::vector<RemixMotionDesc> ReadMotionArray(const std::vector<uint8_t>& rom, uint32_t ram, uint32_t count) {
    std::vector<RemixMotionDesc> motions;
    if (count == 0) {
        return motions;
    }
    const size_t arrayRom = RequireRemixRamToRom(ram, "motion array");
    for (uint32_t index = 0; index < count; index++) {
        const size_t base = arrayRom + index * MOTION_DESC_SIZE;
        motions.push_back({ReadBigEndian32(rom, base), ReadBigEndian32(rom, base + 4),
                           ReadBigEndian32(rom, base + 8)});
    }
    return motions;
}

std::vector<RemixMotionDesc> ReadMenuArray(const std::vector<uint8_t>& rom, uint32_t submotionRam,
                                           uint32_t countRam) {
    const uint32_t count = ReadBigEndian32(rom, RequireRemixRamToRom(countRam, "menu count pointer"));
    if (count > MENU_COUNT_MAX) {
        throw std::runtime_error("Remix menu count " + HexString(count) + " exceeds " + HexString(MENU_COUNT_MAX) +
                                 " (count pointer " + HexString(countRam) + ")");
    }
    return ReadMotionArray(rom, submotionRam, count);
}

RemixFighter ReadFighter(const std::vector<uint8_t>& rom, uint32_t index, uint32_t structRam) {
    const size_t structRom = RequireRemixRamToRom(structRam, "FTData struct");
    RemixFighter fighter{};
    fighter.remixId = index;
    for (size_t slot = 0; slot < REMIX_FILE_ID_COUNT; slot++) {
        fighter.fileIds[slot] = ReadBigEndian32(rom, structRom + FT_FILE_IDS_OFFSET + 4 * slot);
    }
    fighter.fileMainSize = ReadBigEndian32(rom, structRom + FT_FILE_MAIN_SIZE_OFFSET);
    fighter.attribOffset = ReadBigEndian32(rom, structRom + FT_ATTRIB_OFFSET);
    fighter.fileAnimSize = ReadBigEndian32(rom, structRom + FT_FILE_ANIM_SIZE_OFFSET);
    fighter.mainMotions = ReadMotionArray(rom, ReadBigEndian32(rom, structRom + FT_MAINMOTION_PTR_OFFSET),
                                          ReadBigEndian32(rom, structRom + FT_MAINMOTION_COUNT_OFFSET));
    fighter.menuMotions = ReadMenuArray(rom, ReadBigEndian32(rom, structRom + FT_SUBMOTION_PTR_OFFSET),
                                        ReadBigEndian32(rom, structRom + FT_SUBMOTION_COUNT_PTR_OFFSET));
    return fighter;
}

}

std::vector<RemixFighter> ReadRemixFighters(const std::vector<uint8_t>& rom) {
    std::vector<RemixFighter> fighters;
    for (uint32_t index = 0; index < REMIX_FIGHTER_MAX_COUNT; index++) {
        const uint32_t structRam = ReadBigEndian32(rom, REMIX_STRUCT_TABLE_ROM + 4 * index);
        if (structRam == 0) {
            return fighters;
        }
        fighters.push_back(ReadFighter(rom, index, structRam));
    }
    throw std::runtime_error("Remix struct table at " + HexString(REMIX_STRUCT_TABLE_ROM) +
                             " has no NULL terminator within " + HexString(REMIX_FIGHTER_MAX_COUNT) + " entries");
}

}
