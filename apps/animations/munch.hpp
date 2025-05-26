/* _    _  _ _  _ _ ____ ____ _ ___
 * |    |  |  \/  | | __ |__/ | |  \
 * |___ |__| _/\_ | |__] |  \ | |__/
 * =================================
 * Luxigrid - Munch Animation
 * Copyright (c) 2024 OverScore Media - MIT License
 *
 * Adapted from Aurora: https://github.com/pixelmatix/aurora
 *
 * Original Copyright Notice:
 * ==================================================
 * Copyright (c) 2014 Jason Coon - MIT License
 *
 * Munch pattern created by J.B. Langston:
 * https://github.com/jblang/aurora/blob/master/PatternMunch.h
 * ==================================
 *
 * MIT LICENSE:
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef MUNCH_GUARD
#define MUNCH_GUARD

#include "Arduino.h"
#include "../../lib/luxigrid.h"

#include "../../lib/animation-helpers.hpp"

byte count = 0;
byte dir = 1;
byte flip = 0;
byte generation = 0;

///////////////////
// SETUP FUNCTION
///////////////////
void setup() {
	setupMatrix();

	// Allocate memory for the leds data structure, and set the FastLED palette
	leds = (CRGB *)malloc(NUM_LEDS * sizeof(CRGB));
	memset(leds, 0x00, NUM_LEDS * sizeof(CRGB));
	currentPalette = RainbowColors_p;
	last_frame = millis();
}

///////////////////
// MAIN LOOP
///////////////////
void loop() {
	// If an OTA update is in progress, skip this iteration of the loop
	if (otaUpdateInProgress) {
		vTaskDelay(10 / portTICK_PERIOD_MS);
		return;
	}

	if (1000 / default_fps + last_frame < millis()) {
		for (uint16_t x = 0; x < MATRIX_WIDTH; x++) {
			for (uint16_t y = 0; y < MATRIX_HEIGHT; y++) {
				leds[XY16(x, y)] = (x ^ y ^ flip) < count ? ColorFromPalette(currentPalette, ((x ^ y) << 2) + generation) : CRGB::Black;
			}
		}

		count += dir;

		if (count <= 0 || count >= MATRIX_WIDTH) {
			dir = -dir;
		}

		if (count <= 0) {
			if (flip == 0) {
				flip = MATRIX_WIDTH - 1;
			}

			else {
				flip = 0;
			}
		}

		generation++;
		updateScreen();
		last_frame = millis();
	}
}

#endif