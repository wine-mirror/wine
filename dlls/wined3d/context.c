/*
 * Context and render target management in wined3d
 *
 * Copyright 2007 Stefan Dösinger for CodeWeavers
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

#define GLINFO_LOCATION ((IWineD3DImpl *)(This->wineD3D))->gl_info

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
 *
 *****************************************************************************/
static void Context_MarkStateDirty(WineD3DContext *context, DWORD state) {
    DWORD rep = StateTable[state].representative;
    DWORD idx;
    BYTE shift;

    if(!rep || isStateDirty(context, rep)) return;

    context->dirtyArray[context->numDirtyEntries++] = rep;
    idx = rep >> 5;
    shift = rep & 0x1f;
    context->isStateDirty[idx] |= (1 << shift);
}

/* Returns an array of compatible FBconfig(s).
 * The array must be freed with XFree. Requires ENTER_GL()
 */
static GLXFBConfig* pbuffer_find_fbconfigs(
    IWineD3DDeviceImpl* This,
    IWineD3DSwapChainImpl* implicitSwapchainImpl,
    IWineD3DSurfaceImpl* RenderSurface) {

    GLXFBConfig* cfgs = NULL;
    int nCfgs = 0;
    int attribs[256];
    int nAttribs = 0;

    IWineD3DSurface *StencilSurface = This->stencilBufferTarget;
    WINED3DFORMAT BackBufferFormat = RenderSurface->resource.format;
    WINED3DFORMAT StencilBufferFormat = (NULL != StencilSurface) ? ((IWineD3DSurfaceImpl *) StencilSurface)->resource.format : 0;

    /* TODO:
     *  if StencilSurface == NULL && zBufferTarget != NULL then switch the zbuffer off,
     *  it StencilSurface != NULL && zBufferTarget == NULL switch it on
     */

#define PUSH1(att)        attribs[nAttribs++] = (att);
#define PUSH2(att,value)  attribs[nAttribs++] = (att); attribs[nAttribs++] = (value);

    /* PUSH2(GLX_BIND_TO_TEXTURE_RGBA_ATI, True); examples of this are few and far between (but I've got a nice working one!)*/

    PUSH2(GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT);
    PUSH2(GLX_X_RENDERABLE,  TRUE);
    PUSH2(GLX_DOUBLEBUFFER,  TRUE);
    TRACE("calling makeglcfg\n");
    D3DFmtMakeGlCfg(BackBufferFormat, StencilBufferFormat, attribs, &nAttribs, FALSE /* alternate */);
    PUSH1(None);
    TRACE("calling chooseFGConfig\n");
    cfgs = glXChooseFBConfig(implicitSwapchainImpl->display,
                             DefaultScreen(implicitSwapchainImpl->display),
                             attribs, &nCfgs);
    if (cfgs == NULL) {
        /* OK we didn't find the exact config, so use any reasonable match */
        /* TODO: fill in the 'requested' and 'current' depths, and make sure that's
           why we failed. */
        static BOOL show_message = TRUE;
        if (show_message) {
            ERR("Failed to find exact match, finding alternative but you may "
                "suffer performance issues, try changing xfree's depth to match the requested depth\n");
            show_message = FALSE;
        }
        nAttribs = 0;
        PUSH2(GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT | GLX_WINDOW_BIT);
        /* PUSH2(GLX_X_RENDERABLE,  TRUE); */
        PUSH2(GLX_RENDER_TYPE,   GLX_RGBA_BIT);
        PUSH2(GLX_DOUBLEBUFFER, FALSE);
        TRACE("calling makeglcfg\n");
        D3DFmtMakeGlCfg(BackBufferFormat, StencilBufferFormat, attribs, &nAttribs, TRUE /* alternate */);
        PUSH1(None);
        cfgs = glXChooseFBConfig(implicitSwapchainImpl->display,
                                 DefaultScreen(implicitSwapchainImpl->display),
                                 attribs, &nCfgs);
    }

    if (cfgs == NULL) {
        ERR("Could not get a valid FBConfig for (%u,%s)/(%u,%s)\n",
            BackBufferFormat, debug_d3dformat(BackBufferFormat),
            StencilBufferFormat, debug_d3dformat(StencilBufferFormat));
    } else {
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
            glXSwapBuffers(implicitSwapChainImpl->display,
                           implicitSwapChainImpl->drawable);
            printf("Hit Enter to get next frame ...\n");
            getchar();
        }
#endif
    }
#undef PUSH1
#undef PUSH2

   return cfgs;
}

/*****************************************************************************
 * CreateContext
 *
 * Creates a new context for a window, or a pbuffer context.
 * TODO: Merge this with AddContext
 *
 * Params:
 *  This: Device to activate the context for
 *  target: Surface this context will render to
 *  win: Taget window. NULL for a pbuffer
 *
 *****************************************************************************/
WineD3DContext *CreateContext(IWineD3DDeviceImpl *This, IWineD3DSurfaceImpl *target, Window win) {
    Drawable drawable = win, oldDrawable;
    XVisualInfo *visinfo = NULL;
    GLXFBConfig *cfgs = NULL;
    GLXContext ctx = NULL, oldCtx;
    WineD3DContext *ret = NULL;
    IWineD3DSwapChainImpl *swapchain = (IWineD3DSwapChainImpl *) This->swapchains[0];

    TRACE("(%p): Creating a %s context for render target %p\n", This, win ? "onscreen" : "offscreen", target);

    if(!win) {
        int attribs[256];
        int nAttribs = 0;

        TRACE("Creating a pBuffer drawable for the new context\n");

        cfgs = pbuffer_find_fbconfigs(This, swapchain, target);
        if(!cfgs) {
            ERR("Cannot find a frame buffer configuration for the pbuffer\n");
            goto out;
        }

        attribs[nAttribs++] = GLX_PBUFFER_WIDTH;
        attribs[nAttribs++] = target->currentDesc.Width;
        attribs[nAttribs++] = GLX_PBUFFER_HEIGHT;
        attribs[nAttribs++] = target->currentDesc.Height;
        attribs[nAttribs++] = None;

        visinfo = glXGetVisualFromFBConfig(swapchain->display, cfgs[0]);
        if(!visinfo) {
            ERR("Cannot find a visual for the pbuffer\n");
            goto out;
        }

        drawable = glXCreatePbuffer(swapchain->display, cfgs[0], attribs);

        if(!drawable) {
            ERR("Cannot create a pbuffer\n");
            goto out;
        }
        XFree(cfgs);
        cfgs = NULL;
    }

    ctx = glXCreateContext(swapchain->display, visinfo,
                           This->numContexts ? This->contexts[0]->glCtx : NULL,
                           GL_TRUE);
    if(!ctx) {
        ERR("Failed to create a glX context\n");
        if(drawable != win) glXDestroyPbuffer(swapchain->display, drawable);
        goto out;
    }
    ret = AddContext(This, ctx, drawable);
    if(!ret) {
        ERR("Failed to add the newly created context to the context list\n");
        glXDestroyContext(swapchain->display, ctx);
        if(drawable != win) glXDestroyPbuffer(swapchain->display, drawable);
        goto out;
    }
    ret->surface = (IWineD3DSurface *) target;

    TRACE("Successfully created new context %p\n", ret);

    /* Set up the context defaults */
    oldCtx  = glXGetCurrentContext();
    oldDrawable = glXGetCurrentDrawable();
    if(glXMakeCurrent(swapchain->display, drawable, ctx) == FALSE) {
        ERR("Cannot activate context to set up defaults\n");
        goto out;
    }

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

    glPixelStorei(GL_PACK_ALIGNMENT, SURFACE_ALIGNMENT);
    checkGLcall("glPixelStorei(GL_PACK_ALIGNMENT, SURFACE_ALIGNMENT);");
    glPixelStorei(GL_UNPACK_ALIGNMENT, SURFACE_ALIGNMENT);
    checkGLcall("glPixelStorei(GL_UNPACK_ALIGNMENT, SURFACE_ALIGNMENT);");

    glXMakeCurrent(swapchain->display, oldDrawable, oldCtx);

out:
    if(visinfo) XFree(visinfo);
    if(cfgs) XFree(cfgs);
    return ret;
}

/*****************************************************************************
 * DestroyContext
 *
 * Destroys a wineD3DContext
 * TODO: Merge with DeleteContext
 *
 * Params:
 *  This: Device to activate the context for
 *  context: Context to destroy
 *
 *****************************************************************************/
void DestroyContext(IWineD3DDeviceImpl *This, WineD3DContext *context) {
    IWineD3DSwapChainImpl *swapchain = (IWineD3DSwapChainImpl *) This->swapchains[0];

    glXDestroyContext(swapchain->display, context->glCtx);
    /* Assume pbuffer for now*/
    glXDestroyPbuffer(swapchain->display, context->drawable);
    DeleteContext(This, context);
}

/*****************************************************************************
 * SetupForBlit
 *
 * Sets up a context for DirectDraw blitting.
 * All texture units are disabled, except unit 0
 * Texture unit 0 is activted where GL_TEXTURE_2D is activated
 * fog, lighting, blending, alpha test, z test, scissor test, culling diabled
 * color writing enabled for all channels
 * register combiners disabled, shaders disabled
 * world matris is set to identity, texture matrix 0 too
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

    TRACE("Setting up context %p for blitting\n", context);
    if(context->last_was_blit) {
        TRACE("Context is already set up for blitting, nothing to do\n");
        return;
    }
    context->last_was_blit = TRUE;

    /* TODO: Use a display list */

    /* Disable shaders */
    This->shader_backend->shader_cleanup((IWineD3DDevice *) This);
    Context_MarkStateDirty(context, STATE_VSHADER);
    Context_MarkStateDirty(context, STATE_PIXELSHADER);

    /* Disable all textures. The caller can then bind a texture it wants to blit
     * from
     */
    if(GL_SUPPORT(NV_REGISTER_COMBINERS)) {
        glDisable(GL_REGISTER_COMBINERS_NV);
        checkGLcall("glDisable(GL_REGISTER_COMBINERS_NV)");
    }
    if (GL_SUPPORT(ARB_MULTITEXTURE)) {
        for(i = GL_LIMITS(samplers) - 1; i > 0 ; i--) {
            if (GL_SUPPORT(ARB_MULTITEXTURE)) {
                GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB + i));
                checkGLcall("glActiveTextureARB");
            }
            glDisable(GL_TEXTURE_CUBE_MAP_ARB);
            checkGLcall("glDisable GL_TEXTURE_CUBE_MAP_ARB");
            glDisable(GL_TEXTURE_3D);
            checkGLcall("glDisable GL_TEXTURE_3D");
            glDisable(GL_TEXTURE_2D);
            checkGLcall("glDisable GL_TEXTURE_2D");
            glDisable(GL_TEXTURE_1D);
            checkGLcall("glDisable GL_TEXTURE_1D");

            if(i < MAX_TEXTURES) {
                Context_MarkStateDirty(context, STATE_TEXTURESTAGE(i, WINED3DTSS_COLOROP));
            }
            Context_MarkStateDirty(context, STATE_SAMPLER(i));
        }
        GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB));
        checkGLcall("glActiveTextureARB");
    }
    glDisable(GL_TEXTURE_CUBE_MAP_ARB);
    checkGLcall("glDisable GL_TEXTURE_CUBE_MAP_ARB");
    glDisable(GL_TEXTURE_3D);
    checkGLcall("glDisable GL_TEXTURE_3D");
    glDisable(GL_TEXTURE_1D);
    checkGLcall("glDisable GL_TEXTURE_1D");
    glEnable(GL_TEXTURE_2D);
    checkGLcall("glEnable GL_TEXTURE_2D");

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    glMatrixMode(GL_TEXTURE);
    checkGLcall("glMatrixMode(GL_TEXTURE)");
    glLoadIdentity();
    checkGLcall("glLoadIdentity()");
    Context_MarkStateDirty(context, STATE_TRANSFORM(WINED3DTS_TEXTURE0));

    if (GL_SUPPORT(EXT_TEXTURE_LOD_BIAS)) {
        glTexEnvf(GL_TEXTURE_FILTER_CONTROL_EXT,
                  GL_TEXTURE_LOD_BIAS_EXT,
                  0.0);
        checkGLcall("glTexEnvi GL_TEXTURE_LOD_BIAS_EXT ...");
    }
    Context_MarkStateDirty(context, STATE_SAMPLER(0));
    Context_MarkStateDirty(context, STATE_TEXTURESTAGE(0, WINED3DTSS_COLOROP));

    /* Other misc states */
    glDisable(GL_ALPHA_TEST);
    checkGLcall("glDisable(GL_ALPHA_TEST)");
    Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_ALPHATESTENABLE));
    glDisable(GL_LIGHTING);
    checkGLcall("glDisable GL_LIGHTING");
    Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_LIGHTING));
    glDisable(GL_DEPTH_TEST);
    checkGLcall("glDisable GL_DEPTH_TEST");
    Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_ZENABLE));
    glDisable(GL_FOG);
    checkGLcall("glDisable GL_FOG");
    Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_FOGENABLE));
    glDisable(GL_BLEND);
    checkGLcall("glDisable GL_BLEND");
    Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_ALPHABLENDENABLE));
    glDisable(GL_CULL_FACE);
    checkGLcall("glDisable GL_CULL_FACE");
    Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_CULLMODE));
    glDisable(GL_STENCIL_TEST);
    checkGLcall("glDisable GL_STENCIL_TEST");
    Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_STENCILENABLE));
    if(GL_SUPPORT(ARB_POINT_SPRITE)) {
        glDisable(GL_POINT_SPRITE_ARB);
        checkGLcall("glDisable GL_POINT_SPRITE_ARB");
        Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_POINTSPRITEENABLE));
    }
    glColorMask(GL_TRUE, GL_TRUE,GL_TRUE,GL_TRUE);
    checkGLcall("glColorMask");
    Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_CLIPPING));

    /* Setup transforms */
    glMatrixMode(GL_MODELVIEW);
    checkGLcall("glMatrixMode(GL_MODELVIEW)");
    glLoadIdentity();
    checkGLcall("glLoadIdentity()");
    Context_MarkStateDirty(context, STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(0)));

    glMatrixMode(GL_PROJECTION);
    checkGLcall("glMatrixMode(GL_PROJECTION)");
    glLoadIdentity();
    checkGLcall("glLoadIdentity()");
    glOrtho(0, width, height, 0, 0.0, -1.0);
    checkGLcall("glOrtho");
    Context_MarkStateDirty(context, STATE_TRANSFORM(WINED3DTS_PROJECTION));

    context->last_was_rhw = TRUE;
    Context_MarkStateDirty(context, STATE_VDECL); /* because of last_was_rhw = TRUE */

    glDisable(GL_CLIP_PLANE0); checkGLcall("glDisable(clip plane 0)");
    glDisable(GL_CLIP_PLANE1); checkGLcall("glDisable(clip plane 1)");
    glDisable(GL_CLIP_PLANE2); checkGLcall("glDisable(clip plane 2)");
    glDisable(GL_CLIP_PLANE3); checkGLcall("glDisable(clip plane 3)");
    glDisable(GL_CLIP_PLANE4); checkGLcall("glDisable(clip plane 4)");
    glDisable(GL_CLIP_PLANE5); checkGLcall("glDisable(clip plane 5)");
    Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_CLIPPING));

    glViewport(0, 0, width, height);
    checkGLcall("glViewport");
    Context_MarkStateDirty(context, STATE_VIEWPORT);
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
    DWORD tid = This->createParms.BehaviorFlags & D3DCREATE_MULTITHREADED ? GetCurrentThreadId() : 0;
    int                           i;
    DWORD                         dirtyState, idx;
    BYTE                          shift;
    WineD3DContext                *context = This->activeContext;
    BOOL                          oldRenderOffscreen = This->render_offscreen;

    TRACE("(%p): Selecting context for render target %p, thread %d\n", This, target, tid);

    if(This->lastActiveRenderTarget != target) {
        IWineD3DSwapChain *swapchain = NULL;
        HRESULT hr;
        BOOL readTexture = wined3d_settings.offscreen_rendering_mode != ORM_FBO && This->render_offscreen;

        hr = IWineD3DSurface_GetContainer(target, &IID_IWineD3DSwapChain, (void **) &swapchain);
        if(hr == WINED3D_OK && swapchain) {
            TRACE("Rendering onscreen\n");
            context = ((IWineD3DSwapChainImpl *) swapchain)->context;
            This->render_offscreen = FALSE;
            /* The context != This->activeContext will catch a NOP context change. This can occur
             * if we are switching back to swapchain rendering in case of FBO or Back Buffer offscreen
             * rendering. No context change is needed in that case
             */

            if (wined3d_settings.offscreen_rendering_mode == ORM_FBO && oldRenderOffscreen) {
                set_render_target_fbo((IWineD3DDevice *) This, 0, target);
            }
            IWineD3DSwapChain_Release(swapchain);

            if(oldRenderOffscreen) {
                Context_MarkStateDirty(context, WINED3DRS_CULLMODE);
            }
        } else {
            TRACE("Rendering offscreen\n");
            This->render_offscreen = TRUE;

            switch(wined3d_settings.offscreen_rendering_mode) {
                case ORM_FBO:
                    /* FBOs do not need a different context. Stay with whatever context is active at the moment */
                    if(This->activeContext) {
                        context = This->activeContext;
                    } else {
                        /* This may happen if the app jumps streight into offscreen rendering
                         * Start using the context of the primary swapchain
                         */
                        context = ((IWineD3DSwapChainImpl *) This->swapchains[0])->context;
                    }
                    set_render_target_fbo((IWineD3DDevice *) This, 0, target);
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
                        This->pbufferContext = CreateContext(This, targetimpl, 0 /* Window */);
                        This->pbufferWidth = targetimpl->currentDesc.Width;
                        This->pbufferHeight = targetimpl->currentDesc.Height;
                    }

                    if(This->pbufferContext) {
                        context = This->pbufferContext;
                        break;
                    } else {
                        ERR("Failed to create a buffer context and drawable, falling back to back buffer offscreen rendering\n");
                        wined3d_settings.offscreen_rendering_mode = ORM_BACKBUFFER;
                    }
                }

                case ORM_BACKBUFFER:
                    /* Stay with the currently active context for back buffer rendering */
                    if(This->activeContext) {
                        context = This->activeContext;
                    } else {
                        /* This may happen if the app jumps streight into offscreen rendering
                         * Start using the context of the primary swapchain
                         */
                        context = ((IWineD3DSwapChainImpl *) This->swapchains[0])->context;
                    }
                    break;
            }

            if(!oldRenderOffscreen) {
                Context_MarkStateDirty(context, WINED3DRS_CULLMODE);
            }
        }
        if (readTexture) {
            /* Do that before switching the context:
             * Read the back buffer of the old drawable into the destination texture
             */
            IWineD3DSurface_SetPBufferState(This->lastActiveRenderTarget, TRUE /* inPBuffer */, FALSE /* inTexture */);
            IWineD3DSurface_AddDirtyRect(This->lastActiveRenderTarget, NULL);
            IWineD3DSurface_PreLoad(This->lastActiveRenderTarget);
            IWineD3DSurface_SetPBufferState(This->lastActiveRenderTarget, FALSE /* inPBuffer */, FALSE /* inTexture */);
        }
        This->lastActiveRenderTarget = target;
        if(oldRenderOffscreen != This->render_offscreen && This->depth_copy_state != WINED3D_DCS_NO_COPY) {
            This->depth_copy_state = WINED3D_DCS_COPY;
        }
    } else {
        /* Stick to the old context */
        context = This->activeContext;
    }

    /* Activate the opengl context */
    if(context != This->activeContext) {
        Bool ret;
        TRACE("Switching gl ctx to %p, drawable=%ld, ctx=%p\n", context, context->drawable, context->glCtx);
        ret = glXMakeCurrent(((IWineD3DSwapChainImpl *) This->swapchains[0])->display, context->drawable, context->glCtx);
        if(ret == FALSE) {
            ERR("Failed to activate the new context\n");
        }
        This->activeContext = context;
    }

    switch(usage) {
        case CTXUSAGE_RESOURCELOAD:
            /* This does not require any special states to be set up */
            break;

        case CTXUSAGE_DRAWPRIM:
            /* This needs all dirty states applied */
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
}

/*****************************************************************************
 * AddContext
 *
 * Adds a new WineD3DContext keeping track of a glx context and drawable.
 * Creating the context and drawable is up to the swapchain or texture. This
 * function adds it to the context array to allow ActivateContext to find it
 * and keep track of dirty states. (Later on it will also be able to duplicate
 * the context for another thread).
 *
 * This method is not called in performance-critical code paths, only when a
 * new render target or swapchain is created. Thus performance is not an issue
 * here.
 *
 * Params:
 *  This: Device to add the context for
 *  glCtx: glX context to add
 *  drawable: drawable used with this context.
 *
 *****************************************************************************/
WineD3DContext *AddContext(IWineD3DDeviceImpl *This, GLXContext glCtx, Drawable drawable) {
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

    This->contexts[This->numContexts]->glCtx = glCtx;
    This->contexts[This->numContexts]->drawable = drawable;
    HeapFree(GetProcessHeap(), 0, oldArray);

    /* Mark all states dirty to force a proper initialization of the states on the first use of the context
     */
    for(state = 0; state <= STATE_HIGHEST; state++) {
        Context_MarkStateDirty(This->contexts[This->numContexts], state);
    }

    This->numContexts++;
    TRACE("Created context %p\n", This->contexts[This->numContexts - 1]);
    return This->contexts[This->numContexts - 1];
}

/*****************************************************************************
 * DeleteContext
 *
 * Removes a context from the context manager. The opengl context is not
 * destroyed or unset. context is not a valid pointer after that call.
 *
 * Simmilar to the former call this isn't a performance critical function.
 *
 * Params:
 *  This: Device to activate the context for
 *  context: Context to remove
 *
 *****************************************************************************/
void DeleteContext(IWineD3DDeviceImpl *This, WineD3DContext *context) {
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
        for(s = 0; s < This->numContexts; s++) {
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
