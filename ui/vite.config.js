import {defineConfig, loadEnv} from 'vite';
import {viteSingleFile} from 'vite-plugin-singlefile';
import {createHtmlPlugin} from 'vite-plugin-html';
import {minify} from 'html-minifier-terser';

export default defineConfig(({mode}) => {
	const {VITE_CURRENT_APP, VITE_API_URL} = loadEnv(mode, process.cwd())

	return {
		define: {
			__APP_SPECIFIC_CONFIG_FILENAME__: JSON.stringify(VITE_CURRENT_APP ? `./apps/${VITE_CURRENT_APP}/${VITE_CURRENT_APP}.js` : null),
			__API_URL__: JSON.stringify(mode === 'development' ? VITE_API_URL : null),
		},

		build: {
			cssMinify: true,
		},

		plugins: [
			viteSingleFile({
				removeViteModuleLoader: true,
			}),

			createHtmlPlugin({
				minify: true,
			}),

			// Custom minifier for .hbs imports
			{
				async transform(code, id) {
					if (id.endsWith('.hbs')) {
						const minifiedHtml = await minify(code, {
							collapseWhitespace: true,
							removeComments: true,
							removeRedundantAttributes: true,
							removeScriptTypeAttributes: true,
							removeStyleLinkTypeAttributes: true,
							useShortDoctype: true,
						});

						return {code: `export default \`${minifiedHtml}\``, map: null};
					}

					return null;
				},
			},
		],
	}
})
