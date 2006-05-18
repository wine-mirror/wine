/*
 * vertex declaration implementation
 *
 * Copyright 2002-2005 Raphael Junqueira
 * Copyright 2004 Jason Edmeades
 * Copyright 2004 Christian Costa
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
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
  D3DVSDE_NORMAL2      = 16,
  MAX_D3DVSDE          = 17
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

static DWORD IWineD3DVertexDeclarationImpl_ParseToken8(const DWORD* pToken) {
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
      DWORD count        = ((token & D3DVSD_CONSTCOUNTMASK)   >> D3DVSD_CONSTCOUNTSHIFT);
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

/* structure used by the d3d8 to d3d9 conversion lookup table */
typedef struct _Decl8to9Lookup {
    int usage;
    int usageIndex;
} Decl8to9Lookup;

static HRESULT IWineD3DVertexDeclarationImpl_ParseDeclaration8(IWineD3DVertexDeclaration *iface, const DWORD *pDecl) {
IWineD3DVertexDeclarationImpl *This = (IWineD3DVertexDeclarationImpl *)iface;
#define MAKE_LOOKUP(_reg,_usage,_usageindex) decl8to9Lookup[_reg].usage = _usage; \
                                                 decl8to9Lookup[_reg].usageIndex = _usageindex;
    const DWORD* pToken = pDecl;
    DWORD len = 0;
    DWORD stream = 0;
    DWORD token;
    DWORD tokenlen;
    DWORD tokentype;
    DWORD nTokens = 0;
    int offset    = 0;

    WINED3DVERTEXELEMENT convToW[128];
/* TODO: find out where rhw (or positionT) is for declaration8 */
    Decl8to9Lookup decl8to9Lookup[MAX_D3DVSDE];
    MAKE_LOOKUP(D3DVSDE_POSITION,     D3DDECLUSAGE_POSITION, 0);
    MAKE_LOOKUP(D3DVSDE_POSITION2,    D3DDECLUSAGE_POSITION, 1);
    MAKE_LOOKUP(D3DVSDE_BLENDWEIGHT,  D3DDECLUSAGE_BLENDWEIGHT, 0);
    MAKE_LOOKUP(D3DVSDE_BLENDINDICES, D3DDECLUSAGE_BLENDINDICES, 0);
    MAKE_LOOKUP(D3DVSDE_NORMAL,       D3DDECLUSAGE_NORMAL, 0);
    MAKE_LOOKUP(D3DVSDE_NORMAL2,      D3DDECLUSAGE_NORMAL, 1);
    MAKE_LOOKUP(D3DVSDE_DIFFUSE,      D3DDECLUSAGE_COLOR, 0);
    MAKE_LOOKUP(D3DVSDE_SPECULAR,     D3DDECLUSAGE_COLOR, 1);
    MAKE_LOOKUP(D3DVSDE_TEXCOORD0,    D3DDECLUSAGE_TEXCOORD, 0);
    MAKE_LOOKUP(D3DVSDE_TEXCOORD1,    D3DDECLUSAGE_TEXCOORD, 1);
    MAKE_LOOKUP(D3DVSDE_TEXCOORD2,    D3DDECLUSAGE_TEXCOORD, 2);
    MAKE_LOOKUP(D3DVSDE_TEXCOORD3,    D3DDECLUSAGE_TEXCOORD, 3);
    MAKE_LOOKUP(D3DVSDE_TEXCOORD4,    D3DDECLUSAGE_TEXCOORD, 4);
    MAKE_LOOKUP(D3DVSDE_TEXCOORD5,    D3DDECLUSAGE_TEXCOORD, 5);
    MAKE_LOOKUP(D3DVSDE_TEXCOORD6,    D3DDECLUSAGE_TEXCOORD, 6);
    MAKE_LOOKUP(D3DVSDE_TEXCOORD7,    D3DDECLUSAGE_TEXCOORD, 7);

    #undef MAKE_LOOKUP
    TRACE("(%p) :  pDecl(%p)\n", This, pDecl);
    /* Convert from a directx* declaration into a directx9 one, so we only have to deal with one type of declaration everywhere else */
    while (D3DVSD_END() != *pToken) {
        token = *pToken;
        tokenlen = IWineD3DVertexDeclarationImpl_ParseToken8(pToken);
        tokentype = ((token & D3DVSD_TOKENTYPEMASK) >> D3DVSD_TOKENTYPESHIFT);

        if (D3DVSD_TOKEN_STREAM == tokentype && 0 == (D3DVSD_STREAMTESSMASK & token)) {
            /**
            * how really works streams,
            *  in DolphinVS dx8 dsk sample they seems to decal reg numbers !!!
            */
            stream = ((token & D3DVSD_STREAMNUMBERMASK) >> D3DVSD_STREAMNUMBERSHIFT);
            offset = 0;

        } else if (D3DVSD_TOKEN_STREAMDATA == tokentype && 0 == (0x10000000 & tokentype)) {
            DWORD type = ((token & D3DVSD_DATATYPEMASK)  >> D3DVSD_DATATYPESHIFT);
            DWORD reg  = ((token & D3DVSD_VERTEXREGMASK) >> D3DVSD_VERTEXREGSHIFT);

            convToW[nTokens].Stream     = stream;
            convToW[nTokens].Method     = D3DDECLMETHOD_DEFAULT;
            convToW[nTokens].Usage      = decl8to9Lookup[reg].usage;
            convToW[nTokens].UsageIndex = decl8to9Lookup[reg].usageIndex;
            convToW[nTokens].Type       = type;
            convToW[nTokens].Offset     = offset;
            convToW[nTokens].Reg        = reg;
            offset += glTypeLookup[type].size * glTypeLookup[type].typesize;
            ++nTokens;
        } else if (D3DVSD_TOKEN_STREAMDATA == tokentype &&  0x10000000 & tokentype ) {
             TRACE(" 0x%08lx SKIP(%lu)\n", tokentype, ((tokentype & D3DVSD_SKIPCOUNTMASK) >> D3DVSD_SKIPCOUNTSHIFT));
             offset += sizeof(DWORD) * ((tokentype & D3DVSD_SKIPCOUNTMASK) >> D3DVSD_SKIPCOUNTSHIFT);
        } else if (D3DVSD_TOKEN_CONSTMEM  == tokentype) {
            DWORD i;
            DWORD count        = ((token & D3DVSD_CONSTCOUNTMASK)   >> D3DVSD_CONSTCOUNTSHIFT);
            DWORD constaddress = ((token & D3DVSD_CONSTADDRESSMASK) >> D3DVSD_CONSTADDRESSSHIFT);
            if (This->constants == NULL ) {
                This->constants = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_VSHADER_CONSTANTS * 4 * sizeof(float));
            }
            TRACE(" 0x%08lx CONST(%lu, %lu)\n", token, constaddress, count);
            for (i = 0; i < count; ++i) {
                TRACE("        c[%lu] = (0x%08lx, 0x%08lx, 0x%08lx, 0x%08lx)\n",
                    constaddress,
                    *pToken,
                    *(pToken + 1),
                    *(pToken + 2),
                    *(pToken + 3));

                This->constants[constaddress * 4] = *(const float*) (pToken+ i * 4 + 1);
                This->constants[constaddress * 4 + 1] = *(const float *)(pToken + i * 4 + 2);
                This->constants[constaddress * 4 + 2] = *(const float *)(pToken + i * 4 + 3);
                This->constants[constaddress * 4 + 3] = *(const float *)(pToken + i * 4 + 4);
                FIXME("        c[%lu] = (%8f, %8f, %8f, %8f)\n",
                    constaddress,
                    *(const float*) (pToken+ i * 4 + 1),
                    *(const float*) (pToken + i * 4 + 2),
                    *(const float*) (pToken + i * 4 +3),
                    *(const float*) (pToken + i * 4 + 4));
                ++constaddress;
            }
        }

        len += tokenlen;
        pToken += tokenlen;
    }

  /* here D3DVSD_END() */
  len +=  IWineD3DVertexDeclarationImpl_ParseToken8(pToken);

  convToW[nTokens].Stream = 0xFF;
  convToW[nTokens].Type = D3DDECLTYPE_UNUSED;
  ++nTokens;

  /* compute size */
  This->declaration8Length = len * sizeof(DWORD);
  /* copy the declaration */
  This->pDeclaration8 = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, This->declaration8Length);
  memcpy(This->pDeclaration8, pDecl, This->declaration8Length);

  /* compute convToW size */
  This->declarationWNumElements = nTokens;
  /* copy the convTo9 declaration */
  This->pDeclarationWine = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nTokens * sizeof(WINED3DVERTEXELEMENT));
  memcpy(This->pDeclarationWine, convToW, nTokens * sizeof(WINED3DVERTEXELEMENT));

  /* returns */
  return WINED3D_OK;
}

static HRESULT IWineD3DVertexDeclarationImpl_ParseDeclaration9(IWineD3DVertexDeclaration* iface, const D3DVERTEXELEMENT9* pDecl) {
    IWineD3DVertexDeclarationImpl *This = (IWineD3DVertexDeclarationImpl *)iface;
    const D3DVERTEXELEMENT9* pToken = pDecl;
    int i;

    TRACE("(%p) :  pDecl(%p)\n", This, pDecl);

    This->declaration9NumElements = 1;
    for(pToken = pDecl;0xFF != pToken->Stream && This->declaration9NumElements < 128 ; pToken++) This->declaration9NumElements++;

    if (This->declaration9NumElements == 128) {
        FIXME("?(%p) Error parsing vertex declaration\n", This);
        return WINED3DERR_INVALIDCALL;
    }


    /* copy the declaration */
    This->pDeclaration9 = HeapAlloc(GetProcessHeap(), 0, This->declaration9NumElements * sizeof(D3DVERTEXELEMENT9));
    memcpy(This->pDeclaration9, pDecl, This->declaration9NumElements * sizeof(D3DVERTEXELEMENT9));

    /* copy to wine style declaration */
    This->pDeclarationWine = HeapAlloc(GetProcessHeap(), 0, This->declaration9NumElements * sizeof(WINED3DVERTEXELEMENT));
    for(i = 0; i < This->declaration9NumElements; ++i) {
        memcpy(This->pDeclarationWine + i, This->pDeclaration9 + i, sizeof(D3DVERTEXELEMENT9));
        This->pDeclarationWine[i].Reg = -1;
    }

    This->declarationWNumElements = This->declaration9NumElements;

  return WINED3D_OK;
}

/* *******************************************
   IWineD3DVertexDeclaration IUnknown parts follow
   ******************************************* */
HRESULT WINAPI IWineD3DVertexDeclarationImpl_QueryInterface(IWineD3DVertexDeclaration *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DVertexDeclarationImpl *This = (IWineD3DVertexDeclarationImpl *)iface;
    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DBase)
        || IsEqualGUID(riid, &IID_IWineD3DVertexDeclaration)){
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }
    *ppobj = NULL;
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
        HeapFree(GetProcessHeap(), 0, This->pDeclarationWine);
        HeapFree(GetProcessHeap(), 0, This->constants);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* *******************************************
   IWineD3DVertexDeclaration parts follow
   ******************************************* */

HRESULT WINAPI IWineD3DVertexDeclarationImpl_GetParent(IWineD3DVertexDeclaration *iface, IUnknown** parent){
    IWineD3DVertexDeclarationImpl *This = (IWineD3DVertexDeclarationImpl *)iface;

    *parent= This->parent;
    IUnknown_AddRef(*parent);
    TRACE("(%p) : returning %p\n", This, *parent);
    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DVertexDeclarationImpl_GetDevice(IWineD3DVertexDeclaration *iface, IWineD3DDevice** ppDevice) {
    IWineD3DVertexDeclarationImpl *This = (IWineD3DVertexDeclarationImpl *)iface;
    TRACE("(%p) : returning %p\n", This, This->wineD3DDevice);

    *ppDevice = (IWineD3DDevice *) This->wineD3DDevice;
    IWineD3DDevice_AddRef(*ppDevice);

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DVertexDeclarationImpl_GetDeclaration8(IWineD3DVertexDeclaration* iface, DWORD* pData, DWORD* pSizeOfData) {
    IWineD3DVertexDeclarationImpl *This = (IWineD3DVertexDeclarationImpl *)iface;
    if (NULL == pData) {
        *pSizeOfData = This->declaration8Length;
        return WINED3D_OK;
    }

    /* The Incredibles and Teenage Mutant Ninja Turtles require this in d3d9 for NumElements == 0,
    TODO: this needs to be tested against windows */
    if(*pSizeOfData == 0) {
        TRACE("(%p) : Requested the vertex declaration without specifying the size of the return buffer\n", This);
        *pSizeOfData = This->declaration8Length;
        memcpy(pData, This->pDeclaration8, This->declaration8Length);
        return WINED3D_OK;
    }

    if (*pSizeOfData < This->declaration8Length) {
        FIXME("(%p) : Returning WINED3DERR_MOREDATA numElements %ld expected %ld\n", iface, *pSizeOfData, This->declaration8Length);
        *pSizeOfData = This->declaration8Length;
        return WINED3DERR_MOREDATA;
    }
    TRACE("(%p) : GetVertexDeclaration8 copying to %p\n", This, pData);
    memcpy(pData, This->pDeclaration8, This->declaration8Length);
    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DVertexDeclarationImpl_GetDeclaration9(IWineD3DVertexDeclaration* iface,  D3DVERTEXELEMENT9* pData, DWORD* pNumElements) {
    IWineD3DVertexDeclarationImpl *This = (IWineD3DVertexDeclarationImpl *)iface;

    TRACE("(This %p, pData %p, pNumElements %p)\n", This, pData, pNumElements);

    TRACE("Setting *pNumElements to %d\n", This->declaration9NumElements);
    *pNumElements = This->declaration9NumElements;

    /* Passing a NULL pData is used to just retrieve the number of elements */
    if (!pData) {
        TRACE("NULL pData passed. Returning WINED3D_OK.\n");
        return WINED3D_OK;
    }

    TRACE("Copying %p to %p\n", This->pDeclaration9, pData);
    memcpy(pData, This->pDeclaration9, This->declaration9NumElements * sizeof(*pData));
    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DVertexDeclarationImpl_GetDeclaration(IWineD3DVertexDeclaration *iface, VOID *pData, DWORD *pSize) {
    IWineD3DVertexDeclarationImpl *This = (IWineD3DVertexDeclarationImpl *)iface;
    HRESULT hr = WINED3D_OK;

    TRACE("(%p) : d3d version %d r\n", This, ((IWineD3DImpl *)This->wineD3DDevice->wineD3D)->dxVersion);
    switch (((IWineD3DImpl *)This->wineD3DDevice->wineD3D)->dxVersion) {
    case 8:
        hr = IWineD3DVertexDeclarationImpl_GetDeclaration8(iface, (DWORD *)pData, pSize);
    break;
    case 9:
        hr = IWineD3DVertexDeclarationImpl_GetDeclaration9(iface, (D3DVERTEXELEMENT9 *)pData, pSize);
    break;
    default:
        FIXME("(%p)  : Unsupported DirectX version %u\n", This, ((IWineD3DImpl *)This->wineD3DDevice->wineD3D)->dxVersion);
    break;
    }
    return hr;
}

HRESULT WINAPI IWineD3DVertexDeclarationImpl_SetDeclaration(IWineD3DVertexDeclaration *iface, VOID *pDecl) {
    IWineD3DVertexDeclarationImpl *This = (IWineD3DVertexDeclarationImpl *)iface;
    HRESULT hr = WINED3D_OK;

    TRACE("(%p) : d3d version %d\n", This, ((IWineD3DImpl *)This->wineD3DDevice->wineD3D)->dxVersion);
    switch (((IWineD3DImpl *)This->wineD3DDevice->wineD3D)->dxVersion) {
    case 8:
        TRACE("Parsing declaration 8\n");
        hr = IWineD3DVertexDeclarationImpl_ParseDeclaration8(iface, (CONST DWORD *)pDecl);
    break;
    case 9:
        TRACE("Parsing declaration 9\n");
        hr = IWineD3DVertexDeclarationImpl_ParseDeclaration9(iface, (CONST D3DVERTEXELEMENT9 *)pDecl);
    break;
    default:
        FIXME("(%p)  : Unsupported DirectX version %u\n", This, ((IWineD3DImpl *)This->wineD3DDevice->wineD3D)->dxVersion);
    break;
    }
    TRACE("Returning\n");
    return hr;
}

const IWineD3DVertexDeclarationVtbl IWineD3DVertexDeclaration_Vtbl =
{
    /* IUnknown */
    IWineD3DVertexDeclarationImpl_QueryInterface,
    IWineD3DVertexDeclarationImpl_AddRef,
    IWineD3DVertexDeclarationImpl_Release,
    /* IWineD3DVertexDeclaration */
    IWineD3DVertexDeclarationImpl_GetParent,
    IWineD3DVertexDeclarationImpl_GetDevice,
    IWineD3DVertexDeclarationImpl_GetDeclaration,
    IWineD3DVertexDeclarationImpl_SetDeclaration
};
