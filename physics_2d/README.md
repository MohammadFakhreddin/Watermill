# Collision System Implementation Documentation

## Overview
This document outlines the collision system implementation completed today, including the signal-driven architecture, player movement system, and potential debugging approaches.

## Architecture Overview

### Core Components

1. **Collider2d Base Class** (`physics_2d/Collider2d.hpp`)
   - Base class for all 2D colliders
   - Manages Transform reference and Physics2D entity registration
   - Handles layer and layer mask configuration
   - Automatic cleanup in destructor

2. **AABB_Collider Derived Class** (`physics_2d/AABB_Collider.hpp`)
   - Axis-Aligned Bounding Box collision detection
   - Size and Offset properties with dirty tracking
   - Signal-driven updates using Transform's DirtyListener

3. **Player Class** (`executables/time_shift/Player.hpp`)
   - Handles player movement input and physics interaction
   - References Transform and AABB_Collider for movement
   - Configurable movement speed (currently 0.1f)

4. **Transform Signal System** (`engine/entity_system/Transform.hpp`)
   - DirtyListener signal emits when transform changes
   - Enables efficient, event-driven collision updates

## Signal-Driven Architecture

### Transform Dirty Notifications
```cpp
// Transform.hpp
Signal<> DirtyListener;

// Transform.cpp - SetGlobalTransformDirty()
DirtyListener.Emit();

// AABB_Collider.cpp - Constructor
mTransformDirtySignalId = mTransform->DirtyListener.Register(this, &AABB_Collider::SetTransformDirty);
```

### Efficiency Benefits
- Physics2D.move only called when position actually changes
- No per-frame collision updates for stationary objects
- Automatic cleanup via signal unregistration in destructor

## Player Movement System

### Input Flow
1. GameScene::UpdateInputAxis receives input
2. Calls _player->SetMovementInput(inputAxis)
3. Player::Update processes movement in next frame
4. Transform position changes trigger collision detection

### Movement Logic
```cpp
// Player.cpp - Update()
glm::vec3 moveVector = glm::vec3(mMovementInput.xy, 0.0f) * mMoveSpeed * deltaTime;
glm::vec3 newPosition = currentPosition + moveVector;
mTransform->SetGlobalPosition(newPosition);
```

## Collision Detection

### AABB Sweep Collision
- Non-trigger colliders use raycast-based sweep collision
- Four corner points tested for precise collision detection
- Sliding collision response along surfaces
- Epsilon-based collision resolution to prevent tunneling

### Trigger vs Solid Colliders
- Triggers: Simple position update without collision response
- Solids: Complex sweep collision with surface sliding

## GameScene Integration

### Player Discovery
```cpp
// GameScene.cpp - ReadLevelFromJson()
if (tag == Constants::GameTags::Player) {
    _player = std::make_unique<Player>(transform.get(), collider.get(), 0.1f);
}
```

### Input Wiring
```cpp
// GameScene.cpp - UpdateInputAxis()
if (_player != nullptr) {
    _player->SetMovementInput(inputAxis);
}
```

## Potential Issues and Debugging

### 1. Movement Speed Issues
**Current Status**: Movement speed set to 0.1f
**Potential Problem**: May be too slow to be noticeable
**Debug Steps**:
- Temporarily increase movement speed to 5.0f for testing
- Add debug logging to Player::Update() to verify input reception
- Check if deltaTime is reasonable (should be ~0.016f for 60fps)

### 2. Collision Layer Configuration
**Potential Problem**: Player collider may not be on correct physics layer
**Debug Steps**:
- Verify DeterminePhysicsLayer() returns correct layer for Player tag
- Check if player layer mask allows collision with environment
- Add logging to AABB_Collider constructor to verify layer setup

### 3. Transform Signal Flow
**Potential Problem**: Signal may not be properly connected or firing
**Debug Steps**:
- Add logging to AABB_Collider::SetTransformDirty() to verify signal reception
- Verify mTransformDirtySignalId != SignalIdInvalid after registration
- Check Transform::SetGlobalTransformDirty() is being called

### 4. Physics2D Integration
**Potential Problem**: Physics2D may not be properly initialized or configured
**Debug Steps**:
- Verify _physics2D is initialized in GameScene
- Check Physics2D::Instance is not null in AABB_Collider
- Verify entity registration succeeded (mEntityId != 0)

### 5. Input System
**Potential Problem**: Input may not be reaching the player
**Debug Steps**:
- Add logging to GameScene::UpdateInputAxis() to verify input reception
- Log Player::SetMovementInput() calls
- Verify _player is not null when input is processed

## Code Locations

### Key Files Modified
- `physics_2d/Collider2d.hpp` - Base collider class
- `physics_2d/Collider2d.cpp` - Base implementation
- `physics_2d/AABB_Collider.hpp` - AABB collider with signal integration
- `physics_2d/AABB_Collider.cpp` - Complex collision detection logic
- `engine/entity_system/Transform.hpp` - Added DirtyListener signal
- `engine/entity_system/Transform.cpp` - Signal emission in SetGlobalTransformDirty()
- `executables/time_shift/Player.hpp` - Player class definition
- `executables/time_shift/Player.cpp` - Movement logic implementation
- `executables/time_shift/GameScene.hpp` - Added player member
- `executables/time_shift/GameScene.cpp` - Player discovery and input wiring

### Critical Code Sections
- AABB_Collider constructor: Signal registration at line 37
- Player::Update(): Movement logic at lines 22-34
- GameScene player creation: Around line where Player tag is handled
- Transform signal emission: SetGlobalTransformDirty() method

## Testing Strategy for Tomorrow

### 1. Basic Functionality Test
```cpp
// Add to Player::Update() for debugging
if (glm::length(mMovementInput) > 0.0f) {
    printf("Player input: %f, %f\n", mMovementInput.x, mMovementInput.y);
    printf("Move vector: %f, %f\n", moveVector.x, moveVector.y);
    printf("New position: %f, %f\n", newPosition.x, newPosition.y);
}
```

### 2. Signal Flow Test
```cpp
// Add to AABB_Collider::SetTransformDirty()
printf("Transform dirty signal received for entity %d\n", mEntityId);

// Add to AABB_Collider constructor after signal registration
printf("Registered for transform dirty signals: %d\n", mTransformDirtySignalId);
```

### 3. Physics Integration Test
```cpp
// Add to AABB_Collider constructor
printf("Registered physics entity: %d, layer: %d, mask: %d\n", mEntityId, mLayer, mLayerMask);
```

## Next Steps

1. **Immediate Debugging**: Start with movement speed increase and debug logging
2. **Signal Verification**: Ensure Transform dirty signals are working
3. **Physics Layer Check**: Verify correct layer configuration
4. **Input Validation**: Confirm input is reaching player system
5. **Collision Response**: Test if collision detection is working but response is wrong

## Success Criteria

The system should be working when:
- Player responds to keyboard input with visible movement
- Player stops at walls/obstacles (collision detection working)
- Player slides along surfaces when moving into walls
- No performance issues from unnecessary physics updates

---

*Documentation created: 2025-09-27*
*Status: Collision system implemented, player movement not functional - requires debugging*