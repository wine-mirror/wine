/*
 *	Edit control
 *
 *	Copyright  David W. Metcalfe, 1994
 *	Copyright  William Magro, 1995, 1996
 *	Copyright  Frans van Dorsselaer, 1996
 *
 */

/*
 *	UNDER CONSTRUCTION, please read EDIT.TODO
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "win.h"
#include "local.h"
#include "stackframe.h"
#include "stddebug.h"
#include "debug.h"
#include "xmalloc.h"
#include "callback.h"

#define BUFLIMIT_MULTI		65535	/* maximum text buffer length */
#define BUFLIMIT_SINGLE		32767	/* maximum text buffer length */
#define BUFSTART_MULTI		1024	/* starting length for multi-line control */
#define BUFSTART_SINGLE		256	/* starting length for single line control */
#define GROWLENGTH		64	/* buffers grow by this much */
#define HSCROLL_FRACTION	3	/* scroll window by 1/3 width */

typedef enum
{
	END_0 = 0,
	END_DELIMIT,
	END_NONE,
	END_HARD,
	END_SOFT,
} LINE_END;

typedef struct {
	int offset;
	int length;
	LINE_END ending;
} LINEDEF;

typedef struct
{
	int TextWidth;		/* width of the widest line in pixels */
	HLOCAL hBuf;
	char *text;
	HFONT hFont;
	LINEDEF *LineDefs;
	int XOffset;		/* possitive offset of the viewport in pixels */
	int FirstVisibleLine;
	int LineCount;
	int LineHeight;		/* height of a screen line in pixels */
	int AveCharWidth;	/* average character width in pixels */
	unsigned int BufLimit;
	unsigned int BufSize;
	BOOL TextChanged;
	BOOL Redraw;
	int SelStart;		/* offset of selection start, == SelEnd if no selection */
	int SelEnd;		/* offset of selection end == current caret position */
	int NumTabStops;
	LPINT TabStops;
	EDITWORDBREAKPROC WordBreakProc;
	char PasswordChar;
} EDITSTATE;


#define SWAP_INT(x,y) do { int temp = (x); (x) = (y); (y) = temp; } while(0)
#define ORDER_INT(x,y) do { if ((y) < (x)) SWAP_INT((x),(y)); } while(0)

/* macros to access window styles */
#define IsMultiLine(wndPtr) ((wndPtr)->dwStyle & ES_MULTILINE)
#define IsVScrollBar(wndPtr) ((wndPtr)->dwStyle & WS_VSCROLL)
#define IsHScrollBar(wndPtr) ((wndPtr)->dwStyle & WS_HSCROLL)
#define IsReadOnly(wndPtr) ((wndPtr)->dwStyle & ES_READONLY)
#define IsWordWrap(wndPtr) (((wndPtr)->dwStyle & ES_AUTOHSCROLL) == 0)

#define EDITSTATEPTR(wndPtr) (*(EDITSTATE **)((wndPtr)->wExtra))

#ifdef WINELIB32
#define EDIT_SEND_CTLCOLOR(wndPtr,hdc) \
		SendMessage((wndPtr)->parent->hwndSelf, WM_CTLCOLOREDIT, \
				(WPARAM)(hdc), (LPARAM)(wndPtr)->hwndSelf)
#define EDIT_NOTIFY_PARENT(wndPtr, wNotifyCode) \
		SendMessage((wndPtr)->parent->hwndSelf, WM_COMMAND, \
				MAKEWPARAM((wndPtr)->wIDmenu, wNotifyCode), \
				(LPARAM)(wndPtr)->hwndSelf )
#define DPRINTF_EDIT_MSG(str) \
		dprintf_edit(stddeb, \
			"edit: " str ": hwnd=%08x, wParam=%08x, lParam=%08lx\n", \
			(UINT)hwnd, (UINT)wParam, (DWORD)lParam)
#else
#define EDIT_SEND_CTLCOLOR(wndPtr,hdc) \
		SendMessage((wndPtr)->parent->hwndSelf, WM_CTLCOLOR, \
				(WPARAM)(hdc), MAKELPARAM((wndPtr)->hwndSelf, CTLCOLOR_EDIT))
#define EDIT_NOTIFY_PARENT(wndPtr, wNotifyCode) \
		SendMessage((wndPtr)->parent->hwndSelf, WM_COMMAND, \
				(wndPtr)->wIDmenu, \
				MAKELPARAM((wndPtr)->hwndSelf, wNotifyCode))
#define DPRINTF_EDIT_MSG(str) \
		dprintf_edit(stddeb, \
			"edit: " str ": hwnd=%04x, wParam=%04x, lParam=%08lx\n", \
			(UINT)hwnd, (UINT)wParam, (DWORD)lParam)
#endif

/*********************************************************************
 *
 *	Declarations
 *
 *	Files like these should really be kept in alphabetical order.
 *
 */
LRESULT EditWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

static void    EDIT_BuildLineDefs(WND *wndPtr);
static int     EDIT_CallWordBreakProc(WND *wndPtr, char *s, int index, int count, int action);
static int     EDIT_ColFromWndX(WND *wndPtr, int line, int x);
static void    EDIT_DelEnd(WND *wndPtr);
static void    EDIT_DelLeft(WND *wndPtr);
static void    EDIT_DelRight(WND *wndPtr);
static int     EDIT_GetAveCharWidth(WND *wndPtr);
static int     EDIT_GetLineHeight(WND *wndPtr);
static void    EDIT_GetLineRect(WND *wndPtr, int line, int scol, int ecol, LPRECT rc);
static char *  EDIT_GetPointer(WND *wndPtr);
static LRESULT EDIT_GetRect(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static BOOL    EDIT_GetRedraw(WND *wndPtr);
static int     EDIT_GetTextWidth(WND *wndPtr);
static int     EDIT_GetVisibleLineCount(WND *wndPtr);
static int     EDIT_GetWndWidth(WND *wndPtr);
static int     EDIT_GetXOffset(WND *wndPtr);
static int     EDIT_LineFromWndY(WND *wndPtr, int y);
static BOOL    EDIT_MakeFit(WND *wndPtr, int size);
static void    EDIT_MoveBackward(WND *wndPtr, BOOL extend);
static void    EDIT_MoveDownward(WND *wndPtr, BOOL extend);
static void    EDIT_MoveEnd(WND *wndPtr, BOOL extend);
static void    EDIT_MoveForward(WND *wndPtr, BOOL extend);
static void    EDIT_MoveHome(WND *wndPtr, BOOL extend);
static void    EDIT_MovePageDown(WND *wndPtr, BOOL extend);
static void    EDIT_MovePageUp(WND *wndPtr, BOOL extend);
static void    EDIT_MoveUpward(WND *wndPtr, BOOL extend);
static void    EDIT_MoveWordBackward(WND *wndPtr, BOOL extend);
static void    EDIT_MoveWordForward(WND *wndPtr, BOOL extend);
static void    EDIT_PaintLine(WND *wndPtr, HDC hdc, int line);
static int     EDIT_PaintText(WND *wndPtr, HDC hdc, int x, int y, int line, int col, int count, BOOL rev);
static void    EDIT_ReleasePointer(WND *wndPtr);
static LRESULT EDIT_ReplaceSel(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static void    EDIT_ScrollIntoView(WND *wndPtr);
static int     EDIT_WndXFromCol(WND *wndPtr, int line, int col);
static int     EDIT_WndYFromLine(WND *wndPtr, int line);
static int     EDIT_WordBreakProc(char *s, int index, int count, int action);

static LRESULT EDIT_EM_CanUndo(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_EM_EmptyUndoBuffer(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_EM_FmtLines(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_EM_GetFirstVisibleLine(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_EM_GetHandle(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_EM_GetLine(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_EM_GetLineCount(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_EM_GetModify(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_EM_GetPasswordChar(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_EM_GetRect(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_EM_GetSel(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_EM_GetThumb(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_EM_GetWordBreakProc(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_EM_LimitText(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_EM_LineFromChar(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_EM_LineIndex(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_EM_LineLength(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_EM_LineScroll(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_EM_ReplaceSel(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_EM_Scroll(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_EM_SetHandle(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_EM_SetModify(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_EM_SetPasswordChar(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_EM_SetReadOnly(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_EM_SetRect(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_EM_SetRectNP(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_EM_SetSel(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_EM_SetTabStops(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_EM_SetWordBreakProc(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_EM_Undo(WND *wndPtr, WPARAM wParam, LPARAM lParam);

static LRESULT EDIT_WM_Char(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_WM_Clear(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_WM_Copy(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_WM_Cut(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_WM_Create(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_WM_Destroy(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_WM_Enable(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_WM_EraseBkGnd(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_WM_GetDlgCode(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_WM_GetFont(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_WM_GetText(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_WM_GetTextLength(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_WM_HScroll(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_WM_KeyDown(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_WM_KillFocus(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_WM_LButtonDblClk(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_WM_LButtonDown(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_WM_LButtonUp(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_WM_MouseMove(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_WM_Paint(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_WM_Paste(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_WM_SetCursor(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_WM_SetFocus(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_WM_SetFont(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_WM_SetRedraw(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_WM_SetText(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_WM_Size(WND *wndPtr, WPARAM wParam, LPARAM lParam);
static LRESULT EDIT_WM_VScroll(WND *wndPtr, WPARAM wParam, LPARAM lParam);


/*********************************************************************
 *
 *	EditWndProc()
 *
 */
LRESULT EditWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lResult = 0L;
	WND *wndPtr = WIN_FindWndPtr(hwnd);

	switch (msg) {
	case EM_CANUNDO:
		DPRINTF_EDIT_MSG("EM_CANUNDO");
		lResult = EDIT_EM_CanUndo(wndPtr, wParam, lParam);
		break;
	case EM_EMPTYUNDOBUFFER:
		DPRINTF_EDIT_MSG("EM_EMPTYUNDOBUFFER");
		lResult = EDIT_EM_EmptyUndoBuffer(wndPtr, wParam, lParam);
		break;
	case EM_FMTLINES:
		DPRINTF_EDIT_MSG("EM_FMTLINES");
		lResult = EDIT_EM_FmtLines(wndPtr, wParam, lParam);
		break;
	case EM_GETFIRSTVISIBLELINE:
		DPRINTF_EDIT_MSG("EM_GETFIRSTVISIBLELINE");
		lResult = EDIT_EM_GetFirstVisibleLine(wndPtr, wParam, lParam);
		break;
	case EM_GETHANDLE:
		DPRINTF_EDIT_MSG("EM_GETHANDLE");
		lResult = EDIT_EM_GetHandle(wndPtr, wParam, lParam);
		break;
	case EM_GETLINE:
		DPRINTF_EDIT_MSG("EM_GETLINE");
		lResult = EDIT_EM_GetLine(wndPtr, wParam, lParam);
		break;
	case EM_GETLINECOUNT:
		DPRINTF_EDIT_MSG("EM_GETLINECOUNT");
		lResult = EDIT_EM_GetLineCount(wndPtr, wParam, lParam);
		break;
	case EM_GETMODIFY:
		DPRINTF_EDIT_MSG("EM_GETMODIFY");
		lResult = EDIT_EM_GetModify(wndPtr, wParam, lParam);
		break;
	case EM_GETPASSWORDCHAR:
		DPRINTF_EDIT_MSG("EM_GETPASSWORDCHAR");
		lResult = EDIT_EM_GetPasswordChar(wndPtr, wParam, lParam);
		break;
	case EM_GETRECT:
		DPRINTF_EDIT_MSG("EM_GETRECT");
		lResult = EDIT_EM_GetRect(wndPtr, wParam, lParam);
		break;
	case EM_GETSEL:
		DPRINTF_EDIT_MSG("EM_GETSEL");
		lResult = EDIT_EM_GetSel(wndPtr, wParam, lParam);
		break;
	case EM_GETTHUMB:
		DPRINTF_EDIT_MSG("EM_GETTHUMB");
		lResult = EDIT_EM_GetThumb(wndPtr, wParam, lParam);
		break;
	case EM_GETWORDBREAKPROC:
		DPRINTF_EDIT_MSG("EM_GETWORDBREAKPROC");
		lResult = EDIT_EM_GetWordBreakProc(wndPtr, wParam, lParam);
		break;
	case EM_LIMITTEXT:
		DPRINTF_EDIT_MSG("EM_LIMITTEXT");
		lResult = EDIT_EM_LimitText(wndPtr, wParam, lParam);
		break;
	case EM_LINEFROMCHAR:
		DPRINTF_EDIT_MSG("EM_LINEFROMCHAR");
		lResult = EDIT_EM_LineFromChar(wndPtr, wParam, lParam);
		break;
	case EM_LINEINDEX:
		DPRINTF_EDIT_MSG("EM_LINEINDEX");
		lResult = EDIT_EM_LineIndex(wndPtr, wParam, lParam);
		break;
	case EM_LINELENGTH:
		DPRINTF_EDIT_MSG("EM_LINELENGTH");
		lResult = EDIT_EM_LineLength(wndPtr, wParam, lParam);
		break;
	case EM_LINESCROLL:
		DPRINTF_EDIT_MSG("EM_LINESCROLL");
		lResult = EDIT_EM_LineScroll(wndPtr, wParam, lParam);
		break;
	case EM_REPLACESEL:
		DPRINTF_EDIT_MSG("EM_REPLACESEL");
		lResult = EDIT_EM_ReplaceSel(wndPtr, wParam, lParam);
		break;
	case EM_SCROLL:
		DPRINTF_EDIT_MSG("EM_SCROLL");
		lResult = EDIT_EM_Scroll(wndPtr, wParam, lParam);
 		break;
	case EM_SETHANDLE:
		DPRINTF_EDIT_MSG("EM_SETHANDLE");
		lResult = EDIT_EM_SetHandle(wndPtr, wParam, lParam);
		break;
	case EM_SETMODIFY:
		DPRINTF_EDIT_MSG("EM_SETMODIFY");
		lResult = EDIT_EM_SetModify(wndPtr, wParam, lParam);
		break;
	case EM_SETPASSWORDCHAR:
		DPRINTF_EDIT_MSG("EM_SETPASSWORDCHAR");
		lResult = EDIT_EM_SetPasswordChar(wndPtr, wParam, lParam);
		break;
	case EM_SETREADONLY:
		DPRINTF_EDIT_MSG("EM_SETREADONLY");
		lResult = EDIT_EM_SetReadOnly(wndPtr, wParam, lParam);
 		break;
	case EM_SETRECT:
		DPRINTF_EDIT_MSG("EM_SETRECT");
		lResult = EDIT_EM_SetRect(wndPtr, wParam, lParam);
		break;
	case EM_SETRECTNP:
		DPRINTF_EDIT_MSG("EM_SETRECTNP");
		lResult = EDIT_EM_SetRectNP(wndPtr, wParam, lParam);
		break;
	case EM_SETSEL:
		DPRINTF_EDIT_MSG("EM_SETSEL");
		lResult = EDIT_EM_SetSel(wndPtr, wParam, lParam);
		break;
	case EM_SETTABSTOPS:
		DPRINTF_EDIT_MSG("EM_SETTABSTOPS");
		lResult = EDIT_EM_SetTabStops(wndPtr, wParam, lParam);
		break;
	case EM_SETWORDBREAKPROC:
		DPRINTF_EDIT_MSG("EM_SETWORDBREAKPROC");
		lResult = EDIT_EM_SetWordBreakProc(wndPtr, wParam, lParam);
		break;
	case EM_UNDO:
	case WM_UNDO:
		DPRINTF_EDIT_MSG("EM_UNDO / WM_UNDO");
		lResult = EDIT_EM_Undo(wndPtr, wParam, lParam);
		break;
	case WM_GETDLGCODE:
		DPRINTF_EDIT_MSG("WM_GETDLGCODE");
		lResult = EDIT_WM_GetDlgCode(wndPtr, wParam, lParam);
		break;
	case WM_CHAR:
		DPRINTF_EDIT_MSG("WM_CHAR");
		lResult = EDIT_WM_Char(wndPtr, wParam, lParam);
		break;
	case WM_CLEAR:
		DPRINTF_EDIT_MSG("WM_CLEAR");
		lResult = EDIT_WM_Clear(wndPtr, wParam, lParam);
		break;
	case WM_COPY:
		DPRINTF_EDIT_MSG("WM_COPY");
		lResult = EDIT_WM_Copy(wndPtr, wParam, lParam);
		break;
	case WM_CREATE:
		DPRINTF_EDIT_MSG("WM_CREATE");
		lResult = EDIT_WM_Create(wndPtr, wParam, lParam);
		break;
	case WM_CUT:
		DPRINTF_EDIT_MSG("WM_CUT");
		lResult = EDIT_WM_Cut(wndPtr, wParam, lParam);
		break;
	case WM_DESTROY:
		DPRINTF_EDIT_MSG("WM_DESTROY");
		lResult = EDIT_WM_Destroy(wndPtr, wParam, lParam);
		break;
	case WM_ENABLE:
		DPRINTF_EDIT_MSG("WM_ENABLE");
		lResult = EDIT_WM_Enable(wndPtr, wParam, lParam);
		break;
	case WM_ERASEBKGND:
		DPRINTF_EDIT_MSG("WM_ERASEBKGND");
		lResult = EDIT_WM_EraseBkGnd(wndPtr, wParam, lParam);
		break;
	case WM_GETFONT:
		DPRINTF_EDIT_MSG("WM_GETFONT");
		lResult = EDIT_WM_GetFont(wndPtr, wParam, lParam);
		break;
	case WM_GETTEXT:
		DPRINTF_EDIT_MSG("WM_GETTEXT");
		lResult = EDIT_WM_GetText(wndPtr, wParam, lParam);
		break;
	case WM_GETTEXTLENGTH:
		DPRINTF_EDIT_MSG("WM_GETTEXTLENGTH");
		lResult = EDIT_WM_GetTextLength(wndPtr, wParam, lParam);
		break;
	case WM_HSCROLL:
		DPRINTF_EDIT_MSG("WM_HSCROLL");
		lResult = EDIT_WM_HScroll(wndPtr, wParam, lParam);
		break;
	case WM_KEYDOWN:
		DPRINTF_EDIT_MSG("WM_KEYDOWN");
		lResult = EDIT_WM_KeyDown(wndPtr, wParam, lParam);
		break;
	case WM_KILLFOCUS:
		DPRINTF_EDIT_MSG("WM_KILLFOCUS");
		lResult = EDIT_WM_KillFocus(wndPtr, wParam, lParam);
		break;
	case WM_LBUTTONDBLCLK:
		DPRINTF_EDIT_MSG("WM_LBUTTONDBLCLK");
		lResult = EDIT_WM_LButtonDblClk(wndPtr, wParam, lParam);
		break;
	case WM_LBUTTONDOWN:
		DPRINTF_EDIT_MSG("WM_LBUTTONDOWN");
		lResult = EDIT_WM_LButtonDown(wndPtr, wParam, lParam);
		break;
	case WM_LBUTTONUP:
		DPRINTF_EDIT_MSG("WM_LBUTTONUP");
		lResult = EDIT_WM_LButtonUp(wndPtr, wParam, lParam);
		break;
	case WM_MOUSEMOVE:
		/*
		 *	DPRINTF_EDIT_MSG("WM_MOUSEMOVE");
		 */
		lResult = EDIT_WM_MouseMove(wndPtr, wParam, lParam);
		break;
	case WM_PAINT:
		DPRINTF_EDIT_MSG("WM_PAINT");
		lResult = EDIT_WM_Paint(wndPtr, wParam, lParam);
		break;
	case WM_PASTE:
		DPRINTF_EDIT_MSG("WM_PASTE");
		lResult = EDIT_WM_Paste(wndPtr, wParam, lParam);
		break;
	case WM_SETCURSOR:
		/*
		 *	DPRINTF_EDIT_MSG("WM_SETCURSOR");
		 */
		lResult = EDIT_WM_SetCursor(wndPtr, wParam, lParam);
		break;
	case WM_SETFOCUS:
		DPRINTF_EDIT_MSG("WM_SETFOCUS");
		lResult = EDIT_WM_SetFocus(wndPtr, wParam, lParam);
		break;
	case WM_SETFONT:
		DPRINTF_EDIT_MSG("WM_SETFONT");
		lResult = EDIT_WM_SetFont(wndPtr, wParam, lParam);
		break;
	case WM_SETREDRAW:
		DPRINTF_EDIT_MSG("WM_SETREDRAW");
		lResult = EDIT_WM_SetRedraw(wndPtr, wParam, lParam);
		break;
	case WM_SETTEXT:
		DPRINTF_EDIT_MSG("WM_SETTEXT");
		lResult = EDIT_WM_SetText(wndPtr, wParam, lParam);
		break;
	case WM_SIZE:
		DPRINTF_EDIT_MSG("WM_SIZE");
		lResult = EDIT_WM_Size(wndPtr, wParam, lParam);
		break;
	case WM_VSCROLL:
		DPRINTF_EDIT_MSG("WM_VSCROLL");
		lResult = EDIT_WM_VScroll(wndPtr, wParam, lParam);
		break;
	default:
		if (msg >= WM_USER)
			fprintf(stdnimp, "edit: undocumented message %d >= WM_USER, please report.\n", msg);
		lResult = DefWindowProc(hwnd, msg, wParam, lParam);
		break;
	}
	EDIT_ReleasePointer(wndPtr);
	return lResult;
}


/*********************************************************************
 *
 *	EDIT_BuildLineDefs
 *
 *	Build array of pointers to text lines.
 *	Lines can end with '\0' (last line), nothing (if it is to long),
 *	a delimiter (usually ' '), a soft return '\r\r\n' or a hard return '\r\n'
 *
 */
static void EDIT_BuildLineDefs(WND *wndPtr)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	char *text = EDIT_GetPointer(wndPtr);
	int ww = EDIT_GetWndWidth(wndPtr);
	HDC hdc;
	HFONT hFont;
	HFONT oldFont = 0;
	char *start, *cp;
	int prev, next;
	int width;
	int length;
	LINE_END ending;

	hdc = GetDC(wndPtr->hwndSelf);
	hFont = (HFONT)EDIT_WM_GetFont(wndPtr, 0, 0L);
	if (hFont)
		oldFont = SelectObject(hdc, hFont);

	if (!IsMultiLine(wndPtr)) {
		es->LineCount = 1;
		es->LineDefs = xrealloc(es->LineDefs, sizeof(LINEDEF));
		es->LineDefs[0].offset = 0;
		es->LineDefs[0].length = EDIT_WM_GetTextLength(wndPtr, 0, 0L);
		es->LineDefs[0].ending = END_0;
		es->TextWidth = LOWORD(GetTabbedTextExtent(hdc, text,
					es->LineDefs[0].length,
					es->NumTabStops, es->TabStops));
	} else {
		es->LineCount = 0;
		start = text;
		do {
			if (!(cp = strstr(start, "\r\n"))) {
				ending = END_0;
				length = strlen(start);
			} else if ((cp > start) && (*(cp - 1) == '\r')) {
				ending = END_SOFT;
				length = cp - start - 1;
			} else {
				ending = END_HARD;
				length = cp - start;
			}
			width = LOWORD(GetTabbedTextExtent(hdc, start, length,
						es->NumTabStops, es->TabStops));

			if (IsWordWrap(wndPtr) && (width > ww)) {
				next = 0;
				do {
					prev = next;
					next = EDIT_CallWordBreakProc(wndPtr, start,
							prev + 1, length, WB_RIGHT);
					width = LOWORD(GetTabbedTextExtent(hdc, start, next,
							es->NumTabStops, es->TabStops));
				} while (width <= ww);
				if (prev) {
					length = prev;
					if (EDIT_CallWordBreakProc(wndPtr, start, length - 1,
									length, WB_ISDELIMITER)) {
						length--;
						ending = END_DELIMIT;
					} else
						ending = END_NONE;
				} else {
				}
				width = LOWORD(GetTabbedTextExtent(hdc, start, length,
							es->NumTabStops, es->TabStops));
			}

			es->LineDefs = xrealloc(es->LineDefs, (es->LineCount + 1) * sizeof(LINEDEF));
			es->LineDefs[es->LineCount].offset = start - text;
			es->LineDefs[es->LineCount].length = length;
			es->LineDefs[es->LineCount].ending = ending;
			es->LineCount++;
			es->TextWidth = MAX(es->TextWidth, width);

			start += length;
			switch (ending) {
			case END_SOFT:
				start += 3;
				break;
			case END_HARD:
				start += 2;
				break;
			case END_DELIMIT:
				start++;
				break;
			default:
				break;
			}
		} while (*start || (ending == END_SOFT) || (ending == END_HARD));
	}
	if (hFont)
		SelectObject(hdc, oldFont);
	ReleaseDC(wndPtr->hwndSelf, hdc);
}


/*********************************************************************
 *
 *	EDIT_CallWordBreakProc
 *
 *	Call appropriate WordBreakProc (internal or external).
 *
 */
static int EDIT_CallWordBreakProc(WND *wndPtr, char *s, int index, int count, int action)
{
	EDITWORDBREAKPROC wbp = (EDITWORDBREAKPROC)EDIT_EM_GetWordBreakProc(wndPtr, 0, 0L);

	if (wbp) {
		return CallWordBreakProc((FARPROC)wbp,
				(LONG)MAKE_SEGPTR(s), index, count, action);
	} else
		return EDIT_WordBreakProc(s, index, count, action);
}


/*********************************************************************
 *
 *	EDIT_ColFromWndX
 *
 *	Calculates, for a given line and X-coordinate on the screen, the column.
 *
 */
static int EDIT_ColFromWndX(WND *wndPtr, int line, int x)
{	
	int linecount = EDIT_EM_GetLineCount(wndPtr, 0, 0L);
	int lineindex = EDIT_EM_LineIndex(wndPtr, line, 0L);
	int linelength = EDIT_EM_LineLength(wndPtr, lineindex, 0L);
	int i;

	line = MAX(0, MIN(line, linecount - 1));
	for (i = 0 ; i < linelength ; i++)
		if (EDIT_WndXFromCol(wndPtr, line, i) >= x)
			break;
	return i;
}


/*********************************************************************
 *
 *	EDIT_DelEnd
 *
 *	Delete all characters on this line to right of cursor.
 *
 */
static void EDIT_DelEnd(WND *wndPtr)
{
	EDIT_EM_SetSel(wndPtr, FALSE, MAKELPARAM(-1, 0));
	EDIT_MoveEnd(wndPtr, TRUE);
	EDIT_WM_Clear(wndPtr, 0, 0L);
}


/*********************************************************************
 *
 *	EDIT_DelLeft
 *
 *	Delete character to left of cursor.
 *
 */
static void EDIT_DelLeft(WND *wndPtr)
{
	EDIT_EM_SetSel(wndPtr, FALSE, MAKELPARAM(-1, 0));
	EDIT_MoveBackward(wndPtr, TRUE);
	EDIT_WM_Clear(wndPtr, 0, 0L);
}


/*********************************************************************
 *
 *	EDIT_DelRight
 *
 *	Delete character to right of cursor.
 *
 */
static void EDIT_DelRight(WND *wndPtr)
{
	EDIT_EM_SetSel(wndPtr, FALSE, MAKELPARAM(-1, 0));
	EDIT_MoveForward(wndPtr, TRUE);
	EDIT_WM_Clear(wndPtr, 0, 0L);
}


/*********************************************************************
 *
 *	EDIT_GetAveCharWidth
 *
 */
static int EDIT_GetAveCharWidth(WND *wndPtr)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	
	return es->AveCharWidth;
}


/*********************************************************************
 *
 *	EDIT_GetLineHeight
 *
 */
static int EDIT_GetLineHeight(WND *wndPtr)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	
	return es->LineHeight;
}


/*********************************************************************
 *
 *	EDIT_GetLineRect
 *
 *	Calculates the bounding rectangle for a line from a starting
 *	column to an ending column.
 *
 */
static void EDIT_GetLineRect(WND *wndPtr, int line, int scol, int ecol, LPRECT rc)
{
	rc->top = EDIT_WndYFromLine(wndPtr, line);
	rc->bottom = rc->top + EDIT_GetLineHeight(wndPtr);
	rc->left = EDIT_WndXFromCol(wndPtr, line, scol);
	rc->right = (ecol < 0) ? EDIT_GetWndWidth(wndPtr) : EDIT_WndXFromCol(wndPtr, line, ecol);
}


/*********************************************************************
 *
 *	EDIT_GetPointer
 *
 *	This acts as a LOCAL_Lock(), but it locks only once.  This way
 *	you can call it whenever you like, without unlocking.
 *
 */
static char *EDIT_GetPointer(WND *wndPtr)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	
	if (!es->text && es->hBuf)
		es->text = LOCAL_Lock(wndPtr->hInstance, es->hBuf);
	return es->text;
}


/*********************************************************************
 *
 *	EDIT_GetRect
 *
 *	Beware: This is not the function called on EM_GETRECT.
 *	It expects a (LPRECT) in lParam, not a (SEGPTR).
 *	It is used internally, as if there were no pointer difficulties.
 *
 */
static LRESULT EDIT_GetRect(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	GetClientRect(wndPtr->hwndSelf, (LPRECT)lParam);
	return 0L;
}


/*********************************************************************
 *
 *	EDIT_GetRedraw
 *
 */
static BOOL EDIT_GetRedraw(WND *wndPtr)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	return es->Redraw;
}


/*********************************************************************
 *
 *	EDIT_GetTextWidth
 *
 */
static int EDIT_GetTextWidth(WND *wndPtr)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	
	return es->TextWidth;
}


/*********************************************************************
 *
 *	EDIT_GetVisibleLineCount
 *
 */
static int EDIT_GetVisibleLineCount(WND *wndPtr)
{
	RECT rc;
	
	EDIT_GetRect(wndPtr, 0, (LPARAM)&rc);
	return MAX(1, MAX(rc.bottom - rc.top, 0) / EDIT_GetLineHeight(wndPtr));
}


/*********************************************************************
 *
 *	EDIT_GetWndWidth
 *
 */
static int EDIT_GetWndWidth(WND *wndPtr)
{
	RECT rc;
	
	EDIT_GetRect(wndPtr, 0, (LPARAM)&rc);
	return rc.right - rc.left;
}


/*********************************************************************
 *
 *	EDIT_GetXOffset
 *
 */
static int EDIT_GetXOffset(WND *wndPtr)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	
	return es->XOffset;
}


/*********************************************************************
 *
 *	EDIT_LineFromWndY
 *
 *	Calculates, for a given Y-coordinate on the screen, the line.
 *
 */
static int EDIT_LineFromWndY(WND *wndPtr, int y)
{
	int firstvis = EDIT_EM_GetFirstVisibleLine(wndPtr, 0, 0L);
	int lineheight = EDIT_GetLineHeight(wndPtr);
	int linecount = EDIT_EM_GetLineCount(wndPtr, 0, 0L);

	return MAX(0, MIN(linecount - 1, y / lineheight + firstvis));
}


/*********************************************************************
 *
 *	EDIT_MakeFit
 *
 *	Try to fit size + 1 bytes in the buffer.  Contrain to limits.
 *
 */
static BOOL EDIT_MakeFit(WND *wndPtr, int size)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	if (size <= es->BufSize)
		return TRUE;
	if (size > es->BufLimit)
		return FALSE;
	es->BufSize = ((size / GROWLENGTH) + 1) * GROWLENGTH;
	if (es->BufSize > es->BufLimit)
		es->BufSize = es->BufLimit;

	dprintf_edit(stddeb, "edit: EDIT_MakeFit: ReAlloc to %d+1\n", es->BufSize);

	return LOCAL_ReAlloc(wndPtr->hInstance, es->hBuf, es->BufSize + 1, LMEM_MOVEABLE);
}


/*********************************************************************
 *
 *	EDIT_MoveBackward
 *
 */
static void EDIT_MoveBackward(WND *wndPtr, BOOL extend)
{
	int s = LOWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	int e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	int l = EDIT_EM_LineFromChar(wndPtr, e, 0L);
	int lineindex = EDIT_EM_LineIndex(wndPtr, l, 0L);

	if (e - lineindex == 0) {
		if (l) {
			lineindex = EDIT_EM_LineIndex(wndPtr, l - 1, 0L);
			e = lineindex + EDIT_EM_LineLength(wndPtr, lineindex, 0L);
		}
	} else
		e--;
	if (!extend)
		s = e;
	EDIT_EM_SetSel(wndPtr, TRUE, MAKELPARAM(s, e));
}


/*********************************************************************
 *
 *	EDIT_MoveDownward
 *
 */
static void EDIT_MoveDownward(WND *wndPtr, BOOL extend)
{
	int s = LOWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	int e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	int l = EDIT_EM_LineFromChar(wndPtr, e, 0L);
	int linecount = EDIT_EM_GetLineCount(wndPtr, e, 0L);
	int lineindex = EDIT_EM_LineIndex(wndPtr, l, 0L);
	int x;

	if (l < linecount - 1) {
		x = EDIT_WndXFromCol(wndPtr, l, e - lineindex);
		l++;
		e = EDIT_EM_LineIndex(wndPtr, l, 0L) +
				EDIT_ColFromWndX(wndPtr, l, x);
	}
	if (!extend)
		s = e;
	EDIT_EM_SetSel(wndPtr, TRUE, MAKELPARAM(s, e));
}


/*********************************************************************
 *
 *	EDIT_MoveEnd
 *
 */
static void EDIT_MoveEnd(WND *wndPtr, BOOL extend)
{
	int s = LOWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	int e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	int l = EDIT_EM_LineFromChar(wndPtr, e, 0L);
	int linelength = EDIT_EM_LineLength(wndPtr, e, 0L);
	int lineindex = EDIT_EM_LineIndex(wndPtr, l, 0L);

	e = lineindex + linelength;
	if (!extend)
		s = e;
	EDIT_EM_SetSel(wndPtr, TRUE, MAKELPARAM(s, e));
}


/*********************************************************************
 *
 *	EDIT_MoveForward
 *
 */
static void EDIT_MoveForward(WND *wndPtr, BOOL extend)
{
	int s = LOWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	int e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	int l = EDIT_EM_LineFromChar(wndPtr, e, 0L);
	int linecount = EDIT_EM_GetLineCount(wndPtr, e, 0L);
	int linelength = EDIT_EM_LineLength(wndPtr, e, 0L);
	int lineindex = EDIT_EM_LineIndex(wndPtr, l, 0L);

	if (e - lineindex == linelength) {
		if (l != linecount - 1)
			e = EDIT_EM_LineIndex(wndPtr, l + 1, 0L);
	} else
		e++;
	if (!extend)
		s = e;
	EDIT_EM_SetSel(wndPtr, TRUE, MAKELPARAM(s, e));
}


/*********************************************************************
 *
 *	EDIT_MoveHome
 *
 *	Home key: move to beginning of line.
 *
 */
static void EDIT_MoveHome(WND *wndPtr, BOOL extend)
{
	int s = LOWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	int e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	int l = EDIT_EM_LineFromChar(wndPtr, e, 0L);
	int lineindex = EDIT_EM_LineIndex(wndPtr, l, 0L);

	e = lineindex;
	if (!extend)
		s = e;
	EDIT_EM_SetSel(wndPtr, TRUE, MAKELPARAM(s, e));
}


/*********************************************************************
 *
 *	EDIT_MovePageDown
 *
 */
static void EDIT_MovePageDown(WND *wndPtr, BOOL extend)
{
	int s = LOWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	int e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	int l = EDIT_EM_LineFromChar(wndPtr, e, 0L);
	int linecount = EDIT_EM_GetLineCount(wndPtr, e, 0L);
	int lineindex = EDIT_EM_LineIndex(wndPtr, l, 0L);
	int x;

	if (l < linecount - 1) {
		x = EDIT_WndXFromCol(wndPtr, l, e - lineindex);
		l = MIN(linecount - 1, l + EDIT_GetVisibleLineCount(wndPtr));
		e = EDIT_EM_LineIndex(wndPtr, l, 0L) +
				EDIT_ColFromWndX(wndPtr, l, x);
	}
	if (!extend)
		s = e;
	EDIT_EM_SetSel(wndPtr, TRUE, MAKELPARAM(s, e));
}


/*********************************************************************
 *
 *	EDIT_MovePageUp
 *
 */
static void EDIT_MovePageUp(WND *wndPtr, BOOL extend)
{
	int s = LOWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	int e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	int l = EDIT_EM_LineFromChar(wndPtr, e, 0L);
	int lineindex = EDIT_EM_LineIndex(wndPtr, l, 0L);
	int x;

	if (l) {
		x = EDIT_WndXFromCol(wndPtr, l, e - lineindex);
		l = MAX(0, l - EDIT_GetVisibleLineCount(wndPtr));
		e = EDIT_EM_LineIndex(wndPtr, l, 0L) +
				EDIT_ColFromWndX(wndPtr, l, x);
	}
	if (!extend)
		s = e;
	EDIT_EM_SetSel(wndPtr, TRUE, MAKELPARAM(s, e));
}


/*********************************************************************
 *
 *	EDIT_MoveUpward
 *
 */
static void EDIT_MoveUpward(WND *wndPtr, BOOL extend)
{
	int s = LOWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	int e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	int l = EDIT_EM_LineFromChar(wndPtr, e, 0L);
	int lineindex = EDIT_EM_LineIndex(wndPtr, l, 0L);
	int x;

	if (l) {
		x = EDIT_WndXFromCol(wndPtr, l, e - lineindex);
		l--;
		e = EDIT_EM_LineIndex(wndPtr, l, 0L) +
				EDIT_ColFromWndX(wndPtr, l, x);
	}
	if (!extend)
		s = e;
	EDIT_EM_SetSel(wndPtr, TRUE, MAKELPARAM(s, e));
}


/*********************************************************************
 *
 *	EDIT_MoveWordBackward
 *
 */
static void EDIT_MoveWordBackward(WND *wndPtr, BOOL extend)
{
	int s = LOWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	int e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	int l = EDIT_EM_LineFromChar(wndPtr, e, 0L);
	int linelength = EDIT_EM_LineLength(wndPtr, e, 0L);
	int lineindex = EDIT_EM_LineIndex(wndPtr, l, 0L);
	char *text;

	if (e - lineindex == 0) {
		if (l) {
			lineindex = EDIT_EM_LineIndex(wndPtr, l - 1, 0L);
			e = lineindex + EDIT_EM_LineLength(wndPtr, lineindex, 0L);
		}
	} else {
		text = EDIT_GetPointer(wndPtr);
		e = lineindex + EDIT_CallWordBreakProc(wndPtr, text + lineindex,
				e - lineindex, linelength, WB_LEFT);
	}
	if (!extend)
		s = e;
	EDIT_EM_SetSel(wndPtr, TRUE, MAKELPARAM(s, e));
}


/*********************************************************************
 *
 *	EDIT_MoveWordForward
 *
 */
static void EDIT_MoveWordForward(WND *wndPtr, BOOL extend)
{
	int s = LOWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	int e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	int l = EDIT_EM_LineFromChar(wndPtr, e, 0L);
	int linecount = EDIT_EM_GetLineCount(wndPtr, e, 0L);
	int linelength = EDIT_EM_LineLength(wndPtr, e, 0L);
	int lineindex = EDIT_EM_LineIndex(wndPtr, l, 0L);
	char *text;

	if (e - lineindex == linelength) {
		if (l != linecount - 1)
			e = EDIT_EM_LineIndex(wndPtr, l + 1, 0L);
	} else {
		text = EDIT_GetPointer(wndPtr);
		e = lineindex + EDIT_CallWordBreakProc(wndPtr, text + lineindex,
				e - lineindex + 1, linelength, WB_RIGHT);
	}
	if (!extend)
		s = e;
	EDIT_EM_SetSel(wndPtr, TRUE, MAKELPARAM(s, e));
}


/*********************************************************************
 *
 *	EDIT_PaintLine
 *
 */
static void EDIT_PaintLine(WND *wndPtr, HDC hdc, int line)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	int firstvis = EDIT_EM_GetFirstVisibleLine(wndPtr, 0, 0L);
	int viscount = EDIT_GetVisibleLineCount(wndPtr);
	int linecount = EDIT_EM_GetLineCount(wndPtr, 0, 0L);
	int LineStart;
	int LineEnd;
	int ReverseStart;
	int ReverseEnd;
	int x;
	int y;

	if ((line < firstvis) || (line > firstvis + viscount) || (line >= linecount))
		return;

	dprintf_edit(stddeb, "edit: EDIT_PaintLine: line=%d\n", line);

	x = EDIT_WndXFromCol(wndPtr, line, 0);
	y = EDIT_WndYFromLine(wndPtr, line);
	LineStart = EDIT_EM_LineIndex(wndPtr, line, 0L);
	LineEnd = LineStart + EDIT_EM_LineLength(wndPtr, LineStart, 0L);
	ReverseStart = MIN(es->SelStart, es->SelEnd);
	ReverseEnd = MAX(es->SelStart, es->SelEnd);
	ReverseStart = MIN(LineEnd, MAX(LineStart, ReverseStart));
	ReverseEnd = MIN(LineEnd, MAX(LineStart, ReverseEnd));
	if (ReverseStart != ReverseEnd) {
		x += EDIT_PaintText(wndPtr, hdc, x, y, line,
				0, ReverseStart - LineStart, FALSE);
		x += EDIT_PaintText(wndPtr, hdc, x, y, line,
				ReverseStart - LineStart,
				ReverseEnd - ReverseStart, TRUE);
		x += EDIT_PaintText(wndPtr, hdc, x, y, line,
				ReverseEnd - LineStart,
				LineEnd - ReverseEnd, FALSE);
	} else
		x += EDIT_PaintText(wndPtr, hdc, x, y, line,
				0, LineEnd - LineStart, FALSE);
}


/*********************************************************************
 *
 *	EDIT_PaintText
 *
 */
static int EDIT_PaintText(WND *wndPtr, HDC hdc, int x, int y, int line, int col, int count, BOOL rev)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	COLORREF BkColor;
	COLORREF TextColor;
	int ret;
	char *text;
	int lineindex = EDIT_EM_LineIndex(wndPtr, line, 0L);
	int xoffset = EDIT_GetXOffset(wndPtr);

	if (count < 1)
		return 0;
		
	BkColor = GetBkColor(hdc);
	TextColor = GetTextColor(hdc);
	if (rev) {
		SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
		SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
	}
	text = EDIT_GetPointer(wndPtr);
	ret = LOWORD(TabbedTextOut(hdc, x, y, text + lineindex + col, count,
					es->NumTabStops, es->TabStops, -xoffset));	
	if (rev) {
		SetBkColor(hdc, BkColor);
		SetTextColor(hdc, TextColor);
	}
	return ret;
}	


/*********************************************************************
 *
 *	EDIT_ReleasePointer
 *
 *	This is the only helper function that can be called with es = NULL.
 *	It is called at the end of EditWndProc() to unlock the buffer.
 *	
 */
static void EDIT_ReleasePointer(WND *wndPtr)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	
	if (!es)
		return;
	if (es->text && es->hBuf)
		LOCAL_Unlock(wndPtr->hInstance, es->hBuf);
	es->text = NULL;
}


/*********************************************************************
 *
 *	EDIT_ReplaceSel
 *
 *	Beware: This is not the function called on EM_REPLACESEL.
 *	It expects a (char *) in lParam, not a (SEGPTR).
 *	It is used internally, as if there were no pointer difficulties.
 *
 */
static LRESULT EDIT_ReplaceSel(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	const char *str = (char *)lParam;
	int strl = strlen(str);
	int tl = EDIT_WM_GetTextLength(wndPtr, 0, 0L);
	int s = LOWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	int e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	int i;
	char *p;
	char *text;
	BOOL redraw;

	ORDER_INT(s,e);
	if (!EDIT_MakeFit(wndPtr, tl - (e - s) + strl)) {
		EDIT_NOTIFY_PARENT(wndPtr, EN_MAXTEXT);
		return 0L;
	}
	redraw = EDIT_GetRedraw(wndPtr);
	EDIT_WM_SetRedraw(wndPtr, FALSE, 0L);
	EDIT_WM_Clear(wndPtr, 0, 0L);
	tl = EDIT_WM_GetTextLength(wndPtr, 0, 0L);
	e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	text = EDIT_GetPointer(wndPtr);
	for (p = text + tl ; p >= text + e ; p--)
		p[strl] = p[0];
	for (i = 0 , p = text + e ; i < strl ; i++)
		p[i] = str[i];
	EDIT_BuildLineDefs(wndPtr);
	e += strl;
	EDIT_EM_SetSel(wndPtr, TRUE, MAKELPARAM(e, e));
	EDIT_EM_SetModify(wndPtr, TRUE, 0L);
	EDIT_NOTIFY_PARENT(wndPtr, EN_UPDATE);
	EDIT_WM_SetRedraw(wndPtr, redraw, 0L);
	if (redraw) {
		InvalidateRect(wndPtr->hwndSelf, NULL, TRUE);
		EDIT_NOTIFY_PARENT(wndPtr, EN_CHANGE);
	}
	return 0L;
}
 

/*********************************************************************
 *
 *	EDIT_ScrollIntoView
 *
 *	Makes sure the caret is visible.
 *
 */
static void EDIT_ScrollIntoView(WND *wndPtr)
{
	int e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	int l = EDIT_EM_LineFromChar(wndPtr, e, 0L);
	int lineindex = EDIT_EM_LineIndex(wndPtr, l, 0L);
	int firstvis = EDIT_EM_GetFirstVisibleLine(wndPtr, 0, 0L);
	int vislinecount = EDIT_GetVisibleLineCount(wndPtr);
	int wndwidth = EDIT_GetWndWidth(wndPtr);
	int charwidth = EDIT_GetAveCharWidth(wndPtr);
	int x = EDIT_WndXFromCol(wndPtr, l, e - lineindex);
	int dy = 0;
	int dx = 0;

	if (l >= firstvis + vislinecount)
		dy = l - vislinecount + 1 - firstvis;
	if (l < firstvis)
		dy = l - firstvis;
	if (x < 0)
		dx = x - wndwidth / HSCROLL_FRACTION / charwidth * charwidth;
	if (x > wndwidth)
		dx = x - (HSCROLL_FRACTION - 1) * wndwidth / HSCROLL_FRACTION / charwidth * charwidth;
	if (dy || dx) {
		EDIT_EM_LineScroll(wndPtr, 0, MAKELPARAM(dy, dx));
		if (dy) 
			EDIT_NOTIFY_PARENT(wndPtr, EN_VSCROLL);
		if (dx)
			EDIT_NOTIFY_PARENT(wndPtr, EN_HSCROLL);
	}
}


/*********************************************************************
 *
 *	EDIT_WndXFromCol
 *
 *	Calculates, for a given line and column, the X-coordinate on the screen.
 *
 */
static int EDIT_WndXFromCol(WND *wndPtr, int line, int col)
{	
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	char *text = EDIT_GetPointer(wndPtr);
	int ret;
	HDC hdc;
	HFONT hFont;
	HFONT oldFont = 0;
	int linecount = EDIT_EM_GetLineCount(wndPtr, 0, 0L);
	int lineindex = EDIT_EM_LineIndex(wndPtr, line, 0L);
	int linelength = EDIT_EM_LineLength(wndPtr, lineindex, 0L);
	int xoffset = EDIT_GetXOffset(wndPtr);

	hdc = GetDC(wndPtr->hwndSelf);
	hFont = (HFONT)EDIT_WM_GetFont(wndPtr, 0, 0L);
	if (hFont)
		oldFont = SelectObject(hdc, hFont);
	line = MAX(0, MIN(line, linecount - 1));
	col = MAX(0, MIN(col, linelength));
	ret = LOWORD(GetTabbedTextExtent(hdc,
			text + lineindex, col,
			es->NumTabStops, es->TabStops)) - xoffset;
	if (hFont)
		SelectObject(hdc, oldFont);
	ReleaseDC(wndPtr->hwndSelf, hdc);
	return ret;
}


/*********************************************************************
 *
 *	EDIT_WndYFromLine
 *
 *	Calculates, for a given line, the Y-coordinate on the screen.
 *
 */
static int EDIT_WndYFromLine(WND *wndPtr, int line)
{
	int firstvis = EDIT_EM_GetFirstVisibleLine(wndPtr, 0, 0L);
	int lineheight = EDIT_GetLineHeight(wndPtr);

	return (line - firstvis) * lineheight;
}


/*********************************************************************
 *
 *	EDIT_WordBreakProc
 *
 *	Find the beginning of words.
 *	Note:	unlike the specs for a WordBreakProc, this function only
 *		allows to be called without linebreaks between s[0] upto
 *		s[count - 1].  Remember it is only called
 *		internally, so we can decide this for ourselves.
 *
 */
static int EDIT_WordBreakProc(char *s, int index, int count, int action)
{
	int ret = 0;

	dprintf_edit(stddeb, "edit: EDIT_WordBreakProc: s=%p, index=%d"
			", count=%d, action=%d\n", s, index, count, action);

	switch (action) {
	case WB_LEFT:
		if (!count) 
			break;
		if (index)
			index--;
		if (s[index] == ' ') {
			while (index && (s[index] == ' '))
				index--;
			if (index) {
				while (index && (s[index] != ' '))
					index--;
				if (s[index] == ' ')
					index++;
			}
		} else {
			while (index && (s[index] != ' '))
				index--;
			if (s[index] == ' ')
				index++;
		}
		ret = index;
		break;
	case WB_RIGHT:
		if (!count)
			break;
		if (index)
			index--;
		if (s[index] == ' ')
			while ((index < count) && (s[index] == ' ')) index++;
		else {
			while (s[index] && (s[index] != ' ') && (index < count))
				index++;
			while ((s[index] == ' ') && (index < count)) index++;
		}
		ret = index;
		break;
	case WB_ISDELIMITER:
		ret = (s[index] == ' ');
		break;
	default:
		fprintf(stderr, "edit: EDIT_WordBreakProc: unknown action code, please report !\n");
		break;
	}
	return ret;
}


/*********************************************************************
 *
 *	EM_CANUNDO
 *
 */
static LRESULT EDIT_EM_CanUndo(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	return 0L;
}


/*********************************************************************
 *
 *	EM_EMPTYUNDOBUFFER
 *
 */
static LRESULT EDIT_EM_EmptyUndoBuffer(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	return 0L;
}


/*********************************************************************
 *
 *	EM_FMTLINES
 *
 */
static LRESULT EDIT_EM_FmtLines(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	fprintf(stdnimp, "edit: EM_FMTLINES: message not implemented.\n");
	return wParam ? -1L : 0L;
}


/*********************************************************************
 *
 *	EM_GETFIRSTVISIBLELINE
 *
 */
static LRESULT EDIT_EM_GetFirstVisibleLine(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	
	return (LRESULT)es->FirstVisibleLine;
}


/*********************************************************************
 *
 *	EM_GETHANDLE
 *
 */
static LRESULT EDIT_EM_GetHandle(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	return (LRESULT)es->hBuf;
}
 

/*********************************************************************
 *
 *	EM_GETLINE
 *
 */
static LRESULT EDIT_EM_GetLine(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	char *text;
	char *src;
	char *dst;
	int len;
	int i;
	int linecount = EDIT_EM_GetLineCount(wndPtr, 0, 0L);

	if (!IsMultiLine(wndPtr))
		wParam = 0;
	if ((UINT)wParam >= linecount)
		return 0L;
	text = EDIT_GetPointer(wndPtr);
	src = text + EDIT_EM_LineIndex(wndPtr, wParam, 0L);
	dst = (char *)PTR_SEG_TO_LIN(lParam);
	len = MIN(*(WORD *)dst, EDIT_EM_LineLength(wndPtr, wParam, 0L));
	for (i = 0 ; i < len ; i++) {
		*dst = *src;
		src++;
		dst++;
	}
	return (LRESULT)len;
}


/*********************************************************************
 *
 *	EM_GETLINECOUNT
 *
 */
static LRESULT EDIT_EM_GetLineCount(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	
	return (LRESULT)es->LineCount;
}


/*********************************************************************
 *
 *	EM_GETMODIFY
 *
 */
static LRESULT EDIT_EM_GetModify(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	return (LRESULT)es->TextChanged;
}


/*********************************************************************
 *
 *	EM_GETPASSWORDCHAR
 *
 */
static LRESULT EDIT_EM_GetPasswordChar(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	return (LRESULT)es->PasswordChar;
}


/*********************************************************************
 *
 *	EM_GETRECT
 *
 */
static LRESULT EDIT_EM_GetRect(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	return EDIT_GetRect(wndPtr, wParam, (LPARAM)PTR_SEG_TO_LIN(lParam));
}


/*********************************************************************
 *
 *	EM_GETSEL
 *
 */
static LRESULT EDIT_EM_GetSel(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	return MAKELONG(es->SelStart, es->SelEnd);
}


/*********************************************************************
 *
 *	EM_GETTHUMB
 *
 *	undocumented: is this right ?
 *
 */
static LRESULT EDIT_EM_GetThumb(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	return MAKELONG(EDIT_WM_VScroll(wndPtr, EM_GETTHUMB, 0L),
		EDIT_WM_HScroll(wndPtr, EM_GETTHUMB, 0L));
}


/*********************************************************************
 *
 *	EM_GETWORDBREAKPROC
 *
 */
static LRESULT EDIT_EM_GetWordBreakProc(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	return (LRESULT)es->WordBreakProc;
}


/*********************************************************************
 *
 *	EM_LIMITTEXT
 *
 */
static LRESULT EDIT_EM_LimitText(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	if (IsMultiLine(wndPtr)) {
		if (wParam)
			es->BufLimit = MIN((UINT)wParam, BUFLIMIT_MULTI);
		else
			es->BufLimit = BUFLIMIT_MULTI;
	} else {
		if (wParam)
			es->BufLimit = MIN((UINT)wParam, BUFLIMIT_SINGLE);
		else
			es->BufLimit = BUFLIMIT_SINGLE;
	}
	return 0L;
}


/*********************************************************************
 *
 *	EM_LINEFROMCHAR
 *
 */
static LRESULT EDIT_EM_LineFromChar(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	int l;

	if (!IsMultiLine(wndPtr))
		return 0L;
	if ((INT)wParam == -1)
		wParam = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	l = EDIT_EM_GetLineCount(wndPtr, 0, 0L) - 1;
	while (EDIT_EM_LineIndex(wndPtr, l, 0L) > (UINT)wParam)
		l--;
	return (LRESULT)l;
}


/*********************************************************************
 *
 *	EM_LINEINDEX
 *
 */
static LRESULT EDIT_EM_LineIndex(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	int e;
	int l;

	if ((INT)wParam < 0) {
		e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
		l = EDIT_EM_GetLineCount(wndPtr, 0, 0L) - 1;
		while (es->LineDefs[l].offset > (UINT)wParam)
			l--;
		return (LRESULT)es->LineDefs[l].offset;
	}
	if (wParam >= es->LineCount)
		return -1L;
	return (LRESULT)es->LineDefs[wParam].offset;
}


/*********************************************************************
 *
 *	EM_LINELENGTH
 *
 */
static LRESULT EDIT_EM_LineLength(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	int SelStartLine;
	int SelEndLine;

	if (!IsMultiLine(wndPtr))
		return (LRESULT)es->LineDefs[0].length;
	if ((INT)wParam >= 0)
		return (LRESULT)es->LineDefs[EDIT_EM_LineFromChar(wndPtr, wParam, 0L)].length;
	SelStartLine = EDIT_EM_LineFromChar(wndPtr, es->SelStart, 0L);
	SelEndLine = EDIT_EM_LineFromChar(wndPtr, es->SelEnd, 0L);
	return (LRESULT)(es->SelStart - es->LineDefs[SelStartLine].offset +
			es->LineDefs[SelEndLine].offset +
			es->LineDefs[SelEndLine].length - es->SelEnd);
}
 

/*********************************************************************
 *
 *	EM_LINESCROLL
 *
 */
static LRESULT EDIT_EM_LineScroll(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	int linecount = EDIT_EM_GetLineCount(wndPtr, 0, 0L);
	int firstvis = EDIT_EM_GetFirstVisibleLine(wndPtr, 0, 0L);
	int newfirstvis = firstvis + (INT)LOWORD(lParam);
	int xoffset = EDIT_GetXOffset(wndPtr);
	int newxoffset = xoffset + (INT)HIWORD(lParam);
	int textwidth = EDIT_GetTextWidth(wndPtr);
	int dx;
	int dy;
	POINT pos;
	HRGN hRgn;

	if (newfirstvis < 0)
		newfirstvis = 0;
	if (newfirstvis >= linecount)
		newfirstvis = linecount - 1;

	if (newxoffset < 0)
		newxoffset = 0;
	if (newxoffset >= textwidth)
		newxoffset = textwidth;
	dx = xoffset - newxoffset;
	dy = EDIT_WndYFromLine(wndPtr, firstvis) - EDIT_WndYFromLine(wndPtr, newfirstvis);
	if (dx || dy) {
		if (wndPtr->hwndSelf == GetFocus())
			HideCaret(wndPtr->hwndSelf);
		if (EDIT_GetRedraw(wndPtr)) {
			hRgn = CreateRectRgn(0, 0, 0, 0);
			GetUpdateRgn(wndPtr->hwndSelf, hRgn, FALSE);
			ValidateRgn(wndPtr->hwndSelf, 0);
			OffsetRgn(hRgn, dx, dy);
			InvalidateRgn(wndPtr->hwndSelf, hRgn, TRUE);
			DeleteObject(hRgn);
			ScrollWindow(wndPtr->hwndSelf, dx, dy, NULL, NULL);
		}
		es->FirstVisibleLine = newfirstvis;
		es->XOffset = newxoffset;
		if (IsVScrollBar(wndPtr))
			SetScrollPos(wndPtr->hwndSelf, SB_VERT,
				EDIT_WM_VScroll(wndPtr, EM_GETTHUMB, 0L), TRUE);
		if (IsHScrollBar(wndPtr))
			SetScrollPos(wndPtr->hwndSelf, SB_HORZ,
				EDIT_WM_HScroll(wndPtr, EM_GETTHUMB, 0L), TRUE);
		if (wndPtr->hwndSelf == GetFocus()) {
			GetCaretPos(&pos);
			SetCaretPos(pos.x + dx, pos.y + dy);
			ShowCaret(wndPtr->hwndSelf);
		}
	}
	return -1L;
}


/*********************************************************************
 *
 *	EM_REPLACESEL
 *
 */
static LRESULT EDIT_EM_ReplaceSel(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	return (LRESULT)EDIT_ReplaceSel(wndPtr, wParam,
				(LPARAM)(char *)PTR_SEG_TO_LIN(lParam));
}
 

/*********************************************************************
 *
 *	EM_SCROLL
 *
 *	FIXME: undocumented message.
 */
static LRESULT EDIT_EM_Scroll(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	fprintf(stdnimp, "edit: EM_SCROLL: message not implemented (undocumented), please report.\n");
	return 0L;
}


/*********************************************************************
 *
 *	EM_SETHANDLE
 *
 */
static LRESULT EDIT_EM_SetHandle(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	if (IsMultiLine(wndPtr)) {
		EDIT_ReleasePointer(wndPtr);
		/*
		 *	FIXME:	specs say: old buffer should be freed 
		 *		by the aplication, but e.g. Notepad doesn't.
		 *		Should we LOCAL_Free() the old hBuf ?
		 */
		LOCAL_Free(wndPtr->hInstance, es->hBuf);
		es->hBuf = (HLOCAL)wParam;
		es->BufSize = MIN(1, LOCAL_Size(wndPtr->hInstance, es->hBuf)) - 1;
		es->hBuf = LOCAL_ReAlloc(wndPtr->hInstance, es->hBuf, es->BufSize + 1, LMEM_MOVEABLE);
		es->LineCount = 0;
		es->FirstVisibleLine = 0;
		es->SelStart = es->SelEnd = 0;
		EDIT_EM_EmptyUndoBuffer(wndPtr, 0, 0L);
		EDIT_EM_SetModify(wndPtr, FALSE, 0L);
		EDIT_BuildLineDefs(wndPtr);
		if (EDIT_GetRedraw(wndPtr))
			InvalidateRect(wndPtr->hwndSelf, NULL, TRUE);
	}
	return 0L;
}


/*********************************************************************
 *
 *	EM_SETMODIFY
 *
 */
static LRESULT EDIT_EM_SetModify(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	es->TextChanged = wParam;
	return 0L;
}


/*********************************************************************
 *
 *	EM_SETPASSWORDCHAR
 *
 */
static LRESULT EDIT_EM_SetPasswordChar(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	es->PasswordChar = (char)wParam;
	return 0L;
}


/*********************************************************************
 *
 *	EM_SETREADONLY
 *
 */
static LRESULT EDIT_EM_SetReadOnly(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	if ((BOOL)wParam)
		wndPtr->dwStyle |= ES_READONLY;
	else
		wndPtr->dwStyle &= ~(DWORD)ES_READONLY;
	return 0L;
}


/*********************************************************************
 *
 *	EM_SETRECT
 *
 */
static LRESULT EDIT_EM_SetRect(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	fprintf(stdnimp,"edit: EM_SETRECT: message not implemented, please report.\n");
	return 0L;
}


/*********************************************************************
 *
 *	EM_SETRECTNP
 *
 */
static LRESULT EDIT_EM_SetRectNP(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	fprintf(stdnimp,"edit: EM_SETRECTNP: message not implemented, please report.\n");
	return 0L;
}


/*********************************************************************
 *
 *	EM_SETSEL
 *
 */
static LRESULT EDIT_EM_SetSel(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	int ns = (INT)LOWORD(lParam);
	int ne = (INT)HIWORD(lParam);
	int s = LOWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	int e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	int sl;
	int el;
	int sc;
	int ec;
	int elineindex;
	int tl = EDIT_WM_GetTextLength(wndPtr, 0, 0L);
	RECT rc;
	HRGN hRgn1, hRgn2;
	HRGN oldRgn, newRgn;

	if (ns < 0) {
		ne = e;
		ns = e;
	} else {
		ns = MIN(ns, tl);
		if (ne < 0)
			ne = tl;
		ne = MIN(ne, tl);
	}
	es->SelStart = ns;
	es->SelEnd = ne;
	if (wndPtr->hwndSelf == GetFocus()) {
		el = EDIT_EM_LineFromChar(wndPtr, ne, 0L);
		elineindex = EDIT_EM_LineIndex(wndPtr, el, 0L);
		SetCaretPos(EDIT_WndXFromCol(wndPtr, el, ne - elineindex),
				EDIT_WndYFromLine(wndPtr, el));
	}
	if (wParam)
		EDIT_ScrollIntoView(wndPtr);

	/*
	 *	Let's do some repainting
	 */
	ORDER_INT(s, e);
	ORDER_INT(ns, ne);
	if (EDIT_GetRedraw(wndPtr) && 
			!(((s == e) && (ns == ne)) ||
				((ns == s) && (ne == e)))) {
		/* Yes, there is something to paint */
		hRgn1 = CreateRectRgn(0, 0, 0, 0);
		hRgn2 = CreateRectRgn(0, 0, 0, 0);
		oldRgn = CreateRectRgn(0, 0, 0, 0);
		newRgn = CreateRectRgn(0, 0, 0, 0);
		/* Build the old selection region */
		sl = EDIT_EM_LineFromChar(wndPtr, s, 0L);
		el = EDIT_EM_LineFromChar(wndPtr, e, 0L);
		sc = s - EDIT_EM_LineIndex(wndPtr, sl, 0L);
		ec = e - EDIT_EM_LineIndex(wndPtr, el, 0L);
		if (sl == el) {
			EDIT_GetLineRect(wndPtr, sl, sc, ec, &rc);
			SetRectRgn(oldRgn, rc.left, rc.top, rc.right, rc.bottom);
		} else {
			EDIT_GetLineRect(wndPtr, sl, sc, -1, &rc);
			SetRectRgn(hRgn1, rc.left, rc.top, rc.right, rc.bottom);
			EDIT_GetLineRect(wndPtr, el, 0, ec, &rc);
			SetRectRgn(hRgn2, rc.left, rc.top, rc.right, rc.bottom);
			CombineRgn(oldRgn, hRgn1, hRgn2, RGN_OR);
			if (el > sl + 1) {
				EDIT_GetLineRect(wndPtr, sl + 1, 0, -1, &rc);
				rc.bottom = rc.top + (rc.bottom - rc.top) * (el - sl - 1);
				CombineRgn(hRgn1, oldRgn, 0, RGN_COPY);
				SetRectRgn(hRgn2, rc.left, rc.top, rc.right, rc.bottom);
				CombineRgn(oldRgn, hRgn1, hRgn2, RGN_OR);
			}
		}
		/* Build the new selection region */
		sl = EDIT_EM_LineFromChar(wndPtr, ns, 0L);
		el = EDIT_EM_LineFromChar(wndPtr, ne, 0L);
		sc = ns - EDIT_EM_LineIndex(wndPtr, sl, 0L);
		ec = ne - EDIT_EM_LineIndex(wndPtr, el, 0L);
		if (sl == el) {
			EDIT_GetLineRect(wndPtr, sl, sc, ec, &rc);
			SetRectRgn(newRgn, rc.left, rc.top, rc.right, rc.bottom);
		} else {
			EDIT_GetLineRect(wndPtr, sl, sc, -1, &rc);
			SetRectRgn(hRgn1, rc.left, rc.top, rc.right, rc.bottom);
			EDIT_GetLineRect(wndPtr, el, 0, ec, &rc);
			SetRectRgn(hRgn2, rc.left, rc.top, rc.right, rc.bottom);
			CombineRgn(newRgn, hRgn1, hRgn2, RGN_OR);
			if (el > sl + 1) {
				EDIT_GetLineRect(wndPtr, sl + 1, 0, -1, &rc);
				rc.bottom = rc.top + (rc.bottom - rc.top) * (el - sl - 1);
				CombineRgn(hRgn1, newRgn, 0, RGN_COPY);
				SetRectRgn(hRgn2, rc.left, rc.top, rc.right, rc.bottom);
				CombineRgn(newRgn, hRgn1, hRgn2, RGN_OR);
			}
		}
		/* Only difference needs painting */
		CombineRgn(hRgn1, oldRgn, newRgn, RGN_XOR);
		/* Only part in formatting RECT needs painting */
		
		InvalidateRgn(wndPtr->hwndSelf, hRgn1, FALSE);
		DeleteObject(hRgn1);
		DeleteObject(hRgn2);
		DeleteObject(oldRgn);
		DeleteObject(newRgn);		
	}
	return -1L;
}


/*********************************************************************
 *
 *	EM_SETTABSTOPS
 *
 */
static LRESULT EDIT_EM_SetTabStops(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	if (!IsMultiLine(wndPtr))
		return 0L;
	if (es->TabStops)
		free(es->TabStops);
	es->NumTabStops = wParam;
	if (wParam == 0)
		es->TabStops = NULL;
	else {
		es->TabStops = (unsigned short *)xmalloc(wParam * sizeof(unsigned short));
		memcpy(es->TabStops, (unsigned short *)PTR_SEG_TO_LIN(lParam),
				wParam * sizeof(unsigned short));
	}
	return 1L;
}


/*********************************************************************
 *
 *	EM_SETWORDBREAKPROC
 *
 */
static LRESULT EDIT_EM_SetWordBreakProc(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	es->WordBreakProc = (EDITWORDBREAKPROC)lParam;
	return 0L;
}


/*********************************************************************
 *
 *	EM_UNDO
 *
 */
static LRESULT EDIT_EM_Undo(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	return 0L;
}


/*********************************************************************
 *
 *	WM_CHAR
 *
 */
static LRESULT EDIT_WM_Char(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	char str[2];
	char c = (char)wParam;

	switch (c) {
	case '\r':
	case '\n':
		if (IsMultiLine(wndPtr)) {
			if (IsReadOnly(wndPtr)) {
				EDIT_MoveHome(wndPtr, FALSE);
				EDIT_MoveDownward(wndPtr, FALSE);
			} else
				EDIT_ReplaceSel(wndPtr, 0, (LPARAM)"\r\n");
		}
		break;
	case '\t':
		if (IsMultiLine(wndPtr) && !IsReadOnly(wndPtr))
			EDIT_ReplaceSel(wndPtr, 0, (LPARAM)"\t");
		break;
	default:
		if (!IsReadOnly(wndPtr) && (c >= ' ') && (c != 127)) {
 			str[0] = c;
 			str[1] = '\0';
 			EDIT_ReplaceSel(wndPtr, 0, (LPARAM)str);
 		}
		break;
	}
	return 0L;
}


/*********************************************************************
 *
 *	WM_CLEAR
 *
 */
static LRESULT EDIT_WM_Clear(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	int s = LOWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	int e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	char *text;
	BOOL redraw;
	
	if (s != e) {
		redraw = EDIT_GetRedraw(wndPtr);
		EDIT_WM_SetRedraw(wndPtr, FALSE, 0L);
		ORDER_INT(s, e);
		text = EDIT_GetPointer(wndPtr);
		strcpy(text + s, text + e);
		EDIT_BuildLineDefs(wndPtr);
		EDIT_EM_SetSel(wndPtr, TRUE, MAKELPARAM(s, s));
		EDIT_EM_SetModify(wndPtr, TRUE, 0L);
		EDIT_NOTIFY_PARENT(wndPtr, EN_UPDATE);
		EDIT_WM_SetRedraw(wndPtr, redraw, 0L);
		if (redraw) {
			InvalidateRect(wndPtr->hwndSelf, NULL, TRUE);
			EDIT_NOTIFY_PARENT(wndPtr, EN_CHANGE);
		}
	}
	return -1L;
}


/*********************************************************************
 *
 *	WM_COPY
 *
 */
static LRESULT EDIT_WM_Copy(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	int s = LOWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	int e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	HGLOBAL hdst;
	char *text;
	char *dst;
	char *src;
	int i;

	if (e == s)
		return -1L;
	ORDER_INT(s, e);
	hdst = GlobalAlloc(GMEM_MOVEABLE, (DWORD)(e - s + 1));
	dst = GlobalLock(hdst);
	text = EDIT_GetPointer(wndPtr);
	src = text + s;
	for (i = 0 ; i < e - s ; i++)
		*dst++ = *src++;
	*dst = '\0';
	GlobalUnlock(hdst);
	OpenClipboard(wndPtr->hwndSelf);
	EmptyClipboard();
	SetClipboardData(CF_TEXT, hdst);
	CloseClipboard();
	return -1L;
}


/*********************************************************************
 *
 *	WM_CREATE
 *
 */
static LRESULT EDIT_WM_Create(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	CREATESTRUCT *cs = (CREATESTRUCT *)PTR_SEG_TO_LIN(lParam);
	EDITSTATE *es;
	char *windowName = NULL;
	char *text;

	es = xmalloc(sizeof(EDITSTATE));
	memset(es, 0, sizeof(EDITSTATE));
	*(EDITSTATE **)wndPtr->wExtra = es;

	if (IsMultiLine(wndPtr)) {
		es->BufLimit = BUFLIMIT_MULTI;
		es->PasswordChar = '\0';
	} else {
		es->BufLimit = BUFLIMIT_SINGLE;
		es->PasswordChar = (cs->style & ES_PASSWORD) ? '*' : '\0';
	}

	es->BufSize = IsMultiLine(wndPtr) ? BUFSTART_MULTI : BUFSTART_SINGLE;
	if (cs->lpszName)
		windowName = (char *)PTR_SEG_TO_LIN(cs->lpszName);
	if (windowName)
		es->BufSize = MAX(es->BufSize, strlen(windowName));
	es->hBuf = LOCAL_Alloc(wndPtr->hInstance, LMEM_MOVEABLE, es->BufSize + 1);
	text = EDIT_GetPointer(wndPtr);
	if (!text) {
		fprintf(stderr, "edit: WM_CREATE: unable to heap buffer, please report !\n");
		return -1L;
	}
	if (windowName)
		strcpy(text, windowName);
	else
		text[0] = '\0';

	if (cs->style & WS_VSCROLL)
		cs->style |= ES_AUTOVSCROLL;
	if (cs->style & WS_HSCROLL)
		cs->style |= ES_AUTOHSCROLL;

	/* remove the WS_CAPTION style if it has been set - this is really a  */
	/* pseudo option made from a combination of WS_BORDER and WS_DLGFRAME */
	if ((cs->style & WS_BORDER) && (cs->style & WS_DLGFRAME))
		cs->style ^= WS_DLGFRAME;

	EDIT_WM_SetFont(wndPtr, 0, 0L);
	EDIT_WM_SetRedraw(wndPtr, TRUE, 0L);
	return 0L;
}


/*********************************************************************
 *
 *	WM_CUT
 *
 */
static LRESULT EDIT_WM_Cut(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	EDIT_WM_Copy(wndPtr, 0, 0L);
	EDIT_WM_Clear(wndPtr, 0, 0L);
	return -1L;
}


/*********************************************************************
 *
 *	WM_DESTROY
 *
 */
static LRESULT EDIT_WM_Destroy(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	free(es->LineDefs);
	if (es->TabStops)
		free(es->TabStops);
	EDIT_ReleasePointer(wndPtr);
	LOCAL_Free(wndPtr->hInstance, es->hBuf);
	free(es);
	*(EDITSTATE **)&wndPtr->wExtra = NULL;
	return 0L;
}


/*********************************************************************
 *
 *	WM_ENABLE
 *
 */
static LRESULT EDIT_WM_Enable(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	return 0L;
}


/*********************************************************************
 *
 *	WM_ERASEBKGND
 *
 */
static LRESULT EDIT_WM_EraseBkGnd(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	HBRUSH hBrush;
	RECT rc;

	hBrush = (HBRUSH)EDIT_SEND_CTLCOLOR(wndPtr, wParam);

	GetClientRect(wndPtr->hwndSelf, &rc);
	IntersectClipRect((HDC)wParam, rc.left, rc.top, rc.right, rc.bottom);
	GetClipBox((HDC)wParam, &rc);
	/*
	 *	FIXME:	specs say that we should UnrealizeObject() the brush,
	 *		but the specs of UnrealizeObject() say that we shouldn't
	 *		unrealize a stock object.  The default brush that
	 *		DefWndProc() returns is ... a stock object.
	 */
	FillRect((HDC)wParam, &rc, hBrush);
	return -1L;
}


/*********************************************************************
 *
 *	WM_GETDLGCODE
 *
 */
static LRESULT EDIT_WM_GetDlgCode(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	return DLGC_HASSETSEL | DLGC_WANTCHARS | DLGC_WANTARROWS;
}


/*********************************************************************
 *
 *	WM_GETFONT
 *
 */
static LRESULT EDIT_WM_GetFont(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	return (LRESULT)es->hFont;
}


/*********************************************************************
 *
 *	WM_GETTEXT
 *
 */
static LRESULT EDIT_WM_GetText(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	char *text = EDIT_GetPointer(wndPtr);
	int len;
	LRESULT lResult = 0L;

	len = strlen(text);
	if ((int)wParam > len) {
		strcpy((char *)PTR_SEG_TO_LIN(lParam), text);
		lResult = (LRESULT)len ;
	}
	return lResult;
}


/*********************************************************************
 *
 *	WM_GETTEXTLENGTH
 *
 */
static LRESULT EDIT_WM_GetTextLength(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	LRESULT ret;
	char *text = EDIT_GetPointer(wndPtr);

	ret = (LRESULT)strlen(text);
	return ret;
}
 

/*********************************************************************
 *
 *	WM_HSCROLL
 *
 *	FIXME: scrollbar code itself is broken, so this one is a hack.
 *
 */
static LRESULT EDIT_WM_HScroll(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	int wndwidth = EDIT_GetWndWidth(wndPtr);
	int textwidth = EDIT_GetTextWidth(wndPtr);
	int charwidth = EDIT_GetAveCharWidth(wndPtr);
	int xoffset = EDIT_GetXOffset(wndPtr);
	int dx = 0;
	BOOL not = TRUE;
	LRESULT ret = 0L;

	switch (wParam) {
	case SB_LINELEFT:
		dx = -charwidth;
		break;
	case SB_LINERIGHT:
		dx = charwidth;
		break;
	case SB_PAGELEFT:
		dx = -wndwidth / HSCROLL_FRACTION / charwidth * charwidth;
		break;
	case SB_PAGERIGHT:
		dx = wndwidth / HSCROLL_FRACTION / charwidth * charwidth;
		break;
	case SB_LEFT:
		dx = -xoffset;
		break;
	case SB_RIGHT:
		dx = textwidth - xoffset;
		break;
	case SB_THUMBTRACK:
/*
 *		not = FALSE;
 */
	case SB_THUMBPOSITION:
		dx = LOWORD(lParam) * textwidth / 100 - xoffset;
		break;
	/* The next two are undocumented ! */
	case EM_GETTHUMB:
		ret = textwidth ? MAKELONG(xoffset * 100 / textwidth, 0) : 0;
		break;
	case EM_LINESCROLL:
		dx = LOWORD(lParam);
		break;
	case SB_ENDSCROLL:
	default:
		break;
	}
	if (dx) {
		EDIT_EM_LineScroll(wndPtr, 0, MAKELPARAM(0, dx));
		if (not)
			EDIT_NOTIFY_PARENT(wndPtr, EN_HSCROLL);
	}
	return ret;
}


/*********************************************************************
 *
 *	WM_KEYDOWN
 *
 *	Handling of special keys that don't produce a WM_CHAR
 *	(i.e. non-printable keys) & Backspace & Delete
 *
 */
static LRESULT EDIT_WM_KeyDown(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	int s = LOWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	int e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	BOOL shift;
	BOOL control;

	if (GetKeyState(VK_MENU) & 0x8000)
		return 0L;

	shift = GetKeyState(VK_SHIFT) & 0x8000;
	control = GetKeyState(VK_CONTROL) & 0x8000;

	switch (wParam) {
	case VK_LEFT:
	case VK_UP:
		if (IsMultiLine(wndPtr) && (wParam == VK_UP))
			EDIT_MoveUpward(wndPtr, shift);
		else
			if (control)
				EDIT_MoveWordBackward(wndPtr, shift);
			else
				EDIT_MoveBackward(wndPtr, shift);
		break;
	case VK_RIGHT:
	case VK_DOWN:
		if (IsMultiLine(wndPtr) && (wParam == VK_DOWN))
			EDIT_MoveDownward(wndPtr, shift);
		else if (control)
			EDIT_MoveWordForward(wndPtr, shift);
		else
			EDIT_MoveForward(wndPtr, shift);
		break;
	case VK_HOME:
		EDIT_MoveHome(wndPtr, shift);
		break;
	case VK_END:
		EDIT_MoveEnd(wndPtr, shift);
		break;
	case VK_PRIOR:
		if (IsMultiLine(wndPtr))
			EDIT_MovePageUp(wndPtr, shift);
		break;
	case VK_NEXT:
		if (IsMultiLine(wndPtr))
			EDIT_MovePageDown(wndPtr, shift);
		break;
	case VK_BACK:
		if (!IsReadOnly(wndPtr) && !control)
			if (e != s)
				EDIT_WM_Clear(wndPtr, 0, 0L);
			else
				EDIT_DelLeft(wndPtr);
		break;
	case VK_DELETE:
		if (!IsReadOnly(wndPtr) && !(shift && control))
			if (e != s) {
				if (shift)
					EDIT_WM_Cut(wndPtr, 0, 0L);
				else
					EDIT_WM_Clear(wndPtr, 0, 0L);
			} else {
				if (shift)
					EDIT_DelLeft(wndPtr);
				else if (control)
					EDIT_DelEnd(wndPtr);
				else
					EDIT_DelRight(wndPtr);
			}
		break;
	case VK_INSERT:
		if (shift) {
			if (!IsReadOnly(wndPtr))
				EDIT_WM_Paste(wndPtr, 0, 0L);
		} else if (control)
			EDIT_WM_Copy(wndPtr, 0, 0L);
		break;
	}
	return 0L;
}


/*********************************************************************
 *
 *	WM_KILLFOCUS
 *
 */
static LRESULT EDIT_WM_KillFocus(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	DestroyCaret();
	if(!(wndPtr->dwStyle & ES_NOHIDESEL))
		EDIT_EM_SetSel(wndPtr, FALSE, MAKELPARAM(-1, 0));
	EDIT_NOTIFY_PARENT(wndPtr, EN_KILLFOCUS);
	return 0L;
}


/*********************************************************************
 *
 *	WM_LBUTTONDBLCLK
 *
 */
static LRESULT EDIT_WM_LButtonDblClk(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	int s;
	int e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	int l = EDIT_EM_LineFromChar(wndPtr, e, 0L);
	int lineindex = EDIT_EM_LineIndex(wndPtr, l, 0L);
	int linelength = EDIT_EM_LineLength(wndPtr, e, 0L);
	char *text = EDIT_GetPointer(wndPtr);

	s = lineindex + EDIT_CallWordBreakProc(wndPtr, text + lineindex, e - lineindex, linelength, WB_LEFT);
	e = lineindex + EDIT_CallWordBreakProc(wndPtr, text + lineindex, e - lineindex, linelength, WB_RIGHT);
	EDIT_EM_SetSel(wndPtr, TRUE, MAKELPARAM(s, e));
	return 0L;
}


/*********************************************************************
 *
 *	WM_LBUTTONDOWN
 *
 */
static LRESULT EDIT_WM_LButtonDown(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	int l;
	int s;
	int e;

	SetFocus(wndPtr->hwndSelf);
	SetCapture(wndPtr->hwndSelf);
	l = EDIT_LineFromWndY(wndPtr, (INT)HIWORD(lParam));
	e = EDIT_EM_LineIndex(wndPtr, l ,0L) +
			EDIT_ColFromWndX(wndPtr, l, (INT)LOWORD(lParam));
	if (GetKeyState(VK_SHIFT) & 0x8000)
		s = LOWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	else 
		s = e;
	EDIT_EM_SetSel(wndPtr, TRUE, MAKELPARAM(s, e));
	return 0L;
}


/*********************************************************************
 *
 *	WM_LBUTTONUP
 *
 */
static LRESULT EDIT_WM_LButtonUp(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	if (GetCapture() == wndPtr->hwndSelf)
		ReleaseCapture();
	return 0L;
}


/*********************************************************************
 *
 *	WM_MOUSEMOVE
 *
 */
static LRESULT EDIT_WM_MouseMove(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	int Line;
	int Col;
	int x = (INT)LOWORD(lParam);

	if (GetCapture() == wndPtr->hwndSelf) {
		Line = EDIT_LineFromWndY(wndPtr, (INT)HIWORD(lParam));
		Line = MAX(Line, EDIT_EM_GetFirstVisibleLine(wndPtr, 0, 0L));
		Line = MIN(Line, EDIT_EM_GetFirstVisibleLine(wndPtr, 0, 0L) + EDIT_GetVisibleLineCount(wndPtr) - 1);
		x = MIN(EDIT_GetWndWidth(wndPtr), MAX(0, x));
		Col = EDIT_ColFromWndX(wndPtr, Line, x);
		EDIT_EM_SetSel(wndPtr, FALSE, MAKELPARAM(es->SelStart,
				EDIT_EM_LineIndex(wndPtr, Line, 0L) + Col));
	}
	return 0L;
}


/*********************************************************************
 *
 *	WM_PAINT
 *
 */
static LRESULT EDIT_WM_Paint(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	PAINTSTRUCT ps;
	int i;
	int firstvis = EDIT_EM_GetFirstVisibleLine(wndPtr, 0, 0L);
	HDC hdc;
	HFONT hFont;
	HFONT oldFont = 0;
	RECT rc;
	RECT rcLine;
	RECT rcRgn;

	hdc = BeginPaint(wndPtr->hwndSelf, &ps);
	GetClientRect(wndPtr->hwndSelf, &rc);
	IntersectClipRect(hdc, rc.left, rc.top, rc.right, rc.bottom);
	hFont = EDIT_WM_GetFont(wndPtr, 0, 0L);
	if (hFont)
		oldFont = SelectObject(hdc, hFont);
	EDIT_SEND_CTLCOLOR(wndPtr, hdc);
	GetClipBox(hdc, &rcRgn);
	for (i = firstvis ; i <= MIN(firstvis + EDIT_GetVisibleLineCount(wndPtr), firstvis + es->LineCount - 1) ; i++ ) {
		EDIT_GetLineRect(wndPtr, i, 0, -1, &rcLine);
		if (IntersectRect(&rc, &rcRgn, &rcLine))
			EDIT_PaintLine(wndPtr, hdc, i);
	}
	if (hFont)
		SelectObject(hdc, oldFont);
	EndPaint(wndPtr->hwndSelf, &ps);
	return 0L;
}


/*********************************************************************
 *
 *	WM_PASTE
 *
 */
static LRESULT EDIT_WM_Paste(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	HGLOBAL hsrc;
	char *src;

	OpenClipboard(wndPtr->hwndSelf);
	if ((hsrc = GetClipboardData(CF_TEXT))) {
		src = (char *)GlobalLock(hsrc);
		EDIT_ReplaceSel(wndPtr, 0, (LPARAM)src);
		GlobalUnlock(hsrc);
	}
	CloseClipboard();
	return -1L;
}


/*********************************************************************
 *
 *	WM_SETCURSOR
 *
 */
static LRESULT EDIT_WM_SetCursor(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	if (LOWORD(lParam) == HTCLIENT) {
		SetCursor(LoadCursor(0, IDC_IBEAM));
		return -1L;
	} else
		return 0L;
}


/*********************************************************************
 *
 *	WM_SETFOCUS
 *
 */
static LRESULT EDIT_WM_SetFocus(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	LPARAM sel = EDIT_EM_GetSel(wndPtr, 0, 0L);

	CreateCaret(wndPtr->hwndSelf, 0, 2, EDIT_GetLineHeight(wndPtr));
	EDIT_EM_SetSel(wndPtr, FALSE, sel);
	ShowCaret(wndPtr->hwndSelf);
	EDIT_NOTIFY_PARENT(wndPtr, EN_SETFOCUS);
	return 0L;
}


/*********************************************************************
 *
 *	WM_SETFONT
 *
 */
static LRESULT EDIT_WM_SetFont(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	TEXTMETRIC tm;
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	LPARAM sel = EDIT_EM_GetSel(wndPtr, 0, 0L);
	HDC hdc;
	HFONT oldFont = 0;

	es->hFont = (HFONT)wParam;
	hdc = GetDC(wndPtr->hwndSelf);
	if (es->hFont)
		oldFont = SelectObject(hdc, es->hFont);
	GetTextMetrics(hdc, &tm);
	es->LineHeight = HIWORD(GetTextExtent(hdc, "", 0));
	es->AveCharWidth = tm.tmAveCharWidth;
	if (es->hFont)
		SelectObject(hdc, oldFont);
	ReleaseDC(wndPtr->hwndSelf, hdc);
	EDIT_BuildLineDefs(wndPtr);
	if (lParam && EDIT_GetRedraw(wndPtr))
		InvalidateRect(wndPtr->hwndSelf, NULL, TRUE);
	if (wndPtr->hwndSelf == GetFocus()) {
		DestroyCaret();
		CreateCaret(wndPtr->hwndSelf, 0, 2, EDIT_GetLineHeight(wndPtr));
		EDIT_EM_SetSel(wndPtr, FALSE, sel);
		ShowCaret(wndPtr->hwndSelf);
	}
	return 0L;
}


/*********************************************************************
 *
 *	WM_SETREDRAW
 *
 */
static LRESULT EDIT_WM_SetRedraw(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	es->Redraw = wParam ? TRUE : FALSE;
	return 0L;
}


/*********************************************************************
 *
 *	WM_SETTEXT
 *
 */
static LRESULT EDIT_WM_SetText(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	EDIT_EM_SetSel(wndPtr, FALSE, MAKELPARAM(0, -1));
	EDIT_WM_Clear(wndPtr, 0, 0L);
	if (lParam)
		EDIT_EM_ReplaceSel(wndPtr, 0, lParam);
	EDIT_EM_EmptyUndoBuffer(wndPtr, 0, 0L);
	EDIT_EM_SetModify(wndPtr, TRUE, 0L);
	return 0L;
}


/*********************************************************************
 *
 *	WM_SIZE
 *
 */
static LRESULT EDIT_WM_Size(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	if (EDIT_GetRedraw(wndPtr) &&
			((wParam == SIZE_MAXIMIZED) ||
				(wParam == SIZE_RESTORED)))
		InvalidateRect(wndPtr->hwndSelf, NULL, TRUE);
	return 0L;
}


/*********************************************************************
 *
 *	WM_VSCROLL
 *
 *	FIXME: scrollbar code itself is broken, so this one is a hack.
 *
 */
static LRESULT EDIT_WM_VScroll(WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	int linecount = EDIT_EM_GetLineCount(wndPtr, 0, 0L);
	int firstvis = EDIT_EM_GetFirstVisibleLine(wndPtr, 0, 0L);
	int vislinecount = EDIT_GetVisibleLineCount(wndPtr);
	int dy = 0;
	BOOL not = TRUE;
	LRESULT ret = 0L;

	switch (wParam) {
	case SB_LINEUP:
		dy = -1;
		break;
	case SB_LINEDOWN:
		dy = 1;
		break;
	case SB_PAGEUP:
		dy = -vislinecount;
		break;
	case SB_PAGEDOWN:
		dy = vislinecount;
		break;
	case SB_TOP:
		dy = -firstvis;
		break;
	case SB_BOTTOM:
		dy = linecount - 1 - firstvis;
		break;
	case SB_THUMBTRACK:
/*
 *		not = FALSE;
 */
	case SB_THUMBPOSITION:
		dy = LOWORD(lParam) * (linecount - 1) / 100 - firstvis;
		break;
	/* The next two are undocumented ! */
	case EM_GETTHUMB:
		ret = (linecount > 1) ? MAKELONG(firstvis * 100 / (linecount - 1), 0) : 0L;
		break;
	case EM_LINESCROLL:
		dy = LOWORD(lParam);
		break;
	case SB_ENDSCROLL:
	default:
		break;
	}
	if (dy) {
		EDIT_EM_LineScroll(wndPtr, 0, MAKELPARAM(dy, 0));
		if (not)
			EDIT_NOTIFY_PARENT(wndPtr, EN_VSCROLL);
	}
	return ret;
}
