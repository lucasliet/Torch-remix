#pragma once

#include <factories/BlobFactory.h>

namespace SSB64 {

// Emits the remix/fighters blob. Exporters are inherited from BlobFactory, so the
// o2r entry is a standard BLOB resource whose payload is the remix/fighters format.
class RemixFightersFactory : public BlobFactory {
public:
    std::optional<std::shared_ptr<IParsedData>> parse(std::vector<uint8_t>& buffer, YAML::Node& data) override;
};

}
