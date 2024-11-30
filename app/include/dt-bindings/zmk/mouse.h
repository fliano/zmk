/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <zephyr/dt-bindings/dt-util.h>

/* Mouse press behavior */
/* Left click */
#define MB1 BIT(0)
#define LCLK (MB1)

/* Right click */
#define MB2 BIT(1)
#define RCLK (MB2)

/* Middle click */
#define MB3 BIT(2)
#define MCLK (MB3)

/* Backward */
#define MB4 BIT(3)
#define MBBWD (MB4)

/* Forward */
#define MB5 BIT(4)
#define MBFWD (MB5)

/* Mouse movement behavior */
#define MOVE_VERT(vert) ((vert) & 0x00FF)
#define MOVE_VERT_DECODE(encoded) (int8_t)((encoded) & 0x00FF)
#define MOVE_HOR(hor) (((hor) & 0x00FF) << 8)
#define MOVE_HOR_DECODE(encoded) (int8_t)(((encoded) & 0xFF00) >> 8)

#define MOVE(hor, vert) (MOVE_HOR(hor) + MOVE_VERT(vert))

#define MOVE_UP MOVE_VERT(-600)
#define MOVE_DOWN MOVE_VERT(600)
#define MOVE_LEFT MOVE_HOR(-600)
#define MOVE_RIGHT MOVE_HOR(600)
