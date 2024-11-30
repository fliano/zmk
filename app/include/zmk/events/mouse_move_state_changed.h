/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zmk/hid.h>
#include <zmk/event_manager.h>
#include <zmk/mouse.h>

struct zmk_mouse_move_state_changed {
    int8_t d_x;
    int8_t d_y;
    bool state;
    int64_t timestamp;
};

ZMK_EVENT_DECLARE(zmk_mouse_move_state_changed);

static inline int raise_zmk_mouse_move_state_changed_from_encoded(uint16_t encoded, bool pressed,
                                                                  int64_t timestamp) {
    return raise_zmk_mouse_move_state_changed(
        (struct zmk_mouse_move_state_changed){.d_x = MOVE_HOR_DECODE(encoded),
                                              .d_y = MOVE_VERT_DECODE(encoded),
                                              .state = pressed,
                                              .timestamp = timestamp});
}
