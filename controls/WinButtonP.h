/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

/*
 * Modifications for Wine
 *
 * 8/28/93  David Metcalfe (david@prism.demon.co.uk)
 *          Created from Command widget and added 3D effect
 */

/* 
 * WinButtonP.h - Private definitions for WinButton widget
 * 
 */

#ifndef _WinButtonP_h
#define _WinButtonP_h

#include "WinButton.h"
#include "WinLabelP.h"

/***********************************************************************
 *
 * WinButton Widget Private Data
 *
 ***********************************************************************/

typedef enum {
  HighlightNone,		/* Do not highlight. */
  HighlightWhenUnset,		/* Highlight only when unset, this is
				   to preserve current command widget 
				   functionality. */
  HighlightAlways		/* Always highlight, lets the toggle widget
				   and other subclasses do the right thing. */
} XtCommandHighlight;

/************************************
 *
 *  Class structure
 *
 ***********************************/


   /* New fields for the WinButton widget class record */
typedef struct _WinButtonClass 
  {
    int makes_compiler_happy;  /* not used */
  } WinButtonClassPart;

   /* Full class record declaration */
typedef struct _WinButtonClassRec {
    CoreClassPart	core_class;
    SimpleClassPart	simple_class;
    WinLabelClassPart	winlabel_class;
    WinButtonClassPart  winbutton_class;
} WinButtonClassRec;

extern WinButtonClassRec winButtonClassRec;

/***************************************
 *
 *  Instance (widget) structure 
 *
 **************************************/

    /* New fields for the WinButton widget record */
typedef struct {
    /* resources */
    Dimension   highlight_thickness;
    Dimension   shadow_thickness;
    Pixel       shadow_shade;
    Pixel       shadow_highlight;
    XtCallbackList callbacks;

    /* private state */
    Pixmap      	gray_pixmap;
    GC          	normal_GC;
    GC          	inverse_GC;
    GC                  shadow_highlight_gc;
    GC                  shadow_shade_gc;
    Boolean     	set;
    XtCommandHighlight	highlighted;
    /* more resources */
    int			shape_style;    
    Dimension		corner_round;
} WinButtonPart;


/*    XtEventsPtr eventTable;*/


   /* Full widget declaration */
typedef struct _WinButtonRec {
    CorePart         core;
    SimplePart	     simple;
    WinLabelPart     winlabel;
    WinButtonPart    winbutton;
} WinButtonRec;

#endif /* _WinButtonP_h */


