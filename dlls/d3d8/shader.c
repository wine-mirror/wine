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

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "wine/debug.h"

#include <math.h>

#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

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
} SHADER_OPCODE;

/** Vertex Shader Declaration data types tokens */
static CONST char* VertexShaderDeclDataTypes [] = {
  "D3DVSDT_FLOAT1",
  "D3DVSDT_FLOAT2",
  "D3DVSDT_FLOAT3",
  "D3DVSDT_FLOAT4",
  "D3DVSDT_D3DCOLOR",
  "D3DVSDT_UBYTE4",
  "D3DVSDT_SHORT2",
  "D3DVSDT_SHORT4",
  NULL
};

static CONST char* VertexShaderDeclRegister [] = {
  "D3DVSDE_POSITION",
  "D3DVSDE_BLENDWEIGHT",
  "D3DVSDE_BLENDINDICES",
  "D3DVSDE_NORMAL",
  "D3DVSDE_PSIZE",
  "D3DVSDE_DIFFUSE",
  "D3DVSDE_SPECULAR",
  "D3DVSDE_TEXCOORD0",
  "D3DVSDE_TEXCOORD1",
  "D3DVSDE_TEXCOORD2",
  "D3DVSDE_TEXCOORD3",
  "D3DVSDE_TEXCOORD4",
  "D3DVSDE_TEXCOORD5",
  "D3DVSDE_TEXCOORD6",
  "D3DVSDE_TEXCOORD7",
  "D3DVSDE_POSITION2",
  "D3DVSDE_NORMAL2",
  NULL
};

/*******************************
 * vshader functions software VM
 */

void vshader_add(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0, D3DSHADERVECTOR* s1) {
  d->x = s0->x + s1->x;
  d->y = s0->y + s1->y;
  d->z = s0->z + s1->z;
  d->w = s0->w + s1->w;
}

void vshader_dp3(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0, D3DSHADERVECTOR* s1) {
  d->x = d->y = d->z = d->w = s0->x * s1->x + s0->y * s1->y + s0->z * s1->z;
}

void vshader_dp4(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0, D3DSHADERVECTOR* s1) {
  d->x = d->y = d->z = d->w = s0->x * s1->x + s0->y * s1->y + s0->z * s1->z + s0->w * s1->w;

  /*
  DPRINTF("executing dp4: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	  s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w);
  */
}

void vshader_dst(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0, D3DSHADERVECTOR* s1) {
  d->x = 1.0f;
  d->y = s0->y * s1->y;
  d->z = s0->z;
  d->w = s1->w;

  /*
  DPRINTF("executing dst: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	  s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w);
  */
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

  /*
  DPRINTF("executing exp: s0=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
          s0->x, s0->y, s0->z, s0->w, d->x, d->y, d->z, d->w);
  */
}

void vshader_lit(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0) {
  d->x = 1.0f;
  d->y = (0.0f < s0->x) ? s0->x : 0.0f;
  d->z = (0.0f < s0->x && 0.0f < s0->y) ? powf(s0->y, s0->w) : 0.0f;
  d->w = 1.0f;

  /*
  DPRINTF("executing lit: s0=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	  s0->x, s0->y, s0->z, s0->w, d->x, d->y, d->z, d->w);
  */
}

void vshader_logp(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0) {
  float tmp_f = fabsf(s0->w); 
  d->x = d->y = d->z = d->w = (0.0f != tmp_f) ? logf(tmp_f) / logf(2.0f) : -HUGE;
}

void vshader_mad(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0, D3DSHADERVECTOR* s1, D3DSHADERVECTOR* s2) {
  d->x = s0->x * s1->x + s2->x;
  d->y = s0->y * s1->y + s2->y;
  d->z = s0->z * s1->z + s2->z;
  d->w = s0->w * s1->w + s2->w;
}

void vshader_max(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0, D3DSHADERVECTOR* s1) {
  d->x = (s0->x >= s1->x) ? s0->x : s1->x;
  d->y = (s0->y >= s1->y) ? s0->y : s1->y;
  d->z = (s0->z >= s1->z) ? s0->z : s1->z;
  d->w = (s0->w >= s1->w) ? s0->w : s1->w;
}

void vshader_min(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0, D3DSHADERVECTOR* s1) {
  d->x = (s0->x < s1->x) ? s0->x : s1->x;
  d->y = (s0->y < s1->y) ? s0->y : s1->y;
  d->z = (s0->z < s1->z) ? s0->z : s1->z;
  d->w = (s0->w < s1->w) ? s0->w : s1->w;
}

void vshader_mov(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0) {
  d->x = s0->x;
  d->y = s0->y;
  d->z = s0->z;
  d->w = s0->w;

  /*
  DPRINTF("executing mov: s0=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
	  s0->x, s0->y, s0->z, s0->w, d->x, d->y, d->z, d->w);
  */
}

void vshader_mul(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0, D3DSHADERVECTOR* s1) {
  d->x = s0->x * s1->x;
  d->y = s0->y * s1->y;
  d->z = s0->z * s1->z;
  d->w = s0->w * s1->w;

  /*
  DPRINTF("executing mul: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
          s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w);
  */
}

void vshader_nop(void) {
  /* NOPPPP ahhh too easy ;) */
}

void vshader_rcp(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0) {
  d->x = d->y = d->z = d->w = (0.0f == s0->w) ? HUGE : 1.0f / s0->w;
}

void vshader_rsq(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0) {
  float tmp_f = fabsf(s0->w);
  d->x = d->y = d->z = d->w = (0.0f == tmp_f) ? HUGE : ((1.0f != tmp_f) ? 1.0f / sqrtf(tmp_f) : 1.0f);
}

void vshader_sge(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0, D3DSHADERVECTOR* s1) {
  d->x = (s0->x >= s1->x) ? 1.0f : 0.0f;
  d->y = (s0->y >= s1->y) ? 1.0f : 0.0f;
  d->z = (s0->z >= s1->z) ? 1.0f : 0.0f;
  d->w = (s0->w >= s1->w) ? 1.0f : 0.0f;
}

void vshader_slt(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0, D3DSHADERVECTOR* s1) {
  d->x = (s0->x < s1->x) ? 1.0f : 0.0f;
  d->y = (s0->y < s1->y) ? 1.0f : 0.0f;
  d->z = (s0->z < s1->z) ? 1.0f : 0.0f;
  d->w = (s0->w < s1->w) ? 1.0f : 0.0f;
}

void vshader_sub(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0, D3DSHADERVECTOR* s1) {
  d->x = s0->x - s1->x;
  d->y = s0->y - s1->y;
  d->z = s0->z - s1->z;
  d->w = s0->w - s1->w;
}

/**
 * Version 1.1 specific
 */

void vshader_exp(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0) {
  d->x = d->y = d->z = d->w = powf(2.0f, s0->w);
}

void vshader_log(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0) {
  float tmp_f = fabsf(s0->w); 
  d->x = d->y = d->z = d->w = (0.0f != tmp_f) ? logf(tmp_f) / logf(2.0f) : -HUGE;
}

void vshader_frc(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0) {
  d->x = s0->x - floorf(s0->x);
  d->y = s0->y - floorf(s0->y);
  d->z = 0.0f;
  d->w = 1.0f;
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
}

void vshader_m4x3(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0, D3DMATRIX43 mat) {
  FIXME("check\n");
  d->x = mat[0][0] * s0->x + mat[0][1] * s0->y + mat[0][2] * s0->z + mat[0][3] * s0->w;
  d->y = mat[1][0] * s0->x + mat[1][1] * s0->y + mat[1][2] * s0->z + mat[1][3] * s0->w;
  d->z = mat[2][0] * s0->x + mat[2][1] * s0->y + mat[2][2] * s0->z + mat[2][3] * s0->w;
  d->w = 1.0f;
}

void vshader_m3x4(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0, D3DMATRIX34 mat) {
  FIXME("check\n");
  d->x = mat[0][0] * s0->x + mat[0][1] * s0->y + mat[0][2] * s0->z;
  d->y = mat[2][0] * s0->x + mat[1][1] * s0->y + mat[1][2] * s0->z;
  d->z = mat[2][0] * s0->x + mat[2][1] * s0->y + mat[2][2] * s0->z;
  d->w = mat[3][0] * s0->x + mat[3][1] * s0->y + mat[3][2] * s0->z;
}

void vshader_m3x3(D3DSHADERVECTOR* d, D3DSHADERVECTOR* s0, D3DMATRIX33 mat) {
  FIXME("check\n");
  d->x = mat[0][0] * s0->x + mat[0][1] * s0->y + mat[2][2] * s0->z;
  d->y = mat[1][0] * s0->x + mat[1][1] * s0->y + mat[2][2] * s0->z;
  d->z = mat[2][0] * s0->x + mat[2][1] * s0->y + mat[2][2] * s0->z;
  d->w = 1.0f;
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
  {D3DSIO_NOP,  "nop",  0, vshader_nop},
  {D3DSIO_MOV,  "mov",  2, vshader_mov},
  {D3DSIO_ADD,  "add",  3, vshader_add},
  {D3DSIO_SUB,  "sub",  3, vshader_sub},
  {D3DSIO_MAD,  "mad",  4, vshader_mad},
  {D3DSIO_MUL,  "mul",  3, vshader_mul},
  {D3DSIO_RCP,  "rcp",  2, vshader_rcp},
  {D3DSIO_RSQ,  "rsq",  2, vshader_rsq},
  {D3DSIO_DP3,  "dp3",  3, vshader_dp3},
  {D3DSIO_DP4,  "dp4",  3, vshader_dp4},
  {D3DSIO_MIN,  "min",  3, vshader_min},
  {D3DSIO_MAX,  "max",  3, vshader_max},
  {D3DSIO_SLT,  "slt",  3, vshader_slt},
  {D3DSIO_SGE,  "sge",  3, vshader_sge},
  {D3DSIO_EXP,  "exp",  2, vshader_exp},
  {D3DSIO_LOG,  "log",  2, vshader_log},
  {D3DSIO_LIT,  "lit",  2, vshader_lit},
  {D3DSIO_DST,  "dst",  3, vshader_dst},
  {D3DSIO_LRP,  "lrp",  5, vshader_lrp},
  {D3DSIO_FRC,  "frc",  2, vshader_frc},
  {D3DSIO_M4x4, "m4x4", 3, vshader_m4x4},
  {D3DSIO_M4x3, "m4x3", 3, vshader_m4x3},
  {D3DSIO_M3x4, "m3x4", 3, vshader_m3x4},
  {D3DSIO_M3x3, "m3x3", 3, vshader_m3x3},
  {D3DSIO_M3x2, "m3x2", 3, vshader_m3x2},
  /** FIXME: use direct acces so add the others opcodes as stubs */
  {D3DSIO_EXPP, "expp", 2, vshader_expp},
  {D3DSIO_LOGP, "logp", 2, vshader_logp},

  {0, NULL, 0, NULL}
};


const SHADER_OPCODE* vshader_program_get_opcode(const DWORD code) {
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

void vshader_program_dump_param(const DWORD param, int input) {
  static const char* rastout_reg_names[] = { "oPos", "oFog", "oPts" };
  static const char swizzle_reg_chars[] = "xyzw";

  DWORD reg = param & 0x00001FFF;
  DWORD regtype = ((param & D3DSP_REGTYPE_MASK) >> D3DSP_REGTYPE_SHIFT);

  if ((param & D3DSP_SRCMOD_MASK) == D3DSPSM_NEG) DPRINTF("-");
  
  switch (regtype << D3DSP_REGTYPE_SHIFT) {
  case D3DSPR_TEMP:
    DPRINTF("R[%lu]", reg);
    break;
  case D3DSPR_INPUT:
    DPRINTF("V[%lu]", reg);
    break;
  case D3DSPR_CONST:
    DPRINTF("C[%s%lu]", (reg & D3DVS_ADDRMODE_RELATIVE) ? "a0.x + " : "", reg);
    break;
  case D3DSPR_ADDR: /*case D3DSPR_TEXTURE:*/
    DPRINTF("a[%lu]", reg);
    break;
  case D3DSPR_RASTOUT:
    DPRINTF("%s", rastout_reg_names[reg]);
    break;
  case D3DSPR_ATTROUT:
    DPRINTF("oD[%lu]", reg);
    break;
  case D3DSPR_TEXCRDOUT:
    DPRINTF("oT[%lu]", reg);
    break;
  default:
    break;
  }

  if (!input) {
    /** operand output */
    if ((param & D3DSP_WRITEMASK_ALL) != D3DSP_WRITEMASK_ALL) {
      if (param & D3DSP_WRITEMASK_0) DPRINTF(".x");
      if (param & D3DSP_WRITEMASK_1) DPRINTF(".y");
      if (param & D3DSP_WRITEMASK_2) DPRINTF(".z");
      if (param & D3DSP_WRITEMASK_3) DPRINTF(".w");
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
	DPRINTF(".%c", swizzle_reg_chars[swizzle_x]);
      } else {
	DPRINTF(".%c%c%c%c", 
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
DWORD vshader_program_parse(VERTEXSHADER8* vshader) {
  const DWORD* pToken = vshader->function;
  const SHADER_OPCODE* curOpcode = NULL;
  DWORD len = 0;  
  DWORD i;

  if (NULL != pToken) {
    while (D3DVS_END() != *pToken) {
      if (vshader_is_version_token(*pToken)) { /** version */
	DPRINTF("vs.%lu.%lu\n", (*pToken >> 8) & 0x0F, (*pToken & 0x0F));
	++pToken;
	++len;
	continue;
      } 
      if (vshader_is_comment_token(*pToken)) { /** comment */
	DWORD comment_len = (*pToken & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;
	++pToken;
	/*DPRINTF("comment[%ld] ;%s\n", comment_len, (char*)pToken);*/
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
	  DPRINTF("unrecognized opcode: %08lx\n", *pToken);
	  ++pToken;
	  ++len;
	}
      } else {
	DPRINTF("%s ", curOpcode->name);
	if (curOpcode->num_params > 0) {
	  vshader_program_dump_param(*pToken, 0);
	  ++pToken;
	  ++len;
	  for (i = 1; i < curOpcode->num_params; ++i) {
	    DPRINTF(", ");
	    vshader_program_dump_param(*pToken, 1);
	    ++pToken;
	    ++len;
	  }
	}
	DPRINTF("\n");
      }
    }
    vshader->functionLength = (len + 1) * sizeof(DWORD);
  } else {
    vshader->functionLength = 1; /* no Function defined use fixed function vertex processing */
  }
  return len * sizeof(DWORD);
}

BOOL vshader_program_execute_HAL(VERTEXSHADER8* vshader,
				 VSHADERINPUTDATA8* input,
				 VSHADEROUTPUTDATA8* output) {
  /** 
   * TODO: use the NV_vertex_program (or 1_1) extension 
   *  and specifics vendors (ARB_vertex_program??) variants for it 
   */
  return TRUE;
}

#define TRACE_VECTOR(name) DPRINTF( #name "=(%f, %f, %f, %f)\n", name.x, name.y, name.z, name.w)

BOOL vshader_program_execute_SW(VERTEXSHADER8* vshader,
				VSHADERINPUTDATA8* input,
				VSHADEROUTPUTDATA8* output) {
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
  /*
  TRACE_VECTOR(vshader->data->C[0]);
  TRACE_VECTOR(vshader->data->C[1]);
  TRACE_VECTOR(vshader->data->C[2]);
  TRACE_VECTOR(vshader->data->C[3]);
  TRACE_VECTOR(vshader->data->C[4]);
  TRACE_VECTOR(vshader->data->C[5]);
  TRACE_VECTOR(input->V[D3DVSDE_POSITION]);
  TRACE_VECTOR(input->V[D3DVSDE_BLENDWEIGHT]);
  TRACE_VECTOR(input->V[D3DVSDE_BLENDINDICES]);
  TRACE_VECTOR(input->V[D3DVSDE_NORMAL]);
  TRACE_VECTOR(input->V[D3DVSDE_PSIZE]);
  TRACE_VECTOR(input->V[D3DVSDE_DIFFUSE]);
  TRACE_VECTOR(input->V[D3DVSDE_SPECULAR]);
  TRACE_VECTOR(input->V[D3DVSDE_TEXCOORD0]);
  TRACE_VECTOR(input->V[D3DVSDE_TEXCOORD1]);
  */

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
	  DPRINTF("unrecognized opcode: pos=%d token=%08lX\n", (pToken - 1) - vshader->function, *(pToken - 1));
	}
	DPRINTF("unrecognized opcode param: pos=%d token=%08lX what=", pToken - vshader->function, *pToken);
	vshader_program_dump_param(*pToken, i);
	DPRINTF("\n");
	++i;
	++pToken;
      }
      /*return FALSE;*/
    } else {     
      if (curOpcode->num_params > 0) {	
	/*DPRINTF(">> execting opcode: pos=%d opcode_name=%s token=%08lX\n", pToken - vshader->function, curOpcode->name, *pToken);*/
	for (i = 0; i < curOpcode->num_params; ++i) {
	  DWORD reg = pToken[i] & 0x00001FFF;
	  DWORD regtype = ((pToken[i] & D3DSP_REGTYPE_MASK) >> D3DSP_REGTYPE_SHIFT);

	  switch (regtype << D3DSP_REGTYPE_SHIFT) {
	  case D3DSPR_TEMP:
	    /*DPRINTF("p[%d]=R[%d]\n", i, reg);*/
	    p[i] = &R[reg];
	    break;
	  case D3DSPR_INPUT:
	    /*DPRINTF("p[%d]=V[%s]\n", i, VertexShaderDeclRegister[reg]);*/
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
	    /*DPRINTF("p[%d]=A[%d]\n", i, reg);*/
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
	    /*DPRINTF("p[%d]=oD[%d]\n", i, reg);*/
	    p[i] = &output->oD[reg];
	    break;
	  case D3DSPR_TEXCRDOUT:
	    /*DPRINTF("p[%d]=oT[%d]\n", i, reg);*/
	    p[i] = &output->oT[reg];
	    break;
	  default:
	    break;
	  }
	  
	  if (i > 0) { /* input reg */
	    DWORD swizzle = (pToken[i] & D3DVS_SWIZZLE_MASK) >> D3DVS_SWIZZLE_SHIFT;
	    UINT isNegative = ((*pToken & D3DSP_SRCMOD_MASK) == D3DSPSM_NEG);

	    if (!isNegative && (D3DVS_NOSWIZZLE >> D3DVS_SWIZZLE_SHIFT) == swizzle) {
	      /*DPRINTF("p[%d] not swizzled\n", i);*/
	      p_send[i] = p[i];
	    } else {
	      DWORD swizzle_x = swizzle & 0x03;
	      DWORD swizzle_y = (swizzle >> 2) & 0x03;
	      DWORD swizzle_z = (swizzle >> 4) & 0x03;
	      DWORD swizzle_w = (swizzle >> 6) & 0x03;
	      /*DPRINTF("p[%d] swizzled\n", i);*/
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
      
      /*
      TRACE_VECTOR(output->oPos);
      TRACE_VECTOR(output->oD[0]);
      TRACE_VECTOR(output->oD[1]);
      TRACE_VECTOR(output->oT[0]);
      TRACE_VECTOR(output->oT[1]);
      TRACE_VECTOR(R[0]);
      TRACE_VECTOR(R[1]);
      TRACE_VECTOR(R[2]);
      TRACE_VECTOR(R[3]);
      TRACE_VECTOR(R[4]);
      */

      /* to next opcode token */
      pToken += curOpcode->num_params;
    }
  }
  return TRUE;
}

/************************************
 * Vertex Shader Declaration Parser First draft ...
 */

/** todo check decl validity */
DWORD vshader_decl_parse_token(const DWORD* pToken) {
  const DWORD token = *pToken;
  DWORD tokenlen = 1;

  switch ((token & D3DVSD_TOKENTYPEMASK) >> D3DVSD_TOKENTYPESHIFT) { /* maybe a macro to inverse ... */
  case D3DVSD_TOKEN_NOP:
    TRACE(" 0x%08lx NOP()\n", token);
    break;
  case D3DVSD_TOKEN_STREAM:
    if (token & D3DVSD_STREAMTESSMASK) {
      TRACE(" 0x%08lx STREAM_TESS()\n", token);
    } else {
      TRACE(" 0x%08lx STREAM(%lu)\n", token, ((token & D3DVSD_STREAMNUMBERMASK) >> D3DVSD_STREAMNUMBERSHIFT));
    }
    break;
  case D3DVSD_TOKEN_STREAMDATA:
    if (token & 0x10000000) {
      TRACE(" 0x%08lx SKIP(%lu)\n", token, ((token & D3DVSD_SKIPCOUNTMASK) >> D3DVSD_SKIPCOUNTSHIFT));
    } else {
      DWORD type = ((token & D3DVSD_DATATYPEMASK)  >> D3DVSD_DATATYPESHIFT);
      DWORD reg  = ((token & D3DVSD_VERTEXREGMASK) >> D3DVSD_VERTEXREGSHIFT);
      TRACE(" 0x%08lx REG(%s, %s)\n", token, VertexShaderDeclRegister[reg], VertexShaderDeclDataTypes[type]);
    }
    break;
  case D3DVSD_TOKEN_TESSELLATOR:
    if (token & 0x10000000) {
      DWORD type = ((token & D3DVSD_DATATYPEMASK)  >> D3DVSD_DATATYPESHIFT);
      DWORD reg  = ((token & D3DVSD_VERTEXREGMASK) >> D3DVSD_VERTEXREGSHIFT);
      TRACE(" 0x%08lx TESSUV(%s) as %s\n", token, VertexShaderDeclRegister[reg], VertexShaderDeclDataTypes[type]);
    } else {
      DWORD type   = ((token & D3DVSD_DATATYPEMASK)    >> D3DVSD_DATATYPESHIFT);
      DWORD regout = ((token & D3DVSD_VERTEXREGMASK)   >> D3DVSD_VERTEXREGSHIFT);
      DWORD regin  = ((token & D3DVSD_VERTEXREGINMASK) >> D3DVSD_VERTEXREGINSHIFT);
      TRACE(" 0x%08lx TESSNORMAL(%s, %s) as %s\n", token, VertexShaderDeclRegister[regin], VertexShaderDeclRegister[regout], VertexShaderDeclDataTypes[type]);
    }
    break;
  case D3DVSD_TOKEN_CONSTMEM:
    {
      DWORD i;
      DWORD count        = ((token & D3DVSD_CONSTCOUNTMASK)   >> D3DVSD_CONSTCOUNTSHIFT);
      DWORD constaddress = ((token & D3DVSD_CONSTADDRESSMASK) >> D3DVSD_CONSTADDRESSSHIFT);
      TRACE(" 0x%08lx CONST(%lu, %lu)\n", token, constaddress, count);
      ++pToken;
      for (i = 0; i < count; ++i) {
	TRACE("        c[%lu] = (0x%08lx, 0x%08lx, 0x%08lx, 0x%08lx)\n", 
		constaddress, 
		*pToken, 
		*(pToken + 1), 
		*(pToken + 2), 
		*(pToken + 3));
	pToken += 4; 
	++constaddress;
      }
      tokenlen = count + 1;
    }
    break;
  case D3DVSD_TOKEN_EXT:
    {
      DWORD count   = ((token & D3DVSD_CONSTCOUNTMASK) >> D3DVSD_CONSTCOUNTSHIFT);
      DWORD extinfo = ((token & D3DVSD_EXTINFOMASK)    >> D3DVSD_EXTINFOSHIFT);
      TRACE(" 0x%08lx EXT(%lu, %lu)\n", token, count, extinfo);
      /* todo ... print extension */
      tokenlen = count + 1;
    }
    break;
  case D3DVSD_TOKEN_END:
    TRACE(" 0x%08lx END()\n", token);
    break;
  default:
    TRACE(" 0x%08lx UNKNOWN\n", token);
    /* argg error */
  }
  return tokenlen;
}

DWORD vshader_decl_parse(VERTEXSHADER8* vshader) {
  /** parser data */
  const DWORD* pToken = vshader->decl;
  DWORD fvf = 0;
  DWORD len = 0;  
  DWORD stream = 0;
  DWORD token;
  DWORD tokenlen;
  DWORD tokentype;
  DWORD tex = D3DFVF_TEX0;

  while (D3DVSD_END() != *pToken) {
    token = *pToken;
    tokenlen = vshader_decl_parse_token(pToken); 
    tokentype = ((token & D3DVSD_TOKENTYPEMASK) >> D3DVSD_TOKENTYPESHIFT);
    
    /** FVF generation block */
    if (D3DVSD_TOKEN_STREAM == tokentype && 0 == (D3DVSD_STREAMTESSMASK & token)) {
      /** 
       * how really works streams, 
       *  in DolphinVS dx8 dsk sample they seems to decal reg numbers !!!
       */
      stream     = ((token & D3DVSD_STREAMNUMBERMASK) >> D3DVSD_STREAMNUMBERSHIFT);

    } else if (D3DVSD_TOKEN_STREAMDATA == tokentype && 0 == (0x10000000 & tokentype)) {
      DWORD type = ((token & D3DVSD_DATATYPEMASK)  >> D3DVSD_DATATYPESHIFT);
      DWORD reg  = ((token & D3DVSD_VERTEXREGMASK) >> D3DVSD_VERTEXREGSHIFT) - stream;

      switch (reg) {
      case D3DVSDE_POSITION:     
	switch (type) {
	case D3DVSDT_FLOAT3:     fvf |= D3DFVF_XYZ;             break;
	case D3DVSDT_FLOAT4:     fvf |= D3DFVF_XYZRHW;          break;
	default: /** errooooorr what to do ? */
	  ERR("Error in VertexShader declaration of D3DVSDE_POSITION register: unsupported type %s\n", VertexShaderDeclDataTypes[type]);	  
	}
	break;

      case D3DVSDE_BLENDWEIGHT:
	switch (type) {
	case D3DVSDT_FLOAT1:     fvf |= D3DFVF_XYZB1;           break;
	case D3DVSDT_FLOAT2:     fvf |= D3DFVF_XYZB2;           break;
	case D3DVSDT_FLOAT3:     fvf |= D3DFVF_XYZB3;           break;
	case D3DVSDT_FLOAT4:     fvf |= D3DFVF_XYZB4;           break;
	default: /** errooooorr what to do ? */
	  ERR("Error in VertexShader declaration of D3DVSDE_BLENDWEIGHT register: unsupported type %s\n", VertexShaderDeclDataTypes[type]);
	}
	break;

      case D3DVSDE_BLENDINDICES: /* seem to be B5 as said in MSDN Dx9SDK ??  */
	switch (type) {
	case D3DVSDT_UBYTE4:     fvf |= D3DFVF_LASTBETA_UBYTE4;           break;
	default: /** errooooorr what to do ? */
	  ERR("Error in VertexShader declaration of D3DVSDE_BLENDINDINCES register: unsupported type %s\n", VertexShaderDeclDataTypes[type]);
	}
	break; 

      case D3DVSDE_NORMAL: /* TODO: only FLOAT3 supported ... another choice possible ? */
	switch (type) {
	case D3DVSDT_FLOAT3:     fvf |= D3DFVF_NORMAL;          break;
	default: /** errooooorr what to do ? */
	  ERR("Error in VertexShader declaration of D3DVSDE_NORMAL register: unsupported type %s\n", VertexShaderDeclDataTypes[type]);
	}
	break; 

      case D3DVSDE_PSIZE:  /* TODO: only FLOAT1 supported ... another choice possible ? */
	switch (type) {
        case D3DVSDT_FLOAT1:     fvf |= D3DFVF_PSIZE;           break;
	default: /** errooooorr what to do ? */
	  ERR("Error in VertexShader declaration of D3DVSDE_PSIZE register: unsupported type %s\n", VertexShaderDeclDataTypes[type]);
	}
	break;

      case D3DVSDE_DIFFUSE:  /* TODO: only D3DCOLOR supported */
	switch (type) {
	case D3DVSDT_D3DCOLOR:   fvf |= D3DFVF_DIFFUSE;         break;
	default: /** errooooorr what to do ? */
	  ERR("Error in VertexShader declaration of D3DVSDE_DIFFUSE register: unsupported type %s\n", VertexShaderDeclDataTypes[type]);
	}
	break;

      case D3DVSDE_SPECULAR:  /* TODO: only D3DCOLOR supported */
	switch (type) {
	case D3DVSDT_D3DCOLOR:	 fvf |= D3DFVF_SPECULAR;        break;
	default: /** errooooorr what to do ? */
	  ERR("Error in VertexShader declaration of D3DVSDE_SPECULAR register: unsupported type %s\n", VertexShaderDeclDataTypes[type]);
	}
	break;

	/**
	 * TODO: for TEX* only FLOAT2 supported
	 *  by default using texture type info
	 */
      case D3DVSDE_TEXCOORD0:    tex = max(tex, D3DFVF_TEX1);   break; 
      case D3DVSDE_TEXCOORD1:    tex = max(tex, D3DFVF_TEX2);   break;
      case D3DVSDE_TEXCOORD2:    tex = max(tex, D3DFVF_TEX3);   break;
      case D3DVSDE_TEXCOORD3:    tex = max(tex, D3DFVF_TEX4);   break;
      case D3DVSDE_TEXCOORD4:    tex = max(tex, D3DFVF_TEX5);   break;
      case D3DVSDE_TEXCOORD5:    tex = max(tex, D3DFVF_TEX6);   break;
      case D3DVSDE_TEXCOORD6:    tex = max(tex, D3DFVF_TEX7);   break;
      case D3DVSDE_TEXCOORD7:    tex = max(tex, D3DFVF_TEX8);   break;
      case D3DVSDE_POSITION2:   /* maybe D3DFVF_XYZRHW instead D3DFVF_XYZ (of D3DVDE_POSITION) ... to see */
      case D3DVSDE_NORMAL2:     /* FIXME i don't know what to do here ;( */
	FIXME("[%lu] registers in VertexShader declaration not supported yet (token:0x%08lx)\n", reg, token);
	break;
      }
      /*TRACE("VertexShader declaration define %x as current FVF\n", fvf);*/
    }
    len += tokenlen;
    pToken += tokenlen;
  }
  if (tex > 0) {
    /*TRACE("VertexShader declaration define %x as texture level\n", tex);*/
    fvf |= tex;
  }   
  /* here D3DVSD_END() */
  len += vshader_decl_parse_token(pToken);
  vshader->fvf = fvf;
  vshader->declLength = len * sizeof(DWORD);
  return len * sizeof(DWORD);
}


void vshader_fill_input(VERTEXSHADER8* vshader,
			IDirect3DDevice8Impl* device,
			const void* vertexFirstStream,
			DWORD StartVertexIndex, 
			DWORD idxDecal) {
  /** parser data */
  const DWORD* pToken = vshader->decl;
  DWORD stream = 0;
  DWORD token;
  /*DWORD tokenlen;*/
  DWORD tokentype;
  /** for input readers */
  const char* curPos = NULL;
  FLOAT x, y, z, w;
  SHORT u, v, r, t;
  DWORD dw;

  /*TRACE("(%p) - device:%p - stream:%p, startIdx=%lu, idxDecal=%lu\n", vshader, device, vertexFirstStream, StartVertexIndex, idxDecal);*/
  while (D3DVSD_END() != *pToken) {
    token = *pToken;
    tokentype = ((token & D3DVSD_TOKENTYPEMASK) >> D3DVSD_TOKENTYPESHIFT);
    
    /** FVF generation block */
    if (D3DVSD_TOKEN_STREAM == tokentype && 0 == (D3DVSD_STREAMTESSMASK & token)) {
      IDirect3DVertexBuffer8* pVB;
      const char* startVtx = NULL;
      int skip = 0;

      ++pToken;
      /** 
       * how really works streams, 
       *  in DolphinVS dx8 dsk sample use it !!!
       */
      stream = ((token & D3DVSD_STREAMNUMBERMASK) >> D3DVSD_STREAMNUMBERSHIFT);

      if (0 == stream) {
	skip = device->StateBlock.stream_stride[0];
	startVtx = (const char *)vertexFirstStream + (StartVertexIndex * skip);
	curPos = startVtx + idxDecal;
	/*TRACE(" using stream[%lu] with %lu decal => curPos %p\n", stream, idxDecal, curPos);*/
      } else {
	skip = device->StateBlock.stream_stride[stream];
	pVB  = device->StateBlock.stream_source[stream];

	if (NULL == pVB) {
	  ERR("using unitialised stream[%lu]\n", stream);
	  return ;
	} else {
	  startVtx = ((IDirect3DVertexBuffer8Impl*) pVB)->allocatedMemory + (StartVertexIndex * skip);
	  /** do we need to decal if we use idxBuffer */
	  curPos = startVtx + idxDecal;
	  /*TRACE(" using stream[%lu] with %lu decal\n", stream, idxDecal);*/
	}
      }
    } else if (D3DVSD_TOKEN_CONSTMEM == tokentype) {
      /** Const decl */
      DWORD i;
      DWORD count        = ((token & D3DVSD_CONSTCOUNTMASK)   >> D3DVSD_CONSTCOUNTSHIFT);
      DWORD constaddress = ((token & D3DVSD_CONSTADDRESSMASK) >> D3DVSD_CONSTADDRESSSHIFT);
      ++pToken;
      for (i = 0; i < count; ++i) {
	vshader->data->C[constaddress + i].x = *(float*)pToken;
	vshader->data->C[constaddress + i].y = *(float*)(pToken + 1);
	vshader->data->C[constaddress + i].z = *(float*)(pToken + 2);
	vshader->data->C[constaddress + i].w = *(float*)(pToken + 3);
	pToken += 4;
      }

    } else if (D3DVSD_TOKEN_STREAMDATA == tokentype && 0 != (0x10000000 & tokentype)) {
      /** skip datas */
      DWORD skipCount = ((token & D3DVSD_SKIPCOUNTMASK) >> D3DVSD_SKIPCOUNTSHIFT);
      curPos = curPos + skipCount * sizeof(DWORD);
      ++pToken;

    } else if (D3DVSD_TOKEN_STREAMDATA == tokentype && 0 == (0x10000000 & tokentype)) {
      DWORD type = ((token & D3DVSD_DATATYPEMASK)  >> D3DVSD_DATATYPESHIFT);
      DWORD reg  = ((token & D3DVSD_VERTEXREGMASK) >> D3DVSD_VERTEXREGSHIFT);
      ++pToken;

      switch (type) {
      case D3DVSDT_FLOAT1:
	x = *(float*) curPos;
	curPos = curPos + sizeof(float);
	/**/
	vshader->input.V[reg].x = x;
	vshader->input.V[reg].y = 0.0f;
	vshader->input.V[reg].z = 0.0f;
	vshader->input.V[reg].w = 1.0f;
	break;

      case D3DVSDT_FLOAT2:
	x = *(float*) curPos;
	curPos = curPos + sizeof(float);
	y = *(float*) curPos;
	curPos = curPos + sizeof(float);
	/**/
	vshader->input.V[reg].x = x;
	vshader->input.V[reg].y = y;
	vshader->input.V[reg].z = 0.0f;
	vshader->input.V[reg].w = 1.0f;
	break;

      case D3DVSDT_FLOAT3: 
	x = *(float*) curPos;
	curPos = curPos + sizeof(float);
	y = *(float*) curPos;
	curPos = curPos + sizeof(float);
	z = *(float*) curPos;
	curPos = curPos + sizeof(float);
	/**/
	vshader->input.V[reg].x = x;
	vshader->input.V[reg].y = y;
	vshader->input.V[reg].z = z;
	vshader->input.V[reg].w = 1.0f;
	break;

      case D3DVSDT_FLOAT4: 
	x = *(float*) curPos;
	curPos = curPos + sizeof(float);
	y = *(float*) curPos;
	curPos = curPos + sizeof(float);
	z = *(float*) curPos;
	curPos = curPos + sizeof(float);
	w = *(float*) curPos;
	curPos = curPos + sizeof(float);
	/**/
	vshader->input.V[reg].x = x;
	vshader->input.V[reg].y = y;
	vshader->input.V[reg].z = z;
	vshader->input.V[reg].w = w;
	break;

      case D3DVSDT_D3DCOLOR: 
	dw = *(DWORD*) curPos;
	curPos = curPos + sizeof(DWORD);
	/**/
	vshader->input.V[reg].x = (float) (((dw >> 16) & 0xFF) / 255.0f);
	vshader->input.V[reg].y = (float) (((dw >>  8) & 0xFF) / 255.0f);
	vshader->input.V[reg].z = (float) (((dw >>  0) & 0xFF) / 255.0f);
	vshader->input.V[reg].w = (float) (((dw >> 24) & 0xFF) / 255.0f);
	break;

      case D3DVSDT_SHORT2: 
	u = *(SHORT*) curPos;
	curPos = curPos + sizeof(SHORT);
	v = *(SHORT*) curPos;
	curPos = curPos + sizeof(SHORT);
	/**/
	vshader->input.V[reg].x = (float) u;
	vshader->input.V[reg].y = (float) v;
	vshader->input.V[reg].z = 0.0f;
	vshader->input.V[reg].w = 1.0f;
	break;

      case D3DVSDT_SHORT4: 
	u = *(SHORT*) curPos;
	curPos = curPos + sizeof(SHORT);
	v = *(SHORT*) curPos;
	curPos = curPos + sizeof(SHORT);
	r = *(SHORT*) curPos;
	curPos = curPos + sizeof(SHORT);
	t = *(SHORT*) curPos;
	curPos = curPos + sizeof(SHORT);
	/**/
	vshader->input.V[reg].x = (float) u;
	vshader->input.V[reg].y = (float) v;
	vshader->input.V[reg].z = (float) r;
	vshader->input.V[reg].w = (float) t;
	break;

      case D3DVSDT_UBYTE4: 
	dw = *(DWORD*) curPos;
	curPos = curPos + sizeof(DWORD);
	/**/
	vshader->input.V[reg].x = (float) ((dw & 0x000F) >>  0);
	vshader->input.V[reg].y = (float) ((dw & 0x00F0) >>  8);
	vshader->input.V[reg].z = (float) ((dw & 0x0F00) >> 16);
	vshader->input.V[reg].w = (float) ((dw & 0xF000) >> 24);
	
	break;

      default: /** errooooorr what to do ? */
	ERR("Error in VertexShader declaration of %s register: unsupported type %s\n", VertexShaderDeclRegister[reg], VertexShaderDeclDataTypes[type]);
      }
    }

  }
  /* here D3DVSD_END() */
}


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
BOOL WINAPI ValidatePixelShader(LPVOID what) {
  FIXME("(void): stub: %p\n", what);
  return TRUE;
}
