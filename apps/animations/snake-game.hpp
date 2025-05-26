/* _    _  _ _  _ _ ____ ____ _ ___
 * |    |  |  \/  | | __ |__/ | |  \
 * |___ |__| _/\_ | |__] |  \ | |__/
 * =================================
 * Luxigrid - Snake Game Animation
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

#ifndef SNAKE_GAME_GUARD
#define SNAKE_GAME_GUARD

#include "Arduino.h"
#include "../../lib/luxigrid.h"

struct Point {
	int x;
	int y;
};

struct Snake {
	Point head;
	Point tail[2048];
	int length;
	int direction;
};

Point fruit;
Snake snake;

// The fruit can be placed on any tile that doesn't intersect with the snake
void placeFruit() {
	bool valid = false;

	while (!valid) {
		fruit.x = random(0, MATRIX_WIDTH);
		fruit.y = random(0, MATRIX_HEIGHT);
		valid = true;

		for (int i = 0; i < snake.length; i++) {
			if (fruit.x == snake.tail[i].x && fruit.y == snake.tail[i].y) {
				valid = false;
				break;
			}
		}
	}
}

void resetGame() {
	// Randomly place the snake on the screen, and give it a random initial direction
	snake.head = {random(0, MATRIX_WIDTH), random(0, MATRIX_HEIGHT)};
	snake.length = 2;
	snake.direction = random(0, 4);

	// Place the first segment of the tail based on the initial direction
	// It goes Down, Right, Up, Left
	switch (snake.direction) {
		case 0:
			snake.tail[0] = {snake.head.x, (snake.head.y - 1 + MATRIX_HEIGHT) % MATRIX_HEIGHT};
			break;
		case 1:
			snake.tail[0] = {(snake.head.x - 1 + MATRIX_WIDTH) % MATRIX_WIDTH, snake.head.y};
			break;
		case 2:
			snake.tail[0] = {snake.head.x, (snake.head.y + 1) % MATRIX_HEIGHT};
			break;
		case 3:
			snake.tail[0] = {(snake.head.x + 1) % MATRIX_WIDTH, snake.head.y};
			break;
	}

	// Initialize the second segment to be right behind the first segment
	if (snake.length > 1) {
		snake.tail[1] = snake.tail[0];
	}

	placeFruit();
}

bool wouldBeCollision(Point p) {
	for (int i = 0; i < snake.length; i++) {
		if (p.x == snake.tail[i].x && p.y == snake.tail[i].y) {
			return true;
		}
	}
	return false;
}

void moveSnake() {
	// Move the tail
	for (int i = snake.length - 1; i > 0; i--) {
		snake.tail[i] = snake.tail[i - 1];
	}

	if (snake.length > 1) {
		snake.tail[0] = snake.head;
	}

	// Calculate potential next positions
	Point potentialMoves[4];
	potentialMoves[0] = {snake.head.x, (snake.head.y + 1) % MATRIX_HEIGHT};                  // Down
	potentialMoves[1] = {(snake.head.x + 1) % MATRIX_WIDTH, snake.head.y};                   // Right
	potentialMoves[2] = {snake.head.x, (snake.head.y - 1 + MATRIX_HEIGHT) % MATRIX_HEIGHT};  // Up
	potentialMoves[3] = {(snake.head.x - 1 + MATRIX_WIDTH) % MATRIX_WIDTH, snake.head.y};    // Left

	// Determine the best move based on the distance to fruit and the likelihood of avoiding collisions
	int bestMove = -1;
	int shortestDistance = MATRIX_WIDTH * MATRIX_HEIGHT;

	for (int i = 0; i < 4; i++) {
		if (!wouldBeCollision(potentialMoves[i])) {
			int distance = abs(potentialMoves[i].x - fruit.x) + abs(potentialMoves[i].y - fruit.y);

			if (distance < shortestDistance) {
				shortestDistance = distance;
				bestMove = i;
			}
		}
	}

	// Move the snake's head based on the best move
	if (bestMove != -1) {
		snake.head = potentialMoves[bestMove];
		snake.direction = bestMove;
	} else {
		// Reset the game if no valid move is found (i.e., the snake has collided with itself)
		// The snake is fairly "smart", but it's bound to run into itself eventually
		// Of course, that makes for an entertaining animation
		resetGame();
		return;
	}

	// Handle what happens when the snake eats the fruit
	if (snake.head.x == fruit.x && snake.head.y == fruit.y) {
		snake.length++;
		snake.tail[snake.length - 1] = snake.tail[snake.length - 2];
		placeFruit();
	}
}

///////////////////
// SETUP FUNCTION
///////////////////
void setup() {
	setupMatrix();
	resetGame();
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

	moveSnake();
	dma_display->clearScreen();

	// Draw snake
	for (int i = 0; i < snake.length; i++) {
		dma_display->drawPixel(snake.tail[i].x, snake.tail[i].y, dma_display->color565(0, 255, 0));
	}

	dma_display->drawPixel(snake.head.x, snake.head.y, dma_display->color565(0, 255, 0));

	// Draw fruit
	dma_display->drawPixel(fruit.x, fruit.y, dma_display->color565(255, 0, 0));
	delay(20);
}

#endif