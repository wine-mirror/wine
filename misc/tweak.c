/******************************************************************************
 *
 *   tweak.c
 *
 *   Windows 95 style interface tweaks.
 *   Copyright (c) 1997 Dave Cuthbert.
 *
 *   FIXME:  This file is, unfortunately, aptly named:  the method of
 *   displaying Win95 style windows is a tweak.  Lots of stuff does not yet
 *   work -- and probably never will unless some of this code is
 *   incorporated into the mainstream Wine code.
 *
 *   DEVELOPERS, PLEASE NOTE:  Before delving into the mainstream code and
 *   altering it, consider how your changes will affect the Win3.1 interface
 *   (which has taken a major effort to create!).  After you make any sort of
 *   non-trivial change, *test* the Wine code running in Win3.1 mode!  The
 *   object here is to make it so that the person who tests the latest version
 *   of Wine without adding the tweaks into wine.conf notices nothing out of
 *   the ordinary.
 *
 *   Revision history
 *        03-Jul-1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *             Original implementation.
 *
 *****************************************************************************/

#include <stdio.h>
#include <malloc.h>
#include <X11/Xlib.h>
#include <string.h>
#include "dc.h"
#include "debug.h"
#include "graphics.h"
#include "options.h"
#include "stackframe.h"
#include "syscolor.h"
#include "tweak.h"
#include "windows.h"

/* Parameters for windows/nonclient.c */
extern int  NC_CaptionLeftNudge;
extern int  NC_CaptionTopNudge;
extern int  NC_SysControlNudge;
extern int  NC_MaxControlNudge;
extern int  NC_MinControlNudge;
extern UINT32  NC_CaptionTextFlags;
extern HBRUSH32  NC_WinHighlight95;
extern HBRUSH32  NC_WinShadow95;

/* Parameters for controls/menu.c */
extern UINT32  MENU_BarItemTopNudge;
extern UINT32  MENU_BarItemLeftNudge;
extern UINT32  MENU_ItemTopNudge;
extern UINT32  MENU_ItemLeftNudge;
extern UINT32  MENU_HighlightTopNudge;
extern UINT32  MENU_HighlightLeftNudge;
extern UINT32  MENU_HighlightBottomNudge;
extern UINT32  MENU_HighlightRightNudge;

/* General options */
HPEN32  TWEAK_PenFF95;
HPEN32  TWEAK_PenE095;
HPEN32  TWEAK_PenC095;
HPEN32  TWEAK_Pen8095;
HPEN32  TWEAK_Pen0095;

#if defined(WIN_95_LOOK)
int  TWEAK_Win95Look = 1;
#else
int  TWEAK_Win95Look = 0;
#endif

int  TWEAK_WineInitialized = 0;


/******************************************************************************
 *
 *   int  TWEAK_MenuInit()
 * 
 *   Initializes the Win95 tweaks to the menu code.  See controls/menu.c.
 *   Return value indicates success (non-zero) or failure.
 *
 *   Revision history
 *        06-Jul-1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *             Original implementation.
 *
 *****************************************************************************/

static int  TWEAK_MenuInit()
{
    MENU_BarItemTopNudge =
	PROFILE_GetWineIniInt("Tweak.Layout", "MenuBarItemTopNudge", 0);
    MENU_BarItemLeftNudge =
	PROFILE_GetWineIniInt("Tweak.Layout", "MenuBarItemLeftNudge", 0);
    MENU_ItemTopNudge =
	PROFILE_GetWineIniInt("Tweak.Layout", "MenuItemTopNudge", 0);
    MENU_ItemLeftNudge =
	PROFILE_GetWineIniInt("Tweak.Layout", "MenuItemLeftNudge", 0);
    MENU_HighlightTopNudge =
	PROFILE_GetWineIniInt("Tweak.Layout", "MenuHighlightTopNudge", 0);
    MENU_HighlightLeftNudge =
	PROFILE_GetWineIniInt("Tweak.Layout", "MenuHighlightLeftNudge", 0);
    MENU_HighlightBottomNudge =
	PROFILE_GetWineIniInt("Tweak.Layout", "MenuHighlightBottomNudge", 0);
    MENU_HighlightRightNudge =
	PROFILE_GetWineIniInt("Tweak.Layout", "MenuHighlightRightNudge", 0);

    return 1;
}


/******************************************************************************
 *
 *   int  TWEAK_NonClientInit()
 *
 *   Initializes the Win95 tweaks to the non-client drawing functions.  See
 *   windows/nonclient.c.  Return value indicates success (non-zero) or
 *   failure.
 *
 *   Revision history
 *        05-Jul-1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *             Original implementation.
 *
 *****************************************************************************/

static int  TWEAK_NonClientInit()
{
    char  key_value[2];

    NC_CaptionLeftNudge =
	PROFILE_GetWineIniInt("Tweak.Layout", "CaptionLeftNudge", 0);
    NC_CaptionTopNudge =
	PROFILE_GetWineIniInt("Tweak.Layout", "CaptionTopNudge", 0);
    NC_SysControlNudge =
	PROFILE_GetWineIniInt("Tweak.Layout", "SysControlNudge", 0);
    NC_MaxControlNudge =
	PROFILE_GetWineIniInt("Tweak.Layout", "MaxControlNudge", 0);
    NC_MinControlNudge =
	PROFILE_GetWineIniInt("Tweak.Layout", "MinControlNudge", 0);

    NC_WinHighlight95 = CreateSolidBrush32(RGB(0xc0, 0xc0, 0xc0));
    NC_WinShadow95 = CreateSolidBrush32(RGB(0x00, 0x00, 0x00));

    NC_CaptionTextFlags = DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX;

    PROFILE_GetWineIniString("Tweak.Layout", "CaptionAlignment", 
			     TWEAK_Win95Look ? "l" : "c", key_value, 2);

    switch(key_value[0]) {
    case 'l':
    case 'L':
	NC_CaptionTextFlags |= DT_LEFT;
	break;

    case 'r':
    case 'R':
	NC_CaptionTextFlags |= DT_RIGHT;
	break;

    default:
	NC_CaptionTextFlags |= DT_CENTER;
    }

    return 1;
}


/******************************************************************************
 *
 *   int  TWEAK_VarInit()
 *
 *   Initializes the miscellaneous variables which are used in the tweak
 *   routines.  Return value is non-zero on success.
 *
 *   Revision history
 *        07-Jul-1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *             Original implementation.
 *
 *****************************************************************************/

static int  TWEAK_VarInit()
{
    TWEAK_Win95Look = PROFILE_GetWineIniBool("Tweak.Layout", "Win95Look", 0);

    /* FIXME: Each color should really occupy a single entry in the wine.conf
       file, but I couldn't settle on a good (intuitive!) format. */

    TWEAK_PenFF95 = CreatePen32(
	PS_SOLID, 1,
	RGB(PROFILE_GetWineIniInt("Tweak.Colors", "PenFF95.Red", 0xff),
	    PROFILE_GetWineIniInt("Tweak.Colors", "PenFF95.Grn", 0xff),
	    PROFILE_GetWineIniInt("Tweak.Colors", "PenFF95.Blu", 0xff)));
    TWEAK_PenE095 = CreatePen32(
	PS_SOLID, 1,
	RGB(PROFILE_GetWineIniInt("Tweak.Colors", "PenE095.Red", 0xe0),
	    PROFILE_GetWineIniInt("Tweak.Colors", "PenE095.Grn", 0xe0),
	    PROFILE_GetWineIniInt("Tweak.Colors", "PenE095.Blu", 0xe0)));
    TWEAK_PenC095 = CreatePen32(
	PS_SOLID, 1,
	RGB(PROFILE_GetWineIniInt("Tweak.Colors", "PenC095.Red", 0xc0),
	    PROFILE_GetWineIniInt("Tweak.Colors", "PenC095.Grn", 0xc0),
	    PROFILE_GetWineIniInt("Tweak.Colors", "PenC095.Blu", 0xc0)));
    TWEAK_Pen8095 = CreatePen32(
	PS_SOLID, 1,
	RGB(PROFILE_GetWineIniInt("Tweak.Colors", "Pen8095.Red", 0x80),
	    PROFILE_GetWineIniInt("Tweak.Colors", "Pen8095.Grn", 0x80),
	    PROFILE_GetWineIniInt("Tweak.Colors", "Pen8095.Blu", 0x80)));
    TWEAK_Pen0095 = CreatePen32(
	PS_SOLID, 1,
	RGB(PROFILE_GetWineIniInt("Tweak.Colors", "Pen0095.Red", 0x00),
	    PROFILE_GetWineIniInt("Tweak.Colors", "Pen0095.Grn", 0x00),
	    PROFILE_GetWineIniInt("Tweak.Colors", "Pen0095.Blu", 0x00)));

    dprintf_tweak(stddeb, "TWEAK_VarInit: Using %s look and feel.\n",
		  TWEAK_Win95Look ? "Win95" : "Win3.1");
    return 1;
}


/******************************************************************************
 *
 *   int  TWEAK_Init()
 *
 *   Does the full initialization of the Win95 tweak subsystem.  Return value
 *   indicates success.  Called by loader/main.c's MAIN_Init().
 *
 *   Revision history
 *        05-Jul-1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *             Original implementation.
 *
 *****************************************************************************/

int  TWEAK_Init()
{
    TWEAK_VarInit();
    TWEAK_NonClientInit();
    TWEAK_MenuInit();
    return 1;
}


/******************************************************************************
 *
 *   int  TWEAK_CheckOldFonts()
 *
 *   Examines wine.conf for old/invalid font entries and recommend changes to
 *   the user.
 *
 *   Revision history
 *        05-Jul-1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *             Original implementation.
 *
 *****************************************************************************/

static void  TWEAK_CheckOldFontsCallback(char const *, char const *, void *);

static char const  *fontmsgprologue = 
"Wine warning:\n"
"   The following entries in the [fonts] section of the wine.conf file are\n"
"   obsolete or invalid:\n";

static char const  *fontmsgepilogue =
"   These entries should be eliminated or updated.\n"
"   See the documentation/fonts file for more information.\n";

static int  TWEAK_CheckOldFonts()
{
    int  found = 0;

    PROFILE_EnumerateWineIniSection("Fonts", &TWEAK_CheckOldFontsCallback,
				    (void *)&found);
    if(found)
	fprintf(stderr, fontmsgepilogue);

    return 1;
}

static void  TWEAK_CheckOldFontsCallback(
    char const  *key,
    char const  *value,
    void  *found)
{
    /* Ignore any keys that start with potential comment characters "'", '#',
       or ';'. */
    if(key[0] == '\'' || key[0] == '#' || key[0] == ';' || key[0] == '\0')
	return;

    /* Make sure this is a valid key */
    if(strncasecmp(key, "Alias", 5) == 0 ||
       strcasecmp(key, "Default") == 0) {

	/* Valid key; make sure the value doesn't contain a wildcard */
	if(strchr(value, '*')) {
	    if(*(int *)found == 0) {
		fprintf(stderr, fontmsgprologue);
		++*(int *)found;
	    }
	    
	    fprintf(stderr, "     %s=%s [no wildcards allowed]\n", key, value);
	}
    }
    else {
	/* Not a valid key */
	if(*(int *)found == 0) {
	    fprintf(stderr, fontmsgprologue);
	    ++*(int *)found;
	}

	fprintf(stderr, "     %s=%s [obsolete]\n", key, value);
    }

    return;
}


/******************************************************************************
 *
 *   int  TWEAK_CheckConfiguration()
 *
 *   Examines wine.conf for old/bad entries and recommends changes to the user.
 *
 *   Revision history
 *        05-Jul-1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *             Original implementation.
 *
 *****************************************************************************/

int  TWEAK_CheckConfiguration()
{
    TWEAK_CheckOldFonts();
    return 1;
}



/******************************************************************************
 *
 *     Tweak graphic subsystem.
 *
 *****************************************************************************/

/******************************************************************************
 *
 *   void  TWEAK_DrawReliefRect95(
 *      HDC32  hdc,               // Device context on which to draw
 *      RECT32 const  *rect )     // Rectangle to use
 *
 *   Draws the double-bordered Win95-style relief rectangle.
 *
 *   Bugs
 *        There are some checks missing from this function.  Perhaps the
 *        SelectObject32 calls should be examined?  Hasn't failed on me (yet).
 *
 *        Should I really be calling X functions directly from here?  It is
 *        an optimization, but should I be optimizing alpha code?  Probably
 *        not.
 *
 *   Revision history
 *        08-Jul-1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *             Original implementation.
 *
 *****************************************************************************/

void  TWEAK_DrawReliefRect95(
    HDC32  hdc,
    RECT32 const  *rect )
{
    DC  *dc;
    HPEN32  prevpen;

    if((dc = (DC *)GDI_GetObjPtr(hdc, DC_MAGIC))) {

	/* Draw the top/left lines first */
	prevpen = SelectObject32(hdc, TWEAK_PenE095);
	DC_SetupGCForPen(dc);
	XDrawLine(display, dc->u.x.drawable, dc->u.x.gc, rect->left, rect->top,
		  rect->right - 1, rect->top);
	XDrawLine(display, dc->u.x.drawable, dc->u.x.gc, rect->left, rect->top,
		  rect->left, rect->bottom - 1);

	SelectObject32(hdc, TWEAK_PenFF95);
	DC_SetupGCForPen(dc);
	XDrawLine(display, dc->u.x.drawable, dc->u.x.gc, rect->left + 1,
		  rect->top + 1, rect->right - 2, rect->top + 1);
	XDrawLine(display, dc->u.x.drawable, dc->u.x.gc, rect->left + 1,
		  rect->top + 1, rect->left + 1, rect->bottom - 2);


	/* Now the bottom/right lines */
	SelectObject32(hdc, TWEAK_Pen0095);
	DC_SetupGCForPen(dc);
	XDrawLine(display, dc->u.x.drawable, dc->u.x.gc, rect->left,
		  rect->bottom - 1, rect->right - 1, rect->bottom - 1);
	XDrawLine(display, dc->u.x.drawable, dc->u.x.gc, rect->right - 1,
		  rect->top, rect->right - 1, rect->bottom - 1);

	SelectObject32(hdc, TWEAK_Pen8095);
	DC_SetupGCForPen(dc);
	XDrawLine(display, dc->u.x.drawable, dc->u.x.gc, rect->left + 1,
		  rect->bottom - 2, rect->right - 2, rect->bottom - 2);
	XDrawLine(display, dc->u.x.drawable, dc->u.x.gc, rect->right - 2,
		  rect->top + 1, rect->right - 2, rect->bottom - 2);
	
	SelectObject32(hdc, prevpen);
    }

    return;
}


/******************************************************************************
 *
 *   void  TWEAK_DrawRevReliefRect95(
 *      HDC32  hdc,               // Device context on which to draw
 *      RECT32 const  *rect )     // Rectangle to use
 *
 *   Draws the double-bordered Win95-style relief rectangle.
 *
 *   Bugs
 *        There are some checks missing from this function.  Perhaps the
 *        SelectObject32 calls should be examined?  Hasn't failed on me (yet).
 *
 *        Should I really be calling X functions directly from here?  It is
 *        an optimization, but should I be optimizing alpha code?  Probably
 *        not.
 *
 *   Revision history
 *        08-Jul-1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *             Original implementation.
 *
 *****************************************************************************/

void  TWEAK_DrawRevReliefRect95(
    HDC32  hdc,
    RECT32 const  *rect )
{
    DC  *dc;
    HPEN32  prevpen;

    if((dc = (DC *)GDI_GetObjPtr(hdc, DC_MAGIC))) {

	/* Draw the top/left lines first */
	prevpen = SelectObject32(hdc, TWEAK_Pen8095);
	DC_SetupGCForPen(dc);
	XDrawLine(display, dc->u.x.drawable, dc->u.x.gc, rect->left, rect->top,
		  rect->right - 1, rect->top);
	XDrawLine(display, dc->u.x.drawable, dc->u.x.gc, rect->left, rect->top,
		  rect->left, rect->bottom - 1);

	SelectObject32(hdc, TWEAK_Pen0095);
	DC_SetupGCForPen(dc);
	XDrawLine(display, dc->u.x.drawable, dc->u.x.gc, rect->left + 1,
		  rect->top + 1, rect->right - 2, rect->top + 1);
	XDrawLine(display, dc->u.x.drawable, dc->u.x.gc, rect->left + 1,
		  rect->top + 1, rect->left + 1, rect->bottom - 2);


	/* Now the bottom/right lines */
	SelectObject32(hdc, TWEAK_PenFF95);
	DC_SetupGCForPen(dc);
	XDrawLine(display, dc->u.x.drawable, dc->u.x.gc, rect->left,
		  rect->bottom - 1, rect->right - 1, rect->bottom - 1);
	XDrawLine(display, dc->u.x.drawable, dc->u.x.gc, rect->right - 1,
		  rect->top, rect->right - 1, rect->bottom - 1);

	SelectObject32(hdc, TWEAK_PenE095);
	DC_SetupGCForPen(dc);
	XDrawLine(display, dc->u.x.drawable, dc->u.x.gc, rect->left + 1,
		  rect->bottom - 2, rect->right - 2, rect->bottom - 2);
	XDrawLine(display, dc->u.x.drawable, dc->u.x.gc, rect->right - 2,
		  rect->top + 1, rect->right - 2, rect->bottom - 2);
	
	SelectObject32(hdc, prevpen);
    }

    return;
}



/******************************************************************************
 *
 *   void  TWEAK_DrawMenuSeparatorHoriz95(
 *      HDC32  hdc,               // Device context on which to draw
 *      UINT32  xc1,              // Left x-coordinate
 *      UINT32  yc,               // Y-coordinate of the LOWER line
 *      UINT32  xc2 )             // Right x-coordinate
 *
 *   Draws a horizontal menu separator bar Win 95 style.
 *
 *   Bugs
 *        Same as those for DrawReliefRect95.
 *
 *   Revision history
 *        08-Jul-1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *             Original implementation.
 *        11-Jul-1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *             Changed name from DrawMenuSeparator95
 *
 *****************************************************************************/

void  TWEAK_DrawMenuSeparatorHoriz95(
    HDC32  hdc,
    UINT32  xc1,
    UINT32  yc,
    UINT32  xc2 )
{
    DC  *dc;
    HPEN32  prevpen;

    if((dc = (DC *)GDI_GetObjPtr(hdc, DC_MAGIC))) {

	/* Draw the top line */
	prevpen = SelectObject32(hdc, TWEAK_Pen8095);
	DC_SetupGCForPen(dc);
	XDrawLine(display, dc->u.x.drawable, dc->u.x.gc, xc1, yc - 1, xc2,
		  yc - 1);

	/* And the bottom line */
	SelectObject32(hdc, TWEAK_PenFF95);
	DC_SetupGCForPen(dc);
	XDrawLine(display, dc->u.x.drawable, dc->u.x.gc, xc1, yc, xc2, yc);

	SelectObject32(hdc, prevpen);
    }

    return;
}


/******************************************************************************
 *
 *   void  TWEAK_DrawMenuSeparatorVert95(
 *      HDC32  hdc,               // Device context on which to draw
 *      UINT32  xc,               // X-coordinate of the RIGHT line
 *      UINT32  yc1,              // top Y-coordinate
 *      UINT32  yc2 )             // bottom Y-coordinate
 *
 *   Draws a vertical menu separator bar Win 95 style.
 *
 *   Bugs
 *        Same as those for DrawReliefRect95.
 *
 *   Revision history
 *        11-Jul-1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *             Original implementation.
 *
 *****************************************************************************/

void  TWEAK_DrawMenuSeparatorVert95(
    HDC32  hdc,
    UINT32  xc,
    UINT32  yc1,
    UINT32  yc2 )
{
    DC  *dc;
    HPEN32  prevpen;

    if((dc = (DC *)GDI_GetObjPtr(hdc, DC_MAGIC))) {

	/* Draw the top line */
	prevpen = SelectObject32(hdc, TWEAK_Pen8095);
	DC_SetupGCForPen(dc);
	XDrawLine(display, dc->u.x.drawable, dc->u.x.gc, xc, yc1, xc,
		  yc2);

	/* And the bottom line */
	SelectObject32(hdc, TWEAK_PenFF95);
	DC_SetupGCForPen(dc);
	XDrawLine(display, dc->u.x.drawable, dc->u.x.gc, xc + 1, yc1, xc + 1,
		  yc2);

	SelectObject32(hdc, prevpen);
    }

    return;
}
