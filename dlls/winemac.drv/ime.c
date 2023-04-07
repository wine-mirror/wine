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

static HIMCC ImeCreateBlankCompStr(void)
{
    HIMCC rc;
    LPCOMPOSITIONSTRING ptr;
    rc = ImmCreateIMCC(sizeof(COMPOSITIONSTRING));
    ptr = ImmLockIMCC(rc);
    memset(ptr, 0, sizeof(COMPOSITIONSTRING));
    ptr->dwSize = sizeof(COMPOSITIONSTRING);
    ImmUnlockIMCC(rc);
    return rc;
}

static int updateField(DWORD origLen, DWORD origOffset, DWORD currentOffset,
                       LPBYTE target, LPBYTE source, DWORD* lenParam,
                       DWORD* offsetParam, BOOL wchars)
{
     if (origLen > 0 && origOffset > 0)
     {
        int truelen = origLen;
        if (wchars)
            truelen *= sizeof(WCHAR);

        memcpy(&target[currentOffset], &source[origOffset], truelen);

        *lenParam = origLen;
        *offsetParam = currentOffset;
        currentOffset += truelen;
     }
     return currentOffset;
}

static HIMCC updateCompStr(HIMCC old, LPCWSTR compstr, DWORD len, DWORD *flags)
{
    /* We need to make sure the CompStr, CompClause and CompAttr fields are all
     * set and correct. */
    int needed_size;
    HIMCC   rc;
    LPBYTE newdata = NULL;
    LPBYTE olddata = NULL;
    LPCOMPOSITIONSTRING new_one;
    LPCOMPOSITIONSTRING lpcs = NULL;
    INT current_offset = 0;

    TRACE("%s, %li\n", debugstr_wn(compstr, len), len);

    if (old == NULL && compstr == NULL && len == 0)
        return NULL;

    if (compstr == NULL && len != 0)
    {
        ERR("compstr is NULL however we have a len!  Please report\n");
        len = 0;
    }

    if (old != NULL)
    {
        olddata = ImmLockIMCC(old);
        lpcs = (LPCOMPOSITIONSTRING)olddata;
    }

    needed_size = sizeof(COMPOSITIONSTRING) + len * sizeof(WCHAR) +
                  len + sizeof(DWORD) * 2;

    if (lpcs != NULL)
    {
        needed_size += lpcs->dwCompReadAttrLen;
        needed_size += lpcs->dwCompReadClauseLen;
        needed_size += lpcs->dwCompReadStrLen * sizeof(WCHAR);
        needed_size += lpcs->dwResultReadClauseLen;
        needed_size += lpcs->dwResultReadStrLen * sizeof(WCHAR);
        needed_size += lpcs->dwResultClauseLen;
        needed_size += lpcs->dwResultStrLen * sizeof(WCHAR);
        needed_size += lpcs->dwPrivateSize;
    }
    rc = ImmCreateIMCC(needed_size);
    newdata = ImmLockIMCC(rc);
    new_one = (LPCOMPOSITIONSTRING)newdata;

    new_one->dwSize = needed_size;
    current_offset = sizeof(COMPOSITIONSTRING);
    if (lpcs != NULL)
    {
        current_offset = updateField(lpcs->dwCompReadAttrLen,
                                     lpcs->dwCompReadAttrOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwCompReadAttrLen,
                                     &new_one->dwCompReadAttrOffset, FALSE);

        current_offset = updateField(lpcs->dwCompReadClauseLen,
                                     lpcs->dwCompReadClauseOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwCompReadClauseLen,
                                     &new_one->dwCompReadClauseOffset, FALSE);

        current_offset = updateField(lpcs->dwCompReadStrLen,
                                     lpcs->dwCompReadStrOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwCompReadStrLen,
                                     &new_one->dwCompReadStrOffset, TRUE);

        /* new CompAttr, CompClause, CompStr, dwCursorPos */
        new_one->dwDeltaStart = 0;
        new_one->dwCursorPos = lpcs->dwCursorPos;

        current_offset = updateField(lpcs->dwResultReadClauseLen,
                                     lpcs->dwResultReadClauseOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwResultReadClauseLen,
                                     &new_one->dwResultReadClauseOffset, FALSE);

        current_offset = updateField(lpcs->dwResultReadStrLen,
                                     lpcs->dwResultReadStrOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwResultReadStrLen,
                                     &new_one->dwResultReadStrOffset, TRUE);

        current_offset = updateField(lpcs->dwResultClauseLen,
                                     lpcs->dwResultClauseOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwResultClauseLen,
                                     &new_one->dwResultClauseOffset, FALSE);

        current_offset = updateField(lpcs->dwResultStrLen,
                                     lpcs->dwResultStrOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwResultStrLen,
                                     &new_one->dwResultStrOffset, TRUE);

        current_offset = updateField(lpcs->dwPrivateSize,
                                     lpcs->dwPrivateOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwPrivateSize,
                                     &new_one->dwPrivateOffset, FALSE);
    }
    else
    {
        new_one->dwCursorPos = len;
        *flags |= GCS_CURSORPOS;
    }

    /* set new data */
    /* CompAttr */
    new_one->dwCompAttrLen = len;
    if (len > 0)
    {
        new_one->dwCompAttrOffset = current_offset;
        memset(&newdata[current_offset], ATTR_INPUT, len);
        current_offset += len;
    }

    /* CompClause */
    if (len > 0)
    {
        new_one->dwCompClauseLen = sizeof(DWORD) * 2;
        new_one->dwCompClauseOffset = current_offset;
        *(DWORD*)&newdata[current_offset] = 0;
        current_offset += sizeof(DWORD);
        *(DWORD*)&newdata[current_offset] = len;
        current_offset += sizeof(DWORD);
    }
    else
        new_one->dwCompClauseLen = 0;

    /* CompStr */
    new_one->dwCompStrLen = len;
    if (len > 0)
    {
        new_one->dwCompStrOffset = current_offset;
        memcpy(&newdata[current_offset], compstr, len * sizeof(WCHAR));
    }


    ImmUnlockIMCC(rc);
    if (lpcs)
        ImmUnlockIMCC(old);

    return rc;
}

static HIMCC updateResultStr(HIMCC old, LPWSTR resultstr, DWORD len)
{
    /* we need to make sure the ResultStr and ResultClause fields are all
     * set and correct */
    int needed_size;
    HIMCC rc;
    LPBYTE newdata = NULL;
    LPBYTE olddata = NULL;
    LPCOMPOSITIONSTRING new_one;
    LPCOMPOSITIONSTRING lpcs = NULL;
    INT current_offset = 0;

    TRACE("%s, %li\n", debugstr_wn(resultstr, len), len);

    if (old == NULL && resultstr == NULL && len == 0)
        return NULL;

    if (resultstr == NULL && len != 0)
    {
        ERR("resultstr is NULL however we have a len!  Please report\n");
        len = 0;
    }

    if (old != NULL)
    {
        olddata = ImmLockIMCC(old);
        lpcs = (LPCOMPOSITIONSTRING)olddata;
    }

    needed_size = sizeof(COMPOSITIONSTRING) + len * sizeof(WCHAR) +
                  sizeof(DWORD) * 2;

    if (lpcs != NULL)
    {
        needed_size += lpcs->dwCompReadAttrLen;
        needed_size += lpcs->dwCompReadClauseLen;
        needed_size += lpcs->dwCompReadStrLen * sizeof(WCHAR);
        needed_size += lpcs->dwCompAttrLen;
        needed_size += lpcs->dwCompClauseLen;
        needed_size += lpcs->dwCompStrLen * sizeof(WCHAR);
        needed_size += lpcs->dwResultReadClauseLen;
        needed_size += lpcs->dwResultReadStrLen * sizeof(WCHAR);
        needed_size += lpcs->dwPrivateSize;
    }
    rc = ImmCreateIMCC(needed_size);
    newdata = ImmLockIMCC(rc);
    new_one = (LPCOMPOSITIONSTRING)newdata;

    new_one->dwSize = needed_size;
    current_offset = sizeof(COMPOSITIONSTRING);
    if (lpcs != NULL)
    {
        current_offset = updateField(lpcs->dwCompReadAttrLen,
                                     lpcs->dwCompReadAttrOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwCompReadAttrLen,
                                     &new_one->dwCompReadAttrOffset, FALSE);

        current_offset = updateField(lpcs->dwCompReadClauseLen,
                                     lpcs->dwCompReadClauseOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwCompReadClauseLen,
                                     &new_one->dwCompReadClauseOffset, FALSE);

        current_offset = updateField(lpcs->dwCompReadStrLen,
                                     lpcs->dwCompReadStrOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwCompReadStrLen,
                                     &new_one->dwCompReadStrOffset, TRUE);

        current_offset = updateField(lpcs->dwCompAttrLen,
                                     lpcs->dwCompAttrOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwCompAttrLen,
                                     &new_one->dwCompAttrOffset, FALSE);

        current_offset = updateField(lpcs->dwCompClauseLen,
                                     lpcs->dwCompClauseOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwCompClauseLen,
                                     &new_one->dwCompClauseOffset, FALSE);

        current_offset = updateField(lpcs->dwCompStrLen,
                                     lpcs->dwCompStrOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwCompStrLen,
                                     &new_one->dwCompStrOffset, TRUE);

        new_one->dwCursorPos = lpcs->dwCursorPos;
        new_one->dwDeltaStart = 0;

        current_offset = updateField(lpcs->dwResultReadClauseLen,
                                     lpcs->dwResultReadClauseOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwResultReadClauseLen,
                                     &new_one->dwResultReadClauseOffset, FALSE);

        current_offset = updateField(lpcs->dwResultReadStrLen,
                                     lpcs->dwResultReadStrOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwResultReadStrLen,
                                     &new_one->dwResultReadStrOffset, TRUE);

        /* new ResultClause , ResultStr */

        current_offset = updateField(lpcs->dwPrivateSize,
                                     lpcs->dwPrivateOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwPrivateSize,
                                     &new_one->dwPrivateOffset, FALSE);
    }

    /* set new data */
    /* ResultClause */
    if (len > 0)
    {
        new_one->dwResultClauseLen = sizeof(DWORD) * 2;
        new_one->dwResultClauseOffset = current_offset;
        *(DWORD*)&newdata[current_offset] = 0;
        current_offset += sizeof(DWORD);
        *(DWORD*)&newdata[current_offset] = len;
        current_offset += sizeof(DWORD);
    }
    else
        new_one->dwResultClauseLen = 0;

    /* ResultStr */
    new_one->dwResultStrLen = len;
    if (len > 0)
    {
        new_one->dwResultStrOffset = current_offset;
        memcpy(&newdata[current_offset], resultstr, len * sizeof(WCHAR));
    }
    ImmUnlockIMCC(rc);
    if (lpcs)
        ImmUnlockIMCC(old);

    return rc;
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

static BOOL GenerateMessageToTransKey(TRANSMSGLIST *lpTransBuf, UINT *uNumTranMsgs,
                                      UINT msg, WPARAM wParam, LPARAM lParam)
{
    LPTRANSMSG ptr;

    if (*uNumTranMsgs + 1 >= lpTransBuf->uMsgCount)
        return FALSE;

    ptr = lpTransBuf->TransMsg + *uNumTranMsgs;
    ptr->message = msg;
    ptr->wParam = wParam;
    ptr->lParam = lParam;
    (*uNumTranMsgs)++;

    return TRUE;
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

static void UpdateDataInDefaultIMEWindow(INPUTCONTEXT *lpIMC, HWND hwnd, BOOL showable)
{
    LPCOMPOSITIONSTRING compstr;

    if (lpIMC->hCompStr)
        compstr = ImmLockIMCC(lpIMC->hCompStr);
    else
        compstr = NULL;

    if (compstr == NULL || compstr->dwCompStrLen == 0)
        ShowWindow(hwnd, SW_HIDE);
    else if (showable)
        ShowWindow(hwnd, SW_SHOWNOACTIVATE);

    RedrawWindow(hwnd, NULL, NULL, RDW_ERASENOW | RDW_INVALIDATE);

    if (compstr != NULL)
        ImmUnlockIMCC(lpIMC->hCompStr);
}

BOOL WINAPI ImeProcessKey(HIMC hIMC, UINT vKey, LPARAM lKeyData, const LPBYTE lpbKeyState)
{
    LPINPUTCONTEXT lpIMC;
    BOOL inIME;

    TRACE("hIMC %p vKey 0x%04x lKeyData 0x%08Ix lpbKeyState %p\n", hIMC, vKey, lKeyData, lpbKeyState);

    switch (vKey)
    {
        case VK_SHIFT:
        case VK_CONTROL:
        case VK_CAPITAL:
        case VK_MENU:
        return FALSE;
    }

    inIME = MACDRV_CALL(ime_using_input_method, NULL);
    lpIMC = LockRealIMC(hIMC);
    if (lpIMC)
    {
        LPIMEPRIVATE myPrivate;
        HWND hwnd = input_context_get_ui_hwnd( lpIMC );
        myPrivate = ImmLockIMCC(lpIMC->hPrivate);

        if (inIME && !myPrivate->bInternalState)
            ImmSetOpenStatus(RealIMC(FROM_MACDRV), TRUE);
        else if (!inIME && myPrivate->bInternalState)
        {
            ShowWindow( hwnd, SW_HIDE );
            ImmDestroyIMCC(lpIMC->hCompStr);
            lpIMC->hCompStr = ImeCreateBlankCompStr();
            ImmSetOpenStatus(RealIMC(FROM_MACDRV), FALSE);
        }

        myPrivate->repeat = (lKeyData >> 30) & 0x1;

        myPrivate->bInternalState = inIME;
        ImmUnlockIMCC(lpIMC->hPrivate);
    }
    UnlockRealIMC(hIMC);

    return inIME;
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
        myPrivate->repeat = 0;
        ImmUnlockIMCC(lpIMC->hPrivate);
        UnlockRealIMC(hIMC);
    }

    return TRUE;
}

UINT WINAPI ImeToAsciiEx(UINT uVKey, UINT uScanCode, const LPBYTE lpbKeyState,
                         TRANSMSGLIST *lpdwTransKey, UINT fuState, HIMC hIMC)
{
    struct process_text_input_params params;
    UINT vkey;
    LPINPUTCONTEXT lpIMC;
    LPIMEPRIVATE myPrivate;
    HWND hwnd;
    UINT repeat;
    int done = 0;

    TRACE("uVKey 0x%04x uScanCode 0x%04x fuState %u hIMC %p\n", uVKey, uScanCode, fuState, hIMC);

    vkey = LOWORD(uVKey);

    if (vkey == VK_KANA || vkey == VK_KANJI || vkey == VK_MENU)
    {
        TRACE("Skipping metakey\n");
        return 0;
    }

    lpIMC = LockRealIMC(hIMC);
    hwnd = input_context_get_ui_hwnd( lpIMC );
    myPrivate = ImmLockIMCC(lpIMC->hPrivate);
    if (!myPrivate->bInternalState)
    {
        ImmUnlockIMCC(lpIMC->hPrivate);
        UnlockRealIMC(hIMC);
        return 0;
    }

    repeat = myPrivate->repeat;
    ImmUnlockIMCC(lpIMC->hPrivate);
    UnlockRealIMC(hIMC);

    TRACE("Processing Mac 0x%04x\n", vkey);
    params.himc = HandleToULong(hIMC);
    params.vkey = uVKey;
    params.scan = uScanCode;
    params.repeat = repeat;
    params.key_state = lpbKeyState;
    params.done = &done;
    MACDRV_CALL(ime_process_text_input, &params);

    while (!done)
        MsgWaitForMultipleObjectsEx(0, NULL, INFINITE, QS_POSTMESSAGE | QS_SENDMESSAGE, 0);

    if (done < 0)
    {
        UINT msgs = 0;
        UINT msg = (uScanCode & 0x8000) ? WM_KEYUP : WM_KEYDOWN;

        /* KeyStroke not processed by the IME
         * so we need to rebuild the KeyDown message and pass it on to WINE
         */
        if (!GenerateMessageToTransKey(lpdwTransKey, &msgs, msg, vkey, MAKELONG(0x0001, uScanCode)))
            GenerateIMEMessage(hIMC, msg, vkey, MAKELONG(0x0001, uScanCode));

        return msgs;
    }
    else if ((lpIMC = LockRealIMC(hIMC)))
    {
        UpdateDataInDefaultIMEWindow( lpIMC, hwnd, FALSE );
        UnlockRealIMC(hIMC);
    }
    return 0;
}

BOOL WINAPI NotifyIME(HIMC hIMC, DWORD dwAction, DWORD dwIndex, DWORD dwValue)
{
    BOOL bRet = FALSE;
    LPINPUTCONTEXT lpIMC;

    TRACE("%p %li %li %li\n", hIMC, dwAction, dwIndex, dwValue);

    lpIMC = LockRealIMC(hIMC);
    if (lpIMC == NULL)
        return FALSE;

    switch (dwAction)
    {
        case NI_OPENCANDIDATE: FIXME("NI_OPENCANDIDATE\n"); break;
        case NI_CLOSECANDIDATE: FIXME("NI_CLOSECANDIDATE\n"); break;
        case NI_SELECTCANDIDATESTR: FIXME("NI_SELECTCANDIDATESTR\n"); break;
        case NI_CHANGECANDIDATELIST: FIXME("NI_CHANGECANDIDATELIST\n"); break;
        case NI_SETCANDIDATE_PAGESTART: FIXME("NI_SETCANDIDATE_PAGESTART\n"); break;
        case NI_SETCANDIDATE_PAGESIZE: FIXME("NI_SETCANDIDATE_PAGESIZE\n"); break;
        case NI_CONTEXTUPDATED:
            switch (dwValue)
            {
                case IMC_SETCOMPOSITIONWINDOW: FIXME("NI_CONTEXTUPDATED: IMC_SETCOMPOSITIONWINDOW\n"); break;
                case IMC_SETCONVERSIONMODE: FIXME("NI_CONTEXTUPDATED: IMC_SETCONVERSIONMODE\n"); break;
                case IMC_SETSENTENCEMODE: FIXME("NI_CONTEXTUPDATED: IMC_SETSENTENCEMODE\n"); break;
                case IMC_SETCANDIDATEPOS: FIXME("NI_CONTEXTUPDATED: IMC_SETCANDIDATEPOS\n"); break;
                case IMC_SETCOMPOSITIONFONT:
                    {
                        LPIMEPRIVATE myPrivate;
                        TRACE("NI_CONTEXTUPDATED: IMC_SETCOMPOSITIONFONT\n");

                        myPrivate = ImmLockIMCC(lpIMC->hPrivate);
                        if (myPrivate->textfont)
                        {
                            DeleteObject(myPrivate->textfont);
                            myPrivate->textfont = NULL;
                        }
                        myPrivate->textfont = CreateFontIndirectW(&lpIMC->lfFont.W);
                        ImmUnlockIMCC(lpIMC->hPrivate);
                    }
                    break;
                case IMC_SETOPENSTATUS:
                {
                    LPIMEPRIVATE myPrivate;
                    TRACE("NI_CONTEXTUPDATED: IMC_SETOPENSTATUS\n");

                    myPrivate = ImmLockIMCC(lpIMC->hPrivate);
                    if (lpIMC->fOpen != myPrivate->bInternalState && myPrivate->bInComposition)
                    {
                        if(lpIMC->fOpen == FALSE)
                        {
                            GenerateIMEMessage(hIMC, WM_IME_ENDCOMPOSITION, 0, 0);
                            myPrivate->bInComposition = FALSE;
                        }
                        else
                        {
                            GenerateIMEMessage(hIMC, WM_IME_STARTCOMPOSITION, 0, 0);
                            GenerateIMEMessage(hIMC, WM_IME_COMPOSITION, 0, 0);
                        }
                    }
                    myPrivate->bInternalState = lpIMC->fOpen;
                    bRet = TRUE;
                }
                break;
                default: FIXME("NI_CONTEXTUPDATED: Unknown\n"); break;
            }
            break;
        case NI_COMPOSITIONSTR:
            switch (dwIndex)
            {
                case CPS_COMPLETE:
                {
                    HIMCC newCompStr;
                    LPIMEPRIVATE myPrivate;
                    WCHAR *str;
                    UINT len;

                    TRACE("NI_COMPOSITIONSTR: CPS_COMPLETE\n");

                    /* clear existing result */
                    newCompStr = updateResultStr(lpIMC->hCompStr, NULL, 0);

                    ImmDestroyIMCC(lpIMC->hCompStr);
                    lpIMC->hCompStr = newCompStr;

                    myPrivate = ImmLockIMCC(lpIMC->hPrivate);
                    if ((str = input_context_get_comp_str( lpIMC, FALSE, &len )))
                    {
                        WCHAR param = str[0];
                        DWORD flags = GCS_COMPSTR;

                        newCompStr = updateResultStr( lpIMC->hCompStr, str, len );
                        ImmDestroyIMCC(lpIMC->hCompStr);
                        lpIMC->hCompStr = newCompStr;
                        newCompStr = updateCompStr(lpIMC->hCompStr, NULL, 0, &flags);
                        ImmDestroyIMCC(lpIMC->hCompStr);
                        lpIMC->hCompStr = newCompStr;

                        GenerateIMEMessage(hIMC, WM_IME_COMPOSITION, 0, flags);

                        GenerateIMEMessage(hIMC, WM_IME_COMPOSITION, param,
                                           GCS_RESULTSTR | GCS_RESULTCLAUSE);

                        GenerateIMEMessage(hIMC, WM_IME_ENDCOMPOSITION, 0, 0);
                        free( str );
                    }
                    else if (myPrivate->bInComposition)
                        GenerateIMEMessage(hIMC, WM_IME_ENDCOMPOSITION, 0, 0);


                    myPrivate->bInComposition = FALSE;
                    ImmUnlockIMCC(lpIMC->hPrivate);

                    bRet = TRUE;
                }
                break;
                case CPS_CONVERT: FIXME("NI_COMPOSITIONSTR: CPS_CONVERT\n"); break;
                case CPS_REVERT: FIXME("NI_COMPOSITIONSTR: CPS_REVERT\n"); break;
                case CPS_CANCEL:
                {
                    LPIMEPRIVATE myPrivate;

                    TRACE("NI_COMPOSITIONSTR: CPS_CANCEL\n");

                    MACDRV_CALL(ime_clear, NULL);
                    if (lpIMC->hCompStr)
                        ImmDestroyIMCC(lpIMC->hCompStr);

                    lpIMC->hCompStr = ImeCreateBlankCompStr();

                    myPrivate = ImmLockIMCC(lpIMC->hPrivate);
                    if (myPrivate->bInComposition)
                    {
                        GenerateIMEMessage(hIMC, WM_IME_ENDCOMPOSITION, 0, 0);
                        myPrivate->bInComposition = FALSE;
                    }
                    ImmUnlockIMCC(lpIMC->hPrivate);
                    bRet = TRUE;
                }
                break;
                default: FIXME("NI_COMPOSITIONSTR: Unknown\n"); break;
            }
            break;
        default: FIXME("Unknown Message\n"); break;
    }

    UnlockRealIMC(hIMC);
    return bRet;
}

static BOOL IME_SetCompositionString(void* hIMC, DWORD dwIndex, LPCVOID lpComp, DWORD dwCompLen, DWORD cursor_pos, BOOL cursor_valid)
{
    LPINPUTCONTEXT lpIMC;
    DWORD flags = 0;
    WCHAR wParam  = 0;
    LPIMEPRIVATE myPrivate;
    BOOL sendMessage = TRUE;

    TRACE("(%p, %ld, %p, %ld):\n", hIMC, dwIndex, lpComp, dwCompLen);

    /*
     * Explanation:
     *  this sets the composition string in the imm32.dll level
     *  of the composition buffer.
     * TODO: set the Cocoa window's marked text string and tell text input context
     */

    lpIMC = LockRealIMC(hIMC);

    if (lpIMC == NULL)
        return FALSE;

    myPrivate = ImmLockIMCC(lpIMC->hPrivate);

    if (dwIndex == SCS_SETSTR)
    {
        HIMCC newCompStr;

        if (!myPrivate->bInComposition)
        {
            GenerateIMEMessage(hIMC, WM_IME_STARTCOMPOSITION, 0, 0);
            myPrivate->bInComposition = TRUE;
        }

        /* clear existing result */
        newCompStr = updateResultStr(lpIMC->hCompStr, NULL, 0);
        ImmDestroyIMCC(lpIMC->hCompStr);
        lpIMC->hCompStr = newCompStr;

        flags = GCS_COMPSTR;

        if (dwCompLen && lpComp)
        {
            newCompStr = updateCompStr(lpIMC->hCompStr, (LPCWSTR)lpComp, dwCompLen / sizeof(WCHAR), &flags);
            ImmDestroyIMCC(lpIMC->hCompStr);
            lpIMC->hCompStr = newCompStr;

            wParam = ((const WCHAR*)lpComp)[0];
            flags |= GCS_COMPCLAUSE | GCS_COMPATTR | GCS_DELTASTART;

            if (cursor_valid)
            {
                LPCOMPOSITIONSTRING compstr;
                compstr = ImmLockIMCC(lpIMC->hCompStr);
                compstr->dwCursorPos = cursor_pos;
                ImmUnlockIMCC(lpIMC->hCompStr);
                flags |= GCS_CURSORPOS;
            }
        }
        else
        {
            NotifyIME(hIMC, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
            sendMessage = FALSE;
        }

    }

    if (sendMessage) {
        GenerateIMEMessage(hIMC, WM_IME_COMPOSITION, wParam, flags);
        ImmUnlockIMCC(lpIMC->hPrivate);
        UnlockRealIMC(hIMC);
    }

    return TRUE;
}

BOOL WINAPI ImeSetCompositionString(HIMC hIMC, DWORD dwIndex, LPCVOID lpComp, DWORD dwCompLen,
                                    LPCVOID lpRead, DWORD dwReadLen)
{
    TRACE("(%p, %ld, %p, %ld, %p, %ld):\n", hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen);

    if (lpRead && dwReadLen)
        FIXME("Reading string unimplemented\n");

    return IME_SetCompositionString(hIMC, dwIndex, lpComp, dwCompLen, 0, FALSE);
}

static void IME_NotifyComplete(void* hIMC)
{
    NotifyIME(hIMC, NI_COMPOSITIONSTR, CPS_COMPLETE, 0);
}

/* Interfaces to other parts of the Mac driver */

/***********************************************************************
 *              macdrv_ime_set_text
 */
NTSTATUS WINAPI macdrv_ime_set_text(void *arg, ULONG size)
{
    struct ime_set_text_params *params = arg;
    ULONG length = (size - offsetof(struct ime_set_text_params, text)) / sizeof(WCHAR);
    void *himc = param_ptr(params->himc);
    HWND hwnd = UlongToHandle(params->hwnd);

    if (!himc) himc = RealIMC(FROM_MACDRV);

    if (length)
    {
        if (himc)
            IME_SetCompositionString(himc, SCS_SETSTR, params->text, length * sizeof(WCHAR),
                                     params->cursor_pos, !params->complete);
        else
        {
            INPUT input;
            unsigned int i;

            input.type              = INPUT_KEYBOARD;
            input.ki.wVk            = 0;
            input.ki.time           = 0;
            input.ki.dwExtraInfo    = 0;

            for (i = 0; i < length; i++)
            {
                input.ki.wScan      = params->text[i];
                input.ki.dwFlags    = KEYEVENTF_UNICODE;
                __wine_send_input(hwnd, &input, NULL);

                input.ki.dwFlags    = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
                __wine_send_input(hwnd, &input, NULL);
            }
        }
    }

    if (params->complete)
        IME_NotifyComplete(himc);
    return 0;
}

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
