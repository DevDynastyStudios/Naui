#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <leaf/leaf.h>

typedef uint8_t Naui_WidgetType;
enum
{
	NAUI_WIDGET_TEXT,
	NAUI_WIDGET_BUTTON,
	NAUI_WIDGET_TOGGLE,
	NAUI_WIDGET_DROPDOWN,
	NAUI_WIDGET_SLIDER_INT,
	NAUI_WIDGET_SLIDER_FLOAT,
	NAUI_WIDGET_DRAG_INT,
	NAUI_WIDGET_DRAG_FLOAT,
	NAUI_WIDGET_TEXT_FIELD,
	NAUI_WIDGET_KNOB
};

typedef void (*Naui_TextRenderer)(const char* text);
typedef void (*Naui_ButtonRenderer)(Leaf_ID id, const char* label);
typedef void (*Naui_ToggleRenderer)(Leaf_ID id, const char* label, bool* value);
typedef void (*Naui_DropDownRenderer)(Leaf_ID id, const char* label, uint32_t* select, const char** items, size_t item_count);
typedef void (*Naui_SliderIntRenderer)(Leaf_ID id, const char* label, int32_t* value, int32_t min, int32_t max);
typedef void (*Naui_SliderFloatRenderer)(Leaf_ID id, const char* label, float* value, float min, float max, const char* format);

typedef void (*Naui_DragIntRenderer)(Leaf_ID id, const char* label, int* value, int speed, int min, int max);
typedef void (*Naui_DragFloatRenderer)(Leaf_ID id, const char* label, float* value, float speed, float min, float max, const char* format);
typedef void (*Naui_TextFieldRenderer)(Leaf_ID id, const char* label, const char* hint, char* buffer, size_t buffer_size);
typedef void (*Naui_KnobRenderer)(Leaf_ID id, const char* label, float* value, float min, float max, const char* format);

void naui_widget_init();
void naui_widget_reset();
void naui_widget_render_set(Naui_WidgetType widget_type, void* data); // data should be an actual leaf type. Passing NULL should use the default renderer