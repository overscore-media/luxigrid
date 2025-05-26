#ifndef LUXIGRID_WEB_SERVER_GUARD
#define LUXIGRID_WEB_SERVER_GUARD

#include "Arduino.h"

#include "web_ui.h"

// Initially, no OTA update will be in progress
// But it will be set in the updateFirmware method below, when applicable
bool otaUpdateInProgress = false;
uint8_t otaUpdatePercentComplete = 0;

// When this is set, the ESP32 will restart in approximately 1.5s
bool shouldRestart = false;

// TODO: Test this thoroughly
bool isUpdating = false;
size_t totalFirmwareSize = 0;
size_t bytesReceived = 0;

void getSDFiles(AsyncWebServerRequest *request, String path = "/") {
	AsyncResponseStream *response = request->beginResponseStream("application/json");
	JsonDocument jsonDoc;

	if (request->hasParam("path")) {
		path = request->getParam("path")->value();

		// Ensure the path does not end with '/', unless it's the root
		if (path != "/" && path.endsWith("/")) {
			path = path.substring(0, path.length() - 1);
		}
	}

	File dir = SD.open(path);

	if (!dir || !dir.isDirectory()) {
		request->send(400, "text/plain", "Bad Request: Path is not a directory");
		if (dir) {
			dir.close();
		}
		return;
	}

	jsonDoc["path"] = path;

	if (path != "/") {
		String parentPath = path.substring(0, path.lastIndexOf('/'));
		if (parentPath == "") {
			parentPath = "/";
		}
		jsonDoc["parentPath"] = parentPath;
	} else {
		jsonDoc["parentPath"] = nullptr;
	}

	File file = dir.openNextFile();

	while (file) {
		String fileName = file.name();
		String displayName = fileName.substring(fileName.lastIndexOf("/") + 1);
		uint64_t fileSize = static_cast<uint64_t>(file.size());

		// Skip hidden files
		if (!displayName.startsWith(".")) {
			String fullPath = path + (path == "/" ? "" : "/") + displayName;
			JsonObject fileInfo = jsonDoc["files"].add<JsonObject>();
			fileInfo["name"] = displayName;
			fileInfo["path"] = fullPath;
			fileInfo["isDir"] = file.isDirectory();
			fileInfo["size"] = humanReadableSize(fileSize);
		}

		file.close();
		file = dir.openNextFile();
	}

	// Disallow caching on this route
	response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");

	// Send the constructed JSON data to the client
	serializeJson(jsonDoc, *response);
	request->send(response);
}

// Given a file path, recursively delete it (i.e., if it's a directory, delete all files and subdirectories)
void recursivelyDelete(String path) {
	File dir = SD.open(path);

	// If the path points to a file (and not a directory) simply delete that file
	if (!dir.isDirectory()) {
		dir.close();
		SD.remove(path.c_str());
		return;
	}

	// If the path points to a directory
	File file = dir.openNextFile();

	while (file) {
		String filePath = path + '/' + file.name();

		// Recursively delete subdirectories, removing all of their files and finally removing the directory itself
		if (file.isDirectory()) {
			file.close();
			recursivelyDelete(filePath);
		} else {
			file.close();
			SD.remove(filePath.c_str());
		}

		file = dir.openNextFile();
	}

	dir.close();
	// Delete the now-empty directory
	SD.rmdir(path.c_str());
}

// For uploading files onto the SD card
void handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
	// If this is the first chunk, initialize the file upload process
	if (index == 0) {
		if (!request->hasParam("path", true)) {
			request->send(400, "text/plain", "Please specify a path");
			return;
		}

		const AsyncWebParameter *path = request->getParam("path", true);

		// Open the file for writing (create if it doesn't exist or truncate it if it does)
		File file = SD.open((path->value() + (path->value().endsWith("/") ? "" : "/") + filename), FILE_WRITE);

		if (!file) {
			request->send(400, "text/plain", "File upload failed");
			return;
		}

		// Store the file handle in a way that allows access in subsequent calls
		request->_tempFile = file;
	}

	// If there is data, write it to the file
	if (len) {
		File file = request->_tempFile;
		file.write(data, len);
	}

	// If this is the last chunk, finalize the upload
	if (final) {
		File file = request->_tempFile;
		file.close();
		request->send(200, "text/plain", "File upload succeeded");
	}
}

// This function initializes the web server's API routes
void setupWebServer() {
	// Route to serve the main web interface
	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
		AsyncWebServerResponse *response = request->beginResponse(200, "text/html", updatePage, updatePageLength);
		response->addHeader("Content-Encoding", "gzip");
		request->send(response);
	});

	// Route to check if the device is online, and fully loaded
	server.on("/health", HTTP_GET, [](AsyncWebServerRequest *request) {
		if (configIsLoaded) {
			request->send(200);
		} else {
			request->send(400);
		}
	});

	// Route to serve the interface for the SD card file browser
	server.on("/browser", HTTP_GET, [](AsyncWebServerRequest *request) {
		getSDFiles(request);
	});

	// Router to get info about the total/used/free space on the SD card
	server.on("/sdinfo", HTTP_GET, [](AsyncWebServerRequest *request) {
		AsyncResponseStream *response = request->beginResponseStream("application/json");
		JsonDocument jsonDoc;

		uint64_t totalBytes = SD.totalBytes();
		uint64_t usedBytes = SD.usedBytes();
		uint64_t freeBytes = totalBytes - usedBytes;

		jsonDoc["total"] = humanReadableSize(totalBytes);
		jsonDoc["used"] = humanReadableSize(usedBytes);
		jsonDoc["free"] = humanReadableSize(freeBytes);

		// Disallow caching on this route
		response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");

		// Send the constructed JSON data to the client
		serializeJson(jsonDoc, *response);
		request->send(response);
	});

	// This is an annoying necessity when testing file uploads from a different origin (like localhost)
	server.on("/upload", HTTP_OPTIONS, [](AsyncWebServerRequest *request) {
		request->send(200);
	});

	// Route to upload files to the SD card
	server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request) { request->send(200); }, handleFileUpload);

	// Route to create a folder on the SD card
	server.on("/createFolder", HTTP_POST, [](AsyncWebServerRequest *request) {
		String name, type, currentPath;

		bool folderError = false;

		// The request must have the "name" and "currentPath" query parameters
		if (request->hasParam("name") && request->hasParam("currentPath")) {
			name = request->getParam("name")->value();
			currentPath = request->getParam("currentPath")->value();

			// Ensure the path ends with "/"
			if (!currentPath.endsWith("/")) {
				currentPath += "/";
			}

			String fullPath = currentPath + name;

			if (!SD.mkdir(fullPath)) {
				folderError = true;
			}
		} else {
			folderError = true;
		}

		if (folderError) {
			request->send(400, "text/plain", "Folder creation failed");
		} else {
			request->send(200, "text/plain", "Folder created successfully");
		}
	});

	// Route to download a file on the SD card
	server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request) {
		if (!request->hasParam("file")) {
			request->send(400, "text/plain", "Bad Request: No file specified");
			return;
		}

		String filePath = request->getParam("file")->value();

		if (!filePath.startsWith("/")) {
			filePath = "/" + filePath;
		}

		if (!SD.exists(filePath)) {
			Serial.println("File Not Found");
			Serial.println(filePath);

			request->send(404, "text/plain", "Not Found: File does not exist");
			return;
		}

		File file = SD.open(filePath, FILE_READ);

		if (!file || file.isDirectory()) {
			request->send(400, "text/plain", "Bad Request: Not a file");
			if (file) file.close();
			return;
		}

		// Send "application/octet-stream" as the Content-Type because it's not worth looking up the mimetype
		// Most browsers will guess the mimetype based on the file extension, anyway
		request->send(SD, filePath, "application/octet-stream");
		file.close();
	});

	// Route to delete a file (or folder) on the SD card
	server.on("/delete", HTTP_POST, [](AsyncWebServerRequest *request) {
		if (!request->hasParam("path", true)) {
			request->send(400, "text/plain", "Please specify a file/folder to delete");
			return;
		}

		String path = request->getParam("path", true)->value();

		// Ensure the path is not the root directory
		if (path == "/") {
			request->send(403, "text/plain", "Cannot delete the root folder");
			return;
		}

		File file = SD.open(path);

		if (!file) {
			request->send(404, "text/plain", "The specified file or folder does not exist");
			return;
		}

		file.close();
		recursivelyDelete(path);

		String parentPath = path.substring(0, path.lastIndexOf('/'));

		// Ensure parent path defaults to root if it's empty
		if (parentPath == "") {
			parentPath = "/";
		}

		request->send(200, "text/plain", "File/folder deleted successfully");
	});

	// Route for handling WiFi config updates
	server.on("/wifi", HTTP_POST, [](AsyncWebServerRequest *request) {
		bool shouldSaveConfig = false;

		if (request->hasParam("ssid", true)) {
			const AsyncWebParameter *ssid = request->getParam("ssid", true);

			const char *ssidString = ssid->value().c_str();

			if (strlen(ssidString) > 2 && strlen(ssidString) <= 32) {
				strlcpy(wifiConfig.ssid, ssidString, sizeof(wifiConfig.ssid));
				shouldSaveConfig = true;
			} else {
				request->send(400, "text/plain", "WiFi SSID configuration is invalid");
				shouldRestart = true;
				return;
			}
		}

		if (request->hasParam("password", true)) {
			const AsyncWebParameter *password = request->getParam("password", true);

			const char *passwordString = password->value().c_str();

			if (strlen(passwordString) <= 63) {
				strlcpy(wifiConfig.password, passwordString, sizeof(wifiConfig.password));
				shouldSaveConfig = true;
			} else {
				request->send(400, "text/plain", "WiFi Password configuration is invalid");
				shouldRestart = true;
				return;
			}
		}

		AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "OK");
		response->addHeader("Connection", "close");
		response->addHeader("Access-Control-Allow-Origin", "*");
		request->send(response);

		if (shouldSaveConfig) {
			Serial.println("Saving WiFi Config here...");
			Serial.println(wifiConfig.ssid);
			Serial.println(wifiConfig.password);

			wifiConfig.retries = 0;
			wifiConfig.isAccessPoint = false;

			saveWifiConfig();
			shouldRestart = true;
			return;
		}
	});

	// Route to manually update the time
	server.on("/time", HTTP_POST, [](AsyncWebServerRequest *request) {
		if (request->hasParam("timestamp", true)) {
			long long timestamp;
			const AsyncWebParameter *timestampParameter = request->getParam("timestamp", true);

			const char *timestampString = timestampParameter->value().c_str();
			char *endPtr;
			errno = 0;

			// The strtoll function converts the string to a long long
			timestamp = strtoll(timestampString, &endPtr, 10);

			// Send back an error response if an error was thrown
			if (endPtr == timestampString || errno == ERANGE || timestamp < 0) {
				request->send(400, "text/plain", "Time and Date update configuration is invalid");
				shouldRestart = true;
				return;
			} else {
				// Update the RTC with the timestamp, if it is valid
				Serial.println("Received valid timestamp: ");
				Serial.println(timestampString);

				// Assume the timestamp was in milliseconds since 1970
				long long timestampSeconds = timestamp / 1000;
				rtc.adjust(DateTime(static_cast<uint32_t>(timestampSeconds)));
			}
		}

		AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "OK");
		response->addHeader("Connection", "close");
		response->addHeader("Access-Control-Allow-Origin", "*");
		request->send(response);
	});

	// Route to perform an OTA firmware update
	server.on(
	    "/updateFirmware", HTTP_PUT,

	    [](AsyncWebServerRequest *request) {
		    AsyncWebServerResponse *response = request->beginResponse((Update.hasError()) ? 500 : 200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
		    response->addHeader("Connection", "close");
		    response->addHeader("Access-Control-Allow-Origin", "*");
		    request->send(response);
		    shouldRestart = true;
		    return;
	    },

	    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
		    size_t *totalSize;

		    // If this is the first chunk, initialize the file upload process
		    if (index == 0) {
			    // Allocate and store totalSize in the request context
			    request->_tempObject = new size_t(request->contentLength());
			    totalSize = (size_t *)request->_tempObject;

			    // Set the OTA Update in Progress flag
			    otaUpdatePercentComplete = 0;
			    otaUpdateInProgress = true;
			    dma_display->clearScreen();

			    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
				    request->send(400, "text/plain", "OTA could not begin");
				    shouldRestart = true;
				    return;
			    }
		    } else {
			    // For subsequent chunks, get the totalSize here
			    totalSize = (size_t *)request->_tempObject;
		    }

		    // Just in case totalSize wasn't set above, cancel the request with an error here (better than crashing over a null pointer issue)
		    if (totalSize == nullptr) {
			    // Since the ESP32 is not being restarted, unset the OTA Update in Progress flag
			    otaUpdateInProgress = false;
			    return request->send(500, "text/plain", "A server error occurred");
		    }

		    if (len) {
			    if (Update.write(data, len) != len) {
				    request->send(400, "text/plain", "OTA could not begin");
				    shouldRestart = true;
				    return;
			    }

			    float progress = (index + len) / (float)(*totalSize) * 100.0;

			    // Update the on-screen loading progress indicator
			    otaUpdatePercentComplete = static_cast<uint8_t>(round(progress));
		    }

		    if (final) {
			    if (!Update.end(true)) {
				    Update.printError(Serial);
				    request->send(400, "text/plain", "Could not end OTA");
				    shouldRestart = true;
				    return;
			    }
		    } else {
			    return;
		    }
	    });

	// TODO: Better error handling
	// TODO: Only one chunk at a time
	// TODO: Timeout if last part never received
	// TODO: Progress indicator
	// TODO: JS Code and testing

	// Route to perform an OTA firmware update
	server.on(
	    "/updateFirmware", HTTP_POST,

	    [](AsyncWebServerRequest *request) {
		    AsyncWebServerResponse *response = request->beginResponse((Update.hasError()) ? 500 : 200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");

		    response->addHeader("Connection", "close");
		    response->addHeader("Access-Control-Allow-Origin", "*");
		    request->send(response);

		    if (Update.hasError()) {
			    shouldRestart = true;
		    }

		    isUpdating = false;
	    },

	    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
		    if (index == 0) {
			    if (request->hasHeader("X-Firmware-Size")) {
				    if (bytesReceived != 0) {
					    request->send(400, "text/plain", "Update failed");
					    shouldRestart = true;
					    return;
				    }

				    totalFirmwareSize = request->getHeader("X-Firmware-Size")->value().toInt();

				    if (!Update.begin(totalFirmwareSize)) {
					    request->send(400, "text/plain", "Failed to start update");
					    return;
				    }

				    // Set the OTA Update in Progress flag
				    otaUpdatePercentComplete = 0;
				    otaUpdateInProgress = true;
				    dma_display->clearScreen();
			    } else if (bytesReceived == 0) {
				    request->send(400, "text/plain", "Missing firmware size header");
				    return;
			    }
		    }

		    if (len) {
			    size_t amount_written = Update.write(data, len);
			    bytesReceived += amount_written;
			    if (len != amount_written) {
				    request->send(400, "text/plain", "OTA update failed");
				    shouldRestart = true;
				    return;
			    }

			    float progress = bytesReceived / (float)totalFirmwareSize * 100.0;

			    // Update the on-screen loading progress indicator
			    otaUpdatePercentComplete = static_cast<uint8_t>(round(progress));
		    }

		    Serial.print("Written ");
		    Serial.print(bytesReceived);
		    Serial.print(" of ");
		    Serial.println(totalFirmwareSize);

		    if (bytesReceived == totalFirmwareSize) {
			    if (!final) {
				    request->send(400, "text/plain", "OTA update failed to complete");
				    shouldRestart = true;
				    return;
			    } else if (!Update.end(true)) {
				    Update.printError(Serial);
				    request->send(400, "text/plain", "Could not end OTA");
				    shouldRestart = true;
				    return;
			    } else {
				    request->send(200, "text/plain", "OTA Update successful");
				    shouldRestart = true;
			    }
		    }
	    });

	// Route to get the current config
	server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request) {
		AsyncResponseStream *response = request->beginResponseStream("application/json");
		JsonDocument jsonDoc;

		// Wifi Config
		jsonDoc["wifi"]["ssid"] = wifiConfig.ssid;
		jsonDoc["wifi"]["password"] = wifiConfig.password;
		jsonDoc["wifi"]["retries"] = wifiConfig.retries;
		jsonDoc["wifi"]["isAccessPoint"] = wifiConfig.isAccessPoint;

		// Global Config
		jsonDoc["global"]["timezone"] = globalConfig.timezone;
		jsonDoc["global"]["humanReadableTimezone"] = globalConfig.humanReadableTimezone;
		jsonDoc["global"]["disableBH1750"] = globalConfig.disableBH1750;
		jsonDoc["global"]["brightness"] = globalConfig.brightness;
		jsonDoc["global"]["ntpServer"] = globalConfig.ntpServer;
		jsonDoc["global"]["disableNTP"] = globalConfig.disableNTP;
		jsonDoc["global"]["luxThreshold"] = globalConfig.luxThreshold;
		jsonDoc["global"]["bh1750Delay"] = globalConfig.bh1750Delay;
		jsonDoc["global"]["bme680Delay"] = globalConfig.bme680Delay;
		jsonDoc["global"]["is24h"] = globalConfig.is24h;
		jsonDoc["global"]["isCelcius"] = globalConfig.isCelcius;

// If there is app-specific config, handle adding it to the JSON object that will be returned to the client
#ifdef APP_SPECIFIC_CONFIG
		void retrieveAppConfig(JsonDocument & jsonDoc);
		retrieveAppConfig(jsonDoc);
#endif
		// Send the constructed JSON data to the client
		serializeJson(jsonDoc, *response);
		request->send(response);
	});

	// Route to update the config
	server.on("/config", HTTP_POST, [](AsyncWebServerRequest *request) {
		// If this boolean flag is flipped, the configuration will be updated, and the ESP32 will restart
		bool shouldSaveConfig = false;

		if (request->hasParam("isCelcius", true)) {
			const AsyncWebParameter *isCelcius = request->getParam("isCelcius", true);

			if (isCelcius->value() == "true") {
				globalConfig.isCelcius = true;
				shouldSaveConfig = true;
			} else if (isCelcius->value() == "false") {
				globalConfig.isCelcius = false;
				shouldSaveConfig = true;
			} else {
				request->send(400, "text/plain", "Temperature Format configuration is invalid");
				shouldRestart = true;
				return;
			}
		}

		if (request->hasParam("is24h", true)) {
			const AsyncWebParameter *is24h = request->getParam("is24h", true);

			if (is24h->value() == "true") {
				globalConfig.is24h = true;
				shouldSaveConfig = true;
			} else if (is24h->value() == "false") {
				globalConfig.is24h = false;
				shouldSaveConfig = true;
			} else {
				request->send(400, "text/plain", "Temperature Format configuration is invalid");
				shouldRestart = true;
				return;
			}
		}

		// timezone and humanReadableTimezone go hand in hand, so they must both be present on the request
		if ((request->hasParam("timezone", true) && !request->hasParam("humanReadableTimezone", true)) || (!request->hasParam("timezone", true) && request->hasParam("humanReadableTimezone", true))) {
			request->send(400, "text/plain", "Timezone configuration is invalid");
			shouldRestart = true;
			return;
		}

		if (request->hasParam("timezone", true)) {
			const AsyncWebParameter *timezone = request->getParam("timezone", true);
			const AsyncWebParameter *humanReadableTimezone = request->getParam("humanReadableTimezone", true);

			const char *timezoneString = timezone->value().c_str();
			const char *humanReadableTimezoneString = humanReadableTimezone->value().c_str();

			if (strlen(timezoneString) == 0 || strlen(humanReadableTimezoneString) == 0 || strlen(timezoneString) >= sizeof(globalConfig.timezone) || strlen(humanReadableTimezoneString) >= sizeof(globalConfig.humanReadableTimezone)) {
				request->send(400, "text/plain", "Timezone configuration is invalid");
				shouldRestart = true;
				return;
			}

			// Note how these aren't checked against some database, since we don't really have the space
			// But at least they're both going to be valid strings that fit in the blanks
			strlcpy(globalConfig.timezone, timezoneString, sizeof(globalConfig.timezone));
			strlcpy(globalConfig.humanReadableTimezone, humanReadableTimezoneString, sizeof(globalConfig.humanReadableTimezone));
			shouldSaveConfig = true;
		}

		if (request->hasParam("disableBH1750", true)) {
			const AsyncWebParameter *disableBH1750 = request->getParam("disableBH1750", true);

			if (disableBH1750->value() == "true") {
				globalConfig.disableBH1750 = true;
				shouldSaveConfig = true;
			} else if (disableBH1750->value() == "false") {
				globalConfig.disableBH1750 = false;
				shouldSaveConfig = true;
			} else {
				request->send(400, "text/plain", "Disable Light Sensor configuration is invalid");
				shouldRestart = true;
				return;
			}
		}

		if (request->hasParam("brightness", true)) {
			const AsyncWebParameter *brightness = request->getParam("brightness", true);

			if (!stringIsNumeric(brightness->value())) {
				request->send(400, "text/plain", "Manual Brightness configuration is invalid");
				shouldRestart = true;
				return;
			}

			int brightnessToInt = brightness->value().toInt();

			if (brightnessToInt >= 0 && brightnessToInt <= 255) {
				globalConfig.brightness = static_cast<uint8_t>(brightnessToInt);
				shouldSaveConfig = true;
			} else {
				request->send(400, "text/plain", "Manual Brightness configuration is invalid");
				shouldRestart = true;
				return;
			}
		}

		if (request->hasParam("luxThreshold", true)) {
			const AsyncWebParameter *luxThreshold = request->getParam("luxThreshold", true);

			if (!stringIsNumeric(luxThreshold->value())) {
				request->send(400, "text/plain", "Lux Threshold configuration is invalid");
				shouldRestart = true;
				return;
			}

			int luxThresholdToInt = luxThreshold->value().toInt();

			if (luxThresholdToInt >= 0 && luxThresholdToInt <= 65535) {
				globalConfig.luxThreshold = static_cast<uint16_t>(luxThresholdToInt);
				shouldSaveConfig = true;
			} else {
				request->send(400, "text/plain", "Lux Threshold configuration is invalid");
				shouldRestart = true;
				return;
			}
		}

		if (request->hasParam("bh1750Delay", true)) {
			const AsyncWebParameter *bh1750Delay = request->getParam("bh1750Delay", true);

			if (!stringIsNumeric(bh1750Delay->value())) {
				request->send(400, "text/plain", "Light Sensor Measurement Delay configuration is invalid");
				shouldRestart = true;
				return;
			}

			unsigned long bh1750DelayToInt = strtoul(bh1750Delay->value().c_str(), nullptr, 10);

			// 3600000 is 1 hour in milliseconds; absolutely no sense in a value anywhere near this high anyway
			// But it's an unsigned long so time calculations make sense (although maybe a 32-bit integer would do, but whatever)
			if (bh1750DelayToInt <= 3600000) {
				globalConfig.bh1750Delay = bh1750DelayToInt;
				shouldSaveConfig = true;
			} else {
				request->send(400, "text/plain", "Light Sensor Measurement Delay configuration is invalid");
				shouldRestart = true;
				return;
			}
		}

		if (request->hasParam("bme680Delay", true)) {
			const AsyncWebParameter *bme680Delay = request->getParam("bme680Delay", true);

			if (!stringIsNumeric(bme680Delay->value())) {
				request->send(400, "text/plain", "Temperature/Humidity Sensor Measurement Delay configuration is invalid");
				shouldRestart = true;
				return;
			}

			unsigned long bme680DelayToInt = strtoul(bme680Delay->value().c_str(), nullptr, 10);

			// 3600000 is 1 hour in milliseconds; absolutely no sense in a value anywhere near this high anyway
			// But it's an unsigned long so time calculations make sense (although maybe a 32-bit integer would do, but whatever)
			if (bme680DelayToInt <= 3600000) {
				globalConfig.bme680Delay = bme680DelayToInt;
				shouldSaveConfig = true;
			} else {
				request->send(400, "text/plain", "Temperature/Humidity Sensor Measurement Delay configuration is invalid");
				shouldRestart = true;
				return;
			}
		}

		if (request->hasParam("ntpServer", true)) {
			const AsyncWebParameter *ntpServer = request->getParam("ntpServer", true);

			const char *ntpServerString = ntpServer->value().c_str();

			if (strlen(ntpServerString) == 0 || strlen(ntpServerString) >= sizeof(globalConfig.ntpServer)) {
				request->send(400, "text/plain", "Custom NTP Server configuration is invalid");
				shouldRestart = true;
				return;
			}

			// Note how the NTP server is actually checked here; so this will fail if there is no Internet connection
			// But that's not a huge deal, since an NTP server doesn't mean much if you don't have an Internet connection
			if (!verifyNTPServer(ntpServerString)) {
				request->send(400, "text/plain", "Custom NTP Server is not accessible");
				shouldRestart = true;
				return;
			}

			strlcpy(globalConfig.ntpServer, ntpServerString, sizeof(globalConfig.ntpServer));
			shouldSaveConfig = true;
		}

		if (request->hasParam("disableNTP", true)) {
			const AsyncWebParameter *disableNTP = request->getParam("disableNTP", true);

			if (disableNTP->value() == "true") {
				globalConfig.disableNTP = true;
				shouldSaveConfig = true;
			} else if (disableNTP->value() == "false") {
				globalConfig.disableNTP = false;
				shouldSaveConfig = true;
			} else {
				request->send(400, "text/plain", "Disable NTP configuration is invalid");
				shouldRestart = true;
				return;
			}
		}

// If there is app-specific config, handle its validation here
#ifdef APP_SPECIFIC_CONFIG
		extern void validateAppConfig(AsyncWebServerRequest * request, bool &shouldSaveConfig);
		validateAppConfig(request, shouldSaveConfig);
#endif
		AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "OK");
		response->addHeader("Connection", "close");
		response->addHeader("Access-Control-Allow-Origin", "*");
		request->send(response);

		if (shouldSaveConfig) {
			saveGlobalConfig();
// If there is app-specific config, save it to the SD card here, before rebooting
#ifdef APP_SPECIFIC_CONFIG
			extern void saveAppConfig();
			saveAppConfig();
#endif
			shouldRestart = true;
		}
	});
	// #endif

	DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
	DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
	DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

	server.begin();
}

#endif