#include "RemixBlobFactory.h"

#include "RemixRom.h"

namespace SSB64 {

std::optional<std::shared_ptr<IParsedData>> RemixBlobFactory::parse(std::vector<uint8_t>& buffer, YAML::Node& data) {
    RequireRemixRom(buffer, "SSB64:REMIXBLOB");
    const auto offset = GetSafeNode<uint32_t>(data, "offset");
    const auto size = GetSafeNode<uint32_t>(data, "size");
    std::vector<uint8_t> blob = ReadRomRange(buffer, offset, size);
    return std::make_shared<RawBuffer>(blob);
}

}
