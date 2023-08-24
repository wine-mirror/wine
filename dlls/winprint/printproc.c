/*
 * Print processor implementation.
 *
 * Copyright 2022 Piotr Caban for CodeWeavers
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

/* Missing datatypes support:
 *  RAW [FF appended]
 *  RAW [FF auto]
 *  NT EMF 1.003
 *  NT EMF 1.006
 *  NT EMF 1.007
 *  NT EMF 1.008
 *  XPS2GDI
 *
 * Implement print processor features like e.g. printing multiple copies. */

#include <stdlib.h>

#include "windows.h"
#include "winspool.h"
#include "ddk/winsplp.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winprint);

#define PP_MAGIC 0x952173fd

struct pp_data
{
    DWORD magic;
    HANDLE hport;
    WCHAR *doc_name;
    WCHAR *out_file;
};

static struct pp_data* get_handle_data(HANDLE pp)
{
    struct pp_data *ret = (struct pp_data *)pp;

    if (!ret || ret->magic != PP_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }
    return ret;
}

BOOL WINAPI EnumPrintProcessorDatatypesW(WCHAR *server, WCHAR *name, DWORD level,
        BYTE *datatypes, DWORD size, DWORD *needed, DWORD *no)
{
    static const WCHAR raw[] = L"RAW";
    static const WCHAR text[] = L"TEXT";

    DATATYPES_INFO_1W *info = (DATATYPES_INFO_1W *)datatypes;

    TRACE("%s, %s, %ld, %p, %ld, %p, %p\n", debugstr_w(server), debugstr_w(name),
            level, datatypes, size, needed, no);

    if (!needed || !no)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *no = 0;
    *needed = 2 * sizeof(*info) + sizeof(raw) + sizeof(text);

    if (level != 1 || (size && !datatypes))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (size < *needed)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    *no = 2;
    info[0].pName = (WCHAR*)(info + 2);
    info[1].pName = info[0].pName + sizeof(raw) / sizeof(WCHAR);
    memcpy(info[0].pName, raw, sizeof(raw));
    memcpy(info[1].pName, text, sizeof(text));
    return TRUE;
}

HANDLE WINAPI OpenPrintProcessor(WCHAR *port, PRINTPROCESSOROPENDATA *open_data)
{
    struct pp_data *data;
    HANDLE hport;

    TRACE("%s, %p\n", debugstr_w(port), open_data);

    if (!port || !open_data || !open_data->pDatatype)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    if (wcscmp(open_data->pDatatype, L"RAW") && wcscmp(open_data->pDatatype, L"TEXT"))
    {
        SetLastError(ERROR_INVALID_DATATYPE);
        return NULL;
    }

    if (!OpenPrinterW(port, &hport, NULL))
        return NULL;

    data = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(*data));
    if (!data)
        return NULL;
    data->magic = PP_MAGIC;
    data->hport = hport;
    data->doc_name = wcsdup(open_data->pDocumentName);
    data->out_file = wcsdup(open_data->pOutputFile);
    return (HANDLE)data;
}

BOOL WINAPI PrintDocumentOnPrintProcessor(HANDLE pp, WCHAR *doc_name)
{
    struct pp_data *data = get_handle_data(pp);
    HANDLE spool_data;
    DOC_INFO_1W info;
    BYTE buf[0x1000];
    DWORD r, w;

    TRACE("%p, %s\n", pp, debugstr_w(doc_name));

    if (!data)
        return FALSE;

    if (!OpenPrinterW(doc_name, &spool_data, NULL))
        return FALSE;

    info.pDocName = data->doc_name;
    info.pOutputFile = data->out_file;
    info.pDatatype = (WCHAR *)L"RAW";
    if (!StartDocPrinterW(data->hport, 1, (BYTE *)&info))
    {
        ClosePrinter(spool_data);
        return FALSE;
    }

    while (ReadPrinter(spool_data, buf, sizeof(buf), &r) && r)
    {
        if (!WritePrinter(data->hport, buf, r, &w) || r != w)
        {
            ClosePrinter(spool_data);
            EndDocPrinter(data->hport);
            return FALSE;
        }
    }
    ClosePrinter(spool_data);

    return EndDocPrinter(data->hport);
}

BOOL WINAPI ControlPrintProcessor(HANDLE pp, DWORD cmd)
{
    FIXME("%p, %ld\n", pp, cmd);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI ClosePrintProcessor(HANDLE pp)
{
    struct pp_data *data = get_handle_data(pp);

    TRACE("%p\n", pp);

    if (!data)
        return FALSE;

    ClosePrinter(data->hport);
    free(data->doc_name);
    free(data->out_file);

    memset(data, 0, sizeof(*data));
    LocalFree(data);
    return TRUE;
}

HRESULT WINAPI DllRegisterServer(void)
{
    AddPrintProcessorW(NULL, (WCHAR *)L"Windows 4.0", (WCHAR *)L"winprint.dll", (WCHAR *)L"winprint");
    AddPrintProcessorW(NULL, NULL, (WCHAR *)L"winprint.dll", (WCHAR *)L"winprint");
    return S_OK;
}
