# Shared Utilities Documentation

## Overview

The `shared/` directory contains common rendering utilities and components that are used across different parts of the Watermill engine. These utilities provide foundational rendering capabilities, scene management, and geometric primitives.

## Components

### Scene Rendering System

#### SceneRenderPass (`shared/SceneRenderPass.hpp`)

Main coordinator for scene rendering operations.

**Key Features:**
- Multi-pass rendering support
- Resource lifetime management
- Viewport configuration
- Clear operations

**Usage:**
```cpp
class SceneRenderPass {
public:
    explicit SceneRenderPass(
        VkExtent2D imageExtent,
        VkFormat imageFormat,
        VkFormat depthFormat,
        VkSampleCountFlagBits msaaSampleCount
    );

    void Begin(const CommandRecordState& state);
    void End(const CommandRecordState& state);

    VkRenderPass GetRenderPass() const;
    VkFramebuffer GetFramebuffer(int imageIndex) const;
};
```

#### SceneRenderResource (`shared/SceneRenderResource.hpp`)

Manages render targets and resources for scene rendering.

**Resource Management:**
- Color attachments (MSAA and resolved)
- Depth attachments
- Multi-frame support for swapchain
- Resource pooling

**Interface:**
```cpp
class SceneRenderResource {
public:
    // Access to render targets
    const ColorImageGroup& MSAA_Image(const CommandRecordState& state) const;
    const ColorImageGroup& ColorImage(const CommandRecordState& state) const;
    const DepthImageGroup& DepthImage(const CommandRecordState& state) const;

    // Configuration
    VkExtent2D ImageExtent() const;
    VkFormat ImageFormat() const;
    VkSampleCountFlagBits MSAA_SampleCount() const;
};
```

#### SceneFrameBuffer (`shared/SceneFrameBuffer.hpp`)

Framebuffer management for scene rendering.

**Features:**
- Automatic framebuffer creation
- Multi-sampling support
- Depth buffer handling
- Swapchain integration

### Grid Rendering System

#### GridPipeline (`shared/GridPipeline.hpp`)

Specialized pipeline for rendering debug grids and wireframes.

**Applications:**
- Level editor grid overlay
- Debug visualization
- Coordinate system display
- Measurement tools

**Pipeline Configuration:**
```cpp
class GridPipeline {
public:
    void Init();
    void Render(
        const CommandRecordState& state,
        const Camera& camera,
        float gridSize = 1.0f,
        const glm::vec4& color = glm::vec4(1.0f)
    );
};
```

#### GridRenderer (`shared/GridRenderer.hpp`)

High-level interface for grid rendering operations.

**Features:**
- Infinite grid rendering
- Multiple grid scales
- Dynamic color adjustment
- Camera-relative positioning

### Shape Generation

#### ShapeGenerator (`shared/ShapeGenerator.hpp`)

Procedural geometry generation for common 3D shapes.

**Supported Shapes:**
```cpp
namespace ShapeGenerator {
    using Vertices = std::vector<glm::vec3>;
    using Indices = std::vector<uint32_t>;
    using Normals = std::vector<glm::vec3>;
    using Mesh = std::tuple<Vertices, Indices, Normals>;

    // Generate cylinder mesh
    Mesh Cylinder(float radius, float height, int segments);

    // Generate sphere mesh
    Mesh Sphere(float radius, int slices, int stacks);

    // Generate quad mesh
    Mesh Quad();
}
```

**Usage Example:**
```cpp
// Generate a sphere
auto [vertices, indices, normals] = ShapeGenerator::Sphere(1.0f, 32, 16);

// Upload to GPU
auto vertexBuffer = CreateVertexBuffer(vertices);
auto indexBuffer = CreateIndexBuffer(indices);
```

### Common Buffer Structures

#### Buffers (`shared/Buffers.hpp`)

Standard buffer layouts for common rendering data.

**Camera Buffer:**
```cpp
namespace Buffers {
    struct Camera {
        glm::mat4 viewProjection;
        glm::vec3 position;
        float placeholder;  // Padding for alignment
    };
}
```

**Lighting Buffer:**
```cpp
namespace Buffers {
    struct DirectionalLight {
        glm::vec3 direction;
        float ambientStrength;
        glm::vec3 color;
        float placeholder0;  // Padding
    };
}
```

## Implementation Details

### Scene Rendering Flow

1. **Initialization**
   ```cpp
   SceneRenderResource resources(extent, colorFormat, depthFormat, samples);
   SceneRenderPass renderPass(extent, colorFormat, depthFormat, samples);
   ```

2. **Per-Frame Rendering**
   ```cpp
   renderPass.Begin(recordState);

   // Render opaque objects
   RenderOpaqueObjects(recordState);

   // Render transparent objects
   RenderTransparentObjects(recordState);

   renderPass.End(recordState);
   ```

### Grid Rendering Implementation

The grid system uses an infinite grid technique that:
1. Projects grid lines into screen space
2. Fades grid density based on camera distance
3. Draws only visible grid sections
4. Supports multiple subdivision levels

**Shader Implementation:**
```hlsl
float2 grid = abs(frac(worldPos.xz - 0.5) - 0.5) / fwidth(worldPos.xz);
float line = min(grid.x, grid.y);
float4 color = float4(gridColor.rgb, 1.0 - min(line, 1.0));
```

### Shape Generation Algorithms

#### Sphere Generation
```cpp
Mesh ShapeGenerator::Sphere(float radius, int slices, int stacks) {
    Vertices vertices;
    Indices indices;
    Normals normals;

    // Generate vertices using spherical coordinates
    for (int stack = 0; stack <= stacks; ++stack) {
        float phi = M_PI * stack / stacks;
        for (int slice = 0; slice <= slices; ++slice) {
            float theta = 2.0f * M_PI * slice / slices;

            glm::vec3 pos = {
                radius * sin(phi) * cos(theta),
                radius * cos(phi),
                radius * sin(phi) * sin(theta)
            };

            vertices.push_back(pos);
            normals.push_back(normalize(pos));
        }
    }

    // Generate indices for triangles
    // ... triangle generation logic

    return std::make_tuple(vertices, indices, normals);
}
```

#### Cylinder Generation
Creates a cylinder with configurable segments:
- Top and bottom caps
- Side faces with proper UV mapping
- Smooth normals for curved surfaces

### Memory Layout Considerations

#### Buffer Alignment
All buffer structures follow GPU alignment requirements:
- `vec3` padded to 16 bytes
- Struct members aligned to their natural boundaries
- Arrays aligned to 16-byte boundaries

#### Vertex Attributes
Standard vertex layouts used across pipelines:
```cpp
struct StandardVertex {
    glm::vec3 position;    // Location 0
    glm::vec3 normal;      // Location 1
    glm::vec2 texCoord;    // Location 2
    glm::vec4 color;       // Location 3
};
```

## Usage Patterns

### Scene Setup
```cpp
// Initialize scene rendering
auto resources = std::make_unique<SceneRenderResource>(
    extent, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_4_BIT
);

auto renderPass = std::make_unique<SceneRenderPass>(
    extent, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_4_BIT
);

// Create framebuffer
auto frameBuffer = std::make_unique<SceneFrameBuffer>(
    renderPass->GetRenderPass(),
    resources.get()
);
```

### Grid Overlay
```cpp
// Add debug grid to level editor
GridRenderer gridRenderer;
gridRenderer.Init();

// In render loop
gridRenderer.Render(recordState, camera, 1.0f, glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
```

### Procedural Meshes
```cpp
// Generate test geometry
auto sphereMesh = ShapeGenerator::Sphere(2.0f, 32, 16);
auto cylinderMesh = ShapeGenerator::Cylinder(1.0f, 3.0f, 24);

// Convert to GPU resources
auto sphereBuffer = CreateMeshBuffer(sphereMesh);
auto cylinderBuffer = CreateMeshBuffer(cylinderMesh);
```

## Performance Considerations

### Render Pass Optimization
- Minimize render pass state changes
- Batch similar draw calls
- Use subpasses for dependent rendering
- Optimize clear operations

### Memory Management
- Pool framebuffers across frames
- Reuse descriptor sets
- Batch buffer updates
- Minimize memory allocations

### Grid Rendering Efficiency
- Cull grid lines outside view frustum
- Use logarithmic density falloff
- Cache grid mesh data
- Optimize shader for mobile GPUs

## Integration Examples

### Level Editor Integration
```cpp
class LevelEditor {
    std::unique_ptr<SceneRenderPass> _scenePass;
    std::unique_ptr<GridRenderer> _gridRenderer;

    void RenderScene() {
        _scenePass->Begin(recordState);

        // Render level geometry
        RenderLevelMeshes();

        // Overlay debug grid
        _gridRenderer->Render(recordState, _camera);

        _scenePass->End(recordState);
    }
};
```

### Primitive Rendering
```cpp
class PrimitiveRenderer {
    void RenderDebugSphere(const glm::vec3& position, float radius) {
        static auto sphereMesh = ShapeGenerator::Sphere(1.0f, 16, 8);

        // Scale and position
        auto transform = glm::translate(glm::mat4(1.0f), position) *
                        glm::scale(glm::mat4(1.0f), glm::vec3(radius));

        RenderMesh(sphereMesh, transform);
    }
};
```

## Extension Points

### Custom Shapes
Add new shapes to ShapeGenerator:
```cpp
// In ShapeGenerator namespace
Mesh Torus(float majorRadius, float minorRadius, int segments, int rings);
Mesh Plane(float width, float height, int subdivisions);
```

### Advanced Grid Features
Extend GridRenderer with:
- Multi-level subdivision
- Axis highlighting
- Custom line styles
- Measurement tools

### Scene Effects
Add post-processing to SceneRenderPass:
- FXAA anti-aliasing
- Tone mapping
- Bloom effects
- Screen-space reflections

The shared utilities provide a solid foundation for rendering operations while maintaining flexibility for project-specific customizations. These components follow the engine's RAII and performance-oriented design principles.