NAUI_PANEL(welcome)

static void panel_on_attach(void)
{
    Naui_PanelID self = naui_current_panel();
    naui_panel_set_title(self, "Welcome");
    naui_panel_set_size(self, (Naui_Vec2){ 256, 256 });
    naui_panel_enable_flags(self, NAUI_PANEL_FLAG_SERIALIZABLE);
}

static void panel_on_detach(void)
{
    
}

static void panel_on_update(void)
{
    leaf({
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
    })
    {
        leaf({
            .size = {LEAF_SIZE_FIXED(256), LEAF_SIZE_FIXED(256)},
            .color = LEAF_COLOR_WHITE,
            .rounding = {
                .value = fabsf(sinf(naui_time())) * 128.0f,
                .corners = LEAF_CORNER_ALL
            },
            .shadow = {
                .blur_radius = 24.0f,
                .offset = {0.0f, 12.0f},
                .color = leaf_rgba(0, 0, 0, 64)
            }
        });
    }
}