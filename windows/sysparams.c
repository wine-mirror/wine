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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wingdi.h"
#include "winreg.h"
#include "wine/winuser16.h"
#include "winerror.h"

#include "controls.h"
#include "user.h"
#include "wine/debug.h"
#include "sysmetrics.h"

WINE_DEFAULT_DEBUG_CHANNEL(system);

/* System parameter indexes */
#define SPI_SETBEEP_IDX                         0
#define SPI_SETMOUSE_IDX                        1
#define SPI_SETBORDER_IDX                       2
#define SPI_SETKEYBOARDSPEED_IDX                3
#define SPI_ICONHORIZONTALSPACING_IDX           4
#define SPI_SETSCREENSAVETIMEOUT_IDX            5
#define SPI_SETGRIDGRANULARITY_IDX              6
#define SPI_SETKEYBOARDDELAY_IDX                7
#define SPI_ICONVERTICALSPACING_IDX             8
#define SPI_SETICONTITLEWRAP_IDX                9
#define SPI_SETMENUDROPALIGNMENT_IDX            10
#define SPI_SETDOUBLECLKWIDTH_IDX               11
#define SPI_SETDOUBLECLKHEIGHT_IDX              12
#define SPI_SETDOUBLECLICKTIME_IDX              13
#define SPI_SETMOUSEBUTTONSWAP_IDX              14
#define SPI_SETDRAGFULLWINDOWS_IDX              15
#define SPI_SETWORKAREA_IDX                     16
#define SPI_SETSHOWSOUNDS_IDX                   17
#define SPI_SETKEYBOARDPREF_IDX                 18
#define SPI_SETSCREENREADER_IDX                 19
#define SPI_SETSCREENSAVERRUNNING_IDX           20
#define SPI_WINE_IDX                            SPI_SETSCREENSAVERRUNNING_IDX

/**
 * Names of the registry subkeys of HKEY_CURRENT_USER key and value names
 * for the system parameters.
 * Names of the keys are created by adding string "_REGKEY" to
 * "SET" action names, value names are created by adding "_REG_NAME"
 * to the "SET" action name.
 */
#define SPI_SETBEEP_REGKEY              "Control Panel\\Sound"
#define SPI_SETBEEP_VALNAME             "Beep"
#define SPI_SETMOUSE_REGKEY             "Control Panel\\Mouse"
#define SPI_SETMOUSE_VALNAME1           "MouseThreshold1"
#define SPI_SETMOUSE_VALNAME2           "MouseThreshold2"
#define SPI_SETMOUSE_VALNAME3           "MouseSpeed"
#define SPI_SETBORDER_REGKEY            "Control Panel\\Desktop"
#define SPI_SETBORDER_VALNAME           "BorderWidth"
#define SPI_SETKEYBOARDSPEED_REGKEY             "Control Panel\\Keyboard"
#define SPI_SETKEYBOARDSPEED_VALNAME            "KeyboardSpeed"
#define SPI_ICONHORIZONTALSPACING_REGKEY        "Control Panel\\Desktop"
#define SPI_ICONHORIZONTALSPACING_VALNAME       "IconSpacing"
#define SPI_SETSCREENSAVETIMEOUT_REGKEY         "Control Panel\\Desktop"
#define SPI_SETSCREENSAVETIMEOUT_VALNAME        "ScreenSaveTimeOut"
#define SPI_SETSCREENSAVEACTIVE_REGKEY          "Control Panel\\Desktop"
#define SPI_SETSCREENSAVEACTIVE_VALNAME         "ScreenSaveActive"
#define SPI_SETGRIDGRANULARITY_REGKEY           "Control Panel\\Desktop"
#define SPI_SETGRIDGRANULARITY_VALNAME          "GridGranularity"
#define SPI_SETKEYBOARDDELAY_REGKEY             "Control Panel\\Keyboard"
#define SPI_SETKEYBOARDDELAY_VALNAME            "KeyboardDelay"
#define SPI_ICONVERTICALSPACING_REGKEY          "Control Panel\\Desktop"
#define SPI_ICONVERTICALSPACING_VALNAME         "IconVerticalSpacing"
#define SPI_SETICONTITLEWRAP_REGKEY             "Control Panel\\Desktop"
#define SPI_SETICONTITLEWRAP_VALNAME            "IconTitleWrap"
#define SPI_SETMENUDROPALIGNMENT_REGKEY         "Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows"
#define SPI_SETMENUDROPALIGNMENT_VALNAME        "MenuDropAlignment"
#define SPI_SETDOUBLECLKWIDTH_REGKEY            "Control Panel\\Mouse"
#define SPI_SETDOUBLECLKWIDTH_VALNAME           "DoubleClickWidth"
#define SPI_SETDOUBLECLKHEIGHT_REGKEY           "Control Panel\\Mouse"
#define SPI_SETDOUBLECLKHEIGHT_VALNAME          "DoubleClickHeight"
#define SPI_SETDOUBLECLICKTIME_REGKEY           "Control Panel\\Mouse"
#define SPI_SETDOUBLECLICKTIME_VALNAME          "DoubleClickSpeed"
#define SPI_SETMOUSEBUTTONSWAP_REGKEY           "Control Panel\\Mouse"
#define SPI_SETMOUSEBUTTONSWAP_VALNAME          "SwapMouseButtons"
#define SPI_SETDRAGFULLWINDOWS_REGKEY           "Control Panel\\Desktop"
#define SPI_SETDRAGFULLWINDOWS_VALNAME          "DragFullWindows"
#define SPI_SETWORKAREA_REGKEY                  "Control Panel\\Desktop"
#define SPI_SETWORKAREA_VALNAME                 "WINE_WorkArea"
#define SPI_SETSHOWSOUNDS_REGKEY        "Control Panel\\Accessibility\\ShowSounds"
#define SPI_SETSHOWSOUNDS_VALNAME       "On"
#define SPI_SETDESKWALLPAPER_REGKEY	"Control Panel\\Desktop"
#define SPI_SETDESKWALLPAPER_VALNAME	"Wallpaper"
/* FIXME - real values */
#define SPI_SETKEYBOARDPREF_REGKEY      "Control Panel\\Desktop"
#define SPI_SETKEYBOARDPREF_VALNAME     "WINE_KeyboardPref"
#define SPI_SETSCREENREADER_REGKEY      "Control Panel\\Desktop"
#define SPI_SETSCREENREADER_VALNAME     "WINE_ScreenReader"
#define SPI_SETSCREENSAVERRUNNING_REGKEY        "Control Panel\\Desktop"
#define SPI_SETSCREENSAVERRUNNING_VALNAME       "WINE_ScreenSaverRunning"

/* volatile registry branch under CURRENT_USER_REGKEY for temporary values storage */
#define WINE_CURRENT_USER_REGKEY     "Wine"

/* Indicators whether system parameter value is loaded */
static char spi_loaded[SPI_WINE_IDX + 1];

static BOOL notify_change = TRUE;

/* System parameters storage */
static BOOL beep_active = TRUE;
static int mouse_threshold1 = 6;
static int mouse_threshold2 = 10;
static int mouse_speed = 1;
static int border = 1;
static int keyboard_speed = 31;
static int screensave_timeout = 300;
static int grid_granularity = 0;
static int keyboard_delay = 1;
static BOOL icon_title_wrap = TRUE;
static int double_click_time = 500;
static BOOL drag_full_windows = FALSE;
static RECT work_area;
static BOOL keyboard_pref = TRUE;
static BOOL screen_reader = FALSE;
static BOOL screensaver_running = FALSE;

/***********************************************************************
 *		GetTimerResolution (USER.14)
 */
LONG WINAPI GetTimerResolution16(void)
{
	return (1000);
}

/***********************************************************************
 *		ControlPanelInfo (USER.273)
 */
void WINAPI ControlPanelInfo16( INT16 nInfoType, WORD wData, LPSTR lpBuffer )
{
	FIXME("(%d, %04x, %p): stub.\n", nInfoType, wData, lpBuffer);
}

/* This function is a copy of the one in objects/font.c */
static void SYSPARAMS_LogFont32ATo16( const LOGFONTA* font32, LPLOGFONT16 font16 )
{
    font16->lfHeight = font32->lfHeight;
    font16->lfWidth = font32->lfWidth;
    font16->lfEscapement = font32->lfEscapement;
    font16->lfOrientation = font32->lfOrientation;
    font16->lfWeight = font32->lfWeight;
    font16->lfItalic = font32->lfItalic;
    font16->lfUnderline = font32->lfUnderline;
    font16->lfStrikeOut = font32->lfStrikeOut;
    font16->lfCharSet = font32->lfCharSet;
    font16->lfOutPrecision = font32->lfOutPrecision;
    font16->lfClipPrecision = font32->lfClipPrecision;
    font16->lfQuality = font32->lfQuality;
    font16->lfPitchAndFamily = font32->lfPitchAndFamily;
    lstrcpynA( font16->lfFaceName, font32->lfFaceName, LF_FACESIZE );
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

static void SYSPARAMS_NonClientMetrics32ATo16( const NONCLIENTMETRICSA* lpnm32, LPNONCLIENTMETRICS16 lpnm16 )
{
    lpnm16->iBorderWidth	= lpnm32->iBorderWidth;
    lpnm16->iScrollWidth	= lpnm32->iScrollWidth;
    lpnm16->iScrollHeight	= lpnm32->iScrollHeight;
    lpnm16->iCaptionWidth	= lpnm32->iCaptionWidth;
    lpnm16->iCaptionHeight	= lpnm32->iCaptionHeight;
    SYSPARAMS_LogFont32ATo16( &lpnm32->lfCaptionFont,	&lpnm16->lfCaptionFont );
    lpnm16->iSmCaptionWidth	= lpnm32->iSmCaptionWidth;
    lpnm16->iSmCaptionHeight	= lpnm32->iSmCaptionHeight;
    SYSPARAMS_LogFont32ATo16( &lpnm32->lfSmCaptionFont,	&lpnm16->lfSmCaptionFont );
    lpnm16->iMenuWidth		= lpnm32->iMenuWidth;
    lpnm16->iMenuHeight		= lpnm32->iMenuHeight;
    SYSPARAMS_LogFont32ATo16( &lpnm32->lfMenuFont,	&lpnm16->lfMenuFont );
    SYSPARAMS_LogFont32ATo16( &lpnm32->lfStatusFont,	&lpnm16->lfStatusFont );
    SYSPARAMS_LogFont32ATo16( &lpnm32->lfMessageFont,	&lpnm16->lfMessageFont );
}

static void SYSPARAMS_NonClientMetrics32ATo32W( const NONCLIENTMETRICSA* lpnm32A, LPNONCLIENTMETRICSW lpnm32W )
{
    lpnm32W->iBorderWidth	= lpnm32A->iBorderWidth;
    lpnm32W->iScrollWidth	= lpnm32A->iScrollWidth;
    lpnm32W->iScrollHeight	= lpnm32A->iScrollHeight;
    lpnm32W->iCaptionWidth	= lpnm32A->iCaptionWidth;
    lpnm32W->iCaptionHeight	= lpnm32A->iCaptionHeight;
    SYSPARAMS_LogFont32ATo32W( &lpnm32A->lfCaptionFont,		&lpnm32W->lfCaptionFont );
    lpnm32W->iSmCaptionWidth	= lpnm32A->iSmCaptionWidth;
    lpnm32W->iSmCaptionHeight	= lpnm32A->iSmCaptionHeight;
    SYSPARAMS_LogFont32ATo32W( &lpnm32A->lfSmCaptionFont,	&lpnm32W->lfSmCaptionFont );
    lpnm32W->iMenuWidth		= lpnm32A->iMenuWidth;
    lpnm32W->iMenuHeight	= lpnm32A->iMenuHeight;
    SYSPARAMS_LogFont32ATo32W( &lpnm32A->lfMenuFont,		&lpnm32W->lfMenuFont );
    SYSPARAMS_LogFont32ATo32W( &lpnm32A->lfStatusFont,		&lpnm32W->lfStatusFont );
    SYSPARAMS_LogFont32ATo32W( &lpnm32A->lfMessageFont,		&lpnm32W->lfMessageFont );
}

/***********************************************************************
 *           SYSPARAMS_Reset
 *
 * Sets the system parameter which should be always loaded to
 * current value stored in registry.
 * Invalidates lazy loaded parameter, so it will be loaded the next time
 * it is requested.
 *
 * Parameters:
 * uiAction - system parameter to reload value for.
 *      Note, only "SET" values can be used for this parameter.
 *      If uiAction is 0 all system parameters are reset.
 */
void SYSPARAMS_Reset( UINT uiAction )
{
#define WINE_RELOAD_SPI(x) \
    case x: \
        spi_loaded[x##_IDX] = FALSE; \
        SystemParametersInfoA( x, 0, dummy_buf, 0 );\
        if (uiAction) \
            break

#define WINE_IGNORE_SPI(x) \
    case x: \
        if (uiAction) \
            break

#define WINE_INVALIDATE_SPI(x) \
    case x: \
        spi_loaded[x##_IDX] = FALSE; \
        break

    BOOL not_all_processed = TRUE;
    char dummy_buf[10];

    /* Execution falls through all the branches for uiAction == 0 */
    switch (uiAction)
    {
    case 0:
        memset( spi_loaded, 0, sizeof(spi_loaded) );

    WINE_RELOAD_SPI(SPI_SETBORDER);
    WINE_RELOAD_SPI(SPI_ICONHORIZONTALSPACING);
    WINE_RELOAD_SPI(SPI_ICONVERTICALSPACING);
    WINE_IGNORE_SPI(SPI_SETSCREENSAVEACTIVE);
    WINE_RELOAD_SPI(SPI_SETDOUBLECLKWIDTH);
    WINE_RELOAD_SPI(SPI_SETDOUBLECLKHEIGHT);
    WINE_RELOAD_SPI(SPI_SETMOUSEBUTTONSWAP);
    WINE_RELOAD_SPI(SPI_SETSHOWSOUNDS);
    WINE_RELOAD_SPI(SPI_SETMENUDROPALIGNMENT);

    default:
        if (uiAction)
        {
            /* lazy loaded parameters */
            switch (uiAction)
            {
            WINE_INVALIDATE_SPI(SPI_SETBEEP);
            WINE_INVALIDATE_SPI(SPI_SETMOUSE);
            WINE_INVALIDATE_SPI(SPI_SETKEYBOARDSPEED);
            WINE_INVALIDATE_SPI(SPI_SETSCREENSAVETIMEOUT);
            WINE_INVALIDATE_SPI(SPI_SETGRIDGRANULARITY);
            WINE_INVALIDATE_SPI(SPI_SETKEYBOARDDELAY);
            WINE_INVALIDATE_SPI(SPI_SETICONTITLEWRAP);
            WINE_INVALIDATE_SPI(SPI_SETDOUBLECLICKTIME);
            WINE_INVALIDATE_SPI(SPI_SETDRAGFULLWINDOWS);
            WINE_INVALIDATE_SPI(SPI_SETWORKAREA);
            WINE_INVALIDATE_SPI(SPI_SETKEYBOARDPREF);
            WINE_INVALIDATE_SPI(SPI_SETSCREENREADER);
            WINE_INVALIDATE_SPI(SPI_SETSCREENSAVERRUNNING);
            default:
                FIXME( "Unknown action reset: %u\n", uiAction );
                break;
            }
        }
        else
            not_all_processed = FALSE;
	break;
    }

    if (!uiAction && not_all_processed)
        ERR( "Incorrect implementation of SYSPARAMS_Reset. "
             "Not all params are reloaded.\n" );
#undef WINE_INVALIDATE_SPI
#undef WINE_IGNORE_SPI
#undef WINE_RELOAD_SPI
}

/***********************************************************************
 *           get_volatile_regkey
 *
 * Return a handle to the volatile registry key used to store
 * non-permanently modified parameters.
 */
static HKEY get_volatile_regkey(void)
{
    static HKEY volatile_key;

    if (!volatile_key)
    {
        if (RegCreateKeyExA( HKEY_CURRENT_USER, WINE_CURRENT_USER_REGKEY,
                             0, 0, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, 0,
                             &volatile_key, 0 ) != ERROR_SUCCESS)
            ERR("Can't create wine configuration registry branch\n");
    }
    return volatile_key;
}

/***********************************************************************
 *           SYSPARAMS_NotifyChange
 *
 * Sends notification about system parameter update.
 */
void SYSPARAMS_NotifyChange( UINT uiAction, UINT fWinIni )
{
    if (notify_change)
    {
        if (fWinIni & SPIF_UPDATEINIFILE)
        {
            if (fWinIni & (SPIF_SENDWININICHANGE | SPIF_SENDCHANGE))
                SendMessageA(HWND_BROADCAST, WM_SETTINGCHANGE,
                             uiAction, (LPARAM) "");
        }
        else
        {
            /* FIXME notify other wine processes with internal message */
        }
    }
}


/***********************************************************************
 * Loads system parameter from user profile.
 */
BOOL SYSPARAMS_Load( LPSTR lpRegKey, LPSTR lpValName, LPSTR lpBuf )
{
    BOOL ret = FALSE;
    DWORD type;
    HKEY hKey;
    DWORD count;

    if ((RegOpenKeyA( get_volatile_regkey(), lpRegKey,
                     &hKey ) == ERROR_SUCCESS) ||
        (RegOpenKeyA( HKEY_CURRENT_USER, lpRegKey,
                      &hKey ) == ERROR_SUCCESS))
    {
        ret = !RegQueryValueExA( hKey, lpValName, NULL, &type, lpBuf, &count );
        RegCloseKey( hKey );
    }
    return ret;
}

/***********************************************************************
 * Saves system parameter to user profile.
 */
BOOL SYSPARAMS_Save( LPSTR lpRegKey, LPSTR lpValName, LPSTR lpValue,
                     UINT fWinIni )
{
    HKEY hKey;
    HKEY hBaseKey;
    DWORD dwOptions;
    BOOL ret = FALSE;

    if (fWinIni & SPIF_UPDATEINIFILE)
    {
        hBaseKey = HKEY_CURRENT_USER;
        dwOptions = 0;
    }
    else
    {
        hBaseKey = get_volatile_regkey();
        dwOptions = REG_OPTION_VOLATILE;
    }

    if (RegCreateKeyExA( hBaseKey, lpRegKey,
                         0, 0, dwOptions, KEY_ALL_ACCESS,
                         0, &hKey, 0 ) == ERROR_SUCCESS)
    {
        if (RegSetValueExA( hKey, lpValName, 0, REG_SZ,
                            lpValue, strlen(lpValue) + 1 ) == ERROR_SUCCESS)
        {
            ret = TRUE;
            if (hBaseKey == HKEY_CURRENT_USER)
                RegDeleteKeyA( get_volatile_regkey(), lpRegKey );
        }
        RegCloseKey( hKey );
    }
    return ret;
}


/***********************************************************************
 *           SYSPARAMS_GetDoubleClickSize
 *
 * There is no SPI_GETDOUBLECLK* so we export this function instead.
 */
void SYSPARAMS_GetDoubleClickSize( INT *width, INT *height )
{
    char buf[10];

    if (!spi_loaded[SPI_SETDOUBLECLKWIDTH_IDX])
    {
        char buf[10];

        if (SYSPARAMS_Load( SPI_SETDOUBLECLKWIDTH_REGKEY,
                            SPI_SETDOUBLECLKWIDTH_VALNAME, buf ))
        {
            SYSMETRICS_Set( SM_CXDOUBLECLK, atoi( buf ) );
        }
        spi_loaded[SPI_SETDOUBLECLKWIDTH_IDX] = TRUE;
    }
    if (!spi_loaded[SPI_SETDOUBLECLKHEIGHT_IDX])
    {
        if (SYSPARAMS_Load( SPI_SETDOUBLECLKHEIGHT_REGKEY,
                            SPI_SETDOUBLECLKHEIGHT_VALNAME, buf ))
        {
            SYSMETRICS_Set( SM_CYDOUBLECLK, atoi( buf ) );
        }
        spi_loaded[SPI_SETDOUBLECLKHEIGHT_IDX] = TRUE;
    }
    *width  = GetSystemMetrics( SM_CXDOUBLECLK );
    *height = GetSystemMetrics( SM_CYDOUBLECLK );
}


/***********************************************************************
 *           SYSPARAMS_GetMouseButtonSwap
 *
 * There is no SPI_GETMOUSEBUTTONSWAP so we export this function instead.
 */
INT SYSPARAMS_GetMouseButtonSwap( void )
{
        int spi_idx = SPI_SETMOUSEBUTTONSWAP_IDX;

        if (!spi_loaded[spi_idx])
        {
            char buf[5];

            if (SYSPARAMS_Load( SPI_SETMOUSEBUTTONSWAP_REGKEY,
                                SPI_SETMOUSEBUTTONSWAP_VALNAME, buf ))
            {
                SYSMETRICS_Set( SM_SWAPBUTTON, atoi( buf ) );
            }
            spi_loaded[spi_idx] = TRUE;
        }

        return GetSystemMetrics( SM_SWAPBUTTON );
}

/***********************************************************************
 *
 *	SYSPARAMS_GetGUIFont
 *
 *  fills LOGFONT with 'default GUI font'.
 */

static void SYSPARAMS_GetGUIFont( LOGFONTA* plf )
{
	HFONT	hf;

	memset( plf, 0, sizeof(LOGFONTA) );
	hf = (HFONT)GetStockObject( DEFAULT_GUI_FONT );
	if ( GetObjectA( hf, sizeof(LOGFONTA), plf ) != sizeof(LOGFONTA) )
	{
		/*
		 * GetObjectA() would be succeeded always
		 * since this is a stock object
		 */
		ERR("GetObjectA() failed\n");
	}
}

/***********************************************************************
 *		SystemParametersInfoA (USER32.@)
 *
 *     Each system parameter has flag which shows whether the parameter
 * is loaded or not. Parameters, stored directly in SysParametersInfo are
 * loaded from registry only when they are requested and the flag is
 * "false", after the loading the flag is set to "true". On interprocess
 * notification of the parameter change the corresponding parameter flag is
 * set to "false". The parameter value will be reloaded when it is requested
 * the next time.
 *     Parameters, backed by or depend on GetSystemMetrics are processed
 * differently. These parameters are always loaded. They are reloaded right
 * away on interprocess change notification. We can't do lazy loading because
 * we don't want to complicate GetSystemMetrics.
 *     Parameters, backed by X settings are read from corresponding setting.
 * On the parameter change request the setting is changed. Interprocess change
 * notifications are ignored.
 *     When parameter value is updated the changed value is stored in permanent
 * registry branch if saving is requested. Otherwise it is stored
 * in temporary branch
 *
 */
BOOL WINAPI SystemParametersInfoA( UINT uiAction, UINT uiParam,
				   PVOID pvParam, UINT fWinIni )
{
#define WINE_SPI_FIXME(x) \
    case x: \
        FIXME( "Unimplemented action: %u (%s)\n", x, #x ); \
        SetLastError( ERROR_INVALID_SPI_VALUE ); \
        ret = FALSE; \
        break
#define WINE_SPI_WARN(x) \
    case x: \
        WARN( "Ignored action: %u (%s)\n", x, #x ); \
        break

    BOOL ret = TRUE;
    unsigned spi_idx = 0;

    TRACE("(%u, %u, %p, %u)\n", uiAction, uiParam, pvParam, fWinIni);

    switch (uiAction)
    {
    case SPI_GETBEEP:				/*      1 */
        spi_idx = SPI_SETBEEP_IDX;
        if (!spi_loaded[spi_idx])
        {
            char buf[5];

            if (SYSPARAMS_Load( SPI_SETBEEP_REGKEY, SPI_SETBEEP_VALNAME, buf ))
                beep_active  = !strcasecmp( "Yes", buf );
            spi_loaded[spi_idx] = TRUE;
        }

	*(BOOL *)pvParam = beep_active;
        break;

    case SPI_SETBEEP:				/*      2 */
        spi_idx = SPI_SETBEEP_IDX;
        if (SYSPARAMS_Save( SPI_SETBEEP_REGKEY, SPI_SETBEEP_VALNAME,
                            (uiParam ? "Yes" : "No"), fWinIni ))
        {
            beep_active = uiParam;
            spi_loaded[spi_idx] = TRUE;
        }
        else
            ret = FALSE;
        break;

    case SPI_GETMOUSE:                          /*      3 */
        spi_idx = SPI_SETMOUSE_IDX;
        if (!spi_loaded[spi_idx])
        {
            char buf[10];

            if (SYSPARAMS_Load( SPI_SETMOUSE_REGKEY, SPI_SETMOUSE_VALNAME1,
                                buf ))
                mouse_threshold1 = atoi( buf );
            if (SYSPARAMS_Load( SPI_SETMOUSE_REGKEY, SPI_SETMOUSE_VALNAME2,
                                buf ))
                mouse_threshold2 = atoi( buf );
            if (SYSPARAMS_Load( SPI_SETMOUSE_REGKEY, SPI_SETMOUSE_VALNAME3,
                                buf ))
                mouse_speed = atoi( buf );
            spi_loaded[spi_idx] = TRUE;
        }
        ((INT *)pvParam)[0] = mouse_threshold1;
        ((INT *)pvParam)[1] = mouse_threshold2;
        ((INT *)pvParam)[2] = mouse_speed;
        break;

    case SPI_SETMOUSE:                          /*      4 */
    {
        char buf[10];

        spi_idx = SPI_SETMOUSE_IDX;
        sprintf(buf, "%d", ((INT *)pvParam)[0]);

        if (SYSPARAMS_Save( SPI_SETMOUSE_REGKEY, SPI_SETMOUSE_VALNAME1,
                            buf, fWinIni ))
        {
            mouse_threshold1 = ((INT *)pvParam)[0];
            spi_loaded[spi_idx] = TRUE;

            sprintf(buf, "%d", ((INT *)pvParam)[1]);
            SYSPARAMS_Save( SPI_SETMOUSE_REGKEY, SPI_SETMOUSE_VALNAME2,
                            buf, fWinIni );
            mouse_threshold2 = ((INT *)pvParam)[1];

            sprintf(buf, "%d", ((INT *)pvParam)[2]);
            SYSPARAMS_Save( SPI_SETMOUSE_REGKEY, SPI_SETMOUSE_VALNAME3,
                            buf, fWinIni );
            mouse_speed = ((INT *)pvParam)[2];
        }
        else
            ret = FALSE;
        break;
    }

    case SPI_GETBORDER:				/*      5 */
        spi_idx = SPI_SETBORDER_IDX;
        if (!spi_loaded[spi_idx])
        {
            char buf[10];

            if (SYSPARAMS_Load( SPI_SETBORDER_REGKEY, SPI_SETBORDER_VALNAME,
                                buf ))
            {
                int i = atoi( buf );
                if (i > 0) border = i;
            }
            spi_loaded[spi_idx] = TRUE;
            if (TWEAK_WineLook > WIN31_LOOK)
            {
                SYSMETRICS_Set( SM_CXFRAME, border + GetSystemMetrics( SM_CXDLGFRAME ) );
                SYSMETRICS_Set( SM_CYFRAME, border + GetSystemMetrics( SM_CXDLGFRAME ) );
            }
        }
	*(INT *)pvParam = border;
	break;

    case SPI_SETBORDER:                         /*      6 */
    {
        char buf[10];

        spi_idx = SPI_SETBORDER_IDX;
        sprintf(buf, "%u", uiParam);

        if (SYSPARAMS_Save( SPI_SETBORDER_REGKEY, SPI_SETBORDER_VALNAME,
                            buf, fWinIni ))
        {
            if (uiParam > 0)
            {
                border = uiParam;
                spi_loaded[spi_idx] = TRUE;
                if (TWEAK_WineLook > WIN31_LOOK)
                {
                    SYSMETRICS_Set( SM_CXFRAME, uiParam + GetSystemMetrics( SM_CXDLGFRAME ) );
                    SYSMETRICS_Set( SM_CYFRAME, uiParam + GetSystemMetrics( SM_CXDLGFRAME ) );
                }
            }
        }
        else
            ret = FALSE;
        break;
    }

    case SPI_GETKEYBOARDSPEED:                  /*     10 */
        spi_idx = SPI_SETKEYBOARDSPEED_IDX;
        if (!spi_loaded[spi_idx])
        {
            char buf[10];

            if (SYSPARAMS_Load( SPI_SETKEYBOARDSPEED_REGKEY,
                                SPI_SETKEYBOARDSPEED_VALNAME,
                                buf ))
                keyboard_speed = atoi( buf );
            spi_loaded[spi_idx] = TRUE;
        }
	*(INT *)pvParam = keyboard_speed;
	break;

    case SPI_SETKEYBOARDSPEED:                  /*     11 */
    {
        char buf[10];

        spi_idx = SPI_SETKEYBOARDSPEED_IDX;
        if (uiParam > 31)
            uiParam = 31;
        sprintf(buf, "%u", uiParam);

        if (SYSPARAMS_Save( SPI_SETKEYBOARDSPEED_REGKEY,
                            SPI_SETKEYBOARDSPEED_VALNAME,
                            buf, fWinIni ))
        {
            keyboard_speed = uiParam;
            spi_loaded[spi_idx] = TRUE;
        }
        else
            ret = FALSE;
        break;
    }

    /* not implemented in Windows */
    WINE_SPI_WARN(SPI_LANGDRIVER);              /*     12 */

    case SPI_ICONHORIZONTALSPACING:             /*     13 */
        spi_idx = SPI_ICONHORIZONTALSPACING_IDX;
	if (pvParam != NULL)
        {
            if (!spi_loaded[spi_idx])
            {
                char buf[10];

                if (SYSPARAMS_Load( SPI_ICONHORIZONTALSPACING_REGKEY,
                                    SPI_ICONHORIZONTALSPACING_VALNAME, buf ))
                {
                    SYSMETRICS_Set( SM_CXICONSPACING, atoi( buf ) );
                }
                spi_loaded[spi_idx] = TRUE;
            }

            *(INT *)pvParam = GetSystemMetrics( SM_CXICONSPACING );
        }
	else
        {
            char buf[10];

            if (uiParam < 32) uiParam = 32;

            sprintf(buf, "%u", uiParam);
            if (SYSPARAMS_Save( SPI_ICONHORIZONTALSPACING_REGKEY,
                                SPI_ICONHORIZONTALSPACING_VALNAME,
                                buf, fWinIni ))
            {
                SYSMETRICS_Set( SM_CXICONSPACING, uiParam );
                spi_loaded[spi_idx] = TRUE;
            }
            else
                ret = FALSE;
        }
	break;

    case SPI_GETSCREENSAVETIMEOUT:              /*     14 */
        spi_idx = SPI_SETSCREENSAVETIMEOUT_IDX;
        if (!spi_loaded[spi_idx])
        {
            char buf[10];

            if (SYSPARAMS_Load( SPI_SETSCREENSAVETIMEOUT_REGKEY,
                                SPI_SETSCREENSAVETIMEOUT_VALNAME,
                                buf ))
                screensave_timeout = atoi( buf );

            spi_loaded[spi_idx] = TRUE;
        }
	*(INT *)pvParam = screensave_timeout;
	break;

    case SPI_SETSCREENSAVETIMEOUT:              /*     15 */
    {
        char buf[10];

        spi_idx = SPI_SETSCREENSAVETIMEOUT_IDX;
        sprintf(buf, "%u", uiParam);

        if (SYSPARAMS_Save( SPI_SETSCREENSAVETIMEOUT_REGKEY,
                            SPI_SETSCREENSAVETIMEOUT_VALNAME,
                            buf, fWinIni ))
        {
            screensave_timeout = uiParam;
            spi_loaded[spi_idx] = TRUE;
        }
        else
            ret = FALSE;
        break;
    }

    case SPI_GETSCREENSAVEACTIVE:               /*     16 */
	*(BOOL *)pvParam = USER_Driver.pGetScreenSaveActive();
        break;

    case SPI_SETSCREENSAVEACTIVE:               /*     17 */
    {
        char buf[5];

        sprintf(buf, "%u", uiParam);
        USER_Driver.pSetScreenSaveActive( uiParam );
        /* saved value does not affect Wine */
        SYSPARAMS_Save( SPI_SETSCREENSAVEACTIVE_REGKEY,
                        SPI_SETSCREENSAVEACTIVE_VALNAME,
                        buf, fWinIni );
        break;
    }

    case SPI_GETGRIDGRANULARITY:                /*     18 */
        spi_idx = SPI_SETGRIDGRANULARITY_IDX;
        if (!spi_loaded[spi_idx])
        {
            char buf[10];

            if (SYSPARAMS_Load( SPI_SETGRIDGRANULARITY_REGKEY,
                                SPI_SETGRIDGRANULARITY_VALNAME,
                                buf ))
                grid_granularity = atoi( buf );

            spi_loaded[spi_idx] = TRUE;
        }
	*(INT *)pvParam = grid_granularity;
	break;

    case SPI_SETGRIDGRANULARITY:                /*     19 */
    {
        char buf[10];

        spi_idx = SPI_SETGRIDGRANULARITY_IDX;
        sprintf(buf, "%u", uiParam);

        if (SYSPARAMS_Save( SPI_SETGRIDGRANULARITY_REGKEY,
                            SPI_SETGRIDGRANULARITY_VALNAME,
                            buf, fWinIni ))
        {
            grid_granularity = uiParam;
            spi_loaded[spi_idx] = TRUE;
        }
        else
            ret = FALSE;
        break;
    }

    case SPI_SETDESKWALLPAPER:			/*     20 */
        SYSPARAMS_Save(SPI_SETDESKWALLPAPER_REGKEY, SPI_SETDESKWALLPAPER_VALNAME, pvParam, fWinIni);
	ret = SetDeskWallPaper( (LPSTR)pvParam );
	break;
    case SPI_SETDESKPATTERN:			/*     21 */
	/* FIXME: the ability to specify a pattern in pvParam
	   doesn't seem to be documented for Win32 */
	if ((INT16)uiParam == -1)
	{
	    char buffer[256];
	    GetProfileStringA( "Desktop", "Pattern",
			       "170 85 170 85 170 85 170 85",
			       buffer, sizeof(buffer) );
	    ret = DESKTOP_SetPattern( (LPSTR)buffer );
	} else
	    ret = DESKTOP_SetPattern( (LPSTR)pvParam );
	break;

    case SPI_GETKEYBOARDDELAY:                  /*     22 */
        spi_idx = SPI_SETKEYBOARDDELAY_IDX;
        if (!spi_loaded[spi_idx])
        {
            char buf[10];

            if (SYSPARAMS_Load( SPI_SETKEYBOARDDELAY_REGKEY,
                                SPI_SETKEYBOARDDELAY_VALNAME,
                                buf ))
            {
                int i = atoi( buf );
                if ( (i >= 0) && (i <= 3)) keyboard_delay = i;
            }

            spi_loaded[spi_idx] = TRUE;
        }
	*(INT *)pvParam = keyboard_delay;
	break;

    case SPI_SETKEYBOARDDELAY:                  /*     23 */
    {
        char buf[10];

        spi_idx = SPI_SETKEYBOARDDELAY_IDX;
        sprintf(buf, "%u", uiParam);

        if (SYSPARAMS_Save( SPI_SETKEYBOARDDELAY_REGKEY,
                            SPI_SETKEYBOARDDELAY_VALNAME,
                            buf, fWinIni ))
        {
            if (uiParam <= 3)
                keyboard_delay = uiParam;
            spi_loaded[spi_idx] = TRUE;
        }
        else
            ret = FALSE;
        break;
    }

    case SPI_ICONVERTICALSPACING:		/*     24 */
        spi_idx = SPI_ICONVERTICALSPACING_IDX;
	if (pvParam != NULL)
        {
            if (!spi_loaded[spi_idx])
            {
                char buf[10];

                if (SYSPARAMS_Load( SPI_ICONVERTICALSPACING_REGKEY,
                                    SPI_ICONVERTICALSPACING_VALNAME, buf ))
                {
                    SYSMETRICS_Set( SM_CYICONSPACING, atoi( buf ) );
                }
                spi_loaded[spi_idx] = TRUE;
            }

            *(INT *)pvParam = GetSystemMetrics( SM_CYICONSPACING );
        }
	else
        {
            char buf[10];

            if (uiParam < 32) uiParam = 32;

            sprintf(buf, "%u", uiParam);
            if (SYSPARAMS_Save( SPI_ICONVERTICALSPACING_REGKEY,
                                SPI_ICONVERTICALSPACING_VALNAME,
                                buf, fWinIni ))
            {
                SYSMETRICS_Set( SM_CYICONSPACING, uiParam );
                spi_loaded[spi_idx] = TRUE;
            }
            else
                ret = FALSE;
        }

	break;

    case SPI_GETICONTITLEWRAP:                  /*     25 */
        spi_idx = SPI_SETICONTITLEWRAP_IDX;
        if (!spi_loaded[spi_idx])
        {
            char buf[5];

            if (SYSPARAMS_Load( SPI_SETICONTITLEWRAP_REGKEY,
                                SPI_SETICONTITLEWRAP_VALNAME, buf ))
                icon_title_wrap  = atoi(buf);
            spi_loaded[spi_idx] = TRUE;
        }

	*(BOOL *)pvParam = icon_title_wrap;
        break;

    case SPI_SETICONTITLEWRAP:                  /*     26 */
    {
        char buf[5];

        spi_idx = SPI_SETICONTITLEWRAP_IDX;
        sprintf(buf, "%u", uiParam);
        if (SYSPARAMS_Save( SPI_SETICONTITLEWRAP_REGKEY,
                            SPI_SETICONTITLEWRAP_VALNAME,
                            buf, fWinIni ))
        {
            icon_title_wrap = uiParam;
            spi_loaded[spi_idx] = TRUE;
        }
        else
            ret = FALSE;
        break;
    }

    case SPI_GETMENUDROPALIGNMENT:              /*     27 */
        spi_idx = SPI_SETMENUDROPALIGNMENT_IDX;

        if (!spi_loaded[spi_idx])
        {
            char buf[5];

            if (SYSPARAMS_Load( SPI_SETMENUDROPALIGNMENT_REGKEY,
                                SPI_SETMENUDROPALIGNMENT_VALNAME, buf ))
            {
                SYSMETRICS_Set( SM_MENUDROPALIGNMENT, atoi( buf ) );
            }
            spi_loaded[spi_idx] = TRUE;
        }


        *(BOOL *)pvParam = GetSystemMetrics( SM_MENUDROPALIGNMENT );
        break;

    case SPI_SETMENUDROPALIGNMENT:              /*     28 */
    {
        char buf[5];
        spi_idx = SPI_SETMENUDROPALIGNMENT_IDX;

        sprintf(buf, "%u", uiParam);
        if (SYSPARAMS_Save( SPI_SETMENUDROPALIGNMENT_REGKEY,
                            SPI_SETMENUDROPALIGNMENT_VALNAME,
                            buf, fWinIni ))
        {
            SYSMETRICS_Set( SM_MENUDROPALIGNMENT, uiParam );
            spi_loaded[spi_idx] = TRUE;
        }
        else
            ret = FALSE;
        break;
    }

    case SPI_SETDOUBLECLKWIDTH:                 /*     29 */
    {
        char buf[10];
        spi_idx = SPI_SETDOUBLECLKWIDTH_IDX;

        sprintf(buf, "%u", uiParam);
        if (SYSPARAMS_Save( SPI_SETDOUBLECLKWIDTH_REGKEY,
                            SPI_SETDOUBLECLKWIDTH_VALNAME,
                            buf, fWinIni ))
        {
            SYSMETRICS_Set( SM_CXDOUBLECLK, uiParam );
            spi_loaded[spi_idx] = TRUE;
        }
        else
            ret = FALSE;
        break;
    }

    case SPI_SETDOUBLECLKHEIGHT:                /*     30 */
    {
        char buf[10];
        spi_idx = SPI_SETDOUBLECLKHEIGHT_IDX;

        sprintf(buf, "%u", uiParam);
        if (SYSPARAMS_Save( SPI_SETDOUBLECLKHEIGHT_REGKEY,
                            SPI_SETDOUBLECLKHEIGHT_VALNAME,
                            buf, fWinIni ))
        {
            SYSMETRICS_Set( SM_CYDOUBLECLK, uiParam );
            spi_loaded[spi_idx] = TRUE;
        }
        else
            ret = FALSE;
        break;
    }

    case SPI_GETICONTITLELOGFONT:		/*     31 */
    {
	LPLOGFONTA lpLogFont = (LPLOGFONTA)pvParam;
	LOGFONTA	lfDefault;

	/*
	 * The 'default GDI fonts' seems to be returned.
	 * If a returned font is not a correct font in your environment,
	 * please try to fix objects/gdiobj.c at first.
	 */
	SYSPARAMS_GetGUIFont( &lfDefault );

	GetProfileStringA( "Desktop", "IconTitleFaceName",
			   lfDefault.lfFaceName,
			   lpLogFont->lfFaceName, LF_FACESIZE );
	lpLogFont->lfHeight = -GetProfileIntA( "Desktop", "IconTitleSize", 11 );
	lpLogFont->lfWidth = 0;
	lpLogFont->lfEscapement = lpLogFont->lfOrientation = 0;
	lpLogFont->lfWeight = FW_NORMAL;
	lpLogFont->lfItalic = FALSE;
	lpLogFont->lfStrikeOut = FALSE;
	lpLogFont->lfUnderline = FALSE;
	lpLogFont->lfCharSet = lfDefault.lfCharSet; /* at least 'charset' should not be hard-coded */
	lpLogFont->lfOutPrecision = OUT_DEFAULT_PRECIS;
	lpLogFont->lfClipPrecision = CLIP_DEFAULT_PRECIS;
	lpLogFont->lfPitchAndFamily = DEFAULT_PITCH;
	break;
    }

    case SPI_SETDOUBLECLICKTIME:                /*     32 */
    {
        char buf[10];

        spi_idx = SPI_SETDOUBLECLICKTIME_IDX;
        sprintf(buf, "%u", uiParam);

        if (SYSPARAMS_Save( SPI_SETDOUBLECLICKTIME_REGKEY,
                            SPI_SETDOUBLECLICKTIME_VALNAME,
                            buf, fWinIni ))
        {
            if (!uiParam)
                uiParam = 500;
            double_click_time = uiParam;
            spi_loaded[spi_idx] = TRUE;
        }
        else
            ret = FALSE;
        break;
    }

    case SPI_SETMOUSEBUTTONSWAP:                /*     33 */
    {
        char buf[5];
        spi_idx = SPI_SETMOUSEBUTTONSWAP_IDX;

        sprintf(buf, "%u", uiParam);
        if (SYSPARAMS_Save( SPI_SETMOUSEBUTTONSWAP_REGKEY,
                            SPI_SETMOUSEBUTTONSWAP_VALNAME,
                            buf, fWinIni ))
        {
            SYSMETRICS_Set( SM_SWAPBUTTON, uiParam );
            spi_loaded[spi_idx] = TRUE;
        }
        else
            ret = FALSE;
        break;
    }

    WINE_SPI_FIXME(SPI_SETICONTITLELOGFONT);	/*     34 */

    case SPI_GETFASTTASKSWITCH:			/*     35 */
	*(BOOL *)pvParam = 1;
        break;

    case SPI_SETFASTTASKSWITCH:                 /*     36 */
        /* the action is disabled */
        ret = FALSE;
        break;

    case SPI_SETDRAGFULLWINDOWS:                /*     37  WINVER >= 0x0400 */
    {
        char buf[5];

        spi_idx = SPI_SETDRAGFULLWINDOWS_IDX;
        sprintf(buf, "%u", uiParam);
        if (SYSPARAMS_Save( SPI_SETDRAGFULLWINDOWS_REGKEY,
                            SPI_SETDRAGFULLWINDOWS_VALNAME,
                            buf, fWinIni ))
        {
            drag_full_windows = uiParam;
            spi_loaded[spi_idx] = TRUE;
        }
        else
            ret = FALSE;
        break;
    }

    case SPI_GETDRAGFULLWINDOWS:                /*     38  WINVER >= 0x0400 */
        spi_idx = SPI_SETDRAGFULLWINDOWS_IDX;
        if (!spi_loaded[spi_idx])
        {
            char buf[5];

            if (SYSPARAMS_Load( SPI_SETDRAGFULLWINDOWS_REGKEY,
                                SPI_SETDRAGFULLWINDOWS_VALNAME, buf ))
                drag_full_windows  = atoi(buf);
            spi_loaded[spi_idx] = TRUE;
        }

	*(BOOL *)pvParam = drag_full_windows;
        break;

    case SPI_GETNONCLIENTMETRICS: 		/*     41  WINVER >= 0x400 */
    {
	LPNONCLIENTMETRICSA lpnm = (LPNONCLIENTMETRICSA)pvParam;

	if (lpnm->cbSize == sizeof(NONCLIENTMETRICSA))
	{
	    /* clear the struct, so we have 'sane' members */
	    memset(
		(char *)pvParam + sizeof(lpnm->cbSize),
		0,
		lpnm->cbSize - sizeof(lpnm->cbSize)
		);

	    /* initialize geometry entries */
	    lpnm->iBorderWidth = 1;
	    lpnm->iScrollWidth = GetSystemMetrics(SM_CXVSCROLL);
	    lpnm->iScrollHeight = GetSystemMetrics(SM_CYHSCROLL);

	    /* size of the normal caption buttons */
	    lpnm->iCaptionWidth = GetSystemMetrics(SM_CXSIZE);
	    lpnm->iCaptionHeight = GetSystemMetrics(SM_CYSIZE);

	    /* caption font metrics */
	    SystemParametersInfoA( SPI_GETICONTITLELOGFONT, 0, (LPVOID)&(lpnm->lfCaptionFont), 0 );
	    lpnm->lfCaptionFont.lfWeight = FW_BOLD;

	    /* size of the small caption buttons */
	    lpnm->iSmCaptionWidth = GetSystemMetrics(SM_CXSMSIZE);
	    lpnm->iSmCaptionHeight = GetSystemMetrics(SM_CYSMSIZE);

	    /* small caption font metrics */
	    SystemParametersInfoA( SPI_GETICONTITLELOGFONT, 0, (LPVOID)&(lpnm->lfSmCaptionFont), 0 );

	    /* menus, FIXME: names of wine.conf entries are bogus */

	    /* size of the menu (MDI) buttons */
	    lpnm->iMenuWidth = GetSystemMetrics(SM_CXMENUSIZE);
	    lpnm->iMenuHeight = GetSystemMetrics(SM_CYMENUSIZE);

	    /* menu font metrics */
	    SystemParametersInfoA( SPI_GETICONTITLELOGFONT, 0, (LPVOID)&(lpnm->lfMenuFont), 0 );
	    GetProfileStringA( "Desktop", "MenuFont",
			       (TWEAK_WineLook > WIN31_LOOK) ? lpnm->lfCaptionFont.lfFaceName : "System",
			       lpnm->lfMenuFont.lfFaceName, LF_FACESIZE );
	    lpnm->lfMenuFont.lfHeight = -GetProfileIntA( "Desktop", "MenuFontSize", 11 );
	    lpnm->lfMenuFont.lfWeight = (TWEAK_WineLook > WIN31_LOOK) ? FW_NORMAL : FW_BOLD;

	    /* status bar font metrics */
	    SystemParametersInfoA( SPI_GETICONTITLELOGFONT, 0,
				   (LPVOID)&(lpnm->lfStatusFont), 0 );
	    /* message font metrics */
	    SystemParametersInfoA( SPI_GETICONTITLELOGFONT, 0,
				   (LPVOID)&(lpnm->lfMessageFont), 0 );
	}
	else
	{
	    WARN("size mismatch !! (is %d; should be %d)\n", lpnm->cbSize, sizeof(NONCLIENTMETRICSA));
	    /* FIXME: SetLastError? */
	    ret = FALSE;
	}
	break;
    }
    WINE_SPI_FIXME(SPI_SETNONCLIENTMETRICS);	/*     42  WINVER >= 0x400 */

    WINE_SPI_FIXME(SPI_GETMINIMIZEDMETRICS);	/*     43  WINVER >= 0x400 */
    WINE_SPI_FIXME(SPI_SETMINIMIZEDMETRICS);	/*     44  WINVER >= 0x400 */

    case SPI_GETICONMETRICS:			/*     45  WINVER >= 0x400 */
    {
	LPICONMETRICSA lpIcon = pvParam;
	if(lpIcon && lpIcon->cbSize == sizeof(*lpIcon))
	{
	    SystemParametersInfoA( SPI_ICONHORIZONTALSPACING, 0,
				   &lpIcon->iHorzSpacing, FALSE );
	    SystemParametersInfoA( SPI_ICONVERTICALSPACING, 0,
				   &lpIcon->iVertSpacing, FALSE );
	    SystemParametersInfoA( SPI_GETICONTITLEWRAP, 0,
				   &lpIcon->iTitleWrap, FALSE );
	    SystemParametersInfoA( SPI_GETICONTITLELOGFONT, 0,
				   &lpIcon->lfFont, FALSE );
	}
	else
	{
	    ret = FALSE;
	}
	break;
    }
    WINE_SPI_FIXME(SPI_SETICONMETRICS);		/*     46  WINVER >= 0x400 */

    case SPI_SETWORKAREA:                       /*     47  WINVER >= 0x400 */
    {
        char buf[20];
        RECT *pr = (RECT *) pvParam;

        spi_idx = SPI_SETWORKAREA_IDX;
        sprintf(buf, "%d %d %d %d",
                pr->left, pr->top,
                pr->right, pr->bottom );

        if (SYSPARAMS_Save( SPI_SETWORKAREA_REGKEY,
                            SPI_SETWORKAREA_VALNAME,
                            buf, fWinIni ))
        {
            CopyRect( &work_area, (RECT *)pvParam );
            spi_loaded[spi_idx] = TRUE;
        }
        else
            ret = FALSE;
        break;
    }

    case SPI_GETWORKAREA:                       /*     48  WINVER >= 0x400 */
      spi_idx = SPI_SETWORKAREA_IDX;
      if (!spi_loaded[spi_idx])
      {
          char buf[20];

          SetRect( &work_area, 0, 0,
                   GetSystemMetrics( SM_CXSCREEN ),
                   GetSystemMetrics( SM_CYSCREEN ) );

          if (SYSPARAMS_Load( SPI_SETWORKAREA_REGKEY,
                              SPI_SETWORKAREA_VALNAME,
                              buf ))
          {
              sscanf( buf, "%d %d %d %d",
                      &work_area.left, &work_area.top,
                      &work_area.right, &work_area.bottom );
          }
          spi_loaded[spi_idx] = TRUE;
      }
      CopyRect( (RECT *)pvParam, &work_area );

      break;

    WINE_SPI_FIXME(SPI_SETPENWINDOWS);		/*     49  WINVER >= 0x400 */

    case SPI_GETFILTERKEYS:                     /*     50 */
    {
        LPFILTERKEYS lpFilterKeys = (LPFILTERKEYS)pvParam;
        WARN("SPI_GETFILTERKEYS not fully implemented\n");
        if (lpFilterKeys->cbSize == sizeof(FILTERKEYS))
        {
            /* Indicate that no FilterKeys feature available */
            lpFilterKeys->dwFlags = 0;
            lpFilterKeys->iWaitMSec = 0;
            lpFilterKeys->iDelayMSec = 0;
            lpFilterKeys->iRepeatMSec = 0;
            lpFilterKeys->iBounceMSec = 0;
         }
        else
        {
            ret = FALSE;
        }
        break;
    }
    WINE_SPI_FIXME(SPI_SETFILTERKEYS);		/*     51 */

    case SPI_GETTOGGLEKEYS:                     /*     52 */
    {
        LPTOGGLEKEYS lpToggleKeys = (LPTOGGLEKEYS)pvParam;
        WARN("SPI_GETTOGGLEKEYS not fully implemented\n");
        if (lpToggleKeys->cbSize == sizeof(TOGGLEKEYS))
        {
            /* Indicate that no ToggleKeys feature available */
            lpToggleKeys->dwFlags = 0;
        }
        else
        {
            ret = FALSE;
        }
        break;
    }
    WINE_SPI_FIXME(SPI_SETTOGGLEKEYS);		/*     53 */

    case SPI_GETMOUSEKEYS:                      /*     54 */
    {
        LPMOUSEKEYS lpMouseKeys = (LPMOUSEKEYS)pvParam;
        WARN("SPI_GETMOUSEKEYS not fully implemented\n");
        if (lpMouseKeys->cbSize == sizeof(MOUSEKEYS))
        {
            /* Indicate that no MouseKeys feature available */
            lpMouseKeys->dwFlags = 0;
            lpMouseKeys->iMaxSpeed = 360;
            lpMouseKeys->iTimeToMaxSpeed = 1000;
            lpMouseKeys->iCtrlSpeed = 0;
            lpMouseKeys->dwReserved1 = 0;
            lpMouseKeys->dwReserved2 = 0;
        }
        else
        {
            ret = FALSE;
        }
        break;
    }
    WINE_SPI_FIXME(SPI_SETMOUSEKEYS);		/*     55 */

    case SPI_GETSHOWSOUNDS:			/*     56 */
        spi_idx = SPI_SETSHOWSOUNDS_IDX;

        if (!spi_loaded[spi_idx])
        {
            char buf[10];

            if (SYSPARAMS_Load( SPI_SETSHOWSOUNDS_REGKEY,
                                SPI_SETSHOWSOUNDS_VALNAME, buf ))
            {
                SYSMETRICS_Set( SM_SHOWSOUNDS, atoi( buf ) );
            }
            spi_loaded[spi_idx] = TRUE;
        }


        *(INT *)pvParam = GetSystemMetrics( SM_SHOWSOUNDS );
        break;

    case SPI_SETSHOWSOUNDS:			/*     57 */
    {
        char buf[10];
        spi_idx = SPI_SETSHOWSOUNDS_IDX;

        sprintf(buf, "%u", uiParam);
        if (SYSPARAMS_Save( SPI_SETSHOWSOUNDS_REGKEY,
                            SPI_SETSHOWSOUNDS_VALNAME,
                            buf, fWinIni ))
        {
            SYSMETRICS_Set( SM_SHOWSOUNDS, uiParam );
            spi_loaded[spi_idx] = TRUE;
        }
        else
            ret = FALSE;
        break;
    }

    case SPI_GETSTICKYKEYS:                     /*     58 */
    {
        LPSTICKYKEYS lpStickyKeys = (LPSTICKYKEYS)pvParam;
        WARN("SPI_GETSTICKYKEYS not fully implemented\n");
        if (lpStickyKeys->cbSize == sizeof(STICKYKEYS))
        {
            /* Indicate that no StickyKeys feature available */
            lpStickyKeys->dwFlags = 0;
        }
        else
        {
            ret = FALSE;
        }
        break;
    }
    WINE_SPI_FIXME(SPI_SETSTICKYKEYS);		/*     59 */

    case SPI_GETACCESSTIMEOUT:                  /*     60 */
    {
        LPACCESSTIMEOUT lpAccessTimeout = (LPACCESSTIMEOUT)pvParam;
        WARN("SPI_GETACCESSTIMEOUT not fully implemented\n");
        if (lpAccessTimeout->cbSize == sizeof(ACCESSTIMEOUT))
        {
            /* Indicate that no accessibility features timeout is available */
            lpAccessTimeout->dwFlags = 0;
            lpAccessTimeout->iTimeOutMSec = 0;
        }
        else
        {
            ret = FALSE;
        }
        break;
    }
    WINE_SPI_FIXME(SPI_SETACCESSTIMEOUT);	/*     61 */

    case SPI_GETSERIALKEYS:                     /*     62  WINVER >= 0x400 */
    {
        LPSERIALKEYSA lpSerialKeysA = (LPSERIALKEYSA)pvParam;
        WARN("SPI_GETSERIALKEYS not fully implemented\n");
        if (lpSerialKeysA->cbSize == sizeof(SERIALKEYSA))
        {
            /* Indicate that no SerialKeys feature available */
            lpSerialKeysA->dwFlags = 0;
            lpSerialKeysA->lpszActivePort = NULL;
            lpSerialKeysA->lpszPort = NULL;
            lpSerialKeysA->iBaudRate = 0;
            lpSerialKeysA->iPortState = 0;
        }
        else
        {
            ret = FALSE;
        }
        break;
    }
    WINE_SPI_FIXME(SPI_SETSERIALKEYS);		/*     63  WINVER >= 0x400 */

    case SPI_GETSOUNDSENTRY:                    /*     64 */
    {
        LPSOUNDSENTRYA lpSoundSentryA = (LPSOUNDSENTRYA)pvParam;
        WARN("SPI_GETSOUNDSENTRY not fully implemented\n");
        if (lpSoundSentryA->cbSize == sizeof(SOUNDSENTRYA))
        {
            /* Indicate that no SoundSentry feature available */
            lpSoundSentryA->dwFlags = 0;
            lpSoundSentryA->iFSTextEffect = 0;
            lpSoundSentryA->iFSTextEffectMSec = 0;
            lpSoundSentryA->iFSTextEffectColorBits = 0;
            lpSoundSentryA->iFSGrafEffect = 0;
            lpSoundSentryA->iFSGrafEffectMSec = 0;
            lpSoundSentryA->iFSGrafEffectColor = 0;
            lpSoundSentryA->iWindowsEffect = 0;
            lpSoundSentryA->iWindowsEffectMSec = 0;
            lpSoundSentryA->lpszWindowsEffectDLL = 0;
            lpSoundSentryA->iWindowsEffectOrdinal = 0;
        }
        else
        {
            ret = FALSE;
        }
        break;
    }
    WINE_SPI_FIXME(SPI_SETSOUNDSENTRY);		/*     65 */

    case SPI_GETHIGHCONTRAST:			/*     66  WINVER >= 0x400 */
    {
	LPHIGHCONTRASTA lpHighContrastA = (LPHIGHCONTRASTA)pvParam;
	WARN("SPI_GETHIGHCONTRAST not fully implemented\n");
	if (lpHighContrastA->cbSize == sizeof(HIGHCONTRASTA))
	{
	    /* Indicate that no high contrast feature available */
	    lpHighContrastA->dwFlags = 0;
	    lpHighContrastA->lpszDefaultScheme = NULL;
	}
	else
	{
	    ret = FALSE;
	}
	break;
    }
    WINE_SPI_FIXME(SPI_SETHIGHCONTRAST);	/*     67  WINVER >= 0x400 */

    case SPI_GETKEYBOARDPREF:                   /*     68  WINVER >= 0x400 */
        spi_idx = SPI_SETKEYBOARDPREF_IDX;
        if (!spi_loaded[spi_idx])
        {
            char buf[5];

            if (SYSPARAMS_Load( SPI_SETKEYBOARDPREF_REGKEY,
                                SPI_SETKEYBOARDPREF_VALNAME, buf ))
                keyboard_pref  = atoi(buf);
            spi_loaded[spi_idx] = TRUE;
        }

	*(BOOL *)pvParam = keyboard_pref;
        break;

    case SPI_SETKEYBOARDPREF:                   /*     69  WINVER >= 0x400 */
    {
        char buf[5];

        spi_idx = SPI_SETKEYBOARDPREF_IDX;
        sprintf(buf, "%u", uiParam);
        if (SYSPARAMS_Save( SPI_SETKEYBOARDPREF_REGKEY,
                            SPI_SETKEYBOARDPREF_VALNAME,
                            buf, fWinIni ))
        {
            keyboard_pref = uiParam;
            spi_loaded[spi_idx] = TRUE;
        }
        else
            ret = FALSE;
        break;
    }

    case SPI_GETSCREENREADER:                   /*     70  WINVER >= 0x400 */
        spi_idx = SPI_SETSCREENREADER_IDX;
        if (!spi_loaded[spi_idx])
        {
            char buf[5];

            if (SYSPARAMS_Load( SPI_SETSCREENREADER_REGKEY,
                                SPI_SETSCREENREADER_VALNAME, buf ))
                screen_reader  = atoi(buf);
            spi_loaded[spi_idx] = TRUE;
        }

	*(BOOL *)pvParam = screen_reader;
        break;

    case SPI_SETSCREENREADER:                   /*     71  WINVER >= 0x400 */
    {
        char buf[5];

        spi_idx = SPI_SETSCREENREADER_IDX;
        sprintf(buf, "%u", uiParam);
        if (SYSPARAMS_Save( SPI_SETSCREENREADER_REGKEY,
                            SPI_SETSCREENREADER_VALNAME,
                            buf, fWinIni ))
        {
            screen_reader = uiParam;
            spi_loaded[spi_idx] = TRUE;
        }
        else
            ret = FALSE;
        break;
    }

    case SPI_GETANIMATION:			/*     72  WINVER >= 0x400 */
    {
	LPANIMATIONINFO lpAnimInfo = (LPANIMATIONINFO)pvParam;

	/* Tell it "disabled" */
	if (lpAnimInfo->cbSize == sizeof(ANIMATIONINFO))
	    lpAnimInfo->iMinAnimate = 0; /* Minimise and restore animation is disabled (nonzero == enabled) */
	else
	    ret = FALSE;
	break;
    }
    WINE_SPI_WARN(SPI_SETANIMATION);		/*     73  WINVER >= 0x400 */

    WINE_SPI_FIXME(SPI_GETFONTSMOOTHING);	/*     74  WINVER >= 0x400 */
    WINE_SPI_FIXME(SPI_SETFONTSMOOTHING);	/*     75  WINVER >= 0x400 */

    WINE_SPI_FIXME(SPI_SETDRAGWIDTH);		/*     76  WINVER >= 0x400 */
    WINE_SPI_FIXME(SPI_SETDRAGHEIGHT);		/*     77  WINVER >= 0x400 */

    WINE_SPI_FIXME(SPI_SETHANDHELD);		/*     78  WINVER >= 0x400 */

    WINE_SPI_FIXME(SPI_GETLOWPOWERTIMEOUT);	/*     79  WINVER >= 0x400 */
    WINE_SPI_FIXME(SPI_GETPOWEROFFTIMEOUT);	/*     80  WINVER >= 0x400 */
    WINE_SPI_FIXME(SPI_SETLOWPOWERTIMEOUT);	/*     81  WINVER >= 0x400 */
    WINE_SPI_FIXME(SPI_SETPOWEROFFTIMEOUT);	/*     82  WINVER >= 0x400 */
    WINE_SPI_FIXME(SPI_GETLOWPOWERACTIVE);	/*     83  WINVER >= 0x400 */
    WINE_SPI_FIXME(SPI_GETPOWEROFFACTIVE);	/*     84  WINVER >= 0x400 */
    WINE_SPI_FIXME(SPI_SETLOWPOWERACTIVE);	/*     85  WINVER >= 0x400 */
    WINE_SPI_FIXME(SPI_SETPOWEROFFACTIVE);	/*     86  WINVER >= 0x400 */

    WINE_SPI_FIXME(SPI_SETCURSORS);		/*     87  WINVER >= 0x400 */
    WINE_SPI_FIXME(SPI_SETICONS);		/*     88  WINVER >= 0x400 */

    case SPI_GETDEFAULTINPUTLANG: 	/*     89  WINVER >= 0x400 */
        ret = GetKeyboardLayout(0) ? TRUE : FALSE;
        break;

    WINE_SPI_FIXME(SPI_SETDEFAULTINPUTLANG);	/*     90  WINVER >= 0x400 */

    WINE_SPI_FIXME(SPI_SETLANGTOGGLE);		/*     91  WINVER >= 0x400 */

    case SPI_GETWINDOWSEXTENSION:		/*     92  WINVER >= 0x400 */
	WARN("pretend no support for Win9x Plus! for now.\n");
	ret = FALSE; /* yes, this is the result value */
	break;

    WINE_SPI_FIXME(SPI_SETMOUSETRAILS);		/*     93  WINVER >= 0x400 */
    WINE_SPI_FIXME(SPI_GETMOUSETRAILS);		/*     94  WINVER >= 0x400 */

    case SPI_SETSCREENSAVERRUNNING:             /*     97  WINVER >= 0x400 */
        {
        /* SPI_SCREENSAVERRUNNING is an alias for SPI_SETSCREENSAVERRUNNING */
        char buf[5];

        spi_idx = SPI_SETSCREENSAVERRUNNING_IDX;
        sprintf(buf, "%u", uiParam);

        /* save only temporarily */
        if (SYSPARAMS_Save( SPI_SETSCREENSAVERRUNNING_REGKEY,
                            SPI_SETSCREENSAVERRUNNING_VALNAME,
                            buf, 0 ))
        {
            screensaver_running = uiParam;
            spi_loaded[spi_idx] = TRUE;
        }
        else
            ret = FALSE;
        break;
    }

    case SPI_GETMOUSEHOVERWIDTH:		/*     98  _WIN32_WINNT >= 0x400 || _WIN32_WINDOW > 0x400 */
	*(UINT *)pvParam = 4;
	break;
    WINE_SPI_FIXME(SPI_SETMOUSEHOVERWIDTH);	/*     99  _WIN32_WINNT >= 0x400 || _WIN32_WINDOW > 0x400 */

    case SPI_GETMOUSEHOVERHEIGHT:		/*    100  _WIN32_WINNT >= 0x400 || _WIN32_WINDOW > 0x400 */
	*(UINT *)pvParam = 4;
	break;
    WINE_SPI_FIXME(SPI_SETMOUSEHOVERHEIGHT);	/*    101  _WIN32_WINNT >= 0x400 || _WIN32_WINDOW > 0x400 */

    case SPI_GETMOUSEHOVERTIME:			/*    102  _WIN32_WINNT >= 0x400 || _WIN32_WINDOW > 0x400 */
	*(UINT *)pvParam = 400; /* default for menu dropdowns */
	break;
    WINE_SPI_FIXME(SPI_SETMOUSEHOVERTIME);	/*    103  _WIN32_WINNT >= 0x400 || _WIN32_WINDOW > 0x400 */

    case SPI_GETWHEELSCROLLLINES:		/*    104  _WIN32_WINNT >= 0x400 || _WIN32_WINDOW > 0x400 */
	*(UINT *)pvParam = 3; /* default for num scroll lines */
	break;

    WINE_SPI_FIXME(SPI_SETWHEELSCROLLLINES);	/*    105  _WIN32_WINNT >= 0x400 || _WIN32_WINDOW > 0x400 */

    case SPI_GETMENUSHOWDELAY:                  /*    106  _WIN32_WINNT >= 0x400 || _WIN32_WINDOW > 0x400 */
        *(UINT *)pvParam = 400; /* Tested against Windows NT 4.0 and Windows 2000 */
        break;

    WINE_SPI_FIXME(SPI_GETSHOWIMEUI);		/*    110  _WIN32_WINNT >= 0x400 || _WIN32_WINDOW > 0x400 */
    WINE_SPI_FIXME(SPI_SETSHOWIMEUI);		/*    111  _WIN32_WINNT >= 0x400 || _WIN32_WINDOW > 0x400 */

    case SPI_GETSCREENSAVERRUNNING:             /*    114  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
        spi_idx = SPI_SETSCREENSAVERRUNNING_IDX;
        if (!spi_loaded[spi_idx])
        {
            char buf[5];

            if (SYSPARAMS_Load( SPI_SETSCREENSAVERRUNNING_REGKEY,
                                SPI_SETSCREENSAVERRUNNING_VALNAME, buf ))
                screensaver_running  = atoi( buf );
            spi_loaded[spi_idx] = TRUE;
        }

	*(BOOL *)pvParam = screensaver_running;
        break;

    case SPI_GETDESKWALLPAPER:                  /*    115  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
    {
	char buf[MAX_PATH];

        if (uiParam > MAX_PATH)
	{
	    uiParam = MAX_PATH;
	}

        if (SYSPARAMS_Load(SPI_SETDESKWALLPAPER_REGKEY, SPI_SETDESKWALLPAPER_VALNAME, buf))
	{
	    strncpy((char*)pvParam, buf, uiParam);
	}
	else
	{
	    /* Return an empty string */
	    memset((char*)pvParam, 0, uiParam);
	}

	break;
    }

    WINE_SPI_FIXME(SPI_GETACTIVEWINDOWTRACKING);/* 0x1000  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
    WINE_SPI_FIXME(SPI_SETACTIVEWINDOWTRACKING);/* 0x1001  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
    WINE_SPI_FIXME(SPI_GETMENUANIMATION);       /* 0x1002  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
    WINE_SPI_FIXME(SPI_SETMENUANIMATION);       /* 0x1003  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
    WINE_SPI_FIXME(SPI_GETCOMBOBOXANIMATION);   /* 0x1004  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
    WINE_SPI_FIXME(SPI_SETCOMBOBOXANIMATION);   /* 0x1005  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
    WINE_SPI_FIXME(SPI_GETLISTBOXSMOOTHSCROLLING);/* 0x1006  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
    WINE_SPI_FIXME(SPI_SETLISTBOXSMOOTHSCROLLING);/* 0x1007  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
    WINE_SPI_FIXME(SPI_GETGRADIENTCAPTIONS);    /* 0x1008  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
    WINE_SPI_FIXME(SPI_SETGRADIENTCAPTIONS);    /* 0x1009  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
    WINE_SPI_FIXME(SPI_GETKEYBOARDCUES);        /* 0x100A  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
    WINE_SPI_FIXME(SPI_SETKEYBOARDCUES);        /* 0x100B  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
    WINE_SPI_FIXME(SPI_GETACTIVEWNDTRKZORDER);  /* 0x100C  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
    WINE_SPI_FIXME(SPI_SETACTIVEWNDTRKZORDER);  /* 0x100D  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
    WINE_SPI_FIXME(SPI_GETHOTTRACKING);         /* 0x100E  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
    WINE_SPI_FIXME(SPI_SETHOTTRACKING);         /* 0x100F  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
    WINE_SPI_FIXME(SPI_GETSELECTIONFADE);       /* 0x1014  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
    WINE_SPI_FIXME(SPI_SETSELECTIONFADE);       /* 0x1015  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
    WINE_SPI_FIXME(SPI_GETDROPSHADOW);          /* 0x1024  _WIN32_WINNT >= 0x510 */
    WINE_SPI_FIXME(SPI_SETDROPSHADOW);          /* 0x1025  _WIN32_WINNT >= 0x510 */
    WINE_SPI_FIXME(SPI_GETFOREGROUNDLOCKTIMEOUT);/* 0x2000  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
    WINE_SPI_FIXME(SPI_SETFOREGROUNDLOCKTIMEOUT);/* 0x2001  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
    WINE_SPI_FIXME(SPI_GETACTIVEWNDTRKTIMEOUT); /* 0x2002  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
    WINE_SPI_FIXME(SPI_SETACTIVEWNDTRKTIMEOUT); /* 0x2003  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
    WINE_SPI_FIXME(SPI_GETFOREGROUNDFLASHCOUNT);/* 0x2004  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
    WINE_SPI_FIXME(SPI_SETFOREGROUNDFLASHCOUNT);/* 0x2005  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */

    default:
	FIXME( "Unknown action: %u\n", uiAction );
	SetLastError( ERROR_INVALID_SPI_VALUE );
	ret = FALSE;
	break;
    }

    if (ret)
        SYSPARAMS_NotifyChange( uiAction, fWinIni );
    return ret;

#undef WINE_SPI_FIXME
#undef WINE_SPI_WARN
}


/***********************************************************************
 *		SystemParametersInfo (USER.483)
 */
BOOL16 WINAPI SystemParametersInfo16( UINT16 uAction, UINT16 uParam,
                                      LPVOID lpvParam, UINT16 fuWinIni )
{
    BOOL16 ret;

    TRACE("(%u, %u, %p, %u)\n", uAction, uParam, lpvParam, fuWinIni);

    switch (uAction)
    {
    case SPI_GETBEEP:				/*      1 */
    case SPI_GETSCREENSAVEACTIVE:		/*     16 */
    case SPI_GETICONTITLEWRAP:			/*     25 */
    case SPI_GETMENUDROPALIGNMENT:		/*     27 */
    case SPI_GETFASTTASKSWITCH:			/*     35 */
    case SPI_GETDRAGFULLWINDOWS:		/*     38  WINVER >= 0x0400 */
    {
	BOOL tmp;
	ret = SystemParametersInfoA( uAction, uParam, lpvParam ? &tmp : NULL, fuWinIni );
	if (ret && lpvParam)
	    *(BOOL16 *)lpvParam = tmp;
	break;
    }

    case SPI_GETBORDER:				/*      5 */
    case SPI_ICONHORIZONTALSPACING:		/*     13 */
    case SPI_GETSCREENSAVETIMEOUT:		/*     14 */
    case SPI_GETGRIDGRANULARITY:		/*     18 */
    case SPI_GETKEYBOARDDELAY:			/*     22 */
    case SPI_ICONVERTICALSPACING:		/*     24 */
    {
	INT tmp;
	ret = SystemParametersInfoA( uAction, uParam, lpvParam ? &tmp : NULL, fuWinIni );
	if (ret && lpvParam)
	    *(INT16 *)lpvParam = tmp;
	break;
    }

    case SPI_GETKEYBOARDSPEED:			/*     10 */
    {
	DWORD tmp;
	ret = SystemParametersInfoA( uAction, uParam, lpvParam ? &tmp : NULL, fuWinIni );
	if (ret && lpvParam)
	    *(WORD *)lpvParam = tmp;
	break;
    }

    case SPI_GETICONTITLELOGFONT:		/*     31 */
    {
	LOGFONTA tmp;
	ret = SystemParametersInfoA( uAction, uParam, lpvParam ? &tmp : NULL, fuWinIni );
	if (ret && lpvParam)
	    SYSPARAMS_LogFont32ATo16( &tmp, (LPLOGFONT16)lpvParam );
	break;
    }

    case SPI_GETNONCLIENTMETRICS: 		/*     41  WINVER >= 0x400 */
    {
	NONCLIENTMETRICSA tmp;
	LPNONCLIENTMETRICS16 lpnm16 = (LPNONCLIENTMETRICS16)lpvParam;
	if (lpnm16 && lpnm16->cbSize == sizeof(NONCLIENTMETRICS16))
	{
	    tmp.cbSize = sizeof(NONCLIENTMETRICSA);
	    ret = SystemParametersInfoA( uAction, uParam, &tmp, fuWinIni );
	    if (ret)
		SYSPARAMS_NonClientMetrics32ATo16( &tmp, lpnm16 );
	}
	else /* winfile 95 sets cbSize to 340 */
	    ret = SystemParametersInfoA( uAction, uParam, lpvParam, fuWinIni );
	break;
    }

    case SPI_GETWORKAREA:			/*     48  WINVER >= 0x400 */
    {
	RECT tmp;
	ret = SystemParametersInfoA( uAction, uParam, lpvParam ? &tmp : NULL, fuWinIni );
	if (ret && lpvParam)
	    CONV_RECT32TO16( &tmp, (RECT16 *)lpvParam );
	break;
    }

    case SPI_GETMOUSEHOVERWIDTH:		/*     98  _WIN32_WINNT >= 0x400 || _WIN32_WINDOW > 0x400 */
    case SPI_GETMOUSEHOVERHEIGHT:		/*    100  _WIN32_WINNT >= 0x400 || _WIN32_WINDOW > 0x400 */
    case SPI_GETMOUSEHOVERTIME:			/*    102  _WIN32_WINNT >= 0x400 || _WIN32_WINDOW > 0x400 */
    {
	UINT tmp;
	ret = SystemParametersInfoA( uAction, uParam, lpvParam ? &tmp : NULL, fuWinIni );
	if (ret && lpvParam)
	    *(UINT16 *)lpvParam = tmp;
	break;
    }

    default:
	ret = SystemParametersInfoA( uAction, uParam, lpvParam, fuWinIni );
    }

    return ret;
}

/***********************************************************************
 *		SystemParametersInfoW (USER32.@)
 */
BOOL WINAPI SystemParametersInfoW( UINT uiAction, UINT uiParam,
				   PVOID pvParam, UINT fuWinIni )
{
    BOOL ret;

    TRACE("(%u, %u, %p, %u)\n", uiAction, uiParam, pvParam, fuWinIni);

    switch (uiAction)
    {
    case SPI_SETDESKWALLPAPER:			/*     20 */
    case SPI_SETDESKPATTERN:			/*     21 */
    {
	char buffer[256];
	if (pvParam)
            if (!WideCharToMultiByte( CP_ACP, 0, (LPWSTR)pvParam, -1,
                                      buffer, sizeof(buffer), NULL, NULL ))
                buffer[sizeof(buffer)-1] = 0;
	ret = SystemParametersInfoA( uiAction, uiParam, pvParam ? buffer : NULL, fuWinIni );
	break;
    }

    case SPI_GETICONTITLELOGFONT:		/*     31 */
    {
	LOGFONTA tmp;
	ret = SystemParametersInfoA( uiAction, uiParam, pvParam ? &tmp : NULL, fuWinIni );
	if (ret && pvParam)
	    SYSPARAMS_LogFont32ATo32W( &tmp, (LPLOGFONTW)pvParam );
	break;
    }

    case SPI_GETNONCLIENTMETRICS: 		/*     41  WINVER >= 0x400 */
    {
	NONCLIENTMETRICSA tmp;
	LPNONCLIENTMETRICSW lpnmW = (LPNONCLIENTMETRICSW)pvParam;
	if (lpnmW && lpnmW->cbSize == sizeof(NONCLIENTMETRICSW))
	{
	    tmp.cbSize = sizeof(NONCLIENTMETRICSA);
	    ret = SystemParametersInfoA( uiAction, uiParam, &tmp, fuWinIni );
	    if (ret)
		SYSPARAMS_NonClientMetrics32ATo32W( &tmp, lpnmW );
	}
	else
	    ret = FALSE;
	break;
    }

    case SPI_GETICONMETRICS:			/*     45  WINVER >= 0x400 */
    {
	ICONMETRICSA tmp;
	LPICONMETRICSW lpimW = (LPICONMETRICSW)pvParam;
	if (lpimW && lpimW->cbSize == sizeof(ICONMETRICSW))
	{
	    tmp.cbSize = sizeof(ICONMETRICSA);
	    ret = SystemParametersInfoA( uiAction, uiParam, &tmp, fuWinIni );
	    if (ret)
	    {
		lpimW->iHorzSpacing = tmp.iHorzSpacing;
		lpimW->iVertSpacing = tmp.iVertSpacing;
		lpimW->iTitleWrap   = tmp.iTitleWrap;
		SYSPARAMS_LogFont32ATo32W( &tmp.lfFont, &lpimW->lfFont );
	    }
	}
	else
	    ret = FALSE;
	break;
    }

    case SPI_GETHIGHCONTRAST:			/*     66  WINVER >= 0x400 */
    {
	HIGHCONTRASTA tmp;
	LPHIGHCONTRASTW lphcW = (LPHIGHCONTRASTW)pvParam;
	if (lphcW && lphcW->cbSize == sizeof(HIGHCONTRASTW))
	{
	    tmp.cbSize = sizeof(HIGHCONTRASTA);
	    ret = SystemParametersInfoA( uiAction, uiParam, &tmp, fuWinIni );
	    if (ret)
	    {
		lphcW->dwFlags = tmp.dwFlags;
		lphcW->lpszDefaultScheme = NULL;  /* FIXME? */
	    }
	}
	else
	    ret = FALSE;
	break;
    }

    default:
        ret = SystemParametersInfoA( uiAction, uiParam, pvParam, fuWinIni );
        break;
    }
    return ret;
}


/**********************************************************************
 *		SetDoubleClickTime (USER32.@)
 */
BOOL WINAPI SetDoubleClickTime( UINT interval )
{
    return SystemParametersInfoA(SPI_SETDOUBLECLICKTIME, interval, 0, 0);
}


/**********************************************************************
 *		GetDoubleClickTime (USER32.@)
 */
UINT WINAPI GetDoubleClickTime(void)
{
    char buf[10];

    if (!spi_loaded[SPI_SETDOUBLECLICKTIME_IDX])
    {
        if (SYSPARAMS_Load( SPI_SETDOUBLECLICKTIME_REGKEY,
                            SPI_SETDOUBLECLICKTIME_VALNAME,
                            buf ))
        {
            double_click_time = atoi( buf );
            if (!double_click_time) double_click_time = 500;
        }
        spi_loaded[SPI_SETDOUBLECLICKTIME_IDX] = TRUE;
    }
    return double_click_time;
}
