/*
 * Copyright (C) 2007 CodeWeavers, Aric Stewart
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

#ifndef _DDKIMM_H_
#define _DDKIMM_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _tagINPUTCONTEXT {
    HWND                hWnd;
    BOOL                fOpen;
    POINT               ptStatusWndPos;
    POINT               ptSoftKbdPos;
    DWORD               fdwConversion;
    DWORD               fdwSentence;
    union   {
        LOGFONTA        A;
        LOGFONTW        W;
    } lfFont;
    COMPOSITIONFORM     cfCompForm;
    CANDIDATEFORM       cfCandForm[4];
    HIMCC               hCompStr;
    HIMCC               hCandInfo;
    HIMCC               hGuideLine;
    HIMCC               hPrivate;
    DWORD               dwNumMsgBuf;
    HIMCC               hMsgBuf;
    DWORD               fdwInit;
    DWORD               dwReserve[3];
} INPUTCONTEXT, *LPINPUTCONTEXT;

typedef struct _tagIMEINFO {
    DWORD       dwPrivateDataSize;
    DWORD       fdwProperty;
    DWORD       fdwConversionCaps;
    DWORD       fdwSentenceCaps;
    DWORD       fdwUICaps;
    DWORD       fdwSCSCaps;
    DWORD       fdwSelectCaps;
} IMEINFO, *LPIMEINFO;

typedef struct tagCOMPOSITIONSTRING {
    DWORD dwSize;
    DWORD dwCompReadAttrLen;
    DWORD dwCompReadAttrOffset;
    DWORD dwCompReadClauseLen;
    DWORD dwCompReadClauseOffset;
    DWORD dwCompReadStrLen;
    DWORD dwCompReadStrOffset;
    DWORD dwCompAttrLen;
    DWORD dwCompAttrOffset;
    DWORD dwCompClauseLen;
    DWORD dwCompClauseOffset;
    DWORD dwCompStrLen;
    DWORD dwCompStrOffset;
    DWORD dwCursorPos;
    DWORD dwDeltaStart;
    DWORD dwResultReadClauseLen;
    DWORD dwResultReadClauseOffset;
    DWORD dwResultReadStrLen;
    DWORD dwResultReadStrOffset;
    DWORD dwResultClauseLen;
    DWORD dwResultClauseOffset;
    DWORD dwResultStrLen;
    DWORD dwResultStrOffset;
    DWORD dwPrivateSize;
    DWORD dwPrivateOffset;
} COMPOSITIONSTRING, *LPCOMPOSITIONSTRING;

LPINPUTCONTEXT WINAPI ImmLockIMC(HIMC);
BOOL  WINAPI ImmUnlockIMC(HIMC);
DWORD WINAPI ImmGetIMCLockCount(HIMC);
HIMCC  WINAPI ImmCreateIMCC(DWORD);
HIMCC  WINAPI ImmDestroyIMCC(HIMCC);
LPVOID WINAPI ImmLockIMCC(HIMCC);
BOOL   WINAPI ImmUnlockIMCC(HIMCC);
DWORD  WINAPI ImmGetIMCCLockCount(HIMCC);
HIMCC  WINAPI ImmReSizeIMCC(HIMCC, DWORD);
DWORD  WINAPI ImmGetIMCCSize(HIMCC);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  /* _DDKIMM_H_ */
