/* Unit test suite for imagelist control.
 *
 * Copyright 2004 Michael Stefaniuc
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <assert.h>
#include <windows.h>
#include <commctrl.h>

#include "wine/test.h"

HDC desktopDC;

static HIMAGELIST createImageList(cx, cy)
{
    /* Create an ImageList and put an image into it */
    HDC hdc = CreateCompatibleDC(desktopDC);
    HIMAGELIST himl = ImageList_Create(cx, cy, ILC_COLOR, 1, 1);
    HBITMAP hbm = CreateCompatibleBitmap(hdc, cx, cy);

    SelectObject(hdc, hbm);
    ImageList_Add(himl, hbm, NULL);
    DeleteObject(hbm);
    DeleteDC(hdc);

    return himl;
}

static void testHotspot (void)
{
    struct hotspot {
        int dx;
        int dy;
    };

#define SIZEX1 47
#define SIZEY1 31
#define SIZEX2 11
#define SIZEY2 17
#define HOTSPOTS_MAX 4       /* Number of entries in hotspots */
    static const struct hotspot hotspots[HOTSPOTS_MAX] = {
        { 10, 7 },
        { SIZEX1, SIZEY1 },
        { -9, -8 },
        { -7, 35 }
    };
    int i, j, ret;
    HIMAGELIST himl1 = createImageList(SIZEX1, SIZEY1);
    HIMAGELIST himl2 = createImageList(SIZEX2, SIZEY2);

    for (i = 0; i < HOTSPOTS_MAX; i++) {
        for (j = 0; j < HOTSPOTS_MAX; j++) {
            int dx1 = hotspots[i].dx;
            int dy1 = hotspots[i].dy;
            int dx2 = hotspots[j].dx;
            int dy2 = hotspots[j].dy;
            int correctx, correcty, newx, newy;
            HIMAGELIST himlNew;
            POINT ppt;

            ret = ImageList_BeginDrag(himl1, 0, dx1, dy1);
            ok(ret != 0, "BeginDrag failed for { %d, %d }\n", dx1, dy1);
            /* check merging the dragged image with a second image */
            ret = ImageList_SetDragCursorImage(himl2, 0, dx2, dy2);
            ok(ret != 0, "SetDragCursorImage failed for {%d, %d}{%d, %d}\n",
                    dx1, dy1, dx2, dy2);
            /* check new hotspot, it should be the same like the old one */
            himlNew = ImageList_GetDragImage(NULL, &ppt);
            ok(ppt.x == dx1 && ppt.y == dy1,
                    "Expected drag hotspot [%d,%d] got [%ld,%ld]\n",
                    dx1, dy1, ppt.x, ppt.y);
            /* check size of new dragged image */
            ImageList_GetIconSize(himlNew, &newx, &newy);
            correctx = max(SIZEX1, max(SIZEX2 + dx2, SIZEX1 - dx2));
            correcty = max(SIZEY1, max(SIZEY2 + dy2, SIZEY1 - dy2));
            ok(newx == correctx && newy == correcty,
                    "Expected drag image size [%d,%d] got [%d,%d]\n",
                    correctx, correcty, newx, newy);
            ImageList_EndDrag();
        }
    }
#undef SIZEX1
#undef SIZEY1
#undef SIZEX2
#undef SIZEY2
#undef HOTSPOTS_MAX
}

START_TEST(imagelist)
{
    desktopDC=GetDC(NULL);

    InitCommonControls();

    testHotspot();
}
