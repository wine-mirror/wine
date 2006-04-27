/*
 * shaders implementation
 *
 * Copyright 2005      Oliver Stieber
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

/* The maximum size of the program */
#define PGMSIZE 65535

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

void pshader_sincos(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
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

/**
 * log, exp, frc, m*x* seems to be macros ins ... to see
 */
CONST SHADER_OPCODE IWineD3DPixelShaderImpl_shader_ins[] = {
    {D3DSIO_NOP,  "nop", "NOP", 0, pshader_nop, 0, 0},
    {D3DSIO_MOV,  "mov", "MOV", 2, pshader_mov, 0, 0},
    {D3DSIO_ADD,  "add", "ADD", 3, pshader_add, 0, 0},
    {D3DSIO_SUB,  "sub", "SUB", 3, pshader_sub, 0, 0},
    {D3DSIO_MAD,  "mad", "MAD", 4, pshader_mad, 0, 0},
    {D3DSIO_MUL,  "mul", "MUL", 3, pshader_mul, 0, 0},
    {D3DSIO_RCP,  "rcp", "RCP",  2, pshader_rcp, 0, 0},
    {D3DSIO_RSQ,  "rsq",  "RSQ", 2, pshader_rsq, 0, 0},
    {D3DSIO_DP3,  "dp3",  "DP3", 3, pshader_dp3, 0, 0},
    {D3DSIO_DP4,  "dp4",  "DP4", 3, pshader_dp4, 0, 0},
    {D3DSIO_MIN,  "min",  "MIN", 3, pshader_min, 0, 0},
    {D3DSIO_MAX,  "max",  "MAX", 3, pshader_max, 0, 0},
    {D3DSIO_SLT,  "slt",  "SLT", 3, pshader_slt, 0, 0},
    {D3DSIO_SGE,  "sge",  "SGE", 3, pshader_sge, 0, 0},
    {D3DSIO_ABS,  "abs",  "ABS", 2, pshader_abs, 0, 0},
    {D3DSIO_EXP,  "exp",  "EX2", 2, pshader_exp, 0, 0},
    {D3DSIO_LOG,  "log",  "LG2", 2, pshader_log, 0, 0},
    {D3DSIO_DST,  "dst",  "DST", 3, pshader_dst, 0, 0},
    {D3DSIO_LRP,  "lrp",  "LRP", 4, pshader_lrp, 0, 0},
    {D3DSIO_FRC,  "frc",  "FRC", 2, pshader_frc, 0, 0},
    {D3DSIO_M4x4, "m4x4", "undefined", 3, pshader_m4x4, 0, 0},
    {D3DSIO_M4x3, "m4x3", "undefined", 3, pshader_m4x3, 0, 0},
    {D3DSIO_M3x4, "m3x4", "undefined", 3, pshader_m3x4, 0, 0},
    {D3DSIO_M3x3, "m3x3", "undefined", 3, pshader_m3x3, 0, 0},
    {D3DSIO_M3x2, "m3x2", "undefined", 3, pshader_m3x2, 0, 0},


    /** FIXME: use direct access so add the others opcodes as stubs */
    /* DCL is a specil operation */
    {D3DSIO_DCL,      "dcl",      NULL,   2, pshader_dcl,     0, 0},
    {D3DSIO_POW,      "pow",      "POW",  3, pshader_pow,     0, 0},
    {D3DSIO_CRS,      "crs",      "XPS",  3, pshader_crs,     0, 0},
    /* TODO: xyz normalise can be performed as VS_ARB using one temporary register,
        DP3 tmp , vec, vec;
        RSQ tmp, tmp.x;
        MUL vec.xyz, vec, tmp;
    but I think this is better because it accounts for w properly.
        DP3 tmp , vec, vec;
        RSQ tmp, tmp.x;
        MUL vec, vec, tmp;

    */
    {D3DSIO_NRM,      "nrm",      NULL,   2, pshader_nrm,     0, 0},
    {D3DSIO_SINCOS,   "sincos",   NULL,   2, pshader_sincos,  0, 0},

    /* Flow control - requires GLSL or software shaders */
    {D3DSIO_REP ,     "rep",      GLNAME_REQUIRE_GLSL,   1, pshader_rep,     0, 0},
    {D3DSIO_ENDREP,   "endrep",   GLNAME_REQUIRE_GLSL,   0, pshader_endrep,  0, 0},
    {D3DSIO_IF,       "if",       GLNAME_REQUIRE_GLSL,   1, pshader_if,      0, 0},
    {D3DSIO_IFC,      "ifc",      GLNAME_REQUIRE_GLSL,   2, pshader_ifc,     0, 0},
    {D3DSIO_ELSE,     "else",     GLNAME_REQUIRE_GLSL,   0, pshader_else,    0, 0},
    {D3DSIO_ENDIF,    "endif",    GLNAME_REQUIRE_GLSL,   0, pshader_endif,   0, 0},
    {D3DSIO_BREAK,    "break",    GLNAME_REQUIRE_GLSL,   0, pshader_break,   0, 0},
    {D3DSIO_BREAKC,   "breakc",   GLNAME_REQUIRE_GLSL,   2, pshader_breakc,  0, 0},
    {D3DSIO_BREAKP,   "breakp",   GLNAME_REQUIRE_GLSL,   1, pshader_breakp,  0, 0},
    {D3DSIO_CALL,     "call",     GLNAME_REQUIRE_GLSL,   1, pshader_call,    0, 0},
    {D3DSIO_CALLNZ,   "callnz",   GLNAME_REQUIRE_GLSL,   2, pshader_callnz,  0, 0},
    {D3DSIO_LOOP,     "loop",     GLNAME_REQUIRE_GLSL,   2, pshader_loop,    0, 0},
    {D3DSIO_RET,      "ret",      GLNAME_REQUIRE_GLSL,   0, pshader_ret,     0, 0},
    {D3DSIO_ENDLOOP,  "endloop",  GLNAME_REQUIRE_GLSL,   0, pshader_endloop, 0, 0},
    {D3DSIO_LABEL,    "label",    GLNAME_REQUIRE_GLSL,   1, pshader_label,   0, 0},

    {D3DSIO_DEFB,     "defb",     GLNAME_REQUIRE_GLSL,   2, pshader_defb,    0, 0},
    {D3DSIO_DEFI,     "defi",     GLNAME_REQUIRE_GLSL,   2, pshader_defi,    0, 0},

    {D3DSIO_TEXCOORD, "texcoord", "undefined",   1, pshader_texcoord,    0, D3DPS_VERSION(1,3)},
    {D3DSIO_TEXCOORD, "texcrd",   "undefined",   2, pshader_texcoord,    D3DPS_VERSION(1,4), D3DPS_VERSION(1,4)},
    {D3DSIO_TEXKILL,  "texkill",  "KIL",   1, pshader_texkill,     D3DPS_VERSION(1,0), D3DPS_VERSION(3,0)},
    {D3DSIO_TEX,      "tex",      "undefined",   1, pshader_tex,         0, D3DPS_VERSION(1,3)},
    {D3DSIO_TEX,      "texld",    "undefined",   2, pshader_texld,       D3DPS_VERSION(1,4), D3DPS_VERSION(1,4)},
    {D3DSIO_TEX,      "texld",    "undefined",   3, pshader_texld,       D3DPS_VERSION(2,0), -1},
    {D3DSIO_TEXBEM,   "texbem",   "undefined",   2, pshader_texbem,      0, D3DPS_VERSION(1,3)},
    {D3DSIO_TEXBEML,  "texbeml",  GLNAME_REQUIRE_GLSL,   2, pshader_texbeml,     D3DPS_VERSION(1,0), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXREG2AR,"texreg2ar","undefined",   2, pshader_texreg2ar,   D3DPS_VERSION(1,1), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXREG2GB,"texreg2gb","undefined",   2, pshader_texreg2gb,   D3DPS_VERSION(1,2), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXM3x2PAD,   "texm3x2pad",   "undefined",   2, pshader_texm3x2pad,   D3DPS_VERSION(1,0), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXM3x2TEX,   "texm3x2tex",   "undefined",   2, pshader_texm3x2tex,   D3DPS_VERSION(1,0), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXM3x3PAD,   "texm3x3pad",   "undefined",   2, pshader_texm3x3pad,   D3DPS_VERSION(1,0), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXM3x3DIFF,  "texm3x3diff",  GLNAME_REQUIRE_GLSL,   2, pshader_texm3x3diff,  D3DPS_VERSION(0,0), D3DPS_VERSION(0,0)},
    {D3DSIO_TEXM3x3SPEC,  "texm3x3spec",  "undefined",   3, pshader_texm3x3spec,  D3DPS_VERSION(1,0), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXM3x3VSPEC, "texm3x3vspe",  "undefined",   2, pshader_texm3x3vspec, D3DPS_VERSION(1,0), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXM3x3TEX,   "texm3x3tex",   "undefined",   2, pshader_texm3x3tex,   D3DPS_VERSION(1,0), D3DPS_VERSION(1,3)},
    {D3DSIO_EXPP,     "expp",     "EXP", 2, pshader_expp, 0, 0},
    {D3DSIO_LOGP,     "logp",     "LOG", 2, pshader_logp, 0, 0},
    {D3DSIO_CND,      "cnd",      GLNAME_REQUIRE_GLSL,   4, pshader_cnd,         D3DPS_VERSION(1,1), D3DPS_VERSION(1,4)},
    /* def is a special operation */
    {D3DSIO_DEF,      "def",      "undefined",   5, pshader_def,         0, 0},
    {D3DSIO_TEXREG2RGB,   "texreg2rgb",   GLNAME_REQUIRE_GLSL,   2, pshader_texreg2rgb,  D3DPS_VERSION(1,2), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXDP3TEX,    "texdp3tex",    GLNAME_REQUIRE_GLSL,   2, pshader_texdp3tex,   D3DPS_VERSION(1,2), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXM3x2DEPTH, "texm3x2depth", GLNAME_REQUIRE_GLSL,   2, pshader_texm3x2depth,D3DPS_VERSION(1,3), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXDP3,       "texdp3", GLNAME_REQUIRE_GLSL,  2, pshader_texdp3,     D3DPS_VERSION(1,2), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXM3x3,      "texm3x3", GLNAME_REQUIRE_GLSL, 2, pshader_texm3x3,    D3DPS_VERSION(1,2), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXDEPTH,     "texdepth", GLNAME_REQUIRE_GLSL,1, pshader_texdepth,   D3DPS_VERSION(1,4), D3DPS_VERSION(1,4)},
    {D3DSIO_CMP,      "cmp",      GLNAME_REQUIRE_GLSL,   4, pshader_cmp,     D3DPS_VERSION(1,1), D3DPS_VERSION(3,0)},
    {D3DSIO_BEM,      "bem",      GLNAME_REQUIRE_GLSL,   3, pshader_bem,     D3DPS_VERSION(1,4), D3DPS_VERSION(1,4)},
    /* TODO: dp2add can be made out of multiple instuctions */
    {D3DSIO_DP2ADD,   "dp2add",   GLNAME_REQUIRE_GLSL,   2, pshader_dp2add,  0, 0},
    {D3DSIO_DSX,      "dsx",      GLNAME_REQUIRE_GLSL,   2, pshader_dsx,     0, 0},
    {D3DSIO_DSY,      "dsy",      GLNAME_REQUIRE_GLSL,   2, pshader_dsy,     0, 0},
    {D3DSIO_TEXLDD,   "texldd",   GLNAME_REQUIRE_GLSL,   2, pshader_texldd,  0, 0},
    {D3DSIO_SETP,     "setp",     GLNAME_REQUIRE_GLSL,   2, pshader_setp,    0, 0},
    {D3DSIO_TEXLDL,   "texdl",    GLNAME_REQUIRE_GLSL,   2, pshader_texldl,  0, 0},
    {D3DSIO_PHASE,    "phase",    GLNAME_REQUIRE_GLSL,   0, pshader_nop,     0, 0},
    {0,               NULL,       NULL,   0, NULL,            0, 0}
};

inline static const SHADER_OPCODE* pshader_program_get_opcode(IWineD3DPixelShaderImpl *This, const DWORD code) {
    DWORD i = 0;
    DWORD version = This->baseShader.version;
    DWORD hex_version = D3DPS_VERSION(version/10, version%10);
    const SHADER_OPCODE *shader_ins = This->baseShader.shader_ins;

    /** TODO: use dichotomic search */
    while (NULL != shader_ins[i].name) {
        if (((code & D3DSI_OPCODE_MASK) == shader_ins[i].opcode) &&
            (((hex_version >= shader_ins[i].min_version) && (hex_version <= shader_ins[i].max_version)) ||
            ((shader_ins[i].min_version == 0) && (shader_ins[i].max_version == 0)))) {
            return &shader_ins[i];
        }
        ++i;
    }
    FIXME("Unsupported opcode %lx(%ld) masked %lx version %ld\n", code, code, code & D3DSI_OPCODE_MASK, version);
    return NULL;
}

inline static BOOL pshader_is_version_token(DWORD token) {
    return 0xFFFF0000 == (token & 0xFFFF0000);
}

inline static BOOL pshader_is_comment_token(DWORD token) {
    return D3DSIO_COMMENT == (token & D3DSI_OPCODE_MASK);
}


inline static void get_register_name(const DWORD param, char* regstr, char constants[WINED3D_PSHADER_MAX_CONSTANTS]) {
    static const char* rastout_reg_names[] = { "oC0", "oC1", "oC2", "oC3", "oDepth" };

    DWORD reg = param & D3DSP_REGNUM_MASK;
    DWORD regtype = ((param & D3DSP_REGTYPE_MASK) >> D3DSP_REGTYPE_SHIFT);

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

inline static void addline(unsigned int *lineNum, char *pgm, unsigned int *pgmLength, char *line) {
    int lineLen = strlen(line);
    if(lineLen + *pgmLength > PGMSIZE - 1 /* - 1 to allow a NULL at the end */) {
        ERR("The buffer allocated for the vertex program string pgmStr is too small at %d bytes, at least %d bytes in total are required.\n", PGMSIZE, lineLen + *pgmLength);
        return;
    } else {
        memcpy(pgm + *pgmLength, line, lineLen);
    }

    *pgmLength += lineLen;
    ++(*lineNum);
    TRACE("GL HW (%u, %u) : %s", *lineNum, *pgmLength, line);
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

    TRACE(" Register name %s\n", regstr);
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

inline static void pshader_program_get_registers_used(
      IWineD3DPixelShaderImpl *This,
      CONST DWORD* pToken, DWORD* tempsUsed, DWORD* texUsed) {
  
    if (pToken == NULL)
        return;

    *tempsUsed = 0;
    *texUsed = 0;

    while (D3DVS_END() != *pToken) {
        CONST SHADER_OPCODE* curOpcode;

        /* Skip version */
        if (pshader_is_version_token(*pToken)) {
             ++pToken;
             continue;

        /* Skip comments */
        } else if (pshader_is_comment_token(*pToken)) {
             DWORD comment_len = (*pToken & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;
             ++pToken;
             pToken += comment_len;
             continue;
        }

        /* Fetch opcode */       
        curOpcode = pshader_program_get_opcode(This, *pToken);
        ++pToken;

        /* Skip declarations (for now) */
        if (D3DSIO_DCL == curOpcode->opcode) {
            pToken += curOpcode->num_params;
            continue;

        /* Skip definitions (for now) */
        } else if (D3DSIO_DEF == curOpcode->opcode) {
            pToken += curOpcode->num_params;
            continue;

        /* Set texture registers, and temporary registers */
        } else {
            int i;
                        
            for (i = 0; i < curOpcode->num_params; ++i) {
                DWORD regtype = (((*pToken) & D3DSP_REGTYPE_MASK) >> D3DSP_REGTYPE_SHIFT);
                DWORD reg = (*pToken) & D3DSP_REGNUM_MASK;
                if (D3DSPR_TEXTURE == regtype) 
                    *texUsed |= (1 << reg);
                if (D3DSPR_TEMP == regtype)
                    *tempsUsed |= (1 << reg);
                ++pToken;
             }
        }
    }
}

/* NOTE: A description of how to parse tokens can be found at http://msdn.microsoft.com/library/default.asp?url=/library/en-us/graphics/hh/graphics/usermodedisplaydriver_shader_cc8e4e05-f5c3-4ec0-8853-8ce07c1551b2.xml.asp */
inline static VOID IWineD3DPixelShaderImpl_GenerateProgramArbHW(IWineD3DPixelShader *iface, CONST DWORD *pFunction) {
    IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *)iface;
    const DWORD *pToken = pFunction;
    const SHADER_OPCODE *curOpcode = NULL;
    const DWORD *pInstr;
    DWORD i;
    unsigned lineNum = 0; /* The line number of the generated program (for loging)*/
    char *pgmStr = NULL; /* A pointer to the program data generated by this function */
    char  tmpLine[255];
#if 0 /* TODO: loop register (just another address register ) */
    BOOL hasLoops = FALSE;
#endif

    BOOL saturate; /* clamp to 0.0 -> 1.0*/
    int row = 0; /* not sure, something to do with macros? */
    DWORD tcw[2];
    int version = 0; /* The version of the shader */

    /* Keep a running length for pgmStr so that we don't have to caculate strlen every time we concatanate */
    unsigned int pgmLength = 0;

    /* Keep bitmaps of used temporary and texture registers */
    DWORD tempsUsed, texUsed;

#if 0 /* FIXME: Use the buffer that is held by the device, this is ok since fixups will be skipped for software shaders
        it also requires entering a critical section but cuts down the runtime footprint of wined3d and any memory fragmentation that may occur... */
    if (This->device->fixupVertexBufferSize < PGMSIZE) {
        HeapFree(GetProcessHeap(), 0, This->fixupVertexBuffer);
        This->fixupVertexBuffer = HeapAlloc(GetProcessHeap() , 0, PGMSIZE);
        This->fixupVertexBufferSize = PGMSIZE;
        This->fixupVertexBuffer[0] = 0;
    }
    pgmStr = This->device->fixupVertexBuffer;
#else
    pgmStr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, PGMSIZE); /* 64kb should be enough */
#endif

    /* TODO: Think about using a first pass to work out what's required for the second pass. */
    for(i = 0; i < WINED3D_PSHADER_MAX_CONSTANTS; i++)
        This->constants[i] = 0;

    /* First pass: figure out which temporary and texture registers are used */
    pshader_program_get_registers_used(This, pToken, &tempsUsed, &texUsed);
    TRACE("Texture registers used: %#lx, Temp registers used %#lx\n", texUsed, tempsUsed);

    /* TODO: check register usage against GL/Directx limits, and fail if they're exceeded */

    /* Second pass, process opcodes */
    if (NULL != pToken) {
        while (D3DPS_END() != *pToken) {
#if 0 /* For pixel and vertex shader versions 2_0 and later, bits 24 through 27 specify the size in DWORDs of the instruction */
            if (version >= 2) {
                instructionSize = pToken & SIZEBITS >> 27;
            }
#endif
            if (pshader_is_version_token(*pToken)) { /** version */
                int numTemps;
                int numConstants;
                int numTex;

                /* Extract version *10 into integer value (ie. 1.0 == 10, 1.1==11 etc */
                version = (((*pToken >> 8) & 0x0F) * 10) + (*pToken & 0x0F);

                TRACE("found version token ps.%lu.%lu;\n", (*pToken >> 8) & 0x0F, (*pToken & 0x0F));

                /* Each release of pixel shaders has had different numbers of temp registers */
                switch (version) {
                case 10:
                case 11:
                case 12:
                case 13:numTemps=12;
                        numConstants=8;
                        numTex=4;
                        break;
                case 14: numTemps=12;
                        numConstants=8;
                        numTex=6;
                        break;
                case 20: numTemps=12;
                        numConstants=8;
                        numTex=8;
                        FIXME("No work done yet to support ps2.0 in hw\n");
                        break;
                case 30: numTemps=32;
                        numConstants=8;
                        numTex=0; 
                        FIXME("No work done yet to support ps3.0 in hw\n");
                        break;
                default:
                        numTemps=12;
                        numConstants=8;
                        numTex=8; 
                        FIXME("Unrecognized pixel shader version!\n");
                }

                /* FIXME: if jumps are used, use GLSL, else use ARB_fragment_program */
                strcpy(tmpLine, "!!ARBfp1.0\n");
                addline(&lineNum, pgmStr, &pgmLength, tmpLine);

                for(i = 0; i < numTex; i++) {
                    if (texUsed & (1 << i)) {
                        sprintf(tmpLine, "TEMP T%lu;\n", i);
                        addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                    }
                }

                for(i = 0; i < numTemps; i++) {
                    if (tempsUsed & (1 << i)) {
                        sprintf(tmpLine, "TEMP R%lu;\n", i);
                        addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                    }
                }

                sprintf(tmpLine, "TEMP TMP;\n");
                addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                sprintf(tmpLine, "TEMP TMP2;\n");
                addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                sprintf(tmpLine, "TEMP TA;\n");
                addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                sprintf(tmpLine, "TEMP TB;\n");
                addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                sprintf(tmpLine, "TEMP TC;\n");
                addline(&lineNum, pgmStr, &pgmLength, tmpLine);

                strcpy(tmpLine, "PARAM coefdiv = { 0.5, 0.25, 0.125, 0.0625 };\n");
                addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                strcpy(tmpLine, "PARAM coefmul = { 2, 4, 8, 16 };\n");
                addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                strcpy(tmpLine, "PARAM one = { 1.0, 1.0, 1.0, 1.0 };\n");
                addline(&lineNum, pgmStr, &pgmLength, tmpLine);

                for(i = 0; i < numTex; i++) {
                    if (texUsed & (1 << i)) {
                        sprintf(tmpLine, "MOV T%lu, fragment.texcoord[%lu];\n", i, i);
                        addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                    }
                }

                ++pToken;
                continue;
            }

            if (pshader_is_comment_token(*pToken)) { /** comment */
                DWORD comment_len = (*pToken & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;
                ++pToken;
                TRACE("#%s\n", (char*)pToken);
                pToken += comment_len;
                continue;
            }
/* here */
#if 0 /* Not sure what these are here for, they're not required for vshaders */
            code = *pToken;
#endif
            pInstr = pToken;
            curOpcode = pshader_program_get_opcode(This, *pToken);
            ++pToken;
            if (NULL == curOpcode) {
                /* unknown current opcode ... (shouldn't be any!) */
                while (*pToken & 0x80000000) { /* TODO: Think of a sensible name for 0x80000000 */
                    FIXME("unrecognized opcode: %08lx\n", *pToken);
                    ++pToken;
                }
            } else if (GLNAME_REQUIRE_GLSL == curOpcode->glname) {
                /* if the token isn't supported by this cross compiler then skip it and its parameters */
                FIXME("Token %s requires greater functionality than Fragment_Progarm_ARB supports\n", curOpcode->name);
                pToken += curOpcode->num_params;

            } else if (D3DSIO_DEF == curOpcode->opcode) {

                    /* Handle definitions here, they don't fit well with the
                     * other instructions below [for now ] */

                    DWORD reg = *pToken & D3DSP_REGNUM_MASK;

                    TRACE("Found opcode D3D:%s GL:%s, PARAMS:%d, \n",
                    curOpcode->name, curOpcode->glname, curOpcode->num_params);

                    sprintf(tmpLine, "PARAM C%lu = { %f, %f, %f, %f };\n", reg,
                              *((const float *)(pToken + 1)),
                              *((const float *)(pToken + 2)),
                              *((const float *)(pToken + 3)),
                              *((const float *)(pToken + 4)) );

                    addline(&lineNum, pgmStr, &pgmLength, tmpLine);

                    This->constants[reg] = 1;
                    pToken += 5;
                    continue;

            } else {
 
                /* Common processing: [inst] [dst]* [src]* */
                DWORD shift;
                char output_rname[256];
                char output_wmask[20];

                TRACE("Found opcode D3D:%s GL:%s, PARAMS:%d, \n",
                curOpcode->name, curOpcode->glname, curOpcode->num_params);

                saturate = FALSE;

                /* Build opcode for GL vertex_program */
                switch (curOpcode->opcode) {
                case D3DSIO_NOP:
                case D3DSIO_PHASE:
                    continue;
                case D3DSIO_MOV:
                case D3DSIO_CND:
                case D3DSIO_CMP:
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
                case D3DSIO_DST:
                case D3DSIO_FRC:
                case D3DSIO_EXPP:
                case D3DSIO_LOGP:
                case D3DSIO_EXP:
                case D3DSIO_LOG:
                case D3DSIO_LRP:
                case D3DSIO_TEXKILL:
                    TRACE("Appending glname %s to tmpLine\n", curOpcode->glname);
                    strcpy(tmpLine, curOpcode->glname);
                    break;
                case D3DSIO_TEX:
                {
                    char reg_dest[20];
                    char reg_coord[20];
                    char reg_coord_swz[20] = "";
                    DWORD reg_dest_code;
                    DWORD reg_sampler_code;

                    /* All versions have a destination register */
                    reg_dest_code = *pToken & D3DSP_REGNUM_MASK;
                    get_register_name(*pToken++, reg_dest, This->constants);              
                        
                    /* 1.0-1.3: Use destination register as coordinate source. No modifiers.
                       1.4: Use provided coordinate source register. _dw, _dz, swizzle allowed.
                       2.0+: Use provided coordinate source register. No modifiers.
                       3.0+: Use provided coordinate source register. Swizzle allowed */
                    if (version < 14)
                        strcpy(reg_coord, reg_dest);   
              
                    else if (version == 14) { 
                        if (gen_input_modifier_line(*pToken, 0, reg_coord, tmpLine, This->constants))
                            addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                        get_input_register_swizzle(*pToken, reg_coord_swz);
                        pToken++;
                    }
                    else if (version > 14 && version < 30) {
                        get_register_name(*pToken, reg_coord, This->constants);
                        pToken++;
                    }                        
                    else if (version >= 30) {
                        get_input_register_swizzle(*pToken, reg_coord_swz);
                        get_register_name(*pToken, reg_coord, This->constants);
                        pToken++;
                    }

                    /* 1.0-1.4: Use destination register number as texture code.
                       2.0+: Use provided sampler number as texure code. */                              
                    if (version < 20) 
                        reg_sampler_code = reg_dest_code;

                    else {
                        reg_sampler_code = *pToken & D3DSP_REGNUM_MASK;
                        pToken++;
                    }

                    sprintf(tmpLine, "TEX %s, %s%s, texture[%lu], 2D;\n",
                        reg_dest, reg_coord, reg_coord_swz, reg_sampler_code);
                    addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                    continue;
                }
                break;
                case D3DSIO_TEXCOORD:
                {
                    char tmp[20];
                    get_write_mask(*pToken, tmp);
                    if (version != 14) {
                        DWORD reg = *pToken & D3DSP_REGNUM_MASK;
                        sprintf(tmpLine, "MOV T%lu%s, fragment.texcoord[%lu];\n", reg, tmp, reg);
                        addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                        ++pToken;
                    } else {
                        DWORD reg1 = *pToken & D3DSP_REGNUM_MASK;
                        DWORD reg2 = *++pToken & D3DSP_REGNUM_MASK;
                        sprintf(tmpLine, "MOV R%lu%s, fragment.texcoord[%lu];\n", reg1, tmp, reg2);
                        addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                        ++pToken;
                    }
                    continue;
                }
                break;
                case D3DSIO_TEXM3x2PAD:
                {
                    DWORD reg = *pToken & D3DSP_REGNUM_MASK;
                    char buf[50];
                    if (gen_input_modifier_line(*++pToken, 0, buf, tmpLine, This->constants)) {
                        addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                    }
                    sprintf(tmpLine, "DP3 TMP.x, T%lu, %s;\n", reg, buf);
                    addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                    ++pToken;
                    continue;
                }
                break;
                case D3DSIO_TEXM3x2TEX:
                {
                    DWORD reg = *pToken & D3DSP_REGNUM_MASK;
                    char buf[50];
                    if (gen_input_modifier_line(*++pToken, 0, buf, tmpLine, This->constants)) {
                        addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                    }
                    sprintf(tmpLine, "DP3 TMP.y, T%lu, %s;\n", reg, buf);
                    addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                    sprintf(tmpLine, "TEX T%lu, TMP, texture[%lu], 2D;\n", reg, reg);
                    addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                    ++pToken;
                    continue;
                }
                break;
                case D3DSIO_TEXREG2AR:
                {
                    DWORD reg1 = *pToken & D3DSP_REGNUM_MASK;
                    DWORD reg2 = *++pToken & D3DSP_REGNUM_MASK;
                    sprintf(tmpLine, "MOV TMP.r, T%lu.a;\n", reg2);
                    addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                    sprintf(tmpLine, "MOV TMP.g, T%lu.r;\n", reg2);
                    addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                    sprintf(tmpLine, "TEX T%lu, TMP, texture[%lu], 2D;\n", reg1, reg1);
                    addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                    ++pToken;
                    continue;
                }
                break;
                case D3DSIO_TEXREG2GB:
                {
                    DWORD reg1 = *pToken & D3DSP_REGNUM_MASK;
                    DWORD reg2 = *++pToken & D3DSP_REGNUM_MASK;
                    sprintf(tmpLine, "MOV TMP.r, T%lu.g;\n", reg2);
                    addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                    sprintf(tmpLine, "MOV TMP.g, T%lu.b;\n", reg2);
                    addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                    sprintf(tmpLine, "TEX T%lu, TMP, texture[%lu], 2D;\n", reg1, reg1);
                    addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                    ++pToken;
                    continue;
                }
                break;
                case D3DSIO_TEXBEM:
                {
                    DWORD reg1 = *pToken & D3DSP_REGNUM_MASK;
                    DWORD reg2 = *++pToken & D3DSP_REGNUM_MASK;

                    /* FIXME: Should apply the BUMPMAPENV matrix */
                    sprintf(tmpLine, "ADD TMP.rg, fragment.texcoord[%lu], T%lu;\n", reg1, reg2);
                    addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                    sprintf(tmpLine, "TEX T%lu, TMP, texture[%lu], 2D;\n", reg1, reg1);
                    addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                    ++pToken;
                    continue;
                }
                break;
                case D3DSIO_TEXM3x3PAD:
                {
                    DWORD reg = *pToken & D3DSP_REGNUM_MASK;
                    char buf[50];
                    if (gen_input_modifier_line(*++pToken, 0, buf, tmpLine, This->constants)) {
                        addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                    }
                    sprintf(tmpLine, "DP3 TMP.%c, T%lu, %s;\n", 'x'+row, reg, buf);
                    addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                    tcw[row++] = reg;
                    ++pToken;
                    continue;
                }
                break;
                case D3DSIO_TEXM3x3TEX:
                {
                    DWORD reg = *pToken & D3DSP_REGNUM_MASK;
                    char buf[50];
                    if (gen_input_modifier_line(*++pToken, 0, buf, tmpLine, This->constants)) {
                        addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                    }

                    sprintf(tmpLine, "DP3 TMP.z, T%lu, %s;\n", reg, buf);
                    addline(&lineNum, pgmStr, &pgmLength, tmpLine);

                    /* Cubemap textures will be more used than 3D ones. */
                    sprintf(tmpLine, "TEX T%lu, TMP, texture[%lu], CUBE;\n", reg, reg);
                    addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                    row = 0;
                    ++pToken;
                    continue;
                }
                case D3DSIO_TEXM3x3VSPEC:
                {
                    DWORD reg = *pToken & D3DSP_REGNUM_MASK;
                    char buf[50];
                    if (gen_input_modifier_line(*++pToken, 0, buf, tmpLine, This->constants)) {
                        addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                    }
                    sprintf(tmpLine, "DP3 TMP.z, T%lu, %s;\n", reg, buf);
                    addline(&lineNum, pgmStr, &pgmLength, tmpLine);

                    /* Construct the eye-ray vector from w coordinates */
                    sprintf(tmpLine, "MOV TMP2.x, fragment.texcoord[%lu].w;\n", tcw[0]);
                    addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                    sprintf(tmpLine, "MOV TMP2.y, fragment.texcoord[%lu].w;\n", tcw[1]);
                    addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                    sprintf(tmpLine, "MOV TMP2.z, fragment.texcoord[%lu].w;\n", reg);
                    addline(&lineNum, pgmStr, &pgmLength, tmpLine);

                    /* Calculate reflection vector (Assume normal is normalized): RF = 2*(N.E)*N -E */
                    sprintf(tmpLine, "DP3 TMP.w, TMP, TMP2;\n");
                    addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                    sprintf(tmpLine, "MUL TMP, TMP.w, TMP;\n");
                    addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                    sprintf(tmpLine, "MAD TMP, coefmul.x, TMP, -TMP2;\n");
                    addline(&lineNum, pgmStr, &pgmLength, tmpLine);

                    /* Cubemap textures will be more used than 3D ones. */
                    sprintf(tmpLine, "TEX T%lu, TMP, texture[%lu], CUBE;\n", reg, reg);
                    addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                    row = 0;
                    ++pToken;
                    continue;
                }
                break;
                case D3DSIO_TEXM3x3SPEC:
                {
                    DWORD reg = *pToken & D3DSP_REGNUM_MASK;
                    DWORD reg3 = *(pToken + 2) & D3DSP_REGNUM_MASK;
                    char buf[50];
                    if (gen_input_modifier_line(*(pToken + 1), 0, buf, tmpLine, This->constants)) {
                        addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                    }
                    sprintf(tmpLine, "DP3 TMP.z, T%lu, %s;\n", reg, buf);
                    addline(&lineNum, pgmStr, &pgmLength, tmpLine);

                    /* Calculate reflection vector (Assume normal is normalized): RF = 2*(N.E)*N -E */
                    sprintf(tmpLine, "DP3 TMP.w, TMP, C[%lu];\n", reg3);
                    addline(&lineNum, pgmStr, &pgmLength, tmpLine);

                    sprintf(tmpLine, "MUL TMP, TMP.w, TMP;\n");
                    addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                    sprintf(tmpLine, "MAD TMP, coefmul.x, TMP, -C[%lu];\n", reg3);
                    addline(&lineNum, pgmStr, &pgmLength, tmpLine);

                    /* Cubemap textures will be more used than 3D ones. */
                    sprintf(tmpLine, "TEX T%lu, TMP, texture[%lu], CUBE;\n", reg, reg);
                    addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                    row = 0;
                    pToken += 3;
                    continue;
                }
                break;

                default:
                    if (curOpcode->glname == GLNAME_REQUIRE_GLSL) {
                        FIXME("Opcode %s requires Gl Shader languange 1.0\n", curOpcode->name);
                    } else {
                        FIXME("Can't handle opcode %s in hwShader\n", curOpcode->name);
                    }
                    pToken += curOpcode->num_params; 
                    continue;
                }

                /* Process modifiers */
                if (0 != (*pToken & D3DSP_DSTMOD_MASK)) {
                    DWORD mask = *pToken & D3DSP_DSTMOD_MASK;
                    switch (mask) {
                    case D3DSPDM_SATURATE: saturate = TRUE; break;
#if 0 /* as yet unhandled modifiers */
                    case D3DSPDM_CENTROID: centroid = TRUE; break;
                    case D3DSPDM_PP: partialpresision = TRUE; break;
#endif
                    default:
                        TRACE("_unhandled_modifier(0x%08lx)\n", mask);
                    }
                }
                shift = (*pToken & D3DSP_DSTSHIFT_MASK) >> D3DSP_DSTSHIFT_SHIFT;

                /* Generate input and output registers */
                if (curOpcode->num_params > 0) {
                    char regs[5][50];
                    char operands[4][100];
                    char swzstring[20];
                    char tmpOp[256];

                    /* Generate lines that handle input modifier computation */
                    for (i = 1; i < curOpcode->num_params; ++i) {
                        TRACE("(%p) : Param %ld token %lx\n", This, i, *(pToken + i));
                        if (gen_input_modifier_line(*(pToken + i), i - 1, regs[i - 1], tmpOp, This->constants)) {
                            addline(&lineNum, pgmStr, &pgmLength, tmpOp);
                        }
                    }

                    /* Handle output register */
                    get_register_name(*pToken, output_rname, This->constants);
                    strcpy(operands[0], output_rname);
                    get_write_mask(*pToken, output_wmask);
                    strcat(operands[0], output_wmask);

                    /* This function works because of side effects from  gen_input_modifier_line */
                    /* Handle input registers */
                    for (i = 1; i < curOpcode->num_params; ++i) {
                        TRACE("(%p) : Regs = %s\n", This, regs[i - 1]);
                        strcpy(operands[i], regs[i - 1]);
                        get_input_register_swizzle(*(pToken + i), swzstring);
                        strcat(operands[i], swzstring);
                    }

                    switch(curOpcode->opcode) {
                    case D3DSIO_CMP:
                        sprintf(tmpLine, "CMP%s %s, %s, %s, %s;\n", (saturate ? "_SAT" : ""), operands[0], operands[1], operands[3], operands[2]);
                    break;
                    case D3DSIO_CND:
                        sprintf(tmpLine, "ADD TMP, -%s, coefdiv.x;", operands[1]);
                        addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                        sprintf(tmpLine, "CMP%s %s, TMP, %s, %s;\n", (saturate ? "_SAT" : ""), operands[0], operands[2], operands[3]);
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
                    addline(&lineNum, pgmStr, &pgmLength, tmpLine);

                    /* A shift requires another line. */
                    if (shift != 0) {
                        gen_output_modifier_line(saturate, output_wmask, shift, output_rname, tmpLine);
                        addline(&lineNum, pgmStr, &pgmLength, tmpLine);
                    }
                    pToken += curOpcode->num_params;
                }
            }
        }
        /* TODO: What about result.depth? */
        strcpy(tmpLine, "MOV result.color, R0;\n");
        addline(&lineNum, pgmStr, &pgmLength, tmpLine);

        strcpy(tmpLine, "END\n");
        addline(&lineNum, pgmStr, &pgmLength, tmpLine);
    }

    /* finally null terminate the pgmStr*/
    pgmStr[pgmLength] = 0;
    if (GL_SUPPORT(ARB_VERTEX_PROGRAM)) {
        /*  Create the hw shader */

        /* pgmStr sometimes gets too long for a normal TRACE */
        TRACE("Generated program:\n");
        if (TRACE_ON(d3d_shader)) {
            fprintf(stderr, "%s\n", pgmStr);
        }

        /* TODO: change to resource.glObjectHandel or something like that */
        GL_EXTCALL(glGenProgramsARB(1, &This->baseShader.prgId));

        TRACE("Creating a hw pixel shader, prg=%d\n", This->baseShader.prgId);
        GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, This->baseShader.prgId));

        TRACE("Created hw pixel shader, prg=%d\n", This->baseShader.prgId);
        /* Create the program and check for errors */
        GL_EXTCALL(glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, strlen(pgmStr), pgmStr));
        if (glGetError() == GL_INVALID_OPERATION) {
            GLint errPos;
            glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errPos);
            FIXME("HW PixelShader Error at position %d: %s\n",
                  errPos, debugstr_a((const char *)glGetString(GL_PROGRAM_ERROR_STRING_ARB)));
            This->baseShader.prgId = -1;
        }
    }
#if 1 /* if were using the data buffer of device then we don't need to free it */
    HeapFree(GetProcessHeap(), 0, pgmStr);
#endif
}

inline static void pshader_program_dump_ins_modifiers(const DWORD output) {

    DWORD shift = (output & D3DSP_DSTSHIFT_MASK) >> D3DSP_DSTSHIFT_SHIFT;
    DWORD mmask = output & D3DSP_DSTMOD_MASK;

    switch (shift) {
        case 0: break;
        case 13: TRACE("_d8"); break;
        case 14: TRACE("_d4"); break;
        case 15: TRACE("_d2"); break;
        case 1: TRACE("_x2"); break;
        case 2: TRACE("_x4"); break;
        case 3: TRACE("_x8"); break;
        default: TRACE("_unhandled_shift(%ld)", shift); break;
    }

    switch(mmask) {
        case D3DSPDM_NONE: break;
        case D3DSPDM_SATURATE: TRACE("_sat"); break;
        case D3DSPDM_PARTIALPRECISION: TRACE("_pp"); break;
        case D3DSPDM_MSAMPCENTROID: TRACE("_centroid"); break;
        default: TRACE("_unhandled_modifier(%#lx)", mmask); break;
    } 
}

inline static void pshader_program_dump_ps_param(const DWORD param, int input) {
  static const char* rastout_reg_names[] = { "oC0", "oC1", "oC2", "oC3", "oDepth" };
  static const char swizzle_reg_chars[] = "rgba";

   /* the unknown mask is for bits not yet accounted for by any other mask... */
#define UNKNOWN_MASK 0xC000

   /* for registeres about 7 we have to add on bits 11 and 12 to get the correct register */
#define EXTENDED_REG 0x1800

  DWORD reg = param & D3DSP_REGNUM_MASK;
  DWORD regtype = ((param & D3DSP_REGTYPE_MASK) >> D3DSP_REGTYPE_SHIFT) | ((param & EXTENDED_REG) >> 8);

  if (input) {
    if ( ((param & D3DSP_SRCMOD_MASK) == D3DSPSM_NEG) ||
         ((param & D3DSP_SRCMOD_MASK) == D3DSPSM_BIASNEG) ||
         ((param & D3DSP_SRCMOD_MASK) == D3DSPSM_SIGNNEG) ||
         ((param & D3DSP_SRCMOD_MASK) == D3DSPSM_X2NEG) )
      TRACE("-");
    else if ((param & D3DSP_SRCMOD_MASK) == D3DSPSM_COMP)
      TRACE("1-");
  }

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

  case D3DSPR_TEXTURE: /* case D3DSPR_ADDR: */
    TRACE("t%lu", reg);
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
    TRACE("unhandled_rtype(%lx)", regtype);
    break;
  }

    if (!input) {
        /* operand output (for modifiers and shift, see dump_ins_modifiers) */

        if ((param & D3DSP_WRITEMASK_ALL) != D3DSP_WRITEMASK_ALL) {
            TRACE(".");
            if (param & D3DSP_WRITEMASK_0) TRACE(".r");
            if (param & D3DSP_WRITEMASK_1) TRACE(".g");
            if (param & D3DSP_WRITEMASK_2) TRACE(".b");
            if (param & D3DSP_WRITEMASK_3) TRACE(".a");
        }
    } else {
        /** operand input */
        DWORD swizzle = (param & D3DSP_SWIZZLE_MASK) >> D3DSP_SWIZZLE_SHIFT;
        DWORD swizzle_r = swizzle & 0x03;
        DWORD swizzle_g = (swizzle >> 2) & 0x03;
        DWORD swizzle_b = (swizzle >> 4) & 0x03;
        DWORD swizzle_a = (swizzle >> 6) & 0x03;

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
        *  RRGGBBAA
        */
        if ((D3DVS_NOSWIZZLE >> D3DVS_SWIZZLE_SHIFT) != swizzle) { /* ! D3DVS_NOSWIZZLE == 0xE4 << D3DVS_SWIZZLE_SHIFT */
            if (swizzle_r == swizzle_g &&
                swizzle_r == swizzle_b &&
                swizzle_r == swizzle_a) {
                    TRACE(".%c", swizzle_reg_chars[swizzle_r]);
            } else {
                TRACE(".%c%c%c%c",
                swizzle_reg_chars[swizzle_r],
                swizzle_reg_chars[swizzle_g],
                swizzle_reg_chars[swizzle_b],
                swizzle_reg_chars[swizzle_a]);
            }
        }
    }
}

inline static void pshader_program_dump_decl_usage(
    IWineD3DPixelShaderImpl *This, DWORD decl, DWORD param) {

    DWORD regtype = ((param & D3DSP_REGTYPE_MASK) >> D3DSP_REGTYPE_SHIFT) | 
                    ((param & D3DSP_REGTYPE_MASK2) >> D3DSP_REGTYPE_SHIFT2);

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
                TRACE("%s%ld ", "specular", (idx - 1));
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

HRESULT WINAPI IWineD3DPixelShaderImpl_SetFunction(IWineD3DPixelShader *iface, CONST DWORD *pFunction) {
    IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *)iface;
    const DWORD* pToken = pFunction;
    const SHADER_OPCODE *curOpcode = NULL;
    DWORD len = 0;
    DWORD i;
    TRACE("(%p) : Parsing programme\n", This);

    if (NULL != pToken) {
        while (D3DPS_END() != *pToken) {
            if (pshader_is_version_token(*pToken)) { /** version */
                This->baseShader.version = (((*pToken >> 8) & 0x0F) * 10) + (*pToken & 0x0F);
                TRACE("ps_%lu_%lu\n", (*pToken >> 8) & 0x0F, (*pToken & 0x0F));
                ++pToken;
                ++len;
                continue;
            }
            if (pshader_is_comment_token(*pToken)) { /** comment */
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
            curOpcode = pshader_program_get_opcode(This, *pToken);
            ++pToken;
            ++len;
            if (NULL == curOpcode) {

                /* TODO: Think of a good name for 0x80000000 and replace it with a constant */
                while (*pToken & 0x80000000) {

                    /* unknown current opcode ... */
                    TRACE("unrecognized opcode: %08lx", *pToken);
                    ++pToken;
                    ++len;
                    TRACE("\n");
                }

            } else {
                if (curOpcode->opcode == D3DSIO_DCL) {
                    pshader_program_dump_decl_usage(This, *pToken, *(pToken + 1));
                    ++pToken;
                    ++len;
                    pshader_program_dump_ps_param(*pToken, 0);
                    ++pToken;
                    ++len;
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
                    TRACE("%s", curOpcode->name);
                    if (curOpcode->num_params > 0) {
                        pshader_program_dump_ins_modifiers(*pToken);
                        TRACE(" ");
                        pshader_program_dump_ps_param(*pToken, 0);
                        ++pToken;
                        ++len;
                        for (i = 1; i < curOpcode->num_params; ++i) {
                            TRACE(", ");
                            pshader_program_dump_ps_param(*pToken, 1);
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

    /* Generate HW shader in needed */
    if (NULL != pFunction  && wined3d_settings.vs_mode == VS_HW) {
        TRACE("(%p) : Generating hardware program\n", This);
#if 1
        IWineD3DPixelShaderImpl_GenerateProgramArbHW(iface, pFunction);
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
