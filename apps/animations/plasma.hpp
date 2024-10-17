/* _    _  _ _  _ _ ____ ____ _ ___
 * |    |  |  \/  | | __ |__/ | |  \
 * |___ |__| _/\_ | |__] |  \ | |__/
 * =================================
 * Luxigrid - Plasma Animation
 * Copyright (c) 2024 OverScore Media - MIT License
 *
 * Adapted from Aurora: https://github.com/pixelmatix/aurora
 *
 * Original Copyright Notice:
 * ==================================================
 * Copyright (c) 2014 Jason Coon - MIT License
 *
 * Portions of this code are adapted from LedEffects Plasma by Robert Atkins:
 * https://bitbucket.org/ratkins/ledeffects/src/26ed3c51912af6fac5f1304629c7b4ab7ac8ca4b/Plasma.cpp?at=default
 *
 * Copyright (c) 2013 Robert Atkins
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

#ifndef PLASMA_GUARD
#define PLASMA_GUARD

#include "Arduino.h"
#include "../../lib/luxigrid.h"

#include "../../lib/animation-helpers.hpp"

int counter = 0;
int cycles = 0;

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
	if (1000 / default_fps + last_frame < millis()) {
		for (int x = 0; x < MATRIX_WIDTH; x++) {
			for (int y = 0; y < MATRIX_HEIGHT; y++) {
				int16_t v = 0;
				uint8_t wibble = sin8(counter);

				v += sin16(x * wibble * 2 + counter);
				v += cos16(y * (128 - wibble) * 2 + counter);
				v += sin16(y * x * cos8(-counter) / 2);

				leds[XY16(x, y)] = ColorFromPalette(currentPalette, (v >> 8) + 127);
			}
		}

		counter += 1;
		cycles++;

		if (cycles >= 2048) {
			counter = 0;
			cycles = 0;
		}

		updateScreen();
		last_frame = millis();
	}
}

#endif