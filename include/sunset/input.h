#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "internal/utils.h"
#include "sunset/bitmask.h"
#include "sunset/geometry.h"
#include "sunset/rman.h"

typedef struct EngineContext EngineContext;

typedef enum MouseButton {
    MOUSE_BUTTON_LEFT = sunset_flag(0),
    MOUSE_BUTTON_RIGHT = sunset_flag(1),
    MOUSE_BUTTON_MIDDLE = sunset_flag(2),
    MOUSE_BUTTON_UNKNOWN = sunset_flag(3),
    NUM_MOUSE_BUTTONS = 4,
} MouseButton;

typedef struct MouseMoveEvent {
    Point offset;
    Point absolute;
    Bitmask mouse_buttons;
} MouseMoveEvent;

typedef struct MouseClickEvent {
    MouseButton button;
} MouseClickEvent;

typedef enum Focus {
    FOCUS_NULL,
    FOCUS_UI,
    FOCUS_MAIN,
} Focus;

extern DECLARE_RESOURCE_ID(input_focus);

void input_setup(EngineContext *engine_context);
