/*
 * Edit control
 *
 * Copyright  David W. Metcalfe, 1994
 *
 * Release 1, April 1994
 */

static char Copyright[] = "Copyright  David W. Metcalfe, 1994";

#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "win.h"
#include "class.h"
#include "user.h"

/*
#define DEBUG_EDIT
*/

#define NOTIFY_PARENT(hWndCntrl, wNotifyCode) \
	SendMessage(GetParent(hWndCntrl), WM_COMMAND, \
		 GetDlgCtrlID(hWndCntrl), MAKELPARAM(hWndCntrl, wNotifyCode));

#define MAXTEXTLEN 32000   /* maximum text buffer length */
#define EDITLEN     1024   /* starting length for multi-line control */
#define ENTRYLEN     256   /* starting length for single line control */
#define GROWLENGTH    64   /* buffers grow by this much */

#define HSCROLLDIM (ClientWidth(wndPtr) / 3)
                           /* "line" dimension for horizontal scroll */

#define EDIT_HEAP_ALLOC(size)          USER_HEAP_ALLOC(GMEM_MOVEABLE,size)
#define EDIT_HEAP_REALLOC(handle,size) USER_HEAP_REALLOC(handle,size,\
							 GMEM_MOVEABLE)
#define EDIT_HEAP_ADDR(handle)         USER_HEAP_ADDR(handle)
#define EDIT_HEAP_FREE(handle)         USER_HEAP_FREE(handle)

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
} EDITSTATE;


#define ClientWidth(wndPtr) (wndPtr->rectClient.right - \
			     wndPtr->rectClient.left)
#define ClientHeight(wndPtr, es) ((wndPtr->rectClient.bottom - \
				   wndPtr->rectClient.top) / es->txtht)
#define EditBufLen(wndPtr) (wndPtr->dwStyle & ES_MULTILINE \
			    ? EDITLEN : ENTRYLEN)
#define CurrChar (EDIT_TextLine(hwnd, es->CurrLine) + es->CurrCol)
#define SelMarked(es) (es->SelBegLine != -1 && es->SelBegCol != -1 && \
		       es->SelEndLine != -1 && es->SelEndCol != -1)

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
static BOOL Print = FALSE;


LONG EditWndProc(HWND hWnd, WORD uMsg, WORD wParam, LONG lParam);
long EDIT_CreateMsg(HWND hwnd, LONG lParam);
void EDIT_ClearTextPointers(HWND hwnd);
void EDIT_BuildTextPointers(HWND hwnd);
void EDIT_ModTextPointers(HWND hwnd, int lineno, int var);
void EDIT_PaintMsg(HWND hwnd);
HANDLE EDIT_GetTextLine(HWND hwnd, int selection);
char *EDIT_TextLine(HWND hwnd, int sel);
int EDIT_LineLength(EDITSTATE *es, char *str, int len);
void EDIT_WriteTextLine(HWND hwnd, RECT *rc, int y);
void EDIT_WriteText(HWND hwnd, char *lp, int off, int len, int row, 
		    int col, RECT *rc, BOOL blank, BOOL reverse);
HANDLE EDIT_GetStr(EDITSTATE *es, char *lp, int off, int len, int *diff);
void EDIT_CharMsg(HWND hwnd, WORD wParam);
void EDIT_KeyTyped(HWND hwnd, short ch);
int EDIT_CharWidth(EDITSTATE *es, short ch);
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
void EDIT_SetSelMsg(HWND hwnd, LONG lParam);
void EDIT_GetLineCol(HWND hwnd, int off, int *line, int *col);
void EDIT_DeleteSel(HWND hwnd);
void EDIT_ClearSel(HWND hwnd);
int EDIT_TextLineNumber(HWND hwnd, char *lp);
void EDIT_SetAnchor(HWND hwnd, int row, int col);
void EDIT_ExtendSel(HWND hwnd, int x, int y);
void EDIT_StopMarking(HWND hwnd);
LONG EDIT_GetLineMsg(HWND hwnd, WORD wParam, LONG lParam);
LONG EDIT_GetSelMsg(HWND hwnd);
LONG EDIT_LineFromCharMsg(HWND hwnd, WORD wParam);
LONG EDIT_LineIndexMsg(HWND hwnd, WORD wParam);
void swap(int *a, int *b);


LONG EditWndProc(HWND hwnd, WORD uMsg, WORD wParam, LONG lParam)
{
    LONG lResult = 0L;
    HDC hdc;
    char *textPtr;
    int len;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));

    switch (uMsg) {
    case EM_CANUNDO:
	/* cannot process undo message */
	lResult = 0L;
	break;
	
    case EM_FMTLINES:
	printf("edit: cannot process EM_FMTLINES message\n");
	lResult = 0L;
	break;

    case EM_GETFIRSTVISIBLELINE:
	lResult = es->wtop;
	break;

    case EM_GETHANDLE:
	printf("edit: cannot process EM_GETHANDLE message\n");
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
	printf("edit: cannot process EM_GETPASSWORDCHAR message\n");
	break;

    case EM_GETRECT:
	GetWindowRect(hwnd, (LPRECT)lParam);
	break;

    case EM_GETSEL:
	lResult = EDIT_GetSelMsg(hwnd);
	break;

    case EM_GETWORDBREAKPROC:
	printf("edit: cannot process EM_GETWORDBREAKPROC message\n");
	break;

    case EM_LIMITTEXT:
	es->MaxTextLen = wParam;
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
	printf("edit: cannot process EM_LINELENGTH message\n");
	break;

    case EM_LINESCROLL:
	printf("edit: cannot process EM_LINESCROLL message\n");
	break;

    case EM_REPLACESEL:
	printf("edit: cannot process EM_REPLACESEL message\n");
	break;

    case EM_SETHANDLE:
	printf("edit: cannot process EM_SETHANDLE message\n");
	break;

    case EM_SETMODIFY:
	es->TextChanged = wParam;
	break;

    case EM_SETPASSWORDCHAR:
	printf("edit: cannot process EM_SETPASSWORDCHAR message\n");
	break;

    case EM_SETREADONLY:
	printf("edit: cannot process EM_SETREADONLY message\n");
	break;

    case EM_SETRECT:
    case EM_SETRECTNP:
	printf("edit: cannot process EM_SETRECT(NP) message\n");
	break;

    case EM_SETSEL:
	HideCaret(hwnd);
	EDIT_SetSelMsg(hwnd, lParam);
	SetCaretPos(es->WndCol, es->WndRow * es->txtht);
	ShowCaret(hwnd);
	break;

    case EM_SETTABSTOPS:
	printf("edit: cannot process EM_SETTABSTOPS message\n");
	break;

    case EM_SETWORDBREAKPROC:
	printf("edit: cannot process EM_SETWORDBREAKPROC message\n");
	break;

    case WM_CHAR:
	EDIT_CharMsg(hwnd, wParam);
	break;

    case WM_CREATE:
	lResult = EDIT_CreateMsg(hwnd, lParam);
	break;

    case WM_DESTROY:
	EDIT_HEAP_FREE(es->hTextPtrs);
	EDIT_HEAP_FREE(es->hCharWidths);
	EDIT_HEAP_FREE((HANDLE)(*(wndPtr->wExtra)));
	break;

    case WM_ENABLE:
	InvalidateRect(hwnd, NULL, FALSE);
	break;

    case WM_GETTEXT:
	textPtr = (LPSTR)EDIT_HEAP_ADDR(es->hText);
	if ((int)wParam > (len = strlen(textPtr)))
	{
	    strcpy((char *)lParam, textPtr);
	    lResult = (DWORD)len;
	}
	else
	    lResult = 0L;
	break;

    case WM_GETTEXTLENGTH:
	textPtr = (LPSTR)EDIT_HEAP_ADDR(es->hText);
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

    case WM_PAINT:
	EDIT_PaintMsg(hwnd);
	break;

    case WM_SETFOCUS:
	es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));
	CreateCaret(hwnd, 0, 2, es->txtht);
	SetCaretPos(es->WndCol, es->WndRow * es->txtht);
	ShowCaret(hwnd);
	NOTIFY_PARENT(hwnd, EN_SETFOCUS);
	break;

    case WM_SETFONT:
	break;

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

    GlobalUnlock(hwnd);
    return lResult;
}


/*********************************************************************
 *  WM_CREATE message function
 */

long EDIT_CreateMsg(HWND hwnd, LONG lParam)
{
    HDC hdc;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es;
    CLASS *classPtr;
    short *charWidths;
    TEXTMETRIC tm;
    char *text;
    unsigned int *textPtrs;

    /* allocate space for state variable structure */
    (HANDLE)(*(wndPtr->wExtra)) = 
	EDIT_HEAP_ALLOC(sizeof(EDITSTATE));
    es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));
    es->hTextPtrs = EDIT_HEAP_ALLOC(sizeof(int));
    textPtrs = (unsigned int *)EDIT_HEAP_ADDR(es->hTextPtrs);
    es->hCharWidths = EDIT_HEAP_ALLOC(256 * sizeof(short));

    /* initialize state variable structure */
    /* --- char width array */
    hdc = GetDC(hwnd);
    charWidths = (short *)EDIT_HEAP_ADDR(es->hCharWidths);
    memset(charWidths, 0, 256 * sizeof(short));
    GetCharWidth(hdc, 0, 255, charWidths);

    /* --- text buffer */
    es->MaxTextLen = MAXTEXTLEN + 1;
    if (!(wndPtr->hText))
    {
	es->textlen = EditBufLen(wndPtr);
	es->hText = EDIT_HEAP_ALLOC(EditBufLen(wndPtr) + 2);
	text = (LPSTR)EDIT_HEAP_ADDR(es->hText);
	memset(text, 0, es->textlen + 2);
	EDIT_ClearTextPointers(hwnd);
    }
    else
    {
	es->hText = wndPtr->hText;
	wndPtr->hText = 0;
	es->textlen = GetWindowTextLength(hwnd) + 1;
	EDIT_BuildTextPointers(hwnd);
    }

    /* --- other structure variables */
    GetTextMetrics(hdc, &tm);
    es->txtht = tm.tmHeight + tm.tmExternalLeading;
    es->wlines = 0;
    es->wtop = es->wleft = 0;
    es->CurrCol = es->CurrLine = 0;
    es->WndCol = es->WndRow = 0;
    es->TextChanged = FALSE;
    es->textwidth = 0;
    es->SelBegLine = es->SelBegCol = -1;
    es->SelEndLine = es->SelEndCol = -1;

    /* allocate space for a line full of blanks to speed up */
    /* line filling */
    es->hBlankLine = EDIT_HEAP_ALLOC((ClientWidth(wndPtr) / 
				      charWidths[32]) + 2); 
    text = EDIT_HEAP_ADDR(es->hBlankLine);
    memset(text, ' ', (ClientWidth(wndPtr) / charWidths[32]) + 2);

    /* set up text cursor for edit class */
    CLASS_FindClassByName("EDIT", &classPtr);
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
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));
    
    es->hTextPtrs = EDIT_HEAP_REALLOC(es->hTextPtrs, sizeof(int));
    textPtrs = (unsigned int *)EDIT_HEAP_ADDR(es->hTextPtrs);
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
    unsigned int off, len, temp;
    EDITSTATE *es;
    unsigned int *textPtrs;
    short *charWidths;

    es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));
    text = (char *)EDIT_HEAP_ADDR(es->hText);
    textPtrs = (unsigned int *)EDIT_HEAP_ADDR(es->hTextPtrs);
    charWidths = (short *)EDIT_HEAP_ADDR(es->hCharWidths);

    es->textwidth = es->wlines = 0;
    cp = text;

    /* advance through text buffer */
    while (*cp)
    {
	/* increase size of text pointer array */ 
	if (incrs == INITLINES)
	{
	    incrs = 0;
	    es->hTextPtrs = EDIT_HEAP_REALLOC(es->hTextPtrs,
			      (es->wlines + INITLINES) * sizeof(int));
	    textPtrs = (unsigned int *)EDIT_HEAP_ADDR(es->hTextPtrs);
	}
	off = (unsigned int)(cp - text);     /* offset of beginning of line */
	*(textPtrs + es->wlines) = off;
	es->wlines++;
	incrs++;
	len = 0;
	
	/* advance through current line */
	while (*cp && *cp != '\n')
	{
	    len += charWidths[*cp];          /* width of line in pixels */
	    cp++;
	}
	es->textwidth = max(es->textwidth, len);
	if (*cp)
	    cp++;                            /* skip '\n' */
    }
    off = (unsigned int)(cp - text);     /* offset of beginning of line */
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
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));
    unsigned int *textPtrs = (unsigned int *)EDIT_HEAP_ADDR(es->hTextPtrs);

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
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));

    hdc = BeginPaint(hwnd, &ps);
    rc = ps.rcPaint;

#ifdef DEBUG_EDIT
    printf("WM_PAINT: rc=(%d,%d), (%d,%d)\n", rc.left, rc.top, 
	   rc.right, rc.bottom);
#endif

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

#ifdef DEBUG_EDIT
    printf("GetTextLine %d\n", selection);
#endif
    cp = cp1 = EDIT_TextLine(hwnd, selection);
    /* advance through line */
    while (*cp && *cp != '\n')
    {
	len++;
	cp++;
    }

    /* store selected line and return handle */
    hLine = EDIT_HEAP_ALLOC(len + 6);
    line = (char *)EDIT_HEAP_ADDR(hLine);
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
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));
    char *text = (char *)EDIT_HEAP_ADDR(es->hText);
    unsigned int *textPtrs = (unsigned int *)EDIT_HEAP_ADDR(es->hTextPtrs);

    return (text + *(textPtrs + sel));
}
    

/*********************************************************************
 *  EDIT_LineLength
 *
 *  Return length of line _str_ of length _len_ characters in pixels.
 */

int EDIT_LineLength(EDITSTATE *es, char *str, int len)
{
    int i, plen = 0;
    short *charWidths = (short *)EDIT_HEAP_ADDR(es->hCharWidths);

    for (i = 0; i < len; i++)
	plen += charWidths[*(str + i)];

#ifdef DEBUG_EDIT
    printf("EDIT_LineLength: returning %d\n", plen);
#endif
    return plen;
}


/*********************************************************************
 *  EDIT_WriteTextLine
 *
 *  Write the line of text at offset _y_ in text buffer to a window.
 */

void EDIT_WriteTextLine(HWND hwnd, RECT *rect, int y)
{
    int len = 0;
    unsigned char line[200];
    HANDLE hLine;
    unsigned char *lp;
    int lnlen, lnlen1;
    int col, off = 0;
    int sbl, sel, sbc, sec;
    RECT rc;

    BOOL trunc = FALSE;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));

    /* initialize rectangle if NULL, else copy */
    if (rect)
	CopyRect(&rc, rect);
    else
	GetClientRect(hwnd, &rc);

#ifdef DEBUG_EDIT
    printf("WriteTextLine %d\n", y);
#endif

    /* make sure y is inside the window */
    if (y < es->wtop || y > (es->wtop + ClientHeight(wndPtr, es)))
    {
#ifdef DEBUG_EDIT
	printf("EDIT_WriteTextLine: y (%d) is not a displayed line\n", y);
#endif
	return;
    }

    /* make sure rectangle is within window */
    if (rc.left >= ClientWidth(wndPtr) - 1)
    {
#ifdef DEBUG_EDIT
	printf("EDIT_WriteTextLine: rc.left (%d) is greater than right edge\n",
	       rc.left);
#endif
	return;
    }
    if (rc.right <= 0)
    {
#ifdef DEBUG_EDIT
	printf("EDIT_WriteTextLine: rc.right (%d) is less than left edge\n",
	       rc.right);
#endif
	return;
    }
    if (y - es->wtop < (rc.top / es->txtht) || 
	y - es->wtop > (rc.bottom / es->txtht))
    {
#ifdef DEBUG_EDIT
	printf("EDIT_WriteTextLine: y (%d) is outside window\n", y);
#endif
	return;
    }

    /* get the text and length of line */
    if ((hLine = EDIT_GetTextLine(hwnd, y)) == 0)
	return;
    lp = (unsigned char *)EDIT_HEAP_ADDR(hLine);
    lnlen = EDIT_LineLength(es, lp, strlen(lp));
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
	    col = EDIT_LineLength(es, lp, sbc);
	    if (col > (es->wleft + rc.left))
	    {
		len = min(col - off, rc.right - off);
		EDIT_WriteText(hwnd, lp, off, len, y - es->wtop, 
			       rc.left, &rc, FALSE, FALSE);
		off = col;
	    }
	    if (y == sel)
	    {
		col = EDIT_LineLength(es, lp, sec);
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
	    col = EDIT_LineLength(es, lp, sec);
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
	      
    EDIT_HEAP_FREE(hLine);
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
    char *str, *blanks;
    int diff, num_spaces;
    HRGN hrgnClip;
    COLORREF oldTextColor, oldBkgdColor;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));
    short *charWidths = (short *)EDIT_HEAP_ADDR(es->hCharWidths);

#ifdef DEBUG_EDIT
    printf("EDIT_WriteText lp=%s, off=%d, len=%d, row=%d, col=%d, reverse=%d\n", lp, off, len, row, col, reverse);
#endif

    hdc = GetDC(hwnd);
    hStr = EDIT_GetStr(es, lp, off, len, &diff);
    str = (char *)EDIT_HEAP_ADDR(hStr);
    hrgnClip = CreateRectRgnIndirect(rc);
    SelectClipRgn(hdc, hrgnClip);

    SendMessage(GetParent(hwnd), WM_CTLCOLOR, (WORD)hdc,
		MAKELPARAM(hwnd, CTLCOLOR_EDIT));

    if (reverse)
    {
	oldBkgdColor = GetBkColor(hdc);
	oldTextColor = GetTextColor(hdc);
	SetBkColor(hdc, oldTextColor);
	SetTextColor(hdc, oldBkgdColor);
    }

    TextOut(hdc, col - diff, row * es->txtht, str, strlen(str));

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
	    blanks = EDIT_HEAP_ADDR(es->hBlankLine);
	    num_spaces = (rc->right - col - len) / charWidths[32];
	    TextOut(hdc, col + len, row * es->txtht, blanks, num_spaces);
	}
    }

    EDIT_HEAP_FREE(hStr);
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

HANDLE EDIT_GetStr(EDITSTATE *es, char *lp, int off, int len, int *diff)
{
    HANDLE hStr;
    char *str;
    int ch = 0, i = 0, j, tmp;
    int ch1;
    short *charWidths = (short *)EDIT_HEAP_ADDR(es->hCharWidths);

#ifdef DEBUG_EDIT
    printf("EDIT_GetStr %s %d %d\n", lp, off, len);
#endif

    while (i < off)
    {
	i += charWidths[*(lp + ch)];
	ch++;
    }

    /* if stepped past _off_, go back a character */
    if (i - off)
	i -= charWidths[*(lp + --ch)];
    *diff = off - i;
    ch1 = ch;

    while (i < len + off)
    {
	i += charWidths[*(lp + ch)];
	ch++;
    }
    
    hStr = EDIT_HEAP_ALLOC(ch - ch1 + 3);
    str = (char *)EDIT_HEAP_ADDR(hStr);
    for (i = ch1, j = 0; i < ch; i++, j++)
	str[j] = lp[i];
    str[++j] = '\0';
#ifdef DEBUG_EDIT
    printf("EDIT_GetStr: returning %s\n", str);
#endif
    return hStr;
}


/*********************************************************************
 *  WM_CHAR message function
 */

void EDIT_CharMsg(HWND hwnd, WORD wParam)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));

#ifdef DEBUG_EDIT
    printf("EDIT_CharMsg: wParam=%c\n", (char)wParam);
#endif

    switch (wParam)
    {
    case '\r':
    case '\n':
	if (!IsMultiLine())
	    break;
	wParam = '\n';
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
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));
    char *text = (char *)EDIT_HEAP_ADDR(es->hText);
    char *currchar = CurrChar;
    RECT rc;
    BOOL FullPaint = FALSE;

#ifdef DEBUG_EDIT
    printf("EDIT_KeyTyped: ch=%c\n", (char)ch);
#endif

    /* delete selected text (if any) */
    if (SelMarked(es))
	EDIT_DeleteSel(hwnd);

    /* test for typing at end of maximum buffer size */
    if (currchar == text + es->MaxTextLen)
    {
	NOTIFY_PARENT(hwnd, EN_ERRSPACE);
	return;
    }

    if (*currchar == '\0')
    {
	/* insert a newline at end of text */
	*currchar = '\n';
	*(currchar + 1) = '\0';
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
	es->hText = EDIT_HEAP_REALLOC(es->hText, es->textlen + 2);
	if (!es->hText)
	    NOTIFY_PARENT(hwnd, EN_ERRSPACE);
	text = (char *)EDIT_HEAP_ADDR(es->hText);
	text[es->textlen - 1] = '\0';
	currchar = CurrChar;
    }
    /* make space for new character and put char in buffer */
    memmove(currchar + 1, currchar, strlen(currchar) + 1);
    *currchar = ch;
    EDIT_ModTextPointers(hwnd, es->CurrLine + 1, 1);
    es->TextChanged = TRUE;
    NOTIFY_PARENT(hwnd, EN_UPDATE);

    /* re-adjust textwidth, if necessary, and redraw line */
    HideCaret(hwnd);
    if (IsMultiLine() && es->wlines > 1)
    {
	es->textwidth = max(es->textwidth,
		    EDIT_LineLength(es, EDIT_TextLine(hwnd, es->CurrLine),
		    (int)(EDIT_TextLine(hwnd, es->CurrLine + 1) -
			  EDIT_TextLine(hwnd, es->CurrLine))));
    }
    else
	es->textwidth = max(es->textwidth,
			    EDIT_LineLength(es, text, strlen(text)));
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
    if (es->WndCol >= ClientWidth(wndPtr) - EDIT_CharWidth(es, ch))
    {
	/* TODO:- Word wrap to be handled here */

/*	if (!(currchar == text + es->MaxTextLen - 2)) */
	    EDIT_KeyHScroll(hwnd, SB_LINEDOWN);
    }
    es->WndCol += EDIT_CharWidth(es, ch);
    es->CurrCol++;
    SetCaretPos(es->WndCol, es->WndRow * es->txtht);
    ShowCaret(hwnd);
    NOTIFY_PARENT(hwnd, EN_CHANGE);
}


/*********************************************************************
 *  EDIT_CharWidth
 *
 *  Return the width of the given character in pixels.
 */

int EDIT_CharWidth(EDITSTATE *es, short ch)
{
    short *charWidths = (short *)EDIT_HEAP_ADDR(es->hCharWidths);

    return (charWidths[ch]);
}


/*********************************************************************
 *  EDIT_Forward
 *
 *  Cursor right key: move right one character position.
 */

void EDIT_Forward(HWND hwnd)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));
    char *text = (char *)EDIT_HEAP_ADDR(es->hText);
    char *cc = CurrChar + 1;

    if (*cc == '\0')
	return;

    if (*CurrChar == '\n')
    {
	EDIT_Home(hwnd);
	EDIT_Downward(hwnd);
    }
    else
    {
	es->WndCol += EDIT_CharWidth(es, *CurrChar);
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
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));

#ifdef DEBUG_EDIT
    printf("EDIT_Downward: WndRow=%d, wtop=%d, wlines=%d\n", es->WndRow, es->wtop, es->wlines);
#endif

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
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));

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
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));
    char *text = (char *)EDIT_HEAP_ADDR(es->hText);

    if (es->CurrCol)
    {
	--es->CurrCol;
	es->WndCol -= EDIT_CharWidth(es, *CurrChar);
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
    RECT rc;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));
    char *text = (char *)EDIT_HEAP_ADDR(es->hText);

    while (*CurrChar && *CurrChar != '\n')
    {
	es->WndCol += EDIT_CharWidth(es, *CurrChar);
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
    RECT rc;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));

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
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));
    char currpel;

    char *cp = EDIT_TextLine(hwnd, es->CurrLine);
    char *cp1 = strchr(cp, '\n');
    int len = cp1 ? (int)(cp1 - cp) : 0;

    es->CurrCol = min(len, es->CurrCol);
    es->WndCol = min(EDIT_LineLength(es, cp, len) - es->wleft, es->WndCol);
    currpel = EDIT_LineLength(es, cp, es->CurrCol);

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
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));

#ifdef DEBUG_EDIT
    printf("EDIT_KeyDownMsg: key=%x\n", wParam);
#endif

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
    RECT rc;
    int hscrollpos;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));

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
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));

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
    RECT rc;
    int vscrollpos;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));

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
    RECT rc;
    int vscrollpos;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));

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
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));

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
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));

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
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));
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
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));

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
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));

#ifdef DEBUG_EDIT
    printf("EDIT_VScrollLine: direction=%d\n", opt);
#endif

    if (opt == SB_LINEDOWN)
    {
	/* move down one line */
	if (es->wtop + ClientHeight(wndPtr, es) >= es->wlines)
	    return;
	es->wtop++;
	printf("Scroll line down: wtop=%d\n", es->wtop);
    }
    else
    {
	/* move up one line */
	if (es->wtop == 0)
	    return;
	--es->wtop;
	printf("Scroll line up: wtop=%d\n", es->wtop);
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
    RECT rc;
    int vscrollpos;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));

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
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));

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
    RECT rc;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));

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
    char *cp, *cp1;
    int len;
    BOOL end = FALSE;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));

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
    cp1 = strchr(cp, '\n');
    len = cp1 ? (int)(cp1 - cp) : 0;

    es->WndCol = LOWORD(lParam);
    if (es->WndCol > EDIT_LineLength(es, cp, len) - es->wleft || end)
	es->WndCol = EDIT_LineLength(es, cp, len) - es->wleft;
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
    int ch = 0, i = 0;
    char *text;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));
    short *charWidths = (short *)EDIT_HEAP_ADDR(es->hCharWidths);

#ifdef DEBUG_EDIT
    printf("EDIT_PixelToChar: row=%d, pixel=%d\n", row, *pixel);
#endif

    text = EDIT_TextLine(hwnd, row);
    while (i < *pixel)
    {
	i += charWidths[*(text + ch)];
	ch++;
    }

    /* if stepped past _pixel_, go back a character */
    if (i - *pixel)
	i -= charWidths[*(text + ch)];
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
    RECT rc;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));

    if (strlen((char *)lParam) <= es->MaxTextLen)
    {
	len = strlen((char *)lParam);
	EDIT_ClearText(hwnd);
	es->textlen = len;
	es->hText = EDIT_HEAP_REALLOC(es->hText, len + 3);
	text = EDIT_HEAP_ADDR(es->hText);
	strcpy(text, (char *)lParam);
	text[len] = '\n';
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
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));
    unsigned int blen = EditBufLen(wndPtr) + 2;
    char *text;

    es->hText = EDIT_HEAP_REALLOC(es->hText, blen);
    text = EDIT_HEAP_ADDR(es->hText);
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

void EDIT_SetSelMsg(HWND hwnd, LONG lParam)
{
    int so, eo;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));

    so = LOWORD(lParam);
    eo = HIWORD(lParam);
    if (so > eo)
	swap(&so, &eo);

    EDIT_GetLineCol(hwnd, so, &(es->SelBegLine), &(es->SelBegCol));
    EDIT_GetLineCol(hwnd, eo, &(es->SelEndLine), &(es->SelEndCol));

    es->CurrLine = es->SelEndLine;
    es->CurrCol = es->SelEndCol;
    es->WndRow = es->SelEndLine - es->wtop;
    if (es->WndRow < 0)
    {
	es->wtop = es->SelEndLine;
	es->WndRow = 0;
    }
    es->WndCol = EDIT_LineLength(es, EDIT_TextLine(hwnd, es->SelEndLine), 
				 es->SelEndCol) - es->wleft;

    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
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
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));
    char *text = (char *)EDIT_HEAP_ADDR(es->hText);
    unsigned int *textPtrs = (unsigned int *)EDIT_HEAP_ADDR(es->hTextPtrs);

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
    if (*(text + *col) == '\0')
	(*col)--;
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
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));
    char *text = (char *)EDIT_HEAP_ADDR(es->hText);

    if (SelMarked(es))
    {
	bbl = EDIT_TextLine(hwnd, es->SelBegLine) + es->SelBegCol;
	bel = EDIT_TextLine(hwnd, es->SelEndLine) + es->SelEndCol;
	len = (int)(bel - bbl);
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
	es->WndCol = EDIT_LineLength(es, bbl - es->SelBegCol, 
				     es->SelBegCol) - es->wleft;

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
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));

    es->SelBegLine = es->SelBegCol = -1;
    es->SelEndLine = es->SelEndCol = -1;

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
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));
    char *text = (char *)EDIT_HEAP_ADDR(es->hText);
    unsigned int *textPtrs = (unsigned int *)EDIT_HEAP_ADDR(es->hTextPtrs);

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
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));

    EDIT_ClearSel(hwnd);
    es->SelBegLine = es->SelEndLine = row;
    es->SelBegCol = es->SelEndCol = col;
    InvalidateRect(hwnd, NULL, FALSE);
    UpdateWindow(hwnd);
}


/*********************************************************************
 *  EDIT_ExtendSel
 *
 *  Extend selection to the given screen co-ordinates.
 */

void EDIT_ExtendSel(HWND hwnd, int x, int y)
{
    int bbl, bel;
    int ptop, pbot;
    char *cp, *cp1;
    int len;
    BOOL end = FALSE;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));

#ifdef DEBUG_EDIT
    printf("EDIT_ExtendSel: x=%d, y=%d\n", x, y);
#endif

    ptop = min(es->SelBegLine, es->SelEndLine);
    pbot = max(es->SelBegLine, es->SelEndLine);
    cp = EDIT_TextLine(hwnd, es->wtop + y / es->txtht);
    cp1 = strchr(cp, '\n');
    len = cp1 ? (int)(cp1 - cp) : 0;

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
    if (es->WndCol > EDIT_LineLength(es, cp, len) - es->wleft || end)
	es->WndCol = EDIT_LineLength(es, cp, len) - es->wleft;
    es->CurrCol = EDIT_PixelToChar(hwnd, es->CurrLine, &(es->WndCol));
    es->SelEndCol = es->CurrCol;

    bbl = min(es->SelBegLine, es->SelEndLine);
    bel = max(es->SelBegLine, es->SelEndLine);
    while (ptop < bbl)
    {
	EDIT_WriteTextLine(hwnd, NULL, ptop);
	ptop++;
    }
    for (y = bbl; y <= bel; y++)
	EDIT_WriteTextLine(hwnd, NULL, y);
    while (pbot > bel)
    {
	EDIT_WriteTextLine(hwnd, NULL, pbot);
	--pbot;
    }
}


/*********************************************************************
 *  EDIT_StopMarking
 *
 *  Stop text marking (selection).
 */

void EDIT_StopMarking(HWND hwnd)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));

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
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));

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
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));
    unsigned int *textPtrs = (unsigned int *)EDIT_HEAP_ADDR(es->hTextPtrs);

    so = *(textPtrs + es->SelBegLine) + es->SelBegCol;
    eo = *(textPtrs + es->SelEndLine) + es->SelEndCol;

    return MAKELONG(so, eo);
}


/*********************************************************************
 *  EM_LINEFROMCHAR message function
 */

LONG EDIT_LineFromCharMsg(HWND hwnd, WORD wParam)
{
    int row, col;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));

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
    EDITSTATE *es = (EDITSTATE *)EDIT_HEAP_ADDR((HANDLE)(*(wndPtr->wExtra)));
    unsigned int *textPtrs = (unsigned int *)EDIT_HEAP_ADDR(es->hTextPtrs);

    if (wParam == (WORD)-1)
	wParam = es->CurrLine;

    return (LONG)(*(textPtrs + wParam));
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

