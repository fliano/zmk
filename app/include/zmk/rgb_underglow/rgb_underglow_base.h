/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

struct zmk_led_hsb {
    uint16_t h;
    uint8_t s;
    uint8_t b;
};

int zmk_rgb_ug_get_state(bool *state);
int zmk_rgb_ug_on(void);
int zmk_rgb_ug_off(void);
int zmk_rgb_ug_select_effect(int effect);
int zmk_rgb_ug_set_spd(int direction);
int zmk_rgb_ug_set_hsb(struct zmk_led_hsb color);
