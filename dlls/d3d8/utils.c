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

/**
 * @nodoc: todo
 */
void GetSrcAndOpFromValue(DWORD iValue, BOOL isAlphaArg, GLenum* source, GLenum* operand) 
{
  BOOL isAlphaReplicate = FALSE;
  BOOL isComplement     = FALSE;
  
  *operand = GL_SRC_COLOR;
  *source = GL_TEXTURE;
  
  /* Catch alpha replicate */
  if (iValue & D3DTA_ALPHAREPLICATE) {
    iValue = iValue & ~D3DTA_ALPHAREPLICATE;
    isAlphaReplicate = TRUE;
  }
  
  /* Catch Complement */
  if (iValue & D3DTA_COMPLEMENT) {
    iValue = iValue & ~D3DTA_COMPLEMENT;
    isComplement = TRUE;
  }
  
  /* Calculate the operand */
  if (isAlphaReplicate && !isComplement) {
    *operand = GL_SRC_ALPHA;
  } else if (isAlphaReplicate && isComplement) {
    *operand = GL_ONE_MINUS_SRC_ALPHA;
  } else if (isComplement) {
    if (isAlphaArg) {
      *operand = GL_ONE_MINUS_SRC_ALPHA;
    } else {
      *operand = GL_ONE_MINUS_SRC_COLOR;
    }
  } else {
    if (isAlphaArg) {
      *operand = GL_SRC_ALPHA;
    } else {
      *operand = GL_SRC_COLOR;
    }
  }
  
  /* Calculate the source */
  switch (iValue & D3DTA_SELECTMASK) {
  case D3DTA_CURRENT:   *source  = GL_PREVIOUS_EXT;
    break;
  case D3DTA_DIFFUSE:   *source  = GL_PRIMARY_COLOR_EXT;
    break;
  case D3DTA_TEXTURE:   *source  = GL_TEXTURE;
    break;
  case D3DTA_TFACTOR:   *source  = GL_CONSTANT_EXT;
    break;
  case D3DTA_SPECULAR:
    /**
     * According to the GL_ARB_texture_env_combine specs, SPECULAR is 'Secondary color' and
     * isnt supported until base GL supports it
     * There is no concept of temp registers as far as I can tell
     */

  default:
    FIXME("Unrecognized or unhandled texture arg %ld\n", iValue);
    *source = GL_TEXTURE;
  }
}

/* Set texture operations up - The following avoids lots of ifdefs in this routine!*/
#if defined (GL_VERSION_1_3)
  #define useext(A) A
  #define combine_ext 1
#elif defined (GL_EXT_texture_env_combine)
  #define useext(A) A##_EXT
  #define combine_ext 1
#elif defined (GL_ARB_texture_env_combine)
  #define useext(A) A##_ARB
  #define combine_ext 1
#else
  #undef combine_ext
#endif

#if !defined(combine_ext)
void set_tex_op(LPDIRECT3DDEVICE8 iface, BOOL isAlpha, int Stage, D3DTEXTUREOP op, DWORD arg1, DWORD arg2, DWORD arg3)
{ 
        FIXME("Requires opengl combine extensions to work\n");
        return;
}
#else
/* Setup the texture operations texture stage states */
void set_tex_op(LPDIRECT3DDEVICE8 iface, BOOL isAlpha, int Stage, D3DTEXTUREOP op, DWORD arg1, DWORD arg2, DWORD arg3)
{
	GLenum src1, src2, src3;
	GLenum opr1, opr2, opr3;
	GLenum comb_target;
	GLenum src0_target, src1_target, src2_target;
	GLenum opr0_target, opr1_target, opr2_target;
	GLenum scal_target;
	GLenum opr=0, invopr, src3_target, opr3_target;
        BOOL Handled = FALSE;
        ICOM_THIS(IDirect3DDevice8Impl,iface);

        TRACE("Alpha?(%d), Stage:%d Op(%d), a1(%ld), a2(%ld), a3(%ld)\n", isAlpha, Stage, op, arg1, arg2, arg3);
        if (op == D3DTOP_DISABLE) return;

	if (isAlpha) {
		comb_target = useext(GL_COMBINE_ALPHA);
		src0_target = useext(GL_SOURCE0_ALPHA);
		src1_target = useext(GL_SOURCE1_ALPHA);
		src2_target = useext(GL_SOURCE2_ALPHA);
		opr0_target = useext(GL_OPERAND0_ALPHA);
		opr1_target = useext(GL_OPERAND1_ALPHA);
		opr2_target = useext(GL_OPERAND2_ALPHA);
		scal_target = useext(GL_ALPHA_SCALE);
	}
	else {
		comb_target = useext(GL_COMBINE_RGB);
		src0_target = useext(GL_SOURCE0_RGB);
		src1_target = useext(GL_SOURCE1_RGB);
		src2_target = useext(GL_SOURCE2_RGB);
		opr0_target = useext(GL_OPERAND0_RGB);
		opr1_target = useext(GL_OPERAND1_RGB);
		opr2_target = useext(GL_OPERAND2_RGB);
		scal_target = useext(GL_RGB_SCALE);
	}

        /* From MSDN (D3DTSS_ALPHAARG1) : 
           The default argument is D3DTA_TEXTURE. If no texture is set for this stage, 
                   then the default argument is D3DTA_DIFFUSE.
                   FIXME? If texture added/removed, may need to reset back as well?    */
        if (isAlpha && Stage==0 && This->StateBlock->textures[Stage] == NULL && arg1 == D3DTA_TEXTURE) {
            GetSrcAndOpFromValue(D3DTA_DIFFUSE, isAlpha, &src1, &opr1);  
        } else {
            GetSrcAndOpFromValue(arg1, isAlpha, &src1, &opr1);
        }
        GetSrcAndOpFromValue(arg2, isAlpha, &src2, &opr2);
        GetSrcAndOpFromValue(arg3, isAlpha, &src3, &opr3);
        
        TRACE("ct(%x), 1:(%x,%x), 2:(%x,%x), 3:(%x,%x)\n", comb_target, src1, opr1, src2, opr2, src3, opr3);

        Handled = TRUE; /* Assume will be handled */
	switch (op) {
	case D3DTOP_SELECTARG1:
		glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_REPLACE);
		checkGLcall("GL_TEXTURE_ENV, comb_target, GL_REPLACE");
		glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
		checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
		glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
		checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
		glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
		checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
		break;
	case D3DTOP_SELECTARG2:
		glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_REPLACE);
		checkGLcall("GL_TEXTURE_ENV, comb_target, GL_REPLACE");
		glTexEnvi(GL_TEXTURE_ENV, src0_target, src2);
		checkGLcall("GL_TEXTURE_ENV, src0_target, src2");
		glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr2);
		checkGLcall("GL_TEXTURE_ENV, opr0_target, opr2");
		glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
		checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
		break;
	case D3DTOP_MODULATE:
		glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_MODULATE);
		checkGLcall("GL_TEXTURE_ENV, comb_target, GL_MODULATE");
		glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
		checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
		glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
		checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
		glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
		checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
		glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
		checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
		glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
		checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
		break;
	case D3DTOP_MODULATE2X:
		glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_MODULATE);
		checkGLcall("GL_TEXTURE_ENV, comb_target, GL_MODULATE");
		glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
		checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
		glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
		checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
		glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
		checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
		glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
		checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
		glTexEnvi(GL_TEXTURE_ENV, scal_target, 2);
		checkGLcall("GL_TEXTURE_ENV, scal_target, 2");
		break;
	case D3DTOP_MODULATE4X:
		glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_MODULATE);
		checkGLcall("GL_TEXTURE_ENV, comb_target, GL_MODULATE");
		glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
		checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
		glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
		checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
		glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
		checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
		glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
		checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
		glTexEnvi(GL_TEXTURE_ENV, scal_target, 4);
		checkGLcall("GL_TEXTURE_ENV, scal_target, 4");
		break;
	case D3DTOP_ADD:
		glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
		checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD");
		glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
		checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
		glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
		checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
		glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
		checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
		glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
		checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
		glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
		checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
		break;
	case D3DTOP_ADDSIGNED:
		glTexEnvi(GL_TEXTURE_ENV, comb_target, useext(GL_ADD_SIGNED));
		checkGLcall("GL_TEXTURE_ENV, comb_target, useext((GL_ADD_SIGNED)");
		glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
		checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
		glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
		checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
		glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
		checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
		glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
		checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
		glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
		checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
		break;
	case D3DTOP_ADDSIGNED2X:
		glTexEnvi(GL_TEXTURE_ENV, comb_target, useext(GL_ADD_SIGNED));
		checkGLcall("GL_TEXTURE_ENV, comb_target, useext(GL_ADD_SIGNED)");
		glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
		checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
		glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
		checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
		glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
		checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
		glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
		checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
		glTexEnvi(GL_TEXTURE_ENV, scal_target, 2);
		checkGLcall("GL_TEXTURE_ENV, scal_target, 2");
		break;
        case D3DTOP_SUBTRACT:
#if defined(GL_VERSION_1_3) || defined (GL_ARB_texture_env_combine)
		glTexEnvi(GL_TEXTURE_ENV, comb_target, useext(GL_SUBTRACT));
		checkGLcall("GL_TEXTURE_ENV, comb_target, useext(GL_SUBTRACT)");
		glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
		checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
		glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
		checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
		glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
		checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
		glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
		checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
		glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
		checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
#else
                FIXME("This version of opengl does not support GL_SUBTRACT\n");
#endif
		break;
	case D3DTOP_BLENDDIFFUSEALPHA:
		glTexEnvi(GL_TEXTURE_ENV, comb_target, useext(GL_INTERPOLATE));
		checkGLcall("GL_TEXTURE_ENV, comb_target, useext(GL_INTERPOLATE)");
		glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
		checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
		glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
		checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
		glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
		checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
		glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
		checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
		glTexEnvi(GL_TEXTURE_ENV, src2_target, GL_PRIMARY_COLOR);
		checkGLcall("GL_TEXTURE_ENV, src2_target, GL_PRIMARY_COLOR");
		glTexEnvi(GL_TEXTURE_ENV, opr2_target, GL_SRC_ALPHA);
		checkGLcall("GL_TEXTURE_ENV, opr2_target, GL_SRC_ALPHA");
		glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
		checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
		break;
	case D3DTOP_BLENDTEXTUREALPHA:
		glTexEnvi(GL_TEXTURE_ENV, comb_target, useext(GL_INTERPOLATE));
		checkGLcall("GL_TEXTURE_ENV, comb_target, useext(GL_INTERPOLATE)");
		glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
		checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
		glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
		checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
		glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
		checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
		glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
		checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
		glTexEnvi(GL_TEXTURE_ENV, src2_target, GL_TEXTURE);
		checkGLcall("GL_TEXTURE_ENV, src2_target, GL_TEXTURE");
		glTexEnvi(GL_TEXTURE_ENV, opr2_target, GL_SRC_ALPHA);
		checkGLcall("GL_TEXTURE_ENV, opr2_target, GL_SRC_ALPHA");
		glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
		checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
		break;
	case D3DTOP_BLENDFACTORALPHA:
		glTexEnvi(GL_TEXTURE_ENV, comb_target, useext(GL_INTERPOLATE));
		checkGLcall("GL_TEXTURE_ENV, comb_target, useext(GL_INTERPOLATE)");
		glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
		checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
		glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
		checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
		glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
		checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
		glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
		checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
		glTexEnvi(GL_TEXTURE_ENV, src2_target, GL_CONSTANT);
		checkGLcall("GL_TEXTURE_ENV, src2_target, GL_CONSTANT");
		glTexEnvi(GL_TEXTURE_ENV, opr2_target, GL_SRC_ALPHA);
		checkGLcall("GL_TEXTURE_ENV, opr2_target, GL_SRC_ALPHA");
		glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
		checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
		break;
	case D3DTOP_BLENDCURRENTALPHA:
		glTexEnvi(GL_TEXTURE_ENV, comb_target, useext(GL_INTERPOLATE));
		checkGLcall("GL_TEXTURE_ENV, comb_target, useext(GL_INTERPOLATE)");
		glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
		checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
		glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
		checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
		glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
		checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
		glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
		checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
		glTexEnvi(GL_TEXTURE_ENV, src2_target, GL_PREVIOUS);
		checkGLcall("GL_TEXTURE_ENV, src2_target, GL_PREVIOUS");
		glTexEnvi(GL_TEXTURE_ENV, opr2_target, GL_SRC_ALPHA);
		checkGLcall("GL_TEXTURE_ENV, opr2_target, GL_SRC_ALPHA");
		glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
		checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
		break;
        case D3DTOP_DOTPRODUCT3: 
#if defined(GL_EXT_texture_env_dot3) 
		glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_DOT3_RGBA_EXT);
		checkGLcall("GL_TEXTURE_ENV, comb_target, GL_DOT3_RGBA_EXT");
#elif defined(GL_ARB_texture_env_dot3)
		glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_DOT3_RGBA_ARB);
		checkGLcall("GL_TEXTURE_ENV, comb_target, GL_DOT3_RGBA_ARB");
#elif defined (GL_VERSION_1_3)
		glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_DOT3_RGBA);
		checkGLcall("GL_TEXTURE_ENV, comb_target, GL_DOT3_RGBA");
#else
                FIXME("This version of opengl does not support GL_DOT3\n");
#endif
		glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
		checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
		glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
		checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
		glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
		checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
		glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
		checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
		glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
		checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
		break;
	case D3DTOP_LERP:
		glTexEnvi(GL_TEXTURE_ENV, comb_target, useext(GL_INTERPOLATE));
		checkGLcall("GL_TEXTURE_ENV, comb_target, useext(GL_INTERPOLATE)");
		glTexEnvi(GL_TEXTURE_ENV, src0_target, src2);
		checkGLcall("GL_TEXTURE_ENV, src0_target, src2");
		glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr2);
		checkGLcall("GL_TEXTURE_ENV, opr0_target, opr2");
		glTexEnvi(GL_TEXTURE_ENV, src1_target, src3);
		checkGLcall("GL_TEXTURE_ENV, src1_target, src3");
		glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr3);
		checkGLcall("GL_TEXTURE_ENV, opr1_target, opr3");
		glTexEnvi(GL_TEXTURE_ENV, src2_target, src1);
		checkGLcall("GL_TEXTURE_ENV, src2_target, src1");
		glTexEnvi(GL_TEXTURE_ENV, opr2_target, src1);
		checkGLcall("GL_TEXTURE_ENV, opr2_target, src1");
		glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
		checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
		break;
	default:
		Handled = FALSE;
	}
        if (Handled) {
#if defined(GL_NV_texture_env_combine4)
           /* Note: If COMBINE4 in effect cant go back to combine! */
           if (isAlpha) {
               switch (This->UpdateStateBlock->texture_state[Stage][D3DTSS_COLOROP])
               {
               case D3DTOP_ADDSMOOTH:
               case D3DTOP_BLENDTEXTUREALPHAPM:
               case D3DTOP_MODULATEALPHA_ADDCOLOR:
               case D3DTOP_MODULATECOLOR_ADDALPHA:
               case D3DTOP_MODULATEINVALPHA_ADDCOLOR:
               case D3DTOP_MODULATEINVCOLOR_ADDALPHA:
               case D3DTOP_MULTIPLYADD:
                   FIXME("Cant have COMBINE4 and COMBINE in efferct together, alphaop=%d\n", op);
                   return;
               }
           }
#endif
	   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, useext(GL_COMBINE));
	   checkGLcall("GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, useext(GL_COMBINE)");
	   return;
        }

        /* Other texture operations require special extensions: */
#if defined(GL_NV_texture_env_combine4)
	if (isAlpha) {
		invopr = GL_ONE_MINUS_SRC_ALPHA;
		src3_target = GL_SOURCE3_ALPHA_NV;
		opr3_target = GL_OPERAND3_ALPHA_NV;
	}
	else {
		invopr = GL_ONE_MINUS_SRC_COLOR;
		src3_target = GL_SOURCE3_RGB_NV;
		opr3_target = GL_OPERAND3_RGB_NV;
	}
        Handled = TRUE; /* Again, assume handled */
	switch (op) {
	case D3DTOP_ADDSMOOTH:
		glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
		checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD");
		glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
		checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
		glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
		checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
		glTexEnvi(GL_TEXTURE_ENV, src1_target, GL_ZERO);
		checkGLcall("GL_TEXTURE_ENV, src1_target, GL_ZERO");
		glTexEnvi(GL_TEXTURE_ENV, opr1_target, invopr);
		checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
		glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
		checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
		glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
		checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
		glTexEnvi(GL_TEXTURE_ENV, src3_target, src1);
		checkGLcall("GL_TEXTURE_ENV, src3_target, src1");
		switch (opr1) {
		case GL_SRC_COLOR: opr1 = GL_ONE_MINUS_SRC_COLOR; break;
		case GL_ONE_MINUS_SRC_COLOR: opr1 = GL_SRC_COLOR; break;
		case GL_SRC_ALPHA: opr1 = GL_ONE_MINUS_SRC_ALPHA; break;
		case GL_ONE_MINUS_SRC_ALPHA: opr1 = GL_SRC_ALPHA; break;
		}
		glTexEnvi(GL_TEXTURE_ENV, opr3_target, opr1);
		checkGLcall("GL_TEXTURE_ENV, opr3_target, opr1");
		glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
		checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
		break;
	case D3DTOP_BLENDTEXTUREALPHAPM:
		glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
		checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD");
		glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
		checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
		glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
		checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
		glTexEnvi(GL_TEXTURE_ENV, src1_target, GL_ZERO);
		checkGLcall("GL_TEXTURE_ENV, src1_target, GL_ZERO");
		glTexEnvi(GL_TEXTURE_ENV, opr1_target, invopr);
		checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
		glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
		checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
		glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
		checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
		glTexEnvi(GL_TEXTURE_ENV, src3_target, GL_TEXTURE);
		checkGLcall("GL_TEXTURE_ENV, src3_target, GL_TEXTURE");
		glTexEnvi(GL_TEXTURE_ENV, opr3_target, GL_ONE_MINUS_SRC_ALPHA);
		checkGLcall("GL_TEXTURE_ENV, opr3_target, GL_ONE_MINUS_SRC_ALPHA");
		glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
		checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
		break;
	case D3DTOP_MODULATEALPHA_ADDCOLOR:
		glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
		checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD");  /* Add = a0*a1 + a2*a3 */
		glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);        /*   a0 = src1/opr1    */
		checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
		glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
		checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");    /*   a1 = 1 (see docs) */
		glTexEnvi(GL_TEXTURE_ENV, src1_target, GL_ZERO);
		checkGLcall("GL_TEXTURE_ENV, src1_target, GL_ZERO");
		glTexEnvi(GL_TEXTURE_ENV, opr1_target, invopr);      
		checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");  
		glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);        /*   a2 = arg2         */
		checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
		glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
		checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");     /*  a3 = src1 alpha   */
		glTexEnvi(GL_TEXTURE_ENV, src3_target, src1);
		checkGLcall("GL_TEXTURE_ENV, src3_target, src1");
		switch (opr1) {
		case GL_SRC_COLOR: opr1 = GL_SRC_ALPHA; break;
                case GL_ONE_MINUS_SRC_COLOR: opr1 = GL_ONE_MINUS_SRC_ALPHA; break;
		}
		glTexEnvi(GL_TEXTURE_ENV, opr3_target, opr1);
		checkGLcall("GL_TEXTURE_ENV, opr3_target, opr1");
		glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
		checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
		break;
	case D3DTOP_MODULATECOLOR_ADDALPHA:
		glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
		checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD");
		glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
		checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
		glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
		checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
		glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
		checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
		glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
		checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
		glTexEnvi(GL_TEXTURE_ENV, src2_target, src1);
		checkGLcall("GL_TEXTURE_ENV, src2_target, src1");
		switch (opr1) {
		case GL_SRC_COLOR: opr1 = GL_SRC_ALPHA; break;
		case GL_ONE_MINUS_SRC_COLOR: opr1 = GL_ONE_MINUS_SRC_ALPHA; break;
		}
		glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr1);
		checkGLcall("GL_TEXTURE_ENV, opr2_target, opr1");
		glTexEnvi(GL_TEXTURE_ENV, src3_target, GL_ZERO);
		checkGLcall("GL_TEXTURE_ENV, src3_target, GL_ZERO");
		glTexEnvi(GL_TEXTURE_ENV, opr3_target, invopr);
		checkGLcall("GL_TEXTURE_ENV, opr3_target, invopr");
		glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
		checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
		break;
	case D3DTOP_MODULATEINVALPHA_ADDCOLOR:
		glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
		checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD");
		glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
		checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
		glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
		checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
		glTexEnvi(GL_TEXTURE_ENV, src1_target, GL_ZERO);
		checkGLcall("GL_TEXTURE_ENV, src1_target, GL_ZERO");
		glTexEnvi(GL_TEXTURE_ENV, opr1_target, invopr);
		checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
		glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
		checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
		glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
		checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
		glTexEnvi(GL_TEXTURE_ENV, src3_target, src1);
		checkGLcall("GL_TEXTURE_ENV, src3_target, src1");
		switch (opr1) {
		case GL_SRC_COLOR: opr = GL_ONE_MINUS_SRC_ALPHA; break;
		case GL_ONE_MINUS_SRC_COLOR: opr = GL_SRC_ALPHA; break;
		case GL_SRC_ALPHA: opr = GL_ONE_MINUS_SRC_ALPHA; break;
		case GL_ONE_MINUS_SRC_ALPHA: opr = GL_SRC_ALPHA; break;
		}
		glTexEnvi(GL_TEXTURE_ENV, opr3_target, opr);
		checkGLcall("GL_TEXTURE_ENV, opr3_target, opr");
		glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
		checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
		break;
	case D3DTOP_MODULATEINVCOLOR_ADDALPHA:
		glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
		checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD");
		glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
		checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
		switch (opr1) {
		case GL_SRC_COLOR: opr = GL_ONE_MINUS_SRC_COLOR; break;
		case GL_ONE_MINUS_SRC_COLOR: opr = GL_SRC_COLOR; break;
		case GL_SRC_ALPHA: opr = GL_ONE_MINUS_SRC_ALPHA; break;
		case GL_ONE_MINUS_SRC_ALPHA: opr = GL_SRC_ALPHA; break;
		}
		glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr);
		checkGLcall("GL_TEXTURE_ENV, opr0_target, opr");
		glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
		checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
		glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
		checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
		glTexEnvi(GL_TEXTURE_ENV, src2_target, src1);
		checkGLcall("GL_TEXTURE_ENV, src2_target, src1");
		switch (opr1) {
		case GL_SRC_COLOR: opr1 = GL_SRC_ALPHA; break;
		case GL_ONE_MINUS_SRC_COLOR: opr1 = GL_ONE_MINUS_SRC_ALPHA; break;
		}
		glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr1);
		checkGLcall("GL_TEXTURE_ENV, opr2_target, opr1");
		glTexEnvi(GL_TEXTURE_ENV, src3_target, GL_ZERO);
		checkGLcall("GL_TEXTURE_ENV, src3_target, GL_ZERO");
		glTexEnvi(GL_TEXTURE_ENV, opr3_target, invopr);
		checkGLcall("GL_TEXTURE_ENV, opr3_target, invopr");
		glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
		checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
		break;
	case D3DTOP_MULTIPLYADD:
		glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
		checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD");
		glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
		checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
		glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
		checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
		glTexEnvi(GL_TEXTURE_ENV, src1_target, GL_ZERO);
		checkGLcall("GL_TEXTURE_ENV, src1_target, GL_ZERO");
		glTexEnvi(GL_TEXTURE_ENV, opr1_target, invopr);
		checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
		glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
		checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
		glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
		checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
		glTexEnvi(GL_TEXTURE_ENV, src3_target, src3);
		checkGLcall("GL_TEXTURE_ENV, src3_target, src3");
		glTexEnvi(GL_TEXTURE_ENV, opr3_target, opr3);
		checkGLcall("GL_TEXTURE_ENV, opr3_target, opr3");
		glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
		checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
		break;

        default:
                Handled = FALSE;
        }
        if (Handled) {
	    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE4_NV);
	    checkGLcall("GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE4_NV");
            return;
        }
#endif /* GL_NV_texture_env_combine4 */

        /* After all the extensions, if still unhandled, report fixme */
        FIXME("Unhandled texture operation %d\n", op);
}
#endif
