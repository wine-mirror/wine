/* $XConsortium: SmeMenuButton.c,v 1.16 91/03/15 15:59:41 gildea Exp $ */

/*
 * Copyright 1989 Massachusetts Institute of Technology
 *
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
 * SmeMenuButton.c - Source code file for BSB Menu Entry object.
 *
 * Date:    September 26, 1989
 *
 * By:      Chris D. Peterson
 *          MIT X Consortium 
 *          kit@expo.lcs.mit.edu
 *
 * Modifications for Wine
 *
 * 8/23/93  David Metcalfe (david@prism.demon.co.uk)
 *          Added code to translate ampersand to underlined char
 */

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xos.h>

#include <X11/Xmu/Drawing.h>

#include <X11/Xaw/XawInit.h>
#include <X11/Xaw/SimpleMenu.h>
#include "SmeMenuButtP.h"
#include <X11/Xaw/Cardinals.h>

#include <stdio.h>

#define ONE_HUNDRED 100

#define offset(field) XtOffsetOf(SmeMenuButtonRec, sme_bsb.field)

static XtResource resources[] = {
  {XtNlabel,  XtCLabel, XtRString, sizeof(String),
     offset(label), XtRString, NULL},
  {XtNvertSpace,  XtCVertSpace, XtRInt, sizeof(int),
     offset(vert_space), XtRImmediate, (XtPointer) 25},
  {XtNleftBitmap, XtCLeftBitmap, XtRBitmap, sizeof(Pixmap),
     offset(left_bitmap), XtRImmediate, (XtPointer)None},
  {XtNjustify, XtCJustify, XtRJustify, sizeof(XtJustify),
     offset(justify), XtRImmediate, (XtPointer) XtJustifyLeft},
  {XtNrightBitmap, XtCRightBitmap, XtRBitmap, sizeof(Pixmap),
     offset(right_bitmap), XtRImmediate, (XtPointer)None},
  {XtNleftMargin,  XtCHorizontalMargins, XtRDimension, sizeof(Dimension),
     offset(left_margin), XtRImmediate, (XtPointer) 15},
  {XtNrightMargin,  XtCHorizontalMargins, XtRDimension, sizeof(Dimension),
     offset(right_margin), XtRImmediate, (XtPointer) 5},
  {XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
     offset(foreground), XtRString, XtDefaultForeground},
  {XtNfont,  XtCFont, XtRFontStruct, sizeof(XFontStruct *),
     offset(font), XtRString, XtDefaultFont},
  {XtNmenuName, XtCMenuName, XtRString, sizeof(String), 
    offset(menu_name), XtRString, (XtPointer) NULL},
  {XtNinactive, XtCInactive, XtRBoolean, sizeof(Boolean),
    offset(inactive), XtRImmediate, (XtPointer) FALSE },
};   
#undef offset

/*
 * Semi Public function definitions. 
 */

static void Redisplay(), Destroy(), Initialize(), FlipColors();
static void ClassInitialize();
static void PopupMenu(), Unhighlight();
static Boolean SetValues();
static XtGeometryResult QueryGeometry();

/* 
 * Private Function Definitions.
 */

static void GetDefaultSize(), DrawBitmaps(), GetBitmapInfo();
static void CreateGCs(), DestroyGCs();
static void RemoveAmpersand();
    
#define superclass (&smeClassRec)
SmeMenuButtonClassRec smeMenuButtonClassRec = {
  {
    /* superclass         */    (WidgetClass) superclass,
    /* class_name         */    "SmeMenuButton",
    /* size               */    sizeof(SmeMenuButtonRec),
    /* class_initializer  */	ClassInitialize,
    /* class_part_initialize*/	NULL,
    /* Class init'ed      */	FALSE,
    /* initialize         */    Initialize,
    /* initialize_hook    */	NULL,
    /* realize            */    NULL,
    /* actions            */    NULL,
    /* num_actions        */    ZERO,
    /* resources          */    resources,
    /* resource_count     */	XtNumber(resources),
    /* xrm_class          */    NULLQUARK,
    /* compress_motion    */    FALSE, 
    /* compress_exposure  */    FALSE,
    /* compress_enterleave*/ 	FALSE,
    /* visible_interest   */    FALSE,
    /* destroy            */    Destroy,
    /* resize             */    NULL,
    /* expose             */    Redisplay,
    /* set_values         */    SetValues,
    /* set_values_hook    */	NULL,
    /* set_values_almost  */	XtInheritSetValuesAlmost,  
    /* get_values_hook    */	NULL,			
    /* accept_focus       */    NULL,
    /* intrinsics version */	XtVersion,
    /* callback offsets   */    NULL,
    /* tm_table		  */    NULL,
    /* query_geometry	  */    QueryGeometry,
    /* display_accelerator*/    NULL,
    /* extension	  */    NULL
  },{
    /* Menu Entry Fields */
      
    /* highlight */             FlipColors,
    /* unhighlight */           Unhighlight,
    /* notify */		XtInheritNotify,		
    /* extension	  */    NULL
  }, {
    /* BSB Menu entry Fields */  

    /* extension	  */    NULL
  }
};

WidgetClass smeMenuButtonObjectClass = (WidgetClass) &smeMenuButtonClassRec;

/************************************************************
 *
 * Semi-Public Functions.
 *
 ************************************************************/

/*	Function Name: ClassInitialize
 *	Description: Initializes the SmeMenuButtonObject. 
 *	Arguments: none.
 *	Returns: none.
 */

static void 
ClassInitialize()
{
    XawInitializeWidgetSet();
    XtAddConverter( XtRString, XtRJustify, XmuCvtStringToJustify, NULL, 0 );
}

/*      Function Name: Initialize
 *      Description: Initializes the simple menu widget
 *      Arguments: request - the widget requested by the argument list.
 *                 new     - the new widget with both resource and non
 *                           resource values.
 *      Returns: none.
 */

/* ARGSUSED */
static void
Initialize(request, new)
Widget request, new;
{
    SmeMenuButtonObject entry = (SmeMenuButtonObject) new;

    if (entry->sme_bsb.label == NULL) 
	entry->sme_bsb.label = XtName(new);
    else
	entry->sme_bsb.label = XtNewString( entry->sme_bsb.label );

    RemoveAmpersand(new);

    GetDefaultSize(new, &(entry->rectangle.width), &(entry->rectangle.height));
    CreateGCs(new);

    entry->sme_bsb.left_bitmap_width = entry->sme_bsb.left_bitmap_height = 0;
    entry->sme_bsb.right_bitmap_width = entry->sme_bsb.right_bitmap_height = 0;

    GetBitmapInfo(new, TRUE);	/* Left Bitmap Info */
    GetBitmapInfo(new, FALSE);	/* Right Bitmap Info */
}

/*      Function Name: Destroy
 *      Description: Called at destroy time, cleans up.
 *      Arguments: w - the simple menu widget.
 *      Returns: none.
 */

static void
Destroy(w)
Widget w;
{
    SmeMenuButtonObject entry = (SmeMenuButtonObject) w;

    DestroyGCs(w);
    if (entry->sme_bsb.label != XtName(w))
	XtFree(entry->sme_bsb.label);
}

/*      Function Name: Redisplay
 *      Description: Redisplays the contents of the widget.
 *      Arguments: w - the simple menu widget.
 *                 event - the X event that caused this redisplay.
 *                 region - the region the needs to be repainted. 
 *      Returns: none.
 */

/* ARGSUSED */
static void
Redisplay(w, event, region)
Widget w;
XEvent * event;
Region region;
{
    GC gc;
    SmeMenuButtonObject entry = (SmeMenuButtonObject) w;
    int	font_ascent, font_descent, y_loc;
    int ul_x_loc, ul_y_loc, ul_width;

    entry->sme_bsb.set_values_area_cleared = FALSE;    
    font_ascent = entry->sme_bsb.font->max_bounds.ascent;
    font_descent = entry->sme_bsb.font->max_bounds.descent;

    y_loc = entry->rectangle.y;
    
    if (XtIsSensitive(w) && XtIsSensitive( XtParent(w) ) ) {
	if ( w == XawSimpleMenuGetActiveEntry(XtParent(w)) ) {
	    XFillRectangle(XtDisplayOfObject(w), XtWindowOfObject(w), 
			   entry->sme_bsb.norm_gc, 0, y_loc,
			   (unsigned int) entry->rectangle.width,
			   (unsigned int) entry->rectangle.height);
	    gc = entry->sme_bsb.rev_gc;
	}
	else
	    gc = entry->sme_bsb.norm_gc;
    }
    else
	gc = entry->sme_bsb.norm_gray_gc;
    
    if (entry->sme_bsb.label != NULL) {
	int x_loc = entry->sme_bsb.left_margin;
	int len = strlen(entry->sme_bsb.label);
	char * label = entry->sme_bsb.label;

	switch(entry->sme_bsb.justify) {
	    int width, t_width;

	case XtJustifyCenter:
	    t_width = XTextWidth(entry->sme_bsb.font, label, len);
	    width = entry->rectangle.width - (entry->sme_bsb.left_margin +
					      entry->sme_bsb.right_margin);
	    x_loc += (width - t_width)/2;
	    break;
	case XtJustifyRight:
	    t_width = XTextWidth(entry->sme_bsb.font, label, len);
	    x_loc = entry->rectangle.width - (entry->sme_bsb.right_margin +
					      t_width);
	    break;
	case XtJustifyLeft:
	default:
	    break;
	}

	y_loc += ((int)entry->rectangle.height - 
		  (font_ascent + font_descent)) / 2 + font_ascent;
	
	XDrawString(XtDisplayOfObject(w), XtWindowOfObject(w), gc,
		    x_loc, y_loc, label, len);

	if (entry->sme_bsb.ul_pos != -1)
	{
	    ul_x_loc = x_loc + XTextWidth(entry->sme_bsb.font,
		       entry->sme_bsb.label, entry->sme_bsb.ul_pos);
	    ul_y_loc = entry->rectangle.y + (entry->rectangle.height + 
		       font_ascent + font_descent) / 2;
	    ul_width = XTextWidth(entry->sme_bsb.font,
		       entry->sme_bsb.label + entry->sme_bsb.ul_pos, 1);

	    XDrawLine(XtDisplayOfObject(w), XtWindowOfObject(w), gc,
		      ul_x_loc, ul_y_loc, ul_x_loc + ul_width - 1, ul_y_loc);
	}
    }

    DrawBitmaps(w, gc);
}


/*      Function Name: SetValues
 *      Description: Relayout the menu when one of the resources is changed.
 *      Arguments: current - current state of the widget.
 *                 request - what was requested.
 *                 new - what the widget will become.
 *      Returns: none
 */

/* ARGSUSED */
static Boolean
SetValues(current, request, new)
Widget current, request, new;
{
    SmeMenuButtonObject entry = (SmeMenuButtonObject) new;
    SmeMenuButtonObject old_entry = (SmeMenuButtonObject) current;
    Boolean ret_val = FALSE;

    if (old_entry->sme_bsb.label != entry->sme_bsb.label) {
        if (old_entry->sme_bsb.label != XtName( new ) )
	    XtFree( (char *) old_entry->sme_bsb.label );

	if (entry->sme_bsb.label != XtName(new) ) 
	    entry->sme_bsb.label = XtNewString( entry->sme_bsb.label );

	RemoveAmpersand(new);
	ret_val = True;
    }

    if (entry->rectangle.sensitive != old_entry->rectangle.sensitive )
	ret_val = TRUE;

    if (entry->sme_bsb.left_bitmap != old_entry->sme_bsb.left_bitmap) {
	GetBitmapInfo(new, TRUE);
	ret_val = TRUE;
    }

    if (entry->sme_bsb.right_bitmap != old_entry->sme_bsb.right_bitmap) {
	GetBitmapInfo(new, FALSE);
	ret_val = TRUE;
    }

    if ( (old_entry->sme_bsb.font != entry->sme_bsb.font) ||
	 (old_entry->sme_bsb.foreground != entry->sme_bsb.foreground) ) {
	DestroyGCs(current);
	CreateGCs(new);
	ret_val = TRUE;
    }

    if (ret_val) {
	GetDefaultSize(new, 
		       &(entry->rectangle.width), &(entry->rectangle.height));
	entry->sme_bsb.set_values_area_cleared = TRUE;
    }
    return(ret_val);
}

/*	Function Name: QueryGeometry.
 *	Description: Returns the preferred geometry for this widget.
 *	Arguments: w - the menu entry object.
 *                 itended, return_val - the intended and return geometry info.
 *	Returns: A Geometry Result.
 *
 * See the Intrinsics manual for details on what this function is for.
 * 
 * I just return the height and width of the label plus the margins.
 */

static XtGeometryResult
QueryGeometry(w, intended, return_val) 
Widget w;
XtWidgetGeometry *intended, *return_val;
{
    SmeMenuButtonObject entry = (SmeMenuButtonObject) w;
    Dimension width, height;
    XtGeometryResult ret_val = XtGeometryYes;
    XtGeometryMask mode = intended->request_mode;

    GetDefaultSize(w, &width, &height );    

    if ( ((mode & CWWidth) && (intended->width != width)) ||
	 !(mode & CWWidth) ) {
	return_val->request_mode |= CWWidth;
	return_val->width = width;
	ret_val = XtGeometryAlmost;
    }

    if ( ((mode & CWHeight) && (intended->height != height)) ||
	 !(mode & CWHeight) ) {
	return_val->request_mode |= CWHeight;
	return_val->height = height;
	ret_val = XtGeometryAlmost;
    }

    if (ret_val == XtGeometryAlmost) {
	mode = return_val->request_mode;
	
	if ( ((mode & CWWidth) && (width == entry->rectangle.width)) &&
	     ((mode & CWHeight) && (height == entry->rectangle.height)) )
	    return(XtGeometryNo);
    }

    return(ret_val);
}
    
/*      Function Name: FlipColors
 *      Description: Invert the colors of the current entry.
 *      Arguments: w - the bsb menu entry widget.
 *      Returns: none.
 */

static void 
FlipColors(w)
Widget w;
{
    SmeMenuButtonObject entry = (SmeMenuButtonObject) w;

    if (entry->sme_bsb.set_values_area_cleared || entry->sme_bsb.inactive) 
	return;

    XFillRectangle(XtDisplayOfObject(w), XtWindowOfObject(w),
		   entry->sme_bsb.invert_gc, 0, (int) entry->rectangle.y,
		   (unsigned int) entry->rectangle.width, 
		   (unsigned int) entry->rectangle.height);
}

/************************************************************
 *
 * Private Functions.
 *
 ************************************************************/

/*	Function Name: GetDefaultSize
 *	Description: Calculates the Default (preferred) size of
 *                   this menu entry.
 *	Arguments: w - the menu entry widget.
 *                 width, height - default sizes (RETURNED).
 *	Returns: none.
 */

static void
GetDefaultSize(w, width, height) 
Widget w;
Dimension * width, * height;
{
    SmeMenuButtonObject entry = (SmeMenuButtonObject) w;

    if (entry->sme_bsb.label == NULL) 
	*width = 0;
    else
	*width = XTextWidth(entry->sme_bsb.font, entry->sme_bsb.label,
			    strlen(entry->sme_bsb.label));

    *width += entry->sme_bsb.left_margin + entry->sme_bsb.right_margin;
    
    *height = (entry->sme_bsb.font->max_bounds.ascent +
	       entry->sme_bsb.font->max_bounds.descent);

    *height = ((int)*height * ( ONE_HUNDRED + 
			        entry->sme_bsb.vert_space )) / ONE_HUNDRED;
}

/*      Function Name: DrawBitmaps
 *      Description: Draws left and right bitmaps.
 *      Arguments: w - the simple menu widget.
 *                 gc - graphics context to use for drawing.
 *      Returns: none
 */

static void
DrawBitmaps(w, gc)
Widget w;
GC gc;
{
    int x_loc, y_loc;
    SmeMenuButtonObject entry = (SmeMenuButtonObject) w;
    
    if ( (entry->sme_bsb.left_bitmap == None) && 
	 (entry->sme_bsb.right_bitmap == None) ) return;

/*
 * Draw Left Bitmap.
 */

  if (entry->sme_bsb.left_bitmap != None) {
    x_loc = (int)(entry->sme_bsb.left_margin -
	          entry->sme_bsb.left_bitmap_width) / 2;

    y_loc = entry->rectangle.y + (int)(entry->rectangle.height -
				       entry->sme_bsb.left_bitmap_height) / 2;

    XCopyPlane(XtDisplayOfObject(w), entry->sme_bsb.left_bitmap,
	       XtWindowOfObject(w), gc, 0, 0, 
	       entry->sme_bsb.left_bitmap_width,
	       entry->sme_bsb.left_bitmap_height, x_loc, y_loc, 1);
  }

/*
 * Draw Right Bitmap.
 */


  if (entry->sme_bsb.right_bitmap != None) {
    x_loc = entry->rectangle.width -
	      (int)(entry->sme_bsb.right_margin +
		    entry->sme_bsb.right_bitmap_width) / 2;

    y_loc = entry->rectangle.y + (int)(entry->rectangle.height -
				       entry->sme_bsb.right_bitmap_height) / 2;

    XCopyPlane(XtDisplayOfObject(w), entry->sme_bsb.right_bitmap,
	       XtWindowOfObject(w), gc, 0, 0, 
	       entry->sme_bsb.right_bitmap_width,
	       entry->sme_bsb.right_bitmap_height, x_loc, y_loc, 1);
  }
}

/*      Function Name: GetBitmapInfo
 *      Description: Gets the bitmap information from either of the bitmaps.
 *      Arguments: w - the bsb menu entry widget.
 *                 is_left - TRUE if we are testing left bitmap,
 *                           FALSE if we are testing the right bitmap.
 *      Returns: none
 */

static void
GetBitmapInfo(w, is_left)
Widget w;
Boolean is_left;
{
    SmeMenuButtonObject entry = (SmeMenuButtonObject) w;    
    unsigned int depth, bw;
    Window root;
    int x, y;
    unsigned int width, height;
    char buf[BUFSIZ];
    
    if (is_left) {
	if (entry->sme_bsb.left_bitmap != None) {
	    if (!XGetGeometry(XtDisplayOfObject(w), 
			      entry->sme_bsb.left_bitmap, &root, 
			      &x, &y, &width, &height, &bw, &depth)) {
		sprintf(buf, "SmeMenuButton Object: %s %s \"%s\".", "Could not",
			"get Left Bitmap geometry information for menu entry ",
			XtName(w));
		XtAppError(XtWidgetToApplicationContext(w), buf);
	    }
	    if (depth != 1) {
		sprintf(buf, "SmeMenuButton Object: %s \"%s\"%s.", 
			"Left Bitmap of entry ", 
			XtName(w), " is not one bit deep.");
		XtAppError(XtWidgetToApplicationContext(w), buf);
	    }
	    entry->sme_bsb.left_bitmap_width = (Dimension) width; 
	    entry->sme_bsb.left_bitmap_height = (Dimension) height;
	}
    }
    else if (entry->sme_bsb.right_bitmap != None) {
	if (!XGetGeometry(XtDisplayOfObject(w),
			  entry->sme_bsb.right_bitmap, &root,
			  &x, &y, &width, &height, &bw, &depth)) {
	    sprintf(buf, "SmeMenuButton Object: %s %s \"%s\".", "Could not",
		    "get Right Bitmap geometry information for menu entry ",
		    XtName(w));
	    XtAppError(XtWidgetToApplicationContext(w), buf);
	}
	if (depth != 1) {
	    sprintf(buf, "SmeMenuButton Object: %s \"%s\"%s.", 
		    "Right Bitmap of entry ", XtName(w),
		    " is not one bit deep.");
	    XtAppError(XtWidgetToApplicationContext(w), buf);
	}
	entry->sme_bsb.right_bitmap_width = (Dimension) width; 
	entry->sme_bsb.right_bitmap_height = (Dimension) height;
    }
}      

/*      Function Name: CreateGCs
 *      Description: Creates all gc's for the simple menu widget.
 *      Arguments: w - the simple menu widget.
 *      Returns: none.
 */

static void
CreateGCs(w)
Widget w;
{
    SmeMenuButtonObject entry = (SmeMenuButtonObject) w;    
    XGCValues values;
    XtGCMask mask;
    
    values.foreground = XtParent(w)->core.background_pixel;
    values.background = entry->sme_bsb.foreground;
    values.font = entry->sme_bsb.font->fid;
    values.graphics_exposures = FALSE;
    mask        = GCForeground | GCBackground | GCFont | GCGraphicsExposures;
    entry->sme_bsb.rev_gc = XtGetGC(w, mask, &values);
    
    values.foreground = entry->sme_bsb.foreground;
    values.background = XtParent(w)->core.background_pixel;
    entry->sme_bsb.norm_gc = XtGetGC(w, mask, &values);
    
    values.fill_style = FillTiled;
    values.tile   = XmuCreateStippledPixmap(XtScreenOfObject(w), 
					    entry->sme_bsb.foreground,
					    XtParent(w)->core.background_pixel,
					    XtParent(w)->core.depth);
    values.graphics_exposures = FALSE;
    mask |= GCTile | GCFillStyle;
    entry->sme_bsb.norm_gray_gc = XtGetGC(w, mask, &values);
    
    values.foreground ^= values.background;
    values.background = 0;
    values.function = GXxor;
    mask = GCForeground | GCBackground | GCGraphicsExposures | GCFunction;
    entry->sme_bsb.invert_gc = XtGetGC(w, mask, &values);
}

/*      Function Name: DestroyGCs
 *      Description: Removes all gc's for the simple menu widget.
 *      Arguments: w - the simple menu widget.
 *      Returns: none.
 */

static void
DestroyGCs(w)
Widget w;
{
    SmeMenuButtonObject entry = (SmeMenuButtonObject) w;    

    XtReleaseGC(w, entry->sme_bsb.norm_gc);
    XtReleaseGC(w, entry->sme_bsb.norm_gray_gc);
    XtReleaseGC(w, entry->sme_bsb.rev_gc);
    XtReleaseGC(w, entry->sme_bsb.invert_gc);
}

static void
PopupMenu(w)
Widget w;
{
  SmeMenuButtonObject mbw = (SmeMenuButtonObject) w;
  Widget menu, temp;
  Arg arglist[2];
  Cardinal num_args;
  int menu_x, menu_y, menu_width, menu_height, button_width;
  Position button_x, button_y;

  if (mbw->sme_bsb.menu_name == NULL || strlen(mbw->sme_bsb.menu_name) == 0)
      return;

  temp = w;
  while(temp != NULL) {
    menu = XtNameToWidget(temp, mbw->sme_bsb.menu_name);
    if (menu == NULL) 
      temp = XtParent(temp);
    else
      break;
  }

  if (menu == NULL) {
    char error_buf[BUFSIZ];
    sprintf(error_buf, "MenuButton: %s %s.",
	    "Could not find menu widget named", mbw->sme_bsb.menu_name);
    XtAppWarning(XtWidgetToApplicationContext(w), error_buf);
    return;
  }
  if (!XtIsRealized(menu))
    XtRealizeWidget(menu);
  
  menu_width = menu->core.width + 2 * menu->core.border_width;
  button_width = mbw->rectangle.width + 2 * mbw->rectangle.border_width;
  menu_height = menu->core.height + 2 * menu->core.border_width;

  XtTranslateCoords(w, 0, 0, &button_x, &button_y);
  menu_x = button_x + button_width;
  menu_y = button_y;

  if (menu_x >= 0) {
    int scr_width = WidthOfScreen(XtScreen(menu));
    if (menu_x + menu_width > scr_width)
      menu_x = scr_width - menu_width;
  }
  if (menu_x < 0) 
    menu_x = 0;

  if (menu_y >= 0) {
    int scr_height = HeightOfScreen(XtScreen(menu));
    if (menu_y + menu_height > scr_height)
      menu_y = scr_height - menu_height;
  }
  if (menu_y < 0)
    menu_y = 0;

  num_args = 0;
  XtSetArg(arglist[num_args], XtNx, menu_x); num_args++;
  XtSetArg(arglist[num_args], XtNy, menu_y); num_args++;
  XtSetValues(menu, arglist, num_args);

  XtPopupSpringLoaded(menu);
}

static void
Unhighlight(w)
Widget w;
{
    SmeMenuButtonObject mbw = (SmeMenuButtonObject) w;
    Display  *display;
    Screen   *screen;
    Window   win, rootwin;
    int      rootwin_x, rootwin_y;
    int      win_x, win_y;
    unsigned int mask;
    Position left, right, top, bottom;

    if (mbw->sme_bsb.inactive)
	return;
    
    display = XtDisplayOfObject(w);
    screen  = XtScreenOfObject(w);
    XQueryPointer(display, RootWindowOfScreen(screen),
                  &rootwin, &win, &rootwin_x, &rootwin_y,
                  &win_x, &win_y, &mask);

    XtTranslateCoords(w, 0, 0, &left, &top);
    XtTranslateCoords(w, mbw->rectangle.width, mbw->rectangle.height, 
		      &right, &bottom);

    if (rootwin_x >= right && rootwin_y >= top && rootwin_y < bottom)
    {
	PopupMenu(w);
    }

    FlipColors(w);
}

static void
RemoveAmpersand(w)
Widget w;
{
    SmeMenuButtonObject entry = (SmeMenuButtonObject) w;

    entry->sme_bsb.ul_pos = strcspn(entry->sme_bsb.label, "&");
    if (entry->sme_bsb.ul_pos == strlen(entry->sme_bsb.label))
    {
	entry->sme_bsb.ul_pos = -1;
	return;
    }

    /* Remove ampersand from label */
    strcpy(entry->sme_bsb.label + entry->sme_bsb.ul_pos,
	   entry->sme_bsb.label + entry->sme_bsb.ul_pos + 1);
}

