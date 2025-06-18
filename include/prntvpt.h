/*
 * Copyright (C) 2015 Nikolay Sivov for CodeWeavers
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

#ifndef _PRNPTNTV_H_
#define _PRNPTNTV_H_

#ifdef __cplusplus
extern "C" {
#endif

DECLARE_HANDLE(HPTPROVIDER);

#define S_PT_NO_CONFLICT       0x00040001
#define S_PT_CONFLICT_RESOLVED 0x00040002
#define E_PRINTTICKET_FORMAT   0x80040003

typedef enum
{
    kPTPageScope,
    kPTDocumentScope,
    kPTJobScope
} EPrintTicketScope;

typedef enum
{
    kUserDefaultDevmode,
    kPrinterDefaultDevmode
} EDefaultDevmodeType;

HRESULT WINAPI PTOpenProvider(const WCHAR *printer, DWORD version, HPTPROVIDER *provider);
HRESULT WINAPI PTOpenProviderEx(const WCHAR *printer, DWORD max_version, DWORD pref_version, HPTPROVIDER *provider, DWORD *used_version);
HRESULT WINAPI PTCloseProvider(HPTPROVIDER);
HRESULT WINAPI PTConvertDevModeToPrintTicket(HPTPROVIDER, ULONG, PDEVMODEW, EPrintTicketScope, IStream *);
HRESULT WINAPI PTConvertPrintTicketToDevMode(HPTPROVIDER, IStream *, EDefaultDevmodeType, EPrintTicketScope, ULONG *, PDEVMODEW *, BSTR *);
HRESULT WINAPI PTGetPrintCapabilities(HPTPROVIDER, IStream *, IStream *, BSTR *);
HRESULT WINAPI PTMergeAndValidatePrintTicket(HPTPROVIDER, IStream *, IStream *, EPrintTicketScope, IStream *, BSTR *);
HRESULT WINAPI PTReleaseMemory(PVOID);

#ifdef __cplusplus
}
#endif

#endif /* _PRNPTNTV_H_ */
