/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/settings/settings.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(zmk, CONFIG_ZMK_LOG_LEVEL);

#if IS_ENABLED(CONFIG_ZMK_DISPLAY)

#include <zmk/display.h>
#include <lvgl.h>

#endif

#ifdef CONFIG_ZMK_MOUSE
#include <zmk/mouse.h>
#endif /* CONFIG_ZMK_MOUSE */

#ifdef CONFIG_ZMK_POINT_DEVICE
#include <zmk/point_device.h>
#endif /* CONFIG_ZMK_POINT_DEVICE */

int main(void) {
    LOG_INF("Welcome to ZMK!\n");

#if IS_ENABLED(CONFIG_SETTINGS)
    settings_subsys_init();
    settings_load();
#endif

#ifdef CONFIG_ZMK_DISPLAY
    zmk_display_init();

#if IS_ENABLED(CONFIG_ARCH_POSIX)
    // Workaround for an SDL display issue:
    // https://github.com/zephyrproject-rtos/zephyr/issues/71410
    while (1) {
        lv_task_handler();
        k_sleep(K_MSEC(10));
    }
#endif

#endif /* CONFIG_ZMK_DISPLAY */

#ifdef CONFIG_ZMK_MOUSE
    zmk_mouse_init();
#endif /* CONFIG_ZMK_MOUSE */

#ifdef CONFIG_ZMK_POINT_DEVICE
    zmk_pd_init();
#endif /* CONFIG_ZMK_POINT_DEVICE */

    return 0;
}
