#include "ResourceManager.hpp"

#include "BedrockPath.hpp"
#include "ImportTexture.hpp"
#include "LogicalDevice.hpp"
#include "RenderTypes.hpp"
#include "ScopeLock.hpp"
#include "Time.hpp"
#include "imgui.h"
#include "tiny_gltf_loader.h"

using namespace MFA;

//======================================================================================================================

std::shared_ptr<ResourceManager> ResourceManager::Instantiate()
{
    std::shared_ptr<ResourceManager> shared_ptr = _instance.lock();
    if (shared_ptr == nullptr)
    {
        shared_ptr = std::make_shared<ResourceManager>();
        _instance = shared_ptr;
    }
    return shared_ptr;
}

void ResourceManager::Destroy()
{
    auto rc = _instance.lock();
    if (rc == nullptr)
    {
        return;
    }
    _instance.reset();
    rc->threadPool.Terminate();
}

//======================================================================================================================

ResourceManager::ResourceManager()
{
    // TODO: The other approach should have worked as well
    // LogicalDevice::Instance->AddRenderTask([](RT::CommandRecordState & recordState)->bool
    // {
    auto * logicalDevice = LogicalDevice::Instance;
    auto * device = logicalDevice->GetVkDevice();
    auto * physicalDevice = logicalDevice->GetPhysicalDevice();
    auto * commandPool = logicalDevice->GetGraphicCommandPool();
    auto * graphicQueue = logicalDevice->GetGraphicQueue();

    auto commandBuffer = RB::BeginSingleTimeCommand(device, *commandPool);

    auto cpuTexture = Importer::ErrorTexture();
    std::vector<uint8_t> mipLevel{0};
    auto [gpuTexture, stageBuffer] = RB::CreateTexture(
        *cpuTexture,
        device,
        physicalDevice,
        // recordState.commandBuffer,
        commandBuffer,
        (int)mipLevel.size(),
        mipLevel.data()
    );

    _errorTexture = std::move(gpuTexture);

    RB::EndAndSubmitSingleTimeCommand(device, *commandPool, graphicQueue, commandBuffer);
        // return false;
    // });
}

//======================================================================================================================

ResourceManager::~ResourceManager() = default;

//======================================================================================================================

void ResourceManager::RequestImage(char const * nameRaw_, const ImageCallback & callback)
{
    MFA_ASSERT(callback != nullptr);

    std::weak_ptr rcWeak = _instance;
    auto rc = rcWeak.lock();
    if (rc == nullptr)
    {
        return;
    }

    std::string imagePath{nameRaw_};
    // We do not want any spin lock to exists in the main thread for any reason.
    rc->threadPool.AssignTask((int)Priority::High, [rcWeak, imagePath, callback]()->void
    {
        auto rc = rcWeak.lock();
        if (rc == nullptr)
        {
            return;
        }

        auto & lock = rc->_lockMap[imagePath];
        MFA_SCOPE_LOCK(lock);

        auto & imageData = rc->_imageMap[imagePath];
        if (imageData == nullptr)
        {
            imageData = std::make_shared<ImageData>();
            imageData->path = imagePath;
            imageData->currentMipLevel = -1;

            std::filesystem::path originalPath{imagePath};
            std::filesystem::path parentPath = originalPath.parent_path().string();
            std::filesystem::path stem = originalPath.stem().string();
            std::string extension = originalPath.extension().string();

            int mipIdx = 0;
            while(true)
            {
                auto mipPath = (parentPath / stem).string() + "_mip_" + std::to_string(mipIdx) + extension;
                if (std::filesystem::exists(mipPath) == false)
                {
                    break;
                }
                ++mipIdx;
            }

            imageData->mipCount = mipIdx;
            imageData->hasMipmaps = mipIdx > 0;
            imageData->currentMipLevel = imageData->mipCount;
        }
        else
        {
            auto imageShared = imageData->gpuTexture.lock();
            if (imageShared != nullptr)
            {
                callback(imageShared);

                // We have loaded the highest mip level already
                if (imageData->hasMipmaps == false || imageData->currentMipLevel == 0)
                {
                    return;
                }
            }
        }

        imageData->callbacks.emplace_back(callback);
        // To make sure we are not requesting things multiple times
        if (imageData->callbacks.size() != 1)
        {
            return;
        }

        imageData->currentMipLevel = imageData->mipCount;
        rc->RequestNextMipmap(imageData, imageData->currentMipLevel - 1, Priority::High);
    });
}

//======================================================================================================================

void ResourceManager::ForceCleanUp()
{
    auto rc = _instance.lock();
    if (rc == nullptr)
    {
        return;
    }

    rc->threadPool.CancelTasks();
    rc->_imageMap.clear();
    rc->_lockMap.clear();
}

//======================================================================================================================

void ResourceManager::RequestNextMipmap(std::weak_ptr<ImageData> imageDataWeak, int nextMipLevel, Priority priority)
{
    MFA_ASSERT(nextMipLevel >= 0);

    // MFA_LOG_INFO("Mipmap requested: %s", imagePath.c_str());

    std::weak_ptr rcWeak = _instance;

    threadPool.AssignTask((int)priority, [priority, rcWeak, imageDataWeak = std::move(imageDataWeak), nextMipLevel]()
    {
        auto rc = rcWeak.lock();
        if (rc == nullptr)
        {
            return;
        }

        auto imageData = imageDataWeak.lock();
        if (imageData == nullptr)
        {
            return;
        }

        std::string name{imageData->path.string()};
        if (imageData->hasMipmaps == true)
        {
            std::filesystem::path originalPath{name};
            std::filesystem::path parentPath = originalPath.parent_path().string();
            std::filesystem::path stem = originalPath.stem().string();
            std::string extension = originalPath.extension().string();
            auto mipPath = (parentPath / stem).string() + "_mip_" + std::to_string(nextMipLevel) + originalPath.extension().string();

            name = mipPath;
        }

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

        std::shared_ptr<Asset::Texture> cpuTexture;

        std::filesystem::path const path = Path::Instance()->Get(name);
        // if (path.extension() == ".ktx2")
        // {
        //     cpuTexture = Importer::LoadKtxMetadata(path.string().c_str());
        // }
        // else
        // {
        cpuTexture = Importer::UncompressedImage(path.string());
        // }

        auto const buffer = cpuTexture->GetMipmapBuffer(0);
        if (buffer == nullptr || buffer->IsValid() == false)
        {
            MFA_LOG_WARN("Failed to load image: %s", name.c_str());
            return;
        }
        // MFA_ASSERT(buffer != nullptr && buffer->IsValid() == true);

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

        struct QueuedImages
        {
            std::shared_ptr<RenderTypes::GpuTexture> gpuTexture;
            std::shared_ptr<RenderTypes::BufferAndMemory> stageBuffer;
            std::shared_ptr<RenderTypes::CommandBufferGroup> commandBufferGroup;
            int lifeTime = 0;
        };
        std::shared_ptr imageBuffer = std::make_shared<QueuedImages>(QueuedImages{
            .gpuTexture = gpuTexture,
            .stageBuffer = stageBuffer,
            .commandBufferGroup = commandBufferGroup,
            .lifeTime = (int)LogicalDevice::Instance->GetMaxFramePerFlight() + 1
        });
        LogicalDevice::AddRenderTask([imageBuffer](const RT::CommandRecordState & recordState)->bool
        {
            imageBuffer->lifeTime -= 1;
            if (imageBuffer->lifeTime < 0)
            {
                return false;
            }
            return true;
        });

        LogicalDevice::AddRenderTask([
            rcWeak,
            name = std::move(name),
            commandBufferGroup,
            imageDataWeak,
            gpuTexture,
            nextMipLevel,
            priority
        ](const RT::CommandRecordState & recordState)->bool
        {
            auto rc = rcWeak.lock();
            if (rc == nullptr)
            {
                return false;
            }

            auto imageData = imageDataWeak.lock();
            if (imageData == nullptr)
            {
                return false;
            }

            RB::ExecuteCommandBuffer(
                recordState.commandBuffer,
                *commandBufferGroup
            );

            rc->threadPool.AssignTask((int)priority, [rcWeak, gpuTexture, imageDataWeak, nextMipLevel]()->void
            {
                auto rc = rcWeak.lock();
                if (rc == nullptr)
                {
                    return;
                }

                auto imageData = imageDataWeak.lock();
                if (imageData == nullptr)
                {
                    return;
                }

                auto & lock = rc->_lockMap[imageData->path.string()];
                MFA_SCOPE_LOCK(lock);

                imageData->gpuTexture = gpuTexture;
                imageData->currentMipLevel = nextMipLevel;

                for (int i = 0; i < imageData->callbacks.size(); ++i)
                {
                    auto & callback = imageData->callbacks[i];
                    callback(gpuTexture);
                }

                if (imageData->hasMipmaps == true && imageData->currentMipLevel > 0)
                {
                    rc->RequestNextMipmap(imageData, imageData->currentMipLevel / 2, Priority::Low);
                }
                else
                {
                    imageData->callbacks.clear();
                }
            });

            return false;
        });
    });
}

//======================================================================================================================
