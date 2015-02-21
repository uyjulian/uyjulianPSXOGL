/***************************************************************************
                           gpu.c  -  description
                             -------------------
    begin                : Sun Mar 08 2009
    copyright            : (C) 1999-2009 by Pete Bernert
    email                : BlackDove@addcom.de
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
// memory image of the PSX vram
////////////////////////////////////////////////////////////////////////

static uint8_t *psxVSecure;
uint8_t *psxVub;
static int8_t *psxVsb;
uint16_t *psxVuw;
static uint16_t *psxVuw_eom;
static int16_t *psxVsw;
static uint32_t *psxVul;
static int32_t *psxVsl;

GLfloat gl_z = 0.0f;
bool bNeedInterlaceUpdate = false;
bool bNeedRGB24Update = false;

static uint32_t ulStatusControl[256];

////////////////////////////////////////////////////////////////////////
// global GPU vars
////////////////////////////////////////////////////////////////////////

static int GPUdataRet;
int lGPUstatusRet;

uint32_t dwGPUVersion = 0;
int iGPUHeight = 512;
int iGPUHeightMask = 511;
int GlobalTextIL = 0;
int iTileCheat = 0;

static uint32_t gpuDataM[256];
static uint8_t gpuCommand = 0;
static int gpuDataC = 0;
static int gpuDataP = 0;

VRAMLoad_t VRAMWrite;
VRAMLoad_t VRAMRead;
int iDataWriteMode;
int iDataReadMode;

int lClearOnSwap;
int lClearOnSwapColor;

// possible psx display widths
static int16_t dispWidths[8] = {256, 320, 512, 640, 368, 384, 512, 640};

PSXDisplay_t PSXDisplay;
PSXDisplay_t PreviousPSXDisplay;
TWin_t TWin;
int16_t imageX0, imageX1;
int16_t imageY0, imageY1;
bool bDisplayNotSet = true;
static int lSelectedSlot = 0;
static uint8_t *pGfxCardScreen = 0;
static int iRenderFVR = 0;
uint32_t ulGPUInfoVals[16];
static int iFakePrimBusy = 0;
static uint32_t vBlank = 0;
static bool oddLines;

////////////////////////////////////////////////////////////////////////
// GPU INIT... here starts it all (first func called by emu)
////////////////////////////////////////////////////////////////////////

int32_t LoadPSE()
{
	memset(ulStatusControl, 0, 256 * sizeof(uint32_t));

	// different ways of accessing PSX VRAM

	psxVSecure = (uint8_t *)malloc((iGPUHeight * 2) * 1024 +
	                               (1024 * 1024)); // always alloc one extra MB for soft drawing funcs security
	if (!psxVSecure)
		return -1;

	psxVub = psxVSecure + 512 * 1024; // security offset into double sized psx vram!
	psxVsb = (int8_t *)psxVub;
	psxVsw = (int16_t *)psxVub;
	psxVsl = (int32_t *)psxVub;
	psxVuw = (uint16_t *)psxVub;
	psxVul = (uint32_t *)psxVub;

	psxVuw_eom = psxVuw + 1024 * iGPUHeight; // pre-calc of end of vram

	memset(psxVSecure, 0x00, (iGPUHeight * 2) * 1024 + (1024 * 1024));
	memset(ulGPUInfoVals, 0x00, 16 * sizeof(uint32_t));

	PSXDisplay.RGB24 = 0; // init vars
	PreviousPSXDisplay.RGB24 = 0;
	PSXDisplay.Interlaced = 0;
	PSXDisplay.InterlacedTest = 0;
	PSXDisplay.DrawOffset.x = 0;
	PSXDisplay.DrawOffset.y = 0;
	PSXDisplay.DrawArea.x0 = 0;
	PSXDisplay.DrawArea.y0 = 0;
	PSXDisplay.DrawArea.x1 = 320;
	PSXDisplay.DrawArea.y1 = 240;
	PSXDisplay.DisplayMode.x = 320;
	PSXDisplay.DisplayMode.y = 240;
	PSXDisplay.Disabled = false;
	PreviousPSXDisplay.Range.x0 = 0;
	PreviousPSXDisplay.Range.x1 = 0;
	PreviousPSXDisplay.Range.y0 = 0;
	PreviousPSXDisplay.Range.y1 = 0;
	PSXDisplay.Range.x0 = 0;
	PSXDisplay.Range.x1 = 0;
	PSXDisplay.Range.y0 = 0;
	PSXDisplay.Range.y1 = 0;
	PreviousPSXDisplay.DisplayPosition.x = 1;
	PreviousPSXDisplay.DisplayPosition.y = 1;
	PSXDisplay.DisplayPosition.x = 1;
	PSXDisplay.DisplayPosition.y = 1;
	PreviousPSXDisplay.DisplayModeNew.y = 0;
	PSXDisplay.Double = 1;
	GPUdataRet = 0x400;

	PSXDisplay.DisplayModeNew.x = 0;
	PSXDisplay.DisplayModeNew.y = 0;

	// PreviousPSXDisplay.Height = PSXDisplay.Height = 239;

	iDataWriteMode = DR_NORMAL;

	// Reset transfer values, to prevent mis-transfer of data
	memset(&VRAMWrite, 0, sizeof(VRAMLoad_t));
	memset(&VRAMRead, 0, sizeof(VRAMLoad_t));

	// device initialised already !
	// lGPUstatusRet = 0x74000000;
	vBlank = 0;
	oddLines = false;

	STATUSREG = 0x14802000;
	GPUIsIdle;
	GPUIsReadyForCommands;

	return 0;
}


static GLFWwindow* glfwwindow;

static void osd_close_display(void) // close display
{
	glfwTerminate();
}

static void refreshContext()
{
    GLrefresh();
}

static void window_close_callback(GLFWwindow* window)
{
    glfwDestroyWindow(window);
}

static void framebuffer_size_callback(GLFWwindow*, int width, int height)
{
    rRatioRect.right = width;
	rRatioRect.bottom = height;
	iResX=width;
	iResY=height;
	refreshContext();
}

static void sysdep_create_display(void) // create display
{

	glfwInit();
	//glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	glfwwindow = glfwCreateWindow(iResX, iResY, "troll", NULL, NULL);
	glfwSetWindowCloseCallback(glfwwindow, window_close_callback); 
	glfwSetFramebufferSizeCallback(glfwwindow, framebuffer_size_callback);
	glfwMakeContextCurrent(glfwwindow);

}

////////////////////////////////////////////////////////////////////////

int32_t OpenPSE()
{
	sysdep_create_display();
	InitializeTextureStore();
	rRatioRect.left = rRatioRect.top = 0;
	rRatioRect.right = iResX;
	rRatioRect.bottom = iResY;
	GLinitialize();
	if (glfwwindow)
		return 0;
	return -1;
}

int32_t ClosePSE() 
{
	GLcleanup(); 

	if (pGfxCardScreen)
		free(pGfxCardScreen); 
	pGfxCardScreen = 0;
	osd_close_display();
	return 0;
}

int32_t ShutdownPSE()
{
	if (psxVSecure)
		free(psxVSecure);
	psxVSecure = 0;
	return 0;
}

int iLastRGB24 = 0; 
static int iSkipTwo = 0;

void updateDisplay(void) // UPDATE DISPLAY
{

	bFakeFrontBuffer = false;
	bRenderFrontBuffer = false;

	if (iRenderFVR) // frame buffer read fix mode still active?
	{
		iRenderFVR--; // -> if some frames in a row without read access: turn off mode
		if (!iRenderFVR)
			bFullVRam = false;
	}

	if (iLastRGB24 && iLastRGB24 != PSXDisplay.RGB24 + 1) // (mdec) garbage check
	{
		iSkipTwo = 2; // -> skip two frames to avoid garbage if color mode changes
	}
	iLastRGB24 = 0;

	if (PSXDisplay.RGB24) // && !bNeedUploadAfter)          // (mdec) upload wanted?
	{
		PrepareFullScreenUpload(-1);
		UploadScreen(PSXDisplay.Interlaced); // -> upload whole screen from psx vram
		bNeedUploadTest = false;
		bNeedInterlaceUpdate = false;
		bNeedUploadAfter = false;
		bNeedRGB24Update = false;
	}
	else if (bNeedInterlaceUpdate) // smaller upload?
	{
		bNeedInterlaceUpdate = false;
		xrUploadArea = xrUploadAreaIL; // -> upload this rect
		UploadScreen(true);
	}

	if (PSXDisplay.Disabled) // display disabled?
	{
		// moved here
		clearToBlack();
		gl_z = 0.0f;
		bDisplayNotSet = true;
	}

	if (iSkipTwo) // we are in skipping mood?
	{
		iSkipTwo--;
		iDrawnSomething = 0; // -> simply lie about something drawn
	}

	//----------------------------------------------------//
	// main buffer swapping (well, or skip it)

	if (iDrawnSomething)
		glfwSwapBuffers(glfwwindow);

	iDrawnSomething = 0;

	//----------------------------------------------------//

	if (lClearOnSwap) // clear buffer after swap?
	{
		if (bDisplayNotSet) // -> set new vals
			SetOGLDisplaySettings(1);
		clearWithColor(lClearOnSwapColor);
		lClearOnSwap = 0; // -> done
	}

	gl_z = 0.0f;

	//----------------------------------------------------//
	// additional uploads immediatly after swapping

	if (bNeedUploadAfter) // upload wanted?
	{
		bNeedUploadAfter = false;
		bNeedUploadTest = false;
		UploadScreen(-1); // -> upload
	}

	if (bNeedUploadTest)
	{
		bNeedUploadTest = false;
		if (PSXDisplay.InterlacedTest &&
		    PreviousPSXDisplay.DisplayPosition.x == PSXDisplay.DisplayPosition.x &&
		    PreviousPSXDisplay.DisplayEnd.x == PSXDisplay.DisplayEnd.x &&
		    PreviousPSXDisplay.DisplayPosition.y == PSXDisplay.DisplayPosition.y &&
		    PreviousPSXDisplay.DisplayEnd.y == PSXDisplay.DisplayEnd.y)
		{
			PrepareFullScreenUpload(true);
			UploadScreen(true);
		}
	}

}

void updateFrontDisplay(void)
{
	bFakeFrontBuffer = false;
	bRenderFrontBuffer = false;

	if (iDrawnSomething) 
		glfwSwapBuffers(glfwwindow);
}

static void ChangeDispOffsetsX(void) // CENTER X
{
	int lx, l;
	int16_t sO;

	if (!PSXDisplay.Range.x1)
		return; // some range given?

	l = PSXDisplay.DisplayMode.x;

	l *= (int)PSXDisplay.Range.x1; // some funky calculation
	l /= 2560;
	lx = l;
	l &= 0xfffffff8;

	if (l == PreviousPSXDisplay.Range.x1)
		return; // some change?

	sO = PreviousPSXDisplay.Range.x0; // store old

	if (lx >= PSXDisplay.DisplayMode.x) // range bigger?
	{
		PreviousPSXDisplay.Range.x1 = // -> take display width
		    PSXDisplay.DisplayMode.x;
		PreviousPSXDisplay.Range.x0 = 0; // -> start pos is 0
	}
	else // range smaller? center it
	{
		PreviousPSXDisplay.Range.x1 = l; // -> store width (8 pixel aligned)
		PreviousPSXDisplay.Range.x0 =    // -> calc start pos
		    (PSXDisplay.Range.x0 - 500) / 8;
		if (PreviousPSXDisplay.Range.x0 < 0) // -> we don't support neg. values yet
			PreviousPSXDisplay.Range.x0 = 0;

		if ((PreviousPSXDisplay.Range.x0 + lx) > // -> uhuu... that's too much
		    PSXDisplay.DisplayMode.x)
		{
			PreviousPSXDisplay.Range.x0 = // -> adjust start
			    PSXDisplay.DisplayMode.x - lx;
			PreviousPSXDisplay.Range.x1 += lx - l; // -> adjust width
		}
	}

	if (sO != PreviousPSXDisplay.Range.x0) // something changed?
	{
		bDisplayNotSet = true; // -> recalc display stuff
	}
}

static void ChangeDispOffsetsY(void) // CENTER Y
{
	int iT;
	int16_t sO; // store previous y size

	if (PSXDisplay.PAL)
		iT = 48;
	else
		iT = 28; // different offsets on PAL/NTSC

	if (PSXDisplay.Range.y0 >= iT) // crossed the security line? :)
	{
		PreviousPSXDisplay.Range.y1 = // -> store width
		    PSXDisplay.DisplayModeNew.y;

		sO = (PSXDisplay.Range.y0 - iT - 4) * PSXDisplay.Double; // -> calc offset
		if (sO < 0)
			sO = 0;

		PSXDisplay.DisplayModeNew.y += sO; // -> add offset to y size, too
	}
	else
		sO = 0; // else no offset

	if (sO != PreviousPSXDisplay.Range.y0) // something changed?
	{
		PreviousPSXDisplay.Range.y0 = sO;
		bDisplayNotSet = true; // -> recalc display stuff
	}
}


static void updateDisplayIfChanged(void)
{
	bool bUp;

	if ((PSXDisplay.DisplayMode.y == PSXDisplay.DisplayModeNew.y) &&
	    (PSXDisplay.DisplayMode.x == PSXDisplay.DisplayModeNew.x))
	{
		if ((PSXDisplay.RGB24 == PSXDisplay.RGB24New) && (PSXDisplay.Interlaced == PSXDisplay.InterlacedNew))
			return; // nothing has changed? fine, no swap buffer needed
	}
	else // some res change?
	{
		glLoadIdentity();
		glOrtho(0, PSXDisplay.DisplayModeNew.x, // -> new psx resolution
		        PSXDisplay.DisplayModeNew.y, 0, -1, 1);
	}

	bDisplayNotSet = true; // re-calc offsets/display area

	bUp = false;
	if (PSXDisplay.RGB24 != PSXDisplay.RGB24New) // clean up textures, if rgb mode change (usually mdec on/off)
	{
		PreviousPSXDisplay.RGB24 = 0; // no full 24 frame uploaded yet
		ResetTextureArea(false);
		bUp = true;
	}

	PSXDisplay.RGB24 = PSXDisplay.RGB24New; // get new infos
	PSXDisplay.DisplayMode.y = PSXDisplay.DisplayModeNew.y;
	PSXDisplay.DisplayMode.x = PSXDisplay.DisplayModeNew.x;
	PSXDisplay.Interlaced = PSXDisplay.InterlacedNew;

	PSXDisplay.DisplayEnd.x = // calc new ends
	    PSXDisplay.DisplayPosition.x + PSXDisplay.DisplayMode.x;
	PSXDisplay.DisplayEnd.y =
	    PSXDisplay.DisplayPosition.y + PSXDisplay.DisplayMode.y + PreviousPSXDisplay.DisplayModeNew.y;
	PreviousPSXDisplay.DisplayEnd.x = PreviousPSXDisplay.DisplayPosition.x + PSXDisplay.DisplayMode.x;
	PreviousPSXDisplay.DisplayEnd.y =
	    PreviousPSXDisplay.DisplayPosition.y + PSXDisplay.DisplayMode.y + PreviousPSXDisplay.DisplayModeNew.y;

	ChangeDispOffsetsX();

	if (bUp)
		updateDisplay(); // yeah, real update (swap buffer)
}

static uint16_t usFirstPos = 2;

void UpdateLacePSE()
{
	STATUSREG ^= GPUSTATUS_ODDLINES; // interlaced bit toggle, if the CC game fix is not active (see gpuReadStatus)

	if (PSXDisplay.Interlaced) // interlaced mode?
	{
		if (PSXDisplay.DisplayMode.x > 0 && PSXDisplay.DisplayMode.y > 0)
		{
			updateDisplay(); // -> swap buffers (new frame)
		}
	}
	else if (bRenderFrontBuffer) // no interlace mode? and some stuff in front has changed?
	{
		updateFrontDisplay(); // -> update front buffer
	}
	else if (usFirstPos == 1) // initial updates (after startup)
	{
		updateDisplay();
	}
}

////////////////////////////////////////////////////////////////////////
// process read request from GPU status register
////////////////////////////////////////////////////////////////////////

uint32_t ReadStatusPSE()
{
	if (vBlank || oddLines == false)
	{ // vblank or even lines
		STATUSREG &= ~(GPUSTATUS_ODDLINES);
	}
	else
	{ // Oddlines and not vblank
		STATUSREG |= GPUSTATUS_ODDLINES;
	}

	if (iFakePrimBusy) // 27.10.2007 - emulating some 'busy' while drawing... pfff... not perfect, but since our
	                   // emulated dma is not done in an extra thread...
	{
		iFakePrimBusy--;

		if (iFakePrimBusy & 1) // we do a busy-idle-busy-idle sequence after/while drawing prims
		{
			GPUIsBusy;
			GPUIsNotReadyForCommands;
		}
		else
		{
			GPUIsIdle;
			GPUIsReadyForCommands;
		}
	}

	return STATUSREG;
}

////////////////////////////////////////////////////////////////////////
// processes data send to GPU status register
// these are always single packet commands.
////////////////////////////////////////////////////////////////////////

void WriteStatusPSE(uint32_t gdata)
{
	uint32_t lCommand = (gdata >> 24) & 0xff;

	ulStatusControl[lCommand] = gdata;

	switch (lCommand)
	{
	//--------------------------------------------------//
	// reset gpu
	case 0x00:
		memset(ulGPUInfoVals, 0x00, 16 * sizeof(uint32_t));
		lGPUstatusRet = 0x14802000;
		PSXDisplay.Disabled = 1;
		iDataWriteMode = iDataReadMode = DR_NORMAL;
		PSXDisplay.DrawOffset.x = PSXDisplay.DrawOffset.y = 0;
		drawX = drawY = 0;
		drawW = drawH = 0;
		usMirror = 0;
		GlobalTextAddrX = 0;
		GlobalTextAddrY = 0;
		GlobalTextTP = 0;
		GlobalTextABR = 0;
		PSXDisplay.RGB24 = false;
		PSXDisplay.Interlaced = false;
		bUsingTWin = false;
		return;

	// dis/enable display
	case 0x03:
		PreviousPSXDisplay.Disabled = PSXDisplay.Disabled;
		PSXDisplay.Disabled = (gdata & 1);

		if (PSXDisplay.Disabled)
			STATUSREG |= GPUSTATUS_DISPLAYDISABLED;
		else
			STATUSREG &= ~GPUSTATUS_DISPLAYDISABLED;

		return;

	// setting transfer mode
	case 0x04:
		gdata &= 0x03; // only want the lower two bits

		iDataWriteMode = iDataReadMode = DR_NORMAL;
		if (gdata == 0x02)
			iDataWriteMode = DR_VRAMTRANSFER;
		if (gdata == 0x03)
			iDataReadMode = DR_VRAMTRANSFER;

		STATUSREG &= ~GPUSTATUS_DMABITS; // clear the current settings of the DMA bits
		STATUSREG |= (gdata << 29);      // set the DMA bits according to the received data

		return;

	// setting display position
	case 0x05:
	{
		int16_t sx = (int16_t)(gdata & 0x3ff);
		int16_t sy;

		if (iGPUHeight == 1024)
		{
			if (dwGPUVersion == 2)
				sy = (int16_t)((gdata >> 12) & 0x3ff);
			else
				sy = (int16_t)((gdata >> 10) & 0x3ff);
		}
		else
			sy = (int16_t)((gdata >> 10) & 0x3ff); // really: 0x1ff, but we adjust it later

		if (sy & 0x200)
		{
			sy |= 0xfc00;
			PreviousPSXDisplay.DisplayModeNew.y = sy / PSXDisplay.Double;
			sy = 0;
		}
		else
			PreviousPSXDisplay.DisplayModeNew.y = 0;

		if (sx > 1000)
			sx = 0;

		if (usFirstPos)
		{
			usFirstPos--;
			if (usFirstPos)
			{
				PreviousPSXDisplay.DisplayPosition.x = sx;
				PreviousPSXDisplay.DisplayPosition.y = sy;
				PSXDisplay.DisplayPosition.x = sx;
				PSXDisplay.DisplayPosition.y = sy;
			}
		}

		{
			if ((!PSXDisplay.Interlaced) && PSXDisplay.DisplayPosition.x == sx && PSXDisplay.DisplayPosition.y == sy)
				return;
			PreviousPSXDisplay.DisplayPosition.x = PSXDisplay.DisplayPosition.x;
			PreviousPSXDisplay.DisplayPosition.y = PSXDisplay.DisplayPosition.y;
			PSXDisplay.DisplayPosition.x = sx;
			PSXDisplay.DisplayPosition.y = sy;
		}

		PSXDisplay.DisplayEnd.x = PSXDisplay.DisplayPosition.x + PSXDisplay.DisplayMode.x;
		PSXDisplay.DisplayEnd.y =
		    PSXDisplay.DisplayPosition.y + PSXDisplay.DisplayMode.y + PreviousPSXDisplay.DisplayModeNew.y;

		PreviousPSXDisplay.DisplayEnd.x = PreviousPSXDisplay.DisplayPosition.x + PSXDisplay.DisplayMode.x;
		PreviousPSXDisplay.DisplayEnd.y =
		    PreviousPSXDisplay.DisplayPosition.y + PSXDisplay.DisplayMode.y + PreviousPSXDisplay.DisplayModeNew.y;

		bDisplayNotSet = true;

		if (!(PSXDisplay.Interlaced))
		{
			updateDisplay();
		}
		else if (PSXDisplay.InterlacedTest && ((PreviousPSXDisplay.DisplayPosition.x != PSXDisplay.DisplayPosition.x) ||
		                                       (PreviousPSXDisplay.DisplayPosition.y != PSXDisplay.DisplayPosition.y)))
			PSXDisplay.InterlacedTest--;

		return;
	}

	// setting width
	case 0x06:

		PSXDisplay.Range.x0 = gdata & 0x7ff; // 0x3ff;
		PSXDisplay.Range.x1 = (gdata >> 12) & 0xfff; // 0x7ff;

		PSXDisplay.Range.x1 -= PSXDisplay.Range.x0;

		ChangeDispOffsetsX();

		return;

	// setting height
	case 0x07:

		PreviousPSXDisplay.Height = PSXDisplay.Height;

		PSXDisplay.Range.y0 = gdata & 0x3ff;
		PSXDisplay.Range.y1 = (gdata >> 10) & 0x3ff;

		PSXDisplay.Height = PSXDisplay.Range.y1 - PSXDisplay.Range.y0 + PreviousPSXDisplay.DisplayModeNew.y;

		if (PreviousPSXDisplay.Height != PSXDisplay.Height)
		{
			PSXDisplay.DisplayModeNew.y = PSXDisplay.Height * PSXDisplay.Double;
			ChangeDispOffsetsY();
			updateDisplayIfChanged();
		}
		return;

	// setting display infos
	case 0x08:

		PSXDisplay.DisplayModeNew.x = dispWidths[(gdata & 0x03) | ((gdata & 0x40) >> 4)];

		if (gdata & 0x04)
			PSXDisplay.Double = 2;
		else
			PSXDisplay.Double = 1;
		PSXDisplay.DisplayModeNew.y = PSXDisplay.Height * PSXDisplay.Double;

		ChangeDispOffsetsY();

		PSXDisplay.PAL = (gdata & 0x08) ? true : false;           // if 1 - PAL mode, else NTSC
		PSXDisplay.RGB24New = (gdata & 0x10) ? true : false;      // if 1 - TrueColor
		PSXDisplay.InterlacedNew = (gdata & 0x20) ? true : false; // if 1 - Interlace

		STATUSREG &= ~GPUSTATUS_WIDTHBITS; // clear the width bits

		STATUSREG |= (((gdata & 0x03) << 17) | ((gdata & 0x40) << 10)); // set the width bits

		PreviousPSXDisplay.InterlacedNew = false;
		if (PSXDisplay.InterlacedNew)
		{
			if (!PSXDisplay.Interlaced)
			{
				PSXDisplay.InterlacedTest = 2;
				PreviousPSXDisplay.DisplayPosition.x = PSXDisplay.DisplayPosition.x;
				PreviousPSXDisplay.DisplayPosition.y = PSXDisplay.DisplayPosition.y;
				PreviousPSXDisplay.InterlacedNew = true;
			}

			STATUSREG |= GPUSTATUS_INTERLACED;
		}
		else
		{
			PSXDisplay.InterlacedTest = 0;
			STATUSREG &= ~GPUSTATUS_INTERLACED;
		}

		if (PSXDisplay.PAL)
			STATUSREG |= GPUSTATUS_PAL;
		else
			STATUSREG &= ~GPUSTATUS_PAL;

		if (PSXDisplay.Double == 2)
			STATUSREG |= GPUSTATUS_DOUBLEHEIGHT;
		else
			STATUSREG &= ~GPUSTATUS_DOUBLEHEIGHT;

		if (PSXDisplay.RGB24New)
			STATUSREG |= GPUSTATUS_RGB24;
		else
			STATUSREG &= ~GPUSTATUS_RGB24;

		updateDisplayIfChanged();

		return;

	//--------------------------------------------------//
	// ask about GPU version and other stuff
	case 0x10:

		gdata &= 0xff;

		switch (gdata)
		{
		case 0x02:
			GPUdataRet = ulGPUInfoVals[INFO_TW]; // tw infos
			return;
		case 0x03:
			GPUdataRet = ulGPUInfoVals[INFO_DRAWSTART]; // draw start
			return;
		case 0x04:
			GPUdataRet = ulGPUInfoVals[INFO_DRAWEND]; // draw end
			return;
		case 0x05:
		case 0x06:
			GPUdataRet = ulGPUInfoVals[INFO_DRAWOFF]; // draw offset
			return;
		case 0x07:
			if (dwGPUVersion == 2)
				GPUdataRet = 0x01;
			else
				GPUdataRet = 0x02; // gpu type
			return;
		case 0x08:
		case 0x0F: // some bios addr?
			GPUdataRet = 0xBFC03720;
			return;
		}
		return;
		//--------------------------------------------------//
	}
}

////////////////////////////////////////////////////////////////////////
// vram read/write helpers
////////////////////////////////////////////////////////////////////////

bool bNeedWriteUpload = false;

static __inline void FinishedVRAMWrite(void)
{
	if (bNeedWriteUpload)
	{
		bNeedWriteUpload = false;
		CheckWriteUpdate();
	}

	// set register to NORMAL operation
	iDataWriteMode = DR_NORMAL;

	// reset transfer values, to prevent mis-transfer of data
	VRAMWrite.ColsRemaining = 0;
	VRAMWrite.RowsRemaining = 0;
}

static __inline void FinishedVRAMRead(void)
{
	// set register to NORMAL operation
	iDataReadMode = DR_NORMAL;
	// reset transfer values, to prevent mis-transfer of data
	VRAMRead.x = 0;
	VRAMRead.y = 0;
	VRAMRead.Width = 0;
	VRAMRead.Height = 0;
	VRAMRead.ColsRemaining = 0;
	VRAMRead.RowsRemaining = 0;

	// indicate GPU is no longer ready for VRAM data in the STATUS REGISTER
	STATUSREG &= ~GPUSTATUS_READYFORVRAM;
}

////////////////////////////////////////////////////////////////////////
// vram read check ex (reading from card's back/frontbuffer if needed...
// slow!)
////////////////////////////////////////////////////////////////////////

void CheckVRamReadEx(int x, int y, int dx, int dy)
{
	uint16_t sArea;
	int ux, uy, udx, udy, wx, wy;
	uint16_t *p1, *p2;
	float XS, YS;
	uint8_t *ps;
	uint8_t *px;
	uint16_t s, sx;

	if (STATUSREG & GPUSTATUS_RGB24)
		return;

	if (((dx > PSXDisplay.DisplayPosition.x) && (x < PSXDisplay.DisplayEnd.x) && (dy > PSXDisplay.DisplayPosition.y) &&
	     (y < PSXDisplay.DisplayEnd.y)))
		sArea = 0;
	else if ((!(PSXDisplay.InterlacedTest) && (dx > PreviousPSXDisplay.DisplayPosition.x) &&
	          (x < PreviousPSXDisplay.DisplayEnd.x) && (dy > PreviousPSXDisplay.DisplayPosition.y) &&
	          (y < PreviousPSXDisplay.DisplayEnd.y)))
		sArea = 1;
	else
	{
		return;
	}

	//////////////

	if (iRenderFVR)
	{
		bFullVRam = true;
		iRenderFVR = 2;
		return;
	}
	bFullVRam = true;
	iRenderFVR = 2;

	//////////////

	p2 = 0;

	if (sArea == 0)
	{
		ux = PSXDisplay.DisplayPosition.x;
		uy = PSXDisplay.DisplayPosition.y;
		udx = PSXDisplay.DisplayEnd.x - ux;
		udy = PSXDisplay.DisplayEnd.y - uy;
		if ((PreviousPSXDisplay.DisplayEnd.x - PreviousPSXDisplay.DisplayPosition.x) == udx &&
		    (PreviousPSXDisplay.DisplayEnd.y - PreviousPSXDisplay.DisplayPosition.y) == udy)
			p2 = (psxVuw + (1024 * PreviousPSXDisplay.DisplayPosition.y) + PreviousPSXDisplay.DisplayPosition.x);
	}
	else
	{
		ux = PreviousPSXDisplay.DisplayPosition.x;
		uy = PreviousPSXDisplay.DisplayPosition.y;
		udx = PreviousPSXDisplay.DisplayEnd.x - ux;
		udy = PreviousPSXDisplay.DisplayEnd.y - uy;
		if ((PSXDisplay.DisplayEnd.x - PSXDisplay.DisplayPosition.x) == udx &&
		    (PSXDisplay.DisplayEnd.y - PSXDisplay.DisplayPosition.y) == udy)
			p2 = (psxVuw + (1024 * PSXDisplay.DisplayPosition.y) + PSXDisplay.DisplayPosition.x);
	}

	p1 = (psxVuw + (1024 * uy) + ux);
	if (p1 == p2)
		p2 = 0;

	x = 0;
	y = 0;
	wx = dx = udx;
	wy = dy = udy;

	if (udx <= 0)
		return;
	if (udy <= 0)
		return;
	if (dx <= 0)
		return;
	if (dy <= 0)
		return;
	if (wx <= 0)
		return;
	if (wy <= 0)
		return;

	XS = (float)rRatioRect.right / (float)wx;
	YS = (float)rRatioRect.bottom / (float)wy;

	dx = (int)((float)(dx)*XS);
	dy = (int)((float)(dy)*YS);

	if (dx > iResX)
		dx = iResX;
	if (dy > iResY)
		dy = iResY;

	if (dx <= 0)
		return;
	if (dy <= 0)
		return;

	// ogl y adjust
	y = iResY - y - dy;

	x += rRatioRect.left;
	y -= rRatioRect.top;

	if (y < 0)
		y = 0;
	if ((y + dy) > iResY)
		dy = iResY - y;

	if (!pGfxCardScreen)
	{
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		pGfxCardScreen = (uint8_t *)malloc(iResX * iResY * 4);
	}

	ps = pGfxCardScreen;

	if (!sArea)
		glReadBuffer(GL_FRONT);

	glReadPixels(x, y, dx, dy, GL_RGB, GL_UNSIGNED_BYTE, ps);

	if (!sArea)
		glReadBuffer(GL_BACK);

	s = 0;

	XS = (float)dx / (float)(udx);
	YS = (float)dy / (float)(udy + 1);

	for (y = udy; y > 0; y--)
	{
		for (x = 0; x < udx; x++)
		{
			if (p1 >= psxVuw && p1 < psxVuw_eom)
			{
				px = ps + (3 * ((int)((float)x * XS)) + (3 * dx) * ((int)((float)y * YS)));
				sx = (*px) >> 3;
				px++;
				s = sx;
				sx = (*px) >> 3;
				px++;
				s |= sx << 5;
				sx = (*px) >> 3;
				s |= sx << 10;
				s &= ~0x8000;
				*p1 = s;
			}
			if (p2 >= psxVuw && p2 < psxVuw_eom)
				*p2 = s;

			p1++;
			if (p2)
				p2++;
		}

		p1 += 1024 - udx;
		if (p2)
			p2 += 1024 - udx;
	}
}

////////////////////////////////////////////////////////////////////////
// vram read check (reading from card's back/frontbuffer if needed...
// slow!)
////////////////////////////////////////////////////////////////////////

void CheckVRamRead(int x, int y, int dx, int dy, bool bFront)
{
	uint16_t sArea;
	uint16_t *p;
	int ux, uy, udx, udy, wx, wy;
	float XS, YS;
	uint8_t *ps, *px;
	uint16_t s = 0, sx;

	if (STATUSREG & GPUSTATUS_RGB24)
		return;

	if (((dx > PSXDisplay.DisplayPosition.x) && (x < PSXDisplay.DisplayEnd.x) && (dy > PSXDisplay.DisplayPosition.y) &&
	     (y < PSXDisplay.DisplayEnd.y)))
		sArea = 0;
	else if ((!(PSXDisplay.InterlacedTest) && (dx > PreviousPSXDisplay.DisplayPosition.x) &&
	          (x < PreviousPSXDisplay.DisplayEnd.x) && (dy > PreviousPSXDisplay.DisplayPosition.y) &&
	          (y < PreviousPSXDisplay.DisplayEnd.y)))
		sArea = 1;
	else
	{
		return;
	}

	ux = x;
	uy = y;
	udx = dx;
	udy = dy;

	if (sArea == 0)
	{
		x -= PSXDisplay.DisplayPosition.x;
		dx -= PSXDisplay.DisplayPosition.x;
		y -= PSXDisplay.DisplayPosition.y;
		dy -= PSXDisplay.DisplayPosition.y;
		wx = PSXDisplay.DisplayEnd.x - PSXDisplay.DisplayPosition.x;
		wy = PSXDisplay.DisplayEnd.y - PSXDisplay.DisplayPosition.y;
	}
	else
	{
		x -= PreviousPSXDisplay.DisplayPosition.x;
		dx -= PreviousPSXDisplay.DisplayPosition.x;
		y -= PreviousPSXDisplay.DisplayPosition.y;
		dy -= PreviousPSXDisplay.DisplayPosition.y;
		wx = PreviousPSXDisplay.DisplayEnd.x - PreviousPSXDisplay.DisplayPosition.x;
		wy = PreviousPSXDisplay.DisplayEnd.y - PreviousPSXDisplay.DisplayPosition.y;
	}
	if (x < 0)
	{
		ux -= x;
		x = 0;
	}
	if (y < 0)
	{
		uy -= y;
		y = 0;
	}
	if (dx > wx)
	{
		udx -= (dx - wx);
		dx = wx;
	}
	if (dy > wy)
	{
		udy -= (dy - wy);
		dy = wy;
	}
	udx -= ux;
	udy -= uy;

	p = (psxVuw + (1024 * uy) + ux);

	if (udx <= 0)
		return;
	if (udy <= 0)
		return;
	if (dx <= 0)
		return;
	if (dy <= 0)
		return;
	if (wx <= 0)
		return;
	if (wy <= 0)
		return;

	XS = (float)rRatioRect.right / (float)wx;
	YS = (float)rRatioRect.bottom / (float)wy;

	dx = (int)((float)(dx)*XS);
	dy = (int)((float)(dy)*YS);
	x = (int)((float)x * XS);
	y = (int)((float)y * YS);

	dx -= x;
	dy -= y;

	if (dx > iResX)
		dx = iResX;
	if (dy > iResY)
		dy = iResY;

	if (dx <= 0)
		return;
	if (dy <= 0)
		return;

	// ogl y adjust
	y = iResY - y - dy;

	x += rRatioRect.left;
	y -= rRatioRect.top;

	if (y < 0)
		y = 0;
	if ((y + dy) > iResY)
		dy = iResY - y;

	if (!pGfxCardScreen)
	{
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		pGfxCardScreen = (uint8_t *)malloc(iResX * iResY * 4);
	}

	ps = pGfxCardScreen;

	if (bFront)
		glReadBuffer(GL_FRONT);

	glReadPixels(x, y, dx, dy, GL_RGB, GL_UNSIGNED_BYTE, ps);

	if (bFront)
		glReadBuffer(GL_BACK);

	XS = (float)dx / (float)(udx);
	YS = (float)dy / (float)(udy + 1);

	for (y = udy; y > 0; y--)
	{
		for (x = 0; x < udx; x++)
		{
			if (p >= psxVuw && p < psxVuw_eom)
			{
				px = ps + (3 * ((int)((float)x * XS)) + (3 * dx) * ((int)((float)y * YS)));
				sx = (*px) >> 3;
				px++;
				s = sx;
				sx = (*px) >> 3;
				px++;
				s |= sx << 5;
				sx = (*px) >> 3;
				s |= sx << 10;
				s &= ~0x8000;
				*p = s;
			}
			p++;
		}
		p += 1024 - udx;
	}
}

////////////////////////////////////////////////////////////////////////
// core read from vram
////////////////////////////////////////////////////////////////////////

void ReadDataMemPSE(uint32_t *pMem, int iSize)
{
	int i;

	if (iDataReadMode != DR_VRAMTRANSFER)
		return;

	GPUIsBusy;

	// adjust read ptr, if necessary
	while (VRAMRead.ImagePtr >= psxVuw_eom)
		VRAMRead.ImagePtr -= iGPUHeight * 1024;
	while (VRAMRead.ImagePtr < psxVuw)
		VRAMRead.ImagePtr += iGPUHeight * 1024;

	if ((iFrameReadType & 1 && iSize > 1) &&
	    !(iDrawnSomething == 2 && VRAMRead.x == VRAMWrite.x && VRAMRead.y == VRAMWrite.y &&
	      VRAMRead.Width == VRAMWrite.Width && VRAMRead.Height == VRAMWrite.Height))
		CheckVRamRead(VRAMRead.x, VRAMRead.y, VRAMRead.x + VRAMRead.RowsRemaining, VRAMRead.y + VRAMRead.ColsRemaining,
		              true);

	for (i = 0; i < iSize; i++)
	{
		// do 2 seperate 16bit reads for compatibility (wrap issues)
		if ((VRAMRead.ColsRemaining > 0) && (VRAMRead.RowsRemaining > 0))
		{
			// lower 16 bit
			GPUdataRet = (uint32_t)*VRAMRead.ImagePtr;

			VRAMRead.ImagePtr++;
			if (VRAMRead.ImagePtr >= psxVuw_eom)
				VRAMRead.ImagePtr -= iGPUHeight * 1024;
			VRAMRead.RowsRemaining--;

			if (VRAMRead.RowsRemaining <= 0)
			{
				VRAMRead.RowsRemaining = VRAMRead.Width;
				VRAMRead.ColsRemaining--;
				VRAMRead.ImagePtr += 1024 - VRAMRead.Width;
				if (VRAMRead.ImagePtr >= psxVuw_eom)
					VRAMRead.ImagePtr -= iGPUHeight * 1024;
			}

			// higher 16 bit (always, even if it's an odd width)
			GPUdataRet |= (uint32_t)(*VRAMRead.ImagePtr) << 16;
			*pMem++ = GPUdataRet;

			if (VRAMRead.ColsRemaining <= 0)
			{
				FinishedVRAMRead();
				goto ENDREAD;
			}

			VRAMRead.ImagePtr++;
			if (VRAMRead.ImagePtr >= psxVuw_eom)
				VRAMRead.ImagePtr -= iGPUHeight * 1024;
			VRAMRead.RowsRemaining--;
			if (VRAMRead.RowsRemaining <= 0)
			{
				VRAMRead.RowsRemaining = VRAMRead.Width;
				VRAMRead.ColsRemaining--;
				VRAMRead.ImagePtr += 1024 - VRAMRead.Width;
				if (VRAMRead.ImagePtr >= psxVuw_eom)
					VRAMRead.ImagePtr -= iGPUHeight * 1024;
			}
			if (VRAMRead.ColsRemaining <= 0)
			{
				FinishedVRAMRead();
				goto ENDREAD;
			}
		}
		else
		{
			FinishedVRAMRead();
			goto ENDREAD;
		}
	}

ENDREAD:
	GPUIsIdle;
}

uint32_t ReadDataPSE()
{
	uint32_t l;
	ReadDataMemPSE(&l, 1);
	return GPUdataRet;
}

////////////////////////////////////////////////////////////////////////
// helper table to know how much data is used by drawing commands
////////////////////////////////////////////////////////////////////////

static const uint8_t primTableCX[256] = {
    // 00
    0,   0,   3,   0,   0,   0,   0,   0,
    // 08
    0,   0,   0,   0,   0,   0,   0,   0,
    // 10
    0,   0,   0,   0,   0,   0,   0,   0,
    // 18
    0,   0,   0,   0,   0,   0,   0,   0,
    // 20
    4,   4,   4,   4,   7,   7,   7,   7,
    // 28
    5,   5,   5,   5,   9,   9,   9,   9,
    // 30
    6,   6,   6,   6,   9,   9,   9,   9,
    // 38
    8,   8,   8,   8,   12,  12,  12,  12,
    // 40
    3,   3,   3,   3,   0,   0,   0,   0,
    // 48
    //    5,5,5,5,6,6,6,6,      //FLINE
    254, 254, 254, 254, 254, 254, 254, 254,
    // 50
    4,   4,   4,   4,   0,   0,   0,   0,
    // 58
    //    7,7,7,7,9,9,9,9,    //    LINEG3    LINEG4
    255, 255, 255, 255, 255, 255, 255, 255,
    // 60
    3,   3,   3,   3,   4,   4,   4,   4, //    TILE    SPRT
    // 68
    2,   2,   2,   2,   3,   3,   3,   3, //    TILE1
    // 70
    2,   2,   2,   2,   3,   3,   3,   3,
    // 78
    2,   2,   2,   2,   3,   3,   3,   3,
    // 80
    4,   0,   0,   0,   0,   0,   0,   0,
    // 88
    0,   0,   0,   0,   0,   0,   0,   0,
    // 90
    0,   0,   0,   0,   0,   0,   0,   0,
    // 98
    0,   0,   0,   0,   0,   0,   0,   0,
    // a0
    3,   0,   0,   0,   0,   0,   0,   0,
    // a8
    0,   0,   0,   0,   0,   0,   0,   0,
    // b0
    0,   0,   0,   0,   0,   0,   0,   0,
    // b8
    0,   0,   0,   0,   0,   0,   0,   0,
    // c0
    3,   0,   0,   0,   0,   0,   0,   0,
    // c8
    0,   0,   0,   0,   0,   0,   0,   0,
    // d0
    0,   0,   0,   0,   0,   0,   0,   0,
    // d8
    0,   0,   0,   0,   0,   0,   0,   0,
    // e0
    0,   1,   1,   1,   1,   1,   1,   0,
    // e8
    0,   0,   0,   0,   0,   0,   0,   0,
    // f0
    0,   0,   0,   0,   0,   0,   0,   0,
    // f8
    0,   0,   0,   0,   0,   0,   0,   0};

////////////////////////////////////////////////////////////////////////
// processes data send to GPU data register
////////////////////////////////////////////////////////////////////////

void WriteDataMemPSE(uint32_t *pMem, int iSize)
{
	uint8_t command;
	uint32_t gdata = 0;
	int i = 0;
	GPUIsBusy;
	GPUIsNotReadyForCommands;

STARTVRAM:

	if (iDataWriteMode == DR_VRAMTRANSFER)
	{
		// make sure we are in vram
		while (VRAMWrite.ImagePtr >= psxVuw_eom)
			VRAMWrite.ImagePtr -= iGPUHeight * 1024;
		while (VRAMWrite.ImagePtr < psxVuw)
			VRAMWrite.ImagePtr += iGPUHeight * 1024;

		// now do the loop
		while (VRAMWrite.ColsRemaining > 0)
		{
			while (VRAMWrite.RowsRemaining > 0)
			{
				if (i >= iSize)
				{
					goto ENDVRAM;
				}
				i++;

				gdata = *pMem++;

				// Write odd pixel - Wrap from beginning to next index if going past GPU width
				if (VRAMWrite.Width + VRAMWrite.x - VRAMWrite.RowsRemaining >= 1024)
				{
					*((VRAMWrite.ImagePtr++) - 1024) = (uint16_t)gdata;
				}
				else
				{
					*VRAMWrite.ImagePtr++ = (uint16_t)gdata;
				}
				if (VRAMWrite.ImagePtr >= psxVuw_eom)
					VRAMWrite.ImagePtr -= iGPUHeight * 1024; // Check if went past framebuffer
				VRAMWrite.RowsRemaining--;

				// Check if end at odd pixel drawn
				if (VRAMWrite.RowsRemaining <= 0)
				{
					VRAMWrite.ColsRemaining--;
					if (VRAMWrite.ColsRemaining <= 0) // last pixel is odd width
					{
						gdata = (gdata & 0xFFFF) | (((uint32_t)(*VRAMWrite.ImagePtr)) << 16);
						FinishedVRAMWrite();
						goto ENDVRAM;
					}
					VRAMWrite.RowsRemaining = VRAMWrite.Width;
					VRAMWrite.ImagePtr += 1024 - VRAMWrite.Width;
				}

				// Write even pixel - Wrap from beginning to next index if going past GPU width
				if (VRAMWrite.Width + VRAMWrite.x - VRAMWrite.RowsRemaining >= 1024)
				{
					*((VRAMWrite.ImagePtr++) - 1024) = (uint16_t)(gdata >> 16);
				}
				else
					*VRAMWrite.ImagePtr++ = (uint16_t)(gdata >> 16);
				if (VRAMWrite.ImagePtr >= psxVuw_eom)
					VRAMWrite.ImagePtr -= iGPUHeight * 1024; // Check if went past framebuffer
				VRAMWrite.RowsRemaining--;
			}

			VRAMWrite.RowsRemaining = VRAMWrite.Width;
			VRAMWrite.ColsRemaining--;
			VRAMWrite.ImagePtr += 1024 - VRAMWrite.Width;
		}

		FinishedVRAMWrite();
	}

ENDVRAM:

	if (iDataWriteMode == DR_NORMAL)
	{
		void (**primFunc)(uint8_t *);
		primFunc = primTableJ;

		for (; i < iSize;)
		{
			if (iDataWriteMode == DR_VRAMTRANSFER)
				goto STARTVRAM;

			gdata = *pMem++;
			i++;

			if (gpuDataC == 0)
			{
				command = (uint8_t)((gdata >> 24) & 0xff);

				if (primTableCX[command])
				{
					gpuDataC = primTableCX[command];
					gpuCommand = command;
					gpuDataM[0] = gdata;
					gpuDataP = 1;
				}
				else
					continue;
			}
			else
			{
				gpuDataM[gpuDataP] = gdata;
				if (gpuDataC > 128)
				{
					if ((gpuDataC == 254 && gpuDataP >= 3) || (gpuDataC == 255 && gpuDataP >= 4 && !(gpuDataP & 1)))
					{
						if ((gpuDataM[gpuDataP] & 0xF000F000) == 0x50005000)
							gpuDataP = gpuDataC - 1;
					}
				}
				gpuDataP++;
			}

			if (gpuDataP == gpuDataC)
			{
				gpuDataC = gpuDataP = 0;
				primFunc[gpuCommand]((uint8_t *)gpuDataM);

			}
		}
	}

	GPUdataRet = gdata;

	GPUIsReadyForCommands;
	GPUIsIdle;
}

////////////////////////////////////////////////////////////////////////

void WriteDataPSE(uint32_t gdata)
{
	WriteDataMemPSE(&gdata, 1);
}

////////////////////////////////////////////////////////////////////////
// Pete Special: make an 'intelligent' dma chain check (<-Tekken3)
////////////////////////////////////////////////////////////////////////

static uint32_t lUsedAddr[3];

static __inline bool CheckForEndlessLoop(uint32_t laddr)
{
	if (laddr == lUsedAddr[1])
		return true;
	if (laddr == lUsedAddr[2])
		return true;

	if (laddr < lUsedAddr[0])
		lUsedAddr[1] = laddr;
	else
		lUsedAddr[2] = laddr;
	lUsedAddr[0] = laddr;
	return false;
}

////////////////////////////////////////////////////////////////////////
// core gives a dma chain to gpu: same as the gpuwrite interface funcs
////////////////////////////////////////////////////////////////////////

int32_t DmaChainPSE(uint32_t *baseAddrL, uint32_t addr)
{
	uint32_t dmaMem;
	uint8_t *baseAddrB;
	int16_t count;
	uint32_t DMACommandCounter = 0;

	GPUIsBusy;

	lUsedAddr[0] = lUsedAddr[1] = lUsedAddr[2] = 0xffffff;

	baseAddrB = (uint8_t *)baseAddrL;

	do
	{
		if (iGPUHeight == 512)
			addr &= 0x1FFFFC;

		if (DMACommandCounter++ > 2000000)
			break;
		if (CheckForEndlessLoop(addr))
			break;

		count = baseAddrB[addr + 3];

		dmaMem = addr + 4;

		if (count > 0)
			WriteDataMemPSE(&baseAddrL[dmaMem >> 2], count);

		addr = baseAddrL[addr >> 2] & 0xffffff;
	} while (addr != 0xffffff);

	GPUIsIdle;

	return 0;
}


////////////////////////////////////////////////////////////////////////

int32_t FreezePSE(uint32_t ulGetFreezeData, GPUFreeze_t *pF)
{
	if (ulGetFreezeData == 2)
	{
		int lSlotNum = *((int *)pF);
		if (lSlotNum < 0)
			return 0;
		if (lSlotNum > 8)
			return 0;
		lSelectedSlot = lSlotNum + 1;
		return 1;
	}

	if (!pF)
		return 0;
	if (pF->ulFreezeVersion != 1)
		return 0;

	if (ulGetFreezeData == 1)
	{
		pF->ulStatus = STATUSREG;
		memcpy(pF->ulControl, ulStatusControl, 256 * sizeof(uint32_t));
		memcpy(pF->psxVRam, psxVub, 1024 * iGPUHeight * 2);

		return 1;
	}

	if (ulGetFreezeData != 0)
		return 0;

	STATUSREG = pF->ulStatus;
	memcpy(ulStatusControl, pF->ulControl, 256 * sizeof(uint32_t));
	memcpy(psxVub, pF->psxVRam, 1024 * iGPUHeight * 2);

	ResetTextureArea(true);

	WriteStatusPSE(ulStatusControl[0]);
	WriteStatusPSE(ulStatusControl[1]);
	WriteStatusPSE(ulStatusControl[2]);
	WriteStatusPSE(ulStatusControl[3]);
	WriteStatusPSE(ulStatusControl[8]);
	WriteStatusPSE(ulStatusControl[6]);
	WriteStatusPSE(ulStatusControl[7]);
	WriteStatusPSE(ulStatusControl[5]);
	WriteStatusPSE(ulStatusControl[4]);

	return 1;
}

////////////////////////////////////////////////////////////////////////
// special "emu infos" / "emu effects" functions
////////////////////////////////////////////////////////////////////////

// 00 = black
// 01 = white
// 10 = red
// 11 = transparent

static uint8_t cFont[10][120] = {
    // 0
    {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x05, 0x54, 0x00, 0x00, 0x80, 0x00,
     0x14, 0x05, 0x00, 0x00, 0x80, 0x00, 0x14, 0x05, 0x00, 0x00, 0x80, 0x00, 0x14, 0x05, 0x00, 0x00, 0x80, 0x00, 0x14,
     0x05, 0x00, 0x00, 0x80, 0x00, 0x14, 0x05, 0x00, 0x00, 0x80, 0x00, 0x14, 0x05, 0x00, 0x00, 0x80, 0x00, 0x14, 0x05,
     0x00, 0x00, 0x80, 0x00, 0x14, 0x05, 0x00, 0x00, 0x80, 0x00, 0x05, 0x54, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
     0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa},
    // 1
    {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x50, 0x00, 0x00, 0x80, 0x00,
     0x05, 0x50, 0x00, 0x00, 0x80, 0x00, 0x00, 0x50, 0x00, 0x00, 0x80, 0x00, 0x00, 0x50, 0x00, 0x00, 0x80, 0x00, 0x00,
     0x50, 0x00, 0x00, 0x80, 0x00, 0x00, 0x50, 0x00, 0x00, 0x80, 0x00, 0x00, 0x50, 0x00, 0x00, 0x80, 0x00, 0x00, 0x50,
     0x00, 0x00, 0x80, 0x00, 0x00, 0x50, 0x00, 0x00, 0x80, 0x00, 0x05, 0x55, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
     0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa},
    // 2
    {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x05, 0x54, 0x00, 0x00, 0x80, 0x00,
     0x14, 0x05, 0x00, 0x00, 0x80, 0x00, 0x00, 0x05, 0x00, 0x00, 0x80, 0x00, 0x00, 0x05, 0x00, 0x00, 0x80, 0x00, 0x00,
     0x14, 0x00, 0x00, 0x80, 0x00, 0x00, 0x50, 0x00, 0x00, 0x80, 0x00, 0x01, 0x40, 0x00, 0x00, 0x80, 0x00, 0x05, 0x00,
     0x00, 0x00, 0x80, 0x00, 0x14, 0x00, 0x00, 0x00, 0x80, 0x00, 0x15, 0x55, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
     0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa},
    // 3
    {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x05, 0x54, 0x00, 0x00, 0x80, 0x00,
     0x14, 0x05, 0x00, 0x00, 0x80, 0x00, 0x00, 0x05, 0x00, 0x00, 0x80, 0x00, 0x00, 0x05, 0x00, 0x00, 0x80, 0x00, 0x01,
     0x54, 0x00, 0x00, 0x80, 0x00, 0x00, 0x05, 0x00, 0x00, 0x80, 0x00, 0x00, 0x05, 0x00, 0x00, 0x80, 0x00, 0x00, 0x05,
     0x00, 0x00, 0x80, 0x00, 0x14, 0x05, 0x00, 0x00, 0x80, 0x00, 0x05, 0x54, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
     0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa},
    // 4
    {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x14, 0x00, 0x00, 0x80, 0x00,
     0x00, 0x54, 0x00, 0x00, 0x80, 0x00, 0x01, 0x54, 0x00, 0x00, 0x80, 0x00, 0x01, 0x54, 0x00, 0x00, 0x80, 0x00, 0x05,
     0x14, 0x00, 0x00, 0x80, 0x00, 0x14, 0x14, 0x00, 0x00, 0x80, 0x00, 0x15, 0x55, 0x00, 0x00, 0x80, 0x00, 0x00, 0x14,
     0x00, 0x00, 0x80, 0x00, 0x00, 0x14, 0x00, 0x00, 0x80, 0x00, 0x00, 0x55, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
     0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa},
    // 5
    {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x15, 0x55, 0x00, 0x00, 0x80, 0x00,
     0x14, 0x00, 0x00, 0x00, 0x80, 0x00, 0x14, 0x00, 0x00, 0x00, 0x80, 0x00, 0x14, 0x00, 0x00, 0x00, 0x80, 0x00, 0x15,
     0x54, 0x00, 0x00, 0x80, 0x00, 0x00, 0x05, 0x00, 0x00, 0x80, 0x00, 0x00, 0x05, 0x00, 0x00, 0x80, 0x00, 0x00, 0x05,
     0x00, 0x00, 0x80, 0x00, 0x14, 0x05, 0x00, 0x00, 0x80, 0x00, 0x05, 0x54, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
     0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa},
    // 6
    {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x01, 0x54, 0x00, 0x00, 0x80, 0x00,
     0x05, 0x00, 0x00, 0x00, 0x80, 0x00, 0x14, 0x00, 0x00, 0x00, 0x80, 0x00, 0x14, 0x00, 0x00, 0x00, 0x80, 0x00, 0x15,
     0x54, 0x00, 0x00, 0x80, 0x00, 0x15, 0x05, 0x00, 0x00, 0x80, 0x00, 0x14, 0x05, 0x00, 0x00, 0x80, 0x00, 0x14, 0x05,
     0x00, 0x00, 0x80, 0x00, 0x14, 0x05, 0x00, 0x00, 0x80, 0x00, 0x05, 0x54, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
     0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa},
    // 7
    {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x15, 0x55, 0x00, 0x00, 0x80, 0x00,
     0x14, 0x05, 0x00, 0x00, 0x80, 0x00, 0x00, 0x14, 0x00, 0x00, 0x80, 0x00, 0x00, 0x14, 0x00, 0x00, 0x80, 0x00, 0x00,
     0x50, 0x00, 0x00, 0x80, 0x00, 0x00, 0x50, 0x00, 0x00, 0x80, 0x00, 0x01, 0x40, 0x00, 0x00, 0x80, 0x00, 0x01, 0x40,
     0x00, 0x00, 0x80, 0x00, 0x05, 0x00, 0x00, 0x00, 0x80, 0x00, 0x05, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
     0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa},
    // 8
    {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x05, 0x54, 0x00, 0x00, 0x80, 0x00,
     0x14, 0x05, 0x00, 0x00, 0x80, 0x00, 0x14, 0x05, 0x00, 0x00, 0x80, 0x00, 0x14, 0x05, 0x00, 0x00, 0x80, 0x00, 0x05,
     0x54, 0x00, 0x00, 0x80, 0x00, 0x14, 0x05, 0x00, 0x00, 0x80, 0x00, 0x14, 0x05, 0x00, 0x00, 0x80, 0x00, 0x14, 0x05,
     0x00, 0x00, 0x80, 0x00, 0x14, 0x05, 0x00, 0x00, 0x80, 0x00, 0x05, 0x54, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
     0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa},
    // 9
    {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x05, 0x54, 0x00, 0x00, 0x80, 0x00,
     0x14, 0x05, 0x00, 0x00, 0x80, 0x00, 0x14, 0x05, 0x00, 0x00, 0x80, 0x00, 0x14, 0x05, 0x00, 0x00, 0x80, 0x00, 0x14,
     0x15, 0x00, 0x00, 0x80, 0x00, 0x05, 0x55, 0x00, 0x00, 0x80, 0x00, 0x00, 0x05, 0x00, 0x00, 0x80, 0x00, 0x00, 0x05,
     0x00, 0x00, 0x80, 0x00, 0x00, 0x14, 0x00, 0x00, 0x80, 0x00, 0x05, 0x50, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
     0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa}};

////////////////////////////////////////////////////////////////////////

static void PaintPicDot(uint8_t *p, uint8_t c)
{
	if (c == 0)
	{
		*p++ = 0x00;
		*p++ = 0x00;
		*p = 0x00;
		return;
	}
	if (c == 1)
	{
		*p++ = 0xff;
		*p++ = 0xff;
		*p = 0xff;
		return;
	}
	if (c == 2)
	{
		*p++ = 0x00;
		*p++ = 0x00;
		*p = 0xff;
		return;
	}
}

////////////////////////////////////////////////////////////////////////

void GetScreenPicPSE(uint8_t *pMem)
{
	float XS, YS;
	int x, y, v;
	uint8_t *ps, *px, *pf;
	uint8_t c;

	if (!pGfxCardScreen)
	{
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		pGfxCardScreen = (uint8_t *)malloc(iResX * iResY * 4);
	}

	ps = pGfxCardScreen;

	glReadBuffer(GL_FRONT);

	glReadPixels(0, 0, iResX, iResY, GL_RGB, GL_UNSIGNED_BYTE, ps);

	glReadBuffer(GL_BACK);

	XS = (float)iResX / 128;
	YS = (float)iResY / 96;
	pf = pMem;

	for (y = 96; y > 0; y--)
	{
		for (x = 0; x < 128; x++)
		{
			px = ps + (3 * ((int)((float)x * XS)) + (3 * iResX) * ((int)((float)y * YS)));
			*(pf + 0) = *(px + 2);
			*(pf + 1) = *(px + 1);
			*(pf + 2) = *(px + 0);
			pf += 3;
		}
	}

	/////////////////////////////////////////////////////////////////////
	// generic number/border painter

	pf = pMem + (103 * 3);

	for (y = 0; y < 20; y++)
	{
		for (x = 0; x < 6; x++)
		{
			c = cFont[lSelectedSlot][x + y * 6];
			v = (c & 0xc0) >> 6;
			PaintPicDot(pf, (uint8_t)v);
			pf += 3; // paint the dots into the rect
			v = (c & 0x30) >> 4;
			PaintPicDot(pf, (uint8_t)v);
			pf += 3;
			v = (c & 0x0c) >> 2;
			PaintPicDot(pf, (uint8_t)v);
			pf += 3;
			v = c & 0x03;
			PaintPicDot(pf, (uint8_t)v);
			pf += 3;
		}
		pf += 104 * 3;
	}

	pf = pMem;
	for (x = 0; x < 128; x++)
	{
		*(pf + (95 * 128 * 3)) = 0x00;
		*pf++ = 0x00;
		*(pf + (95 * 128 * 3)) = 0x00;
		*pf++ = 0x00;
		*(pf + (95 * 128 * 3)) = 0xff;
		*pf++ = 0xff;
	}
	pf = pMem;
	for (y = 0; y < 96; y++)
	{
		*(pf + (127 * 3)) = 0x00;
		*pf++ = 0x00;
		*(pf + (127 * 3)) = 0x00;
		*pf++ = 0x00;
		*(pf + (127 * 3)) = 0xff;
		*pf++ = 0xff;
		pf += 127 * 3;
	}
}

////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// main emu can set display infos (A/M/G/D)
////////////////////////////////////////////////////////////////////////

void VBlankPSE(int val)
{
	vBlank = val;
	oddLines = oddLines ? false : true; // bit changes per frame when not interlaced
	// printf("VB %x (%x)\n", oddLines, vBlank);
}

void HSyncPSE(int val)
{
	// Interlaced mode - update bit every scanline
	if (PSXDisplay.Interlaced)
	{
		oddLines = ((val % 2) ? false : true);
	}
	// printf("HS %x (%x)\n", oddLines, vBlank);
}
