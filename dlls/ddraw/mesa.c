/* Direct3D Common functions
   (c) 1998 Lionel ULMER
   
   This file contains all MESA common code */

#include "windef.h"
#include "wine/obj_base.h"
#include "ddraw.h"
#include "d3d.h"
#include "debugtools.h"

#include "mesa_private.h"

DEFAULT_DEBUG_CHANNEL(ddraw)
    
#define D3DTPRIVATE(x) mesa_d3dt_private *dtpriv = (mesa_d3dt_private*)(x)->private

void set_render_state(D3DRENDERSTATETYPE dwRenderStateType,
		      DWORD dwRenderState, RenderState *rs)
{

  if (TRACE_ON(ddraw))
    _dump_renderstate(dwRenderStateType, dwRenderState);

  /* First, all the stipple patterns */
  if ((dwRenderStateType >= D3DRENDERSTATE_STIPPLEPATTERN00) && 
      (dwRenderStateType <= D3DRENDERSTATE_STIPPLEPATTERN31)) {
    ERR("Unhandled dwRenderStateType stipple %d!\n",dwRenderStateType);
  } else {
    ENTER_GL();
    
    /* All others state variables */
    switch (dwRenderStateType) {

    case D3DRENDERSTATE_TEXTUREHANDLE: {    /*  1 */
      IDirect3DTexture2Impl* tex = (IDirect3DTexture2Impl*) dwRenderState;
      D3DTPRIVATE(tex);
      
      if (tex == NULL) {
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
      } else {
	TRACE("setting OpenGL texture handle : %d\n", dtpriv->tex_name);
	glEnable(GL_TEXTURE_2D);
	/* Default parameters */
	glBindTexture(GL_TEXTURE_2D, dtpriv->tex_name);
	/* To prevent state change, we could test here what are the parameters
	   stored in the texture */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, rs->mag);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, rs->min);
      }
    } break;

    case D3DRENDERSTATE_TEXTUREPERSPECTIVE: /* 4 */
      if (dwRenderState)
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
      else
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
      break;
      
    case D3DRENDERSTATE_ZENABLE:          /*  7 */
      if (dwRenderState)
	glEnable(GL_DEPTH_TEST);
      else
	glDisable(GL_DEPTH_TEST);
      break;
      
    case D3DRENDERSTATE_FILLMODE:           /*  8 */
      switch ((D3DFILLMODE) dwRenderState) {
      case D3DFILL_SOLID:
	break;

      default:
	ERR("Unhandled fill mode !\n");
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
	ERR("Unhandled shade mode !\n");
      }
      break;
      
    case D3DRENDERSTATE_ZWRITEENABLE:     /* 14 */
      if (dwRenderState)
	glDepthMask(GL_TRUE);
      else
	glDepthMask(GL_FALSE);
      break;
      
    case D3DRENDERSTATE_TEXTUREMAG:         /* 17 */
      switch ((D3DTEXTUREFILTER) dwRenderState) {
      case D3DFILTER_NEAREST:
	rs->mag = GL_NEAREST;
	break;
	
      case D3DFILTER_LINEAR:
	rs->mag = GL_LINEAR;
	break;
	
      default:
	ERR("Unhandled texture mag !\n");
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
	ERR("Unhandled texture min !\n");
      }
      break;
      
    case D3DRENDERSTATE_SRCBLEND:           /* 19 */
      switch ((D3DBLEND) dwRenderState) {
      case D3DBLEND_SRCALPHA:
	rs->src = GL_SRC_ALPHA;
	break;

      default:
	ERR("Unhandled blend mode !\n");
      }
      
      glBlendFunc(rs->src, rs->dst);
      break;
      
    case D3DRENDERSTATE_DESTBLEND:          /* 20 */
      switch ((D3DBLEND) dwRenderState) {
      case D3DBLEND_INVSRCALPHA:
	rs->dst = GL_ONE_MINUS_SRC_ALPHA;
	break;
	
      default:
	ERR("Unhandled blend mode !\n");
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
	ERR("Unhandled texture environment !\n");
      }
      break;
      
    case D3DRENDERSTATE_CULLMODE:           /* 22 */
      switch ((D3DCULL) dwRenderState) {
      case D3DCULL_NONE:
	glDisable(GL_CULL_FACE);
	break;
	
      case D3DCULL_CW:
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
	break;
	
      case D3DCULL_CCW:
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	break;
	
      default:
	ERR("Unhandled cull mode !\n");
      }
      break;
      
    case D3DRENDERSTATE_ZFUNC:            /* 23 */
      switch ((D3DCMPFUNC) dwRenderState) {
      case D3DCMP_NEVER:
	glDepthFunc(GL_NEVER);
	break;
      case D3DCMP_LESS:
	glDepthFunc(GL_LESS);
	break;
      case D3DCMP_EQUAL:
	glDepthFunc(GL_EQUAL);
	break;
      case D3DCMP_LESSEQUAL:
	glDepthFunc(GL_LEQUAL);
	break;
      case D3DCMP_GREATER:
	glDepthFunc(GL_GREATER);
	break;
      case D3DCMP_NOTEQUAL:
	glDepthFunc(GL_NOTEQUAL);
	break;
      case D3DCMP_GREATEREQUAL:
	glDepthFunc(GL_GEQUAL);
	break;
      case D3DCMP_ALWAYS:
	glDepthFunc(GL_ALWAYS);
	break;

      default:
	ERR("Unexpected value\n");
      }
      break;
      
    case D3DRENDERSTATE_DITHERENABLE:     /* 26 */
      if (dwRenderState)
	glEnable(GL_DITHER);
      else
	glDisable(GL_DITHER);
      break;
      
    case D3DRENDERSTATE_ALPHABLENDENABLE:   /* 27 */
      if (dwRenderState)
	glEnable(GL_BLEND);
      else
	glDisable(GL_BLEND);
      break;

    case D3DRENDERSTATE_COLORKEYENABLE:     /* 41 */
      if (dwRenderState)
	glEnable(GL_BLEND);
      else
	glDisable(GL_BLEND);
      break;

    case D3DRENDERSTATE_FLUSHBATCH:         /* 50 */
      break;
      
    default:
      ERR("Unhandled dwRenderStateType %d!\n",dwRenderStateType);
      break;
    }

    LEAVE_GL();
  }
}
