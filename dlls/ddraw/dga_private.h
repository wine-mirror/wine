#ifndef __WINE_DDRAW_DGA_PRIVATE_H
#define __WINE_DDRAW_DGA_PRIVATE_H

#include "ddraw_private.h"
#include "x11_private.h"

#include "ts_xf86dga.h"

#ifdef HAVE_LIBXXF86VM
# include "ts_xf86vmode.h"
extern XF86VidModeModeInfo *orig_mode;
#endif /* defined(HAVE_LIBXXF86VM) */

extern ICOM_VTABLE(IDirectDrawSurface4) dga_dds4vt;
extern ICOM_VTABLE(IDirectDraw)		dga_ddvt;
extern ICOM_VTABLE(IDirectDrawPalette)	dga_ddpalvt;

typedef struct dga_dd_private {
    DWORD	fb_height;		/* height of the viewport */
    DWORD	fb_width;		/* width of the viewport */
    caddr_t	fb_addr;		/* start address of the framebuffer */
    DWORD	fb_memsize;		/* total memory on the card */
    DWORD	vpmask;			/* viewports in use flag bitmap */
    void      (*InstallColormap)(Display *, int, Colormap) ;
} dga_dd_private;

typedef x11_dp_private dga_dp_private;	/* reuse X11 palette stuff */

typedef struct dga_ds_private {
    DWORD	fb_height;
    int		*oldDIBmap;
} dga_ds_private;

/* For usage in DGA2 */
extern ULONG WINAPI DGA_IDirectDrawSurface4Impl_Release(LPDIRECTDRAWSURFACE4 iface) ;
extern HRESULT WINAPI DGA_IDirectDrawSurface4Impl_SetPalette(LPDIRECTDRAWSURFACE4 iface,LPDIRECTDRAWPALETTE pal) ;
extern HRESULT WINAPI DGA_IDirectDrawSurface4Impl_Unlock(LPDIRECTDRAWSURFACE4 iface,LPVOID surface) ;
extern HRESULT WINAPI DGA_IDirectDrawSurface4Impl_GetDC(LPDIRECTDRAWSURFACE4 iface,HDC* lphdc);

extern HRESULT WINAPI DGA_IDirectDraw2Impl_CreateSurface_no_VT(LPDIRECTDRAW2 iface,LPDDSURFACEDESC lpddsd,
							       LPDIRECTDRAWSURFACE *lpdsf,IUnknown *lpunk) ;

extern HRESULT WINAPI DGA_IDirectDraw2Impl_QueryInterface(LPDIRECTDRAW2 iface,REFIID refiid,LPVOID *obj) ;
extern HRESULT WINAPI DGA_IDirectDraw2Impl_GetCaps(LPDIRECTDRAW2 iface,LPDDCAPS caps1,LPDDCAPS caps2) ;

extern HRESULT WINAPI DGA_IDirectDraw2Impl_GetDisplayMode(LPDIRECTDRAW2 iface,LPDDSURFACEDESC lpddsfd) ;
extern HRESULT WINAPI DGA_IDirectDraw2Impl_GetAvailableVidMem(LPDIRECTDRAW2 iface,LPDDSCAPS ddscaps,LPDWORD total,LPDWORD free) ;


#endif /* __WINE_DDRAW_DGA_PRIVATE_H */
