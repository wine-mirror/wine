/*
 * Direct3D state management
 *
 * Copyright 2006 Stefan Dösinger for CodeWeavers
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

#define GLINFO_LOCATION ((IWineD3DImpl *)(stateblock->wineD3DDevice->wineD3D))->gl_info

static void state_unknown(DWORD state, IWineD3DStateBlockImpl *stateblock) {
    /* State which does exist, but wined3d doesn't know about */
    if(STATE_IS_RENDER(state)) {
        WINED3DRENDERSTATETYPE RenderState = state - STATE_RENDER(0);
        FIXME("(%s, %d) Unknown renderstate\n", debug_d3drenderstate(RenderState), stateblock->renderState[RenderState]);
    } else {
        FIXME("(%d) Unknown state with unknown type\n", state);
    }
}

static void state_nogl(DWORD state, IWineD3DStateBlockImpl *stateblock) {
    /* Used for states which are not mapped to a gl state as-is, but used somehow different,
     * e.g as a parameter for drawing, or which are unimplemented in windows d3d
     */
    if(STATE_IS_RENDER(state)) {
        WINED3DRENDERSTATETYPE RenderState = state - STATE_RENDER(0);
        TRACE("(%s,%d) no direct mapping to gl\n", debug_d3drenderstate(RenderState), stateblock->renderState[RenderState]);
    } else {
        /* Shouldn't have an unknown type here */
        FIXME("%d no direct mapping to gl of state with unknown type\n", state);
    }
}

static void state_undefined(DWORD state, IWineD3DStateBlockImpl *stateblock) {
    /* Print a WARN, this allows the stateblock code to loop over all states to generate a display
     * list without causing confusing terminal output. Deliberately no special debug name here
     * because its undefined.
     */
    WARN("undefined state %d\n", state);
}

static void state_fillmode(DWORD state, IWineD3DStateBlockImpl *stateblock) {
    D3DFILLMODE Value = stateblock->renderState[WINED3DRS_FILLMODE];

    switch(Value) {
        case D3DFILL_POINT:
            glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
            checkGLcall("glPolygonMode(GL_FRONT_AND_BACK, GL_POINT)");
            break;
        case D3DFILL_WIREFRAME:
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            checkGLcall("glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)");
            break;
        case D3DFILL_SOLID:
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            checkGLcall("glPolygonMode(GL_FRONT_AND_BACK, GL_FILL)");
            break;
        default:
            FIXME("Unrecognized WINED3DRS_FILLMODE value %d\n", Value);
    }
}

static void state_lighting(DWORD state, IWineD3DStateBlockImpl *stateblock) {

    /* TODO: Lighting is only enabled if Vertex normals are passed by the application,
     * so merge the lighting render state with the vertex declaration once it is available
     */

    if (stateblock->renderState[WINED3DRS_LIGHTING]) {
        glEnable(GL_LIGHTING);
        checkGLcall("glEnable GL_LIGHTING");
    } else {
        glDisable(GL_LIGHTING);
        checkGLcall("glDisable GL_LIGHTING");
    }
}

static void state_zenable(DWORD state, IWineD3DStateBlockImpl *stateblock) {
    switch ((WINED3DZBUFFERTYPE) stateblock->renderState[WINED3DRS_ZENABLE]) {
        case WINED3DZB_FALSE:
            glDisable(GL_DEPTH_TEST);
            checkGLcall("glDisable GL_DEPTH_TEST");
            break;
        case WINED3DZB_TRUE:
            glEnable(GL_DEPTH_TEST);
            checkGLcall("glEnable GL_DEPTH_TEST");
            break;
        case WINED3DZB_USEW:
            glEnable(GL_DEPTH_TEST);
            checkGLcall("glEnable GL_DEPTH_TEST");
            FIXME("W buffer is not well handled\n");
            break;
        default:
            FIXME("Unrecognized D3DZBUFFERTYPE value %d\n", stateblock->renderState[WINED3DRS_ZENABLE]);
    }
}

static void state_cullmode(DWORD state, IWineD3DStateBlockImpl *stateblock) {
    /* TODO: Put this into the offscreen / onscreen rendering block due to device->render_offscreen */

    /* If we are culling "back faces with clockwise vertices" then
       set front faces to be counter clockwise and enable culling
       of back faces                                               */
    switch ((WINED3DCULL) stateblock->renderState[WINED3DRS_CULLMODE]) {
        case WINED3DCULL_NONE:
            glDisable(GL_CULL_FACE);
            checkGLcall("glDisable GL_CULL_FACE");
            break;
        case WINED3DCULL_CW:
            glEnable(GL_CULL_FACE);
            checkGLcall("glEnable GL_CULL_FACE");
            if (stateblock->wineD3DDevice->render_offscreen) {
                glFrontFace(GL_CW);
                checkGLcall("glFrontFace GL_CW");
            } else {
                glFrontFace(GL_CCW);
                checkGLcall("glFrontFace GL_CCW");
            }
            glCullFace(GL_BACK);
            break;
        case WINED3DCULL_CCW:
            glEnable(GL_CULL_FACE);
            checkGLcall("glEnable GL_CULL_FACE");
            if (stateblock->wineD3DDevice->render_offscreen) {
                glFrontFace(GL_CCW);
                checkGLcall("glFrontFace GL_CCW");
            } else {
                glFrontFace(GL_CW);
                checkGLcall("glFrontFace GL_CW");
            }
            glCullFace(GL_BACK);
            break;
        default:
            FIXME("Unrecognized/Unhandled WINED3DCULL value %d\n", stateblock->renderState[WINED3DRS_CULLMODE]);
    }
}

static void state_shademode(DWORD state, IWineD3DStateBlockImpl *stateblock) {
    switch ((WINED3DSHADEMODE) stateblock->renderState[WINED3DRS_SHADEMODE]) {
        case WINED3DSHADE_FLAT:
            glShadeModel(GL_FLAT);
            checkGLcall("glShadeModel(GL_FLAT)");
            break;
        case WINED3DSHADE_GOURAUD:
            glShadeModel(GL_SMOOTH);
            checkGLcall("glShadeModel(GL_SMOOTH)");
            break;
        case WINED3DSHADE_PHONG:
            FIXME("WINED3DSHADE_PHONG isn't supported\n");
            break;
        default:
            FIXME("Unrecognized/Unhandled WINED3DSHADEMODE value %d\n", stateblock->renderState[WINED3DRS_SHADEMODE]);
    }
}

static void state_ditherenable(DWORD state, IWineD3DStateBlockImpl *stateblock) {
    if (stateblock->renderState[WINED3DRS_DITHERENABLE]) {
        glEnable(GL_DITHER);
        checkGLcall("glEnable GL_DITHER");
    } else {
        glDisable(GL_DITHER);
        checkGLcall("glDisable GL_DITHER");
    }
}

static void state_zwritenable(DWORD state, IWineD3DStateBlockImpl *stateblock) {
    /* TODO: Test if in d3d z writing is enabled even if ZENABLE is off. If yes,
     * this has to be merged with ZENABLE and ZFUNC
     */
    if (stateblock->renderState[WINED3DRS_ZWRITEENABLE]) {
        glDepthMask(1);
        checkGLcall("glDepthMask(1)");
    } else {
        glDepthMask(0);
        checkGLcall("glDepthMask(0)");
    }
}

static void state_zfunc(DWORD state, IWineD3DStateBlockImpl *stateblock) {
    int glParm = CompareFunc(stateblock->renderState[WINED3DRS_ZFUNC]);

    if(glParm) {
        glDepthFunc(glParm);
        checkGLcall("glDepthFunc");
    }
}

static void state_ambient(DWORD state, IWineD3DStateBlockImpl *stateblock) {
    float col[4];
    D3DCOLORTOGLFLOAT4(stateblock->renderState[WINED3DRS_AMBIENT], col);

    TRACE("Setting ambient to (%f,%f,%f,%f)\n", col[0], col[1], col[2], col[3]);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, col);
    checkGLcall("glLightModel for MODEL_AMBIENT");
}

static void state_blend(DWORD state, IWineD3DStateBlockImpl *stateblock) {
    int srcBlend = GL_ZERO;
    int dstBlend = GL_ZERO;
    float col[4];

    /* GL_LINE_SMOOTH needs GL_BLEND to work, according to the red book, and special blending params */
    /* TODO: Is enabling blending really affected by the blendfactor??? */
    if (stateblock->renderState[WINED3DRS_ALPHABLENDENABLE]      ||
        stateblock->renderState[WINED3DRS_EDGEANTIALIAS]         ||
        stateblock->renderState[WINED3DRS_ANTIALIASEDLINEENABLE] ||
        stateblock->renderState[WINED3DRS_BLENDFACTOR] != 0xFFFFFFFF) {
        glEnable(GL_BLEND);
        checkGLcall("glEnable GL_BLEND");
    } else {
        glDisable(GL_BLEND);
        checkGLcall("glDisable GL_BLEND");
        /* Nothing more to do - get out */
        return;
    };

    switch (stateblock->renderState[WINED3DRS_SRCBLEND]) {
        case WINED3DBLEND_ZERO               : srcBlend = GL_ZERO;  break;
        case WINED3DBLEND_ONE                : srcBlend = GL_ONE;  break;
        case WINED3DBLEND_SRCCOLOR           : srcBlend = GL_SRC_COLOR;  break;
        case WINED3DBLEND_INVSRCCOLOR        : srcBlend = GL_ONE_MINUS_SRC_COLOR;  break;
        case WINED3DBLEND_SRCALPHA           : srcBlend = GL_SRC_ALPHA;  break;
        case WINED3DBLEND_INVSRCALPHA        : srcBlend = GL_ONE_MINUS_SRC_ALPHA;  break;
        case WINED3DBLEND_DESTALPHA          : srcBlend = GL_DST_ALPHA;  break;
        case WINED3DBLEND_INVDESTALPHA       : srcBlend = GL_ONE_MINUS_DST_ALPHA;  break;
        case WINED3DBLEND_DESTCOLOR          : srcBlend = GL_DST_COLOR;  break;
        case WINED3DBLEND_INVDESTCOLOR       : srcBlend = GL_ONE_MINUS_DST_COLOR;  break;
        case WINED3DBLEND_SRCALPHASAT        : srcBlend = GL_SRC_ALPHA_SATURATE;  break;

        case WINED3DBLEND_BOTHSRCALPHA       : srcBlend = GL_SRC_ALPHA;
            dstBlend = GL_SRC_ALPHA;
            break;

        case WINED3DBLEND_BOTHINVSRCALPHA    : srcBlend = GL_ONE_MINUS_SRC_ALPHA;
            dstBlend = GL_ONE_MINUS_SRC_ALPHA;
            break;

        case WINED3DBLEND_BLENDFACTOR        : srcBlend = GL_CONSTANT_COLOR;   break;
        case WINED3DBLEND_INVBLENDFACTOR     : srcBlend = GL_ONE_MINUS_CONSTANT_COLOR;  break;
        default:
            FIXME("Unrecognized src blend value %d\n", stateblock->renderState[WINED3DRS_SRCBLEND]);
    }

    switch (stateblock->renderState[WINED3DRS_DESTBLEND]) {
        case WINED3DBLEND_ZERO               : dstBlend = GL_ZERO;  break;
        case WINED3DBLEND_ONE                : dstBlend = GL_ONE;  break;
        case WINED3DBLEND_SRCCOLOR           : dstBlend = GL_SRC_COLOR;  break;
        case WINED3DBLEND_INVSRCCOLOR        : dstBlend = GL_ONE_MINUS_SRC_COLOR;  break;
        case WINED3DBLEND_SRCALPHA           : dstBlend = GL_SRC_ALPHA;  break;
        case WINED3DBLEND_INVSRCALPHA        : dstBlend = GL_ONE_MINUS_SRC_ALPHA;  break;
        case WINED3DBLEND_DESTALPHA          : dstBlend = GL_DST_ALPHA;  break;
        case WINED3DBLEND_INVDESTALPHA       : dstBlend = GL_ONE_MINUS_DST_ALPHA;  break;
        case WINED3DBLEND_DESTCOLOR          : dstBlend = GL_DST_COLOR;  break;
        case WINED3DBLEND_INVDESTCOLOR       : dstBlend = GL_ONE_MINUS_DST_COLOR;  break;
        case WINED3DBLEND_SRCALPHASAT        : dstBlend = GL_SRC_ALPHA_SATURATE;  break;

        case WINED3DBLEND_BOTHSRCALPHA       : dstBlend = GL_SRC_ALPHA;
            srcBlend = GL_SRC_ALPHA;
            break;

        case WINED3DBLEND_BOTHINVSRCALPHA    : dstBlend = GL_ONE_MINUS_SRC_ALPHA;
            srcBlend = GL_ONE_MINUS_SRC_ALPHA;
            break;

        case D3DBLEND_BLENDFACTOR        : dstBlend = GL_CONSTANT_COLOR;   break;
        case D3DBLEND_INVBLENDFACTOR     : dstBlend = GL_ONE_MINUS_CONSTANT_COLOR;  break;
        default:
            FIXME("Unrecognized dst blend value %d\n", stateblock->renderState[WINED3DRS_DESTBLEND]);
    }

    if(stateblock->renderState[WINED3DRS_EDGEANTIALIAS] ||
       stateblock->renderState[WINED3DRS_ANTIALIASEDLINEENABLE]) {
        glEnable(GL_LINE_SMOOTH);
        checkGLcall("glEnable(GL_LINE_SMOOTH)");
        if(srcBlend != GL_SRC_ALPHA) {
            FIXME("WINED3DRS_EDGEANTIALIAS enabled, but incompatible src blending param - what to do?\n");
            srcBlend = GL_SRC_ALPHA;
        }
        if(dstBlend != GL_ONE_MINUS_SRC_ALPHA) {
            FIXME("WINED3DRS_EDGEANTIALIAS enabled, but incompatible dst blending param - what to do?\n");
            dstBlend = GL_ONE_MINUS_SRC_ALPHA;
        }
    } else {
        glDisable(GL_LINE_SMOOTH);
        checkGLcall("glDisable(GL_LINE_SMOOTH)");
    }

    TRACE("glBlendFunc src=%x, dst=%x\n", srcBlend, dstBlend);
    glBlendFunc(srcBlend, dstBlend);
    checkGLcall("glBlendFunc");

    /* TODO: Remove when state management done */
    stateblock->wineD3DDevice->dstBlend = dstBlend;
    stateblock->wineD3DDevice->srcBlend = srcBlend;

    TRACE("Setting BlendFactor to %d\n", stateblock->renderState[WINED3DRS_BLENDFACTOR]);
    D3DCOLORTOGLFLOAT4(stateblock->renderState[WINED3DRS_BLENDFACTOR], col);
    glBlendColor (col[0],col[1],col[2],col[3]);
    checkGLcall("glBlendColor");
}

static void state_alpha(DWORD state, IWineD3DStateBlockImpl *stateblock) {
    int glParm = 0;
    float ref;
    BOOL enable_ckey = FALSE;

    IWineD3DSurfaceImpl *surf;

    /* Find out if the texture on the first stage has a ckey set
     * The alpha state func reads the texture settings, even though alpha and texture are not grouped
     * together. This is to avoid making a huge alpha+texture+texture stage+ckey block due to the hardly
     * used WINED3DRS_COLORKEYENABLE state(which is d3d <= 3 only). The texture function will call alpha
     * in case it finds some texture+colorkeyenable combination which needs extra care.
     */
    if(stateblock->textures[0]) {
        surf = (IWineD3DSurfaceImpl *) ((IWineD3DTextureImpl *)stateblock->textures[0])->surfaces[0];
        if(surf->CKeyFlags & DDSD_CKSRCBLT) enable_ckey = TRUE;
    }

    if (stateblock->renderState[WINED3DRS_ALPHATESTENABLE] ||
        (stateblock->renderState[WINED3DRS_COLORKEYENABLE] && enable_ckey)) {
        glEnable(GL_ALPHA_TEST);
        checkGLcall("glEnable GL_ALPHA_TEST");
    } else {
        glDisable(GL_ALPHA_TEST);
        checkGLcall("glDisable GL_ALPHA_TEST");
        /* Alpha test is disabled, don't bother setting the params - it will happen on the next
         * enable call
         */
        return;
    }

    if(stateblock->renderState[WINED3DRS_COLORKEYENABLE] && enable_ckey) {
        glParm = GL_NOTEQUAL;
        ref = 0.0;
    } else {
        ref = ((float) stateblock->renderState[WINED3DRS_ALPHAREF]) / 255.0f;
        glParm = CompareFunc(stateblock->renderState[WINED3DRS_ALPHAFUNC]);
    }
    if(glParm) {
        stateblock->wineD3DDevice->alphafunc = glParm; /* Remove when state management done */
        glAlphaFunc(glParm, ref);
        checkGLcall("glAlphaFunc");
    }
    /* TODO: Some texture blending operations seem to affect the alpha test */
}

static void state_clipping(DWORD state, IWineD3DStateBlockImpl *stateblock) {
    DWORD enable  = 0xFFFFFFFF;
    DWORD disable = 0x00000000;

    /* TODO: Keep track of previously enabled clipplanes to avoid unneccessary resetting
     * of already set values
     */

    /* If enabling / disabling all
     * TODO: Is this correct? Doesn't D3DRS_CLIPPING disable clipping on the viewport frustrum?
     */
    if (stateblock->renderState[WINED3DRS_CLIPPING]) {
        enable  = stateblock->renderState[WINED3DRS_CLIPPLANEENABLE];
        disable = ~stateblock->renderState[WINED3DRS_CLIPPLANEENABLE];
    } else {
        disable = 0xffffffff;
        enable  = 0x00;
    }

    if (enable & WINED3DCLIPPLANE0)  { glEnable(GL_CLIP_PLANE0);  checkGLcall("glEnable(clip plane 0)"); }
    if (enable & WINED3DCLIPPLANE1)  { glEnable(GL_CLIP_PLANE1);  checkGLcall("glEnable(clip plane 1)"); }
    if (enable & WINED3DCLIPPLANE2)  { glEnable(GL_CLIP_PLANE2);  checkGLcall("glEnable(clip plane 2)"); }
    if (enable & WINED3DCLIPPLANE3)  { glEnable(GL_CLIP_PLANE3);  checkGLcall("glEnable(clip plane 3)"); }
    if (enable & WINED3DCLIPPLANE4)  { glEnable(GL_CLIP_PLANE4);  checkGLcall("glEnable(clip plane 4)"); }
    if (enable & WINED3DCLIPPLANE5)  { glEnable(GL_CLIP_PLANE5);  checkGLcall("glEnable(clip plane 5)"); }

    if (disable & WINED3DCLIPPLANE0) { glDisable(GL_CLIP_PLANE0); checkGLcall("glDisable(clip plane 0)"); }
    if (disable & WINED3DCLIPPLANE1) { glDisable(GL_CLIP_PLANE1); checkGLcall("glDisable(clip plane 1)"); }
    if (disable & WINED3DCLIPPLANE2) { glDisable(GL_CLIP_PLANE2); checkGLcall("glDisable(clip plane 2)"); }
    if (disable & WINED3DCLIPPLANE3) { glDisable(GL_CLIP_PLANE3); checkGLcall("glDisable(clip plane 3)"); }
    if (disable & WINED3DCLIPPLANE4) { glDisable(GL_CLIP_PLANE4); checkGLcall("glDisable(clip plane 4)"); }
    if (disable & WINED3DCLIPPLANE5) { glDisable(GL_CLIP_PLANE5); checkGLcall("glDisable(clip plane 5)"); }

    /** update clipping status */
    if (enable) {
        stateblock->clip_status.ClipUnion = 0;
        stateblock->clip_status.ClipIntersection = 0xFFFFFFFF;
    } else {
        stateblock->clip_status.ClipUnion = 0;
        stateblock->clip_status.ClipIntersection = 0;
    }
}

static void state_blendop(DWORD state, IWineD3DStateBlockImpl *stateblock) {
    int glParm = GL_FUNC_ADD;

    if(!GL_SUPPORT(EXT_BLEND_MINMAX)) {
        WARN("Unsupported in local OpenGL implementation: glBlendEquation\n");
        return;
    }

    switch ((WINED3DBLENDOP) stateblock->renderState[WINED3DRS_BLENDOP]) {
        case WINED3DBLENDOP_ADD              : glParm = GL_FUNC_ADD;              break;
        case WINED3DBLENDOP_SUBTRACT         : glParm = GL_FUNC_SUBTRACT;         break;
        case WINED3DBLENDOP_REVSUBTRACT      : glParm = GL_FUNC_REVERSE_SUBTRACT; break;
        case WINED3DBLENDOP_MIN              : glParm = GL_MIN;                   break;
        case WINED3DBLENDOP_MAX              : glParm = GL_MAX;                   break;
        default:
            FIXME("Unrecognized/Unhandled D3DBLENDOP value %d\n", stateblock->renderState[WINED3DRS_BLENDOP]);
    }

    TRACE("glBlendEquation(%x)\n", glParm);
    GL_EXTCALL(glBlendEquation(glParm));
    checkGLcall("glBlendEquation");
}

static void
state_specularenable(DWORD state, IWineD3DStateBlockImpl *stateblock) {
    /* Originally this used glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,GL_SEPARATE_SPECULAR_COLOR)
     * and (GL_LIGHT_MODEL_COLOR_CONTROL,GL_SINGLE_COLOR) to swap between enabled/disabled
     * specular color. This is wrong:
     * Separate specular color means the specular colour is maintained separately, whereas
     * single color means it is merged in. However in both cases they are being used to
     * some extent.
     * To disable specular color, set it explicitly to black and turn off GL_COLOR_SUM_EXT
     * NOTE: If not supported don't give FIXMEs the impact is really minimal and very few people are
     * running 1.4 yet!
     *
     *
     * If register combiners are enabled, enabling / disabling GL_COLOR_SUM has no effect.
     * Instead, we need to setup the FinalCombiner properly.
     *
     * The default setup for the FinalCombiner is:
     *
     * <variable>       <input>                             <mapping>               <usage>
     * GL_VARIABLE_A_NV GL_FOG,                             GL_UNSIGNED_IDENTITY_NV GL_ALPHA
     * GL_VARIABLE_B_NV GL_SPARE0_PLUS_SECONDARY_COLOR_NV   GL_UNSIGNED_IDENTITY_NV GL_RGB
     * GL_VARIABLE_C_NV GL_FOG                              GL_UNSIGNED_IDENTITY_NV GL_RGB
     * GL_VARIABLE_D_NV GL_ZERO                             GL_UNSIGNED_IDENTITY_NV GL_RGB
     * GL_VARIABLE_E_NV GL_ZERO                             GL_UNSIGNED_IDENTITY_NV GL_RGB
     * GL_VARIABLE_F_NV GL_ZERO                             GL_UNSIGNED_IDENTITY_NV GL_RGB
     * GL_VARIABLE_G_NV GL_SPARE0_NV                        GL_UNSIGNED_IDENTITY_NV GL_ALPHA
     *
     * That's pretty much fine as it is, except for variable B, which needs to take
     * either GL_SPARE0_PLUS_SECONDARY_COLOR_NV or GL_SPARE0_NV, depending on
     * whether WINED3DRS_SPECULARENABLE is enabled or not.
     */

    TRACE("Setting specular enable state\n");
    /* TODO: Add to the material setting functions */
    if (stateblock->renderState[WINED3DRS_SPECULARENABLE]) {
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, (float*) &stateblock->material.Specular);
        checkGLcall("glMaterialfv");
        if (GL_SUPPORT(EXT_SECONDARY_COLOR)) {
            glEnable(GL_COLOR_SUM_EXT);
        } else {
            TRACE("Specular colors cannot be enabled in this version of opengl\n");
        }
        checkGLcall("glEnable(GL_COLOR_SUM)");

        if (GL_SUPPORT(NV_REGISTER_COMBINERS)) {
            GL_EXTCALL(glFinalCombinerInputNV(GL_VARIABLE_B_NV, GL_SPARE0_PLUS_SECONDARY_COLOR_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB));
            checkGLcall("glFinalCombinerInputNV()");
        }
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

        if (GL_SUPPORT(NV_REGISTER_COMBINERS)) {
            GL_EXTCALL(glFinalCombinerInputNV(GL_VARIABLE_B_NV, GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB));
            checkGLcall("glFinalCombinerInputNV()");
        }
    }
}

static void state_texfactor(DWORD state, IWineD3DStateBlockImpl *stateblock) {
    unsigned int i;

    /* Note the texture color applies to all textures whereas
     * GL_TEXTURE_ENV_COLOR applies to active only
     */
    float col[4];
    D3DCOLORTOGLFLOAT4(stateblock->renderState[WINED3DRS_TEXTUREFACTOR], col);

    if (!GL_SUPPORT(NV_REGISTER_COMBINERS)) {
        /* And now the default texture color as well */
        for (i = 0; i < GL_LIMITS(texture_stages); i++) {
            /* Note the WINED3DRS value applies to all textures, but GL has one
             * per texture, so apply it now ready to be used!
             */
            if (GL_SUPPORT(ARB_MULTITEXTURE)) {
                GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB + i));
                checkGLcall("glActiveTextureARB");
            } else if (i>0) {
                FIXME("Program using multiple concurrent textures which this opengl implementation doesn't support\n");
            }

            glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, &col[0]);
            checkGLcall("glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);");
        }
    }
}

static void
renderstate_stencil_twosided(IWineD3DStateBlockImpl *stateblock, GLint face, GLint func, GLint ref, GLuint mask, GLint stencilFail, GLint depthFail, GLint stencilPass ) {
#if 0 /* Don't use OpenGL 2.0 calls for now */
            if(GL_EXTCALL(glStencilFuncSeparate) && GL_EXTCALL(glStencilOpSeparate)) {
                GL_EXTCALL(glStencilFuncSeparate(face, func, ref, mask));
                checkGLcall("glStencilFuncSeparate(...)");
                GL_EXTCALL(glStencilOpSeparate(face, stencilFail, depthFail, stencilPass));
                checkGLcall("glStencilOpSeparate(...)");
        }
            else
#endif
    if(GL_SUPPORT(EXT_STENCIL_TWO_SIDE)) {
        glEnable(GL_STENCIL_TEST_TWO_SIDE_EXT);
        checkGLcall("glEnable(GL_STENCIL_TEST_TWO_SIDE_EXT)");
        GL_EXTCALL(glActiveStencilFaceEXT(face));
        checkGLcall("glActiveStencilFaceEXT(...)");
        glStencilFunc(func, ref, mask);
        checkGLcall("glStencilFunc(...)");
        glStencilOp(stencilFail, depthFail, stencilPass);
        checkGLcall("glStencilOp(...)");
    } else if(GL_SUPPORT(ATI_SEPARATE_STENCIL)) {
        GL_EXTCALL(glStencilFuncSeparateATI(face, func, ref, mask));
        checkGLcall("glStencilFuncSeparateATI(...)");
        GL_EXTCALL(glStencilOpSeparateATI(face, stencilFail, depthFail, stencilPass));
        checkGLcall("glStencilOpSeparateATI(...)");
    } else {
        ERR("Separate (two sided) stencil not supported on this version of opengl. Caps weren't honored?\n");
    }
}

static void
state_stencil(DWORD state, IWineD3DStateBlockImpl *stateblock) {
    DWORD onesided_enable = FALSE;
    DWORD twosided_enable = FALSE;
    GLint func = GL_ALWAYS;
    GLint func_ccw = GL_ALWAYS;
    GLint ref = 0;
    GLuint mask = 0;
    GLint stencilFail = GL_KEEP;
    GLint depthFail = GL_KEEP;
    GLint stencilPass = GL_KEEP;
    GLint stencilFail_ccw = GL_KEEP;
    GLint depthFail_ccw = GL_KEEP;
    GLint stencilPass_ccw = GL_KEEP;

    if( stateblock->set.renderState[WINED3DRS_STENCILENABLE] )
        onesided_enable = stateblock->renderState[WINED3DRS_STENCILENABLE];
    if( stateblock->set.renderState[WINED3DRS_TWOSIDEDSTENCILMODE] )
        twosided_enable = stateblock->renderState[WINED3DRS_TWOSIDEDSTENCILMODE];
    if( stateblock->set.renderState[WINED3DRS_STENCILFUNC] )
        if( !( func = CompareFunc(stateblock->renderState[WINED3DRS_STENCILFUNC]) ) )
            func = GL_ALWAYS;
    if( stateblock->set.renderState[WINED3DRS_CCW_STENCILFUNC] )
        if( !( func_ccw = CompareFunc(stateblock->renderState[WINED3DRS_CCW_STENCILFUNC]) ) )
            func = GL_ALWAYS;
    if( stateblock->set.renderState[WINED3DRS_STENCILREF] )
        ref = stateblock->renderState[WINED3DRS_STENCILREF];
    if( stateblock->set.renderState[WINED3DRS_STENCILMASK] )
        mask = stateblock->renderState[WINED3DRS_STENCILMASK];
    if( stateblock->set.renderState[WINED3DRS_STENCILFAIL] )
        stencilFail = StencilOp(stateblock->renderState[WINED3DRS_STENCILFAIL]);
    if( stateblock->set.renderState[WINED3DRS_STENCILZFAIL] )
        depthFail = StencilOp(stateblock->renderState[WINED3DRS_STENCILZFAIL]);
    if( stateblock->set.renderState[WINED3DRS_STENCILPASS] )
        stencilPass = StencilOp(stateblock->renderState[WINED3DRS_STENCILPASS]);
    if( stateblock->set.renderState[WINED3DRS_CCW_STENCILFAIL] )
        stencilFail_ccw = StencilOp(stateblock->renderState[WINED3DRS_CCW_STENCILFAIL]);
    if( stateblock->set.renderState[WINED3DRS_CCW_STENCILZFAIL] )
        depthFail_ccw = StencilOp(stateblock->renderState[WINED3DRS_CCW_STENCILZFAIL]);
    if( stateblock->set.renderState[WINED3DRS_CCW_STENCILPASS] )
        stencilPass_ccw = StencilOp(stateblock->renderState[WINED3DRS_CCW_STENCILPASS]);

    TRACE("(onesided %d, twosided %d, ref %x, mask %x,  \
            GL_FRONT: func: %x, fail %x, zfail %x, zpass %x  \
            GL_BACK: func: %x, fail %x, zfail %x, zpass %x )\n",
    onesided_enable, twosided_enable, ref, mask,
    func, stencilFail, depthFail, stencilPass,
    func_ccw, stencilFail_ccw, depthFail_ccw, stencilPass_ccw);

    if (twosided_enable) {
        renderstate_stencil_twosided(stateblock, GL_FRONT, func, ref, mask, stencilFail, depthFail, stencilPass);
        renderstate_stencil_twosided(stateblock, GL_BACK, func_ccw, ref, mask, stencilFail_ccw, depthFail_ccw, stencilPass_ccw);
    } else {
        if (onesided_enable) {
            glEnable(GL_STENCIL_TEST);
            checkGLcall("glEnable GL_STENCIL_TEST");
            glStencilFunc(func, ref, mask);
            checkGLcall("glStencilFunc(...)");
            glStencilOp(stencilFail, depthFail, stencilPass);
            checkGLcall("glStencilOp(...)");
        } else {
            glDisable(GL_STENCIL_TEST);
            checkGLcall("glDisable GL_STENCIL_TEST");
        }
    }
}

static void state_stencilwrite(DWORD state, IWineD3DStateBlockImpl *stateblock) {
    glStencilMask(stateblock->renderState[WINED3DRS_STENCILWRITEMASK]);
    checkGLcall("glStencilMask");
}

static void state_fog(DWORD state, IWineD3DStateBlockImpl *stateblock) {
    /* TODO: Put this into the vertex type block once that is in the state table */
    BOOL fogenable = stateblock->renderState[WINED3DRS_FOGENABLE];
    float fogstart, fogend;

    union {
        DWORD d;
        float f;
    } tmpvalue;

    if (!fogenable) {
        /* No fog? Disable it, and we're done :-) */
        glDisable(GL_FOG);
        checkGLcall("glDisable GL_FOG");
    }

    tmpvalue.d = stateblock->renderState[WINED3DRS_FOGSTART];
    fogstart = tmpvalue.f;
    tmpvalue.d = stateblock->renderState[WINED3DRS_FOGEND];
    fogend = tmpvalue.f;

#if 0
    /* Activate when vertex shaders are in the state table */
    if(stateblock->vertexShader && ((IWineD3DVertexShaderImpl *)stateblock->vertexShader)->baseShader.function &&
       ((IWineD3DVertexShaderImpl *)stateblock->vertexShader)->usesFog) {
        glFogi(GL_FOG_MODE, GL_LINEAR);
        checkGLcall("glFogi(GL_FOG_MODE, GL_LINEAR)");
        fogstart = 1.0;
        fogend = 0.0;
        stateblock->wineD3DDevice->last_was_foggy_shader = TRUE;
    }
#endif

    /* DX 7 sdk: "If both render states(vertex and table fog) are set to valid modes,
     * the system will apply only pixel(=table) fog effects."
     */
    /* else */ if(stateblock->renderState[WINED3DRS_FOGTABLEMODE] == D3DFOG_NONE) {
        glHint(GL_FOG_HINT, GL_FASTEST);
        checkGLcall("glHint(GL_FOG_HINT, GL_FASTEST)");
#if 0
        stateblock->wineD3DDevice->last_was_foggy_shader = FALSE;
#endif
        switch (stateblock->renderState[WINED3DRS_FOGVERTEXMODE]) {
            /* Processed vertices have their fog factor stored in the specular value. Fall too the none case.
             * If we are drawing untransformed vertices atm, d3ddevice_set_ortho will update the fog
             */
            case D3DFOG_EXP:  {
                if(!stateblock->wineD3DDevice->last_was_rhw) {
                    glFogi(GL_FOG_MODE, GL_EXP);
                    checkGLcall("glFogi(GL_FOG_MODE, GL_EXP");
                    if(GL_SUPPORT(EXT_FOG_COORD)) {
                        glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT);
                        checkGLcall("glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT");
                    }
                    break;
                }
            }
            case D3DFOG_EXP2: {
                if(!stateblock->wineD3DDevice->last_was_rhw) {
                    glFogi(GL_FOG_MODE, GL_EXP2);
                    checkGLcall("glFogi(GL_FOG_MODE, GL_EXP2");
                    if(GL_SUPPORT(EXT_FOG_COORD)) {
                        glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT);
                        checkGLcall("glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT");
                    }
                    break;
                }
            }
            case D3DFOG_LINEAR: {
                if(!stateblock->wineD3DDevice->last_was_rhw) {
                    glFogi(GL_FOG_MODE, GL_LINEAR);
                    checkGLcall("glFogi(GL_FOG_MODE, GL_LINEAR");
                    if(GL_SUPPORT(EXT_FOG_COORD)) {
                        glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT);
                        checkGLcall("glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT");
                    }
                    break;
                }
            }
            case D3DFOG_NONE: {
                /* Both are none? According to msdn the alpha channel of the specular
                 * color contains a fog factor. Set it in drawStridedSlow.
                 * Same happens with Vertexfog on transformed vertices
                 */
                if(GL_SUPPORT(EXT_FOG_COORD)) {
                    glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FOG_COORDINATE_EXT);
                    checkGLcall("glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FOG_COORDINATE_EXT)\n");
                    glFogi(GL_FOG_MODE, GL_LINEAR);
                    checkGLcall("glFogi(GL_FOG_MODE, GL_LINEAR)");
                    fogstart = 0xff;
                    fogend = 0x0;
                } else {
                    /* Disable GL fog, handle this in software in drawStridedSlow */
                    fogenable = FALSE;
                }
                break;
            }
            default: FIXME("Unexpected WINED3DRS_FOGVERTEXMODE %d\n", stateblock->renderState[WINED3DRS_FOGVERTEXMODE]);
        }
    } else {
        glHint(GL_FOG_HINT, GL_NICEST);
        checkGLcall("glHint(GL_FOG_HINT, GL_NICEST)");
#if 0
        stateblock->wineD3DDevice->last_was_foggy_shader = FALSE;
#endif
        switch (stateblock->renderState[WINED3DRS_FOGTABLEMODE]) {
            case D3DFOG_EXP:
                glFogi(GL_FOG_MODE, GL_EXP);
                checkGLcall("glFogi(GL_FOG_MODE, GL_EXP");
                if(GL_SUPPORT(EXT_FOG_COORD)) {
                    glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT);
                    checkGLcall("glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT");
                }
                break;

            case D3DFOG_EXP2:
                glFogi(GL_FOG_MODE, GL_EXP2);
                checkGLcall("glFogi(GL_FOG_MODE, GL_EXP2");
                if(GL_SUPPORT(EXT_FOG_COORD)) {
                    glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT);
                    checkGLcall("glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT");
                }
                break;

            case D3DFOG_LINEAR:
                glFogi(GL_FOG_MODE, GL_LINEAR);
                checkGLcall("glFogi(GL_FOG_MODE, GL_LINEAR");
                if(GL_SUPPORT(EXT_FOG_COORD)) {
                    glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT);
                    checkGLcall("glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT");
                }
                break;

            case D3DFOG_NONE:   /* Won't happen */
            default:
                FIXME("Unexpected WINED3DRS_FOGTABLEMODE %d\n", stateblock->renderState[WINED3DRS_FOGTABLEMODE]);
        }
    }

    if(fogenable) {
        glEnable(GL_FOG);
        checkGLcall("glEnable GL_FOG");

        glFogfv(GL_FOG_START, &fogstart);
        checkGLcall("glFogf(GL_FOG_START, fogstart");
        TRACE("Fog Start == %f\n", fogstart);

        glFogfv(GL_FOG_END, &fogend);
        checkGLcall("glFogf(GL_FOG_END, fogend");
        TRACE("Fog End == %f\n", fogend);
    } else {
        glDisable(GL_FOG);
        checkGLcall("glDisable GL_FOG");
    }

    if (GL_SUPPORT(NV_FOG_DISTANCE)) {
        glFogi(GL_FOG_DISTANCE_MODE_NV, GL_EYE_PLANE_ABSOLUTE_NV);
    }
}

static void state_fogcolor(DWORD state, IWineD3DStateBlockImpl *stateblock) {
    float col[4];
    D3DCOLORTOGLFLOAT4(stateblock->renderState[WINED3DRS_FOGCOLOR], col);
    /* Set the default alpha blend color */
    glFogfv(GL_FOG_COLOR, &col[0]);
    checkGLcall("glFog GL_FOG_COLOR");
}

static void state_fogdensity(DWORD state, IWineD3DStateBlockImpl *stateblock) {
    union {
        DWORD d;
        float f;
    } tmpvalue;
    tmpvalue.d = stateblock->renderState[WINED3DRS_FOGDENSITY];
    glFogfv(GL_FOG_DENSITY, &tmpvalue.f);
    checkGLcall("glFogf(GL_FOG_DENSITY, (float) Value)");
}

const struct StateEntry StateTable[] =
{
      /* State name                                         representative,                                     apply function */
    { /* 0,  Undefined                              */      0,                                                  state_undefined     },
    { /* 1,  WINED3DRS_TEXTUREHANDLE                */      0 /* Handled in ddraw */,                           state_undefined     },
    { /* 2,  WINED3DRS_ANTIALIAS                    */      STATE_RENDER(WINED3DRS_ANTIALIAS),                  state_unknown       },
    { /* 3,  WINED3DRS_TEXTUREADDRESS               */      0 /* Handled in ddraw */,                           state_undefined     },
    { /* 4,  WINED3DRS_TEXTUREPERSPECTIVE           */      STATE_RENDER(WINED3DRS_TEXTUREPERSPECTIVE),         state_unknown       },
    { /* 5,  WINED3DRS_WRAPU                        */      STATE_RENDER(WINED3DRS_WRAPU),                      state_unknown       },
    { /* 6,  WINED3DRS_WRAPV                        */      STATE_RENDER(WINED3DRS_WRAPV),                      state_unknown       },
    { /* 7,  WINED3DRS_ZENABLE                      */      STATE_RENDER(WINED3DRS_ZENABLE),                    state_zenable       },
    { /* 8,  WINED3DRS_FILLMODE                     */      STATE_RENDER(WINED3DRS_FILLMODE),                   state_fillmode      },
    { /* 9,  WINED3DRS_SHADEMODE                    */      STATE_RENDER(WINED3DRS_SHADEMODE),                  state_shademode     },
    { /* 10, WINED3DRS_LINEPATTERN                  */      STATE_RENDER(WINED3DRS_LINEPATTERN),                state_unknown       },
    { /* 11, WINED3DRS_MONOENABLE                   */      STATE_RENDER(WINED3DRS_MONOENABLE),                 state_unknown       },
    { /* 12, WINED3DRS_ROP2                         */      STATE_RENDER(WINED3DRS_ROP2),                       state_unknown       },
    { /* 13, WINED3DRS_PLANEMASK                    */      STATE_RENDER(WINED3DRS_PLANEMASK),                  state_unknown       },
    { /* 14, WINED3DRS_ZWRITEENABLE                 */      STATE_RENDER(WINED3DRS_ZWRITEENABLE),               state_zwritenable   },
    { /* 15, WINED3DRS_ALPHATESTENABLE              */      STATE_RENDER(WINED3DRS_ALPHATESTENABLE),            state_alpha         },
    { /* 16, WINED3DRS_LASTPIXEL                    */      STATE_RENDER(WINED3DRS_LASTPIXEL),                  state_unknown       },
    { /* 17, WINED3DRS_TEXTUREMAG                   */      0 /* Handled in ddraw */,                           state_undefined     },
    { /* 18, WINED3DRS_TEXTUREMIN                   */      0 /* Handled in ddraw */,                           state_undefined     },
    { /* 19, WINED3DRS_SRCBLEND                     */      STATE_RENDER(WINED3DRS_ALPHABLENDENABLE),           state_blend         },
    { /* 20, WINED3DRS_DESTBLEND                    */      STATE_RENDER(WINED3DRS_ALPHABLENDENABLE),           state_blend         },
    { /* 21, WINED3DRS_TEXTUREMAPBLEND              */      0 /* Handled in ddraw */,                           state_undefined     },
    { /* 22, WINED3DRS_CULLMODE                     */      STATE_RENDER(WINED3DRS_CULLMODE),                   state_cullmode      },
    { /* 23, WINED3DRS_ZFUNC                        */      STATE_RENDER(WINED3DRS_ZFUNC),                      state_zfunc         },
    { /* 24, WINED3DRS_ALPHAREF                     */      STATE_RENDER(WINED3DRS_ALPHATESTENABLE),            state_alpha         },
    { /* 25, WINED3DRS_ALPHAFUNC                    */      STATE_RENDER(WINED3DRS_ALPHATESTENABLE),            state_alpha         },
    { /* 26, WINED3DRS_DITHERENABLE                 */      STATE_RENDER(WINED3DRS_DITHERENABLE),               state_ditherenable  },
    { /* 27, WINED3DRS_ALPHABLENDENABLE             */      STATE_RENDER(WINED3DRS_ALPHABLENDENABLE),           state_blend         },
    { /* 28, WINED3DRS_FOGENABLE                    */      STATE_RENDER(WINED3DRS_FOGENABLE),                  state_fog           },
    { /* 29, WINED3DRS_SPECULARENABLE               */      STATE_RENDER(WINED3DRS_SPECULARENABLE),             state_specularenable},
    { /* 30, WINED3DRS_ZVISIBLE                     */      0 /* Not supported according to the msdn */,        state_nogl          },
    { /* 31, WINED3DRS_SUBPIXEL                     */      STATE_RENDER(WINED3DRS_SUBPIXEL),                   state_unknown       },
    { /* 32, WINED3DRS_SUBPIXELX                    */      STATE_RENDER(WINED3DRS_SUBPIXELX),                  state_unknown       },
    { /* 33, WINED3DRS_STIPPLEDALPHA                */      STATE_RENDER(WINED3DRS_STIPPLEDALPHA),              state_unknown       },
    { /* 34, WINED3DRS_FOGCOLOR                     */      STATE_RENDER(WINED3DRS_FOGCOLOR),                   state_fogcolor      },
    { /* 35, WINED3DRS_FOGTABLEMODE                 */      STATE_RENDER(WINED3DRS_FOGENABLE),                  state_fog           },
    { /* 36, WINED3DRS_FOGSTART                     */      STATE_RENDER(WINED3DRS_FOGENABLE),                  state_fog           },
    { /* 37, WINED3DRS_FOGEND                       */      STATE_RENDER(WINED3DRS_FOGENABLE),                  state_fog           },
    { /* 38, WINED3DRS_FOGDENSITY                   */      STATE_RENDER(WINED3DRS_FOGDENSITY),                 state_fogdensity    },
    { /* 39, WINED3DRS_STIPPLEENABLE                */      STATE_RENDER(WINED3DRS_STIPPLEENABLE),              state_unknown       },
    { /* 40, WINED3DRS_EDGEANTIALIAS                */      STATE_RENDER(WINED3DRS_ALPHABLENDENABLE),           state_blend         },
    { /* 41, WINED3DRS_COLORKEYENABLE               */      STATE_RENDER(WINED3DRS_ALPHATESTENABLE),            state_alpha         },
    { /* 42, undefined                              */      0,                                                  state_undefined     },
    { /* 43, WINED3DRS_BORDERCOLOR                  */      STATE_RENDER(WINED3DRS_BORDERCOLOR),                state_unknown       },
    { /* 44, WINED3DRS_TEXTUREADDRESSU              */      0, /* Handled in ddraw */                           state_undefined     },
    { /* 45, WINED3DRS_TEXTUREADDRESSV              */      0, /* Handled in ddraw */                           state_undefined     },
    { /* 46, WINED3DRS_MIPMAPLODBIAS                */      STATE_RENDER(WINED3DRS_MIPMAPLODBIAS),              state_unknown       },
    { /* 47, WINED3DRS_ZBIAS                        */      STATE_RENDER(WINED3DRS_ZBIAS),                      state_unknown       },
    { /* 48, WINED3DRS_RANGEFOGENABLE               */      0,                                                  state_nogl          },
    { /* 49, WINED3DRS_ANISOTROPY                   */      STATE_RENDER(WINED3DRS_ANISOTROPY),                 state_unknown       },
    { /* 50, WINED3DRS_FLUSHBATCH                   */      STATE_RENDER(WINED3DRS_FLUSHBATCH),                 state_unknown       },
    { /* 51, WINED3DRS_TRANSLUCENTSORTINDEPENDENT   */      STATE_RENDER(WINED3DRS_TRANSLUCENTSORTINDEPENDENT), state_unknown       },
    { /* 52, WINED3DRS_STENCILENABLE                */      STATE_RENDER(WINED3DRS_STENCILENABLE),              state_stencil       },
    { /* 53, WINED3DRS_STENCILFAIL                  */      STATE_RENDER(WINED3DRS_STENCILENABLE),              state_stencil       },
    { /* 54, WINED3DRS_STENCILZFAIL                 */      STATE_RENDER(WINED3DRS_STENCILENABLE),              state_stencil       },
    { /* 55, WINED3DRS_STENCILPASS                  */      STATE_RENDER(WINED3DRS_STENCILENABLE),              state_stencil       },
    { /* 56, WINED3DRS_STENCILFUNC                  */      STATE_RENDER(WINED3DRS_STENCILENABLE),              state_stencil       },
    { /* 57, WINED3DRS_STENCILREF                   */      STATE_RENDER(WINED3DRS_STENCILENABLE),              state_stencil       },
    { /* 58, WINED3DRS_STENCILMASK                  */      STATE_RENDER(WINED3DRS_STENCILENABLE),              state_stencil       },
    { /* 59, WINED3DRS_STENCILWRITEMASK             */      STATE_RENDER(WINED3DRS_STENCILWRITEMASK),           state_stencilwrite  },
    { /* 60, WINED3DRS_TEXTUREFACTOR                */      STATE_RENDER(WINED3DRS_TEXTUREFACTOR),              state_texfactor     },
    /* A BIG hole. If wanted, 'fixed' states like the vertex type or the bound shaders can be put here */
    { /* 61, Undefined                              */      0,                                                  state_undefined     },
    { /* 62, Undefined                              */      0,                                                  state_undefined     },
    { /* 63, Undefined                              */      0,                                                  state_undefined     },
    { /* 64, Undefined                              */      0,                                                  state_undefined     },
    { /* 65, Undefined                              */      0,                                                  state_undefined     },
    { /* 66, Undefined                              */      0,                                                  state_undefined     },
    { /* 67, Undefined                              */      0,                                                  state_undefined     },
    { /* 68, Undefined                              */      0,                                                  state_undefined     },
    { /* 69, Undefined                              */      0,                                                  state_undefined     },
    { /* 70, Undefined                              */      0,                                                  state_undefined     },
    { /* 71, Undefined                              */      0,                                                  state_undefined     },
    { /* 72, Undefined                              */      0,                                                  state_undefined     },
    { /* 73, Undefined                              */      0,                                                  state_undefined     },
    { /* 74, Undefined                              */      0,                                                  state_undefined     },
    { /* 75, Undefined                              */      0,                                                  state_undefined     },
    { /* 76, Undefined                              */      0,                                                  state_undefined     },
    { /* 77, Undefined                              */      0,                                                  state_undefined     },
    { /* 78, Undefined                              */      0,                                                  state_undefined     },
    { /* 79, Undefined                              */      0,                                                  state_undefined     },
    { /* 80, Undefined                              */      0,                                                  state_undefined     },
    { /* 81, Undefined                              */      0,                                                  state_undefined     },
    { /* 82, Undefined                              */      0,                                                  state_undefined     },
    { /* 83, Undefined                              */      0,                                                  state_undefined     },
    { /* 84, Undefined                              */      0,                                                  state_undefined     },
    { /* 85, Undefined                              */      0,                                                  state_undefined     },
    { /* 86, Undefined                              */      0,                                                  state_undefined     },
    { /* 87, Undefined                              */      0,                                                  state_undefined     },
    { /* 88, Undefined                              */      0,                                                  state_undefined     },
    { /* 89, Undefined                              */      0,                                                  state_undefined     },
    { /* 90, Undefined                              */      0,                                                  state_undefined     },
    { /* 91, Undefined                              */      0,                                                  state_undefined     },
    { /* 92, Undefined                              */      0,                                                  state_undefined     },
    { /* 93, Undefined                              */      0,                                                  state_undefined     },
    { /* 94, Undefined                              */      0,                                                  state_undefined     },
    { /* 95, Undefined                              */      0,                                                  state_undefined     },
    { /* 96, Undefined                              */      0,                                                  state_undefined     },
    { /* 97, Undefined                              */      0,                                                  state_undefined     },
    { /* 98, Undefined                              */      0,                                                  state_undefined     },
    { /* 99, Undefined                              */      0,                                                  state_undefined     },
    { /*100, Undefined                              */      0,                                                  state_undefined     },
    { /*101, Undefined                              */      0,                                                  state_undefined     },
    { /*102, Undefined                              */      0,                                                  state_undefined     },
    { /*103, Undefined                              */      0,                                                  state_undefined     },
    { /*104, Undefined                              */      0,                                                  state_undefined     },
    { /*105, Undefined                              */      0,                                                  state_undefined     },
    { /*106, Undefined                              */      0,                                                  state_undefined     },
    { /*107, Undefined                              */      0,                                                  state_undefined     },
    { /*108, Undefined                              */      0,                                                  state_undefined     },
    { /*109, Undefined                              */      0,                                                  state_undefined     },
    { /*110, Undefined                              */      0,                                                  state_undefined     },
    { /*111, Undefined                              */      0,                                                  state_undefined     },
    { /*112, Undefined                              */      0,                                                  state_undefined     },
    { /*113, Undefined                              */      0,                                                  state_undefined     },
    { /*114, Undefined                              */      0,                                                  state_undefined     },
    { /*115, Undefined                              */      0,                                                  state_undefined     },
    { /*116, Undefined                              */      0,                                                  state_undefined     },
    { /*117, Undefined                              */      0,                                                  state_undefined     },
    { /*118, Undefined                              */      0,                                                  state_undefined     },
    { /*119, Undefined                              */      0,                                                  state_undefined     },
    { /*120, Undefined                              */      0,                                                  state_undefined     },
    { /*121, Undefined                              */      0,                                                  state_undefined     },
    { /*122, Undefined                              */      0,                                                  state_undefined     },
    { /*123, Undefined                              */      0,                                                  state_undefined     },
    { /*124, Undefined                              */      0,                                                  state_undefined     },
    { /*125, Undefined                              */      0,                                                  state_undefined     },
    { /*126, Undefined                              */      0,                                                  state_undefined     },
    { /*127, Undefined                              */      0,                                                  state_undefined     },
    /* Big hole ends */
    { /*128, WINED3DRS_WRAP0                        */      STATE_RENDER(WINED3DRS_WRAP0),                      state_unknown       },
    { /*129, WINED3DRS_WRAP1                        */      STATE_RENDER(WINED3DRS_WRAP0),                      state_unknown       },
    { /*130, WINED3DRS_WRAP2                        */      STATE_RENDER(WINED3DRS_WRAP0),                      state_unknown       },
    { /*131, WINED3DRS_WRAP3                        */      STATE_RENDER(WINED3DRS_WRAP0),                      state_unknown       },
    { /*132, WINED3DRS_WRAP4                        */      STATE_RENDER(WINED3DRS_WRAP0),                      state_unknown       },
    { /*133, WINED3DRS_WRAP5                        */      STATE_RENDER(WINED3DRS_WRAP0),                      state_unknown       },
    { /*134, WINED3DRS_WRAP6                        */      STATE_RENDER(WINED3DRS_WRAP0),                      state_unknown       },
    { /*135, WINED3DRS_WRAP7                        */      STATE_RENDER(WINED3DRS_WRAP0),                      state_unknown       },
    { /*136, WINED3DRS_CLIPPING                     */      STATE_RENDER(WINED3DRS_CLIPPING),                   state_clipping      },
    { /*137, WINED3DRS_LIGHTING                     */      STATE_RENDER(WINED3DRS_LIGHTING) /* Vertex decl! */,state_lighting      },
    { /*138, WINED3DRS_EXTENTS                      */      STATE_RENDER(WINED3DRS_EXTENTS),                    state_unknown       },
    { /*139, WINED3DRS_AMBIENT                      */      STATE_RENDER(WINED3DRS_AMBIENT),                    state_ambient       },
    { /*140, WINED3DRS_FOGVERTEXMODE                */      STATE_RENDER(WINED3DRS_FOGENABLE),                  state_fog           },
    { /*141, WINED3DRS_COLORVERTEX                  */      STATE_RENDER(WINED3DRS_COLORVERTEX),                state_unknown       },
    { /*142, WINED3DRS_LOCALVIEWER                  */      STATE_RENDER(WINED3DRS_LOCALVIEWER),                state_unknown       },
    { /*143, WINED3DRS_NORMALIZENORMALS             */      STATE_RENDER(WINED3DRS_NORMALIZENORMALS),           state_unknown       },
    { /*144, WINED3DRS_COLORKEYBLENDENABLE          */      STATE_RENDER(WINED3DRS_COLORKEYBLENDENABLE),        state_unknown       },
    { /*145, WINED3DRS_DIFFUSEMATERIALSOURCE        */      STATE_RENDER(WINED3DRS_COLORVERTEX),                state_unknown       },
    { /*146, WINED3DRS_SPECULARMATERIALSOURCE       */      STATE_RENDER(WINED3DRS_COLORVERTEX),                state_unknown       },
    { /*147, WINED3DRS_AMBIENTMATERIALSOURCE        */      STATE_RENDER(WINED3DRS_COLORVERTEX),                state_unknown       },
    { /*148, WINED3DRS_EMISSIVEMATERIALSOURCE       */      STATE_RENDER(WINED3DRS_COLORVERTEX),                state_unknown       },
    { /*149, Undefined                              */      0,                                                  state_undefined     },
    { /*150, Undefined                              */      0,                                                  state_undefined     },
    { /*151, WINED3DRS_VERTEXBLEND                  */      0,                                                  state_nogl          },
    { /*152, WINED3DRS_CLIPPLANEENABLE              */      STATE_RENDER(WINED3DRS_CLIPPING),                   state_clipping      },
    { /*153, WINED3DRS_SOFTWAREVERTEXPROCESSING     */      0,                                                  state_nogl          },
    { /*154, WINED3DRS_POINTSIZE                    */      STATE_RENDER(WINED3DRS_POINTSIZE),                  state_unknown       },
    { /*155, WINED3DRS_POINTSIZE_MIN                */      STATE_RENDER(WINED3DRS_POINTSIZE_MIN),              state_unknown       },
    { /*156, WINED3DRS_POINTSPRITEENABLE            */      STATE_RENDER(WINED3DRS_POINTSPRITEENABLE),          state_unknown       },
    { /*157, WINED3DRS_POINTSCALEENABLE             */      STATE_RENDER(WINED3DRS_POINTSCALEENABLE),           state_unknown       },
    { /*158, WINED3DRS_POINTSCALE_A                 */      STATE_RENDER(WINED3DRS_POINTSCALEENABLE),           state_unknown       },
    { /*159, WINED3DRS_POINTSCALE_B                 */      STATE_RENDER(WINED3DRS_POINTSCALEENABLE),           state_unknown       },
    { /*160, WINED3DRS_POINTSCALE_C                 */      STATE_RENDER(WINED3DRS_POINTSCALEENABLE),           state_unknown       },
    { /*161, WINED3DRS_MULTISAMPLEANTIALIAS         */      STATE_RENDER(WINED3DRS_MULTISAMPLEANTIALIAS),       state_unknown       },
    { /*162, WINED3DRS_MULTISAMPLEMASK              */      STATE_RENDER(WINED3DRS_MULTISAMPLEMASK),            state_unknown       },
    { /*163, WINED3DRS_PATCHEDGESTYLE               */      STATE_RENDER(WINED3DRS_PATCHEDGESTYLE),             state_unknown       },
    { /*164, WINED3DRS_PATCHSEGMENTS                */      STATE_RENDER(WINED3DRS_PATCHSEGMENTS),              state_unknown       },
    { /*165, WINED3DRS_DEBUGMONITORTOKEN            */      STATE_RENDER(WINED3DRS_DEBUGMONITORTOKEN),          state_unknown       },
    { /*166, WINED3DRS_POINTSIZE_MAX                */      STATE_RENDER(WINED3DRS_POINTSIZE_MAX),              state_unknown       },
    { /*167, WINED3DRS_INDEXEDVERTEXBLENDENABLE     */      STATE_RENDER(WINED3DRS_INDEXEDVERTEXBLENDENABLE),   state_unknown       },
    { /*168, WINED3DRS_COLORWRITEENABLE             */      STATE_RENDER(WINED3DRS_COLORWRITEENABLE),           state_unknown       },
    { /*169, Undefined                              */      0,                                                  state_undefined     },
    { /*170, WINED3DRS_TWEENFACTOR                  */      0,                                                  state_nogl          },
    { /*171, WINED3DRS_BLENDOP                      */      STATE_RENDER(WINED3DRS_BLENDOP),                    state_blendop       },
    { /*172, WINED3DRS_POSITIONDEGREE               */      STATE_RENDER(WINED3DRS_POSITIONDEGREE),             state_unknown       },
    { /*173, WINED3DRS_NORMALDEGREE                 */      STATE_RENDER(WINED3DRS_NORMALDEGREE),               state_unknown       },
      /*172, WINED3DRS_POSITIONORDER                */      /* Value assigned to 2 state names */
      /*173, WINED3DRS_NORMALORDER                  */      /* Value assigned to 2 state names */
    { /*174, WINED3DRS_SCISSORTESTENABLE            */      STATE_RENDER(WINED3DRS_SCISSORTESTENABLE),          state_unknown       },
    { /*175, WINED3DRS_SLOPESCALEDEPTHBIAS          */      STATE_RENDER(WINED3DRS_DEPTHBIAS),                  state_unknown       },
    { /*176, WINED3DRS_ANTIALIASEDLINEENABLE        */      STATE_RENDER(WINED3DRS_ALPHABLENDENABLE),           state_blend         },
    { /*177, undefined                              */      0,                                                  state_undefined     },
    { /*178, WINED3DRS_MINTESSELLATIONLEVEL         */      STATE_RENDER(WINED3DRS_ENABLEADAPTIVETESSELLATION), state_unknown       },
    { /*179, WINED3DRS_MAXTESSELLATIONLEVEL         */      STATE_RENDER(WINED3DRS_ENABLEADAPTIVETESSELLATION), state_unknown       },
    { /*180, WINED3DRS_ADAPTIVETESS_X               */      STATE_RENDER(WINED3DRS_ENABLEADAPTIVETESSELLATION), state_unknown       },
    { /*181, WINED3DRS_ADAPTIVETESS_Y               */      STATE_RENDER(WINED3DRS_ENABLEADAPTIVETESSELLATION), state_unknown       },
    { /*182, WINED3DRS_ADAPTIVETESS_Z               */      STATE_RENDER(WINED3DRS_ENABLEADAPTIVETESSELLATION), state_unknown       },
    { /*183, WINED3DRS_ADAPTIVETESS_W               */      STATE_RENDER(WINED3DRS_ENABLEADAPTIVETESSELLATION), state_unknown       },
    { /*184, WINED3DRS_ENABLEADAPTIVETESSELLATION   */      STATE_RENDER(WINED3DRS_ENABLEADAPTIVETESSELLATION), state_unknown       },
    { /*185, WINED3DRS_TWOSIDEDSTENCILMODE          */      STATE_RENDER(WINED3DRS_STENCILENABLE),              state_stencil       },
    { /*186, WINED3DRS_CCW_STENCILFAIL              */      STATE_RENDER(WINED3DRS_STENCILENABLE),              state_stencil       },
    { /*187, WINED3DRS_CCW_STENCILZFAIL             */      STATE_RENDER(WINED3DRS_STENCILENABLE),              state_stencil       },
    { /*188, WINED3DRS_CCW_STENCILPASS              */      STATE_RENDER(WINED3DRS_STENCILENABLE),              state_stencil       },
    { /*189, WINED3DRS_CCW_STENCILFUNC              */      STATE_RENDER(WINED3DRS_STENCILENABLE),              state_stencil       },
    { /*190, WINED3DRS_COLORWRITEENABLE1            */      STATE_RENDER(WINED3DRS_COLORWRITEENABLE),           state_unknown       },
    { /*191, WINED3DRS_COLORWRITEENABLE2            */      STATE_RENDER(WINED3DRS_COLORWRITEENABLE),           state_unknown       },
    { /*192, WINED3DRS_COLORWRITEENABLE3            */      STATE_RENDER(WINED3DRS_COLORWRITEENABLE),           state_unknown       },
    { /*193, WINED3DRS_BLENDFACTOR                  */      STATE_RENDER(WINED3DRS_ALPHABLENDENABLE),           state_blend         },
    { /*194, WINED3DRS_SRGBWRITEENABLE              */      0,                                                  state_nogl          },
    { /*195, WINED3DRS_DEPTHBIAS                    */      STATE_RENDER(WINED3DRS_DEPTHBIAS),                  state_unknown       },
    { /*196, undefined                              */      0,                                                  state_undefined     },
    { /*197, undefined                              */      0,                                                  state_undefined     },
    { /*198, WINED3DRS_WRAP8                        */      STATE_RENDER(WINED3DRS_WRAP0),                      state_unknown       },
    { /*199, WINED3DRS_WRAP9                        */      STATE_RENDER(WINED3DRS_WRAP0),                      state_unknown       },
    { /*200, WINED3DRS_WRAP10                       */      STATE_RENDER(WINED3DRS_WRAP0),                      state_unknown       },
    { /*201, WINED3DRS_WRAP11                       */      STATE_RENDER(WINED3DRS_WRAP0),                      state_unknown       },
    { /*202, WINED3DRS_WRAP12                       */      STATE_RENDER(WINED3DRS_WRAP0),                      state_unknown       },
    { /*203, WINED3DRS_WRAP13                       */      STATE_RENDER(WINED3DRS_WRAP0),                      state_unknown       },
    { /*204, WINED3DRS_WRAP14                       */      STATE_RENDER(WINED3DRS_WRAP0),                      state_unknown       },
    { /*205, WINED3DRS_WRAP15                       */      STATE_RENDER(WINED3DRS_WRAP0),                      state_unknown       },
    { /*206, WINED3DRS_SEPARATEALPHABLENDENABLE     */      STATE_RENDER(WINED3DRS_SEPARATEALPHABLENDENABLE),   state_unknown       },
    { /*207, WINED3DRS_SRCBLENDALPHA                */      STATE_RENDER(WINED3DRS_SEPARATEALPHABLENDENABLE),   state_unknown       },
    { /*208, WINED3DRS_DESTBLENDALPHA               */      STATE_RENDER(WINED3DRS_SEPARATEALPHABLENDENABLE),   state_unknown       },
    { /*209, WINED3DRS_BLENDOPALPHA                 */      STATE_RENDER(WINED3DRS_SEPARATEALPHABLENDENABLE),   state_unknown       },
};
