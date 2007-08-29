/*
 * Unit test suite for graphics objects
 *
 * Copyright (C) 2007 Google (Evan Stade)
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
#include "gdiplus.h"
#include "wingdi.h"
#include "wine/test.h"

#define expect(expected, got) ok(got == expected, "Expected %.8x, got %.8x\n", expected, got)
#define TABLE_LEN (23)

static void test_constructor_destructor(void)
{
    GpStatus stat;
    GpGraphics *graphics = NULL;
    HDC hdc = GetDC(0);

    stat = GdipCreateFromHDC(NULL, &graphics);
    expect(OutOfMemory, stat);
    stat = GdipDeleteGraphics(graphics);
    expect(InvalidParameter, stat);

    stat = GdipCreateFromHDC(hdc, &graphics);
    expect(Ok, stat);
    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);

    stat = GdipCreateFromHWND(NULL, &graphics);
    expect(Ok, stat);
    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);

    stat = GdipDeleteGraphics(NULL);
    expect(InvalidParameter, stat);
    ReleaseDC(0, hdc);
}

typedef struct node{
    GraphicsState data;
    struct node * next;
} node;

/* Linked list prepend function. */
static void log_state(GraphicsState data, node ** log)
{
    node * new_entry = HeapAlloc(GetProcessHeap(), 0, sizeof(node));

    new_entry->data = data;
    new_entry->next = *log;
    *log = new_entry;
}

/* Checks if there are duplicates in the list, and frees it. */
static void check_no_duplicates(node * log)
{
    INT dups = 0;
    node * temp = NULL;

    if(!log)
        goto end;

    do{
        HeapFree(GetProcessHeap(), 0, temp);
        temp = log;
        while((temp = temp->next))
            if(log->data == temp->data)
                dups++;

    }while((log = log->next));

    HeapFree(GetProcessHeap(), 0, temp);

end:
    expect(0, dups);
}

static void test_save_restore(void)
{
    GpStatus stat;
    GraphicsState state_a, state_b, state_c;
    InterpolationMode mode;
    GpGraphics *graphics1, *graphics2;
    node * state_log = NULL;
    HDC hdc = GetDC(0);

    /* Invalid saving. */
    GdipCreateFromHDC(hdc, &graphics1);
    stat = GdipSaveGraphics(graphics1, NULL);
    expect(InvalidParameter, stat);
    stat = GdipSaveGraphics(NULL, &state_a);
    expect(InvalidParameter, stat);
    GdipDeleteGraphics(graphics1);

    log_state(state_a, &state_log);

    /* Basic save/restore. */
    GdipCreateFromHDC(hdc, &graphics1);
    GdipSetInterpolationMode(graphics1, InterpolationModeBilinear);
    stat = GdipSaveGraphics(graphics1, &state_a);
    todo_wine
        expect(Ok, stat);
    GdipSetInterpolationMode(graphics1, InterpolationModeBicubic);
    stat = GdipRestoreGraphics(graphics1, state_a);
    todo_wine
        expect(Ok, stat);
    GdipGetInterpolationMode(graphics1, &mode);
    todo_wine
        expect(InterpolationModeBilinear, mode);
    GdipDeleteGraphics(graphics1);

    log_state(state_a, &state_log);

    /* Restoring garbage doesn't affect saves. */
    GdipCreateFromHDC(hdc, &graphics1);
    GdipSetInterpolationMode(graphics1, InterpolationModeBilinear);
    GdipSaveGraphics(graphics1, &state_a);
    GdipSetInterpolationMode(graphics1, InterpolationModeBicubic);
    GdipSaveGraphics(graphics1, &state_b);
    GdipSetInterpolationMode(graphics1, InterpolationModeNearestNeighbor);
    stat = GdipRestoreGraphics(graphics1, 0xdeadbeef);
    todo_wine
        expect(Ok, stat);
    GdipRestoreGraphics(graphics1, state_b);
    GdipGetInterpolationMode(graphics1, &mode);
    todo_wine
        expect(InterpolationModeBicubic, mode);
    GdipRestoreGraphics(graphics1, state_a);
    GdipGetInterpolationMode(graphics1, &mode);
    todo_wine
        expect(InterpolationModeBilinear, mode);
    GdipDeleteGraphics(graphics1);

    log_state(state_a, &state_log);
    log_state(state_b, &state_log);

    /* Restoring older state invalidates newer saves (but not older saves). */
    GdipCreateFromHDC(hdc, &graphics1);
    GdipSetInterpolationMode(graphics1, InterpolationModeBilinear);
    GdipSaveGraphics(graphics1, &state_a);
    GdipSetInterpolationMode(graphics1, InterpolationModeBicubic);
    GdipSaveGraphics(graphics1, &state_b);
    GdipSetInterpolationMode(graphics1, InterpolationModeNearestNeighbor);
    GdipSaveGraphics(graphics1, &state_c);
    GdipSetInterpolationMode(graphics1, InterpolationModeHighQualityBilinear);
    GdipRestoreGraphics(graphics1, state_b);
    GdipGetInterpolationMode(graphics1, &mode);
    todo_wine
        expect(InterpolationModeBicubic, mode);
    GdipRestoreGraphics(graphics1, state_c);
    GdipGetInterpolationMode(graphics1, &mode);
    todo_wine
        expect(InterpolationModeBicubic, mode);
    GdipRestoreGraphics(graphics1, state_a);
    GdipGetInterpolationMode(graphics1, &mode);
    todo_wine
        expect(InterpolationModeBilinear, mode);
    GdipDeleteGraphics(graphics1);

    log_state(state_a, &state_log);
    log_state(state_b, &state_log);
    log_state(state_c, &state_log);

    /* Restoring older save from one graphics object does not invalidate
     * newer save from other graphics object. */
    GdipCreateFromHDC(hdc, &graphics1);
    GdipCreateFromHDC(hdc, &graphics2);
    GdipSetInterpolationMode(graphics1, InterpolationModeBilinear);
    GdipSaveGraphics(graphics1, &state_a);
    GdipSetInterpolationMode(graphics2, InterpolationModeBicubic);
    GdipSaveGraphics(graphics2, &state_b);
    GdipSetInterpolationMode(graphics1, InterpolationModeNearestNeighbor);
    GdipSetInterpolationMode(graphics2, InterpolationModeNearestNeighbor);
    GdipRestoreGraphics(graphics1, state_a);
    GdipGetInterpolationMode(graphics1, &mode);
    todo_wine
        expect(InterpolationModeBilinear, mode);
    GdipRestoreGraphics(graphics2, state_b);
    GdipGetInterpolationMode(graphics2, &mode);
    todo_wine
        expect(InterpolationModeBicubic, mode);
    GdipDeleteGraphics(graphics1);
    GdipDeleteGraphics(graphics2);

    /* You can't restore a state to a graphics object that didn't save it. */
    GdipCreateFromHDC(hdc, &graphics1);
    GdipCreateFromHDC(hdc, &graphics2);
    GdipSetInterpolationMode(graphics1, InterpolationModeBilinear);
    GdipSaveGraphics(graphics1, &state_a);
    GdipSetInterpolationMode(graphics1, InterpolationModeNearestNeighbor);
    GdipSetInterpolationMode(graphics2, InterpolationModeNearestNeighbor);
    GdipRestoreGraphics(graphics2, state_a);
    GdipGetInterpolationMode(graphics2, &mode);
    expect(InterpolationModeNearestNeighbor, mode);
    GdipDeleteGraphics(graphics1);
    GdipDeleteGraphics(graphics2);

    log_state(state_a, &state_log);

    /* The same state value should never be returned twice. */
    todo_wine
        check_no_duplicates(state_log);

    ReleaseDC(0, hdc);
}

START_TEST(graphics)
{
    struct GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;

    gdiplusStartupInput.GdiplusVersion              = 1;
    gdiplusStartupInput.DebugEventCallback          = NULL;
    gdiplusStartupInput.SuppressBackgroundThread    = 0;
    gdiplusStartupInput.SuppressExternalCodecs      = 0;

    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    test_constructor_destructor();
    test_save_restore();

    GdiplusShutdown(gdiplusToken);
}
