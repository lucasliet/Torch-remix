#pragma once

#include <factories/BlobFactory.h>

namespace SSB64 {

// Emits one reconstructed Remix audio bank selected by the `bank` yaml parameter.
// Exporters are inherited from BlobFactory (standard BLOB resource in the o2r).
class RemixAudioFactory : public BlobFactory {
public:
    std::optional<std::shared_ptr<IParsedData>> parse(std::vector<uint8_t>& buffer, YAML::Node& data) override;
};

}
