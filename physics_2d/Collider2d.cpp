#include "Collider2d.hpp"

namespace MFA
{
    Collider2d::Collider2d(
        Transform* transform,
        Physics2D::Layer layer,
        Physics2D::Layer layerMask,
        bool isTrigger
    )
        : mTransform(transform)
        , mLayer(layer)
        , mLayerMask(layerMask)
        , mIsTrigger(isTrigger)
    {
        if (mTransform)
        {
            mLastPosition = mTransform->GlobalPosition();
        }
    }

    Collider2d::~Collider2d()
    {
        if (mEntityId != 0 && Physics2D::Instance)
        {
            Physics2D::Instance->UnRegister(mEntityId);
        }
    }
}
