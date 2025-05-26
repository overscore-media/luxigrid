/* _    _  _ _  _ _ ____ ____ _ ___
 * |    |  |  \/  | | __ |__/ | |  \
 * |___ |__| _/\_ | |__] |  \ | |__/
 * =================================
 * Luxigrid - TV Test Pattern Animation
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

#ifndef TV_TEST_PATTERN_GUARD
#define TV_TEST_PATTERN_GUARD

#include "Arduino.h"
#include "../../lib/luxigrid.h"

const uint8_t barWidth = MATRIX_WIDTH / 8;

const uint8_t cornerLength = 5;
const uint8_t cornerWidth = 1;

const uint16_t red = dma_display->color444(15, 0, 0);
const uint16_t yellow = dma_display->color444(15, 15, 0);
const uint16_t green = dma_display->color444(0, 15, 0);
const uint16_t cyan = dma_display->color444(0, 15, 15);
const uint16_t blue = dma_display->color444(0, 0, 15);
const uint16_t magenta = dma_display->color444(15, 0, 15);
const uint16_t white = dma_display->color444(15, 15, 15);
const uint16_t black = dma_display->color444(0, 0, 0);

const uint8_t numScanlines = 4;
const uint8_t scanlineFadeStart = 20;

uint8_t scanLinesY = 0;
uint8_t scanLinesPreviousY = -1;

uint16_t getBackgroundColour(int x, int y) {
	// Check if the pixel is in any of the corner areas
	if ((x < cornerLength && y < cornerWidth) || (x < cornerWidth && y < cornerLength) ||                                                                    // Top-left
	    (x >= MATRIX_WIDTH - cornerLength && y < cornerWidth) || (x >= MATRIX_WIDTH - cornerWidth && y < cornerLength) ||                                    // Top-right
	    (x < cornerLength && y >= MATRIX_HEIGHT - cornerWidth) || (x < cornerWidth && y >= MATRIX_HEIGHT - cornerLength) ||                                  // Bottom-left
	    (x >= MATRIX_WIDTH - cornerLength && y >= MATRIX_HEIGHT - cornerWidth) || (x >= MATRIX_WIDTH - cornerWidth && y >= MATRIX_HEIGHT - cornerLength)) {  // Bottom-right
		return white;
	}

	// Colour bars
	switch (x / barWidth) {
		case 0:
			return red;
		case 1:
			return yellow;
		case 2:
			return green;
		case 3:
			return cyan;
		case 4:
			return blue;
		case 5:
			return magenta;
		case 6:
			return white;
		case 7:
			return black;
		default:
			return black;
	}
}

void drawScanLines(int yPosition, int previousYPosition) {
	for (int i = 0; i < numScanlines; i++) {
		int y = yPosition - i;
		int yPrev = previousYPosition - i;

		if (y < 0) {
			y += MATRIX_HEIGHT;
		}

		if (yPrev < 0) {
			yPrev += MATRIX_HEIGHT;
		}

		// Redraw the background behind the old scanline
		for (int x = 0; x < MATRIX_WIDTH; x++) {
			uint16_t backgroundColour = getBackgroundColour(x, yPrev);
			dma_display->drawPixel(x, yPrev, backgroundColour);
		}

		// Draw the new scan line
		for (int x = 0; x < MATRIX_WIDTH; x++) {
			uint16_t backgroundColour = getBackgroundColour(x, y);

			uint8_t r = (backgroundColour >> 11) & 0x1F;
			uint8_t g = (backgroundColour >> 5) & 0x3F;
			uint8_t b = backgroundColour & 0x1F;

			int fadeAdjustment = max(0, scanlineFadeStart - i * 5);

			r = min(31, r + fadeAdjustment);
			g = min(63, g + fadeAdjustment);
			b = min(31, b + fadeAdjustment);

			uint16_t newColour = (r << 11) | (g << 5) | b;
			dma_display->drawPixel(x, y, newColour);
		}
	}
}

///////////////////
// SETUP FUNCTION
///////////////////
void setup() {
	setupMatrix();

	// Draw colour bars
	dma_display->fillRect(0 * barWidth, 0, barWidth, MATRIX_HEIGHT, red);
	dma_display->fillRect(1 * barWidth, 0, barWidth, MATRIX_HEIGHT, yellow);
	dma_display->fillRect(2 * barWidth, 0, barWidth, MATRIX_HEIGHT, green);
	dma_display->fillRect(3 * barWidth, 0, barWidth, MATRIX_HEIGHT, cyan);
	dma_display->fillRect(4 * barWidth, 0, barWidth, MATRIX_HEIGHT, blue);
	dma_display->fillRect(5 * barWidth, 0, barWidth, MATRIX_HEIGHT, magenta);
	dma_display->fillRect(6 * barWidth, 0, barWidth, MATRIX_HEIGHT, white);
	dma_display->fillRect(7 * barWidth, 0, barWidth, MATRIX_HEIGHT, black);

	// Top-left corner
	dma_display->fillRect(0, 0, cornerLength, cornerWidth, white);
	dma_display->fillRect(0, 0, cornerWidth, cornerLength, white);

	// Top-right corner
	dma_display->fillRect(MATRIX_WIDTH - cornerLength, 0, cornerLength, cornerWidth, white);
	dma_display->fillRect(MATRIX_WIDTH - cornerWidth, 0, cornerWidth, cornerLength, white);

	// Bottom-left corner
	dma_display->fillRect(0, MATRIX_HEIGHT - cornerWidth, cornerLength, cornerWidth, white);
	dma_display->fillRect(0, MATRIX_HEIGHT - cornerLength, cornerWidth, cornerLength, white);

	// Bottom-right corner
	dma_display->fillRect(MATRIX_WIDTH - cornerLength, MATRIX_HEIGHT - cornerWidth, cornerLength, cornerWidth, white);
	dma_display->fillRect(MATRIX_WIDTH - cornerWidth, MATRIX_HEIGHT - cornerLength, cornerWidth, cornerLength, white);
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

	drawScanLines(scanLinesY, scanLinesPreviousY);

	// Move the scanline down; reset them when they reach the bottom of the screen
	scanLinesPreviousY = scanLinesY;
	scanLinesY++;

	if (scanLinesY >= MATRIX_HEIGHT) {
		scanLinesY = 0;
	}

	delay(50);
}

#endif