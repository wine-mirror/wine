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

#ifndef _WinButton_h
#define _WinButton_h

#include "WinLabel.h"

/* WinButton widget resources:

 Name		     Class		RepType		Default Value
 ----		     -----		-------		-------------
 accelerators	     Accelerators	AcceleratorTable NULL
 ancestorSensitive   AncestorSensitive	Boolean		True
 background	     Background		Pixel		XtDefaultBackground
 backgroundPixmap    Pixmap		Pixmap		XtUnspecifiedPixmap
 bitmap		     Pixmap		Pixmap		None
 borderColor	     BorderColor	Pixel		XtDefaultForeground
 borderPixmap	     Pixmap		Pixmap		XtUnspecifiedPixmap
 borderWidth	     BorderWidth	Dimension	1
 callback	     Callback		XtCallbackList	NULL
 colormap	     Colormap		Colormap	parent's colormap
 cornerRoundPercent  CornerRoundPercent	Dimension	25
 cursor		     Cursor		Cursor		None
 cursorName	     Cursor		String		NULL
 depth		     Depth		int		parent's depth
 destroyCallback     Callback		XtCallbackList	NULL
 encoding	     Encoding		UnsignedChar	XawTextEncoding8bit
 font		     Font		XFontStruct*	XtDefaultFont
 foreground	     Foreground		Pixel		XtDefaultForeground
 height		     Height		Dimension	text height
 highlightThickness  Thickness		Dimension	0 if shaped, else 2
 insensitiveBorder   Insensitive	Pixmap		Gray
 internalHeight	     Height		Dimension	2
 internalWidth	     Width		Dimension	4
 justify	     Justify		XtJustify	XtJustifyCenter
 label		     Label		String		NULL
 leftBitmap	     LeftBitmap		Pixmap		None
 mappedWhenManaged   MappedWhenManaged	Boolean		True
 pointerColor	     Foreground		Pixel		XtDefaultForeground
 pointerColorBackground Background	Pixel		XtDefaultBackground
 resize		     Resize		Boolean		True
 screen		     Screen		Screen		parent's Screen
 sensitive	     Sensitive		Boolean		True
 shadowHighlight     ShadowHighlight    Pixel           White
 shadowShade         ShadowShade        Pixel           Grey25
 shadowThickness     ShadowThickness    Dimension       2 
 shapeStyle	     ShapeStyle		ShapeStyle	Rectangle
 translations	     Translations	TranslationTable see doc or source
 width		     Width		Dimension	text width
 x		     Position		Position	0
 y		     Position		Position	0

*/

#define XtNhighlightThickness "highlightThickness"

#define XtNshapeStyle "shapeStyle"
#define XtCShapeStyle "ShapeStyle"
#define XtRShapeStyle "ShapeStyle"
#define XtNcornerRoundPercent "cornerRoundPercent"
#define XtCCornerRoundPercent "CornerRoundPercent"
#define XtNshadowThickness "shadowThickness"
#define XtCShadowThickness "ShadowThickness"
#define XtNshadowHighlight "shadowHighlight"
#define XtCShadowHighlight "ShadowHighlight"
#define XtNshadowShade "shadowShade"
#define XtCShadowShade "ShadowShade"

#define XawShapeRectangle XmuShapeRectangle
#define XawShapeOval XmuShapeOval
#define XawShapeEllipse XmuShapeEllipse
#define XawShapeRoundedRectangle XmuShapeRoundedRectangle

extern WidgetClass     winButtonWidgetClass;

typedef struct _WinButtonClassRec   *WinButtonWidgetClass;
typedef struct _WinButtonRec        *WinButtonWidget;

#endif /* _WinButton_h */
/* DON'T ADD STUFF AFTER THIS */
