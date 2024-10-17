/* _    _  _ _  _ _ ____ ____ _ ___
 * |    |  |  \/  | | __ |__/ | |  \
 * |___ |__| _/\_ | |__] |  \ | |__/
 * =================================
 * Luxigrid - Hue Value Spectrum Animation
 * Copyright (c) 2024 OverScore Media - MIT License
 *
 * Adapted from ESP32-HUB75-MatrixPanel-DMA:
 * https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA/blob/master/examples/HueValueSpectrum/HueValueSpectrum.ino
 * Copyright (c) 2018-2032 Faptastic - MIT License
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

#ifndef HUE_VALUE_SPECTRUM_GUARD
#define HUE_VALUE_SPECTRUM_GUARD

#include "Arduino.h"
#include "../../lib/luxigrid.h"

unsigned long fps_timer;
unsigned int default_fps = 30;
unsigned long last_frame = 0, ms_previous = 0;

float r, g, b;
float pixR, pixG, pixB;

///////////////////
// SETUP FUNCTION
///////////////////
void setup() {
	setupMatrix();
}

///////////////////
// MAIN LOOP
///////////////////
void loop() {
	float t = (float)((millis() % 4000) / 4000.f);
	float tt = (float)((millis() % 16000) / 16000.f);

	for (int x = 0; x < (PANEL_RES_X * PANEL_CHAIN); x++) {
		// Calculate the overal shade
		float f = (((sin(tt - (float)x / PANEL_RES_Y / 32.0) * 2.f * PI) + 1) / 2) * 255;

		// Convert HSV to RGB
		r = max(min(cosf(2.f * PI * (t + ((float)x / PANEL_RES_Y + 0.f) / 3.f)) + 0.5f, 1.f), 0.f);
		g = max(min(cosf(2.f * PI * (t + ((float)x / PANEL_RES_Y + 1.f) / 3.f)) + 0.5f, 1.f), 0.f);
		b = max(min(cosf(2.f * PI * (t + ((float)x / PANEL_RES_Y + 2.f) / 3.f)) + 0.5f, 1.f), 0.f);

		// Iterate pixels for every row
		for (int y = 0; y < PANEL_RES_Y; y++) {
			if (y * 2 < PANEL_RES_Y) {
				// Top-middle part of screen, transition of value
				float t = (2.f * y + 1) / PANEL_RES_Y;

				pixR = (r * t) * f;
				pixG = (g * t) * f;
				pixB = (b * t) * f;
			} else {
				// Middle to bottom of screen, transition of saturation
				float t = (2.f * (PANEL_RES_Y - y) - 1) / PANEL_RES_Y;

				pixR = (r * t + 1 - t) * f;
				pixG = (g * t + 1 - t) * f;
				pixB = (b * t + 1 - t) * f;
			}

			dma_display->drawPixelRGB888(x, y, pixR, pixG, pixB);
		}
	}
}

#endif