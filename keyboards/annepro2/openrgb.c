#include <string.h>
#include "quantum.h"
#include "color.h"
#include "raw_hid.h"
#include "annepro2.h"
#include "qmk_ap2_led.h"
#include "openrgb.h"

bool openrgb_is_direct = false;
static uint8_t raw_hid_buffer[RAW_EPSIZE];

void openrgb_init(void) {
    openrgb_is_direct = false;
    memset(raw_hid_buffer, 0, sizeof(raw_hid_buffer));
}

void openrgb_direct_disable(void) {
    if (openrgb_is_direct) {
        openrgb_is_direct = false;
        annepro2LedSetManual(0);
        annepro2LedResetForegroundColor();
        annepro2LedEnable();
    }
}

static void openrgb_get_protocol_version(void) {
    raw_hid_buffer[0] = OPENRGB_GET_PROTOCOL_VERSION;
    raw_hid_buffer[1] = OPENRGB_PROTOCOL_VERSION;
}

static void openrgb_get_qmk_version(void) {
    raw_hid_buffer[0] = OPENRGB_GET_QMK_VERSION;
    const char *ver = "0.9.0";
#ifdef QMK_VERSION
    ver = QMK_VERSION;
#endif
    uint8_t current_byte = 1;
    for (uint8_t i = 0; (current_byte < (RAW_EPSIZE - 2)) && (ver[i] != 0); i++) {
        raw_hid_buffer[current_byte++] = (uint8_t)ver[i];
    }
    raw_hid_buffer[current_byte] = 0;
}

static void openrgb_get_device_info(void) {
    raw_hid_buffer[0] = OPENRGB_GET_DEVICE_INFO;
    raw_hid_buffer[1] = KEY_COUNT;                  // 70
    raw_hid_buffer[2] = MATRIX_COLS * MATRIX_ROWS;  // 70

    const char *prod = "Anne Pro 2";
    const char *mfg  = "ObinsLab";

    uint8_t current_byte = 3;
    for (uint8_t i = 0; (current_byte < ((RAW_EPSIZE - 2) / 2)) && (prod[i] != 0); i++) {
        raw_hid_buffer[current_byte++] = (uint8_t)prod[i];
    }
    raw_hid_buffer[current_byte++] = 0;

    for (uint8_t i = 0; (current_byte + 2 < RAW_EPSIZE) && (mfg[i] != 0); i++) {
        raw_hid_buffer[current_byte++] = (uint8_t)mfg[i];
    }
    raw_hid_buffer[current_byte] = 0;
}

static uint8_t current_openrgb_mode = 1;
static uint8_t current_openrgb_h    = 0;
static uint8_t current_openrgb_s    = 0;
static uint8_t current_openrgb_v    = 255;

static void openrgb_hsv_to_rgb(uint8_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b) {
    if (s == 0) {
        *r = v;
        *g = v;
        *b = v;
        return;
    }

    uint8_t region    = (uint16_t)h * 6 / 255;
    uint8_t remainder = ((uint16_t)h * 2 - (uint16_t)region * 85) * 3;

    uint8_t p = ((uint16_t)v * (255 - s)) >> 8;
    uint8_t q = ((uint16_t)v * (255 - (((uint16_t)s * remainder) >> 8))) >> 8;
    uint8_t t = ((uint16_t)v * (255 - (((uint16_t)s * (255 - remainder)) >> 8))) >> 8;

    switch (region) {
        case 6:
        case 0:
            *r = v;
            *g = t;
            *b = p;
            break;
        case 1:
            *r = q;
            *g = v;
            *b = p;
            break;
        case 2:
            *r = p;
            *g = v;
            *b = t;
            break;
        case 3:
            *r = p;
            *g = q;
            *b = v;
            break;
        case 4:
            *r = t;
            *g = p;
            *b = v;
            break;
        default:
            *r = v;
            *g = p;
            *b = q;
            break;
    }
}

static void openrgb_get_mode_info(void) {
    raw_hid_buffer[0] = OPENRGB_GET_MODE_INFO;
    raw_hid_buffer[1] = current_openrgb_mode;
    raw_hid_buffer[2] = 0;                  // speed
    raw_hid_buffer[3] = current_openrgb_h;  // hue
    raw_hid_buffer[4] = current_openrgb_s;  // sat
    raw_hid_buffer[5] = current_openrgb_v;  // val
}

static void openrgb_get_led_info(uint8_t *data) {
    const uint8_t first_led   = data[1];
    const uint8_t number_leds = data[2];

    raw_hid_buffer[0] = OPENRGB_GET_LED_INFO;

    for (uint8_t i = 0; i < number_leds; i++) {
        const uint8_t led_idx  = first_led + i;
        const uint8_t data_idx = i * 7;

        if (led_idx >= KEY_COUNT) {
            raw_hid_buffer[data_idx + 3] = OPENRGB_FAILURE;
            continue;
        }

        uint8_t row = led_idx / NUM_COLUMN;
        uint8_t col = led_idx % NUM_COLUMN;

        // Normalized X across 0..224, Y across 0..64
        uint8_t x = (col * 224) / (NUM_COLUMN - 1);
        uint8_t y = (row * 64) / (NUM_ROW - 1);

        raw_hid_buffer[data_idx + 1] = x;
        raw_hid_buffer[data_idx + 2] = y;
        raw_hid_buffer[data_idx + 3] = 0x04; // LED_FLAG_KEYLIGHT
        raw_hid_buffer[data_idx + 4] = ledColors[led_idx].p.red;
        raw_hid_buffer[data_idx + 5] = ledColors[led_idx].p.green;
        raw_hid_buffer[data_idx + 6] = ledColors[led_idx].p.blue;

        keypos_t pos = {.row = row, .col = col};
        uint16_t keycode = keymap_key_to_keycode(0, pos);
        raw_hid_buffer[data_idx + 7] = (uint8_t)(keycode & 0xFF);
    }
}

static void openrgb_get_enabled_modes(void) {
    raw_hid_buffer[0] = OPENRGB_GET_ENABLED_MODES;
    raw_hid_buffer[1] = 1; // Mode 1 = Direct Mode
    raw_hid_buffer[2] = 2; // Mode 2 = Static Mode
}

static void openrgb_set_mode(uint8_t *data) {
    const uint8_t h    = data[1];
    const uint8_t s    = data[2];
    const uint8_t v    = data[3];
    const uint8_t mode = data[4];

    current_openrgb_h    = h;
    current_openrgb_s    = s;
    current_openrgb_v    = v;
    current_openrgb_mode = mode;

    raw_hid_buffer[0] = OPENRGB_SET_MODE;

    if (mode == 1) { // Direct Mode
        openrgb_is_direct = true;
        annepro2LedEnable();
        annepro2LedSetManual(1);
    } else if (mode == 2) { // Static Mode
        openrgb_is_direct = true;
        annepro2LedEnable();
        annepro2LedSetManual(1);

        uint8_t red = 0, green = 0, blue = 0;
        openrgb_hsv_to_rgb(h, s, v, &red, &green, &blue);

        annepro2Led_t color = {
            .p.red   = red,
            .p.green = green,
            .p.blue  = blue,
            .p.alpha = 0xFF,
        };
        for (int i = 0; i < KEY_COUNT; i++) {
            ledColors[i] = color;
            ledMask[i]   = color;
        }
        annepro2LedColorSetMono(color);
        annepro2LedMaskSetMono(color);
        annepro2LedColorSetAll();
        annepro2LedMaskSetAll();
    } else if (mode == 0) { // Off
        openrgb_is_direct = true;
        annepro2LedDisable();
    } else { // Static or local profiles
        openrgb_is_direct = false;
        annepro2LedSetManual(0);
        annepro2LedResetForegroundColor();
        annepro2LedEnable();
    }

    raw_hid_buffer[RAW_EPSIZE - 2] = OPENRGB_SUCCESS;
}

static void openrgb_direct_mode_set_single_led(uint8_t *data) {
    uint8_t led = data[1];
    uint8_t r   = data[2];
    uint8_t g   = data[3];
    uint8_t b   = data[4];

    raw_hid_buffer[0] = OPENRGB_DIRECT_MODE_SET_SINGLE_LED;

    if (led >= KEY_COUNT) {
        raw_hid_buffer[RAW_EPSIZE - 2] = OPENRGB_FAILURE;
        return;
    }

    if (!openrgb_is_direct) {
        openrgb_is_direct = true;
        annepro2LedEnable();
        annepro2LedSetManual(1);
    }

    ledColors[led].p.red   = r;
    ledColors[led].p.green = g;
    ledColors[led].p.blue  = b;
    ledColors[led].p.alpha = 0xFF;

    ledMask[led].p.red   = r;
    ledMask[led].p.green = g;
    ledMask[led].p.blue  = b;
    ledMask[led].p.alpha = 0xFF;

    uint8_t row = led / NUM_COLUMN;
    uint8_t col = led % NUM_COLUMN;
    annepro2LedColorSetKey(row, col, ledColors[led]);
    annepro2LedMaskSetKey(row, col, ledMask[led]);

    raw_hid_buffer[RAW_EPSIZE - 2] = OPENRGB_SUCCESS;
}

static void openrgb_direct_mode_set_leds(uint8_t *data) {
    const uint8_t first_led   = data[1];
    const uint8_t number_leds = data[2];
    uint8_t row_dirty_mask    = 0;

    if (!openrgb_is_direct) {
        openrgb_is_direct = true;
        annepro2LedEnable();
        annepro2LedSetManual(1);
    }

    for (uint8_t i = 0; i < number_leds; i++) {
        uint8_t color_idx = first_led + i;
        if (color_idx >= KEY_COUNT) break;

        uint8_t data_idx = i * 3;
        ledColors[color_idx].p.red   = data[data_idx + 3];
        ledColors[color_idx].p.green = data[data_idx + 4];
        ledColors[color_idx].p.blue  = data[data_idx + 5];
        ledColors[color_idx].p.alpha = 0xFF;

        ledMask[color_idx].p.red   = data[data_idx + 3];
        ledMask[color_idx].p.green = data[data_idx + 4];
        ledMask[color_idx].p.blue  = data[data_idx + 5];
        ledMask[color_idx].p.alpha = 0xFF;

        uint8_t row = color_idx / NUM_COLUMN;
        row_dirty_mask |= (1 << row);
    }

    for (uint8_t r = 0; r < NUM_ROW; r++) {
        if (row_dirty_mask & (1 << r)) {
            annepro2LedColorSetRow(r);
            annepro2LedMaskSetRow(r);
        }
    }
}

void openrgb_receive(uint8_t *data, uint8_t length) {
    (void)length;
    memset(raw_hid_buffer, 0, sizeof(raw_hid_buffer));

    switch (*data) {
        case OPENRGB_GET_PROTOCOL_VERSION:
            openrgb_get_protocol_version();
            break;
        case OPENRGB_GET_QMK_VERSION:
            openrgb_get_qmk_version();
            break;
        case OPENRGB_GET_DEVICE_INFO:
            openrgb_get_device_info();
            break;
        case OPENRGB_GET_MODE_INFO:
            openrgb_get_mode_info();
            break;
        case OPENRGB_GET_LED_INFO:
            openrgb_get_led_info(data);
            break;
        case OPENRGB_GET_ENABLED_MODES:
            openrgb_get_enabled_modes();
            break;
        case OPENRGB_SET_MODE:
            openrgb_set_mode(data);
            break;
        case OPENRGB_DIRECT_MODE_SET_SINGLE_LED:
            openrgb_direct_mode_set_single_led(data);
            break;
        case OPENRGB_DIRECT_MODE_SET_LEDS:
            openrgb_direct_mode_set_leds(data);
            break;
        default:
            raw_hid_buffer[0]              = *data;
            raw_hid_buffer[RAW_EPSIZE - 2] = OPENRGB_FAILURE;
            break;
    }

    if (*data != OPENRGB_DIRECT_MODE_SET_LEDS) {
        raw_hid_buffer[RAW_EPSIZE - 1] = OPENRGB_END_OF_MESSAGE;
        raw_hid_send(raw_hid_buffer, RAW_EPSIZE);
    }
}

void raw_hid_receive(uint8_t *data, uint8_t length) {
    openrgb_receive(data, length);
}
