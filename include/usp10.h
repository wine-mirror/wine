/*
 * Copyright (C) 2005 Steven Edwards
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

#ifndef __USP10_H
#define __USP10_H

/** ScriptStringAnalyse */
#define  SSA_PASSWORD         0x00000001
#define  SSA_TAB              0x00000002
#define  SSA_CLIP             0x00000004
#define  SSA_FIT              0x00000008
#define  SSA_DZWG             0x00000010
#define  SSA_FALLBACK         0x00000020
#define  SSA_BREAK            0x00000040
#define  SSA_GLYPHS           0x00000080
#define  SSA_RTL              0x00000100
#define  SSA_GCP              0x00000200
#define  SSA_HOTKEY           0x00000400
#define  SSA_METAFILE         0x00000800
#define  SSA_LINK             0x00001000
#define  SSA_HIDEHOTKEY       0x00002000
#define  SSA_HOTKEYONLY       0x00002400
#define  SSA_FULLMEASURE      0x04000000
#define  SSA_LPKANSIFALLBACK  0x08000000
#define  SSA_PIDX             0x10000000
#define  SSA_LAYOUTRTL        0x20000000
#define  SSA_DONTGLYPH        0x40000000 
#define  SSA_NOKASHIDA        0x80000000 

/** StringIsComplex */
#define  SIC_COMPLEX     1
#define  SIC_ASCIIDIGIT  2
#define  SIC_NEUTRAL     4

typedef struct tag_SCRIPT_CONTROL {
  DWORD uDefaultLanguage	:16;
  DWORD fContextDigits		:1;
  DWORD fInvertPreBoundDir	:1;
  DWORD fInvertPostBoundDir	:1;
  DWORD fLinkStringBefore	:1;
  DWORD fLinkStringAfter	:1;
  DWORD fNeutralOverride	:1;
  DWORD fNumericOverride	:1;
  DWORD fLegacyBidiClass	:1;
  DWORD fReserved		:8;
} SCRIPT_CONTROL;

typedef struct {
  DWORD langid			:16;
  DWORD fNumeric		:1;
  DWORD fComplex		:1;     
  DWORD fNeedsWordBreaking	:1;     
  DWORD fNeedsCaretInfo		:1;
  DWORD bCharSet		:8;
  DWORD fControl		:1;
  DWORD fPrivateUseArea		:1;
  DWORD fNeedsCharacterJustify	:1;
  DWORD fInvalidGlyph		:1;
  DWORD fInvalidLogAttr		:1;
  DWORD fCDM			:1;
  DWORD fAmbiguousCharSet	:1;
  DWORD fClusterSizeVaries	:1;
  DWORD fRejectInvalid		:1;
} SCRIPT_PROPERTIES;

typedef struct tag_SCRIPT_STATE {
  WORD uBidiLevel		:5;
  WORD fOverrideDirection	:1;
  WORD fInhibitSymSwap		:1;
  WORD fCharShape		:1;
  WORD fDigitSubstitute		:1;
  WORD fInhibitLigate		:1;
  WORD fDisplayZWG		:1;
  WORD fArabicNumContext	:1;
  WORD fGcpClusters		:1;
  WORD fReserved		:1;
  WORD fEngineReserved		:2;
} SCRIPT_STATE;

typedef struct tag_SCRIPT_ANALYSIS {
  WORD eScript			:10;
  WORD fRTL			:1;
  WORD fLayoutRTL		:1;
  WORD fLinkBefore		:1;
  WORD fLinkAfter		:1;
  WORD fLogicalOrder		:1;
  WORD fNoGlyphIndex		:1;
  SCRIPT_STATE 	s;
} SCRIPT_ANALYSIS;

typedef struct tag_SCRIPT_ITEM {
  int iCharPos;
  SCRIPT_ANALYSIS a;
} SCRIPT_ITEM;

typedef struct tag_SCRIPT_DIGITSUBSTITUTE {
  DWORD NationalDigitLanguage		:16;
  DWORD TraditionalDigitLanguage	:16;
  DWORD DigitSubstitute			:8;
  DWORD dwReserved;
} SCRIPT_DIGITSUBSTITUTE;

typedef struct tag_SCRIPT_FONTPROPERTIES {
  int   cBytes;
  WORD wgBlank;
  WORD wgDefault;
  WORD wgInvalid;
  WORD wgKashida;
  int iKashidaWidth;
} SCRIPT_FONTPROPERTIES;

typedef struct tag_SCRIPT_TABDEF {
  int cTabStops;
  int iScale;
  int *pTabStops;
  int iTabOrigin;
} SCRIPT_TABDEF;

typedef void *SCRIPT_CACHE;
typedef void *SCRIPT_STRING_ANALYSIS; 

/* Function Declairations */

HRESULT WINAPI ScriptGetProperties(const SCRIPT_PROPERTIES ***ppSp, int *piNumScripts);
HRESULT WINAPI ScriptRecordDigitSubstitution(LCID Locale, SCRIPT_DIGITSUBSTITUTE *psds);
HRESULT WINAPI ScriptApplyDigitSubstitution(const SCRIPT_DIGITSUBSTITUTE* psds, 
                                            SCRIPT_CONTROL* psc, SCRIPT_STATE* pss);
HRESULT WINAPI ScriptItemize(const WCHAR *pwcInChars, int cInChars, int cMaxItems, 
                             const SCRIPT_CONTROL *psControl, const SCRIPT_STATE *psState, 
                             SCRIPT_ITEM *pItems, int *pcItems);
HRESULT WINAPI ScriptGetFontProperties(HDC hdc, SCRIPT_CACHE *psc, SCRIPT_FONTPROPERTIES *sfp);
HRESULT WINAPI ScriptStringAnalyse(HDC hdc, 
				   const void *pString, 
				   int cString, 
				   int cGlyphs,
				   int iCharset,
				   DWORD dwFlags,
				   int iReqWidth,
				   SCRIPT_CONTROL *psControl,
				   SCRIPT_STATE *psState,
				   const int *piDx,
				   SCRIPT_TABDEF *pTabdef,
				   const BYTE *pbInClass,
				   SCRIPT_STRING_ANALYSIS *pssa);
HRESULT WINAPI ScriptStringFree(SCRIPT_STRING_ANALYSIS *pssa);
HRESULT WINAPI ScriptIsComplex(const WCHAR* pwcInChars, int cInChars, DWORD dwFlags);

#endif /* __USP10_H */
