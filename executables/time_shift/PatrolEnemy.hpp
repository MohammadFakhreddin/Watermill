#pragma once

#include "Transform.hpp"

#include <glm/glm.hpp>

#include <vector>

class PatrolEnemy
{
public:

    explicit PatrolEnemy(
        MFA::Transform * transform,
        float speed,
        std::vector<glm::vec2> patrolPoints
    );

    void Update(float deltaTime);

private:

    MFA::Transform * _transform {};
    float _speed {};
    int _currentIndex = 0;
    int _direction = 1;
    float _originalScaleX {};

    std::vector<glm::vec2> _patrolPoints {};

};
