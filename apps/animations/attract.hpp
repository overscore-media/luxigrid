/* _    _  _ _  _ _ ____ ____ _ ___
 * |    |  |  \/  | | __ |__/ | |  \
 * |___ |__| _/\_ | |__] |  \ | |__/
 * =================================
 * Luxigrid - Attract Animation
 * Copyright (c) 2024 OverScore Media - MIT License
 *
 * Adapted from Aurora: https://github.com/pixelmatix/aurora
 *
 * Original Copyright Notice:
 * ==================================================
 * Copyright (c) 2014 Jason Coon - MIT License
 *
 * Portions of this code are adapted from "Attractor" in "The Nature of Code" by Daniel Shiffman: http://natureofcode.com/
 * Copyright (c) 2014 Daniel Shiffman
 * http://www.shiffman.net
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

#ifndef ATTRACT_GUARD
#define ATTRACT_GUARD

#include "Arduino.h"
#include "../../lib/luxigrid.h"

#include "../../lib/animation-helpers.hpp"
#include "../../lib/Vector.h"
#include "../../lib/Boid.h"

class Attractor {
	public:
	float mass;        // Mass, tied to size
	float G;           // Gravitational Constant
	PVector location;  // Location

	Attractor() {
		location = PVector(MATRIX_CENTER_X, MATRIX_CENTER_Y);
		mass = 10;
		G = .5;
	}

	PVector attract(Boid m) {
		PVector force = location - m.location;           // Calculate direction of force
		float d = force.mag();                           // Distance between objects
		d = constrain(d, 5.0, 32.0);                     // Limiting the distance to eliminate "extreme" results for very close or very far objects
		force.normalize();                               // Normalize vector (distance doesn't matter here, we just want this vector for direction)
		float strength = (G * mass * m.mass) / (d * d);  // Calculate gravitational force magnitude
		force *= strength;                               // Get force vector --> magnitude * direction
		return force;
	}
};

static const uint8_t AVAILABLE_BOID_COUNT = 40;
Boid boids[AVAILABLE_BOID_COUNT];

const uint8_t count = 12;
Attractor attractor;

///////////////////
// SETUP FUNCTION
///////////////////
void setup() {
	// Initialize the LED Matrix
	setupMatrix();

	// Allocate memory for the leds data structure, and set the FastLED palette
	leds = (CRGB *)malloc(NUM_LEDS * sizeof(CRGB));
	memset(leds, 0x00, NUM_LEDS * sizeof(CRGB));
	currentPalette = RainbowColors_p;
	last_frame = millis();

	int direction = random(0, 2);
	if (direction == 0) {
		direction = -1;
	}

	for (int i = 0; i < count; i++) {
		Boid boid = Boid(15, 31 - i);
		boid.mass = 1;
		boid.velocity.x = ((float)random(40, 50)) / 100.0;
		boid.velocity.x *= direction;
		boid.velocity.y = 0;
		boid.colorIndex = i * 32;
		boids[i] = boid;
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
		uint8_t dim = beatsin8(2, 170, 250);

		for (int i = 0; i < NUM_LEDS; i++) {
			leds[i].nscale8(dim);
		}

		for (int i = 0; i < count; i++) {
			Boid boid = boids[i];

			PVector force = attractor.attract(boid);
			boid.applyForce(force);

			boid.update();

			leds[XY16(boid.location.x, boid.location.y)] = ColorFromPalette(currentPalette, boid.colorIndex);

			boids[i] = boid;
		}

		updateScreen();
		last_frame = millis();
	}
}

#endif