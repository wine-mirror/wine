/*
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  M.I.T. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * M.I.T. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL M.I.T.
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Modifications for Wine
 *
 * 8/27/93  David Metcalfe (david@prism.demon.co.uk)
 *          Converted to WinMenuButton
 */

/***********************************************************************
 *
 * WinMenuButton Widget
 *
 ***********************************************************************/

/*
 * WinMenuButtP.h - Private Header file for WinMenuButton widget.
 *
 * This is the private header file for the WinMenuButton widget.
 * It is intended to provide an easy method of activating pulldown menus.
 *
 * Date:    May 2, 1989
 *
 * By:      Chris D. Peterson
 *          MIT X Consortium 
 *          kit@expo.lcs.mit.edu
 */

#ifndef _WinMenuButtonP_h
#define _WinMenuButtonP_h

#include "WinMenuButto.h"
#include "WinCommandP.h"

/************************************
 *
 *  Class structure
 *
 ***********************************/


   /* New fields for the WinMenuButton widget class record */
typedef struct _WinMenuButtonClass 
{
  int makes_compiler_happy;  /* not used */
} WinMenuButtonClassPart;

   /* Full class record declaration */
typedef struct _WinMenuButtonClassRec {
	CoreClassPart	    core_class;
  SimpleClassPart	    simple_class;
  WinLabelClassPart	    winLabel_class;
  WinCommandClassPart	    winCommand_class;
  WinMenuButtonClassPart    winMenuButton_class;
} WinMenuButtonClassRec;

extern WinMenuButtonClassRec winMenuButtonClassRec;

/***************************************
 *
 *  Instance (widget) structure 
 *
 **************************************/

    /* New fields for the WinMenuButton widget record */
typedef struct {
  /* resources */
  String menu_name;

} WinMenuButtonPart;

   /* Full widget declaration */
typedef struct _WinMenuButtonRec {
    CorePart         core;
    SimplePart	     simple;
    WinLabelPart     winlabel;
    WinCommandPart   wincommand;
    WinMenuButtonPart winmenu_button;
} WinMenuButtonRec;

#endif /* _WinMenuButtonP_h */


