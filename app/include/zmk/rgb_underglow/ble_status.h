#pragma once

#include <zmk/endpoints.h>
#include <zmk/event_manager.h>

struct output_state {
    struct zmk_endpoint_instance selected_endpoint;
    bool active_profile_connected;
    bool active_profile_bonded;
};

static struct output_state get_output_state(const zmk_event_t *_eh);
int zmk_rgb_underglow_set_color_ble(struct output_state os);
