/* Direct3D Common functions
   (c) 1998 Lionel ULMER
   
   This file contains all common miscellaneous code that spans
   different 'objects' */

#include "wintypes.h"
#include "wine/obj_base.h"
#include "ddraw.h"
#include "d3d.h"
#include "debug.h"

#include "d3d_private.h"

#ifdef HAVE_MESAGL

static void _dump_renderstate(D3DRENDERSTATETYPE type,
			      DWORD value) {
  char *states[] = {
    NULL,
    "D3DRENDERSTATE_TEXTUREHANDLE",
    "D3DRENDERSTATE_ANTIALIAS",
    "D3DRENDERSTATE_TEXTUREADDRESS",
    "D3DRENDERSTATE_TEXTUREPERSPECTIVE",
    "D3DRENDERSTATE_WRAPU",
    "D3DRENDERSTATE_WRAPV",
    "D3DRENDERSTATE_ZENABLE",
    "D3DRENDERSTATE_FILLMODE",
    "D3DRENDERSTATE_SHADEMODE",
    "D3DRENDERSTATE_LINEPATTERN",
    "D3DRENDERSTATE_MONOENABLE",
    "D3DRENDERSTATE_ROP2",
    "D3DRENDERSTATE_PLANEMASK",
    "D3DRENDERSTATE_ZWRITEENABLE",
    "D3DRENDERSTATE_ALPHATESTENABLE",
    "D3DRENDERSTATE_LASTPIXEL",
    "D3DRENDERSTATE_TEXTUREMAG",
    "D3DRENDERSTATE_TEXTUREMIN",
    "D3DRENDERSTATE_SRCBLEND",
    "D3DRENDERSTATE_DESTBLEND",
    "D3DRENDERSTATE_TEXTUREMAPBLEND",
    "D3DRENDERSTATE_CULLMODE",
    "D3DRENDERSTATE_ZFUNC",
    "D3DRENDERSTATE_ALPHAREF",
    "D3DRENDERSTATE_ALPHAFUNC",
    "D3DRENDERSTATE_DITHERENABLE",
    "D3DRENDERSTATE_ALPHABLENDENABLE",
    "D3DRENDERSTATE_FOGENABLE",
    "D3DRENDERSTATE_SPECULARENABLE",
    "D3DRENDERSTATE_ZVISIBLE",
    "D3DRENDERSTATE_SUBPIXEL",
    "D3DRENDERSTATE_SUBPIXELX",
    "D3DRENDERSTATE_STIPPLEDALPHA",
    "D3DRENDERSTATE_FOGCOLOR",
    "D3DRENDERSTATE_FOGTABLEMODE",
    "D3DRENDERSTATE_FOGTABLESTART",
    "D3DRENDERSTATE_FOGTABLEEND",
    "D3DRENDERSTATE_FOGTABLEDENSITY",
    "D3DRENDERSTATE_STIPPLEENABLE",
    "D3DRENDERSTATE_EDGEANTIALIAS",
    "D3DRENDERSTATE_COLORKEYENABLE",
    "ERR",
    "D3DRENDERSTATE_BORDERCOLOR",
    "D3DRENDERSTATE_TEXTUREADDRESSU",
    "D3DRENDERSTATE_TEXTUREADDRESSV",
    "D3DRENDERSTATE_MIPMAPLODBIAS",
    "D3DRENDERSTATE_ZBIAS",
    "D3DRENDERSTATE_RANGEFOGENABLE",
    "D3DRENDERSTATE_ANISOTROPY",
    "D3DRENDERSTATE_FLUSHBATCH",
    "ERR", "ERR", "ERR", "ERR", "ERR", "ERR", "ERR",
    "ERR", "ERR", "ERR", "ERR", "ERR", "ERR",
    "D3DRENDERSTATE_STIPPLEPATTERN00",
    "D3DRENDERSTATE_STIPPLEPATTERN01",
    "D3DRENDERSTATE_STIPPLEPATTERN02",
    "D3DRENDERSTATE_STIPPLEPATTERN03",
    "D3DRENDERSTATE_STIPPLEPATTERN04",
    "D3DRENDERSTATE_STIPPLEPATTERN05",
    "D3DRENDERSTATE_STIPPLEPATTERN06",
    "D3DRENDERSTATE_STIPPLEPATTERN07",
    "D3DRENDERSTATE_STIPPLEPATTERN08",
    "D3DRENDERSTATE_STIPPLEPATTERN09",
    "D3DRENDERSTATE_STIPPLEPATTERN10",
    "D3DRENDERSTATE_STIPPLEPATTERN11",
    "D3DRENDERSTATE_STIPPLEPATTERN12",
    "D3DRENDERSTATE_STIPPLEPATTERN13",
    "D3DRENDERSTATE_STIPPLEPATTERN14",
    "D3DRENDERSTATE_STIPPLEPATTERN15",
    "D3DRENDERSTATE_STIPPLEPATTERN16",
    "D3DRENDERSTATE_STIPPLEPATTERN17",
    "D3DRENDERSTATE_STIPPLEPATTERN18",
    "D3DRENDERSTATE_STIPPLEPATTERN19",
    "D3DRENDERSTATE_STIPPLEPATTERN20",
    "D3DRENDERSTATE_STIPPLEPATTERN21",
    "D3DRENDERSTATE_STIPPLEPATTERN22",
    "D3DRENDERSTATE_STIPPLEPATTERN23",
    "D3DRENDERSTATE_STIPPLEPATTERN24",
    "D3DRENDERSTATE_STIPPLEPATTERN25",
    "D3DRENDERSTATE_STIPPLEPATTERN26",
    "D3DRENDERSTATE_STIPPLEPATTERN27",
    "D3DRENDERSTATE_STIPPLEPATTERN28",
    "D3DRENDERSTATE_STIPPLEPATTERN29",
    "D3DRENDERSTATE_STIPPLEPATTERN30",
    "D3DRENDERSTATE_STIPPLEPATTERN31"
  };

  DUMP(" %s = 0x%08lx\n", states[type], value);
}


void set_render_state(D3DRENDERSTATETYPE dwRenderStateType,
		      DWORD dwRenderState, RenderState *rs)
{

  if (TRACE_ON(ddraw))
    _dump_renderstate(dwRenderStateType, dwRenderState);

  /* First, all the stipple patterns */
  if ((dwRenderStateType >= D3DRENDERSTATE_STIPPLEPATTERN00) && 
      (dwRenderStateType <= D3DRENDERSTATE_STIPPLEPATTERN31)) {
    ERR(ddraw, "Unhandled stipple !\n");
  } else {
    /* All others state variables */
    switch (dwRenderStateType) {

    case D3DRENDERSTATE_TEXTUREHANDLE: {    /*  1 */
      LPDIRECT3DTEXTURE2 tex = (LPDIRECT3DTEXTURE2) dwRenderState;
      
      if (tex == NULL) {
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
      } else {
	TRACE(ddraw, "setting OpenGL texture handle : %d\n", tex->tex_name);
	glEnable(GL_TEXTURE_2D);
	/* Default parameters */
	glBindTexture(GL_TEXTURE_2D, tex->tex_name);
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
	ERR(ddraw, "Unhandled fill mode !\n");
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
	ERR(ddraw, "Unhandled shade mode !\n");
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
	ERR(ddraw, "Unhandled texture mag !\n");
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
	ERR(ddraw, "Unhandled texture min !\n");
      }
      break;
      
    case D3DRENDERSTATE_SRCBLEND:           /* 19 */
      switch ((D3DBLEND) dwRenderState) {
      case D3DBLEND_SRCALPHA:
	rs->src = GL_SRC_ALPHA;
	break;

      default:
	ERR(ddraw, "Unhandled blend mode !\n");
      }
      
      glBlendFunc(rs->src, rs->dst);
      break;
      
    case D3DRENDERSTATE_DESTBLEND:          /* 20 */
      switch ((D3DBLEND) dwRenderState) {
      case D3DBLEND_INVSRCALPHA:
	rs->dst = GL_ONE_MINUS_SRC_ALPHA;
	break;
	
      default:
	ERR(ddraw, "Unhandled blend mode !\n");
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
	ERR(ddraw, "Unhandled texture environment !\n");
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
	ERR(ddraw, "Unhandled cull mode !\n");
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
	ERR(ddraw, "Unexpected value\n");
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
      ERR(ddraw, "Unhandled Render State\n");
      break;
    }
  }
}


#endif /* HAVE_MESAGL */
