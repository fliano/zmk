#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>

#include <math.h>
#include <stdlib.h>

#include <zephyr/logging/log.h>

#include <zmk/rgb_underglow/current_status.h>
#include <zmk/rgb_underglow/rgb_underglow_base.h>
#include <zmk/usb.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/workqueue.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static void rgb_underglow_status_timeout_work(struct k_work *work) {
    zmk_rgb_underglow_apply_current_state();
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

    k_timer_start(&underglow_timeout_timer, K_SECONDS(1), K_NO_WAIT);

    if (sc->state_of_charge < CONFIG_ZMK_RGB_UNDERGLOW_BATTERY_CRITICALLY_LOW_THRESHOLD) {
        struct zmk_led_hsb color = {h : 0, s : 100, b : 30};
        return zmk_rgb_ug_set_hsb(color);
    } else if (sc->state_of_charge < CONFIG_ZMK_RGB_UNDERGLOW_BATTERY_LOW_THRESHOLD) {
        struct zmk_led_hsb color = {h : 60, s : 100, b : 30};
        return zmk_rgb_ug_set_hsb(color);
    } else {
        struct zmk_led_hsb color = {h : 120, s : 100, b : 30};
        return zmk_rgb_ug_set_hsb(color);
    }
}

ZMK_LISTENER(rgb_battery, rgb_underglow_battery_state_event_listener);
ZMK_SUBSCRIPTION(rgb_battery, zmk_battery_state_changed);
