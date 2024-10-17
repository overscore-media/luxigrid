import refreshAfterUpdate from '../../lib/refreshAfterUpdate.js'
import {returnValidIntInRange} from '../../lib/returnValidIntInRange.js'

import APP_CONFIG_HTML from './gif-player-settings.hbs'

export const APP_NAME = 'GIF Player'
export const APP_BUTTON_NAME = 'GIF Player Settings'

export const APP_ICON = '<svg xmlns="http://www.w3.org/2000/svg" fill="none" stroke-width="1.5" color="#000" viewBox="0 0 24 24"><path stroke="#fff" stroke-linecap="round" d="M4 6V3.6a.6.6 0 0 1 .6-.6h14.8a.6.6 0 0 1 .6.6V6M4 18v2.4a.6.6 0 0 0 .6.6h14.8a.6.6 0 0 0 .6-.6V18"/><path stroke="#fff" stroke-linecap="round" stroke-linejoin="round" d="M15.5 15V9h3M15.5 12h2M12 15V9M8.5 9h-3v6h3v-2.4"/></svg>'

export default function gifPlayerConfig() {
	const appConfigElement = document.createElement('form')
	appConfigElement.innerHTML = APP_CONFIG_HTML.replace('{{appIcon}}', APP_ICON)
	appConfigElement.id = 'gif-player-settings'
	appConfigElement.className = 'hidden flex-col w-full'
	document.querySelector('#configuration').appendChild(appConfigElement)

	///
	//
	// GIF PLAYER SETTINGS
	//
	///
	const gifPlayerSettings = document.querySelector('#gif-player-settings')
	const gifPlayerButton = document.querySelector('#app-config-button')

	gifPlayerButton.addEventListener('click', () => {
		window.dashboard.style.display = 'none'
		gifPlayerSettings.style.display = 'flex'
		window.currentSection = gifPlayerSettings
	})

	let maxGifDuration = window.fetchedConfig['gif-player'].maxGifDuration
	let gifDelay = window.fetchedConfig['gif-player'].gifDelay

	const maxGifDurationInput = document.querySelector('#max-gif-duration')
	const gifDelayInput = document.querySelector('#gif-delay')

	maxGifDurationInput.value = maxGifDuration
	gifDelayInput.value = gifDelay

	// 3600000 is one hour in milliseconds
	maxGifDurationInput.addEventListener('input', e => {
		maxGifDuration = returnValidIntInRange(e.target.value, 0, 3600000)
		e.target.value = maxGifDuration
	})

	maxGifDurationInput.addEventListener('keydown', e => {
		if (e.key === 'ArrowUp' && maxGifDuration + 1 <= 3600000) {
			maxGifDuration++
			e.target.value = maxGifDuration
		}

		if (e.key === 'ArrowDown' && maxGifDuration - 1 >= 0) {
			maxGifDuration--
			e.target.value = maxGifDuration
		}
	})

	gifDelayInput.addEventListener('input', e => {
		gifDelay = returnValidIntInRange(e.target.value, 0, 3600000)
		e.target.value = gifDelay
	})

	gifDelayInput.addEventListener('keydown', e => {
		if (e.key === 'ArrowUp' && gifDelay + 1 <= 3600000) {
			gifDelay++
			e.target.value = gifDelay
		}

		if (e.key === 'ArrowDown' && gifDelay - 1 >= 0) {
			gifDelay--
			e.target.value = gifDelay
		}
	})

	appConfigElement.addEventListener('submit', async e => {
		e.preventDefault()

		try {
			const formData = new FormData()
			formData.append('maxGifDuration', maxGifDuration)
			formData.append('gifDelay', gifDelay)
			await fetch(`${window.API_URL}/config`, {method: 'POST', body: formData})
			alert('GIF Player Settings updated successfully! Please allow a moment for your Luxigrid to restart, and your changes will take effect.')
			refreshAfterUpdate()
		} catch (error) {
			console.log(error)
			alert('GIF Player Settings update failed. Please try again.')
		}
	})
}
