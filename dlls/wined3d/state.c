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
    { /* 15, WINED3DRS_ALPHATESTENABLE              */      STATE_RENDER(WINED3DRS_ALPHATESTENABLE),            state_unknown       },
    { /* 16, WINED3DRS_LASTPIXEL                    */      STATE_RENDER(WINED3DRS_LASTPIXEL),                  state_unknown       },
    { /* 17, WINED3DRS_TEXTUREMAG                   */      0 /* Handled in ddraw */,                           state_undefined     },
    { /* 18, WINED3DRS_TEXTUREMIN                   */      0 /* Handled in ddraw */,                           state_undefined     },
    { /* 19, WINED3DRS_SRCBLEND                     */      STATE_RENDER(WINED3DRS_ALPHABLENDENABLE),           state_unknown       },
    { /* 20, WINED3DRS_DESTBLEND                    */      STATE_RENDER(WINED3DRS_ALPHABLENDENABLE),           state_unknown       },
    { /* 21, WINED3DRS_TEXTUREMAPBLEND              */      0 /* Handled in ddraw */,                           state_undefined     },
    { /* 22, WINED3DRS_CULLMODE                     */      STATE_RENDER(WINED3DRS_CULLMODE),                   state_cullmode      },
    { /* 23, WINED3DRS_ZFUNC                        */      STATE_RENDER(WINED3DRS_ZFUNC),                      state_zfunc         },
    { /* 24, WINED3DRS_ALPHAREF                     */      STATE_RENDER(WINED3DRS_ALPHATESTENABLE),            state_unknown       },
    { /* 25, WINED3DRS_ALPHAFUNC                    */      STATE_RENDER(WINED3DRS_ALPHATESTENABLE),            state_unknown       },
    { /* 26, WINED3DRS_DITHERENABLE                 */      STATE_RENDER(WINED3DRS_DITHERENABLE),               state_ditherenable  },
    { /* 27, WINED3DRS_ALPHABLENDENABLE             */      STATE_RENDER(WINED3DRS_ALPHABLENDENABLE),           state_unknown       },
    { /* 28, WINED3DRS_FOGENABLE                    */      STATE_RENDER(WINED3DRS_FOGENABLE),                  state_unknown       },
    { /* 29, WINED3DRS_SPECULARENABLE               */      STATE_RENDER(WINED3DRS_SPECULARENABLE),             state_unknown       },
    { /* 30, WINED3DRS_ZVISIBLE                     */      0 /* Not supported according to the msdn */,        state_nogl          },
    { /* 31, WINED3DRS_SUBPIXEL                     */      STATE_RENDER(WINED3DRS_SUBPIXEL),                   state_unknown       },
    { /* 32, WINED3DRS_SUBPIXELX                    */      STATE_RENDER(WINED3DRS_SUBPIXELX),                  state_unknown       },
    { /* 33, WINED3DRS_STIPPLEDALPHA                */      STATE_RENDER(WINED3DRS_STIPPLEDALPHA),              state_unknown       },
    { /* 34, WINED3DRS_FOGCOLOR                     */      STATE_RENDER(WINED3DRS_FOGCOLOR),                   state_unknown       },
    { /* 35, WINED3DRS_FOGTABLEMODE                 */      STATE_RENDER(WINED3DRS_FOGENABLE)/*vertex type*/,   state_unknown       },
    { /* 36, WINED3DRS_FOGSTART                     */      STATE_RENDER(WINED3DRS_FOGENABLE),                  state_unknown       },
    { /* 37, WINED3DRS_FOGEND                       */      STATE_RENDER(WINED3DRS_FOGENABLE),                  state_unknown       },
    { /* 38, WINED3DRS_FOGDENSITY                   */      STATE_RENDER(WINED3DRS_FOGDENSITY),                 state_unknown       },
    { /* 39, WINED3DRS_STIPPLEENABLE                */      STATE_RENDER(WINED3DRS_STIPPLEENABLE),              state_unknown       },
    { /* 40, WINED3DRS_EDGEANTIALIAS                */      STATE_RENDER(WINED3DRS_ALPHABLENDENABLE),           state_unknown       },
    { /* 41, WINED3DRS_COLORKEYENABLE               */      STATE_RENDER(WINED3DRS_ALPHATESTENABLE),            state_unknown       },
    { /* 42, undefined                              */      0,                                                  state_undefined     },
    { /* 43, WINED3DRS_BORDERCOLOR                  */      STATE_RENDER(WINED3DRS_BORDERCOLOR),                state_unknown       },
    { /* 44, WINED3DRS_TEXTUREADDRESSU              */      0, /* Handled in ddraw */                           state_undefined     },
    { /* 45, WINED3DRS_TEXTUREADDRESSV              */      0, /* Handled in ddraw */                           state_undefined     },
    { /* 46, WINED3DRS_MIPMAPLODBIAS                */      STATE_RENDER(WINED3DRS_MIPMAPLODBIAS),              state_unknown       },
    { /* 47, WINED3DRS_ZBIAS                        */      STATE_RENDER(WINED3DRS_ZBIAS),                      state_unknown       },
    { /* 48, WINED3DRS_RANGEFOGENABLE               */      STATE_RENDER(WINED3DRS_RANGEFOGENABLE),             state_unknown       },
    { /* 49, WINED3DRS_ANISOTROPY                   */      STATE_RENDER(WINED3DRS_ANISOTROPY),                 state_unknown       },
    { /* 50, WINED3DRS_FLUSHBATCH                   */      STATE_RENDER(WINED3DRS_FLUSHBATCH),                 state_unknown       },
    { /* 51, WINED3DRS_TRANSLUCENTSORTINDEPENDENT   */      STATE_RENDER(WINED3DRS_TRANSLUCENTSORTINDEPENDENT), state_unknown       },
    { /* 52, WINED3DRS_STENCILENABLE                */      STATE_RENDER(WINED3DRS_STENCILENABLE),              state_unknown       },
    { /* 53, WINED3DRS_STENCILFAIL                  */      STATE_RENDER(WINED3DRS_STENCILENABLE),              state_unknown       },
    { /* 54, WINED3DRS_STENCILZFAIL                 */      STATE_RENDER(WINED3DRS_STENCILENABLE),              state_unknown       },
    { /* 55, WINED3DRS_STENCILPASS                  */      STATE_RENDER(WINED3DRS_STENCILENABLE),              state_unknown       },
    { /* 56, WINED3DRS_STENCILFUNC                  */      STATE_RENDER(WINED3DRS_STENCILENABLE),              state_unknown       },
    { /* 57, WINED3DRS_STENCILREF                   */      STATE_RENDER(WINED3DRS_STENCILENABLE),              state_unknown       },
    { /* 58, WINED3DRS_STENCILMASK                  */      STATE_RENDER(WINED3DRS_STENCILENABLE),              state_unknown       },
    { /* 59, WINED3DRS_STENCILWRITEMASK             */      STATE_RENDER(WINED3DRS_STENCILWRITEMASK),           state_unknown       },
    { /* 60, WINED3DRS_TEXTUREFACTOR                */      STATE_RENDER(WINED3DRS_TEXTUREFACTOR),              state_unknown       },
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
    { /*136, WINED3DRS_CLIPPING                     */      STATE_RENDER(WINED3DRS_CLIPPING),                   state_unknown       },
    { /*137, WINED3DRS_LIGHTING                     */      STATE_RENDER(WINED3DRS_LIGHTING) /* Vertex decl! */,state_lighting      },
    { /*138, WINED3DRS_EXTENTS                      */      STATE_RENDER(WINED3DRS_EXTENTS),                    state_unknown       },
    { /*139, WINED3DRS_AMBIENT                      */      STATE_RENDER(WINED3DRS_AMBIENT),                    state_unknown       },
    { /*140, WINED3DRS_FOGVERTEXMODE                */      STATE_RENDER(WINED3DRS_FOGENABLE),                  state_unknown       },
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
    { /*152, WINED3DRS_CLIPPLANEENABLE              */      STATE_RENDER(WINED3DRS_CLIPPING),                   state_unknown       },
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
    { /*171, WINED3DRS_BLENDOP                      */      STATE_RENDER(WINED3DRS_BLENDOP),                    state_unknown       },
    { /*172, WINED3DRS_POSITIONDEGREE               */      STATE_RENDER(WINED3DRS_POSITIONDEGREE),             state_unknown       },
    { /*173, WINED3DRS_NORMALDEGREE                 */      STATE_RENDER(WINED3DRS_NORMALDEGREE),               state_unknown       },
      /*172, WINED3DRS_POSITIONORDER                */      /* Value assigned to 2 state names */
      /*173, WINED3DRS_NORMALORDER                  */      /* Value assigned to 2 state names */
    { /*174, WINED3DRS_SCISSORTESTENABLE            */      STATE_RENDER(WINED3DRS_SCISSORTESTENABLE),          state_unknown       },
    { /*175, WINED3DRS_SLOPESCALEDEPTHBIAS          */      STATE_RENDER(WINED3DRS_DEPTHBIAS),                  state_unknown       },
    { /*176, WINED3DRS_ANTIALIASEDLINEENABLE        */      STATE_RENDER(WINED3DRS_ALPHABLENDENABLE),           state_unknown       },
    { /*177, undefined                              */      0,                                                  state_undefined     },
    { /*178, WINED3DRS_MINTESSELLATIONLEVEL         */      STATE_RENDER(WINED3DRS_ENABLEADAPTIVETESSELLATION), state_unknown       },
    { /*179, WINED3DRS_MAXTESSELLATIONLEVEL         */      STATE_RENDER(WINED3DRS_ENABLEADAPTIVETESSELLATION), state_unknown       },
    { /*180, WINED3DRS_ADAPTIVETESS_X               */      STATE_RENDER(WINED3DRS_ENABLEADAPTIVETESSELLATION), state_unknown       },
    { /*181, WINED3DRS_ADAPTIVETESS_Y               */      STATE_RENDER(WINED3DRS_ENABLEADAPTIVETESSELLATION), state_unknown       },
    { /*182, WINED3DRS_ADAPTIVETESS_Z               */      STATE_RENDER(WINED3DRS_ENABLEADAPTIVETESSELLATION), state_unknown       },
    { /*183, WINED3DRS_ADAPTIVETESS_W               */      STATE_RENDER(WINED3DRS_ENABLEADAPTIVETESSELLATION), state_unknown       },
    { /*184, WINED3DRS_ENABLEADAPTIVETESSELLATION   */      STATE_RENDER(WINED3DRS_ENABLEADAPTIVETESSELLATION), state_unknown       },
    { /*185, WINED3DRS_TWOSIDEDSTENCILMODE          */      STATE_RENDER(WINED3DRS_STENCILENABLE),              state_unknown       },
    { /*186, WINED3DRS_CCW_STENCILFAIL              */      STATE_RENDER(WINED3DRS_STENCILENABLE),              state_unknown       },
    { /*187, WINED3DRS_CCW_STENCILZFAIL             */      STATE_RENDER(WINED3DRS_STENCILENABLE),              state_unknown       },
    { /*188, WINED3DRS_CCW_STENCILPASS              */      STATE_RENDER(WINED3DRS_STENCILENABLE),              state_unknown       },
    { /*189, WINED3DRS_CCW_STENCILFUNC              */      STATE_RENDER(WINED3DRS_STENCILENABLE),              state_unknown       },
    { /*190, WINED3DRS_COLORWRITEENABLE1            */      STATE_RENDER(WINED3DRS_COLORWRITEENABLE),           state_unknown       },
    { /*191, WINED3DRS_COLORWRITEENABLE2            */      STATE_RENDER(WINED3DRS_COLORWRITEENABLE),           state_unknown       },
    { /*192, WINED3DRS_COLORWRITEENABLE3            */      STATE_RENDER(WINED3DRS_COLORWRITEENABLE),           state_unknown       },
    { /*193, WINED3DRS_BLENDFACTOR                  */      STATE_RENDER(WINED3DRS_ALPHABLENDENABLE),           state_unknown       },
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
