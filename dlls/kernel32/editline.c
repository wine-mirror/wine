/*
 * line edition function for Win32 console
 *
 * Copyright 2001 Eric Pouech
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wincon.h"
#include "wine/unicode.h"
#include "winnls.h"
#include "wine/debug.h"
#include "console_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(console);

struct WCEL_Context;

typedef struct
{
    WCHAR			val;		/* vk or unicode char */
    void			(*func)(struct WCEL_Context* ctx);
} KeyEntry;

typedef struct
{
    DWORD			keyState;	/* keyState (from INPUT_RECORD) to match */
    BOOL			chkChar;	/* check vk or char */
    const KeyEntry*		entries;	/* array of entries */
} KeyMap;

typedef struct WCEL_Context {
    WCHAR*			line;		/* the line being edited */
    size_t			alloc;		/* number of WCHAR in line */
    unsigned    		len;		/* number of chars in line */
    unsigned                    last_rub;       /* number of chars to rub to get to start
                                                   (for consoles that can't change cursor pos) */
    unsigned                    last_max;       /* max number of chars written
                                                   (for consoles that can't change cursor pos) */
    unsigned			ofs;    	/* offset for cursor in current line */
    WCHAR*			yanked;		/* yanked line */
    unsigned			mark;		/* marked point (emacs mode only) */
    CONSOLE_SCREEN_BUFFER_INFO	csbi;		/* current state (initial cursor, window size, attribute) */
    CONSOLE_CURSOR_INFO         cinfo;          /* original cursor state (size, visibility) */
    HANDLE			hConIn;
    HANDLE			hConOut;
    unsigned			done : 1,	/* to 1 when we're done with editing */
	                        error : 1,	/* to 1 when an error occurred in the editing */
                                can_wrap : 1,   /* to 1 when multi-line edition can take place */
                                shall_echo : 1, /* to 1 when characters should be echo:ed when keyed-in */
                                insert : 1,     /* to 1 when new characters are inserted (otherwise overwrite) */
                                insertkey : 1,  /* to 1 when the Insert key toggle is active */
                                can_pos_cursor : 1; /* to 1 when console can (re)position cursor */
    unsigned			histSize;
    unsigned			histPos;
    WCHAR*			histCurr;
} WCEL_Context;

#if 0
static void WCEL_Dump(WCEL_Context* ctx, const char* pfx)
{
    MESSAGE("%s: [line=%s[alloc=%u] ofs=%u len=%u start=(%d,%d) mask=%c%c%c]\n"
	    "\t\thist=(size=%u pos=%u curr=%s)\n"
            "\t\tyanked=%s\n",
	    pfx, debugstr_w(ctx->line), ctx->alloc, ctx->ofs, ctx->len,
	    ctx->csbi.dwCursorPosition.X, ctx->csbi.dwCursorPosition.Y,
	    ctx->done ? 'D' : 'd', ctx->error ? 'E' : 'e', ctx->can_wrap ? 'W' : 'w',
	    ctx->histSize, ctx->histPos, debugstr_w(ctx->histCurr),
            debugstr_w(ctx->yanked));
}
#endif

/* ====================================================================
 *
 * Console helper functions
 *
 * ====================================================================*/

static BOOL WCEL_Get(WCEL_Context* ctx, INPUT_RECORD* ir)
{
    DWORD num_read;

    if (ReadConsoleInputW(ctx->hConIn, ir, 1, &num_read)) return TRUE;
    ctx->error = 1;
    return FALSE;
}

static inline void WCEL_Beep(WCEL_Context* ctx)
{
    Beep(400, 300);
}

static inline BOOL WCEL_IsSingleLine(WCEL_Context* ctx, size_t len)
{
    return ctx->csbi.dwCursorPosition.X + ctx->len + len <= ctx->csbi.dwSize.X;
}

static inline int WCEL_CharWidth(WCHAR wch)
{
    return wch < ' ' ? 2 : 1;
}

static inline int WCEL_StringWidth(const WCHAR* str, int beg, int len)
{
    int         i, ofs;

    for (i = 0, ofs = 0; i < len; i++)
        ofs += WCEL_CharWidth(str[beg + i]);
    return ofs;
}

static inline COORD WCEL_GetCoord(WCEL_Context* ctx, int strofs)
{
    COORD       c;
    int         len = ctx->csbi.dwSize.X - ctx->csbi.dwCursorPosition.X;
    int         ofs;

    ofs = WCEL_StringWidth(ctx->line, 0, strofs);

    c.Y = ctx->csbi.dwCursorPosition.Y;
    if (ofs >= len)
    {
        ofs -= len;
        c.X = ofs % ctx->csbi.dwSize.X;
        c.Y += 1 + ofs / ctx->csbi.dwSize.X;
    }
    else c.X = ctx->csbi.dwCursorPosition.X + ofs;
    return c;
}

static DWORD WCEL_WriteConsole(WCEL_Context* ctx, DWORD beg, DWORD len)
{
    DWORD i, last, dw, ret = 0;
    WCHAR tmp[2];

    for (i = last = 0; i < len; i++)
    {
        if (ctx->line[beg + i] < ' ')
        {
            if (last != i)
            {
                WriteConsoleW(ctx->hConOut, &ctx->line[beg + last], i - last, &dw, NULL);
                ret += dw;
            }
            tmp[0] = '^';
            tmp[1] = '@' + ctx->line[beg + i];
            WriteConsoleW(ctx->hConOut, tmp, 2, &dw, NULL);
            last = i + 1;
            ret += dw;
        }
    }
    if (last != len)
    {
        WriteConsoleW(ctx->hConOut, &ctx->line[beg + last], len - last, &dw, NULL);
        ret += dw;
    }
    return ret;
}

static inline void WCEL_WriteNChars(WCEL_Context* ctx, char ch, int count)
{
    DWORD       dw;

    if (count > 0)
    {
        while (count--) WriteFile(ctx->hConOut, &ch, 1, &dw, NULL);
    }
}

static inline void WCEL_Update(WCEL_Context* ctx, int beg, int len)
{
    int         i, last;
    DWORD       count;
    WCHAR       tmp[2];

    /* bare console case is handled in CONSOLE_ReadLine (we always reprint the whole string) */
    if (!ctx->shall_echo || !ctx->can_pos_cursor) return;

    for (i = last = beg; i < beg + len; i++)
    {
        if (ctx->line[i] < ' ')
        {
            if (last != i)
            {
                WriteConsoleOutputCharacterW(ctx->hConOut, &ctx->line[last], i - last,
                                             WCEL_GetCoord(ctx, last), &count);
                FillConsoleOutputAttribute(ctx->hConOut, ctx->csbi.wAttributes, i - last,
                                           WCEL_GetCoord(ctx, last), &count);
            }
            tmp[0] = '^';
            tmp[1] = '@' + ctx->line[i];
            WriteConsoleOutputCharacterW(ctx->hConOut, tmp, 2,
                                         WCEL_GetCoord(ctx, i), &count);
            FillConsoleOutputAttribute(ctx->hConOut, ctx->csbi.wAttributes, 2,
                                       WCEL_GetCoord(ctx, i), &count);
            last = i + 1;
        }
    }
    if (last != beg + len)
    {
        WriteConsoleOutputCharacterW(ctx->hConOut, &ctx->line[last], i - last,
                                     WCEL_GetCoord(ctx, last), &count);
        FillConsoleOutputAttribute(ctx->hConOut, ctx->csbi.wAttributes, i - last,
                                   WCEL_GetCoord(ctx, last), &count);
    }
}

/* ====================================================================
 *
 * context manipulation functions
 *
 * ====================================================================*/

static BOOL WCEL_Grow(WCEL_Context* ctx, size_t len)
{
    if (!WCEL_IsSingleLine(ctx, len) && !ctx->can_wrap)
    {
        FIXME("Mode doesn't allow wrapping. However, we should allow overwriting the current string\n");
        return FALSE;
    }

    if (ctx->len + len >= ctx->alloc)
    {
	WCHAR*	newline;
        size_t  newsize;

        /* round up size to 32 byte-WCHAR boundary */
        newsize = (ctx->len + len + 1 + 31) & ~31;

	if (ctx->line)
	    newline = HeapReAlloc(GetProcessHeap(), 0, ctx->line, sizeof(WCHAR) * newsize);
	else
	    newline = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR) * newsize);

	if (!newline) return FALSE;
	ctx->line = newline;
	ctx->alloc = newsize;
    }
    return TRUE;
}

static void WCEL_DeleteString(WCEL_Context* ctx, int beg, int end)
{
    unsigned    str_len = end - beg;

    if (end < ctx->len)
	memmove(&ctx->line[beg], &ctx->line[end], (ctx->len - end) * sizeof(WCHAR));
    /* we need to clean from ctx->len - str_len to ctx->len */

    if (ctx->shall_echo)
    {
        COORD       cbeg = WCEL_GetCoord(ctx, ctx->len - str_len);
        COORD       cend = WCEL_GetCoord(ctx, ctx->len);
        CHAR_INFO   ci;

        ci.Char.UnicodeChar = ' ';
        ci.Attributes = ctx->csbi.wAttributes;

        if (cbeg.Y == cend.Y)
        {
            /* partial erase of sole line */
            CONSOLE_FillLineUniform(ctx->hConOut, cbeg.X, cbeg.Y,
                                    cend.X - cbeg.X, &ci);
        }
        else
        {
            int         i;
            /* erase til eol on first line */
            CONSOLE_FillLineUniform(ctx->hConOut, cbeg.X, cbeg.Y,
                                    ctx->csbi.dwSize.X - cbeg.X, &ci);
            /* completely erase all the others (full lines) */
            for (i = cbeg.Y + 1; i < cend.Y; i++)
                CONSOLE_FillLineUniform(ctx->hConOut, 0, i, ctx->csbi.dwSize.X, &ci);
            /* erase from beginning of line until last pos on last line */
            CONSOLE_FillLineUniform(ctx->hConOut, 0, cend.Y, cend.X, &ci);
        }
    }
    ctx->len -= str_len;
    WCEL_Update(ctx, 0, ctx->len);
    ctx->line[ctx->len] = 0;
}

static void WCEL_InsertString(WCEL_Context* ctx, const WCHAR* str)
{
    size_t	len = lstrlenW(str), updtlen;

    if (!len) return;
    if (ctx->insert)
    {
        if (!WCEL_Grow(ctx, len)) return;
        if (ctx->len > ctx->ofs)
            memmove(&ctx->line[ctx->ofs + len], &ctx->line[ctx->ofs], (ctx->len - ctx->ofs) * sizeof(WCHAR));
        ctx->len += len;
        updtlen = ctx->len - ctx->ofs;
    }
    else
    {
        if (ctx->ofs + len > ctx->len)
        {
            if (!WCEL_Grow(ctx, (ctx->ofs + len) - ctx->len)) return;
            ctx->len = ctx->ofs + len;
        }
        updtlen = len;
    }
    memcpy(&ctx->line[ctx->ofs], str, len * sizeof(WCHAR));
    ctx->line[ctx->len] = 0;
    WCEL_Update(ctx, ctx->ofs, updtlen);
    ctx->ofs += len;
}

static void WCEL_InsertChar(WCEL_Context* ctx, WCHAR c)
{
    WCHAR	buffer[2];

    buffer[0] = c;
    buffer[1] = 0;
    WCEL_InsertString(ctx, buffer);
}

static void WCEL_FreeYank(WCEL_Context* ctx)
{
    HeapFree(GetProcessHeap(), 0, ctx->yanked);
    ctx->yanked = NULL;
}

static void WCEL_SaveYank(WCEL_Context* ctx, int beg, int end)
{
    int len = end - beg;
    if (len <= 0) return;

    WCEL_FreeYank(ctx);
    /* After WCEL_FreeYank ctx->yanked is empty */
    ctx->yanked = HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR));
    if (!ctx->yanked) return;
    memcpy(ctx->yanked, &ctx->line[beg], len * sizeof(WCHAR));
    ctx->yanked[len] = 0;
}

/* FIXME NTDLL doesn't export iswalnum, and I don't want to link in msvcrt when most
 * of the data lay in unicode lib
 */
static inline BOOL WCEL_iswalnum(WCHAR wc)
{
    return get_char_typeW(wc) & (C1_ALPHA|C1_DIGIT|C1_LOWER|C1_UPPER);
}

static int WCEL_GetLeftWordTransition(WCEL_Context* ctx, int ofs)
{
    ofs--;
    while (ofs >= 0 && !WCEL_iswalnum(ctx->line[ofs])) ofs--;
    while (ofs >= 0 && WCEL_iswalnum(ctx->line[ofs])) ofs--;
    if (ofs >= 0) ofs++;
    return max(ofs, 0);
}

static int WCEL_GetRightWordTransition(WCEL_Context* ctx, int ofs)
{
    ofs++;
    while (ofs <= ctx->len && WCEL_iswalnum(ctx->line[ofs])) ofs++;
    while (ofs <= ctx->len && !WCEL_iswalnum(ctx->line[ofs])) ofs++;
    return min(ofs, ctx->len);
}

static WCHAR* WCEL_GetHistory(WCEL_Context* ctx, int idx)
{
    WCHAR*	ptr;

    if (idx == ctx->histSize - 1)
    {
	ptr = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(ctx->histCurr) + 1) * sizeof(WCHAR));
	lstrcpyW(ptr, ctx->histCurr);
    }
    else
    {
	int	len = CONSOLE_GetHistory(idx, NULL, 0);

	if ((ptr = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR))))
	{
	    CONSOLE_GetHistory(idx, ptr, len);
	}
    }
    return ptr;
}

static void	WCEL_HistoryInit(WCEL_Context* ctx)
{
    ctx->histPos  = CONSOLE_GetNumHistoryEntries();
    ctx->histSize = ctx->histPos + 1;
    ctx->histCurr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WCHAR));
}

static void    WCEL_MoveToHist(WCEL_Context* ctx, int idx)
{
    WCHAR*	data = WCEL_GetHistory(ctx, idx);
    int		len = lstrlenW(data) + 1;

    /* save current line edition for recall when needed (FIXME seems broken to me) */
    if (ctx->histPos == ctx->histSize - 1)
    {
	HeapFree(GetProcessHeap(), 0, ctx->histCurr);
	ctx->histCurr = HeapAlloc(GetProcessHeap(), 0, (ctx->len + 1) * sizeof(WCHAR));
	memcpy(ctx->histCurr, ctx->line, (ctx->len + 1) * sizeof(WCHAR));
    }
    /* need to clean also the screen if new string is shorter than old one */
    WCEL_DeleteString(ctx, 0, ctx->len);
    ctx->ofs = 0;
    /* insert new string */
    if (WCEL_Grow(ctx, len))
    {
	WCEL_InsertString(ctx, data);
	ctx->histPos = idx;
    }
    HeapFree(GetProcessHeap(), 0, data);
}

static void    WCEL_FindPrevInHist(WCEL_Context* ctx)
{
    int startPos = ctx->histPos;
    WCHAR*	data;
    unsigned int    len, oldofs;

    if (ctx->histPos && ctx->histPos == ctx->histSize) {
        startPos--;
        ctx->histPos--;
    }

    do {
       data = WCEL_GetHistory(ctx, ctx->histPos);

       if (ctx->histPos) ctx->histPos--;
       else ctx->histPos = (ctx->histSize-1);

       len = lstrlenW(data) + 1;
       if ((len >= ctx->ofs) &&
           (memcmp(ctx->line, data, ctx->ofs * sizeof(WCHAR)) == 0)) {

           /* need to clean also the screen if new string is shorter than old one */
           WCEL_DeleteString(ctx, 0, ctx->len);

           if (WCEL_Grow(ctx, len))
           {
              oldofs = ctx->ofs;
              ctx->ofs = 0;
              WCEL_InsertString(ctx, data);
              ctx->ofs = oldofs;
              if (ctx->shall_echo)
                  SetConsoleCursorPosition(ctx->hConOut, WCEL_GetCoord(ctx, ctx->ofs));
              HeapFree(GetProcessHeap(), 0, data);
              return;
           }
       }
       HeapFree(GetProcessHeap(), 0, data);
    } while (ctx->histPos != startPos);

    return;
}

/* ====================================================================
 *
 * basic edition functions
 *
 * ====================================================================*/

static void WCEL_Done(WCEL_Context* ctx)
{
    WCHAR       nl = '\n';
    if (!WCEL_Grow(ctx, 2)) return;
    ctx->line[ctx->len++] = '\r';
    ctx->line[ctx->len++] = '\n';
    ctx->line[ctx->len] = 0;
    WriteConsoleW(ctx->hConOut, &nl, 1, NULL, NULL);
    if (ctx->insertkey)
        SetConsoleCursorInfo(ctx->hConOut, &ctx->cinfo);
    ctx->done = 1;
}

static void WCEL_MoveLeft(WCEL_Context* ctx)
{
    if (ctx->ofs > 0) ctx->ofs--;
}

static void WCEL_MoveRight(WCEL_Context* ctx)
{
    if (ctx->ofs < ctx->len) ctx->ofs++;
}

static void WCEL_MoveToLeftWord(WCEL_Context* ctx)
{
    unsigned int	new_ofs = WCEL_GetLeftWordTransition(ctx, ctx->ofs);
    if (new_ofs != ctx->ofs) ctx->ofs = new_ofs;
}

static void WCEL_MoveToRightWord(WCEL_Context* ctx)
{
    unsigned int	new_ofs = WCEL_GetRightWordTransition(ctx, ctx->ofs);
    if (new_ofs != ctx->ofs) ctx->ofs = new_ofs;
}

static void WCEL_MoveToBeg(WCEL_Context* ctx)
{
    ctx->ofs = 0;
}

static void WCEL_MoveToEnd(WCEL_Context* ctx)
{
    ctx->ofs = ctx->len;
}

static void WCEL_SetMark(WCEL_Context* ctx)
{
    ctx->mark = ctx->ofs;
}

static void WCEL_ExchangeMark(WCEL_Context* ctx)
{
    unsigned tmp;

    if (ctx->mark > ctx->len) return;
    tmp = ctx->ofs;
    ctx->ofs = ctx->mark;
    ctx->mark = tmp;
}

static void WCEL_CopyMarkedZone(WCEL_Context* ctx)
{
    unsigned beg, end;

    if (ctx->mark > ctx->len || ctx->mark == ctx->ofs) return;
    if (ctx->mark > ctx->ofs)
    {
	beg = ctx->ofs;		end = ctx->mark;
    }
    else
    {
	beg = ctx->mark;	end = ctx->ofs;
    }
    WCEL_SaveYank(ctx, beg, end);
}

static void WCEL_TransposeChar(WCEL_Context* ctx)
{
    WCHAR	c;

    if (!ctx->ofs || ctx->ofs == ctx->len) return;

    c = ctx->line[ctx->ofs];
    ctx->line[ctx->ofs] = ctx->line[ctx->ofs - 1];
    ctx->line[ctx->ofs - 1] = c;

    WCEL_Update(ctx, ctx->ofs - 1, 2);
    ctx->ofs++;
}

static void WCEL_TransposeWords(WCEL_Context* ctx)
{
    unsigned int	left_ofs = WCEL_GetLeftWordTransition(ctx, ctx->ofs),
        right_ofs = WCEL_GetRightWordTransition(ctx, ctx->ofs);
    if (left_ofs < ctx->ofs && right_ofs > ctx->ofs)
    {
        unsigned len_r = right_ofs - ctx->ofs;
        unsigned len_l = ctx->ofs - left_ofs;

        char*   tmp = HeapAlloc(GetProcessHeap(), 0, len_r * sizeof(WCHAR));
        if (!tmp) return;

        memcpy(tmp, &ctx->line[ctx->ofs], len_r * sizeof(WCHAR));
        memmove(&ctx->line[left_ofs + len_r], &ctx->line[left_ofs], len_l * sizeof(WCHAR));
        memcpy(&ctx->line[left_ofs], tmp, len_r * sizeof(WCHAR));

        HeapFree(GetProcessHeap(), 0, tmp);
        WCEL_Update(ctx, left_ofs, len_l + len_r);
        ctx->ofs = right_ofs;
    }
}

static void WCEL_LowerCaseWord(WCEL_Context* ctx)
{
    unsigned int	new_ofs = WCEL_GetRightWordTransition(ctx, ctx->ofs);
    if (new_ofs != ctx->ofs)
    {
	unsigned int	i;
	for (i = ctx->ofs; i <= new_ofs; i++)
	    ctx->line[i] = tolowerW(ctx->line[i]);
        WCEL_Update(ctx, ctx->ofs, new_ofs - ctx->ofs + 1);
	ctx->ofs = new_ofs;
    }
}

static void WCEL_UpperCaseWord(WCEL_Context* ctx)
{
    unsigned int	new_ofs = WCEL_GetRightWordTransition(ctx, ctx->ofs);
    if (new_ofs != ctx->ofs)
    {
	unsigned int	i;
	for (i = ctx->ofs; i <= new_ofs; i++)
	    ctx->line[i] = toupperW(ctx->line[i]);
	WCEL_Update(ctx, ctx->ofs, new_ofs - ctx->ofs + 1);
	ctx->ofs = new_ofs;
    }
}

static void WCEL_CapitalizeWord(WCEL_Context* ctx)
{
    unsigned int	new_ofs = WCEL_GetRightWordTransition(ctx, ctx->ofs);
    if (new_ofs != ctx->ofs)
    {
	unsigned int	i;

	ctx->line[ctx->ofs] = toupperW(ctx->line[ctx->ofs]);
	for (i = ctx->ofs + 1; i <= new_ofs; i++)
	    ctx->line[i] = tolowerW(ctx->line[i]);
	WCEL_Update(ctx, ctx->ofs, new_ofs - ctx->ofs + 1);
	ctx->ofs = new_ofs;
    }
}

static void WCEL_Yank(WCEL_Context* ctx)
{
    if (ctx->yanked)
        WCEL_InsertString(ctx, ctx->yanked);
}

static void WCEL_KillToEndOfLine(WCEL_Context* ctx)
{
    WCEL_SaveYank(ctx, ctx->ofs, ctx->len);
    WCEL_DeleteString(ctx, ctx->ofs, ctx->len);
}

static void WCEL_KillFromBegOfLine(WCEL_Context* ctx)
{
    if (ctx->ofs)
    {
        WCEL_SaveYank(ctx, 0, ctx->ofs);
        WCEL_DeleteString(ctx, 0, ctx->ofs);
        ctx->ofs = 0;
    }
}

static void WCEL_KillMarkedZone(WCEL_Context* ctx)
{
    unsigned beg, end;

    if (ctx->mark > ctx->len || ctx->mark == ctx->ofs) return;
    if (ctx->mark > ctx->ofs)
    {
	beg = ctx->ofs;		end = ctx->mark;
    }
    else
    {
	beg = ctx->mark;	end = ctx->ofs;
    }
    WCEL_SaveYank(ctx, beg, end);
    WCEL_DeleteString(ctx, beg, end);
    ctx->ofs = beg;
}

static void WCEL_DeletePrevChar(WCEL_Context* ctx)
{
    if (ctx->ofs)
    {
	WCEL_DeleteString(ctx, ctx->ofs - 1, ctx->ofs);
	ctx->ofs--;
    }
}

static void WCEL_DeleteCurrChar(WCEL_Context* ctx)
{
    if (ctx->ofs < ctx->len)
	WCEL_DeleteString(ctx, ctx->ofs, ctx->ofs + 1);
}

static void WCEL_DeleteLeftWord(WCEL_Context* ctx)
{
    unsigned int	new_ofs = WCEL_GetLeftWordTransition(ctx, ctx->ofs);
    if (new_ofs != ctx->ofs)
    {
	WCEL_DeleteString(ctx, new_ofs, ctx->ofs);
	ctx->ofs = new_ofs;
    }
}

static void WCEL_DeleteRightWord(WCEL_Context* ctx)
{
    unsigned int	new_ofs = WCEL_GetRightWordTransition(ctx, ctx->ofs);
    if (new_ofs != ctx->ofs)
    {
	WCEL_DeleteString(ctx, ctx->ofs, new_ofs);
    }
}

static void WCEL_MoveToPrevHist(WCEL_Context* ctx)
{
    if (ctx->histPos) WCEL_MoveToHist(ctx, ctx->histPos - 1);
}

static void WCEL_MoveToNextHist(WCEL_Context* ctx)
{
    if (ctx->histPos < ctx->histSize - 1) WCEL_MoveToHist(ctx, ctx->histPos + 1);
}

static void WCEL_MoveToFirstHist(WCEL_Context* ctx)
{
    if (ctx->histPos != 0) WCEL_MoveToHist(ctx, 0);
}

static void WCEL_MoveToLastHist(WCEL_Context* ctx)
{
    if (ctx->histPos != ctx->histSize - 1) WCEL_MoveToHist(ctx, ctx->histSize - 1);
}

static void WCEL_Redraw(WCEL_Context* ctx)
{
    if (ctx->shall_echo)
    {
        COORD       c = WCEL_GetCoord(ctx, ctx->len);
        CHAR_INFO   ci;

        WCEL_Update(ctx, 0, ctx->len);

        ci.Char.UnicodeChar = ' ';
        ci.Attributes = ctx->csbi.wAttributes;

        CONSOLE_FillLineUniform(ctx->hConOut, c.X, c.Y, ctx->csbi.dwSize.X - c.X, &ci);
    }
}

static void WCEL_RepeatCount(WCEL_Context* ctx)
{
#if 0
/* FIXME: wait until all console code is in kernel32 */
    INPUT_RECORD        ir;
    unsigned            repeat = 0;

    while (WCEL_Get(ctx, &ir, FALSE))
    {
        if (ir.EventType != KEY_EVENT) break;
        if (ir.Event.KeyEvent.bKeyDown)
        {
            if ((ir.Event.KeyEvent.dwControlKeyState & ~(NUMLOCK_ON|SCROLLLOCK_ON|CAPSLOCK_ON)) != 0)
                break;
            if (ir.Event.KeyEvent.uChar.UnicodeChar < '0' ||
                ir.Event.KeyEvent.uChar.UnicodeChar > '9')
                break;
            repeat = repeat * 10 + ir.Event.KeyEvent.uChar.UnicodeChar - '0';
        }
        WCEL_Get(ctx, &ir, TRUE);
    }
    FIXME("=> %u\n", repeat);
#endif
}

static void WCEL_ToggleInsert(WCEL_Context* ctx)
{
    CONSOLE_CURSOR_INFO cinfo;

    ctx->insertkey = !ctx->insertkey;

    if (GetConsoleCursorInfo(ctx->hConOut, &cinfo))
    {
        cinfo.dwSize = ctx->insertkey ? 100 : 25;
        SetConsoleCursorInfo(ctx->hConOut, &cinfo);
    }
}

/* ====================================================================
 *
 * 		Key Maps
 *
 * ====================================================================*/

#define CTRL(x)	((x) - '@')
static const KeyEntry StdKeyMap[] =
{
    {/*VK_BACK*/  0x08, WCEL_DeletePrevChar     },
    {/*VK_RETURN*/0x0d, WCEL_Done               },
    {/*VK_DELETE*/0x2e, WCEL_DeleteCurrChar     },
    {	0,		NULL			}
};

static const KeyEntry EmacsKeyMapCtrl[] =
{
    {	CTRL('@'),	WCEL_SetMark		},
    {	CTRL('A'),	WCEL_MoveToBeg		},
    {	CTRL('B'),	WCEL_MoveLeft		},
    /* C: done in server */
    {	CTRL('D'),	WCEL_DeleteCurrChar	},
    {	CTRL('E'),	WCEL_MoveToEnd		},
    {	CTRL('F'),	WCEL_MoveRight		},
    {	CTRL('G'),	WCEL_Beep		},
    {	CTRL('H'),	WCEL_DeletePrevChar	},
    /* I: meaningless (or tab ???) */
    {	CTRL('J'),	WCEL_Done		},
    {	CTRL('K'),	WCEL_KillToEndOfLine	},
    {   CTRL('L'),      WCEL_Redraw             },
    {	CTRL('M'),	WCEL_Done		},
    {	CTRL('N'),	WCEL_MoveToNextHist	},
    /* O; insert line... meaningless */
    {	CTRL('P'),	WCEL_MoveToPrevHist	},
    /* Q: [NIY] quoting... */
    /* R: [NIY] search backwards... */
    /* S: [NIY] search forwards... */
    {	CTRL('T'),	WCEL_TransposeChar	},
    {   CTRL('U'),      WCEL_RepeatCount        },
    /* V: paragraph down... meaningless */
    {	CTRL('W'),	WCEL_KillMarkedZone	},
    {	CTRL('X'),	WCEL_ExchangeMark	},
    {	CTRL('Y'),	WCEL_Yank		},
    /* Z: meaningless */
    {	0,		NULL			}
};

static const KeyEntry EmacsKeyMapAlt[] =
{
    {/*DEL*/127,	WCEL_DeleteLeftWord	},
    {	'<',		WCEL_MoveToFirstHist	},
    {	'>',		WCEL_MoveToLastHist	},
    {	'?',		WCEL_Beep		},
    {	'b',		WCEL_MoveToLeftWord	},
    {   'c',		WCEL_CapitalizeWord	},
    {	'd',		WCEL_DeleteRightWord	},
    {	'f',		WCEL_MoveToRightWord	},
    {	'l',		WCEL_LowerCaseWord	},
    {   't',		WCEL_TransposeWords	},
    {	'u',		WCEL_UpperCaseWord	},
    {	'w', 		WCEL_CopyMarkedZone	},
    {	0,		NULL			}
};

static const KeyEntry EmacsStdKeyMap[] =
{
    {/*VK_PRIOR*/0x21, 	WCEL_MoveToPrevHist	},
    {/*VK_NEXT*/ 0x22,	WCEL_MoveToNextHist 	},
    {/*VK_END*/  0x23,	WCEL_MoveToEnd		},
    {/*VK_HOME*/ 0x24,	WCEL_MoveToBeg		},
    {/*VK_RIGHT*/0x27,	WCEL_MoveRight 		},
    {/*VK_LEFT*/ 0x25,	WCEL_MoveLeft 		},
    {/*VK_INSERT*/0x2d, WCEL_ToggleInsert 	},
    {	0,		NULL 			}
};

static const KeyMap EmacsKeyMap[] =
{
    {0,                  0, StdKeyMap},
    {0,                  0, EmacsStdKeyMap},
    {RIGHT_ALT_PRESSED,  1, EmacsKeyMapAlt},	/* right alt  */
    {LEFT_ALT_PRESSED,   1, EmacsKeyMapAlt},	/* left  alt  */
    {RIGHT_CTRL_PRESSED, 1, EmacsKeyMapCtrl},	/* right ctrl */
    {LEFT_CTRL_PRESSED,  1, EmacsKeyMapCtrl},	/* left  ctrl */
    {0,                  0, NULL}
};

static const KeyEntry Win32StdKeyMap[] =
{
    {/*VK_LEFT*/ 0x25, 	WCEL_MoveLeft 		},
    {/*VK_RIGHT*/0x27,	WCEL_MoveRight		},
    {/*VK_HOME*/ 0x24,	WCEL_MoveToBeg 		},
    {/*VK_END*/  0x23,	WCEL_MoveToEnd 		},
    {/*VK_UP*/   0x26, 	WCEL_MoveToPrevHist 	},
    {/*VK_DOWN*/ 0x28,	WCEL_MoveToNextHist	},
    {/*VK_INSERT*/0x2d, WCEL_ToggleInsert 	},
    {/*VK_F8*/   0x77,	WCEL_FindPrevInHist	},
    {	0,		NULL 			}
};

static const KeyEntry Win32KeyMapCtrl[] =
{
    {/*VK_LEFT*/ 0x25, 	WCEL_MoveToLeftWord 	},
    {/*VK_RIGHT*/0x27,	WCEL_MoveToRightWord	},
    {/*VK_END*/  0x23,	WCEL_KillToEndOfLine	},
    {/*VK_HOME*/ 0x24,	WCEL_KillFromBegOfLine	},
    {	0,		NULL 			}
};

static const KeyMap Win32KeyMap[] =
{
    {0,                  0, StdKeyMap},
    {SHIFT_PRESSED,      0, StdKeyMap},
    {0,                  0, Win32StdKeyMap},
    {RIGHT_CTRL_PRESSED, 0, Win32KeyMapCtrl},
    {LEFT_CTRL_PRESSED,  0, Win32KeyMapCtrl},
    {0,                  0, NULL}
};
#undef CTRL

/* ====================================================================
 *
 * 		Read line master function
 *
 * ====================================================================*/

WCHAR* CONSOLE_Readline(HANDLE hConsoleIn, BOOL can_pos_cursor)
{
    WCEL_Context	ctx;
    INPUT_RECORD	ir;
    const KeyMap*	km;
    const KeyEntry*	ke;
    unsigned		ofs;
    void		(*func)(struct WCEL_Context* ctx);
    DWORD               mode, input_mode, ks;
    int                 use_emacs;
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    memset(&ctx, 0, sizeof(ctx));
    ctx.hConIn = hConsoleIn;
    WCEL_HistoryInit(&ctx);

    if (!CONSOLE_GetEditionMode(hConsoleIn, &use_emacs))
        use_emacs = 0;

    if ((ctx.hConOut = CreateFileA("CONOUT$", GENERIC_READ|GENERIC_WRITE, 0, NULL,
				    OPEN_EXISTING, 0, 0 )) == INVALID_HANDLE_VALUE ||
	!GetConsoleScreenBufferInfo(ctx.hConOut, &ctx.csbi))
	return NULL;
    if (!GetConsoleMode(hConsoleIn, &mode)) mode = 0;
    input_mode = mode;
    ctx.shall_echo = (mode & ENABLE_ECHO_INPUT) ? 1 : 0;
    ctx.insert = (mode & (ENABLE_INSERT_MODE|ENABLE_EXTENDED_FLAGS)) == (ENABLE_INSERT_MODE|ENABLE_EXTENDED_FLAGS) ? 1 : 0;
    if (!GetConsoleMode(ctx.hConOut, &mode)) mode = 0;
    ctx.can_wrap = (mode & ENABLE_WRAP_AT_EOL_OUTPUT) ? 1 : 0;
    ctx.can_pos_cursor = can_pos_cursor;
    GetConsoleCursorInfo(ctx.hConOut, &ctx.cinfo);

    if (!WCEL_Grow(&ctx, 1))
    {
	CloseHandle(ctx.hConOut);
	return NULL;
    }
    ctx.line[0] = 0;

/* EPP     WCEL_Dump(&ctx, "init"); */

    while (!ctx.done && !ctx.error && WCEL_Get(&ctx, &ir))
    {
	if (ir.EventType != KEY_EVENT) continue;
	TRACE("key%s repeatCount=%u, keyCode=%02x scanCode=%02x char=%02x keyState=%08x\n",
	      ir.Event.KeyEvent.bKeyDown ? "Down" : "Up  ", ir.Event.KeyEvent.wRepeatCount,
	      ir.Event.KeyEvent.wVirtualKeyCode, ir.Event.KeyEvent.wVirtualScanCode,
	      ir.Event.KeyEvent.uChar.UnicodeChar, ir.Event.KeyEvent.dwControlKeyState);
	if (!ir.Event.KeyEvent.bKeyDown) continue;

/* EPP  	WCEL_Dump(&ctx, "before func"); */
	ofs = ctx.ofs;
        /* mask out some bits which don't interest us */
        ks = ir.Event.KeyEvent.dwControlKeyState & ~(NUMLOCK_ON|SCROLLLOCK_ON|CAPSLOCK_ON|ENHANCED_KEY);

	func = NULL;
	for (km = (use_emacs) ? EmacsKeyMap : Win32KeyMap; km->entries != NULL; km++)
	{
	    if (km->keyState != ks)
		continue;
	    if (km->chkChar)
	    {
		for (ke = &km->entries[0]; ke->func != 0; ke++)
		    if (ke->val == ir.Event.KeyEvent.uChar.UnicodeChar) break;
	    }
	    else
	    {
		for (ke = &km->entries[0]; ke->func != 0; ke++)
		    if (ke->val == ir.Event.KeyEvent.wVirtualKeyCode) break;

	    }
	    if (ke->func)
	    {
		func = ke->func;
		break;
	    }
	}

        CONSOLE_GetEditionMode(hConsoleIn, &use_emacs);

        GetConsoleMode(hConsoleIn, &mode);
        if (input_mode != mode)
        {
            input_mode = mode;
            ctx.insertkey = 0;
        }
        ctx.insert = (mode & (ENABLE_INSERT_MODE|ENABLE_EXTENDED_FLAGS)) ==
                     (ENABLE_INSERT_MODE|ENABLE_EXTENDED_FLAGS);
        if (ctx.insertkey)
            ctx.insert = !ctx.insert;

        GetConsoleScreenBufferInfo(ctx.hConOut, &csbi);
        ctx.csbi.wAttributes = csbi.wAttributes;

	if (func)
	    (func)(&ctx);
	else if (!(ir.Event.KeyEvent.dwControlKeyState & LEFT_ALT_PRESSED))
	    WCEL_InsertChar(&ctx, ir.Event.KeyEvent.uChar.UnicodeChar);
	else TRACE("Dropped event\n");

/* EPP         WCEL_Dump(&ctx, "after func"); */
        if (!ctx.shall_echo) continue;
        if (ctx.can_pos_cursor)
        {
            if (ctx.ofs != ofs)
                SetConsoleCursorPosition(ctx.hConOut, WCEL_GetCoord(&ctx, ctx.ofs));
        }
        else if (!ctx.done && !ctx.error)
        {
            DWORD last;
            /* erase previous chars */
            WCEL_WriteNChars(&ctx, '\b', ctx.last_rub);

            /* write chars up to cursor */
            ctx.last_rub = WCEL_WriteConsole(&ctx, 0, ctx.ofs);
            /* write chars past cursor */
            last = ctx.last_rub + WCEL_WriteConsole(&ctx, ctx.ofs, ctx.len - ctx.ofs);
            if (last < ctx.last_max) /* ctx.line has been shortened, erase */
            {
                WCEL_WriteNChars(&ctx, ' ', ctx.last_max - last);
                WCEL_WriteNChars(&ctx, '\b', ctx.last_max - last);
                ctx.last_max = last;
            }
            else ctx.last_max = last;
            /* reposition at cursor */
            WCEL_WriteNChars(&ctx, '\b', last - ctx.last_rub);
        }
    }
    if (ctx.error)
    {
	HeapFree(GetProcessHeap(), 0, ctx.line);
	ctx.line = NULL;
    }
    WCEL_FreeYank(&ctx);
    if (ctx.line)
	CONSOLE_AppendHistory(ctx.line);

    CloseHandle(ctx.hConOut);
    HeapFree(GetProcessHeap(), 0, ctx.histCurr);
    return ctx.line;
}
