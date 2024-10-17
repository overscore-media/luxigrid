/* _    _  _ _  _ _ ____ ____ _ ___
 * |    |  |  \/  | | __ |__/ | |  \
 * |___ |__| _/\_ | |__] |  \ | |__/
 * =================================
 * Luxigrid - Pong Wars Animation
 * Copyright (c) 2024 OverScore Media - MIT License
 *
 * Adapted from: https://github.com/anoken/pong-wars-forM5Stack
 * Copyright (c) 2024 anoken2017 - MIT License
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

#ifndef PONG_WARS_GUARD
#define PONG_WARS_GUARD

#include "Arduino.h"
#include "../../lib/luxigrid.h"

struct Colour {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

struct PongWarsConfig {
	Colour colour1;
	Colour colour2;
};

PongWarsConfig pongWarsConfig;
const char *pongWarsConfigFilename = "/config/apps/pong_wars.json";

bool importPongWarsConfig(const JsonDocument &jsonDoc) {
	if (!jsonDoc.containsKey("colour1") || !jsonDoc.containsKey("colour2") || !jsonDoc["colour1"].is<JsonObjectConst>() || !jsonDoc["colour2"].is<JsonObjectConst>()) {
		return false;
	}

	JsonObjectConst colour1 = jsonDoc["colour1"].as<JsonObjectConst>();
	JsonObjectConst colour2 = jsonDoc["colour2"].as<JsonObjectConst>();

	// Return false if the colour1 format is invalid
	if (!colour1.containsKey("r") || !colour1["r"].is<uint8_t>() || !colour1.containsKey("g") || !colour1["g"].is<uint8_t>() || !colour1.containsKey("b") || !colour1["b"].is<uint8_t>()) {
		return false;
	}

	// Return false if the colour2 format is invalid
	if (!colour2.containsKey("r") || !colour2["r"].is<uint8_t>() || !colour2.containsKey("g") || !colour2["g"].is<uint8_t>() || !colour2.containsKey("b") || !colour2["b"].is<uint8_t>()) {
		return false;
	}

	// Otherwise, load the config into pongWarsConfig
	pongWarsConfig.colour1.r = colour1["r"].as<uint8_t>();
	pongWarsConfig.colour1.g = colour1["g"].as<uint8_t>();
	pongWarsConfig.colour1.b = colour1["b"].as<uint8_t>();

	pongWarsConfig.colour2.r = colour2["r"].as<uint8_t>();
	pongWarsConfig.colour2.g = colour2["g"].as<uint8_t>();
	pongWarsConfig.colour2.b = colour2["b"].as<uint8_t>();

	return true;
}

void loadPongWarsConfig() {
	File pongWarsConfigFile = SD.open(pongWarsConfigFilename, FILE_READ);

	DeserializationError deserializationError;
	JsonDocument jsonDoc;

	// Set default pong wars config, and create new config file if none exists
	if (!pongWarsConfigFile) {
		JsonObject colour1 = jsonDoc["colour1"].to<JsonObject>();
		colour1["r"] = 237;
		colour1["g"] = 223;
		colour1["b"] = 214;

		JsonObject colour2 = jsonDoc["colour2"].to<JsonObject>();
		colour2["r"] = 200;
		colour2["g"] = 100;
		colour2["b"] = 81;

		jsonDoc.shrinkToFit();

		// Create the "config" directory if it doesn't exist
		if (!SD.exists("/config")) {
			bool configDir = SD.mkdir("/config");

			if (!configDir) {
				Serial.println("ERROR 2206 - Failed to create directory for config files");

				// ERROR 2206 - Failed to create directory for config files
				crashWithErrorCode(2206);
			}
		}

		// Create the "apps" subdirectory if it doesn't exist
		if (!SD.exists("/config/apps")) {
			bool appsDir = SD.mkdir("/config/apps");

			if (!appsDir) {
				Serial.println("ERROR 2206 - Failed to create directory for config files");

				// ERROR 2206 - Failed to create directory for config files
				crashWithErrorCode(2206);
			}
		}

		File newPongWarsConfigFile = SD.open(pongWarsConfigFilename, FILE_WRITE, true);

		if (!newPongWarsConfigFile) {
			Serial.println("ERROR 2201 - Failed to Create App-Specific Config File");

			// ERROR 2201 - Failed to Create App-Specific Config File
			crashWithErrorCode(2201);
		}

		if (serializeJsonPretty(jsonDoc, newPongWarsConfigFile) == 0) {
			Serial.println("ERROR 2202 - Failed to Write Defaults to App-Specific Config File");

			// ERROR: 2202 - Failed to Write Defaults to App-Specific Config File
			crashWithErrorCode(2202);
		}

		newPongWarsConfigFile.close();

		// Reboot the ESP32
		restart();
	} else {
		deserializationError = deserializeJson(jsonDoc, pongWarsConfigFile);

		if (!importPongWarsConfig(jsonDoc)) {
			Serial.println("ERROR 2203 - JSON App-Specific Config Present, but Validation Failed");

			// ERROR 2203 - JSON App-Specific Config Present, but Validation Failed
			crashWithErrorCode(2203);
		}
	}

	pongWarsConfigFile.close();
}

///////////////////
// RETRIEVE APP CONFIG
///////////////////
void retrieveAppConfig(JsonDocument &jsonDoc) {
	// Pong Wars Config
	JsonObject colour1 = jsonDoc["pong-wars"]["colour1"].to<JsonObject>();
	colour1["r"] = pongWarsConfig.colour1.r;
	colour1["g"] = pongWarsConfig.colour1.g;
	colour1["b"] = pongWarsConfig.colour1.b;
	JsonObject colour2 = jsonDoc["pong-wars"]["colour2"].to<JsonObject>();
	colour2["r"] = pongWarsConfig.colour2.r;
	colour2["g"] = pongWarsConfig.colour2.g;
	colour2["b"] = pongWarsConfig.colour2.b;
}

///////////////////
// VALIDATE APP CONFIG
///////////////////
void validateAppConfig(AsyncWebServerRequest *request, bool &shouldSaveConfig) {
	if (request->hasParam("colours", true)) {
		const AsyncWebParameter *colours = request->getParam("colours", true);

		JsonDocument jsonDoc;
		DeserializationError deserializationError = deserializeJson(jsonDoc, colours->value());

		if (deserializationError || !importPongWarsConfig(jsonDoc)) {
			request->send(400, "text/plain", "Pong Wars colour configuration is invalid");
			restart();
		}

		shouldSaveConfig = true;
	}
}

///////////////////
// SAVE APP CONFIG
///////////////////
void saveAppConfig() {
	File pongWarsConfigFile = SD.open(pongWarsConfigFilename, FILE_WRITE, true);

	JsonDocument jsonDoc;
	JsonObject colour1 = jsonDoc["colour1"].to<JsonObject>();
	colour1["r"] = pongWarsConfig.colour1.r;
	colour1["g"] = pongWarsConfig.colour1.g;
	colour1["b"] = pongWarsConfig.colour1.b;
	JsonObject colour2 = jsonDoc["colour2"].to<JsonObject>();
	colour2["r"] = pongWarsConfig.colour2.r;
	colour2["g"] = pongWarsConfig.colour2.g;
	colour2["b"] = pongWarsConfig.colour2.b;

	if (serializeJsonPretty(jsonDoc, pongWarsConfigFile) == 0) {
		Serial.println("ERROR 2205 - Failed to Save Changes to App-Specific Config File");

		// ERROR 2205 - Failed to Save Changes to App-Specific Config File
		crashWithErrorCode(2205);
	}
}

// 画像の幅と高さ、四角形のサイズ、四角形の数を定義 [Define square size and the number of squares to fill the screen with]
int SQUARE_SIZE = 2;  // 16
int numSquaresX = MATRIX_WIDTH / SQUARE_SIZE + 1;
int numSquaresY = MATRIX_HEIGHT / SQUARE_SIZE + 1;

// 四角形の位置情報を格納する2次元ベクトル [A 2D vector that stores the position information of a square]
std::vector<std::vector<int>> squares;

// ボールの情報を格納する構造体のベクトル [A vector of structs that store ball information]
struct _pong {
	double x;
	double y;
	double dx;
	double dy;
	int class_num;
};

double vel = SQUARE_SIZE / 2;

std::vector<_pong> pong;

uint16_t color1;
uint16_t color2;

// 乱数生成関数 [Random number generator function]
double randomNum(double min, double max) {
	int rand_int = rand() % 100;
	double rand_uni = (rand_int / 100.0) * (max - min) + min;
	return rand_uni;
}

// 数値の符号を返す関数 [This function returns the sign of a number]
double sign(double A) {
	return (A == 0) ? 0 : A / abs(A);
}

// 四角形とボールの衝突判定と反射を処理する関数 [This function handles collision detection and bouncing when balls hit squares]
int updateSquareAndBounce(int numSquaresX, int numSquaresY, double x, double y, double &dx, double &dy, int class_num) {
	double updatedDx = dx;
	double updatedDy = dy;

	// ボールの円周上の複数のポイントをチェックする [Check multiple points on the circumference of the ball]
	for (double angle = 0; angle < M_PI * 2; angle += M_PI / 4) {
		int checkX = x + cos(angle) * (SQUARE_SIZE / 2);
		int checkY = y + sin(angle) * (SQUARE_SIZE / 2);

		// チェックしたポイントが画面内かどうか確認 [Check if the point in question is within the screen]
		int i = floor(checkX / SQUARE_SIZE);
		int j = floor(checkY / SQUARE_SIZE);

		if (i >= 0 && i < numSquaresX && j >= 0 && j < numSquaresY) {
			if (squares[i][j] != class_num) {
				squares[i][j] = class_num;

				// 角度からバウンド方向を決定 [Determine the bounce direction from the angle]
				if (abs(cos(angle)) > abs(sin(angle))) {
					updatedDx = -updatedDx;
				} else {
					updatedDy = -updatedDy;
				}
			}
		}
	}

	// ボールがループにはまらないように、バウンドにノイズを加える [Add noise to bounces to keep the balls from getting stuck in loops]
	double theta = M_PI / 4 * (1 + randomNum(-0.1, 1));
	dx = sign(updatedDx) * vel * cos(theta);
	dy = sign(updatedDy) * vel * sin(theta);

	return 0;
}

// 画面の境界とボールの衝突を判定し、必要に応じて反射させる関数 [This function determines the collision of the ball with the screen boundary, bouncing if necessary]
int checkBoundaryCollision(double x, double y, double &dx, double &dy) {
	if (x + dx > MATRIX_WIDTH - SQUARE_SIZE / 2 || x + dx < SQUARE_SIZE / 2) {
		dx = -dx;
	}

	if (y + dy > MATRIX_HEIGHT - SQUARE_SIZE / 2 || y + dy < SQUARE_SIZE / 2) {
		dy = -dy;
	}

	return 0;
}

///////////////////
// SETUP FUNCTION
///////////////////
void setup() {
	setupMatrix();
	loadPongWarsConfig();

	// Indicate that the app-specific configuration has been loaded
	configIsLoaded = true;

	color1 = dma_display->color565(pongWarsConfig.colour1.r, pongWarsConfig.colour1.g, pongWarsConfig.colour1.b);
	color2 = dma_display->color565(pongWarsConfig.colour2.r, pongWarsConfig.colour2.g, pongWarsConfig.colour2.b);

	// 四角形の位置情報を初期化 [Initialize the position information of the squares]
	squares.resize(numSquaresX);
	for (int i = 0; i < numSquaresX; i++) {
		squares[i].resize(numSquaresY);
	}

	// 画面を4つのエリアに分割し、それぞれに異なるクラスの四角形を配置 [Divide the screen into areas, and place squares of different classes in each area]
	int pong_no = 2;
	pong.resize(pong_no);

	for (int i = 0; i < numSquaresX; i++) {
		for (int j = 0; j < numSquaresY; j++) {
			if (i <= (numSquaresX / 2)) {
				squares[i][j] = 0;
			} else {
				squares[i][j] = 1;
			}
		}
	}

	// ボールの初期位置と速度を設定 [Set the initial position and speeds of the balls]
	pong[0].x = MATRIX_WIDTH / 4;
	pong[0].y = MATRIX_HEIGHT / 4;
	pong[0].dx = vel;
	pong[0].dy = -vel;
	pong[0].class_num = 0;

	pong[1].x = MATRIX_WIDTH * 3 / 4;
	pong[1].y = MATRIX_HEIGHT / 4;
	pong[1].dx = -vel;
	pong[1].dy = vel;
	pong[1].class_num = 1;
}

///////////////////
// MAIN LOOP
///////////////////
void loop() {
	// 四角形の描画 [Draw the squares]
	for (int i = 0; i < squares.size(); i++) {
		for (int j = 0; j < squares[i].size(); j++) {
			// 各クラスに応じて四角形を描画 [Draw the squares of each class]
			if (squares[i][j] == 0) {
				dma_display->fillRect(i * SQUARE_SIZE, j * SQUARE_SIZE, SQUARE_SIZE, SQUARE_SIZE, color1);
			}
			if (squares[i][j] == 1) {
				dma_display->fillRect(i * SQUARE_SIZE, j * SQUARE_SIZE, SQUARE_SIZE, SQUARE_SIZE, color2);
			}
		}
	}

	// ボールの描画と更新 [Draw and update the balls]
	for (int i = 0; i < pong.size(); i++) {
		// 各クラスに応じてボールの色を設定 [Set the ball's color according to its class_num]
		uint16_t color = pong[i].class_num == 0 ? color2 : color1;

		// ボールを描画 [Draw the ball]
		dma_display->fillCircle(pong[i].x, pong[i].y, SQUARE_SIZE / 2, color);

		// 四角形との衝突判定と反射、境界との衝突判定 [collision detection and reflection with rectangles, collision detection with boundaries]
		updateSquareAndBounce(numSquaresX, numSquaresY, pong[i].x, pong[i].y, pong[i].dx, pong[i].dy, pong[i].class_num);
		checkBoundaryCollision(pong[i].x, pong[i].y, pong[i].dx, pong[i].dy);

		// ボールの位置を更新 [Update ball position]
		pong[i].x += pong[i].dx;
		pong[i].y += pong[i].dy;
	}

	delay(10);
}

#endif