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
  d->x = powf(2.0f, tmp_f);
  d->y = s0->w - tmp_f;
  d->z = powf(2.0f, s0->w);
  d->w = 1.0f;

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
 * log, exp, frc, m*x* seems to be macros ins ... to see
 */
static CONST SHADER_OPCODE vshader_ins [] = {
  {D3DSIO_MOV,  "mov",  2, vshader_mov},
  {D3DSIO_MAX,  "max",  3, vshader_max},
  {D3DSIO_MIN,  "min",  3, vshader_min},
  {D3DSIO_SGE,  "sge",  3, vshader_sge},
  {D3DSIO_SLT,  "slt",  3, vshader_slt},
  {D3DSIO_ADD,  "add",  3, vshader_add},
  {D3DSIO_SUB,  "sub",  3, vshader_sub},
  {D3DSIO_MUL,  "mul",  3, vshader_mul},
  {D3DSIO_RCP,  "rcp",  2, vshader_rcp},
  {D3DSIO_MAD,  "mad",  4, vshader_mad},
  {D3DSIO_DP3,  "dp3",  3, vshader_dp3},
  {D3DSIO_DP4,  "dp4",  3, vshader_dp4},
  {D3DSIO_RSQ,  "rsq",  2, vshader_rsq},
  {D3DSIO_DST,  "dst",  3, vshader_dst},
  {D3DSIO_LIT,  "lit",  2, vshader_lit},
  {D3DSIO_EXPP, "expp", 2, vshader_expp},
  {D3DSIO_LOGP, "logp", 2, vshader_logp},
  {D3DSIO_NOP,  "nop",  0, vshader_nop},
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
      curOpcode = vshader_program_get_opcode(*pToken);
      ++pToken;
      ++len;
      if (NULL == curOpcode) {
	/* unkown current opcode ... */
	while (*pToken & 0x80000000) {
	  DPRINTF("unrecognized opcode: %08lX\n", *pToken);
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
  
  while (D3DVS_END() != *pToken) {
    curOpcode = vshader_program_get_opcode(*pToken);
    ++pToken;
    if (NULL == curOpcode) {
      /* unkown current opcode ... */
      while (*pToken & 0x80000000) {
	DPRINTF("unrecognized opcode: pos=%d token=%08lX\n", pToken - vshader->function, *pToken);
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
  DWORD token;
  DWORD tokenlen;
  DWORD tokentype;
  DWORD tex = D3DFVF_TEX0;

  while (D3DVSD_END() != *pToken) {
    token = *pToken;
    tokenlen = vshader_decl_parse_token(pToken); 
    tokentype = ((*pToken & D3DVSD_TOKENTYPEMASK) >> D3DVSD_TOKENTYPESHIFT);
    
    /** FVF generation block */
    if (D3DVSD_TOKEN_STREAMDATA == tokentype && 0 == (0x10000000 & tokentype)) {
      DWORD type = ((token & D3DVSD_DATATYPEMASK)  >> D3DVSD_DATATYPESHIFT);
      DWORD reg  = ((token & D3DVSD_VERTEXREGMASK) >> D3DVSD_VERTEXREGSHIFT);

      switch (reg) {
      case D3DVSDE_POSITION:     
	switch (type) {
	case D3DVSDT_FLOAT3:     fvf |= D3DFVF_XYZ;             break;
	case D3DVSDT_FLOAT4:     fvf |= D3DFVF_XYZRHW;          break;
	default: /** errooooorr what to do ? */
	  ERR("Error in VertexShader declaration of D3DVSDE_POSITION register: unsupported type %lu\n", type);	  
	}
	break;

      case D3DVSDE_BLENDWEIGHT:
	switch (type) {
	case D3DVSDT_FLOAT1:     fvf |= D3DFVF_XYZB1;           break;
	case D3DVSDT_FLOAT2:     fvf |= D3DFVF_XYZB2;           break;
	case D3DVSDT_FLOAT3:     fvf |= D3DFVF_XYZB3;           break;
	case D3DVSDT_FLOAT4:     fvf |= D3DFVF_XYZB4;           break;
	default: /** errooooorr what to do ? */
	  ERR("Error in VertexShader declaration of D3DVSDE_BLENDWEIGHT register: unsupported type %lu\n", type);
	}
	break;

      case D3DVSDE_BLENDINDICES: /* seem to be B5 as said in MSDN Dx9SDK ??  */
	switch (type) {
	case D3DVSDT_UBYTE4:     fvf |= D3DFVF_LASTBETA_UBYTE4;           break;
	default: /** errooooorr what to do ? */
	  ERR("Error in VertexShader declaration of D3DVSDE_BLENDINDINCES register: unsupported type %lu\n", type);
	}
	break; 

      case D3DVSDE_NORMAL: /* TODO: only FLOAT3 supported ... another choice possible ? */
	switch (type) {
	case D3DVSDT_FLOAT3:     fvf |= D3DFVF_NORMAL;          break;
	default: /** errooooorr what to do ? */
	  ERR("Error in VertexShader declaration of D3DVSDE_NORMAL register: unsupported type %lu\n", type);
	}
	break; 

      case D3DVSDE_PSIZE:  /* TODO: only FLOAT1 supported ... another choice possible ? */
	switch (type) {
        case D3DVSDT_FLOAT1:     fvf |= D3DFVF_PSIZE;           break;
	default: /** errooooorr what to do ? */
	  ERR("Error in VertexShader declaration of D3DVSDE_PSIZE register: unsupported type %lu\n", type);
	}
	break;

      case D3DVSDE_DIFFUSE:  /* TODO: only D3DCOLOR supported */
	switch (type) {
	case D3DVSDT_D3DCOLOR:   fvf |= D3DFVF_DIFFUSE;         break;
	default: /** errooooorr what to do ? */
	  ERR("Error in VertexShader declaration of D3DVSDE_DIFFUSE register: unsupported type %lu\n", type);
	}
	break;

      case D3DVSDE_SPECULAR:  /* TODO: only D3DCOLOR supported */
	switch (type) {
	case D3DVSDT_D3DCOLOR:	 fvf |= D3DFVF_SPECULAR;        break;
	default: /** errooooorr what to do ? */
	  ERR("Error in VertexShader declaration of D3DVSDE_SPECULAR register: unsupported type %lu\n", type);
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

/***********************************************************************
 *		ValidateVertexShader (D3D8.@)
 */
BOOL WINAPI ValidateVertexShader(LPVOID what, LPVOID toto) {
  FIXME("(void): stub: %p\n", what);
  return TRUE;
}

/***********************************************************************
 *		ValidatePixelShader (D3D8.@)
 */
BOOL WINAPI ValidatePixelShader(LPVOID what, LPVOID toto) {
  FIXME("(void): stub: %p\n", what);
  return TRUE;
}
