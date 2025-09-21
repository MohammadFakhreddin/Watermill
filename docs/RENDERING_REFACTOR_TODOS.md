# Rendering System Refactor TODOs

This document outlines critical architectural issues in the current rendering systems that need to be addressed for better performance, maintainability, and correctness.

## 🚨 Critical Issues

### 1. **SpriteRenderer Architecture Problems**

#### **Issue: No Proper Instanced Rendering**
**Current State:**
- Each sprite draw call creates individual draw commands
- No batching of similar sprites
- Excessive GPU state changes
- Poor performance with many sprites

**Problems:**
```cpp
// Current inefficient approach (each sprite = separate draw call)
for (auto& sprite : sprites) {
    spriteRenderer.Draw(sprite.position, sprite.texture, sprite.color);
    // Each call results in separate vkCmdDraw()
}
```

**Impact:**
- **Performance**: Hundreds of draw calls instead of a few batched calls
- **GPU Utilization**: Poor parallelism due to excessive CPU→GPU communication
- **Scalability**: Cannot handle large numbers of sprites efficiently

#### **Issue: No Render Order Respect**
**Current State:**
- Sprites don't maintain proper Z-ordering
- No depth sorting for transparency
- Rendering order inconsistencies

**Problems:**
- UI elements appear behind game objects
- Transparent sprites render incorrectly
- No control over sprite layering

**Required Solution:**
```cpp
// Proper instanced rendering with ordering
struct SpriteInstance {
    glm::mat4 transform;
    glm::vec4 color;
    glm::vec4 uvRect;
    float depth;        // For proper ordering
    int textureIndex;   // Texture array index
};

// Batch all sprites, sort by depth, render in batches
```

### 2. **WebViewContainer Buffer Management Issues**

#### **Issue: Inefficient ViewProjection Sharing**
**Current State:**
- Each WebView pipeline creates its own ViewProjection buffer
- Duplicate camera matrix data across systems
- No centralized ViewProjection management

**Problems:**
```cpp
// Current: Each pipeline has its own VP buffer
SolidFillPipeline solidPipeline(renderPass, vpBuffer1, ...);
ImagePipeline imagePipeline(renderPass, vpBuffer2, ...);
BorderPipeline borderPipeline(renderPass, vpBuffer3, ...);
// Multiple VP buffers with same data!
```

**Waste:**
- **Memory**: 3-4x ViewProjection buffer allocation
- **Bandwidth**: Multiple identical buffer updates per frame
- **Complexity**: Managing multiple VP buffers

#### **Issue: No Proper Model Matrix Management**
**Current State:**
- Model matrices passed via push constants
- Limited push constant space (128 bytes typical)
- No efficient batching of UI elements

**Problems:**
- Push constant size limitations
- Cannot batch UI elements with different transforms
- Inefficient for complex UI layouts

### 3. **Shared ViewProjection Architecture Issues**

#### **Problem: Inconsistent VP Management Across Systems**
**Current State:**
- Gizmos: Uses BufferTracker with shared VP buffer ✅
- SpriteRenderer: Uses push constants for VP ❌
- WebView: Multiple VP buffers per pipeline ❌

**Inconsistency Impact:**
- Different systems handle camera changes differently
- No unified camera update mechanism
- Difficult to maintain and debug

## 🛠️ Required Refactoring

### **Phase 1: Centralized ViewProjection Management**

#### **1.1 Create Shared ViewProjection System**
```cpp
class SharedViewProjectionManager {
public:
    void UpdateViewProjection(const glm::mat4& vp);
    std::shared_ptr<RT::BufferGroup> GetVPBuffer() const;
    void Update(RT::CommandRecordState& recordState);

private:
    std::unique_ptr<HostVisibleBufferTracker> _vpTracker;
};
```

#### **1.2 Update All Systems to Use Shared VP**
- **Gizmos**: Already uses proper pattern ✅
- **SpriteRenderer**: Remove VP from push constants, use shared buffer
- **WebView**: All pipelines share single VP buffer
- **Future Systems**: Use shared VP buffer by default

### **Phase 2: SpriteRenderer Instanced Rendering**

#### **2.1 Design Proper Instance Data Structure**
```cpp
struct SpriteInstanceData {
    glm::mat4 transform;    // Model matrix
    glm::vec4 color;        // Tint color
    glm::vec4 uvRect;       // Texture coordinates (u0,v0,u1,v1)
    float depth;            // Z-order for sorting
    int textureIndex;       // Index into texture array
    int padding[2];         // Align to 16 bytes
};
```

#### **2.2 Implement Render Order System**
```cpp
class SpriteRenderer {
public:
    void QueueSprite(const SpriteInstanceData& sprite);
    void Render(RT::CommandRecordState& recordState);

private:
    std::vector<SpriteInstanceData> _spriteQueue;

    void SortSpritesByDepth();
    void BatchSpritesByTexture();
    void RenderBatches(RT::CommandRecordState& recordState);
};
```

#### **2.3 Instanced Rendering Pipeline**
```cpp
// Vertex shader receives:
layout(location = 0) in vec3 position;        // Quad vertices
layout(location = 1) in vec2 uv;              // Quad UVs

// Instance data:
layout(location = 2) in mat4 instanceTransform;
layout(location = 3) in vec4 instanceColor;
layout(location = 4) in vec4 instanceUVRect;
layout(location = 5) in float instanceDepth;
layout(location = 6) in int instanceTextureIndex;
```

### **Phase 3: WebView Buffer Optimization**

#### **3.1 Unified Buffer Management**
```cpp
class WebViewUniformManager {
public:
    void SetViewProjection(const glm::mat4& vp);
    void AddModelMatrix(const glm::mat4& model);
    std::shared_ptr<RT::BufferGroup> GetVPBuffer() const;
    std::shared_ptr<RT::BufferGroup> GetModelBuffer() const;

private:
    std::shared_ptr<SharedViewProjectionManager> _vpManager;
    std::unique_ptr<HostVisibleBufferTracker> _modelTracker;
    std::vector<glm::mat4> _modelMatrices;
};
```

#### **3.2 Remove Duplicate VP Buffers**
```cpp
// Before: Each pipeline has own VP buffer
SolidFillPipeline(renderPass, vpBuffer1, ...);
ImagePipeline(renderPass, vpBuffer2, ...);

// After: All pipelines share VP buffer
auto sharedVP = vpManager.GetVPBuffer();
SolidFillPipeline(renderPass, sharedVP, ...);
ImagePipeline(renderPass, sharedVP, ...);
```

## 📋 Implementation Plan

### **Priority 1: Critical Performance Issues**

#### **Task 1.1: SpriteRenderer Instanced Rendering**
**Effort:** Large (2-3 weeks)
**Impact:** High performance improvement

**Steps:**
1. Design instance data layout
2. Create instanced vertex buffer system
3. Implement depth sorting
4. Add texture batching
5. Update shaders for instanced rendering
6. Performance testing and validation

#### **Task 1.2: Shared ViewProjection System**
**Effort:** Medium (1-2 weeks)
**Impact:** Memory reduction, consistency

**Steps:**
1. Create SharedViewProjectionManager
2. Update Gizmos to use shared system
3. Refactor SpriteRenderer VP handling
4. Update WebView pipelines
5. Test camera transitions

### **Priority 2: Architecture Cleanup**

#### **Task 2.1: WebView Buffer Consolidation**
**Effort:** Medium (1-2 weeks)
**Impact:** Memory efficiency, maintainability

**Steps:**
1. Audit current WebView buffer usage
2. Design unified buffer management
3. Refactor all WebView pipelines
4. Test HTML/CSS rendering
5. Performance validation

#### **Task 2.2: Rendering Order System**
**Effort:** Medium (1 week)
**Impact:** Correct visual rendering

**Steps:**
1. Define depth/layer system
2. Implement sprite sorting
3. Add transparency handling
4. Test with complex UI scenes

## 🎯 Success Metrics

### **Performance Targets**
- **Sprite Rendering**: 10,000+ sprites at 60 FPS (vs current ~100-500)
- **Draw Calls**: <10 draw calls for typical UI (vs current 50-200)
- **Memory Usage**: 70% reduction in VP buffer memory
- **Frame Time**: 50% reduction in rendering overhead

### **Quality Targets**
- **Rendering Order**: Correct depth sorting for all sprites
- **Transparency**: Proper alpha blending order
- **Consistency**: All systems use same VP management
- **Maintainability**: Single code path for buffer management

## 🚧 Breaking Changes

### **API Changes Required**

#### **SpriteRenderer API**
```cpp
// Old API (immediate rendering)
spriteRenderer.DrawSprite(position, texture, color);

// New API (queued rendering)
spriteRenderer.QueueSprite({
    .transform = transform,
    .color = color,
    .uvRect = uvRect,
    .depth = depth,
    .textureIndex = textureIdx
});
// Must call Render() to execute
spriteRenderer.Render(recordState);
```

#### **WebView Integration**
```cpp
// Old: Each pipeline creates own resources
webView.Initialize(renderPass);

// New: Shared resource management
webView.Initialize(renderPass, sharedVPManager);
```

## 📝 Migration Strategy

### **Backward Compatibility**
1. **Phase 1**: Add new APIs alongside old ones
2. **Phase 2**: Update example usage to new APIs
3. **Phase 3**: Deprecate old APIs with warnings
4. **Phase 4**: Remove old APIs in next major version

### **Testing Strategy**
1. **Unit Tests**: Each rendering system in isolation
2. **Integration Tests**: Combined rendering scenarios
3. **Performance Tests**: Before/after benchmarks
4. **Visual Tests**: Screenshot comparison for correctness

## 🔧 Implementation Notes

### **Shader Changes Required**
- **Sprite Vertex Shader**: Add instanced rendering support
- **WebView Shaders**: Update to use shared VP buffer
- **Uniform Buffer Layouts**: Standardize across all systems

### **Memory Layout Considerations**
- **Alignment**: Ensure 16-byte alignment for GPU data
- **Padding**: Account for GPU memory alignment requirements
- **Caching**: Consider cache-friendly data layouts

### **Platform Considerations**
- **Mobile GPUs**: Ensure instanced rendering performs well
- **Vulkan Limits**: Respect device limits for instance counts
- **Memory Budget**: Monitor VRAM usage on integrated GPUs

---

## 📚 References

- [Vulkan Instanced Rendering Best Practices](https://docs.vulkan.org)
- [GPU Memory Management Guidelines](docs/MEMORY_MANAGEMENT.md)
- [Watermill Rendering Architecture](docs/ARCHITECTURE.md)

---

**Document Version:** 1.0
**Last Updated:** 2025-09-21
**Next Review:** After Phase 1 completion