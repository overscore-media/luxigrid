/* _    _  _ _  _ _ ____ ____ _ ___
 * |    |  |  \/  | | __ |__/ | |  \
 * |___ |__| _/\_ | |__] |  \ | |__/
 * =================================
 * Luxigrid - Maze Animation
 * Copyright (c) 2024 OverScore Media - MIT License
 *
 * Adapted from Aurora: https://github.com/pixelmatix/aurora
 *
 * Original Copyright Notice:
 * ==================================================
 * Copyright (c) 2014 Jason Coon - MIT License
 *
 * Many thanks to Jamis Buck for the documentation of the Growing Tree maze generation algorithm:
 * http://weblog.jamisbuck.org/2011/1/27/maze-generation-growing-tree-algorithm
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

#ifndef MAZE_GUARD
#define MAZE_GUARD

#include "Arduino.h"
#include "../../lib/luxigrid.h"

#include "../../lib/animation-helpers.hpp"

enum Directions {
	None = 0,
	Up = 1,
	Down = 2,
	Left = 4,
	Right = 8,
};

struct Point {
	int x;
	int y;

	static Point New(int x, int y) {
		Point point;
		point.x = x;
		point.y = y;
		return point;
	}

	Point Move(Directions direction) {
		switch (direction) {
			case Up:
				return New(x, y - 1);
			case Down:
				return New(x, y + 1);
			case Left:
				return New(x - 1, y);
			case Right:
			default:
				return New(x + 1, y);
		}
	}

	static Directions Opposite(Directions direction) {
		switch (direction) {
			case Up:
				return Down;
			case Down:
				return Up;
			case Left:
				return Right;
			case Right:
			default:
				return Left;
		}
	}
};

static const int width = MATRIX_WIDTH / 2;
static const int height = MATRIX_HEIGHT / 2;

Directions grid[width][height];

Point point;

Point cells[256];
int cellCount = 0;

int algorithm = 0;
int algorithmCount = 1;

byte hue = 0;
byte hueOffset = 0;

Directions directions[4] = {Up, Down, Left, Right};

// Shift cells after the specified index down by one
void removeCell(int index) {
	for (int i = index; i < cellCount - 1; i++) {
		cells[i] = cells[i + 1];
	}

	cellCount--;
}

void shuffleDirections() {
	for (int a = 0; a < 4; a++) {
		int r = random(a, 4);
		Directions temp = directions[a];
		directions[a] = directions[r];
		directions[r] = temp;
	}
}

Point createPoint(int x, int y) {
	Point point;
	point.x = x;
	point.y = y;
	return point;
}

CRGB chooseColor(int index) {
	byte h = index + hueOffset;

	switch (algorithm) {
		case 0:
		default:
			return ColorFromPalette(currentPalette, h);
		case 1:
			return ColorFromPalette(currentPalette, hue++);
	}
}

int chooseIndex(int max) {
	switch (algorithm) {
		case 0:
		default:
			// Choose newest (recursive backtracker)
			return max - 1;
		case 1:
			// Choose random(Prim's)
			return random(max);
	}
}

void drawNextCell() {
	int index = chooseIndex(cellCount);

	if (index < 0)
		return;

	point = cells[index];

	Point imagePoint = createPoint(point.x * 2, point.y * 2);

	shuffleDirections();

	CRGB color = chooseColor(index);

	for (int i = 0; i < 4; i++) {
		Directions direction = directions[i];

		Point newPoint = point.Move(direction);
		if (newPoint.x >= 0 && newPoint.y >= 0 && newPoint.x < width && newPoint.y < height && grid[newPoint.y][newPoint.x] == None) {
			grid[point.y][point.x] = (Directions)((int)grid[point.y][point.x] | (int)direction);
			grid[newPoint.y][newPoint.x] = (Directions)((int)grid[newPoint.y][newPoint.x] | (int)point.Opposite(direction));

			Point newImagePoint = imagePoint.Move(direction);

			leds[XY16(newImagePoint.x, newImagePoint.y)] = color;

			cellCount++;
			cells[cellCount - 1] = newPoint;

			index = -1;
			break;
		}
	}

	if (index > -1) {
		Point finishedPoint = cells[index];
		imagePoint = createPoint(finishedPoint.x * 2, finishedPoint.y * 2);

		leds[XY16(imagePoint.x, imagePoint.y)] = color;

		removeCell(index);
	}
}

void generateMaze() {
	while (cellCount > 1) {
		drawNextCell();
	}
}

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

	cellCount = 0;
	hue = 0;
}

///////////////////
// MAIN LOOP
///////////////////
void loop() {
	if (1000 / default_fps + last_frame < millis()) {
		if (cellCount < 1) {
			// Reset the screen here
			memset(leds, 0x00, NUM_LEDS * sizeof(CRGB));

			// Reset the maze grid
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					grid[y][x] = None;
				}
			}

			int x = random(width);
			int y = random(height);

			cells[0] = createPoint(x, y);

			cellCount = 1;

			hue = 0;
			hueOffset = random(0, 256);
		}

		drawNextCell();

		if (cellCount < 1) {
			algorithm++;

			if (algorithm >= algorithmCount) {
				algorithm = 0;
			}

			return;
		}

		updateScreen();
		last_frame = millis();
	}
}

#endif