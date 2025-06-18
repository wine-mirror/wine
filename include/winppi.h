/*
 * Copyright 2023 Piotr Caban for CodeWeavers
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

#ifndef _WINPPI_
#define _WINPPI_

HANDLE WINAPI GdiGetSpoolFileHandle(LPWSTR pwszPrinterName,
        LPDEVMODEW pDevmode, LPWSTR pwszDocName);
BOOL WINAPI GdiDeleteSpoolFileHandle(HANDLE SpoolFileHandle);
DWORD WINAPI GdiGetPageCount(HANDLE SpoolFileHandle);
HDC WINAPI GdiGetDC(HANDLE SpoolFileHandle);
HANDLE WINAPI GdiGetPageHandle(HANDLE SpoolFileHandle,
        DWORD Page, LPDWORD pdwPageType);
BOOL WINAPI GdiStartDocEMF(HANDLE SpoolFileHandle, DOCINFOW *pDocInfo);
BOOL WINAPI GdiStartPageEMF(HANDLE SpoolFileHandle);
BOOL WINAPI GdiPlayPageEMF(HANDLE SpoolFileHandle, HANDLE hemf,
        RECT *prectDocument, RECT *prectBorder, RECT *prectClip);
BOOL WINAPI GdiEndPageEMF(HANDLE SpoolFileHandle, DWORD dwOptimization);
BOOL WINAPI GdiEndDocEMF(HANDLE SpoolFileHandle);
BOOL WINAPI GdiGetDevmodeForPage(HANDLE SpoolFileHandle, DWORD dwPageNumber,
        PDEVMODEW *pCurrDM, PDEVMODEW *pLastDM);
BOOL WINAPI GdiResetDCEMF(HANDLE SpoolFileHandle, PDEVMODEW pCurrDM);

#endif /* _WINPPI_ */
