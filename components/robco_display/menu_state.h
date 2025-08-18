
#pragma once
#include <string>
#include <vector>
#include <cstdint>


struct MenuEntry {
    std::string title;
    enum class Type { STATIC, SUBMENU, ACTION, STATUS, LOGS } type;
    std::vector<MenuEntry> subitems;
    std::vector<std::string> logs;
    std::string status_value;
};


class MenuState {
public:
    MenuState();
    void set_boot_messages(const std::vector<std::string>& messages);
    void set_menu(const std::vector<MenuEntry>& menu);
    void on_key_press(uint8_t keycode);
    const std::vector<std::string>& get_boot_messages() const;
    bool is_boot_complete() const;
    const MenuEntry* get_current_menu() const;
    int get_selected_index() const;
    std::string get_display_text() const;
    void add_log(const std::string& entry);
    void remove_log(int index);
    const std::vector<std::string>& get_logs() const;
    void set_status(const std::string& value);
    std::string get_status() const;
private:
    std::vector<std::string> boot_messages_;
    bool boot_complete_ = false;
    std::vector<MenuEntry> menu_;
    int selected_index_ = 0;
    int current_menu_level_ = 0;
    std::vector<int> menu_stack_;
    std::vector<std::string> logs_;
    std::string status_value_;
};
