/*
 * System parameters functions
 *
 * Copyright 1994 Alexandre Julliard
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

#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "wine/wingdi16.h"
#include "winerror.h"

#include "initguid.h"
#include "d3dkmdt.h"
#include "devguid.h"
#include "setupapi.h"
#include "controls.h"
#include "user_private.h"
#include "wine/asm.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(system);


DEFINE_DEVPROPKEY(DEVPROPKEY_MONITOR_GPU_LUID, 0xca085853, 0x16ce, 0x48aa, 0xb1, 0x14, 0xde, 0x9c, 0x72, 0x33, 0x42, 0x23, 1);
DEFINE_DEVPROPKEY(DEVPROPKEY_MONITOR_OUTPUT_ID, 0xca085853, 0x16ce, 0x48aa, 0xb1, 0x14, 0xde, 0x9c, 0x72, 0x33, 0x42, 0x23, 2);

/* Wine specific monitor properties */
DEFINE_DEVPROPKEY(WINE_DEVPROPKEY_MONITOR_STATEFLAGS, 0x233a9ef3, 0xafc4, 0x4abd, 0xb5, 0x64, 0xc3, 0x2f, 0x21, 0xf1, 0x53, 0x5b, 2);
DEFINE_DEVPROPKEY(WINE_DEVPROPKEY_MONITOR_ADAPTERNAME, 0x233a9ef3, 0xafc4, 0x4abd, 0xb5, 0x64, 0xc3, 0x2f, 0x21, 0xf1, 0x53, 0x5b, 5);


static HDC display_dc;
static CRITICAL_SECTION display_dc_section;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &display_dc_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": display_dc_section") }
};
static CRITICAL_SECTION display_dc_section = { &critsect_debug, -1 ,0, 0, 0, 0 };


/* System parameters storage */
static UINT system_dpi;

static void SYSPARAMS_LogFont32WTo32A( const LOGFONTW* font32W, LPLOGFONTA font32A )
{
    font32A->lfHeight = font32W->lfHeight;
    font32A->lfWidth = font32W->lfWidth;
    font32A->lfEscapement = font32W->lfEscapement;
    font32A->lfOrientation = font32W->lfOrientation;
    font32A->lfWeight = font32W->lfWeight;
    font32A->lfItalic = font32W->lfItalic;
    font32A->lfUnderline = font32W->lfUnderline;
    font32A->lfStrikeOut = font32W->lfStrikeOut;
    font32A->lfCharSet = font32W->lfCharSet;
    font32A->lfOutPrecision = font32W->lfOutPrecision;
    font32A->lfClipPrecision = font32W->lfClipPrecision;
    font32A->lfQuality = font32W->lfQuality;
    font32A->lfPitchAndFamily = font32W->lfPitchAndFamily;
    WideCharToMultiByte( CP_ACP, 0, font32W->lfFaceName, -1, font32A->lfFaceName, LF_FACESIZE, NULL, NULL );
    font32A->lfFaceName[LF_FACESIZE-1] = 0;
}

static void SYSPARAMS_LogFont32ATo32W( const LOGFONTA* font32A, LPLOGFONTW font32W )
{
    font32W->lfHeight = font32A->lfHeight;
    font32W->lfWidth = font32A->lfWidth;
    font32W->lfEscapement = font32A->lfEscapement;
    font32W->lfOrientation = font32A->lfOrientation;
    font32W->lfWeight = font32A->lfWeight;
    font32W->lfItalic = font32A->lfItalic;
    font32W->lfUnderline = font32A->lfUnderline;
    font32W->lfStrikeOut = font32A->lfStrikeOut;
    font32W->lfCharSet = font32A->lfCharSet;
    font32W->lfOutPrecision = font32A->lfOutPrecision;
    font32W->lfClipPrecision = font32A->lfClipPrecision;
    font32W->lfQuality = font32A->lfQuality;
    font32W->lfPitchAndFamily = font32A->lfPitchAndFamily;
    MultiByteToWideChar( CP_ACP, 0, font32A->lfFaceName, -1, font32W->lfFaceName, LF_FACESIZE );
    font32W->lfFaceName[LF_FACESIZE-1] = 0;
}

static void SYSPARAMS_NonClientMetrics32WTo32A( const NONCLIENTMETRICSW* lpnm32W, LPNONCLIENTMETRICSA lpnm32A )
{
    lpnm32A->iBorderWidth	= lpnm32W->iBorderWidth;
    lpnm32A->iScrollWidth	= lpnm32W->iScrollWidth;
    lpnm32A->iScrollHeight	= lpnm32W->iScrollHeight;
    lpnm32A->iCaptionWidth	= lpnm32W->iCaptionWidth;
    lpnm32A->iCaptionHeight	= lpnm32W->iCaptionHeight;
    SYSPARAMS_LogFont32WTo32A(  &lpnm32W->lfCaptionFont,	&lpnm32A->lfCaptionFont );
    lpnm32A->iSmCaptionWidth	= lpnm32W->iSmCaptionWidth;
    lpnm32A->iSmCaptionHeight	= lpnm32W->iSmCaptionHeight;
    SYSPARAMS_LogFont32WTo32A( &lpnm32W->lfSmCaptionFont,	&lpnm32A->lfSmCaptionFont );
    lpnm32A->iMenuWidth		= lpnm32W->iMenuWidth;
    lpnm32A->iMenuHeight	= lpnm32W->iMenuHeight;
    SYSPARAMS_LogFont32WTo32A( &lpnm32W->lfMenuFont,		&lpnm32A->lfMenuFont );
    SYSPARAMS_LogFont32WTo32A( &lpnm32W->lfStatusFont,		&lpnm32A->lfStatusFont );
    SYSPARAMS_LogFont32WTo32A( &lpnm32W->lfMessageFont,		&lpnm32A->lfMessageFont );
    if (lpnm32A->cbSize > FIELD_OFFSET(NONCLIENTMETRICSA, iPaddedBorderWidth))
    {
        if (lpnm32W->cbSize > FIELD_OFFSET(NONCLIENTMETRICSW, iPaddedBorderWidth))
            lpnm32A->iPaddedBorderWidth = lpnm32W->iPaddedBorderWidth;
        else
            lpnm32A->iPaddedBorderWidth = 0;
    }
}

static void SYSPARAMS_NonClientMetrics32ATo32W( const NONCLIENTMETRICSA* lpnm32A, LPNONCLIENTMETRICSW lpnm32W )
{
    lpnm32W->iBorderWidth	= lpnm32A->iBorderWidth;
    lpnm32W->iScrollWidth	= lpnm32A->iScrollWidth;
    lpnm32W->iScrollHeight	= lpnm32A->iScrollHeight;
    lpnm32W->iCaptionWidth	= lpnm32A->iCaptionWidth;
    lpnm32W->iCaptionHeight	= lpnm32A->iCaptionHeight;
    SYSPARAMS_LogFont32ATo32W(  &lpnm32A->lfCaptionFont,	&lpnm32W->lfCaptionFont );
    lpnm32W->iSmCaptionWidth	= lpnm32A->iSmCaptionWidth;
    lpnm32W->iSmCaptionHeight	= lpnm32A->iSmCaptionHeight;
    SYSPARAMS_LogFont32ATo32W( &lpnm32A->lfSmCaptionFont,	&lpnm32W->lfSmCaptionFont );
    lpnm32W->iMenuWidth		= lpnm32A->iMenuWidth;
    lpnm32W->iMenuHeight	= lpnm32A->iMenuHeight;
    SYSPARAMS_LogFont32ATo32W( &lpnm32A->lfMenuFont,		&lpnm32W->lfMenuFont );
    SYSPARAMS_LogFont32ATo32W( &lpnm32A->lfStatusFont,		&lpnm32W->lfStatusFont );
    SYSPARAMS_LogFont32ATo32W( &lpnm32A->lfMessageFont,		&lpnm32W->lfMessageFont );
    if (lpnm32W->cbSize > FIELD_OFFSET(NONCLIENTMETRICSW, iPaddedBorderWidth))
    {
        if (lpnm32A->cbSize > FIELD_OFFSET(NONCLIENTMETRICSA, iPaddedBorderWidth))
            lpnm32W->iPaddedBorderWidth = lpnm32A->iPaddedBorderWidth;
        else
            lpnm32W->iPaddedBorderWidth = 0;
    }
}


/* Helper functions to retrieve monitors info */

static BOOL CALLBACK get_virtual_screen_proc( HMONITOR monitor, HDC hdc, LPRECT rect, LPARAM lp )
{
    RECT *virtual_rect = (RECT *)lp;

    UnionRect( virtual_rect, virtual_rect, rect );
    return TRUE;
}

RECT get_virtual_screen_rect(void)
{
    RECT rect = {0};

    NtUserEnumDisplayMonitors( 0, NULL, get_virtual_screen_proc, (LPARAM)&rect );
    return rect;
}

static BOOL CALLBACK get_primary_monitor_proc( HMONITOR monitor, HDC hdc, LPRECT rect, LPARAM lp )
{
    RECT *primary_rect = (RECT *)lp;

    if (!rect->top && !rect->left && rect->right && rect->bottom)
    {
        *primary_rect = *rect;
        return FALSE;
    }

    return TRUE;
}

RECT get_primary_monitor_rect(void)
{
    RECT rect = {0};

    NtUserEnumDisplayMonitors( 0, NULL, get_primary_monitor_proc, (LPARAM)&rect );
    return rect;
}

HDC get_display_dc(void)
{
    EnterCriticalSection( &display_dc_section );
    if (!display_dc)
    {
        HDC dc;

        LeaveCriticalSection( &display_dc_section );
        dc = CreateDCW( L"DISPLAY", NULL, NULL, NULL );
        EnterCriticalSection( &display_dc_section );
        if (display_dc)
            DeleteDC(dc);
        else
            display_dc = dc;
    }
    return display_dc;
}

void release_display_dc( HDC hdc )
{
    LeaveCriticalSection( &display_dc_section );
}

static HANDLE get_display_device_init_mutex( void )
{
    HANDLE mutex = CreateMutexW( NULL, FALSE, L"display_device_init" );

    WaitForSingleObject( mutex, INFINITE );
    return mutex;
}

static void release_display_device_init_mutex( HANDLE mutex )
{
    ReleaseMutex( mutex );
    CloseHandle( mutex );
}

/* Wait until graphics driver is loaded by explorer */
void wait_graphics_driver_ready(void)
{
    static BOOL ready = FALSE;

    if (!ready)
    {
        SendMessageW( GetDesktopWindow(), WM_NULL, 0, 0 );
        ready = TRUE;
    }
}

/***********************************************************************
 *           SYSPARAMS_Init
 */
void SYSPARAMS_Init(void)
{
    system_dpi = NtUserGetSystemDpiForProcess( NULL );
}

static BOOL update_desktop_wallpaper(void)
{
    DWORD pid;

    if (GetWindowThreadProcessId( GetDesktopWindow(), &pid ) && pid == GetCurrentProcessId())
    {
        WCHAR wallpaper[MAX_PATH], pattern[256];

        if (NtUserSystemParametersInfo( SPI_GETDESKWALLPAPER, ARRAYSIZE(wallpaper), wallpaper, 0 ) &&
            NtUserCallOneParam( (ULONG_PTR)pattern, NtUserGetDeskPattern ))
        {
            update_wallpaper( wallpaper, pattern );
        }
    }
    else SendMessageW( GetDesktopWindow(), WM_SETTINGCHANGE, SPI_SETDESKWALLPAPER, 0 );
    return TRUE;
}


/***********************************************************************
 *	        SystemParametersInfoForDpi (USER32.@)
 */
BOOL WINAPI SystemParametersInfoForDpi( UINT action, UINT val, PVOID ptr, UINT winini, UINT dpi )
{
    BOOL ret = NtUserSystemParametersInfoForDpi( action, val, ptr, winini, dpi );
    if (ret && (action == SPI_SETDESKWALLPAPER || action == SPI_SETDESKPATTERN))
        ret = update_desktop_wallpaper();
    return ret;
}


/***********************************************************************
 *		SystemParametersInfoW (USER32.@)
 */
BOOL WINAPI SystemParametersInfoW( UINT action, UINT val, void *ptr, UINT winini )
{
    BOOL ret = NtUserSystemParametersInfo( action, val, ptr, winini );
    if (ret && (action == SPI_SETDESKWALLPAPER || action == SPI_SETDESKPATTERN))
        ret = update_desktop_wallpaper();
    return ret;
}


/***********************************************************************
 *		SystemParametersInfoA (USER32.@)
 */
BOOL WINAPI SystemParametersInfoA( UINT uiAction, UINT uiParam,
				   PVOID pvParam, UINT fuWinIni )
{
    BOOL ret;

    TRACE("(%u, %u, %p, %u)\n", uiAction, uiParam, pvParam, fuWinIni);

    switch (uiAction)
    {
    case SPI_SETDESKWALLPAPER:			/*     20 */
    case SPI_SETDESKPATTERN:			/*     21 */
    {
	WCHAR buffer[256];
	if (pvParam)
            if (!MultiByteToWideChar( CP_ACP, 0, pvParam, -1, buffer, ARRAY_SIZE( buffer )))
                buffer[ARRAY_SIZE(buffer)-1] = 0;
	ret = SystemParametersInfoW( uiAction, uiParam, pvParam ? buffer : NULL, fuWinIni );
	break;
    }

    case SPI_GETICONTITLELOGFONT:		/*     31 */
    {
	LOGFONTW tmp;
	ret = SystemParametersInfoW( uiAction, uiParam, pvParam ? &tmp : NULL, fuWinIni );
	if (ret && pvParam)
            SYSPARAMS_LogFont32WTo32A( &tmp, pvParam );
	break;
    }

    case SPI_GETNONCLIENTMETRICS: 		/*     41  WINVER >= 0x400 */
    {
	NONCLIENTMETRICSW tmp;
        LPNONCLIENTMETRICSA lpnmA = pvParam;
        if (lpnmA && (lpnmA->cbSize == sizeof(NONCLIENTMETRICSA) ||
                      lpnmA->cbSize == FIELD_OFFSET(NONCLIENTMETRICSA, iPaddedBorderWidth)))
	{
	    tmp.cbSize = sizeof(NONCLIENTMETRICSW);
	    ret = SystemParametersInfoW( uiAction, uiParam, &tmp, fuWinIni );
	    if (ret)
		SYSPARAMS_NonClientMetrics32WTo32A( &tmp, lpnmA );
	}
	else
	    ret = FALSE;
	break;
    }

    case SPI_SETNONCLIENTMETRICS: 		/*     42  WINVER >= 0x400 */
    {
        NONCLIENTMETRICSW tmp;
        LPNONCLIENTMETRICSA lpnmA = pvParam;
        if (lpnmA && (lpnmA->cbSize == sizeof(NONCLIENTMETRICSA) ||
                      lpnmA->cbSize == FIELD_OFFSET(NONCLIENTMETRICSA, iPaddedBorderWidth)))
        {
            tmp.cbSize = sizeof(NONCLIENTMETRICSW);
            SYSPARAMS_NonClientMetrics32ATo32W( lpnmA, &tmp );
            ret = SystemParametersInfoW( uiAction, uiParam, &tmp, fuWinIni );
        }
        else
            ret = FALSE;
        break;
    }

    case SPI_GETICONMETRICS:			/*     45  WINVER >= 0x400 */
    {
	ICONMETRICSW tmp;
        LPICONMETRICSA lpimA = pvParam;
	if (lpimA && lpimA->cbSize == sizeof(ICONMETRICSA))
	{
	    tmp.cbSize = sizeof(ICONMETRICSW);
	    ret = SystemParametersInfoW( uiAction, uiParam, &tmp, fuWinIni );
	    if (ret)
	    {
		lpimA->iHorzSpacing = tmp.iHorzSpacing;
		lpimA->iVertSpacing = tmp.iVertSpacing;
		lpimA->iTitleWrap   = tmp.iTitleWrap;
		SYSPARAMS_LogFont32WTo32A( &tmp.lfFont, &lpimA->lfFont );
	    }
	}
	else
	    ret = FALSE;
	break;
    }

    case SPI_SETICONMETRICS:			/*     46  WINVER >= 0x400 */
    {
        ICONMETRICSW tmp;
        LPICONMETRICSA lpimA = pvParam;
        if (lpimA && lpimA->cbSize == sizeof(ICONMETRICSA))
        {
            tmp.cbSize = sizeof(ICONMETRICSW);
            tmp.iHorzSpacing = lpimA->iHorzSpacing;
            tmp.iVertSpacing = lpimA->iVertSpacing;
            tmp.iTitleWrap = lpimA->iTitleWrap;
            SYSPARAMS_LogFont32ATo32W(  &lpimA->lfFont, &tmp.lfFont);
            ret = SystemParametersInfoW( uiAction, uiParam, &tmp, fuWinIni );
        }
        else
            ret = FALSE;
        break;
    }

    case SPI_GETHIGHCONTRAST:			/*     66  WINVER >= 0x400 */
    {
	HIGHCONTRASTW tmp;
        LPHIGHCONTRASTA lphcA = pvParam;
	if (lphcA && lphcA->cbSize == sizeof(HIGHCONTRASTA))
	{
	    tmp.cbSize = sizeof(HIGHCONTRASTW);
	    ret = SystemParametersInfoW( uiAction, uiParam, &tmp, fuWinIni );
	    if (ret)
	    {
		lphcA->dwFlags = tmp.dwFlags;
		lphcA->lpszDefaultScheme = NULL;  /* FIXME? */
	    }
	}
	else
	    ret = FALSE;
	break;
    }

    case SPI_GETDESKWALLPAPER:                  /*     115 */
    {
        WCHAR buffer[MAX_PATH];
        ret = (SystemParametersInfoW( SPI_GETDESKWALLPAPER, uiParam, buffer, fuWinIni ) &&
               WideCharToMultiByte(CP_ACP, 0, buffer, -1, pvParam, uiParam, NULL, NULL));
        break;
    }

    default:
        ret = SystemParametersInfoW( uiAction, uiParam, pvParam, fuWinIni );
        break;
    }
    return ret;
}


/***********************************************************************
 *		GetSystemMetrics (USER32.@)
 */
INT WINAPI GetSystemMetrics( INT index )
{
    return NtUserCallOneParam( index, NtUserGetSystemMetrics );
}


/***********************************************************************
 *		GetSystemMetricsForDpi (USER32.@)
 */
INT WINAPI GetSystemMetricsForDpi( INT index, UINT dpi )
{
    return NtUserCallTwoParam( index, dpi, NtUserGetSystemMetricsForDpi );
}


/***********************************************************************
 *		SwapMouseButton (USER32.@)
 *  Reverse or restore the meaning of the left and right mouse buttons
 *  fSwap  [I ] TRUE - reverse, FALSE - original
 * RETURN
 *   previous state 
 */
BOOL WINAPI SwapMouseButton( BOOL fSwap )
{
    BOOL prev = GetSystemMetrics(SM_SWAPBUTTON);
    SystemParametersInfoW(SPI_SETMOUSEBUTTONSWAP, fSwap, 0, 0);
    return prev;
}


/**********************************************************************
 *		SetDoubleClickTime (USER32.@)
 */
BOOL WINAPI SetDoubleClickTime( UINT interval )
{
    return SystemParametersInfoW(SPI_SETDOUBLECLICKTIME, interval, 0, 0);
}


/*************************************************************************
 *		GetSysColor (USER32.@)
 */
COLORREF WINAPI DECLSPEC_HOTPATCH GetSysColor( INT index )
{
    return NtUserCallOneParam( index, NtUserGetSysColor );
}


/*************************************************************************
 *		SetSysColorsTemp (USER32.@)
 */
DWORD_PTR WINAPI SetSysColorsTemp( const COLORREF *pPens, const HBRUSH *pBrushes, DWORD_PTR n)
{
    FIXME( "no longer supported\n" );
    return FALSE;
}


/***********************************************************************
 *		GetSysColorBrush (USER32.@)
 */
HBRUSH WINAPI DECLSPEC_HOTPATCH GetSysColorBrush( INT index )
{
    return UlongToHandle( NtUserCallOneParam( index, NtUserGetSysColorBrush ));
}


/***********************************************************************
 *		SYSCOLOR_GetPen
 */
HPEN SYSCOLOR_GetPen( INT index )
{
    return UlongToHandle( NtUserCallOneParam( index, NtUserGetSysColorPen ));
}


/***********************************************************************
 *		SYSCOLOR_Get55AABrush
 */
HBRUSH SYSCOLOR_Get55AABrush(void)
{
    return UlongToHandle( NtUserCallOneParam( COLOR_55AA_BRUSH, NtUserGetSysColorBrush ));
}

/***********************************************************************
 *		ChangeDisplaySettingsA (USER32.@)
 */
LONG WINAPI ChangeDisplaySettingsA( LPDEVMODEA devmode, DWORD flags )
{
    if (devmode) devmode->dmDriverExtra = 0;

    return ChangeDisplaySettingsExA(NULL,devmode,NULL,flags,NULL);
}


/***********************************************************************
 *		ChangeDisplaySettingsW (USER32.@)
 */
LONG WINAPI ChangeDisplaySettingsW( LPDEVMODEW devmode, DWORD flags )
{
    if (devmode) devmode->dmDriverExtra = 0;

    return ChangeDisplaySettingsExW(NULL,devmode,NULL,flags,NULL);
}


/***********************************************************************
 *		ChangeDisplaySettingsExA (USER32.@)
 */
LONG WINAPI ChangeDisplaySettingsExA( LPCSTR devname, LPDEVMODEA devmode, HWND hwnd,
                                      DWORD flags, LPVOID lparam )
{
    LONG ret;
    UNICODE_STRING nameW;

    if (devname) RtlCreateUnicodeStringFromAsciiz(&nameW, devname);
    else nameW.Buffer = NULL;

    if (devmode)
    {
        DEVMODEW *devmodeW;

        devmodeW = GdiConvertToDevmodeW(devmode);
        if (devmodeW)
        {
            ret = ChangeDisplaySettingsExW(nameW.Buffer, devmodeW, hwnd, flags, lparam);
            HeapFree(GetProcessHeap(), 0, devmodeW);
        }
        else
            ret = DISP_CHANGE_SUCCESSFUL;
    }
    else
    {
        ret = ChangeDisplaySettingsExW(nameW.Buffer, NULL, hwnd, flags, lparam);
    }

    if (devname) RtlFreeUnicodeString(&nameW);
    return ret;
}


/***********************************************************************
 *		ChangeDisplaySettingsExW (USER32.@)
 */
LONG WINAPI ChangeDisplaySettingsExW( LPCWSTR devname, LPDEVMODEW devmode, HWND hwnd,
                                      DWORD flags, LPVOID lparam )
{
    UNICODE_STRING str;
    RtlInitUnicodeString( &str, devname );
    return NtUserChangeDisplaySettings( &str, devmode, hwnd, flags, lparam );
}


/***********************************************************************
 *		EnumDisplaySettingsW (USER32.@)
 *
 * RETURNS
 *	TRUE if nth setting exists found (described in the LPDEVMODEW struct)
 *	FALSE if we do not have the nth setting
 */
BOOL WINAPI EnumDisplaySettingsW( LPCWSTR name, DWORD n, LPDEVMODEW devmode )
{
    return EnumDisplaySettingsExW(name, n, devmode, 0);
}


/***********************************************************************
 *		EnumDisplaySettingsA (USER32.@)
 */
BOOL WINAPI EnumDisplaySettingsA(LPCSTR name,DWORD n,LPDEVMODEA devmode)
{
    return EnumDisplaySettingsExA(name, n, devmode, 0);
}


/***********************************************************************
 *		EnumDisplaySettingsExA (USER32.@)
 */
BOOL WINAPI EnumDisplaySettingsExA(LPCSTR lpszDeviceName, DWORD iModeNum,
                                   LPDEVMODEA lpDevMode, DWORD dwFlags)
{
    DEVMODEW devmodeW;
    BOOL ret;
    UNICODE_STRING nameW;

    if (lpszDeviceName) RtlCreateUnicodeStringFromAsciiz(&nameW, lpszDeviceName);
    else nameW.Buffer = NULL;

    memset(&devmodeW, 0, sizeof(devmodeW));
    devmodeW.dmSize = sizeof(devmodeW);
    ret = EnumDisplaySettingsExW(nameW.Buffer,iModeNum,&devmodeW,dwFlags);
    if (ret)
    {
        lpDevMode->dmSize = FIELD_OFFSET(DEVMODEA, dmICMMethod);
        lpDevMode->dmSpecVersion = devmodeW.dmSpecVersion;
        lpDevMode->dmDriverVersion = devmodeW.dmDriverVersion;
        WideCharToMultiByte(CP_ACP, 0, devmodeW.dmDeviceName, -1,
                            (LPSTR)lpDevMode->dmDeviceName, CCHDEVICENAME, NULL, NULL);
        lpDevMode->dmDriverExtra      = 0; /* FIXME */
        lpDevMode->dmBitsPerPel       = devmodeW.dmBitsPerPel;
        lpDevMode->dmPelsHeight       = devmodeW.dmPelsHeight;
        lpDevMode->dmPelsWidth        = devmodeW.dmPelsWidth;
        lpDevMode->u2.dmDisplayFlags  = devmodeW.u2.dmDisplayFlags;
        lpDevMode->dmDisplayFrequency = devmodeW.dmDisplayFrequency;
        lpDevMode->dmFields           = devmodeW.dmFields;

        lpDevMode->u1.s2.dmPosition.x = devmodeW.u1.s2.dmPosition.x;
        lpDevMode->u1.s2.dmPosition.y = devmodeW.u1.s2.dmPosition.y;
        lpDevMode->u1.s2.dmDisplayOrientation = devmodeW.u1.s2.dmDisplayOrientation;
        lpDevMode->u1.s2.dmDisplayFixedOutput = devmodeW.u1.s2.dmDisplayFixedOutput;
    }
    if (lpszDeviceName) RtlFreeUnicodeString(&nameW);
    return ret;
}


/***********************************************************************
 *             EnumDisplaySettingsExW (USER32.@)
 */
BOOL WINAPI EnumDisplaySettingsExW( const WCHAR *device, DWORD mode,
                                    DEVMODEW *dev_mode, DWORD flags )
{
    UNICODE_STRING str;
    RtlInitUnicodeString( &str, device );
    return NtUserEnumDisplaySettings( &str, mode, dev_mode, flags );
}

/**********************************************************************
 *              get_monitor_dpi
 */
UINT get_monitor_dpi( HMONITOR monitor )
{
    /* FIXME: use the monitor DPI instead */
    return system_dpi;
}

/**********************************************************************
 *              get_win_monitor_dpi
 */
UINT get_win_monitor_dpi( HWND hwnd )
{
    /* FIXME: use the monitor DPI instead */
    return system_dpi;
}

/**********************************************************************
 *              get_thread_dpi
 */
UINT get_thread_dpi(void)
{
    switch (GetAwarenessFromDpiAwarenessContext( GetThreadDpiAwarenessContext() ))
    {
    case DPI_AWARENESS_UNAWARE:      return USER_DEFAULT_SCREEN_DPI;
    case DPI_AWARENESS_SYSTEM_AWARE: return system_dpi;
    default:                         return 0;  /* no scaling */
    }
}

/**********************************************************************
 *              map_dpi_point
 */
POINT map_dpi_point( POINT pt, UINT dpi_from, UINT dpi_to )
{
    if (dpi_from && dpi_to && dpi_from != dpi_to)
    {
        pt.x = MulDiv( pt.x, dpi_to, dpi_from );
        pt.y = MulDiv( pt.y, dpi_to, dpi_from );
    }
    return pt;
}

/**********************************************************************
 *              point_win_to_phys_dpi
 */
POINT point_win_to_phys_dpi( HWND hwnd, POINT pt )
{
    return map_dpi_point( pt, GetDpiForWindow( hwnd ), get_win_monitor_dpi( hwnd ) );
}

/**********************************************************************
 *              point_phys_to_win_dpi
 */
POINT point_phys_to_win_dpi( HWND hwnd, POINT pt )
{
    return map_dpi_point( pt, get_win_monitor_dpi( hwnd ), GetDpiForWindow( hwnd ));
}

/**********************************************************************
 *              point_win_to_thread_dpi
 */
POINT point_win_to_thread_dpi( HWND hwnd, POINT pt )
{
    UINT dpi = get_thread_dpi();
    if (!dpi) dpi = get_win_monitor_dpi( hwnd );
    return map_dpi_point( pt, GetDpiForWindow( hwnd ), dpi );
}

/**********************************************************************
 *              point_thread_to_win_dpi
 */
POINT point_thread_to_win_dpi( HWND hwnd, POINT pt )
{
    UINT dpi = get_thread_dpi();
    if (!dpi) dpi = get_win_monitor_dpi( hwnd );
    return map_dpi_point( pt, dpi, GetDpiForWindow( hwnd ));
}

/**********************************************************************
 *              map_dpi_rect
 */
RECT map_dpi_rect( RECT rect, UINT dpi_from, UINT dpi_to )
{
    if (dpi_from && dpi_to && dpi_from != dpi_to)
    {
        rect.left   = MulDiv( rect.left, dpi_to, dpi_from );
        rect.top    = MulDiv( rect.top, dpi_to, dpi_from );
        rect.right  = MulDiv( rect.right, dpi_to, dpi_from );
        rect.bottom = MulDiv( rect.bottom, dpi_to, dpi_from );
    }
    return rect;
}

/**********************************************************************
 *              rect_win_to_thread_dpi
 */
RECT rect_win_to_thread_dpi( HWND hwnd, RECT rect )
{
    UINT dpi = get_thread_dpi();
    if (!dpi) dpi = get_win_monitor_dpi( hwnd );
    return map_dpi_rect( rect, GetDpiForWindow( hwnd ), dpi );
}

/**********************************************************************
 *              rect_thread_to_win_dpi
 */
RECT rect_thread_to_win_dpi( HWND hwnd, RECT rect )
{
    UINT dpi = get_thread_dpi();
    if (!dpi) dpi = get_win_monitor_dpi( hwnd );
    return map_dpi_rect( rect, dpi, GetDpiForWindow( hwnd ) );
}

/**********************************************************************
 *              SetProcessDpiAwarenessContext   (USER32.@)
 */
BOOL WINAPI SetProcessDpiAwarenessContext( DPI_AWARENESS_CONTEXT context )
{
    ULONG awareness;

    switch (GetAwarenessFromDpiAwarenessContext( context ))
    {
    case DPI_AWARENESS_UNAWARE:
        awareness = NTUSER_DPI_UNAWARE;
        break;
    case DPI_AWARENESS_SYSTEM_AWARE:
        awareness = NTUSER_DPI_SYSTEM_AWARE;
        break;
    case DPI_AWARENESS_PER_MONITOR_AWARE:
        awareness = context == DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
            ? NTUSER_DPI_PER_MONITOR_AWARE_V2 : NTUSER_DPI_PER_MONITOR_AWARE;
        break;
    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (!NtUserSetProcessDpiAwarenessContext( awareness, 0 ))
    {
        SetLastError( ERROR_ACCESS_DENIED );
        return FALSE;
    }

    TRACE( "set to %p\n", context );
    return TRUE;
}

/**********************************************************************
 *              GetProcessDpiAwarenessInternal   (USER32.@)
 */
BOOL WINAPI GetProcessDpiAwarenessInternal( HANDLE process, DPI_AWARENESS *awareness )
{
    *awareness = NtUserGetProcessDpiAwarenessContext( process ) & 3;
    return TRUE;
}

/**********************************************************************
 *              SetProcessDpiAwarenessInternal   (USER32.@)
 */
BOOL WINAPI SetProcessDpiAwarenessInternal( DPI_AWARENESS awareness )
{
    static const DPI_AWARENESS_CONTEXT contexts[3] = { DPI_AWARENESS_CONTEXT_UNAWARE,
                                                       DPI_AWARENESS_CONTEXT_SYSTEM_AWARE,
                                                       DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE };

    if (awareness < DPI_AWARENESS_UNAWARE || awareness > DPI_AWARENESS_PER_MONITOR_AWARE)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    return SetProcessDpiAwarenessContext( contexts[awareness] );
}

/***********************************************************************
 *              AreDpiAwarenessContextsEqual   (USER32.@)
 */
BOOL WINAPI AreDpiAwarenessContextsEqual( DPI_AWARENESS_CONTEXT ctx1, DPI_AWARENESS_CONTEXT ctx2 )
{
    DPI_AWARENESS aware1 = GetAwarenessFromDpiAwarenessContext( ctx1 );
    DPI_AWARENESS aware2 = GetAwarenessFromDpiAwarenessContext( ctx2 );
    return aware1 != DPI_AWARENESS_INVALID && aware1 == aware2;
}

/***********************************************************************
 *              GetAwarenessFromDpiAwarenessContext   (USER32.@)
 */
DPI_AWARENESS WINAPI GetAwarenessFromDpiAwarenessContext( DPI_AWARENESS_CONTEXT context )
{
    switch ((ULONG_PTR)context)
    {
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x80000010:
    case 0x80000011:
    case 0x80000012:
        return (ULONG_PTR)context & 3;
    case (ULONG_PTR)DPI_AWARENESS_CONTEXT_UNAWARE:
    case (ULONG_PTR)DPI_AWARENESS_CONTEXT_SYSTEM_AWARE:
    case (ULONG_PTR)DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE:
        return ~(ULONG_PTR)context;
    default:
        return DPI_AWARENESS_INVALID;
    }
}

/***********************************************************************
 *              IsValidDpiAwarenessContext   (USER32.@)
 */
BOOL WINAPI IsValidDpiAwarenessContext( DPI_AWARENESS_CONTEXT context )
{
    return GetAwarenessFromDpiAwarenessContext( context ) != DPI_AWARENESS_INVALID;
}

/***********************************************************************
 *              SetProcessDPIAware   (USER32.@)
 */
BOOL WINAPI SetProcessDPIAware(void)
{
    TRACE("\n");
    NtUserSetProcessDpiAwarenessContext( NTUSER_DPI_SYSTEM_AWARE, 0 );
    return TRUE;
}

/***********************************************************************
 *              IsProcessDPIAware   (USER32.@)
 */
BOOL WINAPI IsProcessDPIAware(void)
{
    return GetAwarenessFromDpiAwarenessContext( GetThreadDpiAwarenessContext() ) != DPI_AWARENESS_UNAWARE;
}

/**********************************************************************
 *              EnableNonClientDpiScaling   (USER32.@)
 */
BOOL WINAPI EnableNonClientDpiScaling( HWND hwnd )
{
    FIXME("(%p): stub\n", hwnd);
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *              GetDpiForSystem   (USER32.@)
 */
UINT WINAPI GetDpiForSystem(void)
{
    if (!IsProcessDPIAware()) return USER_DEFAULT_SCREEN_DPI;
    return system_dpi;
}

/**********************************************************************
 *              GetThreadDpiAwarenessContext   (USER32.@)
 */
DPI_AWARENESS_CONTEXT WINAPI GetThreadDpiAwarenessContext(void)
{
    struct user_thread_info *info = get_user_thread_info();

    if (info->dpi_awareness) return ULongToHandle( info->dpi_awareness );
    return UlongToHandle( (NtUserGetProcessDpiAwarenessContext( GetCurrentProcess() ) & 3 ) | 0x10 );
}

/**********************************************************************
 *              SetThreadDpiAwarenessContext   (USER32.@)
 */
DPI_AWARENESS_CONTEXT WINAPI SetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT context )
{
    struct user_thread_info *info = get_user_thread_info();
    DPI_AWARENESS prev, val = GetAwarenessFromDpiAwarenessContext( context );

    if (val == DPI_AWARENESS_INVALID)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (!(prev = info->dpi_awareness))
    {
        prev = NtUserGetProcessDpiAwarenessContext( GetCurrentProcess() ) & 3;
        prev |= 0x80000010;  /* restore to process default */
    }
    if (((ULONG_PTR)context & ~(ULONG_PTR)0x13) == 0x80000000) info->dpi_awareness = 0;
    else info->dpi_awareness = val | 0x10;
    return ULongToHandle( prev );
}

/**********************************************************************
 *              LogicalToPhysicalPointForPerMonitorDPI   (USER32.@)
 */
BOOL WINAPI LogicalToPhysicalPointForPerMonitorDPI( HWND hwnd, POINT *pt )
{
    RECT rect;

    if (!GetWindowRect( hwnd, &rect )) return FALSE;
    if (pt->x < rect.left || pt->y < rect.top || pt->x > rect.right || pt->y > rect.bottom) return FALSE;
    *pt = point_win_to_phys_dpi( hwnd, *pt );
    return TRUE;
}

/**********************************************************************
 *              PhysicalToLogicalPointForPerMonitorDPI   (USER32.@)
 */
BOOL WINAPI PhysicalToLogicalPointForPerMonitorDPI( HWND hwnd, POINT *pt )
{
    DPI_AWARENESS_CONTEXT context;
    RECT rect;
    BOOL ret = FALSE;

    context = SetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE );
    if (GetWindowRect( hwnd, &rect ) &&
        pt->x >= rect.left && pt->y >= rect.top && pt->x <= rect.right && pt->y <= rect.bottom)
    {
        *pt = point_phys_to_win_dpi( hwnd, *pt );
        ret = TRUE;
    }
    SetThreadDpiAwarenessContext( context );
    return ret;
}

/***********************************************************************
 *		MonitorFromRect (USER32.@)
 */
HMONITOR WINAPI MonitorFromRect( const RECT *rect, DWORD flags )
{
    return UlongToHandle( NtUserCallTwoParam( (LONG_PTR)rect, flags, NtUserMonitorFromRect ));
}

/***********************************************************************
 *		MonitorFromPoint (USER32.@)
 */
HMONITOR WINAPI MonitorFromPoint( POINT pt, DWORD flags )
{
    RECT rect;

    SetRect( &rect, pt.x, pt.y, pt.x + 1, pt.y + 1 );
    return MonitorFromRect( &rect, flags );
}

/***********************************************************************
 *		MonitorFromWindow (USER32.@)
 */
HMONITOR WINAPI MonitorFromWindow( HWND hwnd, DWORD flags )
{
    return UlongToHandle( NtUserCallHwndParam(  hwnd, flags, NtUserMonitorFromWindow ));
}

/***********************************************************************
 *		GetMonitorInfoA (USER32.@)
 */
BOOL WINAPI GetMonitorInfoA( HMONITOR monitor, LPMONITORINFO info )
{
    MONITORINFOEXW miW;
    BOOL ret;

    if (info->cbSize == sizeof(MONITORINFO)) return GetMonitorInfoW( monitor, info );
    if (info->cbSize != sizeof(MONITORINFOEXA)) return FALSE;

    miW.cbSize = sizeof(miW);
    ret = GetMonitorInfoW( monitor, (MONITORINFO *)&miW );
    if (ret)
    {
        MONITORINFOEXA *miA = (MONITORINFOEXA *)info;
        miA->rcMonitor = miW.rcMonitor;
        miA->rcWork = miW.rcWork;
        miA->dwFlags = miW.dwFlags;
        WideCharToMultiByte(CP_ACP, 0, miW.szDevice, -1, miA->szDevice, sizeof(miA->szDevice), NULL, NULL);
    }
    return ret;
}

/***********************************************************************
 *		GetMonitorInfoW (USER32.@)
 */
BOOL WINAPI GetMonitorInfoW( HMONITOR monitor, LPMONITORINFO info )
{
    return NtUserCallTwoParam( HandleToUlong(monitor), (ULONG_PTR)info, NtUserGetMonitorInfo );
}

#ifdef __i386__
/* Some apps pass a non-stdcall callback to EnumDisplayMonitors,
 * so we need a small assembly wrapper to call it.
 */
extern BOOL enum_mon_callback_wrapper( void *proc, HMONITOR monitor, HDC hdc, RECT *rect, LPARAM lparam );
__ASM_GLOBAL_FUNC( enum_mon_callback_wrapper,
    "pushl %ebp\n\t"
    __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
    __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
    "movl %esp,%ebp\n\t"
    __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
    "subl $8,%esp\n\t"
    "pushl 24(%ebp)\n\t"        /* lparam */
    /* MJ's Help Diagnostic expects that %ecx contains the address to the rect. */
    "movl 20(%ebp),%ecx\n\t"    /* rect */
    "pushl %ecx\n\t"
    "pushl 16(%ebp)\n\t"        /* hdc */
    "pushl 12(%ebp)\n\t"        /* monitor */
    "movl 8(%ebp),%eax\n"       /* proc */
    "call *%eax\n\t"
    "leave\n\t"
    __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
    __ASM_CFI(".cfi_same_value %ebp\n\t")
    "ret" )
#endif /* __i386__ */

BOOL WINAPI User32CallEnumDisplayMonitor( struct enum_display_monitor_params *params, ULONG size )
{
#ifdef __i386__
    return enum_mon_callback_wrapper( params->proc, params->monitor, params->hdc,
                                      &params->rect, params->lparam );
#else
    return params->proc( params->monitor, params->hdc, &params->rect, params->lparam );
#endif
}

/***********************************************************************
 *		EnumDisplayDevicesA (USER32.@)
 */
BOOL WINAPI EnumDisplayDevicesA( LPCSTR device, DWORD index, DISPLAY_DEVICEA *info, DWORD flags )
{
    UNICODE_STRING deviceW;
    DISPLAY_DEVICEW ddW;
    BOOL ret;

    if (device)
        RtlCreateUnicodeStringFromAsciiz( &deviceW, device );
    else
        deviceW.Buffer = NULL;

    ddW.cb = sizeof(ddW);
    ret = EnumDisplayDevicesW( deviceW.Buffer, index, &ddW, flags );
    RtlFreeUnicodeString( &deviceW );

    if (!ret)
        return ret;

    WideCharToMultiByte( CP_ACP, 0, ddW.DeviceName, -1, info->DeviceName, sizeof(info->DeviceName), NULL, NULL );
    WideCharToMultiByte( CP_ACP, 0, ddW.DeviceString, -1, info->DeviceString, sizeof(info->DeviceString), NULL, NULL );
    info->StateFlags = ddW.StateFlags;

    if (info->cb >= offsetof(DISPLAY_DEVICEA, DeviceID) + sizeof(info->DeviceID))
        WideCharToMultiByte( CP_ACP, 0, ddW.DeviceID, -1, info->DeviceID, sizeof(info->DeviceID), NULL, NULL );
    if (info->cb >= offsetof(DISPLAY_DEVICEA, DeviceKey) + sizeof(info->DeviceKey))
        WideCharToMultiByte( CP_ACP, 0, ddW.DeviceKey, -1, info->DeviceKey, sizeof(info->DeviceKey), NULL, NULL );

    return TRUE;
}

/***********************************************************************
 *		EnumDisplayDevicesW (USER32.@)
 */
BOOL WINAPI EnumDisplayDevicesW( LPCWSTR device, DWORD index, DISPLAY_DEVICEW *info, DWORD flags )
{
    UNICODE_STRING str;
    RtlInitUnicodeString( &str, device );
    return NT_SUCCESS(NtUserEnumDisplayDevices( &str, index, info, flags ));
}

/**********************************************************************
 *              GetAutoRotationState [USER32.@]
 */
BOOL WINAPI GetAutoRotationState( AR_STATE *state )
{
    TRACE("(%p)\n", state);

    if (!state)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *state = AR_NOSENSOR;
    return TRUE;
}

/**********************************************************************
 *              GetDisplayAutoRotationPreferences [USER32.@]
 */
BOOL WINAPI GetDisplayAutoRotationPreferences( ORIENTATION_PREFERENCE *orientation )
{
    FIXME("(%p): stub\n", orientation);
    *orientation = ORIENTATION_PREFERENCE_NONE;
    return TRUE;
}

/* physical<->logical mapping functions from win8 that are nops in later versions */

/***********************************************************************
 *              GetPhysicalCursorPos   (USER32.@)
 */
BOOL WINAPI GetPhysicalCursorPos( POINT *point )
{
    return GetCursorPos( point );
}

/***********************************************************************
 *              SetPhysicalCursorPos   (USER32.@)
 */
BOOL WINAPI SetPhysicalCursorPos( INT x, INT y )
{
    return NtUserSetCursorPos( x, y );
}

/***********************************************************************
 *		WindowFromPhysicalPoint (USER32.@)
 */
HWND WINAPI WindowFromPhysicalPoint( POINT pt )
{
    return WindowFromPoint( pt );
}

/***********************************************************************
 *		LogicalToPhysicalPoint (USER32.@)
 */
BOOL WINAPI LogicalToPhysicalPoint( HWND hwnd, POINT *point )
{
    return TRUE;
}

/***********************************************************************
 *		PhysicalToLogicalPoint (USER32.@)
 */
BOOL WINAPI PhysicalToLogicalPoint( HWND hwnd, POINT *point )
{
    return TRUE;
}

static DISPLAYCONFIG_ROTATION get_dc_rotation(const DEVMODEW *devmode)
{
    if (devmode->dmFields & DM_DISPLAYORIENTATION)
        return devmode->u1.s2.dmDisplayOrientation + 1;
    else
        return DISPLAYCONFIG_ROTATION_IDENTITY;
}

static DISPLAYCONFIG_SCANLINE_ORDERING get_dc_scanline_ordering(const DEVMODEW *devmode)
{
    if (!(devmode->dmFields & DM_DISPLAYFLAGS))
        return DISPLAYCONFIG_SCANLINE_ORDERING_UNSPECIFIED;
    else if (devmode->u2.dmDisplayFlags & DM_INTERLACED)
        return DISPLAYCONFIG_SCANLINE_ORDERING_INTERLACED;
    else
        return DISPLAYCONFIG_SCANLINE_ORDERING_PROGRESSIVE;
}

static DISPLAYCONFIG_PIXELFORMAT get_dc_pixelformat(DWORD dmBitsPerPel)
{
    if ((dmBitsPerPel == 8) || (dmBitsPerPel == 16) ||
        (dmBitsPerPel == 24) || (dmBitsPerPel == 32))
        return dmBitsPerPel / 8;
    else
        return DISPLAYCONFIG_PIXELFORMAT_NONGDI;
}

static void set_mode_target_info(DISPLAYCONFIG_MODE_INFO *info, const LUID *gpu_luid, UINT32 target_id,
                                 UINT32 flags, const DEVMODEW *devmode)
{
    DISPLAYCONFIG_TARGET_MODE *mode = &(info->u.targetMode);

    info->infoType = DISPLAYCONFIG_MODE_INFO_TYPE_TARGET;
    info->adapterId = *gpu_luid;
    info->id = target_id;

    /* FIXME: Populate pixelRate/hSyncFreq/totalSize with real data */
    mode->targetVideoSignalInfo.pixelRate = devmode->dmDisplayFrequency * devmode->dmPelsWidth * devmode->dmPelsHeight;
    mode->targetVideoSignalInfo.hSyncFreq.Numerator = devmode->dmDisplayFrequency * devmode->dmPelsWidth;
    mode->targetVideoSignalInfo.hSyncFreq.Denominator = 1;
    mode->targetVideoSignalInfo.vSyncFreq.Numerator = devmode->dmDisplayFrequency;
    mode->targetVideoSignalInfo.vSyncFreq.Denominator = 1;
    mode->targetVideoSignalInfo.activeSize.cx = devmode->dmPelsWidth;
    mode->targetVideoSignalInfo.activeSize.cy = devmode->dmPelsHeight;
    if (flags & QDC_DATABASE_CURRENT)
    {
        mode->targetVideoSignalInfo.totalSize.cx = 0;
        mode->targetVideoSignalInfo.totalSize.cy = 0;
    }
    else
    {
        mode->targetVideoSignalInfo.totalSize.cx = devmode->dmPelsWidth;
        mode->targetVideoSignalInfo.totalSize.cy = devmode->dmPelsHeight;
    }
    mode->targetVideoSignalInfo.u.videoStandard = D3DKMDT_VSS_OTHER;
    mode->targetVideoSignalInfo.scanLineOrdering = get_dc_scanline_ordering(devmode);
}

static void set_path_target_info(DISPLAYCONFIG_PATH_TARGET_INFO *info, const LUID *gpu_luid,
                                 UINT32 target_id, UINT32 mode_index, const DEVMODEW *devmode)
{
    info->adapterId = *gpu_luid;
    info->id = target_id;
    info->u.modeInfoIdx = mode_index;
    info->outputTechnology = DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DISPLAYPORT_EXTERNAL;
    info->rotation = get_dc_rotation(devmode);
    info->scaling = DISPLAYCONFIG_SCALING_IDENTITY;
    info->refreshRate.Numerator = devmode->dmDisplayFrequency;
    info->refreshRate.Denominator = 1;
    info->scanLineOrdering = get_dc_scanline_ordering(devmode);
    info->targetAvailable = TRUE;
    info->statusFlags = DISPLAYCONFIG_TARGET_IN_USE;
}

static void set_mode_source_info(DISPLAYCONFIG_MODE_INFO *info, const LUID *gpu_luid,
                                 UINT32 source_id, const DEVMODEW *devmode)
{
    DISPLAYCONFIG_SOURCE_MODE *mode = &(info->u.sourceMode);

    info->infoType = DISPLAYCONFIG_MODE_INFO_TYPE_SOURCE;
    info->adapterId = *gpu_luid;
    info->id = source_id;

    mode->width = devmode->dmPelsWidth;
    mode->height = devmode->dmPelsHeight;
    mode->pixelFormat = get_dc_pixelformat(devmode->dmBitsPerPel);
    if (devmode->dmFields & DM_POSITION)
    {
        mode->position = devmode->u1.s2.dmPosition;
    }
    else
    {
        mode->position.x = 0;
        mode->position.y = 0;
    }
}

static void set_path_source_info(DISPLAYCONFIG_PATH_SOURCE_INFO *info, const LUID *gpu_luid,
                                 UINT32 source_id, UINT32 mode_index)
{
    info->adapterId = *gpu_luid;
    info->id = source_id;
    info->u.modeInfoIdx = mode_index;
    info->statusFlags = DISPLAYCONFIG_SOURCE_IN_USE;
}

static BOOL source_mode_exists(const DISPLAYCONFIG_MODE_INFO *modeinfo, UINT32 num_modes,
                               UINT32 source_id, UINT32 *found_mode_index)
{
    UINT32 i;

    for (i = 0; i < num_modes; i++)
    {
        if (modeinfo[i].infoType == DISPLAYCONFIG_MODE_INFO_TYPE_SOURCE &&
            modeinfo[i].id == source_id)
        {
            *found_mode_index = i;
            return TRUE;
        }
    }
    return FALSE;
}

/***********************************************************************
 *              QueryDisplayConfig (USER32.@)
 */
LONG WINAPI QueryDisplayConfig(UINT32 flags, UINT32 *numpathelements, DISPLAYCONFIG_PATH_INFO *pathinfo,
                               UINT32 *numinfoelements, DISPLAYCONFIG_MODE_INFO *modeinfo,
                               DISPLAYCONFIG_TOPOLOGY_ID *topologyid)
{
    LONG adapter_index, ret;
    HANDLE mutex;
    HDEVINFO devinfo;
    SP_DEVINFO_DATA device_data = {sizeof(device_data)};
    DWORD monitor_index = 0, state_flags, type;
    UINT32 output_id, source_mode_index, path_index = 0, mode_index = 0;
    LUID gpu_luid;
    WCHAR device_name[CCHDEVICENAME];
    DEVMODEW devmode;

    FIXME("(%08x %p %p %p %p %p): semi-stub\n", flags, numpathelements, pathinfo, numinfoelements, modeinfo, topologyid);

    if (!numpathelements || !numinfoelements)
        return ERROR_INVALID_PARAMETER;

    if (!*numpathelements || !*numinfoelements)
        return ERROR_INVALID_PARAMETER;

    if (flags != QDC_ALL_PATHS &&
        flags != QDC_ONLY_ACTIVE_PATHS &&
        flags != QDC_DATABASE_CURRENT)
        return ERROR_INVALID_PARAMETER;

    if (((flags == QDC_DATABASE_CURRENT) && !topologyid) ||
        ((flags != QDC_DATABASE_CURRENT) && topologyid))
        return ERROR_INVALID_PARAMETER;

    if (flags != QDC_ONLY_ACTIVE_PATHS)
        FIXME("only returning active paths\n");

    if (topologyid)
    {
        FIXME("setting toplogyid to DISPLAYCONFIG_TOPOLOGY_INTERNAL\n");
        *topologyid = DISPLAYCONFIG_TOPOLOGY_INTERNAL;
    }

    wait_graphics_driver_ready();
    mutex = get_display_device_init_mutex();

    /* Iterate through "targets"/monitors.
     * Each target corresponds to a path, and each path corresponds to one or two unique modes.
     */
    devinfo = SetupDiGetClassDevsW(&GUID_DEVCLASS_MONITOR, L"DISPLAY", NULL, DIGCF_PRESENT);
    if (devinfo == INVALID_HANDLE_VALUE)
    {
        ret = ERROR_GEN_FAILURE;
        goto done;
    }

    ret = ERROR_GEN_FAILURE;
    while (SetupDiEnumDeviceInfo(devinfo, monitor_index++, &device_data))
    {
        /* Only count active monitors */
        if (!SetupDiGetDevicePropertyW(devinfo, &device_data, &WINE_DEVPROPKEY_MONITOR_STATEFLAGS,
                                       &type, (BYTE *)&state_flags, sizeof(state_flags), NULL, 0))
            goto done;
        if (!(state_flags & DISPLAY_DEVICE_ACTIVE))
            continue;

        if (!SetupDiGetDevicePropertyW(devinfo, &device_data, &DEVPROPKEY_MONITOR_GPU_LUID,
                                       &type, (BYTE *)&gpu_luid, sizeof(gpu_luid), NULL, 0))
            goto done;

        if (!SetupDiGetDevicePropertyW(devinfo, &device_data, &DEVPROPKEY_MONITOR_OUTPUT_ID,
                                       &type, (BYTE *)&output_id, sizeof(output_id), NULL, 0))
            goto done;

        if (!SetupDiGetDevicePropertyW(devinfo, &device_data, &WINE_DEVPROPKEY_MONITOR_ADAPTERNAME,
                                       &type, (BYTE *)device_name, sizeof(device_name), NULL, 0))
            goto done;

        memset(&devmode, 0, sizeof(devmode));
        devmode.dmSize = sizeof(devmode);
        if (!EnumDisplaySettingsW(device_name, ENUM_CURRENT_SETTINGS, &devmode))
            goto done;

        /* Extract the adapter index from device_name to use as the source ID */
        adapter_index = wcstol(device_name + lstrlenW(L"\\\\.\\DISPLAY"), NULL, 10);
        adapter_index--;

        if (path_index == *numpathelements || mode_index == *numinfoelements)
        {
            ret = ERROR_INSUFFICIENT_BUFFER;
            goto done;
        }

        pathinfo[path_index].flags = DISPLAYCONFIG_PATH_ACTIVE;
        set_mode_target_info(&modeinfo[mode_index], &gpu_luid, output_id, flags, &devmode);
        set_path_target_info(&(pathinfo[path_index].targetInfo), &gpu_luid, output_id, mode_index, &devmode);

        mode_index++;
        if (mode_index == *numinfoelements)
        {
            ret = ERROR_INSUFFICIENT_BUFFER;
            goto done;
        }

        /* Multiple targets can be driven by the same source, ensure a mode
         * hasn't already been added for this source.
         */
        if (!source_mode_exists(modeinfo, mode_index, adapter_index, &source_mode_index))
        {
            set_mode_source_info(&modeinfo[mode_index], &gpu_luid, adapter_index, &devmode);
            source_mode_index = mode_index;
            mode_index++;
        }
        set_path_source_info(&(pathinfo[path_index].sourceInfo), &gpu_luid, adapter_index, source_mode_index);
        path_index++;
    }

    *numpathelements = path_index;
    *numinfoelements = mode_index;
    ret = ERROR_SUCCESS;

done:
    SetupDiDestroyDeviceInfoList(devinfo);
    release_display_device_init_mutex(mutex);
    return ret;
}

/***********************************************************************
 *              DisplayConfigGetDeviceInfo (USER32.@)
 */
LONG WINAPI DisplayConfigGetDeviceInfo(DISPLAYCONFIG_DEVICE_INFO_HEADER *packet)
{
    LONG ret = ERROR_GEN_FAILURE;
    HANDLE mutex;
    HDEVINFO devinfo;
    SP_DEVINFO_DATA device_data = {sizeof(device_data)};
    DWORD index = 0, type;
    LUID gpu_luid;

    TRACE("(%p)\n", packet);

    if (!packet || packet->size < sizeof(*packet))
        return ERROR_GEN_FAILURE;
    wait_graphics_driver_ready();

    switch (packet->type)
    {
    case DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME:
    {
        DISPLAYCONFIG_SOURCE_DEVICE_NAME *source_name = (DISPLAYCONFIG_SOURCE_DEVICE_NAME *)packet;
        WCHAR device_name[CCHDEVICENAME];
        LONG source_id;

        TRACE("DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME\n");

        if (packet->size < sizeof(*source_name))
            return ERROR_INVALID_PARAMETER;

        mutex = get_display_device_init_mutex();
        devinfo = SetupDiGetClassDevsW(&GUID_DEVCLASS_MONITOR, L"DISPLAY", NULL, DIGCF_PRESENT);
        if (devinfo == INVALID_HANDLE_VALUE)
        {
            release_display_device_init_mutex(mutex);
            return ret;
        }

        while (SetupDiEnumDeviceInfo(devinfo, index++, &device_data))
        {
            if (!SetupDiGetDevicePropertyW(devinfo, &device_data, &DEVPROPKEY_MONITOR_GPU_LUID,
                                           &type, (BYTE *)&gpu_luid, sizeof(gpu_luid), NULL, 0))
                continue;

            if ((source_name->header.adapterId.LowPart != gpu_luid.LowPart) ||
                (source_name->header.adapterId.HighPart != gpu_luid.HighPart))
                continue;

            /* QueryDisplayConfig() derives the source ID from the adapter name. */
            if (!SetupDiGetDevicePropertyW(devinfo, &device_data, &WINE_DEVPROPKEY_MONITOR_ADAPTERNAME,
                                           &type, (BYTE *)device_name, sizeof(device_name), NULL, 0))
                continue;

            source_id = wcstol(device_name + lstrlenW(L"\\\\.\\DISPLAY"), NULL, 10);
            source_id--;
            if (source_name->header.id != source_id)
                continue;

            lstrcpyW(source_name->viewGdiDeviceName, device_name);
            ret = ERROR_SUCCESS;
            break;
        }
        SetupDiDestroyDeviceInfoList(devinfo);
        release_display_device_init_mutex(mutex);
        return ret;
    }
    case DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME:
    {
        DISPLAYCONFIG_TARGET_DEVICE_NAME *target_name = (DISPLAYCONFIG_TARGET_DEVICE_NAME *)packet;

        FIXME("DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME: stub\n");

        if (packet->size < sizeof(*target_name))
            return ERROR_INVALID_PARAMETER;

        return ERROR_NOT_SUPPORTED;
    }
    case DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_PREFERRED_MODE:
    {
        DISPLAYCONFIG_TARGET_PREFERRED_MODE *preferred_mode = (DISPLAYCONFIG_TARGET_PREFERRED_MODE *)packet;

        FIXME("DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_PREFERRED_MODE: stub\n");

        if (packet->size < sizeof(*preferred_mode))
            return ERROR_INVALID_PARAMETER;

        return ERROR_NOT_SUPPORTED;
    }
    case DISPLAYCONFIG_DEVICE_INFO_GET_ADAPTER_NAME:
    {
        DISPLAYCONFIG_ADAPTER_NAME *adapter_name = (DISPLAYCONFIG_ADAPTER_NAME *)packet;

        FIXME("DISPLAYCONFIG_DEVICE_INFO_GET_ADAPTER_NAME: stub\n");

        if (packet->size < sizeof(*adapter_name))
            return ERROR_INVALID_PARAMETER;

        return ERROR_NOT_SUPPORTED;
    }
    case DISPLAYCONFIG_DEVICE_INFO_SET_TARGET_PERSISTENCE:
    case DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_BASE_TYPE:
    case DISPLAYCONFIG_DEVICE_INFO_GET_SUPPORT_VIRTUAL_RESOLUTION:
    case DISPLAYCONFIG_DEVICE_INFO_SET_SUPPORT_VIRTUAL_RESOLUTION:
    case DISPLAYCONFIG_DEVICE_INFO_GET_ADVANCED_COLOR_INFO:
    case DISPLAYCONFIG_DEVICE_INFO_SET_ADVANCED_COLOR_STATE:
    case DISPLAYCONFIG_DEVICE_INFO_GET_SDR_WHITE_LEVEL:
    default:
        FIXME("Unimplemented packet type: %u\n", packet->type);
        return ERROR_INVALID_PARAMETER;
    }
}

/***********************************************************************
 *              SetDisplayConfig (USER32.@)
 */
LONG WINAPI SetDisplayConfig(UINT32 path_info_count, DISPLAYCONFIG_PATH_INFO *path_info, UINT32 mode_info_count,
        DISPLAYCONFIG_MODE_INFO *mode_info, UINT32 flags)
{
    FIXME("path_info_count %u, path_info %p, mode_info_count %u, mode_info %p, flags %#x stub.\n",
            path_info_count, path_info, mode_info_count, mode_info, flags);

    return ERROR_SUCCESS;
}
