#include "ResourceManager.hpp"

#include "BedrockPath.hpp"
#include "LogicalDevice.hpp"
#include "ScopeLock.hpp"
#include "RenderTypes.hpp"
#include "JobSystem.hpp"
#include "ImportTexture.hpp"

using namespace MFA;

//======================================================================================================================

std::shared_ptr<ResourceManager> ResourceManager::Instance()
{
    std::shared_ptr<ResourceManager> shared_ptr = _instance.lock();
    if (shared_ptr == nullptr)
    {
        shared_ptr = std::make_shared<ResourceManager>();
        _instance = shared_ptr;
    }
    return shared_ptr;
}

//======================================================================================================================

ResourceManager::ImageFuture ResourceManager::RequestImage(char const * name_, bool const forceReload)
{
    auto promise = std::make_shared<ImagePromise>();

    auto & [lock, imageWeak] = _imageMap[name_];

    MFA_SCOPE_LOCK(lock);

    if (forceReload == false)
    {
        auto imageShared = imageWeak.lock();
        if (imageShared != nullptr)
        {
            promise->set_value(std::move(imageShared));
            return promise->get_future();
        }
    }

    auto future = promise->get_future();
    _imagePromises[name_].Push(promise);

    JobSystem::Instance()->AssignTask([this, name = std::string(name_)]()
    {
        auto const path = Path::Instance()->Get(name);
        auto const cpuTexture = Importer::UncompressedImage(path);

        _nextUpdateTasks.Push([cpuTexture, this, name](RT::CommandRecordState & recordState)
        {
            auto & [lock, imageWeak] = _imageMap[name];
            MFA_SCOPE_LOCK(lock);

            auto * device = LogicalDevice::Instance;

            auto [gpuTexture, stageBuffer] = RB::CreateTexture(
                *cpuTexture,
                device->GetVkDevice(),
                device->GetPhysicalDevice(),
                recordState.commandBuffer
            );

            _activeBuffers.emplace_back();
            auto & item = _activeBuffers.back();
            item.gpuTexture = gpuTexture;
            item.stageBuffer = stageBuffer;
            item.lifeTime = (int)device->GetMaxFramePerFlight() + 1;

            imageWeak = gpuTexture;

            auto & imagePromises = _imagePromises[name];
            while (imagePromises.IsEmpty() == false)
            {
                auto promise = imagePromises.Pop();
                promise->set_value(gpuTexture);
            }
        });
    });

    return future;
}

//======================================================================================================================

// std::future<std::shared_ptr<Blob>> ResourceManager::RequestBlob(char const * name, bool forceReload)
// {
//
// }

//======================================================================================================================

void ResourceManager::UpdateBuffers(RT::CommandRecordState & recordState)
{
    while (_nextUpdateTasks.IsEmpty() == false)
    {
        auto task = _nextUpdateTasks.Pop();
        task(recordState);
    }

    for (int i = (int)_activeBuffers.size() - 1; i >= 0; i--)
    {
        auto & item = _activeBuffers[i];
        item.lifeTime -= 1;
        if (item.lifeTime <= 0)
        {
            _activeBuffers.erase(_activeBuffers.begin() + i);
        }
    }
}

//======================================================================================================================
