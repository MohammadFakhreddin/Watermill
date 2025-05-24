#pragma once

#include "RenderTypes.hpp"
#include "render_pass/DisplayRenderPass.hpp"
#include "WebViewContainer.hpp"
#include "MenuScene.hpp"
#include "GameScene.hpp"

class TimeShiftApp
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

    std::unordered_map<std::string, std::shared_ptr<MFA::Blob>> _blobMap;
    std::unordered_map<std::string, std::shared_ptr<MFA::CustomFontRenderer>> _fontMap{};
    std::unordered_map<std::string, std::tuple<std::shared_ptr<MFA::RT::GpuTexture>, glm::vec2>> _imageMap;

    std::unique_ptr<MenuScene> _menuScene;
    std::unique_ptr<GameScene> _gameScene;
    std::vector<IScene *> _scenes {};
    int _activeSceneIndex = 0;
    int _nextSceneIndex = 0;

    glm::vec2 _inputAxis{};
    bool _inputA{};
    bool _inputB{};
};
