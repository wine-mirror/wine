/*
 *	Edit control
 *
 *	Copyright  David W. Metcalfe, 1994
 *	Copyright  William Magro, 1995, 1996
 *	Copyright  Frans van Dorsselaer, 1996, 1997
 *
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 *	please read EDIT.TODO (and update it when you change things)
 */

#include "config.h"

#include <string.h>
#include <stdlib.h>

#include "winbase.h"
#include "winnt.h"
#include "win.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "wine/unicode.h"
#include "controls.h"
#include "local.h"
#include "user.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(edit);
WINE_DECLARE_DEBUG_CHANNEL(combo);
WINE_DECLARE_DEBUG_CHANNEL(relay);

#define BUFLIMIT_MULTI		65534	/* maximum buffer size (not including '\0')
					   FIXME: BTW, new specs say 65535 (do you dare ???) */
#define BUFLIMIT_SINGLE		32766	/* maximum buffer size (not including '\0') */
#define GROWLENGTH		32	/* buffers granularity in bytes: must be power of 2 */
#define ROUND_TO_GROW(size)	(((size) + (GROWLENGTH - 1)) & ~(GROWLENGTH - 1))
#define HSCROLL_FRACTION	3	/* scroll window by 1/3 width */

/*
 *	extra flags for EDITSTATE.flags field
 */
#define EF_MODIFIED		0x0001	/* text has been modified */
#define EF_FOCUSED		0x0002	/* we have input focus */
#define EF_UPDATE		0x0004	/* notify parent of changed state */
#define EF_VSCROLL_TRACK	0x0008	/* don't SetScrollPos() since we are tracking the thumb */
#define EF_HSCROLL_TRACK	0x0010	/* don't SetScrollPos() since we are tracking the thumb */
#define EF_AFTER_WRAP		0x0080	/* the caret is displayed after the last character of a
					   wrapped line, instead of in front of the next character */
#define EF_USE_SOFTBRK		0x0100	/* Enable soft breaks in text. */

typedef enum
{
	END_0 = 0,	/* line ends with terminating '\0' character */
	END_WRAP,	/* line is wrapped */
	END_HARD,	/* line ends with a hard return '\r\n' */
	END_SOFT	/* line ends with a soft return '\r\r\n' */
} LINE_END;

typedef struct tagLINEDEF {
	INT length;		/* bruto length of a line in bytes */
	INT net_length;	/* netto length of a line in visible characters */
	LINE_END ending;
	INT width;		/* width of the line in pixels */
	INT index; /* line index into the buffer */
	struct tagLINEDEF *next;
} LINEDEF;

typedef struct
{
	BOOL is_unicode;		/* how the control was created */
	LPWSTR text;			/* the actual contents of the control */
	UINT buffer_size;		/* the size of the buffer in characters */
	UINT buffer_limit;		/* the maximum size to which the buffer may grow in characters */
	HFONT font;			/* NULL means standard system font */
	INT x_offset;			/* scroll offset	for multi lines this is in pixels
								for single lines it's in characters */
	INT line_height;		/* height of a screen line in pixels */
	INT char_width;		/* average character width in pixels */
	DWORD style;			/* sane version of wnd->dwStyle */
	WORD flags;			/* flags that are not in es->style or wnd->flags (EF_XXX) */
	INT undo_insert_count;	/* number of characters inserted in sequence */
	UINT undo_position;		/* character index of the insertion and deletion */
	LPWSTR undo_text;		/* deleted text */
	UINT undo_buffer_size;		/* size of the deleted text buffer */
	INT selection_start;		/* == selection_end if no selection */
	INT selection_end;		/* == current caret position */
	WCHAR password_char;		/* == 0 if no password char, and for multi line controls */
	INT left_margin;		/* in pixels */
	INT right_margin;		/* in pixels */
	RECT format_rect;
	INT text_width;			/* width of the widest line in pixels for multi line controls
					   and just line width for single line controls	*/
	INT region_posx;		/* Position of cursor relative to region: */
	INT region_posy;		/* -1: to left, 0: within, 1: to right */
	EDITWORDBREAKPROC16 word_break_proc16;
	void *word_break_proc;		/* 32-bit word break proc: ANSI or Unicode */
	INT line_count;		/* number of lines */
	INT y_offset;			/* scroll offset in number of lines */
	BOOL bCaptureState; /* flag indicating whether mouse was captured */
	BOOL bEnableState;             /* flag keeping the enable state */
	HWND hwndParent;               /* Handle of parent for sending EN_* messages.
				          Even if parent will change, EN_* messages
					  should be sent to the first parent. */
	HWND hwndListBox;              /* handle of ComboBox's listbox or NULL */
	/*
	 *	only for multi line controls
	 */
	INT lock_count;		/* amount of re-entries in the EditWndProc */
	INT tabs_count;
	LPINT tabs;
	LINEDEF *first_line_def;	/* linked list of (soft) linebreaks */
	HLOCAL hloc32W;		/* our unicode local memory block */
	HLOCAL16 hloc16;	/* alias for 16-bit control receiving EM_GETHANDLE16
				   or EM_SETHANDLE16 */
	HLOCAL hloc32A;		/* alias for ANSI control receiving EM_GETHANDLE
				   or EM_SETHANDLE */
} EDITSTATE;


#define SWAP_INT32(x,y) do { INT temp = (INT)(x); (x) = (INT)(y); (y) = temp; } while(0)
#define ORDER_INT(x,y) do { if ((INT)(y) < (INT)(x)) SWAP_INT32((x),(y)); } while(0)

#define SWAP_UINT32(x,y) do { UINT temp = (UINT)(x); (x) = (UINT)(y); (y) = temp; } while(0)
#define ORDER_UINT(x,y) do { if ((UINT)(y) < (UINT)(x)) SWAP_UINT32((x),(y)); } while(0)

#define DPRINTF_EDIT_NOTIFY(hwnd, str) \
	do {TRACE("notification " str " sent to hwnd=%08x\n", \
		       (UINT)(hwnd));} while(0)

/* used for disabled or read-only edit control */
#define EDIT_SEND_CTLCOLORSTATIC(hwnd,hdc) \
	(SendMessageW(GetParent(hwnd), WM_CTLCOLORSTATIC, \
			(WPARAM)(hdc), (LPARAM)(hwnd)))
#define EDIT_SEND_CTLCOLOR(hwnd,hdc) \
	(SendMessageW(GetParent(hwnd), WM_CTLCOLOREDIT, \
			(WPARAM)(hdc), (LPARAM)(hwnd)))
#define EDIT_NOTIFY_PARENT(hwnd, es, wNotifyCode, str) \
	do \
	{ /* Notify parent which has created this edit control */ \
	    DPRINTF_EDIT_NOTIFY((es)->hwndParent, str); \
	    SendMessageW((es)->hwndParent, WM_COMMAND, \
		     MAKEWPARAM(GetWindowLongA((hwnd),GWL_ID), wNotifyCode), \
		     (LPARAM)(hwnd)); \
	} while(0)
#define DPRINTF_EDIT_MSG16(str) \
	TRACE(\
		     "16 bit : " str ": hwnd=%08x, wParam=%08x, lParam=%08x\n", \
		     hwnd, (UINT)wParam, (UINT)lParam)
#define DPRINTF_EDIT_MSG32(str) \
	TRACE(\
		     "32 bit %c : " str ": hwnd=%08x, wParam=%08x, lParam=%08x\n", \
		     unicode ? 'W' : 'A', \
		     hwnd, (UINT)wParam, (UINT)lParam)

/*********************************************************************
 *
 *	Declarations
 *
 */

/*
 *	These functions have trivial implementations
 *	We still like to call them internally
 *	"static inline" makes them more like macro's
 */
static inline BOOL	EDIT_EM_CanUndo(EDITSTATE *es);
static inline void	EDIT_EM_EmptyUndoBuffer(EDITSTATE *es);
static inline void	EDIT_WM_Clear(HWND hwnd, EDITSTATE *es);
static inline void	EDIT_WM_Cut(HWND hwnd, EDITSTATE *es);

/*
 *	Helper functions only valid for one type of control
 */
static void	EDIT_BuildLineDefs_ML(HWND hwnd, EDITSTATE *es, INT iStart, INT iEnd, INT delta, HRGN hrgn);
static void	EDIT_CalcLineWidth_SL(HWND hwnd, EDITSTATE *es);
static LPWSTR	EDIT_GetPasswordPointer_SL(EDITSTATE *es);
static void	EDIT_MoveDown_ML(HWND hwnd, EDITSTATE *es, BOOL extend);
static void	EDIT_MovePageDown_ML(HWND hwnd, EDITSTATE *es, BOOL extend);
static void	EDIT_MovePageUp_ML(HWND hwnd, EDITSTATE *es, BOOL extend);
static void	EDIT_MoveUp_ML(HWND hwnd, EDITSTATE *es, BOOL extend);
/*
 *	Helper functions valid for both single line _and_ multi line controls
 */
static INT	EDIT_CallWordBreakProc(EDITSTATE *es, INT start, INT index, INT count, INT action);
static INT	EDIT_CharFromPos(HWND hwnd, EDITSTATE *es, INT x, INT y, LPBOOL after_wrap);
static void	EDIT_ConfinePoint(EDITSTATE *es, LPINT x, LPINT y);
static void	EDIT_GetLineRect(HWND hwnd, EDITSTATE *es, INT line, INT scol, INT ecol, LPRECT rc);
static void	EDIT_InvalidateText(HWND hwnd, EDITSTATE *es, INT start, INT end);
static void	EDIT_LockBuffer(HWND hwnd, EDITSTATE *es);
static BOOL	EDIT_MakeFit(HWND hwnd, EDITSTATE *es, UINT size);
static BOOL	EDIT_MakeUndoFit(EDITSTATE *es, UINT size);
static void	EDIT_MoveBackward(HWND hwnd, EDITSTATE *es, BOOL extend);
static void	EDIT_MoveEnd(HWND hwnd, EDITSTATE *es, BOOL extend);
static void	EDIT_MoveForward(HWND hwnd, EDITSTATE *es, BOOL extend);
static void	EDIT_MoveHome(HWND hwnd, EDITSTATE *es, BOOL extend);
static void	EDIT_MoveWordBackward(HWND hwnd, EDITSTATE *es, BOOL extend);
static void	EDIT_MoveWordForward(HWND hwnd, EDITSTATE *es, BOOL extend);
static void	EDIT_PaintLine(HWND hwnd, EDITSTATE *es, HDC hdc, INT line, BOOL rev);
static INT	EDIT_PaintText(EDITSTATE *es, HDC hdc, INT x, INT y, INT line, INT col, INT count, BOOL rev);
static void	EDIT_SetCaretPos(HWND hwnd, EDITSTATE *es, INT pos, BOOL after_wrap); 
static void	EDIT_SetRectNP(HWND hwnd, EDITSTATE *es, LPRECT lprc);
static void	EDIT_UnlockBuffer(HWND hwnd, EDITSTATE *es, BOOL force);
static void	EDIT_UpdateScrollInfo(HWND hwnd, EDITSTATE *es);
static INT CALLBACK EDIT_WordBreakProc(LPWSTR s, INT index, INT count, INT action);
/*
 *	EM_XXX message handlers
 */
static LRESULT	EDIT_EM_CharFromPos(HWND hwnd, EDITSTATE *es, INT x, INT y);
static BOOL	EDIT_EM_FmtLines(EDITSTATE *es, BOOL add_eol);
static HLOCAL	EDIT_EM_GetHandle(EDITSTATE *es);
static HLOCAL16	EDIT_EM_GetHandle16(HWND hwnd, EDITSTATE *es);
static INT	EDIT_EM_GetLine(EDITSTATE *es, INT line, LPARAM lParam, BOOL unicode);
static LRESULT	EDIT_EM_GetSel(EDITSTATE *es, LPUINT start, LPUINT end);
static LRESULT	EDIT_EM_GetThumb(HWND hwnd, EDITSTATE *es);
static INT	EDIT_EM_LineFromChar(EDITSTATE *es, INT index);
static INT	EDIT_EM_LineIndex(EDITSTATE *es, INT line);
static INT	EDIT_EM_LineLength(EDITSTATE *es, INT index);
static BOOL	EDIT_EM_LineScroll(HWND hwnd, EDITSTATE *es, INT dx, INT dy);
static BOOL	EDIT_EM_LineScroll_internal(HWND hwnd, EDITSTATE *es, INT dx, INT dy);
static LRESULT	EDIT_EM_PosFromChar(HWND hwnd, EDITSTATE *es, INT index, BOOL after_wrap);
static void	EDIT_EM_ReplaceSel(HWND hwnd, EDITSTATE *es, BOOL can_undo, LPCWSTR lpsz_replace, BOOL send_update);
static LRESULT	EDIT_EM_Scroll(HWND hwnd, EDITSTATE *es, INT action);
static void	EDIT_EM_ScrollCaret(HWND hwnd, EDITSTATE *es);
static void	EDIT_EM_SetHandle(HWND hwnd, EDITSTATE *es, HLOCAL hloc);
static void	EDIT_EM_SetHandle16(HWND hwnd, EDITSTATE *es, HLOCAL16 hloc);
static void	EDIT_EM_SetLimitText(EDITSTATE *es, INT limit);
static void	EDIT_EM_SetMargins(EDITSTATE *es, INT action, INT left, INT right);
static void	EDIT_EM_SetPasswordChar(HWND hwnd, EDITSTATE *es, WCHAR c);
static void	EDIT_EM_SetSel(HWND hwnd, EDITSTATE *es, UINT start, UINT end, BOOL after_wrap);
static BOOL	EDIT_EM_SetTabStops(EDITSTATE *es, INT count, LPINT tabs);
static BOOL	EDIT_EM_SetTabStops16(EDITSTATE *es, INT count, LPINT16 tabs);
static void	EDIT_EM_SetWordBreakProc(HWND hwnd, EDITSTATE *es, LPARAM lParam);
static void	EDIT_EM_SetWordBreakProc16(HWND hwnd, EDITSTATE *es, EDITWORDBREAKPROC16 wbp);
static BOOL	EDIT_EM_Undo(HWND hwnd, EDITSTATE *es);
/*
 *	WM_XXX message handlers
 */
static void	EDIT_WM_Char(HWND hwnd, EDITSTATE *es, WCHAR c);
static void	EDIT_WM_Command(HWND hwnd, EDITSTATE *es, INT code, INT id, HWND conrtol);
static void	EDIT_WM_ContextMenu(HWND hwnd, EDITSTATE *es, INT x, INT y);
static void	EDIT_WM_Copy(HWND hwnd, EDITSTATE *es);
static LRESULT	EDIT_WM_Create(HWND hwnd, EDITSTATE *es, LPCWSTR name);
static void	EDIT_WM_Destroy(HWND hwnd, EDITSTATE *es);
static LRESULT	EDIT_WM_EraseBkGnd(HWND hwnd, EDITSTATE *es, HDC dc);
static INT	EDIT_WM_GetText(EDITSTATE *es, INT count, LPARAM lParam, BOOL unicode);
static LRESULT	EDIT_WM_HScroll(HWND hwnd, EDITSTATE *es, INT action, INT pos);
static LRESULT	EDIT_WM_KeyDown(HWND hwnd, EDITSTATE *es, INT key);
static LRESULT	EDIT_WM_KillFocus(HWND hwnd, EDITSTATE *es);
static LRESULT	EDIT_WM_LButtonDblClk(HWND hwnd, EDITSTATE *es);
static LRESULT	EDIT_WM_LButtonDown(HWND hwnd, EDITSTATE *es, DWORD keys, INT x, INT y);
static LRESULT	EDIT_WM_LButtonUp(HWND hwndSelf, EDITSTATE *es);
static LRESULT	EDIT_WM_MButtonDown(HWND hwnd);
static LRESULT	EDIT_WM_MouseMove(HWND hwnd, EDITSTATE *es, INT x, INT y);
static LRESULT	EDIT_WM_NCCreate(HWND hwnd, DWORD style, HWND hwndParent, BOOL unicode);
static void	EDIT_WM_Paint(HWND hwnd, EDITSTATE *es, WPARAM wParam);
static void	EDIT_WM_Paste(HWND hwnd, EDITSTATE *es);
static void	EDIT_WM_SetFocus(HWND hwnd, EDITSTATE *es);
static void	EDIT_WM_SetFont(HWND hwnd, EDITSTATE *es, HFONT font, BOOL redraw);
static void	EDIT_WM_SetText(HWND hwnd, EDITSTATE *es, LPARAM lParam, BOOL unicode);
static void	EDIT_WM_Size(HWND hwnd, EDITSTATE *es, UINT action, INT width, INT height);
static LRESULT  EDIT_WM_StyleChanged (HWND hwnd, EDITSTATE *es, WPARAM which, const STYLESTRUCT *style);
static LRESULT	EDIT_WM_SysKeyDown(HWND hwnd, EDITSTATE *es, INT key, DWORD key_data);
static void	EDIT_WM_Timer(HWND hwnd, EDITSTATE *es);
static LRESULT	EDIT_WM_VScroll(HWND hwnd, EDITSTATE *es, INT action, INT pos);
static void EDIT_UpdateText(HWND hwnd, EDITSTATE *es, LPRECT rc, BOOL bErase);
static void EDIT_UpdateTextRegion(HWND hwnd, EDITSTATE *es, HRGN hrgn, BOOL bErase);

LRESULT WINAPI EditWndProcA(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI EditWndProcW(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

/*********************************************************************
 * edit class descriptor
 */
const struct builtin_class_descr EDIT_builtin_class =
{
    "Edit",               /* name */
    CS_GLOBALCLASS | CS_DBLCLKS | CS_PARENTDC,   /* style */
    EditWndProcA,         /* procA */
    EditWndProcW,         /* procW */
    sizeof(EDITSTATE *),  /* extra */
    IDC_IBEAMA,           /* cursor */
    0                     /* brush */
};


/*********************************************************************
 *
 *	EM_CANUNDO
 *
 */
static inline BOOL EDIT_EM_CanUndo(EDITSTATE *es)
{
	return (es->undo_insert_count || strlenW(es->undo_text));
}


/*********************************************************************
 *
 *	EM_EMPTYUNDOBUFFER
 *
 */
static inline void EDIT_EM_EmptyUndoBuffer(EDITSTATE *es)
{
	es->undo_insert_count = 0;
	*es->undo_text = '\0';
}


/*********************************************************************
 *
 *	WM_CLEAR
 *
 */
static inline void EDIT_WM_Clear(HWND hwnd, EDITSTATE *es)
{
	static const WCHAR empty_stringW[] = {0};

	/* Protect read-only edit control from modification */
	if(es->style & ES_READONLY)
	    return;

	EDIT_EM_ReplaceSel(hwnd, es, TRUE, empty_stringW, TRUE);
}


/*********************************************************************
 *
 *	WM_CUT
 *
 */
static inline void EDIT_WM_Cut(HWND hwnd, EDITSTATE *es)
{
	EDIT_WM_Copy(hwnd, es);
	EDIT_WM_Clear(hwnd, es);
}


/**********************************************************************
 *         get_app_version
 *
 * Returns the window version in case Wine emulates a later version
 * of windows then the application expects.
 * 
 * In a number of cases when windows runs an application that was
 * designed for an earlier windows version, windows reverts
 * to "old" behaviour of that earlier version.
 * 
 * An example is a disabled  edit control that needs to be painted. 
 * Old style behaviour is to send a WM_CTLCOLOREDIT message. This was 
 * changed in Win95, NT4.0 by a WM_CTLCOLORSTATIC message _only_ for 
 * applications with an expected version 0f 4.0 or higher.
 * 
 */
static DWORD get_app_version(void)
{
    static DWORD version;
    if (!version)
    {
        DWORD dwEmulatedVersion;
        OSVERSIONINFOW info;
        DWORD dwProcVersion = GetProcessVersion(0);

	info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
        GetVersionExW( &info );
        dwEmulatedVersion = MAKELONG( info.dwMinorVersion, info.dwMajorVersion );
        /* FIXME: this may not be 100% correct; see discussion on the
         * wine developer list in Nov 1999 */
        version = dwProcVersion < dwEmulatedVersion ? dwProcVersion : dwEmulatedVersion; 
    }
    return version;
}


/*********************************************************************
 *
 *	EditWndProc_common
 *
 *	The messages are in the order of the actual integer values
 *	(which can be found in include/windows.h)
 *	Wherever possible the 16 bit versions are converted to
 *	the 32 bit ones, so that we can 'fall through' to the
 *	helper functions.  These are mostly 32 bit (with a few
 *	exceptions, clearly indicated by a '16' extension to their
 *	names).
 *
 */
static LRESULT WINAPI EditWndProc_common( HWND hwnd, UINT msg,
                                          WPARAM wParam, LPARAM lParam, BOOL unicode )
{
	EDITSTATE *es = (EDITSTATE *)GetWindowLongA( hwnd, 0 );
	LRESULT result = 0;

	switch (msg) {
	case WM_DESTROY:
		DPRINTF_EDIT_MSG32("WM_DESTROY");
		if (es) EDIT_WM_Destroy(hwnd, es);
                result = 0;
                goto END;

	case WM_NCCREATE:
		DPRINTF_EDIT_MSG32("WM_NCCREATE");
		if(unicode)
		{
		    LPCREATESTRUCTW cs = (LPCREATESTRUCTW)lParam;
		    result = EDIT_WM_NCCreate(hwnd, cs->style, cs->hwndParent, TRUE);
		}
		else
		{
		    LPCREATESTRUCTA cs = (LPCREATESTRUCTA)lParam;
		    result = EDIT_WM_NCCreate(hwnd, cs->style, cs->hwndParent, FALSE);
		}
                goto END;
	}

	if (!es)
        {
	    if(unicode)
		result = DefWindowProcW(hwnd, msg, wParam, lParam);
	    else
		result = DefWindowProcA(hwnd, msg, wParam, lParam);
            goto END;
        }


	EDIT_LockBuffer(hwnd, es);
	switch (msg) {
	case EM_GETSEL16:
		DPRINTF_EDIT_MSG16("EM_GETSEL");
		wParam = 0;
		lParam = 0;
		/* fall through */
	case EM_GETSEL:
		DPRINTF_EDIT_MSG32("EM_GETSEL");
		result = EDIT_EM_GetSel(es, (LPUINT)wParam, (LPUINT)lParam);
		break;

	case EM_SETSEL16:
		DPRINTF_EDIT_MSG16("EM_SETSEL");
		if (SLOWORD(lParam) == -1)
			EDIT_EM_SetSel(hwnd, es, (UINT)-1, 0, FALSE);
		else
			EDIT_EM_SetSel(hwnd, es, LOWORD(lParam), HIWORD(lParam), FALSE);
		if (!wParam)
			EDIT_EM_ScrollCaret(hwnd, es);
		result = 1;
		break;
	case EM_SETSEL:
		DPRINTF_EDIT_MSG32("EM_SETSEL");
		EDIT_EM_SetSel(hwnd, es, wParam, lParam, FALSE);
		EDIT_EM_ScrollCaret(hwnd, es);
		result = 1;
		break;

	case EM_GETRECT16:
		DPRINTF_EDIT_MSG16("EM_GETRECT");
		if (lParam)
			CONV_RECT32TO16(&es->format_rect, MapSL(lParam));
		break;
	case EM_GETRECT:
		DPRINTF_EDIT_MSG32("EM_GETRECT");
		if (lParam)
			CopyRect((LPRECT)lParam, &es->format_rect);
		break;

	case EM_SETRECT16:
		DPRINTF_EDIT_MSG16("EM_SETRECT");
		if ((es->style & ES_MULTILINE) && lParam) {
			RECT rc;
			CONV_RECT16TO32(MapSL(lParam), &rc);
			EDIT_SetRectNP(hwnd, es, &rc);
			EDIT_UpdateText(hwnd, es, NULL, TRUE);
		}
		break;
	case EM_SETRECT:
		DPRINTF_EDIT_MSG32("EM_SETRECT");
		if ((es->style & ES_MULTILINE) && lParam) {
			EDIT_SetRectNP(hwnd, es, (LPRECT)lParam);
			EDIT_UpdateText(hwnd, es, NULL, TRUE);
		}
		break;

	case EM_SETRECTNP16:
		DPRINTF_EDIT_MSG16("EM_SETRECTNP");
		if ((es->style & ES_MULTILINE) && lParam) {
			RECT rc;
			CONV_RECT16TO32(MapSL(lParam), &rc);
			EDIT_SetRectNP(hwnd, es, &rc);
		}
		break;
	case EM_SETRECTNP:
		DPRINTF_EDIT_MSG32("EM_SETRECTNP");
		if ((es->style & ES_MULTILINE) && lParam)
			EDIT_SetRectNP(hwnd, es, (LPRECT)lParam);
		break;

	case EM_SCROLL16:
		DPRINTF_EDIT_MSG16("EM_SCROLL");
		/* fall through */
	case EM_SCROLL:
		DPRINTF_EDIT_MSG32("EM_SCROLL");
		result = EDIT_EM_Scroll(hwnd, es, (INT)wParam);
 		break;

	case EM_LINESCROLL16:
		DPRINTF_EDIT_MSG16("EM_LINESCROLL");
		wParam = (WPARAM)(INT)SHIWORD(lParam);
		lParam = (LPARAM)(INT)SLOWORD(lParam);
		/* fall through */
	case EM_LINESCROLL:
		DPRINTF_EDIT_MSG32("EM_LINESCROLL");
		result = (LRESULT)EDIT_EM_LineScroll(hwnd, es, (INT)wParam, (INT)lParam);
		break;

	case EM_SCROLLCARET16:
		DPRINTF_EDIT_MSG16("EM_SCROLLCARET");
		/* fall through */
	case EM_SCROLLCARET:
		DPRINTF_EDIT_MSG32("EM_SCROLLCARET");
		EDIT_EM_ScrollCaret(hwnd, es);
		result = 1;
		break;

	case EM_GETMODIFY16:
		DPRINTF_EDIT_MSG16("EM_GETMODIFY");
		/* fall through */
	case EM_GETMODIFY:
		DPRINTF_EDIT_MSG32("EM_GETMODIFY");
		result = ((es->flags & EF_MODIFIED) != 0);
		break;

	case EM_SETMODIFY16:
		DPRINTF_EDIT_MSG16("EM_SETMODIFY");
		/* fall through */
	case EM_SETMODIFY:
		DPRINTF_EDIT_MSG32("EM_SETMODIFY");
		if (wParam)
			es->flags |= EF_MODIFIED;
		else
                        es->flags &= ~(EF_MODIFIED | EF_UPDATE);  /* reset pending updates */
		break;

	case EM_GETLINECOUNT16:
		DPRINTF_EDIT_MSG16("EM_GETLINECOUNT");
		/* fall through */
	case EM_GETLINECOUNT:
		DPRINTF_EDIT_MSG32("EM_GETLINECOUNT");
		result = (es->style & ES_MULTILINE) ? es->line_count : 1;
		break;

	case EM_LINEINDEX16:
		DPRINTF_EDIT_MSG16("EM_LINEINDEX");
		if ((INT16)wParam == -1)
			wParam = (WPARAM)-1;
		/* fall through */
	case EM_LINEINDEX:
		DPRINTF_EDIT_MSG32("EM_LINEINDEX");
		result = (LRESULT)EDIT_EM_LineIndex(es, (INT)wParam);
		break;

	case EM_SETHANDLE16:
		DPRINTF_EDIT_MSG16("EM_SETHANDLE");
		EDIT_EM_SetHandle16(hwnd, es, (HLOCAL16)wParam);
		break;
	case EM_SETHANDLE:
		DPRINTF_EDIT_MSG32("EM_SETHANDLE");
		EDIT_EM_SetHandle(hwnd, es, (HLOCAL)wParam);
		break;

	case EM_GETHANDLE16:
		DPRINTF_EDIT_MSG16("EM_GETHANDLE");
		result = (LRESULT)EDIT_EM_GetHandle16(hwnd, es);
		break;
	case EM_GETHANDLE:
		DPRINTF_EDIT_MSG32("EM_GETHANDLE");
		result = (LRESULT)EDIT_EM_GetHandle(es);
		break;

	case EM_GETTHUMB16:
		DPRINTF_EDIT_MSG16("EM_GETTHUMB");
		/* fall through */
	case EM_GETTHUMB:
		DPRINTF_EDIT_MSG32("EM_GETTHUMB");
		result = EDIT_EM_GetThumb(hwnd, es);
		break;

	/* messages 0x00bf and 0x00c0 missing from specs */

	case WM_USER+15:
		DPRINTF_EDIT_MSG16("undocumented WM_USER+15, please report");
		/* fall through */
	case 0x00bf:
		DPRINTF_EDIT_MSG32("undocumented 0x00bf, please report");
		result = DefWindowProcW(hwnd, msg, wParam, lParam);
		break;

	case WM_USER+16:
		DPRINTF_EDIT_MSG16("undocumented WM_USER+16, please report");
		/* fall through */
	case 0x00c0:
		DPRINTF_EDIT_MSG32("undocumented 0x00c0, please report");
		result = DefWindowProcW(hwnd, msg, wParam, lParam);
		break;

	case EM_LINELENGTH16:
		DPRINTF_EDIT_MSG16("EM_LINELENGTH");
		/* fall through */
	case EM_LINELENGTH:
		DPRINTF_EDIT_MSG32("EM_LINELENGTH");
		result = (LRESULT)EDIT_EM_LineLength(es, (INT)wParam);
		break;

	case EM_REPLACESEL16:
		DPRINTF_EDIT_MSG16("EM_REPLACESEL");
		lParam = (LPARAM)MapSL(lParam);
		unicode = FALSE;  /* 16-bit message is always ascii */
		/* fall through */
	case EM_REPLACESEL:
	{
		LPWSTR textW;
		DPRINTF_EDIT_MSG32("EM_REPLACESEL");

		if(unicode)
		    textW = (LPWSTR)lParam;
		else
		{
		    LPSTR textA = (LPSTR)lParam;
		    INT countW = MultiByteToWideChar(CP_ACP, 0, textA, -1, NULL, 0);
		    if((textW = HeapAlloc(GetProcessHeap(), 0, countW * sizeof(WCHAR))))
			MultiByteToWideChar(CP_ACP, 0, textA, -1, textW, countW);
		}

		EDIT_EM_ReplaceSel(hwnd, es, (BOOL)wParam, textW, TRUE);
		result = 1;

		if(!unicode)
		    HeapFree(GetProcessHeap(), 0, textW);
		break;
	}
	/* message 0x00c3 missing from specs */

	case WM_USER+19:
		DPRINTF_EDIT_MSG16("undocumented WM_USER+19, please report");
		/* fall through */
	case 0x00c3:
		DPRINTF_EDIT_MSG32("undocumented 0x00c3, please report");
		result = DefWindowProcW(hwnd, msg, wParam, lParam);
		break;

	case EM_GETLINE16:
		DPRINTF_EDIT_MSG16("EM_GETLINE");
		lParam = (LPARAM)MapSL(lParam);
		unicode = FALSE;  /* 16-bit message is always ascii */
		/* fall through */
	case EM_GETLINE:
		DPRINTF_EDIT_MSG32("EM_GETLINE");
		result = (LRESULT)EDIT_EM_GetLine(es, (INT)wParam, lParam, unicode);
		break;

	case EM_LIMITTEXT16:
		DPRINTF_EDIT_MSG16("EM_LIMITTEXT");
		/* fall through */
	case EM_SETLIMITTEXT:
		DPRINTF_EDIT_MSG32("EM_SETLIMITTEXT");
		EDIT_EM_SetLimitText(es, (INT)wParam);
		break;

	case EM_CANUNDO16:
		DPRINTF_EDIT_MSG16("EM_CANUNDO");
		/* fall through */
	case EM_CANUNDO:
		DPRINTF_EDIT_MSG32("EM_CANUNDO");
		result = (LRESULT)EDIT_EM_CanUndo(es);
		break;

	case EM_UNDO16:
		DPRINTF_EDIT_MSG16("EM_UNDO");
		/* fall through */
	case EM_UNDO:
		/* fall through */
	case WM_UNDO:
		DPRINTF_EDIT_MSG32("EM_UNDO / WM_UNDO");
		result = (LRESULT)EDIT_EM_Undo(hwnd, es);
		break;

	case EM_FMTLINES16:
		DPRINTF_EDIT_MSG16("EM_FMTLINES");
		/* fall through */
	case EM_FMTLINES:
		DPRINTF_EDIT_MSG32("EM_FMTLINES");
		result = (LRESULT)EDIT_EM_FmtLines(es, (BOOL)wParam);
		break;

	case EM_LINEFROMCHAR16:
		DPRINTF_EDIT_MSG16("EM_LINEFROMCHAR");
		/* fall through */
	case EM_LINEFROMCHAR:
		DPRINTF_EDIT_MSG32("EM_LINEFROMCHAR");
		result = (LRESULT)EDIT_EM_LineFromChar(es, (INT)wParam);
		break;

	/* message 0x00ca missing from specs */

	case WM_USER+26:
		DPRINTF_EDIT_MSG16("undocumented WM_USER+26, please report");
		/* fall through */
	case 0x00ca:
		DPRINTF_EDIT_MSG32("undocumented 0x00ca, please report");
		result = DefWindowProcW(hwnd, msg, wParam, lParam);
		break;

	case EM_SETTABSTOPS16:
		DPRINTF_EDIT_MSG16("EM_SETTABSTOPS");
		result = (LRESULT)EDIT_EM_SetTabStops16(es, (INT)wParam, MapSL(lParam));
		break;
	case EM_SETTABSTOPS:
		DPRINTF_EDIT_MSG32("EM_SETTABSTOPS");
		result = (LRESULT)EDIT_EM_SetTabStops(es, (INT)wParam, (LPINT)lParam);
		break;

	case EM_SETPASSWORDCHAR16:
		DPRINTF_EDIT_MSG16("EM_SETPASSWORDCHAR");
		unicode = FALSE;  /* 16-bit message is always ascii */
		/* fall through */
	case EM_SETPASSWORDCHAR:
	{
		WCHAR charW = 0;
		DPRINTF_EDIT_MSG32("EM_SETPASSWORDCHAR");

		if(unicode)
		    charW = (WCHAR)wParam;
		else
		{
		    CHAR charA = wParam;
		    MultiByteToWideChar(CP_ACP, 0, &charA, 1, &charW, 1);
		}

		EDIT_EM_SetPasswordChar(hwnd, es, charW);
		break;
	}

	case EM_EMPTYUNDOBUFFER16:
		DPRINTF_EDIT_MSG16("EM_EMPTYUNDOBUFFER");
		/* fall through */
	case EM_EMPTYUNDOBUFFER:
		DPRINTF_EDIT_MSG32("EM_EMPTYUNDOBUFFER");
		EDIT_EM_EmptyUndoBuffer(es);
		break;

	case EM_GETFIRSTVISIBLELINE16:
		DPRINTF_EDIT_MSG16("EM_GETFIRSTVISIBLELINE");
		result = es->y_offset;
		break;
	case EM_GETFIRSTVISIBLELINE:
		DPRINTF_EDIT_MSG32("EM_GETFIRSTVISIBLELINE");
		result = (es->style & ES_MULTILINE) ? es->y_offset : es->x_offset;
		break;

	case EM_SETREADONLY16:
		DPRINTF_EDIT_MSG16("EM_SETREADONLY");
		/* fall through */
	case EM_SETREADONLY:
		DPRINTF_EDIT_MSG32("EM_SETREADONLY");
		if (wParam) {
                    SetWindowLongA( hwnd, GWL_STYLE,
                                    GetWindowLongA( hwnd, GWL_STYLE ) | ES_READONLY );
                    es->style |= ES_READONLY;
		} else {
                    SetWindowLongA( hwnd, GWL_STYLE,
                                    GetWindowLongA( hwnd, GWL_STYLE ) & ~ES_READONLY );
                    es->style &= ~ES_READONLY;
		}
                result = 1;
 		break;

	case EM_SETWORDBREAKPROC16:
		DPRINTF_EDIT_MSG16("EM_SETWORDBREAKPROC");
		EDIT_EM_SetWordBreakProc16(hwnd, es, (EDITWORDBREAKPROC16)lParam);
		break;
	case EM_SETWORDBREAKPROC:
		DPRINTF_EDIT_MSG32("EM_SETWORDBREAKPROC");
		EDIT_EM_SetWordBreakProc(hwnd, es, lParam);
		break;

	case EM_GETWORDBREAKPROC16:
		DPRINTF_EDIT_MSG16("EM_GETWORDBREAKPROC");
		result = (LRESULT)es->word_break_proc16;
		break;
	case EM_GETWORDBREAKPROC:
		DPRINTF_EDIT_MSG32("EM_GETWORDBREAKPROC");
		result = (LRESULT)es->word_break_proc;
		break;

	case EM_GETPASSWORDCHAR16:
		DPRINTF_EDIT_MSG16("EM_GETPASSWORDCHAR");
		unicode = FALSE;  /* 16-bit message is always ascii */
		/* fall through */
	case EM_GETPASSWORDCHAR:
	{
		DPRINTF_EDIT_MSG32("EM_GETPASSWORDCHAR");

		if(unicode)
		    result = es->password_char;
		else
		{
		    WCHAR charW = es->password_char;
		    CHAR charA = 0;
		    WideCharToMultiByte(CP_ACP, 0, &charW, 1, &charA, 1, NULL, NULL);
		    result = charA;
		}
		break;
	}

	/* The following EM_xxx are new to win95 and don't exist for 16 bit */

	case EM_SETMARGINS:
		DPRINTF_EDIT_MSG32("EM_SETMARGINS");
		EDIT_EM_SetMargins(es, (INT)wParam, SLOWORD(lParam), SHIWORD(lParam));
		break;

	case EM_GETMARGINS:
		DPRINTF_EDIT_MSG32("EM_GETMARGINS");
		result = MAKELONG(es->left_margin, es->right_margin);
		break;

	case EM_GETLIMITTEXT:
		DPRINTF_EDIT_MSG32("EM_GETLIMITTEXT");
		result = es->buffer_limit;
		break;

	case EM_POSFROMCHAR:
		DPRINTF_EDIT_MSG32("EM_POSFROMCHAR");
		result = EDIT_EM_PosFromChar(hwnd, es, (INT)wParam, FALSE);
		break;

	case EM_CHARFROMPOS:
		DPRINTF_EDIT_MSG32("EM_CHARFROMPOS");
		result = EDIT_EM_CharFromPos(hwnd, es, SLOWORD(lParam), SHIWORD(lParam));
		break;

        /* End of the EM_ messages which were in numerical order; what order
         * are these in?  vaguely alphabetical?
         */

	case WM_GETDLGCODE:
		DPRINTF_EDIT_MSG32("WM_GETDLGCODE");
		result = DLGC_HASSETSEL | DLGC_WANTCHARS | DLGC_WANTARROWS;

		if (lParam && (((LPMSG)lParam)->message == WM_KEYDOWN))
		{
		   int vk = (int)((LPMSG)lParam)->wParam;

		   if (vk == VK_RETURN && (GetWindowLongA( hwnd, GWL_STYLE ) & ES_WANTRETURN))
		   {
		      result |= DLGC_WANTMESSAGE;
		   }
		   else if (es->hwndListBox && (vk == VK_RETURN || vk == VK_ESCAPE))
		   {
		      if (SendMessageW(GetParent(hwnd), CB_GETDROPPEDSTATE, 0, 0))
		         result |= DLGC_WANTMESSAGE;
		   }
		}
		break;

	case WM_CHAR:
	{
		WCHAR charW;
		DPRINTF_EDIT_MSG32("WM_CHAR");

		if(unicode)
		    charW = wParam;
		else
		{
		    CHAR charA = wParam;
		    MultiByteToWideChar(CP_ACP, 0, &charA, 1, &charW, 1);
		}

		if ((charW == VK_RETURN || charW == VK_ESCAPE) && es->hwndListBox)
		{
		   if (SendMessageW(GetParent(hwnd), CB_GETDROPPEDSTATE, 0, 0))
		      SendMessageW(GetParent(hwnd), WM_KEYDOWN, charW, 0);
		   break;
		}
		EDIT_WM_Char(hwnd, es, charW);
		break;
	}

	case WM_CLEAR:
		DPRINTF_EDIT_MSG32("WM_CLEAR");
		EDIT_WM_Clear(hwnd, es);
		break;

	case WM_COMMAND:
		DPRINTF_EDIT_MSG32("WM_COMMAND");
		EDIT_WM_Command(hwnd, es, HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
		break;

 	case WM_CONTEXTMENU:
		DPRINTF_EDIT_MSG32("WM_CONTEXTMENU");
		EDIT_WM_ContextMenu(hwnd, es, SLOWORD(lParam), SHIWORD(lParam));
		break;

	case WM_COPY:
		DPRINTF_EDIT_MSG32("WM_COPY");
		EDIT_WM_Copy(hwnd, es);
		break;

	case WM_CREATE:
		DPRINTF_EDIT_MSG32("WM_CREATE");
		if(unicode)
		    result = EDIT_WM_Create(hwnd, es, ((LPCREATESTRUCTW)lParam)->lpszName);
		else
		{
		    LPCSTR nameA = ((LPCREATESTRUCTA)lParam)->lpszName;
		    LPWSTR nameW = NULL;
		    if(nameA)
		    {
			INT countW = MultiByteToWideChar(CP_ACP, 0, nameA, -1, NULL, 0);
			if((nameW = HeapAlloc(GetProcessHeap(), 0, countW * sizeof(WCHAR))))
			    MultiByteToWideChar(CP_ACP, 0, nameA, -1, nameW, countW);
		    }
		    result = EDIT_WM_Create(hwnd, es, nameW);
		    if(nameW)
			HeapFree(GetProcessHeap(), 0, nameW);
		}
		break;

	case WM_CUT:
		DPRINTF_EDIT_MSG32("WM_CUT");
		EDIT_WM_Cut(hwnd, es);
		break;

	case WM_ENABLE:
		DPRINTF_EDIT_MSG32("WM_ENABLE");
                es->bEnableState = (BOOL) wParam;
		EDIT_UpdateText(hwnd, es, NULL, TRUE);
		break;

	case WM_ERASEBKGND:
		DPRINTF_EDIT_MSG32("WM_ERASEBKGND");
		result = EDIT_WM_EraseBkGnd(hwnd, es, (HDC)wParam);
		break;

	case WM_GETFONT:
		DPRINTF_EDIT_MSG32("WM_GETFONT");
		result = (LRESULT)es->font;
		break;

	case WM_GETTEXT:
		DPRINTF_EDIT_MSG32("WM_GETTEXT");
		result = (LRESULT)EDIT_WM_GetText(es, (INT)wParam, lParam, unicode);
		break;

	case WM_GETTEXTLENGTH:
		DPRINTF_EDIT_MSG32("WM_GETTEXTLENGTH");
		result = strlenW(es->text);
		break;

	case WM_HSCROLL:
		DPRINTF_EDIT_MSG32("WM_HSCROLL");
		result = EDIT_WM_HScroll(hwnd, es, LOWORD(wParam), SHIWORD(wParam));
		break;

	case WM_KEYDOWN:
		DPRINTF_EDIT_MSG32("WM_KEYDOWN");
		result = EDIT_WM_KeyDown(hwnd, es, (INT)wParam);
		break;

	case WM_KILLFOCUS:
		DPRINTF_EDIT_MSG32("WM_KILLFOCUS");
		result = EDIT_WM_KillFocus(hwnd, es);
		break;

	case WM_LBUTTONDBLCLK:
		DPRINTF_EDIT_MSG32("WM_LBUTTONDBLCLK");
		result = EDIT_WM_LButtonDblClk(hwnd, es);
		break;

	case WM_LBUTTONDOWN:
		DPRINTF_EDIT_MSG32("WM_LBUTTONDOWN");
		result = EDIT_WM_LButtonDown(hwnd, es, (DWORD)wParam, SLOWORD(lParam), SHIWORD(lParam));
		break;

	case WM_LBUTTONUP:
		DPRINTF_EDIT_MSG32("WM_LBUTTONUP");
		result = EDIT_WM_LButtonUp(hwnd, es);
		break;

	case WM_MBUTTONDOWN:                        
  		DPRINTF_EDIT_MSG32("WM_MBUTTONDOWN");    
  		result = EDIT_WM_MButtonDown(hwnd);
		break;

	case WM_MOUSEACTIVATE:
		/*
		 *	FIXME: maybe DefWindowProc() screws up, but it seems that
		 *		modeless dialog boxes need this.  If we don't do this, the focus
		 *		will _not_ be set by DefWindowProc() for edit controls in a
		 *		modeless dialog box ???
		 */
		DPRINTF_EDIT_MSG32("WM_MOUSEACTIVATE");
		SetFocus(hwnd);
		result = MA_ACTIVATE;
		break;

	case WM_MOUSEMOVE:
		/*
		 *	DPRINTF_EDIT_MSG32("WM_MOUSEMOVE");
		 */
		result = EDIT_WM_MouseMove(hwnd, es, SLOWORD(lParam), SHIWORD(lParam));
		break;

	case WM_PAINT:
		DPRINTF_EDIT_MSG32("WM_PAINT");
	        EDIT_WM_Paint(hwnd, es, wParam);
		break;

	case WM_PASTE:
		DPRINTF_EDIT_MSG32("WM_PASTE");
		EDIT_WM_Paste(hwnd, es);
		break;

	case WM_SETFOCUS:
		DPRINTF_EDIT_MSG32("WM_SETFOCUS");
		EDIT_WM_SetFocus(hwnd, es);
		break;

	case WM_SETFONT:
		DPRINTF_EDIT_MSG32("WM_SETFONT");
		EDIT_WM_SetFont(hwnd, es, (HFONT)wParam, LOWORD(lParam) != 0);
		break;

	case WM_SETREDRAW:
		/* FIXME: actually set an internal flag and behave accordingly */
		break;

	case WM_SETTEXT:
		DPRINTF_EDIT_MSG32("WM_SETTEXT");
		EDIT_WM_SetText(hwnd, es, lParam, unicode);
		result = TRUE;
		break;

	case WM_SIZE:
		DPRINTF_EDIT_MSG32("WM_SIZE");
		EDIT_WM_Size(hwnd, es, (UINT)wParam, LOWORD(lParam), HIWORD(lParam));
		break;

        case WM_STYLECHANGED:
		DPRINTF_EDIT_MSG32("WM_STYLECHANGED");
                result = EDIT_WM_StyleChanged (hwnd, es, wParam, (const STYLESTRUCT *)lParam);
                break;
               
        case WM_STYLECHANGING:
		DPRINTF_EDIT_MSG32("WM_STYLECHANGING");
                result = 0; /* See EDIT_WM_StyleChanged */
                break;

	case WM_SYSKEYDOWN:
		DPRINTF_EDIT_MSG32("WM_SYSKEYDOWN");
		result = EDIT_WM_SysKeyDown(hwnd, es, (INT)wParam, (DWORD)lParam);
		break;

	case WM_TIMER:
		DPRINTF_EDIT_MSG32("WM_TIMER");
		EDIT_WM_Timer(hwnd, es);
		break;

	case WM_VSCROLL:
		DPRINTF_EDIT_MSG32("WM_VSCROLL");
		result = EDIT_WM_VScroll(hwnd, es, LOWORD(wParam), SHIWORD(wParam));
		break;

        case WM_MOUSEWHEEL:
                {
                    int gcWheelDelta = 0;
                    UINT pulScrollLines = 3;
                    SystemParametersInfoW(SPI_GETWHEELSCROLLLINES,0, &pulScrollLines, 0);

                    if (wParam & (MK_SHIFT | MK_CONTROL)) {
                        result = DefWindowProcW(hwnd, msg, wParam, lParam);
                        break;
                    }
                    gcWheelDelta -= SHIWORD(wParam);
                    if (abs(gcWheelDelta) >= WHEEL_DELTA && pulScrollLines)
                    {
                        int cLineScroll= (int) min((UINT) es->line_count, pulScrollLines);
                        cLineScroll *= (gcWheelDelta / WHEEL_DELTA);
			result = EDIT_EM_LineScroll(hwnd, es, 0, cLineScroll);
                    }
                }
                break;
	default:
		if(unicode)
		    result = DefWindowProcW(hwnd, msg, wParam, lParam);
		else
		    result = DefWindowProcA(hwnd, msg, wParam, lParam);
		break;
	}
	EDIT_UnlockBuffer(hwnd, es, FALSE);
    END:
	return result;
}

/*********************************************************************
 *
 *	EditWndProcW   (USER32.@)
 */
LRESULT WINAPI EditWndProcW(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (!IsWindow( hWnd )) return 0;
    return EditWndProc_common(hWnd, uMsg, wParam, lParam, TRUE);
}

/*********************************************************************
 *
 *	EditWndProc   (USER32.@)
 */
LRESULT WINAPI EditWndProcA(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (!IsWindow( hWnd )) return 0;
    return EditWndProc_common(hWnd, uMsg, wParam, lParam, FALSE);
}

/*********************************************************************
 *
 *	EDIT_BuildLineDefs_ML
 *
 *	Build linked list of text lines.
 *	Lines can end with '\0' (last line), a character (if it is wrapped),
 *	a soft return '\r\r\n' or a hard return '\r\n'
 *
 */
static void EDIT_BuildLineDefs_ML(HWND hwnd, EDITSTATE *es, INT istart, INT iend, INT delta, HRGN hrgn)
{
	HDC dc;
	HFONT old_font = 0;
	LPWSTR current_position, cp;
	INT fw;
	LINEDEF *current_line;
	LINEDEF *previous_line;
	LINEDEF *start_line;
	INT line_index = 0, nstart_line = 0, nstart_index = 0;
	INT line_count = es->line_count;
	INT orig_net_length;
	RECT rc;

	if (istart == iend && delta == 0)
		return;

	dc = GetDC(hwnd);
	if (es->font)
		old_font = SelectObject(dc, es->font);

	previous_line = NULL;
	current_line = es->first_line_def;

	/* Find starting line. istart must lie inside an existing line or
	 * at the end of buffer */
	do {
		if (istart < current_line->index + current_line->length || 
				current_line->ending == END_0)
			break;
		
		previous_line = current_line;	
		current_line = current_line->next;
		line_index++;
	} while (current_line);

	if (!current_line) /* Error occurred start is not inside previous buffer */
	{
		FIXME(" modification occurred outside buffer\n");
		return;
	}

	/* Remember start of modifications in order to calculate update region */
	nstart_line = line_index;
	nstart_index = current_line->index;

	/* We must start to reformat from the previous line since the modifications
	 * may have caused the line to wrap upwards. */
	if (!(es->style & ES_AUTOHSCROLL) && line_index > 0)
	{
		line_index--;
		current_line = previous_line;
	}
	start_line = current_line;

	fw = es->format_rect.right - es->format_rect.left;
	current_position = es->text + current_line->index;
	do {
		if (current_line != start_line)
		{
			if (!current_line || current_line->index + delta > current_position - es->text)
			{
				/* The buffer has been expanded, create a new line and 
				   insert it into the link list */
				LINEDEF *new_line = HeapAlloc(GetProcessHeap(), 0, sizeof(LINEDEF));
				new_line->next = previous_line->next;
				previous_line->next = new_line;
				current_line = new_line;
				es->line_count++;
			}
			else if (current_line->index + delta < current_position - es->text)
			{
				/* The previous line merged with this line so we delete this extra entry */
				previous_line->next = current_line->next;
				HeapFree(GetProcessHeap(), 0, current_line);
				current_line = previous_line->next;
				es->line_count--;
				continue;
			}
			else /* current_line->index + delta == current_position */
			{
				if (current_position - es->text > iend)
					break; /* We reached end of line modifications */
				/* else recalulate this line */
			}
		}

		current_line->index = current_position - es->text;
		orig_net_length = current_line->net_length;

		/* Find end of line */
		cp = current_position;
		while (*cp) {
			if ((*cp == '\r') && (*(cp + 1) == '\n'))
				break;
			cp++;
		}

		/* Mark type of line termination */
		if (!(*cp)) {
			current_line->ending = END_0;
			current_line->net_length = strlenW(current_position);
		} else if ((cp > current_position) && (*(cp - 1) == '\r')) {
			current_line->ending = END_SOFT;
			current_line->net_length = cp - current_position - 1;
		} else {
			current_line->ending = END_HARD;
			current_line->net_length = cp - current_position;
		}

		/* Calculate line width */
		current_line->width = (INT)LOWORD(GetTabbedTextExtentW(dc,
					current_position, current_line->net_length,
					es->tabs_count, es->tabs));

		/* FIXME: check here for lines that are too wide even in AUTOHSCROLL (> 32767 ???) */
		if ((!(es->style & ES_AUTOHSCROLL)) && (current_line->width > fw)) {
			INT next = 0;
			INT prev;
			do {
				prev = next;
				next = EDIT_CallWordBreakProc(es, current_position - es->text,
						prev + 1, current_line->net_length, WB_RIGHT);
				current_line->width = (INT)LOWORD(GetTabbedTextExtentW(dc,
							current_position, next, es->tabs_count, es->tabs));
			} while (current_line->width <= fw);
			if (!prev) { /* Didn't find a line break so force a break */
				next = 0;
				do {
					prev = next;
					next++;
					current_line->width = (INT)LOWORD(GetTabbedTextExtentW(dc,
								current_position, next, es->tabs_count, es->tabs));
				} while (current_line->width <= fw);
				if (!prev)
					prev = 1;
			}

			/* If the first line we are calculating, wrapped before istart, we must 
			 * adjust istart in order for this to be reflected in the update region. */
			if (current_line->index == nstart_index && istart > current_line->index + prev)
				istart = current_line->index + prev;
			/* else if we are updating the previous line before the first line we
			 * are re-calculating and it expanded */
			else if (current_line == start_line && 
					current_line->index != nstart_index && orig_net_length < prev)
			{ 
			  /* Line expanded due to an upwards line wrap so we must partially include
			   * previous line in update region */
				nstart_line = line_index;
				nstart_index = current_line->index;
				istart = current_line->index + orig_net_length;
			}

			current_line->net_length = prev;
			current_line->ending = END_WRAP;
			current_line->width = (INT)LOWORD(GetTabbedTextExtentW(dc, current_position,
					current_line->net_length, es->tabs_count, es->tabs));
		}


		/* Adjust length to include line termination */
		switch (current_line->ending) {
		case END_SOFT:
			current_line->length = current_line->net_length + 3;
			break;
		case END_HARD:
			current_line->length = current_line->net_length + 2;
			break;
		case END_WRAP:
		case END_0:
			current_line->length = current_line->net_length;
			break;
		}
		es->text_width = max(es->text_width, current_line->width);
		current_position += current_line->length;
		previous_line = current_line;
		current_line = current_line->next;
		line_index++;
	} while (previous_line->ending != END_0);

	/* Finish adjusting line indexes by delta or remove hanging lines */
	if (previous_line->ending == END_0)
	{
		LINEDEF *pnext = NULL;

		previous_line->next = NULL;
		while (current_line)
		{
			pnext = current_line->next;
			HeapFree(GetProcessHeap(), 0, current_line);
			current_line = pnext;
			es->line_count--;
		}
	}
	else
	{
		while (current_line)
		{
			current_line->index += delta;
			current_line = current_line->next;
		}
	}

	/* Calculate rest of modification rectangle */
	if (hrgn)
	{
		HRGN tmphrgn;
	   /* 
		* We calculate two rectangles. One for the first line which may have
		* an indent with respect to the format rect. The other is a format-width
		* rectangle that spans the rest of the lines that changed or moved.
		*/
		rc.top = es->format_rect.top + nstart_line * es->line_height - 
			(es->y_offset * es->line_height); /* Adjust for vertical scrollbar */
		rc.bottom = rc.top + es->line_height;
		rc.left = es->format_rect.left + (INT)LOWORD(GetTabbedTextExtentW(dc, 
					es->text + nstart_index, istart - nstart_index, 
					es->tabs_count, es->tabs)) - es->x_offset; /* Adjust for horz scroll */
		rc.right = es->format_rect.right;
		SetRectRgn(hrgn, rc.left, rc.top, rc.right, rc.bottom);

		rc.top = rc.bottom;
		rc.left = es->format_rect.left;
		rc.right = es->format_rect.right;
	   /* 
		* If lines were added or removed we must re-paint the remainder of the 
	    * lines since the remaining lines were either shifted up or down. 
		*/
		if (line_count < es->line_count) /* We added lines */
			rc.bottom = es->line_count * es->line_height;
		else if (line_count > es->line_count) /* We removed lines */
			rc.bottom = line_count * es->line_height;
		else
			rc.bottom = line_index * es->line_height;
		rc.bottom -= (es->y_offset * es->line_height); /* Adjust for vertical scrollbar */
		tmphrgn = CreateRectRgn(rc.left, rc.top, rc.right, rc.bottom);
		CombineRgn(hrgn, hrgn, tmphrgn, RGN_OR);
		DeleteObject(tmphrgn);
	}

	if (es->font)
		SelectObject(dc, old_font);

	ReleaseDC(hwnd, dc);
}

/*********************************************************************
 *
 *	EDIT_CalcLineWidth_SL
 *
 */
static void EDIT_CalcLineWidth_SL(HWND hwnd, EDITSTATE *es)
{
    es->text_width = SLOWORD(EDIT_EM_PosFromChar(hwnd, es, strlenW(es->text), FALSE));
}

/*********************************************************************
 *
 *	EDIT_CallWordBreakProc
 *
 *	Call appropriate WordBreakProc (internal or external).
 *
 *	Note: The "start" argument should always be an index referring
 *		to es->text.  The actual wordbreak proc might be
 *		16 bit, so we can't always pass any 32 bit LPSTR.
 *		Hence we assume that es->text is the buffer that holds
 *		the string under examination (we can decide this for ourselves).
 *
 */
/* ### start build ### */
extern WORD CALLBACK EDIT_CallTo16_word_lwww(EDITWORDBREAKPROC16,SEGPTR,WORD,WORD,WORD);
/* ### stop build ### */
static INT EDIT_CallWordBreakProc(EDITSTATE *es, INT start, INT index, INT count, INT action)
{
    INT ret, iWndsLocks;

    /* To avoid any deadlocks, all the locks on the window structures
       must be suspended before the control is passed to the application */
    iWndsLocks = WIN_SuspendWndsLock();

	if (es->word_break_proc16) {
	    HGLOBAL16 hglob16;
	    SEGPTR segptr;
	    INT countA;

	    countA = WideCharToMultiByte(CP_ACP, 0, es->text + start, count, NULL, 0, NULL, NULL);
	    hglob16 = GlobalAlloc16(GMEM_MOVEABLE | GMEM_ZEROINIT, countA);
	    segptr = K32WOWGlobalLock16(hglob16);
	    WideCharToMultiByte(CP_ACP, 0, es->text + start, count, MapSL(segptr), countA, NULL, NULL);
	    ret = (INT)EDIT_CallTo16_word_lwww(es->word_break_proc16,
						segptr, index, countA, action);
	    GlobalUnlock16(hglob16);
	    GlobalFree16(hglob16);
	}
	else if (es->word_break_proc)
        {
	    if(es->is_unicode)
	    {
		EDITWORDBREAKPROCW wbpW = (EDITWORDBREAKPROCW)es->word_break_proc;

		TRACE_(relay)("(UNICODE wordbrk=%p,str=%s,idx=%d,cnt=%d,act=%d)\n",
			es->word_break_proc, debugstr_wn(es->text + start, count), index, count, action);
		ret = wbpW(es->text + start, index, count, action);
	    }
	    else
	    {
		EDITWORDBREAKPROCA wbpA = (EDITWORDBREAKPROCA)es->word_break_proc;
		INT countA;
		CHAR *textA;

		countA = WideCharToMultiByte(CP_ACP, 0, es->text + start, count, NULL, 0, NULL, NULL);
		textA = HeapAlloc(GetProcessHeap(), 0, countA);
		WideCharToMultiByte(CP_ACP, 0, es->text + start, count, textA, countA, NULL, NULL);
		TRACE_(relay)("(ANSI wordbrk=%p,str=%s,idx=%d,cnt=%d,act=%d)\n",
			es->word_break_proc, debugstr_an(textA, countA), index, countA, action);
		ret = wbpA(textA, index, countA, action);
		HeapFree(GetProcessHeap(), 0, textA);
	    }
        }
	else
            ret = EDIT_WordBreakProc(es->text + start, index, count, action);

    WIN_RestoreWndsLock(iWndsLocks);
    return ret;
}


/*********************************************************************
 *
 *	EDIT_CharFromPos
 *
 *	Beware: This is not the function called on EM_CHARFROMPOS
 *		The position _can_ be outside the formatting / client
 *		rectangle
 *		The return value is only the character index
 *
 */
static INT EDIT_CharFromPos(HWND hwnd, EDITSTATE *es, INT x, INT y, LPBOOL after_wrap)
{
	INT index;
	HDC dc;
	HFONT old_font = 0;

	if (es->style & ES_MULTILINE) {
		INT line = (y - es->format_rect.top) / es->line_height + es->y_offset;
		INT line_index = 0;
		LINEDEF *line_def = es->first_line_def;
		INT low, high;
		while ((line > 0) && line_def->next) {
			line_index += line_def->length;
			line_def = line_def->next;
			line--;
		}
		x += es->x_offset - es->format_rect.left;
		if (x >= line_def->width) {
			if (after_wrap)
				*after_wrap = (line_def->ending == END_WRAP);
			return line_index + line_def->net_length;
		}
		if (x <= 0) {
			if (after_wrap)
				*after_wrap = FALSE;
			return line_index;
		}
		dc = GetDC(hwnd);
		if (es->font)
			old_font = SelectObject(dc, es->font);
                    low = line_index + 1;
                    high = line_index + line_def->net_length + 1;
                    while (low < high - 1)
                    {
                        INT mid = (low + high) / 2;
			if (LOWORD(GetTabbedTextExtentW(dc, es->text + line_index,mid - line_index, es->tabs_count, es->tabs)) > x) high = mid;
                        else low = mid;
                    }
                    index = low;

		if (after_wrap)
			*after_wrap = ((index == line_index + line_def->net_length) &&
							(line_def->ending == END_WRAP));
	} else {
		LPWSTR text;
		SIZE size;
		if (after_wrap)
			*after_wrap = FALSE;
		x -= es->format_rect.left;
		if (!x)
			return es->x_offset;
		text = EDIT_GetPasswordPointer_SL(es);
		dc = GetDC(hwnd);
		if (es->font)
			old_font = SelectObject(dc, es->font);
		if (x < 0)
                {
                    INT low = 0;
                    INT high = es->x_offset;
                    while (low < high - 1)
                    {
                        INT mid = (low + high) / 2;
                        GetTextExtentPoint32W( dc, text + mid,
                                               es->x_offset - mid, &size );
                        if (size.cx > -x) low = mid;
                        else high = mid;
                    }
                    index = low;
		}
                else
                {
                    INT low = es->x_offset;
                    INT high = strlenW(es->text) + 1;
                    while (low < high - 1)
                    {
                        INT mid = (low + high) / 2;
                        GetTextExtentPoint32W( dc, text + es->x_offset,
                                               mid - es->x_offset, &size );
                        if (size.cx > x) high = mid;
                        else low = mid;
                    }
                    index = low;
		}
		if (es->style & ES_PASSWORD)
			HeapFree(GetProcessHeap(), 0, text);
	}
	if (es->font)
		SelectObject(dc, old_font);
	ReleaseDC(hwnd, dc);
	return index;
}


/*********************************************************************
 *
 *	EDIT_ConfinePoint
 *
 *	adjusts the point to be within the formatting rectangle
 *	(so CharFromPos returns the nearest _visible_ character)
 *
 */
static void EDIT_ConfinePoint(EDITSTATE *es, LPINT x, LPINT y)
{
	*x = min(max(*x, es->format_rect.left), es->format_rect.right - 1);
	*y = min(max(*y, es->format_rect.top), es->format_rect.bottom - 1);
}


/*********************************************************************
 *
 *	EDIT_GetLineRect
 *
 *	Calculates the bounding rectangle for a line from a starting
 *	column to an ending column.
 *
 */
static void EDIT_GetLineRect(HWND hwnd, EDITSTATE *es, INT line, INT scol, INT ecol, LPRECT rc)
{
	INT line_index =  EDIT_EM_LineIndex(es, line);

	if (es->style & ES_MULTILINE)
		rc->top = es->format_rect.top + (line - es->y_offset) * es->line_height;
	else
		rc->top = es->format_rect.top;
	rc->bottom = rc->top + es->line_height;
	rc->left = (scol == 0) ? es->format_rect.left : SLOWORD(EDIT_EM_PosFromChar(hwnd, es, line_index + scol, TRUE));
	rc->right = (ecol == -1) ? es->format_rect.right : SLOWORD(EDIT_EM_PosFromChar(hwnd, es, line_index + ecol, TRUE));
}


/*********************************************************************
 *
 *	EDIT_GetPasswordPointer_SL
 *
 *	note: caller should free the (optionally) allocated buffer
 *
 */
static LPWSTR EDIT_GetPasswordPointer_SL(EDITSTATE *es)
{
	if (es->style & ES_PASSWORD) {
		INT len = strlenW(es->text);
		LPWSTR text = HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR));
		text[len] = '\0';
		while(len) text[--len] = es->password_char;
		return text;
	} else
		return es->text;
}


/*********************************************************************
 *
 *	EDIT_LockBuffer
 *
 *	This acts as a LOCAL_Lock(), but it locks only once.  This way
 *	you can call it whenever you like, without unlocking.
 *
 */
static void EDIT_LockBuffer(HWND hwnd, EDITSTATE *es)
{
    HINSTANCE hInstance = GetWindowLongA( hwnd, GWL_HINSTANCE );
	if (!es) {
		ERR("no EDITSTATE ... please report\n");
		return;
	}
	if (!es->text) {
	    CHAR *textA = NULL;
	    UINT countA = 0;
	    BOOL _16bit = FALSE;

	    if(es->hloc32W)
	    {
		if(es->hloc32A)
		{
		    TRACE("Synchronizing with 32-bit ANSI buffer\n");
		    textA = LocalLock(es->hloc32A);
		    countA = strlen(textA) + 1;
		}
		else if(es->hloc16)
		{
		    TRACE("Synchronizing with 16-bit ANSI buffer\n");
		    textA = LOCAL_Lock(hInstance, es->hloc16);
		    countA = strlen(textA) + 1;
		    _16bit = TRUE;
		}
	    }
	    else {
		ERR("no buffer ... please report\n");
		return;
	    }

	    if(textA)
	    {
		HLOCAL hloc32W_new;
		UINT countW_new = MultiByteToWideChar(CP_ACP, 0, textA, countA, NULL, 0);
		TRACE("%d bytes translated to %d WCHARs\n", countA, countW_new);
		if(countW_new > es->buffer_size + 1)
		{
		    UINT alloc_size = ROUND_TO_GROW(countW_new * sizeof(WCHAR));
		    TRACE("Resizing 32-bit UNICODE buffer from %d+1 to %d WCHARs\n", es->buffer_size, countW_new);
		    hloc32W_new = LocalReAlloc(es->hloc32W, alloc_size, LMEM_MOVEABLE | LMEM_ZEROINIT);
		    if(hloc32W_new)
		    {
			es->hloc32W = hloc32W_new;
			es->buffer_size = LocalSize(hloc32W_new)/sizeof(WCHAR) - 1;
			TRACE("Real new size %d+1 WCHARs\n", es->buffer_size);
		    }
		    else
			WARN("FAILED! Will synchronize partially\n");
		}
	    }

	    /*TRACE("Locking 32-bit UNICODE buffer\n");*/
	    es->text = LocalLock(es->hloc32W);

	    if(textA)
	    {
		MultiByteToWideChar(CP_ACP, 0, textA, countA, es->text, es->buffer_size + 1);
		if(_16bit)
		    LOCAL_Unlock(hInstance, es->hloc16);
		else
		    LocalUnlock(es->hloc32A);
	    }
	}
	es->lock_count++;
}


/*********************************************************************
 *
 *	EDIT_SL_InvalidateText
 *
 *	Called from EDIT_InvalidateText().
 *	Does the job for single-line controls only.
 *
 */
static void EDIT_SL_InvalidateText(HWND hwnd, EDITSTATE *es, INT start, INT end)
{
	RECT line_rect;
	RECT rc;

	EDIT_GetLineRect(hwnd, es, 0, start, end, &line_rect);
	if (IntersectRect(&rc, &line_rect, &es->format_rect))
		EDIT_UpdateText(hwnd, es, &rc, FALSE);
}


/*********************************************************************
 *
 *	EDIT_ML_InvalidateText
 *
 *	Called from EDIT_InvalidateText().
 *	Does the job for multi-line controls only.
 *
 */
static void EDIT_ML_InvalidateText(HWND hwnd, EDITSTATE *es, INT start, INT end)
{
	INT vlc = (es->format_rect.bottom - es->format_rect.top) / es->line_height;
	INT sl = EDIT_EM_LineFromChar(es, start);
	INT el = EDIT_EM_LineFromChar(es, end);
	INT sc;
	INT ec;
	RECT rc1;
	RECT rcWnd;
	RECT rcLine;
	RECT rcUpdate;
	INT l;

	if ((el < es->y_offset) || (sl > es->y_offset + vlc))
		return;

	sc = start - EDIT_EM_LineIndex(es, sl);
	ec = end - EDIT_EM_LineIndex(es, el);
	if (sl < es->y_offset) {
		sl = es->y_offset;
		sc = 0;
	}
	if (el > es->y_offset + vlc) {
		el = es->y_offset + vlc;
		ec = EDIT_EM_LineLength(es, EDIT_EM_LineIndex(es, el));
	}
	GetClientRect(hwnd, &rc1);
	IntersectRect(&rcWnd, &rc1, &es->format_rect);
	if (sl == el) {
		EDIT_GetLineRect(hwnd, es, sl, sc, ec, &rcLine);
		if (IntersectRect(&rcUpdate, &rcWnd, &rcLine))
			EDIT_UpdateText(hwnd, es, &rcUpdate, FALSE);
	} else {
		EDIT_GetLineRect(hwnd, es, sl, sc,
				EDIT_EM_LineLength(es,
					EDIT_EM_LineIndex(es, sl)),
				&rcLine);
		if (IntersectRect(&rcUpdate, &rcWnd, &rcLine))
			EDIT_UpdateText(hwnd, es, &rcUpdate, FALSE);
		for (l = sl + 1 ; l < el ; l++) {
			EDIT_GetLineRect(hwnd, es, l, 0,
				EDIT_EM_LineLength(es,
					EDIT_EM_LineIndex(es, l)),
				&rcLine);
			if (IntersectRect(&rcUpdate, &rcWnd, &rcLine))
				EDIT_UpdateText(hwnd, es, &rcUpdate, FALSE);
		}
		EDIT_GetLineRect(hwnd, es, el, 0, ec, &rcLine);
		if (IntersectRect(&rcUpdate, &rcWnd, &rcLine))
			EDIT_UpdateText(hwnd, es, &rcUpdate, FALSE);
	}
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
static void EDIT_InvalidateText(HWND hwnd, EDITSTATE *es, INT start, INT end)
{
	if (end == start)
		return;

	if (end == -1)
		end = strlenW(es->text);

	ORDER_INT(start, end);

	if (es->style & ES_MULTILINE)
		EDIT_ML_InvalidateText(hwnd, es, start, end);
	else
		EDIT_SL_InvalidateText(hwnd, es, start, end);
}


/*********************************************************************
 *
 *	EDIT_MakeFit
 *
 *	Try to fit size + 1 characters in the buffer. Constrain to limits.
 *
 */
static BOOL EDIT_MakeFit(HWND hwnd, EDITSTATE *es, UINT size)
{
	HLOCAL hNew32W;

	if (size <= es->buffer_size)
		return TRUE;
	if (size > es->buffer_limit) {
		EDIT_NOTIFY_PARENT(hwnd, es, EN_MAXTEXT, "EN_MAXTEXT");
		return FALSE;
	}
	if (size > es->buffer_limit)
		size = es->buffer_limit;

	TRACE("trying to ReAlloc to %d+1 characters\n", size);

	/* Force edit to unlock it's buffer. es->text now NULL */
	EDIT_UnlockBuffer(hwnd, es, TRUE);

	if (es->hloc32W) {
	    UINT alloc_size = ROUND_TO_GROW((size + 1) * sizeof(WCHAR));
	    if ((hNew32W = LocalReAlloc(es->hloc32W, alloc_size, LMEM_MOVEABLE | LMEM_ZEROINIT))) {
		TRACE("Old 32 bit handle %08x, new handle %08x\n", es->hloc32W, hNew32W);
		es->hloc32W = hNew32W;
		es->buffer_size = LocalSize(hNew32W)/sizeof(WCHAR) - 1;
	    }
	}

	EDIT_LockBuffer(hwnd, es);

	if (es->buffer_size < size) {
		WARN("FAILED !  We now have %d+1\n", es->buffer_size);
		EDIT_NOTIFY_PARENT(hwnd, es, EN_ERRSPACE, "EN_ERRSPACE");
		return FALSE;
	} else {
		TRACE("We now have %d+1\n", es->buffer_size);
		return TRUE;
	}
}


/*********************************************************************
 *
 *	EDIT_MakeUndoFit
 *
 *	Try to fit size + 1 bytes in the undo buffer.
 *
 */
static BOOL EDIT_MakeUndoFit(EDITSTATE *es, UINT size)
{
	UINT alloc_size;

	if (size <= es->undo_buffer_size)
		return TRUE;

	TRACE("trying to ReAlloc to %d+1\n", size);

	alloc_size = ROUND_TO_GROW((size + 1) * sizeof(WCHAR));
	if ((es->undo_text = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, es->undo_text, alloc_size))) {
		es->undo_buffer_size = alloc_size/sizeof(WCHAR);
		return TRUE;
	}
	else
	{
		WARN("FAILED !  We now have %d+1\n", es->undo_buffer_size);
		return FALSE;
	}
}


/*********************************************************************
 *
 *	EDIT_MoveBackward
 *
 */
static void EDIT_MoveBackward(HWND hwnd, EDITSTATE *es, BOOL extend)
{
	INT e = es->selection_end;

	if (e) {
		e--;
		if ((es->style & ES_MULTILINE) && e &&
				(es->text[e - 1] == '\r') && (es->text[e] == '\n')) {
			e--;
			if (e && (es->text[e - 1] == '\r'))
				e--;
		}
	}
	EDIT_EM_SetSel(hwnd, es, extend ? es->selection_start : e, e, FALSE);
	EDIT_EM_ScrollCaret(hwnd, es);
}


/*********************************************************************
 *
 *	EDIT_MoveDown_ML
 *
 *	Only for multi line controls
 *	Move the caret one line down, on a column with the nearest
 *	x coordinate on the screen (might be a different column).
 *
 */
static void EDIT_MoveDown_ML(HWND hwnd, EDITSTATE *es, BOOL extend)
{
	INT s = es->selection_start;
	INT e = es->selection_end;
	BOOL after_wrap = (es->flags & EF_AFTER_WRAP);
	LRESULT pos = EDIT_EM_PosFromChar(hwnd, es, e, after_wrap);
	INT x = SLOWORD(pos);
	INT y = SHIWORD(pos);

	e = EDIT_CharFromPos(hwnd, es, x, y + es->line_height, &after_wrap);
	if (!extend)
		s = e;
	EDIT_EM_SetSel(hwnd, es, s, e, after_wrap);
	EDIT_EM_ScrollCaret(hwnd, es);
}


/*********************************************************************
 *
 *	EDIT_MoveEnd
 *
 */
static void EDIT_MoveEnd(HWND hwnd, EDITSTATE *es, BOOL extend)
{
	BOOL after_wrap = FALSE;
	INT e;

	/* Pass a high value in x to make sure of receiving the end of the line */
	if (es->style & ES_MULTILINE)
		e = EDIT_CharFromPos(hwnd, es, 0x3fffffff,
			HIWORD(EDIT_EM_PosFromChar(hwnd, es, es->selection_end, es->flags & EF_AFTER_WRAP)), &after_wrap);
	else
		e = strlenW(es->text);
	EDIT_EM_SetSel(hwnd, es, extend ? es->selection_start : e, e, after_wrap);
	EDIT_EM_ScrollCaret(hwnd, es);
}


/*********************************************************************
 *
 *	EDIT_MoveForward
 *
 */
static void EDIT_MoveForward(HWND hwnd, EDITSTATE *es, BOOL extend)
{
	INT e = es->selection_end;

	if (es->text[e]) {
		e++;
		if ((es->style & ES_MULTILINE) && (es->text[e - 1] == '\r')) {
			if (es->text[e] == '\n')
				e++;
			else if ((es->text[e] == '\r') && (es->text[e + 1] == '\n'))
				e += 2;
		}
	}
	EDIT_EM_SetSel(hwnd, es, extend ? es->selection_start : e, e, FALSE);
	EDIT_EM_ScrollCaret(hwnd, es);
}


/*********************************************************************
 *
 *	EDIT_MoveHome
 *
 *	Home key: move to beginning of line.
 *
 */
static void EDIT_MoveHome(HWND hwnd, EDITSTATE *es, BOOL extend)
{
	INT e;

	/* Pass the x_offset in x to make sure of receiving the first position of the line */
	if (es->style & ES_MULTILINE)
		e = EDIT_CharFromPos(hwnd, es, -es->x_offset,
			HIWORD(EDIT_EM_PosFromChar(hwnd, es, es->selection_end, es->flags & EF_AFTER_WRAP)), NULL);
	else
		e = 0;
	EDIT_EM_SetSel(hwnd, es, extend ? es->selection_start : e, e, FALSE);
	EDIT_EM_ScrollCaret(hwnd, es);
}


/*********************************************************************
 *
 *	EDIT_MovePageDown_ML
 *
 *	Only for multi line controls
 *	Move the caret one page down, on a column with the nearest
 *	x coordinate on the screen (might be a different column).
 *
 */
static void EDIT_MovePageDown_ML(HWND hwnd, EDITSTATE *es, BOOL extend)
{
	INT s = es->selection_start;
	INT e = es->selection_end;
	BOOL after_wrap = (es->flags & EF_AFTER_WRAP);
	LRESULT pos = EDIT_EM_PosFromChar(hwnd, es, e, after_wrap);
	INT x = SLOWORD(pos);
	INT y = SHIWORD(pos);

	e = EDIT_CharFromPos(hwnd, es, x,
		y + (es->format_rect.bottom - es->format_rect.top),
		&after_wrap);
	if (!extend)
		s = e;
	EDIT_EM_SetSel(hwnd, es, s, e, after_wrap);
	EDIT_EM_ScrollCaret(hwnd, es);
}


/*********************************************************************
 *
 *	EDIT_MovePageUp_ML
 *
 *	Only for multi line controls
 *	Move the caret one page up, on a column with the nearest
 *	x coordinate on the screen (might be a different column).
 *
 */
static void EDIT_MovePageUp_ML(HWND hwnd, EDITSTATE *es, BOOL extend)
{
	INT s = es->selection_start;
	INT e = es->selection_end;
	BOOL after_wrap = (es->flags & EF_AFTER_WRAP);
	LRESULT pos = EDIT_EM_PosFromChar(hwnd, es, e, after_wrap);
	INT x = SLOWORD(pos);
	INT y = SHIWORD(pos);

	e = EDIT_CharFromPos(hwnd, es, x,
		y - (es->format_rect.bottom - es->format_rect.top),
		&after_wrap);
	if (!extend)
		s = e;
	EDIT_EM_SetSel(hwnd, es, s, e, after_wrap);
	EDIT_EM_ScrollCaret(hwnd, es);
}


/*********************************************************************
 *
 *	EDIT_MoveUp_ML
 *
 *	Only for multi line controls
 *	Move the caret one line up, on a column with the nearest
 *	x coordinate on the screen (might be a different column).
 *
 */ 
static void EDIT_MoveUp_ML(HWND hwnd, EDITSTATE *es, BOOL extend)
{
	INT s = es->selection_start;
	INT e = es->selection_end;
	BOOL after_wrap = (es->flags & EF_AFTER_WRAP);
	LRESULT pos = EDIT_EM_PosFromChar(hwnd, es, e, after_wrap);
	INT x = SLOWORD(pos);
	INT y = SHIWORD(pos);

	e = EDIT_CharFromPos(hwnd, es, x, y - es->line_height, &after_wrap);
	if (!extend)
		s = e;
	EDIT_EM_SetSel(hwnd, es, s, e, after_wrap);
	EDIT_EM_ScrollCaret(hwnd, es);
}


/*********************************************************************
 *
 *	EDIT_MoveWordBackward
 *
 */
static void EDIT_MoveWordBackward(HWND hwnd, EDITSTATE *es, BOOL extend)
{
	INT s = es->selection_start;
	INT e = es->selection_end;
	INT l;
	INT ll;
	INT li;

	l = EDIT_EM_LineFromChar(es, e);
	ll = EDIT_EM_LineLength(es, e);
	li = EDIT_EM_LineIndex(es, l);
	if (e - li == 0) {
		if (l) {
			li = EDIT_EM_LineIndex(es, l - 1);
			e = li + EDIT_EM_LineLength(es, li);
		}
	} else {
		e = li + (INT)EDIT_CallWordBreakProc(es,
				li, e - li, ll, WB_LEFT);
	}
	if (!extend)
		s = e;
	EDIT_EM_SetSel(hwnd, es, s, e, FALSE);
	EDIT_EM_ScrollCaret(hwnd, es);
}


/*********************************************************************
 *
 *	EDIT_MoveWordForward
 *
 */
static void EDIT_MoveWordForward(HWND hwnd, EDITSTATE *es, BOOL extend)
{
	INT s = es->selection_start;
	INT e = es->selection_end;
	INT l;
	INT ll;
	INT li;

	l = EDIT_EM_LineFromChar(es, e);
	ll = EDIT_EM_LineLength(es, e);
	li = EDIT_EM_LineIndex(es, l);
	if (e - li == ll) {
		if ((es->style & ES_MULTILINE) && (l != es->line_count - 1))
			e = EDIT_EM_LineIndex(es, l + 1);
	} else {
		e = li + EDIT_CallWordBreakProc(es,
				li, e - li + 1, ll, WB_RIGHT);
	}
	if (!extend)
		s = e;
	EDIT_EM_SetSel(hwnd, es, s, e, FALSE);
	EDIT_EM_ScrollCaret(hwnd, es);
}


/*********************************************************************
 *
 *	EDIT_PaintLine
 *
 */
static void EDIT_PaintLine(HWND hwnd, EDITSTATE *es, HDC dc, INT line, BOOL rev)
{
	INT s = es->selection_start;
	INT e = es->selection_end;
	INT li;
	INT ll;
	INT x;
	INT y;
	LRESULT pos;

	if (es->style & ES_MULTILINE) {
		INT vlc = (es->format_rect.bottom - es->format_rect.top) / es->line_height;
		if ((line < es->y_offset) || (line > es->y_offset + vlc) || (line >= es->line_count))
			return;
	} else if (line)
		return;

	TRACE("line=%d\n", line);

	pos = EDIT_EM_PosFromChar(hwnd, es, EDIT_EM_LineIndex(es, line), FALSE);
	x = SLOWORD(pos);
	y = SHIWORD(pos);
	li = EDIT_EM_LineIndex(es, line);
	ll = EDIT_EM_LineLength(es, li);
	s = es->selection_start;
	e = es->selection_end;
	ORDER_INT(s, e);
	s = min(li + ll, max(li, s));
	e = min(li + ll, max(li, e));
	if (rev && (s != e) &&
			((es->flags & EF_FOCUSED) || (es->style & ES_NOHIDESEL))) {
		x += EDIT_PaintText(es, dc, x, y, line, 0, s - li, FALSE);
		x += EDIT_PaintText(es, dc, x, y, line, s - li, e - s, TRUE);
		x += EDIT_PaintText(es, dc, x, y, line, e - li, li + ll - e, FALSE);
	} else
		x += EDIT_PaintText(es, dc, x, y, line, 0, ll, FALSE);
}


/*********************************************************************
 *
 *	EDIT_PaintText
 *
 */
static INT EDIT_PaintText(EDITSTATE *es, HDC dc, INT x, INT y, INT line, INT col, INT count, BOOL rev)
{
	COLORREF BkColor;
	COLORREF TextColor;
	INT ret;
	INT li;
	INT BkMode;
	SIZE size;

	if (!count)
		return 0;
	BkMode = GetBkMode(dc);
	BkColor = GetBkColor(dc);
	TextColor = GetTextColor(dc);
	if (rev) {
		SetBkColor(dc, GetSysColor(COLOR_HIGHLIGHT));
		SetTextColor(dc, GetSysColor(COLOR_HIGHLIGHTTEXT));
		SetBkMode( dc, OPAQUE);
	}
	li = EDIT_EM_LineIndex(es, line);
	if (es->style & ES_MULTILINE) {
		ret = (INT)LOWORD(TabbedTextOutW(dc, x, y, es->text + li + col, count,
					es->tabs_count, es->tabs, es->format_rect.left - es->x_offset));
	} else {
		LPWSTR text = EDIT_GetPasswordPointer_SL(es);
		TextOutW(dc, x, y, text + li + col, count);
		GetTextExtentPoint32W(dc, text + li + col, count, &size);
		ret = size.cx;
		if (es->style & ES_PASSWORD)
			HeapFree(GetProcessHeap(), 0, text);
	}
	if (rev) {
		SetBkColor(dc, BkColor);
		SetTextColor(dc, TextColor);
		SetBkMode( dc, BkMode);
	}
	return ret;
}


/*********************************************************************
 *
 *	EDIT_SetCaretPos
 *
 */
static void EDIT_SetCaretPos(HWND hwnd, EDITSTATE *es, INT pos,
			     BOOL after_wrap)
{
	LRESULT res = EDIT_EM_PosFromChar(hwnd, es, pos, after_wrap);
	SetCaretPos(SLOWORD(res), SHIWORD(res));
}


/*********************************************************************
 *
 *	EDIT_SetRectNP
 *
 *	note:	this is not (exactly) the handler called on EM_SETRECTNP
 *		it is also used to set the rect of a single line control
 *
 */
static void EDIT_SetRectNP(HWND hwnd, EDITSTATE *es, LPRECT rc)
{
	CopyRect(&es->format_rect, rc);
	if (es->style & WS_BORDER) {
		INT bw = GetSystemMetrics(SM_CXBORDER) + 1;
		if(TWEAK_WineLook == WIN31_LOOK)
			bw += 2;
		es->format_rect.left += bw;
		es->format_rect.top += bw;
		es->format_rect.right -= bw;
		es->format_rect.bottom -= bw;
	}
	es->format_rect.left += es->left_margin;
	es->format_rect.right -= es->right_margin;
	es->format_rect.right = max(es->format_rect.right, es->format_rect.left + es->char_width);
	if (es->style & ES_MULTILINE)
	{
	    INT fw, vlc, max_x_offset, max_y_offset;

	    vlc = (es->format_rect.bottom - es->format_rect.top) / es->line_height;
	    es->format_rect.bottom = es->format_rect.top + max(1, vlc) * es->line_height;

	    /* correct es->x_offset */
	    fw = es->format_rect.right - es->format_rect.left;
	    max_x_offset = es->text_width - fw;
	    if(max_x_offset < 0) max_x_offset = 0;
	    if(es->x_offset > max_x_offset)
		es->x_offset = max_x_offset;

	    /* correct es->y_offset */
	    max_y_offset = es->line_count - vlc;
	    if(max_y_offset < 0) max_y_offset = 0;
	    if(es->y_offset > max_y_offset)
		es->y_offset = max_y_offset;

	    /* force scroll info update */
	    EDIT_UpdateScrollInfo(hwnd, es);
	}
	else
	/* Windows doesn't care to fix text placement for SL controls */
		es->format_rect.bottom = es->format_rect.top + es->line_height;

	if ((es->style & ES_MULTILINE) && !(es->style & ES_AUTOHSCROLL))
		EDIT_BuildLineDefs_ML(hwnd, es, 0, strlenW(es->text), 0, (HRGN)0);
}


/*********************************************************************
 *
 *	EDIT_UnlockBuffer
 *
 */
static void EDIT_UnlockBuffer(HWND hwnd, EDITSTATE *es, BOOL force)
{
    HINSTANCE hInstance = GetWindowLongA( hwnd, GWL_HINSTANCE );

    /* Edit window might be already destroyed */
    if(!IsWindow(hwnd))
    {
	WARN("edit hwnd %04x already destroyed\n", hwnd);
	return;
    }

	if (!es) {
		ERR("no EDITSTATE ... please report\n");
		return;
	}
	if (!es->lock_count) {
		ERR("lock_count == 0 ... please report\n");
		return;
	}
	if (!es->text) {
		ERR("es->text == 0 ... please report\n");
		return;
	}

	if (force || (es->lock_count == 1)) {
	    if (es->hloc32W) {
		CHAR *textA = NULL;
		BOOL _16bit = FALSE;
		UINT countA = 0;
		UINT countW = strlenW(es->text) + 1;

		if(es->hloc32A)
		{
		    UINT countA_new = WideCharToMultiByte(CP_ACP, 0, es->text, countW, NULL, 0, NULL, NULL);
		    TRACE("Synchronizing with 32-bit ANSI buffer\n");
		    TRACE("%d WCHARs translated to %d bytes\n", countW, countA_new);
		    countA = LocalSize(es->hloc32A);
		    if(countA_new > countA)
		    {
			HLOCAL hloc32A_new;
			UINT alloc_size = ROUND_TO_GROW(countA_new);
			TRACE("Resizing 32-bit ANSI buffer from %d to %d bytes\n", countA, alloc_size);
			hloc32A_new = LocalReAlloc(es->hloc32A, alloc_size, LMEM_MOVEABLE | LMEM_ZEROINIT);
			if(hloc32A_new)
			{
			    es->hloc32A = hloc32A_new;
			    countA = LocalSize(hloc32A_new);
			    TRACE("Real new size %d bytes\n", countA);
			}
			else
			    WARN("FAILED! Will synchronize partially\n");
		    }
		    textA = LocalLock(es->hloc32A);
		}
		else if(es->hloc16)
		{
		    UINT countA_new = WideCharToMultiByte(CP_ACP, 0, es->text, countW, NULL, 0, NULL, NULL);
		    TRACE("Synchronizing with 16-bit ANSI buffer\n");
		    TRACE("%d WCHARs translated to %d bytes\n", countW, countA_new);
		    countA = LOCAL_Size(hInstance, es->hloc16);
		    if(countA_new > countA)
		    {
			HLOCAL16 hloc16_new;
			UINT alloc_size = ROUND_TO_GROW(countA_new);
			TRACE("Resizing 16-bit ANSI buffer from %d to %d bytes\n", countA, alloc_size);
			hloc16_new = LOCAL_ReAlloc(hInstance, es->hloc16, alloc_size, LMEM_MOVEABLE | LMEM_ZEROINIT);
			if(hloc16_new)
			{
			    es->hloc16 = hloc16_new;
			    countA = LOCAL_Size(hInstance, hloc16_new);
			    TRACE("Real new size %d bytes\n", countA);
			}
			else
			    WARN("FAILED! Will synchronize partially\n");
		    }
		    textA = LOCAL_Lock(hInstance, es->hloc16);
		    _16bit = TRUE;
		}

		if(textA)
		{
		    WideCharToMultiByte(CP_ACP, 0, es->text, countW, textA, countA, NULL, NULL);
		    if(_16bit)
			LOCAL_Unlock(hInstance, es->hloc16);
		    else
			LocalUnlock(es->hloc32A);
		}

		LocalUnlock(es->hloc32W);
		es->text = NULL;
	    }
	    else {
		ERR("no buffer ... please report\n");
		return;
	    }
	}
	es->lock_count--;
}


/*********************************************************************
 *
 *	EDIT_UpdateScrollInfo
 *
 */
static void EDIT_UpdateScrollInfo(HWND hwnd, EDITSTATE *es)
{
    if ((es->style & WS_VSCROLL) && !(es->flags & EF_VSCROLL_TRACK))
    {
	SCROLLINFO si;
	si.cbSize	= sizeof(SCROLLINFO);
	si.fMask	= SIF_PAGE | SIF_POS | SIF_RANGE | SIF_DISABLENOSCROLL;
	si.nMin		= 0;
	si.nMax		= es->line_count - 1;
	si.nPage	= (es->format_rect.bottom - es->format_rect.top) / es->line_height;
	si.nPos		= es->y_offset;
	TRACE("SB_VERT, nMin=%d, nMax=%d, nPage=%d, nPos=%d\n",
		si.nMin, si.nMax, si.nPage, si.nPos);
	SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
    }

    if ((es->style & WS_HSCROLL) && !(es->flags & EF_HSCROLL_TRACK))
    {
	SCROLLINFO si;
	si.cbSize	= sizeof(SCROLLINFO);
	si.fMask	= SIF_PAGE | SIF_POS | SIF_RANGE | SIF_DISABLENOSCROLL;
	si.nMin		= 0;
	si.nMax		= es->text_width - 1;
	si.nPage	= es->format_rect.right - es->format_rect.left;
	si.nPos		= es->x_offset;
	TRACE("SB_HORZ, nMin=%d, nMax=%d, nPage=%d, nPos=%d\n",
		si.nMin, si.nMax, si.nPage, si.nPos);
	SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);
    }
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
static INT CALLBACK EDIT_WordBreakProc(LPWSTR s, INT index, INT count, INT action)
{
	INT ret = 0;

	TRACE("s=%p, index=%d, count=%d, action=%d\n", s, index, count, action);

	if(!s) return 0;

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
		ERR("unknown action code, please report !\n");
		break;
	}
	return ret;
}


/*********************************************************************
 *
 *	EM_CHARFROMPOS
 *
 *      returns line number (not index) in high-order word of result.
 *      NB : Q137805 is unclear about this. POINT * pointer in lParam apply 
 *      to Richedit, not to the edit control. Original documentation is valid.
 *	FIXME: do the specs mean to return -1 if outside client area or
 *		if outside formatting rectangle ???
 *
 */
static LRESULT EDIT_EM_CharFromPos(HWND hwnd, EDITSTATE *es, INT x, INT y)
{
	POINT pt;
	RECT rc;
	INT index;

	pt.x = x;
	pt.y = y;
	GetClientRect(hwnd, &rc);
	if (!PtInRect(&rc, pt))
		return -1;

	index = EDIT_CharFromPos(hwnd, es, x, y, NULL);
	return MAKELONG(index, EDIT_EM_LineFromChar(es, index));
}


/*********************************************************************
 *
 *	EM_FMTLINES
 *
 * Enable or disable soft breaks.
 */
static BOOL EDIT_EM_FmtLines(EDITSTATE *es, BOOL add_eol)
{
	es->flags &= ~EF_USE_SOFTBRK;
	if (add_eol) {
		es->flags |= EF_USE_SOFTBRK;
		FIXME("soft break enabled, not implemented\n");
	}
	return add_eol;
}


/*********************************************************************
 *
 *	EM_GETHANDLE
 *
 *	Hopefully this won't fire back at us.
 *	We always start with a fixed buffer in the local heap.
 *	Despite of the documentation says that the local heap is used
 *	only if DS_LOCALEDIT flag is set, NT and 2000 always allocate
 *	buffer on the local heap.
 *
 */
static HLOCAL EDIT_EM_GetHandle(EDITSTATE *es)
{
	HLOCAL hLocal;

	if (!(es->style & ES_MULTILINE))
		return 0;

	if(es->is_unicode)
	    hLocal = es->hloc32W;
	else
	{
	    if(!es->hloc32A)
	    {
		CHAR *textA;
		UINT countA, alloc_size;
		TRACE("Allocating 32-bit ANSI alias buffer\n");
		countA = WideCharToMultiByte(CP_ACP, 0, es->text, -1, NULL, 0, NULL, NULL);
		alloc_size = ROUND_TO_GROW(countA);
		if(!(es->hloc32A = LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT, alloc_size)))
		{
		    ERR("Could not allocate %d bytes for 32-bit ANSI alias buffer\n", alloc_size);
		    return 0;
		}
		textA = LocalLock(es->hloc32A);
		WideCharToMultiByte(CP_ACP, 0, es->text, -1, textA, countA, NULL, NULL);
		LocalUnlock(es->hloc32A);
	    }
	    hLocal = es->hloc32A;
	}

	TRACE("Returning %04X, LocalSize() = %d\n", hLocal, LocalSize(hLocal));
	return hLocal;
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
static HLOCAL16 EDIT_EM_GetHandle16(HWND hwnd, EDITSTATE *es)
{
    HINSTANCE hInstance = GetWindowLongA( hwnd, GWL_HINSTANCE );
	CHAR *textA;
	UINT countA, alloc_size;

	if (!(es->style & ES_MULTILINE))
		return 0;

	if (es->hloc16)
		return es->hloc16;

	if (!LOCAL_HeapSize(hInstance)) {
		if (!LocalInit16(hInstance, 0,
				GlobalSize16(hInstance))) {
			ERR("could not initialize local heap\n");
			return 0;
		}
		TRACE("local heap initialized\n");
	}

	countA = WideCharToMultiByte(CP_ACP, 0, es->text, -1, NULL, 0, NULL, NULL);
	alloc_size = ROUND_TO_GROW(countA);

	TRACE("Allocating 16-bit ANSI alias buffer\n");
	if (!(es->hloc16 = LOCAL_Alloc(hInstance, LMEM_MOVEABLE | LMEM_ZEROINIT, alloc_size))) {
		ERR("could not allocate new 16 bit buffer\n");
		return 0;
	}

	if (!(textA = (LPSTR)LOCAL_Lock(hInstance, es->hloc16))) {
		ERR("could not lock new 16 bit buffer\n");
		LOCAL_Free(hInstance, es->hloc16);
		es->hloc16 = 0;
		return 0;
	}

	WideCharToMultiByte(CP_ACP, 0, es->text, -1, textA, countA, NULL, NULL);
	LOCAL_Unlock(hInstance, es->hloc16);

	TRACE("Returning %04X, LocalSize() = %d\n", es->hloc16, LOCAL_Size(hInstance, es->hloc16));
	return es->hloc16;
}


/*********************************************************************
 *
 *	EM_GETLINE
 *
 */
static INT EDIT_EM_GetLine(EDITSTATE *es, INT line, LPARAM lParam, BOOL unicode)
{
	LPWSTR src;
	INT line_len, dst_len;
	INT i;

	if (es->style & ES_MULTILINE) {
		if (line >= es->line_count)
			return 0;
	} else
		line = 0;
	i = EDIT_EM_LineIndex(es, line);
	src = es->text + i;
	line_len = EDIT_EM_LineLength(es, i);
	dst_len = *(WORD *)lParam;
	if(unicode)
	{
	    LPWSTR dst = (LPWSTR)lParam;
	    if(dst_len <= line_len)
	    {
		memcpy(dst, src, dst_len * sizeof(WCHAR));
		return dst_len;
	    }
	    else /* Append 0 if enough space */
	    {
		memcpy(dst, src, line_len * sizeof(WCHAR));
		dst[line_len] = 0;
		return line_len;
	    }
	}
	else
	{
	    LPSTR dst = (LPSTR)lParam;
	    INT ret;
	    ret = WideCharToMultiByte(CP_ACP, 0, src, line_len, dst, dst_len, NULL, NULL);
	    if(!ret) /* Insufficient buffer size */
		return dst_len;
	    if(ret < dst_len) /* Append 0 if enough space */
		dst[ret] = 0;
	    return ret;
	}
}


/*********************************************************************
 *
 *	EM_GETSEL
 *
 */
static LRESULT EDIT_EM_GetSel(EDITSTATE *es, LPUINT start, LPUINT end)
{
	UINT s = es->selection_start;
	UINT e = es->selection_end;

	ORDER_UINT(s, e);
	if (start)
		*start = s;
	if (end)
		*end = e;
	return MAKELONG(s, e);
}


/*********************************************************************
 *
 *	EM_GETTHUMB
 *
 *	FIXME: is this right ?  (or should it be only VSCROLL)
 *	(and maybe only for edit controls that really have their
 *	own scrollbars) (and maybe only for multiline controls ?)
 *	All in all: very poorly documented
 *
 */
static LRESULT EDIT_EM_GetThumb(HWND hwnd, EDITSTATE *es)
{
	return MAKELONG(EDIT_WM_VScroll(hwnd, es, EM_GETTHUMB16, 0),
		EDIT_WM_HScroll(hwnd, es, EM_GETTHUMB16, 0));
}


/*********************************************************************
 *
 *	EM_LINEFROMCHAR
 *
 */
static INT EDIT_EM_LineFromChar(EDITSTATE *es, INT index)
{
	INT line;
	LINEDEF *line_def;

	if (!(es->style & ES_MULTILINE))
		return 0;
	if (index > (INT)strlenW(es->text))
		return es->line_count - 1;
	if (index == -1)
		index = min(es->selection_start, es->selection_end);

	line = 0;
	line_def = es->first_line_def;
	index -= line_def->length;
	while ((index >= 0) && line_def->next) {
		line++;
		line_def = line_def->next;
		index -= line_def->length;
	}
	return line;
}


/*********************************************************************
 *
 *	EM_LINEINDEX
 *
 */
static INT EDIT_EM_LineIndex(EDITSTATE *es, INT line)
{
	INT line_index;
	LINEDEF *line_def;

	if (!(es->style & ES_MULTILINE))
		return 0;
	if (line >= es->line_count)
		return -1;

	line_index = 0;
	line_def = es->first_line_def;
	if (line == -1) {
		INT index = es->selection_end - line_def->length;
		while ((index >= 0) && line_def->next) {
			line_index += line_def->length;
			line_def = line_def->next;
			index -= line_def->length;
		}
	} else {
		while (line > 0) {
			line_index += line_def->length;
			line_def = line_def->next;
			line--;
		}
	}
	return line_index;
}


/*********************************************************************
 *
 *	EM_LINELENGTH
 *
 */
static INT EDIT_EM_LineLength(EDITSTATE *es, INT index)
{
	LINEDEF *line_def;

	if (!(es->style & ES_MULTILINE))
		return strlenW(es->text);

	if (index == -1) {
		/* get the number of remaining non-selected chars of selected lines */
		INT32 l; /* line number */
		INT32 li; /* index of first char in line */
		INT32 count;
		l = EDIT_EM_LineFromChar(es, es->selection_start);
		/* # chars before start of selection area */
		count = es->selection_start - EDIT_EM_LineIndex(es, l);
		l = EDIT_EM_LineFromChar(es, es->selection_end);
		/* # chars after end of selection */
		li = EDIT_EM_LineIndex(es, l);
		count += li + EDIT_EM_LineLength(es, li) - es->selection_end;
		return count;
	}
	line_def = es->first_line_def;
	index -= line_def->length;
	while ((index >= 0) && line_def->next) {
		line_def = line_def->next;
		index -= line_def->length;
	}
	return line_def->net_length;
}


/*********************************************************************
 *
 *	EM_LINESCROLL
 *
 *	NOTE: dx is in average character widths, dy - in lines;
 *
 */
static BOOL EDIT_EM_LineScroll(HWND hwnd, EDITSTATE *es, INT dx, INT dy)
{
	if (!(es->style & ES_MULTILINE))
		return FALSE;

	dx *= es->char_width;
	return EDIT_EM_LineScroll_internal(hwnd, es, dx, dy);
}

/*********************************************************************
 *
 *	EDIT_EM_LineScroll_internal
 *
 *	Version of EDIT_EM_LineScroll for internal use.
 *	It doesn't refuse if ES_MULTILINE is set and assumes that
 *	dx is in pixels, dy - in lines.
 *
 */
static BOOL EDIT_EM_LineScroll_internal(HWND hwnd, EDITSTATE *es, INT dx, INT dy)
{
	INT nyoff;
	INT x_offset_in_pixels;

	if (es->style & ES_MULTILINE)
	{
	    x_offset_in_pixels = es->x_offset;
	}
	else
	{
	    dy = 0;
	    x_offset_in_pixels = SLOWORD(EDIT_EM_PosFromChar(hwnd, es, es->x_offset, FALSE));
	}

	if (-dx > x_offset_in_pixels)
		dx = -x_offset_in_pixels;
	if (dx > es->text_width - x_offset_in_pixels)
		dx = es->text_width - x_offset_in_pixels;
	nyoff = max(0, es->y_offset + dy);
	if (nyoff >= es->line_count)
		nyoff = es->line_count - 1;
	dy = (es->y_offset - nyoff) * es->line_height;
	if (dx || dy) {
		RECT rc1;
		RECT rc;

		es->y_offset = nyoff;
		if(es->style & ES_MULTILINE)
		    es->x_offset += dx;
		else
		    es->x_offset += dx / es->char_width;

		GetClientRect(hwnd, &rc1);
		IntersectRect(&rc, &rc1, &es->format_rect);
		ScrollWindowEx(hwnd, -dx, dy,
				NULL, &rc, (HRGN)NULL, NULL, SW_INVALIDATE);
		/* force scroll info update */
		EDIT_UpdateScrollInfo(hwnd, es);
	}
	if (dx && !(es->flags & EF_HSCROLL_TRACK))
		EDIT_NOTIFY_PARENT(hwnd, es, EN_HSCROLL, "EN_HSCROLL");
	if (dy && !(es->flags & EF_VSCROLL_TRACK))
		EDIT_NOTIFY_PARENT(hwnd, es, EN_VSCROLL, "EN_VSCROLL");
	return TRUE;
}


/*********************************************************************
 *
 *	EM_POSFROMCHAR
 *
 */
static LRESULT EDIT_EM_PosFromChar(HWND hwnd, EDITSTATE *es, INT index, BOOL after_wrap)
{
	INT len = strlenW(es->text);
	INT l;
	INT li;
	INT x;
	INT y = 0;
	HDC dc;
	HFONT old_font = 0;
	SIZE size;

	index = min(index, len);
	dc = GetDC(hwnd);
	if (es->font)
		old_font = SelectObject(dc, es->font);
	if (es->style & ES_MULTILINE) {
		l = EDIT_EM_LineFromChar(es, index);
		y = (l - es->y_offset) * es->line_height;
		li = EDIT_EM_LineIndex(es, l);
		if (after_wrap && (li == index) && l) {
			INT l2 = l - 1;
			LINEDEF *line_def = es->first_line_def;
			while (l2) {
				line_def = line_def->next;
				l2--;
			}
			if (line_def->ending == END_WRAP) {
				l--;
				y -= es->line_height;
				li = EDIT_EM_LineIndex(es, l);
			}
		}
		x = LOWORD(GetTabbedTextExtentW(dc, es->text + li, index - li,
				es->tabs_count, es->tabs)) - es->x_offset;
	} else {
		LPWSTR text = EDIT_GetPasswordPointer_SL(es);
		if (index < es->x_offset) {
			GetTextExtentPoint32W(dc, text + index,
					es->x_offset - index, &size);
			x = -size.cx;
		} else {
			GetTextExtentPoint32W(dc, text + es->x_offset,
					index - es->x_offset, &size);
			 x = size.cx;
		}
		y = 0;
		if (es->style & ES_PASSWORD)
			HeapFree(GetProcessHeap(), 0, text);
	}
	x += es->format_rect.left;
	y += es->format_rect.top;
	if (es->font)
		SelectObject(dc, old_font);
	ReleaseDC(hwnd, dc);
	return MAKELONG((INT16)x, (INT16)y);
}


/*********************************************************************
 *
 *	EM_REPLACESEL
 *
 *	FIXME: handle ES_NUMBER and ES_OEMCONVERT here
 *
 */
static void EDIT_EM_ReplaceSel(HWND hwnd, EDITSTATE *es, BOOL can_undo, LPCWSTR lpsz_replace, BOOL send_update)
{
	UINT strl = strlenW(lpsz_replace);
	UINT tl = strlenW(es->text);
	UINT utl;
	UINT s;
	UINT e;
	UINT i;
	LPWSTR p;
	HRGN hrgn = 0;

	TRACE("%s, can_undo %d, send_update %d\n",
	    debugstr_w(lpsz_replace), can_undo, send_update);

	s = es->selection_start;
	e = es->selection_end;

	if ((s == e) && !strl)
		return;

	ORDER_UINT(s, e);

	if (!EDIT_MakeFit(hwnd, es, tl - (e - s) + strl))
		return;

	if (e != s) {
		/* there is something to be deleted */
		TRACE("deleting stuff.\n");
		if (can_undo) {
			utl = strlenW(es->undo_text);
			if (!es->undo_insert_count && (*es->undo_text && (s == es->undo_position))) {
				/* undo-buffer is extended to the right */
				EDIT_MakeUndoFit(es, utl + e - s);
				strncpyW(es->undo_text + utl, es->text + s, e - s + 1);
				(es->undo_text + utl)[e - s] = 0; /* ensure 0 termination */
			} else if (!es->undo_insert_count && (*es->undo_text && (e == es->undo_position))) {
				/* undo-buffer is extended to the left */
				EDIT_MakeUndoFit(es, utl + e - s);
				for (p = es->undo_text + utl ; p >= es->undo_text ; p--)
					p[e - s] = p[0];
				for (i = 0 , p = es->undo_text ; i < e - s ; i++)
					p[i] = (es->text + s)[i];
				es->undo_position = s;
			} else {
				/* new undo-buffer */
				EDIT_MakeUndoFit(es, e - s);
				strncpyW(es->undo_text, es->text + s, e - s + 1);
				es->undo_text[e - s] = 0; /* ensure 0 termination */
				es->undo_position = s;
			}
			/* any deletion makes the old insertion-undo invalid */
			es->undo_insert_count = 0;
		} else
			EDIT_EM_EmptyUndoBuffer(es);

		/* now delete */
		strcpyW(es->text + s, es->text + e);
	}
	if (strl) {
		/* there is an insertion */
		if (can_undo) {
			if ((s == es->undo_position) ||
					((es->undo_insert_count) &&
					(s == es->undo_position + es->undo_insert_count)))
				/*
				 * insertion is new and at delete position or
				 * an extension to either left or right
				 */
				es->undo_insert_count += strl;
			else {
				/* new insertion undo */
				es->undo_position = s;
				es->undo_insert_count = strl;
				/* new insertion makes old delete-buffer invalid */
				*es->undo_text = '\0';
			}
		} else
			EDIT_EM_EmptyUndoBuffer(es);

		/* now insert */
		tl = strlenW(es->text);
		TRACE("inserting stuff (tl %d, strl %d, selstart %d ('%s'), text '%s')\n", tl, strl, s, debugstr_w(es->text + s), debugstr_w(es->text));
		for (p = es->text + tl ; p >= es->text + s ; p--)
			p[strl] = p[0];
		for (i = 0 , p = es->text + s ; i < strl ; i++)
			p[i] = lpsz_replace[i];
		if(es->style & ES_UPPERCASE)
			CharUpperBuffW(p, strl);
		else if(es->style & ES_LOWERCASE)
			CharLowerBuffW(p, strl);
		s += strl;
	}
	if (es->style & ES_MULTILINE)
	{
		INT s = min(es->selection_start, es->selection_end);

		hrgn = CreateRectRgn(0, 0, 0, 0);
		EDIT_BuildLineDefs_ML(hwnd, es, s, s + strl, 
				strl - abs(es->selection_end - es->selection_start), hrgn);
	}
	else
	    EDIT_CalcLineWidth_SL(hwnd, es);

	EDIT_EM_SetSel(hwnd, es, s, s, FALSE);
	es->flags |= EF_MODIFIED;
	if (send_update) es->flags |= EF_UPDATE;
	EDIT_EM_ScrollCaret(hwnd, es);
	
	/* force scroll info update */
	EDIT_UpdateScrollInfo(hwnd, es);

	if (hrgn)
	{
		EDIT_UpdateTextRegion(hwnd, es, hrgn, TRUE);
		DeleteObject(hrgn);
	}
	else
	EDIT_UpdateText(hwnd, es, NULL, TRUE);

	if(es->flags & EF_UPDATE)
	{
	    es->flags &= ~EF_UPDATE;
	    EDIT_NOTIFY_PARENT(hwnd, es, EN_CHANGE, "EN_CHANGE");
	}
}


/*********************************************************************
 *
 *	EM_SCROLL
 *
 */
static LRESULT EDIT_EM_Scroll(HWND hwnd, EDITSTATE *es, INT action)
{
	INT dy;

	if (!(es->style & ES_MULTILINE))
		return (LRESULT)FALSE;

	dy = 0;

	switch (action) {
	case SB_LINEUP:
		if (es->y_offset)
			dy = -1;
		break;
	case SB_LINEDOWN:
		if (es->y_offset < es->line_count - 1)
			dy = 1;
		break;
	case SB_PAGEUP:
		if (es->y_offset)
			dy = -(es->format_rect.bottom - es->format_rect.top) / es->line_height;
		break;
	case SB_PAGEDOWN:
		if (es->y_offset < es->line_count - 1)
			dy = (es->format_rect.bottom - es->format_rect.top) / es->line_height;
		break;
	default:
		return (LRESULT)FALSE;
	}
	if (dy) {
	    INT vlc = (es->format_rect.bottom - es->format_rect.top) / es->line_height;
	    /* check if we are going to move too far */
	    if(es->y_offset + dy > es->line_count - vlc)
		dy = es->line_count - vlc - es->y_offset;

	    /* Notification is done in EDIT_EM_LineScroll */
	    if(dy)
		EDIT_EM_LineScroll(hwnd, es, 0, dy);
	}
	return MAKELONG((INT16)dy, (BOOL16)TRUE);
}


/*********************************************************************
 *
 *	EM_SCROLLCARET
 *
 */
static void EDIT_EM_ScrollCaret(HWND hwnd, EDITSTATE *es)
{
	if (es->style & ES_MULTILINE) {
		INT l;
		INT li;
		INT vlc;
		INT ww;
		INT cw = es->char_width;
		INT x;
		INT dy = 0;
		INT dx = 0;

		l = EDIT_EM_LineFromChar(es, es->selection_end);
		li = EDIT_EM_LineIndex(es, l);
		x = SLOWORD(EDIT_EM_PosFromChar(hwnd, es, es->selection_end, es->flags & EF_AFTER_WRAP));
		vlc = (es->format_rect.bottom - es->format_rect.top) / es->line_height;
		if (l >= es->y_offset + vlc)
			dy = l - vlc + 1 - es->y_offset;
		if (l < es->y_offset)
			dy = l - es->y_offset;
		ww = es->format_rect.right - es->format_rect.left;
		if (x < es->format_rect.left)
			dx = x - es->format_rect.left - ww / HSCROLL_FRACTION / cw * cw;
		if (x > es->format_rect.right)
			dx = x - es->format_rect.left - (HSCROLL_FRACTION - 1) * ww / HSCROLL_FRACTION / cw * cw;
		if (dy || dx)
		{
		    /* check if we are going to move too far */
		    if(es->x_offset + dx + ww > es->text_width)
			dx = es->text_width - ww - es->x_offset;
		    if(dx || dy)
			EDIT_EM_LineScroll_internal(hwnd, es, dx, dy);
		}
	} else {
		INT x;
		INT goal;
		INT format_width;

		if (!(es->style & ES_AUTOHSCROLL))
			return;

		x = SLOWORD(EDIT_EM_PosFromChar(hwnd, es, es->selection_end, FALSE));
		format_width = es->format_rect.right - es->format_rect.left;
		if (x < es->format_rect.left) {
			goal = es->format_rect.left + format_width / HSCROLL_FRACTION;
			do {
				es->x_offset--;
				x = SLOWORD(EDIT_EM_PosFromChar(hwnd, es, es->selection_end, FALSE));
			} while ((x < goal) && es->x_offset);
			/* FIXME: use ScrollWindow() somehow to improve performance */
			EDIT_UpdateText(hwnd, es, NULL, TRUE);
		} else if (x > es->format_rect.right) {
			INT x_last;
			INT len = strlenW(es->text);
			goal = es->format_rect.right - format_width / HSCROLL_FRACTION;
			do {
				es->x_offset++;
				x = SLOWORD(EDIT_EM_PosFromChar(hwnd, es, es->selection_end, FALSE));
				x_last = SLOWORD(EDIT_EM_PosFromChar(hwnd, es, len, FALSE));
			} while ((x > goal) && (x_last > es->format_rect.right));
			/* FIXME: use ScrollWindow() somehow to improve performance */
			EDIT_UpdateText(hwnd, es, NULL, TRUE);
		}
	}

    if(es->flags & EF_FOCUSED)
	EDIT_SetCaretPos(hwnd, es, es->selection_end, es->flags & EF_AFTER_WRAP);
}


/*********************************************************************
 *
 *	EM_SETHANDLE
 *
 *	FIXME:	ES_LOWERCASE, ES_UPPERCASE, ES_OEMCONVERT, ES_NUMBER ???
 *
 */
static void EDIT_EM_SetHandle(HWND hwnd, EDITSTATE *es, HLOCAL hloc)
{
    HINSTANCE hInstance = GetWindowLongA( hwnd, GWL_HINSTANCE );

	if (!(es->style & ES_MULTILINE))
		return;

	if (!hloc) {
		WARN("called with NULL handle\n");
		return;
	}

	EDIT_UnlockBuffer(hwnd, es, TRUE);

	if(es->hloc16)
	{
	    LOCAL_Free(hInstance, es->hloc16);
	    es->hloc16 = (HLOCAL16)NULL;
	}

	if(es->is_unicode)
	{
	    if(es->hloc32A)
	    {
		LocalFree(es->hloc32A);
		es->hloc32A = (HLOCAL)NULL;
	    }
	    es->hloc32W = hloc;
	}
	else
	{
	    INT countW, countA;
	    HLOCAL hloc32W_new;
	    WCHAR *textW;
	    CHAR *textA;

	    countA = LocalSize(hloc);
	    textA = LocalLock(hloc);
	    countW = MultiByteToWideChar(CP_ACP, 0, textA, countA, NULL, 0);
	    if(!(hloc32W_new = LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT, countW * sizeof(WCHAR))))
	    {
		ERR("Could not allocate new unicode buffer\n");
		return;
	    }
	    textW = LocalLock(hloc32W_new);
	    MultiByteToWideChar(CP_ACP, 0, textA, countA, textW, countW);
	    LocalUnlock(hloc32W_new);
	    LocalUnlock(hloc);

	    if(es->hloc32W)
		LocalFree(es->hloc32W);

	    es->hloc32W = hloc32W_new;
	    es->hloc32A = hloc;
	}

	es->buffer_size = LocalSize(es->hloc32W)/sizeof(WCHAR) - 1;

	EDIT_LockBuffer(hwnd, es);

	es->x_offset = es->y_offset = 0;
	es->selection_start = es->selection_end = 0;
	EDIT_EM_EmptyUndoBuffer(es);
	es->flags &= ~EF_MODIFIED;
	es->flags &= ~EF_UPDATE;
	EDIT_BuildLineDefs_ML(hwnd, es, 0, strlenW(es->text), 0, (HRGN)0);
	EDIT_UpdateText(hwnd, es, NULL, TRUE);
	EDIT_EM_ScrollCaret(hwnd, es);
	/* force scroll info update */
	EDIT_UpdateScrollInfo(hwnd, es);
}


/*********************************************************************
 *
 *	EM_SETHANDLE16
 *
 *	FIXME:	ES_LOWERCASE, ES_UPPERCASE, ES_OEMCONVERT, ES_NUMBER ???
 *
 */
static void EDIT_EM_SetHandle16(HWND hwnd, EDITSTATE *es, HLOCAL16 hloc)
{
    HINSTANCE hInstance = GetWindowLongA( hwnd, GWL_HINSTANCE );
	INT countW, countA;
	HLOCAL hloc32W_new;
	WCHAR *textW;
	CHAR *textA;

	if (!(es->style & ES_MULTILINE))
		return;

	if (!hloc) {
		WARN("called with NULL handle\n");
		return;
	}

	EDIT_UnlockBuffer(hwnd, es, TRUE);

	if(es->hloc32A)
	{
	    LocalFree(es->hloc32A);
	    es->hloc32A = (HLOCAL)NULL;
	}

	countA = LOCAL_Size(hInstance, hloc);
	textA = LOCAL_Lock(hInstance, hloc);
	countW = MultiByteToWideChar(CP_ACP, 0, textA, countA, NULL, 0);
	if(!(hloc32W_new = LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT, countW * sizeof(WCHAR))))
	{
	    ERR("Could not allocate new unicode buffer\n");
	    return;
	}
	textW = LocalLock(hloc32W_new);
	MultiByteToWideChar(CP_ACP, 0, textA, countA, textW, countW);
	LocalUnlock(hloc32W_new);
	LOCAL_Unlock(hInstance, hloc);

	if(es->hloc32W)
	    LocalFree(es->hloc32W);

	es->hloc32W = hloc32W_new;
	es->hloc16 = hloc;

	es->buffer_size = LocalSize(es->hloc32W)/sizeof(WCHAR) - 1;

	EDIT_LockBuffer(hwnd, es);

	es->x_offset = es->y_offset = 0;
	es->selection_start = es->selection_end = 0;
	EDIT_EM_EmptyUndoBuffer(es);
	es->flags &= ~EF_MODIFIED;
	es->flags &= ~EF_UPDATE;
	EDIT_BuildLineDefs_ML(hwnd, es, 0, strlenW(es->text), 0, (HRGN)0);
	EDIT_UpdateText(hwnd, es, NULL, TRUE);
	EDIT_EM_ScrollCaret(hwnd, es);
	/* force scroll info update */
	EDIT_UpdateScrollInfo(hwnd, es);
}


/*********************************************************************
 *
 *	EM_SETLIMITTEXT
 *
 *	FIXME: in WinNT maxsize is 0x7FFFFFFF / 0xFFFFFFFF
 *	However, the windows version is not complied to yet in all of edit.c
 *
 */
static void EDIT_EM_SetLimitText(EDITSTATE *es, INT limit)
{
	if (es->style & ES_MULTILINE) {
		if (limit)
			es->buffer_limit = min(limit, BUFLIMIT_MULTI);
		else
			es->buffer_limit = BUFLIMIT_MULTI;
	} else {
		if (limit)
			es->buffer_limit = min(limit, BUFLIMIT_SINGLE);
		else
			es->buffer_limit = BUFLIMIT_SINGLE;
	}
}


/*********************************************************************
 *
 *	EM_SETMARGINS
 * 
 * EC_USEFONTINFO is used as a left or right value i.e. lParam and not as an
 * action wParam despite what the docs say. EC_USEFONTINFO means one third
 * of the char's width, according to the new docs.
 *
 */
static void EDIT_EM_SetMargins(EDITSTATE *es, INT action,
			       INT left, INT right)
{
	if (action & EC_LEFTMARGIN) {
		if (left != EC_USEFONTINFO)
			es->left_margin = left;
		else
			es->left_margin = es->char_width / 3;
	}

	if (action & EC_RIGHTMARGIN) {
		if (right != EC_USEFONTINFO)
 			es->right_margin = right;
		else
			es->right_margin = es->char_width / 3;
	}
	TRACE("left=%d, right=%d\n", es->left_margin, es->right_margin);
}


/*********************************************************************
 *
 *	EM_SETPASSWORDCHAR
 *
 */
static void EDIT_EM_SetPasswordChar(HWND hwnd, EDITSTATE *es, WCHAR c)
{
    LONG style;

	if (es->style & ES_MULTILINE)
		return;

	if (es->password_char == c)
		return;

        style = GetWindowLongA( hwnd, GWL_STYLE );
	es->password_char = c;
	if (c) {
            SetWindowLongA( hwnd, GWL_STYLE, style | ES_PASSWORD );
            es->style |= ES_PASSWORD;
	} else {
            SetWindowLongA( hwnd, GWL_STYLE, style & ~ES_PASSWORD );
            es->style &= ~ES_PASSWORD;
	}
	EDIT_UpdateText(hwnd, es, NULL, TRUE);
}


/*********************************************************************
 *
 *	EDIT_EM_SetSel
 *
 *	note:	unlike the specs say: the order of start and end
 *		_is_ preserved in Windows.  (i.e. start can be > end)
 *		In other words: this handler is OK
 *
 */
static void EDIT_EM_SetSel(HWND hwnd, EDITSTATE *es, UINT start, UINT end, BOOL after_wrap)
{
	UINT old_start = es->selection_start;
	UINT old_end = es->selection_end;
	UINT len = strlenW(es->text);

	if (start == (UINT)-1) {
		start = es->selection_end;
		end = es->selection_end;
	} else {
		start = min(start, len);
		end = min(end, len);
	}
	es->selection_start = start;
	es->selection_end = end;
	if (after_wrap)
		es->flags |= EF_AFTER_WRAP;
	else
		es->flags &= ~EF_AFTER_WRAP;
/* This is a little bit more efficient than before, not sure if it can be improved. FIXME? */
        ORDER_UINT(start, end);
        ORDER_UINT(end, old_end);
        ORDER_UINT(start, old_start);
        ORDER_UINT(old_start, old_end);
	if (end != old_start)
        {
/*
 * One can also do 
 *          ORDER_UINT32(end, old_start);
 *          EDIT_InvalidateText(hwnd, es, start, end);
 *          EDIT_InvalidateText(hwnd, es, old_start, old_end);
 * in place of the following if statement.                          
 */
            if (old_start > end )
            {
                EDIT_InvalidateText(hwnd, es, start, end);
                EDIT_InvalidateText(hwnd, es, old_start, old_end);
            }
            else
            {
                EDIT_InvalidateText(hwnd, es, start, old_start);
                EDIT_InvalidateText(hwnd, es, end, old_end);
            }
	}
        else EDIT_InvalidateText(hwnd, es, start, old_end);
}


/*********************************************************************
 *
 *	EM_SETTABSTOPS
 *
 */
static BOOL EDIT_EM_SetTabStops(EDITSTATE *es, INT count, LPINT tabs)
{
	if (!(es->style & ES_MULTILINE))
		return FALSE;
	if (es->tabs)
		HeapFree(GetProcessHeap(), 0, es->tabs);
	es->tabs_count = count;
	if (!count)
		es->tabs = NULL;
	else {
		es->tabs = HeapAlloc(GetProcessHeap(), 0, count * sizeof(INT));
		memcpy(es->tabs, tabs, count * sizeof(INT));
	}
	return TRUE;
}


/*********************************************************************
 *
 *	EM_SETTABSTOPS16
 *
 */
static BOOL EDIT_EM_SetTabStops16(EDITSTATE *es, INT count, LPINT16 tabs)
{
	if (!(es->style & ES_MULTILINE))
		return FALSE;
	if (es->tabs)
		HeapFree(GetProcessHeap(), 0, es->tabs);
	es->tabs_count = count;
	if (!count)
		es->tabs = NULL;
	else {
		INT i;
		es->tabs = HeapAlloc(GetProcessHeap(), 0, count * sizeof(INT));
		for (i = 0 ; i < count ; i++)
			es->tabs[i] = *tabs++;
	}
	return TRUE;
}


/*********************************************************************
 *
 *	EM_SETWORDBREAKPROC
 *
 */
static void EDIT_EM_SetWordBreakProc(HWND hwnd, EDITSTATE *es, LPARAM lParam)
{
	if (es->word_break_proc == (void *)lParam)
		return;

	es->word_break_proc = (void *)lParam;
	es->word_break_proc16 = NULL;

	if ((es->style & ES_MULTILINE) && !(es->style & ES_AUTOHSCROLL)) {
		EDIT_BuildLineDefs_ML(hwnd, es, 0, strlenW(es->text), 0, (HRGN)0);
		EDIT_UpdateText(hwnd, es, NULL, TRUE);
	}
}


/*********************************************************************
 *
 *	EM_SETWORDBREAKPROC16
 *
 */
static void EDIT_EM_SetWordBreakProc16(HWND hwnd, EDITSTATE *es, EDITWORDBREAKPROC16 wbp)
{
	if (es->word_break_proc16 == wbp)
		return;

	es->word_break_proc = NULL;
	es->word_break_proc16 = wbp;
	if ((es->style & ES_MULTILINE) && !(es->style & ES_AUTOHSCROLL)) {
		EDIT_BuildLineDefs_ML(hwnd, es, 0, strlenW(es->text), 0, (HRGN)0);
		EDIT_UpdateText(hwnd, es, NULL, TRUE);
	}
}


/*********************************************************************
 *
 *	EM_UNDO / WM_UNDO
 *
 */
static BOOL EDIT_EM_Undo(HWND hwnd, EDITSTATE *es)
{
	INT ulength;
	LPWSTR utext;

	/* Protect read-only edit control from modification */
	if(es->style & ES_READONLY)
	    return FALSE;

	ulength = strlenW(es->undo_text);
	utext = HeapAlloc(GetProcessHeap(), 0, (ulength + 1) * sizeof(WCHAR));

	strcpyW(utext, es->undo_text);

	TRACE("before UNDO:insertion length = %d, deletion buffer = %s\n",
		     es->undo_insert_count, debugstr_w(utext));

	EDIT_EM_SetSel(hwnd, es, es->undo_position, es->undo_position + es->undo_insert_count, FALSE);
	EDIT_EM_EmptyUndoBuffer(es);
	EDIT_EM_ReplaceSel(hwnd, es, TRUE, utext, FALSE);
	EDIT_EM_SetSel(hwnd, es, es->undo_position, es->undo_position + es->undo_insert_count, FALSE);
        /* send the notification after the selection start and end are set */
        EDIT_NOTIFY_PARENT(hwnd, es, EN_CHANGE, "EN_CHANGE");
	EDIT_EM_ScrollCaret(hwnd, es);
	HeapFree(GetProcessHeap(), 0, utext);

	TRACE("after UNDO:insertion length = %d, deletion buffer = %s\n",
			es->undo_insert_count, debugstr_w(es->undo_text));
	return TRUE;
}


/*********************************************************************
 *
 *	WM_CHAR
 *
 */
static void EDIT_WM_Char(HWND hwnd, EDITSTATE *es, WCHAR c)
{
        BOOL control;

	/* Protect read-only edit control from modification */
	if(es->style & ES_READONLY)
	    return;

	control = GetKeyState(VK_CONTROL) & 0x8000;

	switch (c) {
	case '\r':
	    /* If the edit doesn't want the return and it's not a multiline edit, do nothing */
	    if(!(es->style & ES_MULTILINE) && !(es->style & ES_WANTRETURN))
		break;
	case '\n':
		if (es->style & ES_MULTILINE) {
			if (es->style & ES_READONLY) {
				EDIT_MoveHome(hwnd, es, FALSE);
				EDIT_MoveDown_ML(hwnd, es, FALSE);
			} else {
				static const WCHAR cr_lfW[] = {'\r','\n',0};
				EDIT_EM_ReplaceSel(hwnd, es, TRUE, cr_lfW, TRUE);
			}
		}
		break;
	case '\t':
		if ((es->style & ES_MULTILINE) && !(es->style & ES_READONLY))
		{
			static const WCHAR tabW[] = {'\t',0};
			EDIT_EM_ReplaceSel(hwnd, es, TRUE, tabW, TRUE);
		}
		break;
	case VK_BACK:
		if (!(es->style & ES_READONLY) && !control) {
			if (es->selection_start != es->selection_end)
				EDIT_WM_Clear(hwnd, es);
			else {
				/* delete character left of caret */
				EDIT_EM_SetSel(hwnd, es, (UINT)-1, 0, FALSE);
				EDIT_MoveBackward(hwnd, es, TRUE);
				EDIT_WM_Clear(hwnd, es);
			}
		}
		break;
	case 0x03: /* ^C */
		SendMessageW(hwnd, WM_COPY, 0, 0);
		break;
	case 0x16: /* ^V */
		SendMessageW(hwnd, WM_PASTE, 0, 0);
		break;
	case 0x18: /* ^X */
		SendMessageW(hwnd, WM_CUT, 0, 0);
		break;
	
	default:
		if (!(es->style & ES_READONLY) && (c >= ' ') && (c != 127)) {
			WCHAR str[2];
 			str[0] = c;
 			str[1] = '\0';
 			EDIT_EM_ReplaceSel(hwnd, es, TRUE, str, TRUE);
 		}
		break;
	}
}


/*********************************************************************
 *
 *	WM_COMMAND
 *
 */
static void EDIT_WM_Command(HWND hwnd, EDITSTATE *es, INT code, INT id, HWND control)
{
	if (code || control)
		return;

	switch (id) {
		case EM_UNDO:
			EDIT_EM_Undo(hwnd, es);
			break;
		case WM_CUT:
			EDIT_WM_Cut(hwnd, es);
			break;
		case WM_COPY:
			EDIT_WM_Copy(hwnd, es);
			break;
		case WM_PASTE:
			EDIT_WM_Paste(hwnd, es);
			break;
		case WM_CLEAR:
			EDIT_WM_Clear(hwnd, es);
			break;
		case EM_SETSEL:
			EDIT_EM_SetSel(hwnd, es, 0, (UINT)-1, FALSE);
			EDIT_EM_ScrollCaret(hwnd, es);
			break;
		default:
			ERR("unknown menu item, please report\n");
			break;
	}
}


/*********************************************************************
 *
 *	WM_CONTEXTMENU
 *
 *	Note: the resource files resource/sysres_??.rc cannot define a
 *		single popup menu.  Hence we use a (dummy) menubar
 *		containing the single popup menu as its first item.
 *
 *	FIXME: the message identifiers have been chosen arbitrarily,
 *		hence we use MF_BYPOSITION.
 *		We might as well use the "real" values (anybody knows ?)
 *		The menu definition is in resources/sysres_??.rc.
 *		Once these are OK, we better use MF_BYCOMMAND here
 *		(as we do in EDIT_WM_Command()).
 *
 */
static void EDIT_WM_ContextMenu(HWND hwnd, EDITSTATE *es, INT x, INT y)
{
	HMENU menu = LoadMenuA(GetModuleHandleA("USER32"), "EDITMENU");
	HMENU popup = GetSubMenu(menu, 0);
	UINT start = es->selection_start;
	UINT end = es->selection_end;

	ORDER_UINT(start, end);

	/* undo */
	EnableMenuItem(popup, 0, MF_BYPOSITION | (EDIT_EM_CanUndo(es) && !(es->style & ES_READONLY) ? MF_ENABLED : MF_GRAYED));
	/* cut */
	EnableMenuItem(popup, 2, MF_BYPOSITION | ((end - start) && !(es->style & ES_PASSWORD) && !(es->style & ES_READONLY) ? MF_ENABLED : MF_GRAYED));
	/* copy */
	EnableMenuItem(popup, 3, MF_BYPOSITION | ((end - start) && !(es->style & ES_PASSWORD) ? MF_ENABLED : MF_GRAYED));
	/* paste */
	EnableMenuItem(popup, 4, MF_BYPOSITION | (IsClipboardFormatAvailable(CF_UNICODETEXT) && !(es->style & ES_READONLY) ? MF_ENABLED : MF_GRAYED));
	/* delete */
	EnableMenuItem(popup, 5, MF_BYPOSITION | ((end - start) && !(es->style & ES_READONLY) ? MF_ENABLED : MF_GRAYED));
	/* select all */
	EnableMenuItem(popup, 7, MF_BYPOSITION | (start || (end != strlenW(es->text)) ? MF_ENABLED : MF_GRAYED));

	TrackPopupMenu(popup, TPM_LEFTALIGN | TPM_RIGHTBUTTON, x, y, 0, hwnd, NULL);
	DestroyMenu(menu);
}


/*********************************************************************
 *
 *	WM_COPY
 *
 */
static void EDIT_WM_Copy(HWND hwnd, EDITSTATE *es)
{
	INT s = es->selection_start;
	INT e = es->selection_end;
	HGLOBAL hdst;
	LPWSTR dst;

	if (e == s)
		return;
	ORDER_INT(s, e);
	hdst = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, (DWORD)(e - s + 1) * sizeof(WCHAR));
	dst = GlobalLock(hdst);
	strncpyW(dst, es->text + s, e - s);
	dst[e - s] = 0; /* ensure 0 termination */
	TRACE("%s\n", debugstr_w(dst));
	GlobalUnlock(hdst);
	OpenClipboard(hwnd);
	EmptyClipboard();
	SetClipboardData(CF_UNICODETEXT, hdst);
	CloseClipboard();
}


/*********************************************************************
 *
 *	WM_CREATE
 *
 */
static LRESULT EDIT_WM_Create(HWND hwnd, EDITSTATE *es, LPCWSTR name)
{
	TRACE("%s\n", debugstr_w(name));
       /*
        *	To initialize some final structure members, we call some helper
        *	functions.  However, since the EDITSTATE is not consistent (i.e.
        *	not fully initialized), we should be very careful which
        *	functions can be called, and in what order.
        */
        EDIT_WM_SetFont(hwnd, es, 0, FALSE);
        EDIT_EM_EmptyUndoBuffer(es);

       if (name && *name) {
	   EDIT_EM_ReplaceSel(hwnd, es, FALSE, name, FALSE);
	   /* if we insert text to the editline, the text scrolls out
            * of the window, as the caret is placed after the insert
            * pos normally; thus we reset es->selection... to 0 and
            * update caret
            */
	   es->selection_start = es->selection_end = 0;
           /* send the notification after the selection start and end are set */
           EDIT_NOTIFY_PARENT(hwnd, es, EN_CHANGE, "EN_CHANGE");
	   EDIT_EM_ScrollCaret(hwnd, es);
       }
       /* force scroll info update */
       EDIT_UpdateScrollInfo(hwnd, es);
       return 0;
}


/*********************************************************************
 *
 *	WM_DESTROY
 *
 */
static void EDIT_WM_Destroy(HWND hwnd, EDITSTATE *es)
{
    HINSTANCE hInstance = GetWindowLongA( hwnd, GWL_HINSTANCE );
	LINEDEF *pc, *pp;

	if (es->hloc32W) {
		while (LocalUnlock(es->hloc32W)) ;
		LocalFree(es->hloc32W);
	}
	if (es->hloc32A) {
		while (LocalUnlock(es->hloc32A)) ;
		LocalFree(es->hloc32A);
	}
	if (es->hloc16) {
		while (LOCAL_Unlock(hInstance, es->hloc16)) ;
		LOCAL_Free(hInstance, es->hloc16);
	}

	pc = es->first_line_def;
	while (pc)
	{
		pp = pc->next;
		HeapFree(GetProcessHeap(), 0, pc);
		pc = pp;
	}

        SetWindowLongA( hwnd, 0, 0 );
	HeapFree(GetProcessHeap(), 0, es);
}


/*********************************************************************
 *
 *	WM_ERASEBKGND
 *
 */
static LRESULT EDIT_WM_EraseBkGnd(HWND hwnd, EDITSTATE *es, HDC dc)
{
	HBRUSH brush;
	RECT rc;

        if ( get_app_version() >= 0x40000 &&(
                    !es->bEnableState || (es->style & ES_READONLY)))
                brush = (HBRUSH)EDIT_SEND_CTLCOLORSTATIC(hwnd, dc);
        else
                brush = (HBRUSH)EDIT_SEND_CTLCOLOR(hwnd, dc);

        if (!brush)
                brush = (HBRUSH)GetStockObject(WHITE_BRUSH);

	GetClientRect(hwnd, &rc);
	IntersectClipRect(dc, rc.left, rc.top, rc.right, rc.bottom);
	GetClipBox(dc, &rc);
	/*
	 *	FIXME:	specs say that we should UnrealizeObject() the brush,
	 *		but the specs of UnrealizeObject() say that we shouldn't
	 *		unrealize a stock object.  The default brush that
	 *		DefWndProc() returns is ... a stock object.
	 */
	FillRect(dc, &rc, brush);
	return -1;
}


/*********************************************************************
 *
 *	WM_GETTEXT
 *
 */
static INT EDIT_WM_GetText(EDITSTATE *es, INT count, LPARAM lParam, BOOL unicode)
{
    if(!count) return 0;

    if(unicode)
    {
	LPWSTR textW = (LPWSTR)lParam;
	strncpyW(textW, es->text, count);
	textW[count - 1] = 0; /* ensure 0 termination */
	return strlenW(textW);
    }
    else
    {
	LPSTR textA = (LPSTR)lParam;
	WideCharToMultiByte(CP_ACP, 0, es->text, -1, textA, count, NULL, NULL);
	textA[count - 1] = 0; /* ensure 0 termination */
	return strlen(textA);
    }
}

/*********************************************************************
 *
 *	WM_HSCROLL
 *
 */
static LRESULT EDIT_WM_HScroll(HWND hwnd, EDITSTATE *es, INT action, INT pos)
{
	INT dx;
	INT fw;

	if (!(es->style & ES_MULTILINE))
		return 0;

	if (!(es->style & ES_AUTOHSCROLL))
		return 0;

	dx = 0;
	fw = es->format_rect.right - es->format_rect.left;
	switch (action) {
	case SB_LINELEFT:
		TRACE("SB_LINELEFT\n");
		if (es->x_offset)
			dx = -es->char_width;
		break;
	case SB_LINERIGHT:
		TRACE("SB_LINERIGHT\n");
		if (es->x_offset < es->text_width)
			dx = es->char_width;
		break;
	case SB_PAGELEFT:
		TRACE("SB_PAGELEFT\n");
		if (es->x_offset)
			dx = -fw / HSCROLL_FRACTION / es->char_width * es->char_width;
		break;
	case SB_PAGERIGHT:
		TRACE("SB_PAGERIGHT\n");
		if (es->x_offset < es->text_width)
			dx = fw / HSCROLL_FRACTION / es->char_width * es->char_width;
		break;
	case SB_LEFT:
		TRACE("SB_LEFT\n");
		if (es->x_offset)
			dx = -es->x_offset;
		break;
	case SB_RIGHT:
		TRACE("SB_RIGHT\n");
		if (es->x_offset < es->text_width)
			dx = es->text_width - es->x_offset;
		break;
	case SB_THUMBTRACK:
		TRACE("SB_THUMBTRACK %d\n", pos);
		es->flags |= EF_HSCROLL_TRACK;
		if(es->style & WS_HSCROLL)
		    dx = pos - es->x_offset;
		else
		{
		    INT fw, new_x;
		    /* Sanity check */
		    if(pos < 0 || pos > 100) return 0;
		    /* Assume default scroll range 0-100 */
		    fw = es->format_rect.right - es->format_rect.left;
		    new_x = pos * (es->text_width - fw) / 100;
		    dx = es->text_width ? (new_x - es->x_offset) : 0;
		}
		break;
	case SB_THUMBPOSITION:
		TRACE("SB_THUMBPOSITION %d\n", pos);
		es->flags &= ~EF_HSCROLL_TRACK;
		if(GetWindowLongA( hwnd, GWL_STYLE ) & WS_HSCROLL)
		    dx = pos - es->x_offset;
		else
		{
		    INT fw, new_x;
		    /* Sanity check */
		    if(pos < 0 || pos > 100) return 0;
		    /* Assume default scroll range 0-100 */
		    fw = es->format_rect.right - es->format_rect.left;
		    new_x = pos * (es->text_width - fw) / 100;
		    dx = es->text_width ? (new_x - es->x_offset) : 0;
		}
		if (!dx) {
			/* force scroll info update */
			EDIT_UpdateScrollInfo(hwnd, es);
			EDIT_NOTIFY_PARENT(hwnd, es, EN_HSCROLL, "EN_HSCROLL");
		}
		break;
	case SB_ENDSCROLL:
		TRACE("SB_ENDSCROLL\n");
		break;
	/*
	 *	FIXME : the next two are undocumented !
	 *	Are we doing the right thing ?
	 *	At least Win 3.1 Notepad makes use of EM_GETTHUMB this way,
	 *	although it's also a regular control message.
	 */
	case EM_GETTHUMB: /* this one is used by NT notepad */
	case EM_GETTHUMB16:
	{
		LRESULT ret;
		if(GetWindowLongA( hwnd, GWL_STYLE ) & WS_HSCROLL)
		    ret = GetScrollPos(hwnd, SB_HORZ);
		else
		{
		    /* Assume default scroll range 0-100 */
		    INT fw = es->format_rect.right - es->format_rect.left;
		    ret = es->text_width ? es->x_offset * 100 / (es->text_width - fw) : 0;
		}
		TRACE("EM_GETTHUMB: returning %ld\n", ret);
		return ret;
	}
	case EM_LINESCROLL16:
		TRACE("EM_LINESCROLL16\n");
		dx = pos;
		break;

	default:
		ERR("undocumented WM_HSCROLL action %d (0x%04x), please report\n",
                    action, action);
		return 0;
	}
	if (dx)
	{
	    INT fw = es->format_rect.right - es->format_rect.left;
	    /* check if we are going to move too far */
	    if(es->x_offset + dx + fw > es->text_width)
		dx = es->text_width - fw - es->x_offset;
	    if(dx)
		EDIT_EM_LineScroll_internal(hwnd, es, dx, 0);
	}
	return 0;
}


/*********************************************************************
 *
 *	EDIT_CheckCombo
 *
 */
static BOOL EDIT_CheckCombo(HWND hwnd, EDITSTATE *es, UINT msg, INT key)
{
   HWND hLBox = es->hwndListBox;
   HWND hCombo;
   BOOL bDropped;
   int  nEUI;

   if (!hLBox)
      return FALSE;

   hCombo   = GetParent(hwnd);
   bDropped = TRUE;
   nEUI     = 0;

   TRACE_(combo)("[%04x]: handling msg %04x (%04x)\n",
       		     hwnd, (UINT16)msg, (UINT16)key);

   if (key == VK_UP || key == VK_DOWN)
   {
      if (SendMessageW(hCombo, CB_GETEXTENDEDUI, 0, 0))
         nEUI = 1;

      if (msg == WM_KEYDOWN || nEUI)
          bDropped = (BOOL)SendMessageW(hCombo, CB_GETDROPPEDSTATE, 0, 0);
   }

   switch (msg)
   {
      case WM_KEYDOWN:
         if (!bDropped && nEUI && (key == VK_UP || key == VK_DOWN))
         {
            /* make sure ComboLBox pops up */
            SendMessageW(hCombo, CB_SETEXTENDEDUI, FALSE, 0);
            key = VK_F4;
            nEUI = 2;
         }

         SendMessageW(hLBox, WM_KEYDOWN, (WPARAM)key, 0);
         break;

      case WM_SYSKEYDOWN: /* Handle Alt+up/down arrows */
         if (nEUI)
            SendMessageW(hCombo, CB_SHOWDROPDOWN, bDropped ? FALSE : TRUE, 0);
         else
            SendMessageW(hLBox, WM_KEYDOWN, (WPARAM)VK_F4, 0);
         break;
   }

   if(nEUI == 2)
      SendMessageW(hCombo, CB_SETEXTENDEDUI, TRUE, 0);

   return TRUE;
}


/*********************************************************************
 *
 *	WM_KEYDOWN
 *
 *	Handling of special keys that don't produce a WM_CHAR
 *	(i.e. non-printable keys) & Backspace & Delete
 *
 */
static LRESULT EDIT_WM_KeyDown(HWND hwnd, EDITSTATE *es, INT key)
{
	BOOL shift;
	BOOL control;

	if (GetKeyState(VK_MENU) & 0x8000)
		return 0;

	shift = GetKeyState(VK_SHIFT) & 0x8000;
	control = GetKeyState(VK_CONTROL) & 0x8000;

	switch (key) {
	case VK_F4:
	case VK_UP:
		if (EDIT_CheckCombo(hwnd, es, WM_KEYDOWN, key) || key == VK_F4)
			break;

		/* fall through */
	case VK_LEFT:
		if ((es->style & ES_MULTILINE) && (key == VK_UP))
			EDIT_MoveUp_ML(hwnd, es, shift);
		else
			if (control)
				EDIT_MoveWordBackward(hwnd, es, shift);
			else
				EDIT_MoveBackward(hwnd, es, shift);
		break;
	case VK_DOWN:
		if (EDIT_CheckCombo(hwnd, es, WM_KEYDOWN, key))
			break;
		/* fall through */
	case VK_RIGHT:
		if ((es->style & ES_MULTILINE) && (key == VK_DOWN))
			EDIT_MoveDown_ML(hwnd, es, shift);
		else if (control)
			EDIT_MoveWordForward(hwnd, es, shift);
		else
			EDIT_MoveForward(hwnd, es, shift);
		break;
	case VK_HOME:
		EDIT_MoveHome(hwnd, es, shift);
		break;
	case VK_END:
		EDIT_MoveEnd(hwnd, es, shift);
		break;
	case VK_PRIOR:
		if (es->style & ES_MULTILINE)
			EDIT_MovePageUp_ML(hwnd, es, shift);
		else
			EDIT_CheckCombo(hwnd, es, WM_KEYDOWN, key);
		break;
	case VK_NEXT:
		if (es->style & ES_MULTILINE)
			EDIT_MovePageDown_ML(hwnd, es, shift);
		else
			EDIT_CheckCombo(hwnd, es, WM_KEYDOWN, key);
		break;
	case VK_DELETE:
		if (!(es->style & ES_READONLY) && !(shift && control)) {
			if (es->selection_start != es->selection_end) {
				if (shift)
					EDIT_WM_Cut(hwnd, es);
				else
					EDIT_WM_Clear(hwnd, es);
			} else {
				if (shift) {
					/* delete character left of caret */
					EDIT_EM_SetSel(hwnd, es, (UINT)-1, 0, FALSE);
					EDIT_MoveBackward(hwnd, es, TRUE);
					EDIT_WM_Clear(hwnd, es);
				} else if (control) {
					/* delete to end of line */
					EDIT_EM_SetSel(hwnd, es, (UINT)-1, 0, FALSE);
					EDIT_MoveEnd(hwnd, es, TRUE);
					EDIT_WM_Clear(hwnd, es);
				} else {
					/* delete character right of caret */
					EDIT_EM_SetSel(hwnd, es, (UINT)-1, 0, FALSE);
					EDIT_MoveForward(hwnd, es, TRUE);
					EDIT_WM_Clear(hwnd, es);
				}
			}
		}
		break;
	case VK_INSERT:
		if (shift) {
			if (!(es->style & ES_READONLY))
				EDIT_WM_Paste(hwnd, es);
		} else if (control)
			EDIT_WM_Copy(hwnd, es);
		break;
	case VK_RETURN:
	    /* If the edit doesn't want the return send a message to the default object */
	    if(!(es->style & ES_WANTRETURN))
	    {
		HWND hwndParent = GetParent(hwnd);
		DWORD dw = SendMessageW( hwndParent, DM_GETDEFID, 0, 0 );
		if (HIWORD(dw) == DC_HASDEFID)
		{
		    SendMessageW( hwndParent, WM_COMMAND, 
				  MAKEWPARAM( LOWORD(dw), BN_CLICKED ),
 			      (LPARAM)GetDlgItem( hwndParent, LOWORD(dw) ) );
		}
	    }
	    break;
	}
	return 0;
}


/*********************************************************************
 *
 *	WM_KILLFOCUS
 *
 */
static LRESULT EDIT_WM_KillFocus(HWND hwnd, EDITSTATE *es)
{
	es->flags &= ~EF_FOCUSED;
	DestroyCaret();
	if(!(es->style & ES_NOHIDESEL))
		EDIT_InvalidateText(hwnd, es, es->selection_start, es->selection_end);
	EDIT_NOTIFY_PARENT(hwnd, es, EN_KILLFOCUS, "EN_KILLFOCUS");
	return 0;
}


/*********************************************************************
 *
 *	WM_LBUTTONDBLCLK
 *
 *	The caret position has been set on the WM_LBUTTONDOWN message
 *
 */
static LRESULT EDIT_WM_LButtonDblClk(HWND hwnd, EDITSTATE *es)
{
	INT s;
	INT e = es->selection_end;
	INT l;
	INT li;
	INT ll;

	if (!(es->flags & EF_FOCUSED))
		return 0;

	l = EDIT_EM_LineFromChar(es, e);
	li = EDIT_EM_LineIndex(es, l);
	ll = EDIT_EM_LineLength(es, e);
	s = li + EDIT_CallWordBreakProc(es, li, e - li, ll, WB_LEFT);
	e = li + EDIT_CallWordBreakProc(es, li, e - li, ll, WB_RIGHT);
	EDIT_EM_SetSel(hwnd, es, s, e, FALSE);
	EDIT_EM_ScrollCaret(hwnd, es);
	return 0;
}


/*********************************************************************
 *
 *	WM_LBUTTONDOWN
 *
 */
static LRESULT EDIT_WM_LButtonDown(HWND hwnd, EDITSTATE *es, DWORD keys, INT x, INT y)
{
	INT e;
	BOOL after_wrap;

	if (!(es->flags & EF_FOCUSED))
		return 0;

	es->bCaptureState = TRUE;
	SetCapture(hwnd);
	EDIT_ConfinePoint(es, &x, &y);
	e = EDIT_CharFromPos(hwnd, es, x, y, &after_wrap);
	EDIT_EM_SetSel(hwnd, es, (keys & MK_SHIFT) ? es->selection_start : e, e, after_wrap);
	EDIT_EM_ScrollCaret(hwnd, es);
	es->region_posx = es->region_posy = 0;
	SetTimer(hwnd, 0, 100, NULL);
	return 0;
}


/*********************************************************************
 *
 *	WM_LBUTTONUP
 *
 */
static LRESULT EDIT_WM_LButtonUp(HWND hwndSelf, EDITSTATE *es)
{
	if (es->bCaptureState && GetCapture() == hwndSelf) {
		KillTimer(hwndSelf, 0);
		ReleaseCapture();
	}
	es->bCaptureState = FALSE;
	return 0;
}


/*********************************************************************
 *
 *	WM_MBUTTONDOWN
 *
 */
static LRESULT EDIT_WM_MButtonDown(HWND hwnd)
{  
    SendMessageW(hwnd,WM_PASTE,0,0);  
    return 0;
}


/*********************************************************************
 *
 *	WM_MOUSEMOVE
 *
 */
static LRESULT EDIT_WM_MouseMove(HWND hwnd, EDITSTATE *es, INT x, INT y)
{
	INT e;
	BOOL after_wrap;
	INT prex, prey;

	if (GetCapture() != hwnd)
		return 0;

	/*
	 *	FIXME: gotta do some scrolling if outside client
	 *		area.  Maybe reset the timer ?
	 */
	prex = x; prey = y;
	EDIT_ConfinePoint(es, &x, &y);
	es->region_posx = (prex < x) ? -1 : ((prex > x) ? 1 : 0);
	es->region_posy = (prey < y) ? -1 : ((prey > y) ? 1 : 0);
	e = EDIT_CharFromPos(hwnd, es, x, y, &after_wrap);
	EDIT_EM_SetSel(hwnd, es, es->selection_start, e, after_wrap);
	return 0;
}


/*********************************************************************
 *
 *	WM_NCCREATE
 *
 * See also EDIT_WM_StyleChanged
 */
static LRESULT EDIT_WM_NCCreate(HWND hwnd, DWORD style, HWND hwndParent, BOOL unicode)
{
	EDITSTATE *es;
	UINT alloc_size;

	TRACE("Creating %s edit control, style = %08lx\n",
		unicode ? "Unicode" : "ANSI", style);

	if (!(es = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*es))))
		return FALSE;
        SetWindowLongA( hwnd, 0, (LONG)es );

       /*
        *      Note: since the EDITSTATE has not been fully initialized yet,
        *            we can't use any API calls that may send
        *            WM_XXX messages before WM_NCCREATE is completed.
        */

 	es->is_unicode = unicode;
 	es->style = style;

        es->bEnableState = !(style & WS_DISABLED);

	/* Save parent, which will be notified by EN_* messages */
	es->hwndParent = hwndParent;

	if (es->style & ES_COMBO)
	   es->hwndListBox = GetDlgItem(hwndParent, ID_CB_LISTBOX);

        /* Number overrides lowercase overrides uppercase (at least it
         * does in Win95).  However I'll bet that ES_NUMBER would be
         * invalid under Win 3.1.
         */
        if (es->style & ES_NUMBER) {
                ; /* do not override the ES_NUMBER */
        }  else if (es->style & ES_LOWERCASE) {
                es->style &= ~ES_UPPERCASE;
        }
	if (es->style & ES_MULTILINE) {
		es->buffer_limit = BUFLIMIT_MULTI;
		if (es->style & WS_VSCROLL)
			es->style |= ES_AUTOVSCROLL;
		if (es->style & WS_HSCROLL)
			es->style |= ES_AUTOHSCROLL;
		es->style &= ~ES_PASSWORD;
		if ((es->style & ES_CENTER) || (es->style & ES_RIGHT)) {
                        /* Confirmed - RIGHT overrides CENTER */
			if (es->style & ES_RIGHT)
				es->style &= ~ES_CENTER;
			es->style &= ~WS_HSCROLL;
			es->style &= ~ES_AUTOHSCROLL;
		}

		/* FIXME: for now, all multi line controls are AUTOVSCROLL */
		es->style |= ES_AUTOVSCROLL;
	} else {
		es->buffer_limit = BUFLIMIT_SINGLE;
                if (WIN31_LOOK == TWEAK_WineLook ||
                    WIN95_LOOK == TWEAK_WineLook) {
		        es->style &= ~ES_CENTER;
		        es->style &= ~ES_RIGHT;
                } else {
			if (es->style & ES_RIGHT)
				es->style &= ~ES_CENTER;
                }
		es->style &= ~WS_HSCROLL;
		es->style &= ~WS_VSCROLL;
		es->style &= ~ES_AUTOVSCROLL;
		es->style &= ~ES_WANTRETURN;
		if (es->style & ES_PASSWORD)
			es->password_char = '*';

		/* FIXME: for now, all single line controls are AUTOHSCROLL */
		es->style |= ES_AUTOHSCROLL;
	}

	alloc_size = ROUND_TO_GROW((es->buffer_size + 1) * sizeof(WCHAR));
	if(!(es->hloc32W = LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT, alloc_size)))
	    return FALSE;
	es->buffer_size = LocalSize(es->hloc32W)/sizeof(WCHAR) - 1;

	if (!(es->undo_text = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (es->buffer_size + 1) * sizeof(WCHAR))))
		return FALSE;
	es->undo_buffer_size = es->buffer_size;

	if (es->style & ES_MULTILINE)
		if (!(es->first_line_def = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LINEDEF))))
			return FALSE;
	es->line_count = 1;

	/*
	 * In Win95 look and feel, the WS_BORDER style is replaced by the 
	 * WS_EX_CLIENTEDGE style for the edit control. This gives the edit 
	 * control a non client area.  Not always.  This coordinates in some
         * way with the window creation code in dialog.c  When making 
         * modifications please ensure that the code still works for edit
         * controls created directly with style 0x50800000, exStyle 0 (
         * which should have a single pixel border)
	 */
	if (TWEAK_WineLook != WIN31_LOOK)
	{
	  es->style      &= ~WS_BORDER;
	}
	else
	{
	  if ((es->style & WS_BORDER) && !(es->style & WS_DLGFRAME))
              SetWindowLongA( hwnd, GWL_STYLE,
                              GetWindowLongA( hwnd, GWL_STYLE ) & ~WS_BORDER );
	}

	return TRUE;
}

/*********************************************************************
 *
 *	WM_PAINT
 *
 */
static void EDIT_WM_Paint(HWND hwnd, EDITSTATE *es, WPARAM wParam)
{
	PAINTSTRUCT ps;
	INT i;
	HDC dc;
	HFONT old_font = 0;
	RECT rc;
	RECT rcLine;
	RECT rcRgn;
	BOOL rev = es->bEnableState &&
				((es->flags & EF_FOCUSED) ||
					(es->style & ES_NOHIDESEL));
        if (!wParam)
            dc = BeginPaint(hwnd, &ps);
        else
            dc = (HDC) wParam;
	if(es->style & WS_BORDER) {
		GetClientRect(hwnd, &rc);
		if(es->style & ES_MULTILINE) {
			if(es->style & WS_HSCROLL) rc.bottom++;
			if(es->style & WS_VSCROLL) rc.right++;
		}
		Rectangle(dc, rc.left, rc.top, rc.right, rc.bottom);
	}
	IntersectClipRect(dc, es->format_rect.left,
				es->format_rect.top,
				es->format_rect.right,
				es->format_rect.bottom);
	if (es->style & ES_MULTILINE) {
		GetClientRect(hwnd, &rc);
		IntersectClipRect(dc, rc.left, rc.top, rc.right, rc.bottom);
	}
	if (es->font)
		old_font = SelectObject(dc, es->font);
        if ( get_app_version() >= 0x40000 &&(
                    !es->bEnableState || (es->style & ES_READONLY)))
                EDIT_SEND_CTLCOLORSTATIC(hwnd, dc);
        else
                EDIT_SEND_CTLCOLOR(hwnd, dc);

	if (!es->bEnableState)
		SetTextColor(dc, GetSysColor(COLOR_GRAYTEXT));
	GetClipBox(dc, &rcRgn);
	if (es->style & ES_MULTILINE) {
		INT vlc = (es->format_rect.bottom - es->format_rect.top) / es->line_height;
		for (i = es->y_offset ; i <= min(es->y_offset + vlc, es->y_offset + es->line_count - 1) ; i++) {
			EDIT_GetLineRect(hwnd, es, i, 0, -1, &rcLine);
			if (IntersectRect(&rc, &rcRgn, &rcLine))
				EDIT_PaintLine(hwnd, es, dc, i, rev);
		}
	} else {
		EDIT_GetLineRect(hwnd, es, 0, 0, -1, &rcLine);
		if (IntersectRect(&rc, &rcRgn, &rcLine))
			EDIT_PaintLine(hwnd, es, dc, 0, rev);
	}
	if (es->font)
		SelectObject(dc, old_font);

        if (!wParam)
            EndPaint(hwnd, &ps);
}


/*********************************************************************
 *
 *	WM_PASTE
 *
 */
static void EDIT_WM_Paste(HWND hwnd, EDITSTATE *es)
{
	HGLOBAL hsrc;
	LPWSTR src;

	/* Protect read-only edit control from modification */
	if(es->style & ES_READONLY)
	    return;

	OpenClipboard(hwnd);
	if ((hsrc = GetClipboardData(CF_UNICODETEXT))) {
		src = (LPWSTR)GlobalLock(hsrc);
		EDIT_EM_ReplaceSel(hwnd, es, TRUE, src, TRUE);
		GlobalUnlock(hsrc);
	}
	CloseClipboard();
}


/*********************************************************************
 *
 *	WM_SETFOCUS
 *
 */
static void EDIT_WM_SetFocus(HWND hwnd, EDITSTATE *es)
{
	es->flags |= EF_FOCUSED;
	CreateCaret(hwnd, 0, 2, es->line_height);
	EDIT_SetCaretPos(hwnd, es, es->selection_end,
			 es->flags & EF_AFTER_WRAP);
	if(!(es->style & ES_NOHIDESEL))
		EDIT_InvalidateText(hwnd, es, es->selection_start, es->selection_end);
	ShowCaret(hwnd);
	EDIT_NOTIFY_PARENT(hwnd, es, EN_SETFOCUS, "EN_SETFOCUS");
}


/*********************************************************************
 *
 *	WM_SETFONT
 *
 * With Win95 look the margins are set to default font value unless 
 * the system font (font == 0) is being set, in which case they are left
 * unchanged.
 *
 */
static void EDIT_WM_SetFont(HWND hwnd, EDITSTATE *es, HFONT font, BOOL redraw)
{
	TEXTMETRICW tm;
	HDC dc;
	HFONT old_font = 0;
	RECT r;

	es->font = font;
	dc = GetDC(hwnd);
	if (font)
		old_font = SelectObject(dc, font);
	GetTextMetricsW(dc, &tm);
	es->line_height = tm.tmHeight;
	es->char_width = tm.tmAveCharWidth;
	if (font)
		SelectObject(dc, old_font);
	ReleaseDC(hwnd, dc);
	if (font && (TWEAK_WineLook > WIN31_LOOK))
		EDIT_EM_SetMargins(es, EC_LEFTMARGIN | EC_RIGHTMARGIN,
				   EC_USEFONTINFO, EC_USEFONTINFO);

	/* Force the recalculation of the format rect for each font change */
	GetClientRect(hwnd, &r);
	EDIT_SetRectNP(hwnd, es, &r);

	if (es->style & ES_MULTILINE)
		EDIT_BuildLineDefs_ML(hwnd, es, 0, strlenW(es->text), 0, (HRGN)0);
	else
	    EDIT_CalcLineWidth_SL(hwnd, es);

	if (redraw)
		EDIT_UpdateText(hwnd, es, NULL, TRUE);
	if (es->flags & EF_FOCUSED) {
		DestroyCaret();
		CreateCaret(hwnd, 0, 2, es->line_height);
		EDIT_SetCaretPos(hwnd, es, es->selection_end,
				 es->flags & EF_AFTER_WRAP);
		ShowCaret(hwnd);
	}
}


/*********************************************************************
 *
 *	WM_SETTEXT
 *
 * NOTES
 *  For multiline controls (ES_MULTILINE), reception of WM_SETTEXT triggers:
 *  The modified flag is reset. No notifications are sent.
 *
 *  For single-line controls, reception of WM_SETTEXT triggers:
 *  The modified flag is reset. EN_UPDATE and EN_CHANGE notifications are sent.
 *
 */
static void EDIT_WM_SetText(HWND hwnd, EDITSTATE *es, LPARAM lParam, BOOL unicode)
{
    LPWSTR text = NULL;

    if(unicode)
	text = (LPWSTR)lParam;
    else if (lParam)
    {
	LPCSTR textA = (LPCSTR)lParam;
	INT countW = MultiByteToWideChar(CP_ACP, 0, textA, -1, NULL, 0);
	if((text = HeapAlloc(GetProcessHeap(), 0, countW * sizeof(WCHAR))))
	    MultiByteToWideChar(CP_ACP, 0, textA, -1, text, countW);
    }

	EDIT_EM_SetSel(hwnd, es, 0, (UINT)-1, FALSE);
	if (text) {
		TRACE("%s\n", debugstr_w(text));
		EDIT_EM_ReplaceSel(hwnd, es, FALSE, text, FALSE);
		if(!unicode)
		    HeapFree(GetProcessHeap(), 0, text);
	} else {
		static const WCHAR empty_stringW[] = {0};
		TRACE("<NULL>\n");
		EDIT_EM_ReplaceSel(hwnd, es, FALSE, empty_stringW, FALSE);
	}
	es->x_offset = 0;
	es->flags &= ~EF_MODIFIED;
	EDIT_EM_SetSel(hwnd, es, 0, 0, FALSE);
        /* Send the notification after the selection start and end have been set
         * edit control doesn't send notification on WM_SETTEXT
         * if it is multiline, or it is part of combobox
         */
	if( !((es->style & ES_MULTILINE) || es->hwndListBox))
       {
	    EDIT_NOTIFY_PARENT(hwnd, es, EN_CHANGE, "EN_CHANGE");
           EDIT_NOTIFY_PARENT(hwnd, es, EN_UPDATE, "EN_UPDATE");
       }
	EDIT_EM_ScrollCaret(hwnd, es);
}


/*********************************************************************
 *
 *	WM_SIZE
 *
 */
static void EDIT_WM_Size(HWND hwnd, EDITSTATE *es, UINT action, INT width, INT height)
{
	if ((action == SIZE_MAXIMIZED) || (action == SIZE_RESTORED)) {
		RECT rc;
		TRACE("width = %d, height = %d\n", width, height);
		SetRect(&rc, 0, 0, width, height);
		EDIT_SetRectNP(hwnd, es, &rc);
		EDIT_UpdateText(hwnd, es, NULL, TRUE);
	}
}


/*********************************************************************
 *
 *	WM_STYLECHANGED
 *
 * This message is sent by SetWindowLong on having changed either the Style
 * or the extended style.
 *
 * We ensure that the window's version of the styles and the EDITSTATE's agree.
 *
 * See also EDIT_WM_NCCreate
 *
 * It appears that the Windows version of the edit control allows the style
 * (as retrieved by GetWindowLong) to be any value and maintains an internal
 * style variable which will generally be different.  In this function we 
 * update the internal style based on what changed in the externally visible
 * style.
 *
 * Much of this content as based upon the MSDN, especially:
 *  Platform SDK Documentation -> User Interface Services ->
 *      Windows User Interface -> Edit Controls -> Edit Control Reference ->
 *      Edit Control Styles
 */
static LRESULT  EDIT_WM_StyleChanged (HWND hwnd,
                                      EDITSTATE *es,
                                      WPARAM which,
                                      const STYLESTRUCT *style)
{
        if (GWL_STYLE == which) {
                DWORD style_change_mask;
                DWORD new_style;
                /* Only a subset of changes can be applied after the control
                 * has been created.
                 */
                style_change_mask = ES_UPPERCASE | ES_LOWERCASE |
                                    ES_NUMBER;
                if (es->style & ES_MULTILINE) 
                        style_change_mask |= ES_WANTRETURN;

                new_style = style->styleNew & style_change_mask;

                /* Number overrides lowercase overrides uppercase (at least it
                 * does in Win95).  However I'll bet that ES_NUMBER would be
                 * invalid under Win 3.1.
                 */
                if (new_style & ES_NUMBER) {
                        ; /* do not override the ES_NUMBER */
                }  else if (new_style & ES_LOWERCASE) {
                        new_style &= ~ES_UPPERCASE;
                }
        
                es->style = (es->style & ~style_change_mask) | new_style;
        } else if (GWL_EXSTYLE == which) {
                ; /* FIXME - what is needed here */
        } else {
                WARN ("Invalid style change %d\n",which);
        }

        return 0;
}

/*********************************************************************
 *
 *	WM_SYSKEYDOWN
 *
 */
static LRESULT EDIT_WM_SysKeyDown(HWND hwnd, EDITSTATE *es, INT key, DWORD key_data)
{
	if ((key == VK_BACK) && (key_data & 0x2000)) {
		if (EDIT_EM_CanUndo(es))
			EDIT_EM_Undo(hwnd, es);
		return 0;
	} else if (key == VK_UP || key == VK_DOWN) {
		if (EDIT_CheckCombo(hwnd, es, WM_SYSKEYDOWN, key))
			return 0;
	}
	return DefWindowProcW(hwnd, WM_SYSKEYDOWN, (WPARAM)key, (LPARAM)key_data);
}


/*********************************************************************
 *
 *	WM_TIMER
 *
 */
static void EDIT_WM_Timer(HWND hwnd, EDITSTATE *es)
{
	if (es->region_posx < 0) {
		EDIT_MoveBackward(hwnd, es, TRUE);
	} else if (es->region_posx > 0) {
		EDIT_MoveForward(hwnd, es, TRUE);
	}
/*
 *	FIXME: gotta do some vertical scrolling here, like
 *		EDIT_EM_LineScroll(hwnd, 0, 1);
 */
}

/*********************************************************************
 *
 *	WM_VSCROLL
 *
 */
static LRESULT EDIT_WM_VScroll(HWND hwnd, EDITSTATE *es, INT action, INT pos)
{
	INT dy;

	if (!(es->style & ES_MULTILINE))
		return 0;

	if (!(es->style & ES_AUTOVSCROLL))
		return 0;

	dy = 0;
	switch (action) {
	case SB_LINEUP:
	case SB_LINEDOWN:
	case SB_PAGEUP:
	case SB_PAGEDOWN:
		TRACE("action %d\n", action);
		EDIT_EM_Scroll(hwnd, es, action);
		return 0;
	case SB_TOP:
		TRACE("SB_TOP\n");
		dy = -es->y_offset;
		break;
	case SB_BOTTOM:
		TRACE("SB_BOTTOM\n");
		dy = es->line_count - 1 - es->y_offset;
		break;
	case SB_THUMBTRACK:
		TRACE("SB_THUMBTRACK %d\n", pos);
		es->flags |= EF_VSCROLL_TRACK;
		if(es->style & WS_VSCROLL)
		    dy = pos - es->y_offset;
		else
		{
		    /* Assume default scroll range 0-100 */
		    INT vlc, new_y;
		    /* Sanity check */
		    if(pos < 0 || pos > 100) return 0;
		    vlc = (es->format_rect.bottom - es->format_rect.top) / es->line_height;
		    new_y = pos * (es->line_count - vlc) / 100;
		    dy = es->line_count ? (new_y - es->y_offset) : 0;
		    TRACE("line_count=%d, y_offset=%d, pos=%d, dy = %d\n",
			    es->line_count, es->y_offset, pos, dy);
		}
		break;
	case SB_THUMBPOSITION:
		TRACE("SB_THUMBPOSITION %d\n", pos);
		es->flags &= ~EF_VSCROLL_TRACK;
		if(es->style & WS_VSCROLL)
		    dy = pos - es->y_offset;
		else
		{
		    /* Assume default scroll range 0-100 */
		    INT vlc, new_y;
		    /* Sanity check */
		    if(pos < 0 || pos > 100) return 0;
		    vlc = (es->format_rect.bottom - es->format_rect.top) / es->line_height;
		    new_y = pos * (es->line_count - vlc) / 100;
		    dy = es->line_count ? (new_y - es->y_offset) : 0;
		    TRACE("line_count=%d, y_offset=%d, pos=%d, dy = %d\n",
			    es->line_count, es->y_offset, pos, dy);
		}
		if (!dy)
		{
			/* force scroll info update */
			EDIT_UpdateScrollInfo(hwnd, es);
			EDIT_NOTIFY_PARENT(hwnd, es, EN_VSCROLL, "EN_VSCROLL");
		}
		break;
	case SB_ENDSCROLL:
		TRACE("SB_ENDSCROLL\n");
		break;
	/*
	 *	FIXME : the next two are undocumented !
	 *	Are we doing the right thing ?
	 *	At least Win 3.1 Notepad makes use of EM_GETTHUMB this way,
	 *	although it's also a regular control message.
	 */
	case EM_GETTHUMB: /* this one is used by NT notepad */
	case EM_GETTHUMB16:
	{
		LRESULT ret;
		if(GetWindowLongA( hwnd, GWL_STYLE ) & WS_VSCROLL)
		    ret = GetScrollPos(hwnd, SB_VERT);
		else
		{
		    /* Assume default scroll range 0-100 */
		    INT vlc = (es->format_rect.bottom - es->format_rect.top) / es->line_height;
		    ret = es->line_count ? es->y_offset * 100 / (es->line_count - vlc) : 0;
		}
		TRACE("EM_GETTHUMB: returning %ld\n", ret);
		return ret;
	}
	case EM_LINESCROLL16:
		TRACE("EM_LINESCROLL16 %d\n", pos);
		dy = pos;
		break;

	default:
		ERR("undocumented WM_VSCROLL action %d (0x%04x), please report\n",
                    action, action);
		return 0;
	}
	if (dy)
		EDIT_EM_LineScroll(hwnd, es, 0, dy);
	return 0;
}

/*********************************************************************
 *
 *	EDIT_UpdateText
 *
 */
static void EDIT_UpdateTextRegion(HWND hwnd, EDITSTATE *es, HRGN hrgn, BOOL bErase)
{
    if (es->flags & EF_UPDATE) EDIT_NOTIFY_PARENT(hwnd, es, EN_UPDATE, "EN_UPDATE");
    InvalidateRgn(hwnd, hrgn, bErase);
}


/*********************************************************************
 *
 *	EDIT_UpdateText
 *
 */
static void EDIT_UpdateText(HWND hwnd, EDITSTATE *es, LPRECT rc, BOOL bErase)
{
    if (es->flags & EF_UPDATE) EDIT_NOTIFY_PARENT(hwnd, es, EN_UPDATE, "EN_UPDATE");
    InvalidateRect(hwnd, rc, bErase);
}
