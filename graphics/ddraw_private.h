#ifndef __GRAPHICS_WINE_DDRAW_PRIVATE_H
#define __GRAPHICS_WINE_DDRAW_PRIVATE_H

#include "config.h"

#ifdef HAVE_LIBXXF86DGA2
#include "ts_xf86dga2.h"
#endif /* defined(HAVE_LIBXXF86DGA2) */

#include "ddraw.h"
#include "winuser.h"

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
typedef struct IDirectDrawPaletteImpl IDirectDrawPaletteImpl;
typedef struct IDirectDrawClipperImpl IDirectDrawClipperImpl;
typedef struct IDirectDrawImpl IDirectDrawImpl;
typedef struct IDirectDraw2Impl IDirectDraw2Impl;
typedef struct IDirectDraw4Impl IDirectDraw4Impl;
typedef struct IDirectDrawSurfaceImpl IDirectDrawSurfaceImpl;
typedef struct IDirectDrawSurface3Impl IDirectDrawSurface2Impl;
typedef struct IDirectDrawSurface4Impl IDirectDrawSurface3Impl;
typedef struct IDirectDrawSurface4Impl IDirectDrawSurface4Impl;
typedef struct IDirectDrawColorControlImpl IDirectDrawColorControlImpl;

#include "d3d_private.h"

/*****************************************************************************
 * IDirectDrawPalette implementation structure
 */
struct IDirectDrawPaletteImpl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirectDrawPalette);
    DWORD                            ref;
    /* IDirectDrawPalette fields */
    IDirectDrawImpl* ddraw;
    Colormap         cm;
    PALETTEENTRY     palents[256];
    int              installed;
    /* This is to store the palette in 'screen format' */
    int              screen_palents[256];
};

/*****************************************************************************
 * IDirectDrawClipper implementation structure
 */
struct IDirectDrawClipperImpl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirectDrawClipper);
    DWORD                            ref;

    /* IDirectDrawClipper fields */
    HWND hWnd;
};

/*****************************************************************************
 * IDirectDraw implementation structure
 */
struct _common_directdrawdata
{
    DDPIXELFORMAT directdraw_pixelformat;
    DDPIXELFORMAT screen_pixelformat;
    int           pixmap_depth;
    void (*pixel_convert)(void *src, void *dst, DWORD width, DWORD height, LONG pitch, IDirectDrawPaletteImpl* palette);
    void (*palette_convert)(LPPALETTEENTRY palent, void *screen_palette, DWORD start, DWORD count);
    DWORD         height,width;	/* SetDisplayMode */
    HWND          mainWindow;   /* SetCooperativeLevel */

    /* This is for Wine's fake mainWindow.
       We need it also in DGA mode to make some games (for example Monkey Island III work) */
    ATOM          winclass;
    HWND          window;
    Window        drawable;
    PAINTSTRUCT   ps;
    int           paintable;
};

struct _dga_directdrawdata
{
    DWORD        fb_width,fb_height,fb_memsize;
    void*        fb_addr;
    unsigned int vpmask;
#ifdef HAVE_LIBXXF86DGA2
    int version;
    XDGADevice *dev;
#endif /* define(HAVE_LIBXXF86DGA2) */
};

struct _xlib_directdrawdata
{
#ifdef HAVE_LIBXXSHM
    int xshm_active, xshm_compl;
#endif /* defined(HAVE_LIBXXSHM) */
    
    /* are these needed for anything? (draw_surf is the active surface)
    IDirectDrawSurfaceImpl* surfs;
    DWORD		num_surfs, alloc_surfs, draw_surf; */
};

struct IDirectDrawImpl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirectDraw);
    DWORD                     ref;
    /* IDirectDraw fields */
    struct _common_directdrawdata   d;
    union {
        struct _xlib_directdrawdata xlib;
        struct _dga_directdrawdata  dga;
    } e;
};

/*****************************************************************************
 * IDirectDraw2 implementation structure
 */
struct IDirectDraw2Impl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirectDraw2);
    DWORD                      ref;
    /* IDirectDraw2 fields */
    struct _common_directdrawdata   d;
    union {
        struct _xlib_directdrawdata xlib;
        struct _dga_directdrawdata  dga;
    } e;
};

/*****************************************************************************
 * IDirectDraw4 implementation structure
 */
struct IDirectDraw4Impl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirectDraw4);
    DWORD                      ref;
    /* IDirectDraw4 fields */
    struct _common_directdrawdata   d;
    union {
        struct _xlib_directdrawdata xlib;
        struct _dga_directdrawdata  dga;
    } e;
};

/*****************************************************************************
 * IDirectDrawSurface implementation structure
 */
struct _common_directdrawsurface
{
    IDirectDrawPaletteImpl*     palette;
    IDirectDraw2Impl*           ddraw;

    struct _surface_chain 	*chain;

    DDSURFACEDESC               surface_desc;

    /* For Get / Release DC methods */
    HBITMAP DIBsection;
    void *bitmap_data;
    HDC hdc;
    HGDIOBJ holdbitmap;
    
    /* Callback for loaded textures */
    IDirect3DTexture2Impl*      texture;
    HRESULT WINAPI            (*SetColorKey_cb)(IDirect3DTexture2Impl *texture, DWORD dwFlags, LPDDCOLORKEY ckey ) ;

    /* Storage for attached device (void * as it can be either a Device or a Device2) */
    void                       *d3d_device;
};

struct _dga_directdrawsurface
{
    DWORD		fb_height;
};

struct _xlib_directdrawsurface
{
    XImage		*image;
#ifdef HAVE_LIBXXSHM
    XShmSegmentInfo	shminfo;
#endif
};

struct IDirectDrawSurfaceImpl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirectDrawSurface);
    DWORD                            ref;
    /* IDirectDrawSurface fields */
    struct _common_directdrawsurface	s;
    union {
	struct _dga_directdrawsurface	dga;
	struct _xlib_directdrawsurface	xlib;
    } t;
    LPDIRECTDRAWCLIPPER lpClipper;
};

/*****************************************************************************
 * IDirectDrawSurface2 implementation structure
 */
struct IDirectDrawSurface2Impl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirectDrawSurface2);
    DWORD                             ref;
    /* IDirectDrawSurface2 fields */
    struct _common_directdrawsurface	s;
    union {
	struct _dga_directdrawsurface	dga;
	struct _xlib_directdrawsurface	xlib;
    } t;
    LPDIRECTDRAWCLIPPER lpClipper;
};

/*****************************************************************************
 * IDirectDrawSurface3 implementation structure
 */
struct IDirectDrawSurface3Impl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirectDrawSurface3);
    DWORD                             ref;
    /* IDirectDrawSurface3 fields */
    struct _common_directdrawsurface	s;
    union {
	struct _dga_directdrawsurface	dga;
	struct _xlib_directdrawsurface	xlib;
    } t;
    LPDIRECTDRAWCLIPPER lpClipper;
};

/*****************************************************************************
 * IDirectDrawSurface4 implementation structure
 */
struct IDirectDrawSurface4Impl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirectDrawSurface4);
    DWORD                             ref;

    /* IDirectDrawSurface4 fields */
    struct _common_directdrawsurface	s;
    union {
	struct _dga_directdrawsurface	dga;
	struct _xlib_directdrawsurface	xlib;
    } t;
    LPDIRECTDRAWCLIPPER lpClipper;
} ;

struct _surface_chain {
	IDirectDrawSurface4Impl	**surfaces;
	int			nrofsurfaces;
};

/*****************************************************************************
 * IDirectDrawColorControl implementation structure
 */
struct IDirectDrawColorControlImpl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirectDrawColorControl);
    DWORD                                 ref;
    /* IDirectDrawColorControl fields */
    /* none */
};


#endif /* __GRAPHICS_WINE_DDRAW_PRIVATE_H */
