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
 * 8/23/93  David Metcalfe (david@prism.demon.co.uk)
 *          Added code to translate ampersand to underlined char
 *
 * 8/27/93  David Metcalfe (david@prism.demon.co.uk)
 *          Converted to WinLabel
 */

/* 
 * WinLabelP.h - Private definitions for WinLabel widget
 * 
 */

#ifndef _WinLabelP_h
#define _WinLabelP_h

/***********************************************************************
 *
 * WinLabel Widget Private Data
 *
 ***********************************************************************/

#include "WinLabel.h"
#include <X11/Xaw/SimpleP.h>

/* New fields for the WinLabel widget class record */

typedef struct {int foo;} WinLabelClassPart;

/* Full class record declaration */
typedef struct _WinLabelClassRec {
    CoreClassPart	core_class;
    SimpleClassPart	simple_class;
    WinLabelClassPart	winlabel_class;
} WinLabelClassRec;

extern WinLabelClassRec winLabelClassRec;

/* New fields for the WinLabel widget record */
typedef struct {
    /* resources */
    Pixel	foreground;
    XFontStruct	*font;
    char	*label;
    XtJustify	justify;
    Dimension	internal_width;
    Dimension	internal_height;
    Pixmap	pixmap;
    Boolean	resize;
    unsigned char encoding;
    Pixmap	left_bitmap;

    /* private state */
    GC		normal_GC;
    GC          gray_GC;
    Pixmap	stipple;
    Position	label_x;
    Position	label_y;
    Dimension	label_width;
    Dimension	label_height;
    Dimension	label_len;
    int		lbm_y;			/* where in label */
    unsigned int lbm_width, lbm_height;	 /* size of pixmap */

    int ul_pos;                 /* Offset in chars of underlined character */
                                /* in label */
} WinLabelPart;


/****************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************/

typedef struct _WinLabelRec {
    CorePart	core;
    SimplePart	simple;
    WinLabelPart winlabel;
} WinLabelRec;

#define LEFT_OFFSET(lw) ((lw)->winlabel.left_bitmap \
			 ? (lw)->winlabel.lbm_width + \
			 (lw)->winlabel.internal_width \
			 : 0)

#endif /* _WinLabelP_h */

