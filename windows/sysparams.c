/*
 * System parameters functions
 *
 * Copyright 1994 Alexandre Julliard
 */

#include "config.h"

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/winuser16.h"
#include "winerror.h"

#include "keyboard.h"
#include "tweak.h"
#include "user.h"
#include "desktop.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(system);

/***********************************************************************
 *	GetTimerResolution (USER.14)
 */
LONG WINAPI GetTimerResolution16(void)
{
	return (1000);
}

/***********************************************************************
 *	SystemParametersInfoA   (USER32.540)
 */
BOOL WINAPI SystemParametersInfoA( UINT uAction, UINT uParam,
                                       LPVOID lpvParam, UINT fuWinIni )
{
	int timeout;

	switch (uAction) {
	case SPI_GETBEEP:
	        *(BOOL *) lpvParam = KEYBOARD_GetBeepActive();
		break;
	case SPI_SETBEEP:
	       KEYBOARD_SetBeepActive(uParam);
	       break;

	case SPI_GETBORDER:
		*(INT *)lpvParam = GetSystemMetrics( SM_CXFRAME );
		break;

	case SPI_GETDRAGFULLWINDOWS:
		*(BOOL *) lpvParam = FALSE;
		break;

	case SPI_SETDRAGFULLWINDOWS:
		break;

	case SPI_GETFASTTASKSWITCH:
		if ( GetProfileIntA( "windows", "CoolSwitch", 1 ) == 1 )
			*(BOOL *) lpvParam = TRUE;
		else
			*(BOOL *) lpvParam = FALSE;
		break;
		
	case SPI_GETGRIDGRANULARITY:
		*(INT*)lpvParam=GetProfileIntA("desktop","GridGranularity",1);
		break;

	case SPI_GETICONTITLEWRAP:
		*(BOOL*)lpvParam=GetProfileIntA("desktop","IconTitleWrap",TRUE);
		break;

	case SPI_GETKEYBOARDDELAY:
		*(INT*)lpvParam=GetProfileIntA("keyboard","KeyboardDelay",1);
		break;

	case SPI_GETKEYBOARDSPEED:
		*(DWORD*)lpvParam=GetProfileIntA("keyboard","KeyboardSpeed",30);
		break;

	case SPI_GETMENUDROPALIGNMENT:
		*(BOOL*)lpvParam=GetSystemMetrics(SM_MENUDROPALIGNMENT); /* XXX check this */
		break;

	case SPI_GETSCREENSAVEACTIVE:	        
	       if(USER_Driver->pGetScreenSaveActive() || 
		  GetProfileIntA( "windows", "ScreenSaveActive", 1 ) == 1)
	        	*(BOOL*)lpvParam = TRUE;
		else
			*(BOOL*)lpvParam = FALSE;
		break;

	case SPI_GETSCREENSAVETIMEOUT:
	        timeout = USER_Driver->pGetScreenSaveTimeout();
		if(!timeout)
			timeout = GetProfileIntA( "windows", "ScreenSaveTimeout", 300 );
		*(INT *) lpvParam = timeout * 1000;
		break;

	case SPI_ICONHORIZONTALSPACING:
		/* FIXME Get/SetProfileInt */
		if (lpvParam == NULL)
			/*SetSystemMetrics( SM_CXICONSPACING, uParam )*/ ;
		else
			*(INT*)lpvParam=GetSystemMetrics(SM_CXICONSPACING);
		break;

	case SPI_ICONVERTICALSPACING:
		/* FIXME Get/SetProfileInt */
		if (lpvParam == NULL)
			/*SetSystemMetrics( SM_CYICONSPACING, uParam )*/ ;
		else
			*(INT*)lpvParam=GetSystemMetrics(SM_CYICONSPACING);
		break;

	case SPI_GETICONTITLELOGFONT: {
		LPLOGFONTA lpLogFont = (LPLOGFONTA)lpvParam;

		/* from now on we always have an alias for MS Sans Serif */

		GetProfileStringA("Desktop", "IconTitleFaceName", "MS Sans Serif", 
			lpLogFont->lfFaceName, LF_FACESIZE );
		lpLogFont->lfHeight = -GetProfileIntA("Desktop","IconTitleSize", 13);
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

	case SPI_GETICONMETRICS: {
	    LPICONMETRICSA lpIcon = lpvParam;
	    if(!lpIcon || lpIcon->cbSize != sizeof(*lpIcon))
	        return FALSE;
	    SystemParametersInfoA( SPI_ICONHORIZONTALSPACING, 0,
				   &lpIcon->iHorzSpacing, FALSE );
	    SystemParametersInfoA( SPI_ICONVERTICALSPACING, 0,
				   &lpIcon->iVertSpacing, FALSE );
	    SystemParametersInfoA( SPI_GETICONTITLEWRAP, 0,
				   &lpIcon->iTitleWrap, FALSE );
	    SystemParametersInfoA( SPI_GETICONTITLELOGFONT, 0,
				   &lpIcon->lfFont, FALSE );
	    break;
	}
	case SPI_GETWORKAREA:
		SetRect( (RECT *)lpvParam, 0, 0,
			GetSystemMetrics( SM_CXSCREEN ),
			GetSystemMetrics( SM_CYSCREEN )
		);
		break;
	case SPI_GETNONCLIENTMETRICS: 

#define lpnm ((LPNONCLIENTMETRICSA)lpvParam)
		
		if (lpnm->cbSize == sizeof(NONCLIENTMETRICSA))
		{
		    LPLOGFONTA lpLogFont = &(lpnm->lfMenuFont);
		
		    /* clear the struct, so we have 'sane' members */
		    memset(
			(char*)lpvParam+sizeof(lpnm->cbSize),
			0,
			lpnm->cbSize-sizeof(lpnm->cbSize)
		    );

		    /* FIXME: initialize geometry entries */
		    /* FIXME: As these values are presumably in device units,
		     *  we should calculate the defaults based on the screen dpi
		     */
		    /* caption */
		    lpnm->iCaptionWidth = ((TWEAK_WineLook > WIN31_LOOK)  ? 32 : 20);
		    lpnm->iCaptionHeight = lpnm->iCaptionWidth;
		    lpnm->lfCaptionFont.lfWeight = FW_BOLD;
		    SystemParametersInfoA(SPI_GETICONTITLELOGFONT, 0, (LPVOID)&(lpnm->lfCaptionFont),0);

		    /* small caption */
		    lpnm->iSmCaptionWidth = ((TWEAK_WineLook > WIN31_LOOK)  ? 32 : 17);
		    lpnm->iSmCaptionHeight = lpnm->iSmCaptionWidth;
		    SystemParametersInfoA(SPI_GETICONTITLELOGFONT, 0, (LPVOID)&(lpnm->lfSmCaptionFont),0);

		    /* menus, FIXME: names of wine.conf entrys are bogus */

		    lpnm->iMenuWidth = GetProfileIntA("Desktop","MenuWidth", 13);	/* size of the menu buttons*/
		    lpnm->iMenuHeight = GetProfileIntA("Desktop","MenuHeight", 
						(TWEAK_WineLook > WIN31_LOOK) ? 13 : 27);

		    GetProfileStringA("Desktop", "MenuFont", 
			(TWEAK_WineLook > WIN31_LOOK) ? "MS Sans Serif": "System", 
			lpLogFont->lfFaceName, LF_FACESIZE );

		    lpLogFont->lfHeight = -GetProfileIntA("Desktop","MenuFontSize", 13);
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

		    SystemParametersInfoA(SPI_GETICONTITLELOGFONT, 0,
							(LPVOID)&(lpnm->lfStatusFont),0);
		    SystemParametersInfoA(SPI_GETICONTITLELOGFONT, 0,
							(LPVOID)&(lpnm->lfMessageFont),0);
		}
		else
		{
		    WARN("size mismatch !! (is %d; should be %d)\n", lpnm->cbSize, sizeof(NONCLIENTMETRICSA));
		    return FALSE;
		}
#undef lpnm
		break;

        case SPI_GETANIMATION: {
                LPANIMATIONINFO lpAnimInfo = (LPANIMATIONINFO)lpvParam;
 
                /* Tell it "disabled" */
                lpAnimInfo->cbSize = sizeof(ANIMATIONINFO);
                uParam = sizeof(ANIMATIONINFO);
                lpAnimInfo->iMinAnimate = 0; /* Minimise and restore animation is disabled (nonzero == enabled) */
                break;
        }
 
        case SPI_SETANIMATION: {
                LPANIMATIONINFO lpAnimInfo = (LPANIMATIONINFO)lpvParam;
 
                /* Do nothing */
                WARN("SPI_SETANIMATION ignored.\n");
                lpAnimInfo->cbSize = sizeof(ANIMATIONINFO);
                uParam = sizeof(ANIMATIONINFO);
                break;
        }

        case SPI_GETHIGHCONTRAST:
        {
                LPHIGHCONTRASTA lpHighContrastA = (LPHIGHCONTRASTA)lpvParam;

                FIXME("SPI_GETHIGHCONTRAST not fully implemented\n");

                if ( lpHighContrastA->cbSize == sizeof( HIGHCONTRASTA ) )
                {
                        /* Indicate that there is no high contrast available */
                        lpHighContrastA->dwFlags = 0;
                        lpHighContrastA->lpszDefaultScheme = NULL;
                }
                else
                {
                        return FALSE;
                }

                break;
        }

	default:
		return SystemParametersInfo16(uAction,uParam,lpvParam,fuWinIni);
	}
	return TRUE;
}


/***********************************************************************
 *	SystemParametersInfo16   (USER.483)
 */
BOOL16 WINAPI SystemParametersInfo16( UINT16 uAction, UINT16 uParam,
                                      LPVOID lpvParam, UINT16 fuWinIni )
{
	int timeout; 
	char buffer[256];

	switch (uAction)
        {
		case SPI_GETBEEP:
  		        *(BOOL *) lpvParam = KEYBOARD_GetBeepActive();
			break;
		
		case SPI_GETBORDER:
			*(INT16 *)lpvParam = GetSystemMetrics( SM_CXFRAME );
			break;

		case SPI_GETFASTTASKSWITCH:
		    if ( GetProfileIntA( "windows", "CoolSwitch", 1 ) == 1 )
			  *(BOOL16 *) lpvParam = TRUE;
			else
			  *(BOOL16 *) lpvParam = FALSE;
			break;

		case SPI_GETGRIDGRANULARITY:
                    *(INT16 *) lpvParam = GetProfileIntA( "desktop", 
                                                          "GridGranularity",
                                                          1 );
                    break;

                case SPI_GETICONTITLEWRAP:
                    *(BOOL16 *) lpvParam = GetProfileIntA( "desktop",
                                                           "IconTitleWrap",
                                                           TRUE );
                    break;

                case SPI_GETKEYBOARDDELAY:
                    *(INT16 *) lpvParam = GetProfileIntA( "keyboard",
                                                          "KeyboardDelay", 1 );
                    break;

                case SPI_GETKEYBOARDSPEED:
                    *(WORD *) lpvParam = GetProfileIntA( "keyboard",
                                                           "KeyboardSpeed",
                                                           30 );
                    break;

		case SPI_GETMENUDROPALIGNMENT:
			*(BOOL16 *) lpvParam = GetSystemMetrics( SM_MENUDROPALIGNMENT ); /* XXX check this */
			break;

		case SPI_GETSCREENSAVEACTIVE:
		  if(USER_Driver->pGetScreenSaveActive() ||
		     GetProfileIntA( "windows", "ScreenSaveActive", 1 ) == 1)
   		        *(BOOL16 *) lpvParam = TRUE;
                    else
                        *(BOOL16 *) lpvParam = FALSE;
                    break;

		case SPI_GETSCREENSAVETIMEOUT:
			timeout = USER_Driver->pGetScreenSaveTimeout();
			if(!timeout)
			    timeout = GetProfileIntA( "windows", "ScreenSaveTimeout", 300 );
			*(INT16 *) lpvParam = timeout;
			break;

		case SPI_ICONHORIZONTALSPACING:
                    /* FIXME Get/SetProfileInt */
			if (lpvParam == NULL)
                            /*SetSystemMetrics( SM_CXICONSPACING, uParam )*/ ;
                        else
                            *(INT16 *)lpvParam = GetSystemMetrics( SM_CXICONSPACING );
			break;

		case SPI_ICONVERTICALSPACING:
                    /* FIXME Get/SetProfileInt */
                    if (lpvParam == NULL)
                        /*SetSystemMetrics( SM_CYICONSPACING, uParam )*/ ;
		    else
                        *(INT16 *)lpvParam = GetSystemMetrics(SM_CYICONSPACING);
                    break;

		case SPI_SETBEEP:
		        KEYBOARD_SetBeepActive(uParam);
			break;

		case SPI_SETSCREENSAVEACTIVE:
			USER_Driver->pSetScreenSaveActive(uParam);
			break;

		case SPI_SETSCREENSAVETIMEOUT:
			USER_Driver->pSetScreenSaveTimeout(uParam);
			break;

		case SPI_SETDESKWALLPAPER:
			return (SetDeskWallPaper((LPSTR) lpvParam));
			break;

		case SPI_SETDESKPATTERN:
			if ((INT16)uParam == -1) {
				GetProfileStringA("Desktop", "Pattern", 
						"170 85 170 85 170 85 170 85", 
						buffer, sizeof(buffer) );
				return (DESKTOP_SetPattern((LPSTR) buffer));
			} else
				return (DESKTOP_SetPattern((LPSTR) lpvParam));
			break;

	        case SPI_GETICONTITLELOGFONT: 
	        {
                    LPLOGFONT16 lpLogFont = (LPLOGFONT16)lpvParam;
		    GetProfileStringA("Desktop", "IconTitleFaceName", "MS Sans Serif", 
					lpLogFont->lfFaceName, LF_FACESIZE );
                    lpLogFont->lfHeight = -GetProfileIntA("Desktop","IconTitleSize", 13);
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
		case SPI_GETNONCLIENTMETRICS:

#define lpnm ((LPNONCLIENTMETRICS16)lpvParam)
		    if( lpnm->cbSize == sizeof(NONCLIENTMETRICS16) )
		    {
			/* clear the struct, so we have 'sane' members */
			memset(
			    (char*)lpvParam+sizeof(lpnm->cbSize),
			    0,
			    lpnm->cbSize-sizeof(lpnm->cbSize)
			);
			/* FIXME: initialize geometry entries */
			SystemParametersInfo16( SPI_GETICONTITLELOGFONT, 0, 
							(LPVOID)&(lpnm->lfCaptionFont),0);
			lpnm->lfCaptionFont.lfWeight = FW_BOLD;
			SystemParametersInfo16( SPI_GETICONTITLELOGFONT, 0,
							(LPVOID)&(lpnm->lfSmCaptionFont),0);
			SystemParametersInfo16( SPI_GETICONTITLELOGFONT, 0,
							(LPVOID)&(lpnm->lfMenuFont),0);
			SystemParametersInfo16( SPI_GETICONTITLELOGFONT, 0,
							(LPVOID)&(lpnm->lfStatusFont),0);
			SystemParametersInfo16( SPI_GETICONTITLELOGFONT, 0,
							(LPVOID)&(lpnm->lfMessageFont),0);
		    }
		    else /* winfile 95 sets sbSize to 340 */
		        SystemParametersInfoA( uAction, uParam, lpvParam, fuWinIni );
#undef lpnm
		    break;

		case SPI_LANGDRIVER:
		case SPI_SETBORDER:
		case SPI_SETDOUBLECLKHEIGHT:
		case SPI_SETDOUBLECLICKTIME:
		case SPI_SETDOUBLECLKWIDTH:
		case SPI_SETFASTTASKSWITCH:
		case SPI_SETKEYBOARDDELAY:
		case SPI_SETKEYBOARDSPEED:
			WARN("Option %d ignored.\n", uAction);
			break;

                case SPI_GETWORKAREA:
                    SetRect16( (RECT16 *)lpvParam, 0, 0,
                               GetSystemMetrics( SM_CXSCREEN ),
                               GetSystemMetrics( SM_CYSCREEN ) );
                    break;

                /* FIXME need to implement SPI_SETMOUSEHOVER... */
                case SPI_GETMOUSEHOVERWIDTH:
                    *(INT16 *)lpvParam = 4;
                    break;

                case SPI_GETMOUSEHOVERHEIGHT:
                    *(INT16 *)lpvParam = 4;
                    break;

                case SPI_GETMOUSEHOVERTIME:
                    *(INT16 *)lpvParam = 400; /* default for menu dropdowns */
                    break;
                
		default:
			FIXME("Unknown option %d.\n", uAction);
			SetLastError(ERROR_INVALID_SPI_VALUE);
			return 0;
	}
	return 1;
}

/***********************************************************************
 *	SystemParametersInfoW   (USER32.541)
 */
BOOL WINAPI SystemParametersInfoW( UINT uAction, UINT uParam,
                                       LPVOID lpvParam, UINT fuWinIni )
{
    char buffer[256];

    switch (uAction)
    {
    case SPI_SETDESKWALLPAPER:
        if (lpvParam)
        {
            lstrcpynWtoA(buffer,(LPWSTR)lpvParam,sizeof(buffer));
            return SetDeskWallPaper(buffer);
        }
        return SetDeskWallPaper(NULL);

    case SPI_SETDESKPATTERN:
        if ((INT) uParam == -1)
        {
            GetProfileStringA("Desktop", "Pattern", 
                                "170 85 170 85 170 85 170 85", 
                                buffer, sizeof(buffer) );
            return (DESKTOP_SetPattern((LPSTR) buffer));
        }
        if (lpvParam)
        {
            lstrcpynWtoA(buffer,(LPWSTR)lpvParam,sizeof(buffer));
            return DESKTOP_SetPattern(buffer);
        }
        return DESKTOP_SetPattern(NULL);

    case SPI_GETICONTITLELOGFONT:
        {
            LPLOGFONTW lpLogFont = (LPLOGFONTW)lpvParam;

 	    GetProfileStringA("Desktop", "IconTitleFaceName", "MS Sans Serif", 
			 buffer, sizeof(buffer) );
	    lstrcpynAtoW(lpLogFont->lfFaceName, buffer, LF_FACESIZE);
            lpLogFont->lfHeight = 17;
            lpLogFont->lfWidth = 0;
            lpLogFont->lfEscapement = lpLogFont->lfOrientation = 0;
            lpLogFont->lfWeight = FW_NORMAL;
            lpLogFont->lfItalic = lpLogFont->lfStrikeOut = lpLogFont->lfUnderline = FALSE;
            lpLogFont->lfCharSet = ANSI_CHARSET;
            lpLogFont->lfOutPrecision = OUT_DEFAULT_PRECIS;
            lpLogFont->lfClipPrecision = CLIP_DEFAULT_PRECIS;
            lpLogFont->lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;
        }
        break;
	case SPI_GETICONMETRICS: {
	    LPICONMETRICSW lpIcon = lpvParam;
	    if(!lpIcon || lpIcon->cbSize != sizeof(*lpIcon))
	        return FALSE;
	    SystemParametersInfoW( SPI_ICONHORIZONTALSPACING, 0,
				   &lpIcon->iHorzSpacing, FALSE );
	    SystemParametersInfoW( SPI_ICONVERTICALSPACING, 0,
				   &lpIcon->iVertSpacing, FALSE );
	    SystemParametersInfoW( SPI_GETICONTITLEWRAP, 0,
				   &lpIcon->iTitleWrap, FALSE );
	    SystemParametersInfoW( SPI_GETICONTITLELOGFONT, 0,
				   &lpIcon->lfFont, FALSE );
	    break;
	}
    case SPI_GETNONCLIENTMETRICS: {
	/* FIXME: implement correctly */
	LPNONCLIENTMETRICSW	lpnm=(LPNONCLIENTMETRICSW)lpvParam;

	/* clear the struct, so we have 'sane' members */
	memset((char*)lpvParam+sizeof(lpnm->cbSize),
		0,
		lpnm->cbSize-sizeof(lpnm->cbSize)
	);
	SystemParametersInfoW(SPI_GETICONTITLELOGFONT,0,(LPVOID)&(lpnm->lfCaptionFont),0);
	lpnm->lfCaptionFont.lfWeight = FW_BOLD;
	SystemParametersInfoW(SPI_GETICONTITLELOGFONT,0,(LPVOID)&(lpnm->lfSmCaptionFont),0);
	SystemParametersInfoW(SPI_GETICONTITLELOGFONT,0,(LPVOID)&(lpnm->lfMenuFont),0);
	SystemParametersInfoW(SPI_GETICONTITLELOGFONT,0,(LPVOID)&(lpnm->lfStatusFont),0);
	SystemParametersInfoW(SPI_GETICONTITLELOGFONT,0,(LPVOID)&(lpnm->lfMessageFont),0);
	break;
    }

    case SPI_GETHIGHCONTRAST:
    {
       LPHIGHCONTRASTW lpHighContrastW = (LPHIGHCONTRASTW)lpvParam;

       FIXME("SPI_GETHIGHCONTRAST not fully implemented\n");

       if ( lpHighContrastW->cbSize == sizeof( HIGHCONTRASTW ) )
       {
          /* Indicate that there is no high contrast available */
          lpHighContrastW->dwFlags = 0;
          lpHighContrastW->lpszDefaultScheme = NULL;
       }
       else
       {
          return FALSE;
       }

       break;
    }

    default:
        return SystemParametersInfoA(uAction,uParam,lpvParam,fuWinIni);
	
    }
    return TRUE;
}
