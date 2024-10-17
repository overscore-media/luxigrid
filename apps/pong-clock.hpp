/* _    _  _ _  _ _ ____ ____ _ ___
 * |    |  |  \/  | | __ |__/ | |  \
 * |___ |__| _/\_ | |__] |  \ | |__/
 * =================================
 * Luxigrid - Morphing Clock
 * Copyright (c) 2024 OverScore Media - MIT License
 *
 * Adapted from https://github.com/conejoninja/pongclock/
 * Copyright 2019 Daniel Esteban - conejo@conejo.me - MIT License
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

#ifndef PONG_CLOCK_GUARD
#define PONG_CLOCK_GUARD

#include "Arduino.h"
#include "../lib/luxigrid.h"

unsigned long lastTime = 0;
unsigned long timerDelay = 8;

void drawNet() {
	for (uint8_t i = 1; i < 32; i += 2) {
		dma_display->drawPixel(31, i, dma_display->color565(75, 75, 75));
	}
}

void drawPlayer(uint8_t x, uint8_t y) {
	dma_display->fillRect(x, y, 2, 8, dma_display->color565(255, 255, 255));
}

void drawBall(uint8_t x, uint8_t y) {
	dma_display->fillRect(x, y, 2, 2, dma_display->color565(255, 255, 255));
}

float calculateEndPoint(float x, float y, float vx, float vy, bool hit) {
	while (true) {
		x += vx;
		y += vy;

		if (hit) {
			if (x >= 60 || x <= 2) {
				return y;
			}
		} else {
			if (x >= 62 || x <= 0) {
				return y;
			}
		}

		if (y >= 30 || y <= 0) {
			vy = -vy;
		}
	}
}

void updateTime(uint8_t hour, uint8_t minute) {
	static uint8_t prevHour = 255;
	static uint8_t prevMinute = 255;

	// Clear the screen if the time has changed
	if (hour != prevHour || minute != prevMinute) {
		dma_display->clearScreen();
		prevHour = hour;
		prevMinute = minute;
	}

	dma_display->setCursor(0, 5);
	printCenteredText(String(hour) + " " + String(minute));
}

float ballX, ballY;
float leftPlayerTargetY, rightPlayerTargetY;
int16_t leftPlayerY, rightPlayerY;
float ballVX, ballVY;
int playerLoss, gameStopped;

int prevBallX, prevBallY;
int prevLeftPlayerY, prevRightPlayerY;

uint8_t hour, minute;

///////////////////
// SETUP FUNCTION
///////////////////
void setup() {
	setupMatrix();
	dma_display->setFont(&Org_01);

	ballX = 31.0;
	ballY = random(160) / 10.0 + 8.0;
	leftPlayerTargetY = ballY;
	rightPlayerTargetY = ballY;
	leftPlayerY = 8;
	rightPlayerY = 18;
	ballVX = 1.0;
	ballVY = 0.5;

	prevBallX = ballX;
	prevBallY = ballY;
	prevLeftPlayerY = leftPlayerY;
	prevRightPlayerY = rightPlayerY;

	playerLoss = 0;
	gameStopped = 0;

	DateTime rtcTime = rtc.now();
	time_t utcTimestamp = rtcTime.unixtime();

	struct tm tmNow;
	localtime_r(&utcTimestamp, &tmNow);

	TimeInfo now = getTimeInfo(tmNow);

	if (now.second > 29) {
		ballVY = -0.5;
	}

	hour = now.hour;
	minute = now.minute;
	updateTime(hour, minute);
}

///////////////////
// MAIN LOOP
///////////////////
void loop() {
	DateTime rtcTime = rtc.now();
	time_t utcTimestamp = rtcTime.unixtime();

	struct tm tmNow;
	localtime_r(&utcTimestamp, &tmNow);

	TimeInfo now = getTimeInfo(tmNow);

	int hour = now.hour;

	if ((millis() - lastTime) > timerDelay) {
		lastTime = millis();

		// Clear previous ball position (clearing whole screen every frame leads to flickering)
		dma_display->fillRect(prevBallX, prevBallY, 2, 2, dma_display->color565(0, 0, 0));

		// Clear previous left and right paddles positions
		dma_display->fillRect(0, prevLeftPlayerY, 2, 8, dma_display->color565(0, 0, 0));
		dma_display->fillRect(62, prevRightPlayerY, 2, 8, dma_display->color565(0, 0, 0));

		if (gameStopped < 20) {
			gameStopped++;
		} else {
			ballX += ballVX;
			ballY += ballVY;

			if ((ballX >= 60 && playerLoss != 1) || (ballX <= 2 && playerLoss != -1)) {
				ballVX = -ballVX;

				// Perform a random, last second flick to inflict effect on the ball
				int tmp = random(4);

				if (tmp > 0) {
					tmp = random(2);

					if (tmp == 0) {
						if (ballVY > 0 && ballVY < 2.5) {
							ballVY += 0.2;
						} else if (ballVY < 0 && ballVY > -2.5) {
							ballVY -= 0.2;
						}

						if (ballX >= 60) {
							rightPlayerTargetY += 1 + 3 * (random(1000) / 1000.0);
						} else {
							leftPlayerTargetY += 1 + 3 * (random(1000) / 1000.0);
						}
					} else {
						if (ballVY > 0.5) {
							ballVY -= 0.2;
						} else if (ballVY < -0.5) {
							ballVY += 0.2;
						}

						if (ballX >= 60) {
							rightPlayerTargetY -= 1 + 3 * (random(1000) / 1000.0);
						} else {
							leftPlayerTargetY -= 1 + 3 * (random(1000) / 1000.0);
						}
					}
				}

				if (leftPlayerTargetY < 0) {
					leftPlayerTargetY = 0;
				} else if (leftPlayerTargetY > 24) {
					leftPlayerTargetY = 24;
				}

				if (rightPlayerTargetY < 0) {
					rightPlayerTargetY = 0;
				} else if (rightPlayerTargetY > 24) {
					rightPlayerTargetY = 24;
				}
			} else if ((ballX > 62 && playerLoss == 1) || (ballX < 0 && playerLoss == -1)) {
				// Reset Game
				ballX = 31.0;
				ballY = random(1000) / 1000.0 * 16 + 8;
				ballVX = 1.0;
				ballVY = 0.5;

				if (random(2) == 0) {
					ballVY = -0.5;
				}

				hour = now.hour;
				minute = now.minute;

				updateTime(hour, minute);

				playerLoss = 0;
				gameStopped = 0;
			}

			if (ballY >= 30 || ballY <= 0) {
				ballVY = -ballVY;
			}

			// When the ball is on the other side of the court, move the player "randomly" to simulate an AI
			if (ballX == float(40 + random(13))) {
				leftPlayerTargetY = ballY - 3;

				if (leftPlayerTargetY < 0) {
					leftPlayerTargetY = 0;
				} else if (leftPlayerTargetY > 24) {
					leftPlayerTargetY = 24;
				}
			}

			if (ballX == float(8 + random(13))) {
				rightPlayerTargetY = ballY - 3;

				if (rightPlayerTargetY < 0) {
					rightPlayerTargetY = 0;
				} else if (rightPlayerTargetY > 24) {
					rightPlayerTargetY = 24;
				}
			}

			if (static_cast<uint16_t>(leftPlayerTargetY) > leftPlayerY) {
				leftPlayerY++;
			} else if (static_cast<uint16_t>(leftPlayerTargetY) < leftPlayerY) {
				leftPlayerY--;
			}

			if (static_cast<uint16_t>(rightPlayerTargetY) > rightPlayerY) {
				rightPlayerY++;
			} else if (static_cast<uint16_t>(rightPlayerTargetY) < rightPlayerY) {
				rightPlayerY--;
			}

			// If the ball is in the middle, check if we need to lose and calculate the endpoint to avoid/hit the ball
			if (ballX == 32) {
				if (minute != now.minute && playerLoss == 0) {
					if (now.minute == 0) {
						// Need to change the hour
						playerLoss = 1;
					} else {
						// Need to change the minute
						playerLoss = -1;
					}
				}

				// Moving to the left
				if (ballVX < 0) {
					leftPlayerTargetY = calculateEndPoint(ballX, ballY, ballVX, ballVY, playerLoss != -1) - 3;

					// We need to lose
					if (playerLoss == -1) {
						if (leftPlayerTargetY < 16) {
							leftPlayerTargetY = 19 + 5 * (random(1000) / 1000.0);
						} else {
							leftPlayerTargetY = 5 * (random(1000) / 1000.0);
						}
					}

					if (leftPlayerTargetY < 0) {
						leftPlayerTargetY = 0;
					} else if (leftPlayerTargetY > 24) {
						leftPlayerTargetY = 24;
					}
				}

				// Moving to the right
				if (ballVX > 0) {
					rightPlayerTargetY = calculateEndPoint(ballX, ballY, ballVX, ballVY, playerLoss != -1) - 3;

					// We need to lose
					if (playerLoss == 1) {
						if (rightPlayerTargetY < 16) {
							rightPlayerTargetY = 19 + 5 * (random(1000) / 1000.0);
						} else {
							rightPlayerTargetY = 5 * (random(1000) / 1000.0);
						}
					}

					if (rightPlayerTargetY < 0) {
						rightPlayerTargetY = 0;
					} else if (rightPlayerTargetY > 24) {
						rightPlayerTargetY = 24;
					}
				}
			}

			if (ballY < 0) {
				ballY = 0;
			}

			if (ballY > 30) {
				ballY = 30;
			}
		}

		// Show stuff on the display
		drawNet();
		drawPlayer(0, leftPlayerY);
		drawPlayer(62, rightPlayerY);
		drawBall(ballX, ballY);
		updateTime(hour, minute);

		prevBallX = ballX;
		prevBallY = ballY;
		prevLeftPlayerY = leftPlayerY;
		prevRightPlayerY = rightPlayerY;
	}

	delay(30);
}

#endif