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

/* GL related defines */
/* ------------------ */
#define GL_SUPPORT(ExtName)           (GLINFO_LOCATION.supported[ExtName] != 0)
#define GL_LIMITS(ExtName)            (GLINFO_LOCATION.max_##ExtName)
#define GL_EXTCALL(FuncName)          (GLINFO_LOCATION.FuncName)

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
#define HIGHEST_TRANSFORMSTATE 512 
                         /* Highest value in D3DTRANSFORMSTATETYPE */
#define HIGHEST_RENDER_STATE   209
                         /* Highest D3DRS_ value                   */
#define HIGHEST_TEXTURE_STATE   32
                         /* Highest D3DTSS_ value                  */
#define WINED3D_VSHADER_MAX_CONSTANTS  96   
                         /* Maximum number of constants provided to the shaders */
#define MAX_CLIPPLANES  D3DMAXUSERCLIPPLANES

#define MAX_PALETTES      256

/* Checking of API calls */
/* --------------------- */
#define checkGLcall(A) \
{ \
    GLint err = glGetError();   \
    if (err != GL_NO_ERROR) { \
       FIXME(">>>>>>>>>>>>>>>>> %x from %s @ %s / %d\n", err, A, __FILE__, __LINE__); \
    } else { \
       TRACE("%s call ok %s / %d\n", A, __FILE__, __LINE__); \
    } \
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
#define vcheckGLcall(A) \
{ \
    GLint err = glGetError();   \
    if (err != GL_NO_ERROR) { \
       FIXME(">>>>>>>>>>>>>>>>> %x from %s @ %s / %d\n", err, A, __FILE__, __LINE__); \
    } else { \
       VTRACE(("%s call ok %s / %d\n", A, __FILE__, __LINE__)); \
    } \
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
                    long  StartIdx,
                    short idxBytes,
                    const void *idxData,
                    int   minIndex);

/*****************************************************************************
 * Structures required to draw primitives 
 */

typedef struct Direct3DStridedData {
    BYTE     *lpData;        /* Pointer to start of data               */
    DWORD     dwStride;      /* Stride between occurances of this data */
    DWORD     dwType;        /* Type (as in D3DVSDT_TYPE)              */
} Direct3DStridedData;

typedef struct Direct3DVertexStridedData {
    union {
        struct {
             Direct3DStridedData  position;
             Direct3DStridedData  blendWeights;
             Direct3DStridedData  blendMatrixIndices;
             Direct3DStridedData  normal;
             Direct3DStridedData  pSize;
             Direct3DStridedData  diffuse;
             Direct3DStridedData  specular;
             Direct3DStridedData  texCoords[8];
        } s;
        Direct3DStridedData input[16];  /* Indexed by constants in D3DVSDE_REGISTER */
    } u;
} Direct3DVertexStridedData;

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
    IUnknown               *parent;
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
    D3DDEVICE_CREATION_PARAMETERS   createParms;
    D3DPRESENT_PARAMETERS           presentParms;
    UINT                            adapterNo;
    D3DDEVTYPE                      devType;

    /* Render Target Support */
    IWineD3DSurfaceImpl    *frontBuffer;
    IWineD3DSurfaceImpl    *backBuffer;
    IWineD3DSurfaceImpl    *depthStencilBuffer;

    IWineD3DSurfaceImpl    *renderTarget;
    IWineD3DSurfaceImpl    *stencilBufferTarget;

    /* palettes texture management */
    PALETTEENTRY                  palettes[MAX_PALETTES][256];
    UINT                          currentPalette;

    /* For rendering to a texture using glCopyTexImage */
    BOOL                          renderUpsideDown;

    /* Textures for when no other textures are mapped */
    UINT                          dummyTextureName[8];

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
    IWineD3DDeviceImpl     *wineD3DDevice;

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
    BOOL                    dirty;
    D3DFORMAT               format;

} IWineD3DBaseTextureClass;

typedef struct IWineD3DBaseTextureImpl
{
    /* IUnknown & WineD3DResource Information     */
    IWineD3DBaseTextureVtbl  *lpVtbl;
    IWineD3DResourceClass     resource;
    IWineD3DBaseTextureClass  baseTexture;

} IWineD3DBaseTextureImpl;

extern IWineD3DBaseTextureVtbl IWineD3DBaseTexture_Vtbl;

/*****************************************************************************
 * IWineD3DTexture implementation structure (extends IWineD3DBaseTextureImpl)
 */
typedef struct IWineD3DTextureImpl
{
    /* IUnknown & WineD3DResource/WineD3DBaseTexture Information     */
    IWineD3DTextureVtbl      *lpVtbl;
    IWineD3DResourceClass     resource;
    IWineD3DBaseTextureClass  baseTexture;

    /* IWineD3DTexture */
    IWineD3DSurfaceImpl      *surfaces[MAX_LEVELS];
    
    UINT                      width;
    UINT                      height;
    DWORD                     usage;

} IWineD3DTextureImpl;

extern IWineD3DTextureVtbl IWineD3DTexture_Vtbl;

/*****************************************************************************
 * IWineD3DCubeTexture implementation structure (extends IWineD3DBaseTextureImpl)
 */
typedef struct IWineD3DCubeTextureImpl
{
    /* IUnknown & WineD3DResource/WineD3DBaseTexture Information     */
    IWineD3DCubeTextureVtbl  *lpVtbl;
    IWineD3DResourceClass     resource;
    IWineD3DBaseTextureClass  baseTexture;

    /* IWineD3DCubeTexture */
    IWineD3DSurfaceImpl      *surfaces[6][MAX_LEVELS];

    UINT                      edgeLength;
    DWORD                     usage;
} IWineD3DCubeTextureImpl;

extern IWineD3DCubeTextureVtbl IWineD3DCubeTexture_Vtbl;

/*****************************************************************************
 * IWineD3DVolume implementation structure (extends IUnknown)
 */
typedef struct IWineD3DVolumeImpl
{
    /* IUnknown fields */
    IWineD3DVolumeVtbl     *lpVtbl;
    DWORD                   ref;     /* Note: Ref counting not required */

    /* WineD3DVolume Information */
    IUnknown               *parent;
    D3DRESOURCETYPE         resourceType;
    IWineD3DDeviceImpl     *wineD3DDevice;

    D3DVOLUME_DESC          currentDesc;
    UINT                    textureName;
    BYTE                   *allocatedMemory;
    IUnknown               *container;
    UINT                    bytesPerPixel;

    BOOL                    lockable;
    BOOL                    locked;
    D3DBOX                  lockedBox;
    D3DBOX                  dirtyBox;
    BOOL                    dirty;


} IWineD3DVolumeImpl;

extern IWineD3DVolumeVtbl IWineD3DVolume_Vtbl;

/*****************************************************************************
 * IWineD3DVolumeTexture implementation structure (extends IWineD3DBaseTextureImpl)
 */
typedef struct IWineD3DVolumeTextureImpl
{
    /* IUnknown & WineD3DResource/WineD3DBaseTexture Information     */
    IWineD3DVolumeTextureVtbl *lpVtbl;
    IWineD3DResourceClass     resource;
    IWineD3DBaseTextureClass  baseTexture;

    /* IWineD3DVolumeTexture */
    IWineD3DVolumeImpl       *volumes[MAX_LEVELS];

    UINT                      width;
    UINT                      height;
    UINT                      depth;
    DWORD                     usage;
} IWineD3DVolumeTextureImpl;

extern IWineD3DVolumeTextureVtbl IWineD3DVolumeTexture_Vtbl;

/*****************************************************************************
 * IWineD3DSurface implementation structure
 */
struct IWineD3DSurfaceImpl
{
    /* IUnknown & IWineD3DResource Information     */
    IWineD3DSurfaceVtbl      *lpVtbl;
    IWineD3DResourceClass     resource;

    /* IWineD3DSurface fields */
    IUnknown                 *container;
    D3DSURFACE_DESC           currentDesc;
    BYTE                     *allocatedMemory;

    UINT                      textureName;
    UINT                      bytesPerPixel;
    
    BOOL                      lockable;
    BOOL                      locked;
    RECT                      lockedRect;
    RECT                      dirtyRect;
    BOOL                      Dirty;
    BOOL                      inTexture;
    BOOL                      inPBuffer;
};

extern IWineD3DSurfaceVtbl IWineD3DSurface_Vtbl;

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
        BOOL                      stream_source[MAX_STREAMS];
        BOOL                      textures[8];
        BOOL                      transform[HIGHEST_TRANSFORMSTATE];
        BOOL                      viewport;
        BOOL                      renderState[HIGHEST_RENDER_STATE];
        BOOL                      textureState[8][HIGHEST_TEXTURE_STATE];
        BOOL                      clipplane[MAX_CLIPPLANES];
} SAVEDSTATES;

struct IWineD3DStateBlockImpl
{
    /* IUnknown fields */
    IWineD3DStateBlockVtbl   *lpVtbl;
    DWORD                     ref;     /* Note: Ref counting not required */
    
    /* IWineD3DStateBlock information */
    IUnknown                 *parent;
    IWineD3DDeviceImpl       *wineD3DDevice;
    D3DSTATEBLOCKTYPE         blockType;

    /* Array indicating whether things have been set or changed */
    SAVEDSTATES               changed;
    SAVEDSTATES               set;
  
    /* Drawing - Vertex Shader or FVF related */
    DWORD                     fvf;
    void                     *vertexShader; /* TODO: Replace void * with IWineD3DVertexShader * */
    BOOL                      streamIsUP;

    /* Stream Source */
    UINT                      stream_stride[MAX_STREAMS];
    UINT                      stream_offset[MAX_STREAMS];
    IWineD3DVertexBuffer     *stream_source[MAX_STREAMS];

    /* Indices */
    IWineD3DIndexBuffer*      pIndexData;
    UINT                      baseVertexIndex; /* Note: only used for d3d8 */

    /* Transform */
    D3DMATRIX                 transforms[HIGHEST_TRANSFORMSTATE];

    /* Lights */
    PLIGHTINFOEL             *lights; /* NOTE: active GL lights must be front of the chain */
    
    /* Clipping */
    double                    clipplane[MAX_CLIPPLANES][4];
    WINED3DCLIPSTATUS         clip_status;

    /* ViewPort */
    WINED3DVIEWPORT           viewport;

    /* Material */
    WINED3DMATERIAL           material;

    /* Indexed Vertex Blending */
    D3DVERTEXBLENDFLAGS       vertex_blend;
    FLOAT                     tween_factor;

    /* RenderState */
    DWORD                     renderState[HIGHEST_RENDER_STATE];

    /* Texture */
    IWineD3DBaseTexture      *textures[8];
    int                       textureDimensions[8];

    /* Texture State Stage */
    DWORD                     textureState[8][HIGHEST_TEXTURE_STATE];

};

extern IWineD3DStateBlockVtbl IWineD3DStateBlock_Vtbl;

/*****************************************************************************
 * Utility function prototypes 
 */

/* Trace routines */
const char* debug_d3dformat(D3DFORMAT fmt);
const char* debug_d3ddevicetype(D3DDEVTYPE devtype);
const char* debug_d3dresourcetype(D3DRESOURCETYPE res);
const char* debug_d3dusage(DWORD usage);
const char* debug_d3dprimitivetype(D3DPRIMITIVETYPE PrimitiveType);
const char* debug_d3drenderstate(DWORD state);
const char* debug_d3dtexturestate(DWORD state);
const char* debug_d3dpool(D3DPOOL pool);

/* Routines for GL <-> D3D values */
GLenum StencilOp(DWORD op);
void   set_tex_op(IWineD3DDevice *iface, BOOL isAlpha, int Stage, D3DTEXTUREOP op, DWORD arg1, DWORD arg2, DWORD arg3);
void   set_texture_matrix(const float *smat, DWORD flags);
void   GetSrcAndOpFromValue(DWORD iValue, BOOL isAlphaArg, GLenum* source, GLenum* operand);

SHORT  D3DFmtGetBpp(IWineD3DDeviceImpl* This, D3DFORMAT fmt);
GLenum D3DFmt2GLFmt(IWineD3DDeviceImpl* This, D3DFORMAT fmt);
GLenum D3DFmt2GLType(IWineD3DDeviceImpl *This, D3DFORMAT fmt);
GLint  D3DFmt2GLIntFmt(IWineD3DDeviceImpl* This, D3DFORMAT fmt);

/*****************************************************************************
 * To enable calling of inherited functions, requires prototypes 
 */
    /*** IUnknown methods ***/
    extern HRESULT WINAPI IWineD3DImpl_QueryInterface(IWineD3D *iface, REFIID riid, void** ppvObject);
    extern ULONG WINAPI IWineD3DImpl_AddRef(IWineD3D *iface);
    extern ULONG WINAPI IWineD3DImpl_Release(IWineD3D *iface);
    /*** IWineD3D methods ***/
    extern HRESULT WINAPI IWineD3DImpl_GetParent(IWineD3D *iface, IUnknown **pParent);
    extern UINT WINAPI IWineD3DImpl_GetAdapterCount(IWineD3D *iface);
    extern HRESULT WINAPI IWineD3DImpl_RegisterSoftwareDevice(IWineD3D *iface, void * pInitializeFunction);
    extern HMONITOR WINAPI IWineD3DImpl_GetAdapterMonitor(IWineD3D *iface, UINT Adapter);
    extern UINT WINAPI IWineD3DImpl_GetAdapterModeCount(IWineD3D *iface, UINT Adapter, D3DFORMAT Format);
    extern HRESULT WINAPI IWineD3DImpl_EnumAdapterModes(IWineD3D *iface, UINT  Adapter, UINT  Mode, D3DFORMAT Format, D3DDISPLAYMODE * pMode);
    extern HRESULT WINAPI IWineD3DImpl_GetAdapterDisplayMode(IWineD3D *iface, UINT  Adapter, D3DDISPLAYMODE * pMode);
    extern HRESULT WINAPI IWineD3DImpl_GetAdapterIdentifier(IWineD3D *iface, UINT Adapter, DWORD Flags, WINED3DADAPTER_IDENTIFIER* pIdentifier);
    extern HRESULT WINAPI IWineD3DImpl_CheckDeviceMultiSampleType(IWineD3D *iface, UINT  Adapter, D3DDEVTYPE  DeviceType, D3DFORMAT  SurfaceFormat, BOOL  Windowed, D3DMULTISAMPLE_TYPE  MultiSampleType, DWORD *pQuality);
    extern HRESULT WINAPI IWineD3DImpl_CheckDepthStencilMatch(IWineD3D *iface, UINT  Adapter, D3DDEVTYPE  DeviceType, D3DFORMAT  AdapterFormat, D3DFORMAT  RenderTargetFormat, D3DFORMAT  DepthStencilFormat);
    extern HRESULT WINAPI IWineD3DImpl_CheckDeviceType(IWineD3D *iface, UINT  Adapter, D3DDEVTYPE  CheckType, D3DFORMAT  DisplayFormat, D3DFORMAT  BackBufferFormat, BOOL  Windowed);
    extern HRESULT WINAPI IWineD3DImpl_CheckDeviceFormat(IWineD3D *iface, UINT  Adapter, D3DDEVTYPE  DeviceType, D3DFORMAT  AdapterFormat, DWORD  Usage, D3DRESOURCETYPE  RType, D3DFORMAT  CheckFormat);
    extern HRESULT WINAPI IWineD3DImpl_CheckDeviceFormatConversion(IWineD3D *iface, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SourceFormat, D3DFORMAT TargetFormat);
    extern HRESULT WINAPI IWineD3DImpl_GetDeviceCaps(IWineD3D *iface, UINT  Adapter, D3DDEVTYPE  DeviceType, void * pCaps);
    extern HRESULT WINAPI IWineD3DImpl_CreateDevice(IWineD3D *iface, UINT  Adapter, D3DDEVTYPE  DeviceType,HWND  hFocusWindow, DWORD  BehaviorFlags, WINED3DPRESENT_PARAMETERS * pPresentationParameters, IWineD3DDevice ** ppReturnedDeviceInterface, IUnknown *parent, D3DCB_CREATERENDERTARGETFN pFn);

    /*** IUnknown methods ***/
    extern HRESULT WINAPI IWineD3DDeviceImpl_QueryInterface(IWineD3DDevice *iface, REFIID riid, void** ppvObject);
    extern ULONG WINAPI IWineD3DDeviceImpl_AddRef(IWineD3DDevice *iface);
    extern ULONG WINAPI IWineD3DDeviceImpl_Release(IWineD3DDevice *iface);
    /*** IWineD3D methods ***/
    extern HRESULT WINAPI IWineD3DDeviceImpl_GetParent(IWineD3DDevice *iface, IUnknown **pParent);
    extern HRESULT WINAPI IWineD3DDeviceImpl_CreateVertexBuffer(IWineD3DDevice *iface, UINT  Length,DWORD  Usage,DWORD  FVF,D3DPOOL  Pool,IWineD3DVertexBuffer **ppVertexBuffer, HANDLE *sharedHandle, IUnknown *parent);
    extern HRESULT WINAPI IWineD3DDeviceImpl_CreateIndexBuffer(IWineD3DDevice *iface, UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IWineD3DIndexBuffer** ppIndexBuffer, HANDLE* pSharedHandle, IUnknown *parent);
    extern HRESULT WINAPI IWineD3DDeviceImpl_CreateStateBlock(IWineD3DDevice *iface, D3DSTATEBLOCKTYPE Type, IWineD3DStateBlock **ppStateBlock, IUnknown *parent);
    extern HRESULT WINAPI IWineD3DDeviceImpl_CreateRenderTarget(IWineD3DDevice *iface, UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Lockable, IWineD3DSurface** ppSurface, HANDLE* pSharedHandle, IUnknown *parent);
    extern HRESULT WINAPI IWineD3DDeviceImpl_CreateOffscreenPlainSurface(IWineD3DDevice *iface, UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, IWineD3DSurface** ppSurface, HANDLE* pSharedHandle, IUnknown *parent);
    extern HRESULT WINAPI IWineD3DDeviceImpl_CreateTexture(IWineD3DDevice *iface, UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IWineD3DTexture** ppTexture, HANDLE* pSharedHandle, IUnknown *parent, D3DCB_CREATESURFACEFN pFn);
    extern HRESULT WINAPI IWineD3DDeviceImpl_CreateVolumeTexture(IWineD3DDevice *iface, UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IWineD3DVolumeTexture** ppVolumeTexture, HANDLE* pSharedHandle, IUnknown *parent, D3DCB_CREATEVOLUMEFN pFn);
    extern HRESULT WINAPI IWineD3DDeviceImpl_CreateVolume(IWineD3DDevice *iface, UINT Width, UINT Height, UINT Depth, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IWineD3DVolume** ppVolumeTexture, HANDLE* pSharedHandle, IUnknown *parent);
    extern HRESULT WINAPI IWineD3DDeviceImpl_CreateCubeTexture(IWineD3DDevice *iface, UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IWineD3DCubeTexture** ppCubeTexture, HANDLE* pSharedHandle, IUnknown *parent, D3DCB_CREATESURFACEFN pFn);
    extern HRESULT WINAPI IWineD3DDeviceImpl_SetFVF(IWineD3DDevice *iface, DWORD  fvf);
    extern HRESULT WINAPI IWineD3DDeviceImpl_GetFVF(IWineD3DDevice *iface, DWORD * pfvf);
    extern HRESULT WINAPI IWineD3DDeviceImpl_SetStreamSource(IWineD3DDevice *iface, UINT  StreamNumber,IWineD3DVertexBuffer * pStreamData,UINT Offset,UINT  Stride);
    extern HRESULT WINAPI IWineD3DDeviceImpl_GetStreamSource(IWineD3DDevice *iface, UINT  StreamNumber,IWineD3DVertexBuffer ** ppStreamData,UINT *pOffset, UINT * pStride);
    extern HRESULT WINAPI IWineD3DDeviceImpl_SetTransform(IWineD3DDevice *iface, D3DTRANSFORMSTATETYPE  State,CONST D3DMATRIX * pMatrix);
    extern HRESULT WINAPI IWineD3DDeviceImpl_GetTransform(IWineD3DDevice *iface, D3DTRANSFORMSTATETYPE  State,D3DMATRIX * pMatrix);
    extern HRESULT WINAPI IWineD3DDeviceImpl_MultiplyTransform(IWineD3DDevice *iface, D3DTRANSFORMSTATETYPE  State, CONST D3DMATRIX * pMatrix);
    extern HRESULT WINAPI IWineD3DDeviceImpl_SetLight(IWineD3DDevice *iface, DWORD  Index,CONST WINED3DLIGHT * pLight);
    extern HRESULT WINAPI IWineD3DDeviceImpl_GetLight(IWineD3DDevice *iface, DWORD  Index,WINED3DLIGHT * pLight);
    extern HRESULT WINAPI IWineD3DDeviceImpl_SetLightEnable(IWineD3DDevice *iface, DWORD  Index,BOOL  Enable);
    extern HRESULT WINAPI IWineD3DDeviceImpl_GetLightEnable(IWineD3DDevice *iface, DWORD  Index,BOOL * pEnable);
    extern HRESULT WINAPI IWineD3DDeviceImpl_SetClipPlane(IWineD3DDevice *iface, DWORD  Index,CONST float * pPlane);
    extern HRESULT WINAPI IWineD3DDeviceImpl_GetClipPlane(IWineD3DDevice *iface, DWORD  Index,float * pPlane);
    extern HRESULT WINAPI IWineD3DDeviceImpl_SetClipStatus(IWineD3DDevice *iface, CONST WINED3DCLIPSTATUS * pClipStatus);
    extern HRESULT WINAPI IWineD3DDeviceImpl_GetClipStatus(IWineD3DDevice *iface, WINED3DCLIPSTATUS * pClipStatus);
    extern HRESULT WINAPI IWineD3DDeviceImpl_SetMaterial(IWineD3DDevice *iface, CONST WINED3DMATERIAL * pMaterial);
    extern HRESULT WINAPI IWineD3DDeviceImpl_GetMaterial(IWineD3DDevice *iface, WINED3DMATERIAL *pMaterial);
    extern HRESULT WINAPI IWineD3DDeviceImpl_SetIndices(IWineD3DDevice *iface, IWineD3DIndexBuffer * pIndexData,UINT  BaseVertexIndex);
    extern HRESULT WINAPI IWineD3DDeviceImpl_GetIndices(IWineD3DDevice *iface, IWineD3DIndexBuffer ** ppIndexData,UINT * pBaseVertexIndex);
    extern HRESULT WINAPI IWineD3DDeviceImpl_SetViewport(IWineD3DDevice *iface, CONST WINED3DVIEWPORT * pViewport);
    extern HRESULT WINAPI IWineD3DDeviceImpl_GetViewport(IWineD3DDevice *iface, WINED3DVIEWPORT * pViewport);
    extern HRESULT WINAPI IWineD3DDeviceImpl_SetRenderState(IWineD3DDevice *iface, D3DRENDERSTATETYPE  State,DWORD  Value);
    extern HRESULT WINAPI IWineD3DDeviceImpl_GetRenderState(IWineD3DDevice *iface, D3DRENDERSTATETYPE  State,DWORD * pValue);
    extern HRESULT WINAPI IWineD3DDeviceImpl_SetTextureStageState(IWineD3DDevice *iface, DWORD  Stage,D3DTEXTURESTAGESTATETYPE  Type,DWORD  Value);
    extern HRESULT WINAPI IWineD3DDeviceImpl_GetTextureStageState(IWineD3DDevice *iface, DWORD  Stage,D3DTEXTURESTAGESTATETYPE  Type,DWORD * pValue);
    extern HRESULT WINAPI IWineD3DDeviceImpl_BeginScene(IWineD3DDevice *iface);
    extern HRESULT WINAPI IWineD3DDeviceImpl_EndScene(IWineD3DDevice *iface);
    extern HRESULT WINAPI IWineD3DDeviceImpl_Present(IWineD3DDevice *iface, CONST RECT * pSourceRect,CONST RECT * pDestRect,HWND  hDestWindowOverride,CONST RGNDATA * pDirtyRegion);
    extern HRESULT WINAPI IWineD3DDeviceImpl_Clear(IWineD3DDevice *iface, DWORD  Count,CONST D3DRECT * pRects,DWORD  Flags,D3DCOLOR  Color,float  Z,DWORD  Stencil);
    extern HRESULT WINAPI IWineD3DDeviceImpl_DrawPrimitive(IWineD3DDevice *iface, D3DPRIMITIVETYPE  PrimitiveType,UINT  StartVertex,UINT  PrimitiveCount);
    extern HRESULT WINAPI IWineD3DDeviceImpl_DrawIndexedPrimitive(IWineD3DDevice *iface, D3DPRIMITIVETYPE  PrimitiveType,INT baseVIdx, UINT  minIndex,UINT  NumVertices,UINT  startIndex,UINT  primCount);
    extern HRESULT WINAPI IWineD3DDeviceImpl_DrawPrimitiveUP(IWineD3DDevice *iface, D3DPRIMITIVETYPE  PrimitiveType,UINT  PrimitiveCount,CONST void * pVertexStreamZeroData,UINT  VertexStreamZeroStride);
    extern HRESULT WINAPI IWineD3DDeviceImpl_DrawIndexedPrimitiveUP(IWineD3DDevice *iface, D3DPRIMITIVETYPE  PrimitiveType,UINT  MinVertexIndex,UINT  NumVertexIndices,UINT  PrimitiveCount,CONST void * pIndexData,D3DFORMAT  IndexDataFormat,CONST void * pVertexStreamZeroData,UINT  VertexStreamZeroStride);
    /*** Internal use IWineD3D methods ***/
    extern void WINAPI IWineD3DDeviceImpl_SetupTextureStates(IWineD3DDevice *iface, DWORD Stage, DWORD Flags);

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
    extern D3DRESOURCETYPE WINAPI IWineD3DResourceImpl_GetType(IWineD3DResource *iface);

    /*** IUnknown methods ***/
    extern HRESULT WINAPI IWineD3DVertexBufferImpl_QueryInterface(IWineD3DVertexBuffer *iface, REFIID riid, void** ppvObject);
    extern ULONG WINAPI IWineD3DVertexBufferImpl_AddRef(IWineD3DVertexBuffer *iface);
    extern ULONG WINAPI IWineD3DVertexBufferImpl_Release(IWineD3DVertexBuffer *iface);
    /*** IWineD3DResource methods ***/
    extern HRESULT WINAPI IWineD3DVertexBufferImpl_GetParent(IWineD3DVertexBuffer *iface, IUnknown **pParent);
    extern HRESULT WINAPI IWineD3DVertexBufferImpl_GetDevice(IWineD3DVertexBuffer *iface, IWineD3DDevice ** ppDevice);
    extern HRESULT WINAPI IWineD3DVertexBufferImpl_SetPrivateData(IWineD3DVertexBuffer *iface, REFGUID  refguid, CONST void * pData, DWORD  SizeOfData, DWORD  Flags);
    extern HRESULT WINAPI IWineD3DVertexBufferImpl_GetPrivateData(IWineD3DVertexBuffer *iface, REFGUID  refguid, void * pData, DWORD * pSizeOfData);
    extern HRESULT WINAPI IWineD3DVertexBufferImpl_FreePrivateData(IWineD3DVertexBuffer *iface, REFGUID  refguid);
    extern DWORD WINAPI IWineD3DVertexBufferImpl_SetPriority(IWineD3DVertexBuffer *iface, DWORD  PriorityNew);
    extern DWORD WINAPI IWineD3DVertexBufferImpl_GetPriority(IWineD3DVertexBuffer *iface);
    extern void WINAPI IWineD3DVertexBufferImpl_PreLoad(IWineD3DVertexBuffer *iface);
    extern D3DRESOURCETYPE WINAPI IWineD3DVertexBufferImpl_GetType(IWineD3DVertexBuffer *iface);
    /*** IWineD3DVertexBuffer methods ***/
    extern HRESULT WINAPI IWineD3DVertexBufferImpl_Lock(IWineD3DVertexBuffer *iface, UINT  OffsetToLock, UINT  SizeToLock, BYTE ** ppbData, DWORD  Flags);
    extern HRESULT WINAPI IWineD3DVertexBufferImpl_Unlock(IWineD3DVertexBuffer *iface);
    extern HRESULT WINAPI IWineD3DVertexBufferImpl_GetDesc(IWineD3DVertexBuffer *iface, D3DVERTEXBUFFER_DESC  * pDesc);

    /*** IUnknown methods ***/
    extern HRESULT WINAPI IWineD3DIndexBufferImpl_QueryInterface(IWineD3DIndexBuffer *iface, REFIID riid, void** ppvObject);
    extern ULONG WINAPI IWineD3DIndexBufferImpl_AddRef(IWineD3DIndexBuffer *iface);
    extern ULONG WINAPI IWineD3DIndexBufferImpl_Release(IWineD3DIndexBuffer *iface);
    /*** IWineD3DResource methods ***/
    extern HRESULT WINAPI IWineD3DIndexBufferImpl_GetParent(IWineD3DIndexBuffer *iface, IUnknown **pParent);
    extern HRESULT WINAPI IWineD3DIndexBufferImpl_GetDevice(IWineD3DIndexBuffer *iface, IWineD3DDevice ** ppDevice);
    extern HRESULT WINAPI IWineD3DIndexBufferImpl_SetPrivateData(IWineD3DIndexBuffer *iface, REFGUID  refguid, CONST void * pData, DWORD  SizeOfData, DWORD  Flags);
    extern HRESULT WINAPI IWineD3DIndexBufferImpl_GetPrivateData(IWineD3DIndexBuffer *iface, REFGUID  refguid, void * pData, DWORD * pSizeOfData);
    extern HRESULT WINAPI IWineD3DIndexBufferImpl_FreePrivateData(IWineD3DIndexBuffer *iface, REFGUID  refguid);
    extern DWORD WINAPI IWineD3DIndexBufferImpl_SetPriority(IWineD3DIndexBuffer *iface, DWORD  PriorityNew);
    extern DWORD WINAPI IWineD3DIndexBufferImpl_GetPriority(IWineD3DIndexBuffer *iface);
    extern void WINAPI IWineD3DIndexBufferImpl_PreLoad(IWineD3DIndexBuffer *iface);
    extern D3DRESOURCETYPE WINAPI IWineD3DIndexBufferImpl_GetType(IWineD3DIndexBuffer *iface);
    /*** IWineD3DIndexBuffer methods ***/
    extern HRESULT WINAPI IWineD3DIndexBufferImpl_Lock(IWineD3DIndexBuffer *iface, UINT  OffsetToLock, UINT  SizeToLock, BYTE ** ppbData, DWORD  Flags);
    extern HRESULT WINAPI IWineD3DIndexBufferImpl_Unlock(IWineD3DIndexBuffer *iface);
    extern HRESULT WINAPI IWineD3DIndexBufferImpl_GetDesc(IWineD3DIndexBuffer *iface, D3DINDEXBUFFER_DESC  * pDesc);

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
    extern D3DRESOURCETYPE WINAPI IWineD3DBaseTextureImpl_GetType(IWineD3DBaseTexture *iface);
    /*** IWineD3DBaseTexture methods ***/
    extern DWORD WINAPI IWineD3DBaseTextureImpl_SetLOD(IWineD3DBaseTexture *iface, DWORD LODNew);
    extern DWORD WINAPI IWineD3DBaseTextureImpl_GetLOD(IWineD3DBaseTexture *iface);
    extern DWORD WINAPI IWineD3DBaseTextureImpl_GetLevelCount(IWineD3DBaseTexture *iface);
    extern HRESULT WINAPI IWineD3DBaseTextureImpl_SetAutoGenFilterType(IWineD3DBaseTexture *iface, D3DTEXTUREFILTERTYPE FilterType);
    extern D3DTEXTUREFILTERTYPE WINAPI IWineD3DBaseTextureImpl_GetAutoGenFilterType(IWineD3DBaseTexture *iface);
    extern void WINAPI IWineD3DBaseTextureImpl_GenerateMipSubLevels(IWineD3DBaseTexture *iface);
    extern BOOL WINAPI IWineD3DBaseTextureImpl_SetDirty(IWineD3DBaseTexture *iface, BOOL);
    extern BOOL WINAPI IWineD3DBaseTextureImpl_GetDirty(IWineD3DBaseTexture *iface);

    /*** IUnknown methods ***/
    extern HRESULT WINAPI IWineD3DTextureImpl_QueryInterface(IWineD3DTexture *iface, REFIID riid, void** ppvObject);
    extern ULONG WINAPI IWineD3DTextureImpl_AddRef(IWineD3DTexture *iface);
    extern ULONG WINAPI IWineD3DTextureImpl_Release(IWineD3DTexture *iface);
    /*** IWineD3DResource methods ***/
    extern HRESULT WINAPI IWineD3DTextureImpl_GetParent(IWineD3DTexture *iface, IUnknown **pParent);
    extern HRESULT WINAPI IWineD3DTextureImpl_GetDevice(IWineD3DTexture *iface, IWineD3DDevice ** ppDevice);
    extern HRESULT WINAPI IWineD3DTextureImpl_SetPrivateData(IWineD3DTexture *iface, REFGUID  refguid, CONST void * pData, DWORD  SizeOfData, DWORD  Flags);
    extern HRESULT WINAPI IWineD3DTextureImpl_GetPrivateData(IWineD3DTexture *iface, REFGUID  refguid, void * pData, DWORD * pSizeOfData);
    extern HRESULT WINAPI IWineD3DTextureImpl_FreePrivateData(IWineD3DTexture *iface, REFGUID  refguid);
    extern DWORD WINAPI IWineD3DTextureImpl_SetPriority(IWineD3DTexture *iface, DWORD  PriorityNew);
    extern DWORD WINAPI IWineD3DTextureImpl_GetPriority(IWineD3DTexture *iface);
    extern void WINAPI IWineD3DTextureImpl_PreLoad(IWineD3DTexture *iface);
    extern D3DRESOURCETYPE WINAPI IWineD3DTextureImpl_GetType(IWineD3DTexture *iface);
    /*** IWineD3DBaseTexture methods ***/
    extern DWORD WINAPI IWineD3DTextureImpl_SetLOD(IWineD3DTexture *iface, DWORD LODNew);
    extern DWORD WINAPI IWineD3DTextureImpl_GetLOD(IWineD3DTexture *iface);
    extern DWORD WINAPI IWineD3DTextureImpl_GetLevelCount(IWineD3DTexture *iface);
    extern HRESULT WINAPI IWineD3DTextureImpl_SetAutoGenFilterType(IWineD3DTexture *iface, D3DTEXTUREFILTERTYPE FilterType);
    extern D3DTEXTUREFILTERTYPE WINAPI IWineD3DTextureImpl_GetAutoGenFilterType(IWineD3DTexture *iface);
    extern void WINAPI IWineD3DTextureImpl_GenerateMipSubLevels(IWineD3DTexture *iface);
    extern BOOL WINAPI IWineD3DTextureImpl_SetDirty(IWineD3DTexture *iface, BOOL);
    extern BOOL WINAPI IWineD3DTextureImpl_GetDirty(IWineD3DTexture *iface);
    /*** IWineD3DTexture methods ***/
    extern HRESULT WINAPI IWineD3DTextureImpl_GetLevelDesc(IWineD3DTexture *iface, UINT Level, WINED3DSURFACE_DESC* pDesc);
    extern HRESULT WINAPI IWineD3DTextureImpl_GetSurfaceLevel(IWineD3DTexture *iface, UINT Level, IWineD3DSurface** ppSurfaceLevel);
    extern HRESULT WINAPI IWineD3DTextureImpl_LockRect(IWineD3DTexture *iface, UINT Level, D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags);
    extern HRESULT WINAPI IWineD3DTextureImpl_UnlockRect(IWineD3DTexture *iface, UINT Level);
    extern HRESULT WINAPI IWineD3DTextureImpl_AddDirtyRect(IWineD3DTexture *iface, CONST RECT* pDirtyRect);

    /*** IUnknown methods ***/
    extern HRESULT WINAPI IWineD3DCubeTextureImpl_QueryInterface(IWineD3DCubeTexture *iface, REFIID riid, void** ppvObject);
    extern ULONG WINAPI IWineD3DCubeTextureImpl_AddRef(IWineD3DCubeTexture *iface);
    extern ULONG WINAPI IWineD3DCubeTextureImpl_Release(IWineD3DCubeTexture *iface);
    /*** IWineD3DResource methods ***/
    extern HRESULT WINAPI IWineD3DCubeTextureImpl_GetParent(IWineD3DCubeTexture *iface, IUnknown **pParent);
    extern HRESULT WINAPI IWineD3DCubeTextureImpl_GetDevice(IWineD3DCubeTexture *iface, IWineD3DDevice ** ppDevice);
    extern HRESULT WINAPI IWineD3DCubeTextureImpl_SetPrivateData(IWineD3DCubeTexture *iface, REFGUID  refguid, CONST void * pData, DWORD  SizeOfData, DWORD  Flags);
    extern HRESULT WINAPI IWineD3DCubeTextureImpl_GetPrivateData(IWineD3DCubeTexture *iface, REFGUID  refguid, void * pData, DWORD * pSizeOfData);
    extern HRESULT WINAPI IWineD3DCubeTextureImpl_FreePrivateData(IWineD3DCubeTexture *iface, REFGUID  refguid);
    extern DWORD WINAPI IWineD3DCubeTextureImpl_SetPriority(IWineD3DCubeTexture *iface, DWORD  PriorityNew);
    extern DWORD WINAPI IWineD3DCubeTextureImpl_GetPriority(IWineD3DCubeTexture *iface);
    extern void WINAPI IWineD3DCubeTextureImpl_PreLoad(IWineD3DCubeTexture *iface);
    extern D3DRESOURCETYPE WINAPI IWineD3DCubeTextureImpl_GetType(IWineD3DCubeTexture *iface);
    /*** IWineD3DBaseTexture methods ***/
    extern DWORD WINAPI IWineD3DCubeTextureImpl_SetLOD(IWineD3DCubeTexture *iface, DWORD LODNew);
    extern DWORD WINAPI IWineD3DCubeTextureImpl_GetLOD(IWineD3DCubeTexture *iface);
    extern DWORD WINAPI IWineD3DCubeTextureImpl_GetLevelCount(IWineD3DCubeTexture *iface);
    extern HRESULT WINAPI IWineD3DCubeTextureImpl_SetAutoGenFilterType(IWineD3DCubeTexture *iface, D3DTEXTUREFILTERTYPE FilterType);
    extern D3DTEXTUREFILTERTYPE WINAPI IWineD3DCubeTextureImpl_GetAutoGenFilterType(IWineD3DCubeTexture *iface);
    extern void WINAPI IWineD3DCubeTextureImpl_GenerateMipSubLevels(IWineD3DCubeTexture *iface);
    extern BOOL WINAPI IWineD3DCubeTextureImpl_SetDirty(IWineD3DCubeTexture *iface, BOOL);
    extern BOOL WINAPI IWineD3DCubeTextureImpl_GetDirty(IWineD3DCubeTexture *iface);
    /*** IWineD3DCubeTexture methods ***/
    extern HRESULT WINAPI IWineD3DCubeTextureImpl_GetLevelDesc(IWineD3DCubeTexture *iface, UINT Level,WINED3DSURFACE_DESC* pDesc);
    extern HRESULT WINAPI IWineD3DCubeTextureImpl_GetCubeMapSurface(IWineD3DCubeTexture *iface, D3DCUBEMAP_FACES FaceType, UINT Level, IWineD3DSurface** ppCubeMapSurface);
    extern HRESULT WINAPI IWineD3DCubeTextureImpl_LockRect(IWineD3DCubeTexture *iface, D3DCUBEMAP_FACES FaceType, UINT Level, D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags);
    extern HRESULT WINAPI IWineD3DCubeTextureImpl_UnlockRect(IWineD3DCubeTexture *iface, D3DCUBEMAP_FACES FaceType, UINT Level);
    extern HRESULT WINAPI IWineD3DCubeTextureImpl_AddDirtyRect(IWineD3DCubeTexture *iface, D3DCUBEMAP_FACES FaceType, CONST RECT* pDirtyRect);

    /*** IUnknown methods ***/
    extern HRESULT WINAPI IWineD3DVolumeTextureImpl_QueryInterface(IWineD3DVolumeTexture *iface, REFIID riid, void** ppvObject);
    extern ULONG WINAPI IWineD3DVolumeTextureImpl_AddRef(IWineD3DVolumeTexture *iface);
    extern ULONG WINAPI IWineD3DVolumeTextureImpl_Release(IWineD3DVolumeTexture *iface);
    /*** IWineD3DResource methods ***/
    extern HRESULT WINAPI IWineD3DVolumeTextureImpl_GetParent(IWineD3DVolumeTexture *iface, IUnknown **pParent);
    extern HRESULT WINAPI IWineD3DVolumeTextureImpl_GetDevice(IWineD3DVolumeTexture *iface, IWineD3DDevice ** ppDevice);
    extern HRESULT WINAPI IWineD3DVolumeTextureImpl_SetPrivateData(IWineD3DVolumeTexture *iface, REFGUID  refguid, CONST void * pData, DWORD  SizeOfData, DWORD  Flags);
    extern HRESULT WINAPI IWineD3DVolumeTextureImpl_GetPrivateData(IWineD3DVolumeTexture *iface, REFGUID  refguid, void * pData, DWORD * pSizeOfData);
    extern HRESULT WINAPI IWineD3DVolumeTextureImpl_FreePrivateData(IWineD3DVolumeTexture *iface, REFGUID  refguid);
    extern DWORD WINAPI IWineD3DVolumeTextureImpl_SetPriority(IWineD3DVolumeTexture *iface, DWORD  PriorityNew);
    extern DWORD WINAPI IWineD3DVolumeTextureImpl_GetPriority(IWineD3DVolumeTexture *iface);
    extern void WINAPI IWineD3DVolumeTextureImpl_PreLoad(IWineD3DVolumeTexture *iface);
    extern D3DRESOURCETYPE WINAPI IWineD3DVolumeTextureImpl_GetType(IWineD3DVolumeTexture *iface);
    /*** IWineD3DBaseTexture methods ***/
    extern DWORD WINAPI IWineD3DVolumeTextureImpl_SetLOD(IWineD3DVolumeTexture *iface, DWORD LODNew);
    extern DWORD WINAPI IWineD3DVolumeTextureImpl_GetLOD(IWineD3DVolumeTexture *iface);
    extern DWORD WINAPI IWineD3DVolumeTextureImpl_GetLevelCount(IWineD3DVolumeTexture *iface);
    extern HRESULT WINAPI IWineD3DVolumeTextureImpl_SetAutoGenFilterType(IWineD3DVolumeTexture *iface, D3DTEXTUREFILTERTYPE FilterType);
    extern D3DTEXTUREFILTERTYPE WINAPI IWineD3DVolumeTextureImpl_GetAutoGenFilterType(IWineD3DVolumeTexture *iface);
    extern void WINAPI IWineD3DVolumeTextureImpl_GenerateMipSubLevels(IWineD3DVolumeTexture *iface);
    extern BOOL WINAPI IWineD3DVolumeTextureImpl_SetDirty(IWineD3DVolumeTexture *iface, BOOL);
    extern BOOL WINAPI IWineD3DVolumeTextureImpl_GetDirty(IWineD3DVolumeTexture *iface);
    /*** IWineD3DVolumeTexture methods ***/
    extern HRESULT WINAPI IWineD3DVolumeTextureImpl_GetLevelDesc(IWineD3DVolumeTexture *iface, UINT Level, WINED3DVOLUME_DESC *pDesc);
    extern HRESULT WINAPI IWineD3DVolumeTextureImpl_GetVolumeLevel(IWineD3DVolumeTexture *iface, UINT Level, IWineD3DVolume** ppVolumeLevel);
    extern HRESULT WINAPI IWineD3DVolumeTextureImpl_LockBox(IWineD3DVolumeTexture *iface, UINT Level, D3DLOCKED_BOX* pLockedVolume, CONST D3DBOX* pBox, DWORD Flags);
    extern HRESULT WINAPI IWineD3DVolumeTextureImpl_UnlockBox(IWineD3DVolumeTexture *iface, UINT Level);
    extern HRESULT WINAPI IWineD3DVolumeTextureImpl_AddDirtyBox(IWineD3DVolumeTexture *iface, CONST D3DBOX* pDirtyBox);

    /*** IUnknown methods ***/
    extern HRESULT WINAPI IWineD3DSurfaceImpl_QueryInterface(IWineD3DSurface *iface, REFIID riid, void** ppvObject);
    extern ULONG WINAPI IWineD3DSurfaceImpl_AddRef(IWineD3DSurface *iface);
    extern ULONG WINAPI IWineD3DSurfaceImpl_Release(IWineD3DSurface *iface);
    /*** IWineD3DResource methods ***/
    extern HRESULT WINAPI IWineD3DSurfaceImpl_GetParent(IWineD3DSurface *iface, IUnknown **pParent);
    extern HRESULT WINAPI IWineD3DSurfaceImpl_GetDevice(IWineD3DSurface *iface, IWineD3DDevice ** ppDevice);
    extern HRESULT WINAPI IWineD3DSurfaceImpl_SetPrivateData(IWineD3DSurface *iface, REFGUID  refguid, CONST void * pData, DWORD  SizeOfData, DWORD  Flags);
    extern HRESULT WINAPI IWineD3DSurfaceImpl_GetPrivateData(IWineD3DSurface *iface, REFGUID  refguid, void * pData, DWORD * pSizeOfData);
    extern HRESULT WINAPI IWineD3DSurfaceImpl_FreePrivateData(IWineD3DSurface *iface, REFGUID  refguid);
    extern DWORD WINAPI IWineD3DSurfaceImpl_SetPriority(IWineD3DSurface *iface, DWORD  PriorityNew);
    extern DWORD WINAPI IWineD3DSurfaceImpl_GetPriority(IWineD3DSurface *iface);
    extern void WINAPI IWineD3DSurfaceImpl_PreLoad(IWineD3DSurface *iface);
    extern D3DRESOURCETYPE WINAPI IWineD3DSurfaceImpl_GetType(IWineD3DSurface *iface);
    /*** IWineD3DSurface methods ***/
    extern HRESULT WINAPI IWineD3DSurfaceImpl_GetContainer(IWineD3DSurface *iface, REFIID  riid, void ** ppContainer);
    extern HRESULT WINAPI IWineD3DSurfaceImpl_GetDesc(IWineD3DSurface *iface, WINED3DSURFACE_DESC * pDesc);
    extern HRESULT WINAPI IWineD3DSurfaceImpl_LockRect(IWineD3DSurface *iface, D3DLOCKED_RECT * pLockedRect, CONST RECT * pRect,DWORD  Flags);
    extern HRESULT WINAPI IWineD3DSurfaceImpl_UnlockRect(IWineD3DSurface *iface);
    extern HRESULT WINAPI IWineD3DSurfaceImpl_GetDC(IWineD3DSurface *iface, HDC *pHdc);
    extern HRESULT WINAPI IWineD3DSurfaceImpl_ReleaseDC(IWineD3DSurface *iface, HDC hdc);
    /* Internally used methods */
    extern HRESULT WINAPI IWineD3DSurfaceImpl_CleanDirtyRect(IWineD3DSurface *iface);
    extern HRESULT WINAPI IWineD3DSurfaceImpl_AddDirtyRect(IWineD3DSurface *iface, CONST RECT* pRect);
    extern HRESULT WINAPI IWineD3DSurfaceImpl_LoadTexture(IWineD3DSurface *iface, UINT gl_target, UINT gl_level);
    extern HRESULT WINAPI IWineD3DSurfaceImpl_SaveSnapshot(IWineD3DSurface *iface, const char *filename);

    /*** IUnknown methods ***/
    extern HRESULT WINAPI IWineD3DVolumeImpl_QueryInterface(IWineD3DVolume *iface, REFIID riid, void** ppvObject);
    extern ULONG WINAPI IWineD3DVolumeImpl_AddRef(IWineD3DVolume *iface);
    extern ULONG WINAPI IWineD3DVolumeImpl_Release(IWineD3DVolume *iface);
    /*** IWineD3DVolume methods ***/
    extern HRESULT WINAPI IWineD3DVolumeImpl_GetParent(IWineD3DVolume *iface, IUnknown **pParent);
    extern HRESULT WINAPI IWineD3DVolumeImpl_GetDevice(IWineD3DVolume *iface, IWineD3DDevice ** ppDevice);
    extern HRESULT WINAPI IWineD3DVolumeImpl_SetPrivateData(IWineD3DVolume *iface, REFGUID  refguid, CONST void * pData, DWORD  SizeOfData, DWORD  Flags);
    extern HRESULT WINAPI IWineD3DVolumeImpl_GetPrivateData(IWineD3DVolume *iface, REFGUID  refguid, void * pData, DWORD * pSizeOfData);
    extern HRESULT WINAPI IWineD3DVolumeImpl_FreePrivateData(IWineD3DVolume *iface, REFGUID  refguid);
    extern HRESULT WINAPI IWineD3DVolumeImpl_GetContainer(IWineD3DVolume *iface, REFIID  riid, void ** ppContainer);
    extern HRESULT WINAPI IWineD3DVolumeImpl_GetDesc(IWineD3DVolume *iface, WINED3DVOLUME_DESC * pDesc);
    extern HRESULT WINAPI IWineD3DVolumeImpl_LockBox(IWineD3DVolume *iface, D3DLOCKED_BOX* pLockedVolume, CONST D3DBOX* pBox, DWORD Flags);
    extern HRESULT WINAPI IWineD3DVolumeImpl_UnlockBox(IWineD3DVolume *iface);
    extern HRESULT WINAPI IWineD3DVolumeImpl_AddDirtyBox(IWineD3DVolume *iface, CONST D3DBOX* pDirtyBox);
    extern HRESULT WINAPI IWineD3DVolumeImpl_CleanDirtyBox(IWineD3DVolume *iface);

    /*** IUnknown methods ***/
    extern HRESULT WINAPI IWineD3DStateBlockImpl_QueryInterface(IWineD3DStateBlock *iface, REFIID riid, void** ppvObject);
    extern ULONG WINAPI IWineD3DStateBlockImpl_AddRef(IWineD3DStateBlock *iface);
    extern ULONG WINAPI IWineD3DStateBlockImpl_Release(IWineD3DStateBlock *iface);
    /*** IWineD3DStateBlock methods ***/
    extern HRESULT WINAPI IWineD3DStateBlockImpl_GetParent(IWineD3DStateBlock *iface, IUnknown **pParent);
    extern HRESULT WINAPI IWineD3DStateBlockImpl_InitStartupStateBlock(IWineD3DStateBlock *iface);


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
