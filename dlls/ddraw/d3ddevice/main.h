/*
 * Copyright 2002 Lionel Ulmer
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

/* This is defined here so as to be able to put them in 'drivers' */

void InitDefaultStateBlock(STATEBLOCK* lpStateBlock, int version);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_1T_QueryInterface(LPDIRECT3DDEVICE7 iface,
                                                   REFIID riid,
                                                   LPVOID* obp);

ULONG WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_1T_AddRef(LPDIRECT3DDEVICE7 iface);

ULONG WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_1T_Release(LPDIRECT3DDEVICE7 iface);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_GetCaps(LPDIRECT3DDEVICE7 iface,
                                   LPD3DDEVICEDESC7 lpD3DHELDevDesc);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_EnumTextureFormats(LPDIRECT3DDEVICE7 iface,
                                                 LPD3DENUMPIXELFORMATSCALLBACK lpD3DEnumPixelProc,
                                                 LPVOID lpArg);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_1T_BeginScene(LPDIRECT3DDEVICE7 iface);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_1T_EndScene(LPDIRECT3DDEVICE7 iface);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_1T_GetDirect3D(LPDIRECT3DDEVICE7 iface,
						LPDIRECT3D7* lplpDirect3D3);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_SetRenderTarget(LPDIRECT3DDEVICE7 iface,
						 LPDIRECTDRAWSURFACE7 lpNewRenderTarget,
						 DWORD dwFlags);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_GetRenderTarget(LPDIRECT3DDEVICE7 iface,
						 LPDIRECTDRAWSURFACE7* lplpRenderTarget);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_Clear(LPDIRECT3DDEVICE7 iface,
                                 DWORD dwCount,
                                 LPD3DRECT lpRects,
                                 DWORD dwFlags,
                                 D3DCOLOR dwColor,
                                 D3DVALUE dvZ,
                                 DWORD dwStencil);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_SetTransform(LPDIRECT3DDEVICE7 iface,
                                              D3DTRANSFORMSTATETYPE dtstTransformStateType,
                                              LPD3DMATRIX lpD3DMatrix);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_GetTransform(LPDIRECT3DDEVICE7 iface,
                                              D3DTRANSFORMSTATETYPE dtstTransformStateType,
                                              LPD3DMATRIX lpD3DMatrix);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_SetViewport(LPDIRECT3DDEVICE7 iface,
                                       LPD3DVIEWPORT7 lpData);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_MultiplyTransform(LPDIRECT3DDEVICE7 iface,
                                                   D3DTRANSFORMSTATETYPE dtstTransformStateType,
                                                   LPD3DMATRIX lpD3DMatrix);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_GetViewport(LPDIRECT3DDEVICE7 iface,
                                       LPD3DVIEWPORT7 lpData);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_SetMaterial(LPDIRECT3DDEVICE7 iface,
                                       LPD3DMATERIAL7 lpMat);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_GetMaterial(LPDIRECT3DDEVICE7 iface,
                                       LPD3DMATERIAL7 lpMat);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_SetLight(LPDIRECT3DDEVICE7 iface,
                                    DWORD dwLightIndex,
                                    LPD3DLIGHT7 lpLight);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_GetLight(LPDIRECT3DDEVICE7 iface,
                                    DWORD dwLightIndex,
                                    LPD3DLIGHT7 lpLight);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_SetRenderState(LPDIRECT3DDEVICE7 iface,
                                                D3DRENDERSTATETYPE dwRenderStateType,
                                                DWORD dwRenderState);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_GetRenderState(LPDIRECT3DDEVICE7 iface,
                                                D3DRENDERSTATETYPE dwRenderStateType,
                                                LPDWORD lpdwRenderState);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_BeginStateBlock(LPDIRECT3DDEVICE7 iface);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_EndStateBlock(LPDIRECT3DDEVICE7 iface,
                                         LPDWORD lpdwBlockHandle);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_PreLoad(LPDIRECT3DDEVICE7 iface,
                                   LPDIRECTDRAWSURFACE7 lpddsTexture);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_DrawPrimitive(LPDIRECT3DDEVICE7 iface,
                                            D3DPRIMITIVETYPE d3dptPrimitiveType,
                                            DWORD d3dvtVertexType,
                                            LPVOID lpvVertices,
                                            DWORD dwVertexCount,
                                            DWORD dwFlags);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_DrawIndexedPrimitive(LPDIRECT3DDEVICE7 iface,
                                                   D3DPRIMITIVETYPE d3dptPrimitiveType,
                                                   DWORD d3dvtVertexType,
                                                   LPVOID lpvVertices,
                                                   DWORD dwVertexCount,
                                                   LPWORD dwIndices,
                                                   DWORD dwIndexCount,
                                                   DWORD dwFlags);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_SetClipStatus(LPDIRECT3DDEVICE7 iface,
                                               LPD3DCLIPSTATUS lpD3DClipStatus);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_GetClipStatus(LPDIRECT3DDEVICE7 iface,
                                               LPD3DCLIPSTATUS lpD3DClipStatus);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_DrawPrimitiveStrided(LPDIRECT3DDEVICE7 iface,
                                                   D3DPRIMITIVETYPE d3dptPrimitiveType,
                                                   DWORD dwVertexType,
                                                   LPD3DDRAWPRIMITIVESTRIDEDDATA lpD3DDrawPrimStrideData,
                                                   DWORD dwVertexCount,
                                                   DWORD dwFlags);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_DrawIndexedPrimitiveStrided(LPDIRECT3DDEVICE7 iface,
                                                          D3DPRIMITIVETYPE d3dptPrimitiveType,
                                                          DWORD dwVertexType,
                                                          LPD3DDRAWPRIMITIVESTRIDEDDATA lpD3DDrawPrimStrideData,
                                                          DWORD dwVertexCount,
                                                          LPWORD lpIndex,
                                                          DWORD dwIndexCount,
                                                          DWORD dwFlags);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_DrawPrimitiveVB(LPDIRECT3DDEVICE7 iface,
					      D3DPRIMITIVETYPE d3dptPrimitiveType,
					      LPDIRECT3DVERTEXBUFFER7 lpD3DVertexBuf,
					      DWORD dwStartVertex,
					      DWORD dwNumVertices,
					      DWORD dwFlags);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_DrawIndexedPrimitiveVB(LPDIRECT3DDEVICE7 iface,
						     D3DPRIMITIVETYPE d3dptPrimitiveType,
						     LPDIRECT3DVERTEXBUFFER7 lpD3DVertexBuf,
						     DWORD dwStartVertex,
						     DWORD dwNumVertices,
						     LPWORD lpwIndices,
						     DWORD dwIndexCount,
						     DWORD dwFlags);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_ComputeSphereVisibility(LPDIRECT3DDEVICE7 iface,
                                                      LPD3DVECTOR lpCenters,
                                                      LPD3DVALUE lpRadii,
                                                      DWORD dwNumSpheres,
                                                      DWORD dwFlags,
                                                      LPDWORD lpdwReturnValues);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_GetTexture(LPDIRECT3DDEVICE7 iface,
					 DWORD dwStage,
					 LPDIRECTDRAWSURFACE7* lpTexture);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_SetTexture(LPDIRECT3DDEVICE7 iface,
					 DWORD dwStage,
					 LPDIRECTDRAWSURFACE7 lpTexture);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_GetTextureStageState(LPDIRECT3DDEVICE7 iface,
                                                   DWORD dwStage,
                                                   D3DTEXTURESTAGESTATETYPE d3dTexStageStateType,
                                                   LPDWORD lpdwState);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_SetTextureStageState(LPDIRECT3DDEVICE7 iface,
                                                   DWORD dwStage,
                                                   D3DTEXTURESTAGESTATETYPE d3dTexStageStateType,
                                                   DWORD dwState);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_ValidateDevice(LPDIRECT3DDEVICE7 iface,
                                             LPDWORD lpdwPasses);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_ApplyStateBlock(LPDIRECT3DDEVICE7 iface,
                                           DWORD dwBlockHandle);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_CaptureStateBlock(LPDIRECT3DDEVICE7 iface,
                                             DWORD dwBlockHandle);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_DeleteStateBlock(LPDIRECT3DDEVICE7 iface,
                                            DWORD dwBlockHandle);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_CreateStateBlock(LPDIRECT3DDEVICE7 iface,
                                            D3DSTATEBLOCKTYPE d3dsbType,
                                            LPDWORD lpdwBlockHandle);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_Load(LPDIRECT3DDEVICE7 iface,
                                LPDIRECTDRAWSURFACE7 lpDestTex,
                                LPPOINT lpDestPoint,
                                LPDIRECTDRAWSURFACE7 lpSrcTex,
                                LPRECT lprcSrcRect,
                                DWORD dwFlags);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_LightEnable(LPDIRECT3DDEVICE7 iface,
                                       DWORD dwLightIndex,
                                       BOOL bEnable);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_GetLightEnable(LPDIRECT3DDEVICE7 iface,
                                          DWORD dwLightIndex,
                                          BOOL* pbEnable);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_SetClipPlane(LPDIRECT3DDEVICE7 iface,
                                        DWORD dwIndex,
                                        D3DVALUE* pPlaneEquation);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_GetClipPlane(LPDIRECT3DDEVICE7 iface,
                                        DWORD dwIndex,
                                        D3DVALUE* pPlaneEquation);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_GetInfo(LPDIRECT3DDEVICE7 iface,
                                   DWORD dwDevInfoID,
                                   LPVOID pDevInfoStruct,
                                   DWORD dwSize);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_2T_1T_GetCaps(LPDIRECT3DDEVICE3 iface,
                                         LPD3DDEVICEDESC lpD3DHWDevDesc,
                                         LPD3DDEVICEDESC lpD3DHELDevDesc);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_2T_1T_GetStats(LPDIRECT3DDEVICE3 iface,
                                          LPD3DSTATS lpD3DStats);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_2T_1T_AddViewport(LPDIRECT3DDEVICE3 iface,
					     LPDIRECT3DVIEWPORT3 lpDirect3DViewport3);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_2T_1T_DeleteViewport(LPDIRECT3DDEVICE3 iface,
						LPDIRECT3DVIEWPORT3 lpDirect3DViewport3);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_2T_1T_NextViewport(LPDIRECT3DDEVICE3 iface,
					      LPDIRECT3DVIEWPORT3 lpDirect3DViewport3,
					      LPDIRECT3DVIEWPORT3* lplpDirect3DViewport3,
					      DWORD dwFlags);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_2T_SetCurrentViewport(LPDIRECT3DDEVICE3 iface,
						 LPDIRECT3DVIEWPORT3 lpDirect3DViewport3);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_2T_GetCurrentViewport(LPDIRECT3DDEVICE3 iface,
						 LPDIRECT3DVIEWPORT3* lplpDirect3DViewport3);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_Begin(LPDIRECT3DDEVICE3 iface,
                                 D3DPRIMITIVETYPE d3dptPrimitiveType,
                                 DWORD dwVertexTypeDesc,
                                 DWORD dwFlags);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_BeginIndexed(LPDIRECT3DDEVICE3 iface,
                                        D3DPRIMITIVETYPE d3dptPrimitiveType,
                                        DWORD d3dvtVertexType,
                                        LPVOID lpvVertices,
                                        DWORD dwNumVertices,
                                        DWORD dwFlags);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_2T_Vertex(LPDIRECT3DDEVICE3 iface,
                                     LPVOID lpVertexType);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_2T_Index(LPDIRECT3DDEVICE3 iface,
                                    WORD wVertexIndex);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_2T_End(LPDIRECT3DDEVICE3 iface,
                                  DWORD dwFlags);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_2T_GetLightState(LPDIRECT3DDEVICE3 iface,
                                            D3DLIGHTSTATETYPE dwLightStateType,
                                            LPDWORD lpdwLightState);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_2T_SetLightState(LPDIRECT3DDEVICE3 iface,
                                            D3DLIGHTSTATETYPE dwLightStateType,
                                            DWORD dwLightState);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_2_1T_SwapTextureHandles(LPDIRECT3DDEVICE2 iface,
                                                 LPDIRECT3DTEXTURE2 lpD3DTex1,
                                                 LPDIRECT3DTEXTURE2 lpD3DTex2);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_2_1T_EnumTextureFormats(LPDIRECT3DDEVICE2 iface,
                                                 LPD3DENUMTEXTUREFORMATSCALLBACK lpD3DEnumTextureProc,
                                                 LPVOID lpArg);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_2_Begin(LPDIRECT3DDEVICE2 iface,
                                 D3DPRIMITIVETYPE d3dpt,
                                 D3DVERTEXTYPE dwVertexTypeDesc,
                                 DWORD dwFlags);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_2_BeginIndexed(LPDIRECT3DDEVICE2 iface,
                                        D3DPRIMITIVETYPE d3dptPrimitiveType,
                                        D3DVERTEXTYPE d3dvtVertexType,
                                        LPVOID lpvVertices,
                                        DWORD dwNumVertices,
                                        DWORD dwFlags);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_2_DrawPrimitive(LPDIRECT3DDEVICE2 iface,
                                         D3DPRIMITIVETYPE d3dptPrimitiveType,
                                         D3DVERTEXTYPE d3dvtVertexType,
                                         LPVOID lpvVertices,
                                         DWORD dwVertexCount,
                                         DWORD dwFlags);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_2_DrawIndexedPrimitive(LPDIRECT3DDEVICE2 iface,
                                                D3DPRIMITIVETYPE d3dptPrimitiveType,
                                                D3DVERTEXTYPE d3dvtVertexType,
                                                LPVOID lpvVertices,
                                                DWORD dwVertexCount,
                                                LPWORD dwIndices,
                                                DWORD dwIndexCount,
                                                DWORD dwFlags);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_1_Initialize(LPDIRECT3DDEVICE iface,
                                      LPDIRECT3D lpDirect3D,
                                      LPGUID lpGUID,
                                      LPD3DDEVICEDESC lpD3DDVDesc);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_1_CreateExecuteBuffer(LPDIRECT3DDEVICE iface,
                                               LPD3DEXECUTEBUFFERDESC lpDesc,
                                               LPDIRECT3DEXECUTEBUFFER* lplpDirect3DExecuteBuffer,
                                               IUnknown* pUnkOuter);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_1_Execute(LPDIRECT3DDEVICE iface,
                                   LPDIRECT3DEXECUTEBUFFER lpDirect3DExecuteBuffer,
                                   LPDIRECT3DVIEWPORT lpDirect3DViewport,
                                   DWORD dwFlags);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_1_Pick(LPDIRECT3DDEVICE iface,
                                LPDIRECT3DEXECUTEBUFFER lpDirect3DExecuteBuffer,
                                LPDIRECT3DVIEWPORT lpDirect3DViewport,
                                DWORD dwFlags,
                                LPD3DRECT lpRect);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_1_GetPickRecords(LPDIRECT3DDEVICE iface,
                                          LPDWORD lpCount,
                                          LPD3DPICKRECORD lpD3DPickRec);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_1_CreateMatrix(LPDIRECT3DDEVICE iface,
                                        LPD3DMATRIXHANDLE lpD3DMatHandle);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_1_SetMatrix(LPDIRECT3DDEVICE iface,
                                     D3DMATRIXHANDLE D3DMatHandle,
                                     LPD3DMATRIX lpD3DMatrix);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_1_GetMatrix(LPDIRECT3DDEVICE iface,
                                     D3DMATRIXHANDLE D3DMatHandle,
                                     LPD3DMATRIX lpD3DMatrix);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_1_DeleteMatrix(LPDIRECT3DDEVICE iface,
                                        D3DMATRIXHANDLE D3DMatHandle);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_QueryInterface(LPDIRECT3DDEVICE3 iface,
                                           REFIID riid,
                                           LPVOID* obp);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_QueryInterface(LPDIRECT3DDEVICE2 iface,
                                           REFIID riid,
                                           LPVOID* obp);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_QueryInterface(LPDIRECT3DDEVICE iface,
                                           REFIID riid,
                                           LPVOID* obp);

ULONG WINAPI
Thunk_IDirect3DDeviceImpl_3_AddRef(LPDIRECT3DDEVICE3 iface);

ULONG WINAPI
Thunk_IDirect3DDeviceImpl_2_AddRef(LPDIRECT3DDEVICE2 iface);

ULONG WINAPI
Thunk_IDirect3DDeviceImpl_1_AddRef(LPDIRECT3DDEVICE iface);

ULONG WINAPI
Thunk_IDirect3DDeviceImpl_3_Release(LPDIRECT3DDEVICE3 iface);

ULONG WINAPI
Thunk_IDirect3DDeviceImpl_2_Release(LPDIRECT3DDEVICE2 iface);

ULONG WINAPI
Thunk_IDirect3DDeviceImpl_1_Release(LPDIRECT3DDEVICE iface);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_AddViewport(LPDIRECT3DDEVICE2 iface,
					LPDIRECT3DVIEWPORT2 lpDirect3DViewport2);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_AddViewport(LPDIRECT3DDEVICE iface,
					LPDIRECT3DVIEWPORT lpDirect3DViewport);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_DeleteViewport(LPDIRECT3DDEVICE2 iface,
					   LPDIRECT3DVIEWPORT2 lpDirect3DViewport2);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_DeleteViewport(LPDIRECT3DDEVICE iface,
					   LPDIRECT3DVIEWPORT lpDirect3DViewport);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_NextViewport(LPDIRECT3DDEVICE3 iface,
					 LPDIRECT3DVIEWPORT2 lpDirect3DViewport2,
					 LPDIRECT3DVIEWPORT2* lplpDirect3DViewport2,
					 DWORD dwFlags);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_NextViewport(LPDIRECT3DDEVICE3 iface,
					 LPDIRECT3DVIEWPORT lpDirect3DViewport,
					 LPDIRECT3DVIEWPORT* lplpDirect3DViewport,
					 DWORD dwFlags);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_GetDirect3D(LPDIRECT3DDEVICE3 iface,
					LPDIRECT3D3* lplpDirect3D3);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetDirect3D(LPDIRECT3DDEVICE2 iface,
					LPDIRECT3D2* lplpDirect3D2);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_GetDirect3D(LPDIRECT3DDEVICE iface,
					LPDIRECT3D* lplpDirect3D);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_SetCurrentViewport(LPDIRECT3DDEVICE2 iface,
					       LPDIRECT3DVIEWPORT2 lpDirect3DViewport2);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetCurrentViewport(LPDIRECT3DDEVICE2 iface,
					       LPDIRECT3DVIEWPORT2* lpDirect3DViewport2);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_EnumTextureFormats(LPDIRECT3DDEVICE3 iface,
                                               LPD3DENUMPIXELFORMATSCALLBACK lpD3DEnumPixelProc,
                                               LPVOID lpArg);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_BeginScene(LPDIRECT3DDEVICE3 iface);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_BeginScene(LPDIRECT3DDEVICE2 iface);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_BeginScene(LPDIRECT3DDEVICE iface);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_EndScene(LPDIRECT3DDEVICE3 iface);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_EndScene(LPDIRECT3DDEVICE2 iface);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_EndScene(LPDIRECT3DDEVICE iface);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_SetTransform(LPDIRECT3DDEVICE3 iface,
                                         D3DTRANSFORMSTATETYPE dtstTransformStateType,
                                         LPD3DMATRIX lpD3DMatrix);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_SetTransform(LPDIRECT3DDEVICE2 iface,
                                         D3DTRANSFORMSTATETYPE dtstTransformStateType,
                                         LPD3DMATRIX lpD3DMatrix);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_GetTransform(LPDIRECT3DDEVICE3 iface,
                                         D3DTRANSFORMSTATETYPE dtstTransformStateType,
                                         LPD3DMATRIX lpD3DMatrix);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetTransform(LPDIRECT3DDEVICE2 iface,
                                         D3DTRANSFORMSTATETYPE dtstTransformStateType,
                                         LPD3DMATRIX lpD3DMatrix);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_MultiplyTransform(LPDIRECT3DDEVICE3 iface,
                                              D3DTRANSFORMSTATETYPE dtstTransformStateType,
                                              LPD3DMATRIX lpD3DMatrix);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_MultiplyTransform(LPDIRECT3DDEVICE2 iface,
                                              D3DTRANSFORMSTATETYPE dtstTransformStateType,
                                              LPD3DMATRIX lpD3DMatrix);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_SetRenderState(LPDIRECT3DDEVICE3 iface,
                                           D3DRENDERSTATETYPE dwRenderStateType,
                                           DWORD dwRenderState);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_SetRenderState(LPDIRECT3DDEVICE2 iface,
                                           D3DRENDERSTATETYPE dwRenderStateType,
                                           DWORD dwRenderState);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_GetRenderState(LPDIRECT3DDEVICE3 iface,
                                           D3DRENDERSTATETYPE dwRenderStateType,
                                           LPDWORD lpdwRenderState);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetRenderState(LPDIRECT3DDEVICE2 iface,
                                           D3DRENDERSTATETYPE dwRenderStateType,
                                           LPDWORD lpdwRenderState);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_DrawPrimitive(LPDIRECT3DDEVICE3 iface,
                                          D3DPRIMITIVETYPE d3dptPrimitiveType,
                                          DWORD d3dvtVertexType,
                                          LPVOID lpvVertices,
                                          DWORD dwVertexCount,
                                          DWORD dwFlags);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_DrawIndexedPrimitive(LPDIRECT3DDEVICE3 iface,
                                                 D3DPRIMITIVETYPE d3dptPrimitiveType,
                                                 DWORD d3dvtVertexType,
                                                 LPVOID lpvVertices,
                                                 DWORD dwVertexCount,
                                                 LPWORD dwIndices,
                                                 DWORD dwIndexCount,
                                                 DWORD dwFlags);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_SetClipStatus(LPDIRECT3DDEVICE3 iface,
                                          LPD3DCLIPSTATUS lpD3DClipStatus);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_SetClipStatus(LPDIRECT3DDEVICE2 iface,
                                          LPD3DCLIPSTATUS lpD3DClipStatus);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_GetClipStatus(LPDIRECT3DDEVICE3 iface,
                                          LPD3DCLIPSTATUS lpD3DClipStatus);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetClipStatus(LPDIRECT3DDEVICE2 iface,
                                          LPD3DCLIPSTATUS lpD3DClipStatus);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_DrawPrimitiveStrided(LPDIRECT3DDEVICE3 iface,
                                                 D3DPRIMITIVETYPE d3dptPrimitiveType,
                                                 DWORD dwVertexType,
                                                 LPD3DDRAWPRIMITIVESTRIDEDDATA lpD3DDrawPrimStrideData,
                                                 DWORD dwVertexCount,
                                                 DWORD dwFlags);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_DrawIndexedPrimitiveStrided(LPDIRECT3DDEVICE3 iface,
                                                        D3DPRIMITIVETYPE d3dptPrimitiveType,
                                                        DWORD dwVertexType,
                                                        LPD3DDRAWPRIMITIVESTRIDEDDATA lpD3DDrawPrimStrideData,
                                                        DWORD dwVertexCount,
                                                        LPWORD lpIndex,
                                                        DWORD dwIndexCount,
                                                        DWORD dwFlags);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_ComputeSphereVisibility(LPDIRECT3DDEVICE3 iface,
                                                    LPD3DVECTOR lpCenters,
                                                    LPD3DVALUE lpRadii,
                                                    DWORD dwNumSpheres,
                                                    DWORD dwFlags,
                                                    LPDWORD lpdwReturnValues);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_GetTextureStageState(LPDIRECT3DDEVICE3 iface,
                                                 DWORD dwStage,
                                                 D3DTEXTURESTAGESTATETYPE d3dTexStageStateType,
                                                 LPDWORD lpdwState);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_SetTextureStageState(LPDIRECT3DDEVICE3 iface,
                                                 DWORD dwStage,
                                                 D3DTEXTURESTAGESTATETYPE d3dTexStageStateType,
                                                 DWORD dwState);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_ValidateDevice(LPDIRECT3DDEVICE3 iface,
                                           LPDWORD lpdwPasses);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetCaps(LPDIRECT3DDEVICE2 iface,
                                    LPD3DDEVICEDESC lpD3DHWDevDesc,
                                    LPD3DDEVICEDESC lpD3DHELDevDesc);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_GetCaps(LPDIRECT3DDEVICE iface,
                                    LPD3DDEVICEDESC lpD3DHWDevDesc,
                                    LPD3DDEVICEDESC lpD3DHELDevDesc);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_SwapTextureHandles(LPDIRECT3DDEVICE iface,
                                               LPDIRECT3DTEXTURE lpD3Dtex1,
                                               LPDIRECT3DTEXTURE lpD3DTex2);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetStats(LPDIRECT3DDEVICE2 iface,
                                     LPD3DSTATS lpD3DStats);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_GetStats(LPDIRECT3DDEVICE iface,
                                     LPD3DSTATS lpD3DStats);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_SetRenderTarget(LPDIRECT3DDEVICE3 iface,
                                            LPDIRECTDRAWSURFACE4 lpNewRenderTarget,
                                            DWORD dwFlags);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_GetRenderTarget(LPDIRECT3DDEVICE3 iface,
                                            LPDIRECTDRAWSURFACE4* lplpRenderTarget);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_SetRenderTarget(LPDIRECT3DDEVICE2 iface,
                                            LPDIRECTDRAWSURFACE lpNewRenderTarget,
                                            DWORD dwFlags);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetRenderTarget(LPDIRECT3DDEVICE2 iface,
                                            LPDIRECTDRAWSURFACE* lplpRenderTarget);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_Vertex(LPDIRECT3DDEVICE2 iface,
                                   LPVOID lpVertexType);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_Index(LPDIRECT3DDEVICE2 iface,
                                  WORD wVertexIndex);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_End(LPDIRECT3DDEVICE2 iface,
                                DWORD dwFlags);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetLightState(LPDIRECT3DDEVICE2 iface,
                                          D3DLIGHTSTATETYPE dwLightStateType,
                                          LPDWORD lpdwLightState);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_SetLightState(LPDIRECT3DDEVICE2 iface,
                                          D3DLIGHTSTATETYPE dwLightStateType,
                                          DWORD dwLightState);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_EnumTextureFormats(LPDIRECT3DDEVICE iface,
                                               LPD3DENUMTEXTUREFORMATSCALLBACK lpD3DEnumTextureProc,
                                               LPVOID lpArg);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_SetTexture(LPDIRECT3DDEVICE3 iface,
				       DWORD dwStage,
				       LPDIRECT3DTEXTURE2 lpTexture2);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_DrawPrimitiveVB(LPDIRECT3DDEVICE3 iface,
					    D3DPRIMITIVETYPE d3dptPrimitiveType,
					    LPDIRECT3DVERTEXBUFFER lpD3DVertexBuf,
					    DWORD dwStartVertex,
					    DWORD dwNumVertices,
					    DWORD dwFlags);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_DrawIndexedPrimitiveVB(LPDIRECT3DDEVICE3 iface,
						   D3DPRIMITIVETYPE d3dptPrimitiveType,
						   LPDIRECT3DVERTEXBUFFER lpD3DVertexBuf,
						   LPWORD lpwIndices,
						   DWORD dwIndexCount,
						   DWORD dwFlags);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_GetTexture(LPDIRECT3DDEVICE3 iface,
				       DWORD dwStage,
				       LPDIRECT3DTEXTURE2* lplpTexture2);
