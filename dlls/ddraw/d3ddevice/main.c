/* Direct3D Device
 * Copyright (c) 1998 Lionel ULMER
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

#include <string.h>

#include "windef.h"
#include "winerror.h"
#include "objbase.h"
#include "ddraw.h"
#include "d3d.h"
#include "wine/debug.h"

#include "d3d_private.h"
#include "main.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_1T_QueryInterface(LPDIRECT3DDEVICE7 iface,
                                                   REFIID riid,
                                                   LPVOID* obp)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p/%p)->(%s,%p)\n", This, iface, debugstr_guid(riid), obp);

    *obp = NULL;

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
    if ( IsEqualGUID( &IID_IDirect3DDevice2, riid ) ) {
        IDirect3DDevice7_AddRef(ICOM_INTERFACE(This, IDirect3DDevice7));
        *obp = ICOM_INTERFACE(This, IDirect3DDevice2);
	TRACE("  Creating IDirect3DDevice2 interface %p\n", *obp);
	return S_OK;
    }
    if ( IsEqualGUID( &IID_IDirect3DDevice3, riid ) ) {
        IDirect3DDevice7_AddRef(ICOM_INTERFACE(This, IDirect3DDevice7));
        *obp = ICOM_INTERFACE(This, IDirect3DDevice3);
	TRACE("  Creating IDirect3DDevice3 interface %p\n", *obp);
	return S_OK;
    }
    if ( IsEqualGUID( &IID_IDirect3DDevice7, riid ) ) {
        IDirect3DDevice7_AddRef(ICOM_INTERFACE(This, IDirect3DDevice7));
        *obp = ICOM_INTERFACE(This, IDirect3DDevice7);
	TRACE("  Creating IDirect3DDevice7 interface %p\n", *obp);
	return S_OK;
    }
    FIXME("(%p): interface for IID %s NOT found!\n", This, debugstr_guid(riid));
    return OLE_E_ENUM_NOMORE;
}

ULONG WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_1T_AddRef(LPDIRECT3DDEVICE7 iface)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p/%p)->() incrementing from %lu.\n", This, iface, This->ref);
    return ++(This->ref);
}

ULONG WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_1T_Release(LPDIRECT3DDEVICE7 iface)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p/%p)->() decrementing from %lu.\n", This, iface, This->ref);
    if (!--(This->ref)) {
        int i;
	/* Release texture associated with the device */
	for (i = 0; i < MAX_TEXTURES; i++) {
	    if (This->current_texture[i] != NULL)
	        IDirect3DTexture2_Release(ICOM_INTERFACE(This->current_texture[i], IDirect3DTexture2));
	}
	    	  
	HeapFree(GetProcessHeap(), 0, This);
	return 0;
    }
    return This->ref;
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
						LPDIRECT3D7* lplpDirect3D3)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p/%p)->(%p)\n", This, iface, lplpDirect3D3);

    *lplpDirect3D3 = ICOM_INTERFACE(This->d3d, IDirect3D7);
    IDirect3D7_AddRef(ICOM_INTERFACE(This->d3d, IDirect3D7));
    
    TRACE(" returning interface %p\n", *lplpDirect3D3);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_SetRenderTarget(LPDIRECT3DDEVICE7 iface,
						 LPDIRECTDRAWSURFACE7 lpNewRenderTarget,
						 DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p/%p)->(%p,%08lx): stub!\n", This, iface, lpNewRenderTarget, dwFlags);
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
    FIXME("(%p/%p)->(%08lx,%p,%08lx,%08lx,%f,%08lx): stub!\n", This, iface, dwCount, lpRects, dwFlags, (DWORD) dwColor, dvZ, dwStencil);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_SetTransform(LPDIRECT3DDEVICE7 iface,
                                              D3DTRANSFORMSTATETYPE dtstTransformStateType,
                                              LPD3DMATRIX lpD3DMatrix)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p/%p)->(%08x,%p): stub!\n", This, iface, dtstTransformStateType, lpD3DMatrix);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_GetTransform(LPDIRECT3DDEVICE7 iface,
                                              D3DTRANSFORMSTATETYPE dtstTransformStateType,
                                              LPD3DMATRIX lpD3DMatrix)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p/%p)->(%08x,%p): stub!\n", This, iface, dtstTransformStateType, lpD3DMatrix);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_SetViewport(LPDIRECT3DDEVICE7 iface,
                                       LPD3DVIEWPORT7 lpData)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p/%p)->(%p)\n", This, iface, lpData);

    if (TRACE_ON(ddraw)) {
        TRACE(" viewport is : \n");
	TRACE("    - dwX = %ld   dwY = %ld\n",
	      lpData->dwX, lpData->dwY);
	TRACE("    - dwWidth = %ld   dwHeight = %ld\n",
	      lpData->dwWidth, lpData->dwHeight);
	TRACE("    - dvMinZ = %f   dvMaxZ = %f\n",
	      lpData->dvMinZ, lpData->dvMaxZ);
    }
    This->active_viewport = *lpData;

    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_MultiplyTransform(LPDIRECT3DDEVICE7 iface,
                                                   D3DTRANSFORMSTATETYPE dtstTransformStateType,
                                                   LPD3DMATRIX lpD3DMatrix)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p/%p)->(%08x,%p): stub!\n", This, iface, dtstTransformStateType, lpD3DMatrix);
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
        TRACE(" returning viewport : \n");
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
    FIXME("(%p/%p)->(%p): stub!\n", This, iface, lpMat);
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
    FIXME("(%p/%p)->(%08lx,%p): stub!\n", This, iface, dwLightIndex, lpLight);
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
Main_IDirect3DDeviceImpl_7_DrawPrimitiveVB(LPDIRECT3DDEVICE7 iface,
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
Main_IDirect3DDeviceImpl_7_DrawIndexedPrimitiveVB(LPDIRECT3DDEVICE7 iface,
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
Main_IDirect3DDeviceImpl_7_GetTexture(LPDIRECT3DDEVICE7 iface,
                                      DWORD dwStage,
                                      LPDIRECTDRAWSURFACE7* lpTexture)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p/%p)->(%08lx,%p): stub!\n", This, iface, dwStage, lpTexture);
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
    FIXME("(%p/%p)->(%08lx,%08x,%p): stub!\n", This, iface, dwStage, d3dTexStageStateType, lpdwState);
    return DD_OK;
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
    FIXME("(%p/%p)->(%p): stub!\n", This, iface, lpdwPasses);
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
    FIXME("(%p/%p)->(%p,%p,%p,%p,%08lx): stub!\n", This, iface, lpDestTex, lpDestPoint, lpSrcTex, lprcSrcRect, dwFlags);
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
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p/%p)->(%08lx,%p): stub!\n", This, iface, dwLightIndex, pbEnable);
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

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_GetClipPlane(LPDIRECT3DDEVICE7 iface,
                                        DWORD dwIndex,
                                        D3DVALUE* pPlaneEquation)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p/%p)->(%08lx,%p): stub!\n", This, iface, dwIndex, pPlaneEquation);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_GetInfo(LPDIRECT3DDEVICE7 iface,
                                   DWORD dwDevInfoID,
                                   LPVOID pDevInfoStruct,
                                   DWORD dwSize)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p/%p)->(%08lx,%p,%08lx): stub!\n", This, iface, dwDevInfoID, pDevInfoStruct, dwSize);
    return DD_OK;
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

    /* Should check if the viewport was added or not */

    /* Set this viewport as the current viewport */
    This->current_viewport = ICOM_OBJECT(IDirect3DViewportImpl, IDirect3DViewport3, lpDirect3DViewport3);

    /* Activate this viewport */
    This->current_viewport->active_device = This;
    This->current_viewport->activate(This->current_viewport);    
    
    /* And copy the values in the structure used by the device */
    if (This->current_viewport->use_vp2) {
        This->active_viewport.dwX = This->current_viewport->viewports.vp2.dwX;
	This->active_viewport.dwY = This->current_viewport->viewports.vp2.dwY;
	This->active_viewport.dwHeight = This->current_viewport->viewports.vp2.dwHeight;
	This->active_viewport.dwWidth = This->current_viewport->viewports.vp2.dwWidth;
	This->active_viewport.dvMinZ = This->current_viewport->viewports.vp2.dvMinZ;
	This->active_viewport.dvMaxZ = This->current_viewport->viewports.vp2.dvMaxZ;
    } else {
        This->active_viewport.dwX = This->current_viewport->viewports.vp1.dwX;
	This->active_viewport.dwY = This->current_viewport->viewports.vp1.dwY;
	This->active_viewport.dwHeight = This->current_viewport->viewports.vp1.dwHeight;
	This->active_viewport.dwWidth = This->current_viewport->viewports.vp1.dwWidth;
	This->active_viewport.dvMinZ = This->current_viewport->viewports.vp1.dvMinZ;
	This->active_viewport.dvMaxZ = This->current_viewport->viewports.vp1.dvMaxZ;
    }

    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_2T_GetCurrentViewport(LPDIRECT3DDEVICE3 iface,
						 LPDIRECT3DVIEWPORT3* lplpDirect3DViewport3)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    TRACE("(%p/%p)->(%p)\n", This, iface, lplpDirect3DViewport3);

    *lplpDirect3DViewport3 = ICOM_INTERFACE(This->current_viewport, IDirect3DViewport3);
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
    FIXME("(%p/%p)->(%08x,%08lx,%08lx): stub!\n", This, iface, d3dptPrimitiveType, dwVertexTypeDesc, dwFlags);
    return DD_OK;
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
    FIXME("(%p/%p)->(%p): stub!\n", This, iface, lpVertexType);
    return DD_OK;
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
    FIXME("(%p/%p)->(%08lx): stub!\n", This, iface, dwFlags);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_2T_GetLightState(LPDIRECT3DDEVICE3 iface,
                                            D3DLIGHTSTATETYPE dwLightStateType,
                                            LPDWORD lpdwLightState)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    FIXME("(%p/%p)->(%08x,%p): stub!\n", This, iface, dwLightStateType, lpdwLightState);
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
Main_IDirect3DDeviceImpl_3_DrawPrimitiveVB(LPDIRECT3DDEVICE3 iface,
                                           D3DPRIMITIVETYPE d3dptPrimitiveType,
                                           LPDIRECT3DVERTEXBUFFER lpD3DVertexBuf,
                                           DWORD dwStartVertex,
                                           DWORD dwNumVertices,
                                           DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    FIXME("(%p/%p)->(%08x,%p,%08lx,%08lx,%08lx): stub!\n", This, iface, d3dptPrimitiveType, lpD3DVertexBuf, dwStartVertex, dwNumVertices, dwFlags);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_DrawIndexedPrimitiveVB(LPDIRECT3DDEVICE3 iface,
                                                  D3DPRIMITIVETYPE d3dptPrimitiveType,
                                                  LPDIRECT3DVERTEXBUFFER lpD3DVertexBuf,
                                                  LPWORD lpwIndices,
                                                  DWORD dwIndexCount,
                                                  DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    FIXME("(%p/%p)->(%08x,%p,%p,%08lx,%08lx): stub!\n", This, iface, d3dptPrimitiveType, lpD3DVertexBuf, lpwIndices, dwIndexCount, dwFlags);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_GetTexture(LPDIRECT3DDEVICE3 iface,
                                      DWORD dwStage,
                                      LPDIRECT3DTEXTURE2* lplpTexture2)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    FIXME("(%p/%p)->(%08lx,%p): stub!\n", This, iface, dwStage, lplpTexture2);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DDeviceImpl_2_SwapTextureHandles(LPDIRECT3DDEVICE2 iface,
                                              LPDIRECT3DTEXTURE2 lpD3DTex1,
                                              LPDIRECT3DTEXTURE2 lpD3DTex2)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    FIXME("(%p/%p)->(%p,%p): stub!\n", This, iface, lpD3DTex1, lpD3DTex2);
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
Main_IDirect3DDeviceImpl_1_SwapTextureHandles(LPDIRECT3DDEVICE iface,
                                              LPDIRECT3DTEXTURE lpD3Dtex1,
                                              LPDIRECT3DTEXTURE lpD3DTex2)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice, iface);
    FIXME("(%p/%p)->(%p,%p): stub!\n", This, iface, lpD3Dtex1, lpD3DTex2);
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

    dump_mat(lpD3DMatrix);
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
Thunk_IDirect3DDeviceImpl_2_NextViewport(LPDIRECT3DDEVICE3 iface,
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
Thunk_IDirect3DDeviceImpl_1_NextViewport(LPDIRECT3DDEVICE3 iface,
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
    *lplpDirect3D3 = COM_INTERFACE_CAST(IDirect3DImpl, IDirect3D7, IDirect3D3, ret_ptr);
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
    *lplpDirect3D2 = COM_INTERFACE_CAST(IDirect3DImpl, IDirect3D7, IDirect3D2, ret_ptr);
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
    *lplpDirect3D = COM_INTERFACE_CAST(IDirect3DImpl, IDirect3D7, IDirect3D, ret_ptr);
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
