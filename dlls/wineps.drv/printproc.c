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

#include "psdrv.h"

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
    PSDRV_PDEVICE *pdev;
    struct brush_pattern *patterns;
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
    struct pp_data *data = (struct pp_data *)arg;

    switch (rec->iType)
    {
    case EMR_HEADER:
    {
        const ENHMETAHEADER *header = (const ENHMETAHEADER *)rec;

        data->patterns = calloc(sizeof(*data->patterns), header->nHandles);
        return data->patterns && PSDRV_StartPage(&data->pdev->dev);
    }
    case EMR_POLYBEZIER:
    {
        const EMRPOLYBEZIER *p = (const EMRPOLYBEZIER *)rec;

        return PSDRV_PolyBezier(&data->pdev->dev, (const POINT *)p->aptl, p->cptl);
    }
    case EMR_POLYGON:
    {
        const EMRPOLYGON *p = (const EMRPOLYGON *)rec;

        return PSDRV_PolyPolygon(&data->pdev->dev, (const POINT *)p->aptl,
                (const INT *)&p->cptl, 1);
    }
    case EMR_POLYLINE:
    {
        const EMRPOLYLINE *p = (const EMRPOLYLINE *)rec;

        return PSDRV_PolyPolyline(&data->pdev->dev,
                (const POINT *)p->aptl, &p->cptl, 1);
    }
    case EMR_POLYBEZIERTO:
    {
        const EMRPOLYBEZIERTO *p = (const EMRPOLYBEZIERTO *)rec;

        return PSDRV_PolyBezierTo(&data->pdev->dev, (const POINT *)p->aptl, p->cptl) &&
            MoveToEx(data->pdev->dev.hdc, p->aptl[p->cptl - 1].x, p->aptl[p->cptl - 1].y, NULL);
    }
    case EMR_POLYLINETO:
    {
        const EMRPOLYLINETO *p = (const EMRPOLYLINETO *)rec;
        POINT *pts;
        DWORD cnt;
        int ret;

        cnt = p->cptl + 1;
        pts = malloc(sizeof(*pts) * cnt);
        if (!pts) return 0;

        GetCurrentPositionEx(data->pdev->dev.hdc, pts);
        memcpy(pts + 1, p->aptl, sizeof(*pts) * p->cptl);
        ret = PSDRV_PolyPolyline(&data->pdev->dev, pts, &cnt, 1) &&
            MoveToEx(data->pdev->dev.hdc, pts[cnt - 1].x, pts[cnt - 1].y, NULL);
        free(pts);
        return ret;
    }
    case EMR_POLYPOLYLINE:
    {
        const EMRPOLYPOLYLINE *p = (const EMRPOLYPOLYLINE *)rec;

        return PSDRV_PolyPolyline(&data->pdev->dev,
                (const POINT *)(p->aPolyCounts + p->nPolys),
                p->aPolyCounts, p->nPolys);
    }
    case EMR_POLYPOLYGON:
    {
        const EMRPOLYPOLYGON *p = (const EMRPOLYPOLYGON *)rec;

        return PSDRV_PolyPolygon(&data->pdev->dev,
                (const POINT *)(p->aPolyCounts + p->nPolys),
                (const INT *)p->aPolyCounts, p->nPolys);
    }
    case EMR_EOF:
        return PSDRV_EndPage(&data->pdev->dev);
    case EMR_SETPIXELV:
    {
        const EMRSETPIXELV *p = (const EMRSETPIXELV *)rec;

        return PSDRV_SetPixel(&data->pdev->dev, p->ptlPixel.x,
                p->ptlPixel.y, p->crColor);
    }
    case EMR_SELECTOBJECT:
    {
        const EMRSELECTOBJECT *so = (const EMRSELECTOBJECT *)rec;
        struct brush_pattern *pattern;
        HGDIOBJ obj;

        if (so->ihObject & 0x80000000)
        {
            obj = GetStockObject(so->ihObject & 0x7fffffff);
            pattern = NULL;
        }
        else
        {
            obj = (htable->objectHandle)[so->ihObject];
            pattern = data->patterns + so->ihObject;
        }
        SelectObject(data->pdev->dev.hdc, obj);

        switch (GetObjectType(obj))
        {
        case OBJ_PEN: return PSDRV_SelectPen(&data->pdev->dev, obj, NULL) != NULL;
        case OBJ_BRUSH: return PSDRV_SelectBrush(&data->pdev->dev, obj, pattern) != NULL;
        default:
            FIXME("unhandled object type %ld\n", GetObjectType(obj));
            return 1;
        }
    }
    case EMR_ELLIPSE:
    {
        const EMRELLIPSE *p = (const EMRELLIPSE *)rec;
        const RECTL *r = &p->rclBox;

        return PSDRV_Ellipse(&data->pdev->dev, r->left, r->top, r->right, r->bottom);
    }
    case EMR_RECTANGLE:
    {
        const EMRRECTANGLE *rect = (const EMRRECTANGLE *)rec;

        return PSDRV_Rectangle(&data->pdev->dev, rect->rclBox.left,
                rect->rclBox.top, rect->rclBox.right, rect->rclBox.bottom);
    }
    case EMR_ROUNDRECT:
    {
        const EMRROUNDRECT *p = (const EMRROUNDRECT *)rec;

        return PSDRV_RoundRect(&data->pdev->dev, p->rclBox.left,
                p->rclBox.top, p->rclBox.right, p->rclBox.bottom,
                p->szlCorner.cx, p->szlCorner.cy);
    }
    case EMR_ARC:
    {
        const EMRARC *p = (const EMRARC *)rec;

        return PSDRV_Arc(&data->pdev->dev, p->rclBox.left, p->rclBox.top,
                p->rclBox.right, p->rclBox.bottom, p->ptlStart.x,
                p->ptlStart.y, p->ptlEnd.x, p->ptlEnd.y);
    }
    case EMR_CHORD:
    {
        const EMRCHORD *p = (const EMRCHORD *)rec;

        return PSDRV_Chord(&data->pdev->dev, p->rclBox.left, p->rclBox.top,
                p->rclBox.right, p->rclBox.bottom, p->ptlStart.x,
                p->ptlStart.y, p->ptlEnd.x, p->ptlEnd.y);
    }
    case EMR_PIE:
    {
        const EMRPIE *p = (const EMRPIE *)rec;

        return PSDRV_Pie(&data->pdev->dev, p->rclBox.left, p->rclBox.top,
                p->rclBox.right, p->rclBox.bottom, p->ptlStart.x,
                p->ptlStart.y, p->ptlEnd.x, p->ptlEnd.y);
    }
    case EMR_LINETO:
    {
        const EMRLINETO *line = (const EMRLINETO *)rec;

        return PSDRV_LineTo(&data->pdev->dev, line->ptl.x, line->ptl.y) &&
            MoveToEx(data->pdev->dev.hdc, line->ptl.x, line->ptl.y, NULL);
    }
    case EMR_CREATEMONOBRUSH:
    {
        const EMRCREATEMONOBRUSH *p = (const EMRCREATEMONOBRUSH *)rec;

        if (!PlayEnhMetaFileRecord(data->pdev->dev.hdc, htable, rec, n))
            return 0;
        data->patterns[p->ihBrush].usage = p->iUsage;
        data->patterns[p->ihBrush].info = (BITMAPINFO *)((BYTE *)p + p->offBmi);
        data->patterns[p->ihBrush].bits.ptr = (BYTE *)p + p->offBits;
        return 1;
    }
    case EMR_CREATEDIBPATTERNBRUSHPT:
    {
        const EMRCREATEDIBPATTERNBRUSHPT *p = (const EMRCREATEDIBPATTERNBRUSHPT *)rec;

        if (!PlayEnhMetaFileRecord(data->pdev->dev.hdc, htable, rec, n))
            return 0;
        data->patterns[p->ihBrush].usage = p->iUsage;
        data->patterns[p->ihBrush].info = (BITMAPINFO *)((BYTE *)p + p->offBmi);
        data->patterns[p->ihBrush].bits.ptr = (BYTE *)p + p->offBits;
        return 1;
    }

    case EMR_MOVETOEX:
    case EMR_SETWORLDTRANSFORM:
    case EMR_MODIFYWORLDTRANSFORM:
        return PlayEnhMetaFileRecord(data->pdev->dev.hdc, htable, rec, n);
    default:
        FIXME("unsupported record: %ld\n", rec->iType);
    }

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

    ret = EnumEnhMetaFile(NULL, hmf, hmf_proc, (void *)data, NULL);
    DeleteEnhMetaFile(hmf);
    free(data->patterns);
    data->patterns = NULL;
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
    HDC hdc;

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

    hdc = CreateCompatibleDC(NULL);
    if (!hdc)
    {
        LocalFree(data);
        return NULL;
    }
    SetGraphicsMode(hdc, GM_ADVANCED);
    data->pdev = create_psdrv_physdev(hdc, open_data->pPrinterName,
            (const PSDRV_DEVMODE *)open_data->pDevMode);
    if (!data->pdev)
    {
        DeleteDC(hdc);
        LocalFree(data);
        return NULL;
    }
    data->pdev->dev.hdc = hdc;
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
    data->pdev->job.id = StartDocPrinterW(data->hport, 1, (BYTE *)&info);
    if (!data->pdev->job.id)
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

    data->pdev->job.hprinter = data->hport;
    if (!PSDRV_WriteHeader(&data->pdev->dev, data->doc_name))
    {
        WARN("Failed to write header\n");
        goto cleanup;
    }
    data->pdev->job.banding = FALSE;
    data->pdev->job.OutOfPage = TRUE;
    data->pdev->job.PageNo = 0;
    data->pdev->job.quiet = FALSE;
    data->pdev->job.passthrough_state = passthrough_none;
    data->pdev->job.doc_name = strdupW(data->doc_name);

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
    if (data->pdev->job.PageNo)
        PSDRV_WriteFooter(&data->pdev->dev);

    HeapFree(GetProcessHeap(), 0, data->pdev->job.doc_name);
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
    DeleteDC(data->pdev->dev.hdc);
    HeapFree(GetProcessHeap(), 0, data->pdev->Devmode);
    HeapFree(GetProcessHeap(), 0, data->pdev);

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
