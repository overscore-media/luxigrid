/* _    _  _ _  _ _ ____ ____ _ ___
 * |    |  |  \/  | | __ |__/ | |  \
 * |___ |__| _/\_ | |__] |  \ | |__/
 * =================================
 * Luxigrid - Swirl Animation
 * Copyright (c) 2024 OverScore Media - MIT License
 *
 * Adapted from Aurora: https://github.com/pixelmatix/aurora
 *
 * Original Copyright Notice:
 * ==================================================
 * Copyright (c) 2014 Jason Coon - MIT License
 *
 * Portions of this code are adapted from SmartMatrixSwirl by Mark Kriegsman: https://gist.github.com/kriegsman/5adca44e14ad025e6d3b
 * https://www.youtube.com/watch?v=bsGBT-50cts
 * Copyright (c) 2014 Mark Kriegsman
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

#ifndef SWIRL_GUARD
#define SWIRL_GUARD

#include "Arduino.h"
#include "../../lib/luxigrid.h"

#include "../../lib/animation-helpers.hpp"

const uint8_t borderWidth = 2;

// XY needs to be defined or the blur2d call below won't work
uint16_t XY(uint8_t x, uint8_t y) {
	return XY16(x, y);
}

void setup() {
	setupMatrix();

	// Allocate memory for the leds data structure, and set the FastLED palette
	leds = (CRGB *)malloc(NUM_LEDS * sizeof(CRGB));
	memset(leds, 0x00, NUM_LEDS * sizeof(CRGB));
	currentPalette = RainbowColors_p;
	last_frame = millis();
}

void loop() {
	if (1000 / default_fps + last_frame < millis()) {
		uint8_t blurAmount = beatsin8(2, 10, 255);

		blur2d(leds, MATRIX_WIDTH > 255 ? 255 : MATRIX_WIDTH, MATRIX_HEIGHT > 255 ? 255 : MATRIX_HEIGHT, blurAmount);

		// Use two out-of-sync sine waves
		uint8_t i = beatsin8(256 / MATRIX_HEIGHT, borderWidth, MATRIX_WIDTH - borderWidth);
		uint8_t j = beatsin8(2048 / MATRIX_WIDTH, borderWidth, MATRIX_HEIGHT - borderWidth);

		// Also calculate some reflections
		uint8_t ni = (MATRIX_WIDTH - 1) - i;
		uint8_t nj = (MATRIX_HEIGHT - 1) - j;

		// The colour of each point shifts over time, each at a different speed.
		uint16_t ms = millis();
		leds[XY16(i, j)] += ColorFromPalette(currentPalette, ms / 11);
		leds[XY16(ni, nj)] += ColorFromPalette(currentPalette, ms / 17);
		leds[XY16(i, nj)] += ColorFromPalette(currentPalette, ms / 37);
		leds[XY16(ni, j)] += ColorFromPalette(currentPalette, ms / 41);

		updateScreen();
		last_frame = millis();
	}
}

#endif