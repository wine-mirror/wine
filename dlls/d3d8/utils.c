/*
 * D3D8 utils
 *
 * Copyright 2002-2003 Jason Edmeades
 *                     Raphael Junqueira
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

#include <math.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "wine/debug.h"

#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);


#if 0
# define VTRACE(A) TRACE A
#else 
# define VTRACE(A) 
#endif

const char* debug_d3ddevicetype(D3DDEVTYPE devtype) {
  switch (devtype) {
#define DEVTYPE_TO_STR(dev) case dev: return #dev
    DEVTYPE_TO_STR(D3DDEVTYPE_HAL);
    DEVTYPE_TO_STR(D3DDEVTYPE_REF);
    DEVTYPE_TO_STR(D3DDEVTYPE_SW);    
#undef DEVTYPE_TO_STR
  default:
    FIXME("Unrecognized %u D3DDEVTYPE!\n", devtype);
    return "unrecognized";
  }
}

const char* debug_d3dusage(DWORD usage) {
  switch (usage) {
#define D3DUSAGE_TO_STR(u) case u: return #u
    D3DUSAGE_TO_STR(D3DUSAGE_RENDERTARGET);
    D3DUSAGE_TO_STR(D3DUSAGE_DEPTHSTENCIL);
    D3DUSAGE_TO_STR(D3DUSAGE_WRITEONLY);
    D3DUSAGE_TO_STR(D3DUSAGE_SOFTWAREPROCESSING);
    D3DUSAGE_TO_STR(D3DUSAGE_DONOTCLIP);
    D3DUSAGE_TO_STR(D3DUSAGE_POINTS);
    D3DUSAGE_TO_STR(D3DUSAGE_RTPATCHES);
    D3DUSAGE_TO_STR(D3DUSAGE_NPATCHES);
    D3DUSAGE_TO_STR(D3DUSAGE_DYNAMIC);
#undef D3DUSAGE_TO_STR
  case 0: return "none";
  default:
    FIXME("Unrecognized %lu Usage!\n", usage);
    return "unrecognized";
  }
}

const char* debug_d3dformat(D3DFORMAT fmt) {
  switch (fmt) {
#define FMT_TO_STR(fmt) case fmt: return #fmt
    FMT_TO_STR(D3DFMT_UNKNOWN);
    FMT_TO_STR(D3DFMT_R8G8B8);
    FMT_TO_STR(D3DFMT_A8R8G8B8);
    FMT_TO_STR(D3DFMT_X8R8G8B8);
    FMT_TO_STR(D3DFMT_R5G6B5);
    FMT_TO_STR(D3DFMT_X1R5G5B5);
    FMT_TO_STR(D3DFMT_A1R5G5B5);
    FMT_TO_STR(D3DFMT_A4R4G4B4);
    FMT_TO_STR(D3DFMT_R3G3B2);
    FMT_TO_STR(D3DFMT_A8);
    FMT_TO_STR(D3DFMT_A8R3G3B2);
    FMT_TO_STR(D3DFMT_X4R4G4B4);
    FMT_TO_STR(D3DFMT_A8P8);
    FMT_TO_STR(D3DFMT_P8);
    FMT_TO_STR(D3DFMT_L8);
    FMT_TO_STR(D3DFMT_A8L8);
    FMT_TO_STR(D3DFMT_A4L4);
    FMT_TO_STR(D3DFMT_V8U8);
    FMT_TO_STR(D3DFMT_L6V5U5);
    FMT_TO_STR(D3DFMT_X8L8V8U8);
    FMT_TO_STR(D3DFMT_Q8W8V8U8);
    FMT_TO_STR(D3DFMT_V16U16);
    FMT_TO_STR(D3DFMT_W11V11U10);
    FMT_TO_STR(D3DFMT_UYVY);
    FMT_TO_STR(D3DFMT_YUY2);
    FMT_TO_STR(D3DFMT_DXT1);
    FMT_TO_STR(D3DFMT_DXT2);
    FMT_TO_STR(D3DFMT_DXT3);
    FMT_TO_STR(D3DFMT_DXT4);
    FMT_TO_STR(D3DFMT_DXT5);
    FMT_TO_STR(D3DFMT_D16_LOCKABLE);
    FMT_TO_STR(D3DFMT_D32);
    FMT_TO_STR(D3DFMT_D15S1);
    FMT_TO_STR(D3DFMT_D24S8);
    FMT_TO_STR(D3DFMT_D16);
    FMT_TO_STR(D3DFMT_D24X8);
    FMT_TO_STR(D3DFMT_D24X4S4);
    FMT_TO_STR(D3DFMT_VERTEXDATA);
    FMT_TO_STR(D3DFMT_INDEX16);
    FMT_TO_STR(D3DFMT_INDEX32);
#undef FMT_TO_STR
  default:
    FIXME("Unrecognized %u D3DFORMAT!\n", fmt);
    return "unrecognized";
  }
}

const char* debug_d3dressourcetype(D3DRESOURCETYPE res) {
  switch (res) {
#define RES_TO_STR(res) case res: return #res;
    RES_TO_STR(D3DRTYPE_SURFACE);
    RES_TO_STR(D3DRTYPE_VOLUME);
    RES_TO_STR(D3DRTYPE_TEXTURE);
    RES_TO_STR(D3DRTYPE_VOLUMETEXTURE);
    RES_TO_STR(D3DRTYPE_CUBETEXTURE);
    RES_TO_STR(D3DRTYPE_VERTEXBUFFER);
    RES_TO_STR(D3DRTYPE_INDEXBUFFER);
#undef  RES_TO_STR
  default:
    FIXME("Unrecognized %u D3DRESOURCETYPE!\n", res);
    return "unrecognized";
  }
}

const char* debug_d3dprimitivetype(D3DPRIMITIVETYPE PrimitiveType) {
  switch (PrimitiveType) {
#define PRIM_TO_STR(prim) case prim: return #prim;
    PRIM_TO_STR(D3DPT_POINTLIST);
    PRIM_TO_STR(D3DPT_LINELIST);
    PRIM_TO_STR(D3DPT_LINESTRIP);
    PRIM_TO_STR(D3DPT_TRIANGLELIST);
    PRIM_TO_STR(D3DPT_TRIANGLESTRIP);
    PRIM_TO_STR(D3DPT_TRIANGLEFAN);
#undef  PRIM_TO_STR
  default:
    FIXME("Unrecognized %u D3DPRIMITIVETYPE!\n", PrimitiveType);
    return "unrecognized";
  }
}

const char* debug_d3dpool(D3DPOOL Pool) {
  switch (Pool) {
#define POOL_TO_STR(p) case p: return #p;
    POOL_TO_STR(D3DPOOL_DEFAULT);
    POOL_TO_STR(D3DPOOL_MANAGED);
    POOL_TO_STR(D3DPOOL_SYSTEMMEM);
    POOL_TO_STR(D3DPOOL_SCRATCH);
#undef  POOL_TO_STR
  default:
    FIXME("Unrecognized %u D3DPOOL!\n", Pool);
    return "unrecognized";
  }
}

/*
 * Simple utility routines used for dx -> gl mapping of byte formats
 */
int D3DPrimitiveListGetVertexSize(D3DPRIMITIVETYPE PrimitiveType, int iNumPrim) {
  switch (PrimitiveType) {
  case D3DPT_POINTLIST:     return iNumPrim;
  case D3DPT_LINELIST:      return iNumPrim * 2;
  case D3DPT_LINESTRIP:     return iNumPrim + 1;
  case D3DPT_TRIANGLELIST:  return iNumPrim * 3;
  case D3DPT_TRIANGLESTRIP: return iNumPrim + 2;
  case D3DPT_TRIANGLEFAN:   return iNumPrim + 2;
  default:
    FIXME("Unrecognized %u D3DPRIMITIVETYPE!\n", PrimitiveType);
    return 0;
  }
}

int D3DPrimitive2GLenum(D3DPRIMITIVETYPE PrimitiveType) {
  switch (PrimitiveType) {
  case D3DPT_POINTLIST:     return GL_POINTS;
  case D3DPT_LINELIST:      return GL_LINES;
  case D3DPT_LINESTRIP:     return GL_LINE_STRIP;
  case D3DPT_TRIANGLELIST:  return GL_TRIANGLES;
  case D3DPT_TRIANGLESTRIP: return GL_TRIANGLE_STRIP;
  case D3DPT_TRIANGLEFAN:   return GL_TRIANGLE_FAN;
  default:
    FIXME("Unrecognized %u D3DPRIMITIVETYPE!\n", PrimitiveType);
    return GL_POLYGON;
  }
}

int D3DFVFGetSize(D3DFORMAT fvf) {
  int ret = 0;
  if      (fvf & D3DFVF_XYZ)    ret += 3 * sizeof(float);
  else if (fvf & D3DFVF_XYZRHW) ret += 4 * sizeof(float);
  if (fvf & D3DFVF_NORMAL)      ret += 3 * sizeof(float);
  if (fvf & D3DFVF_PSIZE)       ret += sizeof(float);
  if (fvf & D3DFVF_DIFFUSE)     ret += sizeof(DWORD);
  if (fvf & D3DFVF_SPECULAR)    ret += sizeof(DWORD);
  /*if (fvf & D3DFVF_TEX1)        ret += 1;*/
  return ret;
}

GLenum D3DFmt2GLDepthFmt(D3DFORMAT fmt) {
  switch (fmt) {
  /* depth/stencil buffer */
  case D3DFMT_D16_LOCKABLE:
  case D3DFMT_D16:
  case D3DFMT_D15S1:
  case D3DFMT_D24X4S4:
  case D3DFMT_D24S8:
  case D3DFMT_D24X8:
  case D3DFMT_D32:
    return GL_DEPTH_COMPONENT;
  default:
    FIXME("Unhandled fmt(%u,%s)\n", fmt, debug_d3dformat(fmt));
  }
  return 0;
}

GLenum D3DFmt2GLDepthType(D3DFORMAT fmt) {
  switch (fmt) {
  /* depth/stencil buffer */
  case D3DFMT_D15S1:
  case D3DFMT_D16_LOCKABLE:     
  case D3DFMT_D16:              
    return GL_UNSIGNED_SHORT;
  case D3DFMT_D24X4S4:          
  case D3DFMT_D24S8:            
  case D3DFMT_D24X8:            
  case D3DFMT_D32:              
    return GL_UNSIGNED_INT;
  default:
    FIXME("Unhandled fmt(%u,%s)\n", fmt, debug_d3dformat(fmt));
  }
  return 0;
}

SHORT D3DFmtGetBpp(IDirect3DDevice8Impl* This, D3DFORMAT fmt) {
    SHORT retVal;

    switch (fmt) {
    /* color buffer */
    case D3DFMT_P8:               retVal = 1; break;
    case D3DFMT_R3G3B2:           retVal = 1; break;
    case D3DFMT_R5G6B5:           retVal = 2; break;
    case D3DFMT_X1R5G5B5:         retVal = 2; break;
    case D3DFMT_A4R4G4B4:         retVal = 2; break;
    case D3DFMT_X4R4G4B4:         retVal = 2; break;
    case D3DFMT_A1R5G5B5:         retVal = 2; break;
    case D3DFMT_R8G8B8:           retVal = 3; break;
    case D3DFMT_X8R8G8B8:         retVal = 4; break;
    case D3DFMT_A8R8G8B8:         retVal = 4; break;
    /* depth/stencil buffer */
    case D3DFMT_D16_LOCKABLE:     retVal = 2; break;
    case D3DFMT_D16:              retVal = 2; break;
    case D3DFMT_D15S1:            retVal = 2; break;
    case D3DFMT_D24X4S4:          retVal = 4; break;
    case D3DFMT_D24S8:            retVal = 4; break;
    case D3DFMT_D24X8:            retVal = 4; break;
    case D3DFMT_D32:              retVal = 4; break;
    /* unknown */				  
    case D3DFMT_UNKNOWN:
      /* Guess at the highest value of the above */
      TRACE("D3DFMT_UNKNOWN - Guessing at 4 bytes/pixel %u\n", fmt);
      retVal = 4;
      break;

    default:
      FIXME("Unhandled fmt(%u,%s)\n", fmt, debug_d3dformat(fmt));
      retVal = 4;
    }
    TRACE("bytes/Pxl for fmt(%u,%s) = %d\n", fmt, debug_d3dformat(fmt), retVal);
    return retVal;
}

GLint D3DFmt2GLIntFmt(IDirect3DDevice8Impl* This, D3DFORMAT fmt) {
    GLint retVal;

    switch (fmt) {
    case D3DFMT_P8:               retVal = GL_COLOR_INDEX8_EXT; break;
    case D3DFMT_A8P8:             retVal = GL_COLOR_INDEX8_EXT; break;

    case D3DFMT_A4R4G4B4:         retVal = GL_RGBA4; break;
    case D3DFMT_A8R8G8B8:         retVal = GL_RGBA8; break;
    case D3DFMT_X8R8G8B8:         retVal = GL_RGB8; break;
    case D3DFMT_R8G8B8:           retVal = GL_RGB8; break;
    case D3DFMT_R5G6B5:           retVal = GL_RGB5; break; /* fixme: internal format 6 for g? */
    case D3DFMT_A1R5G5B5:         retVal = GL_RGB5_A1; break;
    default:
        FIXME("Unhandled fmt(%u,%s)\n", fmt, debug_d3dformat(fmt));
        retVal = GL_RGB8;
    }
#if defined(GL_EXT_texture_compression_s3tc)
    if (GL_SUPPORT(EXT_TEXTURE_COMPRESSION_S3TC)) {
      switch (fmt) {
      case D3DFMT_DXT1:             retVal = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT; break;
      case D3DFMT_DXT3:             retVal = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; break;
      case D3DFMT_DXT5:             retVal = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; break;
      default:
	/* stupid compiler */
	break;
      }
    }
#endif
    TRACE("fmt2glintFmt for fmt(%u,%s) = %x\n", fmt, debug_d3dformat(fmt), retVal);
    return retVal;
}

GLenum D3DFmt2GLFmt(IDirect3DDevice8Impl* This, D3DFORMAT fmt) {
    GLenum retVal;

    switch (fmt) {
    case D3DFMT_P8:               retVal = GL_COLOR_INDEX; break;
    case D3DFMT_A8P8:             retVal = GL_COLOR_INDEX; break;

    case D3DFMT_A4R4G4B4:         retVal = GL_BGRA; break;
    case D3DFMT_A8R8G8B8:         retVal = GL_BGRA; break;
    case D3DFMT_X8R8G8B8:         retVal = GL_BGRA; break;
    case D3DFMT_R8G8B8:           retVal = GL_BGR; break;
    case D3DFMT_R5G6B5:           retVal = GL_RGB; break;
    case D3DFMT_A1R5G5B5:         retVal = GL_BGRA; break;
    default:
        FIXME("Unhandled fmt(%u,%s)\n", fmt, debug_d3dformat(fmt));
        retVal = GL_BGR;
    }
#if defined(GL_EXT_texture_compression_s3tc)
    if (GL_SUPPORT(EXT_TEXTURE_COMPRESSION_S3TC)) {
      switch (fmt) {
      case D3DFMT_DXT1:             retVal = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT; break;
      case D3DFMT_DXT3:             retVal = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; break;
      case D3DFMT_DXT5:             retVal = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; break;
      default:
	/* stupid compiler */
	break;
      }
    }
#endif
    TRACE("fmt2glFmt for fmt(%u,%s) = %x\n", fmt, debug_d3dformat(fmt), retVal);
    return retVal;
}

GLenum D3DFmt2GLType(IDirect3DDevice8Impl* This, D3DFORMAT fmt) {
    GLenum retVal;

    switch (fmt) {
    case D3DFMT_P8:               retVal = GL_UNSIGNED_BYTE; break;
    case D3DFMT_A8P8:             retVal = GL_UNSIGNED_BYTE; break;

    case D3DFMT_A4R4G4B4:         retVal = GL_UNSIGNED_SHORT_4_4_4_4_REV; break;
    case D3DFMT_A8R8G8B8:         retVal = GL_UNSIGNED_BYTE; break;
    case D3DFMT_X8R8G8B8:         retVal = GL_UNSIGNED_BYTE; break;
    case D3DFMT_R5G6B5:           retVal = GL_UNSIGNED_SHORT_5_6_5; break;
    case D3DFMT_R8G8B8:           retVal = GL_UNSIGNED_BYTE; break;
    case D3DFMT_A1R5G5B5:         retVal = GL_UNSIGNED_SHORT_1_5_5_5_REV; break;
    default:
        FIXME("Unhandled fmt(%u,%s)\n", fmt, debug_d3dformat(fmt));
        retVal = GL_UNSIGNED_BYTE;
    }
#if defined(GL_EXT_texture_compression_s3tc)
    if (GL_SUPPORT(EXT_TEXTURE_COMPRESSION_S3TC)) {
      switch (fmt) {
      case D3DFMT_DXT1:             retVal = 0; break;
      case D3DFMT_DXT3:             retVal = 0; break;
      case D3DFMT_DXT5:             retVal = 0; break;
      default:
	/* stupid compiler */
	break;
      }
    }
#endif
    TRACE("fmt2glType for fmt(%u,%s) = %x\n", fmt, debug_d3dformat(fmt), retVal);
    return retVal;
}

int SOURCEx_RGB_EXT(DWORD arg) {
    switch(arg) {
    case D3DTSS_COLORARG0: return GL_SOURCE2_RGB_EXT;
    case D3DTSS_COLORARG1: return GL_SOURCE0_RGB_EXT;
    case D3DTSS_COLORARG2: return GL_SOURCE1_RGB_EXT;
    case D3DTSS_ALPHAARG0:
    case D3DTSS_ALPHAARG1:
    case D3DTSS_ALPHAARG2:
    default:
        FIXME("Invalid arg %ld\n", arg);
        return GL_SOURCE0_RGB_EXT;
    }
}

int OPERANDx_RGB_EXT(DWORD arg) {
    switch(arg) {
    case D3DTSS_COLORARG0: return GL_OPERAND2_RGB_EXT;
    case D3DTSS_COLORARG1: return GL_OPERAND0_RGB_EXT;
    case D3DTSS_COLORARG2: return GL_OPERAND1_RGB_EXT;
    case D3DTSS_ALPHAARG0:
    case D3DTSS_ALPHAARG1:
    case D3DTSS_ALPHAARG2:
    default:
        FIXME("Invalid arg %ld\n", arg);
        return GL_OPERAND0_RGB_EXT;
    }
}

int SOURCEx_ALPHA_EXT(DWORD arg) {
    switch(arg) {
    case D3DTSS_ALPHAARG0:  return GL_SOURCE2_ALPHA_EXT;
    case D3DTSS_ALPHAARG1:  return GL_SOURCE0_ALPHA_EXT;
    case D3DTSS_ALPHAARG2:  return GL_SOURCE1_ALPHA_EXT;
    case D3DTSS_COLORARG0:
    case D3DTSS_COLORARG1:
    case D3DTSS_COLORARG2:
    default:
        FIXME("Invalid arg %ld\n", arg);
        return GL_SOURCE0_ALPHA_EXT;
    }
}

int OPERANDx_ALPHA_EXT(DWORD arg) {
    switch(arg) {
    case D3DTSS_ALPHAARG0:  return GL_OPERAND2_ALPHA_EXT;
    case D3DTSS_ALPHAARG1:  return GL_OPERAND0_ALPHA_EXT;
    case D3DTSS_ALPHAARG2:  return GL_OPERAND1_ALPHA_EXT;
    case D3DTSS_COLORARG0:
    case D3DTSS_COLORARG1:
    case D3DTSS_COLORARG2:
    default:
        FIXME("Invalid arg %ld\n", arg);
        return GL_OPERAND0_ALPHA_EXT;
    }
}

GLenum StencilOp(DWORD op) {
    switch(op) {                
    case D3DSTENCILOP_KEEP    : return GL_KEEP;
    case D3DSTENCILOP_ZERO    : return GL_ZERO;
    case D3DSTENCILOP_REPLACE : return GL_REPLACE;
    case D3DSTENCILOP_INCRSAT : return GL_INCR;
    case D3DSTENCILOP_DECRSAT : return GL_DECR;
    case D3DSTENCILOP_INVERT  : return GL_INVERT; 
#if defined(GL_VERSION_1_4)
    case D3DSTENCILOP_INCR    : return GL_INCR_WRAP;
    case D3DSTENCILOP_DECR    : return GL_DECR_WRAP;
#elif defined(GL_EXT_stencil_wrap)
    case D3DSTENCILOP_INCR    : return GL_INCR_WRAP_EXT;
    case D3DSTENCILOP_DECR    : return GL_DECR_WRAP_EXT;
#else
    case D3DSTENCILOP_INCR    : FIXME("Unsupported stencil op D3DSTENCILOP_INCR\n");
                                return GL_INCR; /* Fixme - needs to support wrap */
    case D3DSTENCILOP_DECR    : FIXME("Unsupported stencil op D3DSTENCILOP_DECR\n");
                                return GL_DECR; /* Fixme - needs to support wrap */
#endif
    default:
        FIXME("Invalid stencil op %ld\n", op);
        return GL_ALWAYS;
    }
}
