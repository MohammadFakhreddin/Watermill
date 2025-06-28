#pragma once

#include "AssetTexture.hpp"

#include <memory>
#include <string>

namespace MFA::Importer
{

    [[nodiscard]]
    std::shared_ptr<Asset::Texture> UncompressedImage(std::string const& path);

    [[nodiscard]]
    std::shared_ptr<Asset::Texture> ErrorTexture();

    void LoadMipLevel(Asset::Texture & texture, uint8_t mipLevel);

}
