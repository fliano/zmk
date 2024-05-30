#include <zmk/rgb_underglow/startup_mutex.h>

bool is_starting_up() {
    if (!k_mutex_lock(&startup_mutex, K_NO_WAIT)) {
        return true;
    } else {
        k_mutex_unlock(&startup_mutex);
        return false;
    }
}
