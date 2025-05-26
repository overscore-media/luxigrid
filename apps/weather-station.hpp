/* _    _  _ _  _ _ ____ ____ _ ___
 * |    |  |  \/  | | __ |__/ | |  \
 * |___ |__| _/\_ | |__] |  \ | |__/
 * =================================
 * Luxigrid - Weather Station
 * Copyright (c) 2024 OverScore Media - MIT License
 *
 * Inspired by a project by Rui Santos
 *
 * Original Copyright Notice:
 * ==================================================
 * Rui Santos
 * Complete project details at https://RandomNerdTutorials.com/esp32-http-get-open-weather-map-thingspeak-arduino/
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files.
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
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

#ifndef WEATHER_STATION_GUARD
#define WEATHER_STATION_GUARD

#include "Arduino.h"
#include "../lib/luxigrid.h"

#include <WiFiUdp.h>
#include <HTTPClient.h>

struct WeatherStationConfig {
	// The number of milliseconds to wait before pinging OpenMeteo again (5 minutes by default)
	unsigned long refreshInterval;
	// -180 to 180 degrees; 4 digits of precision and a couple extra characters just in case
	char latitude[12];
	char longitude[12];
	// Whether the weather station should fetch data from OpenMeteo if available
	// If true, rely only on local sensor data
	bool insideOnly;
};

WeatherStationConfig weatherStationConfig;
const char *weatherStationConfigFilename = "/config/apps/weather_station.json";

// Validate a latitude/longitude string
bool validateLatLong(const char *coords) {
	int coordsLength = strlen(coords);

	// "0.0000" (min length) to "-180.0000" (max length)
	if (coordsLength < 6 || coordsLength > 9) {
		return false;
	}

	// The first character may optionally be a negative sign
	int startIndex = (coords[0] == '-') ? 1 : 0;

	// The integer part should be 1 to 3 digits
	int intIndex = startIndex;
	while (isdigit(coords[intIndex])) {
		intIndex++;
	}

	// The integer part must have 1 to 3 digits
	if (intIndex - startIndex < 1 || intIndex - startIndex > 3) {
		return false;
	}

	// Check if the integer part is in range (-180 to 180)
	int integerPart = atoi(coords);
	if (integerPart < -180 || integerPart > 180) {
		return false;
	}

	// There must be a decimal point
	if (coords[intIndex] != '.') {
		return false;
	}

	// Move past the decimal point
	intIndex++;

	// There must be exactly four digits after the decimal point
	for (int j = 0; j < 4; j++) {
		if (!isdigit(coords[intIndex + j])) {
			return false;
		}
	}

	// There must be nothing after the four digits
	if (intIndex + 4 > coordsLength || coords[intIndex + 4] != '\0') {
		return false;
	}

	return true;
}

bool importWeatherStationConfig(const JsonDocument &jsonDoc) {
	if (!jsonDoc["refreshInterval"].is<unsigned long>() || !jsonDoc["latitude"].is<const char *>() || strlen(jsonDoc["latitude"]) >= sizeof(weatherStationConfig.latitude) || !validateLatLong(jsonDoc["latitude"]) || !jsonDoc["longitude"].is<const char *>() || strlen(jsonDoc["longitude"]) >= sizeof(weatherStationConfig.longitude) || !validateLatLong(jsonDoc["longitude"]) || !jsonDoc["insideOnly"].is<bool>()) {
		return false;
	}

	// A refresh interval smaller than 1 minute is not allowed
	// It's only fair; OpenMeteo is generously available for free
	if (jsonDoc["refreshInterval"].as<unsigned long>() < 60000) {
		return false;
	}

	weatherStationConfig.refreshInterval = jsonDoc["refreshInterval"].as<unsigned long>();
	strlcpy(weatherStationConfig.latitude, jsonDoc["latitude"], sizeof(weatherStationConfig.latitude));
	strlcpy(weatherStationConfig.longitude, jsonDoc["longitude"], sizeof(weatherStationConfig.longitude));
	weatherStationConfig.insideOnly = jsonDoc["insideOnly"].as<bool>();

	return true;
}

void loadWeatherStationConfig() {
	File weatherStationConfigFile = SD.open(weatherStationConfigFilename, FILE_READ);

	DeserializationError deserializationError;
	JsonDocument jsonDoc;

	// Set default weather station config, and create new config file if none exists
	if (!weatherStationConfigFile) {
		// 5 minutes in milliseconds
		jsonDoc["refreshInterval"] = 300000;
		// Coordinates of CN Tower; Downtown Toronto
		jsonDoc["latitude"] = "43.6426";
		jsonDoc["longitude"] = "-79.3871";
		// Set to insideOnly by default; can be changed later from Web UI
		jsonDoc["insideOnly"] = true;

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

		File newWeatherStationConfigFile = SD.open(weatherStationConfigFilename, FILE_WRITE, true);

		if (!newWeatherStationConfigFile) {
			Serial.println("ERROR 2201 - Failed to Create App-Specific Config File");

			// ERROR 2201 - Failed to Create App-Specific Config File
			crashWithErrorCode(2201);
		}

		if (serializeJsonPretty(jsonDoc, newWeatherStationConfigFile) == 0) {
			Serial.println("ERROR 2202 - Failed to Write Defaults to App-Specific Config File");

			// ERROR: 2202 - Failed to Write Defaults to App-Specific Config File
			crashWithErrorCode(2202);
		}

		newWeatherStationConfigFile.close();

		// Reboot the ESP32
		restart();
	} else {
		deserializationError = deserializeJson(jsonDoc, weatherStationConfigFile);

		if (!importWeatherStationConfig(jsonDoc)) {
			Serial.println("ERROR 2203 - JSON App-Specific Config Present, but Validation Failed");

			// ERROR 2203 - JSON App-Specific Config Present, but Validation Failed
			crashWithErrorCode(2203);
		}
	}

	weatherStationConfigFile.close();
}

///////////////////
// RETRIEVE APP CONFIG
///////////////////
void retrieveAppConfig(JsonDocument &jsonDoc) {
	// Weather Station Config
	jsonDoc["weather-station"]["refreshInterval"] = weatherStationConfig.refreshInterval;
	jsonDoc["weather-station"]["latitude"] = weatherStationConfig.latitude;
	jsonDoc["weather-station"]["longitude"] = weatherStationConfig.longitude;
	jsonDoc["weather-station"]["insideOnly"] = weatherStationConfig.insideOnly;
}

///////////////////
// VALIDATE APP CONFIG
///////////////////
void validateAppConfig(AsyncWebServerRequest *request, bool &shouldSaveConfig) {
	if (request->hasParam("refreshInterval", true)) {
		const AsyncWebParameter *refreshInterval = request->getParam("refreshInterval", true);

		if (!stringIsNumeric(refreshInterval->value())) {
			request->send(400, "text/plain", "OpenMeteo Refresh Interval configuration is invalid");
			shouldRestart = true;
			return;
		}

		unsigned long refreshIntervalToInt = strtoul(refreshInterval->value().c_str(), nullptr, 10);

		// 3600000 is 1 hour in milliseconds; absolutely no sense in a value anywhere near this high anyway
		// But it's an unsigned long so time calculations make sense (although maybe a 32-bit integer would do, but whatever)
		// Also, the refresh interval cannot be shorter than a minute
		if (refreshIntervalToInt <= 3600000 || refreshIntervalToInt < 60000) {
			weatherStationConfig.refreshInterval = refreshIntervalToInt;
			shouldSaveConfig = true;
		} else {
			request->send(400, "text/plain", "OpenMeteo Refresh Interval configuration is invalid");
			shouldRestart = true;
			return;
		}
	}

	if (request->hasParam("latitude", true)) {
		const AsyncWebParameter *latitude = request->getParam("latitude", true);

		if (!validateLatLong(latitude->value().c_str())) {
			request->send(400, "text/plain", "Weather Station latitude configuration is invalid");
			shouldRestart = true;
			return;
		}

		strlcpy(weatherStationConfig.latitude, latitude->value().c_str(), sizeof(weatherStationConfig.latitude));
		shouldSaveConfig = true;
	}

	if (request->hasParam("longitude", true)) {
		const AsyncWebParameter *longitude = request->getParam("longitude", true);

		if (!validateLatLong(longitude->value().c_str())) {
			request->send(400, "text/plain", "Weather Station longitude configuration is invalid");
			shouldRestart = true;
			return;
		}

		strlcpy(weatherStationConfig.longitude, longitude->value().c_str(), sizeof(weatherStationConfig.longitude));
		shouldSaveConfig = true;
	}

	if (request->hasParam("insideOnly", true)) {
		const AsyncWebParameter *insideOnly = request->getParam("insideOnly", true);

		if (insideOnly->value() == "true") {
			weatherStationConfig.insideOnly = true;
		} else if (insideOnly->value() == "false") {
			weatherStationConfig.insideOnly = false;
		} else {
			request->send(400, "text/plain", "Weather Station inside-only configuration is invalid");
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
	File weatherStationConfigFile = SD.open(weatherStationConfigFilename, FILE_WRITE, true);

	JsonDocument jsonDoc;
	jsonDoc["refreshInterval"] = weatherStationConfig.refreshInterval;
	jsonDoc["latitude"] = weatherStationConfig.latitude;
	jsonDoc["longitude"] = weatherStationConfig.longitude;
	jsonDoc["insideOnly"] = weatherStationConfig.insideOnly;

	if (serializeJsonPretty(jsonDoc, weatherStationConfigFile) == 0) {
		Serial.println("ERROR 2205 - Failed to Save Changes to App-Specific Config File");

		// ERROR 2205 - Failed to Save Changes to App-Specific Config File
		crashWithErrorCode(2205);
	}
}

unsigned long lastTime = 0;

int temp, hum, press;
float wind_speed;
int pop;
uint16_t weather_code;

char timeBuffer[50];
char dateBuffer[50];

// Whether a connection was established with OpenMeteo
// Otherwise, revert to showing indoor conditions
bool isOnline;

String serverPath = "";

// Used to check against to prevent unnecessary screen updates/flickering
int lastHour;
int lastMinute;
int lastSecond;
int lastDay;
int lastTemp;
int lastWeatherCode = 999;

// Display OPEN-METEO.COM in the "Weather Description" space for 20s after every API request
bool showDataAttribution = true;
unsigned long lastAttributionTime = 0;
unsigned long attributionDelay = 20000;

// WMO Weather interpretation codes
// Adapted from https://open-meteo.com/en/docs (scroll to bottom of page)
const char *getWeatherDescription(uint16_t weatherId) {
	if (weatherId == 0 || weatherId == 1) {
		return "CLEAR";
	} else if (weatherId == 2) {
		return "PARTLY CLOUDY";
	} else if (weatherId == 3) {
		return "OVERCAST";
	} else if (weatherId == 45 || weatherId == 48) {
		return "FOG";
	} else if (weatherId == 51 || weatherId == 53 || weatherId == 55) {
		return "DRIZZLE";
	} else if (weatherId == 56 || weatherId == 57) {
		return "FREEZING DRIZZLE";
	} else if (weatherId == 61 || weatherId == 63) {
		return "RAIN";
	} else if (weatherId == 65) {
		// Shaun!
		return "HEAVY RAIN";
	} else if (weatherId == 66 || weatherId == 67) {
		return "FREEZING RAIN";
	} else if (weatherId == 71) {
		return "LIGHT SNOW";
	} else if (weatherId == 73) {
		return "MODERATE SNOW";
	} else if (weatherId == 75) {
		return "HEAVY SNOW";
	} else if (weatherId == 77) {
		return "SNOW GRAINS";
	} else if (weatherId == 80 || weatherId == 81 || weatherId == 82) {
		return "SHOWERS";
	} else if (weatherId == 85 || weatherId == 86) {
		return "SNOW SHOWERS";
	} else if (weatherId == 95) {
		return "THUNDERSTORM";
	}

	return "";
}

JsonDocument httpGETRequest(const char *serverName) {
	WiFiClient client;
	HTTPClient http;

	http.useHTTP10(true);
	http.begin(client, serverName);

	int httpResponseCode = http.GET();

	Serial.print("HTTP Response code: ");
	Serial.println(httpResponseCode);

	if (httpResponseCode == 200) {
		isOnline = true;
	} else {
		isOnline = false;
	}

	JsonDocument doc;
	deserializeJson(doc, http.getStream());
	http.end();
	return doc;
}

///////////////////
// SETUP FUNCTION
///////////////////
void setup() {
	setupMatrix();
	loadWeatherStationConfig();

	// Indicate that the app-specific configuration has been loaded
	configIsLoaded = true;

	dma_display->setFont(&TomThumb);
	isOnline = !wifiConfig.isAccessPoint && !weatherStationConfig.insideOnly;

	String lat = weatherStationConfig.latitude;
	String lon = weatherStationConfig.longitude;
	serverPath = "http://api.open-meteo.com/v1/forecast?latitude=" + lat + "&longitude=" + lon + "&current=" + "temperature_2m,relative_humidity_2m,surface_pressure,wind_speed_10m,is_day,weather_code" + "&hourly=" + "precipitation_probability" + "&timezone=" + globalConfig.humanReadableTimezone + "&forecast_days=1";
	lastAttributionTime = millis();
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

	// Skip updating if the time hasn't changed
	if (now.hour == lastHour && now.minute == lastMinute && now.second == lastSecond) {
		delay(150);
		return;
	}

	lastHour = now.hour;
	lastMinute = now.minute;
	lastSecond = now.second;

	int hour = now.hour;
	char ampm[3];

	if (!globalConfig.is24h) {
		strcpy(ampm, "AM");

		if (hour >= 12) {
			strcpy(ampm, "PM");
			if (hour > 12) hour -= 12;
		}

		// Convert 00:xx to 12:xx AM
		if (hour == 0) {
			hour = 12;
		}
	}

	// Send an HTTP GET request to OpenMeteo for the latest weather conditions
	if (!wifiConfig.isAccessPoint && !weatherStationConfig.insideOnly && (lastTime == 0 || ((millis() - lastTime) > weatherStationConfig.refreshInterval && weatherStationConfig.refreshInterval >= 60000))) {
		JsonDocument doc = httpGETRequest(serverPath.c_str());

		// Round temperature to the nearest whole number, instead of just cutting off the decimals
		String tempString = doc["current"]["temperature_2m"];

		temp = (int)round(globalConfig.isCelcius ? tempString.toFloat() : ((tempString.toFloat() * 9.0 / 5.0) + 32));
		hum = doc["current"]["relative_humidity_2m"].as<int>();
		press = doc["current"]["surface_pressure"].as<int>();
		wind_speed = doc["current"]["wind_speed_10m"].as<float>();
		pop = doc["hourly"]["precipitation_probability"][0].as<int>();
		weather_code = doc["current"]["weather_code"].as<uint16_t>();

		lastTime = millis();

		// Display the weather data attribution (will be shown for 20s after every API request)
		showDataAttribution = true;
		lastAttributionTime = millis();
	}

	// If an OTA update is in progress, skip this iteration of the loop
	if (otaUpdateInProgress) {
		return;
	}

	// Display time
	dma_display->fillRect(34, 1, 30, 5, 0);
	sprintf(timeBuffer, "%d:%02d", hour, now.minute);
	dma_display->setTextColor(dma_display->color565(179, 255, 246));
	dma_display->setFont(&TomThumb);
	dma_display->setTextSize(1);
	dma_display->setCursor(36, 6);
	dma_display->print(timeBuffer);
	dma_display->setCursor(dma_display->getCursorX() + 1, dma_display->getCursorY());
	dma_display->print(ampm);

	// Display date
	if (now.day != lastDay) {
		dma_display->fillRect(34, 7, 30, 5, 0);
		dma_display->setTextColor(dma_display->color565(64, 184, 163));

		dma_display->setCursor(34, 12);
		sprintf(dateBuffer, "%02d", now.month);
		dma_display->print(dateBuffer);
		dma_display->drawPixel(dma_display->getCursorX(), dma_display->getCursorY() - 3, dma_display->color565(64, 184, 163));
		dma_display->setCursor(dma_display->getCursorX() + 2, dma_display->getCursorY());
		sprintf(dateBuffer, "%02d", now.day);
		dma_display->print(dateBuffer);
		dma_display->drawPixel(dma_display->getCursorX(), dma_display->getCursorY() - 3, dma_display->color565(64, 184, 163));
		dma_display->setCursor(dma_display->getCursorX() + 2, dma_display->getCursorY());
		sprintf(dateBuffer, "%02d", now.year % 100);
		dma_display->print(dateBuffer);
		lastDay = now.day;
	}

	// Display temperature
	dma_display->setFont(&TomThumb);
	dma_display->setTextSize(3);
	dma_display->setTextColor(dma_display->color565(255, 255, 255));
	dma_display->setCursor(0, 15);

	// Only the show the degrees sign and temperature symbol if the temperature is less than 3 digits
	static bool showDegrees = false;

	if (isOnline) {
		if (temp != lastTemp) {
			dma_display->fillRect(0, 0, 30, 15, 0);
			dma_display->print(temp);

			if (temp < 100) {
				showDegrees = true;
			}
		}

		lastTemp = temp;
	} else {
		int indoorTemp = (int)round(globalConfig.isCelcius ? temperature : (temperature * 9.0 / 5.0) + 32);

		if (indoorTemp != lastTemp) {
			dma_display->fillRect(0, 0, 30, 15, 0);
			dma_display->print(indoorTemp);

			if (indoorTemp < 100) {
				showDegrees = true;
			}
		}

		lastTemp = indoorTemp;
	}

	// Show a small circle as a degrees sign, and C or F depending on what temperature units are being used
	if (showDegrees) {
		dma_display->drawCircle(dma_display->getCursorX() - 1, 8, 1, dma_display->color565(255, 255, 255));
		dma_display->setTextSize(1);
		dma_display->setCursor(dma_display->getCursorX() + 1, dma_display->getCursorY());
		dma_display->print(globalConfig.isCelcius ? 'C' : 'F');
		showDegrees = false;
	}

	// Display weather description
	dma_display->setTextSize(1);
	dma_display->setCursor(0, 23);
	dma_display->setTextColor(dma_display->color565(255, 255, 255));

	if (isOnline && weather_code != lastWeatherCode && !showDataAttribution) {
		dma_display->fillRect(0, 18, 64, 5, 0);
		dma_display->print(String(getWeatherDescription(weather_code)));
		lastWeatherCode = weather_code;
	} else if (showDataAttribution) {
		// Go back to showing the weather description after showing the data attribution for 20s
		if ((millis() - lastAttributionTime) > attributionDelay) {
			showDataAttribution = false;
		}

		dma_display->setTextSize(1);
		dma_display->setCursor(0, 23);
		dma_display->setTextColor(dma_display->color565(174, 204, 252));
		dma_display->fillRect(0, 18, 64, 5, 0);
		dma_display->print("OPEN-METEO.COM :)");
		lastWeatherCode = 999;
	} else if (!isOnline) {
		dma_display->print("INSIDE");
		lastWeatherCode = 0;
	}

	// Clear bottom portion of screen only
	dma_display->fillRect(0, 26, 64, 5, dma_display->color565(0, 0, 0));

	// Display humidity
	dma_display->setCursor(0, 31);
	dma_display->setTextColor(dma_display->color565(247, 165, 42));

	if (isOnline) {
		dma_display->print(hum);
	} else {
		dma_display->print("HUM ");
		dma_display->print((int)humidity);
	}

	dma_display->print('%');

	// Display pressure
	dma_display->setTextColor(dma_display->color565(73, 156, 245));
	if (isOnline) {
		dma_display->setCursor(dma_display->getCursorX() + 3, 31);
		dma_display->print(press);
	} else {
		dma_display->setCursor(dma_display->getCursorX() + 5, 31);
		dma_display->print(pressure / 1000);
		dma_display->print(" KPA");
	}

	// Display wind speed
	dma_display->setCursor(dma_display->getCursorX() + 3, 31);
	dma_display->setTextColor(dma_display->color565(100, 100, 255));

	if (isOnline) {
		dma_display->print(String(wind_speed, 1));
	}

	// Display POP
	dma_display->setCursor(dma_display->getCursorX() + 3, 31);
	dma_display->setTextColor(dma_display->color565(0, 0, 255));

	if (isOnline) {
		dma_display->print(pop);
		dma_display->print('%');
	}

	delay(250);
}

#endif