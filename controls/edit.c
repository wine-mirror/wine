/*
 * Edit control
 *
 * Copyright  David W. Metcalfe, 1994
 * Copyright  William Magro, 1995, 1996
 * Copyright  Frans van Dorsselaer, 1996
 *
 * Note: I'm doing a rewrite in order to implement word wrap
 *       Please e-mail me if you want to word on edit.c as well, so
 *       we can synchronize.
 *
 *       Frans van Dorsselaer
 *       dorssel@rulhm1.LeidenUniv.nl
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "instance.h"
#include "local.h"
#include "win.h"
#include "class.h"
#include "stackframe.h" /* for MAKE_SEGPTR */
#include "user.h"
#include "stddebug.h"
#include "debug.h"
#include "xmalloc.h"
#include "callback.h"

#ifdef WINELIB32
#define NOTIFY_PARENT(hWndCntrl, wNotifyCode) \
	SendMessage(GetParent(hWndCntrl), WM_COMMAND, \
		    MAKEWPARAM(GetDlgCtrlID(hWndCntrl),wNotifyCode), \
		    (LPARAM)hWndCntrl );
#else
#define NOTIFY_PARENT(hWndCntrl, wNotifyCode) \
	SendMessage(GetParent(hWndCntrl), WM_COMMAND, \
		 GetDlgCtrlID(hWndCntrl), MAKELPARAM(hWndCntrl, wNotifyCode));
#endif

#define MAXTEXTLEN 30000   /* maximum text buffer length */
#define EDITLEN     1024   /* starting length for multi-line control */
#define ENTRYLEN     256   /* starting length for single line control */
#define GROWLENGTH    64   /* buffers grow by this much */

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
    INT *CharWidths;         /* widths of chars in font */
    unsigned int *textptrs;  /* list of line offsets */
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
    HANDLE hDeletedText;     /* handle to deleted text buffer for undo */
    int DeletedLength;       /* length of deleted text */
    int DeletedCurrLine;     /* starting line from which text was deleted */
    int DeletedCurrCol;      /* starting col from which text was deleted */
    int NumTabStops;         /* number of tab stops in buffer hTabStops */
    unsigned short *TabStops;/* tab stops buffer */
    BOOL HaveFocus;          /* TRUE if this edit has the focus */
    int ClientWidth;         /* computed from the window's ClientRect */
    int ClientHeight;        /* ditto */
    char PasswordChar;       /* The password character */
    EDITWORDBREAKPROC WordBreakProc;
    int CaretPrepareCount;   /* Did we already prepare the caret ? */
    BOOL CaretHidden;        /* Did we hide the caret during painting ? */
    int oldWndCol;           /* WndCol before we started painting */
    int oldWndRow;           /* ditto for WndRow */
} EDITSTATE;


#define EditBufStartLen(hwnd) (GetWindowLong(hwnd,GWL_STYLE) & ES_MULTILINE \
			       ? EDITLEN : ENTRYLEN)
#define CurrChar (EDIT_TextLine(hwnd, es->CurrLine) + es->CurrCol)
#define SelMarked(es) (((es)->SelBegCol != (es)->SelEndCol) || \
		       ((es)->SelBegLine  != (es)->SelEndLine))
#define ROUNDUP(numer, denom) (((numer) % (denom)) \
			       ? ((((numer) + (denom)) / (denom)) * (denom)) \
			       : (numer) + (denom))

/* "line" dimension for horizontal scroll */
#define HSCROLLDIM(es) ((es)->ClientWidth / 3)

/* macros to access window styles */
#define IsMultiLine(hwnd) (GetWindowLong(hwnd,GWL_STYLE) & ES_MULTILINE)
#define IsVScrollBar(hwnd) (GetWindowLong(hwnd,GWL_STYLE) & WS_VSCROLL)
#define IsHScrollBar(hwnd) (GetWindowLong(hwnd,GWL_STYLE) & WS_HSCROLL)
#define IsReadOnly(hwnd) (GetWindowLong(hwnd,GWL_STYLE) & ES_READONLY)

/* internal variables */
static BOOL TextMarking;         /* TRUE if text marking in progress */
static BOOL ButtonDown;          /* TRUE if left mouse button down */
static int ButtonRow;              /* row in text buffer when button pressed */
static int ButtonCol;              /* col in text buffer when button pressed */

#define SWAP_INT(x,y) do { int temp = (x); (x) = (y); (y) = temp; } while(0)


/*********************************************************************
 *  EDIT_HeapAlloc
 *
 *  Allocate the specified number of bytes on the specified local heap.
 */
static HLOCAL EDIT_HeapAlloc(HWND hwnd, int bytes, WORD flags)
{
    HLOCAL ret;
    
    ret = LOCAL_Alloc( WIN_GetWindowInstance(hwnd), flags, bytes );
    if (!ret)
	fprintf(stderr, "EDIT_HeapAlloc: Out of heap-memory\n");
    return ret;
}

/*********************************************************************
 *  EDIT_HeapLock
 *
 *  Return the address of the memory pointed to by the handle.
 */
static void *EDIT_HeapLock(HWND hwnd, HANDLE handle)
{
    HINSTANCE hinstance = WIN_GetWindowInstance( hwnd );
#if defined(WINELIB)
    return LOCAL_Lock( hinstance, handle );
#else
    HANDLE offs;
    
    if (handle == 0) return 0;
    offs = LOCAL_Lock( hinstance, handle );
    return PTR_SEG_OFF_TO_LIN( hinstance, offs );
#endif
}

/*********************************************************************
 *  EDIT_HeapUnlock
 */
static void EDIT_HeapUnlock(HWND hwnd, HANDLE handle)
{
    if (handle == 0) return;
    LOCAL_Unlock( WIN_GetWindowInstance( hwnd ), handle );
}

/*********************************************************************
 *  EDIT_HeapReAlloc
 *
 *  Reallocate the memory pointed to by the handle.
 */
static HLOCAL EDIT_HeapReAlloc(HWND hwnd, HANDLE handle, int bytes)
{
    return LOCAL_ReAlloc( WIN_GetWindowInstance(hwnd), handle, bytes, 
			  LMEM_FIXED );
}


/*********************************************************************
 *  EDIT_HeapFree
 *
 *  Frees the memory pointed to by the handle.
 */
static void EDIT_HeapFree(HWND hwnd, HANDLE handle)
{
    LOCAL_Free( WIN_GetWindowInstance(hwnd), handle );
}


/*********************************************************************
 *  EDIT_HeapSize
 *
 *  Return the size of the given object on the local heap.
 */
static unsigned int EDIT_HeapSize(HWND hwnd, HANDLE handle)
{
    return LOCAL_Size( WIN_GetWindowInstance(hwnd), handle );
}

/********************************************************************
 * EDIT_RecalcSize
 * 
 * Sets the ClientWidth/ClientHeight fields of the EDITSTATE
 * Called on WM_SIZE and WM_SetFont messages
 */
static void EDIT_RecalcSize(HWND hwnd, EDITSTATE *es)
{
    RECT rect;
    GetClientRect(hwnd,&rect);
    es->ClientWidth = rect.right > rect.left ? rect.right - rect.left : 0;
    es->ClientHeight = rect.bottom > rect.top ? (rect.bottom - rect.top) / es->txtht : 0;
}

/*********************************************************************
 * EDIT_GetEditState
 */
static EDITSTATE *EDIT_GetEditState(HWND hwnd)
{
    return (EDITSTATE *)GetWindowLong(hwnd,0);
}

/*********************************************************************
 *  EDIT_CaretPrepare
 *
 *  Save the caret state before any painting is done.
 */
static void EDIT_CaretPrepare(HWND hwnd)
{
    EDITSTATE *es = EDIT_GetEditState(hwnd);

    if (!es) return;

    if (!es->CaretPrepareCount)
    {
	es->CaretHidden = FALSE;
	es->oldWndCol = es->WndCol;
	es->oldWndRow = es->WndRow;
    }

    es->CaretPrepareCount++;
}

/*********************************************************************
 *  EDIT_CaretHide
 *
 *  Called before some painting is done.
 */
static void EDIT_CaretHide(HWND hwnd)
{
    EDITSTATE *es = EDIT_GetEditState(hwnd);

    if (!es) return;
    if (!es->HaveFocus) return;
    if (!es->CaretPrepareCount) return;
    
    if (!es->CaretHidden)
    {
    	HideCaret(hwnd);
    	es->CaretHidden = TRUE;
    }
}

/*********************************************************************
 *  EDIT_CaretUpdate
 *
 *  Called after all painting is done.
 */
static void EDIT_CaretUpdate(HWND hwnd)
{
    EDITSTATE *es = EDIT_GetEditState(hwnd);

    if (!es) return;
    if (!es->CaretPrepareCount) return;
    
    es->CaretPrepareCount--;
    
    if (es->CaretPrepareCount) return;
    if (!es->HaveFocus) return;

    if ((es->WndCol != es->oldWndCol) || (es->WndRow != es->oldWndRow))
	SetCaretPos(es->WndCol, es->WndRow * es->txtht);

    if (es->CaretHidden)
    {
	ShowCaret(hwnd);
	es->CaretHidden = FALSE;
    }
}

/*********************************************************************
 *  EDIT_WordBreakProc
 *
 *  Find the beginning of words.
 *  Note: unlike the specs for a WordBreakProc, this function only
 *        allows to be called without linebreaks between s[0] upto
 *        s[count - 1].  Remember it is only called
 *        internally, so we can decide this for ourselves.
 */
static int EDIT_WordBreakProc(char *s, int index, int count, int action)
{
    int ret = 0;

    dprintf_edit(stddeb, "EDIT_WordBreakProc: s=%p, index=%d"
	", count=%d, action=%d\n", s, index, count, action);

    switch (action) {
    case WB_LEFT:
	if (!count) break;
	if (index) index--;
	if (s[index] == ' ') {
	    while (index && (s[index] == ' ')) index--;
	    if (index) {
		while (index && (s[index] != ' ')) index--;
		if (s[index] == ' ') index++;
	    }
	} else {
	    while (index && (s[index] != ' ')) index--;
	    if (s[index] == ' ') index++;
	}
	ret = index;
	break;
    case WB_RIGHT:
	if (!count) break;
	if (index) index--;
	if (s[index] == ' ')
	    while ((index < count) && (s[index] == ' ')) index++;
	else {
	    while (s[index] && (s[index] != ' ') && (index < count)) index++;
	    while ((s[index] == ' ') && (index < count)) index++;
	}
	ret = index;
	break;
    case WB_ISDELIMITER:
	ret = (s[index] == ' ');
	break;
    default:
	fprintf(stderr, "EDIT_WordBreakProc: unknown action code !\n");
	break;
    }
    return ret;
}

/*********************************************************************
 *  EDIT_CallWordBreakProc
 *
 *  Call appropriate WordBreakProc (internal or external).
 */
static int CALLBACK EDIT_CallWordBreakProc(HWND hwnd, char *pch, 
					int ichCurrent, int cch, int code)
{
    EDITSTATE *es = EDIT_GetEditState(hwnd);
    
    if (es->WordBreakProc) {
	/* FIXME: do some stuff to make pch a SEGPTR */
	return CallWordBreakProc((FARPROC)es->WordBreakProc, (LONG)pch, ichCurrent, cch, code);
    } else return EDIT_WordBreakProc(pch, ichCurrent, cch, code);
}

/*********************************************************************
 *  EDIT_GetNextTabStop
 *
 *  Return the next tab stop beyond _pcol_.
 */
static int EDIT_GetNextTabStop(HWND hwnd, int pcol)
{
    int i;
    int baseUnitWidth = LOWORD(GetDialogBaseUnits());
    EDITSTATE *es = EDIT_GetEditState(hwnd);

    if (es->NumTabStops == 0)
	return ROUNDUP(pcol, 8 * baseUnitWidth);
    if (es->NumTabStops == 1)
	return ROUNDUP(pcol, es->TabStops[0] * baseUnitWidth / 4);
    for (i = 0; i < es->NumTabStops; i++)
    {
	if (es->TabStops[i] * baseUnitWidth / 4 >= pcol)
	    return es->TabStops[i] * baseUnitWidth / 4;
    }
    return pcol;
}

/*********************************************************************
 *  EDIT_CharWidth
 *
 *  Return the width of the given character in pixels.
 *  The current column offset in pixels _pcol_ is required to calculate 
 *  the width of a tab.
 */
static int EDIT_CharWidth(HWND hwnd, short ch, int pcol)
{
    EDITSTATE *es = EDIT_GetEditState(hwnd);

    if (ch == VK_TAB) return EDIT_GetNextTabStop(hwnd, pcol) - pcol;
    return es->CharWidths[ch];
}

/*********************************************************************
 *  EDIT_ClearTextPointers
 *
 *  Clear and initialize text line pointer array.
 */
static void EDIT_ClearTextPointers(HWND hwnd)
{
    EDITSTATE *es = EDIT_GetEditState(hwnd);
    
    dprintf_edit( stddeb, "EDIT_ClearTextPointers\n" );
    es->textptrs = xrealloc(es->textptrs, sizeof(int));
    es->textptrs[0] = 0;
}

/*********************************************************************
 *  EDIT_BuildTextPointers
 *
 *  Build array of pointers to text lines.
 */
static void EDIT_BuildTextPointers(HWND hwnd)
{
    char *text, *cp;
    unsigned int off, len, line;
    EDITSTATE *es;

    es = EDIT_GetEditState(hwnd);
    text = EDIT_HeapLock(hwnd, es->hText);

    es->textwidth = 0;
    if (IsMultiLine(hwnd)) {
	es->wlines = 0;
	cp = text;
	while ((cp = strchr(cp,'\n')) != NULL) {
	    es->wlines++; cp++;
	}
    } else es->wlines = 1;
    
    dprintf_edit( stddeb, "EDIT_BuildTextPointers: realloc\n" );
    es->textptrs = xrealloc(es->textptrs, (es->wlines + 2) * sizeof(int));
    
    cp = text;
    dprintf_edit(stddeb,"BuildTextPointers: %d lines, pointer %p\n", 
		 es->wlines, es->textptrs);

    /* advance through text buffer */
    line = 0;
    while (*cp)
    {
	off = cp - text;     /* offset of beginning of line */
        dprintf_edit(stddeb,"BuildTextPointers: line %d offs %d\n", line, off);
	es->textptrs[line] = off;
	line++;
	len = 0;
	
	/* advance through current line */
	while (*cp && *cp != '\n')
	{
	    len += EDIT_CharWidth(hwnd, (BYTE)*cp, len);
	                                     /* width of line in pixels */
	    cp++;
	}
	es->textwidth = MAX(es->textwidth, len);
	if (*cp)
	    cp++;                            /* skip '\n' */
    }
    off = cp - text;
    es->textptrs[line] = off;
    EDIT_HeapUnlock(hwnd, es->hText);
}

/*********************************************************************
 *  EDIT_ModTextPointers
 *
 *  Modify text pointers from a specified position.
 */
static void EDIT_ModTextPointers(HWND hwnd, int lineno, int var)
{
    EDITSTATE *es = EDIT_GetEditState(hwnd);
    for(;lineno < es->wlines; lineno++) es->textptrs[lineno] += var;
}

/*********************************************************************
 *  EDIT_TextLine
 *
 *  Return a pointer to the text in the specified line.
 */
static char *EDIT_TextLine(HWND hwnd, int sel)
{
    EDITSTATE *es = EDIT_GetEditState(hwnd);
    char *text = EDIT_HeapLock(hwnd, es->hText);

    if (sel > es->wlines) return NULL;
    dprintf_edit(stddeb,"EDIT_TextLine: text %p, line %d offs %d\n", 
		 text, sel, es->textptrs[sel]);
    return text + es->textptrs[sel];
}
    
/*********************************************************************
 *  EDIT_GetTextLine
 *
 *  Get a copy of the text in the specified line.
 */
static char *EDIT_GetTextLine(HWND hwnd, int selection)
{
    int len;
    char *cp, *cp1;

    dprintf_edit(stddeb,"GetTextLine %d\n", selection);
    cp1 = EDIT_TextLine(hwnd, selection);

    /* Find end of line */
    cp = strchr( cp1, '\r' );
    if (cp == NULL) len = strlen(cp1);
    else len = cp - cp1;
    
    /* store selected line and return handle */
    cp = xmalloc( len + 1 );
    strncpy( cp, cp1, len);
    cp[len] = 0;
    return cp;
}

/*********************************************************************
 *  EDIT_StrWidth
 *
 *  Return length of string _str_ of length _len_ characters in pixels.
 *  The current column offset in pixels _pcol_ is required to calculate 
 *  the width of a tab.
 */
static int EDIT_StrWidth(HWND hwnd, unsigned char *str, int len, int pcol)
{
    int i, plen = 0;

    for (i = 0; i < len; i++)
	plen += EDIT_CharWidth(hwnd, (BYTE)(*(str + i)), pcol + plen);

    dprintf_edit(stddeb,"EDIT_StrWidth: returning %d, len=%d\n", plen,len);
    return plen;
}

/*********************************************************************
 *  EDIT_LineLength
 *
 *  Return length of line _num_ in characters.
 */
static int EDIT_LineLength(HWND hwnd, int num)
{
    char *cp = EDIT_TextLine(hwnd, num);
    char *cp1;

    if(!cp)return 0;
    cp1 = strchr(cp, '\r');
    return cp1 ? (cp1 - cp) : strlen(cp);
}

/*********************************************************************
 *  EDIT_GetStr
 *
 *  Return sub-string starting at pixel _off_ of length _len_ pixels.
 *  If _off_ is part way through a character, the negative offset of
 *  the beginning of the character is returned in _diff_, else _diff_ 
 *  will be zero.
 */
static HANDLE EDIT_GetStr(HWND hwnd, char *lp, int off, int len, int *diff)
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
    if (i > off)
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
    
    hStr = EDIT_HeapAlloc(hwnd, ch - ch1 + 3, LMEM_FIXED);
    str = (char *)EDIT_HeapLock(hwnd, hStr);
    for (i = ch1, j = 0; i < ch; i++, j++)
	str[j] = lp[i];
    str[j] = '\0';
    dprintf_edit(stddeb,"EDIT_GetStr: returning %s\n", str);
    return hStr;
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
static void EDIT_WriteText(HWND hwnd, char *lp, int off, int len, int row, 
			   int col, RECT *rc, BOOL blank, BOOL reverse)
{
    HDC hdc;
    HANDLE hStr;
    char *str, *cp, *cp1;
    int diff=0, tabwidth, scol;
    HRGN hrgnClip;
    COLORREF oldTextColor, oldBkgdColor;
    HFONT oldfont;
    EDITSTATE *es = EDIT_GetEditState(hwnd);
    RECT rc2;

    dprintf_edit(stddeb,"EDIT_WriteText lp=%s, off=%d, len=%d, row=%d, col=%d, reverse=%d\n", lp, off, len, row, col, reverse);

    EDIT_CaretHide(hwnd);

    if( off < 0 ) {
      len += off;
      col -= off;
      off = 0;
    }
	
    hdc = GetDC(hwnd);
    hStr = EDIT_GetStr(hwnd, lp, off, len, &diff);
    str = (char *)EDIT_HeapLock(hwnd, hStr);
    hrgnClip = CreateRectRgnIndirect(rc);
    SelectClipRgn(hdc, hrgnClip);
    DeleteObject(hrgnClip);

    if (es->hFont)
	oldfont = (HFONT)SelectObject(hdc, (HANDLE)es->hFont);
    else
        oldfont = 0;		/* -Wall does not see the use of if */

#ifdef WINELIB32
    SendMessage(GetParent(hwnd), WM_CTLCOLOREDIT, (WPARAM)hdc, (LPARAM)hwnd);
#else
    SendMessage(GetParent(hwnd), WM_CTLCOLOR, (WPARAM)hdc,
		MAKELPARAM(hwnd, CTLCOLOR_EDIT));
#endif

    if (reverse)
    {
	oldBkgdColor = GetBkColor(hdc);
	oldTextColor = GetTextColor(hdc);
	SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
	SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
    }
    else			/* -Wall does not see the use of if */
        oldTextColor = oldBkgdColor = 0;

    if ((es->PasswordChar && GetWindowLong( hwnd, GWL_STYLE ) & ES_PASSWORD))
    {
        int len = strlen(str);
        char *buff = xmalloc( len+1 );
        memset( buff, es->PasswordChar, len );
        buff[len] = '\0';
	TextOut( hdc, col - diff, row * es->txtht, buff, len );
    }
    else if (!(cp = strchr(str, VK_TAB)))
	TextOut(hdc, col - diff, row * es->txtht, str, strlen(str));
    else
    {
	TextOut(hdc, col - diff, row * es->txtht, str, (int)(cp - str));
	scol = EDIT_StrWidth(hwnd, str, (int)(cp - str), 0);
	tabwidth = EDIT_CharWidth(hwnd, VK_TAB, scol);
	SetRect(&rc2, scol, row * es->txtht, scol+tabwidth, (row + 1) * es->txtht);
	ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc2, "", 0, NULL);
	cp++;
	scol += tabwidth;

	while ((cp1 = strchr(cp, VK_TAB)))
	{
	    TextOut(hdc, scol, row * es->txtht, cp, (int)(cp1 - cp));
	    scol += EDIT_StrWidth(hwnd, cp, (int)(cp1 - cp), scol);
	    tabwidth = EDIT_CharWidth(hwnd, VK_TAB, scol);
	    SetRect(&rc2, scol, row * es->txtht, scol+tabwidth, (row + 1) * es->txtht);
	    ExtTextOut( hdc, 0, 0, ETO_OPAQUE, &rc2, "", 0, NULL );
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
    if (blank && ((rc->right - col) > len))
    {
        SetRect( &rc2, col + len, row * es->txtht,
                 rc->right, (row + 1) * es->txtht );
        ExtTextOut( hdc, 0, 0, ETO_OPAQUE, &rc2, "", 0, NULL );
    }

    if (es->hFont)
	SelectObject(hdc, (HANDLE)oldfont);

    EDIT_HeapFree(hwnd, hStr);
    ReleaseDC(hwnd, hdc);
}

/*********************************************************************
 *  EDIT_WriteTextLine
 *
 *  Write the line of text at offset _y_ in text buffer to a window.
 */
static void EDIT_WriteTextLine(HWND hwnd, RECT *rect, int y)
{
    int len = 0;
    unsigned char *lp;
    int lnlen, lnlen1;
    int col, off = 0;
    int sbl, sel, sbc, sec;
    RECT rc;
    EDITSTATE *es = EDIT_GetEditState(hwnd);

    /* initialize rectangle if NULL, else copy */
    if (rect)
	CopyRect(&rc, rect);
    else
	GetClientRect(hwnd, &rc);

    dprintf_edit(stddeb,"WriteTextLine %d\n", y);

    /* make sure y is inside the window */
    if (y < es->wtop || y > (es->wtop + es->ClientHeight))
    {
	dprintf_edit(stddeb,"EDIT_WriteTextLine: y (%d) is not a displayed line\n", y);
	return;
    }

    /* make sure rectangle is within window */
    if (rc.left >= es->ClientWidth - 1)
    {
	dprintf_edit(stddeb,"EDIT_WriteTextLine: rc.left (%ld) is greater than right edge\n",
	       (LONG)rc.left);
	return;
    }
    if (rc.right <= 0)
    {
	dprintf_edit(stddeb,"EDIT_WriteTextLine: rc.right (%ld) is less than left edge\n",
	       (LONG)rc.right);
	return;
    }
    if (y - es->wtop < (rc.top / es->txtht) || 
	y - es->wtop > (rc.bottom / es->txtht))
    {
	dprintf_edit(stddeb,"EDIT_WriteTextLine: y (%d) is outside window\n", y);
	return;
    }

    /* get the text and length of line */
    lp = EDIT_GetTextLine( hwnd, y );
    if (lp == NULL) return;
    
    lnlen = EDIT_StrWidth( hwnd, lp, strlen(lp), 0 );
    lnlen1 = lnlen;

    /* build the line to display */
    if (lnlen < (es->wleft + rc.left))
    {
	lnlen = 0;
        return;
    }
    else
    {
	off += es->wleft;
	lnlen -= off;
    }

    if (lnlen > rc.left)
    {
	off += rc.left;
	lnlen = lnlen1 - off;
    }
    len = MIN(lnlen, rc.right - rc.left);

    if (SelMarked(es) && (es->HaveFocus))
    {
	sbl = es->SelBegLine;
	sel = es->SelEndLine;
	sbc = es->SelBegCol;
	sec = es->SelEndCol;

	/* put lowest marker first */
	if (sbl > sel)
	{
	    SWAP_INT(sbl, sel);
	    SWAP_INT(sbc, sec);
	}
	if (sbl == sel && sbc > sec)
	    SWAP_INT(sbc, sec);

	if (y < sbl || y > sel)
	    EDIT_WriteText(hwnd, lp, off, len, y - es->wtop, rc.left, &rc,
			   TRUE, FALSE);
	else if (y > sbl && y < sel)
	    EDIT_WriteText(hwnd, lp, off, len, y - es->wtop, rc.left, &rc,
			   TRUE, TRUE);
	else if (y == sbl)
	{
	    col = EDIT_StrWidth(hwnd, lp, sbc, 0);
	    if (col > (es->wleft + rc.left))
	    {
		len = MIN(col - off, rc.right - off);
		EDIT_WriteText(hwnd, lp, off, len, y - es->wtop, 
			       rc.left, &rc, FALSE, FALSE);
		off = col;
	    }
	    if (y == sel)
	    {
		col = EDIT_StrWidth(hwnd, lp, sec, 0);
		if (col < (es->wleft + rc.right))
		{
		    len = MIN(col - off, rc.right - off);
		    EDIT_WriteText(hwnd, lp, off, len, y - es->wtop,
				   off - es->wleft, &rc, FALSE, TRUE);
		    off = col;
		    len = MIN(lnlen - off, rc.right - off);
		    EDIT_WriteText(hwnd, lp, off, len, y - es->wtop,
				   off - es->wleft, &rc, TRUE, FALSE);
		}
		else
		{
		    len = MIN(lnlen - off, rc.right - off);
		    EDIT_WriteText(hwnd, lp, off, len, y - es->wtop,
				   off - es->wleft, &rc, TRUE, TRUE);
		}
	    }
	    else
	    {
		len = MIN(lnlen - off, rc.right - off);
		if (col < (es->wleft + rc.right))
		    EDIT_WriteText(hwnd, lp, off, len, y - es->wtop,
				   off - es->wleft, &rc, TRUE, TRUE);
	    }
	}
	else if (y == sel)
	{
	    col = EDIT_StrWidth(hwnd, lp, sec, 0);
	    if (col < (es->wleft + rc.right))
	    {
		len = MIN(col - off, rc.right - off);
		EDIT_WriteText(hwnd, lp, off, len, y - es->wtop,
			       off - es->wleft, &rc, FALSE, TRUE);
		off = col;
		len = MIN(lnlen - off, rc.right - off);
		EDIT_WriteText(hwnd, lp, off, len, y - es->wtop,
			       off - es->wleft, &rc, TRUE, FALSE);
	    }
	}
    }
    else 
	EDIT_WriteText(hwnd, lp, off, len, y - es->wtop, rc.left, &rc,
		       TRUE, FALSE);

    free( lp );
}

/*********************************************************************
 *  EDIT_ComputeVScrollPos
 *
 *  Compute the vertical scroll bar position from the window
 *  position and text width.
 */
static int EDIT_ComputeVScrollPos(HWND hwnd)
{
    int vscrollpos;
    INT minpos, maxpos;
    EDITSTATE *es = EDIT_GetEditState(hwnd);

    GetScrollRange(hwnd, SB_VERT, &minpos, &maxpos);

    if (es->wlines > es->ClientHeight)
	vscrollpos = (double)(es->wtop) / (double)(es->wlines - 
		     es->ClientHeight) * (maxpos - minpos);
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
static int EDIT_ComputeHScrollPos(HWND hwnd)
{
    int hscrollpos;
    INT minpos, maxpos;
    EDITSTATE *es = EDIT_GetEditState(hwnd);

    GetScrollRange(hwnd, SB_HORZ, &minpos, &maxpos);

    if (es->textwidth > es->ClientWidth)
	hscrollpos = (double)(es->wleft) / (double)(es->textwidth - 
		     es->ClientWidth) * (maxpos - minpos);
    else
	hscrollpos = minpos;

    return hscrollpos;
}

/*********************************************************************
 *  EDIT_KeyHScroll
 *
 *  Scroll text horizontally using cursor keys.
 */
static void EDIT_KeyHScroll(HWND hwnd, WORD opt)
{
    int hscrollpos;
    EDITSTATE *es = EDIT_GetEditState(hwnd);

    if (opt == SB_LINEDOWN)
    {
	es->wleft += HSCROLLDIM(es);
	es->WndCol -= HSCROLLDIM(es);
    }
    else
    {
	if (es->wleft == 0)
	    return;
	if (es->wleft - HSCROLLDIM(es) < 0)
	{
	    es->WndCol += es->wleft;
	    es->wleft = 0;
	}	    
	else
	{
	    es->wleft -= HSCROLLDIM(es);
	    es->WndCol += HSCROLLDIM(es);
	}
    }

    InvalidateRect(hwnd, NULL, FALSE);
    UpdateWindow(hwnd);

    if (IsHScrollBar(hwnd))
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
static void EDIT_KeyVScrollLine(HWND hwnd, WORD opt)
{
    RECT rc;
    int y, vscrollpos;
    EDITSTATE *es = EDIT_GetEditState(hwnd);

    if (!IsMultiLine(hwnd))
	return;

    if (opt == SB_LINEDOWN)
    {
	/* move down one line */
	if (es->wtop + es->ClientHeight >= es->wlines)
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
	EDIT_CaretHide(hwnd);    

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
    if (IsVScrollBar(hwnd))
    {
	vscrollpos = EDIT_ComputeVScrollPos(hwnd);
	SetScrollPos(hwnd, SB_VERT, vscrollpos, TRUE);
    }
}

/*********************************************************************
 *  EDIT_End
 *
 *  End key: move to end of line.
 */
static void EDIT_End(HWND hwnd)
{
    EDITSTATE *es = EDIT_GetEditState(hwnd);

    while (*CurrChar && *CurrChar != '\r')
    {
	es->WndCol += EDIT_CharWidth(hwnd, (BYTE)(*CurrChar), es->WndCol + es->wleft);
	es->CurrCol++;
    }

    if (es->WndCol >= es->ClientWidth)
    {
	es->wleft = es->WndCol - es->ClientWidth + HSCROLLDIM(es);
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
static void EDIT_Home(HWND hwnd)
{
    EDITSTATE *es = EDIT_GetEditState(hwnd);

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
static void EDIT_StickEnd(HWND hwnd)
{
    EDITSTATE *es = EDIT_GetEditState(hwnd);
    int len = EDIT_LineLength(hwnd, es->CurrLine);
    char *cp = EDIT_TextLine(hwnd, es->CurrLine);
    int currpel;

    es->CurrCol = MIN(len, es->CurrCol);
    es->WndCol = MIN(EDIT_StrWidth(hwnd, cp, len, 0) - es->wleft, es->WndCol);
    currpel = EDIT_StrWidth(hwnd, cp, es->CurrCol, 0);

    if (es->wleft > currpel)
    {
	es->wleft = MAX(0, currpel - 20);
	es->WndCol = currpel - es->wleft;
	UpdateWindow(hwnd);
    }
    else if (currpel - es->wleft >= es->ClientWidth)
    {
	es->wleft = currpel - (es->ClientWidth - 5);
	es->WndCol = currpel - es->wleft;
	UpdateWindow(hwnd);
    }
}

/*********************************************************************
 *  EDIT_Downward
 *
 *  Cursor down key: move down one line.
 */
static void EDIT_Downward(HWND hwnd)
{
    EDITSTATE *es = EDIT_GetEditState(hwnd);

    dprintf_edit(stddeb,"EDIT_Downward: WndRow=%d, wtop=%d, wlines=%d\n", 
	     es->WndRow, es->wtop, es->wlines);

    if (IsMultiLine(hwnd) && (es->WndRow + es->wtop + 1 < es->wlines))
    {
	es->CurrLine++;
	if (es->WndRow == es->ClientHeight - 1)
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
static void EDIT_Upward(HWND hwnd)
{
    EDITSTATE *es = EDIT_GetEditState(hwnd);

    if (IsMultiLine(hwnd) && es->CurrLine != 0)
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
 *  EDIT_Forward
 *
 *  Cursor right key: move right one character position.
 */
static void EDIT_Forward(HWND hwnd)
{
    EDITSTATE *es = EDIT_GetEditState(hwnd);

    if (*CurrChar == '\0')
	return;

    if (*CurrChar == '\r')
    {
        if (es->CurrLine < (es->wlines - 1))
        {
	    EDIT_Home(hwnd);
	    EDIT_Downward(hwnd);
        }
    }
    else
    {
	es->WndCol += EDIT_CharWidth(hwnd, (BYTE)(*CurrChar), es->WndCol + es->wleft);
	es->CurrCol++;
	if (es->WndCol >= es->ClientWidth)
	    EDIT_KeyHScroll(hwnd, SB_LINEDOWN);
    }
}

/*********************************************************************
 *  EDIT_Backward
 *
 *  Cursor left key: move left one character position.
 */
static void EDIT_Backward(HWND hwnd)
{
    EDITSTATE *es = EDIT_GetEditState(hwnd);

    if (es->CurrCol)
    {
	--es->CurrCol;
	if (*CurrChar == VK_TAB)
	    es->WndCol -= EDIT_CharWidth(hwnd, (BYTE)(*CurrChar), 
					 EDIT_StrWidth(hwnd, 
					 EDIT_TextLine(hwnd, es->CurrLine), 
					 es->CurrCol, 0));
	else
	    es->WndCol -= EDIT_CharWidth(hwnd, (BYTE)(*CurrChar), 0);
	if (es->WndCol < 0)
	    EDIT_KeyHScroll(hwnd, SB_LINEUP);
    }
    else if (IsMultiLine(hwnd) && es->CurrLine != 0)
    {
	EDIT_Upward(hwnd);
	EDIT_End(hwnd);
    }
}

/*********************************************************************
 *  EDIT_KeyVScrollPage
 *
 *  Scroll text vertically by one page using keyboard.
 */
static void EDIT_KeyVScrollPage(HWND hwnd, WORD opt)
{
    int vscrollpos;
    EDITSTATE *es = EDIT_GetEditState(hwnd);

    if (IsMultiLine(hwnd))
    {
	if (opt == SB_PAGEUP)
	{
	    if (es->wtop > es->ClientHeight) es->wtop -= es->ClientHeight;
	}
	else
	{
	    if (es->wtop + es->ClientHeight < es->wlines)
	    {
		es->wtop += es->ClientHeight;
		if (es->wtop > es->wlines - es->ClientHeight)
		    es->wtop = es->wlines - es->ClientHeight;
	    }
	}
	if (es->wtop < 0)
	    es->wtop = 0;

	es->CurrLine = es->wtop + es->WndRow;
	EDIT_StickEnd(hwnd);
	InvalidateRect(hwnd, NULL, TRUE);
	UpdateWindow(hwnd);

	/* reset the vertical scroll bar */
	if (IsVScrollBar(hwnd))
	{
	    vscrollpos = EDIT_ComputeVScrollPos(hwnd);
	    SetScrollPos(hwnd, SB_VERT, vscrollpos, TRUE);
	}
    }
}

#ifdef SUPERFLUOUS_FUNCTIONS
/*********************************************************************
 *  EDIT_KeyVScrollDoc
 *
 *  Scroll text to top and bottom of document using keyboard.
 */
static void EDIT_KeyVScrollDoc(HWND hwnd, WORD opt)
{
    int vscrollpos;
    EDITSTATE *es = EDIT_GetEditState(hwnd);

    if (!IsMultiLine(hwnd))
	return;

    if (opt == SB_TOP)
	es->wtop = es->wleft = 0;
    else if (es->wtop + es->ClientHeight < es->wlines)
    {
	es->wtop = es->wlines - es->ClientHeight;
	es->wleft = 0;
    }

    es->CurrLine = es->wlines;
    es->WndRow = es->wlines - es->wtop;
    EDIT_End(hwnd);
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);

    /* reset the vertical scroll bar */
    if (IsVScrollBar(hwnd))
    {
	vscrollpos = EDIT_ComputeVScrollPos(hwnd);
	SetScrollPos(hwnd, SB_VERT, vscrollpos, TRUE);
    }
}
#endif

/*********************************************************************
 *  EDIT_DelKey
 *
 *  Delete character to right of cursor.
 */
static void EDIT_DelKey(HWND hwnd)
{
    RECT rc;
    EDITSTATE *es = EDIT_GetEditState(hwnd);
    char *currchar = CurrChar;
    BOOL repaint;

    if (IsMultiLine(hwnd) && !strncmp(currchar,"\r\n\0",3))
	return;

    if(*currchar == '\n') {
        repaint = TRUE;
        strcpy(currchar, currchar + 1);
    } else if (*currchar == '\r') {
        repaint = TRUE;
        strcpy(currchar, currchar + 2);
    } else {
        repaint = FALSE;
        strcpy(currchar, currchar + 1);
    }
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
 *  EDIT_VScrollLine
 *
 *  Scroll text vertically by one line using scrollbars.
 */
static void EDIT_VScrollLine(HWND hwnd, WORD opt)
{
    RECT rc;
    int y;
    EDITSTATE *es = EDIT_GetEditState(hwnd);

    dprintf_edit(stddeb,"EDIT_VScrollLine: direction=%d\n", opt);

    if (opt == SB_LINEDOWN)
    {
	/* move down one line */
	if (es->wtop + es->ClientHeight >= es->wlines)
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
	EDIT_CaretHide(hwnd);

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
static void EDIT_VScrollPage(HWND hwnd, WORD opt)
{
    int vscrollpos;
    EDITSTATE *es = EDIT_GetEditState(hwnd);

    if (opt == SB_PAGEUP)
    {
	if (es->wtop)
	    es->wtop -= es->ClientHeight;
    }
    else
    {
	if (es->wtop + es->ClientHeight < es->wlines)
	{
	    es->wtop += es->ClientHeight;
	    if (es->wtop > es->wlines - es->ClientHeight)
		es->wtop = es->wlines - es->ClientHeight;
	}
    }
    if (es->wtop < 0)
	es->wtop = 0;
    
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);

    /* reset the vertical scroll bar */
    if (IsVScrollBar(hwnd))
    {
	vscrollpos = EDIT_ComputeVScrollPos(hwnd);
	SetScrollPos(hwnd, SB_VERT, vscrollpos, TRUE);
    }
}

/*********************************************************************
 *  EDIT_PixelToChar
 *
 *  Convert a pixel offset in the given row to a character offset,
 *  adjusting the pixel offset to the nearest whole character if
 *  necessary.
 */
static int EDIT_PixelToChar(HWND hwnd, int row, int *pixel)
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
 *  EDIT_ClearText
 *
 *  Clear text from text buffer.
 */
static void EDIT_ClearText(HWND hwnd)
{
    EDITSTATE *es = EDIT_GetEditState(hwnd);
    unsigned int blen = EditBufStartLen(hwnd) + 2;
    char *text;

    dprintf_edit(stddeb,"EDIT_ClearText %d\n",blen);
    es->hText = EDIT_HeapReAlloc(hwnd, es->hText, blen);
    text = EDIT_HeapLock(hwnd, es->hText);
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
 *  EDIT_GetLineCol
 *
 *  Return line and column in text buffer from character offset.
 */
static void EDIT_GetLineCol(HWND hwnd, int off, int *line, int *col)
{
    int lineno;
    char *cp, *cp1;
    EDITSTATE *es = EDIT_GetEditState(hwnd);
    char *text = EDIT_HeapLock(hwnd, es->hText);

    /* check for (0,0) */
    if (!off || !es->wlines)
    {
	*line = 0;
	*col = 0;
	return;
    }

    if (off < 0 || off > strlen(text)) off = strlen(text);
    cp1 = text;
    for (lineno = 0; lineno < es->wlines; lineno++)
    {
	cp = text + es->textptrs[lineno];
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
 *  EDIT_UpdateSel
 *
 *  Redraw the current selection, after losing focus, for example.
 */
static void EDIT_UpdateSel(HWND hwnd)
{
    EDITSTATE *es = EDIT_GetEditState(hwnd);
    int y, bbl, bel;
    RECT rc;
    
    /* Note which lines need redrawing. */
    bbl=MIN(es->SelBegLine,es->SelEndLine);
    bel=MAX(es->SelBegLine,es->SelEndLine);

    /* Redraw the affected lines */
    GetClientRect(hwnd, &rc);
    for (y = bbl; y <= bel; y++)
    {
        EDIT_WriteTextLine(hwnd, &rc, y);
    }
}

/*********************************************************************
 *  EDIT_ClearSel
 *
 *  Clear the current selection.
 */
static void EDIT_ClearSel(HWND hwnd)
{
    EDITSTATE *es = EDIT_GetEditState(hwnd);
    int y, bbl, bel;
    RECT rc;
    
    /* Note which lines need redrawing. */
    bbl=MIN(es->SelBegLine,es->SelEndLine);
    bel=MAX(es->SelBegLine,es->SelEndLine);

    /* Clear the selection */
    es->SelBegLine = es->SelBegCol = 0;
    es->SelEndLine = es->SelEndCol = 0;

    /* Redraw the affected lines */
    GetClientRect(hwnd, &rc);
    for (y = bbl; y <= bel; y++)
    {
        EDIT_WriteTextLine(hwnd, &rc, y);
    }
}

/*********************************************************************
 *  EDIT_SaveDeletedText
 *
 *  Save deleted text in deleted text buffer.
 */
static void EDIT_SaveDeletedText(HWND hwnd, char *deltext, int len,
				 int line, int col)
{
    char *text;
    EDITSTATE *es = EDIT_GetEditState(hwnd);

    dprintf_edit( stddeb, "EDIT_SaveDeletedText\n" );
    if (!es->hDeletedText)
        es->hDeletedText = GlobalAlloc( GMEM_MOVEABLE, len );
    else 
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
 *  EDIT_DeleteSel
 *
 *  Delete the current selected text (if any)
 */
static void EDIT_DeleteSel(HWND hwnd)
{
    char *selStart, *selEnd;
    int bl, el, bc, ec;
    int len;
    EDITSTATE *es = EDIT_GetEditState(hwnd);

    if (SelMarked(es))
    {
        /* Get the real beginning and ending lines and columns */
        bl = es->SelBegLine;
        el = es->SelEndLine;
        if ( bl > el ) {
            bl = el;
            el = es->SelBegLine;
            bc = es->SelEndCol;
            ec = es->SelBegCol;
        } else if ( bl == el ) {
            bc = MIN(es->SelBegCol,es->SelEndCol);
            ec = MAX(es->SelBegCol,es->SelEndCol);
        } else {
            bc = es->SelBegCol;
            ec = es->SelEndCol;
        }
            
	selStart = EDIT_TextLine(hwnd, bl) + bc;
	selEnd = EDIT_TextLine(hwnd, el) + ec;
	len = (int)(selEnd - selStart);
	EDIT_SaveDeletedText(hwnd, selStart, len, bl, bc);
	es->TextChanged = TRUE;
        EDIT_ClearSel(hwnd);
	strcpy(selStart, selEnd);

	es->CurrLine = bl;
	es->CurrCol = bc;
	es->WndRow = bl - es->wtop;
	if (es->WndRow < 0)
	{
	    es->wtop = bl;
	    es->WndRow = 0;
	}
	es->WndCol = EDIT_StrWidth(hwnd, selStart - bc, bc, 0) - es->wleft;

	EDIT_BuildTextPointers(hwnd);
	es->PaintBkgd = TRUE;

        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
    }
}

#ifdef SUPERFLUOUS_FUNCTIONS
/*********************************************************************
 *  EDIT_TextLineNumber
 *
 *  Return the line number in the text buffer of the supplied
 *  character pointer.
 */
static int EDIT_TextLineNumber(HWND hwnd, char *lp)
{
    int lineno;
    char *cp;
    EDITSTATE *es = EDIT_GetEditState(hwnd);
    char *text = EDIT_HeapLock(hwnd, es->hText);

    for (lineno = 0; lineno < es->wlines; lineno++)
    {
	cp = text + es->textptrs[lineno];
	if (cp == lp)
	    return lineno;
	if (cp > lp)
	    break;
    }
    return lineno - 1;
}
#endif

/*********************************************************************
 *  EDIT_SetAnchor
 *
 *  Set down anchor for text marking.
 */
static void EDIT_SetAnchor(HWND hwnd, int row, int col)
{
    EDITSTATE *es = EDIT_GetEditState(hwnd);

    if (SelMarked(es)) EDIT_ClearSel(hwnd);
    es->SelBegLine = es->SelEndLine = row;
    es->SelBegCol = es->SelEndCol = col;
}

/*********************************************************************
 *  EDIT_ExtendSel
 *
 *  Extend selection to the given screen co-ordinates.
 */
static void EDIT_ExtendSel(HWND hwnd, INT x, INT y)
{
    int bbl, bel, bbc;
    char *cp;
    int len, line;
    EDITSTATE *es = EDIT_GetEditState(hwnd);
    RECT rc;

    dprintf_edit(stddeb,"EDIT_ExtendSel: x=%d, y=%d\n", x, y);

    bbl = es->SelEndLine;
    bbc = es->SelEndCol;
    y = MAX(y,0);
    if (IsMultiLine(hwnd))
    {
        if ((line = es->wtop + y / es->txtht) >= es->wlines)
	    line = es->wlines - 1;
    }
    else
        line = 0;
        
    cp = EDIT_TextLine(hwnd, line);
    len = EDIT_LineLength(hwnd, line);

    es->WndRow = y / es->txtht;
    if (!IsMultiLine(hwnd))
	    es->WndRow = 0;
    else if (es->WndRow > es->wlines - es->wtop - 1)
	    es->WndRow = es->wlines - es->wtop - 1;
    es->CurrLine = es->wtop + es->WndRow;
    es->SelEndLine = es->CurrLine;

    es->WndCol = es->wleft + MAX(x,0);
    if (es->WndCol > EDIT_StrWidth(hwnd, cp, len, 0))
	es->WndCol = EDIT_StrWidth(hwnd, cp, len, 0);
    es->CurrCol = EDIT_PixelToChar(hwnd, es->CurrLine, &(es->WndCol));
    es->WndCol -= es->wleft;
    es->SelEndCol = es->CurrCol;

    bel = es->SelEndLine;

    /* return if no new characters to mark */
    if (bbl == bel && bbc == es->SelEndCol) return;

    /* put lowest marker first */
    if (bbl > bel) SWAP_INT(bbl, bel);

    /* Update lines on which selection has changed */
    GetClientRect(hwnd, &rc);
    for (y = bbl; y <= bel; y++)
    {
        EDIT_WriteTextLine(hwnd, &rc, y);
    }
}

/*********************************************************************
 *  EDIT_InsertText
 *
 *  Insert text at current line and column.
 */
static void EDIT_InsertText(HWND hwnd, char *str, int len)
{
    int plen;
    EDITSTATE *es = EDIT_GetEditState(hwnd);
    char *p, *text = EDIT_HeapLock(hwnd, es->hText);
    
    plen = strlen(text) + len;
    if (plen + 1 > es->textlen)
    {
      dprintf_edit(stddeb,"InsertText: Realloc\n");
      es->hText = EDIT_HeapReAlloc(hwnd, es->hText, es->textlen + len);
      text = EDIT_HeapLock(hwnd, es->hText);
      es->textlen = plen + 1;
    }
    for (p = CurrChar + strlen(CurrChar); p >= CurrChar; p--) p[len] = *p;
    memcpy(CurrChar, str, len);

    EDIT_BuildTextPointers(hwnd);
    es->PaintBkgd = TRUE;
    es->TextChanged = TRUE;

    EDIT_GetLineCol(hwnd, (int)((CurrChar + len) - text), &(es->CurrLine),
		                                    &(es->CurrCol));
    es->WndRow = es->CurrLine - es->wtop;
    es->WndCol = EDIT_StrWidth(hwnd, EDIT_TextLine(hwnd, es->CurrLine),
				 es->CurrCol, 0) - es->wleft;
}

/*********************************************************************
 *  EDIT_ClearDeletedText
 *
 *  Clear deleted text buffer.
 */
static void EDIT_ClearDeletedText(HWND hwnd)
{
    EDITSTATE *es = EDIT_GetEditState(hwnd);

    GlobalFree(es->hDeletedText);
    es->hDeletedText = 0;
    es->DeletedLength = 0;
}

/*********************************************************************
 *  EDIT_CopyToClipboard
 *
 *  Copy the specified text to the clipboard.
 */
static void EDIT_CopyToClipboard(HWND hwnd)
{
    HANDLE hMem;
    char *lpMem;
    int i, len;
    char *bbl, *bel;
    EDITSTATE *es = EDIT_GetEditState(hwnd);

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
 *  EDIT_KeyTyped
 *
 *  Process keystrokes that produce displayable characters.
 */
static void EDIT_KeyTyped(HWND hwnd, short ch)
{
    EDITSTATE *es = EDIT_GetEditState(hwnd);
    char *text = EDIT_HeapLock(hwnd, es->hText);
    char *currchar, *p;
    RECT rc;
    BOOL FullPaint = FALSE;

    dprintf_edit(stddeb,"EDIT_KeyTyped: ch=%c\n", (char)ch);

    /* delete selected text (if any) */
    if (SelMarked(es))
	EDIT_DeleteSel(hwnd);

    /* currchar must be assigned after deleting the selection */
    currchar = CurrChar;

    /* test for typing at end of maximum buffer size */
    if (currchar == text + es->MaxTextLen)
    {
	NOTIFY_PARENT(hwnd, EN_ERRSPACE);
	return;
    }

    if (*currchar == '\0' && IsMultiLine(hwnd))
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
	dprintf_edit( stddeb, "EDIT_KeyTyped: realloc\n" );
	es->hText = EDIT_HeapReAlloc(hwnd, es->hText, es->textlen + 2);
	if (!es->hText)
	    NOTIFY_PARENT(hwnd, EN_ERRSPACE);
	text = EDIT_HeapLock(hwnd, es->hText);
	text[es->textlen - 1] = '\0';
	currchar = CurrChar;
    }
    /* make space for new character and put char in buffer */
    if (ch == '\n')
    {
        for (p = currchar + strlen(currchar); p >= currchar; p--) p[2] = p[0];
	*currchar = '\r';
	*(currchar + 1) = '\n';
	EDIT_ModTextPointers(hwnd, es->CurrLine + 1, 2);
    }
    else
    {
        for (p = currchar + strlen(currchar); p >= currchar; p--) p[1] = p[0];
	*currchar = ch;
	EDIT_ModTextPointers(hwnd, es->CurrLine + 1, 1);
    }
    es->TextChanged = TRUE;
    NOTIFY_PARENT(hwnd, EN_UPDATE);

    /* re-adjust textwidth, if necessary, and redraw line */
    if (IsMultiLine(hwnd) && es->wlines > 1)
    {
	es->textwidth = MAX(es->textwidth,
		    EDIT_StrWidth(hwnd, EDIT_TextLine(hwnd, es->CurrLine),
		    (int)(EDIT_TextLine(hwnd, es->CurrLine + 1) -
			  EDIT_TextLine(hwnd, es->CurrLine)), 0));
    } else {
      es->textwidth = MAX(es->textwidth,
			  EDIT_StrWidth(hwnd, text, strlen(text), 0));
    }

    if (ch == '\n')
    {
	if (es->wleft > 0)
	    FullPaint = TRUE;
	es->wleft = 0;
	EDIT_BuildTextPointers(hwnd);
	EDIT_End(hwnd);
	EDIT_Forward(hwnd);
        EDIT_SetAnchor(hwnd, es->CurrLine, es->CurrCol);

	/* invalidate rest of window */
	GetClientRect(hwnd, &rc);
	if (!FullPaint)
	    rc.top = es->WndRow * es->txtht;
	InvalidateRect(hwnd, &rc, FALSE);

	UpdateWindow(hwnd);
	NOTIFY_PARENT(hwnd, EN_CHANGE);
	return;
    }

    /* test end of window */
    if (es->WndCol >= es->ClientWidth - 
	                    EDIT_CharWidth(hwnd, (BYTE)ch, es->WndCol + es->wleft))
    {
	/* TODO:- Word wrap to be handled here */

/*	if (!(currchar == text + es->MaxTextLen - 2)) */
	    EDIT_KeyHScroll(hwnd, SB_LINEDOWN);
    }
    es->WndCol += EDIT_CharWidth(hwnd, (BYTE)ch, es->WndCol + es->wleft);
    es->CurrCol++;
    EDIT_SetAnchor(hwnd, es->CurrLine, es->CurrCol);
    EDIT_WriteTextLine(hwnd, NULL, es->wtop + es->WndRow);
    NOTIFY_PARENT(hwnd, EN_CHANGE);
    dprintf_edit(stddeb,"KeyTyped O.K.\n");
}

/*********************************************************************
 *  EM_UNDO message function
 */
static LRESULT EDIT_UndoMsg(HWND hwnd)
{
    char *text;
    EDITSTATE *es = EDIT_GetEditState(hwnd);
    
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
	es->WndCol = EDIT_StrWidth(hwnd, EDIT_TextLine(hwnd, es->CurrLine),
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
 *  EM_SETHANDLE message function
 */
static void EDIT_SetHandleMsg(HWND hwnd, WPARAM wParam)
{
    EDITSTATE *es = EDIT_GetEditState(hwnd);

    if (IsMultiLine(hwnd))
    {
	es->hText = (HANDLE)wParam;
	es->textlen = EDIT_HeapSize(hwnd, es->hText);
	es->wlines = 0;
	es->wtop = es->wleft = 0;
	es->CurrLine = es->CurrCol = 0;
	es->WndRow = es->WndCol = 0;
	es->TextChanged = FALSE;
	es->textwidth = 0;
	es->SelBegLine = es->SelBegCol = 0;
	es->SelEndLine = es->SelEndCol = 0;
	dprintf_edit(stddeb, "EDIT_SetHandleMsg: handle %04lx, textlen=%d\n",
		     (DWORD)wParam, es->textlen);

	EDIT_BuildTextPointers(hwnd);
	es->PaintBkgd = TRUE;
	InvalidateRect(hwnd, NULL, TRUE);
	UpdateWindow(hwnd);
    }
}

/*********************************************************************
 *  EM_SETTABSTOPS message function
 */
static LRESULT EDIT_SetTabStopsMsg(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    EDITSTATE *es = EDIT_GetEditState(hwnd);

    dprintf_edit( stddeb, "EDIT_SetTabStops\n" );
    es->NumTabStops = wParam;
    if (wParam == 0)
	es->TabStops = xrealloc(es->TabStops, 2);
    else if (wParam == 1)
    {
	es->TabStops = xrealloc(es->TabStops, 2);
	es->TabStops[0] = LOWORD(lParam);
    }
    else
    {
	es->TabStops = xrealloc(es->TabStops, wParam * sizeof(*es->TabStops));
	memcpy(es->TabStops, (unsigned short *)PTR_SEG_TO_LIN(lParam), wParam);
    }
    return 0;
}

/*********************************************************************
 *  EM_GETLINE message function
 */
static LRESULT EDIT_GetLineMsg(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    char *cp;
    int len = 0;
    unsigned char *buffer = (char *)PTR_SEG_TO_LIN(lParam);

       /* the line wanted */
    cp  = EDIT_TextLine  (hwnd, wParam);
    len = EDIT_LineLength(hwnd, wParam);
    
       /* if cp==NULL nothing will be copied - I hope */
    if ((char *) NULL == cp && 0 != len) {
       fprintf(stdnimp,"edit: EDIT_GetLineMsg cp == NULL && len != 0"); 
       return 0L;
    }

    if (0>len)
       fprintf(stdnimp,"edit: EDIT_GetLineMsg len < 0"); 

       /* suggested reason for the following line:
          never copy more than the buffer's size ? 
          I thought that this would make sense only if
          the lstrcpyn fun was used instead of the gnu strncpy.
       */
    len = MIN(len, (WORD)(*buffer));

    if (0>len)
       fprintf(stdnimp,"edit: EDIT_GetLineMsg len < 0 after MIN"); 

    dprintf_edit( stddeb, "EDIT_GetLineMsg: %d %d, len %d\n", (int)(WORD)(*buffer), (int)(WORD)(*(char *)buffer), len);
    lstrcpyn(buffer, cp, len);

    return (LRESULT)len;
}

/*********************************************************************
 *  EM_GETSEL message function
 */
static LRESULT EDIT_GetSelMsg(HWND hwnd)
{
    int so, eo;
    EDITSTATE *es = EDIT_GetEditState(hwnd);

    so = es->textptrs[es->SelBegLine] + es->SelBegCol;
    eo = es->textptrs[es->SelEndLine] + es->SelEndCol;

    return (LRESULT)MAKELONG(so, eo);
}

/*********************************************************************
 *  EM_REPLACESEL message function
 */
static void EDIT_ReplaceSel(HWND hwnd, LPARAM lParam)
{
    EDIT_DeleteSel(hwnd);
    EDIT_InsertText(hwnd, (char *)PTR_SEG_TO_LIN(lParam),
                    strlen((char *)PTR_SEG_TO_LIN(lParam)));
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
}

/*********************************************************************
 *  EM_LINEFROMCHAR message function
 */
static LRESULT EDIT_LineFromCharMsg(HWND hwnd, WPARAM wParam)
{
    int row, col;
    EDITSTATE *es = EDIT_GetEditState(hwnd);

    if (wParam == (WORD)-1)
	return (LRESULT)(es->SelBegLine);
    else
	EDIT_GetLineCol(hwnd, wParam, &row, &col);

    return (LRESULT)row;
}


/*********************************************************************
 *  EM_LINEINDEX message function
 */
static LRESULT EDIT_LineIndexMsg(HWND hwnd, WPARAM wParam)
{
    EDITSTATE *es = EDIT_GetEditState(hwnd);

    if (wParam == (WORD)-1) wParam = es->CurrLine;
    return es->textptrs[wParam];
}


/*********************************************************************
 *  EM_LINELENGTH message function
 */
static LRESULT EDIT_LineLengthMsg(HWND hwnd, WPARAM wParam)
{
    int row, col, len;
    int sbl, sbc, sel, sec;
    EDITSTATE *es = EDIT_GetEditState(hwnd);

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
		SWAP_INT(sbl, sel);
		SWAP_INT(sbc, sec);
	    }
	    if (sbl == sel && sbc > sec)
		SWAP_INT(sbc, sec);

	    if (sbc == sel)
	    {
		len = es->textptrs[sbl + 1] - es->textptrs[sbl] - 1;
		return len - sec - sbc;
	    }

	    len = es->textptrs[sel + 1] - es->textptrs[sel] - sec - 1;
	    return len + sbc;
	}
	else    /* no selection marked */
	{
	    len = es->textptrs[es->CurrLine + 1] - es->textptrs[es->CurrLine] - 1;
	    return len;
	}
    }
    else    /* line number specified */
    {
	EDIT_GetLineCol(hwnd, wParam, &row, &col);
	len = es->textptrs[row + 1] - es->textptrs[row];
	return len;
    }
}

/*********************************************************************
 *  EM_SETSEL message function
 */
static void EDIT_SetSelMsg(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    INT so, eo;
    EDITSTATE *es = EDIT_GetEditState(hwnd);

    so = LOWORD(lParam);
    eo = HIWORD(lParam);

    if (so == -1)       /* if so == -1, clear selection */
    {
	EDIT_ClearSel(hwnd);
	return;
    }

    if (so == eo)       /* if so == eo, set caret only */
    {
	EDIT_GetLineCol(hwnd, (int) so, &(es->CurrLine), &(es->CurrCol));
	es->WndRow = es->CurrLine - es->wtop;

	if (!wParam)
	{
	    if (es->WndRow < 0 || es->WndRow > es->ClientHeight)
	    {
		es->wtop = es->CurrLine;
		es->WndRow = 0;
	    }
	    es->WndCol = EDIT_StrWidth(hwnd, 
					EDIT_TextLine(hwnd, es->CurrLine), 
					es->CurrCol, 0) - es->wleft;
	    if (es->WndCol > es->ClientWidth)
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
	if (eo >= 0 && so > eo)	  /* eo == -1 flag to extend to end of text */
        {
            INT tmp;
            tmp = so;
            so = eo;
            eo = tmp;
        }

	EDIT_GetLineCol(hwnd, (int) so, &(es->SelBegLine), &(es->SelBegCol));
	EDIT_GetLineCol(hwnd, (int) eo, &(es->SelEndLine), &(es->SelEndCol));
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
	    else if (es->WndRow > es->ClientHeight)
	    {
		es->wtop += es->WndRow - es->ClientHeight;
		es->WndRow = es->ClientHeight;
	    }
	    es->WndCol = EDIT_StrWidth(hwnd, 
					EDIT_TextLine(hwnd, es->SelEndLine), 
					es->SelEndCol, 0) - es->wleft;
	    if (es->WndCol > es->ClientWidth)
	    {
		es->wleft += es->WndCol - es->ClientWidth;
		es->WndCol = es->ClientWidth;
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
 *  WM_SETFONT
 */
static void EDIT_WM_SetFont(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    TEXTMETRIC tm;
    HFONT oldfont;
    EDITSTATE *es = EDIT_GetEditState(hwnd);

    es->hFont = (HANDLE)wParam;
    hdc = GetDC(hwnd);
    oldfont = (HFONT)SelectObject(hdc, (HANDLE)es->hFont);
    GetCharWidth(hdc, 0, 255, es->CharWidths);
    GetTextMetrics(hdc, &tm);
    es->txtht = tm.tmHeight + tm.tmExternalLeading;
    SelectObject(hdc, (HANDLE)oldfont);
    ReleaseDC(hwnd, hdc);

    es->WndRow = (es->CurrLine - es->wtop) / es->txtht;
    es->WndCol = EDIT_StrWidth(hwnd, EDIT_TextLine(hwnd, es->CurrLine),
				 es->CurrCol, 0) - es->wleft;

    InvalidateRect(hwnd, NULL, TRUE);
    es->PaintBkgd = TRUE;
    if (lParam) UpdateWindow(hwnd);
    EDIT_RecalcSize(hwnd,es);

    if (es->HaveFocus)
    {
	EDIT_CaretHide(hwnd);
	DestroyCaret();
	CreateCaret(hwnd, 0, 2, es->txtht);
	SetCaretPos(es->WndCol, es->WndRow * es->txtht);
    }
}

/*********************************************************************
 *  WM_PASTE
 */
static void EDIT_WM_Paste(HWND hwnd)
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
 *  WM_PAINT
 */
static void EDIT_WM_Paint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc;
    int y;
    RECT rc;
    EDITSTATE *es = EDIT_GetEditState(hwnd);

    EDIT_CaretHide(hwnd);

    hdc = BeginPaint(hwnd, &ps);
    GetClientRect(hwnd, &rc);
    IntersectClipRect(hdc, rc.left, rc.top, rc.right, rc.bottom);

    dprintf_edit(stddeb,"WM_PAINT: rc=(%ld,%ld), (%ld,%ld)\n", (LONG)rc.left, 
           (LONG)rc.top, (LONG)rc.right, (LONG)rc.bottom);

    if (es->PaintBkgd)
	FillWindow(GetParent(hwnd), hwnd, hdc, (HBRUSH)CTLCOLOR_EDIT);

    for (y = (rc.top / es->txtht); y <= (rc.bottom / es->txtht); y++)
    {
	if (y < (IsMultiLine(hwnd) ? es->wlines : 1) - es->wtop)
	    EDIT_WriteTextLine(hwnd, &rc, y + es->wtop);
    }

    EndPaint(hwnd, &ps);
}

static BOOL LOCAL_HeapExists(HANDLE ds)
{
/* There is always a local heap in WineLib */
#ifndef WINELIB
    INSTANCEDATA *ptr = (INSTANCEDATA *)PTR_SEG_OFF_TO_LIN( ds, 0 );
    if (!ptr->heap) return 0;
#endif
    return 1;
}

/*********************************************************************
 *  WM_NCCREATE
 */
static long EDIT_WM_NCCreate(HWND hwnd, LPARAM lParam)
{
    CREATESTRUCT *createStruct = (CREATESTRUCT *)PTR_SEG_TO_LIN(lParam);
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    EDITSTATE *es;
    char *text = NULL;
    HANDLE ds;

    /* store pointer to local or global heap in window structure so that */
    /* EDITSTATE structure itself can be stored on local heap  */
    /* allocate space for state variable structure */
    es = xmalloc( sizeof(EDITSTATE) );
    SetWindowLong( hwnd, 0, (LONG)es );
    es->textptrs = xmalloc(sizeof(int));
    es->CharWidths = xmalloc(256 * sizeof(INT));
    es->ClientWidth = es->ClientHeight = 1;
    /* --- text buffer */
    es->MaxTextLen = MAXTEXTLEN + 1;
    es->PasswordChar = '*';
    
    es->WordBreakProc = NULL;
    
    /* Caret stuff */
    es->CaretPrepareCount = 1;
    es->CaretHidden = FALSE;
    es->HaveFocus = FALSE;
    /*
     * Hack - If there is no local heap then hwnd should be a globalHeap block
     * and the local heap needs to be initilised to the same size(minus something)
     * as the global block
     */
    ds = WIN_GetWindowInstance(hwnd);
    
    if (!LOCAL_HeapExists(ds))
    {
        DWORD globalSize;
        globalSize = GlobalSize(ds);
        dprintf_edit(stddeb, "No local heap allocated global size is %ld 0x%lx\n",globalSize, globalSize);
        /*
         * I assume the local heap should start at 0 
         */
        LocalInit(ds, 0, globalSize);
        /*
         * Apparantly we should do an UnlockSegment here but i think this
         * is because LocalInit is supposed to do a LockSegment. Since
         * Local Init doesn't do this then it doesn't seem like a good idea to do the
         * UnlockSegment here yet!
         * UnlockSegment(hwnd);
         */
        
    }
    

    if (!(createStruct->lpszName))
    {
	dprintf_edit( stddeb, "EDIT_WM_NCCREATE: lpszName == 0\n" );
	es->textlen = EditBufStartLen(hwnd) + 1;
	es->hText = EDIT_HeapAlloc(hwnd, es->textlen + 2, LMEM_MOVEABLE);
	text = EDIT_HeapLock(hwnd, es->hText);
	memset(text, 0, es->textlen + 2);
        es->wlines = 0;
        es->textwidth = 0;
	EDIT_ClearTextPointers(hwnd);
	if (IsMultiLine(hwnd)) strcpy(text, "\r\n");
	EDIT_BuildTextPointers(hwnd);
    }
    else
    {
        char *windowName = (char *)PTR_SEG_TO_LIN( createStruct->lpszName );
	dprintf_edit( stddeb, "EDIT_WM_NCCREATE: lpszName != 0\n" );
        
	if (strlen(windowName) < EditBufStartLen(hwnd))
	{
	    es->textlen = EditBufStartLen(hwnd) + 3;
	    es->hText = EDIT_HeapAlloc(hwnd, es->textlen + 2, LMEM_MOVEABLE);
            if (es->hText)
            {
                text = EDIT_HeapLock(hwnd, es->hText);
                if (text)
                {
                    strcpy(text, windowName);
                    if(IsMultiLine(hwnd)) {
                        strcat(text, "\r\n");
                    }
                    *(text + es->textlen) = '\0';
                }
            }
	}
	else
	{
	    es->textlen = strlen(windowName) + 3;
	    es->hText = EDIT_HeapAlloc(hwnd, es->textlen + 2, LMEM_MOVEABLE);
	    text = EDIT_HeapLock(hwnd, es->hText);
	    strcpy(text, windowName);
	    if(IsMultiLine(hwnd)) strcat(text, "\r\n");
	    *(text + es->textlen) = '\0';
	}
        if (text)
        {
            *(text + es->textlen + 1) = '\0';
            EDIT_BuildTextPointers(hwnd);
        }
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
 *  WM_CREATE
 */
static LRESULT EDIT_WM_Create(HWND hwnd, LPARAM lParam)
{
    HDC hdc;
    EDITSTATE *es = EDIT_GetEditState(hwnd);
    CLASS *classPtr;
    TEXTMETRIC tm;

    /* initialize state variable structure */
    hdc = GetDC(hwnd);

    /* --- char width array                                        */
    /*     only initialise chars <= 32 as X returns strange widths */
    /*     for other chars                                         */
    memset(es->CharWidths, 0, 256 * sizeof(INT));
    GetCharWidth(hdc, 32, 254, &es->CharWidths[32]);

    /* --- other structure variables */
    GetTextMetrics(hdc, &tm);
    es->txtht = tm.tmHeight + tm.tmExternalLeading;
    EDIT_RecalcSize(hwnd,es);
    es->wtop = es->wleft = 0;
    es->CurrCol = es->CurrLine = 0;
    es->WndCol = es->WndRow = 0;
    es->TextChanged = FALSE;
    es->SelBegLine = es->SelBegCol = 0;
    es->SelEndLine = es->SelEndCol = 0;
    es->hFont = 0;
    es->hDeletedText = 0;
    es->DeletedLength = 0;
    es->NumTabStops = 0;
    es->TabStops = xmalloc( sizeof(short) );

    /* set up text cursor for edit class */
    {
        char editname[] = "EDIT";
        CLASS_FindClassByName( MAKE_SEGPTR(editname), 0, &classPtr);
        classPtr->wc.hCursor = LoadCursor(0, IDC_IBEAM);
    }

    /* paint background on first WM_PAINT */
    es->PaintBkgd = TRUE;

    ReleaseDC(hwnd, hdc);
    return 0L;
}

/*********************************************************************
 *  WM_VSCROLL
 */
static void EDIT_WM_VScroll(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
/*
 *    EDITSTATE *es = EDIT_GetEditState(hwnd);
 */
    if (IsMultiLine(hwnd))
    {
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
}

/*********************************************************************
 *  WM_HSCROLL
 */
static void EDIT_WM_HScroll(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
/*
 *    EDITSTATE *es = EDIT_GetEditState(hwnd);
 */
    switch (wParam)
    {
    case SB_LINEUP:
    case SB_LINEDOWN:
	break;
    }
}

/*********************************************************************
 *  WM_SIZE
 */
static void EDIT_WM_Size(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    EDITSTATE *es = EDIT_GetEditState(hwnd);
  
    EDIT_RecalcSize(hwnd,es);
    if (wParam != SIZE_MAXIMIZED && wParam != SIZE_RESTORED) return;
    InvalidateRect(hwnd, NULL, TRUE);
    es->PaintBkgd = TRUE;
    UpdateWindow(hwnd);
}

/*********************************************************************
 *  WM_LBUTTONDOWN
 */
static void EDIT_WM_LButtonDown(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    char *cp;
    int len;
    BOOL end = FALSE;
    EDITSTATE *es = EDIT_GetEditState(hwnd);

    ButtonDown = TRUE;

    if ((wParam & MK_SHIFT)) {
        EDIT_ExtendSel(hwnd, LOWORD(lParam), HIWORD(lParam));
        return;
    }

    es->WndRow = HIWORD(lParam) / es->txtht;
    dprintf_edit( stddeb, "EDIT_LButtonDown: %04x %08lx, WndRow %d\n", wParam,
		  lParam, es->WndRow );
    if (!IsMultiLine(hwnd)) es->WndRow = 0;
    else if (es->WndRow > es->wlines - es->wtop - 1)
    {
        es->WndRow = es->wlines - es->wtop - 1;
	end = TRUE;
    }
    es->CurrLine = es->wtop + es->WndRow;

    cp = EDIT_TextLine(hwnd, es->CurrLine);
    len = EDIT_LineLength(hwnd, es->CurrLine);
    es->WndCol = LOWORD(lParam) + es->wleft;
    if (end || es->WndCol > EDIT_StrWidth(hwnd, cp, len, 0))
        es->WndCol = EDIT_StrWidth(hwnd, cp, len, 0);
    dprintf_edit( stddeb, "EDIT_LButtonDown: CurrLine %d wtop %d wndcol %d\n",
		  es->CurrLine, es->wtop, es->WndCol);
    es->CurrCol = EDIT_PixelToChar(hwnd, es->CurrLine, &(es->WndCol));
    es->WndCol -= es->wleft;

    ButtonRow = es->CurrLine;
    ButtonCol = es->CurrCol;
    EDIT_SetAnchor(hwnd, ButtonRow, ButtonCol);
}

/*********************************************************************
 *  WM_MOUSEMOVE
 */
static void EDIT_WM_MouseMove(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
/*
 *    EDITSTATE *es = EDIT_GetEditState(hwnd);
 */
    if (!(wParam & MK_LBUTTON)) return;

    if (ButtonDown)
    {
	TextMarking = TRUE;
	ButtonDown = FALSE;
    }

    if (TextMarking)
	EDIT_ExtendSel(hwnd, LOWORD(lParam), HIWORD(lParam));
}

/*********************************************************************
 *  WM_CHAR
 */
static void EDIT_WM_Char(HWND hwnd, WPARAM wParam)
{
    dprintf_edit(stddeb,"EDIT_WM_Char: wParam=%c\n", (char)wParam);

    switch (wParam)
    {
    case '\r':
    case '\n':
	if (!IsMultiLine(hwnd))
	    break;
	if (IsReadOnly(hwnd))
	{
	    EDIT_Home(hwnd);
	    EDIT_Downward(hwnd);
	    break;
	}
	wParam = '\n';
	EDIT_KeyTyped(hwnd, wParam);
	break;

    case VK_TAB:
	if (IsReadOnly(hwnd)) break;
	if (!IsMultiLine(hwnd))
	    break;
	EDIT_KeyTyped(hwnd, wParam);
	break;

    default:
	if (IsReadOnly(hwnd)) break;
	if (wParam >= 20 && wParam <= 254 && wParam != 127 )
	    EDIT_KeyTyped(hwnd, wParam);
	break;
    }
}

/*********************************************************************
 *  WM_KEYDOWN
 */
static void EDIT_WM_KeyDown(HWND hwnd, WPARAM wParam)
{
    EDITSTATE *es = EDIT_GetEditState(hwnd);
    BOOL motionKey = FALSE;

    dprintf_edit(stddeb,"EDIT_WM_KeyDown: key=%x\n", wParam);

    switch (wParam)
    {
    case VK_UP:
	if (IsMultiLine(hwnd))
	    EDIT_Upward(hwnd);
	else
	    EDIT_Backward(hwnd);
        motionKey = TRUE;
	break;

    case VK_DOWN:
	if (IsMultiLine(hwnd))
	    EDIT_Downward(hwnd);
	else
	    EDIT_Forward(hwnd);
        motionKey = TRUE;
	break;

    case VK_RIGHT:
	EDIT_Forward(hwnd);
        motionKey = TRUE;
	break;

    case VK_LEFT:
	EDIT_Backward(hwnd);
        motionKey = TRUE;
	break;

    case VK_HOME:
	EDIT_Home(hwnd);
        motionKey = TRUE;
	break;

    case VK_END:
	EDIT_End(hwnd);
        motionKey = TRUE;
	break;

    case VK_PRIOR:
	if (IsMultiLine(hwnd))
	{
	    EDIT_KeyVScrollPage(hwnd, SB_PAGEUP);
	}
        motionKey = TRUE;
	break;

    case VK_NEXT:
	if (IsMultiLine(hwnd))
	{
	    EDIT_KeyVScrollPage(hwnd, SB_PAGEDOWN);
	}
        motionKey = TRUE;
	break;

    case VK_BACK:
	if (IsReadOnly(hwnd)) break;
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
	if (IsReadOnly(hwnd)) break;
	if (SelMarked(es))
	    EDIT_DeleteSel(hwnd);
	else
	    EDIT_DelKey(hwnd);
	break;

    case VK_SHIFT:
        motionKey = TRUE;
        break;

    default:
        return;
    }

    /* FIXME: GetKeyState appears to have its bits reversed */
#if CorrectGetKeyState
    if(motionKey && (0x80 & GetKeyState(VK_SHIFT))) {
#else
    if(motionKey && (0x01 & GetKeyState(VK_SHIFT))) {
#endif
        EDIT_ExtendSel(hwnd, es->WndCol, es->WndRow*es->txtht);
    } else {
        EDIT_SetAnchor(hwnd, es->CurrLine, es->CurrCol);
    }
}

/*********************************************************************
 *  WM_SETTEXT
 */
static LRESULT EDIT_WM_SetText(HWND hwnd, LPARAM lParam)
{
    int len;
    char *text,*settext;
    EDITSTATE *es = EDIT_GetEditState(hwnd);

    settext = PTR_SEG_TO_LIN( lParam );
    dprintf_edit( stddeb,"WM_SetText, length %d\n",strlen(settext) );
    if (strlen(settext) <= es->MaxTextLen)
    {
	len = settext != NULL ? strlen(settext) : 0;
	EDIT_ClearText(hwnd);
	es->textlen = len;
	dprintf_edit( stddeb, "EDIT_WM_SetText: realloc\n" );
	es->hText = EDIT_HeapReAlloc(hwnd, es->hText, len + 3);
	text = EDIT_HeapLock(hwnd, es->hText);
	if (lParam)
	    strcpy(text, (char *)PTR_SEG_TO_LIN(lParam));
	text[len]     = '\0';
	text[len + 1] = '\0';
	text[len + 2] = '\0';
	EDIT_BuildTextPointers(hwnd);
	InvalidateRect(hwnd, NULL, TRUE);
	es->PaintBkgd = TRUE;
	es->TextChanged = TRUE;
	return 0;
    }
    else
	return EN_ERRSPACE;
}

/*********************************************************************
 *  WM_LBUTTONDBLCLK
 */
static void EDIT_WM_LButtonDblClk(HWND hwnd)
{
    EDITSTATE *es = EDIT_GetEditState(hwnd);

    dprintf_edit(stddeb, "WM_LBUTTONDBLCLK: hwnd=%d\n", hwnd);
    if (SelMarked(es)) EDIT_ClearSel(hwnd);
    es->SelBegLine = es->SelEndLine = es->CurrLine;
    es->SelBegCol = EDIT_CallWordBreakProc(hwnd,
			EDIT_TextLine(hwnd, es->CurrLine),
			es->CurrCol, 
			EDIT_LineLength(hwnd, es->CurrLine), 
			WB_LEFT);
    es->SelEndCol = EDIT_CallWordBreakProc(hwnd,
			EDIT_TextLine(hwnd, es->CurrLine),
			es->CurrCol, 
			EDIT_LineLength(hwnd, es->CurrLine), 
			WB_RIGHT);
    EDIT_WriteTextLine(hwnd, NULL, es->CurrLine);
}

/*********************************************************************
 * EditWndProc()
 */
LRESULT EditWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = 0;
    char *textPtr;
    int len;
    EDITSTATE *es = EDIT_GetEditState(hwnd);

    EDIT_CaretPrepare(hwnd);

    switch (uMsg) {
    case EM_CANUNDO:
	lResult = (LRESULT)es->hDeletedText;
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
	lResult = (LRESULT)es->hText;
	break;

    case EM_GETLINE:
	if (IsMultiLine(hwnd))
	    lResult = EDIT_GetLineMsg(hwnd, wParam, lParam);
	break;

    case EM_GETLINECOUNT:
	if (IsMultiLine(hwnd))
	    lResult = es->wlines;
	break;

    case EM_GETMODIFY:
	lResult = es->TextChanged;
	break;

    case EM_GETPASSWORDCHAR:
        lResult = es->PasswordChar;
	break;

    case EM_GETRECT:
	GetWindowRect(hwnd, (LPRECT)PTR_SEG_TO_LIN(lParam));
	break;

    case EM_GETSEL:
	lResult = EDIT_GetSelMsg(hwnd);
	break;

    case EM_GETWORDBREAKPROC:
	dprintf_edit(stddeb, "EM_GETWORDBREAKPROC\n");
	lResult = (LRESULT)es->WordBreakProc;
	break;

    case EM_LIMITTEXT:
	if (wParam)
	    es->MaxTextLen = wParam;
	else if (IsMultiLine(hwnd))
	    es->MaxTextLen = 65535;
	else
	    es->MaxTextLen = 32767;
	break;

    case EM_LINEFROMCHAR:
	lResult = EDIT_LineFromCharMsg(hwnd, wParam);
	break;

    case EM_LINEINDEX:
	if (IsMultiLine(hwnd))
	    lResult = EDIT_LineIndexMsg(hwnd, wParam);
	break;

    case EM_LINELENGTH:
	lResult = EDIT_LineLengthMsg(hwnd, wParam);
	break;

    case EM_LINESCROLL:
	fprintf(stdnimp,"edit: cannot process EM_LINESCROLL message\n");
	break;

    case EM_REPLACESEL:
	EDIT_ReplaceSel(hwnd, lParam);
	break;

    case EM_SETHANDLE:
	EDIT_ClearDeletedText(hwnd);
	EDIT_SetHandleMsg(hwnd, wParam);
	break;

    case EM_SETMODIFY:
	es->TextChanged = wParam;
	break;

    case EM_SETPASSWORDCHAR:
        es->PasswordChar = (char) wParam;
	break;

    case EM_SETREADONLY:
	dprintf_edit(stddeb, "EM_SETREADONLY, wParam=%d\n", wParam);
	SetWindowLong(hwnd, GWL_STYLE,
	    (BOOL)wParam ? (GetWindowLong(hwnd, GWL_STYLE) | ES_READONLY)
		: (GetWindowLong(hwnd, GWL_STYLE) & ~(DWORD)ES_READONLY));
	break;

    case EM_SETRECT:
    case EM_SETRECTNP:
	fprintf(stdnimp,"edit: cannot process EM_SETRECT(NP) message\n");
	break;

    case EM_SETSEL:
	EDIT_SetSelMsg(hwnd, wParam, lParam);
	break;

    case EM_SETTABSTOPS:
	lResult = EDIT_SetTabStopsMsg(hwnd, wParam, lParam);
	break;

    case EM_SETWORDBREAKPROC:
	dprintf_edit(stddeb, "EM_SETWORDBREAKPROC, lParam=%08lx\n",
			(DWORD)lParam);
	es->WordBreakProc = (EDITWORDBREAKPROC)lParam;
	break;

    case EM_UNDO:
	lResult = EDIT_UndoMsg(hwnd);
	break;

    case WM_GETDLGCODE:
        lResult = DLGC_HASSETSEL | DLGC_WANTCHARS | DLGC_WANTARROWS;
	break;

    case WM_CHAR:
	EDIT_WM_Char(hwnd, wParam);
	break;

    case WM_COPY:
	EDIT_CopyToClipboard(hwnd);
	EDIT_ClearSel(hwnd);
	break;

    case WM_CREATE:
	lResult = EDIT_WM_Create(hwnd, lParam);
	break;

    case WM_CUT:
	EDIT_CopyToClipboard(hwnd);
	EDIT_DeleteSel(hwnd);
	break;

    case WM_DESTROY:
	free(es->textptrs);
	free(es->CharWidths);
	free(es->TabStops);
	EDIT_HeapFree(hwnd, es->hText);
	free( EDIT_GetEditState(hwnd) );
	break;

    case WM_ENABLE:
	InvalidateRect(hwnd, NULL, FALSE);
	break;

    case WM_GETTEXT:
	textPtr = EDIT_HeapLock(hwnd, es->hText);
	len = strlen( textPtr );
	if ((int)wParam > len)
	{
	    strcpy((char *)PTR_SEG_TO_LIN(lParam), textPtr);
	    lResult = (LRESULT)len ;
	}
	EDIT_HeapUnlock(hwnd, es->hText);
	break;

    case WM_GETTEXTLENGTH:
	textPtr = EDIT_HeapLock(hwnd, es->hText);
	lResult = (LRESULT)strlen(textPtr);
	EDIT_HeapUnlock(hwnd, es->hText);
	break;

    case WM_HSCROLL:
	EDIT_WM_HScroll(hwnd, wParam, lParam);
	break;

    case WM_KEYDOWN:
	EDIT_WM_KeyDown(hwnd, wParam);
	break;

    case WM_KILLFOCUS:
	dprintf_edit(stddeb, "WM_KILLFOCUS\n");
	es->HaveFocus = FALSE;
	DestroyCaret();
	if (SelMarked(es))
            if(GetWindowLong(hwnd,GWL_STYLE) & ES_NOHIDESEL)
	        EDIT_UpdateSel(hwnd);
            else
	        EDIT_ClearSel(hwnd);
	NOTIFY_PARENT(hwnd, EN_KILLFOCUS);
	break;

    case WM_LBUTTONDOWN:
	SetFocus(hwnd);
	SetCapture(hwnd);
	EDIT_WM_LButtonDown(hwnd, wParam, lParam);
	break;

    case WM_LBUTTONUP:
	if (GetCapture() != hwnd) break;
	ReleaseCapture();
	ButtonDown = FALSE;
        TextMarking = FALSE;
	break;

    case WM_MOUSEMOVE:
	if (es->HaveFocus)
	    EDIT_WM_MouseMove(hwnd, wParam, lParam);
	break;

    case WM_MOVE:
	break;

    case WM_NCCREATE:
	lResult = EDIT_WM_NCCreate(hwnd, lParam);
	break;
	
    case WM_PAINT:
	EDIT_WM_Paint(hwnd);
	break;

    case WM_PASTE:
	EDIT_WM_Paste(hwnd);
	break;

    case WM_SETFOCUS:
	dprintf_edit(stddeb, "WM_SETFOCUS\n");
	es->HaveFocus = TRUE;
	if (SelMarked(es)) EDIT_UpdateSel(hwnd);
	CreateCaret(hwnd, 0, 2, es->txtht);
	SetCaretPos(es->WndCol, es->WndRow * es->txtht);
	es->CaretHidden = TRUE;
	NOTIFY_PARENT(hwnd, EN_SETFOCUS);
	break;

    case WM_SETFONT:
	EDIT_WM_SetFont(hwnd, wParam, lParam);
	break;
#if 0
    case WM_SETREDRAW:
	dprintf_edit(stddeb, "WM_SETREDRAW: hwnd=%d, wParam=%x\n",
		     hwnd, wParam);
	break;
#endif
    case WM_SETTEXT:
	EDIT_ClearDeletedText(hwnd);
	EDIT_WM_SetText(hwnd, lParam);
	break;

    case WM_SIZE:
	EDIT_WM_Size(hwnd, wParam, lParam);
	break;

    case WM_VSCROLL:
	EDIT_WM_VScroll(hwnd, wParam, lParam);
	break;

    case WM_LBUTTONDBLCLK:
	EDIT_WM_LButtonDblClk(hwnd);
	break;

    default:
	lResult = DefWindowProc(hwnd, uMsg, wParam, lParam);
	break;
    }

    EDIT_CaretUpdate(hwnd);

    return lResult;
}

