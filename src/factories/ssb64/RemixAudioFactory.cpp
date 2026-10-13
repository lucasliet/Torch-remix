#include "RemixAudioFactory.h"

#include "RemixAudioCtl.h"
#include "RemixRom.h"

#include <algorithm>
#include <cstdint>
#include <map>
#include <stdexcept>

namespace SSB64 {
namespace {

// Bank table pointer stored at music ctl + 4 (docs/remix/audio_extraction.md). Used as a ROM sanity check.
constexpr uint32_t REMIX_MUSIC_BANK_POINTER = 0x003C9EA0u;
constexpr uint32_t REMIX_SBK_ROM = 0x02504750u;
constexpr uint16_t REMIX_SBK_ENTRY_COUNT = 0x177;

// Remix FGM segments (tools/remix_dump_audio.py, FGM.asm). The table pointers are relative to the blob start
// when below the segment size, and RAM addresses relative to the moved map otherwise.
constexpr uint32_t REMIX_FGM_UNK_ROM = 0x02934370u;
constexpr uint32_t REMIX_FGM_UNK_SIZE = 0x820u;
constexpr uint32_t REMIX_FGM_TBL_ROM = 0x02934B90u;
constexpr uint32_t REMIX_FGM_TBL_SIZE = 0x2DD0u;
constexpr uint32_t REMIX_FGM_UCD_ROM = 0x02937960u;
constexpr uint32_t REMIX_FGM_UCD_SIZE = 0x4B20u;
constexpr uint32_t REMIX_SFX_FGM_MAP_RAM = 0x80073F80u;
constexpr uint32_t REMIX_FGM_MICROCODE_MAP_RAM = 0x80076D50u;
// FGM.asm DIFFERENCE with midi_count 374 or 375. Matches the record bytes of the vanilla entries.
constexpr int64_t REMIX_FGM_MAP_DIFFERENCE = -0x2A00;
// Upper bound for the last record of a run; the next record start caps every other record.
constexpr size_t REMIX_FGM_RECORD_CAP = 0x100;

void RequireRemixAudioLayout(const std::vector<uint8_t>& rom) {
    const uint32_t bankPointer = ReadBigEndian32(rom, REMIX_MUSIC_CTL.vanillaRom + 4);
    if (bankPointer != REMIX_MUSIC_BANK_POINTER) {
        throw std::runtime_error("SSB64:REMIXAUDIO music ctl at ROM " + HexString(REMIX_MUSIC_CTL.vanillaRom) +
                                 " points to bank " + HexString(bankPointer) + ", expected " +
                                 HexString(REMIX_MUSIC_BANK_POINTER) + "; this is not the Smash Remix ROM");
    }
    const uint16_t sequenceCount = ReadBigEndian16(rom, REMIX_SBK_ROM + 2);
    if (sequenceCount != REMIX_SBK_ENTRY_COUNT) {
        throw std::runtime_error("SSB64:REMIXAUDIO sbk at ROM " + HexString(REMIX_SBK_ROM) + " has " +
                                 std::to_string(sequenceCount) + " entries, expected " +
                                 std::to_string(REMIX_SBK_ENTRY_COUNT));
    }
}

std::vector<uint8_t> BuildSampleBank(const std::vector<uint8_t>& rom, const RemixCtlSpec& spec) {
    const RemixCtlImage ctl = BuildRemixCtlImage(rom, spec);
    const size_t length = FindRemixSampleExtent(rom, ctl, spec);
    return ReadRomRange(rom, spec.tblRom, length);
}

std::vector<uint8_t> BuildSequenceBank(const std::vector<uint8_t>& rom) {
    const uint16_t count = ReadBigEndian16(rom, REMIX_SBK_ROM + 2);
    size_t length = 4 + 8 * static_cast<size_t>(count);
    for (size_t index = 0; index < count; index++) {
        const size_t entry = REMIX_SBK_ROM + 4 + 8 * index;
        const size_t end = static_cast<size_t>(ReadBigEndian32(rom, entry)) + ReadBigEndian32(rom, entry + 4);
        length = std::max(length, end);
    }
    return ReadRomRange(rom, REMIX_SBK_ROM, length);
}

void AppendBigEndian32(std::vector<uint8_t>& out, uint32_t value) {
    out.push_back(static_cast<uint8_t>(value >> 24));
    out.push_back(static_cast<uint8_t>(value >> 16));
    out.push_back(static_cast<uint8_t>(value >> 8));
    out.push_back(static_cast<uint8_t>(value));
}

size_t ResolveFgmRecordRom(uint32_t blobRom, uint32_t blobSize, uint32_t mapRam, uint32_t pointer) {
    if (pointer < blobSize) {
        return blobRom + pointer;
    }
    const int64_t ram = static_cast<int64_t>(mapRam) + REMIX_FGM_MAP_DIFFERENCE + pointer;
    if (ram < 0 || ram > UINT32_MAX) {
        throw std::runtime_error("SSB64:REMIXAUDIO FGM pointer " + HexString(pointer) + " resolves to RAM " +
                                 std::to_string(ram) + ", outside 32-bit RAM");
    }
    return RequireRemixRamToRom(static_cast<uint32_t>(ram), "FGM record");
}

size_t FgmRecordEnd(size_t start, size_t nextStart) {
    return std::min(start + REMIX_FGM_RECORD_CAP, nextStart);
}

// Rebuilds an FGM table as `BE u32 count`, `count` BE u32 offsets relative to the blob start, then the distinct
// records. Entries that point to the same record share one copy.
std::vector<uint8_t> BuildFgmCompactBlob(const std::vector<uint8_t>& rom, uint32_t blobRom, uint32_t blobSize,
                                         uint32_t mapRam) {
    const uint32_t count = ReadBigEndian32(rom, blobRom);
    const size_t headerSize = 4 + 4 * static_cast<size_t>(count);
    std::vector<size_t> starts;
    starts.reserve(count);
    for (uint32_t index = 0; index < count; index++) {
        const uint32_t pointer = ReadBigEndian32(rom, blobRom + 4 + 4 * static_cast<size_t>(index));
        starts.push_back(ResolveFgmRecordRom(blobRom, blobSize, mapRam, pointer));
    }
    std::vector<size_t> distinct(starts);
    std::sort(distinct.begin(), distinct.end());
    distinct.erase(std::unique(distinct.begin(), distinct.end()), distinct.end());

    std::map<size_t, size_t> offsetOf;
    std::vector<uint8_t> records;
    for (size_t index = 0; index < distinct.size(); index++) {
        const size_t start = distinct[index];
        const size_t nextStart = index + 1 < distinct.size() ? distinct[index + 1] : rom.size();
        const size_t end = FgmRecordEnd(start, std::min(nextStart, rom.size()));
        const std::vector<uint8_t> record = ReadRomRange(rom, start, end - start);
        offsetOf[start] = headerSize + records.size();
        records.insert(records.end(), record.begin(), record.end());
    }

    std::vector<uint8_t> blob;
    AppendBigEndian32(blob, count);
    for (const size_t start : starts) {
        AppendBigEndian32(blob, static_cast<uint32_t>(offsetOf.at(start)));
    }
    blob.insert(blob.end(), records.begin(), records.end());
    return blob;
}

std::vector<uint8_t> BuildRemixAudioBank(const std::vector<uint8_t>& rom, const std::string& bank) {
    RequireRemixAudioLayout(rom);
    if (bank == "B1_sounds1_ctl") {
        return BuildRemixCtlImage(rom, REMIX_MUSIC_CTL).bytes;
    }
    if (bank == "B1_sounds1_tbl") {
        return BuildSampleBank(rom, REMIX_MUSIC_CTL);
    }
    if (bank == "B1_sounds2_ctl") {
        return BuildRemixCtlImage(rom, REMIX_SFX_CTL).bytes;
    }
    if (bank == "B1_sounds2_tbl") {
        return BuildSampleBank(rom, REMIX_SFX_CTL);
    }
    if (bank == "S1_music_sbk") {
        return BuildSequenceBank(rom);
    }
    if (bank == "fgm_unk") {
        return ReadRomRange(rom, REMIX_FGM_UNK_ROM, REMIX_FGM_UNK_SIZE);
    }
    if (bank == "fgm_tbl") {
        return BuildFgmCompactBlob(rom, REMIX_FGM_TBL_ROM, REMIX_FGM_TBL_SIZE, REMIX_SFX_FGM_MAP_RAM);
    }
    if (bank == "fgm_ucd") {
        return BuildFgmCompactBlob(rom, REMIX_FGM_UCD_ROM, REMIX_FGM_UCD_SIZE, REMIX_FGM_MICROCODE_MAP_RAM);
    }
    throw std::runtime_error("SSB64:REMIXAUDIO unknown bank '" + bank +
                             "'; expected B1_sounds1_ctl, B1_sounds1_tbl, B1_sounds2_ctl, B1_sounds2_tbl, "
                             "S1_music_sbk, fgm_unk, fgm_tbl or fgm_ucd");
}

}

std::optional<std::shared_ptr<IParsedData>> RemixAudioFactory::parse(std::vector<uint8_t>& buffer,
                                                                     YAML::Node& data) {
    RequireRemixRom(buffer, "SSB64:REMIXAUDIO");
    const auto bank = GetSafeNode<std::string>(data, "bank");
    std::vector<uint8_t> blob = BuildRemixAudioBank(buffer, bank);
    return std::make_shared<RawBuffer>(blob);
}

}
