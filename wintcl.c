#define _WINMAIN
/* 
 * main.c --
 *
 *	This file contains the main program for "wish", a windowing
 *	shell based on Tk and Tcl.  It also provides a template that
 *	can be used as the basis for main programs for other Tk
 *	applications.
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
 * Modifiyed by Peter MacDonald for windows API.
 */

#ifndef lint
static char rcsid[] = "$Header: /user6/ouster/wish/RCS/main.c,v 1.72 93/02/03 10:20:42 ouster Exp $ SPRITE (Berkeley)";
#endif


#include <stdio.h>
#include <stdarg.h>
#include "tkConfig.h"
#include "tkInt.h"

#include "callback.h"

#define dprintf(n,s) dodprintf(s)

void dodprintf(char *str, ... )
{
	va_list va;
	char buf[2000];
	va_start(va, str);
	vsprintf(buf,str,va);
	puts(buf);
	va_end(str);
}

#define TK_EXTENDED
#ifdef TK_EXTENDED
#    include "tclExtend.h"
     extern Tcl_Interp *tk_mainInterp;  /* Need to process signals */
#endif

/*
 * Declarations for library procedures:
 */

extern int isatty();

/*
 * Command used to initialize wish:
 */

#ifdef TK_EXTENDED
static char initCmd[] = "load wishx.tcl";
#else
static char initCmd[] = "source $tk_library/wish.tcl";
#endif

/*
 * Global variables used by the main program:
 */

static Tk_Window w;		/* The main window for the application.  If
				 * NULL then the application no longer
				 * exists. */
static Tcl_Interp *interp;	/* Interpreter for this application. */
static int x, y;		/* Coordinates of last location moved to;
				 * used by "moveto" and "lineto" commands. */
static Tcl_CmdBuf buffer;	/* Used to assemble lines of terminal input
				 * into Tcl commands. */
static int tty;			/* Non-zero means standard input is a
				 * terminal-like device.  Zero means it's
				 * a file. */

/*
 * Command-line options:
 */

int synchronize = 0;
char *fileName = NULL;
char *name = NULL;
char *display = NULL;
char *geometry = NULL;

Tk_ArgvInfo argTable[] = {
    {"-file", TK_ARGV_STRING, (char *) NULL, (char *) &fileName,
	"File from which to read commands"},
    {"-geometry", TK_ARGV_STRING, (char *) NULL, (char *) &geometry,
	"Initial geometry for window"},
    {"-display", TK_ARGV_STRING, (char *) NULL, (char *) &display,
	"Display to use"},
    {"-name", TK_ARGV_STRING, (char *) NULL, (char *) &name,
	"Name to use for application"},
    {"-sync", TK_ARGV_CONSTANT, (char *) 1, (char *) &synchronize,
	"Use synchronous mode for display server"},
    {(char *) NULL, TK_ARGV_END, (char *) NULL, (char *) NULL,
	(char *) NULL}
};

/*
 * Declaration for Tcl command procedure to create demo widget.  This
 * procedure is only invoked if SQUARE_DEMO is defined.
 */

extern int Tk_SquareCmd _ANSI_ARGS_((ClientData clientData,
	Tcl_Interp *interp, int argc, char **argv));

/*
 * Forward declarations for procedures defined later in this file:
 */

static void		DelayedMap _ANSI_ARGS_((ClientData clientData));
static int		LinetoCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
static int		MovetoCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
static void		StdinProc _ANSI_ARGS_((ClientData clientData,
			    int mask));
static void		StructureProc _ANSI_ARGS_((ClientData clientData,
			    XEvent *eventPtr));
static int		_WinCallBack _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
static int		_getStrHandle _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Main program for Wish.
 *
 * Results:
 *	None. This procedure never returns (it exits the process when
 *	it's done
 *
 * Side effects:
 *	This procedure initializes the wish world and then starts
 *	interpreting commands;  almost anything could happen, depending
 *	on the script being interpreted.
 *
 *----------------------------------------------------------------------
 */

int
main( int argc,				/* Number of arguments. */
    char **argv)			/* Array of argument strings. */
{
    char *args, *p, *msg;
    char buf[20];  char bigBuf[300];
    int result;
    Tk_3DBorder border;

#ifdef TK_EXTENDED
    tk_mainInterp = interp = Tcl_CreateExtendedInterp();
#else
    interp = Tcl_CreateInterp();
#endif
#ifdef TCL_MEM_DEBUG
    Tcl_InitMemory(interp);
#endif

    /*
     * Parse command-line arguments.
     */

#if 0
    if (Tk_ParseArgv(interp, (Tk_Window) NULL, &argc, argv, argTable, 0)
	    != TCL_OK) {
	fprintf(stderr, "%s\n", interp->result);
	exit(1);
    }
#endif
    if (name == NULL) {
	if (fileName != NULL) {
	    p = fileName;
	} else {
	    p = argv[0];
	}
	name = strrchr(p, '/');
	if (name != NULL) {
	    name++;
	} else {
	    name = p;
	}
    }

    /*
     * Initialize the Tk application and arrange to map the main window
     * after the startup script has been executed, if any.  This way
     * the script can withdraw the window so it isn't ever mapped
     * at all.
     */

    w = Tk_CreateMainWindow(interp, display, name);
    if (w == NULL) {
	fprintf(stderr, "%s\n", interp->result);
	exit(1);
    }
    Tk_SetClass(w, "Tk");
    Tk_CreateEventHandler(w, StructureNotifyMask, StructureProc,
	    (ClientData) NULL);
    Tk_DoWhenIdle(DelayedMap, (ClientData) NULL);
    if (synchronize) {
	XSynchronize(Tk_Display(w), True);
    }
    Tk_GeometryRequest(w, 200, 200);
    border = Tk_Get3DBorder(interp, w, None, "#ffe4c4");
    if (border == NULL) {
	Tcl_SetResult(interp, (char *) NULL, TCL_STATIC);
	Tk_SetWindowBackground(w, WhitePixelOfScreen(Tk_Screen(w)));
    } else {
	Tk_SetBackgroundFromBorder(w, border);
    }
    XSetForeground(Tk_Display(w), DefaultGCOfScreen(Tk_Screen(w)),
	    BlackPixelOfScreen(Tk_Screen(w)));

    /*
     * Make command-line arguments available in the Tcl variables "argc"
     * and "argv".  Also set the "geometry" variable from the geometry
     * specified on the command line.
     */

#if 0
    args = Tcl_Merge(argc-1, argv+1);
    Tcl_SetVar(interp, "argv", args, TCL_GLOBAL_ONLY);
    ckfree(args);
    sprintf(buf, "%d", argc-1);
    Tcl_SetVar(interp, "argc", buf, TCL_GLOBAL_ONLY);
#endif
    if (geometry != NULL) {
	Tcl_SetVar(interp, "geometry", geometry, TCL_GLOBAL_ONLY);
    }

    /*
     * Add a few application-specific commands to the application's
     * interpreter.
     */

    Tcl_CreateCommand(interp, "lineto", LinetoCmd, (ClientData) w,
	    (void (*)()) NULL);
    Tcl_CreateCommand(interp, "moveto", MovetoCmd, (ClientData) w,
	    (void (*)()) NULL);
#ifdef SQUARE_DEMO
    Tcl_CreateCommand(interp, "square", Tk_SquareCmd, (ClientData) w,
	    (void (*)()) NULL);
#endif
    Tcl_CreateCommand(interp, "wincallback", _WinCallBack, (ClientData) w,
	    (void (*)()) NULL);
    Tcl_CreateCommand(interp, "getstrhandle", _getStrHandle, (ClientData) w,
	    (void (*)()) NULL);

    /*
     * Execute Wish's initialization script, followed by the script specified
     * on the command line, if any.
     */

#ifdef TK_EXTENDED
    tclAppName     = "Wish";
    tclAppLongname = "Wish - Tk Shell";
    tclAppVersion  = TK_VERSION;
    Tcl_ShellEnvInit (interp, TCLSH_ABORT_STARTUP_ERR,
                      name,
                      0, NULL,           /* argv var already set  */
                      fileName == NULL,  /* interactive?          */
                      NULL);             /* Standard default file */
#endif
    result = Tcl_Eval(interp, initCmd, 0, (char **) NULL);
    if (result != TCL_OK) {
	goto error;
    }
    strcpy(bigBuf, Tcl_GetVar(interp, "auto_path", TCL_GLOBAL_ONLY));
    strcat(bigBuf," /usr/local/windows");
    Tcl_SetVar(interp, "auto_path", bigBuf, TCL_GLOBAL_ONLY);
    dprintf(4,("set auto_path \"$auto_path /usr/local/windows\""));
    if (result != TCL_OK) {
	goto error;
    }
#if 0
    tty = isatty(0);
    if (fileName != NULL) {
	result = Tcl_VarEval(interp, "source ", fileName, (char *) NULL);
	if (result != TCL_OK) {
	    goto error;
	}
	tty = 0;
    } else {
	/*
	 * Commands will come from standard input.  Set up a handler
	 * to receive those characters and print a prompt if the input
	 * device is a terminal.
	 */

	Tk_CreateFileHandler(0, TK_READABLE, StdinProc, (ClientData) 0);
	if (tty) {
	    printf("wish: ");
	}
    }
#endif
    fflush(stdout);
    buffer = Tcl_CreateCmdBuf();
    (void) Tcl_Eval(interp, "update", 0, (char **) NULL);

    /*
     * Loop infinitely, waiting for commands to execute.  When there
     * are no windows left, Tk_MainLoop returns and we clean up and
     * exit.
     */
/* Tcl_Eval( interp, "button .hello -text \"Hello, world\" -command {\n puts stdout \"Hello, world\"; destroy .\n\
}\n pack append . .hello {top}\n", 0, (char **)NULL); */
    _WinMain(argc,argv);
    Tcl_DeleteInterp(interp);
    Tcl_DeleteCmdBuf(buffer);
    exit(0);

error:
    msg = Tcl_GetVar(interp, "errorInfo", TCL_GLOBAL_ONLY);
    if (msg == NULL) {
	msg = interp->result;
    }
    fprintf(stderr, "%s\n", msg);
    Tcl_Eval(interp, "destroy .", 0, (char **) NULL);
    exit(1);
    return 0;			/* Needed only to prevent compiler warnings. */
}

/*
 *----------------------------------------------------------------------
 *
 * StdinProc --
 *
 *	This procedure is invoked by the event dispatcher whenever
 *	standard input becomes readable.  It grabs the next line of
 *	input characters, adds them to a command being assembled, and
 *	executes the command if it's complete.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Could be almost arbitrary, depending on the command that's
 *	typed.
 *
 *----------------------------------------------------------------------
 */

    /* ARGSUSED */
static void
StdinProc(clientData, mask)
    ClientData clientData;		/* Not used. */
    int mask;				/* Not used. */
{
#define BUFFER_SIZE 4000
    char input[BUFFER_SIZE+1];
    static int gotPartial = 0;
    char *cmd;
    int result, count;

count=strlen(input);
    if (count <= 0) {
	if (!gotPartial) {
	    if (tty) {
		Tcl_Eval(interp, "destroy .", 0, (char **) NULL);
		exit(0);
	    } else {
		Tk_DeleteFileHandler(0);
	    }
	    return;
	} else {
	    input[0] = 0;
	}
    } else {
	input[count] = 0;
    }
    cmd = Tcl_AssembleCmd(buffer, input);
    if (cmd == NULL) {
	gotPartial = 1;
	return;
    }
    gotPartial = 0;
    result = Tcl_RecordAndEval(interp, cmd, 0);
    if (*interp->result != 0) {
	if ((result != TCL_OK) || (tty)) {
	    printf("%s\n", interp->result);
	}
    }
    if (tty) {
	printf("wish: ");
	fflush(stdout);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * StructureProc --
 *
 *	This procedure is invoked whenever a structure-related event
 *	occurs on the main window.  If the window is deleted, the
 *	procedure modifies "w" to record that fact.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Variable "w" may get set to NULL.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static void
StructureProc(clientData, eventPtr)
    ClientData clientData;	/* Information about window. */
    XEvent *eventPtr;		/* Information about event. */
{
    if (eventPtr->type == DestroyNotify) {
	w = NULL;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DelayedMap --
 *
 *	This procedure is invoked by the event dispatcher once the
 *	startup script has been processed.  It waits for all other
 *	pending idle handlers to be processed (so that all the
 *	geometry information will be correct), then maps the
 *	application's main window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The main window gets mapped.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static void
DelayedMap(clientData)
    ClientData clientData;	/* Not used. */
{

    while (Tk_DoOneEvent(TK_IDLE_EVENTS) != 0) {
	/* Empty loop body. */
    }
    if (w == NULL) {
	return;
    }
    Tk_MapWindow(w);
}

/*
 *----------------------------------------------------------------------
 *
 * MoveToCmd and LineToCmd --
 *
 *	This procedures are registered as the command procedures for
 *	"moveto" and "lineto" Tcl commands.  They provide a trivial
 *	drawing facility.  They don't really work right, in that the
 *	drawn information isn't persistent on the screen (it will go
 *	away if the window is iconified and de-iconified again).  The
 *	commands are here partly for testing and partly to illustrate
 *	how to add application-specific commands to Tk.  You probably
 *	shouldn't use these commands in any real scripts.
 *
 * Results:
 *	The procedures return standard Tcl results.
 *
 * Side effects:
 *	The screen gets modified.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
MovetoCmd(dummy, interp, argc, argv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    char **argv;			/* Argument strings. */
{
    if (argc != 3) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" x y\"", (char *) NULL);
	return TCL_ERROR;
    }
    x = strtol(argv[1], (char **) NULL, 0);
    y = strtol(argv[2], (char **) NULL, 0);
    return TCL_OK;
}
	/* ARGSUSED */
static int
LinetoCmd(dummy, interp, argc, argv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    char **argv;			/* Argument strings. */
{
    int newX, newY;

    if (argc != 3) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" x y\"", (char *) NULL);
	return TCL_ERROR;
    }
    newX = strtol(argv[1], (char **) NULL, 0);
    newY = strtol(argv[2], (char **) NULL, 0);
    Tk_MakeWindowExist(w);
    XDrawLine(Tk_Display(w), Tk_WindowId(w),
	    DefaultGCOfScreen(Tk_Screen(w)), x, y, newX, newY);
    x = newX;
    y = newY;
    return TCL_OK;
}

/*===============================================================*/

#define _WIN_CODE_SECTION
static int dte(char *str,...);
#include <windows.h>
#include <assert.h>


LPWNDCLASS	LpWndClass;

static int tclexec(char *str, ... )
{	int result;
	va_list va;
	char buf[2000];
	va_start(va, str);
	vsprintf(buf,str,va);
	dprintf(32,("tclexec'ing:%s",buf));
	result = Tcl_Eval( interp, buf, 0, (char **)NULL);
	va_end(str);
	if (result != TCL_OK)
	{	printf("error evaluating %s\n", buf);
		fprintf(stderr, "%s\n", interp->result);
		exit(-1);
	}
	return(result);
}

static void str2lower(char *str)
{
	while (*str)
	{	*str = tolower(*str);
		str++;
	}
}

static char *_handles[300];  
static int _handInd=0;

static char* getStrHandle(int hndl)
{	static char buf[20];
	if((hndl<_handInd) && (hndl>=0))
		return(_handles[hndl]);
	sprintf(buf, "%d", hndl);
	return(buf);
}

static int findHandle(char* str)
{	int i;
	for (i=0; i<_handInd; i++)
		if (!strcmp(str,_handles[i]))
			return(i);
	return(-1);
}

typedef enum {enum_win, enum_frame, enum_canvas, enum_button, enum_menu} class_enum;

static int allocStrHandle(char *seed, class_enum class)
{
	static char *classes[]= {"win", "frame", "canvas", "button", "menu"};
	static int classInds[10] = { 0, 0, 0, 0, 0, 0};
	char buf[200];
	assert((class>=0) && (class<=4));
	if (seed && *seed)
		sprintf(buf,"%s.%s%d", seed, classes[class], ++classInds[class]);
	else
		sprintf(buf,"%s%d", classes[class], ++classInds[class]);
	str2lower(buf);
	_handles[_handInd]=strdup(buf);
	return(_handInd++);
}

#if 0
static _WinMain(int argc, char *argv[])
{	int i; char buf[300];

	*buf = 0;
	for (i=1; i<argc; i++)
	{	if (*buf) strcat(buf," ");
		strcat(buf,argv[i]);
	}
	WinMain(getpid(),NULL,buf,SW_SHOWNORMAL); /* or SW_SHOWMINNOACTIVE*/
	exit(0);
}
#endif

static int _WinCallBack( ClientData  clientData, Tcl_Interp *interp,
	int         argc, char      **argv)
{
	int i;
	if (argc>2)
	{	if (!strcmp(argv[1],"menu"))
		{
#if 0
			LpWndClass->lpfnWndProc(findHandle(argv[2]),
				WM_COMMAND,atoi(argv[4]),0); 
#else
			CALLWNDPROC(LpWndClass->lpfnWndProc, 
				    findHandle(argv[2]),
				    WM_COMMAND,
				    atoi(argv[4]),
				    0);
#endif
		}
	}
/*	for (i=0; i<argc; i++)
		printf("%s\n", argv[i]);
	printf("\n"); */
	return(TCL_OK);
}

static int _getStrHandle( ClientData  clientData, Tcl_Interp *interp,
	int         argc, char      **argv)
{
	int i;
	if (argc != 2)
	{	sprintf(interp->result, "%s: invalid arg\n", argv[0]);
		return(TCL_ERROR);
	}
	i = atoi(argv[1]);
	strcpy(interp->result, getStrHandle(i));
	return(TCL_OK);
}

_MsMsg2XvMsg(HWND hwnd, char *event)
{
	WORD message, wParam = 0; LONG lParam = 0;
	if (1) message=WM_PAINT; 
#if 0
	LpWndClass->lpfnWndProc(hwnd,message,wParam,lParam); 
#else
	CALLWNDPROC(LpWndClass->lpfnWndProc, hwnd, message, wParam, lParam);
#endif
}

static short *closed_bits;

HWND  CreateWindow(LPSTR szAppName, LPSTR Label, DWORD ol, int x, int y, int w, int h, HWND d, HMENU e, HANDLE i, LPSTR g)
{	int result, n;
	static int called=0;
	if (x == CW_USEDEFAULT) x=0;
	if (y == CW_USEDEFAULT) y=0;
	if (w == CW_USEDEFAULT) w=500;
	if (h == CW_USEDEFAULT) h=400;
	n=allocStrHandle("",enum_win);
	tclexec( "CreateWindow %s \"%s\" %d %d %d %d", getStrHandle(n), Label, x, y, w, h);
	called=1;
	return((HWND)n);
}

int  DrawText(HDC a, LPSTR str, int c, LPRECT d, WORD flag)
{	int x=d->left, y=d->top, w = d->right, h=d->bottom;
	if (flag&DT_SINGLELINE);
	if (flag&DT_CENTER);
		x=200;
	if (flag&DT_VCENTER);
		y=200;
	/*tclexec(".%s create text 200 200 -text \"%s\" -anchor n\n",getStrHandle(a), str); */
	tclexec("DrawText %s \"%s\" %d %d %d %d\n",getStrHandle(a), str, 
		y,x,h,w);
}

BOOL GetMessage(LPMSG msg,HWND b,WORD c, WORD d)
{	static int called=0;
	if (!called)
		_MsMsg2XvMsg(b, 0);
	called=1;
	return(1);
}

long DispatchMessage(MSG *msg)
{	
	if (tk_NumMainWindows > 0) {
		Tk_DoOneEvent(0); 
		return(0);
	}
	exit(0);
}

BOOL TranslateMessage(LPMSG a){}

void MyEventProc( ClientData clientData, XEvent *eventPtr)
{
}


BOOL  RegisterClass(LPWNDCLASS a) 
{
	/* Tk_CreateEventHandler(win,mask,proc,data); */
	LpWndClass = a; return(1); 
}

BOOL ShowWindow(HWND a, int b) 
{
	if (b != SW_SHOWNORMAL)
	{  assert(b==SW_SHOWMINNOACTIVE);  /* iconize */
	}
	return(TRUE);
}

void UpdateWindow(HWND a)
{
}

HDC  BeginPaint(HWND a, LPPAINTSTRUCT b) 
{	return(a); 
}
void EndPaint(HWND a, LPPAINTSTRUCT b) { }

void GetClientRect(HWND a, LPRECT b)
{	b->top = 0; b->left = 0;
	b->bottom = b->top+0;
	b->right = b->left+0;
}

HMENU CreateMenu(void)
{	static int n, called=0;
	int result;
	if (!called) 
	{	n=allocStrHandle("",enum_frame);
		tclexec( "CreateMenuBar %s\n",getStrHandle(n));
	}
	else
	{	n=allocStrHandle("",enum_menu);
		tclexec( "CreateMenuEntry %s any 0\n",getStrHandle(n));
	}
	called=1;
	return(n);
}

BOOL  AppendMenu(HMENU a, WORD b, WORD c, LPSTR d)
{	char *buf; int dist,n;
	char *cmd = getStrHandle(a);
	if (d)
	{	char *t;
		buf = strdup(d);
		if (t=strchr(buf,'&'))
			strcpy(t,strchr(d,'&')+1);
		dist = t-buf;
	}
	tclexec("AppendMenu %s %d %s {%s} %d", cmd, b, getStrHandle(c),
		buf, dist); 
	return(1);
}

/*   Graphics Primitives */
BOOL  Rectangle(HDC a, int xLeft, int yTop, int xRight, int yBottom)
{
	XDrawRectangle(Tk_Display(w), Tk_WindowId(w), DefaultGCOfScreen(Tk_Screen(w)),
		xLeft, yTop, xRight-xLeft+1, yBottom-yTop+1);
}

int FillRect(HDC a,LPRECT b,HBRUSH c)
{
	XFillRectangle(Tk_Display(w), Tk_WindowId(w), DefaultGCOfScreen(Tk_Screen(w)),
		b->left, b->top, b->right-b->left+1, b->bottom-b->top+1);
}

static int _LineX=0, _LineY=0;

int LineTo(HDC a, int b, int c)
{
	XDrawLine(Tk_Display(w), Tk_WindowId(w), DefaultGCOfScreen(Tk_Screen(w)),
		_LineX,b, _LineY,c);
	_LineX=b;  _LineY=c;
}

int MoveTo(HDC a, int b, int c) { _LineX=b;  _LineY=c; }

BOOL Arc(HDC a, int xLeft, int yTop, int xRight, int yBottom, 
	int xStart, int yStart, int xEnd, int yEnd)
{	int w, h, x, y;
/*	XDrawArc(Tk_Display(w), Tk_WindowId(w), DefaultGCOfScreen(Tk_Screen(w)), x,y,..);*/
}

BOOL Pie(HDC a, int xLeft, int yTop, int xRight, int yBottom, 
	int xStart, int yStart, int xEnd, int yEnd)
{	int w, h, x,y;
}

BOOL Chord(HDC a, int xLeft, int yTop, int xRight, int yBottom, 
	int xStart, int yStart, int xEnd, int yEnd)
{	int w, h, x,y;
}

static int dte(char *str,...)
{       char cmd[300], *ptr, *ptr2;
	int n, i;
	va_list va;
	va_start(va, str);
        strcpy(cmd, str);
	n = va_arg(va,int);
	for (i=0; i<n; i++)
	{	ptr = va_arg(va,char *);
		ptr2 = va_arg(va,char *);
		if (!strncmp("LPSTR",ptr,5))
			sprintf(cmd+strlen(cmd)," \"%s\"", (ptr2?ptr2:""));
		else if (!strncmp("char",ptr,4))
			sprintf(cmd+strlen(cmd)," %c\0", ptr2);
		else if (!strncmp("HANDLE",ptr,6))
			sprintf(cmd+strlen(cmd)," %s", getStrHandle((int)ptr2));
		else
			sprintf(cmd+strlen(cmd)," %d", ptr2);
	}
	strcat(cmd,"\n");
	va_end(va);
	tclexec(cmd);
}

int wsprintf(LPSTR a,LPSTR b,...) {}

BOOL  IsCharAlpha(char ch) { return(isalpha(ch)); }
BOOL  IsCharAlphaNumeric(char ch) { return(isalnum(ch)); }
BOOL  IsCharUpper(char ch) { return(isupper(ch)); }
BOOL  IsCharLower(char ch) { return(islower(ch)); }
int	 lstrcmp( LPSTR a, LPSTR b ) { return(strcmp(a,b)); }
int	 lstrcmpi( LPSTR a, LPSTR b ) { return(strcasecmp(a,b)); }
LPSTR	lstrcpy( LPSTR a, LPSTR b ) { return(strcpy(a,b)); }
LPSTR	lstrcat( LPSTR a, LPSTR b ) { return(strcat(a,b)); }
int	 lstrlen( LPSTR a) { return(strlen(a)); }
int	 _lopen( LPSTR a, int b) { return(open(a,b)); }
int	 _lclose( int a) { return(close(a)); }
int	 _lcreat( LPSTR a, int b) { return(creat(a,b)); }
LONG	_llseek( int a, long b, int c) { return(lseek(a,b,c)); }
WORD	_lread( int a, LPSTR b, int c) { return(read(a,b,c)); }
WORD	_lwrite( int a, LPSTR b, int c) { return(write(a,b,c)); }
BOOL  ExitWindows(DWORD dwReserved, WORD wReturnCode) {exit(0); }

