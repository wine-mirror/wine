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
 * 8/27/93  David Metcalfe (david@prism.demon.co.uk)
 *          Convrted to WinCommand
 */

/* 
 * WinCommandP.h - Private definitions for WinCommand widget
 * 
 */

#ifndef _WinCommandP_h
#define _WinCommandP_h

#include "WinCommand.h"
#include "WinLabelP.h"

/***********************************************************************
 *
 * WinCommand Widget Private Data
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


   /* New fields for the WinCommand widget class record */
typedef struct _WinCommandClass 
  {
    int makes_compiler_happy;  /* not used */
  } WinCommandClassPart;

   /* Full class record declaration */
typedef struct _WinCommandClassRec {
    CoreClassPart	core_class;
    SimpleClassPart	simple_class;
    WinLabelClassPart	winlabel_class;
    WinCommandClassPart wincommand_class;
} WinCommandClassRec;

extern WinCommandClassRec winCommandClassRec;

/***************************************
 *
 *  Instance (widget) structure 
 *
 **************************************/

    /* New fields for the WinCommand widget record */
typedef struct {
    /* resources */
    Dimension   highlight_thickness;
    XtCallbackList callbacks;

    /* private state */
    Pixmap      	gray_pixmap;
    GC          	normal_GC;
    GC          	inverse_GC;
    Boolean     	set;
    XtCommandHighlight	highlighted;
    /* more resources */
    int			shape_style;    
    Dimension		corner_round;
} WinCommandPart;


/*    XtEventsPtr eventTable;*/


   /* Full widget declaration */
typedef struct _WinCommandRec {
    CorePart         core;
    SimplePart	     simple;
    WinLabelPart     winlabel;
    WinCommandPart   wincommand;
} WinCommandRec;

#endif /* _WinCommandP_h */


