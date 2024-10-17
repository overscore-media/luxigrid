#include "Arduino.h"
#include "../lib/luxigrid.h"

// Include the web server functions
#include "../lib/web_server.hpp"

// Internal variables
SPIClass spi = SPIClass(VSPI);

unsigned long wifiTimeout = 25000;  // 25 seconds

// Exported variables
MatrixPanel_I2S_DMA *dma_display = nullptr;
AsyncWebServer server(80);
BH1750 lightSensor;
Adafruit_BME680 bme680;
DNSServer dnsServer;
RTC_DS3231 rtc;

WIFIConfig wifiConfig;
GlobalConfig globalConfig;

// This means it will attempt a connection once, then retry up to this many times before giving up and starting its own network
const uint8_t MAX_WIFI_RETRIES = 2;

// If there is no app-specific config, consider the config to be loaded at the end of setupMatrix
// Otherwise, apps with app-specific config will have to manage this variable themselves
bool configIsLoaded = false;

void setupLEDMatrix() {
	HUB75_I2S_CFG::i2s_pins _pins = {
	    R1_PIN, G1_PIN, B1_PIN, R2_PIN, G2_PIN, B2_PIN,
	    A_PIN, B_PIN, C_PIN, D_PIN, E_PIN,
	    LAT_PIN, OE_PIN, CLK_PIN};

	HUB75_I2S_CFG mxconfig(PANEL_RES_X, PANEL_RES_Y, PANEL_CHAIN, _pins, HUB75_I2S_CFG::FM6124);

	// mxconfig.latch_blanking = 10;
	// mxconfig.i2sspeed = HUB75_I2S_CFG::HZ_8M;

	dma_display = new MatrixPanel_I2S_DMA(mxconfig);
	dma_display->begin();
	dma_display->clearScreen();
	dma_display->setBrightness(128);
	dma_display->setRotation(0);

	playBootAnimation();
}

void setupSDCard() {
	spi.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SPI_CS);

	if (!SD.begin(SPI_CS, spi, 80000000)) {
		Serial.println("ERROR 1100 - SD Card Mount Failed");

		// ERROR: 1100 - SD Card Mount Failure
		crashWithErrorCode(1100);
	}

	uint8_t cardType = SD.cardType();

	// This is much more unlikely compared to 1100
	if (cardType == CARD_NONE) {
		Serial.println("No SD card Attached");

		// ERROR: 1101 - No SD Card Attached
		crashWithErrorCode(1101);
	}

	Serial.print("SD Card Type: ");

	if (cardType == CARD_MMC) {
		Serial.println("MMC");
	} else if (cardType == CARD_SD) {
		Serial.println("SDSC");
	} else if (cardType == CARD_SDHC) {
		Serial.println("SDHC");
	} else {
		Serial.println("UNKNOWN");
	}

	uint64_t cardSize = SD.cardSize() / (1024 * 1024);

	Serial.printf("SD Card Size: %lluMB\n", cardSize);
	Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
	Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
}

void setupWiFi() {
	String ipAddressString;

	if (wifiConfig.isAccessPoint) {
		WiFi.softAP(wifiConfig.ssid);
		dnsServer.start(53, "*", WiFi.softAPIP());
		ipAddressString = WiFi.softAPIP().toString();
	} else {
		dma_display->setFont(&Org_01);
		dma_display->setTextSize(1);

		// https://github.com/espressif/arduino-esp32/issues/1212#issuecomment-398616159
		// DO NOT TOUCH
		//  This is here to force the ESP32 to reset the WiFi and initialise correctly.
		Serial.print("WIFI status = ");
		Serial.println(WiFi.getMode());
		WiFi.disconnect(true);
		delay(1000);
		WiFi.mode(WIFI_STA);
		delay(1000);
		Serial.print("WIFI status = ");
		Serial.println(WiFi.getMode());
		// End silly stuff !!!

		WiFi.begin(wifiConfig.ssid, wifiConfig.password);

		unsigned long wifiStartTime = millis();

		while (WiFi.status() != WL_CONNECTED) {
			unsigned long currentMillis = millis();

			if (currentMillis - wifiStartTime >= wifiTimeout) {
				// Check if "retries" is over the specified threshold
				if (wifiConfig.retries >= MAX_WIFI_RETRIES) {
					// If it is over the limit, delete configuration file and reboot
					// This will force a new configuration file to be generated, with a random SSID in access-point mode
					SD.remove(wifiConfigFilename);
					restart();
				} else {
					// If not, increment the "retries" here, save the changes to the config file on the SD card, and reboot
					wifiConfig.retries += 1;
					saveWifiConfig();
					restart();
				}
			}

			playWiFiAnimation();
		}

		ipAddressString = WiFi.localIP().toString();
	}

	Serial.println("");
	Serial.print("Connected to ");
	Serial.println(wifiConfig.ssid);
	Serial.print("IP address: ");
	Serial.println(ipAddressString);

	setupWebServer();
	showWiFiInformation(wifiConfig.ssid, ipAddressString);
}

void setupLightSensor() {
	Wire.begin();
	lightSensor.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23);
}

void setupBME680() {
	if (!bme680.begin()) {
		Serial.println("ERROR 3001 - BME680 Sensor Not Found");

		// ERROR: 2001 - BME680 Sensor Not Found
		crashWithErrorCode(3001);
	}

	bme680.setTemperatureOversampling(BME680_OS_8X);
	bme680.setHumidityOversampling(BME680_OS_8X);
	bme680.setPressureOversampling(BME680_OS_4X);
	bme680.setIIRFilterSize(BME680_FILTER_SIZE_3);
	// Disable the gas sensor
	bme680.setGasHeater(0, 0);
}

void setupTime() {
	if (!rtc.begin()) {
		Serial.println("ERROR 4001 - Real-Time Clock Module Not Found");

		// ERROR: 4001 - Real-Time Clock Module Not FOund
		crashWithErrorCode(4001);
	}

	// If the RTC doesn't have any idea what time it is, reset to Jan 1, 2024 at midnight
	if (rtc.lostPower()) {
		rtc.adjust(DateTime(2024, 1, 1, 0, 0, 0));
	}

	// If we have an Internet connection and NTP is not disabled in the global config, sync with the NTP server
	if (!wifiConfig.isAccessPoint && !globalConfig.disableNTP) {
		configTime(0, 0, globalConfig.ntpServer);

		// Since the timezone is not assumed to be set at this point, this will return UTC time
		struct tm utcTime;
		getLocalTime(&utcTime);

		// Adjust the RTC to UTC time, from NTP
		rtc.adjust(DateTime(
		    utcTime.tm_year + 1900,
		    utcTime.tm_mon + 1,
		    utcTime.tm_mday,
		    utcTime.tm_hour,
		    utcTime.tm_min,
		    utcTime.tm_sec));
	}

	// Set the RTC to this to simulate DST rollover
	// rtc.adjust(DateTime(2023, 3, 12, 6, 59, 50));

	// Set the local timezone, with standard POSIX functions
	// If the time is retrieved via standard POSIX functions, it will account for the timezone and DST if applicable
	setenv("TZ", globalConfig.timezone, 1);
	tzset();
}

void setupMatrix() {
	setCpuFrequencyMhz(240);
	Serial.begin(115200);

	setupLEDMatrix();

	setupLightSensor();
	setupBME680();
	setupSDCard();

	loadGlobalConfig();
	loadWifiConfig();

	setupWiFi();
	setupTime();

	// Start the background tasks loop (see runBackgroundTasks in utils.cpp)
	// This way, stuff like the light sensor polling, DNS server processing (if applicable), and other tasks can run independent of the loop() function and delay()
	// Create a task that will run on the second core (-1 means no core affinity, so it can run on any core)
	xTaskCreatePinnedToCore(
	    runBackgroundTasks,   /* Task function */
	    "runBackgroundTasks", /* Name of the task, for debugging purposes */
	    10000,                /* Stack size for the task */
	    NULL,                 /* Parameter to pass to the task */
	    1,                    /* Task priority */
	    NULL,                 /* Task handle */
	    0                     /* Core where the task should run */
	);

	// Reset the display so apps can set their own configuration
	dma_display->clearScreen();
	dma_display->setCursor(0, 0);
	dma_display->setTextSize(1);
	dma_display->setTextColor(dma_display->color565(255, 255, 255));
	dma_display->setFont(NULL);

// If there is no app-specific config, consider everything loaded and ready to go here
// Otherwise, the app will have to specify that its config has been loaded itself
#ifndef APP_SPECIFIC_CONFIG
	configIsLoaded = true;
#endif
}