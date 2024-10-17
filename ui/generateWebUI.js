import {readFile, writeFile} from 'node:fs/promises';
import {gzip} from 'node:zlib';
import {promisify} from 'node:util';

import {JSDOM} from 'jsdom';
import postcss from 'postcss';

const gzipAsync = promisify(gzip);

async function generateWebUI() {
	const inputFileName = 'dist/index.html';
	const outputFileName = 'dist/html.h';

	try {
		const data = await readFile(inputFileName, 'utf8');

		// Use JSDOM to remove unused CSS custom properties/variables
		const dom = new JSDOM(data);
		const document = dom.window.document;
		const styleSheets = document.querySelectorAll('style');

		for (const styleSheet of styleSheets) {
			const css = styleSheet.textContent;
			const result = await postcss([
				root => {
					root.walkDecls(decl => {
						if (decl.prop.startsWith('--')) {
							const varName = decl.prop;
							let isUsed = false;
							root.walkDecls(d => {
								if (d.value.includes(varName)) {
									isUsed = true;
								}
							});
							if (!isUsed) {
								decl.remove();
							}
						}
					});
				},
			]).process(css, {from: undefined});

			styleSheet.textContent = result.css;
		}

		// GZIP compress the HTML
		const compressed = await gzipAsync(data);

		// Convert compressed data to a uint8_t array format, with 64 bytes per line
		const hexArray = Array.from(compressed).map(byte => `0x${byte.toString(16).padStart(2, '0')}`);
		const formattedArray = [];

		while (hexArray.length) {
			formattedArray.push('  ' + hexArray.splice(0, 64).join(', '));
		}

		const topComment = '// THIS FILE IS AUTO-GENERATED. PLEASE DO NOT TRY TO EDIT OR SOURCE-CONTROL IT.\n'
		const guard = '#ifndef LUXIGRID_WEB_UI_GUARD\n#define LUXIGRID_WEB_UI_GUARD\n\n'

		// Include a variable that stores the size of the array of bytes
		const lengthContent = `const uint16_t updatePageLength = ${compressed.length};`;

		// The updatePage variable will store the gzipped binary array
		const arrayContent = `const uint8_t updatePage[] = {\n${formattedArray.join(',\n')}\n};`;

		// Write to the output file
		await writeFile(outputFileName, `${topComment}${guard}${lengthContent}\n\n${arrayContent}\n\n#endif`, 'utf8');
		console.log(`\n✓ File processed, compressed, and stringified successfully - see ${outputFileName}`);
	} catch (err) {
		console.error(`Error processing file: ${err}`);
	}
}

generateWebUI();
