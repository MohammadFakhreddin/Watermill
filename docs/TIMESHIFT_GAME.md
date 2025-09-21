# TimeShift Game Documentation

## Overview

TimeShift is a 2D platformer game built on the Watermill engine that demonstrates advanced graphics programming techniques, HTML/CSS UI integration, and modern game architecture patterns. The game features time-based scoring, enemy AI, and a multi-level progression system.

## Game Features

- **2D Platformer Mechanics**: Classic side-scrolling gameplay
- **Time-Based Scoring**: Score based on completion time
- **Enemy AI**: Patrol-based enemy movement
- **Multi-Level System**: Three progressively challenging levels
- **HTML/CSS UI**: Modern web-based user interface
- **Debug Tools**: Built-in debug menu and shader reloading

## Architecture

### Application Structure

The TimeShift application follows a scene-based architecture:

```
TimeShiftApp
├── Scene Management System
├── Rendering Pipeline
├── Input System
├── Resource Manager
└── Debug Interface
```

### Main Components

#### TimeShiftMain (`executables/time_shift/TimeShiftMain.cpp`)

Entry point that initializes:
- Window (1920x1080, resizable)
- Vulkan logical device
- Job system for parallel tasks
- Resource manager
- Application instance

#### TimeShiftApp (`executables/time_shift/TimeShiftApp.hpp`)

Core application class managing:
- Scene lifecycle
- Render pass configuration
- Input routing
- Debug menu

**Scene Types:**
```cpp
enum class SceneId {
    Menu,
    Level1,
    Level2,
    Level3,
    Scoreboard
};
```

### Scene System

#### IScene Interface (`executables/time_shift/IScene.hpp`)

Base interface for all scenes:
```cpp
class IScene {
    virtual void OnUpdate(float deltaTime) = 0;
    virtual void OnRender(const CommandRecordState& state) = 0;
    virtual void OnResize(uint32_t width, uint32_t height) = 0;
    virtual void OnAxisEvent(const AxisEvent& event) = 0;
    virtual void OnButtonEvent(const ButtonEvent& event) = 0;
};
```

#### GameScene (`executables/time_shift/scene/GameScene.hpp`)

Main gameplay scene featuring:
- Level loading from JSON/binary format
- Physics integration
- Entity-component system
- Camera management
- Enemy AI

#### MenuScene (`executables/time_shift/scene/MenuScene.hpp`)

Main menu with:
- Start game option
- Exit game option
- Keyboard/gamepad navigation

#### ScoreboardScene (`executables/time_shift/scene/ScoreboardScene.hpp`)

End-game score display:
- Final time display
- Return to menu option
- Score persistence

## Rendering Pipeline

### Display Render Pass (`executables/time_shift/render/DisplayRenderPass.hpp`)

Main rendering coordinator managing multiple specialized renderers.

### Specialized Renderers

#### SpriteRenderer (`executables/time_shift/render/SpriteRenderer.hpp`)
2D sprite rendering with:
- Texture mapping
- Transform support
- Color tinting
- Batch optimization

#### SolidFillRenderer
Renders solid color primitives for backgrounds and UI elements.

#### BorderRenderer
Draws UI borders and frames.

#### ImageRenderer
Static image display for backgrounds and UI.

#### TextOverlayPipeline
Font rendering system for text display.

### Sprite Pipeline (`executables/time_shift/SpritePipeline.hpp`)

Vulkan pipeline for sprite rendering:

**Vertex Structure:**
```cpp
struct Vertex {
    glm::vec3 position;
    glm::vec2 uv;
};
```

**Push Constants:**
```cpp
struct PushConstants {
    glm::vec4 color;
    glm::mat4 model;
    glm::mat4 viewProjection;
};
```

## Level System

### Level Format

Levels use a hybrid JSON + binary format:

**JSON Structure:**
```json
{
    "name": "Level 1",
    "entities": [
        {
            "components": [
                {
                    "type": "SpriteRenderer",
                    "texture": "player.png",
                    "width": 32,
                    "height": 32
                },
                {
                    "type": "BoxCollider2D",
                    "width": 32,
                    "height": 32
                }
            ]
        }
    ]
}
```

**Binary Data:**
- Vertex buffers
- Index buffers
- Texture atlas data

### Level Parser (`executables/time_shift/level/LevelParser.hpp`)

Handles:
- JSON parsing
- Binary data loading
- Component instantiation
- Entity creation

## Enemy AI

### PatrolEnemy Component

Implements waypoint-based movement:
```cpp
class PatrolEnemy {
    std::vector<glm::vec2> waypoints;
    float speed;
    int currentWaypointIndex;

    void Update(float deltaTime) {
        // Move towards current waypoint
        // Switch to next waypoint when reached
        // Flip sprite based on direction
    }
};
```

## Input System

### Dual Input Support

Supports both keyboard and gamepad:

**Keyboard Controls:**
- Arrow Keys: Movement
- Space: Jump/Action
- Escape: Back/Pause
- F1: Debug menu
- F5: Reload shaders

**Gamepad Controls:**
- Left Stick/D-Pad: Movement
- Button A: Jump/Action
- Button B: Back/Pause

### Input Events

```cpp
struct AxisEvent {
    AxisId axis;
    float value;
};

struct ButtonEvent {
    ButtonId button;
    bool pressed;
};
```

## Resource Management

### ResourceManager (`executables/time_shift/ResourceManager.hpp`)

Features:
- Asynchronous texture loading
- Mipmap generation
- Resource caching
- Error texture fallback

**Usage:**
```cpp
auto texture = ResourceManager::LoadTexture("player.png");
auto shader = ResourceManager::LoadShader("sprite.vert", "sprite.frag");
```

## UI System

### HTML/CSS Integration

Uses WebView module for modern UI:

**HTML Structure:**
```html
<div class="hud">
    <div class="level-name">Level 1</div>
    <div class="timer">00:00</div>
    <div class="score">0</div>
</div>
```

**CSS Styling:**
```css
.hud {
    position: fixed;
    top: 10px;
    right: 10px;
    background: rgba(0, 0, 0, 0.5);
    color: white;
    padding: 10px;
}
```

## Debug Features

### Debug Menu

Accessible via F1, provides:
- Performance metrics
- Entity count
- Physics debug view
- Render statistics

### Shader Reloading

Press F5 to:
- Reload all shaders
- Recompile pipelines
- Apply changes without restart

## Performance Optimizations

### Rendering
- Sprite batching
- Texture atlasing
- Descriptor set pooling
- Command buffer reuse

### Memory
- Object pooling
- Resource caching
- Lazy loading
- Automatic cleanup

### Physics
- Spatial partitioning
- Broad-phase culling
- Sleep states
- Fixed timestep

## Build Configuration

### CMake Setup

```cmake
add_executable(TimeShift
    TimeShiftMain.cpp
    TimeShiftApp.cpp
    # ... other source files
)

target_link_libraries(TimeShift
    engine
    shared
    webview
    physics_2d
)
```

### Required Assets

Place in `assets/` directory:
- Textures (PNG format)
- Shaders (HLSL/GLSL)
- Levels (JSON + binary)
- Fonts (TTF format)

## Development Guide

### Adding New Levels

1. Create level JSON file
2. Add entity definitions
3. Configure components
4. Add to scene list
5. Update level progression

### Adding New Components

1. Create component class
2. Implement update/render
3. Register in level parser
4. Add to entity system

### Extending Enemy AI

1. Inherit from Enemy base
2. Implement behavior
3. Add to level format
4. Configure parameters

## Troubleshooting

### Common Issues

**Black Screen:**
- Check shader compilation
- Verify texture loading
- Validate render pass

**Physics Issues:**
- Check collider sizes
- Verify transform hierarchy
- Debug physics world

**Performance:**
- Profile with debug menu
- Check batch counts
- Monitor memory usage

## Future Enhancements

Potential improvements:
- Multiplayer support
- Level editor
- Particle effects
- Sound system
- Save/load system
- Achievements
- Leaderboards

This documentation provides a foundation for understanding and extending the TimeShift game. The modular architecture allows for easy enhancement while maintaining performance and code quality.