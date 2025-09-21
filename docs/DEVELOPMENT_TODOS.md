# Development TODOs

This document tracks major refactoring tasks and architectural improvements for the Watermill engine.

## High Priority Tasks

### 1. **Rendering Pipeline ViewProjection Architecture Cleanup** 🔥
**Status:** Identified
**Priority:** High
**Estimated Effort:** Large

#### Problem Statement
Inconsistent ViewProjection matrix handling across rendering pipelines:
- Some pipelines use push constants for ViewProjection matrices
- Others use uniform buffers
- ViewProjection struct gets mixed with other data in push constants
- No standardized approach across different renderers (Gizmos, Sprite, WebView)

#### Current State Analysis
- **BufferTracker exists**: `engine/render_system/BufferTracker.hpp` provides `HostVisibleBufferTracker` and `LocalBufferTracker`
- **Gizmos**: Uses ViewProjection in push constants (LinePipeline, PointPipeline)
- **Sprite**: Needs analysis of current VP handling
- **WebView**: Needs analysis of current VP handling

#### Proposed Solution
1. **Standardize ViewProjection struct**: Pure VP matrix only, no mixed data
2. **Use BufferTracker for VP**: Move ViewProjection from push constants to uniform buffers
3. **Centralize VP management**: One shared ViewProjection buffer per render pass
4. **Clean push constants**: Keep only per-draw data (model matrix, color, etc.)

#### Required Changes
- **Shaders**: Update to use uniform buffer binding for ViewProjection instead of push constants
- **Pipelines**: Modify LinePipeline, PointPipeline, SpritePipeline descriptor set layouts
- **Renderers**: Update to use BufferTracker for ViewProjection management
- **Push Constants**: Remove ViewProjection, keep only per-draw data

#### Implementation Strategy
1. Start with Gizmos system (LinePipeline, PointPipeline)
2. Extend to other rendering systems
3. Update all related shaders
4. Test thoroughly across all rendering paths

---

## Medium Priority Tasks

### 2. **Physics2D Integration Completion**
**Status:** In Progress
**Priority:** Medium

Basic Physics2D system exists but needs full integration with entity system and debug visualization.

### 3. **Asset Pipeline Improvements**
**Status:** Planned
**Priority:** Medium

Asynchronous loading, hot-reloading, and optimization improvements needed.

---

## Notes

- Always maintain backward compatibility during refactoring
- Update documentation when implementing changes
- Profile performance impact of architectural changes
- Test thoroughly across all rendering systems

---

**Last Updated:** 2025-09-21