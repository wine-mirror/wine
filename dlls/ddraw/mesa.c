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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include "windef.h"
#include "objbase.h"
#include "ddraw.h"
#include "d3d.h"
#include "wine/debug.h"

#include "mesa_private.h"

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

void set_render_state(D3DRENDERSTATETYPE dwRenderStateType,
		      DWORD dwRenderState, RenderState *rs)
{
    if (TRACE_ON(ddraw))
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
		
		if (tex == NULL) {
		    glBindTexture(GL_TEXTURE_2D, 0);
		    glDisable(GL_TEXTURE_2D);
		    TRACE("disabling texturing\n");
		} else {
		    IDirect3DTextureGLImpl *gl_tex = (IDirect3DTextureGLImpl *) tex->tex_private;
		    
		    glEnable(GL_TEXTURE_2D);
		    /* Default parameters */
		    glBindTexture(GL_TEXTURE_2D, gl_tex->tex_name);
		    /* To prevent state change, we could test here what are the parameters
		       stored in the texture */
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, rs->mag);
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, rs->min);
		    TRACE("setting OpenGL texture handle : %d\n", gl_tex->tex_name);
		}
	    } break;

	      
	    case D3DRENDERSTATE_TEXTUREADDRESSU:  /* 44 */
	    case D3DRENDERSTATE_TEXTUREADDRESSV:  /* 45 */
	    case D3DRENDERSTATE_TEXTUREADDRESS: { /*  3 */
	        GLenum arg = GL_REPEAT; /* Default value */
	        switch ((D3DTEXTUREADDRESS) dwRenderState) {
		    case D3DTADDRESS_WRAP:   arg = GL_REPEAT; break;
		    case D3DTADDRESS_CLAMP:  arg = GL_CLAMP; break;
		    case D3DTADDRESS_BORDER: arg = GL_CLAMP_TO_EDGE; break;
		    default: ERR("Unhandled TEXTUREADDRESS mode %ld !\n", dwRenderState);
		}
		if ((dwRenderStateType == D3DRENDERSTATE_TEXTUREADDRESSU) ||
		    (dwRenderStateType == D3DRENDERSTATE_TEXTUREADDRESS))
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, arg);
		if ((dwRenderStateType == D3DRENDERSTATE_TEXTUREADDRESSV) ||
		    (dwRenderStateType == D3DRENDERSTATE_TEXTUREADDRESS))
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, arg);
	    } break;
	      
	    case D3DRENDERSTATE_TEXTUREPERSPECTIVE: /* 4 */
	        if (dwRenderState)
		    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
		else
		    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
	        break;

	    case D3DRENDERSTATE_WRAPU: /* 5 */
	        if (dwRenderState)
		    ERR("WRAPU mode unsupported by OpenGL.. Expect graphical glitches !\n");
	        break;
	      
	    case D3DRENDERSTATE_WRAPV: /* 6 */
	        if (dwRenderState)
		    ERR("WRAPV mode unsupported by OpenGL.. Expect graphical glitches !\n");
	        break;

	    case D3DRENDERSTATE_ZENABLE:          /*  7 */
	        /* To investigate : in OpenGL, if we disable the depth test, the Z buffer will NOT be
		   updated either.. No idea about what happens in D3D.
		   
		   Maybe replacing the Z function by ALWAYS would be a better idea. */
	        if (dwRenderState == D3DZB_TRUE)
		    glEnable(GL_DEPTH_TEST);
		else if (dwRenderState == D3DZB_FALSE)
		    glDisable(GL_DEPTH_TEST);
		else {
		    glEnable(GL_DEPTH_TEST);
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
	        if (dwRenderState)
		    glDepthMask(GL_TRUE);
		else
		    glDepthMask(GL_FALSE);
	        break;
	      
	    case D3DRENDERSTATE_ALPHATESTENABLE:  /* 15 */
	        if (dwRenderState)
		    glEnable(GL_ALPHA_TEST);
	        else
		    glDisable(GL_ALPHA_TEST);
	        break;

	    case D3DRENDERSTATE_TEXTUREMAG:       /* 17 */
	        switch ((D3DTEXTUREFILTER) dwRenderState) {
		    case D3DFILTER_NEAREST:
		        rs->mag = GL_NEAREST;
			break;
		    case D3DFILTER_LINEAR:
			rs->mag = GL_LINEAR;
			break;
		    default:
			ERR("Unhandled texture mag %ld !\n",dwRenderState);
	        }
	        break;

	    case D3DRENDERSTATE_TEXTUREMIN:         /* 18 */
	        switch ((D3DTEXTUREFILTER) dwRenderState) {
		    case D3DFILTER_NEAREST:
		        rs->min = GL_NEAREST;
			break;
		    case D3DFILTER_LINEAR:
			rs->mag = GL_LINEAR;
			break;
		    default:
			ERR("Unhandled texture min %ld !\n",dwRenderState);
		}
	        break;

	    case D3DRENDERSTATE_SRCBLEND:           /* 19 */
	        switch ((D3DBLEND) dwRenderState) {
		    case D3DBLEND_ZERO:
		          rs->src = GL_ZERO;
			  break;
		    case D3DBLEND_ONE:
		          rs->src = GL_ONE;
			  break;
		    case D3DBLEND_SRCALPHA:
		          rs->src = GL_SRC_ALPHA;
			  break;
		    case D3DBLEND_INVSRCALPHA:
		          rs->src = GL_ONE_MINUS_SRC_ALPHA;
			  break;
		    case D3DBLEND_DESTALPHA:
		          rs->src = GL_DST_ALPHA;
			  break;
		    case D3DBLEND_INVDESTALPHA:
		          rs->src = GL_ONE_MINUS_DST_ALPHA;
			  break;
		    case D3DBLEND_DESTCOLOR:
		          rs->src = GL_DST_COLOR;
			  break;
		    case D3DBLEND_INVDESTCOLOR:
		          rs->src = GL_ONE_MINUS_DST_COLOR;
			  break;
		    case D3DBLEND_BOTHSRCALPHA:
		          rs->src = GL_SRC_ALPHA;
			  rs->dst = GL_SRC_ALPHA;
			  break;
		    case D3DBLEND_BOTHINVSRCALPHA:
		          rs->src = GL_ONE_MINUS_SRC_ALPHA;
			  rs->dst = GL_ONE_MINUS_SRC_ALPHA;
			  break;
		    case D3DBLEND_SRCALPHASAT:
		          rs->src = GL_SRC_ALPHA_SATURATE;
			  break;
		    case D3DBLEND_SRCCOLOR:
		    case D3DBLEND_INVSRCCOLOR:
		          /* Cannot be supported with OpenGL */
			  break;
		    default:
			  ERR("Unhandled src blend mode %ld !\n",dwRenderState);
		}
	        glBlendFunc(rs->src, rs->dst);
	        break;

	    case D3DRENDERSTATE_DESTBLEND:          /* 20 */
	        switch ((D3DBLEND) dwRenderState) {
		    case D3DBLEND_ZERO:
		        rs->dst = GL_ZERO;
			break;
		    case D3DBLEND_ONE:
		        rs->dst = GL_ONE;
			break;
		    case D3DBLEND_SRCCOLOR:
		        rs->dst = GL_SRC_COLOR;
			break;
		    case D3DBLEND_INVSRCCOLOR:
		        rs->dst = GL_ONE_MINUS_SRC_COLOR;
			break;
		    case D3DBLEND_SRCALPHA:
		        rs->dst = GL_SRC_ALPHA;
			break;
		    case D3DBLEND_INVSRCALPHA:
		        rs->dst = GL_ONE_MINUS_SRC_ALPHA;
			break;
		    case D3DBLEND_DESTALPHA:
		        rs->dst = GL_DST_ALPHA;
			break;
		    case D3DBLEND_INVDESTALPHA:
		        rs->dst = GL_ONE_MINUS_DST_ALPHA;
			break;
		    case D3DBLEND_DESTCOLOR:
		    case D3DBLEND_INVDESTCOLOR:
		        /* Cannot be supported with OpenGL */
			break;
		    default:
			ERR("Unhandled dest blend mode %ld !\n",dwRenderState);
		}
	        glBlendFunc(rs->src, rs->dst);
	        break;

	    case D3DRENDERSTATE_TEXTUREMAPBLEND:    /* 21 */
	        switch ((D3DTEXTUREBLEND) dwRenderState) {
		    case D3DTBLEND_MODULATE:
		    case D3DTBLEND_MODULATEALPHA:
		        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			break;
		    default:
			  ERR("Unhandled texture environment %ld !\n",dwRenderState);
		}
	        break;

	    case D3DRENDERSTATE_CULLMODE:           /* 22 */
	        switch ((D3DCULL) dwRenderState) {
		    case D3DCULL_NONE:
		         glDisable(GL_CULL_FACE);
			 break;
			 /* Not sure about these... The DirectX doc is, well, pretty unclear :-) */
		    case D3DCULL_CW:
			 glEnable(GL_CULL_FACE);
			 glFrontFace(GL_CCW);
			 glCullFace(GL_BACK);
			 break;
		    case D3DCULL_CCW:
			 glEnable(GL_CULL_FACE);
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
	      
	    case D3DRENDERSTATE_ALPHAREF:   /* 24 */
	        rs->alpha_ref = dwRenderState / 255.0;
	        glAlphaFunc(rs->alpha_func, rs->alpha_ref);
	        break;
	      
	    case D3DRENDERSTATE_ALPHAFUNC: /* 25 */
	        rs->alpha_func = convert_D3D_compare_to_GL(dwRenderState);
	        glAlphaFunc(rs->alpha_func, rs->alpha_ref);
	        break;

	    case D3DRENDERSTATE_DITHERENABLE:     /* 26 */
	        if (dwRenderState)
		    glEnable(GL_DITHER);
		else
		    glDisable(GL_DITHER);
	        break;

	    case D3DRENDERSTATE_ALPHABLENDENABLE:   /* 27 */
	        if (dwRenderState) {
		    glEnable(GL_BLEND);
		    rs->alpha_blend_enable = TRUE;
		} else {
		    glDisable(GL_BLEND);
		    rs->alpha_blend_enable = FALSE;
		}
	        break;
	      
	    case D3DRENDERSTATE_FOGENABLE: /* 28 */
	        if (dwRenderState) {
		    glEnable(GL_FOG);
		    rs->fog_on = TRUE;
		} else {
		    glDisable(GL_FOG);
		    rs->fog_on = FALSE;
		}
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
	        GLint color[4];
		color[0] = (dwRenderState >> 16) & 0xFF;
		color[1] = (dwRenderState >>  8) & 0xFF;
		color[2] = (dwRenderState >>  0) & 0xFF;
		color[3] = (dwRenderState >> 24) & 0xFF;
		glFogiv(GL_FOG_COLOR, color);
	    } break;

	      
	    case D3DRENDERSTATE_COLORKEYENABLE:     /* 41 */
	        if (dwRenderState)
		    glEnable(GL_BLEND);
		else
		    glDisable(GL_BLEND);
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
	        if (dwRenderState)
		    glDisable(GL_STENCIL_TEST);
		else
		    glDisable(GL_STENCIL_TEST);
		break;
	    
	    case D3DRENDERSTATE_STENCILFAIL:      /* 53 */
	        rs->stencil_fail = convert_D3D_stencilop_to_GL(dwRenderState);
		glStencilOp(rs->stencil_fail, rs->stencil_zfail, rs->stencil_pass);
		break;

	    case D3DRENDERSTATE_STENCILZFAIL:     /* 54 */
	        rs->stencil_zfail = convert_D3D_stencilop_to_GL(dwRenderState);
		glStencilOp(rs->stencil_fail, rs->stencil_zfail, rs->stencil_pass);
		break;

	    case D3DRENDERSTATE_STENCILPASS:      /* 55 */
	        rs->stencil_pass = convert_D3D_stencilop_to_GL(dwRenderState);
		glStencilOp(rs->stencil_fail, rs->stencil_zfail, rs->stencil_pass);
		break;

	    case D3DRENDERSTATE_STENCILFUNC:      /* 56 */
	        rs->stencil_func = convert_D3D_compare_to_GL(dwRenderState);
		glStencilFunc(rs->stencil_func, rs->stencil_ref, rs->stencil_mask);
		break;

	    case D3DRENDERSTATE_STENCILREF:       /* 57 */
	        rs->stencil_ref = dwRenderState;
		glStencilFunc(rs->stencil_func, rs->stencil_ref, rs->stencil_mask);
		break;

	    case D3DRENDERSTATE_STENCILMASK:      /* 58 */
	        rs->stencil_mask = dwRenderState;
		glStencilFunc(rs->stencil_func, rs->stencil_ref, rs->stencil_mask);
		break;
	  
	    case D3DRENDERSTATE_STENCILWRITEMASK: /* 59 */
	        glStencilMask(dwRenderState);
	        break;

	    case D3DRENDERSTATE_CLIPPING:    /* 136 */
	        /* Nothing to do here... Even if what we receive is already clipped by the application,
		   we cannot tell OpenGL to not re-clip it. */
	        break;

	    case D3DRENDERSTATE_LIGHTING:    /* 137 */
	        if (dwRenderState)
		    glEnable(GL_LIGHTING);
		else
		    glDisable(GL_LIGHTING);
	        break;
		
	    case D3DRENDERSTATE_AMBIENT: {            /* 139 */
	        float light[4];

		light[0] = ((dwRenderState >> 16) & 0xFF) / 255.0;
		light[1] = ((dwRenderState >>  8) & 0xFF) / 255.0;
		light[2] = ((dwRenderState >>  0) & 0xFF) / 255.0;
		light[3] = ((dwRenderState >> 24) & 0xFF) / 255.0;
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, (float *) light);
	    } break;

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

	    default:
	        ERR("Unhandled dwRenderStateType %s (%08x) !\n", _get_renderstate(dwRenderStateType), dwRenderStateType);
	}
	LEAVE_GL();
    }
}
