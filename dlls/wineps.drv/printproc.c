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

#include <stdlib.h>

#include <windows.h>
#include <winspool.h>
#include <ddk/winsplp.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);

#define EMFSPOOL_VERSION 0x10000
#define PP_MAGIC 0x952173fe

struct pp_data
{
    DWORD magic;
    HANDLE hport;
    WCHAR *doc_name;
    WCHAR *out_file;
};

typedef enum
{
    EMRI_METAFILE = 1,
    EMRI_ENGINE_FONT,
    EMRI_DEVMODE,
    EMRI_TYPE1_FONT,
    EMRI_PRESTARTPAGE,
    EMRI_DESIGNVECTOR,
    EMRI_SUBSET_FONT,
    EMRI_DELTA_FONT,
    EMRI_FORM_METAFILE,
    EMRI_BW_METAFILE,
    EMRI_BW_FORM_METAFILE,
    EMRI_METAFILE_DATA,
    EMRI_METAFILE_EXT,
    EMRI_BW_METAFILE_EXT,
    EMRI_ENGINE_FONT_EXT,
    EMRI_TYPE1_FONT_EXT,
    EMRI_DESIGNVECTOR_EXT,
    EMRI_SUBSET_FONT_EXT,
    EMRI_DELTA_FONT_EXT,
    EMRI_PS_JOB_DATA,
    EMRI_EMBED_FONT_EXT,
} record_type;

typedef struct
{
    unsigned int dwVersion;
    unsigned int cjSize;
    unsigned int dpszDocName;
    unsigned int dpszOutput;
} emfspool_header;

typedef struct
{
    unsigned int ulID;
    unsigned int cjSize;
} record_hdr;

BOOL WINAPI SeekPrinter(HANDLE, LARGE_INTEGER, LARGE_INTEGER*, DWORD, BOOL);

static const WCHAR emf_1003[] = L"NT EMF 1.003";

#define EMRICASE(x) case x: return #x
static const char * debugstr_rec_type(int id)
{
    switch (id)
    {
    EMRICASE(EMRI_METAFILE);
    EMRICASE(EMRI_ENGINE_FONT);
    EMRICASE(EMRI_DEVMODE);
    EMRICASE(EMRI_TYPE1_FONT);
    EMRICASE(EMRI_PRESTARTPAGE);
    EMRICASE(EMRI_DESIGNVECTOR);
    EMRICASE(EMRI_SUBSET_FONT);
    EMRICASE(EMRI_DELTA_FONT);
    EMRICASE(EMRI_FORM_METAFILE);
    EMRICASE(EMRI_BW_METAFILE);
    EMRICASE(EMRI_BW_FORM_METAFILE);
    EMRICASE(EMRI_METAFILE_DATA);
    EMRICASE(EMRI_METAFILE_EXT);
    EMRICASE(EMRI_BW_METAFILE_EXT);
    EMRICASE(EMRI_ENGINE_FONT_EXT);
    EMRICASE(EMRI_TYPE1_FONT_EXT);
    EMRICASE(EMRI_DESIGNVECTOR_EXT);
    EMRICASE(EMRI_SUBSET_FONT_EXT);
    EMRICASE(EMRI_DELTA_FONT_EXT);
    EMRICASE(EMRI_PS_JOB_DATA);
    EMRICASE(EMRI_EMBED_FONT_EXT);
    default:
        FIXME("unknown record type: %d\n", id);
        return NULL;
    }
}
#undef EMRICASE

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

static int WINAPI hmf_proc(HDC hdc, HANDLETABLE *htable,
        const ENHMETARECORD *rec, int n, LPARAM arg)
{
    FIXME("unsupported record: %ld\n", rec->iType);
    return 1;
}

static BOOL print_metafile(struct pp_data *data, HANDLE hdata)
{
    record_hdr header;
    HENHMETAFILE hmf;
    BYTE *buf;
    BOOL ret;
    DWORD r;

    if (!ReadPrinter(hdata, &header, sizeof(header), &r))
        return FALSE;
    if (r != sizeof(header))
    {
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }

    buf = malloc(header.cjSize);
    if (!buf)
        return FALSE;

    if (!ReadPrinter(hdata, buf, header.cjSize, &r))
    {
        free(buf);
        return FALSE;
    }
    if (r != header.cjSize)
    {
        free(buf);
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }

    hmf = SetEnhMetaFileBits(header.cjSize, buf);
    free(buf);
    if (!hmf)
        return FALSE;

    ret = EnumEnhMetaFile(NULL, hmf, hmf_proc, NULL, NULL);
    DeleteEnhMetaFile(hmf);
    return ret;
}

BOOL WINAPI EnumPrintProcessorDatatypesW(WCHAR *server, WCHAR *name, DWORD level,
        BYTE *datatypes, DWORD size, DWORD *needed, DWORD *no)
{
    DATATYPES_INFO_1W *info = (DATATYPES_INFO_1W *)datatypes;

    TRACE("%s, %s, %ld, %p, %ld, %p, %p\n", debugstr_w(server), debugstr_w(name),
            level, datatypes, size, needed, no);

    if (!needed || !no)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *no = 0;
    *needed = sizeof(*info) + sizeof(emf_1003);

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

    *no = 1;
    info->pName = (WCHAR*)(info + 1);
    memcpy(info + 1, emf_1003, sizeof(emf_1003));
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
    if (wcscmp(open_data->pDatatype, emf_1003))
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
    emfspool_header header;
    LARGE_INTEGER pos, cur;
    record_hdr record;
    HANDLE spool_data;
    DOC_INFO_1W info;
    BOOL ret;
    DWORD r;

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

    if (!(ret = ReadPrinter(spool_data, &header, sizeof(header), &r)))
        goto cleanup;
    if (r != sizeof(header))
    {
        SetLastError(ERROR_INVALID_DATA);
        ret = FALSE;
        goto cleanup;
    }

    if (header.dwVersion != EMFSPOOL_VERSION)
    {
        FIXME("unrecognized spool file format\n");
        SetLastError(ERROR_INTERNAL_ERROR);
        goto cleanup;
    }
    pos.QuadPart = header.cjSize;
    if (!(ret = SeekPrinter(spool_data, pos, NULL, FILE_BEGIN, FALSE)))
        goto cleanup;

    while (1)
    {
        if (!(ret = ReadPrinter(spool_data, &record, sizeof(record), &r)))
            goto cleanup;
        if (!r)
            break;
        if (r != sizeof(record))
        {
            SetLastError(ERROR_INVALID_DATA);
            ret = FALSE;
            goto cleanup;
        }

        switch (record.ulID)
        {
        case EMRI_METAFILE_DATA:
            pos.QuadPart = record.cjSize;
            ret = SeekPrinter(spool_data, pos, NULL, FILE_CURRENT, FALSE);
            if (!ret)
                goto cleanup;
            break;
        case EMRI_METAFILE_EXT:
        case EMRI_BW_METAFILE_EXT:
            pos.QuadPart = 0;
            ret = SeekPrinter(spool_data, pos, &cur, FILE_CURRENT, FALSE);
            if (ret)
            {
                cur.QuadPart += record.cjSize;
                ret = ReadPrinter(spool_data, &pos, sizeof(pos), &r);
                if (r != sizeof(pos))
                {
                    SetLastError(ERROR_INVALID_DATA);
                    ret = FALSE;
                }
            }
            pos.QuadPart = -pos.QuadPart - 2 * sizeof(record);
            if (ret)
                ret = SeekPrinter(spool_data, pos, NULL, FILE_CURRENT, FALSE);
            if (ret)
                ret = print_metafile(data, spool_data);
            if (ret)
                ret = SeekPrinter(spool_data, cur, NULL, FILE_BEGIN, FALSE);
            if (!ret)
                goto cleanup;
            break;
        default:
            FIXME("%s not supported, skipping\n", debugstr_rec_type(record.ulID));
            pos.QuadPart = record.cjSize;
            ret = SeekPrinter(spool_data, pos, NULL, FILE_CURRENT, FALSE);
            if (!ret)
                goto cleanup;
            break;
        }
    }

cleanup:
    ClosePrinter(spool_data);
    return EndDocPrinter(data->hport) && ret;
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
    AddPrintProcessorW(NULL, (WCHAR *)L"Windows 4.0", (WCHAR *)L"wineps.drv", (WCHAR *)L"wineps");
    AddPrintProcessorW(NULL, NULL, (WCHAR *)L"wineps.drv", (WCHAR *)L"wineps");
    return S_OK;
}
