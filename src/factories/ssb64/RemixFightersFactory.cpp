#include "RemixFightersFactory.h"

#include "RemixFighterTable.h"
#include "RemixRom.h"

namespace SSB64 {
namespace {

// Layout of remix/fighters (docs/remix/implementation_plan.md). Integers are little-endian.
constexpr uint32_t REMIX_FIGHTERS_MAGIC = 0x46584D52u;  // bytes 'R' 'M' 'X' 'F'
constexpr uint32_t REMIX_FIGHTERS_VERSION = 1;
constexpr size_t REMIX_FIGHTERS_RECORD_SIZE = 0x80;

void AppendLittleEndian32(std::vector<uint8_t>& out, uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8) {
        out.push_back(static_cast<uint8_t>(value >> shift));
    }
}

void AppendMotions(std::vector<uint8_t>& out, const std::vector<RemixMotionDesc>& motions) {
    for (const RemixMotionDesc& motion : motions) {
        AppendLittleEndian32(out, motion.animFileId);
        AppendLittleEndian32(out, motion.commandOffset);
        AppendLittleEndian32(out, motion.flags);
    }
}

void AppendRecord(std::vector<uint8_t>& out, const RemixFighter& fighter, uint32_t mainOffset,
                  uint32_t menuOffset) {
    const size_t start = out.size();
    AppendLittleEndian32(out, fighter.remixId);
    for (uint32_t fileId : fighter.fileIds) {
        AppendLittleEndian32(out, fileId);
    }
    AppendLittleEndian32(out, fighter.fileMainSize);
    AppendLittleEndian32(out, fighter.attribOffset);
    AppendLittleEndian32(out, fighter.fileAnimSize);
    AppendLittleEndian32(out, static_cast<uint32_t>(fighter.mainMotions.size()));
    AppendLittleEndian32(out, mainOffset);
    AppendLittleEndian32(out, static_cast<uint32_t>(fighter.menuMotions.size()));
    AppendLittleEndian32(out, menuOffset);
    out.resize(start + REMIX_FIGHTERS_RECORD_SIZE, 0);
}

std::vector<uint8_t> SerializeRemixFighters(const std::vector<RemixFighter>& fighters) {
    std::vector<uint8_t> records;
    std::vector<uint8_t> motions;
    for (const RemixFighter& fighter : fighters) {
        const auto mainOffset = static_cast<uint32_t>(motions.size());
        AppendMotions(motions, fighter.mainMotions);
        const auto menuOffset = static_cast<uint32_t>(motions.size());
        AppendMotions(motions, fighter.menuMotions);
        AppendRecord(records, fighter, mainOffset, menuOffset);
    }

    std::vector<uint8_t> blob;
    AppendLittleEndian32(blob, REMIX_FIGHTERS_MAGIC);
    AppendLittleEndian32(blob, REMIX_FIGHTERS_VERSION);
    AppendLittleEndian32(blob, static_cast<uint32_t>(fighters.size()));
    AppendLittleEndian32(blob, 0);
    blob.insert(blob.end(), records.begin(), records.end());
    blob.insert(blob.end(), motions.begin(), motions.end());
    return blob;
}

}

std::optional<std::shared_ptr<IParsedData>> RemixFightersFactory::parse(std::vector<uint8_t>& buffer,
                                                                        YAML::Node& data) {
    RequireRemixRom(buffer, "SSB64:REMIXFIGHTERS");
    std::vector<uint8_t> blob = SerializeRemixFighters(ReadRemixFighters(buffer));
    return std::make_shared<RawBuffer>(blob);
}

}
