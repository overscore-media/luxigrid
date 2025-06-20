/* _    _  _ _  _ _ ____ ____ _ ___
 * |    |  |  \/  | | __ |__/ | |  \
 * |___ |__| _/\_ | |__] |  \ | |__/
 * =================================
 * Luxigrid - Life Animation
 * Copyright (c) 2024 OverScore Media - MIT License
 *
 * Adapted from Aurora: https://github.com/pixelmatix/aurora
 *
 * Original Copyright Notice:
 * ==================================================
 * Copyright (c) 2014 Jason Coon - MIT License
 *
 * Portions of this code are adapted from Andrew: http://pastebin.com/f22bfe94d
 * which, in turn, was "Adapted from the Life example on the Processing.org site"
 *
 * Made much more colorful by J.B. Langston: https://github.com/jblang/aurora/commit/6db5a884e3df5d686445c4f6b669f1668841929b
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

#ifndef LIFE_GUARD
#define LIFE_GUARD

#include "Arduino.h"
#include "../../lib/luxigrid.h"

#include "../../lib/animation-helpers.hpp"
#include "../../lib/Vector.h"
#include "../../lib/Boid.h"

class Cell {
	public:
	byte alive : 1;
	byte prev : 1;
	byte hue : 6;
	byte brightness;
};

Cell world[MATRIX_WIDTH][MATRIX_HEIGHT];

const unsigned int density = 50;
int generation = 0;

void regenerateWorld() {
	for (int i = 0; i < MATRIX_WIDTH; i++) {
		for (int j = 0; j < MATRIX_HEIGHT; j++) {
			if (random(100) < density) {
				world[i][j].alive = 1;
				world[i][j].brightness = 255;
			} else {
				world[i][j].alive = 0;
				world[i][j].brightness = 0;
			}

			world[i][j].prev = world[i][j].alive;
			world[i][j].hue = 0;
		}
	}
}

// Check how many neighours a given cell has
int neighbours(int x, int y) {
	return (world[(x + 1) % MATRIX_WIDTH][y].prev) +
	       (world[x][(y + 1) % MATRIX_HEIGHT].prev) +
	       (world[(x + MATRIX_WIDTH - 1) % MATRIX_WIDTH][y].prev) +
	       (world[x][(y + MATRIX_HEIGHT - 1) % MATRIX_HEIGHT].prev) +
	       (world[(x + 1) % MATRIX_WIDTH][(y + 1) % MATRIX_HEIGHT].prev) +
	       (world[(x + MATRIX_WIDTH - 1) % MATRIX_WIDTH][(y + 1) % MATRIX_HEIGHT].prev) +
	       (world[(x + MATRIX_WIDTH - 1) % MATRIX_WIDTH][(y + MATRIX_HEIGHT - 1) % MATRIX_HEIGHT].prev) +
	       (world[(x + 1) % MATRIX_WIDTH][(y + MATRIX_HEIGHT - 1) % MATRIX_HEIGHT].prev);
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
	// If an OTA update is in progress, skip this iteration of the loop
	if (otaUpdateInProgress) {
		vTaskDelay(10 / portTICK_PERIOD_MS);
		return;
	}

	if (1000 / default_fps + last_frame < millis()) {
		// If the generation is 0, reset
		if (generation == 0) {
			memset(leds, 0x00, NUM_LEDS * sizeof(CRGB));
			regenerateWorld();
		}

		// Display the current generation
		for (int i = 0; i < MATRIX_WIDTH; i++) {
			for (int j = 0; j < MATRIX_HEIGHT; j++) {
				leds[XY16(i, j)] = ColorFromPalette(currentPalette, world[i][j].hue * 4, world[i][j].brightness);
			}
		}

		// Birth and death cycle
		for (int x = 0; x < MATRIX_WIDTH; x++) {
			for (int y = 0; y < MATRIX_HEIGHT; y++) {
				// Default is for each cell to stay the same
				if (world[x][y].brightness > 0 && world[x][y].prev == 0) {
					world[x][y].brightness *= 0.9;
				}

				int count = neighbours(x, y);

				if (count == 3 && world[x][y].prev == 0) {
					// A new cell is born
					world[x][y].alive = 1;
					world[x][y].hue += 2;
					world[x][y].brightness = 255;
				} else if ((count < 2 || count > 3) && world[x][y].prev == 1) {
					// Cell dies
					world[x][y].alive = 0;
				}
			}
		}

		// Copy next generation into place
		for (int x = 0; x < MATRIX_WIDTH; x++) {
			for (int y = 0; y < MATRIX_HEIGHT; y++) {
				world[x][y].prev = world[x][y].alive;
			}
		}

		generation++;

		// Reset the generation after a number of cycles (otherwise it gets boring pretty quickly)
		if (generation >= 256) {
			generation = 0;
		}

		updateScreen();
		last_frame = millis();
	}
}

#endif