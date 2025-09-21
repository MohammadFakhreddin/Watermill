# WebView Integration Documentation

## Overview

The WebView module enables HTML/CSS rendering within the Vulkan graphics pipeline, allowing developers to create rich user interfaces using familiar web technologies while maintaining high-performance GPU rendering. This integration bridges the gap between web content and native graphics.

## Architecture

### Core Design

The WebView module acts as a rendering backend for the litehtml library, implementing the `litehtml::document_container` interface to translate HTML/CSS rendering commands into Vulkan draw calls.

```
HTML/CSS Content
       ↓
   LiteHTML Parser
       ↓
WebViewContainer (document_container)
       ↓
Specialized Vulkan Pipelines
       ↓
   GPU Rendering
```

## Key Components

### WebViewContainer (`webview/WebViewContainer.hpp`)

The main integration point between litehtml and Vulkan.

**Key Responsibilities:**
- HTML document parsing and management
- CSS style application
- Rendering callback implementation
- DOM manipulation API
- Resource management

**Core Methods:**
```cpp
class WebViewContainer : public litehtml::document_container {
public:
    // Document management
    void LoadHTML(const std::string& html, const std::string& css);
    void Update(float deltaTime);
    void Render(const CommandRecordState& state);

    // DOM manipulation
    litehtml::element::ptr GetElementById(const std::string& id);
    void SetText(const std::string& id, const std::string& text);
    void AddClass(const std::string& id, const std::string& className);
    void RemoveClass(const std::string& id, const std::string& className);

    // Event handling
    void OnMouseMove(int x, int y);
    void OnMouseClick(int x, int y, MouseButton button);

protected:
    // Litehtml callbacks
    void draw_solid_fill(const position& pos, web_color color, const border_radiuses& radius) override;
    void draw_borders(const borders& borders, const position& draw_pos, bool root) override;
    void draw_image(const char* src, const char* baseurl, const position& pos) override;
    void draw_text(const char* text, font_metrics* fm, const position& pos, web_color color) override;
};
```

## Rendering Pipelines

### SolidFillPipeline (`webview/renderer/SolidFillPipeline.hpp`)

Renders solid color backgrounds with border radius support.

**Shader Features:**
- Distance field calculation for rounded corners
- Per-vertex color interpolation
- Instanced rendering for efficiency

**Vertex Data:**
```hlsl
struct Vertex {
    float2 position;
    float4 color;
    float4 borderRadius; // tl, tr, br, bl
};
```

### ImagePipeline (`webview/renderer/ImagePipeline.hpp`)

Handles image rendering with texture sampling.

**Features:**
- Texture coordinate mapping
- Border radius clipping
- Bilinear/trilinear filtering
- Mipmap support

### BorderPipeline (`webview/renderer/BorderPipeline.hpp`)

Renders CSS borders with complex styles.

**Capabilities:**
- Variable width borders
- Different colors per side
- Border radius support
- Dashed/dotted styles (future)

### TextOverlayPipeline

Font rendering with text shaping.

**Features:**
- TrueType font support
- Unicode text rendering
- Kerning and ligatures
- Subpixel positioning

## HTML to Vulkan Translation

### Rendering Flow

1. **Parse Phase**
   ```cpp
   auto doc = litehtml::document::createFromString(html, this, context);
   ```

2. **Layout Phase**
   ```cpp
   doc->render(width);  // Calculate positions
   ```

3. **Draw Phase**
   ```cpp
   doc->draw(0, 0, clip);  // Triggers callbacks
   ```

4. **Command Recording**
   ```cpp
   void draw_solid_fill(...) {
       // Record Vulkan commands
       solidFillRenderer.Draw(commandBuffer, ...);
   }
   ```

## CSS Feature Support

### Supported Properties

**Layout:**
- `display`: block, inline, inline-block, none
- `position`: static, relative, absolute, fixed
- `width`, `height`, `min-width`, `max-width`
- `margin`, `padding`
- `float`, `clear`

**Styling:**
- `background-color`, `background-image`
- `border`: width, style, color, radius
- `color`, `font-family`, `font-size`
- `text-align`, `vertical-align`
- `opacity`, `visibility`

**Advanced:**
- `border-radius` with per-corner values
- `box-shadow` (limited)
- `transform`: translate, scale, rotate
- `z-index` for layering

### Limitations

- No CSS animations (use code-based animation)
- Limited gradient support
- No video elements
- Simplified flexbox/grid

## DOM Manipulation

### Runtime Updates

```cpp
// Update text content
container.SetText("score", "1000");

// Toggle classes
container.AddClass("button", "active");
container.RemoveClass("button", "disabled");

// Direct element access
auto element = container.GetElementById("player-health");
if (element) {
    element->set_attr("style", "width: 50%");
}
```

### Event Handling

```cpp
// Mouse events
container.OnMouseMove(mouseX, mouseY);
container.OnMouseClick(mouseX, mouseY, MouseButton::Left);

// Check hover state
if (container.IsHovered("button-start")) {
    // Handle hover
}
```

## Performance Optimization

### Batching Strategies

1. **State Sorting**: Group similar draw calls
2. **Texture Atlasing**: Combine small images
3. **Instanced Rendering**: Draw multiple elements at once
4. **Dirty Tracking**: Only update changed regions

### Memory Management

```cpp
class WebViewContainer {
    // Double buffering for smooth updates
    std::vector<RenderState> _states;
    int _activeIdx = 0;

    // Resource pooling
    std::unordered_map<size_t, BufferHandle> _bufferCache;
    std::unordered_map<std::string, TextureHandle> _textureCache;
};
```

### Update Strategies

```cpp
// Partial updates
container.UpdateElement("timer", newTime);

// Batch updates
container.BeginUpdate();
container.SetText("score", score);
container.SetText("lives", lives);
container.EndUpdate();  // Single re-layout

// Conditional rendering
if (container.NeedsRedraw()) {
    container.Render(commandBuffer);
}
```

## Integration Examples

### Game HUD

```html
<div id="hud">
    <div class="health-bar">
        <div id="health-fill" style="width: 100%"></div>
    </div>
    <div id="score">0</div>
    <div id="ammo">30/30</div>
</div>
```

```css
#hud {
    position: fixed;
    top: 10px;
    left: 10px;
    color: white;
}

.health-bar {
    width: 200px;
    height: 20px;
    border: 2px solid white;
    background: rgba(0,0,0,0.5);
}

#health-fill {
    height: 100%;
    background: linear-gradient(to right, red, green);
}
```

```cpp
// Update health bar
float healthPercent = player.health / player.maxHealth;
container.SetStyle("health-fill", "width", std::to_string(healthPercent * 100) + "%");
```

### Menu System

```html
<div id="main-menu">
    <h1>Game Title</h1>
    <button id="btn-start" class="menu-button">Start Game</button>
    <button id="btn-options" class="menu-button">Options</button>
    <button id="btn-exit" class="menu-button">Exit</button>
</div>
```

```cpp
// Handle menu selection
if (container.IsClicked("btn-start")) {
    StartGame();
} else if (container.IsClicked("btn-options")) {
    ShowOptions();
}
```

## Shader Implementation

### Border Radius Calculation

```hlsl
float roundedBoxSDF(float2 pos, float2 size, float4 radius) {
    // Determine which corner we're in
    float2 quadrant = step(size * 0.5, pos);
    float cornerRadius = mix(
        mix(radius.x, radius.y, quadrant.x),
        mix(radius.w, radius.z, quadrant.x),
        quadrant.y
    );

    // Calculate distance to rounded corner
    float2 cornerPos = size * quadrant;
    float2 cornerDist = abs(pos - cornerPos) - (size * 0.5 - cornerRadius);

    return length(max(cornerDist, 0.0)) - cornerRadius;
}

// In fragment shader
float dist = roundedBoxSDF(fragPos, elementSize, borderRadius);
float alpha = 1.0 - smoothstep(-1.0, 1.0, dist);
outColor = float4(color.rgb, color.a * alpha);
```

## Best Practices

### HTML/CSS Guidelines

1. **Keep It Simple**: Avoid complex layouts
2. **Use IDs**: For elements that need updates
3. **Minimize DOM**: Fewer elements = better performance
4. **Cache Selectors**: Store element references
5. **Batch Updates**: Group DOM changes

### Performance Tips

1. **Static Content**: Pre-render unchanging UI
2. **Texture Atlas**: Combine UI images
3. **Update Throttling**: Limit update frequency
4. **Conditional Rendering**: Skip unchanged content
5. **Profile**: Monitor draw call counts

### Memory Considerations

1. **Release Resources**: Clean up unused textures
2. **Pool Objects**: Reuse render states
3. **Limit Fonts**: Each font uses memory
4. **Compress Textures**: Use appropriate formats
5. **Monitor Usage**: Track memory allocation

## Troubleshooting

### Common Issues

**UI Not Rendering:**
- Check HTML/CSS syntax
- Verify container initialization
- Ensure render pass is active

**Poor Performance:**
- Reduce DOM complexity
- Enable batching
- Check texture sizes
- Profile draw calls

**Visual Artifacts:**
- Verify depth testing settings
- Check blend modes
- Update viewport correctly

**Text Issues:**
- Ensure fonts are loaded
- Check text encoding (UTF-8)
- Verify font metrics

## Future Enhancements

Planned improvements:
- CSS animations
- SVG rendering
- Canvas 2D API
- WebGL context
- JavaScript integration (via embedded engine)
- Improved CSS Grid/Flexbox
- Custom elements/components

The WebView integration provides a powerful way to create modern, responsive user interfaces while maintaining the performance benefits of native GPU rendering. This hybrid approach combines the best of web technologies with high-performance graphics programming.