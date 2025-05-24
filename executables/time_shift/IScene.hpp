#pragma once

#include "RenderTypes.hpp"
#include "WebViewContainer.hpp"

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
