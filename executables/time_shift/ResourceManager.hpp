#pragma once

#include "ThreadSafeQueue.hpp"
#include "RenderTypes.hpp"

#include <future>

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

    static std::shared_ptr<ResourceManager> Instance(bool createNewIfNotExists = false);

    ~ResourceManager();

    using ImageCallback = std::function<void(std::shared_ptr<MFA::RT::GpuTexture>)>;
    void RequestImage(char const * name, const ImageCallback & callback, bool forceReload = false);

    // TODO
    // std::future<std::shared_ptr<MFA::Blob>> RequestBlob(char const * name, bool forceReload = false);

    void UpdateBuffers(MFA::RenderTypes::CommandRecordState & recordState);

    // Temporary we should not need this
    void ForceCleanUp();

private:

    inline static std::weak_ptr<ResourceManager> _instance {};

    MFA::ThreadSafeQueue<std::function<void(ResourceManager * instance, MFA::RenderTypes::CommandRecordState & recordState)>> _nextUpdateTasks{};

    std::unordered_map<std::string, std::tuple<std::atomic<bool>, std::weak_ptr<MFA::RenderTypes::GpuTexture>>> _imageMap{};
    std::unordered_map<std::string, MFA::ThreadSafeQueue<ImageCallback>> _imageCallbacks{};

    struct QueuedImages
    {
        std::shared_ptr<MFA::RenderTypes::GpuTexture> gpuTexture;
        std::shared_ptr<MFA::RenderTypes::BufferAndMemory> stageBuffer;
        std::shared_ptr<MFA::RenderTypes::CommandBufferGroup> commandBufferGroup;
        int lifeTime = 0;
    };
    // Holding the buffers until we are sure they are fully processed
    MFA::ThreadSafeQueue<QueuedImages> _activeBuffers{};

};
