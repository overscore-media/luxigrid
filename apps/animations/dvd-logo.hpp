/* _    _  _ _  _ _ ____ ____ _ ___
 * |    |  |  \/  | | __ |__/ | |  \
 * |___ |__| _/\_ | |__] |  \ | |__/
 * =================================
 * Luxigrid - DVD Logo Animation
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

#ifndef DVD_LOGO_GUARD
#define DVD_LOGO_GUARD

#include "Arduino.h"
#include "../../lib/luxigrid.h"

// The position and velocity of the logo
uint8_t x, y, deltaX, deltaY;

// The colour components of the logo at any given time
uint8_t r, g, b;

// This will be true if the logo hits a corner of the screen, until it hits the edge of the screen again
bool hitCorner = false;

// Generated from pixel art made in Aseprite, using https://javl.github.io/image2cpp/
const unsigned char dvd_logo[] = {0xca, 0xc0, 0xaa, 0xa0, 0xae, 0xa0, 0xc4, 0xc0, 0x00, 0x00, 0x7f, 0xc0};

// The logo's rectangular bounding box
const uint8_t width = 11;
const uint8_t height = 6;

// Pick a new colour value for the logo
// Not all colours are possible; those under rgb(100, 100, 100) aren't vibrant enough
void randomizeColours() {
	r = random(100, 256);
	g = random(100, 256);
	b = random(100, 256);
}

///////////////////
// SETUP FUNCTION
///////////////////
void setup() {
	setupMatrix();

	// Start the logo somewhere on the screen
	x = random(0, PANEL_RES_X - width - 2);
	y = random(0, PANEL_RES_Y - height - 2);

	// Randomly set deltaX and deltaY to either 1 or -1
	deltaX = random(2) * 2 - 1;
	deltaY = random(2) * 2 - 1;
}

///////////////////
// MAIN LOOP
///////////////////
void loop() {
	dma_display->clearScreen();

	// Make a rainbow effect after hitting a corner of the screen
	if (hitCorner) {
		randomizeColours();
	}

	x += deltaX;
	y += deltaY;

	bool hitEdge = false;

	// Check collision with horizontal edges
	if (x <= 0 || x >= PANEL_RES_X - width) {
		deltaX = -deltaX;
		hitEdge = true;
	}

	// Check collision with vertical edges
	if (y <= 0 || y >= PANEL_RES_Y - height) {
		deltaY = -deltaY;
		hitEdge = true;
	}

	if (hitEdge) {
		randomizeColours();
		// Only check for a corner hit if an edge hit has already been registered
		hitCorner = (x <= 0 || x >= PANEL_RES_X - width) && (y <= 0 || y >= PANEL_RES_Y - height);
	}

	// Render the logo in its new position
	dma_display->drawBitmap(x, y, dvd_logo, 11, 6, dma_display->color565(r, g, b));
	delay(50);
}

#endif