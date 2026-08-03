// TODO(doomguy): rewrite the slop into human slop

NAUI_APP("Naui Sandbox")

void naui_app_start(void)
{
	naui_load_theme("Default");
	naui_load_font(0, "MYRIADPRO-REGULAR");
	naui_set_main_viewport(NAUI_ATTACH_PANEL(welcome));

    const int six_seven = 67;
    naui_log(NAUI_LOG_DEBUG, "secret debug message :) guess what... %d", six_seven);
    naui_log(NAUI_LOG_INFO, "hello everybody my name is markiplier");
    naui_log(NAUI_LOG_WARNING, "im thinking miku");
    naui_log(NAUI_LOG_ERROR, "teto teto teeeto");

    Naui_String sb = naui_sb_create();
    naui_sb_append_string(sb, naui_string("urmom"), naui_string(" so "), naui_string("fat"));
    naui_log(NAUI_LOG_INFO, naui_string_fmt, naui_string_spread(naui_sb_to_string(sb)));
    naui_sb_destroy(sb);

    // this aborts the program...
    // naui_log(NAUI_LOG_FUCKED, "hello everybody my name is markiplier");
}

void naui_app_end(void)
{

}

void naui_app_update(void)
{
	naui_widgets_reset();

	naui_render_main_titlebar("Naui Editor");
	naui_render_panels_and_viewport();
}
