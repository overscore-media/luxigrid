/* _    _  _ _  _ _ ____ ____ _ ___
 * |    |  |  \/  | | __ |__/ | |  \
 * |___ |__| _/\_ | |__] |  \ | |__/
 * =================================
 * Luxigrid - GIF Player
 * Copyright (c) 2024 OverScore Media - MIT License
 *
 * Adapted from ESP32-HUB75-MatrixPanel-DMA:
 * https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA/tree/master/examples/AnimatedGIFPanel_SD
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

#ifndef GIF_PLAYER_GUARD
#define GIF_PLAYER_GUARD

#include "Arduino.h"
#include "../lib/luxigrid.h"

struct GifPlayerConfig {
	// Max duration to display each GIF for, in milliseconds
	unsigned long maxGifDuration;
	// Delay between GIF files
	unsigned long gifDelay;
};

GifPlayerConfig gifPlayerConfig;

#include "../lib/gif.hpp"

const char *gifPlayerConfigFilename = "/config/apps/gif_player.json";

bool importGifPlayerConfig(const JsonDocument &jsonDoc) {
	// Return false if the GIF player config doesn't have a valid maxGifDuration or gifDelay
	if (!jsonDoc.containsKey("maxGifDuration") || !jsonDoc["maxGifDuration"].is<unsigned long>() || !jsonDoc.containsKey("gifDelay") || !jsonDoc["gifDelay"].is<unsigned long>()) {
		return false;
	}

	// Otherwise, load the config into gifPlayerConfig
	gifPlayerConfig.maxGifDuration = jsonDoc["maxGifDuration"].as<unsigned long>();
	gifPlayerConfig.gifDelay = jsonDoc["gifDelay"].as<unsigned long>();
	return true;
}

void loadGifPlayerConfig() {
	File gifPlayerConfigFile = SD.open(gifPlayerConfigFilename, FILE_READ);

	DeserializationError deserializationError;
	JsonDocument jsonDoc;

	// Set default GIF player config, and create new config file if none exists
	if (!gifPlayerConfigFile) {
		jsonDoc["maxGifDuration"] = 30000;
		jsonDoc["gifDelay"] = 500;

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

		File newGifPlayerConfigFile = SD.open(gifPlayerConfigFilename, FILE_WRITE, true);

		if (!newGifPlayerConfigFile) {
			Serial.println("ERROR 2201 - Failed to Create App-Specific Config File");

			// ERROR 2201 - Failed to Create App-Specific Config File
			crashWithErrorCode(2201);
		}

		if (serializeJsonPretty(jsonDoc, newGifPlayerConfigFile) == 0) {
			Serial.println("ERROR 2202 - Failed to Write Defaults to App-Specific Config File");

			// ERROR: 2202 - Failed to Write Defaults to App-Specific Config File
			crashWithErrorCode(2202);
		}

		newGifPlayerConfigFile.close();

		// Reboot the ESP32
		restart();
	} else {
		deserializationError = deserializeJson(jsonDoc, gifPlayerConfigFile);

		if (!importGifPlayerConfig(jsonDoc)) {
			Serial.println("ERROR 2203 - JSON App-Specific Config Present, but Validation Failed");

			// ERROR 2203 - JSON App-Specific Config Present, but Validation Failed
			crashWithErrorCode(2203);
		}
	}

	gifPlayerConfigFile.close();
}

///////////////////
// RETRIEVE APP CONFIG
///////////////////
void retrieveAppConfig(JsonDocument &jsonDoc) {
	// GIF Player Config
	jsonDoc["gif-player"]["maxGifDuration"] = gifPlayerConfig.maxGifDuration;
	jsonDoc["gif-player"]["gifDelay"] = gifPlayerConfig.gifDelay;
}

///////////////////
// VALIDATE APP CONFIG
///////////////////
void validateAppConfig(AsyncWebServerRequest *request, bool &shouldSaveConfig) {
	if (request->hasParam("maxGifDuration", true)) {
		const AsyncWebParameter *maxGifDuration = request->getParam("maxGifDuration", true);

		if (!stringIsNumeric(maxGifDuration->value())) {
			request->send(400, "text/plain", "Max GIF Duration configuration is invalid");
			restart();
		}

		unsigned long maxGifDurationToInt = strtoul(maxGifDuration->value().c_str(), nullptr, 10);

		// 3600000 is 1 hour in milliseconds; absolutely no sense in a value anywhere near this high anyway
		// But it's an unsigned long so time calculations make sense (although maybe a 32-bit integer would do, but whatever)
		if (maxGifDurationToInt <= 3600000) {
			gifPlayerConfig.maxGifDuration = maxGifDurationToInt;
			shouldSaveConfig = true;
		} else {
			request->send(400, "text/plain", "Max GIF Duration configuration is invalid");
			restart();
		}
	}

	if (request->hasParam("gifDelay", true)) {
		const AsyncWebParameter *gifDelay = request->getParam("gifDelay", true);

		if (!stringIsNumeric(gifDelay->value())) {
			request->send(400, "text/plain", "GIF Delay configuration is invalid");
			restart();
		}

		unsigned long gifDelayToInt = strtoul(gifDelay->value().c_str(), nullptr, 10);

		// 3600000 is 1 hour in milliseconds; absolutely no sense in a value anywhere near this high anyway
		// But it's an unsigned long so time calculations make sense (although maybe a 32-bit integer would do, but whatever)
		if (gifDelayToInt <= 3600000) {
			gifPlayerConfig.gifDelay = gifDelayToInt;
			shouldSaveConfig = true;
		} else {
			request->send(400, "text/plain", "GIF Delay configuration is invalid");
			restart();
		}
	}
}

///////////////////
// SAVE APP CONFIG
///////////////////
void saveAppConfig() {
	File gifPlayerConfigFile = SD.open(gifPlayerConfigFilename, FILE_WRITE, true);

	JsonDocument jsonDoc;
	jsonDoc["maxGifDuration"] = gifPlayerConfig.maxGifDuration;
	jsonDoc["gifDelay"] = gifPlayerConfig.gifDelay;

	if (serializeJsonPretty(jsonDoc, gifPlayerConfigFile) == 0) {
		Serial.println("ERROR 2205 - Failed to Save Changes to App-Specific Config File");

		// ERROR 2205 - Failed to Save Changes to App-Specific Config File
		crashWithErrorCode(2205);
	}
}

static int totalFiles = 0;

// Temporary GIF file holder
File FSGifFile;

// GIF files path
std::vector<std::string> GifFiles;

// Max duration to display each GIF for, in milliseconds
const int maxGifDuration = 30000;

const char *gifsFolder = "/gifs";

void displayError(String errorText) {
	dma_display->clearScreen();
	dma_display->setFont(&Org_01);

	// Triangle
	dma_display->fillTriangle(32, 2, 20, 22, 44, 22, dma_display->color565(255, 255, 0));

	// Error text and code
	dma_display->setTextColor(dma_display->color565(255, 0, 0));
	dma_display->setCursor(0, 29);
	printCenteredText(errorText);

	while (true) {
		// Exclamation Mark (Black)
		dma_display->fillRect(31, 9, 3, 8, dma_display->color565(0, 0, 0));
		dma_display->fillRect(31, 19, 3, 2, dma_display->color565(0, 0, 0));

		delay(500);

		// Exclamation Mark (Blue)
		dma_display->fillRect(31, 9, 3, 8, dma_display->color565(0, 0, 255));
		dma_display->fillRect(31, 19, 3, 2, dma_display->color565(0, 0, 255));

		delay(500);
	}
}

///////////////////
// SETUP FUNCTION
///////////////////
void setup() {
	setupMatrix();

	loadGifPlayerConfig();
	configIsLoaded = true;

	// Throw an error if the /gifs folder doesn't exist and an error occurred while attempting to create it
	if (!SD.exists(gifsFolder) && !SD.mkdir(gifsFolder)) {
		displayError("MISSING FOLDER");
	}

	File root = SD.open(gifsFolder);

	if (!root) {
		displayError("MISSING FOLDER");
	}

	File file = root.openNextFile();

	if (!file) {
		displayError("NO GIFS");
	}

	while (file) {
		// Ignore subdirectories
		if (!file.isDirectory()) {
			String fileName = file.name();

			// Ignore all but files with the .gif extension
			if (fileName.endsWith(".gif")) {
				Serial.print("  FILE: ");
				Serial.print(fileName);
				Serial.print("  SIZE: ");
				Serial.println(file.size());

				std::string fullPath = "/gifs/" + std::string(fileName.c_str());
				Serial.println(fullPath.c_str());

				GifFiles.push_back(fullPath);
				totalFiles++;
			}
		}

		file = root.openNextFile();
	}

	if (!totalFiles) {
		displayError("NO GIFS");
	}

	if (file) {
		file.close();
	}

	Serial.printf("Found %d GIFs to play.", totalFiles);

	gif.begin(LITTLE_ENDIAN_PIXELS);
}

///////////////////
// MAIN LOOP
///////////////////
void loop() {
	// Iterate over a vector using a range-based for loop
	for (auto &elem : GifFiles) {
		gifPlay(elem.c_str());
		gif.reset();

		delay(gifPlayerConfig.gifDelay);
	}
}

#endif