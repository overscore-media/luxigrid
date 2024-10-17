/* _    _  _ _  _ _ ____ ____ _ ___
 * |    |  |  \/  | | __ |__/ | |  \
 * |___ |__| _/\_ | |__] |  \ | |__/
 * =================================
 * Luxigrid - Snakes Animation
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

#ifndef SNAKES_GUARD
#define SNAKES_GUARD

#include "Arduino.h"
#include "../../lib/luxigrid.h"

#include "../../lib/animation-helpers.hpp"

static const byte SNAKE_LENGTH = 16;
CRGB colors[SNAKE_LENGTH];
uint8_t initialHue;

enum Direction {
	UP,
	DOWN,
	LEFT,
	RIGHT
};

struct Pixel {
	uint8_t x;
	uint8_t y;
};

struct Snake {
	Pixel pixels[SNAKE_LENGTH];
	Direction direction;

	void newDirection() {
		switch (direction) {
			case UP:
			case DOWN:
				direction = random(0, 2) == 1 ? RIGHT : LEFT;
				break;
			case LEFT:
			case RIGHT:
				direction = random(0, 2) == 1 ? DOWN : UP;
			default:
				break;
		}
	}

	void shuffleDown() {
		for (byte i = SNAKE_LENGTH - 1; i > 0; i--) {
			pixels[i] = pixels[i - 1];
		}
	}

	void reset() {
		direction = UP;

		for (int i = 0; i < SNAKE_LENGTH; i++) {
			pixels[i].x = 0;
			pixels[i].y = 0;
		}
	}

	void move() {
		switch (direction) {
			case UP:
				pixels[0].y = (pixels[0].y + 1) % MATRIX_HEIGHT;
				break;
			case LEFT:
				pixels[0].x = (pixels[0].x + 1) % MATRIX_WIDTH;
				break;
			case DOWN:
				pixels[0].y = pixels[0].y == 0 ? MATRIX_HEIGHT - 1 : pixels[0].y - 1;
				break;
			case RIGHT:
				pixels[0].x = pixels[0].x == 0 ? MATRIX_WIDTH - 1 : pixels[0].x - 1;
				break;
		}
	}

	void draw(CRGB colors[SNAKE_LENGTH]) {
		for (byte i = 0; i < SNAKE_LENGTH; i++) {
			// Original
			// leds[XY16(pixels[i].x, pixels[i].y)] = colors[i] %= (255 - i * (255 / SNAKE_LENGTH));
			// Longer Trails
			// leds[XY16(pixels[i].x, pixels[i].y)] = colors[i] %= (255 - i * (255 / SNAKE_LENGTH / 1.5));
			// Compromise
			leds[XY16(pixels[i].x, pixels[i].y)] = colors[i] %= (255 - i * (10));
		}
	}
};

static const int snakeCount = 8;
Snake snakes[snakeCount];

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

	for (int i = 0; i < snakeCount; i++) {
		Snake *snake = &snakes[i];
		snake->reset();
	}
}

///////////////////
// MAIN LOOP
///////////////////
void loop() {
	if (1000 / default_fps + last_frame < millis()) {
		fill_palette(colors, SNAKE_LENGTH, initialHue++, 5, currentPalette, 255, LINEARBLEND);

		for (int i = 0; i < snakeCount; i++) {
			Snake *snake = &snakes[i];
			snake->shuffleDown();

			if (random(10) > 8) {
				snake->newDirection();
			}

			snake->move();
			snake->draw(colors);
		}

		updateScreen();
		last_frame = millis();
	}
}

#endif