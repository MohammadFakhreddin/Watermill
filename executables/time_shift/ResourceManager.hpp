#pragma once

#include "ThreadSafeQueue.hpp"
#include "RenderTypes.hpp"

#include <future>
#include <unordered_map>
#include <filesystem>

namespace MFA::RenderTypes
{
    struct BufferAndMemory;
}
namespace MFA
{
    namespace Asset
    {
        class Texture;
    }

    class Blob;
    namespace RenderTypes
    {
        struct GpuTexture;
        struct CommandRecordState;
    }
}

class ResourceManager : public std::enable_shared_from_this<ResourceManager>
{
public:

    static std::shared_ptr<ResourceManager> Instance(bool createNewIfNotExists = false);

    explicit ResourceManager();
    ~ResourceManager();

    using ImageCallback = std::function<void(std::shared_ptr<MFA::RT::GpuTexture>)>;
    void RequestImage(char const * name, const ImageCallback & callback);

    // Temporary we should not need this
    void ForceCleanUp();

    [[nodiscard]]
    std::shared_ptr<MFA::RT::GpuTexture> const & ErrorTexture() const
    {
        return _errorTexture;
    }

private:

    struct ImageData
    {
        std::filesystem::path path;
        bool hasMipmaps = false;
        int mipCount = 0;
        int currentMipLevel = -1;
        // MFA::ThreadSafeQueue<ImageCallback> callbacks;
        std::vector<ImageCallback> callbacks;
        std::weak_ptr<MFA::RenderTypes::GpuTexture> gpuTexture;
    };

    void RequestNextMipmap(std::weak_ptr<ImageData> imageData, int nextMipLevel);

    inline static std::weak_ptr<ResourceManager> _instance {};

    std::unordered_map<std::string, std::shared_ptr<ImageData>> _imageMap{};
    std::unordered_map<std::string, std::atomic<bool>> _lockMap{};

    std::shared_ptr<MFA::RT::GpuTexture> _errorTexture{};
};
