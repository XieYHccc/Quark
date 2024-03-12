#pragma once

#include "Foundation/KeyCodes.h"
#include "Foundation/MouseCodes.h"

struct MousePosition 
{
    float x_pos;
    float y_pos;
};

class Input {
public:
    static bool IsKeyPressed(Keycode key);
    static bool IsKeyReleased(Keycode key);
    static bool IsMousePressed(MouseCode button);

    static MousePosition GetMousePosition();
    static float GetMouseX();
    static float GetMouseY();

public:
    static bool first_mouse_;
    static MousePosition last_position_;

};