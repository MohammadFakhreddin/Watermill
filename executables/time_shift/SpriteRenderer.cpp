#include "SpriteRenderer.hpp"

#include "LogicalDevice.hpp"

using namespace MFA;

//------------------------------------------------------------------------------------------------------------------

SpriteRenderer::SpriteRenderer(std::shared_ptr<Pipeline> pipeline)
    : _pipeline(std::move(pipeline))
{}

//------------------------------------------------------------------------------------------------------------------

std::unique_ptr<SpriteRenderer::SpriteData> SpriteRenderer::AllocateImageData(
    VkCommandBuffer commandBuffer,

    RT::GpuTexture const &gpuTexture,

    int const vertexCount,
    Position const * vertices,
    UV const * uvs,

    int const indexCount,
    Index const * indices
) const
{
    auto const device = LogicalDevice::Instance;

    std::vector<Pipeline::Vertex> vertexDataList{};
    for (int i = 0; i < vertexCount; ++i)
    {
        vertexDataList.emplace_back();
        auto & vertexData = vertexDataList.back();
        vertexData.position = vertices[i];
        vertexData.uv = uvs[i];
    }

    auto const vertexBuffer = RB::CreateVertexBuffer(
        device->GetVkDevice(),
        device->GetPhysicalDevice(),
        sizeof(Pipeline::Vertex) * vertexCount,
        1
    );

    auto const vertexStageBuffer = RB::CreateStageBuffer(
        device->GetVkDevice(),
        device->GetPhysicalDevice(),
        vertexBuffer->bufferSize,
        1
    );

    auto const indexBuffer = RB::CreateIndexBuffer(
        device->GetVkDevice(),
        device->GetPhysicalDevice(),
        sizeof(Pipeline::Index) * indexCount,
        1
    );

    auto const indexStageBuffer = RB::CreateStageBuffer(
        device->GetVkDevice(),
        device->GetPhysicalDevice(),
        vertexBuffer->bufferSize,
        1
    );

    auto imageData = std::make_unique<SpriteData>(SpriteData{
        .vertexData = LocalBufferTracker(
            vertexBuffer,
            vertexStageBuffer,
            Alias(vertexDataList.data(), vertexDataList.size() * vertexCount)
        ),
        .indexData = LocalBufferTracker(
            indexBuffer,
            indexStageBuffer,
            Alias(indices, sizeof(Index) * indexCount)

        ),
        .descriptorSet = _pipeline->CreateDescriptorSet(gpuTexture)
    });

    return imageData;
}

//------------------------------------------------------------------------------------------------------------------

void SpriteRenderer::FreeImageData(SpriteData &imageData) {}

//------------------------------------------------------------------------------------------------------------------

void SpriteRenderer::Draw(
    RT::CommandRecordState &recordState,
    Pipeline::PushConstants const &pushConstants,
    SpriteData const &imageData
) const
{
}

//------------------------------------------------------------------------------------------------------------------

