/*
 * state block implementation
 *
 * Copyright 2002 Raphael Junqueira
 * Copyright 2004 Jason Edmeades
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
#define GLINFO_LOCATION ((IWineD3DImpl *)(((IWineD3DDeviceImpl *)This->wineD3DDevice)->wineD3D))->gl_info

/**********************************************************
 * IWineD3DStateBlockImpl IUnknown parts follows
 **********************************************************/
HRESULT WINAPI IWineD3DStateBlockImpl_QueryInterface(IWineD3DStateBlock *iface,REFIID riid,LPVOID *ppobj)
{
    IWineD3DStateBlockImpl *This = (IWineD3DStateBlockImpl *)iface;
    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DStateBlock)){
        IUnknown_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }
    return E_NOINTERFACE;
}

ULONG WINAPI IWineD3DStateBlockImpl_AddRef(IWineD3DStateBlock *iface) {
    IWineD3DStateBlockImpl *This = (IWineD3DStateBlockImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef increasing from %ld\n", This, refCount - 1);
    return refCount;
}

ULONG WINAPI IWineD3DStateBlockImpl_Release(IWineD3DStateBlock *iface) {
    IWineD3DStateBlockImpl *This = (IWineD3DStateBlockImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p) : Releasing from %ld\n", This, refCount + 1);

    if (!refCount) {
        IWineD3DDevice_Release((IWineD3DDevice *)This->wineD3DDevice);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return refCount;
}

/**********************************************************
 * IWineD3DStateBlockImpl parts follows
 **********************************************************/
HRESULT WINAPI IWineD3DStateBlockImpl_GetParent(IWineD3DStateBlock *iface, IUnknown **pParent) {
    IWineD3DStateBlockImpl *This = (IWineD3DStateBlockImpl *)iface;
    IUnknown_AddRef(This->parent);
    *pParent = This->parent;
    return D3D_OK;
}

HRESULT WINAPI IWineD3DStateBlockImpl_InitStartupStateBlock(IWineD3DStateBlock* iface) {
    IWineD3DStateBlockImpl *This = (IWineD3DStateBlockImpl *)iface;
    IWineD3DDeviceImpl     *ThisDevice = (IWineD3DDeviceImpl *)(This->wineD3DDevice);
    union {
        D3DLINEPATTERN lp;
        DWORD d;
    } lp;
    union {
        float f;
        DWORD d;
    } tmpfloat;
    unsigned int i;

    /* Note this may have a large overhead but it should only be executed
       once, in order to initialize the complete state of the device and 
       all opengl equivalents                                            */
    TRACE("-----------------------> Setting up device defaults...\n");
    This->blockType = D3DSBT_ALL;

    /* FIXME: Set some of the defaults for lights, transforms etc */
    memcpy(&This->transforms[D3DTS_PROJECTION], &identity, sizeof(identity));
    memcpy(&This->transforms[D3DTS_VIEW], &identity, sizeof(identity));
    for (i = 0; i < 256; ++i) {
      memcpy(&This->transforms[D3DTS_WORLDMATRIX(i)], &identity, sizeof(identity));
    }
 
    /* Render states: */
    if (ThisDevice->presentParms.EnableAutoDepthStencil) {
       IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_ZENABLE, D3DZB_TRUE);
    } else {
       IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_ZENABLE, D3DZB_FALSE);
    }
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_FILLMODE, D3DFILL_SOLID);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_SHADEMODE, D3DSHADE_GOURAUD);

    lp.lp.wRepeatFactor = 0; lp.lp.wLinePattern = 0;
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_LINEPATTERN, lp.d);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_ZWRITEENABLE, TRUE);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_ALPHATESTENABLE, FALSE);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_LASTPIXEL, TRUE);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_SRCBLEND, D3DBLEND_ONE);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_DESTBLEND, D3DBLEND_ZERO);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_CULLMODE, D3DCULL_CCW);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_ALPHAFUNC, D3DCMP_ALWAYS);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_ALPHAREF, 0xff); /*??*/
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_DITHERENABLE, FALSE);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_ALPHABLENDENABLE, FALSE);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_FOGENABLE, FALSE);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_SPECULARENABLE, FALSE);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_ZVISIBLE, 0);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_FOGCOLOR, 0);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_FOGTABLEMODE, D3DFOG_NONE);
    tmpfloat.f = 0.0f;
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_FOGSTART, tmpfloat.d);
    tmpfloat.f = 1.0f;
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_FOGEND, tmpfloat.d);
    tmpfloat.f = 1.0f;
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_FOGDENSITY, tmpfloat.d);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_EDGEANTIALIAS, FALSE);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_ZBIAS, 0);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_RANGEFOGENABLE, FALSE);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_STENCILENABLE, FALSE);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_STENCILPASS, D3DSTENCILOP_KEEP);

    /* Setting stencil func also uses values for stencil ref/mask, so manually set defaults
     * so only a single call performed (and ensure defaults initialized before making that call)    
     *
     * IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_STENCILREF, 0);
     * IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_STENCILMASK, 0xFFFFFFFF);
     */
    This->renderState[WINED3DRS_STENCILREF] = 0;
    This->renderState[WINED3DRS_STENCILMASK] = 0xFFFFFFFF;
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_STENCILFUNC, D3DCMP_ALWAYS);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_STENCILWRITEMASK, 0xFFFFFFFF);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_TEXTUREFACTOR, 0xFFFFFFFF);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_WRAP0, 0);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_WRAP1, 0);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_WRAP2, 0);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_WRAP3, 0);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_WRAP4, 0);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_WRAP5, 0);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_WRAP6, 0);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_WRAP7, 0);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_CLIPPING, TRUE);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_LIGHTING, TRUE);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_AMBIENT, 0);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_FOGVERTEXMODE, D3DFOG_NONE);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_COLORVERTEX, TRUE);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_LOCALVIEWER, TRUE);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_NORMALIZENORMALS, FALSE);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_SPECULARMATERIALSOURCE, D3DMCS_COLOR2);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_AMBIENTMATERIALSOURCE, D3DMCS_COLOR2);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_VERTEXBLEND, D3DVBF_DISABLE);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_CLIPPLANEENABLE, 0);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_SOFTWAREVERTEXPROCESSING, FALSE);
    tmpfloat.f = 1.0f;
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_POINTSIZE, tmpfloat.d);
    tmpfloat.f = 0.0f;
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_POINTSIZE_MIN, tmpfloat.d);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_POINTSPRITEENABLE, FALSE);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_POINTSCALEENABLE, FALSE);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_POINTSCALE_A, TRUE);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_POINTSCALE_B, TRUE);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_POINTSCALE_C, TRUE);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_MULTISAMPLEANTIALIAS, TRUE);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_MULTISAMPLEMASK, 0xFFFFFFFF);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_PATCHEDGESTYLE, D3DPATCHEDGE_DISCRETE);
    tmpfloat.f = 1.0f;
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_PATCHSEGMENTS, tmpfloat.d);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_DEBUGMONITORTOKEN, D3DDMT_DISABLE);
    tmpfloat.f = 64.0f;
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_POINTSIZE_MAX, tmpfloat.d);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_INDEXEDVERTEXBLENDENABLE, FALSE);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_COLORWRITEENABLE, 0x0000000F);
    tmpfloat.f = 0.0f;
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_TWEENFACTOR, tmpfloat.d);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_BLENDOP, D3DBLENDOP_ADD);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_POSITIONORDER, WINED3DDEGREE_CUBIC);
    IWineD3DDevice_SetRenderState((IWineD3DDevice *)This->wineD3DDevice, WINED3DRS_NORMALORDER, WINED3DDEGREE_LINEAR);

    /** clipping status */
    This->clip_status.ClipUnion = 0;
    This->clip_status.ClipIntersection = 0xFFFFFFFF;
        
    /* Texture Stage States - Put directly into state block, we will call function below */
    for (i = 0; i < GL_LIMITS(textures); i++) {
        TRACE("Setting up default texture states for texture Stage %d\n", i);
        memcpy(&This->transforms[D3DTS_TEXTURE0 + i], &identity, sizeof(identity));
        This->textureState[i][D3DTSS_COLOROP               ] = (i==0)? D3DTOP_MODULATE :  D3DTOP_DISABLE;
        This->textureState[i][D3DTSS_COLORARG1             ] = D3DTA_TEXTURE;
        This->textureState[i][D3DTSS_COLORARG2             ] = D3DTA_CURRENT;
        This->textureState[i][D3DTSS_ALPHAOP               ] = (i==0)? D3DTOP_SELECTARG1 :  D3DTOP_DISABLE;
        This->textureState[i][D3DTSS_ALPHAARG1             ] = D3DTA_TEXTURE;
        This->textureState[i][D3DTSS_ALPHAARG2             ] = D3DTA_CURRENT;
        This->textureState[i][D3DTSS_BUMPENVMAT00          ] = (DWORD) 0.0;
        This->textureState[i][D3DTSS_BUMPENVMAT01          ] = (DWORD) 0.0;
        This->textureState[i][D3DTSS_BUMPENVMAT10          ] = (DWORD) 0.0;
        This->textureState[i][D3DTSS_BUMPENVMAT11          ] = (DWORD) 0.0;
        This->textureState[i][D3DTSS_TEXCOORDINDEX         ] = i;
        This->textureState[i][D3DTSS_ADDRESSU              ] = D3DTADDRESS_WRAP;
        This->textureState[i][D3DTSS_ADDRESSV              ] = D3DTADDRESS_WRAP;
        This->textureState[i][D3DTSS_BORDERCOLOR           ] = 0x00;
        This->textureState[i][D3DTSS_MAGFILTER             ] = D3DTEXF_POINT;
        This->textureState[i][D3DTSS_MINFILTER             ] = D3DTEXF_POINT;
        This->textureState[i][D3DTSS_MIPFILTER             ] = D3DTEXF_NONE;
        This->textureState[i][D3DTSS_MIPMAPLODBIAS         ] = 0;
        This->textureState[i][D3DTSS_MAXMIPLEVEL           ] = 0;
        This->textureState[i][D3DTSS_MAXANISOTROPY         ] = 1;
        This->textureState[i][D3DTSS_BUMPENVLSCALE         ] = (DWORD) 0.0;
        This->textureState[i][D3DTSS_BUMPENVLOFFSET        ] = (DWORD) 0.0;
        This->textureState[i][D3DTSS_TEXTURETRANSFORMFLAGS ] = D3DTTFF_DISABLE;
        This->textureState[i][D3DTSS_ADDRESSW              ] = D3DTADDRESS_WRAP;
        This->textureState[i][D3DTSS_COLORARG0             ] = D3DTA_CURRENT;
        This->textureState[i][D3DTSS_ALPHAARG0             ] = D3DTA_CURRENT;
        This->textureState[i][D3DTSS_RESULTARG             ] = D3DTA_CURRENT;
    }

    /* Under DirectX you can have texture stage operations even if no texture is
       bound, whereas opengl will only do texture operations when a valid texture is
       bound. We emulate this by creating dummy textures and binding them to each
       texture stage, but disable all stages by default. Hence if a stage is enabled
       then the default texture will kick in until replaced by a SetTexture call     */

    ENTER_GL();

    for (i = 0; i < GL_LIMITS(textures); i++) {
        GLubyte white = 255;

        /* Note this avoids calling settexture, so pretend it has been called */
        This->set.textures[i] = TRUE;
        This->changed.textures[i] = TRUE;
        This->textures[i] = NULL;

        /* Make appropriate texture active */
        if (GL_SUPPORT(ARB_MULTITEXTURE)) {
            GLACTIVETEXTURE(i);
        } else if (i > 0) {
            FIXME("Program using multiple concurrent textures which this opengl implementation doesn't support\n");
        }

        /* Generate an opengl texture name */
        glGenTextures(1, &ThisDevice->dummyTextureName[i]);
        checkGLcall("glGenTextures");
        TRACE("Dummy Texture %d given name %d\n", i, ThisDevice->dummyTextureName[i]);

        /* Generate a dummy 1d texture */
        This->textureDimensions[i] = GL_TEXTURE_1D;
        glBindTexture(GL_TEXTURE_1D, ThisDevice->dummyTextureName[i]);
        checkGLcall("glBindTexture");

        glTexImage1D(GL_TEXTURE_1D, 0, GL_LUMINANCE, 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, &white); 
        checkGLcall("glTexImage1D");

        /* Reapply all the texture state information to this texture */
        IWineD3DDevice_SetupTextureStates((IWineD3DDevice *)This->wineD3DDevice, i, REAPPLY_ALL);
    }

    LEAVE_GL();

    /* Defaulting palettes - Note these are device wide but reinitialized here for convenience*/
    for (i = 0; i < MAX_PALETTES; ++i) {
      int j;
      for (j = 0; j < 256; ++j) {
        This->wineD3DDevice->palettes[i][j].peRed   = 0xFF;
        This->wineD3DDevice->palettes[i][j].peGreen = 0xFF;
        This->wineD3DDevice->palettes[i][j].peBlue  = 0xFF;
        This->wineD3DDevice->palettes[i][j].peFlags = 0xFF;
      }
    }
    This->wineD3DDevice->currentPalette = 0;

    TRACE("-----------------------> Device defaults now set up...\n");
    return D3D_OK;
}

/**********************************************************
 * IWineD3DStateBlock VTbl follows
 **********************************************************/

const IWineD3DStateBlockVtbl IWineD3DStateBlock_Vtbl =
{
    IWineD3DStateBlockImpl_QueryInterface,
    IWineD3DStateBlockImpl_AddRef,
    IWineD3DStateBlockImpl_Release,
    IWineD3DStateBlockImpl_GetParent,
    IWineD3DStateBlockImpl_InitStartupStateBlock
};
