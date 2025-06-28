#include "ResourceManager.hpp"

#include "BedrockPath.hpp"
#include "LogicalDevice.hpp"
#include "ScopeLock.hpp"
#include "RenderTypes.hpp"
#include "JobSystem.hpp"
#include "ImportTexture.hpp"

using namespace MFA;

//======================================================================================================================

std::shared_ptr<ResourceManager> ResourceManager::Instance(bool const createNewIfNotExists)
{
    std::shared_ptr<ResourceManager> shared_ptr = _instance.lock();
    if (shared_ptr == nullptr && createNewIfNotExists == true)
    {
        shared_ptr = std::make_shared<ResourceManager>();
        _instance = shared_ptr;
    }
    return shared_ptr;
}

//======================================================================================================================

ResourceManager::~ResourceManager() = default;

//======================================================================================================================

void ResourceManager::RequestImage(char const * name_, const ImageCallback & callback, bool const forceReload)
{
    auto & [lock, imageWeak] = _imageMap[name_];

    MFA_SCOPE_LOCK(lock);

    if (forceReload == false)
    {
        auto imageShared = imageWeak.lock();
        if (imageShared != nullptr)
        {
            callback(imageShared);
        }
    }

    _imageCallbacks[name_].Push(callback);
    // To make sure we are not requesting things multiple times
    if (_imageCallbacks[name_].ItemCount() != 1)
    {
        return;
    }

    // TODO: In the new design, our functions should return a command buffer that they have allocated themselves and we just combine them together for execution
    JobSystem::Instance()->AssignTask([name = std::string(name_)]()
    {
        auto * logicalDevice = LogicalDevice::Instance;
        auto * device = logicalDevice->GetVkDevice();
        auto * commandPool = logicalDevice->GetGraphicCommandPool();
        auto * physicalDevice = logicalDevice->GetPhysicalDevice();
        MFA_SCOPE_LOCK(commandPool->lock);

        auto commandBufferGroup = RB::BeginSecondaryCommand(
            device,
            *commandPool
        );

        auto commandBuffer = commandBufferGroup->commandBuffers[0];

        auto const path = Path::Instance()->Get(name);
        auto const cpuTexture = Importer::UncompressedImage(path);

        auto const buffer = cpuTexture->GetMipmapBuffer(0);
        MFA_ASSERT(buffer != nullptr && buffer->IsValid() == true);

        std::vector<uint8_t> mipLevel{0};
        auto [gpuTexture, stageBuffer] = RB::CreateTexture(
            *cpuTexture,
            device,
            physicalDevice,
            commandBuffer,
            mipLevel.size(),
            mipLevel.data()
        );

        RB::EndCommandBuffer(commandBuffer);

        auto const instance = _instance.lock();
        if (instance == nullptr)
        {
            return;
        }

        auto & [lock, imageWeak] = instance->_imageMap[name];
        MFA_SCOPE_LOCK(lock);

        QueuedImages item {};
        item.gpuTexture = gpuTexture;
        item.stageBuffer = stageBuffer;
        item.commandBufferGroup = commandBufferGroup;
        item.lifeTime = (int)LogicalDevice::Instance->GetMaxFramePerFlight() + 1;
        instance->_activeBuffers.Push(item);

        imageWeak = gpuTexture;

        instance->_nextUpdateTasks.Push([name = std::move(name), commandBuffer](ResourceManager * instance, const RT::CommandRecordState & recordState)
        {
            MFA_ASSERT(JobSystem::Instance() == nullptr || JobSystem::Instance()->IsMainThread() == true);

            vkCmdExecuteCommands(recordState.commandBuffer, 1, &commandBuffer);

            auto & [lock, imageWeak] = instance->_imageMap[name];
            auto gpuTexture = imageWeak.lock();
            if (gpuTexture != nullptr)
            {
                // I think it is better to call these callback from the main thread.
                auto & imageCallbacks = instance->_imageCallbacks[name];
                while (imageCallbacks.IsEmpty() == false)
                {
                    auto callback = imageCallbacks.Pop();
                    callback(gpuTexture);
                }
            }

            instance->_imageCallbacks.erase(name);

        });
    });
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
        task(this, recordState);
    }

    {
        auto const  count = _activeBuffers.ItemCount();
        for (int i = 0; i < count; i++)
        {
            auto item = _activeBuffers.Pop();
            item.lifeTime -= 1;
            if (item.lifeTime > 0)
            {
                _activeBuffers.Push(std::move(item));
            }
        }
    }
}

//======================================================================================================================

void ResourceManager::ForceCleanUp()
{
    while (_nextUpdateTasks.IsEmpty() == false)
    {
        _nextUpdateTasks.Pop();
    }

    _imageMap.clear();
    _imageCallbacks.clear();
}

//======================================================================================================================
