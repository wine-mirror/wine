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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
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
static HRESULT  WINAPI IWineD3DPixelShaderImpl_QueryInterface(IWineD3DPixelShader *iface, REFIID riid, LPVOID *ppobj)
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

static ULONG  WINAPI IWineD3DPixelShaderImpl_AddRef(IWineD3DPixelShader *iface) {
    IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *)iface;
    TRACE("(%p) : AddRef increasing from %ld\n", This, This->ref);
    return InterlockedIncrement(&This->ref);
}

static ULONG  WINAPI IWineD3DPixelShaderImpl_Release(IWineD3DPixelShader *iface) {
    IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *)iface;
    ULONG ref;
    TRACE("(%p) : Releasing from %ld\n", This, This->ref);
    ref = InterlockedDecrement(&This->ref);
    if (ref == 0) {
        if (This->baseShader.shader_mode == SHADER_GLSL && This->baseShader.prgId != 0) {
            /* If this shader is still attached to a program, GL will perform a lazy delete */
            TRACE("Deleting shader object %u\n", This->baseShader.prgId);
            GL_EXTCALL(glDeleteObjectARB(This->baseShader.prgId));
            checkGLcall("glDeleteObjectARB");
        }
        shader_delete_constant_list(&This->baseShader.constantsF);
        shader_delete_constant_list(&This->baseShader.constantsB);
        shader_delete_constant_list(&This->baseShader.constantsI);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* *******************************************
   IWineD3DPixelShader IWineD3DPixelShader parts follow
   ******************************************* */

static HRESULT  WINAPI IWineD3DPixelShaderImpl_GetParent(IWineD3DPixelShader *iface, IUnknown** parent){
    IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *)iface;

    *parent = This->parent;
    IUnknown_AddRef(*parent);
    TRACE("(%p) : returning %p\n", This, *parent);
    return WINED3D_OK;
}

static HRESULT  WINAPI IWineD3DPixelShaderImpl_GetDevice(IWineD3DPixelShader* iface, IWineD3DDevice **pDevice){
    IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *)iface;
    IWineD3DDevice_AddRef((IWineD3DDevice *)This->wineD3DDevice);
    *pDevice = (IWineD3DDevice *)This->wineD3DDevice;
    TRACE("(%p) returning %p\n", This, *pDevice);
    return WINED3D_OK;
}


static HRESULT  WINAPI IWineD3DPixelShaderImpl_GetFunction(IWineD3DPixelShader* impl, VOID* pData, UINT* pSizeOfData) {
  IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *)impl;
  TRACE("(%p) : pData(%p), pSizeOfData(%p)\n", This, pData, pSizeOfData);

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

static void pshader_add(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
    d->x = s0->x + s1->x;
    d->y = s0->y + s1->y;
    d->z = s0->z + s1->z;
    d->w = s0->w + s1->w;
    PSTRACE(("executing add: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
                s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

static void pshader_dp3(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
    d->x = d->y = d->z = d->w = s0->x * s1->x + s0->y * s1->y + s0->z * s1->z;
    PSTRACE(("executing dp3: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
                s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

static void pshader_dp4(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
    d->x = d->y = d->z = d->w = s0->x * s1->x + s0->y * s1->y + s0->z * s1->z + s0->w * s1->w;
    PSTRACE(("executing dp4: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
        s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

static void pshader_dst(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
    d->x = 1.0f;
    d->y = s0->y * s1->y;
    d->z = s0->z;
    d->w = s1->w;
    PSTRACE(("executing dst: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
        s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

static void pshader_expp(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
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

static void pshader_logp(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    float tmp_f = fabsf(s0->w);
    d->x = d->y = d->z = d->w = (0.0f != tmp_f) ? logf(tmp_f) / logf(2.0f) : -HUGE_VAL;
    PSTRACE(("executing logp: s0=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
                s0->x, s0->y, s0->z, s0->w, d->x, d->y, d->z, d->w));
}

static void pshader_mad(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1, WINED3DSHADERVECTOR* s2) {
    d->x = s0->x * s1->x + s2->x;
    d->y = s0->y * s1->y + s2->y;
    d->z = s0->z * s1->z + s2->z;
    d->w = s0->w * s1->w + s2->w;
    PSTRACE(("executing mad: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) s2=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
        s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, s2->x, s2->y, s2->z, s2->w, d->x, d->y, d->z, d->w));
}

static void pshader_max(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
    d->x = (s0->x >= s1->x) ? s0->x : s1->x;
    d->y = (s0->y >= s1->y) ? s0->y : s1->y;
    d->z = (s0->z >= s1->z) ? s0->z : s1->z;
    d->w = (s0->w >= s1->w) ? s0->w : s1->w;
    PSTRACE(("executing max: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
        s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

static void pshader_min(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
    d->x = (s0->x < s1->x) ? s0->x : s1->x;
    d->y = (s0->y < s1->y) ? s0->y : s1->y;
    d->z = (s0->z < s1->z) ? s0->z : s1->z;
    d->w = (s0->w < s1->w) ? s0->w : s1->w;
    PSTRACE(("executing min: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
        s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

static void pshader_mov(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    d->x = s0->x;
    d->y = s0->y;
    d->z = s0->z;
    d->w = s0->w;
    PSTRACE(("executing mov: s0=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
        s0->x, s0->y, s0->z, s0->w, d->x, d->y, d->z, d->w));
}

static void pshader_mul(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
    d->x = s0->x * s1->x;
    d->y = s0->y * s1->y;
    d->z = s0->z * s1->z;
    d->w = s0->w * s1->w;
    PSTRACE(("executing mul: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
        s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

static void pshader_nop(void) {
    /* NOPPPP ahhh too easy ;) */
    PSTRACE(("executing nop\n"));
}

static void pshader_rcp(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    d->x = d->y = d->z = d->w = (0.0f == s0->w) ? HUGE_VAL : 1.0f / s0->w;
    PSTRACE(("executing rcp: s0=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
        s0->x, s0->y, s0->z, s0->w, d->x, d->y, d->z, d->w));
}

static void pshader_rsq(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    float tmp_f = fabsf(s0->w);
    d->x = d->y = d->z = d->w = (0.0f == tmp_f) ? HUGE_VAL : ((1.0f != tmp_f) ? 1.0f / sqrtf(tmp_f) : 1.0f);
    PSTRACE(("executing rsq: s0=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
        s0->x, s0->y, s0->z, s0->w, d->x, d->y, d->z, d->w));
}

static void pshader_sge(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
    d->x = (s0->x >= s1->x) ? 1.0f : 0.0f;
    d->y = (s0->y >= s1->y) ? 1.0f : 0.0f;
    d->z = (s0->z >= s1->z) ? 1.0f : 0.0f;
    d->w = (s0->w >= s1->w) ? 1.0f : 0.0f;
    PSTRACE(("executing sge: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
        s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

static void pshader_slt(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
    d->x = (s0->x < s1->x) ? 1.0f : 0.0f;
    d->y = (s0->y < s1->y) ? 1.0f : 0.0f;
    d->z = (s0->z < s1->z) ? 1.0f : 0.0f;
    d->w = (s0->w < s1->w) ? 1.0f : 0.0f;
    PSTRACE(("executing slt: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
        s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

static void pshader_sub(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
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

static void pshader_exp(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    d->x = d->y = d->z = d->w = powf(2.0f, s0->w);
    PSTRACE(("executing exp: s0=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
        s0->x, s0->y, s0->z, s0->w, d->x, d->y, d->z, d->w));
}

static void pshader_log(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    float tmp_f = fabsf(s0->w);
    d->x = d->y = d->z = d->w = (0.0f != tmp_f) ? logf(tmp_f) / logf(2.0f) : -HUGE_VAL;
    PSTRACE(("executing log: s0=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
        s0->x, s0->y, s0->z, s0->w, d->x, d->y, d->z, d->w));
}

static void pshader_frc(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
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

static void pshader_m4x4(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, /*WINED3DSHADERVECTOR* mat1*/ D3DMATRIX44 mat) {
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

static void pshader_m4x3(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, D3DMATRIX34 mat) {
    d->x = mat[0][0] * s0->x + mat[0][1] * s0->y + mat[0][2] * s0->z + mat[0][3] * s0->w;
    d->y = mat[1][0] * s0->x + mat[1][1] * s0->y + mat[1][2] * s0->z + mat[1][3] * s0->w;
    d->z = mat[2][0] * s0->x + mat[2][1] * s0->y + mat[2][2] * s0->z + mat[2][3] * s0->w;
    d->w = 1.0f;
    PSTRACE(("executing m4x3(1): mat=(%f, %f, %f, %f)    s0=(%f)     d=(%f) \n", mat[0][0], mat[0][1], mat[0][2], mat[0][3], s0->x, d->x));
    PSTRACE(("executing m4x3(2): mat=(%f, %f, %f, %f)       (%f)       (%f) \n", mat[1][0], mat[1][1], mat[1][2], mat[1][3], s0->y, d->y));
    PSTRACE(("executing m4x3(3): mat=(%f, %f, %f, %f) X     (%f)  =    (%f) \n", mat[2][0], mat[2][1], mat[2][2], mat[2][3], s0->z, d->z));
    PSTRACE(("executing m4x3(4):                            (%f)       (%f) \n", s0->w, d->w));
}

static void pshader_m3x4(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, D3DMATRIX43 mat) {
    d->x = mat[0][0] * s0->x + mat[0][1] * s0->y + mat[0][2] * s0->z;
    d->y = mat[1][0] * s0->x + mat[1][1] * s0->y + mat[1][2] * s0->z;
    d->z = mat[2][0] * s0->x + mat[2][1] * s0->y + mat[2][2] * s0->z;
    d->w = mat[3][0] * s0->x + mat[3][1] * s0->y + mat[3][2] * s0->z;
    PSTRACE(("executing m3x4(1): mat=(%f, %f, %f)    s0=(%f)     d=(%f) \n", mat[0][0], mat[0][1], mat[0][2], s0->x, d->x));
    PSTRACE(("executing m3x4(2): mat=(%f, %f, %f)       (%f)       (%f) \n", mat[1][0], mat[1][1], mat[1][2], s0->y, d->y));
    PSTRACE(("executing m3x4(3): mat=(%f, %f, %f) X     (%f)  =    (%f) \n", mat[2][0], mat[2][1], mat[2][2], s0->z, d->z));
    PSTRACE(("executing m3x4(4): mat=(%f, %f, %f)       (%f)       (%f) \n", mat[3][0], mat[3][1], mat[3][2], s0->w, d->w));
}

static void pshader_m3x3(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, D3DMATRIX33 mat) {
    d->x = mat[0][0] * s0->x + mat[0][1] * s0->y + mat[0][2] * s0->z;
    d->y = mat[1][0] * s0->x + mat[1][1] * s0->y + mat[1][2] * s0->z;
    d->z = mat[2][0] * s0->x + mat[2][1] * s0->y + mat[2][2] * s0->z;
    d->w = 1.0f;
    PSTRACE(("executing m3x3(1): mat=(%f, %f, %f)    s0=(%f)     d=(%f) \n", mat[0][0], mat[0][1], mat[0][2], s0->x, d->x));
    PSTRACE(("executing m3x3(2): mat=(%f, %f, %f)       (%f)       (%f) \n", mat[1][0], mat[1][1], mat[1][2], s0->y, d->y));
    PSTRACE(("executing m3x3(3): mat=(%f, %f, %f) X     (%f)  =    (%f) \n", mat[2][0], mat[2][1], mat[2][2], s0->z, d->z));
    PSTRACE(("executing m3x3(4):                                       (%f) \n", d->w));
}

static void pshader_m3x2(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, D3DMATRIX23 mat) {
    FIXME("check\n");
    d->x = mat[0][0] * s0->x + mat[0][1] * s0->y + mat[0][2] * s0->z;
    d->y = mat[1][0] * s0->x + mat[1][1] * s0->y + mat[1][2] * s0->z;
    d->z = 0.0f;
    d->w = 1.0f;
}

/**
 * Version 2.0 specific
 */
static void pshader_lrp(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1, WINED3DSHADERVECTOR* s2) {
    d->x = s0->x * (s1->x - s2->x) + s2->x;
    d->y = s0->y * (s1->y - s2->y) + s2->y;
    d->z = s0->z * (s1->z - s2->z) + s2->z;
    d->w = s0->w * (s1->w - s2->w) + s2->w;
}

static void pshader_crs(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
    d->x = s0->y * s1->z - s0->z * s1->y;
    d->y = s0->z * s1->x - s0->x * s1->z;
    d->z = s0->x * s1->y - s0->y * s1->x;
    d->w = 0.9f; /* w is undefined, so set it to something safeish */

    PSTRACE(("executing crs: s0=(%f, %f, %f, %f) s1=(%f, %f, %f, %f) => d=(%f, %f, %f, %f)\n",
                s0->x, s0->y, s0->z, s0->w, s1->x, s1->y, s1->z, s1->w, d->x, d->y, d->z, d->w));
}

static void pshader_abs(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    d->x = fabsf(s0->x);
    d->y = fabsf(s0->y);
    d->z = fabsf(s0->z);
    d->w = fabsf(s0->w);
    PSTRACE(("executing abs: s0=(%f, %f, %f, %f)  => d=(%f, %f, %f, %f)\n",
                s0->x, s0->y, s0->z, s0->w, d->x, d->y, d->z, d->w));
}

    /* Stubs */
static void pshader_texcoord(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}

static void pshader_texkill(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}

static void pshader_tex(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}
static void pshader_texld(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

static void pshader_texbem(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

static void pshader_texbeml(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

static void pshader_texreg2ar(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

static void pshader_texreg2gb(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

static void pshader_texm3x2pad(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

static void pshader_texm3x2tex(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

static void pshader_texm3x3tex(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
    FIXME(" : Stub\n");
}

static void pshader_texm3x3pad(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

static void pshader_texm3x3diff(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

static void pshader_texm3x3spec(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
    FIXME(" : Stub\n");
}

static void pshader_texm3x3vspec(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

static void pshader_cnd(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1, WINED3DSHADERVECTOR* s2) {
    FIXME(" : Stub\n");
}

/* Def is C[n] = {n.nf, n.nf, n.nf, n.nf} */
static void pshader_def(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1, WINED3DSHADERVECTOR* s2, WINED3DSHADERVECTOR* s3) {
    FIXME(" : Stub\n");
}

static void pshader_texreg2rgb(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

static void pshader_texdp3tex(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

static void pshader_texm3x2depth(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

static void pshader_texdp3(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

static void pshader_texm3x3(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

static void pshader_texdepth(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}

static void pshader_cmp(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1, WINED3DSHADERVECTOR* s2) {
    FIXME(" : Stub\n");
}

static void pshader_bem(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
    FIXME(" : Stub\n");
}

static void pshader_call(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}

static void pshader_callnz(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

static void pshader_loop(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

static void pshader_ret(void) {
    FIXME(" : Stub\n");
}

static void pshader_endloop(void) {
    FIXME(" : Stub\n");
}

static void pshader_dcl(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

static void pshader_pow(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
    FIXME(" : Stub\n");
}

static void pshader_nrm(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

static void pshader_sincos3(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

static void pshader_sincos2(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1, WINED3DSHADERVECTOR* s2) {
     FIXME(" : Stub\n");
}

static void pshader_rep(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}

static void pshader_endrep(void) {
    FIXME(" : Stub\n");
}

static void pshader_if(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}

static void pshader_ifc(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

static void pshader_else(void) {
    FIXME(" : Stub\n");
}

static void pshader_label(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}

static void pshader_endif(void) {
    FIXME(" : Stub\n");
}

static void pshader_break(void) {
    FIXME(" : Stub\n");
}

static void pshader_breakc(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0) {
    FIXME(" : Stub\n");
}

static void pshader_breakp(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}

static void pshader_defb(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}

static void pshader_defi(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1, WINED3DSHADERVECTOR* s2, WINED3DSHADERVECTOR* s3) {
    FIXME(" : Stub\n");
}

static void pshader_dp2add(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1, WINED3DSHADERVECTOR* s2) {
    FIXME(" : Stub\n");
}

static void pshader_dsx(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}

static void pshader_dsy(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}

static void pshader_texldd(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1, WINED3DSHADERVECTOR* s2, WINED3DSHADERVECTOR* s3) {
    FIXME(" : Stub\n");
}

static void pshader_setp(WINED3DSHADERVECTOR* d, WINED3DSHADERVECTOR* s0, WINED3DSHADERVECTOR* s1) {
    FIXME(" : Stub\n");
}

static void pshader_texldl(WINED3DSHADERVECTOR* d) {
    FIXME(" : Stub\n");
}

/**
 * log, exp, frc, m*x* seems to be macros ins ... to see
 */
CONST SHADER_OPCODE IWineD3DPixelShaderImpl_shader_ins[] = {

    /* Arithmethic */
    {D3DSIO_NOP,  "nop", "NOP", 0, 0, pshader_nop, pshader_hw_map2gl, NULL, 0, 0},
    {D3DSIO_MOV,  "mov", "MOV", 1, 2, pshader_mov, pshader_hw_map2gl, shader_glsl_mov, 0, 0},
    {D3DSIO_ADD,  "add", "ADD", 1, 3, pshader_add, pshader_hw_map2gl, shader_glsl_arith, 0, 0},
    {D3DSIO_SUB,  "sub", "SUB", 1, 3, pshader_sub, pshader_hw_map2gl, shader_glsl_arith, 0, 0},
    {D3DSIO_MAD,  "mad", "MAD", 1, 4, pshader_mad, pshader_hw_map2gl, shader_glsl_mad, 0, 0},
    {D3DSIO_MUL,  "mul", "MUL", 1, 3, pshader_mul, pshader_hw_map2gl, shader_glsl_arith, 0, 0},
    {D3DSIO_RCP,  "rcp", "RCP",  1, 2, pshader_rcp, pshader_hw_map2gl, shader_glsl_rcp, 0, 0},
    {D3DSIO_RSQ,  "rsq",  "RSQ", 1, 2, pshader_rsq, pshader_hw_map2gl, shader_glsl_map2gl, 0, 0},
    {D3DSIO_DP3,  "dp3",  "DP3", 1, 3, pshader_dp3, pshader_hw_map2gl, shader_glsl_dot, 0, 0},
    {D3DSIO_DP4,  "dp4",  "DP4", 1, 3, pshader_dp4, pshader_hw_map2gl, shader_glsl_dot, 0, 0},
    {D3DSIO_MIN,  "min",  "MIN", 1, 3, pshader_min, pshader_hw_map2gl, shader_glsl_map2gl, 0, 0},
    {D3DSIO_MAX,  "max",  "MAX", 1, 3, pshader_max, pshader_hw_map2gl, shader_glsl_map2gl, 0, 0},
    {D3DSIO_SLT,  "slt",  "SLT", 1, 3, pshader_slt, pshader_hw_map2gl, shader_glsl_compare, 0, 0},
    {D3DSIO_SGE,  "sge",  "SGE", 1, 3, pshader_sge, pshader_hw_map2gl, shader_glsl_compare, 0, 0},
    {D3DSIO_ABS,  "abs",  "ABS", 1, 2, pshader_abs, pshader_hw_map2gl, shader_glsl_map2gl, 0, 0},
    {D3DSIO_EXP,  "exp",  "EX2", 1, 2, pshader_exp, pshader_hw_map2gl, shader_glsl_map2gl, 0, 0},
    {D3DSIO_LOG,  "log",  "LG2", 1, 2, pshader_log, pshader_hw_map2gl, shader_glsl_map2gl, 0, 0},
    {D3DSIO_EXPP, "expp", "EXP", 1, 2, pshader_expp, pshader_hw_map2gl, shader_glsl_map2gl, 0, 0},
    {D3DSIO_LOGP, "logp", "LOG", 1, 2, pshader_logp, pshader_hw_map2gl, shader_glsl_map2gl, 0, 0},
    {D3DSIO_DST,  "dst",  "DST", 1, 3, pshader_dst, pshader_hw_map2gl, shader_glsl_dst, 0, 0},
    {D3DSIO_LRP,  "lrp",  "LRP", 1, 4, pshader_lrp, pshader_hw_map2gl, shader_glsl_lrp, 0, 0},
    {D3DSIO_FRC,  "frc",  "FRC", 1, 2, pshader_frc, pshader_hw_map2gl, shader_glsl_map2gl, 0, 0},
    {D3DSIO_CND,  "cnd",  NULL, 1, 4, pshader_cnd, pshader_hw_cnd, shader_glsl_cnd, D3DPS_VERSION(1,1), D3DPS_VERSION(1,4)},
    {D3DSIO_CMP,  "cmp",  NULL, 1, 4, pshader_cmp, pshader_hw_cmp, shader_glsl_cmp, D3DPS_VERSION(1,2), D3DPS_VERSION(3,0)},
    {D3DSIO_POW,  "pow",  "POW", 1, 3, pshader_pow,  NULL, shader_glsl_map2gl, 0, 0},
    {D3DSIO_CRS,  "crs",  "XPS", 1, 3, pshader_crs,  NULL, shader_glsl_map2gl, 0, 0},
    /* TODO: xyz normalise can be performed as VS_ARB using one temporary register,
        DP3 tmp , vec, vec;
        RSQ tmp, tmp.x;
        MUL vec.xyz, vec, tmp;
    but I think this is better because it accounts for w properly.
        DP3 tmp , vec, vec;
        RSQ tmp, tmp.x;
        MUL vec, vec, tmp;

    */
    {D3DSIO_NRM,      "nrm",      NULL, 1, 2, pshader_nrm,     NULL, shader_glsl_map2gl, 0, 0},
    {D3DSIO_SINCOS,   "sincos",   NULL, 1, 4, pshader_sincos2, NULL, shader_glsl_sincos, D3DPS_VERSION(2,0), D3DPS_VERSION(2,0)},
    {D3DSIO_SINCOS,   "sincos",   NULL, 1, 2, pshader_sincos3, NULL, shader_glsl_sincos, D3DPS_VERSION(3,0), -1},
    /* TODO: dp2add can be made out of multiple instuctions */
    {D3DSIO_DP2ADD,   "dp2add",   GLNAME_REQUIRE_GLSL,  1, 4, pshader_dp2add,  NULL, pshader_glsl_dp2add, D3DPS_VERSION(2,0), -1},

    /* Matrix */
    {D3DSIO_M4x4, "m4x4", "undefined", 1, 3, pshader_m4x4, NULL, shader_glsl_mnxn, 0, 0},
    {D3DSIO_M4x3, "m4x3", "undefined", 1, 3, pshader_m4x3, NULL, shader_glsl_mnxn, 0, 0},
    {D3DSIO_M3x4, "m3x4", "undefined", 1, 3, pshader_m3x4, NULL, shader_glsl_mnxn, 0, 0},
    {D3DSIO_M3x3, "m3x3", "undefined", 1, 3, pshader_m3x3, NULL, shader_glsl_mnxn, 0, 0},
    {D3DSIO_M3x2, "m3x2", "undefined", 1, 3, pshader_m3x2, NULL, shader_glsl_mnxn, 0, 0},

    /* Register declarations */
    {D3DSIO_DCL,      "dcl",      NULL, 0, 2, pshader_dcl,     NULL, NULL, 0, 0},

    /* Flow control - requires GLSL or software shaders */
    {D3DSIO_REP ,     "rep",      GLNAME_REQUIRE_GLSL, 0, 1, pshader_rep,     NULL, NULL, 0, 0},
    {D3DSIO_ENDREP,   "endrep",   GLNAME_REQUIRE_GLSL, 0, 0, pshader_endrep,  NULL, NULL, 0, 0},
    {D3DSIO_IF,       "if",       GLNAME_REQUIRE_GLSL, 0, 1, pshader_if,      NULL, NULL, 0, 0},
    {D3DSIO_IFC,      "ifc",      GLNAME_REQUIRE_GLSL, 0, 2, pshader_ifc,     NULL, NULL, 0, 0},
    {D3DSIO_ELSE,     "else",     GLNAME_REQUIRE_GLSL, 0, 0, pshader_else,    NULL, NULL, 0, 0},
    {D3DSIO_ENDIF,    "endif",    GLNAME_REQUIRE_GLSL, 0, 0, pshader_endif,   NULL, NULL, 0, 0},
    {D3DSIO_BREAK,    "break",    GLNAME_REQUIRE_GLSL, 0, 0, pshader_break,   NULL, NULL, 0, 0},
    {D3DSIO_BREAKC,   "breakc",   GLNAME_REQUIRE_GLSL, 0, 2, pshader_breakc,  NULL, NULL, 0, 0},
    {D3DSIO_BREAKP,   "breakp",   GLNAME_REQUIRE_GLSL, 0, 1, pshader_breakp,  NULL, NULL, 0, 0},
    {D3DSIO_CALL,     "call",     GLNAME_REQUIRE_GLSL, 0, 1, pshader_call,    NULL, NULL, 0, 0},
    {D3DSIO_CALLNZ,   "callnz",   GLNAME_REQUIRE_GLSL, 0, 2, pshader_callnz,  NULL, NULL, 0, 0},
    {D3DSIO_LOOP,     "loop",     GLNAME_REQUIRE_GLSL, 0, 2, pshader_loop,    NULL, shader_glsl_loop, 0, 0},
    {D3DSIO_RET,      "ret",      GLNAME_REQUIRE_GLSL, 0, 0, pshader_ret,     NULL, NULL, 0, 0},
    {D3DSIO_ENDLOOP,  "endloop",  GLNAME_REQUIRE_GLSL, 0, 0, pshader_endloop, NULL, shader_glsl_endloop, 0, 0},
    {D3DSIO_LABEL,    "label",    GLNAME_REQUIRE_GLSL, 0, 1, pshader_label,   NULL, NULL, 0, 0},

    /* Constant definitions */
    {D3DSIO_DEF,      "def",      "undefined",         1, 5, pshader_def,     NULL, NULL, 0, 0},
    {D3DSIO_DEFB,     "defb",     GLNAME_REQUIRE_GLSL, 1, 2, pshader_defb,    NULL, NULL, 0, 0},
    {D3DSIO_DEFI,     "defi",     GLNAME_REQUIRE_GLSL, 1, 5, pshader_defi,    NULL, NULL, 0, 0},

    /* Texture */
    {D3DSIO_TEXCOORD, "texcoord", "undefined", 1, 1, pshader_texcoord,    pshader_hw_texcoord, pshader_glsl_texcoord, 0, D3DPS_VERSION(1,3)},
    {D3DSIO_TEXCOORD, "texcrd",   "undefined", 1, 2, pshader_texcoord,    pshader_hw_texcoord, pshader_glsl_texcoord, D3DPS_VERSION(1,4), D3DPS_VERSION(1,4)},
    {D3DSIO_TEXKILL,  "texkill",  "KIL",       1, 1, pshader_texkill,     pshader_hw_map2gl, pshader_glsl_texkill, D3DPS_VERSION(1,0), D3DPS_VERSION(3,0)},
    {D3DSIO_TEX,      "tex",      "undefined", 1, 1, pshader_tex,         pshader_hw_tex, pshader_glsl_tex, 0, D3DPS_VERSION(1,3)},
    {D3DSIO_TEX,      "texld",    "undefined", 1, 2, pshader_texld,       pshader_hw_tex, pshader_glsl_tex, D3DPS_VERSION(1,4), D3DPS_VERSION(1,4)},
    {D3DSIO_TEX,      "texld",    "undefined", 1, 3, pshader_texld,       pshader_hw_tex, pshader_glsl_tex, D3DPS_VERSION(2,0), -1},
    {D3DSIO_TEXBEM,   "texbem",   "undefined", 1, 2, pshader_texbem,      pshader_hw_texbem, pshader_glsl_texbem, 0, D3DPS_VERSION(1,3)},
    {D3DSIO_TEXBEML,  "texbeml",  GLNAME_REQUIRE_GLSL, 1, 2, pshader_texbeml, NULL, NULL, D3DPS_VERSION(1,0), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXREG2AR,"texreg2ar","undefined", 1, 2, pshader_texreg2ar,   pshader_hw_texreg2ar, NULL, D3DPS_VERSION(1,1), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXREG2GB,"texreg2gb","undefined", 1, 2, pshader_texreg2gb,   pshader_hw_texreg2gb, NULL, D3DPS_VERSION(1,1), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXREG2RGB,   "texreg2rgb",   GLNAME_REQUIRE_GLSL, 1, 2, pshader_texreg2rgb,  NULL, NULL, D3DPS_VERSION(1,2), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXM3x2PAD,   "texm3x2pad",   "undefined", 1, 2, pshader_texm3x2pad,   pshader_hw_texm3x2pad, pshader_glsl_texm3x2pad, D3DPS_VERSION(1,0), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXM3x2TEX,   "texm3x2tex",   "undefined", 1, 2, pshader_texm3x2tex,   pshader_hw_texm3x2tex, pshader_glsl_texm3x2tex, D3DPS_VERSION(1,0), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXM3x3PAD,   "texm3x3pad",   "undefined", 1, 2, pshader_texm3x3pad,   pshader_hw_texm3x3pad, pshader_glsl_texm3x3pad, D3DPS_VERSION(1,0), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXM3x3DIFF,  "texm3x3diff",  GLNAME_REQUIRE_GLSL, 1, 2, pshader_texm3x3diff,  NULL, NULL, D3DPS_VERSION(0,0), D3DPS_VERSION(0,0)},
    {D3DSIO_TEXM3x3SPEC,  "texm3x3spec",  "undefined", 1, 3, pshader_texm3x3spec,  pshader_hw_texm3x3spec, NULL, D3DPS_VERSION(1,0), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXM3x3VSPEC, "texm3x3vspec",  "undefined", 1, 2, pshader_texm3x3vspec, pshader_hw_texm3x3vspec, pshader_glsl_texm3x3vspec, D3DPS_VERSION(1,0), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXM3x3TEX,   "texm3x3tex",   "undefined", 1, 2, pshader_texm3x3tex,   pshader_hw_texm3x3tex, NULL, D3DPS_VERSION(1,0), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXDP3TEX,    "texdp3tex",    GLNAME_REQUIRE_GLSL, 1, 2, pshader_texdp3tex,   NULL, NULL, D3DPS_VERSION(1,2), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXM3x2DEPTH, "texm3x2depth", GLNAME_REQUIRE_GLSL, 1, 2, pshader_texm3x2depth, NULL, NULL, D3DPS_VERSION(1,3), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXDP3,   "texdp3",   GLNAME_REQUIRE_GLSL, 1, 2, pshader_texdp3,   NULL, NULL, D3DPS_VERSION(1,2), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXM3x3,  "texm3x3",  GLNAME_REQUIRE_GLSL, 1, 2, pshader_texm3x3,  NULL, NULL, D3DPS_VERSION(1,2), D3DPS_VERSION(1,3)},
    {D3DSIO_TEXDEPTH, "texdepth", GLNAME_REQUIRE_GLSL, 1, 1, pshader_texdepth, NULL, NULL, D3DPS_VERSION(1,4), D3DPS_VERSION(1,4)},
    {D3DSIO_BEM,      "bem",      GLNAME_REQUIRE_GLSL, 1, 3, pshader_bem,      NULL, NULL, D3DPS_VERSION(1,4), D3DPS_VERSION(1,4)},
    /* TODO: dp2add can be made out of multiple instuctions */
    {D3DSIO_DSX,      "dsx",      GLNAME_REQUIRE_GLSL, 1, 2, pshader_dsx,     NULL, NULL, 0, 0},
    {D3DSIO_DSY,      "dsy",      GLNAME_REQUIRE_GLSL, 1, 2, pshader_dsy,     NULL, NULL, 0, 0},
    {D3DSIO_TEXLDD,   "texldd",   GLNAME_REQUIRE_GLSL, 1, 5, pshader_texldd,  NULL, NULL, D3DPS_VERSION(2,1), -1},
    {D3DSIO_SETP,     "setp",     GLNAME_REQUIRE_GLSL, 1, 3, pshader_setp,    NULL, NULL, 0, 0},
    {D3DSIO_TEXLDL,   "texdl",    GLNAME_REQUIRE_GLSL, 1, 2, pshader_texldl,  NULL, NULL, 0, 0},
    {D3DSIO_PHASE,    "phase",    GLNAME_REQUIRE_GLSL, 0, 0, pshader_nop,     NULL, NULL, 0, 0},
    {0,               NULL,       NULL,   0, 0, NULL,            NULL, 0, 0}
};

static void pshader_set_limits(
      IWineD3DPixelShaderImpl *This) { 

      This->baseShader.limits.attributes = 0;
      This->baseShader.limits.address = 0;
      This->baseShader.limits.packed_output = 0;

      switch (This->baseShader.hex_version) {
          case D3DPS_VERSION(1,0):
          case D3DPS_VERSION(1,1):
          case D3DPS_VERSION(1,2):
          case D3DPS_VERSION(1,3): 
                   This->baseShader.limits.temporary = 2;
                   This->baseShader.limits.constant_float = 8;
                   This->baseShader.limits.constant_int = 0;
                   This->baseShader.limits.constant_bool = 0;
                   This->baseShader.limits.texcoord = 4;
                   This->baseShader.limits.sampler = 4;
                   This->baseShader.limits.packed_input = 0;
                   break;

          case D3DPS_VERSION(1,4):
                   This->baseShader.limits.temporary = 6;
                   This->baseShader.limits.constant_float = 8;
                   This->baseShader.limits.constant_int = 0;
                   This->baseShader.limits.constant_bool = 0;
                   This->baseShader.limits.texcoord = 6;
                   This->baseShader.limits.sampler = 6;
                   This->baseShader.limits.packed_input = 0;
                   break;
               
          /* FIXME: temporaries must match D3DPSHADERCAPS2_0.NumTemps */ 
          case D3DPS_VERSION(2,0):
          case D3DPS_VERSION(2,1):
                   This->baseShader.limits.temporary = 32;
                   This->baseShader.limits.constant_float = 32;
                   This->baseShader.limits.constant_int = 16;
                   This->baseShader.limits.constant_bool = 16;
                   This->baseShader.limits.texcoord = 8;
                   This->baseShader.limits.sampler = 16;
                   This->baseShader.limits.packed_input = 0;
                   break;

          case D3DPS_VERSION(3,0):
                   This->baseShader.limits.temporary = 32;
                   This->baseShader.limits.constant_float = 224;
                   This->baseShader.limits.constant_int = 16;
                   This->baseShader.limits.constant_bool = 16;
                   This->baseShader.limits.texcoord = 0;
                   This->baseShader.limits.sampler = 16;
                   This->baseShader.limits.packed_input = 12;
                   break;

          default: This->baseShader.limits.temporary = 32;
                   This->baseShader.limits.constant_float = 32;
                   This->baseShader.limits.constant_int = 16;
                   This->baseShader.limits.constant_bool = 16;
                   This->baseShader.limits.texcoord = 8;
                   This->baseShader.limits.sampler = 16;
                   This->baseShader.limits.packed_input = 0;
                   FIXME("Unrecognized pixel shader version %#lx\n", 
                       This->baseShader.hex_version);
      }
}

/** Generate a pixel shader string using either GL_FRAGMENT_PROGRAM_ARB
    or GLSL and send it to the card */
inline static VOID IWineD3DPixelShaderImpl_GenerateShader(
    IWineD3DPixelShader *iface,
    shader_reg_maps* reg_maps,
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

    if (This->baseShader.shader_mode == SHADER_GLSL) {

        /* Create the hw GLSL shader object and assign it as the baseShader.prgId */
        GLhandleARB shader_obj = GL_EXTCALL(glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB));

        /* Base Declarations */
        shader_generate_glsl_declarations( (IWineD3DBaseShader*) This, reg_maps, &buffer);

        /* Pack 3.0 inputs */
        if (This->baseShader.hex_version >= D3DPS_VERSION(3,0))
            pshader_glsl_input_pack(&buffer, This->semantics_in);

        /* Base Shader Body */
        shader_generate_main( (IWineD3DBaseShader*) This, &buffer, reg_maps, pFunction);

        /* Pixel shaders < 2.0 place the resulting color in R0 implicitly */
        if (This->baseShader.hex_version < D3DPS_VERSION(2,0))
            shader_addline(&buffer, "gl_FragColor = R0;\n");
        shader_addline(&buffer, "}\n\0");

        TRACE("Compiling shader object %u\n", shader_obj);
        GL_EXTCALL(glShaderSourceARB(shader_obj, 1, (const char**)&buffer.buffer, NULL));
        GL_EXTCALL(glCompileShaderARB(shader_obj));
        print_glsl_info_log(&GLINFO_LOCATION, shader_obj);

        /* Store the shader object */
        This->baseShader.prgId = shader_obj;

    } else if (wined3d_settings.ps_selected_mode == SHADER_ARB) {
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

        /* Base Declarations */
        shader_generate_arb_declarations( (IWineD3DBaseShader*) This, reg_maps, &buffer);

        /* Base Shader Body */
        shader_generate_main( (IWineD3DBaseShader*) This, &buffer, reg_maps, pFunction);

        if (This->baseShader.hex_version < D3DPS_VERSION(2,0))
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

static HRESULT WINAPI IWineD3DPixelShaderImpl_SetFunction(IWineD3DPixelShader *iface, CONST DWORD *pFunction) {

    IWineD3DPixelShaderImpl *This =(IWineD3DPixelShaderImpl *)iface;
    HRESULT hr;
    shader_reg_maps reg_maps;

    /* First pass: trace shader */
    shader_trace_init((IWineD3DBaseShader*) This, pFunction);
    pshader_set_limits(This);

    /* Initialize immediate constant lists */
    list_init(&This->baseShader.constantsF);
    list_init(&This->baseShader.constantsB);
    list_init(&This->baseShader.constantsI);

    /* Second pass: figure out which registers are used, what the semantics are, etc.. */
    memset(&reg_maps, 0, sizeof(shader_reg_maps));
    hr = shader_get_registers_used((IWineD3DBaseShader*) This, &reg_maps,
        This->semantics_in, NULL, pFunction);
    if (hr != WINED3D_OK) return hr;
    /* FIXME: validate reg_maps against OpenGL */

    /* Generate HW shader in needed */
    This->baseShader.shader_mode = wined3d_settings.ps_selected_mode;
    if (NULL != pFunction && This->baseShader.shader_mode != SHADER_SW) {
        TRACE("(%p) : Generating hardware program\n", This);
        IWineD3DPixelShaderImpl_GenerateShader(iface, &reg_maps, pFunction);
    }

    TRACE("(%p) : Copying the function\n", This);
    if (NULL != pFunction) {
        This->baseShader.function = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, This->baseShader.functionLength);
        if (!This->baseShader.function) return E_OUTOFMEMORY;
        memcpy((void *)This->baseShader.function, pFunction, This->baseShader.functionLength);
    } else {
        This->baseShader.function = NULL;
    }

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
