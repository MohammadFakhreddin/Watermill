#include "PointRenderer.hpp"

#include "LogicalDevice.hpp"
#include "BedrockMath.hpp"
#include "BufferTracker.hpp"

namespace MFA
{

	//-------------------------------------------------------------------------------------------------

	PointRenderer::PointRenderer(std::shared_ptr<MFA::PointPipeline> pointPipeline)
		: _pointPipeline(std::move(pointPipeline))
	{}

	//-------------------------------------------------------------------------------------------------

	void PointRenderer::Draw(
	    RT::CommandRecordState &recordState,
	    glm::vec3 const &position,
	    glm::vec4 const &color,
        float const pointSize
    )
    {
        using namespace MFA;

	    if (_vertexBuffer == nullptr || _indexBuffer == nullptr)
	    {
	        PrepareBuffers(recordState);
	    }

        _pointPipeline->BindPipeline(recordState);

        glm::mat4 matrix = Math::Translate(position);

        _pointPipeline->SetPushConstants(
            recordState, PointPipeline::PushConstants{.model = matrix, .color = color, .pointSize = pointSize});

        RB::BindIndexBuffer(recordState, *_indexBuffer, 0, VK_INDEX_TYPE_UINT16);

        RB::BindVertexBuffer(recordState, *_vertexBuffer, 0, 0);

        RB::DrawIndexed(recordState, _indexCount);
    }

    //-------------------------------------------------------------------------------------------------

    void PointRenderer::PrepareBuffers(RT::CommandRecordState & recordState)
	{
	    std::vector<glm::vec3> vertices{ {0.0f, 0.0f, 0.0f} };
	    std::vector<uint16_t> indices{ 0 };

	    Alias const verticesAlias{ vertices.data(), vertices.size() };
	    Alias const indicesAlias{ indices.data(), indices.size() };

	    auto* device = LogicalDevice::Instance;

	    auto const vertexStageBuffer = RB::CreateStageBuffer(
            device->GetVkDevice(),
            device->GetPhysicalDevice(),
            verticesAlias.Len(),
            1
        );
	    _vertexBuffer = RB::CreateVertexBuffer(
            device->GetVkDevice(),
            device->GetPhysicalDevice(),
            recordState.commandBuffer,
            *vertexStageBuffer->buffers[0],
            verticesAlias
        );
	    auto const indexStageBuffer = RB::CreateStageBuffer(
            device->GetVkDevice(),
            device->GetPhysicalDevice(),
            indicesAlias.Len(),
            1
        );
	    _indexBuffer = RB::CreateIndexBuffer(
            device->GetVkDevice(),
            device->GetPhysicalDevice(),
            recordState.commandBuffer,
            *indexStageBuffer->buffers[0],
            indicesAlias
        );
	    _indexCount = indices.size();
	    // TODO: Create a new function called addTemporaryMemory that you just pass the memories you want it to preseve
	    std::shared_ptr<int> counter = std::make_shared<int>(LogicalDevice::Instance->GetMaxFramePerFlight() + 1);
	    LogicalDevice::AddRenderTask([indexStageBuffer, vertexStageBuffer, counter](RT::CommandRecordState const & recordState)->bool
        {
            (*counter) -= 1;
            if (*counter <= 0)
            {
                return false;
            }
            return true;
        });
	}

    //-------------------------------------------------------------------------------------------------

}