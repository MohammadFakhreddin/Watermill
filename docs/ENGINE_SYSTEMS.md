# Engine Systems Documentation

This document provides detailed information about each core system in the Watermill engine.

## Table of Contents
1. [Render System](#render-system)
2. [Asset System](#asset-system)
3. [Entity System](#entity-system)
4. [Job System](#job-system)
5. [Time System](#time-system)
6. [Bedrock Utilities](#bedrock-utilities)
7. [Importer System](#importer-system)

---

## Render System

### Overview
The render system provides a comprehensive Vulkan abstraction layer, managing GPU resources, command buffers, and rendering pipelines.

### Key Components

#### RenderBackend (`engine/render_system/RenderBackend.hpp`)
Low-level Vulkan wrapper providing:
- Device creation and management
- Swapchain handling
- Memory allocation
- Resource creation (buffers, images, pipelines)
- Synchronization primitives

**Key Functions:**
```cpp
namespace MFA::RenderBackend {
    void CreateDevice(...);
    void CreateSwapchain(...);
    void CreateBuffer(...);
    void CreateImage(...);
    void CreateGraphicsPipeline(...);
    void BeginCommandBuffer(...);
    void EndCommandBuffer(...);
}
```

#### LogicalDevice (`engine/render_system/LogicalDevice.hpp`)
Singleton managing the Vulkan logical device:
- Command pool management
- Queue family handling
- Render task submission
- Frame synchronization

**Usage:**
```cpp
auto& device = LogicalDevice::Instance();
device.AddRenderTask([](CommandRecordState& state) {
    // Record rendering commands
});
```

#### RenderTypes (`engine/render_system/RenderTypes.hpp`)
RAII wrappers for Vulkan objects:
- `BufferGroup`: Vertex/index/uniform buffers
- `ImageGroup`: Textures and render targets
- `PipelineGroup`: Shader pipelines
- `DescriptorSetGroup`: Resource bindings

#### IShadingPipeline (`engine/render_system/pipeline/IShadingPipeline.hpp`)
Interface for shader pipeline reloading:
```cpp
class IShadingPipeline {
    virtual void Reload() = 0;
};
```

### Pipeline System

Pipelines are specialized for different rendering tasks:
- Graphics pipelines for mesh rendering
- Compute pipelines for GPU computation
- Specialized 2D pipelines for sprites and UI

### Memory Management

The render system uses:
- Staging buffers for GPU uploads
- Memory pools for efficient allocation
- Automatic resource lifetime management

---

## Asset System

### Overview
Manages game assets including textures, models, shaders, and other resources.

### Asset Types

#### AssetShader (`engine/asset_system/AssetShader.hpp`)
Compiled SPIR-V shader representation:
```cpp
namespace AS {
    class Shader {
        Blob compiledShader;
        std::string entryPoint;
        Stage stage;
    };
}
```

#### AssetTexture (`engine/asset_system/AssetTexture.hpp`)
Comprehensive texture support:
- Uncompressed formats (RGBA, RGB, etc.)
- BC compression (BC1-BC7)
- ASTC compression
- Mipmap chains
- Cube maps

```cpp
namespace AS {
    class Texture {
        enum class Format { UNCOMPRESSED, BC, ASTC };
        Blob buffer;
        int width, height;
        int mipLevels;
        Format format;
    };
}
```

#### AssetGLTF_Model (`engine/asset_system/AssetGLTF_Model.hpp`)
3D model container:
```cpp
namespace AS::GLTF {
    struct Model {
        std::shared_ptr<Mesh> mesh;
        std::vector<std::shared_ptr<Texture>> textures;
    };
}
```

#### AssetGLTF_Mesh (`engine/asset_system/AssetGLTF_Mesh.hpp`)
Complex mesh structure supporting:
- Multiple vertex attributes
- Index buffers
- Skeletal animation
- Morph targets
- Scene hierarchy

### Asset Loading Pipeline

1. **File Reading**: Load from disk
2. **Parsing**: Format-specific parsing
3. **Validation**: Check data integrity
4. **Conversion**: Transform to engine format
5. **Caching**: Store for reuse

---

## Entity System

### Overview
Provides a transform hierarchy system for game objects.

### Transform Component (`engine/entity_system/Transform.hpp`)

Hierarchical 3D transformations with:
- Position (local and global)
- Rotation (quaternion-based)
- Scale (uniform and non-uniform)
- Parent-child relationships

**Key Features:**
```cpp
class Transform {
    // Local space
    void SetLocalPosition(glm::vec3);
    void SetLocalRotation(glm::quat);
    void SetLocalScale(glm::vec3);

    // Global space
    glm::vec3 GetGlobalPosition();
    glm::mat4 GetGlobalMatrix();

    // Hierarchy
    void SetParent(Transform* parent);
    void AddChild(Transform* child);

    // Optimization
    bool IsDirty() const;
    void UpdateMatrix();
};
```

### Dirty Flag System

Efficient matrix recalculation:
1. Mark dirty on change
2. Propagate to children
3. Lazy evaluation on access
4. Cache computed results

---

## Job System

### Overview
Provides parallel task execution with a thread pool architecture.

### Components

#### JobSystem (`engine/job_system/JobSystem.hpp`)
Static interface for task submission:
```cpp
namespace JobSystem {
    template<typename Func>
    auto AssignTask(Func&& function) -> std::future<...>;

    void Init(int threadCount = -1);
    void ShutDown();
}
```

#### ThreadPool (`engine/job_system/ThreadPool.hpp`)
Manages worker threads:
- Dynamic thread count
- Task distribution
- Load balancing
- Graceful shutdown

#### ThreadSafeQueue (`engine/job_system/ThreadSafeQueue.hpp`)
Lock-free concurrent queue:
- Fast enqueue/dequeue
- Multiple producers/consumers
- ABA problem prevention

### Usage Examples

```cpp
// Submit async task
auto future = JobSystem::AssignTask([]() {
    // Perform heavy computation
    return result;
});

// Wait for result
auto result = future.get();

// Fire and forget
JobSystem::AssignTask([]() {
    // Background work
});
```

---

## Time System

### Overview
Manages frame timing and update scheduling.

### Time Manager (`engine/time_system/Time.hpp`)

Singleton providing:
- Delta time calculation
- Frame rate limiting
- Update callbacks
- Performance metrics

**Key Features:**
```cpp
class Time {
    static Time& Instance();

    void Init(float minDeltaTime, float maxDeltaTime);
    void Update();

    float DeltaTime() const;
    float UnscaledDeltaTime() const;
    uint64_t FrameCount() const;

    void AddUpdateTask(std::function<void(float)>);
};
```

### Frame Rate Control

- Minimum frame time (maximum FPS)
- Maximum frame time (minimum FPS)
- Smooth delta time
- Frame skip prevention

---

## Bedrock Utilities

### Overview
Foundation utilities used throughout the engine.

### Memory Management (`engine/bedrock/BedrockMemory.hpp`)

#### Blob Classes
```cpp
// Non-owning memory view
class BaseBlob {
    uint8_t* ptr;
    size_t len;
};

// Owning memory container
class Blob : public BaseBlob {
    // RAII lifetime management
};

// Memory aliasing
template<typename T>
class Alias {
    T& AsType();
};
```

#### Memory Utilities
```cpp
namespace Memory {
    template<typename T>
    void Copy(T* dst, const T* src, size_t count);

    template<typename T>
    bool Compare(const T* a, const T* b, size_t count);

    void* AlignedAlloc(size_t size, size_t alignment);
}
```

### Signal System (`engine/bedrock/BedrockSignal.hpp`)

Type-safe event system:
```cpp
template<typename... Args>
class Signal {
    using Listener = std::function<void(Args...)>;

    SignalId Register(Listener listener);
    void Unregister(SignalId id);
    void Emit(Args... args);
};
```

### Logging (`engine/bedrock/BedrockLog.hpp`)

Debug logging with file/line info:
```cpp
#define MFA_LOG_INFO(format, ...)
#define MFA_LOG_WARN(format, ...)
#define MFA_LOG_ERROR(format, ...)
#define MFA_ASSERT(condition)
```

### String Utilities (`engine/bedrock/BedrockString.hpp`)

String manipulation helpers:
```cpp
namespace String {
    std::vector<std::string> Split(const std::string& str, char delimiter);
    std::string Join(const std::vector<std::string>& parts, const std::string& delimiter);
    std::string Trim(const std::string& str);
    bool StartsWith(const std::string& str, const std::string& prefix);
}
```

### Common Macros (`engine/bedrock/BedrockCommon.hpp`)

Utility macros for code generation:
```cpp
// Generate property with getter/setter
#define MFA_VARIABLE(type, name)

// Non-copyable class
#define MFA_NON_COPYABLE(ClassName)

// Non-movable class
#define MFA_NON_MOVABLE(ClassName)
```

---

## Importer System

### Overview
Loads various asset formats and converts them to engine format.

### Importers

#### GLTF Importer (`engine/importer/ImportGLTF.hpp`)
Loads GLTF 2.0 models:
```cpp
namespace Importer {
    std::shared_ptr<AS::GLTF::Model> GLTF_Model(const std::string& path);
    // Supports:
    // - Meshes with multiple primitives
    // - PBR materials
    // - Animations
    // - Scene hierarchy
}
```

#### Shader Importer (`engine/importer/ImportShader.hpp`)
Compiles shaders to SPIR-V:
```cpp
namespace Importer {
    std::shared_ptr<AS::Shader> Shader(
        const std::string& path,
        AS::ShaderStage stage,
        const std::string& entryPoint = "main"
    );
}
```

#### Texture Importer (`engine/importer/ImportTexture.hpp`)
Loads various image formats:
```cpp
namespace Importer {
    std::shared_ptr<AS::Texture> Texture(
        const std::string& path,
        AS::Texture::Format format = AS::Texture::Format::AUTO
    );
    // Supports:
    // - PNG, JPEG, TGA, BMP
    // - HDR formats
    // - Automatic mipmap generation
    // - Compression options
}
```

#### OBJ Importer (`engine/importer/ImportObj.hpp`)
Simple OBJ model loading:
```cpp
namespace Importer {
    std::shared_ptr<Model> OBJ_Model(const std::string& path);
    // Basic mesh data with:
    // - Vertices
    // - Normals
    // - Texture coordinates
    // - Material groups
}
```

### Import Pipeline

1. **Path Resolution**: Find file on disk
2. **Format Detection**: Identify file type
3. **Data Loading**: Read file contents
4. **Parsing**: Extract structured data
5. **Validation**: Verify data integrity
6. **Conversion**: Transform to engine format
7. **Optimization**: Compress, generate mipmaps, etc.
8. **Caching**: Store for future use

## System Integration

The engine systems work together through well-defined interfaces:

1. **Render System** uses **Asset System** for textures and models
2. **Entity System** transforms are used by **Render System** for positioning
3. **Job System** loads assets asynchronously via **Importer System**
4. **Time System** drives updates across all systems
5. **Bedrock** provides common utilities to all systems

This modular architecture ensures maintainability, performance, and extensibility for game development.