/*
 * The IME for interfacing with XIM
 *
 * Copyright 2008 CodeWeavers, Aric Stewart
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
 *
 *  This flow does not work well for the X11 driver and XIM.
 *   (It works fine for Mac)
 *  As such we will have to reroute step 1.  Instead the x11drv driver will
 *  generate an XIM events and call directly into this IME implementation.
 *  As such we will have to use the alternative ImmGenerateMessage path to be
 *  generate the messages that we want the IMM layer to send to the application.
 */

#include "x11drv_dll.h"
#include "wine/debug.h"
#include "imm.h"
#include "immdev.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);

#define FROM_X11 ((HIMC)0xcafe1337)

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

static HIMC RealIMC(HIMC hIMC)
{
    if (hIMC == FROM_X11)
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
    memset(ptr,0,sizeof(COMPOSITIONSTRING));
    ptr->dwSize = sizeof(COMPOSITIONSTRING);
    ImmUnlockIMCC(rc);
    return rc;
}

static int updateField(DWORD origLen, DWORD origOffset, DWORD currentOffset,
                       LPBYTE target, LPBYTE source, DWORD* lenParam,
                       DWORD* offsetParam, BOOL wchars )
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

static HIMCC updateCompStr(HIMCC old, LPCWSTR compstr, DWORD len)
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

    TRACE("%s, %li\n",debugstr_wn(compstr,len),len);

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

    /* set new data */
    /* CompAttr */
    new_one->dwCompAttrLen = len;
    if (len > 0)
    {
        new_one->dwCompAttrOffset = current_offset;
        memset(&newdata[current_offset],ATTR_INPUT,len);
        current_offset += len;
    }

    /* CompClause */
    if (len > 0)
    {
        new_one->dwCompClauseLen = sizeof(DWORD) * 2;
        new_one->dwCompClauseOffset = current_offset;
        *(DWORD*)(&newdata[current_offset]) = 0;
        current_offset += sizeof(DWORD);
        *(DWORD*)(&newdata[current_offset]) = len;
        current_offset += sizeof(DWORD);
    }
    else
        new_one->dwCompClauseLen = 0;

    /* CompStr */
    new_one->dwCompStrLen = len;
    if (len > 0)
    {
        new_one->dwCompStrOffset = current_offset;
        memcpy(&newdata[current_offset],compstr,len*sizeof(WCHAR));
    }

    /* CursorPos */
    new_one->dwCursorPos = len;

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
    HIMCC   rc;
    LPBYTE newdata = NULL;
    LPBYTE olddata = NULL;
    LPCOMPOSITIONSTRING new_one;
    LPCOMPOSITIONSTRING lpcs = NULL;
    INT current_offset = 0;

    TRACE("%s, %li\n",debugstr_wn(resultstr,len),len);

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
        *(DWORD*)(&newdata[current_offset]) = 0;
        current_offset += sizeof(DWORD);
        *(DWORD*)(&newdata[current_offset]) = len;
        current_offset += sizeof(DWORD);
    }
    else
        new_one->dwResultClauseLen = 0;

    /* ResultStr */
    new_one->dwResultStrLen = len;
    if (len > 0)
    {
        new_one->dwResultStrOffset = current_offset;
        memcpy(&newdata[current_offset],resultstr,len*sizeof(WCHAR));
    }
    ImmUnlockIMCC(rc);
    if (lpcs)
        ImmUnlockIMCC(old);

    return rc;
}

static void GenerateIMEMessage(HIMC hIMC, UINT msg, WPARAM wParam,
                               LPARAM lParam)
{
    LPINPUTCONTEXT lpIMC;
    LPTRANSMSG lpTransMsg;

    lpIMC = LockRealIMC(hIMC);
    if (lpIMC == NULL)
        return;

    lpIMC->hMsgBuf = ImmReSizeIMCC(lpIMC->hMsgBuf, (lpIMC->dwNumMsgBuf + 1) *
                                    sizeof(TRANSMSG));
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
        if (hSelectedFrom[i] == hIMC)
        {
            if (i < hSelectedCount - 1)
                memmove(&hSelectedFrom[i], &hSelectedFrom[i+1], (hSelectedCount - i - 1)*sizeof(HIMC));
            hSelectedCount --;
            return TRUE;
        }
    return FALSE;
}

static void IME_AddToSelected(HIMC hIMC)
{
    hSelectedCount++;
    if (hSelectedFrom)
        hSelectedFrom = HeapReAlloc(GetProcessHeap(), 0, hSelectedFrom, hSelectedCount*sizeof(HIMC));
    else
        hSelectedFrom = HeapAlloc(GetProcessHeap(), 0, sizeof(HIMC));
    hSelectedFrom[hSelectedCount-1] = hIMC;
}

BOOL WINAPI ImeProcessKey(HIMC hIMC, UINT vKey, LPARAM lKeyData, const LPBYTE lpbKeyState)
{
    /* See the comment at the head of this file */
    TRACE("We do no processing via this route\n");
    return FALSE;
}

BOOL WINAPI ImeSelect(HIMC hIMC, BOOL fSelect)
{
    LPINPUTCONTEXT lpIMC;
    TRACE("%p %s\n",hIMC,(fSelect)?"TRUE":"FALSE");

    if (hIMC == FROM_X11)
    {
        ERR("ImeSelect should never be called from X11\n");
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
        myPrivate->bInComposition = FALSE;
        myPrivate->bInternalState = FALSE;
        myPrivate->textfont = NULL;
        myPrivate->hwndDefault = NULL;
        ImmUnlockIMCC(lpIMC->hPrivate);
        UnlockRealIMC(hIMC);
    }

    return TRUE;
}

UINT WINAPI ImeToAsciiEx (UINT uVKey, UINT uScanCode, const LPBYTE lpbKeyState,
                          TRANSMSGLIST *lpdwTransKey, UINT fuState, HIMC hIMC)
{
    /* See the comment at the head of this file */
    TRACE("We do no processing via this route\n");
    return 0;
}

BOOL WINAPI NotifyIME(HIMC hIMC, DWORD dwAction, DWORD dwIndex, DWORD dwValue)
{
    struct xim_preedit_state_params preedit_params;
    BOOL bRet = FALSE;
    LPINPUTCONTEXT lpIMC;

    TRACE("%p %li %li %li\n",hIMC,dwAction,dwIndex,dwValue);

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
                case IMC_SETCOMPOSITIONWINDOW: FIXME("IMC_SETCOMPOSITIONWINDOW\n"); break;
                case IMC_SETCONVERSIONMODE: FIXME("IMC_SETCONVERSIONMODE\n"); break;
                case IMC_SETSENTENCEMODE: FIXME("IMC_SETSENTENCEMODE\n"); break;
                case IMC_SETCANDIDATEPOS: FIXME("IMC_SETCANDIDATEPOS\n"); break;
                case IMC_SETCOMPOSITIONFONT:
                    {
                        LPIMEPRIVATE myPrivate;
                        TRACE("IMC_SETCOMPOSITIONFONT\n");

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
                    TRACE("IMC_SETOPENSTATUS\n");

                    bRet = TRUE;
                    preedit_params.hwnd = lpIMC->hWnd;
                    preedit_params.open = lpIMC->fOpen;
                    X11DRV_CALL( xim_preedit_state, &preedit_params );
                    if (!lpIMC->fOpen)
                    {
                        LPIMEPRIVATE myPrivate;

                        myPrivate = ImmLockIMCC(lpIMC->hPrivate);
                        if (myPrivate->bInComposition)
                        {
                            X11DRV_CALL( xim_reset, lpIMC->hWnd );
                            GenerateIMEMessage(hIMC, WM_IME_ENDCOMPOSITION, 0, 0);
                            myPrivate->bInComposition = FALSE;
                        }
                        ImmUnlockIMCC(lpIMC->hPrivate);
                    }

                    break;
                default: FIXME("Unknown\n"); break;
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

                    TRACE("CPS_COMPLETE\n");

                    /* clear existing result */
                    newCompStr = updateResultStr(lpIMC->hCompStr, NULL, 0);

                    ImmDestroyIMCC(lpIMC->hCompStr);
                    lpIMC->hCompStr = newCompStr;

                    myPrivate = ImmLockIMCC(lpIMC->hPrivate);
                    if ((str = input_context_get_comp_str( lpIMC, FALSE, &len )))
                    {
                        WCHAR param = str[0];

                        newCompStr = updateResultStr( lpIMC->hCompStr, str, len );
                        ImmDestroyIMCC(lpIMC->hCompStr);
                        lpIMC->hCompStr = newCompStr;
                        newCompStr = updateCompStr(lpIMC->hCompStr, NULL, 0);
                        ImmDestroyIMCC(lpIMC->hCompStr);
                        lpIMC->hCompStr = newCompStr;

                        GenerateIMEMessage(hIMC, WM_IME_COMPOSITION, 0,
                                                  GCS_COMPSTR);

                        GenerateIMEMessage(hIMC, WM_IME_COMPOSITION, param,
                                            GCS_RESULTSTR|GCS_RESULTCLAUSE);

                        GenerateIMEMessage(hIMC,WM_IME_ENDCOMPOSITION, 0, 0);
                        free( str );
                    }
                    else if (myPrivate->bInComposition)
                        GenerateIMEMessage(hIMC,WM_IME_ENDCOMPOSITION, 0, 0);

                    myPrivate->bInComposition = FALSE;
                    ImmUnlockIMCC(lpIMC->hPrivate);

                    bRet = TRUE;
                }
                break;
                case CPS_CONVERT: FIXME("CPS_CONVERT\n"); break;
                case CPS_REVERT: FIXME("CPS_REVERT\n"); break;
                case CPS_CANCEL:
                {
                    LPIMEPRIVATE myPrivate;

                    TRACE("CPS_CANCEL\n");

                    X11DRV_CALL( xim_reset, lpIMC->hWnd );

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
                default: FIXME("Unknown\n"); break;
            }
            break;
        default: FIXME("Unknown Message\n"); break;
    }

    UnlockRealIMC(hIMC);
    return bRet;
}

BOOL WINAPI ImeSetCompositionString(HIMC hIMC, DWORD dwIndex, LPCVOID lpComp,
                                    DWORD dwCompLen, LPCVOID lpRead,
                                    DWORD dwReadLen)
{
    LPINPUTCONTEXT lpIMC;
    DWORD flags = 0;
    WCHAR wParam  = 0;
    LPIMEPRIVATE myPrivate;

    TRACE("(%p, %ld, %p, %ld, %p, %ld):\n",
         hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen);


    if (hIMC != FROM_X11)
        FIXME("PROBLEM: This only sets the wine level string\n");

    /*
    * Explanation:
    *  this sets the composition string in the imm32.dll level
    *  of the composition buffer. we cannot manipulate the xim level
    *  buffer, which means that once the xim level buffer changes again
    *  any call to this function from the application will be lost
    */

    if (lpRead && dwReadLen)
        FIXME("Reading string unimplemented\n");

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
            newCompStr = updateCompStr(lpIMC->hCompStr, (LPCWSTR)lpComp, dwCompLen / sizeof(WCHAR));
            ImmDestroyIMCC(lpIMC->hCompStr);
            lpIMC->hCompStr = newCompStr;

             wParam = ((const WCHAR*)lpComp)[0];
             flags |= GCS_COMPCLAUSE | GCS_COMPATTR | GCS_DELTASTART;
        }
        else
        {
            newCompStr = updateCompStr(lpIMC->hCompStr, NULL, 0);
            ImmDestroyIMCC(lpIMC->hCompStr);
            lpIMC->hCompStr = newCompStr;
        }
    }

    GenerateIMEMessage(hIMC, WM_IME_COMPOSITION, wParam, flags);
    ImmUnlockIMCC(lpIMC->hPrivate);
    UnlockRealIMC(hIMC);

    return TRUE;
}

/* Interfaces to XIM and other parts of winex11drv */

NTSTATUS x11drv_ime_set_open_status( UINT open )
{
    HIMC imc;

    imc = RealIMC(FROM_X11);
    ImmSetOpenStatus(imc, open);
    return 0;
}

NTSTATUS x11drv_ime_set_composition_status( UINT open )
{
    HIMC imc;
    LPINPUTCONTEXT lpIMC;
    LPIMEPRIVATE myPrivate;

    imc = RealIMC(FROM_X11);
    lpIMC = ImmLockIMC(imc);
    if (lpIMC == NULL)
        return 0;

    myPrivate = ImmLockIMCC(lpIMC->hPrivate);

    if (open && !myPrivate->bInComposition)
    {
        GenerateIMEMessage(imc, WM_IME_STARTCOMPOSITION, 0, 0);
    }
    else if (!open && myPrivate->bInComposition)
    {
        ShowWindow(myPrivate->hwndDefault, SW_HIDE);
        ImmDestroyIMCC(lpIMC->hCompStr);
        lpIMC->hCompStr = ImeCreateBlankCompStr();
        GenerateIMEMessage(imc, WM_IME_ENDCOMPOSITION, 0, 0);
    }
    myPrivate->bInComposition = open;

    ImmUnlockIMCC(lpIMC->hPrivate);
    ImmUnlockIMC(imc);
    return 0;
}

NTSTATUS x11drv_ime_get_cursor_pos( UINT arg )
{
    LPINPUTCONTEXT lpIMC;
    INT rc = 0;
    LPCOMPOSITIONSTRING compstr;

    if (!hSelectedFrom)
        return rc;

    lpIMC = LockRealIMC(FROM_X11);
    if (lpIMC)
    {
        compstr = ImmLockIMCC(lpIMC->hCompStr);
        rc = compstr->dwCursorPos;
        ImmUnlockIMCC(lpIMC->hCompStr);
    }
    UnlockRealIMC(FROM_X11);
    return rc;
}

NTSTATUS x11drv_ime_set_cursor_pos( UINT pos )
{
    LPINPUTCONTEXT lpIMC;
    LPCOMPOSITIONSTRING compstr;

    if (!hSelectedFrom)
        return 0;

    lpIMC = LockRealIMC(FROM_X11);
    if (!lpIMC)
        return 0;

    compstr = ImmLockIMCC(lpIMC->hCompStr);
    if (!compstr)
    {
        UnlockRealIMC(FROM_X11);
        return 0;
    }

    compstr->dwCursorPos = pos;
    ImmUnlockIMCC(lpIMC->hCompStr);
    UnlockRealIMC(FROM_X11);
    GenerateIMEMessage(FROM_X11, WM_IME_COMPOSITION, pos, GCS_CURSORPOS);
    return 0;
}

NTSTATUS x11drv_ime_update_association( UINT arg )
{
    HWND focus = UlongToHandle( arg );

    ImmGetContext(focus);

    if (focus && hSelectedFrom)
        ImmAssociateContext(focus,RealIMC(FROM_X11));
    return 0;
}


NTSTATUS WINAPI x11drv_ime_set_composition_string( void *param, ULONG size )
{
    return ImeSetCompositionString(FROM_X11, SCS_SETSTR, param, size, NULL, 0);
}

NTSTATUS WINAPI x11drv_ime_set_result( void *params, ULONG len )
{
    WCHAR *lpResult = params;
    HIMC imc;
    LPINPUTCONTEXT lpIMC;
    HIMCC newCompStr;
    LPIMEPRIVATE myPrivate;
    BOOL inComp;
    HWND focus;

    len /= sizeof(WCHAR);
    if ((focus = GetFocus()))
        x11drv_ime_update_association( HandleToUlong( focus ));

    imc = RealIMC(FROM_X11);
    lpIMC = ImmLockIMC(imc);
    if (lpIMC == NULL)
        return 0;

    newCompStr = updateCompStr(lpIMC->hCompStr, NULL, 0);
    ImmDestroyIMCC(lpIMC->hCompStr);
    lpIMC->hCompStr = newCompStr;

    newCompStr = updateResultStr(lpIMC->hCompStr, lpResult, len);
    ImmDestroyIMCC(lpIMC->hCompStr);
    lpIMC->hCompStr = newCompStr;

    myPrivate = ImmLockIMCC(lpIMC->hPrivate);
    inComp = myPrivate->bInComposition;
    ImmUnlockIMCC(lpIMC->hPrivate);

    if (!inComp)
    {
        ImmSetOpenStatus(imc, TRUE);
        GenerateIMEMessage(imc, WM_IME_STARTCOMPOSITION, 0, 0);
    }

    GenerateIMEMessage(imc, WM_IME_COMPOSITION, 0, GCS_COMPSTR);
    GenerateIMEMessage(imc, WM_IME_COMPOSITION, lpResult[0], GCS_RESULTSTR|GCS_RESULTCLAUSE);
    GenerateIMEMessage(imc, WM_IME_ENDCOMPOSITION, 0, 0);

    if (!inComp)
        ImmSetOpenStatus(imc, FALSE);

    ImmUnlockIMC(imc);
    return 0;
}
