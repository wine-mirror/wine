/*
 * DirectDraw driver interface
 *
 * Copyright 2001 TransGaming Technologies, Inc.
 */

#include "ts_xlib.h"
#include "x11drv.h"
#include "x11ddraw.h"
#include "xvidmode.h"

#include "windef.h"
#include "wingdi.h"
#include "ddrawi.h"
#include "bitmap.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(x11drv);

LPDDRAWI_DDRAWSURFACE_LCL X11DRV_DD_Primary;
LPDDRAWI_DDRAWSURFACE_GBL X11DRV_DD_PrimaryGbl;
HBITMAP X11DRV_DD_PrimaryDIB;
Drawable X11DRV_DD_PrimaryDrawable;
ATOM X11DRV_DD_UserClass;
BOOL X11DRV_DD_IsDirect;

static void SetPrimaryDIB(HBITMAP hBmp)
{
  X11DRV_DD_PrimaryDIB = hBmp;
  if (hBmp) {
    BITMAPOBJ *bmp;
    bmp = (BITMAPOBJ *)GDI_GetObjPtr( hBmp, BITMAP_MAGIC );
    X11DRV_DD_PrimaryDrawable = (Pixmap)bmp->physBitmap;
    GDI_ReleaseObj( hBmp );
  } else {
    X11DRV_DD_PrimaryDrawable = 0;
  }
}

static DWORD PASCAL X11DRV_DDHAL_DestroyDriver(LPDDHAL_DESTROYDRIVERDATA data)
{
  data->ddRVal = DD_OK;
  return DDHAL_DRIVER_HANDLED;
}

static DWORD PASCAL X11DRV_DDHAL_CreateSurface(LPDDHAL_CREATESURFACEDATA data)
{
  if (data->lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) {
    X11DRV_DD_Primary = *data->lplpSList;
    X11DRV_DD_PrimaryGbl = X11DRV_DD_Primary->lpGbl;
    SetPrimaryDIB(GET_LPDDRAWSURFACE_GBL_MORE(X11DRV_DD_PrimaryGbl)->hKernelSurface);
    X11DRV_DD_UserClass = GlobalFindAtomA("WINE_DDRAW");
  }
  data->ddRVal = DD_OK;
  return DDHAL_DRIVER_NOTHANDLED;
}

static DWORD PASCAL X11DRV_DDHAL_CreatePalette(LPDDHAL_CREATEPALETTEDATA data)
{
  FIXME("stub\n");
  /* only makes sense to do anything if the X server is running at 8bpp,
   * which few people do nowadays */
  data->ddRVal = DD_OK;
  return DDHAL_DRIVER_HANDLED;
}

static DDHAL_DDCALLBACKS hal_ddcallbacks = {
  sizeof(DDHAL_DDCALLBACKS),
  0x3ff, /* all callbacks are 32-bit */
  X11DRV_DDHAL_DestroyDriver,
  X11DRV_DDHAL_CreateSurface,
  NULL, /* SetColorKey */
  NULL, /* SetMode */
  NULL, /* WaitForVerticalBlank */
  NULL, /* CanCreateSurface */
  X11DRV_DDHAL_CreatePalette,
  NULL, /* GetScanLine */
  NULL, /* SetExclusiveMode */
  NULL  /* FlipToGDISurface */
};

static DWORD PASCAL X11DRV_DDHAL_DestroySurface(LPDDHAL_DESTROYSURFACEDATA data)
{
  if (data->lpDDSurface == X11DRV_DD_Primary) {
    X11DRV_DD_Primary = NULL;
    X11DRV_DD_PrimaryGbl = NULL;
    SetPrimaryDIB(0);
    X11DRV_DD_UserClass = 0;
  }
  data->ddRVal = DD_OK;
  return DDHAL_DRIVER_HANDLED;
}

static DWORD PASCAL X11DRV_DDHAL_SetPalette(LPDDHAL_SETPALETTEDATA data)
{
  Colormap pal = data->lpDDPalette->u1.dwReserved1;
  if (pal) {
    if (data->lpDDSurface == X11DRV_DD_Primary) {
      FIXME("stub\n");
      /* we should probably find the ddraw window (maybe data->lpDD->lpExclusiveOwner->hWnd),
       * and attach the palette to it */
    }
  }
  data->ddRVal = DD_OK;
  return DDHAL_DRIVER_HANDLED;
}

static DDHAL_DDSURFACECALLBACKS hal_ddsurfcallbacks = {
  sizeof(DDHAL_DDSURFACECALLBACKS),
  0x3fff, /* all callbacks are 32-bit */
  X11DRV_DDHAL_DestroySurface,
  NULL, /* Flip */
  NULL, /* SetClipList */
  NULL, /* Lock */
  NULL, /* Unlock */
  NULL, /* Blt */
  NULL, /* SetColorKey */
  NULL, /* AddAttachedSurface */
  NULL, /* GetBltStatus */
  NULL, /* GetFlipStatus */
  NULL, /* UpdateOverlay */
  NULL, /* SetOverlayPosition */
  NULL, /* reserved4 */
  X11DRV_DDHAL_SetPalette
};

static DWORD PASCAL X11DRV_DDHAL_DestroyPalette(LPDDHAL_DESTROYPALETTEDATA data)
{
  Colormap pal = data->lpDDPalette->u1.dwReserved1;
  if (pal) TSXFreeColormap(display, pal);
  data->ddRVal = DD_OK;
  return DDHAL_DRIVER_HANDLED;
}

static DWORD PASCAL X11DRV_DDHAL_SetPaletteEntries(LPDDHAL_SETENTRIESDATA data)
{
  X11DRV_DDHAL_SetPalEntries(data->lpDDPalette->u1.dwReserved1,
			     data->dwBase, data->dwNumEntries,
			     data->lpEntries);
  data->ddRVal = DD_OK;
  return DDHAL_DRIVER_HANDLED;
}

static DDHAL_DDPALETTECALLBACKS hal_ddpalcallbacks = {
  sizeof(DDHAL_DDPALETTECALLBACKS),
  0x3, /* all callbacks are 32-bit */
  X11DRV_DDHAL_DestroyPalette,
  X11DRV_DDHAL_SetPaletteEntries
};

static DDHALINFO hal_info = {
  sizeof(DDHALINFO),
  &hal_ddcallbacks,
  &hal_ddsurfcallbacks,
  &hal_ddpalcallbacks,
  {	/* vmiData */
   0	 /* fpPrimary */
  },
  {	/* ddCaps */
   sizeof(DDCORECAPS)
  },
  0,	/* dwMonitorFrequency */
  NULL,	/* GetDriverInfo */
  0,	/* dwModeIndex */
  NULL,	/* lpdwFourCC */
  0,	/* dwNumModes */
  NULL,	/* lpModeInfo */
  DDHALINFO_ISPRIMARYDISPLAY | DDHALINFO_MODEXILLEGAL,	/* dwFlags */
  NULL,	/* lpPDevice */
  0	/* hInstance */
};

static LPDDHALDDRAWFNS ddraw_fns;
static DWORD ddraw_ver;

static void X11DRV_DDHAL_SetInfo(void)
{
  (ddraw_fns->lpSetInfo)(&hal_info, FALSE);
}

INT X11DRV_DCICommand(INT cbInput, LPVOID lpInData, LPVOID lpOutData)
{
  LPDCICMD lpCmd = (LPDCICMD)lpInData;

  TRACE("(%d,(%ld,%ld,%ld),%p)\n", cbInput, lpCmd->dwCommand,
	lpCmd->dwParam1, lpCmd->dwParam2, lpOutData);

  switch (lpCmd->dwCommand) {
  case DDNEWCALLBACKFNS:
    ddraw_fns = (LPDDHALDDRAWFNS)lpCmd->dwParam1;
    return TRUE;
  case DDVERSIONINFO:
    {
      LPDDVERSIONDATA lpVer = (LPDDVERSIONDATA)lpOutData;
      ddraw_ver = lpCmd->dwParam1;
      if (!lpVer) break;
      /* well, whatever... the DDK says so */
      lpVer->dwHALVersion = DD_RUNTIME_VERSION;
    }
    return TRUE;
  case DDGET32BITDRIVERNAME:
    {
      LPDD32BITDRIVERDATA lpData = (LPDD32BITDRIVERDATA)lpOutData;
      /* here, we could ask ddraw to load a separate DLL, that
       * would contain the 32-bit ddraw HAL */
      strcpy(lpData->szName,"x11drv");
      /* the entry point named here should initialize our hal_info
       * with 32-bit entry points (ignored for now) */
      strcpy(lpData->szEntryPoint,"DriverInit");
      lpData->dwContext = 0;
    }
    return TRUE;
  case DDCREATEDRIVEROBJECT:
    {
      LPDWORD lpInstance = (LPDWORD)lpOutData;

      /* FIXME: get x11drv's hInstance */
#ifdef HAVE_LIBXXF86DGA2
      /*if (!X11DRV_XF86DGA2_CreateDriver(&hal_info))*/
#endif
      {
#ifdef HAVE_LIBXXF86VM
	X11DRV_XF86VM_CreateDriver(&hal_info);
#endif
      }

      (ddraw_fns->lpSetInfo)(&hal_info, FALSE);
      *lpInstance = hal_info.hInstance;
    }
    return TRUE;
  }
  return 0;
}

void X11DRV_DDHAL_SwitchMode(DWORD dwModeIndex, LPVOID fb_addr)
{
  LPDDHALMODEINFO info = &hal_info.lpModeInfo[dwModeIndex];

  hal_info.dwModeIndex        = dwModeIndex;
  hal_info.dwMonitorFrequency = info->wRefreshRate;
  hal_info.vmiData.fpPrimary  = (FLATPTR)fb_addr;
  hal_info.vmiData.dwDisplayWidth  = info->dwWidth;
  hal_info.vmiData.dwDisplayHeight = info->dwHeight;
  hal_info.vmiData.lDisplayPitch   = info->lPitch;
  hal_info.vmiData.ddpfDisplay.dwSize = info->dwBPP ? sizeof(hal_info.vmiData.ddpfDisplay) : 0;
  hal_info.vmiData.ddpfDisplay.dwFlags = (info->wFlags & DDMODEINFO_PALETTIZED) ? DDPF_PALETTEINDEXED8 : 0;
  hal_info.vmiData.ddpfDisplay.u1.dwRGBBitCount = (info->dwBPP > 24) ? 24 : info->dwBPP;
  hal_info.vmiData.ddpfDisplay.u2.dwRBitMask = info->dwRBitMask;
  hal_info.vmiData.ddpfDisplay.u3.dwGBitMask = info->dwGBitMask;
  hal_info.vmiData.ddpfDisplay.u4.dwBBitMask = info->dwBBitMask;

  X11DRV_DDHAL_SetInfo();
}

void X11DRV_DDHAL_SetPalEntries(Colormap pal, DWORD dwBase, DWORD dwNumEntries,
				LPPALETTEENTRY lpEntries)
{
  XColor c;
  int n;

  if (pal) {
    c.flags = DoRed|DoGreen|DoBlue;
    c.pixel = dwBase;
    for (n=0; n<dwNumEntries; n++,c.pixel++) {
      c.red   = lpEntries[n].peRed   << 8;
      c.green = lpEntries[n].peGreen << 8;
      c.blue  = lpEntries[n].peBlue  << 8;
      TSXStoreColor(display, pal, &c);
    }
    TSXFlush(display); /* update display immediate */
  }
}
