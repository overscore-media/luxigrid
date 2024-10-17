export function returnValidIntInRange(val, minValue, maxValue) {
	const parsedValue = Number(val)

	if (!Number.isInteger(parsedValue) || parsedValue < minValue || parsedValue > maxValue) {
		return minValue
	}

	return parsedValue
}
