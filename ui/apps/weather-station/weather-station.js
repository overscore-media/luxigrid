import refreshAfterUpdate from '../../lib/refreshAfterUpdate.js'
import {returnValidIntInRange} from '../../lib/returnValidIntInRange.js'

import APP_CONFIG_HTML from './weather-station-settings.hbs'

export const APP_NAME = 'Weather Station'
export const APP_BUTTON_NAME = 'Weather Station Settings'

export const APP_ICON = '<svg xmlns="http://www.w3.org/2000/svg" fill="none" stroke-width="1.5" viewBox="0 0 24 24"><path stroke="#fff" stroke-linecap="round" stroke-linejoin="round" d="M12 14v2M12 20v2M8 18v2M16 18v2M20 17.607c1.494-.585 3-1.918 3-4.607 0-4-3.333-5-5-5 0-2 0-6-6-6S6 6 6 8c-1.667 0-5 1-5 5 0 2.689 1.506 4.022 3 4.607"/></svg>'

export default function weatherStationConfig() {
	const appConfigElement = document.createElement('form')
	appConfigElement.innerHTML = APP_CONFIG_HTML.replace('{{appIcon}}', APP_ICON)
	appConfigElement.id = 'weather-station-settings'
	appConfigElement.className = 'hidden flex-col w-full'
	document.querySelector('#configuration').appendChild(appConfigElement)

	///
	//
	// WEATHER STATION SETTINGS
	//
	///
	const weatherStationSettings = document.querySelector('#weather-station-settings')
	const weatherStationButton = document.querySelector('#app-config-button')

	weatherStationButton.addEventListener('click', () => {
		window.dashboard.style.display = 'none'
		weatherStationSettings.style.display = 'flex'
		window.currentSection = weatherStationSettings
	})

	let latitude = window.fetchedConfig['weather-station'].latitude
	let longitude = window.fetchedConfig['weather-station'].longitude
	let insideOnly = window.fetchedConfig['weather-station'].insideOnly
	let refreshInterval = window.fetchedConfig['weather-station'].refreshInterval

	const latitudeInput = document.querySelector('#latitude')
	const longitudeInput = document.querySelector('#longitude')
	const insideOnlyInput = document.querySelector('#inside-only')
	const refreshIntervalInput = document.querySelector('#refresh-interval')

	latitudeInput.value = latitude
	longitudeInput.value = longitude
	insideOnlyInput.checked = insideOnly
	refreshIntervalInput.value = refreshInterval

	latitudeInput.addEventListener('input', e => {
		latitude = e.target.value
	})

	longitudeInput.addEventListener('input', e => {
		longitude = e.target.value
	})

	insideOnlyInput.addEventListener('input', e => {
		insideOnly = e.target.checked
	})

	// 3600000 is one hour in milliseconds
	refreshIntervalInput.addEventListener('input', e => {
		refreshInterval = returnValidIntInRange(e.target.value, 0, 3600000)
		e.target.value = refreshInterval
	})

	refreshIntervalInput.addEventListener('keydown', e => {
		if (e.key === 'ArrowUp' && refreshInterval + 1 <= 3600000) {
			refreshInterval++
			e.target.value = refreshInterval
		}

		if (e.key === 'ArrowDown' && refreshInterval - 1 >= 60000) {
			refreshInterval--
			e.target.value = refreshInterval
		}
	})

	appConfigElement.addEventListener('submit', async e => {
		e.preventDefault()

		if (refreshInterval < 60000) {
			alert('Please specify a refresh interval of at least 60000 (one minute).')
			return
		}

		try {
			const formData = new FormData()
			formData.append('latitude', latitude)
			formData.append('longitude', longitude)
			formData.append('insideOnly', insideOnly)
			formData.append('refreshInterval', refreshInterval)
			await fetch(`${window.API_URL}/config`, {method: 'POST', body: formData})
			alert('Weather Station Settings updated successfully! Please allow a moment for your Luxigrid to restart, and your changes will take effect.')
			refreshAfterUpdate()
		} catch (error) {
			console.log(error)
			alert('Weather Station Settings update failed. Please try again.')
		}
	})
}
