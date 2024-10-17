module.exports = {
	env: {
		commonjs: true,
		es2022: true,
		browser: true,
	},
	extends: [
		'xo',
	],
	parser: '@babel/eslint-parser',
	parserOptions: {
		ecmaVersion: 'latest',
		requireConfigFile: false,
	},
	rules: {
		semi: 'off',
		'new-cap': 'off',
		'capitalized-comments': 'off',
		complexity: 'off',
		quotes: ['error', 'single'],
		'one-var': 'off',
		'max-nested-callbacks': 'off',
		'max-params': 'off',
		'max-depth': 'off',
		'prefer-destructuring': 'off',
		'no-constant-binary-expression': 'off',
		'no-negated-condition': 'off',
		'no-await-in-loop': 'off',
		'no-alert': 'off',
	},
};
