/* Window-specific OpenGL functions implementation.

     Copyright (c) 2000 Lionel Ulmer
*/

#ifndef __DLLS_OPENGL32_WGL_H
#define __DLLS_OPENGL32_WGL_H

#include "gdi.h"

typedef void *HGLRC;

typedef struct {
  WORD  nSize; 
  WORD  nVersion; 
  DWORD dwFlags; 
  BYTE  iPixelType; 
  BYTE  cColorBits; 
  BYTE  cRedBits; 
  BYTE  cRedShift; 
  BYTE  cGreenBits; 
  BYTE  cGreenShift; 
  BYTE  cBlueBits; 
  BYTE  cBlueShift; 
  BYTE  cAlphaBits; 
  BYTE  cAlphaShift; 
  BYTE  cAccumBits; 
  BYTE  cAccumRedBits; 
  BYTE  cAccumGreenBits; 
  BYTE  cAccumBlueBits; 
  BYTE  cAccumAlphaBits; 
  BYTE  cDepthBits; 
  BYTE  cStencilBits; 
  BYTE  cAuxBuffers; 
  BYTE  iLayerPlane;
  BYTE  bReserved; 
  COLORREF crTransparent; 
} LAYERPLANEDESCRIPTOR; 
typedef LAYERPLANEDESCRIPTOR* LPLAYERPLANEDESCRIPTOR;

typedef struct {
  FLOAT      x; 
  FLOAT      y; 
} POINTFLOAT; 

typedef struct {
  FLOAT      gmfBlackBoxX; 
  FLOAT      gmfBlackBoxY; 
  POINTFLOAT gmfptGlyphOrigin; 
  FLOAT      gmfCellIncX; 
  FLOAT      gmfCellIncY; 
} GLYPHMETRICSFLOAT; 
typedef GLYPHMETRICSFLOAT *LPGLYPHMETRICSFLOAT;

HGLRC WINAPI wglCreateContext(HDC hdc) ;
HGLRC WINAPI wglCreateLayerContext(HDC hdc,
				   int iLayerPlane) ;
BOOL WINAPI wglCopyContext(HGLRC hglrcSrc,
			   HGLRC hglrcDst,
			   UINT mask) ;
BOOL WINAPI wglDeleteContext(HGLRC hglrc) ;
BOOL WINAPI wglDescribeLayerPlane(HDC hdc,
				  int iPixelFormat,
				  int iLayerPlane,
				  UINT nBytes,
				  LPLAYERPLANEDESCRIPTOR plpd) ;
HGLRC WINAPI wglGetCurrentContext(void) ;
HDC WINAPI wglGetCurrentDC(void) ;
int WINAPI wglGetLayerPaletteEntries(HDC hdc,
				     int iLayerPlane,
				     int iStart,
				     int cEntries,
				     const COLORREF *pcr) ;
void * WINAPI wglGetProcAddress(LPCSTR  lpszProc) ;
BOOL WINAPI wglMakeCurrent(HDC hdc,
			   HGLRC hglrc) ;
BOOL WINAPI wglRealizeLayerPalette(HDC hdc,
				   int iLayerPlane,
				   BOOL bRealize) ;
int WINAPI wglSetLayerPaletteEntries(HDC hdc,
				     int iLayerPlane,
				     int iStart,
				     int cEntries,
				     const COLORREF *pcr) ;
BOOL WINAPI wglShareLists(HGLRC hglrc1,
			  HGLRC hglrc2) ;
BOOL WINAPI wglSwapLayerBuffers(HDC hdc,
				UINT fuPlanes);
BOOL WINAPI wglUseFontBitmaps(HDC hdc,
			      DWORD first,
			      DWORD count,
			      DWORD listBase) ;
BOOL WINAPI wglUseFontOutlines(HDC hdc,
			       DWORD first,
			       DWORD count,
			       DWORD listBase,
			       FLOAT deviation,
			       FLOAT extrusion,
			       int format,
			       LPGLYPHMETRICSFLOAT lpgmf) ;

#endif /* __DLLS_OPENGL32_WGL_H */

