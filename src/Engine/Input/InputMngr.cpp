#include "./InputMngr.h"

#include <iostream>

#include "./KeyCode.h"


void InputMngr::Initialize() {
    mouse_position_ = glm::vec2(0.f);
    mouse_scroll_ = static_cast<short>(0);
    first_mouse_ = true;
}

bool InputMngr::GetKeyDown(unsigned short key_code) {
    if(key_event_map_.count(key_code)==0){
        return false;
    }
    return key_event_map_[key_code]!=KEY_ACTION_UP;
}

bool InputMngr::GetKeyUp(unsigned short key_code) {
    if(key_event_map_.count(key_code)==0){
        return false;
    }
    return key_event_map_[key_code]==KEY_ACTION_UP;
}


bool InputMngr::GetMouseButtonDown(unsigned short mouse_button_index) {
    return GetKeyDown(mouse_button_index);
}

bool InputMngr::GetMouseButtonUp(unsigned short mouse_button_index) {
    return GetKeyUp(mouse_button_index);
}

void InputMngr::Update() {
    for (auto iterator = key_event_map_.begin(); iterator != key_event_map_.end();) {
        if (iterator->second == KEY_ACTION_UP) {
            iterator = key_event_map_.erase(iterator);
        }
        else {
            ++iterator;
        }

        mouse_scroll_ = 0;
    }
}