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

#include "config.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wingdi.h"
#include "winreg.h"
#include "wine/wingdi16.h"
#include "winerror.h"

#include "controls.h"
#include "user_private.h"
#include "wine/gdi_driver.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(system);

/* System parameter indexes */
enum spi_index
{
    SPI_SETWORKAREA_IDX,
    SPI_SETLISTBOXSMOOTHSCROLLING_IDX,
    SPI_USERPREFERENCEMASK_IDX,
    SPI_NONCLIENTMETRICS_IDX,
    SPI_INDEX_COUNT
};

static const struct
{
    const char *name;
    COLORREF    rgb;
} DefSysColors[] =
{
    {"Scrollbar", RGB(212, 208, 200)},              /* COLOR_SCROLLBAR */
    {"Background", RGB(58, 110, 165)},              /* COLOR_BACKGROUND */
    {"ActiveTitle", RGB(10, 36, 106)},              /* COLOR_ACTIVECAPTION */
    {"InactiveTitle", RGB(128, 128, 128)},          /* COLOR_INACTIVECAPTION */
    {"Menu", RGB(212, 208, 200)},                   /* COLOR_MENU */
    {"Window", RGB(255, 255, 255)},                 /* COLOR_WINDOW */
    {"WindowFrame", RGB(0, 0, 0)},                  /* COLOR_WINDOWFRAME */
    {"MenuText", RGB(0, 0, 0)},                     /* COLOR_MENUTEXT */
    {"WindowText", RGB(0, 0, 0)},                   /* COLOR_WINDOWTEXT */
    {"TitleText", RGB(255, 255, 255)},              /* COLOR_CAPTIONTEXT */
    {"ActiveBorder", RGB(212, 208, 200)},           /* COLOR_ACTIVEBORDER */
    {"InactiveBorder", RGB(212, 208, 200)},         /* COLOR_INACTIVEBORDER */
    {"AppWorkSpace", RGB(128, 128, 128)},           /* COLOR_APPWORKSPACE */
    {"Hilight", RGB(10, 36, 106)},                  /* COLOR_HIGHLIGHT */
    {"HilightText", RGB(255, 255, 255)},            /* COLOR_HIGHLIGHTTEXT */
    {"ButtonFace", RGB(212, 208, 200)},             /* COLOR_BTNFACE */
    {"ButtonShadow", RGB(128, 128, 128)},           /* COLOR_BTNSHADOW */
    {"GrayText", RGB(128, 128, 128)},               /* COLOR_GRAYTEXT */
    {"ButtonText", RGB(0, 0, 0)},                   /* COLOR_BTNTEXT */
    {"InactiveTitleText", RGB(212, 208, 200)},      /* COLOR_INACTIVECAPTIONTEXT */
    {"ButtonHilight", RGB(255, 255, 255)},          /* COLOR_BTNHIGHLIGHT */
    {"ButtonDkShadow", RGB(64, 64, 64)},            /* COLOR_3DDKSHADOW */
    {"ButtonLight", RGB(212, 208, 200)},            /* COLOR_3DLIGHT */
    {"InfoText", RGB(0, 0, 0)},                     /* COLOR_INFOTEXT */
    {"InfoWindow", RGB(255, 255, 225)},             /* COLOR_INFOBK */
    {"ButtonAlternateFace", RGB(181, 181, 181)},    /* COLOR_ALTERNATEBTNFACE */
    {"HotTrackingColor", RGB(0, 0, 200)},           /* COLOR_HOTLIGHT */
    {"GradientActiveTitle", RGB(166, 202, 240)},    /* COLOR_GRADIENTACTIVECAPTION */
    {"GradientInactiveTitle", RGB(192, 192, 192)},  /* COLOR_GRADIENTINACTIVECAPTION */
    {"MenuHilight", RGB(10, 36, 106)},              /* COLOR_MENUHILIGHT */
    {"MenuBar", RGB(212, 208, 200)}                 /* COLOR_MENUBAR */
};

/**
 * Names of the registry subkeys of HKEY_CURRENT_USER key and value names
 * for the system parameters.
 * Names of the keys are created by adding string "_REGKEY" to
 * "SET" action names, value names are created by adding "_REG_NAME"
 * to the "SET" action name.
 */
static const WCHAR SPI_SETBEEP_REGKEY[]=                      {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','S','o','u','n','d',0};
static const WCHAR SPI_SETBEEP_VALNAME[]=                     {'B','e','e','p',0};
static const WCHAR SPI_SETMOUSETHRESHOLD1_REGKEY[]=           {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','M','o','u','s','e',0};
static const WCHAR SPI_SETMOUSETHRESHOLD1_VALNAME[]=          {'M','o','u','s','e','T','h','r','e','s','h','o','l','d','1',0};
static const WCHAR SPI_SETMOUSETHRESHOLD2_REGKEY[]=           {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','M','o','u','s','e',0};
static const WCHAR SPI_SETMOUSETHRESHOLD2_VALNAME[]=          {'M','o','u','s','e','T','h','r','e','s','h','o','l','d','2',0};
static const WCHAR SPI_SETMOUSEACCELERATION_REGKEY[]=         {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','M','o','u','s','e',0};
static const WCHAR SPI_SETMOUSEACCELERATION_VALNAME[]=        {'M','o','u','s','e','S','p','e','e','d',0};
static const WCHAR SPI_SETBORDER_REGKEY[]=                    {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p','\\',
                                                               'W','i','n','d','o','w','M','e','t','r','i','c','s',0};
static const WCHAR SPI_SETBORDER_VALNAME[]=                   {'B','o','r','d','e','r','W','i','d','t','h',0};
static const WCHAR SPI_SETKEYBOARDSPEED_REGKEY[]=             {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','K','e','y','b','o','a','r','d',0};
static const WCHAR SPI_SETKEYBOARDSPEED_VALNAME[]=            {'K','e','y','b','o','a','r','d','S','p','e','e','d',0};
static const WCHAR SPI_SETICONHORIZONTALSPACING_REGKEY[]=     {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p','\\',
                                                               'W','i','n','d','o','w','M','e','t','r','i','c','s',0};
static const WCHAR SPI_SETICONHORIZONTALSPACING_VALNAME[]=    {'I','c','o','n','S','p','a','c','i','n','g',0};
static const WCHAR SPI_SETSCREENSAVETIMEOUT_REGKEY[]=         {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETSCREENSAVETIMEOUT_VALNAME[]=        {'S','c','r','e','e','n','S','a','v','e','T','i','m','e','O','u','t',0};
static const WCHAR SPI_SETSCREENSAVEACTIVE_REGKEY[]=          {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETSCREENSAVEACTIVE_VALNAME[]=         {'S','c','r','e','e','n','S','a','v','e','A','c','t','i','v','e',0};
static const WCHAR SPI_SETGRIDGRANULARITY_REGKEY[]=           {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETGRIDGRANULARITY_VALNAME[]=          {'G','r','i','d','G','r','a','n','u','l','a','r','i','t','y',0};
static const WCHAR SPI_SETKEYBOARDDELAY_REGKEY[]=             {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','K','e','y','b','o','a','r','d',0};
static const WCHAR SPI_SETKEYBOARDDELAY_VALNAME[]=            {'K','e','y','b','o','a','r','d','D','e','l','a','y',0};
static const WCHAR SPI_SETICONVERTICALSPACING_REGKEY[]=       {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p','\\',
                                                               'W','i','n','d','o','w','M','e','t','r','i','c','s',0};
static const WCHAR SPI_SETICONVERTICALSPACING_VALNAME[]=      {'I','c','o','n','V','e','r','t','i','c','a','l','S','p','a','c','i','n','g',0};
static const WCHAR SPI_SETICONTITLEWRAP_REGKEY[]=             {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETICONTITLEWRAP_MIRROR[]=             {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p','\\',
                                                               'W','i','n','d','o','w','M','e','t','r','i','c','s',0};
static const WCHAR SPI_SETICONTITLEWRAP_VALNAME[]=            {'I','c','o','n','T','i','t','l','e','W','r','a','p',0};
static const WCHAR SPI_SETICONTITLELOGFONT_REGKEY[]=          {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p','\\',
                                                               'W','i','n','d','o','w','M','e','t','r','i','c','s',0};
static const WCHAR SPI_SETICONTITLELOGFONT_VALNAME[]=         {'I','c','o','n','F','o','n','t',0};
static const WCHAR SPI_SETMENUDROPALIGNMENT_REGKEY[]=         {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETMENUDROPALIGNMENT_MIRROR[]=         {'S','o','f','t','w','a','r','e','\\',
                                                               'M','i','c','r','o','s','o','f','t','\\',
                                                               'W','i','n','d','o','w','s',' ','N','T','\\',
                                                               'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
                                                               'W','i','n','d','o','w','s',0};
static const WCHAR SPI_SETMENUDROPALIGNMENT_VALNAME[]=        {'M','e','n','u','D','r','o','p','A','l','i','g','n','m','e','n','t',0};
static const WCHAR SPI_SETMOUSETRAILS_REGKEY[]=               {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','M','o','u','s','e',0};
static const WCHAR SPI_SETMOUSETRAILS_VALNAME[]=              {'M','o','u','s','e','T','r','a','i','l','s',0};
static const WCHAR SPI_SETSNAPTODEFBUTTON_REGKEY[]=           {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','M','o','u','s','e',0};
static const WCHAR SPI_SETSNAPTODEFBUTTON_VALNAME[]=          {'S','n','a','p','T','o','D','e','f','a','u','l','t','B','u','t','t','o','n',0};
static const WCHAR SPI_SETDOUBLECLKWIDTH_REGKEY[]=            {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','M','o','u','s','e',0};
static const WCHAR SPI_SETDOUBLECLKWIDTH_MIRROR[]=            {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETDOUBLECLKWIDTH_VALNAME[]=           {'D','o','u','b','l','e','C','l','i','c','k','W','i','d','t','h',0};
static const WCHAR SPI_SETDOUBLECLKHEIGHT_REGKEY[]=           {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','M','o','u','s','e',0};
static const WCHAR SPI_SETDOUBLECLKHEIGHT_MIRROR[]=           {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETDOUBLECLKHEIGHT_VALNAME[]=          {'D','o','u','b','l','e','C','l','i','c','k','H','e','i','g','h','t',0};
static const WCHAR SPI_SETDOUBLECLICKTIME_REGKEY[]=           {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','M','o','u','s','e',0};
static const WCHAR SPI_SETDOUBLECLICKTIME_VALNAME[]=          {'D','o','u','b','l','e','C','l','i','c','k','S','p','e','e','d',0};
static const WCHAR SPI_SETMOUSEBUTTONSWAP_REGKEY[]=           {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','M','o','u','s','e',0};
static const WCHAR SPI_SETMOUSEBUTTONSWAP_VALNAME[]=          {'S','w','a','p','M','o','u','s','e','B','u','t','t','o','n','s',0};
static const WCHAR SPI_SETDRAGFULLWINDOWS_REGKEY[]=           {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETDRAGFULLWINDOWS_VALNAME[]=          {'D','r','a','g','F','u','l','l','W','i','n','d','o','w','s',0};
static const WCHAR SPI_SETSHOWSOUNDS_REGKEY[]=                {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\',
                                                               'A','c','c','e','s','s','i','b','i','l','i','t','y','\\',
                                                               'S','h','o','w','S','o','u','n','d','s',0};
static const WCHAR SPI_SETSHOWSOUNDS_VALNAME[]=               {'O','n',0};
static const WCHAR SPI_SETKEYBOARDPREF_REGKEY[]=              {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\',
                                                               'A','c','c','e','s','s','i','b','i','l','i','t','y','\\',
                                                               'K','e','y','b','o','a','r','d',' ','P','r','e','f','e','r','e','n','c','e',0};
static const WCHAR SPI_SETKEYBOARDPREF_VALNAME[]=             {'O','n',0};
static const WCHAR SPI_SETSCREENREADER_REGKEY[]=              {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\',
                                                               'A','c','c','e','s','s','i','b','i','l','i','t','y','\\',
                                                               'B','l','i','n','d',' ','A','c','c','e','s','s',0};
static const WCHAR SPI_SETSCREENREADER_VALNAME[]=             {'O','n',0};
static const WCHAR SPI_SETDESKWALLPAPER_REGKEY[]=             {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETDESKWALLPAPER_VALNAME[]=            {'W','a','l','l','p','a','p','e','r',0};
static const WCHAR SPI_SETFONTSMOOTHING_REGKEY[]=             {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETFONTSMOOTHING_VALNAME[]=            {'F','o','n','t','S','m','o','o','t','h','i','n','g',0};
static const WCHAR SPI_SETDRAGWIDTH_REGKEY[]=                 {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETDRAGWIDTH_VALNAME[]=                {'D','r','a','g','W','i','d','t','h',0};
static const WCHAR SPI_SETDRAGHEIGHT_REGKEY[]=                {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETDRAGHEIGHT_VALNAME[]=               {'D','r','a','g','H','e','i','g','h','t',0};
static const WCHAR SPI_SETLOWPOWERACTIVE_REGKEY[]=            {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETLOWPOWERACTIVE_VALNAME[]=           {'L','o','w','P','o','w','e','r','A','c','t','i','v','e',0};
static const WCHAR SPI_SETPOWEROFFACTIVE_REGKEY[]=            {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETPOWEROFFACTIVE_VALNAME[]=           {'P','o','w','e','r','O','f','f','A','c','t','i','v','e',0};
static const WCHAR SPI_USERPREFERENCEMASK_REGKEY[]=           {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_USERPREFERENCEMASK_VALNAME[]=          {'U','s','e','r','P','r','e','f','e','r','e','n','c','e','m','a','s','k',0};
static const WCHAR SPI_SETMOUSEHOVERWIDTH_REGKEY[]=           {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','M','o','u','s','e',0};
static const WCHAR SPI_SETMOUSEHOVERWIDTH_VALNAME[]=          {'M','o','u','s','e','H','o','v','e','r','W','i','d','t','h',0};
static const WCHAR SPI_SETMOUSEHOVERHEIGHT_REGKEY[]=          {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','M','o','u','s','e',0};
static const WCHAR SPI_SETMOUSEHOVERHEIGHT_VALNAME[]=         {'M','o','u','s','e','H','o','v','e','r','H','e','i','g','h','t',0};
static const WCHAR SPI_SETMOUSEHOVERTIME_REGKEY[]=            {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','M','o','u','s','e',0};
static const WCHAR SPI_SETMOUSEHOVERTIME_VALNAME[]=           {'M','o','u','s','e','H','o','v','e','r','T','i','m','e',0};
static const WCHAR SPI_SETWHEELSCROLLCHARS_REGKEY[]=          {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETWHEELSCROLLCHARS_VALNAME[]=         {'W','h','e','e','l','S','c','r','o','l','l','C','h','a','r','s',0};
static const WCHAR SPI_SETWHEELSCROLLLINES_REGKEY[]=          {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETWHEELSCROLLLINES_VALNAME[]=         {'W','h','e','e','l','S','c','r','o','l','l','L','i','n','e','s',0};
static const WCHAR SPI_SETACTIVEWINDOWTRACKING_REGKEY[]=      {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','M','o','u','s','e',0};
static const WCHAR SPI_SETACTIVEWINDOWTRACKING_VALNAME[]=     {'A','c','t','i','v','e','W','i','n','d','o','w','T','r','a','c','k','i','n','g',0};
static const WCHAR SPI_SETMENUSHOWDELAY_REGKEY[]=             {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETMENUSHOWDELAY_VALNAME[]=            {'M','e','n','u','S','h','o','w','D','e','l','a','y',0};
static const WCHAR SPI_SETBLOCKSENDINPUTRESETS_REGKEY[]=      {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETBLOCKSENDINPUTRESETS_VALNAME[]=     {'B','l','o','c','k','S','e','n','d','I','n','p','u','t','R','e','s','e','t','s',0};
static const WCHAR SPI_SETFOREGROUNDLOCKTIMEOUT_REGKEY[]=     {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETFOREGROUNDLOCKTIMEOUT_VALNAME[]=    {'F','o','r','e','g','r','o','u','n','d','L','o','c','k','T','i','m','e','o','u','t',0};
static const WCHAR SPI_SETACTIVEWNDTRKTIMEOUT_REGKEY[]=       {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETACTIVEWNDTRKTIMEOUT_VALNAME[]=      {'A','c','t','i','v','e','W','n','d','T','r','a','c','k','T','i','m','e','o','u','t',0};
static const WCHAR SPI_SETFOREGROUNDFLASHCOUNT_REGKEY[]=      {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETFOREGROUNDFLASHCOUNT_VALNAME[]=     {'F','o','r','e','g','r','o','u','n','d','F','l','a','s','h','C','o','u','n','t',0};
static const WCHAR SPI_SETCARETWIDTH_REGKEY[]=                {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETCARETWIDTH_VALNAME[]=               {'C','a','r','e','t','W','i','d','t','h',0};
static const WCHAR SPI_SETMOUSECLICKLOCKTIME_REGKEY[]=        {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETMOUSECLICKLOCKTIME_VALNAME[]=       {'C','l','i','c','k','L','o','c','k','T','i','m','e',0};
static const WCHAR SPI_SETMOUSESPEED_REGKEY[]=                {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','M','o','u','s','e',0};
static const WCHAR SPI_SETMOUSESPEED_VALNAME[]=               {'M','o','u','s','e','S','e','n','s','i','t','i','v','i','t','y',0};
static const WCHAR SPI_SETFONTSMOOTHINGTYPE_REGKEY[]=         {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETFONTSMOOTHINGTYPE_VALNAME[]=        {'F','o','n','t','S','m','o','o','t','h','i','n','g','T','y','p','e',0};
static const WCHAR SPI_SETFONTSMOOTHINGCONTRAST_REGKEY[]=     {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETFONTSMOOTHINGCONTRAST_VALNAME[]=    {'F','o','n','t','S','m','o','o','t','h','i','n','g','G','a','m','m','a',0};
static const WCHAR SPI_SETFONTSMOOTHINGORIENTATION_REGKEY[]=  {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETFONTSMOOTHINGORIENTATION_VALNAME[]= {'F','o','n','t','S','m','o','o','t','h','i','n','g','O','r','i','e','n','t','a','t','i','o','n',0};
static const WCHAR SPI_SETFOCUSBORDERWIDTH_REGKEY[]=          {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETFOCUSBORDERWIDTH_VALNAME[]=         {'F','o','c','u','s','B','o','r','d','e','r','W','i','d','t','h',0};
static const WCHAR SPI_SETFOCUSBORDERHEIGHT_REGKEY[]=         {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETFOCUSBORDERHEIGHT_VALNAME[]=        {'F','o','c','u','s','B','o','r','d','e','r','H','e','i','g','h','t',0};

/* FIXME - real values */
static const WCHAR SPI_SETSCREENSAVERRUNNING_REGKEY[]=   {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p',0};
static const WCHAR SPI_SETSCREENSAVERRUNNING_VALNAME[]=  {'W','I','N','E','_','S','c','r','e','e','n','S','a','v','e','r','R','u','n','n','i','n','g',0};

static const WCHAR METRICS_REGKEY[]=                  {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p','\\',
                                                       'W','i','n','d','o','w','M','e','t','r','i','c','s',0};
static const WCHAR METRICS_SCROLLWIDTH_VALNAME[]=     {'S','c','r','o','l','l','W','i','d','t','h',0};
static const WCHAR METRICS_SCROLLHEIGHT_VALNAME[]=    {'S','c','r','o','l','l','H','e','i','g','h','t',0};
static const WCHAR METRICS_CAPTIONWIDTH_VALNAME[]=    {'C','a','p','t','i','o','n','W','i','d','t','h',0};
static const WCHAR METRICS_CAPTIONHEIGHT_VALNAME[]=   {'C','a','p','t','i','o','n','H','e','i','g','h','t',0};
static const WCHAR METRICS_SMCAPTIONWIDTH_VALNAME[]=  {'S','m','C','a','p','t','i','o','n','W','i','d','t','h',0};
static const WCHAR METRICS_SMCAPTIONHEIGHT_VALNAME[]= {'S','m','C','a','p','t','i','o','n','H','e','i','g','h','t',0};
static const WCHAR METRICS_MENUWIDTH_VALNAME[]=       {'M','e','n','u','W','i','d','t','h',0};
static const WCHAR METRICS_MENUHEIGHT_VALNAME[]=      {'M','e','n','u','H','e','i','g','h','t',0};
static const WCHAR METRICS_ICONSIZE_VALNAME[]=        {'S','h','e','l','l',' ','I','c','o','n',' ','S','i','z','e',0};
static const WCHAR METRICS_BORDERWIDTH_VALNAME[]=     {'B','o','r','d','e','r','W','i','d','t','h',0};
static const WCHAR METRICS_PADDEDBORDERWIDTH_VALNAME[]={'P','a','d','d','e','d','B','o','r','d','e','r','W','i','d','t','h',0};
static const WCHAR METRICS_CAPTIONLOGFONT_VALNAME[]=  {'C','a','p','t','i','o','n','F','o','n','t',0};
static const WCHAR METRICS_SMCAPTIONLOGFONT_VALNAME[]={'S','m','C','a','p','t','i','o','n','F','o','n','t',0};
static const WCHAR METRICS_MENULOGFONT_VALNAME[]=     {'M','e','n','u','F','o','n','t',0};
static const WCHAR METRICS_MESSAGELOGFONT_VALNAME[]=  {'M','e','s','s','a','g','e','F','o','n','t',0};
static const WCHAR METRICS_STATUSLOGFONT_VALNAME[]=   {'S','t','a','t','u','s','F','o','n','t',0};
/* minimized metrics */
#define SPI_SETMETRICS_MINWIDTH_REGKEY METRICS_REGKEY
static const WCHAR SPI_SETMETRICS_MINWIDTH_VALNAME[] =   {'M','i','n','W','i','d','t','h',0};
#define SPI_SETMETRICS_MINHORZGAP_REGKEY METRICS_REGKEY
static const WCHAR SPI_SETMETRICS_MINHORZGAP_VALNAME[] = {'M','i','n','H','o','r','z','G','a','p',0};
#define SPI_SETMETRICS_MINVERTGAP_REGKEY METRICS_REGKEY
static const WCHAR SPI_SETMETRICS_MINVERTGAP_VALNAME[] = {'M','i','n','V','e','r','t','G','a','p',0};
#define SPI_SETMETRICS_MINARRANGE_REGKEY METRICS_REGKEY
static const WCHAR SPI_SETMETRICS_MINARRANGE_VALNAME[] = {'M','i','n','A','r','r','a','n','g','e',0};

static const WCHAR WINE_CURRENT_USER_REGKEY[] = {'S','o','f','t','w','a','r','e','\\',
                                                 'W','i','n','e',0};

/* volatile registry branch under WINE_CURRENT_USER_REGKEY for temporary values storage */
static const WCHAR WINE_CURRENT_USER_REGKEY_TEMP_PARAMS[] = {'T','e','m','p','o','r','a','r','y',' ',
                                                             'S','y','s','t','e','m',' ',
                                                             'P','a','r','a','m','e','t','e','r','s',0};

static const WCHAR Yes[]=                                    {'Y','e','s',0};
static const WCHAR No[]=                                     {'N','o',0};
static const WCHAR Desktop[]=                                {'D','e','s','k','t','o','p',0};
static const WCHAR Pattern[]=                                {'P','a','t','t','e','r','n',0};
static const WCHAR MenuFont[]=                               {'M','e','n','u','F','o','n','t',0};
static const WCHAR MenuFontSize[]=                           {'M','e','n','u','F','o','n','t','S','i','z','e',0};
static const WCHAR StatusFont[]=                             {'S','t','a','t','u','s','F','o','n','t',0};
static const WCHAR StatusFontSize[]=                         {'S','t','a','t','u','s','F','o','n','t','S','i','z','e',0};
static const WCHAR MessageFont[]=                            {'M','e','s','s','a','g','e','F','o','n','t',0};
static const WCHAR MessageFontSize[]=                        {'M','e','s','s','a','g','e','F','o','n','t','S','i','z','e',0};
static const WCHAR System[]=                                 {'S','y','s','t','e','m',0};
static const WCHAR IconTitleSize[]=                          {'I','c','o','n','T','i','t','l','e','S','i','z','e',0};
static const WCHAR IconTitleFaceName[]=                      {'I','c','o','n','T','i','t','l','e','F','a','c','e','N','a','m','e',0};
static const WCHAR defPattern[]=                             {'0',' ','0',' ','0',' ','0',' ','0',' ','0',' ','0',' ','0',0};
static const WCHAR CSu[]=                                    {'%','u',0};
static const WCHAR CSd[]=                                    {'%','d',0};

/* Indicators whether system parameter value is loaded */
static char spi_loaded[SPI_INDEX_COUNT];

static BOOL notify_change = TRUE;

/* System parameters storage */
static RECT work_area;
static BYTE user_prefs[4];

static NONCLIENTMETRICSW nonclient_metrics =
{
    sizeof(NONCLIENTMETRICSW),
    1,     /* iBorderWidth */
    16,    /* iScrollWidth */
    16,    /* iScrollHeight */
    18,    /* iCaptionWidth */
    18,    /* iCaptionHeight */
    { 0 }, /* lfCaptionFont */
    13,    /* iSmCaptionWidth */
    15,    /* iSmCaptionHeight */
    { 0 }, /* lfSmCaptionFont */
    18,    /* iMenuWidth */
    18,    /* iMenuHeight */
    { 0 }, /* lfMenuFont */
    { 0 }, /* lfStatusFont */
    { 0 }, /* lfMessageFont */
    0      /* iPaddedBorderWidth */
};

/* some additional non client metric info */
static TEXTMETRICW tmMenuFont;
static UINT CaptionFontAvCharWidth;

static SIZE icon_size = { 32, 32 };

#define NUM_SYS_COLORS     (COLOR_MENUBAR+1)

static COLORREF SysColors[NUM_SYS_COLORS];
static HBRUSH SysColorBrushes[NUM_SYS_COLORS];
static HPEN   SysColorPens[NUM_SYS_COLORS];

static const WORD wPattern55AA[] = { 0x5555, 0xaaaa, 0x5555, 0xaaaa, 0x5555, 0xaaaa, 0x5555, 0xaaaa };

HBRUSH SYSCOLOR_55AABrush = 0;

union sysparam_all_entry;

struct sysparam_entry
{
    BOOL       (*get)( union sysparam_all_entry *entry, UINT int_param, void *ptr_param );
    BOOL       (*set)( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT flags );
    const WCHAR *regkey;
    const WCHAR *regval;
    const WCHAR *mirror;
    BOOL         loaded;
};

struct sysparam_uint_entry
{
    struct sysparam_entry hdr;
    UINT                  val;
};

struct sysparam_bool_entry
{
    struct sysparam_entry hdr;
    BOOL                  val;
};

struct sysparam_dword_entry
{
    struct sysparam_entry hdr;
    DWORD                 val;
};

struct sysparam_font_entry
{
    struct sysparam_entry hdr;
    LOGFONTW              val;
};

union sysparam_all_entry
{
    struct sysparam_entry       hdr;
    struct sysparam_uint_entry  uint;
    struct sysparam_bool_entry  bool;
    struct sysparam_dword_entry dword;
    struct sysparam_font_entry  font;
};

static void SYSPARAMS_LogFont16To32W( const LOGFONT16 *font16, LPLOGFONTW font32 )
{
    font32->lfHeight = font16->lfHeight;
    font32->lfWidth = font16->lfWidth;
    font32->lfEscapement = font16->lfEscapement;
    font32->lfOrientation = font16->lfOrientation;
    font32->lfWeight = font16->lfWeight;
    font32->lfItalic = font16->lfItalic;
    font32->lfUnderline = font16->lfUnderline;
    font32->lfStrikeOut = font16->lfStrikeOut;
    font32->lfCharSet = font16->lfCharSet;
    font32->lfOutPrecision = font16->lfOutPrecision;
    font32->lfClipPrecision = font16->lfClipPrecision;
    font32->lfQuality = font16->lfQuality;
    font32->lfPitchAndFamily = font16->lfPitchAndFamily;
    MultiByteToWideChar( CP_ACP, 0, font16->lfFaceName, -1, font32->lfFaceName, LF_FACESIZE );
    font32->lfFaceName[LF_FACESIZE-1] = 0;
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

struct monitor_info
{
    int count;
    RECT virtual_rect;
};

static BOOL CALLBACK monitor_info_proc( HMONITOR monitor, HDC hdc, LPRECT rect, LPARAM lp )
{
    struct monitor_info *info = (struct monitor_info *)lp;
    info->count++;
    UnionRect( &info->virtual_rect, &info->virtual_rect, rect );
    return TRUE;
}

static void get_monitors_info( struct monitor_info *info )
{
    info->count = 0;
    SetRectEmpty( &info->virtual_rect );
    EnumDisplayMonitors( 0, NULL, monitor_info_proc, (LPARAM)info );
}

RECT get_virtual_screen_rect(void)
{
    struct monitor_info info;
    get_monitors_info( &info );
    return info.virtual_rect;
}

/* get text metrics and/or "average" char width of the specified logfont 
 * for the specified dc */
static void get_text_metr_size( HDC hdc, LOGFONTW *plf, TEXTMETRICW * ptm, UINT *psz)
{
    HFONT hfont, hfontsav;
    TEXTMETRICW tm;
    if( !ptm) ptm = &tm;
    hfont = CreateFontIndirectW( plf);
    if( !hfont || ( hfontsav = SelectObject( hdc, hfont)) == NULL ) {
        ptm->tmHeight = -1;
        if( psz) *psz = 10;
        if( hfont) DeleteObject( hfont);
        return;
    }
    GetTextMetricsW( hdc, ptm);
    if( psz)
        if( !(*psz = GdiGetCharDimensions( hdc, ptm, NULL)))
            *psz = 10;
    SelectObject( hdc, hfontsav);
    DeleteObject( hfont);
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
        HKEY key;
        /* This must be non-volatile! */
        if (RegCreateKeyExW( HKEY_CURRENT_USER, WINE_CURRENT_USER_REGKEY,
                             0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 0,
                             &key, 0 ) != ERROR_SUCCESS)
        {
            ERR("Can't create wine registry branch\n");
        }
        else
        {
            /* @@ Wine registry key: HKCU\Software\Wine\Temporary System Parameters */
            if (RegCreateKeyExW( key, WINE_CURRENT_USER_REGKEY_TEMP_PARAMS,
                                 0, 0, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, 0,
                                 &volatile_key, 0 ) != ERROR_SUCCESS)
                ERR("Can't create non-permanent wine registry branch\n");

            RegCloseKey(key);
        }
    }
    return volatile_key;
}

/***********************************************************************
 *           SYSPARAMS_NotifyChange
 *
 * Sends notification about system parameter update.
 */
static void SYSPARAMS_NotifyChange( UINT uiAction, UINT fWinIni )
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
static BOOL SYSPARAMS_LoadRaw( LPCWSTR lpRegKey, LPCWSTR lpValName, void *lpBuf, DWORD count )
{
    BOOL ret = FALSE;
    DWORD type;
    HKEY hKey;

    memset( lpBuf, 0, count );

    if (RegOpenKeyW( get_volatile_regkey(), lpRegKey, &hKey ) == ERROR_SUCCESS)
    {
        ret = !RegQueryValueExW( hKey, lpValName, NULL, &type, lpBuf, &count );
        RegCloseKey( hKey );
    }

    if (!ret && RegOpenKeyW( HKEY_CURRENT_USER, lpRegKey, &hKey ) == ERROR_SUCCESS)
    {
        ret = !RegQueryValueExW( hKey, lpValName, NULL, &type, lpBuf, &count );
        RegCloseKey( hKey );
    }

    return ret;
}

static BOOL SYSPARAMS_Load( LPCWSTR lpRegKey, LPCWSTR lpValName, LPWSTR lpBuf, DWORD count )
{
  return SYSPARAMS_LoadRaw( lpRegKey, lpValName, (LPBYTE)lpBuf, count );
}

/***********************************************************************
 * Saves system parameter to user profile.
 */

/* Save data as-is */
static BOOL SYSPARAMS_SaveRaw( LPCWSTR lpRegKey, LPCWSTR lpValName, 
                               const void *lpValue, DWORD valueSize,
                               DWORD type, UINT fWinIni )
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
        if (RegSetValueExW( hKey, lpValName, 0, type,
                            lpValue, valueSize) == ERROR_SUCCESS)
        {
            ret = TRUE;
            if (hBaseKey == HKEY_CURRENT_USER)
                RegDeleteKeyW( get_volatile_regkey(), lpRegKey );
        }
        RegCloseKey( hKey );
    }
    return ret;
}

/* Convenience function to save strings */
static BOOL SYSPARAMS_Save( LPCWSTR lpRegKey, LPCWSTR lpValName, LPCWSTR lpValue,
                            UINT fWinIni )
{
    return SYSPARAMS_SaveRaw( lpRegKey, lpValName, (const BYTE*)lpValue, 
        (strlenW(lpValue) + 1)*sizeof(WCHAR), REG_SZ, fWinIni );
}

/* Convenience function to save logical fonts */
static BOOL SYSPARAMS_SaveLogFont( LPCWSTR lpRegKey, LPCWSTR lpValName,
                                   const LOGFONTW *plf, UINT fWinIni )
{
    LOGFONTW lf = *plf;
    int len;

    /* Zero pad the end of lfFaceName so we don't write uninitialised
       data to the registry */
    lf.lfFaceName[LF_FACESIZE-1] = 0;
    len = strlenW(lf.lfFaceName);
    if(len < LF_FACESIZE-1)
        memset(lf.lfFaceName + len, 0, (LF_FACESIZE - 1 - len) * sizeof(WCHAR));

    return SYSPARAMS_SaveRaw( lpRegKey, lpValName, (const BYTE*)&lf,
                              sizeof(LOGFONTW), REG_BINARY, fWinIni );
}


static inline HDC get_display_dc(void)
{
    static const WCHAR DISPLAY[] = {'D','I','S','P','L','A','Y',0};
    static HDC display_dc;
    if (!display_dc)
    {
        display_dc = CreateICW( DISPLAY, NULL, NULL, NULL );
        __wine_make_gdi_object_system( display_dc, TRUE );
    }
    return display_dc;
}

static inline int get_display_dpi(void)
{
    static int display_dpi;
    if (!display_dpi) display_dpi = GetDeviceCaps( get_display_dc(), LOGPIXELSY );
    return display_dpi;
}

/***********************************************************************
 * SYSPARAMS_Twips2Pixels
 *
 * Convert a dimension value that was obtained from the registry.  These are
 * quoted as being "twips" values if negative and pixels if positive.
 * One inch is 1440 twips. So to convert, divide by 1440 to get inches and
 * multiply that by the dots-per-inch to get the size in pixels. 
 * See for example
 *   MSDN Library - April 2001 -> Resource Kits ->
 *       Windows 2000 Resource Kit Reference ->
 *       Technical Reference to the Windows 2000 Registry ->
 *       HKEY_CURRENT_USER -> Control Panel -> Desktop -> WindowMetrics
 */
static inline int SYSPARAMS_Twips2Pixels(int x)
{
    if (x < 0)
        x = (-x * get_display_dpi() + 720) / 1440;
    return x;
}

/***********************************************************************
 * get_reg_metric
 *
 * Get a registry entry from the already open key.  This allows us to open the
 * section once and read several values.
 *
 * Of course this function belongs somewhere more usable but here will do
 * for now.
 */
static int get_reg_metric( HKEY hkey, LPCWSTR lpValName, int default_value )
{
    int value = default_value;
    if (hkey)
    {
        WCHAR buffer[128];
        DWORD type, count = sizeof(buffer);
        if(!RegQueryValueExW( hkey, lpValName, NULL, &type, (LPBYTE)buffer, &count) )
        {
            if (type != REG_SZ)
            {
                /* Are there any utilities for converting registry entries
                 * between formats?
                 */
                /* FIXME_(reg)("We need reg format converter\n"); */
            }
            else
                value = atoiW(buffer);
        }
    }
    return SYSPARAMS_Twips2Pixels(value);
}


/*************************************************************************
 *             SYSPARAMS_SetSysColor
 */
static void SYSPARAMS_SetSysColor( int index, COLORREF color )
{
    if (index < 0 || index >= NUM_SYS_COLORS) return;
    SysColors[index] = color;
    if (SysColorBrushes[index])
    {
        __wine_make_gdi_object_system( SysColorBrushes[index], FALSE);
        DeleteObject( SysColorBrushes[index] );
    }
    SysColorBrushes[index] = CreateSolidBrush( color );
    __wine_make_gdi_object_system( SysColorBrushes[index], TRUE);

    if (SysColorPens[index])
    {
        __wine_make_gdi_object_system( SysColorPens[index], FALSE);
        DeleteObject( SysColorPens[index] );
    }
    SysColorPens[index] = CreatePen( PS_SOLID, 1, color );
    __wine_make_gdi_object_system( SysColorPens[index], TRUE);
}

/* save an int parameter in registry */
static BOOL save_int_param( LPCWSTR regkey, LPCWSTR value, INT *value_ptr,
                            INT new_val, UINT fWinIni )
{
    WCHAR buf[12];

    wsprintfW(buf, CSd, new_val);
    if (!SYSPARAMS_Save( regkey, value, buf, fWinIni )) return FALSE;
    if( value_ptr) *value_ptr = new_val;
    return TRUE;
}

/* load a boolean parameter from the user preference key */
static BOOL get_user_pref_param( UINT offset, UINT mask, BOOL *ret_ptr )
{
    if (!ret_ptr) return FALSE;

    if (!spi_loaded[SPI_USERPREFERENCEMASK_IDX])
    {
        SYSPARAMS_LoadRaw( SPI_USERPREFERENCEMASK_REGKEY,
                           SPI_USERPREFERENCEMASK_VALNAME,
                           user_prefs, sizeof(user_prefs) );
        spi_loaded[SPI_USERPREFERENCEMASK_IDX] = TRUE;
    }
    *ret_ptr = (user_prefs[offset] & mask) != 0;
    return TRUE;
}

/* set a boolean parameter in the user preference key */
static BOOL set_user_pref_param( UINT offset, UINT mask, BOOL value, BOOL fWinIni )
{
    SYSPARAMS_LoadRaw( SPI_USERPREFERENCEMASK_REGKEY,
                       SPI_USERPREFERENCEMASK_VALNAME,
                       user_prefs, sizeof(user_prefs) );
    spi_loaded[SPI_USERPREFERENCEMASK_IDX] = TRUE;

    if (value) user_prefs[offset] |= mask;
    else user_prefs[offset] &= ~mask;

    SYSPARAMS_SaveRaw( SPI_USERPREFERENCEMASK_REGKEY,
                       SPI_USERPREFERENCEMASK_VALNAME,
                       user_prefs, sizeof(user_prefs), REG_BINARY, fWinIni );
    return TRUE;
}

/***********************************************************************
 *           SYSPARAMS_Init
 *
 * Initialisation of the system metrics array.
 */
void SYSPARAMS_Init(void)
{
    HKEY hkey; /* key to the window metrics area of the registry */
    int i, r, g, b;
    char buffer[100];
    HBITMAP h55AABitmap;

    /* initialize system colors */

    if (RegCreateKeyExA(HKEY_CURRENT_USER, "Control Panel\\Colors", 0, 0, 0, KEY_ALL_ACCESS, 0, &hkey, 0))
        hkey = 0;
    for (i = 0; i < NUM_SYS_COLORS; i++)
    {
        BOOL bOk = FALSE;

        /* first try, registry */
        if (hkey)
        {
            DWORD dwDataSize = sizeof(buffer);
            if (!(RegQueryValueExA(hkey,DefSysColors[i].name, 0, 0, (LPBYTE) buffer, &dwDataSize)))
                if (sscanf( buffer, "%d %d %d", &r, &g, &b ) == 3) bOk = TRUE;
        }

        /* second try, win.ini */
        if (!bOk)
        {
            GetProfileStringA( "colors", DefSysColors[i].name, NULL, buffer, 100 );
            if (sscanf( buffer, " %d %d %d", &r, &g, &b ) == 3) bOk = TRUE;
        }

        /* else, take the default */
        if (!bOk)
            SYSPARAMS_SetSysColor( i, DefSysColors[i].rgb );
        else
            SYSPARAMS_SetSysColor( i, RGB(r,g,b) );
    }
    if (hkey) RegCloseKey( hkey );

    /* create 55AA bitmap */

    h55AABitmap = CreateBitmap( 8, 8, 1, 1, wPattern55AA );
    SYSCOLOR_55AABrush = CreatePatternBrush( h55AABitmap );
    __wine_make_gdi_object_system( SYSCOLOR_55AABrush, TRUE );
}


/***********************************************************************
 *              reg_get_logfont
 *
 *  Tries to retrieve logfont info from the specified key and value
 */
static BOOL reg_get_logfont(LPCWSTR key, LPCWSTR value, LOGFONTW *lf)
{
    HKEY hkey;
    LOGFONTW lfbuf;
    DWORD type, size;
    BOOL found = FALSE;
    HKEY base_keys[2];
    int i;

    base_keys[0] = get_volatile_regkey();
    base_keys[1] = HKEY_CURRENT_USER;

    for(i = 0; i < 2 && !found; i++)
    {
        if(RegOpenKeyW(base_keys[i], key, &hkey) == ERROR_SUCCESS)
        {
            size = sizeof(lfbuf);
            if(RegQueryValueExW(hkey, value, NULL, &type, (LPBYTE)&lfbuf, &size) == ERROR_SUCCESS &&
                    type == REG_BINARY)
            {
                if( size == sizeof(lfbuf))
                {
                    found = TRUE;
                    memcpy(lf, &lfbuf, size);
                } else if( size == sizeof( LOGFONT16))
                {    /* win9x-winME format */
                    found = TRUE;
                    SYSPARAMS_LogFont16To32W( (LOGFONT16*) &lfbuf, lf);
                } else
                    WARN("Unknown format in key %s value %s, size is %d\n",
                            debugstr_w( key), debugstr_w( value), size);
            }
            RegCloseKey(hkey);
        }
    }
    if( found && lf->lfHeight > 0) { 
        /* positive height value means points ( inch/72 ) */
        lf->lfHeight = -MulDiv( lf->lfHeight, get_display_dpi(), 72);
    }
    return found;
}

/* adjust some of the raw values found in the registry */
static void normalize_nonclientmetrics( NONCLIENTMETRICSW *pncm)
{
    TEXTMETRICW tm;
    if( pncm->iBorderWidth < 1) pncm->iBorderWidth = 1;
    if( pncm->iCaptionWidth < 8) pncm->iCaptionWidth = 8;
    if( pncm->iScrollWidth < 8) pncm->iScrollWidth = 8;
    if( pncm->iScrollHeight < 8) pncm->iScrollHeight = 8;

    /* get some extra metrics */
    get_text_metr_size( get_display_dc(), &pncm->lfMenuFont,
            &tmMenuFont, NULL);
    get_text_metr_size( get_display_dc(), &pncm->lfCaptionFont,
            NULL, &CaptionFontAvCharWidth);

    /* adjust some heights to the corresponding font */
    pncm->iMenuHeight = max( pncm->iMenuHeight,
            2 + tmMenuFont.tmHeight + tmMenuFont.tmExternalLeading);
    get_text_metr_size( get_display_dc(), &pncm->lfCaptionFont, &tm, NULL);
    pncm->iCaptionHeight = max( pncm->iCaptionHeight, 2 + tm.tmHeight);
    get_text_metr_size( get_display_dc(), &pncm->lfSmCaptionFont, &tm, NULL);
    pncm->iSmCaptionHeight = max( pncm->iSmCaptionHeight, 2 + tm.tmHeight);
}

/* load all the non-client metrics */
static void load_nonclient_metrics(void)
{
    HKEY hkey;
    NONCLIENTMETRICSW ncm;
    INT r;

    ncm.cbSize = sizeof (ncm);
    if (RegOpenKeyExW (HKEY_CURRENT_USER, METRICS_REGKEY,
                       0, KEY_QUERY_VALUE, &hkey) != ERROR_SUCCESS) hkey = 0;

    /* initialize geometry entries */
    ncm.iBorderWidth =  get_reg_metric(hkey, METRICS_BORDERWIDTH_VALNAME, 1);
    ncm.iScrollWidth = get_reg_metric(hkey, METRICS_SCROLLWIDTH_VALNAME, 16);
    ncm.iScrollHeight = get_reg_metric(hkey, METRICS_SCROLLHEIGHT_VALNAME, 16);
    ncm.iPaddedBorderWidth = get_reg_metric(hkey, METRICS_PADDEDBORDERWIDTH_VALNAME, 0);

    /* size of the normal caption buttons */
    ncm.iCaptionHeight = get_reg_metric(hkey, METRICS_CAPTIONHEIGHT_VALNAME, 18);
    ncm.iCaptionWidth = get_reg_metric(hkey, METRICS_CAPTIONWIDTH_VALNAME, ncm.iCaptionHeight);

    /* caption font metrics */
    if (!reg_get_logfont(METRICS_REGKEY, METRICS_CAPTIONLOGFONT_VALNAME, &ncm.lfCaptionFont))
    {
        SystemParametersInfoW( SPI_GETICONTITLELOGFONT, 0, &ncm.lfCaptionFont, 0 );
        ncm.lfCaptionFont.lfWeight = FW_BOLD;
    }

    /* size of the small caption buttons */
    ncm.iSmCaptionWidth = get_reg_metric(hkey, METRICS_SMCAPTIONWIDTH_VALNAME, 13);
    ncm.iSmCaptionHeight = get_reg_metric(hkey, METRICS_SMCAPTIONHEIGHT_VALNAME, 15);

    /* small caption font metrics */
    if (!reg_get_logfont(METRICS_REGKEY, METRICS_SMCAPTIONLOGFONT_VALNAME, &ncm.lfSmCaptionFont))
        SystemParametersInfoW( SPI_GETICONTITLELOGFONT, 0, &ncm.lfSmCaptionFont, 0 );

    /* menus, FIXME: names of registry entries are bogus */

    /* size of the menu (MDI) buttons */
    ncm.iMenuHeight = get_reg_metric(hkey, METRICS_MENUHEIGHT_VALNAME, 18);
    ncm.iMenuWidth = get_reg_metric(hkey, METRICS_MENUWIDTH_VALNAME, ncm.iMenuHeight);

    /* menu font metrics */
    if (!reg_get_logfont(METRICS_REGKEY, METRICS_MENULOGFONT_VALNAME, &ncm.lfMenuFont))
    {
        SystemParametersInfoW( SPI_GETICONTITLELOGFONT, 0, &ncm.lfMenuFont, 0 );
        GetProfileStringW( Desktop, MenuFont, ncm.lfCaptionFont.lfFaceName,
                           ncm.lfMenuFont.lfFaceName, LF_FACESIZE );
        r = GetProfileIntW( Desktop, MenuFontSize, 0 );
        if (r)
            ncm.lfMenuFont.lfHeight = -r;
        ncm.lfMenuFont.lfWeight = FW_NORMAL;
    }

    /* status bar font metrics */
    if (!reg_get_logfont(METRICS_REGKEY, METRICS_STATUSLOGFONT_VALNAME, &ncm.lfStatusFont))
    {
        SystemParametersInfoW( SPI_GETICONTITLELOGFONT, 0, &ncm.lfStatusFont, 0 );
        GetProfileStringW( Desktop, StatusFont, ncm.lfCaptionFont.lfFaceName,
                           ncm.lfStatusFont.lfFaceName, LF_FACESIZE );
        r = GetProfileIntW( Desktop, StatusFontSize, 0 );
        if (r)
            ncm.lfStatusFont.lfHeight = -r;
        ncm.lfStatusFont.lfWeight = FW_NORMAL;
    }

    /* message font metrics */
    if (!reg_get_logfont(METRICS_REGKEY, METRICS_MESSAGELOGFONT_VALNAME, &ncm.lfMessageFont))
    {
        SystemParametersInfoW( SPI_GETICONTITLELOGFONT, 0, &ncm.lfMessageFont, 0 );
        GetProfileStringW( Desktop, MessageFont, ncm.lfCaptionFont.lfFaceName,
                           ncm.lfMessageFont.lfFaceName, LF_FACESIZE );
        r = GetProfileIntW( Desktop, MessageFontSize, 0 );
        if (r)
            ncm.lfMessageFont.lfHeight = -r;
        ncm.lfMessageFont.lfWeight = FW_NORMAL;
    }

    /* some extra fields not in the nonclient structure */
    icon_size.cx = icon_size.cy = get_reg_metric( hkey, METRICS_ICONSIZE_VALNAME, 32 );

    if (hkey) RegCloseKey( hkey );
    normalize_nonclientmetrics( &ncm);
    nonclient_metrics = ncm;
    spi_loaded[SPI_NONCLIENTMETRICS_IDX] = TRUE;
}

static BOOL CALLBACK enum_monitors( HMONITOR monitor, HDC hdc, LPRECT rect, LPARAM lp )
{
    MONITORINFO mi;

    mi.cbSize = sizeof(mi);
    if (GetMonitorInfoW( monitor, &mi ) && (mi.dwFlags & MONITORINFOF_PRIMARY))
    {
        LPRECT work = (LPRECT)lp;
        *work = mi.rcWork;
        return FALSE;
    }
    return TRUE;
}

/* load a uint parameter from the registry */
static BOOL get_uint_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param )
{
    if (!ptr_param) return FALSE;

    if (!entry->hdr.loaded)
    {
        WCHAR buf[32];

        if (SYSPARAMS_Load( entry->hdr.regkey, entry->hdr.regval, buf, sizeof(buf) ))
            entry->uint.val = atoiW( buf );
        entry->hdr.loaded = TRUE;
    }
    *(UINT *)ptr_param = entry->uint.val;
    return TRUE;
}

/* set a uint parameter in the registry */
static BOOL set_uint_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT flags )
{
    WCHAR buf[32];

    wsprintfW( buf, CSu, int_param );
    if (!SYSPARAMS_Save( entry->hdr.regkey, entry->hdr.regval, buf, flags )) return FALSE;
    if (entry->hdr.mirror) SYSPARAMS_Save( entry->hdr.mirror, entry->hdr.regval, buf, flags );
    entry->uint.val = int_param;
    entry->hdr.loaded = TRUE;
    return TRUE;
}

/* set an int parameter in the registry */
static BOOL set_int_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT flags )
{
    WCHAR buf[32];

    wsprintfW( buf, CSd, int_param );
    if (!SYSPARAMS_Save( entry->hdr.regkey, entry->hdr.regval, buf, flags )) return FALSE;
    if (entry->hdr.mirror) SYSPARAMS_Save( entry->hdr.mirror, entry->hdr.regval, buf, flags );
    entry->uint.val = int_param;
    entry->hdr.loaded = TRUE;
    return TRUE;
}

/* load a twips parameter from the registry */
static BOOL get_twips_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param )
{
    if (!ptr_param) return FALSE;

    if (!entry->hdr.loaded)
    {
        WCHAR buf[32];

        if (SYSPARAMS_Load( entry->hdr.regkey, entry->hdr.regval, buf, sizeof(buf) ))
            entry->uint.val = SYSPARAMS_Twips2Pixels( atoiW(buf) );
        entry->hdr.loaded = TRUE;
    }
    *(UINT *)ptr_param = entry->uint.val;
    return TRUE;
}

/* load a bool parameter from the registry */
static BOOL get_bool_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param )
{
    if (!ptr_param) return FALSE;

    if (!entry->hdr.loaded)
    {
        WCHAR buf[32];

        if (SYSPARAMS_Load( entry->hdr.regkey, entry->hdr.regval, buf, sizeof(buf) ))
            entry->bool.val = atoiW( buf ) != 0;
        entry->hdr.loaded = TRUE;
    }
    *(UINT *)ptr_param = entry->bool.val;
    return TRUE;
}

/* set a bool parameter in the registry */
static BOOL set_bool_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT flags )
{
    WCHAR buf[32];

    wsprintfW( buf, CSu, int_param != 0 );
    if (!SYSPARAMS_Save( entry->hdr.regkey, entry->hdr.regval, buf, flags )) return FALSE;
    if (entry->hdr.mirror) SYSPARAMS_Save( entry->hdr.mirror, entry->hdr.regval, buf, flags );
    entry->bool.val = int_param != 0;
    entry->hdr.loaded = TRUE;
    return TRUE;
}

/* load a bool parameter using Yes/No strings from the registry */
static BOOL get_yesno_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param )
{
    if (!ptr_param) return FALSE;

    if (!entry->hdr.loaded)
    {
        WCHAR buf[32];

        if (SYSPARAMS_Load( entry->hdr.regkey, entry->hdr.regval, buf, sizeof(buf) ))
            entry->bool.val = !lstrcmpiW( Yes, buf );
        entry->hdr.loaded = TRUE;
    }
    *(UINT *)ptr_param = entry->bool.val;
    return TRUE;
}

/* set a bool parameter using Yes/No strings from the registry */
static BOOL set_yesno_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT flags )
{
    const WCHAR *str = int_param ? Yes : No;

    if (!SYSPARAMS_Save( entry->hdr.regkey, entry->hdr.regval, str, flags )) return FALSE;
    if (entry->hdr.mirror) SYSPARAMS_Save( entry->hdr.mirror, entry->hdr.regval, str, flags );
    entry->bool.val = int_param != 0;
    entry->hdr.loaded = TRUE;
    return TRUE;
}

/* load a dword (binary) parameter from the registry */
static BOOL get_dword_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param )
{
    if (!ptr_param) return FALSE;

    if (!entry->hdr.loaded)
    {
        DWORD val;

        if (SYSPARAMS_LoadRaw( entry->hdr.regkey, entry->hdr.regval, &val, sizeof(val) ))
            entry->dword.val = val;
        entry->hdr.loaded = TRUE;
    }
    *(DWORD *)ptr_param = entry->bool.val;
    return TRUE;
}

/* set a dword (binary) parameter in the registry */
static BOOL set_dword_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT flags )
{
    DWORD val = PtrToUlong( ptr_param );

    if (!SYSPARAMS_SaveRaw( entry->hdr.regkey, entry->hdr.regval, &val, sizeof(val), REG_DWORD, flags ))
        return FALSE;
    if (entry->hdr.mirror)
        SYSPARAMS_SaveRaw( entry->hdr.mirror, entry->hdr.regval, &val, sizeof(val), REG_DWORD, flags );
    entry->dword.val = val;
    entry->hdr.loaded = TRUE;
    return TRUE;
}

/* load a font (binary) parameter from the registry */
static BOOL get_font_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param )
{
    if (!ptr_param) return FALSE;

    if (!entry->hdr.loaded)
    {
        LOGFONTW font;

        if (!reg_get_logfont( entry->hdr.regkey, entry->hdr.regval, &font )) return FALSE;
        entry->font.val = font;
        entry->hdr.loaded = TRUE;
    }
    *(LOGFONTW *)ptr_param = entry->font.val;
    return TRUE;
}

/* set a font (binary) parameter in the registry */
static BOOL set_font_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT flags )
{
    LOGFONTW font;
    WCHAR *ptr;

    memcpy( &font, ptr_param, sizeof(font) );
    /* zero pad the end of lfFaceName so we don't save uninitialised data */
    ptr = memchrW( font.lfFaceName, 0, LF_FACESIZE );
    if (ptr) memset( ptr, 0, (font.lfFaceName + LF_FACESIZE - ptr) * sizeof(WCHAR) );

    if (!SYSPARAMS_SaveRaw( entry->hdr.regkey, entry->hdr.regval, &font, sizeof(font), REG_BINARY, flags ))
        return FALSE;
    if (entry->hdr.mirror)
        SYSPARAMS_SaveRaw( entry->hdr.mirror, entry->hdr.regval, &font, sizeof(font), REG_BINARY, flags );
    entry->font.val = font;
    entry->hdr.loaded = TRUE;
    return TRUE;
}

static BOOL get_entry( void *ptr,  UINT int_param, void *ptr_param )
{
    union sysparam_all_entry *entry = ptr;
    return entry->hdr.get( entry, int_param, ptr_param );
}

static BOOL set_entry( void *ptr,  UINT int_param, void *ptr_param, UINT flags )
{
    union sysparam_all_entry *entry = ptr;
    return entry->hdr.set( entry, int_param, ptr_param, flags );
}

#define UINT_ENTRY(name,val) \
    struct sysparam_uint_entry entry_##name = { { get_uint_entry, set_uint_entry, \
                         SPI_SET ## name ##_REGKEY, SPI_SET ## name ##_VALNAME }, (val) }

#define UINT_ENTRY_MIRROR(name,val) \
    struct sysparam_uint_entry entry_##name = { { get_uint_entry, set_uint_entry, \
                         SPI_SET ## name ##_REGKEY, SPI_SET ## name ##_VALNAME, \
                         SPI_SET ## name ##_MIRROR }, (val) }

#define INT_ENTRY(name,val) \
    struct sysparam_uint_entry entry_##name = { { get_uint_entry, set_int_entry, \
                         SPI_SET ## name ##_REGKEY, SPI_SET ## name ##_VALNAME }, (val) }

#define BOOL_ENTRY(name,val) \
    struct sysparam_bool_entry entry_##name = { { get_bool_entry, set_bool_entry, \
                         SPI_SET ## name ##_REGKEY, SPI_SET ## name ##_VALNAME }, (val) }

#define BOOL_ENTRY_MIRROR(name,val) \
    struct sysparam_bool_entry entry_##name = { { get_bool_entry, set_bool_entry, \
                         SPI_SET ## name ##_REGKEY, SPI_SET ## name ##_VALNAME, \
                         SPI_SET ## name ##_MIRROR }, (val) }

#define YESNO_ENTRY(name,val) \
    struct sysparam_bool_entry entry_##name = { { get_yesno_entry, set_yesno_entry, \
                         SPI_SET ## name ##_REGKEY, SPI_SET ## name ##_VALNAME }, (val) }

#define TWIPS_ENTRY(name,val) \
    struct sysparam_uint_entry entry_##name = { { get_twips_entry, set_uint_entry, \
                         SPI_SET ## name ##_REGKEY, SPI_SET ## name ##_VALNAME }, (val) }

#define DWORD_ENTRY(name,val) \
    struct sysparam_dword_entry entry_##name = { { get_dword_entry, set_dword_entry, \
                         SPI_SET ## name ##_REGKEY, SPI_SET ## name ##_VALNAME }, (val) }

#define FONT_ENTRY(name) \
    struct sysparam_font_entry entry_##name = { { get_font_entry, set_font_entry, \
                         SPI_SET ## name ##_REGKEY, SPI_SET ## name ##_VALNAME } }

static UINT_ENTRY( CARETWIDTH, 1 );
static UINT_ENTRY( DRAGWIDTH, 4 );
static UINT_ENTRY( DRAGHEIGHT, 4 );
static UINT_ENTRY( DOUBLECLICKTIME, 500 );
static UINT_ENTRY( FONTSMOOTHING, 0 );
static UINT_ENTRY( GRIDGRANULARITY, 0 );
static UINT_ENTRY( KEYBOARDDELAY, 1 );
static UINT_ENTRY( KEYBOARDSPEED, 31 );
static UINT_ENTRY( MENUSHOWDELAY, 400 );
static UINT_ENTRY( METRICS_MINARRANGE, ARW_HIDE );
static UINT_ENTRY( METRICS_MINHORZGAP, 0 );
static UINT_ENTRY( METRICS_MINVERTGAP, 0 );
static UINT_ENTRY( METRICS_MINWIDTH, 154 );
static UINT_ENTRY( MOUSEHOVERHEIGHT, 4 );
static UINT_ENTRY( MOUSEHOVERTIME, 400 );
static UINT_ENTRY( MOUSEHOVERWIDTH, 4 );
static UINT_ENTRY( MOUSESPEED, 10 );
static UINT_ENTRY( MOUSETRAILS, 0 );
static UINT_ENTRY( SCREENSAVETIMEOUT, 300 );
static UINT_ENTRY( WHEELSCROLLCHARS, 3 );
static UINT_ENTRY( WHEELSCROLLLINES, 3 );
static UINT_ENTRY_MIRROR( DOUBLECLKHEIGHT, 4 );
static UINT_ENTRY_MIRROR( DOUBLECLKWIDTH, 4 );
static UINT_ENTRY_MIRROR( MENUDROPALIGNMENT, 0 );

static INT_ENTRY( MOUSETHRESHOLD1, 6 );
static INT_ENTRY( MOUSETHRESHOLD2, 10 );
static INT_ENTRY( MOUSEACCELERATION, 1 );

static BOOL_ENTRY( BLOCKSENDINPUTRESETS, FALSE );
static BOOL_ENTRY( DRAGFULLWINDOWS, FALSE );
static BOOL_ENTRY( KEYBOARDPREF, TRUE );
static BOOL_ENTRY( LOWPOWERACTIVE, FALSE );
static BOOL_ENTRY( MOUSEBUTTONSWAP, FALSE );
static BOOL_ENTRY( POWEROFFACTIVE, FALSE );
static BOOL_ENTRY( SCREENREADER, FALSE );
static BOOL_ENTRY( SCREENSAVERRUNNING, FALSE );
static BOOL_ENTRY( SHOWSOUNDS, FALSE );
static BOOL_ENTRY( SNAPTODEFBUTTON, FALSE );
static BOOL_ENTRY_MIRROR( ICONTITLEWRAP, TRUE );

static YESNO_ENTRY( BEEP, TRUE );

static TWIPS_ENTRY( BORDER, 1 );
static TWIPS_ENTRY( ICONHORIZONTALSPACING, 75 );
static TWIPS_ENTRY( ICONVERTICALSPACING, 75 );

static DWORD_ENTRY( ACTIVEWINDOWTRACKING, 0 );
static DWORD_ENTRY( ACTIVEWNDTRKTIMEOUT, 0 );
static DWORD_ENTRY( FOCUSBORDERHEIGHT, 1 );
static DWORD_ENTRY( FOCUSBORDERWIDTH, 1 );
static DWORD_ENTRY( FONTSMOOTHINGCONTRAST, 0 );
static DWORD_ENTRY( FONTSMOOTHINGORIENTATION, 0 );
static DWORD_ENTRY( FONTSMOOTHINGTYPE, 0 );
static DWORD_ENTRY( FOREGROUNDFLASHCOUNT, 3 );
static DWORD_ENTRY( FOREGROUNDLOCKTIMEOUT, 0 );
static DWORD_ENTRY( MOUSECLICKLOCKTIME, 1200 );

static FONT_ENTRY( ICONTITLELOGFONT );

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
        { \
            static BOOL warn = TRUE; \
            if (warn) \
            { \
                warn = FALSE; \
                FIXME( "Unimplemented action: %u (%s)\n", x, #x ); \
            } \
        } \
        SetLastError( ERROR_INVALID_SPI_VALUE ); \
        ret = FALSE; \
        break
#define WINE_SPI_WARN(x) \
    case x: \
        WARN( "Ignored action: %u (%s)\n", x, #x ); \
        break

    BOOL ret = TRUE;
    unsigned spi_idx = 0;

    switch (uiAction)
    {
    case SPI_GETBEEP:
        ret = get_entry( &entry_BEEP, uiParam, pvParam );
        break;
    case SPI_SETBEEP:
        ret = set_entry( &entry_BEEP, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETMOUSE:
        ret = get_entry( &entry_MOUSETHRESHOLD1, uiParam, (INT *)pvParam ) &&
              get_entry( &entry_MOUSETHRESHOLD2, uiParam, (INT *)pvParam + 1 ) &&
              get_entry( &entry_MOUSEACCELERATION, uiParam, (INT *)pvParam + 2 );
        break;
    case SPI_SETMOUSE:
        ret = set_entry( &entry_MOUSETHRESHOLD1, ((INT *)pvParam)[0], pvParam, fWinIni ) &&
              set_entry( &entry_MOUSETHRESHOLD2, ((INT *)pvParam)[1], pvParam, fWinIni ) &&
              set_entry( &entry_MOUSEACCELERATION, ((INT *)pvParam)[2], pvParam, fWinIni );
        break;
    case SPI_GETBORDER:
        ret = get_entry( &entry_BORDER, uiParam, pvParam );
        if (*(INT*)pvParam < 1) *(INT*)pvParam = 1;
        break;
    case SPI_SETBORDER:
        nonclient_metrics.iBorderWidth = uiParam > 0 ? uiParam : 1;
        /* raw value goes to registry */
        ret = set_entry( &entry_BORDER, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETKEYBOARDSPEED:
        ret = get_entry( &entry_KEYBOARDSPEED, uiParam, pvParam );
        break;
    case SPI_SETKEYBOARDSPEED:
        if (uiParam > 31) uiParam = 31;
        ret = set_entry( &entry_KEYBOARDSPEED, uiParam, pvParam, fWinIni );
        break;

    /* not implemented in Windows */
    WINE_SPI_WARN(SPI_LANGDRIVER);              /*     12 */

    case SPI_ICONHORIZONTALSPACING:
        if (pvParam != NULL)
            ret = get_entry( &entry_ICONHORIZONTALSPACING, uiParam, pvParam );
        else
            ret = set_entry( &entry_ICONHORIZONTALSPACING, max( 32, uiParam ), pvParam, fWinIni );
        break;
    case SPI_GETSCREENSAVETIMEOUT:
        ret = get_entry( &entry_SCREENSAVETIMEOUT, uiParam, pvParam );
        break;
    case SPI_SETSCREENSAVETIMEOUT:
        ret = set_entry( &entry_SCREENSAVETIMEOUT, uiParam, pvParam, fWinIni );
        break;

    case SPI_GETSCREENSAVEACTIVE:               /*     16 */
        if (!pvParam) return FALSE;
        *(BOOL *)pvParam = USER_Driver->pGetScreenSaveActive();
        break;

    case SPI_SETSCREENSAVEACTIVE:               /*     17 */
    {
        WCHAR buf[12];

        wsprintfW(buf, CSu, uiParam);
        USER_Driver->pSetScreenSaveActive( uiParam );
        /* saved value does not affect Wine */
        SYSPARAMS_Save( SPI_SETSCREENSAVEACTIVE_REGKEY,
                        SPI_SETSCREENSAVEACTIVE_VALNAME,
                        buf, fWinIni );
        break;
    }

    case SPI_GETGRIDGRANULARITY:
        ret = get_entry( &entry_GRIDGRANULARITY, uiParam, pvParam );
        break;
    case SPI_SETGRIDGRANULARITY:
        ret = set_entry( &entry_GRIDGRANULARITY, uiParam, pvParam, fWinIni );
        break;

    case SPI_SETDESKWALLPAPER:			/*     20 */
        if (!pvParam || !SetDeskWallPaper( pvParam )) return FALSE;
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
            ret = DESKTOP_SetPattern( pvParam );
	break;

    case SPI_GETKEYBOARDDELAY:
        ret = get_entry( &entry_KEYBOARDDELAY, uiParam, pvParam );
        break;
    case SPI_SETKEYBOARDDELAY:
        ret = set_entry( &entry_KEYBOARDDELAY, uiParam, pvParam, fWinIni );
        break;
    case SPI_ICONVERTICALSPACING:
        if (pvParam != NULL)
            ret = get_entry( &entry_ICONVERTICALSPACING, uiParam, pvParam );
        else
            ret = set_entry( &entry_ICONVERTICALSPACING, max( 32, uiParam ), pvParam, fWinIni );
        break;
    case SPI_GETICONTITLEWRAP:
        ret = get_entry( &entry_ICONTITLEWRAP, uiParam, pvParam );
        break;
    case SPI_SETICONTITLEWRAP:
        ret = set_entry( &entry_ICONTITLEWRAP, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETMENUDROPALIGNMENT:
        ret = get_entry( &entry_MENUDROPALIGNMENT, uiParam, pvParam );
        break;
    case SPI_SETMENUDROPALIGNMENT:
        ret = set_entry( &entry_MENUDROPALIGNMENT, uiParam, pvParam, fWinIni );
        break;
    case SPI_SETDOUBLECLKWIDTH:
        ret = set_entry( &entry_DOUBLECLKWIDTH, uiParam, pvParam, fWinIni );
        break;
    case SPI_SETDOUBLECLKHEIGHT:
        ret = set_entry( &entry_DOUBLECLKHEIGHT, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETICONTITLELOGFONT:
        if (!pvParam) return FALSE;
        if (!get_entry( &entry_ICONTITLELOGFONT, uiParam, pvParam ))
        {
            LOGFONTW lfDefault, *font = &entry_ICONTITLELOGFONT.val;
            INT r;

            /*
             * The 'default GDI fonts' seems to be returned.
             * If a returned font is not a correct font in your environment,
             * please try to fix objects/gdiobj.c at first.
             */
            GetObjectW( GetStockObject( DEFAULT_GUI_FONT ), sizeof(LOGFONTW), &lfDefault );

            GetProfileStringW( Desktop, IconTitleFaceName, lfDefault.lfFaceName,
                               font->lfFaceName, LF_FACESIZE );

            r = GetProfileIntW( Desktop, IconTitleSize, 0 );
            if (r)
                font->lfHeight = -r;
            else
                font->lfHeight = lfDefault.lfHeight;

            font->lfWidth = 0;
            font->lfEscapement = 0;
            font->lfOrientation = 0;
            font->lfWeight = FW_NORMAL;
            font->lfItalic = FALSE;
            font->lfStrikeOut = FALSE;
            font->lfUnderline = FALSE;
            font->lfCharSet = lfDefault.lfCharSet; /* at least 'charset' should not be hard-coded */
            font->lfOutPrecision = OUT_DEFAULT_PRECIS;
            font->lfClipPrecision = CLIP_DEFAULT_PRECIS;
            font->lfQuality = DEFAULT_QUALITY;
            font->lfPitchAndFamily = DEFAULT_PITCH;
            entry_ICONTITLELOGFONT.hdr.loaded = TRUE;
            *(LOGFONTW *)pvParam = *font;
        }
        break;
    case SPI_SETDOUBLECLICKTIME:
        ret = set_entry( &entry_DOUBLECLICKTIME, uiParam, pvParam, fWinIni );
        break;
    case SPI_SETMOUSEBUTTONSWAP:
        ret = set_entry( &entry_MOUSEBUTTONSWAP, uiParam, pvParam, fWinIni );
        break;
    case SPI_SETICONTITLELOGFONT:
        ret = set_entry( &entry_ICONTITLELOGFONT, uiParam, pvParam, fWinIni );
        break;

    case SPI_GETFASTTASKSWITCH:			/*     35 */
        if (!pvParam) return FALSE;
	*(BOOL *)pvParam = 1;
        break;

    case SPI_SETFASTTASKSWITCH:                 /*     36 */
        /* the action is disabled */
        ret = FALSE;
        break;

    case SPI_SETDRAGFULLWINDOWS:
        ret = set_entry( &entry_DRAGFULLWINDOWS, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETDRAGFULLWINDOWS:
        ret = get_entry( &entry_DRAGFULLWINDOWS, uiParam, pvParam );
        break;

    case SPI_GETNONCLIENTMETRICS:
    {
        LPNONCLIENTMETRICSW lpnm = pvParam;

        if (!lpnm)
        {
            ret = FALSE;
            break;
        }

        if (!spi_loaded[SPI_NONCLIENTMETRICS_IDX]) load_nonclient_metrics();

        if (lpnm->cbSize == sizeof(NONCLIENTMETRICSW))
            *lpnm = nonclient_metrics;
        else if (lpnm->cbSize == FIELD_OFFSET(NONCLIENTMETRICSW, iPaddedBorderWidth))
        {
            memcpy(lpnm, &nonclient_metrics, FIELD_OFFSET(NONCLIENTMETRICSW, iPaddedBorderWidth));
            lpnm->cbSize = FIELD_OFFSET(NONCLIENTMETRICSW, iPaddedBorderWidth);
        }
        else
            ret = FALSE;
        break;
    }

    case SPI_SETNONCLIENTMETRICS:
    {
        LPNONCLIENTMETRICSW lpnm = pvParam;

        if (lpnm && (lpnm->cbSize == sizeof(NONCLIENTMETRICSW) ||
                     lpnm->cbSize == FIELD_OFFSET(NONCLIENTMETRICSW, iPaddedBorderWidth)))
        {
            NONCLIENTMETRICSW ncm;
            ret = set_entry( &entry_BORDER, lpnm->iBorderWidth, NULL, fWinIni );
            if( ret) ret = save_int_param( METRICS_REGKEY,
                    METRICS_SCROLLWIDTH_VALNAME, NULL,
                    lpnm->iScrollWidth, fWinIni );
            if( ret) ret = save_int_param( METRICS_REGKEY,
                    METRICS_SCROLLHEIGHT_VALNAME, NULL,
                    lpnm->iScrollHeight, fWinIni );
            if( ret) ret = save_int_param( METRICS_REGKEY,
                    METRICS_CAPTIONWIDTH_VALNAME, NULL,
                    lpnm->iCaptionWidth, fWinIni );
            if( ret) ret = save_int_param( METRICS_REGKEY,
                    METRICS_CAPTIONHEIGHT_VALNAME, NULL,
                    lpnm->iCaptionHeight, fWinIni );
            if( ret) ret = save_int_param( METRICS_REGKEY,
                    METRICS_SMCAPTIONWIDTH_VALNAME, NULL,
                    lpnm->iSmCaptionWidth, fWinIni );
            if( ret) ret = save_int_param( METRICS_REGKEY,
                    METRICS_SMCAPTIONHEIGHT_VALNAME, NULL,
                    lpnm->iSmCaptionHeight, fWinIni );
            if( ret) ret = save_int_param( METRICS_REGKEY,
                    METRICS_MENUWIDTH_VALNAME, NULL,
                    lpnm->iMenuWidth, fWinIni );
            if( ret) ret = save_int_param( METRICS_REGKEY,
                    METRICS_MENUHEIGHT_VALNAME, NULL,
                    lpnm->iMenuHeight, fWinIni );
            if( ret) ret = SYSPARAMS_SaveLogFont(
                    METRICS_REGKEY, METRICS_MENULOGFONT_VALNAME,
                    &lpnm->lfMenuFont, fWinIni);
            if( ret) ret = SYSPARAMS_SaveLogFont(
                    METRICS_REGKEY, METRICS_CAPTIONLOGFONT_VALNAME,
                    &lpnm->lfCaptionFont, fWinIni);
            if( ret) ret = SYSPARAMS_SaveLogFont(
                    METRICS_REGKEY, METRICS_SMCAPTIONLOGFONT_VALNAME,
                    &lpnm->lfSmCaptionFont, fWinIni);
            if( ret) ret = SYSPARAMS_SaveLogFont(
                    METRICS_REGKEY, METRICS_STATUSLOGFONT_VALNAME,
                    &lpnm->lfStatusFont, fWinIni);
            if( ret) ret = SYSPARAMS_SaveLogFont(
                    METRICS_REGKEY, METRICS_MESSAGELOGFONT_VALNAME,
                    &lpnm->lfMessageFont, fWinIni);
            if( ret) {
                memcpy(&ncm, lpnm, FIELD_OFFSET(NONCLIENTMETRICSW, iPaddedBorderWidth));
                ncm.cbSize = sizeof(ncm);
                ncm.iPaddedBorderWidth = (lpnm->cbSize == sizeof(NONCLIENTMETRICSW)) ?
                                         lpnm->iPaddedBorderWidth : 0;
                normalize_nonclientmetrics( &ncm);
                nonclient_metrics = ncm;
                spi_loaded[SPI_NONCLIENTMETRICS_IDX] = TRUE;
            }
        }
        break;
    }

    case SPI_GETMINIMIZEDMETRICS:
    {
        MINIMIZEDMETRICS * lpMm = pvParam;
        if (lpMm && lpMm->cbSize == sizeof(*lpMm)) {
            get_entry( &entry_METRICS_MINWIDTH, 0, &lpMm->iWidth );
            get_entry( &entry_METRICS_MINHORZGAP, 0, &lpMm->iHorzGap );
            get_entry( &entry_METRICS_MINVERTGAP, 0, &lpMm->iVertGap );
            get_entry( &entry_METRICS_MINARRANGE, 0, &lpMm->iArrange );
            lpMm->iWidth = max( 0, lpMm->iWidth );
            lpMm->iHorzGap = max( 0, lpMm->iHorzGap );
            lpMm->iVertGap = max( 0, lpMm->iVertGap );
            lpMm->iArrange &= 0x0f;
        } else
            ret = FALSE;
        break;
    }
    case SPI_SETMINIMIZEDMETRICS:
    {
        MINIMIZEDMETRICS * lpMm = pvParam;
        if (lpMm && lpMm->cbSize == sizeof(*lpMm))
            ret = set_entry( &entry_METRICS_MINWIDTH, max( 0, lpMm->iWidth ), NULL, fWinIni ) &&
                  set_entry( &entry_METRICS_MINHORZGAP, max( 0, lpMm->iHorzGap ), NULL, fWinIni ) &&
                  set_entry( &entry_METRICS_MINVERTGAP, max( 0, lpMm->iVertGap ), NULL, fWinIni ) &&
                  set_entry( &entry_METRICS_MINARRANGE, lpMm->iArrange & 0x0f, NULL, fWinIni );
        else
            ret = FALSE;
        break;
    }
    case SPI_GETICONMETRICS:
    {
	LPICONMETRICSW lpIcon = pvParam;
	if(lpIcon && lpIcon->cbSize == sizeof(*lpIcon))
	{
            get_entry( &entry_ICONHORIZONTALSPACING, 0, &lpIcon->iHorzSpacing );
            get_entry( &entry_ICONVERTICALSPACING, 0, &lpIcon->iVertSpacing );
            get_entry( &entry_ICONTITLEWRAP, 0, &lpIcon->iTitleWrap );
            get_entry( &entry_ICONTITLELOGFONT, 0, &lpIcon->lfFont );
	}
	else ret = FALSE;
	break;
    }
    case SPI_SETICONMETRICS:
    {
        LPICONMETRICSW lpIcon = pvParam;
        if (lpIcon && lpIcon->cbSize == sizeof(*lpIcon))
            ret = set_entry( &entry_ICONVERTICALSPACING, max(32,lpIcon->iVertSpacing), NULL, fWinIni ) &&
                  set_entry( &entry_ICONHORIZONTALSPACING, max(32,lpIcon->iHorzSpacing), NULL, fWinIni ) &&
                  set_entry( &entry_ICONTITLEWRAP, lpIcon->iTitleWrap, NULL, fWinIni ) &&
                  set_entry( &entry_ICONTITLELOGFONT, 0, &lpIcon->lfFont, fWinIni );
        else
            ret = FALSE;
        break;
    }

    case SPI_SETWORKAREA:                       /*     47  WINVER >= 0x400 */
    {
        if (!pvParam) return FALSE;

        spi_idx = SPI_SETWORKAREA_IDX;
        CopyRect( &work_area, pvParam );
        spi_loaded[spi_idx] = TRUE;
        break;
    }

    case SPI_GETWORKAREA:                       /*     48  WINVER >= 0x400 */
    {
        if (!pvParam) return FALSE;

        spi_idx = SPI_SETWORKAREA_IDX;
        if (!spi_loaded[spi_idx])
        {
            SetRect( &work_area, 0, 0,
                     GetSystemMetrics( SM_CXSCREEN ),
                     GetSystemMetrics( SM_CYSCREEN ) );
            EnumDisplayMonitors( 0, NULL, enum_monitors, (LPARAM)&work_area );
            spi_loaded[spi_idx] = TRUE;
        }
        CopyRect( pvParam, &work_area );
        TRACE("work area %s\n", wine_dbgstr_rect( &work_area ));
        break;
    }

    WINE_SPI_FIXME(SPI_SETPENWINDOWS);		/*     49  WINVER >= 0x400 */

    case SPI_GETFILTERKEYS:                     /*     50 */
    {
        LPFILTERKEYS lpFilterKeys = pvParam;
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
        LPTOGGLEKEYS lpToggleKeys = pvParam;
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
        LPMOUSEKEYS lpMouseKeys = pvParam;
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

    case SPI_GETSHOWSOUNDS:
        ret = get_entry( &entry_SHOWSOUNDS, uiParam, pvParam );
        break;
    case SPI_SETSHOWSOUNDS:
        ret = set_entry( &entry_SHOWSOUNDS, uiParam, pvParam, fWinIni );
        break;

    case SPI_GETSTICKYKEYS:                     /*     58 */
    {
        LPSTICKYKEYS lpStickyKeys = pvParam;
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
        LPACCESSTIMEOUT lpAccessTimeout = pvParam;
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
        LPSERIALKEYSW lpSerialKeysW = pvParam;
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
        LPSOUNDSENTRYW lpSoundSentryW = pvParam;
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
        LPHIGHCONTRASTW lpHighContrastW = pvParam;
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

    case SPI_GETKEYBOARDPREF:
        ret = get_entry( &entry_KEYBOARDPREF, uiParam, pvParam );
        break;
    case SPI_SETKEYBOARDPREF:
        ret = set_entry( &entry_KEYBOARDPREF, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETSCREENREADER:
        ret = get_entry( &entry_SCREENREADER, uiParam, pvParam );
        break;
    case SPI_SETSCREENREADER:
        ret = set_entry( &entry_SCREENREADER, uiParam, pvParam, fWinIni );
        break;

    case SPI_GETANIMATION:			/*     72  WINVER >= 0x400 */
    {
        LPANIMATIONINFO lpAnimInfo = pvParam;

	/* Tell it "disabled" */
	if (lpAnimInfo && lpAnimInfo->cbSize == sizeof(ANIMATIONINFO))
	    lpAnimInfo->iMinAnimate = 0; /* Minimise and restore animation is disabled (nonzero == enabled) */
	else
	    ret = FALSE;
	break;
    }
    WINE_SPI_WARN(SPI_SETANIMATION);		/*     73  WINVER >= 0x400 */

    case SPI_GETFONTSMOOTHING:
        ret = get_entry( &entry_FONTSMOOTHING, uiParam, pvParam );
        if (ret) *(UINT *)pvParam = (*(UINT *)pvParam != 0);
        break;
    case SPI_SETFONTSMOOTHING:
        uiParam = uiParam ? 2 : 0; /* Win NT4/2k/XP behavior */
        ret = set_entry( &entry_FONTSMOOTHING, uiParam, pvParam, fWinIni );
        break;
    case SPI_SETDRAGWIDTH:
        ret = set_entry( &entry_DRAGWIDTH, uiParam, pvParam, fWinIni );
        break;
    case SPI_SETDRAGHEIGHT:
        ret = set_entry( &entry_DRAGHEIGHT, uiParam, pvParam, fWinIni );
        break;

    WINE_SPI_FIXME(SPI_SETHANDHELD);		/*     78  WINVER >= 0x400 */

    WINE_SPI_FIXME(SPI_GETLOWPOWERTIMEOUT);	/*     79  WINVER >= 0x400 */
    WINE_SPI_FIXME(SPI_GETPOWEROFFTIMEOUT);	/*     80  WINVER >= 0x400 */
    WINE_SPI_FIXME(SPI_SETLOWPOWERTIMEOUT);	/*     81  WINVER >= 0x400 */
    WINE_SPI_FIXME(SPI_SETPOWEROFFTIMEOUT);	/*     82  WINVER >= 0x400 */

    case SPI_GETLOWPOWERACTIVE:
        ret = get_entry( &entry_LOWPOWERACTIVE, uiParam, pvParam );
        break;
    case SPI_SETLOWPOWERACTIVE:
        ret = set_entry( &entry_LOWPOWERACTIVE, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETPOWEROFFACTIVE:
        ret = get_entry( &entry_POWEROFFACTIVE, uiParam, pvParam );
        break;
    case SPI_SETPOWEROFFACTIVE:
        ret = set_entry( &entry_POWEROFFACTIVE, uiParam, pvParam, fWinIni );
        break;

    WINE_SPI_FIXME(SPI_SETCURSORS);		/*     87  WINVER >= 0x400 */
    WINE_SPI_FIXME(SPI_SETICONS);		/*     88  WINVER >= 0x400 */

    case SPI_GETDEFAULTINPUTLANG: 	/*     89  WINVER >= 0x400 */
        ret = GetKeyboardLayout(0) != 0;
        break;

    WINE_SPI_FIXME(SPI_SETDEFAULTINPUTLANG);	/*     90  WINVER >= 0x400 */

    WINE_SPI_FIXME(SPI_SETLANGTOGGLE);		/*     91  WINVER >= 0x400 */

    case SPI_GETWINDOWSEXTENSION:		/*     92  WINVER >= 0x400 */
	WARN("pretend no support for Win9x Plus! for now.\n");
	ret = FALSE; /* yes, this is the result value */
	break;
    case SPI_SETMOUSETRAILS:
        ret = set_entry( &entry_MOUSETRAILS, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETMOUSETRAILS:
        ret = get_entry( &entry_MOUSETRAILS, uiParam, pvParam );
        break;
    case SPI_GETSNAPTODEFBUTTON:
        ret = get_entry( &entry_SNAPTODEFBUTTON, uiParam, pvParam );
        break;
    case SPI_SETSNAPTODEFBUTTON:
        ret = set_entry( &entry_SNAPTODEFBUTTON, uiParam, pvParam, fWinIni );
        break;
    case SPI_SETSCREENSAVERRUNNING:
        ret = set_entry( &entry_SCREENSAVERRUNNING, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETMOUSEHOVERWIDTH:
        ret = get_entry( &entry_MOUSEHOVERWIDTH, uiParam, pvParam );
        break;
    case SPI_SETMOUSEHOVERWIDTH:
        ret = set_entry( &entry_MOUSEHOVERWIDTH, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETMOUSEHOVERHEIGHT:
        ret = get_entry( &entry_MOUSEHOVERHEIGHT, uiParam, pvParam );
        break;
    case SPI_SETMOUSEHOVERHEIGHT:
        ret = set_entry( &entry_MOUSEHOVERHEIGHT, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETMOUSEHOVERTIME:
        ret = get_entry( &entry_MOUSEHOVERTIME, uiParam, pvParam );
        break;
    case SPI_SETMOUSEHOVERTIME:
        ret = set_entry( &entry_MOUSEHOVERTIME, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETWHEELSCROLLLINES:
        ret = get_entry( &entry_WHEELSCROLLLINES, uiParam, pvParam );
        break;
    case SPI_SETWHEELSCROLLLINES:
        ret = set_entry( &entry_WHEELSCROLLLINES, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETMENUSHOWDELAY:
        ret = get_entry( &entry_MENUSHOWDELAY, uiParam, pvParam );
        break;
    case SPI_SETMENUSHOWDELAY:
        ret = set_entry( &entry_MENUSHOWDELAY, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETWHEELSCROLLCHARS:
        ret = get_entry( &entry_WHEELSCROLLCHARS, uiParam, pvParam );
        break;
    case SPI_SETWHEELSCROLLCHARS:
        ret = set_entry( &entry_WHEELSCROLLCHARS, uiParam, pvParam, fWinIni );
        break;

    WINE_SPI_FIXME(SPI_GETSHOWIMEUI);		/*    110  _WIN32_WINNT >= 0x400 || _WIN32_WINDOW > 0x400 */
    WINE_SPI_FIXME(SPI_SETSHOWIMEUI);		/*    111  _WIN32_WINNT >= 0x400 || _WIN32_WINDOW > 0x400 */

    case SPI_GETMOUSESPEED:
        ret = get_entry( &entry_MOUSESPEED, uiParam, pvParam );
        break;
    case SPI_SETMOUSESPEED:
        ret = set_entry( &entry_MOUSESPEED, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETSCREENSAVERRUNNING:
        ret = get_entry( &entry_SCREENSAVERRUNNING, uiParam, pvParam );
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
            lstrcpynW(pvParam, buf, uiParam);
	}
	else
	{
	    /* Return an empty string */
            memset(pvParam, 0, uiParam);
	}

	break;
    }
    case SPI_GETACTIVEWINDOWTRACKING:
        ret = get_entry( &entry_ACTIVEWINDOWTRACKING, uiParam, pvParam );
        break;
    case SPI_SETACTIVEWINDOWTRACKING:
        ret = set_entry( &entry_ACTIVEWINDOWTRACKING, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETMENUANIMATION:             /* 0x1002  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
        ret = get_user_pref_param( 0, 0x02, pvParam );
        break;

    case SPI_SETMENUANIMATION:             /* 0x1003  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
        ret = set_user_pref_param( 0, 0x02, PtrToUlong(pvParam), fWinIni );
        break;

    case SPI_GETCOMBOBOXANIMATION:         /* 0x1004  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
        ret = get_user_pref_param( 0, 0x04, pvParam );
        break;

    case SPI_SETCOMBOBOXANIMATION:         /* 0x1005  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
        ret = set_user_pref_param( 0, 0x04, PtrToUlong(pvParam), fWinIni );
        break;

    case SPI_GETLISTBOXSMOOTHSCROLLING:    /* 0x1006  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
        ret = get_user_pref_param( 0, 0x08, pvParam );
        break;

    case SPI_SETLISTBOXSMOOTHSCROLLING:    /* 0x1007  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
        ret = set_user_pref_param( 0, 0x08, PtrToUlong(pvParam), fWinIni );
        break;

    case SPI_GETGRADIENTCAPTIONS:
        ret = get_user_pref_param( 0, 0x10, pvParam );
        break;

    case SPI_SETGRADIENTCAPTIONS:
        ret = set_user_pref_param( 0, 0x10, PtrToUlong(pvParam), fWinIni );
        break;

    case SPI_GETKEYBOARDCUES:
        ret = get_user_pref_param( 0, 0x20, pvParam );
        break;

    case SPI_SETKEYBOARDCUES:
        ret = set_user_pref_param( 0, 0x20, PtrToUlong(pvParam), fWinIni );
        break;

    WINE_SPI_FIXME(SPI_GETACTIVEWNDTRKZORDER);  /* 0x100C  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
    WINE_SPI_FIXME(SPI_SETACTIVEWNDTRKZORDER);  /* 0x100D  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
    case SPI_GETHOTTRACKING:
        ret = get_user_pref_param( 0, 0x80, pvParam );
        break;

    case SPI_SETHOTTRACKING:
        ret = set_user_pref_param( 0, 0x80, PtrToUlong(pvParam), fWinIni );
        break;

    WINE_SPI_FIXME(SPI_GETMENUFADE);            /* 0x1012  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
    WINE_SPI_FIXME(SPI_SETMENUFADE);            /* 0x1013  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
    case SPI_GETSELECTIONFADE:                  /* 0x1014  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
        ret = get_user_pref_param( 1, 0x04, pvParam );
        break;

    case SPI_SETSELECTIONFADE:                  /* 0x1015  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
        ret = set_user_pref_param( 1, 0x04, PtrToUlong(pvParam), fWinIni );
        break;

    case SPI_GETTOOLTIPANIMATION:               /* 0x1016  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
        ret = get_user_pref_param( 1, 0x08, pvParam );
        break;

    case SPI_SETTOOLTIPANIMATION:               /* 0x1017  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
        ret = set_user_pref_param( 1, 0x08, PtrToUlong(pvParam), fWinIni );
        break;

    case SPI_GETTOOLTIPFADE:                    /* 0x1018  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
        ret = get_user_pref_param( 1, 0x10, pvParam );
        break;

    case SPI_SETTOOLTIPFADE:                    /* 0x1019  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
        ret = set_user_pref_param( 1, 0x10, PtrToUlong(pvParam), fWinIni );
        break;

    case SPI_GETCURSORSHADOW:                   /* 0x101A  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
        ret = get_user_pref_param( 1, 0x20, pvParam );
        break;

    case SPI_SETCURSORSHADOW:                   /* 0x101B  _WIN32_WINNT >= 0x500 || _WIN32_WINDOW > 0x400 */
        ret = set_user_pref_param( 1, 0x20, PtrToUlong(pvParam), fWinIni );
        break;

    WINE_SPI_FIXME(SPI_GETMOUSESONAR);          /* 0x101C  _WIN32_WINNT >= 0x510 || _WIN32_WINDOW >= 0x490*/
    WINE_SPI_FIXME(SPI_SETMOUSESONAR);          /* 0x101D  _WIN32_WINNT >= 0x510 || _WIN32_WINDOW >= 0x490*/
    WINE_SPI_FIXME(SPI_GETMOUSECLICKLOCK);      /* 0x101E  _WIN32_WINNT >= 0x510 || _WIN32_WINDOW >= 0x490*/
    WINE_SPI_FIXME(SPI_SETMOUSECLICKLOCK);      /* 0x101F  _WIN32_WINNT >= 0x510 || _WIN32_WINDOW >= 0x490*/

    case SPI_GETMOUSEVANISH:
        ret = get_user_pref_param( 2, 0x01, pvParam );
        break;

    case SPI_SETMOUSEVANISH:
        ret = set_user_pref_param( 2, 0x01, PtrToUlong(pvParam), fWinIni );
        break;

    case SPI_GETFLATMENU:
        ret = get_user_pref_param( 2, 0x02, pvParam );
        break;

    case SPI_SETFLATMENU:
        ret = set_user_pref_param( 2, 0x02, PtrToUlong(pvParam), fWinIni );
        break;

    case SPI_GETDROPSHADOW:
        ret = get_user_pref_param( 2, 0x04, pvParam );
        break;

    case SPI_SETDROPSHADOW:
        ret = set_user_pref_param( 2, 0x04, PtrToUlong(pvParam), fWinIni );
        break;

    case SPI_GETBLOCKSENDINPUTRESETS:
        ret = get_entry( &entry_BLOCKSENDINPUTRESETS, uiParam, pvParam );
        break;
    case SPI_SETBLOCKSENDINPUTRESETS:
        ret = set_entry( &entry_BLOCKSENDINPUTRESETS, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETUIEFFECTS:
        ret = get_user_pref_param( 3, 0x80, pvParam );
        break;

    case SPI_SETUIEFFECTS:
        /* FIXME: this probably should mask other UI effect values when unset */
        ret = set_user_pref_param( 3, 0x80, PtrToUlong(pvParam), fWinIni );
        break;

    /* _WIN32_WINNT >= 0x600 */
    WINE_SPI_FIXME(SPI_GETDISABLEOVERLAPPEDCONTENT);
    WINE_SPI_FIXME(SPI_SETDISABLEOVERLAPPEDCONTENT);
    WINE_SPI_FIXME(SPI_GETCLIENTAREAANIMATION);
    WINE_SPI_FIXME(SPI_SETCLIENTAREAANIMATION);
    WINE_SPI_FIXME(SPI_GETCLEARTYPE);
    WINE_SPI_FIXME(SPI_SETCLEARTYPE);
    WINE_SPI_FIXME(SPI_GETSPEECHRECOGNITION);
    WINE_SPI_FIXME(SPI_SETSPEECHRECOGNITION);

    case SPI_GETFOREGROUNDLOCKTIMEOUT:
        ret = get_entry( &entry_FOREGROUNDLOCKTIMEOUT, uiParam, pvParam );
        break;
    case SPI_SETFOREGROUNDLOCKTIMEOUT:
        /* FIXME: this should check that the calling thread
         * is able to change the foreground window */
        ret = set_entry( &entry_FOREGROUNDLOCKTIMEOUT, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETACTIVEWNDTRKTIMEOUT:
        ret = get_entry( &entry_ACTIVEWNDTRKTIMEOUT, uiParam, pvParam );
        break;
    case SPI_SETACTIVEWNDTRKTIMEOUT:
        ret = get_entry( &entry_ACTIVEWNDTRKTIMEOUT, uiParam, pvParam );
        break;
    case SPI_GETFOREGROUNDFLASHCOUNT:
        ret = get_entry( &entry_FOREGROUNDFLASHCOUNT, uiParam, pvParam );
        break;
    case SPI_SETFOREGROUNDFLASHCOUNT:
        ret = set_entry( &entry_FOREGROUNDFLASHCOUNT, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETCARETWIDTH:
        ret = get_entry( &entry_CARETWIDTH, uiParam, pvParam );
        break;
    case SPI_SETCARETWIDTH:
        ret = set_entry( &entry_CARETWIDTH, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETMOUSECLICKLOCKTIME:
        ret = get_entry( &entry_MOUSECLICKLOCKTIME, uiParam, pvParam );
        break;
    case SPI_SETMOUSECLICKLOCKTIME:
        ret = set_entry( &entry_MOUSECLICKLOCKTIME, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETFONTSMOOTHINGTYPE:
        ret = get_entry( &entry_FONTSMOOTHINGTYPE, uiParam, pvParam );
        break;
    case SPI_SETFONTSMOOTHINGTYPE:
        ret = set_entry( &entry_FONTSMOOTHINGTYPE, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETFONTSMOOTHINGCONTRAST:
        ret = get_entry( &entry_FONTSMOOTHINGCONTRAST, uiParam, pvParam );
        break;
    case SPI_SETFONTSMOOTHINGCONTRAST:
        ret = set_entry( &entry_FONTSMOOTHINGCONTRAST, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETFOCUSBORDERWIDTH:
        ret = get_entry( &entry_FOCUSBORDERWIDTH, uiParam, pvParam );
        break;
    case SPI_GETFOCUSBORDERHEIGHT:
        ret = get_entry( &entry_FOCUSBORDERHEIGHT, uiParam, pvParam );
        break;
    case SPI_SETFOCUSBORDERWIDTH:
        ret = set_entry( &entry_FOCUSBORDERWIDTH, uiParam, pvParam, fWinIni );
        break;
    case SPI_SETFOCUSBORDERHEIGHT:
        ret = set_entry( &entry_FOCUSBORDERHEIGHT, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETFONTSMOOTHINGORIENTATION:
        ret = get_entry( &entry_FONTSMOOTHINGORIENTATION, uiParam, pvParam );
        break;
    case SPI_SETFONTSMOOTHINGORIENTATION:
        ret = set_entry( &entry_FONTSMOOTHINGORIENTATION, uiParam, pvParam, fWinIni );
        break;

    default:
	FIXME( "Unknown action: %u\n", uiAction );
	SetLastError( ERROR_INVALID_SPI_VALUE );
	ret = FALSE;
	break;
    }

    if (ret)
        SYSPARAMS_NotifyChange( uiAction, fWinIni );
    TRACE("(%u, %u, %p, %u) ret %d\n",
            uiAction, uiParam, pvParam, fWinIni, ret);
    return ret;

#undef WINE_SPI_FIXME
#undef WINE_SPI_WARN
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
            if (!MultiByteToWideChar( CP_ACP, 0, pvParam, -1, buffer,
                                      sizeof(buffer)/sizeof(WCHAR) ))
                buffer[sizeof(buffer)/sizeof(WCHAR)-1] = 0;
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
    UINT ret;

    /* some metrics are dynamic */
    switch (index)
    {
    case SM_CXSCREEN:
        return GetDeviceCaps( get_display_dc(), HORZRES );
    case SM_CYSCREEN:
        return GetDeviceCaps( get_display_dc(), VERTRES );
    case SM_CXVSCROLL:
        if (!spi_loaded[SPI_NONCLIENTMETRICS_IDX]) load_nonclient_metrics();
        return nonclient_metrics.iScrollWidth;
    case SM_CYHSCROLL:
        return GetSystemMetrics(SM_CXVSCROLL);
    case SM_CYCAPTION:
        if (!spi_loaded[SPI_NONCLIENTMETRICS_IDX]) load_nonclient_metrics();
        return nonclient_metrics.iCaptionHeight + 1;
    case SM_CXBORDER:
    case SM_CYBORDER:
        /* SM_C{X,Y}BORDER always returns 1 regardless of 'BorderWidth' value in registry */
        return 1;
    case SM_CXDLGFRAME:
    case SM_CYDLGFRAME:
        return 3;
    case SM_CYVTHUMB:
        if (!spi_loaded[SPI_NONCLIENTMETRICS_IDX]) load_nonclient_metrics();
        return nonclient_metrics.iScrollHeight;
    case SM_CXHTHUMB:
        return GetSystemMetrics(SM_CYVTHUMB);
    case SM_CXICON:
        if (!spi_loaded[SPI_NONCLIENTMETRICS_IDX]) load_nonclient_metrics();
        return icon_size.cx;
    case SM_CYICON:
        if (!spi_loaded[SPI_NONCLIENTMETRICS_IDX]) load_nonclient_metrics();
        return icon_size.cy;
    case SM_CXCURSOR:
    case SM_CYCURSOR:
        return 32;
    case SM_CYMENU:
        return GetSystemMetrics(SM_CYMENUSIZE) + 1;
    case SM_CXFULLSCREEN:
        /* see the remark for SM_CXMAXIMIZED, at least this formulation is
         * correct */
        return GetSystemMetrics( SM_CXMAXIMIZED) - 2 * GetSystemMetrics( SM_CXFRAME);
    case SM_CYFULLSCREEN:
        /* see the remark for SM_CYMAXIMIZED, at least this formulation is
         * correct */
        return GetSystemMetrics( SM_CYMAXIMIZED) - GetSystemMetrics( SM_CYMIN);
    case SM_CYKANJIWINDOW:
        return 0;
    case SM_MOUSEPRESENT:
        return 1;
    case SM_CYVSCROLL:
        if (!spi_loaded[SPI_NONCLIENTMETRICS_IDX]) load_nonclient_metrics();
        return nonclient_metrics.iScrollHeight;
    case SM_CXHSCROLL:
        if (!spi_loaded[SPI_NONCLIENTMETRICS_IDX]) load_nonclient_metrics();
        return nonclient_metrics.iScrollHeight;
    case SM_DEBUG:
        return 0;
    case SM_SWAPBUTTON:
        get_entry( &entry_MOUSEBUTTONSWAP, 0, &ret );
        return ret;
    case SM_RESERVED1:
    case SM_RESERVED2:
    case SM_RESERVED3:
    case SM_RESERVED4:
        return 0;
    case SM_CXMIN:
        if (!spi_loaded[SPI_NONCLIENTMETRICS_IDX]) load_nonclient_metrics();
        return 3 * nonclient_metrics.iCaptionWidth + GetSystemMetrics( SM_CYSIZE) +
            4 * CaptionFontAvCharWidth + 2 * GetSystemMetrics( SM_CXFRAME) + 4;
    case SM_CYMIN:
        return GetSystemMetrics( SM_CYCAPTION) + 2 * GetSystemMetrics( SM_CYFRAME);
    case SM_CXSIZE:
        if (!spi_loaded[SPI_NONCLIENTMETRICS_IDX]) load_nonclient_metrics();
        return nonclient_metrics.iCaptionWidth;
    case SM_CYSIZE:
        if (!spi_loaded[SPI_NONCLIENTMETRICS_IDX]) load_nonclient_metrics();
        return nonclient_metrics.iCaptionHeight;
    case SM_CXFRAME:
        if (!spi_loaded[SPI_NONCLIENTMETRICS_IDX]) load_nonclient_metrics();
        return GetSystemMetrics(SM_CXDLGFRAME) + nonclient_metrics.iBorderWidth;
    case SM_CYFRAME:
        if (!spi_loaded[SPI_NONCLIENTMETRICS_IDX]) load_nonclient_metrics();
        return GetSystemMetrics(SM_CYDLGFRAME) + nonclient_metrics.iBorderWidth;
    case SM_CXMINTRACK:
        return GetSystemMetrics(SM_CXMIN);
    case SM_CYMINTRACK:
        return GetSystemMetrics(SM_CYMIN);
    case SM_CXDOUBLECLK:
        get_entry( &entry_DOUBLECLKWIDTH, 0, &ret );
        return ret;
    case SM_CYDOUBLECLK:
        get_entry( &entry_DOUBLECLKHEIGHT, 0, &ret );
        return ret;
    case SM_CXICONSPACING:
        get_entry( &entry_ICONHORIZONTALSPACING, 0, &ret );
        return ret;
    case SM_CYICONSPACING:
        get_entry( &entry_ICONVERTICALSPACING, 0, &ret );
        return ret;
    case SM_MENUDROPALIGNMENT:
        get_entry( &entry_MENUDROPALIGNMENT, 0, &ret );
        return ret;
    case SM_PENWINDOWS:
        return 0;
    case SM_DBCSENABLED:
    {
        CPINFO cpinfo;
        GetCPInfo( CP_ACP, &cpinfo );
        return (cpinfo.MaxCharSize > 1);
    }
    case SM_CMOUSEBUTTONS:
        return 3;
    case SM_SECURE:
        return 0;
    case SM_CXEDGE:
        return GetSystemMetrics(SM_CXBORDER) + 1;
    case SM_CYEDGE:
        return GetSystemMetrics(SM_CYBORDER) + 1;
    case SM_CXMINSPACING:
        get_entry( &entry_METRICS_MINHORZGAP, 0, &ret );
        return GetSystemMetrics(SM_CXMINIMIZED) + max( 0, (INT)ret );
    case SM_CYMINSPACING:
        get_entry( &entry_METRICS_MINVERTGAP, 0, &ret );
        return GetSystemMetrics(SM_CYMINIMIZED) + max( 0, (INT)ret );
    case SM_CXSMICON:
    case SM_CYSMICON:
        return 16;
    case SM_CYSMCAPTION:
        return GetSystemMetrics(SM_CYSMSIZE) + 1;
    case SM_CXSMSIZE:
        if (!spi_loaded[SPI_NONCLIENTMETRICS_IDX]) load_nonclient_metrics();
        return nonclient_metrics.iSmCaptionWidth;
    case SM_CYSMSIZE:
        if (!spi_loaded[SPI_NONCLIENTMETRICS_IDX]) load_nonclient_metrics();
        return nonclient_metrics.iSmCaptionHeight;
    case SM_CXMENUSIZE:
        if (!spi_loaded[SPI_NONCLIENTMETRICS_IDX]) load_nonclient_metrics();
        return nonclient_metrics.iMenuWidth;
    case SM_CYMENUSIZE:
        if (!spi_loaded[SPI_NONCLIENTMETRICS_IDX]) load_nonclient_metrics();
        return nonclient_metrics.iMenuHeight;
    case SM_ARRANGE:
        get_entry( &entry_METRICS_MINARRANGE, 0, &ret );
        return ret & 0x0f;
    case SM_CXMINIMIZED:
        get_entry( &entry_METRICS_MINWIDTH, 0, &ret );
        return max( 0, (INT)ret ) + 6;
    case SM_CYMINIMIZED:
        if (!spi_loaded[SPI_NONCLIENTMETRICS_IDX]) load_nonclient_metrics();
        return nonclient_metrics.iCaptionHeight + 6;
    case SM_CXMAXTRACK:
        return GetSystemMetrics(SM_CXVIRTUALSCREEN) + 4 + 2 * GetSystemMetrics(SM_CXFRAME);
    case SM_CYMAXTRACK:
        return GetSystemMetrics(SM_CYVIRTUALSCREEN) + 4 + 2 * GetSystemMetrics(SM_CYFRAME);
    case SM_CXMAXIMIZED:
        /* FIXME: subtract the width of any vertical application toolbars*/
        return GetSystemMetrics(SM_CXSCREEN) + 2 * GetSystemMetrics(SM_CXFRAME);
    case SM_CYMAXIMIZED:
        /* FIXME: subtract the width of any horizontal application toolbars*/
        return GetSystemMetrics(SM_CYSCREEN) + 2 * GetSystemMetrics(SM_CYCAPTION);
    case SM_NETWORK:
        return 3;  /* FIXME */
    case SM_CLEANBOOT:
        return 0; /* 0 = ok, 1 = failsafe, 2 = failsafe + network */
    case SM_CXDRAG:
        get_entry( &entry_DRAGWIDTH, 0, &ret );
        return ret;
    case SM_CYDRAG:
        get_entry( &entry_DRAGHEIGHT, 0, &ret );
        return ret;
    case SM_SHOWSOUNDS:
        get_entry( &entry_SHOWSOUNDS, 0, &ret );
        return ret;
    case SM_CXMENUCHECK:
    case SM_CYMENUCHECK:
        if (!spi_loaded[SPI_NONCLIENTMETRICS_IDX]) load_nonclient_metrics();
        return tmMenuFont.tmHeight <= 0 ? 13 :
        ((tmMenuFont.tmHeight + tmMenuFont.tmExternalLeading + 1) / 2) * 2 - 1;
    case SM_SLOWMACHINE:
        return 0;  /* Never true */
    case SM_MIDEASTENABLED:
        return 0;  /* FIXME */
    case SM_MOUSEWHEELPRESENT:
        return 1;
    case SM_XVIRTUALSCREEN:
    {
        RECT rect = get_virtual_screen_rect();
        return rect.left;
    }
    case SM_YVIRTUALSCREEN:
    {
        RECT rect = get_virtual_screen_rect();
        return rect.top;
    }
    case SM_CXVIRTUALSCREEN:
    {
        RECT rect = get_virtual_screen_rect();
        return rect.right - rect.left;
    }
    case SM_CYVIRTUALSCREEN:
    {
        RECT rect = get_virtual_screen_rect();
        return rect.bottom - rect.top;
    }
    case SM_CMONITORS:
    {
        struct monitor_info info;
        get_monitors_info( &info );
        return info.count;
    }
    case SM_SAMEDISPLAYFORMAT:
        return 1;
    case SM_IMMENABLED:
        return 0;  /* FIXME */
    case SM_CXFOCUSBORDER:
    case SM_CYFOCUSBORDER:
        return 1;
    case SM_TABLETPC:
    case SM_MEDIACENTER:
        return 0;
    case SM_CMETRICS:
        return SM_CMETRICS;
    default:
        return 0;
    }
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


/**********************************************************************
 *		GetDoubleClickTime (USER32.@)
 */
UINT WINAPI GetDoubleClickTime(void)
{
    UINT time = 0;

    get_entry( &entry_DOUBLECLICKTIME, 0, &time );
    if (!time) time = 500;
    return time;
}


/*************************************************************************
 *		GetSysColor (USER32.@)
 */
COLORREF WINAPI DECLSPEC_HOTPATCH GetSysColor( INT nIndex )
{
    if (nIndex >= 0 && nIndex < NUM_SYS_COLORS)
        return SysColors[nIndex];
    else
        return 0;
}


/*************************************************************************
 *		SetSysColors (USER32.@)
 */
BOOL WINAPI SetSysColors( INT nChanges, const INT *lpSysColor,
                              const COLORREF *lpColorValues )
{
    int i;

    if (IS_INTRESOURCE(lpSysColor)) return FALSE; /* stupid app passes a color instead of an array */

    for (i = 0; i < nChanges; i++) SYSPARAMS_SetSysColor( lpSysColor[i], lpColorValues[i] );

    /* Send WM_SYSCOLORCHANGE message to all windows */

    SendMessageTimeoutW( HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0, SMTO_ABORTIFHUNG, 2000, NULL );

    /* Repaint affected portions of all visible windows */

    RedrawWindow( GetDesktopWindow(), NULL, 0,
                RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_ALLCHILDREN );
    return TRUE;
}


/*************************************************************************
 *		SetSysColorsTemp (USER32.@)
 *
 * UNDOCUMENTED !!
 *
 * Called by W98SE desk.cpl Control Panel Applet:
 * handle = SetSysColorsTemp(ptr, ptr, nCount);     ("set" call)
 * result = SetSysColorsTemp(NULL, NULL, handle);   ("restore" call)
 *
 * pPens is an array of COLORREF values, which seems to be used
 * to indicate the color values to create new pens with.
 *
 * pBrushes is an array of solid brush handles (returned by a previous
 * CreateSolidBrush), which seems to contain the brush handles to set
 * for the system colors.
 *
 * n seems to be used for
 *   a) indicating the number of entries to operate on (length of pPens,
 *      pBrushes)
 *   b) passing the handle that points to the previously used color settings.
 *      I couldn't figure out in hell what kind of handle this is on
 *      Windows. I just use a heap handle instead. Shouldn't matter anyway.
 *
 * RETURNS
 *     heap handle of our own copy of the current syscolors in case of
 *                 "set" call, i.e. pPens, pBrushes != NULL.
 *     TRUE (unconditionally !) in case of "restore" call,
 *          i.e. pPens, pBrushes == NULL.
 *     FALSE in case of either pPens != NULL and pBrushes == NULL
 *          or pPens == NULL and pBrushes != NULL.
 *
 * I'm not sure whether this implementation is 100% correct. [AM]
 */
DWORD_PTR WINAPI SetSysColorsTemp( const COLORREF *pPens, const HBRUSH *pBrushes, DWORD_PTR n)
{
    DWORD i;

    if (pPens && pBrushes) /* "set" call */
    {
        /* allocate our structure to remember old colors */
        LPVOID pOldCol = HeapAlloc(GetProcessHeap(), 0, sizeof(DWORD)+n*sizeof(HPEN)+n*sizeof(HBRUSH));
        LPVOID p = pOldCol;
        *(DWORD *)p = n; p = (char*)p + sizeof(DWORD);
        memcpy(p, SysColorPens, n*sizeof(HPEN)); p = (char*)p + n*sizeof(HPEN);
        memcpy(p, SysColorBrushes, n*sizeof(HBRUSH));

        for (i=0; i < n; i++)
        {
            SysColorPens[i] = CreatePen( PS_SOLID, 1, pPens[i] );
            SysColorBrushes[i] = pBrushes[i];
        }

        return (DWORD_PTR)pOldCol;
    }
    if (!pPens && !pBrushes) /* "restore" call */
    {
        LPVOID pOldCol = (LPVOID)n; /* FIXME: not 64-bit safe */
        LPVOID p = pOldCol;
        DWORD nCount = *(DWORD *)p;
        p = (char*)p + sizeof(DWORD);

        for (i=0; i < nCount; i++)
        {
            DeleteObject(SysColorPens[i]);
            SysColorPens[i] = *(HPEN *)p; p = (char*)p + sizeof(HPEN);
        }
        for (i=0; i < nCount; i++)
        {
            SysColorBrushes[i] = *(HBRUSH *)p; p = (char*)p + sizeof(HBRUSH);
        }
        /* get rid of storage structure */
        HeapFree(GetProcessHeap(), 0, pOldCol);

        return TRUE;
    }
    return FALSE;
}


/***********************************************************************
 *		GetSysColorBrush (USER32.@)
 */
HBRUSH WINAPI DECLSPEC_HOTPATCH GetSysColorBrush( INT index )
{
    if (0 <= index && index < NUM_SYS_COLORS) return SysColorBrushes[index];
    WARN("Unknown index(%d)\n", index );
    return NULL;
}


/***********************************************************************
 *		SYSCOLOR_GetPen
 */
HPEN SYSCOLOR_GetPen( INT index )
{
    /* We can assert here, because this function is internal to Wine */
    assert (0 <= index && index < NUM_SYS_COLORS);
    return SysColorPens[index];
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
    /* make sure the desktop window is created before mode changing */
    GetDesktopWindow();

    return USER_Driver->pChangeDisplaySettingsEx( devname, devmode, hwnd, flags, lparam );
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
 *		EnumDisplaySettingsExW (USER32.@)
 */
BOOL WINAPI EnumDisplaySettingsExW(LPCWSTR lpszDeviceName, DWORD iModeNum,
                                   LPDEVMODEW lpDevMode, DWORD dwFlags)
{
    /* make sure the desktop window is created before mode enumeration */
    GetDesktopWindow();

    return USER_Driver->pEnumDisplaySettingsEx(lpszDeviceName, iModeNum, lpDevMode, dwFlags);
}

/***********************************************************************
 *              SetProcessDPIAware   (USER32.@)
 */
BOOL WINAPI SetProcessDPIAware( VOID )
{
    FIXME( "stub!\n");

    return TRUE;
}
