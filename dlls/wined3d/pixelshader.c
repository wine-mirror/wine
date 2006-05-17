/*
 * shaders implementation
 *
 * Copyright 2002-2003 Jason Edmeades
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2005 Oliver Stieber
 * Copyright 2006 Ivan Gyurdiev
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

#if 0 /* Must not be 1 in cvs version */
# define PSTRACE(A) TRACE A
# define TRACE_VSVECTOR(name) TRACE( #name "=(%f, %f, %f, %f)\n", name.x, name.y, name.z, name.w)
#else
# define PSTRACE(A)
# define TRACE_VSVECTOR(name)
#endif

#define GLNAME_REQUIRE_GLSL  ((const char *)1)
/* *******************************************
   IWineD3DPixelShader IUnknown parts follow
   ******************************************* */
HRESULT WINAPI IWineD3DPixelShaderImpl_QueryInterface(IWineD3DPixelShader *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *)iface;
    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DBase)
        || IsEqualGUID(riid, &IID_IWineD3DBaseShader)
        || IsEqualGUID(riid, &IID_IWineD3DPixelShader)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }
    *ppobj = NULL;
    return E_NOINTERFACE;
}

ULONG WINAPI IWineD3DPixelShaderImpl_AddRef(IWineD3DPixelShader *iface) {
    IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *)iface;
    TRACE("(%p) : AddRef increasing from %ld\n", This, This->ref);
    return InterlockedIncrement(&This->ref);
}

ULONG WINAPI IWineD3DPixelShaderImpl_Release(IWineD3DPixelShader *iface) {
    IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *)iface;
    ULONG ref;
    TRACE("(%p) : Releasing from %ld\n", This, This->ref);
    ref = InterlockedDecrement(&This->ref);
    if (ref == 0) {
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* TODO: At the momeny the function parser is single pass, it achievs this 
   by passing constants to a couple of functions where they are then modified.
   At some point the parser need to be made two pass (So that GLSL can be used if it's required by the shader)
   when happens constants should be worked out in the first pass to tidy up the second pass a bit.
*/

/* *******************************************
   IWineD3DPixelShader IWineD3DPixelShader parts follow
   ******************************************* */

HRESULT WINAPI IWineD3DPixelShaderImpl_GetParent(IWineD3DPixelShader *iface, IUnknown** parent){
    IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *)iface;

    *parent = This->parent;
    IUnknown_AddRef(*parent);
    TRACE("(%p) : returning %p\n", This, *parent);
    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DPixelShaderImpl_GetDevice(IWineD3DPixelShader* iface, IWineD3DDevice **pDevice){
    IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *)iface;
    IWineD3DDevice_AddRef((IWineD3DDevice *)This->wineD3DDevice);
    *pDevice = (IWineD3DDevice *)This->wineD3DDevice;
    TRACE("(%p) returning %p\n", This, *pDevice);
    return WINED3D_OK;
}


HRESULT WINAPI IWineD3DPixelShaderImpl_GetFunction(IWineD3DPixelShader* impl, VOID* pData, UINT* pSizeOfData) {
  IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *)impl;
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
    if (This->baseShader.functionLength == 0) {

    }
    TRACE("(%p) : GetFunction copying to %p\n", This, pData);
    memcpy(pData, This->baseShader.function, This->baseShader.functionLength);
  }
  return WINED3D_OK;
}

/*******************************
 * pshader functions software VM
 */

void pshader_add(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
    d->x = s0->x + s1->x;
    d->y = s0->y + s1->y;
    d->z = s0->z + s1->z;
    d->w = s0->w + s1->w;
    PSTRACE(("executing add: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
                s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

void pshader_dp3(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
    d->x = d->y = d->z = d->w = s0->x * s1->x + s0->y * s1->y + s0->z * s1->z;
    PSTRACE(("executing dp3: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
                s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

void pshader_dp4(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
    d->x = d->y = d->z = d->w = s0->x * s1->x + s0->y * s1->y + s0->z * s1->z + s0->w * s1->w;
    PSTRACE(("executing dp4: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
        s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

void pshader_dst(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
    d->x = 1.0f;
    d->y = s0->y * s1->y;
    d->z = s0->z;
    d->w = s1->w;
    PSTRACE(("executing dst: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
        s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

void pshader_expp(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
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
    PSTRACE(("executing exp: s0=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
                    s0->x, s0->y, s0->z, s0->w, d->x, d->y, d->z, d->w));
}

void pshader_logp(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    float tmp_f = fabsf(s0->w);
    d->x = d->y = d->z = d->w = (0.0f != tmp_f) ? logf(tmp_f) / logf(2.0f) : -HUGE_VAL;
    PSTRACE(("executing logp: s0=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
                s0->x, s0->y, s0->z, s0->w, d->x, d->y, d->z, d->w));
}

void pshader_mad(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1, WINED3DSHADERVECTOR* s2) {
    d->x = s0->x * s1->x + s2->x;
    d->y = s0->y * s1->y + s2->y;
    d->z = s0->z * s1->z + s2->z;
    d->w = s0->w * s1->w + s2->w;
    PSTRACE(("executing mad: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) s2=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
        s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, s2->x, s2->y, s2->z, s2->w, d->x, d->y, d->z, d->w));
}

void pshader_max(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
    d->x = (s0->x >= s1->x) ? s0->x : s1->x;
    d->y = (s0->y >= s1->y) ? s0->y : s1->y;
    d->z = (s0->z >= s1->z) ? s0->z : s1->z;
    d->w = (s0->w >= s1->w) ? s0->w : s1->w;
    PSTRACE(("executing max: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
        s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

void pshader_min(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
    d->x = (s0->x < s1->x) ? s0->x : s1->x;
    d->y = (s0->y < s1->y) ? s0->y : s1->y;
    d->z = (s0->z < s1->z) ? s0->z : s1->z;
    d->w = (s0->w < s1->w) ? s0->w : s1->w;
    PSTRACE(("executing min: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
        s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

void pshader_mov(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    d->x = s0->x;
    d->y = s0->y;
    d->z = s0->z;
    d->w = s0->w;
    PSTRACE(("executing mov: s0=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
        s0->x, s0->y, s0->z, s0->w, d->x, d->y, d->z, d->w));
}

void pshader_mul(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
    d->x = s0->x * s1->x;
    d->y = s0->y * s1->y;
    d->z = s0->z * s1->z;
    d->w = s0->w * s1->w;
    PSTRACE(("executing mul: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
        s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

void pshader_nop(void) {
    /* NOPPPP ahhh too easy ;) */
    PSTRACE(("executing nop\n"));
}

void pshader_rcp(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    d->x = d->y = d->z = d->w = (0.0f == s0->w) ? HUGE_VAL : 1.0f / s0->w;
    PSTRACE(("executing rcp: s0=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
        s0->x, s0->y, s0->z, s0->w, d->x, d->y, d->z, d->w));
}

void pshader_rsq(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    float tmp_f = fabsf(s0->w);
    d->x = d->y = d->z = d->w = (0.0f == tmp_f) ? HUGE_VAL : ((1.0f != tmp_f) ? 1.0f / sqrtf(tmp_f) : 1.0f);
    PSTRACE(("executing rsq: s0=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
        s0->x, s0->y, s0->z, s0->w, d->x, d->y, d->z, d->w));
}

void pshader_sge(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
    d->x = (s0->x >= s1->x) ? 1.0f : 0.0f;
    d->y = (s0->y >= s1->y) ? 1.0f : 0.0f;
    d->z = (s0->z >= s1->z) ? 1.0f : 0.0f;
    d->w = (s0->w >= s1->w) ? 1.0f : 0.0f;
    PSTRACE(("executing sge: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
        s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

void pshader_slt(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
    d->x = (s0->x < s1->x) ? 1.0f : 0.0f;
    d->y = (s0->y < s1->y) ? 1.0f : 0.0f;
    d->z = (s0->z < s1->z) ? 1.0f : 0.0f;
    d->w = (s0->w < s1->w) ? 1.0f : 0.0f;
    PSTRACE(("executing slt: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
        s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

void pshader_sub(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
    d->x = s0->x - s1->x;
    d->y = s0->y - s1->y;
    d->z = s0->z - s1->z;
    d->w = s0->w - s1->w;
    PSTRACE(("executing sub: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
        s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

/**
 * Version 1.1 specific
 */

void pshader_exp(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    d->x = d->y = d->z = d->w = powf(2.0f, s0->w);
    PSTRACE(("executing exp: s0=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
        s0->x, s0->y, s0->z, s0->w, d->x, d->y, d->z, d->w));
}

void pshader_log(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    float tmp_f = fabsf(s0->w);
    d->x = d->y = d->z = d->w = (0.0f != tmp_f) ? logf(tmp_f) / logf(2.0f) : -HUGE_VAL;
    PSTRACE(("executing log: s0=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
        s0->x, s0->y, s0->z, s0->w, d->x, d->y, d->z, d->w));
}

void pshader_frc(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    d->x = s0->x - floorf(s0->x);
    d->y = s0->y - floorf(s0->y);
    d->z = 0.0f;
    d->w = 1.0f;
    PSTRACE(("executing frc: s0=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
        s0->x, s0->y, s0->z, s0->w, d->x, d->y, d->z, d->w));
}

typedef FLOAT D3DMATRIX44[4][4];
typedef FLOAT D3DMATRIX43[4][3];
typedef FLOAT D3DMATRIX34[3][4];
typedef FLOAT D3DMATRIX33[3][3];
typedef FLOAT D3DMATRIX23[2][3];

void pshader_m4x4(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, /*WINED3DSHADERVECTOR* mat1*/ D3DMATRIX44 mat) {
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
    PSTRACE(("executing m4x4(1): mat=(%f, %f, %f, %f)    s0=(%f)     d=(%f) \n", mat[0][0], mat[0][1], mat[0][2], mat[0][3], s0->x, d->x));
    PSTRACE(("executing m4x4(2): mat=(%f, %f, %f, %f)       (%f)       (%f) \n", mat[1][0], mat[1][1], mat[1][2], mat[1][3], s0->y, d->y));
    PSTRACE(("executing m4x4(3): mat=(%f, %f, %f, %f) X     (%f)  =    (%f) \n", mat[2][0], mat[2][1], mat[2][2], mat[2][3], s0->z, d->z));
    PSTRACE(("executing m4x4(4): mat=(%f, %f, %f, %f)       (%f)       (%f) \n", mat[3][0], mat[3][1], mat[3][2], mat[3][3], s0->w, d->w));
}

void pshader_m4x3(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, D3DMATRIX34 mat) {
    d->x = mat[0][0] * s0->x + mat[0][1] * s0->y + mat[0][2] * s0->z + mat[0][3] * s0->w;
    d->y = mat[1][0] * s0->x + mat[1][1] * s0->y + mat[1][2] * s0->z + mat[1][3] * s0->w;
    d->z = mat[2][0] * s0->x + mat[2][1] * s0->y + mat[2][2] * s0->z + mat[2][3] * s0->w;
    d->w = 1.0f;
    PSTRACE(("executing m4x3(1): mat=(%f, %f, %f, %f)    s0=(%f)     d=(%f) \n", mat[0][0], mat[0][1], mat[0][2], mat[0][3], s0->x, d->x));
    PSTRACE(("executing m4x3(2): mat=(%f, %f, %f, %f)       (%f)       (%f) \n", mat[1][0], mat[1][1], mat[1][2], mat[1][3], s0->y, d->y));
    PSTRACE(("executing m4x3(3): mat=(%f, %f, %f, %f) X     (%f)  =    (%f) \n", mat[2][0], mat[2][1], mat[2][2], mat[2][3], s0->z, d->z));
    PSTRACE(("executing m4x3(4):                            (%f)       (%f) \n", s0->w, d->w));
}

void pshader_m3x4(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, D3DMATRIX43 mat) {
    d->x = mat[0][0] * s0->x + mat[0][1] * s0->y + mat[0][2] * s0->z;
    d->y = mat[1][0] * s0->x + mat[1][1] * s0->y + mat[1][2] * s0->z;
    d->z = mat[2][0] * s0->x + mat[2][1] * s0->y + mat[2][2] * s0->z;
    d->w = mat[3][0] * s0->x + mat[3][1] * s0->y + mat[3][2] * s0->z;
    PSTRACE(("executing m3x4(1): mat=(%f, %f, %f)    s0=(%f)     d=(%f) \n", mat[0][0], mat[0][1], mat[0][2], s0->x, d->x));
    PSTRACE(("executing m3x4(2): mat=(%f, %f, %f)       (%f)       (%f) \n", mat[1][0], mat[1][1], mat[1][2], s0->y, d->y));
    PSTRACE(("executing m3x4(3): mat=(%f, %f, %f) X     (%f)  =    (%f) \n", mat[2][0], mat[2][1], mat[2][2], s0->z, d->z));
    PSTRACE(("executing m3x4(4): mat=(%f, %f, %f)       (%f)       (%f) \n", mat[3][0], mat[3][1], mat[3][2], s0->w, d->w));
}

void pshader_m3x3(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, D3DMATRIX33 mat) {
    d->x = mat[0][0] * s0->x + mat[0][1] * s0->y + mat[0][2] * s0->z;
    d->y = mat[1][0] * s0->x + mat[1][1] * s0->y + mat[1][2] * s0->z;
    d->z = mat[2][0] * s0->x + mat[2][1] * s0->y + mat[2][2] * s0->z;
    d->w = 1.0f;
    PSTRACE(("executing m3x3(1): mat=(%f, %f, %f)    s0=(%f)     d=(%f) \n", mat[0][0], mat[0][1], mat[0][2], s0->x, d->x));
    PSTRACE(("executing m3x3(2): mat=(%f, %f, %f)       (%f)       (%f) \n", mat[1][0], mat[1][1], mat[1][2], s0->y, d->y));
    PSTRACE(("executing m3x3(3): mat=(%f, %f, %f) X     (%f)  =    (%f) \n", mat[2][0], mat[2][1], mat[2][2], s0->z, d->z));
    PSTRACE(("executing m3x3(4):                                       (%f) \n", d->w));
}

void pshader_m3x2(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, D3DMATRIX23 mat) {
    FIXME("check\n");
    d->x = mat[0][0] * s0->x + mat[0][1] * s0->y + mat[0][2] * s0->z;
    d->y = mat[1][0] * s0->x + mat[1][1] * s0->y + mat[1][2] * s0->z;
    d->z = 0.0f;
    d->w = 1.0f;
}

/**
 * Version 2.0 specific
 */
void pshader_lrp(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1, WINED3DSHADERVECTOR* s2) {
    d->x = s0->x * (s1->x - s2->x) + s2->x;
    d->y = s0->y * (s1->y - s2->y) + s2->y;
    d->z = s0->z * (s1->z - s2->z) + s2->z;
    d->w = s0->w * (s1->w - s2->w) + s2->w;
}

void pshader_crs(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
    d->x = s0->y * s1->z - s0->z * s1->y;
    d->y = s0->z * s1->x - s0->x * s1->z;
    d->z = s0->x * s1->y - s0->y * s1->x;
    d->w = 0.9f; /* w is undefined, so set it to something safeish */

    PSTRACE(("executing crs: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
                s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

void pshader_abs(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    d->x = fabsf(s0->x);
    d->y = fabsf(s0->y);
    d->z = fabsf(s0->z);
    d->w = fabsf(s0->w);
    PSTRACE(("executing abs: s0=(%f, %f, %f, %f)  => d=(%f, %f, %f, %f)\n",
                s0->x, s0->y, s0->z, s0->w, d->x, d->y, d->z, d->w));
}

    /* Stubs */
void pshader_texcoord(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}

void pshader_texkill(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}

void pshader_tex(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}
void pshader_texld(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

void pshader_texbem(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

void pshader_texbeml(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

void pshader_texreg2ar(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

void pshader_texreg2gb(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

void pshader_texm3x2pad(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

void pshader_texm3x2tex(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

void pshader_texm3x3tex(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
    FIXME(" : Stub\n");
}

void pshader_texm3x3pad(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

void pshader_texm3x3diff(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

void pshader_texm3x3spec(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
    FIXME(" : Stub\n");
}

void pshader_texm3x3vspec(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

void pshader_cnd(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1, WINED3DSHADERVECTOR* s2) {
    FIXME(" : Stub\n");
}

/* Def is C[n] = {n.nf, n.nf, n.nf, n.nf} */
void pshader_def(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1, WINED3DSHADERVECTOR* s2, WINED3DSHADERVECTOR* s3) {
    FIXME(" : Stub\n");
}

void pshader_texreg2rgb(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

void pshader_texdp3tex(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

void pshader_texm3x2depth(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

void pshader_texdp3(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

void pshader_texm3x3(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

void pshader_texdepth(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}

void pshader_cmp(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1, WINED3DSHADERVECTOR* s2) {
    FIXME(" : Stub\n");
}

void pshader_bem(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
    FIXME(" : Stub\n");
}

void pshader_call(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}

void pshader_callnz(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

void pshader_loop(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

void pshader_ret(void) {
    FIXME(" : Stub\n");
}

void pshader_endloop(void) {
    FIXME(" : Stub\n");
}

void pshader_dcl(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

void pshader_pow(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
    FIXME(" : Stub\n");
}

void pshader_nrm(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

void pshader_sincos3(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

void pshader_sincos2(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1, WINED3DSHADERVECTOR* s2) {
     FIXME(" : Stub\n");
}

void pshader_rep(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}

void pshader_endrep(void) {
    FIXME(" : Stub\n");
}

void pshader_if(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}

void pshader_ifc(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

void pshader_else(void) {
    FIXME(" : Stub\n");
}

void pshader_label(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}

void pshader_endif(void) {
    FIXME(" : Stub\n");
}

void pshader_break(void) {
    FIXME(" : Stub\n");
}

void pshader_breakc(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

void pshader_breakp(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}

void pshader_defb(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}

void pshader_defi(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}

void pshader_dp2add(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}

void pshader_dsx(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}

void pshader_dsy(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}

void pshader_texldd(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}

void pshader_setp(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}

void pshader_texldl(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}

/* Prototype */
void pshader_hw_map2gl(SHADER_OPCODE_ARG* arg);
void pshader_hw_tex(SHADER_OPCODE_ARG* arg);
void pshader_hw_texcoord(SHADER_OPCODE_ARG* arg);
void pshader_hw_texreg2ar(SHADER_OPCODE_ARG* arg);
void pshader_hw_texreg2gb(SHADER_OPCODE_ARG* arg);
void pshader_hw_texbem(SHADER_OPCODE_ARG* arg);
void pshader_hw_def(SHADER_OPCODE_ARG* arg);
void pshader_hw_texm3x2pad(SHADER_OPCODE_ARG* arg);
void pshader_hw_texm3x2tex(SHADER_OPCODE_ARG* arg);
void pshader_hw_texm3x3pad(SHADER_OPCODE_ARG* arg);
void pshader_hw_texm3x3tex(SHADER_OPCODE_ARG* arg);
void pshader_hw_texm3x3spec(SHADER_OPCODE_ARG* arg);
void pshader_hw_texm3x3vspec(SHADER_OPCODE_ARG* arg);

/**
 * log, exp, frc, m*x* seems to be macros ins ... to see
 */
CONST SHADER_OPCODE IWineD3DPixelShaderImpl_shader_ins[] = {

    /* Arithmethic */
    {D3DSIO_NOP,  "nop", "NOP", 0, pshader_nop, pshader_hw_map2gl, NULL, 0, 0},
    {D3DSIO_MOV,  "mov", "MOV", 2, pshader_mov, pshader_hw_map2gl, NULL, 0, 0},
    {D3DSIO_ADD,  "add", "ADD", 3, pshader_add, pshader_hw_map2gl, NULL, 0, 0},
    {D3DSIO_SUB,  "sub", "SUB", 3, pshader_sub, pshader_hw_map2gl, NULL, 0, 0},
    {D3DSIO_MAD,  "mad", "MAD", 4, pshader_mad, pshader_hw_map2gl, NULL, 0, 0},
    {D3DSIO_MUL,  "mul", "MUL", 3, pshader_mul, pshader_hw_map2gl, NULL, 0, 0},
    {D3DSIO_RCP,  "rcp", "RCP",  2, pshader_rcp, pshader_hw_map2gl, NULL, 0, 0},
    {D3DSIO_RSQ,  "rsq",  "RSQ", 2, pshader_rsq, pshader_hw_map2gl, NULL, 0, 0},
    {D3DSIO_DP3,  "dp3",  "DP3", 3, pshader_dp3, pshader_hw_map2gl, NULL, 0, 0},
    {D3DSIO_DP4,  "dp4",  "DP4", 3, pshader_dp4, pshader_hw_map2gl, NULL, 0, 0},
    {D3DSIO_MIN,  "min",  "MIN", 3, pshader_min, pshader_hw_map2gl, NULL, 0, 0},
    {D3DSIO_MAX,  "max",  "MAX", 3, pshader_max, pshader_hw_map2gl, NULL, 0, 0},
    {D3DSIO_SLT,  "slt",  "SLT", 3, pshader_slt, pshader_hw_map2gl, NULL, 0, 0},
    {D3DSIO_SGE,  "sge",  "SGE", 3, pshader_sge, pshader_hw_map2gl, NULL, 0, 0},
    {D3DSIO_ABS,  "abs",  "ABS", 2, pshader_abs, pshader_hw_map2gl, NULL, 0, 0},
    {D3DSIO_EXP,  "exp",  "EX2", 2, pshader_exp, pshader_hw_map2gl, NULL, 0, 0},
    {D3DSIO_LOG,  "log",  "LG2", 2, pshader_log, pshader_hw_map2gl, NULL, 0, 0},
    {D3DSIO_EXPP, "expp", "EXP", 2, pshader_expp, pshader_hw_map2gl, NULL, 0, 0},
    {D3DSIO_LOGP, "logp", "LOG", 2, pshader_logp, pshader_hw_map2gl, NULL, 0, 0},
    {D3DSIO_DST,  "dst",  "DST", 3, pshader_dst, pshader_hw_map2gl, NULL, 0, 0},
    {D3DSIO_LRP,  "lrp",  "LRP", 4, pshader_lrp, pshader_hw_map2gl, NULL, 0, 0},
    {D3DSIO_FRC,  "frc",  "FRC", 2, pshader_frc, pshader_hw_map2gl, NULL, 0, 0},
    {D3DSIO_CND,  "cnd",  GLNAME_REQUIRE_GLSL, 4, pshader_cnd, NULL, NULL, D3DPS_VERSION(1,1), D3DPS_VERSION(1,4)},
    {D3DSIO_CMP,  "cmp",  GLNAME_REQUIRE_GLSL, 4, pshader_cmp, NULL, NULL, D3DPS_VERSION(1,1), D3DPS_VERSION(3,0)},
    {D3DSIO_POW,  "pow",  "POW", 3, pshader_pow,  NULL, NULL, 0, 0},
    {D3DSIO_CRS,  "crs",  "XPS", 3, pshader_crs,  NULL, NULL, 0, 0},
    /* TODO: xyz normalise can be performed as VS_ARB using one temporary register,
        DP3 tmp , vec, vec;
        RSQ tmp, tmp.x;
        MUL vec.xyz, vec, tmp;
    but I think this is better because it accounts for w properly.
        DP3 tmp , vec, vec;
        RSQ tmp, tmp.x;
        MUL vec, vec, tmp;

    */
    {D3DSIO_NRM,      "nrm",      NULL,   2, pshader_nrm,     NULL, NULL, 0, 0},
    {D3DSIO_SINCOS,   "sincos",   NULL,   4, pshader_sincos2, NULL, NULL, D3DPS_VERSION(2,0), D3DPS_VERSION(2,0)},
    {D3DSIO_SINCOS,   "sincos",   NULL,   2, pshader_sincos3, NULL, NULL, D3DPS_VERSION(3,0), -1},
    /* TODO: dp2add can be made out of multiple instuctions */
    {D3DSIO_DP2ADD,   "dp2add",   GLNAME_REQUIRE_GLSL,  2, pshader_dp2add,  NULL, NULL, 0, 0},

    /* Matrix */
    {D3DSIO_M4x4, "m4x4", "undefined", 3, pshader_m4x4, NULL, NULL, 0, 0},
    {D3DSIO_M4x3, "m4x3", "undefined", 3, pshader_m4x3, NULL, NULL, 0, 0},
    {D3DSIO_M3x4, "m3x4", "undefined", 3, pshader_m3x4, NULL, NULL, 0, 0},
    {D3DSIO_M3x3, "m3x3", "undefined", 3, pshader_m3x3, NULL, NULL, 0, 0},
    {D3DSIO_M3x2, "m3x2", "undefined", 3, pshader_m3x2, NULL, NULL, 0, 0},

    /* Register declarations */
    {D3DSIO_DCL,      "dcl",      NULL,   2, pshader_dcl,     NULL, NULL, 0, 0},

    /* Flow control - requires GLSL or software shaders */
    {D3DSIO_REP ,     "rep",      GLNAME_REQUIRE_GLSL,   1, pshader_rep,     NULL, NULL, 0, 0},
    {D3DSIO_ENDREP,   "endrep",   GLNAME_REQUIRE_GLSL,   0, pshader_endrep,  NULL, NULL, 0, 0},
    {D3DSIO_IF,       "if",       GLNAME_REQUIRE_GLSL,   1, pshader_if,      NULL, NULL, 0, 0},
    {D3DSIO_IFC,      "ifc",      GLNAME_REQUIRE_GLSL,   2, pshader_ifc,     NULL, NULL, 0, 0},
    {D3DSIO_ELSE,     "else",     GLNAME_REQUIRE_GLSL,   0, pshader_else,    NULL, NULL, 0, 0},
    {D3DSIO_ENDIF,    "endif",    GLNAME_REQUIRE_GLSL,   0, pshader_endif,   NULL, NULL, 0, 0},
    {D3DSIO_BREAK,    "break",    GLNAME_REQUIRE_GLSL,   0, pshader_break,   NULL, NULL, 0, 0},
    {D3DSIO_BREAKC,   "breakc",   GLNAME_REQUIRE_GLSL,   2, pshader_breakc,  NULL, NULL, 0, 0},
    {D3DSIO_BREAKP,   "breakp",   GLNAME_REQUIRE_GLSL,   1, pshader_breakp,  NULL, NULL, 0, 0},
    {D3DSIO_CALL,     "call",     GLNAME_REQUIRE_GLSL,   1, pshader_call,    NULL, NULL, 0, 0},
    {D3DSIO_CALLNZ,   "callnz",   GLNAME_REQUIRE_GLSL,   2, pshader_callnz,  NULL, NULL, 0, 0},
    {D3DSIO_LOOP,     "loop",     GLNAME_REQUIRE_GLSL,   2, pshader_loop,    NULL, NULL, 0, 0},
    {D3DSIO_RET,      "ret",      GLNAME_REQUIRE_GLSL,   0, pshader_ret,     NULL, NULL, 0, 0},
    {D3DSIO_ENDLOOP,  "endloop",  GLNAME_REQUIRE_GLSL,   0, pshader_endloop, NULL, NULL, 0, 0},
    {D3DSIO_LABEL,    "label",    GLNAME_REQUIRE_GLSL,   1, pshader_label,   NULL, NULL, 0, 0},

    /* Constant definitions */
    {D3DSIO_DEF,      "def",      "undefined",           5, pshader_def,     pshader_hw_def, NULL, 0, 0},
    {D3DSIO_DEFB,     "defb",     GLNAME_REQUIRE_GLSL,   2, pshader_defb,    NULL, NULL, 0, 0},
    {D3DSIO_DEFI,     "defi",     GLNAME_REQUIRE_GLSL,   2, pshader_defi,    NULL, NULL, 0, 0},

    /* Texture */
    {D3DSIO_TEXCOORD, "texcoord", "undefined",   1, pshader_texcoord,    pshader_hw_texcoord, NULL, 0, D3DPS_VERSION(1,3)},
    {D3DSIO_TEXCOORD, "texcrd",   "undefined",   2, pshader_texcoord,    pshader_hw_texcoord, NULL, D3DPS_VERSION(1,4), D3DPS_VERSION(1,4)},
    {D3DSIO_TEXKILL,  "texkill",  "KIL",         1, pshader_texkill,     pshader_hw_map2gl, NULL, D3DPS_VERSION(1,0), D3DPS_VERSION(3,0)},
    {D3DSIO_TEX,      "tex",      "undefined",   1, pshader_tex,         pshader_hw_tex, NULL, 0, D3DPS_VERSION(1,3)},
    {D3DSIO_TEX,      "texld",    "undefined",   2, pshader_texld,       pshader_hw_tex, NULL, D3DPS_VERSION(1,4), D3DPS_VERSION(1,4)},
    {D3DSIO_TEX,      "texld",    "undefined",   3, pshader_texld,       pshader_hw_tex, NULL, D3DPS_VERSION(2,0), -1},
    {D3DSIO_TEXBEM,   "texbem",   "undefined",   2, pshader_texbem,      pshader_hw_texbem, NULL, 0, D3DPS_VERSION(1,3)},
    {D3DSIO_TEXBEML,  "texbeml",  GLNAME_REQUIRE_GLSL,   2, pshader_texbeml, NULL, NULL, D3DPS_VERSION(1,0), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXREG2AR,"texreg2ar","undefined",   2, pshader_texreg2ar,   pshader_hw_texreg2ar, NULL, D3DPS_VERSION(1,1), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXREG2GB,"texreg2gb","undefined",   2, pshader_texreg2gb,   pshader_hw_texreg2gb, NULL, D3DPS_VERSION(1,2), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXREG2RGB,   "texreg2rgb",   GLNAME_REQUIRE_GLSL,   2, pshader_texreg2rgb,  NULL, NULL, D3DPS_VERSION(1,2), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXM3x2PAD,   "texm3x2pad",   "undefined",   2, pshader_texm3x2pad,   pshader_hw_texm3x2pad, NULL, D3DPS_VERSION(1,0), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXM3x2TEX,   "texm3x2tex",   "undefined",   2, pshader_texm3x2tex,   pshader_hw_texm3x2tex, NULL, D3DPS_VERSION(1,0), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXM3x3PAD,   "texm3x3pad",   "undefined",   2, pshader_texm3x3pad,   pshader_hw_texm3x3pad, NULL, D3DPS_VERSION(1,0), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXM3x3DIFF,  "texm3x3diff",  GLNAME_REQUIRE_GLSL,   2, pshader_texm3x3diff,  NULL, NULL, D3DPS_VERSION(0,0), D3DPS_VERSION(0,0)},
    {D3DSIO_TEXM3x3SPEC,  "texm3x3spec",  "undefined",   3, pshader_texm3x3spec,  pshader_hw_texm3x3spec, NULL, D3DPS_VERSION(1,0), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXM3x3VSPEC, "texm3x3vspe",  "undefined",   2, pshader_texm3x3vspec, pshader_hw_texm3x3vspec, NULL, D3DPS_VERSION(1,0), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXM3x3TEX,   "texm3x3tex",   "undefined",   2, pshader_texm3x3tex,   pshader_hw_texm3x3tex, NULL, D3DPS_VERSION(1,0), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXDP3TEX,    "texdp3tex",    GLNAME_REQUIRE_GLSL,   2, pshader_texdp3tex,   NULL, NULL, D3DPS_VERSION(1,2), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXM3x2DEPTH, "texm3x2depth", GLNAME_REQUIRE_GLSL,   2, pshader_texm3x2depth, NULL, NULL, D3DPS_VERSION(1,3), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXDP3,   "texdp3",   GLNAME_REQUIRE_GLSL,  2, pshader_texdp3,   NULL, NULL, D3DPS_VERSION(1,2), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXM3x3,  "texm3x3",  GLNAME_REQUIRE_GLSL,  2, pshader_texm3x3,  NULL, NULL, D3DPS_VERSION(1,2), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXDEPTH, "texdepth", GLNAME_REQUIRE_GLSL,  1, pshader_texdepth, NULL, NULL, D3DPS_VERSION(1,4), D3DPS_VERSION(1,4)},
    {D3DSIO_BEM,      "bem",      GLNAME_REQUIRE_GLSL,  3, pshader_bem,      NULL, NULL, D3DPS_VERSION(1,4), D3DPS_VERSION(1,4)},
    /* TODO: dp2add can be made out of multiple instuctions */
    {D3DSIO_DSX,      "dsx",      GLNAME_REQUIRE_GLSL,  2, pshader_dsx,     NULL, NULL, 0, 0},
    {D3DSIO_DSY,      "dsy",      GLNAME_REQUIRE_GLSL,  2, pshader_dsy,     NULL, NULL, 0, 0},
    {D3DSIO_TEXLDD,   "texldd",   GLNAME_REQUIRE_GLSL,  2, pshader_texldd,  NULL, NULL, 0, 0},
    {D3DSIO_SETP,     "setp",     GLNAME_REQUIRE_GLSL,  2, pshader_setp,    NULL, NULL, 0, 0},
    {D3DSIO_TEXLDL,   "texdl",    GLNAME_REQUIRE_GLSL,  2, pshader_texldl,  NULL, NULL, 0, 0},
    {D3DSIO_PHASE,    "phase",    GLNAME_REQUIRE_GLSL,  0, pshader_nop,     NULL, NULL, 0, 0},
    {0,               NULL,       NULL,   0, NULL,            NULL, 0, 0}
};

inline static void get_register_name(const DWORD param, char* regstr, char constants[WINED3D_PSHADER_MAX_CONSTANTS]) {
    static const char* rastout_reg_names[] = { "oC0", "oC1", "oC2", "oC3", "oDepth" };

    DWORD reg = param & D3DSP_REGNUM_MASK;
    DWORD regtype = shader_get_regtype(param);

    switch (regtype) {
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
        FIXME("Unhandled register name Type(%ld)\n", regtype);
        sprintf(regstr, "unrecognized_register");
    break;
    }
}

inline static void get_write_mask(const DWORD output_reg, char *write_mask) {
    *write_mask = 0;
    if ((output_reg & D3DSP_WRITEMASK_ALL) != D3DSP_WRITEMASK_ALL) {
        strcat(write_mask, ".");
        if (output_reg & D3DSP_WRITEMASK_0) strcat(write_mask, "r");
        if (output_reg & D3DSP_WRITEMASK_1) strcat(write_mask, "g");
        if (output_reg & D3DSP_WRITEMASK_2) strcat(write_mask, "b");
        if (output_reg & D3DSP_WRITEMASK_3) strcat(write_mask, "a");
    }
}

inline static void get_input_register_swizzle(const DWORD instr, char *swzstring) {
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

inline static void gen_output_modifier_line(int saturate, char *write_mask, int shift, char *regstr, char* line) {
    /* Generate a line that does the output modifier computation */
    sprintf(line, "MUL%s %s%s, %s, %s;", saturate ? "_SAT" : "", regstr, write_mask, regstr, shift_tab[shift]);
}

inline static int gen_input_modifier_line(const DWORD instr, int tmpreg, char *outregstr, char *line, char constants[WINED3D_PSHADER_MAX_CONSTANTS]) {
    /* Generate a line that does the input modifier computation and return the input register to use */
    static char regstr[256];
    static char tmpline[256];
    int insert_line;

    /* Assume a new line will be added */
    insert_line = 1;

    /* Get register name */
    get_register_name(instr, regstr, constants);

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
        sprintf(line, "RCP T%c, %s.w;", 'A' + tmpreg, regstr);
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

void pshader_set_version(
      IWineD3DPixelShaderImpl *This, 
      DWORD version) {

      DWORD major = (version >> 8) & 0x0F;
      DWORD minor = version & 0x0F;

      This->baseShader.hex_version = version;
      This->baseShader.version = major * 10 + minor;
      TRACE("ps_%lu_%lu\n", major, minor);

      This->baseShader.limits.address = 0;

      switch (This->baseShader.version) {
          case 10:
          case 11:
          case 12:
          case 13: This->baseShader.limits.temporary = 2;
                   This->baseShader.limits.constant_float = 8;
                   This->baseShader.limits.constant_int = 0;
                   This->baseShader.limits.constant_bool = 0;
                   This->baseShader.limits.texture = 4;
                   break;

          case 14: This->baseShader.limits.temporary = 6;
                   This->baseShader.limits.constant_float = 8;
                   This->baseShader.limits.constant_int = 0;
                   This->baseShader.limits.constant_bool = 0;
                   This->baseShader.limits.texture = 6;
                   break;
               
          /* FIXME: temporaries must match D3DPSHADERCAPS2_0.NumTemps */ 
          case 20: This->baseShader.limits.temporary = 32;
                   This->baseShader.limits.constant_float = 32;
                   This->baseShader.limits.constant_int = 16;
                   This->baseShader.limits.constant_bool = 16;
                   This->baseShader.limits.texture = 8;
                   break;

          case 30: This->baseShader.limits.temporary = 32;
                   This->baseShader.limits.constant_float = 224;
                   This->baseShader.limits.constant_int = 16;
                   This->baseShader.limits.constant_bool = 16;
                   This->baseShader.limits.texture = 0;
                   break;

          default: This->baseShader.limits.temporary = 32;
                   This->baseShader.limits.constant_float = 8;
                   This->baseShader.limits.constant_int = 0;
                   This->baseShader.limits.constant_bool = 0;
                   This->baseShader.limits.texture = 8;
                   FIXME("Unrecognized pixel shader version %lx!\n", version);
      }
}

/* Map the opcode 1-to-1 to the GL code */
/* FIXME: fix CMP/CND, get rid of this switch */
void pshader_hw_map2gl(SHADER_OPCODE_ARG* arg) {

     IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
     CONST SHADER_OPCODE* curOpcode = arg->opcode;
     SHADER_BUFFER* buffer = arg->buffer;
     DWORD dst = arg->dst;
     DWORD* src = arg->src;

     unsigned int i;
     char tmpLine[256];

     /* Output token related */
     char output_rname[256];
     char output_wmask[20];
     BOOL saturate = FALSE;
     BOOL centroid = FALSE;
     BOOL partialprecision = FALSE;
     DWORD shift;

     strcpy(tmpLine, curOpcode->glname);

     /* Process modifiers */
     if (0 != (dst & D3DSP_DSTMOD_MASK)) {
         DWORD mask = dst & D3DSP_DSTMOD_MASK;

         saturate = mask & D3DSPDM_SATURATE;
         centroid = mask & D3DSPDM_MSAMPCENTROID;
         partialprecision = mask & D3DSPDM_PARTIALPRECISION;
         mask &= ~(D3DSPDM_MSAMPCENTROID | D3DSPDM_PARTIALPRECISION | D3DSPDM_SATURATE);

         if (mask)
            FIXME("Unrecognized modifier(0x%#lx)\n", mask >> D3DSP_DSTMOD_SHIFT);

         if (centroid)
             FIXME("Unhandled modifier(0x%#lx)\n", mask >> D3DSP_DSTMOD_SHIFT);
     }
     shift = (dst & D3DSP_DSTSHIFT_MASK) >> D3DSP_DSTSHIFT_SHIFT;

      /* Generate input and output registers */
      if (curOpcode->num_params > 0) {
          char regs[5][50];
          char operands[4][100];
          char swzstring[20];
          char tmpOp[256];

          /* Generate lines that handle input modifier computation */
          for (i = 1; i < curOpcode->num_params; ++i) {
              if (gen_input_modifier_line(src[i - 1], i - 1, regs[i - 1], tmpOp, This->constants))
                  shader_addline(buffer, tmpOp);
          }

          /* Handle output register */
          get_register_name(dst, output_rname, This->constants);
          strcpy(operands[0], output_rname);
          get_write_mask(dst, output_wmask);
          strcat(operands[0], output_wmask);

          /* This function works because of side effects from  gen_input_modifier_line */
          /* Handle input registers */
          for (i = 1; i < curOpcode->num_params; ++i) {
              strcpy(operands[i], regs[i - 1]);
              get_input_register_swizzle(src[i - 1], swzstring);
              strcat(operands[i], swzstring);
          }

          switch(curOpcode->opcode) {
              case D3DSIO_CMP:
                  sprintf(tmpLine, "CMP%s %s, %s, %s, %s;\n", (saturate ? "_SAT" : ""),
                      operands[0], operands[1], operands[3], operands[2]);
              break;
            case D3DSIO_CND:
                  shader_addline(buffer, "ADD TMP, -%s, coefdiv.x;\n", operands[1]);
                  sprintf(tmpLine, "CMP%s %s, TMP, %s, %s;\n", (saturate ? "_SAT" : ""),
                      operands[0], operands[2], operands[3]);
              break;

              default:
                  if (saturate && (shift == 0))
                      strcat(tmpLine, "_SAT");
                  strcat(tmpLine, " ");
                  strcat(tmpLine, operands[0]);
                  for (i = 1; i < curOpcode->num_params; i++) {
                      strcat(tmpLine, ", ");
                      strcat(tmpLine, operands[i]);
                  }
                  strcat(tmpLine,";\n");
          }
          shader_addline(buffer, tmpLine);

          /* A shift requires another line. */
          if (shift != 0) {
              gen_output_modifier_line(saturate, output_wmask, shift, output_rname, tmpLine);
              shader_addline(buffer, tmpLine);
          }
      }
}

void pshader_hw_tex(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    DWORD dst = arg->dst;
    DWORD* src = arg->src;
    SHADER_BUFFER* buffer = arg->buffer;
    DWORD version = This->baseShader.version;
   
    char tmpLine[256];
    char reg_dest[40];
    char reg_coord[40];
    char reg_coord_swz[20] = "";
    DWORD reg_dest_code;
    DWORD reg_sampler_code;

    /* All versions have a destination register */
    reg_dest_code = dst & D3DSP_REGNUM_MASK;
    get_register_name(dst, reg_dest, This->constants);

    /* 1.0-1.3: Use destination register as coordinate source. No modifiers.
       1.4: Use provided coordinate source register. _dw, _dz, swizzle allowed.
       2.0+: Use provided coordinate source register. No modifiers.
       3.0+: Use provided coordinate source register. Swizzle allowed */
   if (version < 14)
      strcpy(reg_coord, reg_dest);
   else if (version == 14) {
      if (gen_input_modifier_line(src[0], 0, reg_coord, tmpLine, This->constants))
         shader_addline(buffer, tmpLine);
      get_input_register_swizzle(src[0], reg_coord_swz);
   }
   else if (version > 14 && version < 30)
      get_register_name(src[0], reg_coord, This->constants);
   else if (version >= 30) {
      get_input_register_swizzle(src[0], reg_coord_swz);
      get_register_name(src[0], reg_coord, This->constants);
   }

  /* 1.0-1.4: Use destination register number as texture code.
     2.0+: Use provided sampler number as texure code. */
  if (version < 20)
     reg_sampler_code = reg_dest_code;
  else 
     reg_sampler_code = src[1] & D3DSP_REGNUM_MASK;

  shader_addline(buffer, "TEX %s, %s%s, texture[%lu], 2D;\n",
      reg_dest, reg_coord, reg_coord_swz, reg_sampler_code);
}

void pshader_hw_texcoord(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    DWORD dst = arg->dst;
    DWORD* src = arg->src;
    SHADER_BUFFER* buffer = arg->buffer;
    DWORD version = This->baseShader.version;

    char tmp[20];
    get_write_mask(dst, tmp);
    if (version != 14) {
        DWORD reg = dst & D3DSP_REGNUM_MASK;
        shader_addline(buffer, "MOV T%lu%s, fragment.texcoord[%lu];\n", reg, tmp, reg);
    } else {
        DWORD reg1 = dst & D3DSP_REGNUM_MASK;
        DWORD reg2 = src[0] & D3DSP_REGNUM_MASK;
        shader_addline(buffer, "MOV R%lu%s, fragment.texcoord[%lu];\n", reg1, tmp, reg2);
   }
}

void pshader_hw_texreg2ar(SHADER_OPCODE_ARG* arg) {

     SHADER_BUFFER* buffer = arg->buffer;
 
     DWORD reg1 = arg->dst & D3DSP_REGNUM_MASK;
     DWORD reg2 = arg->src[0] & D3DSP_REGNUM_MASK;
     shader_addline(buffer, "MOV TMP.r, T%lu.a;\n", reg2);
     shader_addline(buffer, "MOV TMP.g, T%lu.r;\n", reg2);
     shader_addline(buffer, "TEX T%lu, TMP, texture[%lu], 2D;\n", reg1, reg1);
}

void pshader_hw_texreg2gb(SHADER_OPCODE_ARG* arg) {
 
     SHADER_BUFFER* buffer = arg->buffer;
    
     DWORD reg1 = arg->dst & D3DSP_REGNUM_MASK;
     DWORD reg2 = arg->src[0] & D3DSP_REGNUM_MASK;
     shader_addline(buffer, "MOV TMP.r, T%lu.g;\n", reg2);
     shader_addline(buffer, "MOV TMP.g, T%lu.b;\n", reg2);
     shader_addline(buffer, "TEX T%lu, TMP, texture[%lu], 2D;\n", reg1, reg1);
}

void pshader_hw_texbem(SHADER_OPCODE_ARG* arg) {

     SHADER_BUFFER* buffer = arg->buffer;
 
     DWORD reg1 = arg->dst  & D3DSP_REGNUM_MASK;
     DWORD reg2 = arg->src[0] & D3DSP_REGNUM_MASK;

     /* FIXME: Should apply the BUMPMAPENV matrix */
     shader_addline(buffer, "ADD TMP.rg, fragment.texcoord[%lu], T%lu;\n", reg1, reg2);
     shader_addline(buffer, "TEX T%lu, TMP, texture[%lu], 2D;\n", reg1, reg1);
}

void pshader_hw_def(SHADER_OPCODE_ARG* arg) {
    
    IWineD3DPixelShaderImpl* shader = (IWineD3DPixelShaderImpl*) arg->shader;
    DWORD reg = arg->dst & D3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;
    
    shader_addline(buffer, 
              "PARAM C%lu = { %f, %f, %f, %f };\n", reg,
              *((float*) (arg->src + 0)),
              *((float*) (arg->src + 1)),
              *((float*) (arg->src + 2)),
              *((float*) (arg->src + 3)) );

    shader->constants[reg] = 1;
}

void pshader_hw_texm3x2pad(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* shader = (IWineD3DPixelShaderImpl*) arg->shader;
    DWORD reg = arg->dst & D3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;
    char tmpLine[256];
    char buf[50];

    if (gen_input_modifier_line(arg->src[0], 0, buf, tmpLine, shader->constants)) 
        shader_addline(buffer, tmpLine);
    shader_addline(buffer, "DP3 TMP.x, T%lu, %s;\n", reg, buf);
}

void pshader_hw_texm3x2tex(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* shader = (IWineD3DPixelShaderImpl*) arg->shader;
    DWORD reg = arg->dst & D3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;
    char tmpLine[256];
    char buf[50];

    if (gen_input_modifier_line(arg->src[0], 0, buf, tmpLine, shader->constants)) 
        shader_addline(buffer, tmpLine);
    shader_addline(buffer, "DP3 TMP.y, T%lu, %s;\n", reg, buf);
    shader_addline(buffer, "TEX T%lu, TMP, texture[%lu], 2D;\n", reg, reg);
}

void pshader_hw_texm3x3pad(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* shader = (IWineD3DPixelShaderImpl*) arg->shader;
    DWORD reg = arg->dst & D3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;
    SHADER_PARSE_STATE current_state = shader->baseShader.parse_state;
    char tmpLine[256];
    char buf[50];

    if (gen_input_modifier_line(arg->src[0], 0, buf, tmpLine, shader->constants)) 
        shader_addline(buffer, tmpLine);
    shader_addline(buffer, "DP3 TMP.%c, T%lu, %s;\n", 'x' + current_state.current_row, reg, buf);
    current_state.texcoord_w[current_state.current_row++] = reg;
}

void pshader_hw_texm3x3tex(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* shader = (IWineD3DPixelShaderImpl*) arg->shader;
    DWORD reg = arg->dst & D3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;
    SHADER_PARSE_STATE current_state = shader->baseShader.parse_state;
    char tmpLine[256];
    char buf[50];

    if (gen_input_modifier_line(arg->src[0], 0, buf, tmpLine, shader->constants)) 
        shader_addline(buffer, tmpLine);
    shader_addline(buffer, "DP3 TMP.z, T%lu, %s;\n", reg, buf);

    /* Cubemap textures will be more used than 3D ones. */
    shader_addline(buffer, "TEX T%lu, TMP, texture[%lu], CUBE;\n", reg, reg);
    current_state.current_row = 0;
}

void pshader_hw_texm3x3vspec(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* shader = (IWineD3DPixelShaderImpl*) arg->shader;
    DWORD reg = arg->dst & D3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;
    SHADER_PARSE_STATE current_state = shader->baseShader.parse_state;
    char tmpLine[256];
    char buf[50];

    if (gen_input_modifier_line(arg->src[0], 0, buf, tmpLine, shader->constants)) 
        shader_addline(buffer, tmpLine);
    shader_addline(buffer, "DP3 TMP.z, T%lu, %s;\n", reg, buf);

    /* Construct the eye-ray vector from w coordinates */
    shader_addline(buffer, "MOV TMP2.x, fragment.texcoord[%lu].w;\n", current_state.texcoord_w[0]);
    shader_addline(buffer, "MOV TMP2.y, fragment.texcoord[%lu].w;\n", current_state.texcoord_w[1]);
    shader_addline(buffer, "MOV TMP2.z, fragment.texcoord[%lu].w;\n", reg);

    /* Calculate reflection vector (Assume normal is normalized): RF = 2*(N.E)*N -E */
    shader_addline(buffer, "DP3 TMP.w, TMP, TMP2;\n");
    shader_addline(buffer, "MUL TMP, TMP.w, TMP;\n");
    shader_addline(buffer, "MAD TMP, coefmul.x, TMP, -TMP2;\n");

    /* Cubemap textures will be more used than 3D ones. */
    shader_addline(buffer, "TEX T%lu, TMP, texture[%lu], CUBE;\n", reg, reg);
    current_state.current_row = 0;
}

void pshader_hw_texm3x3spec(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* shader = (IWineD3DPixelShaderImpl*) arg->shader;
    DWORD reg = arg->dst & D3DSP_REGNUM_MASK;
    DWORD reg3 = arg->src[1] & D3DSP_REGNUM_MASK;
    SHADER_PARSE_STATE current_state = shader->baseShader.parse_state;
    SHADER_BUFFER* buffer = arg->buffer;
    char tmpLine[256];
    char buf[50];

    if (gen_input_modifier_line(arg->src[0], 0, buf, tmpLine, shader->constants)) 
        shader_addline(buffer, tmpLine);
    shader_addline(buffer, "DP3 TMP.z, T%lu, %s;\n", reg, buf);

    /* Calculate reflection vector (Assume normal is normalized): RF = 2*(N.E)*N -E */
    shader_addline(buffer, "DP3 TMP.w, TMP, C[%lu];\n", reg3);
    shader_addline(buffer, "MUL TMP, TMP.w, TMP;\n");
    shader_addline(buffer, "MAD TMP, coefmul.x, TMP, -C[%lu];\n", reg3);

    /* Cubemap textures will be more used than 3D ones. */
    shader_addline(buffer, "TEX T%lu, TMP, texture[%lu], CUBE;\n", reg, reg);
    current_state.current_row = 0;
}

/** Generate a pixel shader string using either GL_FRAGMENT_PROGRAM_ARB
    or GLSL and send it to the card */
inline static VOID IWineD3DPixelShaderImpl_GenerateShader(
    IWineD3DPixelShader *iface,
    CONST DWORD *pFunction) {

    IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *)iface;
    SHADER_BUFFER buffer;

#if 0 /* FIXME: Use the buffer that is held by the device, this is ok since fixups will be skipped for software shaders
        it also requires entering a critical section but cuts down the runtime footprint of wined3d and any memory fragmentation that may occur... */
    if (This->device->fixupVertexBufferSize < SHADER_PGMSIZE) {
        HeapFree(GetProcessHeap(), 0, This->fixupVertexBuffer);
        This->fixupVertexBuffer = HeapAlloc(GetProcessHeap() , 0, SHADER_PGMSIZE);
        This->fixupVertexBufferSize = PGMSIZE;
        This->fixupVertexBuffer[0] = 0;
    }
    buffer.buffer = This->device->fixupVertexBuffer;
#else
    buffer.buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, SHADER_PGMSIZE); 
#endif
    buffer.bsize = 0;
    buffer.lineNo = 0;

    /* TODO: Optionally, generate the GLSL shader instead */
    if (GL_SUPPORT(ARB_VERTEX_PROGRAM)) {
        /*  Create the hw ARB shader */
        shader_addline(&buffer, "!!ARBfp1.0\n");

        shader_addline(&buffer, "TEMP TMP;\n");     /* Used in matrix ops */
        shader_addline(&buffer, "TEMP TMP2;\n");    /* Used in matrix ops */
        shader_addline(&buffer, "TEMP TA;\n");      /* Used for modifiers */
        shader_addline(&buffer, "TEMP TB;\n");      /* Used for modifiers */
        shader_addline(&buffer, "TEMP TC;\n");      /* Used for modifiers */
        shader_addline(&buffer, "PARAM coefdiv = { 0.5, 0.25, 0.125, 0.0625 };\n");
        shader_addline(&buffer, "PARAM coefmul = { 2, 4, 8, 16 };\n");
        shader_addline(&buffer, "PARAM one = { 1.0, 1.0, 1.0, 1.0 };\n");

        /** Call the base shader generation routine to generate most 
            of the pixel shader string for us */
        generate_base_shader( (IWineD3DBaseShader*) This, &buffer, pFunction);

        /*FIXME: This next line isn't valid for certain pixel shader versions */
        shader_addline(&buffer, "MOV result.color, R0;\n");
        shader_addline(&buffer, "END\n\0"); 

        /* TODO: change to resource.glObjectHandle or something like that */
        GL_EXTCALL(glGenProgramsARB(1, &This->baseShader.prgId));

        TRACE("Creating a hw pixel shader, prg=%d\n", This->baseShader.prgId);
        GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, This->baseShader.prgId));

        TRACE("Created hw pixel shader, prg=%d\n", This->baseShader.prgId);
        /* Create the program and check for errors */
        GL_EXTCALL(glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
            buffer.bsize, buffer.buffer));

        if (glGetError() == GL_INVALID_OPERATION) {
            GLint errPos;
            glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errPos);
            FIXME("HW PixelShader Error at position %d: %s\n",
                  errPos, debugstr_a((const char *)glGetString(GL_PROGRAM_ERROR_STRING_ARB)));
            This->baseShader.prgId = -1;
        }
    }

#if 1 /* if were using the data buffer of device then we don't need to free it */
  HeapFree(GetProcessHeap(), 0, buffer.buffer);
#endif
}

HRESULT WINAPI IWineD3DPixelShaderImpl_SetFunction(IWineD3DPixelShader *iface, CONST DWORD *pFunction) {
    IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *)iface;
    const DWORD* pToken = pFunction;
    const SHADER_OPCODE *curOpcode = NULL;
    DWORD opcode_token;
    DWORD len = 0;
    DWORD i;
    TRACE("(%p) : Parsing programme\n", This);

    if (NULL != pToken) {
        while (D3DPS_END() != *pToken) {
            if (shader_is_pshader_version(*pToken)) { /** version */
                pshader_set_version(This, *pToken);
                ++pToken;
                ++len;
                continue;
            }
            if (shader_is_comment(*pToken)) { /** comment */
                DWORD comment_len = (*pToken & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;
                ++pToken;
                TRACE("//%s\n", (char*)pToken);
                pToken += comment_len;
                len += comment_len + 1;
                continue;
            }
            if (!This->baseShader.version) {
                WARN("(%p) : pixel shader doesn't have a valid version identifier\n", This);
            }
            opcode_token = *pToken++;
            curOpcode = shader_get_opcode((IWineD3DBaseShader*) This, opcode_token);
            len++;
            if (NULL == curOpcode) {
                int tokens_read;

                FIXME("Unrecognized opcode: token=%08lX\n", opcode_token);
                tokens_read = shader_skip_unrecognized((IWineD3DBaseShader*) This, pToken);
                pToken += tokens_read;
                len += tokens_read;

            } else {
                if (curOpcode->opcode == D3DSIO_DCL) {
                    DWORD usage = *pToken;
                    DWORD param = *(pToken + 1);
                    DWORD regtype = shader_get_regtype(param);

                    /* Only print extended declaration for samplers or 3.0 input registers */
                    if (regtype == D3DSPR_SAMPLER ||
                        (This->baseShader.version >= 30 && regtype == D3DSPR_INPUT))
                         shader_program_dump_decl_usage(usage, param);
                    else
                         TRACE("dcl");

                    shader_dump_ins_modifiers(param);
                    TRACE(" ");
                    shader_dump_param((IWineD3DBaseShader*) This, param, 0, 0);
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

                    DWORD param, addr_token;
                    int tokens_read;

                    TRACE("%s", curOpcode->name);
                    if (curOpcode->num_params > 0) {

                        tokens_read = shader_get_param((IWineD3DBaseShader*) This,
                            pToken, &param, &addr_token);                       
                        pToken += tokens_read;
                        len += tokens_read;

                        shader_dump_ins_modifiers(param);
                        TRACE(" ");
                        shader_dump_param((IWineD3DBaseShader*) This, param, addr_token, 0);
                         
                        for (i = 1; i < curOpcode->num_params; ++i) {

                            tokens_read = shader_get_param((IWineD3DBaseShader*) This,
                               pToken, &param, &addr_token);
                            pToken += tokens_read;
                            len += tokens_read;

                            TRACE(", ");
                            shader_dump_param((IWineD3DBaseShader*) This, param, addr_token, 1);
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

    /* Generate HW shader in needed */
    if (NULL != pFunction  && wined3d_settings.vs_mode == VS_HW) {
        TRACE("(%p) : Generating hardware program\n", This);
#if 1
        IWineD3DPixelShaderImpl_GenerateShader(iface, pFunction);
#endif
    }

    TRACE("(%p) : Copying the function\n", This);
    /* copy the function ... because it will certainly be released by application */
    if (NULL != pFunction) {
        This->baseShader.function = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, This->baseShader.functionLength);
        memcpy((void *)This->baseShader.function, pFunction, This->baseShader.functionLength);
    } else {
        This->baseShader.function = NULL;
    }

    /* TODO: Some proper return values for failures */
    TRACE("(%p) : Returning WINED3D_OK\n", This);
    return WINED3D_OK;
}

const IWineD3DPixelShaderVtbl IWineD3DPixelShader_Vtbl =
{
    /*** IUnknown methods ***/
    IWineD3DPixelShaderImpl_QueryInterface,
    IWineD3DPixelShaderImpl_AddRef,
    IWineD3DPixelShaderImpl_Release,
    /*** IWineD3DBase methods ***/
    IWineD3DPixelShaderImpl_GetParent,
    /*** IWineD3DBaseShader methods ***/
    IWineD3DPixelShaderImpl_SetFunction,
    /*** IWineD3DPixelShader methods ***/
    IWineD3DPixelShaderImpl_GetDevice,
    IWineD3DPixelShaderImpl_GetFunction
};
