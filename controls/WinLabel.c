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
 * WinLabel.c - WinLabel widget
 *
 */

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xos.h>
#include <X11/Xaw/XawInit.h>
#include "WinLabelP.h"
#include <X11/Xmu/Converters.h>
#include <X11/Xmu/Drawing.h>
#include <stdio.h>
#include <ctype.h>

#define streq(a,b) (strcmp( (a), (b) ) == 0)

#define MULTI_LINE_LABEL 32767

#ifdef CRAY
#define WORD64
#endif

/****************************************************************
 *
 * Full class record constant
 *
 ****************************************************************/

/* Private Data */

#define offset(field) XtOffsetOf(WinLabelRec, field)
static XtResource resources[] = {
    {XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
        offset(winlabel.foreground), XtRString, XtDefaultForeground},
    {XtNfont,  XtCFont, XtRFontStruct, sizeof(XFontStruct *),
        offset(winlabel.font),XtRString, XtDefaultFont},
    {XtNlabel,  XtCLabel, XtRString, sizeof(String),
        offset(winlabel.label), XtRString, NULL},
    {XtNencoding, XtCEncoding, XtRUnsignedChar, sizeof(unsigned char),
        offset(winlabel.encoding), XtRImmediate, 
	                             (XtPointer)XawTextEncoding8bit},
    {XtNjustify, XtCJustify, XtRJustify, sizeof(XtJustify),
	offset(winlabel.justify), XtRImmediate, (XtPointer)XtJustifyCenter},
    {XtNinternalWidth, XtCWidth, XtRDimension,  sizeof(Dimension),
	offset(winlabel.internal_width), XtRImmediate, (XtPointer)4},
    {XtNinternalHeight, XtCHeight, XtRDimension, sizeof(Dimension),
	offset(winlabel.internal_height), XtRImmediate, (XtPointer)4},
    {XtNleftBitmap, XtCLeftBitmap, XtRBitmap, sizeof(Pixmap),
       offset(winlabel.left_bitmap), XtRImmediate, (XtPointer) None},
    {XtNbitmap, XtCPixmap, XtRBitmap, sizeof(Pixmap),
	offset(winlabel.pixmap), XtRImmediate, (XtPointer)None},
    {XtNresize, XtCResize, XtRBoolean, sizeof(Boolean),
	offset(winlabel.resize), XtRImmediate, (XtPointer)True},
};
#undef offset

static void Initialize();
static void Resize();
static void Redisplay();
static Boolean SetValues();
static void ClassInitialize();
static void Destroy();
static XtGeometryResult QueryGeometry();

WinLabelClassRec winLabelClassRec = {
  {
/* core_class fields */	
#define superclass		(&simpleClassRec)
    /* superclass	  	*/	(WidgetClass) superclass,
    /* class_name	  	*/	"WinLabel",
    /* widget_size	  	*/	sizeof(WinLabelRec),
    /* class_initialize   	*/	ClassInitialize,
    /* class_part_initialize	*/	NULL,
    /* class_inited       	*/	FALSE,
    /* initialize	  	*/	Initialize,
    /* initialize_hook		*/	NULL,
    /* realize		  	*/	XtInheritRealize,
    /* actions		  	*/	NULL,
    /* num_actions	  	*/	0,
    /* resources	  	*/	resources,
    /* num_resources	  	*/	XtNumber(resources),
    /* xrm_class	  	*/	NULLQUARK,
    /* compress_motion	  	*/	TRUE,
    /* compress_exposure  	*/	TRUE,
    /* compress_enterleave	*/	TRUE,
    /* visible_interest	  	*/	FALSE,
    /* destroy		  	*/	Destroy,
    /* resize		  	*/	Resize,
    /* expose		  	*/	Redisplay,
    /* set_values	  	*/	SetValues,
    /* set_values_hook		*/	NULL,
    /* set_values_almost	*/	XtInheritSetValuesAlmost,
    /* get_values_hook		*/	NULL,
    /* accept_focus	 	*/	NULL,
    /* version			*/	XtVersion,
    /* callback_private   	*/	NULL,
    /* tm_table		   	*/	NULL,
    /* query_geometry		*/	QueryGeometry,
    /* display_accelerator	*/	XtInheritDisplayAccelerator,
    /* extension		*/	NULL
  },
/* Simple class fields initialization */
  {
    /* change_sensitive		*/	XtInheritChangeSensitive
  }
};
WidgetClass winLabelWidgetClass = (WidgetClass)&winLabelClassRec;
/****************************************************************
 *
 * Private Procedures
 *
 ****************************************************************/

static void ClassInitialize()
{
    XawInitializeWidgetSet();
    XtAddConverter( XtRString, XtRJustify, XmuCvtStringToJustify, NULL, 0 );
}

#ifndef WORD64

#define TXT16 XChar2b

#else

#define TXT16 char

static XChar2b *buf2b;
static int buf2blen = 0;

_WinLabelWidth16(fs, str, n)
    XFontStruct *fs;
    char *str;
    int	n;
{
    int i;
    XChar2b *ptr;

    if (n > buf2blen) {
	buf2b = (XChar2b *)XtRealloc((char *)buf2b, n * sizeof(XChar2b));
	buf2blen = n;
    }
    for (ptr = buf2b, i = n; --i >= 0; ptr++) {
	ptr->byte1 = *str++;
	ptr->byte2 = *str++;
    }
    return XTextWidth16(fs, buf2b, n);
}

_WinLabelDraw16(dpy, d, gc, x, y, str, n)
    Display *dpy;
    Drawable d;
    GC gc;
    int x, y;
    char *str;
    int n;
{
    int i;
    XChar2b *ptr;

    if (n > buf2blen) {
	buf2b = (XChar2b *)XtRealloc((char *)buf2b, n * sizeof(XChar2b));
	buf2blen = n;
    }
    for (ptr = buf2b, i = n; --i >= 0; ptr++) {
	ptr->byte1 = *str++;
	ptr->byte2 = *str++;
    }
    XDrawString16(dpy, d, gc, x, y, buf2b, n);
}

#define XTextWidth16 _WinLabelWidth16
#define XDrawString16 _WinLabelDraw16

#endif /* WORD64 */

/*
 * Calculate width and height of displayed text in pixels
 */

static void SetTextWidthAndHeight(lw)
    WinLabelWidget lw;
{
    register XFontStruct	*fs = lw->winlabel.font;
    char *nl;

    if (lw->winlabel.pixmap != None) {
	Window root;
	int x, y;
	unsigned int width, height, bw, depth;
	if (XGetGeometry(XtDisplay(lw), lw->winlabel.pixmap, &root, &x, &y,
			 &width, &height, &bw, &depth)) {
	    lw->winlabel.label_height = height;
	    lw->winlabel.label_width = width;
	    lw->winlabel.label_len = depth;
	    return;
	}
    }

    lw->winlabel.label_height = fs->max_bounds.ascent + fs->max_bounds.descent;
    if (lw->winlabel.label == NULL) {
	lw->winlabel.label_len = 0;
	lw->winlabel.label_width = 0;
    }
    else if ((nl = index(lw->winlabel.label, '\n')) != NULL) {
	char *label;
	lw->winlabel.label_len = MULTI_LINE_LABEL;
	lw->winlabel.label_width = 0;
	for (label = lw->winlabel.label; nl != NULL; nl = index(label, '\n')) {
	    int width;

	    if (lw->winlabel.encoding)
		width = XTextWidth16(fs, (TXT16*)label, (int)(nl - label)/2);
	    else
		width = XTextWidth(fs, label, (int)(nl - label));
	    if (width > (int)lw->winlabel.label_width)
		lw->winlabel.label_width = width;
	    label = nl + 1;
	    if (*label)
		lw->winlabel.label_height +=
		    fs->max_bounds.ascent + fs->max_bounds.descent;
	}
	if (*label) {
	    int width = XTextWidth(fs, label, strlen(label));

	    if (lw->winlabel.encoding)
		width = XTextWidth16(fs, (TXT16*)label, (int)strlen(label)/2);
	    else
		width = XTextWidth(fs, label, strlen(label));
	    if (width > (int) lw->winlabel.label_width)
		lw->winlabel.label_width = width;
	}
    } else {
	lw->winlabel.label_len = strlen(lw->winlabel.label);
	if (lw->winlabel.encoding)
	    lw->winlabel.label_width =
		XTextWidth16(fs, (TXT16*)lw->winlabel.label,
			     (int) lw->winlabel.label_len/2);
	else
	    lw->winlabel.label_width =
		XTextWidth(fs, lw->winlabel.label, 
			   (int) lw->winlabel.label_len);
    }
}

static void GetnormalGC(lw)
    WinLabelWidget lw;
{
    XGCValues	values;

    values.foreground	= lw->winlabel.foreground;
    values.background	= lw->core.background_pixel;
    values.font		= lw->winlabel.font->fid;
    values.graphics_exposures = False;

    lw->winlabel.normal_GC = XtGetGC(
	(Widget)lw,
	(unsigned) GCForeground | GCBackground | GCFont | GCGraphicsExposures,
	&values);
}

static void GetgrayGC(lw)
    WinLabelWidget lw;
{
    XGCValues	values;

    values.foreground = lw->winlabel.foreground;
    values.background = lw->core.background_pixel;
    values.font	      = lw->winlabel.font->fid;
    values.fill_style = FillTiled;
    values.tile       = XmuCreateStippledPixmap(XtScreen((Widget)lw),
						lw->winlabel.foreground, 
						lw->core.background_pixel,
						lw->core.depth);
    values.graphics_exposures = False;

    lw->winlabel.stipple = values.tile;
    lw->winlabel.gray_GC = XtGetGC((Widget)lw, 
				(unsigned) GCForeground | GCBackground |
					   GCFont | GCTile | GCFillStyle |
					   GCGraphicsExposures,
				&values);
}

static void compute_bitmap_offsets (lw)
    WinLabelWidget lw;
{
    /*
     * label will be displayed at (internal_width, internal_height + lbm_y)
     */
    if (lw->winlabel.lbm_height != 0) {
	lw->winlabel.lbm_y = (((int) lw->core.height) -
			   ((int) lw->winlabel.internal_height * 2) -
			   ((int) lw->winlabel.lbm_height)) / 2;
    } else {
	lw->winlabel.lbm_y = 0;
    }
}


static void set_bitmap_info (lw)
    WinLabelWidget lw;
{
    Window root;
    int x, y;
    unsigned int bw, depth;

    if (!(lw->winlabel.left_bitmap &&
	  XGetGeometry (XtDisplay(lw), lw->winlabel.left_bitmap, &root, &x, &y,
			&lw->winlabel.lbm_width, &lw->winlabel.lbm_height,
			&bw, &depth))) {
	lw->winlabel.lbm_width = lw->winlabel.lbm_height = 0;
    }
    compute_bitmap_offsets (lw);
}

static void
RemoveAmpersand(w)
Widget w;
{
    WinLabelWidget lw = (WinLabelWidget) w;

    lw->winlabel.ul_pos = strcspn(lw->winlabel.label, "&");
    if (lw->winlabel.ul_pos == strlen(lw->winlabel.label))
    {
	lw->winlabel.ul_pos = -1;
	return;
    }

    /* Remove ampersand from label */
    strcpy(lw->winlabel.label + lw->winlabel.ul_pos,
	   lw->winlabel.label + lw->winlabel.ul_pos + 1);
}



/* ARGSUSED */
static void Initialize(request, new)
 Widget request, new;
{
    WinLabelWidget lw = (WinLabelWidget) new;

    if (lw->winlabel.label == NULL) 
        lw->winlabel.label = XtNewString(lw->core.name);
    else {
        lw->winlabel.label = XtNewString(lw->winlabel.label);
    }

    RemoveAmpersand(new);

    GetnormalGC(lw);
    GetgrayGC(lw);

    SetTextWidthAndHeight(lw);

    if (lw->core.height == 0)
        lw->core.height = lw->winlabel.label_height + 
		          2*lw->winlabel.internal_height;

    set_bitmap_info (lw);		/* need core.height */

    if (lw->core.width == 0)		/* need winlabel.lbm_width */
        lw->core.width = (lw->winlabel.label_width + 
			  2 * lw->winlabel.internal_width
			  + LEFT_OFFSET(lw));

    lw->winlabel.label_x = lw->winlabel.label_y = 0;
    (*XtClass(new)->core_class.resize) ((Widget)lw);

} /* Initialize */

/*
 * Repaint the widget window
 */

/* ARGSUSED */
static void Redisplay(w, event, region)
    Widget w;
    XEvent *event;
    Region region;
{
   WinLabelWidget lw = (WinLabelWidget) w;
   GC gc;
   int ul_x_loc, ul_y_loc, ul_width;

   if (region != NULL) {
       int x = lw->winlabel.label_x;
       unsigned int width = lw->winlabel.label_width;
       if (lw->winlabel.lbm_width) {
	   if (lw->winlabel.label_x > (x = lw->winlabel.internal_width))
	       width += lw->winlabel.label_x - x;
       }
       if (XRectInRegion(region, x, lw->winlabel.label_y,
			 width, lw->winlabel.label_height) == RectangleOut)
	   return;
   }

   gc = XtIsSensitive((Widget)lw) ? lw->winlabel.normal_GC 
	                          : lw->winlabel.gray_GC;
#ifdef notdef
   if (region != NULL) XSetRegion(XtDisplay(w), gc, region);
#endif /*notdef*/
   if (lw->winlabel.pixmap == None) {
       int len = lw->winlabel.label_len;
       char *label = lw->winlabel.label;
       Position y = lw->winlabel.label_y + 
	            lw->winlabel.font->max_bounds.ascent;

       /* display left bitmap */
       if (lw->winlabel.left_bitmap && lw->winlabel.lbm_width != 0) {
	   XCopyPlane (XtDisplay(w), lw->winlabel.left_bitmap, XtWindow(w), gc,
		       0, 0, lw->winlabel.lbm_width, lw->winlabel.lbm_height,
		       (int) lw->winlabel.internal_width, 
		       (int) lw->winlabel.internal_height + 
		       lw->winlabel.lbm_y, 
		       (unsigned long) 1L);
       }

       if (len == MULTI_LINE_LABEL) {
	   char *nl;
	   while ((nl = index(label, '\n')) != NULL) {
	       if (lw->winlabel.encoding)
		   XDrawString16(XtDisplay(w), XtWindow(w), gc,
				 lw->winlabel.label_x, y,
				 (TXT16*)label, (int)(nl - label)/2);
	       else
		   XDrawString(XtDisplay(w), XtWindow(w), gc,
			       lw->winlabel.label_x, y, label, 
			       (int)(nl - label));
	       y += lw->winlabel.font->max_bounds.ascent + 
		               lw->winlabel.font->max_bounds.descent;
	       label = nl + 1;
	   }
	   len = strlen(label);
       }
       if (len) {
	   if (lw->winlabel.encoding)
	       XDrawString16(XtDisplay(w), XtWindow(w), gc,
			     lw->winlabel.label_x, y, (TXT16*)label, len/2);
	   else
	       XDrawString(XtDisplay(w), XtWindow(w), gc,
			   lw->winlabel.label_x, y, label, len);

	   if (lw->winlabel.ul_pos != -1)
	   {
	       /* Don't bother with two byte chars at present */
	       if (!lw->winlabel.encoding)
	       {
	           ul_x_loc = lw->winlabel.label_x + 
			      XTextWidth(lw->winlabel.font,
		              lw->winlabel.label, lw->winlabel.ul_pos);
	           ul_y_loc = lw->winlabel.label_height + 
		              lw->winlabel.internal_height + 1;
	           ul_width = XTextWidth(lw->winlabel.font,
		              lw->winlabel.label + lw->winlabel.ul_pos, 1);

	           XDrawLine(XtDisplayOfObject(w), XtWindowOfObject(w), gc,
		        ul_x_loc, ul_y_loc, ul_x_loc + ul_width - 1, ul_y_loc);
	       }
           }
       }
   } else if (lw->winlabel.label_len == 1) { /* depth */
       XCopyPlane(XtDisplay(w), lw->winlabel.pixmap, XtWindow(w), gc,
		  0, 0, lw->winlabel.label_width, lw->winlabel.label_height,
		  lw->winlabel.label_x, lw->winlabel.label_y, 1L);
   } else {
       XCopyArea(XtDisplay(w), lw->winlabel.pixmap, XtWindow(w), gc,
		 0, 0, lw->winlabel.label_width, lw->winlabel.label_height,
		 lw->winlabel.label_x, lw->winlabel.label_y);
   }
#ifdef notdef
   if (region != NULL) XSetClipMask(XtDisplay(w), gc, (Pixmap)None);
#endif /* notdef */
}

static void _Reposition(lw, width, height, dx, dy)
    register WinLabelWidget lw;
    Dimension width, height;
    Position *dx, *dy;
{
    Position newPos;
    Position leftedge = lw->winlabel.internal_width + LEFT_OFFSET(lw);

    switch (lw->winlabel.justify) {

	case XtJustifyLeft   :
	    newPos = leftedge;
	    break;

	case XtJustifyRight  :
	    newPos = width -
		(lw->winlabel.label_width + lw->winlabel.internal_width);
	    break;

	case XtJustifyCenter :
	default:
	    newPos = (int)(width - lw->winlabel.label_width) / 2;
	    break;
    }
    if (newPos < (Position)leftedge)
	newPos = leftedge;
    *dx = newPos - lw->winlabel.label_x;
    lw->winlabel.label_x = newPos;
    *dy = (newPos = (int)(height - lw->winlabel.label_height) / 2)
	  - lw->winlabel.label_y;
    lw->winlabel.label_y = newPos;
    return;
}

static void Resize(w)
    Widget w;
{
    WinLabelWidget lw = (WinLabelWidget)w;
    Position dx, dy;
    _Reposition(lw, w->core.width, w->core.height, &dx, &dy);
    compute_bitmap_offsets (lw);
}

/*
 * Set specified arguments into widget
 */

#define PIXMAP 0
#define WIDTH 1
#define HEIGHT 2
#define NUM_CHECKS 3

static Boolean SetValues(current, request, new, args, num_args)
    Widget current, request, new;
    ArgList args;
    Cardinal *num_args;
{
    WinLabelWidget curlw = (WinLabelWidget) current;
    WinLabelWidget reqlw = (WinLabelWidget) request;
    WinLabelWidget newlw = (WinLabelWidget) new;
    int i;
    Boolean was_resized = False, redisplay = False, checks[NUM_CHECKS];

    for (i = 0; i < NUM_CHECKS; i++)
	checks[i] = FALSE;

    for (i = 0; i < *num_args; i++) {
	if (streq(XtNbitmap, args[i].name))
	    checks[PIXMAP] = TRUE;
	if (streq(XtNwidth, args[i].name))
	    checks[WIDTH] = TRUE;
	if (streq(XtNheight, args[i].name))
	    checks[HEIGHT] = TRUE;
    }

    if (newlw->winlabel.label == NULL) {
	newlw->winlabel.label = newlw->core.name;
    }

    /*
     * resize on bitmap change
     */
    if (curlw->winlabel.left_bitmap != newlw->winlabel.left_bitmap) {
	was_resized = True;
    }

    if (curlw->winlabel.encoding != newlw->winlabel.encoding)
	was_resized = True;

    if (curlw->winlabel.label != newlw->winlabel.label) {
        if (curlw->winlabel.label != curlw->core.name)
	    XtFree( (char *)curlw->winlabel.label );

	if (newlw->winlabel.label != newlw->core.name) {
	    newlw->winlabel.label = XtNewString( newlw->winlabel.label );
	}
	RemoveAmpersand(new);
	was_resized = True;
    }

    if (was_resized || (curlw->winlabel.font != newlw->winlabel.font) ||
	(curlw->winlabel.justify != newlw->winlabel.justify) 
	|| checks[PIXMAP]) {

	SetTextWidthAndHeight(newlw);
	was_resized = True;
    }

    /* recalculate the window size if something has changed. */
    if (newlw->winlabel.resize && was_resized) {
	if ((curlw->core.height == reqlw->core.height) && !checks[HEIGHT])
	    newlw->core.height = (newlw->winlabel.label_height + 
				  2 * newlw->winlabel.internal_height);

	set_bitmap_info (newlw);

	if ((curlw->core.width == reqlw->core.width) && !checks[WIDTH])
	    newlw->core.width = (newlw->winlabel.label_width +
				 LEFT_OFFSET(newlw) +
				 2 * newlw->winlabel.internal_width);
    }

    if (curlw->winlabel.foreground != newlw->winlabel.foreground
	|| curlw->core.background_pixel != newlw->core.background_pixel
	|| curlw->winlabel.font->fid != newlw->winlabel.font->fid) {

	XtReleaseGC(new, curlw->winlabel.normal_GC);
	XtReleaseGC(new, curlw->winlabel.gray_GC);
	XmuReleaseStippledPixmap( XtScreen(current), curlw->winlabel.stipple );
	GetnormalGC(newlw);
	GetgrayGC(newlw);
	redisplay = True;
    }

    if ((curlw->winlabel.internal_width != newlw->winlabel.internal_width)
        || (curlw->winlabel.internal_height != newlw->winlabel.internal_height)
	|| was_resized) {
	/* Resize() will be called if geometry changes succeed */
	Position dx, dy;
	_Reposition(newlw, curlw->core.width, curlw->core.height, &dx, &dy);
    }

    return was_resized || redisplay ||
	   XtIsSensitive(current) != XtIsSensitive(new);
}

static void Destroy(w)
    Widget w;
{
    WinLabelWidget lw = (WinLabelWidget)w;

    XtFree( lw->winlabel.label );
    XtReleaseGC( w, lw->winlabel.normal_GC );
    XtReleaseGC( w, lw->winlabel.gray_GC);
    XmuReleaseStippledPixmap( XtScreen(w), lw->winlabel.stipple );
}


static XtGeometryResult QueryGeometry(w, intended, preferred)
    Widget w;
    XtWidgetGeometry *intended, *preferred;
{
    register WinLabelWidget lw = (WinLabelWidget)w;

    preferred->request_mode = CWWidth | CWHeight;
    preferred->width = (lw->winlabel.label_width + 
			2 * lw->winlabel.internal_width +
			LEFT_OFFSET(lw));
    preferred->height = lw->winlabel.label_height + 
	                2*lw->winlabel.internal_height;
    if (  ((intended->request_mode & (CWWidth | CWHeight))
	   	== (CWWidth | CWHeight)) &&
	  intended->width == preferred->width &&
	  intended->height == preferred->height)
	return XtGeometryYes;
    else if (preferred->width == w->core.width &&
	     preferred->height == w->core.height)
	return XtGeometryNo;
    else
	return XtGeometryAlmost;
}
