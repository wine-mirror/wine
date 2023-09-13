/*
 * Hex Edit control
 *
 * Copyright 2005 Robert Shearman
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
 *
 * TODO:
 * - Selection support
 * - Cut, copy and paste
 * - Mouse support
 */
 
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"

#include "main.h"

/* spaces dividing hex and ASCII */
#define DIV_SPACES 4

typedef struct tagHEXEDIT_INFO
{
    HWND  hwndSelf;
    HFONT hFont;
    UINT  bFocus : 1;
    UINT  bFocusHex : 1; /* TRUE if focus is on hex, FALSE if focus on ASCII */
    UINT  bInsert : 1; /* insert mode if TRUE, overwrite mode if FALSE */
    INT   nHeight; /* height of text */
    INT   nCaretPos; /* caret pos in nibbles */
    BYTE *pData;
    INT   cbData;
    INT   nBytesPerLine; /* bytes of hex to display per line of the control */
    INT   nScrollPos; /* first visible line */
} HEXEDIT_INFO;

static inline LRESULT HexEdit_SetFont (HEXEDIT_INFO *infoPtr, HFONT hFont, BOOL redraw);

static inline BYTE hexchar_to_byte(WCHAR ch)
{
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    else if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    else if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    else
        return -1;
}

static LPWSTR HexEdit_GetLineText(int offset, BYTE *pData, LONG cbData, LONG pad)
{
    WCHAR *lpszLine = malloc((6 + cbData * 3 + pad * 3 + DIV_SPACES + cbData + 1) * sizeof(WCHAR));
    LONG i;

    wsprintfW(lpszLine, L"%04X  ", offset);

    for (i = 0; i < cbData; i++)
        wsprintfW(lpszLine + 6 + i*3, L"%02X ", pData[offset + i]);
    for (i = 0; i < pad * 3; i++)
        lpszLine[6 + cbData * 3 + i] = ' ';

    for (i = 0; i < DIV_SPACES; i++)
        lpszLine[6 + cbData * 3 + pad * 3 + i] = ' ';

    /* attempt an ASCII representation if the characters are printable,
     * otherwise display a '.' */
    for (i = 0; i < cbData; i++)
    {
        /* (C1_ALPHA|C1_BLANK|C1_PUNCT|C1_DIGIT|C1_LOWER|C1_UPPER) */
        if (iswprint(pData[offset + i]))
            lpszLine[6 + cbData * 3 + pad * 3 + DIV_SPACES + i] = pData[offset + i];
        else
            lpszLine[6 + cbData * 3 + pad * 3 + DIV_SPACES + i] = '.';
    }
    lpszLine[6 + cbData * 3 + pad * 3 + DIV_SPACES + cbData] = 0;
    return lpszLine;
}

static void
HexEdit_Paint(HEXEDIT_INFO *infoPtr)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(infoPtr->hwndSelf, &ps);
    INT nXStart, nYStart;
    COLORREF clrOldText;
    HFONT hOldFont;
    INT iMode;
    LONG lByteOffset = infoPtr->nScrollPos * infoPtr->nBytesPerLine;
    int i;

    /* Make a gap from the frame */
    nXStart = GetSystemMetrics(SM_CXBORDER);
    nYStart = GetSystemMetrics(SM_CYBORDER);

    if (GetWindowLongW(infoPtr->hwndSelf, GWL_STYLE) & WS_DISABLED)
        clrOldText = SetTextColor(hdc, GetSysColor(COLOR_GRAYTEXT));
    else
        clrOldText = SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));

    iMode = SetBkMode(hdc, TRANSPARENT);
    hOldFont = SelectObject(hdc, infoPtr->hFont);

    for (i = lByteOffset; i < infoPtr->cbData; i += infoPtr->nBytesPerLine)
    {
        LPWSTR lpszLine;
        LONG nLineLen = min(infoPtr->cbData - i, infoPtr->nBytesPerLine);

        lpszLine = HexEdit_GetLineText(i, infoPtr->pData, nLineLen, infoPtr->nBytesPerLine - nLineLen);

        /* FIXME: draw hex <-> ASCII mapping highlighted? */
        TextOutW(hdc, nXStart, nYStart, lpszLine, lstrlenW(lpszLine));

        nYStart += infoPtr->nHeight;
        free(lpszLine);
    }

    SelectObject(hdc, hOldFont);
    SetBkMode(hdc, iMode);
    SetTextColor(hdc, clrOldText);

    EndPaint(infoPtr->hwndSelf, &ps);
}

static void
HexEdit_UpdateCaret(HEXEDIT_INFO *infoPtr)
{
    HDC hdc;
    HFONT hOldFont;
    SIZE size;
    INT nCaretBytePos = infoPtr->nCaretPos/2;
    INT nByteLinePos = nCaretBytePos % infoPtr->nBytesPerLine;
    INT nLine = nCaretBytePos / infoPtr->nBytesPerLine;
    LONG nLineLen = min(infoPtr->cbData - nLine * infoPtr->nBytesPerLine, infoPtr->nBytesPerLine);
    LPWSTR lpszLine = HexEdit_GetLineText(nLine * infoPtr->nBytesPerLine, infoPtr->pData, nLineLen, infoPtr->nBytesPerLine - nLineLen);
    INT nCharOffset;

    /* calculate offset of character caret is on in the line */
    if (infoPtr->bFocusHex)
        nCharOffset = 6 + nByteLinePos*3 + infoPtr->nCaretPos % 2;
    else
        nCharOffset = 6 + infoPtr->nBytesPerLine*3 + DIV_SPACES + nByteLinePos;

    hdc = GetDC(infoPtr->hwndSelf);
    hOldFont = SelectObject(hdc, infoPtr->hFont);

    GetTextExtentPoint32W(hdc, lpszLine, nCharOffset, &size);

    SelectObject(hdc, hOldFont);
    ReleaseDC(infoPtr->hwndSelf, hdc);

    if (!nLineLen) size.cx = 0;

    free(lpszLine);

    SetCaretPos(
        GetSystemMetrics(SM_CXBORDER) + size.cx,
        GetSystemMetrics(SM_CYBORDER) + (nLine - infoPtr->nScrollPos) * infoPtr->nHeight);
}

static void
HexEdit_UpdateScrollbars(HEXEDIT_INFO *infoPtr)
{
    RECT rcClient;
    INT nLines = infoPtr->cbData / infoPtr->nBytesPerLine;
    INT nVisibleLines;
    SCROLLINFO si;

    GetClientRect(infoPtr->hwndSelf, &rcClient);
    InflateRect(&rcClient, -GetSystemMetrics(SM_CXBORDER), -GetSystemMetrics(SM_CYBORDER));

    nVisibleLines = (rcClient.bottom - rcClient.top) / infoPtr->nHeight;
    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE | SIF_PAGE;
    si.nMin = 0;
    si.nMax = max(nLines - nVisibleLines, nLines);
    si.nPage = nVisibleLines;
    SetScrollInfo(infoPtr->hwndSelf, SB_VERT, &si, TRUE);
}

static void
HexEdit_EnsureVisible(HEXEDIT_INFO *infoPtr, INT nCaretPos)
{
    INT nLine = nCaretPos / (2 * infoPtr->nBytesPerLine);
    SCROLLINFO si;

    si.cbSize = sizeof(si);
    si.fMask  = SIF_POS | SIF_PAGE;
    GetScrollInfo(infoPtr->hwndSelf, SB_VERT, &si);
    if (nLine < si.nPos)
        si.nPos = nLine;
    else if (nLine >= si.nPos + si.nPage)
        si.nPos = nLine - si.nPage + 1;
    else
        return;
    si.fMask = SIF_POS;

    SetScrollInfo(infoPtr->hwndSelf, SB_VERT, &si, FALSE);
    SendMessageW(infoPtr->hwndSelf, WM_VSCROLL, MAKELONG(SB_THUMBPOSITION, 0), 0);
}


static LRESULT
HexEdit_SetData(HEXEDIT_INFO *infoPtr, INT cbData, const BYTE *pData)
{
    free(infoPtr->pData);
    infoPtr->cbData = 0;

    infoPtr->pData = malloc(cbData);
    memcpy(infoPtr->pData, pData, cbData);
    infoPtr->cbData = cbData;

    infoPtr->nCaretPos = 0;
    HexEdit_UpdateScrollbars(infoPtr);
    HexEdit_UpdateCaret(infoPtr);
    InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);
    return TRUE;
}

static LRESULT
HexEdit_GetData(HEXEDIT_INFO *infoPtr, INT cbData, BYTE *pData)
{
    if (pData)
        memcpy(pData, infoPtr->pData, min(cbData, infoPtr->cbData));
    return infoPtr->cbData;
}

static inline LRESULT
HexEdit_Char (HEXEDIT_INFO *infoPtr, WCHAR ch)
{
    INT nCaretBytePos = infoPtr->nCaretPos/2;

    assert(nCaretBytePos >= 0);

    /* backspace is special */
    if (ch == '\b')
    {
        if (infoPtr->nCaretPos == 0)
            return 0;

        /* if at end of byte then delete the whole byte */
        if (infoPtr->bFocusHex && (infoPtr->nCaretPos % 2 == 0))
        {
            memmove(infoPtr->pData + nCaretBytePos - 1,
                    infoPtr->pData + nCaretBytePos,
                    infoPtr->cbData - nCaretBytePos);
            infoPtr->cbData--;
            infoPtr->nCaretPos -= 2; /* backtrack two nibble */
        }
        else /* blank upper nibble */
        {
            infoPtr->pData[nCaretBytePos] &= 0x0f;
            infoPtr->nCaretPos--; /* backtrack one nibble */
        }
    }
    else
    {
        if (infoPtr->bFocusHex && hexchar_to_byte(ch) == (BYTE)-1)
        {
            MessageBeep(MB_ICONWARNING);
            return 0;
        }
    
        if ((infoPtr->bInsert && (infoPtr->nCaretPos % 2 == 0)) || (nCaretBytePos >= infoPtr->cbData))
        {
            /* make room for another byte */
            infoPtr->cbData++;
            infoPtr->pData = realloc(infoPtr->pData, infoPtr->cbData + 1);

            /* move everything after caret up one byte */
            memmove(infoPtr->pData + nCaretBytePos + 1,
                    infoPtr->pData + nCaretBytePos,
                    infoPtr->cbData - nCaretBytePos);
            /* zero new byte */
            infoPtr->pData[nCaretBytePos] = 0x0;
        }
    
        /* overwrite a byte */
    
        assert(nCaretBytePos < infoPtr->cbData);
    
        if (infoPtr->bFocusHex)
        {
            BYTE orig_byte = infoPtr->pData[nCaretBytePos];
            BYTE digit = hexchar_to_byte(ch);
            if (infoPtr->nCaretPos % 2) /* set low nibble */
                infoPtr->pData[nCaretBytePos] = (orig_byte & 0xf0) | digit;
            else /* set high nibble */
                infoPtr->pData[nCaretBytePos] = (orig_byte & 0x0f) | digit << 4;
            infoPtr->nCaretPos++; /* advance one nibble */
        }
        else
        {
            infoPtr->pData[nCaretBytePos] = (BYTE)ch;
            infoPtr->nCaretPos += 2; /* advance two nibbles */
        }
    }

    HexEdit_UpdateScrollbars(infoPtr);
    InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);
    HexEdit_UpdateCaret(infoPtr);
    HexEdit_EnsureVisible(infoPtr, infoPtr->nCaretPos);
    return 0;
}

static inline LRESULT
HexEdit_Destroy (HEXEDIT_INFO *infoPtr)
{
    HWND hwnd = infoPtr->hwndSelf;
    free(infoPtr->pData);
    /* free info data */
    free(infoPtr);
    SetWindowLongPtrW(hwnd, 0, 0);
    return 0;
}

static inline LRESULT
HexEdit_GetFont (HEXEDIT_INFO *infoPtr)
{
    return (LRESULT)infoPtr->hFont;
}

static inline LRESULT
HexEdit_KeyDown (HEXEDIT_INFO *infoPtr, DWORD key, DWORD flags)
{
    INT nInc = (infoPtr->bFocusHex) ? 1 : 2;
    SCROLLINFO si;

    switch (key)
    {
    case VK_LEFT:
        infoPtr->nCaretPos -= nInc;
        if (infoPtr->nCaretPos < 0)
            infoPtr->nCaretPos = 0;
        break;
    case VK_RIGHT:
        infoPtr->nCaretPos += nInc;
        if (infoPtr->nCaretPos > infoPtr->cbData*2)
            infoPtr->nCaretPos = infoPtr->cbData*2;
        break;
    case VK_UP:
        if ((infoPtr->nCaretPos - infoPtr->nBytesPerLine*2) >= 0)
            infoPtr->nCaretPos -= infoPtr->nBytesPerLine*2;
        break;
    case VK_DOWN:
        if ((infoPtr->nCaretPos + infoPtr->nBytesPerLine*2) <= infoPtr->cbData*2)
            infoPtr->nCaretPos += infoPtr->nBytesPerLine*2;
        break;
    case VK_HOME:
        infoPtr->nCaretPos = 0;
        break;
    case VK_END:
        infoPtr->nCaretPos = infoPtr->cbData*2;
        break;
    case VK_PRIOR: /* page up */
        si.cbSize = sizeof(si);
        si.fMask = SIF_PAGE;
        GetScrollInfo(infoPtr->hwndSelf, SB_VERT, &si);
        if ((infoPtr->nCaretPos - (INT)si.nPage*infoPtr->nBytesPerLine*2) >= 0)
            infoPtr->nCaretPos -= si.nPage*infoPtr->nBytesPerLine*2;
        else
            infoPtr->nCaretPos = 0;
        break;
    case VK_NEXT: /* page down */
        si.cbSize = sizeof(si);
        si.fMask = SIF_PAGE;
        GetScrollInfo(infoPtr->hwndSelf, SB_VERT, &si);
        if ((infoPtr->nCaretPos + (INT)si.nPage*infoPtr->nBytesPerLine*2) <= infoPtr->cbData*2)
            infoPtr->nCaretPos += si.nPage*infoPtr->nBytesPerLine*2;
        else
            infoPtr->nCaretPos = infoPtr->cbData*2;
        break;
    default:
        return 0;
    }

    HexEdit_UpdateCaret(infoPtr);
    HexEdit_EnsureVisible(infoPtr, infoPtr->nCaretPos);

    return 0;
}


static inline LRESULT
HexEdit_KillFocus (HEXEDIT_INFO *infoPtr, HWND receiveFocus)
{
    infoPtr->bFocus = FALSE;
    DestroyCaret();

    return 0;
}


static inline LRESULT
HexEdit_LButtonDown (HEXEDIT_INFO *infoPtr)
{
    SetFocus(infoPtr->hwndSelf);

    /* FIXME: hittest and set caret */

    return 0;
}


static inline LRESULT HexEdit_NCCreate (HWND hwnd, LPCREATESTRUCTW lpcs)
{
    HEXEDIT_INFO *infoPtr;
    SetWindowLongW(hwnd, GWL_EXSTYLE,
                   lpcs->dwExStyle | WS_EX_CLIENTEDGE);

    /* allocate memory for info structure */
    infoPtr = malloc(sizeof(HEXEDIT_INFO));
    memset(infoPtr, 0, sizeof(HEXEDIT_INFO));
    SetWindowLongPtrW(hwnd, 0, (DWORD_PTR)infoPtr);

    /* initialize info structure */
    infoPtr->nCaretPos = 0;
    infoPtr->hwndSelf = hwnd;
    infoPtr->nBytesPerLine = 2;
    infoPtr->bFocusHex = TRUE;
    infoPtr->bInsert = TRUE;

    return DefWindowProcW(infoPtr->hwndSelf, WM_NCCREATE, 0, (LPARAM)lpcs);
}

static inline LRESULT
HexEdit_SetFocus (HEXEDIT_INFO *infoPtr, HWND lostFocus)
{
    infoPtr->bFocus = TRUE;

    CreateCaret(infoPtr->hwndSelf, NULL, 1, infoPtr->nHeight);
    HexEdit_UpdateCaret(infoPtr);
    ShowCaret(infoPtr->hwndSelf);

    return 0;
}


static inline LRESULT
HexEdit_SetFont (HEXEDIT_INFO *infoPtr, HFONT hFont, BOOL redraw)
{
    TEXTMETRICW tm;
    HDC hdc;
    HFONT hOldFont = NULL;
    LONG i;
    RECT rcClient;

    infoPtr->hFont = hFont;

    hdc = GetDC(infoPtr->hwndSelf);
    if (infoPtr->hFont)
        hOldFont = SelectObject(hdc, infoPtr->hFont);

    GetTextMetricsW(hdc, &tm);
    infoPtr->nHeight = tm.tmHeight + tm.tmExternalLeading;

    GetClientRect(infoPtr->hwndSelf, &rcClient);

    for (i = 0; ; i++)
    {
        BYTE *pData = malloc(i);
        WCHAR *lpszLine;
        SIZE size;

        memset(pData, 0, i);
        lpszLine = HexEdit_GetLineText(0, pData, i, 0);
        GetTextExtentPoint32W(hdc, lpszLine, lstrlenW(lpszLine), &size);
        free(lpszLine);
        free(pData);
        if (size.cx > (rcClient.right - rcClient.left))
        {
            infoPtr->nBytesPerLine = i - 1;
            break;
        }
    }

    HexEdit_UpdateScrollbars(infoPtr);

    if (infoPtr->hFont)
        SelectObject(hdc, hOldFont);
    ReleaseDC (infoPtr->hwndSelf, hdc);

    if (redraw)
        InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);

    return 0;
}

static inline LRESULT
HexEdit_VScroll (HEXEDIT_INFO *infoPtr, INT action)
{
    SCROLLINFO si;

    /* get all scroll bar info */
    si.cbSize = sizeof(si);
    si.fMask  = SIF_ALL;
    GetScrollInfo(infoPtr->hwndSelf, SB_VERT, &si);

    switch (LOWORD(action))
    {
    case SB_TOP: /* user pressed the home key */
        si.nPos = si.nMin;
        break;

    case SB_BOTTOM: /* user pressed the end key */
        si.nPos = si.nMax;
        break;

    case SB_LINEUP: /* user clicked the up arrow */
        si.nPos -= 1;
        break;

    case SB_LINEDOWN: /* user clicked the down arrow */
        si.nPos += 1;
        break;

    case SB_PAGEUP: /* user clicked the scroll bar above the scroll thumb */
        si.nPos -= si.nPage;
        break;

    case SB_PAGEDOWN: /* user clicked the scroll bar below the scroll thumb */
        si.nPos += si.nPage;
        break;

    case SB_THUMBTRACK: /* user dragged the scroll thumb */
        si.nPos = si.nTrackPos;
        break;

    default:
        break; 
    }

    /* set the position and then retrieve it to let the system handle the
     * cases where the position is out of range */
    si.fMask = SIF_POS;
    SetScrollInfo(infoPtr->hwndSelf, SB_VERT, &si, TRUE);
    GetScrollInfo(infoPtr->hwndSelf, SB_VERT, &si);

    if (si.nPos != infoPtr->nScrollPos)
    {                    
        ScrollWindow(infoPtr->hwndSelf, 0, infoPtr->nHeight * (infoPtr->nScrollPos - si.nPos), NULL, NULL);
        infoPtr->nScrollPos = si.nPos;
        UpdateWindow(infoPtr->hwndSelf);

        /* need to update caret position since it depends on the scroll position */
        HexEdit_UpdateCaret(infoPtr);
    }
    return 0;
}


static LRESULT WINAPI
HexEdit_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HEXEDIT_INFO *infoPtr = (HEXEDIT_INFO *)GetWindowLongPtrW(hwnd, 0);

    if (!infoPtr && (uMsg != WM_NCCREATE))
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
        case HEM_SETDATA:
            return HexEdit_SetData (infoPtr, (INT)wParam, (const BYTE *)lParam);

        case HEM_GETDATA:
            return HexEdit_GetData (infoPtr, (INT)wParam, (BYTE *)lParam);

	case WM_CHAR:
	    return HexEdit_Char (infoPtr, (WCHAR)wParam);

	case WM_DESTROY:
	    return HexEdit_Destroy (infoPtr);

	case WM_GETDLGCODE:
	    return DLGC_WANTCHARS | DLGC_WANTARROWS;

	case WM_GETFONT:
	    return HexEdit_GetFont (infoPtr);

	case WM_KEYDOWN:
	    return HexEdit_KeyDown (infoPtr, wParam, lParam);

	case WM_KILLFOCUS:
	    return HexEdit_KillFocus (infoPtr, (HWND)wParam);

	case WM_LBUTTONDOWN:
	    return HexEdit_LButtonDown (infoPtr);

	case WM_NCCREATE:
	    return HexEdit_NCCreate (hwnd, (LPCREATESTRUCTW)lParam);

	case WM_PAINT:
	    HexEdit_Paint(infoPtr);
	    return 0;

	case WM_SETFOCUS:
	    return HexEdit_SetFocus (infoPtr, (HWND)wParam);

	case WM_SETFONT:
	    return HexEdit_SetFont (infoPtr, (HFONT)wParam, LOWORD(lParam));

        case WM_VSCROLL:
            return HexEdit_VScroll (infoPtr, (INT)wParam);

	default:
	    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

void HexEdit_Register(void)
{
    WNDCLASSW wndClass;

    ZeroMemory(&wndClass, sizeof(WNDCLASSW));
    wndClass.style         = 0;
    wndClass.lpfnWndProc   = HexEdit_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(HEXEDIT_INFO *);
    wndClass.hCursor       = LoadCursorW(0, (const WCHAR *)IDC_IBEAM);
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndClass.lpszClassName = L"HexEdit";

    RegisterClassW(&wndClass);
}
