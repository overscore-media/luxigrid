/* _    _  _ _  _ _ ____ ____ _ ___
 * |    |  |  \/  | | __ |__/ | |  \
 * |___ |__| _/\_ | |__] |  \ | |__/
 * =================================
 * Luxigrid - Julia Set Animation
 * Copyright (c) 2024 OverScore Media - MIT License
 *
 * Adapted from ESP32-HUB75-MatrixPanel-DMA:
 * https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA/blob/master/examples/Julia_Set_Demo/Julia_Set_Demo.ino
 * Copyright (c) 2018-2032 Faptastic - MIT License
 *
 * Inspired by https://en.wikipedia.org/wiki/Fast_inverse_square_root
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

#ifndef JULIA_SET_GUARD
#define JULIA_SET_GUARD

#include "Arduino.h"
#include "../../lib/luxigrid.h"

// Cast float as int32_t
int32_t intfloat(float n) {
	return *(int32_t *)&n;
}

// Cast int32_t as float
float floatint(int32_t n) {
	return *(float *)&n;
}

// Fast approx sqrt(x)
float floatsqrt(float n) {
	return floatint(0x1fbb4000 + (intfloat(n) >> 1));
}

// Fast approx 1/x
float floatinv(float n) {
	return floatint(0x7f000000 - intfloat(n));
}

// Fast approx log2(x)
float floatlog2(float n) {
	return (float)((intfloat(n) << 1) - 0x7f000000) * 5.9604645e-08f;
}

// Escape time mandelbrot set function,
// with arbitrary start point zx, zy
// and arbitrary seed point ax, ay
//
// For julia set
// zx = pos_x,  zy = pos_y;
// ax = seed_x, ay = seed_y;
//
// For mandelbrot set
// zx = 0, zy = 0;
// ax = pos_x,  ay = pos_y;

// Escape radius
const float bailOut = 4;

// Colour speed
const int32_t itmult = 1 << 10;

// Precalculated sin table, for performance reasons
float sint[256];

// https://en.wikipedia.org/wiki/Mandelbrot_set
int32_t iteratefloat(float ax, float ay, float zx, float zy, uint16_t mxIT) {
	float zzl = 0;

	for (int it = 0; it < mxIT; it++) {
		float zzx = zx * zx;
		float zzy = zy * zy;

		// Is the point escaped?
		if (zzx + zzy >= bailOut) {
			if (it > 0) {
				// Calculate smooth colouring
				float zza = floatlog2(zzl);
				float zzb = floatlog2(zzx + zzy);
				float zzc = floatlog2(bailOut);
				float zzd = (zzc - zza) * floatinv(zzb - zza);

				return it * itmult + zzd * itmult;
			}
		};

		zy = 2.f * zx * zy + ay;
		zx = zzx - zzy + ax;
		zzl = zzx + zzy;
	}

	return 0;
}

// Palette Colour taken from:
// https://editor.p5js.org/Kouzerumatsukite/sketches/DwTiq9D01
// color palette originally made by piano_miles, written in p5js
// hsv2rgb(IT, cos(4096*it)/2+0.5, 1-sin(2048*it)/2-0.5)
void drawPixelPalette(int x, int y, uint32_t m) {
	float r = 0.f, g = 0.f, b = 0.f;

	if (m) {
		char n = m >> 4;
		float l = abs(sint[m >> 2 & 255]) * 255.f;
		float s = (sint[m & 255] + 1.f) * 0.5f;

		r = (max(min(sint[n & 255] + 0.5f, 1.f), 0.f) * s + (1 - s)) * l;
		g = (max(min(sint[n + 85 & 255] + 0.5f, 1.f), 0.f) * s + (1 - s)) * l;
		b = (max(min(sint[n + 170 & 255] + 0.5f, 1.f), 0.f) * s + (1 - s)) * l;
	}

	dma_display->drawPixelRGB888(x, y, r, g, b);
}

void drawCanvas() {
	uint32_t lastMicros = micros();

	double t = (double)lastMicros / 8000000;
	double k = sin(t * 3.212 / 2) * sin(t * 3.212 / 2) / 16 + 1;
	float cosk = (k - cos(t)) / 2;
	float xoff = (cos(t) * cosk + k / 2 - 0.25);
	float yoff = (sin(t) * cosk);

	for (uint8_t y = 0; y < PANEL_RES_Y; y++) {
		for (uint8_t x = 0; x < PANEL_RES_X; x++) {
			uint32_t itcount = iteratefloat(xoff, yoff, ((x - 64) + 1) / 64.f, (y) / 64.f, 64);
			uint32_t itcolor = itcount ? floatsqrt(itcount) * 4 + t * 1024 : 0;

			drawPixelPalette(x, y, itcolor);
		}
	}
}

///////////////////
// SETUP FUNCTION
///////////////////
void setup() {
	setupMatrix();

	// Precalculate the sine table
	for (int i = 0; i < 256; i++) {
		sint[i] = sinf(i / 256.f * 2.f * PI);
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

	drawCanvas();
	delay(1);
}

#endif