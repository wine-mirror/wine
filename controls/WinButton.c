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
 * WinButton.c - WinButton button widget
 */

#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xmu/Misc.h>
#include <X11/Xaw/XawInit.h>
#include "WinButtonP.h"
#include <X11/Xmu/Converters.h>

#define DEFAULT_HIGHLIGHT_THICKNESS 0
#define DEFAULT_SHAPE_HIGHLIGHT 32767

/****************************************************************
 *
 * Full class record constant
 *
 ****************************************************************/

/* Private Data */

static char defaultTranslations[] =
    "<EnterWindow>:	highlight()		\n\
     <LeaveWindow>:	reset()			\n\
     <Btn1Down>:	set()			\n\
     <Btn1Up>:		notify() unset()	";

#define offset(field) XtOffsetOf(WinButtonRec, field)
static XtResource resources[] = { 
   {XtNcallback, XtCCallback, XtRCallback, sizeof(XtPointer), 
      offset(winbutton.callbacks), XtRCallback, (XtPointer)NULL},
   {XtNhighlightThickness, XtCThickness, XtRDimension, sizeof(Dimension),
      offset(winbutton.highlight_thickness), XtRImmediate,
      (XtPointer) DEFAULT_SHAPE_HIGHLIGHT},
   {XtNshapeStyle, XtCShapeStyle, XtRShapeStyle, sizeof(int),
      offset(winbutton.shape_style), XtRImmediate, 
	    (XtPointer)XawShapeRectangle},
   {XtNcornerRoundPercent, XtCCornerRoundPercent, 
	XtRDimension, sizeof(Dimension),
	offset(winbutton.corner_round), XtRImmediate, (XtPointer) 25},
   {XtNshadowThickness, XtCShadowThickness, XtRDimension, sizeof(Dimension),
	offset(winbutton.shadow_thickness), XtRImmediate, (XtPointer) 2},
   {XtNshadowHighlight, XtCShadowHighlight, XtRPixel, sizeof(Pixel),
        offset(winbutton.shadow_highlight), XtRString, (XtPointer) "white"},
   {XtNshadowShade, XtCShadowShade, XtRPixel, sizeof(Pixel),
	offset(winbutton.shadow_shade), XtRString, (XtPointer) "grey25"},
};
#undef offset

static Boolean SetValues();
static void Initialize(), Redisplay(), Set(), Reset(), Notify(), Unset();
static void Highlight(), Unhighlight(), Destroy(), PaintWinButtonWidget();
static void ClassInitialize();
static Boolean ShapeButton();
static void Realize(), Resize();

static XtActionsRec actionsList[] = {
  {"set",		Set},
  {"notify",		Notify},
  {"highlight",		Highlight},
  {"reset",		Reset},
  {"unset",		Unset},
  {"unhighlight",	Unhighlight}
};

#define SuperClass ((WinLabelWidgetClass)&winLabelClassRec)

WinButtonClassRec winButtonClassRec = {
  {
    (WidgetClass) SuperClass,		/* superclass		  */	
    "WinButton",			/* class_name		  */
    sizeof(WinButtonRec),		/* size			  */
    ClassInitialize,			/* class_initialize	  */
    NULL,				/* class_part_initialize  */
    FALSE,				/* class_inited		  */
    Initialize,				/* initialize		  */
    NULL,				/* initialize_hook	  */
    Realize,				/* realize		  */
    actionsList,			/* actions		  */
    XtNumber(actionsList),		/* num_actions		  */
    resources,				/* resources		  */
    XtNumber(resources),		/* resource_count	  */
    NULLQUARK,				/* xrm_class		  */
    FALSE,				/* compress_motion	  */
    TRUE,				/* compress_exposure	  */
    TRUE,				/* compress_enterleave    */
    FALSE,				/* visible_interest	  */
    Destroy,				/* destroy		  */
    Resize,				/* resize		  */
    Redisplay,				/* expose		  */
    SetValues,				/* set_values		  */
    NULL,				/* set_values_hook	  */
    XtInheritSetValuesAlmost,		/* set_values_almost	  */
    NULL,				/* get_values_hook	  */
    NULL,				/* accept_focus		  */
    XtVersion,				/* version		  */
    NULL,				/* callback_private	  */
    defaultTranslations,		/* tm_table		  */
    XtInheritQueryGeometry,		/* query_geometry	  */
    XtInheritDisplayAccelerator,	/* display_accelerator	  */
    NULL				/* extension		  */
  },  /* CoreClass fields initialization */
  {
    XtInheritChangeSensitive		/* change_sensitive	*/
  },  /* SimpleClass fields initialization */
  {
    0,                                     /* field not used    */
  },  /* WinLabelClass fields initialization */
  {
    0,                                     /* field not used    */
  },  /* WinButtonClass fields initialization */
};

  /* for public consumption */
WidgetClass winButtonWidgetClass = (WidgetClass) &winButtonClassRec;

/****************************************************************
 *
 * Private Procedures
 *
 ****************************************************************/

static GC 
Get_GC(cbw, fg, bg)
WinButtonWidget cbw;
Pixel fg, bg;
{
  XGCValues	values;
  
  values.foreground   = fg;
  values.background	= bg;
  values.font		= cbw->winlabel.font->fid;
  values.cap_style = CapProjecting;
  
  if (cbw->winbutton.highlight_thickness > 1 )
    values.line_width   = cbw->winbutton.highlight_thickness;
  else 
    values.line_width   = 0;
  
  return XtGetGC((Widget)cbw,
		 (GCForeground|GCBackground|GCFont|GCLineWidth|GCCapStyle),
		 &values);
}

static void 
Get_Shadow_GCs(cbw)
WinButtonWidget cbw;
{
  XGCValues	values;
  
  values.foreground     = cbw->winbutton.shadow_highlight;
  values.line_width	= cbw->winbutton.shadow_thickness;
  values.cap_style      = CapProjecting;
  cbw->winbutton.shadow_highlight_gc =
	  XtGetGC((Widget)cbw, (GCForeground|GCLineWidth|GCCapStyle), &values);

  values.foreground     = cbw->winbutton.shadow_shade;
  values.line_width	= cbw->winbutton.shadow_thickness;
  values.cap_style      = CapProjecting;
  cbw->winbutton.shadow_shade_gc =
	  XtGetGC((Widget)cbw, (GCForeground|GCLineWidth|GCCapStyle), &values);
}


/* ARGSUSED */
static void 
Initialize(request, new, args, num_args)
Widget request, new;
ArgList args;			/* unused */
Cardinal *num_args;		/* unused */
{
  WinButtonWidget cbw = (WinButtonWidget) new;
  int shape_event_base, shape_error_base;

  if (cbw->winbutton.shape_style != XawShapeRectangle
      && !XShapeQueryExtension(XtDisplay(new), &shape_event_base, 
			       &shape_error_base))
      cbw->winbutton.shape_style = XawShapeRectangle;
  if (cbw->winbutton.highlight_thickness == DEFAULT_SHAPE_HIGHLIGHT) {
      if (cbw->winbutton.shape_style != XawShapeRectangle)
	  cbw->winbutton.highlight_thickness = 0;
      else
	  cbw->winbutton.highlight_thickness = DEFAULT_HIGHLIGHT_THICKNESS;
  }

  XtVaSetValues(new, XtNbackground, "grey75");
  cbw->winbutton.normal_GC = Get_GC(cbw, cbw->winlabel.foreground, 
				  cbw->core.background_pixel);
  cbw->winbutton.inverse_GC = Get_GC(cbw, cbw->core.background_pixel, 
				   cbw->winlabel.foreground);
  XtReleaseGC(new, cbw->winlabel.normal_GC);
  cbw->winlabel.normal_GC = cbw->winbutton.normal_GC;

  Get_Shadow_GCs(cbw);

  cbw->winbutton.set = FALSE;
  cbw->winbutton.highlighted = HighlightNone;
}

static Region 
HighlightRegion(cbw)
WinButtonWidget cbw;
{
  static Region outerRegion = NULL, innerRegion, emptyRegion;
  XRectangle rect;

  if (cbw->winbutton.highlight_thickness == 0 ||
      cbw->winbutton.highlight_thickness >
      (Dimension) ((Dimension) Min(cbw->core.width, cbw->core.height)/2))
    return(NULL);

  if (outerRegion == NULL) {
    /* save time by allocating scratch regions only once. */
    outerRegion = XCreateRegion();
    innerRegion = XCreateRegion();
    emptyRegion = XCreateRegion();
  }

  rect.x = rect.y = 0;
  rect.width = cbw->core.width;
  rect.height = cbw->core.height;
  XUnionRectWithRegion( &rect, emptyRegion, outerRegion );
  rect.x = rect.y = cbw->winbutton.highlight_thickness;
  rect.width -= cbw->winbutton.highlight_thickness * 2;
  rect.height -= cbw->winbutton.highlight_thickness * 2;
  XUnionRectWithRegion( &rect, emptyRegion, innerRegion );
  XSubtractRegion( outerRegion, innerRegion, outerRegion );
  return outerRegion;
}

/***************************
*
*  Action Procedures
*
***************************/

/* ARGSUSED */
static void 
Set(w,event,params,num_params)
Widget w;
XEvent *event;
String *params;		/* unused */
Cardinal *num_params;	/* unused */
{
  WinButtonWidget cbw = (WinButtonWidget)w;

  if (cbw->winbutton.set)
    return;

  cbw->winbutton.set= TRUE;
  if (XtIsRealized(w))
    PaintWinButtonWidget(w, (Region) NULL, TRUE);
}

/* ARGSUSED */
static void
Unset(w,event,params,num_params)
Widget w;
XEvent *event;
String *params;		/* unused */
Cardinal *num_params;
{
  WinButtonWidget cbw = (WinButtonWidget)w;

  if (!cbw->winbutton.set)
    return;

  cbw->winbutton.set = FALSE;
  if (XtIsRealized(w)) {
    XClearWindow(XtDisplay(w), XtWindow(w));
    PaintWinButtonWidget(w, (Region) NULL, TRUE);
  }
}

/* ARGSUSED */
static void 
Reset(w,event,params,num_params)
Widget w;
XEvent *event;
String *params;		/* unused */
Cardinal *num_params;   /* unused */
{
  WinButtonWidget cbw = (WinButtonWidget)w;

  if (cbw->winbutton.set) {
    cbw->winbutton.highlighted = HighlightNone;
    Unset(w, event, params, num_params);
  }
  else
    Unhighlight(w, event, params, num_params);
}

/* ARGSUSED */
static void 
Highlight(w,event,params,num_params)
Widget w;
XEvent *event;
String *params;		
Cardinal *num_params;	
{
  WinButtonWidget cbw = (WinButtonWidget)w;

  if ( *num_params == (Cardinal) 0) 
    cbw->winbutton.highlighted = HighlightWhenUnset;
  else {
    if ( *num_params != (Cardinal) 1) 
      XtWarning("Too many parameters passed to highlight action table.");
    switch (params[0][0]) {
    case 'A':
    case 'a':
      cbw->winbutton.highlighted = HighlightAlways;
      break;
    default:
      cbw->winbutton.highlighted = HighlightWhenUnset;
      break;
    }
  }

  if (XtIsRealized(w))
    PaintWinButtonWidget(w, HighlightRegion(cbw), TRUE);
}

/* ARGSUSED */
static void 
Unhighlight(w,event,params,num_params)
Widget w;
XEvent *event;
String *params;		/* unused */
Cardinal *num_params;	/* unused */
{
  WinButtonWidget cbw = (WinButtonWidget)w;

  cbw->winbutton.highlighted = HighlightNone;
  if (XtIsRealized(w))
    PaintWinButtonWidget(w, HighlightRegion(cbw), TRUE);
}

/* ARGSUSED */
static void 
Notify(w,event,params,num_params)
Widget w;
XEvent *event;
String *params;		/* unused */
Cardinal *num_params;	/* unused */
{
  WinButtonWidget cbw = (WinButtonWidget)w; 

  /* check to be sure state is still Set so that user can cancel
     the action (e.g. by moving outside the window, in the default
     bindings.
  */
  if (cbw->winbutton.set)
    XtCallCallbackList(w, cbw->winbutton.callbacks, NULL);
}

/*
 * Repaint the widget window
 */

/************************
*
*  REDISPLAY (DRAW)
*
************************/

/* ARGSUSED */
static void 
Redisplay(w, event, region)
Widget w;
XEvent *event;
Region region;
{
  PaintWinButtonWidget(w, region, FALSE);
}

/*	Function Name: PaintWinButtonWidget
 *	Description: Paints the winbutton widget.
 *	Arguments: w - the winbutton widget.
 *                 region - region to paint (passed to the superclass).
 *                 change - did it change either set or highlight state?
 *	Returns: none
 */

static void 
PaintWinButtonWidget(w, region, change)
Widget w;
Region region;
Boolean change;
{
  WinButtonWidget cbw = (WinButtonWidget) w;
  Boolean very_thick;
  GC norm_gc, rev_gc;
  int offset;
   
  very_thick = cbw->winbutton.highlight_thickness >
               (Dimension)((Dimension) Min(cbw->core.width, 
					   cbw->core.height)/2);

  offset = cbw->winbutton.shadow_thickness / 2;
  if (cbw->winbutton.set) {
      XClearWindow(XtDisplay(w), XtWindow(w));
      region = NULL;		/* Force label to repaint text. */
  }
  else {
      XDrawLine(XtDisplay(w), XtWindow(w), cbw->winbutton.shadow_highlight_gc,
		offset, offset, cbw->core.width - offset, offset);
      XDrawLine(XtDisplay(w), XtWindow(w), cbw->winbutton.shadow_highlight_gc,
		offset, offset, offset, cbw->core.height - offset);
      XDrawLine(XtDisplay(w), XtWindow(w), cbw->winbutton.shadow_shade_gc,
		offset, cbw->core.height - offset + 1, 
		cbw->core.width - offset, cbw->core.height - offset);
      XDrawLine(XtDisplay(w), XtWindow(w), cbw->winbutton.shadow_shade_gc,
		cbw->core.width - offset, offset + 1, 
		cbw->core.width - offset, cbw->core.height - offset);
  }

  if (cbw->winbutton.highlight_thickness <= 0)
  {
    (*SuperClass->core_class.expose) (w, (XEvent *) NULL, region);
    return;
  }

/*
 * If we are set then use the same colors as if we are not highlighted. 
 */

  if (cbw->winbutton.set == (cbw->winbutton.highlighted == HighlightNone)) {
    norm_gc = cbw->winbutton.inverse_GC;
    rev_gc = cbw->winbutton.normal_GC;
  }
  else {
    norm_gc = cbw->winbutton.normal_GC;
    rev_gc = cbw->winbutton.inverse_GC;
  }

  if ( !( (!change && (cbw->winbutton.highlighted == HighlightNone)) ||
	  ((cbw->winbutton.highlighted == HighlightWhenUnset) &&
	   (cbw->winbutton.set))) ) {
    if (very_thick) {
      cbw->winlabel.normal_GC = norm_gc; /* Give the label the right GC. */
      XFillRectangle(XtDisplay(w),XtWindow(w), rev_gc,
		     0, 0, cbw->core.width, cbw->core.height);
    }
    else {
      /* wide lines are centered on the path, so indent it */
      int offset = cbw->winbutton.highlight_thickness/2;
      XDrawRectangle(XtDisplay(w),XtWindow(w), rev_gc, offset, offset, 
		     cbw->core.width - cbw->winbutton.highlight_thickness,
		     cbw->core.height - cbw->winbutton.highlight_thickness);
    }
  }
  (*SuperClass->core_class.expose) (w, (XEvent *) NULL, region);
}

static void 
Destroy(w)
Widget w;
{
  WinButtonWidget cbw = (WinButtonWidget) w;

  /* so WinLabel can release it */
  if (cbw->winlabel.normal_GC == cbw->winbutton.normal_GC)
    XtReleaseGC( w, cbw->winbutton.inverse_GC );
  else
    XtReleaseGC( w, cbw->winbutton.normal_GC );
}

/*
 * Set specified arguments into widget
 */

/* ARGSUSED */
static Boolean 
SetValues (current, request, new)
Widget current, request, new;
{
  WinButtonWidget oldcbw = (WinButtonWidget) current;
  WinButtonWidget cbw = (WinButtonWidget) new;
  Boolean redisplay = False;

  if ( oldcbw->core.sensitive != cbw->core.sensitive && !cbw->core.sensitive) {
    /* about to become insensitive */
    cbw->winbutton.set = FALSE;
    cbw->winbutton.highlighted = HighlightNone;
    redisplay = TRUE;
  }
  
  if ( (oldcbw->winlabel.foreground != cbw->winlabel.foreground)           ||
       (oldcbw->core.background_pixel != cbw->core.background_pixel) ||
       (oldcbw->winbutton.highlight_thickness != 
                                   cbw->winbutton.highlight_thickness) ||
       (oldcbw->winlabel.font != cbw->winlabel.font) ) 
  {
    if (oldcbw->winlabel.normal_GC == oldcbw->winbutton.normal_GC)
	/* WinLabel has release one of these */
      XtReleaseGC(new, cbw->winbutton.inverse_GC);
    else
      XtReleaseGC(new, cbw->winbutton.normal_GC);

    cbw->winbutton.normal_GC = Get_GC(cbw, cbw->winlabel.foreground, 
				    cbw->core.background_pixel);
    cbw->winbutton.inverse_GC = Get_GC(cbw, cbw->core.background_pixel, 
				     cbw->winlabel.foreground);
    XtReleaseGC(new, cbw->winlabel.normal_GC);
    cbw->winlabel.normal_GC = (cbw->winbutton.set
			    ? cbw->winbutton.inverse_GC
			    : cbw->winbutton.normal_GC);
    
    redisplay = True;
  }

  if ( XtIsRealized(new)
       && oldcbw->winbutton.shape_style != cbw->winbutton.shape_style
       && !ShapeButton(cbw, TRUE))
  {
      cbw->winbutton.shape_style = oldcbw->winbutton.shape_style;
  }

  return (redisplay);
}

static void ClassInitialize()
{
    XawInitializeWidgetSet();
    XtSetTypeConverter( XtRString, XtRShapeStyle, XmuCvtStringToShapeStyle,
		        NULL, 0, XtCacheNone, NULL );
}


static Boolean
ShapeButton(cbw, checkRectangular)
WinButtonWidget cbw;
Boolean checkRectangular;
{
    Dimension corner_size;

    if ( (cbw->winbutton.shape_style == XawShapeRoundedRectangle) ) {
	corner_size = (cbw->core.width < cbw->core.height) ? cbw->core.width 
	                                                   : cbw->core.height;
	corner_size = (int) (corner_size * cbw->winbutton.corner_round) / 100;
    }

    if (checkRectangular || cbw->winbutton.shape_style != XawShapeRectangle) {
	if (!XmuReshapeWidget((Widget) cbw, cbw->winbutton.shape_style,
			      corner_size, corner_size)) {
	    cbw->winbutton.shape_style = XawShapeRectangle;
	    return(False);
	}
    }
    return(TRUE);
}

static void Realize(w, valueMask, attributes)
    Widget w;
    Mask *valueMask;
    XSetWindowAttributes *attributes;
{
    (*winButtonWidgetClass->core_class.superclass->core_class.realize)
	(w, valueMask, attributes);

    ShapeButton( (WinButtonWidget) w, FALSE);
}

static void Resize(w)
    Widget w;
{
    if (XtIsRealized(w)) 
	ShapeButton( (WinButtonWidget) w, FALSE);

    (*winButtonWidgetClass->core_class.superclass->core_class.resize)(w);
}

