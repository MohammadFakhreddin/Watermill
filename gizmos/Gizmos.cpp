//
// Created by moham on 9/20/2025.
//

#include "Gizmos.hpp"
#include "LineRenderer.hpp"
#include "PointRenderer.hpp"
#include "LinePipeline.hpp"
#include "PointPipeline.hpp"
#include "BufferTracker.hpp"
#include "RenderBackend.hpp"
#include "LogicalDevice.hpp"
#include "BedrockAssert.hpp"

namespace MFA
{
    //==================================================================================================================

    Gizmos::Gizmos(
        std::shared_ptr<DisplayRenderPass> const &displayRenderPass,
        glm::mat4 viewProjection
    )
    {
        // Set singleton instance
        MFA_ASSERT(Instance == nullptr);
        Instance = this;

        auto *device = LogicalDevice::Instance;
        // Create uniform buffer for view projection matrix
        std::shared_ptr viewProjectionBuffer = RB::CreateHostVisibleUniformBuffer(
            device->GetVkDevice(),
            device->GetPhysicalDevice(),
            sizeof(glm::mat4),
            device->GetMaxFramePerFlight()
        );
        // Create buffer tracker for view projection
        _viewProjectionTracker = std::make_unique<HostVisibleBufferTracker>(viewProjectionBuffer, Alias(viewProjection));

        {
            auto linePipeline = std::make_shared<LinePipeline>(displayRenderPass, viewProjectionBuffer, device->GetMaxFramePerFlight());
            _lineRenderer = std::make_unique<LineRenderer>(std::move(linePipeline));
        }
        {
            auto pointPipeline = std::make_shared<PointPipeline>(displayRenderPass, viewProjectionBuffer, device->GetMaxFramePerFlight());
            _pointRenderer = std::make_unique<PointRenderer>(std::move(pointPipeline));
        }
    }

    //==================================================================================================================

    Gizmos::~Gizmos()
    {
        MFA_ASSERT(Instance == this);
        Instance = nullptr;
    }

    //==================================================================================================================

    void Gizmos::SetViewProjection(glm::mat4 const &viewProjection)
    {
        if (Instance != nullptr) Instance->Private_SetViewProjection(viewProjection);
    }

    //==================================================================================================================

    void Gizmos::UpdateBuffers(RT::CommandRecordState &recordState) const
    {
        _lineRenderer->UpdateBuffers(recordState);
        _pointRenderer->UpdateBuffers(recordState);
    }

    //==================================================================================================================

    void Gizmos::Render(RT::CommandRecordState &recordState)
    {
        _viewProjectionTracker->Update(recordState);
        for (auto & renderTask : _renderTasks)
        {
            renderTask(recordState);
        }
        _renderTasks.clear();
    }

    //==================================================================================================================

    void Gizmos::Private_DrawLine(glm::vec3 const &from, glm::vec3 const &to, glm::vec4 const &color)
    {
        _renderTasks.emplace_back([this, from, to, color](RT::CommandRecordState &recordState) -> void
        {
            _lineRenderer->Draw(recordState, from, to, color);
        });
    }

    //==================================================================================================================

    void Gizmos::Private_DrawPoint(glm::vec3 const &position, glm::vec4 const &color, float pointSize)
    {
        _renderTasks.emplace_back([this, position, color, pointSize](RT::CommandRecordState &recordState) -> void
                                  { _pointRenderer->Draw(recordState, position, color, pointSize); });
    }

    //==================================================================================================================

    void Gizmos::Private_SetViewProjection(glm::mat4 const &viewProjection) const
    {
        _viewProjectionTracker->SetData(Alias(viewProjection));
    }

    //==================================================================================================================

}