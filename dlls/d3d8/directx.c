/*
 * IDirect3D8 implementation
 *
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2003-2004 Raphael Junqueira
 * Copyright 2004 Christian Costa
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

#include <stdarg.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/debug.h"
#include "wine/unicode.h"

#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(d3d_caps);

/* x11drv GDI escapes */
#define X11DRV_ESCAPE 6789
enum x11drv_escape_codes
{
    X11DRV_GET_DISPLAY,   /* get X11 display for a DC */
    X11DRV_GET_DRAWABLE,  /* get current drawable for a DC */
    X11DRV_GET_FONT,      /* get current X font for a DC */
};

#define NUM_FORMATS 7
static const D3DFORMAT device_formats[NUM_FORMATS] = {
  D3DFMT_P8,
  D3DFMT_R3G3B2,
  D3DFMT_R5G6B5, 
  D3DFMT_X1R5G5B5,
  D3DFMT_X4R4G4B4,
  D3DFMT_R8G8B8,
  D3DFMT_X8R8G8B8
};

static void IDirect3D8Impl_FillGLCaps(LPDIRECT3D8 iface, Display* display);


/* retrieve the X display to use on a given DC */
inline static Display *get_display( HDC hdc )
{
    Display *display;
    enum x11drv_escape_codes escape = X11DRV_GET_DISPLAY;

    if (!ExtEscape( hdc, X11DRV_ESCAPE, sizeof(escape), (LPCSTR)&escape,
                    sizeof(display), (LPSTR)&display )) display = NULL;
    return display;
}

/* IDirect3D IUnknown parts follow: */
HRESULT WINAPI IDirect3D8Impl_QueryInterface(LPDIRECT3D8 iface,REFIID riid,LPVOID *ppobj)
{
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3D8)) {
        IDirect3D8Impl_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IDirect3D8Impl_AddRef(LPDIRECT3D8 iface) {
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef from %ld\n", This, ref - 1);

    return ref;
}

ULONG WINAPI IDirect3D8Impl_Release(LPDIRECT3D8 iface) {
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %ld\n", This, ref);

    if (ref == 0) {
        IWineD3D_Release(This->WineD3D);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3D Interface follow: */
HRESULT WINAPI IDirect3D8Impl_RegisterSoftwareDevice (LPDIRECT3D8 iface, void* pInitializeFunction) {
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    return IWineD3D_RegisterSoftwareDevice(This->WineD3D, pInitializeFunction);
}

UINT WINAPI IDirect3D8Impl_GetAdapterCount (LPDIRECT3D8 iface) {
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    return IWineD3D_GetAdapterCount(This->WineD3D);
}

HRESULT  WINAPI  IDirect3D8Impl_GetAdapterIdentifier       (LPDIRECT3D8 iface,
                                                            UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER8* pIdentifier) {
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    WINED3DADAPTER_IDENTIFIER adapter_id;

    /* dx8 and dx9 have different structures to be filled in, with incompatible 
       layouts so pass in pointers to the places to be filled via an internal 
       structure                                                                */
    adapter_id.Driver           = pIdentifier->Driver;          
    adapter_id.Description      = pIdentifier->Description;     
    adapter_id.DeviceName       = NULL;      
    adapter_id.DriverVersion    = &pIdentifier->DriverVersion;   
    adapter_id.VendorId         = &pIdentifier->VendorId;        
    adapter_id.DeviceId         = &pIdentifier->DeviceId;        
    adapter_id.SubSysId         = &pIdentifier->SubSysId;        
    adapter_id.Revision         = &pIdentifier->Revision;        
    adapter_id.DeviceIdentifier = &pIdentifier->DeviceIdentifier;
    adapter_id.WHQLLevel        = &pIdentifier->WHQLLevel;       

    return IWineD3D_GetAdapterIdentifier(This->WineD3D, Adapter, Flags, &adapter_id);
}

UINT WINAPI IDirect3D8Impl_GetAdapterModeCount (LPDIRECT3D8 iface,UINT Adapter) {
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    return IWineD3D_GetAdapterModeCount(This->WineD3D, Adapter, D3DFMT_UNKNOWN);
}

HRESULT WINAPI IDirect3D8Impl_EnumAdapterModes (LPDIRECT3D8 iface, UINT Adapter, UINT Mode, D3DDISPLAYMODE* pMode) {
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    return IWineD3D_EnumAdapterModes(This->WineD3D, Adapter, D3DFMT_UNKNOWN, Mode, pMode);
}

HRESULT WINAPI IDirect3D8Impl_GetAdapterDisplayMode (LPDIRECT3D8 iface, UINT Adapter, D3DDISPLAYMODE* pMode) {
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    return IWineD3D_GetAdapterDisplayMode(This->WineD3D, Adapter, pMode);
}

HRESULT  WINAPI  IDirect3D8Impl_CheckDeviceType            (LPDIRECT3D8 iface,
                                                            UINT Adapter, D3DDEVTYPE CheckType, D3DFORMAT DisplayFormat,
                                                            D3DFORMAT BackBufferFormat, BOOL Windowed) {
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    return IWineD3D_CheckDeviceType(This->WineD3D, Adapter, CheckType, DisplayFormat,
                                    BackBufferFormat, Windowed);
}

HRESULT  WINAPI  IDirect3D8Impl_CheckDeviceFormat          (LPDIRECT3D8 iface,
                                                            UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat,
                                                            DWORD Usage, D3DRESOURCETYPE RType, D3DFORMAT CheckFormat) {
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    return IWineD3D_CheckDeviceFormat(This->WineD3D, Adapter, DeviceType, AdapterFormat,
                                    Usage, RType, CheckFormat);
}

HRESULT  WINAPI  IDirect3D8Impl_CheckDeviceMultiSampleType(LPDIRECT3D8 iface,
							   UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SurfaceFormat,
							   BOOL Windowed, D3DMULTISAMPLE_TYPE MultiSampleType) {
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    return IWineD3D_CheckDeviceMultiSampleType(This->WineD3D, Adapter, DeviceType, SurfaceFormat,
                                               Windowed, MultiSampleType, NULL);
}

HRESULT  WINAPI  IDirect3D8Impl_CheckDepthStencilMatch(LPDIRECT3D8 iface, 
						       UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat,
						       D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat) {
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    return IWineD3D_CheckDepthStencilMatch(This->WineD3D, Adapter, DeviceType, AdapterFormat,
                                           RenderTargetFormat, DepthStencilFormat);
}

HRESULT  WINAPI  IDirect3D8Impl_GetDeviceCaps(LPDIRECT3D8 iface, UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS8* pCaps) {
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    HRESULT hrc = D3D_OK;
    WINED3DCAPS *pWineCaps;

    TRACE("(%p) Relay %d %u %p \n", This, Adapter, DeviceType, pCaps);

    if(NULL == pCaps){
        return D3DERR_INVALIDCALL;
    }
    pWineCaps = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WINED3DCAPS));
    if(pWineCaps == NULL){
        return D3DERR_INVALIDCALL; /*well this is what MSDN says to return*/
    }
    D3D8CAPSTOWINECAPS(pCaps, pWineCaps)
    hrc = IWineD3D_GetDeviceCaps(This->WineD3D, Adapter, DeviceType, pWineCaps);
    HeapFree(GetProcessHeap(), 0, pWineCaps);
    TRACE("(%p) returning %p\n", This, pCaps);
    return hrc;
}

HMONITOR WINAPI  IDirect3D8Impl_GetAdapterMonitor(LPDIRECT3D8 iface, UINT Adapter) {
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    return IWineD3D_GetAdapterMonitor(This->WineD3D, Adapter);
}

static void IDirect3D8Impl_FillGLCaps(LPDIRECT3D8 iface, Display* display) {
    const char *GL_Extensions = NULL;
    const char *GLX_Extensions = NULL;
    GLint gl_max;
    const char* gl_string = NULL;
    const char* gl_string_cursor = NULL;
    Bool test = 0;
    int major, minor;
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;

    if (This->gl_info.bIsFilled) return;
    This->gl_info.bIsFilled = 1;

    TRACE_(d3d_caps)("(%p, %p)\n", This, display);

    if (NULL != display) {
      test = glXQueryVersion(display, &major, &minor);
      This->gl_info.glx_version = ((major & 0x0000FFFF) << 16) | (minor & 0x0000FFFF);
      gl_string = glXGetClientString(display, GLX_VENDOR);
    } else {
      gl_string = glGetString(GL_VENDOR);
    }
    
    if (strstr(gl_string, "NVIDIA")) {
      This->gl_info.gl_vendor = VENDOR_NVIDIA;
    } else if (strstr(gl_string, "ATI")) {
      This->gl_info.gl_vendor = VENDOR_ATI;
    } else {
      This->gl_info.gl_vendor = VENDOR_WINE;
    }
   
    TRACE_(d3d_caps)("found GL_VENDOR (%s)->(0x%04x)\n", debugstr_a(gl_string), This->gl_info.gl_vendor);
    
    gl_string = glGetString(GL_VERSION);
    switch (This->gl_info.gl_vendor) {
    case VENDOR_NVIDIA:
      gl_string_cursor = strstr(gl_string, "NVIDIA");
      gl_string_cursor = strstr(gl_string_cursor, " ");
      while (*gl_string_cursor && ' ' == *gl_string_cursor) ++gl_string_cursor;
      if (*gl_string_cursor) {
	char tmp[16];
	int cursor = 0;

	while (*gl_string_cursor <= '9' && *gl_string_cursor >= '0') {
	  tmp[cursor++] = *gl_string_cursor;
	  ++gl_string_cursor;
	}
	tmp[cursor] = 0;
	major = atoi(tmp);
	
	if (*gl_string_cursor != '.') WARN_(d3d_caps)("malformed GL_VERSION (%s)\n", debugstr_a(gl_string));
	++gl_string_cursor;

	while (*gl_string_cursor <= '9' && *gl_string_cursor >= '0') {
	  tmp[cursor++] = *gl_string_cursor;
	  ++gl_string_cursor;
	}
	tmp[cursor] = 0;
	minor = atoi(tmp);
	break;
      }
    case VENDOR_ATI:
      major = minor = 0;
      gl_string_cursor = strchr(gl_string, '-');
      if (gl_string_cursor) {
        int error = 0;
        gl_string_cursor++;
	
        /* Check if version number is of the form x.y.z */
        if (*gl_string_cursor > '9' && *gl_string_cursor < '0')
          error = 1;
        if (!error && *(gl_string_cursor+2) > '9' && *(gl_string_cursor+2) < '0')
          error = 1;
        if (!error && *(gl_string_cursor+4) > '9' && *(gl_string_cursor+4) < '0')
          error = 1;
	if (!error && *(gl_string_cursor+1) != '.' && *(gl_string_cursor+3) != '.')
          error = 1;
        /* Mark version number as malformed */
        if (error)
          gl_string_cursor = 0;
      }
      if (!gl_string_cursor)
        WARN_(d3d_caps)("malformed GL_VERSION (%s)\n", debugstr_a(gl_string));
      else {
        major = *gl_string_cursor - '0';
        minor = (*(gl_string_cursor+2) - '0') * 256 + (*(gl_string_cursor+4) - '0');
      }      
      break;
    default:
      major = 0;
      minor = 9;
    }
    This->gl_info.gl_driver_version = MAKEDWORD_VERSION(major, minor);

    FIXME_(d3d_caps)("found GL_VERSION  (%s)->(0x%08lx)\n", debugstr_a(gl_string), This->gl_info.gl_driver_version);

    gl_string = glGetString(GL_RENDERER);
    strcpy(This->gl_info.gl_renderer, gl_string);

    switch (This->gl_info.gl_vendor) {
    case VENDOR_NVIDIA:
      if (strstr(This->gl_info.gl_renderer, "GeForce4 Ti")) {
	This->gl_info.gl_card = CARD_NVIDIA_GEFORCE4_TI4600;
      } else if (strstr(This->gl_info.gl_renderer, "GeForceFX")) {
	This->gl_info.gl_card = CARD_NVIDIA_GEFORCEFX_5900ULTRA;
      } else {
	This->gl_info.gl_card = CARD_NVIDIA_GEFORCE4_TI4600;
      }
      break;
    case VENDOR_ATI:
      if (strstr(This->gl_info.gl_renderer, "RADEON 9800 PRO")) {
        This->gl_info.gl_card = CARD_ATI_RADEON_9800PRO;
      } else if (strstr(This->gl_info.gl_renderer, "RADEON 9700 PRO")) {
        This->gl_info.gl_card = CARD_ATI_RADEON_9700PRO;
      } else {
        This->gl_info.gl_card = CARD_ATI_RADEON_8500;
      }
      break;
    default:
      This->gl_info.gl_card = CARD_WINE;
      break;
    }

    FIXME_(d3d_caps)("found GL_RENDERER (%s)->(0x%04x)\n", debugstr_a(This->gl_info.gl_renderer), This->gl_info.gl_card);

    /*
     * Initialize openGL extension related variables
     *  with Default values
     */
    memset(&This->gl_info.supported, 0, sizeof(This->gl_info.supported));
    This->gl_info.max_textures   = 1;
    This->gl_info.ps_arb_version = PS_VERSION_NOT_SUPPORTED;
    This->gl_info.vs_arb_version = VS_VERSION_NOT_SUPPORTED;
    This->gl_info.vs_nv_version  = VS_VERSION_NOT_SUPPORTED;
    This->gl_info.vs_ati_version = VS_VERSION_NOT_SUPPORTED;

#define USE_GL_FUNC(type, pfn) This->gl_info.pfn = NULL;
    GL_EXT_FUNCS_GEN;
#undef USE_GL_FUNC

    /* Retrieve opengl defaults */
    glGetIntegerv(GL_MAX_CLIP_PLANES, &gl_max);
    This->gl_info.max_clipplanes = min(MAX_CLIPPLANES, gl_max);
    TRACE_(d3d_caps)("ClipPlanes support - num Planes=%d\n", gl_max);

    glGetIntegerv(GL_MAX_LIGHTS, &gl_max);
    This->gl_info.max_lights = gl_max;
    TRACE_(d3d_caps)("Lights support - max lights=%d\n", gl_max);

    /* Parse the gl supported features, in theory enabling parts of our code appropriately */
    GL_Extensions = glGetString(GL_EXTENSIONS);
    TRACE_(d3d_caps)("GL_Extensions reported:\n");  
    
    if (NULL == GL_Extensions) {
      ERR("   GL_Extensions returns NULL\n");      
    } else {
      while (*GL_Extensions != 0x00) {
        const char *Start = GL_Extensions;
        char ThisExtn[256];

        memset(ThisExtn, 0x00, sizeof(ThisExtn));
        while (*GL_Extensions != ' ' && *GL_Extensions != 0x00) {
	  GL_Extensions++;
        }
        memcpy(ThisExtn, Start, (GL_Extensions - Start));
        TRACE_(d3d_caps)("- %s\n", ThisExtn);

	/**
	 * ARB 
	 */
	if (strcmp(ThisExtn, "GL_ARB_fragment_program") == 0) {
	  This->gl_info.ps_arb_version = PS_VERSION_11;
	  TRACE_(d3d_caps)(" FOUND: ARB Pixel Shader support - version=%02x\n", This->gl_info.ps_arb_version);
	  This->gl_info.supported[ARB_FRAGMENT_PROGRAM] = TRUE;
        } else if (strcmp(ThisExtn, "GL_ARB_multisample") == 0) {
	  TRACE_(d3d_caps)(" FOUND: ARB Multisample support\n");
	  This->gl_info.supported[ARB_MULTISAMPLE] = TRUE;
	} else if (strcmp(ThisExtn, "GL_ARB_multitexture") == 0) {
	  glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &gl_max);
	  TRACE_(d3d_caps)(" FOUND: ARB Multitexture support - GL_MAX_TEXTURE_UNITS_ARB=%u\n", gl_max);
	  This->gl_info.supported[ARB_MULTITEXTURE] = TRUE;
	  This->gl_info.max_textures = min(8, gl_max);
        } else if (strcmp(ThisExtn, "GL_ARB_texture_cube_map") == 0) {
	  TRACE_(d3d_caps)(" FOUND: ARB Texture Cube Map support\n");
	  This->gl_info.supported[ARB_TEXTURE_CUBE_MAP] = TRUE;
	  TRACE_(d3d_caps)(" IMPLIED: NVIDIA (NV) Texture Gen Reflection support\n");
	  This->gl_info.supported[NV_TEXGEN_REFLECTION] = TRUE;
        } else if (strcmp(ThisExtn, "GL_ARB_texture_compression") == 0) {
	  TRACE_(d3d_caps)(" FOUND: ARB Texture Compression support\n");
	  This->gl_info.supported[ARB_TEXTURE_COMPRESSION] = TRUE;
        } else if (strcmp(ThisExtn, "GL_ARB_texture_env_add") == 0) {
	  TRACE_(d3d_caps)(" FOUND: ARB Texture Env Add support\n");
	  This->gl_info.supported[ARB_TEXTURE_ENV_ADD] = TRUE;
        } else if (strcmp(ThisExtn, "GL_ARB_texture_env_combine") == 0) {
	  TRACE_(d3d_caps)(" FOUND: ARB Texture Env combine support\n");
	  This->gl_info.supported[ARB_TEXTURE_ENV_COMBINE] = TRUE;
        } else if (strcmp(ThisExtn, "GL_ARB_texture_env_dot3") == 0) {
	  TRACE_(d3d_caps)(" FOUND: ARB Dot3 support\n");
	  This->gl_info.supported[ARB_TEXTURE_ENV_DOT3] = TRUE;
	} else if (strcmp(ThisExtn, "GL_ARB_texture_border_clamp") == 0) {
	  TRACE_(d3d_caps)(" FOUND: ARB Texture border clamp support\n");
	  This->gl_info.supported[ARB_TEXTURE_BORDER_CLAMP] = TRUE;
	} else if (strcmp(ThisExtn, "GL_ARB_texture_mirrored_repeat") == 0) {
	  TRACE_(d3d_caps)(" FOUND: ARB Texture mirrored repeat support\n");
	  This->gl_info.supported[ARB_TEXTURE_MIRRORED_REPEAT] = TRUE;
	} else if (strstr(ThisExtn, "GL_ARB_vertex_program")) {
	  This->gl_info.vs_arb_version = VS_VERSION_11;
	  TRACE_(d3d_caps)(" FOUND: ARB Vertex Shader support - version=%02x\n", This->gl_info.vs_arb_version);
	  This->gl_info.supported[ARB_VERTEX_PROGRAM] = TRUE;

	/**
	 * EXT
	 */
        } else if (strcmp(ThisExtn, "GL_EXT_fog_coord") == 0) {
	  TRACE_(d3d_caps)(" FOUND: EXT Fog coord support\n");
	  This->gl_info.supported[EXT_FOG_COORD] = TRUE;
        } else if (strcmp(ThisExtn, "GL_EXT_paletted_texture") == 0) { /* handle paletted texture extensions */
	  TRACE_(d3d_caps)(" FOUND: EXT Paletted texture support\n");
	  This->gl_info.supported[EXT_PALETTED_TEXTURE] = TRUE;
        } else if (strcmp(ThisExtn, "GL_EXT_point_parameters") == 0) {
	  TRACE_(d3d_caps)(" FOUND: EXT Point parameters support\n");
	  This->gl_info.supported[EXT_POINT_PARAMETERS] = TRUE;
	} else if (strcmp(ThisExtn, "GL_EXT_secondary_color") == 0) {
	  TRACE_(d3d_caps)(" FOUND: EXT Secondary coord support\n");
	  This->gl_info.supported[EXT_SECONDARY_COLOR] = TRUE;
	} else if (strcmp(ThisExtn, "GL_EXT_stencil_wrap") == 0) {
	  TRACE_(d3d_caps)(" FOUND: EXT Stencil wrap support\n");
	  This->gl_info.supported[EXT_STENCIL_WRAP] = TRUE;
	} else if (strcmp(ThisExtn, "GL_EXT_texture_compression_s3tc") == 0) {
	  TRACE_(d3d_caps)(" FOUND: EXT Texture S3TC compression support\n");
	  This->gl_info.supported[EXT_TEXTURE_COMPRESSION_S3TC] = TRUE;
        } else if (strcmp(ThisExtn, "GL_EXT_texture_env_add") == 0) {
	  TRACE_(d3d_caps)(" FOUND: EXT Texture Env Add support\n");
	  This->gl_info.supported[EXT_TEXTURE_ENV_ADD] = TRUE;
        } else if (strcmp(ThisExtn, "GL_EXT_texture_env_combine") == 0) {
	  TRACE_(d3d_caps)(" FOUND: EXT Texture Env combine support\n");
	  This->gl_info.supported[EXT_TEXTURE_ENV_COMBINE] = TRUE;
        } else if (strcmp(ThisExtn, "GL_EXT_texture_env_dot3") == 0) {
	  TRACE_(d3d_caps)(" FOUND: EXT Dot3 support\n");
	  This->gl_info.supported[EXT_TEXTURE_ENV_DOT3] = TRUE;
	} else if (strcmp(ThisExtn, "GL_EXT_texture_filter_anisotropic") == 0) {
	  TRACE_(d3d_caps)(" FOUND: EXT Texture Anisotropic filter support\n");
	  This->gl_info.supported[EXT_TEXTURE_FILTER_ANISOTROPIC] = TRUE;
	} else if (strcmp(ThisExtn, "GL_EXT_texture_lod") == 0) {
	  TRACE_(d3d_caps)(" FOUND: EXT Texture LOD support\n");
	  This->gl_info.supported[EXT_TEXTURE_LOD] = TRUE;
	} else if (strcmp(ThisExtn, "GL_EXT_texture_lod_bias") == 0) {
	  TRACE_(d3d_caps)(" FOUND: EXT Texture LOD bias support\n");
	  This->gl_info.supported[EXT_TEXTURE_LOD_BIAS] = TRUE;
	} else if (strcmp(ThisExtn, "GL_EXT_vertex_weighting") == 0) {
	  TRACE_(d3d_caps)(" FOUND: EXT Vertex weighting support\n");
	  This->gl_info.supported[EXT_VERTEX_WEIGHTING] = TRUE;

	/**
	 * NVIDIA 
	 */
	} else if (strstr(ThisExtn, "GL_NV_fog_distance")) {
	  TRACE_(d3d_caps)(" FOUND: NVIDIA (NV) Fog Distance support\n");
	  This->gl_info.supported[NV_FOG_DISTANCE] = TRUE;
	} else if (strstr(ThisExtn, "GL_NV_fragment_program")) {
	  This->gl_info.ps_nv_version = PS_VERSION_11;
	  TRACE_(d3d_caps)(" FOUND: NVIDIA (NV) Pixel Shader support - version=%02x\n", This->gl_info.ps_nv_version);
        } else if (strcmp(ThisExtn, "GL_NV_register_combiners") == 0) {
	  TRACE_(d3d_caps)(" FOUND: NVIDIA (NV) Register combiners (1) support\n");
	  This->gl_info.supported[NV_REGISTER_COMBINERS] = TRUE;
        } else if (strcmp(ThisExtn, "GL_NV_register_combiners2") == 0) {
	  TRACE_(d3d_caps)(" FOUND: NVIDIA (NV) Register combiners (2) support\n");
	  This->gl_info.supported[NV_REGISTER_COMBINERS2] = TRUE;
        } else if (strcmp(ThisExtn, "GL_NV_texgen_reflection") == 0) {
	  TRACE_(d3d_caps)(" FOUND: NVIDIA (NV) Texture Gen Reflection support\n");
	  This->gl_info.supported[NV_TEXGEN_REFLECTION] = TRUE;
        } else if (strcmp(ThisExtn, "GL_NV_texture_env_combine4") == 0) {
	  TRACE_(d3d_caps)(" FOUND: NVIDIA (NV) Texture Env combine (4) support\n");
	  This->gl_info.supported[NV_TEXTURE_ENV_COMBINE4] = TRUE;
        } else if (strcmp(ThisExtn, "GL_NV_texture_shader") == 0) {
	  TRACE_(d3d_caps)(" FOUND: NVIDIA (NV) Texture Shader (1) support\n");
	  This->gl_info.supported[NV_TEXTURE_SHADER] = TRUE;
        } else if (strcmp(ThisExtn, "GL_NV_texture_shader2") == 0) {
	  TRACE_(d3d_caps)(" FOUND: NVIDIA (NV) Texture Shader (2) support\n");
	  This->gl_info.supported[NV_TEXTURE_SHADER2] = TRUE;
        } else if (strcmp(ThisExtn, "GL_NV_texture_shader3") == 0) {
	  TRACE_(d3d_caps)(" FOUND: NVIDIA (NV) Texture Shader (3) support\n");
	  This->gl_info.supported[NV_TEXTURE_SHADER3] = TRUE;
	} else if (strstr(ThisExtn, "GL_NV_vertex_program")) {
	  This->gl_info.vs_nv_version = max(This->gl_info.vs_nv_version, (0 == strcmp(ThisExtn, "GL_NV_vertex_program1_1")) ? VS_VERSION_11 : VS_VERSION_10);
	  This->gl_info.vs_nv_version = max(This->gl_info.vs_nv_version, (0 == strcmp(ThisExtn, "GL_NV_vertex_program2"))   ? VS_VERSION_20 : VS_VERSION_10);
	  TRACE_(d3d_caps)(" FOUND: NVIDIA (NV) Vertex Shader support - version=%02x\n", This->gl_info.vs_nv_version);
	  This->gl_info.supported[NV_VERTEX_PROGRAM] = TRUE;

	/**
	 * ATI
	 */
	/** TODO */
        } else if (strcmp(ThisExtn, "GL_ATI_texture_env_combine3") == 0) {
	  TRACE_(d3d_caps)(" FOUND: ATI Texture Env combine (3) support\n");
	  This->gl_info.supported[ATI_TEXTURE_ENV_COMBINE3] = TRUE;
        } else if (strcmp(ThisExtn, "GL_ATI_texture_mirror_once") == 0) {
	  TRACE_(d3d_caps)(" FOUND: ATI Texture Mirror Once support\n");
	  This->gl_info.supported[ATI_TEXTURE_MIRROR_ONCE] = TRUE;
	} else if (strcmp(ThisExtn, "GL_EXT_vertex_shader") == 0) {
	  This->gl_info.vs_ati_version = VS_VERSION_11;
	  TRACE_(d3d_caps)(" FOUND: ATI (EXT) Vertex Shader support - version=%02x\n", This->gl_info.vs_ati_version);
	  This->gl_info.supported[EXT_VERTEX_SHADER] = TRUE;
	}


        if (*GL_Extensions == ' ') GL_Extensions++;
      }
    }

#define USE_GL_FUNC(type, pfn) This->gl_info.pfn = (type) glXGetProcAddressARB( (const GLubyte *) #pfn);
    GL_EXT_FUNCS_GEN;
#undef USE_GL_FUNC

    if (display != NULL) {
        GLX_Extensions = glXQueryExtensionsString(display, DefaultScreen(display));
        TRACE_(d3d_caps)("GLX_Extensions reported:\n");  
    
        if (NULL == GLX_Extensions) {
          ERR("   GLX_Extensions returns NULL\n");      
        } else {
          while (*GLX_Extensions != 0x00) {
            const char *Start = GLX_Extensions;
            char ThisExtn[256];
           
            memset(ThisExtn, 0x00, sizeof(ThisExtn));
            while (*GLX_Extensions != ' ' && *GLX_Extensions != 0x00) {
              GLX_Extensions++;
            }
            memcpy(ThisExtn, Start, (GLX_Extensions - Start));
            TRACE_(d3d_caps)("- %s\n", ThisExtn);
            if (*GLX_Extensions == ' ') GLX_Extensions++;
          }
        }
    }

#define USE_GL_FUNC(type, pfn) This->gl_info.pfn = (type) glXGetProcAddressARB( (const GLubyte *) #pfn);
    GLX_EXT_FUNCS_GEN;
#undef USE_GL_FUNC

    /* Only save the values obtained when a display is provided */
    if (display != NULL) This->isGLInfoValid = TRUE;

}

/* Internal function called back during the CreateDevice to create a render target */
HRESULT WINAPI D3D8CB_CreateRenderTarget(IUnknown *device, UINT Width, UINT Height, 
                                         WINED3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, 
                                         DWORD MultisampleQuality, BOOL Lockable, 
                                         IWineD3DSurface** ppSurface, HANDLE* pSharedHandle) {
    HRESULT res = D3D_OK;
    IDirect3DSurface8Impl *d3dSurface = NULL;

    /* Note - Throw away MultisampleQuality and SharedHandle - only relevant for d3d9  */
    res = IDirect3DDevice8_CreateRenderTarget((IDirect3DDevice8 *)device, Width, Height, 
                                         (D3DFORMAT)Format, MultiSample, Lockable, 
                                         (IDirect3DSurface8 **)&d3dSurface);
    if (res == D3D_OK) {
        *ppSurface = d3dSurface->wineD3DSurface;
    }
    return res;
}

/* Callback for creating the inplicite swapchain when the device is created */
HRESULT WINAPI D3D8CB_CreateAdditionalSwapChain(IUnknown *device,
                                                WINED3DPRESENT_PARAMETERS* pPresentationParameters,
                                                IWineD3DSwapChain ** ppSwapChain){
    HRESULT res = D3D_OK;
    IDirect3DSwapChain8Impl *d3dSwapChain = NULL;
    /* We have to pass the presentation parameters back and forth */
    D3DPRESENT_PARAMETERS localParameters;
    localParameters.BackBufferWidth                = *(pPresentationParameters->BackBufferWidth);
    localParameters.BackBufferHeight               = *(pPresentationParameters->BackBufferHeight);
    localParameters.BackBufferFormat               = *(pPresentationParameters->BackBufferFormat);
    localParameters.BackBufferCount                = *(pPresentationParameters->BackBufferCount);
    localParameters.MultiSampleType                = *(pPresentationParameters->MultiSampleType);
  /* d3d9 only */
  /* localParameters.MultiSampleQuality             = *(pPresentationParameters->MultiSampleQuality); */
    localParameters.SwapEffect                     = *(pPresentationParameters->SwapEffect);
    localParameters.hDeviceWindow                  = *(pPresentationParameters->hDeviceWindow);
    localParameters.Windowed                       = *(pPresentationParameters->Windowed);
    localParameters.EnableAutoDepthStencil         = *(pPresentationParameters->EnableAutoDepthStencil);
    localParameters.AutoDepthStencilFormat         = *(pPresentationParameters->AutoDepthStencilFormat);
    localParameters.Flags                          = *(pPresentationParameters->Flags);
    localParameters.FullScreen_RefreshRateInHz     = *(pPresentationParameters->FullScreen_RefreshRateInHz);
/* not in d3d8 */
/*    localParameters.PresentationInterval           = *(pPresentationParameters->PresentationInterval); */

    TRACE("(%p) rellaying\n", device);
    /*copy the presentation parameters*/
    res = IDirect3DDevice8_CreateAdditionalSwapChain((IDirect3DDevice8 *)device, &localParameters, (IDirect3DSwapChain8 **)&d3dSwapChain);

    if (res == D3D_OK){
        *ppSwapChain = d3dSwapChain->wineD3DSwapChain;
    } else {
        FIXME("failed to create additional swap chain\n");
        *ppSwapChain = NULL;
    }
    /* Copy back the presentation parameters */
    TRACE("(%p) setting up return parameters\n", device);
   *pPresentationParameters->BackBufferWidth               = localParameters.BackBufferWidth;
   *pPresentationParameters->BackBufferHeight              = localParameters.BackBufferHeight;
   *pPresentationParameters->BackBufferFormat              = localParameters.BackBufferFormat;
   *pPresentationParameters->BackBufferCount               = localParameters.BackBufferCount;
   *pPresentationParameters->MultiSampleType               = localParameters.MultiSampleType;
/*   *pPresentationParameters->MultiSampleQuality            leave alone in case wined3d set something internally */
   *pPresentationParameters->SwapEffect                    = localParameters.SwapEffect;
   *pPresentationParameters->hDeviceWindow                 = localParameters.hDeviceWindow;
   *pPresentationParameters->Windowed                      = localParameters.Windowed;
   *pPresentationParameters->EnableAutoDepthStencil        = localParameters.EnableAutoDepthStencil;
   *pPresentationParameters->AutoDepthStencilFormat        = localParameters.AutoDepthStencilFormat;
   *pPresentationParameters->Flags                         = localParameters.Flags;
   *pPresentationParameters->FullScreen_RefreshRateInHz    = localParameters.FullScreen_RefreshRateInHz;
/*   *pPresentationParameters->PresentationInterval          leave alone in case wined3d set something internally */

   return res;
}

HRESULT  WINAPI  IDirect3D8Impl_CreateDevice               (LPDIRECT3D8 iface,
                                                            UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow,
                                                            DWORD BehaviourFlags, D3DPRESENT_PARAMETERS* pPresentationParameters,
                                                            IDirect3DDevice8** ppReturnedDeviceInterface) {
    IDirect3DDevice8Impl *object;
    HWND whichHWND;
    int num;
    XVisualInfo template;
    HDC hDc;
    WINED3DPRESENT_PARAMETERS localParameters;

    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    TRACE("(%p)->(Adptr:%d, DevType: %x, FocusHwnd: %p, BehFlags: %lx, PresParms: %p, RetDevInt: %p)\n", This, Adapter, DeviceType,
          hFocusWindow, BehaviourFlags, pPresentationParameters, ppReturnedDeviceInterface);

    if (Adapter >= IDirect3D8Impl_GetAdapterCount(iface)) {
        return D3DERR_INVALIDCALL;
    }

    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DDevice8Impl));
    if (NULL == object) {
      return D3DERR_OUTOFVIDEOMEMORY;
    }
    object->lpVtbl = &Direct3DDevice8_Vtbl;
    object->ref = 1;
    object->direct3d8 = This;
    /** The device AddRef the direct3d8 Interface else crash in propers clients codes */
    IDirect3D8_AddRef((LPDIRECT3D8) object->direct3d8);

    /* Allocate an associated WineD3DDevice object */
    localParameters.BackBufferWidth                = &pPresentationParameters->BackBufferWidth;             
    localParameters.BackBufferHeight               = &pPresentationParameters->BackBufferHeight;
    localParameters.BackBufferFormat               = &pPresentationParameters->BackBufferFormat;           
    localParameters.BackBufferCount                = &pPresentationParameters->BackBufferCount;            
    localParameters.MultiSampleType                = &pPresentationParameters->MultiSampleType;            
    localParameters.MultiSampleQuality             = NULL; /* New at dx9 */
    localParameters.SwapEffect                     = &pPresentationParameters->SwapEffect;                 
    localParameters.hDeviceWindow                  = &pPresentationParameters->hDeviceWindow;              
    localParameters.Windowed                       = &pPresentationParameters->Windowed;                   
    localParameters.EnableAutoDepthStencil         = &pPresentationParameters->EnableAutoDepthStencil;     
    localParameters.AutoDepthStencilFormat         = &pPresentationParameters->AutoDepthStencilFormat;     
    localParameters.Flags                          = &pPresentationParameters->Flags;                      
    localParameters.FullScreen_RefreshRateInHz     = &pPresentationParameters->FullScreen_RefreshRateInHz; 
    localParameters.PresentationInterval           = &pPresentationParameters->FullScreen_PresentationInterval;    /* Renamed in dx9 */
    IWineD3D_CreateDevice(This->WineD3D, Adapter, DeviceType, hFocusWindow, BehaviourFlags, &localParameters, &object->WineD3DDevice, (IUnknown *)object, D3D8CB_CreateAdditionalSwapChain);

    /** use StateBlock Factory here, for creating the startup stateBlock */
    object->StateBlock = NULL;
    IDirect3DDeviceImpl_CreateStateBlock(object, D3DSBT_ALL, NULL);
    object->UpdateStateBlock = object->StateBlock;

    /* Save the creation parameters */
    object->CreateParms.AdapterOrdinal = Adapter;
    object->CreateParms.DeviceType = DeviceType;
    object->CreateParms.hFocusWindow = hFocusWindow;
    object->CreateParms.BehaviorFlags = BehaviourFlags;

    *ppReturnedDeviceInterface = (LPDIRECT3DDEVICE8) object;

    /* Initialize settings */
    object->PresentParms.BackBufferCount = 1; /* Opengl only supports one? */
    object->adapterNo = Adapter;
    object->devType = DeviceType;

    /* Initialize openGl - Note the visual is chosen as the window is created and the glcontext cannot
         use different properties after that point in time. FIXME: How to handle when requested format 
         doesn't match actual visual? Cannot choose one here - code removed as it ONLY works if the one
         it chooses is identical to the one already being used!                                        */
    /* FIXME: Handle stencil appropriately via EnableAutoDepthStencil / AutoDepthStencilFormat */

    /* Which hwnd are we using? */
    whichHWND = pPresentationParameters->hDeviceWindow;
    if (!whichHWND) {
        whichHWND = hFocusWindow;
    }
    object->win_handle = whichHWND;
    object->win     = (Window)GetPropA( whichHWND, "__wine_x11_whole_window" );

    hDc = GetDC(whichHWND);
    object->display = get_display(hDc);

    TRACE("(%p)->(DepthStencil:(%u,%s), BackBufferFormat:(%u,%s))\n", This, 
          pPresentationParameters->AutoDepthStencilFormat, debug_d3dformat(pPresentationParameters->AutoDepthStencilFormat),
          pPresentationParameters->BackBufferFormat, debug_d3dformat(pPresentationParameters->BackBufferFormat));

    ENTER_GL();

    /* Create a context based off the properties of the existing visual */
    template.visualid = (VisualID)GetPropA(GetDesktopWindow(), "__wine_x11_visual_id");
    object->visInfo = XGetVisualInfo(object->display, VisualIDMask, &template, &num);
    if (NULL == object->visInfo) {
        ERR("cannot really get XVisual\n"); 
        LEAVE_GL();
        return D3DERR_NOTAVAILABLE;
     }
    object->glCtx = glXCreateContext(object->display, object->visInfo, NULL, GL_TRUE);
    if (NULL == object->glCtx) {
      ERR("cannot create glxContext\n"); 
      LEAVE_GL();
      return D3DERR_NOTAVAILABLE;
     }
    LEAVE_GL();

    ReleaseDC(whichHWND, hDc);
    
    if (object->glCtx == NULL) {
        ERR("Error in context creation !\n");
        return D3DERR_INVALIDCALL;
    } else {
        TRACE("Context created (HWND=%p, glContext=%p, Window=%ld, VisInfo=%p)\n",
	      whichHWND, object->glCtx, object->win, object->visInfo);
    }

    /* If not windowed, need to go fullscreen, and resize the HWND to the appropriate  */
    /*        dimensions                                                               */
    if (!pPresentationParameters->Windowed) {
#if 1
	DEVMODEW devmode;
	HDC hdc;
        int bpp = 0;
	memset(&devmode, 0, sizeof(DEVMODEW));
	devmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT; 
	MultiByteToWideChar(CP_ACP, 0, "Gamers CG", -1, devmode.dmDeviceName, CCHDEVICENAME);
        hdc = CreateDCA("DISPLAY", NULL, NULL, NULL);
        bpp = GetDeviceCaps(hdc, BITSPIXEL);
        DeleteDC(hdc);
	devmode.dmBitsPerPel = (bpp >= 24) ? 32 : bpp;/*Stupid XVidMode cannot change bpp D3DFmtGetBpp(object, pPresentationParameters->BackBufferFormat);*/
	devmode.dmPelsWidth  = pPresentationParameters->BackBufferWidth;
	devmode.dmPelsHeight = pPresentationParameters->BackBufferHeight;
	ChangeDisplaySettingsExW(devmode.dmDeviceName, &devmode, object->win_handle, CDS_FULLSCREEN, NULL);
#else
        FIXME("Requested full screen support not implemented, expect windowed operation\n");
#endif

        /* Make popup window */
        SetWindowLongA(whichHWND, GWL_STYLE, WS_POPUP);
        SetWindowPos(object->win_handle, HWND_TOP, 0, 0, 
		     pPresentationParameters->BackBufferWidth,
                     pPresentationParameters->BackBufferHeight, SWP_SHOWWINDOW | SWP_FRAMECHANGED);
    }

    TRACE("Creating back buffer\n");
    /* MSDN: If Windowed is TRUE and either of the BackBufferWidth/Height values is zero,
       then the corresponding dimension of the client area of the hDeviceWindow
       (or the focus window, if hDeviceWindow is NULL) is taken. */
    if (pPresentationParameters->Windowed && ((pPresentationParameters->BackBufferWidth  == 0) ||
                                              (pPresentationParameters->BackBufferHeight == 0))) {
        RECT Rect;

        GetClientRect(whichHWND, &Rect);

        if (pPresentationParameters->BackBufferWidth == 0) {
           pPresentationParameters->BackBufferWidth = Rect.right;
           TRACE("Updating width to %d\n", pPresentationParameters->BackBufferWidth);
        }
        if (pPresentationParameters->BackBufferHeight == 0) {
           pPresentationParameters->BackBufferHeight = Rect.bottom;
           TRACE("Updating height to %d\n", pPresentationParameters->BackBufferHeight);
        }
    }

    /* Save the presentation parms now filled in correctly */
    memcpy(&object->PresentParms, pPresentationParameters, sizeof(D3DPRESENT_PARAMETERS));


    IDirect3DDevice8Impl_CreateRenderTarget((LPDIRECT3DDEVICE8) object,
                                            pPresentationParameters->BackBufferWidth,
                                            pPresentationParameters->BackBufferHeight,
                                            pPresentationParameters->BackBufferFormat,
					    pPresentationParameters->MultiSampleType,
					    TRUE,
                                            (LPDIRECT3DSURFACE8*) &object->frontBuffer);

    IDirect3DDevice8Impl_CreateRenderTarget((LPDIRECT3DDEVICE8) object,
                                            pPresentationParameters->BackBufferWidth,
                                            pPresentationParameters->BackBufferHeight,
                                            pPresentationParameters->BackBufferFormat,
					    pPresentationParameters->MultiSampleType,
					    TRUE,
                                            (LPDIRECT3DSURFACE8*) &object->backBuffer);

    if (pPresentationParameters->EnableAutoDepthStencil) {
       IDirect3DDevice8Impl_CreateDepthStencilSurface((LPDIRECT3DDEVICE8) object,
	  					      pPresentationParameters->BackBufferWidth,
						      pPresentationParameters->BackBufferHeight,
						      pPresentationParameters->AutoDepthStencilFormat,
						      D3DMULTISAMPLE_NONE,
						      (LPDIRECT3DSURFACE8*) &object->depthStencilBuffer);
    } else {
      object->depthStencilBuffer = NULL;
    }
    TRACE("FrontBuf @ %p, BackBuf @ %p, DepthStencil @ %p\n",object->frontBuffer, object->backBuffer, object->depthStencilBuffer);

    /* init the default renderTarget management */
    object->drawable = object->win;
    object->render_ctx = object->glCtx;
    object->renderTarget = object->backBuffer;
    IDirect3DSurface8Impl_AddRef((LPDIRECT3DSURFACE8) object->renderTarget);
    object->stencilBufferTarget = object->depthStencilBuffer;
    if (NULL != object->stencilBufferTarget) {
      IDirect3DSurface8Impl_AddRef((LPDIRECT3DSURFACE8) object->stencilBufferTarget);
    }

    ENTER_GL();

    if (glXMakeCurrent(object->display, object->win, object->glCtx) == False) {
      ERR("Error in setting current context (context %p drawable %ld)!\n", object->glCtx, object->win);
    }
    checkGLcall("glXMakeCurrent");

    /* Clear the screen */
    glClearColor(1.0, 0.0, 0.0, 0.0);
    checkGLcall("glClearColor");
    glColor3f(1.0, 1.0, 1.0);
    checkGLcall("glColor3f");

    glEnable(GL_LIGHTING);
    checkGLcall("glEnable");

    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
    checkGLcall("glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);");

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
    checkGLcall("glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);");

    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
    checkGLcall("glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);");

    /* 
     * Initialize openGL extension related variables
     *  with Default values 
     */
    IDirect3D8Impl_FillGLCaps(iface, object->display);

    /* Setup all the devices defaults */
    IDirect3DDeviceImpl_InitStartupStateBlock(object);

    LEAVE_GL();

    { /* Set a default viewport */
       D3DVIEWPORT8 vp;
       vp.X      = 0;
       vp.Y      = 0;
       vp.Width  = pPresentationParameters->BackBufferWidth;
       vp.Height = pPresentationParameters->BackBufferHeight;
       vp.MinZ   = 0.0f;
       vp.MaxZ   = 1.0f;
       IDirect3DDevice8Impl_SetViewport((LPDIRECT3DDEVICE8) object, &vp);
    }

    /* Initialize the current view state */
    object->modelview_valid = 1;
    object->proj_valid = 0;
    object->view_ident = 1;
    object->last_was_rhw = 0;
    glGetIntegerv(GL_MAX_LIGHTS, &object->maxConcurrentLights);
    TRACE("(%p,%d) All defaults now set up, leaving CreateDevice with %p\n", This, Adapter, object);

    /* Clear the screen */
    IDirect3DDevice8Impl_Clear((LPDIRECT3DDEVICE8) object, 0, NULL, D3DCLEAR_STENCIL|D3DCLEAR_ZBUFFER|D3DCLEAR_TARGET, 0x00, 1.0, 0);

    return D3D_OK;
}

const IDirect3D8Vtbl Direct3D8_Vtbl =
{
    IDirect3D8Impl_QueryInterface,
    IDirect3D8Impl_AddRef,
    IDirect3D8Impl_Release,
    IDirect3D8Impl_RegisterSoftwareDevice,
    IDirect3D8Impl_GetAdapterCount,
    IDirect3D8Impl_GetAdapterIdentifier,
    IDirect3D8Impl_GetAdapterModeCount,
    IDirect3D8Impl_EnumAdapterModes,
    IDirect3D8Impl_GetAdapterDisplayMode,
    IDirect3D8Impl_CheckDeviceType,
    IDirect3D8Impl_CheckDeviceFormat,
    IDirect3D8Impl_CheckDeviceMultiSampleType,
    IDirect3D8Impl_CheckDepthStencilMatch,
    IDirect3D8Impl_GetDeviceCaps,
    IDirect3D8Impl_GetAdapterMonitor,
    IDirect3D8Impl_CreateDevice
};
