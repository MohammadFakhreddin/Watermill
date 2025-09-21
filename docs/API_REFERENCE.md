# API Reference

This document provides a comprehensive reference for the key classes, interfaces, and APIs in the Watermill engine.

## Table of Contents
1. [Render System APIs](#render-system-apis)
2. [Asset System APIs](#asset-system-apis)
3. [Entity System APIs](#entity-system-apis)
4. [Job System APIs](#job-system-apis)
5. [Time System APIs](#time-system-apis)
6. [Bedrock Utilities APIs](#bedrock-utilities-apis)
7. [WebView APIs](#webview-apis)
8. [Shared Components APIs](#shared-components-apis)

---

## Render System APIs

### RenderBackend Namespace

Core Vulkan abstraction functions:

```cpp
namespace MFA::RenderBackend {
    // Device Management
    bool CreateDevice(const DeviceCreateInfo& createInfo);
    void DestroyDevice();
    VkDevice GetVkDevice();
    VkPhysicalDevice GetVkPhysicalDevice();

    // Swapchain Operations
    bool CreateSwapchain(const SwapchainCreateInfo& createInfo);
    void DestroySwapchain();
    VkSwapchainKHR GetVkSwapchain();

    // Buffer Management
    bool CreateBuffer(
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkBuffer& buffer,
        VkDeviceMemory& bufferMemory
    );
    void DestroyBuffer(VkBuffer buffer, VkDeviceMemory memory);

    // Image Operations
    bool CreateImage(
        uint32_t width,
        uint32_t height,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkImage& image,
        VkDeviceMemory& imageMemory
    );
    void DestroyImage(VkImage image, VkDeviceMemory memory);

    // Pipeline Creation
    bool CreateGraphicsPipeline(
        const GraphicsPipelineCreateInfo& createInfo,
        VkPipeline& pipeline
    );
    void DestroyPipeline(VkPipeline pipeline);

    // Command Buffer Operations
    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
}
```

### LogicalDevice Class

```cpp
class LogicalDevice {
public:
    static LogicalDevice& Instance();

    // Initialization
    bool Init(const DeviceCreateInfo& createInfo);
    void Shutdown();

    // Render Task Management
    using RenderTask = std::function<void(CommandRecordState&)>;
    void AddRenderTask(RenderTask task);
    void ExecuteRenderTasks();

    // Queue Access
    VkQueue GetGraphicsQueue() const;
    VkQueue GetPresentQueue() const;
    uint32_t GetGraphicsQueueFamily() const;
    uint32_t GetPresentQueueFamily() const;

    // Command Pool Management
    VkCommandPool GetCommandPool() const;
    VkCommandBuffer AllocateCommandBuffer();
    void FreeCommandBuffer(VkCommandBuffer commandBuffer);

    // Synchronization
    VkSemaphore CreateSemaphore();
    VkFence CreateFence(bool signaled = false);
    void WaitForFence(VkFence fence);
    void ResetFence(VkFence fence);
};
```

### RenderTypes Namespace

RAII wrappers for Vulkan objects:

```cpp
namespace MFA::RT {
    // Buffer Wrapper
    class BufferGroup {
    public:
        explicit BufferGroup(
            VkDeviceSize size,
            VkBufferUsageFlags usage,
            VkMemoryPropertyFlags properties
        );
        ~BufferGroup();

        VkBuffer GetVkBuffer() const;
        VkDeviceMemory GetVkDeviceMemory() const;
        VkDeviceSize GetSize() const;

        void* Map();
        void Unmap();
        void CopyDataTo(const void* data, size_t size, size_t offset = 0);
    };

    // Image Wrapper
    class ColorImageGroup {
    public:
        explicit ColorImageGroup(
            uint32_t width,
            uint32_t height,
            VkFormat format,
            VkImageUsageFlags usage,
            VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT
        );
        ~ColorImageGroup();

        VkImage GetVkImage() const;
        VkImageView GetVkImageView() const;
        VkDeviceMemory GetVkDeviceMemory() const;
        VkExtent2D GetExtent() const;
        VkFormat GetFormat() const;
    };

    // Depth Image
    class DepthImageGroup {
    public:
        explicit DepthImageGroup(
            uint32_t width,
            uint32_t height,
            VkFormat depthFormat = VK_FORMAT_D32_SFLOAT,
            VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT
        );
        ~DepthImageGroup();

        VkImage GetVkImage() const;
        VkImageView GetVkImageView() const;
        VkDeviceMemory GetVkDeviceMemory() const;
    };

    // Pipeline Wrapper
    class PipelineGroup {
    public:
        explicit PipelineGroup(const GraphicsPipelineCreateInfo& createInfo);
        ~PipelineGroup();

        VkPipeline GetVkPipeline() const;
        VkPipelineLayout GetVkPipelineLayout() const;
    };

    // Descriptor Set Wrapper
    class DescriptorSetGroup {
    public:
        explicit DescriptorSetGroup(
            VkDescriptorSetLayout layout,
            VkDescriptorPool pool
        );
        ~DescriptorSetGroup();

        VkDescriptorSet GetVkDescriptorSet() const;
        void UpdateImageSampler(uint32_t binding, VkImageView imageView, VkSampler sampler);
        void UpdateBuffer(uint32_t binding, VkBuffer buffer, VkDeviceSize range);
    };

    // Command Record State
    struct CommandRecordState {
        VkCommandBuffer commandBuffer;
        VkRenderPass renderPass;
        VkFramebuffer framebuffer;
        uint32_t imageIndex;
        VkExtent2D renderExtent;
    };
}
```

---

## Asset System APIs

### Asset Classes

```cpp
namespace AS {
    // Shader Asset
    class Shader {
    public:
        enum class Stage {
            VERTEX,
            FRAGMENT,
            GEOMETRY,
            COMPUTE
        };

        explicit Shader(
            const Blob& spirvCode,
            Stage stage,
            const std::string& entryPoint = "main"
        );

        const Blob& GetSpirvCode() const;
        Stage GetStage() const;
        const std::string& GetEntryPoint() const;
        VkShaderStageFlagBits GetVkStage() const;
    };

    // Texture Asset
    class Texture {
    public:
        enum class Format {
            UNCOMPRESSED,
            BC,
            ASTC
        };

        struct UncompressedData {
            int components;
            bool hasAlpha;
        };

        explicit Texture(
            const Blob& buffer,
            int width,
            int height,
            Format format,
            int mipLevels = 1
        );

        const Blob& GetBuffer() const;
        int GetWidth() const;
        int GetHeight() const;
        Format GetFormat() const;
        int GetMipLevels() const;
        size_t GetPixelSize() const;
    };

    // GLTF Model
    namespace GLTF {
        struct Model {
            std::shared_ptr<Mesh> mesh;
            std::vector<std::shared_ptr<Texture>> textures;
        };

        class Mesh {
        public:
            struct Vertex {
                glm::vec3 position;
                glm::vec3 normal;
                glm::vec2 texCoord;
                glm::vec4 color;
            };

            explicit Mesh(
                const std::vector<Vertex>& vertices,
                const std::vector<uint32_t>& indices
            );

            const std::vector<Vertex>& GetVertices() const;
            const std::vector<uint32_t>& GetIndices() const;
            size_t GetVertexCount() const;
            size_t GetIndexCount() const;
        };
    }
}
```

---

## Entity System APIs

### Transform Class

```cpp
class Transform {
public:
    Transform();
    explicit Transform(Transform* parent);
    ~Transform();

    // Local Space Operations
    void SetLocalPosition(const glm::vec3& position);
    void SetLocalRotation(const glm::quat& rotation);
    void SetLocalScale(const glm::vec3& scale);

    const glm::vec3& GetLocalPosition() const;
    const glm::quat& GetLocalRotation() const;
    const glm::vec3& GetLocalScale() const;

    // Global Space Operations
    glm::vec3 GetGlobalPosition();
    glm::quat GetGlobalRotation();
    glm::vec3 GetGlobalScale();
    glm::mat4 GetGlobalMatrix();

    // Hierarchy Management
    void SetParent(Transform* parent);
    Transform* GetParent() const;
    void AddChild(Transform* child);
    void RemoveChild(Transform* child);
    const std::vector<Transform*>& GetChildren() const;

    // Utility Operations
    void LookAt(const glm::vec3& target, const glm::vec3& up = glm::vec3(0, 1, 0));
    void Translate(const glm::vec3& translation);
    void Rotate(const glm::quat& rotation);
    void Scale(const glm::vec3& scale);

    // Optimization
    bool IsDirty() const;
    void MarkDirty();
    void UpdateMatrix();

private:
    void NotifyChildrenDirty();
    void CalculateGlobalMatrix();
};
```

---

## Job System APIs

### JobSystem Namespace

```cpp
namespace JobSystem {
    // Initialization
    void Init(int threadCount = -1);  // -1 = hardware concurrency
    void Shutdown();

    // Task Assignment
    template<typename Func>
    auto AssignTask(Func&& function) -> std::future<decltype(function())>;

    template<typename Func, typename... Args>
    auto AssignTask(Func&& function, Args&&... args)
        -> std::future<decltype(function(args...))>;

    // Parallel Algorithms
    template<typename Iterator, typename Func>
    void ParallelFor(Iterator begin, Iterator end, Func function);

    template<typename Container, typename Func>
    void ParallelForEach(Container& container, Func function);

    // Thread Information
    int GetThreadCount();
    bool IsWorkerThread();
    int GetCurrentThreadId();
}

// Usage Examples:
auto future = JobSystem::AssignTask([]() {
    return HeavyComputation();
});

JobSystem::ParallelFor(vec.begin(), vec.end(), [](auto& item) {
    ProcessItem(item);
});
```

---

## Time System APIs

### Time Class

```cpp
class Time {
public:
    static Time& Instance();

    // Initialization
    void Init(float minDeltaTime = 1.0f/60.0f, float maxDeltaTime = 1.0f/10.0f);
    void Update();

    // Time Queries
    float DeltaTime() const;
    float UnscaledDeltaTime() const;
    float TimeScale() const;
    void SetTimeScale(float scale);

    uint64_t FrameCount() const;
    float TotalTime() const;
    float FPS() const;

    // Update Tasks
    using UpdateTask = std::function<void(float)>;
    void AddUpdateTask(UpdateTask task);
    void RemoveUpdateTask(UpdateTask task);

    // Frame Rate Control
    void SetTargetFPS(float fps);
    float GetTargetFPS() const;
    void SetVSync(bool enabled);
    bool IsVSyncEnabled() const;
};
```

---

## Bedrock Utilities APIs

### Memory Management

```cpp
namespace MFA {
    // Base Memory View
    class BaseBlob {
    public:
        BaseBlob() = default;
        BaseBlob(uint8_t* ptr, size_t len);

        uint8_t* Ptr() const;
        size_t Len() const;
        bool IsValid() const;
        void Clear();

        template<typename T>
        T* As() const;

        template<typename T>
        T& AsRef() const;
    };

    // Owning Memory Container
    class Blob : public BaseBlob {
    public:
        explicit Blob(size_t size);
        explicit Blob(const BaseBlob& source);
        ~Blob();

        Blob(const Blob&) = delete;
        Blob& operator=(const Blob&) = delete;
        Blob(Blob&& other) noexcept;
        Blob& operator=(Blob&& other) noexcept;

        void Resize(size_t newSize);
        void Reserve(size_t capacity);
    };

    // Memory Aliasing
    template<typename T>
    class Alias {
    public:
        explicit Alias(const BaseBlob& blob);
        explicit Alias(T* ptr);

        T& operator*() const;
        T* operator->() const;
        T* Ptr() const;
        operator T*() const;

        bool IsValid() const;
    };

    // Memory Utilities
    namespace Memory {
        template<typename T>
        void Copy(T* dst, const T* src, size_t count);

        template<typename T>
        bool Compare(const T* a, const T* b, size_t count);

        void* AlignedAlloc(size_t size, size_t alignment);
        void AlignedFree(void* ptr);

        template<typename T>
        size_t AlignedSize(size_t count, size_t alignment = alignof(T));
    }
}
```

### Signal System

```cpp
namespace MFA {
    template<typename... Args>
    class Signal {
    public:
        using Listener = std::function<void(Args...)>;
        using SignalId = uint32_t;

        // Registration
        SignalId Register(Listener listener);
        void Unregister(SignalId id);
        void Clear();

        // Emission
        void Emit(Args... args);
        void operator()(Args... args);

        // Query
        size_t ListenerCount() const;
        bool HasListeners() const;

    private:
        std::unordered_map<SignalId, Listener> _listeners;
        SignalId _nextId = 1;
    };

    // Automatic signal management
    class SignalConnection {
    public:
        template<typename... Args>
        SignalConnection(Signal<Args...>& signal, typename Signal<Args...>::Listener listener);
        ~SignalConnection();

        void Disconnect();

    private:
        std::function<void()> _disconnector;
    };
}
```

### Logging

```cpp
// Logging Macros
#define MFA_LOG_DEBUG(format, ...)    // Debug level logging
#define MFA_LOG_INFO(format, ...)     // Info level logging
#define MFA_LOG_WARN(format, ...)     // Warning level logging
#define MFA_LOG_ERROR(format, ...)    // Error level logging

#define MFA_ASSERT(condition)         // Debug assertion
#define MFA_VERIFY(condition)         // Release assertion

// Usage:
MFA_LOG_INFO("Player health: %d", playerHealth);
MFA_ASSERT(buffer.IsValid());
```

---

## WebView APIs

### WebViewContainer Class

```cpp
class WebViewContainer : public litehtml::document_container {
public:
    WebViewContainer();
    ~WebViewContainer();

    // Document Management
    void LoadHTML(const std::string& html, const std::string& css = "");
    void LoadHTMLFromFile(const std::string& htmlPath, const std::string& cssPath = "");
    void Update(float deltaTime);
    void Render(const CommandRecordState& state);

    // Viewport Configuration
    void SetViewport(uint32_t width, uint32_t height);
    void Resize(uint32_t width, uint32_t height);

    // DOM Manipulation
    litehtml::element::ptr GetElementById(const std::string& id);
    void SetText(const std::string& id, const std::string& text);
    void SetAttribute(const std::string& id, const std::string& attr, const std::string& value);
    void SetStyle(const std::string& id, const std::string& property, const std::string& value);

    // CSS Class Management
    void AddClass(const std::string& id, const std::string& className);
    void RemoveClass(const std::string& id, const std::string& className);
    void ToggleClass(const std::string& id, const std::string& className);
    bool HasClass(const std::string& id, const std::string& className);

    // Event Handling
    void OnMouseMove(int x, int y);
    void OnMouseDown(int x, int y, MouseButton button);
    void OnMouseUp(int x, int y, MouseButton button);
    bool OnMouseClick(int x, int y, MouseButton button);

    // State Queries
    bool IsHovered(const std::string& id) const;
    bool IsPressed(const std::string& id) const;
    bool IsClicked(const std::string& id) const;
    bool NeedsRedraw() const;

    // Performance
    void EnableBatching(bool enable);
    void SetUpdateThrottle(float minUpdateInterval);

protected:
    // Litehtml callbacks (override document_container)
    void draw_solid_fill(const litehtml::position& pos,
                        const litehtml::web_color& color,
                        const litehtml::border_radiuses& radius) override;

    void draw_borders(const litehtml::borders& borders,
                     const litehtml::position& draw_pos,
                     bool root) override;

    void draw_image(const char* src, const char* baseurl,
                   const litehtml::position& pos) override;

    void draw_text(const char* text, litehtml::font_metrics* fm,
                  const litehtml::position& pos,
                  const litehtml::web_color& color) override;
};
```

---

## Shared Components APIs

### SceneRenderPass Class

```cpp
class SceneRenderPass {
public:
    explicit SceneRenderPass(
        VkExtent2D imageExtent,
        VkFormat imageFormat,
        VkFormat depthFormat,
        VkSampleCountFlagBits msaaSampleCount
    );
    ~SceneRenderPass();

    // Render Pass Operations
    void Begin(const CommandRecordState& state,
               const glm::vec4& clearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    void End(const CommandRecordState& state);

    // Access
    VkRenderPass GetRenderPass() const;
    VkFramebuffer GetFramebuffer(int imageIndex) const;
    VkExtent2D GetExtent() const;

    // Configuration
    void SetClearColor(const glm::vec4& color);
    void SetClearDepth(float depth);
};
```

### ShapeGenerator Functions

```cpp
namespace ShapeGenerator {
    // Type Definitions
    using Vertices = std::vector<glm::vec3>;
    using Indices = std::vector<uint32_t>;
    using Normals = std::vector<glm::vec3>;
    using TexCoords = std::vector<glm::vec2>;
    using Mesh = std::tuple<Vertices, Indices, Normals>;
    using DetailedMesh = std::tuple<Vertices, Indices, Normals, TexCoords>;

    // Basic Shapes
    Mesh Sphere(float radius, int slices, int stacks);
    Mesh Cylinder(float radius, float height, int segments);
    Mesh Quad();
    Mesh Cube(float size = 1.0f);

    // Advanced Shapes
    DetailedMesh UVSphere(float radius, int slices, int stacks);
    DetailedMesh Plane(float width, float height, int subdivisions = 1);
    DetailedMesh Torus(float majorRadius, float minorRadius, int segments, int rings);

    // Utility Functions
    void CalculateNormals(Vertices& vertices, const Indices& indices, Normals& normals);
    void CalculateTangents(const Vertices& vertices, const Indices& indices,
                          const TexCoords& texCoords, std::vector<glm::vec3>& tangents);
    void SmoothNormals(Vertices& vertices, Normals& normals, float threshold = 0.5f);
}
```

---

## Common Usage Patterns

### Resource Initialization

```cpp
// Initialize engine systems
LogicalDevice::Instance().Init(deviceCreateInfo);
JobSystem::Init();
Time::Instance().Init();

// Create render resources
auto colorImage = std::make_unique<RT::ColorImageGroup>(width, height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
auto depthImage = std::make_unique<RT::DepthImageGroup>(width, height);
```

### Asset Loading

```cpp
// Load assets asynchronously
auto textureTask = JobSystem::AssignTask([=]() {
    return Importer::Texture("player.png");
});

auto shaderTask = JobSystem::AssignTask([=]() {
    return Importer::Shader("sprite.vert", AS::Shader::Stage::VERTEX);
});

// Wait for completion
auto texture = textureTask.get();
auto shader = shaderTask.get();
```

### Scene Rendering

```cpp
// Setup scene
SceneRenderPass renderPass(extent, colorFormat, depthFormat, samples);
SceneRenderResource resources(extent, colorFormat, depthFormat, samples);

// Render loop
LogicalDevice::Instance().AddRenderTask([&](CommandRecordState& state) {
    renderPass.Begin(state);

    // Render scene objects
    for (auto& object : sceneObjects) {
        object.Render(state);
    }

    renderPass.End(state);
});
```

This API reference provides the essential interfaces for working with the Watermill engine. Each system is designed to work together while maintaining clear separation of concerns and following modern C++ best practices.