#pragma once

#include "BedrockSignal.hpp"
#include "RenderBackend.hpp"
#include "render_pass/DisplayRenderPass.hpp"

#include "imgui.h"

#include <memory>

namespace MFA
{

	class UI
	{
	public:

        using CustomFontCallback = std::function<void(ImGuiIO & io)>;

        struct Params
        {
            bool lightMode = true;
            CustomFontCallback fontCallback;
        };

		explicit UI(std::shared_ptr<DisplayRenderPass> displayRenderPass, Params const & params);

		~UI();

        void Update(float deltaTimeInSec);

        bool Render(RT::CommandRecordState& recordState);

        void DisplayDockSpace();

        Signal<> UpdateSignal{};
	    Signal<> MenuBarSignal{};

        static bool InputText(char const * text, std::string & value);

	    template<typename T>
        static void List(
            char const * title,
            char const * element,
            std::vector<T> & list,
            std::function<void(int, T &)> DrawElement
        )
	    {
	        if (ImGui::TreeNode(title))
	        {

	            ImGui::PushItemWidth(100);
	            if (ImGui::Button("+") && list.size() < 50)
	            {
	                list.emplace_back();
	            }

	            ImGui::SameLine();

	            if (ImGui::Button("-") && list.empty() == false)
	            {
	                list.pop_back();
	            }
	            ImGui::PopItemWidth();

	            if (list.empty() == false)
	            {
	                for (int idx = 0; idx < list.size(); ++idx)
	                {
	                    if (idx != 0)
	                    {
	                        ImGui::Separator();
	                    }
	                    std::string elementTitle;
	                    MFA_STRING(elementTitle, "%s %d", element, idx);
	                    if (ImGui::TreeNode(elementTitle.c_str()))
	                    {
	                        DrawElement(idx, list[idx]);
	                        ImGui::TreePop();
	                    }
	                }
	            }

	            ImGui::TreePop();
	        }

	    }

	    static bool ComboString(
            char const * title,
            std::string & activeItem,
            std::vector<char const *> const & allItems
        );

	    template<typename T>
        static void ComboEnum(
            char const * title,
            T & value,
            std::vector<T> enums,
            std::unordered_map<T, int> enumIndices,
            std::vector<char const *> choices
        )
	    {
	        int idx = 0;
	        auto const findResult = enumIndices.find(value);
	        if (findResult != enumIndices.end())
	        {
	            idx = findResult->second;
	        }
	        ImGui::Combo(title, &idx, choices.data(), choices.size());
	        value = enums[idx];
	    }

		// Static wrapper functions for private non-static methods
		[[nodiscard]]
		static ImFont* AddFont(char const * path)
		{
			if (Instance != nullptr)
			{
				return Instance->_AddFont(path);
			}
			return nullptr;
		}

		[[nodiscard]]
		static bool HasFocus()
		{
			if (Instance != nullptr)
			{
				return Instance->_HasFocus();
			}
			return false;
		}

		static void BeginWindow(std::string const& windowName, ImGuiWindowFlags flags = 0)
		{
			if (Instance != nullptr)
			{
				Instance->Private_BeginWindow(windowName, flags);
			}
		}

		static void BeginWindow(char const * windowName, ImGuiWindowFlags flags = 0)
		{
			if (Instance != nullptr)
			{
				Instance->Private_BeginWindow(windowName, flags);
			}
		}

		static void EndWindow()
		{
			if (Instance != nullptr)
			{
				Instance->Private_EndWindow();
			}
		}

		[[nodiscard]]
		static bool IsDarkMode()
		{
			if (Instance != nullptr)
			{
				return Instance->Private_IsDarkMode();
			}
			return false;
		}

		[[nodiscard]]
		static ImTextureID AddTexture(VkSampler sampler, VkImageView imageView)
		{
			if (Instance != nullptr)
			{
				return Instance->Private_AddTexture(sampler, imageView);
			}
			return nullptr;
		}

		static void UpdateTexture(ImTextureID textureID, VkSampler sampler, VkImageView imageView)
		{
			if (Instance != nullptr)
			{
				Instance->Private_UpdateTexture(textureID, sampler, imageView);
			}
		}

		static void RemoveTexture(ImTextureID textureID)
		{
			if (Instance != nullptr)
			{
				Instance->_RemoveTexture(textureID);
			}
		}

	private:

		// Private non-static methods (called by static wrappers)
		[[nodiscard]]
		ImFont* _AddFont(char const * path);

		[[nodiscard]]
		bool _HasFocus() const;

		void Private_BeginWindow(std::string const& windowName, ImGuiWindowFlags flags = 0);

		void Private_BeginWindow(char const * windowName, ImGuiWindowFlags flags = 0);

		void Private_EndWindow();

		[[nodiscard]]
		bool Private_IsDarkMode() const { return _darkMode; }

		[[nodiscard]]
		ImTextureID Private_AddTexture(VkSampler sampler, VkImageView imageView);

		void Private_UpdateTexture(ImTextureID textureID, VkSampler sampler, VkImageView imageView);

		void _RemoveTexture(ImTextureID textureID);

        struct PushConstants
        {
            float scale[2];
            float translate[2];
        };

        void OnResize();

        void CreateDescriptorSetLayout();

        void CreatePipeline();

        void CreateFontTexture(CustomFontCallback const & fontCallback);

		static void BindKeyboard();

        void UpdateDescriptorSets();

		static void UpdateMousePositionAndButtons();

        void UpdateMouseCursor() const;

        int EventWatch(SDL_Event* event);

	private:

	    inline static UI* Instance = nullptr;

        std::shared_ptr<DisplayRenderPass> _displayRenderPass{};

        std::shared_ptr<RT::SamplerGroup> _fontSampler{};
        std::shared_ptr<RT::DescriptorSetLayoutGroup> _descriptorSetLayout{};
        std::shared_ptr<RT::DescriptorPool> _descriptorPool{};
        RT::DescriptorSetGroup _fontDescriptorSetGroup{};

	    struct ImageHolder
	    {

	        explicit ImageHolder(
	            std::shared_ptr<RT::DescriptorSetLayoutGroup> descriptorSetLayout,
                std::shared_ptr<RT::DescriptorPool> descriptorPool
            );

            ~ImageHolder();

	        std::shared_ptr<RT::DescriptorSetLayoutGroup> _descriptorSetLayout{};
	        std::shared_ptr<RT::DescriptorPool> _descriptorPool{};
	        RT::DescriptorSetGroup descriptorSetGroup{};

        };

        std::unordered_map<int, std::shared_ptr<ImageHolder>> _imageDescriptorSetGroups{};
	    int _nextImageIndex = 0;

        std::shared_ptr<RT::PipelineGroup> _pipeline{};
        std::shared_ptr<RT::GpuTexture> _fontTexture{};
        bool _hasFocus = false;

        std::vector<std::shared_ptr<MFA::Blob>> _cpuVertexBuffers{};
        std::vector<std::shared_ptr<MFA::Blob>> _cpuIndexBuffers{};
        std::vector<std::shared_ptr<RT::BufferAndMemory>> _gpuVertexBuffers{};
        std::vector<std::shared_ptr<RT::BufferAndMemory>> _gpuIndexBuffers{};
        
        SDL_Cursor* _mouseCursors[ImGuiMouseCursor_COUNT]{};
        int _eventWatchId = -1;
        SignalId _resizeSignalId = SignalIdInvalid;
        bool _darkMode = false;

        int _myWindowID = -1;
        std::string _iniLocation = "";

	};
    
}
