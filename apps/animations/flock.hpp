/* _    _  _ _  _ _ ____ ____ _ ___
 * |    |  |  \/  | | __ |__/ | |  \
 * |___ |__| _/\_ | |__] |  \ | |__/
 * =================================
 * Luxigrid - Flock Animation
 * Copyright (c) 2024 OverScore Media - MIT License
 *
 * Adapted from Aurora: https://github.com/pixelmatix/aurora
 *
 * Original Copyright Notice:
 * ==================================================
 * Copyright (c) 2014 Jason Coon - MIT License
 *
 * Portions of this code are adapted from "Flocking" in "The Nature of Code" by Daniel Shiffman: http://natureofcode.com/
 * Copyright (c) 2014 Daniel Shiffman
 * http://www.shiffman.net
 *
 * Demonstration of Craig Reynolds' "Flocking" behavior: http://www.red3d.com/cwr/
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

#ifndef FLOCK_GUARD
#define FLOCK_GUARD

#include "Arduino.h"
#include "../../lib/luxigrid.h"

#include "../../lib/animation-helpers.hpp"
#include "../../lib/Vector.h"
#include "../../lib/Boid.h"

static const uint8_t AVAILABLE_BOID_COUNT = 40;
Boid boids[AVAILABLE_BOID_COUNT];

static const int boidCount = 10;
Boid predator;

PVector wind;
byte hue = 0;
bool predatorPresent = true;

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

	for (int i = 0; i < boidCount; i++) {
		boids[i] = Boid(15, 15);
		boids[i].maxspeed = 0.380;
		boids[i].maxforce = 0.015;
	}

	predatorPresent = random(0, 2) >= 1;

	if (predatorPresent) {
		predator = Boid(31, 31);
		predatorPresent = true;
		predator.maxspeed = 0.385;
		predator.maxforce = 0.020;
		predator.neighbordist = 16.0;
		predator.desiredseparation = 0.0;
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
			leds[i].nscale8(230);
		}

		updateScreen();

		bool applyWind = random(0, 255) > 250;

		if (applyWind) {
			wind.x = Boid::randomf() * .015;
			wind.y = Boid::randomf() * .015;
		}

		CRGB color = ColorFromPalette(currentPalette, hue);

		for (int i = 0; i < boidCount; i++) {
			Boid *boid = &boids[i];

			// Flee from predator
			if (predatorPresent) {
				boid->repelForce(predator.location, 10);
			}

			boid->run(boids, boidCount);
			boid->wrapAroundBorders();
			PVector location = boid->location;
			leds[XY16(location.x, location.y)] = color;

			if (applyWind) {
				boid->applyForce(wind);
				applyWind = false;
			}
		}

		if (predatorPresent) {
			predator.run(boids, boidCount);
			predator.wrapAroundBorders();
			color = ColorFromPalette(currentPalette, hue + 128);
			PVector location = predator.location;
			leds[XY16(location.x, location.y)] = color;
		}

		EVERY_N_MILLIS(200) {
			hue++;
		}

		EVERY_N_SECONDS(30) {
			predatorPresent = !predatorPresent;
		}

		last_frame = millis();
	}
}

#endif