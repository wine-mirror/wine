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
#define GLMULTITEXCOORD1F(a,b)                              \
            glMultiTexCoord1f(GL_TEXTURE0 + a, b);
#define GLMULTITEXCOORD2F(a,b,c)                            \
            glMultiTexCoord2f(GL_TEXTURE0 + a, b, c);
#define GLMULTITEXCOORD3F(a,b,c,d)                          \
            glMultiTexCoord3f(GL_TEXTURE0 + a, b, c, d);
#define GLMULTITEXCOORD4F(a,b,c,d,e)                        \
            glMultiTexCoord4f(GL_TEXTURE0 + a, b, c, d, e);
#else 
#define GLACTIVETEXTURE(textureNo)                             \
            glActiveTextureARB(GL_TEXTURE0_ARB + textureNo);   \
            checkGLcall("glActiveTextureARB");
#define GLMULTITEXCOORD1F(a,b)                                 \
            glMultiTexCoord1fARB(GL_TEXTURE0_ARB + a, b);
#define GLMULTITEXCOORD2F(a,b,c)                               \
            glMultiTexCoord2fARB(GL_TEXTURE0_ARB + a, b, c);
#define GLMULTITEXCOORD3F(a,b,c,d)                             \
            glMultiTexCoord3fARB(GL_TEXTURE0_ARB + a, b, c, d);
#define GLMULTITEXCOORD4F(a,b,c,d,e)                           \
            glMultiTexCoord4fARB(GL_TEXTURE0_ARB + a, b, c, d, e);
#endif

/* DirectX Device Limits */
/* --------------------- */
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

/* Routines for GL <-> D3D values */
GLenum StencilOp(DWORD op);
void   set_tex_op(IWineD3DDevice *iface, BOOL isAlpha, int Stage, D3DTEXTUREOP op, DWORD arg1, DWORD arg2, DWORD arg3);
void   set_texture_matrix(const float *smat, DWORD flags);
void   GetSrcAndOpFromValue(DWORD iValue, BOOL isAlphaArg, GLenum* source, GLenum* operand);

SHORT  D3DFmtGetBpp(IWineD3DDeviceImpl* This, D3DFORMAT fmt);
GLenum D3DFmt2GLFmt(IWineD3DDeviceImpl* This, D3DFORMAT fmt);
GLenum D3DFmt2GLType(IWineD3DDeviceImpl *This, D3DFORMAT fmt);
GLint  D3DFmt2GLIntFmt(IWineD3DDeviceImpl* This, D3DFORMAT fmt);


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
