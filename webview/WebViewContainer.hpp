#pragma once

#include "renderer/CustomFontRenderer.hpp"
#include "renderer/ImageRenderer.hpp"

#include <gumbo.h>
#include "litehtml.h"

#include <glm/glm.hpp>

namespace MFA
{
    struct LocalBufferTracker;
    class Blob;
    class BorderRenderer;
    class ImageRenderer;
    class SolidFillRenderer;
    class CustomFontRenderer;
}
// TODO: Feature request, ID for items.
// Idea: I can port this for unity
class WebViewContainer : public litehtml::document_container
{

public:

	using FontRenderer = MFA::CustomFontRenderer;
	using SolidFillRenderer = MFA::SolidFillRenderer;
    using ImageRenderer = MFA::ImageRenderer;
    using BorderRenderer = MFA::BorderRenderer;
    using RequestBlob = std::function<std::shared_ptr<MFA::Blob>(char const * address, bool force)>;
    using RequestFont = std::function<std::shared_ptr<FontRenderer>(char const * font)>;
    using RequestImage = std::function<std::tuple<std::shared_ptr<MFA::RT::GpuTexture>, glm::vec2>(char const * path)>;

    struct Params
    {
        std::shared_ptr<SolidFillRenderer> solidFillRenderer{};
        std::shared_ptr<ImageRenderer> imageRenderer{};
        std::shared_ptr<BorderRenderer> borderRenderer{};
        RequestBlob requestBlob;
        RequestFont requestFont;
        RequestImage requestImage;
    };

	// TODO: We need an image renderer class as well
    explicit WebViewContainer(
        char const * htmlAddress,
        litehtml::position clip,
        Params params
	);

	~WebViewContainer() override;

	void Update();

    void UpdateBuffer(MFA::RT::CommandRecordState& recordState);

	void DisplayPass(MFA::RT::CommandRecordState& recordState);

    [[nodiscard]]
    GumboNode * GetElementById(const char *id);

    [[nodiscard]]
    GumboNode * GetElementByTag(const char* tag);

    void SetText(GumboNode * node, char const * text);

    void AddClass(GumboNode * node, char const * keyword);

    void RemoveClass(GumboNode * node, char const * keyword);

    // TODO: Add Item!
    // TODO: Modify style!

    void OnReload(litehtml::position clip);

    void OnResize(litehtml::position clip);

protected:

    [[nodiscard]]
    GumboNode * GetElementById(const char *id, GumboNode *node);

    [[nodiscard]]
    GumboNode * GetElementByTag(const char* tag, GumboNode* node);

	litehtml::element::ptr create_element(
		const char* tag_name,
		const litehtml::string_map& attributes,
		const std::shared_ptr<litehtml::document>& doc
	) override;

	litehtml::uint_ptr create_font(const char* faceName,
		int size,
		int weight,
		litehtml::font_style italic,
		unsigned int decoration,
		litehtml::font_metrics* fm
	) override;

	void del_clip() override;

	void delete_font(litehtml::uint_ptr hFont) override;

	void draw_borders(
		litehtml::uint_ptr hdc,
		const litehtml::borders& borders,
		const litehtml::position& draw_pos,
		bool root
	) override;

	void draw_conic_gradient(
		litehtml::uint_ptr hdc,
		const litehtml::background_layer& layer,
		const litehtml::background_layer::conic_gradient& gradient
	) override;

	void draw_image(
		litehtml::uint_ptr hdc,
		const litehtml::background_layer& layer,
		const std::string& url,
		const std::string& base_url
	) override;

	void draw_linear_gradient(
		litehtml::uint_ptr hdc,
		const litehtml::background_layer& layer,
		const litehtml::background_layer::linear_gradient& gradient
	) override;

	void draw_list_marker(
		litehtml::uint_ptr hdc,
		const litehtml::list_marker& marker
	) override;

	void draw_radial_gradient(
		litehtml::uint_ptr hdc,
		const litehtml::background_layer& layer,
		const litehtml::background_layer::radial_gradient& gradient
	) override;

	void draw_solid_fill(
		litehtml::uint_ptr hdc,
		const litehtml::background_layer& layer,
		const litehtml::web_color& color
	) override;

	void draw_text(litehtml::uint_ptr hdc,
		const char* text,
		litehtml::uint_ptr hFont,
		litehtml::web_color color,
		const litehtml::position& pos
	) override;

	void get_client_rect(litehtml::position& client) const override;

	const char* get_default_font_name() const override;

	int get_default_font_size() const override;

	void get_image_size(
		const char* src,
		const char* baseurl,
		litehtml::size& sz
	) override;

	void get_language(litehtml::string& language, litehtml::string& culture) const override;

	void get_media_features(litehtml::media_features& media) const override;

	void import_css(litehtml::string& text, const litehtml::string& url, litehtml::string& baseurl) override;

	void link(const std::shared_ptr<litehtml::document>& doc, const litehtml::element::ptr& el) override;

	void load_image(const char* src, const char* baseurl, bool redraw_on_ready) override;

	void on_anchor_click(const char* url, const litehtml::element::ptr& el) override;

	void on_mouse_event(const litehtml::element::ptr& el, litehtml::mouse_event event) override;

	int pt_to_px(int pt) const override;

	void set_base_url(const char* base_url) override;

	void set_caption(const char* caption) override;

	void set_clip(const litehtml::position& pos, const litehtml::border_radiuses& bdr_radius) override;

	void set_cursor(const char* cursor) override;

	int text_width(const char* text, litehtml::uint_ptr hFont) override;

	void transform_text(litehtml::string& text, litehtml::text_transform tt) override;

private:

	[[nodiscard]]
	static glm::vec4 ConvertColor(litehtml::web_color const & webColor);

    void SwitchActiveState();

    std::string _htmlAddress{};
    std::shared_ptr<SolidFillRenderer> _solidFillRenderer = nullptr;
    std::shared_ptr<ImageRenderer> _imageRenderer = nullptr;
    std::shared_ptr<BorderRenderer> _borderRenderer = nullptr;
    RequestBlob _requestBlob{};
    RequestFont _requestFont{};
    RequestImage _requestImage{};

    std::string _parentAddress{};
    std::shared_ptr<MFA::Blob> _htmlBlob{};

	litehtml::position _clip {};

    struct FontData
    {
        int id = -1;
        int size = 14;
        std::shared_ptr<FontRenderer> renderer{};
    };
    std::vector<FontData> _fontList{};

private:

    litehtml::document::ptr _html = nullptr;
    GumboOutput* _gumboOutput{};

    float _bodyWidth{};
    float _bodyHeight{};
    glm::mat4 _modelMat{};

	bool _isDirty = true;
    // bool _freeData = false;

    template<typename Value>
    using StateMap = std::unordered_map<size_t, std::shared_ptr<Value>>;

    struct State
    {
        std::vector<std::function<void(MFA::RT::CommandRecordState &)>> drawCalls{};
        std::vector<std::function<void(MFA::RT::CommandRecordState &)>> bufferCalls{};
        StateMap<FontRenderer::TextData> textMap{};
        StateMap<ImageRenderer::ImageData> imageMap{};
        StateMap<MFA::LocalBufferTracker> solidMap{};
        StateMap<MFA::LocalBufferTracker> borderMap{};
        int lifeTime{};
    };
    std::vector<State> _states{};
    int _activeIdx = 0;
    State * _activeState = nullptr;
};
