/*
 * DirectDraw driver interface
 *
 * Copyright 2001 TransGaming Technologies, Inc.
 */

#include "windef.h"
#include "wingdi.h"
#include "ddrawi.h"
#include "xvidmode.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(x11drv);

static DWORD PASCAL X11DRV_DDHAL_DestroyDriver(LPDDHAL_DESTROYDRIVERDATA data)
{
  data->ddRVal = DD_OK;
  return DDHAL_DRIVER_HANDLED;
}

static DWORD PASCAL X11DRV_DDHAL_SetMode(LPDDHAL_SETMODEDATA data)
{
#ifdef HAVE_LIBXXF86VM
  if (xf86vm_mode_count) {
    X11DRV_XF86VM_SetCurrentMode(data->dwModeIndex);
    data->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;
  }
#endif
  return DDHAL_DRIVER_NOTHANDLED;
}

static DDHAL_DDCALLBACKS hal_ddcallbacks = {
  sizeof(DDHAL_DDCALLBACKS),
  0x3ff, /* all callbacks are 32-bit */
  X11DRV_DDHAL_DestroyDriver,
  NULL, /* CreateSurface */
  NULL, /* SetColorKey */
  X11DRV_DDHAL_SetMode,
  NULL, /* WaitForVerticalBlank */
  NULL, /* CanCreateSurface */
  NULL, /* CreatePalette */
  NULL, /* GetScanLine */
  NULL, /* SetExclusiveMode */
  NULL  /* FlipToGDISurface */
};

static DDHALINFO hal_info = {
  sizeof(DDHALINFO),
  &hal_ddcallbacks,
  /* more stuff */
};

static LPDDHALDDRAWFNS ddraw_fns;
static DWORD ddraw_ver;

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
       * with 32-bit entry points */
      strcpy(lpData->szEntryPoint,"DriverInit");
      lpData->dwContext = 0;
    }
    return TRUE;
  case DDCREATEDRIVEROBJECT:
    {
      LPDWORD lpInstance = (LPDWORD)lpOutData;

#ifdef HAVE_LIBXXF86VM
      hal_info.dwNumModes = xf86vm_mode_count;
      hal_info.lpModeInfo = xf86vm_modes;
      hal_info.dwModeIndex = X11DRV_XF86VM_GetCurrentMode();
#endif
      /* FIXME: get x11drv's hInstance */

      (ddraw_fns->lpSetInfo)(&hal_info, FALSE);
      *lpInstance = hal_info.hInstance;
    }
    return TRUE;
  }
  return 0;
}
