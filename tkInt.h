/*
 * tkInt.h --
 *
 *	Declarations for things used internally by the Tk
 *	procedures but not exported outside the module.
 *
 * Copyright 1990-1992 Regents of the University of California.
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /user6/ouster/wish/RCS/tkInt.h,v 1.84 93/01/23 16:59:14 ouster Exp $ SPRITE (Berkeley)
 */

#ifndef _TKINT
#define _TKINT

#ifndef _XLIB_H_
#include <X11/Xlib.h>
#endif
#ifndef _XUTIL_H
#include <X11/Xutil.h>
#endif
#ifndef _TK
#include "tk.h"
#endif
#ifndef _TCL
#include "tcl.h"
#endif
#ifndef _TCLHASH
#include "tclHash.h"
#endif

/*
 * Opaque type declarations:
 */

typedef struct Tk_PostscriptInfo Tk_PostscriptInfo;
typedef struct TkGrabEvent TkGrabEvent;

/*
 * One of the following structures is maintained for each display
 * containing a window managed by Tk:
 */

typedef struct TkDisplay {
    Display *display;		/* Xlib's info about display. */
    struct TkDisplay *nextPtr;	/* Next in list of all displays. */
    char *name;			/* Name of display (with any screen
				 * identifier removed).  Malloc-ed. */
    Time lastEventTime;		/* Time of last event received for this
				 * display. */

    /*
     * Information used by tkFocus.c and tkEvent.c:
     */

    struct TkWindow *focusTopLevelPtr;
				/* Pointer to the top-level window that
				 * currently contains the focus for this
				 * display.  NULL means none of the
				 * top-levels managed by this application
				 * contains the focus. */
    int focussedOnEnter;	/* Non-zero means the focus was set
				 * implicitly from an Enter event rather
				 * than from a FocusIn event. */

    /*
     * Information used primarily by tkBind.c:
     */

    int bindInfoStale;		/* Non-zero means the variables in this
				 * part of the structure are potentially
				 * incorrect and should be recomputed. */
    unsigned int modeModMask;	/* Has one bit set to indicate the modifier
				 * corresponding to "mode shift".  If no
				 * such modifier, than this is zero. */
    enum {IGNORE, CAPS, SHIFT} lockUsage;
				/* Indicates how to interpret lock modifier. */

    /*
     * Information used by tkError.c only:
     */

    struct TkErrorHandler *errorPtr;
				/* First in list of error handlers
				 * for this display.  NULL means
				 * no handlers exist at present. */
    int deleteCount;		/* Counts # of handlers deleted since
				 * last time inactive handlers were
				 * garbage-collected.  When this number
				 * gets big, handlers get cleaned up. */

    /*
     * Information used by tkSend.c only:
     */

    Tk_Window commWindow;	/* Window used for communication
				 * between interpreters during "send"
				 * commands.  NULL means send info hasn't
				 * been initialized yet. */
    Atom commProperty;		/* X's name for comm property. */
    Atom registryProperty;	/* X's name for property containing
				 * registry of interpreter names. */

    /*
     * Information used by tkSelect.c only:
     */

    Tk_Window selectionOwner;	/* Current owner of selection, or
				 * NULL if selection isn't owned by
				 * a window in this process.  */
    int selectionSerial;	/* Serial number of last XSelectionSetOwner
				 * request we made to server (used to
				 * filter out redundant SelectionClear
				 * events. */
    Time selectionTime;		/* Timestamp used to acquire selection. */
    Atom multipleAtom;		/* Atom for MULTIPLE.  None means
				 * selection stuff isn't initialized. */
    Atom incrAtom;		/* Atom for INCR. */
    Atom targetsAtom;		/* Atom for TARGETS. */
    Atom timestampAtom;		/* Atom for TIMESTAMP. */
    Atom textAtom;		/* Atom for TEXT. */
    Atom compoundTextAtom;	/* Atom for COMPOUND_TEXT. */
    Atom applicationAtom;	/* Atom for APPLICATION. */
    Atom windowNameAtom;	/* Atom for WINDOW_NAME. */

    /*
     * Information used by tkAtom.c only:
     */

    int atomInit;		/* 0 means stuff below hasn't been
				 * initialized yet. */
    Tcl_HashTable nameTable;	/* Maps from names to Atom's. */
    Tcl_HashTable atomTable;	/* Maps from Atom's back to names. */

    /*
     * Information used by tkCursor.c only:
     */

    Font cursorFont;		/* Font to use for standard cursors.
				 * None means font not loaded yet. */

    /*
     * Information used by tkGrab.c only:
     */

    struct TkWindow *grabWinPtr;
				/* Window in which the pointer is currently
				 * grabbed, or NULL if none. */
    struct TkWindow *eventualGrabWinPtr;
				/* Value that grabWinPtr will have once the
				 * grab event queue (below) has been
				 * completely emptied. */
    struct TkWindow *buttonWinPtr;
				/* Window in which first mouse button was
				 * pressed while grab was in effect, or NULL
				 * if no such press in effect. */
    struct TkWindow *serverWinPtr;
				/* If no application contains the pointer then
				 * this is NULL.  Otherwise it contains the
				 * last window for which we've gotten an
				 * Enter or Leave event from the server (i.e.
				 * the last window known to have contained
				 * the pointer).  Doesn't reflect events
				 * that were synthesized in tkGrab.c. */
    TkGrabEvent *firstGrabEventPtr;
				/* First in list of enter/leave events
				 * synthesized by grab code.  These events
				 * must be processed in order before any other
				 * events are processed.  NULL means no such
				 * events. */
    TkGrabEvent *lastGrabEventPtr;
				/* Last in list of synthesized events, or NULL
				 * if list is empty. */
    int grabFlags;		/* Miscellaneous flag values.  See definitions
				 * in tkGrab.c. */

    /*
     * Miscellaneous information:
     */

    Tk_ColorModel *colorModels;	/* Array of color models, one per screen;
				 * indicates whether windows should attempt
				 * to use full color for display, just mono,
				 * etc.  Malloc'ed. */
} TkDisplay;

/*
 * One of the following structures exists for each error handler
 * created by a call to Tk_CreateErrorHandler.  The structure
 * is managed by tkError.c.
 */

typedef struct TkErrorHandler {
    TkDisplay *dispPtr;		/* Display to which handler applies. */
    unsigned long firstRequest;	/* Only errors with serial numbers
				 * >= to this are considered. */
    unsigned long lastRequest;	/* Only errors with serial numbers
				 * <= to this are considered.  This
				 * field is filled in when XUnhandle
				 * is called.  -1 means XUnhandle
				 * hasn't been called yet. */
    int error;			/* Consider only errors with this
				 * error_code (-1 means consider
				 * all errors). */
    int request;		/* Consider only errors with this
				 * major request code (-1 means
				 * consider all major codes). */
    int minorCode;		/* Consider only errors with this
				 * minor request code (-1 means
				 * consider all minor codes). */
    Tk_ErrorProc *errorProc;	/* Procedure to invoke when a matching
				 * error occurs.  NULL means just ignore
				 * errors. */
    ClientData clientData;	/* Arbitrary value to pass to
				 * errorProc. */
    struct TkErrorHandler *nextPtr;
				/* Pointer to next older handler for
				 * this display, or NULL for end of
				 * list. */
} TkErrorHandler;

/*
 * One of the following structures exists for each event handler
 * created by calling Tk_CreateEventHandler.  This information
 * is used by tkEvent.c only.
 */

typedef struct TkEventHandler {
    unsigned long mask;		/* Events for which to invoke
				 * proc. */
    Tk_EventProc *proc;		/* Procedure to invoke when an event
				 * in mask occurs. */
    ClientData clientData;	/* Argument to pass to proc. */
    struct TkEventHandler *nextPtr;
				/* Next in list of handlers
				 * associated with window (NULL means
				 * end of list). */
} TkEventHandler;

/*
 * One of the following structures exists for each selection
 * handler created by calling Tk_CreateSelHandler.  This
 * information is used by tkSelect.c only.
 */

typedef struct TkSelHandler {
    Atom target;		/* Target type for selection
				 * conversion, such as TARGETS or
				 * STRING. */
    Atom format;		/* Format in which selection
				 * info will be returned, such
				 * as STRING or ATOM. */
    Tk_SelectionProc *proc;	/* Procedure to generate selection
				 * in this format. */
    ClientData clientData;	/* Argument to pass to proc. */
    int size;			/* Size of units returned by proc
				 * (8 for STRING, 32 for almost
				 * anything else). */
    struct TkSelHandler *nextPtr;
				/* Next selection handler associated
				 * with same window (NULL for end of
				 * list). */
} TkSelHandler;

/*
 * Tk keeps one of the following data structures for each main
 * window (created by a call to Tk_CreateMainWindow).  It stores
 * information that is shared by all of the windows associated
 * with a particular main window.
 */

typedef struct TkMainInfo {
    struct TkWindow *winPtr;	/* Pointer to main window. */
    Tcl_Interp *interp;		/* Interpreter associated with application. */
    Tcl_HashTable nameTable;	/* Hash table mapping path names to TkWindow
				 * structs for all windows related to this
				 * main window.  Managed by tkWindow.c. */
    Tk_BindingTable bindingTable;
				/* Used in conjunction with "bind" command
				 * to bind events to Tcl commands. */
    struct TkWindow *focusPtr;	/* Identifies window that currently has the
				 * focus (or that will get the focus the next
				 * time the pointer enters any of the top-level
				 * windows associated with this main window).
				 * NULL means nobody has the focus.
				 * Managed by tkFocus.c. */
    struct TkWindow *focusDefaultPtr;
				/* Window that is to receive the focus by
				 * default when the focusPtr window is
				 * deleted. */
    struct ElArray *optionRootPtr;
				/* Top level of option hierarchy for this
				 * main window.  NULL means uninitialized.
				 * Managed by tkOption.c. */
} TkMainInfo;

/*
 * Tk keeps one of the following structures for each window.
 * Some of the information (like size and location) is a shadow
 * of information managed by the X server, and some is special
 * information used here, such as event and geometry management
 * information.  This information is (mostly) managed by tkWindow.c.
 * WARNING: the declaration below must be kept consistent with the
 * Tk_ClientWindow structure in tk.h.  If you change one, be sure to
 * change the other!!
 */

typedef struct TkWindow {

    /*
     * Structural information:
     */

    Display *display;		/* Display containing window. */
    TkDisplay *dispPtr;		/* Tk's information about display
				 * for window. */
    int screenNum;		/* Index of screen for window, among all
				 * those for dispPtr. */
    Visual *visual;		/* Visual to use for window.  If not default,
				 * MUST be set before X window is created. */
    int depth;			/* Number of bits/pixel. */
    Window window;		/* X's id for window.   NULL means window
				 * hasn't actually been created yet, or it's
				 * been deleted. */
    struct TkWindow *childList;	/* First in list of child windows,
				 * or NULL if no children. */
    struct TkWindow *parentPtr;	/* Pointer to parent window (logical
				 * parent, not necessarily X parent), or
				 * NULL if this is a main window. */
    struct TkWindow *nextPtr;	/* Next in list of children with
				 * same parent (NULL if end of
				 * list). */
    TkMainInfo *mainPtr;	/* Information shared by all windows
				 * associated with a particular main
				 * window.  NULL means this window is
				 * a rogue that isn't associated with
				 * any application (at present, there
				 * should never be any rogues).  */

    /*
     * Name and type information for the window:
     */

    char *pathName;		/* Path name of window (concatenation
				 * of all names between this window and
				 * its top-level ancestor).  This is a
				 * pointer into an entry in
				 * mainPtr->nameTable or NULL if mainPtr
				 * is NULL. */
    Tk_Uid nameUid;		/* Name of the window within its parent
				 * (unique within the parent). */
    Tk_Uid classUid;		/* Class of the window.  NULL means window
				 * hasn't been given a class yet. */

    /*
     * Geometry and other attributes of window.  This information
     * may not be updated on the server immediately;  stuff that
     * hasn't been reflected in the server yet is called "dirty".
     * At present, information can be dirty only if the window
     * hasn't yet been created.
     */

    XWindowChanges changes;	/* Geometry and other info about
				 * window. */
    unsigned int dirtyChanges;	/* Bits indicate fields of "changes"
				 * that are dirty. */
    XSetWindowAttributes atts;	/* Current attributes of window. */
    unsigned long dirtyAtts;	/* Bits indicate fields of "atts"
				 * that are dirty. */

    unsigned int flags;		/* Various flag values:  these are all
				 * defined in tk.h (confusing, but they're
				 * needed there for some query macros). */

    /*
     * Information kept by the event manager (tkEvent.c):
     */

    TkEventHandler *handlerList;/* First in list of event handlers
				 * declared for this window, or
				 * NULL if none. */
    /*
     * Information related to input focussing (tkFocus.c):
     */

    Tk_FocusProc *focusProc;	/* Procedure to invoke when this window
				 * gets or loses the input focus.  NULL
				 * means this window is not prepared to
				 * receive the focus. */
    ClientData focusData;	/* Arbitrary value to pass to focusProc. */

    /*
     * Information used by tkOption.c to manage options for the
     * window.
     */

    int optionLevel;		/* -1 means no option information is
				 * currently cached for this window.
				 * Otherwise this gives the level in
				 * the option stack at which info is
				 * cached. */
    /*
     * Information used by tkSelect.c to manage the selection.
     */

    TkSelHandler *selHandlerList;
				/* First in list of handlers for
				 * returning the selection in various
				 * forms. */
    Tk_LostSelProc *selClearProc;
    ClientData selClearData;	/* Info to pass to selClearProc. */

    /*
     * Information used by tkGeometry.c for geometry management.
     */

    Tk_GeometryProc *geomProc;	/* Procedure to handle geometry
				 * requests (NULL means no window is
				 * unmanaged). */
    ClientData geomData;	/* Argument for geomProc. */
    int reqWidth, reqHeight;	/* Arguments from last call to
				 * Tk_GeometryRequest, or 0's if
				 * Tk_GeometryRequest hasn't been
				 * called. */
    int internalBorderWidth;	/* Width of internal border of window
				 * (0 means no internal border).  Geom.
				 * mgr. should not place children on top
				 * of the border. */

    /*
     * Information maintained by tkWm.c for window manager communication.
     */

    struct TkWmInfo *wmInfoPtr;	/* For top-level windows, points to
				 * structure with wm-related info (see
				 * tkWm.c).  For other windows, this
				 * is NULL. */
} TkWindow;

/*
 * The context below is used to map from an X window id to
 * the TkWindow structure associated with the window.
 */

extern XContext tkWindowContext;

/*
 * Pointer to first entry in list of all displays currently known.
 */

extern TkDisplay *tkDisplayList;

/*
 * Flags passed to TkMeasureChars:
 */

#define TK_WHOLE_WORDS		1
#define TK_AT_LEAST_ONE		2
#define TK_PARTIAL_OK		4
#define TK_NEWLINES_NOT_SPECIAL	8

/*
 * Location of library directory containing Tk scripts.  This value
 * is put in the $tkLibrary variable for each application.
 */

#ifndef TK_LIBRARY
#define TK_LIBRARY "/usr/local/lib/tk"
#endif

/*
 * Miscellaneous variables shared among Tk modules but not exported
 * to the outside world:
 */

extern Tk_Uid		tkActiveUid;
extern Tk_Uid		tkDisabledUid;
extern Tk_Uid		tkNormalUid;

/*
 * Internal procedures shared among Tk modules but not exported
 * to the outside world:
 */

extern int		TkAreaToPolygon _ANSI_ARGS_((double *polyPtr,
			    int numPoints, double *rectPtr));
extern void		TkBezierPoints _ANSI_ARGS_((double control[],
			    int numSteps, double *coordPtr));
extern void		TkBindEventProc _ANSI_ARGS_((TkWindow *winPtr,
			    XEvent *eventPtr));
extern Time		TkCurrentTime _ANSI_ARGS_((TkDisplay *dispPtr));
extern int		TkDeadAppCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
extern void		TkDisplayChars _ANSI_ARGS_((Display *display,
			    Drawable drawable, GC gc,
			    XFontStruct *fontStructPtr, char *string,
			    int numChars, int x, int y, int flags));
extern void		TkEventDeadWindow _ANSI_ARGS_((TkWindow *winPtr));
extern void		TkFocusDeadWindow _ANSI_ARGS_((TkWindow *winPtr));
extern int		TkFocusFilterEvent _ANSI_ARGS_((TkWindow *winPtr,
			    XEvent *eventPtr));
extern void		TkGetButtPoints _ANSI_ARGS_((double p1[], double p2[],
			    double width, int project, double m1[],
			    double m2[]));
extern int		TkGetInterpNames _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_Window tkwin));
extern int		TkGetMiterPoints _ANSI_ARGS_((double p1[], double p2[],
			    double p3[], double width, double m1[],
			    double m2[]));
extern void		TkGrabDeadWindow _ANSI_ARGS_((TkWindow *winPtr));
extern void		TkGrabTriggerProc _ANSI_ARGS_((XEvent *eventPtr));
extern int		TkLineToArea _ANSI_ARGS_((double end1Ptr[2],
			    double end2Ptr[2], double rectPtr[4]));
extern double		TkLineToPoint _ANSI_ARGS_((double end1Ptr[2],
			    double end2Ptr[2], double pointPtr[2]));
extern void		TkMakeBezierPostscript _ANSI_ARGS_((Tcl_Interp *interp,
			    double *pointPtr, int numPoints,
			    Tk_PostscriptInfo *psInfoPtr));
extern int		TkMeasureChars _ANSI_ARGS_((XFontStruct *fontStructPtr,
			    char *source, int maxChars, int startX, int maxX,
			    int flags, int *nextXPtr));
extern void		TkOptionDeadWindow _ANSI_ARGS_((TkWindow *winPtr));
extern int		TkOvalToArea _ANSI_ARGS_((double *ovalPtr,
			    double *rectPtr));
extern double		TkOvalToPoint _ANSI_ARGS_((double ovalPtr[4],
			    double width, int filled, double pointPtr[2]));
extern int		TkPointerEvent _ANSI_ARGS_((XEvent *eventPtr,
			    TkWindow *winPtr));
extern int		TkPolygonToArea _ANSI_ARGS_((double *polyPtr,
			    int numPoints, double *rectPtr));
extern double		TkPolygonToPoint _ANSI_ARGS_((double *polyPtr,
			    int numPoints, double *pointPtr));
extern void		TkSelDeadWindow _ANSI_ARGS_((TkWindow *winPtr));
extern void		TkSelEventProc _ANSI_ARGS_((Tk_Window tkwin,
			    XEvent *eventPtr));
extern void		TkSelPropProc _ANSI_ARGS_((XEvent *eventPtr));
extern void		TkUnderlineChars _ANSI_ARGS_((Display *display,
			    Drawable drawable, GC gc,
			    XFontStruct *fontStructPtr, char *string,
			    int x, int y, int flags, int firstChar,
			    int lastChar));
extern void		TkWmDeadWindow _ANSI_ARGS_((TkWindow *winPtr));
extern void		TkWmMapWindow _ANSI_ARGS_((TkWindow *winPtr));
extern void		TkWmProtocolEventProc _ANSI_ARGS_((TkWindow *winPtr,
			    XEvent *evenvPtr));
extern void		TkWmSetClass _ANSI_ARGS_((TkWindow *winPtr));
extern void		TkWmNewWindow _ANSI_ARGS_((TkWindow *winPtr));

#endif  /* _TKINT */
