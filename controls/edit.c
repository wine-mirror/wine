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
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(edit);
DECLARE_DEBUG_CHANNEL(combo);
DECLARE_DEBUG_CHANNEL(relay);

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
#define EF_UPDATE		0x0004	/* notify parent of changed state on next WM_PAINT */
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
#define EDIT_SEND_CTLCOLORSTATIC(wnd,hdc) \
	(SendMessageW((wnd)->parent->hwndSelf, WM_CTLCOLORSTATIC, \
			(WPARAM)(hdc), (LPARAM)(wnd)->hwndSelf))
#define EDIT_SEND_CTLCOLOR(wnd,hdc) \
	(SendMessageW((wnd)->parent->hwndSelf, WM_CTLCOLOREDIT, \
			(WPARAM)(hdc), (LPARAM)(wnd)->hwndSelf))
#define EDIT_NOTIFY_PARENT(wnd, wNotifyCode, str) \
	do {DPRINTF_EDIT_NOTIFY((wnd)->parent->hwndSelf, str); \
	SendMessageW((wnd)->parent->hwndSelf, WM_COMMAND, \
		     MAKEWPARAM((wnd)->wIDmenu, wNotifyCode), \
		     (LPARAM)(wnd)->hwndSelf);} while(0)
#define DPRINTF_EDIT_MSG16(str) \
	TRACE(\
		     "16 bit : " str ": hwnd=%08x, wParam=%08x, lParam=%08x\n", \
		     (UINT)wnd->hwndSelf, (UINT)wParam, (UINT)lParam)
#define DPRINTF_EDIT_MSG32(str) \
	TRACE(\
		     "32 bit %c : " str ": hwnd=%08x, wParam=%08x, lParam=%08x\n", \
		     unicode ? 'W' : 'A', \
		     (UINT)wnd->hwndSelf, (UINT)wParam, (UINT)lParam)

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
static inline void	EDIT_WM_Clear(WND *wnd, EDITSTATE *es);
static inline void	EDIT_WM_Cut(WND *wnd, EDITSTATE *es);

/*
 *	Helper functions only valid for one type of control
 */
static void	EDIT_BuildLineDefs_ML(WND *wnd, EDITSTATE *es);
static void	EDIT_CalcLineWidth_SL(WND *wnd, EDITSTATE *es);
static LPWSTR	EDIT_GetPasswordPointer_SL(EDITSTATE *es);
static void	EDIT_MoveDown_ML(WND *wnd, EDITSTATE *es, BOOL extend);
static void	EDIT_MovePageDown_ML(WND *wnd, EDITSTATE *es, BOOL extend);
static void	EDIT_MovePageUp_ML(WND *wnd, EDITSTATE *es, BOOL extend);
static void	EDIT_MoveUp_ML(WND *wnd, EDITSTATE *es, BOOL extend);
/*
 *	Helper functions valid for both single line _and_ multi line controls
 */
static INT	EDIT_CallWordBreakProc(EDITSTATE *es, INT start, INT index, INT count, INT action);
static INT	EDIT_CharFromPos(WND *wnd, EDITSTATE *es, INT x, INT y, LPBOOL after_wrap);
static void	EDIT_ConfinePoint(EDITSTATE *es, LPINT x, LPINT y);
static void	EDIT_GetLineRect(WND *wnd, EDITSTATE *es, INT line, INT scol, INT ecol, LPRECT rc);
static void	EDIT_InvalidateText(WND *wnd, EDITSTATE *es, INT start, INT end);
static void	EDIT_LockBuffer(WND *wnd, EDITSTATE *es);
static BOOL	EDIT_MakeFit(WND *wnd, EDITSTATE *es, UINT size);
static BOOL	EDIT_MakeUndoFit(EDITSTATE *es, UINT size);
static void	EDIT_MoveBackward(WND *wnd, EDITSTATE *es, BOOL extend);
static void	EDIT_MoveEnd(WND *wnd, EDITSTATE *es, BOOL extend);
static void	EDIT_MoveForward(WND *wnd, EDITSTATE *es, BOOL extend);
static void	EDIT_MoveHome(WND *wnd, EDITSTATE *es, BOOL extend);
static void	EDIT_MoveWordBackward(WND *wnd, EDITSTATE *es, BOOL extend);
static void	EDIT_MoveWordForward(WND *wnd, EDITSTATE *es, BOOL extend);
static void	EDIT_PaintLine(WND *wnd, EDITSTATE *es, HDC hdc, INT line, BOOL rev);
static INT	EDIT_PaintText(EDITSTATE *es, HDC hdc, INT x, INT y, INT line, INT col, INT count, BOOL rev);
static void	EDIT_SetCaretPos(WND *wnd, EDITSTATE *es, INT pos, BOOL after_wrap); 
static void	EDIT_SetRectNP(WND *wnd, EDITSTATE *es, LPRECT lprc);
static void	EDIT_UnlockBuffer(WND *wnd, EDITSTATE *es, BOOL force);
static void	EDIT_UpdateScrollInfo(WND *wnd, EDITSTATE *es);
static INT CALLBACK EDIT_WordBreakProc(LPWSTR s, INT index, INT count, INT action);
/*
 *	EM_XXX message handlers
 */
static LRESULT	EDIT_EM_CharFromPos(WND *wnd, EDITSTATE *es, INT x, INT y);
static BOOL	EDIT_EM_FmtLines(EDITSTATE *es, BOOL add_eol);
static HLOCAL	EDIT_EM_GetHandle(EDITSTATE *es);
static HLOCAL16	EDIT_EM_GetHandle16(WND *wnd, EDITSTATE *es);
static INT	EDIT_EM_GetLine(EDITSTATE *es, INT line, LPWSTR lpch);
static LRESULT	EDIT_EM_GetSel(EDITSTATE *es, LPUINT start, LPUINT end);
static LRESULT	EDIT_EM_GetThumb(WND *wnd, EDITSTATE *es);
static INT	EDIT_EM_LineFromChar(EDITSTATE *es, INT index);
static INT	EDIT_EM_LineIndex(EDITSTATE *es, INT line);
static INT	EDIT_EM_LineLength(EDITSTATE *es, INT index);
static BOOL	EDIT_EM_LineScroll(WND *wnd, EDITSTATE *es, INT dx, INT dy);
static BOOL	EDIT_EM_LineScroll_internal(WND *wnd, EDITSTATE *es, INT dx, INT dy);
static LRESULT	EDIT_EM_PosFromChar(WND *wnd, EDITSTATE *es, INT index, BOOL after_wrap);
static void	EDIT_EM_ReplaceSel(WND *wnd, EDITSTATE *es, BOOL can_undo, LPCWSTR lpsz_replace, BOOL send_update);
static LRESULT	EDIT_EM_Scroll(WND *wnd, EDITSTATE *es, INT action);
static void	EDIT_EM_ScrollCaret(WND *wnd, EDITSTATE *es);
static void	EDIT_EM_SetHandle(WND *wnd, EDITSTATE *es, HLOCAL hloc);
static void	EDIT_EM_SetHandle16(WND *wnd, EDITSTATE *es, HLOCAL16 hloc);
static void	EDIT_EM_SetLimitText(EDITSTATE *es, INT limit);
static void	EDIT_EM_SetMargins(EDITSTATE *es, INT action, INT left, INT right);
static void	EDIT_EM_SetPasswordChar(WND *wnd, EDITSTATE *es, WCHAR c);
static void	EDIT_EM_SetSel(WND *wnd, EDITSTATE *es, UINT start, UINT end, BOOL after_wrap);
static BOOL	EDIT_EM_SetTabStops(EDITSTATE *es, INT count, LPINT tabs);
static BOOL	EDIT_EM_SetTabStops16(EDITSTATE *es, INT count, LPINT16 tabs);
static void	EDIT_EM_SetWordBreakProc(WND *wnd, EDITSTATE *es, LPARAM lParam);
static void	EDIT_EM_SetWordBreakProc16(WND *wnd, EDITSTATE *es, EDITWORDBREAKPROC16 wbp);
static BOOL	EDIT_EM_Undo(WND *wnd, EDITSTATE *es);
/*
 *	WM_XXX message handlers
 */
static void	EDIT_WM_Char(WND *wnd, EDITSTATE *es, WCHAR c);
static void	EDIT_WM_Command(WND *wnd, EDITSTATE *es, INT code, INT id, HWND conrtol);
static void	EDIT_WM_ContextMenu(WND *wnd, EDITSTATE *es, INT x, INT y);
static void	EDIT_WM_Copy(WND *wnd, EDITSTATE *es);
static LRESULT	EDIT_WM_Create(WND *wnd, EDITSTATE *es, LPCWSTR name);
static void	EDIT_WM_Destroy(WND *wnd, EDITSTATE *es);
static LRESULT	EDIT_WM_EraseBkGnd(WND *wnd, EDITSTATE *es, HDC dc);
static INT	EDIT_WM_GetText(EDITSTATE *es, INT count, LPARAM lParam, BOOL unicode);
static LRESULT	EDIT_WM_HScroll(WND *wnd, EDITSTATE *es, INT action, INT pos);
static LRESULT	EDIT_WM_KeyDown(WND *wnd, EDITSTATE *es, INT key);
static LRESULT	EDIT_WM_KillFocus(WND *wnd, EDITSTATE *es);
static LRESULT	EDIT_WM_LButtonDblClk(WND *wnd, EDITSTATE *es);
static LRESULT	EDIT_WM_LButtonDown(WND *wnd, EDITSTATE *es, DWORD keys, INT x, INT y);
static LRESULT	EDIT_WM_LButtonUp(HWND hwndSelf, EDITSTATE *es);
static LRESULT	EDIT_WM_MButtonDown(WND *wnd);
static LRESULT	EDIT_WM_MouseMove(WND *wnd, EDITSTATE *es, INT x, INT y);
static LRESULT	EDIT_WM_NCCreate(WND *wnd, DWORD style, HWND hwndParent, BOOL unicode);
static void	EDIT_WM_Paint(WND *wnd, EDITSTATE *es, WPARAM wParam);
static void	EDIT_WM_Paste(WND *wnd, EDITSTATE *es);
static void	EDIT_WM_SetFocus(WND *wnd, EDITSTATE *es);
static void	EDIT_WM_SetFont(WND *wnd, EDITSTATE *es, HFONT font, BOOL redraw);
static void	EDIT_WM_SetText(WND *wnd, EDITSTATE *es, LPARAM lParam, BOOL unicode);
static void	EDIT_WM_Size(WND *wnd, EDITSTATE *es, UINT action, INT width, INT height);
static LRESULT	EDIT_WM_SysKeyDown(WND *wnd, EDITSTATE *es, INT key, DWORD key_data);
static void	EDIT_WM_Timer(WND *wnd, EDITSTATE *es);
static LRESULT	EDIT_WM_VScroll(WND *wnd, EDITSTATE *es, INT action, INT pos);
static void EDIT_UpdateText(WND *wnd, LPRECT rc, BOOL bErase);

LRESULT WINAPI EditWndProcA(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI EditWndProcW(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

/*********************************************************************
 * edit class descriptor
 */
const struct builtin_class_descr EDIT_builtin_class =
{
    "Edit",               /* name */
    CS_GLOBALCLASS | CS_DBLCLKS /*| CS_PARENTDC*/,   /* style */
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
static inline void EDIT_WM_Clear(WND *wnd, EDITSTATE *es)
{
	static const WCHAR empty_stringW[] = {0};

	/* Protect read-only edit control from modification */
	if(es->style & ES_READONLY)
	    return;

	EDIT_EM_ReplaceSel(wnd, es, TRUE, empty_stringW, TRUE);

	if (es->flags & EF_UPDATE) {
		es->flags &= ~EF_UPDATE;
		EDIT_NOTIFY_PARENT(wnd, EN_CHANGE, "EN_CHANGE");
	}
}


/*********************************************************************
 *
 *	WM_CUT
 *
 */
static inline void EDIT_WM_Cut(WND *wnd, EDITSTATE *es)
{
	EDIT_WM_Copy(wnd, es);
	EDIT_WM_Clear(wnd, es);
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

        GetVersionExW( &info );
        dwEmulatedVersion = MAKELONG( info.dwMinorVersion, info.dwMajorVersion );
        /* fixme: this may not be 100% correct; see discussion on the
         * wine developer list in Nov 1999 */
        version = dwProcVersion < dwEmulatedVersion ? dwProcVersion : dwEmulatedVersion; 
    }
    return version;
}


/*********************************************************************
 *
 *	EditWndProc_locked
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
static LRESULT WINAPI EditWndProc_locked( WND *wnd, UINT msg,
                                          WPARAM wParam, LPARAM lParam, BOOL unicode )
{
	EDITSTATE *es = *(EDITSTATE **)((wnd)->wExtra);
	LRESULT result = 0;

	switch (msg) {
	case WM_DESTROY:
		DPRINTF_EDIT_MSG32("WM_DESTROY");
		EDIT_WM_Destroy(wnd, es);
                result = 0;
                goto END;

	case WM_NCCREATE:
		DPRINTF_EDIT_MSG32("WM_NCCREATE");
		if(unicode)
		{
		    LPCREATESTRUCTW cs = (LPCREATESTRUCTW)lParam;
		    result = EDIT_WM_NCCreate(wnd, cs->style, cs->hwndParent, TRUE);
		}
		else
		{
		    LPCREATESTRUCTA cs = (LPCREATESTRUCTA)lParam;
		    result = EDIT_WM_NCCreate(wnd, cs->style, cs->hwndParent, FALSE);
		}
                goto END;
	}

	if (!es)
        {
	    if(unicode)
		result = DefWindowProcW(wnd->hwndSelf, msg, wParam, lParam);
	    else
		result = DefWindowProcA(wnd->hwndSelf, msg, wParam, lParam);
            goto END;
        }


	EDIT_LockBuffer(wnd, es);
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
			EDIT_EM_SetSel(wnd, es, (UINT)-1, 0, FALSE);
		else
			EDIT_EM_SetSel(wnd, es, LOWORD(lParam), HIWORD(lParam), FALSE);
		if (!wParam)
			EDIT_EM_ScrollCaret(wnd, es);
		result = 1;
		break;
	case EM_SETSEL:
		DPRINTF_EDIT_MSG32("EM_SETSEL");
		EDIT_EM_SetSel(wnd, es, wParam, lParam, FALSE);
		EDIT_EM_ScrollCaret(wnd, es);
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
			EDIT_SetRectNP(wnd, es, &rc);
			EDIT_UpdateText(wnd, NULL, TRUE);
		}
		break;
	case EM_SETRECT:
		DPRINTF_EDIT_MSG32("EM_SETRECT");
		if ((es->style & ES_MULTILINE) && lParam) {
			EDIT_SetRectNP(wnd, es, (LPRECT)lParam);
			EDIT_UpdateText(wnd, NULL, TRUE);
		}
		break;

	case EM_SETRECTNP16:
		DPRINTF_EDIT_MSG16("EM_SETRECTNP");
		if ((es->style & ES_MULTILINE) && lParam) {
			RECT rc;
			CONV_RECT16TO32(MapSL(lParam), &rc);
			EDIT_SetRectNP(wnd, es, &rc);
		}
		break;
	case EM_SETRECTNP:
		DPRINTF_EDIT_MSG32("EM_SETRECTNP");
		if ((es->style & ES_MULTILINE) && lParam)
			EDIT_SetRectNP(wnd, es, (LPRECT)lParam);
		break;

	case EM_SCROLL16:
		DPRINTF_EDIT_MSG16("EM_SCROLL");
		/* fall through */
	case EM_SCROLL:
		DPRINTF_EDIT_MSG32("EM_SCROLL");
		result = EDIT_EM_Scroll(wnd, es, (INT)wParam);
 		break;

	case EM_LINESCROLL16:
		DPRINTF_EDIT_MSG16("EM_LINESCROLL");
		wParam = (WPARAM)(INT)SHIWORD(lParam);
		lParam = (LPARAM)(INT)SLOWORD(lParam);
		/* fall through */
	case EM_LINESCROLL:
		DPRINTF_EDIT_MSG32("EM_LINESCROLL");
		result = (LRESULT)EDIT_EM_LineScroll(wnd, es, (INT)wParam, (INT)lParam);
		break;

	case EM_SCROLLCARET16:
		DPRINTF_EDIT_MSG16("EM_SCROLLCARET");
		/* fall through */
	case EM_SCROLLCARET:
		DPRINTF_EDIT_MSG32("EM_SCROLLCARET");
		EDIT_EM_ScrollCaret(wnd, es);
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
		EDIT_EM_SetHandle16(wnd, es, (HLOCAL16)wParam);
		break;
	case EM_SETHANDLE:
		DPRINTF_EDIT_MSG32("EM_SETHANDLE");
		EDIT_EM_SetHandle(wnd, es, (HLOCAL)wParam);
		break;

	case EM_GETHANDLE16:
		DPRINTF_EDIT_MSG16("EM_GETHANDLE");
		result = (LRESULT)EDIT_EM_GetHandle16(wnd, es);
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
		result = EDIT_EM_GetThumb(wnd, es);
		break;

	/* messages 0x00bf and 0x00c0 missing from specs */

	case WM_USER+15:
		DPRINTF_EDIT_MSG16("undocumented WM_USER+15, please report");
		/* fall through */
	case 0x00bf:
		DPRINTF_EDIT_MSG32("undocumented 0x00bf, please report");
		if(unicode)
		    result = DefWindowProcW(wnd->hwndSelf, msg, wParam, lParam);
		else
		    result = DefWindowProcA(wnd->hwndSelf, msg, wParam, lParam);
		break;

	case WM_USER+16:
		DPRINTF_EDIT_MSG16("undocumented WM_USER+16, please report");
		/* fall through */
	case 0x00c0:
		DPRINTF_EDIT_MSG32("undocumented 0x00c0, please report");
		if(unicode)
		    result = DefWindowProcW(wnd->hwndSelf, msg, wParam, lParam);
		else
		    result = DefWindowProcA(wnd->hwndSelf, msg, wParam, lParam);
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

		EDIT_EM_ReplaceSel(wnd, es, (BOOL)wParam, textW, TRUE);
		if (es->flags & EF_UPDATE) {
			es->flags &= ~EF_UPDATE;
			EDIT_NOTIFY_PARENT(wnd, EN_CHANGE, "EN_CHANGE");
		}
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
		if(unicode)
		    result = DefWindowProcW(wnd->hwndSelf, msg, wParam, lParam);
		else
		    result = DefWindowProcA(wnd->hwndSelf, msg, wParam, lParam);
		break;

	case EM_GETLINE16:
		DPRINTF_EDIT_MSG16("EM_GETLINE");
		lParam = (LPARAM)MapSL(lParam);
		/* fall through */
	case EM_GETLINE:
		DPRINTF_EDIT_MSG32("EM_GETLINE");
		result = (LRESULT)EDIT_EM_GetLine(es, (INT)wParam, (LPWSTR)lParam);
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
		result = (LRESULT)EDIT_EM_Undo(wnd, es);
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
		if(unicode)
		    result = DefWindowProcW(wnd->hwndSelf, msg, wParam, lParam);
		else
		    result = DefWindowProcA(wnd->hwndSelf, msg, wParam, lParam);
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

		EDIT_EM_SetPasswordChar(wnd, es, charW);
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
			wnd->dwStyle |= ES_READONLY;
			es->style |= ES_READONLY;
		} else {
			wnd->dwStyle &= ~ES_READONLY;
			es->style &= ~ES_READONLY;
		}
                result = 1;
 		break;

	case EM_SETWORDBREAKPROC16:
		DPRINTF_EDIT_MSG16("EM_SETWORDBREAKPROC");
		EDIT_EM_SetWordBreakProc16(wnd, es, (EDITWORDBREAKPROC16)lParam);
		break;
	case EM_SETWORDBREAKPROC:
		DPRINTF_EDIT_MSG32("EM_SETWORDBREAKPROC");
		EDIT_EM_SetWordBreakProc(wnd, es, lParam);
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
		result = EDIT_EM_PosFromChar(wnd, es, (INT)wParam, FALSE);
		break;

	case EM_CHARFROMPOS:
		DPRINTF_EDIT_MSG32("EM_CHARFROMPOS");
		result = EDIT_EM_CharFromPos(wnd, es, SLOWORD(lParam), SHIWORD(lParam));
		break;

	case WM_GETDLGCODE:
		DPRINTF_EDIT_MSG32("WM_GETDLGCODE");
		result = DLGC_HASSETSEL | DLGC_WANTCHARS | DLGC_WANTARROWS;

		if (lParam && (((LPMSG)lParam)->message == WM_KEYDOWN))
		{
		   int vk = (int)((LPMSG)lParam)->wParam;

		   if ((wnd->dwStyle & ES_WANTRETURN) && vk == VK_RETURN)
		   {
		      result |= DLGC_WANTMESSAGE;
		   }
		   else if (es->hwndListBox && (vk == VK_RETURN || vk == VK_ESCAPE))
		   {
		      if (SendMessageW(wnd->parent->hwndSelf, CB_GETDROPPEDSTATE, 0, 0))
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
		   if (SendMessageW(wnd->parent->hwndSelf, CB_GETDROPPEDSTATE, 0, 0))
		      SendMessageW(wnd->parent->hwndSelf, WM_KEYDOWN, charW, 0);
		   break;
		}
		EDIT_WM_Char(wnd, es, charW);
		break;
	}

	case WM_CLEAR:
		DPRINTF_EDIT_MSG32("WM_CLEAR");
		EDIT_WM_Clear(wnd, es);
		break;

	case WM_COMMAND:
		DPRINTF_EDIT_MSG32("WM_COMMAND");
		EDIT_WM_Command(wnd, es, HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
		break;

 	case WM_CONTEXTMENU:
		DPRINTF_EDIT_MSG32("WM_CONTEXTMENU");
		EDIT_WM_ContextMenu(wnd, es, SLOWORD(lParam), SHIWORD(lParam));
		break;

	case WM_COPY:
		DPRINTF_EDIT_MSG32("WM_COPY");
		EDIT_WM_Copy(wnd, es);
		break;

	case WM_CREATE:
		DPRINTF_EDIT_MSG32("WM_CREATE");
		if(unicode)
		    result = EDIT_WM_Create(wnd, es, ((LPCREATESTRUCTW)lParam)->lpszName);
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
		    result = EDIT_WM_Create(wnd, es, nameW);
		    if(nameW)
			HeapFree(GetProcessHeap(), 0, nameW);
		}
		break;

	case WM_CUT:
		DPRINTF_EDIT_MSG32("WM_CUT");
		EDIT_WM_Cut(wnd, es);
		break;

	case WM_ENABLE:
		DPRINTF_EDIT_MSG32("WM_ENABLE");
                es->bEnableState = (BOOL) wParam;
		EDIT_UpdateText(wnd, NULL, TRUE);
		break;

	case WM_ERASEBKGND:
		DPRINTF_EDIT_MSG32("WM_ERASEBKGND");
		result = EDIT_WM_EraseBkGnd(wnd, es, (HDC)wParam);
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
		result = EDIT_WM_HScroll(wnd, es, LOWORD(wParam), SHIWORD(wParam));
		break;

	case WM_KEYDOWN:
		DPRINTF_EDIT_MSG32("WM_KEYDOWN");
		result = EDIT_WM_KeyDown(wnd, es, (INT)wParam);
		break;

	case WM_KILLFOCUS:
		DPRINTF_EDIT_MSG32("WM_KILLFOCUS");
		result = EDIT_WM_KillFocus(wnd, es);
		break;

	case WM_LBUTTONDBLCLK:
		DPRINTF_EDIT_MSG32("WM_LBUTTONDBLCLK");
		result = EDIT_WM_LButtonDblClk(wnd, es);
		break;

	case WM_LBUTTONDOWN:
		DPRINTF_EDIT_MSG32("WM_LBUTTONDOWN");
		result = EDIT_WM_LButtonDown(wnd, es, (DWORD)wParam, SLOWORD(lParam), SHIWORD(lParam));
		break;

	case WM_LBUTTONUP:
		DPRINTF_EDIT_MSG32("WM_LBUTTONUP");
		result = EDIT_WM_LButtonUp(wnd->hwndSelf, es);
		break;

	case WM_MBUTTONDOWN:                        
  		DPRINTF_EDIT_MSG32("WM_MBUTTONDOWN");    
  		result = EDIT_WM_MButtonDown(wnd);
		break;

	case WM_MOUSEACTIVATE:
		/*
		 *	FIXME: maybe DefWindowProc() screws up, but it seems that
		 *		modeless dialog boxes need this.  If we don't do this, the focus
		 *		will _not_ be set by DefWindowProc() for edit controls in a
		 *		modeless dialog box ???
		 */
		DPRINTF_EDIT_MSG32("WM_MOUSEACTIVATE");
		SetFocus(wnd->hwndSelf);
		result = MA_ACTIVATE;
		break;

	case WM_MOUSEMOVE:
		/*
		 *	DPRINTF_EDIT_MSG32("WM_MOUSEMOVE");
		 */
		result = EDIT_WM_MouseMove(wnd, es, SLOWORD(lParam), SHIWORD(lParam));
		break;

	case WM_PAINT:
		DPRINTF_EDIT_MSG32("WM_PAINT");
	        EDIT_WM_Paint(wnd, es, wParam);
		break;

	case WM_PASTE:
		DPRINTF_EDIT_MSG32("WM_PASTE");
		EDIT_WM_Paste(wnd, es);
		break;

	case WM_SETFOCUS:
		DPRINTF_EDIT_MSG32("WM_SETFOCUS");
		EDIT_WM_SetFocus(wnd, es);
		break;

	case WM_SETFONT:
		DPRINTF_EDIT_MSG32("WM_SETFONT");
		EDIT_WM_SetFont(wnd, es, (HFONT)wParam, LOWORD(lParam) != 0);
		break;

	case WM_SETREDRAW:
		/* FIXME: actually set an internal flag and behave accordingly */
		break;

	case WM_SETTEXT:
		DPRINTF_EDIT_MSG32("WM_SETTEXT");
		EDIT_WM_SetText(wnd, es, lParam, unicode);
		result = TRUE;
		break;

	case WM_SIZE:
		DPRINTF_EDIT_MSG32("WM_SIZE");
		EDIT_WM_Size(wnd, es, (UINT)wParam, LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_SYSKEYDOWN:
		DPRINTF_EDIT_MSG32("WM_SYSKEYDOWN");
		result = EDIT_WM_SysKeyDown(wnd, es, (INT)wParam, (DWORD)lParam);
		break;

	case WM_TIMER:
		DPRINTF_EDIT_MSG32("WM_TIMER");
		EDIT_WM_Timer(wnd, es);
		break;

	case WM_VSCROLL:
		DPRINTF_EDIT_MSG32("WM_VSCROLL");
		result = EDIT_WM_VScroll(wnd, es, LOWORD(wParam), SHIWORD(wParam));
		break;

        case WM_MOUSEWHEEL:
                {
                    int gcWheelDelta = 0;
                    UINT pulScrollLines = 3;
                    SystemParametersInfoW(SPI_GETWHEELSCROLLLINES,0, &pulScrollLines, 0);

                    if (wParam & (MK_SHIFT | MK_CONTROL)) {
			if(unicode)
			    result = DefWindowProcW(wnd->hwndSelf, msg, wParam, lParam);
			else
			    result = DefWindowProcA(wnd->hwndSelf, msg, wParam, lParam);
                        break;
                    }
                    gcWheelDelta -= SHIWORD(wParam);
                    if (abs(gcWheelDelta) >= WHEEL_DELTA && pulScrollLines)
                    {
                        int cLineScroll= (int) min((UINT) es->line_count, pulScrollLines);
                        cLineScroll *= (gcWheelDelta / WHEEL_DELTA);
			result = EDIT_EM_LineScroll(wnd, es, 0, cLineScroll);
                    }
                }
                break;
	default:
		if(unicode)
		    result = DefWindowProcW(wnd->hwndSelf, msg, wParam, lParam);
		else
		    result = DefWindowProcA(wnd->hwndSelf, msg, wParam, lParam);
		break;
	}
	EDIT_UnlockBuffer(wnd, es, FALSE);
    END:
	return result;
}

/*********************************************************************
 *
 *	EditWndProcW   (USER32.@)
 */
LRESULT WINAPI EditWndProcW(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT res;
    WND *wndPtr = WIN_FindWndPtr(hWnd);

    res = EditWndProc_locked(wndPtr, uMsg, wParam, lParam, TRUE);

    WIN_ReleaseWndPtr(wndPtr);
    return res;
}

/*********************************************************************
 *
 *	EditWndProcA   (USER32.@)
 */
LRESULT WINAPI EditWndProcA(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT res;
    WND *wndPtr = WIN_FindWndPtr(hWnd);

    res = EditWndProc_locked(wndPtr, uMsg, wParam, lParam, FALSE);

    WIN_ReleaseWndPtr(wndPtr);
    return res;
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
static void EDIT_BuildLineDefs_ML(WND *wnd, EDITSTATE *es)
{
	HDC dc;
	HFONT old_font = 0;
	LPWSTR start, cp;
	INT fw;
	LINEDEF *current_def;
	LINEDEF **previous_next;

	current_def = es->first_line_def;
	do {
		LINEDEF *next_def = current_def->next;
		HeapFree(GetProcessHeap(), 0, current_def);
		current_def = next_def;
	} while (current_def);
	es->line_count = 0;
	es->text_width = 0;

	dc = GetDC(wnd->hwndSelf);
	if (es->font)
		old_font = SelectObject(dc, es->font);

	fw = es->format_rect.right - es->format_rect.left;
	start = es->text;
	previous_next = &es->first_line_def;
	do {
		current_def = HeapAlloc(GetProcessHeap(), 0, sizeof(LINEDEF));
		current_def->next = NULL;
		cp = start;
		while (*cp) {
			if ((*cp == '\r') && (*(cp + 1) == '\n'))
				break;
			cp++;
		}
		if (!(*cp)) {
			current_def->ending = END_0;
			current_def->net_length = strlenW(start);
		} else if ((cp > start) && (*(cp - 1) == '\r')) {
			current_def->ending = END_SOFT;
			current_def->net_length = cp - start - 1;
		} else {
			current_def->ending = END_HARD;
			current_def->net_length = cp - start;
		}
		current_def->width = (INT)LOWORD(GetTabbedTextExtentW(dc,
					start, current_def->net_length,
					es->tabs_count, es->tabs));
		/* FIXME: check here for lines that are too wide even in AUTOHSCROLL (> 32767 ???) */
		if ((!(es->style & ES_AUTOHSCROLL)) && (current_def->width > fw)) {
			INT next = 0;
			INT prev;
			do {
				prev = next;
				next = EDIT_CallWordBreakProc(es, start - es->text,
						prev + 1, current_def->net_length, WB_RIGHT);
				current_def->width = (INT)LOWORD(GetTabbedTextExtentW(dc,
							start, next, es->tabs_count, es->tabs));
			} while (current_def->width <= fw);
			if (!prev) {
				next = 0;
				do {
					prev = next;
					next++;
					current_def->width = (INT)LOWORD(GetTabbedTextExtentW(dc,
								start, next, es->tabs_count, es->tabs));
				} while (current_def->width <= fw);
				if (!prev)
					prev = 1;
			}
			current_def->net_length = prev;
			current_def->ending = END_WRAP;
			current_def->width = (INT)LOWORD(GetTabbedTextExtentW(dc, start,
						current_def->net_length, es->tabs_count, es->tabs));
		}
		switch (current_def->ending) {
		case END_SOFT:
			current_def->length = current_def->net_length + 3;
			break;
		case END_HARD:
			current_def->length = current_def->net_length + 2;
			break;
		case END_WRAP:
		case END_0:
			current_def->length = current_def->net_length;
			break;
		}
		es->text_width = max(es->text_width, current_def->width);
		start += current_def->length;
		*previous_next = current_def;
		previous_next = &current_def->next;
		es->line_count++;
	} while (current_def->ending != END_0);
	if (es->font)
		SelectObject(dc, old_font);
	ReleaseDC(wnd->hwndSelf, dc);
}

/*********************************************************************
 *
 *	EDIT_CalcLineWidth_SL
 *
 */
static void EDIT_CalcLineWidth_SL(WND *wnd, EDITSTATE *es)
{
    es->text_width = SLOWORD(EDIT_EM_PosFromChar(wnd, es, strlenW(es->text), FALSE));
}

/*********************************************************************
 *
 *	EDIT_CallWordBreakProc
 *
 *	Call appropriate WordBreakProc (internal or external).
 *
 *	Note: The "start" argument should always be an index refering
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

    /* To avoid any deadlocks, all the locks on the windows structures
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
static INT EDIT_CharFromPos(WND *wnd, EDITSTATE *es, INT x, INT y, LPBOOL after_wrap)
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
		dc = GetDC(wnd->hwndSelf);
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
		dc = GetDC(wnd->hwndSelf);
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
	ReleaseDC(wnd->hwndSelf, dc);
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
static void EDIT_GetLineRect(WND *wnd, EDITSTATE *es, INT line, INT scol, INT ecol, LPRECT rc)
{
	INT line_index =  EDIT_EM_LineIndex(es, line);

	if (es->style & ES_MULTILINE)
		rc->top = es->format_rect.top + (line - es->y_offset) * es->line_height;
	else
		rc->top = es->format_rect.top;
	rc->bottom = rc->top + es->line_height;
	rc->left = (scol == 0) ? es->format_rect.left : SLOWORD(EDIT_EM_PosFromChar(wnd, es, line_index + scol, TRUE));
	rc->right = (ecol == -1) ? es->format_rect.right : SLOWORD(EDIT_EM_PosFromChar(wnd, es, line_index + ecol, TRUE));
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
static void EDIT_LockBuffer(WND *wnd, EDITSTATE *es)
{
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
		    textA = LOCAL_Lock(wnd->hInstance, es->hloc16);
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
		    LOCAL_Unlock(wnd->hInstance, es->hloc16);
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
static void EDIT_SL_InvalidateText(WND *wnd, EDITSTATE *es, INT start, INT end)
{
	RECT line_rect;
	RECT rc;

	EDIT_GetLineRect(wnd, es, 0, start, end, &line_rect);
	if (IntersectRect(&rc, &line_rect, &es->format_rect))
		EDIT_UpdateText(wnd, &rc, FALSE);
}


/*********************************************************************
 *
 *	EDIT_ML_InvalidateText
 *
 *	Called from EDIT_InvalidateText().
 *	Does the job for multi-line controls only.
 *
 */
static void EDIT_ML_InvalidateText(WND *wnd, EDITSTATE *es, INT start, INT end)
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
	GetClientRect(wnd->hwndSelf, &rc1);
	IntersectRect(&rcWnd, &rc1, &es->format_rect);
	if (sl == el) {
		EDIT_GetLineRect(wnd, es, sl, sc, ec, &rcLine);
		if (IntersectRect(&rcUpdate, &rcWnd, &rcLine))
			EDIT_UpdateText(wnd, &rcUpdate, FALSE);
	} else {
		EDIT_GetLineRect(wnd, es, sl, sc,
				EDIT_EM_LineLength(es,
					EDIT_EM_LineIndex(es, sl)),
				&rcLine);
		if (IntersectRect(&rcUpdate, &rcWnd, &rcLine))
			EDIT_UpdateText(wnd, &rcUpdate, FALSE);
		for (l = sl + 1 ; l < el ; l++) {
			EDIT_GetLineRect(wnd, es, l, 0,
				EDIT_EM_LineLength(es,
					EDIT_EM_LineIndex(es, l)),
				&rcLine);
			if (IntersectRect(&rcUpdate, &rcWnd, &rcLine))
				EDIT_UpdateText(wnd, &rcUpdate, FALSE);
		}
		EDIT_GetLineRect(wnd, es, el, 0, ec, &rcLine);
		if (IntersectRect(&rcUpdate, &rcWnd, &rcLine))
			EDIT_UpdateText(wnd, &rcUpdate, FALSE);
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
static void EDIT_InvalidateText(WND *wnd, EDITSTATE *es, INT start, INT end)
{
	if (end == start)
		return;

	if (end == -1)
		end = strlenW(es->text);

	ORDER_INT(start, end);

	if (es->style & ES_MULTILINE)
		EDIT_ML_InvalidateText(wnd, es, start, end);
	else
		EDIT_SL_InvalidateText(wnd, es, start, end);
}


/*********************************************************************
 *
 *	EDIT_MakeFit
 *
 *	Try to fit size + 1 characters in the buffer. Constrain to limits.
 *
 */
static BOOL EDIT_MakeFit(WND *wnd, EDITSTATE *es, UINT size)
{
	HLOCAL hNew32W;

	if (size <= es->buffer_size)
		return TRUE;
	if (size > es->buffer_limit) {
		EDIT_NOTIFY_PARENT(wnd, EN_MAXTEXT, "EN_MAXTEXT");
		return FALSE;
	}
	if (size > es->buffer_limit)
		size = es->buffer_limit;

	TRACE("trying to ReAlloc to %d+1 characters\n", size);

	/* Force edit to unlock it's buffer. es->text now NULL */
	EDIT_UnlockBuffer(wnd, es, TRUE);

	if (es->hloc32W) {
	    UINT alloc_size = ROUND_TO_GROW((size + 1) * sizeof(WCHAR));
	    if ((hNew32W = LocalReAlloc(es->hloc32W, alloc_size, LMEM_MOVEABLE | LMEM_ZEROINIT))) {
		TRACE("Old 32 bit handle %08x, new handle %08x\n", es->hloc32W, hNew32W);
		es->hloc32W = hNew32W;
		es->buffer_size = LocalSize(hNew32W)/sizeof(WCHAR) - 1;
	    }
	}

	EDIT_LockBuffer(wnd, es);

	if (es->buffer_size < size) {
		WARN("FAILED !  We now have %d+1\n", es->buffer_size);
		EDIT_NOTIFY_PARENT(wnd, EN_ERRSPACE, "EN_ERRSPACE");
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
static void EDIT_MoveBackward(WND *wnd, EDITSTATE *es, BOOL extend)
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
	EDIT_EM_SetSel(wnd, es, extend ? es->selection_start : e, e, FALSE);
	EDIT_EM_ScrollCaret(wnd, es);
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
static void EDIT_MoveDown_ML(WND *wnd, EDITSTATE *es, BOOL extend)
{
	INT s = es->selection_start;
	INT e = es->selection_end;
	BOOL after_wrap = (es->flags & EF_AFTER_WRAP);
	LRESULT pos = EDIT_EM_PosFromChar(wnd, es, e, after_wrap);
	INT x = SLOWORD(pos);
	INT y = SHIWORD(pos);

	e = EDIT_CharFromPos(wnd, es, x, y + es->line_height, &after_wrap);
	if (!extend)
		s = e;
	EDIT_EM_SetSel(wnd, es, s, e, after_wrap);
	EDIT_EM_ScrollCaret(wnd, es);
}


/*********************************************************************
 *
 *	EDIT_MoveEnd
 *
 */
static void EDIT_MoveEnd(WND *wnd, EDITSTATE *es, BOOL extend)
{
	BOOL after_wrap = FALSE;
	INT e;

	/* Pass a high value in x to make sure of receiving the end of the line */
	if (es->style & ES_MULTILINE)
		e = EDIT_CharFromPos(wnd, es, 0x3fffffff,
			HIWORD(EDIT_EM_PosFromChar(wnd, es, es->selection_end, es->flags & EF_AFTER_WRAP)), &after_wrap);
	else
		e = strlenW(es->text);
	EDIT_EM_SetSel(wnd, es, extend ? es->selection_start : e, e, after_wrap);
	EDIT_EM_ScrollCaret(wnd, es);
}


/*********************************************************************
 *
 *	EDIT_MoveForward
 *
 */
static void EDIT_MoveForward(WND *wnd, EDITSTATE *es, BOOL extend)
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
	EDIT_EM_SetSel(wnd, es, extend ? es->selection_start : e, e, FALSE);
	EDIT_EM_ScrollCaret(wnd, es);
}


/*********************************************************************
 *
 *	EDIT_MoveHome
 *
 *	Home key: move to beginning of line.
 *
 */
static void EDIT_MoveHome(WND *wnd, EDITSTATE *es, BOOL extend)
{
	INT e;

	/* Pass the x_offset in x to make sure of receiving the first position of the line */
	if (es->style & ES_MULTILINE)
		e = EDIT_CharFromPos(wnd, es, -es->x_offset,
			HIWORD(EDIT_EM_PosFromChar(wnd, es, es->selection_end, es->flags & EF_AFTER_WRAP)), NULL);
	else
		e = 0;
	EDIT_EM_SetSel(wnd, es, extend ? es->selection_start : e, e, FALSE);
	EDIT_EM_ScrollCaret(wnd, es);
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
static void EDIT_MovePageDown_ML(WND *wnd, EDITSTATE *es, BOOL extend)
{
	INT s = es->selection_start;
	INT e = es->selection_end;
	BOOL after_wrap = (es->flags & EF_AFTER_WRAP);
	LRESULT pos = EDIT_EM_PosFromChar(wnd, es, e, after_wrap);
	INT x = SLOWORD(pos);
	INT y = SHIWORD(pos);

	e = EDIT_CharFromPos(wnd, es, x,
		y + (es->format_rect.bottom - es->format_rect.top),
		&after_wrap);
	if (!extend)
		s = e;
	EDIT_EM_SetSel(wnd, es, s, e, after_wrap);
	EDIT_EM_ScrollCaret(wnd, es);
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
static void EDIT_MovePageUp_ML(WND *wnd, EDITSTATE *es, BOOL extend)
{
	INT s = es->selection_start;
	INT e = es->selection_end;
	BOOL after_wrap = (es->flags & EF_AFTER_WRAP);
	LRESULT pos = EDIT_EM_PosFromChar(wnd, es, e, after_wrap);
	INT x = SLOWORD(pos);
	INT y = SHIWORD(pos);

	e = EDIT_CharFromPos(wnd, es, x,
		y - (es->format_rect.bottom - es->format_rect.top),
		&after_wrap);
	if (!extend)
		s = e;
	EDIT_EM_SetSel(wnd, es, s, e, after_wrap);
	EDIT_EM_ScrollCaret(wnd, es);
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
static void EDIT_MoveUp_ML(WND *wnd, EDITSTATE *es, BOOL extend)
{
	INT s = es->selection_start;
	INT e = es->selection_end;
	BOOL after_wrap = (es->flags & EF_AFTER_WRAP);
	LRESULT pos = EDIT_EM_PosFromChar(wnd, es, e, after_wrap);
	INT x = SLOWORD(pos);
	INT y = SHIWORD(pos);

	e = EDIT_CharFromPos(wnd, es, x, y - es->line_height, &after_wrap);
	if (!extend)
		s = e;
	EDIT_EM_SetSel(wnd, es, s, e, after_wrap);
	EDIT_EM_ScrollCaret(wnd, es);
}


/*********************************************************************
 *
 *	EDIT_MoveWordBackward
 *
 */
static void EDIT_MoveWordBackward(WND *wnd, EDITSTATE *es, BOOL extend)
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
	EDIT_EM_SetSel(wnd, es, s, e, FALSE);
	EDIT_EM_ScrollCaret(wnd, es);
}


/*********************************************************************
 *
 *	EDIT_MoveWordForward
 *
 */
static void EDIT_MoveWordForward(WND *wnd, EDITSTATE *es, BOOL extend)
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
	EDIT_EM_SetSel(wnd, es, s, e, FALSE);
	EDIT_EM_ScrollCaret(wnd, es);
}


/*********************************************************************
 *
 *	EDIT_PaintLine
 *
 */
static void EDIT_PaintLine(WND *wnd, EDITSTATE *es, HDC dc, INT line, BOOL rev)
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

	pos = EDIT_EM_PosFromChar(wnd, es, EDIT_EM_LineIndex(es, line), FALSE);
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
	SIZE size;

	if (!count)
		return 0;
	BkColor = GetBkColor(dc);
	TextColor = GetTextColor(dc);
	if (rev) {
		SetBkColor(dc, GetSysColor(COLOR_HIGHLIGHT));
		SetTextColor(dc, GetSysColor(COLOR_HIGHLIGHTTEXT));
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
	}
	return ret;
}


/*********************************************************************
 *
 *	EDIT_SetCaretPos
 *
 */
static void EDIT_SetCaretPos(WND *wnd, EDITSTATE *es, INT pos,
			     BOOL after_wrap)
{
	LRESULT res = EDIT_EM_PosFromChar(wnd, es, pos, after_wrap);
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
static void EDIT_SetRectNP(WND *wnd, EDITSTATE *es, LPRECT rc)
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
	    EDIT_UpdateScrollInfo(wnd, es);
	}
	else
	/* Windows doesn't care to fix text placement for SL controls */
		es->format_rect.bottom = es->format_rect.top + es->line_height;

	if ((es->style & ES_MULTILINE) && !(es->style & ES_AUTOHSCROLL))
		EDIT_BuildLineDefs_ML(wnd, es);
}


/*********************************************************************
 *
 *	EDIT_UnlockBuffer
 *
 */
static void EDIT_UnlockBuffer(WND *wnd, EDITSTATE *es, BOOL force)
{
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
		    countA = LOCAL_Size(wnd->hInstance, es->hloc16);
		    if(countA_new > countA)
		    {
			HLOCAL16 hloc16_new;
			UINT alloc_size = ROUND_TO_GROW(countA_new);
			TRACE("Resizing 16-bit ANSI buffer from %d to %d bytes\n", countA, alloc_size);
			hloc16_new = LOCAL_ReAlloc(wnd->hInstance, es->hloc16, alloc_size, LMEM_MOVEABLE | LMEM_ZEROINIT);
			if(hloc16_new)
			{
			    es->hloc16 = hloc16_new;
			    countA = LOCAL_Size(wnd->hInstance, hloc16_new);
			    TRACE("Real new size %d bytes\n", countA);
			}
			else
			    WARN("FAILED! Will synchronize partially\n");
		    }
		    textA = LOCAL_Lock(wnd->hInstance, es->hloc16);
		    _16bit = TRUE;
		}

		if(textA)
		{
		    WideCharToMultiByte(CP_ACP, 0, es->text, countW, textA, countA, NULL, NULL);
		    if(_16bit)
			LOCAL_Unlock(wnd->hInstance, es->hloc16);
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
static void EDIT_UpdateScrollInfo(WND *wnd, EDITSTATE *es)
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
	SetScrollInfo(wnd->hwndSelf, SB_VERT, &si, TRUE);
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
	SetScrollInfo(wnd->hwndSelf, SB_HORZ, &si, TRUE);
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
static LRESULT EDIT_EM_CharFromPos(WND *wnd, EDITSTATE *es, INT x, INT y)
{
	POINT pt;
	RECT rc;
	INT index;

	pt.x = x;
	pt.y = y;
	GetClientRect(wnd->hwndSelf, &rc);
	if (!PtInRect(&rc, pt))
		return -1;

	index = EDIT_CharFromPos(wnd, es, x, y, NULL);
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
static HLOCAL16 EDIT_EM_GetHandle16(WND *wnd, EDITSTATE *es)
{
	CHAR *textA;
	UINT countA, alloc_size;

	if (!(es->style & ES_MULTILINE))
		return 0;

	if (es->hloc16)
		return es->hloc16;

	if (!LOCAL_HeapSize(wnd->hInstance)) {
		if (!LocalInit16(wnd->hInstance, 0,
				GlobalSize16(wnd->hInstance))) {
			ERR("could not initialize local heap\n");
			return 0;
		}
		TRACE("local heap initialized\n");
	}

	countA = WideCharToMultiByte(CP_ACP, 0, es->text, -1, NULL, 0, NULL, NULL);
	alloc_size = ROUND_TO_GROW(countA);

	TRACE("Allocating 16-bit ANSI alias buffer\n");
	if (!(es->hloc16 = LOCAL_Alloc(wnd->hInstance, LMEM_MOVEABLE | LMEM_ZEROINIT, alloc_size))) {
		ERR("could not allocate new 16 bit buffer\n");
		return 0;
	}

	if (!(textA = (LPSTR)LOCAL_Lock(wnd->hInstance, es->hloc16))) {
		ERR("could not lock new 16 bit buffer\n");
		LOCAL_Free(wnd->hInstance, es->hloc16);
		es->hloc16 = 0;
		return 0;
	}

	WideCharToMultiByte(CP_ACP, 0, es->text, -1, textA, countA, NULL, NULL);
	LOCAL_Unlock(wnd->hInstance, es->hloc16);

	TRACE("Returning %04X, LocalSize() = %d\n", es->hloc16, LOCAL_Size(wnd->hInstance, es->hloc16));
	return es->hloc16;
}


/*********************************************************************
 *
 *	EM_GETLINE
 *
 */
static INT EDIT_EM_GetLine(EDITSTATE *es, INT line, LPWSTR lpch)
{
	LPWSTR src;
	INT len;
	INT i;

	if (es->style & ES_MULTILINE) {
		if (line >= es->line_count)
			return 0;
	} else
		line = 0;
	i = EDIT_EM_LineIndex(es, line);
	src = es->text + i;
	len = min(*(WORD *)lpch, EDIT_EM_LineLength(es, i));
	for (i = 0 ; i < len ; i++) {
		*lpch = *src;
		src++;
		lpch++;
	}
	return (LRESULT)len;
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
static LRESULT EDIT_EM_GetThumb(WND *wnd, EDITSTATE *es)
{
	return MAKELONG(EDIT_WM_VScroll(wnd, es, EM_GETTHUMB16, 0),
		EDIT_WM_HScroll(wnd, es, EM_GETTHUMB16, 0));
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
		INT32 li;
		INT32 count;
		li = EDIT_EM_LineFromChar(es, es->selection_start);
		/* # chars before start of selection area */
		count = es->selection_start - EDIT_EM_LineIndex(es, li);
		li = EDIT_EM_LineFromChar(es, es->selection_end);
		/* # chars after end of selection */
		count += EDIT_EM_LineIndex(es, li) +
			EDIT_EM_LineLength(es, li) - es->selection_end;
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
static BOOL EDIT_EM_LineScroll(WND *wnd, EDITSTATE *es, INT dx, INT dy)
{
	if (!(es->style & ES_MULTILINE))
		return FALSE;

	dx *= es->char_width;
	return EDIT_EM_LineScroll_internal(wnd, es, dx, dy);
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
static BOOL EDIT_EM_LineScroll_internal(WND *wnd, EDITSTATE *es, INT dx, INT dy)
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
	    x_offset_in_pixels = SLOWORD(EDIT_EM_PosFromChar(wnd, es, es->x_offset, FALSE));
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

		GetClientRect(wnd->hwndSelf, &rc1);
		IntersectRect(&rc, &rc1, &es->format_rect);
		ScrollWindowEx(wnd->hwndSelf, -dx, dy,
				NULL, &rc, (HRGN)NULL, NULL, SW_INVALIDATE);
		/* force scroll info update */
		EDIT_UpdateScrollInfo(wnd, es);
	}
	if (dx && !(es->flags & EF_HSCROLL_TRACK))
		EDIT_NOTIFY_PARENT(wnd, EN_HSCROLL, "EN_HSCROLL");
	if (dy && !(es->flags & EF_VSCROLL_TRACK))
		EDIT_NOTIFY_PARENT(wnd, EN_VSCROLL, "EN_VSCROLL");
	return TRUE;
}


/*********************************************************************
 *
 *	EM_POSFROMCHAR
 *
 */
static LRESULT EDIT_EM_PosFromChar(WND *wnd, EDITSTATE *es, INT index, BOOL after_wrap)
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
	dc = GetDC(wnd->hwndSelf);
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
	ReleaseDC(wnd->hwndSelf, dc);
	return MAKELONG((INT16)x, (INT16)y);
}


/*********************************************************************
 *
 *	EM_REPLACESEL
 *
 *	FIXME: handle ES_NUMBER and ES_OEMCONVERT here
 *
 */
static void EDIT_EM_ReplaceSel(WND *wnd, EDITSTATE *es, BOOL can_undo, LPCWSTR lpsz_replace, BOOL send_update)
{
	UINT strl = strlenW(lpsz_replace);
	UINT tl = strlenW(es->text);
	UINT utl;
	UINT s;
	UINT e;
	UINT i;
	LPWSTR p;

	TRACE("%s, can_undo %d, send_update %d\n",
	    debugstr_w(lpsz_replace), can_undo, send_update);

	s = es->selection_start;
	e = es->selection_end;

	if ((s == e) && !strl)
		return;

	ORDER_UINT(s, e);

	if (!EDIT_MakeFit(wnd, es, tl - (e - s) + strl))
		return;

	if (e != s) {
		/* there is something to be deleted */
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
	/* FIXME: really inefficient */
	if (es->style & ES_MULTILINE)
		EDIT_BuildLineDefs_ML(wnd, es);
	else
	    EDIT_CalcLineWidth_SL(wnd, es);

	EDIT_EM_SetSel(wnd, es, s, s, FALSE);
	es->flags |= EF_MODIFIED;
	if (send_update) es->flags |= EF_UPDATE;
	EDIT_EM_ScrollCaret(wnd, es);
	
	/* force scroll info update */
	EDIT_UpdateScrollInfo(wnd, es);

	/* FIXME: really inefficient */
	EDIT_UpdateText(wnd, NULL, TRUE);
}


/*********************************************************************
 *
 *	EM_SCROLL
 *
 */
static LRESULT EDIT_EM_Scroll(WND *wnd, EDITSTATE *es, INT action)
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
		EDIT_EM_LineScroll(wnd, es, 0, dy);
	}
	return MAKELONG((INT16)dy, (BOOL16)TRUE);
}


/*********************************************************************
 *
 *	EM_SCROLLCARET
 *
 */
static void EDIT_EM_ScrollCaret(WND *wnd, EDITSTATE *es)
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
		x = SLOWORD(EDIT_EM_PosFromChar(wnd, es, es->selection_end, es->flags & EF_AFTER_WRAP));
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
			EDIT_EM_LineScroll_internal(wnd, es, dx, dy);
		}
	} else {
		INT x;
		INT goal;
		INT format_width;

		if (!(es->style & ES_AUTOHSCROLL))
			return;

		x = SLOWORD(EDIT_EM_PosFromChar(wnd, es, es->selection_end, FALSE));
		format_width = es->format_rect.right - es->format_rect.left;
		if (x < es->format_rect.left) {
			goal = es->format_rect.left + format_width / HSCROLL_FRACTION;
			do {
				es->x_offset--;
				x = SLOWORD(EDIT_EM_PosFromChar(wnd, es, es->selection_end, FALSE));
			} while ((x < goal) && es->x_offset);
			/* FIXME: use ScrollWindow() somehow to improve performance */
			EDIT_UpdateText(wnd, NULL, TRUE);
		} else if (x > es->format_rect.right) {
			INT x_last;
			INT len = strlenW(es->text);
			goal = es->format_rect.right - format_width / HSCROLL_FRACTION;
			do {
				es->x_offset++;
				x = SLOWORD(EDIT_EM_PosFromChar(wnd, es, es->selection_end, FALSE));
				x_last = SLOWORD(EDIT_EM_PosFromChar(wnd, es, len, FALSE));
			} while ((x > goal) && (x_last > es->format_rect.right));
			/* FIXME: use ScrollWindow() somehow to improve performance */
			EDIT_UpdateText(wnd, NULL, TRUE);
		}
	}

    if(es->flags & EF_FOCUSED)
	EDIT_SetCaretPos(wnd, es, es->selection_end, es->flags & EF_AFTER_WRAP);
}


/*********************************************************************
 *
 *	EM_SETHANDLE
 *
 *	FIXME:	ES_LOWERCASE, ES_UPPERCASE, ES_OEMCONVERT, ES_NUMBER ???
 *
 */
static void EDIT_EM_SetHandle(WND *wnd, EDITSTATE *es, HLOCAL hloc)
{
	if (!(es->style & ES_MULTILINE))
		return;

	if (!hloc) {
		WARN("called with NULL handle\n");
		return;
	}

	EDIT_UnlockBuffer(wnd, es, TRUE);

	if(es->hloc16)
	{
	    LOCAL_Free(wnd->hInstance, es->hloc16);
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

	EDIT_LockBuffer(wnd, es);

	es->x_offset = es->y_offset = 0;
	es->selection_start = es->selection_end = 0;
	EDIT_EM_EmptyUndoBuffer(es);
	es->flags &= ~EF_MODIFIED;
	es->flags &= ~EF_UPDATE;
	EDIT_BuildLineDefs_ML(wnd, es);
	EDIT_UpdateText(wnd, NULL, TRUE);
	EDIT_EM_ScrollCaret(wnd, es);
	/* force scroll info update */
	EDIT_UpdateScrollInfo(wnd, es);
}


/*********************************************************************
 *
 *	EM_SETHANDLE16
 *
 *	FIXME:	ES_LOWERCASE, ES_UPPERCASE, ES_OEMCONVERT, ES_NUMBER ???
 *
 */
static void EDIT_EM_SetHandle16(WND *wnd, EDITSTATE *es, HLOCAL16 hloc)
{
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

	EDIT_UnlockBuffer(wnd, es, TRUE);

	if(es->hloc32A)
	{
	    LocalFree(es->hloc32A);
	    es->hloc32A = (HLOCAL)NULL;
	}

	countA = LOCAL_Size(wnd->hInstance, hloc);
	textA = LOCAL_Lock(wnd->hInstance, hloc);
	countW = MultiByteToWideChar(CP_ACP, 0, textA, countA, NULL, 0);
	if(!(hloc32W_new = LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT, countW * sizeof(WCHAR))))
	{
	    ERR("Could not allocate new unicode buffer\n");
	    return;
	}
	textW = LocalLock(hloc32W_new);
	MultiByteToWideChar(CP_ACP, 0, textA, countA, textW, countW);
	LocalUnlock(hloc32W_new);
	LOCAL_Unlock(wnd->hInstance, hloc);

	if(es->hloc32W)
	    LocalFree(es->hloc32W);

	es->hloc32W = hloc32W_new;
	es->hloc16 = hloc;

	es->buffer_size = LocalSize(es->hloc32W)/sizeof(WCHAR) - 1;

	EDIT_LockBuffer(wnd, es);

	es->x_offset = es->y_offset = 0;
	es->selection_start = es->selection_end = 0;
	EDIT_EM_EmptyUndoBuffer(es);
	es->flags &= ~EF_MODIFIED;
	es->flags &= ~EF_UPDATE;
	EDIT_BuildLineDefs_ML(wnd, es);
	EDIT_UpdateText(wnd, NULL, TRUE);
	EDIT_EM_ScrollCaret(wnd, es);
	/* force scroll info update */
	EDIT_UpdateScrollInfo(wnd, es);
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
static void EDIT_EM_SetPasswordChar(WND *wnd, EDITSTATE *es, WCHAR c)
{
	if (es->style & ES_MULTILINE)
		return;

	if (es->password_char == c)
		return;

	es->password_char = c;
	if (c) {
		wnd->dwStyle |= ES_PASSWORD;
		es->style |= ES_PASSWORD;
	} else {
		wnd->dwStyle &= ~ES_PASSWORD;
		es->style &= ~ES_PASSWORD;
	}
	EDIT_UpdateText(wnd, NULL, TRUE);
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
static void EDIT_EM_SetSel(WND *wnd, EDITSTATE *es, UINT start, UINT end, BOOL after_wrap)
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
 *          EDIT_InvalidateText(wnd, es, start, end);
 *          EDIT_InvalidateText(wnd, es, old_start, old_end);
 * in place of the following if statement.                          
 */
            if (old_start > end )
            {
                EDIT_InvalidateText(wnd, es, start, end);
                EDIT_InvalidateText(wnd, es, old_start, old_end);
            }
            else
            {
                EDIT_InvalidateText(wnd, es, start, old_start);
                EDIT_InvalidateText(wnd, es, end, old_end);
            }
	}
        else EDIT_InvalidateText(wnd, es, start, old_end);
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
static void EDIT_EM_SetWordBreakProc(WND *wnd, EDITSTATE *es, LPARAM lParam)
{
	if (es->word_break_proc == (void *)lParam)
		return;

	es->word_break_proc = (void *)lParam;
	es->word_break_proc16 = NULL;

	if ((es->style & ES_MULTILINE) && !(es->style & ES_AUTOHSCROLL)) {
		EDIT_BuildLineDefs_ML(wnd, es);
		EDIT_UpdateText(wnd, NULL, TRUE);
	}
}


/*********************************************************************
 *
 *	EM_SETWORDBREAKPROC16
 *
 */
static void EDIT_EM_SetWordBreakProc16(WND *wnd, EDITSTATE *es, EDITWORDBREAKPROC16 wbp)
{
	if (es->word_break_proc16 == wbp)
		return;

	es->word_break_proc = NULL;
	es->word_break_proc16 = wbp;
	if ((es->style & ES_MULTILINE) && !(es->style & ES_AUTOHSCROLL)) {
		EDIT_BuildLineDefs_ML(wnd, es);
		EDIT_UpdateText(wnd, NULL, TRUE);
	}
}


/*********************************************************************
 *
 *	EM_UNDO / WM_UNDO
 *
 */
static BOOL EDIT_EM_Undo(WND *wnd, EDITSTATE *es)
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

	EDIT_EM_SetSel(wnd, es, es->undo_position, es->undo_position + es->undo_insert_count, FALSE);
	EDIT_EM_EmptyUndoBuffer(es);
	EDIT_EM_ReplaceSel(wnd, es, TRUE, utext, TRUE);
	EDIT_EM_SetSel(wnd, es, es->undo_position, es->undo_position + es->undo_insert_count, FALSE);
	EDIT_EM_ScrollCaret(wnd, es);
	HeapFree(GetProcessHeap(), 0, utext);

	TRACE("after UNDO:insertion length = %d, deletion buffer = %s\n",
			es->undo_insert_count, debugstr_w(es->undo_text));

	if (es->flags & EF_UPDATE) {
		es->flags &= ~EF_UPDATE;
		EDIT_NOTIFY_PARENT(wnd, EN_CHANGE, "EN_CHANGE");
	}

	return TRUE;
}


/*********************************************************************
 *
 *	WM_CHAR
 *
 */
static void EDIT_WM_Char(WND *wnd, EDITSTATE *es, WCHAR c)
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
				EDIT_MoveHome(wnd, es, FALSE);
				EDIT_MoveDown_ML(wnd, es, FALSE);
			} else {
				static const WCHAR cr_lfW[] = {'\r','\n',0};
				EDIT_EM_ReplaceSel(wnd, es, TRUE, cr_lfW, TRUE);
				if (es->flags & EF_UPDATE) {
					es->flags &= ~EF_UPDATE;
					EDIT_NOTIFY_PARENT(wnd, EN_CHANGE, "EN_CHANGE");
				}
			}
		}
		break;
	case '\t':
		if ((es->style & ES_MULTILINE) && !(es->style & ES_READONLY))
		{
			static const WCHAR tabW[] = {'\t',0};
			EDIT_EM_ReplaceSel(wnd, es, TRUE, tabW, TRUE);
			if (es->flags & EF_UPDATE) {
				es->flags &= ~EF_UPDATE;
				EDIT_NOTIFY_PARENT(wnd, EN_CHANGE, "EN_CHANGE");
			}
		}
		break;
	case VK_BACK:
		if (!(es->style & ES_READONLY) && !control) {
			if (es->selection_start != es->selection_end)
				EDIT_WM_Clear(wnd, es);
			else {
				/* delete character left of caret */
				EDIT_EM_SetSel(wnd, es, (UINT)-1, 0, FALSE);
				EDIT_MoveBackward(wnd, es, TRUE);
				EDIT_WM_Clear(wnd, es);
			}
		}
		break;
	case 0x03: /* ^C */
		SendMessageW(wnd->hwndSelf, WM_COPY, 0, 0);
		break;
	case 0x16: /* ^V */
		SendMessageW(wnd->hwndSelf, WM_PASTE, 0, 0);
		break;
	case 0x18: /* ^X */
		SendMessageW(wnd->hwndSelf, WM_CUT, 0, 0);
		break;
	
	default:
		if (!(es->style & ES_READONLY) && (c >= ' ') && (c != 127)) {
			WCHAR str[2];
 			str[0] = c;
 			str[1] = '\0';
 			EDIT_EM_ReplaceSel(wnd, es, TRUE, str, TRUE);
			if (es->flags & EF_UPDATE) {
				es->flags &= ~EF_UPDATE;
				EDIT_NOTIFY_PARENT(wnd, EN_CHANGE, "EN_CHANGE");
			}
 		}
		break;
	}
}


/*********************************************************************
 *
 *	WM_COMMAND
 *
 */
static void EDIT_WM_Command(WND *wnd, EDITSTATE *es, INT code, INT id, HWND control)
{
	if (code || control)
		return;

	switch (id) {
		case EM_UNDO:
			EDIT_EM_Undo(wnd, es);
			break;
		case WM_CUT:
			EDIT_WM_Cut(wnd, es);
			break;
		case WM_COPY:
			EDIT_WM_Copy(wnd, es);
			break;
		case WM_PASTE:
			EDIT_WM_Paste(wnd, es);
			break;
		case WM_CLEAR:
			EDIT_WM_Clear(wnd, es);
			break;
		case EM_SETSEL:
			EDIT_EM_SetSel(wnd, es, 0, (UINT)-1, FALSE);
			EDIT_EM_ScrollCaret(wnd, es);
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
static void EDIT_WM_ContextMenu(WND *wnd, EDITSTATE *es, INT x, INT y)
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

	TrackPopupMenu(popup, TPM_LEFTALIGN | TPM_RIGHTBUTTON, x, y, 0, wnd->hwndSelf, NULL);
	DestroyMenu(menu);
}


/*********************************************************************
 *
 *	WM_COPY
 *
 */
static void EDIT_WM_Copy(WND *wnd, EDITSTATE *es)
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
	OpenClipboard(wnd->hwndSelf);
	EmptyClipboard();
	SetClipboardData(CF_UNICODETEXT, hdst);
	CloseClipboard();
}


/*********************************************************************
 *
 *	WM_CREATE
 *
 */
static LRESULT EDIT_WM_Create(WND *wnd, EDITSTATE *es, LPCWSTR name)
{
	TRACE("%s\n", debugstr_w(name));
       /*
        *	To initialize some final structure members, we call some helper
        *	functions.  However, since the EDITSTATE is not consistent (i.e.
        *	not fully initialized), we should be very careful which
        *	functions can be called, and in what order.
        */
        EDIT_WM_SetFont(wnd, es, 0, FALSE);
        EDIT_EM_EmptyUndoBuffer(es);

       if (name && *name) {
	   EDIT_EM_ReplaceSel(wnd, es, FALSE, name, TRUE);
	   /* if we insert text to the editline, the text scrolls out
            * of the window, as the caret is placed after the insert
            * pos normally; thus we reset es->selection... to 0 and
            * update caret
            */
	   es->selection_start = es->selection_end = 0;
	   EDIT_EM_ScrollCaret(wnd, es);
	   if (es->flags & EF_UPDATE) {
		es->flags &= ~EF_UPDATE;
		EDIT_NOTIFY_PARENT(wnd, EN_CHANGE, "EN_CHANGE");
	   }
       }
       /* force scroll info update */
       EDIT_UpdateScrollInfo(wnd, es);
       return 0;
}


/*********************************************************************
 *
 *	WM_DESTROY
 *
 */
static void EDIT_WM_Destroy(WND *wnd, EDITSTATE *es)
{
	if (es->hloc32W) {
		while (LocalUnlock(es->hloc32W)) ;
		LocalFree(es->hloc32W);
	}
	if (es->hloc32A) {
		while (LocalUnlock(es->hloc32A)) ;
		LocalFree(es->hloc32A);
	}
	if (es->hloc16) {
		while (LOCAL_Unlock(wnd->hInstance, es->hloc16)) ;
		LOCAL_Free(wnd->hInstance, es->hloc16);
	}
	HeapFree(GetProcessHeap(), 0, es);
	*(EDITSTATE **)wnd->wExtra = NULL;
}


/*********************************************************************
 *
 *	WM_ERASEBKGND
 *
 */
static LRESULT EDIT_WM_EraseBkGnd(WND *wnd, EDITSTATE *es, HDC dc)
{
	HBRUSH brush;
	RECT rc;

        if ( get_app_version() >= 0x40000 &&(
                    !es->bEnableState || (es->style & ES_READONLY)))
                brush = (HBRUSH)EDIT_SEND_CTLCOLORSTATIC(wnd, dc);
        else
                brush = (HBRUSH)EDIT_SEND_CTLCOLOR(wnd, dc);

        if (!brush)
                brush = (HBRUSH)GetStockObject(WHITE_BRUSH);

	GetClientRect(wnd->hwndSelf, &rc);
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
static LRESULT EDIT_WM_HScroll(WND *wnd, EDITSTATE *es, INT action, INT pos)
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
		if(wnd->dwStyle & WS_HSCROLL)
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
			EDIT_UpdateScrollInfo(wnd, es);
			EDIT_NOTIFY_PARENT(wnd, EN_HSCROLL, "EN_HSCROLL");
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
		if(wnd->dwStyle & WS_HSCROLL)
		    ret = GetScrollPos(wnd->hwndSelf, SB_HORZ);
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
		EDIT_EM_LineScroll_internal(wnd, es, dx, 0);
	}
	return 0;
}


/*********************************************************************
 *
 *	EDIT_CheckCombo
 *
 */
static BOOL EDIT_CheckCombo(WND *wnd, EDITSTATE *es, UINT msg, INT key)
{
   HWND hLBox = es->hwndListBox;
   HWND hCombo;
   BOOL bDropped;
   int  nEUI;

   if (!hLBox)
      return FALSE;

   hCombo   = wnd->parent->hwndSelf;
   bDropped = TRUE;
   nEUI     = 0;

   TRACE_(combo)("[%04x]: handling msg %04x (%04x)\n",
       		     wnd->hwndSelf, (UINT16)msg, (UINT16)key);

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
static LRESULT EDIT_WM_KeyDown(WND *wnd, EDITSTATE *es, INT key)
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
		if (EDIT_CheckCombo(wnd, es, WM_KEYDOWN, key) || key == VK_F4)
			break;

		/* fall through */
	case VK_LEFT:
		if ((es->style & ES_MULTILINE) && (key == VK_UP))
			EDIT_MoveUp_ML(wnd, es, shift);
		else
			if (control)
				EDIT_MoveWordBackward(wnd, es, shift);
			else
				EDIT_MoveBackward(wnd, es, shift);
		break;
	case VK_DOWN:
		if (EDIT_CheckCombo(wnd, es, WM_KEYDOWN, key))
			break;
		/* fall through */
	case VK_RIGHT:
		if ((es->style & ES_MULTILINE) && (key == VK_DOWN))
			EDIT_MoveDown_ML(wnd, es, shift);
		else if (control)
			EDIT_MoveWordForward(wnd, es, shift);
		else
			EDIT_MoveForward(wnd, es, shift);
		break;
	case VK_HOME:
		EDIT_MoveHome(wnd, es, shift);
		break;
	case VK_END:
		EDIT_MoveEnd(wnd, es, shift);
		break;
	case VK_PRIOR:
		if (es->style & ES_MULTILINE)
			EDIT_MovePageUp_ML(wnd, es, shift);
		else
			EDIT_CheckCombo(wnd, es, WM_KEYDOWN, key);
		break;
	case VK_NEXT:
		if (es->style & ES_MULTILINE)
			EDIT_MovePageDown_ML(wnd, es, shift);
		else
			EDIT_CheckCombo(wnd, es, WM_KEYDOWN, key);
		break;
	case VK_DELETE:
		if (!(es->style & ES_READONLY) && !(shift && control)) {
			if (es->selection_start != es->selection_end) {
				if (shift)
					EDIT_WM_Cut(wnd, es);
				else
					EDIT_WM_Clear(wnd, es);
			} else {
				if (shift) {
					/* delete character left of caret */
					EDIT_EM_SetSel(wnd, es, (UINT)-1, 0, FALSE);
					EDIT_MoveBackward(wnd, es, TRUE);
					EDIT_WM_Clear(wnd, es);
				} else if (control) {
					/* delete to end of line */
					EDIT_EM_SetSel(wnd, es, (UINT)-1, 0, FALSE);
					EDIT_MoveEnd(wnd, es, TRUE);
					EDIT_WM_Clear(wnd, es);
				} else {
					/* delete character right of caret */
					EDIT_EM_SetSel(wnd, es, (UINT)-1, 0, FALSE);
					EDIT_MoveForward(wnd, es, TRUE);
					EDIT_WM_Clear(wnd, es);
				}
			}
		}
		break;
	case VK_INSERT:
		if (shift) {
			if (!(es->style & ES_READONLY))
				EDIT_WM_Paste(wnd, es);
		} else if (control)
			EDIT_WM_Copy(wnd, es);
		break;
	case VK_RETURN:
	    /* If the edit doesn't want the return send a message to the default object */
	    if(!(es->style & ES_WANTRETURN))
	    {
		HWND hwndParent = GetParent(wnd->hwndSelf);
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
static LRESULT EDIT_WM_KillFocus(WND *wnd, EDITSTATE *es)
{
	es->flags &= ~EF_FOCUSED;
	DestroyCaret();
	if(!(es->style & ES_NOHIDESEL))
		EDIT_InvalidateText(wnd, es, es->selection_start, es->selection_end);
	EDIT_NOTIFY_PARENT(wnd, EN_KILLFOCUS, "EN_KILLFOCUS");
	return 0;
}


/*********************************************************************
 *
 *	WM_LBUTTONDBLCLK
 *
 *	The caret position has been set on the WM_LBUTTONDOWN message
 *
 */
static LRESULT EDIT_WM_LButtonDblClk(WND *wnd, EDITSTATE *es)
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
	EDIT_EM_SetSel(wnd, es, s, e, FALSE);
	EDIT_EM_ScrollCaret(wnd, es);
	return 0;
}


/*********************************************************************
 *
 *	WM_LBUTTONDOWN
 *
 */
static LRESULT EDIT_WM_LButtonDown(WND *wnd, EDITSTATE *es, DWORD keys, INT x, INT y)
{
	INT e;
	BOOL after_wrap;

	if (!(es->flags & EF_FOCUSED))
		return 0;

	es->bCaptureState = TRUE;
	SetCapture(wnd->hwndSelf);
	EDIT_ConfinePoint(es, &x, &y);
	e = EDIT_CharFromPos(wnd, es, x, y, &after_wrap);
	EDIT_EM_SetSel(wnd, es, (keys & MK_SHIFT) ? es->selection_start : e, e, after_wrap);
	EDIT_EM_ScrollCaret(wnd, es);
	es->region_posx = es->region_posy = 0;
	SetTimer(wnd->hwndSelf, 0, 100, NULL);
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
static LRESULT EDIT_WM_MButtonDown(WND *wnd)
{  
    SendMessageW(wnd->hwndSelf,WM_PASTE,0,0);  
    return 0;
}


/*********************************************************************
 *
 *	WM_MOUSEMOVE
 *
 */
static LRESULT EDIT_WM_MouseMove(WND *wnd, EDITSTATE *es, INT x, INT y)
{
	INT e;
	BOOL after_wrap;
	INT prex, prey;

	if (GetCapture() != wnd->hwndSelf)
		return 0;

	/*
	 *	FIXME: gotta do some scrolling if outside client
	 *		area.  Maybe reset the timer ?
	 */
	prex = x; prey = y;
	EDIT_ConfinePoint(es, &x, &y);
	es->region_posx = (prex < x) ? -1 : ((prex > x) ? 1 : 0);
	es->region_posy = (prey < y) ? -1 : ((prey > y) ? 1 : 0);
	e = EDIT_CharFromPos(wnd, es, x, y, &after_wrap);
	EDIT_EM_SetSel(wnd, es, es->selection_start, e, after_wrap);
	return 0;
}


/*********************************************************************
 *
 *	WM_NCCREATE
 *
 */
static LRESULT EDIT_WM_NCCreate(WND *wnd, DWORD style, HWND hwndParent, BOOL unicode)
{
	EDITSTATE *es;
	UINT alloc_size;

	TRACE("Creating %s edit control\n", unicode ? "Unicode" : "ANSI");

	if (!(es = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*es))))
		return FALSE;
	*(EDITSTATE **)wnd->wExtra = es;

       /*
        *      Note: since the EDITSTATE has not been fully initialized yet,
        *            we can't use any API calls that may send
        *            WM_XXX messages before WM_NCCREATE is completed.
        */

 	es->is_unicode = unicode;
 	es->style = style;

        es->bEnableState = !(style & WS_DISABLED);

	/*
	 * In Win95 look and feel, the WS_BORDER style is replaced by the 
	 * WS_EX_CLIENTEDGE style for the edit control. This gives the edit 
	 * control a non client area.
	 */
	if (TWEAK_WineLook != WIN31_LOOK)
	{
	  if (es->style & WS_BORDER)
	  {
	    es->style      &= ~WS_BORDER;
	    wnd->dwStyle   &= ~WS_BORDER;
	    wnd->dwExStyle |= WS_EX_CLIENTEDGE;
	  }
	}
	else
	{
	  if ((es->style & WS_BORDER) && !(es->style & WS_DLGFRAME))
	    wnd->dwStyle &= ~WS_BORDER;
	}

	if (es->style & ES_COMBO)
	   es->hwndListBox = GetDlgItem(hwndParent, ID_CB_LISTBOX);

	if (es->style & ES_MULTILINE) {
		es->buffer_limit = BUFLIMIT_MULTI;
		if (es->style & WS_VSCROLL)
			es->style |= ES_AUTOVSCROLL;
		if (es->style & WS_HSCROLL)
			es->style |= ES_AUTOHSCROLL;
		es->style &= ~ES_PASSWORD;
		if ((es->style & ES_CENTER) || (es->style & ES_RIGHT)) {
			if (es->style & ES_RIGHT)
				es->style &= ~ES_CENTER;
			es->style &= ~WS_HSCROLL;
			es->style &= ~ES_AUTOHSCROLL;
		}

		/* FIXME: for now, all multi line controls are AUTOVSCROLL */
		es->style |= ES_AUTOVSCROLL;
	} else {
		es->buffer_limit = BUFLIMIT_SINGLE;
		es->style &= ~ES_CENTER;
		es->style &= ~ES_RIGHT;
		es->style &= ~WS_HSCROLL;
		es->style &= ~WS_VSCROLL;
		es->style &= ~ES_AUTOVSCROLL;
		es->style &= ~ES_WANTRETURN;
		if (es->style & ES_UPPERCASE) {
			es->style &= ~ES_LOWERCASE;
			es->style &= ~ES_NUMBER;
		} else if (es->style & ES_LOWERCASE)
			es->style &= ~ES_NUMBER;
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

	return TRUE;
}

/*********************************************************************
 *
 *	WM_PAINT
 *
 */
static void EDIT_WM_Paint(WND *wnd, EDITSTATE *es, WPARAM wParam)
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
            dc = BeginPaint(wnd->hwndSelf, &ps);
        else
            dc = (HDC) wParam;
	if(es->style & WS_BORDER) {
		GetClientRect(wnd->hwndSelf, &rc);
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
		GetClientRect(wnd->hwndSelf, &rc);
		IntersectClipRect(dc, rc.left, rc.top, rc.right, rc.bottom);
	}
	if (es->font)
		old_font = SelectObject(dc, es->font);
        if ( get_app_version() >= 0x40000 &&(
                    !es->bEnableState || (es->style & ES_READONLY)))
                EDIT_SEND_CTLCOLORSTATIC(wnd, dc);
        else
                EDIT_SEND_CTLCOLOR(wnd, dc);

	if (!es->bEnableState)
		SetTextColor(dc, GetSysColor(COLOR_GRAYTEXT));
	GetClipBox(dc, &rcRgn);
	if (es->style & ES_MULTILINE) {
		INT vlc = (es->format_rect.bottom - es->format_rect.top) / es->line_height;
		for (i = es->y_offset ; i <= min(es->y_offset + vlc, es->y_offset + es->line_count - 1) ; i++) {
			EDIT_GetLineRect(wnd, es, i, 0, -1, &rcLine);
			if (IntersectRect(&rc, &rcRgn, &rcLine))
				EDIT_PaintLine(wnd, es, dc, i, rev);
		}
	} else {
		EDIT_GetLineRect(wnd, es, 0, 0, -1, &rcLine);
		if (IntersectRect(&rc, &rcRgn, &rcLine))
			EDIT_PaintLine(wnd, es, dc, 0, rev);
	}
	if (es->font)
		SelectObject(dc, old_font);

        if (!wParam)
            EndPaint(wnd->hwndSelf, &ps);
}


/*********************************************************************
 *
 *	WM_PASTE
 *
 */
static void EDIT_WM_Paste(WND *wnd, EDITSTATE *es)
{
	HGLOBAL hsrc;
	LPWSTR src;

	/* Protect read-only edit control from modification */
	if(es->style & ES_READONLY)
	    return;

	OpenClipboard(wnd->hwndSelf);
	if ((hsrc = GetClipboardData(CF_UNICODETEXT))) {
		src = (LPWSTR)GlobalLock(hsrc);
		EDIT_EM_ReplaceSel(wnd, es, TRUE, src, TRUE);
		GlobalUnlock(hsrc);

		if (es->flags & EF_UPDATE) {
			es->flags &= ~EF_UPDATE;
			EDIT_NOTIFY_PARENT(wnd, EN_CHANGE, "EN_CHANGE");
		}
	}
	CloseClipboard();
}


/*********************************************************************
 *
 *	WM_SETFOCUS
 *
 */
static void EDIT_WM_SetFocus(WND *wnd, EDITSTATE *es)
{
	es->flags |= EF_FOCUSED;
	CreateCaret(wnd->hwndSelf, 0, 2, es->line_height);
	EDIT_SetCaretPos(wnd, es, es->selection_end,
			 es->flags & EF_AFTER_WRAP);
	if(!(es->style & ES_NOHIDESEL))
		EDIT_InvalidateText(wnd, es, es->selection_start, es->selection_end);
	ShowCaret(wnd->hwndSelf);
	EDIT_NOTIFY_PARENT(wnd, EN_SETFOCUS, "EN_SETFOCUS");
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
static void EDIT_WM_SetFont(WND *wnd, EDITSTATE *es, HFONT font, BOOL redraw)
{
	TEXTMETRICW tm;
	HDC dc;
	HFONT old_font = 0;
	RECT r;

	es->font = font;
	dc = GetDC(wnd->hwndSelf);
	if (font)
		old_font = SelectObject(dc, font);
	GetTextMetricsW(dc, &tm);
	es->line_height = tm.tmHeight;
	es->char_width = tm.tmAveCharWidth;
	if (font)
		SelectObject(dc, old_font);
	ReleaseDC(wnd->hwndSelf, dc);
	if (font && (TWEAK_WineLook > WIN31_LOOK))
		EDIT_EM_SetMargins(es, EC_LEFTMARGIN | EC_RIGHTMARGIN,
				   EC_USEFONTINFO, EC_USEFONTINFO);

	/* Force the recalculation of the format rect for each font change */
	GetClientRect(wnd->hwndSelf, &r);
	EDIT_SetRectNP(wnd, es, &r);

	if (es->style & ES_MULTILINE)
		EDIT_BuildLineDefs_ML(wnd, es);
	else
	    EDIT_CalcLineWidth_SL(wnd, es);

	if (redraw)
		EDIT_UpdateText(wnd, NULL, TRUE);
	if (es->flags & EF_FOCUSED) {
		DestroyCaret();
		CreateCaret(wnd->hwndSelf, 0, 2, es->line_height);
		EDIT_SetCaretPos(wnd, es, es->selection_end,
				 es->flags & EF_AFTER_WRAP);
		ShowCaret(wnd->hwndSelf);
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
static void EDIT_WM_SetText(WND *wnd, EDITSTATE *es, LPARAM lParam, BOOL unicode)
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

	EDIT_EM_SetSel(wnd, es, 0, (UINT)-1, FALSE);
	if (text) {
		TRACE("%s\n", debugstr_w(text));
		EDIT_EM_ReplaceSel(wnd, es, FALSE, text, !(es->style & (ES_MULTILINE | ES_COMBO)));
		if(!unicode)
		    HeapFree(GetProcessHeap(), 0, text);
	} else {
		static const WCHAR empty_stringW[] = {0};
		TRACE("<NULL>\n");
		EDIT_EM_ReplaceSel(wnd, es, FALSE, empty_stringW, !(es->style & (ES_MULTILINE | ES_COMBO)));
	}
	es->x_offset = 0;
	es->flags &= ~EF_MODIFIED;
	EDIT_EM_SetSel(wnd, es, 0, 0, FALSE);
	EDIT_EM_ScrollCaret(wnd, es);

	if (es->flags & EF_UPDATE) {
		es->flags &= ~EF_UPDATE;
		EDIT_NOTIFY_PARENT(wnd, EN_CHANGE, "EN_CHANGE");
	}
}


/*********************************************************************
 *
 *	WM_SIZE
 *
 */
static void EDIT_WM_Size(WND *wnd, EDITSTATE *es, UINT action, INT width, INT height)
{
	if ((action == SIZE_MAXIMIZED) || (action == SIZE_RESTORED)) {
		RECT rc;
		TRACE("width = %d, height = %d\n", width, height);
		SetRect(&rc, 0, 0, width, height);
		EDIT_SetRectNP(wnd, es, &rc);
		EDIT_UpdateText(wnd, NULL, TRUE);
	}
}


/*********************************************************************
 *
 *	WM_SYSKEYDOWN
 *
 */
static LRESULT EDIT_WM_SysKeyDown(WND *wnd, EDITSTATE *es, INT key, DWORD key_data)
{
	if ((key == VK_BACK) && (key_data & 0x2000)) {
		if (EDIT_EM_CanUndo(es))
			EDIT_EM_Undo(wnd, es);
		return 0;
	} else if (key == VK_UP || key == VK_DOWN) {
		if (EDIT_CheckCombo(wnd, es, WM_SYSKEYDOWN, key))
			return 0;
	}
	return DefWindowProcW(wnd->hwndSelf, WM_SYSKEYDOWN, (WPARAM)key, (LPARAM)key_data);
}


/*********************************************************************
 *
 *	WM_TIMER
 *
 */
static void EDIT_WM_Timer(WND *wnd, EDITSTATE *es)
{
	if (es->region_posx < 0) {
		EDIT_MoveBackward(wnd, es, TRUE);
	} else if (es->region_posx > 0) {
		EDIT_MoveForward(wnd, es, TRUE);
	}
/*
 *	FIXME: gotta do some vertical scrolling here, like
 *		EDIT_EM_LineScroll(wnd, 0, 1);
 */
}

/*********************************************************************
 *
 *	WM_VSCROLL
 *
 */
static LRESULT EDIT_WM_VScroll(WND *wnd, EDITSTATE *es, INT action, INT pos)
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
		EDIT_EM_Scroll(wnd, es, action);
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
			EDIT_UpdateScrollInfo(wnd, es);
			EDIT_NOTIFY_PARENT(wnd, EN_VSCROLL, "EN_VSCROLL");
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
		if(wnd->dwStyle & WS_VSCROLL)
		    ret = GetScrollPos(wnd->hwndSelf, SB_VERT);
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
		EDIT_EM_LineScroll(wnd, es, 0, dy);
	return 0;
}


/*********************************************************************
 *
 *	EDIT_UpdateText
 *
 */
static void EDIT_UpdateText(WND *wnd, LPRECT rc, BOOL bErase)
{
    EDITSTATE *es = *(EDITSTATE **)((wnd)->wExtra);

    /* EF_UPDATE will be turned off in paint */
    if (es->flags & EF_UPDATE)
	EDIT_NOTIFY_PARENT(wnd, EN_UPDATE, "EN_UPDATE");

    InvalidateRect(wnd->hwndSelf, rc, bErase);
}
