#include <naui/naui.h>
#include <stdio.h>

NAUI_APP("Sandbox")

void naui_app_start(void)
{
	naui_load_theme("Default");
	naui_load_font(0, "MYRIADPRO-REGULAR");
	NAUI_ATTACH_PANEL(test);
	NAUI_ATTACH_PANEL(test2);
	naui_deserialize_viewport("Default_Layout.json"); // finds already attached panels based on the serialized type name and docks them to the main viewport as serialized
}

void naui_app_end(void)
{
	naui_serialize_viewport("Default_Layout.json"); // serializes the panels in the main viewport based on the type (test, test2)
}

static void render_main_titlebar(const char *title)
{
	const float titlebar_height = 32.0f;
	naui_app_set_caption_height(naui_any_panel_hovered() ? 0 : (int32_t)titlebar_height);

	Naui_Vec2 padding = naui_theme_vec2(NAUI_PANEL_TITLEBAR_PADDING_TAG);
	leaf({
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(titlebar_height)},
		.color = {naui_theme_leaf_color(NAUI_PANEL_TITLEBAR_BG_COLOR_TAG)},
		.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
		.padding = LEAF_PADDING_AXES(padding.x, 0.0f)
	})
	{
		leaf({
			.size = {LEAF_SIZE_DERIVED, LEAF_SIZE_PERCENT(0.5f)},
			.image = naui_get_image("logo-small"),
			.color = LEAF_COLOR_WHITE,
			.aspect_ratio = 1.0f
		});
		leaf({
			.positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
			.size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
			.child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
		})
		leaf_text(title, {
			.font_size = naui_theme_float(NAUI_PANEL_FONT_SIZE_TAG),
			.color = naui_theme_leaf_color(NAUI_PANEL_TITLEBAR_TEXT_COLOR_TAG),
			.alignment = LEAF_TEXT_ALIGN_CENTER
		});
	}
}

void naui_app_update(void)
{
	render_main_titlebar("Hello World :3");
	naui_render_panels_and_viewport();
	if (naui_key_pressed(NAUI_KEY_ESCAPE))
		naui_app_close();
}
