#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>

#include <math.h>
#include <stdlib.h>

#include <zephyr/logging/log.h>

#include <zmk/rgb_underglow/rgb_underglow_base.h>
#include <zmk/rgb_underglow/startup_mutex.h>
#include <zmk/rgb_underglow/current_status.h>
#include <zmk/rgb_underglow/battery_status.h>
#include <zmk/rgb_underglow/ble_status.h>
#include <zmk/rgb_underglow/status_on_startup.h>

#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>

#include <zmk/usb.h>
#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zmk/workqueue.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

enum STARTUP_STATE {
    BATTERY,
    CONNECTING,
    CONNECTED,
};
static enum zmk_activity_state last_activity_state = ZMK_ACTIVITY_SLEEP;
static int64_t last_checkpoint = 0;
static enum STARTUP_STATE startup_state = BATTERY;
static struct k_timer *running_timer;

static void zmk_on_startup_timer_tick_work(struct k_work *work) {
    LOG_WRN("work");
    uint8_t state_of_charge = zmk_battery_state_of_charge();
    struct output_state os = zmk_get_output_state();

    if (os.selected_endpoint.transport == ZMK_TRANSPORT_USB) {
        k_timer_stop(running_timer);
        zmk_rgb_underglow_apply_current_state();
        return;
    }

    int64_t uptime = k_uptime_get();
    if (last_checkpoint + 3000 < uptime && startup_state != CONNECTING) {
        switch (startup_state) {
        case BATTERY:
            startup_state = os.active_profile_connected ? CONNECTED : CONNECTING;
            last_checkpoint = uptime;
            break;
        case CONNECTED:
            k_timer_stop(running_timer); // probably won't work
            zmk_rgb_underglow_apply_current_state();
            return;
        }
    }

    if (startup_state == CONNECTING && os.active_profile_connected) {
        startup_state = CONNECTED;
        last_checkpoint = uptime;
    }

    switch (startup_state) {
    case BATTERY:
        rgb_underglow_set_color_battery(state_of_charge);
        return;
    case CONNECTING:
    case CONNECTED:
        zmk_rgb_underglow_set_color_ble(os);
        return;
    }
}

K_WORK_DEFINE(on_startup_timer_tick_work, zmk_on_startup_timer_tick_work);

static void on_startup_timer_tick_stop_cb(struct k_timer *timer) {
    LOG_WRN("startup timer stopped");
    stop_startup();
}

static void on_startup_timer_tick_cb(struct k_timer *timer) {
    running_timer = timer;
    k_work_submit_to_queue(zmk_workqueue_lowprio_work_q(), &on_startup_timer_tick_work);
}

K_TIMER_DEFINE(on_startup_timer_tick, on_startup_timer_tick_cb, on_startup_timer_tick_stop_cb);

int startup_handler(const zmk_event_t *eh) {
    struct zmk_activity_state_changed *ev = as_zmk_activity_state_changed(eh);
    if (ev == NULL) {
        return -ENOTSUP;
    }

    LOG_INF("activity state changed %d %d %d, %d", ZMK_ACTIVITY_ACTIVE, ZMK_ACTIVITY_IDLE,
            ZMK_ACTIVITY_SLEEP, ev->state);
    switch (ev->state) {
    case ZMK_ACTIVITY_ACTIVE:
        /*if (last_activity_state == ZMK_ACTIVITY_SLEEP) {*/
        if (!start_startup()) {
            LOG_ERR("already starting up");
            /*break;*/
        }
        startup_state = BATTERY;
        last_checkpoint = k_uptime_get();
        k_timer_start(&on_startup_timer_tick, K_NO_WAIT, K_MSEC(100));
        break;
        /*}*/
    default:
        LOG_INF("defaulting and not changing color");
        if (is_starting_up()) {
            LOG_WRN("stopping timer incomind");
            k_timer_stop(&on_startup_timer_tick);
        }
        break;
    }

    last_activity_state = ev->state;
    return 0;
}

ZMK_LISTENER(status_on_startup, startup_handler);
ZMK_SUBSCRIPTION(status_on_startup, zmk_activity_state_changed);
