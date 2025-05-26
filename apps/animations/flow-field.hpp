/* _    _  _ _  _ _ ____ ____ _ ___
 * |    |  |  \/  | | __ |__/ | |  \
 * |___ |__| _/\_ | |__] |  \ | |__/
 * =================================
 * Luxigrid - Flow Field Animation
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

#ifndef FLOW_FIELD_GUARD
#define FLOW_FIELD_GUARD

#include "Arduino.h"
#include "../../lib/luxigrid.h"

#include "../../lib/animation-helpers.hpp"
#include "../../lib/Vector.h"
#include "../../lib/Boid.h"

static const uint8_t AVAILABLE_BOID_COUNT = 40;
Boid boids[AVAILABLE_BOID_COUNT];

uint16_t x;
uint16_t y;
uint16_t z;

const uint16_t speed = 1;
const uint16_t scale = 26;
static const int count = 40;

byte hue = 0;

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

	x = random16();
	y = random16();
	z = random16();

	for (int i = 0; i < count; i++) {
		boids[i] = Boid(random(MATRIX_WIDTH), 0);
	}
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
		for (int i = 0; i < NUM_LEDS; i++) {
			leds[i].nscale8(240);
		}

		for (int i = 0; i < count; i++) {
			Boid *boid = &boids[i];

			int ioffset = scale * boid->location.x;
			int joffset = scale * boid->location.y;

			byte angle = inoise8(x + ioffset, y + joffset, z);

			boid->velocity.x = (float)sin8(angle) * 0.0078125 - 1.0;
			boid->velocity.y = -((float)cos8(angle) * 0.0078125 - 1.0);
			boid->update();

			leds[XY16(boid->location.x, boid->location.y)] = ColorFromPalette(currentPalette, angle + hue);

			if (boid->location.x < 0 || boid->location.x >= MATRIX_WIDTH || boid->location.y < 0 || boid->location.y >= MATRIX_HEIGHT) {
				boid->location.x = random(MATRIX_WIDTH);
				boid->location.y = 0;
			}
		}

		EVERY_N_MILLIS(200) {
			hue++;
		}

		x += speed;
		y += speed;
		z += speed;

		updateScreen();

		last_frame = millis();
	}
}

#endif