/* _    _  _ _  _ _ ____ ____ _ ___
 * |    |  |  \/  | | __ |__/ | |  \
 * |___ |__| _/\_ | |__] |  \ | |__/
 * =================================
 * Luxigrid - GIF Helper Functions
 * Copyright (c) 2024 OverScore Media - MIT License
 *
 * Adapted from ESP32-HUB75-MatrixPanel-DMA:
 * https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA/tree/master/examples/AnimatedGIFPanel_SD
 * Copyright (c) 2018-2032 Faptastic - MIT License
 *
 * Makes use of https://github.com/bitbank2/AnimatedGIF by Larry Bank
 * Licensed under Apache 2.0 License - See licenses/APACHE_LICENSE.txt
 * Copyright (c) 2020-2024 BitBank Software, Inc.
 * ==================================
 *
 * MIT LICENSE:
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef GIF_GUARD
#define GIF_GUARD

#include "Arduino.h"

#include <AnimatedGIF.h>
#include <SD.h>

#include "luxigrid.h"

AnimatedGIF gif;

// Bring in maxGifDuration from wherever it was defined outside this file (i.e., gif-player.cpp)
// extern const int maxGifDuration;

extern GifPlayerConfig gifPlayerConfig;

extern File FSGifFile;

static void *GIFOpenFile(const char *fname, int32_t *pSize) {
	FSGifFile = SD.open(fname);

	if (FSGifFile) {
		*pSize = FSGifFile.size();
		return (void *)&FSGifFile;
	}

	return NULL;
}

static void GIFCloseFile(void *pHandle) {
	File *f = static_cast<File *>(pHandle);

	if (f != NULL) {
		f->close();
	}
}

static int32_t GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen) {
	int32_t iBytesRead;
	iBytesRead = iLen;
	File *f = static_cast<File *>(pFile->fHandle);

	// Note: If you read a file all the way to the last byte, seek() stops working
	if ((pFile->iSize - pFile->iPos) < iLen) {
		iBytesRead = pFile->iSize - pFile->iPos - 1;  // <-- Ugly work-around
	}

	if (iBytesRead <= 0) {
		return 0;
	}

	iBytesRead = (int32_t)f->read(pBuf, iBytesRead);
	pFile->iPos = f->position();

	return iBytesRead;
}

static int32_t GIFSeekFile(GIFFILE *pFile, int32_t iPosition) {
	int i = micros();
	File *f = static_cast<File *>(pFile->fHandle);

	f->seek(iPosition);
	pFile->iPos = (int32_t)f->position();
	i = micros() - i;

	return pFile->iPos;
}

void GIFDraw(GIFDRAW *pDraw) {
	uint8_t *s;
	uint16_t *d, *usPalette, usTemp[320];
	int x, y, iWidth;

	iWidth = pDraw->iWidth;

	if (iWidth > PANEL_RES_X) {
		iWidth = PANEL_RES_X;
	}

	usPalette = pDraw->pPalette;

	// Current line
	y = pDraw->iY + pDraw->y;
	s = pDraw->pPixels;

	// Restore to background colour
	if (pDraw->ucDisposalMethod == 2) {
		for (x = 0; x < iWidth; x++) {
			if (s[x] == pDraw->ucTransparent) {
				s[x] = pDraw->ucBackground;
			}
		}

		pDraw->ucHasTransparency = 0;
	}

	// Apply the new pixels to the main image
	if (pDraw->ucHasTransparency) {
		uint8_t *pEnd, c, ucTransparent = pDraw->ucTransparent;
		int x, iCount;
		pEnd = s + iWidth;
		x = 0;

		// Count non-transparent pixels
		iCount = 0;

		while (x < iWidth) {
			c = ucTransparent - 1;
			d = usTemp;

			// While looking for opaque pixels
			while (c != ucTransparent && s < pEnd) {
				c = *s++;

				if (c == ucTransparent) {  // Done, stop
					s--;                   // Back up to treat it like transparent
				} else {                   // Opaque
					*d++ = usPalette[c];
					iCount++;
				}
			}

			// Are there any opaque pixels?
			if (iCount) {
				for (int xOffset = 0; xOffset < iCount; xOffset++) {
					dma_display->drawPixel(x + xOffset, y, usTemp[xOffset]);  // 565 Color Format
				}

				x += iCount;
				iCount = 0;
			}

			// No, look for a run of transparent pixels
			c = ucTransparent;

			while (c == ucTransparent && s < pEnd) {
				c = *s++;

				if (c == ucTransparent) {
					iCount++;
				} else {
					s--;
				}
			}
			if (iCount) {
				x += iCount;  // Skip these
				iCount = 0;
			}
		}
	} else {
		s = pDraw->pPixels;

		// Translate the 8-bit pixels through the RGB565 palette (already byte-reversed)
		for (x = 0; x < iWidth; x++) {
			dma_display->drawPixel(x, y, usPalette[*s++]);
		}
	}
}

// 0 = infinite
int gifPlay(const char *gifPath) {
	if (!gif.open(gifPath, GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw)) {
		log_n("Could not open gif %s", gifPath);
	}

	// Delay for the last frame
	int frameDelay = 0;
	// Overall delay
	int then = 0;

	while (gif.playFrame(true, &frameDelay)) {
		then += frameDelay;

		// Avoid being trapped in infinite GIF's
		if (then > gifPlayerConfig.maxGifDuration) {
			break;
		}
	}

	gif.close();

	return then;
}

#endif