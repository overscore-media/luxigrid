/* _    _  _ _  _ _ ____ ____ _ ___
 * |    |  |  \/  | | __ |__/ | |  \
 * |___ |__| _/\_ | |__] |  \ | |__/
 * =================================
 * Luxigrid - Code Rain Animation
 * Copyright (c) 2024 OverScore Media - MIT License
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

#ifndef CODE_RAIN_GUARD
#define CODE_RAIN_GUARD

#include "Arduino.h"
#include "../../lib/luxigrid.h"

// Each raindrop is one pixel, and each has a "tail" of progressively-dimmer pixels behind it as it falls
// There's one for each vertical column of the screen (in this case 64)
int raindrops[64];
int tailLengths[64];
uint16_t raindropColours[64];

// The array of possible colours for raindrops
uint16_t colours[3] = {
    dma_display->color565(100, 255, 100),
    dma_display->color565(0, 255, 0),
    dma_display->color565(0, 50, 0),
};

///////////////////
// SETUP FUNCTION
///////////////////
void setup() {
	setupMatrix();

	// Start a collection of raindrops at random vertical positions around the screen
	// Give them tails of varying lengths, and set the raindrops' colours to one of the three possibilities
	for (int i = 0; i < PANEL_RES_X; i++) {
		raindrops[i] = random(PANEL_RES_Y);
		tailLengths[i] = random(5, 12);
		raindropColours[i] = colours[random(3)];
	}
}

///////////////////
// MAIN LOOP
///////////////////
void loop() {
	for (int x = 0; x < PANEL_RES_X; x++) {
		// Increment the position of the raindrop (i.e., move it down)
		raindrops[x]++;

		// Check if the raindrop (including its tail) has moved off the bottom of the screen
		if (raindrops[x] - tailLengths[x] >= PANEL_RES_Y) {
			raindrops[x] = -tailLengths[x];
			tailLengths[x] = random(5, 12);
			raindropColours[x] = colours[random(3)];
		}

		// Draw the head of the raindrop (if it's within the screen bounds)
		if (raindrops[x] >= 0 && raindrops[x] < PANEL_RES_Y) {
			dma_display->drawPixel(x, raindrops[x], raindropColours[x]);
		}

		// Draw the tail of the raindrop
		for (int j = 1; j <= tailLengths[x]; j++) {
			int tailY = raindrops[x] - j;

			// The end of the tail gets dimmer the further away it is from raindrop pixel itself
			if (tailY >= 0 && tailY < PANEL_RES_Y) {
				uint8_t brightness = 255 - (j * (255 / tailLengths[x]));
				dma_display->drawPixel(x, tailY, dma_display->color565(0, brightness, 0));
			}
		}
	}

	delay(1000 / 30);
}

#endif