#pragma once

#include <unordered_map>
#include "glm/glm.hpp"


class InputMngr {
public:
    static InputMngr& Instance() {
        static InputMngr instance;
        return instance;
    }

public:
    void Initialize();

    void Update();

    void RecordKey(unsigned short key_code, unsigned short key_action) { key_event_map_[key_code] = key_action; }
    void RecordScroll(short mouse_scroll) { mouse_scroll_ += mouse_scroll; }
    void RecordMousePosition(float x, float y) { mouse_position_.x = x; mouse_position_.y = y; }

    bool GetKeyDown(unsigned short key_code);
    bool GetKeyUp(unsigned short key_code);
    bool GetMouseButtonDown(unsigned short mouse_button_index);
    bool GetMouseButtonUp(unsigned short mouse_button_index);
    short GetMouseScroll() { return mouse_scroll_; }
    glm::uvec2 GetMousePosition() { return mouse_position_; }

    bool IsFirstMouse() { return first_mouse_; }
    void SetFirstMouse(bool b) { first_mouse_ = b; }

private:
    InputMngr() {}

private:
    std::unordered_map<unsigned short,unsigned short> key_event_map_; // 0:Up 1:Down 2:Keep Down
    glm::vec2 mouse_position_;
    short mouse_scroll_;
    bool first_mouse_;

};

