#pragma once

#include "RenderTypes.hpp"

#include <glm/glm.hpp>
#include <functional>

struct InputParams
{
    std::function<glm::vec2()> InputAxis;
    std::function<bool()> IsButtonA_Pressed;
    std::function<bool()> IsButtonB_Pressed;
};

class IScene
{

public:

    virtual ~IScene() = default;

    virtual void Update(float deltaTime) = 0;

    virtual void UpdateBuffer(MFA::RT::CommandRecordState &recordState) = 0;

    virtual void Render(MFA::RT::CommandRecordState & recordState) = 0;

    virtual void Resize() = 0;

    virtual void Reload() = 0;

};
