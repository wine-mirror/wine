/*
 * Utility functions for the WineD3D Library
 *
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2003-2004 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
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
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

/*****************************************************************************
 * Trace formatting of useful values
 */
const char* debug_d3dformat(WINED3DFORMAT fmt) {
  switch (fmt) {
#define FMT_TO_STR(fmt) case fmt: return #fmt
    FMT_TO_STR(WINED3DFMT_UNKNOWN);
    FMT_TO_STR(WINED3DFMT_R8G8B8);
    FMT_TO_STR(WINED3DFMT_A8R8G8B8);
    FMT_TO_STR(WINED3DFMT_A8B8G8R8);
    FMT_TO_STR(WINED3DFMT_X8R8G8B8);
    FMT_TO_STR(WINED3DFMT_R5G6B5);
    FMT_TO_STR(WINED3DFMT_X1R5G5B5);
    FMT_TO_STR(WINED3DFMT_A1R5G5B5);
    FMT_TO_STR(WINED3DFMT_A4R4G4B4);
    FMT_TO_STR(WINED3DFMT_R3G3B2);
    FMT_TO_STR(WINED3DFMT_A8);
    FMT_TO_STR(WINED3DFMT_A8R3G3B2);
    FMT_TO_STR(WINED3DFMT_X4R4G4B4);
    FMT_TO_STR(WINED3DFMT_G16R16);
    FMT_TO_STR(WINED3DFMT_A2R10G10B10);
    FMT_TO_STR(WINED3DFMT_A16B16G16R16);
    FMT_TO_STR(WINED3DFMT_A8P8);
    FMT_TO_STR(WINED3DFMT_P8);
    FMT_TO_STR(WINED3DFMT_L8);
    FMT_TO_STR(WINED3DFMT_A8L8);
    FMT_TO_STR(WINED3DFMT_A4L4);
    FMT_TO_STR(WINED3DFMT_V8U8);
    FMT_TO_STR(WINED3DFMT_L6V5U5);
    FMT_TO_STR(WINED3DFMT_X8L8V8U8);
    FMT_TO_STR(WINED3DFMT_Q8W8V8U8);
    FMT_TO_STR(WINED3DFMT_V16U16);
    FMT_TO_STR(WINED3DFMT_W11V11U10);
    FMT_TO_STR(WINED3DFMT_A2W10V10U10);
    FMT_TO_STR(WINED3DFMT_UYVY);
    FMT_TO_STR(WINED3DFMT_YUY2);
    FMT_TO_STR(WINED3DFMT_DXT1);
    FMT_TO_STR(WINED3DFMT_DXT2);
    FMT_TO_STR(WINED3DFMT_DXT3);
    FMT_TO_STR(WINED3DFMT_DXT4);
    FMT_TO_STR(WINED3DFMT_DXT5);
    FMT_TO_STR(WINED3DFMT_MULTI2_ARGB);
    FMT_TO_STR(WINED3DFMT_G8R8_G8B8);
    FMT_TO_STR(WINED3DFMT_R8G8_B8G8);
    FMT_TO_STR(WINED3DFMT_D16_LOCKABLE);
    FMT_TO_STR(WINED3DFMT_D32);
    FMT_TO_STR(WINED3DFMT_D15S1);
    FMT_TO_STR(WINED3DFMT_D24S8);
    FMT_TO_STR(WINED3DFMT_D24FS8);
    FMT_TO_STR(WINED3DFMT_D24X8);
    FMT_TO_STR(WINED3DFMT_D24X4S4);
    FMT_TO_STR(WINED3DFMT_D16);
    FMT_TO_STR(WINED3DFMT_D32F_LOCKABLE);
    FMT_TO_STR(WINED3DFMT_VERTEXDATA);
    FMT_TO_STR(WINED3DFMT_INDEX16);
    FMT_TO_STR(WINED3DFMT_INDEX32);
    FMT_TO_STR(WINED3DFMT_Q16W16V16U16);
    FMT_TO_STR(WINED3DFMT_R16F);
    FMT_TO_STR(WINED3DFMT_G16R16F);
    FMT_TO_STR(WINED3DFMT_A16B16G16R16F);
    FMT_TO_STR(WINED3DFMT_R32F);
    FMT_TO_STR(WINED3DFMT_G32R32F);
    FMT_TO_STR(WINED3DFMT_A32B32G32R32F);
    FMT_TO_STR(WINED3DFMT_CxV8U8);
#undef FMT_TO_STR
  default:
    FIXME("Unrecognized %u D3DFORMAT!\n", fmt);
    return "unrecognized";
  }
}

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
#define WINED3DUSAGE_TO_STR(u) case u: return #u
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_RENDERTARGET);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_DEPTHSTENCIL);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_WRITEONLY);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_SOFTWAREPROCESSING);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_DONOTCLIP);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_POINTS);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_RTPATCHES);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_NPATCHES);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_DYNAMIC);
#undef WINED3DUSAGE_TO_STR
  case 0: return "none";
  default:
    FIXME("Unrecognized %lu Usage!\n", usage);
    return "unrecognized";
  }
}

const char* debug_d3dresourcetype(WINED3DRESOURCETYPE res) {
  switch (res) {
#define RES_TO_STR(res) case res: return #res;
    RES_TO_STR(WINED3DRTYPE_SURFACE);
    RES_TO_STR(WINED3DRTYPE_VOLUME);
    RES_TO_STR(WINED3DRTYPE_TEXTURE);
    RES_TO_STR(WINED3DRTYPE_VOLUMETEXTURE);
    RES_TO_STR(WINED3DRTYPE_CUBETEXTURE);
    RES_TO_STR(WINED3DRTYPE_VERTEXBUFFER);
    RES_TO_STR(WINED3DRTYPE_INDEXBUFFER);
#undef  RES_TO_STR
  default:
    FIXME("Unrecognized %u WINED3DRESOURCETYPE!\n", res);
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

const char* debug_d3drenderstate(DWORD state) {
  switch (state) {
#define D3DSTATE_TO_STR(u) case u: return #u
    D3DSTATE_TO_STR(WINED3DRS_ZENABLE                   );
    D3DSTATE_TO_STR(WINED3DRS_FILLMODE                  );
    D3DSTATE_TO_STR(WINED3DRS_SHADEMODE                 );
    D3DSTATE_TO_STR(WINED3DRS_LINEPATTERN               );
    D3DSTATE_TO_STR(WINED3DRS_ZWRITEENABLE              );
    D3DSTATE_TO_STR(WINED3DRS_ALPHATESTENABLE           );
    D3DSTATE_TO_STR(WINED3DRS_LASTPIXEL                 );
    D3DSTATE_TO_STR(WINED3DRS_SRCBLEND                  );
    D3DSTATE_TO_STR(WINED3DRS_DESTBLEND                 );
    D3DSTATE_TO_STR(WINED3DRS_CULLMODE                  );
    D3DSTATE_TO_STR(WINED3DRS_ZFUNC                     );
    D3DSTATE_TO_STR(WINED3DRS_ALPHAREF                  );
    D3DSTATE_TO_STR(WINED3DRS_ALPHAFUNC                 );
    D3DSTATE_TO_STR(WINED3DRS_DITHERENABLE              );
    D3DSTATE_TO_STR(WINED3DRS_ALPHABLENDENABLE          );
    D3DSTATE_TO_STR(WINED3DRS_FOGENABLE                 );
    D3DSTATE_TO_STR(WINED3DRS_SPECULARENABLE            );
    D3DSTATE_TO_STR(WINED3DRS_ZVISIBLE                  );
    D3DSTATE_TO_STR(WINED3DRS_FOGCOLOR                  );
    D3DSTATE_TO_STR(WINED3DRS_FOGTABLEMODE              );
    D3DSTATE_TO_STR(WINED3DRS_FOGSTART                  );
    D3DSTATE_TO_STR(WINED3DRS_FOGEND                    );
    D3DSTATE_TO_STR(WINED3DRS_FOGDENSITY                );
    D3DSTATE_TO_STR(WINED3DRS_EDGEANTIALIAS             );
    D3DSTATE_TO_STR(WINED3DRS_ZBIAS                     );
    D3DSTATE_TO_STR(WINED3DRS_RANGEFOGENABLE            );
    D3DSTATE_TO_STR(WINED3DRS_STENCILENABLE             );
    D3DSTATE_TO_STR(WINED3DRS_STENCILFAIL               );
    D3DSTATE_TO_STR(WINED3DRS_STENCILZFAIL              );
    D3DSTATE_TO_STR(WINED3DRS_STENCILPASS               );
    D3DSTATE_TO_STR(WINED3DRS_STENCILFUNC               );
    D3DSTATE_TO_STR(WINED3DRS_STENCILREF                );
    D3DSTATE_TO_STR(WINED3DRS_STENCILMASK               );
    D3DSTATE_TO_STR(WINED3DRS_STENCILWRITEMASK          );
    D3DSTATE_TO_STR(WINED3DRS_TEXTUREFACTOR             );
    D3DSTATE_TO_STR(WINED3DRS_WRAP0                     );
    D3DSTATE_TO_STR(WINED3DRS_WRAP1                     );
    D3DSTATE_TO_STR(WINED3DRS_WRAP2                     );
    D3DSTATE_TO_STR(WINED3DRS_WRAP3                     );
    D3DSTATE_TO_STR(WINED3DRS_WRAP4                     );
    D3DSTATE_TO_STR(WINED3DRS_WRAP5                     );
    D3DSTATE_TO_STR(WINED3DRS_WRAP6                     );
    D3DSTATE_TO_STR(WINED3DRS_WRAP7                     );
    D3DSTATE_TO_STR(WINED3DRS_CLIPPING                  );
    D3DSTATE_TO_STR(WINED3DRS_LIGHTING                  );
    D3DSTATE_TO_STR(WINED3DRS_AMBIENT                   );
    D3DSTATE_TO_STR(WINED3DRS_FOGVERTEXMODE             );
    D3DSTATE_TO_STR(WINED3DRS_COLORVERTEX               );
    D3DSTATE_TO_STR(WINED3DRS_LOCALVIEWER               );
    D3DSTATE_TO_STR(WINED3DRS_NORMALIZENORMALS          );
    D3DSTATE_TO_STR(WINED3DRS_DIFFUSEMATERIALSOURCE     );
    D3DSTATE_TO_STR(WINED3DRS_SPECULARMATERIALSOURCE    );
    D3DSTATE_TO_STR(WINED3DRS_AMBIENTMATERIALSOURCE     );
    D3DSTATE_TO_STR(WINED3DRS_EMISSIVEMATERIALSOURCE    );
    D3DSTATE_TO_STR(WINED3DRS_VERTEXBLEND               );
    D3DSTATE_TO_STR(WINED3DRS_CLIPPLANEENABLE           );
    D3DSTATE_TO_STR(WINED3DRS_SOFTWAREVERTEXPROCESSING  );
    D3DSTATE_TO_STR(WINED3DRS_POINTSIZE                 );
    D3DSTATE_TO_STR(WINED3DRS_POINTSIZE_MIN             );
    D3DSTATE_TO_STR(WINED3DRS_POINTSPRITEENABLE         );
    D3DSTATE_TO_STR(WINED3DRS_POINTSCALEENABLE          );
    D3DSTATE_TO_STR(WINED3DRS_POINTSCALE_A              );
    D3DSTATE_TO_STR(WINED3DRS_POINTSCALE_B              );
    D3DSTATE_TO_STR(WINED3DRS_POINTSCALE_C              );
    D3DSTATE_TO_STR(WINED3DRS_MULTISAMPLEANTIALIAS      );
    D3DSTATE_TO_STR(WINED3DRS_MULTISAMPLEMASK           );
    D3DSTATE_TO_STR(WINED3DRS_PATCHEDGESTYLE            );
    D3DSTATE_TO_STR(WINED3DRS_PATCHSEGMENTS             );
    D3DSTATE_TO_STR(WINED3DRS_DEBUGMONITORTOKEN         );
    D3DSTATE_TO_STR(WINED3DRS_POINTSIZE_MAX             );
    D3DSTATE_TO_STR(WINED3DRS_INDEXEDVERTEXBLENDENABLE  );
    D3DSTATE_TO_STR(WINED3DRS_COLORWRITEENABLE          );
    D3DSTATE_TO_STR(WINED3DRS_TWEENFACTOR               );
    D3DSTATE_TO_STR(WINED3DRS_BLENDOP                   );
    D3DSTATE_TO_STR(WINED3DRS_POSITIONDEGREE            );
    D3DSTATE_TO_STR(WINED3DRS_NORMALDEGREE              );
    D3DSTATE_TO_STR(WINED3DRS_SCISSORTESTENABLE         );
    D3DSTATE_TO_STR(WINED3DRS_SLOPESCALEDEPTHBIAS       );
    D3DSTATE_TO_STR(WINED3DRS_ANTIALIASEDLINEENABLE     );
    D3DSTATE_TO_STR(WINED3DRS_MINTESSELLATIONLEVEL      );
    D3DSTATE_TO_STR(WINED3DRS_MAXTESSELLATIONLEVEL      );
    D3DSTATE_TO_STR(WINED3DRS_ADAPTIVETESS_X            );
    D3DSTATE_TO_STR(WINED3DRS_ADAPTIVETESS_Y            );
    D3DSTATE_TO_STR(WINED3DRS_ADAPTIVETESS_Z            );
    D3DSTATE_TO_STR(WINED3DRS_ADAPTIVETESS_W            );
    D3DSTATE_TO_STR(WINED3DRS_ENABLEADAPTIVETESSELLATION);
    D3DSTATE_TO_STR(WINED3DRS_TWOSIDEDSTENCILMODE       );
    D3DSTATE_TO_STR(WINED3DRS_CCW_STENCILFAIL           );
    D3DSTATE_TO_STR(WINED3DRS_CCW_STENCILZFAIL          );
    D3DSTATE_TO_STR(WINED3DRS_CCW_STENCILPASS           );
    D3DSTATE_TO_STR(WINED3DRS_CCW_STENCILFUNC           );
    D3DSTATE_TO_STR(WINED3DRS_COLORWRITEENABLE1         );
    D3DSTATE_TO_STR(WINED3DRS_COLORWRITEENABLE2         );
    D3DSTATE_TO_STR(WINED3DRS_COLORWRITEENABLE3         );
    D3DSTATE_TO_STR(WINED3DRS_BLENDFACTOR               );
    D3DSTATE_TO_STR(WINED3DRS_SRGBWRITEENABLE           );
    D3DSTATE_TO_STR(WINED3DRS_DEPTHBIAS                 );
    D3DSTATE_TO_STR(WINED3DRS_WRAP8                     );
    D3DSTATE_TO_STR(WINED3DRS_WRAP9                     );
    D3DSTATE_TO_STR(WINED3DRS_WRAP10                    );
    D3DSTATE_TO_STR(WINED3DRS_WRAP11                    );
    D3DSTATE_TO_STR(WINED3DRS_WRAP12                    );
    D3DSTATE_TO_STR(WINED3DRS_WRAP13                    );
    D3DSTATE_TO_STR(WINED3DRS_WRAP14                    );
    D3DSTATE_TO_STR(WINED3DRS_WRAP15                    );
    D3DSTATE_TO_STR(WINED3DRS_SEPARATEALPHABLENDENABLE  );
    D3DSTATE_TO_STR(WINED3DRS_SRCBLENDALPHA             );
    D3DSTATE_TO_STR(WINED3DRS_DESTBLENDALPHA            );
    D3DSTATE_TO_STR(WINED3DRS_BLENDOPALPHA              );
#undef D3DSTATE_TO_STR
  default:
    FIXME("Unrecognized %lu render state!\n", state);
    return "unrecognized";
  }
}

const char* debug_d3dsamplerstate(DWORD state) {
  switch (state) {
#define D3DSTATE_TO_STR(u) case u: return #u
    D3DSTATE_TO_STR(WINED3DSAMP_BORDERCOLOR  );
    D3DSTATE_TO_STR(WINED3DSAMP_ADDRESSU     );
    D3DSTATE_TO_STR(WINED3DSAMP_ADDRESSV     );
    D3DSTATE_TO_STR(WINED3DSAMP_ADDRESSW     );
    D3DSTATE_TO_STR(WINED3DSAMP_MAGFILTER    );
    D3DSTATE_TO_STR(WINED3DSAMP_MINFILTER    );
    D3DSTATE_TO_STR(WINED3DSAMP_MIPFILTER    );
    D3DSTATE_TO_STR(WINED3DSAMP_MIPMAPLODBIAS);
    D3DSTATE_TO_STR(WINED3DSAMP_MAXMIPLEVEL  );
    D3DSTATE_TO_STR(WINED3DSAMP_MAXANISOTROPY);
    D3DSTATE_TO_STR(WINED3DSAMP_SRGBTEXTURE  );
    D3DSTATE_TO_STR(WINED3DSAMP_ELEMENTINDEX );
    D3DSTATE_TO_STR(WINED3DSAMP_DMAPOFFSET   );
#undef D3DSTATE_TO_STR
  default:
    FIXME("Unrecognized %lu sampler state!\n", state);
    return "unrecognized";
  }
}

const char* debug_d3dtexturestate(DWORD state) {
  switch (state) {
#define D3DSTATE_TO_STR(u) case u: return #u
    D3DSTATE_TO_STR(WINED3DTSS_COLOROP               );
    D3DSTATE_TO_STR(WINED3DTSS_COLORARG1             );
    D3DSTATE_TO_STR(WINED3DTSS_COLORARG2             );
    D3DSTATE_TO_STR(WINED3DTSS_ALPHAOP               );
    D3DSTATE_TO_STR(WINED3DTSS_ALPHAARG1             );
    D3DSTATE_TO_STR(WINED3DTSS_ALPHAARG2             );
    D3DSTATE_TO_STR(WINED3DTSS_BUMPENVMAT00          );
    D3DSTATE_TO_STR(WINED3DTSS_BUMPENVMAT01          );
    D3DSTATE_TO_STR(WINED3DTSS_BUMPENVMAT10          );
    D3DSTATE_TO_STR(WINED3DTSS_BUMPENVMAT11          );
    D3DSTATE_TO_STR(WINED3DTSS_TEXCOORDINDEX         );
    D3DSTATE_TO_STR(WINED3DTSS_BUMPENVLSCALE         );
    D3DSTATE_TO_STR(WINED3DTSS_BUMPENVLOFFSET        );
    D3DSTATE_TO_STR(WINED3DTSS_TEXTURETRANSFORMFLAGS );
    D3DSTATE_TO_STR(WINED3DTSS_ADDRESSW              );
    D3DSTATE_TO_STR(WINED3DTSS_COLORARG0             );
    D3DSTATE_TO_STR(WINED3DTSS_ALPHAARG0             );
    D3DSTATE_TO_STR(WINED3DTSS_RESULTARG             );
    D3DSTATE_TO_STR(WINED3DTSS_CONSTANT              );
#undef D3DSTATE_TO_STR
  case 12:
    /* Note D3DTSS are not consecutive, so skip these */
    return "unused";
    break;
  default:
    FIXME("Unrecognized %lu texture state!\n", state);
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
/*****************************************************************************
 * Useful functions mapping GL <-> D3D values
 */
GLenum StencilOp(DWORD op) {
    switch(op) {
    case D3DSTENCILOP_KEEP    : return GL_KEEP;
    case D3DSTENCILOP_ZERO    : return GL_ZERO;
    case D3DSTENCILOP_REPLACE : return GL_REPLACE;
    case D3DSTENCILOP_INCRSAT : return GL_INCR;
    case D3DSTENCILOP_DECRSAT : return GL_DECR;
    case D3DSTENCILOP_INVERT  : return GL_INVERT;
    case D3DSTENCILOP_INCR    : return GL_INCR_WRAP_EXT;
    case D3DSTENCILOP_DECR    : return GL_DECR_WRAP_EXT;
    default:
        FIXME("Invalid stencil op %ld\n", op);
        return GL_ALWAYS;
    }
}

/* Set texture operations up - The following avoids lots of ifdefs in this routine!*/
#if defined (GL_VERSION_1_3)
# define useext(A) A
# define combine_ext 1
#elif defined (GL_EXT_texture_env_combine)
# define useext(A) A##_EXT
# define combine_ext 1
#elif defined (GL_ARB_texture_env_combine)
# define useext(A) A##_ARB
# define combine_ext 1
#else
# undef combine_ext
#endif

#if !defined(combine_ext)
void set_tex_op(IWineD3DDevice *iface, BOOL isAlpha, int Stage, D3DTEXTUREOP op, DWORD arg1, DWORD arg2, DWORD arg3)
{
        FIXME("Requires opengl combine extensions to work\n");
        return;
}
#else
/* Setup the texture operations texture stage states */
void set_tex_op(IWineD3DDevice *iface, BOOL isAlpha, int Stage, D3DTEXTUREOP op, DWORD arg1, DWORD arg2, DWORD arg3)
{
#define GLINFO_LOCATION ((IWineD3DImpl *)(This->wineD3D))->gl_info
        GLenum src1, src2, src3;
        GLenum opr1, opr2, opr3;
        GLenum comb_target;
        GLenum src0_target, src1_target, src2_target;
        GLenum opr0_target, opr1_target, opr2_target;
        GLenum scal_target;
        GLenum opr=0, invopr, src3_target, opr3_target;
        BOOL Handled = FALSE;
        IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

        TRACE("Alpha?(%d), Stage:%d Op(%d), a1(%ld), a2(%ld), a3(%ld)\n", isAlpha, Stage, op, arg1, arg2, arg3);

        ENTER_GL();

        /* Note: Operations usually involve two ars, src0 and src1 and are operations of
           the form (a1 <operation> a2). However, some of the more complex operations
           take 3 parameters. Instead of the (sensible) addition of a3, Microsoft added
           in a third parameter called a0. Therefore these are operations of the form
           a0 <operation> a1 <operation> a2, ie the new parameter goes to the front.

           However, below we treat the new (a0) parameter as src2/opr2, so in the actual
           functions below, expect their syntax to differ slightly to those listed in the
           manuals, ie replace arg1 with arg3, arg2 with arg1 and arg3 with arg2
           This affects D3DTOP_MULTIPLYADD and D3DTOP_LERP                               */

        if (isAlpha) {
                comb_target = useext(GL_COMBINE_ALPHA);
                src0_target = useext(GL_SOURCE0_ALPHA);
                src1_target = useext(GL_SOURCE1_ALPHA);
                src2_target = useext(GL_SOURCE2_ALPHA);
                opr0_target = useext(GL_OPERAND0_ALPHA);
                opr1_target = useext(GL_OPERAND1_ALPHA);
                opr2_target = useext(GL_OPERAND2_ALPHA);
                scal_target = GL_ALPHA_SCALE;
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

        /* From MSDN (WINED3DTSS_ALPHAARG1) :
           The default argument is D3DTA_TEXTURE. If no texture is set for this stage,
                   then the default argument is D3DTA_DIFFUSE.
                   FIXME? If texture added/removed, may need to reset back as well?    */
        if (isAlpha && This->stateBlock->textures[Stage] == NULL && arg1 == D3DTA_TEXTURE) {
            GetSrcAndOpFromValue(D3DTA_DIFFUSE, isAlpha, &src1, &opr1);
        } else {
            GetSrcAndOpFromValue(arg1, isAlpha, &src1, &opr1);
        }
        GetSrcAndOpFromValue(arg2, isAlpha, &src2, &opr2);
        GetSrcAndOpFromValue(arg3, isAlpha, &src3, &opr3);

        TRACE("ct(%x), 1:(%x,%x), 2:(%x,%x), 3:(%x,%x)\n", comb_target, src1, opr1, src2, opr2, src3, opr3);

        Handled = TRUE; /* Assume will be handled */

        /* Other texture operations require special extensions: */
        if (GL_SUPPORT(NV_TEXTURE_ENV_COMBINE4)) {
          if (isAlpha) {
            opr = GL_SRC_ALPHA;
            invopr = GL_ONE_MINUS_SRC_ALPHA;
            src3_target = GL_SOURCE3_ALPHA_NV;
            opr3_target = GL_OPERAND3_ALPHA_NV;
          } else {
            opr = GL_SRC_COLOR;
            invopr = GL_ONE_MINUS_SRC_COLOR;
            src3_target = GL_SOURCE3_RGB_NV;
            opr3_target = GL_OPERAND3_RGB_NV;
          }
          switch (op) {
          case D3DTOP_DISABLE: /* Only for alpha */
            glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
            checkGLcall("GL_TEXTURE_ENV, comb_target, GL_REPLACE");
            glTexEnvi(GL_TEXTURE_ENV, src0_target, GL_PREVIOUS_EXT);
            checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
            glTexEnvi(GL_TEXTURE_ENV, opr0_target, GL_SRC_ALPHA);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
            glTexEnvi(GL_TEXTURE_ENV, src1_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src1_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr1_target, invopr);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
            glTexEnvi(GL_TEXTURE_ENV, src2_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src2_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr);
            checkGLcall("GL_TEXTURE_ENV, opr2_target, opr");
            glTexEnvi(GL_TEXTURE_ENV, src3_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src3_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr3_target, opr);
            checkGLcall("GL_TEXTURE_ENV, opr3_target, opr");
            break;
          case D3DTOP_SELECTARG1:                                          /* = a1 * 1 + 0 * 0 */
          case D3DTOP_SELECTARG2:                                          /* = a2 * 1 + 0 * 0 */
            glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
            checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD");
            if (op == D3DTOP_SELECTARG1) {
              glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
              checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
              glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
              checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
            } else {
              glTexEnvi(GL_TEXTURE_ENV, src0_target, src2);
              checkGLcall("GL_TEXTURE_ENV, src0_target, src2");
              glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr2);
              checkGLcall("GL_TEXTURE_ENV, opr0_target, opr2");
            }
            glTexEnvi(GL_TEXTURE_ENV, src1_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src1_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr1_target, invopr);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
            glTexEnvi(GL_TEXTURE_ENV, src2_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src2_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr);
            checkGLcall("GL_TEXTURE_ENV, opr2_target, opr");
            glTexEnvi(GL_TEXTURE_ENV, src3_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src3_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr3_target, opr);
            checkGLcall("GL_TEXTURE_ENV, opr3_target, opr");
            break;

          case D3DTOP_MODULATE:
            glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
            checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD"); /* Add = a0*a1 + a2*a3 */
            glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
            glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
            glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
            checkGLcall("GL_TEXTURE_ENV, src1_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
            glTexEnvi(GL_TEXTURE_ENV, src2_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
            glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr);
            checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
            glTexEnvi(GL_TEXTURE_ENV, src3_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src3_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr3_target, opr);
            checkGLcall("GL_TEXTURE_ENV, opr3_target, opr1");
            glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
            checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
            break;
          case D3DTOP_MODULATE2X:
            glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
            checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD"); /* Add = a0*a1 + a2*a3 */
            glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
            glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
            glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
            checkGLcall("GL_TEXTURE_ENV, src1_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
            glTexEnvi(GL_TEXTURE_ENV, src2_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
            glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr);
            checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
            glTexEnvi(GL_TEXTURE_ENV, src3_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src3_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr3_target, opr);
            checkGLcall("GL_TEXTURE_ENV, opr3_target, opr1");
            glTexEnvi(GL_TEXTURE_ENV, scal_target, 2);
            checkGLcall("GL_TEXTURE_ENV, scal_target, 2");
            break;
          case D3DTOP_MODULATE4X:
            glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
            checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD"); /* Add = a0*a1 + a2*a3 */
            glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
            glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
            glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
            checkGLcall("GL_TEXTURE_ENV, src1_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
            glTexEnvi(GL_TEXTURE_ENV, src2_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
            glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr);
            checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
            glTexEnvi(GL_TEXTURE_ENV, src3_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src3_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr3_target, opr);
            checkGLcall("GL_TEXTURE_ENV, opr3_target, opr1");
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
            glTexEnvi(GL_TEXTURE_ENV, src1_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src1_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr1_target, invopr);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
            glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
            checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
            glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
            checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
            glTexEnvi(GL_TEXTURE_ENV, src3_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src3_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr3_target, invopr);
            checkGLcall("GL_TEXTURE_ENV, opr3_target, invopr");
            glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
            checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
            break;

          case D3DTOP_ADDSIGNED:
            glTexEnvi(GL_TEXTURE_ENV, comb_target, useext(GL_ADD_SIGNED));
            checkGLcall("GL_TEXTURE_ENV, comb_target, useext(GL_ADD_SIGNED)");
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
            glTexEnvi(GL_TEXTURE_ENV, src3_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src3_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr3_target, invopr);
            checkGLcall("GL_TEXTURE_ENV, opr3_target, invopr");
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
            glTexEnvi(GL_TEXTURE_ENV, src1_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src1_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr1_target, invopr);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
            glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
            checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
            glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
            checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
            glTexEnvi(GL_TEXTURE_ENV, src3_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src3_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr3_target, invopr);
            checkGLcall("GL_TEXTURE_ENV, opr3_target, invopr");
            glTexEnvi(GL_TEXTURE_ENV, scal_target, 2);
            checkGLcall("GL_TEXTURE_ENV, scal_target, 2");
            break;

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
            case GL_SRC_COLOR: opr = GL_ONE_MINUS_SRC_COLOR; break;
            case GL_ONE_MINUS_SRC_COLOR: opr = GL_SRC_COLOR; break;
            case GL_SRC_ALPHA: opr = GL_ONE_MINUS_SRC_ALPHA; break;
            case GL_ONE_MINUS_SRC_ALPHA: opr = GL_SRC_ALPHA; break;
            }
            glTexEnvi(GL_TEXTURE_ENV, opr3_target, opr);
            checkGLcall("GL_TEXTURE_ENV, opr3_target, opr");
            glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
            checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
            break;

          case D3DTOP_BLENDDIFFUSEALPHA:
            glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
            checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD");
            glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
            glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
            glTexEnvi(GL_TEXTURE_ENV, src1_target, useext(GL_PRIMARY_COLOR));
            checkGLcall("GL_TEXTURE_ENV, src1_target, useext(GL_PRIMARY_COLOR)");
            glTexEnvi(GL_TEXTURE_ENV, opr1_target, invopr);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
            glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
            checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
            glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
            checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
            glTexEnvi(GL_TEXTURE_ENV, src3_target, useext(GL_PRIMARY_COLOR));
            checkGLcall("GL_TEXTURE_ENV, src3_target, useext(GL_PRIMARY_COLOR)");
            glTexEnvi(GL_TEXTURE_ENV, opr3_target, GL_ONE_MINUS_SRC_ALPHA);
            checkGLcall("GL_TEXTURE_ENV, opr3_target, GL_ONE_MINUS_SRC_ALPHA");
            glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
            checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
            break;
          case D3DTOP_BLENDTEXTUREALPHA:
            glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
            checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD");
            glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
            glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
            glTexEnvi(GL_TEXTURE_ENV, src1_target, GL_TEXTURE);
            checkGLcall("GL_TEXTURE_ENV, src1_target, GL_TEXTURE");
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
          case D3DTOP_BLENDFACTORALPHA:
            glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
            checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD");
            glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
            glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
            glTexEnvi(GL_TEXTURE_ENV, src1_target, useext(GL_CONSTANT));
            checkGLcall("GL_TEXTURE_ENV, src1_target, useext(GL_CONSTANT)");
            glTexEnvi(GL_TEXTURE_ENV, opr1_target, invopr);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
            glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
            checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
            glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
            checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
            glTexEnvi(GL_TEXTURE_ENV, src3_target, useext(GL_CONSTANT));
            checkGLcall("GL_TEXTURE_ENV, src3_target, useext(GL_CONSTANT)");
            glTexEnvi(GL_TEXTURE_ENV, opr3_target, GL_ONE_MINUS_SRC_ALPHA);
            checkGLcall("GL_TEXTURE_ENV, opr3_target, GL_ONE_MINUS_SRC_ALPHA");
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
            switch (opr) {
            case GL_SRC_COLOR: opr = GL_SRC_ALPHA; break;
            case GL_ONE_MINUS_SRC_COLOR: opr = GL_ONE_MINUS_SRC_ALPHA; break;
            }
            glTexEnvi(GL_TEXTURE_ENV, opr3_target, opr);
            checkGLcall("GL_TEXTURE_ENV, opr3_target, opr");
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
            case GL_SRC_COLOR: opr = GL_SRC_ALPHA; break;
            case GL_ONE_MINUS_SRC_COLOR: opr = GL_ONE_MINUS_SRC_ALPHA; break;
            }
            glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr);
            checkGLcall("GL_TEXTURE_ENV, opr2_target, opr");
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
            case GL_SRC_COLOR: opr = GL_SRC_ALPHA; break;
            case GL_ONE_MINUS_SRC_COLOR: opr = GL_ONE_MINUS_SRC_ALPHA; break;
            }
            glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr);
            checkGLcall("GL_TEXTURE_ENV, opr2_target, opr");
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
            glTexEnvi(GL_TEXTURE_ENV, src0_target, src3);
            checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
            glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr3);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
            glTexEnvi(GL_TEXTURE_ENV, src1_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src1_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr1_target, invopr);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
            glTexEnvi(GL_TEXTURE_ENV, src2_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
            glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr1);
            checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
            glTexEnvi(GL_TEXTURE_ENV, src3_target, src2);
            checkGLcall("GL_TEXTURE_ENV, src3_target, src3");
            glTexEnvi(GL_TEXTURE_ENV, opr3_target, opr2);
            checkGLcall("GL_TEXTURE_ENV, opr3_target, opr3");
            glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
            checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
            break;

          case D3DTOP_BUMPENVMAP:
            {
              if (GL_SUPPORT(NV_TEXTURE_SHADER)) {
                /*
                  texture unit 0: GL_TEXTURE_2D
                  texture unit 1: GL_DOT_PRODUCT_NV
                  texture unit 2: GL_DOT_PRODUCT_DIFFUSE_CUBE_MAP_NV
                  texture unit 3: GL_DOT_PRODUCT_REFLECT_CUBE_MAP_NV
                */
                float m[2][2];

                union {
                  float f;
                  DWORD d;
                } tmpvalue;

                tmpvalue.d = This->stateBlock->textureState[Stage][WINED3DTSS_BUMPENVMAT00];
                m[0][0] = tmpvalue.f;
                tmpvalue.d = This->stateBlock->textureState[Stage][WINED3DTSS_BUMPENVMAT01];
                m[0][1] = tmpvalue.f;
                tmpvalue.d = This->stateBlock->textureState[Stage][WINED3DTSS_BUMPENVMAT10];
                m[1][0] = tmpvalue.f;
                tmpvalue.d = This->stateBlock->textureState[Stage][WINED3DTSS_BUMPENVMAT11];
                m[1][1] = tmpvalue.f;

                /*FIXME("Stage %d matrix is (%.2f,%.2f),(%.2f,%.2f)\n", Stage, m[0][0], m[0][1], m[1][0], m[1][0]);*/

                if (FALSE == This->texture_shader_active) {
                  This->texture_shader_active = TRUE;
                  glEnable(GL_TEXTURE_SHADER_NV);
                }

                /*
                glEnable(GL_REGISTER_COMBINERS_NV);
                glCombinerParameteriNV (GL_NUM_GENERAL_COMBINERS_NV, 1);
                glCombinerInputNV (GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_A_NV, GL_TEXTURE0_ARB, GL_SIGNED_IDENTITY_NV, GL_RGB);
                glCombinerInputNV (GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_B_NV, GL_NONE, GL_UNSIGNED_INVERT_NV, GL_RGB);
                glCombinerInputNV (GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_C_NV, GL_TEXTURE2_ARB, GL_SIGNED_IDENTITY_NV, GL_RGB);
                glCombinerInputNV (GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_D_NV, GL_NONE, GL_UNSIGNED_INVERT_NV, GL_RGB);
                glCombinerOutputNV (GL_COMBINER0_NV, GL_RGB, GL_DISCARD_NV, GL_DISCARD_NV, GL_PRIMARY_COLOR_NV, 0, 0, 0, 0, 0);
                glCombinerInputNV (GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_A_NV, GL_TEXTURE0_ARB, GL_SIGNED_IDENTITY_NV, GL_ALPHA);
                glCombinerInputNV (GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_B_NV, GL_NONE, GL_UNSIGNED_INVERT_NV, GL_ALPHA);
                glCombinerInputNV (GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_C_NV, GL_TEXTURE2_ARB, GL_SIGNED_IDENTITY_NV, GL_ALPHA);
                glCombinerInputNV (GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_D_NV, GL_NONE, GL_UNSIGNED_INVERT_NV, GL_ALPHA);
                glCombinerOutputNV (GL_COMBINER0_NV, GL_ALPHA, GL_DISCARD_NV, GL_DISCARD_NV, GL_PRIMARY_COLOR_NV, 0, 0, 0, 0, 0);
                glDisable (GL_PER_STAGE_CONSTANTS_NV);
                glCombinerParameteriNV (GL_COLOR_SUM_CLAMP_NV, 0);
                glFinalCombinerInputNV (GL_VARIABLE_A_NV, 0, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
                glFinalCombinerInputNV (GL_VARIABLE_B_NV, 0, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
                glFinalCombinerInputNV (GL_VARIABLE_C_NV, 0, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
                glFinalCombinerInputNV (GL_VARIABLE_D_NV, GL_PRIMARY_COLOR_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
                glFinalCombinerInputNV (GL_VARIABLE_E_NV, 0, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
                glFinalCombinerInputNV (GL_VARIABLE_F_NV, 0, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
                glFinalCombinerInputNV (GL_VARIABLE_G_NV, GL_PRIMARY_COLOR_NV, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA);
                */
                /*
                int i;
                for (i = 0; i < Stage; i++) {
                  if (GL_SUPPORT(ARB_MULTITEXTURE)) {
                    GL_ACTIVETEXTURE(i);
                    glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, GL_TEXTURE_2D);
                    checkGLcall("Activate texture..");
                  } else if (i>0) {
                    FIXME("Program using multiple concurrent textures which this opengl implementation doesn't support\n");
                  }
                }
                */
                /*
                  GL_ACTIVETEXTURE(Stage - 1);
                  glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, GL_TEXTURE_2D);
                */
                /*
                GL_ACTIVETEXTURE(Stage);
                checkGLcall("Activate texture.. to update const color");
                glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, GL_OFFSET_TEXTURE_2D_NV);
                checkGLcall("glTexEnv");
                glTexEnvi(GL_TEXTURE_SHADER_NV, GL_PREVIOUS_TEXTURE_INPUT_NV, GL_TEXTURE0_ARB + Stage - 1);
                checkGLcall("glTexEnv");
                glTexEnvfv(GL_TEXTURE_SHADER_NV, GL_OFFSET_TEXTURE_MATRIX_NV, (float*)&m[0]);
                checkGLcall("glTexEnv");
                */
                LEAVE_GL();
                return;
                break;
              }
            }

          case D3DTOP_BUMPENVMAPLUMINANCE:

          default:
            Handled = FALSE;
          }
          if (Handled) {
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE4_NV);
            checkGLcall("GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE4_NV");

            LEAVE_GL();
            return;
          }
        } /* GL_NV_texture_env_combine4 */

        Handled = TRUE; /* Again, assume handled */
        switch (op) {
        case D3DTOP_DISABLE: /* Only for alpha */
                glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_REPLACE);
                checkGLcall("GL_TEXTURE_ENV, comb_target, GL_REPLACE");
                glTexEnvi(GL_TEXTURE_ENV, src0_target, GL_PREVIOUS_EXT);
                checkGLcall("GL_TEXTURE_ENV, src0_target, GL_PREVIOUS_EXT");
                glTexEnvi(GL_TEXTURE_ENV, opr0_target, GL_SRC_ALPHA);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, GL_SRC_ALPHA");
                glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                break;
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
          if (GL_SUPPORT(ARB_TEXTURE_ENV_COMBINE)) {
                glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_SUBTRACT);
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
          } else {
                FIXME("This version of opengl does not support GL_SUBTRACT\n");
          }
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
                glTexEnvi(GL_TEXTURE_ENV, src2_target, useext(GL_PRIMARY_COLOR));
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
                glTexEnvi(GL_TEXTURE_ENV, src2_target, useext(GL_CONSTANT));
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
                glTexEnvi(GL_TEXTURE_ENV, src2_target, useext(GL_PREVIOUS));
                checkGLcall("GL_TEXTURE_ENV, src2_target, GL_PREVIOUS");
                glTexEnvi(GL_TEXTURE_ENV, opr2_target, GL_SRC_ALPHA);
                checkGLcall("GL_TEXTURE_ENV, opr2_target, GL_SRC_ALPHA");
                glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                break;
        case D3DTOP_DOTPRODUCT3:
                if (GL_SUPPORT(ARB_TEXTURE_ENV_DOT3)) {
                  glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_DOT3_RGBA_ARB);
                  checkGLcall("GL_TEXTURE_ENV, comb_target, GL_DOT3_RGBA_ARB");
                } else if (GL_SUPPORT(EXT_TEXTURE_ENV_DOT3)) {
                  glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_DOT3_RGBA_EXT);
                  checkGLcall("GL_TEXTURE_ENV, comb_target, GL_DOT3_RGBA_EXT");
                } else {
                  FIXME("This version of opengl does not support GL_DOT3\n");
                }
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
                glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
                glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
                checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
                glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
                glTexEnvi(GL_TEXTURE_ENV, src2_target, src3);
                checkGLcall("GL_TEXTURE_ENV, src2_target, src3");
                glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr3);
                checkGLcall("GL_TEXTURE_ENV, opr2_target, opr3");
                glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                break;
        case D3DTOP_ADDSMOOTH:
                if (GL_SUPPORT(ATI_TEXTURE_ENV_COMBINE3)) {
                  glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI);
                  checkGLcall("GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI");
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
                  glTexEnvi(GL_TEXTURE_ENV, src1_target, src1);
                  checkGLcall("GL_TEXTURE_ENV, src1_target, src1");
                  glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr1);
                  checkGLcall("GL_TEXTURE_ENV, opr1_target, opr1");
                  glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
                  checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
                  glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
                  checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
                  glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                  checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                } else
                  Handled = FALSE;
                break;
        case D3DTOP_BLENDTEXTUREALPHAPM:
                if (GL_SUPPORT(ATI_TEXTURE_ENV_COMBINE3)) {
                  glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI);
                  checkGLcall("GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI");
                  glTexEnvi(GL_TEXTURE_ENV, src0_target, GL_TEXTURE);
                  checkGLcall("GL_TEXTURE_ENV, src0_target, GL_TEXTURE");
                  glTexEnvi(GL_TEXTURE_ENV, opr0_target, GL_ONE_MINUS_SRC_ALPHA);
                  checkGLcall("GL_TEXTURE_ENV, opr0_target, GL_ONE_MINUS_SRC_APHA");
                  glTexEnvi(GL_TEXTURE_ENV, src1_target, src1);
                  checkGLcall("GL_TEXTURE_ENV, src1_target, src1");
                  glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr1);
                  checkGLcall("GL_TEXTURE_ENV, opr1_target, opr1");
                  glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
                  checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
                  glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
                  checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
                  glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                  checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                } else
                  Handled = FALSE;
                break;
        case D3DTOP_MODULATEALPHA_ADDCOLOR:
                if (GL_SUPPORT(ATI_TEXTURE_ENV_COMBINE3)) {
                  glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI);
                  checkGLcall("GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI");
                  glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                  checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                  switch (opr1) {
                  case GL_SRC_COLOR: opr = GL_SRC_ALPHA; break;
                  case GL_ONE_MINUS_SRC_COLOR: opr = GL_ONE_MINUS_SRC_ALPHA; break;
                  case GL_SRC_ALPHA: opr = GL_SRC_ALPHA; break;
                  case GL_ONE_MINUS_SRC_ALPHA: opr = GL_ONE_MINUS_SRC_ALPHA; break;
                  }
                  glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr);
                  checkGLcall("GL_TEXTURE_ENV, opr0_target, opr");
                  glTexEnvi(GL_TEXTURE_ENV, src1_target, src1);
                  checkGLcall("GL_TEXTURE_ENV, src1_target, src1");
                  glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr1);
                  checkGLcall("GL_TEXTURE_ENV, opr1_target, opr1");
                  glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
                  checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
                  glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
                  checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
                  glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                  checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                } else
                  Handled = FALSE;
                break;
        case D3DTOP_MODULATECOLOR_ADDALPHA:
                if (GL_SUPPORT(ATI_TEXTURE_ENV_COMBINE3)) {
                  glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI);
                  checkGLcall("GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI");
                  glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                  checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                  glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
                  checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
                  glTexEnvi(GL_TEXTURE_ENV, src1_target, src1);
                  checkGLcall("GL_TEXTURE_ENV, src1_target, src1");
                  switch (opr1) {
                  case GL_SRC_COLOR: opr = GL_SRC_ALPHA; break;
                  case GL_ONE_MINUS_SRC_COLOR: opr = GL_ONE_MINUS_SRC_ALPHA; break;
                  case GL_SRC_ALPHA: opr = GL_SRC_ALPHA; break;
                  case GL_ONE_MINUS_SRC_ALPHA: opr = GL_ONE_MINUS_SRC_ALPHA; break;
                  }
                  glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr);
                  checkGLcall("GL_TEXTURE_ENV, opr1_target, opr");
                  glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
                  checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
                  glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
                  checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
                  glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                  checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                } else
                  Handled = FALSE;
                break;
        case D3DTOP_MODULATEINVALPHA_ADDCOLOR:
                if (GL_SUPPORT(ATI_TEXTURE_ENV_COMBINE3)) {
                  glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI);
                  checkGLcall("GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI");
                  glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                  checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                  switch (opr1) {
                  case GL_SRC_COLOR: opr = GL_ONE_MINUS_SRC_ALPHA; break;
                  case GL_ONE_MINUS_SRC_COLOR: opr = GL_SRC_ALPHA; break;
                  case GL_SRC_ALPHA: opr = GL_ONE_MINUS_SRC_ALPHA; break;
                  case GL_ONE_MINUS_SRC_ALPHA: opr = GL_SRC_ALPHA; break;
                  }
                  glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr);
                  checkGLcall("GL_TEXTURE_ENV, opr0_target, opr");
                  glTexEnvi(GL_TEXTURE_ENV, src1_target, src1);
                  checkGLcall("GL_TEXTURE_ENV, src1_target, src1");
                  glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr1);
                  checkGLcall("GL_TEXTURE_ENV, opr1_target, opr1");
                  glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
                  checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
                  glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
                  checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
                  glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                  checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                } else
                  Handled = FALSE;
                break;
        case D3DTOP_MODULATEINVCOLOR_ADDALPHA:
                if (GL_SUPPORT(ATI_TEXTURE_ENV_COMBINE3)) {
                  glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI);
                  checkGLcall("GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI");
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
                  glTexEnvi(GL_TEXTURE_ENV, src1_target, src1);
                  checkGLcall("GL_TEXTURE_ENV, src1_target, src1");
                  switch (opr1) {
                  case GL_SRC_COLOR: opr = GL_SRC_ALPHA; break;
                  case GL_ONE_MINUS_SRC_COLOR: opr = GL_ONE_MINUS_SRC_ALPHA; break;
                  case GL_SRC_ALPHA: opr = GL_SRC_ALPHA; break;
                  case GL_ONE_MINUS_SRC_ALPHA: opr = GL_ONE_MINUS_SRC_ALPHA; break;
                  }
                  glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr);
                  checkGLcall("GL_TEXTURE_ENV, opr1_target, opr");
                  glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
                  checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
                  glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
                  checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
                  glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                  checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                } else
                  Handled = FALSE;
                break;
        case D3DTOP_MULTIPLYADD:
                if (GL_SUPPORT(ATI_TEXTURE_ENV_COMBINE3)) {
                  glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI);
                  checkGLcall("GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI");
                  glTexEnvi(GL_TEXTURE_ENV, src0_target, src3);
                  checkGLcall("GL_TEXTURE_ENV, src0_target, src3");
                  glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr3);
                  checkGLcall("GL_TEXTURE_ENV, opr0_target, opr3");
                  glTexEnvi(GL_TEXTURE_ENV, src1_target, src1);
                  checkGLcall("GL_TEXTURE_ENV, src1_target, src1");
                  glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr1);
                  checkGLcall("GL_TEXTURE_ENV, opr1_target, opr1");
                  glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
                  checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
                  glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
                  checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
                  glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                  checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                } else
                  Handled = FALSE;
                break;
        default:
                Handled = FALSE;
        }

        if (Handled) {
          BOOL  combineOK = TRUE;
          if (GL_SUPPORT(NV_TEXTURE_ENV_COMBINE4)) {
            DWORD op2;

            if (isAlpha) {
              op2 = This->stateBlock->textureState[Stage][WINED3DTSS_COLOROP];
            } else {
              op2 = This->stateBlock->textureState[Stage][WINED3DTSS_ALPHAOP];
            }

            /* Note: If COMBINE4 in effect can't go back to combine! */
            switch (op2) {
            case D3DTOP_ADDSMOOTH:
            case D3DTOP_BLENDTEXTUREALPHAPM:
            case D3DTOP_MODULATEALPHA_ADDCOLOR:
            case D3DTOP_MODULATECOLOR_ADDALPHA:
            case D3DTOP_MODULATEINVALPHA_ADDCOLOR:
            case D3DTOP_MODULATEINVCOLOR_ADDALPHA:
            case D3DTOP_MULTIPLYADD:
              /* Ignore those implemented in both cases */
              switch (op) {
              case D3DTOP_SELECTARG1:
              case D3DTOP_SELECTARG2:
                combineOK = FALSE;
                Handled   = FALSE;
                break;
              default:
                FIXME("Can't use COMBINE4 and COMBINE together, thisop=%d, otherop=%ld, isAlpha(%d)\n", op, op2, isAlpha);
                LEAVE_GL();
                return;
              }
            }
          }

          if (combineOK) {
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, useext(GL_COMBINE));
            checkGLcall("GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, useext(GL_COMBINE)");

            LEAVE_GL();
            return;
          }
        }

        LEAVE_GL();

        /* After all the extensions, if still unhandled, report fixme */
        FIXME("Unhandled texture operation %d\n", op);
        #undef GLINFO_LOCATION
}
#endif

/* Setup this textures matrix according to the texture flags*/
void set_texture_matrix(const float *smat, DWORD flags, BOOL calculatedCoords)
{
    float mat[16];

    glMatrixMode(GL_TEXTURE);
    checkGLcall("glMatrixMode(GL_TEXTURE)");

    if (flags == D3DTTFF_DISABLE) {
        glLoadIdentity();
        checkGLcall("glLoadIdentity()");
        return;
    }

    if (flags == (D3DTTFF_COUNT1|D3DTTFF_PROJECTED)) {
        ERR("Invalid texture transform flags: D3DTTFF_COUNT1|D3DTTFF_PROJECTED\n");
        return;
    }

    memcpy(mat, smat, 16 * sizeof(float));

    switch (flags & ~D3DTTFF_PROJECTED) {
    case D3DTTFF_COUNT1: mat[1] = mat[5] = mat[13] = 0;
    case D3DTTFF_COUNT2: mat[2] = mat[6] = mat[10] = mat[14] = 0;
    default: mat[3] = mat[7] = mat[11] = 0, mat[15] = 1;
    }

    if (flags & D3DTTFF_PROJECTED) {
        switch (flags & ~D3DTTFF_PROJECTED) {
        case D3DTTFF_COUNT2:
            mat[3] = mat[1], mat[7] = mat[5], mat[11] = mat[9], mat[15] = mat[13];
            mat[1] = mat[5] = mat[9] = mat[13] = 0;
            break;
        case D3DTTFF_COUNT3:
            mat[3] = mat[2], mat[7] = mat[6], mat[11] = mat[10], mat[15] = mat[14];
            mat[2] = mat[6] = mat[10] = mat[14] = 0;
            break;
        }
    } else if(!calculatedCoords) { /* under directx the R/Z coord can be used for translation, under opengl we use the Q coord instead */
        mat[12] = mat[8];
        mat[13] = mat[9];
    }

    glLoadMatrixf(mat);
    checkGLcall("glLoadMatrixf(mat)");
}

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
    /*
     * According to the GL_ARB_texture_env_combine specs, SPECULAR is
     * 'Secondary color' and isn't supported until base GL supports it
     * There is no concept of temp registers as far as I can tell
     */
    FIXME("Unhandled texture arg D3DTA_SPECULAR\n");
    *source = GL_TEXTURE;
    break;

  default:
    FIXME("Unrecognized texture arg %ld\n", iValue);
    *source = GL_TEXTURE;
  }
}


#define GLINFO_LOCATION ((IWineD3DImpl *)(This->wineD3D))->gl_info
GLint D3DFmt2GLIntFmt(IWineD3DDeviceImpl* This, D3DFORMAT fmt) {
    GLint retVal = 0;

    if (GL_SUPPORT(EXT_TEXTURE_COMPRESSION_S3TC)) {
        switch (fmt) {
        case WINED3DFMT_DXT1:             retVal = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT; break;
        case WINED3DFMT_DXT2:             retVal = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; break;
        case WINED3DFMT_DXT3:             retVal = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; break;
        case WINED3DFMT_DXT4:             retVal = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; break;
        case WINED3DFMT_DXT5:             retVal = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; break;
        default:
            /* stupid compiler */
            break;
        }
    }

    if (retVal == 0) {
        switch (fmt) {
            /* Paletted */
        case WINED3DFMT_P8:               retVal = GL_COLOR_INDEX8_EXT; break;
        case WINED3DFMT_A8P8:             retVal = GL_COLOR_INDEX8_EXT; break;
            /* Luminance */
        case WINED3DFMT_L8:               retVal = GL_LUMINANCE8; break;
        case WINED3DFMT_A8L8:             retVal = GL_LUMINANCE8_ALPHA8; break;
        case WINED3DFMT_A4L4:             retVal = GL_LUMINANCE4_ALPHA4; break;
            /* Bump */
        case WINED3DFMT_V8U8:             retVal = GL_COLOR_INDEX8_EXT; break;
        case WINED3DFMT_V16U16:           retVal = GL_COLOR_INDEX; break;
        case WINED3DFMT_L6V5U5:           retVal = GL_COLOR_INDEX8_EXT; break;
        case WINED3DFMT_X8L8V8U8:         retVal = GL_RGBA8; break;
        case WINED3DFMT_Q8W8V8U8:         retVal = GL_RGBA8; break; /* almost but not quite... */
        case WINED3DFMT_Q16W16V16U16:     retVal = GL_COLOR_INDEX; break; /* almost but not quite... */
            /* color buffer */
        case WINED3DFMT_R3G3B2:           retVal = GL_R3_G3_B2; break;
        case WINED3DFMT_R5G6B5:           retVal = GL_RGB5; break; /* fixme: internal format 6 for g? */
        case WINED3DFMT_R8G8B8:           retVal = GL_RGB8; break;
        case WINED3DFMT_A1R5G5B5:         retVal = GL_RGB5_A1; break;
        case WINED3DFMT_X1R5G5B5:         retVal = GL_RGB5_A1; break;
        case WINED3DFMT_A4R4G4B4:         retVal = GL_RGBA4; break;
        case WINED3DFMT_X4R4G4B4:         retVal = GL_RGB4; break;
        case WINED3DFMT_A8R8G8B8:         retVal = GL_RGBA8; break;
        case WINED3DFMT_A8B8G8R8:         retVal = GL_RGBA8; break;
        case WINED3DFMT_A2R10G10B10:      retVal = GL_RGBA8; break;
        case WINED3DFMT_X8R8G8B8:         retVal = GL_RGB; break;
            /* to see */
        case WINED3DFMT_A8:               retVal = GL_ALPHA8; break;

        /* Depth + Stencil NOTE: OpenGL doesn't support depth-stencil surfaces so the formats are the closes bits match for the data */
        case WINED3DFMT_D24S8:            retVal = GL_COLOR_INDEX; break;
        case WINED3DFMT_D24FS8:           retVal = GL_COLOR_INDEX; break;
        case WINED3DFMT_D24X8:            retVal = GL_COLOR_INDEX; break;
        case WINED3DFMT_D24X4S4:          retVal = GL_COLOR_INDEX; break;
        case WINED3DFMT_D32:              retVal = GL_COLOR_INDEX; break;
        case WINED3DFMT_D16:              retVal = GL_COLOR_INDEX; break;
        case WINED3DFMT_D15S1:            retVal = GL_COLOR_INDEX; break;
        case WINED3DFMT_D16_LOCKABLE:     retVal = GL_COLOR_INDEX; break;

        default:
            FIXME("Unhandled fmt(%u,%s)\n", fmt, debug_d3dformat(fmt));
            retVal = GL_RGB8;
        }
    }
    TRACE("fmt2glintFmt for fmt(%u,%s) = %x\n", fmt, debug_d3dformat(fmt), retVal);
    return retVal;
}

GLenum D3DFmt2GLFmt(IWineD3DDeviceImpl* This, D3DFORMAT fmt) {
    GLenum retVal = 0;

    if (GL_SUPPORT(EXT_TEXTURE_COMPRESSION_S3TC)) {
        switch (fmt) {
        case WINED3DFMT_DXT1:             retVal = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT; break;
        case WINED3DFMT_DXT2:             retVal = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; break;
        case WINED3DFMT_DXT3:             retVal = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; break;
        case WINED3DFMT_DXT4:             retVal = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; break;
        case WINED3DFMT_DXT5:             retVal = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; break;
        default:
            /* stupid compiler */
            break;
        }
    }

    if (retVal == 0) {
        switch (fmt) {
        /* Paletted */
        case WINED3DFMT_P8:               retVal = GL_COLOR_INDEX; break;
        case WINED3DFMT_A8P8:             retVal = GL_COLOR_INDEX; break;
            /* Luminance */
        case WINED3DFMT_L8:               retVal = GL_LUMINANCE; break;
        case WINED3DFMT_A8L8:             retVal = GL_LUMINANCE_ALPHA; break;
        case WINED3DFMT_A4L4:             retVal = GL_LUMINANCE_ALPHA; break;
            /* Bump */
        case WINED3DFMT_V8U8:             retVal = GL_COLOR_INDEX; break;
        case WINED3DFMT_V16U16:           retVal = GL_COLOR_INDEX; break;
        case WINED3DFMT_L6V5U5:           retVal = GL_COLOR_INDEX; break;
        case WINED3DFMT_X8L8V8U8:         retVal = GL_BGRA; break;
        case WINED3DFMT_Q8W8V8U8:         retVal = GL_RGBA; break;
        case WINED3DFMT_Q16W16V16U16:     retVal = GL_COLOR_INDEX; break;
            /* color buffer */
        case WINED3DFMT_R3G3B2:           retVal = GL_RGB; break;
        case WINED3DFMT_R5G6B5:           retVal = GL_RGB; break;
        case WINED3DFMT_R8G8B8:           retVal = GL_RGB; break;
        case WINED3DFMT_A1R5G5B5:         retVal = GL_BGRA; break;
        case WINED3DFMT_X1R5G5B5:         retVal = GL_BGRA; break;
        case WINED3DFMT_A4R4G4B4:         retVal = GL_BGRA; break;
        case WINED3DFMT_X4R4G4B4:         retVal = GL_BGRA; break;
        case WINED3DFMT_A8R8G8B8:         retVal = GL_BGRA; break;
        case WINED3DFMT_A8B8G8R8:         retVal = GL_RGBA; break;
        case WINED3DFMT_A2R10G10B10:      retVal = GL_BGRA; break;
        case WINED3DFMT_X8R8G8B8:         retVal = GL_BGRA; break;
            /* to see */
        case WINED3DFMT_A8:               retVal = GL_ALPHA; break;
            /* Depth + Stencil */
        case WINED3DFMT_D24S8:            retVal = GL_COLOR_INDEX; break;
        case WINED3DFMT_D24FS8:           retVal = GL_COLOR_INDEX; break;
        case WINED3DFMT_D24X8:            retVal = GL_COLOR_INDEX; break;
        case WINED3DFMT_D24X4S4:          retVal = GL_COLOR_INDEX; break;
        case WINED3DFMT_D32:              retVal = GL_COLOR_INDEX; break;
        case WINED3DFMT_D16:              retVal = GL_COLOR_INDEX; break;
        case WINED3DFMT_D15S1:            retVal = GL_COLOR_INDEX; break;
        case WINED3DFMT_D16_LOCKABLE:     retVal = GL_COLOR_INDEX; break;
        default:
            FIXME("Unhandled fmt(%u,%s)\n", fmt, debug_d3dformat(fmt));
            retVal = GL_BGR;
        }
    }

    TRACE("fmt2glFmt for fmt(%u,%s) = %x\n", fmt, debug_d3dformat(fmt), retVal);
    return retVal;
}

GLenum D3DFmt2GLType(IWineD3DDeviceImpl* This, D3DFORMAT fmt) {
    GLenum retVal = 0;

    if (GL_SUPPORT(EXT_TEXTURE_COMPRESSION_S3TC)) {
        switch (fmt) {
        case WINED3DFMT_DXT1:             retVal = 0; break;
        case WINED3DFMT_DXT2:             retVal = 0; break;
        case WINED3DFMT_DXT3:             retVal = 0; break;
        case WINED3DFMT_DXT4:             retVal = 0; break;
        case WINED3DFMT_DXT5:             retVal = 0; break;
        default:
            /* stupid compiler */
            break;
        }
    }

    if (retVal == 0) {
        switch (fmt) {
        /* Paletted */
        case WINED3DFMT_P8:               retVal = GL_UNSIGNED_BYTE; break;
        case WINED3DFMT_A8P8:             retVal = GL_UNSIGNED_BYTE; break;
            /* Luminance */
        case WINED3DFMT_L8:               retVal = GL_UNSIGNED_BYTE; break;
        case WINED3DFMT_A8L8:             retVal = GL_UNSIGNED_BYTE; break;
        case WINED3DFMT_A4L4:             retVal = GL_UNSIGNED_BYTE; break;
            /* Bump */
        case WINED3DFMT_V8U8:             retVal = GL_UNSIGNED_BYTE; break;
        case WINED3DFMT_V16U16:           retVal = GL_UNSIGNED_SHORT; break;
        case WINED3DFMT_L6V5U5:           retVal = GL_UNSIGNED_SHORT_5_5_5_1; break;
        case WINED3DFMT_X8L8V8U8:         retVal = GL_UNSIGNED_BYTE; break;
            /* Color buffer */
        case WINED3DFMT_R3G3B2:           retVal = GL_UNSIGNED_BYTE_2_3_3_REV; break;
        case WINED3DFMT_R5G6B5:           retVal = GL_UNSIGNED_SHORT_5_6_5; break;
        case WINED3DFMT_R8G8B8:           retVal = GL_UNSIGNED_BYTE; break;
        case WINED3DFMT_A1R5G5B5:         retVal = GL_UNSIGNED_SHORT_1_5_5_5_REV; break;
        case WINED3DFMT_X1R5G5B5:         retVal = GL_UNSIGNED_SHORT_1_5_5_5_REV; break;
        case WINED3DFMT_A4R4G4B4:         retVal = GL_UNSIGNED_SHORT_4_4_4_4_REV; break;
        case WINED3DFMT_X4R4G4B4:         retVal = GL_UNSIGNED_SHORT_4_4_4_4_REV; break;
        case WINED3DFMT_A8R8G8B8:         retVal = GL_UNSIGNED_INT_8_8_8_8_REV; break;
        case WINED3DFMT_A8B8G8R8:         retVal = GL_UNSIGNED_INT_8_8_8_8_REV; break;
        case WINED3DFMT_A2R10G10B10:      retVal = GL_UNSIGNED_INT_2_10_10_10_REV; break;
        case WINED3DFMT_X8R8G8B8:         retVal = GL_UNSIGNED_INT_8_8_8_8_REV; break;
            /* to see */
        case WINED3DFMT_A8:               retVal = GL_ALPHA; break;
            /* Depth + Stencil */
        case WINED3DFMT_D24S8:            retVal = GL_UNSIGNED_INT; break;
        case WINED3DFMT_D24FS8:           retVal = GL_UNSIGNED_INT; break;
        case WINED3DFMT_D24X8:            retVal = GL_UNSIGNED_INT; break;
        case WINED3DFMT_D24X4S4:          retVal = GL_UNSIGNED_INT; break;
        case WINED3DFMT_D32:              retVal = GL_UNSIGNED_INT; break;
        case WINED3DFMT_D16:              retVal = GL_UNSIGNED_SHORT; break;
        case WINED3DFMT_D15S1:            retVal = GL_UNSIGNED_SHORT; break;
        case WINED3DFMT_D16_LOCKABLE:     retVal = GL_UNSIGNED_SHORT; break;
            /* compressed textures */
        case WINED3DFMT_DXT1:             retVal = 0; break;
        case WINED3DFMT_DXT2:             retVal = 0; break;
        case WINED3DFMT_DXT3:             retVal = 0; break;
        case WINED3DFMT_DXT4:             retVal = 0; break;
        case WINED3DFMT_DXT5:             retVal = 0; break;

        default:
            FIXME("Unhandled fmt(%u,%s)\n", fmt, debug_d3dformat(fmt));
            retVal = GL_UNSIGNED_BYTE;
        }
    }

    TRACE("fmt2glType for fmt(%u,%s) = %x\n", fmt, debug_d3dformat(fmt), retVal);
    return retVal;
}

SHORT D3DFmtGetBpp(IWineD3DDeviceImpl* This, D3DFORMAT fmt) {
    SHORT retVal;

    switch (fmt) {
    /* color buffer */
    case WINED3DFMT_R3G3B2:           retVal = 1; break;
    case WINED3DFMT_R5G6B5:           retVal = 2; break;
    case WINED3DFMT_R8G8B8:           retVal = 3; break;
    case WINED3DFMT_A1R5G5B5:         retVal = 2; break;
    case WINED3DFMT_X1R5G5B5:         retVal = 2; break;
    case WINED3DFMT_A4R4G4B4:         retVal = 2; break;
    case WINED3DFMT_X4R4G4B4:         retVal = 2; break;
    case WINED3DFMT_A8R8G8B8:         retVal = 4; break;
    case WINED3DFMT_A8B8G8R8:         retVal = 4; break;
    case WINED3DFMT_A2R10G10B10:      retVal = 4; break;
    case WINED3DFMT_X8R8G8B8:         retVal = 4; break;
        /* Paletted */
    case WINED3DFMT_P8:               retVal = 1; break;
    case WINED3DFMT_A8P8:             retVal = 2; break;
        /* depth/stencil buffer */
    case WINED3DFMT_D16_LOCKABLE:     retVal = 2; break;
    case WINED3DFMT_D16:              retVal = 2; break;
    case WINED3DFMT_D32:              retVal = 4; break;
    case WINED3DFMT_D15S1:            retVal = 2; break;
    case WINED3DFMT_D24X4S4:          retVal = 4; break;
    case WINED3DFMT_D24S8:            retVal = 4; break;
    case WINED3DFMT_D24FS8:           retVal = 4; break;
    case WINED3DFMT_D24X8:            retVal = 4; break;
        /* Luminance */
    case WINED3DFMT_L8:               retVal = 1; break;
    case WINED3DFMT_A4L4:             retVal = 1; break;
    case WINED3DFMT_A8L8:             retVal = 2; break;
        /* Bump */
    case WINED3DFMT_V8U8:             retVal = 2; break;
    case WINED3DFMT_L6V5U5:           retVal = 2; break;
    case WINED3DFMT_V16U16:           retVal = 4; break;
    case WINED3DFMT_X8L8V8U8:         retVal = 4; break;
    case WINED3DFMT_Q8W8V8U8:         retVal = 4; break;
    case WINED3DFMT_Q16W16V16U16:     retVal = 8; break;
        /* Compressed */
    case WINED3DFMT_DXT1:             retVal = 1; break; /* Actually  8 bytes per 16 pixels - Special cased later */
    case WINED3DFMT_DXT2:             retVal = 1; break; /* Actually 16 bytes per 16 pixels */
    case WINED3DFMT_DXT3:             retVal = 1; break; /* Actually 16 bytes per 16 pixels */
    case WINED3DFMT_DXT4:             retVal = 1; break; /* Actually 16 bytes per 16 pixels */
    case WINED3DFMT_DXT5:             retVal = 1; break; /* Actually 16 bytes per 16 pixels */
        /* to see */
    case WINED3DFMT_A8:               retVal = 1; break;
        /* unknown */
    case WINED3DFMT_UNKNOWN:
        /* Guess at the highest value of the above */
        TRACE("WINED3DFMT_UNKNOWN - Guessing at 4 bytes/pixel %u\n", fmt);
        retVal = 4;
        break;

    default:
        FIXME("Unhandled fmt(%u,%s)\n", fmt, debug_d3dformat(fmt));
        retVal = 4;
    }
    TRACE("bytes/Pxl for fmt(%u,%s) = %d\n", fmt, debug_d3dformat(fmt), retVal);
    return retVal;
}

/* Convertes a D3D format into a OpenGL configuration format */
int D3DFmtMakeGlCfg(D3DFORMAT BackBufferFormat, D3DFORMAT StencilBufferFormat, int *attribs, int* nAttribs, BOOL alternate){
#define PUSH1(att)        attribs[(*nAttribs)++] = (att);
#define PUSH2(att,value)  attribs[(*nAttribs)++] = (att); attribs[(*nAttribs)++] = (value);
    /*We need to do some Card specific stuff in here at some point,
    D3D now supports floating point format buffers, and there are a number of different OpelGl ways of managing these e.g.
    GLX_ATI_pixel_format_float
    */
    switch (BackBufferFormat) {
        /* color buffer */
    case WINED3DFMT_P8:
        PUSH2(GLX_RENDER_TYPE,  GLX_COLOR_INDEX_BIT);
        PUSH2(GLX_BUFFER_SIZE,  8);
        PUSH2(GLX_DOUBLEBUFFER, TRUE);
        break;

    case WINED3DFMT_R3G3B2:
        PUSH2(GLX_RENDER_TYPE,  GLX_RGBA_BIT);
        PUSH2(GLX_RED_SIZE,     3);
        PUSH2(GLX_GREEN_SIZE,   3);
        PUSH2(GLX_BLUE_SIZE,    2);
        break;

    case WINED3DFMT_A1R5G5B5:
        PUSH2(GLX_ALPHA_SIZE,   1);
    case WINED3DFMT_X1R5G5B5:
        PUSH2(GLX_RED_SIZE,     5);
        PUSH2(GLX_GREEN_SIZE,   5);
        PUSH2(GLX_BLUE_SIZE,    5);
        break;

    case WINED3DFMT_R5G6B5:
        PUSH2(GLX_RED_SIZE,     5);
        PUSH2(GLX_GREEN_SIZE,   6);
        PUSH2(GLX_BLUE_SIZE,    5);
        break;

    case WINED3DFMT_A4R4G4B4:
        PUSH2(GLX_ALPHA_SIZE,   4);
    case WINED3DFMT_X4R4G4B4:
        PUSH2(GLX_RED_SIZE,     4);
        PUSH2(GLX_GREEN_SIZE,   4);
        PUSH2(GLX_BLUE_SIZE,    4);
        break;

    case WINED3DFMT_A8R8G8B8:
        PUSH2(GLX_ALPHA_SIZE,   8);
    case WINED3DFMT_R8G8B8:
    case WINED3DFMT_X8R8G8B8:
        PUSH2(GLX_RED_SIZE,     8);
        PUSH2(GLX_GREEN_SIZE,   8);
        PUSH2(GLX_BLUE_SIZE,    8);
        break;

    case WINED3DFMT_A2R10G10B10:
        PUSH2(GLX_ALPHA_SIZE,   2);
        PUSH2(GLX_RED_SIZE,    10);
        PUSH2(GLX_GREEN_SIZE,  10);
        PUSH2(GLX_BLUE_SIZE,   10);
        break;

    default:
        break;
    }
    if(!alternate){
        switch (StencilBufferFormat) {
    case WINED3DFMT_D16_LOCKABLE:
    case WINED3DFMT_D16:
        PUSH2(GLX_DEPTH_SIZE,   16);
        break;

    case WINED3DFMT_D15S1:
        PUSH2(GLX_DEPTH_SIZE,   15);
        PUSH2(GLX_STENCIL_SIZE, 1);
        /*Does openGl support a 1bit stencil?, I've seen it used elsewhere
        e.g. http://www.ks.uiuc.edu/Research/vmd/doxygen/OpenGLDisplayDevice_8C-source.html*/
        break;

    case WINED3DFMT_D24X8:
        PUSH2(GLX_DEPTH_SIZE,   24);
        break;

    case WINED3DFMT_D24X4S4:
        PUSH2(GLX_DEPTH_SIZE,   24);
        PUSH2(GLX_STENCIL_SIZE, 4);
        break;

    case WINED3DFMT_D24S8:
        PUSH2(GLX_DEPTH_SIZE,   24);
        PUSH2(GLX_STENCIL_SIZE, 8);
        break;

    case WINED3DFMT_D24FS8:
        PUSH2(GLX_DEPTH_SIZE,   24);
        PUSH2(GLX_STENCIL_SIZE, 8);
        break;

    case WINED3DFMT_D32:
        PUSH2(GLX_DEPTH_SIZE,   32);
        break;

    default:
        break;
    }

    } else { /* it the device doesn't support the 'exact' format, try to find something close */
        switch (StencilBufferFormat) {
        case WINED3DFMT_D16_LOCKABLE:
        case WINED3DFMT_D16:
            PUSH2(GLX_DEPTH_SIZE,   1);
            break;

        case WINED3DFMT_D15S1:
            PUSH2(GLX_DEPTH_SIZE,   1);
            PUSH2(GLX_STENCIL_SIZE, 1);
            /*Does openGl support a 1bit stencil?, I've seen it used elsewhere
            e.g. http://www.ks.uiuc.edu/Research/vmd/doxygen/OpenGLDisplayDevice_8C-source.html*/
            break;

        case WINED3DFMT_D24X8:
            PUSH2(GLX_DEPTH_SIZE,   1);
            break;

        case WINED3DFMT_D24X4S4:
            PUSH2(GLX_DEPTH_SIZE,   1);
            PUSH2(GLX_STENCIL_SIZE, 1);
            break;

        case WINED3DFMT_D24S8:
            PUSH2(GLX_DEPTH_SIZE,   1);
            PUSH2(GLX_STENCIL_SIZE, 1);
            break;

        case WINED3DFMT_D24FS8:
            PUSH2(GLX_DEPTH_SIZE,   1);
            PUSH2(GLX_STENCIL_SIZE, 1);
            break;

        case WINED3DFMT_D32:
            PUSH2(GLX_DEPTH_SIZE,   1);
            break;

        default:
            break;
        }
    }

    return *nAttribs;
}

#undef GLINFO_LOCATION
