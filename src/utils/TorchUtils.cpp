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

// Some hosts cannot list or stat files in their read-only app folder (a PS5 title's /app0 refuses
// opendir and stat), so the build may ship a manifest: one file path per line, relative to the
// folder it sits in. Only opening the listed files touches the disk.
static const char* kFileManifestName = ".torch-filelist";

static bool readFileManifest(const fs::path& baseDir, std::set<fs::path>& result) {
    std::ifstream manifest(baseDir / kFileManifestName);
    if (!manifest) {
        return false;
    }
    std::string relativePath;
    while (std::getline(manifest, relativePath)) {
        if (!relativePath.empty()) {
            result.insert(baseDir / relativePath);
        }
    }
    return true;
}

std::vector<fs::path> Torch::getRecursiveFiles(const fs::path baseDir) {
    std::set<fs::path> result;

    if (!readFileManifest(baseDir, result)) {
        for (const auto& entry : fs::recursive_directory_iterator(baseDir)) {
            if (!entry.is_directory()) {
                result.insert(entry.path());
            }
        }
    }

    return std::vector<fs::path>(result.begin(), result.end());
}