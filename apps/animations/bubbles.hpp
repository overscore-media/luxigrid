/* _    _  _ _  _ _ ____ ____ _ ___
 * |    |  |  \/  | | __ |__/ | |  \
 * |___ |__| _/\_ | |__] |  \ | |__/
 * =================================
 * Luxigrid - Bubbles Animation
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

#ifndef BUBBLES_GUARD
#define BUBBLES_GUARD

#include "Arduino.h"
#include "../../lib/luxigrid.h"

#define NUM_BUBBLES 10
#define MAX_SIZE 10

class Bubble {
	public:
	float x, y;
	float dx, dy;
	int size;
	uint16_t color;
	bool exists;
	int popCounter;

	Bubble() : x(0), y(0), dx(0), dy(0), size(0), color(0), exists(false), popCounter(0) {}

	void init(float startX, float startY, float velX, float velY, int bubbleSize, uint16_t bubbleColor) {
		x = startX;
		y = startY;
		dx = velX;
		dy = velY;
		size = bubbleSize;
		color = bubbleColor;
		exists = true;
		popCounter = 0;
	}

	void move() {
		if (!exists) {
			return;
		}

		// If the bubble is in the process of popping
		if (popCounter > 0) {
			pop();
			return;
		}

		// Add velocity
		x += dx;
		y += dy;

		// Boundary collision handling
		if ((x <= size) || (x >= 64 - size)) {
			x = (x <= size) ? size : 64 - size;
			dx = -dx;
		}

		if ((y <= size) || (y >= 32 - size)) {
			y = (y <= size) ? size : 32 - size;
			dy = -dy;
		}

		// Random velocity adjustment
		dx += random(-100, 101) / 1000.0;
		dy += random(-100, 101) / 1000.0;

		// Speed limit enforcement
		float speedLimit = 1.5;
		float currentSpeed = sqrt(dx * dx + dy * dy);
		if (currentSpeed > speedLimit) {
			dx *= speedLimit / currentSpeed;
			dy *= speedLimit / currentSpeed;
		}

		// Random chance to start popping (0.3% chance to pop each frame - up to 5 frames to pop)
		if (random(1000) < 3) {
			popCounter = 5;
		}
	}

	// Shrink the size of the bubble until it's gone
	void pop() {
		size -= 1;

		if (size <= 0) {
			exists = false;
		}
	}

	void draw() {
		if (!exists) {
			return;
		}

		dma_display->drawCircle(x, y, size, color);
	}
};

Bubble bubbles[NUM_BUBBLES];

// Randomly generate a new bubble
void generateBubble(int index) {
	int size = random(1, 6);
	float x = random(size, 64 - size);
	float y = random(size, 32 - size);
	float dx = (random(-100, 101) / 100.0);
	float dy = (random(-100, 101) / 100.0);
	uint16_t color = dma_display->color565(random(0, 256), random(0, 256), random(0, 256));
	bubbles[index].init(x, y, dx, dy, size, color);
}

///////////////////
// SETUP FUNCTION
///////////////////
void setup() {
	setupMatrix();

	for (int i = 0; i < NUM_BUBBLES; i++) {
		generateBubble(i);
	}
}

///////////////////
// MAIN LOOP
///////////////////
void loop() {
	dma_display->clearScreen();

	// Move each bubble
	for (int i = 0; i < NUM_BUBBLES; i++) {
		bubbles[i].move();
	}

	// Handle inter-bubble collisions
	for (int i = 0; i < NUM_BUBBLES; i++) {
		for (int j = i + 1; j < NUM_BUBBLES; j++) {
			if (bubbles[i].exists && bubbles[j].exists) {
				float dx = bubbles[j].x - bubbles[i].x;
				float dy = bubbles[j].y - bubbles[i].y;

				float distance = sqrt(dx * dx + dy * dy);
				float minDist = bubbles[i].size + bubbles[j].size;

				// Calculate new velocities for an elastic collision
				if (distance < minDist) {
					float angle = atan2(dy, dx);
					float magnitude1 = sqrt(bubbles[i].dx * bubbles[i].dx + bubbles[i].dy * bubbles[i].dy);
					float magnitude2 = sqrt(bubbles[j].dx * bubbles[j].dx + bubbles[j].dy * bubbles[j].dy);
					float dir1 = atan2(bubbles[i].dy, bubbles[i].dx);
					float dir2 = atan2(bubbles[j].dy, bubbles[j].dx);
					float newDx1 = magnitude1 * cos(dir1 - angle);
					float newDy1 = magnitude1 * sin(dir1 - angle);
					float newDx2 = magnitude2 * cos(dir2 - angle);
					float newDy2 = magnitude2 * sin(dir2 - angle);

					bubbles[i].dx = cos(angle) * newDx2 + cos(angle + M_PI / 2) * newDy1;
					bubbles[i].dy = sin(angle) * newDx2 + sin(angle + M_PI / 2) * newDy1;
					bubbles[j].dx = cos(angle) * newDx1 + cos(angle + M_PI / 2) * newDy2;
					bubbles[j].dy = sin(angle) * newDx1 + sin(angle + M_PI / 2) * newDy2;

					// Jostle the bubbles around a bit to mitigate overlapping
					float overlap = 0.5 * (minDist - distance + 1);
					bubbles[i].x -= overlap * cos(angle);
					bubbles[i].y -= overlap * sin(angle);
					bubbles[j].x += overlap * cos(angle);
					bubbles[j].y += overlap * sin(angle);
				}
			}
		}
	}

	// Render each bubble
	for (int i = 0; i < NUM_BUBBLES; i++) {
		bubbles[i].draw();
	}

	// Check if new bubbles need to be spawned to replaced popped ones
	for (int i = 0; i < NUM_BUBBLES; i++) {
		if (!bubbles[i].exists) {
			generateBubble(i);
		}
	}

	delay(50);
}

#endif