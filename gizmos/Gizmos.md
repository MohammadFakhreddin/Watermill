# Gizmos Module Documentation

## Overview

The Gizmos module provides a debug visualization system for drawing lines and points in 3D space. It's designed for debugging geometry, physics, AI paths, and other visual debugging needs in the Watermill engine.

## Features

- **Line Drawing** - Render lines between two 3D points with custom colors
- **Point Drawing** - Render points in 3D space with custom colors and sizes
- **Dual API** - Both static (convenience) and instance-based APIs
- **Render Queue** - Efficient batching of draw calls
- **ViewProjection Management** - Automatic camera matrix handling with BufferTracker

## Architecture

### Core Components

```
Gizmos (Main Class)
├── LineRenderer + LinePipeline
├── PointRenderer + PointPipeline
├── HostVisibleBufferTracker (ViewProjection)
└── Render Queue System
```

### Design Patterns

- **Singleton Pattern** - Static API with global instance access
- **Command Pattern** - Render tasks queued and executed later
- **RAII** - Automatic resource management for pipelines and buffers
- **Strategy Pattern** - Separate renderers for different primitive types

## API Reference

### Static API (Recommended for simple usage)

```cpp
// Draw a green line from origin to (1,1,1)
Gizmos::DrawLine(
    glm::vec3(0, 0, 0),
    glm::vec3(1, 1, 1),
    glm::vec4(0, 1, 0, 1)  // Green
);

// Draw a red point at (2,2,2) with size 15
Gizmos::DrawPoint(
    glm::vec3(2, 2, 2),
    glm::vec4(1, 0, 0, 1),  // Red
    15.0f                   // Size
);
```

### Instance API (For advanced usage)

```cpp
// Create gizmos instance
auto gizmos = std::make_unique<Gizmos>(displayRenderPass, initialViewProjection);

// Update camera matrices
gizmos->SetViewProjection(camera.GetViewProjectionMatrix());

// Queue draws (same as static API, but calls instance methods)
gizmos->DrawLine(...);
gizmos->DrawPoint(...);

// Execute all queued draws (call once per frame)
gizmos->Render(recordState);
```

## Class Reference

### Constructor

```cpp
explicit Gizmos(
    std::shared_ptr<DisplayRenderPass> const& displayRenderPass,
    glm::mat4 viewProjection = glm::identity<glm::mat4>()
);
```

**Parameters:**
- `displayRenderPass` - The render pass to use for drawing
- `viewProjection` - Initial view-projection matrix (defaults to identity)

**Behavior:**
- Sets global singleton instance
- Creates ViewProjection uniform buffer with proper frame count
- Initializes LinePipeline and PointPipeline
- Creates LineRenderer and PointRenderer with pipelines

### Destructor

```cpp
~Gizmos();
```

**Behavior:**
- Clears global singleton instance
- Automatic cleanup of all resources via RAII

### Methods

#### SetViewProjection

```cpp
void SetViewProjection(glm::mat4 const& viewProjection) const;
```

Updates the view-projection matrix for camera changes.

**Parameters:**
- `viewProjection` - New camera view-projection matrix

**Usage:**
```cpp
// Update when camera moves
gizmos->SetViewProjection(camera.GetViewProjectionMatrix());
```

#### Render

```cpp
void Render(RT::CommandRecordState& recordState);
```

Executes all queued draw calls and clears the queue.

**Parameters:**
- `recordState` - Vulkan command recording state

**Behavior:**
1. Updates ViewProjection buffer using BufferTracker
2. Executes all queued render tasks
3. Clears the render queue

**Usage:**
```cpp
// Call once per frame after queueing draws
gizmos->Render(recordState);
```

#### Static Draw Methods

```cpp
static void DrawLine(
    glm::vec3 const& from,
    glm::vec3 const& to,
    glm::vec4 const& color = {0.0f, 1.0f, 0.0f, 1.0f}  // Default: Green
);

static void DrawPoint(
    glm::vec3 const& position,
    glm::vec4 const& color = {1.0f, 0.0f, 0.0f, 1.0f},  // Default: Red
    float pointSize = 10.0f                              // Default: 10 pixels
);
```

**Safety:** Methods check if `Instance != nullptr` before calling.

## Implementation Details

### Render Queue System

Draw calls are not executed immediately but queued as lambda functions:

```cpp
void Private_DrawLine(glm::vec3 const& from, glm::vec3 const& to, glm::vec4 const& color)
{
    _renderTasks.emplace_back([this, from, to, color](RT::CommandRecordState& recordState) -> void
    {
        _lineRenderer->Draw(recordState, from, to, color);
    });
}
```

**Benefits:**
- Deferred execution allows proper command buffer state
- Efficient batching of similar draw calls
- Thread-safe queuing (if needed in future)

### ViewProjection Buffer Management

Uses `HostVisibleBufferTracker` for efficient GPU buffer updates:

```cpp
// In constructor
_viewProjectionTracker = std::make_unique<HostVisibleBufferTracker>(
    viewProjectionBuffer,
    Alias(viewProjection)
);

// In SetViewProjection
_viewProjectionTracker->SetData(Alias(viewProjection));

// In Render
_viewProjectionTracker->Update(recordState);
```

**Benefits:**
- Automatic multi-frame buffering (prevents tearing)
- Efficient host-visible memory mapping
- Proper synchronization with GPU

### Pipeline Architecture

Each primitive type has its own pipeline:

- **LinePipeline** - `VK_PRIMITIVE_TOPOLOGY_LINE_STRIP`
- **PointPipeline** - `VK_PRIMITIVE_TOPOLOGY_POINT_LIST`

**Shared Resources:**
- ViewProjection uniform buffer (shared between pipelines)
- DisplayRenderPass (shared render target)

**Per-Draw Data:**
- Model matrix (via push constants)
- Color (via push constants)
- Point size (via push constants for points)

## Usage Examples

### Basic Debug Lines

```cpp
// Draw coordinate axes
Gizmos::DrawLine(glm::vec3(0), glm::vec3(1,0,0), glm::vec4(1,0,0,1)); // X-axis (Red)
Gizmos::DrawLine(glm::vec3(0), glm::vec3(0,1,0), glm::vec4(0,1,0,1)); // Y-axis (Green)
Gizmos::DrawLine(glm::vec3(0), glm::vec3(0,0,1), glm::vec4(0,0,1,1)); // Z-axis (Blue)
```

### Physics Debug Visualization

```cpp
// Draw AABB bounds
void DrawAABB(const AABB& bounds, glm::vec4 color = glm::vec4(1,1,0,1)) {
    auto min = bounds.min;
    auto max = bounds.max;

    // Bottom face
    Gizmos::DrawLine(glm::vec3(min.x, min.y, min.z), glm::vec3(max.x, min.y, min.z), color);
    Gizmos::DrawLine(glm::vec3(max.x, min.y, min.z), glm::vec3(max.x, min.y, max.z), color);
    Gizmos::DrawLine(glm::vec3(max.x, min.y, max.z), glm::vec3(min.x, min.y, max.z), color);
    Gizmos::DrawLine(glm::vec3(min.x, min.y, max.z), glm::vec3(min.x, min.y, min.z), color);

    // Top face
    Gizmos::DrawLine(glm::vec3(min.x, max.y, min.z), glm::vec3(max.x, max.y, min.z), color);
    Gizmos::DrawLine(glm::vec3(max.x, max.y, min.z), glm::vec3(max.x, max.y, max.z), color);
    Gizmos::DrawLine(glm::vec3(max.x, max.y, max.z), glm::vec3(min.x, max.y, max.z), color);
    Gizmos::DrawLine(glm::vec3(min.x, max.y, max.z), glm::vec3(min.x, max.y, min.z), color);

    // Vertical edges
    Gizmos::DrawLine(glm::vec3(min.x, min.y, min.z), glm::vec3(min.x, max.y, min.z), color);
    Gizmos::DrawLine(glm::vec3(max.x, min.y, min.z), glm::vec3(max.x, max.y, min.z), color);
    Gizmos::DrawLine(glm::vec3(max.x, min.y, max.z), glm::vec3(max.x, max.y, max.z), color);
    Gizmos::DrawLine(glm::vec3(min.x, min.y, max.z), glm::vec3(min.x, max.y, max.z), color);
}
```

### AI Path Visualization

```cpp
// Draw waypoint path
void DrawPath(const std::vector<glm::vec3>& waypoints) {
    for (size_t i = 0; i < waypoints.size() - 1; ++i) {
        Gizmos::DrawLine(waypoints[i], waypoints[i + 1], glm::vec4(0, 1, 1, 1)); // Cyan
        Gizmos::DrawPoint(waypoints[i], glm::vec4(1, 1, 0, 1), 8.0f); // Yellow points
    }
    // Draw final waypoint
    if (!waypoints.empty()) {
        Gizmos::DrawPoint(waypoints.back(), glm::vec4(1, 0, 1, 1), 12.0f); // Magenta
    }
}
```

### Integration Example

```cpp
class GameRenderer {
    std::unique_ptr<Gizmos> _gizmos;

public:
    void Initialize() {
        _gizmos = std::make_unique<Gizmos>(_displayRenderPass);
    }

    void Update(const Camera& camera) {
        _gizmos->SetViewProjection(camera.GetViewProjectionMatrix());

        // Queue debug draws
        DrawDebugInfo();
    }

    void Render(RT::CommandRecordState& recordState) {
        // Render main scene...

        // Execute debug draws last
        _gizmos->Render(recordState);
    }

private:
    void DrawDebugInfo() {
        // Physics debug
        for (const auto& body : physicsWorld.GetBodies()) {
            DrawAABB(body.GetAABB());
        }

        // AI debug
        for (const auto& agent : aiSystem.GetAgents()) {
            DrawPath(agent.GetPath());
        }
    }
};
```

## Performance Considerations

### Efficient Usage
- **Batch draws** - Queue all draws then call Render() once
- **Limit draw count** - Use sparingly for debug only
- **Conditional rendering** - Use debug flags to disable in release

### Memory Usage
- Each draw call creates a lambda capture (~48-64 bytes)
- Queue is cleared each frame (no persistent memory growth)
- Pipelines and buffers allocated once

### GPU Performance
- Lines use `VK_PRIMITIVE_TOPOLOGY_LINE_STRIP` (efficient)
- Points use `VK_PRIMITIVE_TOPOLOGY_POINT_LIST` (efficient)
- Shared ViewProjection buffer (minimal state changes)
- Push constants for per-draw data (fast updates)

## Dependencies

### Internal Dependencies
- `LineRenderer` / `LinePipeline`
- `PointRenderer` / `PointPipeline`
- `HostVisibleBufferTracker`
- `DisplayRenderPass`
- `RenderBackend`
- `LogicalDevice`

### External Dependencies
- **GLM** - Mathematics (vec3, vec4, mat4)
- **Vulkan** - Graphics API
- **STL** - containers, functional

## Shader Requirements

The Gizmos module requires compiled shaders:
- `assets/shaders/line_pipeline/LinePipeline.vert.spv`
- `assets/shaders/line_pipeline/LinePipeline.frag.spv`
- `assets/shaders/point_pipeline/PointPipeline.vert.spv`
- `assets/shaders/point_pipeline/PointPipeline.frag.spv`

These shaders handle ViewProjection via uniform buffer and per-draw data via push constants.

## Troubleshooting

### Common Issues

**Gizmos not rendering:**
- Ensure `Gizmos` instance is created before static calls
- Verify `Render()` is called each frame
- Check ViewProjection matrix is valid

**Compilation errors:**
- Include all required headers
- Ensure `DisplayRenderPass` is available
- Check forward declarations are complete

**Performance issues:**
- Limit number of debug draws per frame
- Use conditional compilation for debug-only code
- Profile GPU usage if drawing many lines

---

**Version:** 1.0
**Last Updated:** 2025-09-21
**Compatibility:** Watermill Engine v1.0+