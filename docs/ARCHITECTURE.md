# Watermill Engine Architecture

## Overview

Watermill is a modern C++ game engine built with a modular, performance-focused architecture. The engine leverages Vulkan for graphics rendering, provides HTML/CSS UI capabilities, and implements comprehensive systems for asset management, job scheduling, and scene rendering.

## Architecture Principles

### 1. **Modular Design**
Each system is self-contained with clear interfaces and minimal dependencies. This allows for:
- Independent testing and development
- Easy system replacement or extension
- Clear ownership boundaries

### 2. **RAII Resource Management**
All resources follow RAII (Resource Acquisition Is Initialization) patterns:
- Automatic cleanup via destructors
- Exception-safe resource handling
- No manual memory management required

### 3. **Data-Oriented Design**
Performance-critical systems use cache-friendly data layouts:
- Structure of Arrays (SoA) where beneficial
- Contiguous memory allocation
- Minimal pointer chasing

### 4. **Event-Driven Communication**
Systems communicate through signals/events rather than direct coupling:
- Loose coupling between systems
- Runtime configurability
- Easy debugging and monitoring

## System Layers

```
┌─────────────────────────────────────────┐
│         Application Layer               │
│  (TimeShift Game, Other Executables)    │
├─────────────────────────────────────────┤
│         Integration Layer               │
│    (WebView, Shared Components)         │
├─────────────────────────────────────────┤
│         High-Level Systems              │
│  (Scene Management, Entity System)      │
├─────────────────────────────────────────┤
│         Core Engine Systems             │
│ (Render, Asset, Job, Time, Physics)     │
├─────────────────────────────────────────┤
│         Foundation Layer                │
│      (Bedrock Utilities)                │
├─────────────────────────────────────────┤
│         Platform Layer                  │
│    (Vulkan, SDL2, OS APIs)              │
└─────────────────────────────────────────┘
```

## Core Systems

### Render System (`engine/render_system/`)
**Responsibility:** Vulkan abstraction and GPU resource management

**Key Components:**
- `RenderBackend`: Low-level Vulkan API wrapper
- `LogicalDevice`: Device lifecycle and command management
- `RenderTypes`: RAII wrappers for Vulkan objects
- `IShadingPipeline`: Shader pipeline interface

**Design Patterns:**
- Facade pattern for Vulkan complexity
- Command pattern for GPU operations
- Factory pattern for resource creation

### Asset System (`engine/asset_system/`)
**Responsibility:** Loading and managing game assets

**Key Components:**
- `AssetShader`: Compiled shader management
- `AssetTexture`: Texture loading with compression support
- `AssetGLTF_Model/Mesh`: 3D model representation

**Design Patterns:**
- Immutable asset pattern
- Factory pattern for asset creation
- Format abstraction

### Job System (`engine/job_system/`)
**Responsibility:** Parallel task execution

**Key Components:**
- `JobSystem`: Task submission interface
- `ThreadPool`: Worker thread management
- `ThreadSafeQueue`: Lock-free queuing

**Design Patterns:**
- Thread pool pattern
- Future/Promise for async results
- Producer-consumer queuing

### Entity System (`engine/entity_system/`)
**Responsibility:** Game object management

**Key Components:**
- `Transform`: Hierarchical positioning
- Component-based architecture (extensible)

**Design Patterns:**
- Component pattern
- Dirty flag optimization
- Scene graph hierarchy

### Time System (`engine/time_system/`)
**Responsibility:** Frame timing and updates

**Key Components:**
- `Time`: Delta time calculation
- Frame rate limiting
- Update task scheduling

**Design Patterns:**
- Singleton for global access
- Observer for update callbacks

## Integration Systems

### WebView Module (`webview/`)
Bridges HTML/CSS content with Vulkan rendering:
- Custom `litehtml::document_container` implementation
- Specialized Vulkan pipelines for web content
- DOM manipulation API
- Double-buffered state management

### Shared Components (`shared/`)
Common rendering utilities used across the engine:
- `SceneRenderPass`: Scene rendering coordination
- `SceneRenderResource`: Resource management for scenes
- `ShapeGenerator`: Procedural geometry creation
- `GridPipeline/Renderer`: Debug visualization

## Memory Management

### Bedrock Memory System
Custom memory utilities providing:
- `Blob`: RAII memory container
- `BaseBlob`: Non-owning memory view
- `Alias`: Memory aliasing utilities
- Template-based copy/compare operations

### Resource Lifecycle
1. **Creation:** Resources allocated through factory functions
2. **Ownership:** Clear single ownership or shared_ptr for shared resources
3. **Access:** Const references for read-only access
4. **Destruction:** Automatic cleanup via RAII

## Threading Model

### Main Thread
- Window management
- Input processing
- Render command recording
- Game logic updates

### Worker Threads (Job System)
- Asset loading
- Physics calculations
- AI processing
- Any parallelizable tasks

### Synchronization
- Lock-free queues for job submission
- Futures for async task results
- Minimal mutex usage (only where necessary)

## Rendering Pipeline

### Frame Flow
1. **Update Phase**
   - Process input
   - Update game logic
   - Update transforms
   - Submit render tasks

2. **Record Phase**
   - Record Vulkan commands
   - Update descriptor sets
   - Prepare push constants

3. **Submit Phase**
   - Submit command buffers
   - Queue presentation
   - Synchronize with GPU

### Pipeline Architecture
- Multiple specialized pipelines (sprite, solid fill, border, text)
- Shared descriptor set layouts where possible
- Push constants for per-draw data
- Instancing for batch rendering

## Event System

### Signal/Slot Implementation
Using Bedrock's `Signal<>` template:
- Type-safe event handling
- Automatic disconnection on destruction
- Multiple listeners per event
- No runtime overhead when unused

### Common Events
- Window resize
- Scene transitions
- Resource load completion
- Input events

## Performance Considerations

### Optimization Strategies
1. **Batch Rendering:** Minimize draw calls through instancing
2. **Resource Pooling:** Reuse descriptor sets and buffers
3. **Cache Optimization:** Data-oriented layouts for hot paths
4. **Async Loading:** Background asset loading with job system
5. **Dirty Flagging:** Only update what changes

### Profiling Support
- `ScopeProfiler` for timing measurements
- GPU timing queries (when enabled)
- Memory allocation tracking
- Frame time monitoring

## Extension Points

### Adding New Systems
1. Create system in appropriate engine folder
2. Define clear interface/API
3. Integrate with job system if parallelizable
4. Add to CMakeLists.txt
5. Document in relevant docs

### Adding New Pipelines
1. Implement `IShadingPipeline` interface
2. Create vertex/fragment shaders
3. Define push constant structure
4. Integrate with render pass
5. Add to resource management

## Best Practices

### Code Organization
- One class per file (header + implementation)
- Namespace organization matching folder structure
- Clear separation of interface and implementation

### Resource Management
- Always use RAII wrappers
- Prefer stack allocation
- Use smart pointers for shared ownership
- Validate Vulkan operations in debug

### Error Handling
- Use asserts for programmer errors
- Return error codes for runtime failures
- Log errors with context information
- Graceful degradation where possible

## Dependencies

### External Libraries
- **Vulkan**: Graphics API
- **SDL2**: Platform abstraction
- **GLM**: Mathematics
- **ImGui**: Debug UI
- **LiteHTML**: HTML parsing
- **JSON**: Data serialization

### Internal Dependencies
```
Bedrock → All Systems
Time/Job → Other Systems
Asset → Render, Importers
Entity → Render, Game Logic
Render → Top Level
Importers → Asset System
```

This architecture provides a solid foundation for game development while maintaining flexibility for future enhancements and optimizations.