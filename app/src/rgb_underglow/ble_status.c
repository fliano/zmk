#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>

#include <math.h>
#include <stdlib.h>

#include <zephyr/logging/log.h>

#include <zmk/rgb_underglow/startup_mutex.h>
#include <zmk/rgb_underglow/ble_status.h>
#include <zmk/rgb_underglow/current_status.h>
#include <zmk/rgb_underglow/rgb_underglow_base.h>

#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>

#include <zmk/usb.h>
#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zmk/workqueue.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static struct output_state zmk_get_output_state(const zmk_event_t *_eh) {
    return (struct output_state){.selected_endpoint = zmk_endpoints_selected(),
                                 .active_profile_connected = zmk_ble_active_profile_is_connected(),
                                 .active_profile_bonded = !zmk_ble_active_profile_is_open()};
    ;
}

int zmk_rgb_underglow_set_color_ble(struct output_state os) {
    if (os.selected_endpoint.transport == ZMK_TRANSPORT_BLE) {
        if (os.active_profile_bonded) {
            if (os.active_profile_connected) {
                struct zmk_led_hsb color = {h : 300, s : 100, b : 30};
                return zmk_rgb_ug_set_hsb(color);
            }
            struct zmk_led_hsb color = {h : 180, s : 100, b : 30};
            return zmk_rgb_ug_set_hsb(color);
        }
        struct zmk_led_hsb color = {h : 240, s : 100, b : 30};

        return zmk_rgb_ug_set_hsb(color);
    }
    return 0;
}

static void rgb_underglow_ble_status_timeout_work(struct k_work *work) {
    zmk_rgb_underglow_apply_current_state();
}

K_WORK_DEFINE(underglow_ble_timeout_work, rgb_underglow_ble_status_timeout_work);

static void rgb_underglow_ble_status_timeout_timer(struct k_timer *timer) {
    k_work_submit_to_queue(zmk_workqueue_lowprio_work_q(), &underglow_ble_timeout_work);
}

K_TIMER_DEFINE(underglow_ble_timeout_timer, rgb_underglow_ble_status_timeout_timer, NULL);

static int rgb_underglow_ble_state_event_listener(const zmk_event_t *eh) {
    const struct output_state sc = zmk_get_output_state(eh);

    if (sc.selected_endpoint.transport == ZMK_TRANSPORT_USB)
        return 0;

    if (is_starting_up())
        return 0;

    if (sc.active_profile_connected)
        k_timer_start(&underglow_ble_timeout_timer, K_SECONDS(2), K_NO_WAIT);

    struct zmk_led_hsb color = {h : sc.selected_endpoint.ble.profile_index * 60, s : 100, b : 30};
    zmk_rgb_ug_set_hsb(color);
    return zmk_rgb_ug_select_effect(UNDERGLOW_EFFECT_BREATHE) | zmk_rgb_ug_set_hsb(color);
}

ZMK_LISTENER(rgb_ble, rgb_underglow_ble_state_event_listener);
ZMK_SUBSCRIPTION(rgb_ble, zmk_endpoint_changed);

#if defined(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(rgb_ble, zmk_ble_active_profile_changed);
#endif
