#include "pix.h"
#include <stdio.h>

PIX* pixCreateHeader(l_int32 width, l_int32 height, l_int32 depth);
void pixFree(PIX  *pix);
l_int32 pixCopyResolution(PIX *pixd, const PIX *pixs);
l_ok pixGetDimensions(const PIX *pix, l_int32 *pw, l_int32 *ph, l_int32 *pd);
PIX* pixFlipTB(PIX *pixd);

BOX* boxCreate(l_int32 x, l_int32 y, l_int32 w, l_int32 h);
void boxDestroy(BOX  **pbox);
l_ok boxGetGeometry(BOX *box, l_int32 *px, l_int32 *py, l_int32 *pw, l_int32 *ph);
l_ok boxSetGeometry(BOX *box, l_int32 x, l_int32 y, l_int32 w, l_int32 h);
BOX* boxCopy(BOX *box);
BOX* boxClipToRectangle(BOX* box, l_int32 wi, l_int32 hi);

#define PPM2PPI(res) (l_int32)((l_float32)res / 39.37 + 0.5);
#define PPI2PPM(res) (LONG)((l_float32)(res - 0.5) * 39.37);

typedef HANDLE(__stdcall* pfnCreateFile)(
	_In_ void* lpFileName,
	_In_ DWORD dwDesiredAccess,
	_In_ DWORD dwShareMode,
	_In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	_In_ DWORD dwCreationDisposition,
	_In_ DWORD dwFlagsAndAttributes,
	_In_opt_ HANDLE hTemplateFile
);

//----------------------------------------------------------------------------//
//----------------------------------- Pix ------------------------------------//
//----------------------------------------------------------------------------//

PIX* pixCreate(l_int32 width, l_int32 height, l_int32 depth)
{
	PIX       *pixd;
	l_uint32  *data;

	if ((pixd = pixCreateHeader(width, height, depth)) == NULL)
		return NULL;

	if ((data = (l_uint32 *)malloc(4LL * pixd->wpl * height)) == NULL)
	{
		pixDestroy(&pixd);
		return NULL;
	}

	pixd->data = data;
	//pixSetPadBits(pixd, 0);

	memset(pixd->data, 0, 4LL * pixd->wpl * pixd->h);
	return pixd;
}

PIX* pixCreateHeader(l_int32 width, l_int32 height, l_int32 depth)
{
	l_int32   wpl;
	l_uint64  wpl64, bignum;
	PIX      *pixd;

	if ((depth != 1) && (depth != 2) && (depth != 4) && (depth != 8)
		&& (depth != 16) && (depth != 24) && (depth != 32))
		return NULL;
	if (width <= 0 || height <= 0)
		return NULL;

	/* Avoid overflow in malloc, malicious or otherwise */
	wpl64 = ((l_uint64)width * (l_uint64)depth + 31) / 32;
	if (wpl64 > ((1LL << 24) - 1))
		return NULL;

	wpl = (l_int32)wpl64;
	bignum = 4LL * wpl * height;   /* number of bytes to be requested */
	if (bignum > ((1LL << 31) - 1))
		return NULL;

	pixd = (PIX *)calloc(1, sizeof(PIX));
	pixd->w = width;
	pixd->h = height;
	pixd->d = depth;
	pixd->wpl = wpl;

	pixd->spp = (depth == 24 || depth == 32) ? 3 : 1;
	pixd->refcount = 1;
	pixd->informat = IFF_UNKNOWN;
	return pixd;
}

void pixDestroy(PIX  **ppix)
{
	PIX  *pix;

	if (!ppix)
		return;

	if ((pix = *ppix) == NULL)
		return;

	pixFree(pix);
	*ppix = NULL;
}

void pixFree(PIX  *pix)
{
	l_uint32  *data;
	char      *text;

	if (!pix)
		return;

	pix->refcount--;
	if (pix->refcount > 0)
		return;

	if (pix->data)
		free(pix->data);
	if (pix->text)
		free(pix->text);
	//pixDestroyColormap(pix);
	free(pix);

	return;
}

l_ok pixRasterop(PIX     *pixd,
	l_int32  dx,
	l_int32  dy,
	l_int32  dw,
	l_int32  dh,
	l_int32  op,
	PIX     *pixs,
	l_int32  sx,
	l_int32  sy)
{
	l_int32  dpw, dph, dpd, spw, sph, spd;

	if (!pixd)
		return 1;

	if (op == PIX_DST)   /* no-op */
		return 0;

	pixGetDimensions(pixd, &dpw, &dph, &dpd);

	/* Check if operation is only on dest */
	if (op == PIX_CLR || op == PIX_SET || op == PIX_NOT(PIX_DST))
	{
		//rasteropUniLow(pixGetData(pixd), dpw, dph, dpd, pixGetWpl(pixd),
		//	dx, dy, dw, dh, op);
		return 0;
	}

	/* Two-image rasterop; the depths must match */
	if (!pixs)
		return 1;
	pixGetDimensions(pixs, &spw, &sph, &spd);
	if (dpd != spd)
		return 1;

	//rasteropLow(pixGetData(pixd), dpw, dph, dpd, pixGetWpl(pixd),
	//	dx, dy, dw, dh, op,
	//	pixGetData(pixs), spw, sph, pixGetWpl(pixs), sx, sy);
	return 0;
}

PIX* pixClipRectangle(const PIX* pixs, int x, int y, int w, int h)
{
	BOX* box = boxCreate(x, y, w, h);
	if (!box)
		return NULL;
	PIX* pixd = pixClipRectangle(pixs, box, NULL);
	boxDestroy(&box);
	return pixd;
}

PIX* pixClipRectangle(const PIX* pixs, BOX* box, BOX** pboxc)
{
	if (!pixs || !box || pixs->d != 32)
		return NULL;
	if (pboxc)
		*pboxc = NULL;

	if (box->w <= 0 || box->h <= 0)
		return NULL;
	if (pixs->w < box->x + box->w || pixs->h < box->y + box->h)
		return NULL;

	Pix* pixd = pixCreate(box->w, box->h, pixs->d);
	if (!pixd)
		return NULL;

	pixCopyResolution(pixd, pixs);

	l_uint32 wpls = pixs->wpl;
	l_uint32 wpld = pixd->wpl;
	l_uint32* src = pixs->data + box->y * wpls + box->x;
	l_uint32* dst = pixd->data;
	for (int yy = 0; yy < box->h; yy++)
	{
		memcpy(dst, src, wpld * 4);
		src += wpls;
		dst += wpld;
	}

	return pixd;
}

//PIX* pixClipRectangle(PIX* pixs, BOX* box, BOX** pboxc)
//{
//	l_int32  w, h, d, bx, by, bw, bh;
//	BOX     *boxc;
//	PIX     *pixd;
//
//	if (pboxc)
//		*pboxc = NULL;
//	if (!pixs)
//		return NULL;
//	if (!box)
//		return NULL;
//
//	/* Clip the input box to the pix */
//	pixGetDimensions(pixs, &w, &h, &d);
//	if ((boxc = boxClipToRectangle(box, w, h)) == NULL)
//	{
//		return NULL;
//	}
//	boxGetGeometry(boxc, &bx, &by, &bw, &bh);
//
//	/* Extract the block */
//	if ((pixd = pixCreate(bw, bh, d)) == NULL)
//	{
//		boxDestroy(&boxc);
//		return NULL;
//	}
//
//	pixCopyResolution(pixd, pixs);
//	//pixCopyColormap(pixd, pixs);
//	//pixCopyText(pixd, pixs);
//	// TODO:
//	pixRasterop(pixd, 0, 0, bw, bh, PIX_SRC, pixs, bx, by);
//
//	if (pboxc)
//		*pboxc = boxc;
//	else
//		boxDestroy(&boxc);
//
//	return pixd;
//}

PIX* pixReadMemBmp(const l_uint8 *cdata, size_t size)
{
	l_uint8    pel[4];
	l_uint8   *fdata, *data;
	l_int16    bftype, depth, d;
	l_int32    offset, ihbytes, width, height, height_neg, xres, yres;
	l_int32    compression, imagebytes, fdatabytes, cmapbytes, ncolors, maxcolors;
	l_int32    fdatabpl, extrabytes, pixWpl, pixBpl, i, j, k;
	l_uint32  *line, *pixdata, *pword;
	l_int64    npixels;
	BITMAPFILEHEADER    *bmpfh;
	BITMAPINFOHEADER    *bmpih;
	PIX       *pix, *pix1;
	PIXCMAP   *cmap;

	if (!cdata)
		NULL;
	if (size < sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER))
		NULL;

	/* Verify this is an uncompressed bmp */
	bmpfh = (BITMAPFILEHEADER*)cdata;
	if (bmpfh->bfType != BMP_ID)
		return NULL;
	bmpih = (BITMAPINFOHEADER*)(cdata + sizeof(BITMAPFILEHEADER));
	compression = bmpih->biCompression;
	if (compression != BI_RGB && compression != BI_BITFIELDS)
		return NULL;

	/* Find the offset from the beginning of the file to the image data */
	offset = bmpfh->bfOffBits;

	/* Read the remaining useful data in the infoheader.
	* Note that the first 4 bytes give the infoheader size. */
	ihbytes = bmpih->biSize;
	width = bmpih->biWidth;
	height = bmpih->biHeight;
	depth = bmpih->biBitCount;
	imagebytes = bmpih->biSizeImage;
	xres = bmpih->biXPelsPerMeter;
	yres = bmpih->biYPelsPerMeter;

	/* Some sanity checking.  We impose limits on the image
	* dimensions, resolution and number of pixels.  We make sure the
	* file is the correct size to hold the amount of uncompressed data
	* that is specified in the header.  The number of colormap
	* entries is checked: it can be either 0 (no cmap) or some
	* number between 2 and 256.
	* Note that the imagebytes for uncompressed images is either
	* 0 or the size of the file data.  (The fact that it can
	* be 0 is perhaps some legacy glitch). */
	if (width < 1 || width > L_MAX_ALLOWED_WIDTH)
		return NULL;
	if (height == 0 || height < -L_MAX_ALLOWED_HEIGHT || height > L_MAX_ALLOWED_HEIGHT)
		return NULL;
	if (xres < 0 || xres > L_MAX_ALLOWED_RES ||
		yres < 0 || yres > L_MAX_ALLOWED_RES)
		return NULL;
	height_neg = 0;
	if (height < 0)
	{
		height_neg = 1;
		height = -height;
	}
	if (ihbytes != 40 && ihbytes != 108 && ihbytes != 124)
	{
		return NULL;
	}
	npixels = 1LL * width * height;
	if (npixels > L_MAX_ALLOWED_PIXELS)
		return NULL;
	if (depth != 24 && depth != 32)
		return NULL;

	fdatabpl = 4 * ((1LL * width * depth + 31) / 32);
	fdatabytes = fdatabpl * height;
	if (imagebytes != 0 && imagebytes != fdatabytes)
		return NULL;

	/* In the original spec, BITMAPINFOHEADER is 40 bytes.
	* There have been a number of revisions, to capture more information.
	* For example, the fifth version, BITMAPV5HEADER, adds 84 bytes
	* of ICC color profiles.  We use the size of the infoheader
	* to accommodate these newer formats.  Knowing the size of the
	* infoheader gives more opportunity to sanity check input params. */
	cmapbytes = offset - sizeof(BITMAPFILEHEADER) - ihbytes;
	if (cmapbytes != 0)
		return NULL;
	if (size != 1LL * offset + 1LL * fdatabytes)
		return NULL; // (PIX *)ERROR_PTR("size incommensurate with image data", procName, NULL);

	/* Make a 32 bpp pix if depth is 24 bpp */
	d = (depth == 24) ? 32 : depth;
	if ((pix = pixCreate(width, height, d)) == NULL)
		return NULL;

	pix->xres = PPM2PPI(xres);
	pix->yres = PPM2PPI(yres);
	pix->informat = IFF_BMP;
	pixWpl = pix->wpl;
	pixBpl = 4 * pixWpl;

	/* Acquire the image data.  Image origin for bmp is at lower right. */
	fdata = (l_uint8 *)cdata + offset;  /* start of the bmp image data */
	pixdata = pix->data;
	// TODO: Требуется проверка.
	if (depth != 24)  /* typ. 1 or 8 bpp */
	{
		data = (l_uint8 *)pixdata + pixBpl * (height - 1);
		for (i = 0; i < height; i++)
		{
			memcpy(data, fdata, fdatabpl);
			fdata += fdatabpl;
			data -= pixBpl;
		}
	}
	else
	{  /*  24 bpp file; 32 bpp pix
		*  Note: for bmp files, pel[0] is blue, pel[1] is green,
		*  and pel[2] is red.  This is opposite to the storage
		*  in the pix, which puts the red pixel in the 0 byte,
		*  the green in the 1 byte and the blue in the 2 byte.
		*  Note also that all words are endian flipped after
		*  assignment on L_LITTLE_ENDIAN platforms.
		*
		*  We can then make these assignments for little endians:
		*      SET_DATA_BYTE(pword, 1, pel[0]);      blue
		*      SET_DATA_BYTE(pword, 2, pel[1]);      green
		*      SET_DATA_BYTE(pword, 3, pel[2]);      red
		*  This looks like:
		*          3  (R)     2  (G)        1  (B)        0
		*      |-----------|------------|-----------|-----------|
		*  and after byte flipping:
		*           3          2  (B)     1  (G)        0  (R)
		*      |-----------|------------|-----------|-----------|
		*
		*  For big endians we set:
		*      SET_DATA_BYTE(pword, 2, pel[0]);      blue
		*      SET_DATA_BYTE(pword, 1, pel[1]);      green
		*      SET_DATA_BYTE(pword, 0, pel[2]);      red
		*  This looks like:
		*          0  (R)     1  (G)        2  (B)        3
		*      |-----------|------------|-----------|-----------|
		*  so in both cases we get the correct assignment in the PIX.
		*
		*  Can we do a platform-independent assignment?
		*  Yes, set the bytes without using macros:
		*      *((l_uint8 *)pword) = pel[2];           red
		*      *((l_uint8 *)pword + 1) = pel[1];       green
		*      *((l_uint8 *)pword + 2) = pel[0];       blue
		*  For little endians, before flipping, this looks again like:
		*          3  (R)     2  (G)        1  (B)        0
		*      |-----------|------------|-----------|-----------|
		*/
		extrabytes = fdatabpl - 3 * width;
		line = pixdata + pixWpl * (height - 1);
		for (i = 0; i < height; i++)
		{
			for (j = 0; j < width; j++)
			{
				pword = line + j;
				memcpy(&pel, fdata, 3);
				fdata += 3;
				*((l_uint8 *)pword + COLOR_RED) = pel[0];
				*((l_uint8 *)pword + COLOR_GREEN) = pel[1];
				*((l_uint8 *)pword + COLOR_BLUE) = pel[2];
				//*((l_uint8 *)pword + COLOR_RED) = pel[2];
				//*((l_uint8 *)pword + COLOR_GREEN) = pel[1];
				//*((l_uint8 *)pword + COLOR_BLUE) = pel[0];

				/* should not use alpha byte, but for buggy readers,
				* set it to opaque  */
				*((l_uint8 *)pword + L_ALPHA_CHANNEL) = 255;
			}
			if (extrabytes)
			{
				for (k = 0; k < extrabytes; k++)
				{
					memcpy(&pel, fdata, 1);
					fdata++;
				}
			}
			line -= pixWpl;
		}
	}

	pixEndianByteSwap(pix);
	if (height_neg)
		pixFlipTB(pix);

	return pix;
}

//DWORD pixReadStream(FILE  *fp, l_uint8** lpBuff)
//{
//	l_uint8  *data;
//	size_t    size;
//
//	if (!fp || !lpBuff)
//		return 0;
//	*lpBuff = NULL;
//
//	rewind(fp);
//	fseek(fp, 0L, SEEK_END);
//	size = ftell(fp);
//	rewind(fp);
//
//	data = (l_uint8*)calloc(size, 1);
//	if (!data)
//		return 0;
//	long rdBytes = fread(data, 1, size, fp);
//	if (rdBytes != size)
//	{
//		free(data);
//		return 0;
//	}
//	*lpBuff = data;
//	return rdBytes;
//}

DWORD pixReadFile(void* fileName, l_uint8** lpBuff, bool isUnicode)
{
	HANDLE   hFile;
	l_uint8  *data;
	DWORD    size, rdBytes;

	if (!lpBuff)
		return 0;
	*lpBuff = NULL;
	rdBytes = 0;

	pfnCreateFile createFile = (isUnicode) ? (pfnCreateFile)CreateFileW : (pfnCreateFile)CreateFileA;
	hFile = createFile(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (INVALID_HANDLE_VALUE == hFile)
		return 0;

	__try
	{
		size = GetFileSize(hFile, NULL);
		if (INVALID_FILE_SIZE == size)
			return 0;

		data = (l_uint8*)calloc(size, 1);
		if (!data)
			return 0;

		if (!ReadFile(hFile, data, size, &rdBytes, NULL) || rdBytes != size)
		{
			free(data);
			data = NULL;
			rdBytes = 0;
		}
	}
	__finally
	{
		CloseHandle(hFile);
	}

	*lpBuff = data;
	return rdBytes;
}

//PIX* pixReadStreamBmp(FILE  *fp)
//{
//	l_uint8  *data;
//	size_t    size;
//	PIX      *pix;
//
//	if (!fp)
//		return NULL;
//
//	rewind(fp);
//	fseek(fp, 0L, SEEK_END);
//	size = ftell(fp);
//	rewind(fp);
//
//	data = (l_uint8*)calloc(size, 1);
//	if (!data)
//		return NULL;
//	long rdBytes = fread(data, 1, size, fp);
//	if (rdBytes != size)
//	{
//		free(data);
//		return NULL;
//	}
//
//	pix = pixReadMemBmp(data, size);
//	free(data);
//	return pix;
//}

PIX* pixRead(void* fileName, bool isUnicode)
{
	FILE  *fp;
	PIX   *pix;


	l_uint8* data = NULL;
	DWORD size = pixReadFile(fileName, &data, isUnicode);
	if (!size || !data)
		return NULL;

	pix = pixReadMemBmp(data, size);
	free(data);

	return pix;
}

PIX* pixRead(LPCSTR fileName)
{
	return pixRead((void*)fileName, false);
}

PIX* pixRead(LPCWSTR fileName)
{
	return pixRead((void*)fileName, true);
}

//PIX* pixRead(const char *filename)
//{
//	FILE  *fp;
//	PIX   *pix;
//
//	if (!filename)
//		return NULL;
//
//	if ((fp = fopen(filename, "rb")) == NULL)
//		return NULL;
//
//	pix = pixReadStreamBmp(fp);
//	fclose(fp);
//	if (!pix)
//		return NULL;
//	return pix;
//}

PIX* pixCopy(const PIX  *pixs)
{
	BOX* box = boxCreate(0, 0, pixs->w, pixs->h);
	if (!box)
		return NULL;
	PIX* pixd = pixClipRectangle(pixs, box, NULL);
	boxDestroy(&box);
	return pixd;
}

int pixWrite(PIX* pixs, l_int32 format, l_uint8** lpBuff)
{
	l_int32  ret;
	FILE    *fp;

	if (!pixs || pixs->d != 32)
		return 0;
	if (!lpBuff)
		return 0;
	if (format != IFF_BMP)
		return 0;

	*lpBuff = NULL;

	PIX* pix = pixCopy(pixs);
	if (!pix)
		return 0;

	pixEndianByteSwap(pix);
	LONG lLineSize = pix->wpl * 4;
	LONG lImageSize = lLineSize * pix->h;

	l_uint8* buff = (l_uint8*)malloc(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + lImageSize);
	if (!buff)
		return 0;

	BITMAPINFOHEADER bmih;
	memset(&bmih, 0, sizeof(BITMAPINFOHEADER));
	bmih.biSize = sizeof(BITMAPINFOHEADER);
	bmih.biWidth = pix->w;
	bmih.biHeight = pix->h;
	bmih.biPlanes = 1;
	bmih.biBitCount = pix->d;
	bmih.biCompression = BI_RGB;
	bmih.biSizeImage = lImageSize;
	bmih.biXPelsPerMeter = PPI2PPM(pix->xres);
	bmih.biYPelsPerMeter = PPI2PPM(pix->yres);

	int nBitsOffset = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	LONG lFileSize = nBitsOffset + lImageSize;
	BITMAPFILEHEADER bmfh;
	bmfh.bfType = BMP_ID;
	bmfh.bfOffBits = nBitsOffset;
	bmfh.bfSize = lFileSize;
	bmfh.bfReserved1 = bmfh.bfReserved2 = 0;

	l_uint8* cursor = buff;
	memcpy(cursor, &bmfh, sizeof(BITMAPFILEHEADER)); cursor += sizeof(BITMAPFILEHEADER);
	memcpy(cursor, &bmih, sizeof(BITMAPINFOHEADER)); cursor += sizeof(BITMAPINFOHEADER);
	for (int i = pix->h - 1; i >= 0; i--)
	{
		memcpy(cursor, (char*)pix->data + i * lLineSize, lLineSize); cursor += lLineSize;
	}

	pixDestroy(&pix);

	*lpBuff = buff;
	return (cursor - buff);
}

l_ok pixWrite(const char *fname, PIX* pixs, l_int32 format)
{
	l_uint8* buff = NULL;
	int size = pixWrite(pixs, format, &buff);
	if (!size || !buff)
		return 1;

	FILE *F = fopen(fname, "wb");
	fwrite(buff, 1, size, F);
	fclose(F);
	free(buff);

	//l_int32  ret;
	//FILE    *fp;

	//if (!pixs || pixs->d != 32)
	//	return 1;
	//if (!fname)
	//	return 1;
	//if (format != IFF_BMP)
	//	return 1;

	//PIX* pix = pixCopy(pixs);
	//if (!pix)
	//	return 1;

	//pixEndianByteSwap(pix);
	//LONG lLineSize = pix->wpl * 4;
	//LONG lImageSize = lLineSize * pix->h;
	//
	//BITMAPINFOHEADER bmih;
	//memset(&bmih, 0, sizeof(BITMAPINFOHEADER));
	//bmih.biSize = sizeof(BITMAPINFOHEADER);
	//bmih.biWidth = pix->w;
	//bmih.biHeight = pix->h;
	//bmih.biPlanes = 1;
	//bmih.biBitCount = pix->d;
	//bmih.biCompression = BI_RGB;
	//bmih.biSizeImage = lImageSize;
	//bmih.biXPelsPerMeter = PPI2PPM(pix->xres);
	//bmih.biYPelsPerMeter = PPI2PPM(pix->yres);

	//FILE *F = fopen(fname, "wb");
	//int nBitsOffset = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	//LONG lFileSize = nBitsOffset + lImageSize;
	//BITMAPFILEHEADER bmfh;
	//bmfh.bfType = BMP_ID;
	//bmfh.bfOffBits = nBitsOffset;
	//bmfh.bfSize = lFileSize;
	//bmfh.bfReserved1 = bmfh.bfReserved2 = 0;
	//
	//UINT nWrittenFileHeaderSize = fwrite(&bmfh, 1, sizeof(BITMAPFILEHEADER), F);
	//UINT nWrittenInfoHeaderSize = fwrite(&bmih, 1, sizeof(BITMAPINFOHEADER), F);
	//UINT nWrittenDIBDataSize = 0;
	//for (int i = pix->h - 1; i >= 0; i--)
	//{
	//	nWrittenDIBDataSize += fwrite((char*)pix->data + i * lLineSize, 1, lLineSize, F);
	//}
	////nWrittenDIBDataSize += fwrite(pix->data, 1, lImageSize, F);
	//
	//fclose(F);
	//pixDestroy(&pix);

	//return 0;
}

l_int32 pixCopyResolution(PIX *pixd, const PIX *pixs)
{
	if (!pixs)
		return 1;
	if (!pixd)
		return 1;
	if (pixs == pixd)
		return 0;   /* no-op */

	pixd->xres = pixs->xres;
	pixd->yres = pixs->yres;
	return 0;
}

l_ok pixGetDimensions(const PIX *pix, l_int32 *pw, l_int32 *ph, l_int32 *pd)
{
	if (pw) *pw = 0;
	if (ph) *ph = 0;
	if (pd) *pd = 0;
	if (!pix)
		return 1;

	if (pw) *pw = pix->w;
	if (ph) *ph = pix->h;
	if (pd) *pd = pix->d;
	return 0;
}

l_ok pixEndianByteSwap(PIX  *pixs)
{
	l_uint32  *data;
	l_int32    i, j, h, wpl;
	l_uint32   word;

	if (!pixs)
		return 1;

	data = pixs->data;
	wpl = pixs->wpl;
	h = pixs->h;
	for (i = 0; i < h; i++)
	{
		for (j = 0; j < wpl; j++, data++)
		{
			word = *data;
			*data = (word >> 24) |
				((word >> 8) & 0x0000ff00) |
				((word << 8) & 0x00ff0000) |
				(word << 24);
		}
	}

	return 0;
}

PIX* pixFlipTB(PIX *pixd)
{
	l_int32    h, d, wpl, i, k, h2, bpl;
	l_uint32  *linet, *lineb;
	l_uint32  *data, *buffer;

	if (!pixd)
		return NULL;
	pixGetDimensions(pixd, NULL, &h, &d);
	if (d != 1 && d != 2 && d != 4 && d != 8 && d != 16 && d != 32)
		return NULL;

	data = pixd->data;
	wpl = pixd->wpl;
	if ((buffer = (l_uint32 *)calloc(wpl, sizeof(l_uint32))) == NULL)
		return NULL;

	h2 = h / 2;
	bpl = 4 * wpl;
	for (i = 0, k = h - 1; i < h2; i++, k--)
	{
		linet = data + i * wpl;
		lineb = data + k * wpl;
		memcpy(buffer, linet, bpl);
		memcpy(linet, lineb, bpl);
		memcpy(lineb, buffer, bpl);
	}

	free(buffer);
	return pixd;
}

//----------------------------------------------------------------------------//
//----------------------------------- Box ------------------------------------//
//----------------------------------------------------------------------------//

BOX* boxCreate(l_int32 x, l_int32 y, l_int32 w, l_int32 h)
{
	BOX  *box;

	if (w < 0 || h < 0)
		return NULL;
	if (x < 0)   /* take part in +quad */
	{
		w = w + x;
		x = 0;
		if (w <= 0)
			return NULL;
	}
	if (y < 0)  /* take part in +quad */
	{
		h = h + y;
		y = 0;
		if (h <= 0)
			return NULL;
	}

	box = (BOX *)calloc(1, sizeof(BOX));
	boxSetGeometry(box, x, y, w, h);
	box->refcount = 1;
	return box;
}

void boxDestroy(BOX  **pbox)
{
	BOX  *box;

	if (pbox == NULL)
	{
		return;
	}
	if ((box = *pbox) == NULL)
		return;

	box->refcount--;
	if (box->refcount <= 0)
		free(box);
	*pbox = NULL;
}

l_ok boxGetGeometry(BOX *box, l_int32 *px, l_int32 *py, l_int32 *pw, l_int32 *ph)
{
	if (px) *px = 0;
	if (py) *py = 0;
	if (pw) *pw = 0;
	if (ph) *ph = 0;
	if (!box)
		return 1;

	if (px) *px = box->x;
	if (py) *py = box->y;
	if (pw) *pw = box->w;
	if (ph) *ph = box->h;
	return 0;
}

l_ok boxSetGeometry(BOX *box, l_int32 x, l_int32 y, l_int32 w, l_int32 h)
{
	if (!box)
		return 1;

	if (x != -1) box->x = x;
	if (y != -1) box->y = y;
	if (w != -1) box->w = w;
	if (h != -1) box->h = h;
	return 0;
}

BOX* boxCopy(BOX  *box)
{
	BOX  *boxc;

	if (!box)
		return NULL;

	boxc = boxCreate(box->x, box->y, box->w, box->h);
	return boxc;
}

BOX* boxClipToRectangle(BOX* box, l_int32 wi, l_int32 hi)
{
	BOX  *boxd;

	if (!box)
		return NULL;
	if (box->x >= wi || box->y >= hi ||
		box->x + box->w <= 0 || box->y + box->h <= 0)
		return NULL;

	boxd = boxCopy(box);
	if (boxd->x < 0)
	{
		boxd->w += boxd->x;
		boxd->x = 0;
	}
	if (boxd->y < 0)
	{
		boxd->h += boxd->y;
		boxd->y = 0;
	}
	if (boxd->x + boxd->w > wi)
		boxd->w = wi - boxd->x;
	if (boxd->y + boxd->h > hi)
		boxd->h = hi - boxd->y;
	return boxd;
}

//Pix* grayscalePix(Pix* src)
//{
//	int bytesPerPixel = 4;
//	Pix* dst = new Pix();
//
//	memset(dst, 0, sizeof(PIX));
//	dst->w = src->w;
//	dst->h = src->h;
//	dst->d = 8;
//	dst->spp = 1;
//	dst->wpl = src->wpl / bytesPerPixel;
//	dst->refcount = 1;
//	dst->informat = IFF_BMP;
//	dst->xres = src->xres;
//	dst->yres = src->yres;
//	dst->data = (l_uint32*)malloc(dst->h * dst->w);
//	for (int yy = 0; yy < dst->h; yy++)
//	{
//		for (int xx = 0; xx < dst->w; xx++)
//		{
//			RGBA_Quad rgba = ((RGBA_Quad*)src->data)[xx + yy * src->w];
//			l_uint8 color = ((int)rgba.red + rgba.green + rgba.reserved) / 3;
//			((l_uint8*)dst->data)[xx + yy * dst->w] = color;
//		}
//	}
//
//	return dst;
//}
