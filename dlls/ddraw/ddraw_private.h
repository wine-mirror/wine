/*
 * Copyright 2000-2001 TransGaming Technologies Inc.
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_DLLS_DDRAW_DDRAW_PRIVATE_H
#define __WINE_DLLS_DDRAW_DDRAW_PRIVATE_H

/* MAY NOT CONTAIN X11 or DGA specific includes/defines/structs! */

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "wingdi.h"
#include "winuser.h"
#include "ddraw.h"
#include "d3d.h"
#include "ddcomimpl.h"
#include "ddrawi.h"

/* XXX Put this somewhere proper. */
#define DD_STRUCT_INIT(x)			\
	do {					\
		memset((x), 0, sizeof(*(x)));	\
		(x)->dwSize = sizeof(*x);	\
	} while (0)

#define DD_STRUCT_COPY_BYSIZE(to,from)			\
	do {						\
	    	DWORD __size = (to)->dwSize;		\
	    	DWORD __copysize = __size;		\
	    	DWORD __resetsize = __size;		\
		assert(to != from);                     \
	        if (__resetsize > sizeof(*to))		\
		    __resetsize = sizeof(*to);		\
	    	memset(to,0,__resetsize);               \
	        if ((from)->dwSize < __size) 		\
		    __copysize = (from)->dwSize;	\
		memcpy(to,from,__copysize);		\
		(to)->dwSize = __size;/*restore size*/	\
	} while (0)

#define MAKE_FOURCC(a,b,c,d) ((a << 0) | (b << 8) | (c << 16) | (d << 24))

/*****************************************************************************
 * IDirectDraw implementation structure
 */

typedef struct IDirectDrawImpl IDirectDrawImpl;
typedef struct IDirectDrawPaletteImpl IDirectDrawPaletteImpl;
typedef struct IDirectDrawClipperImpl IDirectDrawClipperImpl;
typedef struct IDirectDrawSurfaceImpl IDirectDrawSurfaceImpl;
typedef struct IDirect3DDeviceImpl IDirect3DDeviceImpl;

typedef void (*pixel_convert_func)(void *src, void *dst, DWORD width,
				   DWORD height, LONG pitch,
				   IDirectDrawPaletteImpl *palette);

typedef void (*palette_convert_func)(LPPALETTEENTRY palent,
				     void *screen_palette, DWORD start,
				     DWORD count);

struct IDirectDrawImpl
{
    ICOM_VFIELD_MULTI(IDirectDraw7);
    ICOM_VFIELD_MULTI(IDirectDraw4);
    ICOM_VFIELD_MULTI(IDirectDraw2);
    ICOM_VFIELD_MULTI(IDirectDraw);
    ICOM_VFIELD_MULTI(IDirect3D7);
    ICOM_VFIELD_MULTI(IDirect3D3);
    ICOM_VFIELD_MULTI(IDirect3D2);
    ICOM_VFIELD_MULTI(IDirect3D);

    LONG ref;

    /* TRUE if created via DirectDrawCreateEx or CoCreateInstance,
     * FALSE if created via DirectDrawCreate. */
    BOOL ex;

    /* Linked list of surfaces, joined by next_ddraw in IDirectSurfaceImpl. */
    IDirectDrawSurfaceImpl* surfaces;
    /* Linked list of palettes, joined by next_ddraw. */
    IDirectDrawPaletteImpl* palettes;
    /* Linked list of clippers, joined by next_ddraw. */
    IDirectDrawClipperImpl* clippers;

    IDirectDrawSurfaceImpl* primary_surface;

    DDRAWI_DIRECTDRAW_LCL local;
    DDCAPS caps;

    HWND window;
    DWORD cooperative_level;
    WNDPROC original_wndproc;

    DWORD width, height;
    LONG pitch;
    DDPIXELFORMAT pixelformat;
    DWORD cur_scanline;

    BOOL fake_vblank;
    
    /* Should each of these go into some structure? */
    DWORD orig_width, orig_height;
    LONG orig_pitch;
    DDPIXELFORMAT orig_pixelformat;

    /* Called when the refcount goes to 0. */
    void (*final_release)(IDirectDrawImpl *This);

    HRESULT (*set_exclusive_mode)(IDirectDrawImpl *This, DWORD dwExcl);

    HRESULT (*create_palette)(IDirectDrawImpl* This, DWORD dwFlags,
			      LPDIRECTDRAWPALETTE* ppPalette,
			      LPUNKNOWN pUnkOuter);

    /* Surface creation functions. For all of these, pOuter == NULL. */

    /* Do not create any backbuffers or the flipping chain. */
    HRESULT (*create_primary)(IDirectDrawImpl* This,
			      const DDSURFACEDESC2* pDDSD,
			      LPDIRECTDRAWSURFACE7* ppSurf, LPUNKNOWN pOuter);

    /* Primary may be NULL if we are creating an unattached backbuffer. */
    HRESULT (*create_backbuffer)(IDirectDrawImpl* This,
				 const DDSURFACEDESC2* pDDSD,
				 LPDIRECTDRAWSURFACE7* ppSurf,
				 LPUNKNOWN pOuter,
				 IDirectDrawSurfaceImpl* primary);

    /* shiny happy offscreenplain surfaces */
    HRESULT (*create_offscreen)(IDirectDrawImpl* This,
				const DDSURFACEDESC2* pDDSD,
				LPDIRECTDRAWSURFACE7* ppSurf,
				LPUNKNOWN pOuter);

    /* dwMipMapLevel is specified as per OpenGL. (i.e. 0 is base) */
    HRESULT (*create_texture)(IDirectDrawImpl* This,
			      const DDSURFACEDESC2* pDDSD,
			      LPDIRECTDRAWSURFACE7* ppSurf, LPUNKNOWN pOuter,
			      DWORD dwMipMapLevel);

    HRESULT (*create_zbuffer)(IDirectDrawImpl* This,
			      const DDSURFACEDESC2* pDDSD,
			      LPDIRECTDRAWSURFACE7* ppSurf, LPUNKNOWN pOuter);

    LPVOID	private;

    /* Everything below here is still questionable. */

    DDPIXELFORMAT screen_pixelformat;

    int           pixmap_depth;
    pixel_convert_func pixel_convert;
    palette_convert_func palette_convert;

    /* Use to fool some too strict games */
    INT32 (*allocate_memory)(IDirectDrawImpl *This, DWORD mem);
    void (*free_memory)(IDirectDrawImpl *This, DWORD mem);
    DWORD total_vidmem, available_vidmem;
    
    /* IDirect3D fields */
    LPVOID d3d_private;

    /* Used as a callback function to create a texture */
    HRESULT (*d3d_create_texture)(IDirectDrawImpl *d3d, IDirectDrawSurfaceImpl *tex, BOOLEAN at_creation, IDirectDrawSurfaceImpl *main);

    /* Used as a callback for Devices to tell to the D3D object it's been created */
    HRESULT (*d3d_added_device)(IDirectDrawImpl *d3d, IDirect3DDeviceImpl *device);
    HRESULT (*d3d_removed_device)(IDirectDrawImpl *d3d, IDirect3DDeviceImpl *device);

    /* This is needed for delayed texture creation and Z buffer blits */
    IDirect3DDeviceImpl *current_device;

    /* This is for the fake mainWindow */
    ATOM	winclass;
    PAINTSTRUCT	ps;
    BOOL	paintable;
};

/*****************************************************************************
 * IDirectDrawPalette implementation structure
 */
struct IDirectDrawPaletteImpl
{
    /* IUnknown fields */
    ICOM_VFIELD_MULTI(IDirectDrawPalette);
    LONG ref;

    DDRAWI_DDRAWPALETTE_LCL local;
    DDRAWI_DDRAWPALETTE_GBL global;

    /* IDirectDrawPalette fields */
    HPALETTE		hpal;
    WORD		palVersion, palNumEntries; /* LOGPALETTE */
    PALETTEENTRY	palents[256];
    /* This is to store the palette in 'screen format' */
    int			screen_palents[256];

    VOID (*final_release)(IDirectDrawPaletteImpl* This);

    IDirectDrawImpl* ddraw_owner;
    IDirectDrawPaletteImpl* prev_ddraw;
    IDirectDrawPaletteImpl* next_ddraw;

    LPVOID		private;
};

/*****************************************************************************
 * IDirectDrawClipper implementation structure
 */
struct IDirectDrawClipperImpl
{
    /* IUnknown fields */
    ICOM_VFIELD_MULTI(IDirectDrawClipper);
    LONG ref;

    /* IDirectDrawClipper fields */
    HWND hWnd;

    IDirectDrawImpl* ddraw_owner;
    IDirectDrawClipperImpl* prev_ddraw;
    IDirectDrawClipperImpl* next_ddraw;
};

/*****************************************************************************
 * IDirectDrawSurface implementation structure
 */

struct IDirectDrawSurfaceImpl
{
    /* IUnknown fields */
    ICOM_VFIELD_MULTI(IDirectDrawSurface7);
    ICOM_VFIELD_MULTI(IDirectDrawSurface3);
    ICOM_VFIELD_MULTI(IDirectDrawGammaControl);
    ICOM_VFIELD_MULTI(IDirect3DTexture2);
    ICOM_VFIELD_MULTI(IDirect3DTexture);
    LONG ref;

    struct IDirectDrawSurfaceImpl* attached; /* attached surfaces */

    struct IDirectDrawSurfaceImpl* next_ddraw; /* ddraw surface chain */
    struct IDirectDrawSurfaceImpl* prev_ddraw;
    struct IDirectDrawSurfaceImpl* next_attached; /* attached surface chain */
    struct IDirectDrawSurfaceImpl* prev_attached;

    IDirectDrawImpl* ddraw_owner;
    IDirectDrawSurfaceImpl* surface_owner;

    IDirectDrawPaletteImpl* palette; /* strong ref */
    IDirectDrawClipperImpl* clipper; /* strong ref */

    DDRAWI_DDRAWSURFACE_LCL local;
    DDRAWI_DDRAWSURFACE_MORE more;
    /* FIXME: since Flip should swap the GBL structures, they should
     * probably not be embedded into the IDirectDrawSurfaceImpl structure... */
    LPDDRAWI_DDRAWSURFACE_GBL_MORE gmore;
    DDRAWI_DDRAWSURFACE_GBL global;
    DDRAWI_DDRAWSURFACE_GBL_MORE global_more;

    DDSURFACEDESC2 surface_desc;

    HDC hDC;
    RECT lastlockrect;
    DWORD lastlocktype;
    BOOL dc_in_use;
    BOOL locked;

    HRESULT (*duplicate_surface)(IDirectDrawSurfaceImpl* src,
				 LPDIRECTDRAWSURFACE7* dst);
    void (*final_release)(IDirectDrawSurfaceImpl *This);
    HRESULT (*late_allocate)(IDirectDrawSurfaceImpl *This);
    BOOL (*attach)(IDirectDrawSurfaceImpl *This, IDirectDrawSurfaceImpl *to);
    BOOL (*detach)(IDirectDrawSurfaceImpl *This);
    void (*lock_update)(IDirectDrawSurfaceImpl* This, LPCRECT pRect, DWORD dwFlags);
    void (*unlock_update)(IDirectDrawSurfaceImpl* This, LPCRECT pRect);
    void (*lose_surface)(IDirectDrawSurfaceImpl* This);
    BOOL (*flip_data)(IDirectDrawSurfaceImpl* front,
		      IDirectDrawSurfaceImpl* back,
		      DWORD dwFlags);
    void (*flip_update)(IDirectDrawSurfaceImpl* front, DWORD dwFlags);
    HRESULT (*get_dc)(IDirectDrawSurfaceImpl* This, HDC* phDC);
    HRESULT (*release_dc)(IDirectDrawSurfaceImpl* This, HDC hDC);
    void (*set_palette)(IDirectDrawSurfaceImpl* This, IDirectDrawPaletteImpl* pal);
    void (*update_palette)(IDirectDrawSurfaceImpl* This, IDirectDrawPaletteImpl* pal,
			   DWORD dwStart, DWORD dwCount, LPPALETTEENTRY palent);
    HWND (*get_display_window)(IDirectDrawSurfaceImpl *This);
    HRESULT (*get_gamma_ramp)(IDirectDrawSurfaceImpl *This, DWORD dwFlags, LPDDGAMMARAMP lpGammaRamp);
    HRESULT (*set_gamma_ramp)(IDirectDrawSurfaceImpl *This, DWORD dwFlags, LPDDGAMMARAMP lpGammaRamp);

    struct PrivateData* private_data;

    DWORD max_lod;
    DWORD priority;

    BOOL lost;

    DWORD uniqueness_value;

    LPVOID private;

    /* Everything below here is dodgy. */
    /* For Direct3D use */
    LPVOID aux_ctx, aux_data;
    void (*aux_release)(LPVOID ctx, LPVOID data);
    BOOL (*aux_flip)(LPVOID ctx, LPVOID data);
    void (*aux_unlock)(LPVOID ctx, LPVOID data, LPRECT lpRect);
    HRESULT (*aux_blt)(struct IDirectDrawSurfaceImpl *This, LPRECT rdst, LPDIRECTDRAWSURFACE7 src, LPRECT rsrc, DWORD dwFlags, LPDDBLTFX lpbltfx);
    HRESULT (*aux_bltfast)(struct IDirectDrawSurfaceImpl *This, DWORD dstx, DWORD dsty, LPDIRECTDRAWSURFACE7 src, LPRECT rsrc, DWORD trans);
    HRESULT (*aux_setcolorkey_cb)(struct IDirectDrawSurfaceImpl *texture, DWORD dwFlags, LPDDCOLORKEY ckey );
    /* This is to get the D3DDevice object associated to this surface */
    struct IDirect3DDeviceImpl *d3ddevice;
    /* This is for texture */
    IDirectDrawSurfaceImpl *mip_main;
    int mipmap_level;
    LPVOID tex_private;
    void (*lock_update_prev)(IDirectDrawSurfaceImpl* This, LPCRECT pRect, DWORD dwFlags);
    void (*unlock_update_prev)(IDirectDrawSurfaceImpl* This, LPCRECT pRect);
    BOOLEAN (*get_dirty_status)(IDirectDrawSurfaceImpl* This, LPCRECT pRect);
};

/*****************************************************************************
 * Driver initialisation functions.
 */
BOOL DDRAW_HAL_Init(HINSTANCE, DWORD, LPVOID);
BOOL DDRAW_User_Init(HINSTANCE, DWORD, LPVOID);

typedef struct {
    const DDDEVICEIDENTIFIER2* info;
    int	preference;	/* how good we are. dga might get 100, xlib 50*/
    HRESULT (*create)(const GUID*, LPDIRECTDRAW7*, LPUNKNOWN, BOOL ex);

    /* For IDirectDraw7::Initialize. */
    HRESULT (*init)(IDirectDrawImpl *, const GUID*);
} ddraw_driver;

void DDRAW_register_driver(const ddraw_driver*);

const ddraw_driver* DDRAW_FindDriver(const GUID* guid);

/******************************************************************************
 * Random utilities
 */

/* Get DDSCAPS of surface (shortcutmacro) */
#define SDDSCAPS(iface) ((iface)->s.surface_desc.ddsCaps.dwCaps)
/* Get the number of bytes per pixel for a given surface */
#define PFGET_BPP(pf) (pf.dwFlags&DDPF_PALETTEINDEXED8?1:((pf.u1.dwRGBBitCount+7)/8))
#define GET_BPP(desc) PFGET_BPP(desc.u4.ddpfPixelFormat)

LONG DDRAW_width_bpp_to_pitch(DWORD width, DWORD bpp);

typedef struct {
    unsigned short	bpp,depth;
    unsigned int	rmask,gmask,bmask;
} ConvertMode;

typedef struct {
    void (*pixel_convert)(void *src, void *dst, DWORD width, DWORD height, LONG pitch, IDirectDrawPaletteImpl* palette);
    void (*palette_convert)(LPPALETTEENTRY palent, void *screen_palette, DWORD start, DWORD count);
} ConvertFuncs;

typedef struct {
    ConvertMode screen, dest;
    ConvertFuncs funcs;
} Convert;

extern Convert ModeEmulations[8];
extern int _common_depth_to_pixelformat(DWORD depth,LPDIRECTDRAW ddraw);
extern BOOL opengl_initialized;
extern BOOL s3tc_initialized;

typedef void (*FUNC_FETCH_2D_TEXEL_RGBA_DXT1)(int srcRowStride, const BYTE *pixdata, int i, int j, void *texel);
typedef void (*FUNC_FETCH_2D_TEXEL_RGBA_DXT3)(int srcRowStride, const BYTE *pixdata, int i, int j, void *texel);
typedef void (*FUNC_FETCH_2D_TEXEL_RGBA_DXT5)(int srcRowStride, const BYTE *pixdata, int i, int j, void *texel);

extern FUNC_FETCH_2D_TEXEL_RGBA_DXT1 fetch_2d_texel_rgba_dxt1;
extern FUNC_FETCH_2D_TEXEL_RGBA_DXT3 fetch_2d_texel_rgba_dxt3;
extern FUNC_FETCH_2D_TEXEL_RGBA_DXT5 fetch_2d_texel_rgba_dxt5;

/******************************************************************************
 * Structure conversion (for thunks)
 */
void DDRAW_Convert_DDSCAPS_1_To_2(const DDSCAPS* pIn, DDSCAPS2* pOut);
void DDRAW_Convert_DDDEVICEIDENTIFIER_2_To_1(const DDDEVICEIDENTIFIER2* pIn,
					     DDDEVICEIDENTIFIER* pOut);

/******************************************************************************
 * Debugging / Flags output functions
 */
extern void DDRAW_dump_DDBLTFX(DWORD flagmask);
extern void DDRAW_dump_DDBLTFAST(DWORD flagmask);
extern void DDRAW_dump_DDBLT(DWORD flagmask);
extern void DDRAW_dump_DDSCAPS(const DDSCAPS *in);
extern void DDRAW_dump_DDSCAPS2(const DDSCAPS2 *in);
extern void DDRAW_dump_pixelformat_flag(DWORD flagmask);
extern void DDRAW_dump_paletteformat(DWORD dwFlags);
extern void DDRAW_dump_pixelformat(const DDPIXELFORMAT *in);
extern void DDRAW_dump_colorkeyflag(DWORD ck);
extern void DDRAW_dump_surface_desc(const DDSURFACEDESC2 *lpddsd);
extern void DDRAW_dump_cooperativelevel(DWORD cooplevel);
extern void DDRAW_dump_lockflag(DWORD lockflag);
extern void DDRAW_dump_DDCOLORKEY(const DDCOLORKEY *in);
extern void DDRAW_dump_DDCAPS(const DDCAPS *lpcaps);
extern void DDRAW_dump_DDENUMSURFACES(DWORD flagmask);
extern void DDRAW_dump_surface_to_disk(IDirectDrawSurfaceImpl *surface, FILE *f, int scale) ;

/* Used for generic dumping */
typedef struct
{
    DWORD val;
    const char* name;
} flag_info;

#define FE(x) { x, #x }

typedef struct
{
    DWORD val;
    const char* name;
    void (*func)(const void *);
    ptrdiff_t offset;
} member_info;

#define DDRAW_dump_flags(flags,names,num_names) DDRAW_dump_flags_(flags, names, num_names, 1)
#define ME(x,f,e) { x, #x, (void (*)(const void *))(f), offsetof(STRUCT, e) }

extern void DDRAW_dump_flags_(DWORD flags, const flag_info* names, size_t num_names, int newline);
extern void DDRAW_dump_members(DWORD flags, const void* data, const member_info* mems, size_t num_mems);

void DirectDrawSurface_RegisterClass(void);
void DirectDrawSurface_UnregisterClass(void);

extern const IDirectDrawSurface3Vtbl DDRAW_IDDS3_Thunk_VTable;

/*****************************************************************************
 * IDirectDrawClipper declarations
 */
HRESULT DDRAW_CreateClipper(IUnknown* pUnkOuter, REFIID riid, LPVOID* ppObj);
void Main_DirectDrawClipper_ForceDestroy(IDirectDrawClipperImpl* This);

HRESULT WINAPI
Main_DirectDrawClipper_SetHwnd(LPDIRECTDRAWCLIPPER iface, DWORD dwFlags,
			       HWND hWnd);
ULONG WINAPI Main_DirectDrawClipper_Release(LPDIRECTDRAWCLIPPER iface);
HRESULT WINAPI
Main_DirectDrawClipper_GetClipList(LPDIRECTDRAWCLIPPER iface, LPRECT lpRect,
				   LPRGNDATA lpClipList, LPDWORD lpdwSize);
HRESULT WINAPI
Main_DirectDrawClipper_SetClipList(LPDIRECTDRAWCLIPPER iface,LPRGNDATA lprgn,
				   DWORD dwFlag);
HRESULT WINAPI
Main_DirectDrawClipper_QueryInterface(LPDIRECTDRAWCLIPPER iface, REFIID riid,
				      LPVOID* ppvObj);
ULONG WINAPI Main_DirectDrawClipper_AddRef( LPDIRECTDRAWCLIPPER iface );
HRESULT WINAPI
Main_DirectDrawClipper_GetHWnd(LPDIRECTDRAWCLIPPER iface, HWND* hWndPtr);
HRESULT WINAPI
Main_DirectDrawClipper_Initialize(LPDIRECTDRAWCLIPPER iface, LPDIRECTDRAW lpDD,
				  DWORD dwFlags);
HRESULT WINAPI
Main_DirectDrawClipper_IsClipListChanged(LPDIRECTDRAWCLIPPER iface,
					 BOOL* lpbChanged);

/*****************************************************************************
 * IDirectDrawPalette MAIN declarations
 */
HRESULT Main_DirectDrawPalette_Construct(IDirectDrawPaletteImpl* This,
					 IDirectDrawImpl* pDD, DWORD dwFlags);
void Main_DirectDrawPalette_final_release(IDirectDrawPaletteImpl* This);

HRESULT Main_DirectDrawPalette_Create(IDirectDrawImpl* pDD, DWORD dwFlags,
				      LPDIRECTDRAWPALETTE* ppPalette,
				      LPUNKNOWN pUnkOuter);
void Main_DirectDrawPalette_ForceDestroy(IDirectDrawPaletteImpl* This);

DWORD Main_DirectDrawPalette_Size(DWORD dwFlags);

HRESULT WINAPI
Main_DirectDrawPalette_GetEntries(LPDIRECTDRAWPALETTE iface, DWORD dwFlags,
				  DWORD dwStart, DWORD dwCount,
				  LPPALETTEENTRY palent);
HRESULT WINAPI
Main_DirectDrawPalette_SetEntries(LPDIRECTDRAWPALETTE iface, DWORD dwFlags,
				  DWORD dwStart, DWORD dwCount,
				  LPPALETTEENTRY palent);
ULONG WINAPI
Main_DirectDrawPalette_Release(LPDIRECTDRAWPALETTE iface);
ULONG WINAPI Main_DirectDrawPalette_AddRef(LPDIRECTDRAWPALETTE iface);
HRESULT WINAPI
Main_DirectDrawPalette_Initialize(LPDIRECTDRAWPALETTE iface,
				  LPDIRECTDRAW ddraw, DWORD dwFlags,
				  LPPALETTEENTRY palent);
HRESULT WINAPI
Main_DirectDrawPalette_GetCaps(LPDIRECTDRAWPALETTE iface, LPDWORD lpdwCaps);
HRESULT WINAPI
Main_DirectDrawPalette_QueryInterface(LPDIRECTDRAWPALETTE iface,
				      REFIID refiid, LPVOID *obj);

/*****************************************************************************
 * IDirectDrawPalette HAL declarations
 */
HRESULT HAL_DirectDrawPalette_Construct(IDirectDrawPaletteImpl* This,
					 IDirectDrawImpl* pDD, DWORD dwFlags);
void HAL_DirectDrawPalette_final_release(IDirectDrawPaletteImpl* This);

HRESULT HAL_DirectDrawPalette_Create(IDirectDrawImpl* pDD, DWORD dwFlags,
				     LPDIRECTDRAWPALETTE* ppPalette,
				     LPUNKNOWN pUnkOuter);

HRESULT WINAPI
HAL_DirectDrawPalette_SetEntries(LPDIRECTDRAWPALETTE iface, DWORD dwFlags,
				 DWORD dwStart, DWORD dwCount,
				 LPPALETTEENTRY palent);

/*****************************************************************************
 * IDirectDraw MAIN declarations
 */
/* internal virtual functions */
void Main_DirectDraw_final_release(IDirectDrawImpl* This);
HRESULT Main_create_offscreen(IDirectDrawImpl* This, const DDSURFACEDESC2 *pDDSD,
			      LPDIRECTDRAWSURFACE7* ppSurf, LPUNKNOWN pOuter);
HRESULT Main_create_texture(IDirectDrawImpl* This, const DDSURFACEDESC2 *pDDSD,
			    LPDIRECTDRAWSURFACE7* ppSurf, LPUNKNOWN pOuter,
			    DWORD dwMipMapLevel);
HRESULT Main_create_zbuffer(IDirectDrawImpl* This, const DDSURFACEDESC2 *pDDSD,
			    LPDIRECTDRAWSURFACE7* ppSurf, LPUNKNOWN pOuter);

/* internal functions */
HRESULT Main_DirectDraw_Construct(IDirectDrawImpl *This, BOOL ex);
void Main_DirectDraw_AddSurface(IDirectDrawImpl* This,
				IDirectDrawSurfaceImpl* surface);
void Main_DirectDraw_RemoveSurface(IDirectDrawImpl* This,
				   IDirectDrawSurfaceImpl* surface);
void Main_DirectDraw_AddClipper(IDirectDrawImpl* This,
				IDirectDrawClipperImpl* clipper);
void Main_DirectDraw_RemoveClipper(IDirectDrawImpl* This,
				   IDirectDrawClipperImpl* clipper);
void Main_DirectDraw_AddPalette(IDirectDrawImpl* This,
				IDirectDrawPaletteImpl* palette);
void Main_DirectDraw_RemovePalette(IDirectDrawImpl* This,
				   IDirectDrawPaletteImpl* palette);

/* interface functions */
ULONG WINAPI Main_DirectDraw_AddRef(LPDIRECTDRAW7 iface);
ULONG WINAPI Main_DirectDraw_Release(LPDIRECTDRAW7 iface);
HRESULT WINAPI Main_DirectDraw_QueryInterface(LPDIRECTDRAW7 iface,
					      REFIID refiid,LPVOID *obj);
HRESULT WINAPI Main_DirectDraw_Compact(LPDIRECTDRAW7 iface);
HRESULT WINAPI Main_DirectDraw_CreateClipper(LPDIRECTDRAW7 iface,
					     DWORD dwFlags,
					     LPDIRECTDRAWCLIPPER *ppClipper,
					     IUnknown *pUnkOuter);
HRESULT WINAPI
Main_DirectDraw_CreatePalette(LPDIRECTDRAW7 iface, DWORD dwFlags,
			      LPPALETTEENTRY palent,
			      LPDIRECTDRAWPALETTE* ppPalette,
			      LPUNKNOWN pUnknown);
HRESULT WINAPI
Main_DirectDraw_CreateSurface(LPDIRECTDRAW7 iface, LPDDSURFACEDESC2 pDDSD,
			      LPDIRECTDRAWSURFACE7 *ppSurf,
			      IUnknown *pUnkOuter);
HRESULT WINAPI
Main_DirectDraw_DuplicateSurface(LPDIRECTDRAW7 iface, LPDIRECTDRAWSURFACE7 src,
				 LPDIRECTDRAWSURFACE7* dst);
HRESULT WINAPI
Main_DirectDraw_EnumSurfaces(LPDIRECTDRAW7 iface, DWORD dwFlags,
			     LPDDSURFACEDESC2 lpDDSD2, LPVOID context,
			     LPDDENUMSURFACESCALLBACK7 callback);
HRESULT WINAPI
Main_DirectDraw_EvaluateMode(LPDIRECTDRAW7 iface,DWORD a,DWORD* b);
HRESULT WINAPI Main_DirectDraw_FlipToGDISurface(LPDIRECTDRAW7 iface);
HRESULT WINAPI
Main_DirectDraw_GetCaps(LPDIRECTDRAW7 iface, LPDDCAPS pDriverCaps,
			LPDDCAPS pHELCaps);
HRESULT WINAPI
Main_DirectDraw_GetFourCCCodes(LPDIRECTDRAW7 iface, LPDWORD pNumCodes,
			       LPDWORD pCodes);
HRESULT WINAPI
Main_DirectDraw_GetGDISurface(LPDIRECTDRAW7 iface,
			      LPDIRECTDRAWSURFACE7 *lplpGDIDDSSurface);
HRESULT WINAPI
Main_DirectDraw_GetMonitorFrequency(LPDIRECTDRAW7 iface,LPDWORD freq);
HRESULT WINAPI
Main_DirectDraw_GetScanLine(LPDIRECTDRAW7 iface, LPDWORD lpdwScanLine);
HRESULT WINAPI
Main_DirectDraw_GetSurfaceFromDC(LPDIRECTDRAW7 iface, HDC hdc,
				 LPDIRECTDRAWSURFACE7 *lpDDS);
HRESULT WINAPI
Main_DirectDraw_GetVerticalBlankStatus(LPDIRECTDRAW7 iface, LPBOOL status);
HRESULT WINAPI
Main_DirectDraw_Initialize(LPDIRECTDRAW7 iface, LPGUID lpGuid);
HRESULT WINAPI Main_DirectDraw_RestoreAllSurfaces(LPDIRECTDRAW7 iface);
HRESULT WINAPI
Main_DirectDraw_SetCooperativeLevel(LPDIRECTDRAW7 iface, HWND hwnd,
				    DWORD cooplevel);
HRESULT WINAPI
Main_DirectDraw_SetDisplayMode(LPDIRECTDRAW7 iface, DWORD dwWidth,
			       DWORD dwHeight, LONG lPitch,
			       DWORD dwRefreshRate, DWORD dwFlags,
			       const DDPIXELFORMAT* pixelformat);
HRESULT WINAPI Main_DirectDraw_RestoreDisplayMode(LPDIRECTDRAW7 iface);
HRESULT WINAPI
Main_DirectDraw_WaitForVerticalBlank(LPDIRECTDRAW7 iface, DWORD dwFlags,
				     HANDLE h);
HRESULT WINAPI
Main_DirectDraw_GetDisplayMode(LPDIRECTDRAW7 iface, LPDDSURFACEDESC2 pDDSD);
HRESULT WINAPI
Main_DirectDraw_GetAvailableVidMem(LPDIRECTDRAW7 iface,LPDDSCAPS2 ddscaps,
				   LPDWORD total, LPDWORD free);
HRESULT WINAPI Main_DirectDraw_TestCooperativeLevel(LPDIRECTDRAW7 iface);
HRESULT WINAPI
Main_DirectDraw_StartModeTest(LPDIRECTDRAW7 iface, LPSIZE pModes,
			      DWORD dwNumModes, DWORD dwFlags);

/*****************************************************************************
 * IDirectDraw USER object declarations
 */
#define USER_DDRAW_PRIV(ddraw) ((User_DirectDrawImpl*)((ddraw)->private))
#define USER_DDRAW_PRIV_VAR(name,ddraw) \
	User_DirectDrawImpl* name = USER_DDRAW_PRIV(ddraw)

typedef struct
{
    int empty;
    /* empty */
} User_DirectDrawImpl_Part;

typedef struct
{
    User_DirectDrawImpl_Part user;
} User_DirectDrawImpl;

void User_DirectDraw_final_release(IDirectDrawImpl* This);
HRESULT User_DirectDraw_create_primary(IDirectDrawImpl* This,
				       const DDSURFACEDESC2* pDDSD,
				       LPDIRECTDRAWSURFACE7* ppSurf,
				       LPUNKNOWN pOuter);
HRESULT User_DirectDraw_create_backbuffer(IDirectDrawImpl* This,
					  const DDSURFACEDESC2* pDDSD,
					  LPDIRECTDRAWSURFACE7* ppSurf,
					  LPUNKNOWN pOuter,
					  IDirectDrawSurfaceImpl* primary);
HRESULT User_DirectDraw_Construct(IDirectDrawImpl *This, BOOL ex);
HRESULT User_DirectDraw_Create(const GUID* pGUID, LPDIRECTDRAW7* pIface,
			       IUnknown* pUnkOuter, BOOL ex);

HRESULT WINAPI
User_DirectDraw_EnumDisplayModes(LPDIRECTDRAW7 iface, DWORD dwFlags,
				 LPDDSURFACEDESC2 pDDSD, LPVOID context,
				 LPDDENUMMODESCALLBACK2 callback);
HRESULT WINAPI
User_DirectDraw_GetDeviceIdentifier(LPDIRECTDRAW7 iface,
				    LPDDDEVICEIDENTIFIER2 pDDDI,
				    DWORD dwFlags);
HRESULT WINAPI
User_DirectDraw_SetDisplayMode(LPDIRECTDRAW7 iface, DWORD dwWidth,
			       DWORD dwHeight, DWORD dwBPP,
			       DWORD dwRefreshRate, DWORD dwFlags);

/*****************************************************************************
 * IDirectDraw HAL declarations
 */
#define HAL_DDRAW_PRIV(ddraw) \
	((HAL_DirectDrawImpl*)((ddraw)->private))
#define HAL_DDRAW_PRIV_VAR(name,ddraw) \
	HAL_DirectDrawImpl* name = HAL_DDRAW_PRIV(ddraw)

typedef struct
{
    DWORD next_vofs;
} HAL_DirectDrawImpl_Part;

typedef struct
{
    User_DirectDrawImpl_Part user;
    HAL_DirectDrawImpl_Part hal;
} HAL_DirectDrawImpl;

void HAL_DirectDraw_final_release(IDirectDrawImpl* This);
HRESULT HAL_DirectDraw_create_primary(IDirectDrawImpl* This,
				      const DDSURFACEDESC2* pDDSD,
				      LPDIRECTDRAWSURFACE7* ppSurf,
				      LPUNKNOWN pOuter);
HRESULT HAL_DirectDraw_create_backbuffer(IDirectDrawImpl* This,
					 const DDSURFACEDESC2* pDDSD,
					 LPDIRECTDRAWSURFACE7* ppSurf,
					 LPUNKNOWN pOuter,
					 IDirectDrawSurfaceImpl* primary);
HRESULT HAL_DirectDraw_create_texture(IDirectDrawImpl* This,
				      const DDSURFACEDESC2* pDDSD,
				      LPDIRECTDRAWSURFACE7* ppSurf,
				      LPUNKNOWN pOuter,
				      DWORD dwMipMapLevel);

HRESULT HAL_DirectDraw_Construct(IDirectDrawImpl *This, BOOL ex);
HRESULT HAL_DirectDraw_Create(const GUID* pGUID, LPDIRECTDRAW7* pIface,
				   IUnknown* pUnkOuter, BOOL ex);

HRESULT WINAPI
HAL_DirectDraw_GetDeviceIdentifier(LPDIRECTDRAW7 iface,
					LPDDDEVICEIDENTIFIER2 pDDDI,
					DWORD dwFlags);
HRESULT WINAPI
HAL_DirectDraw_SetDisplayMode(LPDIRECTDRAW7 iface, DWORD dwWidth,
				   DWORD dwHeight, DWORD dwBPP,
				   DWORD dwRefreshRate, DWORD dwFlags);
HRESULT WINAPI HAL_DirectDraw_RestoreDisplayMode(LPDIRECTDRAW7 iface);

/*****************************************************************************
 * IDirectDrawSurface MAIN declarations
 */
/* Support for IDirectDrawSurface7::Set/Get/FreePrivateData. I don't think
 * anybody uses it for much so a good implementation is optional. */
typedef struct PrivateData
{
    struct PrivateData* next;
    struct PrivateData* prev;

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

extern const IDirectDrawGammaControlVtbl DDRAW_IDDGC_VTable;

/* Non-interface functions */
HRESULT Main_DirectDrawSurface_Construct(IDirectDrawSurfaceImpl* This,
					 IDirectDrawImpl* pDD,
					 const DDSURFACEDESC2* pDDSD);
void Main_DirectDrawSurface_ForceDestroy(IDirectDrawSurfaceImpl* This);

void Main_DirectDrawSurface_final_release(IDirectDrawSurfaceImpl* This);
HRESULT Main_DirectDrawSurface_late_allocate(IDirectDrawSurfaceImpl* This);
BOOL Main_DirectDrawSurface_attach(IDirectDrawSurfaceImpl *This,
				   IDirectDrawSurfaceImpl *to);
BOOL Main_DirectDrawSurface_detach(IDirectDrawSurfaceImpl *This);
void Main_DirectDrawSurface_lock_update(IDirectDrawSurfaceImpl* This,
					LPCRECT pRect, DWORD dwFlags);
void Main_DirectDrawSurface_unlock_update(IDirectDrawSurfaceImpl* This,
					  LPCRECT pRect);
void Main_DirectDrawSurface_lose_surface(IDirectDrawSurfaceImpl* This);
void Main_DirectDrawSurface_set_palette(IDirectDrawSurfaceImpl* This,
					IDirectDrawPaletteImpl* pal);
void Main_DirectDrawSurface_update_palette(IDirectDrawSurfaceImpl* This,
					   IDirectDrawPaletteImpl* pal,
					   DWORD dwStart, DWORD dwCount,
			                   LPPALETTEENTRY palent);
HWND Main_DirectDrawSurface_get_display_window(IDirectDrawSurfaceImpl* This);

HRESULT Main_DirectDrawSurface_get_gamma_ramp(IDirectDrawSurfaceImpl* This,
					      DWORD dwFlags,
					      LPDDGAMMARAMP lpGammaRamp);
HRESULT Main_DirectDrawSurface_set_gamma_ramp(IDirectDrawSurfaceImpl* This,
					      DWORD dwFlags,
					      LPDDGAMMARAMP lpGammaRamp);

BOOL Main_DirectDrawSurface_flip_data(IDirectDrawSurfaceImpl* front,
				      IDirectDrawSurfaceImpl* back,
				      DWORD dwFlags);

#define CHECK_LOST(This)					\
	do {							\
		if (This->lost) return DDERR_SURFACELOST;	\
	} while (0)

#define CHECK_TEXTURE(This)					\
	do {							\
		if (!(This->surface_desc.ddsCaps.dwCaps2	\
		      & DDSCAPS2_TEXTUREMANAGE))		\
			return DDERR_INVALIDOBJECT;		\
	} while (0)

#define LOCK_OBJECT(This) do { } while (0)
#define UNLOCK_OBJECT(This) do { } while (0)

/* IDirectDrawSurface7 (partial) implementation */
ULONG WINAPI Main_DirectDrawSurface_AddRef(LPDIRECTDRAWSURFACE7 iface);
ULONG WINAPI Main_DirectDrawSurface_Release(LPDIRECTDRAWSURFACE7 iface);
HRESULT WINAPI
Main_DirectDrawSurface_QueryInterface(LPDIRECTDRAWSURFACE7 iface, REFIID riid,
				      LPVOID* ppObj);
HRESULT WINAPI
Main_DirectDrawSurface_AddAttachedSurface(LPDIRECTDRAWSURFACE7 iface,
					  LPDIRECTDRAWSURFACE7 pAttach);
HRESULT WINAPI
Main_DirectDrawSurface_AddOverlayDirtyRect(LPDIRECTDRAWSURFACE7 iface,
					   LPRECT pRect);
HRESULT WINAPI
Main_DirectDrawSurface_BltBatch(LPDIRECTDRAWSURFACE7 iface,
				LPDDBLTBATCH pBatch, DWORD dwCount,
				DWORD dwFlags);
HRESULT WINAPI
Main_DirectDrawSurface_ChangeUniquenessValue(LPDIRECTDRAWSURFACE7 iface);
HRESULT WINAPI
Main_DirectDrawSurface_DeleteAttachedSurface(LPDIRECTDRAWSURFACE7 iface,
					     DWORD dwFlags,
					     LPDIRECTDRAWSURFACE7 pAttach);
HRESULT WINAPI
Main_DirectDrawSurface_EnumAttachedSurfaces(LPDIRECTDRAWSURFACE7 iface,
					    LPVOID context,
					    LPDDENUMSURFACESCALLBACK7 cb);
HRESULT WINAPI
Main_DirectDrawSurface_EnumOverlayZOrders(LPDIRECTDRAWSURFACE7 iface,
					  DWORD dwFlags, LPVOID context,
					  LPDDENUMSURFACESCALLBACK7 cb);
HRESULT WINAPI
Main_DirectDrawSurface_Flip(LPDIRECTDRAWSURFACE7 iface,
			    LPDIRECTDRAWSURFACE7 override, DWORD dwFlags);
HRESULT WINAPI
Main_DirectDrawSurface_FreePrivateData(LPDIRECTDRAWSURFACE7 iface,
				       REFGUID tag);
HRESULT WINAPI
Main_DirectDrawSurface_GetAttachedSurface(LPDIRECTDRAWSURFACE7 iface,
					  LPDDSCAPS2 pCaps,
					  LPDIRECTDRAWSURFACE7* ppSurface);
HRESULT WINAPI
Main_DirectDrawSurface_GetBltStatus(LPDIRECTDRAWSURFACE7 iface,
				    DWORD dwFlags);
HRESULT WINAPI
Main_DirectDrawSurface_GetCaps(LPDIRECTDRAWSURFACE7 iface,
			       LPDDSCAPS2 pCaps);
HRESULT WINAPI
Main_DirectDrawSurface_GetClipper(LPDIRECTDRAWSURFACE7 iface,
				  LPDIRECTDRAWCLIPPER* ppClipper);
HRESULT WINAPI
Main_DirectDrawSurface_GetColorKey(LPDIRECTDRAWSURFACE7 iface,
				   DWORD dwFlags, LPDDCOLORKEY pCKey);
HRESULT WINAPI
Main_DirectDrawSurface_GetDC(LPDIRECTDRAWSURFACE7 iface, HDC *phDC);
HRESULT WINAPI
Main_DirectDrawSurface_GetDDInterface(LPDIRECTDRAWSURFACE7 iface,
				      LPVOID* pDD);
HRESULT WINAPI
Main_DirectDrawSurface_GetFlipStatus(LPDIRECTDRAWSURFACE7 iface,
				     DWORD dwFlags);
HRESULT WINAPI
Main_DirectDrawSurface_GetLOD(LPDIRECTDRAWSURFACE7 iface,
			      LPDWORD pdwMaxLOD);
HRESULT WINAPI
Main_DirectDrawSurface_GetOverlayPosition(LPDIRECTDRAWSURFACE7 iface,
					  LPLONG pX, LPLONG pY);
HRESULT WINAPI
Main_DirectDrawSurface_GetPalette(LPDIRECTDRAWSURFACE7 iface,
				  LPDIRECTDRAWPALETTE* ppPalette);
HRESULT WINAPI
Main_DirectDrawSurface_GetPixelFormat(LPDIRECTDRAWSURFACE7 iface,
				      LPDDPIXELFORMAT pDDPixelFormat);
HRESULT WINAPI
Main_DirectDrawSurface_GetPriority(LPDIRECTDRAWSURFACE7 iface,
				   LPDWORD pdwPriority);
HRESULT WINAPI
Main_DirectDrawSurface_GetPrivateData(LPDIRECTDRAWSURFACE7 iface, REFGUID tag,
				      LPVOID pBuffer, LPDWORD pcbBufferSize);
HRESULT WINAPI
Main_DirectDrawSurface_GetSurfaceDesc(LPDIRECTDRAWSURFACE7 iface,
				      LPDDSURFACEDESC2 pDDSD);
HRESULT WINAPI
Main_DirectDrawSurface_GetUniquenessValue(LPDIRECTDRAWSURFACE7 iface,
					  LPDWORD pValue);
HRESULT WINAPI
Main_DirectDrawSurface_Initialize(LPDIRECTDRAWSURFACE7 iface,
				  LPDIRECTDRAW pDD, LPDDSURFACEDESC2 pDDSD);
HRESULT WINAPI
Main_DirectDrawSurface_IsLost(LPDIRECTDRAWSURFACE7 iface);
HRESULT WINAPI
Main_DirectDrawSurface_Lock(LPDIRECTDRAWSURFACE7 iface, LPRECT prect,
			    LPDDSURFACEDESC2 pDDSD, DWORD flags, HANDLE h);
HRESULT WINAPI
Main_DirectDrawSurface_PageLock(LPDIRECTDRAWSURFACE7 iface, DWORD dwFlags);
HRESULT WINAPI
Main_DirectDrawSurface_PageUnlock(LPDIRECTDRAWSURFACE7 iface, DWORD dwFlags);
HRESULT WINAPI
Main_DirectDrawSurface_ReleaseDC(LPDIRECTDRAWSURFACE7 iface, HDC hDC);
HRESULT WINAPI
Main_DirectDrawSurface_SetClipper(LPDIRECTDRAWSURFACE7 iface,
				  LPDIRECTDRAWCLIPPER pDDClipper);
HRESULT WINAPI
Main_DirectDrawSurface_SetColorKey(LPDIRECTDRAWSURFACE7 iface,
				   DWORD dwFlags, LPDDCOLORKEY pCKey);
HRESULT WINAPI
Main_DirectDrawSurface_SetLOD(LPDIRECTDRAWSURFACE7 iface, DWORD dwMaxLOD);
HRESULT WINAPI
Main_DirectDrawSurface_SetOverlayPosition(LPDIRECTDRAWSURFACE7 iface,
					  LONG X, LONG Y);
HRESULT WINAPI
Main_DirectDrawSurface_SetPalette(LPDIRECTDRAWSURFACE7 iface,
				  LPDIRECTDRAWPALETTE pPalette);
HRESULT WINAPI
Main_DirectDrawSurface_SetPriority(LPDIRECTDRAWSURFACE7 iface,
				   DWORD dwPriority);
HRESULT WINAPI
Main_DirectDrawSurface_SetPrivateData(LPDIRECTDRAWSURFACE7 iface,
				      REFGUID tag, LPVOID pData,
				      DWORD cbSize, DWORD dwFlags);
HRESULT WINAPI
Main_DirectDrawSurface_Unlock(LPDIRECTDRAWSURFACE7 iface, LPRECT pRect);
HRESULT WINAPI
Main_DirectDrawSurface_UpdateOverlay(LPDIRECTDRAWSURFACE7 iface,
				     LPRECT pSrcRect,
				     LPDIRECTDRAWSURFACE7 pDstSurface,
				     LPRECT pDstRect, DWORD dwFlags,
				     LPDDOVERLAYFX pFX);
HRESULT WINAPI
Main_DirectDrawSurface_UpdateOverlayDisplay(LPDIRECTDRAWSURFACE7 iface,
					    DWORD dwFlags);
HRESULT WINAPI
Main_DirectDrawSurface_UpdateOverlayZOrder(LPDIRECTDRAWSURFACE7 iface,
					   DWORD dwFlags,
					   LPDIRECTDRAWSURFACE7 pDDSRef);

/*****************************************************************************
 * IDirectDrawSurface DIB declarations
 */
#define DIB_PRIV(surf) ((DIB_DirectDrawSurfaceImpl*)((surf)->private))

#define DIB_PRIV_VAR(name, surf) \
	DIB_DirectDrawSurfaceImpl* name = DIB_PRIV(surf)

struct DIB_DirectDrawSurfaceImpl_Part
{
    HBITMAP DIBsection;
    void* bitmap_data;
    HGDIOBJ holdbitmap;
    BOOL client_memory;
    DWORD d3d_data[4]; /* room for Direct3D driver data */
};

typedef struct
{
    struct DIB_DirectDrawSurfaceImpl_Part dib;
} DIB_DirectDrawSurfaceImpl;

HRESULT
DIB_DirectDrawSurface_Construct(IDirectDrawSurfaceImpl *This,
				IDirectDrawImpl *pDD,
				const DDSURFACEDESC2 *pDDSD);
HRESULT
DIB_DirectDrawSurface_Create(IDirectDrawImpl *pDD,
			     const DDSURFACEDESC2 *pDDSD,
			     LPDIRECTDRAWSURFACE7 *ppSurf,
			     IUnknown *pUnkOuter);

void DIB_DirectDrawSurface_final_release(IDirectDrawSurfaceImpl* This);
BOOL DIB_DirectDrawSurface_flip_data(IDirectDrawSurfaceImpl* front,
				     IDirectDrawSurfaceImpl* back,
				     DWORD dwFlags);

void DIB_DirectDrawSurface_set_palette(IDirectDrawSurfaceImpl* This,
				       IDirectDrawPaletteImpl* pal);
void DIB_DirectDrawSurface_update_palette(IDirectDrawSurfaceImpl* This,
					  IDirectDrawPaletteImpl* pal,
					  DWORD dwStart, DWORD dwCount,
					  LPPALETTEENTRY palent);

HRESULT DIB_DirectDrawSurface_get_dc(IDirectDrawSurfaceImpl* This, HDC* phDC);
HRESULT DIB_DirectDrawSurface_release_dc(IDirectDrawSurfaceImpl* This,HDC hDC);

HRESULT DIB_DirectDrawSurface_alloc_dc(IDirectDrawSurfaceImpl* This,HDC* phDC);
HRESULT DIB_DirectDrawSurface_free_dc(IDirectDrawSurfaceImpl* This, HDC hDC);

HRESULT WINAPI
DIB_DirectDrawSurface_Blt(LPDIRECTDRAWSURFACE7 iface, LPRECT prcDest,
			  LPDIRECTDRAWSURFACE7 pSrcSurf, LPRECT prcSrc,
			  DWORD dwFlags, LPDDBLTFX pBltFx);
HRESULT WINAPI
DIB_DirectDrawSurface_BltFast(LPDIRECTDRAWSURFACE7 iface, DWORD dwX,
			      DWORD dwY, LPDIRECTDRAWSURFACE7 pSrcSurf,
			      LPRECT prcSrc, DWORD dwTrans);
HRESULT WINAPI DIB_DirectDrawSurface_Restore(LPDIRECTDRAWSURFACE7 iface);
HRESULT WINAPI
DIB_DirectDrawSurface_SetSurfaceDesc(LPDIRECTDRAWSURFACE7 iface,
				     LPDDSURFACEDESC2 pDDSD, DWORD dwFlags);

/*****************************************************************************
 * IDirectDrawSurface USER declarations
 */
#define USER_PRIV(surf) ((User_DirectDrawSurfaceImpl*)((surf)->private))

#define USER_PRIV_VAR(name,surf) \
	User_DirectDrawSurfaceImpl* name = USER_PRIV(surf)

struct User_DirectDrawSurfaceImpl_Part
{
    HWND window;
    HDC cached_dc;
    HANDLE update_thread, update_event, refresh_event;
    volatile int wait_count, in_refresh;
    CRITICAL_SECTION crit;
};

typedef struct
{
    struct DIB_DirectDrawSurfaceImpl_Part dib;
    struct User_DirectDrawSurfaceImpl_Part user;
} User_DirectDrawSurfaceImpl;

HRESULT User_DirectDrawSurface_Construct(IDirectDrawSurfaceImpl* This,
					 IDirectDrawImpl* pDD,
					 const DDSURFACEDESC2* pDDSD);

HRESULT User_DirectDrawSurface_Create(IDirectDrawImpl *pDD,
				      const DDSURFACEDESC2 *pDDSD,
				      LPDIRECTDRAWSURFACE7 *ppSurf,
				      IUnknown *pUnkOuter);

void User_DirectDrawSurface_final_release(IDirectDrawSurfaceImpl* This);

void User_DirectDrawSurface_lock_update(IDirectDrawSurfaceImpl* This,
					LPCRECT pRect, DWORD dwFlags);
void User_DirectDrawSurface_unlock_update(IDirectDrawSurfaceImpl* This,
					  LPCRECT pRect);
void User_DirectDrawSurface_set_palette(IDirectDrawSurfaceImpl* This,
					IDirectDrawPaletteImpl* pal);
void User_DirectDrawSurface_update_palette(IDirectDrawSurfaceImpl* This,
					   IDirectDrawPaletteImpl* pal,
					   DWORD dwStart, DWORD dwCount,
					   LPPALETTEENTRY palent);
HRESULT User_DirectDrawSurface_duplicate_surface(IDirectDrawSurfaceImpl* This,
						 LPDIRECTDRAWSURFACE7* ppDup);
BOOL User_DirectDrawSurface_flip_data(IDirectDrawSurfaceImpl* front,
				      IDirectDrawSurfaceImpl* back,
				      DWORD dwFlags);
void User_DirectDrawSurface_flip_update(IDirectDrawSurfaceImpl* This,
					DWORD dwFlags);
HWND User_DirectDrawSurface_get_display_window(IDirectDrawSurfaceImpl* This);

HRESULT User_DirectDrawSurface_get_dc(IDirectDrawSurfaceImpl* This, HDC* phDC);
HRESULT User_DirectDrawSurface_release_dc(IDirectDrawSurfaceImpl* This,
					  HDC hDC);

HRESULT User_DirectDrawSurface_get_gamma_ramp(IDirectDrawSurfaceImpl* This,
					      DWORD dwFlags,
					      LPDDGAMMARAMP lpGammaRamp);
HRESULT User_DirectDrawSurface_set_gamma_ramp(IDirectDrawSurfaceImpl* This,
					      DWORD dwFlags,
					      LPDDGAMMARAMP lpGammaRamp);

/*****************************************************************************
 * IDirectDrawSurface HAL declarations
 */
#define HAL_PRIV(surf) ((HAL_DirectDrawSurfaceImpl*)((surf)->private))

#define HAL_PRIV_VAR(name,surf) \
	HAL_DirectDrawSurfaceImpl* name = HAL_PRIV(surf)

struct HAL_DirectDrawSurfaceImpl_Part
{
    DWORD need_late;
    LPVOID fb_addr;
    DWORD fb_pitch, fb_vofs;
};

typedef struct
{
    struct DIB_DirectDrawSurfaceImpl_Part dib;
    struct User_DirectDrawSurfaceImpl_Part user;
    struct HAL_DirectDrawSurfaceImpl_Part hal;
} HAL_DirectDrawSurfaceImpl;

HRESULT HAL_DirectDrawSurface_Construct(IDirectDrawSurfaceImpl* This,
					IDirectDrawImpl* pDD,
					const DDSURFACEDESC2* pDDSD);

HRESULT HAL_DirectDrawSurface_Create(IDirectDrawImpl *pDD,
				     const DDSURFACEDESC2 *pDDSD,
				     LPDIRECTDRAWSURFACE7 *ppSurf,
				     IUnknown *pUnkOuter);

void HAL_DirectDrawSurface_final_release(IDirectDrawSurfaceImpl* This);
HRESULT HAL_DirectDrawSurface_late_allocate(IDirectDrawSurfaceImpl* This);

void HAL_DirectDrawSurface_set_palette(IDirectDrawSurfaceImpl* This,
				       IDirectDrawPaletteImpl* pal);
void HAL_DirectDrawSurface_update_palette(IDirectDrawSurfaceImpl* This,
					  IDirectDrawPaletteImpl* pal,
					  DWORD dwStart, DWORD dwCount,
					  LPPALETTEENTRY palent);
HRESULT HAL_DirectDrawSurface_duplicate_surface(IDirectDrawSurfaceImpl* This,
						LPDIRECTDRAWSURFACE7* ppDup);
void HAL_DirectDrawSurface_lock_update(IDirectDrawSurfaceImpl* This,
				       LPCRECT pRect, DWORD dwFlags);
void HAL_DirectDrawSurface_unlock_update(IDirectDrawSurfaceImpl* This,
					 LPCRECT pRect);
BOOL HAL_DirectDrawSurface_flip_data(IDirectDrawSurfaceImpl* front,
				     IDirectDrawSurfaceImpl* back,
				     DWORD dwFlags);
void HAL_DirectDrawSurface_flip_update(IDirectDrawSurfaceImpl* This,
				       DWORD dwFlags);
HWND HAL_DirectDrawSurface_get_display_window(IDirectDrawSurfaceImpl* This);

/*****************************************************************************
 * IDirectDrawSurface FAKEZBUFFER declarations
 */
typedef struct
{
    BOOLEAN in_memory;
} FakeZBuffer_DirectDrawSurfaceImpl;

HRESULT FakeZBuffer_DirectDrawSurface_Construct(IDirectDrawSurfaceImpl* This,
						IDirectDrawImpl* pDD,
						const DDSURFACEDESC2* pDDSD);

HRESULT FakeZBuffer_DirectDrawSurface_Create(IDirectDrawImpl* pDD,
					     const DDSURFACEDESC2* pDDSD,
					     LPDIRECTDRAWSURFACE7* ppSurf,
					     IUnknown* pUnkOuter);

void FakeZBuffer_DirectDrawSurface_final_release(IDirectDrawSurfaceImpl* This);

HRESULT FakeZBuffer_DirectDrawSurface_duplicate_surface(IDirectDrawSurfaceImpl* This,
							LPDIRECTDRAWSURFACE7* ppDup);

#endif /* __WINE_DLLS_DDRAW_DDRAW_PRIVATE_H */
