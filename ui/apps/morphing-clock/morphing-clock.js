import hexToRgb from '../../lib/hexToRgb'
import refreshAfterUpdate from '../../lib/refreshAfterUpdate.js'
import rgbToHex from '../../lib/rgbToHex.js'

import APP_CONFIG_HTML from './morphing-clock-settings.hbs'

export const APP_NAME = 'Morphing Clock'
export const APP_BUTTON_NAME = 'Morphing Clock Settings'

export const APP_ICON = '<svg xmlns="http://www.w3.org/2000/svg" fill="none" stroke-width="1.5" viewBox="0 0 24 24"><path stroke="#fff" stroke-linecap="round" stroke-linejoin="round" d="M12 6v6h6"/><path stroke="#fff" stroke-linecap="round" stroke-linejoin="round" d="M21.888 10.5C21.164 5.689 17.013 2 12 2 6.477 2 2 6.477 2 12s4.477 10 10 10c4.1 0 7.625-2.468 9.168-6"/><path stroke="#fff" stroke-linecap="round" stroke-linejoin="round" d="M17 16h4.4a.6.6 0 0 1 .6.6V21"/></svg>'

export default function morphingClockConfig() {
	const appConfigElement = document.createElement('form')
	appConfigElement.innerHTML = APP_CONFIG_HTML.replace('{{appIcon}}', APP_ICON)
	appConfigElement.id = 'morphing-clock-settings'
	appConfigElement.className = 'hidden flex-col w-full'
	document.querySelector('#configuration').appendChild(appConfigElement)

	///
	//
	// MORPHING CLOCK SETTINGS
	//
	///
	const morphingClockSettings = document.querySelector('#morphing-clock-settings')
	const morphingClockButton = document.querySelector('#app-config-button')

	morphingClockButton.addEventListener('click', () => {
		window.dashboard.style.display = 'none'
		morphingClockSettings.style.display = 'flex'
		window.currentSection = morphingClockSettings
	})

	const r = window.fetchedConfig['morphing-clock'].r
	const g = window.fetchedConfig['morphing-clock'].g
	const b = window.fetchedConfig['morphing-clock'].b

	const clockfaceColourInput = document.querySelector('#clockface-colour > input')

	clockfaceColourInput.value = rgbToHex({r, g, b})

	appConfigElement.addEventListener('submit', async e => {
		e.preventDefault()

		try {
			const {r: newR, g: newG, b: newB} = hexToRgb(clockfaceColourInput.value)

			const formData = new FormData()
			formData.append('r', newR)
			formData.append('g', newG)
			formData.append('b', newB)
			await fetch(`${window.API_URL}/config`, {method: 'POST', body: formData})
			alert('Morphing Clock Settings updated successfully! Please allow a moment for your Luxigrid to restart, and your changes will take effect.')
			refreshAfterUpdate()
		} catch (error) {
			console.log(error)
			alert('Morphing Clock Settings update failed. Please try again.')
		}
	})
}
