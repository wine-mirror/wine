/* Unit test suite for imagelist control.
 *
 * Copyright 2004 Michael Stefaniuc
 * Copyright 2002 Mike McCormack for CodeWeavers
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

static BOOL (WINAPI *pImageList_DrawIndirect)(IMAGELISTDRAWPARAMS*) = NULL;

static HDC desktopDC;

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

static HINSTANCE hinst;

static const BYTE icon_bits[32*32/8];
static const BYTE bitmap_bits[48*48/8];

static BOOL DoTest1(void)
{
    HIMAGELIST himl ;

    HICON hicon1 ;
    HICON hicon2 ;
    HICON hicon3 ;

    /* create an imagelist to play with */
    himl = ImageList_Create(84,84,0x10,0,3);
    ok(himl!=0,"failed to create imagelist\n");

    /* load the icons to add to the image list */
    hicon1 = CreateIcon(hinst, 32, 32, 1, 1, icon_bits, icon_bits);
    ok(hicon1 != 0, "no hicon1\n");
    hicon2 = CreateIcon(hinst, 32, 32, 1, 1, icon_bits, icon_bits);
    ok(hicon2 != 0, "no hicon2\n");
    hicon3 = CreateIcon(hinst, 32, 32, 1, 1, icon_bits, icon_bits);
    ok(hicon3 != 0, "no hicon3\n");

    /* remove when nothing exists */
    ok(!ImageList_Remove(himl,0),"removed non-existent icon\n");
    /* removing everything from an empty imagelist should succeed */
    ok(ImageList_RemoveAll(himl),"removed non-existent icon\n");

    /* add three */
    ok(0==ImageList_AddIcon(himl, hicon1),"failed to add icon1\n");
    ok(1==ImageList_AddIcon(himl, hicon2),"failed to add icon2\n");
    ok(2==ImageList_AddIcon(himl, hicon3),"failed to add icon3\n");

    /* remove an index out of range */
    ok(!ImageList_Remove(himl,4711),"removed non-existent icon\n");

    /* remove three */
    ok(ImageList_Remove(himl,0),"can't remove 0\n");
    ok(ImageList_Remove(himl,0),"can't remove 0\n");
    ok(ImageList_Remove(himl,0),"can't remove 0\n");

    /* remove one extra */
    ok(!ImageList_Remove(himl,0),"removed non-existent icon\n");

    /* destroy it */
    ok(ImageList_Destroy(himl),"destroy imagelist failed\n");

    /* icons should be deleted by the imagelist */
    ok(!DeleteObject(hicon1),"icon 1 wasn't deleted\n");
    ok(!DeleteObject(hicon2),"icon 2 wasn't deleted\n");
    ok(!DeleteObject(hicon3),"icon 3 wasn't deleted\n");

    return TRUE;
}

static BOOL DoTest2(void)
{
    HIMAGELIST himl ;

    HICON hicon1 ;
    HICON hicon2 ;
    HICON hicon3 ;

    /* create an imagelist to play with */
    himl = ImageList_Create(84,84,0x10,0,3);
    ok(himl!=0,"failed to create imagelist\n");

    /* load the icons to add to the image list */
    hicon1 = CreateIcon(hinst, 32, 32, 1, 1, icon_bits, icon_bits);
    ok(hicon1 != 0, "no hicon1\n");
    hicon2 = CreateIcon(hinst, 32, 32, 1, 1, icon_bits, icon_bits);
    ok(hicon2 != 0, "no hicon2\n");
    hicon3 = CreateIcon(hinst, 32, 32, 1, 1, icon_bits, icon_bits);
    ok(hicon3 != 0, "no hicon3\n");

    /* remove when nothing exists */
    ok(!ImageList_Remove(himl,0),"removed non-existent icon\n");

    /* add three */
    ok(0==ImageList_AddIcon(himl, hicon1),"failed to add icon1\n");
    ok(1==ImageList_AddIcon(himl, hicon2),"failed to add icon2\n");
    ok(2==ImageList_AddIcon(himl, hicon3),"failed to add icon3\n");

    /* destroy it */
    ok(ImageList_Destroy(himl),"destroy imagelist failed\n");

    /* icons should be deleted by the imagelist */
    ok(!DeleteObject(hicon1),"icon 1 wasn't deleted\n");
    ok(!DeleteObject(hicon2),"icon 2 wasn't deleted\n");
    ok(!DeleteObject(hicon3),"icon 3 wasn't deleted\n");

    return TRUE;
}

static HWND create_a_window(void)
{
    WNDCLASSA cls;
    char className[] = "bmwnd";
    char winName[]   = "Test Bitmap";
    HWND hWnd;

    cls.style         = CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS;
    cls.lpfnWndProc   = DefWindowProcA;
    cls.cbClsExtra    = 0;
    cls.cbWndExtra    = 0;
    cls.hInstance     = 0;
    cls.hIcon         = LoadIconA (0, (LPSTR)IDI_APPLICATION);
    cls.hCursor       = LoadCursorA (0, (LPSTR)IDC_ARROW);
    cls.hbrBackground = (HBRUSH) GetStockObject (WHITE_BRUSH);
    cls.lpszMenuName  = 0;
    cls.lpszClassName = className;

    RegisterClassA (&cls);

    /* Setup windows */
    hWnd = CreateWindowA (className, winName, 
       WS_OVERLAPPEDWINDOW ,
       CW_USEDEFAULT, CW_USEDEFAULT, 100, 100, 0,
       0, hinst, 0);

    return hWnd;
}

static BOOL DoTest3(void)
{
    HIMAGELIST himl ;

    HBITMAP hbm1 ;
    HBITMAP hbm2 ;
    HBITMAP hbm3 ;

    IMAGELISTDRAWPARAMS imldp;
    HDC hdc;
    HWND hwndfortest;

    if (!pImageList_DrawIndirect)
    {
        HMODULE hComCtl32 = LoadLibraryA("comctl32.dll");
        pImageList_DrawIndirect = (void*)GetProcAddress(hComCtl32, "ImageList_DrawIndirect");
        if (!pImageList_DrawIndirect)
        {
            trace("ImageList_DrawIndirect not available, skipping test\n");
            return TRUE;
        }
    }

    hwndfortest = create_a_window();
    hdc = GetDC(hwndfortest);
    ok(hdc!=NULL, "couldn't get DC\n");

    /* create an imagelist to play with */
    himl = ImageList_Create(48,48,0x10,0,3);
    ok(himl!=0,"failed to create imagelist\n");

    /* load the icons to add to the image list */
    hbm1 = CreateBitmap(48, 48, 1, 1, bitmap_bits);
    ok(hbm1 != 0, "no bitmap 1\n");
    hbm2 = CreateBitmap(48, 48, 1, 1, bitmap_bits);
    ok(hbm2 != 0, "no bitmap 2\n");
    hbm3 = CreateBitmap(48, 48, 1, 1, bitmap_bits);
    ok(hbm3 != 0, "no bitmap 3\n");

    /* remove when nothing exists */
    ok(!ImageList_Remove(himl,0),"removed non-existent bitmap\n");

    /* add three */
    ok(0==ImageList_Add(himl, hbm1, 0),"failed to add bitmap 1\n");
    ok(1==ImageList_Add(himl, hbm2, 0),"failed to add bitmap 2\n");

    ok(ImageList_SetImageCount(himl,3),"Setimage count failed\n");
    /*ok(2==ImageList_Add(himl, hbm3, NULL),"failed to add bitmap 3\n"); */
    ok(ImageList_Replace(himl, 2, hbm3, 0),"failed to replace bitmap 3\n");

    Rectangle(hdc, 100, 100, 74, 74);
    memset(&imldp, 0, sizeof imldp);
    ok(!pImageList_DrawIndirect(&imldp), "zero data succeeded!\n");
    imldp.cbSize = sizeof imldp;
    ok(!pImageList_DrawIndirect(&imldp), "zero hdc succeeded!\n");
    imldp.hdcDst = hdc;
    ok(!pImageList_DrawIndirect(&imldp),"zero himl succeeded!\n");
    imldp.himl = himl;
    ok(pImageList_DrawIndirect(&imldp),"should succeeded\n");
    imldp.fStyle = SRCCOPY;
    imldp.rgbBk = CLR_DEFAULT;
    imldp.rgbFg = CLR_DEFAULT;
    imldp.y = 100;
    imldp.x = 100;
    ok(pImageList_DrawIndirect(&imldp),"should succeeded\n");
    imldp.i ++;
    ok(pImageList_DrawIndirect(&imldp),"should succeeded\n");
    imldp.i ++;
    ok(pImageList_DrawIndirect(&imldp),"should succeeded\n");
    imldp.i ++;
    ok(!pImageList_DrawIndirect(&imldp),"should fail\n");

    /* remove three */
    ok(ImageList_Remove(himl, 0), "removing 1st bitmap\n");
    ok(ImageList_Remove(himl, 0), "removing 2nd bitmap\n");
    ok(ImageList_Remove(himl, 0), "removing 3rd bitmap\n");

    /* destroy it */
    ok(ImageList_Destroy(himl),"destroy imagelist failed\n");

    /* icons should be deleted by the imagelist */
    ok(DeleteObject(hbm1),"bitmap 1 can't be deleted\n");
    ok(DeleteObject(hbm2),"bitmap 2 can't be deleted\n");
    ok(DeleteObject(hbm3),"bitmap 3 can't be deleted\n");

    ReleaseDC(hwndfortest, hdc);
    DestroyWindow(hwndfortest);

    return TRUE;
}

START_TEST(imagelist)
{
    desktopDC=GetDC(NULL);
    hinst = GetModuleHandleA(NULL);

    InitCommonControls();

    testHotspot();
    DoTest1();
    DoTest2();
    DoTest3();
}
