#include "menu_state.h"
#include <algorithm>

MenuState::MenuState() {}

void MenuState::set_boot_messages(const std::vector<std::string>& messages) {
    boot_messages_ = messages;
}

void MenuState::set_menu(const std::vector<MenuEntry>& menu) {
    menu_ = menu;
}

void MenuState::on_key_press(uint8_t keycode) {
    if (!boot_complete_) {
        boot_complete_ = true;
        return;
    }
    std::vector<MenuEntry>* current_menu = &menu_;
    for (size_t i = 0; i < menu_stack_.size(); ++i) {
        int idx = menu_stack_[i];
        if (idx >= 0 && idx < current_menu->size()) {
            current_menu = &(*current_menu)[idx].subitems;
        }
    }
    int menu_size = current_menu->size();
    if (menu_size == 0) return;
    if (keycode == 0x52) { // Up
        selected_index_ = (selected_index_ - 1 + menu_size) % menu_size;
    } else if (keycode == 0x51) { // Down
        selected_index_ = (selected_index_ + 1) % menu_size;
    } else if (keycode == 0x28) { // Enter
        if ((*current_menu)[selected_index_].type == MenuEntry::Type::SUBMENU) {
            menu_stack_.push_back(selected_index_);
            selected_index_ = 0;
        }
    } else if (keycode == 0x29) { // Escape
        if (!menu_stack_.empty()) {
            selected_index_ = menu_stack_.back();
            menu_stack_.pop_back();
        }
    }
}

const std::vector<std::string>& MenuState::get_boot_messages() const {
    return boot_messages_;
}

bool MenuState::is_boot_complete() const {
    return boot_complete_;
}

const MenuEntry* MenuState::get_current_menu() const {
    if (menu_stack_.empty()) return nullptr;
    return &menu_[menu_stack_.back()];
}

int MenuState::get_selected_index() const {
    return selected_index_;
}

std::string MenuState::get_display_text() const {
    if (!boot_complete_) {
        std::string text;
        for (const auto& line : boot_messages_) text += line + "\n";
        return text;
    }
    // Find current menu
    const std::vector<MenuEntry>* current_menu = &menu_;
    for (size_t i = 0; i < menu_stack_.size(); ++i) {
        int idx = menu_stack_[i];
        if (idx >= 0 && idx < current_menu->size()) {
            current_menu = &(*current_menu)[idx].subitems;
        }
    }
    std::string text = "MENU:\n";
    for (size_t i = 0; i < current_menu->size(); ++i) {
        text += (i == selected_index_ ? "> " : "  ") + (*current_menu)[i].title + "\n";
    }
    return text;
}

void MenuState::add_log(const std::string& entry) {
    logs_.push_back(entry);
}

void MenuState::remove_log(int index) {
    if (index >= 0 && index < logs_.size()) logs_.erase(logs_.begin() + index);
}

const std::vector<std::string>& MenuState::get_logs() const {
    return logs_;
}

void MenuState::set_status(const std::string& value) {
    status_value_ = value;
}

std::string MenuState::get_status() const {
    return status_value_;
}
