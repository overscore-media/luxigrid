#include "Arduino.h"
#include "../lib/luxigrid.h"

void drawLuxigridLogo(uint16_t color) {
	// L
	dma_display->drawFastVLine(2, 9, 12, color);
	dma_display->drawFastHLine(3, 20, 5, color);
	// U
	dma_display->drawFastVLine(10, 9, 11, color);
	dma_display->drawFastHLine(11, 20, 4, color);
	dma_display->drawFastVLine(15, 9, 11, color);
	// X
	dma_display->drawPixel(23, 9, color);
	dma_display->drawPixel(18, 20, color);
	dma_display->drawFastVLine(18, 10, 4, color);
	dma_display->drawFastVLine(19, 12, 3, color);
	dma_display->drawFastVLine(20, 13, 3, color);
	dma_display->drawFastVLine(21, 14, 3, color);
	dma_display->drawFastVLine(22, 15, 3, color);
	dma_display->drawFastVLine(23, 16, 4, color);
	dma_display->drawFastVLine(19, 18, 2, color);
	dma_display->drawFastVLine(22, 10, 2, color);
	// I
	dma_display->drawFastHLine(26, 9, 5, color);
	dma_display->drawFastVLine(28, 10, 10, color);
	dma_display->drawFastHLine(26, 20, 5, color);
	// G
	dma_display->drawFastVLine(33, 10, 10, color);
	dma_display->drawFastHLine(34, 9, 4, color);
	dma_display->drawPixel(38, 10, color);
	dma_display->drawPixel(37, 16, color);
	dma_display->drawFastVLine(38, 16, 4, color);
	dma_display->drawFastHLine(34, 20, 4, color);
	// R
	dma_display->drawFastVLine(41, 9, 12, color);
	dma_display->drawFastHLine(42, 9, 3, color);
	dma_display->drawPixel(45, 10, color);
	dma_display->drawFastVLine(46, 11, 3, color);
	dma_display->drawPixel(45, 14, color);
	dma_display->drawFastHLine(42, 15, 3, color);
	dma_display->drawPixel(45, 16, color);
	dma_display->drawFastVLine(46, 17, 4, color);
	// I
	dma_display->drawFastHLine(49, 9, 5, color);
	dma_display->drawFastVLine(51, 10, 10, color);
	dma_display->drawFastHLine(49, 20, 5, color);
	// D
	dma_display->drawFastVLine(56, 9, 12, color);
	dma_display->drawFastHLine(57, 9, 3, color);
	dma_display->drawPixel(60, 10, color);
	dma_display->drawFastVLine(61, 11, 8, color);
	dma_display->drawPixel(60, 19, color);
	dma_display->drawFastHLine(57, 20, 3, color);
}

// Gets the next colour for the boot logo to do the rainbow effect
void getNextLogoColour(uint8_t &r, uint8_t &g, uint8_t &b) {
	const uint8_t stepSize = 85;

	if (r == 255 && g < 255 && b == 0) {
		// Red to yellow
		g += stepSize;
	} else if (g == 255 && r > 0) {
		// Yellow to green
		r -= stepSize;
	} else if (g == 255 && b < 255) {
		// Green to cyan
		b += stepSize;
	} else if (b == 255 && g > 0) {
		// Cyan to blue
		g -= stepSize;
	} else if (b == 255 && r < 255) {
		// Blue to magenta
		r += stepSize;
	} else if (r == 255 && b > 0) {
		// Magenta back to red
		b -= stepSize;
	} else {
		// Reset to red if something weird happens
		r = 255;  // Reset to start from red if out of sync
		g = b = 0;
	}
}

void playBootAnimation() {
	dma_display->clearScreen();

	uint8_t r, g, b;

	// Run through the colours first
	for (uint8_t i = 0; i < 16; i++) {
		getNextLogoColour(r, g, b);
		uint16_t color = dma_display->color565(r, g, b);
		drawLuxigridLogo(color);
		delay(30);
	}

	// Then draw a white version, and keep it for 1.5 seconds
	drawLuxigridLogo(dma_display->color565(255, 255, 255));
	delay(1500);
	dma_display->clearScreen();
}

void playWiFiAnimation() {
	dma_display->clearScreen();
	dma_display->setFont(&Org_01);

	// Frame 1
	setTextColor(255, 255, 255);
	printCenteredText("WiFi", true);

	dma_display->setCursor(dma_display->getCursorX() + 1, dma_display->getCursorY());
	setTextColor(100, 200, 255);
	dma_display->print(">");
	setTextColor(255, 255, 255);
	dma_display->print(">");
	dma_display->print(">");

	dma_display->setCursor(dma_display->getCursorX() - 42, dma_display->getCursorY());
	dma_display->print("<");
	dma_display->print("<");
	setTextColor(100, 200, 255);
	dma_display->print("<");
	setTextColor(255, 255, 255);

	// Reset
	delay(250);
	dma_display->clearScreen();

	// Frame 2
	setTextColor(255, 255, 255);
	printCenteredText("WiFi", true);

	dma_display->setCursor(dma_display->getCursorX() + 1, dma_display->getCursorY());
	setTextColor(100, 200, 255);
	dma_display->print(">");
	dma_display->print(">");
	setTextColor(255, 255, 255);
	dma_display->print(">");

	dma_display->setCursor(dma_display->getCursorX() - 42, dma_display->getCursorY());
	dma_display->print("<");
	setTextColor(100, 200, 255);
	dma_display->print("<");
	dma_display->print("<");

	// Reset
	delay(250);
	dma_display->clearScreen();

	// Frame 3
	setTextColor(255, 255, 255);
	printCenteredText("WiFi", true);

	dma_display->setCursor(dma_display->getCursorX() + 1, dma_display->getCursorY());
	dma_display->print(">");
	setTextColor(100, 200, 255);
	dma_display->print(">");
	dma_display->print(">");
	setTextColor(255, 255, 255);

	dma_display->setCursor(dma_display->getCursorX() - 42, dma_display->getCursorY());
	setTextColor(100, 200, 255);
	dma_display->print("<");
	dma_display->print("<");
	setTextColor(255, 255, 255);
	dma_display->print("<");

	// Reset
	delay(250);
	dma_display->clearScreen();

	// Frame 4
	setTextColor(255, 255, 255);
	printCenteredText("WiFi", true);

	dma_display->setCursor(dma_display->getCursorX() + 1, dma_display->getCursorY());
	setTextColor(255, 255, 255);
	dma_display->print(">");
	dma_display->print(">");
	setTextColor(100, 200, 255);
	dma_display->print(">");

	dma_display->setCursor(dma_display->getCursorX() - 42, dma_display->getCursorY());
	setTextColor(100, 200, 255);
	dma_display->print("<");
	setTextColor(255, 255, 255);
	dma_display->print("<");
	dma_display->print("<");

	// Reset
	delay(250);
	setTextColor(255, 255, 255);
	dma_display->clearScreen();
}

void showWiFiInformation(String ssid, String ipAddressString) {
	dma_display->clearScreen();
	dma_display->setFont(&Org_01);

	setTextColor(100, 200, 255);
	dma_display->setCursor(2, 6);
	dma_display->print("WiFi");
	dma_display->setCursor(21, 6);
	dma_display->print("NETWORK");
	dma_display->setCursor(0, 14);
	dma_display->setFont(&TomThumb);
	setTextColor(255, 255, 255);

	printCenteredTruncatedText(ssid);

	dma_display->setFont(&Org_01);
	dma_display->setCursor(2, 22);
	setTextColor(100, 200, 255);
	dma_display->print("IP");
	dma_display->setCursor(21, 22);
	dma_display->print("ADDRESS");
	setTextColor(255, 255, 255);

	dma_display->setFont(&TomThumb);
	dma_display->setCursor(0, 30);
	printCenteredText(ipAddressString);

	int pulseTimes = 2;       // Number of times the rectangle should pulse
	int maxBrightness = 255;  // Maximum brightness level
	int pulseSpeed = 2;       // Speed of the pulse animation

	// Pulsing rectangle animation
	for (int j = 0; j < pulseTimes; j++) {
		// Fade in
		for (int i = 0; i <= maxBrightness; i += pulseSpeed) {
			// If an OTA update is in progress, stop this animation
			if (otaUpdateInProgress) {
				vTaskDelay(10 / portTICK_PERIOD_MS);
				break;
			}

			dma_display->drawRect(0, 0, 64, 32, dma_display->color565(i, i, i));  // Draw rectangle with increasing brightness
			delay(10);                                                            // Short delay to see the animation
		}

		// Fade out
		for (int i = maxBrightness; i >= 0; i -= pulseSpeed) {
			// If an OTA update is in progress, stop this animation
			if (otaUpdateInProgress) {
				vTaskDelay(10 / portTICK_PERIOD_MS);
				break;
			}

			dma_display->drawRect(0, 0, 64, 32, dma_display->color565(i, i, i));  // Draw rectangle with decreasing brightness
			delay(10);                                                            // Short delay to see the animation
		}
	}

	dma_display->clearScreen();
}

// Gets a colour somewhere between pink and green, for a progress between 0 and 100
uint16_t getOTALoadingAnimationProgressColour() {
	uint8_t value = (otaUpdatePercentComplete > 100) ? 100 : otaUpdatePercentComplete;

	// Pink: RGB(255, 100, 175)
	uint8_t startR = 255, startG = 100, startB = 175;
	// Green: RGB(0, 255, 0)
	uint8_t endR = 0, endG = 255, endB = 0;

	uint8_t r = startR + ((endR - startR) * value) / 100;
	uint8_t g = startG + ((endG - startG) * value) / 100;
	uint8_t b = startB + ((endB - startB) * value) / 100;

	return dma_display->color565(r, g, b);
}

// Play the animation shown while an OTA update is in progress
void playOTALoadingAnimation() {
	dma_display->setTextColor(dma_display->color565(255, 255, 255));
	dma_display->setFont(&Org_01);
	printCenteredText("UPDATING...", true);

	// Black rectangle behind the text (alternative to clearing the whole screen)
	dma_display->fillRect(23, 22, 17, 5, dma_display->color565(0, 0, 0));
	// Display the upload progress
	dma_display->setCursor(dma_display->getCursorX(), 26);
	dma_display->setTextColor(getOTALoadingAnimationProgressColour());
	printCenteredText(String(otaUpdatePercentComplete) + "%");

	vTaskDelay(250 / portTICK_PERIOD_MS);
}

void crashWithErrorCode(uint16_t errorCode) {
	dma_display->clearScreen();
	dma_display->setFont(&Org_01);

	// Triangle
	dma_display->fillTriangle(32, 2, 20, 22, 44, 22, dma_display->color565(255, 255, 0));

	// Error text and code
	dma_display->setTextColor(dma_display->color565(255, 0, 0));
	dma_display->setCursor(0, 29);
	printCenteredText("Error " + String(errorCode));

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
