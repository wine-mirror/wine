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
 *          Converted to WinCommand
 */

/*
 * WinCommand.c - WinCommand button widget
 */

#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xmu/Misc.h>
#include <X11/Xaw/XawInit.h>
#include "WinCommandP.h"
#include <X11/Xmu/Converters.h>

#define DEFAULT_HIGHLIGHT_THICKNESS 2
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

#define offset(field) XtOffsetOf(WinCommandRec, field)
static XtResource resources[] = { 
   {XtNcallback, XtCCallback, XtRCallback, sizeof(XtPointer), 
      offset(wincommand.callbacks), XtRCallback, (XtPointer)NULL},
   {XtNhighlightThickness, XtCThickness, XtRDimension, sizeof(Dimension),
      offset(wincommand.highlight_thickness), XtRImmediate,
      (XtPointer) DEFAULT_SHAPE_HIGHLIGHT},
   {XtNshapeStyle, XtCShapeStyle, XtRShapeStyle, sizeof(int),
      offset(wincommand.shape_style), XtRImmediate, 
	    (XtPointer)XawShapeRectangle},
   {XtNcornerRoundPercent, XtCCornerRoundPercent, 
	XtRDimension, sizeof(Dimension),
	offset(wincommand.corner_round), XtRImmediate, (XtPointer) 25},
};
#undef offset

static Boolean SetValues();
static void Initialize(), Redisplay(), Set(), Reset(), Notify(), Unset();
static void Highlight(), Unhighlight(), Destroy(), PaintWinCommandWidget();
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

WinCommandClassRec winCommandClassRec = {
  {
    (WidgetClass) SuperClass,		/* superclass		  */	
    "WinCommand",			/* class_name		  */
    sizeof(WinCommandRec),		/* size			  */
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
  },  /* WinCommandClass fields initialization */
};

  /* for public consumption */
WidgetClass winCommandWidgetClass = (WidgetClass) &winCommandClassRec;

/****************************************************************
 *
 * Private Procedures
 *
 ****************************************************************/

static GC 
Get_GC(cbw, fg, bg)
WinCommandWidget cbw;
Pixel fg, bg;
{
  XGCValues	values;
  
  values.foreground   = fg;
  values.background	= bg;
  values.font		= cbw->winlabel.font->fid;
  values.cap_style = CapProjecting;
  
  if (cbw->wincommand.highlight_thickness > 1 )
    values.line_width   = cbw->wincommand.highlight_thickness;
  else 
    values.line_width   = 0;
  
  return XtGetGC((Widget)cbw,
		 (GCForeground|GCBackground|GCFont|GCLineWidth|GCCapStyle),
		 &values);
}


/* ARGSUSED */
static void 
Initialize(request, new, args, num_args)
Widget request, new;
ArgList args;			/* unused */
Cardinal *num_args;		/* unused */
{
  WinCommandWidget cbw = (WinCommandWidget) new;
  int shape_event_base, shape_error_base;

  if (cbw->wincommand.shape_style != XawShapeRectangle
      && !XShapeQueryExtension(XtDisplay(new), &shape_event_base, 
			       &shape_error_base))
      cbw->wincommand.shape_style = XawShapeRectangle;
  if (cbw->wincommand.highlight_thickness == DEFAULT_SHAPE_HIGHLIGHT) {
      if (cbw->wincommand.shape_style != XawShapeRectangle)
	  cbw->wincommand.highlight_thickness = 0;
      else
	  cbw->wincommand.highlight_thickness = DEFAULT_HIGHLIGHT_THICKNESS;
  }

  cbw->wincommand.normal_GC = Get_GC(cbw, cbw->winlabel.foreground, 
				  cbw->core.background_pixel);
  cbw->wincommand.inverse_GC = Get_GC(cbw, cbw->core.background_pixel, 
				   cbw->winlabel.foreground);
  XtReleaseGC(new, cbw->winlabel.normal_GC);
  cbw->winlabel.normal_GC = cbw->wincommand.normal_GC;

  cbw->wincommand.set = FALSE;
  cbw->wincommand.highlighted = HighlightNone;
}

static Region 
HighlightRegion(cbw)
WinCommandWidget cbw;
{
  static Region outerRegion = NULL, innerRegion, emptyRegion;
  XRectangle rect;

  if (cbw->wincommand.highlight_thickness == 0 ||
      cbw->wincommand.highlight_thickness >
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
  rect.x = rect.y = cbw->wincommand.highlight_thickness;
  rect.width -= cbw->wincommand.highlight_thickness * 2;
  rect.height -= cbw->wincommand.highlight_thickness * 2;
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
  WinCommandWidget cbw = (WinCommandWidget)w;

  if (cbw->wincommand.set)
    return;

  cbw->wincommand.set= TRUE;
  if (XtIsRealized(w))
    PaintWinCommandWidget(w, (Region) NULL, TRUE);
}

/* ARGSUSED */
static void
Unset(w,event,params,num_params)
Widget w;
XEvent *event;
String *params;		/* unused */
Cardinal *num_params;
{
  WinCommandWidget cbw = (WinCommandWidget)w;

  if (!cbw->wincommand.set)
    return;

  cbw->wincommand.set = FALSE;
  if (XtIsRealized(w)) {
    XClearWindow(XtDisplay(w), XtWindow(w));
    PaintWinCommandWidget(w, (Region) NULL, TRUE);
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
  WinCommandWidget cbw = (WinCommandWidget)w;

  if (cbw->wincommand.set) {
    cbw->wincommand.highlighted = HighlightNone;
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
  WinCommandWidget cbw = (WinCommandWidget)w;

  if ( *num_params == (Cardinal) 0) 
    cbw->wincommand.highlighted = HighlightWhenUnset;
  else {
    if ( *num_params != (Cardinal) 1) 
      XtWarning("Too many parameters passed to highlight action table.");
    switch (params[0][0]) {
    case 'A':
    case 'a':
      cbw->wincommand.highlighted = HighlightAlways;
      break;
    default:
      cbw->wincommand.highlighted = HighlightWhenUnset;
      break;
    }
  }

  if (XtIsRealized(w))
    PaintWinCommandWidget(w, HighlightRegion(cbw), TRUE);
}

/* ARGSUSED */
static void 
Unhighlight(w,event,params,num_params)
Widget w;
XEvent *event;
String *params;		/* unused */
Cardinal *num_params;	/* unused */
{
  WinCommandWidget cbw = (WinCommandWidget)w;

  cbw->wincommand.highlighted = HighlightNone;
  if (XtIsRealized(w))
    PaintWinCommandWidget(w, HighlightRegion(cbw), TRUE);
}

/* ARGSUSED */
static void 
Notify(w,event,params,num_params)
Widget w;
XEvent *event;
String *params;		/* unused */
Cardinal *num_params;	/* unused */
{
  WinCommandWidget cbw = (WinCommandWidget)w; 

  /* check to be sure state is still Set so that user can cancel
     the action (e.g. by moving outside the window, in the default
     bindings.
  */
  if (cbw->wincommand.set)
    XtCallCallbackList(w, cbw->wincommand.callbacks, NULL);
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
  PaintWinCommandWidget(w, region, FALSE);
}

/*	Function Name: PaintWinCommandWidget
 *	Description: Paints the wincommand widget.
 *	Arguments: w - the wincommand widget.
 *                 region - region to paint (passed to the superclass).
 *                 change - did it change either set or highlight state?
 *	Returns: none
 */

static void 
PaintWinCommandWidget(w, region, change)
Widget w;
Region region;
Boolean change;
{
  WinCommandWidget cbw = (WinCommandWidget) w;
  Boolean very_thick;
  GC norm_gc, rev_gc;
   
  very_thick = cbw->wincommand.highlight_thickness >
               (Dimension)((Dimension) Min(cbw->core.width, 
					   cbw->core.height)/2);

  if (cbw->wincommand.set) {
    cbw->winlabel.normal_GC = cbw->wincommand.inverse_GC;
    XFillRectangle(XtDisplay(w), XtWindow(w), cbw->wincommand.normal_GC,
		   0, 0, cbw->core.width, cbw->core.height);
    region = NULL;		/* Force label to repaint text. */
  }
  else
      cbw->winlabel.normal_GC = cbw->wincommand.normal_GC;

  if (cbw->wincommand.highlight_thickness <= 0)
  {
    (*SuperClass->core_class.expose) (w, (XEvent *) NULL, region);
    return;
  }

/*
 * If we are set then use the same colors as if we are not highlighted. 
 */

  if (cbw->wincommand.set == (cbw->wincommand.highlighted == HighlightNone)) {
    norm_gc = cbw->wincommand.inverse_GC;
    rev_gc = cbw->wincommand.normal_GC;
  }
  else {
    norm_gc = cbw->wincommand.normal_GC;
    rev_gc = cbw->wincommand.inverse_GC;
  }

  if ( !( (!change && (cbw->wincommand.highlighted == HighlightNone)) ||
	  ((cbw->wincommand.highlighted == HighlightWhenUnset) &&
	   (cbw->wincommand.set))) ) {
    if (very_thick) {
      cbw->winlabel.normal_GC = norm_gc; /* Give the label the right GC. */
      XFillRectangle(XtDisplay(w),XtWindow(w), rev_gc,
		     0, 0, cbw->core.width, cbw->core.height);
    }
    else {
      /* wide lines are centered on the path, so indent it */
      int offset = cbw->wincommand.highlight_thickness/2;
      XDrawRectangle(XtDisplay(w),XtWindow(w), rev_gc, offset, offset, 
		     cbw->core.width - cbw->wincommand.highlight_thickness,
		     cbw->core.height - cbw->wincommand.highlight_thickness);
    }
  }
  (*SuperClass->core_class.expose) (w, (XEvent *) NULL, region);
}

static void 
Destroy(w)
Widget w;
{
  WinCommandWidget cbw = (WinCommandWidget) w;

  /* so WinLabel can release it */
  if (cbw->winlabel.normal_GC == cbw->wincommand.normal_GC)
    XtReleaseGC( w, cbw->wincommand.inverse_GC );
  else
    XtReleaseGC( w, cbw->wincommand.normal_GC );
}

/*
 * Set specified arguments into widget
 */

/* ARGSUSED */
static Boolean 
SetValues (current, request, new)
Widget current, request, new;
{
  WinCommandWidget oldcbw = (WinCommandWidget) current;
  WinCommandWidget cbw = (WinCommandWidget) new;
  Boolean redisplay = False;

  if ( oldcbw->core.sensitive != cbw->core.sensitive && !cbw->core.sensitive) {
    /* about to become insensitive */
    cbw->wincommand.set = FALSE;
    cbw->wincommand.highlighted = HighlightNone;
    redisplay = TRUE;
  }
  
  if ( (oldcbw->winlabel.foreground != cbw->winlabel.foreground)           ||
       (oldcbw->core.background_pixel != cbw->core.background_pixel) ||
       (oldcbw->wincommand.highlight_thickness != 
                                   cbw->wincommand.highlight_thickness) ||
       (oldcbw->winlabel.font != cbw->winlabel.font) ) 
  {
    if (oldcbw->winlabel.normal_GC == oldcbw->wincommand.normal_GC)
	/* WinLabel has release one of these */
      XtReleaseGC(new, cbw->wincommand.inverse_GC);
    else
      XtReleaseGC(new, cbw->wincommand.normal_GC);

    cbw->wincommand.normal_GC = Get_GC(cbw, cbw->winlabel.foreground, 
				    cbw->core.background_pixel);
    cbw->wincommand.inverse_GC = Get_GC(cbw, cbw->core.background_pixel, 
				     cbw->winlabel.foreground);
    XtReleaseGC(new, cbw->winlabel.normal_GC);
    cbw->winlabel.normal_GC = (cbw->wincommand.set
			    ? cbw->wincommand.inverse_GC
			    : cbw->wincommand.normal_GC);
    
    redisplay = True;
  }

  if ( XtIsRealized(new)
       && oldcbw->wincommand.shape_style != cbw->wincommand.shape_style
       && !ShapeButton(cbw, TRUE))
  {
      cbw->wincommand.shape_style = oldcbw->wincommand.shape_style;
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
WinCommandWidget cbw;
Boolean checkRectangular;
{
    Dimension corner_size;

    if ( (cbw->wincommand.shape_style == XawShapeRoundedRectangle) ) {
	corner_size = (cbw->core.width < cbw->core.height) ? cbw->core.width 
	                                                   : cbw->core.height;
	corner_size = (int) (corner_size * cbw->wincommand.corner_round) / 100;
    }

    if (checkRectangular || cbw->wincommand.shape_style != XawShapeRectangle) {
	if (!XmuReshapeWidget((Widget) cbw, cbw->wincommand.shape_style,
			      corner_size, corner_size)) {
	    cbw->wincommand.shape_style = XawShapeRectangle;
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
    (*winCommandWidgetClass->core_class.superclass->core_class.realize)
	(w, valueMask, attributes);

    ShapeButton( (WinCommandWidget) w, FALSE);
}

static void Resize(w)
    Widget w;
{
    if (XtIsRealized(w)) 
	ShapeButton( (WinCommandWidget) w, FALSE);

    (*winCommandWidgetClass->core_class.superclass->core_class.resize)(w);
}

