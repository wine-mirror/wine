/*
 * Direct3D wine types include file
 *
 * Copyright 2002-2003 The wine-d3d team
 * Copyright 2002-2003 Jason Edmeades
 *                     Raphael Junqueira
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

#ifndef __WINE_WINED3D_TYPES_H
#define __WINE_WINED3D_TYPES_H

typedef struct _D3DVECTOR_3 {
    float x;
    float y;
    float z;
} D3DVECTOR_3;

typedef struct _D3DVECTOR_4 {
    float x;
    float y;
    float z;
    float w;
} D3DVECTOR_4;

typedef struct D3DSHADERVECTOR {
  float x;
  float y;
  float z;
  float w;
} D3DSHADERVECTOR;

typedef struct D3DSHADERSCALAR {
  float x;
} D3DSHADERSCALAR;

typedef D3DSHADERVECTOR VSHADERCONSTANTS8[D3D_VSHADER_MAX_CONSTANTS];

typedef struct VSHADERDATA {
  /** Run Time Shader Function Constants */
  /*D3DXBUFFER* constants;*/
  VSHADERCONSTANTS8 C;
  /** Shader Code as char ... */
  CONST DWORD* code;
  UINT codeLength;
} VSHADERDATA;

/** temporary here waiting for buffer code */
typedef struct VSHADERINPUTDATA {
  D3DSHADERVECTOR V[17];
} VSHADERINPUTDATA;

/** temporary here waiting for buffer code */
typedef struct VSHADEROUTPUTDATA {
  D3DSHADERVECTOR oPos;
  D3DSHADERVECTOR oD[2];
  D3DSHADERVECTOR oT[8];
  D3DSHADERVECTOR oFog;
  D3DSHADERVECTOR oPts;
} VSHADEROUTPUTDATA;

typedef D3DSHADERVECTOR PSHADERCONSTANTS8[D3D_PSHADER_MAX_CONSTANTS];

typedef struct PSHADERDATA {
  /** Run Time Shader Function Constants */
  /*D3DXBUFFER* constants;*/
  PSHADERCONSTANTS8 C;
  /** Shader Code as char ... */
  CONST DWORD* code;
  UINT codeLength;
} PSHADERDATA;

/** temporary here waiting for buffer code */
typedef struct PSHADERINPUTDATA {
  D3DSHADERVECTOR V[2];
  D3DSHADERVECTOR T[8];
  D3DSHADERVECTOR S[16];
  /*D3DSHADERVECTOR R[12];*/
} PSHADERINPUTDATA;

/** temporary here waiting for buffer code */
typedef struct PSHADEROUTPUTDATA {
  D3DSHADERVECTOR oC[4];
  D3DSHADERVECTOR oDepth;
} PSHADEROUTPUTDATA;

#endif
