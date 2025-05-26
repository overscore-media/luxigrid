#include "Arduino.h"
#include "../lib/luxigrid.h"

float temperature;
float humidity;
uint32_t pressure;

uint16_t lux = globalConfig.luxThreshold;

uint8_t newBrightness;
uint8_t currentBrightness;

unsigned long lastLightSensorTime = 0;
unsigned long lastBME680Time = 0;

const char *wifiConfigFilename = "/config/wifi.json";
const char *globalConfigFilename = "/config/global.json";

TimeInfo getTimeInfo(tm timeStruct) {
	TimeInfo timeInfo;

	timeInfo.year = timeStruct.tm_year + 1900;
	timeInfo.month = timeStruct.tm_mon + 1;
	timeInfo.day = timeStruct.tm_mday;
	timeInfo.hour = timeStruct.tm_hour;
	timeInfo.minute = timeStruct.tm_min;
	timeInfo.second = timeStruct.tm_sec;

	return timeInfo;
}

// Based on https://github.com/CelliesProjects/minimalUploadAuthESP32/blob/master/minimalUploadAuthESP32.ino
/* format bytes as KB, MB or GB string */
String humanReadableSize(const uint64_t bytes) {
	if (bytes < 1024) {
		return String(bytes) + " B";
	} else if (bytes < (1024 * 1024)) {
		return String(bytes / 1024.0) + " KB";
	} else if (bytes < (1024 * 1024 * 1024)) {
		return String(bytes / 1024.0 / 1024.0) + " MB";
	} else {
		return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
	}
}

bool stringIsNumeric(const String &str) {
	for (char c : str) {
		if (!isDigit(c)) {
			return false;
		}
	}
	return true;
}

String generateRandomString(int length) {
	String randomString = "";
	char characters[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	for (int i = 0; i < length; i++) {
		int randomIndex = random(0, sizeof(characters) - 1);
		randomString += characters[randomIndex];
	}

	return randomString;
}

bool verifyNTPServer(const char *ntpServer) {
	configTime(0, 0, ntpServer);

	struct tm timeinfo;

	// Wait up to 15 seconds to get the time
	if (!getLocalTime(&timeinfo, 15000)) {
		return false;
	}

	return true;
}

void printCenteredText(const String &text, bool centerVertically) {
	int16_t x1, y1;  // Variables to hold the top-left corner coordinates of the bounding box
	uint16_t w, h;   // Variables to hold the width and height of the bounding box

	// Use getTextBounds to get the size of your text
	// Initial cursor position set to (0,0) for the calculation
	dma_display->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);

	// Calculate the starting position to center the text
	int16_t centerX = (dma_display->width() - w) / 2;

	int16_t centerY;

	if (centerVertically) {
		// Don't mind the -4 here, that's just a magic number to make this work
		centerY = ((dma_display->height() - h) / 2) + 4;
	} else {
		centerY = dma_display->getCursorY();
	}

	// Set the cursor to the calculated position
	dma_display->setCursor(centerX, centerY);

	// Print the text
	dma_display->print(text);
}

void setTextColor(uint8_t r, uint8_t g, uint8_t b) {
	dma_display->setTextColor(dma_display->color565(r, g, b));
}

void printCenteredTruncatedText(String text, uint16_t margin, String ellipsis) {
	String truncatedText = text;

	int16_t x1, y1;
	uint16_t textWidth, textHeight;

	bool truncationRequired = false;

	// Calculate maxWidth, taking the margin into account on both sides
	uint16_t maxWidth = PANEL_RES_X - (margin * 2);

	// Use the current Y coordinate of the cursor
	int16_t y = dma_display->getCursorY();

	// Get the initial measurement of the text's rectangle
	dma_display->getTextBounds(truncatedText, 0, y, &x1, &y1, &textWidth, &textHeight);

	// While the text is wider than the max width, keep removing characters until it fits with the ellipsis
	while (textWidth > maxWidth && truncatedText.length() > 1) {
		truncatedText = truncatedText.substring(0, truncatedText.length() - 1);

		// Measure the text again, with the ellipsis, and set the truncationRequired flag
		dma_display->getTextBounds(truncatedText + ellipsis, 0, y, &x1, &y1, &textWidth, &textHeight);
		truncationRequired = true;
	}

	// Add the ellipsis to the text, if necessary
	if (truncationRequired) {
		truncatedText += ellipsis;
	}

	// Print the centered, potentially-truncated text to the panel
	dma_display->setCursor((PANEL_RES_X - textWidth) / 2, y);
	dma_display->print(truncatedText);
}

void getBH1750Readings() {
	if (lightSensor.measurementReady()) {
		float lightLevel = lightSensor.readLightLevel();

		// If lightLevel is negative, that means an error has occurred; ignore the reading
		if (lightLevel < 0) {
			newBrightness = 255;
			return;
		}

		// Cast lux to an unsigned 16-bit integer, because there's no sense keeping it as a float for our purposes
		lux = (uint16_t)lightLevel;

		if (lux >= globalConfig.luxThreshold) {
			newBrightness = 255;
		} else if (lux < (globalConfig.luxThreshold >> 3)) {
			newBrightness = 255 >> 4;
		} else if (lux < (globalConfig.luxThreshold >> 2)) {
			newBrightness = 255 >> 3;
		} else if (lux < (globalConfig.luxThreshold >> 1)) {
			newBrightness = 255 >> 2;
		} else if (lux < globalConfig.luxThreshold) {
			newBrightness = 255 >> 1;
		}

		// Add 1 to newBrightness so it's a power of 2 (unless it's already 255)
		if (newBrightness != 255) {
			newBrightness += 1;
		}
	}
}

void getBME680Readings(unsigned long currentMillis) {
	static bool readingInProgress = false;
	static unsigned long endTime = 0;

	if ((currentMillis - lastBME680Time >= globalConfig.bme680Delay || lastBME680Time == 0) && !readingInProgress) {
		endTime = bme680.beginReading();

		if (endTime == 0) {
			Serial.println("Failed to start reading from BME680");
			return;
		}

		readingInProgress = true;
	}

	// If the reading is in progress and the result is ready to be read
	if (readingInProgress && currentMillis >= endTime) {
		if (!bme680.endReading()) {
			Serial.println("Failed to finish reading from BME680");
		} else {
			temperature = bme680.temperature;
			humidity = bme680.humidity;
			pressure = bme680.pressure;

			lastBME680Time = currentMillis;
			readingInProgress = false;
		}
	}
}

void setPanelBrightness(unsigned long currentMillis) {
	if ((currentMillis - lastLightSensorTime) >= globalConfig.bh1750Delay || lastLightSensorTime == 0) {
		lastLightSensorTime = currentMillis;
		getBH1750Readings();

		if (newBrightness != currentBrightness) {
			dma_display->setBrightness(newBrightness);
			currentBrightness = newBrightness;
		}
	}
}

bool importWifiConfig(const JsonDocument &jsonDoc) {
	if (!jsonDoc["ssid"].is<const char *>() || strlen(jsonDoc["ssid"]) >= sizeof(wifiConfig.ssid) || !jsonDoc["password"].is<const char *>() || strlen(jsonDoc["password"]) >= sizeof(wifiConfig.password)) {
		return false;
	}

	if (!jsonDoc["retries"].is<int8_t>() || !jsonDoc["isAccessPoint"].is<bool>()) {
		return false;
	}

	strlcpy(wifiConfig.ssid, jsonDoc["ssid"], sizeof(wifiConfig.ssid));
	strlcpy(wifiConfig.password, jsonDoc["password"], sizeof(wifiConfig.password));
	wifiConfig.retries = jsonDoc["retries"].as<int8_t>();
	wifiConfig.isAccessPoint = jsonDoc["isAccessPoint"].as<bool>();

	return true;
}

bool importGlobalConfig(const JsonDocument &jsonDoc) {
	if (!jsonDoc["timezone"].is<const char *>() || strlen(jsonDoc["timezone"]) >= sizeof(globalConfig.timezone) || !jsonDoc["humanReadableTimezone"].is<const char *>() || strlen(jsonDoc["humanReadableTimezone"]) >= sizeof(globalConfig.humanReadableTimezone) || !jsonDoc["disableBH1750"].is<bool>() || !jsonDoc["brightness"].is<uint8_t>() || !jsonDoc["ntpServer"].is<const char *>() || strlen(jsonDoc["ntpServer"]) >= sizeof(globalConfig.ntpServer) || !jsonDoc["disableNTP"].is<bool>() || !jsonDoc["luxThreshold"].is<uint16_t>() || !jsonDoc["bh1750Delay"].is<unsigned long>() || !jsonDoc["bme680Delay"].is<unsigned long>() || !jsonDoc["is24h"].is<bool>() || !jsonDoc["isCelcius"].is<bool>()) {
		return false;
	}

	strlcpy(globalConfig.timezone, jsonDoc["timezone"], sizeof(globalConfig.timezone));
	strlcpy(globalConfig.humanReadableTimezone, jsonDoc["humanReadableTimezone"], sizeof(globalConfig.humanReadableTimezone));
	globalConfig.disableBH1750 = jsonDoc["disableBH1750"].as<bool>();
	globalConfig.brightness = jsonDoc["brightness"].as<uint8_t>();
	strlcpy(globalConfig.ntpServer, jsonDoc["ntpServer"], sizeof(globalConfig.ntpServer));
	globalConfig.disableNTP = jsonDoc["disableNTP"].as<bool>();
	globalConfig.luxThreshold = jsonDoc["luxThreshold"].as<uint16_t>();
	globalConfig.bh1750Delay = jsonDoc["bh1750Delay"].as<unsigned long>();
	globalConfig.bme680Delay = jsonDoc["bme680Delay"].as<unsigned long>();
	globalConfig.is24h = jsonDoc["is24h"].as<bool>();
	globalConfig.isCelcius = jsonDoc["isCelcius"].as<bool>();

	return true;
}

void loadWifiConfig() {
	File wifiConfigFile = SD.open(wifiConfigFilename, FILE_READ);

	DeserializationError deserializationError;
	JsonDocument jsonDoc;

	if (!wifiConfigFile) {
		jsonDoc["ssid"] = "matrix-" + generateRandomString(5);
		jsonDoc["password"] = "";
		jsonDoc["retries"] = 0;
		jsonDoc["isAccessPoint"] = true;

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

		File newWifiConfigFile = SD.open(wifiConfigFilename, FILE_WRITE, true);

		if (!newWifiConfigFile) {
			Serial.println("ERROR 2001 - Failed to Create WiFi Config File");

			// ERROR: 2001 - Failed to Create WiFi Config File
			crashWithErrorCode(2001);
		}

		if (serializeJsonPretty(jsonDoc, newWifiConfigFile) == 0) {
			Serial.println("ERROR 2002 - Failed to Write Defaults to WiFi Config File");

			// ERROR: 2002 - Failed to Write Defaults to WiFi Config File
			crashWithErrorCode(2002);
		}

		newWifiConfigFile.close();

		// Reboot the ESP32
		restart();
	} else {
		deserializationError = deserializeJson(jsonDoc, wifiConfigFile);

		if (!importWifiConfig(jsonDoc)) {
			Serial.println("ERROR 2003 - JSON WiFi Config Present, but Validation Failed");

			// ERROR: 2003 - JSON WiFi Config Present, but Validation Failed
			crashWithErrorCode(2003);
		}
	}

	if (deserializationError) {
		Serial.println("ERROR 2004 - Invalid JSON WiFi Config File");

		// ERROR: 2004 - Invalid JSON WiFi Config File
		crashWithErrorCode(2004);
	}

	wifiConfigFile.close();
}

void loadGlobalConfig() {
	File globalConfigFile = SD.open(globalConfigFilename, FILE_READ);

	DeserializationError deserializationError;
	JsonDocument jsonDoc;

	if (!globalConfigFile) {
		// Set default config and create new config file if none exists
		jsonDoc["timezone"] = "EST5EDT,M3.2.0,M11.1.0";
		jsonDoc["humanReadableTimezone"] = "America/Toronto";
		jsonDoc["disableBH1750"] = false;
		jsonDoc["brightness"] = 255;
		jsonDoc["ntpServer"] = "pool.ntp.org";
		jsonDoc["disableNTP"] = false;
		jsonDoc["luxThreshold"] = 200;
		jsonDoc["bh1750Delay"] = 1000;
		jsonDoc["bme680Delay"] = 30000;
		jsonDoc["is24h"] = false;
		jsonDoc["isCelcius"] = true;

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

		File newGlobalConfigFile = SD.open(globalConfigFilename, FILE_WRITE, true);

		if (!newGlobalConfigFile) {
			Serial.println("ERROR 2101 - Failed to Create Global Config File");

			// ERROR: 2101 - Failed to Create Global Config File
			crashWithErrorCode(2101);
		}

		if (serializeJsonPretty(jsonDoc, newGlobalConfigFile) == 0) {
			Serial.println("ERROR 2102 - Failed to Write Defaults to Global Config File");

			// ERROR: 2102 - Failed to Write Defaults to Global Config File
			crashWithErrorCode(2102);
		}

		newGlobalConfigFile.close();

		// Reboot the ESP32
		restart();
	} else {
		deserializationError = deserializeJson(jsonDoc, globalConfigFile);

		if (!importGlobalConfig(jsonDoc)) {
			Serial.println("ERROR 2103 - JSON Global Config Present, but Validation Failed");

			// ERROR: 2103 - JSON Global Config Present, but Validation Failed
			crashWithErrorCode(2103);
		}
	}

	if (deserializationError) {
		Serial.println("ERROR 2104 - Invalid Global JSON Config File");

		// ERROR: 2104 - Invalid Global JSON Config File
		crashWithErrorCode(2104);
	}

	// Might be a better place for this, but this gets the panel brightness set as soon as practical
	if (globalConfig.disableBH1750) {
		dma_display->setBrightness(globalConfig.brightness);
	}

	globalConfigFile.close();
}

void saveWifiConfig() {
	File wifiConfigFile = SD.open(wifiConfigFilename, FILE_WRITE, true);

	JsonDocument jsonDoc;

	jsonDoc["ssid"] = wifiConfig.ssid;
	jsonDoc["password"] = wifiConfig.password;
	jsonDoc["retries"] = wifiConfig.retries;
	jsonDoc["isAccessPoint"] = wifiConfig.isAccessPoint;

	if (serializeJsonPretty(jsonDoc, wifiConfigFile) == 0) {
		Serial.println("ERROR 2005 - Failed to Save Changes to Wifi Config File");

		// ERROR: 2005 - Failed to Save Changes to Wifi Config File
		crashWithErrorCode(2005);
	}

	wifiConfigFile.close();
}

void saveGlobalConfig() {
	File globalConfigFile = SD.open(globalConfigFilename, FILE_WRITE, true);

	JsonDocument jsonDoc;

	jsonDoc["timezone"] = globalConfig.timezone;
	jsonDoc["humanReadableTimezone"] = globalConfig.humanReadableTimezone;
	jsonDoc["disableBH1750"] = globalConfig.disableBH1750;
	jsonDoc["brightness"] = globalConfig.brightness;
	jsonDoc["ntpServer"] = globalConfig.ntpServer;
	jsonDoc["disableNTP"] = globalConfig.disableNTP;
	jsonDoc["luxThreshold"] = globalConfig.luxThreshold;
	jsonDoc["bh1750Delay"] = globalConfig.bh1750Delay;
	jsonDoc["bme680Delay"] = globalConfig.bme680Delay;
	jsonDoc["is24h"] = globalConfig.is24h;
	jsonDoc["isCelcius"] = globalConfig.isCelcius;

	if (serializeJsonPretty(jsonDoc, globalConfigFile) == 0) {
		Serial.println("ERROR 2105 - Failed to Save Changes to Global Config File");

		// ERROR: 2105 - Failed to Save Changes to Global Config File
		crashWithErrorCode(2105);
	}
}

// Task function for handling DNS requests and setting panel brightness
void runBackgroundTasks(void *pvParameters) {
	for (;;) {
		unsigned long currentMillis = millis();

		if (!globalConfig.disableBH1750) {
			setPanelBrightness(currentMillis);
		}

		getBME680Readings(currentMillis);

		if (wifiConfig.isAccessPoint) {
			dnsServer.processNextRequest();
		}

		static bool otaLoadingMessageShown;

		if (otaUpdateInProgress) {
			// If an OTA Update has just started, ensure the screen has been cleared before displaying the loading message
			if (!otaLoadingMessageShown) {
				dma_display->clearScreen();
				otaLoadingMessageShown = true;
			}

			playOTALoadingAnimation();
		} else if (!otaUpdateInProgress && otaLoadingMessageShown) {
			// Clear the screen if an OTA update has been cancelled/is no longer in progress (unlikely but possible)
			dma_display->clearScreen();
			otaLoadingMessageShown = false;
			vTaskDelay(100 / portTICK_PERIOD_MS);
		}

		// Trigger a restart of the ESP32, if some function has called for it
		if (shouldRestart) {
			restart();
		}

		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}

void restart() {
	delay(1500);
	ESP.restart();
}