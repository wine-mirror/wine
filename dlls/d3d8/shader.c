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

typedef void (*shader_fct0_t)(void);
typedef void (*shader_fct1_t)(SHADER8Vector*);
typedef void (*shader_fct2_t)(SHADER8Vector*,SHADER8Vector*);
typedef void (*shader_fct3_t)(SHADER8Vector*,SHADER8Vector*,SHADER8Vector*);
typedef void (*shader_fct4_t)(SHADER8Vector*,SHADER8Vector*,SHADER8Vector*,SHADER8Vector*);

/*
typedef union shader_fct {
  shader_fct0_t fct0;
  shader_fct1_t fct1;
  shader_fct2_t fct2;
  shader_fct3_t fct3;
  shader_fct4_t fct4;
} shader_fct;
*/
typedef void (*shader_fct)();

typedef struct shader_opcode {
  CONST BYTE  opcode;
  const char* name;
  CONST UINT  num_params;
  shader_fct  soft_fct;
  /*
  union {
    shader_fct0_t fct0;
    shader_fct1_t fct1;
    shader_fct2_t fct2;
    shader_fct3_t fct3;
    shader_fct4_t fct4;
  } shader_fct;
  */
} shader_opcode;

typedef struct vshader_input_data {
  /*SHADER8Vector V[16];//0-15 */
  SHADER8Vector V0;
  SHADER8Vector V1;
  SHADER8Vector V2;
  SHADER8Vector V3;
  SHADER8Vector V4;
  SHADER8Vector V5;
  SHADER8Vector V6;
  SHADER8Vector V7;
  SHADER8Vector V8;
  SHADER8Vector V9;
  SHADER8Vector V10;
  SHADER8Vector V11;
  SHADER8Vector V12;
  SHADER8Vector V13;
  SHADER8Vector V14;
  SHADER8Vector V15;
} vshader_input_data;

typedef struct vshader_output_data {
  SHADER8Vector oPos;
  /*SHADER8Vector oD[2];//0-1 */
  SHADER8Vector oD0;
  SHADER8Vector oD1;
  /*SHADER8Vector oT[4];//0-3 */
  SHADER8Vector oT0;
  SHADER8Vector oT1;
  SHADER8Vector oT2;
  SHADER8Vector oT3;
  SHADER8Scalar oFog;
  SHADER8Scalar oPts;
} vshader_output_data;

/*********************
 * vshader software VM
 */

void vshader_add(SHADER8Vector* d, SHADER8Vector* s0, SHADER8Vector* s1) {
  d->x = s0->x + s1->x;
  d->y = s0->y + s1->y;
  d->z = s0->z + s1->z;
  d->w = s0->w + s1->w;
}

void vshader_dp3(SHADER8Vector* d, SHADER8Vector* s0, SHADER8Vector* s1) {
  d->x = d->y = d->z = d->w = s0->x * s1->x + s0->y * s1->y + s0->z * s1->z;
}

void vshader_dp4(SHADER8Vector* d, SHADER8Vector* s0, SHADER8Vector* s1) {
  d->x = d->y = d->z = d->w = s0->x * s1->x + s0->y * s1->y + s0->z * s1->z + s0->w * s1->w;
}

void vshader_dst(SHADER8Vector* d, SHADER8Vector* s0, SHADER8Vector* s1) {
  d->x = 1;
  d->y = s0->y * s1->y;
  d->z = s0->z;
  d->w = s1->w;
}

void vshader_expp(SHADER8Vector* d, SHADER8Vector* s0) {
  float tmp_f = floorf(s0->w);
  d->x = pow(2, tmp_f);
  d->y = s0->w - tmp_f;
  d->z = pow(2, s0->w);
  d->w = 1;
}

void vshader_lit(SHADER8Vector* d, SHADER8Vector* s0) {
  d->x = 1;
  d->y = (0 < s0->x) ? s0->x : 0;
  d->z = (0 < s0->x && 0 < s0->y) ? pow(s0->y, s0->w) : 0;
  d->w = 1;
}

void vshader_logp(SHADER8Vector* d, SHADER8Vector* s0) {
  d->x = d->y = d->z = d->w = (0 != s0->w) ? log(fabsf(s0->w))/log(2) : HUGE;
}

void vshader_mad(SHADER8Vector* d, SHADER8Vector* s0, SHADER8Vector* s1, SHADER8Vector* s2) {
  d->x = s0->x * s1->x + s2->x;
  d->y = s0->y * s1->y + s2->y;
  d->z = s0->z * s1->z + s2->z;
  d->w = s0->w * s1->w + s2->w;
}

void vshader_max(SHADER8Vector* d, SHADER8Vector* s0, SHADER8Vector* s1) {
  d->x = (s0->x >= s1->x) ? s0->x : s1->x;
  d->y = (s0->y >= s1->y) ? s0->y : s1->y;
  d->z = (s0->z >= s1->z) ? s0->z : s1->z;
  d->w = (s0->w >= s1->w) ? s0->w : s1->w;
}

void vshader_min(SHADER8Vector* d, SHADER8Vector* s0, SHADER8Vector* s1) {
  d->x = (s0->x < s1->x) ? s0->x : s1->x;
  d->y = (s0->y < s1->y) ? s0->y : s1->y;
  d->z = (s0->z < s1->z) ? s0->z : s1->z;
  d->w = (s0->w < s1->w) ? s0->w : s1->w;
}

void vshader_mov(SHADER8Vector* d, SHADER8Vector* s0) {
  d->x = s0->x;
  d->y = s0->y;
  d->z = s0->z;
  d->w = s0->w;
}

void vshader_mul(SHADER8Vector* d, SHADER8Vector* s0, SHADER8Vector* s1) {
  d->x = s0->x * s1->x;
  d->y = s0->y * s1->y;
  d->z = s0->z * s1->z;
  d->w = s0->w * s1->w;
}

void vshader_nop(void) {
  /* NOPPPP ahhh too easy ;) */
}

void vshader_rcp(SHADER8Vector* d, SHADER8Vector* s0) {
  d->x = d->y = d->z = d->w = (0 == s0->w) ? HUGE : 1 / s0->w;
}

void vshader_rsq(SHADER8Vector* d, SHADER8Vector* s0) {
  d->x = d->y = d->z = d->w = (0 == s0->w) ? HUGE : 1 / sqrt(fabsf(s0->w));
}

void vshader_sge(SHADER8Vector* d, SHADER8Vector* s0, SHADER8Vector* s1) {
  d->x = (s0->x >= s1->x) ? 1 : 0;
  d->y = (s0->y >= s1->y) ? 1 : 0;
  d->z = (s0->z >= s1->z) ? 1 : 0;
  d->w = (s0->w >= s1->w) ? 1 : 0;
}

void vshader_slt(SHADER8Vector* d, SHADER8Vector* s0, SHADER8Vector* s1) {
  d->x = (s0->x < s1->x) ? 1 : 0;
  d->y = (s0->y < s1->y) ? 1 : 0;
  d->z = (s0->z < s1->z) ? 1 : 0;
  d->w = (s0->w < s1->w) ? 1 : 0;
}

void vshader_sub(SHADER8Vector* d, SHADER8Vector* s0, SHADER8Vector* s1) {
  d->x = s0->x - s1->x;
  d->y = s0->y - s1->y;
  d->z = s0->z - s1->z;
  d->w = s0->w - s1->w;
}

/**
 * log, exp, frc, m*x* seems to be macros ins ... to see
 *
 * @TODO: find this fucking really opcodes values
 */
static CONST shader_opcode vshader_ins [] =
  {
    {0, "mov",  2, vshader_mov},
    {0, "max",  3, vshader_max},
    {0, "min",  3, vshader_min},
    {0, "sge",  3, vshader_sge},
    {0, "slt",  3, vshader_slt},
    {0, "add",  3, vshader_add},
    {0, "sub",  3, vshader_sub},
    {0, "mul",  3, vshader_mul},
    {0, "rcp",  2, vshader_rcp},
    {0, "mad",  4, vshader_mad},
    {0, "dp3",  3, vshader_dp3},
    {0, "dp4",  3, vshader_dp4},
    {0, "rsq",  2, vshader_rsq},
    {0, "dst",  3, vshader_dst},
    {0, "lit",  2, vshader_lit},
    {0, "expp", 2, vshader_expp},
    {0, "logp", 2, vshader_logp},
    {0, "nop",  0, vshader_nop},
    {0, NULL, 0, NULL}
  };


shader_opcode* vshader_get_opcode(const DWORD code) {
  return NULL;
}

/**
 * Function parser ...
 */
BOOL vshader_parse_function(const DWORD* function) {
  return TRUE;
}

BOOL vshader_hardware_execute_function(VERTEXSHADER8* vshader,
				       const vshader_input_data* input,
				       vshader_output_data* output) {
  /** 
   * TODO: use the GL_NV_vertex_program 
   *  and specifics vendors variants for it 
   */
  return TRUE;
}

BOOL vshader_software_execute_function(VERTEXSHADER8* vshader,
				       const vshader_input_data* input,
				       vshader_output_data* output) {
  /** Vertex Shader Temporary Registers */
  /*SHADER8Vector R[12];*/
  /*SHADER8Scalar A0;*/
  /** temporary Vector for modifier management */
  /*SHADER8Vector d;*/
  /** parser datas */
  const DWORD* pToken = vshader->function;
  shader_opcode* curOpcode = NULL;
  
  /* the first dword is the version tag */
  /* TODO: parse it */
  
  ++pToken;
  while (0xFFFFFFFF != *pToken) {
    curOpcode = vshader_get_opcode(*pToken);
    ++pToken;
    if (NULL == curOpcode) {
      /* unkown current opcode ... */
      return FALSE;
    }
    if (curOpcode->num_params > 0) {
      /* TODO */
    }
  }
  return TRUE;
}

/**
 * Declaration Parser First draft ...
 */

#if 0
static CONST char* vshader_decl [] =
  {
    "D3DVSDT_D3DCOLOR",
    "D3DVSDT_FLOAT1",
    "D3DVSDT_FLOAT2",
    "D3DVSDT_FLOAT3",
    "D3DVSDT_FLOAT4",
    "D3DVSDT_UBYTE4",
    NULL
  };
#endif

/** Vertex Shader Declaration parser tokens */
enum D3DVSD_TOKENS {
  D3DVSD_STREAM,
  
  D3DVSD_END
};

BOOL vshader_parse_declaration(VERTEXSHADER8* vshader) {
  /** parser data */
  const DWORD* pToken = vshader->decl;

  ++pToken;
  while (0xFFFFFFFF != *pToken) {
    /** TODO */
    ++pToken;
  }
  return TRUE;
}

HRESULT WINAPI ValidatePixelShader(void) {
  FIXME("(void): stub\n");
  return 0;
}

HRESULT WINAPI ValidateVertexShader(void) {
  FIXME("(void): stub\n");
  return 0;
}
