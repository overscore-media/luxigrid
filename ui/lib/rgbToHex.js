export default function rgbToHex({r, g, b}) {
	return `#${[r, g, b].map(x => x.toString(16).padStart(2, '0')).join('')}`;
}
