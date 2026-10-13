#include "RemixAudioCtl.h"

#include "RemixRom.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>

namespace SSB64 {
namespace {

// Walks ALBankFile pointers and records the largest wavetable end. Only pointers
// that fit in a fully mapped range are followed; the others are skipped, like the
// Python BankParser. Offset 0 is a NULL pointer and is never followed.
class CtlSampleWalker {
public:
    CtlSampleWalker(const RemixCtlImage& ctl, int64_t tblLimit) : mCtl(ctl), mTblLimit(tblLimit) {}

    size_t Walk() {
        WalkBankFile();
        return static_cast<size_t>(mSampleEnd);
    }

private:
    using Visit = void (CtlSampleWalker::*)(uint32_t);

    bool InSpan(uint64_t offset, uint64_t length) const {
        const uint64_t end = offset + length;
        if (end > mCtl.bytes.size()) {
            return false;
        }
        for (const auto& range : mCtl.mappedRanges) {
            if (range.first <= offset && end <= range.second) {
                return true;
            }
        }
        return false;
    }

    void WalkPointer(uint32_t offset, Visit visit) {
        if (offset != 0) {
            (this->*visit)(offset);
        }
    }

    void WalkBankFile() {
        if (!InSpan(0, 4)) {
            return;
        }
        const int16_t count = static_cast<int16_t>(ReadBigEndian16(mCtl.bytes, 2));
        if (count < 0 || !InSpan(0, 4 + 4 * static_cast<uint64_t>(count))) {
            return;
        }
        for (int16_t index = 0; index < count; index++) {
            WalkPointer(ReadBigEndian32(mCtl.bytes, 4 + 4 * static_cast<size_t>(index)), &CtlSampleWalker::WalkBank);
        }
    }

    void WalkBank(uint32_t offset) {
        if (!InSpan(offset, 12)) {
            return;
        }
        const int16_t count = static_cast<int16_t>(ReadBigEndian16(mCtl.bytes, offset));
        if (count < 0 || !InSpan(offset, 12 + 4 * static_cast<uint64_t>(count))) {
            return;
        }
        WalkPointer(ReadBigEndian32(mCtl.bytes, offset + 8), &CtlSampleWalker::WalkInstrument);
        for (int16_t index = 0; index < count; index++) {
            WalkPointer(ReadBigEndian32(mCtl.bytes, offset + 12 + 4 * static_cast<size_t>(index)),
                        &CtlSampleWalker::WalkInstrument);
        }
    }

    void WalkInstrument(uint32_t offset) {
        if (!InSpan(offset, 16)) {
            return;
        }
        const int16_t count = static_cast<int16_t>(ReadBigEndian16(mCtl.bytes, offset + 14));
        if (count < 0 || !InSpan(offset, 16 + 4 * static_cast<uint64_t>(count))) {
            return;
        }
        for (int16_t index = 0; index < count; index++) {
            WalkPointer(ReadBigEndian32(mCtl.bytes, offset + 16 + 4 * static_cast<size_t>(index)),
                        &CtlSampleWalker::WalkSound);
        }
    }

    void WalkSound(uint32_t offset) {
        if (!InSpan(offset, 16)) {
            return;
        }
        WalkPointer(ReadBigEndian32(mCtl.bytes, offset + 8), &CtlSampleWalker::WalkWavetable);
    }

    void WalkWavetable(uint32_t offset) {
        if (!InSpan(offset, 20)) {
            return;
        }
        const int64_t sampleBase = ReadBigEndian32(mCtl.bytes, offset);
        const int64_t sampleLength = static_cast<int32_t>(ReadBigEndian32(mCtl.bytes, offset + 4));
        const int64_t sampleEnd = sampleBase + sampleLength;
        if (sampleEnd > mTblLimit) {
            return;
        }
        mSampleEnd = std::max(mSampleEnd, sampleEnd);
    }

    const RemixCtlImage& mCtl;
    int64_t mTblLimit;
    int64_t mSampleEnd = 0;
};

}

RemixCtlImage BuildRemixCtlImage(const std::vector<uint8_t>& rom, const RemixCtlSpec& spec) {
    const uint32_t codeOffset = REMIX_CODE_RAM - spec.ramBase;
    const size_t codeLength = REMIX_CODE_ROM_END - REMIX_CODE_ROM_BEGIN;
    if (codeOffset < spec.vanillaLength) {
        throw std::runtime_error("Remix ctl at RAM " + HexString(spec.ramBase) + ": vanilla length " +
                                 HexString(spec.vanillaLength) + " overlaps the code image at offset " +
                                 HexString(codeOffset));
    }

    RemixCtlImage image;
    image.bytes.assign(static_cast<size_t>(codeOffset) + codeLength, 0);
    const std::vector<uint8_t> vanilla = ReadRomRange(rom, spec.vanillaRom, spec.vanillaLength);
    const std::vector<uint8_t> code = ReadRomRange(rom, REMIX_CODE_ROM_BEGIN, codeLength);
    std::copy(vanilla.begin(), vanilla.end(), image.bytes.begin());
    std::copy(code.begin(), code.end(), image.bytes.begin() + codeOffset);
    image.mappedRanges = {{0, spec.vanillaLength}, {codeOffset, codeOffset + codeLength}};
    return image;
}

size_t FindRemixSampleExtent(const std::vector<uint8_t>& rom, const RemixCtlImage& ctl,
                                    const RemixCtlSpec& spec) {
    const auto tblLimit = static_cast<int64_t>(rom.size()) - static_cast<int64_t>(spec.tblRom);
    CtlSampleWalker walker(ctl, tblLimit);
    return walker.Walk();
}

}
