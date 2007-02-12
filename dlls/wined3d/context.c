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

    /* Disable shaders */
    This->shader_backend->shader_cleanup((IWineD3DDevice *) This);
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_VSHADER);
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_PIXELSHADER);

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
                IWineD3DDeviceImpl_MarkStateDirty(This, STATE_TEXTURESTAGE(i, WINED3DTSS_COLOROP));
            }
            IWineD3DDeviceImpl_MarkStateDirty(This, STATE_SAMPLER(i));
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
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_TRANSFORM(WINED3DTS_TEXTURE0));

    if (GL_SUPPORT(EXT_TEXTURE_LOD_BIAS)) {
        glTexEnvf(GL_TEXTURE_FILTER_CONTROL_EXT,
                  GL_TEXTURE_LOD_BIAS_EXT,
                  0.0);
        checkGLcall("glTexEnvi GL_TEXTURE_LOD_BIAS_EXT ...");
    }
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_SAMPLER(0));
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_TEXTURESTAGE(0, WINED3DTSS_COLOROP));

    /* Other misc states */
    glDisable(GL_ALPHA_TEST);
    checkGLcall("glDisable(GL_ALPHA_TEST)");
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_RENDER(WINED3DRS_ALPHATESTENABLE));
    glDisable(GL_LIGHTING);
    checkGLcall("glDisable GL_LIGHTING");
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_RENDER(WINED3DRS_LIGHTING));
    glDisable(GL_DEPTH_TEST);
    checkGLcall("glDisable GL_DEPTH_TEST");
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_RENDER(WINED3DRS_ZENABLE));
    glDisable(GL_FOG);
    checkGLcall("glDisable GL_FOG");
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_RENDER(WINED3DRS_FOGENABLE));
    glDisable(GL_BLEND);
    checkGLcall("glDisable GL_BLEND");
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_RENDER(WINED3DRS_ALPHABLENDENABLE));
    glDisable(GL_CULL_FACE);
    checkGLcall("glDisable GL_CULL_FACE");
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_RENDER(WINED3DRS_CULLMODE));
    glDisable(GL_STENCIL_TEST);
    checkGLcall("glDisable GL_STENCIL_TEST");
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_RENDER(WINED3DRS_STENCILENABLE));
    if(GL_SUPPORT(ARB_POINT_SPRITE)) {
        glDisable(GL_POINT_SPRITE_ARB);
        checkGLcall("glDisable GL_POINT_SPRITE_ARB");
        IWineD3DDeviceImpl_MarkStateDirty(This, STATE_RENDER(WINED3DRS_POINTSPRITEENABLE));
    }
    glColorMask(GL_TRUE, GL_TRUE,GL_TRUE,GL_TRUE);
    checkGLcall("glColorMask");
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_RENDER(WINED3DRS_CLIPPING));

    /* Setup transforms */
    glMatrixMode(GL_MODELVIEW);
    checkGLcall("glMatrixMode(GL_MODELVIEW)");
    glLoadIdentity();
    checkGLcall("glLoadIdentity()");
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(0)));

    glMatrixMode(GL_PROJECTION);
    checkGLcall("glMatrixMode(GL_PROJECTION)");
    glLoadIdentity();
    checkGLcall("glLoadIdentity()");
    glOrtho(0, width, height, 0, 0.0, -1.0);
    checkGLcall("glOrtho");
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_TRANSFORM(WINED3DTS_PROJECTION));

    context->last_was_rhw = TRUE;
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_VDECL); /* because of last_was_rhw = TRUE */

    glDisable(GL_CLIP_PLANE0); checkGLcall("glDisable(clip plane 0)");
    glDisable(GL_CLIP_PLANE1); checkGLcall("glDisable(clip plane 1)");
    glDisable(GL_CLIP_PLANE2); checkGLcall("glDisable(clip plane 2)");
    glDisable(GL_CLIP_PLANE3); checkGLcall("glDisable(clip plane 3)");
    glDisable(GL_CLIP_PLANE4); checkGLcall("glDisable(clip plane 4)");
    glDisable(GL_CLIP_PLANE5); checkGLcall("glDisable(clip plane 5)");
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_RENDER(WINED3DRS_CLIPPING));

    glViewport(0, 0, width, height);
    checkGLcall("glViewport");
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_VIEWPORT);
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
    WineD3DContext                *context;

    TRACE("(%p): Selecting context for render target %p, thread %d\n", This, target, tid);
    /* TODO: Render target selection */

    /* TODO: Thread selection */

    /* TODO: Activate the opengl context */
    context = &This->contexts[This->activeContext];

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
