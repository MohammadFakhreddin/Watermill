#pragma once

#include "LineRenderer.hpp"
#include "PointPipeline.hpp"
#include "render_pass/DisplayRenderPass.hpp"

namespace MFA
{
    class Gizmos
    {
    public:

        explicit Gizmos(std::shared_ptr<DisplayRenderPass> displayRenderPass);

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

        void Render(RT::CommandRecordState& recordState, glm::mat4 const & viewProjection);

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

        // std::shared_ptr<DisplayRenderPass> _displayRenderPass{};
        std::unique_ptr<LineRenderer> _lineRenderer{};
        std::unique_ptr<PointPipeline> _pointPipeline{};

    };
}