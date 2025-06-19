#pragma once

#include "RenderTypes.hpp"
#include "render_pass/DisplayRenderPass.hpp"
#include "IScene.hpp"

#include <SDL_events.h>
#include <functional>
#include <unordered_map>

// TODO: Resource manager
// TODO: Optimized json
// TODO: User interface fix
// TODO: Sprite renderer resource allocation fix

namespace MFA
{
    class TextOverlayPipeline;
    class ImageRenderer;
    class BorderRenderer;
    class SolidFillRenderer;
    class CustomFontRenderer;
    class Blob;
}

class SpriteRenderer;

class TimeShiftApp : public std::enable_shared_from_this<TimeShiftApp>
{
public:

    void Run();

private:

    void Update(float deltaTime);

    void Render(MFA::RT::CommandRecordState & recordState);

    void Resize();

    void OnSDL_Event(SDL_Event* event);

    void Reload();

    void InitFontPipeline();

    void AddFont(char const *name, char const *path);

    [[nodiscard]]
    std::shared_ptr<MFA::Blob> RequestBlob(char const *address, bool ignoreCache);

    [[nodiscard]]
    std::shared_ptr<MFA::CustomFontRenderer> RequestFont(char const *font);

    [[nodiscard]]
    std::tuple<std::shared_ptr<MFA::RT::GpuTexture>, glm::vec2> RequestImage(char const *imageName);

    [[nodiscard]]
    glm::vec2 GetInputAxis() const { return _inputAxis;}

    [[nodiscard]]
    bool IsInputA_Pressed() const { return _inputA;}

    [[nodiscard]]
    bool IsInputB_Pressed() const { return _inputB;}

    std::shared_ptr<MFA::DisplayRenderPass> _displayRenderPass;

    std::shared_ptr<MFA::SolidFillRenderer> _solidFillRenderer;
    std::shared_ptr<MFA::BorderRenderer> _borderRenderer;
    std::shared_ptr<MFA::ImageRenderer> _imageRenderer;
    std::shared_ptr<MFA::TextOverlayPipeline> _fontPipeline;
    std::shared_ptr<SpriteRenderer> _spriteRenderer;

    std::unordered_map<std::string, std::shared_ptr<MFA::Blob>> _blobMap;
    std::unordered_map<std::string, std::shared_ptr<MFA::CustomFontRenderer>> _fontMap{};
    std::unordered_map<std::string, std::tuple<std::shared_ptr<MFA::RT::GpuTexture>, glm::vec2>> _imageMap;

    enum class SceneID : int
    {
        Invalid = -1,
        Menu = 0,
        Scoreboard = 1,
        Level1 = 2,
        Level2 = 3,
        Level3 = 4,
        Count = 5
    };

    using SceneRecipe = std::function<std::shared_ptr<IScene>()>;
    std::vector<SceneRecipe> _sceneRecipes {};

    std::shared_ptr<IScene> _currentScene;
    std::vector<std::tuple<std::shared_ptr<IScene>, int>> _previousScenes {};
    SceneID _activeSceneID = SceneID::Invalid;
    SceneID _nextSceneID = SceneID::Invalid;

    glm::vec2 _inputAxis{};
    bool _inputA{};
    bool _inputB{};
};
