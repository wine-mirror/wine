/*
 * Graphics driver management functions
 *
 * Copyright 1996 Alexandre Julliard
 */

#include <string.h>
#include "gdi.h"
#include "heap.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(driver)

typedef struct tagGRAPHICS_DRIVER
{
    struct tagGRAPHICS_DRIVER *next;
    LPSTR                      name;
    const DC_FUNCTIONS        *funcs;
} GRAPHICS_DRIVER;

static GRAPHICS_DRIVER *firstDriver = NULL;
static GRAPHICS_DRIVER *genericDriver = NULL;

/**********************************************************************
 *	     DRIVER_RegisterDriver
 */
BOOL DRIVER_RegisterDriver( LPCSTR name, const DC_FUNCTIONS *funcs )
{
    GRAPHICS_DRIVER *driver = HeapAlloc( GetProcessHeap(), 0, sizeof(*driver) );
    if (!driver) return FALSE;
    driver->funcs = funcs;
    if (name)
    {
        driver->name  = HEAP_strdupA( GetProcessHeap(), 0, name );
        driver->next  = firstDriver;
        firstDriver = driver;
        return TRUE;
    }
    /* No name -> it's the generic driver */
    if (genericDriver)
    {
        WARN(" already a generic driver\n" );
        HeapFree( GetProcessHeap(), 0, driver );
        return FALSE;
    }
    driver->name = NULL;
    genericDriver = driver;
    return TRUE;
}


/**********************************************************************
 *	     DRIVER_FindDriver
 */
const DC_FUNCTIONS *DRIVER_FindDriver( LPCSTR name )
{
    GRAPHICS_DRIVER *driver = firstDriver;

    TRACE(": %s\n", name);
    while (driver && name)
    {
        if (!strcasecmp( driver->name, name )) return driver->funcs;
        driver = driver->next;
    }
    return genericDriver ? genericDriver->funcs : NULL;
}


/**********************************************************************
 *	     DRIVER_UnregisterDriver
 */
BOOL DRIVER_UnregisterDriver( LPCSTR name )
{
    if (name)
    {
        GRAPHICS_DRIVER **ppDriver = &firstDriver;
        while (*ppDriver)
        {
            if (!strcasecmp( (*ppDriver)->name, name ))
            {
                GRAPHICS_DRIVER *driver = *ppDriver;
                (*ppDriver) = driver->next;
                HeapFree( GetProcessHeap(), 0, driver->name );
                HeapFree( GetProcessHeap(), 0, driver );
                return TRUE;
            }
            ppDriver = &(*ppDriver)->next;
        }
        return FALSE;
    }
    else
    {
        if (!genericDriver) return FALSE;
        HeapFree( GetProcessHeap(), 0, genericDriver );
        genericDriver = NULL;
        return TRUE;
    }
}

/*****************************************************************************
 *      DRIVER_GetDriverName
 *
 */
BOOL DRIVER_GetDriverName( LPCSTR device, LPSTR driver, DWORD size )
{
    char *p;
    size = GetProfileStringA("devices", device, "", driver, size);
    if(!size) {
        WARN("Unable to find '%s' in [devices] section of win.ini\n", device);
	return FALSE;
    }
    p = strchr(driver, ',');
    if(!p) {
        WARN("'%s' entry in [devices] section of win.ini is malformed.\n",
	     device);
	return FALSE;
    }
    *p = '\0';
    TRACE("Found '%s' for '%s'\n", driver, device);
    return TRUE;
}
    
/*****************************************************************************
 *      GDI_CallDevInstall16   [GDI32.100]
 *
 * This should thunk to 16-bit and simply call the proc with the given args.
 */
INT WINAPI GDI_CallDevInstall16( FARPROC16 lpfnDevInstallProc, HWND hWnd, 
                                 LPSTR lpModelName, LPSTR OldPort, LPSTR NewPort )
{
    FIXME("(%p, %04x, %s, %s, %s)\n", 
	  lpfnDevInstallProc, hWnd, lpModelName, OldPort, NewPort );
    return -1;
}

/*****************************************************************************
 *      GDI_CallExtDeviceModePropSheet16   [GDI32.101]
 *
 * This should load the correct driver for lpszDevice and calls this driver's
 * ExtDeviceModePropSheet proc. 
 *
 * Note: The driver calls a callback routine for each property sheet page; these 
 * pages are supposed to be filled into the structure pointed to by lpPropSheet.
 * The layout of this structure is:
 * 
 * struct
 * {
 *   DWORD  nPages;
 *   DWORD  unknown;
 *   HPROPSHEETPAGE  pages[10];
 * };
 */
INT WINAPI GDI_CallExtDeviceModePropSheet16( HWND hWnd, LPCSTR lpszDevice, 
                                             LPCSTR lpszPort, LPVOID lpPropSheet )
{
    FIXME("(%04x, %s, %s, %p)\n", 
      hWnd, lpszDevice, lpszPort, lpPropSheet );
    return -1;
}

/*****************************************************************************
 *      GDI_CallExtDeviceMode16   [GDI32.102]
 *
 * This should load the correct driver for lpszDevice and calls this driver's
 * ExtDeviceMode proc.
 */
INT WINAPI GDI_CallExtDeviceMode16( HWND hwnd, 
                                    LPDEVMODEA lpdmOutput, LPSTR lpszDevice, 
                                    LPSTR lpszPort, LPDEVMODEA lpdmInput, 
                                    LPSTR lpszProfile, DWORD fwMode )
{
    char buf[300];
    const DC_FUNCTIONS *funcs;

    TRACE("(%04x, %p, %s, %s, %p, %s, %ld)\n", 
	  hwnd, lpdmOutput, lpszDevice, lpszPort, 
	  lpdmInput, lpszProfile, fwMode );

    if(!DRIVER_GetDriverName( lpszDevice, buf, sizeof(buf) )) return -1;
    funcs = DRIVER_FindDriver( buf );
    if(!funcs || !funcs->pExtDeviceMode) return -1;
    return funcs->pExtDeviceMode(buf, hwnd, lpdmOutput, lpszDevice, lpszPort,
				 lpdmInput, lpszProfile, fwMode);
}

/****************************************************************************
 *      GDI_CallAdvancedSetupDialog16     [GDI32.103]
 *
 * This should load the correct driver for lpszDevice and calls this driver's
 * AdvancedSetupDialog proc.
 */
INT WINAPI GDI_CallAdvancedSetupDialog16( HWND hwnd, LPSTR lpszDevice,
                                          LPDEVMODEA devin, LPDEVMODEA devout )
{
    TRACE("(%04x, %s, %p, %p)\n", 
	  hwnd, lpszDevice, devin, devout );
    return -1;
}

/*****************************************************************************
 *      GDI_CallDeviceCapabilities16      [GDI32.104]
 *
 * This should load the correct driver for lpszDevice and calls this driver's
 * DeviceCapabilities proc.
 */
DWORD WINAPI GDI_CallDeviceCapabilities16( LPCSTR lpszDevice, LPCSTR lpszPort,
                                           WORD fwCapability, LPSTR lpszOutput,
                                           LPDEVMODEA lpdm )
{
    char buf[300];
    const DC_FUNCTIONS *funcs;

    TRACE("(%s, %s, %d, %p, %p)\n", 
	  lpszDevice, lpszPort, fwCapability, lpszOutput, lpdm );


    if(!DRIVER_GetDriverName( lpszDevice, buf, sizeof(buf) )) return -1;
    funcs = DRIVER_FindDriver( buf );
    if(!funcs || !funcs->pDeviceCapabilities) return -1;
    return funcs->pDeviceCapabilities( buf, lpszDevice, lpszPort,
				       fwCapability, lpszOutput, lpdm);

}


