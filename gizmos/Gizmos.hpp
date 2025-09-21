#pragma once

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
        );

        static void DrawPoint(
            glm::vec3 const& position,
            glm::vec4 const& color = { 1.0f, 0.0f, 0.0f, 1.0f },
            float pointSize = 10.0f
        );

        bool Render(RT::CommandRecordState& recordState);

    private:

        void Private_DrawLine();

        void Private_DrawPoint();

        inline static Gizmos * Instance = nullptr;

        std::shared_ptr<DisplayRenderPass> _displayRenderPass{};

    };
}