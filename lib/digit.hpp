/* _    _  _ _  _ _ ____ ____ _ ___
 * |    |  |  \/  | | __ |__/ | |  \
 * |___ |__| _/\_ | |__] |  \ | |__/
 * =================================
 * Luxigrid - Digit Helper Functions
 * Copyright (c) 2024 OverScore Media - MIT License
 *
 * Adapted from https://github.com/dragondaud/myClock
 * Copyright (c) 2018 David M Denney <dragondaud@gmail.com> - MIT License
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

#ifndef DIGIT_GUARD
#define DIGIT_GUARD

#include "Arduino.h"
#include "luxigrid.h"

class Digit {
	public:
	Digit();
	Digit(byte value, uint16_t xo, uint16_t yo, uint16_t color);
	void init(byte value, uint16_t xo, uint16_t yo, uint16_t color);
	void Draw(byte value, uint16_t c);
	void Morph(byte newValue);
	byte Value();
	void DrawColon(uint16_t c);

	private:
	byte _value;
	uint16_t _color, _bg, xOffset, yOffset;
	int animSpeed = 30;
	void drawPixel(uint16_t x, uint16_t y, uint16_t c);
	void drawFillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t c);
	void drawLine(uint16_t x, uint16_t y, uint16_t x2, uint16_t y2, uint16_t c);
	void drawSeg(byte seg);
	void Morph2();
	void Morph3();
	void Morph4();
	void Morph5();
	void Morph6();
	void Morph7();
	void Morph8();
	void Morph9();
	void Morph0();
	void Morph1();
};

const byte sA = 0;
const byte sB = 1;
const byte sC = 2;
const byte sD = 3;
const byte sE = 4;
const byte sF = 5;
const byte sG = 6;
const int segHeight = 6;
const int segWidth = segHeight;
const uint16_t height = 31;
const uint16_t width = 63;

byte digitBits[] = {
    0b11111100,  // 0 ABCDEF--
    0b01100000,  // 1 -BC-----
    0b11011010,  // 2 AB-DE-G-
    0b11110010,  // 3 ABCD--G-
    0b01100110,  // 4 -BC--FG-
    0b10110110,  // 5 A-CD-FG-
    0b10111110,  // 6 A-CDEFG-
    0b11100000,  // 7 ABC-----
    0b11111110,  // 8 ABCDEFG-
    0b11110110,  // 9 ABCD_FG-
};

Digit::Digit() : _value(0), xOffset(0), yOffset(0), _color(0), _bg(dma_display->color565(0, 0, 0)) {}

Digit::Digit(byte value, uint16_t xo, uint16_t yo, uint16_t color) {
	_value = value;
	xOffset = xo;
	yOffset = yo;
	_color = color;
	_bg = dma_display->color565(0, 0, 0);
}

void Digit::init(byte value, uint16_t xo, uint16_t yo, uint16_t color) {
	_value = value;
	xOffset = xo;
	yOffset = yo;
	_color = color;
	_bg = dma_display->color565(0, 0, 0);
}

byte Digit::Value() {
	return _value;
}

void Digit::drawPixel(uint16_t x, uint16_t y, uint16_t c) {
	dma_display->drawPixel(xOffset + x, height - (y + yOffset), c);
}

void Digit::drawLine(uint16_t x, uint16_t y, uint16_t x2, uint16_t y2, uint16_t c) {
	dma_display->drawLine(xOffset + x, height - (y + yOffset), xOffset + x2, height - (y2 + yOffset), c);
}

void Digit::drawFillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t c) {
	dma_display->fillRect(xOffset + x, height - (y + yOffset), w, h, c);
}

// The colon is drawn to the left of this digit
void Digit::DrawColon(uint16_t c) {
	drawFillRect(-3, segHeight - 1, 2, 2, c);
	drawFillRect(-3, segHeight + 1 + 3, 2, 2, c);
}

void Digit::drawSeg(byte seg) {
	switch (seg) {
		case sA:
			drawLine(1, segHeight * 2 + 2, segWidth, segHeight * 2 + 2, _color);
			break;
		case sB:
			drawLine(segWidth + 1, segHeight * 2 + 1, segWidth + 1, segHeight + 2, _color);
			break;
		case sC:
			drawLine(segWidth + 1, 1, segWidth + 1, segHeight, _color);
			break;
		case sD:
			drawLine(1, 0, segWidth, 0, _color);
			break;
		case sE:
			drawLine(0, 1, 0, segHeight, _color);
			break;
		case sF:
			drawLine(0, segHeight * 2 + 1, 0, segHeight + 2, _color);
			break;
		case sG:
			drawLine(1, segHeight + 1, segWidth, segHeight + 1, _color);
			break;
	}
}

void Digit::Draw(byte value, uint16_t c) {
	_color = c;
	byte pattern = digitBits[value];

	if (bitRead(pattern, 7)) drawSeg(sA);
	if (bitRead(pattern, 6)) drawSeg(sB);
	if (bitRead(pattern, 5)) drawSeg(sC);
	if (bitRead(pattern, 4)) drawSeg(sD);
	if (bitRead(pattern, 3)) drawSeg(sE);
	if (bitRead(pattern, 2)) drawSeg(sF);
	if (bitRead(pattern, 1)) drawSeg(sG);

	_value = value;
}

void Digit::Morph0() {
	for (int i = 0; i <= segWidth; i++) {
		// If 1 to 0, slide B to F and E to C
		if (_value == 1) {
			// Slide B to F
			drawLine(segWidth - i, segHeight * 2 + 1, segWidth - i, segHeight + 2, _color);
			if (i > 0) drawLine(segWidth - i + 1, segHeight * 2 + 1, segWidth - i + 1, segHeight + 2, _bg);

			// Slide E to C
			drawLine(segWidth - i, 1, segWidth - i, segHeight, _color);
			if (i > 0) drawLine(segWidth - i + 1, 1, segWidth - i + 1, segHeight, _bg);

			if (i < segWidth) drawPixel(segWidth - i, segHeight * 2 + 2, _color);  // Draw A
			if (i < segWidth) drawPixel(segWidth - i, 0, _color);                  // Draw D
		}

		// If 2 to 0, slide B to F and Flow G to C
		if (_value == 2) {
			// Slide B to F
			drawLine(segWidth - i, segHeight * 2 + 1, segWidth - i, segHeight + 2, _color);
			if (i > 0) drawLine(segWidth - i + 1, segHeight * 2 + 1, segWidth - i + 1, segHeight + 2, _bg);

			drawPixel(1 + i, segHeight + 1, _bg);                                  // Erase G left to right
			if (i < segWidth) drawPixel(segWidth + 1, segHeight + 1 - i, _color);  // Draw C
		}

		// B to F, C to E
		if (_value == 3) {
			// Slide B to F
			drawLine(segWidth - i, segHeight * 2 + 1, segWidth - i, segHeight + 2, _color);
			if (i > 0) drawLine(segWidth - i + 1, segHeight * 2 + 1, segWidth - i + 1, segHeight + 2, _bg);

			// Move C to E
			drawLine(segWidth - i, 1, segWidth - i, segHeight, _color);
			if (i > 0) drawLine(segWidth - i + 1, 1, segWidth - i + 1, segHeight, _bg);

			// Erase G from right to left
			drawPixel(segWidth - i, segHeight + 1, _bg);  // G
		}

		// If 5 to 0, we also need to slide F to B
		if (_value == 5) {
			if (i < segWidth) {
				if (i > 0) drawLine(1 + i, segHeight * 2 + 1, 1 + i, segHeight + 2, _bg);
				drawLine(2 + i, segHeight * 2 + 1, 2 + i, segHeight + 2, _color);
			}
		}

		// If 9 or 5 to 0, Flow G into E
		if (_value == 5 || _value == 9) {
			if (i < segWidth) drawPixel(segWidth - i, segHeight + 1, _bg);
			if (i < segWidth) drawPixel(0, segHeight - i, _color);
		}

		delay(animSpeed);
	}
}

// Zero or two to one
void Digit::Morph1() {
	for (int i = 0; i <= (segWidth + 1); i++) {
		// Move E left to right
		drawLine(0 + i - 1, 1, 0 + i - 1, segHeight, _bg);
		drawLine(0 + i, 1, 0 + i, segHeight, _color);

		// Move F left to right
		drawLine(0 + i - 1, segHeight * 2 + 1, 0 + i - 1, segHeight + 2, _bg);
		drawLine(0 + i, segHeight * 2 + 1, 0 + i, segHeight + 2, _color);

		// Gradually Erase A, G, D
		drawPixel(1 + i, segHeight * 2 + 2, _bg);  // A
		drawPixel(1 + i, 0, _bg);                  // D
		drawPixel(1 + i, segHeight + 1, _bg);      // G

		delay(animSpeed);
	}
}

void Digit::Morph2() {
	for (int i = 0; i <= segWidth; i++) {
		if (i < segWidth) {
			drawPixel(segWidth - i, segHeight * 2 + 2, _color);
			drawPixel(segWidth - i, segHeight + 1, _color);
			drawPixel(segWidth - i, 0, _color);
		}

		drawLine(segWidth + 1 - i, 1, segWidth + 1 - i, segHeight, _bg);
		drawLine(segWidth - i, 1, segWidth - i, segHeight, _color);
		delay(animSpeed);
	}
}

void Digit::Morph3() {
	for (int i = 0; i <= segWidth; i++) {
		drawLine(0 + i, 1, 0 + i, segHeight, _bg);
		drawLine(1 + i, 1, 1 + i, segHeight, _color);
		delay(animSpeed);
	}
}

void Digit::Morph4() {
	for (int i = 0; i < segWidth; i++) {
		drawPixel(segWidth - i, segHeight * 2 + 2, _bg);  // Erase A
		drawPixel(0, segHeight * 2 + 1 - i, _color);      // Draw as F
		drawPixel(1 + i, 0, _bg);                         // Erase D
		delay(animSpeed);
	}
}

void Digit::Morph5() {
	for (int i = 0; i < segWidth; i++) {
		drawPixel(segWidth + 1, segHeight + 2 + i, _bg);     // Erase B
		drawPixel(segWidth - i, segHeight * 2 + 2, _color);  // Draw as A
		drawPixel(segWidth - i, 0, _color);                  // Draw D
		delay(animSpeed);
	}
}

void Digit::Morph6() {
	for (int i = 0; i <= segWidth; i++) {
		// Move C right to left
		drawLine(segWidth - i, 1, segWidth - i, segHeight, _color);
		if (i > 0) drawLine(segWidth - i + 1, 1, segWidth - i + 1, segHeight, _bg);
		delay(animSpeed);
	}
}

void Digit::Morph7() {
	for (int i = 0; i <= (segWidth + 1); i++) {
		// Move E left to right
		drawLine(0 + i - 1, 1, 0 + i - 1, segHeight, _bg);
		drawLine(0 + i, 1, 0 + i, segHeight, _color);

		// Move F left to right
		drawLine(0 + i - 1, segHeight * 2 + 1, 0 + i - 1, segHeight + 2, _bg);
		drawLine(0 + i, segHeight * 2 + 1, 0 + i, segHeight + 2, _color);

		// Erase D and G gradually
		drawPixel(1 + i, 0, _bg);              // D
		drawPixel(1 + i, segHeight + 1, _bg);  // G
		delay(animSpeed);
	}
}

void Digit::Morph8() {
	for (int i = 0; i <= segWidth; i++) {
		// Move B right to left
		drawLine(segWidth - i, segHeight * 2 + 1, segWidth - i, segHeight + 2, _color);
		if (i > 0) drawLine(segWidth - i + 1, segHeight * 2 + 1, segWidth - i + 1, segHeight + 2, _bg);

		// Move C right to left
		drawLine(segWidth - i, 1, segWidth - i, segHeight, _color);
		if (i > 0) drawLine(segWidth - i + 1, 1, segWidth - i + 1, segHeight, _bg);

		// Gradually draw D and G
		if (i < segWidth) {
			drawPixel(segWidth - i, 0, _color);              // D
			drawPixel(segWidth - i, segHeight + 1, _color);  // G
		}
		delay(animSpeed);
	}
}

void Digit::Morph9() {
	for (int i = 0; i <= (segWidth + 1); i++) {
		// Move E left to right
		drawLine(0 + i - 1, 1, 0 + i - 1, segHeight, _bg);
		drawLine(0 + i, 1, 0 + i, segHeight, _color);
		delay(animSpeed);
	}
}

void Digit::Morph(byte newValue) {
	switch (newValue) {
		case 0:
			Morph0();
			break;
		case 1:
			Morph1();
			break;
		case 2:
			Morph2();
			break;
		case 3:
			Morph3();
			break;
		case 4:
			Morph4();
			break;
		case 5:
			Morph5();
			break;
		case 6:
			Morph6();
			break;
		case 7:
			Morph7();
			break;
		case 8:
			Morph8();
			break;
		case 9:
			Morph9();
			break;
	}

	_value = newValue;
}

#endif