#pragma once

// input.h

#include <stdbool.h>
#include <stdint.h>

struct input_state {
    bool mouse_buttons[8];
    int mouse_x;
    int mouse_y;
    int mouse_wheel;
};

void input_init(struct input_state *state);

bool input_key_down(struct input_state const *state, int key);

bool input_key_up(struct input_state const *state, int key);

bool input_key_pressed(struct input_state const *state, int key);

bool input_key_released(struct input_state const *state, int key);

bool input_mouse_button_down(struct input_state const *state, uint8_t button);
