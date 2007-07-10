/*
 * Unit test suite for paths
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
#include "wine/test.h"
#include "math.h"

#define expect(expected, got) ok(got == expected, "Expected %.8x, got %.8x\n", expected, got)
#define POINT_TYPE_MAX_LEN (75)

static void stringify_point_type(PathPointType type, char * name)
{
    *name = '\0';

    switch(type & PathPointTypePathTypeMask){
        case PathPointTypeStart:
            strcat(name, "PathPointTypeStart");
            break;
        case PathPointTypeLine:
            strcat(name, "PathPointTypeLine");
            break;
        case PathPointTypeBezier:
            strcat(name, "PathPointTypeBezier");
            break;
        default:
            strcat(name, "Unknown type");
            return;
    }

    type &= ~PathPointTypePathTypeMask;
    if(type & ~((PathPointTypePathMarker | PathPointTypeCloseSubpath))){
        *name = '\0';
        strcat(name, "Unknown type");
        return;
    }

    if(type & PathPointTypePathMarker)
        strcat(name, " | PathPointTypePathMarker");
    if(type & PathPointTypeCloseSubpath)
        strcat(name, " | PathPointTypeCloseSubpath");
}

/* this helper structure and function modeled after gdi path.c test */
typedef struct
{
    REAL X, Y;
    BYTE type;

    /* How many extra entries before this one only on wine
     * but not on native? */
    int wine_only_entries_preceding;

    /* 0 - This entry matches on wine.
     * 1 - This entry corresponds to a single entry on wine that does not match the native entry.
     * 2 - This entry is currently skipped on wine but present on native. */
    int todo;
} path_test_t;

static void ok_path(GpPath* path, const path_test_t *expected, INT expected_size, BOOL todo_size)
{
    BYTE * types;
    INT size, idx = 0, eidx = 0, numskip;
    GpPointF * points;
    char ename[POINT_TYPE_MAX_LEN], name[POINT_TYPE_MAX_LEN];

    if(GdipGetPointCount(path, &size) != Ok){
        skip("Cannot perform path comparisons due to failure to retrieve path.\n");
        return;
    }

    if(todo_size) todo_wine
        ok(size == expected_size, "Path size %d does not match expected size %d\n",
            size, expected_size);
    else
        ok(size == expected_size, "Path size %d does not match expected size %d\n",
            size, expected_size);

    points = HeapAlloc(GetProcessHeap(), 0, size * sizeof(GpPointF));
    types = HeapAlloc(GetProcessHeap(), 0, size);

    if(GdipGetPathPoints(path, points, size) != Ok || GdipGetPathTypes(path, types, size) != Ok){
        skip("Cannot perform path comparisons due to failure to retrieve path.\n");
        goto end;
    }

    numskip = expected_size ? expected[eidx].wine_only_entries_preceding : 0;
    while (idx < size && eidx < expected_size){
        /* We allow a few pixels fudge in matching X and Y coordinates to account for imprecision in
         * floating point to integer conversion */
        BOOL match = (types[idx] == expected[eidx].type) &&
            fabs(points[idx].X - expected[eidx].X) <= 2.0 &&
            fabs(points[idx].Y - expected[eidx].Y) <= 2.0;

        stringify_point_type(expected[eidx].type, ename);
        stringify_point_type(types[idx], name);

        if (expected[eidx].todo || numskip) todo_wine
            ok(match, "Expected #%d: %s (%.1f,%.1f) but got %s (%.1f,%.1f)\n", eidx,
               ename, expected[eidx].X, expected[eidx].Y,
               name, points[idx].X, points[idx].Y);
        else
            ok(match, "Expected #%d: %s (%.1f,%.1f) but got %s (%.1f,%.1f)\n", eidx,
               ename, expected[eidx].X, expected[eidx].Y,
               name, points[idx].X, points[idx].Y);

        if (match || expected[eidx].todo != 2)
            idx++;
        if (match || !numskip--)
            numskip = expected[++eidx].wine_only_entries_preceding;
    }

end:
    HeapFree(GetProcessHeap(), 0, types);
    HeapFree(GetProcessHeap(), 0, points);
}

static void test_constructor_destructor(void)
{
    GpStatus status;
    GpPath* path = NULL;

    status = GdipCreatePath(FillModeAlternate, &path);
    expect(Ok, status);
    ok(path != NULL, "Expected path to be initialized\n");

    status = GdipDeletePath(NULL);
    expect(InvalidParameter, status);

    status = GdipDeletePath(path);
    expect(Ok, status);
}

static path_test_t line2_path[] = {
    {0.0, 50.0, PathPointTypeStart, 0, 0}, /*0*/
    {5.0, 45.0, PathPointTypeLine, 0, 0}, /*1*/
    {0.0, 40.0, PathPointTypeLine, 0, 0}, /*2*/
    {15.0, 35.0, PathPointTypeLine, 0, 0}, /*3*/
    {0.0, 30.0, PathPointTypeLine, 0, 0}, /*4*/
    {25.0, 25.0, PathPointTypeLine | PathPointTypeCloseSubpath, 0, 0}, /*5*/
    {0.0, 20.0, PathPointTypeStart, 0, 0}, /*6*/
    {35.0, 15.0, PathPointTypeLine, 0, 0}, /*7*/
    {0.0, 10.0, PathPointTypeLine, 0, 0} /*8*/
    };

static void test_line2(void)
{
    GpStatus status;
    GpPath* path;
    int i;
    GpPointF line2_points[9];

    for(i = 0; i < 9; i ++){
        line2_points[i].X = i * 5.0 * (REAL)(i % 2);
        line2_points[i].Y = 50.0 - i * 5.0;
    }

    GdipCreatePath(FillModeAlternate, &path);
    status = GdipAddPathLine2(path, line2_points, 3);
    expect(Ok, status);
    status = GdipAddPathLine2(path, &(line2_points[3]), 3);
    expect(Ok, status);
    status = GdipClosePathFigure(path);
    expect(Ok, status);
    status = GdipAddPathLine2(path, &(line2_points[6]), 3);
    expect(Ok, status);

    ok_path(path, line2_path, sizeof(line2_path)/sizeof(path_test_t), FALSE);
}

START_TEST(graphicspath)
{
    struct GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;

    gdiplusStartupInput.GdiplusVersion              = 1;
    gdiplusStartupInput.DebugEventCallback          = NULL;
    gdiplusStartupInput.SuppressBackgroundThread    = 0;
    gdiplusStartupInput.SuppressExternalCodecs      = 0;

    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    test_constructor_destructor();
    test_line2();

    GdiplusShutdown(gdiplusToken);
}
