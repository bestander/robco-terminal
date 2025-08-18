#ifndef CRT_TERMINAL_RENDERER_H
#define CRT_TERMINAL_RENDERER_H

#include <string>
#include <vector>

namespace esphome
{
    namespace robco_display
    {

        class CRTTerminalRenderer
        {
        public:
            CRTTerminalRenderer();
            void render(const std::vector<std::string> &lines, bool is_menu = false);
            void init();
        };
    } // namespace robco_display
} // namespace esphome
#endif // CRT_TERMINAL_RENDERER_H
