#include "GridRenderer.hpp"

#include "LogicalDevice.hpp"
#include "RenderBackend.hpp"
#include "ShapeGenerator.hpp"

using namespace MFA;

//======================================================================================================================

GridRenderer::GridRenderer(std::shared_ptr<Pipeline> pipeline)
	: _pipeline(std::move(pipeline))
{
    AllocateBuffers();
}

//======================================================================================================================

void GridRenderer::Draw(RT::CommandRecordState &recordState, Pipeline::PushConstants const &pushConstants) const
{
    // Note: We could have achieve the same thing with viewport and scissor and a fixed vertex buffer.
    _pipeline->BindPipeline(recordState);
    _pipeline->SetPushConstant(recordState, pushConstants);
    RB::BindVertexBuffer(recordState, *_vertexBuffer);
    RB::BindIndexBuffer(recordState, *_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(recordState.commandBuffer, _indexCount, 1, 0, 0, 0);
}

//======================================================================================================================

void GridRenderer::AllocateBuffers()
{
    auto const [
        vertices,
        indices,
        normals
    ] = ShapeGenerator::Quad();

    auto const vertexAlias = Alias(vertices.data(), vertices.size());
    auto const indexAlias = Alias(indices.data(), indices.size());
    _indexCount = (int)indices.size();

    auto * device = LogicalDevice::Instance;

    const auto cb = RB::BeginSingleTimeCommand(device->GetVkDevice(), *device->GetGraphicCommandPool());

    auto const vertexStageBuffer = RB::CreateStageBuffer(
        device->GetVkDevice(),
        device->GetPhysicalDevice(),
        vertexAlias.Len(),
        1
    );

    _vertexBuffer = RB::CreateVertexBuffer(
        device->GetVkDevice(),
        device->GetPhysicalDevice(),
        cb,
        *vertexStageBuffer->buffers[0],
        vertexAlias
    );

    auto const indexStageBuffer = RB::CreateStageBuffer(
        device->GetVkDevice(),
        device->GetPhysicalDevice(),
        indexAlias.Len(),
        1
    );

    _indexBuffer = RB::CreateIndexBuffer(
    device->GetVkDevice(),
        device->GetPhysicalDevice(),
        cb,
        *indexStageBuffer->buffers[0],
        indexAlias
    );

    RB::EndAndSubmitSingleTimeCommand(
        device->GetVkDevice(),
        *device->GetGraphicCommandPool(),
        device->GetGraphicQueue(),
        cb
    );
}

//======================================================================================================================
