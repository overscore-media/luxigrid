/** @type {import('tailwindcss').Config} */
export default {
	content: [
		'./index.html',
		'./main.js',
		'./templates/**/*.hbs',
		'./apps/**/*.js',
		'./apps/**/*.hbs',
	],
	theme: {
		extend: {},
	},
	plugins: [
		require('@tailwindcss/forms'),
	],
}