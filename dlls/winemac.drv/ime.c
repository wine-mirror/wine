/*
 * The IME for interfacing with Mac input methods
 *
 * Copyright 2008, 2013 CodeWeavers, Aric Stewart
 * Copyright 2013 Ken Thomases for CodeWeavers Inc.
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

/*
 * Notes:
 *  The normal flow for IMM/IME Processing is as follows.
 * 1) The Keyboard Driver generates key messages which are first passed to
 *    the IMM and then to IME via ImeProcessKey. If the IME returns 0  then
 *    it does not want the key and the keyboard driver then generates the
 *    WM_KEYUP/WM_KEYDOWN messages.  However if the IME is going to process the
 *    key it returns non-zero.
 * 2) If the IME is going to process the key then the IMM calls ImeToAsciiEx to
 *    process the key.  the IME modifies the HIMC structure to reflect the
 *    current state and generates any messages it needs the IMM to process.
 * 3) IMM checks the messages and send them to the application in question. From
 *    here the IMM level deals with if the application is IME aware or not.
 */

#include "macdrv_dll.h"
#include "imm.h"
#include "immdev.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);

#define FROM_MACDRV ((HIMC)0xcafe1337)

static HIMC *hSelectedFrom = NULL;
static INT  hSelectedCount = 0;

static WCHAR *input_context_get_comp_str( INPUTCONTEXT *ctx, BOOL result, UINT *length )
{
    COMPOSITIONSTRING *string;
    WCHAR *text = NULL;
    UINT len, off;

    if (!(string = ImmLockIMCC( ctx->hCompStr ))) return NULL;
    len = result ? string->dwResultStrLen : string->dwCompStrLen;
    off = result ? string->dwResultStrOffset : string->dwCompStrOffset;

    if (len && off && (text = malloc( (len + 1) * sizeof(WCHAR) )))
    {
        memcpy( text, (BYTE *)string + off, len * sizeof(WCHAR) );
        text[len] = 0;
        *length = len;
    }

    ImmUnlockIMCC( ctx->hCompStr );
    return text;
}

static HWND input_context_get_ui_hwnd( INPUTCONTEXT *ctx )
{
    struct ime_private *priv;
    HWND hwnd;
    if (!(priv = ImmLockIMCC( ctx->hPrivate ))) return NULL;
    hwnd = priv->hwndDefault;
    ImmUnlockIMCC( ctx->hPrivate );
    return hwnd;
}

static HFONT input_context_select_ui_font( INPUTCONTEXT *ctx, HDC hdc )
{
    struct ime_private *priv;
    HFONT font = NULL;
    if (!(priv = ImmLockIMCC( ctx->hPrivate ))) return NULL;
    if (priv->textfont) font = SelectObject( hdc, priv->textfont );
    ImmUnlockIMCC( ctx->hPrivate );
    return font;
}

static HIMC RealIMC(HIMC hIMC)
{
    if (hIMC == FROM_MACDRV)
    {
        INT i;
        HWND wnd = GetFocus();
        HIMC winHimc = ImmGetContext(wnd);
        for (i = 0; i < hSelectedCount; i++)
            if (winHimc == hSelectedFrom[i])
                return winHimc;
        return NULL;
    }
    else
        return hIMC;
}

static LPINPUTCONTEXT LockRealIMC(HIMC hIMC)
{
    HIMC real_imc = RealIMC(hIMC);
    if (real_imc)
        return ImmLockIMC(real_imc);
    else
        return NULL;
}

static BOOL UnlockRealIMC(HIMC hIMC)
{
    HIMC real_imc = RealIMC(hIMC);
    if (real_imc)
        return ImmUnlockIMC(real_imc);
    else
        return FALSE;
}

static void GenerateIMEMessage(HIMC hIMC, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LPINPUTCONTEXT lpIMC;
    LPTRANSMSG lpTransMsg;

    lpIMC = LockRealIMC(hIMC);
    if (lpIMC == NULL)
        return;

    lpIMC->hMsgBuf = ImmReSizeIMCC(lpIMC->hMsgBuf, (lpIMC->dwNumMsgBuf + 1) * sizeof(TRANSMSG));
    if (!lpIMC->hMsgBuf)
        return;

    lpTransMsg = ImmLockIMCC(lpIMC->hMsgBuf);
    if (!lpTransMsg)
        return;

    lpTransMsg += lpIMC->dwNumMsgBuf;
    lpTransMsg->message = msg;
    lpTransMsg->wParam = wParam;
    lpTransMsg->lParam = lParam;

    ImmUnlockIMCC(lpIMC->hMsgBuf);
    lpIMC->dwNumMsgBuf++;

    ImmGenerateMessage(RealIMC(hIMC));
    UnlockRealIMC(hIMC);
}

static BOOL IME_RemoveFromSelected(HIMC hIMC)
{
    int i;
    for (i = 0; i < hSelectedCount; i++)
    {
        if (hSelectedFrom[i] == hIMC)
        {
            if (i < hSelectedCount - 1)
                memmove(&hSelectedFrom[i], &hSelectedFrom[i + 1], (hSelectedCount - i - 1) * sizeof(HIMC));
            hSelectedCount--;
            return TRUE;
        }
    }
    return FALSE;
}

static void IME_AddToSelected(HIMC hIMC)
{
    hSelectedCount++;
    if (hSelectedFrom)
        hSelectedFrom = HeapReAlloc(GetProcessHeap(), 0, hSelectedFrom, hSelectedCount * sizeof(HIMC));
    else
        hSelectedFrom = HeapAlloc(GetProcessHeap(), 0, sizeof(HIMC));
    hSelectedFrom[hSelectedCount - 1] = hIMC;
}

BOOL WINAPI ImeSelect(HIMC hIMC, BOOL fSelect)
{
    LPINPUTCONTEXT lpIMC;
    TRACE("%p %s\n", hIMC, fSelect ? "TRUE" : "FALSE");

    if (hIMC == FROM_MACDRV)
    {
        ERR("ImeSelect should never be called from Cocoa\n");
        return FALSE;
    }

    if (!hIMC)
        return TRUE;

    /* not selected */
    if (!fSelect)
        return IME_RemoveFromSelected(hIMC);

    IME_AddToSelected(hIMC);

    /* Initialize our structures */
    lpIMC = LockRealIMC(hIMC);
    if (lpIMC != NULL)
    {
        LPIMEPRIVATE myPrivate;
        myPrivate = ImmLockIMCC(lpIMC->hPrivate);
        if (myPrivate->bInComposition)
            GenerateIMEMessage(hIMC, WM_IME_ENDCOMPOSITION, 0, 0);
        if (myPrivate->bInternalState)
            ImmSetOpenStatus(RealIMC(FROM_MACDRV), FALSE);
        myPrivate->bInComposition = FALSE;
        myPrivate->bInternalState = FALSE;
        myPrivate->textfont = NULL;
        myPrivate->hwndDefault = NULL;
        ImmUnlockIMCC(lpIMC->hPrivate);
        UnlockRealIMC(hIMC);
    }

    return TRUE;
}

/* Interfaces to other parts of the Mac driver */

/**************************************************************************
 *              macdrv_ime_query_char_rect
 */
NTSTATUS WINAPI macdrv_ime_query_char_rect(void *arg, ULONG size)
{
    struct ime_query_char_rect_params *params = arg;
    struct ime_query_char_rect_result *result = param_ptr(params->result);
    void *himc = param_ptr(params->himc);
    IMECHARPOSITION charpos;
    BOOL ret = FALSE;

    result->location = params->location;
    result->length = params->length;

    if (!himc) himc = RealIMC(FROM_MACDRV);

    charpos.dwSize = sizeof(charpos);
    charpos.dwCharPos = params->location;
    if (ImmRequestMessageW(himc, IMR_QUERYCHARPOSITION, (ULONG_PTR)&charpos))
    {
        int i;

        SetRect(&result->rect, charpos.pt.x, charpos.pt.y, 0, charpos.pt.y + charpos.cLineHeight);

        /* iterate over rest of length to extend rect */
        for (i = 1; i < params->length; i++)
        {
            charpos.dwSize = sizeof(charpos);
            charpos.dwCharPos = params->location + i;
            if (!ImmRequestMessageW(himc, IMR_QUERYCHARPOSITION, (ULONG_PTR)&charpos) ||
                charpos.pt.y != result->rect.top)
            {
                result->length = i;
                break;
            }

            result->rect.right = charpos.pt.x;
        }

        ret = TRUE;
    }

    if (!ret)
    {
        LPINPUTCONTEXT ic = ImmLockIMC(himc);

        if (ic)
        {
            WCHAR *str;
            HWND hwnd;
            UINT len;

            if ((hwnd = input_context_get_ui_hwnd( ic )) && IsWindowVisible( hwnd ) &&
                (str = input_context_get_comp_str( ic, FALSE, &len )))
            {
                HDC dc = GetDC( hwnd );
                HFONT font = input_context_select_ui_font( ic, dc );
                SIZE size;

                if (result->location > len) result->location = len;
                if (result->location + result->length > len) result->length = len - result->location;

                GetTextExtentPoint32W( dc, str, result->location, &size );
                charpos.rcDocument.left = size.cx;
                charpos.rcDocument.top = 0;
                GetTextExtentPoint32W( dc, str, result->location + result->length, &size );
                charpos.rcDocument.right = size.cx;
                charpos.rcDocument.bottom = size.cy;

                if (ic->cfCompForm.dwStyle == CFS_DEFAULT)
                    OffsetRect(&charpos.rcDocument, 10, 10);

                LPtoDP(dc, (POINT*)&charpos.rcDocument, 2);
                MapWindowPoints( hwnd, 0, (POINT *)&charpos.rcDocument, 2 );
                result->rect = charpos.rcDocument;
                ret = TRUE;

                if (font) SelectObject( dc, font );
                ReleaseDC( hwnd, dc );
                free( str );
            }
        }

        ImmUnlockIMC(himc);
    }

    if (!ret)
    {
        GUITHREADINFO gti;
        gti.cbSize = sizeof(gti);
        if (GetGUIThreadInfo(0, &gti))
        {
            MapWindowPoints(gti.hwndCaret, 0, (POINT*)&gti.rcCaret, 2);
            result->rect = gti.rcCaret;
            ret = TRUE;
        }
    }

    if (ret && result->length && result->rect.left == result->rect.right)
        result->rect.right++;

    return ret;
}
