/*
 * shaders implementation
 *
 * Copyright 2002 Raphael Junqueira
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

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "wine/debug.h"

#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_shader);

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
 *  http://msdn.microsoft.com/library/en-us/directx9_c/directx/graphics/programmingguide/GettingStarted/VertexFormats/vformats.asp
 *
 * NVIDIA: DX8 Vertex Shader to NV Vertex Program
 *  http://developer.nvidia.com/view.asp?IO=vstovp
 *
 * NVIDIA: Memory Management with VAR
 *  http://developer.nvidia.com/view.asp?IO=var_memory_management 
 */

typedef void (*shader_fct_t)();

typedef struct SHADER_OPCODE {
  CONST BYTE    opcode;
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
  float tmp_f = floorf(s0->w);
  DWORD tmp_d = 0;
  tmp_f = powf(2.0f, s0->w);
  tmp_d = *((DWORD*) &tmp_f) & 0xFFFFFF00;

  d->x  = powf(2.0f, tmp_f);
  d->y  = s0->w - tmp_f;
  d->z  = *((float*) &tmp_d);
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
  d->x = d->y = d->z = d->w = (0.0f != tmp_f) ? logf(tmp_f) / logf(2.0f) : -HUGE;
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
  d->x = d->y = d->z = d->w = (0.0f == s0->w) ? HUGE : 1.0f / s0->w;
  VSTRACE(("executing rcp: s0=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	  s0->x, s0->y, s0->z, s0->w, d->x, d->y, d->z, d->w));
}

void vshader_rsq(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0) {
  float tmp_f = fabsf(s0->w);
  d->x = d->y = d->z = d->w = (0.0f == tmp_f) ? HUGE : ((1.0f != tmp_f) ? 1.0f / sqrtf(tmp_f) : 1.0f);
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
  d->x = d->y = d->z = d->w = (0.0f != tmp_f) ? logf(tmp_f) / logf(2.0f) : -HUGE;
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
typedef FLOAT D3DMATRIX43[4][3];
typedef FLOAT D3DMATRIX34[4][4];
typedef FLOAT D3DMATRIX33[4][3];
typedef FLOAT D3DMATRIX32[4][2];

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
  VSTRACE(("executing m4x4(1): mat=(%f, %f, %f, %f)    s0=(%f)     d=(%f) \n", mat[0][0], mat[0][1], mat[0][2], mat[0][3], s0->x, d->x));
  VSTRACE(("executing m4x4(2): mat=(%f, %f, %f, %f)       (%f)       (%f) \n", mat[1][0], mat[1][1], mat[1][2], mat[1][3], s0->y, d->y));
  VSTRACE(("executing m4x4(3): mat=(%f, %f, %f, %f) X     (%f)  =    (%f) \n", mat[2][0], mat[2][1], mat[2][2], mat[2][3], s0->z, d->z));
  VSTRACE(("executing m4x4(4): mat=(%f, %f, %f, %f)       (%f)       (%f) \n", mat[3][0], mat[3][1], mat[3][2], mat[3][3], s0->w, d->w));
}

void vshader_m4x3(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0, D3DMATRIX43 mat) {
  d->x = mat[0][0] * s0->x + mat[0][1] * s0->y + mat[0][2] * s0->z + mat[0][3] * s0->w;
  d->y = mat[1][0] * s0->x + mat[1][1] * s0->y + mat[1][2] * s0->z + mat[1][3] * s0->w;
  d->z = mat[2][0] * s0->x + mat[2][1] * s0->y + mat[2][2] * s0->z + mat[2][3] * s0->w;
  d->w = 1.0f;
  VSTRACE(("executing m4x3(1): mat=(%f, %f, %f, %f)    s0=(%f)     d=(%f) \n", mat[0][0], mat[0][1], mat[0][2], mat[0][3], s0->x, d->x));
  VSTRACE(("executing m4x3(2): mat=(%f, %f, %f, %f)       (%f)       (%f) \n", mat[1][0], mat[1][1], mat[1][2], mat[1][3], s0->y, d->y));
  VSTRACE(("executing m4x3(3): mat=(%f, %f, %f, %f) X     (%f)  =    (%f) \n", mat[2][0], mat[2][1], mat[2][2], mat[2][3], s0->z, d->z));
  VSTRACE(("executing m4x3(4):                            (%f)       (%f) \n", s0->w, d->w));
}

void vshader_m3x4(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0, D3DMATRIX34 mat) {
  d->x = mat[0][0] * s0->x + mat[0][1] * s0->y + mat[0][2] * s0->z;
  d->y = mat[2][0] * s0->x + mat[1][1] * s0->y + mat[1][2] * s0->z;
  d->z = mat[2][0] * s0->x + mat[2][1] * s0->y + mat[2][2] * s0->z;
  d->w = mat[3][0] * s0->x + mat[3][1] * s0->y + mat[3][2] * s0->z;
  VSTRACE(("executing m3x4(1): mat=(%f, %f, %f)    s0=(%f)     d=(%f) \n", mat[0][0], mat[0][1], mat[0][2], s0->x, d->x));
  VSTRACE(("executing m3x4(2): mat=(%f, %f, %f)       (%f)       (%f) \n", mat[1][0], mat[1][1], mat[1][2], s0->y, d->y));
  VSTRACE(("executing m3x4(3): mat=(%f, %f, %f) X     (%f)  =    (%f) \n", mat[2][0], mat[2][1], mat[2][2], s0->z, d->z));
  VSTRACE(("executing m3x4(4): mat=(%f, %f, %f)       (%f)       (%f) \n", mat[3][0], mat[3][1], mat[3][2], s0->w, d->w));
}

void vshader_m3x3(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0, D3DMATRIX33 mat) {
  d->x = mat[0][0] * s0->x + mat[0][1] * s0->y + mat[2][2] * s0->z;
  d->y = mat[1][0] * s0->x + mat[1][1] * s0->y + mat[2][2] * s0->z;
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
 */
static CONST SHADER_OPCODE vshader_ins [] = {
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
  {D3DSIO_LRP,  "lrp",  5, vshader_lrp, 0, 0},
  {D3DSIO_FRC,  "frc",  2, vshader_frc, 0, 0},
  {D3DSIO_M4x4, "m4x4", 3, vshader_m4x4, 0, 0},
  {D3DSIO_M4x3, "m4x3", 3, vshader_m4x3, 0, 0},
  {D3DSIO_M3x4, "m3x4", 3, vshader_m3x4, 0, 0},
  {D3DSIO_M3x3, "m3x3", 3, vshader_m3x3, 0, 0},
  {D3DSIO_M3x2, "m3x2", 3, vshader_m3x2, 0, 0},
  /** FIXME: use direct access so add the others opcodes as stubs */
  {D3DSIO_EXPP, "expp", 2, vshader_expp, 0, 0},
  {D3DSIO_LOGP, "logp", 2, vshader_logp, 0, 0},

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
    TRACE("C[%s%lu]", (reg & D3DVS_ADDRMODE_RELATIVE) ? "a0.x + " : "", reg);
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

inline static BOOL vshader_is_version_token(DWORD token) {
  return 0xFFFE0000 == (token & 0xFFFE0000);
}

inline static BOOL vshader_is_comment_token(DWORD token) {
  return D3DSIO_COMMENT == (token & D3DSI_OPCODE_MASK);
}

/**
 * Function parser ...
 */
inline static VOID IDirect3DVertexShaderImpl_ParseProgram(IDirect3DVertexShaderImpl* vshader, CONST DWORD* pFunction) {
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
	/* unkown current opcode ... */
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
    
  IDirect3DVertexShaderImpl_ParseProgram(object, pFunction);

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
  D3DSHADERVECTOR* p[4];
  D3DSHADERVECTOR* p_send[4];
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
      /* unkown current opcode ... */
      while (*pToken & 0x80000000) {
	if (i == 0) {
	  TRACE("unrecognized opcode: pos=%d token=%08lX\n", (pToken - 1) - vshader->function, *(pToken - 1));
	}
	TRACE("unrecognized opcode param: pos=%d token=%08lX what=", pToken - vshader->function, *pToken);
	vshader_program_dump_param(*pToken, i);
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
	    if (reg & D3DVS_ADDRMODE_RELATIVE) {
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
  {D3DSIO_LRP,  "lrp",  5, vshader_lrp, 0, 0},
  {D3DSIO_FRC,  "frc",  2, vshader_frc, 0, 0},
  {D3DSIO_M4x4, "m4x4", 3, vshader_m4x4, 0, 0},
  {D3DSIO_M4x3, "m4x3", 3, vshader_m4x3, 0, 0},
  {D3DSIO_M3x4, "m3x4", 3, vshader_m3x4, 0, 0},
  {D3DSIO_M3x3, "m3x3", 3, vshader_m3x3, 0, 0},
  {D3DSIO_M3x2, "m3x2", 3, vshader_m3x2, 0, 0},

  {D3DSIO_TEXCOORD,     "texcoord",     1, pshader_texcoord,     D3DPS_VERSION(1,1), D3DPS_VERSION(1,3)},
  {D3DSIO_TEXKILL,      "texkill",      1, pshader_texkill,      D3DPS_VERSION(1,1), D3DPS_VERSION(1,3)},
  {D3DSIO_TEX,          "tex",          1, pshader_tex,          D3DPS_VERSION(1,1), D3DPS_VERSION(1,4)}, 
  {D3DSIO_TEXBEM,       "texbem",       2, pshader_texbem,       D3DPS_VERSION(1,1), D3DPS_VERSION(1,3)}, 
  {D3DSIO_TEXBEML,      "texbeml",      2, pshader_texbeml,      D3DPS_VERSION(1,1), D3DPS_VERSION(1,3)}, 
  {D3DSIO_TEXREG2AR,    "texreg2ar",    2, pshader_texreg2ar,    D3DPS_VERSION(1,1), D3DPS_VERSION(1,3)}, 
  {D3DSIO_TEXREG2GB,    "texreg2gb",    2, pshader_texreg2gb,    D3DPS_VERSION(1,2), D3DPS_VERSION(1,3)}, 
  {D3DSIO_TEXM3x2PAD,   "texm3x2pad",   2, pshader_texm3x2pad,   D3DPS_VERSION(1,1), D3DPS_VERSION(1,3)}, 
  {D3DSIO_TEXM3x2TEX,   "texm3x2tex",   2, pshader_texm3x2tex,   D3DPS_VERSION(1,1), D3DPS_VERSION(1,3)}, 
  {D3DSIO_TEXM3x3PAD,   "texm3x3pad",   2, pshader_texm3x3pad,   D3DPS_VERSION(1,1), D3DPS_VERSION(1,3)}, 
  {D3DSIO_TEXM3x3TEX,   "texm3x3tex",   2, pshader_texm3x3tex,   D3DPS_VERSION(1,1), D3DPS_VERSION(1,3)}, 
  {D3DSIO_TEXM3x3DIFF,  "texm3x3diff",  2, pshader_texm3x3diff,  D3DPS_VERSION(0,0), D3DPS_VERSION(0,0)}, 
  {D3DSIO_TEXM3x3SPEC,  "texm3x3spec",  3, pshader_texm3x3spec,  D3DPS_VERSION(1,1), D3DPS_VERSION(1,3)}, 
  {D3DSIO_TEXM3x3VSPEC, "texm3x3vspec", 2, pshader_texm3x3vspec, D3DPS_VERSION(1,1), D3DPS_VERSION(1,3)}, 

  {D3DSIO_EXPP,         "expp",         2, vshader_expp, 0, 0},
  {D3DSIO_LOGP,         "logp",         2, vshader_logp, 0, 0},

  {D3DSIO_CND,          "cnd",          4, pshader_cnd,          D3DPS_VERSION(1,1), D3DPS_VERSION(1,4)},
  {D3DSIO_DEF,          "def",          5, pshader_def,          D3DPS_VERSION(1,1), D3DPS_VERSION(3,0)},
  {D3DSIO_TEXREG2RGB,   "texbreg2rgb",  2, pshader_texreg2rgb,   D3DPS_VERSION(1,2), D3DPS_VERSION(1,3)}, 
  
  {D3DSIO_TEXDP3TEX,    "texdp3tex",    2, pshader_texdp3tex,    D3DPS_VERSION(1,2), D3DPS_VERSION(1,3)}, 
  {D3DSIO_TEXM3x2DEPTH, "texm3x2depth", 2, pshader_texm3x2depth, D3DPS_VERSION(1,3), D3DPS_VERSION(1,3)}, 
  {D3DSIO_TEXDP3,       "texdp3",       2, pshader_texdp3,       D3DPS_VERSION(1,2), D3DPS_VERSION(1,3)}, 
  {D3DSIO_TEXM3x3,      "texm3x3",      2, pshader_texm3x3,      D3DPS_VERSION(1,2), D3DPS_VERSION(1,3)},
  {D3DSIO_TEXDEPTH,     "texdepth",     1, pshader_texdepth,     D3DPS_VERSION(1,4), D3DPS_VERSION(1,4)},
  {D3DSIO_CMP,          "cmp",          4, pshader_cmp,          D3DPS_VERSION(1,1), D3DPS_VERSION(3,0)}, 
  {D3DSIO_BEM,          "bem",          3, pshader_bem,          D3DPS_VERSION(1,4), D3DPS_VERSION(1,4)}, 

  {0, NULL, 0, NULL}
};

inline static const SHADER_OPCODE* pshader_program_get_opcode(const DWORD code) {
  DWORD i = 0;
  /** TODO: use dichotomic search */
  while (NULL != pshader_ins[i].name) {
    if ((code & D3DSI_OPCODE_MASK) == pshader_ins[i].opcode) {
      return &pshader_ins[i];
    }
    ++i;
  }
  return NULL;
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
    if (shift > 0) {
      TRACE("_x%u", 1 << shift);
    }
  } 
  /**
   * TODO: fix the divide shifts: d2, d4, d8
   *  so i have to find a sample
   */
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

  if ((param & D3DSP_SRCMOD_MASK) == D3DSPSM_NEG) {
    TRACE("-");
  }
  
  switch (regtype << D3DSP_REGTYPE_SHIFT) {
  case D3DSPR_TEMP:
    TRACE("R[%lu]", reg);
    break;
  case D3DSPR_INPUT:
    TRACE("V[%lu]", reg);
    break;
  case D3DSPR_CONST:
    TRACE("C[%s%lu]", (reg & D3DVS_ADDRMODE_RELATIVE) ? "a0.x + " : "", reg);
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
      if (param & D3DSP_WRITEMASK_0) TRACE(".r");
      if (param & D3DSP_WRITEMASK_1) TRACE(".g");
      if (param & D3DSP_WRITEMASK_2) TRACE(".b");
      if (param & D3DSP_WRITEMASK_3) TRACE(".a");
    }
  } else {
    /** operand input */
    DWORD swizzle = (param & D3DSP_SWIZZLE_MASK) >> D3DSP_SWIZZLE_SHIFT;
    DWORD swizzle_x = swizzle & 0x03;
    DWORD swizzle_y = (swizzle >> 2) & 0x03;
    DWORD swizzle_z = (swizzle >> 4) & 0x03;
    DWORD swizzle_w = (swizzle >> 6) & 0x03;
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
    if (0 != (param & D3DSP_SRCMOD_MASK)) {
      DWORD mask = param & D3DSP_SRCMOD_MASK;
      /*TRACE("_modifier(0x%08lx) ", mask);*/
      switch (mask) {
      case D3DSPSM_NONE:    break;
      case D3DSPSM_NEG:     break;
      case D3DSPSM_BIAS:    TRACE("_bias"); break;
      case D3DSPSM_BIASNEG: TRACE("_bias"); break;
      case D3DSPSM_SIGN:    TRACE("_sign"); break;
      case D3DSPSM_SIGNNEG: TRACE("_sign"); break;
      case D3DSPSM_COMP:    TRACE("_comp"); break;
      case D3DSPSM_X2:      TRACE("_x2"); break;
      case D3DSPSM_X2NEG:   TRACE("_bx2"); break;
      case D3DSPSM_DZ:      TRACE("_dz"); break;
      case D3DSPSM_DW:      TRACE("_dw"); break;
      default:
	TRACE("_unknown(0x%08lx)", mask);
      }
    }
  }
}

inline static BOOL pshader_is_version_token(DWORD token) {
  return 0xFFFF0000 == (token & 0xFFFF0000);
}

inline static BOOL pshader_is_comment_token(DWORD token) {
  return D3DSIO_COMMENT == (token & D3DSI_OPCODE_MASK);
}



/**
 * Pixel Shaders
 *
 * http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/reference/Shaders/PixelShader1_X/modifiers/sourceregistermodifiers.asp
 * http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/reference/Shaders/PixelShader2_0/Registers/Registers.asp
 * http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/reference/d3d/interfaces/IDirect3DPixelShader9/_IDirect3DPixelShader9.asp
 *
 */
inline static VOID IDirect3DPixelShaderImpl_ParseProgram(IDirect3DPixelShaderImpl* pshader, CONST DWORD* pFunction) {
  const DWORD* pToken = pFunction;
  const SHADER_OPCODE* curOpcode = NULL;
  DWORD code;
  DWORD len = 0;  
  DWORD i;

  if (NULL != pToken) {
    while (D3DPS_END() != *pToken) { 
      if (pshader_is_version_token(*pToken)) { /** version */
	TRACE("ps.%lu.%lu\n", (*pToken >> 8) & 0x0F, (*pToken & 0x0F));
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
      curOpcode = pshader_program_get_opcode(code);
      ++pToken;
      ++len;
      if (NULL == curOpcode) {
	/* unkown current opcode ... */
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
	      TRACE("%f", *((float*) pToken));
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
    FIXME("(%p) : VertexShader_SetConstant not fully supported yet\n", This);
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
 */
BOOL WINAPI ValidateVertexShader(LPVOID what, LPVOID toto) {
  FIXME("(void): stub: %p %p\n", what, toto);
  return TRUE;
}

/***********************************************************************
 *		ValidatePixelShader (D3D8.@)
 */
BOOL WINAPI ValidatePixelShader(LPVOID what, LPVOID toto) {
  FIXME("(void): stub: %p %p\n", what, toto);
  return TRUE;
}
