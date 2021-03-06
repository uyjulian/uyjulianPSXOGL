/***************************************************************************
                          prim.c  -  description
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

#include "externals.h"

////////////////////////////////////////////////////////////////////////
// globals
////////////////////////////////////////////////////////////////////////

static bool bDrawTextured; // current active drawing states
static bool bDrawSmoothShaded;
static bool bOldSmoothShaded;
static bool bDrawNonShaded;
bool bDrawMultiPass;
int32_t iDrawnSomething = 0;

bool bRenderFrontBuffer = false; // flag for front buffer rendering

GLubyte ubGloAlpha;                 // texture alpha
GLubyte ubGloColAlpha;              // color alpha
bool bFullVRam = false;             // sign for tex win
GLuint gTexName;                    // binded texture
bool bTexEnabled;                   // texture enable flag
static bool bBlendEnable;           // blend enable flag
PSXRect_t xrUploadArea;             // rect to upload
PSXRect_t xrUploadAreaIL;           // rect to upload
static PSXRect_t xrUploadAreaRGB24; // rect to upload rgb24
int32_t iSpriteTex = 0;                 // flag for "hey, it's a sprite"
uint16_t usMirror;                  // mirror, mirror on the wall

bool bNeedUploadAfter = false;   // sign for uploading in next frame
bool bNeedUploadTest = false;    // sign for upload test
bool bUsingTWin = false;         // tex win active flag
static bool bUsingMovie = false; // movie active flag
PSXRect_t xrMovieArea;           // rect for movie upload
static int16_t sSprite_ux2;      // needed for sprire adjust
static int16_t sSprite_vy2;      //
static uint32_t ulClutID;        // clut

int32_t drawX, drawY, drawW, drawH; // offscreen drawing checkers
int16_t sxmin, sxmax, symin, symax;

////////////////////////////////////////////////////////////////////////
// Offset stuff
////////////////////////////////////////////////////////////////////////

// please note: it is hardly do-able in a hw/accel plugin to get the
//              real psx polygon coord mapping right... the following
//              works not to bad with many games, though

static __inline bool CheckCoord4()
{
	if (lx0 < 0)
	{
		if (((lx1 - lx0) > CHKMAX_X) || ((lx2 - lx0) > CHKMAX_X))
		{
			if (lx3 < 0)
			{
				if ((lx1 - lx3) > CHKMAX_X)
					return true;
				if ((lx2 - lx3) > CHKMAX_X)
					return true;
			}
		}
	}
	if (lx1 < 0)
	{
		if ((lx0 - lx1) > CHKMAX_X)
			return true;
		if ((lx2 - lx1) > CHKMAX_X)
			return true;
		if ((lx3 - lx1) > CHKMAX_X)
			return true;
	}
	if (lx2 < 0)
	{
		if ((lx0 - lx2) > CHKMAX_X)
			return true;
		if ((lx1 - lx2) > CHKMAX_X)
			return true;
		if ((lx3 - lx2) > CHKMAX_X)
			return true;
	}
	if (lx3 < 0)
	{
		if (((lx1 - lx3) > CHKMAX_X) || ((lx2 - lx3) > CHKMAX_X))
		{
			if (lx0 < 0)
			{
				if ((lx1 - lx0) > CHKMAX_X)
					return true;
				if ((lx2 - lx0) > CHKMAX_X)
					return true;
			}
		}
	}

	if (ly0 < 0)
	{
		if ((ly1 - ly0) > CHKMAX_Y)
			return true;
		if ((ly2 - ly0) > CHKMAX_Y)
			return true;
	}
	if (ly1 < 0)
	{
		if ((ly0 - ly1) > CHKMAX_Y)
			return true;
		if ((ly2 - ly1) > CHKMAX_Y)
			return true;
		if ((ly3 - ly1) > CHKMAX_Y)
			return true;
	}
	if (ly2 < 0)
	{
		if ((ly0 - ly2) > CHKMAX_Y)
			return true;
		if ((ly1 - ly2) > CHKMAX_Y)
			return true;
		if ((ly3 - ly2) > CHKMAX_Y)
			return true;
	}
	if (ly3 < 0)
	{
		if ((ly1 - ly3) > CHKMAX_Y)
			return true;
		if ((ly2 - ly3) > CHKMAX_Y)
			return true;
	}

	return false;
}

static __inline bool CheckCoord3()
{
	if (lx0 < 0)
	{
		if ((lx1 - lx0) > CHKMAX_X)
			return true;
		if ((lx2 - lx0) > CHKMAX_X)
			return true;
	}
	if (lx1 < 0)
	{
		if ((lx0 - lx1) > CHKMAX_X)
			return true;
		if ((lx2 - lx1) > CHKMAX_X)
			return true;
	}
	if (lx2 < 0)
	{
		if ((lx0 - lx2) > CHKMAX_X)
			return true;
		if ((lx1 - lx2) > CHKMAX_X)
			return true;
	}
	if (ly0 < 0)
	{
		if ((ly1 - ly0) > CHKMAX_Y)
			return true;
		if ((ly2 - ly0) > CHKMAX_Y)
			return true;
	}
	if (ly1 < 0)
	{
		if ((ly0 - ly1) > CHKMAX_Y)
			return true;
		if ((ly2 - ly1) > CHKMAX_Y)
			return true;
	}
	if (ly2 < 0)
	{
		if ((ly0 - ly2) > CHKMAX_Y)
			return true;
		if ((ly1 - ly2) > CHKMAX_Y)
			return true;
	}

	return false;
}

static __inline bool CheckCoord2()
{
	if (lx0 < 0)
	{
		if ((lx1 - lx0) > CHKMAX_X)
			return true;
	}
	if (lx1 < 0)
	{
		if ((lx0 - lx1) > CHKMAX_X)
			return true;
	}
	if (ly0 < 0)
	{
		if ((ly1 - ly0) > CHKMAX_Y)
			return true;
	}
	if (ly1 < 0)
	{
		if ((ly0 - ly1) > CHKMAX_Y)
			return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////
// POLYGON OFFSET FUNCS
////////////////////////////////////////////////////////////////////////

static void offsetPSXLine(void)
{
	int16_t x0, x1, y0, y1, dx, dy;
	float px, py;

	x0 = lx0 + 1 + PSXDisplay.DrawOffset.x;
	x1 = lx1 + 1 + PSXDisplay.DrawOffset.x;
	y0 = ly0 + 1 + PSXDisplay.DrawOffset.y;
	y1 = ly1 + 1 + PSXDisplay.DrawOffset.y;

	dx = x1 - x0;
	dy = y1 - y0;

	// tricky line width without sqrt

	if (dx >= 0)
	{
		if (dy >= 0)
		{
			px = 0.5f;
			if (dx > dy)
				py = -0.5f;
			else if (dx < dy)
				py = 0.5f;
			else
				py = 0.0f;
		}
		else
		{
			py = -0.5f;
			dy = -dy;
			if (dx > dy)
				px = 0.5f;
			else if (dx < dy)
				px = -0.5f;
			else
				px = 0.0f;
		}
	}
	else
	{
		if (dy >= 0)
		{
			py = 0.5f;
			dx = -dx;
			if (dx > dy)
				px = -0.5f;
			else if (dx < dy)
				px = 0.5f;
			else
				px = 0.0f;
		}
		else
		{
			px = -0.5f;
			if (dx > dy)
				py = -0.5f;
			else if (dx < dy)
				py = 0.5f;
			else
				py = 0.0f;
		}
	}

	lx0 = (int16_t)((float)x0 - px);
	lx3 = (int16_t)((float)x0 + py);

	ly0 = (int16_t)((float)y0 - py);
	ly3 = (int16_t)((float)y0 - px);

	lx1 = (int16_t)((float)x1 - py);
	lx2 = (int16_t)((float)x1 + px);

	ly1 = (int16_t)((float)y1 + px);
	ly2 = (int16_t)((float)y1 + py);
}

static void offsetPSX3(void)
{
	lx0 += PSXDisplay.DrawOffset.x;
	ly0 += PSXDisplay.DrawOffset.y;
	lx1 += PSXDisplay.DrawOffset.x;
	ly1 += PSXDisplay.DrawOffset.y;
	lx2 += PSXDisplay.DrawOffset.x;
	ly2 += PSXDisplay.DrawOffset.y;
}

static void offsetPSX4(void)
{
	lx0 += PSXDisplay.DrawOffset.x;
	ly0 += PSXDisplay.DrawOffset.y;
	lx1 += PSXDisplay.DrawOffset.x;
	ly1 += PSXDisplay.DrawOffset.y;
	lx2 += PSXDisplay.DrawOffset.x;
	ly2 += PSXDisplay.DrawOffset.y;
	lx3 += PSXDisplay.DrawOffset.x;
	ly3 += PSXDisplay.DrawOffset.y;
}

static bool offsetline(void)
{
	int16_t x0, x1, y0, y1, dx, dy;
	float px, py;

	if (bDisplayNotSet)
		SetOGLDisplaySettings(1);

	lx0 = (int16_t)(((int32_t)lx0 << SIGNSHIFT) >> SIGNSHIFT);
	lx1 = (int16_t)(((int32_t)lx1 << SIGNSHIFT) >> SIGNSHIFT);
	ly0 = (int16_t)(((int32_t)ly0 << SIGNSHIFT) >> SIGNSHIFT);
	ly1 = (int16_t)(((int32_t)ly1 << SIGNSHIFT) >> SIGNSHIFT);

	if (CheckCoord2())
		return true;

	x0 = (lx0 + PSXDisplay.CumulOffset.x) + 1;
	x1 = (lx1 + PSXDisplay.CumulOffset.x) + 1;
	y0 = (ly0 + PSXDisplay.CumulOffset.y) + 1;
	y1 = (ly1 + PSXDisplay.CumulOffset.y) + 1;

	dx = x1 - x0;
	dy = y1 - y0;

	if (dx >= 0)
	{
		if (dy >= 0)
		{
			px = 0.5f;
			if (dx > dy)
				py = -0.5f;
			else if (dx < dy)
				py = 0.5f;
			else
				py = 0.0f;
		}
		else
		{
			py = -0.5f;
			dy = -dy;
			if (dx > dy)
				px = 0.5f;
			else if (dx < dy)
				px = -0.5f;
			else
				px = 0.0f;
		}
	}
	else
	{
		if (dy >= 0)
		{
			py = 0.5f;
			dx = -dx;
			if (dx > dy)
				px = -0.5f;
			else if (dx < dy)
				px = 0.5f;
			else
				px = 0.0f;
		}
		else
		{
			px = -0.5f;
			if (dx > dy)
				py = -0.5f;
			else if (dx < dy)
				py = 0.5f;
			else
				py = 0.0f;
		}
	}

	vertex[0].x = (int16_t)((float)x0 - px);
	vertex[3].x = (int16_t)((float)x0 + py);

	vertex[0].y = (int16_t)((float)y0 - py);
	vertex[3].y = (int16_t)((float)y0 - px);

	vertex[1].x = (int16_t)((float)x1 - py);
	vertex[2].x = (int16_t)((float)x1 + px);

	vertex[1].y = (int16_t)((float)y1 + px);
	vertex[2].y = (int16_t)((float)y1 + py);

	if (vertex[0].x == vertex[3].x && // ortho rect? done
	    vertex[1].x == vertex[2].x && vertex[0].y == vertex[1].y && vertex[2].y == vertex[3].y)
		return false;
	if (vertex[0].x == vertex[1].x && vertex[2].x == vertex[3].x && vertex[0].y == vertex[3].y &&
	    vertex[1].y == vertex[2].y)
		return false;

	return false;
}

static bool offset3(void)
{
	if (bDisplayNotSet)
		SetOGLDisplaySettings(1);

	lx0 = (int16_t)(((int32_t)lx0 << SIGNSHIFT) >> SIGNSHIFT);
	lx1 = (int16_t)(((int32_t)lx1 << SIGNSHIFT) >> SIGNSHIFT);
	lx2 = (int16_t)(((int32_t)lx2 << SIGNSHIFT) >> SIGNSHIFT);
	ly0 = (int16_t)(((int32_t)ly0 << SIGNSHIFT) >> SIGNSHIFT);
	ly1 = (int16_t)(((int32_t)ly1 << SIGNSHIFT) >> SIGNSHIFT);
	ly2 = (int16_t)(((int32_t)ly2 << SIGNSHIFT) >> SIGNSHIFT);

	if (CheckCoord3())
		return true;

	vertex[0].x = lx0;
	vertex[0].y = ly0;

	vertex[1].x = lx1;
	vertex[1].y = ly1;

	vertex[2].x = lx2;
	vertex[2].y = ly2;

	vertex[0].x += PSXDisplay.CumulOffset.x;
	vertex[1].x += PSXDisplay.CumulOffset.x;
	vertex[2].x += PSXDisplay.CumulOffset.x;
	vertex[0].y += PSXDisplay.CumulOffset.y;
	vertex[1].y += PSXDisplay.CumulOffset.y;
	vertex[2].y += PSXDisplay.CumulOffset.y;

	return false;
}

static bool offset4(void)
{
	if (bDisplayNotSet)
		SetOGLDisplaySettings(1);

	lx0 = (int16_t)(((int32_t)lx0 << SIGNSHIFT) >> SIGNSHIFT);
	lx1 = (int16_t)(((int32_t)lx1 << SIGNSHIFT) >> SIGNSHIFT);
	lx2 = (int16_t)(((int32_t)lx2 << SIGNSHIFT) >> SIGNSHIFT);
	lx3 = (int16_t)(((int32_t)lx3 << SIGNSHIFT) >> SIGNSHIFT);
	ly0 = (int16_t)(((int32_t)ly0 << SIGNSHIFT) >> SIGNSHIFT);
	ly1 = (int16_t)(((int32_t)ly1 << SIGNSHIFT) >> SIGNSHIFT);
	ly2 = (int16_t)(((int32_t)ly2 << SIGNSHIFT) >> SIGNSHIFT);
	ly3 = (int16_t)(((int32_t)ly3 << SIGNSHIFT) >> SIGNSHIFT);

	if (CheckCoord4())
		return true;

	vertex[0].x = lx0;
	vertex[0].y = ly0;

	vertex[1].x = lx1;
	vertex[1].y = ly1;

	vertex[2].x = lx2;
	vertex[2].y = ly2;

	vertex[3].x = lx3;
	vertex[3].y = ly3;

	vertex[0].x += PSXDisplay.CumulOffset.x;
	vertex[1].x += PSXDisplay.CumulOffset.x;
	vertex[2].x += PSXDisplay.CumulOffset.x;
	vertex[3].x += PSXDisplay.CumulOffset.x;
	vertex[0].y += PSXDisplay.CumulOffset.y;
	vertex[1].y += PSXDisplay.CumulOffset.y;
	vertex[2].y += PSXDisplay.CumulOffset.y;
	vertex[3].y += PSXDisplay.CumulOffset.y;

	return false;
}

static void offsetST(void)
{
	if (bDisplayNotSet)
		SetOGLDisplaySettings(1);

	lx0 = (int16_t)(((int32_t)lx0 << SIGNSHIFT) >> SIGNSHIFT);
	ly0 = (int16_t)(((int32_t)ly0 << SIGNSHIFT) >> SIGNSHIFT);

	if (lx0 < -512 && PSXDisplay.DrawOffset.x <= -512)
		lx0 += 2048;

	if (ly0 < -512 && PSXDisplay.DrawOffset.y <= -512)
		ly0 += 2048;

	ly1 = ly0;
	ly2 = ly3 = ly0 + sprtH;
	lx3 = lx0;
	lx1 = lx2 = lx0 + sprtW;

	vertex[0].x = lx0 + PSXDisplay.CumulOffset.x;
	vertex[1].x = lx1 + PSXDisplay.CumulOffset.x;
	vertex[2].x = lx2 + PSXDisplay.CumulOffset.x;
	vertex[3].x = lx3 + PSXDisplay.CumulOffset.x;
	vertex[0].y = ly0 + PSXDisplay.CumulOffset.y;
	vertex[1].y = ly1 + PSXDisplay.CumulOffset.y;
	vertex[2].y = ly2 + PSXDisplay.CumulOffset.y;
	vertex[3].y = ly3 + PSXDisplay.CumulOffset.y;
}

static void offsetScreenUpload(int32_t Position)
{
	if (bDisplayNotSet)
		SetOGLDisplaySettings(1);

	if (Position == -1)
	{
		int32_t lmdx, lmdy;

		lmdx = xrUploadArea.x0;
		lmdy = xrUploadArea.y0;

		lx0 -= lmdx;
		ly0 -= lmdy;
		lx1 -= lmdx;
		ly1 -= lmdy;
		lx2 -= lmdx;
		ly2 -= lmdy;
		lx3 -= lmdx;
		ly3 -= lmdy;
	}
	else if (Position)
	{
		lx0 -= PSXDisplay.DisplayPosition.x;
		ly0 -= PSXDisplay.DisplayPosition.y;
		lx1 -= PSXDisplay.DisplayPosition.x;
		ly1 -= PSXDisplay.DisplayPosition.y;
		lx2 -= PSXDisplay.DisplayPosition.x;
		ly2 -= PSXDisplay.DisplayPosition.y;
		lx3 -= PSXDisplay.DisplayPosition.x;
		ly3 -= PSXDisplay.DisplayPosition.y;
	}
	else
	{
		lx0 -= PreviousPSXDisplay.DisplayPosition.x;
		ly0 -= PreviousPSXDisplay.DisplayPosition.y;
		lx1 -= PreviousPSXDisplay.DisplayPosition.x;
		ly1 -= PreviousPSXDisplay.DisplayPosition.y;
		lx2 -= PreviousPSXDisplay.DisplayPosition.x;
		ly2 -= PreviousPSXDisplay.DisplayPosition.y;
		lx3 -= PreviousPSXDisplay.DisplayPosition.x;
		ly3 -= PreviousPSXDisplay.DisplayPosition.y;
	}

	vertex[0].x = lx0 + PreviousPSXDisplay.Range.x0;
	vertex[1].x = lx1 + PreviousPSXDisplay.Range.x0;
	vertex[2].x = lx2 + PreviousPSXDisplay.Range.x0;
	vertex[3].x = lx3 + PreviousPSXDisplay.Range.x0;
	vertex[0].y = ly0 + PreviousPSXDisplay.Range.y0;
	vertex[1].y = ly1 + PreviousPSXDisplay.Range.y0;
	vertex[2].y = ly2 + PreviousPSXDisplay.Range.y0;
	vertex[3].y = ly3 + PreviousPSXDisplay.Range.y0;
}

static void offsetBlk(void)
{
	if (bDisplayNotSet)
		SetOGLDisplaySettings(1);

	vertex[0].x = lx0 - PSXDisplay.GDrawOffset.x + PreviousPSXDisplay.Range.x0;
	vertex[1].x = lx1 - PSXDisplay.GDrawOffset.x + PreviousPSXDisplay.Range.x0;
	vertex[2].x = lx2 - PSXDisplay.GDrawOffset.x + PreviousPSXDisplay.Range.x0;
	vertex[3].x = lx3 - PSXDisplay.GDrawOffset.x + PreviousPSXDisplay.Range.x0;
	vertex[0].y = ly0 - PSXDisplay.GDrawOffset.y + PreviousPSXDisplay.Range.y0;
	vertex[1].y = ly1 - PSXDisplay.GDrawOffset.y + PreviousPSXDisplay.Range.y0;
	vertex[2].y = ly2 - PSXDisplay.GDrawOffset.y + PreviousPSXDisplay.Range.y0;
	vertex[3].y = ly3 - PSXDisplay.GDrawOffset.y + PreviousPSXDisplay.Range.y0;
}

static void assignTextureVRAMWrite(void)
{
	if (gl_ux[1] == 255)
	{
		vertex[0].sow = (gl_ux[0] * 255.99f) / 255.0f;
		vertex[1].sow = (gl_ux[1] * 255.99f) / 255.0f;
		vertex[2].sow = (gl_ux[2] * 255.99f) / 255.0f;
		vertex[3].sow = (gl_ux[3] * 255.99f) / 255.0f;
	}
	else
	{
		vertex[0].sow = gl_ux[0];
		vertex[1].sow = gl_ux[1];
		vertex[2].sow = gl_ux[2];
		vertex[3].sow = gl_ux[3];
	}

	vertex[0].tow = gl_vy[0];
	vertex[1].tow = gl_vy[1];
	vertex[2].tow = gl_vy[2];
	vertex[3].tow = gl_vy[3];
}

static GLuint gLastTex = 0;

/////////////////////////////////////////////////////////

static void assignTextureSprite(void)
{
	if (bUsingTWin)
	{
		vertex[0].sow = vertex[3].sow = (float)gl_ux[0] / TWin.UScaleFactor;
		vertex[1].sow = vertex[2].sow = (float)sSprite_ux2 / TWin.UScaleFactor;
		vertex[0].tow = vertex[1].tow = (float)gl_vy[0] / TWin.VScaleFactor;
		vertex[2].tow = vertex[3].tow = (float)sSprite_vy2 / TWin.VScaleFactor;
		gLastTex = gTexName;
	}
	else
	{

		vertex[0].sow = vertex[3].sow = gl_ux[0];
		vertex[1].sow = vertex[2].sow = sSprite_ux2;
		vertex[0].tow = vertex[1].tow = gl_vy[0];
		vertex[2].tow = vertex[3].tow = sSprite_vy2;
	}

	if (usMirror & 0x1000)
	{
		vertex[0].sow = vertex[1].sow;
		vertex[1].sow = vertex[2].sow = vertex[3].sow;
		vertex[3].sow = vertex[0].sow;
	}

	if (usMirror & 0x2000)
	{
		vertex[0].tow = vertex[3].tow;
		vertex[2].tow = vertex[3].tow = vertex[1].tow;
		vertex[1].tow = vertex[0].tow;
	}
}

/////////////////////////////////////////////////////////

static void assignTexture3(void)
{
	if (bUsingTWin)
	{
		vertex[0].sow = (float)gl_ux[0] / TWin.UScaleFactor;
		vertex[0].tow = (float)gl_vy[0] / TWin.VScaleFactor;
		vertex[1].sow = (float)gl_ux[1] / TWin.UScaleFactor;
		vertex[1].tow = (float)gl_vy[1] / TWin.VScaleFactor;
		vertex[2].sow = (float)gl_ux[2] / TWin.UScaleFactor;
		vertex[2].tow = (float)gl_vy[2] / TWin.VScaleFactor;
		gLastTex = gTexName;
	}
	else
	{
		vertex[0].sow = gl_ux[0];
		vertex[0].tow = gl_vy[0];
		vertex[1].sow = gl_ux[1];
		vertex[1].tow = gl_vy[1];
		vertex[2].sow = gl_ux[2];
		vertex[2].tow = gl_vy[2];
	}
}

/////////////////////////////////////////////////////////

static void assignTexture4(void)
{
	if (bUsingTWin)
	{
		vertex[0].sow = (float)gl_ux[0] / TWin.UScaleFactor;
		vertex[0].tow = (float)gl_vy[0] / TWin.VScaleFactor;
		vertex[1].sow = (float)gl_ux[1] / TWin.UScaleFactor;
		vertex[1].tow = (float)gl_vy[1] / TWin.VScaleFactor;
		vertex[2].sow = (float)gl_ux[2] / TWin.UScaleFactor;
		vertex[2].tow = (float)gl_vy[2] / TWin.VScaleFactor;
		vertex[3].sow = (float)gl_ux[3] / TWin.UScaleFactor;
		vertex[3].tow = (float)gl_vy[3] / TWin.VScaleFactor;
		gLastTex = gTexName;
	}
	else
	{
		vertex[0].sow = gl_ux[0];
		vertex[0].tow = gl_vy[0];
		vertex[1].sow = gl_ux[1];
		vertex[1].tow = gl_vy[1];
		vertex[2].sow = gl_ux[2];
		vertex[2].tow = gl_vy[2];
		vertex[3].sow = gl_ux[3];
		vertex[3].tow = gl_vy[3];
	}
}

////////////////////////////////////////////////////////////////////////
// Update global TP infos
////////////////////////////////////////////////////////////////////////

static void UpdateGlobalTP(uint16_t gdata)
{
	GlobalTextAddrX = (gdata << 6) & 0x3c0;

	if (iGPUHeight == 1024) // ZN mode
	{
		if (dwGPUVersion == 2) // very special zn gpu
		{
			GlobalTextAddrY = ((gdata & 0x60) << 3);
			GlobalTextIL = (gdata & 0x2000) >> 13;
			GlobalTextABR = (uint16_t)((gdata >> 7) & 0x3);
			GlobalTextTP = (gdata >> 9) & 0x3;
			if (GlobalTextTP == 3)
				GlobalTextTP = 2;
			GlobalTexturePage = (GlobalTextAddrX >> 6) + (GlobalTextAddrY >> 4);
			usMirror = 0;
			STATUSREG = (STATUSREG & 0xffffe000) | (gdata & 0x1fff);
			return;
		}
		else // "enhanced" psx gpu
		{
			GlobalTextAddrY = (uint16_t)(((gdata << 4) & 0x100) | ((gdata >> 2) & 0x200));
		}
	}
	else
		GlobalTextAddrY = (gdata << 4) & 0x100; // "normal" psx gpu

	usMirror = gdata & 0x3000;

	GlobalTextTP = (gdata >> 7) & 0x3; // tex mode (4,8,15)
	if (GlobalTextTP == 3)
		GlobalTextTP = 2;               // seen in Wild9 :(
	GlobalTextABR = (gdata >> 5) & 0x3; // blend mode

	GlobalTexturePage = (GlobalTextAddrX >> 6) + (GlobalTextAddrY >> 4);

	STATUSREG &= ~0x07ff;          // Clear the necessary bits
	STATUSREG |= (gdata & 0x07ff); // set the necessary bits
}

static uint32_t DoubleBGR2RGB(uint32_t BGR)
{
	uint32_t ebx, eax, edx;

	ebx = (BGR & 0x000000ff) << 1;
	if (ebx & 0x00000100)
		ebx = 0x000000ff;

	eax = (BGR & 0x0000ff00) << 1;
	if (eax & 0x00010000)
		eax = 0x0000ff00;

	edx = (BGR & 0x00ff0000) << 1;
	if (edx & 0x01000000)
		edx = 0x00ff0000;

	return (ebx | eax | edx);
}

static uint16_t BGR24to16(uint32_t BGR)
{
	return ((BGR >> 3) & 0x1f) | ((BGR & 0xf80000) >> 9) | ((BGR & 0xf800) >> 6);
}

////////////////////////////////////////////////////////////////////////
// OpenGL primitive drawing commands
////////////////////////////////////////////////////////////////////////

static __inline void PRIMdrawMain(GLenum mode, int32_t howMany, bool enableColor, bool enableVertex, bool enableTextureCoord, OGLVertex *vertex1, OGLVertex *vertex2, OGLVertex *vertex3, OGLVertex *vertex4)
{
	if (enableColor)
		glEnableClientState(GL_COLOR_ARRAY);
	else
		glDisableClientState(GL_COLOR_ARRAY);
	if (enableVertex)
		glEnableClientState(GL_VERTEX_ARRAY);
	else
		glDisableClientState(GL_VERTEX_ARRAY);
	if (enableTextureCoord)
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	else
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	OGLVertex vertexToDraw[4];
	vertexToDraw[0] = *vertex1;
	vertexToDraw[1] = *vertex2;
	vertexToDraw[2] = *vertex3;
	if (howMany == 4)
		vertexToDraw[3] = *vertex4;

	if (enableColor)
		glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(vertexToDraw[0]), &vertexToDraw[0].c.col[0]);
	if (enableVertex)
		glVertexPointer(3, GL_FLOAT, sizeof(vertexToDraw[0]), &vertexToDraw[0].x);
	if (enableTextureCoord)
		glTexCoordPointer(2, GL_FLOAT, sizeof(vertexToDraw[0]), &vertexToDraw[0].sow);
	
	glDrawArrays(mode, 0, howMany);
}

static __inline void PRIMdrawTexturedQuad(OGLVertex *vertex1, OGLVertex *vertex2, OGLVertex *vertex3,
                                          OGLVertex *vertex4)
{
	PRIMdrawMain(GL_TRIANGLE_STRIP, 4, false, true, true, vertex1, vertex2, vertex4, vertex3);
}

static __inline void PRIMdrawTexturedTri(OGLVertex *vertex1, OGLVertex *vertex2, OGLVertex *vertex3)
{
	PRIMdrawMain(GL_TRIANGLES, 3, false, true, true, vertex1, vertex2, vertex3, NULL);
}

static __inline void PRIMdrawTexGouraudTriColor(OGLVertex *vertex1, OGLVertex *vertex2, OGLVertex *vertex3)
{
	PRIMdrawMain(GL_TRIANGLES, 3, true, true, true, vertex1, vertex2, vertex3, NULL);
}

static __inline void PRIMdrawTexGouraudTriColorQuad(OGLVertex *vertex1, OGLVertex *vertex2, OGLVertex *vertex3,
                                                    OGLVertex *vertex4)
{
	PRIMdrawMain(GL_TRIANGLE_STRIP, 4, true, true, true, vertex1, vertex2, vertex4, vertex3);
}

static __inline void PRIMdrawTri(OGLVertex *vertex1, OGLVertex *vertex2, OGLVertex *vertex3)
{
	PRIMdrawMain(GL_TRIANGLES, 3, false, true, false, vertex1, vertex2, vertex3, NULL);
}

static __inline void PRIMdrawTri2(OGLVertex *vertex1, OGLVertex *vertex2, OGLVertex *vertex3, OGLVertex *vertex4)
{
	PRIMdrawMain(GL_TRIANGLE_STRIP, 4, false, true, false, vertex1, vertex3, vertex2, vertex4);
}

static __inline void PRIMdrawGouraudTriColor(OGLVertex *vertex1, OGLVertex *vertex2, OGLVertex *vertex3)
{
	PRIMdrawMain(GL_TRIANGLE_STRIP, 3, true, true, false, vertex1, vertex2, vertex3, NULL);
}

static __inline void PRIMdrawGouraudTri2Color(OGLVertex *vertex1, OGLVertex *vertex2, OGLVertex *vertex3,
                                              OGLVertex *vertex4)
{
	PRIMdrawMain(GL_TRIANGLE_STRIP, 4, true, true, false, vertex1, vertex3, vertex2, vertex4);
}

static __inline void PRIMdrawFlatLine(OGLVertex *vertex1, OGLVertex *vertex2, OGLVertex *vertex3, OGLVertex *vertex4)
{
	glColor4ubv(vertex1->c.col);
	PRIMdrawMain(GL_QUADS, 4, false, true, false, vertex1, vertex2, vertex3, vertex4);
}

static __inline void PRIMdrawGouraudLine(OGLVertex *vertex1, OGLVertex *vertex2, OGLVertex *vertex3, OGLVertex *vertex4)
{
	PRIMdrawMain(GL_QUADS, 4, true, true, false, vertex1, vertex2, vertex3, vertex4);
}

static __inline void PRIMdrawQuad(OGLVertex *vertex1, OGLVertex *vertex2, OGLVertex *vertex3, OGLVertex *vertex4)
{
	PRIMdrawMain(GL_QUADS, 4, false, true, false, vertex1, vertex2, vertex3, vertex4);
}

////////////////////////////////////////////////////////////////////////
// Transparent blending settings
////////////////////////////////////////////////////////////////////////

static GLenum obm1 = GL_ZERO;
static GLenum obm2 = GL_ZERO;

typedef struct SEMITRANSTAG
{
	GLenum srcFac;
	GLenum dstFac;
	GLubyte alpha;
} SemiTransParams;

static SemiTransParams TransSets[4] = {{GL_SRC_ALPHA, GL_SRC_ALPHA, 127},
                                       {GL_ONE, GL_ONE, 255},
                                       {GL_ZERO, GL_ONE_MINUS_SRC_COLOR, 255},
                                       {GL_ONE_MINUS_SRC_ALPHA, GL_ONE, 192}};

////////////////////////////////////////////////////////////////////////

static void SetSemiTrans(void)
{
	/*
	* 0.5 x B + 0.5 x F
	* 1.0 x B + 1.0 x F
	* 1.0 x B - 1.0 x F
	* 1.0 x B +0.25 x F
	*/

	if (!DrawSemiTrans) // no semi trans at all?
	{
		if (bBlendEnable)
		{
			glDisable(GL_BLEND);
			bBlendEnable = false;
		}                                 // -> don't wanna blend
		ubGloAlpha = ubGloColAlpha = 255; // -> full alpha
		return;                           // -> and bye
	}

	ubGloAlpha = ubGloColAlpha = TransSets[GlobalTextABR].alpha;

	if (!bBlendEnable)
	{
		glEnable(GL_BLEND);
		bBlendEnable = true;
	} // wanna blend

	if (TransSets[GlobalTextABR].srcFac != obm1 || TransSets[GlobalTextABR].dstFac != obm2)
	{
		obm1 = TransSets[GlobalTextABR].srcFac;
		obm2 = TransSets[GlobalTextABR].dstFac;
		glBlendFunc(obm1, obm2); // set blend func
	}
}

////////////////////////////////////////////////////////////////////////
// multi pass in old 'Advanced blending' mode... got it from Lewpy :)
////////////////////////////////////////////////////////////////////////

static SemiTransParams MultiTexTransSets[4][2] = {
    {{GL_ONE, GL_SRC_ALPHA, 127}, {GL_SRC_ALPHA, GL_ONE, 127}},
    {{GL_ONE, GL_SRC_ALPHA, 255}, {GL_SRC_ALPHA, GL_ONE, 255}},
    {{GL_ZERO, GL_ONE_MINUS_SRC_COLOR, 255}, {GL_ZERO, GL_ONE_MINUS_SRC_COLOR, 255}},
    {{GL_SRC_ALPHA, GL_ONE, 127}, {GL_ONE_MINUS_SRC_ALPHA, GL_ONE, 255}}};

////////////////////////////////////////////////////////////////////////

static SemiTransParams MultiColTransSets[4] = {{GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, 127},
                                               {GL_ONE, GL_ONE, 255},
                                               {GL_ZERO, GL_ONE_MINUS_SRC_COLOR, 255},
                                               {GL_SRC_ALPHA, GL_ONE, 127}};

////////////////////////////////////////////////////////////////////////

static void SetSemiTransMulti(int32_t Pass)
{
	static GLenum bm1 = GL_ZERO;
	static GLenum bm2 = GL_ONE;

	ubGloAlpha = 255;
	ubGloColAlpha = 255;

	// are we enabling SemiTransparent mode?
	if (DrawSemiTrans)
	{
		if (bDrawTextured)
		{
			bm1 = MultiTexTransSets[GlobalTextABR][Pass].srcFac;
			bm2 = MultiTexTransSets[GlobalTextABR][Pass].dstFac;
			ubGloAlpha = MultiTexTransSets[GlobalTextABR][Pass].alpha;
		}
		// no texture
		else
		{
			bm1 = MultiColTransSets[GlobalTextABR].srcFac;
			bm2 = MultiColTransSets[GlobalTextABR].dstFac;
			ubGloColAlpha = MultiColTransSets[GlobalTextABR].alpha;
		}
	}
	// no shading
	else
	{
		if (Pass == 0)
		{
			// disable blending
			bm1 = GL_ONE;
			bm2 = GL_ZERO;
		}
		else
		{
			// disable blending, but add src col a second time
			bm1 = GL_ONE;
			bm2 = GL_ONE;
		}
	}

	if (!bBlendEnable)
	{
		glEnable(GL_BLEND);
		bBlendEnable = true;
	} // wanna blend

	if (bm1 != obm1 || bm2 != obm2)
	{
		glBlendFunc(bm1, bm2); // set blend func
		obm1 = bm1;
		obm2 = bm2;
	}
}

////////////////////////////////////////////////////////////////////////

static __inline void SetRenderState(uint32_t DrawAttributes)
{
	bDrawNonShaded = (SHADETEXBIT(DrawAttributes)) ? true : false;
	DrawSemiTrans = (SEMITRANSBIT(DrawAttributes)) ? true : false;
}

////////////////////////////////////////////////////////////////////////

static __inline void SetRenderColor(uint32_t DrawAttributes)
{
	if (bDrawNonShaded)
	{
		g_m1 = g_m2 = g_m3 = 128;
	}
	else
	{
		g_m1 = DrawAttributes & 0xff;
		g_m2 = (DrawAttributes >> 8) & 0xff;
		g_m3 = (DrawAttributes >> 16) & 0xff;
	}
}

////////////////////////////////////////////////////////////////////////

static void SetRenderMode(uint32_t DrawAttributes, bool bSCol)
{
	{
		bDrawMultiPass = false;
		SetSemiTrans();
	}

	if (bDrawTextured) // texture ? build it/get it from cache
	{
		GLuint currTex;
		if (bUsingTWin)
			currTex = LoadTextureWnd(GlobalTexturePage, GlobalTextTP, ulClutID);
		else if (bUsingMovie)
			currTex = LoadTextureMovie();
		else
			currTex = SelectSubTextureS(GlobalTextTP, ulClutID);

		if (gTexName != currTex)
		{
			gTexName = currTex;
			glBindTexture(GL_TEXTURE_2D, currTex);
		}

		if (!bTexEnabled) // -> turn texturing on
		{
			bTexEnabled = true;
			glEnable(GL_TEXTURE_2D);
		}
	}
	else // no texture ?
	    if (bTexEnabled)
	{
		bTexEnabled = false;
		glDisable(GL_TEXTURE_2D);
	} // -> turn texturing off

	if (bSCol) // also set color ?
	{

		if (bDrawNonShaded) // -> non shaded?
		{
			vertex[0].c.lcol = 0xffffff;
		}
		else // -> shaded?
		{
			vertex[0].c.lcol = DoubleBGR2RGB(DrawAttributes);
		}
		vertex[0].c.col[3] = ubGloAlpha; // -> set color with
		glColor4ubv(vertex[0].c.col);    //    texture alpha
	}

	if (bDrawSmoothShaded != bOldSmoothShaded) // shading changed?
	{
		if (bDrawSmoothShaded)
			glShadeModel(GL_SMOOTH); // -> set actual shading
		else
			glShadeModel(GL_FLAT);
		bOldSmoothShaded = bDrawSmoothShaded;
	}
}

////////////////////////////////////////////////////////////////////////
// Fucking stupid screen coord checking
////////////////////////////////////////////////////////////////////////

static bool ClipVertexListScreen(void)
{
	if (lx0 >= PSXDisplay.DisplayEnd.x)
		goto NEXTSCRTEST;
	if (ly0 >= PSXDisplay.DisplayEnd.y)
		goto NEXTSCRTEST;
	if (lx2 < PSXDisplay.DisplayPosition.x)
		goto NEXTSCRTEST;
	if (ly2 < PSXDisplay.DisplayPosition.y)
		goto NEXTSCRTEST;

	return true;

NEXTSCRTEST:
	if (PSXDisplay.InterlacedTest)
		return false;

	if (lx0 >= PreviousPSXDisplay.DisplayEnd.x)
		return false;
	if (ly0 >= PreviousPSXDisplay.DisplayEnd.y)
		return false;
	if (lx2 < PreviousPSXDisplay.DisplayPosition.x)
		return false;
	if (ly2 < PreviousPSXDisplay.DisplayPosition.y)
		return false;

	return true;
}

////////////////////////////////////////////////////////////////////////

static bool bDrawOffscreenFront(void)
{
	if (sxmin < PSXDisplay.DisplayPosition.x)
		return false; // must be complete in front
	if (symin < PSXDisplay.DisplayPosition.y)
		return false;
	if (sxmax > PSXDisplay.DisplayEnd.x)
		return false;
	if (symax > PSXDisplay.DisplayEnd.y)
		return false;
	return true;
}

static bool bOnePointInBack(void)
{
	if (sxmax < PreviousPSXDisplay.DisplayPosition.x)
		return false;

	if (symax < PreviousPSXDisplay.DisplayPosition.y)
		return false;

	if (sxmin >= PreviousPSXDisplay.DisplayEnd.x)
		return false;

	if (symin >= PreviousPSXDisplay.DisplayEnd.y)
		return false;

	return true;
}

static bool bDrawOffscreen4(void)
{
	bool bFront;
	int16_t sW, sH;

	sxmax = max(lx0, max(lx1, max(lx2, lx3)));
	if (sxmax < drawX)
		return false;
	sxmin = min(lx0, min(lx1, min(lx2, lx3)));
	if (sxmin > drawW)
		return false;
	symax = max(ly0, max(ly1, max(ly2, ly3)));
	if (symax < drawY)
		return false;
	symin = min(ly0, min(ly1, min(ly2, ly3)));
	if (symin > drawH)
		return false;

	if (PSXDisplay.Disabled)
		return true; // disabled? ever

	sW = drawW - 1;
	sH = drawH - 1;

	sxmin = min(sW, max(sxmin, drawX));
	sxmax = max(drawX, min(sxmax, sW));
	symin = min(sH, max(symin, drawY));
	symax = max(drawY, min(symax, sH));

	if (bOnePointInBack())
		return bFullVRam;

	bFront = bDrawOffscreenFront();

	if (bFront)
	{
		if (PSXDisplay.InterlacedTest)
			return bFullVRam; // -> ok, no need for adjust

		vertex[0].x = lx0 - PSXDisplay.DisplayPosition.x + PreviousPSXDisplay.Range.x0;
		vertex[1].x = lx1 - PSXDisplay.DisplayPosition.x + PreviousPSXDisplay.Range.x0;
		vertex[2].x = lx2 - PSXDisplay.DisplayPosition.x + PreviousPSXDisplay.Range.x0;
		vertex[3].x = lx3 - PSXDisplay.DisplayPosition.x + PreviousPSXDisplay.Range.x0;
		vertex[0].y = ly0 - PSXDisplay.DisplayPosition.y + PreviousPSXDisplay.Range.y0;
		vertex[1].y = ly1 - PSXDisplay.DisplayPosition.y + PreviousPSXDisplay.Range.y0;
		vertex[2].y = ly2 - PSXDisplay.DisplayPosition.y + PreviousPSXDisplay.Range.y0;
		vertex[3].y = ly3 - PSXDisplay.DisplayPosition.y + PreviousPSXDisplay.Range.y0;

		return bFullVRam; // -> but no od
	}

	return true;
}

////////////////////////////////////////////////////////////////////////

static bool bDrawOffscreen3(void)
{
	bool bFront;
	int16_t sW, sH;

	sxmax = max(lx0, max(lx1, lx2));
	if (sxmax < drawX)
		return false;
	sxmin = min(lx0, min(lx1, lx2));
	if (sxmin > drawW)
		return false;
	symax = max(ly0, max(ly1, ly2));
	if (symax < drawY)
		return false;
	symin = min(ly0, min(ly1, ly2));
	if (symin > drawH)
		return false;

	if (PSXDisplay.Disabled)
		return true; // disabled? ever

	sW = drawW - 1;
	sH = drawH - 1;
	sxmin = min(sW, max(sxmin, drawX));
	sxmax = max(drawX, min(sxmax, sW));
	symin = min(sH, max(symin, drawY));
	symax = max(drawY, min(symax, sH));

	if (bOnePointInBack())
		return bFullVRam;

	bFront = bDrawOffscreenFront();

	if (bFront)
	{
		if (PSXDisplay.InterlacedTest)
			return bFullVRam; // -> ok, no need for adjust

		vertex[0].x = lx0 - PSXDisplay.DisplayPosition.x + PreviousPSXDisplay.Range.x0;
		vertex[1].x = lx1 - PSXDisplay.DisplayPosition.x + PreviousPSXDisplay.Range.x0;
		vertex[2].x = lx2 - PSXDisplay.DisplayPosition.x + PreviousPSXDisplay.Range.x0;
		vertex[0].y = ly0 - PSXDisplay.DisplayPosition.y + PreviousPSXDisplay.Range.y0;
		vertex[1].y = ly1 - PSXDisplay.DisplayPosition.y + PreviousPSXDisplay.Range.y0;
		vertex[2].y = ly2 - PSXDisplay.DisplayPosition.y + PreviousPSXDisplay.Range.y0;

		return bFullVRam; // -> but no od
	}

	return true;
}

////////////////////////////////////////////////////////////////////////

bool FastCheckAgainstScreen(int16_t imageX0, int16_t imageY0, int16_t imageX1, int16_t imageY1)
{
	PSXRect_t xUploadArea;

	imageX1 += imageX0;
	imageY1 += imageY0;

	if (imageX0 < PreviousPSXDisplay.DisplayPosition.x)
		xUploadArea.x0 = PreviousPSXDisplay.DisplayPosition.x;
	else if (imageX0 > PreviousPSXDisplay.DisplayEnd.x)
		xUploadArea.x0 = PreviousPSXDisplay.DisplayEnd.x;
	else
		xUploadArea.x0 = imageX0;

	if (imageX1 < PreviousPSXDisplay.DisplayPosition.x)
		xUploadArea.x1 = PreviousPSXDisplay.DisplayPosition.x;
	else if (imageX1 > PreviousPSXDisplay.DisplayEnd.x)
		xUploadArea.x1 = PreviousPSXDisplay.DisplayEnd.x;
	else
		xUploadArea.x1 = imageX1;

	if (imageY0 < PreviousPSXDisplay.DisplayPosition.y)
		xUploadArea.y0 = PreviousPSXDisplay.DisplayPosition.y;
	else if (imageY0 > PreviousPSXDisplay.DisplayEnd.y)
		xUploadArea.y0 = PreviousPSXDisplay.DisplayEnd.y;
	else
		xUploadArea.y0 = imageY0;

	if (imageY1 < PreviousPSXDisplay.DisplayPosition.y)
		xUploadArea.y1 = PreviousPSXDisplay.DisplayPosition.y;
	else if (imageY1 > PreviousPSXDisplay.DisplayEnd.y)
		xUploadArea.y1 = PreviousPSXDisplay.DisplayEnd.y;
	else
		xUploadArea.y1 = imageY1;

	if ((xUploadArea.x0 != xUploadArea.x1) && (xUploadArea.y0 != xUploadArea.y1))
		return true;
	else
		return false;
}

bool CheckAgainstScreen(int16_t imageX0, int16_t imageY0, int16_t imageX1, int16_t imageY1)
{
	imageX1 += imageX0;
	imageY1 += imageY0;

	if (imageX0 < PreviousPSXDisplay.DisplayPosition.x)
		xrUploadArea.x0 = PreviousPSXDisplay.DisplayPosition.x;
	else if (imageX0 > PreviousPSXDisplay.DisplayEnd.x)
		xrUploadArea.x0 = PreviousPSXDisplay.DisplayEnd.x;
	else
		xrUploadArea.x0 = imageX0;

	if (imageX1 < PreviousPSXDisplay.DisplayPosition.x)
		xrUploadArea.x1 = PreviousPSXDisplay.DisplayPosition.x;
	else if (imageX1 > PreviousPSXDisplay.DisplayEnd.x)
		xrUploadArea.x1 = PreviousPSXDisplay.DisplayEnd.x;
	else
		xrUploadArea.x1 = imageX1;

	if (imageY0 < PreviousPSXDisplay.DisplayPosition.y)
		xrUploadArea.y0 = PreviousPSXDisplay.DisplayPosition.y;
	else if (imageY0 > PreviousPSXDisplay.DisplayEnd.y)
		xrUploadArea.y0 = PreviousPSXDisplay.DisplayEnd.y;
	else
		xrUploadArea.y0 = imageY0;

	if (imageY1 < PreviousPSXDisplay.DisplayPosition.y)
		xrUploadArea.y1 = PreviousPSXDisplay.DisplayPosition.y;
	else if (imageY1 > PreviousPSXDisplay.DisplayEnd.y)
		xrUploadArea.y1 = PreviousPSXDisplay.DisplayEnd.y;
	else
		xrUploadArea.y1 = imageY1;

	if ((xrUploadArea.x0 != xrUploadArea.x1) && (xrUploadArea.y0 != xrUploadArea.y1))
		return true;
	else
		return false;
}

bool FastCheckAgainstFrontScreen(int16_t imageX0, int16_t imageY0, int16_t imageX1, int16_t imageY1)
{
	PSXRect_t xUploadArea;

	imageX1 += imageX0;
	imageY1 += imageY0;

	if (imageX0 < PSXDisplay.DisplayPosition.x)
		xUploadArea.x0 = PSXDisplay.DisplayPosition.x;
	else if (imageX0 > PSXDisplay.DisplayEnd.x)
		xUploadArea.x0 = PSXDisplay.DisplayEnd.x;
	else
		xUploadArea.x0 = imageX0;

	if (imageX1 < PSXDisplay.DisplayPosition.x)
		xUploadArea.x1 = PSXDisplay.DisplayPosition.x;
	else if (imageX1 > PSXDisplay.DisplayEnd.x)
		xUploadArea.x1 = PSXDisplay.DisplayEnd.x;
	else
		xUploadArea.x1 = imageX1;

	if (imageY0 < PSXDisplay.DisplayPosition.y)
		xUploadArea.y0 = PSXDisplay.DisplayPosition.y;
	else if (imageY0 > PSXDisplay.DisplayEnd.y)
		xUploadArea.y0 = PSXDisplay.DisplayEnd.y;
	else
		xUploadArea.y0 = imageY0;

	if (imageY1 < PSXDisplay.DisplayPosition.y)
		xUploadArea.y1 = PSXDisplay.DisplayPosition.y;
	else if (imageY1 > PSXDisplay.DisplayEnd.y)
		xUploadArea.y1 = PSXDisplay.DisplayEnd.y;
	else
		xUploadArea.y1 = imageY1;

	if ((xUploadArea.x0 != xUploadArea.x1) && (xUploadArea.y0 != xUploadArea.y1))
		return true;
	else
		return false;
}

bool CheckAgainstFrontScreen(int16_t imageX0, int16_t imageY0, int16_t imageX1, int16_t imageY1)
{
	imageX1 += imageX0;
	imageY1 += imageY0;

	if (imageX0 < PSXDisplay.DisplayPosition.x)
		xrUploadArea.x0 = PSXDisplay.DisplayPosition.x;
	else if (imageX0 > PSXDisplay.DisplayEnd.x)
		xrUploadArea.x0 = PSXDisplay.DisplayEnd.x;
	else
		xrUploadArea.x0 = imageX0;

	if (imageX1 < PSXDisplay.DisplayPosition.x)
		xrUploadArea.x1 = PSXDisplay.DisplayPosition.x;
	else if (imageX1 > PSXDisplay.DisplayEnd.x)
		xrUploadArea.x1 = PSXDisplay.DisplayEnd.x;
	else
		xrUploadArea.x1 = imageX1;

	if (imageY0 < PSXDisplay.DisplayPosition.y)
		xrUploadArea.y0 = PSXDisplay.DisplayPosition.y;
	else if (imageY0 > PSXDisplay.DisplayEnd.y)
		xrUploadArea.y0 = PSXDisplay.DisplayEnd.y;
	else
		xrUploadArea.y0 = imageY0;

	if (imageY1 < PSXDisplay.DisplayPosition.y)
		xrUploadArea.y1 = PSXDisplay.DisplayPosition.y;
	else if (imageY1 > PSXDisplay.DisplayEnd.y)
		xrUploadArea.y1 = PSXDisplay.DisplayEnd.y;
	else
		xrUploadArea.y1 = imageY1;

	if ((xrUploadArea.x0 != xrUploadArea.x1) && (xrUploadArea.y0 != xrUploadArea.y1))
		return true;
	else
		return false;
}

////////////////////////////////////////////////////////////////////////

void PrepareFullScreenUpload(int32_t Position)
{
	if (Position == -1) // rgb24
	{
		if (PSXDisplay.Interlaced)
		{
			xrUploadArea.x0 = PSXDisplay.DisplayPosition.x;
			xrUploadArea.x1 = PSXDisplay.DisplayEnd.x;
			xrUploadArea.y0 = PSXDisplay.DisplayPosition.y;
			xrUploadArea.y1 = PSXDisplay.DisplayEnd.y;
		}
		else
		{
			xrUploadArea.x0 = PreviousPSXDisplay.DisplayPosition.x;
			xrUploadArea.x1 = PreviousPSXDisplay.DisplayEnd.x;
			xrUploadArea.y0 = PreviousPSXDisplay.DisplayPosition.y;
			xrUploadArea.y1 = PreviousPSXDisplay.DisplayEnd.y;
		}

		if (bNeedRGB24Update)
		{
			if (lClearOnSwap)
			{
				//       lClearOnSwap=0;
			}
			else if (PSXDisplay.Interlaced &&
			         PreviousPSXDisplay.RGB24 < 2) // in interlaced mode we upload at least two full frames (GT1 menu)
			{
				PreviousPSXDisplay.RGB24++;
			}
			else
			{
				xrUploadArea.y1 = min(xrUploadArea.y0 + xrUploadAreaRGB24.y1, xrUploadArea.y1);
				xrUploadArea.y0 += xrUploadAreaRGB24.y0;
			}
		}
	}
	else if (Position)
	{
		xrUploadArea.x0 = PSXDisplay.DisplayPosition.x;
		xrUploadArea.x1 = PSXDisplay.DisplayEnd.x;
		xrUploadArea.y0 = PSXDisplay.DisplayPosition.y;
		xrUploadArea.y1 = PSXDisplay.DisplayEnd.y;
	}
	else
	{
		xrUploadArea.x0 = PreviousPSXDisplay.DisplayPosition.x;
		xrUploadArea.x1 = PreviousPSXDisplay.DisplayEnd.x;
		xrUploadArea.y0 = PreviousPSXDisplay.DisplayPosition.y;
		xrUploadArea.y1 = PreviousPSXDisplay.DisplayEnd.y;
	}

	if (xrUploadArea.x0 < 0)
		xrUploadArea.x0 = 0;
	else if (xrUploadArea.x0 > 1023)
		xrUploadArea.x0 = 1023;

	if (xrUploadArea.x1 < 0)
		xrUploadArea.x1 = 0;
	else if (xrUploadArea.x1 > 1024)
		xrUploadArea.x1 = 1024;

	if (xrUploadArea.y0 < 0)
		xrUploadArea.y0 = 0;
	else if (xrUploadArea.y0 > iGPUHeightMask)
		xrUploadArea.y0 = iGPUHeightMask;

	if (xrUploadArea.y1 < 0)
		xrUploadArea.y1 = 0;
	else if (xrUploadArea.y1 > iGPUHeight)
		xrUploadArea.y1 = iGPUHeight;

	if (PSXDisplay.RGB24)
	{
		InvalidateTextureArea(xrUploadArea.x0, xrUploadArea.y0, xrUploadArea.x1 - xrUploadArea.x0,
		                      xrUploadArea.y1 - xrUploadArea.y0);
	}
}

////////////////////////////////////////////////////////////////////////

void UploadScreen(int32_t Position)
{
	int16_t x, y, YStep, XStep, U, s, UStep, ux[4], vy[4];
	int16_t xa, xb, ya, yb;

	if (xrUploadArea.x0 > 1023)
		xrUploadArea.x0 = 1023;
	if (xrUploadArea.x1 > 1024)
		xrUploadArea.x1 = 1024;
	if (xrUploadArea.y0 > iGPUHeightMask)
		xrUploadArea.y0 = iGPUHeightMask;
	if (xrUploadArea.y1 > iGPUHeight)
		xrUploadArea.y1 = iGPUHeight;

	if (xrUploadArea.x0 == xrUploadArea.x1)
		return;
	if (xrUploadArea.y0 == xrUploadArea.y1)
		return;

	if (PSXDisplay.Disabled)
		return;

	iDrawnSomething = 2;
	iLastRGB24 = PSXDisplay.RGB24 + 1;

	bUsingMovie = true;
	bDrawTextured = true; // just doing textures
	bDrawSmoothShaded = false;

	vertex[0].c.lcol = 0xffffffff;
	glColor4ubv(vertex[0].c.col);

	SetOGLDisplaySettings(0);

	YStep = 256; // max texture size
	XStep = 256;

	UStep = (PSXDisplay.RGB24 ? 128 : 0);

	ya = xrUploadArea.y0;
	yb = xrUploadArea.y1;
	xa = xrUploadArea.x0;
	xb = xrUploadArea.x1;

	for (y = ya; y <= yb; y += YStep) // loop y
	{
		U = 0;
		for (x = xa; x <= xb; x += XStep) // loop x
		{
			ly0 = ly1 = y; // -> get y coords
			ly2 = y + YStep;
			if (ly2 > yb)
				ly2 = yb;
			ly3 = ly2;

			lx0 = lx3 = x; // -> get x coords
			lx1 = x + XStep;
			if (lx1 > xb)
				lx1 = xb;

			lx2 = lx1;

			ux[0] = ux[3] = (xa - x); // -> set tex x coords
			if (ux[0] < 0)
				ux[0] = ux[3] = 0;
			ux[2] = ux[1] = (xb - x);
			if (ux[2] > 256)
				ux[2] = ux[1] = 256;

			vy[0] = vy[1] = (ya - y); // -> set tex y coords
			if (vy[0] < 0)
				vy[0] = vy[1] = 0;
			vy[2] = vy[3] = (yb - y);
			if (vy[2] > 256)
				vy[2] = vy[3] = 256;

			if ((ux[0] >= ux[2]) || // -> cheaters never win...
			    (vy[0] >= vy[2]))
				continue; //    (but winners always cheat...)

			xrMovieArea.x0 = lx0 + U;
			xrMovieArea.y0 = ly0;
			xrMovieArea.x1 = lx2 + U;
			xrMovieArea.y1 = ly2;

			s = ux[2] - ux[0];
			if (s > 255)
				s = 255;

			gl_ux[2] = gl_ux[1] = s;
			s = vy[2] - vy[0];
			if (s > 255)
				s = 255;
			gl_vy[2] = gl_vy[3] = s;
			gl_ux[0] = gl_ux[3] = gl_vy[0] = gl_vy[1] = 0;

			SetRenderState((uint32_t)0x01000000);
			SetRenderMode((uint32_t)0x01000000, false); // upload texture data
			offsetScreenUpload(Position);
			assignTextureVRAMWrite();

			PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);

			U += UStep;
		}
	}

	bUsingMovie = false; // done...
	bDisplayNotSet = true;
}

////////////////////////////////////////////////////////////////////////
// Detect next screen
////////////////////////////////////////////////////////////////////////

static bool IsCompleteInsideNextScreen(int16_t x, int16_t y, int16_t xoff, int16_t yoff)
{
	if (x > PSXDisplay.DisplayPosition.x + 1)
		return false;
	if ((x + xoff) < PSXDisplay.DisplayEnd.x - 1)
		return false;
	yoff += y;
	if (y >= PSXDisplay.DisplayPosition.y && y <= PSXDisplay.DisplayEnd.y)
	{
		if ((yoff) >= PSXDisplay.DisplayPosition.y && (yoff) <= PSXDisplay.DisplayEnd.y)
			return true;
	}
	if (y > PSXDisplay.DisplayPosition.y + 1)
		return false;
	if (yoff < PSXDisplay.DisplayEnd.y - 1)
		return false;
	return true;
}

static bool IsPrimCompleteInsideNextScreen(int16_t x, int16_t y, int16_t xoff, int16_t yoff)
{
	x += PSXDisplay.DrawOffset.x;
	if (x > PSXDisplay.DisplayPosition.x + 1)
		return false;
	y += PSXDisplay.DrawOffset.y;
	if (y > PSXDisplay.DisplayPosition.y + 1)
		return false;
	xoff += PSXDisplay.DrawOffset.x;
	if (xoff < PSXDisplay.DisplayEnd.x - 1)
		return false;
	yoff += PSXDisplay.DrawOffset.y;
	if (yoff < PSXDisplay.DisplayEnd.y - 1)
		return false;
	return true;
}

////////////////////////////////////////////////////////////////////////
// mask stuff...
////////////////////////////////////////////////////////////////////////

// Mask1    Set mask bit while drawing. 1 = on
// Mask2    Do not draw to mask areas. 1= on

static void cmdSTP(uint8_t *baseAddr)
{
	uint32_t gdata = ((uint32_t *)baseAddr)[0];

	STATUSREG &= ~0x1800;                // clear the necessary bits
	STATUSREG |= ((gdata & 0x03) << 11); // set the current bits
}

////////////////////////////////////////////////////////////////////////
// cmd: Set texture page infos
////////////////////////////////////////////////////////////////////////

static void cmdTexturePage(uint8_t *baseAddr)
{
	uint32_t gdata = ((uint32_t *)baseAddr)[0];
	UpdateGlobalTP((uint16_t)gdata);
	GlobalTextREST = (gdata & 0x00ffffff) >> 9;
}

////////////////////////////////////////////////////////////////////////
// cmd: turn on/off texture window
////////////////////////////////////////////////////////////////////////

static void cmdTextureWindow(uint8_t *baseAddr)
{
	uint32_t gdata = ((uint32_t *)baseAddr)[0];
	uint32_t YAlign, XAlign;

	ulGPUInfoVals[INFO_TW] = gdata & 0xFFFFF;

	if (gdata & 0x020)
		TWin.Position.y1 = 8; // xxxx1
	else if (gdata & 0x040)
		TWin.Position.y1 = 16; // xxx10
	else if (gdata & 0x080)
		TWin.Position.y1 = 32; // xx100
	else if (gdata & 0x100)
		TWin.Position.y1 = 64; // x1000
	else if (gdata & 0x200)
		TWin.Position.y1 = 128; // 10000
	else
		TWin.Position.y1 = 256; // 00000

	// Texture window size is determined by the least bit set of the relevant 5 bits

	if (gdata & 0x001)
		TWin.Position.x1 = 8; // xxxx1
	else if (gdata & 0x002)
		TWin.Position.x1 = 16; // xxx10
	else if (gdata & 0x004)
		TWin.Position.x1 = 32; // xx100
	else if (gdata & 0x008)
		TWin.Position.x1 = 64; // x1000
	else if (gdata & 0x010)
		TWin.Position.x1 = 128; // 10000
	else
		TWin.Position.x1 = 256; // 00000

	// Re-calculate the bit field, because we can't trust what is passed in the data

	YAlign = (uint32_t)(32 - (TWin.Position.y1 >> 3));
	XAlign = (uint32_t)(32 - (TWin.Position.x1 >> 3));

	// Absolute position of the start of the texture window

	TWin.Position.y0 = (int16_t)(((gdata >> 15) & YAlign) << 3);
	TWin.Position.x0 = (int16_t)(((gdata >> 10) & XAlign) << 3);

	if ((TWin.Position.x0 == 0 && // tw turned off
	     TWin.Position.y0 == 0 && TWin.Position.x1 == 0 && TWin.Position.y1 == 0) ||
	    (TWin.Position.x1 == 256 && TWin.Position.y1 == 256))
	{
		bUsingTWin = false; // -> just do it
		TWin.UScaleFactor = TWin.VScaleFactor = 1.0f / 256.0f;
	}
	else // tw turned on
	{
		bUsingTWin = true;

		TWin.OPosition.y1 = TWin.Position.y1; // -> get psx sizes
		TWin.OPosition.x1 = TWin.Position.x1;

		if (TWin.Position.x1 <= 2)
			TWin.Position.x1 = 2; // -> set OGL sizes
		else if (TWin.Position.x1 <= 4)
			TWin.Position.x1 = 4;
		else if (TWin.Position.x1 <= 8)
			TWin.Position.x1 = 8;
		else if (TWin.Position.x1 <= 16)
			TWin.Position.x1 = 16;
		else if (TWin.Position.x1 <= 32)
			TWin.Position.x1 = 32;
		else if (TWin.Position.x1 <= 64)
			TWin.Position.x1 = 64;
		else if (TWin.Position.x1 <= 128)
			TWin.Position.x1 = 128;
		else if (TWin.Position.x1 <= 256)
			TWin.Position.x1 = 256;

		if (TWin.Position.y1 <= 2)
			TWin.Position.y1 = 2;
		else if (TWin.Position.y1 <= 4)
			TWin.Position.y1 = 4;
		else if (TWin.Position.y1 <= 8)
			TWin.Position.y1 = 8;
		else if (TWin.Position.y1 <= 16)
			TWin.Position.y1 = 16;
		else if (TWin.Position.y1 <= 32)
			TWin.Position.y1 = 32;
		else if (TWin.Position.y1 <= 64)
			TWin.Position.y1 = 64;
		else if (TWin.Position.y1 <= 128)
			TWin.Position.y1 = 128;
		else if (TWin.Position.y1 <= 256)
			TWin.Position.y1 = 256;
		TWin.UScaleFactor = ((float)TWin.Position.x1) / 256.0f; // -> set scale factor
		TWin.VScaleFactor = ((float)TWin.Position.y1) / 256.0f;
	}
}

////////////////////////////////////////////////////////////////////////
// Check draw area dimensions
////////////////////////////////////////////////////////////////////////

static void ClampToPSXScreen(int32_t *x0, int32_t *y0, int32_t *x1, int32_t *y1)
{
	if (*x0 < 0)
		*x0 = 0;
	else if (*x0 > 1023)
		*x0 = 1023;

	if (*x1 < 0)
		*x1 = 0;
	else if (*x1 > 1023)
		*x1 = 1023;

	if (*y0 < 0)
		*y0 = 0;
	else if (*y0 > iGPUHeightMask)
		*y0 = iGPUHeightMask;

	if (*y1 < 0)
		*y1 = 0;
	else if (*y1 > iGPUHeightMask)
		*y1 = iGPUHeightMask;
}

////////////////////////////////////////////////////////////////////////
// Used in Load Image and Blk Fill
////////////////////////////////////////////////////////////////////////

static void ClampToPSXScreenOffset(int32_t *x0, int32_t *y0, int32_t *x1, int32_t *y1)
{
	if (*x0 < 0)
	{
		*x1 += *x0;
		*x0 = 0;
	}
	else if (*x0 > 1023)
	{
		*x0 = 1023;
		*x1 = 0;
	}

	if (*y0 < 0)
	{
		*y1 += *y0;
		*y0 = 0;
	}
	else if (*y0 > iGPUHeightMask)
	{
		*y0 = iGPUHeightMask;
		*y1 = 0;
	}

	if (*x1 < 0)
		*x1 = 0;

	if ((*x1 + *x0) > 1024)
		*x1 = (1024 - *x0);

	if (*y1 < 0)
		*y1 = 0;

	if ((*y1 + *y0) > iGPUHeight)
		*y1 = (iGPUHeight - *y0);
}

////////////////////////////////////////////////////////////////////////
// cmd: start of drawing area... primitives will be clipped inside
////////////////////////////////////////////////////////////////////////

static void cmdDrawAreaStart(uint8_t *baseAddr)
{
	uint32_t gdata = ((uint32_t *)baseAddr)[0];

	drawX = gdata & 0x3ff; // for soft drawing
	if (drawX >= 1024)
		drawX = 1023;

	if (dwGPUVersion == 2)
	{
		ulGPUInfoVals[INFO_DRAWSTART] = gdata & 0x3FFFFF;
		drawY = (gdata >> 12) & 0x3ff;
	}
	else
	{
		ulGPUInfoVals[INFO_DRAWSTART] = gdata & 0xFFFFF;
		drawY = (gdata >> 10) & 0x3ff;
	}

	if (drawY >= iGPUHeight)
		drawY = iGPUHeightMask;

	PreviousPSXDisplay.DrawArea.y0 = PSXDisplay.DrawArea.y0;
	PreviousPSXDisplay.DrawArea.x0 = PSXDisplay.DrawArea.x0;

	PSXDisplay.DrawArea.y0 = (int16_t)drawY; // for OGL drawing
	PSXDisplay.DrawArea.x0 = (int16_t)drawX;
}

////////////////////////////////////////////////////////////////////////
// cmd: end of drawing area... primitives will be clipped inside
////////////////////////////////////////////////////////////////////////

static void cmdDrawAreaEnd(uint8_t *baseAddr)
{
	uint32_t gdata = ((uint32_t *)baseAddr)[0];

	drawW = gdata & 0x3ff; // for soft drawing
	if (drawW >= 1024)
		drawW = 1023;

	if (dwGPUVersion == 2)
	{
		ulGPUInfoVals[INFO_DRAWEND] = gdata & 0x3FFFFF;
		drawH = (gdata >> 12) & 0x3ff;
	}
	else
	{
		ulGPUInfoVals[INFO_DRAWEND] = gdata & 0xFFFFF;
		drawH = (gdata >> 10) & 0x3ff;
	}

	if (drawH >= iGPUHeight)
		drawH = iGPUHeightMask;

	PSXDisplay.DrawArea.y1 = (int16_t)drawH; // for OGL drawing
	PSXDisplay.DrawArea.x1 = (int16_t)drawW;

	ClampToPSXScreen(&PSXDisplay.DrawArea.x0, // clamp
	                 &PSXDisplay.DrawArea.y0, &PSXDisplay.DrawArea.x1, &PSXDisplay.DrawArea.y1);

	bDisplayNotSet = true;
}

////////////////////////////////////////////////////////////////////////
// cmd: draw offset... will be added to prim coords
////////////////////////////////////////////////////////////////////////

static void cmdDrawOffset(uint8_t *baseAddr)
{
	uint32_t gdata = ((uint32_t *)baseAddr)[0];

	PreviousPSXDisplay.DrawOffset.x = PSXDisplay.DrawOffset.x = (int16_t)(gdata & 0x7ff);

	if (dwGPUVersion == 2)
	{
		ulGPUInfoVals[INFO_DRAWOFF] = gdata & 0x7FFFFF;
		PSXDisplay.DrawOffset.y = (int16_t)((gdata >> 12) & 0x7ff);
	}
	else
	{
		ulGPUInfoVals[INFO_DRAWOFF] = gdata & 0x3FFFFF;
		PSXDisplay.DrawOffset.y = (int16_t)((gdata >> 11) & 0x7ff);
	}

	PSXDisplay.DrawOffset.x = (int16_t)(((int32_t)PSXDisplay.DrawOffset.x << 21) >> 21);
	PSXDisplay.DrawOffset.y = (int16_t)(((int32_t)PSXDisplay.DrawOffset.y << 21) >> 21);

	PSXDisplay.CumulOffset.x = // new OGL prim offsets
	    PSXDisplay.DrawOffset.x - PSXDisplay.GDrawOffset.x + PreviousPSXDisplay.Range.x0;
	PSXDisplay.CumulOffset.y = PSXDisplay.DrawOffset.y - PSXDisplay.GDrawOffset.y + PreviousPSXDisplay.Range.y0;
}

////////////////////////////////////////////////////////////////////////
// cmd: load image to vram
////////////////////////////////////////////////////////////////////////

static void primLoadImage(uint8_t *baseAddr)
{
	uint16_t *sgpuData = ((uint16_t *)baseAddr);

	VRAMWrite.x = sgpuData[2] & 0x03ff;
	VRAMWrite.y = sgpuData[3] & iGPUHeightMask;
	VRAMWrite.Width = sgpuData[4];
	VRAMWrite.Height = sgpuData[5];

	iDataWriteMode = DR_VRAMTRANSFER;
	VRAMWrite.ImagePtr = psxVuw + (VRAMWrite.y << 10) + VRAMWrite.x;
	VRAMWrite.RowsRemaining = VRAMWrite.Width;
	VRAMWrite.ColsRemaining = VRAMWrite.Height;

	bNeedWriteUpload = true;
}

////////////////////////////////////////////////////////////////////////

static void PrepareRGB24Upload(void)
{
	VRAMWrite.x = (VRAMWrite.x * 2) / 3;
	VRAMWrite.Width = (VRAMWrite.Width * 2) / 3;

	if (!PSXDisplay.InterlacedTest && // NEW
	    CheckAgainstScreen(VRAMWrite.x, VRAMWrite.y, VRAMWrite.Width, VRAMWrite.Height))
	{
		xrUploadArea.x0 -= PreviousPSXDisplay.DisplayPosition.x;
		xrUploadArea.x1 -= PreviousPSXDisplay.DisplayPosition.x;
		xrUploadArea.y0 -= PreviousPSXDisplay.DisplayPosition.y;
		xrUploadArea.y1 -= PreviousPSXDisplay.DisplayPosition.y;
	}
	else if (CheckAgainstFrontScreen(VRAMWrite.x, VRAMWrite.y, VRAMWrite.Width, VRAMWrite.Height))
	{
		xrUploadArea.x0 -= PSXDisplay.DisplayPosition.x;
		xrUploadArea.x1 -= PSXDisplay.DisplayPosition.x;
		xrUploadArea.y0 -= PSXDisplay.DisplayPosition.y;
		xrUploadArea.y1 -= PSXDisplay.DisplayPosition.y;
	}
	else
		return;

	if (bRenderFrontBuffer)
	{
		updateFrontDisplay();
	}

	if (bNeedRGB24Update == false)
	{
		xrUploadAreaRGB24 = xrUploadArea;
		bNeedRGB24Update = true;
	}
	else
	{
		xrUploadAreaRGB24.x0 = min(xrUploadAreaRGB24.x0, xrUploadArea.x0);
		xrUploadAreaRGB24.x1 = max(xrUploadAreaRGB24.x1, xrUploadArea.x1);
		xrUploadAreaRGB24.y0 = min(xrUploadAreaRGB24.y0, xrUploadArea.y0);
		xrUploadAreaRGB24.y1 = max(xrUploadAreaRGB24.y1, xrUploadArea.y1);
	}
}

////////////////////////////////////////////////////////////////////////

void CheckWriteUpdate()
{
	int32_t iX = 0, iY = 0;

	if (VRAMWrite.Width)
		iX = 1;
	if (VRAMWrite.Height)
		iY = 1;

	InvalidateTextureArea(VRAMWrite.x, VRAMWrite.y, VRAMWrite.Width - iX, VRAMWrite.Height - iY);

	if (PSXDisplay.RGB24)
	{
		PrepareRGB24Upload();
		return;
	}

	if (!PSXDisplay.InterlacedTest && CheckAgainstScreen(VRAMWrite.x, VRAMWrite.y, VRAMWrite.Width, VRAMWrite.Height))
	{

		if (bRenderFrontBuffer)
		{
			updateFrontDisplay();
		}

		UploadScreen(false);

		bNeedUploadTest = true;
	}
	else
	{
		if (CheckAgainstFrontScreen(VRAMWrite.x, VRAMWrite.y, VRAMWrite.Width, VRAMWrite.Height))
		{
			if (PSXDisplay.InterlacedTest)
			{
				if (PreviousPSXDisplay.InterlacedNew)
				{
					PreviousPSXDisplay.InterlacedNew = false;
					bNeedInterlaceUpdate = true;
					xrUploadAreaIL.x0 = PSXDisplay.DisplayPosition.x;
					xrUploadAreaIL.y0 = PSXDisplay.DisplayPosition.y;
					xrUploadAreaIL.x1 = PSXDisplay.DisplayPosition.x + PSXDisplay.DisplayModeNew.x;
					xrUploadAreaIL.y1 = PSXDisplay.DisplayPosition.y + PSXDisplay.DisplayModeNew.y;
					if (xrUploadAreaIL.x1 > 1023)
						xrUploadAreaIL.x1 = 1023;
					if (xrUploadAreaIL.y1 > 511)
						xrUploadAreaIL.y1 = 511;
				}

				if (bNeedInterlaceUpdate == false)
				{
					xrUploadAreaIL = xrUploadArea;
					bNeedInterlaceUpdate = true;
				}
				else
				{
					xrUploadAreaIL.x0 = min(xrUploadAreaIL.x0, xrUploadArea.x0);
					xrUploadAreaIL.x1 = max(xrUploadAreaIL.x1, xrUploadArea.x1);
					xrUploadAreaIL.y0 = min(xrUploadAreaIL.y0, xrUploadArea.y0);
					xrUploadAreaIL.y1 = max(xrUploadAreaIL.y1, xrUploadArea.y1);
				}
				return;
			}

			if (!bNeedUploadAfter)
			{
				bNeedUploadAfter = true;
				xrUploadArea.x0 = VRAMWrite.x;
				xrUploadArea.x1 = VRAMWrite.x + VRAMWrite.Width;
				xrUploadArea.y0 = VRAMWrite.y;
				xrUploadArea.y1 = VRAMWrite.y + VRAMWrite.Height;
			}
			else
			{
				xrUploadArea.x0 = min(xrUploadArea.x0, VRAMWrite.x);
				xrUploadArea.x1 = max(xrUploadArea.x1, VRAMWrite.x + VRAMWrite.Width);
				xrUploadArea.y0 = min(xrUploadArea.y0, VRAMWrite.y);
				xrUploadArea.y1 = max(xrUploadArea.y1, VRAMWrite.y + VRAMWrite.Height);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////
// cmd: vram -> psx mem
////////////////////////////////////////////////////////////////////////

static void primStoreImage(uint8_t *baseAddr)
{
	uint16_t *sgpuData = ((uint16_t *)baseAddr);

	VRAMRead.x = sgpuData[2] & 0x03ff;
	VRAMRead.y = sgpuData[3] & iGPUHeightMask;
	VRAMRead.Width = sgpuData[4];
	VRAMRead.Height = sgpuData[5];

	VRAMRead.ImagePtr = psxVuw + (VRAMRead.y << 10) + VRAMRead.x;
	VRAMRead.RowsRemaining = VRAMRead.Width;
	VRAMRead.ColsRemaining = VRAMRead.Height;

	iDataReadMode = DR_VRAMTRANSFER;

	STATUSREG |= GPUSTATUS_READYFORVRAM;
}

////////////////////////////////////////////////////////////////////////
// cmd: blkfill - NO primitive! Doesn't care about draw areas...
////////////////////////////////////////////////////////////////////////

static void primBlkFill(uint8_t *baseAddr)
{
	uint32_t *gpuData = ((uint32_t *)baseAddr);
	int16_t *sgpuData = ((int16_t *)baseAddr);

	iDrawnSomething = 1;

	sprtX = sgpuData[2];
	sprtY = sgpuData[3];
	sprtW = sgpuData[4] & 0x3ff;
	sprtH = sgpuData[5] & iGPUHeightMask;

	sprtW = (sprtW + 15) & ~15;

	// Increase H & W if they are one int16_t of full values, because they never can be full values
	if (sprtH == iGPUHeightMask)
		sprtH = iGPUHeight;
	if (sprtW == 1023)
		sprtW = 1024;

	// x and y of start
	ly0 = ly1 = sprtY;
	ly2 = ly3 = (sprtY + sprtH);
	lx0 = lx3 = sprtX;
	lx1 = lx2 = (sprtX + sprtW);

	offsetBlk();

	if (ClipVertexListScreen())
	{
		PSXDisplay_t *pd;
		if (PSXDisplay.InterlacedTest)
			pd = &PSXDisplay;
		else
			pd = &PreviousPSXDisplay;

		if ((lx0 <= pd->DisplayPosition.x + 16) && (ly0 <= pd->DisplayPosition.y + 16) &&
		    (lx2 >= pd->DisplayEnd.x - 16) && (ly2 >= pd->DisplayEnd.y - 16))
		{
			clearWithColor(gpuData[0]);
			gl_z = 0.0f;

			if (gpuData[0] != 0x02000000 && (ly0 > pd->DisplayPosition.y || ly2 < pd->DisplayEnd.y))
			{
				bDrawTextured = false;
				bDrawSmoothShaded = false;
				SetRenderState((uint32_t)0x01000000);
				SetRenderMode((uint32_t)0x01000000, false);
				vertex[0].c.lcol = 0xff000000;
				glColor4ubv(vertex[0].c.col);
				if (ly0 > pd->DisplayPosition.y)
				{
					vertex[0].x = 0;
					vertex[0].y = 0;
					vertex[1].x = pd->DisplayEnd.x - pd->DisplayPosition.x;
					vertex[1].y = 0;
					vertex[2].x = vertex[1].x;
					vertex[2].y = ly0 - pd->DisplayPosition.y;
					vertex[3].x = 0;
					vertex[3].y = vertex[2].y;
					PRIMdrawQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);
				}
				if (ly2 < pd->DisplayEnd.y)
				{
					vertex[0].x = 0;
					vertex[0].y = (pd->DisplayEnd.y - pd->DisplayPosition.y) - (pd->DisplayEnd.y - ly2);
					vertex[1].x = pd->DisplayEnd.x - pd->DisplayPosition.x;
					vertex[1].y = vertex[0].y;
					vertex[2].x = vertex[1].x;
					vertex[2].y = pd->DisplayEnd.y;
					vertex[3].x = 0;
					vertex[3].y = vertex[2].y;
					PRIMdrawQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);
				}
			}
		}
		else
		{
			bDrawTextured = false;
			bDrawSmoothShaded = false;
			SetRenderState((uint32_t)0x01000000);
			SetRenderMode((uint32_t)0x01000000, false);
			vertex[0].c.lcol = gpuData[0] | 0xff000000;
			glColor4ubv(vertex[0].c.col);
			PRIMdrawQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);
		}
	}

	// mmm... will clean all stuff, also if not all _should_ be cleaned...

	if (IsCompleteInsideNextScreen(sprtX, sprtY, sprtW, sprtH))
	{
		lClearOnSwapColor = COLOR(gpuData[0]);
		lClearOnSwap = 1;
	}

	ClampToPSXScreenOffset(&sprtX, &sprtY, &sprtW, &sprtH);
	if ((sprtW == 0) || (sprtH == 0))
		return;
	InvalidateTextureArea(sprtX, sprtY, sprtW - 1, sprtH - 1);

	sprtW += sprtX;
	sprtH += sprtY;

	FillSoftwareArea(sprtX, sprtY, sprtW, sprtH, BGR24to16(gpuData[0]));
}

////////////////////////////////////////////////////////////////////////
// cmd: move image vram -> vram
////////////////////////////////////////////////////////////////////////

static void MoveImageWrapped(int16_t imageX0, int16_t imageY0, int16_t imageX1, int16_t imageY1, int16_t imageSX,
                             int16_t imageSY)
{
	int32_t i, j, imageXE, imageYE;

	if (iFrameReadType & 2)
	{
		imageXE = imageX0 + imageSX;
		imageYE = imageY0 + imageSY;

		if (imageYE > iGPUHeight && imageXE > 1024)
		{
			CheckVRamRead(0, 0, (imageXE & 0x3ff), (imageY0 & iGPUHeightMask), false);
		}

		if (imageXE > 1024)
		{
			CheckVRamRead(0, imageY0, (imageXE & 0x3ff), (imageYE > iGPUHeight) ? iGPUHeight : imageYE, false);
		}

		if (imageYE > iGPUHeight)
		{
			CheckVRamRead(imageX0, 0, (imageXE > 1024) ? 1024 : imageXE, imageYE & iGPUHeightMask, false);
		}

		CheckVRamRead(imageX0, imageY0, (imageXE > 1024) ? 1024 : imageXE,
		              (imageYE > iGPUHeight) ? iGPUHeight : imageYE, false);
	}

	for (j = 0; j < imageSY; j++)
		for (i = 0; i < imageSX; i++)
			psxVuw[(1024 * ((imageY1 + j) & iGPUHeightMask)) + ((imageX1 + i) & 0x3ff)] =
			    psxVuw[(1024 * ((imageY0 + j) & iGPUHeightMask)) + ((imageX0 + i) & 0x3ff)];

	if (!PSXDisplay.RGB24)
	{
		imageXE = imageX1 + imageSX;
		imageYE = imageY1 + imageSY;

		if (imageYE > iGPUHeight && imageXE > 1024)
		{
			InvalidateTextureArea(0, 0, (imageXE & 0x3ff) - 1, (imageYE & iGPUHeightMask) - 1);
		}

		if (imageXE > 1024)
		{
			InvalidateTextureArea(0, imageY1, (imageXE & 0x3ff) - 1,
			                      ((imageYE > iGPUHeight) ? iGPUHeight : imageYE) - imageY1 - 1);
		}

		if (imageYE > iGPUHeight)
		{
			InvalidateTextureArea(imageX1, 0, ((imageXE > 1024) ? 1024 : imageXE) - imageX1 - 1,
			                      (imageYE & iGPUHeightMask) - 1);
		}

		InvalidateTextureArea(imageX1, imageY1, ((imageXE > 1024) ? 1024 : imageXE) - imageX1 - 1,
		                      ((imageYE > iGPUHeight) ? iGPUHeight : imageYE) - imageY1 - 1);
	}
}

////////////////////////////////////////////////////////////////////////

static void primMoveImage(uint8_t *baseAddr)
{
	int16_t *sgpuData = ((int16_t *)baseAddr);
	int16_t imageY0, imageX0, imageY1, imageX1, imageSX, imageSY, i, j;

	imageX0 = sgpuData[2] & 0x03ff;
	imageY0 = sgpuData[3] & iGPUHeightMask;
	imageX1 = sgpuData[4] & 0x03ff;
	imageY1 = sgpuData[5] & iGPUHeightMask;
	imageSX = sgpuData[6];
	imageSY = sgpuData[7];

	if ((imageX0 == imageX1) && (imageY0 == imageY1))
		return;
	if (imageSX <= 0)
		return;
	if (imageSY <= 0)
		return;

	if (iGPUHeight == 1024 && sgpuData[7] > 1024)
		return;

	if ((imageY0 + imageSY) > iGPUHeight || (imageX0 + imageSX) > 1024 || (imageY1 + imageSY) > iGPUHeight ||
	    (imageX1 + imageSX) > 1024)
	{
		MoveImageWrapped(imageX0, imageY0, imageX1, imageY1, imageSX, imageSY);
		if ((imageY0 + imageSY) > iGPUHeight)
			imageSY = iGPUHeight - imageY0;
		if ((imageX0 + imageSX) > 1024)
			imageSX = 1024 - imageX0;
		if ((imageY1 + imageSY) > iGPUHeight)
			imageSY = iGPUHeight - imageY1;
		if ((imageX1 + imageSX) > 1024)
			imageSX = 1024 - imageX1;
	}

	if (iFrameReadType & 2)
		CheckVRamRead(imageX0, imageY0, imageX0 + imageSX, imageY0 + imageSY, false);

	if (imageSX & 1)
	{
		uint16_t *SRCPtr, *DSTPtr;
		uint16_t LineOffset;

		SRCPtr = psxVuw + (1024 * imageY0) + imageX0;
		DSTPtr = psxVuw + (1024 * imageY1) + imageX1;

		LineOffset = 1024 - imageSX;

		for (j = 0; j < imageSY; j++)
		{
			for (i = 0; i < imageSX; i++)
				*DSTPtr++ = *SRCPtr++;
			SRCPtr += LineOffset;
			DSTPtr += LineOffset;
		}
	}
	else
	{
		uint32_t *SRCPtr, *DSTPtr;
		uint16_t LineOffset;
		int32_t dx = imageSX >> 1;

		SRCPtr = (uint32_t *)(psxVuw + (1024 * imageY0) + imageX0);
		DSTPtr = (uint32_t *)(psxVuw + (1024 * imageY1) + imageX1);

		LineOffset = 512 - dx;

		for (j = 0; j < imageSY; j++)
		{
			for (i = 0; i < dx; i++)
				*DSTPtr++ = *SRCPtr++;
			SRCPtr += LineOffset;
			DSTPtr += LineOffset;
		}
	}

	if (!PSXDisplay.RGB24)
	{
		InvalidateTextureArea(imageX1, imageY1, imageSX - 1, imageSY - 1);

		if (CheckAgainstScreen(imageX1, imageY1, imageSX, imageSY))
		{
			if (imageX1 >= PreviousPSXDisplay.DisplayPosition.x && imageX1 < PreviousPSXDisplay.DisplayEnd.x &&
			    imageY1 >= PreviousPSXDisplay.DisplayPosition.y && imageY1 < PreviousPSXDisplay.DisplayEnd.y)
			{
				imageX1 += imageSX;
				imageY1 += imageSY;

				if (imageX1 >= PreviousPSXDisplay.DisplayPosition.x && imageX1 <= PreviousPSXDisplay.DisplayEnd.x &&
				    imageY1 >= PreviousPSXDisplay.DisplayPosition.y && imageY1 <= PreviousPSXDisplay.DisplayEnd.y)
				{
					if (!(imageX0 >= PSXDisplay.DisplayPosition.x && imageX0 < PSXDisplay.DisplayEnd.x &&
					      imageY0 >= PSXDisplay.DisplayPosition.y && imageY0 < PSXDisplay.DisplayEnd.y))
					{
						if (bRenderFrontBuffer)
						{
							updateFrontDisplay();
						}

						UploadScreen(false);
					}
					else
						bFakeFrontBuffer = true;
				}
			}

			bNeedUploadTest = true;
		}
		else
		{
			if (CheckAgainstFrontScreen(imageX1, imageY1, imageSX, imageSY))
			{
				if (!PSXDisplay.InterlacedTest &&
				    //          !bFullVRam &&
				    ((imageX0 >= PreviousPSXDisplay.DisplayPosition.x && imageX0 < PreviousPSXDisplay.DisplayEnd.x &&
				      imageY0 >= PreviousPSXDisplay.DisplayPosition.y && imageY0 < PreviousPSXDisplay.DisplayEnd.y) ||
				     (imageX0 >= PSXDisplay.DisplayPosition.x && imageX0 < PSXDisplay.DisplayEnd.x &&
				      imageY0 >= PSXDisplay.DisplayPosition.y && imageY0 < PSXDisplay.DisplayEnd.y)))
					return;

				bNeedUploadTest = true;

				if (!bNeedUploadAfter)
				{
					bNeedUploadAfter = true;
					xrUploadArea.x0 = imageX0;
					xrUploadArea.x1 = imageX0 + imageSX;
					xrUploadArea.y0 = imageY0;
					xrUploadArea.y1 = imageY0 + imageSY;
				}
				else
				{
					xrUploadArea.x0 = min(xrUploadArea.x0, imageX0);
					xrUploadArea.x1 = max(xrUploadArea.x1, imageX0 + imageSX);
					xrUploadArea.y0 = min(xrUploadArea.y0, imageY0);
					xrUploadArea.y1 = max(xrUploadArea.y1, imageY0 + imageSY);
				}
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////
// cmd: draw free-size Tile
////////////////////////////////////////////////////////////////////////

static void primTileS(uint8_t *baseAddr)
{
	uint32_t *gpuData = ((uint32_t *)baseAddr);
	int16_t *sgpuData = ((int16_t *)baseAddr);

	sprtX = sgpuData[2];
	sprtY = sgpuData[3];
	sprtW = sgpuData[4] & 0x3ff;
	sprtH = sgpuData[5] & iGPUHeightMask;

	// x and y of start

	lx0 = sprtX;
	ly0 = sprtY;

	offsetST();

	bDrawTextured = false;
	bDrawSmoothShaded = false;

	SetRenderState(gpuData[0]);

	if (IsPrimCompleteInsideNextScreen(lx0, ly0, lx2, ly2))
	{
		lClearOnSwapColor = COLOR(gpuData[0]);
		lClearOnSwap = 1;
	}

	offsetPSX4();
	if (bDrawOffscreen4())
	{
		InvalidateTextureAreaEx();
		FillSoftwareAreaTrans(lx0, ly0, lx2, ly2, BGR24to16(gpuData[0]));
	}

	SetRenderMode(gpuData[0], false);

	if (bIgnoreNextTile)
	{
		bIgnoreNextTile = false;
		return;
	}

	vertex[0].c.lcol = gpuData[0];
	vertex[0].c.col[3] = ubGloColAlpha;
	glColor4ubv(vertex[0].c.col);

	PRIMdrawQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);

	iDrawnSomething = 1;
}

////////////////////////////////////////////////////////////////////////
// cmd: draw 1 dot Tile (point)
////////////////////////////////////////////////////////////////////////

static void primTile1(uint8_t *baseAddr)
{
	uint32_t *gpuData = ((uint32_t *)baseAddr);
	int16_t *sgpuData = ((int16_t *)baseAddr);

	sprtX = sgpuData[2];
	sprtY = sgpuData[3];
	sprtW = 1;
	sprtH = 1;

	lx0 = sprtX;
	ly0 = sprtY;

	offsetST();

	bDrawTextured = false;
	bDrawSmoothShaded = false;

	SetRenderState(gpuData[0]);

	offsetPSX4();

	if (bDrawOffscreen4())
	{
		InvalidateTextureAreaEx();
		FillSoftwareAreaTrans(lx0, ly0, lx2, ly2, BGR24to16(gpuData[0]));
	}

	SetRenderMode(gpuData[0], false);

	vertex[0].c.lcol = gpuData[0];
	vertex[0].c.col[3] = ubGloColAlpha;
	glColor4ubv(vertex[0].c.col);

	PRIMdrawQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);

	iDrawnSomething = 1;
}

////////////////////////////////////////////////////////////////////////
// cmd: draw 8 dot Tile (small rect)
////////////////////////////////////////////////////////////////////////

static void primTile8(uint8_t *baseAddr)
{
	uint32_t *gpuData = ((uint32_t *)baseAddr);
	int16_t *sgpuData = ((int16_t *)baseAddr);

	sprtX = sgpuData[2];
	sprtY = sgpuData[3];
	sprtW = 8;
	sprtH = 8;

	lx0 = sprtX;
	ly0 = sprtY;

	offsetST();

	bDrawTextured = false;
	bDrawSmoothShaded = false;
	SetRenderState(gpuData[0]);

	offsetPSX4();

	if (bDrawOffscreen4())
	{
		InvalidateTextureAreaEx();
		FillSoftwareAreaTrans(lx0, ly0, lx2, ly2, BGR24to16(gpuData[0]));
	}

	SetRenderMode(gpuData[0], false);

	vertex[0].c.lcol = gpuData[0];
	vertex[0].c.col[3] = ubGloColAlpha;
	glColor4ubv(vertex[0].c.col);

	PRIMdrawQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);

	iDrawnSomething = 1;
}

////////////////////////////////////////////////////////////////////////
// cmd: draw 16 dot Tile (medium rect)
////////////////////////////////////////////////////////////////////////

static void primTile16(uint8_t *baseAddr)
{
	uint32_t *gpuData = ((uint32_t *)baseAddr);
	int16_t *sgpuData = ((int16_t *)baseAddr);

	sprtX = sgpuData[2];
	sprtY = sgpuData[3];
	sprtW = 16;
	sprtH = 16;
	// x and y of start
	lx0 = sprtX;
	ly0 = sprtY;

	offsetST();

	bDrawTextured = false;
	bDrawSmoothShaded = false;
	SetRenderState(gpuData[0]);

	offsetPSX4();

	if (bDrawOffscreen4())
	{
		InvalidateTextureAreaEx();
		FillSoftwareAreaTrans(lx0, ly0, lx2, ly2, BGR24to16(gpuData[0]));
	}

	SetRenderMode(gpuData[0], false);

	vertex[0].c.lcol = gpuData[0];
	vertex[0].c.col[3] = ubGloColAlpha;
	glColor4ubv(vertex[0].c.col);

	PRIMdrawQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);

	iDrawnSomething = 1;
}

////////////////////////////////////////////////////////////////////////
// cmd: small sprite (textured rect)
////////////////////////////////////////////////////////////////////////

static void primSprt8(uint8_t *baseAddr)
{
	uint32_t *gpuData = ((uint32_t *)baseAddr);
	int16_t *sgpuData = ((int16_t *)baseAddr);
	int16_t s;

	iSpriteTex = 1;

	sprtX = sgpuData[2];
	sprtY = sgpuData[3];
	sprtW = 8;
	sprtH = 8;

	lx0 = sprtX;
	ly0 = sprtY;

	offsetST();

	// do texture stuff
	gl_ux[0] = gl_ux[3] = baseAddr[8]; // gpuData[2]&0xff;

	if (usMirror & 0x1000)
	{
		s = gl_ux[0];
		s -= sprtW - 1;
		if (s < 0)
		{
			s = 0;
		}
		gl_ux[0] = gl_ux[3] = s;
	}

	sSprite_ux2 = s = gl_ux[0] + sprtW;
	if (s)
		s--;
	if (s > 255)
		s = 255;
	gl_ux[1] = gl_ux[2] = s;
	// Y coords
	gl_vy[0] = gl_vy[1] = baseAddr[9]; //(gpuData[2]>>8)&0xff;

	if (usMirror & 0x2000)
	{
		s = gl_vy[0];
		s -= sprtH - 1;
		if (s < 0)
		{
			s = 0;
		}
		gl_vy[0] = gl_vy[1] = s;
	}

	sSprite_vy2 = s = gl_vy[0] + sprtH;
	if (s)
		s--;
	if (s > 255)
		s = 255;
	gl_vy[2] = gl_vy[3] = s;

	ulClutID = (gpuData[2] >> 16);

	bDrawTextured = true;
	bDrawSmoothShaded = false;
	SetRenderState(gpuData[0]);

	offsetPSX4();

	if (bDrawOffscreen4())
	{
		InvalidateTextureAreaEx();
		SetRenderColor(gpuData[0]);
		lx0 -= PSXDisplay.DrawOffset.x;
		ly0 -= PSXDisplay.DrawOffset.y;

		if (bUsingTWin)
			DrawSoftwareSpriteTWin(baseAddr, 8, 8);
		else if (usMirror)
			DrawSoftwareSpriteMirror(baseAddr, 8, 8);
		else
			DrawSoftwareSprite(baseAddr, 8, 8, baseAddr[8], baseAddr[9]);
	}

	SetRenderMode(gpuData[0], true);

	sSprite_ux2 = gl_ux[0] + sprtW;
	sSprite_vy2 = gl_vy[0] + sprtH;

	assignTextureSprite();

	PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);

	if (bDrawMultiPass)
	{
		SetSemiTransMulti(1);
		PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);
	}

	if (ubOpaqueDraw)
	{
		DEFOPAQUEON();

		PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);
		DEFOPAQUEOFF();
	}

	iSpriteTex = 0;
	iDrawnSomething = 1;
}

////////////////////////////////////////////////////////////////////////
// cmd: medium sprite (textured rect)
////////////////////////////////////////////////////////////////////////

static void primSprt16(uint8_t *baseAddr)
{
	uint32_t *gpuData = ((uint32_t *)baseAddr);
	int16_t *sgpuData = ((int16_t *)baseAddr);
	int16_t s;

	iSpriteTex = 1;

	sprtX = sgpuData[2];
	sprtY = sgpuData[3];
	sprtW = 16;
	sprtH = 16;

	lx0 = sprtX;
	ly0 = sprtY;

	offsetST();

	// do texture stuff
	gl_ux[0] = gl_ux[3] = baseAddr[8]; // gpuData[2]&0xff;

	if (usMirror & 0x1000)
	{
		s = gl_ux[0];
		s -= sprtW - 1;
		if (s < 0)
		{
			s = 0;
		}
		gl_ux[0] = gl_ux[3] = s;
	}

	sSprite_ux2 = s = gl_ux[0] + sprtW;
	if (s)
		s--;
	if (s > 255)
		s = 255;
	gl_ux[1] = gl_ux[2] = s;
	// Y coords
	gl_vy[0] = gl_vy[1] = baseAddr[9]; //(gpuData[2]>>8)&0xff;

	if (usMirror & 0x2000)
	{
		s = gl_vy[0];
		s -= sprtH - 1;
		if (s < 0)
		{
			s = 0;
		}
		gl_vy[0] = gl_vy[1] = s;
	}

	sSprite_vy2 = s = gl_vy[0] + sprtH;
	if (s)
		s--;
	if (s > 255)
		s = 255;
	gl_vy[2] = gl_vy[3] = s;

	ulClutID = (gpuData[2] >> 16);

	bDrawTextured = true;
	bDrawSmoothShaded = false;
	SetRenderState(gpuData[0]);

	offsetPSX4();

	if (bDrawOffscreen4())
	{
		InvalidateTextureAreaEx();
		SetRenderColor(gpuData[0]);
		lx0 -= PSXDisplay.DrawOffset.x;
		ly0 -= PSXDisplay.DrawOffset.y;
		if (bUsingTWin)
			DrawSoftwareSpriteTWin(baseAddr, 16, 16);
		else if (usMirror)
			DrawSoftwareSpriteMirror(baseAddr, 16, 16);
		else
			DrawSoftwareSprite(baseAddr, 16, 16, baseAddr[8], baseAddr[9]);
	}

	SetRenderMode(gpuData[0], true);

	sSprite_ux2 = gl_ux[0] + sprtW;
	sSprite_vy2 = gl_vy[0] + sprtH;

	assignTextureSprite();

	PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);

	if (bDrawMultiPass)
	{
		SetSemiTransMulti(1);
		PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);
	}

	if (ubOpaqueDraw)
	{
		DEFOPAQUEON();

		PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);
		DEFOPAQUEOFF();
	}

	iSpriteTex = 0;
	iDrawnSomething = 1;
}

////////////////////////////////////////////////////////////////////////
// cmd: free-size sprite (textured rect)
////////////////////////////////////////////////////////////////////////

static void primSprtSRest(uint8_t *baseAddr, uint16_t type)
{
	uint32_t *gpuData = ((uint32_t *)baseAddr);
	int16_t *sgpuData = ((int16_t *)baseAddr);
	int16_t s;
	uint16_t sTypeRest = 0;

	sprtX = sgpuData[2];
	sprtY = sgpuData[3];
	sprtW = sgpuData[6] & 0x3ff;
	sprtH = sgpuData[7] & 0x1ff;

	// do texture stuff
	switch (type)
	{
	case 1:
		gl_vy[0] = gl_vy[1] = baseAddr[9];
		s = 256 - baseAddr[8];
		sprtW -= s;
		sprtX += s;
		gl_ux[0] = gl_ux[3] = 0;
		break;
	case 2:
		gl_ux[0] = gl_ux[3] = baseAddr[8];
		s = 256 - baseAddr[9];
		sprtH -= s;
		sprtY += s;
		gl_vy[0] = gl_vy[1] = 0;
		break;
	case 3:
		s = 256 - baseAddr[8];
		sprtW -= s;
		sprtX += s;
		gl_ux[0] = gl_ux[3] = 0;
		s = 256 - baseAddr[9];
		sprtH -= s;
		sprtY += s;
		gl_vy[0] = gl_vy[1] = 0;
		break;

	case 4:
		gl_vy[0] = gl_vy[1] = baseAddr[9];
		s = 512 - baseAddr[8];
		sprtW -= s;
		sprtX += s;
		gl_ux[0] = gl_ux[3] = 0;
		break;
	case 5:
		gl_ux[0] = gl_ux[3] = baseAddr[8];
		s = 512 - baseAddr[9];
		sprtH -= s;
		sprtY += s;
		gl_vy[0] = gl_vy[1] = 0;
		break;
	case 6:
		s = 512 - baseAddr[8];
		sprtW -= s;
		sprtX += s;
		gl_ux[0] = gl_ux[3] = 0;
		s = 512 - baseAddr[9];
		sprtH -= s;
		sprtY += s;
		gl_vy[0] = gl_vy[1] = 0;
		break;
	}

	if (usMirror & 0x1000)
	{
		s = gl_ux[0];
		s -= sprtW - 1;
		if (s < 0)
			s = 0;
		gl_ux[0] = gl_ux[3] = s;
	}
	if (usMirror & 0x2000)
	{
		s = gl_vy[0];
		s -= sprtH - 1;
		if (s < 0)
		{
			s = 0;
		}
		gl_vy[0] = gl_vy[1] = s;
	}

	sSprite_ux2 = s = gl_ux[0] + sprtW;
	if (s > 255)
		s = 255;
	gl_ux[1] = gl_ux[2] = s;
	sSprite_vy2 = s = gl_vy[0] + sprtH;
	if (s > 255)
		s = 255;
	gl_vy[2] = gl_vy[3] = s;

	if (!bUsingTWin)
	{
		if (sSprite_ux2 > 256)
		{
			sprtW = 256 - gl_ux[0];
			sSprite_ux2 = 256;
			sTypeRest += 1;
		}
		if (sSprite_vy2 > 256)
		{
			sprtH = 256 - gl_vy[0];
			sSprite_vy2 = 256;
			sTypeRest += 2;
		}
	}

	lx0 = sprtX;
	ly0 = sprtY;

	offsetST();

	ulClutID = (gpuData[2] >> 16);

	bDrawTextured = true;
	bDrawSmoothShaded = false;
	SetRenderState(gpuData[0]);

	offsetPSX4();

	if (bDrawOffscreen4())
	{
		InvalidateTextureAreaEx();
		SetRenderColor(gpuData[0]);
		lx0 -= PSXDisplay.DrawOffset.x;
		ly0 -= PSXDisplay.DrawOffset.y;
		if (bUsingTWin)
			DrawSoftwareSpriteTWin(baseAddr, sprtW, sprtH);
		else if (usMirror)
			DrawSoftwareSpriteMirror(baseAddr, sprtW, sprtH);
		else
			DrawSoftwareSprite(baseAddr, sprtW, sprtH, baseAddr[8], baseAddr[9]);
	}

	SetRenderMode(gpuData[0], true);

	sSprite_ux2 = gl_ux[0] + sprtW;
	sSprite_vy2 = gl_vy[0] + sprtH;

	assignTextureSprite();

	PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);

	if (bDrawMultiPass)
	{
		SetSemiTransMulti(1);
		PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);
	}

	if (ubOpaqueDraw)
	{
		DEFOPAQUEON();

		PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);
		DEFOPAQUEOFF();
	}

	if (sTypeRest && type < 4)
	{
		if (sTypeRest & 1 && type == 1)
			primSprtSRest(baseAddr, 4);
		if (sTypeRest & 2 && type == 2)
			primSprtSRest(baseAddr, 5);
		if (sTypeRest == 3 && type == 3)
			primSprtSRest(baseAddr, 6);
	}
}

static void primSprtS(uint8_t *baseAddr)
{
	uint32_t *gpuData = ((uint32_t *)baseAddr);
	int16_t *sgpuData = ((int16_t *)baseAddr);

	int16_t s;
	uint16_t sTypeRest = 0;

	sprtX = sgpuData[2];
	sprtY = sgpuData[3];
	sprtW = sgpuData[6] & 0x3ff;
	sprtH = sgpuData[7] & 0x1ff;

	if (!sprtH)
		return;
	if (!sprtW)
		return;

	iSpriteTex = 1;

	// do texture stuff
	gl_ux[0] = gl_ux[3] = baseAddr[8]; // gpuData[2]&0xff;
	gl_vy[0] = gl_vy[1] = baseAddr[9]; //(gpuData[2]>>8)&0xff;

	if (usMirror & 0x1000)
	{
		s = gl_ux[0];
		s -= sprtW - 1;
		if (s < 0)
		{
			s = 0;
		}
		gl_ux[0] = gl_ux[3] = s;
	}
	if (usMirror & 0x2000)
	{
		s = gl_vy[0];
		s -= sprtH - 1;
		if (s < 0)
		{
			s = 0;
		}
		gl_vy[0] = gl_vy[1] = s;
	}

	sSprite_ux2 = s = gl_ux[0] + sprtW;
	if (s)
		s--;
	if (s > 255)
		s = 255;
	gl_ux[1] = gl_ux[2] = s;
	sSprite_vy2 = s = gl_vy[0] + sprtH;
	if (s)
		s--;
	if (s > 255)
		s = 255;
	gl_vy[2] = gl_vy[3] = s;

	if (!bUsingTWin)
	{
		if (sSprite_ux2 > 256)
		{
			sprtW = 256 - gl_ux[0];
			sSprite_ux2 = 256;
			sTypeRest += 1;
		}
		if (sSprite_vy2 > 256)
		{
			sprtH = 256 - gl_vy[0];
			sSprite_vy2 = 256;
			sTypeRest += 2;
		}
	}

	lx0 = sprtX;
	ly0 = sprtY;

	offsetST();

	ulClutID = (gpuData[2] >> 16);

	bDrawTextured = true;
	bDrawSmoothShaded = false;
	SetRenderState(gpuData[0]);

	offsetPSX4();

	if (bDrawOffscreen4())
	{
		InvalidateTextureAreaEx();
		SetRenderColor(gpuData[0]);
		lx0 -= PSXDisplay.DrawOffset.x;
		ly0 -= PSXDisplay.DrawOffset.y;
		if (bUsingTWin)
			DrawSoftwareSpriteTWin(baseAddr, sprtW, sprtH);
		else if (usMirror)
			DrawSoftwareSpriteMirror(baseAddr, sprtW, sprtH);
		else
			DrawSoftwareSprite(baseAddr, sprtW, sprtH, baseAddr[8], baseAddr[9]);
	}

	SetRenderMode(gpuData[0], true);

	sSprite_ux2 = gl_ux[0] + sprtW;
	sSprite_vy2 = gl_vy[0] + sprtH;

	assignTextureSprite();

	PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);

	if (bDrawMultiPass)
	{
		SetSemiTransMulti(1);
		PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);
	}

	if (ubOpaqueDraw)
	{
		DEFOPAQUEON();

		PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);
		DEFOPAQUEOFF();
	}

	if (sTypeRest)
	{
		if (sTypeRest & 1)
			primSprtSRest(baseAddr, 1);
		if (sTypeRest & 2)
			primSprtSRest(baseAddr, 2);
		if (sTypeRest == 3)
			primSprtSRest(baseAddr, 3);
	}

	iSpriteTex = 0;
	iDrawnSomething = 1;
}

////////////////////////////////////////////////////////////////////////
// cmd: flat shaded Poly4
////////////////////////////////////////////////////////////////////////

static void primPolyF4(uint8_t *baseAddr)
{
	uint32_t *gpuData = ((uint32_t *)baseAddr);
	int16_t *sgpuData = ((int16_t *)baseAddr);

	lx0 = sgpuData[2];
	ly0 = sgpuData[3];
	lx1 = sgpuData[4];
	ly1 = sgpuData[5];
	lx2 = sgpuData[6];
	ly2 = sgpuData[7];
	lx3 = sgpuData[8];
	ly3 = sgpuData[9];

	if (offset4())
		return;

	bDrawTextured = false;
	bDrawSmoothShaded = false;
	SetRenderState(gpuData[0]);

	offsetPSX4();
	if (bDrawOffscreen4())
	{
		InvalidateTextureAreaEx();
		drawPoly4F(gpuData[0]);
	}

	SetRenderMode(gpuData[0], false);

	vertex[0].c.lcol = gpuData[0];
	vertex[0].c.col[3] = ubGloColAlpha;
	glColor4ubv(vertex[0].c.col);

	PRIMdrawTri2(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);

	iDrawnSomething = 1;
}

////////////////////////////////////////////////////////////////////////
// cmd: smooth shaded Poly4
////////////////////////////////////////////////////////////////////////

static void primPolyG4(uint8_t *baseAddr)
{
	uint32_t *gpuData = (uint32_t *)baseAddr;
	int16_t *sgpuData = ((int16_t *)baseAddr);

	lx0 = sgpuData[2];
	ly0 = sgpuData[3];
	lx1 = sgpuData[6];
	ly1 = sgpuData[7];
	lx2 = sgpuData[10];
	ly2 = sgpuData[11];
	lx3 = sgpuData[14];
	ly3 = sgpuData[15];

	if (offset4())
		return;

	bDrawTextured = false;
	bDrawSmoothShaded = true;
	SetRenderState(gpuData[0]);

	offsetPSX4();

	if (bDrawOffscreen4())
	{
		InvalidateTextureAreaEx();
		drawPoly4G(gpuData[0], gpuData[2], gpuData[4], gpuData[6]);
	}

	SetRenderMode(gpuData[0], false);

	vertex[0].c.lcol = gpuData[0];
	vertex[1].c.lcol = gpuData[2];
	vertex[2].c.lcol = gpuData[4];
	vertex[3].c.lcol = gpuData[6];

	vertex[0].c.col[3] = vertex[1].c.col[3] = vertex[2].c.col[3] = vertex[3].c.col[3] = ubGloAlpha;

	PRIMdrawGouraudTri2Color(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);

	iDrawnSomething = 1;
}

////////////////////////////////////////////////////////////////////////
// cmd: flat shaded Texture3
////////////////////////////////////////////////////////////////////////

static bool DoLineCheck(uint32_t *)
{
	bool bQuad = false;
	int16_t dx, dy;

	if (lx0 == lx1)
	{
		dx = lx0 - lx2;
		if (dx < 0)
			dx = -dx;

		if (ly1 == ly2)
		{
			dy = ly1 - ly0;
			if (dy < 0)
				dy = -dy;
			if (dx <= 1)
			{
				vertex[3] = vertex[2];
				vertex[2] = vertex[0];
				vertex[2].x = vertex[3].x;
			}
			else if (dy <= 1)
			{
				vertex[3] = vertex[2];
				vertex[2].y = vertex[0].y;
			}
			else
				return false;

			bQuad = true;
		}
		else if (ly0 == ly2)
		{
			dy = ly0 - ly1;
			if (dy < 0)
				dy = -dy;
			if (dx <= 1)
			{
				vertex[3] = vertex[1];
				vertex[3].x = vertex[2].x;
			}
			else if (dy <= 1)
			{
				vertex[3] = vertex[2];
				vertex[3].y = vertex[1].y;
			}
			else
				return false;

			bQuad = true;
		}
	}

	if (lx0 == lx2)
	{
		dx = lx0 - lx1;
		if (dx < 0)
			dx = -dx;

		if (ly2 == ly1)
		{
			dy = ly2 - ly0;
			if (dy < 0)
				dy = -dy;
			if (dx <= 1)
			{
				vertex[3] = vertex[1];
				vertex[1] = vertex[0];
				vertex[1].x = vertex[3].x;
			}
			else if (dy <= 1)
			{
				vertex[3] = vertex[1];
				vertex[1].y = vertex[0].y;
			}
			else
				return false;

			bQuad = true;
		}
		else if (ly0 == ly1)
		{
			dy = ly2 - ly0;
			if (dy < 0)
				dy = -dy;
			if (dx <= 1)
			{
				vertex[3] = vertex[2];
				vertex[3].x = vertex[1].x;
			}
			else if (dy <= 1)
			{
				vertex[3] = vertex[1];
				vertex[3].y = vertex[2].y;
			}
			else
				return false;

			bQuad = true;
		}
	}

	if (lx1 == lx2)
	{
		dx = lx1 - lx0;
		if (dx < 0)
			dx = -dx;

		if (ly1 == ly0)
		{
			dy = ly1 - ly2;
			if (dy < 0)
				dy = -dy;

			if (dx <= 1)
			{
				vertex[3] = vertex[2];
				vertex[2].x = vertex[0].x;
			}
			else if (dy <= 1)
			{
				vertex[3] = vertex[2];
				vertex[2] = vertex[0];
				vertex[2].y = vertex[3].y;
			}
			else
				return false;

			bQuad = true;
		}
		else if (ly2 == ly0)
		{
			dy = ly2 - ly1;
			if (dy < 0)
				dy = -dy;

			if (dx <= 1)
			{
				vertex[3] = vertex[1];
				vertex[1].x = vertex[0].x;
			}
			else if (dy <= 1)
			{
				vertex[3] = vertex[1];
				vertex[1] = vertex[0];
				vertex[1].y = vertex[3].y;
			}
			else
				return false;

			bQuad = true;
		}
	}

	if (!bQuad)
		return false;

	PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[3], &vertex[2]);

	if (bDrawMultiPass)
	{
		SetSemiTransMulti(1);
		PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[3], &vertex[2]);
	}

	if (ubOpaqueDraw)
	{
		DEFOPAQUEON();
		PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[3], &vertex[2]);
		DEFOPAQUEOFF();
	}

	iDrawnSomething = 1;

	return true;
}

////////////////////////////////////////////////////////////////////////

static void primPolyFT3(uint8_t *baseAddr)
{
	uint32_t *gpuData = ((uint32_t *)baseAddr);
	int16_t *sgpuData = ((int16_t *)baseAddr);

	lx0 = sgpuData[2];
	ly0 = sgpuData[3];
	lx1 = sgpuData[6];
	ly1 = sgpuData[7];
	lx2 = sgpuData[10];
	ly2 = sgpuData[11];

	if (offset3())
		return;

	// do texture UV coordinates stuff
	gl_ux[0] = gl_ux[3] = baseAddr[8]; // gpuData[2]&0xff;
	gl_vy[0] = gl_vy[3] = baseAddr[9]; //(gpuData[2]>>8)&0xff;
	gl_ux[1] = baseAddr[16];           // gpuData[4]&0xff;
	gl_vy[1] = baseAddr[17];           //(gpuData[4]>>8)&0xff;
	gl_ux[2] = baseAddr[24];           // gpuData[6]&0xff;
	gl_vy[2] = baseAddr[25];           //(gpuData[6]>>8)&0xff;

	UpdateGlobalTP((uint16_t)(gpuData[4] >> 16));
	ulClutID = gpuData[2] >> 16;

	bDrawTextured = true;
	bDrawSmoothShaded = false;
	SetRenderState(gpuData[0]);

	offsetPSX3();
	if (bDrawOffscreen3())
	{
		InvalidateTextureAreaEx();
		SetRenderColor(gpuData[0]);
		drawPoly3FT(baseAddr);
	}

	SetRenderMode(gpuData[0], true);

	assignTexture3();

	if (DoLineCheck(gpuData))
		return;

	PRIMdrawTexturedTri(&vertex[0], &vertex[1], &vertex[2]);

	if (bDrawMultiPass)
	{
		SetSemiTransMulti(1);
		PRIMdrawTexturedTri(&vertex[0], &vertex[1], &vertex[2]);
	}

	if (ubOpaqueDraw)
	{
		DEFOPAQUEON();
		PRIMdrawTexturedTri(&vertex[0], &vertex[1], &vertex[2]);
		DEFOPAQUEOFF();
	}

	iDrawnSomething = 1;
}

////////////////////////////////////////////////////////////////////////
// cmd: flat shaded Texture4
////////////////////////////////////////////////////////////////////////

static void RectTexAlign(void)
{
	int32_t UFlipped = false;
	int32_t VFlipped = false;

	if (gTexName == gTexFrameName)
		return;

	if (ly0 == ly1)
	{
		if (!((lx1 == lx3 && ly3 == ly2 && lx2 == lx0) || (lx1 == lx2 && ly2 == ly3 && lx3 == lx0)))
			return;

		if (ly0 < ly2)
		{
			if (vertex[0].tow > vertex[2].tow)
				VFlipped = 1;
		}
		else
		{
			if (vertex[0].tow < vertex[2].tow)
				VFlipped = 2;
		}
	}
	else if (ly0 == ly2)
	{
		if (!((lx2 == lx3 && ly3 == ly1 && lx1 == lx0) || (lx2 == lx1 && ly1 == ly3 && lx3 == lx0)))
			return;

		if (ly0 < ly1)
		{
			if (vertex[0].tow > vertex[1].tow)
				VFlipped = 3;
		}
		else
		{
			if (vertex[0].tow < vertex[1].tow)
				VFlipped = 4;
		}
	}
	else if (ly0 == ly3)
	{
		if (!((lx3 == lx2 && ly2 == ly1 && lx1 == lx0) || (lx3 == lx1 && ly1 == ly2 && lx2 == lx0)))
			return;

		if (ly0 < ly1)
		{
			if (vertex[0].tow > vertex[1].tow)
				VFlipped = 5;
		}
		else
		{
			if (vertex[0].tow < vertex[1].tow)
				VFlipped = 6;
		}
	}
	else
		return;

	if (lx0 == lx1)
	{
		if (lx0 < lx2)
		{
			if (vertex[0].sow > vertex[2].sow)
				UFlipped = 1;
		}
		else
		{
			if (vertex[0].sow < vertex[2].sow)
				UFlipped = 2;
		}
	}
	else if (lx0 == lx2)
	{
		if (lx0 < lx1)
		{
			if (vertex[0].sow > vertex[1].sow)
				UFlipped = 3;
		}
		else
		{
			if (vertex[0].sow < vertex[1].sow)
				UFlipped = 4;
		}
	}
	else if (lx0 == lx3)
	{
		if (lx0 < lx1)
		{
			if (vertex[0].sow > vertex[1].sow)
				UFlipped = 5;
		}
		else
		{
			if (vertex[0].sow < vertex[1].sow)
				UFlipped = 6;
		}
	}

	if (UFlipped)
	{
		if (bUsingTWin)
		{
			switch (UFlipped)
			{
			case 1:
				vertex[2].sow += 1.0f / TWin.UScaleFactor;
				vertex[3].sow += 1.0f / TWin.UScaleFactor;
				break;
			case 2:
				vertex[0].sow += 1.0f / TWin.UScaleFactor;
				vertex[1].sow += 1.0f / TWin.UScaleFactor;
				break;
			case 3:
				vertex[1].sow += 1.0f / TWin.UScaleFactor;
				vertex[3].sow += 1.0f / TWin.UScaleFactor;
				break;
			case 4:
				vertex[0].sow += 1.0f / TWin.UScaleFactor;
				vertex[2].sow += 1.0f / TWin.UScaleFactor;
				break;
			case 5:
				vertex[1].sow += 1.0f / TWin.UScaleFactor;
				vertex[2].sow += 1.0f / TWin.UScaleFactor;
				break;
			case 6:
				vertex[0].sow += 1.0f / TWin.UScaleFactor;
				vertex[3].sow += 1.0f / TWin.UScaleFactor;
				break;
			}
		}
		else
		{
			switch (UFlipped)
			{
			case 1:
				vertex[2].sow += 1.0f;
				vertex[3].sow += 1.0f;
				break;
			case 2:
				vertex[0].sow += 1.0f;
				vertex[1].sow += 1.0f;
				break;
			case 3:
				vertex[1].sow += 1.0f;
				vertex[3].sow += 1.0f;
				break;
			case 4:
				vertex[0].sow += 1.0f;
				vertex[2].sow += 1.0f;
				break;
			case 5:
				vertex[1].sow += 1.0f;
				vertex[2].sow += 1.0f;
				break;
			case 6:
				vertex[0].sow += 1.0f;
				vertex[3].sow += 1.0f;
				break;
			}
		}
	}

	if (VFlipped)
	{
		if (bUsingTWin)
		{
			switch (VFlipped)
			{
			case 1:
				vertex[2].tow += 1.0f / TWin.VScaleFactor;
				vertex[3].tow += 1.0f / TWin.VScaleFactor;
				break;
			case 2:
				vertex[0].tow += 1.0f / TWin.VScaleFactor;
				vertex[1].tow += 1.0f / TWin.VScaleFactor;
				break;
			case 3:
				vertex[1].tow += 1.0f / TWin.VScaleFactor;
				vertex[3].tow += 1.0f / TWin.VScaleFactor;
				break;
			case 4:
				vertex[0].tow += 1.0f / TWin.VScaleFactor;
				vertex[2].tow += 1.0f / TWin.VScaleFactor;
				break;
			case 5:
				vertex[1].tow += 1.0f / TWin.VScaleFactor;
				vertex[2].tow += 1.0f / TWin.VScaleFactor;
				break;
			case 6:
				vertex[0].tow += 1.0f / TWin.VScaleFactor;
				vertex[3].tow += 1.0f / TWin.VScaleFactor;
				break;
			}
		}
		else
		{
			switch (VFlipped)
			{
			case 1:
				vertex[2].tow += 1.0f;
				vertex[3].tow += 1.0f;
				break;
			case 2:
				vertex[0].tow += 1.0f;
				vertex[1].tow += 1.0f;
				break;
			case 3:
				vertex[1].tow += 1.0f;
				vertex[3].tow += 1.0f;
				break;
			case 4:
				vertex[0].tow += 1.0f;
				vertex[2].tow += 1.0f;
				break;
			case 5:
				vertex[1].tow += 1.0f;
				vertex[2].tow += 1.0f;
				break;
			case 6:
				vertex[0].tow += 1.0f;
				vertex[3].tow += 1.0f;
				break;
			}
		}
	}
}

static void primPolyFT4(uint8_t *baseAddr)
{
	uint32_t *gpuData = ((uint32_t *)baseAddr);
	int16_t *sgpuData = ((int16_t *)baseAddr);

	lx0 = sgpuData[2];
	ly0 = sgpuData[3];
	lx1 = sgpuData[6];
	ly1 = sgpuData[7];
	lx2 = sgpuData[10];
	ly2 = sgpuData[11];
	lx3 = sgpuData[14];
	ly3 = sgpuData[15];

	if (offset4())
		return;

	gl_vy[0] = baseAddr[9];  //((gpuData[2]>>8)&0xff);
	gl_vy[1] = baseAddr[17]; //((gpuData[4]>>8)&0xff);
	gl_vy[2] = baseAddr[25]; //((gpuData[6]>>8)&0xff);
	gl_vy[3] = baseAddr[33]; //((gpuData[8]>>8)&0xff);

	gl_ux[0] = baseAddr[8];  //(gpuData[2]&0xff);
	gl_ux[1] = baseAddr[16]; //(gpuData[4]&0xff);
	gl_ux[2] = baseAddr[24]; //(gpuData[6]&0xff);
	gl_ux[3] = baseAddr[32]; //(gpuData[8]&0xff);

	UpdateGlobalTP((uint16_t)(gpuData[4] >> 16));
	ulClutID = (gpuData[2] >> 16);

	bDrawTextured = true;
	bDrawSmoothShaded = false;
	SetRenderState(gpuData[0]);

	offsetPSX4();
	if (bDrawOffscreen4())
	{
		InvalidateTextureAreaEx();
		SetRenderColor(gpuData[0]);
		drawPoly4FT(baseAddr);
	}

	SetRenderMode(gpuData[0], true);

	assignTexture4();

	RectTexAlign();

	PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[3], &vertex[2]);

	if (bDrawMultiPass)
	{
		SetSemiTransMulti(1);
		PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[3], &vertex[2]);
	}

	if (ubOpaqueDraw)
	{
		DEFOPAQUEON();

		PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[3], &vertex[2]);
		DEFOPAQUEOFF();
	}

	iDrawnSomething = 1;
}

////////////////////////////////////////////////////////////////////////
// cmd: smooth shaded Texture3
////////////////////////////////////////////////////////////////////////

static void primPolyGT3(uint8_t *baseAddr)
{
	uint32_t *gpuData = ((uint32_t *)baseAddr);
	int16_t *sgpuData = ((int16_t *)baseAddr);

	lx0 = sgpuData[2];
	ly0 = sgpuData[3];
	lx1 = sgpuData[8];
	ly1 = sgpuData[9];
	lx2 = sgpuData[14];
	ly2 = sgpuData[15];

	if (offset3())
		return;

	// do texture stuff
	gl_ux[0] = gl_ux[3] = baseAddr[8]; // gpuData[2]&0xff;
	gl_vy[0] = gl_vy[3] = baseAddr[9]; //(gpuData[2]>>8)&0xff;
	gl_ux[1] = baseAddr[20];           // gpuData[5]&0xff;
	gl_vy[1] = baseAddr[21];           //(gpuData[5]>>8)&0xff;
	gl_ux[2] = baseAddr[32];           // gpuData[8]&0xff;
	gl_vy[2] = baseAddr[33];           //(gpuData[8]>>8)&0xff;

	UpdateGlobalTP((uint16_t)(gpuData[5] >> 16));
	ulClutID = (gpuData[2] >> 16);

	bDrawTextured = true;
	bDrawSmoothShaded = true;
	SetRenderState(gpuData[0]);

	offsetPSX3();
	if (bDrawOffscreen3())
	{
		InvalidateTextureAreaEx();
		drawPoly3GT(baseAddr);
	}

	SetRenderMode(gpuData[0], false);

	assignTexture3();

	if (bDrawNonShaded)
	{
		vertex[0].c.lcol = 0xffffff;
		vertex[0].c.col[3] = ubGloAlpha;
		glColor4ubv(vertex[0].c.col);

		PRIMdrawTexturedTri(&vertex[0], &vertex[1], &vertex[2]);

		if (ubOpaqueDraw)
		{
			DEFOPAQUEON();
			PRIMdrawTexturedTri(&vertex[0], &vertex[1], &vertex[2]);
			DEFOPAQUEOFF();
		}
		return;
	}

	vertex[0].c.lcol = DoubleBGR2RGB(gpuData[0]);
	vertex[1].c.lcol = DoubleBGR2RGB(gpuData[3]);
	vertex[2].c.lcol = DoubleBGR2RGB(gpuData[6]);

	vertex[0].c.col[3] = vertex[1].c.col[3] = vertex[2].c.col[3] = ubGloAlpha;

	PRIMdrawTexGouraudTriColor(&vertex[0], &vertex[1], &vertex[2]);

	if (bDrawMultiPass)
	{
		SetSemiTransMulti(1);
		PRIMdrawTexGouraudTriColor(&vertex[0], &vertex[1], &vertex[2]);
	}

	if (ubOpaqueDraw)
	{
		DEFOPAQUEON();
		PRIMdrawTexGouraudTriColor(&vertex[0], &vertex[1], &vertex[2]);
		DEFOPAQUEOFF();
	}

	iDrawnSomething = 1;
}

////////////////////////////////////////////////////////////////////////
// cmd: smooth shaded Poly3
////////////////////////////////////////////////////////////////////////

static void primPolyG3(uint8_t *baseAddr)
{
	uint32_t *gpuData = ((uint32_t *)baseAddr);
	int16_t *sgpuData = ((int16_t *)baseAddr);

	lx0 = sgpuData[2];
	ly0 = sgpuData[3];
	lx1 = sgpuData[6];
	ly1 = sgpuData[7];
	lx2 = sgpuData[10];
	ly2 = sgpuData[11];

	if (offset3())
		return;

	bDrawTextured = false;
	bDrawSmoothShaded = true;
	SetRenderState(gpuData[0]);

	offsetPSX3();
	if (bDrawOffscreen3())
	{
		InvalidateTextureAreaEx();
		drawPoly3G(gpuData[0], gpuData[2], gpuData[4]);
	}

	SetRenderMode(gpuData[0], false);

	vertex[0].c.lcol = gpuData[0];
	vertex[1].c.lcol = gpuData[2];
	vertex[2].c.lcol = gpuData[4];
	vertex[0].c.col[3] = vertex[1].c.col[3] = vertex[2].c.col[3] = ubGloColAlpha;

	PRIMdrawGouraudTriColor(&vertex[0], &vertex[1], &vertex[2]);

	iDrawnSomething = 1;
}

////////////////////////////////////////////////////////////////////////
// cmd: smooth shaded Texture4
////////////////////////////////////////////////////////////////////////

static void primPolyGT4(uint8_t *baseAddr)
{
	uint32_t *gpuData = ((uint32_t *)baseAddr);
	int16_t *sgpuData = ((int16_t *)baseAddr);

	lx0 = sgpuData[2];
	ly0 = sgpuData[3];
	lx1 = sgpuData[8];
	ly1 = sgpuData[9];
	lx2 = sgpuData[14];
	ly2 = sgpuData[15];
	lx3 = sgpuData[20];
	ly3 = sgpuData[21];

	if (offset4())
		return;

	// do texture stuff
	gl_ux[0] = baseAddr[8];  // gpuData[2]&0xff;
	gl_vy[0] = baseAddr[9];  //(gpuData[2]>>8)&0xff;
	gl_ux[1] = baseAddr[20]; // gpuData[5]&0xff;
	gl_vy[1] = baseAddr[21]; //(gpuData[5]>>8)&0xff;
	gl_ux[2] = baseAddr[32]; // gpuData[8]&0xff;
	gl_vy[2] = baseAddr[33]; //(gpuData[8]>>8)&0xff;
	gl_ux[3] = baseAddr[44]; // gpuData[11]&0xff;
	gl_vy[3] = baseAddr[45]; //(gpuData[11]>>8)&0xff;

	UpdateGlobalTP((uint16_t)(gpuData[5] >> 16));
	ulClutID = (gpuData[2] >> 16);

	bDrawTextured = true;
	bDrawSmoothShaded = true;
	SetRenderState(gpuData[0]);

	offsetPSX4();
	if (bDrawOffscreen4())
	{
		InvalidateTextureAreaEx();
		drawPoly4GT(baseAddr);
	}

	SetRenderMode(gpuData[0], false);

	assignTexture4();

	RectTexAlign();

	if (bDrawNonShaded)
	{
		vertex[0].c.lcol = 0xffffff;
		vertex[0].c.col[3] = ubGloAlpha;
		glColor4ubv(vertex[0].c.col);

		PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[3], &vertex[2]);

		if (ubOpaqueDraw)
		{
			ubGloAlpha = ubGloColAlpha = 0xff;
			DEFOPAQUEON();
			PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[3], &vertex[2]);
			DEFOPAQUEOFF();
		}
		return;
	}

	vertex[0].c.lcol = DoubleBGR2RGB(gpuData[0]);
	vertex[1].c.lcol = DoubleBGR2RGB(gpuData[3]);
	vertex[2].c.lcol = DoubleBGR2RGB(gpuData[6]);
	vertex[3].c.lcol = DoubleBGR2RGB(gpuData[9]);

	vertex[0].c.col[3] = vertex[1].c.col[3] = vertex[2].c.col[3] = vertex[3].c.col[3] = ubGloAlpha;

	PRIMdrawTexGouraudTriColorQuad(&vertex[0], &vertex[1], &vertex[3], &vertex[2]);

	if (bDrawMultiPass)
	{
		SetSemiTransMulti(1);
		PRIMdrawTexGouraudTriColorQuad(&vertex[0], &vertex[1], &vertex[3], &vertex[2]);
	}

	if (ubOpaqueDraw)
	{
		ubGloAlpha = ubGloColAlpha = 0xff;
		DEFOPAQUEON();
		PRIMdrawTexGouraudTriColorQuad(&vertex[0], &vertex[1], &vertex[3], &vertex[2]);
		DEFOPAQUEOFF();
	}

	iDrawnSomething = 1;
}

////////////////////////////////////////////////////////////////////////
// cmd: smooth shaded Poly3
////////////////////////////////////////////////////////////////////////

static void primPolyF3(uint8_t *baseAddr)
{
	uint32_t *gpuData = ((uint32_t *)baseAddr);
	int16_t *sgpuData = ((int16_t *)baseAddr);

	lx0 = sgpuData[2];
	ly0 = sgpuData[3];
	lx1 = sgpuData[4];
	ly1 = sgpuData[5];
	lx2 = sgpuData[6];
	ly2 = sgpuData[7];

	if (offset3())
		return;

	bDrawTextured = false;
	bDrawSmoothShaded = false;
	SetRenderState(gpuData[0]);

	offsetPSX3();
	if (bDrawOffscreen3())
	{
		InvalidateTextureAreaEx();
		drawPoly3F(gpuData[0]);
	}

	SetRenderMode(gpuData[0], false);

	vertex[0].c.lcol = gpuData[0];
	vertex[0].c.col[3] = ubGloColAlpha;
	glColor4ubv(vertex[0].c.col);

	PRIMdrawTri(&vertex[0], &vertex[1], &vertex[2]);

	iDrawnSomething = 1;
}

////////////////////////////////////////////////////////////////////////
// cmd: shaded polylines
////////////////////////////////////////////////////////////////////////

static void primLineGEx(uint8_t *baseAddr)
{
	uint32_t *gpuData = ((uint32_t *)baseAddr);
	int32_t iMax = 255;
	int16_t cx0, cx1, cy0, cy1;
	int32_t i;
	bool bDraw = true;

	bDrawTextured = false;
	bDrawSmoothShaded = true;
	SetRenderState(gpuData[0]);
	SetRenderMode(gpuData[0], false);

	vertex[0].c.lcol = vertex[3].c.lcol = gpuData[0];
	vertex[0].c.col[3] = vertex[3].c.col[3] = ubGloColAlpha;
	ly1 = (int16_t)((gpuData[1] >> 16) & 0xffff);
	lx1 = (int16_t)(gpuData[1] & 0xffff);

	i = 2;

	// while((gpuData[i]>>24)!=0x55)
	// while((gpuData[i]&0x50000000)!=0x50000000)
	// currently best way to check for poly line end:
	while (!(((gpuData[i] & 0xF000F000) == 0x50005000) && i >= 4))
	{
		ly0 = ly1;
		lx0 = lx1;
		vertex[1].c.lcol = vertex[2].c.lcol = vertex[0].c.lcol;
		vertex[0].c.lcol = vertex[3].c.lcol = gpuData[i];
		vertex[0].c.col[3] = vertex[3].c.col[3] = ubGloColAlpha;

		i++;

		ly1 = (int16_t)((gpuData[i] >> 16) & 0xffff);
		lx1 = (int16_t)(gpuData[i] & 0xffff);

		if (offsetline())
			bDraw = false;
		else
			bDraw = true;

		if (bDraw && ((lx0 != lx1) || (ly0 != ly1)))
		{

			cx0 = lx0;
			cx1 = lx1;
			cy0 = ly0;
			cy1 = ly1;
			offsetPSXLine();
			if (bDrawOffscreen4())
			{
				InvalidateTextureAreaEx();
				drawPoly4G(gpuData[i - 3], gpuData[i - 1], gpuData[i - 3], gpuData[i - 1]);
			}
			lx0 = cx0;
			lx1 = cx1;
			ly0 = cy0;
			ly1 = cy1;

			PRIMdrawGouraudLine(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);
		}
		i++;

		if (i > iMax)
			break;
	}

	iDrawnSomething = 1;
}

////////////////////////////////////////////////////////////////////////
// cmd: shaded polyline2
////////////////////////////////////////////////////////////////////////

static void primLineG2(uint8_t *baseAddr)
{
	uint32_t *gpuData = ((uint32_t *)baseAddr);
	int16_t *sgpuData = ((int16_t *)baseAddr);

	lx0 = sgpuData[2];
	ly0 = sgpuData[3];
	lx1 = sgpuData[6];
	ly1 = sgpuData[7];

	vertex[0].c.lcol = vertex[3].c.lcol = gpuData[0];
	vertex[1].c.lcol = vertex[2].c.lcol = gpuData[2];
	vertex[0].c.col[3] = vertex[1].c.col[3] = vertex[2].c.col[3] = vertex[3].c.col[3] = ubGloColAlpha;

	bDrawTextured = false;
	bDrawSmoothShaded = true;

	if ((lx0 == lx1) && (ly0 == ly1))
		return;

	if (offsetline())
		return;

	SetRenderState(gpuData[0]);
	SetRenderMode(gpuData[0], false);

	offsetPSXLine();
	if (bDrawOffscreen4())
	{
		InvalidateTextureAreaEx();
		drawPoly4G(gpuData[0], gpuData[2], gpuData[0], gpuData[2]);
	}

	// if(ClipVertexList4())
	PRIMdrawGouraudLine(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);

	iDrawnSomething = 1;
}

////////////////////////////////////////////////////////////////////////
// cmd: drawing flat polylines
////////////////////////////////////////////////////////////////////////

static void primLineFEx(uint8_t *baseAddr)
{
	uint32_t *gpuData = ((uint32_t *)baseAddr);
	int32_t iMax;
	int16_t cx0, cx1, cy0, cy1;
	int32_t i;

	iMax = 255;

	bDrawTextured = false;
	bDrawSmoothShaded = false;
	SetRenderState(gpuData[0]);
	SetRenderMode(gpuData[0], false);

	vertex[0].c.lcol = gpuData[0];
	vertex[0].c.col[3] = ubGloColAlpha;

	ly1 = (int16_t)((gpuData[1] >> 16) & 0xffff);
	lx1 = (int16_t)(gpuData[1] & 0xffff);

	i = 2;

	// while(!(gpuData[i]&0x40000000))
	// while((gpuData[i]>>24)!=0x55)
	// while((gpuData[i]&0x50000000)!=0x50000000)
	// currently best way to check for poly line end:
	while (!(((gpuData[i] & 0xF000F000) == 0x50005000) && i >= 3))
	{
		ly0 = ly1;
		lx0 = lx1;
		ly1 = (int16_t)((gpuData[i] >> 16) & 0xffff);
		lx1 = (int16_t)(gpuData[i] & 0xffff);

		if (!offsetline())
		{
			cx0 = lx0;
			cx1 = lx1;
			cy0 = ly0;
			cy1 = ly1;
			offsetPSXLine();
			if (bDrawOffscreen4())
			{
				InvalidateTextureAreaEx();
				drawPoly4F(gpuData[0]);
			}
			lx0 = cx0;
			lx1 = cx1;
			ly0 = cy0;
			ly1 = cy1;
			PRIMdrawFlatLine(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);
		}

		i++;
		if (i > iMax)
			break;
	}

	iDrawnSomething = 1;
}

////////////////////////////////////////////////////////////////////////
// cmd: drawing flat polyline2
////////////////////////////////////////////////////////////////////////

static void primLineF2(uint8_t *baseAddr)
{
	uint32_t *gpuData = ((uint32_t *)baseAddr);
	int16_t *sgpuData = ((int16_t *)baseAddr);

	lx0 = sgpuData[2];
	ly0 = sgpuData[3];
	lx1 = sgpuData[4];
	ly1 = sgpuData[5];

	if (offsetline())
		return;

	bDrawTextured = false;
	bDrawSmoothShaded = false;
	SetRenderState(gpuData[0]);
	SetRenderMode(gpuData[0], false);

	vertex[0].c.lcol = gpuData[0];
	vertex[0].c.col[3] = ubGloColAlpha;

	offsetPSXLine();
	if (bDrawOffscreen4())
	{
		InvalidateTextureAreaEx();
		drawPoly4F(gpuData[0]);
	}

	// if(ClipVertexList4())
	PRIMdrawFlatLine(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);

	iDrawnSomething = 1;
}

////////////////////////////////////////////////////////////////////////
// cmd: well, easiest command... not implemented
////////////////////////////////////////////////////////////////////////

static void primNI(uint8_t *)
{
}

////////////////////////////////////////////////////////////////////////
// cmd func ptr table
////////////////////////////////////////////////////////////////////////

void (*primTableJ[256])(uint8_t *) = {
    // 00
    primNI,         primNI,         primBlkFill,      primNI,
    primNI,         primNI,         primNI,           primNI,
    // 08
    primNI,         primNI,         primNI,           primNI,
    primNI,         primNI,         primNI,           primNI,
    // 10
    primNI,         primNI,         primNI,           primNI,
    primNI,         primNI,         primNI,           primNI,
    // 18
    primNI,         primNI,         primNI,           primNI,
    primNI,         primNI,         primNI,           primNI,
    // 20
    primPolyF3,     primPolyF3,     primPolyF3,       primPolyF3,
    primPolyFT3,    primPolyFT3,    primPolyFT3,      primPolyFT3,
    // 28
    primPolyF4,     primPolyF4,     primPolyF4,       primPolyF4,
    primPolyFT4,    primPolyFT4,    primPolyFT4,      primPolyFT4,
    // 30
    primPolyG3,     primPolyG3,     primPolyG3,       primPolyG3,
    primPolyGT3,    primPolyGT3,    primPolyGT3,      primPolyGT3,
    // 38
    primPolyG4,     primPolyG4,     primPolyG4,       primPolyG4,
    primPolyGT4,    primPolyGT4,    primPolyGT4,      primPolyGT4,
    // 40
    primLineF2,     primLineF2,     primLineF2,       primLineF2,
    primNI,         primNI,         primNI,           primNI,
    // 48
    primLineFEx,    primLineFEx,    primLineFEx,      primLineFEx,
    primLineFEx,    primLineFEx,    primLineFEx,      primLineFEx,
    // 50
    primLineG2,     primLineG2,     primLineG2,       primLineG2,
    primNI,         primNI,         primNI,           primNI,
    // 58
    primLineGEx,    primLineGEx,    primLineGEx,      primLineGEx,
    primLineGEx,    primLineGEx,    primLineGEx,      primLineGEx,
    // 60
    primTileS,      primTileS,      primTileS,        primTileS,
    primSprtS,      primSprtS,      primSprtS,        primSprtS,
    // 68
    primTile1,      primTile1,      primTile1,        primTile1,
    primNI,         primNI,         primNI,           primNI,
    // 70
    primTile8,      primTile8,      primTile8,        primTile8,
    primSprt8,      primSprt8,      primSprt8,        primSprt8,
    // 78
    primTile16,     primTile16,     primTile16,       primTile16,
    primSprt16,     primSprt16,     primSprt16,       primSprt16,
    // 80
    primMoveImage,  primNI,         primNI,           primNI,
    primNI,         primNI,         primNI,           primNI,
    // 88
    primNI,         primNI,         primNI,           primNI,
    primNI,         primNI,         primNI,           primNI,
    // 90
    primNI,         primNI,         primNI,           primNI,
    primNI,         primNI,         primNI,           primNI,
    // 98
    primNI,         primNI,         primNI,           primNI,
    primNI,         primNI,         primNI,           primNI,
    // a0
    primLoadImage,  primNI,         primNI,           primNI,
    primNI,         primNI,         primNI,           primNI,
    // a8
    primNI,         primNI,         primNI,           primNI,
    primNI,         primNI,         primNI,           primNI,
    // b0
    primNI,         primNI,         primNI,           primNI,
    primNI,         primNI,         primNI,           primNI,
    // b8
    primNI,         primNI,         primNI,           primNI,
    primNI,         primNI,         primNI,           primNI,
    // c0
    primStoreImage, primNI,         primNI,           primNI,
    primNI,         primNI,         primNI,           primNI,
    // c8
    primNI,         primNI,         primNI,           primNI,
    primNI,         primNI,         primNI,           primNI,
    // d0
    primNI,         primNI,         primNI,           primNI,
    primNI,         primNI,         primNI,           primNI,
    // d8
    primNI,         primNI,         primNI,           primNI,
    primNI,         primNI,         primNI,           primNI,
    // e0
    primNI,         cmdTexturePage, cmdTextureWindow, cmdDrawAreaStart,
    cmdDrawAreaEnd, cmdDrawOffset,  cmdSTP,           primNI,
    // e8
    primNI,         primNI,         primNI,           primNI,
    primNI,         primNI,         primNI,           primNI,
    // f0
    primNI,         primNI,         primNI,           primNI,
    primNI,         primNI,         primNI,           primNI,
    // f8
    primNI,         primNI,         primNI,           primNI,
    primNI,         primNI,         primNI,           primNI};
