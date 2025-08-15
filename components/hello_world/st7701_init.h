#ifndef ST7701_INIT_H
#define ST7701_INIT_H

#include <stdint.h>

static const uint8_t st7701_type1_init_operations[] = {
    0xFF, 5, 0x77, 0x01, 0x00, 0x00, 0x13,  // Command set
    0xEF, 3, 0x08, 0x00, 0x45,                // Register settings
    0xE0, 2, 0x09, 0x07,                      // Power control
    0xE1, 2, 0xE2, 0x45,                      // Power control
    0xE2, 2, 0x0B, 0x0B,                      // Power control
    0xB0, 1, 0x08,                            // Positive voltage gamma
    0xB1, 1, 0x10,                            // Negative voltage gamma
    0xB4, 1, 0x12,                            // Display control
    0x36, 1, 0x00,                            // Memory access control (RGB)
    0x3A, 1, 0x55,                            // Pixel format (RGB565)
    0x11, 0,                                  // Sleep out
    0xFF, 5, 0x77, 0x01, 0x00, 0x00, 0x10,  // Command set
    0xC0, 2, 0x3B, 0x00,                      // Power control
    0xC1, 1, 0x0D,                            // Power control
    0xC2, 2, 0x31, 0x08,                      // Power control
    0xCD, 1, 0x08,                            // Display control
    0x29, 0,                                  // Display on
};

#endif