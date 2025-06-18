#include <zephyr/logging/log.h>
#include <zmk/rgb_underglow/startup_mutex.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static K_MUTEX_DEFINE(startup_mutex);
bool starting_up = false;

bool set_starting_up(bool value) {
    if (k_mutex_lock(&startup_mutex, K_MSEC(300)) != 0) {
        LOG_WRN("failing to set starting up status since mutex is locked");
        return false;
    }
    if (starting_up == value) {
        LOG_DBG("already set to %d, not changing", value);
    }
    starting_up = value;
    int unlock = k_mutex_unlock(&startup_mutex);
    LOG_DBG("unlocked mutex, set starting_up to %d, status was %d, 0, %d, %d", value, unlock,
            -EPERM, -EINVAL);
    return true;
}

bool is_starting_up() {
    if (k_mutex_lock(&startup_mutex, K_MSEC(300)) != 0) {
        LOG_INF("Cannot get starting up status since mutex is locked");
        return true;
    } else {
        bool ret = starting_up;
        int unlock = k_mutex_unlock(&startup_mutex);
        LOG_INF("status was %d, 0, %d, %d", unlock, -EPERM, -EINVAL);
        return ret;
    }
}

bool start_startup() { return set_starting_up(true); }

void stop_startup() { return set_starting_up(false); }
