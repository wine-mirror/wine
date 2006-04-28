/*
 * Direct3D wine internal private include file
 *
 * Copyright 2002-2003 The wine-d3d team
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2004 Jason Edmeades
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
#include "ddraw.h"
#include "wine/wined3d_interface.h"
#include "wine/wined3d_gl.h"

/* Device caps */
#define MAX_PALETTES      256
#define MAX_STREAMS       16
#define MAX_TEXTURES      8
#define MAX_SAMPLERS      16
#define MAX_ACTIVE_LIGHTS 8
#define MAX_CLIPPLANES    D3DMAXUSERCLIPPLANES
#define MAX_LEVELS        256

#define MAX_VSHADER_CONSTANTS 96
#define MAX_PSHADER_CONSTANTS 32

/* Used for CreateStateBlock */
#define NUM_SAVEDPIXELSTATES_R     35
#define NUM_SAVEDPIXELSTATES_T     18
#define NUM_SAVEDPIXELSTATES_S     12
#define NUM_SAVEDVERTEXSTATES_R    31
#define NUM_SAVEDVERTEXSTATES_T    2
#define NUM_SAVEDVERTEXSTATES_S    1

extern const DWORD SavedPixelStates_R[NUM_SAVEDPIXELSTATES_R];
extern const DWORD SavedPixelStates_T[NUM_SAVEDPIXELSTATES_T];
extern const DWORD SavedPixelStates_S[NUM_SAVEDPIXELSTATES_S];
extern const DWORD SavedVertexStates_R[NUM_SAVEDVERTEXSTATES_R];
extern const DWORD SavedVertexStates_T[NUM_SAVEDVERTEXSTATES_T];
extern const DWORD SavedVertexStates_S[NUM_SAVEDVERTEXSTATES_S];

typedef enum _WINELOOKUP {
    WINELOOKUP_WARPPARAM = 0,
    WINELOOKUP_MAGFILTER = 1,
    MAX_LOOKUPS          = 2
} WINELOOKUP;

extern int minLookup[MAX_LOOKUPS];
extern int maxLookup[MAX_LOOKUPS];
extern DWORD *stateLookup[MAX_LOOKUPS];

extern DWORD minMipLookup[D3DTEXF_ANISOTROPIC + 1][D3DTEXF_LINEAR + 1];

typedef struct _WINED3DGLTYPE {
    int         d3dType;
    GLint       size;
    GLenum      glType;
    GLboolean   normalized;
    int         typesize;
} WINED3DGLTYPE;

/* NOTE: Make sure these are in the correct numerical order. (see /include/d3d9types.h typedef enum _D3DDECLTYPE) */
WINED3DGLTYPE static const glTypeLookup[D3DDECLTYPE_UNUSED] = {
                                  {D3DDECLTYPE_FLOAT1,    1, GL_FLOAT           , GL_FALSE ,sizeof(float)},
                                  {D3DDECLTYPE_FLOAT2,    2, GL_FLOAT           , GL_FALSE ,sizeof(float)},
                                  {D3DDECLTYPE_FLOAT3,    3, GL_FLOAT           , GL_FALSE ,sizeof(float)},
                                  {D3DDECLTYPE_FLOAT4,    4, GL_FLOAT           , GL_FALSE ,sizeof(float)},
                                  {D3DDECLTYPE_D3DCOLOR,  4, GL_UNSIGNED_BYTE   , GL_TRUE  ,sizeof(BYTE)},
                                  {D3DDECLTYPE_UBYTE4,    4, GL_UNSIGNED_BYTE   , GL_FALSE ,sizeof(BYTE)},
                                  {D3DDECLTYPE_SHORT2,    2, GL_SHORT           , GL_FALSE ,sizeof(short int)},
                                  {D3DDECLTYPE_SHORT4,    4, GL_SHORT           , GL_FALSE ,sizeof(short int)},
                                  {D3DDECLTYPE_UBYTE4N,   4, GL_UNSIGNED_BYTE   , GL_FALSE ,sizeof(BYTE)},
                                  {D3DDECLTYPE_SHORT2N,   2, GL_SHORT           , GL_FALSE ,sizeof(short int)},
                                  {D3DDECLTYPE_SHORT4N,   4, GL_SHORT           , GL_FALSE ,sizeof(short int)},
                                  {D3DDECLTYPE_USHORT2N,  2, GL_UNSIGNED_SHORT  , GL_FALSE ,sizeof(short int)},
                                  {D3DDECLTYPE_USHORT4N,  4, GL_UNSIGNED_SHORT  , GL_FALSE ,sizeof(short int)},
                                  {D3DDECLTYPE_UDEC3,     3, GL_UNSIGNED_SHORT  , GL_FALSE ,sizeof(short int)},
                                  {D3DDECLTYPE_DEC3N,     3, GL_SHORT           , GL_FALSE ,sizeof(short int)},
                                  {D3DDECLTYPE_FLOAT16_2, 2, GL_FLOAT           , GL_FALSE ,sizeof(short int)},
                                  {D3DDECLTYPE_FLOAT16_4, 4, GL_FLOAT           , GL_FALSE ,sizeof(short int)}};

#define WINED3D_ATR_TYPE(_attribute)          glTypeLookup[sd->u.s._attribute.dwType].d3dType
#define WINED3D_ATR_SIZE(_attribute)          glTypeLookup[sd->u.s._attribute.dwType].size
#define WINED3D_ATR_GLTYPE(_attribute)        glTypeLookup[sd->u.s._attribute.dwType].glType
#define WINED3D_ATR_NORMALIZED(_attribute)    glTypeLookup[sd->u.s._attribute.dwType].normalized
#define WINED3D_ATR_TYPESIZE(_attribute)      glTypeLookup[sd->u.s._attribute.dwType].typesize

/**
 * Settings 
 */
#define VS_NONE    0
#define VS_HW      1
#define VS_SW      2

#define PS_NONE    0
#define PS_HW      1

#define VBO_NONE   0
#define VBO_HW     1

#define NP2_NONE   0
#define NP2_REPACK 1

typedef struct wined3d_settings_s {
/* vertex and pixel shader modes */
  int vs_mode;
  int ps_mode;
  int vbo_mode;
/* nonpower 2 function */
  int nonpower2_mode;
} wined3d_settings_t;

extern wined3d_settings_t wined3d_settings;

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

/* GL related defines */
/* ------------------ */
#define GL_SUPPORT(ExtName)           (GLINFO_LOCATION.supported[ExtName] != 0)
#define GL_LIMITS(ExtName)            (GLINFO_LOCATION.max_##ExtName)
#define GL_EXTCALL(FuncName)          (GLINFO_LOCATION.FuncName)
#define GL_VEND(_VendName)            (GLINFO_LOCATION.gl_vendor == VENDOR_##_VendName ? TRUE : FALSE)

#define D3DCOLOR_B_R(dw) (((dw) >> 16) & 0xFF)
#define D3DCOLOR_B_G(dw) (((dw) >>  8) & 0xFF)
#define D3DCOLOR_B_B(dw) (((dw) >>  0) & 0xFF)
#define D3DCOLOR_B_A(dw) (((dw) >> 24) & 0xFF)

#define D3DCOLOR_R(dw) (((float) (((dw) >> 16) & 0xFF)) / 255.0f)
#define D3DCOLOR_G(dw) (((float) (((dw) >>  8) & 0xFF)) / 255.0f)
#define D3DCOLOR_B(dw) (((float) (((dw) >>  0) & 0xFF)) / 255.0f)
#define D3DCOLOR_A(dw) (((float) (((dw) >> 24) & 0xFF)) / 255.0f)

#define D3DCOLORTOGLFLOAT4(dw, vec) \
  (vec)[0] = D3DCOLOR_R(dw); \
  (vec)[1] = D3DCOLOR_G(dw); \
  (vec)[2] = D3DCOLOR_B(dw); \
  (vec)[3] = D3DCOLOR_A(dw);
  
/* Note: The following is purely to keep the source code as clear from #ifdefs as possible */
#if defined(GL_VERSION_1_3)
#define GLACTIVETEXTURE(textureNo)                          \
            glActiveTexture(GL_TEXTURE0 + textureNo);       \
            checkGLcall("glActiveTexture");      
#define GLCLIENTACTIVETEXTURE(textureNo)                    \
            glClientActiveTexture(GL_TEXTURE0 + textureNo);
#define GLMULTITEXCOORD1F(a,b)                              \
            glMultiTexCoord1f(GL_TEXTURE0 + a, b);
#define GLMULTITEXCOORD2F(a,b,c)                            \
            glMultiTexCoord2f(GL_TEXTURE0 + a, b, c);
#define GLMULTITEXCOORD3F(a,b,c,d)                          \
            glMultiTexCoord3f(GL_TEXTURE0 + a, b, c, d);
#define GLMULTITEXCOORD4F(a,b,c,d,e)                        \
            glMultiTexCoord4f(GL_TEXTURE0 + a, b, c, d, e);
#define GLTEXTURECUBEMAP GL_TEXTURE_CUBE_MAP
#else 
#define GLACTIVETEXTURE(textureNo)                             \
            glActiveTextureARB(GL_TEXTURE0_ARB + textureNo);   \
            checkGLcall("glActiveTextureARB");
#define GLCLIENTACTIVETEXTURE(textureNo)                    \
            glClientActiveTextureARB(GL_TEXTURE0_ARB + textureNo);
#define GLMULTITEXCOORD1F(a,b)                                 \
            glMultiTexCoord1fARB(GL_TEXTURE0_ARB + a, b);
#define GLMULTITEXCOORD2F(a,b,c)                               \
            glMultiTexCoord2fARB(GL_TEXTURE0_ARB + a, b, c);
#define GLMULTITEXCOORD3F(a,b,c,d)                             \
            glMultiTexCoord3fARB(GL_TEXTURE0_ARB + a, b, c, d);
#define GLMULTITEXCOORD4F(a,b,c,d,e)                           \
            glMultiTexCoord4fARB(GL_TEXTURE0_ARB + a, b, c, d, e);
#define GLTEXTURECUBEMAP GL_TEXTURE_CUBE_MAP_ARB
#endif

/* DirectX Device Limits */
/* --------------------- */
#define MAX_LEVELS  256  /* Maximum number of mipmap levels. Guessed at 256 */

#define MAX_STREAMS  16  /* Maximum possible streams - used for fixed size arrays
                            See MaxStreams in MSDN under GetDeviceCaps */
                         /* Maximum number of constants provided to the shaders */
#define HIGHEST_TRANSFORMSTATE 512 
                         /* Highest value in D3DTRANSFORMSTATETYPE */
#define MAX_CLIPPLANES  D3DMAXUSERCLIPPLANES

#define MAX_PALETTES      256

/* Checking of API calls */
/* --------------------- */
#define checkGLcall(A)                                          \
{                                                               \
    GLint err = glGetError();                                   \
    if (err == GL_NO_ERROR) {                                   \
       TRACE("%s call ok %s / %d\n", A, __FILE__, __LINE__);    \
                                                                \
    } else do {                                                 \
       FIXME(">>>>>>>>>>>>>>>>> %x from %s @ %s / %d\n",        \
		err, A, __FILE__, __LINE__);                    \
       err = glGetError();                                      \
    } while (err != GL_NO_ERROR);                               \
} 

/* Trace routines / diagnostics */
/* ---------------------------- */

/* Dump out a matrix and copy it */
#define conv_mat(mat,gl_mat)                                                                \
do {                                                                                        \
    TRACE("%f %f %f %f\n", (mat)->u.s._11, (mat)->u.s._12, (mat)->u.s._13, (mat)->u.s._14); \
    TRACE("%f %f %f %f\n", (mat)->u.s._21, (mat)->u.s._22, (mat)->u.s._23, (mat)->u.s._24); \
    TRACE("%f %f %f %f\n", (mat)->u.s._31, (mat)->u.s._32, (mat)->u.s._33, (mat)->u.s._34); \
    TRACE("%f %f %f %f\n", (mat)->u.s._41, (mat)->u.s._42, (mat)->u.s._43, (mat)->u.s._44); \
    memcpy(gl_mat, (mat), 16 * sizeof(float));                                              \
} while (0)

/* Macro to dump out the current state of the light chain */
#define DUMP_LIGHT_CHAIN()                    \
{                                             \
  PLIGHTINFOEL *el = This->stateBlock->lights;\
  while (el) {                                \
    TRACE("Light %p (glIndex %ld, d3dIndex %ld, enabled %d)\n", el, el->glIndex, el->OriginalIndex, el->lightEnabled);\
    el = el->next;                            \
  }                                           \
}

/* Trace vector and strided data information */
#define TRACE_VECTOR(name) TRACE( #name "=(%f, %f, %f, %f)\n", name.x, name.y, name.z, name.w);
#define TRACE_STRIDED(sd,name) TRACE( #name "=(data:%p, stride:%ld, type:%ld)\n", sd->u.s.name.lpData, sd->u.s.name.dwStride, sd->u.s.name.dwType);

/* Defines used for optimizations */

/*    Only reapply what is necessary */
#define REAPPLY_ALPHAOP  0x0001
#define REAPPLY_ALL      0xFFFF

/* Advance declaration of structures to satisfy compiler */
typedef struct IWineD3DStateBlockImpl IWineD3DStateBlockImpl;
typedef struct IWineD3DSurfaceImpl    IWineD3DSurfaceImpl;
typedef struct IWineD3DPaletteImpl    IWineD3DPaletteImpl;

/* Tracking */

/* TODO: Move some of this to the device */
long globalChangeGlRam(long glram);

/* Memory and object tracking */

/*Structure for holding information on all direct3d objects
useful for making sure tracking is ok and when release is called on a device!
and probably quite handy for debugging and dumping states out
*/
typedef struct WineD3DGlobalStatistics {
    int glsurfaceram; /* The aproximate amount of glTexture memory allocated for textures */
} WineD3DGlobalStatistics;

extern WineD3DGlobalStatistics* wineD3DGlobalStatistics;

/* Global variables */
extern const float identity[16];

/*****************************************************************************
 * Compilable extra diagnostics
 */

/* Trace information per-vertex: (extremely high amount of trace) */
#if 0 /* NOTE: Must be 0 in cvs */
# define VTRACE(A) TRACE A
#else 
# define VTRACE(A) 
#endif

/* Checking of per-vertex related GL calls */
/* --------------------- */
#define vcheckGLcall(A)                                         \
{                                                               \
    GLint err = glGetError();                                   \
    if (err == GL_NO_ERROR) {                                   \
       VTRACE(("%s call ok %s / %d\n", A, __FILE__, __LINE__)); \
                                                                \
    } else do {                                                 \
       FIXME(">>>>>>>>>>>>>>>>> %x from %s @ %s / %d\n",        \
                err, A, __FILE__, __LINE__);                    \
       err = glGetError();                                      \
    } while (err != GL_NO_ERROR);                               \
}

/* TODO: Confirm each of these works when wined3d move completed */
#if 0 /* NOTE: Must be 0 in cvs */
  /* To avoid having to get gigabytes of trace, the following can be compiled in, and at the start
     of each frame, a check is made for the existence of C:\D3DTRACE, and if if exists d3d trace
     is enabled, and if it doesn't exists it is disabled.                                           */
# define FRAME_DEBUGGING
  /*  Adding in the SINGLE_FRAME_DEBUGGING gives a trace of just what makes up a single frame, before
      the file is deleted                                                                            */
# if 1 /* NOTE: Must be 1 in cvs, as this is mostly more useful than a trace from program start */
#  define SINGLE_FRAME_DEBUGGING
# endif  
  /* The following, when enabled, lets you see the makeup of the frame, by drawprimitive calls.
     It can only be enabled when FRAME_DEBUGGING is also enabled                               
     The contents of the back buffer are written into /tmp/backbuffer_* after each primitive 
     array is drawn.                                                                            */
# if 0 /* NOTE: Must be 0 in cvs, as this give a lot of ppm files when compiled in */                                                                                       
#  define SHOW_FRAME_MAKEUP 1
# endif  
  /* The following, when enabled, lets you see the makeup of the all the textures used during each
     of the drawprimitive calls. It can only be enabled when SHOW_FRAME_MAKEUP is also enabled.
     The contents of the textures assigned to each stage are written into 
     /tmp/texture_*_<Stage>.ppm after each primitive array is drawn.                            */
# if 0 /* NOTE: Must be 0 in cvs, as this give a lot of ppm files when compiled in */
#  define SHOW_TEXTURE_MAKEUP 0
# endif  
extern BOOL isOn;
extern BOOL isDumpingFrames;
extern LONG primCounter;
#endif

/*****************************************************************************
 * Prototypes
 */

/* Routine common to the draw primitive and draw indexed primitive routines */
void drawPrimitive(IWineD3DDevice *iface,
                    int PrimitiveType,
                    long NumPrimitives,
                    /* for Indexed: */
                    long  StartVertexIndex,
                    UINT  numberOfVertices,
                    long  StartIdx,
                    short idxBytes,
                    const void *idxData,
                    int   minIndex,
                    WineDirect3DVertexStridedData *DrawPrimStrideData);

/* Routine to fill gl caps for swapchains and IWineD3D */
BOOL IWineD3DImpl_FillGLCaps(WineD3D_GL_Info *gl_info,
                             Display* display);

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
    const IWineD3DVtbl     *lpVtbl;
    LONG                    ref;     /* Note: Ref counting not required */

    /* WineD3D Information */
    IUnknown               *parent;
    UINT                    dxVersion;

    /* GL Information */
    BOOL                    isGLInfoValid;
    WineD3D_GL_Info         gl_info;
} IWineD3DImpl;

extern const IWineD3DVtbl IWineD3D_Vtbl;

typedef struct SwapChainList {
    IWineD3DSwapChain         *swapchain;
    struct SwapChainList      *next;
} SwapChainList;

/** Hacked out start of a context manager!! **/
typedef struct glContext {
    int Width;
    int Height;
    int usedcount;
    GLXContext context;

    Drawable drawable;
    IWineD3DSurface *pSurface;
#if 0 /* TODO: someway to represent the state of the context */
    IWineD3DStateBlock *pStateBlock;
#endif
/* a few other things like format */
} glContext;

/* TODO: setup some flags in the regestry to enable, disable pbuffer support
(since it will break quite a few things until contexts are managed properly!) */
extern BOOL pbuffer_support;
/* allocate one pbuffer per surface */
extern BOOL pbuffer_per_surface;

/* Maximum number of contexts/pbuffers to keep in cache,
set to 100 because ATI's drivers don't support deleting pBuffers properly
this needs to be migrated to a list and some option availalbe for controle the cache size.
*/
#define CONTEXT_CACHE 100

typedef struct ResourceList {
    IWineD3DResource         *resource;
    struct ResourceList      *next;
} ResourceList;

/* A helper function that dumps a resource list */
void dumpResources(ResourceList *resources);

/*****************************************************************************
 * IWineD3DDevice implementation structure
 */
typedef struct IWineD3DDeviceImpl
{
    /* IUnknown fields      */
    const IWineD3DDeviceVtbl *lpVtbl;
    LONG                    ref;     /* Note: Ref counting not required */

    /* WineD3D Information  */
    IUnknown               *parent;
    IWineD3D               *wineD3D;

    /* X and GL Information */
    GLint                   maxConcurrentLights;

    /* Optimization */
    BOOL                    modelview_valid;
    BOOL                    proj_valid;
    BOOL                    view_ident;        /* true iff view matrix is identity                */
    BOOL                    last_was_rhw;      /* true iff last draw_primitive was in xyzrhw mode */
    BOOL                    viewport_changed;  /* Was the viewport changed since the last draw?   */
    GLenum                  tracking_parm;     /* Which source is tracking current colour         */
    LONG                    tracking_color;    /* used iff GL_COLOR_MATERIAL was enabled          */
#define                         DISABLED_TRACKING  0  /* Disabled                                 */
#define                         IS_TRACKING        1  /* tracking_parm is tracking diffuse color  */
#define                         NEEDS_TRACKING     2  /* Tracking needs to be enabled when needed */
#define                         NEEDS_DISABLE      3  /* Tracking needs to be disabled when needed*/
    UINT                    srcBlend;
    UINT                    dstBlend;
    UINT                    alphafunc;
    UINT                    stencilfunc;
    BOOL                    texture_shader_active;  /* TODO: Confirm use is correct */

    /* State block related */
    BOOL                    isRecordingState;
    IWineD3DStateBlockImpl *stateBlock;
    IWineD3DStateBlockImpl *updateStateBlock;

    /* Internal use fields  */
    WINED3DDEVICE_CREATION_PARAMETERS createParms;
    UINT                            adapterNo;
    D3DDEVTYPE                      devType;

    SwapChainList          *swapchains;

    ResourceList           *resources; /* a linked list to track resources created by the device */

    /* Render Target Support */
    IWineD3DSurface        *depthStencilBuffer;

    IWineD3DSurface        *renderTarget;
    IWineD3DSurface        *stencilBufferTarget;

    /* palettes texture management */
    PALETTEENTRY            palettes[MAX_PALETTES][256];
    UINT                    currentPalette;

    /* For rendering to a texture using glCopyTexImage */
    BOOL                    renderUpsideDown;

    /* Cursor management */
    BOOL                    bCursorVisible;
    UINT                    xHotSpot;
    UINT                    yHotSpot;
    UINT                    xScreenSpace;
    UINT                    yScreenSpace;

    /* Textures for when no other textures are mapped */
    UINT                          dummyTextureName[MAX_TEXTURES];

    /* Debug stream management */
    BOOL                     debug;

    /* Device state management */
    HRESULT                 state;
    BOOL                    d3d_initialized;

    /* Screen buffer resources */
    glContext contextCache[CONTEXT_CACHE];

    /* A flag to check if endscene has been called before changing the render tartet */
    BOOL sceneEnded;

    /* process vertex shaders using software or hardware */
    BOOL softwareVertexProcessing;

    /* DirectDraw stuff */
    HWND ddraw_window;

} IWineD3DDeviceImpl;

extern const IWineD3DDeviceVtbl IWineD3DDevice_Vtbl;

/* Support for IWineD3DResource ::Set/Get/FreePrivateData. I don't think
 * anybody uses it for much so a good implementation is optional. */
typedef struct PrivateData
{
    struct PrivateData* next;

    GUID tag;
    DWORD flags; /* DDSPD_* */
    DWORD uniqueness_value;

    union
    {
        LPVOID data;
        LPUNKNOWN object;
    } ptr;

    DWORD size;
} PrivateData;

/*****************************************************************************
 * IWineD3DResource implementation structure
 */
typedef struct IWineD3DResourceClass
{
    /* IUnknown fields */
    LONG                    ref;     /* Note: Ref counting not required */

    /* WineD3DResource Information */
    IUnknown               *parent;
    WINED3DRESOURCETYPE     resourceType;
    IWineD3DDeviceImpl     *wineD3DDevice;
    WINED3DPOOL             pool;
    UINT                    size;
    DWORD                   usage;
    WINED3DFORMAT           format;
    BYTE                   *allocatedMemory;
    PrivateData            *privateData;

} IWineD3DResourceClass;

typedef struct IWineD3DResourceImpl
{
    /* IUnknown & WineD3DResource Information     */
    const IWineD3DResourceVtbl *lpVtbl;
    IWineD3DResourceClass   resource;
} IWineD3DResourceImpl;


/*****************************************************************************
 * IWineD3DVertexBuffer implementation structure (extends IWineD3DResourceImpl)
 */
typedef struct IWineD3DVertexBufferImpl
{
    /* IUnknown & WineD3DResource Information     */
    const IWineD3DVertexBufferVtbl *lpVtbl;
    IWineD3DResourceClass     resource;

    /* WineD3DVertexBuffer specifics */
    DWORD                     fvf;

} IWineD3DVertexBufferImpl;

extern const IWineD3DVertexBufferVtbl IWineD3DVertexBuffer_Vtbl;


/*****************************************************************************
 * IWineD3DIndexBuffer implementation structure (extends IWineD3DResourceImpl)
 */
typedef struct IWineD3DIndexBufferImpl
{
    /* IUnknown & WineD3DResource Information     */
    const IWineD3DIndexBufferVtbl *lpVtbl;
    IWineD3DResourceClass     resource;

    /* WineD3DVertexBuffer specifics */
} IWineD3DIndexBufferImpl;

extern const IWineD3DIndexBufferVtbl IWineD3DIndexBuffer_Vtbl;

/*****************************************************************************
 * IWineD3DBaseTexture D3D- > openGL state map lookups
 */
#define WINED3DFUNC_NOTSUPPORTED  -2
#define WINED3DFUNC_UNIMPLEMENTED -1

typedef enum winetexturestates {
    WINED3DTEXSTA_ADDRESSU       = 0,
    WINED3DTEXSTA_ADDRESSV       = 1,
    WINED3DTEXSTA_ADDRESSW       = 2,
    WINED3DTEXSTA_BORDERCOLOR    = 3,
    WINED3DTEXSTA_MAGFILTER      = 4,
    WINED3DTEXSTA_MINFILTER      = 5,
    WINED3DTEXSTA_MIPFILTER      = 6,
    WINED3DTEXSTA_MAXMIPLEVEL    = 7,
    WINED3DTEXSTA_MAXANISOTROPY  = 8,
    WINED3DTEXSTA_SRGBTEXTURE    = 9,
    WINED3DTEXSTA_ELEMENTINDEX   = 10,
    WINED3DTEXSTA_DMAPOFFSET     = 11,
    WINED3DTEXSTA_TSSADDRESSW    = 12,
    MAX_WINETEXTURESTATES        = 13,
} winetexturestates;

typedef struct Wined3dTextureStateMap {
    CONST int state;
    int function;
} Wined3dTextureStateMap;

/*****************************************************************************
 * IWineD3DBaseTexture implementation structure (extends IWineD3DResourceImpl)
 */
typedef struct IWineD3DBaseTextureClass
{
    UINT                    levels;
    BOOL                    dirty;
    D3DFORMAT               format;
    DWORD                   usage;
    UINT                    textureName;
    UINT                    LOD;
    WINED3DTEXTUREFILTERTYPE filterType;
    DWORD                   states[MAX_WINETEXTURESTATES];

} IWineD3DBaseTextureClass;

typedef struct IWineD3DBaseTextureImpl
{
    /* IUnknown & WineD3DResource Information     */
    const IWineD3DBaseTextureVtbl *lpVtbl;
    IWineD3DResourceClass     resource;
    IWineD3DBaseTextureClass  baseTexture;

} IWineD3DBaseTextureImpl;

/*****************************************************************************
 * IWineD3DTexture implementation structure (extends IWineD3DBaseTextureImpl)
 */
typedef struct IWineD3DTextureImpl
{
    /* IUnknown & WineD3DResource/WineD3DBaseTexture Information     */
    const IWineD3DTextureVtbl *lpVtbl;
    IWineD3DResourceClass     resource;
    IWineD3DBaseTextureClass  baseTexture;

    /* IWineD3DTexture */
    IWineD3DSurface          *surfaces[MAX_LEVELS];
    
    UINT                      width;
    UINT                      height;
    float                     pow2scalingFactorX;
    float                     pow2scalingFactorY;

} IWineD3DTextureImpl;

extern const IWineD3DTextureVtbl IWineD3DTexture_Vtbl;

/*****************************************************************************
 * IWineD3DCubeTexture implementation structure (extends IWineD3DBaseTextureImpl)
 */
typedef struct IWineD3DCubeTextureImpl
{
    /* IUnknown & WineD3DResource/WineD3DBaseTexture Information     */
    const IWineD3DCubeTextureVtbl *lpVtbl;
    IWineD3DResourceClass     resource;
    IWineD3DBaseTextureClass  baseTexture;

    /* IWineD3DCubeTexture */
    IWineD3DSurface          *surfaces[6][MAX_LEVELS];

    UINT                      edgeLength;
    float                     pow2scalingFactor;

} IWineD3DCubeTextureImpl;

extern const IWineD3DCubeTextureVtbl IWineD3DCubeTexture_Vtbl;

/*****************************************************************************
 * IWineD3DVolume implementation structure (extends IUnknown)
 */
typedef struct IWineD3DVolumeImpl
{
    /* IUnknown & WineD3DResource fields */
    const IWineD3DVolumeVtbl  *lpVtbl;
    IWineD3DResourceClass      resource;

    /* WineD3DVolume Information */
    D3DVOLUME_DESC          currentDesc;
    IWineD3DBase            *container;
    UINT                    bytesPerPixel;

    BOOL                    lockable;
    BOOL                    locked;
    WINED3DBOX              lockedBox;
    WINED3DBOX              dirtyBox;
    BOOL                    dirty;


} IWineD3DVolumeImpl;

extern const IWineD3DVolumeVtbl IWineD3DVolume_Vtbl;

/*****************************************************************************
 * IWineD3DVolumeTexture implementation structure (extends IWineD3DBaseTextureImpl)
 */
typedef struct IWineD3DVolumeTextureImpl
{
    /* IUnknown & WineD3DResource/WineD3DBaseTexture Information     */
    const IWineD3DVolumeTextureVtbl *lpVtbl;
    IWineD3DResourceClass     resource;
    IWineD3DBaseTextureClass  baseTexture;

    /* IWineD3DVolumeTexture */
    IWineD3DVolume           *volumes[MAX_LEVELS];

    UINT                      width;
    UINT                      height;
    UINT                      depth;
} IWineD3DVolumeTextureImpl;

extern const IWineD3DVolumeTextureVtbl IWineD3DVolumeTexture_Vtbl;

typedef struct _WINED3DSURFACET_DESC
{
    WINED3DMULTISAMPLE_TYPE MultiSampleType;
    DWORD                   MultiSampleQuality;
    UINT                    Width;
    UINT                    Height;
} WINED3DSURFACET_DESC;

/*****************************************************************************
 * IWineD3DSurface implementation structure
 */
struct IWineD3DSurfaceImpl
{
    /* IUnknown & IWineD3DResource Information     */
    const IWineD3DSurfaceVtbl *lpVtbl;
    IWineD3DResourceClass     resource;

    /* IWineD3DSurface fields */
    IWineD3DBase              *container;
    WINED3DSURFACET_DESC      currentDesc;
    IWineD3DPaletteImpl      *palette;

    UINT                      textureName;
    UINT                      bytesPerPixel;

    /* TODO: move this off into a management class(maybe!) */
    WORD                      Flags;

    UINT                      pow2Width;
    UINT                      pow2Height;
    UINT                      pow2Size;

#if 0
    /* precalculated x and y scalings for texture coords */
    float                     pow2scalingFactorX; /* =  (Width  / pow2Width ) */
    float                     pow2scalingFactorY; /* =  (Height / pow2Height) */
#endif

    RECT                      lockedRect;
    RECT                      dirtyRect;

    glDescriptor              glDescription;
};

extern const IWineD3DSurfaceVtbl IWineD3DSurface_Vtbl;

/* Surface flags: */
#define SFLAG_OVERSIZE    0x0001 /* Surface is bigger than gl size, blts only */
#define SFLAG_CONVERTED   0x0002 /* Converted for color keying or Palettized */
#define SFLAG_DIBSECTION  0x0004 /* Has a DIB section attached for getdc */
#define SFLAG_DIRTY       0x0008 /* Surface was locked by the app */
#define SFLAG_LOCKABLE    0x0010 /* Surface can be locked */
#define SFLAG_DISCARD     0x0020 /* ??? */
#define SFLAG_LOCKED      0x0040 /* Surface is locked atm */
#define SFLAG_ACTIVELOCK  0x0080 /* Not locked, but surface memory is needed */
#define SFLAG_INTEXTURE   0x0100 /* ??? */
#define SFLAG_INPBUFFER   0x0200 /* ??? */
#define SFLAG_NONPOW2     0x0400 /* Surface sizes are not a power of 2 */
#define SFLAG_DYNLOCK     0x0800 /* Surface is often locked by the app */
#define SFLAG_DYNCHANGE   0x1800 /* Surface contents are changed very often, implies DYNLOCK */

/* In some conditions the surface memory must not be freed:
 * SFLAG_OVERSIZE: Not all data can be kept in GL
 * SFLAG_CONVERTED: Converting the data back would take too long
 * SFLAG_DIBSECTION: The dib code manages the memory
 * SFLAG_DIRTY: GL surface isn't up to date
 * SFLAG_LOCKED: The app requires access to the surface data
 * SFLAG_ACTIVELOCK: Some wined3d code needs the memory
 * SFLAG_DYNLOCK: Avoid freeing the data for performance
 * SFLAG_DYNCHANGE: Same reason as DYNLOCK
 */
#define SFLAG_DONOTFREE  (SFLAG_OVERSIZE   | \
                          SFLAG_CONVERTED  | \
                          SFLAG_DIBSECTION | \
                          SFLAG_DIRTY      | \
                          SFLAG_LOCKED     | \
                          SFLAG_ACTIVELOCK | \
                          SFLAG_DYNLOCK    | \
                          SFLAG_DYNCHANGE    )

/*****************************************************************************
 * IWineD3DVertexDeclaration implementation structure
 */
typedef struct IWineD3DVertexDeclarationImpl {
 /* IUnknown  Information     */
  const IWineD3DVertexDeclarationVtbl *lpVtbl;
  LONG                    ref;     /* Note: Ref counting not required */

  IUnknown               *parent;
  /** precomputed fvf if simple declaration */
  IWineD3DDeviceImpl     *wineD3DDevice;
  DWORD   fvf[MAX_STREAMS];
  DWORD   allFVF;

  /** dx8 compatible Declaration fields */
  DWORD*  pDeclaration8;
  DWORD   declaration8Length;

  /** dx9+ */
  D3DVERTEXELEMENT9 *pDeclaration9;
  UINT               declaration9NumElements;

  WINED3DVERTEXELEMENT  *pDeclarationWine;
  UINT                   declarationWNumElements;
  
  float                 *constants;
  
} IWineD3DVertexDeclarationImpl;

extern const IWineD3DVertexDeclarationVtbl IWineD3DVertexDeclaration_Vtbl;

/*****************************************************************************
 * IWineD3DStateBlock implementation structure
 */

/* Internal state Block for Begin/End/Capture/Create/Apply info  */
/*   Note: Very long winded but gl Lists are not flexible enough */
/*   to resolve everything we need, so doing it manually for now */
typedef struct SAVEDSTATES {
        BOOL                      indices;
        BOOL                      material;
        BOOL                      fvf;
        BOOL                      streamSource[MAX_STREAMS];
        BOOL                      streamFreq[MAX_STREAMS];
        BOOL                      textures[MAX_TEXTURES];
        BOOL                      transform[HIGHEST_TRANSFORMSTATE + 1];
        BOOL                      viewport;
        BOOL                      renderState[WINEHIGHEST_RENDER_STATE + 1];
        BOOL                      textureState[MAX_TEXTURES][WINED3D_HIGHEST_TEXTURE_STATE + 1];
        BOOL                      samplerState[MAX_SAMPLERS][WINED3D_HIGHEST_SAMPLER_STATE + 1];
        BOOL                      clipplane[MAX_CLIPPLANES];
        BOOL                      vertexDecl;
        BOOL                      pixelShader;
        BOOL                      pixelShaderConstants[MAX_PSHADER_CONSTANTS];
        BOOL                      vertexShader;
        BOOL                      vertexShaderConstants[MAX_VSHADER_CONSTANTS];
} SAVEDSTATES;

typedef enum {
    WINESHADERCNST_NONE     = 0,
    WINESHADERCNST_FLOAT    = 1,
    WINESHADERCNST_INTEGER  = 2,
    WINESHADERCNST_BOOL     = 3
} WINESHADERCNST;

struct IWineD3DStateBlockImpl
{
    /* IUnknown fields */
    const IWineD3DStateBlockVtbl *lpVtbl;
    LONG                      ref;     /* Note: Ref counting not required */

    /* IWineD3DStateBlock information */
    IUnknown                 *parent;
    IWineD3DDeviceImpl       *wineD3DDevice;
    WINED3DSTATEBLOCKTYPE     blockType;

    /* Array indicating whether things have been set or changed */
    SAVEDSTATES               changed;
    SAVEDSTATES               set;

    /* Drawing - Vertex Shader or FVF related */
    DWORD                     fvf;
    /* Vertex Shader Declaration */
    IWineD3DVertexDeclaration *vertexDecl;

    IWineD3DVertexShader      *vertexShader;

    /* Vertex Shader Constants */
    BOOL                       vertexShaderConstantB[MAX_VSHADER_CONSTANTS];
    INT                        vertexShaderConstantI[MAX_VSHADER_CONSTANTS * 4];
    float                      vertexShaderConstantF[MAX_VSHADER_CONSTANTS * 4];
    WINESHADERCNST             vertexShaderConstantT[MAX_VSHADER_CONSTANTS]; /* TODO: Think about changing this to a char to possibly save a little memory */

    /* Stream Source */
    BOOL                      streamIsUP;
    UINT                      streamStride[MAX_STREAMS];
    UINT                      streamOffset[MAX_STREAMS];
    IWineD3DVertexBuffer     *streamSource[MAX_STREAMS];
    UINT                      streamFreq[MAX_STREAMS];
    UINT                      streamFlags[MAX_STREAMS];     /*0 | D3DSTREAMSOURCE_INSTANCEDATA | D3DSTREAMSOURCE_INDEXEDDATA  */

    /* Indices */
    IWineD3DIndexBuffer*      pIndexData;
    UINT                      baseVertexIndex; /* Note: only used for d3d8 */

    /* Transform */
    D3DMATRIX                 transforms[HIGHEST_TRANSFORMSTATE + 1];

    /* Lights */
    PLIGHTINFOEL             *lights; /* NOTE: active GL lights must be front of the chain */

    /* Clipping */
    double                    clipplane[MAX_CLIPPLANES][4];
    WINED3DCLIPSTATUS         clip_status;

    /* ViewPort */
    WINED3DVIEWPORT           viewport;

    /* Material */
    WINED3DMATERIAL           material;

    /* Pixel Shader */
    IWineD3DPixelShader      *pixelShader;

    /* Pixel Shader Constants */
    BOOL                       pixelShaderConstantB[MAX_PSHADER_CONSTANTS];
    INT                        pixelShaderConstantI[MAX_PSHADER_CONSTANTS * 4];
    float                      pixelShaderConstantF[MAX_PSHADER_CONSTANTS * 4];
    WINESHADERCNST             pixelShaderConstantT[MAX_PSHADER_CONSTANTS]; /* TODO: Think about changing this to a char to possibly save a little memory */

    /* Indexed Vertex Blending */
    D3DVERTEXBLENDFLAGS       vertex_blend;
    FLOAT                     tween_factor;

    /* RenderState */
    DWORD                     renderState[WINEHIGHEST_RENDER_STATE + 1];

    /* Texture */
    IWineD3DBaseTexture      *textures[MAX_TEXTURES];
    int                       textureDimensions[MAX_SAMPLERS];

    /* Texture State Stage */
    DWORD                     textureState[MAX_TEXTURES][WINED3D_HIGHEST_TEXTURE_STATE + 1];
    /* Sampler States */
    DWORD                     samplerState[MAX_SAMPLERS][WINED3D_HIGHEST_SAMPLER_STATE + 1];

};

extern const IWineD3DStateBlockVtbl IWineD3DStateBlock_Vtbl;

/*****************************************************************************
 * IWineD3DQueryImpl implementation structure (extends IUnknown)
 */
typedef struct IWineD3DQueryImpl
{
    const IWineD3DQueryVtbl  *lpVtbl;
    LONG                      ref;     /* Note: Ref counting not required */
    
    IUnknown                 *parent;
    /*TODO: replace with iface usage */
#if 0
    IWineD3DDevice         *wineD3DDevice;
#else
    IWineD3DDeviceImpl       *wineD3DDevice;
#endif
    /* IWineD3DQuery fields */

    D3DQUERYTYPE              type;
    /* TODO: Think about using a IUnknown instead of a void* */
    void                     *extendedData;
    
  
} IWineD3DQueryImpl;

extern const IWineD3DQueryVtbl IWineD3DQuery_Vtbl;

/* Datastructures for IWineD3DQueryImpl.extendedData */
typedef struct  WineQueryOcclusionData {
       unsigned int queryId;
} WineQueryOcclusionData;


/*****************************************************************************
 * IWineD3DSwapChainImpl implementation structure (extends IUnknown)
 */

typedef struct IWineD3DSwapChainImpl
{
    /*IUnknown part*/
    IWineD3DSwapChainVtbl    *lpVtbl;
    LONG                      ref;     /* Note: Ref counting not required */

    IUnknown                 *parent;
    IWineD3DDeviceImpl       *wineD3DDevice;

    /* IWineD3DSwapChain fields */
    IWineD3DSurface          *backBuffer;
    IWineD3DSurface          *frontBuffer;
    BOOL                      wantsDepthStencilBuffer;
    D3DPRESENT_PARAMETERS     presentParms;

    /* TODO: move everything up to drawable off into a context manager
      and store the 'data' in the contextManagerData interface.
    IUnknown                  *contextManagerData;
    */

    HWND                    win_handle;
    Window                  win;
    Display                *display;

    GLXContext              glCtx;
    XVisualInfo            *visInfo;
    GLXContext              render_ctx;
    /* This has been left in device for now, but needs moving off into a rendertarget management class and separated out from swapchains and devices. */
    Drawable                drawable;
} IWineD3DSwapChainImpl;

extern IWineD3DSwapChainVtbl IWineD3DSwapChain_Vtbl;

/*****************************************************************************
 * Utility function prototypes 
 */

/* Trace routines */
const char* debug_d3dformat(WINED3DFORMAT fmt);
const char* debug_d3ddevicetype(D3DDEVTYPE devtype);
const char* debug_d3dresourcetype(WINED3DRESOURCETYPE res);
const char* debug_d3dusage(DWORD usage);
const char* debug_d3dprimitivetype(D3DPRIMITIVETYPE PrimitiveType);
const char* debug_d3drenderstate(DWORD state);
const char* debug_d3dtexturestate(DWORD state);
const char* debug_d3dpool(WINED3DPOOL pool);

/* Routines for GL <-> D3D values */
GLenum StencilOp(DWORD op);
void   set_tex_op(IWineD3DDevice *iface, BOOL isAlpha, int Stage, D3DTEXTUREOP op, DWORD arg1, DWORD arg2, DWORD arg3);
void   set_texture_matrix(const float *smat, DWORD flags, BOOL calculatedCoords);
void   GetSrcAndOpFromValue(DWORD iValue, BOOL isAlphaArg, GLenum* source, GLenum* operand);

SHORT  D3DFmtGetBpp(IWineD3DDeviceImpl* This, D3DFORMAT fmt);
GLenum D3DFmt2GLFmt(IWineD3DDeviceImpl* This, D3DFORMAT fmt);
GLenum D3DFmt2GLType(IWineD3DDeviceImpl *This, D3DFORMAT fmt);
GLint  D3DFmt2GLIntFmt(IWineD3DDeviceImpl* This, D3DFORMAT fmt);

int D3DFmtMakeGlCfg(D3DFORMAT BackBufferFormat, D3DFORMAT StencilBufferFormat, int *attribs, int* nAttribs, BOOL alternate);


/*****************************************************************************
 * To enable calling of inherited functions, requires prototypes 
 *
 * Note: Only require classes which are subclassed, ie resource, basetexture, 
 */
    /*** IUnknown methods ***/
    extern HRESULT WINAPI IWineD3DResourceImpl_QueryInterface(IWineD3DResource *iface, REFIID riid, void** ppvObject);
    extern ULONG WINAPI IWineD3DResourceImpl_AddRef(IWineD3DResource *iface);
    extern ULONG WINAPI IWineD3DResourceImpl_Release(IWineD3DResource *iface);
    /*** IWineD3DResource methods ***/
    extern HRESULT WINAPI IWineD3DResourceImpl_GetParent(IWineD3DResource *iface, IUnknown **pParent);
    extern HRESULT WINAPI IWineD3DResourceImpl_GetDevice(IWineD3DResource *iface, IWineD3DDevice ** ppDevice);
    extern HRESULT WINAPI IWineD3DResourceImpl_SetPrivateData(IWineD3DResource *iface, REFGUID  refguid, CONST void * pData, DWORD  SizeOfData, DWORD  Flags);
    extern HRESULT WINAPI IWineD3DResourceImpl_GetPrivateData(IWineD3DResource *iface, REFGUID  refguid, void * pData, DWORD * pSizeOfData);
    extern HRESULT WINAPI IWineD3DResourceImpl_FreePrivateData(IWineD3DResource *iface, REFGUID  refguid);
    extern DWORD WINAPI IWineD3DResourceImpl_SetPriority(IWineD3DResource *iface, DWORD  PriorityNew);
    extern DWORD WINAPI IWineD3DResourceImpl_GetPriority(IWineD3DResource *iface);
    extern void WINAPI IWineD3DResourceImpl_PreLoad(IWineD3DResource *iface);
    extern WINED3DRESOURCETYPE WINAPI IWineD3DResourceImpl_GetType(IWineD3DResource *iface);
    /*** class static members ***/
    void IWineD3DResourceImpl_CleanUp(IWineD3DResource *iface);

    /*** IUnknown methods ***/
    extern HRESULT WINAPI IWineD3DBaseTextureImpl_QueryInterface(IWineD3DBaseTexture *iface, REFIID riid, void** ppvObject);
    extern ULONG WINAPI IWineD3DBaseTextureImpl_AddRef(IWineD3DBaseTexture *iface);
    extern ULONG WINAPI IWineD3DBaseTextureImpl_Release(IWineD3DBaseTexture *iface);
    /*** IWineD3DResource methods ***/
    extern HRESULT WINAPI IWineD3DBaseTextureImpl_GetParent(IWineD3DBaseTexture *iface, IUnknown **pParent);
    extern HRESULT WINAPI IWineD3DBaseTextureImpl_GetDevice(IWineD3DBaseTexture *iface, IWineD3DDevice ** ppDevice);
    extern HRESULT WINAPI IWineD3DBaseTextureImpl_SetPrivateData(IWineD3DBaseTexture *iface, REFGUID  refguid, CONST void * pData, DWORD  SizeOfData, DWORD  Flags);
    extern HRESULT WINAPI IWineD3DBaseTextureImpl_GetPrivateData(IWineD3DBaseTexture *iface, REFGUID  refguid, void * pData, DWORD * pSizeOfData);
    extern HRESULT WINAPI IWineD3DBaseTextureImpl_FreePrivateData(IWineD3DBaseTexture *iface, REFGUID  refguid);
    extern DWORD WINAPI IWineD3DBaseTextureImpl_SetPriority(IWineD3DBaseTexture *iface, DWORD  PriorityNew);
    extern DWORD WINAPI IWineD3DBaseTextureImpl_GetPriority(IWineD3DBaseTexture *iface);
    extern void WINAPI IWineD3DBaseTextureImpl_PreLoad(IWineD3DBaseTexture *iface);
    extern WINED3DRESOURCETYPE WINAPI IWineD3DBaseTextureImpl_GetType(IWineD3DBaseTexture *iface);
    /*** IWineD3DBaseTexture methods ***/
    extern DWORD WINAPI IWineD3DBaseTextureImpl_SetLOD(IWineD3DBaseTexture *iface, DWORD LODNew);
    extern DWORD WINAPI IWineD3DBaseTextureImpl_GetLOD(IWineD3DBaseTexture *iface);
    extern DWORD WINAPI IWineD3DBaseTextureImpl_GetLevelCount(IWineD3DBaseTexture *iface);
    extern HRESULT WINAPI IWineD3DBaseTextureImpl_SetAutoGenFilterType(IWineD3DBaseTexture *iface, WINED3DTEXTUREFILTERTYPE FilterType);
    extern WINED3DTEXTUREFILTERTYPE WINAPI IWineD3DBaseTextureImpl_GetAutoGenFilterType(IWineD3DBaseTexture *iface);
    extern void WINAPI IWineD3DBaseTextureImpl_GenerateMipSubLevels(IWineD3DBaseTexture *iface);
    extern BOOL WINAPI IWineD3DBaseTextureImpl_SetDirty(IWineD3DBaseTexture *iface, BOOL);
    extern BOOL WINAPI IWineD3DBaseTextureImpl_GetDirty(IWineD3DBaseTexture *iface);

    extern BYTE* WINAPI IWineD3DVertexBufferImpl_GetMemory(IWineD3DVertexBuffer* iface, DWORD iOffset);
    extern HRESULT WINAPI IWineD3DVertexBufferImpl_ReleaseMemory(IWineD3DVertexBuffer* iface);
    extern HRESULT WINAPI IWineD3DBaseTextureImpl_BindTexture(IWineD3DBaseTexture *iface);
    extern HRESULT WINAPI IWineD3DBaseTextureImpl_UnBindTexture(IWineD3DBaseTexture *iface);
    extern void WINAPI IWineD3DBaseTextureImpl_ApplyStateChanges(IWineD3DBaseTexture *iface, const DWORD textureStates[WINED3D_HIGHEST_TEXTURE_STATE + 1], const DWORD samplerStates[WINED3D_HIGHEST_SAMPLER_STATE + 1]);
    /*** class static members ***/
    void IWineD3DBaseTextureImpl_CleanUp(IWineD3DBaseTexture *iface);

/* An enum for the type of constants that are used... addressing causes
 * problems with being able to work out what's used and what's not.. so
 * maybe we'll have to rely on the server vertex shader const functions?
 */
enum vsConstantsEnum {
    VS_CONSTANT_NOT_USED = 0,
    VS_CONSTANT_CONSTANT,
    VS_CONSTANT_INTEGER,
    VS_CONSTANT_BOOLEAN,
    VS_CONSTANT_FLOAT
};

typedef void (*shader_fct_t)();

typedef struct SHADER_OPCODE {
    unsigned int  opcode;
    const char*   name;
    const char*   glname;
    CONST UINT    num_params;
    shader_fct_t  soft_fct;
    DWORD         min_version;
    DWORD         max_version;
} SHADER_OPCODE;

/*****************************************************************************
 * IDirect3DBaseShader implementation structure
 */
typedef struct IWineD3DBaseShaderClass
{
    DWORD                           version;
    CONST SHADER_OPCODE             *shader_ins;
    CONST DWORD                     *function;
    UINT                            functionLength;
    GLuint                          prgId;
} IWineD3DBaseShaderClass;

typedef struct IWineD3DBaseShaderImpl {
    /* IUnknown */
    const IWineD3DBaseShaderVtbl    *lpVtbl;
    LONG                            ref;

    /* IWineD3DBaseShader */
    IWineD3DBaseShaderClass         baseShader;
} IWineD3DBaseShaderImpl;

inline static int shader_get_regtype(const DWORD param) {
    return (((param & D3DSP_REGTYPE_MASK) >> D3DSP_REGTYPE_SHIFT) |
            ((param & D3DSP_REGTYPE_MASK2) >> D3DSP_REGTYPE_SHIFT2));
}

/*****************************************************************************
 * IDirect3DVertexShader implementation structure
 */
typedef struct IWineD3DVertexShaderImpl {
    /* IUnknown parts*/   
    const IWineD3DVertexShaderVtbl *lpVtbl;
    LONG                        ref;     /* Note: Ref counting not required */

    /* IWineD3DBaseShader */
    IWineD3DBaseShaderClass     baseShader;

    /* IWineD3DVertexShaderImpl */
    IUnknown                    *parent;
    IWineD3DDeviceImpl          *wineD3DDevice;

    DWORD usage;

    /* vertex declaration array mapping */
    BOOL                        namedArrays;    /* don't map use named functions */
    BOOL                        declaredArrays; /* mapping requires */
    INT                         arrayUsageMap[WINED3DSHADERDECLUSAGE_MAX_USAGE];    /* lookup table for the maps */
    INT                         highestConstant;
    CHAR                        constantsUsedBitmap[256];
    /* FIXME: This needs to be populated with some flags of VS_CONSTANT_NOT_USED, VS_CONSTANT_CONSTANT, VS_CONSTANT_INTEGER, VS_CONSTANT_BOOLEAN, VS_CONSTANT_FLOAT, a half byte bitmap will be the best option, but I'll keep it as chards for siplicity */
    /* run time datas...  */
    VSHADERDATA                *data;
    IWineD3DVertexDeclaration  *vertexDeclaration;
#if 0 /* needs reworking */
    /* run time datas */
    VSHADERINPUTDATA input;
    VSHADEROUTPUTDATA output;
#endif
} IWineD3DVertexShaderImpl;
extern const SHADER_OPCODE IWineD3DVertexShaderImpl_shader_ins[];
extern const IWineD3DVertexShaderVtbl IWineD3DVertexShader_Vtbl;

/*****************************************************************************
 * IDirect3DPixelShader implementation structure
 */
typedef struct IWineD3DPixelShaderImpl {
    /* IUnknown parts */
    const IWineD3DPixelShaderVtbl *lpVtbl;
    LONG                        ref;     /* Note: Ref counting not required */

    /* IWineD3DBaseShader */
    IWineD3DBaseShaderClass     baseShader;

    /* IWineD3DPixelShaderImpl */
    IUnknown                   *parent;
    IWineD3DDeviceImpl         *wineD3DDevice;

    CHAR                        constants[WINED3D_PSHADER_MAX_CONSTANTS];

    /* run time data */
    PSHADERDATA                *data;

#if 0 /* needs reworking */
    PSHADERINPUTDATA input;
    PSHADEROUTPUTDATA output;
#endif
} IWineD3DPixelShaderImpl;

extern const SHADER_OPCODE IWineD3DPixelShaderImpl_shader_ins[];
extern const IWineD3DPixelShaderVtbl IWineD3DPixelShader_Vtbl;

/*****************************************************************************
 * IWineD3DPalette implementation structure
 */
struct IWineD3DPaletteImpl {
    /* IUnknown parts */
    const IWineD3DPaletteVtbl  *lpVtbl;
    LONG                       ref;

    IUnknown                   *parent;
    IWineD3DDeviceImpl         *wineD3DDevice;

    /* IWineD3DPalette */
    HPALETTE                   hpal;
    WORD                       palVersion;     /*|               */
    WORD                       palNumEntries;  /*|  LOGPALETTE   */
    PALETTEENTRY               palents[256];   /*|               */
    /* This is to store the palette in 'screen format' */
    int                        screen_palents[256];
    DWORD                      Flags;
};

extern const IWineD3DPaletteVtbl IWineD3DPalette_Vtbl;
DWORD IWineD3DPaletteImpl_Size(DWORD dwFlags);

/* DirectDraw utility functions */
extern WINED3DFORMAT pixelformat_for_depth(DWORD depth);

#endif
