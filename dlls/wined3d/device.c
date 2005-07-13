/*
 * IWineD3DDevice implementation
 *
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2003-2004 Raphael Junqueira
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(d3d_caps);
WINE_DECLARE_DEBUG_CHANNEL(d3d_fps);
WINE_DECLARE_DEBUG_CHANNEL(d3d_shader);
#define GLINFO_LOCATION ((IWineD3DImpl *)(This->wineD3D))->gl_info

/* x11drv GDI escapes */
#define X11DRV_ESCAPE 6789
enum x11drv_escape_codes
{
    X11DRV_GET_DISPLAY,   /* get X11 display for a DC */
    X11DRV_GET_DRAWABLE,  /* get current drawable for a DC */
    X11DRV_GET_FONT,      /* get current X font for a DC */
};

/* retrieve the X display to use on a given DC */
inline static Display *get_display( HDC hdc )
{
    Display *display;
    enum x11drv_escape_codes escape = X11DRV_GET_DISPLAY;

    if (!ExtEscape( hdc, X11DRV_ESCAPE, sizeof(escape), (LPCSTR)&escape,
                    sizeof(display), (LPSTR)&display )) display = NULL;
    return display;
}

/* Memory tracking and object counting */
static unsigned int emulated_textureram = 64*1024*1024;

/* helper macros */
#define D3DMEMCHECK(object, ppResult) if(NULL == object) { *ppResult = NULL; WARN("Out of memory\n"); return D3DERR_OUTOFVIDEOMEMORY;}

#define D3DCREATEOBJECTINSTANCE(object, type) { \
    object=HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IWineD3D##type##Impl)); \
    D3DMEMCHECK(object, pp##type); \
    object->lpVtbl = &IWineD3D##type##_Vtbl;  \
    object->wineD3DDevice = This; \
    object->parent       = parent; \
    object->ref          = 1; \
    *pp##type = (IWineD3D##type *) object; \
}

#define  D3DCREATERESOURCEOBJECTINSTANCE(object, type, d3dtype, _size){ \
    object=HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IWineD3D##type##Impl)); \
    D3DMEMCHECK(object, pp##type); \
    object->lpVtbl = &IWineD3D##type##_Vtbl;  \
    object->resource.wineD3DDevice   = This; \
    object->resource.parent          = parent; \
    object->resource.resourceType    = d3dtype; \
    object->resource.ref             = 1; \
    object->resource.pool            = Pool; \
    object->resource.format          = Format; \
    object->resource.usage           = Usage; \
    object->resource.size            = _size; \
    /* Check that we have enough video ram left */ \
    if (Pool == D3DPOOL_DEFAULT) { \
        if (IWineD3DDevice_GetAvailableTextureMem(iface) <= _size) { \
            WARN("Out of 'bogus' video memory\n"); \
            HeapFree(GetProcessHeap(),0,object); \
            *pp##type = NULL; \
            return D3DERR_OUTOFVIDEOMEMORY; \
        } \
        globalChangeGlRam(_size); \
    } \
    object->resource.allocatedMemory = (0 == _size ? NULL : Pool == D3DPOOL_DEFAULT ? NULL : HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, _size)); \
    if (object->resource.allocatedMemory == NULL && _size != 0 && Pool != D3DPOOL_DEFAULT) { \
        FIXME("Out of memory!\n"); \
        HeapFree(GetProcessHeap(), 0, object); \
        *pp##type = NULL; \
        return D3DERR_OUTOFVIDEOMEMORY; \
    } \
    *pp##type = (IWineD3D##type *) object; \
    TRACE("(%p) : Created resource %p\n", This, object); \
}

#define D3DINITILIZEBASETEXTURE(_basetexture) { \
    _basetexture.levels     = Levels; \
    _basetexture.filterType = (Usage & D3DUSAGE_AUTOGENMIPMAP) ? D3DTEXF_LINEAR : D3DTEXF_NONE; \
    _basetexture.LOD        = 0; \
    _basetexture.dirty      = TRUE; \
}

/**********************************************************
 * Global variable / Constants follow
 **********************************************************/
const float identity[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};  /* When needed for comparisons */

/**********************************************************
 * Utility functions follow
 **********************************************************/
/* Convert the D3DLIGHT properties into equivalent gl lights */
void setup_light(IWineD3DDevice *iface, LONG Index, PLIGHTINFOEL *lightInfo) {

    float quad_att;
    float colRGBA[] = {0.0, 0.0, 0.0, 0.0};
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    /* Light settings are affected by the model view in OpenGL, the View transform in direct3d*/
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixf((float *) &This->stateBlock->transforms[D3DTS_VIEW].u.m[0][0]);

    /* Diffuse: */
    colRGBA[0] = lightInfo->OriginalParms.Diffuse.r;
    colRGBA[1] = lightInfo->OriginalParms.Diffuse.g;
    colRGBA[2] = lightInfo->OriginalParms.Diffuse.b;
    colRGBA[3] = lightInfo->OriginalParms.Diffuse.a;
    glLightfv(GL_LIGHT0+Index, GL_DIFFUSE, colRGBA);
    checkGLcall("glLightfv");

    /* Specular */
    colRGBA[0] = lightInfo->OriginalParms.Specular.r;
    colRGBA[1] = lightInfo->OriginalParms.Specular.g;
    colRGBA[2] = lightInfo->OriginalParms.Specular.b;
    colRGBA[3] = lightInfo->OriginalParms.Specular.a;
    glLightfv(GL_LIGHT0+Index, GL_SPECULAR, colRGBA);
    checkGLcall("glLightfv");

    /* Ambient */
    colRGBA[0] = lightInfo->OriginalParms.Ambient.r;
    colRGBA[1] = lightInfo->OriginalParms.Ambient.g;
    colRGBA[2] = lightInfo->OriginalParms.Ambient.b;
    colRGBA[3] = lightInfo->OriginalParms.Ambient.a;
    glLightfv(GL_LIGHT0+Index, GL_AMBIENT, colRGBA);
    checkGLcall("glLightfv");

    /* Attenuation - Are these right? guessing... */
    glLightf(GL_LIGHT0+Index, GL_CONSTANT_ATTENUATION,  lightInfo->OriginalParms.Attenuation0);
    checkGLcall("glLightf");
    glLightf(GL_LIGHT0+Index, GL_LINEAR_ATTENUATION,    lightInfo->OriginalParms.Attenuation1);
    checkGLcall("glLightf");

    quad_att = 1.4/(lightInfo->OriginalParms.Range*lightInfo->OriginalParms.Range);
    if (quad_att < lightInfo->OriginalParms.Attenuation2) quad_att = lightInfo->OriginalParms.Attenuation2;
    glLightf(GL_LIGHT0+Index, GL_QUADRATIC_ATTENUATION, quad_att);
    checkGLcall("glLightf");

    switch (lightInfo->OriginalParms.Type) {
    case D3DLIGHT_POINT:
        /* Position */
        glLightfv(GL_LIGHT0+Index, GL_POSITION, &lightInfo->lightPosn[0]);
        checkGLcall("glLightfv");
        glLightf(GL_LIGHT0 + Index, GL_SPOT_CUTOFF, lightInfo->cutoff);
        checkGLcall("glLightf");
        /* FIXME: Range */
        break;

    case D3DLIGHT_SPOT:
        /* Position */
        glLightfv(GL_LIGHT0+Index, GL_POSITION, &lightInfo->lightPosn[0]);
        checkGLcall("glLightfv");
        /* Direction */
        glLightfv(GL_LIGHT0+Index, GL_SPOT_DIRECTION, &lightInfo->lightDirn[0]);
        checkGLcall("glLightfv");
        glLightf(GL_LIGHT0 + Index, GL_SPOT_EXPONENT, lightInfo->exponent);
        checkGLcall("glLightf");
        glLightf(GL_LIGHT0 + Index, GL_SPOT_CUTOFF, lightInfo->cutoff);
        checkGLcall("glLightf");
        /* FIXME: Range */
        break;

    case D3DLIGHT_DIRECTIONAL:
        /* Direction */
        glLightfv(GL_LIGHT0+Index, GL_POSITION, &lightInfo->lightPosn[0]); /* Note gl uses w position of 0 for direction! */
        checkGLcall("glLightfv");
        glLightf(GL_LIGHT0+Index, GL_SPOT_CUTOFF, lightInfo->cutoff);
        checkGLcall("glLightf");
        glLightf(GL_LIGHT0+Index, GL_SPOT_EXPONENT, 0.0f);
        checkGLcall("glLightf");
        break;

    default:
        FIXME("Unrecognized light type %d\n", lightInfo->OriginalParms.Type);
    }

    /* Restore the modelview matrix */
    glPopMatrix();
}

/* Apply the current values to the specified texture stage */
void WINAPI IWineD3DDeviceImpl_SetupTextureStates(IWineD3DDevice *iface, DWORD Stage, DWORD Flags) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    int i = 0;
    float col[4];
    BOOL changeTexture = TRUE;

    TRACE("-----------------------> Updating the texture at stage %ld to have new texture state information\n", Stage);
    for (i = 1; i < HIGHEST_TEXTURE_STATE; i++) {

        BOOL skip = FALSE;

        switch (i) {
        /* Performance: For texture states where multiples effect the outcome, only bother
              applying the last one as it will pick up all the other values                */
        case WINED3DTSS_COLORARG0:  /* Will be picked up when setting color op */
        case WINED3DTSS_COLORARG1:  /* Will be picked up when setting color op */
        case WINED3DTSS_COLORARG2:  /* Will be picked up when setting color op */
        case WINED3DTSS_ALPHAARG0:  /* Will be picked up when setting alpha op */
        case WINED3DTSS_ALPHAARG1:  /* Will be picked up when setting alpha op */
        case WINED3DTSS_ALPHAARG2:  /* Will be picked up when setting alpha op */
           skip = TRUE;
           break;

        /* Performance: If the texture states only impact settings for the texture unit
             (compared to the texture object) then there is no need to reapply them. The
             only time they need applying is the first time, since we cheat and put the
             values into the stateblock without applying.
             Per-texture unit: texture function (eg. combine), ops and args
                               texture env color
                               texture generation settings
           Note: Due to some special conditions there may be a need to do particular ones
             of these, which is what the Flags allows                                     */
        case WINED3DTSS_COLOROP:
        case WINED3DTSS_TEXCOORDINDEX:
            if (!(Flags == REAPPLY_ALL)) skip=TRUE;
            break;

        case WINED3DTSS_ALPHAOP:
            if (!(Flags & REAPPLY_ALPHAOP)) skip=TRUE;
            break;

        default:
            skip = FALSE;
        }

        if (skip == FALSE) {
           /* Performance: Only change to this texture if we have to */
           if (changeTexture) {
               /* Make appropriate texture active */
               if (GL_SUPPORT(ARB_MULTITEXTURE)) {
                   GLACTIVETEXTURE(Stage);
                } else if (Stage > 0) {
                    FIXME("Program using multiple concurrent textures which this opengl implementation doesn't support\n");
                }
                changeTexture = FALSE;
           }

           /* Now apply the change */
           IWineD3DDevice_SetTextureStageState(iface, Stage, i, This->stateBlock->textureState[Stage][i]);
        }
    }

    /* apply the sampler states to the texture */
    for (i = 1; i <= HIGHEST_SAMPLER_STATE;i++) {
      IWineD3DDevice_SetSamplerState(iface, Stage, i, This->stateBlock->samplerState[Stage][i]);
    }

    /* Note the D3DRS value applies to all textures, but GL has one
     *  per texture, so apply it now ready to be used!
     */
    D3DCOLORTOGLFLOAT4(This->stateBlock->renderState[WINED3DRS_TEXTUREFACTOR], col);
    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, &col[0]);
    checkGLcall("glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);");

    TRACE("-----------------------> Updated the texture at stage %ld to have new texture state information\n", Stage);
}

/**********************************************************
 * IUnknown parts follows
 **********************************************************/

HRESULT WINAPI IWineD3DDeviceImpl_QueryInterface(IWineD3DDevice *iface,REFIID riid,LPVOID *ppobj)
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    /* FIXME: This needs to extend an IWineD3DBaseObject */

    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DDevice)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }

    return E_NOINTERFACE;
}

ULONG WINAPI IWineD3DDeviceImpl_AddRef(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef increasing from %ld\n", This, refCount - 1);
    return refCount;
}

ULONG WINAPI IWineD3DDeviceImpl_Release(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p) : Releasing from %ld\n", This, refCount + 1);

    if (!refCount) {
        /* TODO: Clean up all the surfaces and textures! */
        /* FIXME: Create targets and state blocks in d3d8 */
        if (((IWineD3DImpl *)This->wineD3D)->dxVersion > 8) { /*We don't create a state block in d3d8 yet*/
            /* NOTE: You must release the parent if the object was created via a callback
            ** ***************************/
            int i;
            IUnknown* swapChainParent;

            /* Release all of the swapchains, except the implicit swapchain (#0) */
            for(i = 1; i < This->numberOfSwapChains; i++) {
                /*  TODO: don't access swapchains[x] directly! */
                IWineD3DSwapChain_Release(This->swapchains[i]);
            }

            if (This->stateBlock != NULL) {
                IWineD3DStateBlock_Release((IWineD3DStateBlock *)This->stateBlock);
            }

            if (This->swapchains[0] != NULL) {
                /* Swapchain 0 is special because it's created in startup with a hanging parent, so we have to release its parent now */
                /*  TODO: don't access swapchains[x] directly!, check that there are no-more swapchains left for this device! */
                IWineD3DSwapChain_GetParent(This->swapchains[0], &swapChainParent);
                IUnknown_Release(swapChainParent);           /* once for the get parent */
                if (IUnknown_Release(swapChainParent)  > 0) {  /* the second time for when it was created */
                    FIXME("(%p) Something's still holding the implicit swapchain\n",This);
                }
            }

        }
        IWineD3D_Release(This->wineD3D);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return refCount;
}

/**********************************************************
 * IWineD3DDevice implementation follows
 **********************************************************/
HRESULT WINAPI IWineD3DDeviceImpl_GetParent(IWineD3DDevice *iface, IUnknown **pParent) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    *pParent = This->parent;
    IUnknown_AddRef(This->parent);
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_CreateVertexBuffer(IWineD3DDevice *iface, UINT Size, DWORD Usage, 
                             DWORD FVF, D3DPOOL Pool, IWineD3DVertexBuffer** ppVertexBuffer, HANDLE *sharedHandle,
                             IUnknown *parent) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DVertexBufferImpl *object;
    WINED3DFORMAT Format = WINED3DFMT_VERTEXDATA; /* Dummy format for now */
    D3DCREATERESOURCEOBJECTINSTANCE(object, VertexBuffer, D3DRTYPE_VERTEXBUFFER, Size)
    
    /*TODO: use VBO's */
    if (Pool == D3DPOOL_DEFAULT ) { /* Allocate some system memory for now */
        object->resource.allocatedMemory  = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, object->resource.size);
    }
    object->fvf = FVF;

    TRACE("(%p) : Size=%d, Usage=%ld, FVF=%lx, Pool=%d - Memory@%p, Iface@%p\n", This, Size, Usage, FVF, Pool, object->resource.allocatedMemory, object);
    *ppVertexBuffer = (IWineD3DVertexBuffer *)object;

    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_CreateIndexBuffer(IWineD3DDevice *iface, UINT Length, DWORD Usage, 
                                                    WINED3DFORMAT Format, D3DPOOL Pool, IWineD3DIndexBuffer** ppIndexBuffer,
                                                    HANDLE *sharedHandle, IUnknown *parent) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DIndexBufferImpl *object;
    TRACE("(%p) Creating index buffer\n", This);
    
    /* Allocate the storage for the device */
    D3DCREATERESOURCEOBJECTINSTANCE(object,IndexBuffer,D3DRTYPE_INDEXBUFFER, Length)
    
    /*TODO: use VBO's */
    if (Pool == D3DPOOL_DEFAULT ) { /* Allocate some system memory for now */
        object->resource.allocatedMemory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,object->resource.size);
    }

    TRACE("(%p) : Len=%d, Use=%lx, Format=(%u,%s), Pool=%d - Memory@%p, Iface@%p\n", This, Length, Usage, Format, 
                           debug_d3dformat(Format), Pool, object, object->resource.allocatedMemory);
    *ppIndexBuffer = (IWineD3DIndexBuffer *) object;

    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_CreateStateBlock(IWineD3DDevice* iface, D3DSTATEBLOCKTYPE Type, IWineD3DStateBlock** ppStateBlock, IUnknown *parent) {

    IWineD3DDeviceImpl     *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DStateBlockImpl *object;
    int i,j;

    D3DCREATEOBJECTINSTANCE(object, StateBlock)
    object->blockType     = Type;

    /* Special case - Used during initialization to produce a placeholder stateblock
          so other functions called can update a state block                         */
    if (Type == (D3DSTATEBLOCKTYPE) 0) {
        /* Don't bother increasing the reference count otherwise a device will never
           be freed due to circular dependencies                                   */
        return D3D_OK;
    }

    /* Otherwise, might as well set the whole state block to the appropriate values */
    IWineD3DDevice_AddRef(iface);
    /* Otherwise, might as well set the whole state block to the appropriate values  */
    if ( This->stateBlock != NULL) {
       memcpy(object, This->stateBlock, sizeof(IWineD3DStateBlockImpl));
    } else {
       memset(object->streamFreq, 1, sizeof(object->streamFreq));
    }

    /* Reset the ref and type after kludging it */
    object->wineD3DDevice = This;
    object->ref           = 1;
    object->blockType     = Type;

    TRACE("Updating changed flags appropriate for type %d\n", Type);

    if (Type == D3DSBT_ALL) {
        TRACE("ALL => Pretend everything has changed\n");
        memset(&object->changed, TRUE, sizeof(This->stateBlock->changed));
    } else if (Type == D3DSBT_PIXELSTATE) {

        memset(&object->changed, FALSE, sizeof(This->stateBlock->changed));
        /* TODO: Pixel Shader Constants */
        object->changed.pixelShader = TRUE;
        for (i = 0; i < NUM_SAVEDPIXELSTATES_R; i++) {
            object->changed.renderState[SavedPixelStates_R[i]] = TRUE;
        }
        for (j = 0; j < GL_LIMITS(textures); i++) {
            for (i = 0; i < NUM_SAVEDPIXELSTATES_T; i++) {
                object->changed.textureState[j][SavedPixelStates_T[i]] = TRUE;
            }
        }
        /* Setting sampler block changes states */
        for (j = 0 ; j < GL_LIMITS(samplers); j++) {
            for (i =0; i < NUM_SAVEDPIXELSTATES_S;i++) {

                object->changed.samplerState[j][SavedPixelStates_S[i]] = TRUE;
            }
        }
    } else if (Type == D3DSBT_VERTEXSTATE) {

        memset(&object->changed, FALSE, sizeof(This->stateBlock->changed));

        /* TODO: Vertex Shader Constants */
        object->changed.vertexShader = TRUE;
        for (i = 0; i < NUM_SAVEDVERTEXSTATES_R; i++) {
            object->changed.renderState[SavedVertexStates_R[i]] = TRUE;
        }
        for (j = 0; j < GL_LIMITS(textures); i++) {
            for (i = 0; i < NUM_SAVEDVERTEXSTATES_T; i++) {
                object->changed.textureState[j][SavedVertexStates_T[i]] = TRUE;
            }
        }
        /* Setting sampler block changes states */
        for (j = 0 ; j < GL_LIMITS(samplers); j++) {
            for (i =0; i < NUM_SAVEDVERTEXSTATES_S;i++) {
                object->changed.samplerState[j][SavedVertexStates_S[i]] = TRUE;
            }
        }

    /* Duplicate light chain */
    {
        PLIGHTINFOEL *src = NULL;
        PLIGHTINFOEL *dst = NULL;
        PLIGHTINFOEL *newEl = NULL;
        src = This->stateBlock->lights;
        object->lights = NULL;


        while (src) {
            newEl = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PLIGHTINFOEL));
            if (newEl == NULL) return D3DERR_OUTOFVIDEOMEMORY;
            memcpy(newEl, src, sizeof(PLIGHTINFOEL));
            newEl->prev = dst;
            newEl->changed = TRUE;
            newEl->enabledChanged = TRUE;
            if (dst == NULL) {
                object->lights = newEl;
            } else {
                dst->next = newEl;
            }
            dst = newEl;
            src = src->next;
        }

     }

    } else {
        FIXME("Unrecognized state block type %d\n", Type);
    }

    TRACE("(%p) returning token (ptr to stateblock) of %p\n", This, object);
    return D3D_OK;
}


/* ************************************
MSDN:
[in] Render targets are not lockable unless the application specifies TRUE for Lockable. Note that lockable render targets reduce performance on some graphics hardware.

Discard
 [in] Set this flag to TRUE to enable z-buffer discarding, and FALSE otherwise. 

If this flag is set, the contents of the depth stencil buffer will be invalid after calling either IDirect3DDevice9::Present or IDirect3DDevice9::SetDepthStencilSurface with a different depth surface.

******************************** */
 
HRESULT  WINAPI IWineD3DDeviceImpl_CreateSurface(IWineD3DDevice *iface, UINT Width, UINT Height, WINED3DFORMAT Format, BOOL Lockable, BOOL Discard, UINT Level, IWineD3DSurface **ppSurface,D3DRESOURCETYPE Type, DWORD Usage, D3DPOOL Pool, D3DMULTISAMPLE_TYPE MultiSample ,DWORD MultisampleQuality, HANDLE* pSharedHandle, IUnknown *parent) {
    IWineD3DDeviceImpl  *This = (IWineD3DDeviceImpl *)iface;    
    IWineD3DSurfaceImpl *object; /*NOTE: impl ref allowed since this is a create function */
    unsigned int pow2Width, pow2Height;
    unsigned int Size       = 1;
    TRACE("(%p) Create surface\n",This);
    
    /** FIXME: Check ranges on the inputs are valid 
     * MSDN
     *   MultisampleQuality
     *    [in] Quality level. The valid range is between zero and one less than the level
     *    returned by pQualityLevels used by IDirect3D9::CheckDeviceMultiSampleType. 
     *    Passing a larger value returns the error D3DERR_INVALIDCALL. The MultisampleQuality
     *    values of paired render targets, depth stencil surfaces, and the MultiSample type
     *    must all match.
      *******************************/


    /**
    * TODO: Discard MSDN
    * [in] Set this flag to TRUE to enable z-buffer discarding, and FALSE otherwise.
    *
    * If this flag is set, the contents of the depth stencil buffer will be
    * invalid after calling either IDirect3DDevice9::Present or  * IDirect3DDevice9::SetDepthStencilSurface
    * with a different depth surface.
    *
    *This flag has the same behavior as the constant, D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL, in D3DPRESENTFLAG.
    ***************************/

    if(MultisampleQuality < 0) {
        FIXME("Invalid multisample level %ld \n", MultisampleQuality);
        return D3DERR_INVALIDCALL; /* TODO: Check that this is the case! */
    }

    if(MultisampleQuality > 0) {
        FIXME("MultisampleQuality set to %ld, substituting 0  \n" , MultisampleQuality);
        MultisampleQuality=0;
    }

    /* Non-power2 support */

    /* Find the nearest pow2 match */
    pow2Width = pow2Height = 1;
    while (pow2Width < Width) pow2Width <<= 1;
    while (pow2Height < Height) pow2Height <<= 1;

    if (pow2Width > Width || pow2Height > Height) {
         /** TODO: add support for non power two compressed textures (OpenGL 2 provices support for * non-power-two textures gratis) **/
        if (Format == WINED3DFMT_DXT1 || Format == WINED3DFMT_DXT2 || Format == WINED3DFMT_DXT3
               || Format == WINED3DFMT_DXT4 || Format == WINED3DFMT_DXT5) {
            FIXME("(%p) Compressed non-power-two textures are not supported w(%d) h(%d) \n",
                    This, Width, Height);
            return D3DERR_NOTAVAILABLE;
        }
    }

    /** TODO: Check against the maximum texture sizes supported by the video card **/


    /** DXTn mipmaps use the same number of 'levels' down to eg. 8x1, but since
     *  it is based around 4x4 pixel blocks it requires padding, so allocate enough
     *  space!
      *********************************/
    if (Format == WINED3DFMT_DXT1) {
        /* DXT1 is half byte per pixel */
       Size = ((max(Width,4) * D3DFmtGetBpp(This, Format)) * max(Height,4)) >> 1;

    } else if (Format == WINED3DFMT_DXT2 || Format == WINED3DFMT_DXT3 ||
               Format == WINED3DFMT_DXT4 || Format == WINED3DFMT_DXT5) {
       Size = ((max(Width,4) * D3DFmtGetBpp(This, Format)) * max(Height,4));
    } else {
       Size = (Width     * D3DFmtGetBpp(This, Format)) * Height;
    }

    /** Create the and initilise surface resource **/
    D3DCREATERESOURCEOBJECTINSTANCE(object,Surface,D3DRTYPE_SURFACE, Size)
    object->container = (IUnknown*) This;

    object->currentDesc.Width      = Width;
    object->currentDesc.Height     = Height;
    object->currentDesc.MultiSampleType    = MultiSample;
    object->currentDesc.MultiSampleQuality = MultisampleQuality;

/* Setup some glformat defaults */
    object->glDescription.glFormat         = D3DFmt2GLFmt(This, object->resource.format);
    object->glDescription.glFormatInternal = D3DFmt2GLIntFmt(This, object->resource.format);
    object->glDescription.glType           = D3DFmt2GLType(This, object->resource.format);
    object->glDescription.textureName      = 0;
    object->glDescription.level            = Level;
    object->glDescription.target           = GL_TEXTURE_2D;

    /* Internal data */
    object->pow2Width  = pow2Width;
    object->pow2Height = pow2Height;
    object->nonpow2    = (pow2Width != Width || pow2Height != Height) ? TRUE : FALSE;
    object->discard    = Discard;
    object->activeLock = FALSE;
    object->bytesPerPixel = D3DFmtGetBpp(This, Format);
    object->pow2Size      = (pow2Width * object->bytesPerPixel) * pow2Height;

    /** TODO: change this into a texture transform matrix so that it's processed in hardware **/

    /* Precalculated scaling for 'faked' non power of two texture coords */
    object->pow2scalingFactorX  =  (((float)Width)  / ((float)pow2Width));
    object->pow2scalingFactorY  =  (((float)Height) / ((float)pow2Height));
    TRACE(" xf(%f) yf(%f) \n", object->pow2scalingFactorX, object->pow2scalingFactorY);

    TRACE("Pool %d %d %d %d",Pool, D3DPOOL_DEFAULT, D3DPOOL_MANAGED, D3DPOOL_SYSTEMMEM);

    /** Quick lockable sanity check TODO: remove this after surfaces, usage and locablility have been debugged properly
    * this function is too deap to need to care about things like this.
    * Levels need to be checked too, and possibly Type wince they all affect what can be done.
    * ****************************************/
    switch(Pool) {
    case D3DPOOL_SCRATCH:
        if(Lockable == FALSE)
            FIXME("Create suface called with a pool of SCRATCH and a Lockable of FALSE \
                which are mutually exclusive, setting lockable to true\n");
                Lockable = TRUE;
    break;
    case D3DPOOL_SYSTEMMEM:
        if(Lockable == FALSE) FIXME("Create surface called with a pool of SYSTEMMEM and a Lockable of FALSE, \
                                    this is acceptable but unexpected (I can't know how the surface can be usable!)\n");
    case D3DPOOL_MANAGED:
        if(Usage == D3DUSAGE_DYNAMIC) FIXME("Create surface called with a pool of MANAGED and a \
                                             Usage of DYNAMIC which are mutually exclusive, not doing \
                                             anything just telling you.\n");
    break;
    case D3DPOOL_DEFAULT: /*TODO: Create offscreen plain can cause this check to fail..., find out if it should */
        if(!(Usage & D3DUSAGE_DYNAMIC) && !(Usage & D3DUSAGE_RENDERTARGET)
           && !(Usage && D3DUSAGE_DEPTHSTENCIL ) && Lockable == TRUE)
            FIXME("Creating a surface with a POOL of DEFAULT with Locable true, that doesn't specify DYNAMIC usage.\n");
    break;
    default:
        FIXME("(%p) Unknown pool %d\n", This, Pool);
    break;
    };

    if (Usage & D3DUSAGE_RENDERTARGET && Pool != D3DPOOL_DEFAULT) {
        FIXME("Trying to create a render target that isn't in the default pool\n");
    }


    object->locked   = FALSE;
    object->lockable = (WINED3DFMT_D16_LOCKABLE == Format) ? TRUE : Lockable;

    /* mark the texture as dirty so that it get's loaded first time around*/
    IWineD3DSurface_AddDirtyRect(*ppSurface, NULL);
    TRACE("(%p) : w(%d) h(%d) fmt(%d,%s) lockable(%d) surf@%p, surfmem@%p, %d bytes\n",
           This, Width, Height, Format, debug_d3dformat(Format),
           (WINED3DFMT_D16_LOCKABLE == Format), *ppSurface, object->resource.allocatedMemory, object->resource.size);
    return D3D_OK;

}

HRESULT  WINAPI IWineD3DDeviceImpl_CreateTexture(IWineD3DDevice *iface, UINT Width, UINT Height, UINT Levels,
                                                 DWORD Usage, WINED3DFORMAT Format, D3DPOOL Pool,
                                                 IWineD3DTexture** ppTexture, HANDLE* pSharedHandle, IUnknown *parent,
                                                 D3DCB_CREATESURFACEFN D3DCB_CreateSurface) {

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DTextureImpl *object;
    unsigned int i;
    UINT tmpW;
    UINT tmpH;
    HRESULT hr;

    TRACE("(%p), Width(%d) Height(%d) Levels(%d) Usage(%ld) .... \n", This, Width, Height, Levels, Usage);

    D3DCREATERESOURCEOBJECTINSTANCE(object, Texture, D3DRTYPE_TEXTURE, 0);
    D3DINITILIZEBASETEXTURE(object->baseTexture);    
    object->width  = Width;
    object->height = Height;
    
    /* Calculate levels for mip mapping */
    if (Levels == 0) {
        TRACE("calculating levels %d\n", object->baseTexture.levels);
        object->baseTexture.levels++;
        tmpW = Width;
        tmpH = Height;
        while (tmpW > 1 && tmpH > 1) {
            tmpW = max(1, tmpW >> 1);
            tmpH = max(1, tmpH >> 1);
            object->baseTexture.levels++;
        }
        TRACE("Calculated levels = %d\n", object->baseTexture.levels);
    }

    /* Generate all the surfaces */
    tmpW = Width;
    tmpH = Height;
    for (i = 0; i < object->baseTexture.levels; i++)
    {
        /* use the callback to create the texture surface */
        hr = D3DCB_CreateSurface(This->parent, tmpW, tmpH, Format, Usage, Pool, i, &object->surfaces[i],NULL);
        if(hr!= D3D_OK) {
            int j;
            FIXME("Failed to create surface  %p \n",object);
            /* clean up */
            for(j = 0 ; j < i ; j++) {
                IWineD3DSurface_Release(object->surfaces[j]);
            }
            /* heap free object */
            HeapFree(GetProcessHeap(),0,object);

            *ppTexture = NULL;
            return hr;
        }

        IWineD3DSurface_SetContainer(object->surfaces[i], (IUnknown *)object);
        TRACE("Created surface level %d @ %p\n", i, object->surfaces[i]);
        /* calculate the next mipmap level */
        tmpW = max(1, tmpW >> 1);
        tmpH = max(1, tmpH >> 1);
    }

    TRACE("(%p) : Created  texture %p\n", This, object);
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_CreateVolumeTexture(IWineD3DDevice *iface,
                                                      UINT Width, UINT Height, UINT Depth,
                                                      UINT Levels, DWORD Usage,
                                                      WINED3DFORMAT Format, D3DPOOL Pool,
                                                      IWineD3DVolumeTexture** ppVolumeTexture,
                                                      HANDLE* pSharedHandle, IUnknown *parent,
                                                      D3DCB_CREATEVOLUMEFN D3DCB_CreateVolume) {

    IWineD3DDeviceImpl        *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DVolumeTextureImpl *object;
    unsigned int               i;
    UINT                       tmpW;
    UINT                       tmpH;
    UINT                       tmpD;

    D3DCREATERESOURCEOBJECTINSTANCE(object, VolumeTexture, D3DRTYPE_VOLUMETEXTURE, 0);
    D3DINITILIZEBASETEXTURE(object->baseTexture);

    TRACE("(%p) : W(%d) H(%d) D(%d), Lvl(%d) Usage(%ld), Fmt(%u,%s), Pool(%s)\n", This, Width, Height,
          Depth, Levels, Usage, Format, debug_d3dformat(Format), debug_d3dpool(Pool));

    object->width  = Width;
    object->height = Height;
    object->depth  = Depth;

    /* Calculate levels for mip mapping */
    if (Levels == 0) {
        object->baseTexture.levels++;
        tmpW = Width;
        tmpH = Height;
        tmpD = Depth;
        while (tmpW > 1 && tmpH > 1 && tmpD > 1) {
            tmpW = max(1, tmpW >> 1);
            tmpH = max(1, tmpH >> 1);
            tmpD = max(1, tmpD >> 1);
            object->baseTexture.levels++;
        }
        TRACE("Calculated levels = %d\n", object->baseTexture.levels);
    }

    /* Generate all the surfaces */
    tmpW = Width;
    tmpH = Height;
    tmpD = Depth;

    for (i = 0; i < object->baseTexture.levels; i++)
    {
        /* Create the volume */
        D3DCB_CreateVolume(This->parent, Width, Height, Depth, Format, Pool, Usage,
                           (IWineD3DVolume **)&object->volumes[i], pSharedHandle);
        IWineD3DVolume_SetContainer(object->volumes[i], (IUnknown *)object);

        tmpW = max(1, tmpW >> 1);
        tmpH = max(1, tmpH >> 1);
        tmpD = max(1, tmpD >> 1);
    }

    *ppVolumeTexture = (IWineD3DVolumeTexture *) object;
    TRACE("(%p) : Created volume texture %p\n", This, object);
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_CreateVolume(IWineD3DDevice *iface,
                                               UINT Width, UINT Height, UINT Depth,
                                               DWORD Usage,
                                               WINED3DFORMAT Format, D3DPOOL Pool,
                                               IWineD3DVolume** ppVolume,
                                               HANDLE* pSharedHandle, IUnknown *parent) {

    IWineD3DDeviceImpl        *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DVolumeImpl        *object; /** NOTE: impl ref allowed since this is a create function **/

    D3DCREATERESOURCEOBJECTINSTANCE(object, Volume, D3DRTYPE_VOLUME, ((Width * D3DFmtGetBpp(This, Format)) * Height * Depth))

    TRACE("(%p) : W(%d) H(%d) D(%d), Usage(%ld), Fmt(%u,%s), Pool(%s)\n", This, Width, Height,
          Depth, Usage, Format, debug_d3dformat(Format), debug_d3dpool(Pool));

    object->currentDesc.Width   = Width;
    object->currentDesc.Height  = Height;
    object->currentDesc.Depth   = Depth;
    object->bytesPerPixel       = D3DFmtGetBpp(This, Format);

    /** Note: Volume textures cannot be dxtn, hence no need to check here **/
    object->lockable            = TRUE;
    object->locked              = FALSE;
    memset(&object->lockedBox, 0, sizeof(D3DBOX));
    object->dirty = FALSE;
    return IWineD3DVolume_CleanDirtyBox((IWineD3DVolume *) object);
}

HRESULT WINAPI IWineD3DDeviceImpl_CreateCubeTexture(IWineD3DDevice *iface, UINT EdgeLength,
                                                    UINT Levels, DWORD Usage,
                                                    WINED3DFORMAT Format, D3DPOOL Pool,
                                                    IWineD3DCubeTexture** ppCubeTexture,
                                                    HANDLE* pSharedHandle, IUnknown *parent,
                                                    D3DCB_CREATESURFACEFN D3DCB_CreateSurface) {

   IWineD3DDeviceImpl      *This = (IWineD3DDeviceImpl *)iface;
   IWineD3DCubeTextureImpl *object; /** NOTE: impl ref allowed since this is a create function **/
   unsigned int             i,j;
   UINT                     tmpW;
   HRESULT                  hr;

   D3DCREATERESOURCEOBJECTINSTANCE(object, CubeTexture, D3DRTYPE_CUBETEXTURE, 0);
   D3DINITILIZEBASETEXTURE(object->baseTexture);

   TRACE("(%p) Create Cube Texture \n", This);

   object->edgeLength           = EdgeLength;

   /* Calculate levels for mip mapping */
   if (Levels == 0) {
       object->baseTexture.levels++;
       tmpW = EdgeLength;
       while (tmpW > 1) {
           tmpW = max(1, tmpW / 2);
           object->baseTexture.levels++;
       }
       TRACE("Calculated levels = %d\n", object->baseTexture.levels);
   }

    /* Generate all the surfaces */
    tmpW = EdgeLength;
    for (i = 0; i < object->baseTexture.levels; i++) {

        /* Create the 6 faces */
        for (j = 0; j < 6; j++) {

            hr=D3DCB_CreateSurface(This->parent, tmpW, tmpW, Format, Usage, Pool,
                                   i /* Level */, &object->surfaces[j][i],pSharedHandle);

            if(hr!= D3D_OK) {
                /* clean up */
                int k;
                int l;
                for (l=0;l<j;l++) {
                    IWineD3DSurface_Release(object->surfaces[j][i]);
                }
                for (k=0;k<i;k++) {
                    for (l=0;l<6;l++) {
                    IWineD3DSurface_Release(object->surfaces[l][j]);
                    }
                }

                FIXME("(%p) Failed to create surface\n",object);
                HeapFree(GetProcessHeap(),0,object);
                *ppCubeTexture = NULL;
                return hr;
            }
            IWineD3DSurface_SetContainer(object->surfaces[j][i], (IUnknown *)object);
            TRACE("Created surface level %d @ %p, \n", i, object->surfaces[j][i]);
        }
        tmpW = max(1, tmpW >> 1);
    }

    TRACE("(%p) : Created Cube Texture %p\n", This, object);
    *ppCubeTexture = (IWineD3DCubeTexture *) object;
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_CreateQuery(IWineD3DDevice *iface, WINED3DQUERYTYPE Type, IWineD3DQuery **ppQuery, IUnknown* parent) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DQueryImpl *object; /*NOTE: impl ref allowed since this is a create function */

    if(NULL == ppQuery) {
        /* Just a check to see if we support this type of query */
        HRESULT hr = D3DERR_NOTAVAILABLE;
        /* Lie and say everything is good (we can return ok fake data from a stub) */
        switch(Type) {
        case WINED3DQUERYTYPE_VCACHE:
        case WINED3DQUERYTYPE_RESOURCEMANAGER:
        case WINED3DQUERYTYPE_VERTEXSTATS:
        case WINED3DQUERYTYPE_EVENT:
        case WINED3DQUERYTYPE_OCCLUSION:
        case WINED3DQUERYTYPE_TIMESTAMP:
        case WINED3DQUERYTYPE_TIMESTAMPDISJOINT:
        case WINED3DQUERYTYPE_TIMESTAMPFREQ:
        case WINED3DQUERYTYPE_PIPELINETIMINGS:
        case WINED3DQUERYTYPE_INTERFACETIMINGS:
        case WINED3DQUERYTYPE_VERTEXTIMINGS:
        case WINED3DQUERYTYPE_PIXELTIMINGS:
        case WINED3DQUERYTYPE_BANDWIDTHTIMINGS:
        case WINED3DQUERYTYPE_CACHEUTILIZATION:
            hr = D3D_OK;
        break;
        default:
            FIXME("(%p) Unhandled query type %d\n",This , Type);
        }
        FIXME("(%p) : Stub request for query type %d returned %ld\n", This, Type, hr);
        return hr;
    }

    D3DCREATEOBJECTINSTANCE(object, Query)
    object->type         = Type;
    object->extendedData = 0;
    TRACE("(%p) : Created Query %p\n", This, object);
    return D3D_OK;
}

/* example at http://www.fairyengine.com/articles/dxmultiviews.htm */
HRESULT WINAPI IWineD3DDeviceImpl_CreateAdditionalSwapChain(IWineD3DDevice* iface, WINED3DPRESENT_PARAMETERS*  pPresentationParameters,                                                                   IWineD3DSwapChain** ppSwapChain,
                                                            IUnknown* parent,
                                                            D3DCB_CREATERENDERTARGETFN D3DCB_CreateRenderTarget,
                                                            D3DCB_CREATEDEPTHSTENCILSURFACEFN D3DCB_CreateDepthStencil) {
    IWineD3DDeviceImpl      *This = (IWineD3DDeviceImpl *)iface;

    HDC                     hDc;
    IWineD3DSwapChainImpl  *object; /** NOTE: impl ref allowed since this is a create function **/
    int                     num;
    XVisualInfo             template;
    GLXContext              oldContext;
    Drawable                oldDrawable;
    HRESULT                 hr = D3D_OK;

    TRACE("(%p) : Created Aditional Swap Chain\n", This);

   /** FIXME: Test under windows to find out what the life cycle of a swap chain is,
   * does a device hold a reference to a swap chain giving them a lifetime of the device
   * or does the swap chain notify the device of it'd destruction.
    *******************************/

    D3DCREATEOBJECTINSTANCE(object, SwapChain)

    /* Initialize other useful values */
    object->presentParms.BackBufferCount = 1; /* TODO:? support for gl_aux buffers */

    /*********************
    * Lookup the window Handle and the relating X window handle
    ********************/

    /* Setup hwnd we are using, plus which display this equates to */
    object->win_handle = *(pPresentationParameters->hDeviceWindow);
    if (!object->win_handle) {
        object->win_handle = This->createParms.hFocusWindow;
    }

    object->win        = (Window)GetPropA(object->win_handle, "__wine_x11_whole_window" );
    hDc                = GetDC(object->win_handle);
    object->display    = get_display(hDc);
    ReleaseDC(object->win_handle, hDc);
    TRACE("Using a display of %p %p  \n", object->display, hDc);

    if (NULL == object->display || NULL == hDc) {
        WARN("Failed to get a display and HDc for Window %p\n", object->win_handle);
        return D3DERR_NOTAVAILABLE;
    }

    if (object->win == 0) {
        WARN("Failed to get a valid XVisuial ID for the window %p\n", object->win_handle);
        return D3DERR_NOTAVAILABLE;
    }
    /**
    * Create an opengl context for the display visual
    *  NOTE: the visual is chosen as the window is created and the glcontext cannot
    *     use different properties after that point in time. FIXME: How to handle when requested format
    *     doesn't match actual visual? Cannot choose one here - code removed as it ONLY works if the one
    *     it chooses is identical to the one already being used!
     **********************************/

    /** FIXME: Handle stencil appropriately via EnableAutoDepthStencil / AutoDepthStencilFormat **/
    ENTER_GL();

    /* Create a new context for this swapchain */
    template.visualid = (VisualID)GetPropA(GetDesktopWindow(), "__wine_x11_visual_id");
    /* TODO: change this to find a similar visual, but one with a stencil/zbuffer buffer that matches the request
    (or the best possible if none is requested) */
    TRACE("Found x visual ID  : %ld\n", template.visualid);

    object->visInfo   = XGetVisualInfo(object->display, VisualIDMask, &template, &num);
    if (NULL == object->visInfo) {
        ERR("cannot really get XVisual\n");
        LEAVE_GL();
        return D3DERR_NOTAVAILABLE;
    } else {
        int n, value;
        /* Write out some debug info about the visual/s */
        TRACE("Using x visual ID  : %ld\n", template.visualid);
        TRACE("        visual info: %p\n", object->visInfo);
        TRACE("        num items  : %d\n", num);
        for(n = 0;n < num; n++) {
            TRACE("=====item=====: %d\n", n + 1);
            TRACE("   visualid      : %ld\n", object->visInfo[n].visualid);
            TRACE("   screen        : %d\n",  object->visInfo[n].screen);
            TRACE("   depth         : %u\n",  object->visInfo[n].depth);
            TRACE("   class         : %d\n",  object->visInfo[n].class);
            TRACE("   red_mask      : %ld\n", object->visInfo[n].red_mask);
            TRACE("   green_mask    : %ld\n", object->visInfo[n].green_mask);
            TRACE("   blue_mask     : %ld\n", object->visInfo[n].blue_mask);
            TRACE("   colormap_size : %d\n",  object->visInfo[n].colormap_size);
            TRACE("   bits_per_rgb  : %d\n",  object->visInfo[n].bits_per_rgb);
            /* log some extra glx info */
            glXGetConfig(object->display, object->visInfo, GLX_AUX_BUFFERS, &value);
            TRACE("   gl_aux_buffers  : %d\n",  value);
            glXGetConfig(object->display, object->visInfo, GLX_BUFFER_SIZE ,&value);
            TRACE("   gl_buffer_size  : %d\n",  value);
            glXGetConfig(object->display, object->visInfo, GLX_RED_SIZE, &value);
            TRACE("   gl_red_size  : %d\n",  value);
            glXGetConfig(object->display, object->visInfo, GLX_GREEN_SIZE, &value);
            TRACE("   gl_green_size  : %d\n",  value);
            glXGetConfig(object->display, object->visInfo, GLX_BLUE_SIZE, &value);
            TRACE("   gl_blue_size  : %d\n",  value);
            glXGetConfig(object->display, object->visInfo, GLX_ALPHA_SIZE, &value);
            TRACE("   gl_alpha_size  : %d\n",  value);
            glXGetConfig(object->display, object->visInfo, GLX_DEPTH_SIZE ,&value);
            TRACE("   gl_depth_size  : %d\n",  value);
            glXGetConfig(object->display, object->visInfo, GLX_STENCIL_SIZE, &value);
            TRACE("   gl_stencil_size : %d\n",  value);
        }
        /* Now choose a simila visual ID*/
    }
#ifdef USE_CONTEXT_MANAGER

    /** TODO: use a context mamager **/
#endif

    {
        IWineD3DSwapChain *implSwapChain;
        if (D3D_OK != IWineD3DDevice_GetSwapChain(iface, 0, &implSwapChain)) {
            /* The first time around we create the context that is shared with all other swapchains and render targets */
            object->glCtx = glXCreateContext(object->display, object->visInfo, NULL, GL_TRUE);
            TRACE("Creating implicit context for vis %p, hwnd %p\n", object->display, object->visInfo);
        } else {

            TRACE("Creating context for vis %p, hwnd %p\n", object->display, object->visInfo);
            /* TODO: don't use Impl structures outside of create functions! (a context manager will replace the ->glCtx) */
            /* and create a new context with the implicit swapchains context as the shared context */
            object->glCtx = glXCreateContext(object->display, object->visInfo, ((IWineD3DSwapChainImpl *)implSwapChain)->glCtx, GL_TRUE);
            IWineD3DSwapChain_Release(implSwapChain);
        }
    }

    /* Cleanup */
    XFree(object->visInfo);
    object->visInfo = NULL;

    if (NULL == object->glCtx) {
        ERR("cannot create glxContext\n");
        LEAVE_GL();
        return D3DERR_NOTAVAILABLE;
    }

    LEAVE_GL();
    if (object->glCtx == NULL) {
        ERR("Error in context creation !\n");
        return D3DERR_INVALIDCALL;
    } else {
        TRACE("Context created (HWND=%p, glContext=%p, Window=%ld, VisInfo=%p)\n",
                object->win_handle, object->glCtx, object->win, object->visInfo);
    }

   /*********************
   * Windowed / Fullscreen
   *******************/

   /**
   * TODO: MSDN says that we are only allowed one fullscreen swapchain per device,
   * so we should really check to see if their is a fullscreen swapchain already
   * I think Windows and X have different ideas about fullscreen, does a single head count as full screen?
    **************************************/

   if (!*(pPresentationParameters->Windowed)) {

        DEVMODEW devmode;
        HDC      hdc;
        int      bpp = 0;

        /* Get info on the current display setup */
        hdc = CreateDCA("DISPLAY", NULL, NULL, NULL);
        bpp = GetDeviceCaps(hdc, BITSPIXEL);
        DeleteDC(hdc);

        /* Change the display settings */
        memset(&devmode, 0, sizeof(DEVMODEW));
        devmode.dmFields     = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
        devmode.dmBitsPerPel = (bpp >= 24) ? 32 : bpp; /* Stupid XVidMode cannot change bpp */
        devmode.dmPelsWidth  = *(pPresentationParameters->BackBufferWidth);
        devmode.dmPelsHeight = *(pPresentationParameters->BackBufferHeight);
        MultiByteToWideChar(CP_ACP, 0, "Gamers CG", -1, devmode.dmDeviceName, CCHDEVICENAME);
        ChangeDisplaySettingsExW(devmode.dmDeviceName, &devmode, object->win_handle, CDS_FULLSCREEN, NULL);

        /* Make popup window */
        SetWindowLongA(object->win_handle, GWL_STYLE, WS_POPUP);
        SetWindowPos(object->win_handle, HWND_TOP, 0, 0,
                     *(pPresentationParameters->BackBufferWidth),
                     *(pPresentationParameters->BackBufferHeight), SWP_SHOWWINDOW | SWP_FRAMECHANGED);


    }


    /** MSDN: If Windowed is TRUE and either of the BackBufferWidth/Height values is zero,
     *  then the corresponding dimension of the client area of the hDeviceWindow
     *  (or the focus window, if hDeviceWindow is NULL) is taken.
      **********************/

    if (*(pPresentationParameters->Windowed) &&
        ((*(pPresentationParameters->BackBufferWidth)  == 0) ||
         (*(pPresentationParameters->BackBufferHeight) == 0))) {

        RECT Rect;
        GetClientRect(object->win_handle, &Rect);

        if (*(pPresentationParameters->BackBufferWidth) == 0) {
           *(pPresentationParameters->BackBufferWidth) = Rect.right;
           TRACE("Updating width to %d\n", *(pPresentationParameters->BackBufferWidth));
        }
        if (*(pPresentationParameters->BackBufferHeight) == 0) {
           *(pPresentationParameters->BackBufferHeight) = Rect.bottom;
           TRACE("Updating height to %d\n", *(pPresentationParameters->BackBufferHeight));
        }
    }

   /*********************
   * finish off parameter initialization
   *******************/

    /* Put the correct figures in the presentation parameters */
    TRACE("Coppying accross presentaion paraneters\n");
    object->presentParms.BackBufferWidth                = *(pPresentationParameters->BackBufferWidth);
    object->presentParms.BackBufferHeight               = *(pPresentationParameters->BackBufferHeight);
    object->presentParms.BackBufferFormat               = *(pPresentationParameters->BackBufferFormat);
    object->presentParms.BackBufferCount                = *(pPresentationParameters->BackBufferCount);
    object->presentParms.MultiSampleType                = *(pPresentationParameters->MultiSampleType);
    object->presentParms.MultiSampleQuality             = *(pPresentationParameters->MultiSampleQuality);
    object->presentParms.SwapEffect                     = *(pPresentationParameters->SwapEffect);
    object->presentParms.hDeviceWindow                  = *(pPresentationParameters->hDeviceWindow);
    object->presentParms.Windowed                       = *(pPresentationParameters->Windowed);
    object->presentParms.EnableAutoDepthStencil         = *(pPresentationParameters->EnableAutoDepthStencil);
    object->presentParms.AutoDepthStencilFormat         = *(pPresentationParameters->AutoDepthStencilFormat);
    object->presentParms.Flags                          = *(pPresentationParameters->Flags);
    object->presentParms.FullScreen_RefreshRateInHz     = *(pPresentationParameters->FullScreen_RefreshRateInHz);
    object->presentParms.PresentationInterval           = *(pPresentationParameters->PresentationInterval);


   /* FIXME: check for any failures */
   /*********************
   * Create the back, front and stencil buffers
   *******************/
    TRACE("calling rendertarget CB\n");
    hr = D3DCB_CreateRenderTarget((IUnknown *) This->parent,
                             object->presentParms.BackBufferWidth,
                             object->presentParms.BackBufferHeight,
                             object->presentParms.BackBufferFormat,
                             object->presentParms.MultiSampleType,
                             object->presentParms.MultiSampleQuality,
                             TRUE /* Lockable */,
                             &object->frontBuffer,
                             NULL /* pShared (always null)*/);
    if (object->frontBuffer != NULL)
        IWineD3DSurface_SetContainer(object->frontBuffer, (IUnknown *)object);
    TRACE("calling rendertarget CB\n");
    hr = D3DCB_CreateRenderTarget((IUnknown *) This->parent,
                             object->presentParms.BackBufferWidth,
                             object->presentParms.BackBufferHeight,
                             object->presentParms.BackBufferFormat,
                             object->presentParms.MultiSampleType,
                             object->presentParms.MultiSampleQuality,
                             TRUE /* Lockable */,
                             &object->backBuffer,
                             NULL /* pShared (always null)*/);
    if (object->backBuffer != NULL)
       IWineD3DSurface_SetContainer(object->backBuffer, (IUnknown *)object);

    /* Under directX swapchains share the depth stencil, so only create one depth-stencil */
    if (pPresentationParameters->EnableAutoDepthStencil) {
        TRACE("Creating depth stencil buffer\n");
        if (This->depthStencilBuffer == NULL ) {
            hr = D3DCB_CreateDepthStencil((IUnknown *) This->parent,
                                    object->presentParms.BackBufferWidth,
                                    object->presentParms.BackBufferHeight,
                                    object->presentParms.AutoDepthStencilFormat,
                                    object->presentParms.MultiSampleType,
                                    object->presentParms.MultiSampleQuality,
                                    FALSE /* FIXME: Discard */,
                                    &This->depthStencilBuffer,
                                    NULL /* pShared (always null)*/  );
            if (This->depthStencilBuffer != NULL)
                IWineD3DSurface_SetContainer(This->depthStencilBuffer, (IUnknown *)iface);
        }

        /** TODO: A check on width, height and multisample types
        *(since the zbuffer must be at least as large as the render target and have the same multisample parameters)
         ****************************/
        object->wantsDepthStencilBuffer = TRUE;
    } else {
        object->wantsDepthStencilBuffer = FALSE;
    }

    TRACE("FrontBuf @ %p, BackBuf @ %p, DepthStencil %d\n",object->frontBuffer, object->backBuffer, object->wantsDepthStencilBuffer);


   /*********************
   * init the default renderTarget management
   *******************/
    object->drawable     = object->win;
    object->render_ctx   = object->glCtx;

    if(hr == D3D_OK) {
        /*********************
         * Setup some defaults and clear down the buffers
         *******************/
        ENTER_GL();
        /** save current context and drawable **/
        oldContext   =   glXGetCurrentContext();
        oldDrawable  =   glXGetCurrentDrawable();

        TRACE("Activating context (display %p context %p drawable %ld)!\n", object->display, object->glCtx, object->win);
        if (glXMakeCurrent(object->display, object->win, object->glCtx) == False) {
            ERR("Error in setting current context (display %p context %p drawable %ld)!\n", object->display, object->glCtx, object->win);
        }
        checkGLcall("glXMakeCurrent");

        TRACE("Setting up the screen\n");
        /* Clear the screen */
        glClearColor(0.0, 0.0, 0.0, 0.0);
        checkGLcall("glClearColor");
        glClearIndex(0);
        glClearDepth(1);
        glClearStencil(0xffff);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_ACCUM_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        checkGLcall("glClear");

        glColor3f(1.0, 1.0, 1.0);
        checkGLcall("glColor3f");

        glEnable(GL_LIGHTING);
        checkGLcall("glEnable");

        glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
        checkGLcall("glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);");

        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
        checkGLcall("glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);");

        glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
        checkGLcall("glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);");

        /* switch back to the original context (unless it was zero)*/
        if (This->numberOfSwapChains != 0) {
            /** TODO: restore the context and drawable **/
            glXMakeCurrent(object->display, oldDrawable, oldContext);
        }

        LEAVE_GL();

        /* TODO: move this off into a linked list implementation! (add swapchain, remove swapchain or something along those lines) */
#if 0
            IListOperator *listOperator;
            IListStore_CreateListOperator(This->swapchainStore, &listOperator);
            IListOperator_Append(listOperator, (void *)object);
            IListOperator_Release(listOperator);
#endif

        This->swapchains[This->numberOfSwapChains++] = (IWineD3DSwapChain *)object;
        TRACE("Set swapchain to %p\n", object);
    } else { /* something went wrong so clean up */
        IUnknown* bufferParent;
        if (object->frontBuffer) {
            IWineD3DSurface_GetParent(object->frontBuffer, &bufferParent);
            IUnknown_Release(bufferParent); /* once for the get parent */
            if(IUnknown_Release(bufferParent) > 0) {
                FIXME("(%p) Something's still holding the front buffer\n",This);
            }
        }
        if (object->backBuffer) {
            IWineD3DSurface_GetParent(object->backBuffer, &bufferParent);
            IUnknown_Release(bufferParent); /* once for the get parent */
            if(IUnknown_Release(bufferParent) > 0) {
                FIXME("(%p) Something's still holding the back buffer\n",This);
            }
        }
        /* NOTE: don't clean up the depthstencil buffer because it belongs to the device */
        /* Clean up the context */
        /* check that we are the current context first (we shouldn't be though!) */
        if (object->glCtx != 0) {
            if(glXGetCurrentContext() == object->glCtx) {
                glXMakeCurrent(object->display, None, NULL);
            }
            glXDestroyContext(object->display, object->glCtx);
        }
        HeapFree(GetProcessHeap(), 0, object);

    }
    return hr;
}

/** NOTE: These are ahead of the other getters and setters to save using a forward declaration **/
UINT     WINAPI  IWineD3DDeviceImpl_GetNumberOfSwapChains(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    /* TODO: move over to using a linked list. */
    TRACE("(%p) returning %d\n", This, This->numberOfSwapChains);
    return This->numberOfSwapChains;
}

HRESULT  WINAPI  IWineD3DDeviceImpl_GetSwapChain(IWineD3DDevice *iface, UINT iSwapChain, IWineD3DSwapChain **pSwapChain) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : swapchain %d \n", This, iSwapChain);

    if(iSwapChain >= IWineD3DDeviceImpl_GetNumberOfSwapChains(iface) || iSwapChain < 0) {
        *pSwapChain = NULL;
        return D3DERR_INVALIDCALL;
    }else{
        /** TODO: move off to a linked list implementation **/
        *pSwapChain = This->swapchains[iSwapChain];
    }

    /* TODO: move over to using stores and linked lists. */

    IWineD3DSwapChain_AddRef(*pSwapChain);
    TRACE("(%p) returning %p\n", This, *pSwapChain);
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_Reset(IWineD3DDevice* iface, WINED3DPRESENT_PARAMETERS*  pPresentationParameters) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    FIXME("(%p) : Stub\n",This);
    return D3D_OK;

}

/*****
 * Vertex Declaration
 *****/
HRESULT WINAPI IWineD3DDeviceImpl_CreateVertexDeclaration(IWineD3DDevice* iface, CONST VOID* pDeclaration, IWineD3DVertexDeclaration** ppVertexDeclaration, IUnknown *parent) {
    IWineD3DDeviceImpl            *This   = (IWineD3DDeviceImpl *)iface;
    IWineD3DVertexDeclarationImpl *object = NULL;
    HRESULT hr = D3D_OK;
    TRACE("(%p) : directXVersion=%u, pFunction=%p, ppDecl=%p\n", This, ((IWineD3DImpl *)This->wineD3D)->dxVersion, pDeclaration, ppVertexDeclaration);
    D3DCREATEOBJECTINSTANCE(object, VertexDeclaration)
    object->allFVF = 0;

    hr = IWineD3DVertexDeclaration_SetDeclaration((IWineD3DVertexDeclaration *)object, (void *)pDeclaration);

    return hr;
}

/* http://msdn.microsoft.com/archive/default.asp?url=/archive/en-us/directx9_c/directx/graphics/programmingguide/programmable/vertexshaders/vscreate.asp */
HRESULT WINAPI IWineD3DDeviceImpl_CreateVertexShader(IWineD3DDevice* iface,  CONST DWORD* pFunction, IWineD3DVertexShader** ppVertexShader, IUnknown *parent) {
    IWineD3DDeviceImpl       *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DVertexShaderImpl *object;  /* NOTE: impl usage is ok, this is a create */
    D3DCREATEOBJECTINSTANCE(object, VertexShader)
    object->function      = pFunction;
    FIXME("(%p) : STUB: Created Vertex shader %p\n", This, ppVertexShader);
    return D3D_OK;

}

HRESULT WINAPI IWineD3DDeviceImpl_CreatePixelShader(IWineD3DDevice* iface, CONST DWORD* pFunction, IWineD3DPixelShader** ppPixelShader, IUnknown *parent) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    FIXME("(%p) : Stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_GetDirect3D(IWineD3DDevice* iface, IWineD3D** ppD3D) {
   IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
   *ppD3D= This->wineD3D;
   TRACE("(%p) : wineD3D returning %p\n", This,  *ppD3D);
   IWineD3D_AddRef(*ppD3D);
   return D3D_OK;

}

UINT WINAPI IWineD3DDeviceImpl_GetAvailableTextureMem(IWineD3DDevice *iface) {
    /** NOTE: There's a probably  a hack-around for this one by putting as many pbuffers, VBO's (or whatever)
    * Into the video ram as possible and seeing how many fit
    * you can also get the correct initial value from via X and ATI's driver
    *******************/
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    static BOOL showfixmes = TRUE;
    if (showfixmes) {
        FIXME("(%p) : stub, emulating %dMib for now, returning %dMib\n", This, (emulated_textureram/(1024*1024)),
         ((emulated_textureram - wineD3DGlobalStatistics->glsurfaceram) / (1024*1024)));
         showfixmes = FALSE;
    }
    TRACE("(%p) :  emulating %dMib for now, returning %dMib\n",  This, (emulated_textureram/(1024*1024)),
         ((emulated_textureram - wineD3DGlobalStatistics->glsurfaceram) / (1024*1024)));
    /* videomemory is simulated videomemory + AGP memory left */
    return (emulated_textureram - wineD3DGlobalStatistics->glsurfaceram);
}



/*****
 * Get / Set FVF
 *****/
HRESULT WINAPI IWineD3DDeviceImpl_SetFVF(IWineD3DDevice *iface, DWORD fvf) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    /* Update the current state block */
    This->updateStateBlock->fvf              = fvf;
    This->updateStateBlock->changed.fvf      = TRUE;
    This->updateStateBlock->set.fvf          = TRUE;

    TRACE("(%p) : FVF Shader FVF set to %lx\n", This, fvf);
    /* clear down the vertex declaration
     NOTE: Axis and Allies doesn't work properly otherwise
     (may be a stateblock problem though!)
    */
    /* No difference if recording or not */
    return IWineD3DDevice_SetVertexDeclaration(iface, NULL);

}


HRESULT WINAPI IWineD3DDeviceImpl_GetFVF(IWineD3DDevice *iface, DWORD *pfvf) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : GetFVF returning %lx\n", This, This->stateBlock->fvf);
    *pfvf = This->stateBlock->fvf;
    return D3D_OK;
}

/*****
 * Get / Set Stream Source
 *****/
HRESULT WINAPI IWineD3DDeviceImpl_SetStreamSource(IWineD3DDevice *iface, UINT StreamNumber,IWineD3DVertexBuffer* pStreamData, UINT OffsetInBytes, UINT Stride) {
    IWineD3DDeviceImpl       *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DVertexBuffer     *oldSrc;

    oldSrc = This->stateBlock->streamSource[StreamNumber];
    TRACE("(%p) : StreamNo: %d, OldStream (%p), NewStream (%p), NewStride %d\n", This, StreamNumber, oldSrc, pStreamData, Stride);

    This->updateStateBlock->changed.streamSource[StreamNumber] = TRUE;
    This->updateStateBlock->set.streamSource[StreamNumber]     = TRUE;
    This->updateStateBlock->streamStride[StreamNumber]         = Stride;
    This->updateStateBlock->streamSource[StreamNumber]         = pStreamData;
    This->updateStateBlock->streamOffset[StreamNumber]         = OffsetInBytes;

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return D3D_OK;
    }

    /* Not recording... */
    if (oldSrc != NULL) IWineD3DVertexBuffer_Release(oldSrc);
    if (pStreamData != NULL) IWineD3DVertexBuffer_AddRef(pStreamData);

    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_GetStreamSource(IWineD3DDevice *iface, UINT StreamNumber,IWineD3DVertexBuffer** pStream, UINT *pOffset, UINT* pStride) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("(%p) : StreamNo: %d, Stream (%p), Stride %d\n", This, StreamNumber, This->stateBlock->streamSource[StreamNumber], This->stateBlock->streamStride[StreamNumber]);
    *pStream = This->stateBlock->streamSource[StreamNumber];
    *pStride = This->stateBlock->streamStride[StreamNumber];
    *pOffset = This->stateBlock->streamOffset[StreamNumber];
    if (*pStream != NULL) IWineD3DVertexBuffer_AddRef(*pStream); /* We have created a new reference to the VB */
    return D3D_OK;
}

/*Should be quite easy, just an extension of vertexdata
ref...
http://msdn.microsoft.com/archive/default.asp?url=/archive/en-us/directx9_c_Summer_04/directx/graphics/programmingguide/advancedtopics/DrawingMultipleInstances.asp

The divider is a bit odd though

VertexOffset = StartVertex / Divider * StreamStride +
               VertexIndex / Divider * StreamStride + StreamOffset

*/
HRESULT WINAPI IWineD3DDeviceImpl_SetStreamSourceFreq(IWineD3DDevice *iface,  UINT StreamNumber, UINT Divider) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_GetStreamSourceFreq(IWineD3DDevice *iface,  UINT StreamNumber, UINT* Divider) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

/*****
 * Get / Set & Multiply Transform
 *****/
HRESULT  WINAPI  IWineD3DDeviceImpl_SetTransform(IWineD3DDevice *iface, D3DTRANSFORMSTATETYPE d3dts, CONST D3DMATRIX* lpmatrix) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    /* Most of this routine, comments included copied from ddraw tree initially: */
    TRACE("(%p) : Transform State=%d\n", This, d3dts);

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        This->updateStateBlock->changed.transform[d3dts] = TRUE;
        This->updateStateBlock->set.transform[d3dts]     = TRUE;
        memcpy(&This->updateStateBlock->transforms[d3dts], lpmatrix, sizeof(D3DMATRIX));
        return D3D_OK;
    }

    /*
     * If the new matrix is the same as the current one,
     * we cut off any further processing. this seems to be a reasonable
     * optimization because as was noticed, some apps (warcraft3 for example)
     * tend towards setting the same matrix repeatedly for some reason.
     *
     * From here on we assume that the new matrix is different, wherever it matters.
     */
    if (!memcmp(&This->stateBlock->transforms[d3dts].u.m[0][0], lpmatrix, sizeof(D3DMATRIX))) {
        TRACE("The app is setting the same matrix over again\n");
        return D3D_OK;
    } else {
        conv_mat(lpmatrix, &This->stateBlock->transforms[d3dts].u.m[0][0]);
    }

    /*
       ScreenCoord = ProjectionMat * ViewMat * WorldMat * ObjectCoord
       where ViewMat = Camera space, WorldMat = world space.

       In OpenGL, camera and world space is combined into GL_MODELVIEW
       matrix.  The Projection matrix stay projection matrix.
     */

    /* Capture the times we can just ignore the change for now */
    if (d3dts == D3DTS_WORLDMATRIX(0)) {
        This->modelview_valid = FALSE;
        return D3D_OK;

    } else if (d3dts == D3DTS_PROJECTION) {
        This->proj_valid = FALSE;
        return D3D_OK;

    } else if (d3dts >= D3DTS_WORLDMATRIX(1) && d3dts <= D3DTS_WORLDMATRIX(255)) {
        /* Indexed Vertex Blending Matrices 256 -> 511  */
        /* Use arb_vertex_blend or NV_VERTEX_WEIGHTING? */
        FIXME("D3DTS_WORLDMATRIX(1..255) not handled\n");
        return D3D_OK;
    }

    /* Now we really are going to have to change a matrix */
    ENTER_GL();

    if (d3dts >= D3DTS_TEXTURE0 && d3dts <= D3DTS_TEXTURE7) { /* handle texture matrices */
        if (d3dts < GL_LIMITS(textures)) {
            int tex = d3dts - D3DTS_TEXTURE0;
            GLACTIVETEXTURE(tex);
            set_texture_matrix((float *)lpmatrix,
                               This->updateStateBlock->textureState[tex][WINED3DTSS_TEXTURETRANSFORMFLAGS]);
        }

    } else if (d3dts == D3DTS_VIEW) { /* handle the VIEW matrice */
        unsigned int k;

        /* If we are changing the View matrix, reset the light and clipping planes to the new view
         * NOTE: We have to reset the positions even if the light/plane is not currently
         *       enabled, since the call to enable it will not reset the position.
         * NOTE2: Apparently texture transforms do NOT need reapplying
         */

        PLIGHTINFOEL *lightChain = NULL;
        This->modelview_valid = FALSE;
        This->view_ident = !memcmp(lpmatrix, identity, 16 * sizeof(float));

        glMatrixMode(GL_MODELVIEW);
        checkGLcall("glMatrixMode(GL_MODELVIEW)");
        glPushMatrix();
        glLoadMatrixf((float *)lpmatrix);
        checkGLcall("glLoadMatrixf(...)");

        /* Reset lights */
        lightChain = This->stateBlock->lights;
        while (lightChain && lightChain->glIndex != -1) {
            glLightfv(GL_LIGHT0 + lightChain->glIndex, GL_POSITION, lightChain->lightPosn);
            checkGLcall("glLightfv posn");
            glLightfv(GL_LIGHT0 + lightChain->glIndex, GL_SPOT_DIRECTION, lightChain->lightDirn);
            checkGLcall("glLightfv dirn");
            lightChain = lightChain->next;
        }

        /* Reset Clipping Planes if clipping is enabled */
        for (k = 0; k < GL_LIMITS(clipplanes); k++) {
            glClipPlane(GL_CLIP_PLANE0 + k, This->stateBlock->clipplane[k]);
            checkGLcall("glClipPlane");
        }
        glPopMatrix();

    } else { /* What was requested!?? */
        WARN("invalid matrix specified: %i\n", d3dts);
    }

    /* Release lock, all done */
    LEAVE_GL();
    return D3D_OK;

}
HRESULT WINAPI IWineD3DDeviceImpl_GetTransform(IWineD3DDevice *iface, D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : for Transform State %d\n", This, State);
    memcpy(pMatrix, &This->stateBlock->transforms[State], sizeof(D3DMATRIX));
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_MultiplyTransform(IWineD3DDevice *iface, D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix) {
    D3DMATRIX *mat = NULL;
    D3DMATRIX temp;

    /* Note: Using 'updateStateBlock' rather than 'stateblock' in the code
     * below means it will be recorded in a state block change, but it
     * works regardless where it is recorded.
     * If this is found to be wrong, change to StateBlock.
     */
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : For state %u\n", This, State);

    if (State < HIGHEST_TRANSFORMSTATE)
    {
        mat = &This->updateStateBlock->transforms[State];
    } else {
        FIXME("Unhandled transform state!!\n");
    }

    /* Copied from ddraw code:  */
    temp.u.s._11 = (mat->u.s._11 * pMatrix->u.s._11) + (mat->u.s._21 * pMatrix->u.s._12) + (mat->u.s._31 * pMatrix->u.s._13) + (mat->u.s._41 * pMatrix->u.s._14);
    temp.u.s._21 = (mat->u.s._11 * pMatrix->u.s._21) + (mat->u.s._21 * pMatrix->u.s._22) + (mat->u.s._31 * pMatrix->u.s._23) + (mat->u.s._41 * pMatrix->u.s._24);
    temp.u.s._31 = (mat->u.s._11 * pMatrix->u.s._31) + (mat->u.s._21 * pMatrix->u.s._32) + (mat->u.s._31 * pMatrix->u.s._33) + (mat->u.s._41 * pMatrix->u.s._34);
    temp.u.s._41 = (mat->u.s._11 * pMatrix->u.s._41) + (mat->u.s._21 * pMatrix->u.s._42) + (mat->u.s._31 * pMatrix->u.s._43) + (mat->u.s._41 * pMatrix->u.s._44);

    temp.u.s._12 = (mat->u.s._12 * pMatrix->u.s._11) + (mat->u.s._22 * pMatrix->u.s._12) + (mat->u.s._32 * pMatrix->u.s._13) + (mat->u.s._42 * pMatrix->u.s._14);
    temp.u.s._22 = (mat->u.s._12 * pMatrix->u.s._21) + (mat->u.s._22 * pMatrix->u.s._22) + (mat->u.s._32 * pMatrix->u.s._23) + (mat->u.s._42 * pMatrix->u.s._24);
    temp.u.s._32 = (mat->u.s._12 * pMatrix->u.s._31) + (mat->u.s._22 * pMatrix->u.s._32) + (mat->u.s._32 * pMatrix->u.s._33) + (mat->u.s._42 * pMatrix->u.s._34);
    temp.u.s._42 = (mat->u.s._12 * pMatrix->u.s._41) + (mat->u.s._22 * pMatrix->u.s._42) + (mat->u.s._32 * pMatrix->u.s._43) + (mat->u.s._42 * pMatrix->u.s._44);

    temp.u.s._13 = (mat->u.s._13 * pMatrix->u.s._11) + (mat->u.s._23 * pMatrix->u.s._12) + (mat->u.s._33 * pMatrix->u.s._13) + (mat->u.s._43 * pMatrix->u.s._14);
    temp.u.s._23 = (mat->u.s._13 * pMatrix->u.s._21) + (mat->u.s._23 * pMatrix->u.s._22) + (mat->u.s._33 * pMatrix->u.s._23) + (mat->u.s._43 * pMatrix->u.s._24);
    temp.u.s._33 = (mat->u.s._13 * pMatrix->u.s._31) + (mat->u.s._23 * pMatrix->u.s._32) + (mat->u.s._33 * pMatrix->u.s._33) + (mat->u.s._43 * pMatrix->u.s._34);
    temp.u.s._43 = (mat->u.s._13 * pMatrix->u.s._41) + (mat->u.s._23 * pMatrix->u.s._42) + (mat->u.s._33 * pMatrix->u.s._43) + (mat->u.s._43 * pMatrix->u.s._44);

    temp.u.s._14 = (mat->u.s._14 * pMatrix->u.s._11) + (mat->u.s._24 * pMatrix->u.s._12) + (mat->u.s._34 * pMatrix->u.s._13) + (mat->u.s._44 * pMatrix->u.s._14);
    temp.u.s._24 = (mat->u.s._14 * pMatrix->u.s._21) + (mat->u.s._24 * pMatrix->u.s._22) + (mat->u.s._34 * pMatrix->u.s._23) + (mat->u.s._44 * pMatrix->u.s._24);
    temp.u.s._34 = (mat->u.s._14 * pMatrix->u.s._31) + (mat->u.s._24 * pMatrix->u.s._32) + (mat->u.s._34 * pMatrix->u.s._33) + (mat->u.s._44 * pMatrix->u.s._34);
    temp.u.s._44 = (mat->u.s._14 * pMatrix->u.s._41) + (mat->u.s._24 * pMatrix->u.s._42) + (mat->u.s._34 * pMatrix->u.s._43) + (mat->u.s._44 * pMatrix->u.s._44);

    /* Apply change via set transform - will reapply to eg. lights this way */
    IWineD3DDeviceImpl_SetTransform(iface, State, &temp);
    return D3D_OK;
}

/*****
 * Get / Set Light
 *   WARNING: This code relies on the fact that D3DLIGHT8 == D3DLIGHT9
 *****/
/* Note lights are real special cases. Although the device caps state only eg. 8 are supported,
   you can reference any indexes you want as long as that number max are enabled at any
   one point in time! Therefore since the indexes can be anything, we need a linked list of them.
   However, this causes stateblock problems. When capturing the state block, I duplicate the list,
   but when recording, just build a chain pretty much of commands to be replayed.                  */

HRESULT WINAPI IWineD3DDeviceImpl_SetLight(IWineD3DDevice *iface, DWORD Index, CONST WINED3DLIGHT* pLight) {
    float rho;
    PLIGHTINFOEL *object, *temp;

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : Idx(%ld), pLight(%p)\n", This, Index, pLight);

    /* If recording state block, just add to end of lights chain */
    if (This->isRecordingState) {
        object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PLIGHTINFOEL));
        if (NULL == object) {
            return D3DERR_OUTOFVIDEOMEMORY;
        }
        memcpy(&object->OriginalParms, pLight, sizeof(D3DLIGHT9));
        object->OriginalIndex = Index;
        object->glIndex = -1;
        object->changed = TRUE;

        /* Add to the END of the chain of lights changes to be replayed */
        if (This->updateStateBlock->lights == NULL) {
            This->updateStateBlock->lights = object;
        } else {
            temp = This->updateStateBlock->lights;
            while (temp->next != NULL) temp=temp->next;
            temp->next = object;
        }
        TRACE("Recording... not performing anything more\n");
        return D3D_OK;
    }

    /* Ok, not recording any longer so do real work */
    object = This->stateBlock->lights;
    while (object != NULL && object->OriginalIndex != Index) object = object->next;

    /* If we didn't find it in the list of lights, time to add it */
    if (object == NULL) {
        PLIGHTINFOEL *insertAt,*prevPos;

        object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PLIGHTINFOEL));
        if (NULL == object) {
            return D3DERR_OUTOFVIDEOMEMORY;
        }
        object->OriginalIndex = Index;
        object->glIndex = -1;

        /* Add it to the front of list with the idea that lights will be changed as needed
           BUT after any lights currently assigned GL indexes                             */
        insertAt = This->stateBlock->lights;
        prevPos  = NULL;
        while (insertAt != NULL && insertAt->glIndex != -1) {
            prevPos  = insertAt;
            insertAt = insertAt->next;
        }

        if (insertAt == NULL && prevPos == NULL) { /* Start of list */
            This->stateBlock->lights = object;
        } else if (insertAt == NULL) { /* End of list */
            prevPos->next = object;
            object->prev = prevPos;
        } else { /* Middle of chain */
            if (prevPos == NULL) {
                This->stateBlock->lights = object;
            } else {
                prevPos->next = object;
            }
            object->prev = prevPos;
            object->next = insertAt;
            insertAt->prev = object;
        }
    }

    /* Initialze the object */
    TRACE("Light %ld setting to type %d, Diffuse(%f,%f,%f,%f), Specular(%f,%f,%f,%f), Ambient(%f,%f,%f,%f)\n", Index, pLight->Type,
          pLight->Diffuse.r, pLight->Diffuse.g, pLight->Diffuse.b, pLight->Diffuse.a,
          pLight->Specular.r, pLight->Specular.g, pLight->Specular.b, pLight->Specular.a,
          pLight->Ambient.r, pLight->Ambient.g, pLight->Ambient.b, pLight->Ambient.a);
    TRACE("... Pos(%f,%f,%f), Dirn(%f,%f,%f)\n", pLight->Position.x, pLight->Position.y, pLight->Position.z,
          pLight->Direction.x, pLight->Direction.y, pLight->Direction.z);
    TRACE("... Range(%f), Falloff(%f), Theta(%f), Phi(%f)\n", pLight->Range, pLight->Falloff, pLight->Theta, pLight->Phi);

    /* Save away the information */
    memcpy(&object->OriginalParms, pLight, sizeof(D3DLIGHT9));

    switch (pLight->Type) {
    case D3DLIGHT_POINT:
        /* Position */
        object->lightPosn[0] = pLight->Position.x;
        object->lightPosn[1] = pLight->Position.y;
        object->lightPosn[2] = pLight->Position.z;
        object->lightPosn[3] = 1.0f;
        object->cutoff = 180.0f;
        /* FIXME: Range */
        break;

    case D3DLIGHT_DIRECTIONAL:
        /* Direction */
        object->lightPosn[0] = -pLight->Direction.x;
        object->lightPosn[1] = -pLight->Direction.y;
        object->lightPosn[2] = -pLight->Direction.z;
        object->lightPosn[3] = 0.0;
        object->exponent     = 0.0f;
        object->cutoff       = 180.0f;
        break;

    case D3DLIGHT_SPOT:
        /* Position */
        object->lightPosn[0] = pLight->Position.x;
        object->lightPosn[1] = pLight->Position.y;
        object->lightPosn[2] = pLight->Position.z;
        object->lightPosn[3] = 1.0;

        /* Direction */
        object->lightDirn[0] = pLight->Direction.x;
        object->lightDirn[1] = pLight->Direction.y;
        object->lightDirn[2] = pLight->Direction.z;
        object->lightDirn[3] = 1.0;

        /*
         * opengl-ish and d3d-ish spot lights use too different models for the
         * light "intensity" as a function of the angle towards the main light direction,
         * so we only can approximate very roughly.
         * however spot lights are rather rarely used in games (if ever used at all).
         * furthermore if still used, probably nobody pays attention to such details.
         */
        if (pLight->Falloff == 0) {
            rho = 6.28f;
        } else {
            rho = pLight->Theta + (pLight->Phi - pLight->Theta)/(2*pLight->Falloff);
        }
        if (rho < 0.0001) rho = 0.0001f;
        object->exponent = -0.3/log(cos(rho/2));
        object->cutoff = pLight->Phi*90/M_PI;

        /* FIXME: Range */
        break;

    default:
        FIXME("Unrecognized light type %d\n", pLight->Type);
    }

    /* Update the live definitions if the light is currently assigned a glIndex */
    if (object->glIndex != -1) {
        setup_light(iface, object->glIndex, object);
    }
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_GetLight(IWineD3DDevice *iface, DWORD Index, WINED3DLIGHT* pLight) {
    PLIGHTINFOEL *lightInfo = NULL;
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : Idx(%ld), pLight(%p)\n", This, Index, pLight);

    /* Locate the light in the live lights */
    lightInfo = This->stateBlock->lights;
    while (lightInfo != NULL && lightInfo->OriginalIndex != Index) lightInfo = lightInfo->next;

    if (lightInfo == NULL) {
        TRACE("Light information requested but light not defined\n");
        return D3DERR_INVALIDCALL;
    }

    memcpy(pLight, &lightInfo->OriginalParms, sizeof(D3DLIGHT9));
    return D3D_OK;
}

/*****
 * Get / Set Light Enable
 *   (Note for consistency, renamed d3dx function by adding the 'set' prefix)
 *****/
HRESULT WINAPI IWineD3DDeviceImpl_SetLightEnable(IWineD3DDevice *iface, DWORD Index, BOOL Enable) {
    PLIGHTINFOEL *lightInfo = NULL;
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : Idx(%ld), enable? %d\n", This, Index, Enable);

    /* If recording state block, just add to end of lights chain with changedEnable set to true */
    if (This->isRecordingState) {
        lightInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PLIGHTINFOEL));
        if (NULL == lightInfo) {
            return D3DERR_OUTOFVIDEOMEMORY;
        }
        lightInfo->OriginalIndex = Index;
        lightInfo->glIndex = -1;
        lightInfo->enabledChanged = TRUE;

        /* Add to the END of the chain of lights changes to be replayed */
        if (This->updateStateBlock->lights == NULL) {
            This->updateStateBlock->lights = lightInfo;
        } else {
            PLIGHTINFOEL *temp = This->updateStateBlock->lights;
            while (temp->next != NULL) temp=temp->next;
            temp->next = lightInfo;
        }
        TRACE("Recording... not performing anything more\n");
        return D3D_OK;
    }

    /* Not recording... So, locate the light in the live lights */
    lightInfo = This->stateBlock->lights;
    while (lightInfo != NULL && lightInfo->OriginalIndex != Index) lightInfo = lightInfo->next;

    /* Special case - enabling an undefined light creates one with a strict set of parms! */
    if (lightInfo == NULL) {
        D3DLIGHT9 lightParms;
        /* Warning - untested code :-) Prob safe to change fixme to a trace but
             wait until someone confirms it seems to work!                     */
        TRACE("Light enabled requested but light not defined, so defining one!\n");
        lightParms.Type = D3DLIGHT_DIRECTIONAL;
        lightParms.Diffuse.r = 1.0;
        lightParms.Diffuse.g = 1.0;
        lightParms.Diffuse.b = 1.0;
        lightParms.Diffuse.a = 0.0;
        lightParms.Specular.r = 0.0;
        lightParms.Specular.g = 0.0;
        lightParms.Specular.b = 0.0;
        lightParms.Specular.a = 0.0;
        lightParms.Ambient.r = 0.0;
        lightParms.Ambient.g = 0.0;
        lightParms.Ambient.b = 0.0;
        lightParms.Ambient.a = 0.0;
        lightParms.Position.x = 0.0;
        lightParms.Position.y = 0.0;
        lightParms.Position.z = 0.0;
        lightParms.Direction.x = 0.0;
        lightParms.Direction.y = 0.0;
        lightParms.Direction.z = 1.0;
        lightParms.Range = 0.0;
        lightParms.Falloff = 0.0;
        lightParms.Attenuation0 = 0.0;
        lightParms.Attenuation1 = 0.0;
        lightParms.Attenuation2 = 0.0;
        lightParms.Theta = 0.0;
        lightParms.Phi = 0.0;
        IWineD3DDeviceImpl_SetLight(iface, Index, &lightParms);

        /* Search for it again! Should be fairly quick as near head of list */
        lightInfo = This->stateBlock->lights;
        while (lightInfo != NULL && lightInfo->OriginalIndex != Index) lightInfo = lightInfo->next;
        if (lightInfo == NULL) {
            FIXME("Adding default lights has failed dismally\n");
            return D3DERR_INVALIDCALL;
        }
    }

    /* OK, we now have a light... */
    if (Enable == FALSE) {

        /* If we are disabling it, check it was enabled, and
           still only do something if it has assigned a glIndex (which it should have!)   */
        if ((lightInfo->lightEnabled) && (lightInfo->glIndex != -1)) {
            TRACE("Disabling light set up at gl idx %ld\n", lightInfo->glIndex);
            ENTER_GL();
            glDisable(GL_LIGHT0 + lightInfo->glIndex);
            checkGLcall("glDisable GL_LIGHT0+Index");
            LEAVE_GL();
        } else {
            TRACE("Nothing to do as light was not enabled\n");
        }
        lightInfo->lightEnabled = FALSE;
    } else {

        /* We are enabling it. If it is enabled, it's really simple */
        if (lightInfo->lightEnabled) {
            /* nop */
            TRACE("Nothing to do as light was enabled\n");

        /* If it already has a glIndex, it's still simple */
        } else if (lightInfo->glIndex != -1) {
            TRACE("Reusing light as already set up at gl idx %ld\n", lightInfo->glIndex);
            lightInfo->lightEnabled = TRUE;
            ENTER_GL();
            glEnable(GL_LIGHT0 + lightInfo->glIndex);
            checkGLcall("glEnable GL_LIGHT0+Index already setup");
            LEAVE_GL();

        /* Otherwise got to find space - lights are ordered gl indexes first */
        } else {
            PLIGHTINFOEL *bsf  = NULL;
            PLIGHTINFOEL *pos  = This->stateBlock->lights;
            PLIGHTINFOEL *prev = NULL;
            int           Index= 0;
            int           glIndex = -1;

            /* Try to minimize changes as much as possible */
            while (pos != NULL && pos->glIndex != -1 && Index < This->maxConcurrentLights) {

                /* Try to remember which index can be replaced if necessary */
                if (bsf==NULL && pos->lightEnabled == FALSE) {
                    /* Found a light we can replace, save as best replacement */
                    bsf = pos;
                }

                /* Step to next space */
                prev = pos;
                pos = pos->next;
                Index ++;
            }

            /* If we have too many active lights, fail the call */
            if ((Index == This->maxConcurrentLights) && (bsf == NULL)) {
                FIXME("Program requests too many concurrent lights\n");
                return D3DERR_INVALIDCALL;

            /* If we have allocated all lights, but not all are enabled,
               reuse one which is not enabled                           */
            } else if (Index == This->maxConcurrentLights) {
                /* use bsf - Simply swap the new light and the BSF one */
                PLIGHTINFOEL *bsfNext = bsf->next;
                PLIGHTINFOEL *bsfPrev = bsf->prev;

                /* Sort out ends */
                if (lightInfo->next != NULL) lightInfo->next->prev = bsf;
                if (bsf->prev != NULL) {
                    bsf->prev->next = lightInfo;
                } else {
                    This->stateBlock->lights = lightInfo;
                }

                /* If not side by side, lots of chains to update */
                if (bsf->next != lightInfo) {
                    lightInfo->prev->next = bsf;
                    bsf->next->prev = lightInfo;
                    bsf->next       = lightInfo->next;
                    bsf->prev       = lightInfo->prev;
                    lightInfo->next = bsfNext;
                    lightInfo->prev = bsfPrev;

                } else {
                    /* Simple swaps */
                    bsf->prev = lightInfo;
                    bsf->next = lightInfo->next;
                    lightInfo->next = bsf;
                    lightInfo->prev = bsfPrev;
                }


                /* Update states */
                glIndex = bsf->glIndex;
                bsf->glIndex = -1;
                lightInfo->glIndex = glIndex;
                lightInfo->lightEnabled = TRUE;

                /* Finally set up the light in gl itself */
                TRACE("Replacing light which was set up at gl idx %ld\n", lightInfo->glIndex);
                ENTER_GL();
                setup_light(iface, glIndex, lightInfo);
                glEnable(GL_LIGHT0 + glIndex);
                checkGLcall("glEnable GL_LIGHT0 new setup");
                LEAVE_GL();

            /* If we reached the end of the allocated lights, with space in the
               gl lights, setup a new light                                     */
            } else if (pos->glIndex == -1) {

                /* We reached the end of the allocated gl lights, so already
                    know the index of the next one!                          */
                glIndex = Index;
                lightInfo->glIndex = glIndex;
                lightInfo->lightEnabled = TRUE;

                /* In an ideal world, it's already in the right place */
                if (lightInfo->prev == NULL || lightInfo->prev->glIndex!=-1) {
                   /* No need to move it */
                } else {
                    /* Remove this light from the list */
                    lightInfo->prev->next = lightInfo->next;
                    if (lightInfo->next != NULL) {
                        lightInfo->next->prev = lightInfo->prev;
                    }

                    /* Add in at appropriate place (inbetween prev and pos) */
                    lightInfo->prev = prev;
                    lightInfo->next = pos;
                    if (prev == NULL) {
                        This->stateBlock->lights = lightInfo;
                    } else {
                        prev->next = lightInfo;
                    }
                    if (pos != NULL) {
                        pos->prev = lightInfo;
                    }
                }

                /* Finally set up the light in gl itself */
                TRACE("Defining new light at gl idx %ld\n", lightInfo->glIndex);
                ENTER_GL();
                setup_light(iface, glIndex, lightInfo);
                glEnable(GL_LIGHT0 + glIndex);
                checkGLcall("glEnable GL_LIGHT0 new setup");
                LEAVE_GL();

            }
        }
    }
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_GetLightEnable(IWineD3DDevice *iface, DWORD Index,BOOL* pEnable) {

    PLIGHTINFOEL *lightInfo = NULL;
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : for idx(%ld)\n", This, Index);

    /* Locate the light in the live lights */
    lightInfo = This->stateBlock->lights;
    while (lightInfo != NULL && lightInfo->OriginalIndex != Index) lightInfo = lightInfo->next;

    if (lightInfo == NULL) {
        TRACE("Light enabled state requested but light not defined\n");
        return D3DERR_INVALIDCALL;
    }
    *pEnable = lightInfo->lightEnabled;
    return D3D_OK;
}

/*****
 * Get / Set Clip Planes
 *****/
HRESULT WINAPI IWineD3DDeviceImpl_SetClipPlane(IWineD3DDevice *iface, DWORD Index, CONST float *pPlane) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : for idx %ld, %p\n", This, Index, pPlane);

    /* Validate Index */
    if (Index >= GL_LIMITS(clipplanes)) {
        TRACE("Application has requested clipplane this device doesn't support\n");
        return D3DERR_INVALIDCALL;
    }

    This->updateStateBlock->changed.clipplane[Index] = TRUE;
    This->updateStateBlock->set.clipplane[Index] = TRUE;
    This->updateStateBlock->clipplane[Index][0] = pPlane[0];
    This->updateStateBlock->clipplane[Index][1] = pPlane[1];
    This->updateStateBlock->clipplane[Index][2] = pPlane[2];
    This->updateStateBlock->clipplane[Index][3] = pPlane[3];

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return D3D_OK;
    }

    /* Apply it */

    ENTER_GL();

    /* Clip Plane settings are affected by the model view in OpenGL, the View transform in direct3d */
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixf((float *) &This->stateBlock->transforms[D3DTS_VIEW].u.m[0][0]);

    TRACE("Clipplane [%f,%f,%f,%f]\n",
          This->updateStateBlock->clipplane[Index][0],
          This->updateStateBlock->clipplane[Index][1],
          This->updateStateBlock->clipplane[Index][2],
          This->updateStateBlock->clipplane[Index][3]);
    glClipPlane(GL_CLIP_PLANE0 + Index, This->updateStateBlock->clipplane[Index]);
    checkGLcall("glClipPlane");

    glPopMatrix();
    LEAVE_GL();

    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_GetClipPlane(IWineD3DDevice *iface, DWORD Index, float *pPlane) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : for idx %ld\n", This, Index);

    /* Validate Index */
    if (Index >= GL_LIMITS(clipplanes)) {
        TRACE("Application has requested clipplane this device doesn't support\n");
        return D3DERR_INVALIDCALL;
    }

    pPlane[0] = This->stateBlock->clipplane[Index][0];
    pPlane[1] = This->stateBlock->clipplane[Index][1];
    pPlane[2] = This->stateBlock->clipplane[Index][2];
    pPlane[3] = This->stateBlock->clipplane[Index][3];
    return D3D_OK;
}

/*****
 * Get / Set Clip Plane Status
 *   WARNING: This code relies on the fact that D3DCLIPSTATUS8 == D3DCLIPSTATUS9
 *****/
HRESULT  WINAPI  IWineD3DDeviceImpl_SetClipStatus(IWineD3DDevice *iface, CONST WINED3DCLIPSTATUS* pClipStatus) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    FIXME("(%p) : stub\n", This);
    if (NULL == pClipStatus) {
      return D3DERR_INVALIDCALL;
    }
    This->updateStateBlock->clip_status.ClipUnion = pClipStatus->ClipUnion;
    This->updateStateBlock->clip_status.ClipIntersection = pClipStatus->ClipIntersection;
    return D3D_OK;
}

HRESULT  WINAPI  IWineD3DDeviceImpl_GetClipStatus(IWineD3DDevice *iface, WINED3DCLIPSTATUS* pClipStatus) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    FIXME("(%p) : stub\n", This);
    if (NULL == pClipStatus) {
      return D3DERR_INVALIDCALL;
    }
    pClipStatus->ClipUnion = This->updateStateBlock->clip_status.ClipUnion;
    pClipStatus->ClipIntersection = This->updateStateBlock->clip_status.ClipIntersection;
    return D3D_OK;
}

/*****
 * Get / Set Material
 *   WARNING: This code relies on the fact that D3DMATERIAL8 == D3DMATERIAL9
 *****/
HRESULT WINAPI IWineD3DDeviceImpl_SetMaterial(IWineD3DDevice *iface, CONST WINED3DMATERIAL* pMaterial) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    This->updateStateBlock->changed.material = TRUE;
    This->updateStateBlock->set.material = TRUE;
    memcpy(&This->updateStateBlock->material, pMaterial, sizeof(WINED3DMATERIAL));

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return D3D_OK;
    }

    ENTER_GL();
    TRACE("(%p) : Diffuse (%f,%f,%f,%f)\n", This, pMaterial->Diffuse.r, pMaterial->Diffuse.g, pMaterial->Diffuse.b, pMaterial->Diffuse.a);
    TRACE("(%p) : Ambient (%f,%f,%f,%f)\n", This, pMaterial->Ambient.r, pMaterial->Ambient.g, pMaterial->Ambient.b, pMaterial->Ambient.a);
    TRACE("(%p) : Specular (%f,%f,%f,%f)\n", This, pMaterial->Specular.r, pMaterial->Specular.g, pMaterial->Specular.b, pMaterial->Specular.a);
    TRACE("(%p) : Emissive (%f,%f,%f,%f)\n", This, pMaterial->Emissive.r, pMaterial->Emissive.g, pMaterial->Emissive.b, pMaterial->Emissive.a);
    TRACE("(%p) : Power (%f)\n", This, pMaterial->Power);

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, (float*) &This->updateStateBlock->material.Ambient);
    checkGLcall("glMaterialfv");
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, (float*) &This->updateStateBlock->material.Diffuse);
    checkGLcall("glMaterialfv");

    /* Only change material color if specular is enabled, otherwise it is set to black */
    if (This->stateBlock->renderState[WINED3DRS_SPECULARENABLE]) {
       glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, (float*) &This->updateStateBlock->material.Specular);
       checkGLcall("glMaterialfv");
    } else {
       float black[4] = {0.0f, 0.0f, 0.0f, 0.0f};
       glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, &black[0]);
       checkGLcall("glMaterialfv");
    }
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, (float*) &This->updateStateBlock->material.Emissive);
    checkGLcall("glMaterialfv");
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, This->updateStateBlock->material.Power);
    checkGLcall("glMaterialf");

    LEAVE_GL();
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_GetMaterial(IWineD3DDevice *iface, WINED3DMATERIAL* pMaterial) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    memcpy(pMaterial, &This->updateStateBlock->material, sizeof (WINED3DMATERIAL));
    TRACE("(%p) : Diffuse (%f,%f,%f,%f)\n", This, pMaterial->Diffuse.r, pMaterial->Diffuse.g, pMaterial->Diffuse.b, pMaterial->Diffuse.a);
    TRACE("(%p) : Ambient (%f,%f,%f,%f)\n", This, pMaterial->Ambient.r, pMaterial->Ambient.g, pMaterial->Ambient.b, pMaterial->Ambient.a);
    TRACE("(%p) : Specular (%f,%f,%f,%f)\n", This, pMaterial->Specular.r, pMaterial->Specular.g, pMaterial->Specular.b, pMaterial->Specular.a);
    TRACE("(%p) : Emissive (%f,%f,%f,%f)\n", This, pMaterial->Emissive.r, pMaterial->Emissive.g, pMaterial->Emissive.b, pMaterial->Emissive.a);
    TRACE("(%p) : Power (%f)\n", This, pMaterial->Power);
    return D3D_OK;
}

/*****
 * Get / Set Indices
 *****/
HRESULT WINAPI IWineD3DDeviceImpl_SetIndices(IWineD3DDevice *iface, IWineD3DIndexBuffer* pIndexData,
                                             UINT BaseVertexIndex) {
    IWineD3DDeviceImpl  *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DIndexBuffer *oldIdxs;

    TRACE("(%p) : Setting to %p, base %d\n", This, pIndexData, BaseVertexIndex);
    oldIdxs = This->updateStateBlock->pIndexData;

    This->updateStateBlock->changed.indices = TRUE;
    This->updateStateBlock->set.indices = TRUE;
    This->updateStateBlock->pIndexData = pIndexData;
    This->updateStateBlock->baseVertexIndex = BaseVertexIndex;

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return D3D_OK;
    }

    if (oldIdxs)    IWineD3DIndexBuffer_Release(oldIdxs);
    if (pIndexData) IWineD3DIndexBuffer_AddRef(This->stateBlock->pIndexData);
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_GetIndices(IWineD3DDevice *iface, IWineD3DIndexBuffer** ppIndexData, UINT* pBaseVertexIndex) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    *ppIndexData = This->stateBlock->pIndexData;

    /* up ref count on ppindexdata */
    if (*ppIndexData) IWineD3DIndexBuffer_AddRef(*ppIndexData);
    *pBaseVertexIndex = This->stateBlock->baseVertexIndex;

    return D3D_OK;
}

/*****
 * Get / Set Viewports
 *****/
HRESULT WINAPI IWineD3DDeviceImpl_SetViewport(IWineD3DDevice *iface, CONST WINED3DVIEWPORT* pViewport) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("(%p)\n", This);
    This->updateStateBlock->changed.viewport = TRUE;
    This->updateStateBlock->set.viewport = TRUE;
    memcpy(&This->updateStateBlock->viewport, pViewport, sizeof(WINED3DVIEWPORT));

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return D3D_OK;
    }

    ENTER_GL();

    TRACE("(%p) : x=%ld, y=%ld, wid=%ld, hei=%ld, minz=%f, maxz=%f\n", This,
          pViewport->X, pViewport->Y, pViewport->Width, pViewport->Height, pViewport->MinZ, pViewport->MaxZ);

    glDepthRange(pViewport->MinZ, pViewport->MaxZ);
    checkGLcall("glDepthRange");
    /* Note: GL requires lower left, DirectX supplies upper left */
    /* TODO: replace usage of renderTarget with context management */
    glViewport(pViewport->X,
               (((IWineD3DSurfaceImpl *)This->renderTarget)->currentDesc.Height - (pViewport->Y + pViewport->Height)),
               pViewport->Width, pViewport->Height);

    checkGLcall("glViewport");

    LEAVE_GL();

    return D3D_OK;

}

HRESULT WINAPI IWineD3DDeviceImpl_GetViewport(IWineD3DDevice *iface, WINED3DVIEWPORT* pViewport) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p)\n", This);
    memcpy(pViewport, &This->stateBlock->viewport, sizeof(WINED3DVIEWPORT));
    return D3D_OK;
}

/*****
 * Get / Set Render States
 * TODO: Verify against dx9 definitions
 *****/
HRESULT WINAPI IWineD3DDeviceImpl_SetRenderState(IWineD3DDevice *iface, D3DRENDERSTATETYPE State, DWORD Value) {

    IWineD3DDeviceImpl  *This     = (IWineD3DDeviceImpl *)iface;
    DWORD                OldValue = This->stateBlock->renderState[State];

    /* Simple way of referring to either a DWORD or a 4 byte float */
    union {
        DWORD d;
        float f;
    } tmpvalue;

    TRACE("(%p)->state = %s(%d), value = %ld\n", This, debug_d3drenderstate(State), State, Value);
    This->updateStateBlock->changed.renderState[State] = TRUE;
    This->updateStateBlock->set.renderState[State] = TRUE;
    This->updateStateBlock->renderState[State] = Value;

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return D3D_OK;
    }

    ENTER_GL();

    switch (State) {
    case WINED3DRS_FILLMODE                  :
        switch ((D3DFILLMODE) Value) {
        case D3DFILL_POINT               : glPolygonMode(GL_FRONT_AND_BACK, GL_POINT); break;
        case D3DFILL_WIREFRAME           : glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); break;
        case D3DFILL_SOLID               : glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); break;
        default:
            FIXME("Unrecognized WINED3DRS_FILLMODE value %ld\n", Value);
        }
        checkGLcall("glPolygonMode (fillmode)");
        break;

    case WINED3DRS_LIGHTING                  :
        if (Value) {
            glEnable(GL_LIGHTING);
            checkGLcall("glEnable GL_LIGHTING");
        } else {
            glDisable(GL_LIGHTING);
            checkGLcall("glDisable GL_LIGHTING");
        }
        break;

    case WINED3DRS_ZENABLE                   :
        switch ((D3DZBUFFERTYPE) Value) {
        case D3DZB_FALSE:
            glDisable(GL_DEPTH_TEST);
            checkGLcall("glDisable GL_DEPTH_TEST");
            break;
        case D3DZB_TRUE:
            glEnable(GL_DEPTH_TEST);
            checkGLcall("glEnable GL_DEPTH_TEST");
            break;
        case D3DZB_USEW:
            glEnable(GL_DEPTH_TEST);
            checkGLcall("glEnable GL_DEPTH_TEST");
            FIXME("W buffer is not well handled\n");
            break;
        default:
            FIXME("Unrecognized D3DZBUFFERTYPE value %ld\n", Value);
        }
        break;

    case WINED3DRS_CULLMODE                  :

        /* If we are culling "back faces with clockwise vertices" then
           set front faces to be counter clockwise and enable culling
           of back faces                                               */
        switch ((D3DCULL) Value) {
        case D3DCULL_NONE:
            glDisable(GL_CULL_FACE);
            checkGLcall("glDisable GL_CULL_FACE");
            break;
        case D3DCULL_CW:
            glEnable(GL_CULL_FACE);
            checkGLcall("glEnable GL_CULL_FACE");
            if (This->renderUpsideDown) {
                glFrontFace(GL_CW);
                checkGLcall("glFrontFace GL_CW");
            } else {
                glFrontFace(GL_CCW);
                checkGLcall("glFrontFace GL_CCW");
            }
            glCullFace(GL_BACK);
            break;
        case D3DCULL_CCW:
            glEnable(GL_CULL_FACE);
            checkGLcall("glEnable GL_CULL_FACE");
            if (This->renderUpsideDown) {
                glFrontFace(GL_CCW);
                checkGLcall("glFrontFace GL_CCW");
            } else {
                glFrontFace(GL_CW);
                checkGLcall("glFrontFace GL_CW");
            }
            glCullFace(GL_BACK);
            break;
        default:
            FIXME("Unrecognized/Unhandled D3DCULL value %ld\n", Value);
        }
        break;

    case WINED3DRS_SHADEMODE                 :
        switch ((D3DSHADEMODE) Value) {
        case D3DSHADE_FLAT:
            glShadeModel(GL_FLAT);
            checkGLcall("glShadeModel");
            break;
        case D3DSHADE_GOURAUD:
            glShadeModel(GL_SMOOTH);
            checkGLcall("glShadeModel");
            break;
        case D3DSHADE_PHONG:
            FIXME("D3DSHADE_PHONG isn't supported?\n");

            LEAVE_GL();
            return D3DERR_INVALIDCALL;
        default:
            FIXME("Unrecognized/Unhandled D3DSHADEMODE value %ld\n", Value);
        }
        break;

    case WINED3DRS_DITHERENABLE              :
        if (Value) {
            glEnable(GL_DITHER);
            checkGLcall("glEnable GL_DITHER");
        } else {
            glDisable(GL_DITHER);
            checkGLcall("glDisable GL_DITHER");
        }
        break;

    case WINED3DRS_ZWRITEENABLE              :
        if (Value) {
            glDepthMask(1);
            checkGLcall("glDepthMask");
        } else {
            glDepthMask(0);
            checkGLcall("glDepthMask");
        }
        break;

    case WINED3DRS_ZFUNC                     :
        {
            int glParm = GL_LESS;

            switch ((D3DCMPFUNC) Value) {
            case D3DCMP_NEVER:         glParm=GL_NEVER; break;
            case D3DCMP_LESS:          glParm=GL_LESS; break;
            case D3DCMP_EQUAL:         glParm=GL_EQUAL; break;
            case D3DCMP_LESSEQUAL:     glParm=GL_LEQUAL; break;
            case D3DCMP_GREATER:       glParm=GL_GREATER; break;
            case D3DCMP_NOTEQUAL:      glParm=GL_NOTEQUAL; break;
            case D3DCMP_GREATEREQUAL:  glParm=GL_GEQUAL; break;
            case D3DCMP_ALWAYS:        glParm=GL_ALWAYS; break;
            default:
                FIXME("Unrecognized/Unhandled D3DCMPFUNC value %ld\n", Value);
            }
            glDepthFunc(glParm);
            checkGLcall("glDepthFunc");
        }
        break;

    case WINED3DRS_AMBIENT                   :
        {
            float col[4];
            D3DCOLORTOGLFLOAT4(Value, col);
            TRACE("Setting ambient to (%f,%f,%f,%f)\n", col[0], col[1], col[2], col[3]);
            glLightModelfv(GL_LIGHT_MODEL_AMBIENT, col);
            checkGLcall("glLightModel for MODEL_AMBIENT");

        }
        break;

    case WINED3DRS_ALPHABLENDENABLE          :
        if (Value) {
            glEnable(GL_BLEND);
            checkGLcall("glEnable GL_BLEND");
        } else {
            glDisable(GL_BLEND);
            checkGLcall("glDisable GL_BLEND");
        };
        break;

    case WINED3DRS_SRCBLEND                  :
    case WINED3DRS_DESTBLEND                 :
        {
            int newVal = GL_ZERO;
            switch (Value) {
            case D3DBLEND_ZERO               : newVal = GL_ZERO;  break;
            case D3DBLEND_ONE                : newVal = GL_ONE;  break;
            case D3DBLEND_SRCCOLOR           : newVal = GL_SRC_COLOR;  break;
            case D3DBLEND_INVSRCCOLOR        : newVal = GL_ONE_MINUS_SRC_COLOR;  break;
            case D3DBLEND_SRCALPHA           : newVal = GL_SRC_ALPHA;  break;
            case D3DBLEND_INVSRCALPHA        : newVal = GL_ONE_MINUS_SRC_ALPHA;  break;
            case D3DBLEND_DESTALPHA          : newVal = GL_DST_ALPHA;  break;
            case D3DBLEND_INVDESTALPHA       : newVal = GL_ONE_MINUS_DST_ALPHA;  break;
            case D3DBLEND_DESTCOLOR          : newVal = GL_DST_COLOR;  break;
            case D3DBLEND_INVDESTCOLOR       : newVal = GL_ONE_MINUS_DST_COLOR;  break;
            case D3DBLEND_SRCALPHASAT        : newVal = GL_SRC_ALPHA_SATURATE;  break;

            case D3DBLEND_BOTHSRCALPHA       : newVal = GL_SRC_ALPHA;
                This->srcBlend = newVal;
                This->dstBlend = newVal;
                break;

            case D3DBLEND_BOTHINVSRCALPHA    : newVal = GL_ONE_MINUS_SRC_ALPHA;
                This->srcBlend = newVal;
                This->dstBlend = newVal;
                break;
            default:
                FIXME("Unrecognized src/dest blend value %ld (%d)\n", Value, State);
            }

            if (State == WINED3DRS_SRCBLEND) This->srcBlend = newVal;
            if (State == WINED3DRS_DESTBLEND) This->dstBlend = newVal;
            TRACE("glBlendFunc src=%x, dst=%x\n", This->srcBlend, This->dstBlend);
            glBlendFunc(This->srcBlend, This->dstBlend);

            checkGLcall("glBlendFunc");
        }
        break;

    case WINED3DRS_ALPHATESTENABLE           :
        if (Value) {
            glEnable(GL_ALPHA_TEST);
            checkGLcall("glEnable GL_ALPHA_TEST");
        } else {
            glDisable(GL_ALPHA_TEST);
            checkGLcall("glDisable GL_ALPHA_TEST");
        }
        break;

    case WINED3DRS_ALPHAFUNC                 :
        {
            int glParm = GL_LESS;
            float ref = ((float) This->stateBlock->renderState[WINED3DRS_ALPHAREF]) / 255.0f;

            switch ((D3DCMPFUNC) Value) {
            case D3DCMP_NEVER:         glParm = GL_NEVER; break;
            case D3DCMP_LESS:          glParm = GL_LESS; break;
            case D3DCMP_EQUAL:         glParm = GL_EQUAL; break;
            case D3DCMP_LESSEQUAL:     glParm = GL_LEQUAL; break;
            case D3DCMP_GREATER:       glParm = GL_GREATER; break;
            case D3DCMP_NOTEQUAL:      glParm = GL_NOTEQUAL; break;
            case D3DCMP_GREATEREQUAL:  glParm = GL_GEQUAL; break;
            case D3DCMP_ALWAYS:        glParm = GL_ALWAYS; break;
            default:
                FIXME("Unrecognized/Unhandled D3DCMPFUNC value %ld\n", Value);
            }
            TRACE("glAlphaFunc with Parm=%x, ref=%f\n", glParm, ref);
            glAlphaFunc(glParm, ref);
            This->alphafunc = glParm;
            checkGLcall("glAlphaFunc");
        }
        break;

    case WINED3DRS_ALPHAREF                  :
        {
            int glParm = This->alphafunc;
            float ref = 1.0f;

            ref = ((float) Value) / 255.0f;
            TRACE("glAlphaFunc with Parm=%x, ref=%f\n", glParm, ref);
            glAlphaFunc(glParm, ref);
            checkGLcall("glAlphaFunc");
        }
        break;

    case WINED3DRS_CLIPPLANEENABLE           :
    case WINED3DRS_CLIPPING                  :
        {
            /* Ensure we only do the changed clip planes */
            DWORD enable  = 0xFFFFFFFF;
            DWORD disable = 0x00000000;

            /* If enabling / disabling all */
            if (State == WINED3DRS_CLIPPING) {
                if (Value) {
                    enable  = This->stateBlock->renderState[WINED3DRS_CLIPPLANEENABLE];
                    disable = 0x00;
                } else {
                    disable = This->stateBlock->renderState[WINED3DRS_CLIPPLANEENABLE];
                    enable  = 0x00;
                }
            } else {
                enable =   Value & ~OldValue;
                disable = ~Value &  OldValue;
            }

            if (enable & D3DCLIPPLANE0)  { glEnable(GL_CLIP_PLANE0);  checkGLcall("glEnable(clip plane 0)"); }
            if (enable & D3DCLIPPLANE1)  { glEnable(GL_CLIP_PLANE1);  checkGLcall("glEnable(clip plane 1)"); }
            if (enable & D3DCLIPPLANE2)  { glEnable(GL_CLIP_PLANE2);  checkGLcall("glEnable(clip plane 2)"); }
            if (enable & D3DCLIPPLANE3)  { glEnable(GL_CLIP_PLANE3);  checkGLcall("glEnable(clip plane 3)"); }
            if (enable & D3DCLIPPLANE4)  { glEnable(GL_CLIP_PLANE4);  checkGLcall("glEnable(clip plane 4)"); }
            if (enable & D3DCLIPPLANE5)  { glEnable(GL_CLIP_PLANE5);  checkGLcall("glEnable(clip plane 5)"); }

            if (disable & D3DCLIPPLANE0) { glDisable(GL_CLIP_PLANE0); checkGLcall("glDisable(clip plane 0)"); }
            if (disable & D3DCLIPPLANE1) { glDisable(GL_CLIP_PLANE1); checkGLcall("glDisable(clip plane 1)"); }
            if (disable & D3DCLIPPLANE2) { glDisable(GL_CLIP_PLANE2); checkGLcall("glDisable(clip plane 2)"); }
            if (disable & D3DCLIPPLANE3) { glDisable(GL_CLIP_PLANE3); checkGLcall("glDisable(clip plane 3)"); }
            if (disable & D3DCLIPPLANE4) { glDisable(GL_CLIP_PLANE4); checkGLcall("glDisable(clip plane 4)"); }
            if (disable & D3DCLIPPLANE5) { glDisable(GL_CLIP_PLANE5); checkGLcall("glDisable(clip plane 5)"); }

            /** update clipping status */
            if (enable) {
              This->stateBlock->clip_status.ClipUnion = 0;
              This->stateBlock->clip_status.ClipIntersection = 0xFFFFFFFF;
            } else {
              This->stateBlock->clip_status.ClipUnion = 0;
              This->stateBlock->clip_status.ClipIntersection = 0;
            }
        }
        break;

    case WINED3DRS_BLENDOP                   :
        {
            int glParm = GL_FUNC_ADD;

            switch ((D3DBLENDOP) Value) {
            case D3DBLENDOP_ADD              : glParm = GL_FUNC_ADD;              break;
            case D3DBLENDOP_SUBTRACT         : glParm = GL_FUNC_SUBTRACT;         break;
            case D3DBLENDOP_REVSUBTRACT      : glParm = GL_FUNC_REVERSE_SUBTRACT; break;
            case D3DBLENDOP_MIN              : glParm = GL_MIN;                   break;
            case D3DBLENDOP_MAX              : glParm = GL_MAX;                   break;
            default:
                FIXME("Unrecognized/Unhandled D3DBLENDOP value %ld\n", Value);
            }
            TRACE("glBlendEquation(%x)\n", glParm);
            glBlendEquation(glParm);
            checkGLcall("glBlendEquation");
        }
        break;

    case WINED3DRS_TEXTUREFACTOR             :
        {
            unsigned int i;

            /* Note the texture color applies to all textures whereas
               GL_TEXTURE_ENV_COLOR applies to active only */
            float col[4];
            D3DCOLORTOGLFLOAT4(Value, col);
            /* Set the default alpha blend color */
            glBlendColor(col[0], col[1], col[2], col[3]);
            checkGLcall("glBlendColor");

            /* And now the default texture color as well */
            for (i = 0; i < GL_LIMITS(textures); i++) {

                /* Note the D3DRS value applies to all textures, but GL has one
                   per texture, so apply it now ready to be used!               */
                if (GL_SUPPORT(ARB_MULTITEXTURE)) {
                    GLACTIVETEXTURE(i);
                } else if (i>0) {
                    FIXME("Program using multiple concurrent textures which this opengl implementation doesn't support\n");
                }

                glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, &col[0]);
                checkGLcall("glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);");
            }
        }
        break;

    case WINED3DRS_SPECULARENABLE            :
        {
            /* Originally this used glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,GL_SEPARATE_SPECULAR_COLOR)
               and (GL_LIGHT_MODEL_COLOR_CONTROL,GL_SINGLE_COLOR) to swap between enabled/disabled
               specular color. This is wrong:
               Separate specular color means the specular colour is maintained separately, whereas
               single color means it is merged in. However in both cases they are being used to
               some extent.
               To disable specular color, set it explicitly to black and turn off GL_COLOR_SUM_EXT
               NOTE: If not supported don't give FIXMEs the impact is really minimal and very few people are
                  running 1.4 yet!
             */
              if (Value) {
                glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, (float*) &This->updateStateBlock->material.Specular);
                checkGLcall("glMaterialfv");
                if (GL_SUPPORT(EXT_SECONDARY_COLOR)) {
                  glEnable(GL_COLOR_SUM_EXT);
                } else {
                  TRACE("Specular colors cannot be enabled in this version of opengl\n");
                }
                checkGLcall("glEnable(GL_COLOR_SUM)");
              } else {
                float black[4] = {0.0f, 0.0f, 0.0f, 0.0f};

                /* for the case of enabled lighting: */
                glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, &black[0]);
                checkGLcall("glMaterialfv");

                /* for the case of disabled lighting: */
                if (GL_SUPPORT(EXT_SECONDARY_COLOR)) {
                  glDisable(GL_COLOR_SUM_EXT);
                } else {
                  TRACE("Specular colors cannot be disabled in this version of opengl\n");
                }
                checkGLcall("glDisable(GL_COLOR_SUM)");
              }
        }
        break;

    case WINED3DRS_STENCILENABLE             :
        if (Value) {
            glEnable(GL_STENCIL_TEST);
            checkGLcall("glEnable GL_STENCIL_TEST");
        } else {
            glDisable(GL_STENCIL_TEST);
            checkGLcall("glDisable GL_STENCIL_TEST");
        }
        break;

    case WINED3DRS_STENCILFUNC               :
        {
           int glParm = GL_ALWAYS;
           int ref = This->stateBlock->renderState[WINED3DRS_STENCILREF];
           GLuint mask = This->stateBlock->renderState[WINED3DRS_STENCILMASK];

           switch ((D3DCMPFUNC) Value) {
           case D3DCMP_NEVER:         glParm=GL_NEVER; break;
           case D3DCMP_LESS:          glParm=GL_LESS; break;
           case D3DCMP_EQUAL:         glParm=GL_EQUAL; break;
           case D3DCMP_LESSEQUAL:     glParm=GL_LEQUAL; break;
           case D3DCMP_GREATER:       glParm=GL_GREATER; break;
           case D3DCMP_NOTEQUAL:      glParm=GL_NOTEQUAL; break;
           case D3DCMP_GREATEREQUAL:  glParm=GL_GEQUAL; break;
           case D3DCMP_ALWAYS:        glParm=GL_ALWAYS; break;
           default:
               FIXME("Unrecognized/Unhandled D3DCMPFUNC value %ld\n", Value);
           }
           TRACE("glStencilFunc with Parm=%x, ref=%d, mask=%x\n", glParm, ref, mask);
           This->stencilfunc = glParm;
           glStencilFunc(glParm, ref, mask);
           checkGLcall("glStencilFunc");
        }
        break;

    case WINED3DRS_STENCILREF                :
        {
           int glParm = This->stencilfunc;
           int ref = 0;
           GLuint mask = This->stateBlock->renderState[WINED3DRS_STENCILMASK];

           ref = Value;
           TRACE("glStencilFunc with Parm=%x, ref=%d, mask=%x\n", glParm, ref, mask);
           glStencilFunc(glParm, ref, mask);
           checkGLcall("glStencilFunc");
        }
        break;

    case WINED3DRS_STENCILMASK               :
        {
           int glParm = This->stencilfunc;
           int ref = This->stateBlock->renderState[WINED3DRS_STENCILREF];
           GLuint mask = Value;

           TRACE("glStencilFunc with Parm=%x, ref=%d, mask=%x\n", glParm, ref, mask);
           glStencilFunc(glParm, ref, mask);
           checkGLcall("glStencilFunc");
        }
        break;

    case WINED3DRS_STENCILFAIL               :
        {
            GLenum fail  ;
            GLenum zpass ;
            GLenum zfail ;

            fail = StencilOp(Value);
            glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, &zpass);
            checkGLcall("glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, &zpass);");
            glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, &zfail);
            checkGLcall("glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, &zfail);");

            TRACE("StencilOp fail=%x, zfail=%x, zpass=%x\n", fail, zfail, zpass);
            glStencilOp(fail, zfail, zpass);
            checkGLcall("glStencilOp(fail, zfail, zpass);");
        }
        break;
    case WINED3DRS_STENCILZFAIL              :
        {
            GLenum fail  ;
            GLenum zpass ;
            GLenum zfail ;

            glGetIntegerv(GL_STENCIL_FAIL, &fail);
            checkGLcall("glGetIntegerv(GL_STENCIL_FAIL, &fail);");
            glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, &zpass);
            checkGLcall("glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, &zpass);");
            zfail = StencilOp(Value);

            TRACE("StencilOp fail=%x, zfail=%x, zpass=%x\n", fail, zfail, zpass);
            glStencilOp(fail, zfail, zpass);
            checkGLcall("glStencilOp(fail, zfail, zpass);");
        }
        break;
    case WINED3DRS_STENCILPASS               :
        {
            GLenum fail  ;
            GLenum zpass ;
            GLenum zfail ;

            glGetIntegerv(GL_STENCIL_FAIL, &fail);
            checkGLcall("glGetIntegerv(GL_STENCIL_FAIL, &fail);");
            zpass = StencilOp(Value);
            glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, &zfail);
            checkGLcall("glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, &zfail);");

            TRACE("StencilOp fail=%x, zfail=%x, zpass=%x\n", fail, zfail, zpass);
            glStencilOp(fail, zfail, zpass);
            checkGLcall("glStencilOp(fail, zfail, zpass);");
        }
        break;

    case WINED3DRS_STENCILWRITEMASK          :
        {
            glStencilMask(Value);
            TRACE("glStencilMask(%lu)\n", Value);
            checkGLcall("glStencilMask");
        }
        break;

    case WINED3DRS_FOGENABLE                 :
        {
          if (Value/* && This->stateBlock->renderState[WINED3DRS_FOGTABLEMODE] != D3DFOG_NONE*/) {
               glEnable(GL_FOG);
               checkGLcall("glEnable GL_FOG");
            } else {
               glDisable(GL_FOG);
               checkGLcall("glDisable GL_FOG");
            }
        }
        break;

    case WINED3DRS_RANGEFOGENABLE            :
        {
            if (Value) {
              TRACE("Enabled RANGEFOG");
            } else {
              TRACE("Disabled RANGEFOG");
            }
        }
        break;

    case WINED3DRS_FOGCOLOR                  :
        {
            float col[4];
            D3DCOLORTOGLFLOAT4(Value, col);
            /* Set the default alpha blend color */
            glFogfv(GL_FOG_COLOR, &col[0]);
            checkGLcall("glFog GL_FOG_COLOR");
        }
        break;

    case WINED3DRS_FOGTABLEMODE              :
        {
          glHint(GL_FOG_HINT, GL_NICEST);
          switch (Value) {
          case D3DFOG_NONE:    /* I don't know what to do here */ checkGLcall("glFogi(GL_FOG_MODE, GL_EXP"); break;
          case D3DFOG_EXP:     glFogi(GL_FOG_MODE, GL_EXP); checkGLcall("glFogi(GL_FOG_MODE, GL_EXP"); break;
          case D3DFOG_EXP2:    glFogi(GL_FOG_MODE, GL_EXP2); checkGLcall("glFogi(GL_FOG_MODE, GL_EXP2"); break;
          case D3DFOG_LINEAR:  glFogi(GL_FOG_MODE, GL_LINEAR); checkGLcall("glFogi(GL_FOG_MODE, GL_LINEAR"); break;
          default:
            FIXME("Unsupported Value(%lu) for WINED3DRS_FOGTABLEMODE!\n", Value);
          }
          if (GL_SUPPORT(NV_FOG_DISTANCE)) {
            glFogi(GL_FOG_DISTANCE_MODE_NV, GL_EYE_PLANE_ABSOLUTE_NV);
          }
        }
        break;

    case WINED3DRS_FOGVERTEXMODE             :
        {
          glHint(GL_FOG_HINT, GL_FASTEST);
          switch (Value) {
          case D3DFOG_NONE:    /* I don't know what to do here */ checkGLcall("glFogi(GL_FOG_MODE, GL_EXP"); break;
          case D3DFOG_EXP:     glFogi(GL_FOG_MODE, GL_EXP); checkGLcall("glFogi(GL_FOG_MODE, GL_EXP"); break;
          case D3DFOG_EXP2:    glFogi(GL_FOG_MODE, GL_EXP2); checkGLcall("glFogi(GL_FOG_MODE, GL_EXP2"); break;
          case D3DFOG_LINEAR:  glFogi(GL_FOG_MODE, GL_LINEAR); checkGLcall("glFogi(GL_FOG_MODE, GL_LINEAR"); break;
          default:
            FIXME("Unsupported Value(%lu) for WINED3DRS_FOGTABLEMODE!\n", Value);
          }
          if (GL_SUPPORT(NV_FOG_DISTANCE)) {
            glFogi(GL_FOG_DISTANCE_MODE_NV, This->stateBlock->renderState[WINED3DRS_RANGEFOGENABLE] ? GL_EYE_RADIAL_NV : GL_EYE_PLANE_ABSOLUTE_NV);
          }
        }
        break;

    case WINED3DRS_FOGSTART                  :
        {
            tmpvalue.d = Value;
            glFogfv(GL_FOG_START, &tmpvalue.f);
            checkGLcall("glFogf(GL_FOG_START, (float) Value)");
            TRACE("Fog Start == %f\n", tmpvalue.f);
        }
        break;

    case WINED3DRS_FOGEND                    :
        {
            tmpvalue.d = Value;
            glFogfv(GL_FOG_END, &tmpvalue.f);
            checkGLcall("glFogf(GL_FOG_END, (float) Value)");
            TRACE("Fog End == %f\n", tmpvalue.f);
        }
        break;

    case WINED3DRS_FOGDENSITY                :
        {
            tmpvalue.d = Value;
            glFogfv(GL_FOG_DENSITY, &tmpvalue.f);
            checkGLcall("glFogf(GL_FOG_DENSITY, (float) Value)");
        }
        break;

    case WINED3DRS_VERTEXBLEND               :
        {
          This->updateStateBlock->vertex_blend = (D3DVERTEXBLENDFLAGS) Value;
          TRACE("Vertex Blending state to %ld\n",  Value);
        }
        break;

    case WINED3DRS_TWEENFACTOR               :
        {
          tmpvalue.d = Value;
          This->updateStateBlock->tween_factor = tmpvalue.f;
          TRACE("Vertex Blending Tween Factor to %f\n", This->updateStateBlock->tween_factor);
        }
        break;

    case WINED3DRS_INDEXEDVERTEXBLENDENABLE  :
        {
          TRACE("Indexed Vertex Blend Enable to %ul\n", (BOOL) Value);
        }
        break;

    case WINED3DRS_COLORVERTEX               :
    case WINED3DRS_DIFFUSEMATERIALSOURCE     :
    case WINED3DRS_SPECULARMATERIALSOURCE    :
    case WINED3DRS_AMBIENTMATERIALSOURCE     :
    case WINED3DRS_EMISSIVEMATERIALSOURCE    :
        {
            GLenum Parm = GL_AMBIENT_AND_DIFFUSE;

            if (This->stateBlock->renderState[WINED3DRS_COLORVERTEX]) {
                TRACE("diff %ld, amb %ld, emis %ld, spec %ld\n",
                      This->stateBlock->renderState[WINED3DRS_DIFFUSEMATERIALSOURCE],
                      This->stateBlock->renderState[WINED3DRS_AMBIENTMATERIALSOURCE],
                      This->stateBlock->renderState[WINED3DRS_EMISSIVEMATERIALSOURCE],
                      This->stateBlock->renderState[WINED3DRS_SPECULARMATERIALSOURCE]);

                if (This->stateBlock->renderState[WINED3DRS_DIFFUSEMATERIALSOURCE] == D3DMCS_COLOR1) {
                    if (This->stateBlock->renderState[WINED3DRS_AMBIENTMATERIALSOURCE] == D3DMCS_COLOR1) {
                        Parm = GL_AMBIENT_AND_DIFFUSE;
                    } else {
                        Parm = GL_DIFFUSE;
                    }
                } else if (This->stateBlock->renderState[WINED3DRS_AMBIENTMATERIALSOURCE] == D3DMCS_COLOR1) {
                    Parm = GL_AMBIENT;
                } else if (This->stateBlock->renderState[WINED3DRS_EMISSIVEMATERIALSOURCE] == D3DMCS_COLOR1) {
                    Parm = GL_EMISSION;
                } else if (This->stateBlock->renderState[WINED3DRS_SPECULARMATERIALSOURCE] == D3DMCS_COLOR1) {
                    Parm = GL_SPECULAR;
                } else {
                    Parm = -1;
                }

                if (Parm == -1) {
                    if (This->tracking_color != DISABLED_TRACKING) This->tracking_color = NEEDS_DISABLE;
                } else {
                    This->tracking_color = NEEDS_TRACKING;
                    This->tracking_parm  = Parm;
                }

            } else {
                if (This->tracking_color != DISABLED_TRACKING) This->tracking_color = NEEDS_DISABLE;
            }
        }
        break;

    case WINED3DRS_LINEPATTERN               :
        {
            union {
                DWORD                 d;
                D3DLINEPATTERN        lp;
            } tmppattern;
            tmppattern.d = Value;

            TRACE("Line pattern: repeat %d bits %x\n", tmppattern.lp.wRepeatFactor, tmppattern.lp.wLinePattern);

            if (tmppattern.lp.wRepeatFactor) {
                glLineStipple(tmppattern.lp.wRepeatFactor, tmppattern.lp.wLinePattern);
                checkGLcall("glLineStipple(repeat, linepattern)");
                glEnable(GL_LINE_STIPPLE);
                checkGLcall("glEnable(GL_LINE_STIPPLE);");
            } else {
                glDisable(GL_LINE_STIPPLE);
                checkGLcall("glDisable(GL_LINE_STIPPLE);");
            }
        }
        break;

    case WINED3DRS_ZBIAS                     :
        {
            if (Value) {
                tmpvalue.d = Value;
                TRACE("ZBias value %f\n", tmpvalue.f);
                glPolygonOffset(0, -tmpvalue.f);
                checkGLcall("glPolygonOffset(0, -Value)");
                glEnable(GL_POLYGON_OFFSET_FILL);
                checkGLcall("glEnable(GL_POLYGON_OFFSET_FILL);");
                glEnable(GL_POLYGON_OFFSET_LINE);
                checkGLcall("glEnable(GL_POLYGON_OFFSET_LINE);");
                glEnable(GL_POLYGON_OFFSET_POINT);
                checkGLcall("glEnable(GL_POLYGON_OFFSET_POINT);");
            } else {
                glDisable(GL_POLYGON_OFFSET_FILL);
                checkGLcall("glDisable(GL_POLYGON_OFFSET_FILL);");
                glDisable(GL_POLYGON_OFFSET_LINE);
                checkGLcall("glDisable(GL_POLYGON_OFFSET_LINE);");
                glDisable(GL_POLYGON_OFFSET_POINT);
                checkGLcall("glDisable(GL_POLYGON_OFFSET_POINT);");
            }
        }
        break;

    case WINED3DRS_NORMALIZENORMALS          :
        if (Value) {
            glEnable(GL_NORMALIZE);
            checkGLcall("glEnable(GL_NORMALIZE);");
        } else {
            glDisable(GL_NORMALIZE);
            checkGLcall("glDisable(GL_NORMALIZE);");
        }
        break;

    case WINED3DRS_POINTSIZE                 :
        tmpvalue.d = Value;
        TRACE("Set point size to %f\n", tmpvalue.f);
        glPointSize(tmpvalue.f);
        checkGLcall("glPointSize(...);");
        break;

    case WINED3DRS_POINTSIZE_MIN             :
        if (GL_SUPPORT(EXT_POINT_PARAMETERS)) {
          tmpvalue.d = Value;
          GL_EXTCALL(glPointParameterfEXT)(GL_POINT_SIZE_MIN_EXT, tmpvalue.f);
          checkGLcall("glPointParameterfEXT(...);");
        } else {
          FIXME("WINED3DRS_POINTSIZE_MIN not supported on this opengl\n");
        }
        break;

    case WINED3DRS_POINTSIZE_MAX             :
        if (GL_SUPPORT(EXT_POINT_PARAMETERS)) {
          tmpvalue.d = Value;
          GL_EXTCALL(glPointParameterfEXT)(GL_POINT_SIZE_MAX_EXT, tmpvalue.f);
          checkGLcall("glPointParameterfEXT(...);");
        } else {
          FIXME("WINED3DRS_POINTSIZE_MAX not supported on this opengl\n");
        }
        break;

    case WINED3DRS_POINTSCALE_A              :
    case WINED3DRS_POINTSCALE_B              :
    case WINED3DRS_POINTSCALE_C              :
    case WINED3DRS_POINTSCALEENABLE          :
        {
            /* If enabled, supply the parameters, otherwise fall back to defaults */
            if (This->stateBlock->renderState[WINED3DRS_POINTSCALEENABLE]) {
                GLfloat att[3] = {1.0f, 0.0f, 0.0f};
                att[0] = *((float*)&This->stateBlock->renderState[WINED3DRS_POINTSCALE_A]);
                att[1] = *((float*)&This->stateBlock->renderState[WINED3DRS_POINTSCALE_B]);
                att[2] = *((float*)&This->stateBlock->renderState[WINED3DRS_POINTSCALE_C]);

                if (GL_SUPPORT(EXT_POINT_PARAMETERS)) {
                  GL_EXTCALL(glPointParameterfvEXT)(GL_DISTANCE_ATTENUATION_EXT, att);
                  checkGLcall("glPointParameterfvEXT(GL_DISTANCE_ATTENUATION_EXT, ...);");
                } else {
                  TRACE("WINED3DRS_POINTSCALEENABLE not supported on this opengl\n");
                }
            } else {
                GLfloat att[3] = {1.0f, 0.0f, 0.0f};
                if (GL_SUPPORT(EXT_POINT_PARAMETERS)) {
                  GL_EXTCALL(glPointParameterfvEXT)(GL_DISTANCE_ATTENUATION_EXT, att);
                  checkGLcall("glPointParameterfvEXT(GL_DISTANCE_ATTENUATION_EXT, ...);");
                } else {
                  TRACE("WINED3DRS_POINTSCALEENABLE not supported, but not on either\n");
                }
            }
            break;
        }

    case WINED3DRS_COLORWRITEENABLE          :
      {
        TRACE("Color mask: r(%d) g(%d) b(%d) a(%d)\n",
              Value & D3DCOLORWRITEENABLE_RED   ? 1 : 0,
              Value & D3DCOLORWRITEENABLE_GREEN ? 1 : 0,
              Value & D3DCOLORWRITEENABLE_BLUE  ? 1 : 0,
              Value & D3DCOLORWRITEENABLE_ALPHA ? 1 : 0);
        glColorMask(Value & D3DCOLORWRITEENABLE_RED   ? GL_TRUE : GL_FALSE,
                    Value & D3DCOLORWRITEENABLE_GREEN ? GL_TRUE : GL_FALSE,
                    Value & D3DCOLORWRITEENABLE_BLUE  ? GL_TRUE : GL_FALSE,
                    Value & D3DCOLORWRITEENABLE_ALPHA ? GL_TRUE : GL_FALSE);
        checkGLcall("glColorMask(...)");
      }
      break;

    case WINED3DRS_LOCALVIEWER               :
      {
        GLint state = (Value) ? 1 : 0;
        TRACE("Local Viewer Enable to %ul\n", (BOOL) Value);
        glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, state);
      }
      break;

    case WINED3DRS_LASTPIXEL                 :
      {
        if (Value) {
          TRACE("Last Pixel Drawing Enabled\n");
        } else {
          FIXME("Last Pixel Drawing Disabled, not handled yet\n");
        }
      }
      break;

    case WINED3DRS_SOFTWAREVERTEXPROCESSING  :
      {
        if (Value) {
          TRACE("Software Processing Enabled\n");
        } else {
          TRACE("Software Processing Disabled\n");
        }
      }
      break;

      /** not supported */
    case WINED3DRS_ZVISIBLE                  :
      {
        LEAVE_GL();
        return D3DERR_INVALIDCALL;
      }

        /* Unhandled yet...! */
    case WINED3DRS_EDGEANTIALIAS             :
    case WINED3DRS_WRAP0                     :
    case WINED3DRS_WRAP1                     :
    case WINED3DRS_WRAP2                     :
    case WINED3DRS_WRAP3                     :
    case WINED3DRS_WRAP4                     :
    case WINED3DRS_WRAP5                     :
    case WINED3DRS_WRAP6                     :
    case WINED3DRS_WRAP7                     :
    case WINED3DRS_POINTSPRITEENABLE         :
    case WINED3DRS_MULTISAMPLEANTIALIAS      :
    case WINED3DRS_MULTISAMPLEMASK           :
    case WINED3DRS_PATCHEDGESTYLE            :
    case WINED3DRS_PATCHSEGMENTS             :
    case WINED3DRS_DEBUGMONITORTOKEN         :
    case WINED3DRS_POSITIONORDER             :
    case WINED3DRS_NORMALORDER               :
        /*Put back later: FIXME("(%p)->(%d,%ld) not handled yet\n", This, State, Value); */
        FIXME("(%p)->(%d,%ld) not handled yet\n", This, State, Value);
        break;
    default:
        FIXME("(%p)->(%d,%ld) unrecognized\n", This, State, Value);
    }

    LEAVE_GL();

    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_GetRenderState(IWineD3DDevice *iface, D3DRENDERSTATETYPE State, DWORD *pValue) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) for State %d = %ld\n", This, State, This->stateBlock->renderState[State]);
    *pValue = This->stateBlock->renderState[State];
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_SetSamplerState(IWineD3DDevice *iface, DWORD Sampler, WINED3DSAMPLERSTATETYPE Type, DWORD Value) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    /**
    * SetSampler is designed to allow for more than the standard up to 8 textures
    *  and Geforce has stopped supporting more than 6 standard textures in openGL.
    * So I have to use ARB for Gforce. (maybe if the sampler > 4 then use ARB?)
    *
    * http://developer.nvidia.com/object/General_FAQ.html#t6
    *
    * There are two new settings for GForce
    * the sampler one:
    * GL_MAX_TEXTURE_IMAGE_UNITS_ARB
    * and the texture one:
    * GL_MAX_TEXTURE_COORDS_ARB.
    * Ok GForce say it's ok to use glTexParameter/glGetTexParameter(...).
     ******************/
    union {
        float f;
        DWORD d;
    } tmpvalue;

    TRACE("(%p) Sampler(%ld), Type(%d) Value(%ld)\n",This, Sampler ,Type, Value);

    if(Sampler >  GL_LIMITS(samplers) || Sampler < 0 || Type > HIGHEST_SAMPLER_STATE || Type < 0) {
        FIXME("out of range %d %d sampler %ld type %u\n", GL_LIMITS(samplers), HIGHEST_SAMPLER_STATE, Sampler, Type);
        return D3DERR_INVALIDCALL;
    }

    This->updateStateBlock->changed.samplerState[Sampler][Type] = TRUE;
    This->updateStateBlock->set.samplerState[Sampler][Type]     = TRUE;
    TRACE("Setting sampler %ld %d to %ld \n", Sampler, Type, Value);
    This->updateStateBlock->samplerState[Sampler][Type]         = Value;

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return D3D_OK;
    }

    /* In addition, IDirect3DDevice9::SetSamplerState will now be used for filtering, tiling,
    clamping, MIPLOD, etc. This will work for up to 16 samplers.
    is this just GL_TEXTURE_2D or is it GL_TEXTURE_1D and GL_TEXTURE_3D as well?
    */
    ENTER_GL();
    VTRACE(("Activating appropriate texture state %ld\n", Sampler));
    if (GL_SUPPORT(ARB_MULTITEXTURE)) {
        GLACTIVETEXTURE(Sampler);
    } else if (Sampler > 0) {
        FIXME("Program using multiple concurrent textures which this opengl implementation doesn't support\n");
    }


    switch (Type) {

    case WINED3DSAMP_ADDRESSU              : /* 1 */
    case WINED3DSAMP_ADDRESSV              : /* 2 */
    case WINED3DSAMP_ADDRESSW              : /* 3 */
        {
            GLint wrapParm = GL_REPEAT;
            switch (Value) {
            case D3DTADDRESS_WRAP:   wrapParm = GL_REPEAT; break;
            case D3DTADDRESS_CLAMP:  wrapParm = GL_CLAMP_TO_EDGE; break;
            case D3DTADDRESS_BORDER:
              {
                if (GL_SUPPORT(ARB_TEXTURE_BORDER_CLAMP)) {
                  wrapParm = GL_CLAMP_TO_BORDER_ARB;
                } else {
                  /* FIXME: Not right, but better */
                  TRACE("Unrecognized or unsupported D3DTADDRESS_* value %ld, state %d\n", Value, Type);
                  wrapParm = GL_REPEAT;
                }
              }
              break;
            case D3DTADDRESS_MIRROR:
              {
                if (GL_SUPPORT(ARB_TEXTURE_MIRRORED_REPEAT)) {
                  wrapParm = GL_MIRRORED_REPEAT_ARB;
                } else {
                  /* Unsupported in OpenGL pre-1.4 */
                  TRACE("Unsupported D3DTADDRESS_MIRROR (needs GL_ARB_texture_mirrored_repeat) state %d\n", Type);
                  wrapParm = GL_REPEAT;
                }
              }
              break;
            case D3DTADDRESS_MIRRORONCE:
              {
                if (GL_SUPPORT(ATI_TEXTURE_MIRROR_ONCE)) {
                  wrapParm = GL_MIRROR_CLAMP_TO_EDGE_ATI;
                } else {
                  TRACE("Unsupported D3DTADDRESS_MIRRORONCE (needs GL_ATI_texture_mirror_once) state %d\n", Type);
                  wrapParm = GL_REPEAT;
                }
              }
              break;

            default:
            /* This is for the whole context, not just the sampler,
            so we should warn if two states are baing set for any given scene */
            if (Type!=0)
                TRACE("Unrecognized or unsupported D3DTADDRESS_* value %ld, state %d\n", Value, Type);
                wrapParm = GL_REPEAT;
            }
            switch (Type) {
            case WINED3DSAMP_ADDRESSU:
                TRACE("Setting WRAP_S for %ld to %d \n", Sampler, wrapParm);
                glTexParameteri(This->stateBlock->textureDimensions[Sampler], GL_TEXTURE_WRAP_S, wrapParm);
                checkGLcall("glTexParameteri(..., GL_TEXTURE_WRAP_S, wrapParm)");
                break;
            case WINED3DSAMP_ADDRESSV:
                TRACE("Setting WRAP_T for %ld to %d\n", Sampler, wrapParm);
                glTexParameteri(This->stateBlock->textureDimensions[Sampler], GL_TEXTURE_WRAP_T, wrapParm);
                checkGLcall("glTexParameteri(..., GL_TEXTURE_WRAP_T, wrapParm)");
                break;

            case WINED3DSAMP_ADDRESSW:
                TRACE("Setting WRAP_R for %ld to %d\n", Sampler, wrapParm);
                glTexParameteri(This->stateBlock->textureDimensions[Sampler], GL_TEXTURE_WRAP_R, wrapParm);
                checkGLcall("glTexParameteri(..., GL_TEXTURE_WRAP_R, wrapParm)");
                break;
            default: /* nop */
                      break; /** stupic compilator */
            }
        }
        break;

    case WINED3DSAMP_BORDERCOLOR           : /* 4 */
        {
            float col[4];
            D3DCOLORTOGLFLOAT4(Value, col);
            TRACE("Setting border color for %ld to %lx\n", Sampler, Value);
            glTexParameterfv(This->stateBlock->textureDimensions[Sampler], GL_TEXTURE_BORDER_COLOR, &col[0]);
            checkGLcall("glTexParameteri(..., GL_TEXTURE_BORDER_COLOR, ...)");
        }
        break;

      case WINED3DSAMP_MAGFILTER             : /* 5 */
      {
        DWORD ValueMAG = This->stateBlock->samplerState[Sampler][WINED3DSAMP_MAGFILTER];
        GLint realVal = GL_NEAREST;

        if (ValueMAG == D3DTEXF_POINT) {
          realVal = GL_NEAREST;
        } else if (ValueMAG == D3DTEXF_LINEAR) {
          realVal = GL_LINEAR;
        } else if (ValueMAG == D3DTEXF_ANISOTROPIC) {
          if (GL_SUPPORT(EXT_TEXTURE_FILTER_ANISOTROPIC)) {
            realVal = GL_LINEAR;
          } else {
            FIXME("Trying to use ANISOTROPIC_FILTERING for WINED3DTSS_MAGFILTER. But not supported by current OpenGL driver\n");
            realVal = GL_NEAREST;
          }
        } else {
          FIXME("Unhandled WINED3DTSS_MAGFILTER value of %ld\n", ValueMAG);
          realVal = GL_NEAREST;
        }
        TRACE("ValueMAG=%ld setting MAGFILTER to %x\n", ValueMAG, realVal);
        glTexParameteri(This->stateBlock->textureDimensions[Sampler], GL_TEXTURE_MAG_FILTER, realVal);
        checkGLcall("glTexParameter GL_TEXTURE_MAG_FILTER, ...");
        /**
         * if we juste choose to use ANISOTROPIC filtering, refresh openGL state
         */
        if (GL_SUPPORT(EXT_TEXTURE_FILTER_ANISOTROPIC) && D3DTEXF_ANISOTROPIC == ValueMAG) {
          glTexParameteri(This->stateBlock->textureDimensions[Sampler],
                          GL_TEXTURE_MAX_ANISOTROPY_EXT,
                          This->stateBlock->samplerState[Sampler][WINED3DSAMP_MAXANISOTROPY]);
          checkGLcall("glTexParameter GL_TEXTURE_MAX_ANISOTROPY_EXT, ...");
        }
      }
      break;

       case WINED3DSAMP_MINFILTER: /* 6 */
       case WINED3DSAMP_MIPFILTER: /* 7 */
       {
            DWORD ValueMIN = This->stateBlock->samplerState[Sampler][WINED3DSAMP_MINFILTER];
            DWORD ValueMIP = This->stateBlock->samplerState[Sampler][WINED3DSAMP_MIPFILTER];
            GLint realVal = GL_LINEAR;

            if (ValueMIN == D3DTEXF_NONE) {
              /* Doesn't really make sense - Windows just seems to disable
                 mipmapping when this occurs                              */
              FIXME("Odd - minfilter of none, just disabling mipmaps\n");
              realVal = GL_LINEAR;
            } else if (ValueMIN == D3DTEXF_POINT) {
                /* GL_NEAREST_* */
              if (ValueMIP == D3DTEXF_NONE) {
                    realVal = GL_NEAREST;
                } else if (ValueMIP == D3DTEXF_POINT) {
                    realVal = GL_NEAREST_MIPMAP_NEAREST;
                } else if (ValueMIP == D3DTEXF_LINEAR) {
                    realVal = GL_NEAREST_MIPMAP_LINEAR;
                } else {
                    FIXME("Unhandled WINED3DTSS_MIPFILTER value of %ld\n", ValueMIP);
                    realVal = GL_NEAREST;
                }
            } else if (ValueMIN == D3DTEXF_LINEAR) {
                /* GL_LINEAR_* */
                if (ValueMIP == D3DTEXF_NONE) {
                    realVal = GL_LINEAR;
                } else if (ValueMIP == D3DTEXF_POINT) {
                    realVal = GL_LINEAR_MIPMAP_NEAREST;
                } else if (ValueMIP == D3DTEXF_LINEAR) {
                    realVal = GL_LINEAR_MIPMAP_LINEAR;
                } else {
                    FIXME("Unhandled WINED3DTSS_MIPFILTER value of %ld\n", ValueMIP);
                    realVal = GL_LINEAR;
                }
            } else if (ValueMIN == D3DTEXF_ANISOTROPIC) {
              if (GL_SUPPORT(EXT_TEXTURE_FILTER_ANISOTROPIC)) {
                if (ValueMIP == D3DTEXF_NONE) {
                  realVal = GL_LINEAR_MIPMAP_LINEAR;
                } else if (ValueMIP == D3DTEXF_POINT) {
                  realVal = GL_LINEAR_MIPMAP_NEAREST;
                } else if (ValueMIP == D3DTEXF_LINEAR) {
                    realVal = GL_LINEAR_MIPMAP_LINEAR;
                } else {
                  FIXME("Unhandled WINED3DTSS_MIPFILTER value of %ld\n", ValueMIP);
                  realVal = GL_LINEAR;
                }
              } else {
                WARN("Trying to use ANISOTROPIC_FILTERING for WINED3DTSS_MINFILTER. But not supported by OpenGL driver\n");
                realVal = GL_LINEAR;
              }
            } else {
                FIXME("Unhandled WINED3DTSS_MINFILTER value of %ld\n", ValueMIN);
                realVal = GL_LINEAR_MIPMAP_LINEAR;
            }

            TRACE("ValueMIN=%ld, ValueMIP=%ld, setting MINFILTER to %x\n", ValueMIN, ValueMIP, realVal);
            glTexParameteri(This->stateBlock->textureDimensions[Sampler], GL_TEXTURE_MIN_FILTER, realVal);
            checkGLcall("glTexParameter GL_TEXTURE_MIN_FILTER, ...");
            /**
             * if we just choose to use ANISOTROPIC filtering, refresh openGL state
             */
            if (GL_SUPPORT(EXT_TEXTURE_FILTER_ANISOTROPIC) && D3DTEXF_ANISOTROPIC == ValueMIN) {
              glTexParameteri(This->stateBlock->textureDimensions[Sampler],  GL_TEXTURE_MAX_ANISOTROPY_EXT,
               This->stateBlock->samplerState[Sampler][WINED3DSAMP_MAXANISOTROPY]);
              checkGLcall("glTexParameter GL_TEXTURE_MAX_ANISOTROPY_EXT, ...");
            }
        }
        break;

    case WINED3DSAMP_MIPMAPLODBIAS         : /* 8 */
      {
        if (GL_SUPPORT(EXT_TEXTURE_LOD_BIAS)) {
          tmpvalue.d = Value;
          glTexEnvf(GL_TEXTURE_FILTER_CONTROL_EXT,
                    GL_TEXTURE_LOD_BIAS_EXT,
                    tmpvalue.f);
          checkGLcall("glTexEnvi GL_TEXTURE_LOD_BIAS_EXT ...");
        }
      }
      break;

    case WINED3DSAMP_MAXMIPLEVEL           : /* 9 */
      {
        /**
         * Not really the same, but the more apprioprate than nothing
         */
        glTexParameteri(This->stateBlock->textureDimensions[Sampler],
                        GL_TEXTURE_BASE_LEVEL,
                        This->stateBlock->samplerState[Sampler][WINED3DSAMP_MAXMIPLEVEL]);
        checkGLcall("glTexParameteri GL_TEXTURE_BASE_LEVEL ...");
      }
      break;

    case WINED3DSAMP_MAXANISOTROPY         :  /* 10 */
      {
        if (GL_SUPPORT(EXT_TEXTURE_FILTER_ANISOTROPIC)) {
          glTexParameteri(This->stateBlock->textureDimensions[Sampler],
                          GL_TEXTURE_MAX_ANISOTROPY_EXT,
                          This->stateBlock->samplerState[Sampler][WINED3DSAMP_MAXANISOTROPY]);
          checkGLcall("glTexParameteri GL_TEXTURE_MAX_ANISOTROPY_EXT ...");
        }
      }
      break;
    case WINED3DSAMP_SRGBTEXTURE            : /* 11 */
   /* Per texture gamma correction, default 0 */
    case WINED3DSAMP_ELEMENTINDEX           : /* 12 */
   /* When a multi-element textures is used this indicates the element, (what's a multielement texture?) */
    case WINED3DSAMP_DMAPOFFSET             : /* 13 */
   /* Value of a precompiled displacement map used by the tesselator, default 0 */
        TRACE("Unsupported sampler setting, Sampler=%ld, Type=%d, Value =%ld\n", Sampler, Type, Value);
    break;
    default:

        TRACE("invalid sampler setting, Sampler=%ld, Type=%d, Value =%ld\n", Sampler, Type, Value);
    };
    LEAVE_GL();
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_GetSamplerState(IWineD3DDevice *iface, DWORD Sampler, WINED3DSAMPLERSTATETYPE Type, DWORD* Value) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    /** TODO: check that sampler is in  range **/
    *Value = This->updateStateBlock->samplerState[Sampler][Type];
    TRACE("(%p) : Sampler %ld Type %u Returning %ld\n", This, Sampler, Type, *Value);
    return D3D_OK;

}

HRESULT WINAPI IWineD3DDeviceImpl_SetScissorRect(IWineD3DDevice *iface, CONST RECT* pRect) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    ENTER_GL();

    /** FIXME: Windows uses a top,left origin openGL uses a bottom Right? **/
    TRACE("(%p)Setting new Scissor Rect to %ld:%ld-%ld:%ld\n", This, pRect->left, pRect->top, pRect->right, pRect->bottom);
    glScissor(pRect->left, pRect->top, pRect->right - pRect->left, pRect->bottom - pRect->top);
    LEAVE_GL();
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_GetScissorRect(IWineD3DDevice *iface, RECT* pRect) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    GLint scissorBox[4];

    ENTER_GL();
    /** FIXME: Windows uses a top,left origin openGL uses a bottom Right? **/
    glGetIntegerv(GL_SCISSOR_BOX, scissorBox);
    pRect->left = scissorBox[1];
    pRect->top = scissorBox[2];
    pRect->right = scissorBox[1] + scissorBox[3];
    pRect->bottom = scissorBox[2] + scissorBox[4];
    TRACE("(%p)Returning a Scissor Rect of %ld:%ld-%ld:%ld\n", This, pRect->left, pRect->top, pRect->right, pRect->bottom);
    LEAVE_GL();
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_SetVertexDeclaration(IWineD3DDevice* iface, IWineD3DVertexDeclaration* pDecl) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;

    TRACE("(%p) : pDecl=%p\n", This, pDecl);

    /* TODO: what about recording stateblocks? */
    if (NULL != pDecl) {
        IWineD3DVertexDeclaration_AddRef(pDecl);
    }
    if (NULL != This->updateStateBlock->vertexDecl) {
      IWineD3DVertexDeclaration_Release(This->updateStateBlock->vertexDecl);
    }
    This->updateStateBlock->vertexDecl = pDecl;
    This->updateStateBlock->changed.vertexDecl = TRUE;
    This->updateStateBlock->set.vertexDecl = TRUE;
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_GetVertexDeclaration(IWineD3DDevice* iface, IWineD3DVertexDeclaration** ppDecl) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("(%p) : ppDecl=%p\n", This, ppDecl);

    *ppDecl = This->updateStateBlock->vertexDecl;
    if (NULL != *ppDecl) IWineD3DVertexDeclaration_AddRef(*ppDecl);
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_SetVertexShader(IWineD3DDevice *iface, IWineD3DVertexShader* pShader) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    static BOOL showFixmes = TRUE;

    This->updateStateBlock->vertexShader = pShader;
    This->updateStateBlock->changed.vertexShader = TRUE;
    This->updateStateBlock->set.vertexShader = TRUE;

    if(pShader == NULL) {
    /* clear down the shader */
        TRACE("Clear down the shader\n");
    }else{
        if(showFixmes) {
            FIXME("(%p) : stub pShader(%p)\n", This, pShader);
            showFixmes = FALSE;
        }
    }

    return D3D_OK;

    /** FIXME: refernece counting? **/
    if (pShader  == NULL) { /* only valid with non FVF shaders */
      TRACE_(d3d_shader)("(%p) : FVF Shader, pShader=%p\n", This, pShader);
      This->updateStateBlock->vertexShader = NULL;
    } else {
       TRACE_(d3d_shader)("(%p) : Created shader, pShader=%p\n", This, pShader);
      This->updateStateBlock->vertexShader = pShader;
    }

    This->updateStateBlock->changed.vertexShader = TRUE;
    This->updateStateBlock->set.vertexShader = TRUE;

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
      TRACE("Recording... not performing anything\n");
      return D3D_OK;
    }
    /**
     * TODO: merge HAL shaders context switching from prototype
     */
    return D3D_OK;

}

HRESULT WINAPI IWineD3DDeviceImpl_GetVertexShader(IWineD3DDevice *iface, IWineD3DVertexShader** ppShader) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    *ppShader = This->stateBlock->vertexShader;
    if(*ppShader != NULL)
        IWineD3DVertexShader_AddRef(*ppShader);
    TRACE("(%p) : returning %p\n", This, *ppShader);
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_SetVertexShaderConstantB(IWineD3DDevice *iface, UINT StartRegister, CONST BOOL  *pConstantData, UINT BoolCount) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_GetVertexShaderConstantB(IWineD3DDevice *iface, UINT StartRegister, BOOL        *pConstantData, UINT BoolCount) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_SetVertexShaderConstantI(IWineD3DDevice *iface, UINT StartRegister, CONST int   *pConstantData, UINT Vector4iCount) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_GetVertexShaderConstantI(IWineD3DDevice *iface, UINT StartRegister, int         *pConstantData, UINT Vector4iCount) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_SetVertexShaderConstantF(IWineD3DDevice *iface, UINT StartRegister, CONST float *pConstantData, UINT Vector4fCount) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_GetVertexShaderConstantF(IWineD3DDevice *iface, UINT StartRegister, float       *pConstantData, UINT Vector4fCount) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_SetPixelShader(IWineD3DDevice *iface, IWineD3DPixelShader  *pShader) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_GetPixelShader(IWineD3DDevice *iface, IWineD3DPixelShader **ppShader) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : stub\n", This);
    return D3D_OK;
}


HRESULT WINAPI IWineD3DDeviceImpl_SetPixelShaderConstantB(IWineD3DDevice *iface, UINT StartRegister, CONST BOOL   *pConstantData, UINT BoolCount) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_GetPixelShaderConstantB(IWineD3DDevice *iface, UINT StartRegister, BOOL         *pConstantData, UINT BoolCount) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_SetPixelShaderConstantI(IWineD3DDevice *iface, UINT StartRegister, CONST int    *pConstantData, UINT Vector4iCount) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_GetPixelShaderConstantI(IWineD3DDevice *iface, UINT StartRegister, int          *pConstantData, UINT Vector4iCount) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_SetPixelShaderConstantF(IWineD3DDevice *iface, UINT StartRegister, CONST float  *pConstantData, UINT Vector4fCount) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_GetPixelShaderConstantF(IWineD3DDevice *iface, UINT StartRegister, float        *pConstantData, UINT Vector4fCount) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_ProcessVertices(IWineD3DDevice *iface, UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IWineD3DVertexBuffer* pDestBuffer, IWineD3DVertexBuffer* pVertexDecl, DWORD Flags) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

/*****
 * Get / Set Texture Stage States
 * TODO: Verify against dx9 definitions
 *****/
HRESULT WINAPI IWineD3DDeviceImpl_SetTextureStageState(IWineD3DDevice *iface, DWORD Stage, WINED3DTEXTURESTAGESTATETYPE Type, DWORD Value) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    /* FIXME: Handle 3d textures? What if TSS value set before set texture? Need to reapply all values? */

    TRACE("(%p) : Stage=%ld, Type=%s(%d), Value=%ld\n", This, Stage, debug_d3dtexturestate(Type), Type, Value);

    /* Reject invalid texture units */
    if (Stage >= GL_LIMITS(textures)) {
        TRACE("Attempt to access invalid texture rejected\n");
        return D3DERR_INVALIDCALL;
    }

    This->updateStateBlock->changed.textureState[Stage][Type] = TRUE;
    This->updateStateBlock->set.textureState[Stage][Type] = TRUE;
    This->updateStateBlock->textureState[Stage][Type] = Value;

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return D3D_OK;
    }

    ENTER_GL();

    /* Make appropriate texture active */
    VTRACE(("Activating appropriate texture state %ld\n", Stage));
    if (GL_SUPPORT(ARB_MULTITEXTURE)) {
        GLACTIVETEXTURE(Stage);
    } else if (Stage > 0) {
        FIXME("Program using multiple concurrent textures which this opengl implementation doesn't support\n");
    }

    switch (Type) {
    case WINED3DTSS_ALPHAOP               :
    case WINED3DTSS_COLOROP               :
        {

            if ((Value == D3DTOP_DISABLE) && (Type == WINED3DTSS_COLOROP)) {
                /* TODO: Disable by making this and all later levels disabled */
                glDisable(GL_TEXTURE_1D);
                checkGLcall("Disable GL_TEXTURE_1D");
                glDisable(GL_TEXTURE_2D);
                checkGLcall("Disable GL_TEXTURE_2D");
                glDisable(GL_TEXTURE_3D);
                checkGLcall("Disable GL_TEXTURE_3D");
                break; /* Don't bother setting the texture operations */
            } else {
                /* Enable only the appropriate texture dimension */
                if (Type == WINED3DTSS_COLOROP) {
                    if (This->stateBlock->textureDimensions[Stage] == GL_TEXTURE_1D) {
                        glEnable(GL_TEXTURE_1D);
                        checkGLcall("Enable GL_TEXTURE_1D");
                    } else {
                        glDisable(GL_TEXTURE_1D);
                        checkGLcall("Disable GL_TEXTURE_1D");
                    }
                    if (This->stateBlock->textureDimensions[Stage] == GL_TEXTURE_2D) {
                      if (GL_SUPPORT(NV_TEXTURE_SHADER) && This->texture_shader_active) {
                        glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, GL_TEXTURE_2D);
                        checkGLcall("Enable GL_TEXTURE_2D");
                      } else {
                        glEnable(GL_TEXTURE_2D);
                        checkGLcall("Enable GL_TEXTURE_2D");
                      }
                    } else {
                        glDisable(GL_TEXTURE_2D);
                        checkGLcall("Disable GL_TEXTURE_2D");
                    }
                    if (This->stateBlock->textureDimensions[Stage] == GL_TEXTURE_3D) {
                        glEnable(GL_TEXTURE_3D);
                        checkGLcall("Enable GL_TEXTURE_3D");
                    } else {
                        glDisable(GL_TEXTURE_3D);
                        checkGLcall("Disable GL_TEXTURE_3D");
                    }
                    if (This->stateBlock->textureDimensions[Stage] == GL_TEXTURE_CUBE_MAP_ARB) {
                        glEnable(GL_TEXTURE_CUBE_MAP_ARB);
                        checkGLcall("Enable GL_TEXTURE_CUBE_MAP");
                    } else {
                        glDisable(GL_TEXTURE_CUBE_MAP_ARB);
                        checkGLcall("Disable GL_TEXTURE_CUBE_MAP");
                    }
                }
            }
            /* Drop through... (Except disable case) */
        case WINED3DTSS_COLORARG0             :
        case WINED3DTSS_COLORARG1             :
        case WINED3DTSS_COLORARG2             :
        case WINED3DTSS_ALPHAARG0             :
        case WINED3DTSS_ALPHAARG1             :
        case WINED3DTSS_ALPHAARG2             :
            {
                BOOL isAlphaArg = (Type == WINED3DTSS_ALPHAOP || Type == WINED3DTSS_ALPHAARG1 ||
                                   Type == WINED3DTSS_ALPHAARG2 || Type == WINED3DTSS_ALPHAARG0);
                if (isAlphaArg) {
                    set_tex_op(iface, TRUE, Stage, This->stateBlock->textureState[Stage][WINED3DTSS_ALPHAOP],
                               This->stateBlock->textureState[Stage][WINED3DTSS_ALPHAARG1],
                               This->stateBlock->textureState[Stage][WINED3DTSS_ALPHAARG2],
                               This->stateBlock->textureState[Stage][WINED3DTSS_ALPHAARG0]);
                } else {
                    set_tex_op(iface, FALSE, Stage, This->stateBlock->textureState[Stage][WINED3DTSS_COLOROP],
                               This->stateBlock->textureState[Stage][WINED3DTSS_COLORARG1],
                               This->stateBlock->textureState[Stage][WINED3DTSS_COLORARG2],
                               This->stateBlock->textureState[Stage][WINED3DTSS_COLORARG0]);
                }
            }
            break;
        }

    case WINED3DTSS_ADDRESSW              :
        {
            GLint wrapParm = GL_REPEAT;

            switch (Value) {
            case D3DTADDRESS_WRAP:   wrapParm = GL_REPEAT; break;
            case D3DTADDRESS_CLAMP:  wrapParm = GL_CLAMP_TO_EDGE; break;
            case D3DTADDRESS_BORDER:
              {
                if (GL_SUPPORT(ARB_TEXTURE_BORDER_CLAMP)) {
                  wrapParm = GL_CLAMP_TO_BORDER_ARB;
                } else {
                  /* FIXME: Not right, but better */
                  FIXME("Unrecognized or unsupported D3DTADDRESS_* value %ld, state %d\n", Value, Type);
                  wrapParm = GL_REPEAT;
                }
              }
              break;
            case D3DTADDRESS_MIRROR:
              {
                if (GL_SUPPORT(ARB_TEXTURE_MIRRORED_REPEAT)) {
                  wrapParm = GL_MIRRORED_REPEAT_ARB;
                } else {
                  /* Unsupported in OpenGL pre-1.4 */
                  FIXME("Unsupported D3DTADDRESS_MIRROR (needs GL_ARB_texture_mirrored_repeat) state %d\n", Type);
                  wrapParm = GL_REPEAT;
                }
              }
              break;
            case D3DTADDRESS_MIRRORONCE:
              {
                if (GL_SUPPORT(ATI_TEXTURE_MIRROR_ONCE)) {
                  wrapParm = GL_MIRROR_CLAMP_TO_EDGE_ATI;
                } else {
                  FIXME("Unsupported D3DTADDRESS_MIRRORONCE (needs GL_ATI_texture_mirror_once) state %d\n", Type);
                  wrapParm = GL_REPEAT;
                }
              }
              break;

            default:
                FIXME("Unrecognized or unsupported D3DTADDRESS_* value %ld, state %d\n", Value, Type);
                wrapParm = GL_REPEAT;
            }

            TRACE("Setting WRAP_R to %d for %x\n", wrapParm, This->stateBlock->textureDimensions[Stage]);
            glTexParameteri(This->stateBlock->textureDimensions[Stage], GL_TEXTURE_WRAP_R, wrapParm);
            checkGLcall("glTexParameteri(..., GL_TEXTURE_WRAP_R, wrapParm)");
        }
        break;

    case WINED3DTSS_TEXCOORDINDEX         :
        {
            /* Values 0-7 are indexes into the FVF tex coords - See comments in DrawPrimitive */

            /* FIXME: From MSDN: The WINED3DTSS_TCI_* flags are mutually exclusive. If you include
                  one flag, you can still specify an index value, which the system uses to
                  determine the texture wrapping mode.
                  eg. SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEPOSITION | 1 );
                  means use the vertex position (camera-space) as the input texture coordinates
                  for this texture stage, and the wrap mode set in the WINED3DRS_WRAP1 render
                  state. We do not (yet) support the D3DRENDERSTATE_WRAPx values, nor tie them up
                  to the TEXCOORDINDEX value */

            /**
             * Be careful the value of the mask 0xF0000 come from d3d8types.h infos
             */
            switch (Value & 0xFFFF0000) {
            case D3DTSS_TCI_PASSTHRU:
              /*Use the specified texture coordinates contained within the vertex format. This value resolves to zero.*/
              glDisable(GL_TEXTURE_GEN_S);
              glDisable(GL_TEXTURE_GEN_T);
              glDisable(GL_TEXTURE_GEN_R);
              checkGLcall("glDisable(GL_TEXTURE_GEN_S,T,R)");
              break;

            case D3DTSS_TCI_CAMERASPACEPOSITION:
              /* CameraSpacePosition means use the vertex position, transformed to camera space,
                 as the input texture coordinates for this stage's texture transformation. This
                 equates roughly to EYE_LINEAR                                                  */
              {
                float s_plane[] = { 1.0, 0.0, 0.0, 0.0 };
                float t_plane[] = { 0.0, 1.0, 0.0, 0.0 };
                float r_plane[] = { 0.0, 0.0, 1.0, 0.0 };
                float q_plane[] = { 0.0, 0.0, 0.0, 1.0 };
                TRACE("WINED3DTSS_TCI_CAMERASPACEPOSITION - Set eye plane\n");

                glMatrixMode(GL_MODELVIEW);
                glPushMatrix();
                glLoadIdentity();
                glTexGenfv(GL_S, GL_EYE_PLANE, s_plane);
                glTexGenfv(GL_T, GL_EYE_PLANE, t_plane);
                glTexGenfv(GL_R, GL_EYE_PLANE, r_plane);
                glTexGenfv(GL_Q, GL_EYE_PLANE, q_plane);
                glPopMatrix();

                TRACE("WINED3DTSS_TCI_CAMERASPACEPOSITION - Set GL_TEXTURE_GEN_x and GL_x, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR\n");
                glEnable(GL_TEXTURE_GEN_S);
                checkGLcall("glEnable(GL_TEXTURE_GEN_S);");
                glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
                checkGLcall("glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR)");
                glEnable(GL_TEXTURE_GEN_T);
                checkGLcall("glEnable(GL_TEXTURE_GEN_T);");
                glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
                checkGLcall("glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR)");
                glEnable(GL_TEXTURE_GEN_R);
                checkGLcall("glEnable(GL_TEXTURE_GEN_R);");
                glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
                checkGLcall("glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR)");
              }
              break;

            case D3DTSS_TCI_CAMERASPACENORMAL:
              {
                if (GL_SUPPORT(NV_TEXGEN_REFLECTION)) {
                  float s_plane[] = { 1.0, 0.0, 0.0, 0.0 };
                  float t_plane[] = { 0.0, 1.0, 0.0, 0.0 };
                  float r_plane[] = { 0.0, 0.0, 1.0, 0.0 };
                  float q_plane[] = { 0.0, 0.0, 0.0, 1.0 };
                  TRACE("WINED3DTSS_TCI_CAMERASPACEPOSITION - Set eye plane\n");

                  glMatrixMode(GL_MODELVIEW);
                  glPushMatrix();
                  glLoadIdentity();
                  glTexGenfv(GL_S, GL_EYE_PLANE, s_plane);
                  glTexGenfv(GL_T, GL_EYE_PLANE, t_plane);
                  glTexGenfv(GL_R, GL_EYE_PLANE, r_plane);
                  glTexGenfv(GL_Q, GL_EYE_PLANE, q_plane);
                  glPopMatrix();

                  glEnable(GL_TEXTURE_GEN_S);
                  checkGLcall("glEnable(GL_TEXTURE_GEN_S);");
                  glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_NV);
                  checkGLcall("glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_NV)");
                  glEnable(GL_TEXTURE_GEN_T);
                  checkGLcall("glEnable(GL_TEXTURE_GEN_T);");
                  glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_NV);
                  checkGLcall("glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_NV)");
                  glEnable(GL_TEXTURE_GEN_R);
                  checkGLcall("glEnable(GL_TEXTURE_GEN_R);");
                  glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_NV);
                  checkGLcall("glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_NV)");
                }
              }
              break;

            case D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR:
              {
                if (GL_SUPPORT(NV_TEXGEN_REFLECTION)) {
                  float s_plane[] = { 1.0, 0.0, 0.0, 0.0 };
                  float t_plane[] = { 0.0, 1.0, 0.0, 0.0 };
                  float r_plane[] = { 0.0, 0.0, 1.0, 0.0 };
                  float q_plane[] = { 0.0, 0.0, 0.0, 1.0 };
                  TRACE("WINED3DTSS_TCI_CAMERASPACEPOSITION - Set eye plane\n");

                  glMatrixMode(GL_MODELVIEW);
                  glPushMatrix();
                  glLoadIdentity();
                  glTexGenfv(GL_S, GL_EYE_PLANE, s_plane);
                  glTexGenfv(GL_T, GL_EYE_PLANE, t_plane);
                  glTexGenfv(GL_R, GL_EYE_PLANE, r_plane);
                  glTexGenfv(GL_Q, GL_EYE_PLANE, q_plane);
                  glPopMatrix();

                  glEnable(GL_TEXTURE_GEN_S);
                  checkGLcall("glEnable(GL_TEXTURE_GEN_S);");
                  glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_NV);
                  checkGLcall("glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_NV)");
                  glEnable(GL_TEXTURE_GEN_T);
                  checkGLcall("glEnable(GL_TEXTURE_GEN_T);");
                  glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_NV);
                  checkGLcall("glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_NV)");
                  glEnable(GL_TEXTURE_GEN_R);
                  checkGLcall("glEnable(GL_TEXTURE_GEN_R);");
                  glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_NV);
                  checkGLcall("glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_NV)");
                }
              }
              break;

            /* Unhandled types: */
            default:
                /* Todo: */
                /* ? disable GL_TEXTURE_GEN_n ? */
                glDisable(GL_TEXTURE_GEN_S);
                glDisable(GL_TEXTURE_GEN_T);
                glDisable(GL_TEXTURE_GEN_R);
                FIXME("Unhandled WINED3DTSS_TEXCOORDINDEX %lx\n", Value);
                break;
            }
        }
        break;

        /* Unhandled */
    case WINED3DTSS_TEXTURETRANSFORMFLAGS :
        set_texture_matrix((float *)&This->stateBlock->transforms[D3DTS_TEXTURE0 + Stage].u.m[0][0], Value);
        break;

    case WINED3DTSS_BUMPENVMAT00          :
    case WINED3DTSS_BUMPENVMAT01          :
        TRACE("BUMPENVMAT0%u Stage=%ld, Type=%d, Value =%ld\n", Type - WINED3DTSS_BUMPENVMAT00, Stage, Type, Value);
        break;
    case WINED3DTSS_BUMPENVMAT10          :
    case WINED3DTSS_BUMPENVMAT11          :
        TRACE("BUMPENVMAT1%u Stage=%ld, Type=%d, Value =%ld\n", Type - WINED3DTSS_BUMPENVMAT10, Stage, Type, Value);
        break;

    case WINED3DTSS_BUMPENVLSCALE         :
      TRACE("BUMPENVLSCALE Stage=%ld, Type=%d, Value =%ld\n", Stage, Type, Value);
      break;

    case WINED3DTSS_BUMPENVLOFFSET        :
      TRACE("BUMPENVLOFFSET Stage=%ld, Type=%d, Value =%ld\n", Stage, Type, Value);
      break;

    case WINED3DTSS_RESULTARG             :
      TRACE("RESULTARG Still a stub, Stage=%ld, Type=%d, Value =%ld\n", Stage, Type, Value);
      break;

    default:
        /* Put back later: FIXME("(%p) : stub, Stage=%ld, Type=%d, Value =%ld\n", This, Stage, Type, Value); */
        TRACE("Still a stub, Stage=%ld, Type=%d, Value =%ld\n", Stage, Type, Value);
    }

    LEAVE_GL();

    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_GetTextureStageState(IWineD3DDevice *iface, DWORD Stage, WINED3DTEXTURESTAGESTATETYPE Type, DWORD* pValue) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : requesting Stage %ld, Type %d getting %ld\n", This, Stage, Type, This->updateStateBlock->textureState[Stage][Type]);
    *pValue = This->updateStateBlock->textureState[Stage][Type];
    return D3D_OK;
}

/*****
 * Get / Set Texture
 *****/
HRESULT WINAPI IWineD3DDeviceImpl_SetTexture(IWineD3DDevice *iface, DWORD Stage, IWineD3DBaseTexture* pTexture) {

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DBaseTexture   *oldTexture;
    BOOL                   reapplyStates = TRUE;
    DWORD                  reapplyFlags = 0;
    INT                    oldTextureDimensions = -1;
    D3DRESOURCETYPE        textureType;

    oldTexture = This->updateStateBlock->textures[Stage];
    TRACE("(%p) : Stage(%ld), Texture (%p)\n", This, Stage, pTexture);

    /* Reject invalid texture units */
    if (Stage >= GL_LIMITS(textures)) {
        TRACE("Attempt to access invalid texture rejected\n");
        return D3DERR_INVALIDCALL;
    }

    This->updateStateBlock->set.textures[Stage] = TRUE;
    This->updateStateBlock->changed.textures[Stage] = TRUE;
    This->updateStateBlock->textures[Stage] = pTexture;

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return D3D_OK;
    }

    oldTextureDimensions = This->updateStateBlock->textureDimensions[Stage];

    ENTER_GL();

    /* Make appropriate texture active */
    if (GL_SUPPORT(ARB_MULTITEXTURE)) {
        GLACTIVETEXTURE(Stage);

    } else if (Stage>0) {
        FIXME("Program using multiple concurrent textures which this opengl implementation doesn't support\n");
    }

    /** NOTE: MSDN says that setTexture increases the reference count,
    * and the the application nust set the texture back to null (or have a leaky application),
    * This means we should pass the refcount upto the parent
     *******************************/
    if (NULL != oldTexture) {

        IUnknown *textureParent;
        IWineD3DBaseTexture_GetParent(oldTexture, (IUnknown **)&textureParent);
        IUnknown_Release(textureParent);
        IUnknown_Release(textureParent); /** NOTE: Twice because GetParent adds a ref **/
        oldTexture = NULL;
    }


    if (NULL != pTexture) {
        IUnknown *textureParent;
        IWineD3DBaseTexture_GetParent(This->updateStateBlock->textures[Stage], (IUnknown **)&textureParent);
        /** NOTE: GetParent will increase the ref count for me, I won't clean up untill the texture is set to NULL **/

        /* Now setup the texture appropraitly */
        textureType = IWineD3DBaseTexture_GetType(pTexture);

        if (textureType == D3DRTYPE_TEXTURE) {

          if (oldTexture == pTexture && !IWineD3DBaseTexture_GetDirty(pTexture)) {
            TRACE("Skipping setting texture as old == new\n");
            reapplyStates = FALSE;

          } else {

            /* Standard 2D texture */
            TRACE("Standard 2d texture\n");
            This->updateStateBlock->textureDimensions[Stage] = GL_TEXTURE_2D;

            /* Load up the texture now */
            IWineD3DTexture_PreLoad((IWineD3DTexture *) pTexture);
          }

        } else if (textureType == D3DRTYPE_VOLUMETEXTURE) {

          if (oldTexture == pTexture && !IWineD3DBaseTexture_GetDirty(pTexture)) {
              TRACE("Skipping setting texture as old == new\n");
              reapplyStates = FALSE;

          } else {

              /* Standard 3D (volume) texture */
              TRACE("Standard 3d texture\n");
              This->updateStateBlock->textureDimensions[Stage] = GL_TEXTURE_3D;

              /* Load up the texture now */
              IWineD3DVolumeTexture_PreLoad((IWineD3DVolumeTexture *) pTexture);
          }

        } else if (textureType == D3DRTYPE_CUBETEXTURE) {

            if (oldTexture == pTexture && !IWineD3DBaseTexture_GetDirty(pTexture)) {
                TRACE("Skipping setting texture as old == new\n");
                reapplyStates = FALSE;

            } else {

                /* Standard Cube texture */
                TRACE("Standard Cube texture\n");
                This->updateStateBlock->textureDimensions[Stage] = GL_TEXTURE_CUBE_MAP_ARB;

                /* Load up the texture now */
                IWineD3DCubeTexture_PreLoad((IWineD3DCubeTexture *) pTexture);
            }

        } else {
            FIXME("(%p) : Incorrect type for a texture : (%d,%s)\n", This, textureType, debug_d3dresourcetype(textureType));
        }

    } else {

        TRACE("Setting to no texture (ie default texture)\n");
        This->updateStateBlock->textureDimensions[Stage] = GL_TEXTURE_1D;
        glBindTexture(GL_TEXTURE_1D, This->dummyTextureName[Stage]);
        checkGLcall("glBindTexture");
        TRACE("Bound dummy Texture to stage %ld (gl name %d)\n", Stage, This->dummyTextureName[Stage]);
    }

    /* Disable the old texture binding and enable the new one (unless operations are disabled) */
    if (oldTextureDimensions != This->updateStateBlock->textureDimensions[Stage]) {

       glDisable(oldTextureDimensions);
       checkGLcall("Disable oldTextureDimensions");

       if (This->stateBlock->textureState[Stage][WINED3DTSS_COLOROP] != D3DTOP_DISABLE) {
          glEnable(This->updateStateBlock->textureDimensions[Stage]);
          checkGLcall("glEnable new texture dimensions");
       }

       /* If Alpha arg1 is texture then handle the special case when there changes between a
          texture and no texture - See comments in set_tex_op                                  */
       if ((This->stateBlock->textureState[Stage][WINED3DTSS_ALPHAARG1] == D3DTA_TEXTURE) &&
           (((oldTexture == NULL) && (pTexture != NULL)) || ((pTexture == NULL) && (oldTexture != NULL))))
       {
           reapplyFlags |= REAPPLY_ALPHAOP;
       }
    }


    /* Even if the texture has been set to null, reapply the stages as a null texture to directx requires
       a dummy texture in opengl, and we always need to ensure the current view of the TextureStates apply */
    if (reapplyStates) {
       IWineD3DDeviceImpl_SetupTextureStates(iface, Stage, reapplyFlags);
    }

    LEAVE_GL();
    TRACE("Texture now fully setup\n");

    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_GetTexture(IWineD3DDevice *iface, DWORD Stage, IWineD3DBaseTexture** ppTexture) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : returning %p for stage %ld\n", This, This->updateStateBlock->textures[Stage], Stage);
    *ppTexture = (IWineD3DBaseTexture *) This->updateStateBlock->textures[Stage];
    if (*ppTexture)
        IWineD3DBaseTexture_AddRef(*ppTexture);
    return D3D_OK;
}

/*****
 * Get Back Buffer
 *****/
HRESULT WINAPI IWineD3DDeviceImpl_GetBackBuffer(IWineD3DDevice *iface, UINT iSwapChain, UINT BackBuffer, D3DBACKBUFFER_TYPE Type,
                                                IWineD3DSurface** ppBackBuffer) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DSwapChain *swapChain;
    HRESULT hr;

    TRACE("(%p) : BackBuf %d Type %d SwapChain %d returning %p\n", This, BackBuffer, Type, iSwapChain, *ppBackBuffer);

    hr = IWineD3DDeviceImpl_GetSwapChain(iface,  iSwapChain, &swapChain);
    if(hr == D3D_OK) {
        hr = IWineD3DSwapChain_GetBackBuffer(swapChain, BackBuffer, Type, ppBackBuffer);
            IWineD3DSwapChain_Release(swapChain);
    }else{
        *ppBackBuffer = NULL;
    }
    return hr;
}

HRESULT WINAPI IWineD3DDeviceImpl_GetDeviceCaps(IWineD3DDevice *iface, WINED3DCAPS* pCaps) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    WARN("(%p) : stub, calling idirect3d for now\n", This);
    return IWineD3D_GetDeviceCaps(This->wineD3D, This->adapterNo, This->devType, pCaps);
}

HRESULT WINAPI IWineD3DDeviceImpl_GetDisplayMode(IWineD3DDevice *iface, UINT iSwapChain, D3DDISPLAYMODE* pMode) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DSwapChain *swapChain;
    HRESULT hr;

    hr = IWineD3DDeviceImpl_GetSwapChain(iface,  iSwapChain, (IWineD3DSwapChain **)&swapChain);
    if (hr == D3D_OK) {
        hr = IWineD3DSwapChain_GetDisplayMode(swapChain, pMode);
        IWineD3DSwapChain_Release(swapChain);
    }else{
        FIXME("(%p) Error getting display mode\n", This);
    }
    return hr;
}
/*****
 * Stateblock related functions
 *****/

 HRESULT WINAPI IWineD3DDeviceImpl_BeginStateBlock(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DStateBlockImpl *object;
    TRACE("(%p)", This);
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IWineD3DStateBlockImpl));
    if(NULL == object ) {
        FIXME("(%p)Error allocating memory for stateblock\n", This);
        return E_OUTOFMEMORY;
    }
    TRACE("(%p) creted object %p\n", This, object);
    object->wineD3DDevice= This;
    /** FIXME: object->parent       = parent; **/
    object->parent       = NULL;
    object->blockType    = D3DSBT_ALL;
    object->ref          = 1;
    object->lpVtbl       = &IWineD3DStateBlock_Vtbl;

    IWineD3DStateBlock_Release((IWineD3DStateBlock*)This->updateStateBlock);
    This->updateStateBlock = object;
    This->isRecordingState = TRUE;

    TRACE("(%p) recording stateblock %p\n",This , object);
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_EndStateBlock(IWineD3DDevice *iface, IWineD3DStateBlock** ppStateBlock) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    if (!This->isRecordingState) {
        FIXME("(%p) not recording! returning error\n", This);
        *ppStateBlock = NULL;
        return D3DERR_INVALIDCALL;
    }

    *ppStateBlock = (IWineD3DStateBlock*)This->updateStateBlock;
    This->isRecordingState = FALSE;
    This->updateStateBlock = This->stateBlock;
    IWineD3DStateBlock_AddRef((IWineD3DStateBlock*)This->updateStateBlock);
    /* IWineD3DStateBlock_AddRef(*ppStateBlock); don't need to do this, since we should really just release UpdateStateBlock first */
    TRACE("(%p) returning token (ptr to stateblock) of %p\n", This, *ppStateBlock);
    return D3D_OK;
}

/*****
 * Scene related functions
 *****/
HRESULT WINAPI IWineD3DDeviceImpl_BeginScene(IWineD3DDevice *iface) {
    /* At the moment we have no need for any functionality at the beginning
       of a scene                                                          */
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_EndScene(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p)\n", This);
    ENTER_GL();
    /* We only have to do this if we need to read the, swapbuffers performs a flush for us */
    glFlush();
    checkGLcall("glFlush");

    TRACE("End Scene\n");
    if(This->renderTarget != NULL) {

        /* If the container of the rendertarget is a texture then we need to save the data from the pbuffer */
        IUnknown *targetContainer = NULL;
        if (D3D_OK == IWineD3DSurface_GetContainer(This->renderTarget, &IID_IWineD3DBaseTexture, (void **)&targetContainer)) {
            TRACE("(%p) : Texture rendertarget %p\n", This ,This->renderTarget);
            /** always dirtify for now. we must find a better way to see that surface have been modified
            (Modifications should will only occur via draw-primitive, but we do need better locking
            switching to render-to-texture should remove the overhead though.
            */
            IWineD3DSurface_SetPBufferState(This->renderTarget, TRUE /* inPBuffer */, FALSE /* inTexture */);
            IWineD3DBaseTexture_SetDirty((IWineD3DBaseTexture *)targetContainer, TRUE);
            IWineD3DBaseTexture_PreLoad((IWineD3DBaseTexture *)targetContainer);
            IWineD3DSurface_SetPBufferState(This->renderTarget, FALSE /* inPBuffer */, FALSE /* inTexture */);
            IUnknown_Release(targetContainer);
        } else
        if (D3D_OK == IWineD3DSurface_GetContainer(This->renderTarget, &IID_IWineD3DDevice, (void **)&targetContainer)) {
                /* The surface is stand-alone, so doesn't currently have a context of it's own */
                TRACE("(%p) : standalone rendertarget %p\n", This, This->renderTarget);
                IWineD3DSurface_SetPBufferState(This->renderTarget, TRUE /* inPBuffer */, FALSE /* inTexture */);
                IWineD3DSurface_AddDirtyRect(This->renderTarget, NULL);
                IWineD3DSurface_PreLoad(This->renderTarget);
                IWineD3DSurface_SetPBufferState(This->renderTarget, FALSE /* inPBuffer */, FALSE /* inTexture */);
                IUnknown_Release(targetContainer);
        }

    }

    LEAVE_GL();
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_Present(IWineD3DDevice *iface,
                                          CONST RECT* pSourceRect, CONST RECT* pDestRect,
                                          HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion) {
   IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DSwapChain *swapChain = NULL;
    int i;
    int swapchains = IWineD3DDeviceImpl_GetNumberOfSwapChains(iface);

    TRACE("(%p) Presenting the frame\n", This);

    for(i = 0 ; i < swapchains ; i ++) {

        IWineD3DDeviceImpl_GetSwapChain(iface, i , (IWineD3DSwapChain **)&swapChain);
        TRACE("presentinng chain %d, %p\n", i, swapChain);
        IWineD3DSwapChain_Present(swapChain, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, 0);
        IWineD3DSwapChain_Release(swapChain);
    }

    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_Clear(IWineD3DDevice *iface, DWORD Count, CONST D3DRECT* pRects,
                                        DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    /* TODO: From MSDN This method fails if you specify the D3DCLEAR_ZBUFFER or D3DCLEAR_STENCIL flags when the
      render target does not have an attached depth buffer. Similarly, if you specify the D3DCLEAR_STENCIL flag
      when the depth-buffer format does not contain stencil buffer information, this method fails. */
    GLbitfield     glMask = 0;
    GLboolean      old_ztest;
    GLfloat        old_z_clear_value;
    GLint          old_stencil_clear_value;
    GLfloat        old_color_clear_value[4];
    unsigned int   i;
    CONST D3DRECT* curRect;

    TRACE("(%p) Count (%ld), pRects (%p), Flags (%lx), Z (%f), Stencil (%ld)\n", This,
          Count, pRects, Flags, Z, Stencil);

    ENTER_GL();

    glEnable(GL_SCISSOR_TEST);
    checkGLcall("glEnable GL_SCISSOR_TEST");

    if (Count > 0 && pRects) {
        curRect = pRects;
    } else {
        curRect = NULL;
    }

    /* Only set the values up once, as they are not changing */
    if (Flags & D3DCLEAR_STENCIL) {
        glGetIntegerv(GL_STENCIL_CLEAR_VALUE, &old_stencil_clear_value);
        glClearStencil(Stencil);
        checkGLcall("glClearStencil");
        glMask = glMask | GL_STENCIL_BUFFER_BIT;
        glStencilMask(0xFFFFFFFF);
    }

    if (Flags & D3DCLEAR_ZBUFFER) {
        glGetBooleanv(GL_DEPTH_WRITEMASK, &old_ztest);
        glDepthMask(GL_TRUE);
        glGetFloatv(GL_DEPTH_CLEAR_VALUE, &old_z_clear_value);
        glClearDepth(Z);
        checkGLcall("glClearDepth");
        glMask = glMask | GL_DEPTH_BUFFER_BIT;
    }

    if (Flags & D3DCLEAR_TARGET) {
        TRACE("Clearing screen with glClear to color %lx\n", Color);
        glGetFloatv(GL_COLOR_CLEAR_VALUE, old_color_clear_value);
        glClearColor(((Color >> 16) & 0xFF) / 255.0f,
                     ((Color >>  8) & 0xFF) / 255.0f,
                     ((Color >>  0) & 0xFF) / 255.0f,
                     ((Color >> 24) & 0xFF) / 255.0f);
        checkGLcall("glClearColor");

        /* Clear ALL colors! */
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glMask = glMask | GL_COLOR_BUFFER_BIT;
    }

    /* Now process each rect in turn */
    for (i = 0; i < Count || i == 0; i++) {

        if (curRect) {
            /* Note gl uses lower left, width/height */
            TRACE("(%p) %p Rect=(%ld,%ld)->(%ld,%ld) glRect=(%ld,%ld), len=%ld, hei=%ld\n", This, curRect,
                  curRect->x1, curRect->y1, curRect->x2, curRect->y2,
                  curRect->x1, (((IWineD3DSurfaceImpl *)This->renderTarget)->currentDesc.Height - curRect->y2),
                  curRect->x2 - curRect->x1, curRect->y2 - curRect->y1);
            glScissor(curRect->x1, (((IWineD3DSurfaceImpl *)This->renderTarget)->currentDesc.Height - curRect->y2),
                      curRect->x2 - curRect->x1, curRect->y2 - curRect->y1);
            checkGLcall("glScissor");
        } else {
            glScissor(This->stateBlock->viewport.X,
                      (((IWineD3DSurfaceImpl *)This->renderTarget)->currentDesc.Height - (This->stateBlock->viewport.Y + This->stateBlock->viewport.Height)),
                      This->stateBlock->viewport.Width,
                      This->stateBlock->viewport.Height);
            checkGLcall("glScissor");
        }

        /* Clear the selected rectangle (or full screen) */
        glClear(glMask);
        checkGLcall("glClear");

        /* Step to the next rectangle */
        if (curRect) curRect = curRect + sizeof(D3DRECT);
    }

    /* Restore the old values (why..?) */
    if (Flags & D3DCLEAR_STENCIL) {
        glClearStencil(old_stencil_clear_value);
        glStencilMask(This->stateBlock->renderState[WINED3DRS_STENCILWRITEMASK]);
    }
    if (Flags & D3DCLEAR_ZBUFFER) {
        glDepthMask(old_ztest);
        glClearDepth(old_z_clear_value);
    }
    if (Flags & D3DCLEAR_TARGET) {
        glClearColor(old_color_clear_value[0],
                     old_color_clear_value[1],
                     old_color_clear_value[2],
                     old_color_clear_value[3]);
        glColorMask(This->stateBlock->renderState[WINED3DRS_COLORWRITEENABLE] & D3DCOLORWRITEENABLE_RED ? GL_TRUE : GL_FALSE,
                    This->stateBlock->renderState[WINED3DRS_COLORWRITEENABLE] & D3DCOLORWRITEENABLE_GREEN ? GL_TRUE : GL_FALSE,
                    This->stateBlock->renderState[WINED3DRS_COLORWRITEENABLE] & D3DCOLORWRITEENABLE_BLUE  ? GL_TRUE : GL_FALSE,
                    This->stateBlock->renderState[WINED3DRS_COLORWRITEENABLE] & D3DCOLORWRITEENABLE_ALPHA ? GL_TRUE : GL_FALSE);
    }

    glDisable(GL_SCISSOR_TEST);
    checkGLcall("glDisable");
    LEAVE_GL();

    return D3D_OK;
}

/*****
 * Drawing functions
 *****/
HRESULT WINAPI IWineD3DDeviceImpl_DrawPrimitive(IWineD3DDevice *iface, D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex,
                                                UINT PrimitiveCount) {

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    This->stateBlock->streamIsUP = FALSE;

    TRACE("(%p) : Type=(%d,%s), Start=%d, Count=%d\n", This, PrimitiveType,
                               debug_d3dprimitivetype(PrimitiveType),
                               StartVertex, PrimitiveCount);
    drawPrimitive(iface, PrimitiveType, PrimitiveCount, StartVertex, -1, 0, NULL, 0);

    return D3D_OK;
}

/* TODO: baseVIndex needs to be provided from This->stateBlock->baseVertexIndex when called from d3d8 */
HRESULT  WINAPI  IWineD3DDeviceImpl_DrawIndexedPrimitive(IWineD3DDevice *iface,
                                                           D3DPRIMITIVETYPE PrimitiveType,
                                                           INT baseVIndex, UINT minIndex,
                                                           UINT NumVertices,UINT startIndex,UINT primCount) {

    IWineD3DDeviceImpl  *This = (IWineD3DDeviceImpl *)iface;
    UINT                 idxStride = 2;
    IWineD3DIndexBuffer *pIB;
    D3DINDEXBUFFER_DESC  IdxBufDsc;

    pIB = This->stateBlock->pIndexData;
    This->stateBlock->streamIsUP = FALSE;

    TRACE("(%p) : Type=(%d,%s), min=%d, CountV=%d, startIdx=%d, baseVidx=%d, countP=%d \n", This,
          PrimitiveType, debug_d3dprimitivetype(PrimitiveType),
          minIndex, NumVertices, startIndex, baseVIndex, primCount);

    IWineD3DIndexBuffer_GetDesc(pIB, &IdxBufDsc);
    if (IdxBufDsc.Format == WINED3DFMT_INDEX16) {
        idxStride = 2;
    } else {
        idxStride = 4;
    }

    drawPrimitive(iface, PrimitiveType, primCount, baseVIndex,
                      startIndex, idxStride,
                      ((IWineD3DIndexBufferImpl *) pIB)->resource.allocatedMemory,
                      minIndex);

    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_DrawPrimitiveUP(IWineD3DDevice *iface, D3DPRIMITIVETYPE PrimitiveType,
                                                    UINT PrimitiveCount, CONST void* pVertexStreamZeroData,
                                                    UINT VertexStreamZeroStride) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("(%p) : Type=(%d,%s), pCount=%d, pVtxData=%p, Stride=%d\n", This, PrimitiveType,
             debug_d3dprimitivetype(PrimitiveType),
             PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride);

    if (This->stateBlock->streamSource[0] != NULL) IWineD3DVertexBuffer_Release(This->stateBlock->streamSource[0]);

    /* Note in the following, it's not this type, but that's the purpose of streamIsUP */
    This->stateBlock->streamSource[0] = (IWineD3DVertexBuffer *)pVertexStreamZeroData;
    This->stateBlock->streamStride[0] = VertexStreamZeroStride;
    This->stateBlock->streamIsUP = TRUE;
    drawPrimitive(iface, PrimitiveType, PrimitiveCount, 0, 0, 0, NULL, 0);
    This->stateBlock->streamStride[0] = 0;
    This->stateBlock->streamSource[0] = NULL;

    /*stream zero settings set to null at end, as per the msdn */
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_DrawIndexedPrimitiveUP(IWineD3DDevice *iface, D3DPRIMITIVETYPE PrimitiveType,
                                                             UINT MinVertexIndex,
                                                             UINT NumVertexIndices,UINT PrimitiveCount,CONST void* pIndexData,
                                                             WINED3DFORMAT IndexDataFormat, CONST void* pVertexStreamZeroData,
                                                             UINT VertexStreamZeroStride) {
    int                 idxStride;
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("(%p) : Type=(%d,%s), MinVtxIdx=%d, NumVIdx=%d, PCount=%d, pidxdata=%p, IdxFmt=%d, pVtxdata=%p, stride=%d\n",
             This, PrimitiveType, debug_d3dprimitivetype(PrimitiveType),
             MinVertexIndex, NumVertexIndices, PrimitiveCount, pIndexData,
             IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);

    if (This->stateBlock->streamSource[0] != NULL) IWineD3DVertexBuffer_Release(This->stateBlock->streamSource[0]);

    if (IndexDataFormat == WINED3DFMT_INDEX16) {
        idxStride = 2;
    } else {
        idxStride = 4;
    }

    /* Note in the following, it's not this type, but that's the purpose of streamIsUP */
    This->stateBlock->streamSource[0] = (IWineD3DVertexBuffer *)pVertexStreamZeroData;
    This->stateBlock->streamIsUP = TRUE;
    This->stateBlock->streamStride[0] = VertexStreamZeroStride;

    drawPrimitive(iface, PrimitiveType, PrimitiveCount, 0, 0, idxStride, pIndexData, MinVertexIndex);

    /* stream zero settings set to null at end as per the msdn */
    This->stateBlock->streamSource[0] = NULL;
    This->stateBlock->streamStride[0] = 0;
    IWineD3DDevice_SetIndices(iface, NULL, 0);

    return D3D_OK;
}

 /* Yet another way to update a texture, some apps use this to load default textures instead of using surface/texture lock/unlock */
HRESULT WINAPI IWineD3DDeviceImpl_UpdateTexture (IWineD3DDevice *iface, IWineD3DBaseTexture *pSourceTexture,  IWineD3DBaseTexture *pDestinationTexture){
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    D3DRESOURCETYPE sourceType;
    D3DRESOURCETYPE destinationType;
    IWineD3DTextureImpl *pDestTexture = (IWineD3DTextureImpl *)pDestinationTexture;
    IWineD3DTextureImpl *pSrcTexture  = (IWineD3DTextureImpl *)pSourceTexture;
    int i;

    sourceType = IWineD3DBaseTexture_GetType(pSourceTexture);
    destinationType = IWineD3DBaseTexture_GetType(pDestinationTexture);
    if(sourceType != D3DRTYPE_TEXTURE && destinationType != D3DRTYPE_TEXTURE){
        FIXME("(%p) Only D3DRTYPE_TEXTURE to D3DRTYPE_TEXTURE supported\n", This);
        return D3DERR_INVALIDCALL;
    }
    TRACE("(%p) Source %p Destination %p\n", This, pSourceTexture, pDestinationTexture);

    /** TODO: Get rid of the casts to IWineD3DBaseTextureImpl
        repalce surfaces[x] with GetSurfaceLevel, or GetCubeMapSurface etc..
        think about moving the code into texture, and adding a member to base texture to occomplish this **/

    /* Make sure that the destination texture is loaded */
    IWineD3DBaseTexture_PreLoad(pDestinationTexture);
    TRACE("Loading source texture\n");

    if(pSrcTexture->surfaces[0] == NULL || pDestTexture->surfaces[0] == NULL){
        FIXME("(%p) Texture src %p or dest %p has not surface %p %p\n", This, pSrcTexture, pDestTexture,
               pSrcTexture->surfaces[0], pDestTexture->surfaces[0]);
    }

    if(((IWineD3DSurfaceImpl *)pSrcTexture->surfaces[0])->resource.pool != D3DPOOL_SYSTEMMEM ||
        ((IWineD3DSurfaceImpl *)pDestTexture->surfaces[0])->resource.pool != D3DPOOL_DEFAULT){

        FIXME("(%p) source %p must be SYSTEMMEM and dest %p must be DEFAULT\n",This, pSrcTexture, pDestTexture);
        return D3DERR_INVALIDCALL;
    }
    /** TODO: check that both textures have the same number of levels  **/
#if 0
    if(IWineD3DBaseTexture_GetLevelCount(pDestinationTexture)  !=IWineD3DBaseTexture_GetLevelCount(pSourceTexture))
            return D3DERR_INVALIDCALL;
#endif
    /** TODO: move this code into baseTexture? device should never touch impl*'s **/
    for(i = 0 ; i < IWineD3DBaseTexture_GetLevelCount(pDestinationTexture) ; i++){
        IWineD3DDevice_UpdateSurface(iface, pSrcTexture->surfaces[i], NULL, pDestTexture->surfaces[i], NULL);
    }

    return D3D_OK;
}

HRESULT  WINAPI  IWineD3DDeviceImpl_StretchRect(IWineD3DDevice *iface, IWineD3DSurface *pSourceSurface,
                                                CONST RECT* pSourceRect, IWineD3DSurface *pDestinationSurface,
                                                CONST RECT* pDestRect, D3DTEXTUREFILTERTYPE Filter) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("(%p) : stub\n", This);
    return D3D_OK;
}
HRESULT  WINAPI  IWineD3DDeviceImpl_GetRenderTargetData(IWineD3DDevice *iface, IWineD3DSurface *pRenderTarget, IWineD3DSurface *pSurface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT  WINAPI  IWineD3DDeviceImpl_GetFrontBufferData(IWineD3DDevice *iface,UINT iSwapChain, IWineD3DSurface *pDestSurface) {
    IWineD3DSwapChain *swapChain;
    HRESULT hr;
    hr = IWineD3DDeviceImpl_GetSwapChain(iface,  iSwapChain, (IWineD3DSwapChain **)&swapChain);
    if(hr == D3D_OK) {
        hr = IWineD3DSwapChain_GetFrontBufferData(swapChain, pDestSurface);
                IWineD3DSwapChain_Release(swapChain);
    }
    return hr;
}

HRESULT  WINAPI  IWineD3DDeviceImpl_ValidateDevice(IWineD3DDevice *iface, DWORD* pNumPasses) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    /* return a sensible default */
    *pNumPasses = 1;
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT  WINAPI  IWineD3DDeviceImpl_SetPaletteEntries(IWineD3DDevice *iface, UINT PaletteNumber, CONST PALETTEENTRY* pEntries) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT  WINAPI  IWineD3DDeviceImpl_GetPaletteEntries(IWineD3DDevice *iface, UINT PaletteNumber, PALETTEENTRY* pEntries) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT  WINAPI  IWineD3DDeviceImpl_SetCurrentTexturePalette(IWineD3DDevice *iface, UINT PaletteNumber) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT  WINAPI  IWineD3DDeviceImpl_GetCurrentTexturePalette(IWineD3DDevice *iface, UINT* PaletteNumber) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT  WINAPI  IWineD3DDeviceImpl_SetSoftwareVertexProcessing(IWineD3DDevice *iface, BOOL bSoftware) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}


BOOL     WINAPI  IWineD3DDeviceImpl_GetSoftwareVertexProcessing(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    FIXME("(%p) : stub\n", This);
    return FALSE;
}


HRESULT  WINAPI  IWineD3DDeviceImpl_GetRasterStatus(IWineD3DDevice *iface, UINT iSwapChain, D3DRASTER_STATUS* pRasterStatus) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    pRasterStatus->InVBlank = TRUE;
    pRasterStatus->ScanLine = 0;
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}


HRESULT  WINAPI  IWineD3DDeviceImpl_SetNPatchMode(IWineD3DDevice *iface, float nSegments) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    static BOOL showfixmes = TRUE;
    if(nSegments != 0.0f) {
        if( showfixmes) {
            FIXME("(%p) : stub nSegments(%f)\n", This, nSegments);
            showfixmes = FALSE;
        }
    }
    return D3D_OK;
}

float    WINAPI  IWineD3DDeviceImpl_GetNPatchMode(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    static BOOL showfixmes = TRUE;
    if( showfixmes) {
        FIXME("(%p) : stub returning(%f)\n", This, 0.0f);
        showfixmes = FALSE;
    }
    return 0.0f;
}


HRESULT  WINAPI  IWineD3DDeviceImpl_UpdateSurface(IWineD3DDevice *iface, IWineD3DSurface *pSourceSurface, CONST RECT* pSourceRect, IWineD3DSurface *pDestinationSurface, CONST POINT* pDestPoint) {
    IWineD3DDeviceImpl  *This         = (IWineD3DDeviceImpl *) iface;
    /** TODO: remove casts to IWineD3DSurfaceImpl
     *       NOTE: move code to surface to accomplish this
      ****************************************/
    IWineD3DSurfaceImpl *pSrcSurface  = (IWineD3DSurfaceImpl *)pSourceSurface;
    int srcWidth, srcHeight, srcSurfaceWidth, srcSurfaceHeight, destSurfaceWidth, destSurfaceHeight;
    WINED3DFORMAT destFormat, srcFormat;
    UINT destSize;
    int destLeft, destTop;
    D3DPOOL     srcPool, destPool;
    int offset    = 0;
    int rowoffset = 0; /* how many bytes to add onto the end of a row to wraparound to the beginning of the next */
    glDescriptor *glDescription = NULL;
    GLenum textureDimensions = GL_TEXTURE_2D;
    IWineD3DBaseTexture *baseTexture;

    WINED3DSURFACE_DESC  winedesc;

    memset(&winedesc, 0, sizeof(winedesc));
    winedesc.Width  = &srcSurfaceWidth;
    winedesc.Height = &srcSurfaceHeight;
    winedesc.Pool   = &srcPool;
    winedesc.Format = &srcFormat;

    IWineD3DSurface_GetDesc(pSourceSurface, &winedesc);

    winedesc.Width  = &destSurfaceWidth;
    winedesc.Height = &destSurfaceHeight;
    winedesc.Pool   = &destPool;
    winedesc.Format = &destFormat;
    winedesc.Size   = &destSize;

    IWineD3DSurface_GetDesc(pDestinationSurface, &winedesc);

    if(srcPool != D3DPOOL_SYSTEMMEM  || destPool != D3DPOOL_DEFAULT){
        FIXME("source %p must be SYSTEMMEM and dest %p must be DEFAULT\n", pSourceSurface, pDestinationSurface);
        return D3DERR_INVALIDCALL;
    }
    /* TODO:  change this to use bindTexture */
    /* Make sure the surface is loaded and upto date */
    IWineD3DSurface_PreLoad(pDestinationSurface);

    IWineD3DSurface_GetGlDesc(pDestinationSurface, &glDescription);

    ENTER_GL();

    /* this needs to be done in lines if the sourceRect != the sourceWidth */
    srcWidth   = pSourceRect ? pSourceRect->right - pSourceRect->left   : srcSurfaceWidth;
    srcHeight  = pSourceRect ? pSourceRect->top   - pSourceRect->bottom : srcSurfaceHeight;
    destLeft   = pDestPoint  ? pDestPoint->x : 0;
    destTop    = pDestPoint  ? pDestPoint->y : 0;


    /* This function doesn't support compressed textures
    the pitch is just bytesPerPixel * width */

    if(srcWidth != srcSurfaceWidth  || (pSourceRect != NULL && pSourceRect->left != 0) ){
        rowoffset = (srcSurfaceWidth - srcWidth) * pSrcSurface->bytesPerPixel;
        offset   += pSourceRect->left * pSrcSurface->bytesPerPixel;
        /* TODO: do we ever get 3bpp?, would a shift and an add be quicker than a mul (well maybe a cycle or two) */
    }
    /* TODO DXT formats */

    if(pSourceRect != NULL && pSourceRect->top != 0){
       offset +=  pSourceRect->top * srcWidth * pSrcSurface->bytesPerPixel;
    }
    TRACE("(%p) glTexSubImage2D, Level %d, left %d, top %d, width %d, height %d , ftm %d, type %d, memory %p\n"
    ,This
    ,glDescription->level
    ,destLeft
    ,destTop
    ,srcWidth
    ,srcHeight
    ,glDescription->glFormat
    ,glDescription->glType
    ,IWineD3DSurface_GetData(pSourceSurface)
    );

    /* Sanity check */
    if (IWineD3DSurface_GetData(pSourceSurface) == NULL) {
    /* need to lock the surface to get the data */
       FIXME("Surfaces has no allocated memory, but should be an in memory only surface\n");
    }
    /* TODO: Cube and volume support */
    if(rowoffset != 0){
        /* not a whole row so we have to do it a line at a time */
        int j;
        /* hopefully using pointer addtion will be quicker than using a point + j * rowoffset */
        unsigned char* data =((unsigned char *)IWineD3DSurface_GetData(pSourceSurface)) + offset;

        for(j = destTop ; j < (srcHeight + destTop) ; j++){

                glTexSubImage2D(glDescription->target
                    ,glDescription->level
                    ,destLeft
                    ,j
                    ,srcWidth
                    ,1
                    ,glDescription->glFormat
                    ,glDescription->glType
                    ,data/* could be quicker using */
                );
            data += rowoffset;
        }

    } else { /* Full width, so just write out the whole texture */

        if (WINED3DFMT_DXT1 == destFormat ||
            WINED3DFMT_DXT3 == destFormat ||
            WINED3DFMT_DXT5 == destFormat) {
            if (GL_SUPPORT(EXT_TEXTURE_COMPRESSION_S3TC)) {
                if (destSurfaceHeight != srcHeight || destSurfaceWidth != srcWidth) {
                    /* FIXME: The easy way to do this is lock the destination, and copy the bits accross */
                    FIXME("Updating part of a compressed texture is not supported at the moment\n");
                } if (destFormat != srcFormat) {
                    FIXME("Updating mixed format compressed texture is not curretly support\n");
                } else {
                    GL_EXTCALL(glCompressedTexImage2DARB)(glDescription->target,
                                                        glDescription->level,
                                                        glDescription->glFormatInternal,
                                                        srcWidth,
                                                        srcHeight,
                                                        0,
                                                        destSize,
                                                        IWineD3DSurface_GetData(pSourceSurface));
                }
            } else {
                FIXME("Attempting to update a DXT compressed texture without hardware support\n");
            }


        } else {
            /* some applications can not handle odd pitches returned by soft non-power2, so we have
               to repack the data from pow2Width/Height to expected Width,Height, this makes the
               data returned by GetData non-power2 width/height with hardware non-power2
               pow2Width/height are set to surface width height, repacking isn't needed so it
               doesn't matter which function gets called. */
            glTexSubImage2D(glDescription->target
                    ,glDescription->level
                    ,destLeft
                    ,destTop
                    ,srcWidth
                    ,srcHeight
                    ,glDescription->glFormat
                    ,glDescription->glType
                    ,IWineD3DSurface_GetData(pSourceSurface)
                );

        }
     }
    checkGLcall("glTexSubImage2D");
    /* I only need to look up baseTexture here, so it may be a good idea to hava a GL_TARGET ->
     * GL_DIMENSIONS lookup, or maybe store the dimensions on the surface (but that's making the
     * surface bigger than it needs to be hmm.. */
    if (D3D_OK == IWineD3DSurface_GetContainer(pDestinationSurface, &IID_IWineD3DBaseTexture, (void **)&baseTexture)) {
        textureDimensions = IWineD3DBaseTexture_GetTextureDimensions(baseTexture);
        IWineD3DBaseTexture_Release(baseTexture);
    }

    glDisable(textureDimensions); /* This needs to be managed better.... */
    LEAVE_GL();

    return D3D_OK;
}

/* Implementation details at http://developer.nvidia.com/attach/6494
and
http://oss.sgi.com/projects/ogl-sample/registry/NV/evaluators.txt
hmm.. no longer supported use
OpenGL evaluators or  tessellate surfaces within your application.
*/

/* http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/reference/d3d/interfaces/idirect3ddevice9/DrawRectPatch.asp */
HRESULT WINAPI IWineD3DDeviceImpl_DrawRectPatch(IWineD3DDevice *iface, UINT Handle, CONST float* pNumSegs, CONST D3DRECTPATCH_INFO* pRectPatchInfo) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) Handle(%d) noSegs(%p) rectpatch(%p) \n", This, Handle, pNumSegs, pRectPatchInfo);
    FIXME("(%p) : Stub\n", This);
    return D3D_OK;

}

/* http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/reference/d3d/interfaces/idirect3ddevice9/DrawTriPatch.asp */
HRESULT WINAPI IWineD3DDeviceImpl_DrawTriPatch(IWineD3DDevice *iface, UINT Handle, CONST float* pNumSegs, CONST D3DTRIPATCH_INFO* pTriPatchInfo) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) Handle(%d) noSegs(%p) tripatch(%p) \n", This, Handle, pNumSegs, pTriPatchInfo);
    FIXME("(%p) : Stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_DeletePatch(IWineD3DDevice *iface, UINT Handle) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) Handle(%d)\n", This, Handle);
    FIXME("(%p) : Stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_ColorFill(IWineD3DDevice *iface, IWineD3DSurface *pSurface, CONST D3DRECT* pRect, D3DCOLOR color) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;

    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

/* rendertarget and deptth stencil functions */
HRESULT  WINAPI  IWineD3DDeviceImpl_GetRenderTarget(IWineD3DDevice* iface,DWORD RenderTargetIndex, IWineD3DSurface **ppRenderTarget) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    /* FIXME: Implelent RenderTargetIndex >0 */
    if(RenderTargetIndex > 0)
        FIXME("(%p) : RenderTargetIndex %ld >0 not currently supported\n", This, RenderTargetIndex);

    *ppRenderTarget = (IWineD3DSurface *)This->renderTarget;
    TRACE("(%p) : RenderTarget %ld Index returning %p\n", This, RenderTargetIndex, *ppRenderTarget);
    /* Note inc ref on returned surface */
    if(*ppRenderTarget != NULL)
        IWineD3DSurface_AddRef(*ppRenderTarget);
    return D3D_OK;
}

HRESULT  WINAPI  IWineD3DDeviceImpl_GetDepthStencilSurface(IWineD3DDevice* iface, IWineD3DSurface **ppZStencilSurface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    *ppZStencilSurface = (IWineD3DSurface *)This->depthStencilBuffer;
    TRACE("(%p) : zStencilSurface  returning %p\n", This,  *ppZStencilSurface);

    if(*ppZStencilSurface != NULL) {
        /* Note inc ref on returned surface */
        IWineD3DSurface_AddRef(*ppZStencilSurface);
    }
    return D3D_OK;
}

/* internal static helper functions */
HRESULT WINAPI static IWineD3DDeviceImpl_ActiveRender(IWineD3DDevice* iface,
                                                IWineD3DSurface *RenderSurface);

HRESULT WINAPI static IWineD3DDeviceImpl_CleanRender(IWineD3DDevice* iface, IWineD3DSwapChainImpl *swapchain);

HRESULT WINAPI IWineD3DDeviceImpl_SetRenderTarget(IWineD3DDevice *iface, DWORD RenderTargetIndex, IWineD3DSurface *pRenderTarget) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    HRESULT  hr = D3D_OK;
    WINED3DVIEWPORT viewport;

    TRACE("(%p) Swapping rendertarget\n",This);
    if (RenderTargetIndex>0) {
        FIXME("(%p) Render targets other than the first are not supported\n",This);
        RenderTargetIndex=0;
    }

    /* MSDN says that null disables the render target
    but a device must always be associated with a render target
    nope MSDN says that we return invalid call to a null rendertarget with an index of 0

    see http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/programmingguide/AdvancedTopics/PixelPipe/MultipleRenderTarget.asp
    for more details
    */
    if (RenderTargetIndex == 0 && pRenderTarget == NULL) {
        FIXME("Trying to set render target 0 to NULL\n");
        return D3DERR_INVALIDCALL;
    }
    /* TODO: raplce Impl* usage with interface usage */
    if (!((IWineD3DSurfaceImpl *)pRenderTarget)->resource.usage & D3DUSAGE_RENDERTARGET) {
        FIXME("(%p)Trying to set the render target to a surface(%p) that wasn't created with a usage of D3DUSAGE_RENDERTARGET\n",This ,pRenderTarget);
        return D3DERR_INVALIDCALL;
    }
    /** TODO: check that the depth stencil format matches the render target, this is only done in debug
     *        builds, but I think wine counts as a 'debug' build for now.
      ******************************/
    /* If we are trying to set what we already have, don't bother */
    if (pRenderTarget == This->renderTarget) {
        TRACE("Trying to do a NOP SetRenderTarget operation\n");
    } else {
        /* Otherwise, set the render target up */
        TRACE("clearing renderer\n");
        /* IWineD3DDeviceImpl_CleanRender(iface); */
        /* OpenGL doesn't support 'sharing' of the stencilBuffer so we may incure an extra memory overhead
        depending on the renter target implementation being used.
        A shared context implementation will share all buffers between all rendertargets (including swapchains),
        implementations that use separate pbuffers for different swapchains or rendertargets will have to duplicate the
        stencil buffer and incure an extra memory overhead */
        hr = IWineD3DDeviceImpl_ActiveRender(iface, pRenderTarget);
    }

    if (SUCCEEDED(hr)) {
        /* Finally, reset the viewport as the MSDN states. */
        /* TODO: Replace impl usage */
        viewport.Height = ((IWineD3DSurfaceImpl *)This->renderTarget)->currentDesc.Height;
        viewport.Width  = ((IWineD3DSurfaceImpl *)This->renderTarget)->currentDesc.Width;
        viewport.X      = 0;
        viewport.Y      = 0;
        viewport.MaxZ   = 1.0f;
        viewport.MinZ   = 0.0f;
        IWineD3DDeviceImpl_SetViewport(iface, &viewport);
    }else{
        FIXME("Unknown error setting the render target\n");
    }
    return hr;
}

HRESULT WINAPI IWineD3DDeviceImpl_SetDepthStencilSurface(IWineD3DDevice *iface, IWineD3DSurface *pNewZStencil) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    HRESULT  hr = D3D_OK;
    WINED3DVIEWPORT viewport;
    IWineD3DSurface *tmp;

    TRACE("(%p) Swapping z-buffer\n",This);

    if (pNewZStencil == This->stencilBufferTarget) {
        TRACE("Trying to do a NOP SetRenderTarget operation\n");
    }else{
        /** OpenGL doesn't support 'sharing' of the stencilBuffer so we may incure an extra memory overhead
        * depending on the renter target implementation being used.
        * A shared context implementation will share all buffers between all rendertargets (including swapchains),
        * implementations that use separate pbuffers for different swapchains or rendertargets will have to duplicate the
        * stencil buffer and incure an extra memory overhead
         ******************************************************/


        tmp = This->stencilBufferTarget;
        This->stencilBufferTarget = pNewZStencil;
        /* should we be calling the parent or the wined3d surface? */
        if (NULL != This->stencilBufferTarget) IWineD3DSurface_AddRef(This->stencilBufferTarget);
        if (NULL != tmp) IWineD3DSurface_Release(tmp);
        hr = D3D_OK;
        /** TODO: glEnable/glDisable on depth/stencil    depending on
         *   pNewZStencil is NULL and the depth/stencil is enabled in d3d
          **********************************************************/
    }

    if (SUCCEEDED(hr)) {
        /* Finally, reset the viewport as the MSDN states.*/
        /* TODO: get ridd of Impl usage */
        viewport.Height = ((IWineD3DSurfaceImpl *)This->renderTarget)->currentDesc.Height;
        viewport.Width  = ((IWineD3DSurfaceImpl *)This->renderTarget)->currentDesc.Width;
        viewport.X      = 0;
        viewport.Y      = 0;
        viewport.MaxZ   = 1.0f;
        viewport.MinZ   = 0.0f;
        IWineD3DDeviceImpl_SetViewport(iface, &viewport);
    }

    return hr;
}


/* Internal functions not in DirectX */
 /** TODO: move this off to the opengl context manager
 *(the swapchain doesn't need to know anything about offscreen rendering!)
  ****************************************************/

HRESULT WINAPI IWineD3DDeviceImpl_CleanRender(IWineD3DDevice* iface, IWineD3DSwapChainImpl *swapchain)
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
#if defined(GL_VERSION_1_3) /* @see comments on ActiveRender */

    TRACE("(%p), %p\n", This, swapchain);

    if (swapchain->win != swapchain->drawable) {
        ENTER_GL();
        if (swapchain->glCtx != swapchain->render_ctx) {
            FIXME("Destroying context %p \n", swapchain->render_ctx);
            glXDestroyContext(swapchain->display, swapchain->render_ctx);

        }
        FIXME("glXDestroyPbuffer %ld \n", swapchain->drawable);
        glXDestroyPbuffer(swapchain->display, swapchain->drawable);
#endif
        LEAVE_GL();
        /* Set everything back the way it ws */
        swapchain->render_ctx = swapchain->glCtx;
        swapchain->drawable = swapchain->win;
    }
  return D3D_OK;
}

/** FIXME: This is currently used called whenever SetRenderTarget or SetStencilBuffer are called
* the functionality needs splitting up so that we don't do more than we should do.
* this only seems to impact performance a little.
 ******************************/
HRESULT WINAPI IWineD3DDeviceImpl_ActiveRender(IWineD3DDevice* iface,
                                               IWineD3DSurface *RenderSurface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
#ifndef USE_RENDER_MANAGER

  IWineD3DSurface *StencilSurface = This->stencilBufferTarget;
  HRESULT ret =  D3DERR_INVALIDCALL;
  /**
   * Currently only active for GLX >= 1.3
   * for others versions we'll have to use GLXPixmaps
   *
   * normally we must test GLX_VERSION_1_3 but nvidia headers are not correct
   * as they implement GLX 1.3 but only define GLX_VERSION_1_2
   * so only check OpenGL version
   * ..........................
   * I don't believe that it is a problem with NVidia headers,
   * XFree only supports GLX1.2, nVidia (and ATI to some extent) provide 1.3 functions
   * in GLX 1.2, there is no mention of the correct way to tell if the extensions are provided.
   * ATI Note:
   * Your application will report GLX version 1.2 on glXQueryVersion.
   * However, it is safe to call the GLX 1.3 functions as described below.
   */
#if defined(GL_VERSION_1_3)

    /** TODO: we only need to look up the configuration !IF! we are setting the target to a texture **/
    GLXFBConfig* cfgs = NULL;
    int nCfgs = 0;
    int attribs[256];
    int nAttribs = 0;
    IWineD3DSwapChain     *currentSwapchain;
    IWineD3DSwapChainImpl *swapchain;
    /** TODO: get rid of Impl usage we should always create a zbuffer/stencil with our contexts if possible,
    * but switch them off if the StencilSurface is set to NULL
    ** *********************************************************/
    D3DFORMAT BackBufferFormat = ((IWineD3DSurfaceImpl *) RenderSurface)->resource.format;
    D3DFORMAT StencilBufferFormat = (NULL != StencilSurface) ? ((IWineD3DSurfaceImpl *) StencilSurface)->resource.format : 0;
#if 0
    UINT Width = ((IWineD3DSurfaceImpl *) RenderSurface)->currentDesc.Width;
    UINT Height = ((IWineD3DSurfaceImpl *) RenderSurface)->currentDesc.Height;
#endif
    IWineD3DSurface *tmp;

    /**TODO:
        if StencilSurface == NULL && zBufferTarget != NULL then switch the zbuffer off,
        it StencilSurface != NULL && zBufferTarget == NULL switch it on
    */

#define PUSH1(att)        attribs[nAttribs++] = (att);
#define PUSH2(att,value)  attribs[nAttribs++] = (att); attribs[nAttribs++] = (value);

    /* PUSH2(GLX_BIND_TO_TEXTURE_RGBA_ATI, True); examples of this are few and far between (but I've got a nice working one!)*/

    /** TODO: remove the reff to Impl (context manager should fix this!) **/
    IWineD3DSwapChainImpl *impSwapChain;
    IWineD3DDevice_GetSwapChain(iface, 0, (IWineD3DSwapChain **)&impSwapChain);
    if (NULL == impSwapChain) { /* NOTE: This should NEVER fail */
        ERR("(%p) Failed to get a the implicit swapchain\n", iface);
    }

    ENTER_GL();

    PUSH2(GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT);
    PUSH2(GLX_X_RENDERABLE,  TRUE);
    PUSH2(GLX_DOUBLEBUFFER, TRUE);
    TRACE("calling makeglcfg\n");
    D3DFmtMakeGlCfg(BackBufferFormat, StencilBufferFormat, attribs, &nAttribs, FALSE /* alternate */);

    PUSH1(None);


    TRACE("calling chooseFGConfig\n");
    cfgs = glXChooseFBConfig(impSwapChain->display, DefaultScreen(impSwapChain->display),
                                                     attribs, &nCfgs);

    if (!cfgs) { /* OK we didn't find the exact config, so use any reasonable match */
        /* TODO: fill in the 'requested' and 'current' depths, also make sure that's
           why we failed and only show this message once! */
        MESSAGE("Failed to find exact match, finding alternative but you may suffer performance issues, try changing xfree's depth to match the requested depth\n"); /**/
        nAttribs = 0;
        PUSH2(GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT | GLX_WINDOW_BIT);
       /* PUSH2(GLX_X_RENDERABLE,  TRUE); */
        PUSH2(GLX_RENDER_TYPE,   GLX_RGBA_BIT);
        PUSH2(GLX_DOUBLEBUFFER, FALSE);
        TRACE("calling makeglcfg\n");
        D3DFmtMakeGlCfg(BackBufferFormat, StencilBufferFormat, attribs, &nAttribs, TRUE /* alternate */);
        PUSH1(None);
        cfgs = glXChooseFBConfig(impSwapChain->display, DefaultScreen(impSwapChain->display),
                                                        attribs, &nCfgs);
    }

#if 0
#ifdef EXTRA_TRACES
        int i;
        for (i = 0; i < nCfgs; ++i) {
            TRACE("for (%u,%s)/(%u,%s) found config[%d]@%p\n", BackBufferFormat,
            debug_d3dformat(BackBufferFormat), StencilBufferFormat,
            debug_d3dformat(StencilBufferFormat), i, cfgs[i]);
        }
#endif
#endif
    if (NULL != This->renderTarget) {
#ifdef EXTRA_TRACES
        glFlush();
        vcheckGLcall("glFlush");
        /** This is only useful if the old render target was a swapchain,
        * we need to supercede this with a function that displays
        * the current buffer on the screen. This is easy to do in glx1.3 but
        * we need to do copy-write pixels in glx 1.2.
         ************************************************/
        glXSwapBuffers(impSwapChain->display, impSwapChain->drawable);

        printf("Hit Enter to get next frame ...\n");
        getchar();
#endif
    }

    if (IWineD3DSurface_GetContainer(This->renderTarget, &IID_IWineD3DSwapChain, (void **)&currentSwapchain) != D3D_OK) {
        /* the selected render target doesn't belong to a swapchain, so use the devices implicit swapchain */
        IWineD3DDevice_GetSwapChain(iface, 0, &currentSwapchain);
    }

    /**
    * TODO: remove the use of IWineD3DSwapChainImpl, a context manager will help since it will replace the
    *  renderTarget = swapchain->backBuffer bit and anything to do with *glContexts
     **********************************************************************/
    if (IWineD3DSurface_GetContainer(RenderSurface, &IID_IWineD3DSwapChain, (void **)&swapchain) == D3D_OK) {
        /* We also need to make sure that the lights &co are also in the context of the swapchains */
        /* FIXME: If the render target gets sent to the frontBuffer should be be presenting it raw? */
        TRACE("making swapchain active\n");
        if (RenderSurface != This->renderTarget) {
            if (RenderSurface ==  swapchain->backBuffer) {
            } else {
                /* This could be flagged so that some operations work directly with the front buffer */
                FIXME("Attempting to set the  renderTarget to the frontBuffer\n");
            }
            if (glXMakeCurrent(swapchain->display, swapchain->win, swapchain->glCtx)
            == False) {
                TRACE("Error in setting current context: context %p drawable %ld !\n",
                       impSwapChain->glCtx, impSwapChain->win);
            }


#if 0 /* TODO: apply the state block to the 'possibly' new context. */
            BOOL oldRecording;
            IWineD3DStateBlockImpl *oldUpdateStateBlock;
            oldUpdateStateBlock = This->updateStateBlock;
            oldRecording= This->isRecordingState;
            This->isRecordingState = FALSE;
            This->updateStateBlock = This->stateBlock;
            IWineD3DStateBlock_Apply((IWineD3DStateBlock *)This->stateBlock);

            This->isRecordingState = oldRecording;
            This->updateStateBlock = oldUpdateStateBlock;
#endif

            IWineD3DDeviceImpl_CleanRender(iface, (IWineD3DSwapChainImpl *)currentSwapchain);
        }
        checkGLcall("glXMakeContextCurrent");

        IWineD3DSwapChain_Release((IWineD3DSwapChain *)swapchain);
    }
#if 0
    else
    if (NULL != cfgs  &&
               (((IWineD3DSwapChainImpl *)currentSwapchain)->drawable == ((IWineD3DSwapChainImpl *)currentSwapchain)->win
                ||  BackBufferFormat != ((IWineD3DSurfaceImpl *)This->renderTarget)->currentDesc.Format
                || (Width > ((IWineD3DSurfaceImpl *)This->renderTarget)->currentDesc.Width
                ||  Height > ((IWineD3DSurfaceImpl *)This->renderTarget)->currentDesc.Height))) {

        /** ********************************************************************
        * This code is far too leaky to be useful. IWineD3DDeviceImpl_CleanRender
        * doesn't seem to work properly and creating a new context every time is 'extremely' overkill.
        * The code does however work, and should be moved to a context manager to
        * manage caching of pbuffers or render to texture are appropriate.
        *
        * There are some real speed vs compatibility issues here:
        *    we should really use a new context for every texture, but that eats ram.
        *    we should also be restoring the texture to the pbuffer but that eats CPU
        *    we can also 'reuse' the current pbuffer if the size is larger than the requested buffer,
        *    but if this means reusing the display backbuffer then we need to make sure that
        *    states are correctly preserved.
        * In many cases I would expect that we can 'skip' some functions, such as preserving states,
        * and gain a good performance increase at the cost of compatibility.
        * I would suggest that, when this is the case, a user configurable flag be made
        * available, allowing the user to choose the best emulated experience for them.
         *********************************************************************/

        /**
        * TODO: support for faces of cube textures, possibly volumes
        * (this should be easy for ATI as I have examples)
        **/

        GLXContext  newContext;
        Drawable    newDrawable;
        XVisualInfo *visinfo;

        TRACE("making new buffer\n");
        nAttribs = 0;
        PUSH2(GLX_PBUFFER_WIDTH,  Width);
        PUSH2(GLX_PBUFFER_HEIGHT, Height);

#if 0 /* ATI render to texture support */
        PUSH2(GLX_LARGEST_PBUFFER, True);/* This is ignored by ATI */
        PUSH2(GLX_TEXTURE_FORMAT_ATI, GLX_TEXTURE_RGBA_ATI);
        PUSH2(GLX_TEXTURE_TARGET_ATI, cubemap? GLX_TEXTURE_CUBE_MAP_ATI : GLX_TEXTURE_2D_ATI);
        PUSH2(GLX_MIPMAP_TEXTURE_ATI, mipmapped? True : False);
#endif

#if 0
        /* TODO: discardable Pbuffer */
        PUSH2(GLX_PRESERVED_CONTENTS, FALSE);
#endif

        PUSH1(None);
        newDrawable = glXCreatePbuffer(impSwapChain->display, cfgs[0], attribs);

        /** ****************************************
        *GLX1.3 isn't supported by XFree 'yet' untill that point ATI emulates pBuffers
        *they note:
        *   In future releases, we may provide the calls glXCreateNewContext,
        *   glXQueryDrawable and glXMakeContextCurrent.
        *    so until then we have to use glXGetVisualFromFBConfig &co..
         ********************************************/


            visinfo = glXGetVisualFromFBConfig(impSwapChain->display, cfgs[0]);
            if (!visinfo) {
                ERR("Error: couldn't get an RGBA, double-buffered visual\n");
            }
            newContext = glXCreateContext(impSwapChain->display, visinfo, impSwapChain->glCtx,  GL_TRUE);
            XFree(visinfo);

        /* Make sure that the sorface exists as a glTexture */
        IWineD3DSurface_PreLoad(RenderSurface);

        newContext = glXCreateNewContext(impSwapChain->display, cfgs[0], GLX_RGBA_TYPE, impSwapChain->glCtx, TRUE);

        if (NULL == newContext) {
            ERR("cannot create glxContext\n");
        }else{
            /* Debug logging, (this function leaks), change to a TRACE when the leak is plugged */
            FIXME("Created context %p drawable %ld \n", newContext, newDrawable);

                if (glXMakeCurrent(impSwapChain->display, newDrawable, newContext) == False) {

            TRACE("Error in setting current context: context %p drawable %ld\n", newContext, newDrawable);
        }

        /* TODO: find out what of the current context needs to be coppied accross */

        checkGLcall("glXMakeContextCurrent");

        /* clean renderer should become part of the context manager so I'm not goint to put in in SwapChain just to remove it */
        IWineD3DDeviceImpl_CleanRender(iface, (IWineD3DSwapChainImpl *)currentSwapchain);
        /** TODO: We may need to copy the bits into the buffer,
        * this should !!ONLY!! be done if an operation is performed on the target
        * without it being cleared and the buffer is not discardable.
        * (basically only bother preserving the contents if there's a possibility that it will be reused)
        ** *********************************************************************/
        impSwapChain->drawable = newDrawable;
        impSwapChain->render_ctx = newContext;

        }

    }
#endif
    /* clean up the current rendertargets swapchain (if it belonged to one) */
    if (currentSwapchain != NULL) {
        IWineD3DSwapChain_Release((IWineD3DSwapChain *)currentSwapchain);
    }

    /* Were done with the opengl context management, setup the rendertargets */

    tmp = This->renderTarget;
    This->renderTarget = RenderSurface;
    IWineD3DSurface_AddRef(This->renderTarget);
    IWineD3DSurface_Release(tmp);



    {
        DWORD value;
        /* The surface must be rendered upside down to cancel the flip produce by glCopyTexImage */
        /* Check that the container is not a swapchain member */

        IWineD3DSwapChain *tmpSwapChain;
        if (D3D_OK != IWineD3DSurface_GetContainer(This->renderTarget, &IID_IWineD3DSwapChain, (void **)&tmpSwapChain)) {
            This->renderUpsideDown = TRUE;
        }else{
            This->renderUpsideDown = FALSE;
            IWineD3DSwapChain_Release(tmpSwapChain);
        }
        /* Force updating the cull mode */
        TRACE("setting render state\n");
        IWineD3DDevice_GetRenderState(iface, WINED3DRS_CULLMODE, &value);
        IWineD3DDevice_SetRenderState(iface, WINED3DRS_CULLMODE, value);

        /* Force updating projection matrix */
        This->last_was_rhw = FALSE;
        This->proj_valid = FALSE;
    }

    ret = D3D_OK;

    if (cfgs != NULL) {
        XFree(cfgs);
    } else {
        ERR("cannot get valides GLXFBConfig for (%u,%s)/(%u,%s)\n", BackBufferFormat,
            debug_d3dformat(BackBufferFormat), StencilBufferFormat, debug_d3dformat(StencilBufferFormat));
    }

#undef PUSH1
#undef PUSH2
    if ( NULL != impSwapChain) {
        IWineD3DSwapChain_Release((IWineD3DSwapChain *)impSwapChain);
    }
    LEAVE_GL();

#endif
#else

    IWineD3DSurface *tmp;
    /* This should be handled outside of this function */
    tmp = This->renderTarget;
    This->renderTarget = RenderSurface;
    IWineD3DSurface_AddRef(This->renderTarget);
    IWineD3DSurface_Release( tmp);
    tmp = This->stencilBufferTarget;
    This->stencilBufferTarget = StencilSurface;

#endif
    return ret;

}

HRESULT  WINAPI  IWineD3DDeviceImpl_SetCursorProperties(IWineD3DDevice* iface, UINT XHotSpot,
                                                        UINT YHotSpot, IWineD3DSurface *pCursorBitmap) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    /* TODO: the use of Impl is deprecated. */
    /* some basic validation checks */
    IWineD3DSurfaceImpl * pSur = (IWineD3DSurfaceImpl *) pCursorBitmap;

    TRACE("(%p) : Spot Pos(%u,%u)\n", This, XHotSpot, YHotSpot);

    if (WINED3DFMT_A8R8G8B8 != pSur->resource.format) {
      ERR("(%p) : surface(%p) has an invalid format\n", This, pCursorBitmap);
      return D3DERR_INVALIDCALL;
    }
    if (32 != pSur->currentDesc.Height || 32 != pSur->currentDesc.Width) {
      ERR("(%p) : surface(%p) has an invalid size\n", This, pCursorBitmap);
      return D3DERR_INVALIDCALL;
    }
    /* TODO: make the cursor 'real' */

    This->xHotSpot = XHotSpot;
    This->yHotSpot = YHotSpot;

    return D3D_OK;
}

void     WINAPI  IWineD3DDeviceImpl_SetCursorPosition(IWineD3DDevice* iface, int XScreenSpace, int YScreenSpace, DWORD Flags) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    TRACE("(%p) : SetPos to (%u,%u)\n", This, XScreenSpace, YScreenSpace);

    This->xScreenSpace = XScreenSpace;
    This->yScreenSpace = YScreenSpace;

    return;

}

BOOL     WINAPI  IWineD3DDeviceImpl_ShowCursor(IWineD3DDevice* iface, BOOL bShow) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    TRACE("(%p) : visible(%d)\n", This, bShow);

    This->bCursorVisible = bShow;

    return D3D_OK;
}

HRESULT  WINAPI  IWineD3DDeviceImpl_TestCooperativeLevel(IWineD3DDevice* iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    FIXME("(%p) : stub\n", This); /* No way of notifying yet! */
    return D3D_OK;
}


HRESULT  WINAPI  IWineD3DDeviceImpl_EvictManagedResources(IWineD3DDevice* iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    /** FIXME: Resource tracking needs to be done,
    * The closes we can do to this is set the priorities of all managed textures low
    * and then reset them.
     ***********************************************************/
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT  WINAPI  IWineD3DDeviceImpl_Rest(IWineD3DDevice* iface, D3DPRESENT_PARAMETERS* pPresentationParameters) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    /** FIXME: Resource trascking needs to be done.
    * in effect this pulls all non only default
    * textures out of video memory and deletes all glTextures (glDeleteTextures)
    * and should clear down the context and set it up according to pPresentationParameters
     ***********************************************************/
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_SetDialogBoxMode(IWineD3DDevice *iface, BOOL bEnableDialogs) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    /** FIXME: always true at the moment **/
    if(bEnableDialogs == FALSE) {
        FIXME("(%p) Dialogs cannot be disabled yet\n", This);
    }
    return D3D_OK;
}


HRESULT  WINAPI  IWineD3DDeviceImpl_GetCreationParameters(IWineD3DDevice *iface, D3DDEVICE_CREATION_PARAMETERS *pParameters) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;

    FIXME("(%p) : stub\n", This);
    /* Setup some reasonable defaults */
    pParameters->AdapterOrdinal = 0; /* always for now */
    pParameters->DeviceType = D3DDEVTYPE_HAL; /* always for now */
    pParameters->hFocusWindow = 0;
    pParameters->BehaviorFlags =0;
    return D3D_OK;
}

void WINAPI IWineD3DDeviceImpl_SetGammaRamp(IWineD3DDevice * iface, UINT iSwapChain, DWORD Flags, CONST D3DGAMMARAMP* pRamp) {
    IWineD3DSwapChain *swapchain;
    HRESULT hrc = D3D_OK;

    TRACE("Relaying  to swapchain\n");

    if ((hrc = IWineD3DDeviceImpl_GetSwapChain(iface, iSwapChain, &swapchain)) == D3D_OK) {
        IWineD3DSwapChain_SetGammaRamp(swapchain, Flags, (D3DGAMMARAMP *)pRamp);
        IWineD3DSwapChain_Release(swapchain);
    }
    return;
}

void WINAPI IWineD3DDeviceImpl_GetGammaRamp(IWineD3DDevice *iface, UINT iSwapChain, D3DGAMMARAMP* pRamp) {
    IWineD3DSwapChain *swapchain;
    HRESULT hrc = D3D_OK;

    TRACE("Relaying  to swapchain\n");

    if ((hrc = IWineD3DDeviceImpl_GetSwapChain(iface, iSwapChain, &swapchain)) == D3D_OK) {
        hrc =IWineD3DSwapChain_GetGammaRamp(swapchain, pRamp);
        IWineD3DSwapChain_Release(swapchain);
    }
    return;
}

/**********************************************************
 * IWineD3DDevice VTbl follows
 **********************************************************/

const IWineD3DDeviceVtbl IWineD3DDevice_Vtbl =
{
    /*** IUnknown methods ***/
    IWineD3DDeviceImpl_QueryInterface,
    IWineD3DDeviceImpl_AddRef,
    IWineD3DDeviceImpl_Release,
    /*** IWineD3DDevice methods ***/
    IWineD3DDeviceImpl_GetParent,
    /*** Creation methods**/
    IWineD3DDeviceImpl_CreateVertexBuffer,
    IWineD3DDeviceImpl_CreateIndexBuffer,
    IWineD3DDeviceImpl_CreateStateBlock,
    IWineD3DDeviceImpl_CreateSurface,
    IWineD3DDeviceImpl_CreateTexture,
    IWineD3DDeviceImpl_CreateVolumeTexture,
    IWineD3DDeviceImpl_CreateVolume,
    IWineD3DDeviceImpl_CreateCubeTexture,
    IWineD3DDeviceImpl_CreateQuery,
    IWineD3DDeviceImpl_CreateAdditionalSwapChain,
    IWineD3DDeviceImpl_CreateVertexDeclaration,
    IWineD3DDeviceImpl_CreateVertexShader,
    IWineD3DDeviceImpl_CreatePixelShader,

    /*** Odd functions **/
    IWineD3DDeviceImpl_EvictManagedResources,
    IWineD3DDeviceImpl_GetAvailableTextureMem,
    IWineD3DDeviceImpl_GetBackBuffer,
    IWineD3DDeviceImpl_GetCreationParameters,
    IWineD3DDeviceImpl_GetDeviceCaps,
    IWineD3DDeviceImpl_GetDirect3D,
    IWineD3DDeviceImpl_GetDisplayMode,
    IWineD3DDeviceImpl_GetNumberOfSwapChains,
    IWineD3DDeviceImpl_GetRasterStatus,
    IWineD3DDeviceImpl_GetSwapChain,
    IWineD3DDeviceImpl_Reset,
    IWineD3DDeviceImpl_SetDialogBoxMode,
    IWineD3DDeviceImpl_SetCursorProperties,
    IWineD3DDeviceImpl_SetCursorPosition,
    IWineD3DDeviceImpl_ShowCursor,
    IWineD3DDeviceImpl_TestCooperativeLevel,
    /*** Getters and setters **/
    IWineD3DDeviceImpl_SetClipPlane,
    IWineD3DDeviceImpl_GetClipPlane,
    IWineD3DDeviceImpl_SetClipStatus,
    IWineD3DDeviceImpl_GetClipStatus,
    IWineD3DDeviceImpl_SetCurrentTexturePalette,
    IWineD3DDeviceImpl_GetCurrentTexturePalette,
    IWineD3DDeviceImpl_SetDepthStencilSurface,
    IWineD3DDeviceImpl_GetDepthStencilSurface,
    IWineD3DDeviceImpl_SetFVF,
    IWineD3DDeviceImpl_GetFVF,
    IWineD3DDeviceImpl_SetGammaRamp,
    IWineD3DDeviceImpl_GetGammaRamp,
    IWineD3DDeviceImpl_SetIndices,
    IWineD3DDeviceImpl_GetIndices,
    IWineD3DDeviceImpl_SetLight,
    IWineD3DDeviceImpl_GetLight,
    IWineD3DDeviceImpl_SetLightEnable,
    IWineD3DDeviceImpl_GetLightEnable,
    IWineD3DDeviceImpl_SetMaterial,
    IWineD3DDeviceImpl_GetMaterial,
    IWineD3DDeviceImpl_SetNPatchMode,
    IWineD3DDeviceImpl_GetNPatchMode,
    IWineD3DDeviceImpl_SetPaletteEntries,
    IWineD3DDeviceImpl_GetPaletteEntries,
    IWineD3DDeviceImpl_SetPixelShader,
    IWineD3DDeviceImpl_GetPixelShader,
    IWineD3DDeviceImpl_SetPixelShaderConstantB,
    IWineD3DDeviceImpl_GetPixelShaderConstantB,
    IWineD3DDeviceImpl_SetPixelShaderConstantI,
    IWineD3DDeviceImpl_GetPixelShaderConstantI,
    IWineD3DDeviceImpl_SetPixelShaderConstantF,
    IWineD3DDeviceImpl_GetPixelShaderConstantF,
    IWineD3DDeviceImpl_SetRenderState,
    IWineD3DDeviceImpl_GetRenderState,
    IWineD3DDeviceImpl_SetRenderTarget,
    IWineD3DDeviceImpl_GetRenderTarget,
    IWineD3DDeviceImpl_SetSamplerState,
    IWineD3DDeviceImpl_GetSamplerState,
    IWineD3DDeviceImpl_SetScissorRect,
    IWineD3DDeviceImpl_GetScissorRect,
    IWineD3DDeviceImpl_SetSoftwareVertexProcessing,
    IWineD3DDeviceImpl_GetSoftwareVertexProcessing,
    IWineD3DDeviceImpl_SetStreamSource,
    IWineD3DDeviceImpl_GetStreamSource,
    IWineD3DDeviceImpl_SetStreamSourceFreq,
    IWineD3DDeviceImpl_GetStreamSourceFreq,
    IWineD3DDeviceImpl_SetTexture,
    IWineD3DDeviceImpl_GetTexture,
    IWineD3DDeviceImpl_SetTextureStageState,
    IWineD3DDeviceImpl_GetTextureStageState,
    IWineD3DDeviceImpl_SetTransform,
    IWineD3DDeviceImpl_GetTransform,
    IWineD3DDeviceImpl_SetVertexDeclaration,
    IWineD3DDeviceImpl_GetVertexDeclaration,
    IWineD3DDeviceImpl_SetVertexShader,
    IWineD3DDeviceImpl_GetVertexShader,
    IWineD3DDeviceImpl_SetVertexShaderConstantB,
    IWineD3DDeviceImpl_GetVertexShaderConstantB,
    IWineD3DDeviceImpl_SetVertexShaderConstantI,
    IWineD3DDeviceImpl_GetVertexShaderConstantI,
    IWineD3DDeviceImpl_SetVertexShaderConstantF,
    IWineD3DDeviceImpl_GetVertexShaderConstantF,
    IWineD3DDeviceImpl_SetViewport,
    IWineD3DDeviceImpl_GetViewport,
    IWineD3DDeviceImpl_MultiplyTransform,
    IWineD3DDeviceImpl_ValidateDevice,
    IWineD3DDeviceImpl_ProcessVertices,
    /*** State block ***/
    IWineD3DDeviceImpl_BeginStateBlock,
    IWineD3DDeviceImpl_EndStateBlock,
    /*** Scene management ***/
    IWineD3DDeviceImpl_BeginScene,
    IWineD3DDeviceImpl_EndScene,
    IWineD3DDeviceImpl_Present,
    IWineD3DDeviceImpl_Clear,
    /*** Drawing ***/
    IWineD3DDeviceImpl_DrawPrimitive,
    IWineD3DDeviceImpl_DrawIndexedPrimitive,
    IWineD3DDeviceImpl_DrawPrimitiveUP,
    IWineD3DDeviceImpl_DrawIndexedPrimitiveUP,
    IWineD3DDeviceImpl_DrawRectPatch,
    IWineD3DDeviceImpl_DrawTriPatch,
    IWineD3DDeviceImpl_DeletePatch,
    IWineD3DDeviceImpl_ColorFill,
    IWineD3DDeviceImpl_UpdateTexture,
    IWineD3DDeviceImpl_UpdateSurface,
    IWineD3DDeviceImpl_StretchRect,
    IWineD3DDeviceImpl_GetRenderTargetData,
    IWineD3DDeviceImpl_GetFrontBufferData,
    /*** Internal use IWineD3DDevice methods ***/
    IWineD3DDeviceImpl_SetupTextureStates
};


const DWORD SavedPixelStates_R[NUM_SAVEDPIXELSTATES_R] = {
    WINED3DRS_ALPHABLENDENABLE   ,
    WINED3DRS_ALPHAFUNC          ,
    WINED3DRS_ALPHAREF           ,
    WINED3DRS_ALPHATESTENABLE    ,
    WINED3DRS_BLENDOP            ,
    WINED3DRS_COLORWRITEENABLE   ,
    WINED3DRS_DESTBLEND          ,
    WINED3DRS_DITHERENABLE       ,
    WINED3DRS_FILLMODE           ,
    WINED3DRS_FOGDENSITY         ,
    WINED3DRS_FOGEND             ,
    WINED3DRS_FOGSTART           ,
    WINED3DRS_LASTPIXEL          ,
    WINED3DRS_SHADEMODE          ,
    WINED3DRS_SRCBLEND           ,
    WINED3DRS_STENCILENABLE      ,
    WINED3DRS_STENCILFAIL        ,
    WINED3DRS_STENCILFUNC        ,
    WINED3DRS_STENCILMASK        ,
    WINED3DRS_STENCILPASS        ,
    WINED3DRS_STENCILREF         ,
    WINED3DRS_STENCILWRITEMASK   ,
    WINED3DRS_STENCILZFAIL       ,
    WINED3DRS_TEXTUREFACTOR      ,
    WINED3DRS_WRAP0              ,
    WINED3DRS_WRAP1              ,
    WINED3DRS_WRAP2              ,
    WINED3DRS_WRAP3              ,
    WINED3DRS_WRAP4              ,
    WINED3DRS_WRAP5              ,
    WINED3DRS_WRAP6              ,
    WINED3DRS_WRAP7              ,
    WINED3DRS_ZENABLE            ,
    WINED3DRS_ZFUNC              ,
    WINED3DRS_ZWRITEENABLE
};

const DWORD SavedPixelStates_T[NUM_SAVEDPIXELSTATES_T] = {
    WINED3DTSS_ADDRESSW              ,
    WINED3DTSS_ALPHAARG0             ,
    WINED3DTSS_ALPHAARG1             ,
    WINED3DTSS_ALPHAARG2             ,
    WINED3DTSS_ALPHAOP               ,
    WINED3DTSS_BUMPENVLOFFSET        ,
    WINED3DTSS_BUMPENVLSCALE         ,
    WINED3DTSS_BUMPENVMAT00          ,
    WINED3DTSS_BUMPENVMAT01          ,
    WINED3DTSS_BUMPENVMAT10          ,
    WINED3DTSS_BUMPENVMAT11          ,
    WINED3DTSS_COLORARG0             ,
    WINED3DTSS_COLORARG1             ,
    WINED3DTSS_COLORARG2             ,
    WINED3DTSS_COLOROP               ,
    WINED3DTSS_RESULTARG             ,
    WINED3DTSS_TEXCOORDINDEX         ,
    WINED3DTSS_TEXTURETRANSFORMFLAGS
};

const DWORD SavedPixelStates_S[NUM_SAVEDPIXELSTATES_S] = {
    WINED3DSAMP_ADDRESSU         ,
    WINED3DSAMP_ADDRESSV         ,
    WINED3DSAMP_ADDRESSW         ,
    WINED3DSAMP_BORDERCOLOR      ,
    WINED3DSAMP_MAGFILTER        ,
    WINED3DSAMP_MINFILTER        ,
    WINED3DSAMP_MIPFILTER        ,
    WINED3DSAMP_MIPMAPLODBIAS    ,
    WINED3DSAMP_MAXMIPLEVEL      ,
    WINED3DSAMP_MAXANISOTROPY    ,
    WINED3DSAMP_SRGBTEXTURE      ,
    WINED3DSAMP_ELEMENTINDEX
};

const DWORD SavedVertexStates_R[NUM_SAVEDVERTEXSTATES_R] = {
    WINED3DRS_AMBIENT                       ,
    WINED3DRS_AMBIENTMATERIALSOURCE         ,
    WINED3DRS_CLIPPING                      ,
    WINED3DRS_CLIPPLANEENABLE               ,
    WINED3DRS_COLORVERTEX                   ,
    WINED3DRS_DIFFUSEMATERIALSOURCE         ,
    WINED3DRS_EMISSIVEMATERIALSOURCE        ,
    WINED3DRS_FOGDENSITY                    ,
    WINED3DRS_FOGEND                        ,
    WINED3DRS_FOGSTART                      ,
    WINED3DRS_FOGTABLEMODE                  ,
    WINED3DRS_FOGVERTEXMODE                 ,
    WINED3DRS_INDEXEDVERTEXBLENDENABLE      ,
    WINED3DRS_LIGHTING                      ,
    WINED3DRS_LOCALVIEWER                   ,
    WINED3DRS_MULTISAMPLEANTIALIAS          ,
    WINED3DRS_MULTISAMPLEMASK               ,
    WINED3DRS_NORMALIZENORMALS              ,
    WINED3DRS_PATCHEDGESTYLE                ,
    WINED3DRS_POINTSCALE_A                  ,
    WINED3DRS_POINTSCALE_B                  ,
    WINED3DRS_POINTSCALE_C                  ,
    WINED3DRS_POINTSCALEENABLE              ,
    WINED3DRS_POINTSIZE                     ,
    WINED3DRS_POINTSIZE_MAX                 ,
    WINED3DRS_POINTSIZE_MIN                 ,
    WINED3DRS_POINTSPRITEENABLE             ,
    WINED3DRS_RANGEFOGENABLE                ,
    WINED3DRS_SPECULARMATERIALSOURCE        ,
    WINED3DRS_TWEENFACTOR                   ,
    WINED3DRS_VERTEXBLEND
};

const DWORD SavedVertexStates_T[NUM_SAVEDVERTEXSTATES_T] = {
    WINED3DTSS_TEXCOORDINDEX         ,
    WINED3DTSS_TEXTURETRANSFORMFLAGS
};

const DWORD SavedVertexStates_S[NUM_SAVEDVERTEXSTATES_S] = {
    WINED3DSAMP_DMAPOFFSET
};
