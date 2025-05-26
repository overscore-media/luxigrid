/* _    _  _ _  _ _ ____ ____ _ ___
 * |    |  |  \/  | | __ |__/ | |  \
 * |___ |__| _/\_ | |__] |  \ | |__/
 * =================================
 * Luxigrid - Pendulum Wave Animation
 * Copyright (c) 2024 OverScore Media - MIT License
 *
 * Adapted from Aurora: https://github.com/pixelmatix/aurora
 *
 * Original Copyright Notice:
 * ==================================================
 * Copyright (c) 2014 Jason Coon - MIT License
 *
 * Inspired by and based on a loading animation for Prismata by Lunarch Studios:
 * http://www.reddit.com/r/gifs/comments/2on8si/connecting_to_server_so_mesmerizing/cmow0sz
 *
 * Lunarch Studios Inc. hereby publishes the Actionscript 3 source code pasted in this
 * comment under the Creative Commons CC0 1.0 Universal Public Domain Dedication.
 * Lunarch Studios Inc. waives all rights to the work worldwide under copyright law,
 * including all related and neighboring rights, to the extent allowed by law.
 * You can copy, modify, distribute and perform the work, even for commercial purposes,
 * all without asking permission.
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

#ifndef PENDULUM_WAVE_GUARD
#define PENDULUM_WAVE_GUARD

#include "Arduino.h"
#include "../../lib/luxigrid.h"

#include "../../lib/animation-helpers.hpp"

#define WAVE_BPM 25
#define AMP_BPM 2
#define SKEW_BPM 4
#define WAVE_TIMEMINSKEW MATRIX_WIDTH / 8
#define WAVE_TIMEMAXSKEW MATRIX_WIDTH / 2

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
		// Reset the screen
		memset(leds, 0x00, NUM_LEDS * sizeof(CRGB));

		for (int x = 0; x < MATRIX_WIDTH; ++x) {
			uint16_t amp = beatsin16(AMP_BPM, MATRIX_HEIGHT / 8, MATRIX_HEIGHT - 1);
			uint16_t offset = (MATRIX_HEIGHT - beatsin16(AMP_BPM, 0, MATRIX_HEIGHT)) / 2;

			uint8_t y = beatsin16(WAVE_BPM, 0, amp, x * beatsin16(SKEW_BPM, WAVE_TIMEMINSKEW, WAVE_TIMEMAXSKEW)) + offset;

			leds[XY16(x, y)] = ColorFromPalette(currentPalette, x * 7);
		}

		updateScreen();
		last_frame = millis();
	}
}

#endif