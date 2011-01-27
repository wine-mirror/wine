/*
 * Unit test suite for metafiles
 *
 * Copyright (C) 2011 Vincent Povirk for CodeWeavers
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

#include "windows.h"
#include <stdio.h>
#include "gdiplus.h"
#include "wine/test.h"

#define expect(expected, got) ok(got == expected, "Expected %.8x, got %.8x\n", expected, got)

typedef struct emfplus_record
{
    ULONG todo;
    ULONG record_type;
    int is_emfplus;
} emfplus_record;

typedef struct emfplus_check_state
{
    const char *desc;
    int count;
    const struct emfplus_record *expected;
} emfplus_check_state;

static void check_record(int count, const char *desc, const struct emfplus_record *expected, const struct emfplus_record *actual)
{
    ok(expected->record_type == actual->record_type,
        "%s.%i: Expected record type 0x%x, got 0x%x\n", desc, count,
        expected->record_type, actual->record_type);

    ok(expected->is_emfplus == actual->is_emfplus,
        "%s.%i: Expected is_emfplus %i, got %i\n", desc, count,
        expected->record_type, actual->record_type);
}

typedef struct EmfPlusRecordHeader
{
    WORD Type;
    WORD Flags;
    DWORD Size;
    DWORD DataSize;
} EmfPlusRecordHeader;

static int CALLBACK enum_emf_proc(HDC hDC, HANDLETABLE *lpHTable, const ENHMETARECORD *lpEMFR,
    int nObj, LPARAM lpData)
{
    emfplus_check_state *state = (emfplus_check_state*)lpData;
    emfplus_record actual;

    if (lpEMFR->iType == EMR_GDICOMMENT)
    {
        const EMRGDICOMMENT *comment = (const EMRGDICOMMENT*)lpEMFR;

        if (comment->cbData >= 4 && memcmp(comment->Data, "EMF+", 4) == 0)
        {
            int offset = 4;

            while (offset + sizeof(EmfPlusRecordHeader) <= comment->cbData)
            {
                const EmfPlusRecordHeader *record = (const EmfPlusRecordHeader*)&comment->Data[offset];

                ok(record->Size == record->DataSize + sizeof(EmfPlusRecordHeader),
                    "%s: EMF+ record datasize %u and size %u mismatch\n", state->desc, record->DataSize, record->Size);

                ok(offset + record->DataSize <= comment->cbData,
                    "%s: EMF+ record truncated\n", state->desc);

                if (offset + record->DataSize > comment->cbData)
                    return 0;

                if (state->expected[state->count].record_type)
                {
                    actual.todo = 0;
                    actual.record_type = record->Type;
                    actual.is_emfplus = 1;

                    check_record(state->count, state->desc, &state->expected[state->count], &actual);

                    state->count++;
                }
                else
                {
                    ok(0, "%s: Unexpected EMF+ 0x%x record\n", state->desc, record->Type);
                }

                offset += record->Size;
            }

            ok(offset == comment->cbData, "%s: truncated EMF+ record data?\n", state->desc);

            return 1;
        }
    }

    if (state->expected[state->count].record_type)
    {
        actual.todo = 0;
        actual.record_type = lpEMFR->iType;
        actual.is_emfplus = 0;

        check_record(state->count, state->desc, &state->expected[state->count], &actual);

        state->count++;
    }
    else
    {
        ok(0, "%s: Unexpected EMF 0x%x record\n", state->desc, lpEMFR->iType);
    }

    return 1;
}

static void check_emfplus(HENHMETAFILE hemf, const emfplus_record *expected, const char *desc)
{
    emfplus_check_state state;

    state.desc = desc;
    state.count = 0;
    state.expected = expected;

    EnumEnhMetaFile(0, hemf, enum_emf_proc, &state, NULL);

    ok(expected[state.count].record_type == 0, "%s: Got %i records, expecting more\n", desc, state.count);
}

static const emfplus_record empty_records[] = {
    {0, EMR_HEADER, 0},
    {0, EmfPlusRecordTypeHeader, 1},
    {0, EmfPlusRecordTypeEndOfFile, 1},
    {0, EMR_EOF, 0},
    {0}
};

static void test_empty(void)
{
    GpStatus stat;
    GpMetafile *metafile;
    GpGraphics *graphics;
    HDC hdc;
    HENHMETAFILE hemf, dummy;
    BOOL ret;
    static const GpRectF frame = {0.0, 0.0, 100.0, 100.0};
    static const WCHAR description[] = {'w','i','n','e','t','e','s','t',0};

    hdc = CreateCompatibleDC(0);

    stat = GdipRecordMetafile(hdc, EmfTypeEmfPlusOnly, &frame, MetafileFrameUnitPixel, description, &metafile);
    todo_wine expect(Ok, stat);

    DeleteDC(hdc);

    if (stat != Ok)
        return;

    stat = GdipGetHemfFromMetafile(metafile, &hemf);
    expect(InvalidParameter, stat);

    stat = GdipGetImageGraphicsContext((GpImage*)metafile, &graphics);
    expect(Ok, stat);

    stat = GdipGetHemfFromMetafile(metafile, &hemf);
    expect(InvalidParameter, stat);

    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);

    stat = GdipGetHemfFromMetafile(metafile, &hemf);
    expect(Ok, stat);

    stat = GdipGetHemfFromMetafile(metafile, &dummy);
    expect(InvalidParameter, stat);

    stat = GdipDisposeImage((GpImage*)metafile);
    expect(Ok, stat);

    check_emfplus(hemf, empty_records, "empty");

    ret = DeleteEnhMetaFile(hemf);
    ok(ret != 0, "Failed to delete enhmetafile %p\n", hemf);
}

START_TEST(metafile)
{
    struct GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;

    gdiplusStartupInput.GdiplusVersion              = 1;
    gdiplusStartupInput.DebugEventCallback          = NULL;
    gdiplusStartupInput.SuppressBackgroundThread    = 0;
    gdiplusStartupInput.SuppressExternalCodecs      = 0;

    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    test_empty();

    GdiplusShutdown(gdiplusToken);
}
