/*
 * Edit control
 *
 * Copyright  David W. Metcalfe, 1994
 *
 * Release 3, July 1994

static char Copyright[] = "Copyright  David W. Metcalfe, 1994";
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <heap.h>
#include "win.h"
#include "class.h"
#include "user.h"
#include "stddebug.h"
/* #define DEBUG_EDIT */
/* #undef  DEBUG_EDIT */
#include "debug.h"


#define NOTIFY_PARENT(hWndCntrl, wNotifyCode) \
	SendMessage(GetParent(hWndCntrl), WM_COMMAND, \
		 GetDlgCtrlID(hWndCntrl), MAKELPARAM(hWndCntrl, wNotifyCode));

#define MAXTEXTLEN 30000   /* maximum text buffer length */
#define EDITLEN     1024   /* starting length for multi-line control */
#define ENTRYLEN     256   /* starting length for single line control */
#define GROWLENGTH    64   /* buffers grow by this much */

#define HSCROLLDIM (ClientWidth(wndPtr) / 3)
                           /* "line" dimension for horizontal scroll */

typedef struct
{
    int wlines;              /* number of lines of text */
    int wtop;                /* top line that is displayed */
    int wleft;               /* left pixel that is displayed */
    unsigned int textlen;    /* text buffer length */
    int textwidth;           /* width of longest line in pixels */
    RECT fmtrc;              /* rectangle in which to format text */
    int txtht;               /* height of text line in pixels */
    HANDLE hText;            /* handle to text buffer */
    HANDLE hCharWidths;      /* widths of chars in font */
    HANDLE hTextPtrs;        /* list of line offsets */
    HANDLE hBlankLine;       /* to fill blank lines quickly */
    int CurrCol;             /* current column */
    int CurrLine;            /* current line */
    int WndCol;              /* current window column */
    int WndRow;              /* current window row */
    BOOL TextChanged;        /* TRUE if text has changed */
    BOOL PaintBkgd;          /* paint control background */
    unsigned int MaxTextLen; /* maximum text buffer length */
    int SelBegLine;          /* beginning line of selection */
    int SelBegCol;           /* beginning column of selection */
    int SelEndLine;          /* ending line of selection */
    int SelEndCol;           /* ending column of selection */
    HFONT hFont;             /* handle of current font (if not default) */
    HANDLE hDeletedText;     /* handle to deleted txet buffer for undo */
    int DeletedLength;       /* length of deleted text */
    int DeletedCurrLine;     /* starting line from which text was deleted */
    int DeletedCurrCol;      /* starting col from which text was deleted */
    int NumTabStops;         /* number of tab stops in buffer hTabStops */
    HANDLE hTabStops;        /* handle of tab stops buffer */
} EDITSTATE;


#define ClientWidth(wndPtr) \
	(wndPtr->rectClient.right > wndPtr->rectClient.left ? \
	wndPtr->rectClient.right - wndPtr->rectClient.left : 0)
#define ClientHeight(wndPtr, es) \
	(wndPtr->rectClient.bottom > wndPtr->rectClient.top ? \
	(wndPtr->rectClient.bottom - wndPtr->rectClient.top) / es->txtht : 0)
#define EditBufLen(wndPtr) (wndPtr->dwStyle & ES_MULTILINE \
			    ? EDITLEN : ENTRYLEN)
#define CurrChar (EDIT_TextLine(hwnd, es->CurrLine) + es->CurrCol)
#define SelMarked(es) (es->SelBegLine != 0 || es->SelBegCol != 0 || \
		       es->SelEndLine != 0 || es->SelEndCol != 0)
#define ROUNDUP(numer, denom) (((numer) % (denom)) \
			       ? ((((numer) + (denom)) / (denom)) * (denom)) \
			       : (numer) + (denom))

/* macros to access window styles */
#define IsAutoVScroll() (wndPtr->dwStyle & ES_AUTOVSCROLL)
#define IsAutoHScroll() (wndPtr->dwStyle & ES_AUTOHSCROLL)
#define IsMultiLine() (wndPtr->dwStyle & ES_MULTILINE)
#define IsVScrollBar() (wndPtr->dwStyle & WS_VSCROLL)
#define IsHScrollBar() (wndPtr->dwStyle & WS_HSCROLL)

/* internal variables */
static BOOL TextMarking;         /* TRUE if text marking in progress */
static BOOL ButtonDown;          /* TRUE if left mouse button down */
static int ButtonRow;              /* row in text buffer when button pressed */
static int ButtonCol;              /* col in text buffer when button pressed */


LONG EditWndProc(HWND hWnd, WORD uMsg, WORD wParam, LONG lParam);
long EDIT_NCCreateMsg(HWND hwnd, LONG lParam);
long EDIT_CreateMsg(HWND hwnd, LONG lParam);
void EDIT_ClearTextPointers(HWND hwnd);
void EDIT_BuildTextPointers(HWND hwnd);
void EDIT_ModTextPointers(HWND hwnd, int lineno, int var);
void EDIT_PaintMsg(HWND hwnd);
HANDLE EDIT_GetTextLine(HWND hwnd, int selection);
char *EDIT_TextLine(HWND hwnd, int sel);
int EDIT_StrLength(HWND hwnd, unsigned char *str, int len, int pcol);
int EDIT_LineLength(HWND hwnd, int num);
void EDIT_WriteTextLine(HWND hwnd, RECT *rc, int y);
void EDIT_WriteText(HWND hwnd, char *lp, int off, int len, int row, 
		    int col, RECT *rc, BOOL blank, BOOL reverse);
HANDLE EDIT_GetStr(HWND hwnd, char *lp, int off, int len, int *diff);
void EDIT_CharMsg(HWND hwnd, WORD wParam);
void EDIT_KeyTyped(HWND hwnd, short ch);
int EDIT_CharWidth(HWND hwnd, short ch, int pcol);
int EDIT_GetNextTabStop(HWND hwnd, int pcol);
void EDIT_Forward(HWND hwnd);
void EDIT_Downward(HWND hwnd);
void EDIT_Upward(HWND hwnd);
void EDIT_Backward(HWND hwnd);
void EDIT_End(HWND hwnd);
void EDIT_Home(HWND hwnd);
void EDIT_StickEnd(HWND hwnd);
void EDIT_KeyDownMsg(HWND hwnd, WORD wParam);
void EDIT_KeyHScroll(HWND hwnd, WORD opt);
void EDIT_KeyVScrollLine(HWND hwnd, WORD opt);
void EDIT_KeyVScrollPage(HWND hwnd, WORD opt);
void EDIT_KeyVScrollDoc(HWND hwnd, WORD opt);
int EDIT_ComputeVScrollPos(HWND hwnd);
int EDIT_ComputeHScrollPos(HWND hwnd);
void EDIT_DelKey(HWND hwnd);
void EDIT_VScrollMsg(HWND hwnd, WORD wParam, LONG lParam);
void EDIT_VScrollLine(HWND hwnd, WORD opt);
void EDIT_VScrollPage(HWND hwnd, WORD opt);
void EDIT_HScrollMsg(HWND hwnd, WORD wParam, LONG lParam);
void EDIT_SizeMsg(HWND hwnd, WORD wParam, LONG lParam);
void EDIT_LButtonDownMsg(HWND hwnd, WORD wParam, LONG lParam);
void EDIT_MouseMoveMsg(HWND hwnd, WORD wParam, LONG lParam);
int EDIT_PixelToChar(HWND hwnd, int row, int *pixel);
LONG EDIT_SetTextMsg(HWND hwnd, LONG lParam);
void EDIT_ClearText(HWND hwnd);
void EDIT_SetSelMsg(HWND hwnd, WORD wParam, LONG lParam);
void EDIT_GetLineCol(HWND hwnd, int off, int *line, int *col);
void EDIT_DeleteSel(HWND hwnd);
void EDIT_ClearSel(HWND hwnd);
int EDIT_TextLineNumber(HWND hwnd, char *lp);
void EDIT_SetAnchor(HWND hwnd, int row, int col);
void EDIT_ExtendSel(HWND hwnd, int x, int y);
void EDIT_WriteSel(HWND hwnd, int y, int start, int end);
void EDIT_StopMarking(HWND hwnd);
LONG EDIT_GetLineMsg(HWND hwnd, WORD wParam, LONG lParam);
LONG EDIT_GetSelMsg(HWND hwnd);
void EDIT_ReplaceSel(HWND hwnd, LONG lParam);
void EDIT_InsertText(HWND hwnd, char *str, int len);
LONG EDIT_LineFromCharMsg(HWND hwnd, WORD wParam);
LONG EDIT_LineIndexMsg(HWND hwnd, WORD wParam);
LONG EDIT_LineLengthMsg(HWND hwnd, WORD wParam);
void EDIT_SetFont(HWND hwnd, WORD wParam, LONG lParam);
void EDIT_SaveDeletedText(HWND hwnd, char *deltext, int len, int line, 
			  int col);
void EDIT_ClearDeletedText(HWND hwnd);
LONG EDIT_UndoMsg(HWND hwnd);
unsigned int EDIT_HeapAlloc(HWND hwnd, int bytes);
void *EDIT_HeapAddr(HWND hwnd, unsigned int handle);
unsigned int EDIT_HeapReAlloc(HWND hwnd, unsigned int handle, int bytes);
void EDIT_HeapFree(HWND hwnd, unsigned int handle);
unsigned int EDIT_HeapSize(HWND hwnd, unsigned int handle);
void EDIT_SetHandleMsg(HWND hwnd, WORD wParam);
LONG EDIT_SetTabStopsMsg(HWND hwnd, WORD wParam, LONG lParam);
void EDIT_CopyToClipboard(HWND hwnd);
void EDIT_PasteMsg(HWND hwnd);
void swap(int *a, int *b);


LONG EditWndProc(HWND hwnd, WORD uMsg, WORD wParam, LONG lParam)
{
    LONG lResult = 0L;
    char *textPtr;
    int len;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));

    switch (uMsg) {
    case EM_CANUNDO:
	lResult = es->hDeletedText;
	break;
	
    case EM_EMPTYUNDOBUFFER:
	EDIT_ClearDeletedText(hwnd);
	break;

    case EM_FMTLINES:
	fprintf(stdnimp,"edit: EM_FMTLINES message received\n");
	if (!wParam)
	    lResult = 1L;
	else
	    lResult = 0L;
	break;

    case EM_GETFIRSTVISIBLELINE:
	lResult = es->wtop;
	break;

    case EM_GETHANDLE:
	lResult = es->hText;
	break;

    case EM_GETLINE:
	if (IsMultiLine())
	    lResult = EDIT_GetLineMsg(hwnd, wParam, lParam);
	else
	    lResult = 0L;
	break;

    case EM_GETLINECOUNT:
	if (IsMultiLine())
	    lResult = es->wlines;
	else
	    lResult = 0L;
	break;

    case EM_GETMODIFY:
	lResult = es->TextChanged;
	break;

    case EM_GETPASSWORDCHAR:
	fprintf(stdnimp,"edit: cannot process EM_GETPASSWORDCHAR message\n");
	break;

    case EM_GETRECT:
	GetWindowRect(hwnd, (LPRECT)lParam);
	break;

    case EM_GETSEL:
	lResult = EDIT_GetSelMsg(hwnd);
	break;

    case EM_GETWORDBREAKPROC:
	fprintf(stdnimp,"edit: cannot process EM_GETWORDBREAKPROC message\n");
	break;

    case EM_LIMITTEXT:
	if (wParam)
	    es->MaxTextLen = wParam;
	else if (IsMultiLine())
	    es->MaxTextLen = 65535;
	else
	    es->MaxTextLen = 32767;
	break;

    case EM_LINEFROMCHAR:
	lResult = EDIT_LineFromCharMsg(hwnd, wParam);
	break;

    case EM_LINEINDEX:
	if (IsMultiLine())
	    lResult = EDIT_LineIndexMsg(hwnd, wParam);
	else
	    lResult = 0L;
	break;

    case EM_LINELENGTH:
	lResult = EDIT_LineLengthMsg(hwnd, wParam);
	break;

    case EM_LINESCROLL:
	fprintf(stdnimp,"edit: cannot process EM_LINESCROLL message\n");
	break;

    case EM_REPLACESEL:
	HideCaret(hwnd);
	EDIT_ReplaceSel(hwnd, lParam);
	SetCaretPos(es->WndCol, es->WndRow * es->txtht);
	ShowCaret(hwnd);
	break;

    case EM_SETHANDLE:
	HideCaret(hwnd);
	EDIT_SetHandleMsg(hwnd, wParam);
	SetCaretPos(es->WndCol, es->WndRow * es->txtht);
	ShowCaret(hwnd);
	break;

    case EM_SETMODIFY:
	es->TextChanged = wParam;
	break;

    case EM_SETPASSWORDCHAR:
	fprintf(stdnimp,"edit: cannot process EM_SETPASSWORDCHAR message\n");
	break;

    case EM_SETREADONLY:
	fprintf(stdnimp,"edit: cannot process EM_SETREADONLY message\n");
	break;

    case EM_SETRECT:
    case EM_SETRECTNP:
	fprintf(stdnimp,"edit: cannot process EM_SETRECT(NP) message\n");
	break;

    case EM_SETSEL:
	HideCaret(hwnd);
	EDIT_SetSelMsg(hwnd, wParam, lParam);
	SetCaretPos(es->WndCol, es->WndRow * es->txtht);
	ShowCaret(hwnd);
	break;

    case EM_SETTABSTOPS:
	lResult = EDIT_SetTabStopsMsg(hwnd, wParam, lParam);
	break;

    case EM_SETWORDBREAKPROC:
	fprintf(stdnimp,"edit: cannot process EM_SETWORDBREAKPROC message\n");
	break;

    case EM_UNDO:
	HideCaret(hwnd);
	lResult = EDIT_UndoMsg(hwnd);
	SetCaretPos(es->WndCol, es->WndRow * es->txtht);
	ShowCaret(hwnd);
	break;

    case WM_GETDLGCODE:
        return DLGC_HASSETSEL | DLGC_WANTCHARS | DLGC_WANTARROWS;

    case WM_CHAR:
	EDIT_CharMsg(hwnd, wParam);
	break;

    case WM_COPY:
	EDIT_CopyToClipboard(hwnd);
	EDIT_ClearSel(hwnd);
	break;

    case WM_CREATE:
	lResult = EDIT_CreateMsg(hwnd, lParam);
	break;

    case WM_CUT:
	EDIT_CopyToClipboard(hwnd);
	EDIT_DeleteSel(hwnd);
	break;

    case WM_DESTROY:
	EDIT_HeapFree(hwnd, es->hTextPtrs);
	EDIT_HeapFree(hwnd, es->hCharWidths);
	EDIT_HeapFree(hwnd, es->hText);
	EDIT_HeapFree(hwnd, (HANDLE)(*(wndPtr->wExtra)));
	break;

    case WM_ENABLE:
	InvalidateRect(hwnd, NULL, FALSE);
	break;

    case WM_GETTEXT:
	textPtr = EDIT_HeapAddr(hwnd, es->hText);
	if ((int)wParam > (len = strlen(textPtr)))
	{
	    strcpy((char *)lParam, textPtr);
	    lResult = (DWORD)len ;
	}
	else
	    lResult = 0L;
	break;

    case WM_GETTEXTLENGTH:
	textPtr = EDIT_HeapAddr(hwnd, es->hText);
	lResult = (DWORD)strlen(textPtr);
	break;

    case WM_HSCROLL:
	EDIT_HScrollMsg(hwnd, wParam, lParam);
	break;

    case WM_KEYDOWN:
	EDIT_KeyDownMsg(hwnd, wParam);
	break;

    case WM_KILLFOCUS:
	DestroyCaret();
	NOTIFY_PARENT(hwnd, EN_KILLFOCUS);
	break;

    case WM_LBUTTONDOWN:
	HideCaret(hwnd);
	SetFocus(hwnd);
	EDIT_LButtonDownMsg(hwnd, wParam, lParam);
	SetCaretPos(es->WndCol, es->WndRow * es->txtht);
	ShowCaret(hwnd);
	break;

    case WM_LBUTTONUP:
	ButtonDown = FALSE;
	if (TextMarking)
	    EDIT_StopMarking(hwnd);
	break;

    case WM_MOUSEMOVE:
	HideCaret(hwnd);
	EDIT_MouseMoveMsg(hwnd, wParam, lParam);
	SetCaretPos(es->WndCol, es->WndRow * es->txtht);
	ShowCaret(hwnd);
	break;

    case WM_MOVE:
	lResult = 0;
	break;

    case WM_NCCREATE:
	lResult = EDIT_NCCreateMsg(hwnd, lParam);
	break;
	
    case WM_PAINT:
	EDIT_PaintMsg(hwnd);
	break;

    case WM_PASTE:
	EDIT_PasteMsg(hwnd);
	break;

    case WM_SETFOCUS:
	CreateCaret(hwnd, 0, 2, es->txtht);
	SetCaretPos(es->WndCol, es->WndRow * es->txtht);
	ShowCaret(hwnd);
	NOTIFY_PARENT(hwnd, EN_SETFOCUS);
	break;

    case WM_SETFONT:
	HideCaret(hwnd);
	EDIT_SetFont(hwnd, wParam, lParam);
	SetCaretPos(es->WndCol, es->WndRow * es->txtht);
	ShowCaret(hwnd);
	break;
#if 0
    case WM_SETREDRAW:
	dprintf_edit(stddeb, "WM_SETREDRAW: hwnd=%d, wParam=%x\n",
		     hwnd, wParam);
	lResult = 0;
	break;
#endif
    case WM_SETTEXT:
	EDIT_SetTextMsg(hwnd, lParam);
	break;

    case WM_SIZE:
	EDIT_SizeMsg(hwnd, wParam, lParam);
	lResult = 0;
	break;

    case WM_VSCROLL:
	EDIT_VScrollMsg(hwnd, wParam, lParam);
	break;

    default:
	lResult = DefWindowProc(hwnd, uMsg, wParam, lParam);
	break;
    }

    return lResult;
}


/*********************************************************************
 *  WM_NCCREATE message function
 */

long EDIT_NCCreateMsg(HWND hwnd, LONG lParam)
{
    CREATESTRUCT *createStruct = (CREATESTRUCT *)lParam;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es;
    unsigned int *textPtrs;
    char *text;

    /* store pointer to local or global heap in window structure so that */
    /* EDITSTATE structure itself can be stored on local heap  */
    if (HEAP_LocalFindHeap(createStruct->hInstance)!=NULL)
      (MDESC **)*(LONG *)(wndPtr->wExtra + 2) = 
	&HEAP_LocalFindHeap(createStruct->hInstance)->free_list;
    else
      {
	(MDESC **)*(LONG *)(wndPtr->wExtra + 2) =
	  GlobalLock(createStruct->hInstance);
	/* GlobalUnlock(createStruct->hInstance); */
      }
    /* allocate space for state variable structure */
    (HANDLE)(*(wndPtr->wExtra)) = EDIT_HeapAlloc(hwnd, sizeof(EDITSTATE));
    es = (EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));
    es->hTextPtrs = EDIT_HeapAlloc(hwnd, sizeof(int));
    textPtrs = (unsigned int *)EDIT_HeapAddr(hwnd, es->hTextPtrs);
    es->hCharWidths = EDIT_HeapAlloc(hwnd, 256 * sizeof(short));

    /* --- text buffer */
    es->MaxTextLen = MAXTEXTLEN + 1;
    if (!(createStruct->lpszName))
    {
	es->textlen = EditBufLen(wndPtr) + 1;
	es->hText = EDIT_HeapAlloc(hwnd, EditBufLen(wndPtr) + 2);
	text = EDIT_HeapAddr(hwnd, es->hText);
	memset(text, 0, es->textlen + 2);
	EDIT_ClearTextPointers(hwnd);
    }
    else
    {
	if (strlen(createStruct->lpszName) < EditBufLen(wndPtr))
	{
	    es->textlen = EditBufLen(wndPtr) + 1;
	    es->hText = EDIT_HeapAlloc(hwnd, EditBufLen(wndPtr) + 2);
	    text = EDIT_HeapAddr(hwnd, es->hText);
	    strcpy(text, createStruct->lpszName);
	    *(text + es->textlen) = '\0';
	}
	else
	{
	    es->hText = EDIT_HeapAlloc(hwnd, 
				       strlen(createStruct->lpszName) + 2);
	    text = EDIT_HeapAddr(hwnd, es->hText);
	    strcpy(text, createStruct->lpszName);
	    es->textlen = strlen(createStruct->lpszName) + 1;
	}
	*(text + es->textlen + 1) = '\0';
	EDIT_BuildTextPointers(hwnd);
    }

    /* ES_AUTOVSCROLL and ES_AUTOHSCROLL are automatically applied if */
    /* the corresponding WS_* style is set                            */
    if (createStruct->style & WS_VSCROLL)
	wndPtr->dwStyle |= ES_AUTOVSCROLL;
    if (createStruct->style & WS_HSCROLL)
	wndPtr->dwStyle |= ES_AUTOHSCROLL;

    /* remove the WS_CAPTION style if it has been set - this is really a  */
    /* pseudo option made from a combination of WS_BORDER and WS_DLGFRAME */
    if (wndPtr->dwStyle & WS_BORDER && wndPtr->dwStyle & WS_DLGFRAME)
	wndPtr->dwStyle ^= WS_DLGFRAME;

    return 1;
}


/*********************************************************************
 *  WM_CREATE message function
 */

long EDIT_CreateMsg(HWND hwnd, LONG lParam)
{
    HDC hdc;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));
    CLASS *classPtr;
    short *charWidths;
    TEXTMETRIC tm;
    char *text;

    /* initialize state variable structure */
    hdc = GetDC(hwnd);

    /* --- char width array                                        */
    /*     only initialise chars <= 32 as X returns strange widths */
    /*     for other chars                                         */
    charWidths = (short *)EDIT_HeapAddr(hwnd, es->hCharWidths);
    memset(charWidths, 0, 256 * sizeof(short));
    GetCharWidth(hdc, 32, 254, &charWidths[32]);

    /* --- other structure variables */
    GetTextMetrics(hdc, &tm);
    es->txtht = tm.tmHeight + tm.tmExternalLeading;
    es->wlines = 0;
    es->wtop = es->wleft = 0;
    es->CurrCol = es->CurrLine = 0;
    es->WndCol = es->WndRow = 0;
    es->TextChanged = FALSE;
    es->textwidth = 0;
    es->SelBegLine = es->SelBegCol = 0;
    es->SelEndLine = es->SelEndCol = 0;
    es->hFont = 0;
    es->hDeletedText = 0;
    es->DeletedLength = 0;
    es->NumTabStops = 0;
    es->hTabStops = EDIT_HeapAlloc(hwnd, sizeof(int));

    /* allocate space for a line full of blanks to speed up */
    /* line filling */
    es->hBlankLine = EDIT_HeapAlloc(hwnd, (ClientWidth(wndPtr) / 
				      charWidths[32]) + 2); 
    text = EDIT_HeapAddr(hwnd, es->hBlankLine);
    memset(text, ' ', (ClientWidth(wndPtr) / charWidths[32]) + 2);

    /* set up text cursor for edit class */
    CLASS_FindClassByName("EDIT", 0, &classPtr);
    classPtr->wc.hCursor = LoadCursor(0, IDC_IBEAM);

    /* paint background on first WM_PAINT */
    es->PaintBkgd = TRUE;

    ReleaseDC(hwnd, hdc);
    return 0L;
}


/*********************************************************************
 *  EDIT_ClearTextPointers
 *
 *  Clear and initialize text line pointer array.
 */

void EDIT_ClearTextPointers(HWND hwnd)
{
    unsigned int *textPtrs;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));
    
    es->hTextPtrs = EDIT_HeapReAlloc(hwnd, es->hTextPtrs, sizeof(int));
    textPtrs = (unsigned int *)EDIT_HeapAddr(hwnd, es->hTextPtrs);
    *textPtrs = 0;
}


/*********************************************************************
 *  EDIT_BuildTextPointers
 *
 *  Build array of pointers to text lines.
 */

#define INITLINES 100

void EDIT_BuildTextPointers(HWND hwnd)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    char *text, *cp;
    int incrs = INITLINES;
    unsigned int off, len;
    EDITSTATE *es;
    unsigned int *textPtrs;
    short *charWidths;

    es = (EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));
    text = EDIT_HeapAddr(hwnd, es->hText);
    textPtrs = (unsigned int *)EDIT_HeapAddr(hwnd, es->hTextPtrs);
    charWidths = (short *)EDIT_HeapAddr(hwnd, es->hCharWidths);

    es->textwidth = es->wlines = 0;
    cp = text;

    /* advance through text buffer */
    while (*cp)
    {
	/* increase size of text pointer array */ 
	if (incrs == INITLINES)
	{
	    incrs = 0;
	    es->hTextPtrs = EDIT_HeapReAlloc(hwnd, es->hTextPtrs,
			      (es->wlines + INITLINES) * sizeof(int));
	    textPtrs = (unsigned int *)EDIT_HeapAddr(hwnd, es->hTextPtrs);
	}
	off = (unsigned int)(cp - text);     /* offset of beginning of line */
	*(textPtrs + es->wlines) = off;
	es->wlines++;
	incrs++;
	len = 0;
	
	/* advance through current line */
	while (*cp && *cp != '\n')
	{
	    len += EDIT_CharWidth(hwnd, (BYTE)*cp, len);
	                                     /* width of line in pixels */
	    cp++;
	}
	es->textwidth = max(es->textwidth, len);
	if (*cp)
	    cp++;                            /* skip '\n' */
    }

    off = (unsigned int)(cp - text);
    *(textPtrs + es->wlines) = off;
}


/*********************************************************************
 *  EDIT_ModTextPointers
 *
 *  Modify text pointers from a specified position.
 */

void EDIT_ModTextPointers(HWND hwnd, int lineno, int var)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));
    unsigned int *textPtrs = 
	(unsigned int *)EDIT_HeapAddr(hwnd, es->hTextPtrs);

    while (lineno < es->wlines)
	*(textPtrs + lineno++) += var;
}


/*********************************************************************
 *  WM_PAINT message function
 */

void EDIT_PaintMsg(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc;
    int y;
    RECT rc;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));

    hdc = BeginPaint(hwnd, &ps);
    rc = ps.rcPaint;

    dprintf_edit(stddeb,"WM_PAINT: rc=(%d,%d), (%d,%d)\n", rc.left, rc.top, 
	   rc.right, rc.bottom);

    if (es->PaintBkgd)
	FillWindow(GetParent(hwnd), hwnd, hdc, CTLCOLOR_EDIT);

    for (y = (rc.top / es->txtht); y <= (rc.bottom / es->txtht); y++)
    {
	if (y < es->wlines - es->wtop)
	    EDIT_WriteTextLine(hwnd, &rc, y + es->wtop);
    }

    EndPaint(hwnd, &ps);
}


/*********************************************************************
 *  EDIT_GetTextLine
 *
 *  Get a copy of the text in the specified line.
 */

HANDLE EDIT_GetTextLine(HWND hwnd, int selection)
{
    char *line;
    HANDLE hLine;
    int len = 0;
    char *cp, *cp1;

    dprintf_edit(stddeb,"GetTextLine %d\n", selection);
    cp = cp1 = EDIT_TextLine(hwnd, selection);
    /* advance through line */
    while (*cp && *cp != '\r')
    {
	len++;
	cp++;
    }

    /* store selected line and return handle */
    hLine = EDIT_HeapAlloc(hwnd, len + 6);
    line = (char *)EDIT_HeapAddr(hwnd, hLine);
    memmove(line, cp1, len);
    line[len] = '\0';
    return hLine;
}


/*********************************************************************
 *  EDIT_TextLine
 *
 *  Return a pointer to the text in the specified line.
 */

char *EDIT_TextLine(HWND hwnd, int sel)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));
    char *text = EDIT_HeapAddr(hwnd, es->hText);
    unsigned int *textPtrs = 
	(unsigned int *)EDIT_HeapAddr(hwnd, es->hTextPtrs);

    if(sel>es->wlines)return NULL;
    return (text + *(textPtrs + sel));
}
    

/*********************************************************************
 *  EDIT_StrLength
 *
 *  Return length of string _str_ of length _len_ characters in pixels.
 *  The current column offset in pixels _pcol_ is required to calculate 
 *  the width of a tab.
 */

int EDIT_StrLength(HWND hwnd, unsigned char *str, int len, int pcol)
{
    int i, plen = 0;

    for (i = 0; i < len; i++)
	plen += EDIT_CharWidth(hwnd, (BYTE)(*(str + i)), pcol + plen);

    dprintf_edit(stddeb,"EDIT_StrLength: returning %d\n", plen);
    return plen;
}


/*********************************************************************
 *  EDIT_LineLength
 *
 *  Return length of line _num_ in characters.
 */

int EDIT_LineLength(HWND hwnd, int num)
{
    char *cp = EDIT_TextLine(hwnd, num);
    char *cp1;

    if(!cp)return 0;
    cp1 = strchr(cp, '\r');
    return cp1 ? (int)(cp1 - cp) : strlen(cp);
}


/*********************************************************************
 *  EDIT_WriteTextLine
 *
 *  Write the line of text at offset _y_ in text buffer to a window.
 */

void EDIT_WriteTextLine(HWND hwnd, RECT *rect, int y)
{
    int len = 0;
    HANDLE hLine;
    unsigned char *lp;
    int lnlen, lnlen1;
    int col, off = 0;
    int sbl, sel, sbc, sec;
    RECT rc;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));

    /* initialize rectangle if NULL, else copy */
    if (rect)
	CopyRect(&rc, rect);
    else
	GetClientRect(hwnd, &rc);

    dprintf_edit(stddeb,"WriteTextLine %d\n", y);

    /* make sure y is inside the window */
    if (y < es->wtop || y > (es->wtop + ClientHeight(wndPtr, es)))
    {
	dprintf_edit(stddeb,"EDIT_WriteTextLine: y (%d) is not a displayed line\n", y);
	return;
    }

    /* make sure rectangle is within window */
    if (rc.left >= ClientWidth(wndPtr) - 1)
    {
	dprintf_edit(stddeb,"EDIT_WriteTextLine: rc.left (%d) is greater than right edge\n",
	       rc.left);
	return;
    }
    if (rc.right <= 0)
    {
	dprintf_edit(stddeb,"EDIT_WriteTextLine: rc.right (%d) is less than left edge\n",
	       rc.right);
	return;
    }
    if (y - es->wtop < (rc.top / es->txtht) || 
	y - es->wtop > (rc.bottom / es->txtht))
    {
	dprintf_edit(stddeb,"EDIT_WriteTextLine: y (%d) is outside window\n", y);
	return;
    }

    /* get the text and length of line */
    if ((hLine = EDIT_GetTextLine(hwnd, y)) == 0)
	return;
    lp = (unsigned char *)EDIT_HeapAddr(hwnd, hLine);
    lnlen = EDIT_StrLength(hwnd, lp, strlen(lp), 0);
    lnlen1 = lnlen;

    /* build the line to display */
    if (lnlen < es->wleft)
	lnlen = 0;
    else
	off += es->wleft;

    if (lnlen > rc.left)
    {
	off += rc.left;
	lnlen = lnlen1 - off;
	len = min(lnlen, rc.right - rc.left);
    }

    if (SelMarked(es))
    {
	sbl = es->SelBegLine;
	sel = es->SelEndLine;
	sbc = es->SelBegCol;
	sec = es->SelEndCol;

	/* put lowest marker first */
	if (sbl > sel)
	{
	    swap(&sbl, &sel);
	    swap(&sbc, &sec);
	}
	if (sbl == sel && sbc > sec)
	    swap(&sbc, &sec);

	if (y < sbl || y > sel)
	    EDIT_WriteText(hwnd, lp, off, len, y - es->wtop, rc.left, &rc,
			   TRUE, FALSE);
	else if (y > sbl && y < sel)
	    EDIT_WriteText(hwnd, lp, off, len, y - es->wtop, rc.left, &rc,
			   TRUE, TRUE);
	else if (y == sbl)
	{
	    col = EDIT_StrLength(hwnd, lp, sbc, 0);
	    if (col > (es->wleft + rc.left))
	    {
		len = min(col - off, rc.right - off);
		EDIT_WriteText(hwnd, lp, off, len, y - es->wtop, 
			       rc.left, &rc, FALSE, FALSE);
		off = col;
	    }
	    if (y == sel)
	    {
		col = EDIT_StrLength(hwnd, lp, sec, 0);
		if (col < (es->wleft + rc.right))
		{
		    len = min(col - off, rc.right - off);
		    EDIT_WriteText(hwnd, lp, off, len, y - es->wtop,
				   off - es->wleft, &rc, FALSE, TRUE);
		    off = col;
		    len = min(lnlen - off, rc.right - off);
		    EDIT_WriteText(hwnd, lp, off, len, y - es->wtop,
				   off - es->wleft, &rc, TRUE, FALSE);
		}
		else
		{
		    len = min(lnlen - off, rc.right - off);
		    EDIT_WriteText(hwnd, lp, off, len, y - es->wtop,
				   off - es->wleft, &rc, TRUE, TRUE);
		}
	    }
	    else
	    {
		len = min(lnlen - off, rc.right - off);
		if (col < (es->wleft + rc.right))
		    EDIT_WriteText(hwnd, lp, off, len, y - es->wtop,
				   off - es->wleft, &rc, TRUE, TRUE);
	    }
	}
	else if (y == sel)
	{
	    col = EDIT_StrLength(hwnd, lp, sec, 0);
	    if (col < (es->wleft + rc.right))
	    {
		len = min(col - off, rc.right - off);
		EDIT_WriteText(hwnd, lp, off, len, y - es->wtop,
			       off - es->wleft, &rc, FALSE, TRUE);
		off = col;
		len = min(lnlen - off, rc.right - off);
		EDIT_WriteText(hwnd, lp, off, len, y - es->wtop,
			       off - es->wleft, &rc, TRUE, FALSE);
	    }
	}
    }
    else
	EDIT_WriteText(hwnd, lp, off, len, y - es->wtop, rc.left, &rc,
		       TRUE, FALSE);
	      
    EDIT_HeapFree(hwnd, hLine);
}


/*********************************************************************
 *  EDIT_WriteText
 *
 *  Write text to a window
 *     lp  - text line
 *     off - offset in text line (in pixels)
 *     len - length from off (in pixels)
 *     row - line in window
 *     col - column in window
 *     rc  - rectangle in which to display line
 *     blank - blank remainder of line?
 *     reverse - reverse color of line?
 */

void EDIT_WriteText(HWND hwnd, char *lp, int off, int len, int row, 
		    int col, RECT *rc, BOOL blank, BOOL reverse)
{
    HDC hdc;
    HANDLE hStr;
    char *str, *cp, *cp1;
    int diff, num_spaces, tabwidth, scol;
    HRGN hrgnClip;
    COLORREF oldTextColor, oldBkgdColor;
    HFONT oldfont;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));
    short *charWidths = (short *)EDIT_HeapAddr(hwnd, es->hCharWidths);
    char *blanks = (char *)EDIT_HeapAddr(hwnd, es->hBlankLine);

    dprintf_edit(stddeb,"EDIT_WriteText lp=%s, off=%d, len=%d, row=%d, col=%d, reverse=%d\n", lp, off, len, row, col, reverse);

    hdc = GetDC(hwnd);
    hStr = EDIT_GetStr(hwnd, lp, off, len, &diff);
    str = (char *)EDIT_HeapAddr(hwnd, hStr);
    hrgnClip = CreateRectRgnIndirect(rc);
    SelectClipRgn(hdc, hrgnClip);

    if (es->hFont)
	oldfont = (HFONT)SelectObject(hdc, (HANDLE)es->hFont);

    SendMessage(GetParent(hwnd), WM_CTLCOLOR, (WORD)hdc,
		MAKELPARAM(hwnd, CTLCOLOR_EDIT));

    if (reverse)
    {
	oldBkgdColor = GetBkColor(hdc);
	oldTextColor = GetTextColor(hdc);
	SetBkColor(hdc, oldTextColor);
	SetTextColor(hdc, oldBkgdColor);
    }

    if (strlen(blanks) < (ClientWidth(wndPtr) / charWidths[32]) + 2)
    {
        es->hBlankLine = EDIT_HeapReAlloc(hwnd, es->hBlankLine,
             (ClientWidth(wndPtr) / charWidths[32]) + 2);
        blanks = EDIT_HeapAddr(hwnd, es->hBlankLine);
        memset(blanks, ' ', (ClientWidth(wndPtr) / charWidths[32]) + 2);
    }

    if (!(cp = strchr(str, VK_TAB)))
	TextOut(hdc, col - diff, row * es->txtht, str, strlen(str));
    else
    {
	TextOut(hdc, col - diff, row * es->txtht, str, (int)(cp - str));
	scol = EDIT_StrLength(hwnd, str, (int)(cp - str), 0);
	tabwidth = EDIT_CharWidth(hwnd, VK_TAB, scol);
	num_spaces = tabwidth / charWidths[32] + 1;
	TextOut(hdc, scol, row * es->txtht, blanks, num_spaces);
	cp++;
	scol += tabwidth;

	while ((cp1 = strchr(cp, VK_TAB)))
	{
	    TextOut(hdc, scol, row * es->txtht, cp, (int)(cp1 - cp));
	    scol += EDIT_StrLength(hwnd, cp, (int)(cp1 - cp), scol);
	    tabwidth = EDIT_CharWidth(hwnd, VK_TAB, scol);
	    num_spaces = tabwidth / charWidths[32] + 1;
	    TextOut(hdc, scol, row * es->txtht, blanks, num_spaces);
	    cp = ++cp1;
	    scol += tabwidth;
	}

	TextOut(hdc, scol, row * es->txtht, cp, strlen(cp));
    }

    if (reverse)
    {
	SetBkColor(hdc, oldBkgdColor);
	SetTextColor(hdc, oldTextColor);
    }

    /* blank out remainder of line if appropriate */
    if (blank)
    {
	if ((rc->right - col) > len)
	{
	    num_spaces = (rc->right - col - len) / charWidths[32];
	    TextOut(hdc, col + len, row * es->txtht, blanks, num_spaces);
	}
    }

    if (es->hFont)
	SelectObject(hdc, (HANDLE)oldfont);

    EDIT_HeapFree(hwnd, hStr);
    ReleaseDC(hwnd, hdc);
}


/*********************************************************************
 *  EDIT_GetStr
 *
 *  Return sub-string starting at pixel _off_ of length _len_ pixels.
 *  If _off_ is part way through a character, the negative offset of
 *  the beginning of the character is returned in _diff_, else _diff_ 
 *  will be zero.
 */

HANDLE EDIT_GetStr(HWND hwnd, char *lp, int off, int len, int *diff)
{
    HANDLE hStr;
    char *str;
    int ch = 0, i = 0, j, s_i=0;
    int ch1;

    dprintf_edit(stddeb,"EDIT_GetStr lp='%s'  off=%d  len=%d\n", lp, off, len);

    if (off < 0) off = 0;
    while (i < off)
    {
	s_i = i;
	i += EDIT_CharWidth(hwnd, (BYTE)(*(lp + ch)), i);
	ch++;
    }
    /* if stepped past _off_, go back a character */
    if (i - off)
    {
	i = s_i;
	ch--;
    }
    *diff = off - i;
    ch1 = ch;
    while (i < len + off)
    {
	if (*(lp + ch) == '\r' || *(lp + ch) == '\n')
	    break;
	i += EDIT_CharWidth(hwnd, (BYTE)(*(lp + ch)), i);
	ch++;
    }
    
    hStr = EDIT_HeapAlloc(hwnd, ch - ch1 + 3);
    str = (char *)EDIT_HeapAddr(hwnd, hStr);
    for (i = ch1, j = 0; i < ch; i++, j++)
	str[j] = lp[i];
    str[++j] = '\0';
    dprintf_edit(stddeb,"EDIT_GetStr: returning %s\n", str);
    return hStr;
}


/*********************************************************************
 *  WM_CHAR message function
 */

void EDIT_CharMsg(HWND hwnd, WORD wParam)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);

    dprintf_edit(stddeb,"EDIT_CharMsg: wParam=%c\n", (char)wParam);

    switch (wParam)
    {
    case '\r':
    case '\n':
	if (!IsMultiLine())
	    break;
	wParam = '\n';
	EDIT_KeyTyped(hwnd, wParam);
	break;

    case VK_TAB:
	if (!IsMultiLine())
	    break;
	EDIT_KeyTyped(hwnd, wParam);
	break;

    default:
	if (wParam >= 20 && wParam <= 126)
	    EDIT_KeyTyped(hwnd, wParam);
	break;
    }
}


/*********************************************************************
 *  EDIT_KeyTyped
 *
 *  Process keystrokes that produce displayable characters.
 */

void EDIT_KeyTyped(HWND hwnd, short ch)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));
    char *text = EDIT_HeapAddr(hwnd, es->hText);
    char *currchar = CurrChar;
    RECT rc;
    BOOL FullPaint = FALSE;

    dprintf_edit(stddeb,"EDIT_KeyTyped: ch=%c\n", (char)ch);

    /* delete selected text (if any) */
    if (SelMarked(es))
	EDIT_DeleteSel(hwnd);

    /* test for typing at end of maximum buffer size */
    if (currchar == text + es->MaxTextLen)
    {
	NOTIFY_PARENT(hwnd, EN_ERRSPACE);
	return;
    }

    if (*currchar == '\0' && IsMultiLine())
    {
	/* insert a newline at end of text */
	*currchar = '\r';
	*(currchar + 1) = '\n';
	*(currchar + 2) = '\0';
	EDIT_BuildTextPointers(hwnd);
    }

    /* insert the typed character */
    if (text[es->textlen - 1] != '\0')
    {
	/* current text buffer is full */
	if (es->textlen == es->MaxTextLen)
	{
	    /* text buffer is at maximum size */
	    NOTIFY_PARENT(hwnd, EN_ERRSPACE);
	    return;
	}

	/* increase the text buffer size */
	es->textlen += GROWLENGTH;
	/* but not above maximum size */
	if (es->textlen > es->MaxTextLen)
	    es->textlen = es->MaxTextLen;
	es->hText = EDIT_HeapReAlloc(hwnd, es->hText, es->textlen + 2);
	if (!es->hText)
	    NOTIFY_PARENT(hwnd, EN_ERRSPACE);
	text = EDIT_HeapAddr(hwnd, es->hText);
	text[es->textlen - 1] = '\0';
	currchar = CurrChar;
    }
    /* make space for new character and put char in buffer */
    if (ch == '\n')
    {
	memmove(currchar + 2, currchar, strlen(currchar) + 1);
	*currchar = '\r';
	*(currchar + 1) = '\n';
	EDIT_ModTextPointers(hwnd, es->CurrLine + 1, 2);
    }
    else
    {
	memmove(currchar + 1, currchar, strlen(currchar) + 1);
	*currchar = ch;
	EDIT_ModTextPointers(hwnd, es->CurrLine + 1, 1);
    }
    es->TextChanged = TRUE;
    NOTIFY_PARENT(hwnd, EN_UPDATE);

    /* re-adjust textwidth, if necessary, and redraw line */
    HideCaret(hwnd);
    if (IsMultiLine() && es->wlines > 1)
    {
	es->textwidth = max(es->textwidth,
		    EDIT_StrLength(hwnd, EDIT_TextLine(hwnd, es->CurrLine),
		    (int)(EDIT_TextLine(hwnd, es->CurrLine + 1) -
			  EDIT_TextLine(hwnd, es->CurrLine)), 0));
    }
    else
	es->textwidth = max(es->textwidth,
			    EDIT_StrLength(hwnd, text, strlen(text), 0));
    EDIT_WriteTextLine(hwnd, NULL, es->wtop + es->WndRow);

    if (ch == '\n')
    {
	if (es->wleft > 0)
	    FullPaint = TRUE;
	es->wleft = 0;
	EDIT_BuildTextPointers(hwnd);
	EDIT_End(hwnd);
	EDIT_Forward(hwnd);

	/* invalidate rest of window */
	GetClientRect(hwnd, &rc);
	if (!FullPaint)
	    rc.top = es->WndRow * es->txtht;
	InvalidateRect(hwnd, &rc, FALSE);

	SetCaretPos(es->WndCol, es->WndRow * es->txtht);
	ShowCaret(hwnd);
	UpdateWindow(hwnd);
	NOTIFY_PARENT(hwnd, EN_CHANGE);
	return;
    }

    /* test end of window */
    if (es->WndCol >= ClientWidth(wndPtr) - 
	                    EDIT_CharWidth(hwnd, (BYTE)ch, es->WndCol + es->wleft))
    {
	/* TODO:- Word wrap to be handled here */

/*	if (!(currchar == text + es->MaxTextLen - 2)) */
	    EDIT_KeyHScroll(hwnd, SB_LINEDOWN);
    }
    es->WndCol += EDIT_CharWidth(hwnd, (BYTE)ch, es->WndCol + es->wleft);
    es->CurrCol++;
    SetCaretPos(es->WndCol, es->WndRow * es->txtht);
    ShowCaret(hwnd);
    NOTIFY_PARENT(hwnd, EN_CHANGE);
}


/*********************************************************************
 *  EDIT_CharWidth
 *
 *  Return the width of the given character in pixels.
 *  The current column offset in pixels _pcol_ is required to calculate 
 *  the width of a tab.
 */

int EDIT_CharWidth(HWND hwnd, short ch, int pcol)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));
    short *charWidths = (short *)EDIT_HeapAddr(hwnd, es->hCharWidths);

    if (ch != VK_TAB)
	return (charWidths[ch]);
    else
	return (EDIT_GetNextTabStop(hwnd, pcol) - pcol);
}


/*********************************************************************
 *  EDIT_GetNextTabStop
 *
 *  Return the next tab stop beyond _pcol_.
 */

int EDIT_GetNextTabStop(HWND hwnd, int pcol)
{
    int i;
    int baseUnitWidth = LOWORD(GetDialogBaseUnits());
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));
    unsigned short *tabstops = EDIT_HeapAddr(hwnd, es->hTabStops);

    if (es->NumTabStops == 0)
	return ROUNDUP(pcol, 8 * baseUnitWidth);
    else if (es->NumTabStops == 1)
	return ROUNDUP(pcol, *tabstops * baseUnitWidth / 4);
    else
    {
	for (i = 0; i < es->NumTabStops; i++)
	{
	    if (*(tabstops + i) * baseUnitWidth / 4 >= pcol)
		return (*(tabstops + i) * baseUnitWidth / 4);
	}
	return pcol;
    }
}


/*********************************************************************
 *  EDIT_Forward
 *
 *  Cursor right key: move right one character position.
 */

void EDIT_Forward(HWND hwnd)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));

    if (*CurrChar == '\0')
	return;

    if (*CurrChar == '\r')
    {
	EDIT_Home(hwnd);
	EDIT_Downward(hwnd);
    }
    else
    {
	es->WndCol += EDIT_CharWidth(hwnd, (BYTE)(*CurrChar), es->WndCol + es->wleft);
	es->CurrCol++;
	if (es->WndCol >= ClientWidth(wndPtr))
	    EDIT_KeyHScroll(hwnd, SB_LINEDOWN);
    }
}



/*********************************************************************
 *  EDIT_Downward
 *
 *  Cursor down key: move down one line.
 */

void EDIT_Downward(HWND hwnd)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));

    dprintf_edit(stddeb,"EDIT_Downward: WndRow=%d, wtop=%d, wlines=%d\n", 
	     es->WndRow, es->wtop, es->wlines);

    if (IsMultiLine() && (es->WndRow + es->wtop + 1 < es->wlines))
    {
	es->CurrLine++;
	if (es->WndRow == ClientHeight(wndPtr, es) - 1)
	{
	    es->WndRow++;
	    EDIT_KeyVScrollLine(hwnd, SB_LINEDOWN);
	}
	else
	    es->WndRow++;
	EDIT_StickEnd(hwnd);
    }
}


/*********************************************************************
 *  EDIT_Upward
 *
 *  Cursor up key: move up one line.
 */

void EDIT_Upward(HWND hwnd)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));

    if (IsMultiLine() && es->CurrLine != 0)
    {
	--es->CurrLine;
	if (es->WndRow == 0)
	{
	    --es->WndRow;
	    EDIT_KeyVScrollLine(hwnd, SB_LINEUP);
	}
	else
	    --es->WndRow;
	EDIT_StickEnd(hwnd);
    }
}


/*********************************************************************
 *  EDIT_Backward
 *
 *  Cursor left key: move left one character position.
 */

void EDIT_Backward(HWND hwnd)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));

    if (es->CurrCol)
    {
	--es->CurrCol;
	if (*CurrChar == VK_TAB)
	    es->WndCol -= EDIT_CharWidth(hwnd, (BYTE)(*CurrChar), 
					 EDIT_StrLength(hwnd, 
					 EDIT_TextLine(hwnd, es->CurrLine), 
					 es->CurrCol, 0));
	else
	    es->WndCol -= EDIT_CharWidth(hwnd, (BYTE)(*CurrChar), 0);
	if (es->WndCol < 0)
	    EDIT_KeyHScroll(hwnd, SB_LINEUP);
    }
    else if (IsMultiLine() && es->CurrLine != 0)
    {
	EDIT_Upward(hwnd);
	EDIT_End(hwnd);
    }
}


/*********************************************************************
 *  EDIT_End
 *
 *  End key: move to end of line.
 */

void EDIT_End(HWND hwnd)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));

    while (*CurrChar && *CurrChar != '\r')
    {
	es->WndCol += EDIT_CharWidth(hwnd, (BYTE)(*CurrChar), es->WndCol + es->wleft);
	es->CurrCol++;
    }

    if (es->WndCol >= ClientWidth(wndPtr))
    {
	es->wleft = es->WndCol - ClientWidth(wndPtr) + HSCROLLDIM;
	es->WndCol -= es->wleft;
	InvalidateRect(hwnd, NULL, FALSE);
	UpdateWindow(hwnd);
    }
}


/*********************************************************************
 *  EDIT_Home
 *
 *  Home key: move to beginning of line.
 */

void EDIT_Home(HWND hwnd)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));

    es->CurrCol = es->WndCol = 0;
    if (es->wleft != 0)
    {
	es->wleft = 0;
	InvalidateRect(hwnd, NULL, FALSE);
	UpdateWindow(hwnd);
    }
}


/*********************************************************************
 *  EDIT_StickEnd
 *
 *  Stick the cursor to the end of the line.
 */

void EDIT_StickEnd(HWND hwnd)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));
    int len = EDIT_LineLength(hwnd, es->CurrLine);
    char *cp = EDIT_TextLine(hwnd, es->CurrLine);
    char currpel;

    es->CurrCol = min(len, es->CurrCol);
    es->WndCol = min(EDIT_StrLength(hwnd, cp, len, 0) - es->wleft, es->WndCol);
    currpel = EDIT_StrLength(hwnd, cp, es->CurrCol, 0);

    if (es->wleft > currpel)
    {
	es->wleft = max(0, currpel - 20);
	es->WndCol = currpel - es->wleft;
	UpdateWindow(hwnd);
    }
    else if (currpel - es->wleft >= ClientWidth(wndPtr))
    {
	es->wleft = currpel - (ClientWidth(wndPtr) - 5);
	es->WndCol = currpel - es->wleft;
	UpdateWindow(hwnd);
    }
}


/*********************************************************************
 *  WM_KEYDOWN message function
 */

void EDIT_KeyDownMsg(HWND hwnd, WORD wParam)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));

    dprintf_edit(stddeb,"EDIT_KeyDownMsg: key=%x\n", wParam);

    HideCaret(hwnd);
    switch (wParam)
    {
    case VK_UP:
	if (SelMarked(es))
	    EDIT_ClearSel(hwnd);
	if (IsMultiLine())
	    EDIT_Upward(hwnd);
	else
	    EDIT_Backward(hwnd);
	break;

    case VK_DOWN:
	if (SelMarked(es))
	    EDIT_ClearSel(hwnd);
	if (IsMultiLine())
	    EDIT_Downward(hwnd);
	else
	    EDIT_Forward(hwnd);
	break;

    case VK_RIGHT:
	if (SelMarked(es))
	    EDIT_ClearSel(hwnd);
	EDIT_Forward(hwnd);
	break;

    case VK_LEFT:
	if (SelMarked(es))
	    EDIT_ClearSel(hwnd);
	EDIT_Backward(hwnd);
	break;

    case VK_HOME:
	if (SelMarked(es))
	    EDIT_ClearSel(hwnd);
	EDIT_Home(hwnd);
	break;

    case VK_END:
	if (SelMarked(es))
	    EDIT_ClearSel(hwnd);
	EDIT_End(hwnd);
	break;

    case VK_PRIOR:
	if (IsMultiLine())
	{
	    if (SelMarked(es))
		EDIT_ClearSel(hwnd);
	    EDIT_KeyVScrollPage(hwnd, SB_PAGEUP);
	}
	break;

    case VK_NEXT:
	if (IsMultiLine())
	{
	    if (SelMarked(es))
		EDIT_ClearSel(hwnd);
	    EDIT_KeyVScrollPage(hwnd, SB_PAGEDOWN);
	}
	break;

    case VK_BACK:
	if (SelMarked(es))
	    EDIT_DeleteSel(hwnd);
	else
	{
	    if (es->CurrCol == 0 && es->CurrLine == 0)
		break;
	    EDIT_Backward(hwnd);
	    EDIT_DelKey(hwnd);
	}
	break;

    case VK_DELETE:
	if (SelMarked(es))
	    EDIT_DeleteSel(hwnd);
	else
	    EDIT_DelKey(hwnd);
	break;
    }

    SetCaretPos(es->WndCol, es->WndRow * es->txtht);
    ShowCaret(hwnd);
}


/*********************************************************************
 *  EDIT_KeyHScroll
 *
 *  Scroll text horizontally using cursor keys.
 */

void EDIT_KeyHScroll(HWND hwnd, WORD opt)
{
    int hscrollpos;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));

    if (opt == SB_LINEDOWN)
    {
	es->wleft += HSCROLLDIM;
	es->WndCol -= HSCROLLDIM;
    }
    else
    {
	if (es->wleft == 0)
	    return;
	if (es->wleft - HSCROLLDIM < 0)
	{
	    es->WndCol += es->wleft;
	    es->wleft = 0;
	}	    
	else
	{
	    es->wleft -= HSCROLLDIM;
	    es->WndCol += HSCROLLDIM;
	}
    }

    InvalidateRect(hwnd, NULL, FALSE);
    UpdateWindow(hwnd);

    if (IsHScrollBar())
    {
	hscrollpos = EDIT_ComputeHScrollPos(hwnd);
	SetScrollPos(hwnd, SB_HORZ, hscrollpos, TRUE);
    }
}


/*********************************************************************
 *  EDIT_KeyVScrollLine
 *
 *  Scroll text vertically by one line using keyboard.
 */

void EDIT_KeyVScrollLine(HWND hwnd, WORD opt)
{
    RECT rc;
    int y, vscrollpos;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));

    if (!IsMultiLine())
	return;

    if (opt == SB_LINEDOWN)
    {
	/* move down one line */
	if (es->wtop + ClientHeight(wndPtr, es) >= es->wlines)
	    return;
	es->wtop++;
    }
    else
    {
	/* move up one line */
	if (es->wtop == 0)
	    return;
	--es->wtop;
    }

    if (IsWindowVisible(hwnd))
    {
	/* adjust client bottom to nearest whole line */
	GetClientRect(hwnd, &rc);
	rc.bottom = (rc.bottom / es->txtht) * es->txtht;

	if (opt == SB_LINEUP)
	{
	    /* move up one line (scroll window down) */
	    ScrollWindow(hwnd, 0, es->txtht, &rc, &rc);
	    /* write top line */
	    EDIT_WriteTextLine(hwnd, NULL, es->wtop);
	    es->WndRow++;
	}
	else
	{
	    /* move down one line (scroll window up) */
	    ScrollWindow(hwnd, 0, -(es->txtht), &rc, &rc);
	    /* write bottom line */
	    y = (((rc.bottom - rc.top) / es->txtht) - 1);
	    EDIT_WriteTextLine(hwnd, NULL, es->wtop + y);
	    --es->WndRow;
	}
    }

    /* reset the vertical scroll bar */
    if (IsVScrollBar())
    {
	vscrollpos = EDIT_ComputeVScrollPos(hwnd);
	SetScrollPos(hwnd, SB_VERT, vscrollpos, TRUE);
    }
}


/*********************************************************************
 *  EDIT_KeyVScrollPage
 *
 *  Scroll text vertically by one page using keyboard.
 */

void EDIT_KeyVScrollPage(HWND hwnd, WORD opt)
{
    int vscrollpos;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));

    if (IsMultiLine())
    {
	if (opt == SB_PAGEUP)
	{
	    if (es->wtop)
		es->wtop -= ClientHeight(wndPtr, es);
	}
	else
	{
	    if (es->wtop + ClientHeight(wndPtr, es) < es->wlines)
	    {
		es->wtop += ClientHeight(wndPtr, es);
		if (es->wtop > es->wlines - ClientHeight(wndPtr, es))
		    es->wtop = es->wlines - ClientHeight(wndPtr, es);
	    }
	}
	if (es->wtop < 0)
	    es->wtop = 0;

	es->CurrLine = es->wtop + es->WndRow;
	EDIT_StickEnd(hwnd);
	InvalidateRect(hwnd, NULL, TRUE);
	UpdateWindow(hwnd);

	/* reset the vertical scroll bar */
	if (IsVScrollBar())
	{
	    vscrollpos = EDIT_ComputeVScrollPos(hwnd);
	    SetScrollPos(hwnd, SB_VERT, vscrollpos, TRUE);
	}
    }
}


/*********************************************************************
 *  EDIT_KeyVScrollDoc
 *
 *  Scroll text to top and bottom of document using keyboard.
 */

void EDIT_KeyVScrollDoc(HWND hwnd, WORD opt)
{
    int vscrollpos;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));

    if (!IsMultiLine())
	return;

    if (opt == SB_TOP)
	es->wtop = es->wleft = 0;
    else if (es->wtop + ClientHeight(wndPtr, es) < es->wlines)
    {
	es->wtop = es->wlines - ClientHeight(wndPtr, es);
	es->wleft = 0;
    }

    es->CurrLine = es->wlines;
    es->WndRow = es->wlines - es->wtop;
    EDIT_End(hwnd);
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);

    /* reset the vertical scroll bar */
    if (IsVScrollBar())
    {
	vscrollpos = EDIT_ComputeVScrollPos(hwnd);
	SetScrollPos(hwnd, SB_VERT, vscrollpos, TRUE);
    }
}


/*********************************************************************
 *  EDIT_ComputeVScrollPos
 *
 *  Compute the vertical scroll bar position from the window
 *  position and text width.
 */

int EDIT_ComputeVScrollPos(HWND hwnd)
{
    int vscrollpos;
    short minpos, maxpos;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));

    GetScrollRange(hwnd, SB_VERT, &minpos, &maxpos);

    if (es->wlines > ClientHeight(wndPtr, es))
	vscrollpos = (double)(es->wtop) / (double)(es->wlines - 
		     ClientHeight(wndPtr, es)) * (maxpos - minpos);
    else
	vscrollpos = minpos;

    return vscrollpos;
}
    

/*********************************************************************
 *  EDIT_ComputeHScrollPos
 *
 *  Compute the horizontal scroll bar position from the window
 *  position and text width.
 */

int EDIT_ComputeHScrollPos(HWND hwnd)
{
    int hscrollpos;
    short minpos, maxpos;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));

    GetScrollRange(hwnd, SB_HORZ, &minpos, &maxpos);

    if (es->textwidth > ClientWidth(wndPtr))
	hscrollpos = (double)(es->wleft) / (double)(es->textwidth - 
		     ClientWidth(wndPtr)) * (maxpos - minpos);
    else
	hscrollpos = minpos;

    return hscrollpos;
}


/*********************************************************************
 *  EDIT_DelKey
 *
 *  Delete character to right of cursor.
 */

void EDIT_DelKey(HWND hwnd)
{
    RECT rc;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));
    char *currchar = CurrChar;
    BOOL repaint = *currchar == '\n';

    if (IsMultiLine() && *currchar == '\n' && *(currchar + 1) == '\0')
	return;
    strcpy(currchar, currchar + 1);
    NOTIFY_PARENT(hwnd, EN_UPDATE);
    
    if (repaint)
    {
	EDIT_BuildTextPointers(hwnd);
	GetClientRect(hwnd, &rc);
	rc.top = es->WndRow * es->txtht;
	InvalidateRect(hwnd, &rc, FALSE);
	UpdateWindow(hwnd);
    }
    else
    {
	EDIT_ModTextPointers(hwnd, es->CurrLine + 1, -1);
	EDIT_WriteTextLine(hwnd, NULL, es->WndRow + es->wtop);
    }

    es->TextChanged = TRUE;
    NOTIFY_PARENT(hwnd, EN_CHANGE);
}

/*********************************************************************
 *  WM_VSCROLL message function
 */

void EDIT_VScrollMsg(HWND hwnd, WORD wParam, LONG lParam)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));

    if (IsMultiLine())
    {
	HideCaret(hwnd);

	switch (wParam)
	{
	case SB_LINEUP:
	case SB_LINEDOWN:
	    EDIT_VScrollLine(hwnd, wParam);
	    break;

	case SB_PAGEUP:
	case SB_PAGEDOWN:
	    EDIT_VScrollPage(hwnd, wParam);
	    break;
	}
    }
    
    SetCaretPos(es->WndCol, es->WndRow);
    ShowCaret(hwnd);
}


/*********************************************************************
 *  EDIT_VScrollLine
 *
 *  Scroll text vertically by one line using scrollbars.
 */

void EDIT_VScrollLine(HWND hwnd, WORD opt)
{
    RECT rc;
    int y;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));

    dprintf_edit(stddeb,"EDIT_VScrollLine: direction=%d\n", opt);

    if (opt == SB_LINEDOWN)
    {
	/* move down one line */
	if (es->wtop + ClientHeight(wndPtr, es) >= es->wlines)
	    return;
	es->wtop++;
    }
    else
    {
	/* move up one line */
	if (es->wtop == 0)
	    return;
	--es->wtop;
    }

    if (IsWindowVisible(hwnd))
    {
	/* adjust client bottom to nearest whole line */
	GetClientRect(hwnd, &rc);
	rc.bottom = (rc.bottom / es->txtht) * es->txtht;

	if (opt == SB_LINEUP)
	{
	    /* move up one line (scroll window down) */
	    ScrollWindow(hwnd, 0, es->txtht, &rc, &rc);
	    /* write top line */
	    EDIT_WriteTextLine(hwnd, NULL, es->wtop);
	    es->WndRow++;
	}
	else
	{
	    /* move down one line (scroll window up) */
	    ScrollWindow(hwnd, 0, -(es->txtht), &rc, &rc);
	    /* write bottom line */
	    y = ((rc.bottom - rc.top / es->txtht) - 1);
	    EDIT_WriteTextLine(hwnd, NULL, es->wtop + y);
	    --es->WndRow;
	}
    }
}


/*********************************************************************
 *  EDIT_VScrollPage
 *
 *  Scroll text vertically by one page using keyboard.
 */

void EDIT_VScrollPage(HWND hwnd, WORD opt)
{
    int vscrollpos;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));

    if (opt == SB_PAGEUP)
    {
	if (es->wtop)
	    es->wtop -= ClientHeight(wndPtr, es);
    }
    else
    {
	if (es->wtop + ClientHeight(wndPtr, es) < es->wlines)
	{
	    es->wtop += ClientHeight(wndPtr, es);
	    if (es->wtop > es->wlines - ClientHeight(wndPtr, es))
		es->wtop = es->wlines - ClientHeight(wndPtr, es);
	}
    }
    if (es->wtop < 0)
	es->wtop = 0;
    
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);

    /* reset the vertical scroll bar */
    if (IsVScrollBar())
    {
	vscrollpos = EDIT_ComputeVScrollPos(hwnd);
	SetScrollPos(hwnd, SB_VERT, vscrollpos, TRUE);
    }
}


/*********************************************************************
 *  WM_HSCROLL message function
 */

void EDIT_HScrollMsg(HWND hwnd, WORD wParam, LONG lParam)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));

    switch (wParam)
    {
    case SB_LINEUP:
    case SB_LINEDOWN:
	HideCaret(hwnd);

	SetCaretPos(es->WndCol, es->WndRow * es->txtht);
	ShowCaret(hwnd);
	break;
    }
}


/*********************************************************************
 *  WM_SIZE message function
 */

void EDIT_SizeMsg(HWND hwnd, WORD wParam, LONG lParam)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));

    if (wParam != SIZE_MAXIMIZED && wParam != SIZE_RESTORED) return;

    InvalidateRect(hwnd, NULL, TRUE);
    es->PaintBkgd = TRUE;
    UpdateWindow(hwnd);
}


/*********************************************************************
 *  WM_LBUTTONDOWN message function
 */

void EDIT_LButtonDownMsg(HWND hwnd, WORD wParam, LONG lParam)
{
    char *cp;
    int len;
    BOOL end = FALSE;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));

    if (SelMarked(es))
	EDIT_ClearSel(hwnd);

    es->WndRow = HIWORD(lParam) / es->txtht;
    if (es->WndRow > es->wlines - es->wtop - 1)
    {
	if (es->wlines)
	    es->WndRow = es->wlines - es->wtop - 1;
	else
	    es->WndRow = 0;
	end = TRUE;
    }
    es->CurrLine = es->wtop + es->WndRow;

    cp = EDIT_TextLine(hwnd, es->CurrLine);
    len = EDIT_LineLength(hwnd, es->CurrLine);
    es->WndCol = LOWORD(lParam);
    if (es->WndCol > EDIT_StrLength(hwnd, cp, len, 0) - es->wleft || end)
	es->WndCol = EDIT_StrLength(hwnd, cp, len, 0) - es->wleft;
    es->CurrCol = EDIT_PixelToChar(hwnd, es->CurrLine, &(es->WndCol));

    ButtonDown = TRUE;
    ButtonRow = es->CurrLine;
    ButtonCol = es->CurrCol;
}


/*********************************************************************
 *  WM_MOUSEMOVE message function
 */

void EDIT_MouseMoveMsg(HWND hwnd, WORD wParam, LONG lParam)
{
    if (wParam != MK_LBUTTON)
	return;

    if (ButtonDown)
    {
	EDIT_SetAnchor(hwnd, ButtonRow, ButtonCol);
	TextMarking = TRUE;
	ButtonDown = FALSE;
    }

    if (TextMarking)
	EDIT_ExtendSel(hwnd, LOWORD(lParam), HIWORD(lParam));
}


/*********************************************************************
 *  EDIT_PixelToChar
 *
 *  Convert a pixel offset in the given row to a character offset,
 *  adjusting the pixel offset to the nearest whole character if
 *  necessary.
 */

int EDIT_PixelToChar(HWND hwnd, int row, int *pixel)
{
    int ch = 0, i = 0, s_i = 0;
    char *text;

    dprintf_edit(stddeb,"EDIT_PixelToChar: row=%d, pixel=%d\n", row, *pixel);

    text = EDIT_TextLine(hwnd, row);
    while (i < *pixel)
    {
	s_i = i;
	i += EDIT_CharWidth(hwnd, (BYTE)(*(text + ch)), i);
	ch++;
    }

    /* if stepped past _pixel_, go back a character */
    if (i - *pixel)
    {
	i = s_i;
	--ch;
    }
    *pixel = i;
    return ch;
}


/*********************************************************************
 *  WM_SETTEXT message function
 */

LONG EDIT_SetTextMsg(HWND hwnd, LONG lParam)
{
    int len;
    char *text;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));

    if (strlen((char *)lParam) <= es->MaxTextLen)
    {
	len = ( lParam? strlen((char *)lParam) : 0 );
	EDIT_ClearText(hwnd);
	es->textlen = len;
	es->hText = EDIT_HeapReAlloc(hwnd, es->hText, len + 3);
	text = EDIT_HeapAddr(hwnd, es->hText);
	if (lParam)
	    strcpy(text, (char *)lParam);
	text[len]     = '\0';
	text[len + 1] = '\0';
	text[len + 2] = '\0';
	EDIT_BuildTextPointers(hwnd);
	InvalidateRect(hwnd, NULL, TRUE);
	es->PaintBkgd = TRUE;
	es->TextChanged = TRUE;
	return 0L;
    }
    else
	return EN_ERRSPACE;
}


/*********************************************************************
 *  EDIT_ClearText
 *
 *  Clear text from text buffer.
 */

void EDIT_ClearText(HWND hwnd)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));
    unsigned int blen = EditBufLen(wndPtr) + 2;
    char *text;

    es->hText = EDIT_HeapReAlloc(hwnd, es->hText, blen);
    text = EDIT_HeapAddr(hwnd, es->hText);
    memset(text, 0, blen);
    es->textlen = 0;
    es->wlines = 0;
    es->CurrLine = es->CurrCol = 0;
    es->WndRow = es->WndCol = 0;
    es->wleft = es->wtop = 0;
    es->textwidth = 0;
    es->TextChanged = FALSE;
    EDIT_ClearTextPointers(hwnd);
}


/*********************************************************************
 *  EM_SETSEL message function
 */

void EDIT_SetSelMsg(HWND hwnd, WORD wParam, LONG lParam)
{
    int so, eo;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));

    so = LOWORD(lParam);
    eo = HIWORD(lParam);

    if (so == -1)       /* if so == -1, clear selection */
    {
	EDIT_ClearSel(hwnd);
	return;
    }

    if (so == eo)       /* if so == eo, set caret only */
    {
	EDIT_GetLineCol(hwnd, so, &(es->CurrLine), &(es->CurrCol));
	es->WndRow = es->CurrLine - es->wtop;

	if (!wParam)
	{
	    if (es->WndRow < 0 || es->WndRow > ClientHeight(wndPtr, es))
	    {
		es->wtop = es->CurrLine;
		es->WndRow = 0;
	    }
	    es->WndCol = EDIT_StrLength(hwnd, 
					EDIT_TextLine(hwnd, es->CurrLine), 
					es->CurrCol, 0) - es->wleft;
	    if (es->WndCol > ClientWidth(wndPtr))
	    {
		es->wleft = es->WndCol;
		es->WndCol = 0;
	    }
	    else if (es->WndCol < 0)
	    {
		es->wleft += es->WndCol;
		es->WndCol = 0;
	    }
	}
    }
    else                /* otherwise set selection */
    {
	if (so > eo)
	    swap(&so, &eo);

	EDIT_GetLineCol(hwnd, so, &(es->SelBegLine), &(es->SelBegCol));
	EDIT_GetLineCol(hwnd, eo, &(es->SelEndLine), &(es->SelEndCol));
	es->CurrLine = es->SelEndLine;
	es->CurrCol = es->SelEndCol;
	es->WndRow = es->SelEndLine - es->wtop;

	if (!wParam)          /* don't suppress scrolling of text */
	{
	    if (es->WndRow < 0)
	    {
		es->wtop = es->SelEndLine;
		es->WndRow = 0;
	    }
	    else if (es->WndRow > ClientHeight(wndPtr, es))
	    {
		es->wtop += es->WndRow - ClientHeight(wndPtr, es);
		es->WndRow = ClientHeight(wndPtr, es);
	    }
	    es->WndCol = EDIT_StrLength(hwnd, 
					EDIT_TextLine(hwnd, es->SelEndLine), 
					es->SelEndCol, 0) - es->wleft;
	    if (es->WndCol > ClientWidth(wndPtr))
	    {
		es->wleft += es->WndCol - ClientWidth(wndPtr);
		es->WndCol = ClientWidth(wndPtr);
	    }
	    else if (es->WndCol < 0)
	    {
		es->wleft += es->WndCol;
		es->WndCol = 0;
	    }
	}

	InvalidateRect(hwnd, NULL, TRUE);
	UpdateWindow(hwnd);
    }
}


/*********************************************************************
 *  EDIT_GetLineCol
 *
 *  Return line and column in text buffer from character offset.
 */

void EDIT_GetLineCol(HWND hwnd, int off, int *line, int *col)
{
    int lineno;
    char *cp, *cp1;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));
    char *text = EDIT_HeapAddr(hwnd, es->hText);
    unsigned int *textPtrs = 
	(unsigned int *)EDIT_HeapAddr(hwnd, es->hTextPtrs);

    /* check for (0,0) */
    if (!off)
    {
	*line = 0;
	*col = 0;
	return;
    }

    if (off > strlen(text)) off = strlen(text);
    cp1 = text;
    for (lineno = 0; lineno < es->wlines; lineno++)
    {
	cp = text + *(textPtrs + lineno);
	if (off == (int)(cp - text))
	{
	    *line = lineno;
	    *col = 0;
	    return;
	}
	if (off < (int)(cp - text))
	    break;
	cp1 = cp;
    }
    *line = lineno - 1;
    *col = off - (int)(cp1 - text);
#if 0
    if (*(text + *col) == '\0')
	(*col)--;
#endif
}


/*********************************************************************
 *  EDIT_DeleteSel
 *
 *  Delete the current selected text (if any)
 */

void EDIT_DeleteSel(HWND hwnd)
{
    char *bbl, *bel;
    int len;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));

    if (SelMarked(es))
    {
	bbl = EDIT_TextLine(hwnd, es->SelBegLine) + es->SelBegCol;
	bel = EDIT_TextLine(hwnd, es->SelEndLine) + es->SelEndCol;
	len = (int)(bel - bbl);
	EDIT_SaveDeletedText(hwnd, bbl, len, es->SelBegLine, es->SelBegCol);
	es->TextChanged = TRUE;
	strcpy(bbl, bel);

	es->CurrLine = es->SelBegLine;
	es->CurrCol = es->SelBegCol;
	es->WndRow = es->SelBegLine - es->wtop;
	if (es->WndRow < 0)
	{
	    es->wtop = es->SelBegLine;
	    es->WndRow = 0;
	}
	es->WndCol = EDIT_StrLength(hwnd, bbl - es->SelBegCol, 
				     es->SelBegCol, 0) - es->wleft;

	EDIT_BuildTextPointers(hwnd);
	es->PaintBkgd = TRUE;
	EDIT_ClearSel(hwnd);
    }
}
    

/*********************************************************************
 *  EDIT_ClearSel
 *
 *  Clear the current selection.
 */

void EDIT_ClearSel(HWND hwnd)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));

    es->SelBegLine = es->SelBegCol = 0;
    es->SelEndLine = es->SelEndCol = 0;

    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
}


/*********************************************************************
 *  EDIT_TextLineNumber
 *
 *  Return the line number in the text buffer of the supplied
 *  character pointer.
 */

int EDIT_TextLineNumber(HWND hwnd, char *lp)
{
    int lineno;
    char *cp;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));
    char *text = EDIT_HeapAddr(hwnd, es->hText);
    unsigned int *textPtrs = 
	(unsigned int *)EDIT_HeapAddr(hwnd, es->hTextPtrs);

    for (lineno = 0; lineno < es->wlines; lineno++)
    {
	cp = text + *(textPtrs + lineno);
	if (cp == lp)
	    return lineno;
	if (cp > lp)
	    break;
    }
    return lineno - 1;
}


/*********************************************************************
 *  EDIT_SetAnchor
 *
 *  Set down anchor for text marking.
 */

void EDIT_SetAnchor(HWND hwnd, int row, int col)
{
    BOOL sel = FALSE;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));

    if (SelMarked(es))
	sel = TRUE;
    EDIT_ClearSel(hwnd);
    es->SelBegLine = es->SelEndLine = row;
    es->SelBegCol = es->SelEndCol = col;
    if (sel)
    {
	InvalidateRect(hwnd, NULL, FALSE);
	UpdateWindow(hwnd);
    }
}


/*********************************************************************
 *  EDIT_ExtendSel
 *
 *  Extend selection to the given screen co-ordinates.
 */

void EDIT_ExtendSel(HWND hwnd, int x, int y)
{
    int bbl, bel, bbc, bec;
    char *cp;
    int len, line;
    BOOL end = FALSE;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));

    dprintf_edit(stddeb,"EDIT_ExtendSel: x=%d, y=%d\n", x, y);

    bbl = es->SelEndLine;
    bbc = es->SelEndCol;
    line = es->wtop + y / es->txtht;
    if (line > es->wlines)
	line = es->wlines;
    cp = EDIT_TextLine(hwnd, line);
    len = EDIT_LineLength(hwnd, line);

    es->WndRow = y / es->txtht;
    if (es->WndRow > es->wlines - es->wtop - 1)
    {
	if (es->wlines)
	    es->WndRow = es->wlines - es->wtop - 1;
	else
	    es->WndRow = 0;
	end = TRUE;
    }
    es->CurrLine = es->wtop + es->WndRow;
    es->SelEndLine = es->CurrLine;

    es->WndCol = x;
    if (es->WndCol > EDIT_StrLength(hwnd, cp, len, 0) - es->wleft || end)
	es->WndCol = EDIT_StrLength(hwnd, cp, len, 0) - es->wleft;
    es->CurrCol = EDIT_PixelToChar(hwnd, es->CurrLine, &(es->WndCol));
    es->SelEndCol = es->CurrCol;

    bel = es->SelEndLine;
    bec = es->SelEndCol;

    /* return if no new characters to mark */
    if (bbl == bel && bbc == bec)
	return;

    /* put lowest marker first */
    if (bbl > bel)
    {
	swap(&bbl, &bel);
	swap(&bbc, &bec);
    }
    if (bbl == bel && bbc > bec)
	swap(&bbc, &bec);

    for (y = bbl; y <= bel; y++)
    {
	if (y == bbl && y == bel)
	    EDIT_WriteSel(hwnd, y, bbc, bec);
	else if (y == bbl)
	    EDIT_WriteSel(hwnd, y, bbc, -1);
	else if (y == bel)
	    EDIT_WriteSel(hwnd, y, 0, bec);
	else
	    EDIT_WriteSel(hwnd, y, 0, -1);
    }
}


/*********************************************************************
 *  EDIT_WriteSel
 *
 *  Display selection by reversing pixels in selected text.
 *  If end == -1, selection applies to end of line.
 */

void EDIT_WriteSel(HWND hwnd, int y, int start, int end)
{
    RECT rc;
    int scol, ecol;
    char *cp;
    HDC hdc;
    HBRUSH hbrush, holdbrush;
    int olddm;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));

    dprintf_edit(stddeb,"EDIT_WriteSel: y=%d start=%d end=%d\n", y, start,end);
    GetClientRect(hwnd, &rc);

    /* make sure y is within the window */
    if (y < es->wtop || y > (es->wtop + ClientHeight(wndPtr, es)))
	return;

    /* get pointer to text */
    cp = EDIT_TextLine(hwnd, y);

    /* get length of line if end == -1 */
    if (end == -1)
	end = EDIT_LineLength(hwnd, y);

    /* For some reason Rectangle, when called with R2_XORPEN filling,
     * appears to leave a 2 pixel gap between characters and between
     * lines.  I have kludged this by adding on two pixels to ecol and
     * to the line height in the call to Rectangle.
     */
    scol = EDIT_StrLength(hwnd, cp, start, 0);
    if (scol > rc.right) return;
    if (scol < rc.left) scol = rc.left;
    ecol = EDIT_StrLength(hwnd, cp, end, 0) + 2;   /* ??? */
    if (ecol < rc.left) return;
    if (ecol > rc.right) ecol = rc.right;

    hdc = GetDC(hwnd);
    hbrush = GetStockObject(BLACK_BRUSH);
    holdbrush = (HBRUSH)SelectObject(hdc, (HANDLE)hbrush);
    olddm = SetROP2(hdc, R2_XORPEN);
    Rectangle(hdc, scol, y * es->txtht, ecol, (y + 1) * es->txtht + 2);
    SetROP2(hdc, olddm);
    SelectObject(hdc, (HANDLE)holdbrush);
    ReleaseDC(hwnd, hdc);
}


/*********************************************************************
 *  EDIT_StopMarking
 *
 *  Stop text marking (selection).
 */

void EDIT_StopMarking(HWND hwnd)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));

    TextMarking = FALSE;
    if (es->SelBegLine > es->SelEndLine)
    {
	swap(&(es->SelBegLine), &(es->SelEndLine));
	swap(&(es->SelBegCol), &(es->SelEndCol));
    }
    if (es->SelBegLine == es->SelEndLine && es->SelBegCol > es->SelEndCol)
	swap(&(es->SelBegCol), &(es->SelEndCol));
}


/*********************************************************************
 *  EM_GETLINE message function
 */

LONG EDIT_GetLineMsg(HWND hwnd, WORD wParam, LONG lParam)
{
    char *cp, *cp1;
    int len;
    char *buffer = (char *)lParam;

    cp = EDIT_TextLine(hwnd, wParam);
    cp1 = EDIT_TextLine(hwnd, wParam + 1);
    len = min((int)(cp1 - cp), (WORD)(*buffer));
    strncpy(buffer, cp, len);

    return (LONG)len;
}


/*********************************************************************
 *  EM_GETSEL message function
 */

LONG EDIT_GetSelMsg(HWND hwnd)
{
    int so, eo;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));
    unsigned int *textPtrs = 
	(unsigned int *)EDIT_HeapAddr(hwnd, es->hTextPtrs);

    so = *(textPtrs + es->SelBegLine) + es->SelBegCol;
    eo = *(textPtrs + es->SelEndLine) + es->SelEndCol;

    return MAKELONG(so, eo);
}


/*********************************************************************
 *  EM_REPLACESEL message function
 */

void EDIT_ReplaceSel(HWND hwnd, LONG lParam)
{
    EDIT_DeleteSel(hwnd);
    EDIT_InsertText(hwnd, (char *)lParam, strlen((char *)lParam));
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
}


/*********************************************************************
 *  EDIT_InsertText
 *
 *  Insert text at current line and column.
 */

void EDIT_InsertText(HWND hwnd, char *str, int len)
{
    int plen;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));
    char *text = EDIT_HeapAddr(hwnd, es->hText);
    
    plen = strlen(text) + len;
    if (plen + 1 > es->textlen)
    {
	es->hText = EDIT_HeapReAlloc(hwnd, es->hText, es->textlen + len);
	es->textlen = plen + 1;
    }
    memmove(CurrChar + len, CurrChar, strlen(CurrChar) + 1);
    memcpy(CurrChar, str, len);

    EDIT_BuildTextPointers(hwnd);
    es->PaintBkgd = TRUE;
    es->TextChanged = TRUE;

    EDIT_GetLineCol(hwnd, (int)((CurrChar + len) - text), &(es->CurrLine),
		                                    &(es->CurrCol));
    es->WndRow = es->CurrLine - es->wtop;
    es->WndCol = EDIT_StrLength(hwnd, EDIT_TextLine(hwnd, es->CurrLine),
				 es->CurrCol, 0) - es->wleft;
}


/*********************************************************************
 *  EM_LINEFROMCHAR message function
 */

LONG EDIT_LineFromCharMsg(HWND hwnd, WORD wParam)
{
    int row, col;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));

    if (wParam == (WORD)-1)
	return (LONG)(es->SelBegLine);
    else
	EDIT_GetLineCol(hwnd, wParam, &row, &col);

    return (LONG)row;
}


/*********************************************************************
 *  EM_LINEINDEX message function
 */

LONG EDIT_LineIndexMsg(HWND hwnd, WORD wParam)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));
    unsigned int *textPtrs = 
	(unsigned int *)EDIT_HeapAddr(hwnd, es->hTextPtrs);

    if (wParam == (WORD)-1)
	wParam = es->CurrLine;

    return (LONG)(*(textPtrs + wParam));
}


/*********************************************************************
 *  EM_LINELENGTH message function
 */

LONG EDIT_LineLengthMsg(HWND hwnd, WORD wParam)
{
    int row, col, len;
    int sbl, sbc, sel, sec;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));
    unsigned int *textPtrs = 
	(unsigned int *)EDIT_HeapAddr(hwnd, es->hTextPtrs);

    if (wParam == (WORD)-1)
    {
	if (SelMarked(es))
	{
	    sbl = es->SelBegLine;
	    sbc = es->SelBegCol;
	    sel = es->SelEndLine;
	    sec = es->SelEndCol;

	    if (sbl > sel)
	    {
		swap(&sbl, &sel);
		swap(&sbc, &sec);
	    }
	    if (sbl == sel && sbc > sec)
		swap(&sbc, &sec);

	    if (sbc == sel)
	    {
		len = *(textPtrs + sbl + 1) - *(textPtrs + sbl) - 1;
		return len - sec - sbc;
	    }

	    len = *(textPtrs + sel + 1) - *(textPtrs + sel) - sec - 1;
	    return len + sbc;
	}
	else    /* no selection marked */
	{
	    len = *(textPtrs + es->CurrLine + 1) - 
		  *(textPtrs + es->CurrLine) - 1;
	    return len;
	}
    }
    else    /* line number specified */
    {
	EDIT_GetLineCol(hwnd, wParam, &row, &col);
	len = *(textPtrs + row + 1) - *(textPtrs + row);
	return len;
    }
}


/*********************************************************************
 *  WM_SETFONT message function
 */

void EDIT_SetFont(HWND hwnd, WORD wParam, LONG lParam)
{
    HDC hdc;
    TEXTMETRIC tm;
    HFONT oldfont;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));
    short *charWidths = (short *)EDIT_HeapAddr(hwnd, es->hCharWidths);

    es->hFont = wParam;
    hdc = GetDC(hwnd);
    oldfont = (HFONT)SelectObject(hdc, (HANDLE)es->hFont);
    GetCharWidth(hdc, 0, 255, charWidths);
    GetTextMetrics(hdc, &tm);
    es->txtht = tm.tmHeight + tm.tmExternalLeading;
    SelectObject(hdc, (HANDLE)oldfont);
    ReleaseDC(hwnd, hdc);

    es->WndRow = (es->CurrLine - es->wtop) / es->txtht;
    es->WndCol = EDIT_StrLength(hwnd, EDIT_TextLine(hwnd, es->CurrLine),
				 es->CurrCol, 0) - es->wleft;

    InvalidateRect(hwnd, NULL, TRUE);
    es->PaintBkgd = TRUE;
    if (lParam) UpdateWindow(hwnd);
}


/*********************************************************************
 *  EDIT_SaveDeletedText
 *
 *  Save deleted text in deleted text buffer.
 */

void EDIT_SaveDeletedText(HWND hwnd, char *deltext, int len,
			                  int line, int col)
{
    char *text;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));

    es->hDeletedText = GlobalReAlloc(es->hDeletedText, len, GMEM_MOVEABLE);
    if (!es->hDeletedText) return;
    text = (char *)GlobalLock(es->hDeletedText);
    memcpy(text, deltext, len);
    GlobalUnlock(es->hDeletedText);
    es->DeletedLength = len;
    es->DeletedCurrLine = line;
    es->DeletedCurrCol = col;
}


/*********************************************************************
 *  EDIT_ClearDeletedText
 *
 *  Clear deleted text buffer.
 */

void EDIT_ClearDeletedText(HWND hwnd)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));

    GlobalFree(es->hDeletedText);
    es->hDeletedText = 0;
    es->DeletedLength = 0;
}


/*********************************************************************
 *  EM_UNDO message function
 */

LONG EDIT_UndoMsg(HWND hwnd)
{
    char *text;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));
    
    if (es->hDeletedText)
    {
	text = (char *)GlobalLock(es->hDeletedText);
	es->CurrLine = es->DeletedCurrLine;
	es->CurrCol = es->DeletedCurrCol;
	EDIT_InsertText(hwnd, text, es->DeletedLength);
	GlobalUnlock(es->hDeletedText);
	EDIT_ClearDeletedText(hwnd);

	es->SelBegLine = es->CurrLine;
	es->SelBegCol = es->CurrCol;
	EDIT_GetLineCol(hwnd, (int)((CurrChar + es->DeletedLength) - text), 
			&(es->CurrLine), &(es->CurrCol));
	es->WndRow = es->CurrLine - es->wtop;
	es->WndCol = EDIT_StrLength(hwnd, EDIT_TextLine(hwnd, es->CurrLine),
				     es->CurrCol, 0) - es->wleft;
	es->SelEndLine = es->CurrLine;
	es->SelEndCol = es->CurrCol;

	InvalidateRect(hwnd, NULL, TRUE);
	UpdateWindow(hwnd);
	return 1;
    }
    else
	return 0;
}


/*********************************************************************
 *  EDIT_HeapAlloc
 *
 *  Allocate the specified number of bytes on the specified local heap.
 */

unsigned int EDIT_HeapAlloc(HWND hwnd, int bytes)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    unsigned int ret;
    ret = ((unsigned int)HEAP_Alloc((MDESC **)
				     *(LONG *)(wndPtr->wExtra + 2), 
				     GMEM_MOVEABLE, bytes) & 0xffff);
    if (ret == 0)
      printf("EDIT_HeapAlloc: Out of heap-memory\n");
    return ret;
}


/*********************************************************************
 *  EDIT_HeapAddr
 *
 *  Return the address of the memory pointed to by the handle.
 */

void *EDIT_HeapAddr(HWND hwnd, unsigned int handle)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);

    return ((void *)((handle) ? ((handle) | ((unsigned int)
		    (*(MDESC **)*(LONG *)(wndPtr->wExtra + 2))   
		     & 0xffff0000)) : 0));
}


/*********************************************************************
 *  EDIT_HeapReAlloc
 *
 *  Reallocate the memory pointed to by the handle.
 */

unsigned int EDIT_HeapReAlloc(HWND hwnd, unsigned int handle, int bytes)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);

    return ((unsigned int)HEAP_ReAlloc((MDESC **)
				       *(LONG *)(wndPtr->wExtra + 2), 
				       EDIT_HeapAddr(hwnd, handle),
				       bytes, GMEM_MOVEABLE) & 0xffff);
}


/*********************************************************************
 *  EDIT_HeapFree
 *
 *  Frees the memory pointed to by the handle.
 */

void EDIT_HeapFree(HWND hwnd, unsigned int handle)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);

    HEAP_Free((MDESC **)*(LONG *)(wndPtr->wExtra + 2), 
	      EDIT_HeapAddr(hwnd, handle));
}


/*********************************************************************
 *  EDIT_HeapSize
 *
 *  Return the size of the given object on the local heap.
 */

unsigned int EDIT_HeapSize(HWND hwnd, unsigned int handle)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);

    return HEAP_LocalSize((MDESC **)*(LONG *)(wndPtr->wExtra + 2), handle);
}


/*********************************************************************
 *  EM_SETHANDLE message function
 */

void EDIT_SetHandleMsg(HWND hwnd, WORD wParam)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));

    if (IsMultiLine())
    {
	es->hText = wParam;
	es->textlen = EDIT_HeapSize(hwnd, es->hText);
	es->wlines = 0;
	es->wtop = es->wleft = 0;
	es->CurrLine = es->CurrCol = 0;
	es->WndRow = es->WndCol = 0;
	es->TextChanged = FALSE;
	es->textwidth = 0;
	es->SelBegLine = es->SelBegCol = 0;
	es->SelEndLine = es->SelEndCol = 0;
	dprintf_edit(stddeb, "EDIT_SetHandleMsg: textlen=%d\n",
                                                es->textlen);

	EDIT_BuildTextPointers(hwnd);
	es->PaintBkgd = TRUE;
	InvalidateRect(hwnd, NULL, TRUE);
	UpdateWindow(hwnd);
    }
}


/*********************************************************************
 *  EM_SETTABSTOPS message function
 */

LONG EDIT_SetTabStopsMsg(HWND hwnd, WORD wParam, LONG lParam)
{
    unsigned short *tabstops;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));

    es->NumTabStops = wParam;
    if (wParam == 0)
	es->hTabStops = EDIT_HeapReAlloc(hwnd, es->hTabStops, 1);
    else if (wParam == 1)
    {
	es->hTabStops = EDIT_HeapReAlloc(hwnd, es->hTabStops, 1);
	tabstops = (unsigned short *)EDIT_HeapAddr(hwnd, es->hTabStops);
	*tabstops = (unsigned short)lParam;
    }
    else
    {
	es->hTabStops = EDIT_HeapReAlloc(hwnd, es->hTabStops, wParam);
	tabstops = (unsigned short *)EDIT_HeapAddr(hwnd, es->hTabStops);
	memcpy(tabstops, (unsigned short *)lParam, wParam);
    }
    return 0L;
}


/*********************************************************************
 *  EDIT_CopyToClipboard
 *
 *  Copy the specified text to the clipboard.
 */

void EDIT_CopyToClipboard(HWND hwnd)
{
    HANDLE hMem;
    char *lpMem;
    int i, len;
    char *bbl, *bel;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = 
	(EDITSTATE *)EDIT_HeapAddr(hwnd, (HANDLE)(*(wndPtr->wExtra)));

    bbl = EDIT_TextLine(hwnd, es->SelBegLine) + es->SelBegCol;
    bel = EDIT_TextLine(hwnd, es->SelEndLine) + es->SelEndCol;
    len = (int)(bel - bbl);

    hMem = GlobalAlloc(GHND, (DWORD)(len + 1));
    lpMem = GlobalLock(hMem);

    for (i = 0; i < len; i++)
	*lpMem++ = *bbl++;

    GlobalUnlock(hMem);
    OpenClipboard(hwnd);
    EmptyClipboard();
    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();
}


/*********************************************************************
 *  WM_PASTE message function
 */

void EDIT_PasteMsg(HWND hwnd)
{
    HANDLE hClipMem;
    char *lpClipMem;

    OpenClipboard(hwnd);
    if (!(hClipMem = GetClipboardData(CF_TEXT)))
    {
	/* no text in clipboard */
	CloseClipboard();
	return;
    }
    lpClipMem = GlobalLock(hClipMem);
    EDIT_InsertText(hwnd, lpClipMem, strlen(lpClipMem));
    GlobalUnlock(hClipMem);
    CloseClipboard();
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
}


/*********************************************************************
 *  Utility functions
 */

void swap(int *a, int *b)
{
    int x;

    x = *a;
    *a = *b;
    *b = x;
}

