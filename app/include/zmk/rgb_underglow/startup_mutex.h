#pragma once

#include <zephyr/kernel.h>

bool is_starting_up(void);
bool start_startup(void);
void stop_startup(void);
