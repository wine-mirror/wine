/*
 * System parameters functions
 *
 * Copyright 1994 Alexandre Julliard
 */

#include "config.h"

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
#include "debugtools.h"
#include "sysmetrics.h"

DEFAULT_DEBUG_CHANNEL(system);

static BOOL beep_active = TRUE;

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
 *		SystemParametersInfoA (USER32.@)
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
    
    TRACE("(%u, %u, %p, %u)\n", uiAction, uiParam, pvParam, fWinIni);
    
    switch (uiAction)
    {
    case SPI_GETBEEP:				/*      1 */
	*(BOOL *)pvParam = beep_active;
        break;
    case SPI_SETBEEP:				/*      2 */
	beep_active = uiParam;
	break;

    WINE_SPI_FIXME(SPI_GETMOUSE);		/*      3 */
    WINE_SPI_FIXME(SPI_SETMOUSE);		/*      4 */

    case SPI_GETBORDER:				/*      5 */
	*(INT *)pvParam = GetSystemMetrics( SM_CXFRAME );
	break;
    WINE_SPI_WARN(SPI_SETBORDER);		/*      6 */

    case SPI_GETKEYBOARDSPEED:			/*     10 */
	*(DWORD *)pvParam = GetProfileIntA( "keyboard", "KeyboardSpeed", 30 );
	break;
    WINE_SPI_WARN(SPI_SETKEYBOARDSPEED);	/*     11 */

    WINE_SPI_WARN(SPI_LANGDRIVER);		/*     12 */

    case SPI_ICONHORIZONTALSPACING:		/*     13 */
	/* FIXME Get/SetProfileInt */
	if (pvParam == NULL)
	    /*SetSystemMetrics( SM_CXICONSPACING, uiParam )*/ ;
	else
	    *(INT *)pvParam = GetSystemMetrics( SM_CXICONSPACING );
	break;

    case SPI_GETSCREENSAVETIMEOUT:		/*     14 */
    {
	int	timeout;
	timeout = USER_Driver.pGetScreenSaveTimeout();
	if (!timeout)
	    timeout = GetProfileIntA( "windows", "ScreenSaveTimeout", 300 );
	*(INT *)pvParam = timeout;
	break;
    }
    case SPI_SETSCREENSAVETIMEOUT:		/*     15 */
	USER_Driver.pSetScreenSaveTimeout( uiParam );
	break;
	
    case SPI_GETSCREENSAVEACTIVE:		/*     16 */
	if (USER_Driver.pGetScreenSaveActive() ||
	    GetProfileIntA( "windows", "ScreenSaveActive", 1 ) == 1)
	    *(BOOL *)pvParam = TRUE;
	else
	    *(BOOL *)pvParam = FALSE;
	break;
    case SPI_SETSCREENSAVEACTIVE:		/*     17 */
	USER_Driver.pSetScreenSaveActive( uiParam );
	break;
	
    case SPI_GETGRIDGRANULARITY:		/*     18 */
	*(INT *)pvParam = GetProfileIntA( "desktop", "GridGranularity", 1 );
	break;
    WINE_SPI_FIXME(SPI_SETGRIDGRANULARITY);	/*     19 */

    case SPI_SETDESKWALLPAPER:			/*     20 */
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

    case SPI_GETKEYBOARDDELAY:			/*     22 */
	*(INT *)pvParam = GetProfileIntA( "keyboard", "KeyboardDelay", 1 );
	break;
    WINE_SPI_WARN(SPI_SETKEYBOARDDELAY);	/*     23 */

    case SPI_ICONVERTICALSPACING:		/*     24 */
	/* FIXME Get/SetProfileInt */
	if (pvParam == NULL)
	    /*SetSystemMetrics( SM_CYICONSPACING, uiParam )*/ ;
	else
	    *(INT *)pvParam = GetSystemMetrics( SM_CYICONSPACING );
	break;

    case SPI_GETICONTITLEWRAP:			/*     25 */
	*(BOOL *)pvParam = GetProfileIntA( "desktop", "IconTitleWrap", TRUE );
	break;
    WINE_SPI_FIXME(SPI_SETICONTITLEWRAP);	/*     26 */

    case SPI_GETMENUDROPALIGNMENT:		/*     27 */
	*(BOOL *)pvParam = GetSystemMetrics( SM_MENUDROPALIGNMENT ); /* XXX check this */
	break;
    WINE_SPI_FIXME(SPI_SETMENUDROPALIGNMENT);	/*     28 */

    WINE_SPI_WARN(SPI_SETDOUBLECLKWIDTH);	/*     29 */
    WINE_SPI_WARN(SPI_SETDOUBLECLKHEIGHT);	/*     30 */

    case SPI_GETICONTITLELOGFONT:		/*     31 */
    {
	LPLOGFONTA lpLogFont = (LPLOGFONTA)pvParam;

	/* from now on we always have an alias for MS Sans Serif */

	GetProfileStringA( "Desktop", "IconTitleFaceName", "MS Sans Serif", 
			   lpLogFont->lfFaceName, LF_FACESIZE );
	lpLogFont->lfHeight = -GetProfileIntA( "Desktop", "IconTitleSize", 13 );
	lpLogFont->lfWidth = 0;
	lpLogFont->lfEscapement = lpLogFont->lfOrientation = 0;
	lpLogFont->lfWeight = FW_NORMAL;
	lpLogFont->lfItalic = FALSE;
	lpLogFont->lfStrikeOut = FALSE;
	lpLogFont->lfUnderline = FALSE;
	lpLogFont->lfCharSet = ANSI_CHARSET;
	lpLogFont->lfOutPrecision = OUT_DEFAULT_PRECIS;
	lpLogFont->lfClipPrecision = CLIP_DEFAULT_PRECIS;
	lpLogFont->lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;
	break;
    }

    WINE_SPI_WARN(SPI_SETDOUBLECLICKTIME);	/*     32 */
    
    WINE_SPI_FIXME(SPI_SETMOUSEBUTTONSWAP);	/*     33 */
    WINE_SPI_FIXME(SPI_SETICONTITLELOGFONT);	/*     34 */

    case SPI_GETFASTTASKSWITCH:			/*     35 */
	if (GetProfileIntA( "windows", "CoolSwitch", 1 ) == 1)
	    *(BOOL *)pvParam = TRUE;
	else
	    *(BOOL *)pvParam = FALSE;
	break;
    WINE_SPI_WARN(SPI_SETFASTTASKSWITCH);	/*     36 */

    WINE_SPI_WARN(SPI_SETDRAGFULLWINDOWS);	/*     37  WINVER >= 0x0400 */
    case SPI_GETDRAGFULLWINDOWS:		/*     38  WINVER >= 0x0400 */
    {
	HKEY hKey;
	char buffer[20];
	DWORD dwBufferSize = sizeof(buffer);

	*(BOOL *)pvParam = FALSE;
	if (RegOpenKeyExA( HKEY_CURRENT_USER,
			   "Control Panel\\desktop",
			   0, KEY_QUERY_VALUE, &hKey ) == ERROR_SUCCESS)
	{
	    if (RegQueryValueExA( hKey, "DragFullWindows", NULL,
				  0, buffer, &dwBufferSize ) == ERROR_SUCCESS)
		*(BOOL *)pvParam = atoi( buffer ) != 0;
	    
	    RegCloseKey( hKey );
	}
	break;
    }

    case SPI_GETNONCLIENTMETRICS: 		/*     41  WINVER >= 0x400 */
    {
	LPNONCLIENTMETRICSA lpnm = (LPNONCLIENTMETRICSA)pvParam;
		
	if (lpnm->cbSize == sizeof(NONCLIENTMETRICSA))
	{
	    LPLOGFONTA lpLogFont = &(lpnm->lfMenuFont);
	    
	    /* clear the struct, so we have 'sane' members */
	    memset(
		(char *)pvParam + sizeof(lpnm->cbSize),
		0,
		lpnm->cbSize - sizeof(lpnm->cbSize)
		);

	    /* FIXME: initialize geometry entries */
	    /* FIXME: As these values are presumably in device units,
	     *  we should calculate the defaults based on the screen dpi
	     */
	    /* caption */
	    lpnm->iCaptionWidth = ((TWEAK_WineLook > WIN31_LOOK)  ? 32 : 20);
	    lpnm->iCaptionHeight = lpnm->iCaptionWidth;
	    SystemParametersInfoA( SPI_GETICONTITLELOGFONT, 0, (LPVOID)&(lpnm->lfCaptionFont), 0 );
	    lpnm->lfCaptionFont.lfWeight = FW_BOLD;

	    /* small caption */
	    lpnm->iSmCaptionWidth = ((TWEAK_WineLook > WIN31_LOOK)  ? 32 : 17);
	    lpnm->iSmCaptionHeight = lpnm->iSmCaptionWidth;
	    SystemParametersInfoA( SPI_GETICONTITLELOGFONT, 0, (LPVOID)&(lpnm->lfSmCaptionFont), 0 );

	    /* menus, FIXME: names of wine.conf entries are bogus */

	    lpnm->iMenuWidth = GetProfileIntA( "Desktop", "MenuWidth", 13 );	/* size of the menu buttons*/
	    lpnm->iMenuHeight = GetProfileIntA( "Desktop", "MenuHeight", 
						(TWEAK_WineLook > WIN31_LOOK) ? 13 : 27 );

	    GetProfileStringA( "Desktop", "MenuFont", 
			       (TWEAK_WineLook > WIN31_LOOK) ? "MS Sans Serif": "System", 
			       lpLogFont->lfFaceName, LF_FACESIZE );

	    lpLogFont->lfHeight = -GetProfileIntA( "Desktop", "MenuFontSize", 13 );
	    lpLogFont->lfWidth = 0;
	    lpLogFont->lfEscapement = lpLogFont->lfOrientation = 0;
	    lpLogFont->lfWeight = (TWEAK_WineLook > WIN31_LOOK) ? FW_NORMAL : FW_BOLD;
	    lpLogFont->lfItalic = FALSE;
	    lpLogFont->lfStrikeOut = FALSE;
	    lpLogFont->lfUnderline = FALSE;
	    lpLogFont->lfCharSet = ANSI_CHARSET;
	    lpLogFont->lfOutPrecision = OUT_DEFAULT_PRECIS;
	    lpLogFont->lfClipPrecision = CLIP_DEFAULT_PRECIS;
	    lpLogFont->lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;

	    SystemParametersInfoA( SPI_GETICONTITLELOGFONT, 0,
				   (LPVOID)&(lpnm->lfStatusFont), 0 );
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

    WINE_SPI_FIXME(SPI_SETWORKAREA);		/*     47  WINVER >= 0x400 */
    case SPI_GETWORKAREA:			/*     48  WINVER >= 0x400 */
	SetRect( (RECT *)pvParam, 0, 0,
		 GetSystemMetrics( SM_CXSCREEN ),
		 GetSystemMetrics( SM_CYSCREEN ) );
	break;
    
    WINE_SPI_FIXME(SPI_SETPENWINDOWS);		/*     49  WINVER >= 0x400 */

    WINE_SPI_FIXME(SPI_GETFILTERKEYS);		/*     50 */
    WINE_SPI_FIXME(SPI_SETFILTERKEYS);		/*     51 */
    WINE_SPI_FIXME(SPI_GETTOGGLEKEYS);		/*     52 */
    WINE_SPI_FIXME(SPI_SETTOGGLEKEYS);		/*     53 */
    WINE_SPI_FIXME(SPI_GETMOUSEKEYS);		/*     54 */
    WINE_SPI_FIXME(SPI_SETMOUSEKEYS);		/*     55 */
    case SPI_GETSHOWSOUNDS:			/*     56 */
        *(INT *)pvParam = GetSystemMetrics( SM_SHOWSOUNDS );
        break;
    case SPI_SETSHOWSOUNDS:			/*     57 */
        SYSMETRICS_Set(SM_SHOWSOUNDS, uiParam);
        break;
    WINE_SPI_FIXME(SPI_GETSTICKYKEYS);		/*     58 */
    WINE_SPI_FIXME(SPI_SETSTICKYKEYS);		/*     59 */
    WINE_SPI_FIXME(SPI_GETACCESSTIMEOUT);	/*     60 */
    WINE_SPI_FIXME(SPI_SETACCESSTIMEOUT);	/*     61 */

    WINE_SPI_FIXME(SPI_GETSERIALKEYS);		/*     62  WINVER >= 0x400 */
    WINE_SPI_FIXME(SPI_SETSERIALKEYS);		/*     63  WINVER >= 0x400 */

    WINE_SPI_FIXME(SPI_GETSOUNDSENTRY);		/*     64 */
    WINE_SPI_FIXME(SPI_SETSOUNDSENTRY);		/*     65 */
    
    case SPI_GETHIGHCONTRAST:			/*     66  WINVER >= 0x400 */
    {
	LPHIGHCONTRASTA lpHighContrastA = (LPHIGHCONTRASTA)pvParam;
	WARN("SPI_GETHIGHCONTRAST not fully implemented\n");
	if (lpHighContrastA->cbSize == sizeof(HIGHCONTRASTA))
	{
	    /* Indicate that there is no high contrast available */
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

    WINE_SPI_FIXME(SPI_GETKEYBOARDPREF);	/*     68  WINVER >= 0x400 */
    WINE_SPI_FIXME(SPI_SETKEYBOARDPREF);	/*     69  WINVER >= 0x400 */

    case SPI_GETSCREENREADER:
    {
       LPBOOL	bool = (LPBOOL)pvParam;
       *bool = FALSE;
       break;
    }

    WINE_SPI_FIXME(SPI_SETSCREENREADER);	/*     71  WINVER >= 0x400 */

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

    WINE_SPI_FIXME(SPI_GETDEFAULTINPUTLANG);	/*     89  WINVER >= 0x400 */
    WINE_SPI_FIXME(SPI_SETDEFAULTINPUTLANG);	/*     90  WINVER >= 0x400 */

    WINE_SPI_FIXME(SPI_SETLANGTOGGLE);		/*     91  WINVER >= 0x400 */

    case SPI_GETWINDOWSEXTENSION:		/*     92  WINVER >= 0x400 */
	WARN("pretend no support for Win9x Plus! for now.\n");
	ret = FALSE; /* yes, this is the result value */
	break;

    WINE_SPI_FIXME(SPI_SETMOUSETRAILS);		/*     93  WINVER >= 0x400 */
    WINE_SPI_FIXME(SPI_GETMOUSETRAILS);		/*     94  WINVER >= 0x400 */
	
    WINE_SPI_FIXME(SPI_SETSCREENSAVERRUNNING);	/*     97  WINVER >= 0x400 */
    /* SPI_SCREENSAVERRUNNING is an alias for SPI_SETSCREENSAVERRUNNING */

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

    default:
	FIXME( "Unknown action: %u\n", uiAction );
	SetLastError( ERROR_INVALID_SPI_VALUE );
	ret = FALSE;
	break;
    }

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
    }

    return ret;
}
