/*
 * state block implementation
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

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

/* Used for CreateStateBlock */
#define NUM_SAVEDPIXELSTATES_R     38
#define NUM_SAVEDPIXELSTATES_T     27
#define NUM_SAVEDVERTEXSTATES_R    33
#define NUM_SAVEDVERTEXSTATES_T    2

/*
 * Globals
 */
extern DWORD SavedPixelStates_R[NUM_SAVEDPIXELSTATES_R];
extern DWORD SavedPixelStates_T[NUM_SAVEDPIXELSTATES_T];
extern DWORD SavedVertexStates_R[NUM_SAVEDVERTEXSTATES_R];
extern DWORD SavedVertexStates_T[NUM_SAVEDVERTEXSTATES_T];
static const float idmatrix[16] = {
  1.0, 0.0, 0.0, 0.0,
  0.0, 1.0, 0.0, 0.0,
  0.0, 0.0, 1.0, 0.0,
  0.0, 0.0, 0.0, 1.0
};

HRESULT WINAPI IDirect3DDeviceImpl_InitStartupStateBlock(IDirect3DDevice8Impl* This) {
    D3DLINEPATTERN lp;
    int i;
    LPDIRECT3DDEVICE8 iface = (LPDIRECT3DDEVICE8) This;

    /* Note this may have a large overhead but it should only be executed
       once, in order to initialize the complete state of the device and 
       all opengl equivalents                                            */
    TRACE("-----------------------> Setting up device defaults...\n");
    This->StateBlock->blockType = D3DSBT_ALL;

    /* FIXME: Set some of the defaults for lights, transforms etc */
    memcpy(&This->StateBlock->transforms[D3DTS_PROJECTION], &idmatrix, sizeof(idmatrix));
    memcpy(&This->StateBlock->transforms[D3DTS_VIEW], &idmatrix, sizeof(idmatrix));
    for (i = 0; i < 256; ++i) {
      memcpy(&This->StateBlock->transforms[D3DTS_WORLDMATRIX(i)], &idmatrix, sizeof(idmatrix));
    }
 
    /* Render states: */
    if (This->PresentParms.EnableAutoDepthStencil) {
       IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_ZENABLE, D3DZB_TRUE);
    } else {
       IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_ZENABLE, D3DZB_FALSE);
    }
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_FILLMODE, D3DFILL_SOLID);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
    lp.wRepeatFactor = 0; lp.wLinePattern = 0; IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_LINEPATTERN, (DWORD) &lp);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_ZWRITEENABLE, TRUE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_ALPHATESTENABLE, FALSE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_LASTPIXEL, TRUE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_SRCBLEND, D3DBLEND_ONE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_DESTBLEND, D3DBLEND_ZERO);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_CULLMODE, D3DCULL_CCW);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_ALPHAFUNC, D3DCMP_ALWAYS);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_ALPHAREF, 0xff); /*??*/
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_DITHERENABLE, FALSE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_ALPHABLENDENABLE, FALSE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_FOGENABLE, FALSE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_SPECULARENABLE, FALSE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_ZVISIBLE, 0);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_FOGCOLOR, 0);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_FOGTABLEMODE, D3DFOG_NONE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_FOGSTART, 0.0f);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_FOGEND, 1.0f);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_FOGDENSITY, 1.0f);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_EDGEANTIALIAS, FALSE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_ZBIAS, 0);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_RANGEFOGENABLE, FALSE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_STENCILENABLE, FALSE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_STENCILREF, 0);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_STENCILMASK, 0xFFFFFFFF);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_STENCILWRITEMASK, 0xFFFFFFFF);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_TEXTUREFACTOR, 0xFFFFFFFF);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_WRAP0, 0);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_WRAP1, 0);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_WRAP2, 0);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_WRAP3, 0);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_WRAP4, 0);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_WRAP5, 0);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_WRAP6, 0);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_WRAP7, 0);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_CLIPPING, TRUE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_LIGHTING, TRUE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_AMBIENT, 0);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_FOGVERTEXMODE, D3DFOG_NONE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_COLORVERTEX, TRUE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_LOCALVIEWER, TRUE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_NORMALIZENORMALS, FALSE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_SPECULARMATERIALSOURCE, D3DMCS_COLOR2);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_COLOR2);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_VERTEXBLEND, D3DVBF_DISABLE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_CLIPPLANEENABLE, 0);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_SOFTWAREVERTEXPROCESSING, FALSE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_POINTSIZE, 1.0f);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_POINTSIZE_MIN, 0.0f);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_POINTSPRITEENABLE, FALSE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_POINTSCALEENABLE, FALSE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_POINTSCALE_A, TRUE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_POINTSCALE_B, TRUE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_POINTSCALE_C, TRUE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_MULTISAMPLEANTIALIAS, TRUE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_MULTISAMPLEMASK, 0xFFFFFFFF);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_PATCHEDGESTYLE, D3DPATCHEDGE_DISCRETE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_PATCHSEGMENTS, 1.0f);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_DEBUGMONITORTOKEN, D3DDMT_DISABLE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_POINTSIZE_MAX, (DWORD) 64.0f);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_INDEXEDVERTEXBLENDENABLE, FALSE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_COLORWRITEENABLE, 0x0000000F);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_TWEENFACTOR, (DWORD) 0.0f);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_BLENDOP, D3DBLENDOP_ADD);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_POSITIONORDER, D3DORDER_CUBIC);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_NORMALORDER, D3DORDER_LINEAR);

    /* Texture Stage States - Put directly into state block, we will call function below */
    for (i=0; i<This->TextureUnits;i++) {
        memcpy(&This->StateBlock->transforms[D3DTS_TEXTURE0+i], &idmatrix, sizeof(idmatrix));
        This->StateBlock->texture_state[i][D3DTSS_COLOROP               ] = (i==0)? D3DTOP_MODULATE :  D3DTOP_DISABLE;
        This->StateBlock->texture_state[i][D3DTSS_COLORARG1             ] = D3DTA_TEXTURE;
        This->StateBlock->texture_state[i][D3DTSS_COLORARG2             ] = D3DTA_CURRENT;
        This->StateBlock->texture_state[i][D3DTSS_ALPHAOP               ] = (i==0)? D3DTOP_SELECTARG1 :  D3DTOP_DISABLE;
        This->StateBlock->texture_state[i][D3DTSS_ALPHAARG1             ] = D3DTA_TEXTURE;
        This->StateBlock->texture_state[i][D3DTSS_ALPHAARG2             ] = D3DTA_CURRENT;
        This->StateBlock->texture_state[i][D3DTSS_BUMPENVMAT00          ] = (DWORD) 0.0;
        This->StateBlock->texture_state[i][D3DTSS_BUMPENVMAT01          ] = (DWORD) 0.0;
        This->StateBlock->texture_state[i][D3DTSS_BUMPENVMAT10          ] = (DWORD) 0.0;
        This->StateBlock->texture_state[i][D3DTSS_BUMPENVMAT11          ] = (DWORD) 0.0;
        This->StateBlock->texture_state[i][D3DTSS_TEXCOORDINDEX         ] = i;
        This->StateBlock->texture_state[i][D3DTSS_ADDRESSU              ] = D3DTADDRESS_WRAP;
        This->StateBlock->texture_state[i][D3DTSS_ADDRESSV              ] = D3DTADDRESS_WRAP;
        This->StateBlock->texture_state[i][D3DTSS_BORDERCOLOR           ] = 0x00;
        This->StateBlock->texture_state[i][D3DTSS_MAGFILTER             ] = D3DTEXF_POINT;
        This->StateBlock->texture_state[i][D3DTSS_MINFILTER             ] = D3DTEXF_POINT;
        This->StateBlock->texture_state[i][D3DTSS_MIPFILTER             ] = D3DTEXF_NONE;
        This->StateBlock->texture_state[i][D3DTSS_MIPMAPLODBIAS         ] = 0;
        This->StateBlock->texture_state[i][D3DTSS_MAXMIPLEVEL           ] = 0;
        This->StateBlock->texture_state[i][D3DTSS_MAXANISOTROPY         ] = 1;
        This->StateBlock->texture_state[i][D3DTSS_BUMPENVLSCALE         ] = (DWORD) 0.0;
        This->StateBlock->texture_state[i][D3DTSS_BUMPENVLOFFSET        ] = (DWORD) 0.0;
        This->StateBlock->texture_state[i][D3DTSS_TEXTURETRANSFORMFLAGS ] = D3DTTFF_DISABLE;
        This->StateBlock->texture_state[i][D3DTSS_ADDRESSW              ] = D3DTADDRESS_WRAP;
        This->StateBlock->texture_state[i][D3DTSS_COLORARG0             ] = D3DTA_CURRENT;
        This->StateBlock->texture_state[i][D3DTSS_ALPHAARG0             ] = D3DTA_CURRENT;
        This->StateBlock->texture_state[i][D3DTSS_RESULTARG             ] = D3DTA_CURRENT;
    }

    /* Under DirectX you can have texture stage operations even if no texture is
       bound, whereas opengl will only do texture operations when a valid texture is
       bound. We emulate this by creating dummy textures and binding them to each
       texture stage, but disable all stages by default. Hence if a stage is enabled
       then the default texture will kick in until replaced by a SetTexture call     */

    for (i = 0; i < This->TextureUnits; i++) {
        GLubyte white = 255;

        /* Note this avoids calling settexture, so pretend it has been called */
        This->StateBlock->Set.textures[i] = TRUE;
        This->StateBlock->Changed.textures[i] = TRUE;
        This->StateBlock->textures[i] = NULL;

        /* Make appropriate texture active */
        if (This->isMultiTexture) {
#if defined(GL_VERSION_1_3)
            glActiveTexture(GL_TEXTURE0 + i);
#else
            glActiveTextureARB(GL_TEXTURE0_ARB + i);
#endif
            checkGLcall("glActiveTextureARB");
        } else if (i > 0) {
            FIXME("Program using multiple concurrent textures which this opengl implementation doesnt support\n");
        }

        /* Generate an opengl texture name */
        glGenTextures(1, &This->dummyTextureName[i]);
        checkGLcall("glGenTextures");
        TRACE("Dummy Texture %d given name %d\n", i, This->dummyTextureName[i]);

        /* Generate a dummy 1d texture */
        This->StateBlock->textureDimensions[i] = GL_TEXTURE_1D;
        glBindTexture(GL_TEXTURE_1D, This->dummyTextureName[i]);
        checkGLcall("glBindTexture");

        glTexImage1D(GL_TEXTURE_1D, 0, GL_LUMINANCE, 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, &white); 
        checkGLcall("glTexImage1D");

        /* Reapply all the texture state information to this texture */
        setupTextureStates(iface, i);
    }

    TRACE("-----------------------> Device defaults now set up...\n");

    return D3D_OK;
}



HRESULT WINAPI IDirect3DDeviceImpl_CreateStateBlock(IDirect3DDevice8Impl* This, D3DSTATEBLOCKTYPE Type, IDirect3DStateBlockImpl** ppStateBlock) {
  IDirect3DStateBlockImpl* object;
  UINT i, j;

  TRACE("(%p) : Type(%d)\n", This, Type);

  /* Allocate Storage */
  object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DStateBlockImpl));
  if (object) {
    if (NULL == This->StateBlock) { /** if it the main stateblock only do init and returns */
      /*object->lpVtbl = &Direct3DStateBlock9_Vtbl;*/
      object->device = This;
      object->ref = 1;
      object->blockType = Type;
      This->StateBlock = object;
      /* don't forget to init it calling InitStartupStateBlock */
      return D3D_OK;
    }
    memcpy(object, This->StateBlock, sizeof(IDirect3DStateBlockImpl));
  } else {
    *ppStateBlock = (IDirect3DStateBlockImpl*) 0xFFFFFFFF;
    return E_OUTOFMEMORY;
  }
  /*object->lpVtbl = &Direct3DStateBlock9_Vtbl;*/
  object->device = This;
  object->ref = 1;
  object->blockType = Type;

  TRACE("Updating changed flags appropriate for type %d\n", Type);

  if (Type == D3DSBT_ALL) {
    TRACE("ALL => Pretend everything has changed\n");
    memset(&object->Changed, TRUE, sizeof(This->StateBlock->Changed));
    
  } else if (Type == D3DSBT_PIXELSTATE) {
    
    memset(&object->Changed, FALSE, sizeof(This->StateBlock->Changed));
    
    /* TODO: Pixel Shader Constants */
    object->Changed.pixelShader = TRUE;
    for (i = 0; i < NUM_SAVEDPIXELSTATES_R; i++) {
      object->Changed.renderstate[SavedPixelStates_R[i]] = TRUE;
    }
    for (j = 0; j < This->TextureUnits; i++) {
      for (i = 0; i < NUM_SAVEDPIXELSTATES_T; i++) {
	object->Changed.texture_state[j][SavedPixelStates_T[i]] = TRUE;
      }
    }
    
  } else if (Type == D3DSBT_VERTEXSTATE) {
    
    memset(&object->Changed, FALSE, sizeof(This->StateBlock->Changed));
    
    /* TODO: Vertex Shader Constants */
    object->Changed.vertexShader = TRUE;
    for (i = 0; i < NUM_SAVEDVERTEXSTATES_R; i++) {
      object->Changed.renderstate[SavedVertexStates_R[i]] = TRUE;
    }
    for (j = 0; j < This->TextureUnits; i++) {
      for (i = 0; i < NUM_SAVEDVERTEXSTATES_T; i++) {
	object->Changed.texture_state[j][SavedVertexStates_T[i]] = TRUE;
      }
    }
    for (i = 0; i < This->maxLights; i++) {
      object->Changed.lightEnable[i] = TRUE;
      object->Changed.lights[i] = TRUE;
    }
    
  } else {
    FIXME("Unrecognized state block type %d\n", Type);
  }
  TRACE("(%p) returning token (ptr to stateblock) of %p\n", This, object);
  
  *ppStateBlock = object;
  return D3D_OK;
}

/** yakkk temporary waiting for Release */
HRESULT WINAPI IDirect3DDeviceImpl_DeleteStateBlock(IDirect3DDevice8Impl* This, IDirect3DStateBlockImpl* pSB) {
  TRACE("(%p) : freeing StateBlock %p\n", This, pSB);
  HeapFree(GetProcessHeap(), 0, (void *)pSB);
  return D3D_OK;
}

HRESULT WINAPI IDirect3DDeviceImpl_BeginStateBlock(IDirect3DDevice8Impl* This) {
  IDirect3DStateBlockImpl* object;
  
  TRACE("(%p)\n", This);
  
  if (This->isRecordingState) {
    TRACE("(%p) already recording! returning error\n", This);
    return D3DERR_INVALIDCALL;
  }

  /* Allocate Storage */
  object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DStateBlockImpl));
  if (object) {
  } else {
    return E_OUTOFMEMORY;
  }
  /*object->lpVtbl = &Direct3DVextexShaderDeclaration8_Vtbl;*/
  object->device = This;
  object->ref = 1;
  
  This->isRecordingState = TRUE;
  This->UpdateStateBlock = object;
  
  return D3D_OK;
}

HRESULT WINAPI IDirect3DDeviceImpl_EndStateBlock(IDirect3DDevice8Impl* This, IDirect3DStateBlockImpl** ppStateBlock) {
  TRACE("(%p)\n", This);
  
  if (!This->isRecordingState) {
    TRACE("(%p) not recording! returning error\n", This);
    *ppStateBlock = NULL;
    return D3DERR_INVALIDCALL;
  }

  This->UpdateStateBlock->blockType = D3DSBT_RECORDED;
  *ppStateBlock = This->UpdateStateBlock;  /* FIXME: AddRef() */
  This->isRecordingState = FALSE;
  This->UpdateStateBlock = This->StateBlock;
  
  TRACE("(%p) returning token (ptr to stateblock) of %p\n", This, *ppStateBlock);
  return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDeviceImpl_ApplyStateBlock(IDirect3DDevice8Impl* This, IDirect3DStateBlockImpl* pSB) {
    UINT i;
    UINT j;
    LPDIRECT3DDEVICE8 iface = (LPDIRECT3DDEVICE8) This;

    TRACE("(%p) : Applying state block %p ------------------v\n", This, pSB);

    /* FIXME: Only apply applicable states not all states */

    if (pSB->blockType == D3DSBT_RECORDED || pSB->blockType == D3DSBT_ALL || pSB->blockType == D3DSBT_VERTEXSTATE) {

        for (i = 0; i < This->maxLights; i++) {

            if (pSB->Set.lightEnable[i] && pSB->Changed.lightEnable[i])
                IDirect3DDevice8Impl_LightEnable(iface, i, pSB->lightEnable[i]);
            if (pSB->Set.lights[i] && pSB->Changed.lights[i])
                IDirect3DDevice8Impl_SetLight(iface, i, &pSB->lights[i]);
        }

        if (pSB->Set.vertexShader && pSB->Changed.vertexShader)
            IDirect3DDevice8Impl_SetVertexShader(iface, pSB->VertexShader);

        /* TODO: Vertex Shader Constants */
    }

    if (pSB->blockType == D3DSBT_RECORDED || pSB->blockType == D3DSBT_ALL || pSB->blockType == D3DSBT_PIXELSTATE) {

        if (pSB->Set.pixelShader && pSB->Changed.pixelShader)
            IDirect3DDevice8Impl_SetPixelShader(iface, pSB->PixelShader);

        /* TODO: Pixel Shader Constants */
    }

    /* Others + Render & Texture */
    if (pSB->blockType == D3DSBT_RECORDED || pSB->blockType == D3DSBT_ALL) {
        for (i = 0; i < HIGHEST_TRANSFORMSTATE; i++) {
            if (pSB->Set.transform[i] && pSB->Changed.transform[i])
                IDirect3DDevice8Impl_SetTransform(iface, i, &pSB->transforms[i]);
        }

        if (pSB->Set.Indices && pSB->Changed.Indices)
            IDirect3DDevice8Impl_SetIndices(iface, pSB->pIndexData, pSB->baseVertexIndex);

        if (pSB->Set.material && pSB->Changed.material)
            IDirect3DDevice8Impl_SetMaterial(iface, &pSB->material);

        if (pSB->Set.viewport && pSB->Changed.viewport)
            IDirect3DDevice8Impl_SetViewport(iface, &pSB->viewport);

        for (i=0; i<MAX_STREAMS; i++) {
            if (pSB->Set.stream_source[i] && pSB->Changed.stream_source[i])
                IDirect3DDevice8Impl_SetStreamSource(iface, i, pSB->stream_source[i], pSB->stream_stride[i]);
        }

        for (i = 0; i < This->clipPlanes; i++) {
            if (pSB->Set.clipplane[i] && pSB->Changed.clipplane[i]) {
                float clip[4];

                clip[0] = pSB->clipplane[i][0];
                clip[1] = pSB->clipplane[i][1];
                clip[2] = pSB->clipplane[i][2];
                clip[3] = pSB->clipplane[i][3];
                IDirect3DDevice8Impl_SetClipPlane(iface, i, clip);
            }
        }

        /* Render */
        for (i = 0; i < HIGHEST_RENDER_STATE; i++) {
            if (pSB->Set.renderstate[i] && pSB->Changed.renderstate[i])
                IDirect3DDevice8Impl_SetRenderState(iface, i, pSB->renderstate[i]);
        }

        /* Texture */
        for (j = 0; j < This->TextureUnits; j++) {
	  for (i = 0; i < HIGHEST_TEXTURE_STATE; i++) {
	    if (pSB->Set.texture_state[j][i] && pSB->Changed.texture_state[j][i]) {
	      IDirect3DDevice8Impl_SetTextureStageState(iface, j, i, pSB->texture_state[j][i]);
	    }
	  } 
	  if (pSB->Set.textures[j] && pSB->Changed.textures[j]) {
	    IDirect3DDevice8Impl_SetTexture(iface, j, pSB->textures[j]);
	  } 
        }


    } else if (pSB->blockType == D3DSBT_PIXELSTATE) {

        for (i = 0; i < NUM_SAVEDPIXELSTATES_R; i++) {
            if (pSB->Set.renderstate[SavedPixelStates_R[i]] && pSB->Changed.renderstate[SavedPixelStates_R[i]])
                IDirect3DDevice8Impl_SetRenderState(iface, SavedPixelStates_R[i], pSB->renderstate[SavedPixelStates_R[i]]);

        }

        for (j = 0; j < This->TextureUnits; i++) {
            for (i = 0; i < NUM_SAVEDPIXELSTATES_T; i++) {
                if (pSB->Set.texture_state[j][SavedPixelStates_T[i]] &&
                    pSB->Changed.texture_state[j][SavedPixelStates_T[i]])
                    IDirect3DDevice8Impl_SetTextureStageState(iface, j, SavedPixelStates_T[i], pSB->texture_state[j][SavedPixelStates_T[i]]);
            }
        }

    } else if (pSB->blockType == D3DSBT_VERTEXSTATE) {

        for (i = 0; i < NUM_SAVEDVERTEXSTATES_R; i++) {
            if (pSB->Set.renderstate[SavedVertexStates_R[i]] && pSB->Changed.renderstate[SavedVertexStates_R[i]])
                IDirect3DDevice8Impl_SetRenderState(iface, SavedVertexStates_R[i], pSB->renderstate[SavedVertexStates_R[i]]);
        }

        for (j = 0; j < This->TextureUnits; i++) {
            for (i = 0; i < NUM_SAVEDVERTEXSTATES_T; i++) {
                if (pSB->Set.texture_state[j][SavedVertexStates_T[i]] &&
                    pSB->Changed.texture_state[j][SavedVertexStates_T[i]])
                    IDirect3DDevice8Impl_SetTextureStageState(iface, j, SavedVertexStates_T[i], pSB->texture_state[j][SavedVertexStates_T[i]]);
            }
        }


    } else {
        FIXME("Unrecognized state block type %d\n", pSB->blockType);
    }
    memcpy(&This->StateBlock->Changed, &pSB->Changed, sizeof(This->StateBlock->Changed));
    TRACE("(%p) : Applied state block %p ------------------^\n", This, pSB);

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDeviceImpl_CaptureStateBlock(IDirect3DDevice8Impl* This, IDirect3DStateBlockImpl* updateBlock) {
    LPDIRECT3DDEVICE8 iface = (LPDIRECT3DDEVICE8) This;

    TRACE("(%p) : Updating state block %p ------------------v \n", This, updateBlock);

    /* If not recorded, then update can just recapture */
    if (updateBlock->blockType != D3DSBT_RECORDED) {
        IDirect3DStateBlockImpl* tmpBlock;
        IDirect3DDeviceImpl_CreateStateBlock(This, updateBlock->blockType, &tmpBlock);
        memcpy(updateBlock, tmpBlock, sizeof(IDirect3DStateBlockImpl));
        IDirect3DDeviceImpl_DeleteStateBlock(This, tmpBlock);

        /* FIXME: This will record states of new lights! May need to have and save set_lights
           across this action */

    } else {
        int i, j;

        /* Recorded => Only update 'changed' values */
        if (updateBlock->Set.vertexShader && updateBlock->VertexShader != This->StateBlock->VertexShader) {
            updateBlock->VertexShader = This->StateBlock->VertexShader;
            TRACE("Updating vertex shader to %ld\n", This->StateBlock->VertexShader);
        }

        /* TODO: Vertex Shader Constants */

        for (i = 0; i < This->maxLights; i++) {
          if (updateBlock->Set.lightEnable[i] && This->StateBlock->lightEnable[i] != updateBlock->lightEnable[i]) {
              TRACE("Updating light enable for light %d to %d\n", i, This->StateBlock->lightEnable[i]);
              updateBlock->lightEnable[i] = This->StateBlock->lightEnable[i];
          }

          if (updateBlock->Set.lights[i] && memcmp(&This->StateBlock->lights[i], 
                                                   &updateBlock->lights[i], 
                                                   sizeof(D3DLIGHT8)) != 0) {
              TRACE("Updating lights for light %d\n", i);
              memcpy(&updateBlock->lights[i], &This->StateBlock->lights[i], sizeof(D3DLIGHT8));
          }
        }

        if (updateBlock->Set.pixelShader && updateBlock->PixelShader != This->StateBlock->PixelShader) {
            TRACE("Updating pixel shader to %ld\n", This->StateBlock->PixelShader);
            updateBlock->lights[i] = This->StateBlock->lights[i];
	    IDirect3DDevice8Impl_SetVertexShader(iface, updateBlock->PixelShader);
        }

        /* TODO: Pixel Shader Constants */

        /* Others + Render & Texture */
	for (i = 0; i < HIGHEST_TRANSFORMSTATE; i++) {
	  if (updateBlock->Set.transform[i] && memcmp(&This->StateBlock->transforms[i], 
						      &updateBlock->transforms[i], 
						      sizeof(D3DMATRIX)) != 0) {
	    TRACE("Updating transform %d\n", i);
	    memcpy(&updateBlock->transforms[i], &This->StateBlock->transforms[i], sizeof(D3DMATRIX));
	  }
	}

	if (updateBlock->Set.Indices && ((updateBlock->pIndexData != This->StateBlock->pIndexData)
					 || (updateBlock->baseVertexIndex != This->StateBlock->baseVertexIndex))) {
	  TRACE("Updating pindexData to %p, baseVertexIndex to %d\n", 
		This->StateBlock->pIndexData, This->StateBlock->baseVertexIndex);
	  updateBlock->pIndexData = This->StateBlock->pIndexData;
	  updateBlock->baseVertexIndex = This->StateBlock->baseVertexIndex;
	}

       if (updateBlock->Set.material && memcmp(&This->StateBlock->material, 
                                                   &updateBlock->material, 
                                                   sizeof(D3DMATERIAL8)) != 0) {
                TRACE("Updating material\n");
                memcpy(&updateBlock->material, &This->StateBlock->material, sizeof(D3DMATERIAL8));
       }
           
       if (updateBlock->Set.viewport && memcmp(&This->StateBlock->viewport, 
                                                   &updateBlock->viewport, 
                                                   sizeof(D3DVIEWPORT8)) != 0) {
                TRACE("Updating viewport\n");
                memcpy(&updateBlock->viewport, &This->StateBlock->viewport, sizeof(D3DVIEWPORT8));
       }

       for (i = 0; i < MAX_STREAMS; i++) {
           if (updateBlock->Set.stream_source[i] && 
                           ((updateBlock->stream_stride[i] != This->StateBlock->stream_stride[i]) ||
                           (updateBlock->stream_source[i] != This->StateBlock->stream_source[i]))) {
               TRACE("Updating stream source %d to %p, stride to %d\n", i, This->StateBlock->stream_source[i], 
                                                                        This->StateBlock->stream_stride[i]);
               updateBlock->stream_stride[i] = This->StateBlock->stream_stride[i];
               updateBlock->stream_source[i] = This->StateBlock->stream_source[i];
           }
       }

       for (i = 0; i < This->clipPlanes; i++) {
           if (updateBlock->Set.clipplane[i] && memcmp(&This->StateBlock->clipplane[i], 
                                                       &updateBlock->clipplane[i], 
                                                       sizeof(updateBlock->clipplane)) != 0) {

               TRACE("Updating clipplane %d\n", i);
               memcpy(&updateBlock->clipplane[i], &This->StateBlock->clipplane[i], 
                                       sizeof(updateBlock->clipplane));
           }
       }

       /* Render */
       for (i = 0; i < HIGHEST_RENDER_STATE; i++) {

           if (updateBlock->Set.renderstate[i] && (updateBlock->renderstate[i] != 
                                                       This->StateBlock->renderstate[i])) {
               TRACE("Updating renderstate %d to %ld\n", i, This->StateBlock->renderstate[i]);
               updateBlock->renderstate[i] = This->StateBlock->renderstate[i];
           }
       }

       /* Texture */
       for (j = 0; j < This->TextureUnits; j++) {
           for (i = 0; i < HIGHEST_TEXTURE_STATE; i++) {

               if (updateBlock->Set.texture_state[j][i] && (updateBlock->texture_state[j][i] != 
                                                                This->StateBlock->texture_state[j][i])) {
                   TRACE("Updating texturestagestate %d,%d to %ld (was %ld)\n", j,i, This->StateBlock->texture_state[j][i], 
                               updateBlock->texture_state[j][i]);
                   updateBlock->texture_state[j][i] =  This->StateBlock->texture_state[j][i];
               }

               if (updateBlock->Set.textures[j] && (updateBlock->textures[j] != This->StateBlock->textures[j])) {
                   TRACE("Updating texture %d to %p (was %p)\n", j, This->StateBlock->textures[j],  updateBlock->textures[j]);
                   updateBlock->textures[j] =  This->StateBlock->textures[j];
               }
           }

       }
    }

    TRACE("(%p) : Updated state block %p ------------------^\n", This, updateBlock);

    return D3D_OK;
}

DWORD SavedPixelStates_R[NUM_SAVEDPIXELSTATES_R] = {
    D3DRS_ALPHABLENDENABLE   ,
    D3DRS_ALPHAFUNC          ,
    D3DRS_ALPHAREF           ,
    D3DRS_ALPHATESTENABLE    ,
    D3DRS_BLENDOP            ,
    D3DRS_COLORWRITEENABLE   ,
    D3DRS_DESTBLEND          ,
    D3DRS_DITHERENABLE       ,
    D3DRS_EDGEANTIALIAS      ,
    D3DRS_FILLMODE           ,
    D3DRS_FOGDENSITY         ,
    D3DRS_FOGEND             ,
    D3DRS_FOGSTART           ,
    D3DRS_LASTPIXEL          ,
    D3DRS_LINEPATTERN        ,
    D3DRS_SHADEMODE          ,
    D3DRS_SRCBLEND           ,
    D3DRS_STENCILENABLE      ,
    D3DRS_STENCILFAIL        ,
    D3DRS_STENCILFUNC        ,
    D3DRS_STENCILMASK        ,
    D3DRS_STENCILPASS        ,
    D3DRS_STENCILREF         ,
    D3DRS_STENCILWRITEMASK   ,
    D3DRS_STENCILZFAIL       ,
    D3DRS_TEXTUREFACTOR      ,
    D3DRS_WRAP0              ,
    D3DRS_WRAP1              ,
    D3DRS_WRAP2              ,
    D3DRS_WRAP3              ,
    D3DRS_WRAP4              ,
    D3DRS_WRAP5              ,
    D3DRS_WRAP6              ,
    D3DRS_WRAP7              ,
    D3DRS_ZBIAS              ,
    D3DRS_ZENABLE            ,
    D3DRS_ZFUNC              ,
    D3DRS_ZWRITEENABLE
};

DWORD SavedPixelStates_T[NUM_SAVEDPIXELSTATES_T] = {
    D3DTSS_ADDRESSU              ,
    D3DTSS_ADDRESSV              ,
    D3DTSS_ADDRESSW              ,
    D3DTSS_ALPHAARG0             ,
    D3DTSS_ALPHAARG1             ,
    D3DTSS_ALPHAARG2             ,
    D3DTSS_ALPHAOP               ,
    D3DTSS_BORDERCOLOR           ,
    D3DTSS_BUMPENVLOFFSET        ,
    D3DTSS_BUMPENVLSCALE         ,
    D3DTSS_BUMPENVMAT00          ,
    D3DTSS_BUMPENVMAT01          ,
    D3DTSS_BUMPENVMAT10          ,
    D3DTSS_BUMPENVMAT11          ,
    D3DTSS_COLORARG0             ,
    D3DTSS_COLORARG1             ,
    D3DTSS_COLORARG2             ,
    D3DTSS_COLOROP               ,
    D3DTSS_MAGFILTER             ,
    D3DTSS_MAXANISOTROPY         ,
    D3DTSS_MAXMIPLEVEL           ,
    D3DTSS_MINFILTER             ,
    D3DTSS_MIPFILTER             ,
    D3DTSS_MIPMAPLODBIAS         ,
    D3DTSS_RESULTARG             ,
    D3DTSS_TEXCOORDINDEX         ,
    D3DTSS_TEXTURETRANSFORMFLAGS
};

DWORD SavedVertexStates_R[NUM_SAVEDVERTEXSTATES_R] = {
    D3DRS_AMBIENT                       ,
    D3DRS_AMBIENTMATERIALSOURCE         ,
    D3DRS_CLIPPING                      ,
    D3DRS_CLIPPLANEENABLE               ,
    D3DRS_COLORVERTEX                   ,
    D3DRS_DIFFUSEMATERIALSOURCE         ,
    D3DRS_EMISSIVEMATERIALSOURCE        ,
    D3DRS_FOGDENSITY                    ,
    D3DRS_FOGEND                        ,
    D3DRS_FOGSTART                      ,
    D3DRS_FOGTABLEMODE                  ,
    D3DRS_FOGVERTEXMODE                 ,
    D3DRS_INDEXEDVERTEXBLENDENABLE      ,
    D3DRS_LIGHTING                      ,
    D3DRS_LOCALVIEWER                   ,
    D3DRS_MULTISAMPLEANTIALIAS          ,
    D3DRS_MULTISAMPLEMASK               ,
    D3DRS_NORMALIZENORMALS              ,
    D3DRS_PATCHEDGESTYLE                ,
    D3DRS_PATCHSEGMENTS                 ,
    D3DRS_POINTSCALE_A                  ,
    D3DRS_POINTSCALE_B                  ,
    D3DRS_POINTSCALE_C                  ,
    D3DRS_POINTSCALEENABLE              ,
    D3DRS_POINTSIZE                     ,
    D3DRS_POINTSIZE_MAX                 ,
    D3DRS_POINTSIZE_MIN                 ,
    D3DRS_POINTSPRITEENABLE             ,
    D3DRS_RANGEFOGENABLE                ,
    D3DRS_SOFTWAREVERTEXPROCESSING      ,
    D3DRS_SPECULARMATERIALSOURCE        ,
    D3DRS_TWEENFACTOR                   ,
    D3DRS_VERTEXBLEND
};

DWORD SavedVertexStates_T[NUM_SAVEDVERTEXSTATES_T] = {
    D3DTSS_TEXCOORDINDEX         ,
    D3DTSS_TEXTURETRANSFORMFLAGS
};
