/*
 * Copyright 2000 Eric Pouech
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

#ifndef __WINE_WINEACM16_H
#define __WINE_WINEACM16_H

#include "wine/windef16.h"
#include "wine/mmsystem16.h"

/***********************************************************************
 * Win16 definitions
 */
typedef BOOL16 (CALLBACK *ACMDRIVERENUMCB16)(
  HACMDRIVERID16 hadid, DWORD dwInstance, DWORD fdwSupport
);
typedef UINT (CALLBACK *ACMFILTERCHOOSEHOOKPROC16)(
  HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam
);
typedef UINT16 (CALLBACK *ACMFORMATCHOOSEHOOKPROC16)(
  HWND16 hwnd, UINT16 uMsg, WPARAM16 wParam, LPARAM lParam
);

typedef struct _ACMDRIVERDETAILS16
{
  DWORD   cbStruct;

  FOURCC  fccType;
  FOURCC  fccComp;

  WORD    wMid;
  WORD    wPid;

  DWORD   vdwACM;
  DWORD   vdwDriver;

  DWORD   fdwSupport;
  DWORD   cFormatTags;
  DWORD   cFilterTags;

  HICON16 hicon;

  CHAR    szShortName[ACMDRIVERDETAILS_SHORTNAME_CHARS];
  CHAR    szLongName[ACMDRIVERDETAILS_LONGNAME_CHARS];
  CHAR    szCopyright[ACMDRIVERDETAILS_COPYRIGHT_CHARS];
  CHAR    szLicensing[ACMDRIVERDETAILS_LICENSING_CHARS];
  CHAR    szFeatures[ACMDRIVERDETAILS_FEATURES_CHARS];
} ACMDRIVERDETAILS16, *NPACMDRIVERDETAILS16, *LPACMDRIVERDETAILS16;

typedef struct _ACMFILTERCHOOSE16
{
  DWORD          cbStruct;
  DWORD          fdwStyle;

  HWND16         hwndOwner;

  LPWAVEFILTER   pwfltr;
  DWORD          cbwfltr;

  LPCSTR         pszTitle;

  char           szFilterTag[ACMFILTERTAGDETAILS_FILTERTAG_CHARS];
  char           szFilter[ACMFILTERDETAILS_FILTER_CHARS];
  LPSTR          pszName;
  DWORD          cchName;

  DWORD          fdwEnum;
  LPWAVEFILTER   pwfltrEnum;

  HINSTANCE16    hInstance;
  LPCSTR         pszTemplateName;
  LPARAM         lCustData;
  ACMFILTERCHOOSEHOOKPROC16 pfnHook;
} ACMFILTERCHOOSE16, *NPACMFILTERCHOOSE16, *LPACMFILTERCHOOSE16;

typedef struct _ACMFILTERDETAILS16
{
  DWORD          cbStruct;
  DWORD          dwFilterIndex;
  DWORD          dwFilterTag;
  DWORD          fdwSupport;
  LPWAVEFILTER   pwfltr;
  DWORD          cbwfltr;
  CHAR           szFilter[ACMFILTERDETAILS_FILTER_CHARS];
} ACMFILTERDETAILS16, *NPACMFILTERDETAILS16, *LPACMFILTERDETAILS16;

typedef struct _ACMFILTERTAGDETAILS16
{
  DWORD cbStruct;
  DWORD dwFilterTagIndex;
  DWORD dwFilterTag;
  DWORD cbFilterSize;
  DWORD fdwSupport;
  DWORD cStandardFilters;
  CHAR  szFilterTag[ACMFILTERTAGDETAILS_FILTERTAG_CHARS];
} ACMFILTERTAGDETAILS16, *NPACMFILTERTAGDETAILS16, *LPACMFILTERTAGDETAILS16;

typedef struct _ACMFORMATCHOOSE16
{
  DWORD            cbStruct;
  DWORD            fdwStyle;

  HWND16           hwndOwner;

  LPWAVEFORMATEX   pwfx;
  DWORD            cbwfx;
  LPCSTR           pszTitle;

  CHAR             szFormatTag[ACMFORMATTAGDETAILS_FORMATTAG_CHARS];
  CHAR             szFormat[ACMFORMATDETAILS_FORMAT_CHARS];

  LPSTR            pszName;
  DWORD            cchName;

  DWORD            fdwEnum;
  LPWAVEFORMATEX   pwfxEnum;

  HINSTANCE16      hInstance;
  LPCSTR           pszTemplateName;
  LPARAM           lCustData;
  ACMFORMATCHOOSEHOOKPROC16 pfnHook;
} ACMFORMATCHOOSE16, *NPACMFORMATCHOOSE16, *LPACMFORMATCHOOSE16;

typedef struct _ACMFORMATDETAILS16
{
    DWORD            cbStruct;
    DWORD            dwFormatIndex;
    DWORD            dwFormatTag;
    DWORD            fdwSupport;
    LPWAVEFORMATEX   pwfx;
    DWORD            cbwfx;
    CHAR             szFormat[ACMFORMATDETAILS_FORMAT_CHARS];
} ACMFORMATDETAILS16, *NPACMFORMATDETAILS16, *LPACMFORMATDETAILS16;

typedef struct _ACMFORMATTAGDETAILS16
{
  DWORD cbStruct;
  DWORD dwFormatTagIndex;
  DWORD dwFormatTag;
  DWORD cbFormatSize;
  DWORD fdwSupport;
  DWORD cStandardFormats;
  CHAR  szFormatTag[ACMFORMATTAGDETAILS_FORMATTAG_CHARS];
} ACMFORMATTAGDETAILS16, *NPACMFORMATTAGDETAILS16, *LPACMFORMATTAGDETAILS16;

typedef ACMSTREAMHEADER ACMSTREAMHEADER16, *NPACMSTREAMHEADER16, *LPACMSTREAMHEADER16;

typedef BOOL16 (CALLBACK *ACMFILTERENUMCB16)(
 HACMDRIVERID16 hadid, LPACMFILTERDETAILS16 pafd,
 DWORD dwInstance, DWORD fdwSupport
);

typedef BOOL16 (CALLBACK *ACMFILTERTAGENUMCB16)(
  HACMDRIVERID16 hadid, LPACMFILTERTAGDETAILS16 paftd,
  DWORD dwInstance, DWORD fdwSupport
);

typedef BOOL16 (CALLBACK *ACMFORMATENUMCB16)(
  HACMDRIVERID16 hadid, LPACMFORMATDETAILS16 pafd,
  DWORD dwInstance, DWORD fdwSupport
);

typedef BOOL16 (CALLBACK *ACMFORMATTAGENUMCB16)(
  HACMDRIVERID16 hadid, LPACMFORMATTAGDETAILS16 paftd,
  DWORD dwInstance, DWORD fdwSupport
);

/***********************************************************************
 * Functions - Win16
 */

DWORD WINAPI acmGetVersion16(void);
MMRESULT16 WINAPI acmMetrics16(
  HACMOBJ16 hao, UINT16 uMetric, LPVOID pMetric
);
MMRESULT16 WINAPI acmDriverEnum16(
  ACMDRIVERENUMCB16 fnCallback, DWORD dwInstance, DWORD fdwEnum
);
MMRESULT16 WINAPI acmDriverDetails16(
  HACMDRIVERID16 hadid, LPACMDRIVERDETAILS16 padd, DWORD fdwDetails
);
MMRESULT16 WINAPI acmDriverAdd16(
  LPHACMDRIVERID16 phadid, HINSTANCE16 hinstModule,
  LPARAM lParam, DWORD dwPriority, DWORD fdwAdd
);
MMRESULT16 WINAPI acmDriverRemove16(
  HACMDRIVERID16 hadid, DWORD fdwRemove
);
MMRESULT16 WINAPI acmDriverOpen16(
  LPHACMDRIVER16 phad, HACMDRIVERID16 hadid, DWORD fdwOpen
);
MMRESULT16 WINAPI acmDriverClose16(
  HACMDRIVER16 had, DWORD fdwClose
);
LRESULT WINAPI acmDriverMessage16(
  HACMDRIVER16 had, UINT16 uMsg, LPARAM lParam1, LPARAM lParam2
);
MMRESULT16 WINAPI acmDriverID16(
  HACMOBJ16 hao, LPHACMDRIVERID16 phadid, DWORD fdwDriverID
);
MMRESULT16 WINAPI acmDriverPriority16(
 HACMDRIVERID16 hadid, DWORD dwPriority, DWORD fdwPriority
);
MMRESULT16 WINAPI acmFormatTagDetails16(
  HACMDRIVER16 had, LPACMFORMATTAGDETAILS16 paftd, DWORD fdwDetails
);
MMRESULT16 WINAPI acmFormatTagEnum16(
  HACMDRIVER16 had, LPACMFORMATTAGDETAILS16 paftd,
  ACMFORMATTAGENUMCB16 fnCallback, DWORD dwInstance, DWORD fdwEnum
);
MMRESULT16 WINAPI acmFormatChoose16(
  LPACMFORMATCHOOSE16 pafmtc
);
MMRESULT16 WINAPI acmFormatDetails16(
  HACMDRIVER16 had, LPACMFORMATDETAILS16 pafd, DWORD fdwDetails
);
MMRESULT16 WINAPI acmFormatEnum16(
  HACMDRIVER16 had, LPACMFORMATDETAILS16 pafd,
  ACMFORMATENUMCB16 fnCallback, DWORD dwInstance, DWORD fdwEnum
);
MMRESULT16 WINAPI acmFormatSuggest16(
  HACMDRIVER16 had, LPWAVEFORMATEX pwfxSrc,
  LPWAVEFORMATEX pwfxDst, DWORD cbwfxDst, DWORD fdwSuggest
);
MMRESULT16 WINAPI acmFilterTagDetails16(
  HACMDRIVER16 had, LPACMFILTERTAGDETAILS16 paftd, DWORD fdwDetails
);
MMRESULT16 WINAPI acmFilterTagEnum16(
  HACMDRIVER16 had, LPACMFILTERTAGDETAILS16 paftd,
  ACMFILTERTAGENUMCB16 fnCallback, DWORD dwInstance, DWORD fdwEnum
);
MMRESULT16 WINAPI acmFilterChoose16(
  LPACMFILTERCHOOSE16 pafltrc
);
MMRESULT16 WINAPI acmFilterDetails16(
  HACMDRIVER16 had, LPACMFILTERDETAILS16 pafd, DWORD fdwDetails
);
MMRESULT16 WINAPI acmFilterEnum16(
  HACMDRIVER16 had, LPACMFILTERDETAILS16 pafd,
  ACMFILTERENUMCB16 fnCallback, DWORD dwInstance, DWORD fdwEnum
);
MMRESULT16 WINAPI acmStreamOpen16(
  LPHACMSTREAM16 phas, HACMDRIVER16 had,
  LPWAVEFORMATEX pwfxSrc, LPWAVEFORMATEX pwfxDst,
  LPWAVEFILTER pwfltr, DWORD dwCallback,
  DWORD dwInstance, DWORD fdwOpen
);
MMRESULT16 WINAPI acmStreamClose16(
  HACMSTREAM16 has, DWORD fdwClose
);
MMRESULT16 WINAPI acmStreamSize16(
  HACMSTREAM16 has, DWORD cbInput,
  LPDWORD pdwOutputBytes, DWORD fdwSize
);
MMRESULT16 WINAPI acmStreamConvert16(
  HACMSTREAM16 has, LPACMSTREAMHEADER16 pash, DWORD fdwConvert
);
MMRESULT16 WINAPI acmStreamReset16(
  HACMSTREAM16 has, DWORD fdwReset
);
MMRESULT16 WINAPI acmStreamPrepareHeader16(
  HACMSTREAM16 has, LPACMSTREAMHEADER16 pash, DWORD fdwPrepare
);
MMRESULT16 WINAPI acmStreamUnprepareHeader16(
  HACMSTREAM16 has, LPACMSTREAMHEADER16 pash, DWORD fdwUnprepare
);

#endif /* __WINE_WINEACM16_H */
