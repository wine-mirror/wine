/* Window-specific OpenGL functions implementation.
 *
 * Copyright (c) 2000 Lionel Ulmer
 * Copyright (c) 2005 Raphael Junqueira
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __DLLS_OPENGL32_WGL_H
#define __DLLS_OPENGL32_WGL_H

#include "wingdi.h"

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
const char * WINAPI wglGetExtensionsStringARB(HDC hdc) ;

BOOL WINAPI wglMakeContextCurrentARB(HDC hDrawDC, HDC hReadDC, HGLRC hglrc);
HDC WINAPI wglGetCurrentReadDCARB(void) ;

#endif /* __DLLS_OPENGL32_WGL_H */
