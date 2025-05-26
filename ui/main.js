import './style.css'

// Timezone information
import timezoneData from './timezones.json'

// HTML templates
import folderDisplay from './templates/folder-display.hbs'
import fileDisplay from './templates/file-display.hbs'

// Utility functions
import {returnValidIntInRange} from './lib/returnValidIntInRange'
import delay from './lib/delay'
import refreshAfterUpdate from './lib/refreshAfterUpdate'

document.addEventListener('DOMContentLoaded', async () => {
	///
	//
	// INITIALIZATION
	//
	///

	// eslint-disable-next-line no-undef
	window.API_URL = import.meta.env.MODE === 'development' ? __API_URL__ : window.location.origin

	// Setup the buttons on the dashboard (other than for app-specific config)
	const wifiButton = document.querySelector('#wifi-button')
	const firmwareButton = document.querySelector('#firmware-button')
	const fileButton = document.querySelector('#file-button')
	const timeButton = document.querySelector('#time-button')
	const advancedButton = document.querySelector('#advanced-button')

	window.dashboard = document.querySelector('#dashboard')

	const wifiSettings = document.querySelector('#wifi-settings')
	const firmwareSettings = document.querySelector('#firmware-settings')
	const fileSettings = document.querySelector('#file-settings')
	const timeSettings = document.querySelector('#time-settings')
	const advancedSettings = document.querySelector('#advanced-settings')

	window.loadingIndicator = document.querySelector('#loading-indicator')

	let configIsLoaded = false

	// Until all configuration is loaded on the server-side, show the loading indicator
	while (!configIsLoaded) {
		try {
			const response = await fetch(`${window.API_URL}/health`);
			if (response.status === 200) {
				configIsLoaded = true

				// Hide the loading indicator once loading is complete
				window.loadingIndicator.classList.add('hidden')
				break
			}
		} catch {
			// Ignore errors here
		}

		// The loading indicator is hidden by default at the start
		// If the initial health check fails, unhide the loading indicator
		window.loadingIndicator.classList.remove('hidden')

		// Delay for 5 seconds between attempts to contact the server
		await delay(5000);
	}

	window.fetchedConfig = {}

	try {
		const response = await fetch(`${window.API_URL}/config`);
		window.fetchedConfig = await response.json()
	} catch (error) {
		console.log(error)
	}

	///
	//
	// APP-SPECIFIC CONFIGURATION
	//
	///
	let appSpecificConfig
	const currentAppName = document.querySelector('#current-app-name')
	const appConfigIconWrapper = document.querySelector('#app-config-icon-wrapper')
	const appConfigButton = document.querySelector('#app-config-button')
	const appConfigButtonName = document.querySelector('#app-config-button-name')

	if (import.meta.env.VITE_CURRENT_APP) {
		appSpecificConfig = await import(
			/* @vite-ignore */
			// eslint-disable-next-line no-undef
			__APP_SPECIFIC_CONFIG_FILENAME__
		)
	}

	// If app-specific config is present (based on the presence of environment variables) load it here
	if (appSpecificConfig) {
		appSpecificConfig.default()
		currentAppName.textContent = appSpecificConfig.APP_NAME
		appConfigIconWrapper.innerHTML = appSpecificConfig.APP_ICON
		appConfigButtonName.textContent = appSpecificConfig.APP_BUTTON_NAME
		appConfigButton.disabled = false
	} else {
		currentAppName.textContent = import.meta.env.VITE_APP_NAME || 'Not Available'
		appConfigButtonName.textContent = 'No App Settings'
		appConfigIconWrapper.innerHTML = (await import('./templates/no-app-settings-icon.hbs')).default
	}

	// Will hold the currently-displayed section of the UI
	window.currentSection = window.dashboard

	// Get the list of all back buttons
	const backButtons = document.querySelectorAll('.back-to-dashboard')

	// View WiFi Settings
	wifiButton.addEventListener('click', () => {
		window.dashboard.style.display = 'none'
		wifiSettings.style.display = 'flex'
		window.currentSection = wifiSettings
	})

	// View Upload Firmware section
	firmwareButton.addEventListener('click', () => {
		window.dashboard.style.display = 'none'
		firmwareSettings.style.display = 'flex'
		window.currentSection = firmwareSettings
	})

	// View SD Card File Browser
	fileButton.addEventListener('click', async () => {
		window.dashboard.style.display = 'none'
		fileSettings.style.display = 'flex'
		window.currentSection = fileSettings
		reloadFileBrowser()
	})

	// View Time Settings
	timeButton.addEventListener('click', () => {
		window.dashboard.style.display = 'none'
		timeSettings.style.display = 'flex'
		window.currentSection = timeSettings
	})

	// View Advanced Settings
	advancedButton.addEventListener('click', () => {
		window.dashboard.style.display = 'none'
		advancedSettings.style.display = 'flex'
		window.currentSection = advancedSettings
	})

	backButtons.forEach(button => {
		button.addEventListener('click', () => {
			window.dashboard.style.display = 'block'
			window.currentSection.style.display = 'none'

			uploadFirmwareButton.disabled = true
		})
	})

	// Populate the information section on the dashboard
	const dashboardIPAddress = document.querySelector('#dashboard-ip-address')
	dashboardIPAddress.textContent = window.API_URL

	const dashboardWifiSSID = document.querySelector('#dashboard-wifi-ssid')
	dashboardWifiSSID.textContent = window.fetchedConfig?.wifi?.ssid || 'Not Connected'

	const dashboardFirmwareVersion = document.querySelector('#dashboard-firmware-version')
	dashboardFirmwareVersion.textContent = import.meta.env.VITE_FIRMWARE_VERSION || 'Unknown'

	///
	//
	// UPLOAD FIRMWARE
	//
	///
	const uploadFirmwareInput = document.querySelector('#upload-firmware-input')
	const uploadFirmwareButton = document.querySelector('#upload-firmware-button')

	// Enable the upload firmware button when a file has been selected
	uploadFirmwareInput.addEventListener('input', async () => {
		uploadFirmwareButton.disabled = false
	})

	const firmwareUploadProgressWrapper = document.querySelector('#firmware-upload-progress-wrapper')
	const firmwareUploadPercentage = document.querySelector('#firmware-upload-percentage')
	const firmwareUploadProgress = document.querySelector('#firmware-upload-progress')

	// Will hold the XHR to upload a new firmware file
	let firmwareUploadRequest

	const CHUNK_SIZE = 10240 // 10KB

	const onFirmwareUploadProgress = (e, totalSize, currentChunkOffset) => {
		if (e.lengthComputable) {
			const percentComplete = ((currentChunkOffset + e.loaded) / totalSize) * 100
			firmwareUploadProgress.value = percentComplete
			firmwareUploadPercentage.textContent = percentComplete.toFixed(0) + '%'
		}
	}

	const onFirmwareUploadComplete = () => {
		alert('Firmware uploaded successfully. Your Luxigrid will now reboot, loading your new application :)')

		// Hide the firmware upload progress bar here
		firmwareUploadProgressWrapper.classList.add('hidden')
		refreshAfterUpdate()
	}

	const uploadChunk = async (file, chunkStart, chunkEnd, totalSize) => new Promise((resolve, reject) => {
		firmwareUploadRequest = new XMLHttpRequest()
		firmwareUploadRequest.open('POST', `${window.API_URL}/updateFirmware`, true)

		if (chunkStart === 0) {
			// Add the full file size header in the first request
			firmwareUploadRequest.setRequestHeader('X-Firmware-Size', totalSize)
		}

		firmwareUploadRequest.upload.addEventListener('progress', e =>
			onFirmwareUploadProgress(e, totalSize, chunkStart),
		)

		firmwareUploadRequest.addEventListener('load', () => {
			if (firmwareUploadRequest.status === 200) {
				resolve()
			} else {
				reject(new Error('Failed to upload chunk'))
			}
		})
		firmwareUploadRequest.addEventListener('error', () => reject(new Error('Chunk upload error')))

		// Prepare the chunk and send it
		const chunk = file.slice(chunkStart, chunkEnd)
		const formData = new FormData()
		formData.append('file', chunk)
		firmwareUploadRequest.send(formData)
	})

	uploadFirmwareButton.addEventListener('click', async () => {
		if (!uploadFirmwareInput.files[0]) {
			alert('Please select a new firmware file to upload.')
			return
		}

		// Basic sanity checking for firmware upload
		// The ESP32 itself has some safeguards, and nothing's perfect, but this should help prevent most issues
		if (!/\.bin$/i.test(uploadFirmwareInput.files[0].name)) {
			alert('Please select a .bin firmware file.')
			return
		}

		if (uploadFirmwareInput.files[0].size > 1024 * 1024 * 2) {
			alert('This file is too large to be a valid firmware file.')
			return
		}

		uploadFirmwareButton.disabled = true

		// Unhide the upload progress bar here
		firmwareUploadProgressWrapper.classList.remove('hidden')

		const file = uploadFirmwareInput.files[0]
		const totalSize = file.size
		let currentChunkStart = 0

		try {
			while (currentChunkStart < totalSize) {
				const chunkEnd = Math.min(currentChunkStart + CHUNK_SIZE, totalSize)
				await uploadChunk(file, currentChunkStart, chunkEnd, totalSize)
				currentChunkStart = chunkEnd
			}

			// All chunks uploaded successfully
			onFirmwareUploadComplete()
		} catch {
			alert('An error occurred during the firmware upload.');
			refreshAfterUpdate()
		}
	})

	///
	//
	// SD CARD FILE BROWSER
	//
	///
	let sdBrowserPath = '/'

	const navigateToFolder = async folderPath => {
		sdBrowserPath = folderPath
		reloadFileBrowser()
	}

	let fileBrowserState = {}
	let sdCardInfoState = {}

	// Button to return to the root directory of the SD card
	const backToRootButton = document.querySelector('#back-to-root')
	backToRootButton.addEventListener('click', () => navigateToFolder('/'))

	// These elements will hold information about SD card space
	const sdTotalSize = document.querySelector('#sd-total-size')
	const sdUsedSpace = document.querySelector('#sd-used-space')
	const sdFreeSpace = document.querySelector('#sd-free-space')

	// If the current file is empty, display this message
	const emptyFolderMessage = document.createElement('span')
	emptyFolderMessage.classList.add('ml-2')
	emptyFolderMessage.textContent = 'This directory is empty'

	// This function reloads the SD file browser (also used to navigate to a new directory)
	// It fetches the latest list of files in the current folder from the API
	const reloadFileBrowser = async () => {
		uploadToSDButton.disabled = true

		try {
			// Get the list of files in the current directory
			const response = await fetch(`${window.API_URL}/browser?path=${sdBrowserPath}`);

			if (!response.ok) {
				throw new Error('Network response was not ok');
			}

			fileBrowserState = await response.json()

			// Get information about SD card space
			const sdCardInfoResponse = await fetch(`${window.API_URL}/sdinfo`)

			if (!sdCardInfoResponse.ok) {
				throw new Error('Network response was not ok')
			}

			sdCardInfoState = await sdCardInfoResponse.json()
			sdTotalSize.textContent = sdCardInfoState.total
			sdUsedSpace.textContent = sdCardInfoState.used
			sdFreeSpace.textContent = sdCardInfoState.free

			const currentDirectoryDisplay = document.querySelector('#current-directory')
			currentDirectoryDisplay.textContent = `Current Directory: ${fileBrowserState.path}`

			// Remove the old list of files/folders
			const fileBrowser = document.querySelector('#file-browser')
			fileBrowser.replaceChildren()

			// If there is a parent folder, allow the user to return to it (or the root folder)
			if (fileBrowserState.parentPath) {
				const goUpButton = document.createElement('li')
				goUpButton.innerHTML = '<button class="browse mb-4 p-2 w-full"><strong class="inline-flex w-full"><div class="h-6 w-6 mr-2"><svg xmlns="http://www.w3.org/2000/svg" fill="none" stroke-width="1.5" viewBox="0 0 24 24"><path stroke="#fff" stroke-linecap="round" stroke-linejoin="round" d="m6 15 6-6 6 6"/></svg></div><span>Parent Directory</span></strong></button>'
				fileBrowser.appendChild(goUpButton)

				goUpButton.querySelector('.browse').onclick = () => {
					navigateToFolder(fileBrowserState.parentPath)
				}

				backToRootButton.style.display = 'flex'
			} else {
				// If there is no parent folder, we're already in the root folder
				backToRootButton.style.display = 'none'
			}

			// Build the table of files and folders in the current folder
			fileBrowserState.files?.forEach((file, fileIndex) => {
				const fileItem = document.createElement('li')

				// Alternate background colours
				if (fileIndex % 2) {
					fileItem.classList.add('bg-blue-900/50')
				} else {
					fileItem.classList.add('bg-blue-800/60')
				}

				// Rounded corners for top and bottom items in list
				if (fileIndex === 0) {
					fileItem.classList.add('rounded-t-md')
				} else if (fileIndex === fileBrowserState.files.length - 1) {
					fileItem.classList.add('rounded-b-md')
				}

				// Trivial Handlebars-like find and replace for folderDisplay and fileDisplay components
				if (file.isDir) {
					// Folders give their name, and allow you to navigate to them on click
					fileItem.innerHTML = folderDisplay.replace('{{folderName}}', file.name)

					fileItem.querySelector('.browse').onclick = () => {
						navigateToFolder(`${file.path}`)
					}
				} else {
					// Files have their name and file size, as well as a download button
					fileItem.innerHTML = fileDisplay.replaceAll('{{fileName}}', file.name).replace('{{fileSize}}', file.size).replace('{{filePath}}', fileBrowserState.path + '/' + file.name)
				}

				// Each file or folder should have a delete button next to it
				fileItem.querySelector('.delete').onclick = () => {
					deleteSDFile(`${fileBrowserState.path}/${file.name}`)
				}

				fileBrowser.appendChild(fileItem)
			})

			// If no files are present, display the empty folder message
			if (!fileBrowserState.files?.length) {
				fileBrowser.appendChild(emptyFolderMessage)
			}
		} catch (error) {
			console.error('There has been a problem with your fetch operation:', error);
		}
	}

	let newFolderName = ''

	const createNewFolderInput = document.querySelector('#create-new-folder-input')
	const createNewFolderButton = document.querySelector('#create-new-folder-button')

	// The Create New Folder button should be disabled when there's no name provided for the new folder
	createNewFolderInput.addEventListener('input', async e => {
		newFolderName = e.target.value
		if (e.target.value.length) {
			createNewFolderButton.disabled = false
		} else {
			createNewFolderButton.disabled = true
		}
	})

	// Logic to create a new folder via the API
	createNewFolderButton.addEventListener('click', async () => {
		try {
			const response = await fetch(`${window.API_URL}/createFolder?name=${newFolderName}&currentPath=${sdBrowserPath}`, {method: 'POST'})
			if (response.ok) {
				alert('New folder created successfully');
				reloadFileBrowser()
			} else {
				alert('Folder creation failed');
			}
		} catch {
			alert('An error occurred during folder creation');
		}

		reloadFileBrowser()
	})

	// Input and button to handle uploading files to the SD card (in the current folder)
	const uploadToSDInput = document.querySelector('#upload-to-sd-input')
	const uploadToSDButton = document.querySelector('#upload-to-sd-button')

	uploadToSDInput.addEventListener('input', async () => {
		uploadToSDButton.disabled = false
	})

	const uploadProgressWrapper = document.querySelector('#upload-progress-wrapper')
	const uploadPercentage = document.querySelector('#upload-percentage')
	const uploadProgress = document.querySelector('#upload-progress')

	// Will hold the XHR to upload a file to the SD card
	let sdUploadRequest

	const onSDUploadProgress = e => {
		if (e.lengthComputable) {
			const percentComplete = (e.loaded / e.total) * 100
			uploadProgress.value = percentComplete
			uploadPercentage.textContent = percentComplete.toFixed(2) + '%'
		}
	}

	const onSDUploadError = () => {
		alert('File upload failed')
	}

	const onSDUploadComplete = () => {
		if (sdUploadRequest.status === 200) {
			alert('File uploaded successfully')

			// Hide the upload progress bar here
			uploadProgressWrapper.classList.add('hidden')

			reloadFileBrowser()
		} else {
			alert('File upload failed')
		}

		sdUploadRequest.removeEventListener('load', onSDUploadComplete);
		sdUploadRequest.removeEventListener('error', onSDUploadError);
		sdUploadRequest.upload.removeEventListener('progress', onSDUploadProgress)
	}

	uploadToSDButton.addEventListener('click', async () => {
		const formData = new FormData()

		if (!uploadToSDInput.files[0]) {
			alert('Please select a file to upload to the SD card')
		}

		formData.append('path', fileBrowserState.path)
		formData.append('file', uploadToSDInput.files[0])

		try {
			// Unhide the upload progress bar here
			uploadProgressWrapper.classList.remove('hidden')

			// Initialize the API request
			sdUploadRequest = new XMLHttpRequest()
			sdUploadRequest.open('POST', `${window.API_URL}/upload`, true)

			// Set the event listeners
			sdUploadRequest.upload.addEventListener('progress', onSDUploadProgress)
			sdUploadRequest.addEventListener('load', onSDUploadComplete)
			sdUploadRequest.addEventListener('error', onSDUploadError)

			// Send the data
			sdUploadRequest.send(formData)
		} catch {
			alert('An error occurred during file upload.');
		}
	})

	// Logic to delete a file on the SD card
	const deleteSDFile = async filePath => {
		try {
			const formData = new FormData()
			formData.append('path', filePath)
			const response = await fetch(`${window.API_URL}/delete`, {method: 'POST', body: formData})

			if (response.ok) {
				alert('File deleted successfully.');
				reloadFileBrowser()
			} else {
				alert('File deletion failed.');
			}
		} catch {
			alert('An error occurred during file deletion.');
		}

		reloadFileBrowser()
	}

	///
	//
	// WIFI SETTINGS
	//
	///
	let wifiSSID = window.fetchedConfig.wifi.ssid
	let wifiPassword = window.fetchedConfig.wifi.password

	const wifiSettingsForm = document.querySelector('#wifi-settings')
	const wifiSSIDInput = document.querySelector('#wifi-ssid')
	const wifiPasswordInput = document.querySelector('#wifi-password')
	const toggleWifiPasswordButton = document.querySelector('#toggle-wifi-password')

	const hideWifiPasswordIcon = document.querySelector('#hide-wifi-password-icon')
	const showWifiPasswordIcon = document.querySelector('#show-wifi-password-icon')

	// Populate the initial values
	wifiSSIDInput.value = wifiSSID
	wifiPasswordInput.value = wifiPassword

	wifiSSIDInput.addEventListener('input', e => {
		wifiSSID = e.target.value.trim()
	})

	wifiPasswordInput.addEventListener('input', e => {
		wifiPassword = e.target.value.trim()
	})

	toggleWifiPasswordButton.addEventListener('click', () => {
		const isPassword = wifiPasswordInput.type === 'password';
		wifiPasswordInput.type = isPassword ? 'text' : 'password';
		toggleWifiPasswordButton.title = isPassword ? 'Hide WiFi password' : 'Show WiFi password'

		if (isPassword) {
			hideWifiPasswordIcon.classList.add('hidden')
			showWifiPasswordIcon.classList.remove('hidden')
		} else {
			hideWifiPasswordIcon.classList.remove('hidden')
			showWifiPasswordIcon.classList.add('hidden')
		}
	});

	wifiSettingsForm.addEventListener('submit', async e => {
		e.preventDefault()

		if (!wifiSSID.length || wifiSSID.length < 2 || wifiSSID.length > 32) {
			alert('Please enter a valid WiFi Network Name (SSID)')
			return
		}

		if (wifiPassword.length > 63) {
			alert('Please enter a valid WiFi Password')
			return
		}

		try {
			const formData = new FormData();
			formData.append('ssid', wifiSSID);
			formData.append('password', wifiPassword)
			const response = await fetch(`${window.API_URL}/wifi`, {method: 'POST', body: formData});

			if (!response.ok) {
				throw new Error(`Error! status: ${response.status}`);
			}

			alert('WiFi Settings updated successfully! Please allow a moment for your LED matrix to restart, and your changes will take effect.')
			refreshAfterUpdate()
		} catch (error) {
			console.log(error)
			alert('WiFi Settings update failed. Please try again.')
		}
	})

	///
	//
	// TIME SETTINGS
	//
	///
	let timeFormat = window.fetchedConfig.global.is24h ? '24' : '12'
	let timezone = window.fetchedConfig.global.humanReadableTimezone

	const timezoneSelector = document.createElement('select');

	// Populate the timezone selector with all the different timezones
	timezoneData.forEach(g => {
		const optgroup = document.createElement('optgroup');
		optgroup.label = g.region;
		g.timezones.forEach(t => {
			const option = document.createElement('option');
			option.value = `${g.region}/${t.name}`
			option.textContent = t.name;
			optgroup.appendChild(option);
		});
		timezoneSelector.appendChild(optgroup);
	});

	// Add the timezone selector to the DOM here
	timezoneSelector.className = 'bg-black/50 text-white focus:bg-black/90 rounded-md w-full'
	const timezoneSelectorWrapper = document.querySelector('#timezone-selector-wrapper')
	timezoneSelectorWrapper.appendChild(timezoneSelector)

	const dateInput = document.getElementById('date-input')
	const hourInput = document.getElementById('hour-input')
	const minuteInput = document.getElementById('minute-input')
	const secondInput = document.getElementById('second-input')
	const timeFormatInput = document.getElementById('time-format')

	timeFormatInput.value = timeFormat
	timezoneSelector.value = timezone

	timeFormatInput.addEventListener('input', e => {
		timeFormat = e.target.value
	})
	timezoneSelector.addEventListener('input', e => {
		timezone = e.target.value
	})

	// Simple function to populate the time selectors with options (hours from 0 to 23; minutes and seconds from 0 to 59)
	function populateTimeSelects(input, start, end) {
		for (let i = start; i <= end; i++) {
			const option = new Option(i.toString().padStart(2, '0'), i);
			input.add(option);
		}
	}

	populateTimeSelects(hourInput, 0, 23);
	populateTimeSelects(minuteInput, 0, 59);
	populateTimeSelects(secondInput, 0, 59);

	const getCurrentTimeButton = document.getElementById('get-current-time')

	function updateCurrentTime() {
		const now = new Date();
		// Set the date input - note the nonsense required to get a local date string
		dateInput.value = (new Date(Date.now() - ((new Date()).getTimezoneOffset() * 60000))).toISOString().substring(0, 10)
		// Set the hour, minute, and second inputs
		hourInput.value = now.getHours();
		minuteInput.value = now.getMinutes();
		secondInput.value = now.getSeconds();
	}

	getCurrentTimeButton.addEventListener('click', () => {
		updateCurrentTime()
	})

	updateCurrentTime()

	// Save the new time and date
	const saveTimestampButton = document.querySelector('#save-timestamp-button')

	saveTimestampButton.addEventListener('click', async () => {
		let utcTimestamp

		try {
			const localDate = new Date(`${dateInput.value} ${hourInput.value}:${minuteInput.value}:${secondInput.value}`);
			const timezoneOffsetMinutes = localDate.getTimezoneOffset();
			const adjustedDate = new Date(localDate.getTime() + (timezoneOffsetMinutes * 60 * 1000));
			utcTimestamp = Date.UTC(adjustedDate.getFullYear(), adjustedDate.getMonth(), adjustedDate.getDate(), adjustedDate.getHours(), adjustedDate.getMinutes(), adjustedDate.getSeconds());
		} catch (error) {
			console.log(error)
			alert('An error occurred while attempting to update the time and date. Please try again.')
		}

		try {
			const formData = new FormData()
			formData.append('timestamp', utcTimestamp)
			await fetch(`${window.API_URL}/time`, {method: 'POST', body: formData})
			alert('Time and date updated successfully!')
		} catch (error) {
			console.log(error)
			alert('Time and date update failed. Please try again.')
		}
	})

	// Save the time settings (like timezone and 12/24 hour time format)
	const saveTimeSettingsButton = document.querySelector('#save-time-settings-button')

	saveTimeSettingsButton.addEventListener('click', async () => {
		const timezoneParts = timezone.split('/')
		const matchingRegion = timezoneData.find(t => t.region === timezoneParts[0])
		const matchingTimezone = matchingRegion && matchingRegion.timezones.find(z => z.name === timezoneParts[1])

		if (!matchingTimezone || (timeFormat !== '12' && timeFormat !== '24')) {
			alert('An error occurred while attempting to update your time settings. Please try again.')
			return
		}

		try {
			const formData = new FormData();
			formData.append('is24h', timeFormat === '24')
			formData.append('timezone', matchingTimezone.value)
			formData.append('humanReadableTimezone', `${matchingRegion.region}/${matchingTimezone.name}`)
			await fetch(`${window.API_URL}/config`, {method: 'POST', body: formData});
			alert('Time Settings updated successfully! Please allow a moment for your Luxigrid to restart')
			refreshAfterUpdate()
		} catch (error) {
			console.log(error)
			alert('Time Settings update failed. Please try again.')
		}
	})

	///
	//
	// ADVANCED SETTINGS
	//
	///
	let temperatureUnits = window.fetchedConfig.global.isCelcius ? 'c' : 'f'
	let disableLightSensor = window.fetchedConfig.global.disableBH1750
	let manualScreenBrightness = window.fetchedConfig.global.brightness
	let luxThreshold = window.fetchedConfig.global.luxThreshold
	let bh1750MeasurementDelay = window.fetchedConfig.global.bh1750Delay
	let bme680MeasurementDelay = window.fetchedConfig.global.bme680Delay
	let customNTPServer = window.fetchedConfig.global.ntpServer
	let disableNTP = window.fetchedConfig.global.disableNTP

	const temperatureUnitsInput = document.querySelector('#temperature-units')
	const disableLightSensorInput = document.querySelector('#disable-bh1750')
	const manualScreenBrightnessInput = document.querySelector('#manual-brightness')
	const luxThresholdInput = document.querySelector('#lux-threshold')
	const bh1750MeasurementDelayInput = document.querySelector('#bh1750-delay')
	const bme680MeasurementDelayInput = document.querySelector('#bme680-delay')
	const customNTPServerInput = document.querySelector('#ntp-server')
	const disableNTPInput = document.querySelector('#disable-ntp')

	temperatureUnitsInput.value = temperatureUnits
	disableLightSensorInput.checked = disableLightSensor
	manualScreenBrightnessInput.value = manualScreenBrightness
	luxThresholdInput.value = luxThreshold
	bh1750MeasurementDelayInput.value = bh1750MeasurementDelay
	bme680MeasurementDelayInput.value = bme680MeasurementDelay
	customNTPServerInput.value = customNTPServer
	disableNTPInput.checked = disableNTP

	temperatureUnitsInput.addEventListener('input', e => {
		temperatureUnits = e.target.value
	})

	disableLightSensorInput.addEventListener('input', e => {
		disableLightSensor = e.target.checked
	})

	manualScreenBrightnessInput.addEventListener('input', e => {
		manualScreenBrightness = returnValidIntInRange(e.target.value, 0, 255)
		e.target.value = manualScreenBrightness
	})

	manualScreenBrightnessInput.addEventListener('keydown', e => {
		if (e.key === 'ArrowUp' && manualScreenBrightness + 1 <= 255) {
			manualScreenBrightness++
			e.target.value = manualScreenBrightness
		}

		if (e.key === 'ArrowDown' && manualScreenBrightness - 1 >= 0) {
			manualScreenBrightness--
			e.target.value = manualScreenBrightness
		}
	})

	luxThresholdInput.addEventListener('input', e => {
		luxThreshold = returnValidIntInRange(e.target.value, 0, 65535)
		e.target.value = luxThreshold
	})

	luxThresholdInput.addEventListener('keydown', e => {
		if (e.key === 'ArrowUp' && luxThreshold + 1 <= 65535) {
			luxThreshold++
			e.target.value = luxThreshold
		}

		if (e.key === 'ArrowDown' && luxThreshold - 1 >= 0) {
			luxThreshold--
			e.target.value = luxThreshold
		}
	})

	// 3600000 is one hour in milliseconds
	bh1750MeasurementDelayInput.addEventListener('input', e => {
		bh1750MeasurementDelay = returnValidIntInRange(e.target.value, 0, 3600000)
		e.target.value = bh1750MeasurementDelay
	})

	bh1750MeasurementDelayInput.addEventListener('keydown', e => {
		if (e.key === 'ArrowUp' && bh1750MeasurementDelay + 1 <= 3600000) {
			bh1750MeasurementDelay++
			e.target.value = bh1750MeasurementDelay
		}

		if (e.key === 'ArrowDown' && bh1750MeasurementDelay - 1 >= 0) {
			bh1750MeasurementDelay--
			e.target.value = bh1750MeasurementDelay
		}
	})

	bme680MeasurementDelayInput.addEventListener('input', e => {
		bme680MeasurementDelay = returnValidIntInRange(e.target.value, 0, 3600000)
		e.target.value = bme680MeasurementDelay
	})

	bme680MeasurementDelayInput.addEventListener('keydown', e => {
		if (e.key === 'ArrowUp' && bme680MeasurementDelay + 1 <= 3600000) {
			bme680MeasurementDelay++
			e.target.value = bme680MeasurementDelay
		}

		if (e.key === 'ArrowDown' && bme680MeasurementDelay - 1 >= 0) {
			bme680MeasurementDelay--
			e.target.value = bme680MeasurementDelay
		}
	})

	customNTPServerInput.addEventListener('input', e => {
		customNTPServer = e.target.value
	})

	disableNTPInput.addEventListener('input', e => {
		disableNTP = e.target.checked
	})

	// This is fairly naive, but it should work for most cases
	function isValidNTPURL(urlString) {
		try {
			const hasPrefix = urlString.startsWith('ntp://')
			const validatedURL = new URL(`${hasPrefix ? '' : 'ntp://'}${urlString}`);
			return Boolean(validatedURL) && validatedURL.protocol === 'ntp:';
		} catch {
			return false;
		}
	}

	const saveAdvancedSettingsButton = document.querySelector('#save-advanced-settings-button')

	saveAdvancedSettingsButton.addEventListener('click', async () => {
		if (temperatureUnits !== 'c' && temperatureUnits !== 'f') {
			alert('Please specify valid temperature units - c or f')
		}

		if (typeof disableLightSensor !== 'boolean') {
			alert('Please indicate whether or not the light sensor should be disabled')
			return
		}

		if (manualScreenBrightness < 0 || manualScreenBrightness > 255) {
			alert('Please enter a manual screen brightness value between 0 and 255')
			return
		}

		if (luxThreshold < 0 || luxThreshold > 65535) {
			alert('Please enter a lux threshold value between 0 and 65,535')
			return
		}

		if (bh1750MeasurementDelay < 0 || bh1750MeasurementDelay > 3600000) {
			alert('Please enter a light sensor measurement delay between 0 and 3,600,000')
			return
		}

		if (bme680MeasurementDelay < 0 || bme680MeasurementDelay > 3600000) {
			alert('Please enter a temperature/humidity sensor measurement delay between 0 and 3,600,000')
			return
		}

		if (customNTPServer.length === 0 || customNTPServer.length > 128 || !isValidNTPURL(customNTPServer)) {
			alert('Please specify a valid NTP server')
			return
		}

		if (typeof disableNTP !== 'boolean') {
			alert('Please indicate whether or not NTP should be disabled')
			return
		}

		// Chop off the "ntp://" if it's part of the URL
		const hasPrefix = customNTPServer.startsWith('ntp://')
		const ntpServerHostname = hasPrefix ? customNTPServer.slice(6) : customNTPServer

		try {
			const formData = new FormData()
			formData.append('disableBH1750', disableLightSensor)
			formData.append('brightness', manualScreenBrightness)
			formData.append('luxThreshold', luxThreshold)
			formData.append('bh1750Delay', bh1750MeasurementDelay)
			formData.append('bme680Delay', bme680MeasurementDelay)
			formData.append('ntpServer', ntpServerHostname)
			formData.append('disableNTP', disableNTP)
			formData.append('isCelcius', temperatureUnits === 'c')
			await fetch(`${window.API_URL}/config`, {method: 'POST', body: formData})
			alert('Advanced Settings updated successfully! Please allow a moment for your Luxigrid to restart.')
			refreshAfterUpdate()
		} catch (error) {
			console.log(error)
			alert('Advanced Settings update failed. Please try again.')
		}
	})
})
