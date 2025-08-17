# Watermill Project - Claude Development Notes

## Project Overview
Watermill is a game engine project using Vulkan for rendering, with a custom entity-component system and 2D physics.

## Project Structure

### Core Systems
- **Engine**: Located in `/engine/` with subsystems for rendering, assets, entities, time, and jobs
- **Physics 2D**: Custom 2D physics system in `/physics_2d/`
- **Executables**: Game implementations in `/executables/` (currently `time_shift`)

### Build System
- CMake-based build system
- Submodules: SDL2, Vulkan-Headers, Vulkan-Loader, GLM, Litehtml
- Physics2D library is included via `add_subdirectory` in main CMakeLists.txt

## Physics System Integration

### Physics2D System
- Located in `/physics_2d/`
- Supports AABB, Sphere, and Box collision shapes
- Layer-based collision filtering
- Raycast support
- Collision callbacks via std::function

### Component System
The game uses a JSON-based level format with components:
- **Transform**: Position, rotation, scale hierarchy
- **SpriteRenderer**: Visual representation
- **BoxCollider2D**: Collision bounds with offset and size
- **PatrolEnemy**: AI movement component
- **Camera**: Orthographic projection settings

### Physics Integration Pattern
1. BoxCollider2D components are parsed from level JSON
2. Each collider is registered with Physics2D system as an AABB
3. Every frame:
   - Transform positions are synced to physics bodies
   - Physics2D::Update() processes collisions
   - Collision callbacks are triggered

### Key Implementation Details

#### GameScene Physics Setup
```cpp
// Initialize Physics2D in constructor
_physics2D = std::make_unique<Physics2D>();

// Register colliders during level load
for (auto const & collider : jsonColliders) {
    auto physicsId = _physics2D->Register(
        Physics2D::Type::AABB,
        layer,
        layerMask,
        callback
    );
    _physicsEntities.push_back({physicsId, collider});
}

// Update physics positions each frame
for (auto & physicsEntity : _physicsEntities) {
    auto globalTransform = collider->transform->GlobalTransform();
    glm::vec2 position = glm::vec2(globalTransform[3].x, globalTransform[3].z);
    _physics2D->MoveAABB(physicsId, min, max, !collider->isTrigger);
}
```

#### Coordinate System
- 3D world uses Y-up coordinate system
- 2D physics maps X,Z world coordinates to 2D X,Y plane
- Transform positions are extracted as (x, z) for 2D physics

## Development Notes

### Removed Dependencies
- LineRenderer and PointRenderer were removed from Physics2D
- Debug rendering will be implemented later via a gizmos system

### Build Commands
Project uses CMake for building. Physics2D library is automatically included.

### Testing Notes
- Collision detection uses isTrigger flag to determine if objects should block movement
- Layer masks allow selective collision between object groups
- AABB collision is the primary collision type for BoxCollider2D components

## Common Patterns

### Component Registration Flow
1. LevelParser reads JSON and creates component instances
2. Components store Transform pointers for position updates
3. GameScene registers physics bodies and maintains ID mapping
4. Update loop syncs transform changes to physics system

### Memory Management
- Components use std::shared_ptr for automatic cleanup
- Physics2D maintains internal entity map with IDs
- Transform hierarchy uses raw pointers (managed by parent shared_ptrs)

## Future Improvements
- [ ] Implement gizmos system for physics debug visualization
- [ ] Add more collision shapes (circles, polygons)
- [ ] Implement physics-based movement for entities
- [ ] Add trigger enter/exit events
- [ ] Optimize spatial partitioning for large numbers of colliders