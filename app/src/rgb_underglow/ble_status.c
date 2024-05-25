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
#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/workqueue.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct output_status_state {
    struct zmk_endpoint_instance selected_endpoint;
    bool active_profile_connected;
    bool active_profile_bonded;
};

static struct output_status_state get_state(const zmk_event_t *_eh) {
    return (struct output_status_state){.selected_endpoint = zmk_endpoints_selected(),
                                        .active_profile_connected =
                                            zmk_ble_active_profile_is_connected(),
                                        .active_profile_bonded = !zmk_ble_active_profile_is_open()};
    ;
}

static void rgb_underglow_status_timeout_work(struct k_work *work) {
    zmk_rgb_underglow_apply_current_state();
}

K_WORK_DEFINE(underglow_timeout_work, rgb_underglow_status_timeout_work);

static void rgb_underglow_status_timeout_timer(struct k_timer *timer) {
    k_work_submit_to_queue(zmk_workqueue_lowprio_work_q(), &underglow_timeout_work);
}

K_TIMER_DEFINE(underglow_timeout_timer, rgb_underglow_status_timeout_timer, NULL);

static int rgb_underglow_ble_state_event_listener(const zmk_event_t *eh) {
    const struct output_status_state sc = get_state(eh);

    if (sc.selected_endpoint.transport == ZMK_TRANSPORT_USB)
        return 0;

    if (sc.active_profile_connected)
        k_timer_start(&underglow_timeout_timer, K_SECONDS(2), K_NO_WAIT);

    struct zmk_led_hsb color = {h : sc.selected_endpoint.ble.profile_index * 60, s : 100, b : 30};
    zmk_rgb_ug_set_hsb(color);
    return zmk_rgb_ug_select_effect(UNDERGLOW_EFFECT_BREATHE) | zmk_rgb_ug_set_hsb(color);
}

ZMK_LISTENER(rgb_ble, rgb_underglow_ble_state_event_listener);
ZMK_SUBSCRIPTION(rgb_ble, zmk_endpoint_changed);
ZMK_SUBSCRIPTION(rgb_ble, zmk_ble_active_profile_changed);
