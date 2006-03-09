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
#ifdef HAVE_FLOAT_H
# include <float.h>
#endif
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
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

/* TODO: setup some flags in the regestry to enable, disable pbuffer support */
/* enable pbuffer support for offscreen textures */
BOOL pbuffer_support     = FALSE;
/* allocate one pbuffer per surface */
BOOL pbuffer_per_surface = FALSE;

/* static function declarations */
static void WINAPI IWineD3DDeviceImpl_AddResource(IWineD3DDevice *iface, IWineD3DResource *resource);

static void WINAPI IWineD3DDeviceImpl_ApplyTextureUnitState(IWineD3DDevice *iface, DWORD Stage, WINED3DTEXTURESTAGESTATETYPE Type);

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
            HeapFree(GetProcessHeap(), 0, object); \
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
    IWineD3DDeviceImpl_AddResource(iface, (IWineD3DResource *)object) ;\
    TRACE("(%p) : Created resource %p\n", This, object); \
}

#define D3DINITIALIZEBASETEXTURE(_basetexture) { \
    _basetexture.levels     = Levels; \
    _basetexture.filterType = (Usage & WINED3DUSAGE_AUTOGENMIPMAP) ? D3DTEXF_LINEAR : D3DTEXF_NONE; \
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
static void setup_light(IWineD3DDevice *iface, LONG Index, PLIGHTINFOEL *lightInfo) {

    float quad_att;
    float colRGBA[] = {0.0, 0.0, 0.0, 0.0};
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    /* Light settings are affected by the model view in OpenGL, the View transform in direct3d*/
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixf((float *)&This->stateBlock->transforms[D3DTS_VIEW].u.m[0][0]);

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

    if ((lightInfo->OriginalParms.Range *lightInfo->OriginalParms.Range) >= FLT_MIN) {
        quad_att = 1.4/(lightInfo->OriginalParms.Range *lightInfo->OriginalParms.Range);
    } else {
        quad_att = 0; /*  0 or  MAX?  (0 seems to be ok) */
    }

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
void WINAPI IWineD3DDeviceImpl_SetupTextureStates(IWineD3DDevice *iface, DWORD Sampler, DWORD Flags) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    float col[4];

    union {
        float f;
        DWORD d;
    } tmpvalue;

    /* In addition, IDirect3DDevice9::SetSamplerState will now be used for filtering, tiling,
    clamping, MIPLOD, etc. This will work for up to 16 samplers.
    */
   
    if (Sampler >= GL_LIMITS(samplers)) {
        FIXME("Trying to set the state of more samplers %ld than are supported %d by this openGL implementation\n", Sampler, GL_LIMITS(samplers));
        return;
    }
    VTRACE(("Activating appropriate texture state %ld\n", Sampler));
    if (GL_SUPPORT(ARB_MULTITEXTURE)) {
        ENTER_GL();
        GLACTIVETEXTURE(Sampler);
        LEAVE_GL();
        /* Could we use bindTexture and then apply the states instead of GLACTIVETEXTURE */
    } else if (Sampler > 0) {
        FIXME("Program using multiple concurrent textures which this opengl implementation doesn't support\n");
        return;
    }

    /* TODO: change this to a lookup table
        LOOKUP_TEXTURE_STATES lists all texture states that should be applied.
        LOOKUP_CONTEXT_SATES list all context applicable states that can be applied
        etc.... it's a lot cleaner, quicker and possibly easier to maintain than running a switch and setting a skip flag...
        especially when there are a number of groups of states. */

    TRACE("-----------------------> Updating the texture at Sampler %ld to have new texture state information\n", Sampler);

    /* The list of states not to apply is a big as the list of states to apply, so it makes sense to produce an inclusive list  */
#define APPLY_STATE(_state)     IWineD3DDeviceImpl_ApplyTextureUnitState(iface, Sampler, _state)
/* these are the only two supported states that need to be applied */
    APPLY_STATE(WINED3DTSS_TEXCOORDINDEX);
    APPLY_STATE(WINED3DTSS_TEXTURETRANSFORMFLAGS);
#if 0 /* not supported at the moment */
    APPLY_STATE(WINED3DTSS_BUMPENVMAT00);
    APPLY_STATE(WINED3DTSS_BUMPENVMAT01);
    APPLY_STATE(WINED3DTSS_BUMPENVMAT10);
    APPLY_STATE(WINED3DTSS_BUMPENVMAT11);
    APPLY_STATE(WINED3DTSS_BUMPENVLSCALE);
    APPLY_STATE(WINED3DTSS_BUMPENVLOFFSET);
    APPLY_STATE(WINED3DTSS_RESULTARG);
    APPLY_STATE(WINED3DTSS_CONSTANT);
#endif
    /* a quick sanity check in case someone forgot to update this function */
    if (WINED3D_HIGHEST_TEXTURE_STATE > WINED3DTSS_CONSTANT) {
        FIXME("(%p) : There are more texture states than expected, update device.c to match\n", This);
    }
#undef APPLY_STATE

    /* apply any sampler states that always need applying */
    if (GL_SUPPORT(EXT_TEXTURE_LOD_BIAS)) {
        tmpvalue.d = This->stateBlock->samplerState[Sampler][WINED3DSAMP_MIPMAPLODBIAS];
        glTexEnvf(GL_TEXTURE_FILTER_CONTROL_EXT,
                GL_TEXTURE_LOD_BIAS_EXT,
                tmpvalue.f);
        checkGLcall("glTexEnvi GL_TEXTURE_LOD_BIAS_EXT ...");
    }

    /* Note the D3DRS value applies to all textures, but GL has one
     *  per texture, so apply it now ready to be used!
     */
    D3DCOLORTOGLFLOAT4(This->stateBlock->renderState[WINED3DRS_TEXTUREFACTOR], col);
    /* Set the default alpha blend color */
    glBlendColor(col[0], col[1], col[2], col[3]);
    checkGLcall("glBlendColor");

    D3DCOLORTOGLFLOAT4(This->stateBlock->renderState[WINED3DRS_TEXTUREFACTOR], col);
    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, &col[0]);
    checkGLcall("glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);");

    /* TODO: NV_POINT_SPRITE */
    if (GL_SUPPORT(ARB_POINT_SPRITE)) {
        if (This->stateBlock->renderState[WINED3DRS_POINTSPRITEENABLE] != FALSE) {
           /* Doesn't work with GL_POINT_SMOOTH on on my ATI 9600, but then ATI drivers are buggered! */
           glDisable(GL_POINT_SMOOTH);

           /* Centre the texture on the vertex */
           VTRACE("glTexEnvf( GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE)\n");
           glTexEnvf( GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);

           VTRACE("glTexEnvf( GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE)\n");
           glTexEnvf( GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);
           checkGLcall("glTexEnvf(...)");
           VTRACE("glEnable( GL_POINT_SPRITE_ARB )\n");
           glEnable( GL_POINT_SPRITE_ARB );
           checkGLcall("glEnable(...)");
        } else {
           VTRACE("glDisable( GL_POINT_SPRITE_ARB )\n");
           glDisable( GL_POINT_SPRITE_ARB );
           checkGLcall("glEnable(...)");
        }
    }

    TRACE("-----------------------> Updated the texture at Sampler %ld to have new texture state information\n", Sampler);
}

/**********************************************************
 * IUnknown parts follows
 **********************************************************/

HRESULT WINAPI IWineD3DDeviceImpl_QueryInterface(IWineD3DDevice *iface,REFIID riid,LPVOID *ppobj)
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DBase)
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
        if (((IWineD3DImpl *)This->wineD3D)->dxVersion >= 8) { /*We don't create state blocks for d3d7 yet*/
            /* NOTE: You must release the parent if the object was created via a callback
            ** ***************************/
            /* TODO: this is exactly the same as required by _Reset.., so at some point we need to move the code so that is can be called by reset add release...  */
            /* Release all of the swapchains, except the implicit swapchain */
            IUnknown* stencilBufferParent;
            IUnknown* swapChainParent;

            /* NOTE: Don't release swapchain 0 here, it's 'special' */
            SwapChainList *nextSwapchain = This->swapchains;
            if (nextSwapchain != NULL) {
                nextSwapchain = nextSwapchain->next;
            } else {
                WARN("Expected to find the implicit swapchain\n");
            }

            /* release all the other swapchains */
            while (nextSwapchain != NULL) {
                SwapChainList *prevSwapchain = nextSwapchain;
                nextSwapchain = nextSwapchain->next;
                IWineD3DSwapChain_Release(prevSwapchain->swapchain);
                /* NOTE: no need to free the list element, it will be done by the release callback
                   HeapFree(GetProcessHeap(), 0, prevSwapchain); */
            }
             /* Release the buffers (with sanity checks)*/
            if(This->stencilBufferTarget != NULL && (IWineD3DSurface_Release(This->stencilBufferTarget) >0)){
                if(This->depthStencilBuffer != This->stencilBufferTarget)
                    FIXME("(%p) Something's still holding the depthStencilBuffer\n",This);
            }
            This->stencilBufferTarget = NULL;

            if(IWineD3DSurface_Release(This->renderTarget) >0){
                 /* This check is a bit silly, itshould be in swapchain_release FIXME("(%p) Something's still holding the renderTarget\n",This); */
            }
            This->renderTarget = NULL;

            IWineD3DSurface_GetParent(This->depthStencilBuffer, &stencilBufferParent);
            IUnknown_Release(stencilBufferParent);          /* once for the get parent */
            if(IUnknown_Release(stencilBufferParent)  >0){  /* the second time for when it was created */
                FIXME("(%p) Something's still holding the depthStencilBuffer\n",This);
            }
            This->depthStencilBuffer = NULL;

            /* Release the update stateblock */
            if(IWineD3DStateBlock_Release((IWineD3DStateBlock *)This->updateStateBlock) > 0){
                if(This->updateStateBlock != This->stateBlock)
                    FIXME("(%p) Something's still holding the Update stateblock\n",This);
            }
            This->updateStateBlock = NULL;
            { /* because were not doing proper internal refcounts releasing the primary state block
                causes recursion with the extra checks in ResourceReleased, to avoid this we have
                to set this->stateBlock = NULL; first */
                IWineD3DStateBlock *stateBlock = (IWineD3DStateBlock *)This->stateBlock;
                This->stateBlock = NULL;

                /* Release the stateblock */
                if(IWineD3DStateBlock_Release(stateBlock) > 0){
                        FIXME("(%p) Something's still holding the Update stateblock\n",This);
                }
            }

            if (This->swapchains != NULL) {
                /* Swapchain 0 is special because it's created in startup with a hanging parent, so we have to release its parent now */
                IWineD3DSwapChain_GetParent(This->swapchains->swapchain, &swapChainParent);
                IUnknown_Release(swapChainParent);           /* once for the get parent */
                if (IUnknown_Release(swapChainParent)  > 0) {  /* the second time for when it was created */
                    FIXME("(%p) Something's still holding the implicit swapchain\n", This);
                }
            }

            if (This->resources != NULL ) {
                FIXME("(%p) Device released with resources still bound, acceptable but unexpected\n", This);

#if 0           /* TODO: Dump a list of all the resources still bound */
                dumpResources(This->resources);
#endif
                /* TODO: set the resources to a lost state */
            }

        }
        IWineD3D_Release(This->wineD3D);
        This->wineD3D = NULL;
        HeapFree(GetProcessHeap(), 0, This);
        TRACE("Freed device  %p\n", This);
        This = NULL;
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
    D3DCREATERESOURCEOBJECTINSTANCE(object, VertexBuffer, WINED3DRTYPE_VERTEXBUFFER, Size)
    
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
    D3DCREATERESOURCEOBJECTINSTANCE(object,IndexBuffer,WINED3DRTYPE_INDEXBUFFER, Length)
    
    /*TODO: use VBO's */
    if (Pool == D3DPOOL_DEFAULT ) { /* Allocate some system memory for now */
        object->resource.allocatedMemory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,object->resource.size);
    }

    TRACE("(%p) : Len=%d, Use=%lx, Format=(%u,%s), Pool=%d - Memory@%p, Iface@%p\n", This, Length, Usage, Format, 
                           debug_d3dformat(Format), Pool, object, object->resource.allocatedMemory);
    *ppIndexBuffer = (IWineD3DIndexBuffer *) object;

    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_CreateStateBlock(IWineD3DDevice* iface, WINED3DSTATEBLOCKTYPE Type, IWineD3DStateBlock** ppStateBlock, IUnknown *parent) {

    IWineD3DDeviceImpl     *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DStateBlockImpl *object;
    int i, j;

    D3DCREATEOBJECTINSTANCE(object, StateBlock)
    object->blockType     = Type;

    /* Special case - Used during initialization to produce a placeholder stateblock
          so other functions called can update a state block                         */
    if (Type == WINED3DSBT_INIT) {
        /* Don't bother increasing the reference count otherwise a device will never
           be freed due to circular dependencies                                   */
        return D3D_OK;
    }

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

    if (Type == WINED3DSBT_ALL) {

        TRACE("ALL => Pretend everything has changed\n");
        memset(&object->changed, TRUE, sizeof(This->stateBlock->changed));
    } else if (Type == WINED3DSBT_PIXELSTATE) {

        TRACE("PIXELSTATE => Pretend all pixel shates have changed\n");
        memset(&object->changed, FALSE, sizeof(This->stateBlock->changed));

        object->changed.pixelShader = TRUE;

        /* Pixel Shader Constants */
        for (i = 0; i < MAX_PSHADER_CONSTANTS; ++i) {
            object->changed.pixelShaderConstants[i] = TRUE;
        }
        for (i = 0; i < NUM_SAVEDPIXELSTATES_R; i++) {
            object->changed.renderState[SavedPixelStates_R[i]] = TRUE;
        }
        for (j = 0; j < GL_LIMITS(textures); j++) {
            for (i = 0; i < NUM_SAVEDPIXELSTATES_T; i++) {
                object->changed.textureState[j][SavedPixelStates_T[i]] = TRUE;
            }
        }
        for (j = 0 ; j < 16; j++) {
            for (i =0; i < NUM_SAVEDPIXELSTATES_S;i++) {

                object->changed.samplerState[j][SavedPixelStates_S[i]] = TRUE;
            }
        }

    } else if (Type == WINED3DSBT_VERTEXSTATE) {

        TRACE("VERTEXSTATE => Pretend all vertex shates have changed\n");
        memset(&object->changed, FALSE, sizeof(This->stateBlock->changed));

        object->changed.vertexShader = TRUE;

        /* Vertex Shader Constants */
        for (i = 0; i < MAX_VSHADER_CONSTANTS; ++i) {
            object->changed.vertexShaderConstants[i] = TRUE;
        }
        for (i = 0; i < NUM_SAVEDVERTEXSTATES_R; i++) {
            object->changed.renderState[SavedVertexStates_R[i]] = TRUE;
        }
        for (j = 0; j < GL_LIMITS(textures); j++) {
            for (i = 0; i < NUM_SAVEDVERTEXSTATES_T; i++) {
                object->changed.textureState[j][SavedVertexStates_T[i]] = TRUE;
            }
        }
        for (j = 0 ; j < 16; j++){
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
 
HRESULT  WINAPI IWineD3DDeviceImpl_CreateSurface(IWineD3DDevice *iface, UINT Width, UINT Height, WINED3DFORMAT Format, BOOL Lockable, BOOL Discard, UINT Level, IWineD3DSurface **ppSurface,WINED3DRESOURCETYPE Type, DWORD Usage, D3DPOOL Pool, D3DMULTISAMPLE_TYPE MultiSample ,DWORD MultisampleQuality, HANDLE* pSharedHandle, IUnknown *parent) {
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
        FIXME("Invalid multisample level %ld\n", MultisampleQuality);
        return D3DERR_INVALIDCALL; /* TODO: Check that this is the case! */
    }

    if(MultisampleQuality > 0) {
        FIXME("MultisampleQuality set to %ld, substituting 0\n", MultisampleQuality);
        MultisampleQuality=0;
    }

    /** FIXME: Check that the format is supported
    *    by the device.
      *******************************/

    /* Non-power2 support */

    /* Find the nearest pow2 match */
    pow2Width = pow2Height = 1;
    while (pow2Width < Width) pow2Width <<= 1;
    while (pow2Height < Height) pow2Height <<= 1;

    if (pow2Width > Width || pow2Height > Height) {
         /** TODO: add support for non power two compressed textures (OpenGL 2 provices support for * non-power-two textures gratis) **/
        if (Format == WINED3DFMT_DXT1 || Format == WINED3DFMT_DXT2 || Format == WINED3DFMT_DXT3
               || Format == WINED3DFMT_DXT4 || Format == WINED3DFMT_DXT5) {
            FIXME("(%p) Compressed non-power-two textures are not supported w(%d) h(%d)\n",
                    This, Width, Height);
            return D3DERR_NOTAVAILABLE;
        }
    }

    /** Check against the maximum texture sizes supported by the video card **/
    if (pow2Width > GL_LIMITS(texture_size) || pow2Height > GL_LIMITS(texture_size)) {
        /* one of three options
        1: Do the same as we do with nonpow 2 and scale the texture, (any texture ops would require the texture to be scaled which is potentially slow)
        2: Set the texture to the maxium size (bad idea)
        3:    WARN and return D3DERR_NOTAVAILABLE;
        */
        WARN("(%p) Application requested a surface w %d, h %d, but the graphics card only supports %d\n", This, Width, Height, GL_LIMITS(texture_size));
         return D3DERR_NOTAVAILABLE;
    }



    /** DXTn mipmaps use the same number of 'levels' down to eg. 8x1, but since
     *  it is based around 4x4 pixel blocks it requires padding, so allocate enough
     *  space!
      *********************************/
    if (WINED3DFMT_UNKNOWN == Format) {
        Size = 0;
    } else if (Format == WINED3DFMT_DXT1) {
        /* DXT1 is half byte per pixel */
       Size = ((max(pow2Width,4) * D3DFmtGetBpp(This, Format)) * max(pow2Height,4)) >> 1;

    } else if (Format == WINED3DFMT_DXT2 || Format == WINED3DFMT_DXT3 ||
               Format == WINED3DFMT_DXT4 || Format == WINED3DFMT_DXT5) {
       Size = ((max(pow2Width,4) * D3DFmtGetBpp(This, Format)) * max(pow2Height,4));
    } else {
       Size = (pow2Width * D3DFmtGetBpp(This, Format)) * pow2Height;
    }

    /** Create and initialise the surface resource **/
    D3DCREATERESOURCEOBJECTINSTANCE(object,Surface,WINED3DRTYPE_SURFACE, Size)
    /* "Standalone" surface */
    IWineD3DSurface_SetContainer((IWineD3DSurface *)object, NULL);

    object->currentDesc.Width      = Width;
    object->currentDesc.Height     = Height;
    object->currentDesc.MultiSampleType    = MultiSample;
    object->currentDesc.MultiSampleQuality = MultisampleQuality;

    /* Setup some glformat defaults */
    if (WINED3DFMT_UNKNOWN != Format) {
        object->glDescription.glFormat         = D3DFmt2GLFmt(This, object->resource.format);
        object->glDescription.glFormatInternal = D3DFmt2GLIntFmt(This, object->resource.format);
        object->glDescription.glType           = D3DFmt2GLType(This, object->resource.format);
    } else {
        object->glDescription.glFormat         = 0;
        object->glDescription.glFormatInternal = 0;
        object->glDescription.glType           = 0;
    }

    object->glDescription.textureName      = 0;
    object->glDescription.level            = Level;
    object->glDescription.target           = GL_TEXTURE_2D;

    /* Internal data */
    object->pow2Width  = pow2Width;
    object->pow2Height = pow2Height;
    object->nonpow2    = (pow2Width != Width || pow2Height != Height) ? TRUE : FALSE;
    object->discard    = Discard;
    object->activeLock = FALSE;

    if (WINED3DFMT_UNKNOWN != Format) {
        object->bytesPerPixel = D3DFmtGetBpp(This, Format);
        object->pow2Size      = (pow2Width * object->bytesPerPixel) * pow2Height;
    } else {
        object->bytesPerPixel = 0;
        object->pow2Size      = 0;
    }

    /** TODO: change this into a texture transform matrix so that it's processed in hardware **/

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
        if(Usage == WINED3DUSAGE_DYNAMIC) FIXME("Create surface called with a pool of MANAGED and a \
                                                Usage of DYNAMIC which are mutually exclusive, not doing \
                                                anything just telling you.\n");
    break;
    case D3DPOOL_DEFAULT: /*TODO: Create offscreen plain can cause this check to fail..., find out if it should */
        if(!(Usage & WINED3DUSAGE_DYNAMIC) && !(Usage & WINED3DUSAGE_RENDERTARGET)
           && !(Usage && WINED3DUSAGE_DEPTHSTENCIL ) && Lockable == TRUE)
            FIXME("Creating a surface with a POOL of DEFAULT with Locable true, that doesn't specify DYNAMIC usage.\n");
    break;
    default:
        FIXME("(%p) Unknown pool %d\n", This, Pool);
    break;
    };

    if (Usage & WINED3DUSAGE_RENDERTARGET && Pool != D3DPOOL_DEFAULT) {
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
    unsigned int pow2Width  = Width;
    unsigned int pow2Height = Height;


    TRACE("(%p), Width(%d) Height(%d) Levels(%d) Usage(%ld) ....\n", This, Width, Height, Levels, Usage);

    /* TODO: It should only be possible to create textures for formats 
             that are reported as supported */
    if (WINED3DFMT_UNKNOWN >= Format) {
        WARN("(%p) : Texture cannot be created with a format of D3DFMT_UNKNOWN\n", This);
        return D3DERR_INVALIDCALL;
    }

    D3DCREATERESOURCEOBJECTINSTANCE(object, Texture, WINED3DRTYPE_TEXTURE, 0);
    D3DINITIALIZEBASETEXTURE(object->baseTexture);    
    object->width  = Width;
    object->height = Height;

    /** Non-power2 support **/
    /* Find the nearest pow2 match */
    pow2Width = pow2Height = 1;
    while (pow2Width < Width) pow2Width <<= 1;
    while (pow2Height < Height) pow2Height <<= 1;

    /** FIXME: add support for real non-power-two if it's provided by the video card **/
    /* Precalculated scaling for 'faked' non power of two texture coords */
    object->pow2scalingFactorX  =  (((float)Width)  / ((float)pow2Width));
    object->pow2scalingFactorY  =  (((float)Height) / ((float)pow2Height));
    TRACE(" xf(%f) yf(%f)\n", object->pow2scalingFactorX, object->pow2scalingFactorY);

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
        if (hr!= D3D_OK) {
            int j;
            FIXME("Failed to create surface  %p\n", object);
            /* clean up */
            for (j = 0 ; j < i ; j++) {
                IWineD3DSurface_Release(object->surfaces[j]);
            }
            /* heap free object */
            HeapFree(GetProcessHeap(), 0, object);

            *ppTexture = NULL;
            return hr;
        }

        IWineD3DSurface_SetContainer(object->surfaces[i], (IWineD3DBase *)object);
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
                                                      IWineD3DVolumeTexture **ppVolumeTexture,
                                                      HANDLE *pSharedHandle, IUnknown *parent,
                                                      D3DCB_CREATEVOLUMEFN D3DCB_CreateVolume) {

    IWineD3DDeviceImpl        *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DVolumeTextureImpl *object;
    unsigned int               i;
    UINT                       tmpW;
    UINT                       tmpH;
    UINT                       tmpD;

    /* TODO: It should only be possible to create textures for formats 
             that are reported as supported */
    if (WINED3DFMT_UNKNOWN >= Format) {
        WARN("(%p) : Texture cannot be created with a format of D3DFMT_UNKNOWN\n", This);
        return D3DERR_INVALIDCALL;
    }

    D3DCREATERESOURCEOBJECTINSTANCE(object, VolumeTexture, WINED3DRTYPE_VOLUMETEXTURE, 0);
    D3DINITIALIZEBASETEXTURE(object->baseTexture);

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

        /* Set it's container to this object */
        IWineD3DVolume_SetContainer(object->volumes[i], (IWineD3DBase *)object);

        /* calcualte the next mipmap level */
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

    D3DCREATERESOURCEOBJECTINSTANCE(object, Volume, WINED3DRTYPE_VOLUME, ((Width * D3DFmtGetBpp(This, Format)) * Height * Depth))

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
    object->dirty               = TRUE;

    return IWineD3DVolume_AddDirtyBox((IWineD3DVolume *) object, NULL);
}

HRESULT WINAPI IWineD3DDeviceImpl_CreateCubeTexture(IWineD3DDevice *iface, UINT EdgeLength,
                                                    UINT Levels, DWORD Usage,
                                                    WINED3DFORMAT Format, D3DPOOL Pool,
                                                    IWineD3DCubeTexture **ppCubeTexture,
                                                    HANDLE *pSharedHandle, IUnknown *parent,
                                                    D3DCB_CREATESURFACEFN D3DCB_CreateSurface) {

    IWineD3DDeviceImpl      *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DCubeTextureImpl *object; /** NOTE: impl ref allowed since this is a create function **/
    unsigned int             i, j;
    UINT                     tmpW;
    HRESULT                  hr;
    unsigned int pow2EdgeLength  = EdgeLength;

    /* TODO: It should only be possible to create textures for formats 
             that are reported as supported */
    if (WINED3DFMT_UNKNOWN >= Format) {
        WARN("(%p) : Texture cannot be created with a format of D3DFMT_UNKNOWN\n", This);
        return D3DERR_INVALIDCALL;
    }

    D3DCREATERESOURCEOBJECTINSTANCE(object, CubeTexture, WINED3DRTYPE_CUBETEXTURE, 0);
    D3DINITIALIZEBASETEXTURE(object->baseTexture);

    TRACE("(%p) Create Cube Texture\n", This);

    /** Non-power2 support **/

    /* Find the nearest pow2 match */
    pow2EdgeLength = 1;
    while (pow2EdgeLength < EdgeLength) pow2EdgeLength <<= 1;

    object->edgeLength           = EdgeLength;
    /* TODO: support for native non-power 2 */
    /* Precalculated scaling for 'faked' non power of two texture coords */
    object->pow2scalingFactor    = ((float)EdgeLength) / ((float)pow2EdgeLength);

    /* Calculate levels for mip mapping */
    if (Levels == 0) {
        object->baseTexture.levels++;
        tmpW = EdgeLength;
        while (tmpW > 1) {
            tmpW = max(1, tmpW >> 1);
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
                for (l = 0; l < j; l++) {
                    IWineD3DSurface_Release(object->surfaces[j][i]);
                }
                for (k = 0; k < i; k++) {
                    for (l = 0; l < 6; l++) {
                    IWineD3DSurface_Release(object->surfaces[l][j]);
                    }
                }

                FIXME("(%p) Failed to create surface\n",object);
                HeapFree(GetProcessHeap(),0,object);
                *ppCubeTexture = NULL;
                return hr;
            }
            IWineD3DSurface_SetContainer(object->surfaces[j][i], (IWineD3DBase *)object);
            TRACE("Created surface level %d @ %p,\n", i, object->surfaces[j][i]);
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

    if (NULL == ppQuery) {
        /* Just a check to see if we support this type of query */
        HRESULT hr = D3DERR_NOTAVAILABLE;
        switch(Type) {
        case WINED3DQUERYTYPE_OCCLUSION:
            TRACE("(%p) occlusion query\n", This);
            if (GL_SUPPORT(ARB_OCCLUSION_QUERY) || GL_SUPPORT(NV_OCCLUSION_QUERY))
                hr = D3D_OK;
            else
                WARN("Unsupported in local OpenGL implementation: ARB_OCCLUSION_QUERY/NV_OCCLUSION_QUERY\n");
            break;
        case WINED3DQUERYTYPE_VCACHE:
        case WINED3DQUERYTYPE_RESOURCEMANAGER:
        case WINED3DQUERYTYPE_VERTEXSTATS:
        case WINED3DQUERYTYPE_EVENT:
        case WINED3DQUERYTYPE_TIMESTAMP:
        case WINED3DQUERYTYPE_TIMESTAMPDISJOINT:
        case WINED3DQUERYTYPE_TIMESTAMPFREQ:
        case WINED3DQUERYTYPE_PIPELINETIMINGS:
        case WINED3DQUERYTYPE_INTERFACETIMINGS:
        case WINED3DQUERYTYPE_VERTEXTIMINGS:
        case WINED3DQUERYTYPE_PIXELTIMINGS:
        case WINED3DQUERYTYPE_BANDWIDTHTIMINGS:
        case WINED3DQUERYTYPE_CACHEUTILIZATION:
        default:
            FIXME("(%p) Unhandled query type %d\n", This, Type);
        }
        return hr;
    }

    D3DCREATEOBJECTINSTANCE(object, Query)
    object->type         = Type;
    /* allocated the 'extended' data based on the type of query requested */
    switch(Type){
    case D3DQUERYTYPE_OCCLUSION:
        if(GL_SUPPORT(ARB_OCCLUSION_QUERY) || GL_SUPPORT(NV_OCCLUSION_QUERY)) {
            TRACE("(%p) Allocating data for an occlusion query\n", This);
            object->extendedData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WineQueryOcclusionData));
            break;
        }
    case D3DQUERYTYPE_VCACHE:
    case D3DQUERYTYPE_RESOURCEMANAGER:
    case D3DQUERYTYPE_VERTEXSTATS:
    case D3DQUERYTYPE_EVENT:
    case D3DQUERYTYPE_TIMESTAMP:
    case D3DQUERYTYPE_TIMESTAMPDISJOINT:
    case D3DQUERYTYPE_TIMESTAMPFREQ:
    case D3DQUERYTYPE_PIPELINETIMINGS:
    case D3DQUERYTYPE_INTERFACETIMINGS:
    case D3DQUERYTYPE_VERTEXTIMINGS:
    case D3DQUERYTYPE_PIXELTIMINGS:
    case D3DQUERYTYPE_BANDWIDTHTIMINGS:
    case D3DQUERYTYPE_CACHEUTILIZATION:
    default:
        object->extendedData = 0;
        FIXME("(%p) Unhandled query type %d\n",This , Type);
    }
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
   * or does the swap chain notify the device of its destruction.
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
    TRACE("Using a display of %p %p\n", object->display, hDc);

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
        for (n = 0;n < num; n++) {
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
   * so we should really check to see if there is a fullscreen swapchain already
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
    object->presentParms.MultiSampleQuality             = NULL == pPresentationParameters->MultiSampleQuality ? 0 : *(pPresentationParameters->MultiSampleQuality);
    object->presentParms.SwapEffect                     = *(pPresentationParameters->SwapEffect);
    object->presentParms.hDeviceWindow                  = *(pPresentationParameters->hDeviceWindow);
    object->presentParms.Windowed                       = *(pPresentationParameters->Windowed);
    object->presentParms.EnableAutoDepthStencil         = *(pPresentationParameters->EnableAutoDepthStencil);
    object->presentParms.AutoDepthStencilFormat         = *(pPresentationParameters->AutoDepthStencilFormat);
    object->presentParms.Flags                          = *(pPresentationParameters->Flags);
    object->presentParms.FullScreen_RefreshRateInHz     = *(pPresentationParameters->FullScreen_RefreshRateInHz);
    object->presentParms.PresentationInterval           = *(pPresentationParameters->PresentationInterval);


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
        IWineD3DSurface_SetContainer(object->frontBuffer, (IWineD3DBase *)object);
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
        IWineD3DSurface_SetContainer(object->backBuffer, (IWineD3DBase *)object);

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
                IWineD3DSurface_SetContainer(This->depthStencilBuffer, 0);
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

    if (hr == D3D_OK) {
        /*********************
         * Setup some defaults and clear down the buffers
         *******************/
        ENTER_GL();
        /** save current context and drawable **/
        oldContext  = glXGetCurrentContext();
        oldDrawable = glXGetCurrentDrawable();

        TRACE("Activating context (display %p context %p drawable %ld)!\n", object->display, object->glCtx, object->win);
        if (glXMakeCurrent(object->display, object->win, object->glCtx) == False) {
            ERR("Error in setting current context (display %p context %p drawable %ld)!\n", object->display, object->glCtx, object->win);
        }
        checkGLcall("glXMakeCurrent");

        TRACE("Setting up the screen\n");
        /* Clear the screen */
        glClearColor(1.0, 0.0, 0.0, 0.0);
        checkGLcall("glClearColor");
        glClearIndex(0);
        glClearDepth(1);
        glClearStencil(0xffff);

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

        /* switch back to the original context (if there was one)*/
        if (This->swapchains != NULL) {
            /** TODO: restore the context and drawable **/
            glXMakeCurrent(object->display, oldDrawable, oldContext);
        }

        LEAVE_GL();

        { /* Finally add the swapchain to the end of the devices' swapchain list */
            SwapChainList **nextSwapchain;
            nextSwapchain = &This->swapchains;
            while (*nextSwapchain != NULL) {
                nextSwapchain = &((*nextSwapchain)->next);
            }
            (*nextSwapchain) = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*This->swapchains));
            (*nextSwapchain)->swapchain = (IWineD3DSwapChain *)object;
        }
        TRACE("Set swapchain to %p\n", object);
    } else { /* something went wrong so clean up */
        IUnknown* bufferParent;
        if (object->frontBuffer) {

            IWineD3DSurface_GetParent(object->frontBuffer, &bufferParent);
            IUnknown_Release(bufferParent); /* once for the get parent */
            if (IUnknown_Release(bufferParent) > 0) {
                FIXME("(%p) Something's still holding the front buffer\n",This);
            }
        }
        if (object->backBuffer) {
            IWineD3DSurface_GetParent(object->backBuffer, &bufferParent);
            IUnknown_Release(bufferParent); /* once for the get parent */
            if (IUnknown_Release(bufferParent) > 0) {
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
    unsigned int numberOfSwapChains = 0;
    SwapChainList          *swapchain;

    swapchain = This->swapchains;
    /* itterate through the list to get a count */
    while (swapchain != NULL) {
        swapchain = swapchain->next;
        numberOfSwapChains++;
    }

    TRACE("(%p) returning %d\n", This, numberOfSwapChains);
    return numberOfSwapChains;
}

HRESULT  WINAPI  IWineD3DDeviceImpl_GetSwapChain(IWineD3DDevice *iface, UINT iSwapChain, IWineD3DSwapChain **pSwapChain) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    SwapChainList *swapchain;
    int i = iSwapChain;
    HRESULT hr = D3DERR_INVALIDCALL;
    swapchain = This->swapchains;
    TRACE("(%p) : swapchain %d\n", This, iSwapChain);


    TRACE("(%p) Finding swapchain %d\n", This, iSwapChain);
    while (i > 0 && swapchain != NULL) {
        swapchain = swapchain->next;
        --i;
    }

    if (i > 0) {
        FIXME("(%p) Unable to find swapchain %d\n", This, iSwapChain);
        *pSwapChain = NULL;
    } else if (swapchain != NULL) {
        /** TODO: move off to a linkesList implementation **/
        *pSwapChain = swapchain->swapchain;
        IWineD3DSwapChain_AddRef(*pSwapChain);
        hr = D3D_OK;
    }

    TRACE("(%p) returning %p\n", This, *pSwapChain);
    return hr;
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
HRESULT WINAPI IWineD3DDeviceImpl_CreateVertexShader(IWineD3DDevice *iface, CONST DWORD *pDeclaration, CONST DWORD *pFunction, IWineD3DVertexShader **ppVertexShader, IUnknown *parent) {
    IWineD3DDeviceImpl       *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DVertexShaderImpl *object;  /* NOTE: impl usage is ok, this is a create */
    HRESULT hr = D3D_OK;
    D3DCREATEOBJECTINSTANCE(object, VertexShader)

    TRACE("(%p) : Created Vertex shader %p\n", This, *ppVertexShader);
    hr = IWineD3DVertexShader_SetFunction(*ppVertexShader, pFunction);

    if (D3D_OK != hr) {
        FIXME("(%p) : Failed to set the function, returning D3DERR_INVALIDCALL\n", iface);
        IWineD3DVertexShader_Release(*ppVertexShader);
        return D3DERR_INVALIDCALL;
    }

#if 0 /* TODO: In D3D* SVP is atatched to the shader, in D3D9 it's attached to the device and isn't stored in the stateblock. */
    if(Usage == WINED3DUSAGE_SOFTWAREVERTEXPROCESSING) {
        /* Foo */
    } else {
        /* Bar */
    }

#endif


    /* If a vertex declaration has been passed, save it to the vertex shader, this affects d3d8 only. */
    if (pDeclaration != NULL) {
        IWineD3DVertexDeclaration *vertexDeclaration;
        hr = IWineD3DDevice_CreateVertexDeclaration(iface, pDeclaration, &vertexDeclaration ,NULL);
        if (D3D_OK == hr) {
            TRACE("(%p) : Setting vertex declaration to %p\n", This, vertexDeclaration);
            object->vertexDeclaration = vertexDeclaration;
        } else {
            FIXME("(%p) : Failed to set the declaration, returning D3DERR_INVALIDCALL\n", iface);
            IWineD3DVertexShader_Release(*ppVertexShader);
            return D3DERR_INVALIDCALL;
        }
    }

    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_CreatePixelShader(IWineD3DDevice *iface, CONST DWORD *pFunction, IWineD3DPixelShader **ppPixelShader, IUnknown *parent) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DPixelShaderImpl *object; /* NOTE: impl allowed, this is a create */
    HRESULT hr = D3D_OK;

    D3DCREATEOBJECTINSTANCE(object, PixelShader)
    hr = IWineD3DPixelShader_SetFunction(*ppPixelShader, pFunction);
    if (D3D_OK == hr) {
        TRACE("(%p) : Created Pixel shader %p\n", This, *ppPixelShader);
    } else {
        WARN("(%p) : Failed to create pixel shader\n", This);
    }

    return hr;
}

HRESULT WINAPI IWineD3DDeviceImpl_GetDirect3D(IWineD3DDevice *iface, IWineD3D **ppD3D) {
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
    HRESULT hr = D3D_OK;

    /* Update the current state block */
    This->updateStateBlock->fvf              = fvf;
    This->updateStateBlock->changed.fvf      = TRUE;
    This->updateStateBlock->set.fvf          = TRUE;

    TRACE("(%p) : FVF Shader FVF set to %lx\n", This, fvf);

    if (0 != fvf) {
        /* clear down the vertex declaration
         NOTE: Axis and Allies doesn't work properly otherwise
         (may be a stateblock problem though!)
        */
      hr = IWineD3DDevice_SetVertexDeclaration(iface, NULL);
    }

    return hr;
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

    /**TODO: instance and index data, see
    http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/programmingguide/advancedtopics/DrawingMultipleInstances.asp
    and
    http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/reference/d3d/interfaces/idirect3ddevice9/SetStreamSourceFreq.asp
     **************/

    /* D3d9 only, but shouldn't  hurt d3d8 */
    UINT streamFlags;

    streamFlags = StreamNumber &(D3DSTREAMSOURCE_INDEXEDDATA | D3DSTREAMSOURCE_INSTANCEDATA);
    if (streamFlags) {
        if (streamFlags & D3DSTREAMSOURCE_INDEXEDDATA) {
           FIXME("stream index data not supported\n");
        }
        if (streamFlags & D3DSTREAMSOURCE_INDEXEDDATA) {
           FIXME("stream instance data not supported\n");
        }
    }

    StreamNumber&= ~(D3DSTREAMSOURCE_INDEXEDDATA | D3DSTREAMSOURCE_INSTANCEDATA);

    if (StreamNumber >= MAX_STREAMS) {
        WARN("Stream out of range %d\n", StreamNumber);
        return D3DERR_INVALIDCALL;
    }

    oldSrc = This->stateBlock->streamSource[StreamNumber];
    TRACE("(%p) : StreamNo: %d, OldStream (%p), NewStream (%p), NewStride %d\n", This, StreamNumber, oldSrc, pStreamData, Stride);

    This->updateStateBlock->changed.streamSource[StreamNumber] = TRUE;
    This->updateStateBlock->set.streamSource[StreamNumber]     = TRUE;
    This->updateStateBlock->streamStride[StreamNumber]         = Stride;
    This->updateStateBlock->streamSource[StreamNumber]         = pStreamData;
    This->updateStateBlock->streamOffset[StreamNumber]         = OffsetInBytes;
    This->updateStateBlock->streamFlags[StreamNumber]          = streamFlags;

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return D3D_OK;
    }

    /* Not recording... */
    /* Need to do a getParent and pass the reffs up */
    /* MSDN says ..... When an application no longer holds a references to this interface, the interface will automatically be freed.
    which suggests that we shouldn't be ref counting? and do need a _release on the stream source to reset the stream source
    so for now, just count internally   */
    if (pStreamData != NULL) {
        IWineD3DVertexBuffer_AddRef(pStreamData);
    }
    if (oldSrc != NULL) {
        IWineD3DVertexBuffer_Release(oldSrc);
    }

    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_GetStreamSource(IWineD3DDevice *iface, UINT StreamNumber,IWineD3DVertexBuffer** pStream, UINT *pOffset, UINT* pStride) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    UINT streamFlags;

    TRACE("(%p) : StreamNo: %d, Stream (%p), Stride %d\n", This, StreamNumber,
           This->stateBlock->streamSource[StreamNumber], This->stateBlock->streamStride[StreamNumber]);


    streamFlags = StreamNumber &(D3DSTREAMSOURCE_INDEXEDDATA | D3DSTREAMSOURCE_INSTANCEDATA);
    if (streamFlags) {
        if (streamFlags & D3DSTREAMSOURCE_INDEXEDDATA) {
           FIXME("stream index data not supported\n");
        }
        if (streamFlags & D3DSTREAMSOURCE_INDEXEDDATA) {
            FIXME("stream instance data not supported\n");
        }
    }

    StreamNumber&= ~(D3DSTREAMSOURCE_INDEXEDDATA | D3DSTREAMSOURCE_INSTANCEDATA);

    if (StreamNumber >= MAX_STREAMS) {
        WARN("Stream out of range %d\n", StreamNumber);
        return D3DERR_INVALIDCALL;
    }
    *pStream = This->stateBlock->streamSource[StreamNumber];
    *pStride = This->stateBlock->streamStride[StreamNumber];
    *pOffset = This->stateBlock->streamOffset[StreamNumber];

     if (*pStream == NULL) {
        FIXME("Attempting to get an empty stream %d, returning D3DERR_INVALIDCALL\n", StreamNumber);
        return  D3DERR_INVALIDCALL;
    }

    IWineD3DVertexBuffer_AddRef(*pStream); /* We have created a new reference to the VB */
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

    TRACE("(%p) StreamNumber(%d), Divider(%d)\n", This, StreamNumber, Divider);
    This->updateStateBlock->streamFlags[StreamNumber] = Divider & (D3DSTREAMSOURCE_INSTANCEDATA  | D3DSTREAMSOURCE_INDEXEDDATA );

    This->updateStateBlock->changed.streamFreq[StreamNumber]  = TRUE;
    This->updateStateBlock->set.streamFreq[StreamNumber]      = TRUE;
    This->updateStateBlock->streamFreq[StreamNumber]          = Divider & 0x7FFFFF;

    if (This->updateStateBlock->streamFlags[StreamNumber] || This->updateStateBlock->streamFreq[StreamNumber] != 1) {
        FIXME("Stream indexing not fully supported\n");
    }

    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_GetStreamSourceFreq(IWineD3DDevice *iface,  UINT StreamNumber, UINT* Divider) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("(%p) StreamNumber(%d), Divider(%p)\n", This, StreamNumber, Divider);
    *Divider = This->updateStateBlock->streamFreq[StreamNumber] | This->updateStateBlock->streamFlags[StreamNumber];

    TRACE("(%p) : returning %d\n", This, *Divider);

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
        /* This is now set with the texture unit states, it may be a good idea to flag the change though! */
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
    temp.u.s._11 = (mat->u.s._11 * pMatrix->u.s._11) + (mat->u.s._21 * pMatrix->u.s._12) +
      (mat->u.s._31 * pMatrix->u.s._13) + (mat->u.s._41 * pMatrix->u.s._14);
    temp.u.s._21 = (mat->u.s._11 * pMatrix->u.s._21) + (mat->u.s._21 * pMatrix->u.s._22) +
      (mat->u.s._31 * pMatrix->u.s._23) + (mat->u.s._41 * pMatrix->u.s._24);
    temp.u.s._31 = (mat->u.s._11 * pMatrix->u.s._31) + (mat->u.s._21 * pMatrix->u.s._32) +
      (mat->u.s._31 * pMatrix->u.s._33) + (mat->u.s._41 * pMatrix->u.s._34);
    temp.u.s._41 = (mat->u.s._11 * pMatrix->u.s._41) + (mat->u.s._21 * pMatrix->u.s._42) +
      (mat->u.s._31 * pMatrix->u.s._43) + (mat->u.s._41 * pMatrix->u.s._44);

    temp.u.s._12 = (mat->u.s._12 * pMatrix->u.s._11) + (mat->u.s._22 * pMatrix->u.s._12) +
      (mat->u.s._32 * pMatrix->u.s._13) + (mat->u.s._42 * pMatrix->u.s._14);
    temp.u.s._22 = (mat->u.s._12 * pMatrix->u.s._21) + (mat->u.s._22 * pMatrix->u.s._22) +
      (mat->u.s._32 * pMatrix->u.s._23) + (mat->u.s._42 * pMatrix->u.s._24);
    temp.u.s._32 = (mat->u.s._12 * pMatrix->u.s._31) + (mat->u.s._22 * pMatrix->u.s._32) +
      (mat->u.s._32 * pMatrix->u.s._33) + (mat->u.s._42 * pMatrix->u.s._34);
    temp.u.s._42 = (mat->u.s._12 * pMatrix->u.s._41) + (mat->u.s._22 * pMatrix->u.s._42) +
      (mat->u.s._32 * pMatrix->u.s._43) + (mat->u.s._42 * pMatrix->u.s._44);

    temp.u.s._13 = (mat->u.s._13 * pMatrix->u.s._11) + (mat->u.s._23 * pMatrix->u.s._12) +
      (mat->u.s._33 * pMatrix->u.s._13) + (mat->u.s._43 * pMatrix->u.s._14);
    temp.u.s._23 = (mat->u.s._13 * pMatrix->u.s._21) + (mat->u.s._23 * pMatrix->u.s._22) +
      (mat->u.s._33 * pMatrix->u.s._23) + (mat->u.s._43 * pMatrix->u.s._24);
    temp.u.s._33 = (mat->u.s._13 * pMatrix->u.s._31) + (mat->u.s._23 * pMatrix->u.s._32) +
      (mat->u.s._33 * pMatrix->u.s._33) + (mat->u.s._43 * pMatrix->u.s._34);
    temp.u.s._43 = (mat->u.s._13 * pMatrix->u.s._41) + (mat->u.s._23 * pMatrix->u.s._42) +
      (mat->u.s._33 * pMatrix->u.s._43) + (mat->u.s._43 * pMatrix->u.s._44);

    temp.u.s._14 = (mat->u.s._14 * pMatrix->u.s._11) + (mat->u.s._24 * pMatrix->u.s._12) +
      (mat->u.s._34 * pMatrix->u.s._13) + (mat->u.s._44 * pMatrix->u.s._14);
    temp.u.s._24 = (mat->u.s._14 * pMatrix->u.s._21) + (mat->u.s._24 * pMatrix->u.s._22) +
      (mat->u.s._34 * pMatrix->u.s._23) + (mat->u.s._44 * pMatrix->u.s._24);
    temp.u.s._34 = (mat->u.s._14 * pMatrix->u.s._31) + (mat->u.s._24 * pMatrix->u.s._32) +
      (mat->u.s._34 * pMatrix->u.s._33) + (mat->u.s._44 * pMatrix->u.s._34);
    temp.u.s._44 = (mat->u.s._14 * pMatrix->u.s._41) + (mat->u.s._24 * pMatrix->u.s._42) +
      (mat->u.s._34 * pMatrix->u.s._43) + (mat->u.s._44 * pMatrix->u.s._44);

    /* Apply change via set transform - will reapply to eg. lights this way */
    return IWineD3DDeviceImpl_SetTransform(iface, State, &temp);
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

    /* Initialize the object */
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
        lightParms.Type         = D3DLIGHT_DIRECTIONAL;
        lightParms.Diffuse.r    = 1.0;
        lightParms.Diffuse.g    = 1.0;
        lightParms.Diffuse.b    = 1.0;
        lightParms.Diffuse.a    = 0.0;
        lightParms.Specular.r   = 0.0;
        lightParms.Specular.g   = 0.0;
        lightParms.Specular.b   = 0.0;
        lightParms.Specular.a   = 0.0;
        lightParms.Ambient.r    = 0.0;
        lightParms.Ambient.g    = 0.0;
        lightParms.Ambient.b    = 0.0;
        lightParms.Ambient.a    = 0.0;
        lightParms.Position.x   = 0.0;
        lightParms.Position.y   = 0.0;
        lightParms.Position.z   = 0.0;
        lightParms.Direction.x  = 0.0;
        lightParms.Direction.y  = 0.0;
        lightParms.Direction.z  = 1.0;
        lightParms.Range        = 0.0;
        lightParms.Falloff      = 0.0;
        lightParms.Attenuation0 = 0.0;
        lightParms.Attenuation1 = 0.0;
        lightParms.Attenuation2 = 0.0;
        lightParms.Theta        = 0.0;
        lightParms.Phi          = 0.0;
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
    TRACE("(%p) : Diffuse (%f,%f,%f,%f)\n", This, pMaterial->Diffuse.r, pMaterial->Diffuse.g,
        pMaterial->Diffuse.b, pMaterial->Diffuse.a);
    TRACE("(%p) : Ambient (%f,%f,%f,%f)\n", This, pMaterial->Ambient.r, pMaterial->Ambient.g,
        pMaterial->Ambient.b, pMaterial->Ambient.a);
    TRACE("(%p) : Specular (%f,%f,%f,%f)\n", This, pMaterial->Specular.r, pMaterial->Specular.g,
        pMaterial->Specular.b, pMaterial->Specular.a);
    TRACE("(%p) : Emissive (%f,%f,%f,%f)\n", This, pMaterial->Emissive.r, pMaterial->Emissive.g,
        pMaterial->Emissive.b, pMaterial->Emissive.a);
    TRACE("(%p) : Power (%f)\n", This, pMaterial->Power);

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, (float*) &This->updateStateBlock->material.Ambient);
    checkGLcall("glMaterialfv(GL_AMBIENT)");
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, (float*) &This->updateStateBlock->material.Diffuse);
    checkGLcall("glMaterialfv(GL_DIFFUSE)");

    /* Only change material color if specular is enabled, otherwise it is set to black */
    if (This->stateBlock->renderState[WINED3DRS_SPECULARENABLE]) {
       glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, (float*) &This->updateStateBlock->material.Specular);
       checkGLcall("glMaterialfv(GL_SPECULAR");
    } else {
       float black[4] = {0.0f, 0.0f, 0.0f, 0.0f};
       glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, &black[0]);
       checkGLcall("glMaterialfv(GL_SPECULAR");
    }
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, (float*) &This->updateStateBlock->material.Emissive);
    checkGLcall("glMaterialfv(GL_EMISSION)");
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, This->updateStateBlock->material.Power);
    checkGLcall("glMaterialf(GL_SHININESS");

    LEAVE_GL();
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_GetMaterial(IWineD3DDevice *iface, WINED3DMATERIAL* pMaterial) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    memcpy(pMaterial, &This->updateStateBlock->material, sizeof (WINED3DMATERIAL));
    TRACE("(%p) : Diffuse (%f,%f,%f,%f)\n", This, pMaterial->Diffuse.r, pMaterial->Diffuse.g,
        pMaterial->Diffuse.b, pMaterial->Diffuse.a);
    TRACE("(%p) : Ambient (%f,%f,%f,%f)\n", This, pMaterial->Ambient.r, pMaterial->Ambient.g,
        pMaterial->Ambient.b, pMaterial->Ambient.a);
    TRACE("(%p) : Specular (%f,%f,%f,%f)\n", This, pMaterial->Specular.r, pMaterial->Specular.g,
        pMaterial->Specular.b, pMaterial->Specular.a);
    TRACE("(%p) : Emissive (%f,%f,%f,%f)\n", This, pMaterial->Emissive.r, pMaterial->Emissive.g,
        pMaterial->Emissive.b, pMaterial->Emissive.a);
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

    if (NULL != pIndexData) {
        IWineD3DIndexBuffer_AddRef(pIndexData);
    }
    if (NULL != oldIdxs) {
        IWineD3DIndexBuffer_Release(oldIdxs);
    }
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_GetIndices(IWineD3DDevice *iface, IWineD3DIndexBuffer** ppIndexData, UINT* pBaseVertexIndex) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    *ppIndexData = This->stateBlock->pIndexData;

    /* up ref count on ppindexdata */
    if (*ppIndexData) {
        IWineD3DIndexBuffer_AddRef(*ppIndexData);
        *pBaseVertexIndex = This->stateBlock->baseVertexIndex;
        TRACE("(%p) index data set to %p + %u\n", This, ppIndexData, This->stateBlock->baseVertexIndex);
    }else{
        TRACE("(%p) No index data set\n", This);
    }
    TRACE("Returning %p %d\n", *ppIndexData, *pBaseVertexIndex);

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
    This->viewport_changed = TRUE;

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

    case WINED3DRS_STENCILFUNC :
    {
        GLint func;
        GLint ref = This->stateBlock->renderState[WINED3DRS_STENCILREF];
        GLuint mask = This->stateBlock->renderState[WINED3DRS_STENCILMASK];

        func = GL_ALWAYS;
        switch ((D3DCMPFUNC)Value) {
            case D3DCMP_NEVER: func = GL_NEVER; break;
            case D3DCMP_LESS: func = GL_LESS; break;
            case D3DCMP_EQUAL: func = GL_EQUAL; break;
            case D3DCMP_LESSEQUAL: func = GL_LEQUAL; break;
            case D3DCMP_GREATER: func = GL_GREATER; break;
            case D3DCMP_NOTEQUAL: func = GL_NOTEQUAL; break;
            case D3DCMP_GREATEREQUAL: func = GL_GEQUAL; break;
            case D3DCMP_ALWAYS: func = GL_ALWAYS; break;
            default:
                FIXME("Unrecognized/Unhandled D3DCMPFUNC value %ld\n", Value);
        }
        This->stencilfunc = func;
        if(!This->stateBlock->renderState[WINED3DRS_TWOSIDEDSTENCILMODE]) {
            if(GL_EXTCALL(glStencilFuncSeparate)) {
                GL_EXTCALL(glStencilFuncSeparate(GL_FRONT, func, ref, mask));
                checkGLcall("glStencilFuncSeparate(GL_FRONT,...)");
            }
            else if(GL_SUPPORT(EXT_STENCIL_TWO_SIDE)) {
                glEnable(GL_STENCIL_TEST_TWO_SIDE_EXT);
                checkGLcall("glEnable(GL_STENCIL_TEST_TWO_SIDE_EXT)");
                GL_EXTCALL(glActiveStencilFaceEXT(GL_FRONT));
                checkGLcall("glActiveStencilFaceEXT(GL_FRONT)");
                glStencilFunc(func, ref, mask);
                checkGLcall("glStencilFunc(...)");
            }
            else if(GL_SUPPORT(ATI_SEPARATE_STENCIL)) {
                GL_EXTCALL(glStencilFuncSeparateATI(GL_FRONT, func, ref, mask));
                checkGLcall("glStencilFuncSeparateATI(GL_FRONT,...)");
            } else {
                TRACE("Separate stencil function not supported on this version of opengl");
                glStencilFunc(func, ref, mask);
                checkGLcall("glStencilFunc(...)");
            }
        } else {
            glStencilFunc(func, ref, mask);
            checkGLcall("glStencilFunc(...)");
        }
        break;
    }
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
    case WINED3DRS_STENCILZFAIL              :
    case WINED3DRS_STENCILPASS               :
    {
        GLint stencilFail;
        GLint depthFail;
        GLint stencilPass;

        GLint action = StencilOp(Value);

        glGetIntegerv(GL_STENCIL_FAIL, &stencilFail);
        glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, &depthFail);
        glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, &stencilPass);

        if(WINED3DRS_STENCILFAIL == State) {
            stencilFail = action;
        }
        else if(WINED3DRS_STENCILZFAIL == State) {
            depthFail = action;
        }
        else if(WINED3DRS_STENCILPASS == State) {
            stencilPass = action;
        }

        if(!This->stateBlock->renderState[WINED3DRS_TWOSIDEDSTENCILMODE]) {
            if(GL_EXTCALL(glStencilOpSeparate)) {
                GL_EXTCALL(glStencilOpSeparate(GL_FRONT, stencilFail, depthFail, stencilPass));
                checkGLcall("glStencilOpSeparate(GL_FRONT,...)");
            }
            else if(GL_SUPPORT(EXT_STENCIL_TWO_SIDE)) {
                glEnable(GL_STENCIL_TEST_TWO_SIDE_EXT);
                checkGLcall("glEnable(GL_STENCIL_TEST_TWO_SIDE_EXT)");
                GL_EXTCALL(glActiveStencilFaceEXT(GL_FRONT));
                checkGLcall("glActiveStencilFaceEXT(GL_FRONT)");
                glStencilOp(stencilFail, depthFail, stencilPass);
                checkGLcall("glStencilOp(...)");
            }
            else if(GL_SUPPORT(ATI_SEPARATE_STENCIL)) {
                GL_EXTCALL(glStencilOpSeparateATI(GL_FRONT, stencilFail, depthFail, stencilPass));
                checkGLcall("glStencilOpSeparateATI(GL_FRONT,...)");
            } else {
                TRACE("Separate stencil operation not supported on this version of opengl");
                glStencilOp(stencilFail, depthFail, stencilPass);
                checkGLcall("glStencilOp(...)");
            }
        } else {
            glStencilOp(stencilFail, depthFail, stencilPass);
            checkGLcall("glStencilOp(...)");
        }
        break;
    }
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

    case WINED3DRS_ZBIAS                     : /* D3D8 only */
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
        /* FIXME: check that pointSize isn't outside glGetFloatv( GL_POINT_SIZE_MAX_ARB, &maxSize ); or -ve */
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
        /*
         * POINTSCALEENABLE controls how point size value is treated. If set to
         * true, the point size is scaled with respect to height of viewport.
         * When set to false point size is in pixels.
         *
         * http://msdn.microsoft.com/library/en-us/directx9_c/point_sprites.asp
         */

        /* Default values */
        GLfloat att[3] = {1.0f, 0.0f, 0.0f};

        /*
         * Minimum valid point size for OpenGL is 1.0f. For Direct3D it is 0.0f.
         * This means that OpenGL will clamp really small point sizes to 1.0f.
         * To correct for this we need to multiply by the scale factor when sizes
         * are less than 1.0f. scale_factor =  1.0f / point_size.
         */
        GLfloat pointSize = *((float*)&This->stateBlock->renderState[WINED3DRS_POINTSIZE]);
        if(pointSize > 0.0f) {
            GLfloat scaleFactor;

            if(pointSize < 1.0f) {
                scaleFactor = pointSize * pointSize;
            } else {
                scaleFactor = 1.0f;
            }

            if(This->stateBlock->renderState[WINED3DRS_POINTSCALEENABLE]) {
                att[0] = *((float*)&This->stateBlock->renderState[WINED3DRS_POINTSCALE_A]) /
                    (This->stateBlock->viewport.Height * This->stateBlock->viewport.Height * scaleFactor);
                att[1] = *((float*)&This->stateBlock->renderState[WINED3DRS_POINTSCALE_B]) /
                    (This->stateBlock->viewport.Height * This->stateBlock->viewport.Height * scaleFactor);
                att[2] = *((float*)&This->stateBlock->renderState[WINED3DRS_POINTSCALE_C]) /
                    (This->stateBlock->viewport.Height * This->stateBlock->viewport.Height * scaleFactor);
            }
        }

        if(GL_SUPPORT(ARB_POINT_PARAMETERS)) {
            GL_EXTCALL(glPointParameterfvARB)(GL_POINT_DISTANCE_ATTENUATION_ARB, att);
            checkGLcall("glPointParameterfvARB(GL_DISTANCE_ATTENUATION_ARB, ...");
        }
        else if(GL_SUPPORT(EXT_POINT_PARAMETERS)) {
            GL_EXTCALL(glPointParameterfvEXT)(GL_DISTANCE_ATTENUATION_EXT, att);
            checkGLcall("glPointParameterfvEXT(GL_DISTANCE_ATTENUATION_EXT, ...");
        } else {
            TRACE("POINT_PARAMETERS not supported in this version of opengl\n");
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
    case WINED3DRS_POINTSPRITEENABLE         :
    {
        /* TODO: NV_POINT_SPRITE */
        if (!GL_SUPPORT(ARB_POINT_SPRITE)) {
            TRACE("Point sprites not supported\n");
            break;
        }

        /*
         * Point sprites are always enabled. Value controls texture coordinate
         * replacement mode. Must be set true for point sprites to use
         * textures.
         */
        glEnable(GL_POINT_SPRITE_ARB);
        checkGLcall("glEnable(GL_POINT_SPRITE_ARB)");

        if (Value) {
            glTexEnvf(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, TRUE);
            checkGLcall("glTexEnvf(GL_POINT_SPRITE, GL_COORD_REPLACE, TRUE)");
        } else {
            glTexEnvf(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, FALSE);
            checkGLcall("glTexEnvf(GL_POINT_SPRITE, GL_COORD_REPLACE, FALSE)");
        }
        break;
    }
    case WINED3DRS_EDGEANTIALIAS             :
    {
        if(Value) {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_BLEND);
            checkGLcall("glEnable(GL_BLEND)");
            glEnable(GL_LINE_SMOOTH);
            checkGLcall("glEnable(GL_LINE_SMOOTH)");
        } else {
            glDisable(GL_BLEND);
            checkGLcall("glDisable(GL_BLEND)");
            glDisable(GL_LINE_SMOOTH);
            checkGLcall("glDisable(GL_LINE_SMOOTH)");
        }
        break;
    }
    case WINED3DRS_WRAP0                     :
    case WINED3DRS_WRAP1                     :
    case WINED3DRS_WRAP2                     :
    case WINED3DRS_WRAP3                     :
    case WINED3DRS_WRAP4                     :
    case WINED3DRS_WRAP5                     :
    case WINED3DRS_WRAP6                     :
    case WINED3DRS_WRAP7                     :
    /**
    http://www.cosc.brocku.ca/Offerings/3P98/course/lectures/texture/
    http://msdn.microsoft.com/archive/default.asp?url=/archive/en-us/directx9_c/directx/graphics/programmingguide/FixedFunction/Textures/texturewrapping.asp
    http://www.gamedev.net/reference/programming/features/rendererdll3/page2.asp
    Descussion that ways to turn on WRAPing to solve an opengl conversion problem.
    http://www.flipcode.org/cgi-bin/fcmsg.cgi?thread_show=10248

    so far as I can tell, wrapping and texture-coordinate generate go hand in hand,
    */
    TRACE("(%p)->(%d,%ld) Texture wraping not yet supported\n",This, State, Value);
    break;
    case WINED3DRS_MULTISAMPLEANTIALIAS      :
    {
        if (!GL_SUPPORT(ARB_MULTISAMPLE)) {
            TRACE("Multisample antialiasing not supported\n");
            break;
        }

        if(Value) {
            glEnable(GL_MULTISAMPLE_ARB);
            checkGLcall("glEnable(GL_MULTISAMPLE_ARB)");
        } else {
            glDisable(GL_MULTISAMPLE_ARB);
            checkGLcall("glDisable(GL_MULTISAMPLE_ARB)");
        }
        break;
    }
    case WINED3DRS_SCISSORTESTENABLE :
    {
        if(Value) {
            glEnable(GL_SCISSOR_TEST);
            checkGLcall("glEnable(GL_SCISSOR_TEST)");
        } else {
            glDisable(GL_SCISSOR_TEST);
            checkGLcall("glDisable(GL_SCISSOR_TEST)");
        }
        break;
    }
    case WINED3DRS_SLOPESCALEDEPTHBIAS :
    {
        if(Value) {
            tmpvalue.d = Value;
            glEnable(GL_POLYGON_OFFSET_FILL);
            checkGLcall("glEnable(GL_POLYGON_OFFSET_FILL)");
            glPolygonOffset(tmpvalue.f, *((float*)&This->stateBlock->renderState[WINED3DRS_DEPTHBIAS]));
            checkGLcall("glPolygonOffset(...)");
        } else {
            glDisable(GL_POLYGON_OFFSET_FILL);
            checkGLcall("glDisable(GL_POLYGON_OFFSET_FILL)");
        }
        break;
    }
    case WINED3DRS_ANTIALIASEDLINEENABLE :
    {
        if(Value) {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_BLEND);
            checkGLcall("glEnable(GL_BLEND)");
            glEnable(GL_LINE_SMOOTH);
            checkGLcall("glEnable(GL_LINE_SMOOTH)");
        } else {
            glDisable(GL_BLEND);
            checkGLcall("glDisable(GL_BLEND)");
            glDisable(GL_LINE_SMOOTH);
            checkGLcall("glDisable(GL_LINE_SMOOTH)");
        }
        break;
    }
    case WINED3DRS_MULTISAMPLEMASK           :
    case WINED3DRS_PATCHEDGESTYLE            :
    case WINED3DRS_PATCHSEGMENTS             :
    case WINED3DRS_DEBUGMONITORTOKEN         :
    case WINED3DRS_POSITIONORDER             :
    case WINED3DRS_NORMALORDER               :
    /* Direct3D9 render states */
    case WINED3DRS_MINTESSELLATIONLEVEL :
    case WINED3DRS_MAXTESSELLATIONLEVEL :
    case WINED3DRS_ADAPTIVETESS_X :
    case WINED3DRS_ADAPTIVETESS_Y :
    case WINED3DRS_ADAPTIVETESS_Z :
    case WINED3DRS_ADAPTIVETESS_W :
    case WINED3DRS_ENABLEADAPTIVETESSELLATION :
    {
        FIXME("(%p)->(%d,%ld) not handled yet\n", This, State, Value);
        break;
    }
    case WINED3DRS_TWOSIDEDSTENCILMODE :
    {
        if(Value) {
            TRACE("Two-sided stencil mode enabled\n");
        } else {
            TRACE("Two-sided stencil mode disabled\n");
        }
        break;
    }
    case WINED3DRS_CCW_STENCILFAIL :
    case WINED3DRS_CCW_STENCILZFAIL :
    case WINED3DRS_CCW_STENCILPASS :
    {
        GLint stencilFail;
        GLint depthFail;
        GLint stencilPass;

        GLint action = StencilOp(Value);

        glGetIntegerv(GL_STENCIL_BACK_FAIL, &stencilFail);
        glGetIntegerv(GL_STENCIL_BACK_PASS_DEPTH_FAIL, &depthFail);
        glGetIntegerv(GL_STENCIL_BACK_PASS_DEPTH_PASS, &stencilPass);

        if(WINED3DRS_CCW_STENCILFAIL == State) {
            stencilFail = action;
        }
        else if(WINED3DRS_CCW_STENCILZFAIL == State) {
            depthFail = action;
        }
        else if(WINED3DRS_CCW_STENCILPASS == State) {
            stencilPass = action;
        }

        if(!This->stateBlock->renderState[WINED3DRS_TWOSIDEDSTENCILMODE]) {
            if(GL_EXTCALL(glStencilOpSeparate)) {
                GL_EXTCALL(glStencilOpSeparate(GL_BACK, stencilFail, depthFail, stencilPass));
                checkGLcall("glStencilOpSeparate(GL_BACK,...)");
            }
            else if(GL_SUPPORT(EXT_STENCIL_TWO_SIDE)) {
                glEnable(GL_STENCIL_TEST_TWO_SIDE_EXT);
                checkGLcall("glEnable(GL_STENCIL_TEST_TWO_SIDE_EXT)");
                GL_EXTCALL(glActiveStencilFaceEXT(GL_BACK));
                checkGLcall("glActiveStencilFaceEXT(GL_BACK)");
                glStencilOp(stencilFail, depthFail, stencilPass);
                checkGLcall("glStencilOp(...)");
            }
            else if(GL_SUPPORT(ATI_SEPARATE_STENCIL)) {
                GL_EXTCALL(glStencilOpSeparateATI(GL_BACK, stencilFail, depthFail, stencilPass));
                checkGLcall("glStencilOpSeparateATI(GL_BACK,...)");
            } else {
                TRACE("Separate stencil operation not supported on this version of opengl");
                glStencilOp(stencilFail, depthFail, stencilPass);
                checkGLcall("glStencilOp(...)");
            }
        } else {
            glStencilOp(stencilFail, depthFail, stencilPass);
            checkGLcall("glStencilOp(...)");
        }
        break;
    }
    case WINED3DRS_CCW_STENCILFUNC :
    {
        GLint func;
        GLint ref = This->stateBlock->renderState[WINED3DRS_STENCILREF];
        GLuint mask = This->stateBlock->renderState[WINED3DRS_STENCILMASK];

        func = GL_ALWAYS;
        switch ((D3DCMPFUNC)Value) {
            case D3DCMP_NEVER: func = GL_NEVER; break;
            case D3DCMP_LESS: func = GL_LESS; break;
            case D3DCMP_EQUAL: func = GL_EQUAL; break;
            case D3DCMP_LESSEQUAL: func = GL_LEQUAL; break;
            case D3DCMP_GREATER: func = GL_GREATER; break;
            case D3DCMP_NOTEQUAL: func = GL_NOTEQUAL; break;
            case D3DCMP_GREATEREQUAL: func = GL_GEQUAL; break;
            case D3DCMP_ALWAYS: func = GL_ALWAYS; break;
            default:
                FIXME("Unrecognized/Unhandled D3DCMPFUNC value %ld\n", Value);
        }
        This->stencilfunc = func;
        if(!This->stateBlock->renderState[WINED3DRS_TWOSIDEDSTENCILMODE]) {
            if(GL_EXTCALL(glStencilFuncSeparate)) {
                GL_EXTCALL(glStencilFuncSeparate(GL_BACK, func, ref, mask));
                checkGLcall("glStencilFuncSeparate(GL_BACK,...)");
            }
            else if(GL_SUPPORT(EXT_STENCIL_TWO_SIDE)) {
                glEnable(GL_STENCIL_TEST_TWO_SIDE_EXT);
                checkGLcall("glEnable(GL_STENCIL_TEST_TWO_SIDE_EXT)");
                GL_EXTCALL(glActiveStencilFaceEXT(GL_BACK));
                checkGLcall("glActiveStencilFaceEXT(GL_BACK)");
                glStencilFunc(func, ref, mask);
                checkGLcall("glStencilFunc(...)");
            }
            else if(GL_SUPPORT(ATI_SEPARATE_STENCIL)) {
                GL_EXTCALL(glStencilFuncSeparateATI(GL_BACK, func, ref, mask));
                checkGLcall("glStencilFuncSeparateATI(GL_BACK,...)");
            } else {
                TRACE("Separate stencil function not supported on this version of opengl");
                glStencilFunc(func, ref, mask);
                checkGLcall("glStencilFunc(...)");
            }
        } else {
            glStencilFunc(func, ref, mask);
            checkGLcall("glStencilFunc(...)");
        }
        break;
    }
    case WINED3DRS_COLORWRITEENABLE1 :
    case WINED3DRS_COLORWRITEENABLE2 :
    case WINED3DRS_COLORWRITEENABLE3 :
    case WINED3DRS_BLENDFACTOR :
    case WINED3DRS_SRGBWRITEENABLE :
    {
        FIXME("(%p)->(%d,%ld) not handled yet\n", This, State, Value);
        break;
    }
    case WINED3DRS_DEPTHBIAS :
    {
        if(Value) {
            tmpvalue.d = Value;
            glEnable(GL_POLYGON_OFFSET_FILL);
            checkGLcall("glEnable(GL_POLYGON_OFFSET_FILL)");
            glPolygonOffset(*((float*)&This->stateBlock->renderState[WINED3DRS_SLOPESCALEDEPTHBIAS]), tmpvalue.f);
            checkGLcall("glPolygonOffset(...)");
        } else {
            glDisable(GL_POLYGON_OFFSET_FILL);
            checkGLcall("glDisable(GL_POLYGON_OFFSET_FILL)");
        }
        break;
    }
    case WINED3DRS_WRAP8 :
    case WINED3DRS_WRAP9 :
    case WINED3DRS_WRAP10 :
    case WINED3DRS_WRAP11 :
    case WINED3DRS_WRAP12 :
    case WINED3DRS_WRAP13 :
    case WINED3DRS_WRAP14 :
    case WINED3DRS_WRAP15 :
    case WINED3DRS_SEPARATEALPHABLENDENABLE :
    case WINED3DRS_SRCBLENDALPHA :
    case WINED3DRS_DESTBLENDALPHA :
    case WINED3DRS_BLENDOPALPHA :
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

/*****
 * Get / Set Sampler States
 * TODO: Verify against dx9 definitions
 *****/

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
    /** NOTE: States are appled in IWineD3DBaseTextre ApplyStateChanges and IWineD3DDevice SetupTextureStates**/
    TRACE("(%p) Sampler(%ld), Type(%d) Value(%ld)\n",This, Sampler ,Type, Value);

    if(Sampler >  GL_LIMITS(samplers) || Sampler < 0 || Type > WINED3D_HIGHEST_SAMPLER_STATE || Type < 0) {
         FIXME("out of range %d %d sampler %ld type %u\n", GL_LIMITS(samplers), WINED3D_HIGHEST_SAMPLER_STATE, Sampler, Type);
        return D3DERR_INVALIDCALL;
    }
    TRACE("Setting sampler %ld %d to %ld\n", Sampler, Type, Value);
    This->updateStateBlock->samplerState[Sampler][Type]         = Value;
    This->updateStateBlock->set.samplerState[Sampler][Type]     = Value;
    This->updateStateBlock->changed.samplerState[Sampler][Type] = Value;

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return D3D_OK;
    }

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
    IWineD3DVertexDeclaration *oldDecl = This->updateStateBlock->vertexDecl;

    TRACE("(%p) : pDecl=%p\n", This, pDecl);

    This->updateStateBlock->vertexDecl = pDecl;
    This->updateStateBlock->changed.vertexDecl = TRUE;
    This->updateStateBlock->set.vertexDecl = TRUE;

    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
    }

    if (NULL != pDecl) {
        IWineD3DVertexDeclaration_AddRef(pDecl);
    }
    if (NULL != oldDecl) {
        IWineD3DVertexDeclaration_Release(oldDecl);
    }
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_GetVertexDeclaration(IWineD3DDevice* iface, IWineD3DVertexDeclaration** ppDecl) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("(%p) : ppDecl=%p\n", This, ppDecl);

    *ppDecl = This->stateBlock->vertexDecl;
    if (NULL != *ppDecl) IWineD3DVertexDeclaration_AddRef(*ppDecl);
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_SetVertexShader(IWineD3DDevice *iface, IWineD3DVertexShader* pShader) {
    IWineD3DDeviceImpl *This        = (IWineD3DDeviceImpl *)iface;
    IWineD3DVertexShader* oldShader = This->updateStateBlock->vertexShader;

    This->updateStateBlock->vertexShader         = pShader;
    This->updateStateBlock->changed.vertexShader = TRUE;
    This->updateStateBlock->set.vertexShader     = TRUE;

    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
    }

    if (NULL != pShader) {
        IWineD3DVertexShader_AddRef(pShader);
    }
    if (NULL != oldShader) {
        IWineD3DVertexShader_Release(oldShader);
    }

    TRACE("(%p) : setting pShader(%p)\n", This, pShader);
    /**
     * TODO: merge HAL shaders context switching from prototype
     */
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_GetVertexShader(IWineD3DDevice *iface, IWineD3DVertexShader** ppShader) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    if (NULL == ppShader) {
        return D3DERR_INVALIDCALL;
    }
    *ppShader = This->stateBlock->vertexShader;
    if( NULL != *ppShader)
        IWineD3DVertexShader_AddRef(*ppShader);

    TRACE("(%p) : returning %p\n", This, *ppShader);
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_SetVertexShaderConstant(IWineD3DDevice *iface, void *dstData, const void *srcData, UINT type, UINT start, UINT count, UINT registersize) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    int i;
    int cnt = min(count, MAX_VSHADER_CONSTANTS - (start + 1));

    TRACE("(iface %p, dstData %p, srcData %p, type %d, start %d, count %d, registersize %d)\n",
            iface, dstData, srcData, type, start, count, registersize);

    if (type != WINESHADERCNST_NONE) {
        if (srcData == NULL || cnt < 0) {
            return D3DERR_INVALIDCALL;
        }

        CopyMemory((char *)dstData + (start * registersize), srcData, cnt * registersize);
    }

    for (i = start; i < cnt + start; ++i) {
        This->updateStateBlock->changed.vertexShaderConstants[i] = TRUE;
        This->updateStateBlock->set.vertexShaderConstants[i]     = TRUE;
        This->updateStateBlock->vertexShaderConstantT[i]         = type;
    }

    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_GetVertexShaderConstant(IWineD3DDevice *iface, void *dstData, const void *srcData, UINT type, UINT start, UINT count, UINT registersize) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    int i;
    int cnt = min(count, MAX_VSHADER_CONSTANTS - (start + 1));

    TRACE("(iface %p, dstData %p, srcData %p, type %d, start %d, count %d, registersize %d)\n",
            iface, dstData, srcData, type, start, count, registersize);

    /* Verify that the requested shader constant was populated with the correct type */
    for (i = start; i < cnt + start; ++i) {
        if (This->updateStateBlock->vertexShaderConstantT[i] != type) {
            TRACE("(%p) : Caller requested 0x%x while type is 0x%x. Returning D3DERR_INVALIDCALL\n", 
                    This, type, This->updateStateBlock->vertexShaderConstantT[i]);
            return D3DERR_INVALIDCALL;
        }
    }

    if (dstData == NULL || cnt < 0) {
        return D3DERR_INVALIDCALL;
    }

    CopyMemory(dstData, (char *)srcData + (start * registersize), cnt * registersize);

    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_SetVertexShaderConstantB(IWineD3DDevice *iface, UINT StartRegister, CONST BOOL  *pConstantData, UINT BoolCount){
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    
    return IWineD3DDeviceImpl_SetVertexShaderConstant(iface, 
            This->updateStateBlock->vertexShaderConstantB, 
            pConstantData, 
            WINESHADERCNST_BOOL, 
            StartRegister, 
            BoolCount, 
            sizeof(*pConstantData));
}

HRESULT WINAPI IWineD3DDeviceImpl_GetVertexShaderConstantB(IWineD3DDevice *iface, UINT StartRegister, BOOL *pConstantData, UINT BoolCount){
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    
    return IWineD3DDeviceImpl_GetVertexShaderConstant(iface, 
            pConstantData, 
            This->updateStateBlock->vertexShaderConstantB, 
            WINESHADERCNST_BOOL, 
            StartRegister, 
            BoolCount, 
            sizeof(*pConstantData));
}

HRESULT WINAPI IWineD3DDeviceImpl_SetVertexShaderConstantI(IWineD3DDevice *iface, UINT StartRegister, CONST int *pConstantData, UINT Vector4iCount){
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    
    return IWineD3DDeviceImpl_SetVertexShaderConstant(iface, 
            This->updateStateBlock->vertexShaderConstantI, 
            pConstantData, 
            WINESHADERCNST_INTEGER, 
            StartRegister, 
            Vector4iCount, 
            4 * sizeof(*pConstantData));
}

HRESULT WINAPI IWineD3DDeviceImpl_GetVertexShaderConstantI(IWineD3DDevice *iface, UINT StartRegister, int         *pConstantData, UINT Vector4iCount){
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    
    return IWineD3DDeviceImpl_GetVertexShaderConstant(iface, 
            pConstantData, 
            This->updateStateBlock->vertexShaderConstantI, 
            WINESHADERCNST_INTEGER, 
            StartRegister, 
            Vector4iCount, 
            4 * sizeof(*pConstantData));
}


HRESULT WINAPI IWineD3DDeviceImpl_SetVertexShaderConstantF(IWineD3DDevice *iface, UINT StartRegister, CONST float *pConstantData, UINT Vector4fCount){
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    return IWineD3DDeviceImpl_SetVertexShaderConstant(iface, 
            This->updateStateBlock->vertexShaderConstantF, 
            pConstantData, 
            WINESHADERCNST_FLOAT, 
            StartRegister, 
            Vector4fCount, 
            4 * sizeof(*pConstantData));
}

HRESULT WINAPI IWineD3DDeviceImpl_GetVertexShaderConstantF(IWineD3DDevice *iface, UINT StartRegister, float       *pConstantData, UINT Vector4fCount){
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    return IWineD3DDeviceImpl_GetVertexShaderConstant(iface, 
            pConstantData, 
            This->updateStateBlock->vertexShaderConstantF, 
            WINESHADERCNST_FLOAT, 
            StartRegister, 
            Vector4fCount, 
            4 * sizeof(*pConstantData));
}

HRESULT WINAPI IWineD3DDeviceImpl_SetVertexShaderConstantN(IWineD3DDevice *iface, UINT StartRegister, UINT VectorNCount){
    return IWineD3DDeviceImpl_SetVertexShaderConstant(iface, 
            NULL, 
            NULL, 
            WINESHADERCNST_NONE, 
            StartRegister, 
            VectorNCount, 
            0);
}

HRESULT WINAPI IWineD3DDeviceImpl_SetPixelShader(IWineD3DDevice *iface, IWineD3DPixelShader *pShader) {
    IWineD3DDeviceImpl *This        = (IWineD3DDeviceImpl *)iface;
    IWineD3DPixelShader *oldShader  = This->updateStateBlock->pixelShader;
    This->updateStateBlock->pixelShader         = pShader;
    This->updateStateBlock->changed.pixelShader = TRUE;
    This->updateStateBlock->set.pixelShader     = TRUE;

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
    }

    if (NULL != pShader) {
        IWineD3DPixelShader_AddRef(pShader);
    }
    if (NULL != oldShader) {
        IWineD3DPixelShader_Release(oldShader);
    }

    TRACE("(%p) : setting pShader(%p)\n", This, pShader);
    /**
     * TODO: merge HAL shaders context switching from prototype
     */
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_GetPixelShader(IWineD3DDevice *iface, IWineD3DPixelShader **ppShader) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    if (NULL == ppShader) {
        WARN("(%p) : PShader is NULL, returning INVALIDCALL\n", This);
        return D3DERR_INVALIDCALL;
    }

    *ppShader =  This->stateBlock->pixelShader;
    if (NULL != ppShader) {
        IWineD3DPixelShader_AddRef(*ppShader);
    }
    TRACE("(%p) : returning %p\n", This, *ppShader);
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_SetPixelShaderConstant(IWineD3DDevice *iface, void *dstData, const void *srcData, UINT type, UINT start, UINT count, UINT registersize) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    int i;
    int cnt = min(count, MAX_PSHADER_CONSTANTS - (start + 1));

    TRACE("(iface %p, dstData %p, srcData %p, type %d, start %d, count %d, registersize %d)\n",
            iface, dstData, srcData, type, start, count, registersize);

    if (type != WINESHADERCNST_NONE) {
        if (srcData == NULL || cnt < 0) {
            return D3DERR_INVALIDCALL;
        }

        CopyMemory((char *)dstData + (start * registersize), srcData, cnt * registersize);
    }

    for (i = start; i < cnt + start; ++i) {
        This->updateStateBlock->changed.pixelShaderConstants[i] = TRUE;
        This->updateStateBlock->set.pixelShaderConstants[i]     = TRUE;
        This->updateStateBlock->pixelShaderConstantT[i]         = type;
    }

    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_GetPixelShaderConstant(IWineD3DDevice *iface, void *dstData, const void *srcData, UINT type, UINT start, UINT count, UINT registersize) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    int i;
    int cnt = min(count, MAX_PSHADER_CONSTANTS - (start + 1));

    TRACE("(iface %p, dstData %p, srcData %p, type %d, start %d, count %d, registersize %d)\n",
            iface, dstData, srcData, type, start, count, registersize);

    /* Verify that the requested shader constant was populated with the correct type */
    for (i = start; i < cnt + start; ++i) {
        if (This->updateStateBlock->pixelShaderConstantT[i] != type) {
            TRACE("(%p) : Caller requested 0x%x while type is 0x%x. Returning D3DERR_INVALIDCALL\n", 
                    This, type, This->updateStateBlock->pixelShaderConstantT[i]);
            return D3DERR_INVALIDCALL;
        }
    }

    if (dstData == NULL || cnt < 0) {
        return D3DERR_INVALIDCALL;
    }

    CopyMemory(dstData, (char *)srcData + (start * registersize), cnt * registersize);

    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_SetPixelShaderConstantB(IWineD3DDevice *iface, UINT StartRegister, CONST BOOL   *pConstantData, UINT BoolCount) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    
    return IWineD3DDeviceImpl_SetPixelShaderConstant(iface, 
            This->updateStateBlock->pixelShaderConstantB, 
            pConstantData, 
            WINESHADERCNST_BOOL, 
            StartRegister, 
            BoolCount, 
            sizeof(*pConstantData));
}

HRESULT WINAPI IWineD3DDeviceImpl_GetPixelShaderConstantB(IWineD3DDevice *iface, UINT StartRegister, BOOL         *pConstantData, UINT BoolCount) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    return IWineD3DDeviceImpl_GetPixelShaderConstant(iface, 
            pConstantData, 
            This->updateStateBlock->pixelShaderConstantB, 
            WINESHADERCNST_BOOL, 
            StartRegister, 
            BoolCount, 
            sizeof(*pConstantData));
}

HRESULT WINAPI IWineD3DDeviceImpl_SetPixelShaderConstantI(IWineD3DDevice *iface, UINT StartRegister, CONST int    *pConstantData, UINT Vector4iCount) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    return IWineD3DDeviceImpl_SetPixelShaderConstant(iface, 
            This->updateStateBlock->pixelShaderConstantI, 
            pConstantData, 
            WINESHADERCNST_INTEGER, 
            StartRegister, 
            Vector4iCount, 
            4 * sizeof(*pConstantData));
}

HRESULT WINAPI IWineD3DDeviceImpl_GetPixelShaderConstantI(IWineD3DDevice *iface, UINT StartRegister, int          *pConstantData, UINT Vector4iCount) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    
    return IWineD3DDeviceImpl_GetPixelShaderConstant(iface, 
            pConstantData, 
            This->updateStateBlock->pixelShaderConstantI, 
            WINESHADERCNST_INTEGER, 
            StartRegister, 
            Vector4iCount, 
            4 * sizeof(*pConstantData));
}

HRESULT WINAPI IWineD3DDeviceImpl_SetPixelShaderConstantF(IWineD3DDevice *iface, UINT StartRegister, CONST float  *pConstantData, UINT Vector4fCount) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    
    return IWineD3DDeviceImpl_SetPixelShaderConstant(iface, 
            This->updateStateBlock->pixelShaderConstantF, 
            pConstantData, 
            WINESHADERCNST_FLOAT, 
            StartRegister, 
            Vector4fCount, 
            4 * sizeof(*pConstantData));
}

HRESULT WINAPI IWineD3DDeviceImpl_GetPixelShaderConstantF(IWineD3DDevice *iface, UINT StartRegister, float        *pConstantData, UINT Vector4fCount) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    return IWineD3DDeviceImpl_GetPixelShaderConstant(iface, 
            pConstantData, 
            This->updateStateBlock->pixelShaderConstantF, 
            WINESHADERCNST_FLOAT, 
            StartRegister, 
            Vector4fCount, 
            4 * sizeof(*pConstantData));
}

HRESULT WINAPI IWineD3DDeviceImpl_SetPixelShaderConstantN(IWineD3DDevice *iface, UINT StartRegister, UINT VectorNCount){
    return IWineD3DDeviceImpl_SetPixelShaderConstant(iface, 
            NULL, 
            NULL, 
            WINESHADERCNST_NONE, 
            StartRegister, 
            VectorNCount, 
            0);
}

HRESULT WINAPI IWineD3DDeviceImpl_ProcessVertices(IWineD3DDevice *iface, UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IWineD3DVertexBuffer* pDestBuffer, IWineD3DVertexBuffer* pVertexDecl, DWORD Flags) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

/*****
 * Apply / Get / Set Texture Stage States
 * TODO: Verify against dx9 definitions
 *****/

/* NOTE: It's expected that this function is going to be called lots of times with the same stage active, so make it the callers responsibility to GLACTIVETEXTURE(Stage) for better state management. Set the correct Texture unit active before calling ApplyTextureStageState */
static void WINAPI IWineD3DDeviceImpl_ApplyTextureUnitState(IWineD3DDevice *iface, DWORD Stage, WINED3DTEXTURESTAGESTATETYPE Type) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    DWORD Value = This->updateStateBlock->textureState[Stage][Type];
    /* FIXME: Handle 3d textures? What if TSS value set before set texture? Need to reapply all values? */

    TRACE("(%p) : Stage=%ld, Type=%s(%d), Value=%ld\n", This, Stage, debug_d3dtexturestate(Type), Type, Value);

    /* Check that the stage is within limits  */
    if (Stage >= GL_LIMITS(textures) || Stage < 0) {
        TRACE("Attempt to access invalid texture rejected\n");
        return;
    }

    ENTER_GL();

    switch (Type) {
    case WINED3DTSS_ALPHAOP               :
    case WINED3DTSS_COLOROP               :
        /* nothing to do as moved to drawprim for now */
        break;
    case WINED3DTSS_ADDRESSW              :
#if 0 /* I'm not sure what D3D does about ADDRESSW appearing twice */
            if (Value < minLookup[WINELOOKUP_WARPPARAM] || Value > maxLookup[WINELOOKUP_WARPPARAM]) {
                FIXME("Unrecognized or unsupported D3DTADDRESS_* value %ld, state %d\n", Value, Type);

            } else {
                GLint wrapParm = stateLookup[WINELOOKUP_WARPPARAM][Value - minLookup[WINELOOKUP_WARPPARAM]];
                TRACE("Setting WRAP_R to %d for %x\n", wrapParm, This->stateBlock->textureDimensions[Stage]);
                glTexParameteri(This->stateBlock->textureDimensions[Stage], GL_TEXTURE_WRAP_R, wrapParm);
                checkGLcall("glTexParameteri(..., GL_TEXTURE_WRAP_R, wrapParm)");
            }
#endif
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
                glDisable(GL_TEXTURE_GEN_Q);
                checkGLcall("glDisable(GL_TEXTURE_GEN_S,T,R,Q)");
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
                        TRACE("WINED3DTSS_TCI_CAMERASPACENORMAL - Set eye plane\n");
        
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
                    TRACE("WINED3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR - Set eye plane\n");
    
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
                glDisable(GL_TEXTURE_GEN_Q);
                FIXME("Unhandled WINED3DTSS_TEXCOORDINDEX %lx\n", Value);
                break;
            }
        }
        break;

        /* Unhandled */
    case WINED3DTSS_TEXTURETRANSFORMFLAGS :
        set_texture_matrix((float *)&This->stateBlock->transforms[D3DTS_TEXTURE0 + Stage].u.m[0][0], Value, (This->stateBlock->textureState[Stage][WINED3DTSS_TEXCOORDINDEX] & 0xFFFF0000) != D3DTSS_TCI_PASSTHRU);
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

    return;
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
    This->updateStateBlock->set.textureState[Stage][Type]     = TRUE;
    This->updateStateBlock->textureState[Stage][Type]         = Value;

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

    oldTexture = This->updateStateBlock->textures[Stage];
    TRACE("(%p) : Stage(%ld), Texture (%p)\n", This, Stage, pTexture);

#if 0 /* TODO: check so vertex textures */
    if (Stage >= D3DVERTEXTEXTURESAMPLER && Stage <= D3DVERTEXTEXTURESAMPLER3){
        This->updateStateBlock->vertexTextures[Stage - D3DVERTEXTEXTURESAMPLER] = pTexture;
        return D3D_OK;
    }
#endif

    /* Reject invalid texture units */
    if (Stage >= GL_LIMITS(textures) || Stage < 0) {
        WARN("Attempt to access invalid texture rejected\n");
        return D3DERR_INVALIDCALL;
    }

    if(pTexture != NULL) {
        /* SetTexture isn't allowed on textures in D3DPOOL_SCRATCH; The same is
        *  the case for D3DPOOL_SYSTEMMEM textures unless WINED3DDEVCAPS_TEXTURESYSTEMMORY is set.
        *  We don't check the caps as GetDeviceCaps is inefficient and we don't set the cap anyway.
        */
        if(((IWineD3DTextureImpl*)pTexture)->resource.pool == D3DPOOL_SCRATCH || ((IWineD3DTextureImpl*)pTexture)->resource.pool == D3DPOOL_SYSTEMMEM) {
            WARN("(%p) Attempt to set scratch texture rejected\n", pTexture);
            return D3DERR_INVALIDCALL;
        }
    }

    oldTexture = This->updateStateBlock->textures[Stage];
    TRACE("GL_LIMITS %d\n",GL_LIMITS(textures));
    TRACE("(%p) : oldtexture(%p)\n", This,oldTexture);

    This->updateStateBlock->set.textures[Stage]     = TRUE;
    This->updateStateBlock->changed.textures[Stage] = TRUE;
    TRACE("(%p) : setting new texture to %p\n", This, pTexture);
    This->updateStateBlock->textures[Stage]         = pTexture;

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return D3D_OK;
    }

    /** NOTE: MSDN says that setTexture increases the reference count,
    * and the the application nust set the texture back to null (or have a leaky application),
    * This means we should pass the refcount up to the parent
     *******************************/
    if (NULL != This->updateStateBlock->textures[Stage]) {
        IWineD3DBaseTexture_AddRef(This->updateStateBlock->textures[Stage]);
    }

    if (NULL != oldTexture) {
        IWineD3DBaseTexture_Release(oldTexture);
    }

    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_GetTexture(IWineD3DDevice *iface, DWORD Stage, IWineD3DBaseTexture** ppTexture) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : (%ld /* Stage */,%p /* ppTexture */)\n", This, Stage, ppTexture);

    /* Reject invalid texture units */
    if (Stage >= GL_LIMITS(textures)) {
        TRACE("Attempt to access invalid texture rejected\n");
        return D3DERR_INVALIDCALL;
    }
    *ppTexture=This->updateStateBlock->textures[Stage];
    if (*ppTexture)
        IWineD3DBaseTexture_AddRef(*ppTexture);
    else
        return D3DERR_INVALIDCALL;
    return D3D_OK;
}

/*****
 * Get Back Buffer
 *****/
HRESULT WINAPI IWineD3DDeviceImpl_GetBackBuffer(IWineD3DDevice *iface, UINT iSwapChain, UINT BackBuffer, D3DBACKBUFFER_TYPE Type,
                                                IWineD3DSurface **ppBackBuffer) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DSwapChain *swapChain;
    HRESULT hr;

    TRACE("(%p) : BackBuf %d Type %d SwapChain %d returning %p\n", This, BackBuffer, Type, iSwapChain, *ppBackBuffer);

    hr = IWineD3DDeviceImpl_GetSwapChain(iface,  iSwapChain, &swapChain);
    if (hr == D3D_OK) {
        hr = IWineD3DSwapChain_GetBackBuffer(swapChain, BackBuffer, Type, ppBackBuffer);
            IWineD3DSwapChain_Release(swapChain);
    } else {
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
    } else {
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
    
    if (This->isRecordingState) {
        return D3DERR_INVALIDCALL;
    }
    
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IWineD3DStateBlockImpl));
    if (NULL == object ) {
        FIXME("(%p)Error allocating memory for stateblock\n", This);
        return E_OUTOFMEMORY;
    }
    TRACE("(%p) creted object %p\n", This, object);
    object->wineD3DDevice= This;
    /** FIXME: object->parent       = parent; **/
    object->parent       = NULL;
    object->blockType    = WINED3DSBT_ALL;
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
        if (D3D_OK == IWineD3DSurface_GetContainer(This->renderTarget, &IID_IWineD3DBaseTexture, (void **)&targetContainer)
            || D3D_OK == IWineD3DSurface_GetContainer(This->renderTarget, &IID_IWineD3DDevice, (void **)&targetContainer)) {
            TRACE("(%p) : Texture rendertarget %p\n", This ,This->renderTarget);
            /** always dirtify for now. we must find a better way to see that surface have been modified
            (Modifications should will only occur via draw-primitive, but we do need better locking
            switching to render-to-texture should remove the overhead though.
            */
            IWineD3DSurface_SetPBufferState(This->renderTarget, TRUE /* inPBuffer */, FALSE /* inTexture */);
            IWineD3DSurface_AddDirtyRect(This->renderTarget, NULL);
            IWineD3DSurface_PreLoad(This->renderTarget);
            IWineD3DSurface_SetPBufferState(This->renderTarget, FALSE /* inPBuffer */, FALSE /* inTexture */);
            IUnknown_Release(targetContainer);
        }
    }
    This->sceneEnded = TRUE;
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
        glClearColor(D3DCOLOR_R(Color),
                     D3DCOLOR_G(Color),
                     D3DCOLOR_B(Color),
                     D3DCOLOR_A(Color));
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
                      (((IWineD3DSurfaceImpl *)This->renderTarget)->currentDesc.Height - 
                      (This->stateBlock->viewport.Y + This->stateBlock->viewport.Height)),
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
    drawPrimitive(iface, PrimitiveType, PrimitiveCount, StartVertex, 0/* NumVertices */, -1 /* indxStart */,
                  0 /* indxSize */, NULL /* indxData */, 0 /* minIndex */);


    return D3D_OK;
}

/* TODO: baseVIndex needs to be provided from This->stateBlock->baseVertexIndex when called from d3d8 */
HRESULT  WINAPI  IWineD3DDeviceImpl_DrawIndexedPrimitive(IWineD3DDevice *iface,
                                                           D3DPRIMITIVETYPE PrimitiveType,
                                                           INT baseVIndex, UINT minIndex,
                                                           UINT NumVertices, UINT startIndex, UINT primCount) {

    IWineD3DDeviceImpl  *This = (IWineD3DDeviceImpl *)iface;
    UINT                 idxStride = 2;
    IWineD3DIndexBuffer *pIB;
    D3DINDEXBUFFER_DESC  IdxBufDsc;

    pIB = This->stateBlock->pIndexData;
    This->stateBlock->streamIsUP = FALSE;

    TRACE("(%p) : Type=(%d,%s), min=%d, CountV=%d, startIdx=%d, baseVidx=%d, countP=%d\n", This,
          PrimitiveType, debug_d3dprimitivetype(PrimitiveType),
          minIndex, NumVertices, startIndex, baseVIndex, primCount);

    IWineD3DIndexBuffer_GetDesc(pIB, &IdxBufDsc);
    if (IdxBufDsc.Format == WINED3DFMT_INDEX16) {
        idxStride = 2;
    } else {
        idxStride = 4;
    }

    drawPrimitive(iface, PrimitiveType, primCount, baseVIndex, NumVertices, startIndex,
                   idxStride, ((IWineD3DIndexBufferImpl *) pIB)->resource.allocatedMemory, minIndex);

    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_DrawPrimitiveUP(IWineD3DDevice *iface, D3DPRIMITIVETYPE PrimitiveType,
                                                    UINT PrimitiveCount, CONST void* pVertexStreamZeroData,
                                                    UINT VertexStreamZeroStride) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("(%p) : Type=(%d,%s), pCount=%d, pVtxData=%p, Stride=%d\n", This, PrimitiveType,
             debug_d3dprimitivetype(PrimitiveType),
             PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride);

    /* release the stream source */
    if (This->stateBlock->streamSource[0] != NULL) {
        IWineD3DVertexBuffer_Release(This->stateBlock->streamSource[0]);
    }

    /* Note in the following, it's not this type, but that's the purpose of streamIsUP */
    This->stateBlock->streamSource[0] = (IWineD3DVertexBuffer *)pVertexStreamZeroData;
    This->stateBlock->streamStride[0] = VertexStreamZeroStride;
    This->stateBlock->streamIsUP = TRUE;

    drawPrimitive(iface, PrimitiveType, PrimitiveCount, 0 /* start vertex */, 0  /* NumVertices */,
                  0 /* indxStart*/, 0 /* indxSize*/, NULL /* indxData */, 0 /* indxMin */);
    /* stream zero settings set to null at end, as per the msdn
            http://msdn.microsoft.com/archive/default.asp?url=/archive/en-us/directx9_c/directx/graphics/reference/d3d/interfaces/idirect3ddevice9/DrawPrimitiveUP.asp
    */
    This->stateBlock->streamStride[0] = 0;
    This->stateBlock->streamSource[0] = NULL;

    /*stream zero settings set to null at end, as per the msdn */
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_DrawIndexedPrimitiveUP(IWineD3DDevice *iface, D3DPRIMITIVETYPE PrimitiveType,
                                                             UINT MinVertexIndex, UINT NumVertices,
                                                             UINT PrimitiveCount, CONST void* pIndexData,
                                                             WINED3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,
                                                             UINT VertexStreamZeroStride) {
    int                 idxStride;
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("(%p) : Type=(%d,%s), MinVtxIdx=%d, NumVIdx=%d, PCount=%d, pidxdata=%p, IdxFmt=%d, pVtxdata=%p, stride=%d\n",
             This, PrimitiveType, debug_d3dprimitivetype(PrimitiveType),
             MinVertexIndex, NumVertices, PrimitiveCount, pIndexData,
             IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);

    if (IndexDataFormat == WINED3DFMT_INDEX16) {
        idxStride = 2;
    } else {
        idxStride = 4;
    }

    /* release the stream and index data */
    if (This->stateBlock->streamSource[0] != NULL) {
        IWineD3DVertexBuffer_Release(This->stateBlock->streamSource[0]);
    }
    if (This->stateBlock->pIndexData) {
        IWineD3DIndexBuffer_Release(This->stateBlock->pIndexData);
    }

    /* Note in the following, it's not this type, but that's the purpose of streamIsUP */
    This->stateBlock->streamSource[0] = (IWineD3DVertexBuffer *)pVertexStreamZeroData;
    This->stateBlock->streamIsUP = TRUE;
    This->stateBlock->streamStride[0] = VertexStreamZeroStride;

    drawPrimitive(iface, PrimitiveType, PrimitiveCount, 0 /* vertexStart */, NumVertices, 0 /* indxStart */, idxStride, pIndexData, MinVertexIndex);
    /* stream zero settings set to null at end as per the msdn
    http://msdn.microsoft.com/archive/default.asp?url=/archive/en-us/directx9_c/directx/graphics/reference/d3d/interfaces/idirect3ddevice9/DrawPrimitiveUP.asp
    */

    /* stream zero settings set to null at end as per the msdn */
    This->stateBlock->streamSource[0] = NULL;
    This->stateBlock->streamStride[0] = 0;

    return D3D_OK;
}

 /* Yet another way to update a texture, some apps use this to load default textures instead of using surface/texture lock/unlock */
HRESULT WINAPI IWineD3DDeviceImpl_UpdateTexture (IWineD3DDevice *iface, IWineD3DBaseTexture *pSourceTexture,  IWineD3DBaseTexture *pDestinationTexture){
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    HRESULT hr = D3D_OK;
    WINED3DRESOURCETYPE sourceType;
    WINED3DRESOURCETYPE destinationType;
    int i ,levels;

    /* TODO: think about moving the code into IWineD3DBaseTexture  */

    TRACE("(%p) Source %p Destination %p\n", This, pSourceTexture, pDestinationTexture);

    /* verify that the source and destination textures aren't NULL */
    if (NULL == pSourceTexture || NULL == pDestinationTexture) {
        WARN("(%p) : source (%p) and destination (%p) textures must not be NULL, returning D3DERR_INVALIDCALL\n",
             This, pSourceTexture, pDestinationTexture);
        hr = D3DERR_INVALIDCALL;
    }

    if (pSourceTexture == pDestinationTexture) {
        WARN("(%p) : source (%p) and destination (%p) textures must be different, returning D3DERR_INVALIDCALL\n",
             This, pSourceTexture, pDestinationTexture);
        hr = D3DERR_INVALIDCALL;
    }
    /* Verify that the source and destination textures are the same type */
    sourceType      = IWineD3DBaseTexture_GetType(pSourceTexture);
    destinationType = IWineD3DBaseTexture_GetType(pDestinationTexture);

    if (sourceType != destinationType) {
        WARN("(%p) Sorce and destination types must match, returning D3DERR_INVALIDCALL\n",
             This);
        hr = D3DERR_INVALIDCALL;
    }

    /* check that both textures have the identical numbers of levels  */
    if (IWineD3DBaseTexture_GetLevelCount(pDestinationTexture)  != IWineD3DBaseTexture_GetLevelCount(pSourceTexture)) {
        WARN("(%p) : source (%p) and destination (%p) textures must have identicle numbers of levels, returning D3DERR_INVALIDCALL\n", This, pSourceTexture, pDestinationTexture);
        hr = D3DERR_INVALIDCALL;
    }

    if (D3D_OK == hr) {

        /* Make sure that the destination texture is loaded */
        IWineD3DBaseTexture_PreLoad(pDestinationTexture);

        /* Update every surface level of the texture */
        levels = IWineD3DBaseTexture_GetLevelCount(pDestinationTexture);

        switch (sourceType) {
        case WINED3DRTYPE_TEXTURE:
            {
                IWineD3DSurface *srcSurface;
                IWineD3DSurface *destSurface;

                for (i = 0 ; i < levels ; ++i) {
                    IWineD3DTexture_GetSurfaceLevel((IWineD3DTexture *)pSourceTexture,      i, &srcSurface);
                    IWineD3DTexture_GetSurfaceLevel((IWineD3DTexture *)pDestinationTexture, i, &destSurface);
                    hr = IWineD3DDevice_UpdateSurface(iface, srcSurface, NULL, destSurface, NULL);
                    IWineD3DSurface_Release(srcSurface);
                    IWineD3DSurface_Release(destSurface);
                    if (D3D_OK != hr) {
                        WARN("(%p) : Call to update surface failed\n", This);
                        return hr;
                    }
                }
            }
            break;
        case WINED3DRTYPE_CUBETEXTURE:
            {
                IWineD3DSurface *srcSurface;
                IWineD3DSurface *destSurface;
                D3DCUBEMAP_FACES faceType;

                for (i = 0 ; i < levels ; ++i) {
                    /* Update each cube face */
                    for (faceType = D3DCUBEMAP_FACE_POSITIVE_X; faceType <= D3DCUBEMAP_FACE_NEGATIVE_Z; ++faceType){
                        hr = IWineD3DCubeTexture_GetCubeMapSurface((IWineD3DCubeTexture *)pSourceTexture,      faceType, i, &srcSurface);
                        if (D3D_OK != hr) {
                            FIXME("(%p) : Failed to get src cube surface facetype %d, level %d\n", This, faceType, i);
                        } else {
                            TRACE("Got srcSurface %p\n", srcSurface);
                        }
                        hr = IWineD3DCubeTexture_GetCubeMapSurface((IWineD3DCubeTexture *)pDestinationTexture, faceType, i, &destSurface);
                        if (D3D_OK != hr) {
                            FIXME("(%p) : Failed to get src cube surface facetype %d, level %d\n", This, faceType, i);
                        } else {
                            TRACE("Got desrSurface %p\n", destSurface);
                        }
                        hr = IWineD3DDevice_UpdateSurface(iface, srcSurface, NULL, destSurface, NULL);
                        IWineD3DSurface_Release(srcSurface);
                        IWineD3DSurface_Release(destSurface);
                        if (D3D_OK != hr) {
                            WARN("(%p) : Call to update surface failed\n", This);
                            return hr;
                        }
                    }
                }
            }
            break;
#if 0 /* TODO: Add support for volume textures */
        case WINED3DRTYPE_VOLUMETEXTURE:
            {
                IWineD3DVolume  srcVolume  = NULL;
                IWineD3DSurface destVolume = NULL;

                for (i = 0 ; i < levels ; ++i) {
                    IWineD3DVolumeTexture_GetVolume((IWineD3DVolumeTexture *)pSourceTexture,      i, &srcVolume);
                    IWineD3DVolumeTexture_GetVolume((IWineD3DVolumeTexture *)pDestinationTexture, i, &destVolume);
                    hr =  IWineD3DFoo_UpdateVolume(iface, srcVolume, NULL, destVolume, NULL);
                    IWineD3DVolume_Release(srcSurface);
                    IWineD3DVolume_Release(destSurface);
                    if (D3D_OK != hr) {
                        WARN("(%p) : Call to update volume failed\n", This);
                        return hr;
                    }
                }
            }
            break;
#endif
        default:
            FIXME("(%p) : Unsupported source and destination type\n", This);
            hr = D3DERR_INVALIDCALL;
        }
    }

    return hr;
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
    /** TODO: remove remove casts to IWineD3DSurfaceImpl *
    *  NOTE It may be best to move the code into surface to occomplish this
    ****************************************/

    WINED3DSURFACE_DESC surfaceDesc;
    unsigned int surfaceWidth, surfaceHeight;
    glDescriptor *targetGlDescription  = NULL;
    glDescriptor *surfaceGlDescription = NULL;
    IWineD3DSwapChainImpl *container = NULL;
    
    IWineD3DSurface_GetGlDesc(pRenderTarget, &targetGlDescription);
    IWineD3DSurface_GetGlDesc(pSurface,      &surfaceGlDescription);
    memset(&surfaceDesc, 0, sizeof(surfaceDesc));

    surfaceDesc.Width  = &surfaceWidth;
    surfaceDesc.Height = &surfaceHeight;
    IWineD3DSurface_GetDesc(pSurface, &surfaceDesc);
   /* check to see if it's the backbuffer or the frontbuffer being requested (to make sure the data is up to date)*/

    /* Ok, I may need to setup some kind of active swapchain reference on the device */
    IWineD3DSurface_GetContainer(pRenderTarget, &IID_IWineD3DSwapChain, (void **)&container);
    ENTER_GL();
    /* TODO: opengl Context switching for swapchains etc... */
    if (NULL != container  || pRenderTarget == This->renderTarget || pRenderTarget == This->depthStencilBuffer) {
        if (NULL != container  && (pRenderTarget == container->backBuffer)) {
            glReadBuffer(GL_BACK);
            vcheckGLcall("glReadBuffer(GL_BACK)");
        } else if ((NULL != container  && (pRenderTarget == container->frontBuffer)) || (pRenderTarget == This->renderTarget)) {
            glReadBuffer(GL_FRONT);
            vcheckGLcall("glReadBuffer(GL_FRONT)");
        } else if (pRenderTarget == This->depthStencilBuffer) {
            FIXME("Reading of depthstencil not yet supported\n");
        }

        glReadPixels(surfaceGlDescription->target,
                    surfaceGlDescription->level,
                    surfaceWidth,
                    surfaceHeight,
                    surfaceGlDescription->glFormat,
                    surfaceGlDescription->glType,
                    (void *)IWineD3DSurface_GetData(pSurface));
        vcheckGLcall("glReadPixels(...)");
        if(NULL != container ){
            IWineD3DSwapChain_Release((IWineD3DSwapChain*) container);
        }
    } else {
        IWineD3DBaseTexture *container;
        GLenum textureDimensions = GL_TEXTURE_2D;

        if (D3D_OK == IWineD3DSurface_GetContainer(pSurface, &IID_IWineD3DBaseTexture, (void **)&container)) {
            textureDimensions = IWineD3DBaseTexture_GetTextureDimensions(container);
            IWineD3DBaseTexture_Release(container);
        }
        /* TODO: 2D -> Cube surface coppies etc.. */
        if (surfaceGlDescription->target != textureDimensions) {
            FIXME("(%p) : Texture dimension mismatch\n", This);
        }
        glEnable(textureDimensions);
        vcheckGLcall("glEnable(GL_TEXTURE_...)");
        /* FIXME: this isn't correct, it need to add a dirty rect if nothing else... */
        glBindTexture(targetGlDescription->target, targetGlDescription->textureName);
        vcheckGLcall("glBindTexture");
        glGetTexImage(surfaceGlDescription->target,
                        surfaceGlDescription->level,
                        surfaceGlDescription->glFormat,
                        surfaceGlDescription->glType,
                        (void *)IWineD3DSurface_GetData(pSurface));
        glDisable(textureDimensions);
        vcheckGLcall("glDisable(GL_TEXTURE_...)");

    }
    LEAVE_GL();
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
    /* TODO: If the window is minimized then validate device should return something other than d3d_ok */
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT  WINAPI  IWineD3DDeviceImpl_SetPaletteEntries(IWineD3DDevice *iface, UINT PaletteNumber, CONST PALETTEENTRY* pEntries) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    int j;
    TRACE("(%p) : PaletteNumber %u\n", This, PaletteNumber);
    if ( PaletteNumber < 0 || PaletteNumber >= MAX_PALETTES) {
        WARN("(%p) : (%u) Out of range 0-%u, returning Invalid Call\n", This, PaletteNumber, MAX_PALETTES);
        return D3DERR_INVALIDCALL;
    }
    for (j = 0; j < 256; ++j) {
        This->palettes[PaletteNumber][j].peRed   = pEntries[j].peRed;
        This->palettes[PaletteNumber][j].peGreen = pEntries[j].peGreen;
        This->palettes[PaletteNumber][j].peBlue  = pEntries[j].peBlue;
        This->palettes[PaletteNumber][j].peFlags = pEntries[j].peFlags;
    }
    TRACE("(%p) : returning\n", This);
    return D3D_OK;
}

HRESULT  WINAPI  IWineD3DDeviceImpl_GetPaletteEntries(IWineD3DDevice *iface, UINT PaletteNumber, PALETTEENTRY* pEntries) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    int j;
    TRACE("(%p) : PaletteNumber %u\n", This, PaletteNumber);
    if ( PaletteNumber < 0 || PaletteNumber >= MAX_PALETTES) {
        WARN("(%p) : (%u) Out of range 0-%u, returning Invalid Call\n", This, PaletteNumber, MAX_PALETTES);
        return D3DERR_INVALIDCALL;
    }
    for (j = 0; j < 256; ++j) {
        pEntries[j].peRed   = This->palettes[PaletteNumber][j].peRed;
        pEntries[j].peGreen = This->palettes[PaletteNumber][j].peGreen;
        pEntries[j].peBlue  = This->palettes[PaletteNumber][j].peBlue;
        pEntries[j].peFlags = This->palettes[PaletteNumber][j].peFlags;
    }
    TRACE("(%p) : returning\n", This);
    return D3D_OK;
}

HRESULT  WINAPI  IWineD3DDeviceImpl_SetCurrentTexturePalette(IWineD3DDevice *iface, UINT PaletteNumber) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : PaletteNumber %u\n", This, PaletteNumber);
    if ( PaletteNumber < 0 || PaletteNumber >= MAX_PALETTES) {
        WARN("(%p) : (%u) Out of range 0-%u, returning Invalid Call\n", This, PaletteNumber, MAX_PALETTES);
        return D3DERR_INVALIDCALL;
    }
    /*TODO: stateblocks */
    This->currentPalette = PaletteNumber;
    TRACE("(%p) : returning\n", This);
    return D3D_OK;
}

HRESULT  WINAPI  IWineD3DDeviceImpl_GetCurrentTexturePalette(IWineD3DDevice *iface, UINT* PaletteNumber) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    if (PaletteNumber == NULL) {
        WARN("(%p) : returning Invalid Call\n", This);
        return D3DERR_INVALIDCALL;
    }
    /*TODO: stateblocks */
    *PaletteNumber = This->currentPalette;
    TRACE("(%p) : returning  %u\n", This, *PaletteNumber);
    return D3D_OK;
}

HRESULT  WINAPI  IWineD3DDeviceImpl_SetSoftwareVertexProcessing(IWineD3DDevice *iface, BOOL bSoftware) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    static BOOL showFixmes = TRUE;
    if (showFixmes) {
        FIXME("(%p) : stub\n", This);
        showFixmes = FALSE;
    }

    This->softwareVertexProcessing = bSoftware;
    return D3D_OK;
}


BOOL     WINAPI  IWineD3DDeviceImpl_GetSoftwareVertexProcessing(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    static BOOL showFixmes = TRUE;
    if (showFixmes) {
        FIXME("(%p) : stub\n", This);
        showFixmes = FALSE;
    }
    return This->softwareVertexProcessing;
}


HRESULT  WINAPI  IWineD3DDeviceImpl_GetRasterStatus(IWineD3DDevice *iface, UINT iSwapChain, D3DRASTER_STATUS* pRasterStatus) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DSwapChain *swapChain;
    HRESULT hr;

    TRACE("(%p) :  SwapChain %d returning %p\n", This, iSwapChain, pRasterStatus);

    hr = IWineD3DDeviceImpl_GetSwapChain(iface,  iSwapChain, (IWineD3DSwapChain **)&swapChain);
    if(hr == D3D_OK){
        hr = IWineD3DSwapChain_GetRasterStatus(swapChain, pRasterStatus);
        IWineD3DSwapChain_Release(swapChain);
    }else{
        FIXME("(%p) IWineD3DSwapChain_GetRasterStatus returned in error\n", This);
    }
    return hr;
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
    int srcWidth, srcHeight;
    unsigned int  srcSurfaceWidth, srcSurfaceHeight, destSurfaceWidth, destSurfaceHeight;
    WINED3DFORMAT destFormat, srcFormat;
    UINT          destSize;
    int destLeft, destTop;
    D3DPOOL       srcPool, destPool;
    int offset    = 0;
    int rowoffset = 0; /* how many bytes to add onto the end of a row to wraparound to the beginning of the next */
    glDescriptor *glDescription = NULL;
    GLenum textureDimensions = GL_TEXTURE_2D;
    IWineD3DBaseTexture *baseTexture;

    WINED3DSURFACE_DESC  winedesc;

    TRACE("(%p) : Source (%p)  Rect (%p) Destination (%p) Point(%p)\n", This, pSourceSurface, pSourceRect, pDestinationSurface, pDestPoint);
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
        WARN("source %p must be SYSTEMMEM and dest %p must be DEFAULT, returning D3DERR_INVALIDCALL\n", pSourceSurface, pDestinationSurface);
        return D3DERR_INVALIDCALL;
    }

    if (destFormat == WINED3DFMT_UNKNOWN) {
        TRACE("(%p) : Converting destination surface from WINED3DFMT_UNKNOWN to the source format\n", This);
        IWineD3DSurface_SetFormat(pDestinationSurface, srcFormat);

        /* Get the update surface description */
        IWineD3DSurface_GetDesc(pDestinationSurface, &winedesc);
    }

    /* Make sure the surface is loaded and up to date */
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
                    ,data /* could be quicker using */
                );
            data += rowoffset;
        }

    } else { /* Full width, so just write out the whole texture */

        if (WINED3DFMT_DXT1 == destFormat ||
            WINED3DFMT_DXT2 == destFormat ||
            WINED3DFMT_DXT3 == destFormat ||
            WINED3DFMT_DXT4 == destFormat ||
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
            if (NP2_REPACK == wined3d_settings.nonpower2_mode) {

                /* some applications cannot handle odd pitches returned by soft non-power2, so we have
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
            } else {

                /* not repacked, the data returned by IWineD3DSurface_GetData is pow2Width x pow2Height */
                glTexSubImage2D(glDescription->target
                    ,glDescription->level
                    ,destLeft
                    ,destTop
                    ,((IWineD3DSurfaceImpl *)pSourceSurface)->pow2Width
                    ,((IWineD3DSurfaceImpl *)pSourceSurface)->pow2Height
                    ,glDescription->glFormat
                    ,glDescription->glType
                    ,IWineD3DSurface_GetData(pSourceSurface)
                );
            }

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

/* Used by DirectX 8 */
HRESULT  WINAPI  IWineD3DDeviceImpl_CopyRects(IWineD3DDevice *iface,
                                                IWineD3DSurface* pSourceSurface,      CONST RECT* pSourceRectsArray, UINT cRects,
                                                IWineD3DSurface* pDestinationSurface, CONST POINT* pDestPointsArray) {

    IWineD3DDeviceImpl  *This = (IWineD3DDeviceImpl *)iface;
    HRESULT              hr = D3D_OK;
    WINED3DFORMAT        srcFormat, destFormat;
    UINT                 srcWidth,  destWidth;
    UINT                 srcHeight, destHeight;
    UINT                 srcSize;
    WINED3DSURFACE_DESC  winedesc;

    TRACE("(%p) pSrcSur=%p, pSourceRects=%p, cRects=%d, pDstSur=%p, pDestPtsArr=%p\n", This,
          pSourceSurface, pSourceRectsArray, cRects, pDestinationSurface, pDestPointsArray);


    /* Check that the source texture is in D3DPOOL_SYSTEMMEM and the destination texture is in D3DPOOL_DEFAULT */
    memset(&winedesc, 0, sizeof(winedesc));

    winedesc.Format = &srcFormat;
    winedesc.Width  = &srcWidth;
    winedesc.Height = &srcHeight;
    winedesc.Size   = &srcSize;
    IWineD3DSurface_GetDesc(pSourceSurface, &winedesc);

    winedesc.Format = &destFormat;
    winedesc.Width  = &destWidth;
    winedesc.Height = &destHeight;
    winedesc.Size   = NULL;
    IWineD3DSurface_GetDesc(pDestinationSurface, &winedesc);

    /* Check that the source and destination formats match */
    if (srcFormat != destFormat && WINED3DFMT_UNKNOWN != destFormat) {
        WARN("(%p) source %p format must match the dest %p format, returning D3DERR_INVALIDCALL\n", This, pSourceSurface, pDestinationSurface);
        return D3DERR_INVALIDCALL;
    } else if (WINED3DFMT_UNKNOWN == destFormat) {
        TRACE("(%p) : Converting destination surface from WINED3DFMT_UNKNOWN to the source format\n", This);
        IWineD3DSurface_SetFormat(pDestinationSurface, srcFormat);
        destFormat = srcFormat;
    }

    /* Quick if complete copy ... */
    if (cRects == 0 && pSourceRectsArray == NULL && pDestPointsArray == NULL) {

        if (srcWidth == destWidth && srcHeight == destHeight) {
            D3DLOCKED_RECT lrSrc;
            D3DLOCKED_RECT lrDst;
            IWineD3DSurface_LockRect(pSourceSurface,      &lrSrc, NULL, WINED3DLOCK_READONLY);
            IWineD3DSurface_LockRect(pDestinationSurface, &lrDst, NULL, 0L);
            TRACE("Locked src and dst, Direct copy as surfaces are equal, w=%d, h=%d\n", srcWidth, srcHeight);

            memcpy(lrDst.pBits, lrSrc.pBits, srcSize);

            IWineD3DSurface_UnlockRect(pSourceSurface);
            IWineD3DSurface_UnlockRect(pDestinationSurface);
            TRACE("Unlocked src and dst\n");

        } else {

            FIXME("Wanted to copy all surfaces but size not compatible, returning D3DERR_INVALIDCALL\n");
            hr = D3DERR_INVALIDCALL;
         }

    } else {

        if (NULL != pSourceRectsArray && NULL != pDestPointsArray) {

            int bytesPerPixel = ((IWineD3DSurfaceImpl *) pSourceSurface)->bytesPerPixel;
            unsigned int i;

            /* Copy rect by rect */
            for (i = 0; i < cRects; ++i) {
                CONST RECT*  r = &pSourceRectsArray[i];
                CONST POINT* p = &pDestPointsArray[i];
                int copyperline;
                int j;
                D3DLOCKED_RECT lrSrc;
                D3DLOCKED_RECT lrDst;
                RECT dest_rect;

                TRACE("Copying rect %d (%ld,%ld),(%ld,%ld) -> (%ld,%ld)\n", i, r->left, r->top, r->right, r->bottom, p->x, p->y);
                if (srcFormat == WINED3DFMT_DXT1) {
                    copyperline = ((r->right - r->left) * bytesPerPixel) / 2; /* DXT1 is half byte per pixel */
                } else {
                    copyperline = ((r->right - r->left) * bytesPerPixel);
                }

                IWineD3DSurface_LockRect(pSourceSurface, &lrSrc, r, WINED3DLOCK_READONLY);
                dest_rect.left  = p->x;
                dest_rect.top   = p->y;
                dest_rect.right = p->x + (r->right - r->left);
                dest_rect.bottom= p->y + (r->bottom - r->top);
                IWineD3DSurface_LockRect(pDestinationSurface, &lrDst, &dest_rect, 0L);
                TRACE("Locked src and dst\n");

                /* Find where to start */
                for (j = 0; j < (r->bottom - r->top - 1); ++j) {
                    memcpy((char*) lrDst.pBits + (j * lrDst.Pitch), (char*) lrSrc.pBits + (j * lrSrc.Pitch), copyperline);
                }
                IWineD3DSurface_UnlockRect(pSourceSurface);
                IWineD3DSurface_UnlockRect(pDestinationSurface);
                TRACE("Unlocked src and dst\n");
            }
        } else {
            FIXME("Wanted to copy partial surfaces not implemented, returning D3DERR_INVALIDCALL\n");
            hr = D3DERR_INVALIDCALL;
        }
    }

    return hr;
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
    TRACE("(%p) Handle(%d) noSegs(%p) rectpatch(%p)\n", This, Handle, pNumSegs, pRectPatchInfo);
    FIXME("(%p) : Stub\n", This);
    return D3D_OK;

}

/* http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/reference/d3d/interfaces/idirect3ddevice9/DrawTriPatch.asp */
HRESULT WINAPI IWineD3DDeviceImpl_DrawTriPatch(IWineD3DDevice *iface, UINT Handle, CONST float* pNumSegs, CONST D3DTRIPATCH_INFO* pTriPatchInfo) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) Handle(%d) noSegs(%p) tripatch(%p)\n", This, Handle, pNumSegs, pTriPatchInfo);
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
    /* I couldn't find a 'really' quick way of doing this in openGl so here goes
    fill a surface with a block of color!  */
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    /* TODO: get rid of the use of IWineD3DSurfaceImpl, move code into surface.c */
    IWineD3DSurfaceImpl *surface = (IWineD3DSurfaceImpl *)pSurface;
    IWineD3DSwapChainImpl *container = NULL;
    BOOL isRenderTarget = FALSE;
    unsigned int width, height;
    unsigned int top, left;
    unsigned int u, v;
    DWORD       *data;
    TRACE("(%p) Colour fill Surface: %p rect: %p color: %ld\n", This, pSurface, pRect, color);

    if (surface->resource.pool != D3DPOOL_DEFAULT) {
        FIXME("call to colorfill with non D3DPOOL_DEFAULT surface\n");
        return D3DERR_INVALIDCALL;
    }

    /* TODO: get rid of IWineD3DSwapChainImpl reference, a 'context' manager may help with this */
    if (D3D_OK == IWineD3DSurface_GetContainer(pSurface, &IID_IWineD3DSwapChain, (void **)&container) || pSurface == This->renderTarget) {
        if (WINED3DUSAGE_RENDERTARGET & surface->resource.usage) {
            /* TODO: make sure we set everything back to the way it was, and context management!
                glGetIntegerv(GL_READ_BUFFER, &prev_read);
                vcheckGLcall("glIntegerv");
                glGetIntegerv(GL_PACK_SWAP_BYTES, &prev_store);
                vcheckGLcall("glIntegerv");
            */
            TRACE("Color fill to render targets may cause some graphics issues\n");
            if (pSurface == container->frontBuffer) {
                glDrawBuffer(GL_FRONT);
            } else {
                glDrawBuffer(GL_BACK);
            }
        } else {
            if (WINED3DUSAGE_DEPTHSTENCIL & surface->resource.usage) {
                FIXME("colouring of depth_stencil? %p buffers is not yet supported? %ld\n", surface, surface->resource.usage);
            } else {
                FIXME("(%p) : Regression %ld %p %p\n", This, surface->resource.usage, pSurface, This->renderTarget);
            }
            if (container != NULL) {
                IWineD3DSwapChain_Release((IWineD3DSwapChain *)container);
            }
            /* we can use GL_STENCIL_INDEX etc...*/
            return D3D_OK;
        }
        if (container != NULL) {
            IWineD3DSwapChain_Release((IWineD3DSwapChain *)container);
        }
        isRenderTarget = TRUE;
    }
    /* TODO: drawing to GL_FRONT and GL_BACK */
    /* TODO: see if things can be speeded up by using the correct
     * colour model of the target texture from the start (16 bit graphics on 32 X are slow anyway!) */
    if (pRect == NULL) {
        top    = 0;
        left   = 0;
        width  = surface->currentDesc.Width;
        height = surface->currentDesc.Height;
    } else {
        left   = pRect->x1;
        top    = pRect->y1;
        width  = pRect->x2 - left;
        height = pRect->y2 - top;
    }

    data = HeapAlloc(GetProcessHeap(), 0, 4 * width);
    /* Create a 'line' of color color, in the correct format for the surface */
    for (u = 0 ; u < width ; u ++) {
            data[u] = color;
    }

    ENTER_GL();
    if (isRenderTarget == FALSE) {
        glDescriptor *glDesc;
        IWineD3DSurface_PreLoad(pSurface);

        /* draw a block of the coloured line on the sufrace */
        IWineD3DSurface_GetGlDesc(pSurface, &glDesc);
        for (v = 0 ; v< height;v++) {
            glTexSubImage2D(glDesc->target
                            ,glDesc->level /* level */
                            ,left
                            ,top + v
                            ,width
                            ,1 /* height */
                            ,GL_RGBA
                            ,GL_UNSIGNED_BYTE
                            ,data
                            );
        }
        checkGLcall("glTexSubImage2D");
        /* clean up */
        glDisable(glDesc->target);
    } else {
        /** FIXME: Using GLClear may be faster **/
        glRasterPos2i(left, top);
        glPixelZoom((float)width ,(float)height);
        glDrawPixels(1, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);
        checkGLcall("glDrawPixels");
    }
    HeapFree(GetProcessHeap(), 0, data);
    LEAVE_GL();
    return D3D_OK;

}

/* rendertarget and deptth stencil functions */
HRESULT  WINAPI  IWineD3DDeviceImpl_GetRenderTarget(IWineD3DDevice* iface,DWORD RenderTargetIndex, IWineD3DSurface **ppRenderTarget) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    /* FIXME: Implelent RenderTargetIndex >0 */
    if(RenderTargetIndex > 0)
        FIXME("(%p) : RenderTargetIndex %ld >0 not currently supported\n", This, RenderTargetIndex);

    *ppRenderTarget = This->renderTarget;
    TRACE("(%p) : RenderTarget %ld Index returning %p\n", This, RenderTargetIndex, *ppRenderTarget);
    /* Note inc ref on returned surface */
    if(*ppRenderTarget != NULL)
        IWineD3DSurface_AddRef(*ppRenderTarget);
    return D3D_OK;
}

HRESULT  WINAPI  IWineD3DDeviceImpl_GetDepthStencilSurface(IWineD3DDevice* iface, IWineD3DSurface **ppZStencilSurface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    *ppZStencilSurface = This->depthStencilBuffer;
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

HRESULT WINAPI IWineD3DDeviceImpl_SetRenderTarget(IWineD3DDevice *iface, DWORD RenderTargetIndex, IWineD3DSurface *pRenderTarget) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    HRESULT  hr = D3D_OK;
    WINED3DVIEWPORT viewport;

    TRACE("(%p) Swapping rendertarget\n",This);
    if (RenderTargetIndex > 0) {
        FIXME("(%p) Render targets other than the first are not supported\n",This);
        RenderTargetIndex = 0;
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
    /* TODO: replace Impl* usage with interface usage */
    if (!((IWineD3DSurfaceImpl *)pRenderTarget)->resource.usage & WINED3DUSAGE_RENDERTARGET) {
        FIXME("(%p)Trying to set the render target to a surface(%p) that wasn't created with a usage of WINED3DUSAGE_RENDERTARGET\n",This ,pRenderTarget);
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

        if (FALSE == This->sceneEnded) {
            IWineD3DDevice_EndScene(iface);
        }
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
    } else {
        FIXME("Unknown error setting the render target\n");
    }
    This->sceneEnded = FALSE;
    return hr;
}

HRESULT WINAPI IWineD3DDeviceImpl_SetDepthStencilSurface(IWineD3DDevice *iface, IWineD3DSurface *pNewZStencil) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    HRESULT  hr = D3D_OK;
    IWineD3DSurface *tmp;

    TRACE("(%p) Swapping z-buffer\n",This);

    if (pNewZStencil == This->stencilBufferTarget) {
        TRACE("Trying to do a NOP SetRenderTarget operation\n");
    } else {
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

    return hr;
}


#ifdef GL_VERSION_1_3
/* Internal functions not in DirectX */
 /** TODO: move this off to the opengl context manager
 *(the swapchain doesn't need to know anything about offscreen rendering!)
  ****************************************************/

HRESULT WINAPI IWineD3DDeviceImpl_CleanRender(IWineD3DDevice* iface, IWineD3DSwapChainImpl *swapchain)
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("(%p), %p\n", This, swapchain);

    if (swapchain->win != swapchain->drawable) {
        /* Set everything back the way it ws */
        swapchain->render_ctx = swapchain->glCtx;
        swapchain->drawable   = swapchain->win;
    }
    return D3D_OK;
}

/* TODO: move this off into a context manager so that GLX_ATI_render_texture and other types of surface can be used. */
HRESULT WINAPI IWineD3DDeviceImpl_FindGLContext(IWineD3DDevice *iface, IWineD3DSurface *pSurface, glContext **context) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    int i;
    unsigned int width;
    unsigned int height;
    WINED3DFORMAT format;
    WINED3DSURFACE_DESC surfaceDesc;
    memset(&surfaceDesc, 0, sizeof(surfaceDesc));
    surfaceDesc.Width  = &width;
    surfaceDesc.Height = &height;
    surfaceDesc.Format = &format;
    IWineD3DSurface_GetDesc(pSurface, &surfaceDesc);
    *context = NULL;
    /* I need a get width/height function (and should do something with the format) */
    for (i = 0; i < CONTEXT_CACHE; ++i) {
        /** NOTE: the contextCache[i].pSurface == pSurface check ceates onepbuffer per surface
        ATI cards don't destroy pbuffers, but as soon as resource releasing callbacks are inplace
        the pSurface can be set to 0 allowing it to be reused from cache **/
        if (This->contextCache[i].Width == width && This->contextCache[i].Height == height
          && (pbuffer_per_surface == FALSE || This->contextCache[i].pSurface == pSurface || This->contextCache[i].pSurface == NULL)) {
            *context = &This->contextCache[i];
            break;
        }
        if (This->contextCache[i].Width == 0) {
            This->contextCache[i].pSurface = pSurface;
            This->contextCache[i].Width    = width;
            This->contextCache[i].Height   = height;
            *context = &This->contextCache[i];
            break;
        }
    }
    if (i == CONTEXT_CACHE) {
        int minUsage = 0x7FFFFFFF; /* MAX_INT */
        glContext *dropContext = 0;
        for (i = 0; i < CONTEXT_CACHE; i++) {
            if (This->contextCache[i].usedcount < minUsage) {
                dropContext = &This->contextCache[i];
                minUsage = This->contextCache[i].usedcount;
            }
        }
        /* clean up the context (this doesn't work for ATI at the moment */
#if 0
        glXDestroyContext(swapchain->display, dropContext->context);
        glXDestroyPbuffer(swapchain->display, dropContext->drawable);
#endif
        FIXME("Leak\n");
        dropContext->Width = 0;
        dropContext->pSurface = pSurface;
        *context = dropContext;
    } else {
        if (++This->contextCache[i].usedcount == 0x7FFFFFFF /* MAX_INT */ - 1 ) {
          for (i = 0; i < CONTEXT_CACHE; i++) {
             This->contextCache[i].usedcount = max(0, This->contextCache[i].usedcount - (0x7FFFFFFF /* MAX_INT */ >> 1));
          }
        }
    }
    if (*context != NULL)
        return D3D_OK;
    else
        return E_OUTOFMEMORY;
}
#endif

/** FIXME: This is currently used called whenever SetRenderTarget or SetStencilBuffer are called
* the functionality needs splitting up so that we don't do more than we should do.
* this only seems to impact performance a little.
 ******************************/
HRESULT WINAPI IWineD3DDeviceImpl_ActiveRender(IWineD3DDevice* iface,
                                               IWineD3DSurface *RenderSurface) {
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

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DSurface *StencilSurface = This->stencilBufferTarget;
    IWineD3DSurface *tmp;
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
    PUSH2(GLX_DOUBLEBUFFER,  TRUE);
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

    if (NULL != cfgs) {
#ifdef EXTRA_TRACES
        int i;
        for (i = 0; i < nCfgs; ++i) {
            TRACE("for (%u,%s)/(%u,%s) found config[%d]@%p\n", BackBufferFormat,
            debug_d3dformat(BackBufferFormat), StencilBufferFormat,
            debug_d3dformat(StencilBufferFormat), i, cfgs[i]);
        }

        if (NULL != This->renderTarget) {
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
        }
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

            IWineD3DDeviceImpl_CleanRender(iface, (IWineD3DSwapChainImpl *)currentSwapchain);
        }
        checkGLcall("glXMakeContextCurrent");

        IWineD3DSwapChain_Release((IWineD3DSwapChain *)swapchain);
    }
    else if (pbuffer_support == TRUE && cfgs != NULL /* && some test to make sure that opengl supports pbuffers */) {

        /** ********************************************************************
        * This is a quickly hacked out implementation of offscreen textures.
        * It will work in most cases but there may be problems if the client
        * modifies the texture directly, or expects the contents of the rendertarget
        * to be persistent.
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

        XVisualInfo *visinfo;
        glContext   *newContext;

        /* Here were using a shared context model */
        if (D3D_OK != IWineD3DDeviceImpl_FindGLContext(iface, RenderSurface, &newContext)) {
            FIXME("(%p) : Failed to find a context for surface %p\n", iface, RenderSurface);
        }

        /* If the context doesn't exist then create a new one */
        /* TODO: This should really be part of findGlContext */
        if (NULL == newContext->context) {

            TRACE("making new buffer\n");
            nAttribs = 0;
            PUSH2(GLX_PBUFFER_WIDTH,  newContext->Width);
            PUSH2(GLX_PBUFFER_HEIGHT, newContext->Height);
            PUSH1(None);

            newContext->drawable  = glXCreatePbuffer(impSwapChain->display, cfgs[0], attribs);

            /** ****************************************
            *GLX1.3 isn't supported by XFree 'yet' until that point ATI emulates pBuffers
            *they note:
            *   In future releases, we may provide the calls glXCreateNewContext,
            *   glXQueryDrawable and glXMakeContextCurrent.
            *    so until then we have to use glXGetVisualFromFBConfig &co..
            ********************************************/


            visinfo = glXGetVisualFromFBConfig(impSwapChain->display, cfgs[0]);
            if (!visinfo) {
                ERR("Error: couldn't get an RGBA, double-buffered visual\n");
            } else {
                newContext->context = glXCreateContext(impSwapChain->display, visinfo, impSwapChain->glCtx,  GL_TRUE);
                XFree(visinfo);
            }
        }
        if (NULL == newContext || NULL == newContext->context) {
            ERR("(%p) : Failed to find a context for surface %p\n", iface, RenderSurface);
        } else {
            /* Debug logging, (this function leaks), change to a TRACE when the leak is plugged */
            if (glXMakeCurrent(impSwapChain->display, newContext->drawable, newContext->context) == False) {
                TRACE("Error in setting current context: context %p drawable %ld\n", newContext->context, newContext->drawable);
            }

            /* Clean up the old context */
            IWineD3DDeviceImpl_CleanRender(iface, (IWineD3DSwapChainImpl *)currentSwapchain);
            /* Set the current context of the swapchain to the new context */
            impSwapChain->drawable   = newContext->drawable;
            impSwapChain->render_ctx = newContext->context;
        }
    }

#if 1 /* Apply the stateblock to the new context
FIXME: This is a bit of a hack, each context should know it's own state,
the directX current directX state should then be applied to the context */
    {
        BOOL oldRecording;
        IWineD3DStateBlockImpl *oldUpdateStateBlock;
        oldUpdateStateBlock = This->updateStateBlock;
        oldRecording= This->isRecordingState;
        This->isRecordingState = FALSE;
        This->updateStateBlock = This->stateBlock;
        IWineD3DStateBlock_Apply((IWineD3DStateBlock *)This->stateBlock);

        This->isRecordingState = oldRecording;
        This->updateStateBlock = oldUpdateStateBlock;
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
    TRACE("(%p) : state (%lu)\n", This, This->state);
    /* TODO: Implement wrapping of the WndProc so that mimimize and maxamise can be monitored and the states adjusted. */
    switch (This->state) {
    case D3D_OK:
        return D3D_OK;
    case D3DERR_DEVICELOST:
        {
            ResourceList *resourceList  = This->resources;
            while (NULL != resourceList) {
                if (((IWineD3DResourceImpl *)resourceList->resource)->resource.pool == D3DPOOL_DEFAULT /* TODO: IWineD3DResource_GetPool(resourceList->resource)*/)
                return D3DERR_DEVICENOTRESET;
                resourceList = resourceList->next;
            }
            return D3DERR_DEVICELOST;
        }
    case D3DERR_DRIVERINTERNALERROR:
        return D3DERR_DRIVERINTERNALERROR;
    }

    /* Unknown state */
    return D3DERR_DRIVERINTERNALERROR;
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
    TRACE("(%p) : pParameters %p\n", This, pParameters);

    *pParameters = This->createParms;
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


/** ********************************************************
*   Notification functions
** ********************************************************/
/** This function must be called in the release of a resource when ref == 0,
* the contents of resource must still be correct,
* any handels to other resource held by the caller must be closed
* (e.g. a texture should release all held surfaces because telling the device that it's been released.)
 *****************************************************/
static void WINAPI IWineD3DDeviceImpl_AddResource(IWineD3DDevice *iface, IWineD3DResource *resource){
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    ResourceList* resourceList;

    TRACE("(%p) : resource %p\n", This, resource);
#if 0
    EnterCriticalSection(&resourceStoreCriticalSection);
#endif
    /* add a new texture to the frot of the linked list */
    resourceList = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ResourceList));
    resourceList->resource = resource;

    /* Get the old head */
    resourceList->next = This->resources;

    This->resources = resourceList;
    TRACE("Added resource %p with element %p pointing to %p\n", resource, resourceList, resourceList->next);

#if 0
    LeaveCriticalSection(&resourceStoreCriticalSection);
#endif
    return;
}

static void WINAPI IWineD3DDeviceImpl_RemoveResource(IWineD3DDevice *iface, IWineD3DResource *resource){
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    ResourceList* resourceList = NULL;
    ResourceList* previousResourceList = NULL;
    
    TRACE("(%p) : resource %p\n", This, resource);

#if 0
    EnterCriticalSection(&resourceStoreCriticalSection);
#endif
    resourceList = This->resources;

    while (resourceList != NULL) {
        if(resourceList->resource == resource) break;
        previousResourceList = resourceList;
        resourceList = resourceList->next;
    }

    if (resourceList == NULL) {
        FIXME("Attempted to remove resource %p that hasn't been stored\n", resource);
#if 0
        LeaveCriticalSection(&resourceStoreCriticalSection);
#endif
        return;
    } else {
            TRACE("Found resource  %p with element %p pointing to %p (previous %p)\n", resourceList->resource, resourceList, resourceList->next, previousResourceList);
    }
    /* make sure we don't leave a hole in the list */
    if (previousResourceList != NULL) {
        previousResourceList->next = resourceList->next;
    } else {
        This->resources = resourceList->next;
    }

#if 0
    LeaveCriticalSection(&resourceStoreCriticalSection);
#endif
    return;
}


void WINAPI IWineD3DDeviceImpl_ResourceReleased(IWineD3DDevice *iface, IWineD3DResource *resource){
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    int counter;

    TRACE("(%p) : resource %p\n", This, resource);
    switch(IWineD3DResource_GetType(resource)){
        case WINED3DRTYPE_SURFACE:
        /* TODO: check front and back buffers, rendertargets etc..  possibly swapchains? */
        break;
        case WINED3DRTYPE_TEXTURE:
        case WINED3DRTYPE_CUBETEXTURE:
        case WINED3DRTYPE_VOLUMETEXTURE:
                for (counter = 0; counter < GL_LIMITS(textures); counter++) {
                    if (This->stateBlock != NULL && This->stateBlock->textures[counter] == (IWineD3DBaseTexture *)resource) {
                        WARN("Texture being released is still by a stateblock, Stage = %u Texture = %p\n", counter, resource);
                        This->stateBlock->textures[counter] = NULL;
                    }
                    if (This->updateStateBlock != This->stateBlock ){
                        if (This->updateStateBlock->textures[counter] == (IWineD3DBaseTexture *)resource) {
                            WARN("Texture being released is still by a stateblock, Stage = %u Texture = %p\n", counter, resource);
                            This->updateStateBlock->textures[counter] = NULL;
                        }
                    }
                }
        break;
        case WINED3DRTYPE_VOLUME:
        /* TODO: nothing really? */
        break;
        case WINED3DRTYPE_VERTEXBUFFER:
        /* MSDN: When an application no longer holds a references to this interface, the interface will automatically be freed. */
        {
            int streamNumber;
            TRACE("Cleaning up stream pointers\n");

            for(streamNumber = 0; streamNumber < MAX_STREAMS; streamNumber ++){
                /* FINDOUT: should a warn be generated if were recording and updateStateBlock->streamSource is lost?
                FINDOUT: should changes.streamSource[StreamNumber] be set ?
                */
                if (This->updateStateBlock != NULL ) { /* ==NULL when device is being destroyed */
                    if ((IWineD3DResource *)This->updateStateBlock->streamSource[streamNumber] == resource) {
                        FIXME("Vertex buffer released whlst bound to a state block  stream %d\n", streamNumber);
                        This->updateStateBlock->streamSource[streamNumber] = 0;
                        /* Set changed flag? */
                    }
                }
                if (This->stateBlock != NULL ) { /* only happens if there is an error in the application, or on reset/release (because we don't manage internal tracking properly) */
                    if ((IWineD3DResource *)This->stateBlock->streamSource[streamNumber] == resource) {
                        TRACE("Vertex buffer released whlst bound to a state block  stream %d\n", streamNumber);
                        This->stateBlock->streamSource[streamNumber] = 0;
                    }
                }
#if 0   /* TODO: Manage internal tracking properly so that 'this shouldn't happen' */
                 else { /* This shouldn't happen */
                    FIXME("Calling application has released the device before relasing all the resources bound to the device\n");
                }
#endif

            }
        }
        break;
        case WINED3DRTYPE_INDEXBUFFER:
        /* MSDN: When an application no longer holds a references to this interface, the interface will automatically be freed.*/
        if (This->updateStateBlock != NULL ) { /* ==NULL when device is being destroyed */
            if (This->updateStateBlock->pIndexData == (IWineD3DIndexBuffer *)resource) {
                This->updateStateBlock->pIndexData =  NULL;
            }
        }
        if (This->stateBlock != NULL ) { /* ==NULL when device is being destroyed */
            if (This->stateBlock->pIndexData == (IWineD3DIndexBuffer *)resource) {
                This->stateBlock->pIndexData =  NULL;
            }
        }

        break;
        default:
        FIXME("(%p) unknown resource type %p %u\n", This, resource, IWineD3DResource_GetType(resource));
        break;
    }


    /* Remove the resoruce from the resourceStore */
    IWineD3DDeviceImpl_RemoveResource(iface, resource);

    TRACE("Resource released\n");

}


/** This function is to be called by the swapchain when it is released and it's ref = 0
 *****************************************************/
void WINAPI IWineD3DDeviceImpl_SwapChainReleased(IWineD3DDevice *iface, IWineD3DSwapChain *swapChain){
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    SwapChainList **nextSwapchain;
    nextSwapchain = &This->swapchains;

    /* Check to see if the swapchian is being used as the render target */
    if (This->renderTarget != NULL) {
        IWineD3DSurface *swapchainBackBuffer;

        IWineD3DSwapChain_GetBackBuffer(swapChain, 0 ,( D3DBACKBUFFER_TYPE) 0, &swapchainBackBuffer);
        if (This->renderTarget == swapchainBackBuffer) {
            /* Don't know what to do, so warn and carry on as usual (which in this case leaves the renderterget in limbo) */
            FIXME("Atempting to release a swapchain that is currently beuing used as a render target, behaviour is undefined\n");
        }
    }

    /* Go through the swapchain list and try to find the swapchain being released */
    while(*nextSwapchain != NULL && (*nextSwapchain)->swapchain != swapChain) {
        nextSwapchain = &(*nextSwapchain)->next;
    }

    /* Check to see if we found the swapchain */
    if (NULL != *nextSwapchain) {
        /* We found the swapchain so remove it from the list */
        TRACE("(%p) releasing swapchain(%p)\n", iface, swapChain);
        HeapFree(GetProcessHeap(), 0 , *nextSwapchain);
        *nextSwapchain = (*nextSwapchain)->next;
    } else {
        /* We didn't find the swapchain on the list, this can only heppen because of a programming error in wined3d */
        FIXME("(%p) Attempting to release a swapchain (%p) that hasn't been stored\n", iface, swapChain);
    }

    TRACE("swapchain (%p) released\n", swapChain);
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
    IWineD3DDeviceImpl_SetPixelShaderConstant,
    IWineD3DDeviceImpl_GetPixelShaderConstant,
    IWineD3DDeviceImpl_SetPixelShaderConstantB,
    IWineD3DDeviceImpl_GetPixelShaderConstantB,
    IWineD3DDeviceImpl_SetPixelShaderConstantI,
    IWineD3DDeviceImpl_GetPixelShaderConstantI,
    IWineD3DDeviceImpl_SetPixelShaderConstantF,
    IWineD3DDeviceImpl_GetPixelShaderConstantF,
    IWineD3DDeviceImpl_SetPixelShaderConstantN,
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
    IWineD3DDeviceImpl_SetVertexShaderConstant,
    IWineD3DDeviceImpl_GetVertexShaderConstant,
    IWineD3DDeviceImpl_SetVertexShaderConstantB,
    IWineD3DDeviceImpl_GetVertexShaderConstantB,
    IWineD3DDeviceImpl_SetVertexShaderConstantI,
    IWineD3DDeviceImpl_GetVertexShaderConstantI,
    IWineD3DDeviceImpl_SetVertexShaderConstantF,
    IWineD3DDeviceImpl_GetVertexShaderConstantF,
    IWineD3DDeviceImpl_SetVertexShaderConstantN,
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
    IWineD3DDeviceImpl_CopyRects,
    IWineD3DDeviceImpl_StretchRect,
    IWineD3DDeviceImpl_GetRenderTargetData,
    IWineD3DDeviceImpl_GetFrontBufferData,
    /*** Internal use IWineD3DDevice methods ***/
    IWineD3DDeviceImpl_SetupTextureStates,
    /*** object tracking ***/
    IWineD3DDeviceImpl_SwapChainReleased,
    IWineD3DDeviceImpl_ResourceReleased
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
