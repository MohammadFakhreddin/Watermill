#pragma once

#include "BedrockPath.hpp"
#include "LogicalDevice.hpp"
#include "RenderTypes.hpp"
#include "SceneRenderPass.hpp"
#include "GridRenderer.hpp"
#include "Time.hpp"
#include "UI.hpp"
#include "camera/ArcballCamera.hpp"

#include <SDL_events.h>

class EditorApp
{
public:

    explicit EditorApp();

    ~EditorApp();

    void Run();

private:

    void Update(float deltaTime);

    void Render(MFA::RT::CommandRecordState & recordState);

    void Resize();

    void Reload();

    void OnSDL_Event(SDL_Event* event);

    void OnUI(float deltaTimeSec);

    void PrepareEditorRenderPass();

    void PrepareGameRenderPass();

    void PrepareRenderPass(
        std::shared_ptr<SceneRenderResource> & renderResource,
        std::shared_ptr<SceneRenderPass> & renderPass,
        std::shared_ptr<SceneFrameBuffer> & frameBuffer,
        std::vector<ImTextureID> & textureIDs,
        VkExtent2D const & windowSize
    );

    void ApplyUI_Style();

    void DisplayParametersWindow();

    void DisplayHierarchyWindow();

    // You need to be able to select and view objects in the editor window
    void DisplayEditorWindow();

    void DisplayGameWindow();

    // Render parameters
    std::shared_ptr<MFA::Path> _path{};
    MFA::LogicalDevice * _device{};
    std::shared_ptr<MFA::UI> _ui{};
    std::unique_ptr<MFA::Time> _time{};
    std::shared_ptr<MFA::SwapChainRenderResource> _swapChainResource{};
    std::shared_ptr<MFA::DepthRenderResource> _depthResource{};
    std::shared_ptr<MFA::MSSAA_RenderResource> _msaaResource{};
    std::shared_ptr<MFA::DisplayRenderPass> _displayRenderPass{};
    std::shared_ptr<MFA::RT::SamplerGroup> _sampler{};

    std::shared_ptr<SceneFrameBuffer> _editorFrameBuffer{};
    std::shared_ptr<SceneRenderResource> _editorRenderResource{};
    std::shared_ptr<SceneRenderPass> _editorRenderPass{};
    std::vector<ImTextureID> _editorTextureID_List{};

    std::shared_ptr<SceneFrameBuffer> _gameFrameBuffer{};
    std::shared_ptr<SceneRenderResource> _gameRenderResource{};
    std::shared_ptr<SceneRenderPass> _gameRenderPass{};
    std::vector<ImTextureID> _gameTextureID_List{};

    struct OldScene
    {
        std::shared_ptr<SceneRenderResource> renderResource{};
        std::shared_ptr<SceneFrameBuffer> frameBuffer{};
        std::vector<ImTextureID> textureIDs{};
        int remLifeTime{};
    };
    std::vector<OldScene> oldScenes{};

    VkExtent2D _editorWindowSize{800, 800};
    bool _editorWindowResized = false;
    bool _editorWindowFocused = false;

    VkExtent2D _gameWindowSize{800, 800};
    bool _gameWindowResized = false;
    bool _gameWindowFocused = false;

    ImFont* _defaultFont{};
    ImFont* _boldFont{};

    // std::shared_ptr<MFA::HostVisibleBufferTracker> _lightBufferTracker{};
    std::shared_ptr<MFA::HostVisibleBufferTracker> _editorCameraBufferTracker{};
    std::shared_ptr<MFA::HostVisibleBufferTracker> _gameCameraBufferTracker{};

    std::shared_ptr<GridPipeline> _editorGridPipeline{};
    std::unique_ptr<GridRenderer> _editorGridRenderer{};

    std::unique_ptr<MFA::ArcballCamera> _editorCamera{};
    std::unique_ptr<MFA::ArcballCamera> _gameCamera{};

    int _activeImageIndex{};

    glm::vec3 _lightDirection = glm::vec3(-1.0f, 0.0f, -1.0f);
    glm::vec3 _lightColor {1.0f, 1.0f, 1.0f};
    float _lightIntensity = 1.0f;
    float _specularLightIntensity = 1.0f;
    int _shininess = 32;
    float _ambientStrength = 0.25f;

    struct Joint
    {
        float length = 3.0f;
        glm::vec2 angle {};

        bool isLengthFixed = true;
        bool isX_AngleFixed = false;
        bool isY_AngleFixed = false;

        glm::mat4 matrix {};
    };
    std::vector<Joint> _hierarchy{};

    glm::vec3 _ikTargetPosition = glm::vec3(7.0f, 1.0f, 7.0f);
    bool _ikEnabled = false;
    float _damping = 0.25f;
};
