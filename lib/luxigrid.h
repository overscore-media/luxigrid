// Include guard
#ifndef LUXIGRID_HEADER_GUARD
#define LUXIGRID_HEADER_GUARD

#include "Arduino.h"

// Set which app is currently loaded
#include "apps.h"

// Library to control the LED matrix
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

// Libraries for SD card and filesystem management
#include "FS.h"
#include "SD.h"
#include "SPI.h"

// Libraries for sensors
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
#include <BH1750.h>

// Libraries for WiFi, the web interface, and OTA Updating
#include <WiFi.h>
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"
#include <Update.h>
#include <DNSServer.h>

// Library for JSON configuration file parsing
#include <ArduinoJson.h>
#include <AsyncJson.h>

// Time Libraries
#include "RTClib.h"
#include "time.h"

// Fonts
#include <Fonts/Org_01.h>
#include <Fonts/TomThumb.h>

// HUB75 Pins
#define R1_PIN 19
#define G1_PIN 2
#define B1_PIN 4

#define R2_PIN 16
#define G2_PIN 15
#define B2_PIN 5

#define A_PIN 18
#define B_PIN 25
#define C_PIN 17
#define D_PIN 33
#define E_PIN 26

#define CLK_PIN 23
#define LAT_PIN 32
#define OE_PIN 0

// Alternate pins for PCB Version 1.30:
// #define A_PIN 18
// #define B_PIN 26  // 26  --> 25
// #define C_PIN 17
// #define D_PIN 25  // 25 --> 33
// #define E_PIN -1  // -1 --> 26

// #define CLK_PIN 23
// #define LAT_PIN 33  // 33 --> 32
// #define OE_PIN 32   // 32 --> 0

// SD Card Pins
#define SPI_SCK 14
#define SPI_MISO 34
#define SPI_MOSI 27
#define SPI_CS 12

// HUB75 panel size constants
#define PANEL_RES_X 64
#define PANEL_RES_Y 32
#define PANEL_CHAIN 1

extern const char *wifiConfigFilename;

// Variable exports
extern MatrixPanel_I2S_DMA *dma_display;
extern AsyncWebServer server;
extern BH1750 lightSensor;
extern Adafruit_BME680 bme680;
extern DNSServer dnsServer;
extern RTC_DS3231 rtc;

extern float temperature;
extern float humidity;
extern uint32_t pressure;

extern uint16_t luxThreshold;
extern uint16_t lux;

extern uint8_t newBrightness;
extern uint8_t currentBrightness;

extern unsigned long lastLightSensorTime;
extern unsigned long lightSensorDelay;

extern unsigned long lastBME680Time;
extern unsigned long bme680Delay;

extern const uint8_t MAX_WIFI_RETRIES;

// Whether the global, time, WiFi, and app-specific config (if applicable) has been fully loaded
extern bool configIsLoaded;

struct WIFIConfig {
	char ssid[33];
	char password[64];
	int8_t retries;
	bool isAccessPoint;
};

struct GlobalConfig {
	char timezone[65];               // 65 means 64 characters and \0
	char humanReadableTimezone[65];  // An assumption based off the fact that the extant timezones aren't this long
	bool disableBH1750;
	uint8_t brightness;  // Manual screen brightness; only relevant if disableBH1750 is true
	char ntpServer[129];
	bool disableNTP;
	uint16_t luxThreshold;
	unsigned long bh1750Delay;
	unsigned long bme680Delay;
	bool is24h;
	bool isCelcius;
};

struct TimeInfo {
	int year;
	int month;
	int day;
	int hour;
	int minute;
	int second;
};

extern WIFIConfig wifiConfig;
extern GlobalConfig globalConfig;

// Setup
void setupLEDMatrix();
void setupSDCard();
void setupWiFi();
void setupWebServer();
void setupLightSensor();
void setupBME680();
void setupTime();
void setupMatrix();

// Animations
void playBootAnimation();
void playWiFiAnimation();
void showWiFiInformation(String ssid, String ipAddressString);
void playOTALoadingAnimation();
void crashWithErrorCode(uint16_t errorCode);

// Utility Functions
TimeInfo getTimeInfo(tm timeStruct);
String humanReadableSize(const uint64_t bytes);
bool stringIsNumeric(const String &str);
String generateRandomString(int length);
bool verifyNTPServer(const char *ntpServer);
void printCenteredText(const String &text, bool centerVertically = false);
void printCenteredTruncatedText(String text, uint16_t margin = 4, String ellipsis = "...");
void setTextColor(uint8_t r, uint8_t g, uint8_t b);
void setPanelBrightness(unsigned long currentMillis);
void getBH1750Readings();
void loadGlobalConfig();
void loadWifiConfig();
void saveWifiConfig();
void saveGlobalConfig();
void runBackgroundTasks(void *pvParameters);
void restart();

#endif