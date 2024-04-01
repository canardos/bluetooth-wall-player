#ifndef SRC_DEVICES_OLED_H_
#define SRC_DEVICES_OLED_H_

#include "graphics/idrawing_surface.h"

Libp::IDrawingSurface<uint8_t>& getOled();

/// Start SPI and initialize the display
void oledInit();

/**
 * Update the OLED brightness based on ambient light levels.
 *
 * @param ambient_millilux Ambient light level in millilux
 */
void oledUpdateBrightness(uint32_t ambient_millilux);

/**
 * Send the on/off command to the display.
 */
void oledSetPower(bool on);

#endif /* SRC_DEVICES_OLED_H_ */
