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

#include <stdarg.h>
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
#include "wine/unicode.h"
#include "wine/debug.h"

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
#define SPI_SETFONTSMOOTHING_IDX                21
#define SPI_SETKEYBOARDCUES_IDX                 22
#define SPI_SETGRADIENTCAPTIONS_IDX             23
#define SPI_SETHOTTRACKING_IDX                  24
#define SPI_SETLISTBOXSMOOTHSCROLLING_IDX       25
#define SPI_SETMOUSEHOVERWIDTH_IDX              26
#define SPI_SETMOUSEHOVERHEIGHT_IDX             27
#define SPI_SETMOUSEHOVERTIME_IDX               28
#define SPI_SETMOUSESCROLLLINES_IDX             29
#define SPI_SETMENUSHOWDELAY_IDX                30

#define SPI_WINE_IDX                            SPI_SETMENUSHOWDELAY_IDX

/**
 * Names of the registry subkeys of HKEY_CURRENT_USER key and value names
 * for the system parameters.
 * Names of the keys are created by adding string "_REGKEY" to
 * "SET" action names, value names are created by adding "_REG_NAME"
 * to the "SET" action name.
 */
static const WCHAR SPI_SETBEEP_REGKEY[]=                      {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','S','o','u','n','d',0};
static const WCHAR SPI_SETBEEP_VALNAME[]=                     {'B','e','e','p',0};
static const WCHAR SPI_SETMOUSE_REGKEY[]=                     {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','M','o','u','s','e',0};
static const WCHAR SPI_SETMOUSE_VALNAME1[]=                   {'M','o','u','s','e','T','h','r','e','s','h','o','l','d','1',0};
static const WCHAR SPI_SETMOUSE_VALNAME2[]=                   {'M','o','u','s','e','T','h','r','e','s','h','o','l','d','2',0};
static const WCHAR SPI_SETMOUSE_VALNAME3[]=                   {'M','o','u','s','e','S','p','e','e','d',0};
static const WCHAR SPI_SETBORDER_REGKEY[]=                    {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETBORDER_VALNAME[]=                   {'B','o','r','d','e','r','W','i','d','t','h',0};
static const WCHAR SPI_SETKEYBOARDSPEED_REGKEY[]=             {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','K','e','y','b','o','a','r','d',0};
static const WCHAR SPI_SETKEYBOARDSPEED_VALNAME[]=            {'K','e','y','b','o','a','r','d','S','p','e','e','d',0};
static const WCHAR SPI_ICONHORIZONTALSPACING_REGKEY[]=        {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p','\\',
                                                               'W','i','n','d','o','w','M','e','t','r','i','c','s',0};
static const WCHAR SPI_ICONHORIZONTALSPACING_VALNAME[]=       {'I','c','o','n','S','p','a','c','i','n','g',0};
static const WCHAR SPI_SETSCREENSAVETIMEOUT_REGKEY[]=         {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETSCREENSAVETIMEOUT_VALNAME[]=        {'S','c','r','e','e','n','S','a','v','e','T','i','m','e','O','u','t',0};
static const WCHAR SPI_SETSCREENSAVEACTIVE_REGKEY[]=          {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETSCREENSAVEACTIVE_VALNAME[]=         {'S','c','r','e','e','n','S','a','v','e','A','c','t','i','v','e',0};
static const WCHAR SPI_SETGRIDGRANULARITY_REGKEY[]=           {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETGRIDGRANULARITY_VALNAME[]=          {'G','r','i','d','G','r','a','n','u','l','a','r','i','t','y',0};
static const WCHAR SPI_SETKEYBOARDDELAY_REGKEY[]=             {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','K','e','y','b','o','a','r','d',0};
static const WCHAR SPI_SETKEYBOARDDELAY_VALNAME[]=            {'K','e','y','b','o','a','r','d','D','e','l','a','y',0};
static const WCHAR SPI_ICONVERTICALSPACING_REGKEY[]=          {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p','\\',
                                                               'W','i','n','d','o','w','M','e','t','r','i','c','s',0};
static const WCHAR SPI_ICONVERTICALSPACING_VALNAME[]=         {'I','c','o','n','V','e','r','t','i','c','a','l','S','p','a','c','i','n','g',0};
static const WCHAR SPI_SETICONTITLEWRAP_REGKEY1[]=            {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p','\\',
                                                               'W','i','n','d','o','w','M','e','t','r','i','c','s',0};
static const WCHAR SPI_SETICONTITLEWRAP_REGKEY2[]=            {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETICONTITLEWRAP_VALNAME[]=            {'I','c','o','n','T','i','t','l','e','W','r','a','p',0};
static const WCHAR SPI_SETMENUDROPALIGNMENT_REGKEY1[]=        {'S','o','f','t','w','a','r','e','\\',
                                                               'M','i','c','r','o','s','o','f','t','\\',
                                                               'W','i','n','d','o','w','s',' ','N','T','\\',
                                                               'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
                                                               'W','i','n','d','o','w','s',0};
static const WCHAR SPI_SETMENUDROPALIGNMENT_REGKEY2[]=        {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETMENUDROPALIGNMENT_VALNAME[]=        {'M','e','n','u','D','r','o','p','A','l','i','g','n','m','e','n','t',0};
static const WCHAR SPI_SETDOUBLECLKWIDTH_REGKEY1[]=           {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','M','o','u','s','e',0};
static const WCHAR SPI_SETDOUBLECLKWIDTH_REGKEY2[]=           {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETDOUBLECLKWIDTH_VALNAME[]=           {'D','o','u','b','l','e','C','l','i','c','k','W','i','d','t','h',0};
static const WCHAR SPI_SETDOUBLECLKHEIGHT_REGKEY1[]=          {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','M','o','u','s','e',0};
static const WCHAR SPI_SETDOUBLECLKHEIGHT_REGKEY2[]=          {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETDOUBLECLKHEIGHT_VALNAME[]=          {'D','o','u','b','l','e','C','l','i','c','k','H','e','i','g','h','t',0};
static const WCHAR SPI_SETDOUBLECLICKTIME_REGKEY[]=           {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','M','o','u','s','e',0};
static const WCHAR SPI_SETDOUBLECLICKTIME_VALNAME[]=          {'D','o','u','b','l','e','C','l','i','c','k','S','p','e','e','d',0};
static const WCHAR SPI_SETMOUSEBUTTONSWAP_REGKEY[]=           {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','M','o','u','s','e',0};
static const WCHAR SPI_SETMOUSEBUTTONSWAP_VALNAME[]=          {'S','w','a','p','M','o','u','s','e','B','u','t','t','o','n','s',0};
static const WCHAR SPI_SETDRAGFULLWINDOWS_REGKEY[]=           {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETDRAGFULLWINDOWS_VALNAME[]=          {'D','r','a','g','F','u','l','l','W','i','n','d','o','w','s',0};
static const WCHAR SPI_SETWORKAREA_REGKEY[]=                  {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETWORKAREA_VALNAME[]=                 {'W','I','N','E','_','W','o','r','k','A','r','e','a',0};
static const WCHAR SPI_SETSHOWSOUNDS_REGKEY[]=                {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\',
                                                               'A','c','c','e','s','s','i','b','i','l','i','t','y','\\',
                                                               'S','h','o','w','S','o','u','n','d','s',0};
static const WCHAR SPI_SETSHOWSOUNDS_VALNAME[]=               {'O','n',0};
static const WCHAR SPI_SETDESKWALLPAPER_REGKEY[]=             {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETDESKWALLPAPER_VALNAME[]=            {'W','a','l','l','p','a','p','e','r',0};
static const WCHAR SPI_SETFONTSMOOTHING_REGKEY[]=             {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETFONTSMOOTHING_VALNAME[]=            {'F','o','n','t','S','m','o','o','t','h','i','n','g',0};
static const WCHAR SPI_USERPREFERENCEMASK_REGKEY[]=           {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_USERPREFERENCEMASK_VALNAME[]=          {'U','s','e','r','P','r','e','f','e','r','e','n','c','e','m','a','s','k',0};
static const WCHAR SPI_SETLISTBOXSMOOTHSCROLLING_REGKEY[]=    {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETLISTBOXSMOOTHSCROLLING_VALNAME[]=   {'S','m','o','o','t','h','S','c','r','o','l','l',0};
static const WCHAR SPI_SETMOUSEHOVERWIDTH_REGKEY[]=           {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','M','o','u','s','e',0};
static const WCHAR SPI_SETMOUSEHOVERWIDTH_VALNAME[]=          {'M','o','u','s','e','H','o','v','e','r','W','i','d','t','h',0};
static const WCHAR SPI_SETMOUSEHOVERHEIGHT_REGKEY[]=          {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','M','o','u','s','e',0};
static const WCHAR SPI_SETMOUSEHOVERHEIGHT_VALNAME[]=         {'M','o','u','s','e','H','o','v','e','r','H','e','i','g','h','t',0};
static const WCHAR SPI_SETMOUSEHOVERTIME_REGKEY[]=            {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','M','o','u','s','e',0};
static const WCHAR SPI_SETMOUSEHOVERTIME_VALNAME[]=           {'M','o','u','s','e','H','o','v','e','r','T','i','m','e',0};
static const WCHAR SPI_SETMOUSESCROLLLINES_REGKEY[]=          {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETMOUSESCROLLLINES_VALNAME[]=         {'W','h','e','e','l','S','c','r','o','l','l','L','i','n','e','s',0};
static const WCHAR SPI_SETMENUSHOWDELAY_REGKEY[]=             {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETMENUSHOWDELAY_VALNAME[]=            {'M','e','n','u','S','h','o','w','D','e','l','a','y',0};

/* FIXME - real values */
static const WCHAR SPI_SETKEYBOARDPREF_REGKEY[]=         {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETKEYBOARDPREF_VALNAME[]=        {'W','I','N','E','_','K','e','y','b','o','a','r','d','P','r','e','f',0};
static const WCHAR SPI_SETSCREENREADER_REGKEY[]=         {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETSCREENREADER_VALNAME[]=        {'W','I','N','E','_','S','c','r','e','e','n','R','e','a','d','e','r',0};
static const WCHAR SPI_SETSCREENSAVERRUNNING_REGKEY[]=   {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETSCREENSAVERRUNNING_VALNAME[]=  {'W','I','N','E','_','S','c','r','e','e','n','S','a','v','e','r','R','u','n','n','i','n','g',0};

/* volatile registry branch under CURRENT_USER_REGKEY for temporary values storage */
static const WCHAR WINE_CURRENT_USER_REGKEY[] = {'W','i','n','e',0};


static const WCHAR Yes[]=                                    {'Y','e','s',0};
static const WCHAR No[]=                                     {'N','o',0};
static const WCHAR Desktop[]=                                {'D','e','s','k','t','o','p',0};
static const WCHAR Pattern[]=                                {'P','a','t','t','e','r','n',0};
static const WCHAR MenuFont[]=                               {'M','e','n','u','F','o','n','t',0};
static const WCHAR MenuFontSize[]=                           {'M','e','n','u','F','o','n','t','S','i','z','e',0};
static const WCHAR System[]=                                 {'S','y','s','t','e','m',0};
static const WCHAR IconTitleSize[]=                          {'I','c','o','n','T','i','t','l','e','S','i','z','e',0};
static const WCHAR IconTitleFaceName[]=                      {'I','c','o','n','T','i','t','l','e','F','a','c','e','N','a','m','e',0};
static const WCHAR defPattern[]=                             {'1','7','0',' ','8','5',' ','1','7','0',' ','8','5',' ','1','7','0',' ','8','5',
                                                              ' ','1','7','0',' ','8','5',0};
static const WCHAR CSu[]=                                    {'%','u',0};
static const WCHAR CSd[]=                                    {'%','d',0};

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
static int mouse_hover_width = 4;
static int mouse_hover_height = 4;
static int mouse_hover_time = 400;
static int mouse_scroll_lines = 3;
static int menu_show_delay = 400;
static BOOL screensaver_running = FALSE;
static BOOL font_smoothing = FALSE;
static BOOL keyboard_cues = FALSE;
static BOOL gradient_captions = FALSE;
static BOOL listbox_smoothscrolling = FALSE;
static BOOL hot_tracking = FALSE;

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
        if (RegCreateKeyExW( HKEY_CURRENT_USER, WINE_CURRENT_USER_REGKEY,
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
    static const WCHAR emptyW[1];

    if (notify_change)
    {
        if (fWinIni & SPIF_UPDATEINIFILE)
        {
            if (fWinIni & (SPIF_SENDWININICHANGE | SPIF_SENDCHANGE))
                SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE,
                                    uiAction, (LPARAM) emptyW,
                                    SMTO_ABORTIFHUNG, 2000, NULL );
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
static BOOL SYSPARAMS_Load( LPCWSTR lpRegKey, LPCWSTR lpValName, LPWSTR lpBuf, DWORD count )
{
    BOOL ret = FALSE;
    DWORD type;
    HKEY hKey;

    if ((RegOpenKeyW( get_volatile_regkey(), lpRegKey, &hKey ) == ERROR_SUCCESS) ||
        (RegOpenKeyW( HKEY_CURRENT_USER, lpRegKey, &hKey ) == ERROR_SUCCESS))
    {
        ret = !RegQueryValueExW( hKey, lpValName, NULL, &type, (LPBYTE)lpBuf, &count);
        RegCloseKey( hKey );
    }
    return ret;
}

/***********************************************************************
 * Saves system parameter to user profile.
 */
static BOOL SYSPARAMS_Save( LPCWSTR lpRegKey, LPCWSTR lpValName, LPCWSTR lpValue,
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

    if (RegCreateKeyExW( hBaseKey, lpRegKey,
                         0, 0, dwOptions, KEY_ALL_ACCESS,
                         0, &hKey, 0 ) == ERROR_SUCCESS)
    {
        if (RegSetValueExW( hKey, lpValName, 0, REG_SZ,
                            (const BYTE*)lpValue,
                            (strlenW(lpValue) + 1)*sizeof(WCHAR)) == ERROR_SUCCESS)
        {
            ret = TRUE;
            if (hBaseKey == HKEY_CURRENT_USER)
                RegDeleteKeyW( get_volatile_regkey(), lpRegKey );
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
    WCHAR buf[10];

    if (!spi_loaded[SPI_SETDOUBLECLKWIDTH_IDX])
    {
        if (SYSPARAMS_Load( SPI_SETDOUBLECLKWIDTH_REGKEY1,
                            SPI_SETDOUBLECLKWIDTH_VALNAME, buf, sizeof(buf) ))
        {
            SYSMETRICS_Set( SM_CXDOUBLECLK, atoiW( buf ) );
        }
        spi_loaded[SPI_SETDOUBLECLKWIDTH_IDX] = TRUE;
    }
    if (!spi_loaded[SPI_SETDOUBLECLKHEIGHT_IDX])
    {
        if (SYSPARAMS_Load( SPI_SETDOUBLECLKHEIGHT_REGKEY1,
                            SPI_SETDOUBLECLKHEIGHT_VALNAME, buf, sizeof(buf) ))
        {
            SYSMETRICS_Set( SM_CYDOUBLECLK, atoiW( buf ) );
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
            WCHAR buf[5];

            if (SYSPARAMS_Load( SPI_SETMOUSEBUTTONSWAP_REGKEY,
                                SPI_SETMOUSEBUTTONSWAP_VALNAME, buf, sizeof(buf) ))
            {
                SYSMETRICS_Set( SM_SWAPBUTTON, atoiW( buf ) );
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

static void SYSPARAMS_GetGUIFont( LOGFONTW* plf )
{
	HFONT	hf;

	memset( plf, 0, sizeof(LOGFONTW) );
	hf = (HFONT)GetStockObject( DEFAULT_GUI_FONT );
	if ( GetObjectW( hf, sizeof(LOGFONTW), plf ) != sizeof(LOGFONTW) )
	{
		/*
		 * GetObjectW() would be succeeded always
		 * since this is a stock object
		 */
		ERR("GetObjectW() failed\n");
	}
}

/* copied from GetSystemMetrics()'s RegistryTwips2Pixels() */
inline static int SYSPARAMS_Twips2Pixels(int x)
{
    if (x < 0)
        x = (-x+7)/15;
    return x;
}

/***********************************************************************
 *		SystemParametersInfoW (USER32.@)
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
 * Some SPI values can also be stored as Twips values in the registry,
 * don't forget the conversion!
 */
BOOL WINAPI SystemParametersInfoW( UINT uiAction, UINT uiParam,
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
        if (!pvParam) return FALSE;

        spi_idx = SPI_SETBEEP_IDX;
        if (!spi_loaded[spi_idx])
        {
            WCHAR buf[5];

            if (SYSPARAMS_Load( SPI_SETBEEP_REGKEY, SPI_SETBEEP_VALNAME, buf, sizeof(buf) ))
                beep_active  = !lstrcmpiW( Yes, buf );
            spi_loaded[spi_idx] = TRUE;
        }

	*(BOOL *)pvParam = beep_active;
        break;

    case SPI_SETBEEP:				/*      2 */
        spi_idx = SPI_SETBEEP_IDX;
        if (SYSPARAMS_Save( SPI_SETBEEP_REGKEY, SPI_SETBEEP_VALNAME,
                            (uiParam ? Yes : No), fWinIni ))
        {
            beep_active = uiParam;
            spi_loaded[spi_idx] = TRUE;
        }
        else
            ret = FALSE;
        break;

    case SPI_GETMOUSE:                          /*      3 */
        if (!pvParam) return FALSE;

        spi_idx = SPI_SETMOUSE_IDX;
        if (!spi_loaded[spi_idx])
        {
            WCHAR buf[10];

            if (SYSPARAMS_Load( SPI_SETMOUSE_REGKEY, SPI_SETMOUSE_VALNAME1,
                                buf, sizeof(buf) ))
                mouse_threshold1 = atoiW( buf );
            if (SYSPARAMS_Load( SPI_SETMOUSE_REGKEY, SPI_SETMOUSE_VALNAME2,
                                buf, sizeof(buf) ))
                mouse_threshold2 = atoiW( buf );
            if (SYSPARAMS_Load( SPI_SETMOUSE_REGKEY, SPI_SETMOUSE_VALNAME3,
                                buf, sizeof(buf) ))
                mouse_speed = atoiW( buf );
            spi_loaded[spi_idx] = TRUE;
        }
        ((INT *)pvParam)[0] = mouse_threshold1;
        ((INT *)pvParam)[1] = mouse_threshold2;
        ((INT *)pvParam)[2] = mouse_speed;
        break;

    case SPI_SETMOUSE:                          /*      4 */
    {
        WCHAR buf[10];

        if (!pvParam) return FALSE;

        spi_idx = SPI_SETMOUSE_IDX;
        wsprintfW(buf, CSd, ((INT *)pvParam)[0]);

        if (SYSPARAMS_Save( SPI_SETMOUSE_REGKEY, SPI_SETMOUSE_VALNAME1,
                            buf, fWinIni ))
        {
            mouse_threshold1 = ((INT *)pvParam)[0];
            spi_loaded[spi_idx] = TRUE;

            wsprintfW(buf, CSd, ((INT *)pvParam)[1]);
            SYSPARAMS_Save( SPI_SETMOUSE_REGKEY, SPI_SETMOUSE_VALNAME2,
                            buf, fWinIni );
            mouse_threshold2 = ((INT *)pvParam)[1];

            wsprintfW(buf, CSd, ((INT *)pvParam)[2]);
            SYSPARAMS_Save( SPI_SETMOUSE_REGKEY, SPI_SETMOUSE_VALNAME3,
                            buf, fWinIni );
            mouse_speed = ((INT *)pvParam)[2];
        }
        else
            ret = FALSE;
        break;
    }

    case SPI_GETBORDER:				/*      5 */
        if (!pvParam) return FALSE;

        spi_idx = SPI_SETBORDER_IDX;
        if (!spi_loaded[spi_idx])
        {
            WCHAR buf[10];

            if (SYSPARAMS_Load( SPI_SETBORDER_REGKEY, SPI_SETBORDER_VALNAME, buf, sizeof(buf) ))
                border = SYSPARAMS_Twips2Pixels( atoiW(buf) );

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
        WCHAR buf[10];

        spi_idx = SPI_SETBORDER_IDX;
        wsprintfW(buf, CSu, uiParam);

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
        if (!pvParam) return FALSE;

        spi_idx = SPI_SETKEYBOARDSPEED_IDX;
        if (!spi_loaded[spi_idx])
        {
            WCHAR buf[10];

            if (SYSPARAMS_Load( SPI_SETKEYBOARDSPEED_REGKEY,
                                SPI_SETKEYBOARDSPEED_VALNAME,
                                buf, sizeof(buf) ))
                keyboard_speed = atoiW( buf );
            spi_loaded[spi_idx] = TRUE;
        }
	*(INT *)pvParam = keyboard_speed;
	break;

    case SPI_SETKEYBOARDSPEED:                  /*     11 */
    {
        WCHAR buf[10];

        spi_idx = SPI_SETKEYBOARDSPEED_IDX;
        if (uiParam > 31)
            uiParam = 31;
        wsprintfW(buf, CSu, uiParam);

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
                WCHAR buf[10];
		int val;

                if (SYSPARAMS_Load( SPI_ICONHORIZONTALSPACING_REGKEY,
                                    SPI_ICONHORIZONTALSPACING_VALNAME, buf, sizeof(buf) ))
                {
                    val = SYSPARAMS_Twips2Pixels( atoiW(buf) );
                    SYSMETRICS_Set( SM_CXICONSPACING, val );
                }
                spi_loaded[spi_idx] = TRUE;
            }

            *(INT *)pvParam = GetSystemMetrics( SM_CXICONSPACING );
        }
	else
        {
            WCHAR buf[10];

            if (uiParam < 32) uiParam = 32;

            wsprintfW(buf, CSu, uiParam);
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
        if (!pvParam) return FALSE;

        spi_idx = SPI_SETSCREENSAVETIMEOUT_IDX;
        if (!spi_loaded[spi_idx])
        {
            WCHAR buf[10];

            if (SYSPARAMS_Load( SPI_SETSCREENSAVETIMEOUT_REGKEY,
                                SPI_SETSCREENSAVETIMEOUT_VALNAME,
                                buf, sizeof(buf) ))
                screensave_timeout = atoiW( buf );

            spi_loaded[spi_idx] = TRUE;
        }
	*(INT *)pvParam = screensave_timeout;
	break;

    case SPI_SETSCREENSAVETIMEOUT:              /*     15 */
    {
        WCHAR buf[10];

        spi_idx = SPI_SETSCREENSAVETIMEOUT_IDX;
        wsprintfW(buf, CSu, uiParam);

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
        if (!pvParam) return FALSE;
	*(BOOL *)pvParam = USER_Driver.pGetScreenSaveActive();
        break;

    case SPI_SETSCREENSAVEACTIVE:               /*     17 */
    {
        WCHAR buf[5];

        wsprintfW(buf, CSu, uiParam);
        USER_Driver.pSetScreenSaveActive( uiParam );
        /* saved value does not affect Wine */
        SYSPARAMS_Save( SPI_SETSCREENSAVEACTIVE_REGKEY,
                        SPI_SETSCREENSAVEACTIVE_VALNAME,
                        buf, fWinIni );
        break;
    }

    case SPI_GETGRIDGRANULARITY:                /*     18 */
        if (!pvParam) return FALSE;

        spi_idx = SPI_SETGRIDGRANULARITY_IDX;
        if (!spi_loaded[spi_idx])
        {
            WCHAR buf[10];

            if (SYSPARAMS_Load( SPI_SETGRIDGRANULARITY_REGKEY,
                                SPI_SETGRIDGRANULARITY_VALNAME,
                                buf, sizeof(buf) ))
                grid_granularity = atoiW( buf );

            spi_loaded[spi_idx] = TRUE;
        }
	*(INT *)pvParam = grid_granularity;
	break;

    case SPI_SETGRIDGRANULARITY:                /*     19 */
    {
        WCHAR buf[10];

        spi_idx = SPI_SETGRIDGRANULARITY_IDX;
        wsprintfW(buf, CSu, uiParam);

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
        if (!pvParam || !SetDeskWallPaper( (LPSTR)pvParam )) return FALSE;
        SYSPARAMS_Save(SPI_SETDESKWALLPAPER_REGKEY, SPI_SETDESKWALLPAPER_VALNAME, pvParam, fWinIni);
	break;
	
    case SPI_SETDESKPATTERN:			/*     21 */
	/* FIXME: the ability to specify a pattern in pvParam
	   doesn't seem to be documented for Win32 */
	if ((INT16)uiParam == -1)
	{
            WCHAR buf[256];
            GetProfileStringW( Desktop, Pattern,
                               defPattern,
                               buf, sizeof(buf)/sizeof(WCHAR) );
            ret = DESKTOP_SetPattern( buf );
        } else
            ret = DESKTOP_SetPattern( (LPWSTR)pvParam );
	break;

    case SPI_GETKEYBOARDDELAY:                  /*     22 */
        if (!pvParam) return FALSE;

        spi_idx = SPI_SETKEYBOARDDELAY_IDX;
        if (!spi_loaded[spi_idx])
        {
            WCHAR buf[10];

            if (SYSPARAMS_Load( SPI_SETKEYBOARDDELAY_REGKEY,
                                SPI_SETKEYBOARDDELAY_VALNAME,
                                buf, sizeof(buf) ))
            {
                int i = atoiW( buf );
                if ( (i >= 0) && (i <= 3)) keyboard_delay = i;
            }

            spi_loaded[spi_idx] = TRUE;
        }
	*(INT *)pvParam = keyboard_delay;
	break;

    case SPI_SETKEYBOARDDELAY:                  /*     23 */
    {
        WCHAR buf[10];

        spi_idx = SPI_SETKEYBOARDDELAY_IDX;
        wsprintfW(buf, CSu, uiParam);

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
                WCHAR buf[10];
		int val;

                if (SYSPARAMS_Load( SPI_ICONVERTICALSPACING_REGKEY,
                                    SPI_ICONVERTICALSPACING_VALNAME, buf, sizeof(buf) ))
                {
                    val = SYSPARAMS_Twips2Pixels( atoiW(buf) );
                    SYSMETRICS_Set( SM_CYICONSPACING, val );
                }
                spi_loaded[spi_idx] = TRUE;
            }

            *(INT *)pvParam = GetSystemMetrics( SM_CYICONSPACING );
        }
	else
        {
            WCHAR buf[10];

            if (uiParam < 32) uiParam = 32;

            wsprintfW(buf, CSu, uiParam);
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
        if (!pvParam) return FALSE;

        spi_idx = SPI_SETICONTITLEWRAP_IDX;
        if (!spi_loaded[spi_idx])
        {
            WCHAR buf[5];

            if (SYSPARAMS_Load( SPI_SETICONTITLEWRAP_REGKEY1,
                                SPI_SETICONTITLEWRAP_VALNAME, buf, sizeof(buf) ))
                icon_title_wrap  = atoiW(buf);
            spi_loaded[spi_idx] = TRUE;
        }

	*(BOOL *)pvParam = icon_title_wrap;
        break;

    case SPI_SETICONTITLEWRAP:                  /*     26 */
    {
        WCHAR buf[5];

        spi_idx = SPI_SETICONTITLEWRAP_IDX;
        wsprintfW(buf, CSu, uiParam);
        if (SYSPARAMS_Save( SPI_SETICONTITLEWRAP_REGKEY1,
                            SPI_SETICONTITLEWRAP_VALNAME,
                            buf, fWinIni ))
        {
            SYSPARAMS_Save( SPI_SETICONTITLEWRAP_REGKEY2,
                            SPI_SETICONTITLEWRAP_VALNAME,
                            buf, fWinIni );
            icon_title_wrap = uiParam;
            spi_loaded[spi_idx] = TRUE;
        }
        else
            ret = FALSE;
        break;
    }

    case SPI_GETMENUDROPALIGNMENT:              /*     27 */
        if (!pvParam) return FALSE;

        spi_idx = SPI_SETMENUDROPALIGNMENT_IDX;

        if (!spi_loaded[spi_idx])
        {
            WCHAR buf[5];

            if (SYSPARAMS_Load( SPI_SETMENUDROPALIGNMENT_REGKEY1,
                                SPI_SETMENUDROPALIGNMENT_VALNAME, buf, sizeof(buf) ))
            {
                SYSMETRICS_Set( SM_MENUDROPALIGNMENT, atoiW( buf ) );
            }
            spi_loaded[spi_idx] = TRUE;
        }

        *(BOOL *)pvParam = GetSystemMetrics( SM_MENUDROPALIGNMENT );
        break;

    case SPI_SETMENUDROPALIGNMENT:              /*     28 */
    {
        WCHAR buf[5];
        spi_idx = SPI_SETMENUDROPALIGNMENT_IDX;

        wsprintfW(buf, CSu, uiParam);
        if (SYSPARAMS_Save( SPI_SETMENUDROPALIGNMENT_REGKEY1,
                            SPI_SETMENUDROPALIGNMENT_VALNAME,
                            buf, fWinIni ))
        {
            SYSPARAMS_Save( SPI_SETMENUDROPALIGNMENT_REGKEY2,
                            SPI_SETMENUDROPALIGNMENT_VALNAME,
                            buf, fWinIni );
            SYSMETRICS_Set( SM_MENUDROPALIGNMENT, uiParam );
            spi_loaded[spi_idx] = TRUE;
        }
        else
            ret = FALSE;
        break;
    }

    case SPI_SETDOUBLECLKWIDTH:                 /*     29 */
    {
        WCHAR buf[10];
        spi_idx = SPI_SETDOUBLECLKWIDTH_IDX;

        wsprintfW(buf, CSu, uiParam);
        if (SYSPARAMS_Save( SPI_SETDOUBLECLKWIDTH_REGKEY1,
                            SPI_SETDOUBLECLKWIDTH_VALNAME,
                            buf, fWinIni ))
        {
            SYSPARAMS_Save( SPI_SETDOUBLECLKWIDTH_REGKEY2,
                            SPI_SETDOUBLECLKWIDTH_VALNAME,
                            buf, fWinIni );
            SYSMETRICS_Set( SM_CXDOUBLECLK, uiParam );
            spi_loaded[spi_idx] = TRUE;
        }
        else
            ret = FALSE;
        break;
    }

    case SPI_SETDOUBLECLKHEIGHT:                /*     30 */
    {
        WCHAR buf[10];
        spi_idx = SPI_SETDOUBLECLKHEIGHT_IDX;

        wsprintfW(buf, CSu, uiParam);
        if (SYSPARAMS_Save( SPI_SETDOUBLECLKHEIGHT_REGKEY1,
                            SPI_SETDOUBLECLKHEIGHT_VALNAME,
                            buf, fWinIni ))
        {
            SYSPARAMS_Save( SPI_SETDOUBLECLKHEIGHT_REGKEY2,
                            SPI_SETDOUBLECLKHEIGHT_VALNAME,
                            buf, fWinIni );
            SYSMETRICS_Set( SM_CYDOUBLECLK, uiParam );
            spi_loaded[spi_idx] = TRUE;
        }
        else
            ret = FALSE;
        break;
    }

    case SPI_GETICONTITLELOGFONT:		/*     31 */
    {
	LPLOGFONTW lpLogFont = (LPLOGFONTW)pvParam;
	LOGFONTW	lfDefault;

        if (!pvParam) return FALSE;

	/*
	 * The 'default GDI fonts' seems to be returned.
	 * If a returned font is not a correct font in your environment,
	 * please try to fix objects/gdiobj.c at first.
	 */
	SYSPARAMS_GetGUIFont( &lfDefault );

	GetProfileStringW( Desktop, IconTitleFaceName,
			   lfDefault.lfFaceName,
			   lpLogFont->lfFaceName, LF_FACESIZE );
	lpLogFont->lfHeight = -GetProfileIntW( Desktop, IconTitleSize, 11 );
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
	lpLogFont->lfQuality = DEFAULT_QUALITY;
	break;
    }

    case SPI_SETDOUBLECLICKTIME:                /*     32 */
    {
        WCHAR buf[10];

        spi_idx = SPI_SETDOUBLECLICKTIME_IDX;
        wsprintfW(buf, CSu, uiParam);

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
        WCHAR buf[5];
        spi_idx = SPI_SETMOUSEBUTTONSWAP_IDX;

        wsprintfW(buf, CSu, uiParam);
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
        if (!pvParam) return FALSE;
	*(BOOL *)pvParam = 1;
        break;

    case SPI_SETFASTTASKSWITCH:                 /*     36 */
        /* the action is disabled */
        ret = FALSE;
        break;

    case SPI_SETDRAGFULLWINDOWS:                /*     37  WINVER >= 0x0400 */
    {
        WCHAR buf[5];

        spi_idx = SPI_SETDRAGFULLWINDOWS_IDX;
        wsprintfW(buf, CSu, uiParam);
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
        if (!pvParam) return FALSE;

        spi_idx = SPI_SETDRAGFULLWINDOWS_IDX;
        if (!spi_loaded[spi_idx])
        {
            WCHAR buf[5];

            if (SYSPARAMS_Load( SPI_SETDRAGFULLWINDOWS_REGKEY,
                                SPI_SETDRAGFULLWINDOWS_VALNAME, buf, sizeof(buf) ))
                drag_full_windows  = atoiW(buf);
            spi_loaded[spi_idx] = TRUE;
        }

	*(BOOL *)pvParam = drag_full_windows;
        break;

    case SPI_GETNONCLIENTMETRICS: 		/*     41  WINVER >= 0x400 */
    {
	LPNONCLIENTMETRICSW lpnm = (LPNONCLIENTMETRICSW)pvParam;

        if (!pvParam) return FALSE;

	if (lpnm->cbSize == sizeof(NONCLIENTMETRICSW))
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
	    SystemParametersInfoW( SPI_GETICONTITLELOGFONT, 0, (LPVOID)&(lpnm->lfCaptionFont), 0 );
	    lpnm->lfCaptionFont.lfWeight = FW_BOLD;

	    /* size of the small caption buttons */
	    lpnm->iSmCaptionWidth = GetSystemMetrics(SM_CXSMSIZE);
	    lpnm->iSmCaptionHeight = GetSystemMetrics(SM_CYSMSIZE);

	    /* small caption font metrics */
	    SystemParametersInfoW( SPI_GETICONTITLELOGFONT, 0, (LPVOID)&(lpnm->lfSmCaptionFont), 0 );

	    /* menus, FIXME: names of wine.conf entries are bogus */

	    /* size of the menu (MDI) buttons */
	    lpnm->iMenuWidth = GetSystemMetrics(SM_CXMENUSIZE);
	    lpnm->iMenuHeight = GetSystemMetrics(SM_CYMENUSIZE);

	    /* menu font metrics */
	    SystemParametersInfoW( SPI_GETICONTITLELOGFONT, 0, (LPVOID)&(lpnm->lfMenuFont), 0 );
	    GetProfileStringW( Desktop, MenuFont,
			       (TWEAK_WineLook > WIN31_LOOK) ? lpnm->lfCaptionFont.lfFaceName : System,
			       lpnm->lfMenuFont.lfFaceName, LF_FACESIZE );
	    lpnm->lfMenuFont.lfHeight = -GetProfileIntW( Desktop, MenuFontSize, 11 );
	    lpnm->lfMenuFont.lfWeight = (TWEAK_WineLook > WIN31_LOOK) ? FW_NORMAL : FW_BOLD;

	    /* status bar font metrics */
	    SystemParametersInfoW( SPI_GETICONTITLELOGFONT, 0,
				   (LPVOID)&(lpnm->lfStatusFont), 0 );
	    /* message font metrics */
	    SystemParametersInfoW( SPI_GETICONTITLELOGFONT, 0,
				   (LPVOID)&(lpnm->lfMessageFont), 0 );
	}
	else
	{
	    WARN("size mismatch !! (is %d; should be %d)\n", lpnm->cbSize, sizeof(NONCLIENTMETRICSW));
	    /* FIXME: SetLastError? */
	    ret = FALSE;
	}
	break;
    }
    WINE_SPI_FIXME(SPI_SETNONCLIENTMETRICS);	/*     42  WINVER >= 0x400 */

    case SPI_GETMINIMIZEDMETRICS: 		/*     43  WINVER >= 0x400 */
    {
        MINIMIZEDMETRICS * lpMm = pvParam;
	if (lpMm && lpMm->cbSize == sizeof(*lpMm))
	{
	    /* these taken from Win2k SP3 */
	    lpMm->iWidth = 154;
	    lpMm->iHorzGap = 0;
	    lpMm->iVertGap = 0;
	    lpMm->iArrange = 8;
	}
	else
	    ret = FALSE;
	break;
    }
    WINE_SPI_FIXME(SPI_SETMINIMIZEDMETRICS);	/*     44  WINVER >= 0x400 */

    case SPI_GETICONMETRICS:			/*     45  WINVER >= 0x400 */
    {
	LPICONMETRICSW lpIcon = pvParam;
	if(lpIcon && lpIcon->cbSize == sizeof(*lpIcon))
	{
	    SystemParametersInfoW( SPI_ICONHORIZONTALSPACING, 0,
				   &lpIcon->iHorzSpacing, FALSE );
	    SystemParametersInfoW( SPI_ICONVERTICALSPACING, 0,
				   &lpIcon->iVertSpacing, FALSE );
	    SystemParametersInfoW( SPI_GETICONTITLEWRAP, 0,
				   &lpIcon->iTitleWrap, FALSE );
	    SystemParametersInfoW( SPI_GETICONTITLELOGFONT, 0,
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
        static const WCHAR CSld[]={'%','l','d',' ','%','l','d',' ','%','l','d',' ','%','l','d',0};
        WCHAR buf[20];
        RECT *pr = (RECT *) pvParam;

        if (!pvParam) return FALSE;

        spi_idx = SPI_SETWORKAREA_IDX;
        wsprintfW(buf, CSld, pr->left, pr->top,
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
        if (!pvParam) return FALSE;

      spi_idx = SPI_SETWORKAREA_IDX;
      if (!spi_loaded[spi_idx])
      {
          WCHAR buf[20];

          SetRect( &work_area, 0, 0,
                   GetSystemMetrics( SM_CXSCREEN ),
                   GetSystemMetrics( SM_CYSCREEN ) );

          if (SYSPARAMS_Load( SPI_SETWORKAREA_REGKEY,
                              SPI_SETWORKAREA_VALNAME,
                              buf, sizeof(buf) ))
          {
              char tmpbuf[20];
              WideCharToMultiByte( CP_ACP, 0, buf, -1,
                                   tmpbuf, sizeof(tmpbuf), NULL, NULL );
              sscanf( tmpbuf, "%ld %ld %ld %ld",
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
        if (lpFilterKeys && lpFilterKeys->cbSize == sizeof(FILTERKEYS))
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
        if (lpToggleKeys && lpToggleKeys->cbSize == sizeof(TOGGLEKEYS))
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
        if (lpMouseKeys && lpMouseKeys->cbSize == sizeof(MOUSEKEYS))
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
        if (!pvParam) return FALSE;

        spi_idx = SPI_SETSHOWSOUNDS_IDX;

        if (!spi_loaded[spi_idx])
        {
            WCHAR buf[10];

            if (SYSPARAMS_Load( SPI_SETSHOWSOUNDS_REGKEY,
                                SPI_SETSHOWSOUNDS_VALNAME, buf, sizeof(buf) ))
            {
                SYSMETRICS_Set( SM_SHOWSOUNDS, atoiW( buf ) );
            }
            spi_loaded[spi_idx] = TRUE;
        }

        *(INT *)pvParam = GetSystemMetrics( SM_SHOWSOUNDS );
        break;

    case SPI_SETSHOWSOUNDS:			/*     57 */
    {
        WCHAR buf[10];
        spi_idx = SPI_SETSHOWSOUNDS_IDX;

        wsprintfW(buf, CSu, uiParam);
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
        if (lpStickyKeys && lpStickyKeys->cbSize == sizeof(STICKYKEYS))
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
        if (lpAccessTimeout && lpAccessTimeout->cbSize == sizeof(ACCESSTIMEOUT))
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
        LPSERIALKEYSW lpSerialKeysW = (LPSERIALKEYSW)pvParam;
        WARN("SPI_GETSERIALKEYS not fully implemented\n");
        if (lpSerialKeysW && lpSerialKeysW->cbSize == sizeof(SERIALKEYSW))
        {
            /* Indicate that no SerialKeys feature available */
            lpSerialKeysW->dwFlags = 0;
            lpSerialKeysW->lpszActivePort = NULL;
            lpSerialKeysW->lpszPort = NULL;
            lpSerialKeysW->iBaudRate = 0;
            lpSerialKeysW->iPortState = 0;
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
        LPSOUNDSENTRYW lpSoundSentryW = (LPSOUNDSENTRYW)pvParam;
        WARN("SPI_GETSOUNDSENTRY not fully implemented\n");
        if (lpSoundSentryW && lpSoundSentryW->cbSize == sizeof(SOUNDSENTRYW))
        {
            /* Indicate that no SoundSentry feature available */
            lpSoundSentryW->dwFlags = 0;
            lpSoundSentryW->iFSTextEffect = 0;
            lpSoundSentryW->iFSTextEffectMSec = 0;
            lpSoundSentryW->iFSTextEffectColorBits = 0;
            lpSoundSentryW->iFSGrafEffect = 0;
            lpSoundSentryW->iFSGrafEffectMSec = 0;
            lpSoundSentryW->iFSGrafEffectColor = 0;
            lpSoundSentryW->iWindowsEffect = 0;
            lpSoundSentryW->iWindowsEffectMSec = 0;
            lpSoundSentryW->lpszWindowsEffectDLL = 0;
            lpSoundSentryW->iWindowsEffectOrdinal = 0;
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
	LPHIGHCONTRASTW lpHighContrastW = (LPHIGHCONTRASTW)pvParam;
	WARN("SPI_GETHIGHCONTRAST not fully implemented\n");
	if (lpHighContrastW && lpHighContrastW->cbSize == sizeof(HIGHCONTRASTW))
	{
	    /* Indicate that no high contrast feature available */
	    lpHighContrastW->dwFlags = 0;
	    lpHighContrastW->lpszDefaultScheme = NULL;
	}
	else
	{
	    ret = FALSE;
	}
	break;
    }
    WINE_SPI_FIXME(SPI_SETHIGHCONTRAST);	/*     67  WINVER >= 0x400 */

    case SPI_GETKEYBOARDPREF:                   /*     68  WINVER >= 0x400 */
        if (!pvParam) return FALSE;

        spi_idx = SPI_SETKEYBOARDPREF_IDX;
        if (!spi_loaded[spi_idx])
        {
            WCHAR buf[5];

            if (SYSPARAMS_Load( SPI_SETKEYBOARDPREF_REGKEY,
                                SPI_SETKEYBOARDPREF_VALNAME, buf, sizeof(buf) ))
                keyboard_pref  = atoiW(buf);
            spi_loaded[spi_idx] = TRUE;
        }

	*(BOOL *)pvParam = keyboard_pref;
        break;

    case SPI_SETKEYBOARDPREF:                   /*     69  WINVER >= 0x400 */
    {
        WCHAR buf[5];

        spi_idx = SPI_SETKEYBOARDPREF_IDX;
        wsprintfW(buf, CSu, uiParam);
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
        if (!pvParam) return FALSE;

        spi_idx = SPI_SETSCREENREADER_IDX;
        if (!spi_loaded[spi_idx])
        {
            WCHAR buf[5];

            if (SYSPARAMS_Load( SPI_SETSCREENREADER_REGKEY,
                                SPI_SETSCREENREADER_VALNAME, buf, sizeof(buf) ))
                screen_reader  = atoiW(buf);
            spi_loaded[spi_idx] = TRUE;
        }

	*(BOOL *)pvParam = screen_reader;
        break;

    case SPI_SETSCREENREADER:                   /*     71  WINVER >= 0x400 */
    {
        WCHAR buf[5];

        spi_idx = SPI_SETSCREENREADER_IDX;
        wsprintfW(buf, CSu, uiParam);
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
	if (lpAnimInfo && lpAnimInfo->cbSize == sizeof(ANIMATIONINFO))
	    lpAnimInfo->iMinAnimate = 0; /* Minimise and restore animation is disabled (nonzero == enabled) */
	else
	    ret = FALSE;
	break;
    }
    WINE_SPI_WARN(SPI_SETANIMATION);		/*     73  WINVER >= 0x400 */

    case SPI_GETFONTSMOOTHING:    /*     74  WINVER >= 0x400 */
        if (!pvParam) return FALSE;

        spi_idx = SPI_SETFONTSMOOTHING_IDX;
        if (!spi_loaded[spi_idx])
        {
            WCHAR buf[5];

            if (SYSPARAMS_Load( SPI_SETFONTSMOOTHING_REGKEY,
                                SPI_SETFONTSMOOTHING_VALNAME, buf, sizeof(buf) ))
            {
                spi_loaded[spi_idx] = TRUE;
                if (buf[0] == 0x01 || buf[0] == 0x02) /* 0x01 for 95/98/NT, 0x02 for 98/ME/2k/XP */
                {
                    font_smoothing = TRUE;
                }
            }
        }

	*(BOOL *)pvParam = font_smoothing;

        break;

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
        WCHAR buf[5];

        spi_idx = SPI_SETSCREENSAVERRUNNING_IDX;
        wsprintfW(buf, CSu, uiParam);

        /* save only temporarily */
        if (SYSPARAMS_Save( SPI_SETSCREENSAVERRUNNING_REGKEY,
                            SPI_SETSCREENSAVERRUNNING_VALNAME,
                            buf, 0  ))
        {
            screensaver_running = uiParam;
            spi_loaded[spi_idx] = TRUE;
        }
        else
            ret = FALSE;
        break;
    }

    case SPI_GETMOUSEHOVERWIDTH:		/*     98  _WIN32_WINNT >= 0x400 || _WIN32_WINDOW > 0x400 */
        if (!pvParam) return FALSE;

        spi_idx = SPI_SETMOUSEHOVERWIDTH_IDX;
        if (!spi_loaded[spi_idx])
        {
            WCHAR buf[10];

            if (SYSPARAMS_Load( SPI_SETMOUSEHOVERWIDTH_REGKEY,
                                SPI_SETMOUSEHOVERWIDTH_VALNAME,
                                buf, sizeof(buf) ))
                mouse_hover_width = atoiW( buf );
            spi_loaded[spi_idx] = TRUE;
        }
	*(INT *)pvParam = mouse_hover_width;
	break;
	
    case SPI_SETMOUSEHOVERWIDTH:		/*     99  _WIN32_WINNT >= 0x400 || _WIN32_WINDOW > 0x400 */
    {
        WCHAR buf[10];

        spi_idx = SPI_SETMOUSEHOVERWIDTH_IDX;
        wsprintfW(buf, CSu, uiParam);

        if (SYSPARAMS_Save( SPI_SETMOUSEHOVERWIDTH_REGKEY,
                            SPI_SETMOUSEHOVERWIDTH_VALNAME,
                            buf, fWinIni ))
        {
            mouse_hover_width = uiParam;
            spi_loaded[spi_idx] = TRUE;
        }
        else
            ret = FALSE;
        break;
    }
	
    case SPI_GETMOUSEHOVERHEIGHT:		/*    100  _WIN32_WINNT >= 0x400 || _WIN32_WINDOW > 0x400 */
        if (!pvParam) return FALSE;

        spi_idx = SPI_SETMOUSEHOVERHEIGHT_IDX;
        if (!spi_loaded[spi_idx])
        {
            WCHAR buf[10];

            if (SYSPARAMS_Load( SPI_SETMOUSEHOVERHEIGHT_REGKEY,
                                SPI_SETMOUSEHOVERHEIGHT_VALNAME,
                                buf, sizeof(buf) ))
                mouse_hover_height = atoiW( buf );

            spi_loaded[spi_idx] = TRUE;
        }
	*(INT *)pvParam = mouse_hover_height;
	break;

    case SPI_SETMOUSEHOVERHEIGHT:		/*    101  _WIN32_WINNT >= 0x400 || _WIN32_WINDOW > 0x400 */
    {
        WCHAR buf[10];

        spi_idx = SPI_SETMOUSEHOVERHEIGHT_IDX;
        wsprintfW(buf, CSu, uiParam);

        if (SYSPARAMS_Save( SPI_SETMOUSEHOVERHEIGHT_REGKEY,
                            SPI_SETMOUSEHOVERHEIGHT_VALNAME,
                            buf, fWinIni ))
        {
            mouse_hover_height = uiParam;
            spi_loaded[spi_idx] = TRUE;
        }
        else
            ret = FALSE;
        break;
    }
    
    case SPI_GETMOUSEHOVERTIME:			/*    102  _WIN32_WINNT >= 0x400 || _WIN32_WINDOW > 0x400 */
        if (!pvParam) return FALSE;

        spi_idx = SPI_SETMOUSEHOVERTIME_IDX;
        if (!spi_loaded[spi_idx])
        {
            WCHAR buf[10];

            if (SYSPARAMS_Load( SPI_SETMOUSEHOVERTIME_REGKEY,
                                SPI_SETMOUSEHOVERTIME_VALNAME,
                                buf, sizeof(buf) ))
                mouse_hover_time = atoiW( buf );

            spi_loaded[spi_idx] = TRUE;
        }
	*(INT *)pvParam = mouse_hover_time;
	break;

    case SPI_SETMOUSEHOVERTIME:			/*    103  _WIN32_WINNT >= 0x400 || _WIN32_WINDOW > 0x400 */
    {
        WCHAR buf[10];

        spi_idx = SPI_SETMOUSEHOVERTIME_IDX;
        wsprintfW(buf, CSu, uiParam);

        if (SYSPARAMS_Save( SPI_SETMOUSEHOVERTIME_REGKEY,
                            SPI_SETMOUSEHOVERTIME_VALNAME,
                            buf, fWinIni ))
        {
            mouse_hover_time = uiParam;
            spi_loaded[spi_idx] = TRUE;
        }
        else
            ret = FALSE;
        break;
    }
    
    case SPI_GETWHEELSCROLLLINES:		/*    104  _WIN32_WINNT >= 0x400 || _WIN32_WINDOW > 0x400 */
        if (!pvParam) return FALSE;

        spi_idx = SPI_SETMOUSESCROLLLINES_IDX;
        if (!spi_loaded[spi_idx])
        {
            WCHAR buf[10];

            if (SYSPARAMS_Load( SPI_SETMOUSESCROLLLINES_REGKEY,
                                SPI_SETMOUSESCROLLLINES_VALNAME,
                                buf, sizeof(buf) ))
                mouse_scroll_lines = atoiW( buf );

            spi_loaded[spi_idx] = TRUE;
        }
	*(INT *)pvParam = mouse_scroll_lines;
	break;

    case SPI_SETWHEELSCROLLLINES:		/*    105  _WIN32_WINNT >= 0x400 || _WIN32_WINDOW > 0x400 */
    {
        WCHAR buf[10];

        spi_idx = SPI_SETMOUSESCROLLLINES_IDX;
        wsprintfW(buf, CSu, uiParam);

        if (SYSPARAMS_Save( SPI_SETMOUSESCROLLLINES_REGKEY,
                            SPI_SETMOUSESCROLLLINES_VALNAME,
                            buf, fWinIni ))
        {
            mouse_scroll_lines = uiParam;
            spi_loaded[spi_idx] = TRUE;
        }
        else
            ret = FALSE;
        break;
    }
    
    case SPI_GETMENUSHOWDELAY:			/*    106  _WIN32_WINNT >= 0x400 || _WIN32_WINDOW > 0x400 */
        if (!pvParam) return FALSE;

        spi_idx = SPI_SETMENUSHOWDELAY_IDX;
        if (!spi_loaded[spi_idx])
        {
            WCHAR buf[10];

            if (SYSPARAMS_Load( SPI_SETMENUSHOWDELAY_REGKEY,
                                SPI_SETMENUSHOWDELAY_VALNAME,
                                buf, sizeof(buf) ))
                menu_show_delay = atoiW( buf );

            spi_loaded[spi_idx] = TRUE;
        }
	*(INT *)pvParam = menu_show_delay;
	break;

    case SPI_SETMENUSHOWDELAY:			/*    107  _WIN32_WINNT >= 0x400 || _WIN32_WINDOW > 0x400 */
    {
        WCHAR buf[10];

        spi_idx = SPI_SETMENUSHOWDELAY_IDX;
        wsprintfW(buf, CSu, uiParam);

        if (SYSPARAMS_Save( SPI_SETMENUSHOWDELAY_REGKEY,
                            SPI_SETMENUSHOWDELAY_VALNAME,
                            buf, fWinIni ))
        {
            menu_show_delay = uiParam;
            spi_loaded[spi_idx] = TRUE;
        }
        else
            ret = FALSE;
        break;
    }
    
    WINE_SPI_FIXME(SPI_GETSHOWIMEUI);		/*    110  _WIN32_WINNT >= 0x400 || _WIN32_WINDOW > 0x400 */
    WINE_SPI_FIXME(SPI_SETSHOWIMEUI);		/*    111  _WIN32_WINNT >= 0x400 || _WIN32_WINDOW > 0x400 */
    WINE_SPI_FIXME(SPI_GETMOUSESPEED);          /*    112  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
    WINE_SPI_FIXME(SPI_SETMOUSESPEED);          /*    113  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */

    case SPI_GETSCREENSAVERRUNNING:             /*    114  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
        if (!pvParam) return FALSE;

        spi_idx = SPI_SETSCREENSAVERRUNNING_IDX;
        if (!spi_loaded[spi_idx])
        {
            WCHAR buf[5];

            if (SYSPARAMS_Load( SPI_SETSCREENSAVERRUNNING_REGKEY,
                                SPI_SETSCREENSAVERRUNNING_VALNAME, buf, sizeof(buf) ))
                screensaver_running  = atoiW( buf );
            spi_loaded[spi_idx] = TRUE;
        }

	*(BOOL *)pvParam = screensaver_running;
        break;

    case SPI_GETDESKWALLPAPER:                  /*    115  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
    {
	WCHAR buf[MAX_PATH];

        if (!pvParam) return FALSE;

        if (uiParam > MAX_PATH)
	{
	    uiParam = MAX_PATH;
	}

        if (SYSPARAMS_Load(SPI_SETDESKWALLPAPER_REGKEY, SPI_SETDESKWALLPAPER_VALNAME, buf, sizeof(buf)))
	{
	    lstrcpynW((WCHAR*)pvParam, buf, uiParam);
	}
	else
	{
	    /* Return an empty string */
	    memset((WCHAR*)pvParam, 0, uiParam);
	}

	break;
    }

    WINE_SPI_FIXME(SPI_GETACTIVEWINDOWTRACKING);/* 0x1000  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
    WINE_SPI_FIXME(SPI_SETACTIVEWINDOWTRACKING);/* 0x1001  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
    WINE_SPI_FIXME(SPI_GETMENUANIMATION);       /* 0x1002  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
    WINE_SPI_FIXME(SPI_SETMENUANIMATION);       /* 0x1003  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
    WINE_SPI_FIXME(SPI_GETCOMBOBOXANIMATION);   /* 0x1004  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
    WINE_SPI_FIXME(SPI_SETCOMBOBOXANIMATION);   /* 0x1005  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */

    case SPI_GETLISTBOXSMOOTHSCROLLING:    /* 0x1006  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
        if (!pvParam) return FALSE;

        spi_idx = SPI_SETLISTBOXSMOOTHSCROLLING_IDX;
        if (!spi_loaded[spi_idx])
        {
            WCHAR buf[5];

            if (SYSPARAMS_Load( SPI_SETLISTBOXSMOOTHSCROLLING_REGKEY,
                                SPI_SETLISTBOXSMOOTHSCROLLING_VALNAME, buf, sizeof(buf) ))
            {
                if ((buf[0]&0x01) == 0x01)
                {
                    listbox_smoothscrolling = TRUE;
                }
            }
            spi_loaded[spi_idx] = TRUE;
        }

        *(BOOL *)pvParam = listbox_smoothscrolling;

        break;

    WINE_SPI_FIXME(SPI_SETLISTBOXSMOOTHSCROLLING);/* 0x1007  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */

    case SPI_GETGRADIENTCAPTIONS:    /* 0x1008  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
        if (!pvParam) return FALSE;

        spi_idx = SPI_SETGRADIENTCAPTIONS_IDX;
        if (!spi_loaded[spi_idx])
        {
            WCHAR buf[5];

            if (SYSPARAMS_Load( SPI_USERPREFERENCEMASK_REGKEY,
                                SPI_USERPREFERENCEMASK_VALNAME, buf, sizeof(buf) ))
            {
                if ((buf[0]&0x10) == 0x10)
                {
                    gradient_captions = TRUE;
                }
            }
            spi_loaded[spi_idx] = TRUE;
        }

        *(BOOL *)pvParam = gradient_captions;

        break;

    WINE_SPI_FIXME(SPI_SETGRADIENTCAPTIONS);    /* 0x1009  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */

    case SPI_GETKEYBOARDCUES:     /* 0x100A  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
        if (!pvParam) return FALSE;

        spi_idx = SPI_SETKEYBOARDCUES_IDX;
        if (!spi_loaded[spi_idx])
        {
            WCHAR buf[5];

            if (SYSPARAMS_Load( SPI_USERPREFERENCEMASK_REGKEY,
                                SPI_USERPREFERENCEMASK_VALNAME, buf, sizeof(buf) ))
            {
                if ((buf[0]&0x20) == 0x20)
                {
                    keyboard_cues = TRUE;
                }
            }
            spi_loaded[spi_idx] = TRUE;
        }

        *(BOOL *)pvParam = keyboard_cues;

        break;

    WINE_SPI_FIXME(SPI_SETKEYBOARDCUES);        /* 0x100B  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
    WINE_SPI_FIXME(SPI_GETACTIVEWNDTRKZORDER);  /* 0x100C  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
    WINE_SPI_FIXME(SPI_SETACTIVEWNDTRKZORDER);  /* 0x100D  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
    case SPI_GETHOTTRACKING:    /* 0x100E  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
        if (!pvParam) return FALSE;

        spi_idx = SPI_SETHOTTRACKING_IDX;
        if (!spi_loaded[spi_idx])
        {
            WCHAR buf[5];

            if (SYSPARAMS_Load( SPI_USERPREFERENCEMASK_REGKEY,
                                SPI_USERPREFERENCEMASK_VALNAME, buf, sizeof(buf) ))
            {
                if ((buf[0]&0x80) == 0x80)
                {
                    hot_tracking = TRUE;
                }
            }
            spi_loaded[spi_idx] = TRUE;
        }

        *(BOOL *)pvParam = hot_tracking;

        break;

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
            if (!MultiByteToWideChar( CP_ACP, 0, (LPSTR)pvParam, -1,
                                      buffer, sizeof(buffer)/sizeof(WCHAR) ))
                buffer[sizeof(buffer)/sizeof(WCHAR)-1] = 0;
	ret = SystemParametersInfoW( uiAction, uiParam, pvParam ? buffer : NULL, fuWinIni );
	break;
    }

    case SPI_GETICONTITLELOGFONT:		/*     31 */
    {
	LOGFONTW tmp;
	ret = SystemParametersInfoW( uiAction, uiParam, pvParam ? &tmp : NULL, fuWinIni );
	if (ret && pvParam)
	    SYSPARAMS_LogFont32WTo32A( &tmp, (LPLOGFONTA)pvParam );
	break;
    }

    case SPI_GETNONCLIENTMETRICS: 		/*     41  WINVER >= 0x400 */
    {
	NONCLIENTMETRICSW tmp;
	LPNONCLIENTMETRICSA lpnmA = (LPNONCLIENTMETRICSA)pvParam;
	if (lpnmA && lpnmA->cbSize == sizeof(NONCLIENTMETRICSA))
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

    case SPI_GETICONMETRICS:			/*     45  WINVER >= 0x400 */
    {
	ICONMETRICSW tmp;
	LPICONMETRICSA lpimA = (LPICONMETRICSA)pvParam;
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

    case SPI_GETHIGHCONTRAST:			/*     66  WINVER >= 0x400 */
    {
	HIGHCONTRASTW tmp;
	LPHIGHCONTRASTA lphcA = (LPHIGHCONTRASTA)pvParam;
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

    default:
        ret = SystemParametersInfoW( uiAction, uiParam, pvParam, fuWinIni );
        break;
    }
    return ret;
}


/**********************************************************************
 *		SetDoubleClickTime (USER32.@)
 */
BOOL WINAPI SetDoubleClickTime( UINT interval )
{
    return SystemParametersInfoW(SPI_SETDOUBLECLICKTIME, interval, 0, 0);
}


/**********************************************************************
 *		GetDoubleClickTime (USER32.@)
 */
UINT WINAPI GetDoubleClickTime(void)
{
    WCHAR buf[10];

    if (!spi_loaded[SPI_SETDOUBLECLICKTIME_IDX])
    {
        if (SYSPARAMS_Load( SPI_SETDOUBLECLICKTIME_REGKEY,
                            SPI_SETDOUBLECLICKTIME_VALNAME,
                            buf, sizeof(buf) ))
        {
            double_click_time = atoiW( buf );
            if (!double_click_time) double_click_time = 500;
        }
        spi_loaded[SPI_SETDOUBLECLICKTIME_IDX] = TRUE;
    }
    return double_click_time;
}
