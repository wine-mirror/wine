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
#include "windows.h"
#include "win.h"
#include "local.h"
#include "stddebug.h"
#include "debug.h"
#include "xmalloc.h"
#include "callback.h"

#define BUFLIMIT_MULTI		65534	/* maximum text buffer length (not including '\0') */
#define BUFLIMIT_SINGLE		32766
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
	UINT offset;
	UINT length;
	LINE_END ending;
} LINEDEF;

typedef struct
{
	UINT TextWidth;		/* width of the widest line in pixels */
	HLOCAL16 hBuf;
	char *text;
	HFONT16 hFont;
	LINEDEF *LineDefs;
	UINT XOffset;		/* offset of the viewport in pixels */
	UINT FirstVisibleLine;
	UINT LineCount;
	UINT LineHeight;	/* height of a screen line in pixels */
	UINT AveCharWidth;	/* average character width in pixels */
	UINT BufLimit;
	UINT BufSize;
	BOOL TextChanged;
	BOOL Redraw;
	UINT SelStart;		/* offset of selection start, == SelEnd if no selection */
	UINT SelEnd;		/* offset of selection end == current caret position */
	UINT NumTabStops;
	LPINT16 TabStops;
	EDITWORDBREAKPROC WordBreakProc;
	char PasswordChar;
} EDITSTATE;


#define SWAP_UINT(x,y) do { UINT temp = (UINT)(x); (x) = (UINT)(y); (y) = temp; } while(0)
#define ORDER_UINT(x,y) do { if ((UINT)(y) < (UINT)(x)) SWAP_UINT((x),(y)); } while(0)

/* macros to access window styles */
#define IsMultiLine(wndPtr) ((wndPtr)->dwStyle & ES_MULTILINE)
#define IsVScrollBar(wndPtr) ((wndPtr)->dwStyle & WS_VSCROLL)
#define IsHScrollBar(wndPtr) ((wndPtr)->dwStyle & WS_HSCROLL)
#define IsReadOnly(wndPtr) ((wndPtr)->dwStyle & ES_READONLY)
#define IsWordWrap(wndPtr) (((wndPtr)->dwStyle & ES_AUTOHSCROLL) == 0)
#define IsPassword(wndPtr) ((wndPtr)->dwStyle & ES_PASSWORD)
#define IsLower(wndPtr) ((wndPtr)->dwStyle & ES_LOWERCASE)
#define IsUpper(wndPtr) ((wndPtr)->dwStyle & ES_UPPERCASE)

#define EDITSTATEPTR(wndPtr) (*(EDITSTATE **)((wndPtr)->wExtra))

#define EDIT_SEND_CTLCOLOR(wndPtr,hdc) \
    (SendMessage32A((wndPtr)->parent->hwndSelf, WM_CTLCOLOREDIT, \
                    (WPARAM32)(hdc), (LPARAM)(wndPtr)->hwndSelf ))
#define EDIT_NOTIFY_PARENT(wndPtr, wNotifyCode) \
    (SendMessage32A((wndPtr)->parent->hwndSelf, WM_COMMAND, \
                    MAKEWPARAM((wndPtr)->wIDmenu, wNotifyCode), \
                    (LPARAM)(wndPtr)->hwndSelf ))
#define DPRINTF_EDIT_MSG(str) \
    dprintf_edit(stddeb, \
                 "edit: " str ": hwnd=%04x, wParam=%04x, lParam=%08x\n", \
                 (UINT32)hwnd, (UINT32)wParam, (UINT32)lParam)


/*********************************************************************
 *
 *	Declarations
 *
 *	Files like these should really be kept in alphabetical order.
 *
 */
LRESULT EditWndProc(HWND hwnd, UINT msg, WPARAM16 wParam, LPARAM lParam);

static void    EDIT_BuildLineDefs(WND *wndPtr);
static INT     EDIT_CallWordBreakProc(WND *wndPtr, char *s, INT index, INT count, INT action);
static UINT    EDIT_ColFromWndX(WND *wndPtr, UINT line, INT x);
static void    EDIT_DelEnd(WND *wndPtr);
static void    EDIT_DelLeft(WND *wndPtr);
static void    EDIT_DelRight(WND *wndPtr);
static UINT    EDIT_GetAveCharWidth(WND *wndPtr);
static UINT    EDIT_GetLineHeight(WND *wndPtr);
static void    EDIT_GetLineRect(WND *wndPtr, UINT line, UINT scol, UINT ecol, LPRECT16 rc);
static char *  EDIT_GetPointer(WND *wndPtr);
static char *  EDIT_GetPasswordPointer(WND *wndPtr);
static LRESULT EDIT_GetRect(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static BOOL    EDIT_GetRedraw(WND *wndPtr);
static UINT    EDIT_GetTextWidth(WND *wndPtr);
static UINT    EDIT_GetVisibleLineCount(WND *wndPtr);
static UINT    EDIT_GetWndWidth(WND *wndPtr);
static UINT    EDIT_GetXOffset(WND *wndPtr);
static void    EDIT_InvalidateText(WND *wndPtr, UINT start, UINT end);
static UINT    EDIT_LineFromWndY(WND *wndPtr, INT y);
static BOOL    EDIT_MakeFit(WND *wndPtr, UINT size);
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
static void    EDIT_PaintLine(WND *wndPtr, HDC32 hdc, UINT line, BOOL rev);
static UINT    EDIT_PaintText(WND *wndPtr, HDC32 hdc, INT x, INT y, UINT line, UINT col, UINT count, BOOL rev);
static void    EDIT_ReleasePointer(WND *wndPtr);
static LRESULT EDIT_ReplaceSel(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static void    EDIT_ScrollIntoView(WND *wndPtr);
static INT     EDIT_WndXFromCol(WND *wndPtr, UINT line, UINT col);
static INT     EDIT_WndYFromLine(WND *wndPtr, UINT line);
static INT     EDIT_WordBreakProc(char *s, INT index, INT count, INT action);

static LRESULT EDIT_EM_CanUndo(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_EmptyUndoBuffer(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_FmtLines(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_GetFirstVisibleLine(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_GetHandle(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_GetLine(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_GetLineCount(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_GetModify(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_GetPasswordChar(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_GetRect(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_GetSel(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_GetThumb(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_GetWordBreakProc(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_LimitText(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_LineFromChar(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_LineIndex(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_LineLength(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_LineScroll(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_ReplaceSel(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_Scroll(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_SetHandle(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_SetModify(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_SetPasswordChar(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_SetReadOnly(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_SetRect(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_SetRectNP(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_SetSel(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_SetTabStops(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_SetWordBreakProc(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_Undo(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);

static LRESULT EDIT_WM_Char(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_WM_Clear(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_WM_Copy(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_WM_Cut(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_WM_Create(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_WM_Destroy(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_WM_Enable(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_WM_EraseBkGnd(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_WM_GetDlgCode(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_WM_GetFont(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_WM_GetText(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_WM_GetTextLength(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_WM_HScroll(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_WM_KeyDown(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_WM_KillFocus(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_WM_LButtonDblClk(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_WM_LButtonDown(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_WM_LButtonUp(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_WM_MouseMove(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_WM_Paint(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_WM_Paste(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_WM_SetCursor(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_WM_SetFocus(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_WM_SetFont(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_WM_SetRedraw(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_WM_SetText(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_WM_Size(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_WM_VScroll(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);


/*********************************************************************
 *
 *	General shortcuts for variable names:
 *
 *	UINT l;		line
 *	UINT c;		column
 *	UINT s;		offset of selection start
 *	UINT e;		offset of selection end
 *	UINT sl;	line on which the selection starts
 *	UINT el;	line on which the selection ends
 *	UINT sc;	column on which the selection starts
 *	UINT ec;	column on which the selection ends
 *	UINT li;	line index (offset)
 *	UINT fv;	first visible line
 *	UINT vlc;	vissible line count
 *	UINT lc;	line count
 *	UINT lh;	line height (in pixels)
 *	UINT tw;	text width (in pixels)
 *	UINT ww;	window width (in pixels)
 *	UINT cw;	character width (average, in pixels)
 *
 */


/*********************************************************************
 *
 *	EditWndProc()
 *
 */
LRESULT EditWndProc(HWND hwnd, UINT msg, WPARAM16 wParam, LPARAM lParam)
{
	LRESULT lResult = 0L;
	WND *wndPtr = WIN_FindWndPtr(hwnd);

	if ((!EDITSTATEPTR(wndPtr)) && (msg != WM_CREATE))
		return DefWindowProc16(hwnd, msg, wParam, lParam);

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
                /* Some programs pass messages obtained through
                 * RegisterWindowMessage() (>= 0xc000); we just ignore them
                 */
		if ((msg >= WM_USER) && (msg < 0xc000))
			fprintf(stdnimp, "edit: undocumented message %d >= WM_USER, please report.\n", msg);
		lResult = DefWindowProc16(hwnd, msg, wParam, lParam);
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
 *	Lines can end with '\0' (last line), nothing (if it is too long),
 *	a delimiter (usually ' '), a soft return '\r\r\n' or a hard return '\r\n'
 *
 */
static void EDIT_BuildLineDefs(WND *wndPtr)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	char *text = EDIT_GetPasswordPointer(wndPtr);
	int ww = EDIT_GetWndWidth(wndPtr);
	HDC32 hdc;
	HFONT16 hFont;
	HFONT16 oldFont = 0;
	char *start, *cp;
	int prev, next;
	int width;
	int length;
	LINE_END ending;

	hdc = GetDC32(wndPtr->hwndSelf);
	hFont = (HFONT16)EDIT_WM_GetFont(wndPtr, 0, 0L);
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
				if (!prev) {
					next = 0;
					do {
						prev = next;
						next++;
						width = LOWORD(GetTabbedTextExtent(hdc, start, next,
								es->NumTabStops, es->TabStops));
					} while (width <= ww);
					if(!prev) prev = 1;
				}
				length = prev;
				if (EDIT_CallWordBreakProc(wndPtr, start, length - 1,
								length, WB_ISDELIMITER)) {
					length--;
					ending = END_DELIMIT;
				} else
					ending = END_NONE;
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
	ReleaseDC32(wndPtr->hwndSelf, hdc);

	free(text);
}


/*********************************************************************
 *
 *	EDIT_CallWordBreakProc
 *
 *	Call appropriate WordBreakProc (internal or external).
 *
 */
static INT EDIT_CallWordBreakProc(WND *wndPtr, char *s, INT index, INT count, INT action)
{
    EDITWORDBREAKPROC wbp = (EDITWORDBREAKPROC)EDIT_EM_GetWordBreakProc(wndPtr, 0, 0L);

    if (!wbp) return EDIT_WordBreakProc(s, index, count, action);
    else
    {
        /* We need a SEGPTR here */

        EDITSTATE *es = EDITSTATEPTR(wndPtr);
        SEGPTR ptr = LOCAL_LockSegptr( wndPtr->hInstance, es->hBuf ) +
                     (UINT16)(s - EDIT_GetPointer(wndPtr));
        INT ret = CallWordBreakProc( (FARPROC16)wbp, ptr,
                                     index, count, action);
        LOCAL_Unlock( wndPtr->hInstance, es->hBuf );
        return ret;
    }
}


/*********************************************************************
 *
 *	EDIT_ColFromWndX
 *
 *	Calculates, for a given line and X-coordinate on the screen, the column.
 *
 */
static UINT EDIT_ColFromWndX(WND *wndPtr, UINT line, INT x)
{	
	UINT lc = (UINT)EDIT_EM_GetLineCount(wndPtr, 0, 0L);
	UINT li = (UINT)EDIT_EM_LineIndex(wndPtr, line, 0L);
	UINT ll = (UINT)EDIT_EM_LineLength(wndPtr, li, 0L);
	UINT i;

	line = MAX(0, MIN(line, lc - 1));
	for (i = 0 ; i < ll ; i++)
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
	EDIT_EM_SetSel(wndPtr, 1, MAKELPARAM(-1, 0));
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
	EDIT_EM_SetSel(wndPtr, 1, MAKELPARAM(-1, 0));
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
	EDIT_EM_SetSel(wndPtr, 1, MAKELPARAM(-1, 0));
	EDIT_MoveForward(wndPtr, TRUE);
	EDIT_WM_Clear(wndPtr, 0, 0L);
}


/*********************************************************************
 *
 *	EDIT_GetAveCharWidth
 *
 */
static UINT EDIT_GetAveCharWidth(WND *wndPtr)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	
	return es->AveCharWidth;
}


/*********************************************************************
 *
 *	EDIT_GetLineHeight
 *
 */
static UINT EDIT_GetLineHeight(WND *wndPtr)
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
static void EDIT_GetLineRect(WND *wndPtr, UINT line, UINT scol, UINT ecol, LPRECT16 rc)
{
	rc->top = EDIT_WndYFromLine(wndPtr, line);
	rc->bottom = rc->top + EDIT_GetLineHeight(wndPtr);
	rc->left = EDIT_WndXFromCol(wndPtr, line, scol);
	rc->right = ((INT)ecol == -1) ? EDIT_GetWndWidth(wndPtr) :
				EDIT_WndXFromCol(wndPtr, line, ecol);
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
 *	EDIT_GetPasswordPointer
 *
 *
 */
static char *EDIT_GetPasswordPointer(WND *wndPtr)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	char *text = xstrdup(EDIT_GetPointer(wndPtr));
	char *p;

	if(es->PasswordChar) {
		p = text;
		while(*p != '\0') {
			if(*p != '\r' && *p != '\n')
				*p = es->PasswordChar;
			p++;
		}
	}
	return text;
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
static LRESULT EDIT_GetRect(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	GetClientRect16( wndPtr->hwndSelf, (LPRECT16)lParam );
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
static UINT EDIT_GetTextWidth(WND *wndPtr)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	
	return es->TextWidth;
}


/*********************************************************************
 *
 *	EDIT_GetVisibleLineCount
 *
 */
static UINT EDIT_GetVisibleLineCount(WND *wndPtr)
{
	RECT16 rc;
	
	EDIT_GetRect(wndPtr, 0, (LPARAM)&rc);
	return MAX(1, MAX(rc.bottom - rc.top, 0) / EDIT_GetLineHeight(wndPtr));
}


/*********************************************************************
 *
 *	EDIT_GetWndWidth
 *
 */
static UINT EDIT_GetWndWidth(WND *wndPtr)
{
	RECT16 rc;
	
	EDIT_GetRect(wndPtr, 0, (LPARAM)&rc);
	return rc.right - rc.left;
}


/*********************************************************************
 *
 *	EDIT_GetXOffset
 *
 */
static UINT EDIT_GetXOffset(WND *wndPtr)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	
	return es->XOffset;
}


/*********************************************************************
 *
 *	EDIT_InvalidateText
 *
 *	Invalidate the text from offset start upto, but not including,
 *	offset end.  Useful for (re)painting the selection.
 *	Regions outside the linewidth are not invalidated.
 *	end == -1 means end == TextLength.
 *	start and end need not be ordered.
 *
 */
static void EDIT_InvalidateText(WND *wndPtr, UINT start, UINT end)
{
	UINT fv = (UINT)EDIT_EM_GetFirstVisibleLine(wndPtr, 0, 0L);
	UINT vlc = EDIT_GetVisibleLineCount(wndPtr);
	UINT sl;
	UINT el;
	UINT sc;
	UINT ec;
	RECT16 rcWnd;
	RECT16 rcLine;
	RECT16 rcUpdate;
	UINT l;

	if (end == start )
		return;

	if ((INT)end == -1)
		end = (UINT)EDIT_WM_GetTextLength(wndPtr, 0, 0L);
	ORDER_UINT(start, end);
	sl = (UINT)EDIT_EM_LineFromChar(wndPtr, start, 0L);
	el = (UINT)EDIT_EM_LineFromChar(wndPtr, end, 0L);
	if ((el < fv) || (sl > fv + vlc))
		return;

	sc = start - (UINT)EDIT_EM_LineIndex(wndPtr, sl, 0L);
	ec = end - (UINT)EDIT_EM_LineIndex(wndPtr, el, 0L);
	if (sl < fv) {
		sl = fv;
		sc = 0;
	}
	if (el > fv + vlc) {
		el = fv + vlc;
		ec = (UINT)EDIT_EM_LineLength(wndPtr,
				(UINT)EDIT_EM_LineIndex(wndPtr, el, 0L), 0L);
	}
	EDIT_GetRect(wndPtr, 0, (LPARAM)&rcWnd);
	if (sl == el) {
		EDIT_GetLineRect(wndPtr, sl, sc, ec, &rcLine);
		if (IntersectRect16(&rcUpdate, &rcWnd, &rcLine))
			InvalidateRect16( wndPtr->hwndSelf, &rcUpdate, FALSE );
	} else {
		EDIT_GetLineRect(wndPtr, sl, sc,
				(UINT)EDIT_EM_LineLength(wndPtr,
					(UINT)EDIT_EM_LineIndex(wndPtr, sl, 0L), 0L),
				&rcLine);
		if (IntersectRect16(&rcUpdate, &rcWnd, &rcLine))
			InvalidateRect16( wndPtr->hwndSelf, &rcUpdate, FALSE );
		for (l = sl + 1 ; l < el ; l++) {
			EDIT_GetLineRect(wndPtr, l, 0,
				(UINT)EDIT_EM_LineLength(wndPtr,
					(UINT)EDIT_EM_LineIndex(wndPtr, l, 0L), 0L),
				&rcLine);
			if (IntersectRect16(&rcUpdate, &rcWnd, &rcLine))
				InvalidateRect16(wndPtr->hwndSelf, &rcUpdate, FALSE);
		}
		EDIT_GetLineRect(wndPtr, el, 0, ec, &rcLine);
		if (IntersectRect16(&rcUpdate, &rcWnd, &rcLine))
			InvalidateRect16( wndPtr->hwndSelf, &rcUpdate, FALSE );
	}
}


/*********************************************************************
 *
 *	EDIT_LineFromWndY
 *
 *	Calculates, for a given Y-coordinate on the screen, the line.
 *
 */
static UINT EDIT_LineFromWndY(WND *wndPtr, INT y)
{
	UINT fv = (UINT)EDIT_EM_GetFirstVisibleLine(wndPtr, 0, 0L);
	UINT lh = EDIT_GetLineHeight(wndPtr);
	UINT lc = (UINT)EDIT_EM_GetLineCount(wndPtr, 0, 0L);

	return MAX(0, MIN(lc - 1, y / lh + fv));
}


/*********************************************************************
 *
 *	EDIT_MakeFit
 *
 *	Try to fit size + 1 bytes in the buffer.  Constrain to limits.
 *
 */
static BOOL EDIT_MakeFit(WND *wndPtr, UINT size)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	if (size <= es->BufSize)
		return TRUE;
	if (size > es->BufLimit)
		return FALSE;
	size = ((size / GROWLENGTH) + 1) * GROWLENGTH;
	if (size > es->BufLimit)
		size = es->BufLimit;

	dprintf_edit(stddeb, "edit: EDIT_MakeFit: trying to ReAlloc to %d+1\n", size);

	if (LOCAL_ReAlloc(wndPtr->hInstance, es->hBuf, size + 1, LMEM_MOVEABLE)) {
		es->BufSize = MIN(LOCAL_Size(wndPtr->hInstance, es->hBuf) - 1, es->BufLimit);
		return TRUE;
	} else
		return FALSE;
}


/*********************************************************************
 *
 *	EDIT_MoveBackward
 *
 */
static void EDIT_MoveBackward(WND *wndPtr, BOOL extend)
{
	UINT s = LOWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	UINT e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	UINT l = (UINT)EDIT_EM_LineFromChar(wndPtr, e, 0L);
	UINT li = (UINT)EDIT_EM_LineIndex(wndPtr, l, 0L);

	if (e - li == 0) {
		if (l) {
			li = (UINT)EDIT_EM_LineIndex(wndPtr, l - 1, 0L);
			e = li + (UINT)EDIT_EM_LineLength(wndPtr, li, 0L);
		}
	} else
		e--;
	if (!extend)
		s = e;
	EDIT_EM_SetSel(wndPtr, 0, MAKELPARAM(s, e));
}


/*********************************************************************
 *
 *	EDIT_MoveDownward
 *
 */
static void EDIT_MoveDownward(WND *wndPtr, BOOL extend)
{
	UINT s = LOWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	UINT e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	UINT l = (UINT)EDIT_EM_LineFromChar(wndPtr, e, 0L);
	UINT lc = (UINT)EDIT_EM_GetLineCount(wndPtr, e, 0L);
	UINT li = (UINT)EDIT_EM_LineIndex(wndPtr, l, 0L);
	INT x;

	if (l < lc - 1) {
		x = EDIT_WndXFromCol(wndPtr, l, e - li);
		l++;
		e = (UINT)EDIT_EM_LineIndex(wndPtr, l, 0L) +
				EDIT_ColFromWndX(wndPtr, l, x);
	}
	if (!extend)
		s = e;
	EDIT_EM_SetSel(wndPtr, 0, MAKELPARAM(s, e));
}


/*********************************************************************
 *
 *	EDIT_MoveEnd
 *
 */
static void EDIT_MoveEnd(WND *wndPtr, BOOL extend)
{
	UINT s = LOWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	UINT e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	UINT l = (UINT)EDIT_EM_LineFromChar(wndPtr, e, 0L);
	UINT ll = (UINT)EDIT_EM_LineLength(wndPtr, e, 0L);
	UINT li = (UINT)EDIT_EM_LineIndex(wndPtr, l, 0L);

	e = li + ll;
	if (!extend)
		s = e;
	EDIT_EM_SetSel(wndPtr, 0, MAKELPARAM(s, e));
}


/*********************************************************************
 *
 *	EDIT_MoveForward
 *
 */
static void EDIT_MoveForward(WND *wndPtr, BOOL extend)
{
	UINT s = LOWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	UINT e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	UINT l = (UINT)EDIT_EM_LineFromChar(wndPtr, e, 0L);
	UINT lc = (UINT)EDIT_EM_GetLineCount(wndPtr, e, 0L);
	UINT ll = (UINT)EDIT_EM_LineLength(wndPtr, e, 0L);
	UINT li = (UINT)EDIT_EM_LineIndex(wndPtr, l, 0L);

	if (e - li == ll) {
		if (l != lc - 1)
			e = (UINT)EDIT_EM_LineIndex(wndPtr, l + 1, 0L);
	} else
		e++;
	if (!extend)
		s = e;
	EDIT_EM_SetSel(wndPtr, 0, MAKELPARAM(s, e));
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
	UINT s = LOWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	UINT e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	UINT l = (UINT)EDIT_EM_LineFromChar(wndPtr, e, 0L);
	UINT li = (UINT)EDIT_EM_LineIndex(wndPtr, l, 0L);

	e = li;
	if (!extend)
		s = e;
	EDIT_EM_SetSel(wndPtr, 0, MAKELPARAM(s, e));
}


/*********************************************************************
 *
 *	EDIT_MovePageDown
 *
 */
static void EDIT_MovePageDown(WND *wndPtr, BOOL extend)
{
	UINT s = LOWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	UINT e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	UINT l = (UINT)EDIT_EM_LineFromChar(wndPtr, e, 0L);
	UINT lc = (UINT)EDIT_EM_GetLineCount(wndPtr, e, 0L);
	UINT li = (UINT)EDIT_EM_LineIndex(wndPtr, l, 0L);
	INT x;

	if (l < lc - 1) {
		x = EDIT_WndXFromCol(wndPtr, l, e - li);
		l = MIN(lc - 1, l + EDIT_GetVisibleLineCount(wndPtr));
		e = (UINT)EDIT_EM_LineIndex(wndPtr, l, 0L) +
				EDIT_ColFromWndX(wndPtr, l, x);
	}
	if (!extend)
		s = e;
	EDIT_EM_SetSel(wndPtr, 0, MAKELPARAM(s, e));
}


/*********************************************************************
 *
 *	EDIT_MovePageUp
 *
 */
static void EDIT_MovePageUp(WND *wndPtr, BOOL extend)
{
	UINT s = LOWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	UINT e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	UINT l = (UINT)EDIT_EM_LineFromChar(wndPtr, e, 0L);
	UINT li = (UINT)EDIT_EM_LineIndex(wndPtr, l, 0L);
	INT x;

	if (l) {
		x = EDIT_WndXFromCol(wndPtr, l, e - li);
		l = MAX(0, l - EDIT_GetVisibleLineCount(wndPtr));
		e = (UINT)EDIT_EM_LineIndex(wndPtr, l, 0L) +
				EDIT_ColFromWndX(wndPtr, l, x);
	}
	if (!extend)
		s = e;
	EDIT_EM_SetSel(wndPtr, 0, MAKELPARAM(s, e));
}


/*********************************************************************
 *
 *	EDIT_MoveUpward
 *
 */
static void EDIT_MoveUpward(WND *wndPtr, BOOL extend)
{
	UINT s = LOWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	UINT e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	UINT l = (UINT)EDIT_EM_LineFromChar(wndPtr, e, 0L);
	UINT li = (UINT)EDIT_EM_LineIndex(wndPtr, l, 0L);
	INT x;

	if (l) {
		x = EDIT_WndXFromCol(wndPtr, l, e - li);
		l--;
		e = (UINT)EDIT_EM_LineIndex(wndPtr, l, 0L) +
				EDIT_ColFromWndX(wndPtr, l, x);
	}
	if (!extend)
		s = e;
	EDIT_EM_SetSel(wndPtr, 0, MAKELPARAM(s, e));
}


/*********************************************************************
 *
 *	EDIT_MoveWordBackward
 *
 */
static void EDIT_MoveWordBackward(WND *wndPtr, BOOL extend)
{
	UINT s = LOWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	UINT e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	UINT l = (UINT)EDIT_EM_LineFromChar(wndPtr, e, 0L);
	UINT ll = (UINT)EDIT_EM_LineLength(wndPtr, e, 0L);
	UINT li = (UINT)EDIT_EM_LineIndex(wndPtr, l, 0L);
	char *text;

	if (e - li == 0) {
		if (l) {
			li = (UINT)EDIT_EM_LineIndex(wndPtr, l - 1, 0L);
			e = li + (UINT)EDIT_EM_LineLength(wndPtr, li, 0L);
		}
	} else {
		text = EDIT_GetPointer(wndPtr);
		e = li + (UINT)EDIT_CallWordBreakProc(wndPtr,
				text + li, e - li, ll, WB_LEFT);
	}
	if (!extend)
		s = e;
	EDIT_EM_SetSel(wndPtr, 0, MAKELPARAM(s, e));
}


/*********************************************************************
 *
 *	EDIT_MoveWordForward
 *
 */
static void EDIT_MoveWordForward(WND *wndPtr, BOOL extend)
{
	UINT s = LOWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	UINT e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	UINT l = (UINT)EDIT_EM_LineFromChar(wndPtr, e, 0L);
	UINT lc = (UINT)EDIT_EM_GetLineCount(wndPtr, e, 0L);
	UINT ll = (UINT)EDIT_EM_LineLength(wndPtr, e, 0L);
	UINT li = (UINT)EDIT_EM_LineIndex(wndPtr, l, 0L);
	char *text;

	if (e - li == ll) {
		if (l != lc - 1)
			e = (UINT)EDIT_EM_LineIndex(wndPtr, l + 1, 0L);
	} else {
		text = EDIT_GetPointer(wndPtr);
		e = li + (UINT)EDIT_CallWordBreakProc(wndPtr,
				text + li, e - li + 1, ll, WB_RIGHT);
	}
	if (!extend)
		s = e;
	EDIT_EM_SetSel(wndPtr, 0, MAKELPARAM(s, e));
}


/*********************************************************************
 *
 *	EDIT_PaintLine
 *
 */
static void EDIT_PaintLine(WND *wndPtr, HDC32 hdc, UINT line, BOOL rev)
{
	UINT fv = (UINT)EDIT_EM_GetFirstVisibleLine(wndPtr, 0, 0L);
	UINT vlc = EDIT_GetVisibleLineCount(wndPtr);
	UINT lc = (UINT)EDIT_EM_GetLineCount(wndPtr, 0, 0L);
	UINT li;
	UINT ll;
	UINT s;
	UINT e;
	INT x;
	INT y;

	if ((line < fv) || (line > fv + vlc) || (line >= lc))
		return;

	dprintf_edit(stddeb, "edit: EDIT_PaintLine: line=%d\n", line);

	x = EDIT_WndXFromCol(wndPtr, line, 0);
	y = EDIT_WndYFromLine(wndPtr, line);
	li = (UINT)EDIT_EM_LineIndex(wndPtr, line, 0L);
	ll = (UINT)EDIT_EM_LineLength(wndPtr, li, 0L);
	s = LOWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	ORDER_UINT(s, e);
	s = MIN(li + ll, MAX(li, s));
	e = MIN(li + ll, MAX(li, e));
	if (rev && (s != e) &&
			((GetFocus32() == wndPtr->hwndSelf) ||
				(wndPtr->dwStyle & ES_NOHIDESEL))) {
		x += EDIT_PaintText(wndPtr, hdc, x, y, line, 0, s - li, FALSE);
		x += EDIT_PaintText(wndPtr, hdc, x, y, line, s - li, e - s, TRUE);
		x += EDIT_PaintText(wndPtr, hdc, x, y, line, e - li, li + ll - e, FALSE);
	} else
		x += EDIT_PaintText(wndPtr, hdc, x, y, line, 0, ll, FALSE);
}


/*********************************************************************
 *
 *	EDIT_PaintText
 *
 */
static UINT EDIT_PaintText(WND *wndPtr, HDC32 hdc, INT x, INT y, UINT line, UINT col, UINT count, BOOL rev)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	COLORREF BkColor;
	COLORREF TextColor;
	UINT ret;
	char *text;
	UINT li;
	UINT xoff;

	if (!count)
		return 0;
	BkColor = GetBkColor(hdc);
	TextColor = GetTextColor(hdc);
	if (rev) {
		SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
		SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
	}
	text = EDIT_GetPasswordPointer(wndPtr);
	li = (UINT)EDIT_EM_LineIndex(wndPtr, line, 0L);
	xoff = EDIT_GetXOffset(wndPtr);
	ret = LOWORD(TabbedTextOut(hdc, x, y, text + li + col, count,
					es->NumTabStops, es->TabStops, -xoff));
	free(text);
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
static LRESULT EDIT_ReplaceSel(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	const char *str = (char *)lParam;
	int strl = strlen(str);
	UINT tl = (UINT)EDIT_WM_GetTextLength(wndPtr, 0, 0L);
	UINT s = LOWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	UINT e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	int i;
	char *p;
	char *text;
	BOOL redraw;

	ORDER_UINT(s,e);
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
	if(IsUpper(wndPtr))
		AnsiUpperBuff(p, strl);
	else if(IsLower(wndPtr))
		AnsiLowerBuff(p, strl);
	EDIT_BuildLineDefs(wndPtr);
	e += strl;
	EDIT_EM_SetSel(wndPtr, 0, MAKELPARAM(e, e));
	EDIT_EM_SetModify(wndPtr, TRUE, 0L);
	EDIT_NOTIFY_PARENT(wndPtr, EN_UPDATE);
	EDIT_WM_SetRedraw(wndPtr, redraw, 0L);
	if (redraw) {
		InvalidateRect32( wndPtr->hwndSelf, NULL, TRUE );
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
	UINT e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	UINT l = (UINT)EDIT_EM_LineFromChar(wndPtr, e, 0L);
	UINT li = (UINT)EDIT_EM_LineIndex(wndPtr, l, 0L);
	UINT fv = (UINT)EDIT_EM_GetFirstVisibleLine(wndPtr, 0, 0L);
	UINT vlc = EDIT_GetVisibleLineCount(wndPtr);
	UINT ww = EDIT_GetWndWidth(wndPtr);
	UINT cw = EDIT_GetAveCharWidth(wndPtr);
	INT x = EDIT_WndXFromCol(wndPtr, l, e - li);
	int dy = 0;
	int dx = 0;

	if (l >= fv + vlc)
		dy = l - vlc + 1 - fv;
	if (l < fv)
		dy = l - fv;
	if (x < 0)
		dx = x - ww / HSCROLL_FRACTION / cw * cw;
	if (x > ww)
		dx = x - (HSCROLL_FRACTION - 1) * ww / HSCROLL_FRACTION / cw * cw;
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
static INT EDIT_WndXFromCol(WND *wndPtr, UINT line, UINT col)
{	
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	char *text = EDIT_GetPasswordPointer(wndPtr);
	INT ret;
	HDC32 hdc;
	HFONT16 hFont;
	HFONT16 oldFont = 0;
	UINT lc = (UINT)EDIT_EM_GetLineCount(wndPtr, 0, 0L);
	UINT li = (UINT)EDIT_EM_LineIndex(wndPtr, line, 0L);
	UINT ll = (UINT)EDIT_EM_LineLength(wndPtr, li, 0L);
	UINT xoff = EDIT_GetXOffset(wndPtr);

	hdc = GetDC32(wndPtr->hwndSelf);
	hFont = (HFONT16)EDIT_WM_GetFont(wndPtr, 0, 0L);
	if (hFont)
		oldFont = SelectObject(hdc, hFont);
	line = MAX(0, MIN(line, lc - 1));
	col = MIN(col, ll);
	ret = LOWORD(GetTabbedTextExtent(hdc,
			text + li, col,
			es->NumTabStops, es->TabStops)) - xoff;
	if (hFont)
		SelectObject(hdc, oldFont);
	ReleaseDC32(wndPtr->hwndSelf, hdc);
	free(text);
	return ret;
}


/*********************************************************************
 *
 *	EDIT_WndYFromLine
 *
 *	Calculates, for a given line, the Y-coordinate on the screen.
 *
 */
static INT EDIT_WndYFromLine(WND *wndPtr, UINT line)
{
	UINT fv = (UINT)EDIT_EM_GetFirstVisibleLine(wndPtr, 0, 0L);
	UINT lh = EDIT_GetLineHeight(wndPtr);

	return (line - fv) * lh;
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
static INT EDIT_WordBreakProc(char *s, INT index, INT count, INT action)
{
	INT ret = 0;

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
static LRESULT EDIT_EM_CanUndo(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	return 0L;
}


/*********************************************************************
 *
 *	EM_EMPTYUNDOBUFFER
 *
 */
static LRESULT EDIT_EM_EmptyUndoBuffer(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	return 0L;
}


/*********************************************************************
 *
 *	EM_FMTLINES
 *
 */
static LRESULT EDIT_EM_FmtLines(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	fprintf(stdnimp, "edit: EM_FMTLINES: message not implemented.\n");
	return wParam ? -1L : 0L;
}


/*********************************************************************
 *
 *	EM_GETFIRSTVISIBLELINE
 *
 */
static LRESULT EDIT_EM_GetFirstVisibleLine(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	
	return (LRESULT)es->FirstVisibleLine;
}


/*********************************************************************
 *
 *	EM_GETHANDLE
 *
 */
static LRESULT EDIT_EM_GetHandle(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	return (LRESULT)es->hBuf;
}
 

/*********************************************************************
 *
 *	EM_GETLINE
 *
 */
static LRESULT EDIT_EM_GetLine(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	char *text;
	char *src;
	char *dst;
	UINT len;
	UINT i;
	UINT lc = (UINT)EDIT_EM_GetLineCount(wndPtr, 0, 0L);

	if (!IsMultiLine(wndPtr))
		wParam = 0;
	if ((UINT)wParam >= lc)
		return 0L;
	text = EDIT_GetPointer(wndPtr);
	src = text + (UINT)EDIT_EM_LineIndex(wndPtr, wParam, 0L);
	dst = (char *)PTR_SEG_TO_LIN(lParam);
	len = MIN(*(WORD *)dst, (UINT)EDIT_EM_LineLength(wndPtr, wParam, 0L));
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
static LRESULT EDIT_EM_GetLineCount(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	
	return (LRESULT)es->LineCount;
}


/*********************************************************************
 *
 *	EM_GETMODIFY
 *
 */
static LRESULT EDIT_EM_GetModify(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	return (LRESULT)es->TextChanged;
}


/*********************************************************************
 *
 *	EM_GETPASSWORDCHAR
 *
 */
static LRESULT EDIT_EM_GetPasswordChar(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	return (LRESULT)es->PasswordChar;
}


/*********************************************************************
 *
 *	EM_GETRECT
 *
 */
static LRESULT EDIT_EM_GetRect(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	return EDIT_GetRect(wndPtr, wParam, (LPARAM)PTR_SEG_TO_LIN(lParam));
}


/*********************************************************************
 *
 *	EM_GETSEL
 *
 */
static LRESULT EDIT_EM_GetSel(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	return MAKELONG(es->SelStart, es->SelEnd);
}


/*********************************************************************
 *
 *	EM_GETTHUMB
 *
 *	FIXME: undocumented: is this right ?
 *
 */
static LRESULT EDIT_EM_GetThumb(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	return MAKELONG(EDIT_WM_VScroll(wndPtr, EM_GETTHUMB, 0L),
		EDIT_WM_HScroll(wndPtr, EM_GETTHUMB, 0L));
}


/*********************************************************************
 *
 *	EM_GETWORDBREAKPROC
 *
 */
static LRESULT EDIT_EM_GetWordBreakProc(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	return (LRESULT)es->WordBreakProc;
}


/*********************************************************************
 *
 *	EM_LIMITTEXT
 *
 */
static LRESULT EDIT_EM_LimitText(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
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
static LRESULT EDIT_EM_LineFromChar(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	UINT l;

	if (!IsMultiLine(wndPtr))
		return 0L;
	if ((INT)wParam == -1)
		wParam = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	l = (UINT)EDIT_EM_GetLineCount(wndPtr, 0, 0L) - 1;
	while ((UINT)EDIT_EM_LineIndex(wndPtr, l, 0L) > (UINT)wParam)
		l--;
	return (LRESULT)l;
}


/*********************************************************************
 *
 *	EM_LINEINDEX
 *
 */
static LRESULT EDIT_EM_LineIndex(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	UINT e;
	UINT l;
	UINT lc = (UINT)EDIT_EM_GetLineCount(wndPtr, 0, 0L);

	if ((INT)wParam == -1) {
		e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
		l = lc - 1;
		while (es->LineDefs[l].offset > e)
			l--;
		return (LRESULT)es->LineDefs[l].offset;
	}
	if ((UINT)wParam >= lc)
		return -1L;
	return (LRESULT)es->LineDefs[(UINT)wParam].offset;
}


/*********************************************************************
 *
 *	EM_LINELENGTH
 *
 */
static LRESULT EDIT_EM_LineLength(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	UINT s;
	UINT e;
	UINT sl;
	UINT el;

	if (!IsMultiLine(wndPtr))
		return (LRESULT)es->LineDefs[0].length;
	if ((INT)wParam == -1) {
		s = LOWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
		e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
		sl = (UINT)EDIT_EM_LineFromChar(wndPtr, s, 0L);
		el = (UINT)EDIT_EM_LineFromChar(wndPtr, e, 0L);
		return (LRESULT)(s - es->LineDefs[sl].offset +
				es->LineDefs[el].offset +
				es->LineDefs[el].length - e);
	}
	return (LRESULT)es->LineDefs[(UINT)EDIT_EM_LineFromChar(wndPtr, wParam, 0L)].length;
}
 

/*********************************************************************
 *
 *	EM_LINESCROLL
 *
 */
static LRESULT EDIT_EM_LineScroll(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	UINT lc = (UINT)EDIT_EM_GetLineCount(wndPtr, 0, 0L);
	UINT fv = (UINT)EDIT_EM_GetFirstVisibleLine(wndPtr, 0, 0L);
	UINT nfv = MAX(0, fv + (INT)LOWORD(lParam));
	UINT xoff = EDIT_GetXOffset(wndPtr);
	UINT nxoff = MAX(0, xoff + (INT)HIWORD(lParam));
	UINT tw = EDIT_GetTextWidth(wndPtr);
	INT dx;
	INT dy;
	POINT16 pos;

	if (nfv >= lc)
		nfv = lc - 1;

	if (nxoff >= tw)
		nxoff = tw;
	dx = xoff - nxoff;
	dy = EDIT_WndYFromLine(wndPtr, fv) - EDIT_WndYFromLine(wndPtr, nfv);
	if (dx || dy) {
		if (wndPtr->hwndSelf == GetFocus32())
			HideCaret(wndPtr->hwndSelf);
		if (EDIT_GetRedraw(wndPtr)) 
			ScrollWindow(wndPtr->hwndSelf, dx, dy, NULL, NULL);
		es->FirstVisibleLine = nfv;
		es->XOffset = nxoff;
		if (IsVScrollBar(wndPtr))
			SetScrollPos32(wndPtr->hwndSelf, SB_VERT,
				EDIT_WM_VScroll(wndPtr, EM_GETTHUMB, 0L), TRUE);
		if (IsHScrollBar(wndPtr))
			SetScrollPos32(wndPtr->hwndSelf, SB_HORZ,
				EDIT_WM_HScroll(wndPtr, EM_GETTHUMB, 0L), TRUE);
		if (wndPtr->hwndSelf == GetFocus32()) {
			GetCaretPos16(&pos);
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
static LRESULT EDIT_EM_ReplaceSel(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	return (LRESULT)EDIT_ReplaceSel(wndPtr, wParam,
				(LPARAM)(char *)PTR_SEG_TO_LIN(lParam));
}
 

/*********************************************************************
 *
 *	EM_SCROLL
 *
 *	FIXME: undocumented message.
 *
 */
static LRESULT EDIT_EM_Scroll(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	fprintf(stdnimp, "edit: EM_SCROLL: message not implemented (undocumented), please report.\n");
	return 0L;
}


/*********************************************************************
 *
 *	EM_SETHANDLE
 *
 */
static LRESULT EDIT_EM_SetHandle(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	if (IsMultiLine(wndPtr)) {
		EDIT_ReleasePointer(wndPtr);
		/*
		 *	old buffer is freed by caller
		 */
		es->hBuf = (HLOCAL16)wParam;
		es->BufSize = LOCAL_Size(wndPtr->hInstance, es->hBuf) - 1;
		es->LineCount = 0;
		es->FirstVisibleLine = 0;
		es->SelStart = es->SelEnd = 0;
		EDIT_EM_EmptyUndoBuffer(wndPtr, 0, 0L);
		EDIT_EM_SetModify(wndPtr, FALSE, 0L);
		EDIT_BuildLineDefs(wndPtr);
		if (EDIT_GetRedraw(wndPtr))
			InvalidateRect32( wndPtr->hwndSelf, NULL, TRUE );
		EDIT_ScrollIntoView(wndPtr);
	}
	return 0L;
}


/*********************************************************************
 *
 *	EM_SETMODIFY
 *
 */
static LRESULT EDIT_EM_SetModify(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	es->TextChanged = (BOOL)wParam;
	return 0L;
}


/*********************************************************************
 *
 *	EM_SETPASSWORDCHAR
 *
 */
static LRESULT EDIT_EM_SetPasswordChar(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
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
static LRESULT EDIT_EM_SetReadOnly(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
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
static LRESULT EDIT_EM_SetRect(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	fprintf(stdnimp,"edit: EM_SETRECT: message not implemented, please report.\n");
	return 0L;
}


/*********************************************************************
 *
 *	EM_SETRECTNP
 *
 */
static LRESULT EDIT_EM_SetRectNP(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	fprintf(stdnimp,"edit: EM_SETRECTNP: message not implemented, please report.\n");
	return 0L;
}


/*********************************************************************
 *
 *	EM_SETSEL
 *
 */
static LRESULT EDIT_EM_SetSel(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	UINT ns = LOWORD(lParam);
	UINT ne = HIWORD(lParam);
	UINT s = LOWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	UINT e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	UINT el;
	UINT eli;
	UINT tl = (UINT)EDIT_WM_GetTextLength(wndPtr, 0, 0L);

	if ((INT)ns == -1) {
		ns = e;
		ne = e;
	}
	else {
		ns = MIN(ns, tl);
		ne = MIN(ne, tl);
	}
	es->SelStart = ns;
	es->SelEnd = ne;
	if (wndPtr->hwndSelf == GetFocus32()) {
		el = (UINT)EDIT_EM_LineFromChar(wndPtr, ne, 0L);
		eli = (UINT)EDIT_EM_LineIndex(wndPtr, el, 0L);
		SetCaretPos(EDIT_WndXFromCol(wndPtr, el, ne - eli),
				EDIT_WndYFromLine(wndPtr, el));
	}
	if (!wParam)
		EDIT_ScrollIntoView(wndPtr);
	if (EDIT_GetRedraw(wndPtr)) {
		ORDER_UINT(s, e);
		ORDER_UINT(s, ns);
		ORDER_UINT(s, ne);
		ORDER_UINT(e, ns);
		ORDER_UINT(e, ne);
		ORDER_UINT(ns, ne);
		if (e != ns) {
			EDIT_InvalidateText(wndPtr, s, e);
			EDIT_InvalidateText(wndPtr, ns, ne);
		} else
			EDIT_InvalidateText(wndPtr, s, ne);
	}
	return -1L;
}


/*********************************************************************
 *
 *	EM_SETTABSTOPS
 *
 */
static LRESULT EDIT_EM_SetTabStops(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	if (!IsMultiLine(wndPtr))
		return 0L;
	if (es->TabStops)
		free(es->TabStops);
	es->NumTabStops = (UINT)wParam;
	if (!wParam)
		es->TabStops = NULL;
	else {
		es->TabStops = (LPINT16)xmalloc(wParam * sizeof(INT16));
		memcpy(es->TabStops, (LPINT16)PTR_SEG_TO_LIN(lParam),
				(UINT)wParam * sizeof(INT16));
	}
	return 1L;
}


/*********************************************************************
 *
 *	EM_SETWORDBREAKPROC
 *
 */
static LRESULT EDIT_EM_SetWordBreakProc(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
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
static LRESULT EDIT_EM_Undo(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	return 0L;
}


/*********************************************************************
 *
 *	WM_CHAR
 *
 */
static LRESULT EDIT_WM_Char(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	char str[2];
	unsigned char c = (unsigned char)wParam;

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
static LRESULT EDIT_WM_Clear(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	UINT s = LOWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	UINT e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	char *text;
	BOOL redraw;
	
	if (s != e) {
		redraw = EDIT_GetRedraw(wndPtr);
		EDIT_WM_SetRedraw(wndPtr, FALSE, 0L);
		ORDER_UINT(s, e);
		text = EDIT_GetPointer(wndPtr);
		strcpy(text + s, text + e);
		EDIT_BuildLineDefs(wndPtr);
		EDIT_EM_SetSel(wndPtr, 0, MAKELPARAM(s, s));
		EDIT_EM_SetModify(wndPtr, TRUE, 0L);
		EDIT_NOTIFY_PARENT(wndPtr, EN_UPDATE);
		EDIT_WM_SetRedraw(wndPtr, redraw, 0L);
		if (redraw) {
			InvalidateRect32( wndPtr->hwndSelf, NULL, TRUE );
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
static LRESULT EDIT_WM_Copy(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	UINT s = LOWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	UINT e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	HGLOBAL16 hdst;
	char *text;
	char *dst;
	char *src;
	int i;

	if (e == s)
		return -1L;
	ORDER_UINT(s, e);
	hdst = GlobalAlloc16(GMEM_MOVEABLE, (DWORD)(e - s + 1));
	dst = GlobalLock16(hdst);
	text = EDIT_GetPointer(wndPtr);
	src = text + s;
	for (i = 0 ; i < e - s ; i++)
		*dst++ = *src++;
	*dst = '\0';
	GlobalUnlock16(hdst);
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
static LRESULT EDIT_WM_Create(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	CREATESTRUCT16 *cs = (CREATESTRUCT16 *)PTR_SEG_TO_LIN(lParam);
	EDITSTATE *es;
	char *text;

	es = xmalloc(sizeof(EDITSTATE));
	memset(es, 0, sizeof(EDITSTATE));
	*(EDITSTATE **)wndPtr->wExtra = es;

	if (cs->style & WS_VSCROLL)
		cs->style |= ES_AUTOVSCROLL;
	if (cs->style & WS_HSCROLL)
		cs->style |= ES_AUTOHSCROLL;

	/* remove the WS_CAPTION style if it has been set - this is really a  */
	/* pseudo option made from a combination of WS_BORDER and WS_DLGFRAME */
	if ((cs->style & WS_BORDER) && (cs->style & WS_DLGFRAME))
		cs->style ^= WS_DLGFRAME;

	if (IsMultiLine(wndPtr)) {
		es->BufSize = BUFSTART_MULTI;
		es->BufLimit = BUFLIMIT_MULTI;
		es->PasswordChar = '\0';
	} else {
		es->BufSize = BUFSTART_SINGLE;
		es->BufLimit = BUFLIMIT_SINGLE;
		es->PasswordChar = (cs->style & ES_PASSWORD) ? '*' : '\0';
	}
	if (!LOCAL_HeapSize(wndPtr->hInstance)) {
		if (!LocalInit(wndPtr->hInstance, 0,
                               GlobalSize16(wndPtr->hInstance))) {
			fprintf(stderr, "edit: WM_CREATE: could not initialize local heap\n");
			return -1L;
		}
		dprintf_edit(stddeb, "edit: WM_CREATE: local heap initialized\n");
	}
	if (!(es->hBuf = LOCAL_Alloc(wndPtr->hInstance, LMEM_MOVEABLE, es->BufSize + 1))) {
		fprintf(stderr, "edit: WM_CREATE: unable to allocate buffer\n");
		return -1L;
	}
	es->BufSize = LOCAL_Size(wndPtr->hInstance, es->hBuf) - 1;
	text = EDIT_GetPointer(wndPtr);
	*text = '\0';
	EDIT_BuildLineDefs(wndPtr);
	EDIT_WM_SetFont(wndPtr, 0, 0L);
	if (cs->lpszName && *(char *)PTR_SEG_TO_LIN(cs->lpszName) != '\0')
		EDIT_EM_ReplaceSel(wndPtr, FALSE, (LPARAM)cs->lpszName);
	EDIT_WM_SetRedraw(wndPtr, TRUE, 0L);
	return 0L;
}


/*********************************************************************
 *
 *	WM_CUT
 *
 */
static LRESULT EDIT_WM_Cut(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
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
static LRESULT EDIT_WM_Destroy(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
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
static LRESULT EDIT_WM_Enable(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDIT_InvalidateText(wndPtr, 0, -1);
	return 0L;
}


/*********************************************************************
 *
 *	WM_ERASEBKGND
 *
 */
static LRESULT EDIT_WM_EraseBkGnd(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	HBRUSH16 hBrush;
	RECT16 rc;

	hBrush = (HBRUSH16)EDIT_SEND_CTLCOLOR(wndPtr, wParam);
	if (!hBrush)
		hBrush = (HBRUSH16)GetStockObject(WHITE_BRUSH);

	GetClientRect16(wndPtr->hwndSelf, &rc);
	IntersectClipRect((HDC16)wParam, rc.left, rc.top, rc.right, rc.bottom);
	GetClipBox16((HDC16)wParam, &rc);
	/*
	 *	FIXME:	specs say that we should UnrealizeObject() the brush,
	 *		but the specs of UnrealizeObject() say that we shouldn't
	 *		unrealize a stock object.  The default brush that
	 *		DefWndProc() returns is ... a stock object.
	 */
	FillRect16((HDC16)wParam, &rc, hBrush);
	return -1L;
}


/*********************************************************************
 *
 *	WM_GETDLGCODE
 *
 */
static LRESULT EDIT_WM_GetDlgCode(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	return DLGC_HASSETSEL | DLGC_WANTCHARS | DLGC_WANTARROWS;
}


/*********************************************************************
 *
 *	WM_GETFONT
 *
 */
static LRESULT EDIT_WM_GetFont(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	return (LRESULT)es->hFont;
}


/*********************************************************************
 *
 *	WM_GETTEXT
 *
 */
static LRESULT EDIT_WM_GetText(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	char *text = EDIT_GetPointer(wndPtr);
	int len;
	LRESULT lResult = 0L;

	len = strlen(text);
	if ((UINT)wParam > len) {
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
static LRESULT EDIT_WM_GetTextLength(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	char *text = EDIT_GetPointer(wndPtr);

	return (LRESULT)strlen(text);
}
 

/*********************************************************************
 *
 *	WM_HSCROLL
 *
 *	FIXME: scrollbar code itself is broken, so this one is a hack.
 *
 */
static LRESULT EDIT_WM_HScroll(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	UINT ww = EDIT_GetWndWidth(wndPtr);
	UINT tw = EDIT_GetTextWidth(wndPtr);
	UINT cw = EDIT_GetAveCharWidth(wndPtr);
	UINT xoff = EDIT_GetXOffset(wndPtr);
	INT dx = 0;
	BOOL not = TRUE;
	LRESULT ret = 0L;

	switch (wParam) {
	case SB_LINELEFT:
		dx = -cw;
		break;
	case SB_LINERIGHT:
		dx = cw;
		break;
	case SB_PAGELEFT:
		dx = -ww / HSCROLL_FRACTION / cw * cw;
		break;
	case SB_PAGERIGHT:
		dx = ww / HSCROLL_FRACTION / cw * cw;
		break;
	case SB_LEFT:
		dx = -xoff;
		break;
	case SB_RIGHT:
		dx = tw - xoff;
		break;
	case SB_THUMBTRACK:
/*
 *		not = FALSE;
 */
	case SB_THUMBPOSITION:
		dx = LOWORD(lParam) * tw / 100 - xoff;
		break;
	/* The next two are undocumented ! */
	case EM_GETTHUMB:
		ret = tw ? MAKELONG(xoff * 100 / tw, 0) : 0;
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
static LRESULT EDIT_WM_KeyDown(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	UINT s = LOWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	UINT e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
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
static LRESULT EDIT_WM_KillFocus(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	UINT s;
	UINT e;

	DestroyCaret();
	if(!(wndPtr->dwStyle & ES_NOHIDESEL)) {
		s = LOWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
		e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
		EDIT_InvalidateText(wndPtr, s, e);
	}
	EDIT_NOTIFY_PARENT(wndPtr, EN_KILLFOCUS);
	return 0L;
}


/*********************************************************************
 *
 *	WM_LBUTTONDBLCLK
 *
 *	The caret position has been set on the WM_LBUTTONDOWN message
 *
 */
static LRESULT EDIT_WM_LButtonDblClk(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	UINT s;
	UINT e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	UINT l = (UINT)EDIT_EM_LineFromChar(wndPtr, e, 0L);
	UINT li = (UINT)EDIT_EM_LineIndex(wndPtr, l, 0L);
	UINT ll = (UINT)EDIT_EM_LineLength(wndPtr, e, 0L);
	char *text = EDIT_GetPointer(wndPtr);

	s = li + EDIT_CallWordBreakProc (wndPtr, text + li, e - li, ll, WB_LEFT);
	e = li + EDIT_CallWordBreakProc(wndPtr, text + li, e - li, ll, WB_RIGHT);
	EDIT_EM_SetSel(wndPtr, 0, MAKELPARAM(s, e));
	return 0L;
}


/*********************************************************************
 *
 *	WM_LBUTTONDOWN
 *
 */
static LRESULT EDIT_WM_LButtonDown(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	INT x = (INT)LOWORD(lParam);
	INT y = (INT)HIWORD(lParam);
	UINT l = EDIT_LineFromWndY(wndPtr, y);
	UINT c;
	UINT s;
	UINT e;
	UINT fv = (UINT)EDIT_EM_GetFirstVisibleLine(wndPtr, 0, 0L);
	UINT vlc = EDIT_GetVisibleLineCount(wndPtr);
	UINT li;

	SetFocus32(wndPtr->hwndSelf);
	SetCapture32(wndPtr->hwndSelf);
	l = MIN(fv + vlc - 1, MAX(fv, l));
	x = MIN(EDIT_GetWndWidth(wndPtr), MAX(0, x));
	c = EDIT_ColFromWndX(wndPtr, l, x);
	li = (UINT)EDIT_EM_LineIndex(wndPtr, l, 0L);
	e = li + c;
	if (GetKeyState(VK_SHIFT) & 0x8000)
		s = LOWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	else 
		s = e;
	EDIT_EM_SetSel(wndPtr, 0, MAKELPARAM(s, e));
	return 0L;
}


/*********************************************************************
 *
 *	WM_LBUTTONUP
 *
 */
static LRESULT EDIT_WM_LButtonUp(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	if (GetCapture32() == wndPtr->hwndSelf)
		ReleaseCapture();
	return 0L;
}


/*********************************************************************
 *
 *	WM_MOUSEMOVE
 *
 */
static LRESULT EDIT_WM_MouseMove(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	INT x;
	INT y;
	UINT l;
	UINT c;
	UINT s;
	UINT fv;
	UINT vlc;
	UINT li;

	if (GetCapture32() == wndPtr->hwndSelf) {
		x = (INT)LOWORD(lParam);
		y = (INT)HIWORD(lParam);
		fv = (UINT)EDIT_EM_GetFirstVisibleLine(wndPtr, 0, 0L);
		vlc = EDIT_GetVisibleLineCount(wndPtr);
		l = EDIT_LineFromWndY(wndPtr, y);
		l = MIN(fv + vlc - 1, MAX(fv, l));
		x = MIN(EDIT_GetWndWidth(wndPtr), MAX(0, x));
		c = EDIT_ColFromWndX(wndPtr, l, x);
		s = LOWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
		li = (UINT)EDIT_EM_LineIndex(wndPtr, l, 0L);
		EDIT_EM_SetSel(wndPtr, 1, MAKELPARAM(s, li + c));
	}
	return 0L;
}


/*********************************************************************
 *
 *	WM_PAINT
 *
 */
static LRESULT EDIT_WM_Paint(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	PAINTSTRUCT16 ps;
	UINT i;
	UINT fv = (UINT)EDIT_EM_GetFirstVisibleLine(wndPtr, 0, 0L);
	UINT vlc = EDIT_GetVisibleLineCount(wndPtr);
	UINT lc = (UINT)EDIT_EM_GetLineCount(wndPtr, 0, 0L);
	HDC16 hdc;
	HFONT16 hFont;
	HFONT16 oldFont = 0;
	RECT16 rc;
	RECT16 rcLine;
	RECT16 rcRgn;
	BOOL rev = IsWindowEnabled(wndPtr->hwndSelf) &&
				((GetFocus32() == wndPtr->hwndSelf) ||
					(wndPtr->dwStyle & ES_NOHIDESEL));

	hdc = BeginPaint16(wndPtr->hwndSelf, &ps);
	GetClientRect16(wndPtr->hwndSelf, &rc);
	IntersectClipRect(hdc, rc.left, rc.top, rc.right, rc.bottom);
	hFont = EDIT_WM_GetFont(wndPtr, 0, 0L);
	if (hFont)
		oldFont = SelectObject(hdc, hFont);
	EDIT_SEND_CTLCOLOR(wndPtr, hdc);
	if (!IsWindowEnabled(wndPtr->hwndSelf))
		SetTextColor(hdc, GetSysColor(COLOR_GRAYTEXT));
	GetClipBox16(hdc, &rcRgn);
	for (i = fv ; i <= MIN(fv + vlc, fv + lc - 1) ; i++ ) {
		EDIT_GetLineRect(wndPtr, i, 0, -1, &rcLine);
		if (IntersectRect16(&rc, &rcRgn, &rcLine))
			EDIT_PaintLine(wndPtr, hdc, i, rev);
	}
	if (hFont)
		SelectObject(hdc, oldFont);
	EndPaint16(wndPtr->hwndSelf, &ps);
	return 0L;
}


/*********************************************************************
 *
 *	WM_PASTE
 *
 */
static LRESULT EDIT_WM_Paste(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	HGLOBAL16 hsrc;
	char *src;

	OpenClipboard(wndPtr->hwndSelf);
	if ((hsrc = GetClipboardData(CF_TEXT))) {
		src = (char *)GlobalLock16(hsrc);
		EDIT_ReplaceSel(wndPtr, 0, (LPARAM)src);
		GlobalUnlock16(hsrc);
	}
	CloseClipboard();
	return -1L;
}


/*********************************************************************
 *
 *	WM_SETCURSOR
 *
 */
static LRESULT EDIT_WM_SetCursor(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	if (LOWORD(lParam) == HTCLIENT) {
		SetCursor(LoadCursor16(0, IDC_IBEAM));
		return -1L;
	} else
		return 0L;
}


/*********************************************************************
 *
 *	WM_SETFOCUS
 *
 */
static LRESULT EDIT_WM_SetFocus(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	UINT s = LOWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));
	UINT e = HIWORD(EDIT_EM_GetSel(wndPtr, 0, 0L));

	CreateCaret(wndPtr->hwndSelf, 0, 2, EDIT_GetLineHeight(wndPtr));
	EDIT_EM_SetSel(wndPtr, 1, MAKELPARAM(s, e));
	if(!(wndPtr->dwStyle & ES_NOHIDESEL))
		EDIT_InvalidateText(wndPtr, s, e);
	ShowCaret(wndPtr->hwndSelf);
	EDIT_NOTIFY_PARENT(wndPtr, EN_SETFOCUS);
	return 0L;
}


/*********************************************************************
 *
 *	WM_SETFONT
 *
 */
static LRESULT EDIT_WM_SetFont(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	TEXTMETRIC16 tm;
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	LPARAM sel = EDIT_EM_GetSel(wndPtr, 0, 0L);
	HDC32 hdc;
	HFONT16 oldFont = 0;

	es->hFont = (HFONT16)wParam;
	hdc = GetDC32(wndPtr->hwndSelf);
	if (es->hFont)
		oldFont = SelectObject(hdc, es->hFont);
	GetTextMetrics16(hdc, &tm);
	es->LineHeight = HIWORD(GetTextExtent(hdc, "X", 1));
	es->AveCharWidth = tm.tmAveCharWidth;
	if (es->hFont)
		SelectObject(hdc, oldFont);
	ReleaseDC32(wndPtr->hwndSelf, hdc);
	EDIT_BuildLineDefs(wndPtr);
	if ((BOOL)lParam && EDIT_GetRedraw(wndPtr))
		InvalidateRect32( wndPtr->hwndSelf, NULL, TRUE );
	if (wndPtr->hwndSelf == GetFocus32()) {
		DestroyCaret();
		CreateCaret(wndPtr->hwndSelf, 0, 2, EDIT_GetLineHeight(wndPtr));
		EDIT_EM_SetSel(wndPtr, 1, sel);
		ShowCaret(wndPtr->hwndSelf);
	}
	return 0L;
}


/*********************************************************************
 *
 *	WM_SETREDRAW
 *
 */
static LRESULT EDIT_WM_SetRedraw(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	es->Redraw = (BOOL)wParam;
	return 0L;
}


/*********************************************************************
 *
 *	WM_SETTEXT
 *
 */
static LRESULT EDIT_WM_SetText(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDIT_EM_SetSel(wndPtr, 1, MAKELPARAM(0, -1));
	EDIT_WM_Clear(wndPtr, 0, 0L);
	if (lParam)
		EDIT_EM_ReplaceSel(wndPtr, 0, lParam);
	EDIT_EM_EmptyUndoBuffer(wndPtr, 0, 0L);
	EDIT_EM_SetModify(wndPtr, TRUE, 0L);
	EDIT_ScrollIntoView(wndPtr);
	return 0L;
}


/*********************************************************************
 *
 *	WM_SIZE
 *
 */
static LRESULT EDIT_WM_Size(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	if (EDIT_GetRedraw(wndPtr) &&
			((wParam == SIZE_MAXIMIZED) ||
				(wParam == SIZE_RESTORED))) {
		if (IsMultiLine(wndPtr) && IsWordWrap(wndPtr))
			EDIT_BuildLineDefs(wndPtr);
		InvalidateRect32( wndPtr->hwndSelf, NULL, TRUE );
	}
	return 0L;
}


/*********************************************************************
 *
 *	WM_VSCROLL
 *
 *	FIXME: scrollbar code itself is broken, so this one is a hack.
 *
 */
static LRESULT EDIT_WM_VScroll(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	UINT lc = (UINT)EDIT_EM_GetLineCount(wndPtr, 0, 0L);
	UINT fv = (UINT)EDIT_EM_GetFirstVisibleLine(wndPtr, 0, 0L);
	UINT vlc = EDIT_GetVisibleLineCount(wndPtr);
	INT dy = 0;
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
		dy = -vlc;
		break;
	case SB_PAGEDOWN:
		dy = vlc;
		break;
	case SB_TOP:
		dy = -fv;
		break;
	case SB_BOTTOM:
		dy = lc - 1 - fv;
		break;
	case SB_THUMBTRACK:
/*
 *		not = FALSE;
 */
	case SB_THUMBPOSITION:
		dy = LOWORD(lParam) * (lc - 1) / 100 - fv;
		break;
	/* The next two are undocumented ! */
	case EM_GETTHUMB:
		ret = (lc > 1) ? MAKELONG(fv * 100 / (lc - 1), 0) : 0L;
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
