/* Copyright 2000 TransGaming Technologies Inc. */

#ifndef __WINE_DLLS_DDRAW_DDRAW_PRIVATE_H
#define __WINE_DLLS_DDRAW_DDRAW_PRIVATE_H

/* MAY NOT CONTAIN X11 or DGA specific includes/defines/structs! */

#include "winbase.h"
#include "wtypes.h"
#include "wingdi.h"
#include "winuser.h"
#include "ddraw.h"
#include "ddcomimpl.h"

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
	        if ((from)->dwSize < __size) 		\
		    __copysize = (from)->dwSize;	\
		memcpy(to,from,__copysize);		\
		(to)->dwSize = __size;/*restore size*/	\
	} while (0)

/*****************************************************************************
 * IDirectDraw implementation structure
 */

typedef struct IDirectDrawImpl IDirectDrawImpl;
typedef struct IDirectDrawPaletteImpl IDirectDrawPaletteImpl;
typedef struct IDirectDrawClipperImpl IDirectDrawClipperImpl;
typedef struct IDirectDrawSurfaceImpl IDirectDrawSurfaceImpl;

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

    DWORD ref;

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

    HWND window;
    DWORD cooperative_level;
    WNDPROC original_wndproc;

    DWORD width, height;
    LONG pitch;
    DDPIXELFORMAT pixelformat;

    /* Should each of these go into some structure? */
    DWORD orig_width, orig_height;
    LONG orig_pitch;
    DDPIXELFORMAT orig_pixelformat;

    /* Called when the refcount goes to 0. */
    void (*final_release)(IDirectDrawImpl *This);

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
    const struct tagDC_FUNCS *funcs, *old_funcs; /* DISPLAY.DRV overrides */

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
    DWORD ref;

    /* IDirectDrawPalette fields */
    DWORD		flags;
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
    DWORD ref;

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
    DWORD ref;

    struct IDirectDrawSurfaceImpl* attached; /* attached surfaces */

    struct IDirectDrawSurfaceImpl* next_ddraw; /* ddraw surface chain */
    struct IDirectDrawSurfaceImpl* prev_ddraw;
    struct IDirectDrawSurfaceImpl* next_attached; /* attached surface chain */
    struct IDirectDrawSurfaceImpl* prev_attached;

    IDirectDrawImpl* ddraw_owner;
    IDirectDrawSurfaceImpl* surface_owner;

    IDirectDrawPaletteImpl* palette; /* strong ref */
    IDirectDrawClipperImpl* clipper; /* strong ref */

    DDSURFACEDESC2 surface_desc;

    HDC hDC;
    BOOL dc_in_use;

    HRESULT (*duplicate_surface)(IDirectDrawSurfaceImpl* src,
				 LPDIRECTDRAWSURFACE7* dst);
    void (*final_release)(IDirectDrawSurfaceImpl *This);
    BOOL (*attach)(IDirectDrawSurfaceImpl *This, IDirectDrawSurfaceImpl *to);
    BOOL (*detach)(IDirectDrawSurfaceImpl *This);
    void (*lock_update)(IDirectDrawSurfaceImpl* This, LPCRECT pRect);
    void (*unlock_update)(IDirectDrawSurfaceImpl* This, LPCRECT pRect);
    void (*lose_surface)(IDirectDrawSurfaceImpl* This);
    void (*flip_data)(IDirectDrawSurfaceImpl* front,
		      IDirectDrawSurfaceImpl* back);
    void (*flip_update)(IDirectDrawSurfaceImpl* front);
    HRESULT (*get_dc)(IDirectDrawSurfaceImpl* This, HDC* phDC);
    HRESULT (*release_dc)(IDirectDrawSurfaceImpl* This, HDC hDC);
    void (*set_palette)(IDirectDrawSurfaceImpl* This, IDirectDrawPaletteImpl* pal);
    void (*update_palette)(IDirectDrawSurfaceImpl* This, IDirectDrawPaletteImpl* pal,
			   DWORD dwStart, DWORD dwCount, LPPALETTEENTRY palent);
    HWND (*get_display_window)(IDirectDrawSurfaceImpl *This);

    struct PrivateData* private_data;

    DWORD max_lod;
    DWORD priority;

    BOOL lost;

    DWORD uniqueness_value;

    LPVOID private;

    /* Everything below here is dodgy. */
    /* For Direct3D use */
    LPVOID			aux_ctx, aux_data;
    void (*aux_release)(LPVOID ctx, LPVOID data);
    BOOL (*aux_flip)(LPVOID ctx, LPVOID data);
    void (*aux_unlock)(LPVOID ctx, LPVOID data, LPRECT lpRect);
    struct IDirect3DTexture2Impl*	texture;
    HRESULT WINAPI		(*SetColorKey_cb)(struct IDirect3DTexture2Impl *texture, DWORD dwFlags, LPDDCOLORKEY ckey ) ; 
};

/*****************************************************************************
 * Driver initialisation functions.
 */
BOOL DDRAW_User_Init(HINSTANCE, DWORD, LPVOID);

typedef struct {
    const DDDEVICEIDENTIFIER2* info;
    int	preference;	/* how good we are. dga might get 100, xlib 50*/
    HRESULT (*create)(const GUID*, LPDIRECTDRAW7*, LPUNKNOWN, BOOL ex);

    /* For IDirectDraw7::Initialize. */
    HRESULT (*init)(IDirectDrawImpl *, const GUID*);
} ddraw_driver;

void DDRAW_register_driver(const ddraw_driver*);
BOOL DDRAW_XVidMode_Init(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv);
BOOL DDRAW_XF86DGA2_Init(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv);

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

extern HRESULT create_direct3d(LPVOID *obj,IDirectDrawImpl*);
extern HRESULT create_direct3d2(LPVOID *obj,IDirectDrawImpl*);
extern HRESULT create_direct3d3(LPVOID *obj,IDirectDrawImpl*);
extern HRESULT create_direct3d7(LPVOID *obj,IDirectDrawImpl*);

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
extern void DDRAW_dump_DDSCAPS(const DDSCAPS2 *in);
extern void DDRAW_dump_pixelformat_flag(DWORD flagmask);
extern void DDRAW_dump_paletteformat(DWORD dwFlags);
extern void DDRAW_dump_pixelformat(const DDPIXELFORMAT *in);
extern void DDRAW_dump_colorkeyflag(DWORD ck);
extern void DDRAW_dump_surface_desc(const DDSURFACEDESC2 *lpddsd);
extern void DDRAW_dump_cooperativelevel(DWORD cooplevel);
extern void DDRAW_dump_DDCOLORKEY(const DDCOLORKEY *in);
#endif /* __WINE_DLLS_DDRAW_DDRAW_PRIVATE_H */
