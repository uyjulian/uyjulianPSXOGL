/***************************************************************************
                          external.h  -  description
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

#ifndef _EXTERNALS_H
#define _EXTERNALS_H

#define PSE_LT_GPU 2

#define MIRROR_TEST 1
#define SCISSOR_TEST 1

#define CLUTUSED 0x80000000

#define SETCOL(x)                                                                                                      \
	if (x.c.lcol != ulOLDCOL)                                                                                          \
	{                                                                                                                  \
		ulOLDCOL = x.c.lcol;                                                                                           \
		glColor4ubv(x.c.col);                                                                                          \
	}
#define SETPCOL(x)                                                                                                     \
	if (x->c.lcol != ulOLDCOL)                                                                                         \
	{                                                                                                                  \
		ulOLDCOL = x->c.lcol;                                                                                          \
		glColor4ubv(x->c.col);                                                                                         \
	}

#define INFO_TW 0
#define INFO_DRAWSTART 1
#define INFO_DRAWEND 2
#define INFO_DRAWOFF 3

#define SIGNSHIFT 21
#define CHKMAX_X 1024
#define CHKMAX_Y 512

// GPU STATUS REGISTER bit values (c) Lewpy

#define DR_NORMAL 0
#define DR_VRAMTRANSFER 1

#define GPUSTATUS_ODDLINES 0x80000000
#define GPUSTATUS_DMABITS 0x60000000 // Two bits
#define GPUSTATUS_READYFORCOMMANDS 0x10000000
#define GPUSTATUS_READYFORVRAM 0x08000000
#define GPUSTATUS_IDLE 0x04000000
#define GPUSTATUS_DISPLAYDISABLED 0x00800000
#define GPUSTATUS_INTERLACED 0x00400000
#define GPUSTATUS_RGB24 0x00200000
#define GPUSTATUS_PAL 0x00100000
#define GPUSTATUS_DOUBLEHEIGHT 0x00080000
#define GPUSTATUS_WIDTHBITS 0x00070000 // Three bits
#define GPUSTATUS_MASKENABLED 0x00001000
#define GPUSTATUS_MASKDRAWN 0x00000800
#define GPUSTATUS_DRAWINGALLOWED 0x00000400
#define GPUSTATUS_DITHER 0x00000200

#define STATUSREG lGPUstatusRet

#define GPUIsBusy (STATUSREG &= ~GPUSTATUS_IDLE)
#define GPUIsIdle (STATUSREG |= GPUSTATUS_IDLE)

#define GPUIsNotReadyForCommands (STATUSREG &= ~GPUSTATUS_READYFORCOMMANDS)
#define GPUIsReadyForCommands (STATUSREG |= GPUSTATUS_READYFORCOMMANDS)

#define KEY_RESETTEXSTORE 1
#define KEY_SHOWFPS 2
#define KEY_RESETOPAQUE 4
#define KEY_RESETDITHER 8
#define KEY_RESETFILTER 16
#define KEY_RESETADVBLEND 32
#define KEY_BLACKWHITE 64
#define KEY_TOGGLEFBTEXTURE 128
#define KEY_STEPDOWN 256
#define KEY_TOGGLEFBREAD 512

#define LOWORD(l) ((uint16_t)(l))
#define HIWORD(l) ((uint16_t)(((uint32_t)(l) >> 16) & 0xFFFF))
#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))

#define COMBINE_EXT 0x8570
#define COMBINE_RGB_EXT 0x8571
#define COMBINE_ALPHA_EXT 0x8572
#define SOURCE0_RGB_EXT 0x8580
#define SOURCE1_RGB_EXT 0x8581
#define SOURCE2_RGB_EXT 0x8582
#define SOURCE0_ALPHA_EXT 0x8588
#define SOURCE1_ALPHA_EXT 0x8589
#define SOURCE2_ALPHA_EXT 0x858A
#define OPERAND0_RGB_EXT 0x8590
#define OPERAND1_RGB_EXT 0x8591
#define OPERAND2_RGB_EXT 0x8592
#define OPERAND0_ALPHA_EXT 0x8598
#define OPERAND1_ALPHA_EXT 0x8599
#define OPERAND2_ALPHA_EXT 0x859A
#define RGB_SCALE_EXT 0x8573
#define ADD_SIGNED_EXT 0x8574
#define INTERPOLATE_EXT 0x8575
#define CONSTANT_EXT 0x8576
#define PRIMARY_COLOR_EXT 0x8577
#define PREVIOUS_EXT 0x8578

#define FUNC_ADD_EXT 0x8006
#define FUNC_REVERSESUBTRACT_EXT 0x800B

typedef void (*PFNGLBLENDEQU)(GLenum mode);
typedef void (*PFNGLCOLORTABLEEXT)(GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type,
                                   const GLvoid *data);

#define GL_UNSIGNED_SHORT_4_4_4_4_EXT 0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1_EXT 0x8034

#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE

#ifndef GL_BGRA_EXT
#define GL_BGRA_EXT 0x80E1
#endif

#ifndef GL_COLOR_INDEX8_EXT
#define GL_COLOR_INDEX8_EXT 0x80E5
#endif

typedef struct RECTTAG
{
	int left;
	int top;
	int right;
	int bottom;
} RECT;

typedef struct VRAMLOADTAG
{
	int16_t x;
	int16_t y;
	int16_t Width;
	int16_t Height;
	int16_t RowsRemaining;
	int16_t ColsRemaining;
	uint16_t *ImagePtr;
} VRAMLoad_t;

typedef struct PSXPOINTTAG
{
	int x;
	int y;
} PSXPoint_t;

typedef struct PSXSPOINTTAG
{
	int16_t x;
	int16_t y;
} PSXSPoint_t;

typedef struct PSXRECTTAG
{
	int16_t x0;
	int16_t x1;
	int16_t y0;
	int16_t y1;
} PSXRect_t;

typedef struct TWINTAG
{
	PSXRect_t Position;
	PSXRect_t OPosition;
	PSXPoint_t TextureSize;
	float UScaleFactor;
	float VScaleFactor;
} TWin_t;

typedef struct PSXDISPLAYTAG
{
	PSXPoint_t DisplayModeNew;
	PSXPoint_t DisplayMode;
	PSXPoint_t DisplayPosition;
	PSXPoint_t DisplayEnd;

	int Double;
	int Height;
	int PAL;
	int InterlacedNew;
	int Interlaced;
	int InterlacedTest;
	int RGB24New;
	int RGB24;
	PSXSPoint_t DrawOffset;
	PSXRect_t DrawArea;
	PSXPoint_t GDrawOffset;
	PSXPoint_t CumulOffset;
	int Disabled;
	PSXRect_t Range;
} PSXDisplay_t;

typedef union COLTAG
{
	uint8_t col[4];
	uint32_t lcol;
} COLTAG;

typedef struct OGLVertexTag
{
	GLfloat x;
	GLfloat y;
	GLfloat z;

	GLfloat sow;
	GLfloat tow;

	COLTAG c;
} OGLVertex;

typedef struct OGLVertexTagInteger
{
	GLint x;
	GLint y;
	GLint z;

	GLfloat sow;
	GLfloat tow;

	COLTAG c;
} OGLVertexInteger;

typedef union EXShortTag
{
	uint8_t c[2];
	uint16_t s;
} EXShort;

typedef union EXLongTag
{
	uint8_t c[4];
	uint32_t l;
	EXShort s[2];
} EXLong;

extern char *pConfigFile;

extern int iResX;
extern int iResY;
extern bool bKeepRatio;
extern bool bForceRatio43;
extern RECT rRatioRect;
extern bool bSnapShot;
extern bool bSmallAlpha;
extern bool bOpaquePass;
extern bool bAdvancedBlend;
extern bool bUseLines;
extern int iTexQuality;
extern bool bUseAntiAlias;
extern bool bGLExt;
extern bool bGLFastMovie;
extern bool bGLSoft;
extern bool bGLBlend;

extern PFNGLBLENDEQU glBlendEquationEXTEx;
extern PFNGLCOLORTABLEEXT glColorTableEXTEx;

extern uint8_t gl_ux[8];
extern uint8_t gl_vy[8];
extern OGLVertex vertex[4];
extern int16_t sprtY, sprtX, sprtH, sprtW;
extern bool bIsFirstFrame;
extern int iWinSize;
extern int iZBufferDepth;
extern GLbitfield uiBufferBits;
extern int iUseMask;
extern int iSetMask;
extern int iDepthFunc;
extern bool bCheckMask;
extern uint16_t sSetMask;
extern uint32_t lSetMask;
extern int iShowFPS;
extern bool bGteAccuracy;
extern bool bSetClip;
extern int iForceVSync;
extern int iUseExts;
extern int iUsePalTextures;
extern GLuint gTexScanName;

extern int GlobalTextAddrX, GlobalTextAddrY, GlobalTextTP;
extern int GlobalTextREST, GlobalTextABR, GlobalTextPAGE;
extern int16_t ly0, lx0, ly1, lx1, ly2, lx2, ly3, lx3;
extern int16_t g_m1;
extern int16_t g_m2;
extern int16_t g_m3;
extern int16_t DrawSemiTrans;

extern bool bNeedUploadTest;
extern bool bNeedUploadAfter;
extern bool bTexEnabled;
extern bool bBlendEnable;
extern bool bDrawDither;
extern int iFilterType;
extern bool bFullVRam;
extern bool bUseMultiPass;
extern int iOffscreenDrawing;
extern bool bOldSmoothShaded;
extern bool bUsingTWin;
extern bool bUsingMovie;
extern PSXRect_t xrMovieArea;
extern PSXRect_t xrUploadArea;
extern PSXRect_t xrUploadAreaIL;
extern PSXRect_t xrUploadAreaRGB24;
extern GLuint gTexName;
extern bool bDrawNonShaded;
extern bool bDrawMultiPass;
extern GLubyte ubGloColAlpha;
extern GLubyte ubGloAlpha;
extern int16_t sSprite_ux2;
extern int16_t sSprite_vy2;
extern bool bRenderFrontBuffer;
extern uint32_t ulOLDCOL;
extern uint32_t ulClutID;
extern void (*primTableJ[256])(uint8_t *);
extern void (*primTableSkip[256])(uint8_t *);
extern uint16_t usMirror;
extern uint32_t dwCfgFixes;
extern uint32_t dwActFixes;
extern uint32_t dwEmuFixes;
extern bool bUseFixes;
extern int iSpriteTex;
extern int iDrawnSomething;

extern int drawX;
extern int drawY;
extern int drawW;
extern int drawH;
extern int16_t sxmin;
extern int16_t sxmax;
extern int16_t symin;
extern int16_t symax;

extern uint8_t ubOpaqueDraw;
extern GLint giWantedRGBA;
extern GLint giWantedFMT;
extern GLint giWantedTYPE;
extern void (*LoadSubTexFn)(int, int, int16_t, int16_t);
extern int GlobalTexturePage;
extern uint32_t (*TCF[])(uint32_t);
extern uint16_t (*PTCF[])(uint16_t);
extern uint32_t (*PalTexturedColourFn)(uint32_t);
extern bool bUseFastMdec;
extern bool bUse15bitMdec;
extern int iFrameTexType;
extern int iFrameReadType;
extern int iClampType;
extern int iSortTexCnt;
extern bool bFakeFrontBuffer;
extern GLuint gTexFrameName;
extern GLuint gTexBlurName;
extern int iVRamSize;
extern int iTexGarbageCollection;
extern int iFTexA;
extern int iFTexB;
extern int iHiResTextures;
extern bool bIgnoreNextTile;

extern VRAMLoad_t VRAMWrite;
extern VRAMLoad_t VRAMRead;
extern int iDataWriteMode;
extern int iDataReadMode;
extern int iColDepth;
extern bool bChangeRes;
extern bool bWindowMode;
extern char szDispBuf[];
extern char szGPUKeys[];
extern PSXDisplay_t PSXDisplay;
extern PSXDisplay_t PreviousPSXDisplay;
extern uint32_t ulKeybits;
extern TWin_t TWin;
extern bool bDisplayNotSet;
extern int lGPUstatusRet;
extern int16_t imageX0, imageX1;
extern int16_t imageY0, imageY1;
extern int lClearOnSwap, lClearOnSwapColor;
extern uint8_t *psxVub;
extern int8_t *psxVsb;
extern uint16_t *psxVuw;
extern int16_t *psxVsw;
extern uint32_t *psxVul;
extern int32_t *psxVsl;
extern GLfloat gl_z;
extern bool bNeedRGB24Update;
extern bool bChangeWinMode;
extern GLuint uiScanLine;
extern int iUseScanLines;
extern float iScanlineColor[]; /* 4 element array of RGBA float */
extern int lSelectedSlot;
extern int iScanBlend;
extern bool bInitCap;
extern int iBlurBuffer;
extern int iLastRGB24;
extern int iRenderFVR;
extern int iNoScreenSaver;
extern uint32_t ulGPUInfoVals[];
extern bool bNeedInterlaceUpdate;
extern bool bNeedWriteUpload;
extern bool bSkipNextFrame;
extern uint32_t vBlank;

extern int bFullScreen;

extern uint32_t dwCoreFlags;
extern GLuint gTexPicName;
extern PSXPoint_t ptCursorPoint[];
extern uint16_t usCursorActive;

extern bool bUseFrameLimit;
extern bool bUseFrameSkip;
extern float fFrameRate;
extern float fFrameRateHz;
extern int iFrameLimit;
extern float fps_skip;
extern float fps_cur;

extern uint32_t ulKeybits;

extern uint32_t dwGPUVersion;
extern int iGPUHeight;
extern int iGPUHeightMask;
extern int GlobalTextIL;
extern int iTileCheat;

void ReadConfig(void);
void ReadConfigFile(void);
// internally used defines

#define GPUCOMMAND(x) ((x >> 24) & 0xff)
#define RED(x) (x & 0xff)
#define BLUE(x) ((x >> 16) & 0xff)
#define GREEN(x) ((x >> 8) & 0xff)
#define COLOR(x) (x & 0xffffff)
#define PRED(x) ((x << 3) & 0xF8)
#define PBLUE(x) ((x >> 2) & 0xF8)
#define PGREEN(x) ((x >> 7) & 0xF8)

#define TEXTUREPAGESIZE 256 * 256

// prototypes

extern int GLinitialize(void);
extern int GLrefresh(void);
extern void GLcleanup(void);
extern bool offset3(void);
extern bool offset4(void);
extern bool offsetline(void);
extern void offsetST(void);
extern void offsetBlk(void);
extern void offsetScreenUpload(int Position);
extern void assignTexture3(void);
extern void assignTexture4(void);
extern void assignTextureSprite(void);
extern void assignTextureVRAMWrite(void);
extern void SetOGLDisplaySettings(bool DisplaySet);
extern void ReadConfig(void);
extern void WriteConfig(void);
extern void SetExtGLFuncs(void);
extern void CreateScanLines(void);

extern void InitFrameCap(void);
extern void SetFrameRateConfig(void);
extern void PCFrameCap(void);
extern void PCcalcfps(void);
extern void FrameSkip(void);
extern void CheckFrameRate(void);
extern void ReInitFrameCap(void);
extern void SetAutoFrameCap(void);

extern uint32_t timeGetTime(void);

extern void DoSnapShot(void);
extern void updateDisplay(void);
extern void updateFrontDisplay(void);
extern void SetAutoFrameCap(void);
extern void SetAspectRatio(void);
extern void CheckVRamRead(int x, int y, int dx, int dy, bool bFront);
extern void CheckVRamReadEx(int x, int y, int dx, int dy);
extern void SetFixes(void);

extern void resetGteVertices(void);
extern int getGteVertex(int16_t sx, int16_t sy, float *fx, float *fy);

extern void ResetStuff(void);

extern void DisplayText(void);
extern void HideText(void);
extern void KillDisplayLists(void);
extern void MakeDisplayLists(void);
extern void BuildDispMenu(int iInc);
extern void SwitchDispMenu(int iStep);
extern void CreatePic(uint8_t *pMem);
extern void DisplayPic(void);
extern void DestroyPic(void);
extern void ShowGunCursor(void);

extern void UploadScreen(int Position);
extern void PrepareFullScreenUpload(int Position);
extern bool CheckAgainstScreen(int16_t imageX0, int16_t imageY0, int16_t imageX1, int16_t imageY1);
extern bool CheckAgainstFrontScreen(int16_t imageX0, int16_t imageY0, int16_t imageX1, int16_t imageY1);
extern bool FastCheckAgainstScreen(int16_t imageX0, int16_t imageY0, int16_t imageX1, int16_t imageY1);
extern bool FastCheckAgainstFrontScreen(int16_t imageX0, int16_t imageY0, int16_t imageX1, int16_t imageY1);
extern bool bCheckFF9G4(unsigned char *baseAddr);
extern void SetScanTrans(void);
extern void SetScanTexTrans(void);
extern void DrawMultiBlur(void);
extern void CheckWriteUpdate(void);

extern void offsetPSXLine(void);
extern void offsetPSX3(void);
extern void offsetPSX4(void);

extern void FillSoftwareAreaTrans(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t col);
extern void FillSoftwareArea(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t col);
extern void drawPoly3G(int rgb1, int rgb2, int rgb3);
extern void drawPoly4G(int rgb1, int rgb2, int rgb3, int rgb4);
extern void drawPoly3F(int rgb);
extern void drawPoly4F(int rgb);
extern void drawPoly4FT(uint8_t *baseAddr);
extern void drawPoly4GT(uint8_t *baseAddr);
extern void drawPoly3FT(uint8_t *baseAddr);
extern void drawPoly3GT(uint8_t *baseAddr);
extern void DrawSoftwareSprite(uint8_t *baseAddr, int16_t w, int16_t h, int tx, int ty);
extern void DrawSoftwareSpriteTWin(uint8_t *baseAddr, int w, int h);
extern void DrawSoftwareSpriteMirror(uint8_t *baseAddr, int w, int h);

extern void InitializeTextureStore(void);
extern void CleanupTextureStore(void);
extern GLuint LoadTextureWnd(int pageid, int TextureMode, uint32_t GivenClutId);
extern GLuint LoadTextureMovie(void);
extern void InvalidateTextureArea(int imageX0, int imageY0, int imageX1, int imageY1);
extern void InvalidateTextureAreaEx(void);
extern void LoadTexturePage(int pageid, int mode, int16_t cx, int16_t cy);
extern void ResetTextureArea(bool bDelTex);
extern GLuint SelectSubTextureS(int TextureMode, uint32_t GivenClutId);
extern void CheckTextureMemory(void);

extern void LoadSubTexturePage(int pageid, int mode, int16_t cx, int16_t cy);
extern void LoadSubTexturePageSort(int pageid, int mode, int16_t cx, int16_t cy);
extern void LoadPackedSubTexturePage(int pageid, int mode, int16_t cx, int16_t cy);
extern void LoadPackedSubTexturePageSort(int pageid, int mode, int16_t cx, int16_t cy);
extern uint32_t XP8RGBA(uint32_t BGR);
extern uint32_t XP8RGBAEx(uint32_t BGR);
extern uint32_t XP8RGBA_0(uint32_t BGR);
extern uint32_t XP8RGBAEx_0(uint32_t BGR);
extern uint32_t XP8BGRA_0(uint32_t BGR);
extern uint32_t XP8BGRAEx_0(uint32_t BGR);
extern uint32_t XP8RGBA_1(uint32_t BGR);
extern uint32_t XP8RGBAEx_1(uint32_t BGR);
extern uint32_t XP8BGRA_1(uint32_t BGR);
extern uint32_t XP8BGRAEx_1(uint32_t BGR);
extern uint32_t P8RGBA(uint32_t BGR);
extern uint32_t P8BGRA(uint32_t BGR);
extern uint32_t CP8RGBA_0(uint32_t BGR);
extern uint32_t CP8RGBAEx_0(uint32_t BGR);
extern uint32_t CP8BGRA_0(uint32_t BGR);
extern uint32_t CP8BGRAEx_0(uint32_t BGR);
extern uint32_t CP8RGBA(uint32_t BGR);
extern uint32_t CP8RGBAEx(uint32_t BGR);
extern uint16_t XP5RGBA_0(uint16_t BGR);
extern uint16_t XP5RGBA_1(uint16_t BGR);
extern uint16_t P5RGBA(uint16_t BGR);
extern uint16_t CP5RGBA_0(uint16_t BGR);
extern uint16_t XP4RGBA_0(uint16_t BGR);
extern uint16_t XP4RGBA_1(uint16_t BGR);
extern uint16_t P4RGBA(uint16_t BGR);
extern uint16_t CP4RGBA_0(uint16_t BGR);

extern uint8_t *LoadDirectMovieFast(void);

#endif
