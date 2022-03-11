#pragma once
#include <windows.h>

/*--------------------------------------------------------------------*
*                          Built-in types                            *
*--------------------------------------------------------------------*/
typedef int                     l_ok;    /*!< return type 0 if OK, 1 on error */
typedef signed char             l_int8;
typedef unsigned char           l_uint8;
typedef short                   l_int16;
typedef unsigned short          l_uint16;
typedef int                     l_int32;
typedef unsigned int            l_uint32;
typedef float                   l_float32;
typedef double                  l_float64;
#ifdef COMPILER_MSVC
typedef __int64                 l_int64;    /*!< signed 64-bit value */
typedef unsigned __int64        l_uint64;   /*!< unsigned 64-bit value */
#else
typedef long long               l_int64;    /*!< signed 64-bit value */
typedef unsigned long long      l_uint64;   /*!< unsigned 64-bit value */
#endif  /* COMPILER_MSVC */

/*! Image Formats */
enum {
	IFF_UNKNOWN = 0,
	IFF_BMP = 1,
};

/*! Header Ids */
enum {
	BMP_ID = 0x4d42,     /*!< BM - for bitmaps    */
};

/*! RGBA Color */
enum {
	COLOR_RED = 0,        /*!< red color index in RGBA_QUAD    */
	COLOR_GREEN = 1,      /*!< green color index in RGBA_QUAD  */
	COLOR_BLUE = 2,       /*!< blue color index in RGBA_QUAD   */
	L_ALPHA_CHANNEL = 3   /*!< alpha value index in RGBA_QUAD  */
};

/* Image dimension limits */
static const l_int32  L_MAX_ALLOWED_WIDTH = 1000000;
static const l_int32  L_MAX_ALLOWED_HEIGHT = 1000000;
static const l_int64  L_MAX_ALLOWED_PIXELS = 400000000LL;
static const l_int32  L_MAX_ALLOWED_RES = 10000000;  /* pixels/meter */

#define   PIX_SRC      (0xc)                      /*!< use source pixels      */
#define   PIX_DST      (0xa)                      /*!< use destination pixels */
#define   PIX_NOT(op)  ((op) ^ 0x0f)              /*!< invert operation %op   */
#define   PIX_CLR      (0x0)                      /*!< clear pixels           */
#define   PIX_SET      (0xf)                      /*!< set pixels             */


/*-------------------------------------------------------------------------*
*                              Basic Pix                                  *
*-------------------------------------------------------------------------*/
struct Pix
{
	l_uint32             w;           /* width in pixels                   */
	l_uint32             h;           /* height in pixels                  */
	l_uint32             d;           /* depth in bits                     */
	l_uint32             spp;         /* number of samples per pixel       */
	l_uint32             wpl;         /* 32-bit words/line                 */
	l_uint32             refcount;    /* reference count (1 if no clones)  */
	l_int32              xres;        /* image res (ppi) in x direction    */
									  /* (use 0 if unknown)                */
	l_int32              yres;        /* image res (ppi) in y direction    */
									  /* (use 0 if unknown)                */
	l_int32              informat;    /* input file format, IFF_*          */
	l_uint32             special;     /* special instructions for I/O, etc */
	char                *text;        /* text string associated with pix   */
	struct PixColormap  *colormap;    /* colormap (may be null)            */
	l_uint32            *data;        /* the image data                    */
};
typedef struct Pix PIX;


struct PixColormap
{
	void            *array;     /* colormap table (array of RGBA_QUAD)     */
	l_int32          depth;     /* of pix (1, 2, 4 or 8 bpp)               */
	l_int32          nalloc;    /* number of color entries allocated       */
	l_int32          n;         /* number of color entries used            */
};
typedef struct PixColormap  PIXCMAP;


/* Colormap table entry (after the BMP version).
* Note that the BMP format stores the colormap table exactly
* as it appears here, with color samples being stored sequentially,
* in the order (b,g,r,a). */
struct RGBA_Quad
{
	l_uint8     blue;
	l_uint8     green;
	l_uint8     red;
	l_uint8     reserved;
};
typedef struct RGBA_Quad  RGBA_QUAD;

/*-------------------------------------------------------------------------*
*                    Basic rectangle and rectangle arrays                 *
*-------------------------------------------------------------------------*/
/*! Basic rectangle */
struct Box
{
	l_int32            x;           /*!< left coordinate                   */
	l_int32            y;           /*!< top coordinate                    */
	l_int32            w;           /*!< box width                         */
	l_int32            h;           /*!< box height                        */
	l_uint32           refcount;    /*!< reference count (1 if no clones)  */
};
typedef struct Box    BOX;

/*! Array of Box */
struct Boxa
{
	l_int32            n;           /*!< number of box in ptr array        */
	l_int32            nalloc;      /*!< number of box ptrs allocated      */
	l_uint32           refcount;    /*!< reference count (1 if no clones)  */
	struct Box       **box;         /*!< box ptr array                     */
};
typedef struct Boxa  BOXA;

/*! Array of Boxa */
struct Boxaa
{
	l_int32            n;           /*!< number of boxa in ptr array       */
	l_int32            nalloc;      /*!< number of boxa ptrs allocated     */
	struct Boxa      **boxa;        /*!< boxa ptr array                    */
};
typedef struct Boxaa  BOXAA;


PIX* pixCreate(l_int32 width, l_int32 height, l_int32 depth);
void pixDestroy(PIX **ppix);
PIX* pixRead(LPCSTR fileName);
PIX* pixRead(LPCWSTR fileName);
int pixWrite(PIX* pixs, l_int32 format, l_uint8** lpBuff);
l_ok pixWrite(const char *fname, PIX* pixs, l_int32 format);
PIX* pixClipRectangle(const PIX* pixs, int x, int y, int w, int h);
PIX* pixClipRectangle(const PIX* pixs, BOX* box, BOX** pboxc);
l_ok pixEndianByteSwap(PIX  *pixs);
