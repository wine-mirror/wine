/*
 * DDK version of imm.h - imm.h for IMM and IME.
 *
 * Copyright 2000 Hidenori Takeshima
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

#ifndef __WINE_IMMDDK_H
#define __WINE_IMMDDK_H

#include "imm.h"

#define	NULLIMC			((HIMC)0)

/* offsets for WndExtra */
#define	IMMGWL_IMC		0
#define	IMMGWL_PRIVATE		(sizeof(LONG))

/* INPUTCONTEXT.fdwInit */
#define	INIT_STATUSWNDPOS	0x00000001
#define	INIT_CONVERSION		0x00000002
#define	INIT_SENTENCE		0x00000004
#define	INIT_LOGFONT		0x00000008
#define	INIT_COMPFORM		0x00000010
#define	INIT_SOFTKBDPOS		0x00000020

/* IMEINFO.fdwProperty (low-order word) */
#define	IME_PROP_END_UNLOAD		0x00000001
#define	IME_PROP_KBD_CHAR_FIRST		0x00000002
#define	IME_PROP_IGNORE_UPKEYS		0x00000004
#define	IME_PROP_NEED_ALTKEY		0x00000008
#define	IME_PROP_NO_KEYS_ON_CLOSE	0x00000010
/* IMEINFO.fdwProperty (high-order word) */
#define	IME_PROP_AT_CARET		0x00010000
#define	IME_PROP_SPECIAL_UI		0x00020000
#define	IME_PROP_CANDLIST_START_FROM_1	0x00040000
#define	IME_PROP_UNICODE		0x00080000
#define	IME_PROP_COMPLETE_ON_UNSELECT	0x00100000


/*** IMM and IME Structures ***/

typedef struct tagINPUTCONTEXT {
	HWND			hWnd;
	BOOL			fOpen;
	POINT			ptStatusWndPos;
	POINT			ptSoftKbdPos;
	DWORD			fdwConversion;
	DWORD			fdwSentence;
	union {
		LOGFONTA		A;
		LOGFONTW		W;
	}			lfFont;
	COMPOSITIONFORM		cfCompForm;
	CANDIDATEFORM		cfCandForm[4];
	HIMCC			hCompStr;
	HIMCC			hCandInfo;
	HIMCC			hGuideLine;
	HIMCC			hPrivate;
	DWORD			dwNumMsgBuf;
	HIMCC			hMsgBuf;
	DWORD			fdwInit;
	DWORD			dwReserve[3];
} INPUTCONTEXT, * LPINPUTCONTEXT;

typedef struct tagCOMPOSITIONSTRING
{
	DWORD	dwSize;
	DWORD	dwCompReadAttrLen;
	DWORD	dwCompReadAttrOffset;
	DWORD	dwCompReadClauseLen;
	DWORD	dwCompReadClauseOffset;
	DWORD	dwCompReadStrLen;
	DWORD	dwCompReadStrOffset;
	DWORD	dwCompAttrLen;
	DWORD	dwCompAttrOffset;
	DWORD	dwCompClauseLen;
	DWORD	dwCompClauseOffset;
	DWORD	dwCompStrLen;
	DWORD	dwCompStrOffset;
	DWORD	dwCursorPos;
	DWORD	dwDeltaStart;
	DWORD	dwResultReadClauseLen;
	DWORD	dwResultReadClauseOffset;
	DWORD	dwResultReadStrLen;
	DWORD	dwResultReadStrOffset;
	DWORD	dwResultClauseLen;
	DWORD	dwResultClauseOffset;
	DWORD	dwResultStrLen;
	DWORD	dwResultStrOffset;
	DWORD	dwPrivateSize;
	DWORD	dwPrivateOffset;
} COMPOSITIONSTRING, * LPCOMPOSITIONSTRING;

typedef struct tagCANDIDATEINFO
{
	DWORD		dwSize;
	DWORD		dwCount;
	DWORD		dwOffset[32];
	DWORD		dwPrivateSize;
	DWORD		dwPrivateOffset;
} CANDIDATEINFO, * LPCANDIDATEINFO;

typedef struct tagGUIDELINE
{
	DWORD	dwSize;
	DWORD	dwLevel;
	DWORD	dwIndex;
	DWORD	dwStrLen;
	DWORD	dwStrOffset;
	DWORD	dwPrivateSize;
	DWORD	dwPrivateOffset;
} GUIDELINE, * LPGUIDELINE;



/*** IME Management Structures ***/

typedef struct tagIMEINFO
{
	DWORD	dwPrivateDataSize;
	DWORD	fdwProperty;
	DWORD	fdwConversionCaps;
	DWORD	fdwSentenceCaps;
	DWORD	fdwUICaps;
	DWORD	fdwSCSCaps;
	DWORD	fdwSelectCaps;
} IMEINFO, * LPIMEINFO;


/*** IME Communication Structures ***/

typedef struct tagSOFTKBDDATA
{
	UINT	uCount;
	WORD	wCode[1][256];
} SOFTKBDDATA, * LPSOFTKBDDATA;


/*** IMM DDK APIs ***/

HWND WINAPI ImmCreateSoftKeyboard(UINT uType, HWND hwndOwner, int x, int y);
BOOL WINAPI ImmDestroySoftKeyboard(HWND hwndSoftKeyboard);
BOOL WINAPI ImmShowSoftKeyboard(HWND hwndSoftKeyboard, int nCmdShow);

LPINPUTCONTEXT WINAPI ImmLockIMC(HIMC hIMC);
BOOL WINAPI ImmUnlockIMC(HIMC hIMC);
DWORD WINAPI ImmGetIMCLockCount(HIMC hIMC);

HIMCC WINAPI ImmCreateIMCC(DWORD dwSize);
HIMCC WINAPI ImmDestroyIMCC(HIMCC hIMCC);
LPVOID WINAPI ImmLockIMCC(HIMCC hIMCC);
BOOL WINAPI ImmUnlockIMCC(HIMCC hIMCC);
DWORD WINAPI ImmGetIMCCLockCount(HIMCC hIMCC);
HIMCC WINAPI ImmReSizeIMCC(HIMCC hIMCC, DWORD dwSize);
DWORD WINAPI ImmGetIMCCSize(HIMCC hIMCC);


#endif  /* __WINE_IMMDDK_H */
