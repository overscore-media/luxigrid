import './stock-ticker.css'

import {returnValidIntInRange} from '../../lib/returnValidIntInRange.js'
import rgbToHex from '../../lib/rgbToHex.js'
import hexToRgb from '../../lib/hexToRgb.js'

import APP_CONFIG_HTML from './stock-ticker-settings.hbs'
import refreshAfterUpdate from '../../lib/refreshAfterUpdate.js'

export const APP_NAME = 'Stock Ticker'
export const APP_BUTTON_NAME = 'Stock Ticker Settings'

export const APP_ICON = '<svg xmlns="http://www.w3.org/2000/svg" fill="none" stroke-width="1.5" viewBox="0 0 24 24"><path stroke="#fff" stroke-linecap="round" stroke-linejoin="round" d="M5 16v-2M12 21v-2M19 13v-2M5 8V6M12 13v-2M19 5V3M7 8.6v4.8a.6.6 0 0 1-.6.6H3.6a.6.6 0 0 1-.6-.6V8.6a.6.6 0 0 1 .6-.6h2.8a.6.6 0 0 1 .6.6ZM14 13.6v4.8a.6.6 0 0 1-.6.6h-2.8a.6.6 0 0 1-.6-.6v-4.8a.6.6 0 0 1 .6-.6h2.8a.6.6 0 0 1 .6.6ZM21 5.6v4.8a.6.6 0 0 1-.6.6h-2.8a.6.6 0 0 1-.6-.6V5.6a.6.6 0 0 1 .6-.6h2.8a.6.6 0 0 1 .6.6Z" /></svg>'

export default function stockTickerConfig() {
	const appConfigElement = document.createElement('form')
	appConfigElement.innerHTML = APP_CONFIG_HTML.replace('{{appIcon}}', APP_ICON)
	appConfigElement.id = 'stock-ticker-settings'
	appConfigElement.className = 'hidden flex-col w-full'
	document.querySelector('#configuration').appendChild(appConfigElement)

	///
	//
	// STOCK TICKER SETTINGS
	//
	///
	const stockTickerSettings = document.querySelector('#stock-ticker-settings')
	const stockButton = document.querySelector('#app-config-button')

	stockButton.addEventListener('click', () => {
		window.dashboard.style.display = 'none'
		stockTickerSettings.style.display = 'flex'
		window.currentSection = stockTickerSettings
	})

	let finnhubApiToken = window.fetchedConfig['stock-ticker'].apiToken
	let apiRefreshInterval = window.fetchedConfig['stock-ticker'].refreshInterval
	let stockDuration = window.fetchedConfig['stock-ticker'].stockDuration

	// Get the array of stocks from the server, and just add a trivial "id" to them
	// Math.random() isn't necessarily unique/secure, but it's good enough to allow for frontend manipulation of the stocks array
	const stocks = window.fetchedConfig['stock-ticker'].stocks?.map(s => ({...s, id: `${Math.random()}`})) || []

	const finnhubApiTokenInput = document.getElementById('api-token')
	const apiRefreshIntervalInput = document.getElementById('refresh-interval')
	const stockDurationInput = document.querySelector('#stock-duration')
	const testApiTokenButton = document.getElementById('test-api-token-button')

	finnhubApiTokenInput.value = finnhubApiToken
	apiRefreshIntervalInput.value = apiRefreshInterval
	stockDurationInput.value = stockDuration

	testApiTokenButton.disabled = !(finnhubApiToken && finnhubApiToken.length === 40)

	finnhubApiTokenInput.addEventListener('input', e => {
		finnhubApiToken = e.target.value.trim()
		testApiTokenButton.disabled = !(finnhubApiToken && finnhubApiToken.length === 40)
	})

	apiRefreshIntervalInput.addEventListener('input', e => {
		apiRefreshInterval = returnValidIntInRange(e.target.value, 0, 3600000)
		e.target.value = apiRefreshInterval
	})

	apiRefreshIntervalInput.addEventListener('keydown', e => {
		if (e.key === 'ArrowUp' && apiRefreshInterval + 1 <= 3600000) {
			apiRefreshInterval++
			e.target.value = apiRefreshInterval
		}

		if (e.key === 'ArrowDown' && apiRefreshInterval - 1 >= 60000) {
			apiRefreshInterval--
			e.target.value = apiRefreshInterval
		}
	})

	stockDurationInput.addEventListener('input', e => {
		stockDuration = returnValidIntInRange(e.target.value, 0, 3600000)
		e.target.value = stockDuration
	})

	stockDurationInput.addEventListener('keydown', e => {
		if (e.key === 'ArrowUp' && stockDuration + 1 <= 3600000) {
			stockDuration++
			e.target.value = stockDuration
		}

		if (e.key === 'ArrowDown' && stockDuration - 1 >= 1000) {
			stockDuration--
			e.target.value = stockDuration
		}
	})

	const testFinnhubToken = async () => {
		if (finnhubApiToken && finnhubApiToken.length === 40) {
			try {
				const response = await fetch(`https://finnhub.io/api/v1/quote?symbol=AAPL&token=${finnhubApiToken}`);
				if (!response.ok) {
					throw new Error();
				}

				const finnhubResponse = await response.json()

				// Basic validation of the JSON response - c and t are to be expected https://finnhub.io/docs/api/quote
				return 'c' in finnhubResponse && 't' in finnhubResponse
			} catch {
				return false
			}
		}

		return false
	}

	testApiTokenButton.addEventListener('click', async e => {
		e.preventDefault()

		const tokenIsValid = await testFinnhubToken()

		if (tokenIsValid) {
			alert('Your Finnhub API Token is valid!')
		} else {
			alert('Finnhub API Token Validation Failed - Please check your token and/or your internet connection.')
		}
	})

	const stocksWrapper = document.getElementById('stocks-wrapper')
	const addStockButton = document.querySelector('#add-stock-button')
	const testStocksButton = document.querySelector('#test-stocks-button')
	const stockTickerSettingsButton = document.querySelector('#stock-ticker-settings')

	addStockButton.disabled = stocks?.length === 8

	function findStockByID(stockID) {
		const stockIndex = stocks.findIndex(s => s.id === stockID)

		if (stockIndex !== -1) {
			return stocks[stockIndex]
		}

		return {}
	}

	function createInputWithLabel(labelText, inputValue, stockID, isTicker, isCompanyName, isBrandColour) {
		const label = document.createElement('label');
		label.className = 'flex flex-col text-center items-center';

		label.innerHTML = `${labelText}: <input placeholder="${labelText}" ${isBrandColour ? 'type="color"' : ''} class="bg-black/50 text-white focus:bg-black/90 m-2 rounded-md ${isBrandColour ? 'p-1 h-10 w-14 block cursor-pointer' : 'p-2'}" maxlength="40">`;
		const input = label.querySelector('input');
		input.value = inputValue;

		if (isTicker) {
			input.maxLength = 4
			input.addEventListener('input', e => {
				const formattedTicker = e.target.value.trim().toUpperCase()
				e.target.value = formattedTicker

				const currentStock = findStockByID(stockID)
				currentStock.ticker = formattedTicker
			});
		}

		if (isCompanyName) {
			input.maxLength = 15

			input.addEventListener('input', e => {
				const formattedCompanyName = e.target.value
				e.target.value = formattedCompanyName

				const currentStock = findStockByID(stockID)
				currentStock.companyName = formattedCompanyName.trim()
			});
		}

		if (isBrandColour) {
			input.value = inputValue ? rgbToHex(inputValue) : ''

			const hexCodeDisplay = document.createElement('div')
			hexCodeDisplay.innerHTML = `<span class="hex-display">${inputValue ? rgbToHex(inputValue) : ''}</span>`
			label.appendChild(hexCodeDisplay)

			input.addEventListener('input', e => {
				hexCodeDisplay.innerHTML = e.target.value

				const currentStock = findStockByID(stockID)
				currentStock.brandColour = hexToRgb(e.target.value)
			});
		}

		return label;
	}

	function addStock(stock) {
		const stockWrapper = document.createElement('div');
		stockWrapper.classList.add('stock-wrapper')

		const stockDiv = document.createElement('div')
		stockDiv.classList.add('flex')
		stockDiv.classList.add('flex-col')
		stockDiv.classList.add('md:flex-row')

		const tickerLabel = createInputWithLabel('Ticker', stock.ticker, stock.id, true);
		stockDiv.appendChild(tickerLabel);

		const companyNameLabel = createInputWithLabel('Company Name', stock.companyName, stock.id, false, true);
		stockDiv.appendChild(companyNameLabel);

		const brandColourLabel = createInputWithLabel('Colour', stock.brandColour, stock.id, false, false, true);
		stockDiv.appendChild(brandColourLabel);

		const removeStockButton = document.createElement('button')
		removeStockButton.className = 'delete-stock inline-flex items-center py-1 px-2 gap-3 rounded-md bg-red-500/50 text-center shadow-md hover:bg-red-600 hover:shadow-inner transition-colors'
		removeStockButton.innerHTML = '<div class="h-4 w-4"><svg xmlns="http://www.w3.org/2000/svg" fill="none" stroke-width="1.5" viewBox="0 0 24 24"><path stroke="#fff" stroke-linecap="round" stroke-linejoin="round" d="m20 9-1.995 11.346A2 2 0 0 1 16.035 22h-8.07a2 2 0 0 1-1.97-1.654L4 9M21 6h-5.625M3 6h5.625m0 0V4a2 2 0 0 1 2-2h2.75a2 2 0 0 1 2 2v2m-6.75 0h6.75" /></svg></div>Remove Stock'

		removeStockButton.onclick = e => {
			e.preventDefault()
			const stockRemovalConfirmed = confirm('Are you sure you would like to remove this stock?')

			if (stockRemovalConfirmed) {
				const removalIndex = stocks.findIndex(s => s.id === stock.id)
				stocks.splice(removalIndex, 1)
				stocksWrapper.removeChild(stockWrapper)
				addStockButton.disabled = false
			}
		}

		stockWrapper.appendChild(stockDiv)
		stockWrapper.appendChild(removeStockButton)
		stocksWrapper.appendChild(stockWrapper);
	}

	stocks.forEach(stock => {
		// Stocks without a ticker are empty
		if (stock.ticker === '') {
			return
		}

		addStock(stock, Math.random())
	});

	addStockButton.addEventListener('click', e => {
		e.preventDefault()

		stocks.push({
			id: `${Math.random()}`, ticker: '', companyName: '', brandColour: {r: 255, g: 255, b: 255},
		})

		addStock(stocks[stocks.length - 1])
		addStockButton.disabled = stocks?.length === 8
	})

	const testTicker = async ticker => {
		if (finnhubApiToken && finnhubApiToken.length === 40) {
			try {
				const response = await fetch(`https://finnhub.io/api/v1/quote?symbol=${ticker}&token=${finnhubApiToken}`);
				if (!response.ok) {
					throw new Error();
				}

				const finnhubResponse = await response.json()

				// Basic validation of the JSON response - https://finnhub.io/docs/api/quote
				return !(finnhubResponse.d === null && finnhubResponse.c === 0 && finnhubResponse.dp === null)
			} catch {
				return false
			}
		}

		return false
	}

	testStocksButton.addEventListener('click', async e => {
		e.preventDefault()

		// Remove the stocks that don't have a ticker, and return just the tickers
		const validTickers = stocks.filter(stock => stock.ticker !== '').map(stock => ({value: stock.ticker, isValid: false}))

		if (validTickers.length) {
			await Promise.all(validTickers.map(async ticker => {
				ticker.isValid = await testTicker(ticker.value)
			}))
		}

		const validationResponse = validTickers.map(({value, isValid}) => `${value}: ${isValid ? 'Valid' : 'Invalid'}`).join(' \n')

		alert('Here are the results of validating your tickers:\n\n' + validationResponse)
	})

	stockTickerSettingsButton.addEventListener('submit', async e => {
		e.preventDefault()

		if (!finnhubApiToken.length || finnhubApiToken.length > 40) {
			alert('Please enter a valid Finnhub API token')
			return
		}

		if (apiRefreshInterval < 60000) {
			alert('Please specify an API refresh interval of at least 60000 (one minute).')
			return
		}

		if (stockDuration < 1000) {
			alert('Please specify a stock duration of at least 1000 (one second).')
			return
		}

		const tokenIsValid = await testFinnhubToken()

		if (!tokenIsValid) {
			alert('Finnhub API Token Validation Failed - Please check your token and/or your internet connection.')
			return
		}

		// Get a list of all stocks with non-empty tickers
		const validStocks = stocks.filter(stock => stock.ticker.length)

		if (validStocks.find(stock => stock.ticker.length > 4)) {
			alert('Please ensure all stock tickers are 4 characters or less')
			return
		}

		if (validStocks.find(stock => stock.companyName.length > 15)) {
			alert('Please ensure all company names are 15 characters or less')
		}

		try {
			const formData = new FormData()
			formData.append('apiToken', finnhubApiToken)
			formData.append('refreshInterval', apiRefreshInterval)
			formData.append('stocks', JSON.stringify(validStocks));
			formData.append('stockDuration', stockDuration)

			const response = await fetch(`${window.API_URL}/config`, {method: 'POST', body: formData})

			if (!response.ok) {
				throw new Error(`Error! status: ${response.status}`)
			}

			alert('Stock Ticker Settings updated successfully! Please allow a moment for your Luxigrid to restart, and your changes will take effect.')
			refreshAfterUpdate()
		} catch (error) {
			console.log(error)
			alert('Stock Ticker Settings update failed. Please try again.')
		}
	})
}
