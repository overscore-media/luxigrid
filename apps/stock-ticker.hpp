/* _    _  _ _  _ _ ____ ____ _ ___
 * |    |  |  \/  | | __ |__/ | |  \
 * |___ |__| _/\_ | |__] |  \ | |__/
 * =================================
 * Luxigrid - Snakes Animation
 * Copyright (c) 2024 OverScore Media - MIT License
 *
 * Concept inspired by ESP32 Stock Ticker by Mike Rankin:
 * https://github.com/mike-rankin/ESP32_Stock_Ticker/blob/main/Code/Stock_Ticker_Demo.ino
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

#ifndef STOCK_TICKER_GUARD
#define STOCK_TICKER_GUARD

#include "Arduino.h"
#include "../lib/luxigrid.h"

#include <WiFiUdp.h>
#include <HTTPClient.h>

String payload = "";
double current = 0;

unsigned long previousTime = 0;

JsonDocument doc;
HTTPClient http;

struct StockPrice {
	double currentPrice;
	double percentChange;
};

struct Colour {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

struct StockInfo {
	char ticker[5];
	char companyName[16];
	Colour brandColour;
	StockPrice price;
};

struct StockTickerConfig {
	unsigned long refreshInterval;
	char apiToken[41];
	StockInfo stocks[8];
	uint8_t numberOfStocks;
	// Delay between stocks
	unsigned long stockDuration;
};

StockTickerConfig stockTickerConfig;
const char *stockTickerConfigFilename = "/config/apps/stock_ticker.json";

// Whether a connection was established with Finnhub API
// Otherwise, show an error message
bool isOnline;

// Whether there are any stocks set in the config
bool noStocks = true;

// Whether there is a valid API token in the config
bool tokenIsValid;

// Whether the API token is still the default value of REPLACE_ME
bool tokenIsUnset;

// Whether all of the stocks are returning valid data back from the API
bool stocksAreValid = true;

// Whether the Finnhub API rate limit has been reached
// Under normal conditions this should never occur, but just in case handle it
bool rateLimitReached = false;

bool importStockTickerConfig(const JsonDocument &jsonDoc) {
	// If the refreshInterval, apiToken, or stocks properties are invalid
	if (!jsonDoc.containsKey("refreshInterval") || !jsonDoc["refreshInterval"].is<unsigned long>() || !jsonDoc.containsKey("apiToken") || !jsonDoc["apiToken"].is<const char *>() || strlen(jsonDoc["apiToken"]) >= sizeof(stockTickerConfig.apiToken) || !jsonDoc.containsKey("stocks") || !jsonDoc["stocks"].is<JsonArrayConst>() || !jsonDoc.containsKey("stockDuration") || !jsonDoc["stockDuration"].is<unsigned long>()) {
		return false;
	}

	// A refresh interval smaller than 1 minute is not allowed
	// It's only fair; Finnhub's Free API is generous
	if (jsonDoc["refreshInterval"].as<unsigned long>() < 60000) {
		return false;
	}

	// A stock duration of under a second doesn't make much sense
	if (jsonDoc["stockDuration"].as<unsigned long>() < 1000) {
		return false;
	}

	// Parse and validate the stocks array
	JsonArrayConst stocks = jsonDoc["stocks"];

	uint8_t index = 0;
	for (JsonObjectConst stock : stocks) {
		// If the length of the stocks array is invalid
		if (index >= sizeof(stockTickerConfig.stocks) / sizeof(stockTickerConfig.stocks[0])) {
			return false;
		}

		// Return false if the ticker, companyName, and brandColour are invalid
		if (!stock.containsKey("ticker") || !stock["ticker"].is<const char *>() || strlen(stock["ticker"]) >= sizeof(stockTickerConfig.stocks[0].ticker) || !stock.containsKey("companyName") || !stock["companyName"].is<const char *>() || strlen(stock["companyName"]) >= sizeof(stockTickerConfig.stocks[0].companyName) || !stock.containsKey("brandColour") || !stock["brandColour"].is<JsonObjectConst>()) {
			return false;
		}

		JsonObjectConst brandColour = stock["brandColour"].as<JsonObjectConst>();

		// Return false if the brandColour format is invalid
		if (!brandColour.containsKey("r") || !brandColour["r"].is<uint8_t>() || !brandColour.containsKey("g") || !brandColour["g"].is<uint8_t>() || !brandColour.containsKey("b") || !brandColour["b"].is<uint8_t>()) {
			return false;
		}

		// Otherwise, add the stock's information to the stockTickerConfig
		strlcpy(stockTickerConfig.stocks[index].ticker, stock["ticker"].as<const char *>(), sizeof(stockTickerConfig.stocks[index].ticker));
		strlcpy(stockTickerConfig.stocks[index].companyName, stock["companyName"].as<const char *>(), sizeof(stockTickerConfig.stocks[index].companyName));

		stockTickerConfig.stocks[index].brandColour.r = brandColour["r"].as<uint8_t>();
		stockTickerConfig.stocks[index].brandColour.g = brandColour["g"].as<uint8_t>();
		stockTickerConfig.stocks[index].brandColour.b = brandColour["b"].as<uint8_t>();

		index++;
	}

	// Add the refreshInterval, apiToken, and stockDuration to the stockTickerConfig
	stockTickerConfig.refreshInterval = jsonDoc["refreshInterval"].as<unsigned long>();
	strlcpy(stockTickerConfig.apiToken, jsonDoc["apiToken"], sizeof(stockTickerConfig.apiToken));
	stockTickerConfig.stockDuration = jsonDoc["stockDuration"].as<unsigned long>();

	// numberOfStocks is the relevant length of the stocks array
	// Because the array can store *up to* 8 stocks, but that doesn't mean it always has that many
	stockTickerConfig.numberOfStocks = (uint8_t)index;

	return true;
}

void loadStockTickerConfig() {
	File stockTickerConfigFile = SD.open(stockTickerConfigFilename, FILE_READ);

	DeserializationError deserializationError;
	JsonDocument jsonDoc;

	// Set default stock ticker config, and create new config file if none exists
	if (!stockTickerConfigFile) {
		// 5 minutes in milliseconds
		jsonDoc["refreshInterval"] = 300000;
		jsonDoc["apiToken"] = "REPLACE_ME";
		JsonArray stocks = jsonDoc["stocks"].to<JsonArray>();
		jsonDoc["stockDuration"] = 6500;

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

		File newStockTickerConfigFile = SD.open(stockTickerConfigFilename, FILE_WRITE, true);

		if (!newStockTickerConfigFile) {
			Serial.println("ERROR 2201 - Failed to Create Stock Ticker Config File");

			// ERROR 2201 - Failed to Create Stock Ticker Config File
			crashWithErrorCode(2201);
		}

		if (serializeJsonPretty(jsonDoc, newStockTickerConfigFile) == 0) {
			Serial.println("ERROR 2202 - Failed to Write Defaults to Global Config File");

			// ERROR: 2202 - Failed to Write Defaults to Global Config File
			crashWithErrorCode(2202);
		}

		newStockTickerConfigFile.close();

		// Reboot the ESP32
		restart();
	} else {
		deserializationError = deserializeJson(jsonDoc, stockTickerConfigFile);

		if (!importStockTickerConfig(jsonDoc)) {
			Serial.println("ERROR 2203 - JSON Stock Ticker Config Present, but Validation Failed");

			// ERROR 2203 - JSON STock Ticker Config Present, but Validation Failed
			crashWithErrorCode(2203);
		}
	}

	if (deserializationError) {
		Serial.println("ERROR 2204 - Invalid Stock Ticker JSON Config File");

		// ERROR 2204 - Invalid Stock Ticker JSON Config File
		crashWithErrorCode(2204);
	}

	stockTickerConfigFile.close();
}

///////////////////
// RETRIEVE APP CONFIG
///////////////////
void retrieveAppConfig(JsonDocument &jsonDoc) {
	// Stock Ticker Config
	jsonDoc["stock-ticker"]["apiToken"] = stockTickerConfig.apiToken;
	jsonDoc["stock-ticker"]["refreshInterval"] = stockTickerConfig.refreshInterval;
	jsonDoc["stock-ticker"]["stockDuration"] = stockTickerConfig.stockDuration;

	jsonDoc["stock-ticker"]["stocks"] = JsonArray();

	for (uint8_t x = 0; x < stockTickerConfig.numberOfStocks; x++) {
		jsonDoc["stock-ticker"]["stocks"][x]["ticker"] = stockTickerConfig.stocks[x].ticker;
		jsonDoc["stock-ticker"]["stocks"][x]["companyName"] = stockTickerConfig.stocks[x].companyName;
		jsonDoc["stock-ticker"]["stocks"][x]["brandColour"]["r"] = stockTickerConfig.stocks[x].brandColour.r;
		jsonDoc["stock-ticker"]["stocks"][x]["brandColour"]["g"] = stockTickerConfig.stocks[x].brandColour.g;
		jsonDoc["stock-ticker"]["stocks"][x]["brandColour"]["b"] = stockTickerConfig.stocks[x].brandColour.b;
	}
}

///////////////////
// VALIDATE APP CONFIG
///////////////////
void validateAppConfig(AsyncWebServerRequest *request, bool &shouldSaveConfig) {
	if (request->hasParam("apiToken", true)) {
		const AsyncWebParameter *apiToken = request->getParam("apiToken", true);

		const char *apiTokenString = apiToken->value().c_str();

		if (strlen(apiTokenString) == 0 || strlen(apiTokenString) >= sizeof(stockTickerConfig.apiToken)) {
			request->send(400, "text/plain", "Finnhub API Token configuration is invalid");
			restart();
		}

		// Test Finnhub API token with a known good ticker (AAPL for Apple)
		http.begin("https://finnhub.io/api/v1/quote?symbol=AAPL&token=" + String(apiTokenString));

		int httpCode = http.GET();

		// Require the token to be validated against the Finnhub API here, unless currently offline
		if (httpCode == 200 || wifiConfig.isAccessPoint) {
			strlcpy(stockTickerConfig.apiToken, apiTokenString, sizeof(stockTickerConfig.apiToken));
			shouldSaveConfig = true;
		} else {
			request->send(400, "text/plain", "Finnhub API Token configuration is invalid");
			restart();
		}

		http.end();
	}

	if (request->hasParam("refreshInterval", true)) {
		const AsyncWebParameter *refreshInterval = request->getParam("refreshInterval", true);

		if (!stringIsNumeric(refreshInterval->value())) {
			request->send(400, "text/plain", "Finnhub API Refresh Interval configuration is invalid");
			restart();
		}

		unsigned long refreshIntervalToInt = strtoul(refreshInterval->value().c_str(), nullptr, 10);

		// 3600000 is 1 hour in milliseconds; absolutely no sense in a value anywhere near this high anyway
		// But it's an unsigned long so time calculations make sense (although maybe a 32-bit integer would do, but whatever)
		// Also, the refresh interval cannot be shorter than a minute
		if (refreshIntervalToInt <= 3600000 || refreshIntervalToInt < 60000) {
			stockTickerConfig.refreshInterval = refreshIntervalToInt;
			shouldSaveConfig = true;
		} else {
			request->send(400, "text/plain", "OpenMeteo Refresh Interval configuration is invalid");
			restart();
		}
	}

	if (request->hasParam("stocks", true)) {
		const AsyncWebParameter *stocks = request->getParam("stocks", true);

		JsonDocument jsonDoc;
		DeserializationError deserializationError = deserializeJson(jsonDoc, stocks->value());

		if (deserializationError) {
			request->send(400, "text/plain", "Stocks configuration is invalid");
			restart();
		}

		uint8_t index = 0;
		bool stocksAreValid = true;
		for (JsonObject stock : jsonDoc.as<JsonArray>()) {
			// If more than 8 stocks, skip any extras
			if (index >= sizeof(stockTickerConfig.stocks) / sizeof(stockTickerConfig.stocks[0])) {
				stocksAreValid = false;
				break;
			}

			// Return false if the ticker, companyName, and brandColour are invalid
			if (!stock.containsKey("ticker") || !stock["ticker"].is<const char *>() || strlen(stock["ticker"]) >= sizeof(stockTickerConfig.stocks[0].ticker) || !stock.containsKey("companyName") || !stock["companyName"].is<const char *>() || strlen(stock["companyName"]) >= sizeof(stockTickerConfig.stocks[0].companyName) || !stock.containsKey("brandColour") || !stock["brandColour"].is<JsonObjectConst>()) {
				stocksAreValid = false;
				break;
			}

			JsonObjectConst brandColour = stock["brandColour"].as<JsonObjectConst>();

			// Return false if the brandColour format is invalid
			if (!brandColour.containsKey("r") || !brandColour["r"].is<uint8_t>() || !brandColour.containsKey("g") || !brandColour["g"].is<uint8_t>() || !brandColour.containsKey("b") || !brandColour["b"].is<uint8_t>()) {
				stocksAreValid = false;
				break;
			}

			strlcpy(stockTickerConfig.stocks[index].ticker, stock["ticker"].as<const char *>(), sizeof(stockTickerConfig.stocks[index].ticker));
			strlcpy(stockTickerConfig.stocks[index].companyName, stock["companyName"].as<const char *>(), sizeof(stockTickerConfig.stocks[index].companyName));

			stockTickerConfig.stocks[index].brandColour.r = brandColour["r"].as<uint8_t>();
			stockTickerConfig.stocks[index].brandColour.g = brandColour["g"].as<uint8_t>();
			stockTickerConfig.stocks[index].brandColour.b = brandColour["b"].as<uint8_t>();

			index++;
		}

		if (stocksAreValid) {
			stockTickerConfig.numberOfStocks = index;
			shouldSaveConfig = true;
		} else {
			request->send(400, "text/plain", "Stocks configuration is invalid");
			restart();
		}
	}

	if (request->hasParam("stockDuration", true)) {
		const AsyncWebParameter *stockDuration = request->getParam("stockDuration", true);

		if (!stringIsNumeric(stockDuration->value())) {
			request->send(400, "text/plain", "Stock Delay configuration is invalid");
			restart();
		}

		unsigned long stockDurationToInt = strtoul(stockDuration->value().c_str(), nullptr, 10);

		// 3600000 is 1 hour in milliseconds; absolutely no sense in a value anywhere near this high anyway
		// But it's an unsigned long so time calculations make sense (although maybe a 32-bit integer would do, but whatever)
		// And a stock duration of under a second doesn't make much sense
		if (stockDurationToInt <= 3600000 || stockDurationToInt < 1000) {
			stockTickerConfig.stockDuration = stockDurationToInt;
			shouldSaveConfig = true;
		} else {
			request->send(400, "text/plain", "Stock Delay configuration is invalid");
			restart();
		}
	}
}

///////////////////
// SAVE APP CONFIG
///////////////////
void saveAppConfig() {
	File stockTickerConfigFile = SD.open(stockTickerConfigFilename, FILE_WRITE, true);

	JsonDocument jsonDoc;

	jsonDoc["apiToken"] = stockTickerConfig.apiToken;
	jsonDoc["refreshInterval"] = stockTickerConfig.refreshInterval;
	jsonDoc["stockDuration"] = stockTickerConfig.stockDuration;
	JsonArray stocks = jsonDoc["stocks"].to<JsonArray>();

	for (size_t i = 0; i < stockTickerConfig.numberOfStocks; i++) {
		JsonObject stock = stocks.add<JsonObject>();
		stock["ticker"] = stockTickerConfig.stocks[i].ticker;
		stock["companyName"] = stockTickerConfig.stocks[i].companyName;
		stock["brandColour"]["r"] = stockTickerConfig.stocks[i].brandColour.r;
		stock["brandColour"]["g"] = stockTickerConfig.stocks[i].brandColour.g;
		stock["brandColour"]["b"] = stockTickerConfig.stocks[i].brandColour.b;
	}

	if (serializeJsonPretty(jsonDoc, stockTickerConfigFile) == 0) {
		Serial.println("ERROR 2205 - Failed to Save Changes to Stock Ticker Config File");

		// ERROR 2205 - Failed to Save Changes to Global Config File
		crashWithErrorCode(2205);
	}
}

// Returns a stock's percent change as a formatted string, to mitigate text overflow issues
String formatPercentageChange(double value) {
	// Very small values or 0 should just be "0.00"
	if (value < 0.00005) {
		return "0.00";
	}

	// Handle relevant decimals under 0.01
	// Turn something like 0.0234567 into ".0235" - round to 4 decimal places and remove the 0 at the start
	if (value < 0.01) {
		String result = String(value, 4);
		result.remove(0, 1);
		return result;
	}

	// There should be exactly two decimal points for values under 1 (that didn't match the above checks)
	// In this case, the starting 0 is preserved. So "0.12345" becomes "0.12"
	if (value < 1.0) {
		return String(value, 2);
	}

	// Two decimal places for values between 1 and 99 inclusive
	if (value >= 1.0 && value < 100) {
		return String(value, 2);
	}

	// One decimal place for values between 100 and 999 inclusive
	if (value >= 100 && value < 1000) {
		return String(value, 1);
	}

	// No decimal places for values greater than or equal to 1000
	return String(value, 0);
}

void printStockInfo(StockInfo currentStockInfo) {
	dma_display->clearScreen();

	// Print company name
	dma_display->setTextSize(1);
	dma_display->setCursor(0, 6);
	dma_display->setTextColor(dma_display->color565(155, 155, 155));
	dma_display->setFont(&TomThumb);
	printCenteredTruncatedText(currentStockInfo.companyName, 1, "...");
	dma_display->setFont(&Org_01);

	int16_t middleRow = 16;

	// Print ticker
	dma_display->setCursor(1, middleRow);
	dma_display->setTextColor(dma_display->color565(currentStockInfo.brandColour.r, currentStockInfo.brandColour.g, currentStockInfo.brandColour.b));
	dma_display->print(currentStockInfo.ticker);

	dma_display->setTextColor(dma_display->color565(255, 255, 255));

	int16_t x1, y1;
	uint16_t w, h;

	// Use the absolute value with the formatPercentChange function
	String percentChange = formatPercentageChange(fabs(currentStockInfo.price.percentChange));

	dma_display->getTextBounds(percentChange, 0, 0, &x1, &y1, &w, &h);

	int16_t textStart = dma_display->width() - (int16_t)w - 4;

	int16_t xPosition = textStart - 11;
	int16_t yPosition = dma_display->getCursorY();

	// Only draw the triangle/bar if there's enough space for it
	bool drawTriangle = xPosition > 20;

	bool changeIsPositive = currentStockInfo.price.percentChange > 0;
	bool changeIsZero = currentStockInfo.price.percentChange == 0;

	if (changeIsPositive) {
		dma_display->setTextColor(dma_display->color565(0, 255, 0));

		if (drawTriangle) {
			// Upwards triangle
			dma_display->fillTriangle(xPosition + 1, yPosition, xPosition + 5, yPosition - 4, xPosition + 9, yPosition, dma_display->color565(0, 255, 0));
			dma_display->drawFastHLine(xPosition + 4, yPosition, 3, dma_display->color565(0, 0, 0));
		}

	} else if (!changeIsZero) {
		dma_display->setTextColor(dma_display->color565(255, 0, 0));

		if (drawTriangle) {
			// Downwards triangle
			dma_display->fillTriangle(xPosition + 1, yPosition - 4, xPosition + 5, yPosition, xPosition + 9, yPosition - 4, dma_display->color565(255, 0, 0));
			dma_display->drawFastHLine(xPosition + 4, yPosition - 4, 3, dma_display->color565(0, 0, 0));
		}
	} else {
		dma_display->setTextColor(dma_display->color565(0, 0, 255));

		if (drawTriangle) {
			// Flat bar (for no change)
			dma_display->fillRect(xPosition + 1, yPosition - 3, 9, 3, dma_display->color565(0, 0, 255));
		}
	}

	// Print percent change
	dma_display->setCursor(textStart, middleRow);
	dma_display->print(percentChange);

	uint16_t percentSignColour = changeIsPositive ? dma_display->color565(0, 155, 0) : changeIsZero ? dma_display->color565(0, 0, 155)
	                                                                                                : dma_display->color565(155, 0, 0);

	// Draw a mini percent sign
	xPosition = dma_display->getCursorX();
	dma_display->drawLine(xPosition, yPosition - 1, xPosition + 2, yPosition - 3, percentSignColour);
	dma_display->drawPixel(xPosition, yPosition - 4, percentSignColour);
	dma_display->drawPixel(xPosition + 2, yPosition, percentSignColour);

	// Draw the cost
	dma_display->setTextColor(dma_display->color565(255, 255, 255));
	dma_display->setCursor(0, 27);
	printCenteredText("$" + String(currentStockInfo.price.currentPrice));

	delay(stockTickerConfig.stockDuration);

	if (stockTickerConfig.numberOfStocks != 1) {
		// Sweep the stock away with a horizontal line before the next stock
		for (int x = 0; x < 64; x++) {
			dma_display->drawLine(x, 0, x, 32, dma_display->color565(255, 255, 255));
			delay(10);
			dma_display->drawLine(x, 0, x, 32, dma_display->color565(0, 0, 0));
		}
	}
}

void refreshStockPrice(StockInfo &stockInfo) {
	float percentchange;

	http.begin("https://finnhub.io/api/v1/quote?symbol=" + String(stockInfo.ticker) + "&token=" + String(stockTickerConfig.apiToken));

	int httpCode = http.GET();

	if (httpCode == 200) {
		payload = http.getString();
		char inp[payload.length()];
		payload.toCharArray(inp, payload.length());
		deserializeJson(doc, inp);

		// c = Current price, d = Change, dp = Percent change, pc = Previous close price
		String v = doc["c"];
		String c = doc["dp"];
		current = v.toDouble();
		percentchange = c.toDouble();

		// An invalid stock returns something like this: {"c":0,"d":null,"dp":null}
		if (v == "0" || c == "null") {
			Serial.println("Invalid Stock - " + String(stockInfo.ticker));
			stocksAreValid = false;
			http.end();
			return;
		}

		Serial.println(payload);

		stockInfo.price.currentPrice = current;
		stockInfo.price.percentChange = percentchange;

		isOnline = true;
	} else {
		Serial.println("HTTP GET request failed");
		Serial.println("HTTP Response Code: " + String(httpCode));

		// A 401 error indicates an invalid API token; a 429 indicates going over the rate limit
		// Any other errors indicate some kind of internet connectivity problem
		if (httpCode == 401) {
			tokenIsValid = false;
		} else if (httpCode == 429) {
			rateLimitReached = true;
		} else {
			isOnline = false;
		}
	}

	http.end();
}

const char *loading = "LOADING";
const char *offline = "OFFLINE";
int x = 12;
int y = 17;

void getNextColor(uint8_t &r, uint8_t &g, uint8_t &b) {
	const uint8_t stepSize = 85;  // Define a step size for color transitions

	if (r == 255 && g < 255 && b == 0) {
		g += stepSize;  // Increase green to transition from red to yellow
	} else if (g == 255 && r > 0) {
		r -= stepSize;  // Decrease red to transition from yellow to green
	} else if (g == 255 && b < 255) {
		b += stepSize;  // Increase blue to transition from green to cyan
	} else if (b == 255 && g > 0) {
		g -= stepSize;  // Decrease green to transition from cyan to blue
	} else if (b == 255 && r < 255) {
		r += stepSize;  // Increase red to transition from blue to magenta
	} else if (r == 255 && b > 0) {
		b -= stepSize;  // Decrease blue to transition from magenta back to red
	} else {
		r = 255;  // Reset to start from red if out of sync
		g = b = 0;
	}
}

bool showLoadingAnimation = false;
Colour loadingLetterColours[7];

void playLoadingAnimation(void *pvParameters) {
	for (;;) {
		if (showLoadingAnimation) {
			x = 12;

			for (int i = 0; loading[i] != '\0'; i++) {
				getNextColor(loadingLetterColours[i].r, loadingLetterColours[i].g, loadingLetterColours[i].b);

				setTextColor(loadingLetterColours[i].r, loadingLetterColours[i].g, loadingLetterColours[i].b);

				dma_display->setCursor(x, y);
				dma_display->print(loading[i]);

				x += 6;
				delay(75);
			}
		}
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}

int offlineX = 64;
const char *offlineMessage = "THIS APP REQUIRES AN INTERNET CONNECTION. SELECT A WIFI NETWORK FROM THE WEB INTERFACE. YOUR IP ADDRESS IS ABOVE.";

void playOfflineAnimation() {
	dma_display->setFont(&Org_01);
	dma_display->setTextWrap(false);
	dma_display->setCursor(12, 7);
	dma_display->setTextColor(dma_display->color565(190, 190, 190));
	dma_display->print("OFFLINE");

	dma_display->setFont(&TomThumb);
	dma_display->setCursor(0, 18);
	dma_display->setTextColor(dma_display->color565(100, 200, 255));
	printCenteredText(WiFi.localIP().toString());

	dma_display->fillRect(0, 24, 64, 5, dma_display->color565(0, 0, 0));
	dma_display->setTextColor(dma_display->color565(255, 255, 255));
	dma_display->setCursor(offlineX, 29);
	dma_display->print(offlineMessage);

	offlineX--;

	// Reset text scroll position
	if (offlineX < -409) {
		offlineX = 64;
	}

	delay(30);
}

const char *unsetTokenMessage = "PLEASE ENTER A VALID FINNHUB API TOKEN FROM THE WEB INTERFACE. YOUR IP ADDRESS IS ABOVE.";

void playUnsetTokenAnimation() {
	dma_display->setFont(&TomThumb);
	dma_display->setTextWrap(false);
	dma_display->setCursor(12, 7);
	dma_display->setTextColor(dma_display->color565(190, 190, 190));
	printCenteredText("TOKEN NOT SET");

	dma_display->setFont(&TomThumb);
	dma_display->setCursor(0, 18);
	dma_display->setTextColor(dma_display->color565(100, 200, 255));
	printCenteredText(WiFi.localIP().toString());

	dma_display->fillRect(0, 24, 64, 5, dma_display->color565(0, 0, 0));
	dma_display->setTextColor(dma_display->color565(255, 255, 255));
	dma_display->setCursor(offlineX, 29);
	dma_display->print(unsetTokenMessage);

	offlineX--;

	// Reset text scroll position
	if (offlineX < -317) {
		offlineX = 64;
	}

	delay(30);
}

const char *invalidTokenMessage = "THE FINNHUB API TOKEN IS NOT VALID. PLEASE ENTER A VALID TOKEN FROM THE WEB INTERFACE. YOUR IP ADDRESS IS ABOVE.";

void playInvalidTokenAnimation() {
	dma_display->setFont(&TomThumb);
	dma_display->setTextWrap(false);
	dma_display->setCursor(12, 7);
	dma_display->setTextColor(dma_display->color565(190, 190, 190));
	printCenteredText("INVALID TOKEN");

	dma_display->setFont(&TomThumb);
	dma_display->setCursor(0, 18);
	dma_display->setTextColor(dma_display->color565(100, 200, 255));
	printCenteredText(WiFi.localIP().toString());

	dma_display->fillRect(0, 24, 64, 5, dma_display->color565(0, 0, 0));
	dma_display->setTextColor(dma_display->color565(255, 255, 255));
	dma_display->setCursor(offlineX, 29);
	dma_display->print(invalidTokenMessage);

	offlineX--;

	// Reset text scroll position
	if (offlineX < -401) {
		offlineX = 64;
	}

	delay(30);
}

const char *stocksMessage = "THE STOCK TICKER CONFIG IS NOT VALID. PLEASE ENTER CORRECT STOCK SYMBOLS FROM THE WEB INTERFACE. YOUR IP ADDRESS IS ABOVE.";

void playStocksAreInvalidAnimation() {
	dma_display->setFont(&TomThumb);
	dma_display->setTextWrap(false);
	dma_display->setCursor(12, 7);
	dma_display->setTextColor(dma_display->color565(190, 190, 190));
	printCenteredText("INVALID STOCKS");

	dma_display->setFont(&TomThumb);
	dma_display->setCursor(0, 18);
	dma_display->setTextColor(dma_display->color565(100, 200, 255));
	printCenteredText(WiFi.localIP().toString());

	dma_display->fillRect(0, 24, 64, 5, dma_display->color565(0, 0, 0));
	dma_display->setTextColor(dma_display->color565(255, 255, 255));
	dma_display->setCursor(offlineX, 29);
	dma_display->print(stocksMessage);

	offlineX--;

	// Reset text scroll position
	if (offlineX < -441) {
		offlineX = 64;
	}

	delay(30);
}

const char *rateLimitMessage = "YOU HAVE REACHED THE RATE LIMIT OF THE FINNHUB API. TRY INCREASING THE API REFRESH INTERVAL FROM THE WEB INTERFACE. VISIT HTTPS://FINNHUB.IO FOR MORE INFORMATION. YOUR IP ADDRESS IS ABOVE.";

void playRateLimitReachedAnimation() {
	dma_display->setFont(&TomThumb);
	dma_display->setTextWrap(false);
	dma_display->setCursor(12, 7);
	dma_display->setTextColor(dma_display->color565(190, 190, 190));
	printCenteredText("RATE LIMITED");

	dma_display->setFont(&TomThumb);
	dma_display->setCursor(0, 18);
	dma_display->setTextColor(dma_display->color565(100, 200, 255));
	printCenteredText(WiFi.localIP().toString());

	dma_display->fillRect(0, 24, 64, 5, dma_display->color565(0, 0, 0));
	dma_display->setTextColor(dma_display->color565(255, 255, 255));
	dma_display->setCursor(offlineX, 29);
	dma_display->print(rateLimitMessage);

	offlineX--;

	// Reset text scroll position
	if (offlineX < -681) {
		offlineX = 64;
	}

	delay(30);
}

const char *connectionIssuesMessage = "UNABLE TO CONNECT TO THE FINNHUB API. YOUR LUXIGRID WILL ATTEMPT TO RECONNECT PERIODICALLY. IF THIS ISSUE PERSISTS, CHECK YOUR INTERNET CONNECTION. YOUR IP ADDRESS IS ABOVE.";

void playConnectionIssuesAnimation() {
	dma_display->setFont(&TomThumb);
	dma_display->setTextWrap(false);
	dma_display->setCursor(12, 7);
	dma_display->setTextColor(dma_display->color565(190, 190, 190));
	printCenteredText("NO CONNECTION");

	dma_display->setFont(&TomThumb);
	dma_display->setCursor(0, 18);
	dma_display->setTextColor(dma_display->color565(100, 200, 255));
	printCenteredText(WiFi.localIP().toString());

	dma_display->fillRect(0, 24, 64, 5, dma_display->color565(0, 0, 0));
	dma_display->setTextColor(dma_display->color565(255, 255, 255));
	dma_display->setCursor(offlineX, 29);
	dma_display->print(connectionIssuesMessage);

	offlineX--;

	// Reset text scroll position
	if (offlineX < -630) {
		offlineX = 64;
	}

	delay(30);
}

const char *noStocksMessage = "NO STOCKS FOUND. PLEASE SET ONE OR MORE STOCKS TO TRACK FROM THE WEB INTERFACE. YOUR IP ADDRESS IS ABOVE.";

void playNoStocksAnimation() {
	dma_display->setFont(&TomThumb);
	dma_display->setTextWrap(false);
	dma_display->setCursor(12, 7);
	dma_display->setTextColor(dma_display->color565(190, 190, 190));
	printCenteredText("NO STOCKS");

	dma_display->setFont(&TomThumb);
	dma_display->setCursor(0, 18);
	dma_display->setTextColor(dma_display->color565(100, 200, 255));
	printCenteredText(WiFi.localIP().toString());

	dma_display->fillRect(0, 24, 64, 5, dma_display->color565(0, 0, 0));
	dma_display->setTextColor(dma_display->color565(255, 255, 255));
	dma_display->setCursor(offlineX, 29);
	dma_display->print(noStocksMessage);

	offlineX--;

	// Reset text scroll position
	if (offlineX < -375) {
		offlineX = 64;
	}

	delay(30);
}

///////////////////
// SETUP FUNCTION
///////////////////
void setup() {
	setupMatrix();

	// App-specific config
	loadStockTickerConfig();
	// Indicate that the app-specific configuration has been loaded
	configIsLoaded = true;

	dma_display->setFont(&Org_01);

	xTaskCreate(
	    playLoadingAnimation,  // Function that should be executed
	    "LoadingAnimation",    // Name of the task (for debugging)
	    2048,                  // Stack size in bytes
	    NULL,                  // Parameter passed into the task
	    1,                     // Task priority
	    NULL                   // Task handle
	);

	// For now, assume we are online until a network issue is encountered
	isOnline = !wifiConfig.isAccessPoint;

	// Assume the token is valid if it's not it's default value
	// If a Finnhub API request returns an error 401, tokenIsValid will be set to false
	tokenIsValid = stockTickerConfig.apiToken != "REPLACE_ME";
	// This is slightly different; used to check if the token is still REPLACE_ME to display a different message to the user
	tokenIsUnset = !tokenIsValid;

	noStocks = stockTickerConfig.numberOfStocks == 0;
}

///////////////////
// MAIN LOOP
///////////////////
void loop() {
	unsigned long currentTime = millis();

	if (!wifiConfig.isAccessPoint && tokenIsValid && !noStocks && stocksAreValid && !rateLimitReached && (previousTime == 0 || (currentTime - previousTime >= stockTickerConfig.refreshInterval && stockTickerConfig.refreshInterval >= 60000))) {
		showLoadingAnimation = true;
		Serial.println("Refreshing Data");

		for (uint8_t x = 0; x < stockTickerConfig.numberOfStocks; x++) {
			if (tokenIsValid && stocksAreValid && !rateLimitReached) {
				refreshStockPrice(stockTickerConfig.stocks[x]);
				delay(150);
			}
		}

		previousTime = currentTime;

		// Stop the loading animation and clear the screen
		showLoadingAnimation = false;
		delay(500);
		dma_display->clearScreen();
	}

	if (wifiConfig.isAccessPoint) {
		playOfflineAnimation();
	} else if (tokenIsUnset) {
		playUnsetTokenAnimation();
	} else if (!tokenIsValid) {
		playInvalidTokenAnimation();
	} else if (noStocks) {
		playNoStocksAnimation();
	} else if (!stocksAreValid) {
		playStocksAreInvalidAnimation();
	} else if (rateLimitReached) {
		playRateLimitReachedAnimation();
	} else if (isOnline) {
		for (uint8_t x = 0; x < stockTickerConfig.numberOfStocks; x++) {
			printStockInfo(stockTickerConfig.stocks[x]);
		}
	} else {
		playConnectionIssuesAnimation();
	}

	delay(30);
}

#endif