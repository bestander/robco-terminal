#include "crt_terminal_renderer.h"
extern "C"
{
#include "lvgl.h"
}

namespace esphome
{
    namespace robco_display
    {
        CRTTerminalRenderer::CRTTerminalRenderer() {}

        void CRTTerminalRenderer::render(const std::vector<std::string> &lines, bool is_menu)
        {
            // Clear screen to black
            lv_obj_t *scr = lv_scr_act();
            lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
            lv_obj_clean(scr);
            // Set up style for bright green text, larger font
            static lv_style_t style;
            lv_style_init(&style);
            lv_style_set_text_color(&style, lv_color_make(0, 255, 0));
            lv_style_set_text_font(&style, &lv_font_montserrat_14);
            int left_margin = is_menu ? 50 : 10;
            int top_margin = is_menu ? 80 : 0;
            int y = top_margin;
            for (const auto &line : lines)
            {
                lv_obj_t *label = lv_label_create(scr);
                lv_obj_add_style(label, &style, 0);
                lv_label_set_text(label, line.c_str());
                lv_obj_set_pos(label, left_margin, y);
                y += 22;
            }
           
        }

    } // namespace robco_display
} // namespace esphome