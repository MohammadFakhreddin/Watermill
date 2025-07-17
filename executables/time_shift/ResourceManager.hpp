#pragma once

#include "RenderTypes.hpp"
#include "ThreadPool.hpp"

#include <filesystem>
#include <future>
#include <unordered_map>

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

class ResourceManager
{
public:

    static std::shared_ptr<ResourceManager> Instantiate();

    static void Destroy();

    explicit ResourceManager();
    ~ResourceManager();

    using ImageCallback = std::function<void(std::shared_ptr<MFA::RT::GpuTexture>)>;
    static void RequestImage(char const * name, const ImageCallback & callback);

    // Temporary we should not need this
    static void ForceCleanUp();

    [[nodiscard]]
    static std::shared_ptr<MFA::RT::GpuTexture> ErrorTexture()
    {
        auto rc = _instance.lock();
        if (rc == nullptr)
        {
            return nullptr;
        }
        return rc->_errorTexture;
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

    enum class Priority : int
    {
        Low = 0,
        High = 1,
    };
    void RequestNextMipmap(std::weak_ptr<ImageData> imageData, int nextMipLevel, Priority priority);

    inline static std::weak_ptr<ResourceManager> _instance {};

    std::unordered_map<std::string, std::shared_ptr<ImageData>> _imageMap{};
    std::unordered_map<std::string, std::atomic<bool>> _lockMap{};

    std::shared_ptr<MFA::RT::GpuTexture> _errorTexture{};

    static constexpr int MaxThreadCount = 2;
    static constexpr int MinThreadCount = 1;
    MFA::ThreadPool threadPool{std::max(std::min(MaxThreadCount, (int)(std::thread::hardware_concurrency() * 0.5f)), MinThreadCount)};

};
