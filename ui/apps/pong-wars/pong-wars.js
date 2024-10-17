import hexToRgb from '../../lib/hexToRgb'
import refreshAfterUpdate from '../../lib/refreshAfterUpdate.js'
import rgbToHex from '../../lib/rgbToHex.js'
import APP_CONFIG_HTML from './pong-wars-settings.hbs'

export const APP_NAME = 'Pong Wars'
export const APP_BUTTON_NAME = 'Pong Wars Settings'

export const APP_ICON = '<svg xmlns="http://www.w3.org/2000/svg" fill="none" stroke-width="1.5" viewBox="0 0 24 24"><path stroke="#fff" stroke-linecap="round" stroke-linejoin="round" d="M18 21a2 2 0 1 0 0-4 2 2 0 0 0 0 4ZM6 7a2 2 0 1 0 0-4 2 2 0 0 0 0 4ZM18 17V7s0-2-2-2h-3M6 7v10s0 2 2 2h3"/><path stroke="#fff" stroke-linecap="round" stroke-linejoin="round" d="M15 7.5 12.5 5 15 2.5M8.5 16.5 11 19l-2.5 2.5"/></svg>'

export default function pongWarsConfig() {
	const appConfigElement = document.createElement('form')
	appConfigElement.innerHTML = APP_CONFIG_HTML.replace('{{appIcon}}', APP_ICON)
	appConfigElement.id = 'pong-wars-settings'
	appConfigElement.className = 'hidden flex-col w-full'
	document.querySelector('#configuration').appendChild(appConfigElement)

	///
	//
	// PONG WARS SETTINGS
	//
	///
	const pongWarsSettings = document.querySelector('#pong-wars-settings')
	const pongWarsButton = document.querySelector('#app-config-button')

	pongWarsButton.addEventListener('click', () => {
		window.dashboard.style.display = 'none'
		pongWarsSettings.style.display = 'flex'
		window.currentSection = pongWarsSettings
	})

	const colour1 = window.fetchedConfig['pong-wars'].colour1
	const colour2 = window.fetchedConfig['pong-wars'].colour2

	const colour1Input = document.querySelector('#colour-1 > input')
	const colour2Input = document.querySelector('#colour-2 > input')

	colour1Input.value = rgbToHex(colour1)
	colour2Input.value = rgbToHex(colour2)

	appConfigElement.addEventListener('submit', async e => {
		e.preventDefault()

		try {
			const formData = new FormData()
			formData.append('colours', JSON.stringify({colour1: hexToRgb(colour1Input.value), colour2: hexToRgb(colour2Input.value)}))
			await fetch(`${window.API_URL}/config`, {method: 'POST', body: formData})
			alert('Pong Wars Settings updated successfully! Please allow a moment for your Luxigrid to restart, and your changes will take effect.')
			refreshAfterUpdate()
		} catch (error) {
			console.log(error)
			alert('Pong Wars Settings update failed. Please try again.')
		}
	})
}
