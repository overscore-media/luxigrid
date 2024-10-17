/* _    _  _ _  _ _ ____ ____ _ ___
 * |    |  |  \/  | | __ |__/ | |  \
 * |___ |__| _/\_ | |__] |  \ | |__/
 * =================================
 * Luxigrid - Animation Helper Functions
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

#ifndef ANIMATION_HELPERS_GUARD
#define ANIMATION_HELPERS_GUARD

#include "Arduino.h"
#include <FastLED.h>

#include "luxigrid.h"

CRGBPalette16 currentPalette;
CRGB *leds;

const uint16_t NUM_LEDS = (MATRIX_WIDTH * MATRIX_HEIGHT) + 1;
uint8_t MATRIX_CENTER_X = MATRIX_WIDTH / 2;
uint8_t MATRIX_CENTER_Y = MATRIX_HEIGHT / 2;

unsigned long fps_timer;
const unsigned int default_fps = 30;
unsigned long last_frame = 0, ms_previous = 0;

uint16_t XY16(uint16_t x, uint16_t y) {
	if (x >= MATRIX_WIDTH) {
		return 0;
	}

	if (y >= MATRIX_HEIGHT) {
		return 0;
	}

	// Everything offset by one to compute out of bounds stuff - never displayed by ShowFrame()
	return (y * MATRIX_WIDTH) + x + 1;
}

void updateScreen() {
	for (int y = 0; y < MATRIX_HEIGHT; ++y) {
		for (int x = 0; x < MATRIX_WIDTH; ++x) {
			uint16_t _pixel = XY16(x, y);
			dma_display->drawPixelRGB888(x, y, leds[_pixel].r, leds[_pixel].g, leds[_pixel].b);
		}
	}
}

#endif