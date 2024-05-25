/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>

#include <math.h>
#include <stdlib.h>

#include <zephyr/logging/log.h>

#include <zephyr/drivers/led_strip.h>
#include <drivers/ext_power.h>

#include <zmk/rgb_underglow/rgb_underglow_base.h>

#include <zmk/activity.h>
#include <zmk/usb.h>
#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/workqueue.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if !DT_HAS_CHOSEN(zmk_underglow)

#error "A zmk,underglow chosen node must be declared"

#endif

#define STRIP_CHOSEN DT_CHOSEN(zmk_underglow)
#define STRIP_NUM_PIXELS DT_PROP(STRIP_CHOSEN, chain_length)

#define HUE_MAX 360
#define SAT_MAX 100
#define BRT_MAX 100

BUILD_ASSERT(CONFIG_ZMK_RGB_UNDERGLOW_BRT_MIN <= CONFIG_ZMK_RGB_UNDERGLOW_BRT_MAX,
             "ERROR: RGB underglow maximum brightness is less than minimum brightness");

enum rgb_underglow_effect {
    UNDERGLOW_EFFECT_SOLID,
    UNDERGLOW_EFFECT_BREATHE,
    UNDERGLOW_EFFECT_SPECTRUM,
    UNDERGLOW_EFFECT_SWIRL,
    UNDERGLOW_EFFECT_NUMBER // Used to track number of underglow effects
};

struct rgb_underglow_state {
    struct zmk_led_hsb color;
    uint8_t animation_speed;
    uint8_t current_effect;
    bool on;
};

static const struct device *led_strip;

static struct led_rgb pixels[STRIP_NUM_PIXELS];

static struct rgb_underglow_state state = default_rgb_settings;

#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_EXT_POWER)
static const struct device *const ext_power = DEVICE_DT_GET(DT_INST(0, zmk_ext_power_generic));
#endif

static struct zmk_led_hsb hsb_scale_min_max(struct zmk_led_hsb hsb) {
    hsb.b = CONFIG_ZMK_RGB_UNDERGLOW_BRT_MIN +
            (CONFIG_ZMK_RGB_UNDERGLOW_BRT_MAX - CONFIG_ZMK_RGB_UNDERGLOW_BRT_MIN) * hsb.b / BRT_MAX;
    return hsb;
}

static struct zmk_led_hsb hsb_scale_zero_max(struct zmk_led_hsb hsb) {
    hsb.b = hsb.b * CONFIG_ZMK_RGB_UNDERGLOW_BRT_MAX / BRT_MAX;
    return hsb;
}

static struct led_rgb hsb_to_rgb(struct zmk_led_hsb hsb) {
    float r, g, b;

    uint8_t i = hsb.h / 60;
    float v = hsb.b / ((float)BRT_MAX);
    float s = hsb.s / ((float)SAT_MAX);
    float f = hsb.h / ((float)HUE_MAX) * 6 - i;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);

    switch (i % 6) {
    case 0:
        r = v;
        g = t;
        b = p;
        break;
    case 1:
        r = q;
        g = v;
        b = p;
        break;
    case 2:
        r = p;
        g = v;
        b = t;
        break;
    case 3:
        r = p;
        g = q;
        b = v;
        break;
    case 4:
        r = t;
        g = p;
        b = v;
        break;
    case 5:
        r = v;
        g = p;
        b = q;
        break;
    }

    struct led_rgb rgb = {r : r * 255, g : g * 255, b : b * 255};

    return rgb;
}

int zmk_rgb_underglow_get_state(bool *on_off) {
    *on_off = state.on;
    return 0;
}

int zmk_rgb_underglow_on(void) {
    state.on = true;
    return zmk_rgb_ug_on();
}

int zmk_rgb_underglow_off(void) {
    state.on = false;
    return zmk_rgb_ug_off();
}

int zmk_rgb_underglow_calc_effect(int direction) {
    return (state.current_effect + UNDERGLOW_EFFECT_NUMBER + direction) % UNDERGLOW_EFFECT_NUMBER;
}

int zmk_rgb_underglow_select_effect(int effect) {
    state.current_effect = effect;
    return zmk_rgb_ug_select_effect(effect);
}

int zmk_rgb_underglow_cycle_effect(int direction) {
    return zmk_rgb_underglow_select_effect(zmk_rgb_underglow_calc_effect(direction));
}

int zmk_rgb_underglow_toggle(void) {
    return state.on ? zmk_rgb_underglow_off() : zmk_rgb_underglow_on();
}

int zmk_rgb_underglow_set_hsb(struct zmk_led_hsb color) {
    state.color = color;
    return zmk_rgb_ug_set_hsb(color);
}

struct zmk_led_hsb zmk_rgb_underglow_calc_hue(int direction) {
    struct zmk_led_hsb color = state.color;

    color.h += HUE_MAX + (direction * CONFIG_ZMK_RGB_UNDERGLOW_HUE_STEP);
    color.h %= HUE_MAX;

    return color;
}

struct zmk_led_hsb zmk_rgb_underglow_calc_sat(int direction) {
    struct zmk_led_hsb color = state.color;

    int s = color.s + (direction * CONFIG_ZMK_RGB_UNDERGLOW_SAT_STEP);
    if (s < 0) {
        s = 0;
    } else if (s > SAT_MAX) {
        s = SAT_MAX;
    }
    color.s = s;

    return color;
}

struct zmk_led_hsb zmk_rgb_underglow_calc_brt(int direction) {
    struct zmk_led_hsb color = state.color;

    int b = color.b + (direction * CONFIG_ZMK_RGB_UNDERGLOW_BRT_STEP);
    color.b = CLAMP(b, 0, BRT_MAX);

    return color;
}

int zmk_rgb_underglow_change_hue(int direction) {
    state.color = zmk_rgb_underglow_calc_hue(direction);
    return zmk_rgb_ug_set_hsb(state.color);
}

int zmk_rgb_underglow_change_sat(int direction) {
    state.color = zmk_rgb_underglow_calc_sat(direction);
    return zmk_rgb_ug_set_hsb(state.color);
}

int zmk_rgb_underglow_change_brt(int direction) {
    state.color = zmk_rgb_underglow_calc_brt(direction);
    return zmk_rgb_ug_set_hsb(state.color);
}

int zmk_rgb_underglow_change_spd(int direction) {
    int speed = state.animation_speed + direction;
    speed = CLAMP(speed, 1, 5);
    state.animation_speed = speed;
    return zmk_rgb_ug_set_spd(speed);
}

#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_IDLE) ||                                          \
    IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_USB)
static int rgb_underglow_auto_state(bool new_state) {
    if (state.on == new_state) {
        return 0;
    }
    if (new_state) {
        return zmk_rgb_underglow_on();
    } else {
        return zmk_rgb_underglow_off();
    }
}

static int rgb_underglow_event_listener(const zmk_event_t *eh) {

#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_IDLE)
    if (as_zmk_activity_state_changed(eh)) {
        return rgb_underglow_auto_state(zmk_activity_get_state() == ZMK_ACTIVITY_ACTIVE);
    }
#endif

#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_USB)
    if (as_zmk_usb_conn_state_changed(eh)) {
        return rgb_underglow_auto_state(zmk_usb_is_powered());
    }
#endif

    return -ENOTSUP;
}

ZMK_LISTENER(rgb_underglow, rgb_underglow_event_listener);
#endif // IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_IDLE) ||
       // IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_USB)

#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_IDLE)
ZMK_SUBSCRIPTION(rgb_underglow, zmk_activity_state_changed);
#endif

#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_USB)
ZMK_SUBSCRIPTION(rgb_underglow, zmk_usb_conn_state_changed);
#endif

#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_BATTERY_STATUS)
static void rgb_underglow_status_timeout_work(struct k_work *work) {
    struct zmk_led_hsb color = {h : 240, s : 100, b : 100};

#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_USB)
    if (zmk_usb_is_powered()) {
        zmk_rgb_underglow_set_hsb(color);
    } else {
        zmk_rgb_underglow_off();
    }
    return;
#endif

    zmk_rgb_underglow_set_hsb(color);
}

K_WORK_DEFINE(underglow_timeout_work, rgb_underglow_status_timeout_work);

static void rgb_underglow_status_timeout_timer(struct k_timer *timer) {
    k_work_submit_to_queue(zmk_workqueue_lowprio_work_q(), &underglow_timeout_work);
}

K_TIMER_DEFINE(underglow_timeout_timer, rgb_underglow_status_timeout_timer, NULL);

static int rgb_underglow_battery_state_event_listener(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *sc = as_zmk_battery_state_changed(eh);
    if (!sc) {
        LOG_ERR("underglow battery state listener called with unsupported argument");
        return -ENOTSUP;
    }

    int ret = -ENOTSUP;
    if (sc->state_of_charge < CONFIG_ZMK_RGB_UNDERGLOW_BATTERY_CRITICALLY_LOW_THRESHOLD) {
        struct zmk_led_hsb color = {h : 0, s : 100, b : 30};
        ret = zmk_rgb_underglow_set_hsb(color);
    } else if (sc->state_of_charge < CONFIG_ZMK_RGB_UNDERGLOW_BATTERY_LOW_THRESHOLD) {
        struct zmk_led_hsb color = {h : 60, s : 100, b : 30};
        ret = zmk_rgb_underglow_set_hsb(color);
    } else {
        struct zmk_led_hsb color = {h : 120, s : 100, b : 30};
        ret = zmk_rgb_underglow_set_hsb(color);
    }

    k_timer_start(&underglow_timeout_timer, K_SECONDS(1), K_NO_WAIT);

    return ret;
}

ZMK_LISTENER(rgb_battery, rgb_underglow_battery_state_event_listener);
ZMK_SUBSCRIPTION(rgb_battery, zmk_battery_state_changed);
#endif
