/*
 *	Edit control
 *
 *	Copyright  David W. Metcalfe, 1994
 *	Copyright  William Magro, 1995, 1996
 *	Copyright  Frans van Dorsselaer, 1996, 1997
 *
 */

/*
 *	please read EDIT.TODO (and update it when you change things)
 *	It also contains a discussion about the 16 to 32 bit transition.
 *
 */


#define NO_TRANSITION_TYPES	/* This file is Win32-clean */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "windows.h"
#include "win.h"
#include "local.h"
#include "resource.h"
#include "stddebug.h"
#include "debug.h"
#include "xmalloc.h"
/*
#include "callback.h"
*/

#define BUFLIMIT_MULTI		65534	/* maximum text buffer length (not including '\0') */
					/* FIXME: BTW, new specs say 65535 (do you dare ???) */
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
	INT32 offset;
	INT32 length;
	LINE_END ending;
} LINEDEF;

typedef struct
{
	HLOCAL16 hBuf16;	/* For when a 16-bit multiline edit
				 * control gets a EM_GETHANDLE (which
				 * should return 16-bit local heap).
				 * From that point on we _have_ to keep
				 * using 16-bit local heap (apps rely
				 * on that ... bummer).
				 */
	HLOCAL32 hBuf32;	/* Don't worry about 'LOCAL'.  LOCAL32 is
				 * identical to GLOBAL32, which is
				 * essentially a HANDLE32 created with
				 * HeapAlloc(GetProcessHeap(), ...) plus
				 * a global32 (and thus local32)
				 * descriptor, which we can return upon
				 * EM_GETHANDLE32.
				 * It is 32-bit linear addressing, so
				 * everything is fine.
				 */
	LPSTR text;		/* Depending on the fact that we are a
				 * 16 or 32 bit control, this is the
				 * pointer that we get after
				 * LocalLock32(hBuf23) (which is a typecast :-)
				 * or LOCAL_Lock(hBuf16).
				 * This is always a 32-bit linear pointer.
				 */
	HFONT32 hFont;
	LINEDEF *LineDefs;	/* Internal table for (soft) linebreaks */
	INT32 TextWidth;	/* width of the widest line in pixels */
	INT32 XOffset;		/* offset of the viewport in pixels */
	INT32 FirstVisibleLine;
	INT32 LineCount;
	INT32 LineHeight;	/* height of a screen line in pixels */
	INT32 AveCharWidth;	/* average character width in pixels */
	INT32 BufLimit;
	INT32 BufSize;
	BOOL32 TextChanged;
	INT32 UndoInsertLen;
	INT32 UndoPos;
	INT32 UndoBufSize;
	HLOCAL32 hUndoBuf;
	LPSTR UndoText;
	BOOL32 Redraw;
	INT32 SelStart;		/* offset of selection start, == SelEnd if no selection */
	INT32 SelEnd;		/* offset of selection end == current caret position */
	INT32 NumTabStops;
	LPINT16 TabStops;
	/*
	 *	FIXME: The following should probably be a (VOID *) that is
	 *	typecast to either 16- or 32-bit callback when used,
	 *	depending on the type of edit control (16 or 32 bit).
	 *
	 *	EDITWORDBREAKPROC WordBreakProc;
	 *
	 *	For now: no more application specific wordbreaking.
	 *	(Internal wordbreak function still works)
	 */
	CHAR PasswordChar;
	INT32 LeftMargin;
	INT32 RightMargin;
	RECT32 FormatRect;
} EDITSTATE;


#define SWAP_INT32(x,y) do { INT32 temp = (INT32)(x); (x) = (INT32)(y); (y) = temp; } while(0)
#define ORDER_INT32(x,y) do { if ((INT32)(y) < (INT32)(x)) SWAP_INT32((x),(y)); } while(0)

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
#define DPRINTF_EDIT_MSG16(str) \
    dprintf_edit(stddeb, \
		 "edit: 16 bit : " str ": hwnd=%08x, wParam=%08x, lParam=%08x\n", \
		 (UINT32)hwnd, (UINT32)wParam, (UINT32)lParam)
#define DPRINTF_EDIT_MSG32(str) \
    dprintf_edit(stddeb, \
		 "edit: 32 bit : " str ": hwnd=%08x, wParam=%08x, lParam=%08x\n", \
		 (UINT32)hwnd, (UINT32)wParam, (UINT32)lParam)


/*********************************************************************
 *
 *	Declarations
 *
 *	Files like these should really be kept in alphabetical order.
 *
 */
LRESULT EditWndProc(HWND32 hwnd, UINT32 msg, WPARAM32 wParam, LPARAM lParam);

static void    EDIT_BuildLineDefs(WND *wndPtr);
static INT32   EDIT_CallWordBreakProc(WND *wndPtr, LPSTR s, INT32 index, INT32 count, INT32 action);
static INT32   EDIT_ColFromWndX(WND *wndPtr, INT32 line, INT32 x);
static void    EDIT_DelEnd(WND *wndPtr);
static void    EDIT_DelLeft(WND *wndPtr);
static void    EDIT_DelRight(WND *wndPtr);
static INT32   EDIT_GetAveCharWidth(WND *wndPtr);
static INT32   EDIT_GetLineHeight(WND *wndPtr);
static void    EDIT_GetLineRect(WND *wndPtr, INT32 line, INT32 scol, INT32 ecol, LPRECT32 rc);
static LPSTR   EDIT_GetPointer(WND *wndPtr);
static LPSTR   EDIT_GetPasswordPointer(WND *wndPtr);
static BOOL32  EDIT_GetRedraw(WND *wndPtr);
static void    EDIT_GetSel(WND *wndPtr, LPINT32 s, LPINT32 e);
static INT32   EDIT_GetTextWidth(WND *wndPtr);
static LPSTR   EDIT_GetUndoPointer(WND *wndPtr);
static INT32   EDIT_GetVisibleLineCount(WND *wndPtr);
static INT32   EDIT_GetWndWidth(WND *wndPtr);
static INT32   EDIT_GetXOffset(WND *wndPtr);
static void    EDIT_InvalidateText(WND *wndPtr, INT32 start, INT32 end);
static INT32   EDIT_LineFromWndY(WND *wndPtr, INT32 y);
static BOOL32  EDIT_MakeFit(WND *wndPtr, INT32 size);
static BOOL32  EDIT_MakeUndoFit(WND *wndPtr, INT32 size);
static void    EDIT_MoveBackward(WND *wndPtr, BOOL32 extend);
static void    EDIT_MoveDownward(WND *wndPtr, BOOL32 extend);
static void    EDIT_MoveEnd(WND *wndPtr, BOOL32 extend);
static void    EDIT_MoveForward(WND *wndPtr, BOOL32 extend);
static void    EDIT_MoveHome(WND *wndPtr, BOOL32 extend);
static void    EDIT_MovePageDown(WND *wndPtr, BOOL32 extend);
static void    EDIT_MovePageUp(WND *wndPtr, BOOL32 extend);
static void    EDIT_MoveUpward(WND *wndPtr, BOOL32 extend);
static void    EDIT_MoveWordBackward(WND *wndPtr, BOOL32 extend);
static void    EDIT_MoveWordForward(WND *wndPtr, BOOL32 extend);
static void    EDIT_PaintLine(WND *wndPtr, HDC32 hdc, INT32 line, BOOL32 rev);
static INT32   EDIT_PaintText(WND *wndPtr, HDC32 hdc, INT32 x, INT32 y, INT32 line, INT32 col, INT32 count, BOOL32 rev);
static void    EDIT_ReleasePointer(WND *wndPtr);
static void    EDIT_ReleaseUndoPointer(WND *wndPtr);
static void    EDIT_SetSel(WND *wndPtr, INT32 ns, INT32 ne);
static INT32   EDIT_WndXFromCol(WND *wndPtr, INT32 line, INT32 col);
static INT32   EDIT_WndYFromLine(WND *wndPtr, INT32 line);
static INT32   EDIT_WordBreakProc(LPSTR s, INT32 index, INT32 count, INT32 action);

static LRESULT EDIT_EM_CanUndo(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_CharFromPos(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_EmptyUndoBuffer(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_FmtLines(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_GetFirstVisibleLine(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_GetHandle(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_GetHandle16(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_GetLimitText(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_GetLine(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_GetLineCount(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_GetMargins(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_GetModify(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_GetPasswordChar(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_GetRect(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_GetRect16(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_GetSel(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_GetThumb(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_GetWordBreakProc(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_LineFromChar(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_LineIndex(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_LineLength(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_LineScroll(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_PosFromChar(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_ReplaceSel(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_Scroll(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_ScrollCaret(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_SetHandle(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_SetHandle16(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_SetLimitText(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_SetMargins(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_SetModify(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_SetPasswordChar(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_SetReadOnly(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_SetRect(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_SetRectNP(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_SetSel(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_SetSel16(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_SetTabStops(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_SetTabStops16(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_SetWordBreakProc(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_EM_Undo(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);

static LRESULT EDIT_WM_Char(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_WM_Clear(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_WM_Command(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_WM_ContextMenu(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
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
static LRESULT EDIT_WM_InitMenuPopup(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
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
static LRESULT EDIT_WM_SysKeyDown(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_WM_Timer(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static LRESULT EDIT_WM_VScroll(WND *wndPtr, WPARAM32 wParam, LPARAM lParam);


/*********************************************************************
 *
 *	General shortcuts for variable names:
 *
 *	INT32 l;	line
 *	INT32 c;	column
 *	INT32 s;	offset of selection start
 *	INT32 e;	offset of selection end
 *	INT32 sl;	line on which the selection starts
 *	INT32 el;	line on which the selection ends
 *	INT32 sc;	column on which the selection starts
 *	INT32 ec;	column on which the selection ends
 *	INT32 li;	line index (offset)
 *	INT32 fv;	first visible line
 *	INT32 vlc;	vissible line count
 *	INT32 lc;	line count
 *	INT32 lh;	line height (in pixels)
 *	INT32 tw;	text width (in pixels)
 *	INT32 ww;	window width (in pixels)
 *	INT32 cw;	character width (average, in pixels)
 *
 */


/*********************************************************************
 *
 *	EditWndProc()
 *
 *	The messages are in the order of the actual integer values
 *	(which can be found in include/windows.h)
 *	Whereever possible the 16 bit versions are converted to
 *	the 32 bit ones, so that we can 'fall through' to the
 *	helper functions.  These are mostly 32 bit (with a few
 *	exceptions, clearly indicated by a '16' extension to their
 *	names).
 *
 */
LRESULT EditWndProc(HWND32 hwnd, UINT32 msg, WPARAM32 wParam, LPARAM lParam)
{
	LRESULT lResult = 0;
	WND *wndPtr = WIN_FindWndPtr(hwnd);

	if ((!EDITSTATEPTR(wndPtr)) && (msg != WM_CREATE))
		return DefWindowProc32A(hwnd, msg, wParam, lParam);

	switch (msg) {
	case EM_GETSEL16:
		DPRINTF_EDIT_MSG16("EM_GETSEL");
		wParam = 0;
		lParam = 0;
		/* fall through */
	case EM_GETSEL32:
		DPRINTF_EDIT_MSG32("EM_GETSEL");
		lResult = EDIT_EM_GetSel(wndPtr, wParam, lParam);
		break;

	case EM_SETSEL16:
		DPRINTF_EDIT_MSG16("EM_SETSEL");
		lResult = EDIT_EM_SetSel16(wndPtr, wParam, lParam);
		break;
	case EM_SETSEL32:
		DPRINTF_EDIT_MSG32("EM_SETSEL");
		lResult = EDIT_EM_SetSel(wndPtr, wParam, lParam);
		break;

	case EM_GETRECT16:
		DPRINTF_EDIT_MSG16("EM_GETRECT");
		lResult = EDIT_EM_GetRect16(wndPtr, wParam, lParam);
		break;
	case EM_GETRECT32:
		DPRINTF_EDIT_MSG32("EM_GETRECT");
		lResult = EDIT_EM_GetRect(wndPtr, wParam, lParam);
		break;

	case EM_SETRECT16:
		DPRINTF_EDIT_MSG16("EM_SETRECT");
		/* fall through */
	case EM_SETRECT32:
		DPRINTF_EDIT_MSG32("EM_SETRECT");
		lResult = EDIT_EM_SetRect(wndPtr, wParam, lParam);
		break;

	case EM_SETRECTNP16:
		DPRINTF_EDIT_MSG16("EM_SETRECTNP");
		/* fall through */
	case EM_SETRECTNP32:
		DPRINTF_EDIT_MSG32("EM_SETRECTNP");
		lResult = EDIT_EM_SetRectNP(wndPtr, wParam, lParam);
		break;

	case EM_SCROLL16:
		DPRINTF_EDIT_MSG16("EM_SCROLL");
		/* fall through */
	case EM_SCROLL32:
		DPRINTF_EDIT_MSG32("EM_SCROLL");
		lResult = EDIT_EM_Scroll(wndPtr, wParam, lParam);
 		break;

	case EM_LINESCROLL16:
		DPRINTF_EDIT_MSG16("EM_LINESCROLL");
		wParam = (WPARAM32)(INT32)(INT16)HIWORD(lParam);
		lParam = (LPARAM)(INT32)(INT16)LOWORD(lParam);
		/* fall through */
	case EM_LINESCROLL32:
		DPRINTF_EDIT_MSG32("EM_LINESCROLL");
		lResult = EDIT_EM_LineScroll(wndPtr, wParam, lParam);
		break;

	case EM_SCROLLCARET16:
		DPRINTF_EDIT_MSG16("EM_SCROLLCARET");
		/* fall through */
	case EM_SCROLLCARET32:
		DPRINTF_EDIT_MSG32("EM_SCROLLCARET");
		lResult = EDIT_EM_ScrollCaret(wndPtr, wParam, lParam);
		break;

	case EM_GETMODIFY16:
		DPRINTF_EDIT_MSG16("EM_GETMODIFY");
		/* fall through */
	case EM_GETMODIFY32:
		DPRINTF_EDIT_MSG32("EM_GETMODIFY");
		lResult = EDIT_EM_GetModify(wndPtr, wParam, lParam);
		break;

	case EM_SETMODIFY16:
		DPRINTF_EDIT_MSG16("EM_SETMODIFY");
		/* fall through */
	case EM_SETMODIFY32:
		DPRINTF_EDIT_MSG32("EM_SETMODIFY");
		lResult = EDIT_EM_SetModify(wndPtr, wParam, lParam);
		break;

	case EM_GETLINECOUNT16:
		DPRINTF_EDIT_MSG16("EM_GETLINECOUNT");
		/* fall through */
	case EM_GETLINECOUNT32:
		DPRINTF_EDIT_MSG32("EM_GETLINECOUNT");
		lResult = EDIT_EM_GetLineCount(wndPtr, wParam, lParam);
		break;

	case EM_LINEINDEX16:
		DPRINTF_EDIT_MSG16("EM_LINEINDEX");
		/* fall through */
	case EM_LINEINDEX32:
		DPRINTF_EDIT_MSG32("EM_LINEINDEX");
		lResult = EDIT_EM_LineIndex(wndPtr, wParam, lParam);
		break;

	case EM_SETHANDLE16:
		DPRINTF_EDIT_MSG16("EM_SETHANDLE");
		lResult = EDIT_EM_SetHandle16(wndPtr, wParam, lParam);
		break;
	case EM_SETHANDLE32:
		DPRINTF_EDIT_MSG32("EM_SETHANDLE");
		lResult = EDIT_EM_SetHandle(wndPtr, wParam, lParam);
		break;

	case EM_GETHANDLE16:
		DPRINTF_EDIT_MSG16("EM_GETHANDLE");
		lResult = EDIT_EM_GetHandle16(wndPtr, wParam, lParam);
		break;
	case EM_GETHANDLE32:
		DPRINTF_EDIT_MSG32("EM_GETHANDLE");
		lResult = EDIT_EM_GetHandle(wndPtr, wParam, lParam);
		break;

	case EM_GETTHUMB16:
		DPRINTF_EDIT_MSG16("EM_GETTHUMB");
		/* fall through */
	case EM_GETTHUMB32:
		DPRINTF_EDIT_MSG32("EM_GETTHUMB");
		lResult = EDIT_EM_GetThumb(wndPtr, wParam, lParam);
		break;

	/* messages 0x00bf and 0x00c0 missing from specs */

	case WM_USER+15:
		DPRINTF_EDIT_MSG16("undocumented WM_USER+15, please report");
		/* fall through */
	case 0x00bf:
		DPRINTF_EDIT_MSG32("undocumented 0x00bf, please report");
		lResult = DefWindowProc32A(hwnd, msg, wParam, lParam);
		break;

	case WM_USER+16:
		DPRINTF_EDIT_MSG16("undocumented WM_USER+16, please report");
		/* fall through */
	case 0x00c0:
		DPRINTF_EDIT_MSG32("undocumented 0x00c0, please report");
		lResult = DefWindowProc32A(hwnd, msg, wParam, lParam);
		break;

	case EM_LINELENGTH16:
		DPRINTF_EDIT_MSG16("EM_LINELENGTH");
		/* fall through */
	case EM_LINELENGTH32:
		DPRINTF_EDIT_MSG32("EM_LINELENGTH");
		lResult = EDIT_EM_LineLength(wndPtr, wParam, lParam);
		break;

	case EM_REPLACESEL16:
		DPRINTF_EDIT_MSG16("EM_REPLACESEL");
		lParam = (LPARAM)PTR_SEG_TO_LIN((SEGPTR)lParam);
		/* fall through */
	case EM_REPLACESEL32:
		DPRINTF_EDIT_MSG32("EM_REPLACESEL");
		lResult = EDIT_EM_ReplaceSel(wndPtr, wParam, lParam);
		break;

	/* message 0x00c3 missing from specs */

	case WM_USER+19:
		DPRINTF_EDIT_MSG16("undocumented WM_USER+19, please report");
		/* fall through */
	case 0x00c3:
		DPRINTF_EDIT_MSG32("undocumented 0x00c3, please report");
		lResult = DefWindowProc32A(hwnd, msg, wParam, lParam);
		break;

	case EM_GETLINE16:
		DPRINTF_EDIT_MSG16("EM_GETLINE");
		lParam = (LPARAM)PTR_SEG_TO_LIN((SEGPTR)lParam);
		/* fall through */
	case EM_GETLINE32:
		DPRINTF_EDIT_MSG32("EM_GETLINE");
		lResult = EDIT_EM_GetLine(wndPtr, wParam, lParam);
		break;

	case EM_LIMITTEXT16:
		DPRINTF_EDIT_MSG16("EM_LIMITTEXT");
		/* fall through */
	case EM_SETLIMITTEXT32:
		DPRINTF_EDIT_MSG32("EM_SETLIMITTEXT");
		lResult = EDIT_EM_SetLimitText(wndPtr, wParam, lParam);
		break;

	case EM_CANUNDO16:
		DPRINTF_EDIT_MSG16("EM_CANUNDO");
		/* fall through */
	case EM_CANUNDO32:
		DPRINTF_EDIT_MSG32("EM_CANUNDO");
		lResult = EDIT_EM_CanUndo(wndPtr, wParam, lParam);
		break;

	case EM_UNDO16:
		DPRINTF_EDIT_MSG16("EM_UNDO");
		/* fall through */
	case EM_UNDO32:
		/* fall through */
	case WM_UNDO:
		DPRINTF_EDIT_MSG32("EM_UNDO / WM_UNDO");
		lResult = EDIT_EM_Undo(wndPtr, wParam, lParam);
		break;

	case EM_FMTLINES16:
		DPRINTF_EDIT_MSG16("EM_FMTLINES");
		/* fall through */
	case EM_FMTLINES32:
		DPRINTF_EDIT_MSG32("EM_FMTLINES");
		lResult = EDIT_EM_FmtLines(wndPtr, wParam, lParam);
		break;

	case EM_LINEFROMCHAR16:
		DPRINTF_EDIT_MSG16("EM_LINEFROMCHAR");
		/* fall through */
	case EM_LINEFROMCHAR32:
		DPRINTF_EDIT_MSG32("EM_LINEFROMCHAR");
		lResult = EDIT_EM_LineFromChar(wndPtr, wParam, lParam);
		break;

	/* message 0x00ca missing from specs */

	case WM_USER+26:
		DPRINTF_EDIT_MSG16("undocumented WM_USER+26, please report");
		/* fall through */
	case 0x00ca:
		DPRINTF_EDIT_MSG32("undocumented 0x00ca, please report");
		lResult = DefWindowProc32A(hwnd, msg, wParam, lParam);
		break;

	case EM_SETTABSTOPS16:
		DPRINTF_EDIT_MSG16("EM_SETTABSTOPS");
		lResult = EDIT_EM_SetTabStops16(wndPtr, wParam, lParam);
		break;
	case EM_SETTABSTOPS32:
		DPRINTF_EDIT_MSG32("EM_SETTABSTOPS");
		lResult = EDIT_EM_SetTabStops(wndPtr, wParam, lParam);
		break;

	case EM_SETPASSWORDCHAR16:
		DPRINTF_EDIT_MSG16("EM_SETPASSWORDCHAR");
		/* fall through */
	case EM_SETPASSWORDCHAR32:
		DPRINTF_EDIT_MSG32("EM_SETPASSWORDCHAR");
		lResult = EDIT_EM_SetPasswordChar(wndPtr, wParam, lParam);
		break;

	case EM_EMPTYUNDOBUFFER16:
		DPRINTF_EDIT_MSG16("EM_EMPTYUNDOBUFFER");
		/* fall through */
	case EM_EMPTYUNDOBUFFER32:
		DPRINTF_EDIT_MSG32("EM_EMPTYUNDOBUFFER");
		lResult = EDIT_EM_EmptyUndoBuffer(wndPtr, wParam, lParam);
		break;

	case EM_GETFIRSTVISIBLELINE16:
		DPRINTF_EDIT_MSG16("EM_GETFIRSTVISIBLELINE");
		/* fall through */
	case EM_GETFIRSTVISIBLELINE32:
		DPRINTF_EDIT_MSG32("EM_GETFIRSTVISIBLELINE");
		lResult = EDIT_EM_GetFirstVisibleLine(wndPtr, wParam, lParam);
		break;

	case EM_SETREADONLY16:
		DPRINTF_EDIT_MSG16("EM_SETREADONLY");
		/* fall through */
	case EM_SETREADONLY32:
		DPRINTF_EDIT_MSG32("EM_SETREADONLY");
		lResult = EDIT_EM_SetReadOnly(wndPtr, wParam, lParam);
 		break;

	case EM_SETWORDBREAKPROC16:
		DPRINTF_EDIT_MSG16("EM_SETWORDBREAKPROC");
		/* fall through */
	case EM_SETWORDBREAKPROC32:
		DPRINTF_EDIT_MSG32("EM_SETWORDBREAKPROC");
		lResult = EDIT_EM_SetWordBreakProc(wndPtr, wParam, lParam);
		break;

	case EM_GETWORDBREAKPROC16:
		DPRINTF_EDIT_MSG16("EM_GETWORDBREAKPROC");
		/* fall through */
	case EM_GETWORDBREAKPROC32:
		DPRINTF_EDIT_MSG32("EM_GETWORDBREAKPROC");
		lResult = EDIT_EM_GetWordBreakProc(wndPtr, wParam, lParam);
		break;

	case EM_GETPASSWORDCHAR16:
		DPRINTF_EDIT_MSG16("EM_GETPASSWORDCHAR");
		/* fall through */
	case EM_GETPASSWORDCHAR32:
		DPRINTF_EDIT_MSG32("EM_GETPASSWORDCHAR");
		lResult = EDIT_EM_GetPasswordChar(wndPtr, wParam, lParam);
		break;

	/* The following EM_xxx are new to win95 and don't exist for 16 bit */

	case EM_SETMARGINS32:
		DPRINTF_EDIT_MSG16("EM_SETMARGINS");
		lResult = EDIT_EM_SetMargins(wndPtr, wParam, lParam);
		break;

	case EM_GETMARGINS32:
		DPRINTF_EDIT_MSG16("EM_GETMARGINS");
		lResult = EDIT_EM_GetMargins(wndPtr, wParam, lParam);
		break;

	case EM_GETLIMITTEXT32:
		DPRINTF_EDIT_MSG16("EM_GETLIMITTEXT");
		lResult = EDIT_EM_GetLimitText(wndPtr, wParam, lParam);
		break;

	case EM_POSFROMCHAR32:
		DPRINTF_EDIT_MSG16("EM_POSFROMCHAR");
		lResult = EDIT_EM_PosFromChar(wndPtr, wParam, lParam);
		break;

	case EM_CHARFROMPOS32:
		DPRINTF_EDIT_MSG16("EM_CHARFROMPOS");
		lResult = EDIT_EM_CharFromPos(wndPtr, wParam, lParam);
		break;

	case WM_GETDLGCODE:
		DPRINTF_EDIT_MSG32("WM_GETDLGCODE");
		lResult = EDIT_WM_GetDlgCode(wndPtr, wParam, lParam);
		break;

	case WM_CHAR:
		DPRINTF_EDIT_MSG32("WM_CHAR");
		lResult = EDIT_WM_Char(wndPtr, wParam, lParam);
		break;

	case WM_CLEAR:
		DPRINTF_EDIT_MSG32("WM_CLEAR");
		lResult = EDIT_WM_Clear(wndPtr, wParam, lParam);
		break;

	case WM_COMMAND:
		DPRINTF_EDIT_MSG32("WM_COMMAND");
		lResult = EDIT_WM_Command(wndPtr, wParam, lParam);
		break;

/*
 *	FIXME: when this one is added to WINE, change RBUTTONUP to CONTEXTMENU
 *	Furthermore, coordinate conversion should no longer be required
 *
 *	case WM_CONTEXTMENU:
 */
 	case WM_RBUTTONUP:
		DPRINTF_EDIT_MSG32("WM_RBUTTONUP");
		ClientToScreen16(wndPtr->hwndSelf, (LPPOINT16)&lParam);
		lResult = EDIT_WM_ContextMenu(wndPtr, wParam, lParam);
		break;

	case WM_COPY:
		DPRINTF_EDIT_MSG32("WM_COPY");
		lResult = EDIT_WM_Copy(wndPtr, wParam, lParam);
		break;

	case WM_CREATE:
		DPRINTF_EDIT_MSG32("WM_CREATE");
		lResult = EDIT_WM_Create(wndPtr, wParam, lParam);
		break;

	case WM_CUT:
		DPRINTF_EDIT_MSG32("WM_CUT");
		lResult = EDIT_WM_Cut(wndPtr, wParam, lParam);
		break;

	case WM_DESTROY:
		DPRINTF_EDIT_MSG32("WM_DESTROY");
		lResult = EDIT_WM_Destroy(wndPtr, wParam, lParam);
		break;

	case WM_ENABLE:
		DPRINTF_EDIT_MSG32("WM_ENABLE");
		lResult = EDIT_WM_Enable(wndPtr, wParam, lParam);
		break;

	case WM_ERASEBKGND:
		DPRINTF_EDIT_MSG32("WM_ERASEBKGND");
		lResult = EDIT_WM_EraseBkGnd(wndPtr, wParam, lParam);
		break;

	case WM_GETFONT:
		DPRINTF_EDIT_MSG32("WM_GETFONT");
		lResult = EDIT_WM_GetFont(wndPtr, wParam, lParam);
		break;

	case WM_GETTEXT:
		DPRINTF_EDIT_MSG32("WM_GETTEXT");
		lResult = EDIT_WM_GetText(wndPtr, wParam, lParam);
		break;

	case WM_GETTEXTLENGTH:
		DPRINTF_EDIT_MSG32("WM_GETTEXTLENGTH");
		lResult = EDIT_WM_GetTextLength(wndPtr, wParam, lParam);
		break;

	case WM_HSCROLL:
		DPRINTF_EDIT_MSG32("WM_HSCROLL");
		lResult = EDIT_WM_HScroll(wndPtr, wParam, lParam);
		break;

	case WM_INITMENUPOPUP:
		DPRINTF_EDIT_MSG32("WM_INITMENUPOPUP");
		lResult = EDIT_WM_InitMenuPopup(wndPtr, wParam, lParam);
		break;

	case WM_KEYDOWN:
		DPRINTF_EDIT_MSG32("WM_KEYDOWN");
		lResult = EDIT_WM_KeyDown(wndPtr, wParam, lParam);
		break;

	case WM_KILLFOCUS:
		DPRINTF_EDIT_MSG32("WM_KILLFOCUS");
		lResult = EDIT_WM_KillFocus(wndPtr, wParam, lParam);
		break;

	case WM_LBUTTONDBLCLK:
		DPRINTF_EDIT_MSG32("WM_LBUTTONDBLCLK");
		lResult = EDIT_WM_LButtonDblClk(wndPtr, wParam, lParam);
		break;

	case WM_LBUTTONDOWN:
		DPRINTF_EDIT_MSG32("WM_LBUTTONDOWN");
		lResult = EDIT_WM_LButtonDown(wndPtr, wParam, lParam);
		break;

	case WM_LBUTTONUP:
		DPRINTF_EDIT_MSG32("WM_LBUTTONUP");
		lResult = EDIT_WM_LButtonUp(wndPtr, wParam, lParam);
		break;

	case WM_MOUSEMOVE:
		/*
		 *	DPRINTF_EDIT_MSG32("WM_MOUSEMOVE");
		 */
		lResult = EDIT_WM_MouseMove(wndPtr, wParam, lParam);
		break;

	case WM_PAINT:
		DPRINTF_EDIT_MSG32("WM_PAINT");
		lResult = EDIT_WM_Paint(wndPtr, wParam, lParam);
		break;

	case WM_PASTE:
		DPRINTF_EDIT_MSG32("WM_PASTE");
		lResult = EDIT_WM_Paste(wndPtr, wParam, lParam);
		break;

	case WM_SETCURSOR:
		/*
		 *	DPRINTF_EDIT_MSG32("WM_SETCURSOR");
		 */
		lResult = EDIT_WM_SetCursor(wndPtr, wParam, lParam);
		break;

	case WM_SETFOCUS:
		DPRINTF_EDIT_MSG32("WM_SETFOCUS");
		lResult = EDIT_WM_SetFocus(wndPtr, wParam, lParam);
		break;

	case WM_SETFONT:
		DPRINTF_EDIT_MSG32("WM_SETFONT");
		lResult = EDIT_WM_SetFont(wndPtr, wParam, lParam);
		break;

	case WM_SETREDRAW:
		DPRINTF_EDIT_MSG32("WM_SETREDRAW");
		lResult = EDIT_WM_SetRedraw(wndPtr, wParam, lParam);
		break;

	case WM_SETTEXT:
		DPRINTF_EDIT_MSG32("WM_SETTEXT");
		lResult = EDIT_WM_SetText(wndPtr, wParam, lParam);
		break;

	case WM_SIZE:
		DPRINTF_EDIT_MSG32("WM_SIZE");
		lResult = EDIT_WM_Size(wndPtr, wParam, lParam);
		break;

	case WM_SYSKEYDOWN:
		DPRINTF_EDIT_MSG32("WM_SYSKEYDOWN");
		lResult = EDIT_WM_SysKeyDown(wndPtr, wParam, lParam);
		break;

	case WM_TIMER:
		DPRINTF_EDIT_MSG32("WM_TIMER");
		lResult = EDIT_WM_Timer(wndPtr, wParam, lParam);
		break;

	case WM_VSCROLL:
		DPRINTF_EDIT_MSG32("WM_VSCROLL");
		lResult = EDIT_WM_VScroll(wndPtr, wParam, lParam);
		break;

	default:
		lResult = DefWindowProc32A(hwnd, msg, wParam, lParam);
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
	LPSTR text = EDIT_GetPasswordPointer(wndPtr);
	INT32 ww = EDIT_GetWndWidth(wndPtr);
	HDC32 hdc;
	HFONT32 hFont;
	HFONT32 oldFont = 0;
	LPSTR start, cp;
	INT32 prev, next;
	INT32 width;
	INT32 length;
	LINE_END ending;

	hdc = GetDC32(wndPtr->hwndSelf);
	hFont = (HFONT32)EDIT_WM_GetFont(wndPtr, 0, 0);
	if (hFont) oldFont = SelectObject32(hdc, hFont);

	if (!IsMultiLine(wndPtr)) {
		es->LineCount = 1;
		es->LineDefs = xrealloc(es->LineDefs, sizeof(LINEDEF));
		es->LineDefs[0].offset = 0;
		es->LineDefs[0].length = EDIT_WM_GetTextLength(wndPtr, 0, 0);
		es->LineDefs[0].ending = END_0;
		es->TextWidth = (INT32)LOWORD(GetTabbedTextExtent(hdc, text,
					es->LineDefs[0].length,
					es->NumTabStops, es->TabStops));
	} else {
		es->LineCount = 0;
		start = text;
		do {
			if (!(cp = strstr(start, "\r\n"))) {
				ending = END_0;
				length = lstrlen32A(start);
			} else if ((cp > start) && (*(cp - 1) == '\r')) {
				ending = END_SOFT;
				length = cp - start - 1;
			} else {
				ending = END_HARD;
				length = cp - start;
			}
			width = (INT32)LOWORD(GetTabbedTextExtent(hdc, start, length,
						es->NumTabStops, es->TabStops));

			if (IsWordWrap(wndPtr) && (width > ww)) {
				next = 0;
				do {
					prev = next;
					next = EDIT_CallWordBreakProc(wndPtr, start,
							prev + 1, length, WB_RIGHT);
					width = (INT32)LOWORD(GetTabbedTextExtent(hdc, start, next,
							es->NumTabStops, es->TabStops));
				} while (width <= ww);
				if (!prev) {
					next = 0;
					do {
						prev = next;
						next++;
						width = (INT32)LOWORD(GetTabbedTextExtent(hdc, start, next,
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
				width = (INT32)LOWORD(GetTabbedTextExtent(hdc, start, length,
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
	if (hFont) SelectObject32(hdc, oldFont);
	ReleaseDC32(wndPtr->hwndSelf, hdc);

	free(text);
}


/*********************************************************************
 *
 *	EDIT_CallWordBreakProc
 *
 *	Call appropriate WordBreakProc (internal or external).
 *
 *	FIXME: Heavily broken now that we have a LOCAL32 buffer.
 *	External wordbreak functions have been disabled in
 *	EM_SETWORDBREAKPROC.
 *
 */
static INT32 EDIT_CallWordBreakProc(WND *wndPtr, LPSTR s, INT32 index, INT32 count, INT32 action)
{
	return EDIT_WordBreakProc(s, index, count, action);
/*
 *	EDITWORDBREAKPROC wbp = (EDITWORDBREAKPROC)EDIT_EM_GetWordBreakProc(wndPtr, 0, 0);
 *
 *	if (!wbp) return EDIT_WordBreakProc(s, index, count, action);
 *	else {
 *		EDITSTATE *es = EDITSTATEPTR(wndPtr);
 *		SEGPTR ptr = LOCAL_LockSegptr( wndPtr->hInstance, es->hBuf16 ) +
 *			(INT16)(s - EDIT_GetPointer(wndPtr));
 *		INT ret = CallWordBreakProc( (FARPROC16)wbp, ptr,
 *						index, count, action);
 *		LOCAL_Unlock( wndPtr->hInstance, es->hBuf16 );
 *		return ret;
 *	}
 */
}


/*********************************************************************
 *
 *	EDIT_ColFromWndX
 *
 *	Calculates, for a given line and X-coordinate on the screen, the column.
 *
 */
static INT32 EDIT_ColFromWndX(WND *wndPtr, INT32 line, INT32 x)
{
	INT32 lc = (INT32)EDIT_EM_GetLineCount(wndPtr, 0, 0);
	INT32 li = (INT32)EDIT_EM_LineIndex(wndPtr, line, 0);
	INT32 ll = (INT32)EDIT_EM_LineLength(wndPtr, li, 0);
	INT32 i;

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
	EDIT_EM_SetSel(wndPtr, -1, 0);
	EDIT_MoveEnd(wndPtr, TRUE);
	EDIT_WM_Clear(wndPtr, 0, 0);
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
	EDIT_EM_SetSel(wndPtr, -1, 0);
	EDIT_MoveBackward(wndPtr, TRUE);
	EDIT_WM_Clear(wndPtr, 0, 0);
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
	EDIT_EM_SetSel(wndPtr, -1, 0);
	EDIT_MoveForward(wndPtr, TRUE);
	EDIT_WM_Clear(wndPtr, 0, 0);
}


/*********************************************************************
 *
 *	EDIT_GetAveCharWidth
 *
 */
static INT32 EDIT_GetAveCharWidth(WND *wndPtr)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	return es->AveCharWidth;
}


/*********************************************************************
 *
 *	EDIT_GetLineHeight
 *
 */
static INT32 EDIT_GetLineHeight(WND *wndPtr)
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
static void EDIT_GetLineRect(WND *wndPtr, INT32 line, INT32 scol, INT32 ecol, LPRECT32 rc)
{
	rc->top = EDIT_WndYFromLine(wndPtr, line);
	rc->bottom = rc->top + EDIT_GetLineHeight(wndPtr);
	rc->left = EDIT_WndXFromCol(wndPtr, line, scol);
	rc->right = (ecol == -1) ? EDIT_GetWndWidth(wndPtr) :
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
static LPSTR EDIT_GetPointer(WND *wndPtr)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	if (!es->text && (es->hBuf32 || es->hBuf16)) {
		if (es->hBuf32)
			es->text = (LPSTR)LocalLock32(es->hBuf32);
		else
			es->text = LOCAL_Lock(wndPtr->hInstance, es->hBuf16);
	}
	return es->text;
}


/*********************************************************************
 *
 *	EDIT_GetPasswordPointer
 *
 *
 */
static LPSTR EDIT_GetPasswordPointer(WND *wndPtr)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	LPSTR text = xstrdup(EDIT_GetPointer(wndPtr));
	LPSTR p;

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
 *	EDIT_GetRedraw
 *
 */
static BOOL32 EDIT_GetRedraw(WND *wndPtr)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	return es->Redraw;
}


/*********************************************************************
 *
 *	EDIT_GetSel
 *
 *	Beware: This is not the function called on EM_GETSEL.
 *		This is the unordered version used internally
 *		(s can be > e).  No return value either.
 *
 */
static void EDIT_GetSel(WND *wndPtr, LPINT32 s, LPINT32 e)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	if (s)
		*s = es->SelStart;
	if (e)
		*e = es->SelEnd;
}


/*********************************************************************
 *
 *	EDIT_GetTextWidth
 *
 */
static INT32 EDIT_GetTextWidth(WND *wndPtr)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	return es->TextWidth;
}


/*********************************************************************
 *
 *	EDIT_GetUndoPointer
 *
 *	This acts as a LocalLock32(), but it locks only once.  This way
 *	you can call it whenever you like, without unlocking.
 *
 */
static LPSTR EDIT_GetUndoPointer(WND *wndPtr)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	if (!es->UndoText && es->hUndoBuf)
		es->UndoText = (LPSTR)LocalLock32(es->hUndoBuf);
	return es->UndoText;
}


/*********************************************************************
 *
 *	EDIT_GetVisibleLineCount
 *
 */
static INT32 EDIT_GetVisibleLineCount(WND *wndPtr)
{
	RECT32 rc;

	EDIT_EM_GetRect(wndPtr, 0, (LPARAM)&rc);
	return MAX(1, MAX(rc.bottom - rc.top, 0) / EDIT_GetLineHeight(wndPtr));
}


/*********************************************************************
 *
 *	EDIT_GetWndWidth
 *
 */
static INT32 EDIT_GetWndWidth(WND *wndPtr)
{
	RECT32 rc;

	EDIT_EM_GetRect(wndPtr, 0, (LPARAM)&rc);
	return rc.right - rc.left;
}


/*********************************************************************
 *
 *	EDIT_GetXOffset
 *
 */
static INT32 EDIT_GetXOffset(WND *wndPtr)
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
static void EDIT_InvalidateText(WND *wndPtr, INT32 start, INT32 end)
{
	INT32 fv = (INT32)EDIT_EM_GetFirstVisibleLine(wndPtr, 0, 0);
	INT32 vlc = EDIT_GetVisibleLineCount(wndPtr);
	INT32 sl;
	INT32 el;
	INT32 sc;
	INT32 ec;
	RECT32 rcWnd;
	RECT32 rcLine;
	RECT32 rcUpdate;
	INT32 l;

	if (end == start)
		return;

	if (end == -1)
		end = (INT32)EDIT_WM_GetTextLength(wndPtr, 0, 0);
	ORDER_INT32(start, end);
	sl = (INT32)EDIT_EM_LineFromChar(wndPtr, start, 0);
	el = (INT32)EDIT_EM_LineFromChar(wndPtr, end, 0);
	if ((el < fv) || (sl > fv + vlc))
		return;

	sc = start - (INT32)EDIT_EM_LineIndex(wndPtr, sl, 0);
	ec = end - (INT32)EDIT_EM_LineIndex(wndPtr, el, 0);
	if (sl < fv) {
		sl = fv;
		sc = 0;
	}
	if (el > fv + vlc) {
		el = fv + vlc;
		ec = (INT32)EDIT_EM_LineLength(wndPtr,
				(INT32)EDIT_EM_LineIndex(wndPtr, el, 0), 0);
	}
	EDIT_EM_GetRect(wndPtr, 0, (LPARAM)&rcWnd);
	if (sl == el) {
		EDIT_GetLineRect(wndPtr, sl, sc, ec, &rcLine);
		if (IntersectRect32(&rcUpdate, &rcWnd, &rcLine))
			InvalidateRect32( wndPtr->hwndSelf, &rcUpdate, FALSE );
	} else {
		EDIT_GetLineRect(wndPtr, sl, sc,
				(INT32)EDIT_EM_LineLength(wndPtr,
					(INT32)EDIT_EM_LineIndex(wndPtr, sl, 0), 0),
				&rcLine);
		if (IntersectRect32(&rcUpdate, &rcWnd, &rcLine))
			InvalidateRect32( wndPtr->hwndSelf, &rcUpdate, FALSE );
		for (l = sl + 1 ; l < el ; l++) {
			EDIT_GetLineRect(wndPtr, l, 0,
				(INT32)EDIT_EM_LineLength(wndPtr,
					(INT32)EDIT_EM_LineIndex(wndPtr, l, 0), 0),
				&rcLine);
			if (IntersectRect32(&rcUpdate, &rcWnd, &rcLine))
				InvalidateRect32(wndPtr->hwndSelf, &rcUpdate, FALSE);
		}
		EDIT_GetLineRect(wndPtr, el, 0, ec, &rcLine);
		if (IntersectRect32(&rcUpdate, &rcWnd, &rcLine))
			InvalidateRect32( wndPtr->hwndSelf, &rcUpdate, FALSE );
	}
}


/*********************************************************************
 *
 *	EDIT_LineFromWndY
 *
 *	Calculates, for a given Y-coordinate on the screen, the line.
 *
 */
static INT32 EDIT_LineFromWndY(WND *wndPtr, INT32 y)
{
	INT32 fv = (INT32)EDIT_EM_GetFirstVisibleLine(wndPtr, 0, 0);
	INT32 lh = EDIT_GetLineHeight(wndPtr);
	INT32 lc = (INT32)EDIT_EM_GetLineCount(wndPtr, 0, 0);

	return MAX(0, MIN(lc - 1, y / lh + fv));
}


/*********************************************************************
 *
 *	EDIT_MakeFit
 *
 *	Try to fit size + 1 bytes in the buffer.  Constrain to limits.
 *
 */
static BOOL32 EDIT_MakeFit(WND *wndPtr, INT32 size)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	HLOCAL32 hNew32;
	HLOCAL16 hNew16;

	if (size <= es->BufSize)
		return TRUE;
	if (size > es->BufLimit) {
		dprintf_edit(stddeb, "edit: notification EN_MAXTEXT sent\n");
		EDIT_NOTIFY_PARENT(wndPtr, EN_MAXTEXT);
		return FALSE;
	}
	size = ((size / GROWLENGTH) + 1) * GROWLENGTH;
	if (size > es->BufLimit)
		size = es->BufLimit;

	dprintf_edit(stddeb, "edit: EDIT_MakeFit: trying to ReAlloc to %d+1\n", size);

	EDIT_ReleasePointer(wndPtr);
	if (es->hBuf32) {
		if ((hNew32 = LocalReAlloc32(es->hBuf32, size + 1, 0))) {
			dprintf_edit(stddeb, "edit: EDIT_MakeFit: Old 32 bit handle %08x, new handle %08x\n", es->hBuf32, hNew32);
			es->hBuf32 = hNew32;
			es->BufSize = MIN(LocalSize32(es->hBuf32) - 1, es->BufLimit);
			if (es->BufSize < size) {
				dprintf_edit(stddeb, "edit: EDIT_MakeFit: FAILED !  We now have %d+1\n", es->BufSize);
				dprintf_edit(stddeb, "edit: notification EN_ERRSPACE sent\n");
				EDIT_NOTIFY_PARENT(wndPtr, EN_ERRSPACE);
				return FALSE;
			}
			dprintf_edit(stddeb, "edit: EDIT_MakeFit: We now have %d+1\n", es->BufSize);
			return TRUE;
		}
	} else {
		if ((hNew16 = LOCAL_ReAlloc(wndPtr->hInstance, es->hBuf16, size + 1, LMEM_MOVEABLE))) {
			dprintf_edit(stddeb, "edit: EDIT_MakeFit: Old 16 bit handle %08x, new handle %08x\n", es->hBuf16, hNew16);
			es->hBuf16 = hNew16;
			es->BufSize = MIN(LOCAL_Size(wndPtr->hInstance, es->hBuf16) - 1, es->BufLimit);
			if (es->BufSize < size) {
				dprintf_edit(stddeb, "edit: EDIT_MakeFit: FAILED !  We now have %d+1\n", es->BufSize);
				dprintf_edit(stddeb, "edit: notification EN_ERRSPACE sent\n");
				EDIT_NOTIFY_PARENT(wndPtr, EN_ERRSPACE);
				return FALSE;
			}
			dprintf_edit(stddeb, "edit: EDIT_MakeFit: We now have %d+1\n", es->BufSize);
			return TRUE;
		}
	}
	dprintf_edit(stddeb, "edit: EDIT_MakeFit: Reallocation failed\n");
	dprintf_edit(stddeb, "edit: notification EN_ERRSPACE sent\n");
	EDIT_NOTIFY_PARENT(wndPtr, EN_ERRSPACE);
	return FALSE;
}


/*********************************************************************
 *
 *	EDIT_MakeUndoFit
 *
 *	Try to fit size + 1 bytes in the undo buffer.
 *
 */
static BOOL32 EDIT_MakeUndoFit(WND *wndPtr, INT32 size)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	HLOCAL32 hNew;

	if (size <= es->UndoBufSize)
		return TRUE;
	size = ((size / GROWLENGTH) + 1) * GROWLENGTH;

	dprintf_edit(stddeb, "edit: EDIT_MakeUndoFit: trying to ReAlloc to %d+1\n", size);

	EDIT_ReleaseUndoPointer(wndPtr);
	if ((hNew = LocalReAlloc32(es->hUndoBuf, size + 1, 0))) {
		dprintf_edit(stddeb, "edit: EDIT_MakeUndoFit: Old handle %08x, new handle %08x\n", es->hUndoBuf, hNew);
		es->hUndoBuf = hNew;
		es->UndoBufSize = LocalSize32(es->hUndoBuf) - 1;
		if (es->UndoBufSize < size) {
			dprintf_edit(stddeb, "edit: EDIT_MakeUndoFit: FAILED !  We now have %d+1\n", es->UndoBufSize);
			return FALSE;
		}
		dprintf_edit(stddeb, "edit: EDIT_MakeUndoFit: We now have %d+1\n", es->UndoBufSize);
		return TRUE;
	}
	return FALSE;
}


/*********************************************************************
 *
 *	EDIT_MoveBackward
 *
 */
static void EDIT_MoveBackward(WND *wndPtr, BOOL32 extend)
{
	INT32 s;
	INT32 e;
	INT32 l;
	INT32 li;

	EDIT_GetSel(wndPtr, &s, &e);
	l = (INT32)EDIT_EM_LineFromChar(wndPtr, e, 0);
	li = (INT32)EDIT_EM_LineIndex(wndPtr, l, 0);
	if (e - li == 0) {
		if (l) {
			li = (INT32)EDIT_EM_LineIndex(wndPtr, l - 1, 0);
			e = li + (INT32)EDIT_EM_LineLength(wndPtr, li, 0);
		}
	} else
		e--;
	if (!extend)
		s = e;
	EDIT_SetSel(wndPtr, s, e);
	EDIT_EM_ScrollCaret(wndPtr, 0, 0);
}


/*********************************************************************
 *
 *	EDIT_MoveDownward
 *
 */
static void EDIT_MoveDownward(WND *wndPtr, BOOL32 extend)
{
	INT32 s;
	INT32 e;
	INT32 l;
	INT32 lc;
	INT32 li;
	INT32 x;

	EDIT_GetSel(wndPtr, &s, &e);
	l = (INT32)EDIT_EM_LineFromChar(wndPtr, e, 0);
	lc = (INT32)EDIT_EM_GetLineCount(wndPtr, e, 0);
	li = (INT32)EDIT_EM_LineIndex(wndPtr, l, 0);
	if (l < lc - 1) {
		x = EDIT_WndXFromCol(wndPtr, l, e - li);
		l++;
		e = (INT32)EDIT_EM_LineIndex(wndPtr, l, 0) +
				EDIT_ColFromWndX(wndPtr, l, x);
	}
	if (!extend)
		s = e;
	EDIT_SetSel(wndPtr, s, e);
	EDIT_EM_ScrollCaret(wndPtr, 0, 0);
}


/*********************************************************************
 *
 *	EDIT_MoveEnd
 *
 */
static void EDIT_MoveEnd(WND *wndPtr, BOOL32 extend)
{
	INT32 s;
	INT32 e;
	INT32 l;
	INT32 ll;
	INT32 li;

	EDIT_GetSel(wndPtr, &s, &e);
	l = (INT32)EDIT_EM_LineFromChar(wndPtr, e, 0);
	ll = (INT32)EDIT_EM_LineLength(wndPtr, e, 0);
	li = (INT32)EDIT_EM_LineIndex(wndPtr, l, 0);
	e = li + ll;
	if (!extend)
		s = e;
	EDIT_SetSel(wndPtr, s, e);
	EDIT_EM_ScrollCaret(wndPtr, 0, 0);
}


/*********************************************************************
 *
 *	EDIT_MoveForward
 *
 */
static void EDIT_MoveForward(WND *wndPtr, BOOL32 extend)
{
	INT32 s;
	INT32 e;
	INT32 l;
	INT32 lc;
	INT32 ll;
	INT32 li;

	EDIT_GetSel(wndPtr, &s, &e);
	l = (INT32)EDIT_EM_LineFromChar(wndPtr, e, 0);
	lc = (INT32)EDIT_EM_GetLineCount(wndPtr, e, 0);
	ll = (INT32)EDIT_EM_LineLength(wndPtr, e, 0);
	li = (INT32)EDIT_EM_LineIndex(wndPtr, l, 0);
	if (e - li == ll) {
		if (l != lc - 1)
			e = (INT32)EDIT_EM_LineIndex(wndPtr, l + 1, 0);
	} else
		e++;
	if (!extend)
		s = e;
	EDIT_SetSel(wndPtr, s, e);
	EDIT_EM_ScrollCaret(wndPtr, 0, 0);
}


/*********************************************************************
 *
 *	EDIT_MoveHome
 *
 *	Home key: move to beginning of line.
 *
 */
static void EDIT_MoveHome(WND *wndPtr, BOOL32 extend)
{
	INT32 s;
	INT32 e;
	INT32 l;
	INT32 li;

	EDIT_GetSel(wndPtr, &s, &e);
	l = (INT32)EDIT_EM_LineFromChar(wndPtr, e, 0);
	li = (INT32)EDIT_EM_LineIndex(wndPtr, l, 0);
	e = li;
	if (!extend)
		s = e;
	EDIT_SetSel(wndPtr, s, e);
	EDIT_EM_ScrollCaret(wndPtr, 0, 0);
}


/*********************************************************************
 *
 *	EDIT_MovePageDown
 *
 */
static void EDIT_MovePageDown(WND *wndPtr, BOOL32 extend)
{
	INT32 s;
	INT32 e;
	INT32 l;
	INT32 lc;
	INT32 li;
	INT32 x;

	EDIT_GetSel(wndPtr, &s, &e);
	l = (INT32)EDIT_EM_LineFromChar(wndPtr, e, 0);
	lc = (INT32)EDIT_EM_GetLineCount(wndPtr, e, 0);
	li = (INT32)EDIT_EM_LineIndex(wndPtr, l, 0);
	if (l < lc - 1) {
		x = EDIT_WndXFromCol(wndPtr, l, e - li);
		l = MIN(lc - 1, l + EDIT_GetVisibleLineCount(wndPtr));
		e = (INT32)EDIT_EM_LineIndex(wndPtr, l, 0) +
				EDIT_ColFromWndX(wndPtr, l, x);
	}
	if (!extend)
		s = e;
	EDIT_SetSel(wndPtr, s, e);
	EDIT_EM_ScrollCaret(wndPtr, 0, 0);
}


/*********************************************************************
 *
 *	EDIT_MovePageUp
 *
 */
static void EDIT_MovePageUp(WND *wndPtr, BOOL32 extend)
{
	INT32 s;
	INT32 e;
	INT32 l;
	INT32 li;
	INT32 x;

	EDIT_GetSel(wndPtr, &s, &e);
	l = (INT32)EDIT_EM_LineFromChar(wndPtr, e, 0);
	li = (INT32)EDIT_EM_LineIndex(wndPtr, l, 0);
	if (l) {
		x = EDIT_WndXFromCol(wndPtr, l, e - li);
		l = MAX(0, l - EDIT_GetVisibleLineCount(wndPtr));
		e = (INT32)EDIT_EM_LineIndex(wndPtr, l, 0) +
				EDIT_ColFromWndX(wndPtr, l, x);
	}
	if (!extend)
		s = e;
	EDIT_SetSel(wndPtr, s, e);
	EDIT_EM_ScrollCaret(wndPtr, 0, 0);
}


/*********************************************************************
 *
 *	EDIT_MoveUpward
 *
 */
static void EDIT_MoveUpward(WND *wndPtr, BOOL32 extend)
{
	INT32 s;
	INT32 e;
	INT32 l;
	INT32 li;
	INT32 x;

	EDIT_GetSel(wndPtr, &s, &e);
	l = (INT32)EDIT_EM_LineFromChar(wndPtr, e, 0);
	li = (INT32)EDIT_EM_LineIndex(wndPtr, l, 0);
	if (l) {
		x = EDIT_WndXFromCol(wndPtr, l, e - li);
		l--;
		e = (INT32)EDIT_EM_LineIndex(wndPtr, l, 0) +
				EDIT_ColFromWndX(wndPtr, l, x);
	}
	if (!extend)
		s = e;
	EDIT_SetSel(wndPtr, s, e);
	EDIT_EM_ScrollCaret(wndPtr, 0, 0);
}


/*********************************************************************
 *
 *	EDIT_MoveWordBackward
 *
 */
static void EDIT_MoveWordBackward(WND *wndPtr, BOOL32 extend)
{
	INT32 s;
	INT32 e;
	INT32 l;
	INT32 ll;
	INT32 li;
	LPSTR text;

	EDIT_GetSel(wndPtr, &s, &e);
	l = (INT32)EDIT_EM_LineFromChar(wndPtr, e, 0);
	ll = (INT32)EDIT_EM_LineLength(wndPtr, e, 0);
	li = (INT32)EDIT_EM_LineIndex(wndPtr, l, 0);
	if (e - li == 0) {
		if (l) {
			li = (INT32)EDIT_EM_LineIndex(wndPtr, l - 1, 0);
			e = li + (INT32)EDIT_EM_LineLength(wndPtr, li, 0);
		}
	} else {
		text = EDIT_GetPointer(wndPtr);
		e = li + (INT32)EDIT_CallWordBreakProc(wndPtr,
				text + li, e - li, ll, WB_LEFT);
	}
	if (!extend)
		s = e;
	EDIT_SetSel(wndPtr, s, e);
	EDIT_EM_ScrollCaret(wndPtr, 0, 0);
}


/*********************************************************************
 *
 *	EDIT_MoveWordForward
 *
 */
static void EDIT_MoveWordForward(WND *wndPtr, BOOL32 extend)
{
	INT32 s;
	INT32 e;
	INT32 l;
	INT32 lc;
	INT32 ll;
	INT32 li;
	LPSTR text;

	EDIT_GetSel(wndPtr, &s, &e);
	l = (INT32)EDIT_EM_LineFromChar(wndPtr, e, 0);
	lc = (INT32)EDIT_EM_GetLineCount(wndPtr, e, 0);
	ll = (INT32)EDIT_EM_LineLength(wndPtr, e, 0);
	li = (INT32)EDIT_EM_LineIndex(wndPtr, l, 0);
	if (e - li == ll) {
		if (l != lc - 1)
			e = (INT32)EDIT_EM_LineIndex(wndPtr, l + 1, 0);
	} else {
		text = EDIT_GetPointer(wndPtr);
		e = li + EDIT_CallWordBreakProc(wndPtr,
				text + li, e - li + 1, ll, WB_RIGHT);
	}
	if (!extend)
		s = e;
	EDIT_SetSel(wndPtr, s, e);
	EDIT_EM_ScrollCaret(wndPtr, 0, 0);
}


/*********************************************************************
 *
 *	EDIT_PaintLine
 *
 */
static void EDIT_PaintLine(WND *wndPtr, HDC32 hdc, INT32 line, BOOL32 rev)
{
	INT32 fv = (INT32)EDIT_EM_GetFirstVisibleLine(wndPtr, 0, 0);
	INT32 vlc = EDIT_GetVisibleLineCount(wndPtr);
	INT32 lc = (INT32)EDIT_EM_GetLineCount(wndPtr, 0, 0);
	INT32 li;
	INT32 ll;
	INT32 s;
	INT32 e;
	INT32 x;
	INT32 y;

	if ((line < fv) || (line > fv + vlc) || (line >= lc))
		return;

	dprintf_edit(stddeb, "edit: EDIT_PaintLine: line=%d\n", line);

	x = EDIT_WndXFromCol(wndPtr, line, 0);
	y = EDIT_WndYFromLine(wndPtr, line);
	li = (INT32)EDIT_EM_LineIndex(wndPtr, line, 0);
	ll = (INT32)EDIT_EM_LineLength(wndPtr, li, 0);
	EDIT_GetSel(wndPtr, &s, &e);
	ORDER_INT32(s, e);
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
static INT32 EDIT_PaintText(WND *wndPtr, HDC32 hdc, INT32 x, INT32 y, INT32 line, INT32 col, INT32 count, BOOL32 rev)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	COLORREF BkColor;
	COLORREF TextColor;
	INT32 ret;
	LPSTR text;
	INT32 li;
	INT32 xoff;

	if (!count)
		return 0;
	BkColor = GetBkColor32(hdc);
	TextColor = GetTextColor32(hdc);
	if (rev) {
		SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
		SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
	}
	text = EDIT_GetPasswordPointer(wndPtr);
	li = (INT32)EDIT_EM_LineIndex(wndPtr, line, 0);
	xoff = EDIT_GetXOffset(wndPtr);
	ret = (INT32)LOWORD(TabbedTextOut(hdc, x, y, text + li + col, count,
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
	if (es->text && (es->hBuf32 || es->hBuf16)) {
		if (es->hBuf32)
			LocalUnlock32(es->hBuf32);
		else
			LOCAL_Unlock(wndPtr->hInstance, es->hBuf16);
	}
	es->text = NULL;
}


/*********************************************************************
 *
 *	EDIT_ReleaseUndoPointer
 *
 *	This is the only helper function that can be called with es = NULL.
 *	It is called at the end of EditWndProc() to unlock the buffer.
 *
 */
static void EDIT_ReleaseUndoPointer(WND *wndPtr)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	if (!es)
		return;
	if (es->UndoText && es->hUndoBuf)
		LocalUnlock32(es->hUndoBuf);
	es->UndoText = NULL;
}


/*********************************************************************
 *
 *	EDIT_SetSel
 *
 *	Beware: This is not the function called on EM_SETSEL.
 *		This is the unordered version used internally
 *		(s can be > e).  Doesn't accept -1 parameters either.
 *		No range checking.
 *
 */
static void EDIT_SetSel(WND *wndPtr, INT32 ns, INT32 ne)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	LRESULT pos;
	INT32 s;
	INT32 e;

	EDIT_EM_GetSel(wndPtr, (WPARAM32)&s, (LPARAM)&e);
	es->SelStart = ns;
	es->SelEnd = ne;
	if (EDIT_GetRedraw(wndPtr)) {
		if (wndPtr->hwndSelf == GetFocus32()) {
			pos = EDIT_EM_PosFromChar(wndPtr, ne, 0);
			SetCaretPos((INT16)LOWORD(pos), (INT16)HIWORD(pos));
		}
		ORDER_INT32(s, ns);
		ORDER_INT32(s, ne);
		ORDER_INT32(e, ns);
		ORDER_INT32(e, ne);
		ORDER_INT32(ns, ne);
		if (e != ns) {
			EDIT_InvalidateText(wndPtr, s, e);
			EDIT_InvalidateText(wndPtr, ns, ne);
		} else
			EDIT_InvalidateText(wndPtr, s, ne);
	}
}


/*********************************************************************
 *
 *	EDIT_WndXFromCol
 *
 *	Calculates, for a given line and column, the X-coordinate on the screen.
 *
 */
static INT32 EDIT_WndXFromCol(WND *wndPtr, INT32 line, INT32 col)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	LPSTR text = EDIT_GetPasswordPointer(wndPtr);
	INT32 ret;
	HDC32 hdc;
	HFONT32 hFont;
	HFONT32 oldFont = 0;
	INT32 lc = (INT32)EDIT_EM_GetLineCount(wndPtr, 0, 0);
	INT32 li = (INT32)EDIT_EM_LineIndex(wndPtr, line, 0);
	INT32 ll = (INT32)EDIT_EM_LineLength(wndPtr, li, 0);
	INT32 xoff = EDIT_GetXOffset(wndPtr);

	hdc = GetDC32(wndPtr->hwndSelf);
	hFont = (HFONT32)EDIT_WM_GetFont(wndPtr, 0, 0);
	if (hFont) oldFont = SelectObject32(hdc, hFont);
	line = MAX(0, MIN(line, lc - 1));
	col = MIN(col, ll);
	ret = (INT32)LOWORD(GetTabbedTextExtent(hdc,
			text + li, col,
			es->NumTabStops, es->TabStops)) - xoff;
	if (hFont) SelectObject32(hdc, oldFont);
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
static INT32 EDIT_WndYFromLine(WND *wndPtr, INT32 line)
{
	INT32 fv = (INT32)EDIT_EM_GetFirstVisibleLine(wndPtr, 0, 0);
	INT32 lh = EDIT_GetLineHeight(wndPtr);

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
static INT32 EDIT_WordBreakProc(LPSTR s, INT32 index, INT32 count, INT32 action)
{
	INT32 ret = 0;

	dprintf_edit(stddeb, "edit: EDIT_WordBreakProc: s=%p, index=%u"
			", count=%u, action=%d\n", s, index, count, action);

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
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	return (LRESULT)(es->UndoInsertLen || lstrlen32A(EDIT_GetUndoPointer(wndPtr)));
}


/*********************************************************************
 *
 *	EM_CHARFROMPOS
 *
 *	FIXME: do the specs mean LineIndex or LineNumber (li v.s. l) ???
 */
static LRESULT EDIT_EM_CharFromPos(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	POINT32 pt;
	RECT32 rc;
	INT32 l;
	INT32 li;
	INT32 c;

	pt.x = LOWORD(lParam);
	pt.y = HIWORD(lParam);
	GetClientRect32(wndPtr->hwndSelf, &rc);

	if (!PtInRect32(&rc, pt))
		return -1;

	l = EDIT_LineFromWndY(wndPtr, pt.y);
	li = EDIT_EM_LineIndex(wndPtr, l, 0);
	c = EDIT_ColFromWndX(wndPtr, l, pt.x);

	return (LRESULT)MAKELONG(li + c, li);
}


/*********************************************************************
 *
 *	EM_EMPTYUNDOBUFFER
 *
 */
static LRESULT EDIT_EM_EmptyUndoBuffer(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	es->UndoInsertLen = 0;
	*EDIT_GetUndoPointer(wndPtr) = '\0';
	return 0;
}


/*********************************************************************
 *
 *	EM_FMTLINES
 *
 */
static LRESULT EDIT_EM_FmtLines(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	fprintf(stdnimp, "edit: EM_FMTLINES: message not implemented\n");
	return wParam ? TRUE : FALSE;
}


/*********************************************************************
 *
 *	EM_GETFIRSTVISIBLELINE
 *
 */
static LRESULT EDIT_EM_GetFirstVisibleLine(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	if (IsMultiLine(wndPtr))
		return (LRESULT)es->FirstVisibleLine;
	else
		return (LRESULT)EDIT_ColFromWndX(wndPtr, 0, 0);
}


/*********************************************************************
 *
 *	EM_GETHANDLE
 *
 */
static LRESULT EDIT_EM_GetHandle(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	if (!IsMultiLine(wndPtr))
		return 0;

	if (es->hBuf32)
		return (LRESULT)es->hBuf32;
	else
		return (LRESULT)es->hBuf16;
}


/*********************************************************************
 *
 *	EM_GETHANDLE16
 *
 *	Hopefully this won't fire back at us.
 *	We always start with a buffer in 32 bit linear memory.
 *	However, with this message a 16 bit application requests
 *	a handle of 16 bit local heap memory, where it expects to find
 *	the text.
 *	It's a pitty that from this moment on we have to use this
 *	local heap, because applications may rely on the handle
 *	in the future.
 *
 *	In this function we'll try to switch to local heap.
 */
static LRESULT EDIT_EM_GetHandle16(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	LPSTR text;
	HLOCAL16 newBuf;
	LPSTR newText;
	INT16 newSize;

	if (!IsMultiLine(wndPtr))
		return 0;

	if (es->hBuf16)
		return (LRESULT)es->hBuf16;

	if (!LOCAL_HeapSize(wndPtr->hInstance)) {
		if (!LocalInit(wndPtr->hInstance, 0,
			       GlobalSize16(wndPtr->hInstance))) {
			fprintf(stderr, "edit: EM_GETHANDLE: could not initialize local heap\n");
			return 0;
		}
		dprintf_edit(stddeb, "edit: EM_GETHANDLE: local heap initialized\n");
	}
	if (!(newBuf = LOCAL_Alloc(wndPtr->hInstance,
				EDIT_WM_GetTextLength(wndPtr, 0, 0) + 1,
				LMEM_MOVEABLE))) {
		fprintf(stderr, "edit: EM_GETHANDLE: could not allocate new 16 bit buffer\n");
		return 0;
	}
	newSize = MIN(LOCAL_Size(wndPtr->hInstance, newBuf) - 1, es->BufLimit);
	if (!(newText = LOCAL_Lock(wndPtr->hInstance, newBuf))) {
		fprintf(stderr, "edit: EM_GETHANDLE: could not lock new 16 bit buffer\n");
		LOCAL_Free(wndPtr->hInstance, newBuf);
		return 0;
	}
	text = EDIT_GetPointer(wndPtr);
	lstrcpy32A(newText, text);
	EDIT_ReleasePointer(wndPtr);
	GlobalFree32(es->hBuf32);
	es->hBuf32 = (HLOCAL32)NULL;
	es->hBuf16 = newBuf;
	es->BufSize = newSize;
	es->text = newText;
	dprintf_edit(stddeb, "edit: EM_GETHANDLE: switched to 16 bit buffer\n");

	return (LRESULT)es->hBuf16;
}


/*********************************************************************
 *
 *	EM_GETLIMITTEXT
 *
 */
static LRESULT EDIT_EM_GetLimitText(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	return es->BufLimit;
}


/*********************************************************************
 *
 *	EM_GETLINE
 *
 */
static LRESULT EDIT_EM_GetLine(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	LPSTR text;
	LPSTR src;
	LPSTR dst;
	INT32 len;
	INT32 i;
	INT32 lc = (INT32)EDIT_EM_GetLineCount(wndPtr, 0, 0);

	if (!IsMultiLine(wndPtr))
		wParam = 0;
	if ((INT32)wParam >= lc)
		return 0;
	text = EDIT_GetPointer(wndPtr);
	src = text + (INT32)EDIT_EM_LineIndex(wndPtr, wParam, 0);
	dst = (LPSTR)lParam;
	len = MIN(*(WORD *)dst, (INT32)EDIT_EM_LineLength(wndPtr, wParam, 0));
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
 *	EM_GETMARGINS
 *
 */
static LRESULT EDIT_EM_GetMargins(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	return (LRESULT)MAKELONG(es->LeftMargin, es->RightMargin);
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
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	CopyRect32((LPRECT32)lParam, &es->FormatRect);
	return 0;
}


/*********************************************************************
 *
 *	EM_GETRECT16
 *
 */
static LRESULT EDIT_EM_GetRect16(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	CONV_RECT32TO16(&es->FormatRect, (LPRECT16)PTR_SEG_TO_LIN((SEGPTR)lParam));
	return 0;
}


/*********************************************************************
 *
 *	EM_GETSEL
 *
 *	Returns the ordered selection range so that
 *	LOWORD(result) < HIWORD(result)
 *
 */
static LRESULT EDIT_EM_GetSel(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	INT32 s;
	INT32 e;

	EDIT_GetSel(wndPtr, &s, &e);
	ORDER_INT32(s, e);
	if (wParam)
		*(LPINT32)wParam = s;
	if (lParam)
		*(LPINT32)lParam = e;
	return MAKELONG((INT16)s, (INT16)e);
}


/*********************************************************************
 *
 *	EM_GETTHUMB
 *
 *	FIXME: is this right ?  (or should it be only HSCROLL)
 *	(and maybe only for edit controls that really have their
 *	own scrollbars) (and maybe only for multiline controls ?)
 *	All in all: very poorly documented
 *
 */
static LRESULT EDIT_EM_GetThumb(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	return MAKELONG(EDIT_WM_VScroll(wndPtr, EM_GETTHUMB16, 0),
		EDIT_WM_HScroll(wndPtr, EM_GETTHUMB16, 0));
}


/*********************************************************************
 *
 *	EM_GETWORDBREAKPROC
 *
 *	FIXME: Application defined WordBreakProc should be returned
 *
 */
static LRESULT EDIT_EM_GetWordBreakProc(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
/*
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	return (LRESULT)es->WordBreakProc;
*/
	return 0;
}


/*********************************************************************
 *
 *	EM_LINEFROMCHAR
 *
 */
static LRESULT EDIT_EM_LineFromChar(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	INT32 l;

	if (!IsMultiLine(wndPtr))
		return 0;
	if ((INT32)wParam == -1)
		EDIT_EM_GetSel(wndPtr, (WPARAM32)&wParam, 0);	/* intentional (looks weird, doesn't it ?) */
	l = (INT32)EDIT_EM_GetLineCount(wndPtr, 0, 0) - 1;
	while ((INT32)EDIT_EM_LineIndex(wndPtr, l, 0) > (INT32)wParam)
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
	INT32 e;
	INT32 l;
	INT32 lc = (INT32)EDIT_EM_GetLineCount(wndPtr, 0, 0);

	if ((INT32)wParam == -1) {
		EDIT_GetSel(wndPtr, NULL, &e);
		l = lc - 1;
		while (es->LineDefs[l].offset > e)
			l--;
		return (LRESULT)es->LineDefs[l].offset;
	}
	if ((INT32)wParam >= lc)
		return -1;
	return (LRESULT)es->LineDefs[(INT32)wParam].offset;
}


/*********************************************************************
 *
 *	EM_LINELENGTH
 *
 */
static LRESULT EDIT_EM_LineLength(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	INT32 s;
	INT32 e;
	INT32 sl;
	INT32 el;

	if (!IsMultiLine(wndPtr))
		return (LRESULT)es->LineDefs[0].length;
	if ((INT32)wParam == -1) {
		EDIT_GetSel(wndPtr, &s, &e);
		sl = (INT32)EDIT_EM_LineFromChar(wndPtr, s, 0);
		el = (INT32)EDIT_EM_LineFromChar(wndPtr, e, 0);
		return (LRESULT)(s - es->LineDefs[sl].offset +
				es->LineDefs[el].offset +
				es->LineDefs[el].length - e);
	}
	return (LRESULT)es->LineDefs[(INT32)EDIT_EM_LineFromChar(wndPtr, wParam, 0)].length;
}


/*********************************************************************
 *
 *	EM_LINESCROLL
 *
 *	FIXME: is wParam in pixels or in average character widths ???
 *	FIXME: we use this internally to scroll single line controls as well
 *	(specs are vague about whether this message is valid or not for
 *	single line controls)
 *
 */
static LRESULT EDIT_EM_LineScroll(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	INT32 lc = (INT32)EDIT_EM_GetLineCount(wndPtr, 0, 0);
	INT32 fv = (INT32)EDIT_EM_GetFirstVisibleLine(wndPtr, 0, 0);
	INT32 nfv = MAX(0, fv + (INT32)lParam);
	INT32 xoff = EDIT_GetXOffset(wndPtr);
	INT32 nxoff = MAX(0, xoff + (INT32)wParam);
	INT32 tw = EDIT_GetTextWidth(wndPtr);
	INT32 dx;
	INT32 dy;

	if (nfv >= lc)
		nfv = lc - 1;

	if (nxoff >= tw)
		nxoff = tw;
	dx = xoff - nxoff;
	dy = EDIT_WndYFromLine(wndPtr, fv) - EDIT_WndYFromLine(wndPtr, nfv);
	if (dx || dy) {
		if (EDIT_GetRedraw(wndPtr))
			ScrollWindow32(wndPtr->hwndSelf, dx, dy, NULL, NULL);
		es->FirstVisibleLine = nfv;
		es->XOffset = nxoff;
		if (IsVScrollBar(wndPtr))
			SetScrollPos32(wndPtr->hwndSelf, SB_VERT,
				EDIT_WM_VScroll(wndPtr, EM_GETTHUMB16, 0), TRUE);
		if (IsHScrollBar(wndPtr))
			SetScrollPos32(wndPtr->hwndSelf, SB_HORZ,
				EDIT_WM_HScroll(wndPtr, EM_GETTHUMB16, 0), TRUE);
	}
	if (IsMultiLine(wndPtr))
		return TRUE;
	else
		return FALSE;
}


/*********************************************************************
 *
 *	EM_POSFROMCHAR
 *
 */
static LRESULT EDIT_EM_PosFromChar(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	INT32 len = (INT32)EDIT_WM_GetTextLength(wndPtr, 0, 0);
	INT32 l;
	INT32 li;

	wParam = MIN(wParam, len);
	l = EDIT_EM_LineFromChar(wndPtr, wParam, 0);
	li = EDIT_EM_LineIndex(wndPtr, l, 0);
	return (LRESULT)MAKELONG(EDIT_WndXFromCol(wndPtr, l, wParam - li),
				EDIT_WndYFromLine(wndPtr, l));
}


/*********************************************************************
 *
 *	EM_REPLACESEL
 *
 */
static LRESULT EDIT_EM_ReplaceSel(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	LPCSTR str = (LPCSTR)lParam;
	INT32 strl = lstrlen32A(str);
	INT32 tl = (INT32)EDIT_WM_GetTextLength(wndPtr, 0, 0);
	INT32 utl;
	INT32 s;
	INT32 e;
	INT32 i;
	LPSTR p;
	LPSTR text;
	LPSTR utext;
	BOOL32 redraw;

	EDIT_EM_GetSel(wndPtr, (WPARAM32)&s, (LPARAM)&e);

	if ((s == e) && !strl)
		return 0;

	if (!EDIT_MakeFit(wndPtr, tl - (e - s) + strl))
		return 0;

	text = EDIT_GetPointer(wndPtr);
	utext = EDIT_GetUndoPointer(wndPtr);
	if (e != s) {
		/* there is something to be deleted */
		if ((BOOL32)wParam) {
			/* we have to be able to undo */
			utl = lstrlen32A(utext);
			if (!es->UndoInsertLen && (*utext && (s == es->UndoPos))) {
				/* undo-buffer is extended to the right */
				EDIT_MakeUndoFit(wndPtr, utl + e - s);
				lstrcpyn32A(utext + utl, text + s, e - s + 1);
			} else if (!es->UndoInsertLen && (*utext && (e == es->UndoPos))) {
				/* undo-buffer is extended to the left */
				EDIT_MakeUndoFit(wndPtr, utl + e - s);
				for (p = utext + utl ; p >= utext ; p--)
					p[e - s] = p[0];
				for (i = 0 , p = utext ; i < e - s ; i++)
					p[i] = (text + s)[i];
				es->UndoPos = s;
			} else {
				/* new undo-buffer */
				EDIT_MakeUndoFit(wndPtr, e - s);
				lstrcpyn32A(utext, text + s, e - s + 1);
				es->UndoPos = s;
			}
			/* any deletion makes the old insertion-undo invalid */
			es->UndoInsertLen = 0;
		} else
			EDIT_EM_EmptyUndoBuffer(wndPtr, 0, 0);

		/* now delete */
		lstrcpy32A(text + s, text + e);
	}
	if (strl) {
		/* there is an insertion */
		if ((BOOL32)wParam) {
			/* we have to be able to undo */
			if ((s == es->UndoPos) ||
					((es->UndoInsertLen) &&
					(s == es->UndoPos + es->UndoInsertLen)))
				/*
				 * insertion is new and at delete position or
				 * an extension to either left or right
				 */
				es->UndoInsertLen += strl;
			else {
				/* new insertion undo */
				es->UndoPos = s;
				es->UndoInsertLen = strl;
				/* new insertion makes old delete-buffer invalid */
				*utext = '\0';
			}
		} else
			EDIT_EM_EmptyUndoBuffer(wndPtr, 0, 0);

		/* now insert */
		tl = lstrlen32A(text);
		for (p = text + tl ; p >= text + s ; p--)
			p[strl] = p[0];
		for (i = 0 , p = text + s ; i < strl ; i++)
			p[i] = str[i];
		if(IsUpper(wndPtr))
			CharUpperBuff32A(p, strl);
		else if(IsLower(wndPtr))
			CharLowerBuff32A(p, strl);
		s += strl;
	}
	redraw = EDIT_GetRedraw(wndPtr);
	EDIT_WM_SetRedraw(wndPtr, FALSE, 0);
	EDIT_BuildLineDefs(wndPtr);
	EDIT_EM_SetSel(wndPtr, s, s);
	EDIT_EM_ScrollCaret(wndPtr, 0, 0);
	EDIT_EM_SetModify(wndPtr, TRUE, 0);
	dprintf_edit(stddeb, "edit: notification EN_UPDATE sent\n");
	EDIT_NOTIFY_PARENT(wndPtr, EN_UPDATE);
	EDIT_WM_SetRedraw(wndPtr, redraw, 0);
	if (redraw) {
		InvalidateRect32( wndPtr->hwndSelf, NULL, TRUE );
		dprintf_edit(stddeb, "edit: notification EN_CHANGE sent\n");
		EDIT_NOTIFY_PARENT(wndPtr, EN_CHANGE);
	}
	return 0;
}


/*********************************************************************
 *
 *	EM_SCROLL
 *
 *	FIXME: Scroll what ???  And where ???
 *
 */
static LRESULT EDIT_EM_Scroll(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	fprintf(stdnimp, "edit: EM_SCROLL: message not implemented\n");
	return 0;
}


/*********************************************************************
 *
 *	EM_SCROLLCARET
 *
 *	Makes sure the caret is visible.
 *	FIXME: We use EM_LINESCROLL, but may we do that for single line
 *		controls ???
 *
 */
static LRESULT EDIT_EM_ScrollCaret(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	INT32 e;
	INT32 l;
	INT32 li;
	INT32 fv = (INT32)EDIT_EM_GetFirstVisibleLine(wndPtr, 0, 0);
	INT32 vlc = EDIT_GetVisibleLineCount(wndPtr);
	INT32 ww = EDIT_GetWndWidth(wndPtr);
	INT32 cw = EDIT_GetAveCharWidth(wndPtr);
	INT32 x;
	INT32 dy = 0;
	INT32 dx = 0;

	EDIT_GetSel(wndPtr, NULL, &e);
	l = (INT32)EDIT_EM_LineFromChar(wndPtr, e, 0);
	li = (INT32)EDIT_EM_LineIndex(wndPtr, l, 0);
	x = EDIT_WndXFromCol(wndPtr, l, e - li);
	if (l >= fv + vlc)
		dy = l - vlc + 1 - fv;
	if (l < fv)
		dy = l - fv;
	if (x < 0)
		dx = x - ww / HSCROLL_FRACTION / cw * cw;
	if (x > ww)
		dx = x - (HSCROLL_FRACTION - 1) * ww / HSCROLL_FRACTION / cw * cw;
	if (dy || dx) {
		EDIT_EM_LineScroll(wndPtr, dx, dy);
		if (dy) {
			dprintf_edit(stddeb, "edit: notification EN_VSCROLL sent\n");
			EDIT_NOTIFY_PARENT(wndPtr, EN_VSCROLL);
		}
		if (dx) {
			dprintf_edit(stddeb, "edit: notification EN_HSCROLL sent\n");
			EDIT_NOTIFY_PARENT(wndPtr, EN_HSCROLL);
		}
	}
	return TRUE;
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
		es->hBuf16 = (HLOCAL16)NULL;
		es->hBuf32 = (HLOCAL32)wParam;
		es->BufSize = LocalSize32(es->hBuf32) - 1;
		es->LineCount = 0;
		es->FirstVisibleLine = 0;
		es->SelStart = es->SelEnd = 0;
		EDIT_EM_EmptyUndoBuffer(wndPtr, 0, 0);
		EDIT_EM_SetModify(wndPtr, FALSE, 0);
		EDIT_BuildLineDefs(wndPtr);
		if (EDIT_GetRedraw(wndPtr))
			InvalidateRect32( wndPtr->hwndSelf, NULL, TRUE );
		EDIT_EM_ScrollCaret(wndPtr, 0, 0);
	}
	return 0;
}


/*********************************************************************
 *
 *	EM_SETHANDLE16
 *
 */
static LRESULT EDIT_EM_SetHandle16(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	if (IsMultiLine(wndPtr)) {
		EDIT_ReleasePointer(wndPtr);
		/*
		 *	old buffer is freed by caller
		 */
		es->hBuf16 = (HLOCAL16)wParam;
		es->hBuf32 = (HLOCAL32)NULL;
		es->BufSize = LOCAL_Size(wndPtr->hInstance, es->hBuf16) - 1;
		es->LineCount = 0;
		es->FirstVisibleLine = 0;
		es->SelStart = es->SelEnd = 0;
		EDIT_EM_EmptyUndoBuffer(wndPtr, 0, 0);
		EDIT_EM_SetModify(wndPtr, FALSE, 0);
		EDIT_BuildLineDefs(wndPtr);
		if (EDIT_GetRedraw(wndPtr))
			InvalidateRect32( wndPtr->hwndSelf, NULL, TRUE );
		EDIT_EM_ScrollCaret(wndPtr, 0, 0);
	}
	return 0;
}


/*********************************************************************
 *
 *	EM_SETLIMITTEXT
 *
 *	FIXME: in WinNT maxsize is 0x7FFFFFFF / 0xFFFFFFFF
 *	However, the windows version is not complied to yet in all of edit.c
 *
 */
static LRESULT EDIT_EM_SetLimitText(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	if (IsMultiLine(wndPtr)) {
		if (wParam)
			es->BufLimit = MIN((INT32)wParam, BUFLIMIT_MULTI);
		else
			es->BufLimit = BUFLIMIT_MULTI;
	} else {
		if (wParam)
			es->BufLimit = MIN((INT32)wParam, BUFLIMIT_SINGLE);
		else
			es->BufLimit = BUFLIMIT_SINGLE;
	}
	return 0;
}


/*********************************************************************
 *
 *	EM_SETMARGINS
 *
 *	FIXME: We let the margins be set, but we don't use them yet !?!
 *
 */
static LRESULT EDIT_EM_SetMargins(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	if (wParam & EC_USEFONTINFO) {
		if (IsMultiLine(wndPtr)) {
			/*
			 *	FIXME: do some GetABCCharWidth, or so
			 *		This is just preliminary
			 */
			es->LeftMargin = es->RightMargin = EDIT_GetAveCharWidth(wndPtr);
		} else
			es->LeftMargin = es->RightMargin = EDIT_GetAveCharWidth(wndPtr);
		return 0;
	}
	if (wParam & EC_LEFTMARGIN)
		es->LeftMargin = LOWORD(lParam);
	if (wParam & EC_RIGHTMARGIN)
		es->RightMargin = HIWORD(lParam);
	return 0;
}


/*********************************************************************
 *
 *	EM_SETMODIFY
 *
 */
static LRESULT EDIT_EM_SetModify(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	es->TextChanged = (BOOL32)wParam;
	return 0;
}


/*********************************************************************
 *
 *	EM_SETPASSWORDCHAR
 *
 *	FIXME: This imlementation is way too simple
 *
 */
static LRESULT EDIT_EM_SetPasswordChar(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	es->PasswordChar = (CHAR)wParam;
	return 0;
}


/*********************************************************************
 *
 *	EM_SETREADONLY
 *
 */
static LRESULT EDIT_EM_SetReadOnly(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	if ((BOOL32)wParam)
		wndPtr->dwStyle |= ES_READONLY;
	else
		wndPtr->dwStyle &= ~(DWORD)ES_READONLY;
	return TRUE;
}


/*********************************************************************
 *
 *	EM_SETRECT
 *
 */
static LRESULT EDIT_EM_SetRect(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	fprintf(stdnimp,"edit: EM_SETRECT: message not implemented\n");
	return 0;
}


/*********************************************************************
 *
 *	EM_SETRECTNP
 *
 */
static LRESULT EDIT_EM_SetRectNP(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	fprintf(stdnimp,"edit: EM_SETRECTNP: message not implemented\n");
	return 0;
}


/*********************************************************************
 *
 *	EM_SETSEL
 *
 */
static LRESULT EDIT_EM_SetSel(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	INT32 ns = (INT32)wParam;
	INT32 ne = (INT32)lParam;
	INT32 tl = (INT32)EDIT_WM_GetTextLength(wndPtr, 0, 0);

	if (ns == -1) {
		EDIT_GetSel(wndPtr, NULL, &ne);
		ns = ne;
	} else if ((!ns) && (ne == -1))
		ne = tl;
	else {
		ns = MAX(0, MIN(ns, tl));
		ne = MAX(0, MIN(ne, tl));
		ORDER_INT32(ns, ne);
	}
	EDIT_SetSel(wndPtr, ns, ne);
	return -1;
}


/*********************************************************************
 *
 *	EM_SETSEL16
 *
 */
static LRESULT EDIT_EM_SetSel16(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	INT32 ns = (INT32)LOWORD(lParam);
	INT32 ne = (INT32)HIWORD(lParam);

	if ((INT16)LOWORD(lParam) == -1)
		ns = -1;
	if ((!ns) && ((INT16)HIWORD(lParam) == -1))
		ne = -1;
	EDIT_EM_SetSel(wndPtr, ns, ne);
	if (!wParam)
		EDIT_EM_ScrollCaret(wndPtr, 0, 0);
	return -1;
}


/*********************************************************************
 *
 *	EM_SETTABSTOPS
 *
 */
static LRESULT EDIT_EM_SetTabStops(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	INT32 i;

	if (!IsMultiLine(wndPtr))
		return FALSE;
	if (es->TabStops)
		free(es->TabStops);
	es->NumTabStops = (INT32)wParam;
	if (!wParam)
		es->TabStops = NULL;
	else {
		es->TabStops = (LPINT16)xmalloc(wParam * sizeof(INT16));
		for ( i = 0 ; i < (INT32)wParam ; i++ )
			es->TabStops[i] = (INT16)((LPINT32)lParam)[i];
	}
	return TRUE;
}


/*********************************************************************
 *
 *	EM_SETTABSTOPS16
 *
 */
static LRESULT EDIT_EM_SetTabStops16(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	if (!IsMultiLine(wndPtr))
		return FALSE;
	if (es->TabStops)
		free(es->TabStops);
	es->NumTabStops = (INT32)wParam;
	if (!wParam)
		es->TabStops = NULL;
	else {
		es->TabStops = (LPINT16)xmalloc(wParam * sizeof(INT16));
		memcpy(es->TabStops, (LPINT16)PTR_SEG_TO_LIN(lParam),
				(INT32)wParam * sizeof(INT16));
	}
	return TRUE;
}


/*********************************************************************
 *
 *	EM_SETWORDBREAKPROC
 *
 */
static LRESULT EDIT_EM_SetWordBreakProc(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
/*
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	es->WordBreakProc = (EDITWORDBREAKPROC)lParam;
*/
	return 0;
}


/*********************************************************************
 *
 *	EM_UNDO / WM_UNDO
 *
 */
static LRESULT EDIT_EM_Undo(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	LPSTR utext = xstrdup(EDIT_GetUndoPointer(wndPtr));

	dprintf_edit(stddeb, "edit: before UNDO:insertion length = %d, deletion buffer = %s\n",
			es->UndoInsertLen, utext);

	EDIT_EM_SetSel(wndPtr, es->UndoPos, es->UndoPos + es->UndoInsertLen);
	EDIT_EM_EmptyUndoBuffer(wndPtr, 0, 0);
	EDIT_EM_ReplaceSel(wndPtr, TRUE, (LPARAM)utext);
	EDIT_EM_SetSel(wndPtr, es->UndoPos, es->UndoPos + es->UndoInsertLen);
	free(utext);

	dprintf_edit(stddeb, "edit: after UNDO: insertion length = %d, deletion buffer = %s\n",
			es->UndoInsertLen, EDIT_GetUndoPointer(wndPtr));

	return TRUE;
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
				EDIT_EM_ReplaceSel(wndPtr, (WPARAM32)TRUE, (LPARAM)"\r\n");
		}
		break;
	case '\t':
		if (IsMultiLine(wndPtr) && !IsReadOnly(wndPtr))
			EDIT_EM_ReplaceSel(wndPtr, (WPARAM32)TRUE, (LPARAM)"\t");
		break;
	default:
		if (!IsReadOnly(wndPtr) && (c >= ' ') && (c != 127)) {
 			str[0] = c;
 			str[1] = '\0';
 			EDIT_EM_ReplaceSel(wndPtr, (WPARAM32)TRUE, (LPARAM)str);
 		}
		break;
	}
	return 0;
}


/*********************************************************************
 *
 *	WM_CLEAR
 *
 */
static LRESULT EDIT_WM_Clear(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDIT_EM_ReplaceSel(wndPtr, TRUE, (LPARAM)"");

	return -1;
}


/*********************************************************************
 *
 *	WM_COMMAND
 *
 */
static LRESULT EDIT_WM_Command(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	if (HIWORD(wParam))
		return 0;

	switch (LOWORD(wParam)) {
		case EM_UNDO32:	
			EDIT_EM_Undo(wndPtr, 0, 0);
			break;
		case WM_CUT:
			EDIT_WM_Cut(wndPtr, 0, 0);
			break;
		case WM_COPY:
			EDIT_WM_Copy(wndPtr, 0, 0);
			break;
		case WM_PASTE:
			EDIT_WM_Paste(wndPtr, 0, 0);
			break;
		case WM_CLEAR:
			EDIT_WM_Clear(wndPtr, 0, 0);
			break;
		case EM_SETSEL32:
			EDIT_EM_SetSel(wndPtr, 0, -1);
			EDIT_EM_ScrollCaret(wndPtr, 0, 0);
			break;
		default:
			dprintf_edit(stddeb, "edit: unknown menu item, please report\n");
			break;
	}
	return -1;
}


/*********************************************************************
 *
 *	WM_CONTEXTMENU
 *
 *	Note: the resource files resource/sysres_??.rc cannot define a
 *		single popup menu.  Hence we use a (dummy) menubar
 *		containing the single popup menu as its first item.
 *
 */
static LRESULT EDIT_WM_ContextMenu(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	HMENU32 hMenu = LoadMenuIndirect32A(SYSRES_GetResPtr(SYSRES_MENU_EDITMENU));
	HMENU32 hPopup = GetSubMenu32(hMenu, 0);

	TrackPopupMenu32(hPopup, TPM_LEFTALIGN | TPM_RIGHTBUTTON, LOWORD(lParam),
			HIWORD(lParam), 0, wndPtr->hwndSelf, NULL);
	DestroyMenu32(hMenu);
	return 0;
}


/*********************************************************************
 *
 *	WM_COPY
 *
 */
static LRESULT EDIT_WM_Copy(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	INT32 s;
	INT32 e;
	HGLOBAL16 hdst;
	LPSTR text;
	LPSTR dst;

	EDIT_GetSel(wndPtr, &s, &e);
	if (e == s)
		return -1;
	ORDER_INT32(s, e);
	hdst = GlobalAlloc16(GMEM_MOVEABLE, (DWORD)(e - s + 1));
	dst = GlobalLock16(hdst);
	text = EDIT_GetPointer(wndPtr);
	lstrcpyn32A(dst, text + s, e - s + 1);
	GlobalUnlock16(hdst);
	OpenClipboard(wndPtr->hwndSelf);
	EmptyClipboard();
	SetClipboardData(CF_TEXT, hdst);
	CloseClipboard();
	return -1;
}


/*********************************************************************
 *
 *	WM_CREATE
 *
 */
static LRESULT EDIT_WM_Create(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	CREATESTRUCT32A *cs = (CREATESTRUCT32A *)lParam;
	EDITSTATE *es;
	LPSTR text;

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
	if (!(es->hBuf32 = LocalAlloc32(LMEM_MOVEABLE, es->BufSize + 1))) {
		fprintf(stderr, "edit: WM_CREATE: unable to allocate buffer\n");
		return -1;
	}
	if (!(es->hUndoBuf = LocalAlloc32(LMEM_MOVEABLE, es->BufSize + 1))) {
		fprintf(stderr, "edit: WM_CREATE: unable to allocate undo buffer\n");
		LocalFree32(es->hBuf32);
		es->hBuf32 = (HLOCAL32)NULL;
		return -1;
	}
	es->BufSize = LocalSize32(es->hBuf32) - 1;
	es->UndoBufSize = LocalSize32(es->hUndoBuf) - 1;
	EDIT_EM_EmptyUndoBuffer(wndPtr, 0, 0);
	text = EDIT_GetPointer(wndPtr);
	*text = '\0';
	EDIT_BuildLineDefs(wndPtr);
	EDIT_WM_SetFont(wndPtr, 0, 0);
	if (cs->lpszName && *(cs->lpszName) != '\0')
		EDIT_EM_ReplaceSel(wndPtr, (WPARAM32)FALSE, (LPARAM)cs->lpszName);
	EDIT_WM_SetRedraw(wndPtr, TRUE, 0);
	return 0;
}


/*********************************************************************
 *
 *	WM_CUT
 *
 */
static LRESULT EDIT_WM_Cut(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDIT_WM_Copy(wndPtr, 0, 0);
	EDIT_WM_Clear(wndPtr, 0, 0);
	return -1;
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
	EDIT_ReleaseUndoPointer(wndPtr);
	LocalFree32(es->hUndoBuf);
	EDIT_ReleasePointer(wndPtr);
	if (es->hBuf32)
		LocalFree32(es->hBuf32);
	else
		LOCAL_Free(wndPtr->hInstance, es->hBuf16);
	free(es);
	*(EDITSTATE **)&wndPtr->wExtra = NULL;
	return 0;
}


/*********************************************************************
 *
 *	WM_ENABLE
 *
 */
static LRESULT EDIT_WM_Enable(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDIT_InvalidateText(wndPtr, 0, -1);
	return 0;
}


/*********************************************************************
 *
 *	WM_ERASEBKGND
 *
 */
static LRESULT EDIT_WM_EraseBkGnd(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	HBRUSH32 hBrush;
	RECT32 rc;

	hBrush = (HBRUSH32)EDIT_SEND_CTLCOLOR(wndPtr, wParam);
	if (!hBrush) hBrush = (HBRUSH32)GetStockObject32(WHITE_BRUSH);

	GetClientRect32(wndPtr->hwndSelf, &rc);
	IntersectClipRect32((HDC32)wParam, rc.left, rc.top,
			     rc.right, rc.bottom);
	GetClipBox32((HDC32)wParam, &rc);
	/*
	 *	FIXME:	specs say that we should UnrealizeObject() the brush,
	 *		but the specs of UnrealizeObject() say that we shouldn't
	 *		unrealize a stock object.  The default brush that
	 *		DefWndProc() returns is ... a stock object.
	 */
	FillRect32((HDC32)wParam, &rc, hBrush);
	return -1;
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
	LPSTR text = EDIT_GetPointer(wndPtr);
	INT32 len;
	LRESULT lResult = 0;

	len = lstrlen32A(text);
	if ((INT32)wParam > len) {
		lstrcpy32A((LPSTR)lParam, text);
		lResult = (LRESULT)len + 1;
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
	LPSTR text = EDIT_GetPointer(wndPtr);

	return (LRESULT)lstrlen32A(text);
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
	INT32 ww = EDIT_GetWndWidth(wndPtr);
	INT32 tw = EDIT_GetTextWidth(wndPtr);
	INT32 cw = EDIT_GetAveCharWidth(wndPtr);
	INT32 xoff = EDIT_GetXOffset(wndPtr);
	INT32 dx = 0;
	BOOL32 not = TRUE;
	LRESULT ret = 0;

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
		dx = HIWORD(wParam) * tw / 100 - xoff;
		break;
	/* The next two are undocumented ! */
	case EM_GETTHUMB16:
		ret = tw ? xoff * 100 / tw : 0;
		break;
	case EM_LINESCROLL16:
		dx = (INT16)HIWORD(wParam);
		break;
	case SB_ENDSCROLL:
	default:
		break;
	}
	if (dx) {
		EDIT_EM_LineScroll(wndPtr, dx, 0);
		if (not) {
			dprintf_edit(stddeb, "edit: notification EN_HSCROLL sent\n");
			EDIT_NOTIFY_PARENT(wndPtr, EN_HSCROLL);
		}
	}
	return ret;
}


/*********************************************************************
 *
 *	WM_INITMENUPOPUP
 *
 *	FIXME: the message identifiers have been chosen arbitrarily,
 *		hence we use MF_BYPOSITION.
 *		We might as well use the "real" values (anybody knows ?)
 *		The menu definition is in resources/sysres_??.rc.
 *		Once these are OK, we better use MF_BYCOMMAND here
 *		(as we do in EDIT_WM_Command()).
 *
 */
static LRESULT EDIT_WM_InitMenuPopup(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	HMENU32 hPopup = (HMENU32)wParam;
	INT32 s;
	INT32 e;

	EDIT_EM_GetSel(wndPtr, (WPARAM32)&s, (LPARAM)&e);

	/* undo */
	EnableMenuItem32(hPopup, 0, MF_BYPOSITION |
		(EDIT_EM_CanUndo(wndPtr, 0, 0) ? MF_ENABLED : MF_GRAYED));
	/* cut */
	EnableMenuItem32(hPopup, 2, MF_BYPOSITION |
		((e - s) && !IsPassword(wndPtr) ? MF_ENABLED : MF_GRAYED));
	/* copy */
	EnableMenuItem32(hPopup, 3, MF_BYPOSITION |
		((e - s) && !IsPassword(wndPtr) ? MF_ENABLED : MF_GRAYED));
	/* paste */
	EnableMenuItem32(hPopup, 4, MF_BYPOSITION |
		(IsClipboardFormatAvailable(CF_TEXT) ? MF_ENABLED : MF_GRAYED));
	/* delete */
	EnableMenuItem32(hPopup, 5, MF_BYPOSITION |
		((e - s) ? MF_ENABLED : MF_GRAYED));
	/* select all */
	EnableMenuItem32(hPopup, 7, MF_BYPOSITION |
		(s || (e != EDIT_WM_GetTextLength(wndPtr, 0, 0)) ? MF_ENABLED : MF_GRAYED));

	return 0;
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
	INT32 s;
	INT32 e;
	BOOL32 shift;
	BOOL32 control;

	if (GetKeyState(VK_MENU) & 0x8000)
		return 0;

	shift = GetKeyState(VK_SHIFT) & 0x8000;
	control = GetKeyState(VK_CONTROL) & 0x8000;

	EDIT_GetSel(wndPtr, &s, &e);
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
				EDIT_WM_Clear(wndPtr, 0, 0);
			else
				EDIT_DelLeft(wndPtr);
		break;
	case VK_DELETE:
		if (!IsReadOnly(wndPtr) && !(shift && control))
			if (e != s) {
				if (shift)
					EDIT_WM_Cut(wndPtr, 0, 0);
				else
					EDIT_WM_Clear(wndPtr, 0, 0);
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
				EDIT_WM_Paste(wndPtr, 0, 0);
		} else if (control)
			EDIT_WM_Copy(wndPtr, 0, 0);
		break;
	}
	return 0;
}


/*********************************************************************
 *
 *	WM_KILLFOCUS
 *
 */
static LRESULT EDIT_WM_KillFocus(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	INT32 s;
	INT32 e;

	DestroyCaret();
	if(!(wndPtr->dwStyle & ES_NOHIDESEL)) {
		EDIT_EM_GetSel(wndPtr, (WPARAM32)&s, (LPARAM)&e);
		EDIT_InvalidateText(wndPtr, s, e);
	}
	dprintf_edit(stddeb, "edit: notification EN_KILLFOCUS sent\n");
	EDIT_NOTIFY_PARENT(wndPtr, EN_KILLFOCUS);
	return 0;
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
	INT32 s;
	INT32 e;
	INT32 l;
	INT32 li;
	INT32 ll;
	LPSTR text = EDIT_GetPointer(wndPtr);

	EDIT_GetSel(wndPtr, NULL, &e);
	l = (INT32)EDIT_EM_LineFromChar(wndPtr, e, 0);
	li = (INT32)EDIT_EM_LineIndex(wndPtr, l, 0);
	ll = (INT32)EDIT_EM_LineLength(wndPtr, e, 0);
	s = li + EDIT_CallWordBreakProc (wndPtr, text + li, e - li, ll, WB_LEFT);
	e = li + EDIT_CallWordBreakProc(wndPtr, text + li, e - li, ll, WB_RIGHT);
	EDIT_EM_SetSel(wndPtr, s, e);
	EDIT_EM_ScrollCaret(wndPtr, 0, 0);
	return 0;
}


/*********************************************************************
 *
 *	WM_LBUTTONDOWN
 *
 */
static LRESULT EDIT_WM_LButtonDown(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	INT32 x = (INT32)(INT16)LOWORD(lParam);
	INT32 y = (INT32)(INT16)HIWORD(lParam);
	INT32 l = EDIT_LineFromWndY(wndPtr, y);
	INT32 c;
	INT32 s;
	INT32 e;
	INT32 fv = (INT32)EDIT_EM_GetFirstVisibleLine(wndPtr, 0, 0);
	INT32 vlc = EDIT_GetVisibleLineCount(wndPtr);
	INT32 li;

	SetFocus32(wndPtr->hwndSelf);
	SetCapture32(wndPtr->hwndSelf);
	l = MIN(fv + vlc - 1, MAX(fv, l));
	x = MIN(EDIT_GetWndWidth(wndPtr), MAX(0, x));
	c = EDIT_ColFromWndX(wndPtr, l, x);
	li = (INT32)EDIT_EM_LineIndex(wndPtr, l, 0);
	e = li + c;
	if (GetKeyState(VK_SHIFT) & 0x8000)
		EDIT_GetSel(wndPtr, &s, NULL);
	else
		s = e;
	EDIT_SetSel(wndPtr, s, e);
	EDIT_EM_ScrollCaret(wndPtr, 0, 0);
	SetTimer32(wndPtr->hwndSelf, 0, 100, NULL);
	return 0;
}


/*********************************************************************
 *
 *	WM_LBUTTONUP
 *
 */
static LRESULT EDIT_WM_LButtonUp(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	if (GetCapture32() == wndPtr->hwndSelf) {
		KillTimer32(wndPtr->hwndSelf, 0);
		ReleaseCapture();
	}
	return 0;
}


/*********************************************************************
 *
 *	WM_MOUSEMOVE
 *
 */
static LRESULT EDIT_WM_MouseMove(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	INT32 x;
	INT32 y;
	INT32 l;
	INT32 c;
	INT32 s;
	INT32 fv;
	INT32 vlc;
	INT32 li;

	if (GetCapture32() == wndPtr->hwndSelf) {
		x = (INT32)(INT16)LOWORD(lParam);
		y = (INT32)(INT16)HIWORD(lParam);
		fv = (INT32)EDIT_EM_GetFirstVisibleLine(wndPtr, 0, 0);
		vlc = EDIT_GetVisibleLineCount(wndPtr);
		l = EDIT_LineFromWndY(wndPtr, y);
		l = MIN(fv + vlc - 1, MAX(fv, l));
		x = MIN(EDIT_GetWndWidth(wndPtr), MAX(0, x));
		c = EDIT_ColFromWndX(wndPtr, l, x);
		EDIT_GetSel(wndPtr, &s, NULL);
		li = (INT32)EDIT_EM_LineIndex(wndPtr, l, 0);
		EDIT_SetSel(wndPtr, s, li + c);
	}
	/*
	 *	FIXME: gotta do some scrolling if outside client (format ?)
	 *		area.  Maybe reset the timer ?
	 */
	return 0;
}


/*********************************************************************
 *
 *	WM_PAINT
 *
 */
static LRESULT EDIT_WM_Paint(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	PAINTSTRUCT32 ps;
	INT32 i;
	INT32 fv = (INT32)EDIT_EM_GetFirstVisibleLine(wndPtr, 0, 0);
	INT32 vlc = EDIT_GetVisibleLineCount(wndPtr);
	INT32 lc = (INT32)EDIT_EM_GetLineCount(wndPtr, 0, 0);
	HDC32 hdc;
	HFONT32 hFont;
	HFONT32 oldFont = 0;
	RECT32 rc;
	RECT32 rcLine;
	RECT32 rcRgn;
 	LRESULT pos;
 	INT32 e;
	BOOL32 rev = IsWindowEnabled32(wndPtr->hwndSelf) &&
				((GetFocus32() == wndPtr->hwndSelf) ||
					(wndPtr->dwStyle & ES_NOHIDESEL));

	hdc = BeginPaint32(wndPtr->hwndSelf, &ps);
	GetClientRect32(wndPtr->hwndSelf, &rc);
	IntersectClipRect32( hdc, rc.left, rc.top, rc.right, rc.bottom );
	hFont = (HFONT32)EDIT_WM_GetFont(wndPtr, 0, 0);
	if (hFont)
		oldFont = (HFONT32)SelectObject32(hdc, hFont);
	EDIT_SEND_CTLCOLOR(wndPtr, hdc);
	if (!IsWindowEnabled32(wndPtr->hwndSelf))
		SetTextColor(hdc, GetSysColor(COLOR_GRAYTEXT));
	GetClipBox32(hdc, &rcRgn);
	for (i = fv ; i <= MIN(fv + vlc, fv + lc - 1) ; i++ ) {
		EDIT_GetLineRect(wndPtr, i, 0, -1, &rcLine);
		if (IntersectRect32(&rc, &rcRgn, &rcLine))
			EDIT_PaintLine(wndPtr, hdc, i, rev);
	}
	if (hFont) SelectObject32(hdc, oldFont);
	if (wndPtr->hwndSelf == GetFocus32()) {
		EDIT_GetSel(wndPtr, NULL, &e);
		pos = EDIT_EM_PosFromChar(wndPtr, e, 0);
		SetCaretPos((INT16)LOWORD(pos), (INT16)HIWORD(pos));
	}
	EndPaint32(wndPtr->hwndSelf, &ps);
	return 0;
}


/*********************************************************************
 *
 *	WM_PASTE
 *
 */
static LRESULT EDIT_WM_Paste(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	HGLOBAL16 hsrc;
	LPSTR src;

	OpenClipboard(wndPtr->hwndSelf);
	if ((hsrc = GetClipboardData(CF_TEXT))) {
		src = (LPSTR)GlobalLock16(hsrc);
		EDIT_EM_ReplaceSel(wndPtr, (WPARAM32)TRUE, (LPARAM)src);
		GlobalUnlock16(hsrc);
	}
	CloseClipboard();
	return -1;
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
		return -1;
	} else
		return 0;
}


/*********************************************************************
 *
 *	WM_SETFOCUS
 *
 */
static LRESULT EDIT_WM_SetFocus(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	INT32 s;
	INT32 e;

	EDIT_GetSel(wndPtr, &s, &e);
	CreateCaret(wndPtr->hwndSelf, 0, 2, EDIT_GetLineHeight(wndPtr));
	EDIT_SetSel(wndPtr, s, e);
	if(!(wndPtr->dwStyle & ES_NOHIDESEL))
		EDIT_InvalidateText(wndPtr, s, e);
	ShowCaret(wndPtr->hwndSelf);
	dprintf_edit(stddeb, "edit: notification EN_SETFOCUS sent\n");
	EDIT_NOTIFY_PARENT(wndPtr, EN_SETFOCUS);
	return 0;
}


/*********************************************************************
 *
 *	WM_SETFONT
 *
 */
static LRESULT EDIT_WM_SetFont(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	TEXTMETRIC32A tm;
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	INT32 s;
	INT32 e;
	HDC32 hdc;
	HFONT32 oldFont = 0;

	EDIT_GetSel(wndPtr, &s, &e);
	es->hFont = (HFONT32)wParam;
	hdc = GetDC32(wndPtr->hwndSelf);
	if (es->hFont) oldFont = SelectObject32(hdc, es->hFont);
	GetTextMetrics32A(hdc, &tm);
	es->LineHeight = tm.tmHeight;
	es->AveCharWidth = tm.tmAveCharWidth;
	if (es->hFont) SelectObject32(hdc, oldFont);
	ReleaseDC32(wndPtr->hwndSelf, hdc);
	EDIT_BuildLineDefs(wndPtr);
	if ((BOOL32)lParam && EDIT_GetRedraw(wndPtr))
		InvalidateRect32( wndPtr->hwndSelf, NULL, TRUE );
	if (wndPtr->hwndSelf == GetFocus32()) {
		DestroyCaret();
		CreateCaret(wndPtr->hwndSelf, 0, 2, EDIT_GetLineHeight(wndPtr));
		EDIT_SetSel(wndPtr, s, e);
		ShowCaret(wndPtr->hwndSelf);
	}
	return 0;
}


/*********************************************************************
 *
 *	WM_SETREDRAW
 *
 */
static LRESULT EDIT_WM_SetRedraw(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);

	es->Redraw = (BOOL32)wParam;
	return 0;
}


/*********************************************************************
 *
 *	WM_SETTEXT
 *
 */
static LRESULT EDIT_WM_SetText(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDIT_EM_SetSel(wndPtr, 0, -1);
	if (lParam)
		EDIT_EM_ReplaceSel(wndPtr, (WPARAM32)FALSE, lParam);
	EDIT_EM_SetModify(wndPtr, TRUE, 0);
	EDIT_EM_ScrollCaret(wndPtr, 0, 0);
	return 1;
}


/*********************************************************************
 *
 *	WM_SIZE
 *
 *	FIXME: What about that FormatRect ???
 *
 */
static LRESULT EDIT_WM_Size(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	EDITSTATE *es = EDITSTATEPTR(wndPtr);
	INT32 e;
	
	EDIT_GetSel(wndPtr, 0, &e);
	GetClientRect32(wndPtr->hwndSelf, &es->FormatRect);
	if (EDIT_GetRedraw(wndPtr) &&
			((wParam == SIZE_MAXIMIZED) ||
				(wParam == SIZE_RESTORED))) {
		if (IsMultiLine(wndPtr) && IsWordWrap(wndPtr))
			EDIT_BuildLineDefs(wndPtr);
		InvalidateRect32( wndPtr->hwndSelf, NULL, TRUE );
	}
	return 0;
}


/*********************************************************************
 *
 *	WM_SYSKEYDOWN
 *
 */
static LRESULT EDIT_WM_SysKeyDown(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
	if ((wParam == VK_BACK) && (lParam & 0x2000) &&
			(BOOL32)EDIT_EM_CanUndo(wndPtr, 0, 0))
		EDIT_EM_Undo(wndPtr, 0, 0);
	return 0;
}


/*********************************************************************
 *
 *	WM_TIMER
 *
 */
static LRESULT EDIT_WM_Timer(WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
/*
 *	FIXME: gotta do some scrolling here, like
 *		EDIT_EM_LineScroll(wndPtr, 0, 1);
 */
	return 0;
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
	INT32 lc = (INT32)EDIT_EM_GetLineCount(wndPtr, 0, 0);
	INT32 fv = (INT32)EDIT_EM_GetFirstVisibleLine(wndPtr, 0, 0);
	INT32 vlc = EDIT_GetVisibleLineCount(wndPtr);
	INT32 dy = 0;
	BOOL32 not = TRUE;
	LRESULT ret = 0;

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
		not = FALSE;
		/* fall through */
	case SB_THUMBPOSITION:
		dy = HIWORD(wParam) * (lc - 1) / 100 - fv;
		break;
	/* The next two are undocumented ! */
	case EM_GETTHUMB16:
		ret = (lc > 1) ? MAKELONG(fv * 100 / (lc - 1), 0) : 0;
		break;
	case EM_LINESCROLL16:
		dy = (INT16)LOWORD(lParam);
		break;
	case SB_ENDSCROLL:
	default:
		break;
	}
	if (dy) {
		EDIT_EM_LineScroll(wndPtr, 0, dy);
		if (not) {
			dprintf_edit(stddeb, "edit: notification EN_VSCROLL sent\n");
			EDIT_NOTIFY_PARENT(wndPtr, EN_VSCROLL);
		}
	}
	return ret;
}
