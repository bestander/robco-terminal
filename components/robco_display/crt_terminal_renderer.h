#ifndef CRT_TERMINAL_RENDERER_H
#define CRT_TERMINAL_RENDERER_H

#include <string>
#include <vector>
extern "C"
{
#include "lvgl.h"
}


namespace esphome
{
    namespace robco_display
    {

        class CRTTerminalRenderer
        {
        public:
            CRTTerminalRenderer();
            void init();
            void render_line(const std::string &line, size_t index, bool is_menu);
            void lock();
            void unlock();
            size_t get_num_lines() const {
                constexpr size_t display_height = 480;
                constexpr size_t char_height = 24;
                return display_height / char_height;
            }
        private:
            std::vector<lv_obj_t *> line_labels;
            lv_style_t label_style;
            bool screen_bg_set_ = false;

        };
    } // namespace robco_display
} // namespace esphome
#endif // CRT_TERMINAL_RENDERER_H
