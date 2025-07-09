#pragma once

#include "ThreadSafeQueue.hpp"
#include "RenderTypes.hpp"

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

class ResourceManager : public std::enable_shared_from_this<ResourceManager>
{
public:

    static std::shared_ptr<ResourceManager> Instance(bool createNewIfNotExists = false);

    ~ResourceManager();

    using ImageCallback = std::function<void(std::shared_ptr<MFA::RT::GpuTexture>)>;
    void RequestImage(char const * name, const ImageCallback & callback, bool forceReload = false);

    // Temporary we should not need this
    void ForceCleanUp();

private:

    inline static std::weak_ptr<ResourceManager> _instance {};

    std::unordered_map<std::string, std::tuple<std::atomic<bool>, std::weak_ptr<MFA::RenderTypes::GpuTexture>>> _imageMap{};
    std::unordered_map<std::string, MFA::ThreadSafeQueue<ImageCallback>> _imageCallbacks{};
};
