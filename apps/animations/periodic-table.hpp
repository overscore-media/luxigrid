/* _    _  _ _  _ _ ____ ____ _ ___
 * |    |  |  \/  | | __ |__/ | |  \
 * |___ |__| _/\_ | |__] |  \ | |__/
 * =================================
 * Luxigrid - Periodic Table Animation
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

#ifndef PERIODIC_TABLE_GUARD
#define PERIODIC_TABLE_GUARD

#include "Arduino.h"
#include "../../lib/luxigrid.h"

#include "../../lib/elements.hpp"

int numElements = 118;

// Background colours; will be selected based on the group of the element
uint16_t red = dma_display->color565(75, 0, 0);
uint16_t yellow = dma_display->color565(75, 75, 0);
uint16_t blue = dma_display->color565(0, 75, 0);
uint16_t green = dma_display->color565(0, 75, 0);

// The time, in milliseconds, to display each element
const int elementDelay = 3000;

void displayElement(const Element& element) {
	dma_display->clearScreen();

	// Set the background colour based on the group (inspired by Wikipedia's colouring scheme)
	dma_display->fillScreen(element.group == 's' ? red : element.group == 'p' ? yellow
	                                                 : element.group == 'd'   ? blue
	                                                                          : green);

	// Print the symbol
	dma_display->setTextSize(2);
	dma_display->setCursor(2, 9);
	dma_display->print(element.symbol);

	int16_t x1, y1;
	uint16_t w, h;

	// Print the atomic number
	dma_display->setTextSize(1);
	dma_display->getTextBounds(String(element.number), 0, 0, &x1, &y1, &w, &h);
	dma_display->setCursor(64 - w - 2, 5);
	dma_display->print(element.number);

	// Print the element's name
	dma_display->getTextBounds(element.name, 0, 0, &x1, &y1, &w, &h);
	dma_display->setCursor((64 - w) / 2, 22);
	dma_display->print(element.name);

	// Print the atomic weight
	dma_display->getTextBounds(element.atomicWeight, 0, 0, &x1, &y1, &w, &h);
	dma_display->setCursor((64 - w) / 2, 29);
	dma_display->print(element.atomicWeight);
}

///////////////////
// SETUP FUNCTION
///////////////////
void setup() {
	setupMatrix();

	dma_display->setFont(&Org_01);
	dma_display->setTextColor(dma_display->color565(255, 255, 255));
	dma_display->setTextSize(1);
}

///////////////////
// MAIN LOOP
///////////////////
void loop() {
	for (int i = 0; i < numElements; i++) {
		displayElement(elements[i]);
		delay(elementDelay);
	}
}

#endif