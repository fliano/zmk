#include <zmk/rgb_underglow/startup_mutex.h>

K_MUTEX_DEFINE(startup_mutex);

bool is_starting_up() {
    if (k_mutex_lock(&startup_mutex, K_NO_WAIT) != 0) {
        return true;
    } else {
        k_mutex_unlock(&startup_mutex);
        return false;
    }
}

bool start_startup() {
    if (k_mutex_lock(&startup_mutex, K_NO_WAIT) != 0) {
        return false;
    }
    return true;
}

void stop_startup() {
    if (!is_starting_up()) {
        LOG_ERR("stopping non running startup");
        return;
    }
    k_mutex_unlock(&startup_mutex);
}
