/*
 *  ImageList implementation
 *
 *  Copyright 1998 Eric Kohl
 *
 *  TODO:
 *    - Use device independent bitmaps (DIBs) instead of device
 *      dependent bitmaps (DDBs).
 *    - Fix image selection (ImageList_Draw).
 *    - Improve error checking.
 *    - Add missing functions.
 *    - Many many other things. (undocumented functions)
 */

/* This must be defined because the HIMAGELIST type is just a pointer
 * to the _IMAGELIST data structure. But MS does not want us to know
 * anything about its contents. Applications just see a pointer to
 * a struct without any content. It's just to keep compatibility.
 */
#define __WINE_IMAGELIST_C
 
/* This must be defined until "GetIconInfo" is implemented completely.
 * To do that the Cursor and Icon code in objects/cursoricon.c must
 * be rewritten.
 */
#define __GET_ICON_INFO_HACK__ 
 
#include <stdlib.h>
#include "windows.h"
#include "imagelist.h"
#include "commctrl.h"
#include "debug.h"

#ifdef __GET_ICON_INFO_HACK__
#include "bitmap.h"
#endif


static void
IMAGELIST_GrowBitmaps (HIMAGELIST himl, INT32 nImageCount)
{
    HDC32     hdcScreen, hdcImageList, hdcBitmap;
    HBITMAP32 hbmNewBitmap;
    INT32     nNewWidth;

    /* no space left for new Image(s) ==> create new Bitmap(s) */
    printf ("IMAGELIST_GrowBitmaps: Create grown bitmaps!!\n");

    nNewWidth = (himl->cCurImage + nImageCount + himl->cGrow) * himl->cx;

    hdcScreen = GetDC32 (GetDesktopWindow32 ());
    hdcImageList = CreateCompatibleDC32 (hdcScreen);
    hdcBitmap = CreateCompatibleDC32 (hdcScreen);
    ReleaseDC32 (hdcScreen, GetDesktopWindow32 ());

    hbmNewBitmap =
        CreateCompatibleBitmap32 (hdcImageList, nNewWidth, himl->cy);

    SelectObject32 (hdcImageList, himl->hbmImage);
    SelectObject32 (hdcBitmap, hbmNewBitmap);
    BitBlt32 (hdcBitmap, 0, 0, himl->cCurImage * himl->cx, himl->cy,
              hdcImageList, 0, 0, SRCCOPY);

    DeleteObject32 (himl->hbmImage);
    himl->hbmImage = hbmNewBitmap;

    if (himl->hbmMask) {
        hbmNewBitmap = 
            CreateCompatibleBitmap32 (hdcImageList, nNewWidth, himl->cy); 
        SelectObject32 (hdcImageList, himl->hbmMask);
        SelectObject32 (hdcBitmap, hbmNewBitmap);
        BitBlt32 (hdcBitmap, 0, 0, himl->cCurImage * himl->cx, himl->cy,
                  hdcImageList, 0, 0, SRCCOPY);
        DeleteObject32 (himl->hbmMask);
        himl->hbmMask = hbmNewBitmap;
    }

    DeleteDC32 (hdcImageList);
    DeleteDC32 (hdcBitmap);
}


INT32 WINAPI
ImageList_Add (HIMAGELIST himl, HBITMAP32 hbmImage, HBITMAP32 hbmMask)
{
    HDC32    hdcImageList, hdcImage, hdcMask;
    INT32    nFirstIndex, nImageCount;
    INT32    nStartX, nRunX, nRunY;
    COLORREF clrColor;
    BITMAP32 bmp;

    GetObject32A (hbmImage, sizeof(BITMAP32), (LPVOID)&bmp);
    nImageCount = bmp.bmWidth / himl->cx;

    if (himl->cCurImage + nImageCount >= himl->cMaxImage)
        IMAGELIST_GrowBitmaps (himl, nImageCount);

    hdcImageList = CreateCompatibleDC32 (0);
    hdcImage = CreateCompatibleDC32 (0);

    SelectObject32 (hdcImageList, himl->hbmImage);
    SelectObject32 (hdcImage, hbmImage);

    BitBlt32 (hdcImageList, himl->cCurImage * himl->cx, 0,
              bmp.bmWidth, himl->cy, hdcImage, 0, 0, SRCCOPY);
          
    if (himl->hbmMask) {
        if (hbmMask) {
            SelectObject32 (hdcImageList, himl->hbmMask);
            SelectObject32 (hdcImage, hbmMask);
            BitBlt32 (hdcImageList, himl->cCurImage * himl->cx, 0,
                      bmp.bmWidth, himl->cy, hdcImage, 0, 0, SRCCOPY);
        }
        else {
            /* create mask from the imagelist's background color */
            hdcMask = CreateCompatibleDC32 (0);
            SelectObject32 (hdcMask, himl->hbmMask);
            nStartX = himl->cCurImage * himl->cx;
            for (nRunY = 0; nRunY < himl->cy; nRunY++) {
                for (nRunX = 0; nRunX < bmp.bmWidth; nRunX++) {
                    clrColor = GetPixel32 (hdcImage, nRunX, nRunY);
                    if (clrColor == himl->clrBk) {
                        SetPixel32 (hdcImageList, nStartX + nRunX, nRunY, 
                                    RGB(0, 0, 0));
                        SetPixel32 (hdcMask, nStartX + nRunX, nRunY, 
                                    RGB(255, 255, 255));
                    }
                    else
                        SetPixel32 (hdcMask, nStartX + nRunX, nRunY, 
                                    RGB(0, 0, 0));        
                }
            }
            DeleteDC32 (hdcMask);
        }
    }

    DeleteDC32 (hdcImageList);
    DeleteDC32 (hdcImage);

    nFirstIndex = himl->cCurImage;
    himl->cCurImage += nImageCount;

    return (nFirstIndex);
}


INT32 WINAPI
ImageList_AddMasked (HIMAGELIST himl, HBITMAP32 hbmImage, COLORREF crMask)
{
    HDC32    hdcImageList, hdcImage, hdcMask;
    INT32    nIndex, nImageCount;
    BITMAP32 bmp;
    INT32    nStartX, nRunX, nRunY;
    COLORREF crColor;

    GetObject32A (hbmImage, sizeof(BITMAP32), &bmp);
    nImageCount = bmp.bmWidth / himl->cx;

    if (himl->cCurImage + nImageCount >= himl->cMaxImage)
        IMAGELIST_GrowBitmaps (himl, nImageCount);

    nIndex = himl->cCurImage;
    himl->cCurImage += nImageCount;

    hdcImageList = CreateCompatibleDC32 (0);
    hdcImage = CreateCompatibleDC32 (0);

    SelectObject32 (hdcImageList, himl->hbmImage);
    SelectObject32 (hdcImage, hbmImage);
    BitBlt32 (hdcImageList, nIndex * himl->cx, 0,
              bmp.bmWidth, himl->cy, hdcImage, 0, 0, SRCCOPY);

    if (himl->hbmMask) {
        /* create Mask */
        hdcMask = CreateCompatibleDC32 (0);
        SelectObject32 (hdcMask, himl->hbmMask);
        nStartX = nIndex * himl->cx;
        for (nRunY = 0; nRunY < himl->cy; nRunY++) {
            for (nRunX = 0; nRunX < bmp.bmWidth; nRunX++) {
                crColor = GetPixel32 (hdcImage, nRunX, nRunY);
                if (crColor == crMask) {
                    SetPixel32 (hdcImageList, nStartX + nRunX, nRunY,
                                RGB(0, 0, 0));
                    SetPixel32 (hdcMask, nStartX + nRunX, nRunY,
                                RGB(255, 255, 255));
                }
                else
                    SetPixel32 (hdcMask, nStartX + nRunX, nRunY, RGB(0, 0, 0));
            }
        }
        DeleteDC32 (hdcMask);
    }

    DeleteDC32 (hdcImageList);
    DeleteDC32 (hdcImage);
  
    return (nIndex);
}


HIMAGELIST WINAPI
ImageList_Create (INT32 cx, INT32 cy, UINT32 flags, INT32 cInitial, INT32 cGrow)
{
    HIMAGELIST himl;
    HANDLE32 hHeap;
    HDC32 hdcDesktop;
    HWND32 hwndDesktop;

    hHeap = GetProcessHeap ();
    himl = (HIMAGELIST) HeapAlloc (hHeap, 0, sizeof(struct _IMAGELIST));
    if (!himl)
        return (0);
    himl->hHeap = hHeap;
    himl->cx = cx;
    himl->cy = cy;
    himl->flags = flags;
    himl->cMaxImage = cInitial + cGrow;
    himl->cGrow = cGrow;
    himl->cCurImage = 0;
    himl->clrBk = CLR_NONE;  /* ??? or CLR_DEFAULT */
    himl->nOvlIdx[0] = -1;
    himl->nOvlIdx[1] = -1;
    himl->nOvlIdx[2] = -1;
    himl->nOvlIdx[3] = -1;

    hwndDesktop = GetDesktopWindow32();
    hdcDesktop = GetDC32 (hwndDesktop);
    
    himl->hbmImage =
        CreateCompatibleBitmap32 (hdcDesktop, himl->cx * himl->cMaxImage, 
                                  himl->cy);
    if (himl->flags & ILC_MASK)
        himl->hbmMask = 
            CreateCompatibleBitmap32 (hdcDesktop, himl->cx * himl->cMaxImage, 
                                      himl->cy); 
    else
        himl->hbmMask = 0;

    ReleaseDC32 (hwndDesktop, hdcDesktop);
    
    return (himl);    
}


BOOL32 WINAPI
ImageList_Destroy (HIMAGELIST himl)
{
    if (himl->hbmImage)
        DeleteObject32 (himl->hbmImage);
    if (himl->hbmMask)
        DeleteObject32 (himl->hbmMask);
        
    HeapFree (himl->hHeap, 0, (LPVOID)himl);
    return (TRUE);
}



BOOL32 WINAPI
ImageList_Draw (HIMAGELIST himl, INT32 i, HDC32 hdc, 
                INT32 x, INT32 y, UINT32 fStyle)
{
    HDC32     hdcImageList,hdcMask;
    HBITMAP32 hbmMask;
    HBRUSH32 hBrush, hOldBrush;
    INT32 nOvlIdx;

    hdcImageList = CreateCompatibleDC32 (0);
  
    if (himl->hbmMask) {  
        SelectObject32 (hdcImageList, himl->hbmMask);
        BitBlt32 (hdc, x, y, himl->cx, himl->cy, hdcImageList,
                  himl->cx * i, 0, SRCAND);  
    }  

    SelectObject32 (hdcImageList, himl->hbmImage);

    BitBlt32 (hdc, x, y, himl->cx, himl->cy, hdcImageList,
              himl->cx * i, 0, SRCPAINT);

    /* Draw overlay image */
    if (fStyle & 0x0700) {
        nOvlIdx = (fStyle & 0x0700) >> 8;
        if ((nOvlIdx >= 1) && (nOvlIdx <= 4)) {
            nOvlIdx = himl->nOvlIdx[nOvlIdx - 1];
            if ((nOvlIdx >= 0) && (nOvlIdx <= himl->cCurImage)) {

                if (himl->hbmMask) {  
                    SelectObject32 (hdcImageList, himl->hbmMask);
                    BitBlt32 (hdc, x, y, himl->cx, himl->cy, hdcImageList,
                              himl->cx * nOvlIdx, 0, SRCAND);  
                }  
                SelectObject32 (hdcImageList, himl->hbmImage);
                BitBlt32 (hdc, x, y, himl->cx, himl->cy, hdcImageList,
                          himl->cx * nOvlIdx, 0, SRCPAINT);

            }
        }
    }

    DeleteDC32 (hdcImageList);
  
    return (TRUE);
}



COLORREF WINAPI
ImageList_GetBkColor (HIMAGELIST himl)
{
    return (himl->clrBk);
}


HICON32 WINAPI
ImageList_GetIcon (HIMAGELIST himl, INT32 i, UINT32 flags)
{


    return (0);
}


BOOL32 WINAPI
ImageList_GetIconSize (HIMAGELIST himl, INT32 *cx, INT32 *cy)
{
    *cx = himl->cx;
    *cy = himl->cy;
    return (TRUE);
}


INT32 WINAPI
ImageList_GetImageCount (HIMAGELIST himl)
{
    return (himl->cCurImage);
}


BOOL32 WINAPI
ImageList_GetImageInfo (HIMAGELIST himl, INT32 i, IMAGEINFO *pImageInfo)
{
    pImageInfo->hbmImage = himl->hbmImage;
    pImageInfo->hbmMask  = himl->hbmMask;
    
    pImageInfo->rcImage.top    = 0;
    pImageInfo->rcImage.bottom = himl->cy;
    pImageInfo->rcImage.left   = i * himl->cx;
    pImageInfo->rcImage.right  = (i+1) * himl->cx;
    
    return (TRUE);
}


HIMAGELIST WINAPI
ImageList_LoadImage32A (HINSTANCE32 hi, LPCSTR lpbmp, INT32 cx, INT32 cGrow, 
                        COLORREF clrMask, UINT32 uType, UINT32 uFlags)
{
    HIMAGELIST himl = NULL;
    HANDLE32   handle;
    INT32      nImageCount;

    handle = LoadImage32A (hi, lpbmp, uType, 0, 0, uFlags);

    if (uType == IMAGE_BITMAP) {
        BITMAP32 bmp;
        GetObject32A (handle, sizeof(BITMAP32), &bmp);
        nImageCount = bmp.bmWidth / cx;

        himl = ImageList_Create (cx, bmp.bmHeight, ILC_MASK | ILC_COLOR,
                                 nImageCount, cGrow);
        ImageList_AddMasked (himl, (HBITMAP32)handle, clrMask);
    }
    else if ((uType == IMAGE_ICON) || (uType == IMAGE_CURSOR)) {
#ifdef __GET_ICON_INFO_HACK__
        HBITMAP32 hbmImage;
        HBITMAP32 hbmMask;
        CURSORICONINFO *ptr;

        if (!(ptr = (CURSORICONINFO *)GlobalLock16(handle))) return (NULL);
        hbmMask  = CreateBitmap32 (ptr->nWidth, ptr->nHeight, 1, 1, 
                                   (char *)(ptr + 1));
        hbmImage = CreateBitmap32 (ptr->nWidth, ptr->nHeight, ptr->bPlanes,
                                   ptr->bBitsPerPixel,
                                   (char *)(ptr + 1) + ptr->nHeight * 
                                   BITMAP_WIDTH_BYTES(ptr->nWidth, 1));
        himl = ImageList_Create (ptr->nWidth, ptr->nHeight,
                                 ILC_MASK | ILC_COLOR, 1, cGrow);
        ImageList_Add (himl, hbmImage, hbmMask);
        DeleteObject32 (hbmImage);
        DeleteObject32 (hbmMask);
        GlobalUnlock16 (handle);
#else
        ICONINFO32 ii;
        BITMAP32 bmp;

        GetIconInfo (hIcon, &ii);
        GetObject32A (ii->hbmMask, sizeof(BITMAP32), (LPVOID)&bmp);
        himl = ImageList_Create (bmp.bmWidth, bmp.bmHeight, 
                                 ILC_MASK | ILC_COLOR, 1, cGrow);
        ImageList_Add (himl, ii->hbmColor, ii->hbmMask);
#endif
    }

    DeleteObject32 (handle);
    
    return (himl);
}


HIMAGELIST WINAPI
ImageList_LoadImage32W (HINSTANCE32 hi, LPCWSTR lpbmp, INT32 cx, INT32 cGrow, 
                        COLORREF clrMask, UINT32 uType, UINT32 uFlags)
{
    HIMAGELIST himl = NULL;
    HANDLE32   handle;
    INT32      nImageCount;

    handle = LoadImage32W (hi, lpbmp, uType, 0, 0, uFlags);

    if (uType == IMAGE_BITMAP) {
        BITMAP32 bmp;
        GetObject32A (handle, sizeof(BITMAP32), &bmp);
        nImageCount = bmp.bmWidth / cx;

        himl = ImageList_Create (cx, bmp.bmHeight, ILC_MASK | ILC_COLOR,
                                 nImageCount, cGrow);
        ImageList_AddMasked (himl, (HBITMAP32)handle, clrMask);
    }
    else if ((uType == IMAGE_ICON) || (uType == IMAGE_CURSOR)) {
#ifdef __GET_ICON_INFO_HACK__
        HBITMAP32 hbmImage;
        HBITMAP32 hbmMask;
        CURSORICONINFO *ptr;

        if (!(ptr = (CURSORICONINFO *)GlobalLock16(handle))) return (NULL);
        hbmMask  = CreateBitmap32 (ptr->nWidth, ptr->nHeight, 1, 1, 
                                   (char *)(ptr + 1));
        hbmImage = CreateBitmap32 (ptr->nWidth, ptr->nHeight, ptr->bPlanes,
                                   ptr->bBitsPerPixel,
                                   (char *)(ptr + 1) + ptr->nHeight * 
                                   BITMAP_WIDTH_BYTES(ptr->nWidth, 1));
        himl = ImageList_Create (ptr->nWidth, ptr->nHeight,
                                 ILC_MASK | ILC_COLOR, 1, cGrow);
        ImageList_Add (himl, hbmImage, hbmMask);
        DeleteObject32 (hbmImage);
        DeleteObject32 (hbmMask);
        GlobalUnlock16 (handle);
#else
        ICONINFO32 ii;
        BITMAP32 bmp;

        GetIconInfo (hIcon, &ii);
        GetObject32A (ii->hbmMask, sizeof(BITMAP32), (LPVOID)&bmp);
        himl = ImageList_Create (bmp.bmWidth, bmp.bmHeight, 
                                 ILC_MASK | ILC_COLOR, 1, cGrow);
        ImageList_Add (himl, ii->hbmColor, ii->hbmMask);
#endif
    }

    DeleteObject32 (handle);
    
    return (himl);
}


HIMAGELIST WINAPI
ImageList_Merge (HIMAGELIST himl1, INT32 i1, HIMAGELIST himl2, INT32 i2,
                 INT32 xOffs, INT32 yOffs)
{
    HIMAGELIST himlDst = NULL;
    HDC32      hdcSrcImage, hdcDstImage;
    INT32      nX1, nX2;

    himlDst = ImageList_Create (himl1->cx, himl1->cy,
                                ILC_MASK | ILC_COLOR, 1, 1);
    if (himlDst) {
        hdcSrcImage = CreateCompatibleDC32 (0);
        hdcDstImage = CreateCompatibleDC32 (0);
        nX1 = i1 * himl1->cx;
        nX2 = i2 * himl2->cx;
        
        /* copy image */
        SelectObject32 (hdcSrcImage, himl1->hbmImage);
        SelectObject32 (hdcDstImage, himlDst->hbmImage);
        BitBlt32 (hdcDstImage, 0, 0, himlDst->cx, himlDst->cy, 
                  hdcSrcImage, nX1, 0, SRCCOPY);  /* SRCCOPY */

        SelectObject32 (hdcSrcImage, himl2->hbmMask);
        BitBlt32 (hdcDstImage, xOffs, yOffs, 
                  himlDst->cx - xOffs, himlDst->cy - yOffs, 
                  hdcSrcImage, nX2, 0, SRCAND);

        SelectObject32 (hdcSrcImage, himl2->hbmImage);
        BitBlt32 (hdcDstImage, xOffs, yOffs, 
                  himlDst->cx - xOffs, himlDst->cy - yOffs, 
                  hdcSrcImage, nX2, 0, SRCPAINT);

        /* copy mask */
        SelectObject32 (hdcSrcImage, himl1->hbmMask);
        SelectObject32 (hdcDstImage, himlDst->hbmMask);
        BitBlt32 (hdcDstImage, 0, 0, himlDst->cx, himlDst->cy, 
                  hdcSrcImage, nX1, 0, SRCCOPY);

        SelectObject32 (hdcSrcImage, himl2->hbmMask);
        BitBlt32 (hdcDstImage, xOffs, yOffs, 
                  himlDst->cx - xOffs, himlDst->cy - yOffs, 
                  hdcSrcImage, nX2, 0, SRCAND);

        DeleteDC32 (hdcSrcImage);
        DeleteDC32 (hdcDstImage);
    }
   
    return (himlDst);
}



BOOL32 WINAPI
ImageList_Replace (HIMAGELIST himl, INT32 i,  HBITMAP32 hbmImage, 
                   HBITMAP32 hbmMask)
{
    HDC32 hdcImageList, hdcImage;

    if ((i >= himl->cCurImage) || (i < 0)) return (FALSE);

    hdcImageList = CreateCompatibleDC32 (0);
    hdcImage = CreateCompatibleDC32 (0);

    /* Replace Image */
    SelectObject32 (hdcImageList, himl->hbmImage);
    SelectObject32 (hdcImage, hbmImage);

    BitBlt32 (hdcImageList, i * himl->cx, 0,
              himl->cx, himl->cy, hdcImage, 0, 0, SRCCOPY);

    if (himl->hbmMask) {
        /* Replace Mask */
        SelectObject32 (hdcImageList, himl->hbmMask);
        SelectObject32 (hdcImage, hbmMask);

        BitBlt32 (hdcImageList, i * himl->cx, 0,
                  himl->cx, himl->cy, hdcImage, 0, 0, SRCCOPY);
    }

    DeleteDC32 (hdcImage);
    DeleteDC32 (hdcImageList);

    return (TRUE);
}


INT32 WINAPI
ImageList_ReplaceIcon (HIMAGELIST himl, INT32 i, HICON32 hIcon)
{
    HDC32     hdcImageList, hdcImage;
    INT32     nIndex;
#ifdef __GET_ICON_INFO_HACK__
    HBITMAP32 hbmImage;
    HBITMAP32 hbmMask;
    CURSORICONINFO *ptr;
#else    
    ICONINFO32 ii;
    BITMAP32 bmp;
#endif

    if ((i >= himl->cCurImage) || (i < -1)) return (-1);

#ifdef __GET_ICON_INFO_HACK__
    if (!(ptr = (CURSORICONINFO *)GlobalLock16(hIcon))) return (-1);
    hbmMask  = CreateBitmap32 (ptr->nWidth, ptr->nHeight, 1, 1, 
                               (char *)(ptr + 1));
    hbmImage = CreateBitmap32 (ptr->nWidth, ptr->nHeight, ptr->bPlanes,
                               ptr->bBitsPerPixel,
                               (char *)(ptr + 1) + ptr->nHeight * 
                               BITMAP_WIDTH_BYTES(ptr->nWidth, 1));
#else
    GetIconInfo (hIcon, &ii);
    GetObject32A (ii->hbmMask, sizeof(BITMAP32), (LPVOID)&bmp);
#endif

    if (i == -1) {
        if (himl->cCurImage + 1 >= himl->cMaxImage)
            IMAGELIST_GrowBitmaps (himl, 1);
        nIndex = himl->cCurImage;
        himl->cCurImage ++;
    }
    else
        nIndex = i;

    hdcImageList = CreateCompatibleDC32 (0);
    hdcImage = CreateCompatibleDC32 (0);

#ifdef __GET_ICON_INFO_HACK__
    SelectObject32 (hdcImageList, himl->hbmImage);
    SelectObject32 (hdcImage, hbmImage);
#else
    SelectObject32 (hdcImage, ii->hbmColor);
#endif

    BitBlt32 (hdcImageList, nIndex * himl->cx, 0,
              himl->cx, himl->cy, hdcImage, 0, 0, SRCCOPY);

    if (himl->hbmMask) {
#ifdef __GET_ICON_INFO_HACK__
        SelectObject32 (hdcImageList, himl->hbmMask);
        SelectObject32 (hdcImage, hbmMask);
#else
        SelectObject32 (hdcImage, ii->hbmMask);
#endif
        BitBlt32 (hdcImageList, nIndex * himl->cx, 0,
                  himl->cx, himl->cy, hdcImage, 0, 0, SRCCOPY);
    }

    DeleteDC32 (hdcImageList);
    DeleteDC32 (hdcImage);
#ifdef __GET_ICON_INFO_HACK        
    DeleteObject32 (hbmImage);
    DeleteObject32 (hbmMask);
    GlobalUnlock16 (hIcon);
#endif
    return (nIndex);
}


COLORREF WINAPI
ImageList_SetBkColor (HIMAGELIST himl, COLORREF clrBk)
{
    COLORREF clrOldBk;

    clrOldBk = himl->clrBk;
    himl->clrBk = clrBk;
    return (clrOldBk);
}



BOOL32 WINAPI
ImageList_SetOverlayImage (HIMAGELIST himl, INT32 iImage, INT32 iOverlay)
{
    if ((iOverlay < 1) || (iOverlay > 4)) return (FALSE);
    if ((iImage < 0) || (iImage > himl->cCurImage)) return (FALSE);
    
    himl->nOvlIdx[iOverlay - 1] = iImage;
    return (TRUE);
}

