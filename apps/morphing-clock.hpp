/* _    _  _ _  _ _ ____ ____ _ ___
 * |    |  |  \/  | | __ |__/ | |  \
 * |___ |__| _/\_ | |__] |  \ | |__/
 * =================================
 * Luxigrid - Morphing Clock
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

#ifndef MORPHING_CLOCK_GUARD
#define MORPHING_CLOCK_GUARD

#include "Arduino.h"
#include "../lib/luxigrid.h"
#include "../lib/digit.hpp"

struct MorphingClockConfig {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

MorphingClockConfig morphingClockConfig;
const char *morphingClockConfigFilename = "/config/apps/morphing_clock.json";

bool importMorphingClockConfig(const JsonDocument &jsonDoc) {
	// Return false if the morphing clock config doesn't have a valid r, g, or b value
	if (!jsonDoc["r"].is<uint8_t>() || !jsonDoc["g"].is<uint8_t>() || !jsonDoc["b"].is<uint8_t>()) {
		return false;
	}

	// Otherwise, load the config into morphingClockConfig
	morphingClockConfig.r = jsonDoc["r"].as<uint8_t>();
	morphingClockConfig.g = jsonDoc["g"].as<uint8_t>();
	morphingClockConfig.b = jsonDoc["b"].as<uint8_t>();
	return true;
}

void loadMorphingClockConfig() {
	File morphingClockConfigFile = SD.open(morphingClockConfigFilename, FILE_READ);

	DeserializationError deserializationError;
	JsonDocument jsonDoc;

	// Set default morphing clock config, and create new config file if none exists
	if (!morphingClockConfigFile) {
		jsonDoc["r"] = 255;
		jsonDoc["g"] = 255;
		jsonDoc["b"] = 255;

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

		File newMorphingClockConfigFile = SD.open(morphingClockConfigFilename, FILE_WRITE, true);

		if (!newMorphingClockConfigFile) {
			Serial.println("ERROR 2201 - Failed to Create App-Specific Config File");

			// ERROR 2201 - Failed to Create App-Specific Config File
			crashWithErrorCode(2201);
		}

		if (serializeJsonPretty(jsonDoc, newMorphingClockConfigFile) == 0) {
			Serial.println("ERROR 2202 - Failed to Write Defaults to App-Specific Config File");

			// ERROR: 2202 - Failed to Write Defaults to App-Specific Config File
			crashWithErrorCode(2202);
		}

		newMorphingClockConfigFile.close();

		// Reboot the ESP32
		restart();
	} else {
		deserializationError = deserializeJson(jsonDoc, morphingClockConfigFile);

		if (!importMorphingClockConfig(jsonDoc)) {
			Serial.println("ERROR 2203 - JSON App-Specific Config Present, but Validation Failed");

			// ERROR 2203 - JSON App-Specific Config Present, but Validation Failed
			crashWithErrorCode(2203);
		}
	}

	morphingClockConfigFile.close();
}

///////////////////
// RETRIEVE APP CONFIG
///////////////////
void retrieveAppConfig(JsonDocument &jsonDoc) {
	// Morphing Clock Config
	jsonDoc["morphing-clock"]["r"] = morphingClockConfig.r;
	jsonDoc["morphing-clock"]["g"] = morphingClockConfig.g;
	jsonDoc["morphing-clock"]["b"] = morphingClockConfig.b;
}

///////////////////
// VALIDATE APP CONFIG
///////////////////
void validateAppConfig(AsyncWebServerRequest *request, bool &shouldSaveConfig) {
	if (request->hasParam("r", true)) {
		if (!request->hasParam("r", true) || !request->hasParam("g", true) || !request->hasParam("b", true)) {
			request->send(400, "text/plain", "Morphing Clock colour configuration is invalid");
			shouldRestart = true;
			return;
		}

		const AsyncWebParameter *r = request->getParam("r", true);
		const AsyncWebParameter *g = request->getParam("g", true);
		const AsyncWebParameter *b = request->getParam("b", true);

		if (!stringIsNumeric(r->value()) || !stringIsNumeric(g->value()) || !stringIsNumeric(b->value())) {
			request->send(400, "text/plain", "Morphing Clock colour configuration is invalid");
			shouldRestart = true;
			return;
		}

		int rInt = r->value().toInt();
		int gInt = g->value().toInt();
		int bInt = b->value().toInt();

		if (rInt >= 0 && rInt <= 255 && gInt >= 0 && gInt <= 255 && bInt >= 0 && bInt <= 255) {
			morphingClockConfig.r = static_cast<uint8_t>(rInt);
			morphingClockConfig.g = static_cast<uint8_t>(gInt);
			morphingClockConfig.b = static_cast<uint8_t>(bInt);

			shouldSaveConfig = true;
		} else {
			request->send(400, "text/plain", "Morphing Clock colour configuration is invalid");
			shouldRestart = true;
			return;
		}

		shouldSaveConfig = true;
	}
}

///////////////////
// SAVE APP CONFIG
///////////////////
void saveAppConfig() {
	File morphingClockConfigFile = SD.open(morphingClockConfigFilename, FILE_WRITE, true);

	JsonDocument jsonDoc;
	jsonDoc["r"] = morphingClockConfig.r;
	jsonDoc["g"] = morphingClockConfig.g;
	jsonDoc["b"] = morphingClockConfig.b;

	if (serializeJsonPretty(jsonDoc, morphingClockConfigFile) == 0) {
		Serial.println("ERROR 2205 - Failed to Save Changes to App-Specific Config File");

		// ERROR 2205 - Failed to Save Changes to App-Specific Config File
		crashWithErrorCode(2205);
	}
}

const byte row1 = 6;
const byte row2 = 14;
const byte row3 = 22;
const byte row4 = 31;

uint16_t clockfaceColour;
Digit digit0;
Digit digit1;
Digit digit2;
Digit digit3;
Digit digit4;
Digit digit5;

TimeInfo pNow;
uint8_t pHH, pMM, pSS;

bool timestampsAreEqual(TimeInfo a, TimeInfo b) {
	return a.second == b.second && a.minute == b.minute && a.hour == b.hour;
}

///////////////////
// SETUP FUNCTION
///////////////////
void setup() {
	setupMatrix();
	loadMorphingClockConfig();

	// Indicate that the app-specific configuration has been loaded
	configIsLoaded = true;

	clockfaceColour = dma_display->color565(morphingClockConfig.r, morphingClockConfig.g, morphingClockConfig.b);
	digit0.init(0, 63 - 1 - 9 * 1, 9, clockfaceColour);
	digit1.init(0, 63 - 1 - 9 * 2, 9, clockfaceColour);
	digit2.init(0, 63 - 4 - 9 * 3, 9, clockfaceColour);
	digit3.init(0, 63 - 4 - 9 * 4, 9, clockfaceColour);
	digit4.init(0, 63 - 7 - 9 * 5, 9, clockfaceColour);
	digit5.init(0, 63 - 7 - 9 * 6, 9, clockfaceColour);

	DateTime rtcTime = rtc.now();
	time_t utcTimestamp = rtcTime.unixtime();

	struct tm tmNow;
	localtime_r(&utcTimestamp, &tmNow);

	TimeInfo now = getTimeInfo(tmNow);

	int ss = now.second;
	int mm = now.minute;
	int hh = now.hour;

	if (!globalConfig.is24h) {
		if (hh > 12) {
			hh -= 12;
		}

		// Convert 00:xx to 12:xx AM
		if (hh == 0) {
			hh = 12;
		}
	}

	int s0 = ss % 10;
	int s1 = ss / 10;
	int m0 = mm % 10;
	int m1 = mm / 10;
	int h0 = hh % 10;
	int h1 = hh / 10;

	digit1.DrawColon(clockfaceColour);
	digit3.DrawColon(clockfaceColour);
	digit0.Draw(s0, clockfaceColour);
	digit1.Draw(s1, clockfaceColour);
	digit2.Draw(m0, clockfaceColour);
	digit3.Draw(m1, clockfaceColour);
	digit4.Draw(h0, clockfaceColour);
	digit5.Draw(h1, clockfaceColour);

	pNow = now;
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

	DateTime rtcTime = rtc.now();
	time_t utcTimestamp = rtcTime.unixtime();

	struct tm tmNow;
	localtime_r(&utcTimestamp, &tmNow);

	TimeInfo now = getTimeInfo(tmNow);

	// Skip ahead if it's still the same time
	if (!timestampsAreEqual(now, pNow)) {
		int ss = now.second;
		int mm = now.minute;
		int hh = now.hour;

		if (!globalConfig.is24h) {
			if (hh > 12) {
				hh -= 12;
			}

			// Convert 00:xx to 12:xx AM
			if (hh == 0) {
				hh = 12;
			}
		}

		// Update seconds, if changed
		if (ss != pSS) {
			int s0 = ss % 10;
			int s1 = ss / 10;

			if (s0 != digit0.Value()) {
				digit0.Morph(s0);
			}

			if (s1 != digit1.Value()) {
				digit1.Morph(s1);
			}

			pSS = ss;
		}

		// Update minutes, if changed
		if (mm != pMM) {
			int m0 = mm % 10;
			int m1 = mm / 10;

			if (m0 != digit2.Value()) {
				digit2.Morph(m0);
			}

			if (m1 != digit3.Value()) {
				digit3.Morph(m1);
			}

			pMM = mm;
		}

		// Update hours, if changed
		if (hh != pHH) {
			int h0 = hh % 10;
			int h1 = hh / 10;

			if (h0 != digit4.Value()) {
				digit4.Morph(h0);
			}

			if (h1 != digit5.Value()) {
				digit5.Morph(h1);
			}

			pHH = hh;
		}

		pNow = now;
	}

	delay(150);
}

#endif