#pragma once

#include <ext/matrix_transform.hpp>
#include <functional>


#include "RenderTypes.hpp"
#include "render_pass/DisplayRenderPass.hpp"

namespace MFA
{
    // Forward declarations
    class LineRenderer;
    class PointRenderer;
    class LinePipeline;
    class PointPipeline;
    class HostVisibleBufferTracker;

    class Gizmos
    {
    public:

        explicit Gizmos(
            std::shared_ptr<DisplayRenderPass> const & displayRenderPass,
            glm::mat4 viewProjection = glm::identity<glm::mat4>()
        );

        ~Gizmos();

        static void DrawLine(
            glm::vec3 const& from,
            glm::vec3 const& to,
            glm::vec4 const& color = { 0.0f, 1.0f, 0.0f, 1.0f }
        )
        {
            if (Instance != nullptr)
            {
                Instance->Private_DrawLine(from, to, color);
            }
        }

        static void DrawPoint(
            glm::vec3 const& position,
            glm::vec4 const& color = { 1.0f, 0.0f, 0.0f, 1.0f },
            float pointSize = 10.0f
        )
        {
            if (Instance != nullptr)
            {
                Instance->Private_DrawPoint(position, color, pointSize);
            }
        }

        void SetViewProjection(glm::mat4 const & viewProjection) const;

        void Render(RT::CommandRecordState& recordState);

    private:

        void Private_DrawLine(
            glm::vec3 const& from,
            glm::vec3 const& to,
            glm::vec4 const& color
        );

        void Private_DrawPoint(
            glm::vec3 const& position,
            glm::vec4 const& color,
            float pointSize
        );

        inline static Gizmos * Instance = nullptr;

        // Renderers
        std::unique_ptr<LineRenderer> _lineRenderer{};
        std::unique_ptr<PointRenderer> _pointRenderer{};

        // ViewProjection buffer tracker
        std::unique_ptr<HostVisibleBufferTracker> _viewProjectionTracker{};

        std::vector<std::function<void(RT::CommandRecordState &)>> _renderTasks{};
    };
}