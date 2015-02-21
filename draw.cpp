/***************************************************************************
                           draw.c  -  description
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

////////////////////////////////////////////////////////////////////////////////////
// draw globals; most will be initialized again later (by config or checks)

// resolution/ratio vars

int iResX = 640;
int iResY = 480;
RECT rRatioRect;

// drawing/coord vars

OGLVertex vertex[4];
GLubyte gl_ux[8];
GLubyte gl_vy[8];
int16_t sprtY, sprtX, sprtH, sprtW;

// gl utils

void clearWithColor(int clearColor)
{
	GLclampf g, b, r;

	g = ((GLclampf)GREEN(clearColor)) / 255.0f;
	b = ((GLclampf)BLUE(clearColor)) / 255.0f;
	r = ((GLclampf)RED(clearColor)) / 255.0f;

	glClearColor(r, g, b, 1);
	glClear(GL_COLOR_BUFFER_BIT);
}

void clearToBlack(void)
{
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
}


OGLVertexGLM OGLVertexToOGLVertexGLM(OGLVertex from)
{
	OGLVertexGLM to;

	to.Position.x = from.x;
	to.Position.y = from.y;
	to.Position.z = from.z;
	to.Texture.x = from.sow;
	to.Texture.y = from.tow;
	to.Color.r = from.c.col[0];
	to.Color.g = from.c.col[1];
	to.Color.b = from.c.col[2];
	to.Color.a = from.c.col[3];
	to.LColor = from.c.lcol;
	return to;
}

OGLVertex OGLVertexGLMToOGLVertex(OGLVertexGLM from)
{
	OGLVertex to;
	to.x = from.Position.x;
	to.y = from.Position.y;
	to.z = from.Position.z;
	to.sow = from.Texture.x;
	to.tow = from.Texture.y;
	to.c.col[0] = from.Color.r;
	to.c.col[1] = from.Color.g;
	to.c.col[2] = from.Color.b;
	to.c.col[3] = from.Color.a;
	to.c.lcol = from.LColor;
	return to;
}

// gl utils end

////////////////////////////////////////////////////////////////////////
// Setup some stuff depending on user settings or in-game toggle
////////////////////////////////////////////////////////////////////////

void SetExtGLFuncs(void)
{

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	TCF[0] = XP8RGBA_0;
	PalTexturedColourFn = XP8RGBA; // -> init col func

	TCF[1] = XP8RGBA_1;
	glAlphaFunc(GL_GREATER, 0.49f);


	//----------------------------------------------------//

	LoadSubTexFn = LoadSubTexturePageSort; // init load tex ptr

	glDisable(GL_BLEND);

}


////////////////////////////////////////////////////////////////////////
// Initialize OGL
////////////////////////////////////////////////////////////////////////

int GLinitialize()
{
	glViewport(rRatioRect.left, // init viewport by ratio rect
	           iResY - (rRatioRect.top + rRatioRect.bottom), rRatioRect.right, rRatioRect.bottom);

	glMatrixMode(GL_TEXTURE); // init psx tex sow and tow 
	glLoadIdentity();
	glScalef(1.0f / 255.99f, 1.0f / 255.99f, 1.0f); // geforce precision hack

	glMatrixMode(GL_PROJECTION); // init projection with psx resolution
	glLoadIdentity();
	glOrtho(0, PSXDisplay.DisplayMode.x, PSXDisplay.DisplayMode.y, 0, -1, 1);

	glDisable(GL_DEPTH_TEST);

	clearToBlack();

	glPolygonMode(GL_FRONT, GL_FILL);
	glPolygonMode(GL_BACK, GL_FILL);

	SetExtGLFuncs();    // init all kind of stuff (tex function pointers)

	glEnable(GL_ALPHA_TEST); // wanna alpha test

	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_POLYGON_SMOOTH);
	glDisable(GL_POINT_SMOOTH);

	ubGloAlpha = 127; // init some drawing vars
	ubGloColAlpha = 127;
	TWin.UScaleFactor = 1;
	TWin.VScaleFactor = 1;
	bDrawMultiPass = false;
	bTexEnabled = false;
	bUsingTWin = false;

	glDisable(GL_DITHER);

	glDisable(GL_FOG); // turn all (currently) unused modes off
	glDisable(GL_LIGHTING);
	glDisable(GL_LOGIC_OP);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_TEXTURE_1D);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);

	glPixelTransferi(GL_RED_SCALE, 1); // to be sure:
	glPixelTransferi(GL_RED_BIAS, 0);  // init more OGL vals
	glPixelTransferi(GL_GREEN_SCALE, 1);
	glPixelTransferi(GL_GREEN_BIAS, 0);
	glPixelTransferi(GL_BLUE_SCALE, 1);
	glPixelTransferi(GL_BLUE_BIAS, 0);
	glPixelTransferi(GL_ALPHA_SCALE, 1);
	glPixelTransferi(GL_ALPHA_BIAS, 0);

	glFlush(); // we are done...
	glFinish();

	CheckTextureMemory(); // check available tex memory

	return 0;
}


int GLrefresh()
{
	glViewport(rRatioRect.left, // init viewport by ratio rect
	           iResY - (rRatioRect.top + rRatioRect.bottom), rRatioRect.right, rRatioRect.bottom);

	glMatrixMode(GL_PROJECTION); // init projection with psx resolution
	glLoadIdentity();
	glOrtho(0, PSXDisplay.DisplayMode.x, PSXDisplay.DisplayMode.y, 0, -1, 1);

	clearToBlack();

	return 0;
}

////////////////////////////////////////////////////////////////////////
// clean up OGL stuff
////////////////////////////////////////////////////////////////////////

void GLcleanup()
{
	CleanupTextureStore(); // bye textures
}

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

// Pete's way: a very easy (and hopefully fast) approach for lines
// without sqrt... using a small float -> int16_t cast trick :)



bool offsetline(void)
{
	int16_t x0, x1, y0, y1, dx, dy;
	float px, py;

	if (bDisplayNotSet)
		SetOGLDisplaySettings(1);


	lx0 = (int16_t)(((int)lx0 << SIGNSHIFT) >> SIGNSHIFT);
	lx1 = (int16_t)(((int)lx1 << SIGNSHIFT) >> SIGNSHIFT);
	ly0 = (int16_t)(((int)ly0 << SIGNSHIFT) >> SIGNSHIFT);
	ly1 = (int16_t)(((int)ly1 << SIGNSHIFT) >> SIGNSHIFT);

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

/////////////////////////////////////////////////////////

bool offset3(void)
{
	if (bDisplayNotSet)
		SetOGLDisplaySettings(1);

	lx0 = (int16_t)(((int)lx0 << SIGNSHIFT) >> SIGNSHIFT);
	lx1 = (int16_t)(((int)lx1 << SIGNSHIFT) >> SIGNSHIFT);
	lx2 = (int16_t)(((int)lx2 << SIGNSHIFT) >> SIGNSHIFT);
	ly0 = (int16_t)(((int)ly0 << SIGNSHIFT) >> SIGNSHIFT);
	ly1 = (int16_t)(((int)ly1 << SIGNSHIFT) >> SIGNSHIFT);
	ly2 = (int16_t)(((int)ly2 << SIGNSHIFT) >> SIGNSHIFT);

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

/////////////////////////////////////////////////////////

bool offset4(void)
{
	if (bDisplayNotSet)
		SetOGLDisplaySettings(1);

	lx0 = (int16_t)(((int)lx0 << SIGNSHIFT) >> SIGNSHIFT);
	lx1 = (int16_t)(((int)lx1 << SIGNSHIFT) >> SIGNSHIFT);
	lx2 = (int16_t)(((int)lx2 << SIGNSHIFT) >> SIGNSHIFT);
	lx3 = (int16_t)(((int)lx3 << SIGNSHIFT) >> SIGNSHIFT);
	ly0 = (int16_t)(((int)ly0 << SIGNSHIFT) >> SIGNSHIFT);
	ly1 = (int16_t)(((int)ly1 << SIGNSHIFT) >> SIGNSHIFT);
	ly2 = (int16_t)(((int)ly2 << SIGNSHIFT) >> SIGNSHIFT);
	ly3 = (int16_t)(((int)ly3 << SIGNSHIFT) >> SIGNSHIFT);

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

/////////////////////////////////////////////////////////

void offsetST(void)
{
	if (bDisplayNotSet)
		SetOGLDisplaySettings(1);

	lx0 = (int16_t)(((int)lx0 << SIGNSHIFT) >> SIGNSHIFT);
	ly0 = (int16_t)(((int)ly0 << SIGNSHIFT) >> SIGNSHIFT);

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

/////////////////////////////////////////////////////////

void offsetScreenUpload(int Position)
{
	if (bDisplayNotSet)
		SetOGLDisplaySettings(1);

	if (Position == -1)
	{
		int lmdx, lmdy;

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

/////////////////////////////////////////////////////////

void offsetBlk(void)
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

////////////////////////////////////////////////////////////////////////
// texture sow/tow calculations
////////////////////////////////////////////////////////////////////////

void assignTextureVRAMWrite(void)
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

void assignTextureSprite(void)
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

void assignTexture3(void)
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

void assignTexture4(void)
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
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// render pos / buffers
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
// SetDisplaySettings: "simply" calcs the new drawing area and updates
//                     the ogl clipping (scissor)

void SetOGLDisplaySettings(bool DisplaySet)
{
	static RECT rprev = {0, 0, 0, 0};
	static int iOldX = 0;
	static int iOldY = 0;
	RECT r;
	float XS, YS;

	bDisplayNotSet = false;

	//----------------------------------------------------// that's a whole screen upload
	if (!DisplaySet)
	{
		RECT rX;
		PSXDisplay.GDrawOffset.x = 0;
		PSXDisplay.GDrawOffset.y = 0;

		PSXDisplay.CumulOffset.x = PSXDisplay.DrawOffset.x + PreviousPSXDisplay.Range.x0;
		PSXDisplay.CumulOffset.y = PSXDisplay.DrawOffset.y + PreviousPSXDisplay.Range.y0;

		rprev.left = rprev.left + 1;

		rX = rRatioRect;
		rX.top = iResY - (rRatioRect.top + rRatioRect.bottom);

		return;
	}
	//----------------------------------------------------//

	PSXDisplay.GDrawOffset.y = PreviousPSXDisplay.DisplayPosition.y;
	PSXDisplay.GDrawOffset.x = PreviousPSXDisplay.DisplayPosition.x;
	PSXDisplay.CumulOffset.x = PSXDisplay.DrawOffset.x - PSXDisplay.GDrawOffset.x + PreviousPSXDisplay.Range.x0;
	PSXDisplay.CumulOffset.y = PSXDisplay.DrawOffset.y - PSXDisplay.GDrawOffset.y + PreviousPSXDisplay.Range.y0;

	r.top = PSXDisplay.DrawArea.y0 - PreviousPSXDisplay.DisplayPosition.y;
	r.bottom = PSXDisplay.DrawArea.y1 - PreviousPSXDisplay.DisplayPosition.y;

	if (r.bottom < 0 || r.top >= PSXDisplay.DisplayMode.y)
	{
		r.top = PSXDisplay.DrawArea.y0 - PSXDisplay.DisplayPosition.y;
		r.bottom = PSXDisplay.DrawArea.y1 - PSXDisplay.DisplayPosition.y;
	}

	r.left = PSXDisplay.DrawArea.x0 - PreviousPSXDisplay.DisplayPosition.x;
	r.right = PSXDisplay.DrawArea.x1 - PreviousPSXDisplay.DisplayPosition.x;

	if (r.right < 0 || r.left >= PSXDisplay.DisplayMode.x)
	{
		r.left = PSXDisplay.DrawArea.x0 - PSXDisplay.DisplayPosition.x;
		r.right = PSXDisplay.DrawArea.x1 - PSXDisplay.DisplayPosition.x;
	}

	if (EqualRect(&r, &rprev) && iOldX == PSXDisplay.DisplayMode.x && iOldY == PSXDisplay.DisplayMode.y)
		return;

	rprev = r;
	iOldX = PSXDisplay.DisplayMode.x;
	iOldY = PSXDisplay.DisplayMode.y;

	XS = (float)rRatioRect.right / (float)PSXDisplay.DisplayMode.x;
	YS = (float)rRatioRect.bottom / (float)PSXDisplay.DisplayMode.y;

	if (PreviousPSXDisplay.Range.x0)
	{
		int16_t s = PreviousPSXDisplay.Range.x0 + PreviousPSXDisplay.Range.x1;

		r.left += PreviousPSXDisplay.Range.x0 + 1;

		r.right += PreviousPSXDisplay.Range.x0;

		if (r.left > s)
			r.left = s;
		if (r.right > s)
			r.right = s;
	}

	if (PreviousPSXDisplay.Range.y0)
	{
		int16_t s = PreviousPSXDisplay.Range.y0 + PreviousPSXDisplay.Range.y1;

		r.top += PreviousPSXDisplay.Range.y0 + 1;
		r.bottom += PreviousPSXDisplay.Range.y0;

		if (r.top > s)
			r.top = s;
		if (r.bottom > s)
			r.bottom = s;
	}

	// Set the ClipArea variables to reflect the new screen,
	// offset from zero (since it is a new display buffer)
	r.left = (int)(((float)(r.left)) * XS);
	r.top = (int)(((float)(r.top)) * YS);
	r.right = (int)(((float)(r.right + 1)) * XS);
	r.bottom = (int)(((float)(r.bottom + 1)) * YS);

	// Limit clip area to the screen size
	if (r.left > iResX)
		r.left = iResX;
	if (r.left < 0)
		r.left = 0;
	if (r.top > iResY)
		r.top = iResY;
	if (r.top < 0)
		r.top = 0;
	if (r.right > iResX)
		r.right = iResX;
	if (r.right < 0)
		r.right = 0;
	if (r.bottom > iResY)
		r.bottom = iResY;
	if (r.bottom < 0)
		r.bottom = 0;

	r.right -= r.left;
	r.bottom -= r.top;
	r.top = iResY - (r.top + r.bottom);

	r.left += rRatioRect.left;
	r.top -= rRatioRect.top;

}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
