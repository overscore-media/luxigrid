/* _    _  _ _  _ _ ____ ____ _ ___
 * |    |  |  \/  | | __ |__/ | |  \
 * |___ |__| _/\_ | |__] |  \ | |__/
 * =================================
 * Luxigrid - Simplex Noise Animation
 * Copyright (c) 2024 OverScore Media - MIT License
 *
 * Adapted from Aurora: https://github.com/pixelmatix/aurora
 *
 * Original Copyright Notice:
 * ==================================================
 * Copyright (c) 2014 Jason Coon - MIT License
 *
 * Portions of this code are adapted from FastLED Fire2012 example by Mark Kriegsman:
 * https://github.com/FastLED/FastLED/blob/master/examples/Noise/Noise.ino
 * Copyright (c) 2013 FastLED
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

#ifndef SIMPLEX_NOISE_GUARD
#define SIMPLEX_NOISE_GUARD

#include "Arduino.h"
#include "../../lib/luxigrid.h"

#include "../../lib/animation-helpers.hpp"

const uint8_t noisesmoothing = 200;
const uint32_t speed = 100;

uint32_t noise_x, noise_y, noise_z;

uint32_t noise_scale_x = 6000;
uint32_t noise_scale_y = 6000;

// Memory will be allocated for this later
uint8_t **noise = nullptr;

void ShowNoiseLayer(byte layer, byte colorrepeat, byte colorshift) {
	for (uint16_t i = 0; i < MATRIX_WIDTH; i++) {
		for (uint16_t j = 0; j < MATRIX_HEIGHT; j++) {
			uint8_t pixel = noise[i][j];
			leds[XY16(i, j)] = ColorFromPalette(currentPalette, colorrepeat * (pixel + colorshift), pixel);
		}
	}
}

void FillNoise() {
	for (uint16_t i = 0; i < MATRIX_WIDTH; i++) {
		uint32_t ioffset = noise_scale_x * (i - MATRIX_CENTER_Y);

		for (uint16_t j = 0; j < MATRIX_HEIGHT; j++) {
			uint32_t joffset = noise_scale_y * (j - MATRIX_CENTER_Y);

			byte data = inoise16(noise_x + ioffset, noise_y + joffset, noise_z) >> 8;

			uint8_t olddata = noise[i][j];
			uint8_t newdata = scale8(olddata, noisesmoothing) + scale8(data, 256 - noisesmoothing);
			data = newdata;

			noise[i][j] = data;
		}
	}
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

	// Allocate memory for the noise effect
	// Original Comment: (there should be some guards for malloc errors eventually)
	noise = (uint8_t **)malloc(MATRIX_WIDTH * sizeof(uint8_t *));

	for (int i = 0; i < MATRIX_WIDTH; ++i) {
		noise[i] = (uint8_t *)malloc(MATRIX_HEIGHT * sizeof(uint8_t));
	}

	noise_x = random16();
	noise_y = random16();
	noise_z = random16();
}

///////////////////
// MAIN LOOP
///////////////////
void loop() {
	if (1000 / default_fps + last_frame < millis()) {
		EVERY_N_SECONDS(15) {
			noise_x = random16();
			noise_y = random16();
			noise_z = random16();
		}

		noise_y += speed;
		noise_z += speed;

		FillNoise();
		ShowNoiseLayer(0, 1, 0);
		updateScreen();
		last_frame = millis();
	}
}

#endif