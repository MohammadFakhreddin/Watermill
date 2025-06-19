#pragma once

#include "RenderTypes.hpp"

#include <glm/glm.hpp>

class IScene : public std::enable_shared_from_this<IScene>
{

public:

    virtual ~IScene() = default;

    virtual void Update(float deltaTime) = 0;

    virtual void UpdateBuffer(MFA::RT::CommandRecordState &recordState) = 0;

    virtual void Render(MFA::RT::CommandRecordState & recordState) = 0;

    virtual void Resize() = 0;

    virtual void Reload() = 0;

    virtual void UpdateInputAxis(glm::vec2 const &inputAxis) = 0;

    virtual void ButtonA_Changed(bool value) = 0;

    virtual void ButtonB_Pressed(bool value) = 0;

};
