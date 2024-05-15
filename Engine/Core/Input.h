#pragma once
#include "Core/KeyCodes.h"
#include "Core/MouseCodes.h"

// TODO: Design a efficient input system
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

    static float GetMouseX();
    static float GetMouseY();

public:
    static MousePosition GetMousePosition();

    static bool first_mouse;
    static MousePosition last_position;

};