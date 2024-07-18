#pragma once
#include <cstdint>

using Keycode = uint16_t;
using MouseCode = uint16_t;

enum : Keycode {
    // From glfw3.h
    KEY_CODE_SPACE = 32,
    KEY_CODE_APOSTROPHE = 39, /* ' */
    KEY_CODE_COMMA = 44,      /* , */
    KEY_CODE_MINUS = 45,      /* - */
    KEY_CODE_PERIOD = 46,     /* . */
    KEY_CODE_SLASH = 47,      /* / */

    KEY_CODE_D0 = 48, /* 0 */
    KEY_CODE_D1 = 49, /* 1 */
    KEY_CODE_D2 = 50, /* 2 */
    KEY_CODE_D3 = 51, /* 3 */
    KEY_CODE_D4 = 52, /* 4 */
    KEY_CODE_D5 = 53, /* 5 */
    KEY_CODE_D6 = 54, /* 6 */
    KEY_CODE_D7 = 55, /* 7 */
    KEY_CODE_D8 = 56, /* 8 */
    KEY_CODE_D9 = 57, /* 9 */

    KEY_CODE_SEMICOLON = 59, /* ; */
    KEY_CODE_EQUAL = 61,     /* = */

    KEY_CODE_A = 65,
    KEY_CODE_B = 66,
    KEY_CODE_C = 67,
    KEY_CODE_D = 68,
    KEY_CODE_E = 69,
    KEY_CODE_F = 70,
    KEY_CODE_G = 71,
    KEY_CODE_H = 72,
    KEY_CODE_I = 73,
    KEY_CODE_J = 74,
    KEY_CODE_K = 75,
    KEY_CODE_L = 76,
    KEY_CODE_M = 77,
    KEY_CODE_N = 78,
    KEY_CODE_O = 79,
    KEY_CODE_P = 80,
    KEY_CODE_Q = 81,
    KEY_CODE_R = 82,
    KEY_CODE_S = 83,
    KEY_CODE_T = 84,
    KEY_CODE_U = 85,
    KEY_CODE_V = 86,
    KEY_CODE_W = 87,
    KEY_CODE_X = 88,
    KEY_CODE_Y = 89,
    KEY_CODE_Z = 90,

    KEY_CODE_LEFT_BRACKET = 91,  /* [ */
    KEY_CODE_BACKSLASH = 92,    /* \ */
    KEY_CODE_RIGHT_BRACKET = 93, /* ] */
    KEY_CODE_GRAVE_ACCENT = 96,  /* ` */

    KEY_CODE_WORLD1 = 161, /* non-US #1 */
    KEY_CODE_WORLD2 = 162, /* non-US #2 */

    /* Function keys */
    KEY_CODE_ESCAPE = 256,
    KEY_CODE_ENTER = 257,
    KEY_CODE_TAB = 258,
    KEY_CODE_BACKSPACE = 259,
    KEY_CODE_INSERT = 260,
    KEY_CODE_DELETE = 261,
    KEY_CODE_RIGHT = 262,
    KEY_CODE_LEFT = 263,
    KEY_CODE_DOWN = 264,
    KEY_CODE_UP = 265,
    KEY_CODE_PAGEUP = 266,
    KEY_CODE_PAGEDOWN = 267,
    KEY_CODE_HOME = 268,
    KEY_CODE_END = 269,
    KEY_CODE_CAPSLOCK = 280,
    KEY_CODE_SCROLLLOCK = 281,
    KEY_CODE_NUMLOCK = 282,
    KEY_CODE_PRINTSCREEN = 283,
    KEY_CODE_PAUSE = 284,
    KEY_CODE_F1 = 290,
    KEY_CODE_F2 = 291,
    KEY_CODE_F3 = 292,
    KEY_CODE_F4 = 293,
    KEY_CODE_F5 = 294,
    KEY_CODE_F6 = 295,
    KEY_CODE_F7 = 296,
    KEY_CODE_F8 = 297,
    KEY_CODE_F9 = 298,
    KEY_CODE_F10 = 299,
    KEY_CODE_F11 = 300,
    KEY_CODE_F12 = 301,
    KEY_CODE_F13 = 302,
    KEY_CODE_F14 = 303,
    KEY_CODE_F15 = 304,
    KEY_CODE_F16 = 305,
    KEY_CODE_F17 = 306,
    KEY_CODE_F18 = 307,
    KEY_CODE_F19 = 308,
    KEY_CODE_F20 = 309,
    KEY_CODE_F21 = 310,
    KEY_CODE_F22 = 311,
    KEY_CODE_F23 = 312,
    KEY_CODE_F24 = 313,
    KEY_CODE_F25 = 314,

    /* Keypad */
    KEY_CODE_KP0 = 320,
    KEY_CODE_KP1 = 321,
    KEY_CODE_KP2 = 322,
    KEY_CODE_KP3 = 323,
    KEY_CODE_KP4 = 324,
    KEY_CODE_KP5 = 325,
    KEY_CODE_KP6 = 326,
    KEY_CODE_KP7 = 327,
    KEY_CODE_KP8 = 328,
    KEY_CODE_KP9 = 329,
    KEY_CODE_KPDECIMAL = 330,
    KEY_CODE_KPDIVIDE = 331,
    KEY_CODE_KPMULTIPLY = 332,
    KEY_CODE_KPSUBTRACT = 333,
    KEY_CODE_KPADD = 334,
    KEY_CODE_KPENTER = 335,
    KEY_CODE_KPEQUAL = 336,

    KEY_CODE_LEFTSHIFT = 340,
    KEY_CODE_LEFTCONTROL = 341,
    KEY_CODE_LEFTALT = 342,
    KEY_CODE_LEFTSUPER = 343,
    KEY_CODE_RIGHTSHIFT = 344,
    KEY_CODE_RIGHTCONTROL = 345,
    KEY_CODE_RIGHTALT = 346,
    KEY_CODE_RIGHTSUPER = 347,
    KEY_CODE_MENU = 348
};

enum : MouseCode {
    // From glfw3.h
    MOUSE_CODE_BUTTON0 = 0,
    MOUSE_CODE_BUTTON1 = 1,
    MOUSE_CODE_BUTTON2 = 2,
    MOUSE_CODE_BUTTON3 = 3,
    MOUSE_CODE_BUTTON4 = 4,
    MOUSE_CODE_BUTTON5 = 5,
    MOUSE_CODE_BUTTON6 = 6,
    MOUSE_CODE_BUTTON7 = 7,

    MOUSE_CODE_BUTTONLAST = MOUSE_CODE_BUTTON7,
    MOUSE_CODE_BUTTONLEFT = MOUSE_CODE_BUTTON0,
    MOUSE_CODE_BUTTONRIGHT = MOUSE_CODE_BUTTON1,
    MOUSE_CODE_BUTTONMIDDLE = MOUSE_CODE_BUTTON2
};