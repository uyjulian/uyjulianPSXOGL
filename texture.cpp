/***************************************************************************
                          texture.c  -  description
                             -------------------
    begin                : Sun Mar 08 2009
    copyright            : (C) 1999-2009 by Pete Bernert
    web                  : www.pbernert.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

////////////////////////////////////////////////////////////////////////////////////
// Texture related functions are here !
//
// The texture handling is heart and soul of this gpu. The plugin was developed
// 1999, by this time no shaders were available. Since the psx gpu is making
// heavy use of CLUT (="color lookup tables", aka palettized textures), it was
// an interesting task to get those emulated at good speed on NV TNT cards
// (which was my major goal when I created the first "gpuPeteTNT"). Later cards
// (Geforce256) supported texture palettes by an OGL extension, but at some point
// this support was dropped again by gfx card vendors.
// Well, at least there is a certain advatage, if no texture palettes extension can
// be used: it is possible to modify the textures in any way, allowing "hi-res"
// textures and other tweaks.
//
// My main texture caching is kinda complex: the plugin is allocating "n" 256x256 textures,
// and it places small psx texture parts inside them. The plugin keeps track what
// part (with what palette) it had placed in which texture, so it can re-use this
// part again. The more ogl textures it can use, the better (of course the managing/
// searching will be slower, but everything is faster than uploading textures again
// and again to a gfx card). My first card (TNT1) had 16 MB Vram, and it worked
// well with many games, but I recommend nowadays 64 MB Vram to get a good speed.
//
// Sadly, there is also a second kind of texture cache needed, for "psx texture windows".
// Those are "repeated" textures, so a psx "texture window" needs to be put in
// a whole texture to use the GL_TEXTURE_WRAP_ features. This cache can get full very
// fast in games which are having an heavy "texture window" usage, like RRT4. As an
// alternative, this plugin can use the OGL "palette" extension on texture windows,
// if available. Nowadays also a fragment shader can easily be used to emulate
// texture wrapping in a texture atlas, so the main cache could hold the texture
// windows as well (that's what I am doing in the OGL2 plugin). But currently the
// OGL1 plugin is a "shader-free" zone, so heavy "texture window" games will cause
// much texture uploads.
//
// Some final advice: take care if you change things in here. I've removed my ASM
// handlers (they didn't cause much speed gain anyway) for readability/portability,
// but still the functions/data structures used here are easy to mess up. I guess it
// can be a pain in the ass to port the plugin to another byte order :)
//
////////////////////////////////////////////////////////////////////////////////////

#include "externals.h"

////////////////////////////////////////////////////////////////////////
// texture conversion buffer ..
////////////////////////////////////////////////////////////////////////

static GLubyte ubPaletteBuffer[256][4];
static GLuint gTexMovieName = 0;
static GLuint gTexBlurName = 0;
GLuint gTexFrameName = 0;
static int iTexGarbageCollection = 1;
static uint32_t dwTexPageComp = 0;
static int iClampType = GL_CLAMP_TO_EDGE;

void (*LoadSubTexFn)(int, int, int16_t, int16_t);

////////////////////////////////////////////////////////////////////////

uint8_t *CheckTextureInSubSCache(int TextureMode, uint32_t GivenClutId, uint16_t *pCache);
void LoadSubTexturePageSort(int pageid, int mode, int16_t cx, int16_t cy);
void LoadPackedSubTexturePageSort(int pageid, int mode, int16_t cx, int16_t cy);
void DefineSubTextureSort(void);

////////////////////////////////////////////////////////////////////////
// some globals
////////////////////////////////////////////////////////////////////////

static GLint giWantedRGBA = 4;
static GLenum giWantedFMT = GL_RGBA;
static GLenum giWantedTYPE = GL_UNSIGNED_BYTE;
int GlobalTexturePage;
static GLint XTexS;
static GLint YTexS;
static GLint DXTexS;
static GLint DYTexS;
static int iSortTexCnt = 32;
static int iFrameTexType = 1;
int iFrameReadType = 0;

uint32_t (*TCF[2])(uint32_t);

////////////////////////////////////////////////////////////////////////
// texture cache implementation
////////////////////////////////////////////////////////////////////////

// "texture window" cache entry

typedef struct textureWndCacheEntryTag
{
	uint32_t ClutID;
	int16_t pageid;
	int16_t textureMode;
	int16_t Opaque;
	int16_t used;
	EXLong pos;
	GLuint texname;
} textureWndCacheEntry;

// "standard texture" cache entry (12 byte per entry, as small as possible... we need lots of them)

typedef struct textureSubCacheEntryTagS
{
	uint32_t ClutID;
	EXLong pos;
	uint8_t posTX;
	uint8_t posTY;
	uint8_t cTexID;
	uint8_t Opaque;
} textureSubCacheEntryS;

//---------------------------------------------

//---------------------------------------------

static textureWndCacheEntry wcWndtexStore[MAXWNDTEXCACHE];
static textureSubCacheEntryS *pscSubtexStore[3][MAXTPAGES_MAX];
static EXLong *pxSsubtexLeft[MAXSORTTEX_MAX];
static GLuint uiStexturePage[MAXSORTTEX_MAX];

static uint16_t usLRUTexPage = 0;

static int iMaxTexWnds = 0;
static int iTexWndTurn = 0;
static int iTexWndLimit = MAXWNDTEXCACHE / 2;

static GLubyte *texturepart = NULL;
static GLubyte *texturebuffer = NULL;
static uint32_t g_x1, g_y1, g_x2, g_y2;
uint8_t ubOpaqueDraw = 0;

static uint16_t MAXTPAGES = 32;
static uint16_t CLUTMASK = 0x7fff;
static uint16_t CLUTYMASK = 0x1ff;
static uint16_t MAXSORTTEX = 196;

////////////////////////////////////////////////////////////////////////
// Texture color conversions
////////////////////////////////////////////////////////////////////////

uint32_t XP8RGBA_0(uint32_t BGR)
{
	if (!(BGR & 0xffff))
		return 0x50000000;
	return ((((BGR << 3) & 0xf8) | ((BGR << 6) & 0xf800) | ((BGR << 9) & 0xf80000)) & 0xffffff) | 0xff000000;
}

uint32_t XP8RGBA_1(uint32_t BGR)
{
	if (!(BGR & 0xffff))
		return 0x50000000;
	if (!(BGR & 0x8000))
	{
		ubOpaqueDraw = 1;
		return ((((BGR << 3) & 0xf8) | ((BGR << 6) & 0xf800) | ((BGR << 9) & 0xf80000)) & 0xffffff);
	}
	return ((((BGR << 3) & 0xf8) | ((BGR << 6) & 0xf800) | ((BGR << 9) & 0xf80000)) & 0xffffff) | 0xff000000;
}

////////////////////////////////////////////////////////////////////////
// CHECK TEXTURE MEM (on plugin startup)
////////////////////////////////////////////////////////////////////////

void CheckTextureMemory(void)
{
	GLboolean b;
	GLboolean *bDetail;
	int i, iCnt;
	int iTSize;
	char *p;

	iTSize = 256;
	p = (char *)malloc(iTSize * iTSize * 4);

	iCnt = 0;
	glGenTextures(MAXSORTTEX, uiStexturePage);
	for (i = 0; i < MAXSORTTEX; i++)
	{
		glBindTexture(GL_TEXTURE_2D, uiStexturePage[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, iClampType);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, iClampType);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, giWantedRGBA, iTSize, iTSize, 0, GL_RGBA, giWantedTYPE, p);
	}
	glBindTexture(GL_TEXTURE_2D, 0);

	free(p);

	bDetail = (GLboolean *)malloc(MAXSORTTEX * sizeof(GLboolean));
	memset(bDetail, 0, MAXSORTTEX * sizeof(GLboolean));
	b = glAreTexturesResident(MAXSORTTEX, uiStexturePage, bDetail);

	glDeleteTextures(MAXSORTTEX, uiStexturePage);

	for (i = 0; i < MAXSORTTEX; i++)
	{
		if (bDetail[i])
			iCnt++;
		uiStexturePage[i] = 0;
	}

	free(bDetail);

	if (b)
		iSortTexCnt = MAXSORTTEX - min(1, 0);
	else
		iSortTexCnt = iCnt - 3 + min(1, 0); // place for menu&texwnd

	if (iSortTexCnt < 8)
		iSortTexCnt = 8;
}

////////////////////////////////////////////////////////////////////////
// Main init of textures
////////////////////////////////////////////////////////////////////////

void InitializeTextureStore()
{
	int i, j;

	if (iGPUHeight == 1024)
	{
		MAXTPAGES = 64;
		CLUTMASK = 0xffff;
		CLUTYMASK = 0x3ff;
		MAXSORTTEX = 128;
		iTexGarbageCollection = 0;
	}
	else
	{
		MAXTPAGES = 32;
		CLUTMASK = 0x7fff;
		CLUTYMASK = 0x1ff;
		MAXSORTTEX = 196;
	}

	memset(vertex, 0, 4 * sizeof(OGLVertex)); // init vertices

	gTexName = 0; // init main tex name

	iTexWndLimit = MAXWNDTEXCACHE;
	iTexWndLimit /= 2;

	memset(wcWndtexStore, 0, sizeof(textureWndCacheEntry) * MAXWNDTEXCACHE);
	texturepart = (GLubyte *)malloc(256 * 256 * 4);
	memset(texturepart, 0, 256 * 256 * 4);
	texturebuffer = NULL;

	for (i = 0; i < 3; i++) // -> info for 32*3
		for (j = 0; j < MAXTPAGES; j++)
		{
			pscSubtexStore[i][j] = (textureSubCacheEntryS *)malloc(CSUBSIZES * sizeof(textureSubCacheEntryS));
			memset(pscSubtexStore[i][j], 0, CSUBSIZES * sizeof(textureSubCacheEntryS));
		}
	for (i = 0; i < MAXSORTTEX; i++) // -> info 0..511
	{
		pxSsubtexLeft[i] = (EXLong *)malloc(CSUBSIZE * sizeof(EXLong));
		memset(pxSsubtexLeft[i], 0, CSUBSIZE * sizeof(EXLong));
		uiStexturePage[i] = 0;
	}
}

////////////////////////////////////////////////////////////////////////
// Clean up on exit
////////////////////////////////////////////////////////////////////////

void CleanupTextureStore()
{
	int i, j;
	textureWndCacheEntry *tsx;
	//----------------------------------------------------//
	glBindTexture(GL_TEXTURE_2D, 0);
	//----------------------------------------------------//
	free(texturepart); // free tex part
	texturepart = 0;
	if (texturebuffer)
	{
		free(texturebuffer);
		texturebuffer = 0;
	}
	//----------------------------------------------------//
	tsx = wcWndtexStore; // loop tex window cache
	for (i = 0; i < MAXWNDTEXCACHE; i++, tsx++)
	{
		if (tsx->texname)                       // -> some tex?
			glDeleteTextures(1, &tsx->texname); // --> delete it
	}
	iMaxTexWnds = 0; // no more tex wnds
	//----------------------------------------------------//
	if (gTexMovieName != 0)                  // some movie tex?
		glDeleteTextures(1, &gTexMovieName); // -> delete it
	gTexMovieName = 0;                       // no more movie tex
	//----------------------------------------------------//
	if (gTexFrameName != 0)                  // some 15bit framebuffer tex?
		glDeleteTextures(1, &gTexFrameName); // -> delete it
	gTexFrameName = 0;                       // no more movie tex
	//----------------------------------------------------//
	if (gTexBlurName != 0)                  // some 15bit framebuffer tex?
		glDeleteTextures(1, &gTexBlurName); // -> delete it
	gTexBlurName = 0;                       // no more movie tex
	//----------------------------------------------------//
	for (i = 0; i < 3; i++)             // -> loop
		for (j = 0; j < MAXTPAGES; j++) // loop tex pages
		{
			free(pscSubtexStore[i][j]); // -> clean mem
		}
	for (i = 0; i < MAXSORTTEX; i++)
	{
		if (uiStexturePage[i]) // --> tex used ?
		{
			glDeleteTextures(1, &uiStexturePage[i]);
			uiStexturePage[i] = 0; // --> delete it
		}
		free(pxSsubtexLeft[i]); // -> clean mem
	}
	//----------------------------------------------------//
}

////////////////////////////////////////////////////////////////////////
// Reset textures in game...
////////////////////////////////////////////////////////////////////////

void ResetTextureArea(bool bDelTex)
{
	int i, j;
	textureSubCacheEntryS *tss;
	EXLong *lu;
	textureWndCacheEntry *tsx;
	//----------------------------------------------------//

	dwTexPageComp = 0;

	//----------------------------------------------------//
	if (bDelTex)
	{
		glBindTexture(GL_TEXTURE_2D, 0);
		gTexName = 0;
	}
	//----------------------------------------------------//
	tsx = wcWndtexStore;
	for (i = 0; i < MAXWNDTEXCACHE; i++, tsx++)
	{
		tsx->used = 0;
		if (bDelTex && tsx->texname)
		{
			glDeleteTextures(1, &tsx->texname);
			tsx->texname = 0;
		}
	}
	iMaxTexWnds = 0;
	//----------------------------------------------------//

	for (i = 0; i < 3; i++)
		for (j = 0; j < MAXTPAGES; j++)
		{
			tss = pscSubtexStore[i][j];
			(tss + SOFFA)->pos.l = 0;
			(tss + SOFFB)->pos.l = 0;
			(tss + SOFFC)->pos.l = 0;
			(tss + SOFFD)->pos.l = 0;
		}

	for (i = 0; i < iSortTexCnt; i++)
	{
		lu = pxSsubtexLeft[i];
		lu->l = 0;
		if (bDelTex && uiStexturePage[i])
		{
			glDeleteTextures(1, &uiStexturePage[i]);
			uiStexturePage[i] = 0;
		}
	}
}

////////////////////////////////////////////////////////////////////////
// Invalidate tex windows
////////////////////////////////////////////////////////////////////////

static void InvalidateWndTextureArea(int X, int Y, int W, int H)
{
	int i, px1, px2, py1, py2, iYM = 1;
	textureWndCacheEntry *tsw = wcWndtexStore;

	W += X - 1;
	H += Y - 1;
	if (X < 0)
		X = 0;
	if (X > 1023)
		X = 1023;
	if (W < 0)
		W = 0;
	if (W > 1023)
		W = 1023;
	if (Y < 0)
		Y = 0;
	if (Y > iGPUHeightMask)
		Y = iGPUHeightMask;
	if (H < 0)
		H = 0;
	if (H > iGPUHeightMask)
		H = iGPUHeightMask;
	W++;
	H++;

	if (iGPUHeight == 1024)
		iYM = 3;

	py1 = min(iYM, Y >> 8);
	py2 = min(iYM, H >> 8); // y: 0 or 1

	px1 = max(0, (X >> 6));
	px2 = min(15, (W >> 6));

	if (py1 == py2)
	{
		py1 = py1 << 4;
		px1 += py1;
		px2 += py1; // change to 0-31
		for (i = 0; i < iMaxTexWnds; i++, tsw++)
		{
			if (tsw->used)
			{
				if (tsw->pageid >= px1 && tsw->pageid <= px2)
				{
					tsw->used = 0;
				}
			}
		}
	}
	else
	{
		py1 = px1 + 16;
		py2 = px2 + 16;
		for (i = 0; i < iMaxTexWnds; i++, tsw++)
		{
			if (tsw->used)
			{
				if ((tsw->pageid >= px1 && tsw->pageid <= px2) || (tsw->pageid >= py1 && tsw->pageid <= py2))
				{
					tsw->used = 0;
				}
			}
		}
	}

	// adjust tex window count
	tsw = wcWndtexStore + iMaxTexWnds - 1;
	while (iMaxTexWnds && !tsw->used)
	{
		iMaxTexWnds--;
		tsw--;
	}
}

////////////////////////////////////////////////////////////////////////
// same for sort textures
////////////////////////////////////////////////////////////////////////

static void MarkFree(textureSubCacheEntryS *tsx)
{
	EXLong *ul, *uls;
	int j, iMax;
	uint8_t x1, y1, dx, dy;

	uls = pxSsubtexLeft[tsx->cTexID];
	iMax = uls->l;
	ul = uls + 1;

	if (!iMax)
		return;

	for (j = 0; j < iMax; j++, ul++)
		if (ul->l == 0xffffffff)
			break;

	if (j < CSUBSIZE - 2)
	{
		if (j == iMax)
			uls->l = uls->l + 1;

		x1 = tsx->posTX;
		dx = tsx->pos.c[2] - tsx->pos.c[3];
		if (tsx->posTX)
		{
			x1--;
			dx += 3;
		}
		y1 = tsx->posTY;
		dy = tsx->pos.c[0] - tsx->pos.c[1];
		if (tsx->posTY)
		{
			y1--;
			dy += 3;
		}

		ul->c[3] = x1;
		ul->c[2] = dx;
		ul->c[1] = y1;
		ul->c[0] = dy;
	}
}

static void InvalidateSubSTextureArea(int X, int Y, int W, int H)
{
	int i, j, k, iMax, px, py, px1, px2, py1, py2, iYM = 1;
	EXLong npos;
	textureSubCacheEntryS *tsb;
	int x1, x2, y1, y2, xa, sw;

	W += X - 1;
	H += Y - 1;
	if (X < 0)
		X = 0;
	if (X > 1023)
		X = 1023;
	if (W < 0)
		W = 0;
	if (W > 1023)
		W = 1023;
	if (Y < 0)
		Y = 0;
	if (Y > iGPUHeightMask)
		Y = iGPUHeightMask;
	if (H < 0)
		H = 0;
	if (H > iGPUHeightMask)
		H = iGPUHeightMask;
	W++;
	H++;

	if (iGPUHeight == 1024)
		iYM = 3;

	py1 = min(iYM, Y >> 8);
	py2 = min(iYM, H >> 8); // y: 0 or 1
	px1 = max(0, (X >> 6) - 3);
	px2 = min(15, (W >> 6) + 3); // x: 0-15

	for (py = py1; py <= py2; py++)
	{
		j = (py << 4) + px1; // get page

		y1 = py * 256;
		y2 = y1 + 255;

		if (H < y1)
			continue;
		if (Y > y2)
			continue;

		if (Y > y1)
			y1 = Y;
		if (H < y2)
			y2 = H;
		if (y2 < y1)
		{
			sw = y1;
			y1 = y2;
			y2 = sw;
		}
		y1 = ((y1 % 256) << 8);
		y2 = (y2 % 256);

		for (px = px1; px <= px2; px++, j++)
		{
			for (k = 0; k < 3; k++)
			{
				xa = x1 = px << 6;
				if (W < x1)
					continue;
				x2 = x1 + (64 << k) - 1;
				if (X > x2)
					continue;

				if (X > x1)
					x1 = X;
				if (W < x2)
					x2 = W;
				if (x2 < x1)
				{
					sw = x1;
					x1 = x2;
					x2 = sw;
				}

				if (dwGPUVersion == 2)
					npos.l = 0x00ff00ff;
				else
					npos.l = ((x1 - xa) << (26 - k)) | ((x2 - xa) << (18 - k)) | y1 | y2;

				{
					tsb = pscSubtexStore[k][j] + SOFFA;
					iMax = tsb->pos.l;
					tsb++;
					for (i = 0; i < iMax; i++, tsb++)
						if (tsb->ClutID && XCHECK(tsb->pos, npos))
						{
							tsb->ClutID = 0;
							MarkFree(tsb);
						}

					//         if(npos.l & 0x00800000)
					{
						tsb = pscSubtexStore[k][j] + SOFFB;
						iMax = tsb->pos.l;
						tsb++;
						for (i = 0; i < iMax; i++, tsb++)
							if (tsb->ClutID && XCHECK(tsb->pos, npos))
							{
								tsb->ClutID = 0;
								MarkFree(tsb);
							}
					}

					//         if(npos.l & 0x00000080)
					{
						tsb = pscSubtexStore[k][j] + SOFFC;
						iMax = tsb->pos.l;
						tsb++;
						for (i = 0; i < iMax; i++, tsb++)
							if (tsb->ClutID && XCHECK(tsb->pos, npos))
							{
								tsb->ClutID = 0;
								MarkFree(tsb);
							}
					}

					//         if(npos.l & 0x00800080)
					{
						tsb = pscSubtexStore[k][j] + SOFFD;
						iMax = tsb->pos.l;
						tsb++;
						for (i = 0; i < iMax; i++, tsb++)
							if (tsb->ClutID && XCHECK(tsb->pos, npos))
							{
								tsb->ClutID = 0;
								MarkFree(tsb);
							}
					}
				}
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////
// Invalidate some parts of cache: main routine
////////////////////////////////////////////////////////////////////////

void InvalidateTextureAreaEx(void)
{
	int16_t W = sxmax - sxmin;
	int16_t H = symax - symin;

	if (W == 0 && H == 0)
		return;

	if (iMaxTexWnds)
		InvalidateWndTextureArea(sxmin, symin, W, H);

	InvalidateSubSTextureArea(sxmin, symin, W, H);
}

////////////////////////////////////////////////////////////////////////

void InvalidateTextureArea(int X, int Y, int W, int H)
{
	if (W == 0 && H == 0)
		return;

	if (iMaxTexWnds)
		InvalidateWndTextureArea(X, Y, W, H);

	InvalidateSubSTextureArea(X, Y, W, H);
}

////////////////////////////////////////////////////////////////////////
// tex window: define
////////////////////////////////////////////////////////////////////////

static void DefineTextureWnd(void)
{
	if (gTexName == 0)
		glGenTextures(1, &gTexName);

	glBindTexture(GL_TEXTURE_2D, gTexName);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, giWantedRGBA, TWin.Position.x1, TWin.Position.y1, 0, giWantedFMT, giWantedTYPE,
	             texturepart);
}

////////////////////////////////////////////////////////////////////////
// tex window: load stretched
////////////////////////////////////////////////////////////////////////

static void LoadStretchWndTexturePage(int pageid, int mode, int16_t cx, int16_t cy)
{
	uint32_t start, row, column, j, sxh, sxm, ldx, ldy, ldxo, s;
	uint32_t palstart;
	uint32_t *px, *pa, *ta;
	uint8_t *cSRCPtr, *cOSRCPtr;
	uint16_t *wSRCPtr, *wOSRCPtr;
	uint32_t LineOffset;
	int pmult = pageid / 16;
	uint32_t (*LTCOL)(uint32_t);

	LTCOL = TCF[DrawSemiTrans];

	ldxo = TWin.Position.x1 - TWin.OPosition.x1;
	ldy = TWin.Position.y1 - TWin.OPosition.y1;

	pa = px = (uint32_t *)ubPaletteBuffer;
	ta = (uint32_t *)texturepart;
	palstart = cx + (cy * 1024);

	ubOpaqueDraw = 0;

	switch (mode)
	{
	//--------------------------------------------------//
	// 4bit texture load ..
	case 0:
		//------------------- ZN STUFF

		if (GlobalTextIL)
		{
			uint32_t TXV, TXU, n_xi, n_yi;

			wSRCPtr = psxVuw + palstart;

			row = 4;
			do
			{
				*px = LTCOL(*wSRCPtr);
				*(px + 1) = LTCOL(*(wSRCPtr + 1));
				*(px + 2) = LTCOL(*(wSRCPtr + 2));
				*(px + 3) = LTCOL(*(wSRCPtr + 3));
				row--;
				px += 4;
				wSRCPtr += 4;
			} while (row);

			column = g_y2 - ldy;
			for (TXV = g_y1; TXV <= column; TXV++)
			{
				ldx = ldxo;
				for (TXU = g_x1; TXU <= g_x2 - ldxo; TXU++)
				{
					n_xi = ((TXU >> 2) & ~0x3c) + ((TXV << 2) & 0x3c);
					n_yi = (TXV & ~0xf) + ((TXU >> 4) & 0xf);

					s = *(pa + ((*(psxVuw + ((GlobalTextAddrY + n_yi) * 1024) + GlobalTextAddrX + n_xi) >>
					             ((TXU & 0x03) << 2)) &
					            0x0f));
					*ta++ = s;

					if (ldx)
					{
						*ta++ = s;
						ldx--;
					}
				}

				if (ldy)
				{
					ldy--;
					for (TXU = g_x1; TXU <= g_x2; TXU++)
						*ta++ = *(ta - (g_x2 - g_x1));
				}
			}

			DefineTextureWnd();

			break;
		}

		//-------------------

		start = ((pageid - 16 * pmult) * 128) + 256 * 2048 * pmult;
		// convert CLUT to 32bits .. and then use THAT as a lookup table

		wSRCPtr = psxVuw + palstart;
		for (row = 0; row < 16; row++)
			*px++ = LTCOL(*wSRCPtr++);

		sxm = g_x1 & 1;
		sxh = g_x1 >> 1;
		if (sxm)
			j = g_x1 + 1;
		else
			j = g_x1;
		cSRCPtr = psxVub + start + (2048 * g_y1) + sxh;
		for (column = g_y1; column <= g_y2; column++)
		{
			cOSRCPtr = cSRCPtr;
			ldx = ldxo;
			if (sxm)
				*ta++ = *(pa + ((*cSRCPtr++ >> 4) & 0xF));

			for (row = j; row <= g_x2 - ldxo; row++)
			{
				s = *(pa + (*cSRCPtr & 0xF));
				*ta++ = s;
				if (ldx)
				{
					*ta++ = s;
					ldx--;
				}
				row++;
				if (row <= g_x2 - ldxo)
				{
					s = *(pa + ((*cSRCPtr >> 4) & 0xF));
					*ta++ = s;
					if (ldx)
					{
						*ta++ = s;
						ldx--;
					}
				}
				cSRCPtr++;
			}
			if (ldy && column & 1)
			{
				ldy--;
				cSRCPtr = cOSRCPtr;
			}
			else
				cSRCPtr = psxVub + start + (2048 * (column + 1)) + sxh;
		}

		DefineTextureWnd();
		break;
	//--------------------------------------------------//
	// 8bit texture load ..
	case 1:
		//------------ ZN STUFF
		if (GlobalTextIL)
		{
			uint32_t TXV, TXU, n_xi, n_yi;

			wSRCPtr = psxVuw + palstart;

			row = 64;
			do
			{
				*px = LTCOL(*wSRCPtr);
				*(px + 1) = LTCOL(*(wSRCPtr + 1));
				*(px + 2) = LTCOL(*(wSRCPtr + 2));
				*(px + 3) = LTCOL(*(wSRCPtr + 3));
				row--;
				px += 4;
				wSRCPtr += 4;
			} while (row);

			column = g_y2 - ldy;
			for (TXV = g_y1; TXV <= column; TXV++)
			{
				ldx = ldxo;
				for (TXU = g_x1; TXU <= g_x2 - ldxo; TXU++)
				{
					n_xi = ((TXU >> 1) & ~0x78) + ((TXU << 2) & 0x40) + ((TXV << 3) & 0x38);
					n_yi = (TXV & ~0x7) + ((TXU >> 5) & 0x7);

					s = *(pa + ((*(psxVuw + ((GlobalTextAddrY + n_yi) * 1024) + GlobalTextAddrX + n_xi) >>
					             ((TXU & 0x01) << 3)) &
					            0xff));
					*ta++ = s;
					if (ldx)
					{
						*ta++ = s;
						ldx--;
					}
				}

				if (ldy)
				{
					ldy--;
					for (TXU = g_x1; TXU <= g_x2; TXU++)
						*ta++ = *(ta - (g_x2 - g_x1));
				}
			}

			DefineTextureWnd();

			break;
		}
		//------------

		start = ((pageid - 16 * pmult) * 128) + 256 * 2048 * pmult;

		// not using a lookup table here... speeds up smaller texture areas
		cSRCPtr = psxVub + start + (2048 * g_y1) + g_x1;
		LineOffset = 2048 - (g_x2 - g_x1 + 1) + ldxo;

		for (column = g_y1; column <= g_y2; column++)
		{
			cOSRCPtr = cSRCPtr;
			ldx = ldxo;
			for (row = g_x1; row <= g_x2 - ldxo; row++)
			{
				s = LTCOL(psxVuw[palstart + *cSRCPtr++]);
				*ta++ = s;
				if (ldx)
				{
					*ta++ = s;
					ldx--;
				}
			}
			if (ldy && column & 1)
			{
				ldy--;
				cSRCPtr = cOSRCPtr;
			}
			else
				cSRCPtr += LineOffset;
		}

		DefineTextureWnd();
		break;
	//--------------------------------------------------//
	// 16bit texture load ..
	case 2:
		start = ((pageid - 16 * pmult) * 64) + 256 * 1024 * pmult;

		wSRCPtr = psxVuw + start + (1024 * g_y1) + g_x1;
		LineOffset = 1024 - (g_x2 - g_x1 + 1) + ldxo;

		for (column = g_y1; column <= g_y2; column++)
		{
			wOSRCPtr = wSRCPtr;
			ldx = ldxo;
			for (row = g_x1; row <= g_x2 - ldxo; row++)
			{
				s = LTCOL(*wSRCPtr++);
				*ta++ = s;
				if (ldx)
				{
					*ta++ = s;
					ldx--;
				}
			}
			if (ldy && column & 1)
			{
				ldy--;
				wSRCPtr = wOSRCPtr;
			}
			else
				wSRCPtr += LineOffset;
		}

		DefineTextureWnd();
		break;
		//--------------------------------------------------//
		// others are not possible !
	}
}

////////////////////////////////////////////////////////////////////////
// tex window: load simple
////////////////////////////////////////////////////////////////////////

static void LoadWndTexturePage(int pageid, int mode, int16_t cx, int16_t cy)
{
	uint32_t start, row, column, j, sxh, sxm;
	uint32_t palstart;
	uint32_t *px, *pa, *ta;
	uint8_t *cSRCPtr;
	uint16_t *wSRCPtr;
	uint32_t LineOffset;
	int pmult = pageid / 16;
	uint32_t (*LTCOL)(uint32_t);

	LTCOL = TCF[DrawSemiTrans];

	pa = px = (uint32_t *)ubPaletteBuffer;
	ta = (uint32_t *)texturepart;
	palstart = cx + (cy * 1024);

	ubOpaqueDraw = 0;

	switch (mode)
	{
	//--------------------------------------------------//
	// 4bit texture load ..
	case 0:
		if (GlobalTextIL)
		{
			uint32_t TXV, TXU, n_xi, n_yi;

			wSRCPtr = psxVuw + palstart;

			row = 4;
			do
			{
				*px = LTCOL(*wSRCPtr);
				*(px + 1) = LTCOL(*(wSRCPtr + 1));
				*(px + 2) = LTCOL(*(wSRCPtr + 2));
				*(px + 3) = LTCOL(*(wSRCPtr + 3));
				row--;
				px += 4;
				wSRCPtr += 4;
			} while (row);

			for (TXV = g_y1; TXV <= g_y2; TXV++)
			{
				for (TXU = g_x1; TXU <= g_x2; TXU++)
				{
					n_xi = ((TXU >> 2) & ~0x3c) + ((TXV << 2) & 0x3c);
					n_yi = (TXV & ~0xf) + ((TXU >> 4) & 0xf);

					*ta++ = *(pa + ((*(psxVuw + ((GlobalTextAddrY + n_yi) * 1024) + GlobalTextAddrX + n_xi) >>
					                 ((TXU & 0x03) << 2)) &
					                0x0f));
				}
			}

			DefineTextureWnd();

			break;
		}

		start = ((pageid - 16 * pmult) * 128) + 256 * 2048 * pmult;

		// convert CLUT to 32bits .. and then use THAT as a lookup table

		wSRCPtr = psxVuw + palstart;
		for (row = 0; row < 16; row++)
			*px++ = LTCOL(*wSRCPtr++);

		sxm = g_x1 & 1;
		sxh = g_x1 >> 1;
		if (sxm)
			j = g_x1 + 1;
		else
			j = g_x1;
		cSRCPtr = psxVub + start + (2048 * g_y1) + sxh;
		for (column = g_y1; column <= g_y2; column++)
		{
			cSRCPtr = psxVub + start + (2048 * column) + sxh;

			if (sxm)
				*ta++ = *(pa + ((*cSRCPtr++ >> 4) & 0xF));

			for (row = j; row <= g_x2; row++)
			{
				*ta++ = *(pa + (*cSRCPtr & 0xF));
				row++;
				if (row <= g_x2)
					*ta++ = *(pa + ((*cSRCPtr >> 4) & 0xF));
				cSRCPtr++;
			}
		}

		DefineTextureWnd();
		break;
	//--------------------------------------------------//
	// 8bit texture load ..
	case 1:
		if (GlobalTextIL)
		{
			uint32_t TXV, TXU, n_xi, n_yi;

			wSRCPtr = psxVuw + palstart;

			row = 64;
			do
			{
				*px = LTCOL(*wSRCPtr);
				*(px + 1) = LTCOL(*(wSRCPtr + 1));
				*(px + 2) = LTCOL(*(wSRCPtr + 2));
				*(px + 3) = LTCOL(*(wSRCPtr + 3));
				row--;
				px += 4;
				wSRCPtr += 4;
			} while (row);

			for (TXV = g_y1; TXV <= g_y2; TXV++)
			{
				for (TXU = g_x1; TXU <= g_x2; TXU++)
				{
					n_xi = ((TXU >> 1) & ~0x78) + ((TXU << 2) & 0x40) + ((TXV << 3) & 0x38);
					n_yi = (TXV & ~0x7) + ((TXU >> 5) & 0x7);

					*ta++ = *(pa + ((*(psxVuw + ((GlobalTextAddrY + n_yi) * 1024) + GlobalTextAddrX + n_xi) >>
					                 ((TXU & 0x01) << 3)) &
					                0xff));
				}
			}

			DefineTextureWnd();

			break;
		}

		start = ((pageid - 16 * pmult) * 128) + 256 * 2048 * pmult;

		// not using a lookup table here... speeds up smaller texture areas
		cSRCPtr = psxVub + start + (2048 * g_y1) + g_x1;
		LineOffset = 2048 - (g_x2 - g_x1 + 1);

		for (column = g_y1; column <= g_y2; column++)
		{
			for (row = g_x1; row <= g_x2; row++)
				*ta++ = LTCOL(psxVuw[palstart + *cSRCPtr++]);
			cSRCPtr += LineOffset;
		}

		DefineTextureWnd();
		break;
	//--------------------------------------------------//
	// 16bit texture load ..
	case 2:
		start = ((pageid - 16 * pmult) * 64) + 256 * 1024 * pmult;

		wSRCPtr = psxVuw + start + (1024 * g_y1) + g_x1;
		LineOffset = 1024 - (g_x2 - g_x1 + 1);

		for (column = g_y1; column <= g_y2; column++)
		{
			for (row = g_x1; row <= g_x2; row++)
				*ta++ = LTCOL(*wSRCPtr++);
			wSRCPtr += LineOffset;
		}

		DefineTextureWnd();
		break;
		//--------------------------------------------------//
		// others are not possible !
	}
}

////////////////////////////////////////////////////////////////////////
// tex window: main selecting, cache handler included
////////////////////////////////////////////////////////////////////////

GLuint LoadTextureWnd(int pageid, int TextureMode, uint32_t GivenClutId)
{
	textureWndCacheEntry *ts, *tsx = NULL;
	int i;
	int16_t cx, cy;
	EXLong npos;

	npos.c[3] = TWin.Position.x0;
	npos.c[2] = TWin.OPosition.x1;
	npos.c[1] = TWin.Position.y0;
	npos.c[0] = TWin.OPosition.y1;

	g_x1 = TWin.Position.x0;
	g_x2 = g_x1 + TWin.Position.x1 - 1;
	g_y1 = TWin.Position.y0;
	g_y2 = g_y1 + TWin.Position.y1 - 1;

	if (TextureMode == 2)
	{
		GivenClutId = 0;
		cx = cy = 0;
	}
	else
	{
		cx = ((GivenClutId << 4) & 0x3F0);
		cy = ((GivenClutId >> 6) & CLUTYMASK);
		GivenClutId = (GivenClutId & CLUTMASK) | (DrawSemiTrans << 30);

		// palette check sum
		{
			uint32_t l = 0, row;
			uint32_t *lSRCPtr = (uint32_t *)(psxVuw + cx + (cy * 1024));
			if (TextureMode == 1)
				for (row = 1; row < 129; row++)
					l += ((*lSRCPtr++) - 1) * row;
			else
				for (row = 1; row < 9; row++)
					l += ((*lSRCPtr++) - 1) << row;
			l = (l + HIWORD(l)) & 0x3fffL;
			GivenClutId |= (l << 16);
		}
	}

	ts = wcWndtexStore;

	for (i = 0; i < iMaxTexWnds; i++, ts++)
	{
		if (ts->used)
		{
			if (ts->pos.l == npos.l && ts->pageid == pageid && ts->textureMode == TextureMode)
			{
				if (ts->ClutID == GivenClutId)
				{
					ubOpaqueDraw = ts->Opaque;
					return ts->texname;
				}
			}
		}
		else
			tsx = ts;
	}

	if (!tsx)
	{
		if (iMaxTexWnds == iTexWndLimit)
		{
			tsx = wcWndtexStore + iTexWndTurn;
			iTexWndTurn++;
			if (iTexWndTurn == iTexWndLimit)
				iTexWndTurn = 0;
		}
		else
		{
			tsx = wcWndtexStore + iMaxTexWnds;
			iMaxTexWnds++;
		}
	}

	gTexName = tsx->texname;

	if (TWin.OPosition.y1 == TWin.Position.y1 && TWin.OPosition.x1 == TWin.Position.x1)
	{
		LoadWndTexturePage(pageid, TextureMode, cx, cy);
	}
	else
	{
		LoadStretchWndTexturePage(pageid, TextureMode, cx, cy);
	}

	tsx->Opaque = ubOpaqueDraw;
	tsx->pos.l = npos.l;
	tsx->ClutID = GivenClutId;
	tsx->pageid = pageid;
	tsx->textureMode = TextureMode;
	tsx->texname = gTexName;
	tsx->used = 1;

	return gTexName;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////

static void DefineTextureMovie(void)
{
	if (gTexMovieName == 0)
	{
		glGenTextures(1, &gTexMovieName);
		gTexName = gTexMovieName;
		glBindTexture(GL_TEXTURE_2D, gTexName);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, iClampType);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, iClampType);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTexImage2D(GL_TEXTURE_2D, 0, giWantedRGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, texturepart);
	}
	else
	{
		gTexName = gTexMovieName;
		glBindTexture(GL_TEXTURE_2D, gTexName);
	}

	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (xrMovieArea.x1 - xrMovieArea.x0), (xrMovieArea.y1 - xrMovieArea.y0),
	                GL_RGBA, GL_UNSIGNED_BYTE, texturepart);
}

////////////////////////////////////////////////////////////////////////
// movie texture: load
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////

static GLuint LoadTextureMovieFast(void)
{
	int row, column;
	uint32_t startxy;

	{
		if (PSXDisplay.RGB24)
		{
			uint8_t *pD;
			uint32_t *ta = (uint32_t *)texturepart;

			startxy = ((1024) * xrMovieArea.y0) + xrMovieArea.x0;

			for (column = xrMovieArea.y0; column < xrMovieArea.y1; column++, startxy += 1024)
			{
				// startxy=((1024)*column)+xrMovieArea.x0;
				pD = (uint8_t *)&psxVuw[startxy];
				for (row = xrMovieArea.x0; row < xrMovieArea.x1; row++)
				{
					*ta++ = *((uint32_t *)pD) | 0xff000000;
					pD += 3;
				}
			}
		}
		else
		{
			uint32_t (*LTCOL)(uint32_t);
			uint32_t *ta;

			LTCOL = XP8RGBA_0; // TCF[0];

			ubOpaqueDraw = 0;
			ta = (uint32_t *)texturepart;

			for (column = xrMovieArea.y0; column < xrMovieArea.y1; column++)
			{
				startxy = (1024 * column) + xrMovieArea.x0;
				for (row = xrMovieArea.x0; row < xrMovieArea.x1; row++)
					*ta++ = LTCOL(psxVuw[startxy++] | 0x8000);
			}
		}
		DefineTextureMovie();
	}
	return gTexName;
}

////////////////////////////////////////////////////////////////////////

GLuint LoadTextureMovie(void)
{
	return LoadTextureMovieFast();
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

static GLuint BlackFake15BitTexture(void)
{
	int pmult;
	int16_t x1, x2, y1, y2;

	if (PSXDisplay.InterlacedTest)
		return 0;

	pmult = GlobalTexturePage / 16;
	x1 = gl_ux[7];
	x2 = gl_ux[6] - gl_ux[7];
	y1 = gl_ux[5];
	y2 = gl_ux[4] - gl_ux[5];

	if (iSpriteTex)
	{
		if (x2 < 255)
			x2++;
		if (y2 < 255)
			y2++;
	}

	y1 += pmult * 256;
	x1 += ((GlobalTexturePage - 16 * pmult) << 6);

	if (FastCheckAgainstFrontScreen(x1, y1, x2, y2) || FastCheckAgainstScreen(x1, y1, x2, y2))
	{
		if (!gTexFrameName)
		{
			glGenTextures(1, &gTexFrameName);
			gTexName = gTexFrameName;
			glBindTexture(GL_TEXTURE_2D, gTexName);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, iClampType);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, iClampType);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			{
				uint32_t *ta = (uint32_t *)texturepart;
				for (y1 = 0; y1 <= 4; y1++)
					for (x1 = 0; x1 <= 4; x1++)
						*ta++ = 0xff000000;
			}
			glTexImage2D(GL_TEXTURE_2D, 0, giWantedRGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, texturepart);
		}
		else
		{
			gTexName = gTexFrameName;
			glBindTexture(GL_TEXTURE_2D, gTexName);
		}

		ubOpaqueDraw = 0;

		return (GLuint)gTexName;
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////

bool bFakeFrontBuffer = false;
bool bIgnoreNextTile = false;

static int iFTex = 512;

static GLuint Fake15BitTexture(void)
{
	int pmult;
	int16_t x1, x2, y1, y2;
	int iYAdjust;
	float ScaleX, ScaleY;
	RECT rSrc;

	if (iFrameTexType == 1)
		return BlackFake15BitTexture();
	if (PSXDisplay.InterlacedTest)
		return 0;

	pmult = GlobalTexturePage / 16;
	x1 = gl_ux[7];
	x2 = gl_ux[6] - gl_ux[7];
	y1 = gl_ux[5];
	y2 = gl_ux[4] - gl_ux[5];

	y1 += pmult * 256;
	x1 += ((GlobalTexturePage - 16 * pmult) << 6);

	if (iFrameTexType == 3)
	{
		if (iFrameReadType == 4)
			return 0;

		if (!FastCheckAgainstFrontScreen(x1, y1, x2, y2) && !FastCheckAgainstScreen(x1, y1, x2, y2))
			return 0;

		if (bFakeFrontBuffer)
			bIgnoreNextTile = true;
		CheckVRamReadEx(x1, y1, x1 + x2, y1 + y2);
		return 0;
	}

	/////////////////////////

	if (FastCheckAgainstFrontScreen(x1, y1, x2, y2))
	{
		x1 -= PSXDisplay.DisplayPosition.x;
		y1 -= PSXDisplay.DisplayPosition.y;
	}
	else if (FastCheckAgainstScreen(x1, y1, x2, y2))
	{
		x1 -= PreviousPSXDisplay.DisplayPosition.x;
		y1 -= PreviousPSXDisplay.DisplayPosition.y;
	}
	else
		return 0;

	bDrawMultiPass = false;

	if (!gTexFrameName)
	{
		char *p;

		if (iResX > 1280 || iResY > 1024)
			iFTex = 2048;
		else if (iResX > 640 || iResY > 480)
			iFTex = 1024;
		else
			iFTex = 512;

		glGenTextures(1, &gTexFrameName);
		gTexName = gTexFrameName;
		glBindTexture(GL_TEXTURE_2D, gTexName);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, iClampType);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, iClampType);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		p = (char *)malloc(iFTex * iFTex * 4);
		memset(p, 0, iFTex * iFTex * 4);
		glTexImage2D(GL_TEXTURE_2D, 0, 3, iFTex, iFTex, 0, GL_RGB, GL_UNSIGNED_BYTE, p);
		free(p);

		glGetError();
	}
	else
	{
		gTexName = gTexFrameName;
		glBindTexture(GL_TEXTURE_2D, gTexName);
	}

	x1 += PreviousPSXDisplay.Range.x0;
	y1 += PreviousPSXDisplay.Range.y0;

	if (PSXDisplay.DisplayMode.x)
		ScaleX = (float)iResX / (float)PSXDisplay.DisplayMode.x;
	else
		ScaleX = 1.0f;
	if (PSXDisplay.DisplayMode.y)
		ScaleY = (float)iResY / (float)PSXDisplay.DisplayMode.y;
	else
		ScaleY = 1.0f;

	rSrc.left = max(x1 * ScaleX, 0);
	rSrc.right = min((x1 + x2) * ScaleX + 0.99f, iResX - 1);
	rSrc.top = max(y1 * ScaleY, 0);
	rSrc.bottom = min((y1 + y2) * ScaleY + 0.99f, iResY - 1);

	iYAdjust = (y1 + y2) - PSXDisplay.DisplayMode.y;
	if (iYAdjust > 0)
		iYAdjust = (int)((float)iYAdjust * ScaleY) + 1;
	else
		iYAdjust = 0;

	gl_vy[0] = 255 - gl_vy[0];
	gl_vy[1] = 255 - gl_vy[1];
	gl_vy[2] = 255 - gl_vy[2];
	gl_vy[3] = 255 - gl_vy[3];

	y1 = min(gl_vy[0], min(gl_vy[1], min(gl_vy[2], gl_vy[3])));

	gl_vy[0] -= y1;
	gl_vy[1] -= y1;
	gl_vy[2] -= y1;
	gl_vy[3] -= y1;
	gl_ux[0] -= gl_ux[7];
	gl_ux[1] -= gl_ux[7];
	gl_ux[2] -= gl_ux[7];
	gl_ux[3] -= gl_ux[7];

	ScaleX *= 256.0f / ((float)(iFTex));
	ScaleY *= 256.0f / ((float)(iFTex));

	y1 = ((float)gl_vy[0] * ScaleY);
	if (y1 > 255)
		y1 = 255;
	gl_vy[0] = y1;
	y1 = ((float)gl_vy[1] * ScaleY);
	if (y1 > 255)
		y1 = 255;
	gl_vy[1] = y1;
	y1 = ((float)gl_vy[2] * ScaleY);
	if (y1 > 255)
		y1 = 255;
	gl_vy[2] = y1;
	y1 = ((float)gl_vy[3] * ScaleY);
	if (y1 > 255)
		y1 = 255;
	gl_vy[3] = y1;

	x1 = ((float)gl_ux[0] * ScaleX);
	if (x1 > 255)
		x1 = 255;
	gl_ux[0] = x1;
	x1 = ((float)gl_ux[1] * ScaleX);
	if (x1 > 255)
		x1 = 255;
	gl_ux[1] = x1;
	x1 = ((float)gl_ux[2] * ScaleX);
	if (x1 > 255)
		x1 = 255;
	gl_ux[2] = x1;
	x1 = ((float)gl_ux[3] * ScaleX);
	if (x1 > 255)
		x1 = 255;
	gl_ux[3] = x1;

	x1 = rSrc.right - rSrc.left;
	if (x1 <= 0)
		x1 = 1;
	if (x1 > iFTex)
		x1 = iFTex;

	y1 = rSrc.bottom - rSrc.top;
	if (y1 <= 0)
		y1 = 1;
	if (y1 + iYAdjust > iFTex)
		y1 = iFTex - iYAdjust;

	if (bFakeFrontBuffer)
		glReadBuffer(GL_FRONT);

	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, iYAdjust, rSrc.left + 0, iResY - rSrc.bottom - 0, x1, y1);

	if (glGetError())
	{
		char *p = (char *)malloc(iFTex * iFTex * 4);
		memset(p, 0, iFTex * iFTex * 4);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, iFTex, iFTex, GL_RGB, GL_UNSIGNED_BYTE, p);
		free(p);
	}

	if (bFakeFrontBuffer)
	{
		glReadBuffer(GL_BACK);
		bIgnoreNextTile = true;
	}

	ubOpaqueDraw = 0;

	if (iSpriteTex)
	{
		sprtW = gl_ux[1] - gl_ux[0];
		sprtH = -(gl_vy[0] - gl_vy[2]);
	}

	return (GLuint)gTexName;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// load texture part (unpacked)
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void LoadSubTexturePageSort(int pageid, int mode, int16_t cx, int16_t cy)
{
	uint32_t start, row, column, j, sxh, sxm;
	uint32_t palstart;
	uint32_t *px, *pa, *ta;
	uint8_t *cSRCPtr;
	uint16_t *wSRCPtr;
	uint32_t LineOffset;
	uint32_t x2a, xalign = 0;
	uint32_t x1 = gl_ux[7];
	uint32_t x2 = gl_ux[6];
	uint32_t y1 = gl_ux[5];
	uint32_t y2 = gl_ux[4];
	uint32_t dx = x2 - x1 + 1;
	uint32_t dy = y2 - y1 + 1;
	int pmult = pageid / 16;
	uint32_t (*LTCOL)(uint32_t);

	LTCOL = TCF[DrawSemiTrans];

	pa = px = (uint32_t *)ubPaletteBuffer;
	ta = (uint32_t *)texturepart;
	palstart = cx + (cy << 10);

	ubOpaqueDraw = 0;

	if (YTexS)
	{
		ta += dx;
		if (XTexS)
			ta += 2;
	}
	if (XTexS)
	{
		ta += 1;
		xalign = 2;
	}

	switch (mode)
	{
	//--------------------------------------------------//
	// 4bit texture load ..
	case 0:
		if (GlobalTextIL)
		{
			uint32_t TXV, TXU, n_xi, n_yi;

			wSRCPtr = psxVuw + palstart;

			row = 4;
			do
			{
				*px = LTCOL(*wSRCPtr);
				*(px + 1) = LTCOL(*(wSRCPtr + 1));
				*(px + 2) = LTCOL(*(wSRCPtr + 2));
				*(px + 3) = LTCOL(*(wSRCPtr + 3));
				row--;
				px += 4;
				wSRCPtr += 4;
			} while (row);

			for (TXV = y1; TXV <= y2; TXV++)
			{
				for (TXU = x1; TXU <= x2; TXU++)
				{
					n_xi = ((TXU >> 2) & ~0x3c) + ((TXV << 2) & 0x3c);
					n_yi = (TXV & ~0xf) + ((TXU >> 4) & 0xf);

					*ta++ = *(pa + ((*(psxVuw + ((GlobalTextAddrY + n_yi) * 1024) + GlobalTextAddrX + n_xi) >>
					                 ((TXU & 0x03) << 2)) &
					                0x0f));
				}
				ta += xalign;
			}
			break;
		}

		start = ((pageid - 16 * pmult) << 7) + 524288 * pmult;
		// convert CLUT to 32bits .. and then use THAT as a lookup table

		wSRCPtr = psxVuw + palstart;

		row = 4;
		do
		{
			*px = LTCOL(*wSRCPtr);
			*(px + 1) = LTCOL(*(wSRCPtr + 1));
			*(px + 2) = LTCOL(*(wSRCPtr + 2));
			*(px + 3) = LTCOL(*(wSRCPtr + 3));
			row--;
			px += 4;
			wSRCPtr += 4;
		} while (row);

		x2a = x2 ? (x2 - 1) : 0; // if(x2) x2a=x2-1; else x2a=0;
		sxm = x1 & 1;
		sxh = x1 >> 1;
		j = sxm ? (x1 + 1) : x1; // if(sxm) j=x1+1; else j=x1;
		for (column = y1; column <= y2; column++)
		{
			cSRCPtr = psxVub + start + (column << 11) + sxh;

			if (sxm)
				*ta++ = *(pa + ((*cSRCPtr++ >> 4) & 0xF));

			for (row = j; row < x2a; row += 2)
			{
				*ta = *(pa + (*cSRCPtr & 0xF));
				*(ta + 1) = *(pa + ((*cSRCPtr >> 4) & 0xF));
				cSRCPtr++;
				ta += 2;
			}

			if (row <= x2)
			{
				*ta++ = *(pa + (*cSRCPtr & 0xF));
				row++;
				if (row <= x2)
					*ta++ = *(pa + ((*cSRCPtr >> 4) & 0xF));
			}

			ta += xalign;
		}

		break;
	//--------------------------------------------------//
	// 8bit texture load ..
	case 1:
		if (GlobalTextIL)
		{
			uint32_t TXV, TXU, n_xi, n_yi;

			wSRCPtr = psxVuw + palstart;

			row = 64;
			do
			{
				*px = LTCOL(*wSRCPtr);
				*(px + 1) = LTCOL(*(wSRCPtr + 1));
				*(px + 2) = LTCOL(*(wSRCPtr + 2));
				*(px + 3) = LTCOL(*(wSRCPtr + 3));
				row--;
				px += 4;
				wSRCPtr += 4;
			} while (row);

			for (TXV = y1; TXV <= y2; TXV++)
			{
				for (TXU = x1; TXU <= x2; TXU++)
				{
					n_xi = ((TXU >> 1) & ~0x78) + ((TXU << 2) & 0x40) + ((TXV << 3) & 0x38);
					n_yi = (TXV & ~0x7) + ((TXU >> 5) & 0x7);

					*ta++ = *(pa + ((*(psxVuw + ((GlobalTextAddrY + n_yi) * 1024) + GlobalTextAddrX + n_xi) >>
					                 ((TXU & 0x01) << 3)) &
					                0xff));
				}
				ta += xalign;
			}

			break;
		}

		start = ((pageid - 16 * pmult) << 7) + 524288 * pmult;

		cSRCPtr = psxVub + start + (y1 << 11) + x1;
		LineOffset = 2048 - dx;

		if (dy * dx > 384)
		{
			wSRCPtr = psxVuw + palstart;

			row = 64;
			do
			{
				*px = LTCOL(*wSRCPtr);
				*(px + 1) = LTCOL(*(wSRCPtr + 1));
				*(px + 2) = LTCOL(*(wSRCPtr + 2));
				*(px + 3) = LTCOL(*(wSRCPtr + 3));
				row--;
				px += 4;
				wSRCPtr += 4;
			} while (row);

			column = dy;
			do
			{
				row = dx;
				do
				{
					*ta++ = *(pa + (*cSRCPtr++));
					row--;
				} while (row);
				ta += xalign;
				cSRCPtr += LineOffset;
				column--;
			} while (column);
		}
		else
		{
			wSRCPtr = psxVuw + palstart;

			column = dy;
			do
			{
				row = dx;
				do
				{
					*ta++ = LTCOL(*(wSRCPtr + *cSRCPtr++));
					row--;
				} while (row);
				ta += xalign;
				cSRCPtr += LineOffset;
				column--;
			} while (column);
		}

		break;
	//--------------------------------------------------//
	// 16bit texture load ..
	case 2:
		start = ((pageid - 16 * pmult) << 6) + 262144 * pmult;

		wSRCPtr = psxVuw + start + (y1 << 10) + x1;
		LineOffset = 1024 - dx;

		column = dy;
		do
		{
			row = dx;
			do
			{
				*ta++ = LTCOL(*wSRCPtr++);
				row--;
			} while (row);
			ta += xalign;
			wSRCPtr += LineOffset;
			column--;
		} while (column);

		break;
		//--------------------------------------------------//
		// others are not possible !
	}

	x2a = dx + xalign;

	if (YTexS)
	{
		ta = (uint32_t *)texturepart;
		pa = (uint32_t *)texturepart + x2a;
		row = x2a;
		do
		{
			*ta++ = *pa++;
			row--;
		} while (row);
		pa = (uint32_t *)texturepart + dy * x2a;
		ta = pa + x2a;
		row = x2a;
		do
		{
			*ta++ = *pa++;
			row--;
		} while (row);
		YTexS--;
		dy += 2;
	}

	if (XTexS)
	{
		ta = (uint32_t *)texturepart;
		pa = ta + 1;
		row = dy;
		do
		{
			*ta = *pa;
			ta += x2a;
			pa += x2a;
			row--;
		} while (row);
		pa = (uint32_t *)texturepart + dx;
		ta = pa + 1;
		row = dy;
		do
		{
			*ta = *pa;
			ta += x2a;
			pa += x2a;
			row--;
		} while (row);
		XTexS--;
		dx += 2;
	}

	DXTexS = dx;
	DYTexS = dy;

	DefineSubTextureSort();
}

/////////////////////////////////////////////////////////////////////////////

void DefineSubTextureSort(void)
{
	if (!gTexName)
	{
		glGenTextures(1, &gTexName);
		glBindTexture(GL_TEXTURE_2D, gTexName);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, iClampType);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, iClampType);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTexImage2D(GL_TEXTURE_2D, 0, giWantedRGBA, 256, 256, 0, giWantedFMT, giWantedTYPE, texturepart);
	}
	else
		glBindTexture(GL_TEXTURE_2D, gTexName);

	glTexSubImage2D(GL_TEXTURE_2D, 0, XTexS, YTexS, DXTexS, DYTexS, giWantedFMT, giWantedTYPE, texturepart);
}

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// texture cache garbage collection
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

static void DoTexGarbageCollection(void)
{
	static uint16_t LRUCleaned = 0;
	uint16_t iC, iC1, iC2;
	int i, j, iMax;
	textureSubCacheEntryS *tsb;

	iC = 4;           //=iSortTexCnt/2,
	LRUCleaned += iC; // we clean different textures each time
	if ((LRUCleaned + iC) >= iSortTexCnt)
		LRUCleaned = 0; // wrap? wrap!
	iC1 = LRUCleaned;   // range of textures to clean
	iC2 = LRUCleaned + iC;

	for (iC = iC1; iC < iC2; iC++) // make some textures available
	{
		pxSsubtexLeft[iC]->l = 0;
	}

	for (i = 0; i < 3; i++) // remove all references to that textures
		for (j = 0; j < MAXTPAGES; j++)
			for (iC = 0; iC < 4; iC++) // loop all texture rect info areas
			{
				tsb = pscSubtexStore[i][j] + (iC * SOFFB);
				iMax = tsb->pos.l;
				if (iMax)
					do
					{
						tsb++;
						if (tsb->cTexID >= iC1 && tsb->cTexID < iC2) // info uses the cleaned textures? remove info
							tsb->ClutID = 0;
					} while (--iMax);
			}

	usLRUTexPage = LRUCleaned;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// search cache for existing (already used) parts
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

uint8_t *CheckTextureInSubSCache(int TextureMode, uint32_t GivenClutId, uint16_t *pCache)
{
	textureSubCacheEntryS *tsx, *tsb, *tsg; //, *tse=NULL;
	int i, iMax;
	EXLong npos;
	uint8_t cx, cy;
	int iC, j, k;
	uint32_t rx, ry, mx, my;
	EXLong *ul = 0, *uls;
	EXLong rfree;
	uint8_t cXAdj, cYAdj;

	npos.l = *((uint32_t *)&gl_ux[4]);

	//--------------------------------------------------------------//
	// find matching texturepart first... speed up...
	//--------------------------------------------------------------//

	tsg = pscSubtexStore[TextureMode][GlobalTexturePage];
	tsg += ((GivenClutId & CLUTCHK) >> CLUTSHIFT) * SOFFB;

	iMax = tsg->pos.l;
	if (iMax)
	{
		i = iMax;
		tsb = tsg + 1;
		do
		{
			if (GivenClutId == tsb->ClutID && (INCHECK(tsb->pos, npos)))
			{
				{
					cx = tsb->pos.c[3] - tsb->posTX;
					cy = tsb->pos.c[1] - tsb->posTY;

					gl_ux[0] -= cx;
					gl_ux[1] -= cx;
					gl_ux[2] -= cx;
					gl_ux[3] -= cx;
					gl_vy[0] -= cy;
					gl_vy[1] -= cy;
					gl_vy[2] -= cy;
					gl_vy[3] -= cy;

					ubOpaqueDraw = tsb->Opaque;
					*pCache = tsb->cTexID;
					return NULL;
				}
			}
			tsb++;
		} while (--i);
	}

	//----------------------------------------------------//

	cXAdj = 1;
	cYAdj = 1;

	rx = (int)gl_ux[6] - (int)gl_ux[7];
	ry = (int)gl_ux[4] - (int)gl_ux[5];

	tsx = NULL;
	tsb = tsg + 1;
	for (i = 0; i < iMax; i++, tsb++)
	{
		if (!tsb->ClutID)
		{
			tsx = tsb;
			break;
		}
	}

	if (!tsx)
	{
		iMax++;
		if (iMax >= SOFFB - 2)
		{
			if (iTexGarbageCollection) // gc mode?
			{
				if (*pCache == 0)
				{
					dwTexPageComp |= (1 << GlobalTexturePage);
					*pCache = 0xffff;
					return 0;
				}

				iMax--;
				tsb = tsg + 1;

				for (i = 0; i < iMax; i++, tsb++) // 1. search other slots with same cluts, and unite the area
					if (GivenClutId == tsb->ClutID)
					{
						if (!tsx)
						{
							tsx = tsb;
							rfree.l = npos.l;
						} //
						else
							tsb->ClutID = 0;
						rfree.c[3] = min(rfree.c[3], tsb->pos.c[3]);
						rfree.c[2] = max(rfree.c[2], tsb->pos.c[2]);
						rfree.c[1] = min(rfree.c[1], tsb->pos.c[1]);
						rfree.c[0] = max(rfree.c[0], tsb->pos.c[0]);
						MarkFree(tsb);
					}

				if (tsx) // 3. if one or more found, create a new rect with bigger size
				{
					*((uint32_t *)&gl_ux[4]) = npos.l = rfree.l;
					rx = (int)rfree.c[2] - (int)rfree.c[3];
					ry = (int)rfree.c[0] - (int)rfree.c[1];
					DoTexGarbageCollection();

					goto ENDLOOP3;
				}
			}

			iMax = 1;
		}
		tsx = tsg + iMax;
		tsg->pos.l = iMax;
	}

	//----------------------------------------------------//
	// now get a free texture space
	//----------------------------------------------------//

	if (iTexGarbageCollection)
		usLRUTexPage = 0;

ENDLOOP3:

	rx += 3;
	if (rx > 255)
	{
		cXAdj = 0;
		rx = 255;
	}
	ry += 3;
	if (ry > 255)
	{
		cYAdj = 0;
		ry = 255;
	}

	iC = usLRUTexPage;

	for (k = 0; k < iSortTexCnt; k++)
	{
		uls = pxSsubtexLeft[iC];
		iMax = uls->l;
		ul = uls + 1;

		//--------------------------------------------------//
		// first time

		if (!iMax)
		{
			rfree.l = 0;

			if (rx > 252 && ry > 252)
			{
				uls->l = 1;
				ul->l = 0xffffffff;
				ul = 0;
				goto ENDLOOP;
			}

			if (rx < 253)
			{
				uls->l = uls->l + 1;
				ul->c[3] = rx;
				ul->c[2] = 255 - rx;
				ul->c[1] = 0;
				ul->c[0] = ry;
				ul++;
			}

			if (ry < 253)
			{
				uls->l = uls->l + 1;
				ul->c[3] = 0;
				ul->c[2] = 255;
				ul->c[1] = ry;
				ul->c[0] = 255 - ry;
			}
			ul = 0;
			goto ENDLOOP;
		}

		//--------------------------------------------------//
		for (i = 0; i < iMax; i++, ul++)
		{
			if (ul->l != 0xffffffff && ry <= ul->c[0] && rx <= ul->c[2])
			{
				rfree = *ul;
				mx = ul->c[2] - 2;
				my = ul->c[0] - 2;
				if (rx < mx && ry < my)
				{
					ul->c[3] += rx;
					ul->c[2] -= rx;
					ul->c[0] = ry;

					for (ul = uls + 1, j = 0; j < iMax; j++, ul++)
						if (ul->l == 0xffffffff)
							break;

					if (j < CSUBSIZE - 2)
					{
						if (j == iMax)
							uls->l = uls->l + 1;

						ul->c[3] = rfree.c[3];
						ul->c[2] = rfree.c[2];
						ul->c[1] = rfree.c[1] + ry;
						ul->c[0] = rfree.c[0] - ry;
					}
				}
				else if (rx < mx)
				{
					ul->c[3] += rx;
					ul->c[2] -= rx;
				}
				else if (ry < my)
				{
					ul->c[1] += ry;
					ul->c[0] -= ry;
				}
				else
				{
					ul->l = 0xffffffff;
				}
				ul = 0;
				goto ENDLOOP;
			}
		}

		//--------------------------------------------------//

		iC++;
		if (iC >= iSortTexCnt)
			iC = 0;
	}

//----------------------------------------------------//
// check, if free space got
//----------------------------------------------------//

ENDLOOP:
	if (ul)
	{
		//////////////////////////////////////////////////////

		{
			dwTexPageComp = 0;

			for (i = 0; i < 3; i++) // cleaning up
				for (j = 0; j < MAXTPAGES; j++)
				{
					tsb = pscSubtexStore[i][j];
					(tsb + SOFFA)->pos.l = 0;
					(tsb + SOFFB)->pos.l = 0;
					(tsb + SOFFC)->pos.l = 0;
					(tsb + SOFFD)->pos.l = 0;
				}
			for (i = 0; i < iSortTexCnt; i++)
			{
				ul = pxSsubtexLeft[i];
				ul->l = 0;
			}
			usLRUTexPage = 0;
		}

		//////////////////////////////////////////////////////
		iC = usLRUTexPage;
		uls = pxSsubtexLeft[usLRUTexPage];
		uls->l = 0;
		ul = uls + 1;
		rfree.l = 0;

		if (rx > 252 && ry > 252)
		{
			uls->l = 1;
			ul->l = 0xffffffff;
		}
		else
		{
			if (rx < 253)
			{
				uls->l = uls->l + 1;
				ul->c[3] = rx;
				ul->c[2] = 255 - rx;
				ul->c[1] = 0;
				ul->c[0] = ry;
				ul++;
			}
			if (ry < 253)
			{
				uls->l = uls->l + 1;
				ul->c[3] = 0;
				ul->c[2] = 255;
				ul->c[1] = ry;
				ul->c[0] = 255 - ry;
			}
		}
		tsg->pos.l = 1;
		tsx = tsg + 1;
	}

	rfree.c[3] += cXAdj;
	rfree.c[1] += cYAdj;

	tsx->cTexID = *pCache = iC;
	tsx->pos = npos;
	tsx->ClutID = GivenClutId;
	tsx->posTX = rfree.c[3];
	tsx->posTY = rfree.c[1];

	cx = gl_ux[7] - rfree.c[3];
	cy = gl_ux[5] - rfree.c[1];

	gl_ux[0] -= cx;
	gl_ux[1] -= cx;
	gl_ux[2] -= cx;
	gl_ux[3] -= cx;
	gl_vy[0] -= cy;
	gl_vy[1] -= cy;
	gl_vy[2] -= cy;
	gl_vy[3] -= cy;

	XTexS = rfree.c[3];
	YTexS = rfree.c[1];

	return &tsx->Opaque;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// search cache for free place (on compress)
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

static bool GetCompressTexturePlace(textureSubCacheEntryS *tsx)
{
	int i, j, k, iMax, iC;
	uint32_t rx, ry, mx, my;
	EXLong *ul = 0, *uls, rfree;
	uint8_t cXAdj = 1, cYAdj = 1;

	rx = (int)tsx->pos.c[2] - (int)tsx->pos.c[3];
	ry = (int)tsx->pos.c[0] - (int)tsx->pos.c[1];

	rx += 3;
	if (rx > 255)
	{
		cXAdj = 0;
		rx = 255;
	}
	ry += 3;
	if (ry > 255)
	{
		cYAdj = 0;
		ry = 255;
	}

	iC = usLRUTexPage;

	for (k = 0; k < iSortTexCnt; k++)
	{
		uls = pxSsubtexLeft[iC];
		iMax = uls->l;
		ul = uls + 1;

		//--------------------------------------------------//
		// first time

		if (!iMax)
		{
			rfree.l = 0;

			if (rx > 252 && ry > 252)
			{
				uls->l = 1;
				ul->l = 0xffffffff;
				ul = 0;
				goto TENDLOOP;
			}

			if (rx < 253)
			{
				uls->l = uls->l + 1;
				ul->c[3] = rx;
				ul->c[2] = 255 - rx;
				ul->c[1] = 0;
				ul->c[0] = ry;
				ul++;
			}

			if (ry < 253)
			{
				uls->l = uls->l + 1;
				ul->c[3] = 0;
				ul->c[2] = 255;
				ul->c[1] = ry;
				ul->c[0] = 255 - ry;
			}
			ul = 0;
			goto TENDLOOP;
		}

		//--------------------------------------------------//
		for (i = 0; i < iMax; i++, ul++)
		{
			if (ul->l != 0xffffffff && ry <= ul->c[0] && rx <= ul->c[2])
			{
				rfree = *ul;
				mx = ul->c[2] - 2;
				my = ul->c[0] - 2;

				if (rx < mx && ry < my)
				{
					ul->c[3] += rx;
					ul->c[2] -= rx;
					ul->c[0] = ry;

					for (ul = uls + 1, j = 0; j < iMax; j++, ul++)
						if (ul->l == 0xffffffff)
							break;

					if (j < CSUBSIZE - 2)
					{
						if (j == iMax)
							uls->l = uls->l + 1;

						ul->c[3] = rfree.c[3];
						ul->c[2] = rfree.c[2];
						ul->c[1] = rfree.c[1] + ry;
						ul->c[0] = rfree.c[0] - ry;
					}
				}
				else if (rx < mx)
				{
					ul->c[3] += rx;
					ul->c[2] -= rx;
				}
				else if (ry < my)
				{
					ul->c[1] += ry;
					ul->c[0] -= ry;
				}
				else
				{
					ul->l = 0xffffffff;
				}
				ul = 0;
				goto TENDLOOP;
			}
		}

		//--------------------------------------------------//

		iC++;
		if (iC >= iSortTexCnt)
			iC = 0;
	}

//----------------------------------------------------//
// check, if free space got
//----------------------------------------------------//

TENDLOOP:
	if (ul)
		return false;

	rfree.c[3] += cXAdj;
	rfree.c[1] += cYAdj;

	tsx->cTexID = iC;
	tsx->posTX = rfree.c[3];
	tsx->posTY = rfree.c[1];

	XTexS = rfree.c[3];
	YTexS = rfree.c[1];

	return true;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// compress texture cache (to make place for new texture part, if needed)
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

static void CompressTextureSpace(void)
{
	textureSubCacheEntryS *tsx, *tsg, *tsb;
	int i, j, k, m, n, iMax;
	EXLong *ul, r, opos;
	int16_t sOldDST = DrawSemiTrans, cx, cy;
	int lOGTP = GlobalTexturePage;
	uint32_t l, row;
	uint32_t *lSRCPtr;

	opos.l = *((uint32_t *)&gl_ux[4]);

	// 1. mark all textures as free
	for (i = 0; i < iSortTexCnt; i++)
	{
		ul = pxSsubtexLeft[i];
		ul->l = 0;
	}
	usLRUTexPage = 0;

	// 2. compress
	for (j = 0; j < 3; j++)
	{
		for (k = 0; k < MAXTPAGES; k++)
		{
			tsg = pscSubtexStore[j][k];

			if ((!(dwTexPageComp & (1 << k))))
			{
				(tsg + SOFFA)->pos.l = 0;
				(tsg + SOFFB)->pos.l = 0;
				(tsg + SOFFC)->pos.l = 0;
				(tsg + SOFFD)->pos.l = 0;
				continue;
			}

			for (m = 0; m < 4; m++, tsg += SOFFB)
			{
				iMax = tsg->pos.l;

				tsx = tsg + 1;
				for (i = 0; i < iMax; i++, tsx++)
				{
					if (tsx->ClutID)
					{
						r.l = tsx->pos.l;
						for (n = i + 1, tsb = tsx + 1; n < iMax; n++, tsb++)
						{
							if (tsx->ClutID == tsb->ClutID)
							{
								r.c[3] = min(r.c[3], tsb->pos.c[3]);
								r.c[2] = max(r.c[2], tsb->pos.c[2]);
								r.c[1] = min(r.c[1], tsb->pos.c[1]);
								r.c[0] = max(r.c[0], tsb->pos.c[0]);
								tsb->ClutID = 0;
							}
						}

						//           if(r.l!=tsx->pos.l)
						{
							cx = ((tsx->ClutID << 4) & 0x3F0);
							cy = ((tsx->ClutID >> 6) & CLUTYMASK);

							if (j != 2)
							{
								// palette check sum
								l = 0;
								lSRCPtr = (uint32_t *)(psxVuw + cx + (cy * 1024));
								if (j == 1)
									for (row = 1; row < 129; row++)
										l += ((*lSRCPtr++) - 1) * row;
								else
									for (row = 1; row < 9; row++)
										l += ((*lSRCPtr++) - 1) << row;
								l = ((l + HIWORD(l)) & 0x3fffL) << 16;
								if (l != (tsx->ClutID & (0x00003fff << 16)))
								{
									tsx->ClutID = 0;
									continue;
								}
							}

							tsx->pos.l = r.l;
							if (!GetCompressTexturePlace(tsx)) // no place?
							{
								for (i = 0; i < 3; i++) // -> clean up everything
									for (j = 0; j < MAXTPAGES; j++)
									{
										tsb = pscSubtexStore[i][j];
										(tsb + SOFFA)->pos.l = 0;
										(tsb + SOFFB)->pos.l = 0;
										(tsb + SOFFC)->pos.l = 0;
										(tsb + SOFFD)->pos.l = 0;
									}
								for (i = 0; i < iSortTexCnt; i++)
								{
									ul = pxSsubtexLeft[i];
									ul->l = 0;
								}
								usLRUTexPage = 0;
								DrawSemiTrans = sOldDST;
								GlobalTexturePage = lOGTP;
								*((uint32_t *)&gl_ux[4]) = opos.l;
								dwTexPageComp = 0;

								return;
							}

							if (tsx->ClutID & (1 << 30))
								DrawSemiTrans = 1;
							else
								DrawSemiTrans = 0;
							*((uint32_t *)&gl_ux[4]) = r.l;

							gTexName = uiStexturePage[tsx->cTexID];
							LoadSubTexFn(k, j, cx, cy);
							uiStexturePage[tsx->cTexID] = gTexName;
							tsx->Opaque = ubOpaqueDraw;
						}
					}
				}

				if (iMax)
				{
					tsx = tsg + iMax;
					while (!tsx->ClutID && iMax)
					{
						tsx--;
						iMax--;
					}
					tsg->pos.l = iMax;
				}
			}
		}
	}

	if (dwTexPageComp == 0xffffffff)
		dwTexPageComp = 0;

	*((uint32_t *)&gl_ux[4]) = opos.l;
	GlobalTexturePage = lOGTP;
	DrawSemiTrans = sOldDST;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// main entry for searching/creating textures, called from prim.c
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

GLuint SelectSubTextureS(int TextureMode, uint32_t GivenClutId)
{
	uint8_t *OPtr;
	uint16_t iCache;
	int16_t cx, cy;

	// sort sow/tow infos for fast access

	uint8_t ma1, ma2, mi1, mi2;
	if (gl_ux[0] > gl_ux[1])
	{
		mi1 = gl_ux[1];
		ma1 = gl_ux[0];
	}
	else
	{
		mi1 = gl_ux[0];
		ma1 = gl_ux[1];
	}
	if (gl_ux[2] > gl_ux[3])
	{
		mi2 = gl_ux[3];
		ma2 = gl_ux[2];
	}
	else
	{
		mi2 = gl_ux[2];
		ma2 = gl_ux[3];
	}
	if (mi1 > mi2)
		gl_ux[7] = mi2;
	else
		gl_ux[7] = mi1;
	if (ma1 > ma2)
		gl_ux[6] = ma1;
	else
		gl_ux[6] = ma2;

	if (gl_vy[0] > gl_vy[1])
	{
		mi1 = gl_vy[1];
		ma1 = gl_vy[0];
	}
	else
	{
		mi1 = gl_vy[0];
		ma1 = gl_vy[1];
	}
	if (gl_vy[2] > gl_vy[3])
	{
		mi2 = gl_vy[3];
		ma2 = gl_vy[2];
	}
	else
	{
		mi2 = gl_vy[2];
		ma2 = gl_vy[3];
	}
	if (mi1 > mi2)
		gl_ux[5] = mi2;
	else
		gl_ux[5] = mi1;
	if (ma1 > ma2)
		gl_ux[4] = ma1;
	else
		gl_ux[4] = ma2;

	// get clut infos in one 32 bit val

	if (TextureMode == 2) // no clut here
	{
		GivenClutId = CLUTUSED | (DrawSemiTrans << 30);
		cx = cy = 0;

		if (iFrameTexType && Fake15BitTexture())
			return (GLuint)gTexName;
	}
	else
	{
		cx = ((GivenClutId << 4) & 0x3F0); // but here
		cy = ((GivenClutId >> 6) & CLUTYMASK);
		GivenClutId = (GivenClutId & CLUTMASK) | (DrawSemiTrans << 30) | CLUTUSED;

		// palette check sum.. removed MMX asm, this easy func works as well
		{
			uint32_t l = 0, row;

			uint32_t *lSRCPtr = (uint32_t *)(psxVuw + cx + (cy * 1024));
			if (TextureMode == 1)
				for (row = 1; row < 129; row++)
					l += ((*lSRCPtr++) - 1) * row;
			else
				for (row = 1; row < 9; row++)
					l += ((*lSRCPtr++) - 1) << row;
			l = (l + HIWORD(l)) & 0x3fffL;
			GivenClutId |= (l << 16);
		}
	}

	// search cache
	iCache = 0;
	OPtr = CheckTextureInSubSCache(TextureMode, GivenClutId, &iCache);

	// cache full? compress and try again
	if (iCache == 0xffff)
	{
		CompressTextureSpace();
		OPtr = CheckTextureInSubSCache(TextureMode, GivenClutId, &iCache);
	}

	// found? fine
	usLRUTexPage = iCache;
	if (!OPtr)
		return uiStexturePage[iCache];

	// not found? upload texture and store infos in cache
	gTexName = uiStexturePage[iCache];
	LoadSubTexFn(GlobalTexturePage, TextureMode, cx, cy);
	uiStexturePage[iCache] = gTexName;
	*OPtr = ubOpaqueDraw;
	return (GLuint)gTexName;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
