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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "user_private.h"
#include "controls.h"
#include "wine/asm.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(system);


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
    return NtUserGetSystemMetrics( index );
}


/***********************************************************************
 *		GetSystemMetricsForDpi (USER32.@)
 */
INT WINAPI GetSystemMetricsForDpi( INT index, UINT dpi )
{
    return NtUserGetSystemMetricsForDpi( index, dpi );
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
    return NtUserGetSysColor( index );
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
    return NtUserGetSysColorBrush( index );
}


/***********************************************************************
 *		SYSCOLOR_GetPen
 */
HPEN SYSCOLOR_GetPen( INT index )
{
    return NtUserGetSysColorPen( index );
}


/***********************************************************************
 *		SYSCOLOR_Get55AABrush
 */
HBRUSH SYSCOLOR_Get55AABrush(void)
{
    return NtUserGetSysColorBrush( COLOR_55AA_BRUSH );
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
        lpDevMode->dmDisplayFlags  = devmodeW.dmDisplayFlags;
        lpDevMode->dmDisplayFrequency = devmodeW.dmDisplayFrequency;
        lpDevMode->dmFields           = devmodeW.dmFields;

        lpDevMode->dmPosition.x = devmodeW.dmPosition.x;
        lpDevMode->dmPosition.y = devmodeW.dmPosition.y;
        lpDevMode->dmDisplayOrientation = devmodeW.dmDisplayOrientation;
        lpDevMode->dmDisplayFixedOutput = devmodeW.dmDisplayFixedOutput;
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
    struct ntuser_thread_info *info = NtUserGetThreadInfo();

    if (info->dpi_awareness) return ULongToHandle( info->dpi_awareness );
    return UlongToHandle( (NtUserGetProcessDpiAwarenessContext( GetCurrentProcess() ) & 3 ) | 0x10 );
}

/**********************************************************************
 *              SetThreadDpiAwarenessContext   (USER32.@)
 */
DPI_AWARENESS_CONTEXT WINAPI SetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT context )
{
    struct ntuser_thread_info *info = NtUserGetThreadInfo();
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
 *              GetThreadDpiHostingBehavior   (USER32.@)
 */
DPI_HOSTING_BEHAVIOR WINAPI GetThreadDpiHostingBehavior( void )
{
    FIXME("(): stub\n");
    return DPI_HOSTING_BEHAVIOR_DEFAULT;
}

/**********************************************************************
 *              SetThreadDpiHostingBehavior   (USER32.@)
 */
DPI_HOSTING_BEHAVIOR WINAPI SetThreadDpiHostingBehavior( DPI_HOSTING_BEHAVIOR value )
{
    FIXME("(%d): stub\n", value);
    return DPI_HOSTING_BEHAVIOR_DEFAULT;
}

/***********************************************************************
 *		MonitorFromRect (USER32.@)
 */
HMONITOR WINAPI MonitorFromRect( const RECT *rect, DWORD flags )
{
    return NtUserMonitorFromRect( rect, flags );
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
    return NtUserMonitorFromWindow( hwnd, flags );
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
    return NtUserGetMonitorInfo( monitor, info );
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

NTSTATUS WINAPI User32CallEnumDisplayMonitor( void *args, ULONG size )
{
    struct enum_display_monitor_params *params = args;
    BOOL ret;
#ifdef __i386__
    ret = enum_mon_callback_wrapper( params->proc, params->monitor, params->hdc,
                                     &params->rect, params->lparam );
#else
    ret = params->proc( params->monitor, params->hdc, &params->rect, params->lparam );
#endif
    return NtCallbackReturn( &ret, sizeof(ret), STATUS_SUCCESS );
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
 *              GetDisplayAutoRotationPreferences (USER32.@)
 */
BOOL WINAPI GetDisplayAutoRotationPreferences( ORIENTATION_PREFERENCE *orientation )
{
    FIXME("(%p): stub\n", orientation);
    *orientation = ORIENTATION_PREFERENCE_NONE;
    return TRUE;
}

/**********************************************************************
 *              SetDisplayAutoRotationPreferences (USER32.@)
 */
BOOL WINAPI SetDisplayAutoRotationPreferences( ORIENTATION_PREFERENCE orientation )
{
    FIXME("(%d): stub\n", orientation);
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

/***********************************************************************
 *              DisplayConfigGetDeviceInfo (USER32.@)
 */
LONG WINAPI DisplayConfigGetDeviceInfo(DISPLAYCONFIG_DEVICE_INFO_HEADER *packet)
{
    return RtlNtStatusToDosError(NtUserDisplayConfigGetDeviceInfo(packet));
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
