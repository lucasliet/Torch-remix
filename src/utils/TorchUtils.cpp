#include "TorchUtils.h"

#include <fstream>
#include <set>
#include <stack>
#include <Companion.h>
#include <factories/BaseFactory.h>
#include "spdlog/spdlog.h"

namespace fs = std::filesystem;

uint32_t Torch::translate(const uint32_t offset) {
    if (SEGMENT_NUMBER(offset) > 0x01) {
        auto segment = SEGMENT_NUMBER(offset);
        const auto addr = Companion::Instance->GetFileOffsetFromSegmentedAddr(segment);
        if (!addr.has_value()) {
            SPDLOG_ERROR("Segment data missing from game config\nPlease add an entry for segment {}", segment);
            throw std::runtime_error("Failed to find offset");
        }

        return addr.value() + SEGMENT_OFFSET(offset);
    }

    return offset;
}

int getFileDepth(const fs::path& base, const fs::path& p) {
    return std::distance(base.begin(), p.begin());
}

// Some hosts cannot list their read-only app folder (a PS5 title's /app0 refuses opendir), so the
// build may ship a manifest: one path per line, relative to the folder it sits in.
static const char* kEntryManifestName = ".torch-filelist";

static bool readEntryManifest(const fs::path& baseDir, std::set<fs::directory_entry>& result) {
    std::ifstream manifest(baseDir / kEntryManifestName);
    if (!manifest) {
        return false;
    }
    std::string relativePath;
    while (std::getline(manifest, relativePath)) {
        if (relativePath.empty()) {
            continue;
        }
        std::error_code ec;
        fs::directory_entry entry(baseDir / relativePath, ec);
        if (ec) {
            throw std::runtime_error("Entry " + relativePath + " of " + (baseDir / kEntryManifestName).string() +
                                     " cannot be read: " + ec.message());
        }
        result.insert(entry);
    }
    return true;
}

std::vector<fs::directory_entry> Torch::getRecursiveEntries(const fs::path baseDir) {
    std::set<fs::directory_entry> result;

    if (readEntryManifest(baseDir, result)) {
        return std::vector<fs::directory_entry>(result.begin(), result.end());
    }

    for (const auto& entry : fs::recursive_directory_iterator(baseDir)) {
        result.insert(entry);
    }

    std::vector<fs::directory_entry> sortedEntries(result.begin(), result.end());
    return sortedEntries;
}