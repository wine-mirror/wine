/*
 * Graphics driver management functions
 *
 * Copyright 1996 Alexandre Julliard
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

#include "config.h"
#include "wine/port.h"

#include <string.h>
#include "winbase.h"
#include "winreg.h"

#include "gdi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(driver);

struct graphics_driver
{
    struct graphics_driver *next;
    struct graphics_driver *prev;
    HMODULE                 module;  /* module handle */
    unsigned int            count;   /* reference count */
    DC_FUNCTIONS            funcs;
};

static struct graphics_driver *first_driver;
static struct graphics_driver *display_driver;
static CRITICAL_SECTION driver_section = CRITICAL_SECTION_INIT( "driver_section" );

/**********************************************************************
 *	     create_driver
 *
 * Allocate and fill the driver structure for a given module.
 */
static struct graphics_driver *create_driver( HMODULE module )
{
    struct graphics_driver *driver;

    if (!(driver = HeapAlloc( GetProcessHeap(), 0, sizeof(*driver)))) return NULL;
    driver->next   = NULL;
    driver->prev   = NULL;
    driver->module = module;
    driver->count  = 1;

    /* fill the function table */

#define GET_FUNC(name) driver->funcs.p##name = (void*)GetProcAddress( module, #name )

    GET_FUNC(AbortDoc);
    GET_FUNC(AbortPath);
    GET_FUNC(AngleArc);
    GET_FUNC(Arc);
    GET_FUNC(ArcTo);
    GET_FUNC(BeginPath);
    GET_FUNC(BitBlt);
    GET_FUNC(ChoosePixelFormat);
    GET_FUNC(Chord);
    GET_FUNC(CloseFigure);
    GET_FUNC(CreateBitmap);
    GET_FUNC(CreateDC);
    GET_FUNC(CreateDIBSection);
    GET_FUNC(DeleteBitmap);
    GET_FUNC(DeleteDC);
    GET_FUNC(DescribePixelFormat);
    GET_FUNC(DeviceCapabilities);
    GET_FUNC(Ellipse);
    GET_FUNC(EndDoc);
    GET_FUNC(EndPage);
    GET_FUNC(EndPath);
    GET_FUNC(EnumDeviceFonts);
    GET_FUNC(ExcludeClipRect);
    GET_FUNC(ExtDeviceMode);
    GET_FUNC(ExtEscape);
    GET_FUNC(ExtFloodFill);
    GET_FUNC(ExtSelectClipRgn);
    GET_FUNC(ExtTextOut);
    GET_FUNC(FillPath);
    GET_FUNC(FillRgn);
    GET_FUNC(FlattenPath);
    GET_FUNC(FrameRgn);
    GET_FUNC(GdiComment);
    GET_FUNC(GetBitmapBits);
    GET_FUNC(GetCharWidth);
    GET_FUNC(GetDCOrgEx);
    GET_FUNC(GetDIBColorTable);
    GET_FUNC(GetDIBits);
    GET_FUNC(GetDeviceCaps);
    GET_FUNC(GetDeviceGammaRamp);
    GET_FUNC(GetNearestColor);
    GET_FUNC(GetPixel);
    GET_FUNC(GetPixelFormat);
    GET_FUNC(GetSystemPaletteEntries);
    GET_FUNC(GetTextExtentPoint);
    GET_FUNC(GetTextMetrics);
    GET_FUNC(IntersectClipRect);
    GET_FUNC(InvertRgn);
    GET_FUNC(LineTo);
    GET_FUNC(MoveTo);
    GET_FUNC(ModifyWorldTransform);
    GET_FUNC(OffsetClipRgn);
    GET_FUNC(OffsetViewportOrg);
    GET_FUNC(OffsetWindowOrg);
    GET_FUNC(PaintRgn);
    GET_FUNC(PatBlt);
    GET_FUNC(Pie);
    GET_FUNC(PolyBezier);
    GET_FUNC(PolyBezierTo);
    GET_FUNC(PolyDraw);
    GET_FUNC(PolyPolygon);
    GET_FUNC(PolyPolyline);
    GET_FUNC(Polygon);
    GET_FUNC(Polyline);
    GET_FUNC(PolylineTo);
    GET_FUNC(RealizeDefaultPalette);
    GET_FUNC(RealizePalette);
    GET_FUNC(Rectangle);
    GET_FUNC(ResetDC);
    GET_FUNC(RestoreDC);
    GET_FUNC(RoundRect);
    GET_FUNC(SaveDC);
    GET_FUNC(ScaleViewportExt);
    GET_FUNC(ScaleWindowExt);
    GET_FUNC(SelectBitmap);
    GET_FUNC(SelectBrush);
    GET_FUNC(SelectClipPath);
    GET_FUNC(SelectFont);
    GET_FUNC(SelectPalette);
    GET_FUNC(SelectPen);
    GET_FUNC(SetBitmapBits);
    GET_FUNC(SetBkColor);
    GET_FUNC(SetBkMode);
    GET_FUNC(SetDCOrg);
    GET_FUNC(SetDIBColorTable);
    GET_FUNC(SetDIBits);
    GET_FUNC(SetDIBitsToDevice);
    GET_FUNC(SetDeviceClipping);
    GET_FUNC(SetDeviceGammaRamp);
    GET_FUNC(SetMapMode);
    GET_FUNC(SetMapperFlags);
    GET_FUNC(SetPixel);
    GET_FUNC(SetPixelFormat);
    GET_FUNC(SetPolyFillMode);
    GET_FUNC(SetROP2);
    GET_FUNC(SetRelAbs);
    GET_FUNC(SetStretchBltMode);
    GET_FUNC(SetTextAlign);
    GET_FUNC(SetTextCharacterExtra);
    GET_FUNC(SetTextColor);
    GET_FUNC(SetTextJustification);
    GET_FUNC(SetViewportExt);
    GET_FUNC(SetViewportOrg);
    GET_FUNC(SetWindowExt);
    GET_FUNC(SetWindowOrg);
    GET_FUNC(SetWorldTransform);
    GET_FUNC(StartDoc);
    GET_FUNC(StartPage);
    GET_FUNC(StretchBlt);
    GET_FUNC(StretchDIBits);
    GET_FUNC(StrokeAndFillPath);
    GET_FUNC(StrokePath);
    GET_FUNC(SwapBuffers);
    GET_FUNC(WidenPath);
#undef GET_FUNC

    /* add it to the list */
    driver->prev = NULL;
    if ((driver->next = first_driver)) driver->next->prev = driver;
    first_driver = driver;
    return driver;
}


/**********************************************************************
 *	     load_display_driver
 *
 * Special case for loading the display driver: get the name from the config file
 */
static struct graphics_driver *load_display_driver(void)
{
    char buffer[MAX_PATH];
    HMODULE module;
    HKEY hkey;

    if (display_driver)  /* already loaded */
    {
        display_driver->count++;
        return display_driver;
    }

    strcpy( buffer, "x11drv" );  /* default value */
    if (!RegOpenKeyA( HKEY_LOCAL_MACHINE, "Software\\Wine\\Wine\\Config\\Wine", &hkey ))
    {
        DWORD type, count = sizeof(buffer);
        RegQueryValueExA( hkey, "GraphicsDriver", 0, &type, buffer, &count );
        RegCloseKey( hkey );
    }

    if (!(module = LoadLibraryA( buffer )))
    {
        MESSAGE( "Could not load graphics driver '%s'\n", buffer );
        return NULL;
    }

    if (!(display_driver = create_driver( module )))
    {
        MESSAGE( "Could not create graphics driver '%s'\n", buffer );
        FreeLibrary( module );
        return NULL;
    }

    display_driver->count++;  /* we don't want to free it */
    return display_driver;
}


/**********************************************************************
 *	     DRIVER_load_driver
 */
const DC_FUNCTIONS *DRIVER_load_driver( LPCSTR name )
{
    HMODULE module;
    struct graphics_driver *driver;

    EnterCriticalSection( &driver_section );

    /* display driver is a special case */
    if (!strcasecmp( name, "display" ))
    {
        driver = load_display_driver();
        LeaveCriticalSection( &driver_section );
        return &driver->funcs;
    }

    if ((module = GetModuleHandleA( name )))
    {
        for (driver = first_driver; driver; driver = driver->next)
        {
            if (driver->module == module)
            {
                driver->count++;
                LeaveCriticalSection( &driver_section );
                return &driver->funcs;
            }
        }
    }

    if (!(module = LoadLibraryA( name )))
    {
        LeaveCriticalSection( &driver_section );
        return NULL;
    }

    if (!(driver = create_driver( module )))
    {
        FreeLibrary( module );
        LeaveCriticalSection( &driver_section );
        return NULL;
    }

    TRACE( "loaded driver %p for %s\n", driver, name );
    LeaveCriticalSection( &driver_section );
    return &driver->funcs;
}


/**********************************************************************
 *	     DRIVER_get_driver
 *
 * Get a new copy of an existing driver.
 */
const DC_FUNCTIONS *DRIVER_get_driver( const DC_FUNCTIONS *funcs )
{
    struct graphics_driver *driver;

    EnterCriticalSection( &driver_section );
    for (driver = first_driver; driver; driver = driver->next)
        if (&driver->funcs == funcs) break;
    if (!driver) ERR( "driver not found, trouble ahead\n" );
    driver->count++;
    LeaveCriticalSection( &driver_section );
    return funcs;
}


/**********************************************************************
 *	     DRIVER_release_driver
 *
 * Release a driver by decrementing ref count and freeing it if needed.
 */
void DRIVER_release_driver( const DC_FUNCTIONS *funcs )
{
    struct graphics_driver *driver;

    EnterCriticalSection( &driver_section );

    for (driver = first_driver; driver; driver = driver->next)
        if (&driver->funcs == funcs) break;

    if (!driver) goto done;
    if (--driver->count) goto done;

    /* removed last reference, free it */
    if (driver->next) driver->next->prev = driver->prev;
    if (driver->prev) driver->prev->next = driver->next;
    else first_driver = driver->next;
    if (driver == display_driver) display_driver = NULL;

    FreeLibrary( driver->module );
    HeapFree( GetProcessHeap(), 0, driver );
 done:
    LeaveCriticalSection( &driver_section );
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
    if(!p)
    {
        WARN("'%s' entry in [devices] section of win.ini is malformed.\n", device);
        return FALSE;
    }
    *p = '\0';
    TRACE("Found '%s' for '%s'\n", driver, device);
    return TRUE;
}

/*****************************************************************************
 *      @ [GDI32.100]
 *
 * This should thunk to 16-bit and simply call the proc with the given args.
 */
INT WINAPI GDI_CallDevInstall16( FARPROC16 lpfnDevInstallProc, HWND hWnd,
                                 LPSTR lpModelName, LPSTR OldPort, LPSTR NewPort )
{
    FIXME("(%p, %p, %s, %s, %s)\n", lpfnDevInstallProc, hWnd, lpModelName, OldPort, NewPort );
    return -1;
}

/*****************************************************************************
 *      @ [GDI32.101]
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
    FIXME("(%p, %s, %s, %p)\n", hWnd, lpszDevice, lpszPort, lpPropSheet );
    return -1;
}

/*****************************************************************************
 *      @ [GDI32.102]
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
    HDC hdc;
    DC *dc;
    INT ret = -1;
    INT (*pExtDeviceMode)(LPSTR,HWND,LPDEVMODEA,LPSTR,LPSTR,LPDEVMODEA,LPSTR,DWORD);

    TRACE("(%p, %p, %s, %s, %p, %s, %ld)\n",
          hwnd, lpdmOutput, lpszDevice, lpszPort, lpdmInput, lpszProfile, fwMode );

    if(!DRIVER_GetDriverName( lpszDevice, buf, sizeof(buf) )) return -1;

    if (!(hdc = CreateICA( buf, lpszDevice, lpszPort, NULL ))) return -1;

    if ((dc = DC_GetDCPtr( hdc )))
    {
	pExtDeviceMode = dc->funcs->pExtDeviceMode;
	GDI_ReleaseObj( hdc );
	if (pExtDeviceMode)
	    ret = pExtDeviceMode(buf, hwnd, lpdmOutput, lpszDevice, lpszPort,
                                            lpdmInput, lpszProfile, fwMode);
    }
    DeleteDC( hdc );
    return ret;
}

/****************************************************************************
 *      @ [GDI32.103]
 *
 * This should load the correct driver for lpszDevice and calls this driver's
 * AdvancedSetupDialog proc.
 */
INT WINAPI GDI_CallAdvancedSetupDialog16( HWND hwnd, LPSTR lpszDevice,
                                          LPDEVMODEA devin, LPDEVMODEA devout )
{
    TRACE("(%p, %s, %p, %p)\n", hwnd, lpszDevice, devin, devout );
    return -1;
}

/*****************************************************************************
 *      @ [GDI32.104]
 *
 * This should load the correct driver for lpszDevice and calls this driver's
 * DeviceCapabilities proc.
 */
DWORD WINAPI GDI_CallDeviceCapabilities16( LPCSTR lpszDevice, LPCSTR lpszPort,
                                           WORD fwCapability, LPSTR lpszOutput,
                                           LPDEVMODEA lpdm )
{
    char buf[300];
    HDC hdc;
    DC *dc;
    INT ret = -1;

    TRACE("(%s, %s, %d, %p, %p)\n", lpszDevice, lpszPort, fwCapability, lpszOutput, lpdm );

    if(!DRIVER_GetDriverName( lpszDevice, buf, sizeof(buf) )) return -1;

    if (!(hdc = CreateICA( buf, lpszDevice, lpszPort, NULL ))) return -1;

    if ((dc = DC_GetDCPtr( hdc )))
    {
        if (dc->funcs->pDeviceCapabilities)
            ret = dc->funcs->pDeviceCapabilities( buf, lpszDevice, lpszPort,
                                                  fwCapability, lpszOutput, lpdm );
        GDI_ReleaseObj( hdc );
    }
    DeleteDC( hdc );
    return ret;
}
