/*
 * shaders implementation
 *
 * Copyright 2002-2003 Jason Edmeades
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2005 Oliver Stieber
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

#include "config.h"

#include <math.h>
#include <stdio.h>

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_shader);

#define GLINFO_LOCATION ((IWineD3DImpl *)(((IWineD3DDeviceImpl *)This->wineD3DDevice)->wineD3D))->gl_info

/* Shader debugging - Change the following line to enable debugging of software
      vertex shaders                                                             */
#if 0 /* Musxt not be 1 in cvs version */
# define VSTRACE(A) TRACE A
# define TRACE_VSVECTOR(name) TRACE( #name "=(%f, %f, %f, %f)\n", name.x, name.y, name.z, name.w)
#else
# define VSTRACE(A)
# define TRACE_VSVECTOR(name)
#endif

#if 1 /* FIXME : Needs sorting when vshader code moved in properly */

/**
 * DirectX9 SDK download
 *  http://msdn.microsoft.com/library/default.asp?url=/downloads/list/directx.asp
 *
 * Exploring D3DX
 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dndrive/html/directx07162002.asp
 *
 * Using Vertex Shaders
 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dndrive/html/directx02192001.asp
 *
 * Dx9 New
 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/whatsnew.asp
 *
 * Dx9 Shaders
 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/reference/Shaders/VertexShader2_0/VertexShader2_0.asp
 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/reference/Shaders/VertexShader2_0/Instructions/Instructions.asp
 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/programmingguide/GettingStarted/VertexDeclaration/VertexDeclaration.asp
 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/reference/Shaders/VertexShader3_0/VertexShader3_0.asp
 *
 * Dx9 D3DX
 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/programmingguide/advancedtopics/VertexPipe/matrixstack/matrixstack.asp
 *
 * FVF
 *  http://msdn.microsoft.com/library/en-us/directx9_c/directx/graphics/programmingguide/GettingStarted/VertexFormats/vformats.asp
 *
 * NVIDIA: DX8 Vertex Shader to NV Vertex Program
 *  http://developer.nvidia.com/view.asp?IO=vstovp
 *
 * NVIDIA: Memory Management with VAR
 *  http://developer.nvidia.com/view.asp?IO=var_memory_management
 */

/* TODO: Vertex and Pixel shaders are almost identicle, the only exception being the way that some of the data is looked up or the availablity of some of the data i.e. some instructions are only valid for pshaders and some for vshaders
because of this the bulk of the software pipeline can be shared between pixel and vertex shaders... and it wouldn't supprise me if the programes can be cross compiled using a large body body shared code */

#define GLNAME_REQUIRE_GLSL  ((const char *)1)

/*******************************
 * vshader functions software VM
 */

void vshader_add(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
  d->x = s0->x + s1->x;
  d->y = s0->y + s1->y;
  d->z = s0->z + s1->z;
  d->w = s0->w + s1->w;
  VSTRACE(("executing add: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	         s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

void vshader_dp3(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
  d->x = d->y = d->z = d->w = s0->x * s1->x + s0->y * s1->y + s0->z * s1->z;
  VSTRACE(("executing dp3: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	         s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

void vshader_dp4(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
  d->x = d->y = d->z = d->w = s0->x * s1->x + s0->y * s1->y + s0->z * s1->z + s0->w * s1->w;
  VSTRACE(("executing dp4: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	  s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

void vshader_dst(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
  d->x = 1.0f;
  d->y = s0->y * s1->y;
  d->z = s0->z;
  d->w = s1->w;
  VSTRACE(("executing dst: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	  s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

void vshader_expp(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
  union {
    float f;
    DWORD d;
  } tmp;

  tmp.f = floorf(s0->w);
  d->x  = powf(2.0f, tmp.f);
  d->y  = s0->w - tmp.f;
  tmp.f = powf(2.0f, s0->w);
  tmp.d &= 0xFFFFFF00U;
  d->z  = tmp.f;
  d->w  = 1.0f;
  VSTRACE(("executing exp: s0=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
                s0->x, s0->y, s0->z, s0->w, d->x, d->y, d->z, d->w));
}

void vshader_lit(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
  d->x = 1.0f;
  d->y = (0.0f < s0->x) ? s0->x : 0.0f;
  d->z = (0.0f < s0->x && 0.0f < s0->y) ? powf(s0->y, s0->w) : 0.0f;
  d->w = 1.0f;
  VSTRACE(("executing lit: s0=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	         s0->x, s0->y, s0->z, s0->w, d->x, d->y, d->z, d->w));
}

void vshader_logp(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
  float tmp_f = fabsf(s0->w);
  d->x = d->y = d->z = d->w = (0.0f != tmp_f) ? logf(tmp_f) / logf(2.0f) : -HUGE_VAL;
  VSTRACE(("executing logp: s0=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	         s0->x, s0->y, s0->z, s0->w, d->x, d->y, d->z, d->w));
}

void vshader_mad(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1, WINED3DSHADERVECTOR* s2) {
  d->x = s0->x * s1->x + s2->x;
  d->y = s0->y * s1->y + s2->y;
  d->z = s0->z * s1->z + s2->z;
  d->w = s0->w * s1->w + s2->w;
  VSTRACE(("executing mad: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) s2=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	  s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, s2->x, s2->y, s2->z, s2->w, d->x, d->y, d->z, d->w));
}

void vshader_max(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
  d->x = (s0->x >= s1->x) ? s0->x : s1->x;
  d->y = (s0->y >= s1->y) ? s0->y : s1->y;
  d->z = (s0->z >= s1->z) ? s0->z : s1->z;
  d->w = (s0->w >= s1->w) ? s0->w : s1->w;
  VSTRACE(("executing max: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	  s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

void vshader_min(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
  d->x = (s0->x < s1->x) ? s0->x : s1->x;
  d->y = (s0->y < s1->y) ? s0->y : s1->y;
  d->z = (s0->z < s1->z) ? s0->z : s1->z;
  d->w = (s0->w < s1->w) ? s0->w : s1->w;
  VSTRACE(("executing min: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	  s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

void vshader_mov(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
  d->x = s0->x;
  d->y = s0->y;
  d->z = s0->z;
  d->w = s0->w;
  VSTRACE(("executing mov: s0=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	  s0->x, s0->y, s0->z, s0->w, d->x, d->y, d->z, d->w));
}

void vshader_mul(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
  d->x = s0->x * s1->x;
  d->y = s0->y * s1->y;
  d->z = s0->z * s1->z;
  d->w = s0->w * s1->w;
  VSTRACE(("executing mul: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	  s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

void vshader_nop(void) {
    /* NOPPPP ahhh too easy ;) */
    VSTRACE(("executing nop\n"));
}

void vshader_rcp(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
  d->x = d->y = d->z = d->w = (0.0f == s0->w) ? HUGE_VAL : 1.0f / s0->w;
  VSTRACE(("executing rcp: s0=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	  s0->x, s0->y, s0->z, s0->w, d->x, d->y, d->z, d->w));
}

void vshader_rsq(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
  float tmp_f = fabsf(s0->w);
  d->x = d->y = d->z = d->w = (0.0f == tmp_f) ? HUGE_VAL : ((1.0f != tmp_f) ? 1.0f / sqrtf(tmp_f) : 1.0f);
  VSTRACE(("executing rsq: s0=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	  s0->x, s0->y, s0->z, s0->w, d->x, d->y, d->z, d->w));
}

void vshader_sge(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
  d->x = (s0->x >= s1->x) ? 1.0f : 0.0f;
  d->y = (s0->y >= s1->y) ? 1.0f : 0.0f;
  d->z = (s0->z >= s1->z) ? 1.0f : 0.0f;
  d->w = (s0->w >= s1->w) ? 1.0f : 0.0f;
  VSTRACE(("executing sge: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	  s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

void vshader_slt(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
  d->x = (s0->x < s1->x) ? 1.0f : 0.0f;
  d->y = (s0->y < s1->y) ? 1.0f : 0.0f;
  d->z = (s0->z < s1->z) ? 1.0f : 0.0f;
  d->w = (s0->w < s1->w) ? 1.0f : 0.0f;
  VSTRACE(("executing slt: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	  s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

void vshader_sub(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
  d->x = s0->x - s1->x;
  d->y = s0->y - s1->y;
  d->z = s0->z - s1->z;
  d->w = s0->w - s1->w;
  VSTRACE(("executing sub: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	  s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

/**
 * Version 1.1 specific
 */

void vshader_exp(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
  d->x = d->y = d->z = d->w = powf(2.0f, s0->w);
  VSTRACE(("executing exp: s0=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	  s0->x, s0->y, s0->z, s0->w, d->x, d->y, d->z, d->w));
}

void vshader_log(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
  float tmp_f = fabsf(s0->w);
  d->x = d->y = d->z = d->w = (0.0f != tmp_f) ? logf(tmp_f) / logf(2.0f) : -HUGE_VAL;
  VSTRACE(("executing log: s0=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	  s0->x, s0->y, s0->z, s0->w, d->x, d->y, d->z, d->w));
}

void vshader_frc(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
  d->x = s0->x - floorf(s0->x);
  d->y = s0->y - floorf(s0->y);
  d->z = 0.0f;
  d->w = 1.0f;
  VSTRACE(("executing frc: s0=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	  s0->x, s0->y, s0->z, s0->w, d->x, d->y, d->z, d->w));
}

typedef FLOAT D3DMATRIX44[4][4];
typedef FLOAT D3DMATRIX43[4][3];
typedef FLOAT D3DMATRIX34[3][4];
typedef FLOAT D3DMATRIX33[3][3];
typedef FLOAT D3DMATRIX23[2][3];

void vshader_m4x4(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, /*WINED3DSHADERVECTOR* mat1*/ D3DMATRIX44 mat) {
  /*
   * Buggy CODE: here only if cast not work for copy/paste
  WINED3DSHADERVECTOR* mat2 = mat1 + 1;
  WINED3DSHADERVECTOR* mat3 = mat1 + 2;
  WINED3DSHADERVECTOR* mat4 = mat1 + 3;
  d->x = mat1->x * s0->x + mat2->x * s0->y + mat3->x * s0->z + mat4->x * s0->w;
  d->y = mat1->y * s0->x + mat2->y * s0->y + mat3->y * s0->z + mat4->y * s0->w;
  d->z = mat1->z * s0->x + mat2->z * s0->y + mat3->z * s0->z + mat4->z * s0->w;
  d->w = mat1->w * s0->x + mat2->w * s0->y + mat3->w * s0->z + mat4->w * s0->w;
  */
  d->x = mat[0][0] * s0->x + mat[0][1] * s0->y + mat[0][2] * s0->z + mat[0][3] * s0->w;
  d->y = mat[1][0] * s0->x + mat[1][1] * s0->y + mat[1][2] * s0->z + mat[1][3] * s0->w;
  d->z = mat[2][0] * s0->x + mat[2][1] * s0->y + mat[2][2] * s0->z + mat[2][3] * s0->w;
  d->w = mat[3][0] * s0->x + mat[3][1] * s0->y + mat[3][2] * s0->z + mat[3][3] * s0->w;
  VSTRACE(("executing m4x4(1): mat=(%f, %f, %f, %f)    s0=(%f)     d=(%f) \n", mat[0][0], mat[0][1], mat[0][2], mat[0][3], s0->x, d->x));
  VSTRACE(("executing m4x4(2): mat=(%f, %f, %f, %f)       (%f)       (%f) \n", mat[1][0], mat[1][1], mat[1][2], mat[1][3], s0->y, d->y));
  VSTRACE(("executing m4x4(3): mat=(%f, %f, %f, %f) X     (%f)  =    (%f) \n", mat[2][0], mat[2][1], mat[2][2], mat[2][3], s0->z, d->z));
  VSTRACE(("executing m4x4(4): mat=(%f, %f, %f, %f)       (%f)       (%f) \n", mat[3][0], mat[3][1], mat[3][2], mat[3][3], s0->w, d->w));
}

void vshader_m4x3(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, D3DMATRIX34 mat) {
  d->x = mat[0][0] * s0->x + mat[0][1] * s0->y + mat[0][2] * s0->z + mat[0][3] * s0->w;
  d->y = mat[1][0] * s0->x + mat[1][1] * s0->y + mat[1][2] * s0->z + mat[1][3] * s0->w;
  d->z = mat[2][0] * s0->x + mat[2][1] * s0->y + mat[2][2] * s0->z + mat[2][3] * s0->w;
  d->w = 1.0f;
  VSTRACE(("executing m4x3(1): mat=(%f, %f, %f, %f)    s0=(%f)     d=(%f) \n", mat[0][0], mat[0][1], mat[0][2], mat[0][3], s0->x, d->x));
  VSTRACE(("executing m4x3(2): mat=(%f, %f, %f, %f)       (%f)       (%f) \n", mat[1][0], mat[1][1], mat[1][2], mat[1][3], s0->y, d->y));
  VSTRACE(("executing m4x3(3): mat=(%f, %f, %f, %f) X     (%f)  =    (%f) \n", mat[2][0], mat[2][1], mat[2][2], mat[2][3], s0->z, d->z));
  VSTRACE(("executing m4x3(4):                            (%f)       (%f) \n", s0->w, d->w));
}

void vshader_m3x4(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, D3DMATRIX43 mat) {
  d->x = mat[0][0] * s0->x + mat[0][1] * s0->y + mat[0][2] * s0->z;
  d->y = mat[1][0] * s0->x + mat[1][1] * s0->y + mat[1][2] * s0->z;
  d->z = mat[2][0] * s0->x + mat[2][1] * s0->y + mat[2][2] * s0->z;
  d->w = mat[3][0] * s0->x + mat[3][1] * s0->y + mat[3][2] * s0->z;
  VSTRACE(("executing m3x4(1): mat=(%f, %f, %f)    s0=(%f)     d=(%f) \n", mat[0][0], mat[0][1], mat[0][2], s0->x, d->x));
  VSTRACE(("executing m3x4(2): mat=(%f, %f, %f)       (%f)       (%f) \n", mat[1][0], mat[1][1], mat[1][2], s0->y, d->y));
  VSTRACE(("executing m3x4(3): mat=(%f, %f, %f) X     (%f)  =    (%f) \n", mat[2][0], mat[2][1], mat[2][2], s0->z, d->z));
  VSTRACE(("executing m3x4(4): mat=(%f, %f, %f)       (%f)       (%f) \n", mat[3][0], mat[3][1], mat[3][2], s0->w, d->w));
}

void vshader_m3x3(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, D3DMATRIX33 mat) {
  d->x = mat[0][0] * s0->x + mat[0][1] * s0->y + mat[0][2] * s0->z;
  d->y = mat[1][0] * s0->x + mat[1][1] * s0->y + mat[1][2] * s0->z;
  d->z = mat[2][0] * s0->x + mat[2][1] * s0->y + mat[2][2] * s0->z;
  d->w = 1.0f;
  VSTRACE(("executing m3x3(1): mat=(%f, %f, %f)    s0=(%f)     d=(%f) \n", mat[0][0], mat[0][1], mat[0][2], s0->x, d->x));
  VSTRACE(("executing m3x3(2): mat=(%f, %f, %f)       (%f)       (%f) \n", mat[1][0], mat[1][1], mat[1][2], s0->y, d->y));
  VSTRACE(("executing m3x3(3): mat=(%f, %f, %f) X     (%f)  =    (%f) \n", mat[2][0], mat[2][1], mat[2][2], s0->z, d->z));
  VSTRACE(("executing m3x3(4):                                       (%f) \n", d->w));
}

void vshader_m3x2(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, D3DMATRIX23 mat) {
  FIXME("check\n");
  d->x = mat[0][0] * s0->x + mat[0][1] * s0->y + mat[0][2] * s0->z;
  d->y = mat[1][0] * s0->x + mat[1][1] * s0->y + mat[1][2] * s0->z;
  d->z = 0.0f;
  d->w = 1.0f;
}

/**
 * Version 2.0 specific
 */
void vshader_lrp(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1, WINED3DSHADERVECTOR* s2) {
  d->x = s0->x * (s1->x - s2->x) + s2->x;
  d->y = s0->y * (s1->y - s2->y) + s2->y;
  d->z = s0->z * (s1->z - s2->z) + s2->z;
  d->w = s0->w * (s1->w - s2->w) + s2->w;
}

void vshader_crs(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
  d->x = s0->y * s1->z - s0->z * s1->y;
  d->y = s0->z * s1->x - s0->x * s1->z;
  d->z = s0->x * s1->y - s0->y * s1->x;
  d->w = 0.9f; /* w is undefined, so set it to something safeish */

  VSTRACE(("executing crs: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
             s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

void vshader_abs(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {

  d->x = fabsf(s0->x);
  d->y = fabsf(s0->y);
  d->z = fabsf(s0->z);
  d->w = fabsf(s0->w);
  VSTRACE(("executing abs: s0=(%f, %f, %f, %f)  => d=(%f, %f, %f, %f)\n",
             s0->x, s0->y, s0->z, s0->w, d->x, d->y, d->z, d->w));
}

    /* Stubs */

/* Def is C[n] = {n.nf, n.nf, n.nf, n.nf} */
void vshader_def(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1, WINED3DSHADERVECTOR* s2, WINED3DSHADERVECTOR* s3) {
    FIXME(" : Stub\n");
}

void vshader_call(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}

void vshader_callnz(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

void vshader_loop(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

void vshader_ret(void) {
    FIXME(" : Stub\n");
}

void vshader_endloop(void) {
    FIXME(" : Stub\n");
}

void vshader_dcl(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

void vshader_pow(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
    FIXME(" : Stub\n");
}

void vshader_sng(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

void vshader_nrm(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

void vshader_sincos(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

void vshader_rep(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}

void vshader_endrep(void) {
    FIXME(" : Stub\n");
}

void vshader_if(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}

void vshader_ifc(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

void vshader_else(void) {
    FIXME(" : Stub\n");
}

void vshader_label(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}

void vshader_endif(void) {
    FIXME(" : Stub\n");
}

void vshader_break(void) {
    FIXME(" : Stub\n");
}

void vshader_breakc(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

void vshader_breakp(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}

void vshader_mova(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}

void vshader_defb(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}

void vshader_defi(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}

void vshader_texldd(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}

void vshader_setp(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}

void vshader_texldl(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}

/**
 * log, exp, frc, m*x* seems to be macros ins ... to see
 */
CONST SHADER_OPCODE IWineD3DVertexShaderImpl_shader_ins[] = {
    {D3DSIO_NOP,  "nop", "NOP", 0, vshader_nop, 0, 0},
    {D3DSIO_MOV,  "mov", "MOV", 2, vshader_mov, 0, 0},
    {D3DSIO_ADD,  "add", "ADD", 3, vshader_add, 0, 0},
    {D3DSIO_SUB,  "sub", "SUB", 3, vshader_sub, 0, 0},
    {D3DSIO_MAD,  "mad", "MAD", 4, vshader_mad, 0, 0},
    {D3DSIO_MUL,  "mul", "MUL", 3, vshader_mul, 0, 0},
    {D3DSIO_RCP,  "rcp", "RCP",  2, vshader_rcp, 0, 0},
    {D3DSIO_RSQ,  "rsq",  "RSQ", 2, vshader_rsq, 0, 0},
    {D3DSIO_DP3,  "dp3",  "DP3", 3, vshader_dp3, 0, 0},
    {D3DSIO_DP4,  "dp4",  "DP4", 3, vshader_dp4, 0, 0},
    {D3DSIO_MIN,  "min",  "MIN", 3, vshader_min, 0, 0},
    {D3DSIO_MAX,  "max",  "MAX", 3, vshader_max, 0, 0},
    {D3DSIO_SLT,  "slt",  "SLT", 3, vshader_slt, 0, 0},
    {D3DSIO_SGE,  "sge",  "SGE", 3, vshader_sge, 0, 0},
    {D3DSIO_ABS,  "abs",  "ABS", 2, vshader_abs, 0, 0},
    {D3DSIO_EXP,  "exp",  "EX2", 2, vshader_exp, 0, 0},
    {D3DSIO_LOG,  "log",  "LG2", 2, vshader_log, 0, 0},
    {D3DSIO_LIT,  "lit",  "LIT", 2, vshader_lit, 0, 0},
    {D3DSIO_DST,  "dst",  "DST", 3, vshader_dst, 0, 0},
    {D3DSIO_LRP,  "lrp",  "LRP", 4, vshader_lrp, 0, 0},
    {D3DSIO_FRC,  "frc",  "FRC", 2, vshader_frc, 0, 0},
    {D3DSIO_M4x4, "m4x4", "undefined", 3, vshader_m4x4, 0, 0},
    {D3DSIO_M4x3, "m4x3", "undefined", 3, vshader_m4x3, 0, 0},
    {D3DSIO_M3x4, "m3x4", "undefined", 3, vshader_m3x4, 0, 0},
    {D3DSIO_M3x3, "m3x3", "undefined", 3, vshader_m3x3, 0, 0},
    {D3DSIO_M3x2, "m3x2", "undefined", 3, vshader_m3x2, 0, 0},

    /** FIXME: use direct access so add the others opcodes as stubs */
    /* DCL is a specil operation */
    {D3DSIO_DCL,      "dcl",      NULL,   2, vshader_dcl,     0, 0},
    {D3DSIO_POW,      "pow",      "POW",  3, vshader_pow,     0, 0},
    {D3DSIO_CRS,      "crs",      "XPS",  3, vshader_crs,     0, 0},
    /* TODO: sng can possibly be performed as
        RCP tmp, vec
        MUL out, tmp, vec*/
    {D3DSIO_SGN,      "sng",      NULL,   2, vshader_sng,     0, 0},
    /* TODO: xyz normalise can be performed as VS_ARB using one temporary register,
        DP3 tmp , vec, vec;
        RSQ tmp, tmp.x;
        MUL vec.xyz, vec, tmp;
    but I think this is better because it accounts for w properly.
        DP3 tmp , vec, vec;
        RSQ tmp, tmp.x;
        MUL vec, vec, tmp;
    
    */
    {D3DSIO_NRM,      "nrm",      NULL,   2, vshader_nrm,     0, 0},
    {D3DSIO_SINCOS,   "sincos",   NULL,   2, vshader_sincos,  0, 0},

    /* Flow control - requires GLSL or software shaders */
    {D3DSIO_REP ,     "rep",      GLNAME_REQUIRE_GLSL,   1, vshader_rep,     0, 0},
    {D3DSIO_ENDREP,   "endrep",   GLNAME_REQUIRE_GLSL,   0, vshader_endrep,  0, 0},
    {D3DSIO_IF,       "if",       GLNAME_REQUIRE_GLSL,   1, vshader_if,      0, 0},
    {D3DSIO_IFC,      "ifc",      GLNAME_REQUIRE_GLSL,   2, vshader_ifc,     0, 0},
    {D3DSIO_ELSE,     "else",     GLNAME_REQUIRE_GLSL,   0, vshader_else,    0, 0},
    {D3DSIO_ENDIF,    "endif",    GLNAME_REQUIRE_GLSL,   0, vshader_endif,   0, 0},
    {D3DSIO_BREAK,    "break",    GLNAME_REQUIRE_GLSL,   0, vshader_break,   0, 0},
    {D3DSIO_BREAKC,   "breakc",   GLNAME_REQUIRE_GLSL,   2, vshader_breakc,  0, 0},
    {D3DSIO_BREAKP,   "breakp",   GLNAME_REQUIRE_GLSL,   1, vshader_breakp,  0, 0},
    {D3DSIO_CALL,     "call",     GLNAME_REQUIRE_GLSL,   1, vshader_call,    0, 0},
    {D3DSIO_CALLNZ,   "callnz",   GLNAME_REQUIRE_GLSL,   2, vshader_callnz,  0, 0},
    {D3DSIO_LOOP,     "loop",     GLNAME_REQUIRE_GLSL,   2, vshader_loop,    0, 0},
    {D3DSIO_RET,      "ret",      GLNAME_REQUIRE_GLSL,   0, vshader_ret,     0, 0},
    {D3DSIO_ENDLOOP,  "endloop",  GLNAME_REQUIRE_GLSL,   0, vshader_endloop, 0, 0},
    {D3DSIO_LABEL,    "label",    GLNAME_REQUIRE_GLSL,   1, vshader_label,   0, 0},

    {D3DSIO_MOVA,     "mova",     GLNAME_REQUIRE_GLSL,   2, vshader_mova,    0, 0},
    {D3DSIO_DEFB,     "defb",     GLNAME_REQUIRE_GLSL,   2, vshader_defb,    0, 0},
    {D3DSIO_DEFI,     "defi",     GLNAME_REQUIRE_GLSL,   2, vshader_defi,    0, 0},
    {D3DSIO_EXPP,     "expp",     "EXP", 2, vshader_expp, 0, 0},
    {D3DSIO_LOGP,     "logp",     "LOG", 2, vshader_logp, 0, 0},
    /* def is a special operation */
    {D3DSIO_DEF,      "def",      NULL,   5, vshader_def, 0, 0},
    {D3DSIO_SETP,     "setp",     GLNAME_REQUIRE_GLSL,   2, vshader_setp,    0, 0},
    {D3DSIO_TEXLDL,   "texdl",    GLNAME_REQUIRE_GLSL,   2, vshader_texldl,  0, 0},
    {0,               NULL,       NULL,   0, NULL,            0, 0}
};

inline static void vshader_program_dump_vs_param(const DWORD param, int input) {
  static const char* rastout_reg_names[] = { "oPos", "oFog", "oPts" };
  static const char swizzle_reg_chars[] = "xyzw";

  DWORD reg = param & D3DSP_REGNUM_MASK; 
  DWORD regtype = shader_get_regtype(param);

  if ((param & D3DSP_SRCMOD_MASK) == D3DSPSM_NEG) TRACE("-");

  switch (regtype) {
  case D3DSPR_TEMP:
    TRACE("r%lu", reg);
    break;
  case D3DSPR_INPUT:
    TRACE("v%lu", reg);
    break;
  case D3DSPR_CONST:
    TRACE("c%s%lu", (param & D3DVS_ADDRMODE_RELATIVE) ? "a0.x + " : "", reg);
    break;
  case D3DSPR_ADDR: /*case D3DSPR_TEXTURE:*/
    TRACE("a%lu", reg);
    break;
  case D3DSPR_RASTOUT:
    TRACE("%s", rastout_reg_names[reg]);
    break;
  case D3DSPR_ATTROUT:
    TRACE("oD%lu", reg);
    break;
  case D3DSPR_TEXCRDOUT:
    TRACE("oT%lu", reg);
    break;
  case D3DSPR_CONSTINT:
    TRACE("i%s%lu", (param & D3DVS_ADDRMODE_RELATIVE) ? "a0.x + " : "", reg);
    break;
  case D3DSPR_CONSTBOOL:
    TRACE("b%s%lu", (param & D3DVS_ADDRMODE_RELATIVE) ? "a0.x + " : "", reg);
    break;
  case D3DSPR_LABEL:
    TRACE("l%lu", reg);
    break;
  case D3DSPR_LOOP:
    TRACE("aL%s%lu", (param & D3DVS_ADDRMODE_RELATIVE) ? "a0.x + " : "", reg);
    break;
  case D3DSPR_SAMPLER:
    TRACE("s%lu", reg);
    break;
  default:
    FIXME("Unknown %lu reg %lu\n",regtype, reg);
    break;
  }

  if (!input) {
    /** operand output */
    if ((param & D3DSP_WRITEMASK_ALL) != D3DSP_WRITEMASK_ALL) {
      if (param & D3DSP_WRITEMASK_0) TRACE(".x");
      if (param & D3DSP_WRITEMASK_1) TRACE(".y");
      if (param & D3DSP_WRITEMASK_2) TRACE(".z");
      if (param & D3DSP_WRITEMASK_3) TRACE(".w");
    }
  } else {
    /** operand input */
    DWORD swizzle = (param & D3DVS_SWIZZLE_MASK) >> D3DVS_SWIZZLE_SHIFT;
    DWORD swizzle_x = swizzle & 0x03;
    DWORD swizzle_y = (swizzle >> 2) & 0x03;
    DWORD swizzle_z = (swizzle >> 4) & 0x03;
    DWORD swizzle_w = (swizzle >> 6) & 0x03;
    /**
     * swizzle bits fields:
     *  WWZZYYXX
     */
    if ((D3DVS_NOSWIZZLE >> D3DVS_SWIZZLE_SHIFT) != swizzle) { /* ! D3DVS_NOSWIZZLE == 0xE4 << D3DVS_SWIZZLE_SHIFT */
      if (swizzle_x == swizzle_y &&
        swizzle_x == swizzle_z &&
        swizzle_x == swizzle_w) {
            TRACE(".%c", swizzle_reg_chars[swizzle_x]);
      } else {
        TRACE(".%c%c%c%c",
        swizzle_reg_chars[swizzle_x],
        swizzle_reg_chars[swizzle_y],
        swizzle_reg_chars[swizzle_z],
        swizzle_reg_chars[swizzle_w]);
      }
    }
  }
}

inline static void vshader_program_dump_decl_usage(
    IWineD3DVertexShaderImpl *This, DWORD decl, DWORD param) {
    DWORD regtype = shader_get_regtype(param);

    TRACE("dcl_");

    if (regtype == D3DSPR_SAMPLER) {
        DWORD ttype = decl & D3DSP_TEXTURETYPE_MASK;

        switch (ttype) {
            case D3DSTT_2D: TRACE("2d "); break;
            case D3DSTT_CUBE: TRACE("cube "); break;
            case D3DSTT_VOLUME: TRACE("volume "); break;
            default: TRACE("unknown_ttype(%08lx) ", ttype);
       }

    } else {

        DWORD usage = decl & D3DSP_DCL_USAGE_MASK;
        DWORD idx = (decl & D3DSP_DCL_USAGEINDEX_MASK) >> D3DSP_DCL_USAGEINDEX_SHIFT;

        switch(usage) {
        case D3DDECLUSAGE_POSITION:
            TRACE("%s%ld ", "position", idx);
            break;
        case D3DDECLUSAGE_BLENDINDICES:
            TRACE("%s ", "blend");
            break;
        case D3DDECLUSAGE_BLENDWEIGHT:
            TRACE("%s ", "weight");
            break;
        case D3DDECLUSAGE_NORMAL:
            TRACE("%s%ld ", "normal", idx);
            break;
        case D3DDECLUSAGE_PSIZE:
            TRACE("%s ", "psize");
            break;
        case D3DDECLUSAGE_COLOR:
            if(idx == 0)  {
                TRACE("%s ", "color");
            } else {
                TRACE("%s ", "specular");
            }
            break;
        case D3DDECLUSAGE_TEXCOORD:
            TRACE("%s%ld ", "texture", idx);
            break;
        case D3DDECLUSAGE_TANGENT:
            TRACE("%s ", "tangent");
            break;
        case D3DDECLUSAGE_BINORMAL:
            TRACE("%s ", "binormal");
            break;
        case D3DDECLUSAGE_TESSFACTOR:
            TRACE("%s ", "tessfactor");
            break;
        case D3DDECLUSAGE_POSITIONT:
            TRACE("%s%ld ", "positionT", idx);
            break;
        case D3DDECLUSAGE_FOG:
            TRACE("%s ", "fog");
            break;
        case D3DDECLUSAGE_DEPTH:
            TRACE("%s ", "depth");
            break;
        case D3DDECLUSAGE_SAMPLE:
            TRACE("%s ", "sample");
            break;
        default:
            FIXME("Unrecognised dcl %08lx", usage);
        }
    }
}

inline static BOOL vshader_is_version_token(DWORD token) {
  return 0xFFFE0000 == (token & 0xFFFE0000);
}

inline static BOOL vshader_is_comment_token(DWORD token) {
  return D3DSIO_COMMENT == (token & D3DSI_OPCODE_MASK);
}

inline static void vshader_program_add_output_param_swizzle(const DWORD param, int is_color, char *hwLine) {
    /** operand output */
    if ((param & D3DSP_WRITEMASK_ALL) != D3DSP_WRITEMASK_ALL) {
      strcat(hwLine, ".");
      if (param & D3DSP_WRITEMASK_0) { strcat(hwLine, "x"); }
      if (param & D3DSP_WRITEMASK_1) { strcat(hwLine, "y"); }
      if (param & D3DSP_WRITEMASK_2) { strcat(hwLine, "z"); }
      if (param & D3DSP_WRITEMASK_3) { strcat(hwLine, "w"); }
    }  
}

inline static void vshader_program_add_input_param_swizzle(const DWORD param, int is_color, char *hwLine) {
    static const char swizzle_reg_chars_color_fix[] = "zyxw";
    static const char swizzle_reg_chars[] = "xyzw";
    const char* swizzle_regs = NULL;
    char  tmpReg[255];

    /** operand input */
    DWORD swizzle = (param & D3DVS_SWIZZLE_MASK) >> D3DVS_SWIZZLE_SHIFT;
    DWORD swizzle_x = swizzle & 0x03;
    DWORD swizzle_y = (swizzle >> 2) & 0x03;
    DWORD swizzle_z = (swizzle >> 4) & 0x03;
    DWORD swizzle_w = (swizzle >> 6) & 0x03;

    if (is_color) {
      swizzle_regs = swizzle_reg_chars_color_fix;
    } else {
      swizzle_regs = swizzle_reg_chars;
    }

    /**
     * swizzle bits fields:
     *  WWZZYYXX
     */
    if ((D3DVS_NOSWIZZLE >> D3DVS_SWIZZLE_SHIFT) == swizzle) { /* D3DVS_NOSWIZZLE == 0xE4 << D3DVS_SWIZZLE_SHIFT */
      if (is_color) {
	sprintf(tmpReg, ".%c%c%c%c",
		swizzle_regs[swizzle_x],
		swizzle_regs[swizzle_y],
		swizzle_regs[swizzle_z],
		swizzle_regs[swizzle_w]);
	strcat(hwLine, tmpReg);
      }
      return ;
    }
    if (swizzle_x == swizzle_y &&
	swizzle_x == swizzle_z &&
	swizzle_x == swizzle_w)
    {
      sprintf(tmpReg, ".%c", swizzle_regs[swizzle_x]);
      strcat(hwLine, tmpReg);
    } else {
      sprintf(tmpReg, ".%c%c%c%c",
	      swizzle_regs[swizzle_x],
	      swizzle_regs[swizzle_y],
	      swizzle_regs[swizzle_z],
	      swizzle_regs[swizzle_w]);
      strcat(hwLine, tmpReg);
    }
}

inline static void vshader_program_add_param(IWineD3DVertexShaderImpl *This, const DWORD param, BOOL is_input, char *hwLine) {
  /* oPos, oFog and oPts in D3D */
  static const char* hwrastout_reg_names[] = { "result.position", "result.fogcoord", "result.pointsize" };

  DWORD reg = param & D3DSP_REGNUM_MASK;
  DWORD regtype = shader_get_regtype(param);
  char  tmpReg[255];
  BOOL is_color = FALSE;

  if ((param & D3DSP_SRCMOD_MASK) == D3DSPSM_NEG) {
      strcat(hwLine, " -");
  } else {
      strcat(hwLine, " ");
  }

  switch (regtype) {
  case D3DSPR_TEMP:
    sprintf(tmpReg, "T%lu", reg);
    strcat(hwLine, tmpReg);
    break;
  case D3DSPR_INPUT:
    if (reg == This->arrayUsageMap[WINED3DSHADERDECLUSAGE_DIFFUSE]
        || reg == This->arrayUsageMap[WINED3DSHADERDECLUSAGE_SPECULAR]) {
        is_color = TRUE;
    }
    /* if the attributes come in as named dcl's then use a named vertex (called namedVertexN) */
    if (This->namedArrays) {
        sprintf(tmpReg, "namedVertex%lu", reg);
    } else {
    /* otherwise the input is on a numbered attribute so use opengl numbered attributes */
        sprintf(tmpReg, "vertex.attrib[%lu]", reg);
    }
    strcat(hwLine, tmpReg);
    break;
  case D3DSPR_CONST:
    /* FIXME: some constants are named so we need a constants map*/
    if (This->constantsUsedBitmap[reg] == VS_CONSTANT_CONSTANT) {
        if (param & D3DVS_ADDRMODE_RELATIVE) {
            FIXME("Relative addressing not expected for a named constant %lu\n", reg);
        }
        sprintf(tmpReg, "const%lu", reg);
    } else {
        sprintf(tmpReg, "C[%s%lu]", (param & D3DVS_ADDRMODE_RELATIVE) ? "A0.x + " : "", reg);
    }
    strcat(hwLine, tmpReg);
    break;
  case D3DSPR_ADDR: /*case D3DSPR_TEXTURE:*/
    sprintf(tmpReg, "A%lu", reg);
    strcat(hwLine, tmpReg);
    break;
  case D3DSPR_RASTOUT:
    sprintf(tmpReg, "%s", hwrastout_reg_names[reg]);
    strcat(hwLine, tmpReg);
    break;
  case D3DSPR_ATTROUT:
    if (reg==0) {
       strcat(hwLine, "result.color.primary");
    } else {
       strcat(hwLine, "result.color.secondary");
    }
    break;
  case D3DSPR_TEXCRDOUT:
    sprintf(tmpReg, "result.texcoord[%lu]", reg);
    strcat(hwLine, tmpReg);
    break;
  default:
    FIXME("Unknown reg type %ld %ld\n", regtype, reg);
    break;
  }

  if (!is_input) {
    vshader_program_add_output_param_swizzle(param, is_color, hwLine);
  } else {
    vshader_program_add_input_param_swizzle(param, is_color, hwLine);
  }
}

DWORD MacroExpansion[4*4];

int ExpandMxMacro(DWORD macro_opcode, const DWORD* args) {
 
  int i;
  int nComponents = 0;
  DWORD opcode =0;
  switch(macro_opcode) {
    case D3DSIO_M4x4:
      nComponents = 4;
      opcode = D3DSIO_DP4;
      break;
    case D3DSIO_M4x3:
      nComponents = 3;
      opcode = D3DSIO_DP4;
      break;
    case D3DSIO_M3x4:
      nComponents = 4;
      opcode = D3DSIO_DP3;
      break;
    case D3DSIO_M3x3:
      nComponents = 3;
      opcode = D3DSIO_DP3;
      break;
    case D3DSIO_M3x2:
      nComponents = 2;
      opcode = D3DSIO_DP3;
      break;
    default:
      break;
  }
  for (i = 0; i < nComponents; i++) {
    MacroExpansion[i*4+0] = opcode;
    MacroExpansion[i*4+1] = ((*args) & ~D3DSP_WRITEMASK_ALL)|(D3DSP_WRITEMASK_0<<i);
    MacroExpansion[i*4+2] = *(args+1);
    MacroExpansion[i*4+3] = (*(args+2))+i;
  }
  return nComponents;
}

static void parse_decl_usage(IWineD3DVertexShaderImpl *This, INT usage, INT arrayNo)
{
    switch(usage & 0xFFFF) {
        case D3DDECLUSAGE_POSITION:
            if((usage & 0xF0000) >> 16 == 0) { /* tween data */
                TRACE("Setting position to %d\n", arrayNo);
                This->arrayUsageMap[WINED3DSHADERDECLUSAGE_POSITION]     = arrayNo;
                This->namedArrays = TRUE;
            } else {
                /* TODO: position indexes go from 0-8!!*/
                TRACE("Setting position 2 to %d because usage = %d\n", arrayNo, (usage & 0xF0000) >> 16);
                /* robots uses positions up to 8, the position arrays are just packed.*/
                if ((usage & 0xF0000) >> 16 > 1) {
                    TRACE("Loaded for position %d (greater than 2)\n", (usage & 0xF0000) >> 16);
                }
                This->arrayUsageMap[WINED3DSHADERDECLUSAGE_POSITION2 + ((usage & 0xF0000) >> 16) -1] = arrayNo;
                This->declaredArrays = TRUE;
            }
        break;
        case D3DDECLUSAGE_BLENDINDICES:
            /* not supported by openGL */
            TRACE("Setting BLENDINDICES to %d\n", arrayNo);
            This->arrayUsageMap[WINED3DSHADERDECLUSAGE_BLENDINDICES] = arrayNo;
            This->declaredArrays = TRUE;
            if ((usage & 0xF0000) >> 16 != 0) FIXME("Extended BLENDINDICES\n");
        break;
        case D3DDECLUSAGE_BLENDWEIGHT:
            TRACE("Setting BLENDWEIGHT to %d\n", arrayNo);
            This->arrayUsageMap[WINED3DSHADERDECLUSAGE_BLENDWEIGHT]  = arrayNo;
            This->namedArrays = TRUE;
            if ((usage & 0xF0000) >> 16 != 0) FIXME("Extended blend weights\n");
        break;
        case D3DDECLUSAGE_NORMAL:
            if((usage & 0xF0000) >> 16 == 0) { /* tween data */
                TRACE("Setting normal to %d\n", arrayNo);
                This->arrayUsageMap[WINED3DSHADERDECLUSAGE_NORMAL]   = arrayNo;
                This->namedArrays = TRUE;
            } else {
                TRACE("Setting normal 2 to %d because usage = %d\n", arrayNo, (usage & 0xF0000) >> 16);
                This->arrayUsageMap[WINED3DSHADERDECLUSAGE_NORMAL2]   = arrayNo;
                This->declaredArrays = TRUE;
            }
        break;
        case D3DDECLUSAGE_PSIZE:
            TRACE("Setting PSIZE to %d\n", arrayNo);
            This->arrayUsageMap[WINED3DSHADERDECLUSAGE_PSIZE]        = arrayNo;
            This->namedArrays = TRUE;
            if ((usage & 0xF0000) >> 16 != 0) FIXME("Extended PSIZE\n");
        break;
        case D3DDECLUSAGE_COLOR:
            if((usage & 0xF0000) >> 16 == 0)  {
                TRACE("Setting DIFFUSE to %d\n", arrayNo);
                This->arrayUsageMap[WINED3DSHADERDECLUSAGE_DIFFUSE]  = arrayNo;
                This->namedArrays = TRUE;
            } else {
                TRACE("Setting SPECULAR to %d\n", arrayNo);
                This->arrayUsageMap[WINED3DSHADERDECLUSAGE_SPECULAR] = arrayNo;
                This->namedArrays = TRUE;
            }
        break;
        case D3DDECLUSAGE_TEXCOORD:
            This->namedArrays = TRUE;
            /* only 7 texture coords have been designed for, so run a quick sanity check */
            if ((usage & 0xF0000) >> 16 > 7) {
                FIXME("(%p) : Program uses texture coordinate %d but only 0-7 have been implemented\n", This, (usage & 0xF0000) >> 16);
            } else {
                TRACE("Setting TEXCOORD %d  to %d\n", ((usage & 0xF0000) >> 16), arrayNo);
                This->arrayUsageMap[WINED3DSHADERDECLUSAGE_TEXCOORD0 + ((usage & 0xF0000) >> 16)] = arrayNo;
            }
        break;
        /* The following aren't supported by openGL,
            if we get them then everything needs to be mapped to numbered attributes instead of named ones.
            this should be caught in the first pass */
        case D3DDECLUSAGE_TANGENT:
            TRACE("Setting TANGENT to %d\n", arrayNo);
            This->arrayUsageMap[WINED3DSHADERDECLUSAGE_TANGENT]      = arrayNo;
            This->declaredArrays = TRUE;
        break;
        case D3DDECLUSAGE_BINORMAL:
            TRACE("Setting BINORMAL to %d\n", arrayNo);
            This->arrayUsageMap[WINED3DSHADERDECLUSAGE_BINORMAL]     = arrayNo;
            This->declaredArrays = TRUE;
        break;
        case D3DDECLUSAGE_TESSFACTOR:
            TRACE("Setting TESSFACTOR to %d\n", arrayNo);
            This->arrayUsageMap[WINED3DSHADERDECLUSAGE_TESSFACTOR]   = arrayNo;
            This->declaredArrays = TRUE;
        break;
        case D3DDECLUSAGE_POSITIONT:
            if((usage & 0xF0000) >> 16 == 0) { /* tween data */
                FIXME("Setting positiont to %d\n", arrayNo);
                This->arrayUsageMap[WINED3DSHADERDECLUSAGE_POSITIONT] = arrayNo;
                This->namedArrays = TRUE;
            } else {
                FIXME("Setting positiont 2 to %d because usage = %d\n", arrayNo, (usage & 0xF0000) >> 16);
                This->arrayUsageMap[WINED3DSHADERDECLUSAGE_POSITIONT2] = arrayNo;
                This->declaredArrays = TRUE;
            if ((usage & 0xF0000) >> 16 != 0) FIXME("Extended positiont\n");
            }
        break;
        case D3DDECLUSAGE_FOG:
            /* supported by OpenGL */
            TRACE("Setting FOG to %d\n", arrayNo);
            This->arrayUsageMap[WINED3DSHADERDECLUSAGE_FOG]          = arrayNo;
            This->namedArrays = TRUE;
        break;
        case D3DDECLUSAGE_DEPTH:
            TRACE("Setting DEPTH to %d\n", arrayNo);
            This->arrayUsageMap[WINED3DSHADERDECLUSAGE_DEPTH]        = arrayNo;
            This->declaredArrays = TRUE;
        break;
        case D3DDECLUSAGE_SAMPLE:
            TRACE("Setting SAMPLE to %d\n", arrayNo);
            This->arrayUsageMap[WINED3DSHADERDECLUSAGE_SAMPLE]       = arrayNo;
            This->declaredArrays = TRUE;
        break;
        default:
        FIXME("Unrecognised dcl %08x", usage & 0xFFFF);
    }
}

void vshader_set_version(
      IWineD3DVertexShaderImpl *This,
      DWORD version) {

      DWORD major = (version >> 8) & 0x0F;
      DWORD minor = version & 0x0F;

      This->baseShader.hex_version = version;
      This->baseShader.version = major * 10 + minor;
      TRACE("vs_%lu_%lu\n", major, minor);

      This->baseShader.limits.texture = 0;

      /* Must match D3DCAPS9.MaxVertexShaderConst: at least 256 for vs_2_0 */
      This->baseShader.limits.constant_float = WINED3D_VSHADER_MAX_CONSTANTS;

      switch (This->baseShader.version) {
          case 10:
          case 11: This->baseShader.limits.temporary = 12;
                   This->baseShader.limits.constant_bool = 0;
                   This->baseShader.limits.constant_int = 0;
                   This->baseShader.limits.address = 1;
                   break;
      
          case 20:
          case 21: This->baseShader.limits.temporary = 12;
                   This->baseShader.limits.constant_bool = 16;
                   This->baseShader.limits.constant_int = 16;
                   This->baseShader.limits.address = 1;
                   break;

          case 30: This->baseShader.limits.temporary = 32;
                   This->baseShader.limits.constant_bool = 32;
                   This->baseShader.limits.constant_int = 32;
                   This->baseShader.limits.address = 1;
                   break;

          default: This->baseShader.limits.temporary = 12;
                   This->baseShader.limits.constant_bool = 0;
                   This->baseShader.limits.constant_int = 0;
                   This->baseShader.limits.address = 1;
                   FIXME("Unrecognized vertex shader version %lu!\n", version);
      }
}

/**
 * Function parser ...
 */

inline static VOID IWineD3DVertexShaderImpl_GenerateProgramArbHW(IWineD3DVertexShader *iface, CONST DWORD* pFunction) {
    IWineD3DVertexShaderImpl *This = (IWineD3DVertexShaderImpl *)iface;
    const DWORD* pToken = pFunction;
    const DWORD* pSavedToken = NULL;
    const SHADER_OPCODE* curOpcode = NULL;
    int nRemInstr = -1;
    DWORD i;
    SHADER_BUFFER buffer;
    char  tmpLine[255];
    DWORD addressUsed, tempsUsed;

#if 0 /* FIXME: Use the buffer that is held by the device, this is ok since fixups will be skipped for software shaders
        it also requires entering a critical section but cuts down the runtime footprint of wined3d and any memory fragmentation that may occur... */
    if (This->device->fixupVertexBufferSize < SHADER_PGMSIZE) {
        HeapFree(GetProcessHeap(), 0, This->fixupVertexBuffer);
        This->fixupVertexBuffer = HeapAlloc(GetProcessHeap() , 0, SHADER_PGMSIZE);
        This->fixupVertexBufferSize = PGMSIZE;
        This->fixupVertexBuffer[0] = 0;
    }
    buffer.buffer = This->device->fixupVertexBuffer;
#endif
    buffer.buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, SHADER_PGMSIZE); 
    buffer.bsize = 0;
    buffer.lineNo = 0;

    /* First pass: figure out which temporary and texture registers are used */
    shader_get_registers_used((IWineD3DBaseShader*) This, pToken, &tempsUsed, &addressUsed);
    TRACE("Address registers used: %#lx, Temp registers used %#lx\n", addressUsed, tempsUsed);

    /* TODO: check register usage against GL/Directx limits, and fail if they're exceeded
        nUseAddressRegister < = GL_MAX_PROGRAM_ADDRESS_REGISTERS_AR
        nUseTempRegister    <=  GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB
    */

    /* Mesa supports only 95 constants */
    if (GL_VEND(MESA) || GL_VEND(WINE))
        This->baseShader.limits.constant_float = 
           min(95, This->baseShader.limits.constant_float);

    /* FIXME: if jumps are used, use GLSL, else use ARB_vertex_program */
    shader_addline(&buffer, "!!ARBvp1.0\n");

    /* Pre-declare registers */
    for (i = 0; i < This->baseShader.limits.temporary; i++) {
        if (tempsUsed & (1 << i))
            shader_addline(&buffer, "TEMP T%lu;\n", i);
    }

    for (i = 0; i < This->baseShader.limits.address; i++) {
        if (addressUsed & (1 << i))
            shader_addline(&buffer, "ADDRESS A%ld;\n", i);
    }

    /* Why do we need to alias those? */
    shader_addline(&buffer, "PARAM C[%d] = { program.env[0..%d] };\n",
        This->baseShader.limits.constant_float, 
        This->baseShader.limits.constant_float - 1);            

  /** second pass, now generate */
  pToken = pFunction;

  if (NULL != pToken) {
    while (1) {
      tmpLine[0] = 0;
      if ((nRemInstr >= 0) && (--nRemInstr == -1))
            /* Macro is finished, continue normal path */ 
            pToken = pSavedToken;
      if (D3DVS_END() == *pToken)
            break;

      /* Skip version */
      if (vshader_is_version_token(*pToken)) {
            ++pToken;
            continue;
      }

      /* Skip comments */
      if (vshader_is_comment_token(*pToken)) {
            DWORD comment_len = (*pToken & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;
            ++pToken;
            TRACE("#%s\n", (char*)pToken);
            pToken += comment_len;
            continue;
      }

      curOpcode = shader_get_opcode((IWineD3DBaseShader*) This, *pToken);
      ++pToken;
      if (NULL == curOpcode) {
        /* unknown current opcode ... (shouldn't be any!) */
        while (*pToken & 0x80000000) {
            FIXME("unrecognized opcode: %08lx\n", *pToken);
            ++pToken;
        }
      } else if (GLNAME_REQUIRE_GLSL == curOpcode->glname) {
            /* if the token isn't supported by this cross compiler then skip it and its parameters */
          
            FIXME("Token %s requires greater functionality than Vertex_Progarm_ARB supports\n", curOpcode->name);
            pToken += curOpcode->num_params;

      } else if (D3DSIO_DEF == curOpcode->opcode) {

            /* Handle definitions here, they don't fit well with the
             * other instructions below [for now ] */

            shader_addline(&buffer, 
                "PARAM const%lu = { %f, %f, %f, %f };\n", *pToken & 0xFF, 
                  *(float *) (pToken + 1), 
                  *(float *) (pToken + 2), 
                  *(float *) (pToken + 3), 
                  *(float *) (pToken + 4));

            This->constantsUsedBitmap[*pToken & 0xFF] = VS_CONSTANT_CONSTANT;
            pToken += 5;
            continue;

      } else if (D3DSIO_DCL == curOpcode->opcode) {

            /* Handle declarations here, they don't fit well with the
             * other instructions below [for now ] */

            if (This->namedArrays) {
                const char* attribName = "undefined";
                switch(*pToken & 0xFFFF) {
                    case D3DDECLUSAGE_POSITION:
                    attribName = "vertex.position";
                    break;
                    case D3DDECLUSAGE_BLENDINDICES:
                    /* not supported by openGL */
                    attribName = "vertex.blend";
                    break;
                    case D3DDECLUSAGE_BLENDWEIGHT:
                    attribName = "vertex.weight";
                    break;
                    case D3DDECLUSAGE_NORMAL:
                    attribName = "vertex.normal";
                    break;
                    case D3DDECLUSAGE_PSIZE:
                    attribName = "vertex.psize";
                    break;
                    case D3DDECLUSAGE_COLOR:
                    if((*pToken & 0xF0000) >> 16 == 0)  {
                        attribName = "vertex.color";
                    } else {
                        attribName = "vertex.color.secondary";
                    }
                    break;
                    case D3DDECLUSAGE_TEXCOORD:
                    {
                        char tmpChar[100];
                        tmpChar[0] = 0;
                        sprintf(tmpChar,"vertex.texcoord[%lu]",(*pToken & 0xF0000) >> 16);
                        attribName = tmpChar;
                        break;
                    }
                    /* The following aren't directly supported by openGL, so shouldn't come up using namedarrays. */
                    case D3DDECLUSAGE_TANGENT:
                    attribName = "vertex.tangent";
                    break;
                    case D3DDECLUSAGE_BINORMAL:
                    attribName = "vertex.binormal";
                    break;
                    case D3DDECLUSAGE_TESSFACTOR:
                    attribName = "vertex.tessfactor";
                    break;
                    case D3DDECLUSAGE_POSITIONT:
                    attribName = "vertex.possitionT";
                    break;
                    case D3DDECLUSAGE_FOG:
                    attribName = "vertex.fogcoord";
                    break;
                    case D3DDECLUSAGE_DEPTH:
                    attribName = "vertex.depth";
                    break;
                    case D3DDECLUSAGE_SAMPLE:
                    attribName = "vertex.sample";
                    break;
                    default:
                    FIXME("Unrecognised dcl %08lx", *pToken & 0xFFFF);
                }
                {
                    ++pToken;
                    sprintf(tmpLine, "ATTRIB ");
                    vshader_program_add_param(This, *pToken, FALSE, tmpLine);
                    if (This->namedArrays) 
                        shader_addline(&buffer, "%s = %s;\n", tmpLine, attribName);
                }
            } else {
                /* eat the token so it doesn't generate a warning */
                ++pToken;
            }
            ++pToken;
            continue;

      } else {

        /* Common Processing: ([instr] [dst] [src]*) */

        switch (curOpcode->opcode) {
        case D3DSIO_NOP:
            continue;
        case D3DSIO_MOV:
            /* Address registers must be loaded with the ARL instruction */
            if (shader_get_regtype(*pToken) == D3DSPR_ADDR) {
                strcpy(tmpLine, "ARL");
                break;
            }
            /* fall through */
        case D3DSIO_ADD:
        case D3DSIO_SUB:
        case D3DSIO_MAD:
        case D3DSIO_MUL:
        case D3DSIO_RCP:
        case D3DSIO_RSQ:
        case D3DSIO_DP3:
        case D3DSIO_DP4:
        case D3DSIO_MIN:
        case D3DSIO_MAX:
        case D3DSIO_SLT:
        case D3DSIO_SGE:
        case D3DSIO_LIT:
        case D3DSIO_DST:
        case D3DSIO_FRC:
        case D3DSIO_EXPP:
        case D3DSIO_LOGP:
        case D3DSIO_EXP:
        case D3DSIO_LOG:
            strcpy(tmpLine, curOpcode->glname);
            break;
        case D3DSIO_M4x4:
        case D3DSIO_M4x3:
        case D3DSIO_M3x4:
        case D3DSIO_M3x3:
        case D3DSIO_M3x2:
            /* Expand the macro and get nusprintf(tmpLine,mber of generated instruction */
            nRemInstr = ExpandMxMacro(curOpcode->opcode, pToken);
            /* Save point to next instruction */
            pSavedToken = pToken + 3;
            /* Execute expanded macro */
            pToken = MacroExpansion;
            continue;

        default:
            if (curOpcode->glname == GLNAME_REQUIRE_GLSL) {
                FIXME("Opcode %s requires Gl Shader languange 1.0\n", curOpcode->name);
            } else {
                FIXME("Can't handle opcode %s in hwShader\n", curOpcode->name);
            }

            pToken += curOpcode->num_params;
            continue;
        }

        if (curOpcode->num_params > 0) {
            vshader_program_add_param(This, *pToken, FALSE, tmpLine);

            ++pToken;
            for (i = 1; i < curOpcode->num_params; ++i) {
                strcat(tmpLine, ",");
                vshader_program_add_param(This, *pToken, TRUE, tmpLine);
                ++pToken;
            }
        }
        shader_addline(&buffer, "%s;\n", tmpLine);
      }
    }
    shader_addline(&buffer, "END\n"); 
  }

  /* finally null terminate the buffer */
  buffer.buffer[buffer.bsize] = 0;

  /* Check that Vertex Shaders are supported */
  if (GL_SUPPORT(ARB_VERTEX_PROGRAM)) {
      /*  Create the hw shader */
      /* TODO: change to resource.glObjectHandel or something like that */
      GL_EXTCALL(glGenProgramsARB(1, &This->baseShader.prgId));
      TRACE("Creating a hw vertex shader, prg=%d\n", This->baseShader.prgId);
      GL_EXTCALL(glBindProgramARB(GL_VERTEX_PROGRAM_ARB, This->baseShader.prgId));

      /* Create the program and check for errors */
      GL_EXTCALL(glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, 
          buffer.bsize, buffer.buffer));

      if (glGetError() == GL_INVALID_OPERATION) {
          GLint errPos;
          glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errPos);
          FIXME("HW VertexShader Error at position %d: %s\n",
                errPos, debugstr_a((const char *)glGetString(GL_PROGRAM_ERROR_STRING_ARB)));
          This->baseShader.prgId = -1;
      }
  }
#if 1 /* if were using the data buffer of device then we don't need to free it */
  HeapFree(GetProcessHeap(), 0, buffer.buffer);
#endif
}

BOOL IWineD3DVertexShaderImpl_ExecuteHAL(IWineD3DVertexShader* iface, WINEVSHADERINPUTDATA* input, WINEVSHADEROUTPUTDATA* output) {
  /**
   * TODO: use the NV_vertex_program (or 1_1) extension
   *  and specifics vendors (ARB_vertex_program??) variants for it
   */
  return TRUE;
}

HRESULT WINAPI IWineD3DVertexShaderImpl_ExecuteSW(IWineD3DVertexShader* iface, WINEVSHADERINPUTDATA* input, WINEVSHADEROUTPUTDATA* output) {
    IWineD3DVertexShaderImpl *This = (IWineD3DVertexShaderImpl *)iface;
        
    /** Vertex Shader Temporary Registers */
    WINED3DSHADERVECTOR R[12];
      /*D3DSHADERSCALAR A0;*/
    WINED3DSHADERVECTOR A[1];
    /** temporary Vector for modifier management */
    WINED3DSHADERVECTOR d;
    WINED3DSHADERVECTOR s[3];
    /** parser datas */
    const DWORD* pToken = This->baseShader.function;
    const SHADER_OPCODE* curOpcode = NULL;
    /** functions parameters */
    WINED3DSHADERVECTOR* p[4];
    WINED3DSHADERVECTOR* p_send[4];
    DWORD i;

    /** init temporary register */
    memset(R, 0, 12 * sizeof(WINED3DSHADERVECTOR));

    /* vshader_program_parse(vshader); */
#if 0 /* Must not be 1 in cvs */
    TRACE("Input:\n");
    TRACE_VSVECTOR(This->data->C[0]);
    TRACE_VSVECTOR(This->data->C[1]);
    TRACE_VSVECTOR(This->data->C[2]);
    TRACE_VSVECTOR(This->data->C[3]);
    TRACE_VSVECTOR(This->data->C[4]);
    TRACE_VSVECTOR(This->data->C[5]);
    TRACE_VSVECTOR(This->data->C[6]);
    TRACE_VSVECTOR(This->data->C[7]);
    TRACE_VSVECTOR(This->data->C[8]);
    TRACE_VSVECTOR(This->data->C[64]);
    TRACE_VSVECTOR(input->V[D3DVSDE_POSITION]);
    TRACE_VSVECTOR(input->V[D3DVSDE_BLENDWEIGHT]);
    TRACE_VSVECTOR(input->V[D3DVSDE_BLENDINDICES]);
    TRACE_VSVECTOR(input->V[D3DVSDE_NORMAL]);
    TRACE_VSVECTOR(input->V[D3DVSDE_PSIZE]);
    TRACE_VSVECTOR(input->V[D3DVSDE_DIFFUSE]);
    TRACE_VSVECTOR(input->V[D3DVSDE_SPECULAR]);
    TRACE_VSVECTOR(input->V[D3DVSDE_TEXCOORD0]);
    TRACE_VSVECTOR(input->V[D3DVSDE_TEXCOORD1]);
#endif

    TRACE_VSVECTOR(vshader->data->C[64]);
    /* TODO: Run through all the tokens and find and labels, if, endifs, loops etc...., and make a labels list */

    /* the first dword is the version tag */
    /* TODO: parse it */

    if (vshader_is_version_token(*pToken)) { /** version */
        ++pToken;
    }
    while (D3DVS_END() != *pToken) {
        if (vshader_is_comment_token(*pToken)) { /** comment */
            DWORD comment_len = (*pToken & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;
            ++pToken;
            pToken += comment_len;
            continue ;
        }
        curOpcode = shader_get_opcode((IWineD3DBaseShader*) This, *pToken);
        ++pToken;
        if (NULL == curOpcode) {
            i = 0;
            /* unknown current opcode ... */
            /* TODO: Think of a name for 0x80000000 and replace its use with a constant */
            while (*pToken & 0x80000000) {
                if (i == 0) {
                    FIXME("unrecognized opcode: pos=%d token=%08lX\n", (pToken - 1) - This->baseShader.function, *(pToken - 1));
                }
                FIXME("unrecognized opcode param: pos=%d token=%08lX what=", pToken - This->baseShader.function, *pToken);
                vshader_program_dump_vs_param(*pToken, i);
                TRACE("\n");
                ++i;
                ++pToken;
            }
            /* return FALSE; */
        } else {
            if (curOpcode->num_params > 0) {
                /* TRACE(">> execting opcode: pos=%d opcode_name=%s token=%08lX\n", pToken - vshader->function, curOpcode->name, *pToken); */
                for (i = 0; i < curOpcode->num_params; ++i) {
                    DWORD reg = pToken[i] & D3DSP_REGNUM_MASK;
                    DWORD regtype = shader_get_regtype(pToken[i]);
    
                    switch (regtype) {
                    case D3DSPR_TEMP:
                        /* TRACE("p[%d]=R[%d]\n", i, reg); */
                        p[i] = &R[reg];
                        break;
                    case D3DSPR_INPUT:
                        /* TRACE("p[%d]=V[%s]\n", i, VertexShaderDeclRegister[reg]); */
                        p[i] = &input->V[reg];
                        break;
                    case D3DSPR_CONST:
                        if (pToken[i] & D3DVS_ADDRMODE_RELATIVE) {
                            p[i] = &This->data->C[(DWORD) A[0].x + reg];
                        } else {
                            p[i] = &This->data->C[reg];
                        }
                        break;
                    case D3DSPR_ADDR: /* case D3DSPR_TEXTURE: */
                        if (0 != reg) {
                            ERR("cannot handle address registers != a0, forcing use of a0\n");
                            reg = 0;
                        }
                        /* TRACE("p[%d]=A[%d]\n", i, reg); */
                        p[i] = &A[reg];
                        break;
                    case D3DSPR_RASTOUT:
                        switch (reg) {
                        case D3DSRO_POSITION:
                            p[i] = &output->oPos;
                            break;
                        case D3DSRO_FOG:
                            p[i] = &output->oFog;
                            break;
                        case D3DSRO_POINT_SIZE:
                            p[i] = &output->oPts;
                            break;
                        }
                        break;
                    case D3DSPR_ATTROUT:
                        /* TRACE("p[%d]=oD[%d]\n", i, reg); */
                        p[i] = &output->oD[reg];
                        break;
                    case D3DSPR_TEXCRDOUT:
                        /* TRACE("p[%d]=oT[%d]\n", i, reg); */
                        p[i] = &output->oT[reg];
                        break;
                    /* TODO Decls and defs */
#if 0
                    case D3DSPR_DCL:
                    case D3DSPR_DEF:
#endif
                    default:
                        break;
                    }

                    if (i > 0) { /* input reg */
                        DWORD swizzle = (pToken[i] & D3DVS_SWIZZLE_MASK) >> D3DVS_SWIZZLE_SHIFT;
                        UINT isNegative = ((pToken[i] & D3DSP_SRCMOD_MASK) == D3DSPSM_NEG);

                        if (!isNegative && (D3DVS_NOSWIZZLE >> D3DVS_SWIZZLE_SHIFT) == swizzle) {
                            /* TRACE("p[%d] not swizzled\n", i); */
                            p_send[i] = p[i];
                        } else {
                            DWORD swizzle_x = swizzle & 0x03;
                            DWORD swizzle_y = (swizzle >> 2) & 0x03;
                            DWORD swizzle_z = (swizzle >> 4) & 0x03;
                            DWORD swizzle_w = (swizzle >> 6) & 0x03;
                            /* TRACE("p[%d] swizzled\n", i); */
                            float* tt = (float*) p[i];
                            s[i].x = (isNegative) ? -tt[swizzle_x] : tt[swizzle_x];
                            s[i].y = (isNegative) ? -tt[swizzle_y] : tt[swizzle_y];
                            s[i].z = (isNegative) ? -tt[swizzle_z] : tt[swizzle_z];
                            s[i].w = (isNegative) ? -tt[swizzle_w] : tt[swizzle_w];
                            p_send[i] = &s[i];
                        }
                    } else { /* output reg */
                        if ((pToken[i] & D3DSP_WRITEMASK_ALL) == D3DSP_WRITEMASK_ALL) {
                            p_send[i] = p[i];
                        } else {
                            p_send[i] = &d; /* to be post-processed for modifiers management */
                        }
                    }
                }
            }

            switch (curOpcode->num_params) {
            case 0:
                curOpcode->soft_fct();
                break;
            case 1:
                curOpcode->soft_fct(p_send[0]);
            break;
            case 2:
                curOpcode->soft_fct(p_send[0], p_send[1]);
                break;
            case 3:
                curOpcode->soft_fct(p_send[0], p_send[1], p_send[2]);
                break;
            case 4:
                curOpcode->soft_fct(p_send[0], p_send[1], p_send[2], p_send[3]);
                break;
            case 5:
                curOpcode->soft_fct(p_send[0], p_send[1], p_send[2], p_send[3], p_send[4]);
                break;
            case 6:
                curOpcode->soft_fct(p_send[0], p_send[1], p_send[2], p_send[3], p_send[4], p_send[5]);
                break;
            default:
                ERR("%s too many params: %u\n", curOpcode->name, curOpcode->num_params);
            }

            /* check if output reg modifier post-process */
            if (curOpcode->num_params > 0 && (pToken[0] & D3DSP_WRITEMASK_ALL) != D3DSP_WRITEMASK_ALL) {
                if (pToken[0] & D3DSP_WRITEMASK_0) p[0]->x = d.x; 
                if (pToken[0] & D3DSP_WRITEMASK_1) p[0]->y = d.y; 
                if (pToken[0] & D3DSP_WRITEMASK_2) p[0]->z = d.z; 
                if (pToken[0] & D3DSP_WRITEMASK_3) p[0]->w = d.w; 
            }
#if 0
            TRACE_VSVECTOR(output->oPos);
            TRACE_VSVECTOR(output->oD[0]);
            TRACE_VSVECTOR(output->oD[1]);
            TRACE_VSVECTOR(output->oT[0]);
            TRACE_VSVECTOR(output->oT[1]);
            TRACE_VSVECTOR(R[0]);
            TRACE_VSVECTOR(R[1]);
            TRACE_VSVECTOR(R[2]);
            TRACE_VSVECTOR(R[3]);
            TRACE_VSVECTOR(R[4]);
            TRACE_VSVECTOR(R[5]);
#endif

            /* to next opcode token */
            pToken += curOpcode->num_params;
        }
#if 0
        TRACE("End of current instruction:\n");
        TRACE_VSVECTOR(output->oPos);
        TRACE_VSVECTOR(output->oD[0]);
        TRACE_VSVECTOR(output->oD[1]);
        TRACE_VSVECTOR(output->oT[0]);
        TRACE_VSVECTOR(output->oT[1]);
        TRACE_VSVECTOR(R[0]);
        TRACE_VSVECTOR(R[1]);
        TRACE_VSVECTOR(R[2]);
        TRACE_VSVECTOR(R[3]);
        TRACE_VSVECTOR(R[4]);
        TRACE_VSVECTOR(R[5]);
#endif
    }
#if 0 /* Must not be 1 in cvs */
    TRACE("Output:\n");
    TRACE_VSVECTOR(output->oPos);
    TRACE_VSVECTOR(output->oD[0]);
    TRACE_VSVECTOR(output->oD[1]);
    TRACE_VSVECTOR(output->oT[0]);
    TRACE_VSVECTOR(output->oT[1]);
#endif
    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DVertexShaderImpl_SetConstantF(IWineD3DVertexShader *iface, UINT StartRegister, CONST FLOAT *pConstantData, UINT Vector4fCount) {
    IWineD3DVertexShaderImpl *This = (IWineD3DVertexShaderImpl *)iface;
    FIXME("(%p) : stub\n", This);
    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DVertexShaderImpl_GetConstantF(IWineD3DVertexShader *iface, UINT StartRegister, FLOAT *pConstantData, UINT Vector4fCount) {
    IWineD3DVertexShaderImpl *This = (IWineD3DVertexShaderImpl *)iface;
    FIXME("(%p) : stub\n", This);
    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DVertexShaderImpl_SetConstantI(IWineD3DVertexShader *iface, UINT StartRegister, CONST int *pConstantData, UINT Vector4iCount) {
    IWineD3DVertexShaderImpl *This = (IWineD3DVertexShaderImpl *)iface;
    if (StartRegister + Vector4iCount > WINED3D_VSHADER_MAX_CONSTANTS) {
        ERR("(%p) : SetVertexShaderConstantI C[%u] invalid\n", This, StartRegister);
        return WINED3DERR_INVALIDCALL;
    }
    if (NULL == pConstantData) {
        return WINED3DERR_INVALIDCALL;
    }
    FIXME("(%p) : stub\n", This);
    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DVertexShaderImpl_GetConstantI(IWineD3DVertexShader *iface, UINT StartRegister, int *pConstantData, UINT Vector4iCount) {
    IWineD3DVertexShaderImpl *This = (IWineD3DVertexShaderImpl *)iface;
    TRACE("(%p) : C[%u] count=%u\n", This, StartRegister, Vector4iCount);
    if (StartRegister + Vector4iCount > WINED3D_VSHADER_MAX_CONSTANTS) {
        return WINED3DERR_INVALIDCALL;
    }
    if (NULL == pConstantData) {
        return WINED3DERR_INVALIDCALL;
    }
    FIXME("(%p) : stub\n", This);
    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DVertexShaderImpl_SetConstantB(IWineD3DVertexShader *iface, UINT StartRegister, CONST BOOL *pConstantData, UINT BoolCount) {
    IWineD3DVertexShaderImpl *This = (IWineD3DVertexShaderImpl *)iface;
    if (StartRegister + BoolCount > WINED3D_VSHADER_MAX_CONSTANTS) {
        ERR("(%p) : SetVertexShaderConstantB C[%u] invalid\n", This, StartRegister);
        return WINED3DERR_INVALIDCALL;
    }
    if (NULL == pConstantData) {
        return WINED3DERR_INVALIDCALL;
    }
    FIXME("(%p) : stub\n", This);
    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DVertexShaderImpl_GetConstantB(IWineD3DVertexShader *iface, UINT StartRegister, BOOL *pConstantData, UINT BoolCount) {
    IWineD3DVertexShaderImpl* This = (IWineD3DVertexShaderImpl *)iface;
    FIXME("(%p) : stub\n", This);
    return WINED3D_OK;
}

#endif

/* *******************************************
   IWineD3DVertexShader IUnknown parts follow
   ******************************************* */
HRESULT WINAPI IWineD3DVertexShaderImpl_QueryInterface(IWineD3DVertexShader *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DVertexShaderImpl *This = (IWineD3DVertexShaderImpl *)iface;
    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown) 
        || IsEqualGUID(riid, &IID_IWineD3DBase)
        || IsEqualGUID(riid, &IID_IWineD3DBaseShader)
        || IsEqualGUID(riid, &IID_IWineD3DVertexShader)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }
    *ppobj = NULL;
    return E_NOINTERFACE;
}

ULONG WINAPI IWineD3DVertexShaderImpl_AddRef(IWineD3DVertexShader *iface) {
    IWineD3DVertexShaderImpl *This = (IWineD3DVertexShaderImpl *)iface;
    TRACE("(%p) : AddRef increasing from %ld\n", This, This->ref);
    return InterlockedIncrement(&This->ref);
}

ULONG WINAPI IWineD3DVertexShaderImpl_Release(IWineD3DVertexShader *iface) {
    IWineD3DVertexShaderImpl *This = (IWineD3DVertexShaderImpl *)iface;
    ULONG ref;
    TRACE("(%p) : Releasing from %ld\n", This, This->ref);
    ref = InterlockedDecrement(&This->ref);
    if (ref == 0) {
        if (This->vertexDeclaration) IWineD3DVertexDeclaration_Release(This->vertexDeclaration);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* *******************************************
   IWineD3DVertexShader IWineD3DVertexShader parts follow
   ******************************************* */

HRESULT WINAPI IWineD3DVertexShaderImpl_GetParent(IWineD3DVertexShader *iface, IUnknown** parent){
    IWineD3DVertexShaderImpl *This = (IWineD3DVertexShaderImpl *)iface;
    
    *parent = This->parent;
    IUnknown_AddRef(*parent);
    TRACE("(%p) : returning %p\n", This, *parent);
    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DVertexShaderImpl_GetDevice(IWineD3DVertexShader* iface, IWineD3DDevice **pDevice){
    IWineD3DVertexShaderImpl *This = (IWineD3DVertexShaderImpl *)iface;
    IWineD3DDevice_AddRef((IWineD3DDevice *)This->wineD3DDevice);
    *pDevice = (IWineD3DDevice *)This->wineD3DDevice;
    TRACE("(%p) returning %p\n", This, *pDevice);
    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DVertexShaderImpl_GetFunction(IWineD3DVertexShader* impl, VOID* pData, UINT* pSizeOfData) {
    IWineD3DVertexShaderImpl *This = (IWineD3DVertexShaderImpl *)impl;
    FIXME("(%p) : pData(%p), pSizeOfData(%p)\n", This, pData, pSizeOfData);

    if (NULL == pData) {
        *pSizeOfData = This->baseShader.functionLength;
        return WINED3D_OK;
    }
    if (*pSizeOfData < This->baseShader.functionLength) {
        *pSizeOfData = This->baseShader.functionLength;
        return WINED3DERR_MOREDATA;
    }
    if (NULL == This->baseShader.function) { /* no function defined */
        TRACE("(%p) : GetFunction no User Function defined using NULL to %p\n", This, pData);
        (*(DWORD **) pData) = NULL;
    } else {
        if(This->baseShader.functionLength == 0){

        }
        TRACE("(%p) : GetFunction copying to %p\n", This, pData);
        memcpy(pData, This->baseShader.function, This->baseShader.functionLength);
    }
    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DVertexShaderImpl_SetFunction(IWineD3DVertexShader *iface, CONST DWORD *pFunction) {
    IWineD3DVertexShaderImpl *This =(IWineD3DVertexShaderImpl *)iface;
    const DWORD* pToken = pFunction;
    const SHADER_OPCODE* curOpcode = NULL;
    DWORD len = 0;
    DWORD i;
    TRACE("(%p) : Parsing programme\n", This);

    /* Initialise vertex input arrays */
    This->namedArrays = FALSE;
    This->declaredArrays = FALSE;
    for (i = 0; i < WINED3DSHADERDECLUSAGE_MAX_USAGE; i++)
        This->arrayUsageMap[i] = -1;

    if (NULL != pToken) {
        while (D3DVS_END() != *pToken) {
            if (vshader_is_version_token(*pToken)) { /** version */
                vshader_set_version(This, *pToken);
                ++pToken;
                ++len;
                continue;
            }
            if (vshader_is_comment_token(*pToken)) { /** comment */
                DWORD comment_len = (*pToken & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;
                ++pToken;
                TRACE("//%s\n", (char*)pToken);
                pToken += comment_len;
                len += comment_len + 1;
                continue;
            }
            curOpcode = shader_get_opcode((IWineD3DBaseShader*) This, *pToken);
            ++pToken;
            ++len;
            if (NULL == curOpcode) {
                /* TODO: Think of a good name for 0x80000000 and replace it with a constant */
                while (*pToken & 0x80000000) {
                    /* unknown current opcode ... */
                    FIXME("unrecognized opcode: %08lx", *pToken);
                    ++pToken;
                    ++len;
                    TRACE("\n");
                }

            } else {
                if (curOpcode->opcode == D3DSIO_DCL) {

                    DWORD usage = *pToken;
                    DWORD param = *(pToken + 1);

                    parse_decl_usage(This, usage, param & D3DSP_REGNUM_MASK);
                    vshader_program_dump_decl_usage(This, usage, param);
                    vshader_program_dump_vs_param(param, 0);
                    pToken += 2;
                    len += 2;

                } else 
                    if (curOpcode->opcode == D3DSIO_DEF) {
                        TRACE("def c%lu = ", *pToken & 0xFF);
                        ++pToken;
                        ++len;
                        TRACE("%f ,", *(float *)pToken);
                        ++pToken;
                        ++len;
                        TRACE("%f ,", *(float *)pToken);
                        ++pToken;
                        ++len;
                        TRACE("%f ,", *(float *)pToken);
                        ++pToken;
                        ++len;
                        TRACE("%f", *(float *)pToken);
                        ++pToken;
                        ++len;
                } else {
                    TRACE("%s ", curOpcode->name);
                    if (curOpcode->num_params > 0) {
                        vshader_program_dump_vs_param(*pToken, 0);
                        ++pToken;
                        ++len;
                        for (i = 1; i < curOpcode->num_params; ++i) {
                            TRACE(", ");
                            vshader_program_dump_vs_param(*pToken, 1);
                            ++pToken;
                            ++len;
                        }
                    }
                }
                TRACE("\n");
            }
        }
        This->baseShader.functionLength = (len + 1) * sizeof(DWORD);
    } else {
        This->baseShader.functionLength = 1; /* no Function defined use fixed function vertex processing */
    }

/* Handy for debugging using numbered arrays instead of named arrays */
#if 1
    /* TODO: using numbered arrays for software shaders makes things easier */
    This->declaredArrays = TRUE;
#endif

    /* named arrays and declared arrays are mutually exclusive */
    if (This->declaredArrays) 
        This->namedArrays = FALSE;

    /* Generate HW shader in needed */
    if (NULL != pFunction  && wined3d_settings.vs_mode == VS_HW) {
#if 1
        IWineD3DVertexShaderImpl_GenerateProgramArbHW(iface, pFunction);
#endif
    }

    /* copy the function ... because it will certainly be released by application */
    if (NULL != pFunction) {
        This->baseShader.function = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, This->baseShader.functionLength);
        memcpy((void *)This->baseShader.function, pFunction, This->baseShader.functionLength);
    } else {
        This->baseShader.function = NULL;
    }
    return WINED3D_OK;
}

const IWineD3DVertexShaderVtbl IWineD3DVertexShader_Vtbl =
{
    /*** IUnknown methods ***/
    IWineD3DVertexShaderImpl_QueryInterface,
    IWineD3DVertexShaderImpl_AddRef,
    IWineD3DVertexShaderImpl_Release,
    /*** IWineD3DBase methods ***/
    IWineD3DVertexShaderImpl_GetParent,
    /*** IWineD3DBaseShader methods ***/
    IWineD3DVertexShaderImpl_SetFunction,
    /*** IWineD3DVertexShader methods ***/
    IWineD3DVertexShaderImpl_GetDevice,
    IWineD3DVertexShaderImpl_GetFunction
};
