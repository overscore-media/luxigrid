/* _    _  _ _  _ _ ____ ____ _ ___
 * |    |  |  \/  | | __ |__/ | |  \
 * |___ |__| _/\_ | |__] |  \ | |__/
 * =================================
 * Luxigrid - Incremental Drift Animation
 * Copyright (c) 2024 OverScore Media - MIT License
 *
 * Adapted from Aurora: https://github.com/pixelmatix/aurora
 * Copyright (c) 2014 Jason Coon - MIT License
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

#ifndef INCREMENTAL_DRIFT_GUARD
#define INCREMENTAL_DRIFT_GUARD

#include "Arduino.h"
#include "../../lib/luxigrid.h"

#include "../../lib/animation-helpers.hpp"

uint8_t beatcos8(accum88 beats_per_minute, uint8_t lowest = 0, uint8_t highest = 255, uint32_t timebase = 0, uint8_t phase_offset = 0) {
	uint8_t beat = beat8(beats_per_minute, timebase);
	uint8_t beatcos = cos8(beat + phase_offset);
	uint8_t rangewidth = highest - lowest;
	uint8_t scaledbeat = scale8(beatcos, rangewidth);
	uint8_t result = lowest + scaledbeat;
	return result;
}

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
		uint8_t dim = beatsin8(2, 230, 250);

		for (int i = 0; i < NUM_LEDS; i++) {
			leds[i].nscale8(dim);
		}

		for (int i = 2; i <= MATRIX_WIDTH / 2; i++) {
			CRGB color = ColorFromPalette(currentPalette, (i - 2) * (240 / (MATRIX_WIDTH / 2)));

			uint8_t x = beatcos8((17 - i) * 2, MATRIX_CENTER_X - i, MATRIX_CENTER_X + i);
			uint8_t y = beatsin8((17 - i) * 2, MATRIX_CENTER_Y - i, MATRIX_CENTER_Y + i);

			leds[XY16(x, y)] = color;
		}

		// Disable the annoying pixel that doesn't want to fall in line
		leds[XY16(49, 16)] = CRGB::Black;

		updateScreen();
		last_frame = millis();
	}
}

#endif