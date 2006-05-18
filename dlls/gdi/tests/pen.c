/*
 * Unit test suite for pens
 *
 * Copyright 2006 Dmitry Timoshkov
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

#include <stdarg.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

#include "wine/test.h"

static void test_logpen(void)
{
    static const struct
    {
        UINT style;
        INT width;
        COLORREF color;
        UINT ret_style;
        INT ret_width;
        COLORREF ret_color;
    } pen[] = {
        { PS_SOLID, -123, RGB(0x12,0x34,0x56), PS_SOLID, 123, RGB(0x12,0x34,0x56) },
        { PS_SOLID, 0, RGB(0x12,0x34,0x56), PS_SOLID, 0, RGB(0x12,0x34,0x56) },
        { PS_SOLID, 123, RGB(0x12,0x34,0x56), PS_SOLID, 123, RGB(0x12,0x34,0x56) },
        { PS_DASH, 123, RGB(0x12,0x34,0x56), PS_DASH, 123, RGB(0x12,0x34,0x56) },
        { PS_DOT, 123, RGB(0x12,0x34,0x56), PS_DOT, 123, RGB(0x12,0x34,0x56) },
        { PS_DASHDOT, 123, RGB(0x12,0x34,0x56), PS_DASHDOT, 123, RGB(0x12,0x34,0x56) },
        { PS_DASHDOTDOT, 123, RGB(0x12,0x34,0x56), PS_DASHDOTDOT, 123, RGB(0x12,0x34,0x56) },
        { PS_NULL, -123, RGB(0x12,0x34,0x56), PS_NULL, 1, 0 },
        { PS_NULL, 123, RGB(0x12,0x34,0x56), PS_NULL, 1, 0 },
        { PS_INSIDEFRAME, 123, RGB(0x12,0x34,0x56), PS_INSIDEFRAME, 123, RGB(0x12,0x34,0x56) },
        { PS_USERSTYLE, 123, RGB(0x12,0x34,0x56), PS_SOLID, 123, RGB(0x12,0x34,0x56) },
        { PS_ALTERNATE, 123, RGB(0x12,0x34,0x56), PS_SOLID, 123, RGB(0x12,0x34,0x56) }
    };
    INT i, size;
    HPEN hpen;
    LOGPEN lp;
    EXTLOGPEN elp;
    LOGBRUSH lb;
    DWORD obj_type, user_style[2] = { 0xabc, 0xdef };
    struct
    {
        EXTLOGPEN elp;
        DWORD style_data[10];
    } ext_pen;

    for (i = 0; i < sizeof(pen)/sizeof(pen[0]); i++)
    {
        trace("testing style %u\n", pen[i].style);

        /********************** cosmetic pens **********************/
        /* CreatePenIndirect behaviour */
        lp.lopnStyle = pen[i].style,
        lp.lopnWidth.x = pen[i].width;
        lp.lopnWidth.y = 11; /* just in case */
        lp.lopnColor = pen[i].color;
        SetLastError(0xdeadbeef);
        hpen = CreatePenIndirect(&lp);
        ok(hpen != 0, "CreatePen error %ld\n", GetLastError());

        obj_type = GetObjectType(hpen);
        ok(obj_type == OBJ_PEN, "wrong object type %lu\n", obj_type);

        memset(&lp, 0xb0, sizeof(lp));
        SetLastError(0xdeadbeef);
        size = GetObject(hpen, sizeof(lp), &lp);
        ok(size == sizeof(lp), "GetObject returned %d, error %ld\n", size, GetLastError());

        ok(lp.lopnStyle == pen[i].ret_style, "expected %u, got %u\n", pen[i].ret_style, lp.lopnStyle);
        ok(lp.lopnWidth.x == pen[i].ret_width, "expected %u, got %ld\n", pen[i].ret_width, lp.lopnWidth.x);
        ok(lp.lopnWidth.y == 0, "expected 0, got %ld\n", lp.lopnWidth.y);
        ok(lp.lopnColor == pen[i].ret_color, "expected %08lx, got %08lx\n", pen[i].ret_color, lp.lopnColor);

        DeleteObject(hpen);

        /* CreatePen behaviour */
        SetLastError(0xdeadbeef);
        hpen = CreatePen(pen[i].style, pen[i].width, pen[i].color);
        ok(hpen != 0, "CreatePen error %ld\n", GetLastError());

        obj_type = GetObjectType(hpen);
        ok(obj_type == OBJ_PEN, "wrong object type %lu\n", obj_type);

        /* check what's the real size of the object */
        size = GetObject(hpen, 0, NULL);
        ok(size == sizeof(lp), "GetObject returned %d, error %ld\n", size, GetLastError());

        /* ask for truncated data */
        memset(&lp, 0xb0, sizeof(lp));
        SetLastError(0xdeadbeef);
        size = GetObject(hpen, sizeof(lp.lopnStyle), &lp);
        ok(!size, "GetObject should fail: size %d, error %ld\n", size, GetLastError());

        /* see how larger buffer sizes are handled */
        memset(&lp, 0xb0, sizeof(lp));
        SetLastError(0xdeadbeef);
        size = GetObject(hpen, sizeof(lp) * 2, &lp);
        ok(size == sizeof(lp), "GetObject returned %d, error %ld\n", size, GetLastError());

        /* see how larger buffer sizes are handled */
        memset(&elp, 0xb0, sizeof(elp));
        SetLastError(0xdeadbeef);
        size = GetObject(hpen, sizeof(elp) * 2, &elp);
        ok(size == sizeof(lp), "GetObject returned %d, error %ld\n", size, GetLastError());

        memset(&lp, 0xb0, sizeof(lp));
        SetLastError(0xdeadbeef);
        size = GetObject(hpen, sizeof(lp), &lp);
        ok(size == sizeof(lp), "GetObject returned %d, error %ld\n", size, GetLastError());

        ok(lp.lopnStyle == pen[i].ret_style, "expected %u, got %u\n", pen[i].ret_style, lp.lopnStyle);
        ok(lp.lopnWidth.x == pen[i].ret_width, "expected %u, got %ld\n", pen[i].ret_width, lp.lopnWidth.x);
        ok(lp.lopnWidth.y == 0, "expected 0, got %ld\n", lp.lopnWidth.y);
        ok(lp.lopnColor == pen[i].ret_color, "expected %08lx, got %08lx\n", pen[i].ret_color, lp.lopnColor);

        memset(&elp, 0xb0, sizeof(elp));
        SetLastError(0xdeadbeef);
        size = GetObject(hpen, sizeof(elp), &elp);

        /* for some reason XP differentiates PS_NULL here */
        if (pen[i].style == PS_NULL)
        {
            ok(size == sizeof(EXTLOGPEN), "GetObject returned %d, error %ld\n", size, GetLastError());
            ok(elp.elpPenStyle == pen[i].ret_style, "expected %u, got %lu\n", pen[i].ret_style, elp.elpPenStyle);
            ok(elp.elpWidth == 0, "expected 0, got %lu\n", elp.elpWidth);
            ok(elp.elpColor == pen[i].ret_color, "expected %08lx, got %08lx\n", pen[i].ret_color, elp.elpColor);
            ok(elp.elpBrushStyle == BS_SOLID, "expected BS_SOLID, got %u\n", elp.elpBrushStyle);
            ok(elp.elpHatch == 0, "expected 0, got %p\n", (void *)elp.elpHatch);
            ok(elp.elpNumEntries == 0, "expected 0, got %lx\n", elp.elpNumEntries);
        }
        else
        {
            ok(size == sizeof(LOGPEN), "GetObject returned %d, error %ld\n", size, GetLastError());
            memcpy(&lp, &elp, sizeof(lp));
            ok(lp.lopnStyle == pen[i].ret_style, "expected %u, got %u\n", pen[i].ret_style, lp.lopnStyle);
            ok(lp.lopnWidth.x == pen[i].ret_width, "expected %u, got %ld\n", pen[i].ret_width, lp.lopnWidth.x);
            ok(lp.lopnWidth.y == 0, "expected 0, got %ld\n", lp.lopnWidth.y);
            ok(lp.lopnColor == pen[i].ret_color, "expected %08lx, got %08lx\n", pen[i].ret_color, lp.lopnColor);
        }

        DeleteObject(hpen);

        /********** cosmetic pens created by ExtCreatePen ***********/
        lb.lbStyle = BS_SOLID;
        lb.lbColor = pen[i].color;
        lb.lbHatch = HS_CROSS; /* just in case */
        SetLastError(0xdeadbeef);
        hpen = ExtCreatePen(pen[i].style, pen[i].width, &lb, 2, user_style);
        if (pen[i].style != PS_USERSTYLE)
        {
            ok(hpen == 0, "ExtCreatePen should fail\n");
            ok(GetLastError() == ERROR_INVALID_PARAMETER,
               "wrong last error value %ld\n", GetLastError());
            SetLastError(0xdeadbeef);
            hpen = ExtCreatePen(pen[i].style, pen[i].width, &lb, 0, NULL);
            if (pen[i].style != PS_NULL)
            {
                ok(hpen == 0, "ExtCreatePen with width != 1 should fail\n");
                ok(GetLastError() == ERROR_INVALID_PARAMETER,
                   "wrong last error value %ld\n", GetLastError());

                SetLastError(0xdeadbeef);
                hpen = ExtCreatePen(pen[i].style, 1, &lb, 0, NULL);
            }
        }
        else
        {
            ok(hpen == 0, "ExtCreatePen with width != 1 should fail\n");
            ok(GetLastError() == ERROR_INVALID_PARAMETER,
               "wrong last error value %ld\n", GetLastError());
            SetLastError(0xdeadbeef);
            hpen = ExtCreatePen(pen[i].style, 1, &lb, 2, user_style);
        }
        if (pen[i].style == PS_INSIDEFRAME)
        {
            /* This style is applicable only for gemetric pens */
            ok(hpen == 0, "ExtCreatePen should fail\n");
            goto test_geometric_pens;
        }
        ok(hpen != 0, "ExtCreatePen error %ld\n", GetLastError());

        obj_type = GetObjectType(hpen);
        /* for some reason XP differentiates PS_NULL here */
        if (pen[i].style == PS_NULL)
            ok(obj_type == OBJ_PEN, "wrong object type %lu\n", obj_type);
        else
            ok(obj_type == OBJ_EXTPEN, "wrong object type %lu\n", obj_type);

        /* check what's the real size of the object */
        SetLastError(0xdeadbeef);
        size = GetObject(hpen, 0, NULL);
        switch (pen[i].style)
        {
        case PS_NULL:
            ok(size == sizeof(LOGPEN),
               "GetObject returned %d, error %ld\n", size, GetLastError());
            break;

        case PS_USERSTYLE:
            ok(size == sizeof(EXTLOGPEN) - sizeof(elp.elpStyleEntry) + sizeof(user_style),
               "GetObject returned %d, error %ld\n", size, GetLastError());
            break;

        default:
            ok(size == sizeof(EXTLOGPEN) - sizeof(elp.elpStyleEntry),
               "GetObject returned %d, error %ld\n", size, GetLastError());
            break;
        }

        /* ask for truncated data */
        memset(&elp, 0xb0, sizeof(elp));
        SetLastError(0xdeadbeef);
        size = GetObject(hpen, sizeof(elp.elpPenStyle), &elp);
        ok(!size, "GetObject should fail: size %d, error %ld\n", size, GetLastError());

        /* see how larger buffer sizes are handled */
        memset(&ext_pen, 0xb0, sizeof(ext_pen));
        SetLastError(0xdeadbeef);
        size = GetObject(hpen, sizeof(ext_pen), &ext_pen.elp);
        switch (pen[i].style)
        {
        case PS_NULL:
            ok(size == sizeof(LOGPEN),
               "GetObject returned %d, error %ld\n", size, GetLastError());
            memcpy(&lp, &ext_pen.elp, sizeof(lp));
            ok(lp.lopnStyle == pen[i].ret_style, "expected %u, got %u\n", pen[i].ret_style, lp.lopnStyle);
            ok(lp.lopnWidth.x == pen[i].ret_width, "expected %u, got %ld\n", pen[i].ret_width, lp.lopnWidth.x);
            ok(lp.lopnWidth.y == 0, "expected 0, got %ld\n", lp.lopnWidth.y);
            ok(lp.lopnColor == pen[i].ret_color, "expected %08lx, got %08lx\n", pen[i].ret_color, lp.lopnColor);

            /* for PS_NULL it also works this way */
            memset(&elp, 0xb0, sizeof(elp));
            SetLastError(0xdeadbeef);
            size = GetObject(hpen, sizeof(elp), &elp);
            ok(size == sizeof(EXTLOGPEN),
                "GetObject returned %d, error %ld\n", size, GetLastError());
            ok(ext_pen.elp.elpHatch == 0xb0b0b0b0, "expected 0xb0b0b0b0, got %p\n", (void *)ext_pen.elp.elpHatch);
            ok(ext_pen.elp.elpNumEntries == 0xb0b0b0b0, "expected 0xb0b0b0b0, got %lx\n", ext_pen.elp.elpNumEntries);
            break;

        case PS_USERSTYLE:
            ok(size == sizeof(EXTLOGPEN) - sizeof(elp.elpStyleEntry) + sizeof(user_style),
               "GetObject returned %d, error %ld\n", size, GetLastError());
            ok(ext_pen.elp.elpHatch == HS_CROSS, "expected HS_CROSS, got %p\n", (void *)ext_pen.elp.elpHatch);
            ok(ext_pen.elp.elpNumEntries == 2, "expected 0, got %lx\n", ext_pen.elp.elpNumEntries);
            ok(ext_pen.elp.elpStyleEntry[0] == 0xabc, "expected 0xabc, got %lx\n", ext_pen.elp.elpStyleEntry[0]);
            ok(ext_pen.elp.elpStyleEntry[1] == 0xdef, "expected 0xabc, got %lx\n", ext_pen.elp.elpStyleEntry[1]);
            break;

        default:
            ok(size == sizeof(EXTLOGPEN) - sizeof(elp.elpStyleEntry),
               "GetObject returned %d, error %ld\n", size, GetLastError());
            ok(ext_pen.elp.elpHatch == HS_CROSS, "expected HS_CROSS, got %p\n", (void *)ext_pen.elp.elpHatch);
            ok(ext_pen.elp.elpNumEntries == 0, "expected 0, got %lx\n", ext_pen.elp.elpNumEntries);
            break;
        }

if (pen[i].style == PS_USERSTYLE)
{
    todo_wine
        ok(ext_pen.elp.elpPenStyle == pen[i].style, "expected %x, got %lx\n", pen[i].style, ext_pen.elp.elpPenStyle);
}
else
        ok(ext_pen.elp.elpPenStyle == pen[i].style, "expected %x, got %lx\n", pen[i].style, ext_pen.elp.elpPenStyle);
        ok(ext_pen.elp.elpWidth == 1, "expected 1, got %lx\n", ext_pen.elp.elpWidth);
        ok(ext_pen.elp.elpColor == pen[i].ret_color, "expected %08lx, got %08lx\n", pen[i].ret_color, ext_pen.elp.elpColor);
        ok(ext_pen.elp.elpBrushStyle == BS_SOLID, "expected BS_SOLID, got %x\n", ext_pen.elp.elpBrushStyle);

        DeleteObject(hpen);

test_geometric_pens:
        /********************** geometric pens **********************/
        lb.lbStyle = BS_SOLID;
        lb.lbColor = pen[i].color;
        lb.lbHatch = HS_CROSS; /* just in case */
        SetLastError(0xdeadbeef);
        hpen = ExtCreatePen(PS_GEOMETRIC | pen[i].style, pen[i].width, &lb, 2, user_style);
        if (pen[i].style != PS_USERSTYLE)
        {
            ok(hpen == 0, "ExtCreatePen should fail\n");
            SetLastError(0xdeadbeef);
            hpen = ExtCreatePen(PS_GEOMETRIC | pen[i].style, pen[i].width, &lb, 0, NULL);
        }
        if (pen[i].style == PS_ALTERNATE)
        {
            /* This style is applicable only for cosmetic pens */
            ok(hpen == 0, "ExtCreatePen should fail\n");
            continue;
        }
        ok(hpen != 0, "ExtCreatePen error %ld\n", GetLastError());

        obj_type = GetObjectType(hpen);
        /* for some reason XP differentiates PS_NULL here */
        if (pen[i].style == PS_NULL)
            ok(obj_type == OBJ_PEN, "wrong object type %lu\n", obj_type);
        else
            ok(obj_type == OBJ_EXTPEN, "wrong object type %lu\n", obj_type);

        /* check what's the real size of the object */
        size = GetObject(hpen, 0, NULL);
        switch (pen[i].style)
        {
        case PS_NULL:
            ok(size == sizeof(LOGPEN),
               "GetObject returned %d, error %ld\n", size, GetLastError());
            break;

        case PS_USERSTYLE:
            ok(size == sizeof(EXTLOGPEN) - sizeof(elp.elpStyleEntry) + sizeof(user_style),
               "GetObject returned %d, error %ld\n", size, GetLastError());
            break;

        default:
            ok(size == sizeof(EXTLOGPEN) - sizeof(elp.elpStyleEntry),
               "GetObject returned %d, error %ld\n", size, GetLastError());
            break;
        }

        /* ask for truncated data */
        memset(&lp, 0xb0, sizeof(lp));
        SetLastError(0xdeadbeef);
        size = GetObject(hpen, sizeof(lp.lopnStyle), &lp);
        ok(!size, "GetObject should fail: size %d, error %ld\n", size, GetLastError());

        memset(&lp, 0xb0, sizeof(lp));
        SetLastError(0xdeadbeef);
        size = GetObject(hpen, sizeof(lp), &lp);
        /* for some reason XP differenciates PS_NULL here */
        if (pen[i].style == PS_NULL)
        {
            ok(size == sizeof(LOGPEN), "GetObject returned %d, error %ld\n", size, GetLastError());
            ok(lp.lopnStyle == pen[i].ret_style, "expected %u, got %u\n", pen[i].ret_style, lp.lopnStyle);
            ok(lp.lopnWidth.x == pen[i].ret_width, "expected %u, got %ld\n", pen[i].ret_width, lp.lopnWidth.x);
            ok(lp.lopnWidth.y == 0, "expected 0, got %ld\n", lp.lopnWidth.y);
            ok(lp.lopnColor == pen[i].ret_color, "expected %08lx, got %08lx\n", pen[i].ret_color, lp.lopnColor);
        }
        else
            /* XP doesn't set last error here */
            ok(!size /*&& GetLastError() == ERROR_INVALID_PARAMETER*/,
               "GetObject should fail: size %d, error %ld\n", size, GetLastError());

        memset(&ext_pen, 0xb0, sizeof(ext_pen));
        SetLastError(0xdeadbeef);
        /* buffer is too small for user styles */
        size = GetObject(hpen, sizeof(elp), &ext_pen.elp);
        switch (pen[i].style)
        {
        case PS_NULL:
            ok(size == sizeof(EXTLOGPEN),
                "GetObject returned %d, error %ld\n", size, GetLastError());
            ok(ext_pen.elp.elpHatch == 0, "expected 0, got %p\n", (void *)ext_pen.elp.elpHatch);
            ok(ext_pen.elp.elpNumEntries == 0, "expected 0, got %lx\n", ext_pen.elp.elpNumEntries);

            /* for PS_NULL it also works this way */
            SetLastError(0xdeadbeef);
            size = GetObject(hpen, sizeof(ext_pen), &lp);
            ok(size == sizeof(LOGPEN),
                "GetObject returned %d, error %ld\n", size, GetLastError());
            ok(lp.lopnStyle == pen[i].ret_style, "expected %u, got %u\n", pen[i].ret_style, lp.lopnStyle);
            ok(lp.lopnWidth.x == pen[i].ret_width, "expected %u, got %ld\n", pen[i].ret_width, lp.lopnWidth.x);
            ok(lp.lopnWidth.y == 0, "expected 0, got %ld\n", lp.lopnWidth.y);
            ok(lp.lopnColor == pen[i].ret_color, "expected %08lx, got %08lx\n", pen[i].ret_color, lp.lopnColor);
            break;

        case PS_USERSTYLE:
            ok(!size /*&& GetLastError() == ERROR_INVALID_PARAMETER*/,
               "GetObject should fail: size %d, error %ld\n", size, GetLastError());
            size = GetObject(hpen, sizeof(ext_pen), &ext_pen.elp);
            ok(size == sizeof(EXTLOGPEN) - sizeof(elp.elpStyleEntry) + sizeof(user_style),
               "GetObject returned %d, error %ld\n", size, GetLastError());
            ok(ext_pen.elp.elpHatch == HS_CROSS, "expected HS_CROSS, got %p\n", (void *)ext_pen.elp.elpHatch);
            ok(ext_pen.elp.elpNumEntries == 2, "expected 0, got %lx\n", ext_pen.elp.elpNumEntries);
            ok(ext_pen.elp.elpStyleEntry[0] == 0xabc, "expected 0xabc, got %lx\n", ext_pen.elp.elpStyleEntry[0]);
            ok(ext_pen.elp.elpStyleEntry[1] == 0xdef, "expected 0xabc, got %lx\n", ext_pen.elp.elpStyleEntry[1]);
            break;

        default:
            ok(size == sizeof(EXTLOGPEN) - sizeof(elp.elpStyleEntry),
               "GetObject returned %d, error %ld\n", size, GetLastError());
            ok(ext_pen.elp.elpHatch == HS_CROSS, "expected HS_CROSS, got %p\n", (void *)ext_pen.elp.elpHatch);
            ok(ext_pen.elp.elpNumEntries == 0, "expected 0, got %lx\n", ext_pen.elp.elpNumEntries);
            break;
        }

        /* for some reason XP differenciates PS_NULL here */
        if (pen[i].style == PS_NULL)
            ok(ext_pen.elp.elpPenStyle == pen[i].ret_style, "expected %x, got %lx\n", pen[i].ret_style, ext_pen.elp.elpPenStyle);
        else
        {
if (pen[i].style == PS_USERSTYLE)
{
    todo_wine
            ok(ext_pen.elp.elpPenStyle == (PS_GEOMETRIC | pen[i].style), "expected %x, got %lx\n", PS_GEOMETRIC | pen[i].style, ext_pen.elp.elpPenStyle);
}
else
            ok(ext_pen.elp.elpPenStyle == (PS_GEOMETRIC | pen[i].style), "expected %x, got %lx\n", PS_GEOMETRIC | pen[i].style, ext_pen.elp.elpPenStyle);
        }

        if (pen[i].style == PS_NULL)
            ok(ext_pen.elp.elpWidth == 0, "expected 0, got %lx\n", ext_pen.elp.elpWidth);
        else
            ok(ext_pen.elp.elpWidth == pen[i].ret_width, "expected %u, got %lx\n", pen[i].ret_width, ext_pen.elp.elpWidth);
        ok(ext_pen.elp.elpColor == pen[i].ret_color, "expected %08lx, got %08lx\n", pen[i].ret_color, ext_pen.elp.elpColor);
        ok(ext_pen.elp.elpBrushStyle == BS_SOLID, "expected BS_SOLID, got %x\n", ext_pen.elp.elpBrushStyle);

        DeleteObject(hpen);
    }
}

START_TEST(pen)
{
    test_logpen();
}
