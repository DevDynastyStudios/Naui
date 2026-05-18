#include "naui/math/math.h"
#include "naui/utils/list.h"

#include <stdio.h>
#include <naui/naui.h>
#include <naui/renderer/asset_manager.h>
#include <magma/mgapp.h>

NAUI_APP("test")

typedef enum
{
	ICON_CIRCLE,
	ICON_SQUARE
} IconType;

typedef struct
{
	IconType type;
	char id[64];
} Icon;

typedef struct
{
	uint64_t id;
	float timer;
} AnimState;

static float font_size = 128.0f;
static Naui_List(Icon) g_icons = NULL;

#define ANIM_STATE_MAX 128
static AnimState g_anim_states[ANIM_STATE_MAX];

static AnimState* anim_state(Leaf_ID id)
{
	for(int i = 0; i < ANIM_STATE_MAX; i++)
	{
		if(g_anim_states[i].id == id.value)
			return &g_anim_states[i];
	}

	for(int i = 0; i < ANIM_STATE_MAX; i++)
	{
		if(g_anim_states[i].id > 0)
			continue;

		g_anim_states[i].id = id.value;
		return &g_anim_states[i];
	}

	return NULL;
}

void naui_app_start(void)
{
	naui_list_push(g_icons, ((Icon){
		.type = ICON_CIRCLE,
		.id = "circle_0"
	}));

	naui_list_push(g_icons, ((Icon){
		.type = ICON_SQUARE,
		.id = "square_0"
	}));

	for(int i = 0; i < ANIM_STATE_MAX; i++)
	{
		g_anim_states[i].id = 0;
	}
	NAUI_ATTACH_PANEL(test);
}

void naui_app_end(void)
{

}

void add_random_icon()
{
	bool circle = rand() % 2;
	Icon icon = { .type = circle ? ICON_CIRCLE : ICON_SQUARE };
	snprintf(icon.id, sizeof(icon.id), "%s_%d", circle ? "circle" : "square", (int)naui_list_len(g_icons));
	naui_list_push(g_icons, icon);
}

void circle_icon(const char* id)
{
	Leaf_ID circle_id = leaf_id(id);
	AnimState* hover_state = anim_state(circle_id);
	if(!hover_state)
		return;

	static float gradient_anim = 0.0f;
	
	gradient_anim += naui_delta_time();
	bool hovered = leaf_hovered(circle_id);
	hover_state->timer = naui_lerp(hover_state->timer, hovered ? 1.0f : 0.0f, naui_delta_time() * 12.0f);
	float hover_anim = hover_state->timer;

	float size = naui_lerp(0.9f, 1.0f, hover_anim);
	float border_width = naui_lerp(5.0f, 10.0f, hover_anim);
	float padding = naui_lerp(6.0f, 10.0f, hover_anim);
	uint8_t background = (uint8_t)naui_lerp(35.0f, 55.0f, hover_anim);

	Leaf_Color color1 = leaf_rgb(
		(uint8_t)(127 + sinf(gradient_anim * 1.2f) * 127),
		(uint8_t)(127 + sinf(gradient_anim * 2.0f) * 127),
		255
	);

	Leaf_Color color2 = leaf_rgb(
		255,
		(uint8_t)(127 + cosf(gradient_anim * 1.5f) * 127),
		(uint8_t)(127 + sinf(gradient_anim * 0.8f) * 127)
	);

	leaf({
		.size = { LEAF_SIZE_FIXED(240.0f), LEAF_SIZE_FIXED(240.0f) },
		.child_alignment = { LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER }
	})
	{
		leaf({
			.id = circle_id,
			.size = { LEAF_SIZE_PERCENT(size), LEAF_SIZE_PERCENT(size) },
			.rounding = 999.0f,
			.padding = LEAF_PADDING_ALL(padding),
			.image = naui_get_image("bebe"),
			.border = {
				leaf_gradient(color1, color2, gradient_anim * 1.5f),
				hovered ? 10.0f : 5.0f
			},

			.color = leaf_rgb(background, background, background + 10),
			.child_alignment = { LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER }
		})
		{
			leaf_text("Bebe", {
				.font_size = LEAF_SIZE_PERCENT(0.5f),
				.color = leaf_rgb(255, 255, 255),
				.alignment = LEAF_TEXT_ALIGN_CENTER
			});
		}
	}

}

void square_icon(const char* id)
{
	Leaf_ID square_id = leaf_id(id);
	AnimState* hover_state = anim_state(square_id);
	if(!hover_state)
		return;

	bool hovered = leaf_hovered(square_id);
	hover_state->timer = naui_lerp(hover_state->timer, hovered ? 1.0f : 0.0f, naui_delta_time() * 12.0f);
	float hover_anim = hover_state->timer;

	float size = naui_lerp(220.0f, 240.0f, hover_anim);
	float border_width = naui_lerp(5.0f, 10.0f, hover_anim);
	float padding = naui_lerp(6.0f, 10.0f, hover_anim);
	uint8_t background = (uint8_t)naui_lerp(35.0f, 55.0f, hover_anim);

	leaf({
		.id = square_id,
		.size = { LEAF_SIZE_FIXED(size * 1.5f), LEAF_SIZE_FIXED(size) },
		.rounding = 20.0f,
		.padding = LEAF_PADDING_ALL(padding),
		.image = naui_get_image("dude"),
		.border = {
			leaf_rgb(150, 150, 150),
			hovered ? 10.0f : 5.0f
		},

		.color = leaf_rgb(background, background, background + 10),
		.child_alignment = { LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER }
	})
	{
		leaf_text("Dude", {
			.font_size = naui_lerp(22.0f, 26.0f, hover_anim),
			.color = leaf_rgb(255, 255, 255),
			.alignment = LEAF_TEXT_ALIGN_CENTER
		});
	}
}

void title_bar(const char* title)
{
	Leaf_ID add_id = leaf_id("titlebar_add_button");
	bool hovered = leaf_hovered(add_id);

	if (hovered && mg_app_mouse_clicked(MG_MOUSE_BUTTON_LEFT))
		add_random_icon();

	leaf({
		.size = { LEAF_SIZE_GROW, LEAF_SIZE_FIXED(64) },
		.color = leaf_rgb(28, 28, 34),
		.direction = LEAF_LAYOUT_HORIZONAL,
		.child_alignment = { LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER },
		.padding = LEAF_PADDING_ALL(16.0f)
	})
	{
		leaf({
			.size = { LEAF_SIZE_GROW, LEAF_SIZE_FIT },
			.child_alignment = { LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER }
		})
		{
			leaf_text(title, {
				.font_size = 28.0f,
				.color = leaf_rgb(255, 255, 255),
				.alignment = LEAF_TEXT_ALIGN_CENTER
			});
		}

		leaf({
			.id = add_id,
			.size = { LEAF_SIZE_FIXED(36), LEAF_SIZE_FIXED(36) },
			.rounding = 8.0f,
			.color = hovered ? leaf_rgb(70, 70, 82) : leaf_rgb(50, 50, 60),
			.child_alignment = { LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER }
		})
		{
			leaf_text("+", {
				.font_size = 24.0f,
				.color = leaf_rgb(255, 255, 255),
				.alignment = LEAF_TEXT_ALIGN_CENTER
			});
		}
	}
}

void main_body()
{
	leaf({
		.size = { LEAF_SIZE_GROW, LEAF_SIZE_GROW },
		.direction = LEAF_LAYOUT_HORIZONAL,
		.child_alignment = { LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER },
		.child_gap = 0.0f
	})
	{
		for(int i = 0; i < naui_list_len(g_icons); i++)
		{
			Icon* icon = &g_icons[i];
			if(icon->type == ICON_CIRCLE)
				circle_icon(icon->id);
			else
				square_icon(icon->id);
		}

	}
}

void naui_app_update(void)
{
	leaf({
		.size = { LEAF_SIZE_GROW, LEAF_SIZE_GROW },
		.direction = LEAF_LAYOUT_VERTICAL,
		.color = leaf_rgb(18, 18, 22)
	})
	{
		title_bar("Naui Demo");
		main_body();
	}
}
