#ifndef RGB_DRIVER_APA102_H
#define RGB_DRIVER_APA102_H

#include "RGB_LED.h"

#include <stdint.h>
#include <stddef.h>
#include <chip.h>


typedef struct {
    LPC_SSP_T *SSP;

    uint8_t brightness;
    size_t count;

} APA102;

#define APA102_BRIGHTNESS_MIN 1
#define APA102_BRIGHTNESS_MAX 0b11111

bool RGB_driver_APA102_init(APA102 *ctx, LPC_SSP_T *LPC_SSP);
/**
 * Set the brightness level
 *
 * The brightness level can be changed at any time and has affect on all
 * following RGB_driver_APA102_set_color() calls.
 *
 * NOTE: brightness levels are capped between APA102_BRIGHTNESS_MIN and
 * APA102_BRIGHTNESS_MAX
 */
void RGB_driver_APA102_set_brightness(APA102 *ctx, int brightness);

/**
 * Start a new transaction of colors to the APA102 RGB LED string
 */
bool RGB_driver_APA102_begin(APA102 *ctx);

/**
 * Set the color of the current RGB LED.
 *
 * The first function call affects the first LED in the strip,
 * then the second etc.
 *
 * NOTE: Before setting the first color, call RGB_driver_APA102_begin()
 * NOTE: After all colors are set, call RGB_driver_APA102_commit()
 */
bool RGB_driver_APA102_set_color(APA102 *ctx, RGBColor color);

/**
 * Finish the current transaction: all APA102 RGB LEDs are updated with
 * their new color(s).
 */
bool RGB_driver_APA102_commit(APA102 *ctx);
#endif

