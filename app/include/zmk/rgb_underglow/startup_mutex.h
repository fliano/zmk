#pragma once

#include <zephyr/kernel.h>

K_MUTEX_DEFINE(startup_mutex);

bool is_starting_up(void);
