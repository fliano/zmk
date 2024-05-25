/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zmk/rgb_underglow/rgb_underglow_base.h>

int zmk_rgb_ug_cs_toggle(void);
int zmk_rgb_ug_cs_get_state(bool *state);
int zmk_rgb_ug_cs_on(void);
int zmk_rgb_ug_cs_off(void);
int zmk_rgb_ug_cs_cycle_effect(int direction);
int zmk_rgb_ug_cs_calc_effect(int direction);
int zmk_rgb_ug_cs_select_effect(int effect);
struct zmk_led_hsb zmk_rgb_ug_cs_calc_hue(int direction);
struct zmk_led_hsb zmk_rgb_ug_cs_calc_sat(int direction);
struct zmk_led_hsb zmk_rgb_ug_cs_calc_brt(int direction);
int zmk_rgb_ug_cs_change_hue(int direction);
int zmk_rgb_ug_cs_change_sat(int direction);
int zmk_rgb_ug_cs_change_brt(int direction);
int zmk_rgb_ug_cs_change_spd(int direction);
int zmk_rgb_ug_cs_set_hsb(struct zmk_led_hsb color);
