/*
 * vertex shaders declaration implementation
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

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "wine/debug.h"

#include <math.h>

#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_shader);

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

/** Vertex Shader Declaration data types tokens */
#define MAX_VSHADER_DECL_TYPES 8
static CONST char* VertexShaderDeclDataTypes[] = {
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

static CONST char* VertexShaderDeclRegister[] = {
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

/** todo check decl validity */
/*inline static*/ DWORD Direct3DVextexShaderDeclarationImpl_ParseToken(const DWORD* pToken) {
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
#if 0
	TRACE("        c[%lu] = (0x%08lx, 0x%08lx, 0x%08lx, 0x%08lx)\n", 
		constaddress, 
		*pToken, 
		*(pToken + 1), 
		*(pToken + 2), 
		*(pToken + 3));
#endif
	TRACE("        c[%lu] = (%8f, %8f, %8f, %8f)\n", 
		constaddress, 
		 *(float*) pToken, 
		 *(float*) (pToken + 1), 
		 *(float*) (pToken + 2), 
		 *(float*) (pToken + 3));
	pToken += 4; 
	++constaddress;
      }
      tokenlen = (4 * count) + 1;
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

HRESULT WINAPI IDirect3DDeviceImpl_CreateVertexShaderDeclaration8(IDirect3DDevice8Impl* This, CONST DWORD* pDeclaration8, IDirect3DVertexShaderDeclarationImpl** ppVertexShaderDecl) {
  /** parser data */
  const DWORD* pToken = pDeclaration8;
  DWORD fvf = 0;
  DWORD len = 0;  
  DWORD stream = 0;
  DWORD token;
  DWORD tokenlen;
  DWORD tokentype;
  DWORD tex = D3DFVF_TEX0;
  /** TRUE if declaration can be matched by a fvf */
  IDirect3DVertexShaderDeclarationImpl* object;
  BOOL  invalid_fvf = FALSE;

  TRACE("(%p) :  pDeclaration8(%p)\n", This, pDeclaration8);

  object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DVertexShaderDeclarationImpl));
  /*object->lpVtbl = &Direct3DVextexShaderDeclaration8_Vtbl;*/
  object->device = This; /* FIXME: AddRef(This) */
  object->ref = 1;
  object->allFVF = 0;

  while (D3DVSD_END() != *pToken) {
    token = *pToken;
    tokenlen = Direct3DVextexShaderDeclarationImpl_ParseToken(pToken); 
    tokentype = ((token & D3DVSD_TOKENTYPEMASK) >> D3DVSD_TOKENTYPESHIFT);
    
    /** FVF generation block */
    if (D3DVSD_TOKEN_STREAM == tokentype && 0 == (D3DVSD_STREAMTESSMASK & token)) {
      /** 
       * how really works streams, 
       *  in DolphinVS dx8 dsk sample they seems to decal reg numbers !!!
       */
      DWORD oldStream = stream;
      stream = ((token & D3DVSD_STREAMNUMBERMASK) >> D3DVSD_STREAMNUMBERSHIFT);

      /* copy fvf if valid */
      if (FALSE == invalid_fvf) {
          fvf |= tex << D3DFVF_TEXCOUNT_SHIFT;
          tex = 0;
          object->fvf[oldStream] = fvf;
          object->allFVF |= fvf;
      } else {
          object->fvf[oldStream] = 0;
          tex = 0;
      }

      /* reset valid/invalid fvf */
      fvf = 0;
      invalid_fvf = FALSE;

    } else if (D3DVSD_TOKEN_STREAMDATA == tokentype && 0 == (0x10000000 & tokentype)) {
      DWORD type = ((token & D3DVSD_DATATYPEMASK)  >> D3DVSD_DATATYPESHIFT);
      DWORD reg  = ((token & D3DVSD_VERTEXREGMASK) >> D3DVSD_VERTEXREGSHIFT);

      switch (reg) {
      case D3DVSDE_POSITION:     
	switch (type) {
	case D3DVSDT_FLOAT3:     fvf |= D3DFVF_XYZ;             break;
	case D3DVSDT_FLOAT4:     fvf |= D3DFVF_XYZRHW;          break;
	default: 
	  /** errooooorr mismatched use of a register, invalid fvf computing */
	  invalid_fvf = TRUE;
	  if (type >= MAX_VSHADER_DECL_TYPES) {
	    TRACE("Mismatched use in VertexShader declaration of D3DVSDE_POSITION register: unsupported and unrecognized type %08lx\n", type);
	  } else {
	    TRACE("Mismatched use in VertexShader declaration of D3DVSDE_POSITION register: unsupported type %s\n", VertexShaderDeclDataTypes[type]);
	  }
	}
	break;
      
      case D3DVSDE_BLENDWEIGHT:
	switch (type) {
	case D3DVSDT_FLOAT1:     fvf |= D3DFVF_XYZB1;           break;
	case D3DVSDT_FLOAT2:     fvf |= D3DFVF_XYZB2;           break;
	case D3DVSDT_FLOAT3:     fvf |= D3DFVF_XYZB3;           break;
	case D3DVSDT_FLOAT4:     fvf |= D3DFVF_XYZB4;           break;
	default: 
	  /** errooooorr mismatched use of a register, invalid fvf computing */
	  invalid_fvf = TRUE;
	  TRACE("Mismatched use in VertexShader declaration of D3DVSDE_BLENDWEIGHT register: unsupported type %s\n", VertexShaderDeclDataTypes[type]);
	}
	break;

      case D3DVSDE_BLENDINDICES: /* seem to be B5 as said in MSDN Dx9SDK ??  */
	switch (type) {
	case D3DVSDT_UBYTE4:     fvf |= D3DFVF_LASTBETA_UBYTE4;           break;
	default: 
	  /** errooooorr mismatched use of a register, invalid fvf computing */
	  invalid_fvf = TRUE;
	  TRACE("Mismatched use in VertexShader declaration of D3DVSDE_BLENDINDINCES register: unsupported type %s\n", VertexShaderDeclDataTypes[type]);
	}
	break; 

      case D3DVSDE_NORMAL: /* TODO: only FLOAT3 supported ... another choice possible ? */
	switch (type) {
	case D3DVSDT_FLOAT3:     fvf |= D3DFVF_NORMAL;          break;
	default: 
	  /** errooooorr mismatched use of a register, invalid fvf computing */
	  invalid_fvf = TRUE;
	  TRACE("Mismatched use in VertexShader declaration of D3DVSDE_NORMAL register: unsupported type %s\n", VertexShaderDeclDataTypes[type]);
	}
	break; 

      case D3DVSDE_PSIZE:  /* TODO: only FLOAT1 supported ... another choice possible ? */
	switch (type) {
        case D3DVSDT_FLOAT1:     fvf |= D3DFVF_PSIZE;           break;
	default: 
	  /** errooooorr mismatched use of a register, invalid fvf computing */
	  invalid_fvf = TRUE;
	  TRACE("Mismatched use in VertexShader declaration of D3DVSDE_PSIZE register: unsupported type %s\n", VertexShaderDeclDataTypes[type]);
	}
	break;

      case D3DVSDE_DIFFUSE:  /* TODO: only D3DCOLOR supported */
	switch (type) {
	case D3DVSDT_D3DCOLOR:   fvf |= D3DFVF_DIFFUSE;         break;
	default: 
	  /** errooooorr mismatched use of a register, invalid fvf computing */
	  invalid_fvf = TRUE;
	  TRACE("Mismatched use in VertexShader declaration of D3DVSDE_DIFFUSE register: unsupported type %s\n", VertexShaderDeclDataTypes[type]);
	}
	break;

      case D3DVSDE_SPECULAR:  /* TODO: only D3DCOLOR supported */
	switch (type) {
	case D3DVSDT_D3DCOLOR:	 fvf |= D3DFVF_SPECULAR;        break;
	default: 
	  /** errooooorr mismatched use of a register, invalid fvf computing */
	  invalid_fvf = TRUE;
	  TRACE("Mismatched use in VertexShader declaration of D3DVSDE_SPECULAR register: unsupported type %s\n", VertexShaderDeclDataTypes[type]);
	}
	break;

      case D3DVSDE_TEXCOORD0:
      case D3DVSDE_TEXCOORD1:
      case D3DVSDE_TEXCOORD2:
      case D3DVSDE_TEXCOORD3:
      case D3DVSDE_TEXCOORD4:
      case D3DVSDE_TEXCOORD5:
      case D3DVSDE_TEXCOORD6:
      case D3DVSDE_TEXCOORD7:
         /* Fixme? - assume all tex coords in same stream */
         {
             int texNo = 1 + (reg - D3DVSDE_TEXCOORD0);
             tex = max(tex, texNo);
             switch (type) {
             case D3DVSDT_FLOAT1: fvf |= D3DFVF_TEXCOORDSIZE1(texNo); break;
             case D3DVSDT_FLOAT2: fvf |= D3DFVF_TEXCOORDSIZE2(texNo); break;
             case D3DVSDT_FLOAT3: fvf |= D3DFVF_TEXCOORDSIZE3(texNo); break;
             case D3DVSDT_FLOAT4: fvf |= D3DFVF_TEXCOORDSIZE4(texNo); break;
             default: 
               /** errooooorr mismatched use of a register, invalid fvf computing */
               invalid_fvf = TRUE;
               TRACE("Mismatched use in VertexShader declaration of D3DVSDE_TEXCOORD? register: unsupported type %s\n", VertexShaderDeclDataTypes[type]);
             }
         }
         break;

      case D3DVSDE_POSITION2:   /* maybe D3DFVF_XYZRHW instead D3DFVF_XYZ (of D3DVDE_POSITION) ... to see */
      case D3DVSDE_NORMAL2:     /* FIXME i don't know what to do here ;( */
	TRACE("[%lu] registers in VertexShader declaration not supported yet (token:0x%08lx)\n", reg, token);
	break;
      }
      TRACE("VertexShader declaration define %lx as current FVF\n", fvf);
    }
    len += tokenlen;
    pToken += tokenlen;
  }
  /* here D3DVSD_END() */
  len +=  Direct3DVextexShaderDeclarationImpl_ParseToken(pToken);

  /* copy fvf if valid */
  if (FALSE == invalid_fvf) {
      fvf |= tex << D3DFVF_TEXCOUNT_SHIFT;
      object->fvf[stream] = fvf;
      object->allFVF |= fvf;
  } else {
      object->fvf[stream] = 0;
  }
  TRACE("Completed, allFVF = %lx\n", object->allFVF);

  /* compute size */
  object->declaration8Length = len * sizeof(DWORD);
  /* copy the declaration */
  object->pDeclaration8 = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, object->declaration8Length);
  memcpy(object->pDeclaration8, pDeclaration8, object->declaration8Length);
  /* returns */
  *ppVertexShaderDecl = object;
  return D3D_OK;
}


HRESULT WINAPI IDirect3DDeviceImpl_FillVertexShaderInput(IDirect3DDevice8Impl* This,
							 IDirect3DVertexShaderImpl* vshader,	 
							 DWORD SkipnStrides) {
  /** parser data */
  const DWORD* pToken = This->UpdateStateBlock->vertexShaderDecl->pDeclaration8;
  DWORD stream = 0;
  DWORD token;
  /*DWORD tokenlen;*/
  DWORD tokentype;
  /** for input readers */
  const char* curPos = NULL;
  FLOAT x, y, z, w;
  SHORT u, v, r, t;
  DWORD dw;

  TRACE("(%p) - This:%p, skipstrides=%lu\n", vshader, This, SkipnStrides);

  while (D3DVSD_END() != *pToken) {
    token = *pToken;
    tokentype = ((token & D3DVSD_TOKENTYPEMASK) >> D3DVSD_TOKENTYPESHIFT);
    
    /** FVF generation block */
    if (D3DVSD_TOKEN_STREAM == tokentype && 0 == (D3DVSD_STREAMTESSMASK & token)) {
      IDirect3DVertexBuffer8* pVB;
      int skip = 0;

      ++pToken;
      /** 
       * how really works streams, 
       *  in DolphinVS dx8 dsk sample use it !!!
       */
      stream = ((token & D3DVSD_STREAMNUMBERMASK) >> D3DVSD_STREAMNUMBERSHIFT);
      skip = This->StateBlock->stream_stride[stream];
      pVB  = This->StateBlock->stream_source[stream];
      
      if (NULL == pVB) {
	  ERR("using unitialised stream[%lu]\n", stream);
	  return D3DERR_INVALIDCALL;
      } else {
          if (This->StateBlock->streamIsUP == TRUE) {
              curPos = ((char *) pVB) + (SkipnStrides * skip);   /* Not really a VB */
          } else {
              curPos = ((IDirect3DVertexBuffer8Impl*) pVB)->allocatedMemory + (SkipnStrides * skip);
          }
	  
	  TRACE(" using stream[%lu] with %p (%p + (Stride %d * skip %ld))\n", stream, curPos, 
                 ((IDirect3DVertexBuffer8Impl*) pVB)->allocatedMemory, skip, SkipnStrides);
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
  return D3D_OK;
}

HRESULT WINAPI IDirect3DVertexShaderDeclarationImpl_GetDeclaration8(IDirect3DVertexShaderDeclarationImpl* This, DWORD* pData, UINT* pSizeOfData) {
  if (NULL == pData) {
    *pSizeOfData = This->declaration8Length;
    return D3D_OK;
  }
  if (*pSizeOfData < This->declaration8Length) {
    *pSizeOfData = This->declaration8Length;
    return D3DERR_MOREDATA;
  }
  TRACE("(%p) : GetVertexShaderDeclaration copying to %p\n", This, pData);
  memcpy(pData, This->pDeclaration8, This->declaration8Length);
  return D3D_OK;
}
