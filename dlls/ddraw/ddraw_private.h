#ifndef __WINE_DLLS_DDRAW_DDRAW_PRIVATE_H
#define __WINE_DLLS_DDRAW_DDRAW_PRIVATE_H

/* MAY NOT CONTAIN X11 or DGA specific includes/defines/structs! */

#include "wtypes.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "ddraw.h"

static const char WINE_UNUSED *ddProp = "WINE_DDRAW_Property";

/****************************************************************************
 * This is the main DirectDraw driver interface. It is supposed to be called
 * only from the base functions and only used by those. It should neither be
 * be called nor used within the interfaces.
 */
typedef struct ddraw_driver {
	LPGUID	guid;		/*under which we are referenced and enumerated*/
	CHAR	type[20];	/* type, usually "display" */
	CHAR	name[40];	/* name, like "WINE Foobar DirectDraw Driver" */
	int	preference;	/* how good we are. dga might get 100, xlib 50*/
	HRESULT	(*createDDRAW)(LPDIRECTDRAW*); /* also check if arg is NULL */
} ddraw_driver;

extern void ddraw_register_driver(ddraw_driver*);

/*****************************************************************************
 * The implementation structures. They must not contain driver specific stuff.
 * 
 * For private data the "LPVOID private" pointer should be used.
 */

typedef struct IDirectDrawImpl	IDirectDrawImpl;
typedef struct IDirectDraw2Impl	IDirectDraw2Impl;
typedef struct IDirectDraw3Impl	IDirectDraw3Impl;
typedef struct IDirectDraw4Impl	IDirectDraw4Impl;
typedef struct IDirectDrawPaletteImpl	IDirectDrawPaletteImpl;
typedef struct IDirectDrawClipperImpl	IDirectDrawClipperImpl;

typedef struct IDirectDrawSurfaceImpl IDirectDrawSurfaceImpl;
typedef struct IDirectDrawSurface2Impl IDirectDrawSurface2Impl;
typedef struct IDirectDrawSurface4Impl IDirectDrawSurface4Impl;


extern struct ICOM_VTABLE(IDirectDrawClipper)	ddclipvt;
extern struct ICOM_VTABLE(IDirectDrawPalette)	ddraw_ddpalvt;

/*****************************************************************************
 * IDirectDraw implementation structure
 */
struct _common_directdrawdata
{
    DDPIXELFORMAT directdraw_pixelformat;
    DDPIXELFORMAT screen_pixelformat;

    int           pixmap_depth;
    void (*pixel_convert)(void *src, void *dst, DWORD width, DWORD height, LONG pitch, IDirectDrawPaletteImpl *palette);
    void (*palette_convert)(LPPALETTEENTRY palent, void *screen_palette, DWORD start, DWORD count);
    DWORD         height,width;	/* set by SetDisplayMode */
    HWND          mainWindow;   /* set by SetCooperativeLevel */

    /* This is for the fake mainWindow */
    ATOM          winclass;
    HWND          window;
    PAINTSTRUCT   ps;
    int           paintable;
};

/*****************************************************************************
 * IDirectDraw implementation structure
 * 
 * Note: All the IDirectDraw*Impl structures _MUST_ have IDENTICAL layout,
 * 	 since we reuse functions across interface versions.
 */
struct IDirectDrawImpl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirectDraw);
    DWORD				ref;
    /* IDirectDraw fields */
    struct _common_directdrawdata	d;
    LPVOID				private;
};
extern HRESULT WINAPI IDirectDrawImpl_SetDisplayMode(
	LPDIRECTDRAW iface,DWORD width,DWORD height,DWORD depth
);
/*
 * IDirectDraw2 implementation structure
 */
struct IDirectDraw2Impl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirectDraw2);
    DWORD				ref;

    /* IDirectDraw2 fields */
    struct _common_directdrawdata	d;
    LPVOID				private;
};
extern HRESULT WINAPI IDirectDraw2Impl_DuplicateSurface(
	LPDIRECTDRAW2 iface,LPDIRECTDRAWSURFACE src,LPDIRECTDRAWSURFACE *dst
);
extern HRESULT WINAPI IDirectDraw2Impl_SetCooperativeLevel(
	LPDIRECTDRAW2 iface,HWND hwnd,DWORD cooplevel
);
extern HRESULT WINAPI IDirectDraw2Impl_GetCaps(
	LPDIRECTDRAW2 iface,LPDDCAPS caps1,LPDDCAPS caps2
) ;
extern HRESULT WINAPI IDirectDraw2Impl_CreateClipper(
    LPDIRECTDRAW2 iface,DWORD x,LPDIRECTDRAWCLIPPER *lpddclip,LPUNKNOWN lpunk
);
extern HRESULT WINAPI common_IDirectDraw2Impl_CreatePalette(
    IDirectDraw2Impl* This,DWORD dwFlags,LPPALETTEENTRY palent,
    IDirectDrawPaletteImpl **lpddpal,LPUNKNOWN lpunk,int *psize
);
extern HRESULT WINAPI IDirectDraw2Impl_CreatePalette(
    LPDIRECTDRAW2 iface,DWORD dwFlags,LPPALETTEENTRY palent,LPDIRECTDRAWPALETTE *lpddpal,LPUNKNOWN lpunk
);
extern HRESULT WINAPI IDirectDraw2Impl_RestoreDisplayMode(LPDIRECTDRAW2 iface);
extern HRESULT WINAPI IDirectDraw2Impl_WaitForVerticalBlank(
	LPDIRECTDRAW2 iface,DWORD x,HANDLE h
);
extern ULONG WINAPI IDirectDraw2Impl_AddRef(LPDIRECTDRAW2 iface);
extern ULONG WINAPI IDirectDraw2Impl_Release(LPDIRECTDRAW2 iface);
extern HRESULT WINAPI IDirectDraw2Impl_QueryInterface(
	LPDIRECTDRAW2 iface,REFIID refiid,LPVOID *obj
);
extern HRESULT WINAPI IDirectDraw2Impl_GetVerticalBlankStatus(
	LPDIRECTDRAW2 iface,BOOL *status
);
extern HRESULT WINAPI IDirectDraw2Impl_EnumDisplayModes(
	LPDIRECTDRAW2 iface,DWORD dwFlags,LPDDSURFACEDESC lpddsfd,LPVOID context,LPDDENUMMODESCALLBACK modescb
);
extern HRESULT WINAPI IDirectDraw2Impl_GetDisplayMode(
	LPDIRECTDRAW2 iface,LPDDSURFACEDESC lpddsfd
);
extern HRESULT WINAPI IDirectDraw2Impl_FlipToGDISurface(LPDIRECTDRAW2 iface);
extern HRESULT WINAPI IDirectDraw2Impl_GetMonitorFrequency(
    LPDIRECTDRAW2 iface,LPDWORD freq
);
extern HRESULT WINAPI IDirectDraw2Impl_GetFourCCCodes(
    LPDIRECTDRAW2 iface,LPDWORD x,LPDWORD y
);
extern HRESULT WINAPI IDirectDraw2Impl_EnumSurfaces(
    LPDIRECTDRAW2 iface,DWORD x,LPDDSURFACEDESC ddsfd,LPVOID context,
    LPDDENUMSURFACESCALLBACK ddsfcb
);
extern HRESULT WINAPI IDirectDraw2Impl_Compact( LPDIRECTDRAW2 iface );
extern HRESULT WINAPI IDirectDraw2Impl_GetGDISurface(
    LPDIRECTDRAW2 iface, LPDIRECTDRAWSURFACE *lplpGDIDDSSurface
);
extern HRESULT WINAPI IDirectDraw2Impl_GetScanLine(
    LPDIRECTDRAW2 iface, LPDWORD lpdwScanLine
);
extern HRESULT WINAPI IDirectDraw2Impl_Initialize(LPDIRECTDRAW2 iface, GUID *lpGUID);
extern HRESULT WINAPI IDirectDraw2Impl_SetDisplayMode(
    LPDIRECTDRAW2 iface,DWORD width,DWORD height,DWORD depth,
    DWORD dwRefreshRate, DWORD dwFlags
);
extern HRESULT WINAPI IDirectDraw2Impl_GetAvailableVidMem(
	LPDIRECTDRAW2 iface,LPDDSCAPS ddscaps,LPDWORD total,LPDWORD free
);
extern HRESULT common_off_screen_CreateSurface(
	IDirectDraw2Impl* This,IDirectDrawSurfaceImpl* lpdsf
);

/*
 * IDirectDraw4 implementation structure
 */
struct IDirectDraw4Impl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirectDraw4);
    DWORD				ref;
    /* IDirectDraw4 fields */
    struct _common_directdrawdata	d;
    LPVOID				private;
};
extern HRESULT WINAPI IDirectDraw4Impl_GetSurfaceFromDC(
	LPDIRECTDRAW4 iface, HDC hdc, LPDIRECTDRAWSURFACE *lpDDS
);
extern HRESULT WINAPI IDirectDraw4Impl_RestoreAllSurfaces(LPDIRECTDRAW4 iface);
extern HRESULT WINAPI IDirectDraw4Impl_TestCooperativeLevel(LPDIRECTDRAW4 iface);
extern HRESULT WINAPI IDirectDraw4Impl_GetDeviceIdentifier(LPDIRECTDRAW4 iface,
						    LPDDDEVICEIDENTIFIER lpdddi,
						    DWORD dwFlags
);

/*****************************************************************************
 * IDirectDrawPalette implementation structure
 */
struct IDirectDrawPaletteImpl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirectDrawPalette);
    DWORD		ref;

    /* IDirectDrawPalette fields */
    IDirectDrawImpl*		ddraw;	/* direct draw, no reference count */
    PALETTEENTRY	palents[256];

    /* This is to store the palette in 'screen format' */
    int			screen_palents[256];
    LPVOID		private;
};
extern HRESULT WINAPI IDirectDrawPaletteImpl_GetEntries(LPDIRECTDRAWPALETTE,DWORD,DWORD,DWORD,LPPALETTEENTRY);
extern HRESULT WINAPI IDirectDrawPaletteImpl_SetEntries(LPDIRECTDRAWPALETTE,DWORD,DWORD,DWORD,LPPALETTEENTRY);
extern ULONG WINAPI IDirectDrawPaletteImpl_Release(LPDIRECTDRAWPALETTE);
extern ULONG WINAPI IDirectDrawPaletteImpl_AddRef(LPDIRECTDRAWPALETTE);
extern HRESULT WINAPI IDirectDrawPaletteImpl_Initialize(LPDIRECTDRAWPALETTE,LPDIRECTDRAW,DWORD,LPPALETTEENTRY);
extern HRESULT WINAPI IDirectDrawPaletteImpl_GetCaps(LPDIRECTDRAWPALETTE,LPDWORD);
extern HRESULT WINAPI IDirectDrawPaletteImpl_QueryInterface(LPDIRECTDRAWPALETTE,REFIID,LPVOID *);

extern HRESULT WINAPI common_IDirectDraw2Impl_CreatePalette(
    IDirectDraw2Impl* This,DWORD dwFlags,LPPALETTEENTRY palent,
    IDirectDrawPaletteImpl **lpddpal,LPUNKNOWN lpunk,int *psize
);

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
 * IDirectDrawSurface implementation structure
 */
struct IDirect3DTexture2Impl;
struct _common_directdrawsurface
{
    IDirectDrawPaletteImpl*     palette;
    IDirectDraw2Impl*           ddraw;

    struct _surface_chain 	*chain;

    DDSURFACEDESC               surface_desc;

    /* For Get / Release DC methods */
    HBITMAP			DIBsection;
    void			*bitmap_data;
    HDC				hdc;
    HGDIOBJ			holdbitmap;
    LPDIRECTDRAWCLIPPER		lpClipper;
    
    /* Callback for loaded textures */
    struct IDirect3DTexture2Impl*	texture;
    HRESULT WINAPI		(*SetColorKey_cb)(struct IDirect3DTexture2Impl *texture, DWORD dwFlags, LPDDCOLORKEY ckey ) ; 
};
extern IDirectDrawSurface4Impl* _common_find_flipto(IDirectDrawSurface4Impl* This,IDirectDrawSurface4Impl* flipto);

struct IDirectDrawSurfaceImpl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirectDrawSurface);
    DWORD                            ref;

    /* IDirectDrawSurface fields */
    struct _common_directdrawsurface	s;
    LPVOID private;
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
    LPVOID private;
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
    LPVOID private;
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
    LPVOID private;
} ;

struct _surface_chain {
	IDirectDrawSurface4Impl	**surfaces;
	int			nrofsurfaces;
};
extern HRESULT common_off_screen_CreateSurface(IDirectDraw2Impl* This,IDirectDrawSurfaceImpl* lpdsf);
extern HRESULT WINAPI IDirectDrawSurface4Impl_Lock(LPDIRECTDRAWSURFACE4 iface,LPRECT lprect,LPDDSURFACEDESC lpddsd,DWORD flags, HANDLE hnd);
extern HRESULT WINAPI IDirectDrawSurface4Impl_Unlock( LPDIRECTDRAWSURFACE4 iface,LPVOID surface);
extern HRESULT WINAPI IDirectDrawSurface4Impl_Blt(LPDIRECTDRAWSURFACE4 iface,LPRECT rdst,LPDIRECTDRAWSURFACE4 src,LPRECT rsrc,DWORD dwFlags,LPDDBLTFX lpbltfx);
extern HRESULT WINAPI IDirectDrawSurface4Impl_BltFast(LPDIRECTDRAWSURFACE4 iface,DWORD dstx,DWORD dsty,LPDIRECTDRAWSURFACE4 src,LPRECT rsrc,DWORD trans);
extern HRESULT WINAPI IDirectDrawSurface4Impl_BltBatch(LPDIRECTDRAWSURFACE4 iface,LPDDBLTBATCH ddbltbatch,DWORD x,DWORD y);
extern HRESULT WINAPI IDirectDrawSurface4Impl_GetCaps(LPDIRECTDRAWSURFACE4 iface,LPDDSCAPS caps);
extern HRESULT WINAPI IDirectDrawSurface4Impl_GetSurfaceDesc(LPDIRECTDRAWSURFACE4 iface,LPDDSURFACEDESC ddsd);
extern ULONG WINAPI IDirectDrawSurface4Impl_AddRef(LPDIRECTDRAWSURFACE4 iface);
extern HRESULT WINAPI IDirectDrawSurface4Impl_GetAttachedSurface(LPDIRECTDRAWSURFACE4 iface,LPDDSCAPS lpddsd,LPDIRECTDRAWSURFACE4 *lpdsf);
extern HRESULT WINAPI IDirectDrawSurface4Impl_Initialize(LPDIRECTDRAWSURFACE4 iface,LPDIRECTDRAW ddraw,LPDDSURFACEDESC lpdsfd);
extern HRESULT WINAPI IDirectDrawSurface4Impl_GetPixelFormat(LPDIRECTDRAWSURFACE4 iface,LPDDPIXELFORMAT pf);
extern HRESULT WINAPI IDirectDrawSurface4Impl_GetBltStatus(LPDIRECTDRAWSURFACE4 iface,DWORD dwFlags);
extern HRESULT WINAPI IDirectDrawSurface4Impl_GetOverlayPosition(LPDIRECTDRAWSURFACE4 iface,LPLONG x1,LPLONG x2);
extern HRESULT WINAPI IDirectDrawSurface4Impl_SetClipper(LPDIRECTDRAWSURFACE4 iface,LPDIRECTDRAWCLIPPER lpClipper);
extern HRESULT WINAPI IDirectDrawSurface4Impl_AddAttachedSurface(LPDIRECTDRAWSURFACE4 iface,LPDIRECTDRAWSURFACE4 surf);
extern HRESULT WINAPI IDirectDrawSurface4Impl_GetDC(LPDIRECTDRAWSURFACE4 iface,HDC* lphdc);
extern HRESULT WINAPI IDirectDrawSurface4Impl_ReleaseDC(LPDIRECTDRAWSURFACE4 iface,HDC hdc);
extern HRESULT WINAPI IDirectDrawSurface4Impl_QueryInterface(LPDIRECTDRAWSURFACE4 iface,REFIID refiid,LPVOID *obj);
extern HRESULT WINAPI IDirectDrawSurface4Impl_IsLost(LPDIRECTDRAWSURFACE4 iface);
extern HRESULT WINAPI IDirectDrawSurface4Impl_EnumAttachedSurfaces(LPDIRECTDRAWSURFACE4 iface,LPVOID context,LPDDENUMSURFACESCALLBACK esfcb);
extern HRESULT WINAPI IDirectDrawSurface4Impl_Restore(LPDIRECTDRAWSURFACE4 iface);
extern HRESULT WINAPI IDirectDrawSurface4Impl_SetColorKey(LPDIRECTDRAWSURFACE4 iface, DWORD dwFlags, LPDDCOLORKEY ckey);
extern HRESULT WINAPI IDirectDrawSurface4Impl_AddOverlayDirtyRect(LPDIRECTDRAWSURFACE4 iface,LPRECT lpRect);
extern HRESULT WINAPI IDirectDrawSurface4Impl_DeleteAttachedSurface(LPDIRECTDRAWSURFACE4 iface,DWORD dwFlags,LPDIRECTDRAWSURFACE4 lpDDSAttachedSurface);
extern HRESULT WINAPI IDirectDrawSurface4Impl_EnumOverlayZOrders(LPDIRECTDRAWSURFACE4 iface,DWORD dwFlags,LPVOID lpContext,LPDDENUMSURFACESCALLBACK lpfnCallback);
extern HRESULT WINAPI IDirectDrawSurface4Impl_GetClipper(LPDIRECTDRAWSURFACE4 iface,LPDIRECTDRAWCLIPPER* lplpDDClipper);
extern HRESULT WINAPI IDirectDrawSurface4Impl_GetColorKey(LPDIRECTDRAWSURFACE4 iface,DWORD dwFlags,LPDDCOLORKEY lpDDColorKey);
extern HRESULT WINAPI IDirectDrawSurface4Impl_GetFlipStatus(LPDIRECTDRAWSURFACE4 iface,DWORD dwFlags);
extern HRESULT WINAPI IDirectDrawSurface4Impl_GetPalette(LPDIRECTDRAWSURFACE4 iface,LPDIRECTDRAWPALETTE* lplpDDPalette);
extern HRESULT WINAPI IDirectDrawSurface4Impl_SetOverlayPosition(LPDIRECTDRAWSURFACE4 iface,LONG lX,LONG lY);
extern HRESULT WINAPI IDirectDrawSurface4Impl_UpdateOverlay(LPDIRECTDRAWSURFACE4 iface,LPRECT lpSrcRect,LPDIRECTDRAWSURFACE4 lpDDDestSurface,LPRECT lpDestRect,DWORD dwFlags,LPDDOVERLAYFX lpDDOverlayFx);
extern HRESULT WINAPI IDirectDrawSurface4Impl_UpdateOverlayDisplay(LPDIRECTDRAWSURFACE4 iface,DWORD dwFlags);
extern HRESULT WINAPI IDirectDrawSurface4Impl_UpdateOverlayZOrder(LPDIRECTDRAWSURFACE4 iface,DWORD dwFlags,LPDIRECTDRAWSURFACE4 lpDDSReference);
extern HRESULT WINAPI IDirectDrawSurface4Impl_GetDDInterface(LPDIRECTDRAWSURFACE4 iface,LPVOID* lplpDD);
extern HRESULT WINAPI IDirectDrawSurface4Impl_PageLock(LPDIRECTDRAWSURFACE4 iface,DWORD dwFlags);
extern HRESULT WINAPI IDirectDrawSurface4Impl_PageUnlock(LPDIRECTDRAWSURFACE4 iface,DWORD dwFlags);
extern HRESULT WINAPI IDirectDrawSurface4Impl_SetSurfaceDesc(LPDIRECTDRAWSURFACE4 iface,LPDDSURFACEDESC lpDDSD,DWORD dwFlags);
extern HRESULT WINAPI IDirectDrawSurface4Impl_SetPrivateData(LPDIRECTDRAWSURFACE4 iface,REFGUID guidTag,LPVOID lpData,DWORD cbSize,DWORD dwFlags);
extern HRESULT WINAPI IDirectDrawSurface4Impl_GetPrivateData(LPDIRECTDRAWSURFACE4 iface,REFGUID guidTag,LPVOID lpBuffer,LPDWORD lpcbBufferSize);
extern HRESULT WINAPI IDirectDrawSurface4Impl_FreePrivateData(LPDIRECTDRAWSURFACE4 iface,REFGUID guidTag);
extern HRESULT WINAPI IDirectDrawSurface4Impl_GetUniquenessValue(LPDIRECTDRAWSURFACE4 iface,LPDWORD lpValue);
extern HRESULT WINAPI IDirectDrawSurface4Impl_ChangeUniquenessValue(LPDIRECTDRAWSURFACE4 iface);

extern void _common_IDirectDrawImpl_SetDisplayMode(IDirectDrawImpl* This);

/* Get DDSCAPS of surface (shortcutmacro) */
#define SDDSCAPS(iface) ((iface)->s.surface_desc.ddsCaps.dwCaps)
/* Get the number of bytes per pixel for a given surface */
#define PFGET_BPP(pf) (pf.dwFlags&DDPF_PALETTEINDEXED8?1:(pf.u.dwRGBBitCount/8))
#define GET_BPP(desc) PFGET_BPP(desc.ddpfPixelFormat)

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

extern Convert ModeEmulations[6];
extern int _common_depth_to_pixelformat(DWORD depth,LPDIRECTDRAW ddraw);

extern HRESULT create_direct3d(LPVOID *obj,IDirectDraw2Impl*);
extern HRESULT create_direct3d2(LPVOID *obj,IDirectDraw2Impl*);

/******************************************************************************
 * Debugging / Flags output functions
 */
extern void _dump_DDBLTFX(DWORD flagmask);
extern void _dump_DDBLTFAST(DWORD flagmask);
extern void _dump_DDBLT(DWORD flagmask);
extern void _dump_DDSCAPS(void *in);
extern void _dump_pixelformat_flag(DWORD flagmask);
extern void _dump_paletteformat(DWORD dwFlags);
extern void _dump_pixelformat(void *in);
extern void _dump_colorkeyflag(DWORD ck);
extern void _dump_surface_desc(DDSURFACEDESC *lpddsd);
extern void _dump_cooperativelevel(DWORD cooplevel);
extern void _dump_surface_desc(DDSURFACEDESC *lpddsd);
extern void _dump_DDCOLORKEY(void *in);
#endif /* __WINE_DLLS_DDRAW_DDRAW_PRIVATE_H */
