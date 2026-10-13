#pragma once

#include <factories/BlobFactory.h>

namespace SSB64 {

// Emits the raw Remix ROM range given by the `offset` and `size` yaml parameters.
// Unlike BLOB, the offset is read as a ROM file offset and never translated as a segmented address, so Remix
// ROM offsets such as 0x024xxxxx (segment byte 0x02) are not rejected as missing segment data.
class RemixBlobFactory : public BlobFactory {
public:
    std::optional<std::shared_ptr<IParsedData>> parse(std::vector<uint8_t>& buffer, YAML::Node& data) override;
};

}
