#ifndef _EXTERNALS_H
#define _EXTERNALS_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
//#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <math.h>

#define __inline inline

#define PSE_LT_GPU 2

#define CLUTUSED 0x80000000

#define CLUTCHK 0x00060000
#define CLUTSHIFT 17

#define SHADETEXBIT(x) ((x >> 24) & 0x1)
#define SEMITRANSBIT(x) ((x >> 25) & 0x1)

#define MAXTPAGES_MAX 64
#define MAXSORTTEX_MAX 196

#define CSUBSIZE 2048

#define CSUBSIZES 4096

#define SOFFA 0
#define SOFFB 1024
#define SOFFC 2048
#define SOFFD 3072

#define MAXWNDTEXCACHE 128

#define XCHECK(pos1, pos2)                                                                                             \
	((pos1.c[0] >= pos2.c[1]) && (pos1.c[1] <= pos2.c[0]) && (pos1.c[2] >= pos2.c[3]) && (pos1.c[3] <= pos2.c[2]))
#define INCHECK(pos2, pos1)                                                                                            \
	((pos1.c[0] <= pos2.c[0]) && (pos1.c[1] >= pos2.c[1]) && (pos1.c[2] <= pos2.c[2]) && (pos1.c[3] >= pos2.c[3]))

#define EqualRect(pr1, pr2)                                                                                            \
	((pr1)->left == (pr2)->left && (pr1)->top == (pr2)->top && (pr1)->right == (pr2)->right &&                         \
	 (pr1)->bottom == (pr2)->bottom)

#define DEFOPAQUEON()                                                                                                    \
	{glAlphaFunc(GL_EQUAL, 0.0f);                                                                                       \
	bBlendEnable = false;                                                                                              \
	glDisable(GL_BLEND);}
#define DEFOPAQUEOFF() glAlphaFunc(GL_GREATER, 0.49f)

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
//#define GPUSTATUS_MASKENABLED 0x00001000
//#define GPUSTATUS_MASKDRAWN 0x00000800
//#define GPUSTATUS_DRAWINGALLOWED 0x00000400
//#define GPUSTATUS_DITHER 0x00000200

#define STATUSREG lGPUstatusRet

#define GPUIsBusy() (STATUSREG &= ~GPUSTATUS_IDLE)
#define GPUIsIdle() (STATUSREG |= GPUSTATUS_IDLE)

#define GPUIsNotReadyForCommands() (STATUSREG &= ~GPUSTATUS_READYFORCOMMANDS)
#define GPUIsReadyForCommands() (STATUSREG |= GPUSTATUS_READYFORCOMMANDS)

#define LOWORD(l) ((uint16_t)(l))
#define HIWORD(l) ((uint16_t)(((uint32_t)(l) >> 16) & 0xFFFF))
#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))

// internally used defines

#define GPUCOMMAND(x) ((x >> 24) & 0xff)
#define RED(x) (x & 0xff)
#define BLUE(x) ((x >> 16) & 0xff)
#define GREEN(x) ((x >> 8) & 0xff)
#define COLOR(x) (x & 0xffffff)
#define PRED(x) ((x << 3) & 0xF8)
#define PBLUE(x) ((x >> 2) & 0xF8)
#define PGREEN(x) ((x >> 7) & 0xF8)

typedef struct RECTTAG
{
	int32_t left;
	int32_t top;
	int32_t right;
	int32_t bottom;
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
	int32_t x;
	int32_t y;
} PSXPoint_t;

typedef struct PSXSPOINTTAG
{
	int32_t x;
	int32_t y;
} PSXSPoint_t;

typedef struct PSXRECTTAG
{
	int32_t x0;
	int32_t x1;
	int32_t y0;
	int32_t y1;
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

	int32_t Double;
	int32_t Height;
	int32_t PAL;
	int32_t InterlacedNew;
	int32_t Interlaced;
	int32_t InterlacedTest;
	int32_t RGB24New;
	int32_t RGB24;
	PSXSPoint_t DrawOffset;
	PSXRect_t DrawArea;
	PSXPoint_t GDrawOffset;
	PSXPoint_t CumulOffset;
	int32_t Disabled;
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

typedef struct GPUFREEZETAG
{
	uint32_t ulFreezeVersion;         // should be always 1 for now (set by main emu)
	uint32_t ulStatus;                // current gpu status
	uint32_t ulControl[256];          // latest control register values
	uint8_t psxVRam[1024 * 1024 * 2]; // current VRam image (full 2 MB for ZN)
} GPUFreeze_t;

extern int32_t iResX;
extern int32_t iResY;

extern uint8_t gl_ux[8];
extern uint8_t gl_vy[8];
extern OGLVertex vertex[4];
extern int32_t sprtY, sprtX, sprtH, sprtW;
extern glm::mat4 projectionMatrix;

extern int32_t GlobalTextAddrX, GlobalTextAddrY, GlobalTextTP;
extern int32_t GlobalTextREST, GlobalTextABR;
extern int16_t ly0, lx0, ly1, lx1, ly2, lx2, ly3, lx3;
extern int16_t g_m1;
extern int16_t g_m2;
extern int16_t g_m3;
extern int16_t DrawSemiTrans;

extern bool bNeedUploadTest;
extern bool bNeedUploadAfter;
extern bool bTexEnabled;
extern bool bFullVRam;
extern bool bUsingTWin;
extern PSXRect_t xrMovieArea;
extern PSXRect_t xrUploadArea;
extern PSXRect_t xrUploadAreaIL;
extern GLuint gTexName;
extern bool bDrawMultiPass;
extern GLubyte ubGloColAlpha;
extern GLubyte ubGloAlpha;
extern bool bRenderFrontBuffer;
extern void (*primTableJ[256])(uint8_t *);
extern uint16_t usMirror;
extern int32_t iSpriteTex;
extern int32_t iDrawnSomething;

extern int32_t drawX;
extern int32_t drawY;
extern int32_t drawW;
extern int32_t drawH;
extern int16_t sxmin;
extern int16_t sxmax;
extern int16_t symin;
extern int16_t symax;

extern uint8_t ubOpaqueDraw;
extern void (*LoadSubTexFn)(int32_t, int32_t, int16_t, int16_t);
extern int32_t GlobalTexturePage;
extern uint32_t (*TCF[])(uint32_t);
extern int32_t iFrameReadType;
extern bool bFakeFrontBuffer;
extern GLuint gTexFrameName;
extern bool bIgnoreNextTile;

extern VRAMLoad_t VRAMWrite;
extern VRAMLoad_t VRAMRead;
extern int32_t iDataWriteMode;
extern int32_t iDataReadMode;
extern PSXDisplay_t PSXDisplay;
extern PSXDisplay_t PreviousPSXDisplay;
extern TWin_t TWin;
extern bool bDisplayNotSet;
extern int32_t lGPUstatusRet;
extern int32_t lClearOnSwap, lClearOnSwapColor;
extern uint8_t *psxVub;
extern uint16_t *psxVuw;
extern GLfloat gl_z;
extern bool bNeedRGB24Update;
extern int32_t iLastRGB24;
extern uint32_t ulGPUInfoVals[];
extern bool bNeedInterlaceUpdate;
extern bool bNeedWriteUpload;
extern uint32_t dwGPUVersion;
extern int32_t iGPUHeight;
extern int32_t iGPUHeightMask;
extern int32_t GlobalTextIL;

// prototypes

extern "C" const char *PSEgetLibName(void);
extern "C" uint32_t PSEgetLibType(void);
extern "C" uint32_t PSEgetLibVersion(void);
extern "C" const char *GPUgetLibInfos(void);
extern "C" void GPUmakeSnapshot(void);
extern "C" int32_t GPUinit(void);
extern "C" int32_t GPUopen(uint32_t *, char *, char *);
extern "C" int32_t GPUclose(void);
extern "C" int32_t GPUshutdown(void);
extern "C" void GPUcursor(int32_t, int32_t, int32_t);
extern "C" void GPUupdateLace(void);
extern "C" uint32_t GPUreadStatus(void);
extern "C" void GPUwriteStatus(uint32_t);
extern "C" void GPUreadDataMem(uint32_t *, int32_t);
extern "C" uint32_t GPUreadData(void);
extern "C" void GPUwriteDataMem(uint32_t *, int32_t);
extern "C" int32_t GPUconfigure(void);
extern "C" void GPUwriteData(uint32_t);
extern "C" void GPUabout(void);
extern "C" int32_t GPUtest(void);
extern "C" int32_t GPUdmaChain(uint32_t *, uint32_t);
extern "C" int32_t GPUfreeze(uint32_t, GPUFreeze_t *);
extern "C" void GPUgetScreenPic(uint8_t *);
extern "C" void GPUshowScreenPic(uint8_t *);
extern "C" void GPUsetfix(uint32_t);
extern "C" void GPUvisualVibration(uint32_t, uint32_t);
extern "C" void GPUvBlank(int32_t);
extern "C" void GPUhSync(int32_t);

extern int32_t LoadPSE();
extern int32_t OpenPSE();
extern int32_t ClosePSE();
extern int32_t ShutdownPSE();
extern void UpdateLacePSE();
extern uint32_t ReadStatusPSE();
extern void WriteStatusPSE(uint32_t);
extern void ReadDataMemPSE(uint32_t *, int32_t);
extern uint32_t ReadDataPSE();
extern void WriteDataMemPSE(uint32_t *, int32_t);
extern void WriteDataPSE(uint32_t);
extern int32_t DmaChainPSE(uint32_t *, uint32_t);
extern int32_t FreezePSE(uint32_t, GPUFreeze_t *);
extern void VBlankPSE(int32_t);
extern void HSyncPSE(int32_t);

extern void clearWithColor(int32_t);
extern void clearToBlack(void);

extern int32_t GLinitialize(void);
extern void GLcleanup(void);
extern void SetOGLDisplaySettings(bool);
extern void SetExtGLFuncs(void);

extern void updateDisplay(void);
extern void updateFrontDisplay(void);
extern void CheckVRamRead(int32_t, int32_t, int32_t, int32_t, bool);
extern void CheckVRamReadEx(int32_t, int32_t, int32_t, int32_t);

extern void UploadScreen(int32_t);
extern void PrepareFullScreenUpload(int32_t);
extern bool CheckAgainstScreen(int16_t, int16_t, int16_t, int16_t);
extern bool CheckAgainstFrontScreen(int16_t, int16_t, int16_t, int16_t);
extern bool FastCheckAgainstScreen(int16_t, int16_t, int16_t, int16_t);
extern bool FastCheckAgainstFrontScreen(int16_t, int16_t, int16_t, int16_t);
extern void CheckWriteUpdate(void);

extern void FillSoftwareAreaTrans(int16_t, int16_t, int16_t, int16_t, uint16_t);
extern void FillSoftwareArea(int16_t, int16_t, int16_t, int16_t, uint16_t);
extern void drawPoly3G(int32_t, int32_t, int32_t);
extern void drawPoly4G(int32_t, int32_t, int32_t, int32_t);
extern void drawPoly3F(int32_t);
extern void drawPoly4F(int32_t);
extern void drawPoly4FT(uint8_t *);
extern void drawPoly4GT(uint8_t *);
extern void drawPoly3FT(uint8_t *);
extern void drawPoly3GT(uint8_t *);
extern void DrawSoftwareSprite(uint8_t *, int16_t, int16_t, int32_t, int32_t);
extern void DrawSoftwareSpriteTWin(uint8_t *, int32_t, int32_t);
extern void DrawSoftwareSpriteMirror(uint8_t *, int32_t, int32_t);

extern void InitializeTextureStore(void);
extern void CleanupTextureStore(void);
extern GLuint LoadTextureWnd(int32_t, int32_t, uint32_t);
extern GLuint LoadTextureMovie(void);
extern void InvalidateTextureArea(int32_t, int32_t, int32_t, int32_t);
extern void InvalidateTextureAreaEx(void);
extern void ResetTextureArea(bool);
extern GLuint SelectSubTextureS(int32_t, uint32_t);
extern void CheckTextureMemory(void);

extern void LoadSubTexturePage(int32_t, int32_t, int16_t, int16_t);
extern void LoadSubTexturePageSort(int32_t, int32_t, int16_t, int16_t);
extern void LoadPackedSubTexturePage(int32_t, int32_t, int16_t, int16_t);
extern void LoadPackedSubTexturePageSort(int32_t, int32_t, int16_t, int16_t);
extern uint32_t XP8RGBA_0(uint32_t);
extern uint32_t XP8RGBA_1(uint32_t);

#endif
