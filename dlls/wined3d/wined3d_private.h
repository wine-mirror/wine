/*
 * Direct3D wine internal private include file
 *
 * Copyright 2002-2003 The wine-d3d team
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2004      Jason Edmeades   
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

#ifndef __WINE_WINED3D_PRIVATE_H
#define __WINE_WINED3D_PRIVATE_H

#include <stdarg.h>
#include <math.h>
#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/debug.h"
#include "wine/unicode.h"

#include "d3d9.h"
#include "d3d9types.h"
#include "wine/wined3d_interface.h"
#include "wine/wined3d_gl.h"

extern int vs_mode;
#define VS_NONE 0
#define VS_HW   1
#define VS_SW   2

extern int ps_mode;
#define PS_NONE 0
#define PS_HW   1

/* X11 locking */

extern void (*wine_tsx11_lock_ptr)(void);
extern void (*wine_tsx11_unlock_ptr)(void);

/* As GLX relies on X, this is needed */
extern int num_lock;

#if 0
#define ENTER_GL() ++num_lock; if (num_lock > 1) FIXME("Recursive use of GL lock to: %d\n", num_lock); wine_tsx11_lock_ptr()
#define LEAVE_GL() if (num_lock != 1) FIXME("Recursive use of GL lock: %d\n", num_lock); --num_lock; wine_tsx11_unlock_ptr()
#else
#define ENTER_GL() wine_tsx11_lock_ptr()
#define LEAVE_GL() wine_tsx11_unlock_ptr()
#endif

/*****************************************************************************
 * Defines
 */
#define GL_SUPPORT(ExtName)           (GLINFO_LOCATION.supported[ExtName] != 0)
#define GL_LIMITS(ExtName)            (GLINFO_LOCATION.max_##ExtName)

#define MAX_STREAMS  16  /* Maximum possible streams - used for fixed size arrays
                            See MaxStreams in MSDN under GetDeviceCaps */
#define HIGHEST_TRANSFORMSTATE 512 
                         /* Highest value in D3DTRANSFORMSTATETYPE */
#define WINED3D_VSHADER_MAX_CONSTANTS  96   
                         /* Maximum number of constants provided to the shaders */
#define MAX_CLIPPLANES  D3DMAXUSERCLIPPLANES

#define checkGLcall(A) \
{ \
    GLint err = glGetError();   \
    if (err != GL_NO_ERROR) { \
       FIXME(">>>>>>>>>>>>>>>>> %x from %s @ %s / %d\n", err, A, __FILE__, __LINE__); \
    } else { \
       TRACE("%s call ok %s / %d\n", A, __FILE__, __LINE__); \
    } \
}

#define conv_mat(mat,gl_mat)                                                                \
do {                                                                                        \
    TRACE("%f %f %f %f\n", (mat)->u.s._11, (mat)->u.s._12, (mat)->u.s._13, (mat)->u.s._14); \
    TRACE("%f %f %f %f\n", (mat)->u.s._21, (mat)->u.s._22, (mat)->u.s._23, (mat)->u.s._24); \
    TRACE("%f %f %f %f\n", (mat)->u.s._31, (mat)->u.s._32, (mat)->u.s._33, (mat)->u.s._34); \
    TRACE("%f %f %f %f\n", (mat)->u.s._41, (mat)->u.s._42, (mat)->u.s._43, (mat)->u.s._44); \
    memcpy(gl_mat, (mat), 16 * sizeof(float));                                              \
} while (0)

/* The following is purely to keep the source code as clear from #ifdefs as possible */
#if defined(GL_VERSION_1_3)
#define GL_ACTIVETEXTURE(textureNo)                          \
            glActiveTexture(GL_TEXTURE0 + textureNo);        \
            checkGLcall("glActiveTexture");      
#else 
#define GL_ACTIVETEXTURE(textureNo)                          \
            glActiveTextureARB(GL_TEXTURE0_ARB + textureNo); \
            checkGLcall("glActiveTextureARB");
#endif

/* Macro to dump out the current state of the light chain */
#define DUMP_LIGHT_CHAIN()                    \
{                                             \
  PLIGHTINFOEL *el = This->stateBlock->lights;\
  while (el) {                                \
    TRACE("Light %p (glIndex %ld, d3dIndex %ld, enabled %d)\n", el, el->glIndex, el->OriginalIndex, el->lightEnabled);\
    el = el->next;                            \
  }                                           \
}

typedef struct IWineD3DStateBlockImpl IWineD3DStateBlockImpl;

extern const float identity[16];

/*****************************************************************************
 * Internal representation of a light
 */
typedef struct PLIGHTINFOEL PLIGHTINFOEL;
struct PLIGHTINFOEL {
    WINED3DLIGHT OriginalParms; /* Note D3D8LIGHT == D3D9LIGHT */
    DWORD        OriginalIndex;
    LONG         glIndex;
    BOOL         lightEnabled;
    BOOL         changed;
    BOOL         enabledChanged;

    /* Converted parms to speed up swapping lights */
    float                         lightPosn[4];
    float                         lightDirn[4];
    float                         exponent;
    float                         cutoff;

    PLIGHTINFOEL *next;
    PLIGHTINFOEL *prev;
};

/*****************************************************************************
 * IWineD3D implementation structure
 */
typedef struct IWineD3DImpl
{
    /* IUnknown fields */
    IWineD3DVtbl           *lpVtbl;
    DWORD                   ref;     /* Note: Ref counting not required */

    /* WineD3D Information */
    IUnknown               *parent;
    UINT                    dxVersion;

    /* GL Information */
    BOOL                    isGLInfoValid;
    WineD3D_GL_Info         gl_info;
} IWineD3DImpl;

extern IWineD3DVtbl IWineD3D_Vtbl;

/*****************************************************************************
 * IWineD3DDevice implementation structure
 */
typedef struct IWineD3DDeviceImpl
{
    /* IUnknown fields      */
    IWineD3DDeviceVtbl     *lpVtbl;
    DWORD                   ref;     /* Note: Ref counting not required */

    /* WineD3D Information  */
    IUnknown               *parent;  /* TODO - to be a new interface eventually */
    IWineD3D               *wineD3D;

    /* X and GL Information */
    HWND                    win_handle;
    Window                  win;
    Display                *display;
    GLXContext              glCtx;
    XVisualInfo            *visInfo;
    GLXContext              render_ctx;
    Drawable                drawable;
    GLint                   maxConcurrentLights;

    /* Optimization */
    BOOL                    modelview_valid;
    BOOL                    proj_valid;
    BOOL                    view_ident;        /* true iff view matrix is identity                */
    BOOL                    last_was_rhw;      /* true iff last draw_primitive was in xyzrhw mode */

    /* State block related */
    BOOL                    isRecordingState;
    IWineD3DStateBlockImpl *stateBlock;
    IWineD3DStateBlockImpl *updateStateBlock;

    /* Internal use fields  */
    D3DDEVICE_CREATION_PARAMETERS   createParms;
    D3DPRESENT_PARAMETERS           presentParms;
    UINT                            adapterNo;
    D3DDEVTYPE                      devType;

} IWineD3DDeviceImpl;

extern IWineD3DDeviceVtbl IWineD3DDevice_Vtbl;

/*****************************************************************************
 * IWineD3DResource implementation structure
 */
typedef struct IWineD3DResourceClass
{
    /* IUnknown fields */
    DWORD                   ref;     /* Note: Ref counting not required */

    /* WineD3DResource Information */
    IUnknown               *parent;
    D3DRESOURCETYPE         resourceType;
    IWineD3DDevice         *wineD3DDevice;

} IWineD3DResourceClass;

typedef struct IWineD3DResourceImpl
{
    /* IUnknown & WineD3DResource Information     */
    IWineD3DResourceVtbl   *lpVtbl;
    IWineD3DResourceClass   resource;
} IWineD3DResourceImpl;

extern IWineD3DResourceVtbl IWineD3DResource_Vtbl;

/*****************************************************************************
 * IWineD3DVertexBuffer implementation structure (extends IWineD3DResourceImpl)
 */
typedef struct IWineD3DVertexBufferImpl
{
    /* IUnknown & WineD3DResource Information     */
    IWineD3DVertexBufferVtbl *lpVtbl;
    IWineD3DResourceClass     resource;

    /* WineD3DVertexBuffer specifics */
    BYTE                     *allocatedMemory;
    D3DVERTEXBUFFER_DESC      currentDesc;

} IWineD3DVertexBufferImpl;

extern IWineD3DVertexBufferVtbl IWineD3DVertexBuffer_Vtbl;

/*****************************************************************************
 * IWineD3DIndexBuffer implementation structure (extends IWineD3DResourceImpl)
 */
typedef struct IWineD3DIndexBufferImpl
{
    /* IUnknown & WineD3DResource Information     */
    IWineD3DIndexBufferVtbl *lpVtbl;
    IWineD3DResourceClass     resource;

    /* WineD3DVertexBuffer specifics */
    BYTE                     *allocatedMemory;
    D3DINDEXBUFFER_DESC       currentDesc;

} IWineD3DIndexBufferImpl;

extern IWineD3DIndexBufferVtbl IWineD3DIndexBuffer_Vtbl;

/*****************************************************************************
 * IWineD3DBaseTexture implementation structure (extends IWineD3DResourceImpl)
 */
typedef struct IWineD3DBaseTextureClass
{
    UINT                    levels;

} IWineD3DBaseTextureClass;

typedef struct IWineD3DBaseTextureImpl
{
    /* IUnknown & WineD3DResource Information     */
    IWineD3DIndexBufferVtbl  *lpVtbl;
    IWineD3DResourceClass     resource;
    IWineD3DBaseTextureClass  baseTexture;

} IWineD3DBaseTextureImpl;

extern IWineD3DBaseTextureVtbl IWineD3DBaseTexture_Vtbl;

/*****************************************************************************
 * IWineD3DStateBlock implementation structure
 */

/* Internal state Block for Begin/End/Capture/Create/Apply info  */
/*   Note: Very long winded but gl Lists are not flexible enough */
/*   to resolve everything we need, so doing it manually for now */
typedef struct SAVEDSTATES {
        BOOL                      material;
        BOOL                      fvf;
        BOOL                      stream_source[MAX_STREAMS];
        BOOL                      transform[HIGHEST_TRANSFORMSTATE];
        BOOL                      clipplane[MAX_CLIPPLANES];
} SAVEDSTATES;

struct IWineD3DStateBlockImpl
{
    /* IUnknown fields */
    IWineD3DStateBlockVtbl   *lpVtbl;
    DWORD                     ref;     /* Note: Ref counting not required */
    
    /* IWineD3DStateBlock information */
    IUnknown                 *parent;
    IWineD3DDevice           *wineD3DDevice;
    D3DSTATEBLOCKTYPE         blockType;

    /* Array indicating whether things have been set or changed */
    SAVEDSTATES               changed;
    SAVEDSTATES               set;
  
    /* Drawing - Vertex Shader or FVF related */
    DWORD                     fvf;

    /* Stream Source */
    UINT                      stream_stride[MAX_STREAMS];
    UINT                      stream_offset[MAX_STREAMS];
    IWineD3DVertexBuffer     *stream_source[MAX_STREAMS];

    /* Transform */
    D3DMATRIX                 transforms[HIGHEST_TRANSFORMSTATE];

    /* Lights */
    PLIGHTINFOEL             *lights; /* NOTE: active GL lights must be front of the chain */
    
    /* Clipping */
    double                    clipplane[MAX_CLIPPLANES][4];
    WINED3DCLIPSTATUS         clip_status;

    /* Material */
    WINED3DMATERIAL           material;

};

extern IWineD3DStateBlockVtbl IWineD3DStateBlock_Vtbl;

/*****************************************************************************
 * Utility function prototypes 
 */
const char* debug_d3dformat(D3DFORMAT fmt);
const char* debug_d3ddevicetype(D3DDEVTYPE devtype);
const char* debug_d3dresourcetype(D3DRESOURCETYPE res);
const char* debug_d3dusage(DWORD usage);

#if 0 /* Needs fixing during rework */
/*****************************************************************************
 * IDirect3DVertexShaderDeclaration implementation structure
 */
struct IDirect3DVertexShaderDeclarationImpl {
  /* The device */
  /*IDirect3DDeviceImpl* device;*/

  /** precomputed fvf if simple declaration */
  DWORD   fvf[MAX_STREAMS];
  DWORD   allFVF;

  /** dx8 compatible Declaration fields */
  DWORD*  pDeclaration8;
  DWORD   declaration8Length;
};


/*****************************************************************************
 * IDirect3DVertexShader implementation structure
 */
struct IDirect3DVertexShaderImpl {
  /* The device */
  /*IDirect3DDeviceImpl* device;*/

  DWORD* function;
  UINT functionLength;
  DWORD usage;
  DWORD version;
  /* run time datas */
  VSHADERDATA* data;
  VSHADERINPUTDATA input;
  VSHADEROUTPUTDATA output;
};


/*****************************************************************************
 * IDirect3DPixelShader implementation structure
 */
struct IDirect3DPixelShaderImpl { 
  /* The device */
  /*IDirect3DDeviceImpl* device;*/

  DWORD* function;
  UINT functionLength;
  DWORD version;
  /* run time datas */
  PSHADERDATA* data;
  PSHADERINPUTDATA input;
  PSHADEROUTPUTDATA output;
};

#endif /* Needs fixing during rework */
#endif
