#include "PatrolEnemy.hpp"

#include "BedrockMath.hpp"

using namespace MFA;

//======================================================================================================================

PatrolEnemy::PatrolEnemy(
    Transform * transform,
    float const speed,
    std::vector<glm::vec2> patrolPoints
)
    : _transform(transform)
    , _speed(speed)
    , _patrolPoints(std::move(patrolPoints))
{
    _originalScaleX = transform->GetLocalScale().x;

    glm::vec2 targetPosition = _patrolPoints[_currentIndex];
    glm::vec3 currentPosition = _transform->GlobalPosition();
    _transform->SetGlobalPosition(glm::vec3{targetPosition, currentPosition.z});

}

//======================================================================================================================

void PatrolEnemy::Update(float deltaTime)
{
    glm::vec2 targetPosition = _patrolPoints[_currentIndex + _direction];
    glm::vec3 currentPosition = _transform->GlobalPosition();
    auto nextPosition = Math::MoveTowards(currentPosition, targetPosition, _speed * deltaTime);
    _transform->SetGlobalPosition(glm::vec3{nextPosition, currentPosition.z});

    // This is where having the entity system would have helped, I can invert the scale for now
    bool isMovingRight = glm::dot((targetPosition - glm::vec2(currentPosition)), Math::RightVec2) > 0.001f;
    auto scale = _transform->GetLocalScale();
    scale.x = _originalScaleX * (isMovingRight == false ? 1.0f : -1.0f);
    _transform->SetLocalScale(scale);

    if (glm::length2(nextPosition - targetPosition) < 0.01f)
    {
        _currentIndex += _direction;

        // Turn around at the ends
        if (_currentIndex == 0 || _currentIndex == _patrolPoints.size() - 1)
        {
            _direction *= -1;
        }
    }
}

//======================================================================================================================
