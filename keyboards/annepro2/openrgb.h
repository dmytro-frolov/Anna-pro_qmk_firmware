#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "usb_descriptor.h"

// OpenRGB protocol version
#define OPENRGB_PROTOCOL_VERSION 0x0C

enum openrgb_command_id {
    OPENRGB_GET_PROTOCOL_VERSION = 1,
    OPENRGB_GET_QMK_VERSION,
    OPENRGB_GET_DEVICE_INFO,
    OPENRGB_GET_MODE_INFO,
    OPENRGB_GET_LED_INFO,
    OPENRGB_GET_ENABLED_MODES,

    OPENRGB_SET_MODE,
    OPENRGB_DIRECT_MODE_SET_SINGLE_LED,
    OPENRGB_DIRECT_MODE_SET_LEDS,
};

enum openrgb_responses {
    OPENRGB_FAILURE        = 25,
    OPENRGB_SUCCESS        = 50,
    OPENRGB_END_OF_MESSAGE = 100,
};

extern bool openrgb_is_direct;

void openrgb_init(void);
void openrgb_receive(uint8_t *data, uint8_t length);
void openrgb_direct_disable(void);
