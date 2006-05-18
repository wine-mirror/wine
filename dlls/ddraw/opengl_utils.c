/* Direct3D Common functions
 * Copyright (c) 1998 Lionel ULMER
 *
 * This file contains all MESA common code
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

#include <stdarg.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "wingdi.h"
#include "ddraw.h"
#include "d3d.h"
#include "wine/debug.h"

#include "opengl_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

GLenum convert_D3D_compare_to_GL(D3DCMPFUNC dwRenderState)
{
    switch (dwRenderState) {
        case D3DCMP_NEVER: return GL_NEVER;
	case D3DCMP_LESS: return GL_LESS;
	case D3DCMP_EQUAL: return GL_EQUAL;
	case D3DCMP_LESSEQUAL: return GL_LEQUAL;
	case D3DCMP_GREATER: return GL_GREATER;
	case D3DCMP_NOTEQUAL: return GL_NOTEQUAL;
	case D3DCMP_GREATEREQUAL: return GL_GEQUAL;
	case D3DCMP_ALWAYS: return GL_ALWAYS;
	default: ERR("Unexpected compare type %d !\n", dwRenderState);
    }
    return GL_ALWAYS;
}

GLenum convert_D3D_stencilop_to_GL(D3DSTENCILOP dwRenderState)
{
    switch (dwRenderState) {
        case D3DSTENCILOP_KEEP: return GL_KEEP;
        case D3DSTENCILOP_ZERO: return GL_ZERO;
        case D3DSTENCILOP_REPLACE: return GL_REPLACE;
        case D3DSTENCILOP_INCRSAT: return GL_INCR;
        case D3DSTENCILOP_DECRSAT: return GL_DECR;
        case D3DSTENCILOP_INVERT: return GL_INVERT;
        case D3DSTENCILOP_INCR: WARN("D3DSTENCILOP_INCR not properly handled !\n"); return GL_INCR;
        case D3DSTENCILOP_DECR: WARN("D3DSTENCILOP_DECR not properly handled !\n"); return GL_DECR;
        default: ERR("Unexpected compare type %d !\n", dwRenderState);      
    }
    return GL_KEEP;
}

GLenum convert_D3D_blendop_to_GL(D3DBLEND dwRenderState)
{
    switch ((D3DBLEND) dwRenderState) {
        case D3DBLEND_ZERO: return GL_ZERO;
        case D3DBLEND_ONE: return GL_ONE;
	case D3DBLEND_SRCALPHA: return GL_SRC_ALPHA;
	case D3DBLEND_INVSRCALPHA: return GL_ONE_MINUS_SRC_ALPHA;
	case D3DBLEND_DESTALPHA: return GL_DST_ALPHA;
	case D3DBLEND_INVDESTALPHA: return GL_ONE_MINUS_DST_ALPHA;
	case D3DBLEND_DESTCOLOR: return GL_DST_COLOR;
	case D3DBLEND_INVDESTCOLOR: return GL_ONE_MINUS_DST_COLOR;
	case D3DBLEND_SRCALPHASAT: return GL_SRC_ALPHA_SATURATE;
	case D3DBLEND_SRCCOLOR: return GL_SRC_COLOR;
	case D3DBLEND_INVSRCCOLOR: return GL_ONE_MINUS_SRC_COLOR;
        default: ERR("Unhandled blend mode %d !\n", dwRenderState); return GL_ZERO;
    }
}

void set_render_state(IDirect3DDeviceImpl* This,
		      D3DRENDERSTATETYPE dwRenderStateType, STATEBLOCK *lpStateBlock)
{
    DWORD dwRenderState = lpStateBlock->render_state[dwRenderStateType - 1];
    IDirect3DDeviceGLImpl *glThis = (IDirect3DDeviceGLImpl *) This;
    
    TRACE("%s = %08lx\n", _get_renderstate(dwRenderStateType), dwRenderState);

    /* First, all the stipple patterns */
    if ((dwRenderStateType >= D3DRENDERSTATE_STIPPLEPATTERN00) &&
	(dwRenderStateType <= D3DRENDERSTATE_STIPPLEPATTERN31)) {
        ERR("Unhandled dwRenderStateType stipple %d!\n",dwRenderStateType);
    } else {
        ENTER_GL();

	/* All others state variables */
	switch (dwRenderStateType) {
	    case D3DRENDERSTATE_TEXTUREHANDLE: {    /*  1 */
	        IDirectDrawSurfaceImpl *tex = (IDirectDrawSurfaceImpl*) dwRenderState;
		
		IDirect3DDevice7_SetTexture(ICOM_INTERFACE(This, IDirect3DDevice7),
					    0, 
					    ICOM_INTERFACE(tex, IDirectDrawSurface7));
	    } break;

	    case D3DRENDERSTATE_ANTIALIAS:        /*  2 */
	        if (dwRenderState)
		    ERR("D3DRENDERSTATE_ANTIALIAS not supported yet !\n");
	        break;
	      
	    case D3DRENDERSTATE_TEXTUREADDRESSU:  /* 44 */
	    case D3DRENDERSTATE_TEXTUREADDRESSV:  /* 45 */
	    case D3DRENDERSTATE_TEXTUREADDRESS: { /*  3 */
	        D3DTEXTURESTAGESTATETYPE d3dTexStageStateType;

		if (dwRenderStateType == D3DRENDERSTATE_TEXTUREADDRESS) d3dTexStageStateType = D3DTSS_ADDRESS;
		else if (dwRenderStateType == D3DRENDERSTATE_TEXTUREADDRESSU) d3dTexStageStateType = D3DTSS_ADDRESSU;
		else d3dTexStageStateType = D3DTSS_ADDRESSV;

		IDirect3DDevice7_SetTextureStageState(ICOM_INTERFACE(This, IDirect3DDevice7),
						      0, d3dTexStageStateType,
						      dwRenderState);
	    } break;
	      
	    case D3DRENDERSTATE_TEXTUREPERSPECTIVE: /* 4 */
	        if (dwRenderState)
		    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
		else
		    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
	        break;

	    case D3DRENDERSTATE_WRAPU: /* 5 */
	    case D3DRENDERSTATE_WRAPV: /* 6 */
	    case D3DRENDERSTATE_WRAP0: /* 128 */
	    case D3DRENDERSTATE_WRAP1: /* 129 */
	    case D3DRENDERSTATE_WRAP2: /* 130 */
	    case D3DRENDERSTATE_WRAP3: /* 131 */
	    case D3DRENDERSTATE_WRAP4: /* 132 */
	    case D3DRENDERSTATE_WRAP5: /* 133 */
	    case D3DRENDERSTATE_WRAP6: /* 134 */
	    case D3DRENDERSTATE_WRAP7: /* 135 */
	        if (dwRenderState)
		    ERR("Texture WRAP modes unsupported by OpenGL.. Expect graphical glitches !\n");
	        break;

	    case D3DRENDERSTATE_ZENABLE:          /*  7 */
	        /* To investigate : in OpenGL, if we disable the depth test, the Z buffer will NOT be
		   updated either.. No idea about what happens in D3D.
		   
		   Maybe replacing the Z function by ALWAYS would be a better idea. */
	        if (dwRenderState == D3DZB_TRUE) {
		    if (glThis->depth_test == FALSE) {
			glEnable(GL_DEPTH_TEST);
			glThis->depth_test = TRUE;
		    }
		} else if (dwRenderState == D3DZB_FALSE) {
		    if (glThis->depth_test) {
			glDisable(GL_DEPTH_TEST);
			glThis->depth_test = FALSE;
		    }
		} else {
		    if (glThis->depth_test == FALSE) {
			glEnable(GL_DEPTH_TEST);
			glThis->depth_test = TRUE;
		    }
		    WARN(" w-buffering not supported.\n");
		}
	        break;

	    case D3DRENDERSTATE_FILLMODE:           /*  8 */
	        switch ((D3DFILLMODE) dwRenderState) {
		    case D3DFILL_POINT:
		        glPolygonMode(GL_FRONT_AND_BACK,GL_POINT);        
			break;
		    case D3DFILL_WIREFRAME:
			glPolygonMode(GL_FRONT_AND_BACK,GL_LINE); 
			break;
		    case D3DFILL_SOLID:
			glPolygonMode(GL_FRONT_AND_BACK,GL_FILL); 
			break;
		    default:
			ERR("Unhandled fill mode %ld !\n",dwRenderState);
		 }
	         break;

	    case D3DRENDERSTATE_SHADEMODE:          /*  9 */
	        switch ((D3DSHADEMODE) dwRenderState) {
		    case D3DSHADE_FLAT:
		        glShadeModel(GL_FLAT);
			break;
		    case D3DSHADE_GOURAUD:
			glShadeModel(GL_SMOOTH);
			break;
		    default:
			ERR("Unhandled shade mode %ld !\n",dwRenderState);
		}
	        break;

	    case D3DRENDERSTATE_ZWRITEENABLE:     /* 14 */
	        if ((dwRenderState != FALSE) && (glThis->depth_mask == FALSE))
		    glDepthMask(GL_TRUE);
		else if ((dwRenderState == FALSE) && (glThis->depth_mask != FALSE))
		    glDepthMask(GL_FALSE);
	        glThis->depth_mask = dwRenderState;
	        break;
	      
	    case D3DRENDERSTATE_ALPHATESTENABLE:  /* 15 */
	        if ((dwRenderState != 0) && (glThis->alpha_test == FALSE))
		    glEnable(GL_ALPHA_TEST);
	        else if ((dwRenderState == 0) && (glThis->alpha_test != FALSE))
		    glDisable(GL_ALPHA_TEST);
		glThis->alpha_test = dwRenderState;
	        break;

	    case D3DRENDERSTATE_TEXTUREMAG: {     /* 17 */
	        DWORD tex_mag = 0xFFFFFFFF;

	        switch ((D3DTEXTUREFILTER) dwRenderState) {
		    case D3DFILTER_NEAREST:
		        tex_mag = D3DTFG_POINT;
			break;
		    case D3DFILTER_LINEAR:
		        tex_mag = D3DTFG_LINEAR;
			break;
		    default:
			ERR("Unhandled texture mag %ld !\n",dwRenderState);
	        }

		if (tex_mag != 0xFFFFFFFF) {
		    IDirect3DDevice7_SetTextureStageState(ICOM_INTERFACE(This, IDirect3DDevice7), 0, D3DTSS_MAGFILTER, tex_mag);
		}
	    } break;

	    case D3DRENDERSTATE_TEXTUREMIN: {       /* 18 */
	        DWORD tex_min = 0xFFFFFFFF;

	        switch ((D3DTEXTUREFILTER) dwRenderState) {
		    case D3DFILTER_NEAREST:
		        tex_min = D3DTFN_POINT;
			break;
		    case D3DFILTER_LINEAR:
		        tex_min = D3DTFN_LINEAR;
			break;
		    default:
			ERR("Unhandled texture min %ld !\n",dwRenderState);
	        }

		if (tex_min != 0xFFFFFFFF) {
		    IDirect3DDevice7_SetTextureStageState(ICOM_INTERFACE(This, IDirect3DDevice7), 0, D3DTSS_MINFILTER, tex_min);
		}
	    } break;

	    case D3DRENDERSTATE_SRCBLEND:           /* 19 */
	    case D3DRENDERSTATE_DESTBLEND:          /* 20 */
	        glBlendFunc(convert_D3D_blendop_to_GL(lpStateBlock->render_state[D3DRENDERSTATE_SRCBLEND - 1]),
			    convert_D3D_blendop_to_GL(lpStateBlock->render_state[D3DRENDERSTATE_DESTBLEND - 1]));
	        break;

	    case D3DRENDERSTATE_TEXTUREMAPBLEND: {  /* 21 */
		IDirect3DDevice7 *d3ddev = ICOM_INTERFACE(This, IDirect3DDevice7);
		
	        switch ((D3DTEXTUREBLEND) dwRenderState) {
		    case D3DTBLEND_DECAL:
		        if (glThis->current_tex_env != GL_REPLACE) {
			    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			    glThis->current_tex_env = GL_REPLACE;
			}
			break;
		    case D3DTBLEND_DECALALPHA:
			if (glThis->current_tex_env != GL_REPLACE) {
			    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
			    glThis->current_tex_env = GL_DECAL;
			}
			break;
		    case D3DTBLEND_MODULATE:
			if (glThis->current_tex_env != GL_MODULATE) {
			    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			    glThis->current_tex_env = GL_MODULATE;
			}
			break;
		    case D3DTBLEND_MODULATEALPHA:
			IDirect3DDevice7_SetTextureStageState(d3ddev, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
			IDirect3DDevice7_SetTextureStageState(d3ddev, 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
			IDirect3DDevice7_SetTextureStageState(d3ddev, 0, D3DTSS_COLORARG2, D3DTA_CURRENT);
			IDirect3DDevice7_SetTextureStageState(d3ddev, 0, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
			IDirect3DDevice7_SetTextureStageState(d3ddev, 0, D3DTSS_COLOROP, D3DTOP_MODULATE);
			IDirect3DDevice7_SetTextureStageState(d3ddev, 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
			break;
		    default:
		        ERR("Unhandled texture environment %ld !\n",dwRenderState);
		}
	    } break;

	    case D3DRENDERSTATE_CULLMODE:           /* 22 */
	        switch ((D3DCULL) dwRenderState) {
		    case D3DCULL_NONE:
		         if (glThis->cull_face != 0) {
			     glDisable(GL_CULL_FACE);
			     glThis->cull_face = 0;
			 }
			 break;
		    case D3DCULL_CW:
			 if (glThis->cull_face == 0) {
			     glEnable(GL_CULL_FACE);
			     glThis->cull_face = 1;
			 }
			 glFrontFace(GL_CCW);
			 glCullFace(GL_BACK);
			 break;
		    case D3DCULL_CCW:
			 if (glThis->cull_face == 0) {
			     glEnable(GL_CULL_FACE);
			     glThis->cull_face = 1;
			 }
			 glFrontFace(GL_CW);
			 glCullFace(GL_BACK);
			 break;
		    default:
			 ERR("Unhandled cull mode %ld !\n",dwRenderState);
		}
	        break;

	    case D3DRENDERSTATE_ZFUNC:            /* 23 */
	        glDepthFunc(convert_D3D_compare_to_GL(dwRenderState));
	        break;
	      
	    case D3DRENDERSTATE_ALPHAREF:    /* 24 */
	    case D3DRENDERSTATE_ALPHAFUNC: { /* 25 */
		    GLenum func = convert_D3D_compare_to_GL(lpStateBlock->render_state[D3DRENDERSTATE_ALPHAFUNC - 1]);
		    GLclampf ref = (lpStateBlock->render_state[D3DRENDERSTATE_ALPHAREF - 1] & 0x000000FF) / 255.0;

		    if ((func != glThis->current_alpha_test_func) || (ref != glThis->current_alpha_test_ref)) {
			glAlphaFunc(func, ref);
			glThis->current_alpha_test_func = func;
			glThis->current_alpha_test_ref = ref;
		    }
	        }
	        break;

	    case D3DRENDERSTATE_DITHERENABLE:     /* 26 */
	        if (dwRenderState)
		    glEnable(GL_DITHER);
		else
		    glDisable(GL_DITHER);
	        break;

	    case D3DRENDERSTATE_ALPHABLENDENABLE:   /* 27 */
	        if ((dwRenderState != 0) && (glThis->blending == 0)) {
		    glEnable(GL_BLEND);
		} else if ((dwRenderState == 0) && (glThis->blending != 0)) {
		    glDisable(GL_BLEND);
		}
	        glThis->blending = dwRenderState;

	        /* Hack for some old games ... */
	        if (glThis->parent.version == 1) {
		    lpStateBlock->render_state[D3DRENDERSTATE_COLORKEYENABLE - 1] = dwRenderState;
		}
	        break;
	      
	    case D3DRENDERSTATE_FOGENABLE: /* 28 */
	        /* Nothing to do here. Only the storage matters :-) */
	        break;

	    case D3DRENDERSTATE_SPECULARENABLE: /* 29 */
	        if (dwRenderState)
		    ERR(" Specular Lighting not supported yet.\n");
	        break;
	      
	    case D3DRENDERSTATE_SUBPIXEL:  /* 31 */
	    case D3DRENDERSTATE_SUBPIXELX: /* 32 */
	        /* We do not support this anyway, so why protest :-) */
	        break; 

 	    case D3DRENDERSTATE_STIPPLEDALPHA: /* 33 */
	        if (dwRenderState)
		    ERR(" Stippled Alpha not supported yet.\n");
		break;

	    case D3DRENDERSTATE_FOGCOLOR: { /* 34 */
	        GLfloat color[4];
		color[0] = ((dwRenderState >> 16) & 0xFF)/255.0f;
		color[1] = ((dwRenderState >>  8) & 0xFF)/255.0f;
		color[2] = ((dwRenderState >>  0) & 0xFF)/255.0f;
		color[3] = ((dwRenderState >> 24) & 0xFF)/255.0f;
		glFogfv(GL_FOG_COLOR,color);
		/* Note: glFogiv does not seem to work */
	    } break;

 	    case D3DRENDERSTATE_FOGTABLEMODE:  /* 35 */
 	    case D3DRENDERSTATE_FOGVERTEXMODE: /* 140 */
 	    case D3DRENDERSTATE_FOGSTART:      /* 36 */
	    case D3DRENDERSTATE_FOGEND:        /* 37 */
	        /* Nothing to do here. Only the storage matters :-) */
		break;

	    case D3DRENDERSTATE_FOGDENSITY:    /* 38 */
		glFogi(GL_FOG_DENSITY,*(float*)&dwRenderState);
		break;

	    case D3DRENDERSTATE_COLORKEYENABLE:     /* 41 */
	        /* Nothing done here, only storage matters. */
	        break;

	    case D3DRENDERSTATE_MIPMAPLODBIAS: /* 46 */
	        IDirect3DDevice7_SetTextureStageState(ICOM_INTERFACE(This, IDirect3DDevice7),
						      0, D3DTSS_MIPMAPLODBIAS,
						      dwRenderState);
	        break;
	      
	    case D3DRENDERSTATE_ZBIAS: /* 47 */
	        /* This is a tad bit hacky.. But well, no idea how to do it better in OpenGL :-/ */
	        if (dwRenderState == 0) {
		    glDisable(GL_POLYGON_OFFSET_FILL);
		    glDisable(GL_POLYGON_OFFSET_LINE);
		    glDisable(GL_POLYGON_OFFSET_POINT);
		} else {
		    glEnable(GL_POLYGON_OFFSET_FILL);
		    glEnable(GL_POLYGON_OFFSET_LINE);
		    glEnable(GL_POLYGON_OFFSET_POINT);
		    glPolygonOffset(1.0, dwRenderState * 1.0);
		}
	        break;
	      
	    case D3DRENDERSTATE_FLUSHBATCH:         /* 50 */
	        break;

	    case D3DRENDERSTATE_STENCILENABLE:    /* 52 */
	        if ((dwRenderState != 0) && (glThis->stencil_test == 0))
		    glEnable(GL_STENCIL_TEST);
		else if ((dwRenderState == 0) && (glThis->stencil_test != 0))
		    glDisable(GL_STENCIL_TEST);
	        glThis->stencil_test = dwRenderState;
		break;
	    
	    case D3DRENDERSTATE_STENCILFAIL:      /* 53 */
	    case D3DRENDERSTATE_STENCILZFAIL:     /* 54 */
	    case D3DRENDERSTATE_STENCILPASS:      /* 55 */
		glStencilOp(convert_D3D_stencilop_to_GL(lpStateBlock->render_state[D3DRENDERSTATE_STENCILFAIL - 1]),
			    convert_D3D_stencilop_to_GL(lpStateBlock->render_state[D3DRENDERSTATE_STENCILZFAIL - 1]),
			    convert_D3D_stencilop_to_GL(lpStateBlock->render_state[D3DRENDERSTATE_STENCILPASS - 1]));
		break;

	    case D3DRENDERSTATE_STENCILFUNC:      /* 56 */
	    case D3DRENDERSTATE_STENCILREF:       /* 57 */
	    case D3DRENDERSTATE_STENCILMASK:      /* 58 */
		glStencilFunc(convert_D3D_compare_to_GL(lpStateBlock->render_state[D3DRENDERSTATE_STENCILFUNC - 1]),
			      lpStateBlock->render_state[D3DRENDERSTATE_STENCILREF - 1],
			      lpStateBlock->render_state[D3DRENDERSTATE_STENCILMASK - 1]);
		break;
	  
	    case D3DRENDERSTATE_STENCILWRITEMASK: /* 59 */
	        glStencilMask(dwRenderState);
	        break;

	    case D3DRENDERSTATE_TEXTUREFACTOR:      /* 60 */
	        /* Only the storage matters... */
	        break;

	    case D3DRENDERSTATE_CLIPPING:          /* 136 */
	    case D3DRENDERSTATE_CLIPPLANEENABLE: { /* 152 */
		    GLint i;
		    DWORD mask, runner;
		    
		    if (dwRenderStateType == D3DRENDERSTATE_CLIPPING) {
			mask = ((dwRenderState) ?
				(This->state_block.render_state[D3DRENDERSTATE_CLIPPLANEENABLE - 1]) : (0x00000000));
		    } else {
			mask = dwRenderState;
		    }
		    for (i = 0, runner = 0x00000001; i < This->max_clipping_planes; i++, runner = (runner << 1)) {
			if (mask & runner) {
			    GLint enabled;
			    glGetIntegerv(GL_CLIP_PLANE0 + i, &enabled);
			    if (enabled == GL_FALSE) {
			        glEnable(GL_CLIP_PLANE0 + i);
				/* Need to force a transform change so that this clipping plane parameters are sent
				 * properly to GL.
				 */
				glThis->transform_state = GL_TRANSFORM_NONE;
			    }
			} else {
			    glDisable(GL_CLIP_PLANE0 + i);
			}
		    }
		}
	        break;

	    case D3DRENDERSTATE_LIGHTING:    /* 137 */
	        /* Nothing to do, only storage matters... */
	        break;
		
	    case D3DRENDERSTATE_AMBIENT: {            /* 139 */
	        float light[4];

		light[0] = ((dwRenderState >> 16) & 0xFF) / 255.0;
		light[1] = ((dwRenderState >>  8) & 0xFF) / 255.0;
		light[2] = ((dwRenderState >>  0) & 0xFF) / 255.0;
		light[3] = ((dwRenderState >> 24) & 0xFF) / 255.0;
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, (float *) light);
	    } break;

	    case D3DRENDERSTATE_COLORVERTEX:          /* 141 */
	          /* Nothing to do here.. Only storage matters */
	          break;
		  
	    case D3DRENDERSTATE_LOCALVIEWER:          /* 142 */
	        if (dwRenderState)
		    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
		else
		    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);
		break;

	    case D3DRENDERSTATE_NORMALIZENORMALS:     /* 143 */
	        if (dwRenderState) {
		    glEnable(GL_NORMALIZE);
		    glEnable(GL_RESCALE_NORMAL);
		} else {
		    glDisable(GL_NORMALIZE);
		    glDisable(GL_RESCALE_NORMAL);
		}
		break;

	    case D3DRENDERSTATE_DIFFUSEMATERIALSOURCE:    /* 145 */
	    case D3DRENDERSTATE_SPECULARMATERIALSOURCE:   /* 146 */
	    case D3DRENDERSTATE_AMBIENTMATERIALSOURCE:    /* 147 */
	    case D3DRENDERSTATE_EMISSIVEMATERIALSOURCE:   /* 148 */
	        /* Nothing to do here. Only the storage matters :-) */
		break;

	    default:
	        ERR("Unhandled dwRenderStateType %s (%08x) value : %08lx !\n",
		    _get_renderstate(dwRenderStateType), dwRenderStateType, dwRenderState);
	}
	LEAVE_GL();
    }
}

void store_render_state(IDirect3DDeviceImpl *This,
			D3DRENDERSTATETYPE dwRenderStateType, DWORD dwRenderState, STATEBLOCK *lpStateBlock)
{
    TRACE("%s = %08lx\n", _get_renderstate(dwRenderStateType), dwRenderState);
    
    /* Some special cases first.. */
    if (dwRenderStateType == D3DRENDERSTATE_SRCBLEND) {
        if (dwRenderState == D3DBLEND_BOTHSRCALPHA) {
	    lpStateBlock->render_state[D3DRENDERSTATE_SRCBLEND - 1] = D3DBLEND_SRCALPHA;
	    lpStateBlock->render_state[D3DRENDERSTATE_DESTBLEND - 1] = D3DBLEND_INVSRCALPHA;
	    return;
	} else if (dwRenderState == D3DBLEND_BOTHINVSRCALPHA) {
	    lpStateBlock->render_state[D3DRENDERSTATE_SRCBLEND - 1] = D3DBLEND_INVSRCALPHA;
	    lpStateBlock->render_state[D3DRENDERSTATE_DESTBLEND - 1] = D3DBLEND_SRCALPHA;
	    return;
	}
    } else if (dwRenderStateType == D3DRENDERSTATE_TEXTUREADDRESS) {
        lpStateBlock->render_state[D3DRENDERSTATE_TEXTUREADDRESSU - 1] = dwRenderState;
        lpStateBlock->render_state[D3DRENDERSTATE_TEXTUREADDRESSV - 1] = dwRenderState;
    } else if (dwRenderStateType == D3DRENDERSTATE_WRAPU) {
        if (dwRenderState) 
	    lpStateBlock->render_state[D3DRENDERSTATE_WRAP0] |= D3DWRAP_U;
	else
	    lpStateBlock->render_state[D3DRENDERSTATE_WRAP0] &= ~D3DWRAP_U;
    } else if (dwRenderStateType == D3DRENDERSTATE_WRAPV) {
        if (dwRenderState) 
	    lpStateBlock->render_state[D3DRENDERSTATE_WRAP0] |= D3DWRAP_V;
	else
	    lpStateBlock->render_state[D3DRENDERSTATE_WRAP0] &= ~D3DWRAP_V;
    }
    
    /* Default case */
    lpStateBlock->render_state[dwRenderStateType - 1] = dwRenderState;
}

void get_render_state(IDirect3DDeviceImpl *This,
		      D3DRENDERSTATETYPE dwRenderStateType, LPDWORD lpdwRenderState, STATEBLOCK *lpStateBlock)
{
    *lpdwRenderState = lpStateBlock->render_state[dwRenderStateType - 1];
    if (TRACE_ON(ddraw))
        TRACE("%s = %08lx\n", _get_renderstate(dwRenderStateType), *lpdwRenderState);
}

void apply_render_state(IDirect3DDeviceImpl *This, STATEBLOCK *lpStateBlock)
{
    DWORD i;
    TRACE("(%p,%p)\n", This, lpStateBlock);
    for(i = 0; i < HIGHEST_RENDER_STATE; i++)
	if (lpStateBlock->set_flags.render_state[i])
            set_render_state(This, i + 1, lpStateBlock);
}


/* Texture management code.

    - upload_surface_to_tex_memory_init initialize the code and computes the GL formats 
      according to the surface description.

    - upload_surface_to_tex_memory does the real upload. If one buffer is split over
      multiple textures, this can be called multiple times after the '_init' call. 'rect'
      can be NULL if the whole buffer needs to be upload.

    - upload_surface_to_tex_memory_release does the clean-up.

   These functions are called in the following cases :
    - texture management (ie to upload a D3D texture to GL when it changes).
    - flush of the 'in-memory' frame buffer to the GL frame buffer using the texture
      engine.
    - use of the texture engine to simulate Blits to the 3D Device.
*/
typedef enum {
    NO_CONVERSION,
    CONVERT_PALETTED,
    CONVERT_CK_565,
    CONVERT_CK_5551,
    CONVERT_CK_4444,
    CONVERT_CK_4444_ARGB,
    CONVERT_CK_1555,
    CONVERT_555,
    CONVERT_CK_RGB24,
    CONVERT_CK_8888,
    CONVERT_CK_8888_ARGB,
    CONVERT_RGB32_888
} CONVERT_TYPES;

/* Note : we suppose that all the code calling this is protected by the GL lock... Otherwise bad things
   may happen :-) */
static GLenum current_format;
static GLenum current_pixel_format;
static CONVERT_TYPES convert_type;
static IDirectDrawSurfaceImpl *current_surface;
static GLuint current_level;
static DWORD current_tex_width;
static DWORD current_tex_height;
static GLuint current_alignement_constraints;
static int current_storage_width;

HRESULT upload_surface_to_tex_memory_init(IDirectDrawSurfaceImpl *surf_ptr, GLuint level, GLenum *current_internal_format,
					  BOOLEAN need_to_alloc, BOOLEAN need_alpha_ck, DWORD tex_width, DWORD tex_height)
{
    const DDPIXELFORMAT * const src_pf = &(surf_ptr->surface_desc.u4.ddpfPixelFormat);
    BOOL error = FALSE;
    BOOL colorkey_active = need_alpha_ck && (surf_ptr->surface_desc.dwFlags & DDSD_CKSRCBLT);
    GLenum internal_format = GL_LUMINANCE; /* A bogus value to be sure to have a nice Mesa warning :-) */
    BYTE bpp = GET_BPP(surf_ptr->surface_desc);
    BOOL sub_texture = TRUE;

    current_surface = surf_ptr;
    current_level = level;

    if (src_pf->dwFlags & DDPF_FOURCC) {
	GLenum retVal;
	int size = surf_ptr->surface_desc.u1.dwLinearSize;
	int width = surf_ptr->surface_desc.dwWidth;
	int height = surf_ptr->surface_desc.dwHeight;
	LPVOID buffer = surf_ptr->surface_desc.lpSurface;

	switch (src_pf->dwFourCC) {
	    case MAKE_FOURCC('D','X','T','1'): retVal = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT; break;
	    case MAKE_FOURCC('D','X','T','3'): retVal = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; break;
	    case MAKE_FOURCC('D','X','T','5'): retVal = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; break;
	    default:
		FIXME("FourCC Not supported\n");
		return DD_OK;
	}

	if (GL_extensions.s3tc_compressed_texture) {
	    GL_extensions.glCompressedTexImage2D(GL_TEXTURE_2D, current_level, retVal, width, height, 0, size, buffer);
	} else
	    ERR("Trying to upload S3TC texture whereas the device does not have support for it\n");

	return DD_OK;
    }

    /* First, do some sanity checks ... */
    if ((surf_ptr->surface_desc.u1.lPitch % bpp) != 0) {
	FIXME("Warning : pitch is not a multiple of BPP - not supported yet !\n");
    } else {
	/* In that case, no need to have any alignement constraints... */
	if (current_alignement_constraints != 1) {
	    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	    current_alignement_constraints = 1;
	}
    }

    /* Note: we only check width here as you cannot have width non-zero while height is set to zero */
    if (tex_width == 0) {
	sub_texture = FALSE;
	
	tex_width = surf_ptr->surface_desc.dwWidth;
	tex_height = surf_ptr->surface_desc.dwHeight;
    }

    current_tex_width = tex_width;
    current_tex_height = tex_height;

    if (src_pf->dwFlags & DDPF_PALETTEINDEXED8) {
	/* ****************
	   Paletted Texture
	   **************** */
	current_format = GL_RGBA;
	internal_format = GL_RGBA;
	current_pixel_format = GL_UNSIGNED_BYTE;
	convert_type = CONVERT_PALETTED;
    } else if (src_pf->dwFlags & DDPF_RGB) {
	/* ************
	   RGB Textures
	   ************ */
	if (src_pf->u1.dwRGBBitCount == 8) {
	    if ((src_pf->dwFlags & DDPF_ALPHAPIXELS) &&
		(src_pf->u5.dwRGBAlphaBitMask != 0x00)) {
		error = TRUE;
	    } else {
		if ((src_pf->u2.dwRBitMask == 0xE0) &&
		    (src_pf->u3.dwGBitMask == 0x1C) &&
		    (src_pf->u4.dwBBitMask == 0x03)) {
		    /* **********************
		       GL_UNSIGNED_BYTE_3_3_2
		       ********************** */
		    if (colorkey_active) {
			/* This texture format will never be used.. So do not care about color keying
			   up until the point in time it will be needed :-) */
			FIXME(" ColorKeying not supported in the RGB 332 format !\n");
		    }
		    current_format = GL_RGB;
		    internal_format = GL_RGB;
		    current_pixel_format = GL_UNSIGNED_BYTE_3_3_2;
		    convert_type = NO_CONVERSION;
		} else {
		    error = TRUE;
		}
	    }
	} else if (src_pf->u1.dwRGBBitCount == 16) {
	    if ((src_pf->dwFlags & DDPF_ALPHAPIXELS) &&
		(src_pf->u5.dwRGBAlphaBitMask != 0x0000)) {
		if ((src_pf->u2.dwRBitMask ==        0xF800) &&
		    (src_pf->u3.dwGBitMask ==        0x07C0) &&
		    (src_pf->u4.dwBBitMask ==        0x003E) &&
		    (src_pf->u5.dwRGBAlphaBitMask == 0x0001)) {
		    current_format = GL_RGBA;
		    internal_format = GL_RGBA;
		    current_pixel_format = GL_UNSIGNED_SHORT_5_5_5_1;
		    if (colorkey_active) {
			convert_type = CONVERT_CK_5551;
		    } else {
			convert_type = NO_CONVERSION;
		    }
		} else if ((src_pf->u2.dwRBitMask ==        0xF000) &&
			   (src_pf->u3.dwGBitMask ==        0x0F00) &&
			   (src_pf->u4.dwBBitMask ==        0x00F0) &&
			   (src_pf->u5.dwRGBAlphaBitMask == 0x000F)) {
		    current_format = GL_RGBA;
		    internal_format = GL_RGBA;
		    current_pixel_format = GL_UNSIGNED_SHORT_4_4_4_4;
		    if (colorkey_active) {
			convert_type = CONVERT_CK_4444;
		    } else {
			convert_type = NO_CONVERSION;
		    }
		} else if ((src_pf->u2.dwRBitMask ==        0x0F00) &&
			   (src_pf->u3.dwGBitMask ==        0x00F0) &&
			   (src_pf->u4.dwBBitMask ==        0x000F) &&
			   (src_pf->u5.dwRGBAlphaBitMask == 0xF000)) {
		    if (colorkey_active) {
			convert_type = CONVERT_CK_4444_ARGB;
			current_format = GL_RGBA;
			internal_format = GL_RGBA;
			current_pixel_format = GL_UNSIGNED_SHORT_4_4_4_4;
		    } else {
			convert_type = NO_CONVERSION;
			current_format = GL_BGRA;
			internal_format = GL_RGBA;
			current_pixel_format = GL_UNSIGNED_SHORT_4_4_4_4_REV;
		    }
		} else if ((src_pf->u2.dwRBitMask ==        0x7C00) &&
			   (src_pf->u3.dwGBitMask ==        0x03E0) &&
			   (src_pf->u4.dwBBitMask ==        0x001F) &&
			   (src_pf->u5.dwRGBAlphaBitMask == 0x8000)) {
		    if (colorkey_active) {
			convert_type = CONVERT_CK_1555;
			current_format = GL_RGBA;
			internal_format = GL_RGBA;
			current_pixel_format = GL_UNSIGNED_SHORT_5_5_5_1;
		    } else {
			convert_type = NO_CONVERSION;
			current_format = GL_BGRA;
			internal_format = GL_RGBA;
			current_pixel_format = GL_UNSIGNED_SHORT_1_5_5_5_REV;
		    }
		} else {
		    error = TRUE;
		}
	    } else {
		if ((src_pf->u2.dwRBitMask == 0xF800) &&
		    (src_pf->u3.dwGBitMask == 0x07E0) &&
		    (src_pf->u4.dwBBitMask == 0x001F)) {
		    if (colorkey_active) {
			convert_type = CONVERT_CK_565;
			current_format = GL_RGBA;
			internal_format = GL_RGBA;
			current_pixel_format = GL_UNSIGNED_SHORT_5_5_5_1;
		    } else {
			convert_type = NO_CONVERSION;
			current_format = GL_RGB;
			internal_format = GL_RGB;
			current_pixel_format = GL_UNSIGNED_SHORT_5_6_5;
		    }
		} else if ((src_pf->u2.dwRBitMask == 0x7C00) &&
			   (src_pf->u3.dwGBitMask == 0x03E0) &&
			   (src_pf->u4.dwBBitMask == 0x001F)) {
		    convert_type = CONVERT_555;
		    current_format = GL_RGBA;
		    internal_format = GL_RGBA;
		    current_pixel_format = GL_UNSIGNED_SHORT_5_5_5_1;
		} else {
		    error = TRUE;
		}
	    }
	} else if (src_pf->u1.dwRGBBitCount == 24) {
	    if ((src_pf->dwFlags & DDPF_ALPHAPIXELS) &&
		(src_pf->u5.dwRGBAlphaBitMask != 0x000000)) {
		error = TRUE;
	    } else {
		if ((src_pf->u2.dwRBitMask == 0xFF0000) &&
		    (src_pf->u3.dwGBitMask == 0x00FF00) &&
		    (src_pf->u4.dwBBitMask == 0x0000FF)) {
		    if (colorkey_active) {
			convert_type = CONVERT_CK_RGB24;
			current_format = GL_RGBA;
			internal_format = GL_RGBA;
			current_pixel_format = GL_UNSIGNED_INT_8_8_8_8;
		    } else {
			convert_type = NO_CONVERSION;
			current_format = GL_BGR;
			internal_format = GL_RGB;
			current_pixel_format = GL_UNSIGNED_BYTE;
		    }
		} else {
		    error = TRUE;
		}
	    }
	} else if (src_pf->u1.dwRGBBitCount == 32) {
	    if ((src_pf->dwFlags & DDPF_ALPHAPIXELS) &&
		(src_pf->u5.dwRGBAlphaBitMask != 0x00000000)) {
		if ((src_pf->u2.dwRBitMask ==        0xFF000000) &&
		    (src_pf->u3.dwGBitMask ==        0x00FF0000) &&
		    (src_pf->u4.dwBBitMask ==        0x0000FF00) &&
		    (src_pf->u5.dwRGBAlphaBitMask == 0x000000FF)) {
		    if (colorkey_active) {
			convert_type = CONVERT_CK_8888;
		    } else {
			convert_type = NO_CONVERSION;
		    }
		    current_format = GL_RGBA;
		    internal_format = GL_RGBA;
		    current_pixel_format = GL_UNSIGNED_INT_8_8_8_8;
		} else if ((src_pf->u2.dwRBitMask ==        0x00FF0000) &&
			   (src_pf->u3.dwGBitMask ==        0x0000FF00) &&
			   (src_pf->u4.dwBBitMask ==        0x000000FF) &&
			   (src_pf->u5.dwRGBAlphaBitMask == 0xFF000000)) {
		    if (colorkey_active) {
			convert_type = CONVERT_CK_8888_ARGB;
			current_format = GL_RGBA;
			internal_format = GL_RGBA;
			current_pixel_format = GL_UNSIGNED_INT_8_8_8_8;
		    } else {
			convert_type = NO_CONVERSION;
			current_format = GL_BGRA;
			internal_format = GL_RGBA;
			current_pixel_format = GL_UNSIGNED_INT_8_8_8_8_REV;
		    }
		} else {
		    error = TRUE;
		}
	    } else {
		if ((src_pf->u2.dwRBitMask == 0x00FF0000) &&
		    (src_pf->u3.dwGBitMask == 0x0000FF00) &&
		    (src_pf->u4.dwBBitMask == 0x000000FF)) {
		    if (need_alpha_ck) {
			convert_type = CONVERT_RGB32_888;
			current_format = GL_RGBA;
			internal_format = GL_RGBA;
			current_pixel_format = GL_UNSIGNED_INT_8_8_8_8;
		    } else {
			convert_type = NO_CONVERSION;
			current_format = GL_BGRA;
			internal_format = GL_RGBA;
			current_pixel_format = GL_UNSIGNED_INT_8_8_8_8_REV;
		    }
		} else {
		    error = TRUE;
		}
	    }
	} else {
	    error = TRUE;
	}
    } else {
	error = TRUE;
    } 

    if (error) {
	ERR("Unsupported pixel format for textures :\n");
	if (ERR_ON(ddraw)) {
	    DDRAW_dump_pixelformat(src_pf);
	}
	return DDERR_INVALIDPIXELFORMAT;
    } else {
	if ((need_to_alloc) ||
	    (internal_format != *current_internal_format)) {
	    glTexImage2D(GL_TEXTURE_2D, level, internal_format,
			 tex_width, tex_height, 0,
			 current_format, current_pixel_format, NULL);
	    *current_internal_format = internal_format;
	}
    }

    if (sub_texture && (convert_type == NO_CONVERSION)) {
	current_storage_width = surf_ptr->surface_desc.u1.lPitch / bpp;
    } else {
	if (surf_ptr->surface_desc.u1.lPitch == (surf_ptr->surface_desc.dwWidth * bpp)) {
	    current_storage_width = 0;
	} else {
	    current_storage_width = surf_ptr->surface_desc.u1.lPitch / bpp;
	}	
    }
    glPixelStorei(GL_UNPACK_ROW_LENGTH, current_storage_width);

    TRACE(" initialized texture upload for level %d with conversion %d.\n", current_level, convert_type);
    
    return DD_OK;
}

HRESULT upload_surface_to_tex_memory(RECT *rect, DWORD xoffset, DWORD yoffset, void **temp_buffer)
{
    const DDSURFACEDESC * const src_d = (DDSURFACEDESC *)&(current_surface->surface_desc);
    void *surf_buffer = NULL;
    RECT lrect;
    DWORD width, height;
    BYTE bpp = GET_BPP(current_surface->surface_desc);
    int line_increase;
    
    if (rect == NULL) {
	lrect.top = 0;
	lrect.left = 0;
	lrect.bottom = current_tex_height;
	lrect.right = current_tex_width;
	rect = &lrect;
    }

    width = rect->right - rect->left;
    height = rect->bottom - rect->top;

    if (current_surface->surface_desc.u4.ddpfPixelFormat.dwFlags & DDPF_FOURCC) {
	GLint retVal;
	int size = current_surface->surface_desc.u1.dwLinearSize;
	int width_ = current_surface->surface_desc.dwWidth;
	int height_ = current_surface->surface_desc.dwHeight;
	LPVOID buffer = current_surface->surface_desc.lpSurface;

	switch (current_surface->surface_desc.u4.ddpfPixelFormat.dwFourCC) {
	    case MAKE_FOURCC('D','X','T','1'): retVal = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT; break;
	    case MAKE_FOURCC('D','X','T','3'): retVal = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; break;
	    case MAKE_FOURCC('D','X','T','5'): retVal = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; break;
	    default:
		FIXME("Not supported\n");
		return DD_OK;
	}

	if (GL_extensions.s3tc_compressed_texture) {
	    /* GL_extensions.glCompressedTexSubImage2D(GL_TEXTURE_2D, current_level, xoffset, yoffset, width, height, retVal, (unsigned char*)temp_buffer); */
	    GL_extensions.glCompressedTexImage2D(GL_TEXTURE_2D, current_level, retVal, width_, height_, 0, size, buffer);
	} else
	    ERR("Trying to upload S3TC texture whereas the device does not have support for it\n");

	return DD_OK;
    }
    
    /* Used when converting stuff */
    line_increase = src_d->u1.lPitch - (width * bpp);

    switch (convert_type) {
        case CONVERT_PALETTED: {
	    IDirectDrawPaletteImpl* pal = current_surface->palette;
	    BYTE table[256][4];
	    unsigned int i;
	    unsigned int x, y;
	    BYTE *src = (BYTE *) (((BYTE *) src_d->lpSurface) + (bpp * rect->left) + (src_d->u1.lPitch * rect->top)), *dst;
	    
	    if (pal == NULL) {
		/* Upload a black texture. The real one will be uploaded on palette change */
		WARN("Palettized texture Loading with a NULL palette !\n");
		memset(table, 0, 256 * 4);
	    } else {
		/* Get the surface's palette */
		for (i = 0; i < 256; i++) {
		    table[i][0] = pal->palents[i].peRed;
		    table[i][1] = pal->palents[i].peGreen;
		    table[i][2] = pal->palents[i].peBlue;
		    if ((src_d->dwFlags & DDSD_CKSRCBLT) &&
			(i >= src_d->ddckCKSrcBlt.dwColorSpaceLowValue) &&
			(i <= src_d->ddckCKSrcBlt.dwColorSpaceHighValue))
			/* We should maybe here put a more 'neutral' color than the standard bright purple
			   one often used by application to prevent the nice purple borders when bi-linear
			   filtering is on */
			table[i][3] = 0x00;
		    else
			table[i][3] = 0xFF;
		}
	    }
	    
	    if (*temp_buffer == NULL)
		*temp_buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 
					 current_tex_width * current_tex_height * sizeof(DWORD));
	    dst = (BYTE *) *temp_buffer;

	    for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
		    BYTE color = *src++;
		    *dst++ = table[color][0];
		    *dst++ = table[color][1];
		    *dst++ = table[color][2];
		    *dst++ = table[color][3];
		}
		src += line_increase;
	    }
	} break;

        case CONVERT_CK_565: {
	    /* Converting the 565 format in 5551 packed to emulate color-keying.
	       
	       Note : in all these conversion, it would be best to average the averaging
	              pixels to get the color of the pixel that will be color-keyed to
		      prevent 'color bleeding'. This will be done later on if ever it is
		      too visible.
		      
	       Note2: when using color-keying + alpha, are the alpha bits part of the
	              color-space or not ?
	    */
	    unsigned int x, y;
	    WORD *src = (WORD *) (((BYTE *) src_d->lpSurface) + (bpp * rect->left) + (src_d->u1.lPitch * rect->top)), *dst;
	    
	    if (*temp_buffer == NULL)
		*temp_buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
					 current_tex_width * current_tex_height * sizeof(WORD));
	    dst = (WORD *) *temp_buffer;
	    
	    for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
		    WORD color = *src++;
		    *dst = ((color & 0xFFC0) | ((color & 0x1F) << 1));
		    if ((color < src_d->ddckCKSrcBlt.dwColorSpaceLowValue) ||
			(color > src_d->ddckCKSrcBlt.dwColorSpaceHighValue))
			*dst |= 0x0001;
		    dst++;
		}
		src = (WORD *) (((BYTE *) src) + line_increase);
	    }
	} break;
	
        case CONVERT_CK_5551: {
	    /* Change the alpha value of the color-keyed pixels to emulate color-keying. */
	    unsigned int x, y;
	    WORD *src = (WORD *) (((BYTE *) src_d->lpSurface) + (bpp * rect->left) + (src_d->u1.lPitch * rect->top)), *dst;
	    
	    if (*temp_buffer == NULL)
		*temp_buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
					 current_tex_width * current_tex_height * sizeof(WORD));
	    dst = (WORD *) *temp_buffer;
	    
	    for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
		    WORD color = *src++;
		    *dst = color & 0xFFFE;
		    if ((color < src_d->ddckCKSrcBlt.dwColorSpaceLowValue) ||
			(color > src_d->ddckCKSrcBlt.dwColorSpaceHighValue))
			*dst |= color & 0x0001;
		    dst++;
		}
		src = (WORD *) (((BYTE *) src) + line_increase);
	    }
	} break;
	
        case CONVERT_CK_4444: {
	    /* Change the alpha value of the color-keyed pixels to emulate color-keying. */
	    unsigned int x, y;
	    WORD *src = (WORD *) (((BYTE *) src_d->lpSurface) + (bpp * rect->left) + (src_d->u1.lPitch * rect->top)), *dst;
	    
	    if (*temp_buffer == NULL)
		*temp_buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
					 current_tex_width * current_tex_height * sizeof(WORD));
	    dst = (WORD *) *temp_buffer;
	    
	    for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
		    WORD color = *src++;
		    *dst = color & 0xFFF0;
		    if ((color < src_d->ddckCKSrcBlt.dwColorSpaceLowValue) ||
			(color > src_d->ddckCKSrcBlt.dwColorSpaceHighValue))
			*dst |= color & 0x000F;
		    dst++;
		}
		src = (WORD *) (((BYTE *) src) + line_increase);
	    }
	} break;
	
        case CONVERT_CK_4444_ARGB: {
	    /* Move the four Alpha bits... */
	    unsigned int x, y;
	    WORD *src = (WORD *) (((BYTE *) src_d->lpSurface) + (bpp * rect->left) + (src_d->u1.lPitch * rect->top)), *dst;
	    
	    if (*temp_buffer == NULL)
		*temp_buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
					 current_tex_width * current_tex_height * sizeof(WORD));
	    dst = (WORD *) *temp_buffer;
	    
	    for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
		    WORD color = *src++;
		    *dst = (color & 0x0FFF) << 4;
		    if ((color < src_d->ddckCKSrcBlt.dwColorSpaceLowValue) ||
			(color > src_d->ddckCKSrcBlt.dwColorSpaceHighValue))
			*dst |= (color & 0xF000) >> 12;
		    dst++;
		}
		src = (WORD *) (((BYTE *) src) + line_increase);
	    }
	} break;
	
        case CONVERT_CK_1555: {
	    unsigned int x, y;
	    WORD *src = (WORD *) (((BYTE *) src_d->lpSurface) + (bpp * rect->left) + (src_d->u1.lPitch * rect->top)), *dst;
	    
	    if (*temp_buffer == NULL)
		*temp_buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
					 current_tex_width * current_tex_height * sizeof(WORD));
	    dst = (WORD *) *temp_buffer;
	    
	    for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
		    WORD color = *src++;
		    *dst = (color & 0x7FFF) << 1;
		    if ((color < src_d->ddckCKSrcBlt.dwColorSpaceLowValue) ||
			(color > src_d->ddckCKSrcBlt.dwColorSpaceHighValue))
			*dst |= (color & 0x8000) >> 15;
		    dst++;
		}
		src = (WORD *) (((BYTE *) src) + line_increase);
	    }
	} break;
	
        case CONVERT_555: {
	    /* Converting the 0555 format in 5551 packed */
	    unsigned int x, y;
	    WORD *src = (WORD *) (((BYTE *) src_d->lpSurface) + (bpp * rect->left) + (src_d->u1.lPitch * rect->top)), *dst;
	    
	    if (*temp_buffer == NULL)
		*temp_buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
					 current_tex_width * current_tex_height * sizeof(WORD));
	    dst = (WORD *) *temp_buffer;
	    
	    if (src_d->dwFlags & DDSD_CKSRCBLT) {
		for (y = 0; y < height; y++) {
		    for (x = 0; x < width; x++) {
			WORD color = *src++;
			*dst = (color & 0x7FFF) << 1;
			if ((color < src_d->ddckCKSrcBlt.dwColorSpaceLowValue) ||
			    (color > src_d->ddckCKSrcBlt.dwColorSpaceHighValue))
			    *dst |= 0x0001;
			dst++;
		    }
		    src = (WORD *) (((BYTE *) src) + line_increase);
		}
	    } else {
		for (y = 0; y < height; y++) {
		    for (x = 0; x < width; x++) {
			WORD color = *src++;
			*dst++ = ((color & 0x7FFF) << 1) | 0x0001;
		    }
		    src = (WORD *) (((BYTE *) src) + line_increase);
		}
	    }
	    
	} break;
	
        case CONVERT_CK_RGB24: {
	    /* This is a pain :-) */
	    unsigned int x, y;
	    BYTE *src = (BYTE *) (((BYTE *) src_d->lpSurface) + (bpp * rect->left) + (src_d->u1.lPitch * rect->top));
	    DWORD *dst;

	    if (*temp_buffer == NULL)
		*temp_buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
					 current_tex_width * current_tex_height * sizeof(DWORD));
	    dst = (DWORD *) *temp_buffer;

	    for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
		    DWORD color = *((DWORD *) src) & 0x00FFFFFF;
		    src += 3;
		    *dst = color << 8;
		    if ((color < src_d->ddckCKSrcBlt.dwColorSpaceLowValue) ||
			(color > src_d->ddckCKSrcBlt.dwColorSpaceHighValue))
			*dst |= 0xFF;
		    dst++;
		}
		src += line_increase;
	    }
	} break;

        case CONVERT_CK_8888: {
	    /* Just use the alpha component to handle color-keying... */
	    unsigned int x, y;
	    DWORD *src = (DWORD *) (((BYTE *) src_d->lpSurface) + (bpp * rect->left) + (src_d->u1.lPitch * rect->top)), *dst;
	    
	    if (*temp_buffer == NULL)
		*temp_buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
					 current_tex_width * current_tex_height * sizeof(DWORD));	    
	    dst = (DWORD *) *temp_buffer;
	    
	    for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
		    DWORD color = *src++;
		    *dst = color & 0xFFFFFF00;
		    if ((color < src_d->ddckCKSrcBlt.dwColorSpaceLowValue) ||
			(color > src_d->ddckCKSrcBlt.dwColorSpaceHighValue))
			*dst |= color & 0x000000FF;
		    dst++;
		}
		src = (DWORD *) (((BYTE *) src) + line_increase);
	    }
	} break;
	
        case CONVERT_CK_8888_ARGB: {
	    unsigned int x, y;
	    DWORD *src = (DWORD *) (((BYTE *) src_d->lpSurface) + (bpp * rect->left) + (src_d->u1.lPitch * rect->top)), *dst;
	    
	    if (*temp_buffer == NULL)
		*temp_buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
					 current_tex_width * current_tex_height * sizeof(DWORD));	    
	    dst = (DWORD *) *temp_buffer;
	    
	    for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
		    DWORD color = *src++;
		    *dst = (color & 0x00FFFFFF) << 8;
		    if ((color < src_d->ddckCKSrcBlt.dwColorSpaceLowValue) ||
			(color > src_d->ddckCKSrcBlt.dwColorSpaceHighValue))
			*dst |= (color & 0xFF000000) >> 24;
		    dst++;
		}
		src = (DWORD *) (((BYTE *) src) + line_increase);
	    }
	} break;
	
        case CONVERT_RGB32_888: {
	    /* Just add an alpha component and handle color-keying... */
	    unsigned int x, y;
	    DWORD *src = (DWORD *) (((BYTE *) src_d->lpSurface) + (bpp * rect->left) + (src_d->u1.lPitch * rect->top)), *dst;
	    
	    if (*temp_buffer == NULL)
		*temp_buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
					 current_tex_width * current_tex_height * sizeof(DWORD));	    
	    dst = (DWORD *) *temp_buffer;
	    
	    if (src_d->dwFlags & DDSD_CKSRCBLT) {
		for (y = 0; y < height; y++) {
		    for (x = 0; x < width; x++) {
			DWORD color = *src++;
			*dst = color << 8;
			if ((color < src_d->ddckCKSrcBlt.dwColorSpaceLowValue) ||
			    (color > src_d->ddckCKSrcBlt.dwColorSpaceHighValue))
			    *dst |= 0xFF;
			dst++;
		    }
		    src = (DWORD *) (((BYTE *) src) + line_increase);
		}
	    } else {
		for (y = 0; y < height; y++) {
		    for (x = 0; x < width; x++) {
			*dst++ = (*src++ << 8) | 0xFF;
		    }
		    src = (DWORD *) (((BYTE *) src) + line_increase);
		}
	    }
	} break;

        case NO_CONVERSION:
	    /* Nothing to do here as the name suggests... Just set-up the buffer correctly */
	    surf_buffer = (((BYTE *) src_d->lpSurface) + (bpp * rect->left) + (src_d->u1.lPitch * rect->top));
	    break;
    }

    if (convert_type != NO_CONVERSION) {
	/* When doing conversion, the storage is always of width 'width' as there will never
	   be any Pitch issue... For now :-)
	*/
	surf_buffer = *temp_buffer;
	if (width != current_storage_width) {
	    glPixelStorei(GL_UNPACK_ROW_LENGTH, width);
	    current_storage_width = width;
	}
    }
    
    glTexSubImage2D(GL_TEXTURE_2D,
		    current_level,
		    xoffset, yoffset,
		    width, height,
		    current_format,
		    current_pixel_format,
		    surf_buffer);

    return DD_OK;
}

HRESULT upload_surface_to_tex_memory_release(void)
{
    current_surface = NULL;

    return DD_OK;
}
