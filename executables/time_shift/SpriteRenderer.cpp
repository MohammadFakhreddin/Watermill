#include "SpriteRenderer.hpp"

#include "LogicalDevice.hpp"

using namespace MFA;

//------------------------------------------------------------------------------------------------------------------

SpriteRenderer::SpriteRenderer(std::shared_ptr<Pipeline> pipeline)
    : _pipeline(std::move(pipeline))
{}

//------------------------------------------------------------------------------------------------------------------

std::tuple<std::unique_ptr<SpriteRenderer::SpriteData>, std::unique_ptr<SpriteRenderer::CommandBufferData>> SpriteRenderer::Allocate(
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

    auto const vertexAlias = Alias(vertexDataList.data(), vertexDataList.size());
    auto const indexAlias = Alias(indices, indexCount);

    auto vertexStageBuffer = RB::CreateStageBuffer(
        device->GetVkDevice(),
        device->GetPhysicalDevice(),
        vertexAlias.Len(),
        1
    );

    auto vertexBuffer = RB::CreateVertexBuffer(
        device->GetVkDevice(),
        device->GetPhysicalDevice(),
        commandBuffer,
        *vertexStageBuffer->buffers[0],
        vertexAlias
    );

    auto indexStageBuffer = RB::CreateStageBuffer(
        device->GetVkDevice(),
        device->GetPhysicalDevice(),
        indexAlias.Len(),
        1
    );

    auto indexBuffer = RB::CreateIndexBuffer(
    device->GetVkDevice(),
        device->GetPhysicalDevice(),
        commandBuffer,
        *indexStageBuffer->buffers[0],
        indexAlias
    );

    auto imageData = std::make_unique<SpriteData>(SpriteData{
        .vertexData = vertexBuffer,
        .indexCount = (size_t)indexCount,
        .indexData = indexBuffer,
        .descriptorSet = _pipeline->CreateDescriptorSet(gpuTexture),
    });

    auto commandBufferData = std::make_unique<CommandBufferData>(CommandBufferData{
        .vertexStageBuffer = vertexStageBuffer,
        .indexStageBuffer = indexStageBuffer
    });

    return std::tuple{std::move(imageData), std::move(commandBufferData)};
}

//------------------------------------------------------------------------------------------------------------------

// void SpriteRenderer::FreeImageData(SpriteData &imageData)
// {
//     imageData.vertexData.reset();
//     imageData.indexData.reset();
//     _pipeline->FreeDescriptorSet(imageData.descriptorSet);
// }

//------------------------------------------------------------------------------------------------------------------

void SpriteRenderer::Draw(
    RT::CommandRecordState &recordState,
    Pipeline::PushConstants const &pushConstants,
    SpriteData const &imageData
) const
{
    _pipeline->BindPipeline(recordState);

    _pipeline->SetPushConstant(recordState, pushConstants);

    RB::AutoBindDescriptorSet(recordState, RB::UpdateFrequency::PerPipeline, imageData.descriptorSet);

    RB::BindVertexBuffer(recordState, *imageData.vertexData);
    RB::BindIndexBuffer(recordState, *imageData.indexData, 0, RB::GetVkIndexType(sizeof(Index)));
    vkCmdDrawIndexed(recordState.commandBuffer, imageData.indexCount, 1, 0, 0, 0);
}

//------------------------------------------------------------------------------------------------------------------

