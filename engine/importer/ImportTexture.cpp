#include "ImportTexture.hpp"

#include "BedrockAssert.hpp"
#include "BedrockDeffer.hpp"
#include "BedrockFile.hpp"
#include "BedrockMemory.hpp"
#include "BedrockPath.hpp"
#include "BedrockPlatforms.hpp"
#include "AssetTexture.hpp"

#include "stb_image.h"
#include "stb_image_resize.h"

namespace MFA::Importer
{

    using Format = AS::Texture::Format;

    struct Data
    {
        int32_t width = 0;
        int32_t height = 0;
        int32_t stbi_components = 0;
        std::shared_ptr<Blob> stbi_pixels;
        Format format = Format::INVALID;
        std::shared_ptr<Blob> pixels;
        uint32_t components = 0;
        [[nodiscard]]
        bool valid() const
        {
            return stbi_pixels != nullptr &&
                stbi_pixels->IsValid() == true &&
                width > 0 &&
                height > 0 &&
                stbi_components > 0;
        }
    };

    enum class LoadResult
    {
        Invalid,
        Success,
        FileNotExists
        // Format not supported
    };

    static LoadResult LoadUncompressed(Data& outImageData, std::string const& path, bool prefer_srgb)
    {
        LoadResult ret = LoadResult::Invalid;

        auto const rawFile = File::Read(path);

        if (rawFile == nullptr)
        {
            return ret;
        }

        auto* readData = stbi_load_from_memory(
            rawFile->Ptr(),
            static_cast<int>(rawFile->Len()),
            &outImageData.width,
            &outImageData.height,
            &outImageData.stbi_components,
            0
        );

        MFA_DEFFER([readData](){stbi_image_free(readData);});
        if (readData != nullptr)
        {
            MFA_ASSERT(outImageData.width > 0);
            MFA_ASSERT(outImageData.height > 0);
            MFA_ASSERT(outImageData.stbi_components > 0);

            auto blobSize = static_cast<size_t>(outImageData.width) *
                outImageData.height *
                outImageData.stbi_components *
                sizeof(uint8_t);

            outImageData.stbi_pixels = Memory::Alloc(readData, blobSize);

            outImageData.components = outImageData.stbi_components;
            if (prefer_srgb)
            {
                switch (outImageData.stbi_components)
                {
                case 1:
                    outImageData.format = Format::UNCOMPRESSED_UNORM_R8_SRGB;
                    break;
                case 2:
                case 3:
                case 4:
                    outImageData.format = Format::UNCOMPRESSED_UNORM_R8G8B8A8_SRGB;
                    outImageData.components = 4;
                    break;
                default: MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
                }
            }
            else
            {
                switch (outImageData.stbi_components)
                {
                case 1:
                    outImageData.format = Format::UNCOMPRESSED_UNORM_R8_LINEAR;
                    break;
                case 2:
                    outImageData.format = Format::UNCOMPRESSED_UNORM_R8G8_LINEAR;
                    break;
                case 3:
                case 4:
                    outImageData.format = Format::UNCOMPRESSED_UNORM_R8G8B8A8_LINEAR;
                    outImageData.components = 4;
                    break;
                default: MFA_LOG_WARN("Unhandled component count: %d", outImageData.stbi_components);
                }
            }
            MFA_ASSERT(outImageData.components >= static_cast<uint32_t>(outImageData.stbi_components));
            if (static_cast<int>(outImageData.components) == outImageData.stbi_components)
            {
                outImageData.pixels = outImageData.stbi_pixels;
            }
            else
            {
                auto const size = static_cast<size_t>(outImageData.width) *
                    outImageData.height *
                    outImageData.components *
                    sizeof(uint8_t);

                outImageData.pixels = Memory::AllocSize(size);

                auto* pixels_array = outImageData.pixels->As<uint8_t>();
                auto const* stbi_pixels_array = outImageData.stbi_pixels->As<uint8_t>();
                // This can be parallized using openMP
                for (int pixel_index = 0; pixel_index < outImageData.width * outImageData.height; pixel_index++)
                {
                    for (uint32_t component_index = 0; component_index < outImageData.components; component_index++)
                    {
                        pixels_array[pixel_index * outImageData.components + component_index] = static_cast<int64_t>(component_index) < outImageData.stbi_components
                            ? stbi_pixels_array[pixel_index * outImageData.stbi_components + component_index]
                            : 255u;
                    }
                }
            }
            ret = LoadResult::Success;
        }
        else
        {
            ret = LoadResult::FileNotExists;
        }
        return ret;
    }

    //-------------------------------------------------------------------------------------------------

    struct ResizeInputParams
    {
        BaseBlob inputImagePixels{};
        int32_t inputImageWidth{};
        int32_t inputImageHeight{};
        uint32_t componentsCount{};
        std::shared_ptr<Blob> outputImagePixels{};
        int32_t outputWidth{};
        int32_t outputHeight{};
        bool useSRGB{};
    };

    static bool ResizeUncompressed(ResizeInputParams const& params)
    {
        MFA_ASSERT(params.inputImageWidth > 0);
        MFA_ASSERT(params.inputImageHeight > 0);
        MFA_ASSERT(params.inputImagePixels.Ptr() != nullptr);
        MFA_ASSERT(params.inputImagePixels.Len() > 0);

        MFA_ASSERT(params.componentsCount > 0);

        MFA_ASSERT(params.outputWidth > 0);
        MFA_ASSERT(params.outputHeight > 0);
        MFA_ASSERT(params.outputImagePixels->Ptr() != nullptr);
        MFA_ASSERT(params.outputImagePixels->Len() > 0);

        auto const resizeResult = params.useSRGB ? stbir_resize_uint8_srgb(
            params.inputImagePixels.Ptr(),
            params.inputImageWidth,
            params.inputImageHeight,
            0,
            params.outputImagePixels->Ptr(),
            params.outputWidth,
            params.outputHeight,
            0,
            params.componentsCount,
            params.componentsCount > 3 ? 3 : STBIR_ALPHA_CHANNEL_NONE,
            0
        ) : stbir_resize_uint8(
            params.inputImagePixels.Ptr(),
            params.inputImageWidth,
            params.inputImageHeight,
            0,
            params.outputImagePixels->Ptr(),
            params.outputWidth,
            params.outputHeight,
            0,
            params.componentsCount
        );
        return resizeResult > 0;
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<AS::Texture> UncompressedImage(std::string const& path)
    {
        std::shared_ptr<AS::Texture> texture{};

        Data imageData{};
        auto const loadImageResult = LoadUncompressed(
            imageData,
            path,
            false
        );

        if (loadImageResult == LoadResult::Success)
        {
            MFA_ASSERT(imageData.valid());
            uint32_t const imageWidth = imageData.width;
            uint32_t const imageHeight = imageData.height;
            auto const pixels = imageData.pixels;
            // auto const components = imageData.components;
            auto const format = imageData.format;

            texture = std::make_shared<AS::Texture>(
                path,
                format,
                1,
                1,
                1
            );

            texture->SetMipmapDimension(0, AS::Texture::Dimensions {
                .width = imageWidth,
                .height = imageHeight,
                .depth = 1
            });
            texture->SetMipmapOffset(0, 0);
            texture->SetMipmapSize(0, pixels->Len());
            texture->SetMipmapData(0, pixels);
        }

        return texture;
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<AS::Texture> ErrorTexture()
    {
        auto data = Memory::AllocSize(4);
        auto * pixels = data->As<uint8_t>();
        pixels[0] = 1;
        pixels[1] = 1;
        pixels[2] = 1;
        pixels[3] = 1;

        auto const texture = std::make_shared<AS::Texture>(
            "",
            Format::UNCOMPRESSED_UNORM_R8G8B8A8_LINEAR,
            1,
            1,
            1
        );

        texture->SetMipmapDimension(0, AS::Texture::Dimensions {
            .width = 1,
            .height = 1,
            .depth = 1
        });
        texture->SetMipmapOffset(0, 0);
        texture->SetMipmapSize(1, data->Len());
        texture->SetMipmapData(0, std::move(data));

        return texture;
    }

}
