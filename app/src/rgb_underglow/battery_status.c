#include <zmk/rgb_underglow/rgb_underglow_base.h>
#include <zmk/usb.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/workqueue.h>

static void rgb_underglow_status_timeout_work(struct k_work *work) {
    struct zmk_led_hsb color = {h : 240, s : 100, b : 100};

#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_USB)
    if (zmk_usb_is_powered()) {
        zmk_rgb_ug_set_hsb(color);
    } else {
        zmk_rgb_ug_off();
    }
    return;
#endif

    zmk_rgb_ug_set_hsb(color);
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
        ret = zmk_rgb_ug_set_hsb(color);
    } else if (sc->state_of_charge < CONFIG_ZMK_RGB_UNDERGLOW_BATTERY_LOW_THRESHOLD) {
        struct zmk_led_hsb color = {h : 60, s : 100, b : 30};
        ret = zmk_rgb_ug_set_hsb(color);
    } else {
        struct zmk_led_hsb color = {h : 120, s : 100, b : 30};
        ret = zmk_rgb_ug_set_hsb(color);
    }

    k_timer_start(&underglow_timeout_timer, K_SECONDS(1), K_NO_WAIT);

    return ret;
}

ZMK_LISTENER(rgb_battery, rgb_underglow_battery_state_event_listener);
ZMK_SUBSCRIPTION(rgb_battery, zmk_battery_state_changed);
