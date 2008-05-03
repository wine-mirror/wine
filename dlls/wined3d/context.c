/*
 * Context and render target management in wined3d
 *
 * Copyright 2007-2008 Stefan Dösinger for CodeWeavers
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
#include <stdio.h>
#ifdef HAVE_FLOAT_H
# include <float.h>
#endif
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

#define GLINFO_LOCATION This->adapter->gl_info

/*****************************************************************************
 * Context_MarkStateDirty
 *
 * Marks a state in a context dirty. Only one context, opposed to
 * IWineD3DDeviceImpl_MarkStateDirty, which marks the state dirty in all
 * contexts
 *
 * Params:
 *  context: Context to mark the state dirty in
 *  state: State to mark dirty
 *  StateTable: Pointer to the state table in use(for state grouping)
 *
 *****************************************************************************/
static void Context_MarkStateDirty(WineD3DContext *context, DWORD state, const struct StateEntry *StateTable) {
    DWORD rep = StateTable[state].representative;
    DWORD idx;
    BYTE shift;

    if(!rep || isStateDirty(context, rep)) return;

    context->dirtyArray[context->numDirtyEntries++] = rep;
    idx = rep >> 5;
    shift = rep & 0x1f;
    context->isStateDirty[idx] |= (1 << shift);
}

/*****************************************************************************
 * AddContextToArray
 *
 * Adds a context to the context array. Helper function for CreateContext
 *
 * This method is not called in performance-critical code paths, only when a
 * new render target or swapchain is created. Thus performance is not an issue
 * here.
 *
 * Params:
 *  This: Device to add the context for
 *  hdc: device context
 *  glCtx: WGL context to add
 *  pbuffer: optional pbuffer used with this context
 *
 *****************************************************************************/
static WineD3DContext *AddContextToArray(IWineD3DDeviceImpl *This, HWND win_handle, HDC hdc, HGLRC glCtx, HPBUFFERARB pbuffer) {
    WineD3DContext **oldArray = This->contexts;
    DWORD state;

    This->contexts = HeapAlloc(GetProcessHeap(), 0, sizeof(*This->contexts) * (This->numContexts + 1));
    if(This->contexts == NULL) {
        ERR("Unable to grow the context array\n");
        This->contexts = oldArray;
        return NULL;
    }
    if(oldArray) {
        memcpy(This->contexts, oldArray, sizeof(*This->contexts) * This->numContexts);
    }

    This->contexts[This->numContexts] = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WineD3DContext));
    if(This->contexts[This->numContexts] == NULL) {
        ERR("Unable to allocate a new context\n");
        HeapFree(GetProcessHeap(), 0, This->contexts);
        This->contexts = oldArray;
        return NULL;
    }

    This->contexts[This->numContexts]->hdc = hdc;
    This->contexts[This->numContexts]->glCtx = glCtx;
    This->contexts[This->numContexts]->pbuffer = pbuffer;
    This->contexts[This->numContexts]->win_handle = win_handle;
    HeapFree(GetProcessHeap(), 0, oldArray);

    /* Mark all states dirty to force a proper initialization of the states on the first use of the context
     */
    for(state = 0; state <= STATE_HIGHEST; state++) {
        Context_MarkStateDirty(This->contexts[This->numContexts], state, This->shader_backend->StateTable);
    }

    This->numContexts++;
    TRACE("Created context %p\n", This->contexts[This->numContexts - 1]);
    return This->contexts[This->numContexts - 1];
}

/* This function takes care of WineD3D pixel format selection. */
static int WineD3D_ChoosePixelFormat(IWineD3DDeviceImpl *This, HDC hdc, WINED3DFORMAT ColorFormat, WINED3DFORMAT DepthStencilFormat, BOOL auxBuffers, int numSamples, BOOL pbuffer, BOOL findCompatible)
{
    int iPixelFormat=0;
    short redBits, greenBits, blueBits, alphaBits, colorBits;
    short depthBits=0, stencilBits=0;

    int i = 0;
    int nCfgs = This->adapter->nCfgs;
    WineD3D_PixelFormat *cfgs = This->adapter->cfgs;

    if(!getColorBits(ColorFormat, &redBits, &greenBits, &blueBits, &alphaBits, &colorBits)) {
        ERR("Unable to get color bits for format %s (%#x)!\n", debug_d3dformat(ColorFormat), ColorFormat);
        return 0;
    }

    if(DepthStencilFormat) {
        getDepthStencilBits(DepthStencilFormat, &depthBits, &stencilBits);
    }

    /* Find a pixel format which EXACTLY matches our requirements (except for depth) */
    for(i=0; i<nCfgs; i++) {
        BOOL exactDepthMatch = TRUE;
        cfgs = &This->adapter->cfgs[i];

        /* For now only accept RGBA formats. Perhaps some day we will
         * allow floating point formats for pbuffers. */
        if(cfgs->iPixelType != WGL_TYPE_RGBA_ARB)
            continue;

        /* In window mode (!pbuffer) we need a window drawable format and double buffering. */
        if(!pbuffer && !(cfgs->windowDrawable && cfgs->doubleBuffer))
            continue;

        /* We like to have aux buffers in backbuffer mode */
        if(auxBuffers && !cfgs->auxBuffers)
            continue;

        /* In pbuffer-mode we need a pbuffer-capable format but we don't want double buffering */
        if(pbuffer && (!cfgs->pbufferDrawable || cfgs->doubleBuffer))
            continue;

        if(cfgs->redSize != redBits)
            continue;
        if(cfgs->greenSize != greenBits)
            continue;
        if(cfgs->blueSize != blueBits)
            continue;
        if(cfgs->alphaSize != alphaBits)
            continue;

        /* We try to locate a format which matches our requirements exactly. In case of
         * depth it is no problem to emulate 16-bit using e.g. 24-bit, so accept that. */
        if(cfgs->depthSize < depthBits)
            continue;
        else if(cfgs->depthSize > depthBits)
            exactDepthMatch = FALSE;

        /* In all cases make sure the number of stencil bits matches our requirements
         * even when we don't need stencil because it could affect performance EXCEPT
         * on cards which don't offer depth formats without stencil like the i915 drivers
         * on Linux. */
        if(stencilBits != cfgs->stencilSize && !(This->adapter->brokenStencil && stencilBits <= cfgs->stencilSize))
            continue;

        /* Check multisampling support */
        if(cfgs->numSamples != numSamples)
            continue;

        /* When we have passed all the checks then we have found a format which matches our
         * requirements. Note that we only check for a limit number of capabilities right now,
         * so there can easily be a dozen of pixel formats which appear to be the 'same' but
         * can still differ in things like multisampling, stereo, SRGB and other flags.
         */

        /* Exit the loop as we have found a format :) */
        if(exactDepthMatch) {
            iPixelFormat = cfgs->iPixelFormat;
            break;
        } else if(!iPixelFormat) {
            /* In the end we might end up with a format which doesn't exactly match our depth
             * requirements. Accept the first format we found because formats with higher iPixelFormat
             * values tend to have more extended capabilities (e.g. multisampling) which we don't need. */
            iPixelFormat = cfgs->iPixelFormat;
        }
    }

    /* When findCompatible is set and no suitable format was found, let ChoosePixelFormat choose a pixel format in order not to crash. */
    if(!iPixelFormat && !findCompatible) {
        ERR("Can't find a suitable iPixelFormat\n");
        return FALSE;
    } else if(!iPixelFormat) {
        PIXELFORMATDESCRIPTOR pfd;

        TRACE("Falling back to ChoosePixelFormat as we weren't able to find an exactly matching pixel format\n");
        /* PixelFormat selection */
        ZeroMemory(&pfd, sizeof(pfd));
        pfd.nSize      = sizeof(pfd);
        pfd.nVersion   = 1;
        pfd.dwFlags    = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW;/*PFD_GENERIC_ACCELERATED*/
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cAlphaBits = alphaBits;
        pfd.cColorBits = colorBits;
        pfd.cDepthBits = depthBits;
        pfd.cStencilBits = stencilBits;
        pfd.iLayerType = PFD_MAIN_PLANE;

        iPixelFormat = ChoosePixelFormat(hdc, &pfd);
        if(!iPixelFormat) {
            /* If this happens something is very wrong as ChoosePixelFormat barely fails */
            ERR("Can't find a suitable iPixelFormat\n");
            return FALSE;
        }
    }

    TRACE("Found iPixelFormat=%d for ColorFormat=%s, DepthStencilFormat=%s\n", iPixelFormat, debug_d3dformat(ColorFormat), debug_d3dformat(DepthStencilFormat));
    return iPixelFormat;
}

/*****************************************************************************
 * CreateContext
 *
 * Creates a new context for a window, or a pbuffer context.
 *
 * * Params:
 *  This: Device to activate the context for
 *  target: Surface this context will render to
 *  win_handle: handle to the window which we are drawing to
 *  create_pbuffer: tells whether to create a pbuffer or not
 *  pPresentParameters: contains the pixelformats to use for onscreen rendering
 *
 *****************************************************************************/
WineD3DContext *CreateContext(IWineD3DDeviceImpl *This, IWineD3DSurfaceImpl *target, HWND win_handle, BOOL create_pbuffer, const WINED3DPRESENT_PARAMETERS *pPresentParms) {
    HDC oldDrawable, hdc;
    HPBUFFERARB pbuffer = NULL;
    HGLRC ctx = NULL, oldCtx;
    WineD3DContext *ret = NULL;
    int s;

    TRACE("(%p): Creating a %s context for render target %p\n", This, create_pbuffer ? "offscreen" : "onscreen", target);

    if(create_pbuffer) {
        HDC hdc_parent = GetDC(win_handle);
        int iPixelFormat = 0;

        IWineD3DSurface *StencilSurface = This->stencilBufferTarget;
        WINED3DFORMAT StencilBufferFormat = (NULL != StencilSurface) ? ((IWineD3DSurfaceImpl *) StencilSurface)->resource.format : 0;

        /* Try to find a pixel format with pbuffer support. */
        iPixelFormat = WineD3D_ChoosePixelFormat(This, hdc_parent, target->resource.format, StencilBufferFormat, FALSE /* auxBuffers */, 0 /* numSamples */, TRUE /* PBUFFER */, FALSE /* findCompatible */);
        if(!iPixelFormat) {
            TRACE("Trying to locate a compatible pixel format because an exact match failed.\n");

            /* For some reason we weren't able to find a format, try to find something instead of crashing.
             * A reason for failure could have been wglChoosePixelFormatARB strictness. */
            iPixelFormat = WineD3D_ChoosePixelFormat(This, hdc_parent, target->resource.format, StencilBufferFormat, FALSE /* auxBuffer */, 0 /* numSamples */, TRUE /* PBUFFER */, TRUE /* findCompatible */);
        }

        /* This shouldn't happen as ChoosePixelFormat always returns something */
        if(!iPixelFormat) {
            ERR("Unable to locate a pixel format for a pbuffer\n");
            ReleaseDC(win_handle, hdc_parent);
            goto out;
        }

        TRACE("Creating a pBuffer drawable for the new context\n");
        pbuffer = GL_EXTCALL(wglCreatePbufferARB(hdc_parent, iPixelFormat, target->currentDesc.Width, target->currentDesc.Height, 0));
        if(!pbuffer) {
            ERR("Cannot create a pbuffer\n");
            ReleaseDC(win_handle, hdc_parent);
            goto out;
        }

        /* In WGL a pbuffer is 'wrapped' inside a HDC to 'fool' wglMakeCurrent */
        hdc = GL_EXTCALL(wglGetPbufferDCARB(pbuffer));
        if(!hdc) {
            ERR("Cannot get a HDC for pbuffer (%p)\n", pbuffer);
            GL_EXTCALL(wglDestroyPbufferARB(pbuffer));
            ReleaseDC(win_handle, hdc_parent);
            goto out;
        }
        ReleaseDC(win_handle, hdc_parent);
    } else {
        PIXELFORMATDESCRIPTOR pfd;
        int iPixelFormat;
        int res;
        WINED3DFORMAT ColorFormat = target->resource.format;
        WINED3DFORMAT DepthStencilFormat = 0;
        BOOL auxBuffers = FALSE;
        int numSamples = 0;

        hdc = GetDC(win_handle);
        if(hdc == NULL) {
            ERR("Cannot retrieve a device context!\n");
            goto out;
        }

        /* In case of ORM_BACKBUFFER, make sure to request an alpha component for X4R4G4B4/X8R8G8B8 as we might need it for the backbuffer. */
        if(wined3d_settings.offscreen_rendering_mode == ORM_BACKBUFFER) {
            auxBuffers = TRUE;

            if(target->resource.format == WINED3DFMT_X4R4G4B4)
                ColorFormat = WINED3DFMT_A4R4G4B4;
            else if(target->resource.format == WINED3DFMT_X8R8G8B8)
                ColorFormat = WINED3DFMT_A8R8G8B8;
        }

        /* DirectDraw supports 8bit paletted render targets and these are used by old games like Starcraft and C&C.
         * Most modern hardware doesn't support 8bit natively so we perform some form of 8bit -> 32bit conversion.
         * The conversion (ab)uses the alpha component for storing the palette index. For this reason we require
         * a format with 8bit alpha, so request A8R8G8B8. */
        if(ColorFormat == WINED3DFMT_P8)
            ColorFormat = WINED3DFMT_A8R8G8B8;

        /* Retrieve the depth stencil format from the present parameters.
         * The choice of the proper format can give a nice performance boost
         * in case of GPU limited programs. */
        if(pPresentParms->EnableAutoDepthStencil) {
            TRACE("pPresentParms->EnableAutoDepthStencil=enabled; using AutoDepthStencilFormat=%s\n", debug_d3dformat(pPresentParms->AutoDepthStencilFormat));
            DepthStencilFormat = pPresentParms->AutoDepthStencilFormat;
        }

        /* D3D only allows multisampling when SwapEffect is set to WINED3DSWAPEFFECT_DISCARD */
        if(pPresentParms->MultiSampleType && (pPresentParms->SwapEffect == WINED3DSWAPEFFECT_DISCARD)) {
            if(!GL_SUPPORT(ARB_MULTISAMPLE))
                ERR("The program is requesting multisampling without support!\n");
            else {
                ERR("Requesting MultiSampleType=%d\n", pPresentParms->MultiSampleType);
                numSamples = pPresentParms->MultiSampleType;
            }
        }

        /* Try to find a pixel format which matches our requirements */
        iPixelFormat = WineD3D_ChoosePixelFormat(This, hdc, ColorFormat, DepthStencilFormat, auxBuffers, numSamples, FALSE /* PBUFFER */, FALSE /* findCompatible */);

        /* Try to locate a compatible format if we weren't able to find anything */
        if(!iPixelFormat) {
            TRACE("Trying to locate a compatible pixel format because an exact match failed.\n");
            iPixelFormat = WineD3D_ChoosePixelFormat(This, hdc, ColorFormat, DepthStencilFormat, auxBuffers, 0 /* numSamples */, FALSE /* PBUFFER */, TRUE /* findCompatible */ );
        }

        /* If we still don't have a pixel format, something is very wrong as ChoosePixelFormat barely fails */
        if(!iPixelFormat) {
            ERR("Can't find a suitable iPixelFormat\n");
            return FALSE;
        }

        DescribePixelFormat(hdc, iPixelFormat, sizeof(pfd), &pfd);
        res = SetPixelFormat(hdc, iPixelFormat, NULL);
        if(!res) {
            int oldPixelFormat = GetPixelFormat(hdc);

            /* By default WGL doesn't allow pixel format adjustments but we need it here.
             * For this reason there is a WINE-specific wglSetPixelFormat which allows you to
             * set the pixel format multiple times. Only use it when it is really needed. */

            if(oldPixelFormat == iPixelFormat) {
                /* We don't have to do anything as the formats are the same :) */
            } else if(oldPixelFormat && GL_SUPPORT(WGL_WINE_PIXEL_FORMAT_PASSTHROUGH)) {
                res = GL_EXTCALL(wglSetPixelFormatWINE(hdc, iPixelFormat, NULL));

                if(!res) {
                    ERR("wglSetPixelFormatWINE failed on HDC=%p for iPixelFormat=%d\n", hdc, iPixelFormat);
                    return FALSE;
                }
            } else if(oldPixelFormat) {
                /* OpenGL doesn't allow pixel format adjustments. Print an error and continue using the old format.
                 * There's a big chance that the old format works although with a performance hit and perhaps rendering errors. */
                ERR("HDC=%p is already set to iPixelFormat=%d and OpenGL doesn't allow changes!\n", hdc, oldPixelFormat);
            } else {
                ERR("SetPixelFormat failed on HDC=%p for iPixelFormat=%d\n", hdc, iPixelFormat);
                return FALSE;
            }
        }
    }

    ctx = pwglCreateContext(hdc);
    if(This->numContexts) pwglShareLists(This->contexts[0]->glCtx, ctx);

    if(!ctx) {
        ERR("Failed to create a WGL context\n");
        if(create_pbuffer) {
            GL_EXTCALL(wglReleasePbufferDCARB(pbuffer, hdc));
            GL_EXTCALL(wglDestroyPbufferARB(pbuffer));
        }
        goto out;
    }
    ret = AddContextToArray(This, win_handle, hdc, ctx, pbuffer);
    if(!ret) {
        ERR("Failed to add the newly created context to the context list\n");
        pwglDeleteContext(ctx);
        if(create_pbuffer) {
            GL_EXTCALL(wglReleasePbufferDCARB(pbuffer, hdc));
            GL_EXTCALL(wglDestroyPbufferARB(pbuffer));
        }
        goto out;
    }
    ret->surface = (IWineD3DSurface *) target;
    ret->isPBuffer = create_pbuffer;
    ret->tid = GetCurrentThreadId();
    if(This->shader_backend->shader_dirtifyable_constants((IWineD3DDevice *) This)) {
        /* Create the dirty constants array and initialize them to dirty */
        ret->vshader_const_dirty = HeapAlloc(GetProcessHeap(), 0,
                sizeof(*ret->vshader_const_dirty) * GL_LIMITS(vshader_constantsF));
        ret->pshader_const_dirty = HeapAlloc(GetProcessHeap(), 0,
                sizeof(*ret->pshader_const_dirty) * GL_LIMITS(pshader_constantsF));
        memset(ret->vshader_const_dirty, 1,
               sizeof(*ret->vshader_const_dirty) * GL_LIMITS(vshader_constantsF));
        memset(ret->pshader_const_dirty, 1,
                sizeof(*ret->pshader_const_dirty) * GL_LIMITS(pshader_constantsF));
    }

    TRACE("Successfully created new context %p\n", ret);

    /* Set up the context defaults */
    oldCtx  = pwglGetCurrentContext();
    oldDrawable = pwglGetCurrentDC();
    if(oldCtx && oldDrawable) {
        /* See comment in ActivateContext context switching */
        This->shader_backend->shader_fragment_enable((IWineD3DDevice *) This, FALSE);
    }
    if(pwglMakeCurrent(hdc, ctx) == FALSE) {
        ERR("Cannot activate context to set up defaults\n");
        goto out;
    }

    ENTER_GL();

    glGetIntegerv(GL_AUX_BUFFERS, &ret->aux_buffers);

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

    glPixelStorei(GL_PACK_ALIGNMENT, This->surface_alignment);
    checkGLcall("glPixelStorei(GL_PACK_ALIGNMENT, This->surface_alignment);");
    glPixelStorei(GL_UNPACK_ALIGNMENT, This->surface_alignment);
    checkGLcall("glPixelStorei(GL_UNPACK_ALIGNMENT, This->surface_alignment);");

    if(GL_SUPPORT(APPLE_CLIENT_STORAGE)) {
        /* Most textures will use client storage if supported. Exceptions are non-native power of 2 textures
         * and textures in DIB sections(due to the memory protection).
         */
        glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
        checkGLcall("glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE)");
    }
    if(GL_SUPPORT(ARB_VERTEX_BLEND)) {
        /* Direct3D always uses n-1 weights for n world matrices and uses 1 - sum for the last one
         * this is equal to GL_WEIGHT_SUM_UNITY_ARB. Enabling it doesn't do anything unless
         * GL_VERTEX_BLEND_ARB isn't enabled too
         */
        glEnable(GL_WEIGHT_SUM_UNITY_ARB);
        checkGLcall("glEnable(GL_WEIGHT_SUM_UNITY_ARB)");
    }
    if(GL_SUPPORT(NV_TEXTURE_SHADER2)) {
        glEnable(GL_TEXTURE_SHADER_NV);
        checkGLcall("glEnable(GL_TEXTURE_SHADER_NV)");

        /* Set up the previous texture input for all shader units. This applies to bump mapping, and in d3d
         * the previous texture where to source the offset from is always unit - 1.
         */
        for(s = 1; s < GL_LIMITS(textures); s++) {
            GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB + s));
            glTexEnvi(GL_TEXTURE_SHADER_NV, GL_PREVIOUS_TEXTURE_INPUT_NV, GL_TEXTURE0_ARB + s - 1);
            checkGLcall("glTexEnvi(GL_TEXTURE_SHADER_NV, GL_PREVIOUS_TEXTURE_INPUT_NV, ...\n");
        }
    }

    if(GL_SUPPORT(ARB_POINT_SPRITE)) {
        for(s = 0; s < GL_LIMITS(textures); s++) {
            GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB + s));
            glTexEnvi(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);
            checkGLcall("glTexEnvi(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE)\n");
        }
    }
    LEAVE_GL();

    /* Never keep GL_FRAGMENT_SHADER_ATI enabled on a context that we switch away from,
     * but enable it for the first context we create, and reenable it on the old context
     */
    if(oldDrawable && oldCtx) {
        pwglMakeCurrent(oldDrawable, oldCtx);
    }
    This->shader_backend->shader_fragment_enable((IWineD3DDevice *) This, TRUE);

out:
    return ret;
}

/*****************************************************************************
 * RemoveContextFromArray
 *
 * Removes a context from the context manager. The opengl context is not
 * destroyed or unset. context is not a valid pointer after that call.
 *
 * Similar to the former call this isn't a performance critical function. A
 * helper function for DestroyContext.
 *
 * Params:
 *  This: Device to activate the context for
 *  context: Context to remove
 *
 *****************************************************************************/
static void RemoveContextFromArray(IWineD3DDeviceImpl *This, WineD3DContext *context) {
    UINT t, s;
    WineD3DContext **oldArray = This->contexts;

    TRACE("Removing ctx %p\n", context);

    This->numContexts--;

    if(This->numContexts) {
        This->contexts = HeapAlloc(GetProcessHeap(), 0, sizeof(*This->contexts) * This->numContexts);
        if(!This->contexts) {
            ERR("Cannot allocate a new context array, PANIC!!!\n");
        }
        t = 0;
        /* Note that we decreased numContexts a few lines up, so use '<=' instead of '<' */
        for(s = 0; s <= This->numContexts; s++) {
            if(oldArray[s] == context) continue;
            This->contexts[t] = oldArray[s];
            t++;
        }
    } else {
        This->contexts = NULL;
    }

    HeapFree(GetProcessHeap(), 0, context);
    HeapFree(GetProcessHeap(), 0, oldArray);
}

/*****************************************************************************
 * DestroyContext
 *
 * Destroys a wineD3DContext
 *
 * Params:
 *  This: Device to activate the context for
 *  context: Context to destroy
 *
 *****************************************************************************/
void DestroyContext(IWineD3DDeviceImpl *This, WineD3DContext *context) {

    /* check that we are the current context first */
    TRACE("Destroying ctx %p\n", context);
    if(pwglGetCurrentContext() == context->glCtx){
        pwglMakeCurrent(NULL, NULL);
    }

    if(context->isPBuffer) {
        GL_EXTCALL(wglReleasePbufferDCARB(context->pbuffer, context->hdc));
        GL_EXTCALL(wglDestroyPbufferARB(context->pbuffer));
    } else ReleaseDC(context->win_handle, context->hdc);
    pwglDeleteContext(context->glCtx);

    HeapFree(GetProcessHeap(), 0, context->vshader_const_dirty);
    HeapFree(GetProcessHeap(), 0, context->pshader_const_dirty);
    RemoveContextFromArray(This, context);
}

/*****************************************************************************
 * SetupForBlit
 *
 * Sets up a context for DirectDraw blitting.
 * All texture units are disabled, texture unit 0 is set as current unit
 * fog, lighting, blending, alpha test, z test, scissor test, culling disabled
 * color writing enabled for all channels
 * register combiners disabled, shaders disabled
 * world matrix is set to identity, texture matrix 0 too
 * projection matrix is setup for drawing screen coordinates
 *
 * Params:
 *  This: Device to activate the context for
 *  context: Context to setup
 *  width: render target width
 *  height: render target height
 *
 *****************************************************************************/
static inline void SetupForBlit(IWineD3DDeviceImpl *This, WineD3DContext *context, UINT width, UINT height) {
    int i;
    const struct StateEntry *StateTable = This->shader_backend->StateTable;

    TRACE("Setting up context %p for blitting\n", context);
    if(context->last_was_blit) {
        TRACE("Context is already set up for blitting, nothing to do\n");
        return;
    }
    context->last_was_blit = TRUE;

    /* TODO: Use a display list */

    /* Disable shaders */
    This->shader_backend->shader_cleanup((IWineD3DDevice *) This);
    Context_MarkStateDirty(context, STATE_VSHADER, StateTable);
    Context_MarkStateDirty(context, STATE_PIXELSHADER, StateTable);

    /* Disable all textures. The caller can then bind a texture it wants to blit
     * from
     */
    if(GL_SUPPORT(NV_REGISTER_COMBINERS)) {
        glDisable(GL_REGISTER_COMBINERS_NV);
        checkGLcall("glDisable(GL_REGISTER_COMBINERS_NV)");
    }
    if (GL_SUPPORT(ARB_MULTITEXTURE)) {
        /* The blitting code uses (for now) the fixed function pipeline, so make sure to reset all fixed
         * function texture unit. No need to care for higher samplers
         */
        for(i = GL_LIMITS(textures) - 1; i > 0 ; i--) {
            GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB + i));
            checkGLcall("glActiveTextureARB");

            if(GL_SUPPORT(ARB_TEXTURE_CUBE_MAP)) {
                glDisable(GL_TEXTURE_CUBE_MAP_ARB);
                checkGLcall("glDisable GL_TEXTURE_CUBE_MAP_ARB");
            }
            glDisable(GL_TEXTURE_3D);
            checkGLcall("glDisable GL_TEXTURE_3D");
            glDisable(GL_TEXTURE_2D);
            checkGLcall("glDisable GL_TEXTURE_2D");

            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
            checkGLcall("glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);");

            Context_MarkStateDirty(context, STATE_TEXTURESTAGE(i, WINED3DTSS_COLOROP), StateTable);
            Context_MarkStateDirty(context, STATE_SAMPLER(i), StateTable);
        }
        GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB));
        checkGLcall("glActiveTextureARB");
    }
    if(GL_SUPPORT(ARB_TEXTURE_CUBE_MAP)) {
        glDisable(GL_TEXTURE_CUBE_MAP_ARB);
        checkGLcall("glDisable GL_TEXTURE_CUBE_MAP_ARB");
    }
    glDisable(GL_TEXTURE_3D);
    checkGLcall("glDisable GL_TEXTURE_3D");
    glDisable(GL_TEXTURE_2D);
    checkGLcall("glDisable GL_TEXTURE_2D");

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    glMatrixMode(GL_TEXTURE);
    checkGLcall("glMatrixMode(GL_TEXTURE)");
    glLoadIdentity();
    checkGLcall("glLoadIdentity()");
    Context_MarkStateDirty(context, STATE_TRANSFORM(WINED3DTS_TEXTURE0), StateTable);

    if (GL_SUPPORT(EXT_TEXTURE_LOD_BIAS)) {
        glTexEnvf(GL_TEXTURE_FILTER_CONTROL_EXT,
                  GL_TEXTURE_LOD_BIAS_EXT,
                  0.0);
        checkGLcall("glTexEnvi GL_TEXTURE_LOD_BIAS_EXT ...");
    }
    Context_MarkStateDirty(context, STATE_SAMPLER(0), StateTable);
    Context_MarkStateDirty(context, STATE_TEXTURESTAGE(0, WINED3DTSS_COLOROP), StateTable);

    /* Other misc states */
    glDisable(GL_ALPHA_TEST);
    checkGLcall("glDisable(GL_ALPHA_TEST)");
    Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_ALPHATESTENABLE), StateTable);
    glDisable(GL_LIGHTING);
    checkGLcall("glDisable GL_LIGHTING");
    Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_LIGHTING), StateTable);
    glDisable(GL_DEPTH_TEST);
    checkGLcall("glDisable GL_DEPTH_TEST");
    Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_ZENABLE), StateTable);
    glDisable(GL_FOG);
    checkGLcall("glDisable GL_FOG");
    Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_FOGENABLE), StateTable);
    glDisable(GL_BLEND);
    checkGLcall("glDisable GL_BLEND");
    Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_ALPHABLENDENABLE), StateTable);
    glDisable(GL_CULL_FACE);
    checkGLcall("glDisable GL_CULL_FACE");
    Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_CULLMODE), StateTable);
    glDisable(GL_STENCIL_TEST);
    checkGLcall("glDisable GL_STENCIL_TEST");
    Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_STENCILENABLE), StateTable);
    glDisable(GL_SCISSOR_TEST);
    checkGLcall("glDisable GL_SCISSOR_TEST");
    Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_SCISSORTESTENABLE), StateTable);
    if(GL_SUPPORT(ARB_POINT_SPRITE)) {
        glDisable(GL_POINT_SPRITE_ARB);
        checkGLcall("glDisable GL_POINT_SPRITE_ARB");
        Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_POINTSPRITEENABLE), StateTable);
    }
    glColorMask(GL_TRUE, GL_TRUE,GL_TRUE,GL_TRUE);
    checkGLcall("glColorMask");
    Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_CLIPPING), StateTable);
    if (GL_SUPPORT(EXT_SECONDARY_COLOR)) {
        glDisable(GL_COLOR_SUM_EXT);
        Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_SPECULARENABLE), StateTable);
        checkGLcall("glDisable(GL_COLOR_SUM_EXT)");
    }
    if (GL_SUPPORT(NV_REGISTER_COMBINERS)) {
        GL_EXTCALL(glFinalCombinerInputNV(GL_VARIABLE_B_NV, GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB));
        Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_SPECULARENABLE), StateTable);
        checkGLcall("glFinalCombinerInputNV");
    }

    /* Setup transforms */
    glMatrixMode(GL_MODELVIEW);
    checkGLcall("glMatrixMode(GL_MODELVIEW)");
    glLoadIdentity();
    checkGLcall("glLoadIdentity()");
    Context_MarkStateDirty(context, STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(0)), StateTable);

    glMatrixMode(GL_PROJECTION);
    checkGLcall("glMatrixMode(GL_PROJECTION)");
    glLoadIdentity();
    checkGLcall("glLoadIdentity()");
    glOrtho(0, width, height, 0, 0.0, -1.0);
    checkGLcall("glOrtho");
    Context_MarkStateDirty(context, STATE_TRANSFORM(WINED3DTS_PROJECTION), StateTable);

    context->last_was_rhw = TRUE;
    Context_MarkStateDirty(context, STATE_VDECL, StateTable); /* because of last_was_rhw = TRUE */

    glDisable(GL_CLIP_PLANE0); checkGLcall("glDisable(clip plane 0)");
    glDisable(GL_CLIP_PLANE1); checkGLcall("glDisable(clip plane 1)");
    glDisable(GL_CLIP_PLANE2); checkGLcall("glDisable(clip plane 2)");
    glDisable(GL_CLIP_PLANE3); checkGLcall("glDisable(clip plane 3)");
    glDisable(GL_CLIP_PLANE4); checkGLcall("glDisable(clip plane 4)");
    glDisable(GL_CLIP_PLANE5); checkGLcall("glDisable(clip plane 5)");
    Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_CLIPPING), StateTable);

    glViewport(0, 0, width, height);
    checkGLcall("glViewport");
    Context_MarkStateDirty(context, STATE_VIEWPORT, StateTable);

    This->shader_backend->shader_fragment_enable((IWineD3DDevice *) This, FALSE);
}

/*****************************************************************************
 * findThreadContextForSwapChain
 *
 * Searches a swapchain for all contexts and picks one for the thread tid.
 * If none can be found the swapchain is requested to create a new context
 *
 *****************************************************************************/
static WineD3DContext *findThreadContextForSwapChain(IWineD3DSwapChain *swapchain, DWORD tid) {
    int i;

    for(i = 0; i < ((IWineD3DSwapChainImpl *) swapchain)->num_contexts; i++) {
        if(((IWineD3DSwapChainImpl *) swapchain)->context[i]->tid == tid) {
            return ((IWineD3DSwapChainImpl *) swapchain)->context[i];
        }

    }

    /* Create a new context for the thread */
    return IWineD3DSwapChainImpl_CreateContextForThread(swapchain);
}

/*****************************************************************************
 * FindContext
 *
 * Finds a context for the current render target and thread
 *
 * Parameters:
 *  target: Render target to find the context for
 *  tid: Thread to activate the context for
 *
 * Returns: The needed context
 *
 *****************************************************************************/
static inline WineD3DContext *FindContext(IWineD3DDeviceImpl *This, IWineD3DSurface *target, DWORD tid, GLint *buffer) {
    IWineD3DSwapChain *swapchain = NULL;
    HRESULT hr;
    BOOL readTexture = wined3d_settings.offscreen_rendering_mode != ORM_FBO && This->render_offscreen;
    WineD3DContext *context = This->activeContext;
    BOOL oldRenderOffscreen = This->render_offscreen;
    const WINED3DFORMAT oldFmt = ((IWineD3DSurfaceImpl *) This->lastActiveRenderTarget)->resource.format;
    const WINED3DFORMAT newFmt = ((IWineD3DSurfaceImpl *) target)->resource.format;
    const struct StateEntry *StateTable = This->shader_backend->StateTable;

    /* To compensate the lack of format switching with some offscreen rendering methods and on onscreen buffers
     * the alpha blend state changes with different render target formats
     */
    if(oldFmt != newFmt) {
        const GlPixelFormatDesc *glDesc;
        const StaticPixelFormatDesc *old = getFormatDescEntry(oldFmt, NULL, NULL);
        const StaticPixelFormatDesc *new = getFormatDescEntry(newFmt, &GLINFO_LOCATION, &glDesc);

        /* Disable blending when the alphaMask has changed and when a format doesn't support blending */
        if((old->alphaMask && !new->alphaMask) || (!old->alphaMask && new->alphaMask) || !(glDesc->Flags & WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING)) {
            Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_ALPHABLENDENABLE), StateTable);
        }
    }

    hr = IWineD3DSurface_GetContainer(target, &IID_IWineD3DSwapChain, (void **) &swapchain);
    if(hr == WINED3D_OK && swapchain) {
        TRACE("Rendering onscreen\n");

        context = findThreadContextForSwapChain(swapchain, tid);

        This->render_offscreen = FALSE;
        /* The context != This->activeContext will catch a NOP context change. This can occur
         * if we are switching back to swapchain rendering in case of FBO or Back Buffer offscreen
         * rendering. No context change is needed in that case
         */

        if(((IWineD3DSwapChainImpl *) swapchain)->frontBuffer == target) {
            *buffer = GL_FRONT;
        } else {
            *buffer = GL_BACK;
        }
        if(wined3d_settings.offscreen_rendering_mode == ORM_PBUFFER) {
            if(This->pbufferContext && tid == This->pbufferContext->tid) {
                This->pbufferContext->tid = 0;
            }
        }
        IWineD3DSwapChain_Release(swapchain);

        if(oldRenderOffscreen) {
            Context_MarkStateDirty(context, WINED3DTS_PROJECTION, StateTable);
            Context_MarkStateDirty(context, STATE_VDECL, StateTable);
            Context_MarkStateDirty(context, STATE_VIEWPORT, StateTable);
            Context_MarkStateDirty(context, STATE_SCISSORRECT, StateTable);
            Context_MarkStateDirty(context, STATE_FRONTFACE, StateTable);
        }

    } else {
        TRACE("Rendering offscreen\n");
        This->render_offscreen = TRUE;
        *buffer = This->offscreenBuffer;

        switch(wined3d_settings.offscreen_rendering_mode) {
            case ORM_FBO:
                /* FBOs do not need a different context. Stay with whatever context is active at the moment */
                if(This->activeContext && tid == This->lastThread) {
                    context = This->activeContext;
                } else {
                    /* This may happen if the app jumps straight into offscreen rendering
                     * Start using the context of the primary swapchain. tid == 0 is no problem
                     * for findThreadContextForSwapChain.
                     *
                     * Can also happen on thread switches - in that case findThreadContextForSwapChain
                     * is perfect to call.
                     */
                    context = findThreadContextForSwapChain(This->swapchains[0], tid);
                }
                break;

            case ORM_PBUFFER:
            {
                IWineD3DSurfaceImpl *targetimpl = (IWineD3DSurfaceImpl *) target;
                if(This->pbufferContext == NULL ||
                   This->pbufferWidth < targetimpl->currentDesc.Width ||
                   This->pbufferHeight < targetimpl->currentDesc.Height) {
                    if(This->pbufferContext) {
                        DestroyContext(This, This->pbufferContext);
                    }

                    /* The display is irrelevant here, the window is 0. But CreateContext needs a valid X connection.
                     * Create the context on the same server as the primary swapchain. The primary swapchain is exists at this point.
                     */
                    This->pbufferContext = CreateContext(This, targetimpl,
                            ((IWineD3DSwapChainImpl *) This->swapchains[0])->context[0]->win_handle,
                            TRUE /* pbuffer */, &((IWineD3DSwapChainImpl *)This->swapchains[0])->presentParms);
                    This->pbufferWidth = targetimpl->currentDesc.Width;
                    This->pbufferHeight = targetimpl->currentDesc.Height;
                   }

                   if(This->pbufferContext) {
                       if(This->pbufferContext->tid != 0 && This->pbufferContext->tid != tid) {
                           FIXME("The PBuffr context is only supported for one thread for now!\n");
                       }
                       This->pbufferContext->tid = tid;
                       context = This->pbufferContext;
                       break;
                   } else {
                       ERR("Failed to create a buffer context and drawable, falling back to back buffer offscreen rendering\n");
                       wined3d_settings.offscreen_rendering_mode = ORM_BACKBUFFER;
                   }
            }

            case ORM_BACKBUFFER:
                /* Stay with the currently active context for back buffer rendering */
                if(This->activeContext && tid == This->lastThread) {
                    context = This->activeContext;
                } else {
                    /* This may happen if the app jumps straight into offscreen rendering
                     * Start using the context of the primary swapchain. tid == 0 is no problem
                     * for findThreadContextForSwapChain.
                     *
                     * Can also happen on thread switches - in that case findThreadContextForSwapChain
                     * is perfect to call.
                     */
                    context = findThreadContextForSwapChain(This->swapchains[0], tid);
                }
                break;
        }

        if (wined3d_settings.offscreen_rendering_mode != ORM_FBO) {
            /* Make sure we have a OpenGL texture name so the PreLoad() used to read the buffer
             * back when we are done won't mark us dirty.
             */
            IWineD3DSurface_PreLoad(target);
        }

        if(!oldRenderOffscreen) {
            Context_MarkStateDirty(context, WINED3DTS_PROJECTION, StateTable);
            Context_MarkStateDirty(context, STATE_VDECL, StateTable);
            Context_MarkStateDirty(context, STATE_VIEWPORT, StateTable);
            Context_MarkStateDirty(context, STATE_SCISSORRECT, StateTable);
            Context_MarkStateDirty(context, STATE_FRONTFACE, StateTable);
        }
    }
    if (readTexture) {
        BOOL oldInDraw = This->isInDraw;

        /* PreLoad requires a context to load the texture, thus it will call ActivateContext.
         * Set the isInDraw to true to signal PreLoad that it has a context. Will be tricky
         * when using offscreen rendering with multithreading
         */
        This->isInDraw = TRUE;

        /* Do that before switching the context:
         * Read the back buffer of the old drawable into the destination texture
         */
        IWineD3DSurface_PreLoad(This->lastActiveRenderTarget);

        /* Assume that the drawable will be modified by some other things now */
        IWineD3DSurface_ModifyLocation(This->lastActiveRenderTarget, SFLAG_INDRAWABLE, FALSE);

        This->isInDraw = oldInDraw;
    }

    if(oldRenderOffscreen != This->render_offscreen && This->depth_copy_state != WINED3D_DCS_NO_COPY) {
        This->depth_copy_state = WINED3D_DCS_COPY;
    }
    return context;
}

/*****************************************************************************
 * ActivateContext
 *
 * Finds a rendering context and drawable matching the device and render
 * target for the current thread, activates them and puts them into the
 * requested state.
 *
 * Params:
 *  This: Device to activate the context for
 *  target: Requested render target
 *  usage: Prepares the context for blitting, drawing or other actions
 *
 *****************************************************************************/
void ActivateContext(IWineD3DDeviceImpl *This, IWineD3DSurface *target, ContextUsage usage) {
    DWORD                         tid = GetCurrentThreadId();
    int                           i;
    DWORD                         dirtyState, idx;
    BYTE                          shift;
    WineD3DContext                *context;
    GLint                         drawBuffer=0;
    const struct StateEntry       *StateTable = This->shader_backend->StateTable;

    TRACE("(%p): Selecting context for render target %p, thread %d\n", This, target, tid);
    if(This->lastActiveRenderTarget != target || tid != This->lastThread) {
        context = FindContext(This, target, tid, &drawBuffer);
        This->lastActiveRenderTarget = target;
        This->lastThread = tid;
    } else {
        /* Stick to the old context */
        context = This->activeContext;
    }

    /* Activate the opengl context */
    if(context != This->activeContext) {
        BOOL ret;

        /* Prevent an unneeded context switch as those are expensive */
        if(context->glCtx && (context->glCtx == pwglGetCurrentContext())) {
            TRACE("Already using gl context %p\n", context->glCtx);
        }
        else {
            TRACE("Switching gl ctx to %p, hdc=%p ctx=%p\n", context, context->hdc, context->glCtx);

            This->shader_backend->shader_fragment_enable((IWineD3DDevice *) This, FALSE);
            ret = pwglMakeCurrent(context->hdc, context->glCtx);
            if(ret == FALSE) {
                ERR("Failed to activate the new context\n");
            } else if(!context->last_was_blit) {
                This->shader_backend->shader_fragment_enable((IWineD3DDevice *) This, TRUE);
            }
        }
        if(This->activeContext->vshader_const_dirty) {
            memset(This->activeContext->vshader_const_dirty, 1,
                   sizeof(*This->activeContext->vshader_const_dirty) * GL_LIMITS(vshader_constantsF));
        }
        if(This->activeContext->pshader_const_dirty) {
            memset(This->activeContext->pshader_const_dirty, 1,
                   sizeof(*This->activeContext->pshader_const_dirty) * GL_LIMITS(pshader_constantsF));
        }
        This->activeContext = context;
    }

    /* We only need ENTER_GL for the gl calls made below and for the helper functions which make GL calls */
    ENTER_GL();
    /* Select the right draw buffer. It is selected in FindContext. */
    if(drawBuffer && context->last_draw_buffer != drawBuffer) {
        TRACE("Drawing to buffer: %#x\n", drawBuffer);
        context->last_draw_buffer = drawBuffer;

        glDrawBuffer(drawBuffer);
        checkGLcall("glDrawBuffer");
    }

    switch(usage) {
        case CTXUSAGE_RESOURCELOAD:
            /* This does not require any special states to be set up */
            break;

        case CTXUSAGE_CLEAR:
            if(context->last_was_blit) {
                This->shader_backend->shader_fragment_enable((IWineD3DDevice *) This, TRUE);
            }

            /* Blending and clearing should be orthogonal, but tests on the nvidia driver show that disabling
             * blending when clearing improves the clearing performance incredibly.
             */
            glDisable(GL_BLEND);
            Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_ALPHABLENDENABLE), StateTable);

            glEnable(GL_SCISSOR_TEST);
            checkGLcall("glEnable GL_SCISSOR_TEST");
            context->last_was_blit = FALSE;
            Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_SCISSORTESTENABLE), StateTable);
            Context_MarkStateDirty(context, STATE_SCISSORRECT, StateTable);
            break;

        case CTXUSAGE_DRAWPRIM:
            /* This needs all dirty states applied */
            if(context->last_was_blit) {
                This->shader_backend->shader_fragment_enable((IWineD3DDevice *) This, TRUE);
            }

            IWineD3DDeviceImpl_FindTexUnitMap(This);

            for(i=0; i < context->numDirtyEntries; i++) {
                dirtyState = context->dirtyArray[i];
                idx = dirtyState >> 5;
                shift = dirtyState & 0x1f;
                context->isStateDirty[idx] &= ~(1 << shift);
                StateTable[dirtyState].apply(dirtyState, This->stateBlock, context);
            }
            context->numDirtyEntries = 0; /* This makes the whole list clean */
            context->last_was_blit = FALSE;
            break;

        case CTXUSAGE_BLIT:
            SetupForBlit(This, context,
                         ((IWineD3DSurfaceImpl *)target)->currentDesc.Width,
                         ((IWineD3DSurfaceImpl *)target)->currentDesc.Height);
            break;

        default:
            FIXME("Unexpected context usage requested\n");
    }
    LEAVE_GL();
}
