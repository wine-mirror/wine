/*
 * Direct3D Device
 *
 * Copyright (c) 1998-2004 Lionel Ulmer
 * Copyright (c) 2002-2005 Christian Costa
 *
 * This file contains all the common stuff for D3D devices.
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

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "objbase.h"
#include "wingdi.h"
#include "ddraw.h"
#include "d3d.h"
#include "wine/debug.h"

#include "d3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

DWORD InitRenderStateTab[] = {
    D3DRENDERSTATE_TEXTUREHANDLE,           (DWORD)NULL,
    D3DRENDERSTATE_ANTIALIAS,               D3DANTIALIAS_NONE,
    /* FIXME: D3DRENDERSTATE_TEXTUREADDRESS */
    D3DRENDERSTATE_TEXTUREPERSPECTIVE,      TRUE,
    /* FIXME: D3DRENDERSTATE_WRAPU */
    /* FIXME: D3DRENDERSTATE_WRAPV */
    D3DRENDERSTATE_ZENABLE,                 D3DZB_TRUE, /* This needs to be set differently according to the Z buffer status */
    D3DRENDERSTATE_FILLMODE,                D3DFILL_SOLID,
    D3DRENDERSTATE_SHADEMODE,               D3DSHADE_GOURAUD,
    D3DRENDERSTATE_LINEPATTERN,             0,
    D3DRENDERSTATE_MONOENABLE,              FALSE,
    D3DRENDERSTATE_ROP2,                    R2_COPYPEN,
    D3DRENDERSTATE_PLANEMASK,               0xFFFFFFFF,
    D3DRENDERSTATE_ZWRITEENABLE,            TRUE,
    D3DRENDERSTATE_ALPHATESTENABLE,         FALSE,
    D3DRENDERSTATE_LASTPIXEL,               TRUE,
    D3DRENDERSTATE_TEXTUREMAG,              D3DFILTER_NEAREST,
    D3DRENDERSTATE_TEXTUREMIN,              D3DFILTER_NEAREST,
    D3DRENDERSTATE_SRCBLEND,                D3DBLEND_ONE,
    D3DRENDERSTATE_DESTBLEND,               D3DBLEND_ZERO,
    D3DRENDERSTATE_TEXTUREMAPBLEND,         D3DTBLEND_MODULATE,
    D3DRENDERSTATE_CULLMODE,                D3DCULL_CCW,
    D3DRENDERSTATE_ZFUNC,                   D3DCMP_LESSEQUAL,
    D3DRENDERSTATE_ALPHAREF,                0,
    D3DRENDERSTATE_ALPHAFUNC,               D3DCMP_ALWAYS,
    D3DRENDERSTATE_DITHERENABLE,            FALSE,
    D3DRENDERSTATE_ALPHABLENDENABLE,        FALSE,
    D3DRENDERSTATE_FOGENABLE,               FALSE,
    D3DRENDERSTATE_SPECULARENABLE,          FALSE,
    D3DRENDERSTATE_ZVISIBLE,                FALSE,
    D3DRENDERSTATE_SUBPIXEL,                FALSE,
    D3DRENDERSTATE_SUBPIXELX,               FALSE,
    D3DRENDERSTATE_STIPPLEDALPHA,           FALSE,
    D3DRENDERSTATE_FOGCOLOR,                D3DRGBA(0,0,0,0),
    D3DRENDERSTATE_FOGTABLEMODE,            D3DFOG_NONE,
    /* FIXME: D3DRENDERSTATE_FOGTABLESTART (same as D3DRENDERSTATE_FOGSTART) */
    /* FIXME: D3DRENDERSTATE_FOGTABLEEND (same as D3DRENDERSTATE_FOGEND) */
    D3DRENDERSTATE_FOGTABLEDENSITY,         0x3F80000, /* 1.0f (same as D3DRENDERSTATE_FOGDENSITY) */
    /* FIXME: D3DRENDERSTATE_STIPPLEENABLE */
    D3DRENDERSTATE_EDGEANTIALIAS,           FALSE,
    D3DRENDERSTATE_COLORKEYENABLE,          FALSE,
    /* FIXME: D3DRENDERSTATE_BORDERCOLOR */
    D3DRENDERSTATE_TEXTUREADDRESSU,         D3DTADDRESS_WRAP,
    D3DRENDERSTATE_TEXTUREADDRESSV,         D3DTADDRESS_WRAP,
    D3DRENDERSTATE_MIPMAPLODBIAS,           0x00000000, /* 0.0f */
    D3DRENDERSTATE_ZBIAS,                   0,
    D3DRENDERSTATE_RANGEFOGENABLE,          FALSE,    
    /* FIXME: D3DRENDERSTATE_ANISOTROPY */
    /* FIXME: D3DRENDERSTATE_FLUSHBATCH */
    /* FIXME: D3DRENDERSTATE_TRANSLUCENTSORTINDEPENDENT */
    D3DRENDERSTATE_STENCILENABLE,           FALSE,
    D3DRENDERSTATE_STENCILFAIL,             D3DSTENCILOP_KEEP,
    D3DRENDERSTATE_STENCILZFAIL,            D3DSTENCILOP_KEEP,
    D3DRENDERSTATE_STENCILPASS,             D3DSTENCILOP_KEEP,
    D3DRENDERSTATE_STENCILFUNC,             D3DCMP_ALWAYS,
    D3DRENDERSTATE_STENCILREF,              0,
    D3DRENDERSTATE_STENCILMASK,             0xFFFFFFFF,
    D3DRENDERSTATE_STENCILWRITEMASK,        0xFFFFFFFF,
    /* FIXME: D3DRENDERSTATE_TEXTUREFACTOR */
    /* FIXME: D3DRENDERSTATE_STIPPLEPATTERN00..31 */
    D3DRENDERSTATE_WRAP0,                   0,
    D3DRENDERSTATE_WRAP1,                   0,
    D3DRENDERSTATE_WRAP2,                   0,
    D3DRENDERSTATE_WRAP3,                   0,
    D3DRENDERSTATE_WRAP4,                   0,
    D3DRENDERSTATE_WRAP5,                   0,
    D3DRENDERSTATE_WRAP6,                   0,
    D3DRENDERSTATE_WRAP7,                   0,
    D3DRENDERSTATE_CLIPPING,                FALSE,
    D3DRENDERSTATE_LIGHTING,                TRUE,
    D3DRENDERSTATE_EXTENTS,                 FALSE,
    D3DRENDERSTATE_AMBIENT,                 D3DRGBA(0,0,0,0),
    D3DRENDERSTATE_FOGVERTEXMODE,           D3DFOG_NONE,
    D3DRENDERSTATE_COLORVERTEX,             TRUE,
    D3DRENDERSTATE_LOCALVIEWER,             TRUE,
    D3DRENDERSTATE_NORMALIZENORMALS,        FALSE,
    /* FIXME: D3DRENDER_STATE_COLORKEYBLENDENABLE */
    D3DRENDERSTATE_DIFFUSEMATERIALSOURCE,   D3DMCS_COLOR1,
    D3DRENDERSTATE_SPECULARMATERIALSOURCE,  D3DMCS_COLOR2,
    D3DRENDERSTATE_AMBIENTMATERIALSOURCE,   D3DMCS_COLOR2,
    D3DRENDERSTATE_EMISSIVEMATERIALSOURCE,  D3DMCS_MATERIAL,
    D3DRENDERSTATE_VERTEXBLEND,             D3DVBLEND_DISABLE,
    D3DRENDERSTATE_CLIPPLANEENABLE,         0
};

DWORD InitLightStateTab[] = {
    D3DLIGHTSTATE_MATERIAL,           (DWORD)NULL,
    D3DLIGHTSTATE_AMBIENT,            D3DRGBA(0,0,0,0),
    D3DLIGHTSTATE_COLORMODEL,         D3DCOLOR_RGB,
    D3DLIGHTSTATE_FOGMODE,            D3DFOG_NONE,
    D3DLIGHTSTATE_FOGSTART,           0x3F80000, /* 1.0f */
    D3DLIGHTSTATE_FOGEND,             0x42C8000, /* 100.0f */
    D3DLIGHTSTATE_FOGDENSITY,         0x3F80000  /* 1.0f */
    /* FIXME: D3DLIGHTSTATE_COLORVERTEX */
};

DWORD InitTextureStageStateTab[] = {
    D3DTSS_COLOROP,          D3DTOP_DISABLE, /* Note, it's manually set for stage 0 */
    D3DTSS_COLORARG1,        D3DTA_TEXTURE,
    D3DTSS_COLORARG2,        D3DTA_CURRENT,
    D3DTSS_ALPHAOP,          D3DTOP_DISABLE, /* Note, it's manually set for stage 0 */
    D3DTSS_ALPHAARG1,        D3DTA_TEXTURE,
    D3DTSS_ALPHAARG2,        D3DTA_CURRENT,
    /* FIXME: D3DTSS_BUMPENVMAT00,01,10,11 */
    /* D3DTSS_TEXCOORDINDEX is set manually */
    D3DTSS_ADDRESS,          D3DTADDRESS_WRAP,
    D3DTSS_ADDRESSU,         D3DTADDRESS_WRAP,
    D3DTSS_ADDRESSV,         D3DTADDRESS_WRAP,
    D3DTSS_BORDERCOLOR,      0x00000000,
    D3DTSS_MAGFILTER,        D3DTFG_POINT,
    D3DTSS_MINFILTER,        D3DTFN_POINT,
    D3DTSS_MIPFILTER,        D3DTFP_NONE,
    D3DTSS_MIPMAPLODBIAS,    0x00000000, /* 0.0f */
    D3DTSS_MAXMIPLEVEL,      0,
    /* D3DTSS_MAXANISOTROPY,    1, */ /* This is to prevent warnings :-) */
    /* FIXME: D3DTSS_BUMPENVLSCALE */
    /* FIXME: D3DTSS_NUMPENVLOFFSET */
    /* FIXME: D3DTSS_TEXTURETRANSFORMFLAGS */
};

	
void InitDefaultStateBlock(STATEBLOCK* lpStateBlock, int version)
{
    unsigned int i, j;  
    TRACE("(%p,%d)\n", lpStateBlock, version);    
    memset(lpStateBlock, 0, sizeof(STATEBLOCK));
    
    /* Initialize render states */
    for (i = 0; i < sizeof(InitRenderStateTab) / sizeof(InitRenderStateTab[0]); i += 2)
    {
        lpStateBlock->render_state[InitRenderStateTab[i] - 1] = InitRenderStateTab[i + 1];
	lpStateBlock->set_flags.render_state[InitRenderStateTab[i] - 1] = TRUE;
    }

    /* Initialize texture stages states */
    for (i = 0; i < MAX_TEXTURES; i++)
    {
       for (j = 0; j < sizeof(InitTextureStageStateTab) / sizeof(InitTextureStageStateTab[0]); j += 2)
       {
           lpStateBlock->texture_stage_state[i][InitTextureStageStateTab[j] - 1] = InitTextureStageStateTab[j + 1];
           lpStateBlock->set_flags.texture_stage_state[i][InitTextureStageStateTab[j] - 1] = TRUE;
       }
       /* Map texture coords 0 to stage 0, 1 to stage 1, etc... */
       lpStateBlock->texture_stage_state[i][D3DTSS_TEXCOORDINDEX - 1] = i;
       lpStateBlock->set_flags.texture_stage_state[i][D3DTSS_TEXCOORDINDEX - 1] = TRUE;
    }
    
    /* The first texture is particular, update it consequently */
    lpStateBlock->texture_stage_state[0][D3DTSS_COLOROP - 1] = D3DTOP_MODULATE;
    lpStateBlock->texture_stage_state[0][D3DTSS_ALPHAOP - 1] = D3DTOP_SELECTARG1;
    lpStateBlock->texture_stage_state[0][D3DTSS_COLORARG2 - 1] = D3DTA_DIFFUSE;
    lpStateBlock->texture_stage_state[0][D3DTSS_ALPHAARG2 - 1] = D3DTA_DIFFUSE;
    
    /* Updates for particular versions */
    if ((version == 1) || (version==2))
       lpStateBlock->render_state[D3DRENDERSTATE_SPECULARENABLE - 1] = TRUE;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_1T_QueryInterface(LPDIRECT3DDEVICE7 iface,
                                                   REFIID riid,
                                                   LPVOID* obp)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p/%p)->(%s,%p)\n", This, iface, debugstr_guid(riid), obp);

    *obp = NULL;

    /* Note: We cannot get an interface whose version is higher than the
     *       Direct3D object that created the device */

    if ( IsEqualGUID( &IID_IUnknown,  riid ) ) {
        IDirect3DDevice7_AddRef(ICOM_INTERFACE(This, IDirect3DDevice7));
	*obp = iface;
	TRACE("  Creating IUnknown interface at %p.\n", *obp);
	return S_OK;
    }
    if ( IsEqualGUID( &IID_IDirect3DDevice, riid ) ) {
        IDirect3DDevice7_AddRef(ICOM_INTERFACE(This, IDirect3DDevice7));
        *obp = ICOM_INTERFACE(This, IDirect3DDevice);
	TRACE("  Creating IDirect3DDevice interface %p\n", *obp);
	return S_OK;
    }
    if ( IsEqualGUID( &IID_IDirect3DDevice2, riid ) && (This->version >= 2)) {
        IDirect3DDevice7_AddRef(ICOM_INTERFACE(This, IDirect3DDevice7));
        *obp = ICOM_INTERFACE(This, IDirect3DDevice2);
	TRACE("  Creating IDirect3DDevice2 interface %p\n", *obp);
	return S_OK;
    }
    if ( IsEqualGUID( &IID_IDirect3DDevice3, riid ) && (This->version >= 3)) {
        IDirect3DDevice7_AddRef(ICOM_INTERFACE(This, IDirect3DDevice7));
        *obp = ICOM_INTERFACE(This, IDirect3DDevice3);
	TRACE("  Creating IDirect3DDevice3 interface %p\n", *obp);
	return S_OK;
    }
    if ( IsEqualGUID( &IID_IDirect3DDevice7, riid ) && (This->version >= 7)) {
        IDirect3DDevice7_AddRef(ICOM_INTERFACE(This, IDirect3DDevice7));
        *obp = ICOM_INTERFACE(This, IDirect3DDevice7);
	TRACE("  Creating IDirect3DDevice7 interface %p\n", *obp);
	return S_OK;
    }
    if ( IsEqualGUID( &IID_IDirectDrawSurface, riid ) ||
         IsEqualGUID( &IID_IDirectDrawSurface2, riid ) ||
         IsEqualGUID( &IID_IDirectDrawSurface3, riid ) ) {
        IDirectDrawSurface7_AddRef(ICOM_INTERFACE(This->surface, IDirectDrawSurface7));
        *obp = ICOM_INTERFACE(This->surface, IDirectDrawSurface3);
	TRACE("  Return IDirectDrawSurface3 interface %p\n", *obp);
	return S_OK;
    }
    if ( IsEqualGUID( &IID_IDirectDrawSurface4, riid ) ||
         IsEqualGUID( &IID_IDirectDrawSurface7, riid ) ) {
        IDirectDrawSurface7_AddRef(ICOM_INTERFACE(This->surface, IDirectDrawSurface7));
        *obp = ICOM_INTERFACE(This->surface, IDirectDrawSurface7);
	TRACE("  Return IDirectDrawSurface7 interface %p\n", *obp);
	return S_OK;
    }
    FIXME("(%p): interface for IID %s NOT found!\n", This, debugstr_guid(riid));
    return OLE_E_ENUM_NOMORE;
}

ULONG WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_1T_AddRef(LPDIRECT3DDEVICE7 iface)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p/%p)->() incrementing from %lu.\n", This, iface, ref - 1);

    return ref;
}

ULONG WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_1T_Release(LPDIRECT3DDEVICE7 iface)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p/%p)->() decrementing from %lu.\n", This, iface, ref + 1);

    if (!ref) {
        int i;
	/* Release texture associated with the device */
	for (i = 0; i < MAX_TEXTURES; i++) {
	    if (This->current_texture[i] != NULL)
	        IDirect3DTexture2_Release(ICOM_INTERFACE(This->current_texture[i], IDirect3DTexture2));
	}
	HeapFree(GetProcessHeap(), 0, This->vertex_buffer);
	HeapFree(GetProcessHeap(), 0, This);
	return 0;
    }
    return ref;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_GetCaps(LPDIRECT3DDEVICE7 iface,
                                   LPD3DDEVICEDESC7 lpD3DHELDevDesc)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p/%p)->(%p): stub!\n", This, iface, lpD3DHELDevDesc);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_EnumTextureFormats(LPDIRECT3DDEVICE7 iface,
                                                 LPD3DENUMPIXELFORMATSCALLBACK lpD3DEnumPixelProc,
                                                 LPVOID lpArg)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p/%p)->(%p,%p): stub!\n", This, iface, lpD3DEnumPixelProc, lpArg);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_1T_BeginScene(LPDIRECT3DDEVICE7 iface)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p/%p)->()\n", This, iface);
    /* Nothing to do */
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_1T_EndScene(LPDIRECT3DDEVICE7 iface)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p/%p)->()\n", This, iface);
    /* Nothing to do */
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_1T_GetDirect3D(LPDIRECT3DDEVICE7 iface,
						LPDIRECT3D7* lplpDirect3D7)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p/%p)->(%p)\n", This, iface, lplpDirect3D7);

    *lplpDirect3D7 = ICOM_INTERFACE(This->d3d, IDirect3D7);
    IDirect3D7_AddRef(ICOM_INTERFACE(This->d3d, IDirect3D7));
    
    TRACE(" returning interface %p\n", *lplpDirect3D7);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_SetRenderTarget(LPDIRECT3DDEVICE7 iface,
						 LPDIRECTDRAWSURFACE7 lpNewRenderTarget,
						 DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    IDirectDrawSurfaceImpl *target_impl = ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirectDrawSurface7, lpNewRenderTarget);

    TRACE("(%p/%p)->(%p,%08lx)\n", This, iface, lpNewRenderTarget, dwFlags);
    if (target_impl != This->surface) {
        WARN(" Change of rendering target not handled yet !\n");
    }
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_GetRenderTarget(LPDIRECT3DDEVICE7 iface,
						 LPDIRECTDRAWSURFACE7* lplpRenderTarget)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p/%p)->(%p)\n", This, iface, lplpRenderTarget);

    *lplpRenderTarget = ICOM_INTERFACE(This->surface, IDirectDrawSurface7);
    IDirectDrawSurface7_AddRef(ICOM_INTERFACE(This->surface, IDirectDrawSurface7));
    
    TRACE(" returning surface at %p.\n", *lplpRenderTarget);
    
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_Clear(LPDIRECT3DDEVICE7 iface,
                                 DWORD dwCount,
                                 LPD3DRECT lpRects,
                                 DWORD dwFlags,
                                 D3DCOLOR dwColor,
                                 D3DVALUE dvZ,
                                 DWORD dwStencil)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p/%p)->(%08lx,%p,%08lx,%08lx,%f,%08lx)\n", This, iface, dwCount, lpRects, dwFlags, (DWORD) dwColor, dvZ, dwStencil);
    return This->clear(This, dwCount, lpRects, dwFlags, dwColor, dvZ, dwStencil);
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_SetTransform(LPDIRECT3DDEVICE7 iface,
                                              D3DTRANSFORMSTATETYPE dtstTransformStateType,
                                              LPD3DMATRIX lpD3DMatrix)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    DWORD matrix_changed = 0x00000000;

    TRACE("(%p/%p)->(%08x,%p)\n", This, iface, dtstTransformStateType, lpD3DMatrix);

    switch (dtstTransformStateType) {
        case D3DTRANSFORMSTATE_WORLD: {
	    if (TRACE_ON(ddraw)) {
	        TRACE(" D3DTRANSFORMSTATE_WORLD :\n"); dump_D3DMATRIX(lpD3DMatrix);
	    }
	    memcpy(This->world_mat, lpD3DMatrix, 16 * sizeof(float));
	    matrix_changed = WORLDMAT_CHANGED;
	} break;

	case D3DTRANSFORMSTATE_VIEW: {
	    if (TRACE_ON(ddraw)) {
	        TRACE(" D3DTRANSFORMSTATE_VIEW :\n");  dump_D3DMATRIX(lpD3DMatrix);
	    }
	    memcpy(This->view_mat, lpD3DMatrix, 16 * sizeof(float));
	    matrix_changed = VIEWMAT_CHANGED;
	} break;

	case D3DTRANSFORMSTATE_PROJECTION: {
	    if (TRACE_ON(ddraw)) {
	        TRACE(" D3DTRANSFORMSTATE_PROJECTION :\n");  dump_D3DMATRIX(lpD3DMatrix);
	    }
	    memcpy(This->proj_mat, lpD3DMatrix, 16 * sizeof(float));
	    matrix_changed = PROJMAT_CHANGED;
	} break;

	case D3DTRANSFORMSTATE_TEXTURE0:
	case D3DTRANSFORMSTATE_TEXTURE1:
	case D3DTRANSFORMSTATE_TEXTURE2:
	case D3DTRANSFORMSTATE_TEXTURE3:
	case D3DTRANSFORMSTATE_TEXTURE4:
	case D3DTRANSFORMSTATE_TEXTURE5:
	case D3DTRANSFORMSTATE_TEXTURE6:
	case D3DTRANSFORMSTATE_TEXTURE7: {
	    DWORD mat_num = dtstTransformStateType - D3DTRANSFORMSTATE_TEXTURE0;
	    if (TRACE_ON(ddraw)) {
	        TRACE(" D3DTRANSFORMSTATE_TEXTURE%ld :\n", mat_num);  dump_D3DMATRIX(lpD3DMatrix);
	    }
	    memcpy(This->tex_mat[mat_num], lpD3DMatrix, 16 * sizeof(float));
	    matrix_changed = TEXMAT0_CHANGED << mat_num;
	} break;
	  
	default:
	    ERR("Unknown transform type %08x !!!\n", dtstTransformStateType);
	    break;
    }

    if (matrix_changed != 0x00000000) This->matrices_updated(This, matrix_changed);

    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_GetTransform(LPDIRECT3DDEVICE7 iface,
                                              D3DTRANSFORMSTATETYPE dtstTransformStateType,
                                              LPD3DMATRIX lpD3DMatrix)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p/%p)->(%08x,%p)\n", This, iface, dtstTransformStateType, lpD3DMatrix);

    switch (dtstTransformStateType) {
        case D3DTRANSFORMSTATE_WORLD: {
	    memcpy(lpD3DMatrix, This->world_mat, 16 * sizeof(D3DVALUE));
	    if (TRACE_ON(ddraw)) {
	        TRACE(" returning D3DTRANSFORMSTATE_WORLD :\n");
		dump_D3DMATRIX(lpD3DMatrix);
	    }
	} break;

	case D3DTRANSFORMSTATE_VIEW: {
	    memcpy(lpD3DMatrix, This->view_mat, 16 * sizeof(D3DVALUE));
	    if (TRACE_ON(ddraw)) {
	        TRACE(" returning D3DTRANSFORMSTATE_VIEW :\n");
		dump_D3DMATRIX(lpD3DMatrix);
	    }
	} break;

	case D3DTRANSFORMSTATE_PROJECTION: {
	    memcpy(lpD3DMatrix, This->proj_mat, 16 * sizeof(D3DVALUE));
	    if (TRACE_ON(ddraw)) {
	        TRACE(" returning D3DTRANSFORMSTATE_PROJECTION :\n");
		dump_D3DMATRIX(lpD3DMatrix);
	    }
	} break;

	case D3DTRANSFORMSTATE_TEXTURE0:
	case D3DTRANSFORMSTATE_TEXTURE1:
	case D3DTRANSFORMSTATE_TEXTURE2:
	case D3DTRANSFORMSTATE_TEXTURE3:
	case D3DTRANSFORMSTATE_TEXTURE4:
	case D3DTRANSFORMSTATE_TEXTURE5:
	case D3DTRANSFORMSTATE_TEXTURE6:
	case D3DTRANSFORMSTATE_TEXTURE7: {
	    DWORD mat_num = dtstTransformStateType - D3DTRANSFORMSTATE_TEXTURE0;
	    memcpy(lpD3DMatrix, This->tex_mat[mat_num], 16 * sizeof(D3DVALUE));
	    if (TRACE_ON(ddraw)) {
	        TRACE(" returning D3DTRANSFORMSTATE_TEXTURE%ld :\n", mat_num);
		dump_D3DMATRIX(lpD3DMatrix);
	    }
	} break;
	  
	default:
	    ERR("Unknown transform type %08x !!!\n", dtstTransformStateType);
	    return DDERR_INVALIDPARAMS;
    }

    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_MultiplyTransform(LPDIRECT3DDEVICE7 iface,
                                                   D3DTRANSFORMSTATETYPE dtstTransformStateType,
                                                   LPD3DMATRIX lpD3DMatrix)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    LPD3DMATRIX mat;
    DWORD matrix_changed = 0x00000000;

    TRACE("(%p/%p)->(%08x,%p)\n", This, iface, dtstTransformStateType, lpD3DMatrix);
    
    if (TRACE_ON(ddraw)) {
        TRACE(" Multiplying by :\n"); dump_D3DMATRIX(lpD3DMatrix);
    }
    
    switch (dtstTransformStateType) {
        case D3DTRANSFORMSTATE_WORLD: {
	    if (TRACE_ON(ddraw)) {
	        TRACE(" Resulting D3DTRANSFORMSTATE_WORLD matrix is :\n");
	    }
	    mat = This->world_mat;
	    matrix_changed = WORLDMAT_CHANGED;
	} break;

        case D3DTRANSFORMSTATE_VIEW: {
	    if (TRACE_ON(ddraw)) {
	        TRACE(" Resulting D3DTRANSFORMSTATE_VIEW matrix is :\n");
	    }
	    mat = This->view_mat;
	    matrix_changed = VIEWMAT_CHANGED;
	} break;

        case D3DTRANSFORMSTATE_PROJECTION: {
	    if (TRACE_ON(ddraw)) {
	        TRACE(" Resulting D3DTRANSFORMSTATE_PROJECTION matrix is :\n");
	    }
	    mat = This->proj_mat;
	    matrix_changed = PROJMAT_CHANGED;
	} break;

	case D3DTRANSFORMSTATE_TEXTURE0:
	case D3DTRANSFORMSTATE_TEXTURE1:
	case D3DTRANSFORMSTATE_TEXTURE2:
	case D3DTRANSFORMSTATE_TEXTURE3:
	case D3DTRANSFORMSTATE_TEXTURE4:
	case D3DTRANSFORMSTATE_TEXTURE5:
	case D3DTRANSFORMSTATE_TEXTURE6:
	case D3DTRANSFORMSTATE_TEXTURE7: {
	    DWORD mat_num = dtstTransformStateType - D3DTRANSFORMSTATE_TEXTURE0;
	    if (TRACE_ON(ddraw)) {
	        TRACE(" Resulting D3DTRANSFORMSTATE_TEXTURE%ld matrix is :\n", mat_num);
	    }
	    mat = This->tex_mat[mat_num];
	    matrix_changed = TEXMAT0_CHANGED << mat_num;
	} break;

	default:
	    ERR("Unknown transform type %08x !!!\n", dtstTransformStateType);
	    return DDERR_INVALIDPARAMS;
    }

    multiply_matrix(mat,mat,lpD3DMatrix);
    
    if (TRACE_ON(ddraw)) {
        dump_D3DMATRIX(mat);
    }
    
    if (matrix_changed != 0x00000000) This->matrices_updated(This, matrix_changed);
    
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_GetViewport(LPDIRECT3DDEVICE7 iface,
                                       LPD3DVIEWPORT7 lpData)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p/%p)->(%p)\n", This, iface, lpData);

    *lpData = This->active_viewport;

    if (TRACE_ON(ddraw)) {
        TRACE(" returning viewport :\n");
	TRACE("    - dwX = %ld   dwY = %ld\n",
	      lpData->dwX, lpData->dwY);
	TRACE("    - dwWidth = %ld   dwHeight = %ld\n",
	      lpData->dwWidth, lpData->dwHeight);
	TRACE("    - dvMinZ = %f   dvMaxZ = %f\n",
	      lpData->dvMinZ, lpData->dvMaxZ);
    }

    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_SetMaterial(LPDIRECT3DDEVICE7 iface,
                                       LPD3DMATERIAL7 lpMat)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p/%p)->(%p): stub!\n", This, iface, lpMat);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_GetMaterial(LPDIRECT3DDEVICE7 iface,
                                       LPD3DMATERIAL7 lpMat)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p/%p)->(%p)\n", This, iface, lpMat);
    
    *lpMat = This->current_material;

    if (TRACE_ON(ddraw)) {
        TRACE(" returning material :\n");
	dump_D3DMATERIAL7(lpMat);
    }

    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_SetLight(LPDIRECT3DDEVICE7 iface,
                                    DWORD dwLightIndex,
                                    LPD3DLIGHT7 lpLight)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p/%p)->(%08lx,%p): stub!\n", This, iface, dwLightIndex, lpLight);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_GetLight(LPDIRECT3DDEVICE7 iface,
                                    DWORD dwLightIndex,
                                    LPD3DLIGHT7 lpLight)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p/%p)->(%08lx,%p)\n", This, iface, dwLightIndex, lpLight);

    if (dwLightIndex >= This->num_set_lights)
        return DDERR_INVALIDPARAMS;

    *lpLight = This->light_parameters[dwLightIndex];

    /* If dltType is zero, then this light has never been set, either
       by calling SetLight or implicitely by calling EnableLight without
       calling SetLight first. */
    if (lpLight->dltType == 0)
        return DDERR_INVALIDPARAMS;

    if (TRACE_ON(ddraw)) {
        TRACE(" returning light :\n");
	dump_D3DLIGHT7(lpLight);
    }

    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_SetRenderState(LPDIRECT3DDEVICE7 iface,
                                                D3DRENDERSTATETYPE dwRenderStateType,
                                                DWORD dwRenderState)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p/%p)->(%08x,%08lx): stub!\n", This, iface, dwRenderStateType, dwRenderState);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_GetRenderState(LPDIRECT3DDEVICE7 iface,
                                                D3DRENDERSTATETYPE dwRenderStateType,
                                                LPDWORD lpdwRenderState)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p/%p)->(%08x,%p): stub!\n", This, iface, dwRenderStateType, lpdwRenderState);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_BeginStateBlock(LPDIRECT3DDEVICE7 iface)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p/%p)->(): stub!\n", This, iface);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_EndStateBlock(LPDIRECT3DDEVICE7 iface,
                                         LPDWORD lpdwBlockHandle)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p/%p)->(%p): stub!\n", This, iface, lpdwBlockHandle);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_PreLoad(LPDIRECT3DDEVICE7 iface,
                                   LPDIRECTDRAWSURFACE7 lpddsTexture)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p/%p)->(%p): stub!\n", This, iface, lpddsTexture);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_DrawPrimitive(LPDIRECT3DDEVICE7 iface,
                                            D3DPRIMITIVETYPE d3dptPrimitiveType,
                                            DWORD d3dvtVertexType,
                                            LPVOID lpvVertices,
                                            DWORD dwVertexCount,
                                            DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p/%p)->(%08x,%08lx,%p,%08lx,%08lx): stub!\n", This, iface, d3dptPrimitiveType, d3dvtVertexType, lpvVertices, dwVertexCount, dwFlags);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_DrawIndexedPrimitive(LPDIRECT3DDEVICE7 iface,
                                                   D3DPRIMITIVETYPE d3dptPrimitiveType,
                                                   DWORD d3dvtVertexType,
                                                   LPVOID lpvVertices,
                                                   DWORD dwVertexCount,
                                                   LPWORD dwIndices,
                                                   DWORD dwIndexCount,
                                                   DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p/%p)->(%08x,%08lx,%p,%08lx,%p,%08lx,%08lx): stub!\n", This, iface, d3dptPrimitiveType, d3dvtVertexType, lpvVertices, dwVertexCount, dwIndices, dwIndexCount, dwFlags);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_SetClipStatus(LPDIRECT3DDEVICE7 iface,
                                               LPD3DCLIPSTATUS lpD3DClipStatus)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p/%p)->(%p): stub!\n", This, iface, lpD3DClipStatus);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_GetClipStatus(LPDIRECT3DDEVICE7 iface,
                                               LPD3DCLIPSTATUS lpD3DClipStatus)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p/%p)->(%p): stub!\n", This, iface, lpD3DClipStatus);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_DrawPrimitiveStrided(LPDIRECT3DDEVICE7 iface,
                                                   D3DPRIMITIVETYPE d3dptPrimitiveType,
                                                   DWORD dwVertexType,
                                                   LPD3DDRAWPRIMITIVESTRIDEDDATA lpD3DDrawPrimStrideData,
                                                   DWORD dwVertexCount,
                                                   DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p/%p)->(%08x,%08lx,%p,%08lx,%08lx): stub!\n", This, iface, d3dptPrimitiveType, dwVertexType, lpD3DDrawPrimStrideData, dwVertexCount, dwFlags);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_DrawIndexedPrimitiveStrided(LPDIRECT3DDEVICE7 iface,
                                                          D3DPRIMITIVETYPE d3dptPrimitiveType,
                                                          DWORD dwVertexType,
                                                          LPD3DDRAWPRIMITIVESTRIDEDDATA lpD3DDrawPrimStrideData,
                                                          DWORD dwVertexCount,
                                                          LPWORD lpIndex,
                                                          DWORD dwIndexCount,
                                                          DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p/%p)->(%08x,%08lx,%p,%08lx,%p,%08lx,%08lx): stub!\n", This, iface, d3dptPrimitiveType, dwVertexType, lpD3DDrawPrimStrideData, dwVertexCount, lpIndex, dwIndexCount, dwFlags);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_DrawPrimitiveVB(LPDIRECT3DDEVICE7 iface,
					      D3DPRIMITIVETYPE d3dptPrimitiveType,
					      LPDIRECT3DVERTEXBUFFER7 lpD3DVertexBuf,
					      DWORD dwStartVertex,
					      DWORD dwNumVertices,
					      DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p/%p)->(%08x,%p,%08lx,%08lx,%08lx): stub!\n", This, iface, d3dptPrimitiveType, lpD3DVertexBuf, dwStartVertex, dwNumVertices, dwFlags);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_DrawIndexedPrimitiveVB(LPDIRECT3DDEVICE7 iface,
						     D3DPRIMITIVETYPE d3dptPrimitiveType,
						     LPDIRECT3DVERTEXBUFFER7 lpD3DVertexBuf,
						     DWORD dwStartVertex,
						     DWORD dwNumVertices,
						     LPWORD lpwIndices,
						     DWORD dwIndexCount,
						     DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p/%p)->(%08x,%p,%08lx,%08lx,%p,%08lx,%08lx): stub!\n", This, iface, d3dptPrimitiveType, lpD3DVertexBuf, dwStartVertex, dwNumVertices, lpwIndices, dwIndexCount, dwFlags);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_ComputeSphereVisibility(LPDIRECT3DDEVICE7 iface,
                                                      LPD3DVECTOR lpCenters,
                                                      LPD3DVALUE lpRadii,
                                                      DWORD dwNumSpheres,
                                                      DWORD dwFlags,
                                                      LPDWORD lpdwReturnValues)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p/%p)->(%p,%p,%08lx,%08lx,%p): stub!\n", This, iface, lpCenters, lpRadii, dwNumSpheres, dwFlags, lpdwReturnValues);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_GetTexture(LPDIRECT3DDEVICE7 iface,
					 DWORD dwStage,
					 LPDIRECTDRAWSURFACE7* lpTexture)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p/%p)->(%08lx,%p)\n", This, iface, dwStage, lpTexture);

    if (This->current_texture[dwStage] != NULL) {
        *lpTexture = ICOM_INTERFACE(This->current_texture[dwStage], IDirectDrawSurface7);
        IDirectDrawSurface7_AddRef(*lpTexture);
    } else {
        *lpTexture = NULL;
    }

    TRACE(" returning interface at %p (for implementation at %p).\n", *lpTexture, This->current_texture[dwStage]);

    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_SetTexture(LPDIRECT3DDEVICE7 iface,
					 DWORD dwStage,
					 LPDIRECTDRAWSURFACE7 lpTexture)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p/%p)->(%08lx,%p): stub!\n", This, iface, dwStage, lpTexture);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_GetTextureStageState(LPDIRECT3DDEVICE7 iface,
                                                   DWORD dwStage,
                                                   D3DTEXTURESTAGESTATETYPE d3dTexStageStateType,
                                                   LPDWORD lpdwState)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p/%p)->(%08lx,%08x,%p)\n", This, iface, dwStage, d3dTexStageStateType, lpdwState);
    if (lpdwState && (dwStage < 8) && d3dTexStageStateType && (d3dTexStageStateType <= HIGHEST_TEXTURE_STAGE_STATE) ) {
        *lpdwState = This->state_block.texture_stage_state[dwStage][d3dTexStageStateType-1];
	return DD_OK;
    }
    return DDERR_INVALIDPARAMS;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_SetTextureStageState(LPDIRECT3DDEVICE7 iface,
                                                   DWORD dwStage,
                                                   D3DTEXTURESTAGESTATETYPE d3dTexStageStateType,
                                                   DWORD dwState)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p/%p)->(%08lx,%08x,%08lx): stub!\n", This, iface, dwStage, d3dTexStageStateType, dwState);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_ValidateDevice(LPDIRECT3DDEVICE7 iface,
                                             LPDWORD lpdwPasses)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p/%p)->(%p): semi-stub!\n", This, iface, lpdwPasses);

    /* For the moment, we have a VERY good hardware which does everything in one pass :-) */
    *lpdwPasses = 1;

    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_ApplyStateBlock(LPDIRECT3DDEVICE7 iface,
                                           DWORD dwBlockHandle)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p/%p)->(%08lx): stub!\n", This, iface, dwBlockHandle);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_CaptureStateBlock(LPDIRECT3DDEVICE7 iface,
                                             DWORD dwBlockHandle)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p/%p)->(%08lx): stub!\n", This, iface, dwBlockHandle);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_DeleteStateBlock(LPDIRECT3DDEVICE7 iface,
                                            DWORD dwBlockHandle)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p/%p)->(%08lx): stub!\n", This, iface, dwBlockHandle);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_CreateStateBlock(LPDIRECT3DDEVICE7 iface,
                                            D3DSTATEBLOCKTYPE d3dsbType,
                                            LPDWORD lpdwBlockHandle)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p/%p)->(%08x,%p): stub!\n", This, iface, d3dsbType, lpdwBlockHandle);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_Load(LPDIRECT3DDEVICE7 iface,
                                LPDIRECTDRAWSURFACE7 lpDestTex,
                                LPPOINT lpDestPoint,
                                LPDIRECTDRAWSURFACE7 lpSrcTex,
                                LPRECT lprcSrcRect,
                                DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p/%p)->(%p,%p,%p,%p,%08lx): Partially Implemented!\n", This, iface, lpDestTex, lpDestPoint, lpSrcTex, lprcSrcRect, dwFlags);
    IDirect3DTexture2_Load(COM_INTERFACE_CAST(IDirectDrawSurfaceImpl, IDirectDrawSurface7, IDirect3DTexture2, lpDestTex),
			   COM_INTERFACE_CAST(IDirectDrawSurfaceImpl, IDirectDrawSurface7, IDirect3DTexture2, lpSrcTex));
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_LightEnable(LPDIRECT3DDEVICE7 iface,
                                       DWORD dwLightIndex,
                                       BOOL bEnable)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p/%p)->(%08lx,%d): stub!\n", This, iface, dwLightIndex, bEnable);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_GetLightEnable(LPDIRECT3DDEVICE7 iface,
                                          DWORD dwLightIndex,
                                          BOOL* pbEnable)
{
    int i;

    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p/%p)->(%08lx,%p)\n", This, iface, dwLightIndex, pbEnable);

    *pbEnable = 0;
    if (dwLightIndex >= This->num_set_lights)
        return DDERR_INVALIDPARAMS;

    /* If dltType is zero, then this light has never been set, either
       by calling SetLight or implicitely by calling EnableLight without
       calling SetLight first. */
    if (This->light_parameters[dwLightIndex].dltType == 0)
        return DDERR_INVALIDPARAMS;

    for (i = 0; i < This->max_active_lights; i++)
        if (This->active_lights[i] == dwLightIndex)
            *pbEnable = TRUE;

    TRACE(" returning %d.\n", *pbEnable);

    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_SetClipPlane(LPDIRECT3DDEVICE7 iface,
                                        DWORD dwIndex,
                                        D3DVALUE* pPlaneEquation)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p/%p)->(%08lx,%p): stub!\n", This, iface, dwIndex, pPlaneEquation);
    return DD_OK;
}

HRESULT  WINAPI  
Main_IDirect3DDeviceImpl_7_GetClipPlane(LPDIRECT3DDEVICE7 iface, DWORD dwIndex, D3DVALUE* pPlaneEquation)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;

    TRACE("(%p)->(%ld,%p)\n", This, dwIndex, pPlaneEquation);

    if (dwIndex>=This->max_clipping_planes) {
	return DDERR_INVALIDPARAMS;
    }

    memcpy( pPlaneEquation, This->clipping_planes[dwIndex].plane, sizeof(D3DVALUE[4]));

    return D3D_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_GetInfo(LPDIRECT3DDEVICE7 iface,
                                   DWORD dwDevInfoID,
                                   LPVOID pDevInfoStruct,
                                   DWORD dwSize)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p/%p)->(%08lx,%p,%08lx)\n", This, iface, dwDevInfoID, pDevInfoStruct, dwSize);

    if (TRACE_ON(ddraw)) {
        TRACE(" info requested : ");
	switch (dwDevInfoID) {
	    case D3DDEVINFOID_TEXTUREMANAGER: TRACE("D3DDEVINFOID_TEXTUREMANAGER\n"); break;
	    case D3DDEVINFOID_D3DTEXTUREMANAGER: TRACE("D3DDEVINFOID_D3DTEXTUREMANAGER\n"); break;
	    case D3DDEVINFOID_TEXTURING: TRACE("D3DDEVINFOID_TEXTURING\n"); break;
	    default: ERR(" invalid flag !!!\n"); return DDERR_INVALIDPARAMS;
	}
    }

    return S_FALSE; /* According to MSDN, this is valid for a non-debug driver */
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_2T_1T_GetCaps(LPDIRECT3DDEVICE3 iface,
                                         LPD3DDEVICEDESC lpD3DHWDevDesc,
                                         LPD3DDEVICEDESC lpD3DHELDevDesc)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    FIXME("(%p/%p)->(%p,%p): stub!\n", This, iface, lpD3DHWDevDesc, lpD3DHELDevDesc);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_2T_1T_GetStats(LPDIRECT3DDEVICE3 iface,
                                          LPD3DSTATS lpD3DStats)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    FIXME("(%p/%p)->(%p): stub!\n", This, iface, lpD3DStats);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_2T_1T_AddViewport(LPDIRECT3DDEVICE3 iface,
					     LPDIRECT3DVIEWPORT3 lpDirect3DViewport3)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    IDirect3DViewportImpl *lpDirect3DViewportImpl = ICOM_OBJECT(IDirect3DViewportImpl, IDirect3DViewport3, lpDirect3DViewport3);
    
    TRACE("(%p/%p)->(%p)\n", This, iface, lpDirect3DViewport3);

    lpDirect3DViewportImpl->next = This->viewport_list;
    This->viewport_list = lpDirect3DViewportImpl;
    
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_2T_1T_DeleteViewport(LPDIRECT3DDEVICE3 iface,
						LPDIRECT3DVIEWPORT3 lpDirect3DViewport3)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    IDirect3DViewportImpl *lpDirect3DViewportImpl = ICOM_OBJECT(IDirect3DViewportImpl, IDirect3DViewport3, lpDirect3DViewport3);
    IDirect3DViewportImpl *cur_viewport, *prev_viewport = NULL;
  
    TRACE("(%p/%p)->(%p)\n", This, iface, lpDirect3DViewport3);

    cur_viewport = This->viewport_list;
    while (cur_viewport != NULL) {
        if (cur_viewport == lpDirect3DViewportImpl) {
	    if (prev_viewport == NULL) This->viewport_list = cur_viewport->next;
	    else prev_viewport->next = cur_viewport->next;
	    /* TODO : add desactivate of the viewport and all associated lights... */
	    return DD_OK;
	}
	prev_viewport = cur_viewport;
	cur_viewport = cur_viewport->next;
    }
    
    return DDERR_INVALIDPARAMS;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_2T_1T_NextViewport(LPDIRECT3DDEVICE3 iface,
					      LPDIRECT3DVIEWPORT3 lpDirect3DViewport3,
					      LPDIRECT3DVIEWPORT3* lplpDirect3DViewport3,
					      DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    IDirect3DViewportImpl *res = NULL;

    TRACE("(%p/%p)->(%p,%p,%08lx)\n", This, iface, lpDirect3DViewport3, lplpDirect3DViewport3, dwFlags);
    
    switch (dwFlags) {
        case D3DNEXT_NEXT: {
	    IDirect3DViewportImpl *lpDirect3DViewportImpl = ICOM_OBJECT(IDirect3DViewportImpl, IDirect3DViewport3, lpDirect3DViewport3);
	    res = lpDirect3DViewportImpl->next;
	} break;
	case D3DNEXT_HEAD: {
	    res = This->viewport_list;
	} break;
	case D3DNEXT_TAIL: {
	    IDirect3DViewportImpl *cur_viewport = This->viewport_list;
	    if (cur_viewport != NULL) {
	        while (cur_viewport->next != NULL) cur_viewport = cur_viewport->next;
	    }
	    res = cur_viewport;
	} break;
	default:
	    *lplpDirect3DViewport3 = NULL;
	    return DDERR_INVALIDPARAMS;
    }
    *lplpDirect3DViewport3 = ICOM_INTERFACE(res, IDirect3DViewport3);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_2T_SetCurrentViewport(LPDIRECT3DDEVICE3 iface,
						 LPDIRECT3DVIEWPORT3 lpDirect3DViewport3)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    TRACE("(%p/%p)->(%p)\n", This, iface, lpDirect3DViewport3);

    /* Do nothing if the specified viewport is the same as the current one */
    if (This->current_viewport == ICOM_OBJECT(IDirect3DViewportImpl, IDirect3DViewport3, lpDirect3DViewport3))
      return DD_OK;
    
    /* Should check if the viewport was added or not */

    /* Release previous viewport and AddRef the new one */
    if (This->current_viewport)
      IDirect3DViewport3_Release(ICOM_INTERFACE(This->current_viewport, IDirect3DViewport3));
    IDirect3DViewport3_AddRef(lpDirect3DViewport3);
    
    /* Set this viewport as the current viewport */
    This->current_viewport = ICOM_OBJECT(IDirect3DViewportImpl, IDirect3DViewport3, lpDirect3DViewport3);

    /* Activate this viewport */
    This->current_viewport->active_device = This;
    This->current_viewport->activate(This->current_viewport);

    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_2T_GetCurrentViewport(LPDIRECT3DDEVICE3 iface,
						 LPDIRECT3DVIEWPORT3* lplpDirect3DViewport3)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    TRACE("(%p/%p)->(%p)\n", This, iface, lplpDirect3DViewport3);

    *lplpDirect3DViewport3 = ICOM_INTERFACE(This->current_viewport, IDirect3DViewport3);

    /* AddRef the returned viewport */
    IDirect3DViewport3_AddRef(*lplpDirect3DViewport3);
    
    TRACE(" returning interface %p\n", *lplpDirect3DViewport3);
    
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_Begin(LPDIRECT3DDEVICE3 iface,
                                 D3DPRIMITIVETYPE d3dptPrimitiveType,
                                 DWORD dwVertexTypeDesc,
                                 DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    TRACE("(%p/%p)->(%08x,%08lx,%08lx)\n", This, iface, d3dptPrimitiveType, dwVertexTypeDesc, dwFlags);

    This->primitive_type = d3dptPrimitiveType;
    This->vertex_type = dwVertexTypeDesc;
    This->render_flags = dwFlags;
    This->vertex_size = get_flexible_vertex_size(This->vertex_type);
    This->nb_vertices = 0;

    return D3D_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_BeginIndexed(LPDIRECT3DDEVICE3 iface,
                                        D3DPRIMITIVETYPE d3dptPrimitiveType,
                                        DWORD d3dvtVertexType,
                                        LPVOID lpvVertices,
                                        DWORD dwNumVertices,
                                        DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    FIXME("(%p/%p)->(%08x,%08lx,%p,%08lx,%08lx): stub!\n", This, iface, d3dptPrimitiveType, d3dvtVertexType, lpvVertices, dwNumVertices, dwFlags);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_2T_Vertex(LPDIRECT3DDEVICE3 iface,
                                     LPVOID lpVertexType)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    TRACE("(%p/%p)->(%p)\n", This, iface, lpVertexType);

    if ((This->nb_vertices+1)*This->vertex_size > This->buffer_size)
    {
	LPBYTE old_buffer;
	This->buffer_size = This->buffer_size ? This->buffer_size * 2 : This->vertex_size * 3;
	old_buffer = This->vertex_buffer;
	This->vertex_buffer = HeapAlloc(GetProcessHeap(), 0, This->buffer_size);
	if (old_buffer)
	{
	    CopyMemory(This->vertex_buffer, old_buffer, This->nb_vertices * This->vertex_size);
	    HeapFree(GetProcessHeap(), 0, old_buffer);
	}
    }

    CopyMemory(This->vertex_buffer + This->nb_vertices++ * This->vertex_size, lpVertexType, This->vertex_size);

    return D3D_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_2T_Index(LPDIRECT3DDEVICE3 iface,
                                    WORD wVertexIndex)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    FIXME("(%p/%p)->(%04x): stub!\n", This, iface, wVertexIndex);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_2T_End(LPDIRECT3DDEVICE3 iface,
                                  DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    TRACE("(%p/%p)->(%08lx)\n", This, iface, dwFlags);

    IDirect3DDevice3_DrawPrimitive(iface, This->primitive_type, This->vertex_type, This->vertex_buffer, This->nb_vertices, This->render_flags);

    return D3D_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_2T_GetLightState(LPDIRECT3DDEVICE3 iface,
                                            D3DLIGHTSTATETYPE dwLightStateType,
                                            LPDWORD lpdwLightState)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    FIXME("(%p/%p)->(%08x,%p): stub !\n", This, iface, dwLightStateType, lpdwLightState);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_2T_SetLightState(LPDIRECT3DDEVICE3 iface,
                                            D3DLIGHTSTATETYPE dwLightStateType,
                                            DWORD dwLightState)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    FIXME("(%p/%p)->(%08x,%08lx): stub!\n", This, iface, dwLightStateType, dwLightState);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_2_1T_SwapTextureHandles(LPDIRECT3DDEVICE2 iface,
                                                 LPDIRECT3DTEXTURE2 lpD3DTex1,
                                                 LPDIRECT3DTEXTURE2 lpD3DTex2)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    IDirectDrawSurfaceImpl tmp,*surf1,*surf2;
    TRACE("(%p/%p)->(%p,%p):\n", This, iface, lpD3DTex1, lpD3DTex2);

    surf1 = ICOM_OBJECT(IDirectDrawSurfaceImpl,IDirect3DTexture2,lpD3DTex1);
    surf2 = ICOM_OBJECT(IDirectDrawSurfaceImpl,IDirect3DTexture2,lpD3DTex2);
    tmp = *surf1;
    *surf1 = *surf2;
    *surf2 = tmp;
    
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_2_NextViewport(LPDIRECT3DDEVICE2 iface,
                                        LPDIRECT3DVIEWPORT2 lpDirect3DViewport2,
                                        LPDIRECT3DVIEWPORT2* lplpDirect3DViewport2,
                                        DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    FIXME("(%p/%p)->(%p,%p,%08lx): stub!\n", This, iface, lpDirect3DViewport2, lplpDirect3DViewport2, dwFlags);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_2_1T_EnumTextureFormats(LPDIRECT3DDEVICE2 iface,
                                                 LPD3DENUMTEXTUREFORMATSCALLBACK lpD3DEnumTextureProc,
                                                 LPVOID lpArg)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    FIXME("(%p/%p)->(%p,%p): stub!\n", This, iface, lpD3DEnumTextureProc, lpArg);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_2_Begin(LPDIRECT3DDEVICE2 iface,
                                 D3DPRIMITIVETYPE d3dpt,
                                 D3DVERTEXTYPE dwVertexTypeDesc,
                                 DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    FIXME("(%p/%p)->(%08x,%08x,%08lx): stub!\n", This, iface, d3dpt, dwVertexTypeDesc, dwFlags);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_2_BeginIndexed(LPDIRECT3DDEVICE2 iface,
                                        D3DPRIMITIVETYPE d3dptPrimitiveType,
                                        D3DVERTEXTYPE d3dvtVertexType,
                                        LPVOID lpvVertices,
                                        DWORD dwNumVertices,
                                        DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    FIXME("(%p/%p)->(%08x,%08x,%p,%08lx,%08lx): stub!\n", This, iface, d3dptPrimitiveType, d3dvtVertexType, lpvVertices, dwNumVertices, dwFlags);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_2_DrawPrimitive(LPDIRECT3DDEVICE2 iface,
                                         D3DPRIMITIVETYPE d3dptPrimitiveType,
                                         D3DVERTEXTYPE d3dvtVertexType,
                                         LPVOID lpvVertices,
                                         DWORD dwVertexCount,
                                         DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    FIXME("(%p/%p)->(%08x,%08x,%p,%08lx,%08lx): stub!\n", This, iface, d3dptPrimitiveType, d3dvtVertexType, lpvVertices, dwVertexCount, dwFlags);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_2_DrawIndexedPrimitive(LPDIRECT3DDEVICE2 iface,
                                                D3DPRIMITIVETYPE d3dptPrimitiveType,
                                                D3DVERTEXTYPE d3dvtVertexType,
                                                LPVOID lpvVertices,
                                                DWORD dwVertexCount,
                                                LPWORD dwIndices,
                                                DWORD dwIndexCount,
                                                DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    FIXME("(%p/%p)->(%08x,%08x,%p,%08lx,%p,%08lx,%08lx): stub!\n", This, iface, d3dptPrimitiveType, d3dvtVertexType, lpvVertices, dwVertexCount, dwIndices, dwIndexCount, dwFlags);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_1_Initialize(LPDIRECT3DDEVICE iface,
                                      LPDIRECT3D lpDirect3D,
                                      LPGUID lpGUID,
                                      LPD3DDEVICEDESC lpD3DDVDesc)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice, iface);
    FIXME("(%p/%p)->(%p,%p,%p): stub!\n", This, iface, lpDirect3D, lpGUID, lpD3DDVDesc);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_1_CreateExecuteBuffer(LPDIRECT3DDEVICE iface,
                                               LPD3DEXECUTEBUFFERDESC lpDesc,
                                               LPDIRECT3DEXECUTEBUFFER* lplpDirect3DExecuteBuffer,
                                               IUnknown* pUnkOuter)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice, iface);
    FIXME("(%p/%p)->(%p,%p,%p): stub!\n", This, iface, lpDesc, lplpDirect3DExecuteBuffer, pUnkOuter);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_1_Execute(LPDIRECT3DDEVICE iface,
                                   LPDIRECT3DEXECUTEBUFFER lpDirect3DExecuteBuffer,
                                   LPDIRECT3DVIEWPORT lpDirect3DViewport,
                                   DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice, iface);
    IDirect3DExecuteBufferImpl *lpDirect3DExecuteBufferImpl = ICOM_OBJECT(IDirect3DExecuteBufferImpl, IDirect3DExecuteBuffer, lpDirect3DExecuteBuffer);
    IDirect3DViewportImpl *lpDirect3DViewportImpl = ICOM_OBJECT(IDirect3DViewportImpl, IDirect3DViewport3, lpDirect3DViewport);
    
    TRACE("(%p/%p)->(%p,%p,%08lx)\n", This, iface, lpDirect3DExecuteBuffer, lpDirect3DViewport, dwFlags);

    /* Put this as the default context */

    /* Execute... */
    lpDirect3DExecuteBufferImpl->execute(lpDirect3DExecuteBufferImpl, This, lpDirect3DViewportImpl);

    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_1_NextViewport(LPDIRECT3DDEVICE iface,
                                        LPDIRECT3DVIEWPORT lpDirect3DViewport,
                                        LPDIRECT3DVIEWPORT* lplpDirect3DViewport,
                                        DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice, iface);
    FIXME("(%p/%p)->(%p,%p,%08lx): stub!\n", This, iface, lpDirect3DViewport, lplpDirect3DViewport, dwFlags);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_1_Pick(LPDIRECT3DDEVICE iface,
                                LPDIRECT3DEXECUTEBUFFER lpDirect3DExecuteBuffer,
                                LPDIRECT3DVIEWPORT lpDirect3DViewport,
                                DWORD dwFlags,
                                LPD3DRECT lpRect)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice, iface);
    FIXME("(%p/%p)->(%p,%p,%08lx,%p): stub!\n", This, iface, lpDirect3DExecuteBuffer, lpDirect3DViewport, dwFlags, lpRect);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_1_GetPickRecords(LPDIRECT3DDEVICE iface,
                                          LPDWORD lpCount,
                                          LPD3DPICKRECORD lpD3DPickRec)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice, iface);
    FIXME("(%p/%p)->(%p,%p): stub!\n", This, iface, lpCount, lpD3DPickRec);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_1_CreateMatrix(LPDIRECT3DDEVICE iface,
                                        LPD3DMATRIXHANDLE lpD3DMatHandle)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice, iface);
    TRACE("(%p/%p)->(%p)\n", This, iface, lpD3DMatHandle);

    *lpD3DMatHandle = (D3DMATRIXHANDLE) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(D3DMATRIX));
    TRACE(" returning matrix handle %p\n", (void *) *lpD3DMatHandle);
    
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_1_SetMatrix(LPDIRECT3DDEVICE iface,
                                     D3DMATRIXHANDLE D3DMatHandle,
                                     LPD3DMATRIX lpD3DMatrix)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice, iface);
    TRACE("(%p/%p)->(%08lx,%p)\n", This, iface, (DWORD) D3DMatHandle, lpD3DMatrix);

    if (TRACE_ON(ddraw)) {
	dump_D3DMATRIX(lpD3DMatrix);
    }
    *((D3DMATRIX *) D3DMatHandle) = *lpD3DMatrix;   
    
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_1_GetMatrix(LPDIRECT3DDEVICE iface,
                                     D3DMATRIXHANDLE D3DMatHandle,
                                     LPD3DMATRIX lpD3DMatrix)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice, iface);
    TRACE("(%p/%p)->(%08lx,%p)\n", This, iface, (DWORD) D3DMatHandle, lpD3DMatrix);

    *lpD3DMatrix = *((D3DMATRIX *) D3DMatHandle);
    
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_1_DeleteMatrix(LPDIRECT3DDEVICE iface,
                                        D3DMATRIXHANDLE D3DMatHandle)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice, iface);
    TRACE("(%p/%p)->(%08lx)\n", This, iface, (DWORD) D3DMatHandle);

    HeapFree(GetProcessHeap(), 0, (void *) D3DMatHandle);
    
    return DD_OK;
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_QueryInterface(LPDIRECT3DDEVICE3 iface,
                                           REFIID riid,
                                           LPVOID* obp)
{
    TRACE("(%p)->(%s,%p) thunking to IDirect3DDevice7 interface.\n", iface, debugstr_guid(riid), obp);
    return IDirect3DDevice7_QueryInterface(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice3, IDirect3DDevice7, iface),
                                           riid,
                                           obp);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_QueryInterface(LPDIRECT3DDEVICE2 iface,
                                           REFIID riid,
                                           LPVOID* obp)
{
    TRACE("(%p)->(%s,%p) thunking to IDirect3DDevice7 interface.\n", iface, debugstr_guid(riid), obp);
    return IDirect3DDevice7_QueryInterface(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice2, IDirect3DDevice7, iface),
                                           riid,
                                           obp);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_QueryInterface(LPDIRECT3DDEVICE iface,
                                           REFIID riid,
                                           LPVOID* obp)
{
    TRACE("(%p)->(%s,%p) thunking to IDirect3DDevice7 interface.\n", iface, debugstr_guid(riid), obp);
    return IDirect3DDevice7_QueryInterface(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice, IDirect3DDevice7, iface),
                                           riid,
                                           obp);
}

ULONG WINAPI
Thunk_IDirect3DDeviceImpl_3_AddRef(LPDIRECT3DDEVICE3 iface)
{
    TRACE("(%p)->() thunking to IDirect3DDevice7 interface.\n", iface);
    return IDirect3DDevice7_AddRef(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice3, IDirect3DDevice7, iface));
}

ULONG WINAPI
Thunk_IDirect3DDeviceImpl_2_AddRef(LPDIRECT3DDEVICE2 iface)
{
    TRACE("(%p)->() thunking to IDirect3DDevice7 interface.\n", iface);
    return IDirect3DDevice7_AddRef(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice2, IDirect3DDevice7, iface));
}

ULONG WINAPI
Thunk_IDirect3DDeviceImpl_1_AddRef(LPDIRECT3DDEVICE iface)
{
    TRACE("(%p)->() thunking to IDirect3DDevice7 interface.\n", iface);
    return IDirect3DDevice7_AddRef(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice, IDirect3DDevice7, iface));
}

ULONG WINAPI
Thunk_IDirect3DDeviceImpl_3_Release(LPDIRECT3DDEVICE3 iface)
{
    TRACE("(%p)->() thunking to IDirect3DDevice7 interface.\n", iface);
    return IDirect3DDevice7_Release(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice3, IDirect3DDevice7, iface));
}

ULONG WINAPI
Thunk_IDirect3DDeviceImpl_2_Release(LPDIRECT3DDEVICE2 iface)
{
    TRACE("(%p)->() thunking to IDirect3DDevice7 interface.\n", iface);
    return IDirect3DDevice7_Release(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice2, IDirect3DDevice7, iface));
}

ULONG WINAPI
Thunk_IDirect3DDeviceImpl_1_Release(LPDIRECT3DDEVICE iface)
{
    TRACE("(%p)->() thunking to IDirect3DDevice7 interface.\n", iface);
    return IDirect3DDevice7_Release(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice, IDirect3DDevice7, iface));
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_AddViewport(LPDIRECT3DDEVICE2 iface,
					LPDIRECT3DVIEWPORT2 lpDirect3DViewport2)
{
    TRACE("(%p)->(%p) thunking to IDirect3DDevice3 interface.\n", iface, lpDirect3DViewport2);
    return IDirect3DDevice3_AddViewport(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice2, IDirect3DDevice3, iface),
					(LPDIRECT3DVIEWPORT3) lpDirect3DViewport2 /* No need to cast here as all interfaces are equivalent */);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_AddViewport(LPDIRECT3DDEVICE iface,
					LPDIRECT3DVIEWPORT lpDirect3DViewport)
{
    TRACE("(%p)->(%p) thunking to IDirect3DDevice3 interface.\n", iface, lpDirect3DViewport);
    return IDirect3DDevice3_AddViewport(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice, IDirect3DDevice3, iface),
					(LPDIRECT3DVIEWPORT3) lpDirect3DViewport /* No need to cast here as all interfaces are equivalent */);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_DeleteViewport(LPDIRECT3DDEVICE2 iface,
					   LPDIRECT3DVIEWPORT2 lpDirect3DViewport2)
{
    TRACE("(%p)->(%p) thunking to IDirect3DDevice3 interface.\n", iface, lpDirect3DViewport2);
    return IDirect3DDevice3_DeleteViewport(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice2, IDirect3DDevice3, iface),
					   (LPDIRECT3DVIEWPORT3) lpDirect3DViewport2 /* No need to cast here as all interfaces are equivalent */);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_DeleteViewport(LPDIRECT3DDEVICE iface,
					   LPDIRECT3DVIEWPORT lpDirect3DViewport)
{
    TRACE("(%p)->(%p) thunking to IDirect3DDevice3 interface.\n", iface, lpDirect3DViewport);
    return IDirect3DDevice3_DeleteViewport(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice, IDirect3DDevice3, iface),
					   (LPDIRECT3DVIEWPORT3) lpDirect3DViewport /* No need to cast here as all interfaces are equivalent */);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_NextViewport(LPDIRECT3DDEVICE2 iface,
					 LPDIRECT3DVIEWPORT2 lpDirect3DViewport2,
					 LPDIRECT3DVIEWPORT2* lplpDirect3DViewport2,
					 DWORD dwFlags)
{
    TRACE("(%p)->(%p,%p,%08lx) thunking to IDirect3DDevice3 interface.\n", iface, lpDirect3DViewport2, lplpDirect3DViewport2, dwFlags);
    return IDirect3DDevice3_NextViewport(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice2, IDirect3DDevice3, iface),
					 (LPDIRECT3DVIEWPORT3) lpDirect3DViewport2 /* No need to cast here as all interfaces are equivalent */,
					 (LPDIRECT3DVIEWPORT3*) lplpDirect3DViewport2,
					 dwFlags);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_NextViewport(LPDIRECT3DDEVICE iface,
					 LPDIRECT3DVIEWPORT lpDirect3DViewport,
					 LPDIRECT3DVIEWPORT* lplpDirect3DViewport,
					 DWORD dwFlags)
{
    TRACE("(%p)->(%p,%p,%08lx) thunking to IDirect3DDevice3 interface.\n", iface, lpDirect3DViewport, lplpDirect3DViewport, dwFlags);
    return IDirect3DDevice3_NextViewport(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice, IDirect3DDevice3, iface),
					 (LPDIRECT3DVIEWPORT3) lpDirect3DViewport /* No need to cast here as all interfaces are equivalent */,
					 (LPDIRECT3DVIEWPORT3*) lplpDirect3DViewport,
					 dwFlags);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_GetDirect3D(LPDIRECT3DDEVICE3 iface,
					LPDIRECT3D3* lplpDirect3D3)
{
    HRESULT ret;
    LPDIRECT3D7 ret_ptr;
  
    TRACE("(%p)->(%p) thunking to IDirect3DDevice7 interface.\n", iface, lplpDirect3D3);
    ret = IDirect3DDevice7_GetDirect3D(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice3, IDirect3DDevice7, iface),
				       &ret_ptr);
    *lplpDirect3D3 = COM_INTERFACE_CAST(IDirectDrawImpl, IDirect3D7, IDirect3D3, ret_ptr);
    TRACE(" returning interface %p\n", *lplpDirect3D3);
    return ret;
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetDirect3D(LPDIRECT3DDEVICE2 iface,
					LPDIRECT3D2* lplpDirect3D2)
{
    HRESULT ret;
    LPDIRECT3D7 ret_ptr;
  
    TRACE("(%p)->(%p) thunking to IDirect3DDevice7 interface.\n", iface, lplpDirect3D2);
    ret = IDirect3DDevice7_GetDirect3D(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice2, IDirect3DDevice7, iface),
				       &ret_ptr);
    *lplpDirect3D2 = COM_INTERFACE_CAST(IDirectDrawImpl, IDirect3D7, IDirect3D2, ret_ptr);
    TRACE(" returning interface %p\n", *lplpDirect3D2);
    return ret;
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_GetDirect3D(LPDIRECT3DDEVICE iface,
					LPDIRECT3D* lplpDirect3D)
{
    HRESULT ret;
    LPDIRECT3D7 ret_ptr;
  
    TRACE("(%p)->(%p) thunking to IDirect3DDevice7 interface.\n", iface, lplpDirect3D);
    ret = IDirect3DDevice7_GetDirect3D(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice, IDirect3DDevice7, iface),
				       &ret_ptr);
    *lplpDirect3D = COM_INTERFACE_CAST(IDirectDrawImpl, IDirect3D7, IDirect3D, ret_ptr);
    TRACE(" returning interface %p\n", *lplpDirect3D);
    return ret;
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_SetCurrentViewport(LPDIRECT3DDEVICE2 iface,
					       LPDIRECT3DVIEWPORT2 lpDirect3DViewport2)
{
    TRACE("(%p)->(%p) thunking to IDirect3DDevice3 interface.\n", iface, lpDirect3DViewport2);
    return IDirect3DDevice3_SetCurrentViewport(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice2, IDirect3DDevice3, iface),
					       (LPDIRECT3DVIEWPORT3) lpDirect3DViewport2 /* No need to cast here as all interfaces are equivalent */);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetCurrentViewport(LPDIRECT3DDEVICE2 iface,
					       LPDIRECT3DVIEWPORT2* lplpDirect3DViewport2)
{
    TRACE("(%p)->(%p) thunking to IDirect3DDevice3 interface.\n", iface, lplpDirect3DViewport2);
    return IDirect3DDevice3_GetCurrentViewport(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice2, IDirect3DDevice3, iface),
					       (LPDIRECT3DVIEWPORT3*) lplpDirect3DViewport2 /* No need to cast here as all interfaces are equivalent */);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_EnumTextureFormats(LPDIRECT3DDEVICE3 iface,
                                               LPD3DENUMPIXELFORMATSCALLBACK lpD3DEnumPixelProc,
                                               LPVOID lpArg)
{
    TRACE("(%p)->(%p,%p) thunking to IDirect3DDevice7 interface.\n", iface, lpD3DEnumPixelProc, lpArg);
    return IDirect3DDevice7_EnumTextureFormats(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice3, IDirect3DDevice7, iface),
                                               lpD3DEnumPixelProc,
                                               lpArg);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_BeginScene(LPDIRECT3DDEVICE3 iface)
{
    TRACE("(%p)->() thunking to IDirect3DDevice7 interface.\n", iface);
    return IDirect3DDevice7_BeginScene(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice3, IDirect3DDevice7, iface));
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_BeginScene(LPDIRECT3DDEVICE2 iface)
{
    TRACE("(%p)->() thunking to IDirect3DDevice7 interface.\n", iface);
    return IDirect3DDevice7_BeginScene(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice2, IDirect3DDevice7, iface));
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_BeginScene(LPDIRECT3DDEVICE iface)
{
    TRACE("(%p)->() thunking to IDirect3DDevice7 interface.\n", iface);
    return IDirect3DDevice7_BeginScene(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice, IDirect3DDevice7, iface));
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_EndScene(LPDIRECT3DDEVICE3 iface)
{
    TRACE("(%p)->() thunking to IDirect3DDevice7 interface.\n", iface);
    return IDirect3DDevice7_EndScene(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice3, IDirect3DDevice7, iface));
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_EndScene(LPDIRECT3DDEVICE2 iface)
{
    TRACE("(%p)->() thunking to IDirect3DDevice7 interface.\n", iface);
    return IDirect3DDevice7_EndScene(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice2, IDirect3DDevice7, iface));
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_EndScene(LPDIRECT3DDEVICE iface)
{
    TRACE("(%p)->() thunking to IDirect3DDevice7 interface.\n", iface);
    return IDirect3DDevice7_EndScene(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice, IDirect3DDevice7, iface));
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_SetTransform(LPDIRECT3DDEVICE3 iface,
                                         D3DTRANSFORMSTATETYPE dtstTransformStateType,
                                         LPD3DMATRIX lpD3DMatrix)
{
    TRACE("(%p)->(%08x,%p) thunking to IDirect3DDevice7 interface.\n", iface, dtstTransformStateType, lpD3DMatrix);
    return IDirect3DDevice7_SetTransform(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice3, IDirect3DDevice7, iface),
                                         dtstTransformStateType,
                                         lpD3DMatrix);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_SetTransform(LPDIRECT3DDEVICE2 iface,
                                         D3DTRANSFORMSTATETYPE dtstTransformStateType,
                                         LPD3DMATRIX lpD3DMatrix)
{
    TRACE("(%p)->(%08x,%p) thunking to IDirect3DDevice7 interface.\n", iface, dtstTransformStateType, lpD3DMatrix);
    return IDirect3DDevice7_SetTransform(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice2, IDirect3DDevice7, iface),
                                         dtstTransformStateType,
                                         lpD3DMatrix);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_GetTransform(LPDIRECT3DDEVICE3 iface,
                                         D3DTRANSFORMSTATETYPE dtstTransformStateType,
                                         LPD3DMATRIX lpD3DMatrix)
{
    TRACE("(%p)->(%08x,%p) thunking to IDirect3DDevice7 interface.\n", iface, dtstTransformStateType, lpD3DMatrix);
    return IDirect3DDevice7_GetTransform(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice3, IDirect3DDevice7, iface),
                                         dtstTransformStateType,
                                         lpD3DMatrix);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetTransform(LPDIRECT3DDEVICE2 iface,
                                         D3DTRANSFORMSTATETYPE dtstTransformStateType,
                                         LPD3DMATRIX lpD3DMatrix)
{
    TRACE("(%p)->(%08x,%p) thunking to IDirect3DDevice7 interface.\n", iface, dtstTransformStateType, lpD3DMatrix);
    return IDirect3DDevice7_GetTransform(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice2, IDirect3DDevice7, iface),
                                         dtstTransformStateType,
                                         lpD3DMatrix);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_MultiplyTransform(LPDIRECT3DDEVICE3 iface,
                                              D3DTRANSFORMSTATETYPE dtstTransformStateType,
                                              LPD3DMATRIX lpD3DMatrix)
{
    TRACE("(%p)->(%08x,%p) thunking to IDirect3DDevice7 interface.\n", iface, dtstTransformStateType, lpD3DMatrix);
    return IDirect3DDevice7_MultiplyTransform(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice3, IDirect3DDevice7, iface),
                                              dtstTransformStateType,
                                              lpD3DMatrix);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_MultiplyTransform(LPDIRECT3DDEVICE2 iface,
                                              D3DTRANSFORMSTATETYPE dtstTransformStateType,
                                              LPD3DMATRIX lpD3DMatrix)
{
    TRACE("(%p)->(%08x,%p) thunking to IDirect3DDevice7 interface.\n", iface, dtstTransformStateType, lpD3DMatrix);
    return IDirect3DDevice7_MultiplyTransform(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice2, IDirect3DDevice7, iface),
                                              dtstTransformStateType,
                                              lpD3DMatrix);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_SetRenderState(LPDIRECT3DDEVICE3 iface,
                                           D3DRENDERSTATETYPE dwRenderStateType,
                                           DWORD dwRenderState)
{
    TRACE("(%p)->(%08x,%08lx) thunking to IDirect3DDevice7 interface.\n", iface, dwRenderStateType, dwRenderState);
    return IDirect3DDevice7_SetRenderState(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice3, IDirect3DDevice7, iface),
                                           dwRenderStateType,
                                           dwRenderState);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_SetRenderState(LPDIRECT3DDEVICE2 iface,
                                           D3DRENDERSTATETYPE dwRenderStateType,
                                           DWORD dwRenderState)
{
    TRACE("(%p)->(%08x,%08lx) thunking to IDirect3DDevice7 interface.\n", iface, dwRenderStateType, dwRenderState);
    return IDirect3DDevice7_SetRenderState(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice2, IDirect3DDevice7, iface),
                                           dwRenderStateType,
                                           dwRenderState);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_GetRenderState(LPDIRECT3DDEVICE3 iface,
                                           D3DRENDERSTATETYPE dwRenderStateType,
                                           LPDWORD lpdwRenderState)
{
    TRACE("(%p)->(%08x,%p) thunking to IDirect3DDevice7 interface.\n", iface, dwRenderStateType, lpdwRenderState);
    return IDirect3DDevice7_GetRenderState(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice3, IDirect3DDevice7, iface),
                                           dwRenderStateType,
                                           lpdwRenderState);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetRenderState(LPDIRECT3DDEVICE2 iface,
                                           D3DRENDERSTATETYPE dwRenderStateType,
                                           LPDWORD lpdwRenderState)
{
    TRACE("(%p)->(%08x,%p) thunking to IDirect3DDevice7 interface.\n", iface, dwRenderStateType, lpdwRenderState);
    return IDirect3DDevice7_GetRenderState(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice2, IDirect3DDevice7, iface),
                                           dwRenderStateType,
                                           lpdwRenderState);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_DrawPrimitive(LPDIRECT3DDEVICE3 iface,
                                          D3DPRIMITIVETYPE d3dptPrimitiveType,
                                          DWORD d3dvtVertexType,
                                          LPVOID lpvVertices,
                                          DWORD dwVertexCount,
                                          DWORD dwFlags)
{
    TRACE("(%p)->(%08x,%08lx,%p,%08lx,%08lx) thunking to IDirect3DDevice7 interface.\n", iface, d3dptPrimitiveType, d3dvtVertexType, lpvVertices, dwVertexCount, dwFlags);
    return IDirect3DDevice7_DrawPrimitive(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice3, IDirect3DDevice7, iface),
                                          d3dptPrimitiveType,
                                          d3dvtVertexType,
                                          lpvVertices,
                                          dwVertexCount,
                                          dwFlags);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_DrawIndexedPrimitive(LPDIRECT3DDEVICE3 iface,
                                                 D3DPRIMITIVETYPE d3dptPrimitiveType,
                                                 DWORD d3dvtVertexType,
                                                 LPVOID lpvVertices,
                                                 DWORD dwVertexCount,
                                                 LPWORD dwIndices,
                                                 DWORD dwIndexCount,
                                                 DWORD dwFlags)
{
    TRACE("(%p)->(%08x,%08lx,%p,%08lx,%p,%08lx,%08lx) thunking to IDirect3DDevice7 interface.\n", iface, d3dptPrimitiveType, d3dvtVertexType, lpvVertices, dwVertexCount, dwIndices, dwIndexCount, dwFlags);
    return IDirect3DDevice7_DrawIndexedPrimitive(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice3, IDirect3DDevice7, iface),
                                                 d3dptPrimitiveType,
                                                 d3dvtVertexType,
                                                 lpvVertices,
                                                 dwVertexCount,
                                                 dwIndices,
                                                 dwIndexCount,
                                                 dwFlags);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_SetClipStatus(LPDIRECT3DDEVICE3 iface,
                                          LPD3DCLIPSTATUS lpD3DClipStatus)
{
    TRACE("(%p)->(%p) thunking to IDirect3DDevice7 interface.\n", iface, lpD3DClipStatus);
    return IDirect3DDevice7_SetClipStatus(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice3, IDirect3DDevice7, iface),
                                          lpD3DClipStatus);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_SetClipStatus(LPDIRECT3DDEVICE2 iface,
                                          LPD3DCLIPSTATUS lpD3DClipStatus)
{
    TRACE("(%p)->(%p) thunking to IDirect3DDevice7 interface.\n", iface, lpD3DClipStatus);
    return IDirect3DDevice7_SetClipStatus(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice2, IDirect3DDevice7, iface),
                                          lpD3DClipStatus);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_GetClipStatus(LPDIRECT3DDEVICE3 iface,
                                          LPD3DCLIPSTATUS lpD3DClipStatus)
{
    TRACE("(%p)->(%p) thunking to IDirect3DDevice7 interface.\n", iface, lpD3DClipStatus);
    return IDirect3DDevice7_GetClipStatus(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice3, IDirect3DDevice7, iface),
                                          lpD3DClipStatus);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetClipStatus(LPDIRECT3DDEVICE2 iface,
                                          LPD3DCLIPSTATUS lpD3DClipStatus)
{
    TRACE("(%p)->(%p) thunking to IDirect3DDevice7 interface.\n", iface, lpD3DClipStatus);
    return IDirect3DDevice7_GetClipStatus(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice2, IDirect3DDevice7, iface),
                                          lpD3DClipStatus);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_DrawPrimitiveStrided(LPDIRECT3DDEVICE3 iface,
                                                 D3DPRIMITIVETYPE d3dptPrimitiveType,
                                                 DWORD dwVertexType,
                                                 LPD3DDRAWPRIMITIVESTRIDEDDATA lpD3DDrawPrimStrideData,
                                                 DWORD dwVertexCount,
                                                 DWORD dwFlags)
{
    TRACE("(%p)->(%08x,%08lx,%p,%08lx,%08lx) thunking to IDirect3DDevice7 interface.\n", iface, d3dptPrimitiveType, dwVertexType, lpD3DDrawPrimStrideData, dwVertexCount, dwFlags);
    return IDirect3DDevice7_DrawPrimitiveStrided(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice3, IDirect3DDevice7, iface),
                                                 d3dptPrimitiveType,
                                                 dwVertexType,
                                                 lpD3DDrawPrimStrideData,
                                                 dwVertexCount,
                                                 dwFlags);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_DrawIndexedPrimitiveStrided(LPDIRECT3DDEVICE3 iface,
                                                        D3DPRIMITIVETYPE d3dptPrimitiveType,
                                                        DWORD dwVertexType,
                                                        LPD3DDRAWPRIMITIVESTRIDEDDATA lpD3DDrawPrimStrideData,
                                                        DWORD dwVertexCount,
                                                        LPWORD lpIndex,
                                                        DWORD dwIndexCount,
                                                        DWORD dwFlags)
{
    TRACE("(%p)->(%08x,%08lx,%p,%08lx,%p,%08lx,%08lx) thunking to IDirect3DDevice7 interface.\n", iface, d3dptPrimitiveType, dwVertexType, lpD3DDrawPrimStrideData, dwVertexCount, lpIndex, dwIndexCount, dwFlags);
    return IDirect3DDevice7_DrawIndexedPrimitiveStrided(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice3, IDirect3DDevice7, iface),
                                                        d3dptPrimitiveType,
                                                        dwVertexType,
                                                        lpD3DDrawPrimStrideData,
                                                        dwVertexCount,
                                                        lpIndex,
                                                        dwIndexCount,
                                                        dwFlags);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_ComputeSphereVisibility(LPDIRECT3DDEVICE3 iface,
                                                    LPD3DVECTOR lpCenters,
                                                    LPD3DVALUE lpRadii,
                                                    DWORD dwNumSpheres,
                                                    DWORD dwFlags,
                                                    LPDWORD lpdwReturnValues)
{
    TRACE("(%p)->(%p,%p,%08lx,%08lx,%p) thunking to IDirect3DDevice7 interface.\n", iface, lpCenters, lpRadii, dwNumSpheres, dwFlags, lpdwReturnValues);
    return IDirect3DDevice7_ComputeSphereVisibility(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice3, IDirect3DDevice7, iface),
                                                    lpCenters,
                                                    lpRadii,
                                                    dwNumSpheres,
                                                    dwFlags,
                                                    lpdwReturnValues);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_GetTextureStageState(LPDIRECT3DDEVICE3 iface,
                                                 DWORD dwStage,
                                                 D3DTEXTURESTAGESTATETYPE d3dTexStageStateType,
                                                 LPDWORD lpdwState)
{
    TRACE("(%p)->(%08lx,%08x,%p) thunking to IDirect3DDevice7 interface.\n", iface, dwStage, d3dTexStageStateType, lpdwState);
    return IDirect3DDevice7_GetTextureStageState(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice3, IDirect3DDevice7, iface),
                                                 dwStage,
                                                 d3dTexStageStateType,
                                                 lpdwState);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_SetTextureStageState(LPDIRECT3DDEVICE3 iface,
                                                 DWORD dwStage,
                                                 D3DTEXTURESTAGESTATETYPE d3dTexStageStateType,
                                                 DWORD dwState)
{
    TRACE("(%p)->(%08lx,%08x,%08lx) thunking to IDirect3DDevice7 interface.\n", iface, dwStage, d3dTexStageStateType, dwState);
    return IDirect3DDevice7_SetTextureStageState(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice3, IDirect3DDevice7, iface),
                                                 dwStage,
                                                 d3dTexStageStateType,
                                                 dwState);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_ValidateDevice(LPDIRECT3DDEVICE3 iface,
                                           LPDWORD lpdwPasses)
{
    TRACE("(%p)->(%p) thunking to IDirect3DDevice7 interface.\n", iface, lpdwPasses);
    return IDirect3DDevice7_ValidateDevice(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice3, IDirect3DDevice7, iface),
                                           lpdwPasses);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetCaps(LPDIRECT3DDEVICE2 iface,
                                    LPD3DDEVICEDESC lpD3DHWDevDesc,
                                    LPD3DDEVICEDESC lpD3DHELDevDesc)
{
    TRACE("(%p)->(%p,%p) thunking to IDirect3DDevice3 interface.\n", iface, lpD3DHWDevDesc, lpD3DHELDevDesc);
    return IDirect3DDevice3_GetCaps(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice2, IDirect3DDevice3, iface),
                                    lpD3DHWDevDesc,
                                    lpD3DHELDevDesc);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_GetCaps(LPDIRECT3DDEVICE iface,
                                    LPD3DDEVICEDESC lpD3DHWDevDesc,
                                    LPD3DDEVICEDESC lpD3DHELDevDesc)
{
    TRACE("(%p)->(%p,%p) thunking to IDirect3DDevice3 interface.\n", iface, lpD3DHWDevDesc, lpD3DHELDevDesc);
    return IDirect3DDevice3_GetCaps(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice, IDirect3DDevice3, iface),
                                    lpD3DHWDevDesc,
                                    lpD3DHELDevDesc);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_SwapTextureHandles(LPDIRECT3DDEVICE iface,
                                              LPDIRECT3DTEXTURE lpD3DTex1,
                                              LPDIRECT3DTEXTURE lpD3DTex2)
{
    TRACE("(%p)->(%p,%p) thunking to IDirect3DDevice2 interface.\n", iface, lpD3DTex1, lpD3DTex2);
    return IDirect3DDevice2_SwapTextureHandles(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice, IDirect3DDevice2, iface),
                                               COM_INTERFACE_CAST(IDirectDrawSurfaceImpl, IDirect3DTexture, IDirect3DTexture2, lpD3DTex1),
                                               COM_INTERFACE_CAST(IDirectDrawSurfaceImpl, IDirect3DTexture, IDirect3DTexture2, lpD3DTex2));
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetStats(LPDIRECT3DDEVICE2 iface,
                                     LPD3DSTATS lpD3DStats)
{
    TRACE("(%p)->(%p) thunking to IDirect3DDevice3 interface.\n", iface, lpD3DStats);
    return IDirect3DDevice3_GetStats(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice2, IDirect3DDevice3, iface),
                                     lpD3DStats);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_GetStats(LPDIRECT3DDEVICE iface,
                                     LPD3DSTATS lpD3DStats)
{
    TRACE("(%p)->(%p) thunking to IDirect3DDevice3 interface.\n", iface, lpD3DStats);
    return IDirect3DDevice3_GetStats(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice, IDirect3DDevice3, iface),
                                     lpD3DStats);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_SetRenderTarget(LPDIRECT3DDEVICE3 iface,
                                            LPDIRECTDRAWSURFACE4 lpNewRenderTarget,
                                            DWORD dwFlags)
{
    TRACE("(%p)->(%p,%08lx) thunking to IDirect3DDevice7 interface.\n", iface, lpNewRenderTarget, dwFlags);
    return IDirect3DDevice7_SetRenderTarget(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice3, IDirect3DDevice7, iface),
                                            (LPDIRECTDRAWSURFACE7) lpNewRenderTarget /* No cast needed as DSurf4 == DSurf7 */,
                                            dwFlags);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_GetRenderTarget(LPDIRECT3DDEVICE3 iface,
                                            LPDIRECTDRAWSURFACE4* lplpRenderTarget)
{
    TRACE("(%p)->(%p) thunking to IDirect3DDevice7 interface.\n", iface, lplpRenderTarget);
    return IDirect3DDevice7_GetRenderTarget(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice3, IDirect3DDevice7, iface),
                                            (LPDIRECTDRAWSURFACE7*) lplpRenderTarget /* No cast needed as DSurf4 == DSurf7 */);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_SetRenderTarget(LPDIRECT3DDEVICE2 iface,
                                            LPDIRECTDRAWSURFACE lpNewRenderTarget,
                                            DWORD dwFlags)
{
    TRACE("(%p)->(%p,%08lx) thunking to IDirect3DDevice7 interface.\n", iface, lpNewRenderTarget, dwFlags);
    return IDirect3DDevice7_SetRenderTarget(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice2, IDirect3DDevice7, iface),
                                            COM_INTERFACE_CAST(IDirectDrawSurfaceImpl, IDirectDrawSurface3, IDirectDrawSurface7, lpNewRenderTarget),
                                            dwFlags);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetRenderTarget(LPDIRECT3DDEVICE2 iface,
                                            LPDIRECTDRAWSURFACE* lplpRenderTarget)
{
    HRESULT ret;
    LPDIRECTDRAWSURFACE7 ret_val;
  
    TRACE("(%p)->(%p) thunking to IDirect3DDevice7 interface.\n", iface, lplpRenderTarget);
    ret = IDirect3DDevice7_GetRenderTarget(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice2, IDirect3DDevice7, iface),
					   &ret_val);
    *lplpRenderTarget = (LPDIRECTDRAWSURFACE) COM_INTERFACE_CAST(IDirectDrawSurfaceImpl, IDirectDrawSurface7, IDirectDrawSurface3, ret_val);
    TRACE(" returning interface %p\n", *lplpRenderTarget);
    return ret;
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_Vertex(LPDIRECT3DDEVICE2 iface,
                                   LPVOID lpVertexType)
{
    TRACE("(%p)->(%p) thunking to IDirect3DDevice3 interface.\n", iface, lpVertexType);
    return IDirect3DDevice3_Vertex(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice2, IDirect3DDevice3, iface),
                                   lpVertexType);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_Index(LPDIRECT3DDEVICE2 iface,
                                  WORD wVertexIndex)
{
    TRACE("(%p)->(%04x) thunking to IDirect3DDevice3 interface.\n", iface, wVertexIndex);
    return IDirect3DDevice3_Index(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice2, IDirect3DDevice3, iface),
                                  wVertexIndex);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_End(LPDIRECT3DDEVICE2 iface,
                                DWORD dwFlags)
{
    TRACE("(%p)->(%08lx) thunking to IDirect3DDevice3 interface.\n", iface, dwFlags);
    return IDirect3DDevice3_End(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice2, IDirect3DDevice3, iface),
                                dwFlags);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetLightState(LPDIRECT3DDEVICE2 iface,
                                          D3DLIGHTSTATETYPE dwLightStateType,
                                          LPDWORD lpdwLightState)
{
    TRACE("(%p)->(%08x,%p) thunking to IDirect3DDevice3 interface.\n", iface, dwLightStateType, lpdwLightState);
    return IDirect3DDevice3_GetLightState(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice2, IDirect3DDevice3, iface),
                                          dwLightStateType,
                                          lpdwLightState);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_SetLightState(LPDIRECT3DDEVICE2 iface,
                                          D3DLIGHTSTATETYPE dwLightStateType,
                                          DWORD dwLightState)
{
    TRACE("(%p)->(%08x,%08lx) thunking to IDirect3DDevice3 interface.\n", iface, dwLightStateType, dwLightState);
    return IDirect3DDevice3_SetLightState(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice2, IDirect3DDevice3, iface),
                                          dwLightStateType,
                                          dwLightState);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_EnumTextureFormats(LPDIRECT3DDEVICE iface,
                                               LPD3DENUMTEXTUREFORMATSCALLBACK lpD3DEnumTextureProc,
                                               LPVOID lpArg)
{
    TRACE("(%p)->(%p,%p) thunking to IDirect3DDevice2 interface.\n", iface, lpD3DEnumTextureProc, lpArg);
    return IDirect3DDevice2_EnumTextureFormats(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice, IDirect3DDevice2, iface),
                                               lpD3DEnumTextureProc,
                                               lpArg);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_SetTexture(LPDIRECT3DDEVICE3 iface,
				       DWORD dwStage,
				       LPDIRECT3DTEXTURE2 lpTexture2)
{
    TRACE("(%p)->(%ld,%p) thunking to IDirect3DDevice7 interface.\n", iface, dwStage, lpTexture2);
    return IDirect3DDevice7_SetTexture(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice3, IDirect3DDevice7, iface),
				       dwStage,
				       COM_INTERFACE_CAST(IDirectDrawSurfaceImpl, IDirect3DTexture2, IDirectDrawSurface7, lpTexture2));
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_DrawPrimitiveVB(LPDIRECT3DDEVICE3 iface,
					    D3DPRIMITIVETYPE d3dptPrimitiveType,
					    LPDIRECT3DVERTEXBUFFER lpD3DVertexBuf,
					    DWORD dwStartVertex,
					    DWORD dwNumVertices,
					    DWORD dwFlags)
{
    TRACE("(%p)->(%08x,%p,%08lx,%08lx,%08lx) thunking to IDirect3DDevice7 interface.\n", iface, 
	  d3dptPrimitiveType, lpD3DVertexBuf, dwStartVertex, dwNumVertices, dwFlags);
    return IDirect3DDevice7_DrawPrimitiveVB(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice3, IDirect3DDevice7, iface),
					    d3dptPrimitiveType,
					    COM_INTERFACE_CAST(IDirect3DVertexBufferImpl, IDirect3DVertexBuffer, IDirect3DVertexBuffer7, lpD3DVertexBuf),
					    dwStartVertex,
					    dwNumVertices,
					    dwFlags);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_DrawIndexedPrimitiveVB(LPDIRECT3DDEVICE3 iface,
						   D3DPRIMITIVETYPE d3dptPrimitiveType,
						   LPDIRECT3DVERTEXBUFFER lpD3DVertexBuf,
						   LPWORD lpwIndices,
						   DWORD dwIndexCount,
						   DWORD dwFlags)
{
    TRACE("(%p)->(%08x,%p,%p,%08lx,%08lx) thunking to IDirect3DDevice7 interface.\n", iface,
	  d3dptPrimitiveType, lpD3DVertexBuf, lpwIndices, dwIndexCount, dwFlags);
    return IDirect3DDevice7_DrawIndexedPrimitiveVB(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice3, IDirect3DDevice7, iface),
						   d3dptPrimitiveType,
						   COM_INTERFACE_CAST(IDirect3DVertexBufferImpl, IDirect3DVertexBuffer, IDirect3DVertexBuffer7, lpD3DVertexBuf),
						   0,
						   dwIndexCount,
						   lpwIndices,
						   dwIndexCount,
						   dwFlags);
}

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_GetTexture(LPDIRECT3DDEVICE3 iface,
				       DWORD dwStage,
				       LPDIRECT3DTEXTURE2* lplpTexture2)
{
    HRESULT ret;
    LPDIRECTDRAWSURFACE7 ret_val;

    TRACE("(%p)->(%ld,%p) thunking to IDirect3DDevice7 interface.\n", iface, dwStage, lplpTexture2);
    ret = IDirect3DDevice7_GetTexture(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice3, IDirect3DDevice7, iface),
				      dwStage,
				      &ret_val);

    *lplpTexture2 = COM_INTERFACE_CAST(IDirectDrawSurfaceImpl, IDirectDrawSurface7, IDirect3DTexture2, ret_val);

    TRACE(" returning interface %p.\n", *lplpTexture2);
    
    return ret;
}
