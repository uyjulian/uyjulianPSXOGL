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

// gl utils end

////////////////////////////////////////////////////////////////////////
// Initialize OGL
////////////////////////////////////////////////////////////////////////

int GLinitialize()
{
	glViewport(0, 0, iResX, iResY);

	glMatrixMode(GL_TEXTURE); // init psx tex sow and tow
	glLoadIdentity();
	glScalef(1.0f / 256.0f, 1.0f / 256.0f, 1.0f);

	glMatrixMode(GL_PROJECTION); // init projection with psx resolution
	glLoadIdentity();
	glOrtho(0, PSXDisplay.DisplayMode.x, PSXDisplay.DisplayMode.y, 0, -1, 1);

	clearToBlack();

	glPolygonMode(GL_FRONT, GL_FILL);
	glPolygonMode(GL_BACK, GL_FILL);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	TCF[0] = XP8RGBA_0;

	TCF[1] = XP8RGBA_1;
	glAlphaFunc(GL_GREATER, 0.49f);

	LoadSubTexFn = LoadSubTexturePageSort; // init load tex ptr

	glEnable(GL_ALPHA_TEST); // wanna alpha test

	ubGloAlpha = 127; // init some drawing vars
	ubGloColAlpha = 127;
	TWin.UScaleFactor = 1;
	TWin.VScaleFactor = 1;
	bDrawMultiPass = false;
	bTexEnabled = false;
	bUsingTWin = false;

	CheckTextureMemory(); // check available tex memory

	return 0;
}

int GLrefresh()
{
	glViewport(0, 0, iResX, iResY);

	glMatrixMode(GL_PROJECTION); // init projection with psx resolution
	glLoadIdentity();
	glOrtho(0, PSXDisplay.DisplayMode.x, PSXDisplay.DisplayMode.y, 0, -1, 1);

	clearToBlack();

	return 0;
}

void GLcleanup()
{
	CleanupTextureStore();
}

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
		PSXDisplay.GDrawOffset.x = 0;
		PSXDisplay.GDrawOffset.y = 0;

		PSXDisplay.CumulOffset.x = PSXDisplay.DrawOffset.x + PreviousPSXDisplay.Range.x0;
		PSXDisplay.CumulOffset.y = PSXDisplay.DrawOffset.y + PreviousPSXDisplay.Range.y0;

		rprev.left = rprev.left + 1;
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

	XS = (float)iResX / (float)PSXDisplay.DisplayMode.x;
	YS = (float)iResY / (float)PSXDisplay.DisplayMode.y;

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
}
