/*
 * Graphics driver management functions
 *
 * Copyright 1996 Alexandre Julliard
 */

#include <string.h>
#include "gdi.h"
#include "heap.h"
#include "debugtools.h"

DECLARE_DEBUG_CHANNEL(driver)
DECLARE_DEBUG_CHANNEL(gdi)

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
    GRAPHICS_DRIVER *driver = HeapAlloc( SystemHeap, 0, sizeof(*driver) );
    if (!driver) return FALSE;
    driver->funcs = funcs;
    if (name)
    {
        driver->name  = HEAP_strdupA( SystemHeap, 0, name );
        driver->next  = firstDriver;
        firstDriver = driver;
        return TRUE;
    }
    /* No name -> it's the generic driver */
    if (genericDriver)
    {
        WARN_(driver)(" already a generic driver\n" );
        HeapFree( SystemHeap, 0, driver );
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
                HeapFree( SystemHeap, 0, driver->name );
                HeapFree( SystemHeap, 0, driver );
                return TRUE;
            }
            ppDriver = &(*ppDriver)->next;
        }
        return FALSE;
    }
    else
    {
        if (!genericDriver) return FALSE;
        HeapFree( SystemHeap, 0, genericDriver );
        genericDriver = NULL;
        return TRUE;
    }
}



/*****************************************************************************
 *      GDI_CallDevInstall16   [GDI32.100]
 *
 * This should thunk to 16-bit and simply call the proc with the given args.
 */
INT WINAPI GDI_CallDevInstall16( FARPROC16 lpfnDevInstallProc, HWND hWnd, 
                                 LPSTR lpModelName, LPSTR OldPort, LPSTR NewPort )
{
    FIXME_(gdi)("(%p, %04x, %s, %s, %s)\n", 
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
    FIXME_(gdi)("(%04x, %s, %s, %p)\n", 
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
                                    LPDEVMODE16 lpdmOutput, LPSTR lpszDevice, 
                                    LPSTR lpszPort, LPDEVMODE16 lpdmInput, 
                                    LPSTR lpszProfile, DWORD fwMode )
{
    FIXME_(gdi)("(%04x, %p, %s, %s, %p, %s, %ld)\n", 
                hwnd, lpdmOutput, lpszDevice, lpszPort, 
                lpdmInput, lpszProfile, fwMode );
    return -1;
}

/****************************************************************************
 *      GDI_CallAdvancedSetupDialog16     [GDI32.103]
 *
 * This should load the correct driver for lpszDevice and calls this driver's
 * AdvancedSetupDialog proc.
 */
INT WINAPI GDI_CallAdvancedSetupDialog16( HWND hwnd, LPSTR lpszDevice,
                                          LPDEVMODE16 devin, LPDEVMODE16 devout )
{
    FIXME_(gdi)("(%04x, %s, %p, %p)\n", 
                hwnd, lpszDevice, devin, devout );
    return -1;
}

/*****************************************************************************
 *      GDI_CallDeviceCapabilities16      [GDI32.104]
 *
 * This should load the correct driver for lpszDevice and calls this driver's
 * DeviceCapabilities proc.
 */
DWORD WINAPI GDI_CallDeviceCapabilities16( LPSTR lpszDevice, LPSTR lpszPort,
                                           DWORD fwCapability, LPSTR lpszOutput, 
                                           LPDEVMODE16 lpdm )
{
    FIXME_(gdi)("(%s, %s, %ld, %p, %p)\n", 
                lpszDevice, lpszPort, fwCapability, lpszOutput, lpdm );
    return -1L;
}

