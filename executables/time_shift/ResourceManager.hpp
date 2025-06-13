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

    static std::shared_ptr<ResourceManager> Instance();

    using ImageCallback = std::function<void(std::shared_ptr<MFA::RT::GpuTexture>)>;
    void RequestImage(char const * name, const ImageCallback & callback, bool forceReload = false);

    // TODO
    // std::future<std::shared_ptr<MFA::Blob>> RequestBlob(char const * name, bool forceReload = false);

    void UpdateBuffers(MFA::RenderTypes::CommandRecordState & recordState);

private:

    using ImagePromise = std::promise<std::shared_ptr<MFA::RT::GpuTexture>>;
    // using ImagePromise = std::promise<std::shared_ptr<MFA::RT::GpuTexture>>;

    inline static std::weak_ptr<ResourceManager> _instance {};

    MFA::ThreadSafeQueue<std::function<void(MFA::RenderTypes::CommandRecordState & recordState)>> _nextUpdateTasks{};

    std::unordered_map<std::string, std::tuple<std::atomic<bool>, std::weak_ptr<MFA::RenderTypes::GpuTexture>>> _imageMap{};
    std::unordered_map<std::string, MFA::ThreadSafeQueue<ImageCallback>> _imageCallbacks{};

    struct QueuedImages
    {
        std::shared_ptr<MFA::RenderTypes::GpuTexture> gpuTexture;
        std::shared_ptr<MFA::RenderTypes::BufferAndMemory> stageBuffer;
        int lifeTime = 0;
    };
    // Holding the buffers until we are sure they are fully processed
    std::vector<QueuedImages> _activeBuffers{};

};
