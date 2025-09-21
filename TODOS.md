# Watermill Engine TODOs

This document tracks critical architectural issues and refactoring tasks for the Watermill engine.

## 🚨 Critical Rendering System Issues

### **SpriteRenderer Problems**

#### **1. No Proper Instanced Rendering**
**Current Issue:**
- Each sprite = separate draw call
- No batching, poor performance with many sprites
- Cannot handle more than ~100-500 sprites efficiently

**Required Fix:**
```cpp
// Current (BAD): Individual draw calls
for (auto& sprite : sprites) {
    spriteRenderer.Draw(sprite); // Separate vkCmdDraw() each time
}

// Needed (GOOD): Instanced rendering
spriteRenderer.QueueSprite(spriteData);  // Batch collection
spriteRenderer.Render(recordState);      // Single instanced draw call
```

#### **2. No Render Order Respect**
**Current Issue:**
- Sprites don't maintain Z-order
- UI appears behind/in front incorrectly
- No depth sorting for transparency

**Required Fix:**
- Add depth field to sprite data
- Sort sprites by depth before rendering
- Proper transparency handling

### **WebViewContainer Problems**

#### **1. Inefficient ViewProjection Sharing**
**Current Issue:**
- Each WebView pipeline creates own ViewProjection buffer
- 3-4x memory waste (SolidFill, Image, Border pipelines each have VP buffer)
- Multiple identical buffer updates per frame

**Required Fix:**
- Single shared ViewProjection buffer across all WebView pipelines
- Centralized ViewProjection management

#### **2. Model Matrix Management**
**Current Issue:**
- Model matrices passed via push constants
- Limited push constant space (128 bytes)
- Cannot batch UI elements efficiently

**Required Fix:**
- Move model matrices to uniform buffer
- Enable batching of UI elements with different transforms

## 🛠️ Required Refactoring Tasks

### **Phase 1: ViewProjection Architecture Unification**
**Status:** 🟡 Partially Complete (Gizmos ✅, Others ❌)

**What's Done:**
- Gizmos uses proper BufferTracker with shared ViewProjection ✅

**What's Needed:**
- SpriteRenderer: Remove VP from push constants, use shared VP buffer
- WebView: Consolidate all pipelines to use single shared VP buffer
- Create SharedViewProjectionManager class

### **Phase 2: SpriteRenderer Instanced Rendering**
**Status:** 🔴 Not Started

**Required Implementation:**
```cpp
struct SpriteInstanceData {
    glm::mat4 transform;    // Model matrix
    glm::vec4 color;        // Tint color
    glm::vec4 uvRect;       // Texture coordinates
    float depth;            // Z-order for sorting
    int textureIndex;       // Texture array index
};

class SpriteRenderer {
    void QueueSprite(const SpriteInstanceData& sprite);
    void Render(RT::CommandRecordState& recordState);  // Batched rendering
};
```

**Benefits:**
- 10,000+ sprites vs current ~100-500
- <10 draw calls vs current 50-200
- Proper rendering order
- Better GPU utilization

### **Phase 3: WebView Buffer Optimization**
**Status:** 🔴 Not Started

**Required Changes:**
```cpp
// Before: Each pipeline has own VP buffer (WASTEFUL)
SolidFillPipeline(renderPass, vpBuffer1, ...);
ImagePipeline(renderPass, vpBuffer2, ...);
BorderPipeline(renderPass, vpBuffer3, ...);

// After: Shared VP buffer (EFFICIENT)
auto sharedVP = vpManager.GetVPBuffer();
SolidFillPipeline(renderPass, sharedVP, ...);
ImagePipeline(renderPass, sharedVP, ...);
BorderPipeline(renderPass, sharedVP, ...);
```

## 📋 Implementation Priority

### **High Priority** 🔥
1. **SpriteRenderer Instanced Rendering** - Critical performance bottleneck
2. **Shared ViewProjection System** - Memory waste and inconsistency

### **Medium Priority** 🟡
3. **WebView Buffer Consolidation** - Memory efficiency
4. **Rendering Order System** - Visual correctness

### **Low Priority** 🟢
5. **Performance Profiling Tools** - Optimization aids
6. **Render Statistics** - Debug information

## 🎯 Success Metrics

### **Performance Targets**
- **Sprites**: 10,000+ at 60 FPS (current: ~100-500)
- **Draw Calls**: <10 for typical UI (current: 50-200)
- **Memory**: 70% reduction in VP buffer usage
- **Frame Time**: 50% reduction in rendering overhead

### **Quality Targets**
- Correct sprite depth sorting
- Proper transparency rendering
- Consistent ViewProjection management across all systems

## 🚧 Breaking Changes Required

### **SpriteRenderer API Changes**
```cpp
// Old API (immediate rendering) - DEPRECATED
spriteRenderer.DrawSprite(position, texture, color);

// New API (queued rendering) - REQUIRED
spriteRenderer.QueueSprite({
    .transform = transform,
    .color = color,
    .depth = depth,
    .textureIndex = textureIdx
});
spriteRenderer.Render(recordState);  // Execute batch
```

### **WebView Integration Changes**
```cpp
// Old: Each pipeline manages own resources
webView.Initialize(renderPass);

// New: Shared resource management
webView.Initialize(renderPass, sharedVPManager);
```

## 📝 Notes

### **Why Current Architecture is Bad**
1. **Performance**: Excessive draw calls kill GPU performance
2. **Memory**: Duplicate ViewProjection buffers waste VRAM
3. **Maintainability**: Inconsistent patterns across systems
4. **Scalability**: Cannot handle production sprite counts
5. **Correctness**: No proper rendering order control

### **Root Cause**
- Each system evolved independently without architectural coordination
- No consideration for batching/instancing during initial implementation
- ViewProjection management was added per-system rather than centralized

### **Impact on Projects**
- **TimeShift Game**: Limited sprite count, UI rendering issues
- **Future Games**: Cannot scale to production requirements
- **Development**: Difficult to debug rendering issues

---

**Priority**: 🚨 **CRITICAL - Must be addressed before production use**

**Estimated Effort**: 4-6 weeks for complete refactor

**Next Steps**:
1. Start with SpriteRenderer instanced rendering
2. Implement shared ViewProjection system
3. Refactor WebView buffer management

---

**Last Updated:** 2025-09-21
**Status:** Planning Phase