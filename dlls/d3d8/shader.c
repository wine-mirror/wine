/*
 * shaders implementation
 *
 * Copyright 2002-2004 Raphael Junqueira
 * Copyright 2004 Jason Edmeades
 * Copyright 2004 Christian Costa
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
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "wine/debug.h"

#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_shader);
WINE_DECLARE_DEBUG_CHANNEL(d3d_hw_shader);

/* Shader debugging - Change the following line to enable debugging of software
      vertex shaders                                                             */
#if 0 /* Must not be 1 in cvs version */
# define VSTRACE(A) TRACE A
# define TRACE_VSVECTOR(name) TRACE( #name "=(%f, %f, %f, %f)\n", name.x, name.y, name.z, name.w)
#else 
# define VSTRACE(A) 
# define TRACE_VSVECTOR(name)
#endif

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
 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/programmingguide/GettingStarted/VertexFormats/vformats.asp
 *
 * NVIDIA: DX8 Vertex Shader to NV Vertex Program
 *  http://developer.nvidia.com/view.asp?IO=vstovp
 *
 * NVIDIA: Memory Management with VAR
 *  http://developer.nvidia.com/view.asp?IO=var_memory_management 
 */

typedef void (*shader_fct_t)();

typedef struct SHADER_OPCODE {
  CONST WORD    opcode;
  const char*   name;
  CONST UINT    num_params;
  shader_fct_t  soft_fct;
  DWORD         min_version;
  DWORD         max_version;
} SHADER_OPCODE;

/*******************************
 * vshader functions software VM
 */

void vshader_add(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0, D3DSHADERVECTOR* s1) {
  d->x = s0->x + s1->x;
  d->y = s0->y + s1->y;
  d->z = s0->z + s1->z;
  d->w = s0->w + s1->w;
  VSTRACE(("executing add: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	         s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

void vshader_dp3(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0, D3DSHADERVECTOR* s1) {
  d->x = d->y = d->z = d->w = s0->x * s1->x + s0->y * s1->y + s0->z * s1->z;
  VSTRACE(("executing dp3: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	         s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

void vshader_dp4(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0, D3DSHADERVECTOR* s1) {
  d->x = d->y = d->z = d->w = s0->x * s1->x + s0->y * s1->y + s0->z * s1->z + s0->w * s1->w;
  VSTRACE(("executing dp4: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	  s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

void vshader_dst(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0, D3DSHADERVECTOR* s1) {
  d->x = 1.0f;
  d->y = s0->y * s1->y;
  d->z = s0->z;
  d->w = s1->w;
  VSTRACE(("executing dst: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	  s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

void vshader_expp(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0) {
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

void vshader_lit(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0) {
  d->x = 1.0f;
  d->y = (0.0f < s0->x) ? s0->x : 0.0f;
  d->z = (0.0f < s0->x && 0.0f < s0->y) ? powf(s0->y, s0->w) : 0.0f;
  d->w = 1.0f;
  VSTRACE(("executing lit: s0=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	         s0->x, s0->y, s0->z, s0->w, d->x, d->y, d->z, d->w));
}

void vshader_logp(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0) {
  float tmp_f = fabsf(s0->w); 
  d->x = d->y = d->z = d->w = (0.0f != tmp_f) ? logf(tmp_f) / logf(2.0f) : -HUGE_VAL;
  VSTRACE(("executing logp: s0=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	         s0->x, s0->y, s0->z, s0->w, d->x, d->y, d->z, d->w));
}

void vshader_mad(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0, D3DSHADERVECTOR* s1, D3DSHADERVECTOR* s2) {
  d->x = s0->x * s1->x + s2->x;
  d->y = s0->y * s1->y + s2->y;
  d->z = s0->z * s1->z + s2->z;
  d->w = s0->w * s1->w + s2->w;
  VSTRACE(("executing mad: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) s2=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	  s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, s2->x, s2->y, s2->z, s2->w, d->x, d->y, d->z, d->w));
}

void vshader_max(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0, D3DSHADERVECTOR* s1) {
  d->x = (s0->x >= s1->x) ? s0->x : s1->x;
  d->y = (s0->y >= s1->y) ? s0->y : s1->y;
  d->z = (s0->z >= s1->z) ? s0->z : s1->z;
  d->w = (s0->w >= s1->w) ? s0->w : s1->w;
  VSTRACE(("executing max: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	  s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

void vshader_min(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0, D3DSHADERVECTOR* s1) {
  d->x = (s0->x < s1->x) ? s0->x : s1->x;
  d->y = (s0->y < s1->y) ? s0->y : s1->y;
  d->z = (s0->z < s1->z) ? s0->z : s1->z;
  d->w = (s0->w < s1->w) ? s0->w : s1->w;
  VSTRACE(("executing min: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	  s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

void vshader_mov(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0) {
  d->x = s0->x;
  d->y = s0->y;
  d->z = s0->z;
  d->w = s0->w;
  VSTRACE(("executing mov: s0=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	  s0->x, s0->y, s0->z, s0->w, d->x, d->y, d->z, d->w));
}

void vshader_mul(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0, D3DSHADERVECTOR* s1) {
  d->x = s0->x * s1->x;
  d->y = s0->y * s1->y;
  d->z = s0->z * s1->z;
  d->w = s0->w * s1->w;
  VSTRACE(("executing mul: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	  s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

void vshader_nop(void) {
  /* NOPPPP ahhh too easy ;) */
}

void vshader_rcp(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0) {
  d->x = d->y = d->z = d->w = (0.0f == s0->w) ? HUGE_VAL : 1.0f / s0->w;
  VSTRACE(("executing rcp: s0=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	  s0->x, s0->y, s0->z, s0->w, d->x, d->y, d->z, d->w));
}

void vshader_rsq(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0) {
  float tmp_f = fabsf(s0->w);
  d->x = d->y = d->z = d->w = (0.0f == tmp_f) ? HUGE_VAL : ((1.0f != tmp_f) ? 1.0f / sqrtf(tmp_f) : 1.0f);
  VSTRACE(("executing rsq: s0=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	  s0->x, s0->y, s0->z, s0->w, d->x, d->y, d->z, d->w));
}

void vshader_sge(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0, D3DSHADERVECTOR* s1) {
  d->x = (s0->x >= s1->x) ? 1.0f : 0.0f;
  d->y = (s0->y >= s1->y) ? 1.0f : 0.0f;
  d->z = (s0->z >= s1->z) ? 1.0f : 0.0f;
  d->w = (s0->w >= s1->w) ? 1.0f : 0.0f;
  VSTRACE(("executing sge: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	  s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

void vshader_slt(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0, D3DSHADERVECTOR* s1) {
  d->x = (s0->x < s1->x) ? 1.0f : 0.0f;
  d->y = (s0->y < s1->y) ? 1.0f : 0.0f;
  d->z = (s0->z < s1->z) ? 1.0f : 0.0f;
  d->w = (s0->w < s1->w) ? 1.0f : 0.0f;
  VSTRACE(("executing slt: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	  s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

void vshader_sub(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0, D3DSHADERVECTOR* s1) {
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

void vshader_exp(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0) {
  d->x = d->y = d->z = d->w = powf(2.0f, s0->w);
  VSTRACE(("executing exp: s0=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	  s0->x, s0->y, s0->z, s0->w, d->x, d->y, d->z, d->w));
}

void vshader_log(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0) {
  float tmp_f = fabsf(s0->w); 
  d->x = d->y = d->z = d->w = (0.0f != tmp_f) ? logf(tmp_f) / logf(2.0f) : -HUGE_VAL;
  VSTRACE(("executing log: s0=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	  s0->x, s0->y, s0->z, s0->w, d->x, d->y, d->z, d->w));
}

void vshader_frc(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0) {
  d->x = s0->x - floorf(s0->x);
  d->y = s0->y - floorf(s0->y);
  d->z = 0.0f;
  d->w = 1.0f;
  VSTRACE(("executing frc: s0=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	  s0->x, s0->y, s0->z, s0->w, d->x, d->y, d->z, d->w));
}

typedef FLOAT D3DMATRIX44[4][4];
typedef FLOAT D3DMATRIX43[3][4];
typedef FLOAT D3DMATRIX34[4][3];
typedef FLOAT D3DMATRIX33[3][3];
typedef FLOAT D3DMATRIX32[2][3];

void vshader_m4x4(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0, /*D3DSHADERVECTOR* mat1*/ D3DMATRIX44 mat) {
  /*
   * BuGGY CODE: here only if cast not work for copy/paste
  D3DSHADERVECTOR* mat2 = mat1 + 1;
  D3DSHADERVECTOR* mat3 = mat1 + 2;
  D3DSHADERVECTOR* mat4 = mat1 + 3; 
  d->x = mat1->x * s0->x + mat2->x * s0->y + mat3->x * s0->z + mat4->x * s0->w;
  d->y = mat1->y * s0->x + mat2->y * s0->y + mat3->y * s0->z + mat4->y * s0->w;
  d->z = mat1->z * s0->x + mat2->z * s0->y + mat3->z * s0->z + mat4->z * s0->w;
  d->w = mat1->w * s0->x + mat2->w * s0->y + mat3->w * s0->z + mat4->w * s0->w;
  */
  d->x = mat[0][0] * s0->x + mat[0][1] * s0->y + mat[0][2] * s0->z + mat[0][3] * s0->w;
  d->y = mat[1][0] * s0->x + mat[1][1] * s0->y + mat[1][2] * s0->z + mat[1][3] * s0->w;
  d->z = mat[2][0] * s0->x + mat[2][1] * s0->y + mat[2][2] * s0->z + mat[2][3] * s0->w;
  d->w = mat[3][0] * s0->x + mat[3][1] * s0->y + mat[3][2] * s0->z + mat[3][3] * s0->w;
  VSTRACE(("executing m4x4(1): mat=(%f, %f, %f, %f)    s0=(%f)     d=(%f)\n", mat[0][0], mat[0][1], mat[0][2], mat[0][3], s0->x, d->x));
  VSTRACE(("executing m4x4(2): mat=(%f, %f, %f, %f)       (%f)       (%f)\n", mat[1][0], mat[1][1], mat[1][2], mat[1][3], s0->y, d->y));
  VSTRACE(("executing m4x4(3): mat=(%f, %f, %f, %f) X     (%f)  =    (%f)\n", mat[2][0], mat[2][1], mat[2][2], mat[2][3], s0->z, d->z));
  VSTRACE(("executing m4x4(4): mat=(%f, %f, %f, %f)       (%f)       (%f)\n", mat[3][0], mat[3][1], mat[3][2], mat[3][3], s0->w, d->w));
}

void vshader_m4x3(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0, D3DMATRIX43 mat) {
  d->x = mat[0][0] * s0->x + mat[0][1] * s0->y + mat[0][2] * s0->z + mat[0][3] * s0->w;
  d->y = mat[1][0] * s0->x + mat[1][1] * s0->y + mat[1][2] * s0->z + mat[1][3] * s0->w;
  d->z = mat[2][0] * s0->x + mat[2][1] * s0->y + mat[2][2] * s0->z + mat[2][3] * s0->w;
  d->w = 1.0f;
  VSTRACE(("executing m4x3(1): mat=(%f, %f, %f, %f)    s0=(%f)     d=(%f)\n", mat[0][0], mat[0][1], mat[0][2], mat[0][3], s0->x, d->x));
  VSTRACE(("executing m4x3(2): mat=(%f, %f, %f, %f)       (%f)       (%f)\n", mat[1][0], mat[1][1], mat[1][2], mat[1][3], s0->y, d->y));
  VSTRACE(("executing m4x3(3): mat=(%f, %f, %f, %f) X     (%f)  =    (%f)\n", mat[2][0], mat[2][1], mat[2][2], mat[2][3], s0->z, d->z));
  VSTRACE(("executing m4x3(4):                            (%f)       (%f)\n", s0->w, d->w));
}

void vshader_m3x4(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0, D3DMATRIX34 mat) {
  d->x = mat[0][0] * s0->x + mat[0][1] * s0->y + mat[0][2] * s0->z;
  d->y = mat[1][0] * s0->x + mat[1][1] * s0->y + mat[1][2] * s0->z;
  d->z = mat[2][0] * s0->x + mat[2][1] * s0->y + mat[2][2] * s0->z;
  d->w = mat[3][0] * s0->x + mat[3][1] * s0->y + mat[3][2] * s0->z;
  VSTRACE(("executing m3x4(1): mat=(%f, %f, %f)    s0=(%f)     d=(%f) \n", mat[0][0], mat[0][1], mat[0][2], s0->x, d->x));
  VSTRACE(("executing m3x4(2): mat=(%f, %f, %f)       (%f)       (%f) \n", mat[1][0], mat[1][1], mat[1][2], s0->y, d->y));
  VSTRACE(("executing m3x4(3): mat=(%f, %f, %f) X     (%f)  =    (%f) \n", mat[2][0], mat[2][1], mat[2][2], s0->z, d->z));
  VSTRACE(("executing m3x4(4): mat=(%f, %f, %f)       (%f)       (%f) \n", mat[3][0], mat[3][1], mat[3][2], s0->w, d->w));
}

void vshader_m3x3(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0, D3DMATRIX33 mat) {
  d->x = mat[0][0] * s0->x + mat[0][1] * s0->y + mat[0][2] * s0->z;
  d->y = mat[1][0] * s0->x + mat[1][1] * s0->y + mat[1][2] * s0->z;
  d->z = mat[2][0] * s0->x + mat[2][1] * s0->y + mat[2][2] * s0->z;
  d->w = 1.0f;
  VSTRACE(("executing m3x3(1): mat=(%f, %f, %f)    s0=(%f)     d=(%f) \n", mat[0][0], mat[0][1], mat[0][2], s0->x, d->x));
  VSTRACE(("executing m3x3(2): mat=(%f, %f, %f)       (%f)       (%f) \n", mat[1][0], mat[1][1], mat[1][2], s0->y, d->y));
  VSTRACE(("executing m3x3(3): mat=(%f, %f, %f) X     (%f)  =    (%f) \n", mat[2][0], mat[2][1], mat[2][2], s0->z, d->z));
  VSTRACE(("executing m3x3(4):                                       (%f) \n", d->w));
}

void vshader_m3x2(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0, D3DMATRIX32 mat) {
  FIXME("check\n");
  d->x = mat[0][0] * s0->x + mat[0][1] * s0->y + mat[0][2] * s0->z;
  d->y = mat[1][0] * s0->x + mat[1][1] * s0->y + mat[1][2] * s0->z;
  d->z = 0.0f;
  d->w = 1.0f;
}

/**
 * Version 2.0 specific
 */
void vshader_lrp(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0, D3DSHADERVECTOR* s1, D3DSHADERVECTOR* s2, D3DSHADERVECTOR* s3) {
  d->x = s0->x * (s1->x - s2->x) + s2->x; 
  d->y = s0->y * (s1->y - s2->y) + s2->y; 
  d->z = s0->z * (s1->z - s2->z) + s2->z; 
  d->w = s0->w * (s1->w - s2->w) + s2->x; 
}

/**
 * log, exp, frc, m*x* seems to be macros ins ... to see
 *   Note opcode must be in uppercase if direct mapping to GL hw shaders
 */
static CONST SHADER_OPCODE vshader_ins [] = {
  {D3DSIO_NOP,  "NOP",  0, vshader_nop, 0, 0},
  {D3DSIO_MOV,  "MOV",  2, vshader_mov, 0, 0},
  {D3DSIO_ADD,  "ADD",  3, vshader_add, 0, 0},
  {D3DSIO_SUB,  "SUB",  3, vshader_sub, 0, 0},
  {D3DSIO_MAD,  "MAD",  4, vshader_mad, 0, 0},
  {D3DSIO_MUL,  "MUL",  3, vshader_mul, 0, 0},
  {D3DSIO_RCP,  "RCP",  2, vshader_rcp, 0, 0},
  {D3DSIO_RSQ,  "RSQ",  2, vshader_rsq, 0, 0},
  {D3DSIO_DP3,  "DP3",  3, vshader_dp3, 0, 0},
  {D3DSIO_DP4,  "DP4",  3, vshader_dp4, 0, 0},
  {D3DSIO_MIN,  "MIN",  3, vshader_min, 0, 0},
  {D3DSIO_MAX,  "MAX",  3, vshader_max, 0, 0},
  {D3DSIO_SLT,  "SLT",  3, vshader_slt, 0, 0},
  {D3DSIO_SGE,  "SGE",  3, vshader_sge, 0, 0},
  {D3DSIO_EXP,  "EXP",  2, vshader_exp, 0, 0},
  {D3DSIO_LOG,  "LOG",  2, vshader_log, 0, 0},
  {D3DSIO_LIT,  "LIT",  2, vshader_lit, 0, 0},
  {D3DSIO_DST,  "DST",  3, vshader_dst, 0, 0},
  {D3DSIO_LRP,  "LRP",  5, vshader_lrp, 0, 0},
  {D3DSIO_FRC,  "FRC",  2, vshader_frc, 0, 0},
  {D3DSIO_M4x4, "M4X4", 3, vshader_m4x4, 0, 0},
  {D3DSIO_M4x3, "M4X3", 3, vshader_m4x3, 0, 0},
  {D3DSIO_M3x4, "M3X4", 3, vshader_m3x4, 0, 0},
  {D3DSIO_M3x3, "M3X3", 3, vshader_m3x3, 0, 0},
  {D3DSIO_M3x2, "M3X2", 3, vshader_m3x2, 0, 0},
  /** FIXME: use direct access so add the others opcodes as stubs */
  {D3DSIO_EXPP, "EXPP", 2, vshader_expp, 0, 0},
  {D3DSIO_LOGP, "LOGP", 2, vshader_logp, 0, 0},

  {0, NULL, 0, NULL, 0, 0}
};


inline static const SHADER_OPCODE* vshader_program_get_opcode(const DWORD code) {
  DWORD i = 0;
  /** TODO: use dichotomic search */
  while (NULL != vshader_ins[i].name) {
    if ((code & D3DSI_OPCODE_MASK) == vshader_ins[i].opcode) {
      return &vshader_ins[i];
    }
    ++i;
  }
  return NULL;
}

inline static BOOL vshader_is_version_token(DWORD token) {
  return 0xFFFE0000 == (token & 0xFFFE0000);
}

inline static BOOL vshader_is_comment_token(DWORD token) {
  return D3DSIO_COMMENT == (token & D3DSI_OPCODE_MASK);
}

inline static void vshader_program_dump_param(const DWORD param, int input) {
  static const char* rastout_reg_names[] = { "oPos", "oFog", "oPts" };
  static const char swizzle_reg_chars[] = "xyzw";

  DWORD reg = param & 0x00001FFF;
  DWORD regtype = ((param & D3DSP_REGTYPE_MASK) >> D3DSP_REGTYPE_SHIFT);

  if ((param & D3DSP_SRCMOD_MASK) == D3DSPSM_NEG) TRACE("-");
  
  switch (regtype << D3DSP_REGTYPE_SHIFT) {
  case D3DSPR_TEMP:
    TRACE("R[%lu]", reg);
    break;
  case D3DSPR_INPUT:
    TRACE("V[%lu]", reg);
    break;
  case D3DSPR_CONST:
    TRACE("C[%s%lu]", (param & D3DVS_ADDRMODE_RELATIVE) ? "a0.x + " : "", reg);
    break;
  case D3DSPR_ADDR: /*case D3DSPR_TEXTURE:*/
    TRACE("a[%lu]", reg);
    break;
  case D3DSPR_RASTOUT:
    TRACE("%s", rastout_reg_names[reg]);
    break;
  case D3DSPR_ATTROUT:
    TRACE("oD[%lu]", reg);
    break;
  case D3DSPR_TEXCRDOUT:
    TRACE("oT[%lu]", reg);
    break;
  default:
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

inline static void vshader_program_add_param(const DWORD param, int input, char *hwLine) {
  /*static const char* rastout_reg_names[] = { "oPos", "oFog", "oPts" }; */
  static const char* hwrastout_reg_names[] = { "result.position", "result.fogcoord", "result.pointsize" };
  static const char swizzle_reg_chars[] = "xyzw";

  DWORD reg = param & 0x00001FFF;
  DWORD regtype = ((param & D3DSP_REGTYPE_MASK) >> D3DSP_REGTYPE_SHIFT);
  char  tmpReg[255];

  if ((param & D3DSP_SRCMOD_MASK) == D3DSPSM_NEG) {
      strcat(hwLine, " -");
  } else {
      strcat(hwLine, " ");
  }
  
  switch (regtype << D3DSP_REGTYPE_SHIFT) {
  case D3DSPR_TEMP:
    sprintf(tmpReg, "T%lu", reg);
    strcat(hwLine, tmpReg);
    break;
  case D3DSPR_INPUT:
    sprintf(tmpReg, "vertex.attrib[%lu]", reg);
    strcat(hwLine, tmpReg);
    break;
  case D3DSPR_CONST:
    sprintf(tmpReg, "C[%s%lu]", (param & D3DVS_ADDRMODE_RELATIVE) ? "A0.x + " : "", reg);
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
    break;
  }

  if (!input) {
    /** operand output */
    if ((param & D3DSP_WRITEMASK_ALL) != D3DSP_WRITEMASK_ALL) {
      strcat(hwLine, ".");
      if (param & D3DSP_WRITEMASK_0) {
          strcat(hwLine, "x");
      }
      if (param & D3DSP_WRITEMASK_1) {
          strcat(hwLine, "y");
      }
      if (param & D3DSP_WRITEMASK_2) {
          strcat(hwLine, "z");
      }
      if (param & D3DSP_WRITEMASK_3) {
          strcat(hwLine, "w");
      }
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
        sprintf(tmpReg, ".%c", swizzle_reg_chars[swizzle_x]);
        strcat(hwLine, tmpReg);
      } else {
        sprintf(tmpReg, ".%c%c%c%c", 
		swizzle_reg_chars[swizzle_x], 
		swizzle_reg_chars[swizzle_y], 
		swizzle_reg_chars[swizzle_z], 
		swizzle_reg_chars[swizzle_w]);
        strcat(hwLine, tmpReg);
      }
    }
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

/**
 * Function parser ...
 */
inline static VOID IDirect3DVertexShaderImpl_GenerateProgramArbHW(IDirect3DVertexShaderImpl* vshader, CONST DWORD* pFunction) {
  const DWORD* pToken = pFunction;
  const DWORD* pSavedToken = NULL;
  const SHADER_OPCODE* curOpcode = NULL;
  int nRemInstr = -1;
  DWORD i;
  unsigned lineNum = 0;
  char *pgmStr = NULL;
  char  tmpLine[255];
  DWORD nUseAddressRegister = 0;
  DWORD nUseTempRegister = 0;
  DWORD regtype;
  DWORD reg;
  IDirect3DDevice8Impl* This = vshader->device;

  pgmStr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 65535); /* 64kb should be enough */

  /**
   * First pass to determine what we need to declare:
   *  - Temporary variables
   *  - Address variables
   */ 
  if (NULL != pToken) {
    while (D3DVS_END() != *pToken) {
      if (vshader_is_version_token(*pToken)) {
	/** skip version */
	++pToken;
	continue;
      } 
      if (vshader_is_comment_token(*pToken)) { /** comment */
	DWORD comment_len = (*pToken & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;
	++pToken;
	pToken += comment_len;
	continue;
      }
      curOpcode = vshader_program_get_opcode(*pToken);
      ++pToken;
      if (NULL == curOpcode) {
	while (*pToken & 0x80000000) {
	  /* skip unrecognized opcode */
	  ++pToken;
	}
      } else {
	if (curOpcode->num_params > 0) {
	  regtype = ((((*pToken) & D3DSP_REGTYPE_MASK) >> D3DSP_REGTYPE_SHIFT) << D3DSP_REGTYPE_SHIFT);
	  reg = ((*pToken)  & 0x00001FFF);
	  /** we should validate GL_MAX_PROGRAM_ADDRESS_REGISTERS_AR limits here */
	  if (D3DSPR_ADDR == regtype && nUseAddressRegister <= reg) nUseAddressRegister = reg + 1;
	  /** we should validate GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB limits here */
	  if (D3DSPR_TEMP == regtype && nUseTempRegister <= reg) nUseTempRegister = reg + 1;
	  ++pToken;
	  for (i = 1; i < curOpcode->num_params; ++i) {
	    regtype = ((((*pToken) & D3DSP_REGTYPE_MASK) >> D3DSP_REGTYPE_SHIFT) << D3DSP_REGTYPE_SHIFT);
	    reg = ((*pToken)  & 0x00001FFF);
	    /** we should validate GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB limits here */
	    if (D3DSPR_TEMP == regtype && nUseTempRegister <= reg) nUseTempRegister = reg + 1;
	    ++pToken;
	  }
	}
      }
    }
  }

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

      if (vshader_is_version_token(*pToken)) { /** version */

        /* Extract version *10 into integer value (ie. 1.0 == 10, 1.1==11 etc */
        int version = (((*pToken >> 8) & 0x0F) * 10) + (*pToken & 0x0F);
        int numTemps;
        int numConstants;

	TRACE_(d3d_hw_shader)("vs.%lu.%lu;\n", (*pToken >> 8) & 0x0F, (*pToken & 0x0F));

        /* Each release of vertex shaders has had different numbers of temp registers */
        switch (version) {
        case 10:
        case 11: numTemps=12; 
                 numConstants=96;
                 strcpy(tmpLine, "!!ARBvp1.0\n");
                 TRACE_(d3d_hw_shader)("GL HW (%u) : %s", strlen(pgmStr), tmpLine); /* Don't add \n to this line as already in tmpLine */
                 break;
        case 20: numTemps=12;
                 numConstants=256;
                 strcpy(tmpLine, "!!ARBvp2.0\n");
                 FIXME_(d3d_hw_shader)("No work done yet to support vs2.0 in hw\n");
                 TRACE_(d3d_hw_shader)("GL HW (%u) : %s", strlen(pgmStr), tmpLine); /* Don't add \n to this line as already in tmpLine */
                 break;
        case 30: numTemps=32; 
                 numConstants=256;
                 strcpy(tmpLine, "!!ARBvp3.0\n");
                 FIXME_(d3d_hw_shader)("No work done yet to support vs3.0 in hw\n");
                 TRACE_(d3d_hw_shader)("GL HW (%u) : %s", strlen(pgmStr), tmpLine); /* Don't add \n to this line as already in tmpLine */
                 break;
        default:
                 numTemps=12;
                 numConstants=96;
                 strcpy(tmpLine, "!!ARBvp1.0\n");
                 FIXME_(d3d_hw_shader)("Unrecognized vertex shader version!\n");
        }
        strcat(pgmStr,tmpLine);
	++lineNum;

        for (i = 0; i < nUseTempRegister/*we should check numTemps here*/; i++) {
            sprintf(tmpLine, "TEMP T%ld;\n", i);
	    ++lineNum;
            TRACE_(d3d_hw_shader)("GL HW (%u, %u) : %s", lineNum, strlen(pgmStr), tmpLine); /* Don't add \n to this line as already in tmpLine */
            strcat(pgmStr,tmpLine);
        }
	for (i = 0; i < nUseAddressRegister; i++) {
            sprintf(tmpLine, "ADDRESS A%ld;\n", i);
	    ++lineNum;
            TRACE_(d3d_hw_shader)("GL HW (%u, %u) : %s", lineNum, strlen(pgmStr), tmpLine); /* Don't add \n to this line as already in tmpLine */
            strcat(pgmStr,tmpLine);
	}
	/* Due to the dynamic constants binding mechanism, we need to declare
	 * all the constants for relative addressing. */
	/* Mesa supports nly 95 constants for VS1.X although we should have at least 96. */
	if (GL_VENDOR_NAME(This) == VENDOR_MESA || GL_VENDOR_NAME(This) == VENDOR_WINE) {
	  numConstants = 95;
	}
	sprintf(tmpLine, "PARAM C[%d] = { program.env[0..%d] };\n", numConstants, numConstants-1);
	TRACE_(d3d_hw_shader)("GL HW (%u,%u) : %s", lineNum, strlen(pgmStr), tmpLine); /* Don't add \n to this line as already in tmpLine */
	strcat(pgmStr, tmpLine);
	++lineNum;

	++pToken;
	continue;
      } 
      if (vshader_is_comment_token(*pToken)) { /** comment */
	DWORD comment_len = (*pToken & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;
	++pToken;
	/*TRACE("comment[%ld] ;%s\n", comment_len, (char*)pToken);*/
	pToken += comment_len;
	continue;
      }
      curOpcode = vshader_program_get_opcode(*pToken);
      ++pToken;
      if (NULL == curOpcode) {
	/* unknown current opcode ... */
	while (*pToken & 0x80000000) {
	  TRACE_(d3d_hw_shader)("unrecognized opcode: %08lx\n", *pToken);
	  ++pToken;
	}
      } else {
        /* Build opcode for GL vertex_program */
        switch (curOpcode->opcode) {
        case D3DSIO_NOP: 
            continue;
        case D3DSIO_MOV:
	    /* Address registers must be loaded with the ARL instruction */
	    if (((*pToken) & D3DSP_REGTYPE_MASK) == D3DSPR_ADDR) {
		if (0 < nUseAddressRegister) {
		    strcpy(tmpLine, "ARL");
		    break;
		} else
		   FIXME_(d3d_hw_shader)("Try to load an undeclared address register!\n");
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
            strcpy(tmpLine, curOpcode->name); 
            break;

        case D3DSIO_EXPP:
            strcpy(tmpLine, "EXP"); 
            break;
        case D3DSIO_LOGP:
            strcpy(tmpLine, "LOG"); 
            break;
        case D3DSIO_EXP: 
            strcpy(tmpLine, "EX2"); 
            break;
        case D3DSIO_LOG: 
            strcpy(tmpLine, "LG2"); 
            break;

        case D3DSIO_M4x4:
        case D3DSIO_M4x3:
        case D3DSIO_M3x4:
        case D3DSIO_M3x3:
        case D3DSIO_M3x2:
            /* Expand the macro and get number of generated instruction */
            nRemInstr = ExpandMxMacro(curOpcode->opcode, pToken);
            /* Save point to next instruction */
            pSavedToken = pToken + 3;
            /* Execute expanded macro */
            pToken = MacroExpansion;
            continue;

        default:
            FIXME_(d3d_hw_shader)("Can't handle opcode %s in hwShader\n", curOpcode->name);
        }

	if (curOpcode->num_params > 0) {
	  vshader_program_add_param(*pToken, 0, tmpLine);
          
	  ++pToken;
	  for (i = 1; i < curOpcode->num_params; ++i) {
            strcat(tmpLine, ",");
	    vshader_program_add_param(*pToken, 1, tmpLine);
	    ++pToken;
	  }
	}
        strcat(tmpLine,";\n");
	++lineNum;
        TRACE_(d3d_hw_shader)("GL HW (%u, %u) : %s", lineNum, strlen(pgmStr), tmpLine); /* Don't add \n to this line as already in tmpLine */
        strcat(pgmStr, tmpLine);
      }
    }
    strcpy(tmpLine, "END\n"); 
    ++lineNum;
    TRACE_(d3d_hw_shader)("GL HW (%u, %u) : %s", lineNum, strlen(pgmStr), tmpLine); /* Don't add \n to this line as already in tmpLine */
    strcat(pgmStr, tmpLine);
  }
  
  /*  Create the hw shader */
  GL_EXTCALL(glGenProgramsARB(1, &vshader->prgId));
  TRACE_(d3d_hw_shader)("Creating a hw vertex shader, prg=%d\n", vshader->prgId);

  GL_EXTCALL(glBindProgramARB(GL_VERTEX_PROGRAM_ARB, vshader->prgId));

  /* Create the program and check for errors */
  GL_EXTCALL(glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, strlen(pgmStr), pgmStr));
  if (glGetError() == GL_INVALID_OPERATION) {
    GLint errPos;
    glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errPos);
    FIXME_(d3d_hw_shader)("HW VertexShader Error at position: %d\n%s\n", errPos, glGetString(GL_PROGRAM_ERROR_STRING_ARB));
    vshader->prgId = -1;
  }
  
  HeapFree(GetProcessHeap(), 0, pgmStr);
}

inline static VOID IDirect3DVertexShaderImpl_ParseProgram(IDirect3DVertexShaderImpl* vshader, CONST DWORD* pFunction, int useHW) {
  const DWORD* pToken = pFunction;
  const SHADER_OPCODE* curOpcode = NULL;
  DWORD len = 0;  
  DWORD i;

  if (NULL != pToken) {
    while (D3DVS_END() != *pToken) {
      if (vshader_is_version_token(*pToken)) { /** version */
	TRACE("vs.%lu.%lu\n", (*pToken >> 8) & 0x0F, (*pToken & 0x0F));
	++pToken;
	++len;
	continue;
      } 
      if (vshader_is_comment_token(*pToken)) { /** comment */
	DWORD comment_len = (*pToken & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;
	++pToken;
	/*TRACE("comment[%ld] ;%s\n", comment_len, (char*)pToken);*/
	pToken += comment_len;
	len += comment_len + 1;
	continue;
      }
      curOpcode = vshader_program_get_opcode(*pToken);
      ++pToken;
      ++len;
      if (NULL == curOpcode) {
	/* unknown current opcode ... */
	while (*pToken & 0x80000000) {
	  TRACE("unrecognized opcode: %08lx\n", *pToken);
	  ++pToken;
	  ++len;
	}
      } else {
	TRACE("%s ", curOpcode->name);
	if (curOpcode->num_params > 0) {
	  vshader_program_dump_param(*pToken, 0);
	  ++pToken;
	  ++len;
	  for (i = 1; i < curOpcode->num_params; ++i) {
	    TRACE(", ");
	    vshader_program_dump_param(*pToken, 1);
	    ++pToken;
	    ++len;
	  }
	}
	TRACE("\n");
      }
    }
    vshader->functionLength = (len + 1) * sizeof(DWORD);
  } else {
    vshader->functionLength = 1; /* no Function defined use fixed function vertex processing */
  }

  /* Generate HW shader in needed */
  if (useHW && NULL != pFunction) {
    IDirect3DVertexShaderImpl_GenerateProgramArbHW(vshader, pFunction);
  }

  /* copy the function ... because it will certainly be released by application */
  if (NULL != pFunction) {
    vshader->function = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, vshader->functionLength);
    memcpy(vshader->function, pFunction, vshader->functionLength);
  } else {
    vshader->function = NULL;
  }
}

HRESULT WINAPI IDirect3DDeviceImpl_CreateVertexShader(IDirect3DDevice8Impl* This, CONST DWORD* pFunction, DWORD Usage, IDirect3DVertexShaderImpl** ppVertexShader) {
  IDirect3DVertexShaderImpl* object;
  int useHW;
  
  object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DVertexShaderImpl));
  if (NULL == object) {
    *ppVertexShader = NULL;
    return D3DERR_OUTOFVIDEOMEMORY;
  }
  /*object->lpVtbl = &Direct3DVextexShader9_Vtbl;*/
  object->device = This; /* FIXME: AddRef(This) */
  object->ref = 1;
  
  object->usage = Usage;
  object->data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(VSHADERDATA8));

  useHW = (((vs_mode == VS_HW) && GL_SUPPORT(ARB_VERTEX_PROGRAM)) &&
            This->devType != D3DDEVTYPE_REF &&
            object->usage != D3DUSAGE_SOFTWAREPROCESSING);
  
  IDirect3DVertexShaderImpl_ParseProgram(object, pFunction, useHW);

  *ppVertexShader = object;
  return D3D_OK;
}

BOOL IDirect3DVertexShaderImpl_ExecuteHAL(IDirect3DVertexShaderImpl* vshader, VSHADERINPUTDATA8* input, VSHADEROUTPUTDATA8* output) {
  /** 
   * TODO: use the NV_vertex_program (or 1_1) extension 
   *  and specifics vendors (ARB_vertex_program??) variants for it 
   */
  return TRUE;
}

HRESULT WINAPI IDirect3DVertexShaderImpl_ExecuteSW(IDirect3DVertexShaderImpl* vshader, VSHADERINPUTDATA8* input, VSHADEROUTPUTDATA8* output) {
  /** Vertex Shader Temporary Registers */
  D3DSHADERVECTOR R[12];
  /*D3DSHADERSCALAR A0;*/
  D3DSHADERVECTOR A[1];
  /** temporary Vector for modifier management */
  D3DSHADERVECTOR d;
  D3DSHADERVECTOR s[3];
  /** parser datas */
  const DWORD* pToken = vshader->function;
  const SHADER_OPCODE* curOpcode = NULL;
  /** functions parameters */
  D3DSHADERVECTOR* p[5];
  D3DSHADERVECTOR* p_send[5];
  DWORD i;

  /** init temporary register */
  memset(R, 0, 12 * sizeof(D3DSHADERVECTOR));

  /* vshader_program_parse(vshader); */
#if 0 /* Must not be 1 in cvs */
  TRACE("Input:\n");
  TRACE_VSVECTOR(vshader->data->C[0]);
  TRACE_VSVECTOR(vshader->data->C[1]);
  TRACE_VSVECTOR(vshader->data->C[2]);
  TRACE_VSVECTOR(vshader->data->C[3]);
  TRACE_VSVECTOR(vshader->data->C[4]);
  TRACE_VSVECTOR(vshader->data->C[5]);
  TRACE_VSVECTOR(vshader->data->C[6]);
  TRACE_VSVECTOR(vshader->data->C[7]);
  TRACE_VSVECTOR(vshader->data->C[8]);
  TRACE_VSVECTOR(vshader->data->C[64]);
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
    curOpcode = vshader_program_get_opcode(*pToken);
    ++pToken;
    if (NULL == curOpcode) {
      i = 0;
      /* unknown current opcode ... */
      while (*pToken & 0x80000000) {
	if (i == 0) {
	  TRACE("unrecognized opcode: pos=%d token=%08lX\n", (pToken - 1) - vshader->function, *(pToken - 1));
	}
	TRACE("unrecognized opcode param: pos=%d token=%08lX what=", pToken - vshader->function, *pToken);
	vshader_program_add_param(*pToken, i, NULL); /* Add function just used for trace error scenario */
	TRACE("\n");
	++i;
	++pToken;
      }
      /*return FALSE;*/
    } else {     
      if (curOpcode->num_params > 0) {	
	/*TRACE(">> execting opcode: pos=%d opcode_name=%s token=%08lX\n", pToken - vshader->function, curOpcode->name, *pToken);*/
	for (i = 0; i < curOpcode->num_params; ++i) {
	  DWORD reg = pToken[i] & 0x00001FFF;
	  DWORD regtype = ((pToken[i] & D3DSP_REGTYPE_MASK) >> D3DSP_REGTYPE_SHIFT);

	  switch (regtype << D3DSP_REGTYPE_SHIFT) {
	  case D3DSPR_TEMP:
	    /*TRACE("p[%d]=R[%d]\n", i, reg);*/
	    p[i] = &R[reg];
	    break;
	  case D3DSPR_INPUT:
	    /*TRACE("p[%d]=V[%s]\n", i, VertexShaderDeclRegister[reg]);*/
	    p[i] = &input->V[reg];
	    break;
	  case D3DSPR_CONST:
	    if (pToken[i] & D3DVS_ADDRMODE_RELATIVE) {
	      p[i] = &vshader->data->C[(DWORD) A[0].x + reg];
	    } else {
	      p[i] = &vshader->data->C[reg];
	    }
	    break;
	  case D3DSPR_ADDR: /*case D3DSPR_TEXTURE:*/
	    if (0 != reg) {
	      ERR("cannot handle address registers != a0, forcing use of a0\n");
	      reg = 0;
	    }
	    /*TRACE("p[%d]=A[%d]\n", i, reg);*/
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
	    /*TRACE("p[%d]=oD[%d]\n", i, reg);*/
	    p[i] = &output->oD[reg];
	    break;
	  case D3DSPR_TEXCRDOUT:
	    /*TRACE("p[%d]=oT[%d]\n", i, reg);*/
	    p[i] = &output->oT[reg];
	    break;
	  default:
	    break;
	  }
	  
	  if (i > 0) { /* input reg */
	    DWORD swizzle = (pToken[i] & D3DVS_SWIZZLE_MASK) >> D3DVS_SWIZZLE_SHIFT;
	    UINT isNegative = ((pToken[i] & D3DSP_SRCMOD_MASK) == D3DSPSM_NEG);

	    if (!isNegative && (D3DVS_NOSWIZZLE >> D3DVS_SWIZZLE_SHIFT) == swizzle) {
	      /*TRACE("p[%d] not swizzled\n", i);*/
	      p_send[i] = p[i];
	    } else {
	      DWORD swizzle_x = swizzle & 0x03;
	      DWORD swizzle_y = (swizzle >> 2) & 0x03;
	      DWORD swizzle_z = (swizzle >> 4) & 0x03;
	      DWORD swizzle_w = (swizzle >> 6) & 0x03;
	      /*TRACE("p[%d] swizzled\n", i);*/
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
  return D3D_OK;
}

HRESULT WINAPI IDirect3DVertexShaderImpl_GetFunction(IDirect3DVertexShaderImpl* This, VOID* pData, UINT* pSizeOfData) {
  if (NULL == pData) {
    *pSizeOfData = This->functionLength;
    return D3D_OK;
  }
  if (*pSizeOfData < This->functionLength) {
    *pSizeOfData = This->functionLength;
    return D3DERR_MOREDATA;
  }
  if (NULL == This->function) { /* no function defined */
    TRACE("(%p) : GetFunction no User Function defined using NULL to %p\n", This, pData);
    (*(DWORD **) pData) = NULL;
  } else {
    TRACE("(%p) : GetFunction copying to %p\n", This, pData);
    memcpy(pData, This->function, This->functionLength);
  }
  return D3D_OK;
}

HRESULT WINAPI IDirect3DVertexShaderImpl_SetConstantF(IDirect3DVertexShaderImpl* This, UINT StartRegister, CONST FLOAT* pConstantData, UINT Vector4fCount) {
  if (StartRegister + Vector4fCount > D3D8_VSHADER_MAX_CONSTANTS) {
    return D3DERR_INVALIDCALL;
  }
  if (NULL == This->data) { /* temporary while datas not supported */
    FIXME("(%p) : VertexShader_SetConstant not fully supported yet\n", This);
    return D3DERR_INVALIDCALL;
  }
  memcpy(&This->data->C[StartRegister], pConstantData, Vector4fCount * 4 * sizeof(FLOAT));
  return D3D_OK;
}

HRESULT WINAPI IDirect3DVertexShaderImpl_GetConstantF(IDirect3DVertexShaderImpl* This, UINT StartRegister, FLOAT* pConstantData, UINT Vector4fCount) {
  if (StartRegister + Vector4fCount > D3D8_VSHADER_MAX_CONSTANTS) {
    return D3DERR_INVALIDCALL;
  }
  if (NULL == This->data) { /* temporary while datas not supported */
    return D3DERR_INVALIDCALL;
  }
  memcpy(pConstantData, &This->data->C[StartRegister], Vector4fCount * 4 * sizeof(FLOAT));
  return D3D_OK;
}


/**********************************************************************************************************************************************
 **********************************************************************************************************************************************
 **********************************************************************************************************************************************
 **********************************************************************************************************************************************
 **********************************************************************************************************************************************/

void pshader_texcoord(D3DSHADERVECTOR* d) {
}

void pshader_texkill(D3DSHADERVECTOR* d) {
}

void pshader_tex(D3DSHADERVECTOR* d) {
}

void pshader_texbem(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0) {
}

void pshader_texbeml(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0) {
}

void pshader_texreg2ar(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0) {
}

void pshader_texreg2gb(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0) {
}

void pshader_texm3x2pad(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0) {
}

void pshader_texm3x2tex(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0) {
}

void pshader_texm3x3pad(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0) {
}

void pshader_texm3x3tex(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0) {
}

void pshader_texm3x3diff(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0) {
}

void pshader_texm3x3spec(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0, D3DSHADERVECTOR* s1) {
}

void pshader_texm3x3vspec(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0) {
}

void pshader_cnd(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0, D3DSHADERVECTOR* s1, D3DSHADERVECTOR* s2) {
}

void pshader_def(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0, D3DSHADERVECTOR* s1, D3DSHADERVECTOR* s2, D3DSHADERVECTOR* s3) {
}

void pshader_texreg2rgb(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0) {
}

void pshader_texdp3tex(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0) {
}

void pshader_texm3x2depth(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0) {
}

void pshader_texdp3(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0) {
}

void pshader_texm3x3(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0) {
}

void pshader_texdepth(D3DSHADERVECTOR* d) {
}

void pshader_cmp(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0, D3DSHADERVECTOR* s1, D3DSHADERVECTOR* s2) {
}

void pshader_bem(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0, D3DSHADERVECTOR* s1) {
}

static CONST SHADER_OPCODE pshader_ins [] = {
  {D3DSIO_NOP,  "nop",  0, vshader_nop, 0, 0},
  {D3DSIO_MOV,  "mov",  2, vshader_mov, 0, 0},
  {D3DSIO_ADD,  "add",  3, vshader_add, 0, 0},
  {D3DSIO_SUB,  "sub",  3, vshader_sub, 0, 0},
  {D3DSIO_MAD,  "mad",  4, vshader_mad, 0, 0},
  {D3DSIO_MUL,  "mul",  3, vshader_mul, 0, 0},
  {D3DSIO_RCP,  "rcp",  2, vshader_rcp, 0, 0},
  {D3DSIO_RSQ,  "rsq",  2, vshader_rsq, 0, 0},
  {D3DSIO_DP3,  "dp3",  3, vshader_dp3, 0, 0},
  {D3DSIO_DP4,  "dp4",  3, vshader_dp4, 0, 0},
  {D3DSIO_MIN,  "min",  3, vshader_min, 0, 0},
  {D3DSIO_MAX,  "max",  3, vshader_max, 0, 0},
  {D3DSIO_SLT,  "slt",  3, vshader_slt, 0, 0},
  {D3DSIO_SGE,  "sge",  3, vshader_sge, 0, 0},
  {D3DSIO_EXP,  "exp",  2, vshader_exp, 0, 0},
  {D3DSIO_LOG,  "log",  2, vshader_log, 0, 0},
  {D3DSIO_LIT,  "lit",  2, vshader_lit, 0, 0},
  {D3DSIO_DST,  "dst",  3, vshader_dst, 0, 0},
  {D3DSIO_LRP,  "lrp",  4, vshader_lrp, 0, 0},
  {D3DSIO_FRC,  "frc",  2, vshader_frc, 0, 0},
  {D3DSIO_M4x4, "m4x4", 3, vshader_m4x4, 0, 0},
  {D3DSIO_M4x3, "m4x3", 3, vshader_m4x3, 0, 0},
  {D3DSIO_M3x4, "m3x4", 3, vshader_m3x4, 0, 0},
  {D3DSIO_M3x3, "m3x3", 3, vshader_m3x3, 0, 0},
  {D3DSIO_M3x2, "m3x2", 3, vshader_m3x2, 0, 0},

  {D3DSIO_TEXCOORD,     "texcoord",     1, pshader_texcoord,     D3DPS_VERSION(1,0), D3DPS_VERSION(1,3)},
  {D3DSIO_TEXCOORD,     "texcrd",       2, pshader_texcoord,     D3DPS_VERSION(1,4), D3DPS_VERSION(1,4)},
  {D3DSIO_TEXKILL,      "texkill",      1, pshader_texkill,      D3DPS_VERSION(1,0), D3DPS_VERSION(1,4)},
  {D3DSIO_TEX,          "tex",          1, pshader_tex,          D3DPS_VERSION(1,0), D3DPS_VERSION(1,3)}, 
  {D3DSIO_TEX,          "texld",        2, pshader_tex,          D3DPS_VERSION(1,4), D3DPS_VERSION(1,4)}, 
  {D3DSIO_TEXBEM,       "texbem",       2, pshader_texbem,       D3DPS_VERSION(1,0), D3DPS_VERSION(1,3)}, 
  {D3DSIO_TEXBEML,      "texbeml",      2, pshader_texbeml,      D3DPS_VERSION(1,0), D3DPS_VERSION(1,3)}, 
  {D3DSIO_TEXREG2AR,    "texreg2ar",    2, pshader_texreg2ar,    D3DPS_VERSION(1,1), D3DPS_VERSION(1,3)}, 
  {D3DSIO_TEXREG2GB,    "texreg2gb",    2, pshader_texreg2gb,    D3DPS_VERSION(1,2), D3DPS_VERSION(1,3)}, 
  {D3DSIO_TEXM3x2PAD,   "texm3x2pad",   2, pshader_texm3x2pad,   D3DPS_VERSION(1,0), D3DPS_VERSION(1,3)}, 
  {D3DSIO_TEXM3x2TEX,   "texm3x2tex",   2, pshader_texm3x2tex,   D3DPS_VERSION(1,0), D3DPS_VERSION(1,3)}, 
  {D3DSIO_TEXM3x3PAD,   "texm3x3pad",   2, pshader_texm3x3pad,   D3DPS_VERSION(1,0), D3DPS_VERSION(1,3)}, 
  {D3DSIO_TEXM3x3TEX,   "texm3x3tex",   2, pshader_texm3x3tex,   D3DPS_VERSION(1,0), D3DPS_VERSION(1,3)}, 
  {D3DSIO_TEXM3x3DIFF,  "texm3x3diff",  2, pshader_texm3x3diff,  D3DPS_VERSION(0,0), D3DPS_VERSION(0,0)}, 
  {D3DSIO_TEXM3x3SPEC,  "texm3x3spec",  3, pshader_texm3x3spec,  D3DPS_VERSION(1,0), D3DPS_VERSION(1,3)}, 
  {D3DSIO_TEXM3x3VSPEC, "texm3x3vspec", 2, pshader_texm3x3vspec, D3DPS_VERSION(1,0), D3DPS_VERSION(1,3)}, 

  {D3DSIO_EXPP,         "expp",         2, vshader_expp, 0, 0},
  {D3DSIO_LOGP,         "logp",         2, vshader_logp, 0, 0},

  {D3DSIO_CND,          "cnd",          4, pshader_cnd,          D3DPS_VERSION(1,1), D3DPS_VERSION(1,4)},
  {D3DSIO_DEF,          "def",          5, pshader_def,          D3DPS_VERSION(1,0), D3DPS_VERSION(3,0)},
  {D3DSIO_TEXREG2RGB,   "texbreg2rgb",  2, pshader_texreg2rgb,   D3DPS_VERSION(1,2), D3DPS_VERSION(1,3)}, 
  
  {D3DSIO_TEXDP3TEX,    "texdp3tex",    2, pshader_texdp3tex,    D3DPS_VERSION(1,2), D3DPS_VERSION(1,3)}, 
  {D3DSIO_TEXM3x2DEPTH, "texm3x2depth", 2, pshader_texm3x2depth, D3DPS_VERSION(1,3), D3DPS_VERSION(1,3)}, 
  {D3DSIO_TEXDP3,       "texdp3",       2, pshader_texdp3,       D3DPS_VERSION(1,2), D3DPS_VERSION(1,3)}, 
  {D3DSIO_TEXM3x3,      "texm3x3",      2, pshader_texm3x3,      D3DPS_VERSION(1,2), D3DPS_VERSION(1,3)},
  {D3DSIO_TEXDEPTH,     "texdepth",     1, pshader_texdepth,     D3DPS_VERSION(1,4), D3DPS_VERSION(1,4)},
  {D3DSIO_CMP,          "cmp",          4, pshader_cmp,          D3DPS_VERSION(1,1), D3DPS_VERSION(3,0)}, 
  {D3DSIO_BEM,          "bem",          3, pshader_bem,          D3DPS_VERSION(1,4), D3DPS_VERSION(1,4)}, 

  {D3DSIO_PHASE,  "phase",  0, vshader_nop, 0, 0},

  {0, NULL, 0, NULL}
};

inline static const SHADER_OPCODE* pshader_program_get_opcode(const DWORD code, const int version) {
  DWORD i = 0;
  DWORD hex_version = D3DPS_VERSION(version/10, version%10);
  /** TODO: use dichotomic search */
  while (NULL != pshader_ins[i].name) {
    if ( ( (code & D3DSI_OPCODE_MASK) == pshader_ins[i].opcode) &&
         ( ( (hex_version >= pshader_ins[i].min_version) && (hex_version <= pshader_ins[i].max_version)) ||
	   ( (pshader_ins[i].min_version == 0) && (pshader_ins[i].max_version == 0) ) ) ) {
      return &pshader_ins[i];
    }
    ++i;
  }
  return NULL;
}

inline static BOOL pshader_is_version_token(DWORD token) {
  return 0xFFFF0000 == (token & 0xFFFF0000);
}

inline static BOOL pshader_is_comment_token(DWORD token) {
  return D3DSIO_COMMENT == (token & D3DSI_OPCODE_MASK);
}

inline static void pshader_program_dump_opcode(const SHADER_OPCODE* curOpcode, const DWORD code, const DWORD output) {
  if (0 != (code & ~D3DSI_OPCODE_MASK)) {
    DWORD mask = (code & ~D3DSI_OPCODE_MASK);
    switch (mask) {
    case 0x40000000:    TRACE("+"); break;
    default:
      TRACE(" unhandled modifier(0x%08lx) ", mask);
    }
  }
  TRACE("%s", curOpcode->name);
  /**
   * normally this is a destination reg modifier 
   * but in pixel shaders asm code its specified as:
   *  dp3_x4 t1.rgba, r1, c1
   * or
   *  dp3_x2_sat r0, t0_bx2, v0_bx2
   * so for better debbuging i use the same norm
   */
  if (0 != (output & D3DSP_DSTSHIFT_MASK)) {
    DWORD shift = (output & D3DSP_DSTSHIFT_MASK) >> D3DSP_DSTSHIFT_SHIFT;
    if (shift < 8) {
      TRACE("_x%u", 1 << shift);
    } else {
      TRACE("_d%u", 1 << (16-shift));
    }
  }
  if (0 != (output & D3DSP_DSTMOD_MASK)) {
    DWORD mask = output & D3DSP_DSTMOD_MASK;
    switch (mask) {
    case D3DSPDM_SATURATE: TRACE("_sat"); break;
    default:
      TRACE("_unhandled_modifier(0x%08lx)", mask);
    }
  }
  TRACE(" ");
}

inline static void pshader_program_dump_param(const DWORD param, int input) {
  static const char* rastout_reg_names[] = { "oC0", "oC1", "oC2", "oC3", "oDepth" };
  static const char swizzle_reg_chars[] = "rgba";

  DWORD reg = param & 0x00001FFF;
  DWORD regtype = ((param & D3DSP_REGTYPE_MASK) >> D3DSP_REGTYPE_SHIFT);

  if (input) {
    if ( ((param & D3DSP_SRCMOD_MASK) == D3DSPSM_NEG) ||
         ((param & D3DSP_SRCMOD_MASK) == D3DSPSM_BIASNEG) ||
         ((param & D3DSP_SRCMOD_MASK) == D3DSPSM_SIGNNEG) ||
         ((param & D3DSP_SRCMOD_MASK) == D3DSPSM_X2NEG) )
      TRACE("-");
    else if ((param & D3DSP_SRCMOD_MASK) == D3DSPSM_COMP)
      TRACE("1-");
  }
  
  switch (regtype << D3DSP_REGTYPE_SHIFT) {
  case D3DSPR_TEMP:
    TRACE("R[%lu]", reg);
    break;
  case D3DSPR_INPUT:
    TRACE("V[%lu]", reg);
    break;
  case D3DSPR_CONST:
    TRACE("C[%s%lu]", (param & D3DVS_ADDRMODE_RELATIVE) ? "a0.x + " : "", reg);
    break;
  case D3DSPR_TEXTURE: /* case D3DSPR_ADDR: */
    TRACE("t[%lu]", reg);
    break;
  case D3DSPR_RASTOUT:
    TRACE("%s", rastout_reg_names[reg]);
    break;
  case D3DSPR_ATTROUT:
    TRACE("oD[%lu]", reg);
    break;
  case D3DSPR_TEXCRDOUT:
    TRACE("oT[%lu]", reg);
    break;
  default:
    break;
  }

  if (!input) {
    /** operand output */
    /**
     * for better debugging traces it's done into opcode dump code
     * @see pshader_program_dump_opcode
    if (0 != (param & D3DSP_DSTMOD_MASK)) {
      DWORD mask = param & D3DSP_DSTMOD_MASK;
      switch (mask) {
      case D3DSPDM_SATURATE: TRACE("_sat"); break;
      default:
      TRACE("_unhandled_modifier(0x%08lx)", mask);
      }
    }
    if (0 != (param & D3DSP_DSTSHIFT_MASK)) {
      DWORD shift = (param & D3DSP_DSTSHIFT_MASK) >> D3DSP_DSTSHIFT_SHIFT;
      if (shift > 0) {
	TRACE("_x%u", 1 << shift);
      }
    }
    */
    if ((param & D3DSP_WRITEMASK_ALL) != D3DSP_WRITEMASK_ALL) {
      TRACE(".");
      if (param & D3DSP_WRITEMASK_0) TRACE("r");
      if (param & D3DSP_WRITEMASK_1) TRACE("g");
      if (param & D3DSP_WRITEMASK_2) TRACE("b");
      if (param & D3DSP_WRITEMASK_3) TRACE("a");
    }
  } else {
    /** operand input */
    DWORD swizzle = (param & D3DSP_SWIZZLE_MASK) >> D3DSP_SWIZZLE_SHIFT;
    DWORD swizzle_x = swizzle & 0x03;
    DWORD swizzle_y = (swizzle >> 2) & 0x03;
    DWORD swizzle_z = (swizzle >> 4) & 0x03;
    DWORD swizzle_w = (swizzle >> 6) & 0x03;

    if (0 != (param & D3DSP_SRCMOD_MASK)) {
      DWORD mask = param & D3DSP_SRCMOD_MASK;
      /*TRACE("_modifier(0x%08lx) ", mask);*/
      switch (mask) {
      case D3DSPSM_NONE:    break;
      case D3DSPSM_NEG:     break;
      case D3DSPSM_BIAS:    TRACE("_bias"); break;
      case D3DSPSM_BIASNEG: TRACE("_bias"); break;
      case D3DSPSM_SIGN:    TRACE("_bx2"); break;
      case D3DSPSM_SIGNNEG: TRACE("_bx2"); break;
      case D3DSPSM_COMP:    break;
      case D3DSPSM_X2:      TRACE("_x2"); break;
      case D3DSPSM_X2NEG:   TRACE("_x2"); break;
      case D3DSPSM_DZ:      TRACE("_dz"); break;
      case D3DSPSM_DW:      TRACE("_dw"); break;
      default:
	TRACE("_unknown(0x%08lx)", mask);
      }
    }

    /**
     * swizzle bits fields:
     *  WWZZYYXX
     */
    if ((D3DSP_NOSWIZZLE >> D3DSP_SWIZZLE_SHIFT) != swizzle) { /* ! D3DVS_NOSWIZZLE == 0xE4 << D3DVS_SWIZZLE_SHIFT */
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

static int constants[D3D8_PSHADER_MAX_CONSTANTS];

inline static void get_register_name(const DWORD param, char* regstr)
{
  static const char* rastout_reg_names[] = { "oC0", "oC1", "oC2", "oC3", "oDepth" };

  DWORD reg = param & 0x00001FFF;
  DWORD regtype = ((param & D3DSP_REGTYPE_MASK) >> D3DSP_REGTYPE_SHIFT);

  switch (regtype << D3DSP_REGTYPE_SHIFT) {
  case D3DSPR_TEMP:
    sprintf(regstr, "R%lu", reg);
    break;
  case D3DSPR_INPUT:
    if (reg==0) {
       strcpy(regstr, "fragment.color.primary");
    } else {
       strcpy(regstr, "fragment.color.secondary");
    }
    break;
  case D3DSPR_CONST:
    if (constants[reg])
      sprintf(regstr, "C%lu", reg);
    else
      sprintf(regstr, "program.env[%lu]", reg);
    break;
  case D3DSPR_TEXTURE: /* case D3DSPR_ADDR: */
    sprintf(regstr,"T%lu", reg);
    break;
  case D3DSPR_RASTOUT:
    sprintf(regstr, "%s", rastout_reg_names[reg]);
    break;
  case D3DSPR_ATTROUT:
    sprintf(regstr, "oD[%lu]", reg);
    break;
  case D3DSPR_TEXCRDOUT:
    sprintf(regstr, "oT[%lu]", reg);
    break;
  default:
    break;
  }
}	

inline static void addline(unsigned int* lineNum, char* pgm, char* line)
{
  ++(*lineNum);
  TRACE_(d3d_hw_shader)("GL HW (%u, %u) : %s\n", *lineNum, strlen(pgm), line);
  strcat(pgm, line);
  strcat(pgm, "\n");
}

static const char* shift_tab[] = {
  "dummy",     /*  0 (none) */ 
  "coefmul.x", /*  1 (x2)   */ 
  "coefmul.y", /*  2 (x4)   */ 
  "coefmul.z", /*  3 (x8)   */ 
  "coefmul.w", /*  4 (x16)  */ 
  "dummy",     /*  5 (x32)  */ 
  "dummy",     /*  6 (x64)  */ 
  "dummy",     /*  7 (x128) */ 
  "dummy",     /*  8 (d256) */ 
  "dummy",     /*  9 (d128) */ 
  "dummy",     /* 10 (d64)  */ 
  "dummy",     /* 11 (d32)  */ 
  "coefdiv.w", /* 12 (d16)  */ 
  "coefdiv.z", /* 13 (d8)   */ 
  "coefdiv.y", /* 14 (d4)   */ 
  "coefdiv.x"  /* 15 (d2)   */ 
};

inline static void get_write_mask(const DWORD output_reg, char* write_mask) 
{
  *write_mask = 0;
  if ((output_reg & D3DSP_WRITEMASK_ALL) != D3DSP_WRITEMASK_ALL) {
    strcat(write_mask, ".");
    if (output_reg & D3DSP_WRITEMASK_0) strcat(write_mask, "r");
    if (output_reg & D3DSP_WRITEMASK_1) strcat(write_mask, "g");
    if (output_reg & D3DSP_WRITEMASK_2) strcat(write_mask, "b");
    if (output_reg & D3DSP_WRITEMASK_3) strcat(write_mask, "a");
  }
}

inline static void get_input_register_swizzle(const DWORD instr, char* swzstring) 
{
    static const char swizzle_reg_chars[] = "rgba";
    DWORD swizzle = (instr & D3DSP_SWIZZLE_MASK) >> D3DSP_SWIZZLE_SHIFT;
    DWORD swizzle_x = swizzle & 0x03;
    DWORD swizzle_y = (swizzle >> 2) & 0x03;
    DWORD swizzle_z = (swizzle >> 4) & 0x03;
    DWORD swizzle_w = (swizzle >> 6) & 0x03;
    /**
     * swizzle bits fields:
     *  WWZZYYXX
     */
    *swzstring = 0;
    if ((D3DSP_NOSWIZZLE >> D3DSP_SWIZZLE_SHIFT) != swizzle) { /* ! D3DVS_NOSWIZZLE == 0xE4 << D3DVS_SWIZZLE_SHIFT */
      if (swizzle_x == swizzle_y && 
	  swizzle_x == swizzle_z && 
	  swizzle_x == swizzle_w) {
	sprintf(swzstring, ".%c", swizzle_reg_chars[swizzle_x]);
      } else {
	sprintf(swzstring, ".%c%c%c%c", 
	      swizzle_reg_chars[swizzle_x], 
	      swizzle_reg_chars[swizzle_y], 
	      swizzle_reg_chars[swizzle_z], 
	      swizzle_reg_chars[swizzle_w]);
      }
    }
}

inline static void gen_output_modifier_line(int saturate, char* write_mask, int shift, char *regstr, char* line)
{
  /* Generate a line that does the output modifier computation */
  sprintf(line, "MUL%s %s%s, %s, %s;", saturate ? "_SAT" : "", regstr, write_mask, regstr, shift_tab[shift]);
}

inline static int gen_input_modifier_line(const DWORD instr, int tmpreg, char* outregstr, char* line)
{
  /* Generate a line that does the input modifier computation and return the input register to use */
  static char regstr[256];
  static char tmpline[256];
  int insert_line;
 
  /* Assume a new line will be added */
  insert_line = 1;
  
  /* Get register name */
  get_register_name(instr, regstr);
  
  switch (instr & D3DSP_SRCMOD_MASK) {
    case D3DSPSM_NONE:
      strcpy(outregstr, regstr);
      insert_line = 0;
      break;
    case D3DSPSM_NEG:
      sprintf(outregstr, "-%s", regstr);
      insert_line = 0;
      break;
    case D3DSPSM_BIAS:
      sprintf(line, "ADD T%c, %s, -coefdiv.x;", 'A' + tmpreg, regstr);
      break;
    case D3DSPSM_BIASNEG:
      sprintf(line, "ADD T%c, -%s, coefdiv.x;", 'A' + tmpreg, regstr);
      break;
    case D3DSPSM_SIGN:
      sprintf(line, "MAD T%c, %s, coefmul.x, -one.x;", 'A' + tmpreg, regstr);
      break;
    case D3DSPSM_SIGNNEG:
      sprintf(line, "MAD T%c, %s, -coefmul.x, one.x;", 'A' + tmpreg, regstr);
      break;
    case D3DSPSM_COMP:
      sprintf(line, "SUB T%c, one.x, %s;", 'A' + tmpreg, regstr);
      break;
    case D3DSPSM_X2:
      sprintf(line, "ADD T%c, %s, %s;", 'A' + tmpreg, regstr, regstr);
      break;
    case D3DSPSM_X2NEG:
      sprintf(line, "ADD T%c, -%s, -%s;", 'A' + tmpreg, regstr, regstr);
      break;
    case D3DSPSM_DZ:
      sprintf(line, "RCP T%c, %s.z;", 'A' + tmpreg, regstr);
      sprintf(tmpline, "MUL T%c, %s, T%c;", 'A' + tmpreg, regstr, 'A' + tmpreg);
      strcat(line, "\n"); /* Hack */
      strcat(line, tmpline);
      break;
    case D3DSPSM_DW:
      sprintf(line, "RCP T%c, %s;", 'A' + tmpreg, regstr);
      sprintf(tmpline, "MUL T%c, %s, T%c;", 'A' + tmpreg, regstr, 'A' + tmpreg);
      strcat(line, "\n"); /* Hack */
      strcat(line, tmpline);
      break;
    default:
      strcpy(outregstr, regstr);
      insert_line = 0;
  }

  if (insert_line) {
    /* Substitute the register name */
    sprintf(outregstr, "T%c", 'A' + tmpreg);
  }

  return insert_line;
}

/**
 * Pixel Shaders
 *
 * http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/reference/Shaders/PixelShader1_X/modifiers/sourceregistermodifiers.asp
 * http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/reference/Shaders/PixelShader2_0/Registers/Registers.asp
 * http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/reference/d3d/interfaces/IDirect3DPixelShader9/_IDirect3DPixelShader9.asp
 *
 */
inline static VOID IDirect3DPixelShaderImpl_GenerateProgramArbHW(IDirect3DPixelShaderImpl* pshader, CONST DWORD* pFunction) {
  const DWORD* pToken = pFunction;
  const SHADER_OPCODE* curOpcode = NULL;
  const DWORD* pInstr;
  DWORD code;
  DWORD i;
  int autoparam;
  unsigned lineNum = 0;
  char *pgmStr = NULL;
  char  tmpLine[255];
  BOOL saturate;
  int row = 0;
  DWORD tcw[2];
  IDirect3DDevice8Impl* This = pshader->device;
  int version = 0;

  for(i = 0; i < D3D8_PSHADER_MAX_CONSTANTS; i++)
    constants[i] = 0;
  
  pgmStr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 65535); /* 64kb should be enough */
  
  if (NULL != pToken) {
    while (D3DPS_END() != *pToken) { 
      if (pshader_is_version_token(*pToken)) { /** version */
        int numTemps;
        int numConstants;

        /* Extract version *10 into integer value (ie. 1.0 == 10, 1.1==11 etc */
        version = (((*pToken >> 8) & 0x0F) * 10) + (*pToken & 0x0F);

	TRACE_(d3d_hw_shader)("ps.%lu.%lu;\n", (*pToken >> 8) & 0x0F, (*pToken & 0x0F));

        /* Each release of pixel shaders has had different numbers of temp registers */
        switch (version) {
        case 10:
        case 11:
        case 12:
        case 13:
        case 14: numTemps=12;
                 numConstants=8;
                 strcpy(tmpLine, "!!ARBfp1.0");
                 break;
        case 20: numTemps=12;
                 numConstants=8;
                 strcpy(tmpLine, "!!ARBfp2.0");
                 FIXME_(d3d_hw_shader)("No work done yet to support ps2.0 in hw\n");
                 break;
        case 30: numTemps=32;
                 numConstants=8;
                 strcpy(tmpLine, "!!ARBfp3.0");
                 FIXME_(d3d_hw_shader)("No work done yet to support ps3.0 in hw\n");
                 break;
        default:
                 numTemps=12;
                 numConstants=8;
                 strcpy(tmpLine, "!!ARBfp1.0");
                 FIXME_(d3d_hw_shader)("Unrecognized pixel shader version!\n");
        }
        addline(&lineNum, pgmStr, tmpLine);

        for(i = 0; i < 6; i++) {
          sprintf(tmpLine, "TEMP T%lu;", i);
          addline(&lineNum, pgmStr, tmpLine);
        }
        for(i = 0; i < 6; i++) {
          sprintf(tmpLine, "TEMP R%lu;", i);
          addline(&lineNum, pgmStr, tmpLine);
        }

        sprintf(tmpLine, "TEMP TMP;");
        addline(&lineNum, pgmStr, tmpLine);
        sprintf(tmpLine, "TEMP TMP2;");
        addline(&lineNum, pgmStr, tmpLine);
        sprintf(tmpLine, "TEMP TA;");
        addline(&lineNum, pgmStr, tmpLine);
        sprintf(tmpLine, "TEMP TB;");
        addline(&lineNum, pgmStr, tmpLine);
        sprintf(tmpLine, "TEMP TC;");
        addline(&lineNum, pgmStr, tmpLine);

        strcpy(tmpLine, "PARAM coefdiv = { 0.5, 0.25, 0.125, 0.0625 };");
        addline(&lineNum, pgmStr, tmpLine);
        strcpy(tmpLine, "PARAM coefmul = { 2, 4, 8, 16 };");
        addline(&lineNum, pgmStr, tmpLine);
        strcpy(tmpLine, "PARAM one = { 1.0, 1.0, 1.0, 1.0 };");
        addline(&lineNum, pgmStr, tmpLine);

        for(i = 0; i < 4; i++) {
          sprintf(tmpLine, "MOV T%lu, fragment.texcoord[%lu];", i, i);
          addline(&lineNum, pgmStr, tmpLine);
        }

        ++pToken;
        continue;
      } 
      if (pshader_is_comment_token(*pToken)) { /** comment */
        DWORD comment_len = (*pToken & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;
        ++pToken;
        /*TRACE("comment[%ld] ;%s\n", comment_len, (char*)pToken);*/
        pToken += comment_len;
        continue;
      }
      code = *pToken;
      pInstr = pToken;
      curOpcode = pshader_program_get_opcode(code, version);
      ++pToken;
      if (NULL == curOpcode) {
        /* unknown current opcode ... */
        while (*pToken & 0x80000000) {
          TRACE("unrecognized opcode: %08lx\n", *pToken);
          ++pToken;
        }
      } else {
        autoparam = 1;
        saturate = FALSE;
        /* Build opcode for GL vertex_program */
        switch (curOpcode->opcode) {
        case D3DSIO_NOP: 
        case D3DSIO_PHASE: 
            continue;
        case D3DSIO_DEF:
            {
              DWORD reg = *pToken & 0x00001FFF;
              sprintf(tmpLine, "PARAM C%lu = { %f, %f, %f, %f };", reg,
                              *((const float*)(pToken+1)),
                              *((const float*)(pToken+2)),
                              *((const float*)(pToken+3)),
                              *((const float*)(pToken+4)) );
              addline(&lineNum, pgmStr, tmpLine);
              constants[reg] = 1;
              autoparam = 0;
              pToken+=5;
            }
            break;
        case D3DSIO_TEXKILL:
            strcpy(tmpLine, "KIL");
            break;
        case D3DSIO_TEX:
            {
              char tmp[20];
              get_write_mask(*pToken, tmp);
              if (version != 14) {
                DWORD reg = *pToken & 0x00001FFF;
                sprintf(tmpLine,"TEX T%lu%s, T%lu, texture[%lu], 2D;", reg, tmp, reg, reg);
                addline(&lineNum, pgmStr, tmpLine);
                autoparam = 0;
                pToken++;
              } else {
                char line[256];
                char reg[20];
                DWORD reg1 = *pToken & 0x00001FFF;
                DWORD reg2 = *(pToken+1) & 0x00001FFF;
                if (gen_input_modifier_line(*(pToken+1), 0, reg, line))
                  addline(&lineNum, pgmStr, line);
                sprintf(tmpLine,"TEX R%lu%s, %s, texture[%lu], 2D;", reg1, tmp, reg, reg2);
                addline(&lineNum, pgmStr, tmpLine);
                autoparam = 0;
                pToken += 2;
              }
            }
            break;
        case D3DSIO_TEXCOORD:
            {
              char tmp[20];
              get_write_mask(*pToken, tmp);
              if (version != 14) {
                DWORD reg = *pToken & 0x00001FFF;
                sprintf(tmpLine, "MOV T%lu%s, fragment.texcoord[%lu];", reg, tmp, reg);
                addline(&lineNum, pgmStr, tmpLine);
                autoparam = 0;
                pToken++;
              } else {
                DWORD reg1 = *pToken & 0x00001FFF;
                DWORD reg2 = *(pToken+1) & 0x00001FFF;
                sprintf(tmpLine, "MOV R%lu%s, fragment.texcoord[%lu];", reg1, tmp, reg2);
                addline(&lineNum, pgmStr, tmpLine);
                autoparam = 0;
                pToken += 2;
              }
            }
            break;
        case D3DSIO_TEXM3x2PAD:
            {
              DWORD reg = *pToken & 0x00001FFF;
              char buf[50];
              if (gen_input_modifier_line(*(pToken+1), 0, buf, tmpLine))
                addline(&lineNum, pgmStr, tmpLine);
              sprintf(tmpLine, "DP3 TMP.x, T%lu, %s;", reg, buf);
              addline(&lineNum, pgmStr, tmpLine);
              autoparam = 0;
              pToken += 2;
            }
            break;
        case D3DSIO_TEXM3x2TEX:
            {
              DWORD reg = *pToken & 0x00001FFF;
              char buf[50];
              if (gen_input_modifier_line(*(pToken+1), 0, buf, tmpLine))
                addline(&lineNum, pgmStr, tmpLine);
              sprintf(tmpLine, "DP3 TMP.y, T%lu, %s;", reg, buf);
              addline(&lineNum, pgmStr, tmpLine);
              sprintf(tmpLine, "TEX T%lu, TMP, texture[%lu], 2D;", reg, reg);
              addline(&lineNum, pgmStr, tmpLine);
              autoparam = 0;
              pToken += 2;
            }
            break;
	case D3DSIO_TEXREG2AR:
            {
              DWORD reg1 = *pToken & 0x00001FFF;
              DWORD reg2 = *(pToken+1) & 0x00001FFF;
              sprintf(tmpLine, "MOV TMP.r, T%lu.a;", reg2);
              addline(&lineNum, pgmStr, tmpLine);
              sprintf(tmpLine, "MOV TMP.g, T%lu.r;", reg2);
              addline(&lineNum, pgmStr, tmpLine);
              sprintf(tmpLine, "TEX T%lu, TMP, texture[%lu], 2D;", reg1, reg1);
              addline(&lineNum, pgmStr, tmpLine);
              autoparam = 0;
              pToken+=2;
            }
            break;
        case D3DSIO_TEXREG2GB:
            {
              DWORD reg1 = *pToken & 0x00001FFF;
              DWORD reg2 = *(pToken+1) & 0x00001FFF;
              sprintf(tmpLine, "MOV TMP.r, T%lu.g;", reg2);
              addline(&lineNum, pgmStr, tmpLine);
              sprintf(tmpLine, "MOV TMP.g, T%lu.b;", reg2);
              addline(&lineNum, pgmStr, tmpLine);
              sprintf(tmpLine, "TEX T%lu, TMP, texture[%lu], 2D;", reg1, reg1);
              addline(&lineNum, pgmStr, tmpLine);
              autoparam = 0;
              pToken+=2;
            }
            break;
	case D3DSIO_TEXBEM:
            {
              DWORD reg1 = *pToken & 0x00001FFF;
              DWORD reg2 = *(pToken+1) & 0x00001FFF;
              /* FIXME: Should apply the BUMPMAPENV matrix */
              sprintf(tmpLine, "ADD TMP.rg, fragment.texcoord[%lu], T%lu;", reg1, reg2);
              addline(&lineNum, pgmStr, tmpLine);
              sprintf(tmpLine, "TEX T%lu, TMP, texture[%lu], 2D;", reg1, reg1);
              addline(&lineNum, pgmStr, tmpLine);
              autoparam = 0;
              pToken+=2;
            }
            break;
        case D3DSIO_TEXM3x3PAD:
            {
              DWORD reg = *pToken & 0x00001FFF;
              char buf[50];
              if (gen_input_modifier_line(*(pToken+1), 0, buf, tmpLine))
                addline(&lineNum, pgmStr, tmpLine);
              sprintf(tmpLine, "DP3 TMP.%c, T%lu, %s;", 'x'+row, reg, buf);
              addline(&lineNum, pgmStr, tmpLine);
              tcw[row++] = reg;
              autoparam = 0;
              pToken += 2;
            }
            break;
        case D3DSIO_TEXM3x3TEX:
            {
              DWORD reg = *pToken & 0x00001FFF;
              char buf[50];
              if (gen_input_modifier_line(*(pToken+1), 0, buf, tmpLine))
                addline(&lineNum, pgmStr, tmpLine);
              sprintf(tmpLine, "DP3 TMP.z, T%lu, %s;", reg, buf);
              addline(&lineNum, pgmStr, tmpLine);
              /* Cubemap textures will be more used than 3D ones. */
              sprintf(tmpLine, "TEX T%lu, TMP, texture[%lu], CUBE;", reg, reg);
              addline(&lineNum, pgmStr, tmpLine);
              row = 0;
              autoparam = 0;
              pToken += 2;
            }
        case D3DSIO_TEXM3x3VSPEC:
            {
              DWORD reg = *pToken & 0x00001FFF;
              char buf[50];
              if (gen_input_modifier_line(*(pToken+1), 0, buf, tmpLine))
                addline(&lineNum, pgmStr, tmpLine);
              sprintf(tmpLine, "DP3 TMP.z, T%lu, %s;", reg, buf);
              addline(&lineNum, pgmStr, tmpLine);
              /* Construct the eye-ray vector from w coordinates */
              sprintf(tmpLine, "MOV TMP2.x, fragment.texcoord[%lu].w;", tcw[0]);
              addline(&lineNum, pgmStr, tmpLine);
              sprintf(tmpLine, "MOV TMP2.y, fragment.texcoord[%lu].w;", tcw[1]);
              addline(&lineNum, pgmStr, tmpLine);
              sprintf(tmpLine, "MOV TMP2.z, fragment.texcoord[%lu].w;", reg);
              addline(&lineNum, pgmStr, tmpLine);
              /* Calculate reflection vector (Assume normal is normalized): RF = 2*(N.E)*N -E */
              sprintf(tmpLine, "DP3 TMP.w, TMP, TMP2;");
              addline(&lineNum, pgmStr, tmpLine);
              sprintf(tmpLine, "MUL TMP, TMP.w, TMP;");
              addline(&lineNum, pgmStr, tmpLine);
              sprintf(tmpLine, "MAD TMP, coefmul.x, TMP, -TMP2;");
              addline(&lineNum, pgmStr, tmpLine);
              /* Cubemap textures will be more used than 3D ones. */
              sprintf(tmpLine, "TEX T%lu, TMP, texture[%lu], CUBE;", reg, reg);
              addline(&lineNum, pgmStr, tmpLine);
              row = 0;
              autoparam = 0;
              pToken += 2;
            }
            break;
        case D3DSIO_TEXM3x3SPEC:
            {
              DWORD reg = *pToken & 0x00001FFF;
              DWORD reg3 = *(pToken+2) & 0x00001FFF;
              char buf[50];
              if (gen_input_modifier_line(*(pToken+1), 0, buf, tmpLine))
                addline(&lineNum, pgmStr, tmpLine);
              sprintf(tmpLine, "DP3 TMP.z, T%lu, %s;", reg, buf);
              addline(&lineNum, pgmStr, tmpLine);
              /* Calculate reflection vector (Assume normal is normalized): RF = 2*(N.E)*N -E */
              sprintf(tmpLine, "DP3 TMP.w, TMP, C[%lu];", reg3);
              addline(&lineNum, pgmStr, tmpLine);
              sprintf(tmpLine, "MUL TMP, TMP.w, TMP;");
              addline(&lineNum, pgmStr, tmpLine);
              sprintf(tmpLine, "MAD TMP, coefmul.x, TMP, -C[%lu];", reg3);
              addline(&lineNum, pgmStr, tmpLine);	      
              /* Cubemap textures will be more used than 3D ones. */
              sprintf(tmpLine, "TEX T%lu, TMP, texture[%lu], CUBE;", reg, reg);
              addline(&lineNum, pgmStr, tmpLine);
              row = 0;
              autoparam = 0;
              pToken += 3;
            }
            break;
        case D3DSIO_CND:
	    break;
        case D3DSIO_CMP:
            break;
        case D3DSIO_MOV:
            strcpy(tmpLine, "MOV");
            break;
        case D3DSIO_MUL:
            strcpy(tmpLine, "MUL");
            break;
        case D3DSIO_DP3:
            strcpy(tmpLine, "DP3");
            break;
        case D3DSIO_MAD:
            strcpy(tmpLine, "MAD");
            break;
        case D3DSIO_ADD:
            strcpy(tmpLine, "ADD");
            break;
        case D3DSIO_SUB:
            strcpy(tmpLine, "SUB");
            break;
        case D3DSIO_LRP:
            strcpy(tmpLine, "LRP");
            break;
        default:
            FIXME_(d3d_hw_shader)("Can't handle opcode %s in hwShader\n", curOpcode->name);
        }
        if (0 != (*pToken & D3DSP_DSTMOD_MASK)) {
          DWORD mask = *pToken & D3DSP_DSTMOD_MASK;
          switch (mask) {
          case D3DSPDM_SATURATE: saturate = TRUE; break;
          default:
            TRACE("_unhandled_modifier(0x%08lx)", mask);
          }
        }
        if (autoparam && (curOpcode->num_params > 0)) {
          char regs[3][50];
          char operands[4][100];
          char tmp[256];
          char swzstring[20];
          int saturate = 0;
	  /* Generate lines that handle input modifier computation */
          for (i = 1; i < curOpcode->num_params; i++) {
            if (gen_input_modifier_line(*(pToken+i), i-1, regs[i-1], tmp))
              addline(&lineNum, pgmStr, tmp);
          }
	  /* Handle saturation only when no shift is present in the output modifier */
          if ((*pToken & D3DSPDM_SATURATE) && (0 == (*pToken & D3DSP_DSTSHIFT_MASK)))
            saturate = 1;
	  /* Handle output register */
          get_register_name(*pToken, tmp);
          strcpy(operands[0], tmp);
          get_write_mask(*pToken, tmp);
          strcat(operands[0], tmp);
	  /* Handle input registers */
          for (i = 1; i < curOpcode->num_params; i++) {
            strcpy(operands[i], regs[i-1]);
            get_input_register_swizzle(*(pToken+i), swzstring);
            strcat(operands[i], swzstring);
          }
	  if (curOpcode->opcode == D3DSIO_CMP) {
            sprintf(tmpLine, "CMP%s %s, %s, %s, %s;", (saturate ? "_SAT" : ""), operands[0], operands[1], operands[3], operands[2]);
	  } else if (curOpcode->opcode == D3DSIO_CND) {
            sprintf(tmpLine, "ADD TMP, -%s, coefdiv.x;", operands[1]);
            addline(&lineNum, pgmStr, tmpLine);
            sprintf(tmpLine, "CMP%s %s, TMP, %s, %s;", (saturate ? "_SAT" : ""), operands[0], operands[2], operands[3]);
          } else {
            if (saturate)
              strcat(tmpLine, "_SAT");
            strcat(tmpLine, " ");
            strcat(tmpLine, operands[0]);
            for (i = 1; i < curOpcode->num_params; i++) {
              strcat(tmpLine, ", ");
              strcat(tmpLine, operands[i]);
            }
            strcat(tmpLine,";");
          }
          addline(&lineNum, pgmStr, tmpLine);
          pToken += curOpcode->num_params;
        }
        if (curOpcode->num_params > 0) {
          DWORD param = *(pInstr+1);
          if (0 != (param & D3DSP_DSTSHIFT_MASK)) {
            /* Generate a line that handle the output modifier computation */
            char regstr[100];
            char write_mask[20];
            DWORD shift = (param & D3DSP_DSTSHIFT_MASK) >> D3DSP_DSTSHIFT_SHIFT;
            get_register_name(param, regstr);
            get_write_mask(param, write_mask);
            gen_output_modifier_line(saturate, write_mask, shift, regstr, tmpLine);
            addline(&lineNum, pgmStr, tmpLine);
          }
        }
      }
    }
    strcpy(tmpLine, "MOV result.color, R0;");
    addline(&lineNum, pgmStr, tmpLine);

    strcpy(tmpLine, "END");
    addline(&lineNum, pgmStr, tmpLine);
  }

  /*  Create the hw shader */
  GL_EXTCALL(glGenProgramsARB(1, &pshader->prgId));
  TRACE_(d3d_hw_shader)("Creating a hw pixel shader, prg=%d\n", pshader->prgId);

  GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, pshader->prgId));

  /* Create the program and check for errors */
  GL_EXTCALL(glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, strlen(pgmStr), pgmStr));
  if (glGetError() == GL_INVALID_OPERATION) {
    GLint errPos;
    glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errPos);
    FIXME_(d3d_hw_shader)("HW PixelShader Error at position: %d\n%s\n", errPos, glGetString(GL_PROGRAM_ERROR_STRING_ARB));
    pshader->prgId = -1;
  }
  
  HeapFree(GetProcessHeap(), 0, pgmStr);
}

inline static VOID IDirect3DPixelShaderImpl_ParseProgram(IDirect3DPixelShaderImpl* pshader, CONST DWORD* pFunction) {
  const DWORD* pToken = pFunction;
  const SHADER_OPCODE* curOpcode = NULL;
  DWORD code;
  DWORD len = 0;  
  DWORD i;
  int version = 0;

  if (NULL != pToken) {
    while (D3DPS_END() != *pToken) { 
      if (pshader_is_version_token(*pToken)) { /** version */
	TRACE("ps.%lu.%lu\n", (*pToken >> 8) & 0x0F, (*pToken & 0x0F));
	version = (((*pToken >> 8) & 0x0F) * 10) + (*pToken & 0x0F);
	++pToken;
	++len;
	continue;
      } 
      if (pshader_is_comment_token(*pToken)) { /** comment */
	DWORD comment_len = (*pToken & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;
	++pToken;
	/*TRACE("comment[%ld] ;%s\n", comment_len, (char*)pToken);*/
	pToken += comment_len;
	len += comment_len + 1;
	continue;
      }
      code = *pToken;
      curOpcode = pshader_program_get_opcode(code, version);
      ++pToken;
      ++len;
      if (NULL == curOpcode) {
	/* unknown current opcode ... */
	while (*pToken & 0x80000000) {
	  TRACE("unrecognized opcode: %08lx\n", *pToken);
	  ++pToken;
	  ++len;
	}
      } else {
	TRACE(" ");
	pshader_program_dump_opcode(curOpcode, code, *pToken);
	if (curOpcode->num_params > 0) {
	  pshader_program_dump_param(*pToken, 0);
	  ++pToken;
	  ++len;
	  for (i = 1; i < curOpcode->num_params; ++i) {
	    TRACE(", ");
	    if (D3DSIO_DEF != code) {
	      pshader_program_dump_param(*pToken, 1);
	    } else {
	      TRACE("%f", *((const float*) pToken));
	    }
	    ++pToken;
	    ++len;
	  }
	}
	TRACE("\n");
      }
      pshader->functionLength = (len + 1) * sizeof(DWORD);
    } 
  } else {
    pshader->functionLength = 1; /* no Function defined use fixed function vertex processing */
  }

  if (NULL != pFunction) {
    IDirect3DPixelShaderImpl_GenerateProgramArbHW(pshader, pFunction);
  }

  if (NULL != pFunction) {
    pshader->function = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, pshader->functionLength);
    memcpy(pshader->function, pFunction, pshader->functionLength);
  } else {
    pshader->function = NULL;
  }
}

HRESULT WINAPI IDirect3DDeviceImpl_CreatePixelShader(IDirect3DDevice8Impl* This, CONST DWORD* pFunction, IDirect3DPixelShaderImpl** ppPixelShader) {
  IDirect3DPixelShaderImpl* object;

  object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DPixelShaderImpl));
  if (NULL == object) {
    *ppPixelShader = NULL;
    return D3DERR_OUTOFVIDEOMEMORY;
  }
  /*object->lpVtbl = &Direct3DPixelShader9_Vtbl;*/
  object->device = This;
  object->ref = 1;
  
  object->data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PSHADERDATA8));
    
  IDirect3DPixelShaderImpl_ParseProgram(object, pFunction);

  *ppPixelShader = object;
  return D3D_OK;
}

HRESULT WINAPI IDirect3DPixelShaderImpl_GetFunction(IDirect3DPixelShaderImpl* This, VOID* pData, UINT* pSizeOfData) {
  if (NULL == pData) {
    *pSizeOfData = This->functionLength;
    return D3D_OK;
  }
  if (*pSizeOfData < This->functionLength) {
    *pSizeOfData = This->functionLength;
    return D3DERR_MOREDATA;
  }
  if (NULL == This->function) { /* no function defined */
    TRACE("(%p) : GetFunction no User Function defined using NULL to %p\n", This, pData);
    (*(DWORD **) pData) = NULL;
  } else {
    TRACE("(%p) : GetFunction copying to %p\n", This, pData);
    memcpy(pData, This->function, This->functionLength);
  }
  return D3D_OK;
}

HRESULT WINAPI IDirect3DPixelShaderImpl_SetConstantF(IDirect3DPixelShaderImpl* This, UINT StartRegister, CONST FLOAT* pConstantData, UINT Vector4fCount) {
  if (StartRegister + Vector4fCount > D3D8_VSHADER_MAX_CONSTANTS) {
    return D3DERR_INVALIDCALL;
  }
  if (NULL == This->data) { /* temporary while datas not supported */
    FIXME("(%p) : PixelShader_SetConstant not fully supported yet\n", This);
    return D3DERR_INVALIDCALL;
  }
  memcpy(&This->data->C[StartRegister], pConstantData, Vector4fCount * 4 * sizeof(FLOAT));
  return D3D_OK;
}

HRESULT WINAPI IDirect3DPixelShaderImpl_GetConstantF(IDirect3DPixelShaderImpl* This, UINT StartRegister, FLOAT* pConstantData, UINT Vector4fCount) {
  if (StartRegister + Vector4fCount > D3D8_VSHADER_MAX_CONSTANTS) {
    return D3DERR_INVALIDCALL;
  }
  if (NULL == This->data) { /* temporary while datas not supported */
    return D3DERR_INVALIDCALL;
  }
  memcpy(pConstantData, &This->data->C[StartRegister], Vector4fCount * 4 * sizeof(FLOAT));
  return D3D_OK;
}


/**********************************************************************************************************************************************
 **********************************************************************************************************************************************
 **********************************************************************************************************************************************
 **********************************************************************************************************************************************
 **********************************************************************************************************************************************/

/***********************************************************************
 *		ValidateVertexShader (D3D8.@)
 *
 * PARAMS
 * toto       result?
 */
BOOL WINAPI ValidateVertexShader(LPVOID pFunction, int param1, int param2, LPVOID toto)
{
      FIXME("(void): stub: pFunction %p, param1 %d, param2 %d, result? %p\n",  pFunction, param1, param2, toto);
        return 0;
}

/***********************************************************************
 *              ValidatePixelShader (D3D8.@)
 *
 * PARAMS
 * toto       result?
 */
BOOL WINAPI ValidatePixelShader(LPVOID pFunction, int param1, int param2, LPVOID toto)
{
      FIXME("(void): stub: pFunction %p, param1 %d, param2 %d, result? %p\n",  pFunction, param1, param2, toto);
        return TRUE;
}
