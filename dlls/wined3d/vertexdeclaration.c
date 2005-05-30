/*
 * vertex declaration implementation
 *
 * Copyright 2002-2005 Raphael Junqueira
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
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_decl);

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

/** Vertex Shader Declaration 8 data types tokens */
#define MAX_VSHADER_DECL_TYPES 8

static CONST char* VertexDecl8_DataTypes[] = {
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

static CONST char* VertexDecl8_Registers[] = {
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

typedef enum _D3DVSD_TOKENTYPE {
  D3DVSD_TOKEN_NOP         = 0,
  D3DVSD_TOKEN_STREAM      = 1,
  D3DVSD_TOKEN_STREAMDATA  = 2,
  D3DVSD_TOKEN_TESSELLATOR = 3,
  D3DVSD_TOKEN_CONSTMEM    = 4,
  D3DVSD_TOKEN_EXT         = 5,
  /* RESERVED              = 6 */
  D3DVSD_TOKEN_END         = 7,
  D3DVSD_FORCE_DWORD       = 0x7FFFFFFF
} D3DVSD_TOKENTYPE;

typedef enum _D3DVSDE_REGISTER {
  D3DVSDE_POSITION     =  0,
  D3DVSDE_BLENDWEIGHT  =  1,
  D3DVSDE_BLENDINDICES =  2,
  D3DVSDE_NORMAL       =  3,
  D3DVSDE_PSIZE        =  4,
  D3DVSDE_DIFFUSE      =  5,
  D3DVSDE_SPECULAR     =  6,
  D3DVSDE_TEXCOORD0    =  7,
  D3DVSDE_TEXCOORD1    =  8,
  D3DVSDE_TEXCOORD2    =  9,
  D3DVSDE_TEXCOORD3    = 10,
  D3DVSDE_TEXCOORD4    = 11,
  D3DVSDE_TEXCOORD5    = 12,
  D3DVSDE_TEXCOORD6    = 13,
  D3DVSDE_TEXCOORD7    = 14,
  D3DVSDE_POSITION2    = 15,
  D3DVSDE_NORMAL2      = 16
} D3DVSDE_REGISTER;

typedef enum _D3DVSDT_TYPE {
  D3DVSDT_FLOAT1   = 0x00,
  D3DVSDT_FLOAT2   = 0x01,
  D3DVSDT_FLOAT3   = 0x02,
  D3DVSDT_FLOAT4   = 0x03,
  D3DVSDT_D3DCOLOR = 0x04,
  D3DVSDT_UBYTE4   = 0x05,
  D3DVSDT_SHORT2   = 0x06,
  D3DVSDT_SHORT4   = 0x07
} D3DVSDT_TYPE;


#define D3DVSD_CONSTADDRESSSHIFT  0
#define D3DVSD_EXTINFOSHIFT       0
#define D3DVSD_STREAMNUMBERSHIFT  0
#define D3DVSD_VERTEXREGSHIFT     0
#define D3DVSD_CONSTRSSHIFT      16
#define D3DVSD_DATATYPESHIFT     16
#define D3DVSD_SKIPCOUNTSHIFT    16
#define D3DVSD_VERTEXREGINSHIFT  20
#define D3DVSD_EXTCOUNTSHIFT     24
#define D3DVSD_CONSTCOUNTSHIFT   25
#define D3DVSD_DATALOADTYPESHIFT 28
#define D3DVSD_STREAMTESSSHIFT   28
#define D3DVSD_TOKENTYPESHIFT    29

#define D3DVSD_CONSTADDRESSMASK  (0x7F     << D3DVSD_CONSTADDRESSSHIFT)
#define D3DVSD_EXTINFOMASK       (0xFFFFFF << D3DVSD_EXTINFOSHIFT)
#define D3DVSD_STREAMNUMBERMASK  (0xF      << D3DVSD_STREAMNUMBERSHIFT)
#define D3DVSD_VERTEXREGMASK     (0x1F     << D3DVSD_VERTEXREGSHIFT)
#define D3DVSD_CONSTRSMASK       (0x1FFF   << D3DVSD_CONSTRSSHIFT)
#define D3DVSD_DATATYPEMASK      (0xF      << D3DVSD_DATATYPESHIFT)
#define D3DVSD_SKIPCOUNTMASK     (0xF      << D3DVSD_SKIPCOUNTSHIFT)
#define D3DVSD_EXTCOUNTMASK      (0x1F     << D3DVSD_EXTCOUNTSHIFT)
#define D3DVSD_VERTEXREGINMASK   (0xF      << D3DVSD_VERTEXREGINSHIFT)
#define D3DVSD_CONSTCOUNTMASK    (0xF      << D3DVSD_CONSTCOUNTSHIFT)
#define D3DVSD_DATALOADTYPEMASK  (0x1      << D3DVSD_DATALOADTYPESHIFT)
#define D3DVSD_STREAMTESSMASK    (0x1      << D3DVSD_STREAMTESSSHIFT)
#define D3DVSD_TOKENTYPEMASK     (0x7      << D3DVSD_TOKENTYPESHIFT)

#define D3DVSD_END() 0xFFFFFFFF
#define D3DVSD_NOP() 0x00000000

DWORD IWineD3DVertexDeclarationImpl_ParseToken8(const DWORD* pToken) {
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
      TRACE(" 0x%08lx REG(%s, %s)\n", token, VertexDecl8_Registers[reg], VertexDecl8_DataTypes[type]);
    }
    break;
  case D3DVSD_TOKEN_TESSELLATOR:
    if (token & 0x10000000) {
      DWORD type = ((token & D3DVSD_DATATYPEMASK)  >> D3DVSD_DATATYPESHIFT);
      DWORD reg  = ((token & D3DVSD_VERTEXREGMASK) >> D3DVSD_VERTEXREGSHIFT);
      TRACE(" 0x%08lx TESSUV(%s) as %s\n", token, VertexDecl8_Registers[reg], VertexDecl8_DataTypes[type]);
    } else {
      DWORD type   = ((token & D3DVSD_DATATYPEMASK)    >> D3DVSD_DATATYPESHIFT);
      DWORD regout = ((token & D3DVSD_VERTEXREGMASK)   >> D3DVSD_VERTEXREGSHIFT);
      DWORD regin  = ((token & D3DVSD_VERTEXREGINMASK) >> D3DVSD_VERTEXREGINSHIFT);
      TRACE(" 0x%08lx TESSNORMAL(%s, %s) as %s\n", token, VertexDecl8_Registers[regin], VertexDecl8_Registers[regout], VertexDecl8_DataTypes[type]);
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
		 *(const float*) pToken, 
		 *(const float*) (pToken + 1), 
		 *(const float*) (pToken + 2), 
		 *(const float*) (pToken + 3));
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

DWORD IWineD3DVertexDeclarationImpl_ParseToken9(const D3DVERTEXELEMENT9* pToken);

HRESULT IWineD3DVertexDeclarationImpl_ParseDeclaration8(IWineD3DDeviceImpl* This, const DWORD* pDecl, IWineD3DVertexDeclarationImpl* object) { 
  const DWORD* pToken = pDecl;
  DWORD fvf = 0;
  BOOL  invalid_fvf = FALSE;
  DWORD tex = D3DFVF_TEX0;
  DWORD len = 0;  
  DWORD stream = 0;
  DWORD token;
  DWORD tokenlen;
  DWORD tokentype;
  DWORD nTokens = 0;
  D3DVERTEXELEMENT9 convTo9[128];

  TRACE("(%p) :  pDecl(%p)\n", This, pDecl);

  while (D3DVSD_END() != *pToken) {
    token = *pToken;
    tokenlen = IWineD3DVertexDeclarationImpl_ParseToken8(pToken); 
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

      convTo9[nTokens].Stream = stream;
      convTo9[nTokens].Method = D3DDECLMETHOD_DEFAULT;
      convTo9[nTokens].UsageIndex = 0;
      convTo9[nTokens].Type = D3DDECLTYPE_UNUSED;

      switch (reg) {
      case D3DVSDE_POSITION:     
	convTo9[nTokens].Usage = D3DDECLUSAGE_POSITION;
	convTo9[nTokens].Type = type;
	switch (type) {
	case D3DVSDT_FLOAT3:     fvf |= D3DFVF_XYZ;             break;
	case D3DVSDT_FLOAT4:     fvf |= D3DFVF_XYZRHW;          break;
	default: 
	  /** Mismatched use of a register, invalid for fixed function fvf computing (ok for VS) */
	  invalid_fvf = TRUE;
	  if (type >= MAX_VSHADER_DECL_TYPES) {
	    TRACE("Mismatched use in VertexShader declaration of D3DVSDE_POSITION register: unsupported and unrecognized type %08lx\n", type);
	  } else {
	    TRACE("Mismatched use in VertexShader declaration of D3DVSDE_POSITION register: unsupported type %s\n", VertexDecl8_DataTypes[type]);
	  }
	}
	break;
      
      case D3DVSDE_BLENDWEIGHT:
	convTo9[nTokens].Usage = D3DDECLUSAGE_BLENDWEIGHT;
	convTo9[nTokens].Type = type;
	switch (type) {
	case D3DVSDT_FLOAT1:     fvf |= D3DFVF_XYZB1;           break;
	case D3DVSDT_FLOAT2:     fvf |= D3DFVF_XYZB2;           break;
	case D3DVSDT_FLOAT3:     fvf |= D3DFVF_XYZB3;           break;
	case D3DVSDT_FLOAT4:     fvf |= D3DFVF_XYZB4;           break;
	default: 
	  /** Mismatched use of a register, invalid for fixed function fvf computing (ok for VS) */
	  invalid_fvf = TRUE;
	  TRACE("Mismatched use in VertexShader declaration of D3DVSDE_BLENDWEIGHT register: unsupported type %s\n", VertexDecl8_DataTypes[type]);
	}
	break;

      case D3DVSDE_BLENDINDICES: /* seem to be B5 as said in MSDN Dx9SDK ??  */
	convTo9[nTokens].Usage = D3DDECLUSAGE_BLENDINDICES;
	convTo9[nTokens].Type = type;
	switch (type) {
	case D3DVSDT_UBYTE4:     fvf |= D3DFVF_LASTBETA_UBYTE4;           break;
	default: 
	  /** Mismatched use of a register, invalid for fixed function fvf computing (ok for VS) */
	  invalid_fvf = TRUE;
	  TRACE("Mismatched use in VertexShader declaration of D3DVSDE_BLENDINDINCES register: unsupported type %s\n", VertexDecl8_DataTypes[type]);
	}
	break; 

      case D3DVSDE_NORMAL: /* TODO: only FLOAT3 supported ... another choice possible ? */
	convTo9[nTokens].Usage = D3DDECLUSAGE_NORMAL;
	convTo9[nTokens].Type = type;
	switch (type) {
	case D3DVSDT_FLOAT3:     fvf |= D3DFVF_NORMAL;          break;
	default: 
	  /** Mismatched use of a register, invalid for fixed function fvf computing (ok for VS) */
	  invalid_fvf = TRUE;
	  TRACE("Mismatched use in VertexShader declaration of D3DVSDE_NORMAL register: unsupported type %s\n", VertexDecl8_DataTypes[type]);
	}
	break; 

      case D3DVSDE_PSIZE:  /* TODO: only FLOAT1 supported ... another choice possible ? */
	convTo9[nTokens].Usage = D3DDECLUSAGE_PSIZE;
	convTo9[nTokens].Type = type;
	switch (type) {
        case D3DVSDT_FLOAT1:     fvf |= D3DFVF_PSIZE;           break;
	default: 
	  /** Mismatched use of a register, invalid for fixed function fvf computing (ok for VS) */
	  invalid_fvf = TRUE;
	  TRACE("Mismatched use in VertexShader declaration of D3DVSDE_PSIZE register: unsupported type %s\n", VertexDecl8_DataTypes[type]);
	}
	break;

      case D3DVSDE_DIFFUSE:  /* TODO: only D3DCOLOR supported */
	convTo9[nTokens].Usage = D3DDECLUSAGE_COLOR;
	convTo9[nTokens].UsageIndex = 0;
	convTo9[nTokens].Type = type;
	switch (type) {
	case D3DVSDT_D3DCOLOR:   fvf |= D3DFVF_DIFFUSE;         break;
	default: 
	  /** Mismatched use of a register, invalid for fixed function fvf computing (ok for VS) */
	  invalid_fvf = TRUE;
	  TRACE("Mismatched use in VertexShader declaration of D3DVSDE_DIFFUSE register: unsupported type %s\n", VertexDecl8_DataTypes[type]);
	}
	break;

      case D3DVSDE_SPECULAR:  /* TODO: only D3DCOLOR supported */
	convTo9[nTokens].Usage = D3DDECLUSAGE_COLOR;
	convTo9[nTokens].UsageIndex = 1;
	convTo9[nTokens].Type = type;
	switch (type) {
	case D3DVSDT_D3DCOLOR:	 fvf |= D3DFVF_SPECULAR;        break;
	default: 
	  /** Mismatched use of a register, invalid for fixed function fvf computing (ok for VS) */
	  invalid_fvf = TRUE;
	  TRACE("Mismatched use in VertexShader declaration of D3DVSDE_SPECULAR register: unsupported type %s\n", VertexDecl8_DataTypes[type]);
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
	     convTo9[nTokens].Usage = D3DDECLUSAGE_TEXCOORD;
	     convTo9[nTokens].UsageIndex = texNo;
	     convTo9[nTokens].Type = type;
             tex = max(tex, texNo);
             switch (type) {
             case D3DVSDT_FLOAT1: fvf |= D3DFVF_TEXCOORDSIZE1(texNo); break;
             case D3DVSDT_FLOAT2: fvf |= D3DFVF_TEXCOORDSIZE2(texNo); break;
             case D3DVSDT_FLOAT3: fvf |= D3DFVF_TEXCOORDSIZE3(texNo); break;
             case D3DVSDT_FLOAT4: fvf |= D3DFVF_TEXCOORDSIZE4(texNo); break;
             default: 
               /** Mismatched use of a register, invalid for fixed function fvf computing (ok for VS) */
               invalid_fvf = TRUE;
               TRACE("Mismatched use in VertexShader declaration of D3DVSDE_TEXCOORD? register: unsupported type %s\n", VertexDecl8_DataTypes[type]);
             }
         }
         break;

      case D3DVSDE_POSITION2:   /* maybe D3DFVF_XYZRHW instead D3DFVF_XYZ (of D3DVDE_POSITION) ... to see */
      case D3DVSDE_NORMAL2:     /* FIXME i don't know what to do here ;( */	
	FIXME("[%lu] registers in VertexShader declaration not supported yet (token:0x%08lx)\n", reg, token);
	break;
      }
      TRACE("VertexShader declaration define %lx as current FVF\n", fvf);
    }
    ++nTokens;
    len += tokenlen;
    pToken += tokenlen;
  }
  /* here D3DVSD_END() */
  len +=  IWineD3DVertexDeclarationImpl_ParseToken8(pToken);
  
  convTo9[nTokens].Stream = 0xFF;
  convTo9[nTokens].Type = D3DDECLTYPE_UNUSED;
  
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
  memcpy(object->pDeclaration8, pDecl, object->declaration8Length);

  /* compute convTo9 size */
  object->declaration9NumElements = nTokens;
  /* copy the convTo9 declaration */
  object->pDeclaration9 = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nTokens * sizeof(D3DVERTEXELEMENT9));
  memcpy(object->pDeclaration9, convTo9, nTokens * sizeof(D3DVERTEXELEMENT9));

  {
    D3DVERTEXELEMENT9* pIt = object->pDeclaration9;
    TRACE("dumping of D3D9 Conversion:\n");
    while (0xFF != pIt->Stream) {
      IWineD3DVertexDeclarationImpl_ParseToken9(pIt);
      ++pIt;
    }  
    IWineD3DVertexDeclarationImpl_ParseToken9(pIt);
  }

  /* returns */
  return D3D_OK;
}

static CONST char* VertexDecl9_DeclUsages[] = {
  "D3DDECLUSAGE_POSITION",
  "D3DDECLUSAGE_BLENDWEIGHT",
  "D3DDECLUSAGE_BLENDINDICES",
  "D3DDECLUSAGE_NORMAL",
  "D3DDECLUSAGE_PSIZE",
  "D3DDECLUSAGE_TEXCOORD",
  "D3DDECLUSAGE_TANGENT",
  "D3DDECLUSAGE_BINORMAL",
  "D3DDECLUSAGE_TESSFACTOR",
  "D3DDECLUSAGE_POSITIONT",
  "D3DDECLUSAGE_COLOR",
  "D3DDECLUSAGE_FOG",
  "D3DDECLUSAGE_DEPTH",
  "D3DDECLUSAGE_SAMPLE",
  NULL
};

static CONST char* VertexDecl9_DeclMethods[] = {
  "D3DDECLMETHOD_DEFAULT",
  "D3DDECLMETHOD_PARTIALU",
  "D3DDECLMETHOD_PARTIALV",
  "D3DDECLMETHOD_CROSSUV",
  "D3DDECLMETHOD_UV",
  "D3DDECLMETHOD_LOOKUP",
  "D3DDECLMETHOD_LOOKUPPRESAMPLED",
  NULL
};

static CONST char* VertexDecl9_DeclTypes[] = {
  "D3DDECLTYPE_FLOAT1",
  "D3DDECLTYPE_FLOAT2",
  "D3DDECLTYPE_FLOAT3",
  "D3DDECLTYPE_FLOAT4",
  "D3DDECLTYPE_D3DCOLOR",
  "D3DDECLTYPE_UBYTE4",
  "D3DDECLTYPE_SHORT2",
  "D3DDECLTYPE_SHORT4",
  /* VS 2.0 */
  "D3DDECLTYPE_UBYTE4N",
  "D3DDECLTYPE_SHORT2N",
  "D3DDECLTYPE_SHORT4N",
  "D3DDECLTYPE_USHORT2N",
  "D3DDECLTYPE_USHORT4N",
  "D3DDECLTYPE_UDEC3",
  "D3DDECLTYPE_DEC3N",
  "D3DDECLTYPE_FLOAT16_2",
  "D3DDECLTYPE_FLOAT16_4",
  "D3DDECLTYPE_UNUSED",
  NULL
};

DWORD IWineD3DVertexDeclarationImpl_ParseToken9(const D3DVERTEXELEMENT9* pToken) {
  DWORD tokenlen = 1;

  if (0xFF != pToken->Stream) {
    TRACE(" D3DDECL(%u, %u, %s, %s, %s, %u)\n",
	  pToken->Stream,
	  pToken->Offset,
	  VertexDecl9_DeclTypes[pToken->Type],
	  VertexDecl9_DeclMethods[pToken->Method],
	  VertexDecl9_DeclUsages[pToken->Usage],
	  pToken->UsageIndex
	  );
  } else {
    TRACE(" D3DDECL_END()\n" );
  }
  return tokenlen;
}

HRESULT IWineD3DVertexDeclarationImpl_ParseDeclaration9(IWineD3DDeviceImpl* This, const D3DVERTEXELEMENT9* pDecl, IWineD3DVertexDeclarationImpl* object) { 
  const D3DVERTEXELEMENT9* pToken = pDecl;
  DWORD fvf = 0;
  BOOL  invalid_fvf = FALSE;
  DWORD tex = D3DFVF_TEX0;
  DWORD len = 0;  
  DWORD stream = 0;

  TRACE("(%p) :  pDecl(%p)\n", This, pDecl);

  while (0xFF != pToken->Stream) {
    DWORD type = pToken->Type;
    DWORD oldStream = stream;
    stream = pToken->Stream;

    IWineD3DVertexDeclarationImpl_ParseToken9(pToken);

    if (D3DDECLMETHOD_DEFAULT != pToken->Method) {
      WARN(
	    "%s register: Unsupported Method of %s (only D3DDECLMETHOD_DEFAULT for now) for VertexDeclaration (type %s)\n", 
	    VertexDecl9_DeclUsages[pToken->Usage], 
	    VertexDecl9_DeclMethods[pToken->Method],
	    VertexDecl9_DeclTypes[pToken->Type]
	    );
    }

    if (oldStream != stream) {
      
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
    }

    switch (pToken->Usage) { 
    case D3DDECLUSAGE_POSITION:
      if (0 < pToken->UsageIndex) {
	invalid_fvf = TRUE;
	TRACE("Mismatched UsageIndex (%u) in VertexDeclaration for D3DDECLUSAGE_POSITION register: unsupported type %s\n", 
	      pToken->UsageIndex, VertexDecl9_DeclTypes[type]);
	break;
      }
      switch (type) {
      case D3DDECLTYPE_FLOAT3: fvf |= D3DFVF_XYZ; break;
      case D3DDECLTYPE_FLOAT4: fvf |= D3DFVF_XYZRHW; break;
      default: 
	/** Mismatched use of a register, invalid for fixed function fvf computing (ok for VS) */
	invalid_fvf = TRUE;
	TRACE("Mismatched use in VertexDeclaration of D3DDECLUSAGE_POSITION register: unsupported type %s\n", VertexDecl9_DeclTypes[type]);
      }
      break;

    case D3DDECLUSAGE_BLENDWEIGHT:
      switch (type) {
      case D3DDECLTYPE_FLOAT1: fvf |= D3DFVF_XYZB1; break;
      case D3DDECLTYPE_FLOAT2: fvf |= D3DFVF_XYZB2; break;
      case D3DDECLTYPE_FLOAT3: fvf |= D3DFVF_XYZB3; break;
      case D3DDECLTYPE_FLOAT4: fvf |= D3DFVF_XYZB4; break;
      default: 
	/** Mismatched use of a register, invalid for fixed function fvf computing (ok for VS) */
	invalid_fvf = TRUE;
	TRACE("Mismatched use in VertexDeclaration of D3DDECLUSAGE_BLENDWEIGHT register: unsupported type %s\n", VertexDecl9_DeclTypes[type]);
      }
      break;
    
    case D3DDECLUSAGE_BLENDINDICES:
      switch (type) {
      case D3DDECLTYPE_UBYTE4: fvf |= D3DFVF_LASTBETA_UBYTE4; break;
      default: 
	/** Mismatched use of a register, invalid for fixed function fvf computing (ok for VS) */
	invalid_fvf = TRUE;
	TRACE("Mismatched use in VertexDeclaration of D3DDECLUSAGE_BLENDINDINCES register: unsupported type %s\n", VertexDecl9_DeclTypes[type]);
      }
      break; 

    case D3DDECLUSAGE_NORMAL:
      if (0 < pToken->UsageIndex) {
	invalid_fvf = TRUE;
	TRACE("Mismatched UsageIndex (%u) in VertexDeclaration for D3DDECLUSAGE_NORMAL register: unsupported type %s\n", 
	      pToken->UsageIndex, VertexDecl9_DeclTypes[type]);
	break;
      }
      switch (type) {
      case D3DDECLTYPE_FLOAT3: fvf |= D3DFVF_NORMAL; break;
      default: 
	/** Mismatched use of a register, invalid for fixed function fvf computing (ok for VS) */
	invalid_fvf = TRUE;
	TRACE("Mismatched use in VertexDeclaration of D3DDECLUSAGE_NORMAL register: unsupported type %s\n", VertexDecl9_DeclTypes[type]);
      }
      break;

    case D3DDECLUSAGE_PSIZE:
      switch (type) {
      case D3DDECLTYPE_FLOAT1: fvf |= D3DFVF_PSIZE; break;
      default: 
	/** Mismatched use of a register, invalid for fixed function fvf computing (ok for VS) */
	invalid_fvf = TRUE;
	TRACE("Mismatched use in VertexDeclaration of D3DDECLUSAGE_PSIZE register: unsupported type %s\n", VertexDecl9_DeclTypes[type]);
      }
      break;

    case D3DDECLUSAGE_TEXCOORD:
      {
	DWORD texNo = pToken->UsageIndex;
	tex = max(tex, texNo);
	switch (type) {
	case D3DDECLTYPE_FLOAT1: fvf |= D3DFVF_TEXCOORDSIZE1(texNo); break;
	case D3DDECLTYPE_FLOAT2: fvf |= D3DFVF_TEXCOORDSIZE2(texNo); break;
	case D3DDECLTYPE_FLOAT3: fvf |= D3DFVF_TEXCOORDSIZE3(texNo); break;
	case D3DDECLTYPE_FLOAT4: fvf |= D3DFVF_TEXCOORDSIZE4(texNo); break;
	default: 
	  /** Mismatched use of a register, invalid for fixed function fvf computing (ok for VS) */
	  invalid_fvf = TRUE;
	  TRACE("Mismatched use in VertexDeclaration of D3DDECLUSAGE_TEXCOORD[%lu] register: unsupported type %s\n", texNo, VertexDecl9_DeclTypes[type]);
	}
      }
      break;

    case D3DDECLUSAGE_COLOR:
      {
	DWORD colorNo = pToken->UsageIndex;
	switch (type) {
	case D3DDECLTYPE_D3DCOLOR:
	  switch (pToken->UsageIndex) {
	  case 0: fvf |= D3DFVF_DIFFUSE; break;
	  case 1: fvf |= D3DFVF_SPECULAR; break;
	  default: 
	    /** Mismatched use of a register, invalid for fixed function fvf computing (ok for VS) */
	    invalid_fvf = TRUE;
	    TRACE("Mismatched use in VertexDeclaration of D3DDECLUSAGE_COLOR[%lu] unsupported COLOR register\n", colorNo);
	  }
	  break;
	default: 
	  /** Mismatched use of a register, invalid for fixed function fvf computing (ok for VS) */
	  invalid_fvf = TRUE;
	  TRACE("Mismatched use in VertexDeclaration of D3DDECLUSAGE_COLOR[%lu] register: unsupported type %s\n", colorNo, VertexDecl9_DeclTypes[type]);
	}
      }
      break;

    case D3DDECLUSAGE_TANGENT:
    case D3DDECLUSAGE_BINORMAL:
    case D3DDECLUSAGE_TESSFACTOR:
    case D3DDECLUSAGE_POSITIONT:
    case D3DDECLUSAGE_FOG:
    case D3DDECLUSAGE_DEPTH:
    case D3DDECLUSAGE_SAMPLE:
      FIXME("%s Usage not supported yet by VertexDeclaration (type is %s)\n", VertexDecl9_DeclUsages[pToken->Usage], VertexDecl9_DeclTypes[type]);
      break;
    }

    ++len;
    ++pToken;
  }
  ++len; /* D3DDECL_END() */

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
  object->declaration9NumElements = len;
  /* copy the declaration */
  object->pDeclaration9 = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len * sizeof(D3DVERTEXELEMENT9));
  memcpy(object->pDeclaration9, pDecl, len * sizeof(D3DVERTEXELEMENT9));
  /* returns */
  
  TRACE("Returns allFVF = %lx\n", object->allFVF);

  return D3D_OK;
}

/* *******************************************
   IWineD3DVertexDeclaration IUnknown parts follow
   ******************************************* */
HRESULT WINAPI IWineD3DVertexDeclarationImpl_QueryInterface(IWineD3DVertexDeclaration *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DVertexDeclarationImpl *This = (IWineD3DVertexDeclarationImpl *)iface;
    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DVertexDeclaration)){
        IUnknown_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }
    return E_NOINTERFACE;
}

ULONG WINAPI IWineD3DVertexDeclarationImpl_AddRef(IWineD3DVertexDeclaration *iface) {
    IWineD3DVertexDeclarationImpl *This = (IWineD3DVertexDeclarationImpl *)iface;
    TRACE("(%p) : AddRef increasing from %ld\n", This, This->ref);
    return InterlockedIncrement(&This->ref);
}

ULONG WINAPI IWineD3DVertexDeclarationImpl_Release(IWineD3DVertexDeclaration *iface) {
    IWineD3DVertexDeclarationImpl *This = (IWineD3DVertexDeclarationImpl *)iface;
    ULONG ref;
    TRACE("(%p) : Releasing from %ld\n", This, This->ref);
    ref = InterlockedDecrement(&This->ref);
    if (ref == 0) {
      HeapFree(GetProcessHeap(), 0, This->pDeclaration8);
      HeapFree(GetProcessHeap(), 0, This->pDeclaration9);
      HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* *******************************************
   IWineD3DVertexDeclaration parts follow
   ******************************************* */

HRESULT WINAPI IWineD3DVertexDeclarationImpl_GetDevice(IWineD3DVertexDeclaration *iface, IWineD3DDevice** ppDevice) {
    IWineD3DVertexDeclarationImpl *This = (IWineD3DVertexDeclarationImpl *)iface;
    TRACE("(%p) : returning %p\n", This, This->wineD3DDevice);
    
    *ppDevice = (IWineD3DDevice *) This->wineD3DDevice;
    IWineD3DDevice_AddRef(*ppDevice);

    return D3D_OK;
}

static HRESULT WINAPI IWineD3DVertexDeclarationImpl_GetDeclaration8(IWineD3DVertexDeclaration* iface, DWORD* pData, DWORD* pSizeOfData) {
  IWineD3DVertexDeclarationImpl *This = (IWineD3DVertexDeclarationImpl *)iface;
  if (NULL == pData) {
    *pSizeOfData = This->declaration8Length;
    return D3D_OK;
  }
  if (*pSizeOfData < This->declaration8Length) {
    *pSizeOfData = This->declaration8Length;
    return D3DERR_MOREDATA;
  }
  TRACE("(%p) : GetVertexDeclaration8 copying to %p\n", This, pData);
  memcpy(pData, This->pDeclaration8, This->declaration8Length);
  return D3D_OK;
}

HRESULT WINAPI IWineD3DVertexDeclarationImpl_GetDeclaration9(IWineD3DVertexDeclaration* iface,  D3DVERTEXELEMENT9* pData, DWORD* pNumElements) {
  IWineD3DVertexDeclarationImpl *This = (IWineD3DVertexDeclarationImpl *)iface;
  if (NULL == pData) {
    *pNumElements = This->declaration9NumElements;
    return D3D_OK;
  }
  if (*pNumElements < This->declaration9NumElements) {
    *pNumElements = This->declaration9NumElements;
    return D3DERR_MOREDATA;
  }
  TRACE("(%p) : GetVertexDeclaration9 copying to %p\n", This, pData);
  memcpy(pData, This->pDeclaration9, This->declaration9NumElements);
  return D3D_OK;
}

HRESULT WINAPI IWineD3DVertexDeclarationImpl_GetDeclaration(IWineD3DVertexDeclaration* iface, UINT iDeclVersion, VOID* pData, DWORD* pSize) {
  if (8 == iDeclVersion) {
    return IWineD3DVertexDeclarationImpl_GetDeclaration8(iface, (DWORD*) pData, pSize);
  } 
  return IWineD3DVertexDeclarationImpl_GetDeclaration9(iface, (D3DVERTEXELEMENT9*) pData, pSize);
}

IWineD3DVertexDeclarationVtbl IWineD3DVertexDeclaration_Vtbl =
{
    IWineD3DVertexDeclarationImpl_QueryInterface,
    IWineD3DVertexDeclarationImpl_AddRef,
    IWineD3DVertexDeclarationImpl_Release,
    IWineD3DVertexDeclarationImpl_GetDevice,
    IWineD3DVertexDeclarationImpl_GetDeclaration,
};
