/*
 *  ImageList implementation
 *
 *  Copyright 1998 Eric Kohl
 *
 *  TODO:
 *    - Fix xBitmap and yBitmap in ImageList_DrawIndirect.
 *    - Fix ILD_TRANSPARENT error in ImageList_DrawIndirect.
 *    - Fix ImageList_GetIcon (might be a result of the
 *      ILD_TRANSPARENT error in ImageList_DrawIndirect).
 *    - Fix drag functions.
 *    - Fix ImageList_Read and ImageList_Write.
 *    - Fix ImageList_SetFilter (undocumented).
 *      BTW does anybody know anything about this function???
 *        - It removes 12 Bytes from the stack (3 Parameters).
 *        - First parameter SHOULD be a HIMAGELIST.
 *        - Second parameter COULD be an index?????
 *        - Third parameter.... ?????????????????????
 *
 *  Comments:
 *    - ImageList_Draw, ImageList_DrawEx and ImageList_GetIcon use
 *      ImageList_DrawIndirect. Since ImageList_DrawIndirect is still
 *      partially imlemented, the functions mentioned above will be 
 *      limited in functionality too.
 */

/* This must be defined because the HIMAGELIST type is just a pointer
 * to the _IMAGELIST data structure. But M$ does not want us to know
 * anything about its contents. Applications just see a pointer to
 * an empty structure. It's just to keep compatibility.
 */
#define __WINE_IMAGELIST_C

 
#include "windows.h"
#include "compobj.h"
#include "storage.h"
#include "imagelist.h"
#include "commctrl.h"
#include "debug.h"


#define _MAX(a,b) (((a)>(b))?(a):(b))
#define _MIN(a,b) (((a)>(b))?(b):(a))

#define MAX_OVERLAYIMAGE 15


/* internal image list data used for Drag & Drop operations */

static HIMAGELIST himlInternalDrag = NULL;
static INT32      nInternalDragHotspotX = 0;
static INT32      nInternalDragHotspotY = 0;

static HWND32     hwndInternalDrag = 0;
static INT32      xInternalPos = 0;
static INT32      yInternalPos = 0;

static HDC32      hdcBackBuffer = 0;
static HBITMAP32  hbmBackBuffer = 0;


/*************************************************************************
 * IMAGELIST_InternalExpandBitmaps [Internal] 
 *
 * Expands the bitmaps of an image list by the given number of images.
 *
 * PARAMS
 *     himl        [I] image list handle
 *     nImageCount [I] Number of images to add.
 *
 * RETURNS
 *     nothing
 *
 * NOTES
 *     This function can NOT be used to reduce the number of images.
 */

static VOID
IMAGELIST_InternalExpandBitmaps (HIMAGELIST himl, INT32 nImageCount)
{
    HDC32     hdcImageList, hdcBitmap;
    HBITMAP32 hbmNewBitmap;
    INT32     nNewWidth, nNewCount;

    TRACE(imagelist, "Create expanded bitmaps!\n");

    nNewCount = himl->cCurImage + nImageCount + himl->cGrow;
    nNewWidth = nNewCount * himl->cx;

    hdcImageList = CreateCompatibleDC32 (0);
    hdcBitmap = CreateCompatibleDC32 (0);

    hbmNewBitmap =
        CreateBitmap32 (nNewWidth, himl->cy, 1, himl->uBitsPixel, NULL);
    if (hbmNewBitmap == 0)
        ERR (imagelist, "creating new image bitmap!\n");

    SelectObject32 (hdcImageList, himl->hbmImage);
    SelectObject32 (hdcBitmap, hbmNewBitmap);
    BitBlt32 (hdcBitmap, 0, 0, himl->cCurImage * himl->cx, himl->cy,
              hdcImageList, 0, 0, SRCCOPY);

    DeleteObject32 (himl->hbmImage);
    himl->hbmImage = hbmNewBitmap;

    if (himl->hbmMask) {
        hbmNewBitmap = 
            CreateBitmap32 (nNewWidth, himl->cy, 1, 1, NULL);

        if (hbmNewBitmap == 0)
            ERR (imagelist, "creating new mask bitmap!");

        SelectObject32 (hdcImageList, himl->hbmMask);
        SelectObject32 (hdcBitmap, hbmNewBitmap);
        BitBlt32 (hdcBitmap, 0, 0, himl->cCurImage * himl->cx, himl->cy,
                  hdcImageList, 0, 0, SRCCOPY);
        DeleteObject32 (himl->hbmMask);
        himl->hbmMask = hbmNewBitmap;
    }

    himl->cMaxImage = nNewCount;

    DeleteDC32 (hdcImageList);
    DeleteDC32 (hdcBitmap);
}


/*************************************************************************
 * ImageList_Add [COMCTL32.39]
 *
 * Add an image or images to an image list.
 *
 * PARAMS
 *     himl     [I] image list handle
 *     hbmImage [I] image bitmap handle
 *     hbmMask  [I] mask bitmap handle
 *
 * RETURNS
 *     Success: Index of the first new image.
 *     Failure: -1
 */

INT32 WINAPI
ImageList_Add (HIMAGELIST himl,	HBITMAP32 hbmImage, HBITMAP32 hbmMask)
{
    HDC32    hdcSrc, hdcDst;
    INT32    nFirstIndex, nImageCount;
    INT32    nStartX, nRunX, nRunY;
    BITMAP32 bmp;

    if (himl == NULL) return (-1);
    if (hbmImage == 0) return (-1);

    GetObject32A (hbmImage, sizeof(BITMAP32), (LPVOID)&bmp);
    nImageCount = bmp.bmWidth / himl->cx;

    if (himl->cCurImage + nImageCount >= himl->cMaxImage)
        IMAGELIST_InternalExpandBitmaps (himl, nImageCount);

    hdcSrc = CreateCompatibleDC32 (0);
    hdcDst = CreateCompatibleDC32 (0);

    SelectObject32 (hdcDst, himl->hbmImage);
    SelectObject32 (hdcSrc, hbmImage);

    BitBlt32 (hdcDst, himl->cCurImage * himl->cx, 0,
              bmp.bmWidth, himl->cy, hdcSrc, 0, 0, SRCCOPY);
          
    if (himl->hbmMask) {
        if (hbmMask) {
            SelectObject32 (hdcDst, himl->hbmMask);
            SelectObject32 (hdcSrc, hbmMask);
            BitBlt32 (hdcDst, himl->cCurImage * himl->cx, 0,
                      bmp.bmWidth, himl->cy, hdcSrc, 0, 0, SRCCOPY);

            /* fix transparent areas of the image bitmap*/
            SelectObject32 (hdcSrc, himl->hbmMask);
            SelectObject32 (hdcDst, himl->hbmImage);
            nStartX = himl->cCurImage * himl->cx;

            for (nRunY = 0; nRunY < himl->cy; nRunY++) {
                for (nRunX = 0; nRunX < bmp.bmWidth; nRunX++) {
                    if (GetPixel32 (hdcSrc, nStartX + nRunX, nRunY) !=
                        RGB(0, 0, 0))
                        SetPixel32 (hdcDst, nStartX + nRunX, nRunY, 
                                    RGB(0, 0, 0));
                }
            }
        }
        else {
            /* create mask from the imagelist's background color */
            SelectObject32 (hdcDst, himl->hbmMask);
            SelectObject32 (hdcSrc, himl->hbmImage);
            nStartX = himl->cCurImage * himl->cx;
            for (nRunY = 0; nRunY < himl->cy; nRunY++) {
                for (nRunX = 0; nRunX < bmp.bmWidth; nRunX++) {
                    if (GetPixel32 (hdcSrc, nStartX + nRunX, nRunY) ==
                        himl->clrBk)
                    {
                        SetPixel32 (hdcSrc, nStartX + nRunX, nRunY, 
                                    RGB(0, 0, 0));
                        SetPixel32 (hdcDst, nStartX + nRunX, nRunY, 
                                    RGB(255, 255, 255));
                    }
                    else
                        SetPixel32 (hdcDst, nStartX + nRunX, nRunY, 
                                    RGB(0, 0, 0));        
                }
            }
        }
    }

    DeleteDC32 (hdcSrc);
    DeleteDC32 (hdcDst);

    nFirstIndex = himl->cCurImage;
    himl->cCurImage += nImageCount;

    return (nFirstIndex);
}


/*************************************************************************
 * ImageList_AddIcon [COMCTL32.40]
 *
 * Adds an icon to an image list.
 *
 * PARAMS
 *     himl  [I] image list handle
 *     hIcon [I] icon handle
 *
 * RETURNS
 *     Success: index of the new image
 *     Failure: -1
 */

INT32 WINAPI
ImageList_AddIcon (HIMAGELIST himl, HICON32 hIcon)
{
    return (ImageList_ReplaceIcon (himl, -1, hIcon));
}


/*************************************************************************
 * ImageList_AddMasked [COMCTL32.41] 
 *
 * Adds an image or images to an image list and creates a mask from the
 * specified bitmap using the mask color.
 *
 * PARAMS
 *     himl     [I] image list handle.
 *     hbmImage [I] image bitmap handle.
 *     clrMask  [I] mask color.
 *
 * RETURNS
 *     Success: Index of the first new image.
 *     Failure: -1
 */

INT32 WINAPI
ImageList_AddMasked (HIMAGELIST himl, HBITMAP32 hbmImage, COLORREF clrMask)
{
    HDC32    hdcImageList, hdcImage, hdcMask;
    INT32    nIndex, nImageCount;
    BITMAP32 bmp;
    INT32    nStartX, nRunX, nRunY;
    COLORREF bkColor;

    if (himl == NULL)
	return (-1);

    bkColor = (clrMask == CLR_NONE) ? himl->clrBk : clrMask;

    GetObject32A (hbmImage, sizeof(BITMAP32), &bmp);
    nImageCount = bmp.bmWidth / himl->cx;

    if (himl->cCurImage + nImageCount >= himl->cMaxImage)
        IMAGELIST_InternalExpandBitmaps (himl, nImageCount);

    nIndex = himl->cCurImage;
    himl->cCurImage += nImageCount;

    hdcImageList = CreateCompatibleDC32 (0);
    hdcImage = CreateCompatibleDC32 (0);

    SelectObject32 (hdcImageList, himl->hbmImage);
    SelectObject32 (hdcImage, hbmImage);
    BitBlt32 (hdcImageList, nIndex * himl->cx, 0, bmp.bmWidth, himl->cy,
              hdcImage, 0, 0, SRCCOPY);

    if (himl->hbmMask) {
        /* create Mask */
        hdcMask = CreateCompatibleDC32 (0);
        SelectObject32 (hdcMask, himl->hbmMask);
        nStartX = nIndex * himl->cx;
        for (nRunY = 0; nRunY < himl->cy; nRunY++) {
            for (nRunX = 0; nRunX < bmp.bmWidth; nRunX++) {
                if (GetPixel32 (hdcImageList, nStartX + nRunX, nRunY) ==
                    bkColor) {
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


/*************************************************************************
 * ImageList_BeginDrag [COMCTL32.42] 
 *
 * Creates a temporary image list that contains one image. It will be used
 * as a drag image.
 *
 * PARAMS
 *     himlTrack [I] Handle of the source image list
 *     iTrack    [I] Index of the drag image in the source image list
 *     dxHotspot [I] X position of the hot spot of the drag image
 *     dyHotspot [I] Y position of the hot spot of the drag image
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL32 WINAPI
ImageList_BeginDrag (HIMAGELIST himlTrack, INT32 iTrack,
	             INT32 dxHotspot, INT32 dyHotspot)
{
    HDC32 hdcSrc, hdcDst;

    FIXME(imagelist, "partially implemented!\n");

    if (himlTrack == NULL)
	return (FALSE);

    if (himlInternalDrag)
        ImageList_EndDrag ();

    himlInternalDrag =
	ImageList_Create (himlTrack->cx, himlTrack->cy,
			  himlTrack->flags, 1, 1);
    if (himlInternalDrag == NULL) {
        ERR(imagelist, "Error creating drag image list!\n");
        return (FALSE);
    }

    nInternalDragHotspotX = dxHotspot;
    nInternalDragHotspotY = dyHotspot;

    hdcSrc = CreateCompatibleDC32 (0);
    hdcDst = CreateCompatibleDC32 (0);

    /* copy image */
    SelectObject32 (hdcSrc, himlTrack->hbmImage);
    SelectObject32 (hdcDst, himlInternalDrag->hbmImage);
    StretchBlt32 (hdcDst, 0, 0, himlInternalDrag->cx, himlInternalDrag->cy, hdcSrc,
                  iTrack * himlTrack->cx, 0, himlTrack->cx, himlTrack->cy, SRCCOPY);

    /* copy mask */
    SelectObject32 (hdcSrc, himlTrack->hbmMask);
    SelectObject32 (hdcDst, himlInternalDrag->hbmMask);
    StretchBlt32 (hdcDst, 0, 0, himlInternalDrag->cx, himlInternalDrag->cy, hdcSrc,
                  iTrack * himlTrack->cx, 0, himlTrack->cx, himlTrack->cy, SRCCOPY);

    DeleteDC32 (hdcSrc);
    DeleteDC32 (hdcDst);

    himlInternalDrag->cCurImage = 1;

    return (TRUE);
}


/*************************************************************************
 * ImageList_Copy [COMCTL32.43] 
 *
 *  Copies an image of the source image list to an image of the 
 *  destination image list. Images can be copied or swapped.
 *
 * PARAMS
 *     himlDst [I] destination image list handle.
 *     iDst    [I] destination image index.
 *     himlSrc [I] source image list handle
 *     iSrc    [I] source image index
 *     uFlags  [I] flags for the copy operation
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 *
 * NOTES
 *     Copying from one image list to another is possible. The original
 *     implementation just copies or swapps within one image list.
 *     Could this feature become a bug??? ;-)
 */

BOOL32 WINAPI
ImageList_Copy (HIMAGELIST himlDst, INT32 iDst,	HIMAGELIST himlSrc,
		INT32 iSrc, INT32 uFlags)
{
    HDC32 hdcSrc, hdcDst;    

    TRACE(imagelist, "iDst=%d  iSrc=%d\n", iDst, iSrc);

    if ((himlSrc == NULL) || (himlDst == NULL)) return (FALSE);
    if ((iDst < 0) || (iDst >= himlDst->cCurImage)) return (FALSE);
    if ((iSrc < 0) || (iSrc >= himlSrc->cCurImage)) return (FALSE);

    hdcSrc = CreateCompatibleDC32 (0);
    if (himlDst == himlSrc)
        hdcDst = hdcSrc;
    else
        hdcDst = CreateCompatibleDC32 (0);

    if (uFlags & ILCF_SWAP) {
        /* swap */
        HBITMAP32 hbmTempImage, hbmTempMask;

        /* create temporary bitmaps */
        hbmTempImage = CreateBitmap32 (himlSrc->cx, himlSrc->cy, 1,
                                       himlSrc->uBitsPixel, NULL);
        hbmTempMask = CreateBitmap32 (himlSrc->cx, himlSrc->cy, 1,
				      1, NULL);

        /* copy (and stretch) destination to temporary bitmaps.(save) */
        /* image */
        SelectObject32 (hdcSrc, himlDst->hbmImage);
        SelectObject32 (hdcDst, hbmTempImage);
        StretchBlt32 (hdcDst, 0, 0, himlSrc->cx, himlSrc->cy,
                      hdcSrc, iDst * himlDst->cx, 0, himlDst->cx, himlDst->cy,
                      SRCCOPY);
        /* mask */
        SelectObject32 (hdcSrc, himlDst->hbmMask);
        SelectObject32 (hdcDst, hbmTempMask);
        StretchBlt32 (hdcDst, 0, 0, himlSrc->cx, himlSrc->cy,
                      hdcSrc, iDst * himlDst->cx, 0, himlDst->cx, himlDst->cy,
                      SRCCOPY);

        /* copy (and stretch) source to destination */
        /* image */
        SelectObject32 (hdcSrc, himlSrc->hbmImage);
        SelectObject32 (hdcDst, himlDst->hbmImage);
        StretchBlt32 (hdcDst, iDst * himlDst->cx, 0, himlDst->cx, himlDst->cy,
                      hdcSrc, iSrc * himlSrc->cx, 0, himlSrc->cx, himlSrc->cy,
                      SRCCOPY);
        /* mask */
        SelectObject32 (hdcSrc, himlSrc->hbmMask);
        SelectObject32 (hdcDst, himlDst->hbmMask);
        StretchBlt32 (hdcDst, iDst * himlDst->cx, 0, himlDst->cx, himlDst->cy,
                      hdcSrc, iSrc * himlSrc->cx, 0, himlSrc->cx, himlSrc->cy,
                      SRCCOPY);

        /* copy (without stretching) temporary bitmaps to source (restore) */
        /* image */
        SelectObject32 (hdcSrc, hbmTempImage);
        SelectObject32 (hdcDst, himlSrc->hbmImage);
        BitBlt32 (hdcDst, iSrc * himlSrc->cx, 0, himlSrc->cx, himlSrc->cy,
                  hdcSrc, 0, 0, SRCCOPY);
        /* mask */
        SelectObject32 (hdcSrc, hbmTempMask);
        SelectObject32 (hdcDst, himlSrc->hbmMask);
        BitBlt32 (hdcDst, iSrc * himlSrc->cx, 0, himlSrc->cx, himlSrc->cy,
                  hdcSrc, 0, 0, SRCCOPY);

        /* delete temporary bitmaps */
        DeleteObject32 (hbmTempMask);
        DeleteObject32 (hbmTempImage);
    }
    else {
        /* copy image */
        SelectObject32 (hdcSrc, himlSrc->hbmImage);
        if (himlSrc == himlDst)
            hdcDst = hdcSrc;
        else
            SelectObject32 (hdcDst, himlDst->hbmImage);
        StretchBlt32 (hdcDst, iDst * himlDst->cx, 0, himlDst->cx, himlDst->cy,
                      hdcSrc, iSrc * himlSrc->cx, 0, himlSrc->cx, himlSrc->cy,
                      SRCCOPY);

        /* copy mask */
        SelectObject32 (hdcSrc, himlSrc->hbmMask);
        if (himlSrc == himlDst)
            hdcDst = hdcSrc;
        else
            SelectObject32 (hdcDst, himlDst->hbmMask);
        StretchBlt32 (hdcDst, iDst * himlDst->cx, 0, himlDst->cx, himlDst->cy,
                      hdcSrc, iSrc * himlSrc->cx, 0, himlSrc->cx, himlSrc->cy,
                      SRCCOPY);
    }

    DeleteDC32 (hdcSrc);
    if (himlSrc != himlDst)
        DeleteDC32 (hdcDst);

    return (TRUE);
}


/*************************************************************************
 * ImageList_Create [COMCTL32.44] 
 *
 * Creates a new image list.
 *
 * PARAMS
 *     cx       [I] image height
 *     cy       [I] image width
 *     flags    [I] creation flags
 *     cInitial [I] initial number of images in the image list
 *     cGrow    [I] number of images by which image list grows
 *
 * RETURNS
 *     Success: Handle of the created image list
 *     Failure: 0
 */

HIMAGELIST WINAPI
ImageList_Create (INT32 cx, INT32 cy, UINT32 flags,
		  INT32 cInitial, INT32 cGrow)
{
    HIMAGELIST himl;
    HDC32      hdc;
    INT32      nCount;
    HBITMAP32  hbmTemp;
    WORD       aBitBlend25[16] = 
        {0x7777, 0xDDDD, 0x7777, 0xDDDD, 0x7777, 0xDDDD, 0x7777, 0xDDDD,
         0x7777, 0xDDDD, 0x7777, 0xDDDD, 0x7777, 0xDDDD, 0x7777, 0xDDDD};
    WORD       aBitBlend50[16] =
        {0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA,
         0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA};

    TRACE (imagelist, "(%d %d 0x%x %d %d)\n", cx, cy, flags, cInitial, cGrow);

    himl = (HIMAGELIST)LocalAlloc32 (LMEM_FIXED | LMEM_ZEROINIT,
                                     sizeof(struct _IMAGELIST));
    if (!himl)
        return (NULL);
    himl->cx = cx;
    himl->cy = cy;
    himl->flags = flags;
    himl->cMaxImage = cInitial + cGrow;
    himl->cInitial = cInitial;
    himl->cGrow = cGrow;
    himl->cCurImage = 0;
    himl->clrBk = CLR_NONE;

    /* initialize overlay mask indices */
    for (nCount = 0; nCount <= MAX_OVERLAYIMAGE; nCount++)
        himl->nOvlIdx[nCount] = -1;

    hdc = CreateCompatibleDC32 (0);
    himl->uBitsPixel = (UINT32)GetDeviceCaps32 (hdc, BITSPIXEL);
    DeleteDC32 (hdc);

    TRACE(imagelist, "Image: %d Bits per Pixel\n", himl->uBitsPixel);

    himl->hbmImage =
        CreateBitmap32 (himl->cx * himl->cMaxImage, himl->cy,
                        1, himl->uBitsPixel, NULL);
    if (himl->hbmImage == 0) {
        ERR(imagelist, "Error creating image bitmap!\n");
        return (0);
    }

    if (himl->flags & ILC_MASK) {
        himl->hbmMask = CreateBitmap32 (himl->cx * himl->cMaxImage, himl->cy,
					1, 1, NULL);
        if (himl->hbmMask == 0) {
            ERR(imagelist, "Error creating mask bitmap!\n");
            if (himl->hbmImage)
                DeleteObject32 (himl->hbmImage);
            return (0);
        }
    }
    else
        himl->hbmMask = 0;

    /* create blending brushes */
    hbmTemp = CreateBitmap32 (16, 16, 1, 1, &aBitBlend25);
    himl->hbrBlend25 = CreatePatternBrush32 (hbmTemp);
    DeleteObject32 (hbmTemp);

    hbmTemp = CreateBitmap32 (16, 16, 1, 1, &aBitBlend50);
    himl->hbrBlend50 = CreatePatternBrush32 (hbmTemp);
    DeleteObject32 (hbmTemp);

    return (himl);    
}


/*************************************************************************
 * ImageList_Destroy [COMCTL32.45] 
 *
 * Destroys an image list.
 *
 * PARAMS
 *     himl [I] image list handle
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL32 WINAPI
ImageList_Destroy (HIMAGELIST himl)
{ 
    if (himl == NULL) return (FALSE);

    if (himl->hbmImage)
        DeleteObject32 (himl->hbmImage);
    if (himl->hbmMask)
        DeleteObject32 (himl->hbmMask);
        
    LocalFree32 ((HLOCAL32)himl);
    return (TRUE);
}


/*************************************************************************
 * ImageList_DragEnter [COMCTL32.46] 
 *
 * Locks window update and displays the drag image at the given position.
 *
 * PARAMS
 *     hwndLock [I] handle of the window that owns the drag image.
 *     x        [I] X position of the drag image.
 *     y        [I] Y position of the drag image.
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 *
 * NOTES
 *     The position of the drag image is relative to the window, not
 *     the client area.
 */

BOOL32 WINAPI
ImageList_DragEnter (HWND32 hwndLock, INT32 x, INT32 y)
{
    if (himlInternalDrag == NULL) return (FALSE);

    if (hwndLock)
	hwndInternalDrag = hwndLock;
    else
	hwndInternalDrag = GetDesktopWindow32 ();

    xInternalPos = x;
    yInternalPos = y;

    hdcBackBuffer = CreateCompatibleDC32 (0);
    hbmBackBuffer = CreateCompatibleBitmap32 (hdcBackBuffer,
		himlInternalDrag->cx, himlInternalDrag->cy);

    ImageList_DragShowNolock (TRUE);

    return (FALSE);
}


/*************************************************************************
 * ImageList_DragLeave [COMCTL32.47] 
 *
 * Unlocks window update and hides the drag image.
 *
 * PARAMS
 *     hwndLock [I] handle of the window that owns the drag image.
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL32 WINAPI
ImageList_DragLeave (HWND32 hwndLock)
{
    if (hwndLock)
	hwndInternalDrag = hwndLock;
    else
	hwndInternalDrag = GetDesktopWindow32 ();

    ImageList_DragShowNolock (FALSE);

    DeleteDC32 (hdcBackBuffer);
    DeleteObject32 (hbmBackBuffer);

    return (TRUE);
}


/*************************************************************************
 * ImageList_DragMove [COMCTL32.48] 
 *
 * Moves the drag image.
 *
 * PARAMS
 *     x [I] X position of the drag image.
 *     y [I] Y position of the drag image.
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 *
 * NOTES
 *     The position of the drag image is relative to the window, not
 *     the client area.
 */

BOOL32 WINAPI
ImageList_DragMove (INT32 x, INT32 y)
{
    ImageList_DragShowNolock (FALSE);

    xInternalPos = x;
    yInternalPos = y;

    ImageList_DragShowNolock (TRUE);

    return (FALSE);
}


/*************************************************************************
 * ImageList_DragShowNolock [COMCTL32.49] 
 *
 * Shows or hides the drag image.
 *
 * PARAMS
 *     bShow [I] TRUE shows the drag image, FALSE hides it.
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 *
 * FIXME
 *     semi-stub.
 */

BOOL32 WINAPI
ImageList_DragShowNolock (BOOL32 bShow)
{
    HDC32 hdcDrag;

    FIXME (imagelist, "semi-stub!\n");
    TRACE (imagelist, "bShow=0x%X!\n", bShow);

    hdcDrag = GetDCEx32 (hwndInternalDrag, 0,
			 DCX_WINDOW | DCX_CACHE | DCX_LOCKWINDOWUPDATE);

    if (bShow) {
	/* show drag image */

	/* save background */

	/* draw drag image */

    }
    else {
	/* hide drag image */

	/* restore background */

    }

    ReleaseDC32 (hwndInternalDrag, hdcDrag);

    return (FALSE);
}


/*************************************************************************
 * ImageList_Draw [COMCTL32.50] Draws an image.
 *
 * PARAMS
 *     himl   [I] image list handle
 *     i      [I] image index
 *     hdc    [I] display context handle
 *     x      [I] x position
 *     y      [I] y position
 *     fStyle [I] drawing flags
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 *
 * NOTES
 *     Calls ImageList_DrawIndirect.
 *
 * SEE
 *     ImageList_DrawIndirect.
 */

BOOL32 WINAPI
ImageList_Draw (HIMAGELIST himl, INT32 i, HDC32 hdc,
		INT32 x, INT32 y, UINT32 fStyle)
{
    IMAGELISTDRAWPARAMS imldp;

    imldp.cbSize  = sizeof(IMAGELISTDRAWPARAMS);
    imldp.himl    = himl;
    imldp.i       = i;
    imldp.hdcDst  = hdc,
    imldp.x       = x;
    imldp.y       = y;
    imldp.cx      = 0;
    imldp.cy      = 0;
    imldp.xBitmap = 0;
    imldp.yBitmap = 0;
    imldp.rgbBk   = CLR_DEFAULT;
    imldp.rgbFg   = CLR_DEFAULT;
    imldp.fStyle  = fStyle;
    imldp.dwRop   = 0;

    return (ImageList_DrawIndirect (&imldp));
}


/*************************************************************************
 * ImageList_DrawEx [COMCTL32.51]
 *
 * Draws an image and allows to use extended drawing features.
 *
 * PARAMS
 *     himl   [I] image list handle
 *     i      [I] image index
 *     hdc    [I] device context handle
 *     x      [I] X position
 *     y      [I] Y position
 *     xOffs  [I] X offset
 *     yOffs  [I] Y offset
 *     rgbBk  [I] background color
 *     rgbFg  [I] foreground color
 *     fStyle [I] drawing flags
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 *
 * NOTES
 *     Calls ImageList_DrawIndirect.
 *
 * SEE
 *     ImageList_DrawIndirect.
 */

BOOL32 WINAPI
ImageList_DrawEx (HIMAGELIST himl, INT32 i, HDC32 hdc, INT32 x, INT32 y,
		  INT32 dx, INT32 dy, COLORREF rgbBk, COLORREF rgbFg,
		  UINT32 fStyle)
{
    IMAGELISTDRAWPARAMS imldp;

    imldp.cbSize  = sizeof(IMAGELISTDRAWPARAMS);
    imldp.himl    = himl;
    imldp.i       = i;
    imldp.hdcDst  = hdc,
    imldp.x       = x;
    imldp.y       = y;
    imldp.cx      = dx;
    imldp.cy      = dy;
    imldp.xBitmap = 0;
    imldp.yBitmap = 0;
    imldp.rgbBk   = rgbBk;
    imldp.rgbFg   = rgbFg;
    imldp.fStyle  = fStyle;
    imldp.dwRop   = 0;

    return (ImageList_DrawIndirect (&imldp));
}


/*************************************************************************
 * ImageList_DrawIndirect [COMCTL32.52] 
 *
 * Draws an image using ...
 *
 * PARAMS
 *     pimldp [I] pointer to IMAGELISTDRAWPARAMS structure.
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL32 WINAPI
ImageList_DrawIndirect (IMAGELISTDRAWPARAMS *pimldp)
{
    HIMAGELIST himlLocal;
    HDC32      hdcImageList, hdcTempImage;
    HBITMAP32  hbmTempImage;
    HBRUSH32   hBrush, hOldBrush;
    INT32      cx, cy;
    INT32      nOvlIdx;
    COLORREF   clrBlend;
    BOOL32     bImage;       /* draw image ? */
    BOOL32     bImageTrans;  /* draw image transparent ? */
    BOOL32     bMask;        /* draw mask ? */
    BOOL32     bMaskTrans;   /* draw mask transparent ? */
    BOOL32     bBlend25;
    BOOL32     bBlend50;

    if (pimldp == NULL) return (FALSE);
    if (pimldp->cbSize < sizeof(IMAGELISTDRAWPARAMS)) return (FALSE);
    if (pimldp->himl == NULL) return (FALSE);
    if ((pimldp->i < 0) || (pimldp->i >= pimldp->himl->cCurImage))
	return (FALSE);

    himlLocal = pimldp->himl;

    cx = (pimldp->cx == 0) ? himlLocal->cx : pimldp->cx;
    cy = (pimldp->cy == 0) ? himlLocal->cy : pimldp->cy;

    /* ILD_NORMAL state */
    bImage      = TRUE;
    bImageTrans = FALSE;
    bMask       = FALSE;
    bMaskTrans  = FALSE;
    bBlend25    = FALSE;
    bBlend50    = FALSE;
    if ((himlLocal->clrBk == CLR_NONE) && (himlLocal->hbmMask))
    {
        bImageTrans = TRUE;
        bMask = TRUE;
        bMaskTrans = TRUE;
    }
    
    /* ILD_IMAGE state (changes) */
    if (pimldp->fStyle & ILD_IMAGE)
    {
        bMask = FALSE;
        bImage = TRUE;
        bImageTrans = FALSE;
    }
    
    /* ILD_MASK state (changes) */
    if ((pimldp->fStyle & ILD_MASK) && (himlLocal->hbmMask))
    {
        bMask  = TRUE;
        bMaskTrans = FALSE;
        bImage = FALSE;
    }
    if ((pimldp->fStyle & ILD_TRANSPARENT) && (himlLocal->hbmMask))
    {
        bMaskTrans = TRUE;
        bImageTrans = TRUE;
    }
    if ((himlLocal->clrBk == CLR_NONE) && (himlLocal->hbmMask))
        bMaskTrans = TRUE;

    if (pimldp->fStyle & ILD_BLEND50)
        bBlend50 = TRUE;
    else if (pimldp->fStyle & ILD_BLEND25)
        bBlend25 = TRUE;

    hdcImageList = CreateCompatibleDC32 (0);

    if (bMask)
    {
	/* draw the mask */
	SelectObject32 (hdcImageList, himlLocal->hbmMask);
	BitBlt32 (pimldp->hdcDst, pimldp->x, pimldp->y, cx, cy,
		  hdcImageList, himlLocal->cx * pimldp->i, 0,
		  bMaskTrans ? SRCAND : SRCCOPY);
    }

    if (bImage)
    {
	/* draw the image */
	SelectObject32 (hdcImageList, himlLocal->hbmImage);

        if (!bImageTrans)
        {
            hBrush = CreateSolidBrush32 (himlLocal->clrBk);
            hOldBrush = SelectObject32 (pimldp->hdcDst, hBrush);
            PatBlt32 (pimldp->hdcDst, pimldp->x, pimldp->y,
                      cx, cy, PATCOPY);
            DeleteObject32 (SelectObject32 (pimldp->hdcDst, hOldBrush));
        }

        BitBlt32 (pimldp->hdcDst, pimldp->x, pimldp->y, cx, cy,
                  hdcImageList, himlLocal->cx * pimldp->i, 0,
                  SRCPAINT);

        if (bBlend25 || bBlend50)
        {
            if (pimldp->rgbFg == CLR_DEFAULT)
                clrBlend = GetSysColor32 (COLOR_HIGHLIGHT);
            else
                clrBlend = pimldp->rgbFg;

            hdcTempImage = CreateCompatibleDC32 (0);
            hbmTempImage = CreateBitmap32 (himlLocal->cx, himlLocal->cy,
                                           1, himlLocal->uBitsPixel, NULL);
            SelectObject32 (hdcTempImage, hbmTempImage);


            /* mask */
            SelectObject32 (hdcTempImage,
                            bBlend50 ? himlLocal->hbrBlend50 : himlLocal->hbrBlend25);
            PatBlt32 (hdcTempImage, 0, 0, himlLocal->cx, himlLocal->cy, PATCOPY);

            SelectObject32 (hdcImageList, himlLocal->hbmMask);
            BitBlt32 (hdcTempImage, 0, 0, himlLocal->cx,
                      himlLocal->cy, hdcImageList, 
                      pimldp->i * himlLocal->cx, 0, SRCPAINT);

            BitBlt32 (pimldp->hdcDst, pimldp->x, pimldp->y, cx, cy,
		      hdcTempImage, 0, 0, SRCAND);

            /* fill */
            hBrush = CreateSolidBrush32 (clrBlend);
            SelectObject32 (hdcTempImage, hBrush);
            PatBlt32 (hdcTempImage, 0, 0, himlLocal->cx, himlLocal->cy, PATCOPY);
            DeleteObject32 (hBrush);

            SelectObject32 (hdcTempImage,
                            bBlend50 ? himlLocal->hbrBlend50 : himlLocal->hbrBlend25);
            PatBlt32 (hdcTempImage, 0, 0, himlLocal->cx, himlLocal->cy, 0x0A0329);

            SelectObject32 (hdcImageList, himlLocal->hbmMask);
            BitBlt32 (hdcTempImage, 0, 0, himlLocal->cx,
                      himlLocal->cy, hdcImageList, 
                      pimldp->i * himlLocal->cx, 0, SRCPAINT);

            BitBlt32 (pimldp->hdcDst, pimldp->x, pimldp->y, cx, cy,
                      hdcTempImage, 0, 0, SRCPAINT);

            DeleteObject32 (hbmTempImage);
            DeleteDC32 (hdcTempImage);
        }
    }   

    /* Draw overlay image */
    if (pimldp->fStyle & 0x0700) {
	nOvlIdx = (pimldp->fStyle & 0x0700) >> 8;
	if ((nOvlIdx >= 1) && (nOvlIdx <= MAX_OVERLAYIMAGE)) {
	    nOvlIdx = pimldp->himl->nOvlIdx[nOvlIdx - 1];
	    if ((nOvlIdx >= 0) && (nOvlIdx <= pimldp->himl->cCurImage)) {
		if (pimldp->himl->hbmMask) {  
                    SelectObject32 (hdcImageList, pimldp->himl->hbmMask);
                    BitBlt32 (pimldp->hdcDst, pimldp->x, pimldp->y, cx, cy,
                              hdcImageList, pimldp->himl->cx * nOvlIdx, 0,
			      SRCAND);
		}  
                SelectObject32 (hdcImageList, pimldp->himl->hbmImage);
                BitBlt32 (pimldp->hdcDst, pimldp->x, pimldp->y,
                          cx, cy, hdcImageList,
                          pimldp->himl->cx * nOvlIdx, 0, SRCPAINT);
            }
        }
    }

    DeleteDC32 (hdcImageList);
  
    return (TRUE);
}


/*************************************************************************
 * ImageList_Duplicate [COMCTL32.53] Duplicates an image list.
 *
 * PARAMS
 *     himlSrc [I] source image list handle
 *
 * RETURNS
 *     Success: Handle of duplicated image list.
 *     Failure: NULL
 */

HIMAGELIST WINAPI
ImageList_Duplicate (HIMAGELIST himlSrc)
{
    HIMAGELIST himlDst;
    HDC32 hdcSrc, hdcDst;

    if (himlSrc == NULL) {
        ERR (imagelist, "Invalid image list handle!\n");
        return (NULL);
    }

    himlDst = ImageList_Create (himlSrc->cx, himlSrc->cy, himlSrc->flags,
                                himlSrc->cInitial, himlSrc->cGrow);

    if (himlDst)
    {
        hdcSrc = CreateCompatibleDC32 (0);
        hdcDst = CreateCompatibleDC32 (0);
        SelectObject32 (hdcSrc, himlSrc->hbmImage);
        SelectObject32 (hdcDst, himlDst->hbmImage);
        BitBlt32 (hdcDst, 0, 0, himlSrc->cCurImage * himlSrc->cx, himlSrc->cy,
                  hdcSrc, 0, 0, SRCCOPY);

        if (himlDst->hbmMask)
        {
            SelectObject32 (hdcSrc, himlSrc->hbmMask);
            SelectObject32 (hdcDst, himlDst->hbmMask);
            BitBlt32 (hdcDst, 0, 0, himlSrc->cCurImage * himlSrc->cx,
                      himlSrc->cy, hdcSrc, 0, 0, SRCCOPY);
        }

        DeleteDC32 (hdcDst);
        DeleteDC32 (hdcSrc);
    }

    return (himlDst);
}


/*************************************************************************
 * ImageList_EndDrag [COMCTL32.54] Finishes a drag operation.
 *
 * Finishes a drag operation.
 *
 * PARAMS
 *     no Parameters
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 *
 * BUGS
 *     semi-stub.
 */

BOOL32 WINAPI
ImageList_EndDrag (VOID)
{
    FIXME (imagelist, "semi-stub!\n");

    if (himlInternalDrag)
    {

        ImageList_Destroy (himlInternalDrag);
        himlInternalDrag = NULL;

        nInternalDragHotspotX = 0;
        nInternalDragHotspotY = 0;

    }

    return (TRUE);
}


/*************************************************************************
 * ImageList_GetBkColor [COMCTL32.55]
 *
 * Returns the background color of an image list.
 *
 * PARAMS
 *     himl [I] Image list handle.
 *
 * RETURNS
 *     Success: background color
 *     Failure: CLR_NONE
 */

COLORREF WINAPI
ImageList_GetBkColor (HIMAGELIST himl)
{
    if (himl == NULL)
	return CLR_NONE;

    return (himl->clrBk);
}


/*************************************************************************
 * ImageList_GetDragImage [COMCTL32.56]
 *
 * Returns the handle to the internal drag image list.
 *
 * PARAMS
 *     ppt        [O] Pointer to the drag position. Can be NULL.
 *     pptHotspot [O] Pointer to the position of the hot spot. Can be NULL.
 *
 * RETURNS
 *     Success: Handle of the drag image list.
 *     Failure: NULL.
 *
 * BUGS
 *     semi-stub.
 */

HIMAGELIST WINAPI
ImageList_GetDragImage (POINT32 *ppt, POINT32 *pptHotspot)
{
    FIXME (imagelist, "semi-stub!\n");

    if (himlInternalDrag)
        return (himlInternalDrag);

    return (NULL);
}


/*************************************************************************
 * ImageList_GetIcon [COMCTL32.57] 
 *
 * Creates an icon from a masked image of an image list.
 *
 * PARAMS
 *     himl  [I] image list handle
 *     i     [I] image index
 *     flags [I] drawing style flags
 *
 * RETURNS
 *     Success: icon handle
 *     Failure: NULL
 */

HICON32 WINAPI
ImageList_GetIcon (HIMAGELIST himl, INT32 i, UINT32 fStyle)
{
    ICONINFO ii;
    HICON32  hIcon;
    HDC32    hdc;
    INT32    nWidth, nHeight;

    if (himl == NULL) return 0;
    if ((i < 0) || (i >= himl->cCurImage)) return 0;

    nWidth = GetSystemMetrics32 (SM_CXICON);
    nHeight = GetSystemMetrics32 (SM_CYICON);

    ii.fIcon = TRUE;
    ii.xHotspot = 0;
    ii.yHotspot = 0;
    ii.hbmMask  = CreateBitmap32 (nWidth, nHeight, 1, 1, NULL);
    ii.hbmColor = CreateBitmap32 (nWidth, nHeight, 1, himl->uBitsPixel, NULL);

    hdc = CreateCompatibleDC32(0);

    /* draw image*/
    SelectObject32 (hdc, ii.hbmColor);
    PatBlt32 (hdc, 0, 0, nWidth, nHeight, BLACKNESS);
    ImageList_Draw (himl, i, hdc, 0, 0, fStyle | ILD_TRANSPARENT);

    /* draw mask*/
    SelectObject32 (hdc, ii.hbmMask);
    PatBlt32 (hdc, 0, 0, nWidth, nHeight, WHITENESS);
    ImageList_Draw (himl, i, hdc, 0, 0, fStyle | ILD_MASK);

    hIcon = CreateIconIndirect (&ii);    

    DeleteDC32 (hdc);
    DeleteObject32 (ii.hbmMask);
    DeleteObject32 (ii.hbmColor);

    return (hIcon);
}


/*************************************************************************
 * ImageList_GetIconSize [COMCTL32.58]
 *
 * Retrieves the size of an image in an image list.
 *
 * PARAMS
 *     himl [I] image list handle
 *     cx   [O] pointer to the image width.
 *     cy   [O] pointer to the image height.
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 *
 * NOTES
 *     All images in an image list have the same size.
 */

BOOL32 WINAPI
ImageList_GetIconSize (HIMAGELIST himl, INT32 *cx, INT32 *cy)
{

    if (himl == NULL) return (FALSE);

    if (cx)
        *cx = himl->cx;
    
    if (cy)
        *cy = himl->cy;
    return (TRUE);
}


/*************************************************************************
 * ImageList_GetImageCount [COMCTL32.59]
 *
 * Returns the number of images in an image list.
 *
 * PARAMS
 *     himl [I] image list handle.
 *
 * RETURNS
 *     Success: Number of images.
 *     Failure: 0
 */

INT32 WINAPI
ImageList_GetImageCount (HIMAGELIST himl)
{
    if (himl == NULL)
	return 0;

    return (himl->cCurImage);
}


/*************************************************************************
 * ImageList_GetImageInfo [COMCTL32.60]
 *
 * Returns information about an image in an image list.
 *
 * PARAMS
 *     himl       [I] image list handle.
 *     i          [I] image index
 *     pImageInfo [O] pointer to the image information.
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL32 WINAPI
ImageList_GetImageInfo (HIMAGELIST himl, INT32 i, IMAGEINFO *pImageInfo)
{
    if ((himl == NULL) || (pImageInfo == NULL)) return (FALSE);
    if ((i < 0) || (i >= himl->cCurImage)) return (FALSE);

    pImageInfo->hbmImage = himl->hbmImage;
    pImageInfo->hbmMask  = himl->hbmMask;
    
    pImageInfo->rcImage.top    = 0;
    pImageInfo->rcImage.bottom = himl->cy;
    pImageInfo->rcImage.left   = i * himl->cx;
    pImageInfo->rcImage.right  = (i+1) * himl->cx;
    
    return (TRUE);
}


/*************************************************************************
 * ImageList_GetImageRect [COMCTL32.61] 
 *
 * Retrieves the rectangle of the specified image in an image list.
 *
 * PARAMS
 *     himl   [I] image list handle
 *     i      [I] image index
 *     lpRect [O] pointer to the image rectangle
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 *
 * NOTES
 *    This is an UNDOCUMENTED function!!!
 */

BOOL32 WINAPI
ImageList_GetImageRect (HIMAGELIST himl, INT32 i, LPRECT32 lpRect)
{
    if ((himl == NULL) || (lpRect == NULL)) return (FALSE);
    if ((i < 0) || (i >= himl->cCurImage)) return (FALSE);

    lpRect->left = i * himl->cx;
    lpRect->top = 0;
    lpRect->right = lpRect->left + himl->cx;
    lpRect->bottom = himl->cy;

    return (TRUE);
}


/*************************************************************************
 * ImageList_LoadImage32A [COMCTL32.62][COMCTL32.63]
 *
 * Creates an image list from a bitmap, icon or cursor.
 *
 * PARAMS
 *     hi      [I] instance handle
 *     lpbmp   [I] name or id of the image
 *     cx      [I] width of each image
 *     cGrow   [I] number of images to expand
 *     clrMask [I] mask color
 *     uType   [I] type of image to load
 *     uFlags  [I] loading flags
 *
 * RETURNS
 *     Success: handle of the loaded image
 *     Failure: NULL
 *
 * SEE
 *     LoadImage ()
 */

HIMAGELIST WINAPI
ImageList_LoadImage32A (HINSTANCE32 hi, LPCSTR lpbmp, INT32 cx,	INT32 cGrow,
			COLORREF clrMask, UINT32 uType,	UINT32 uFlags)
{
    HIMAGELIST himl = NULL;
    HANDLE32   handle;
    INT32      nImageCount;

    handle = LoadImage32A (hi, lpbmp, uType, 0, 0, uFlags);
    if (!handle) return (NULL);

    if (uType == IMAGE_BITMAP) {
        BITMAP32 bmp;
        GetObject32A (handle, sizeof(BITMAP32), &bmp);
        nImageCount = bmp.bmWidth / cx;

        himl = ImageList_Create (cx, bmp.bmHeight, ILC_MASK | ILC_COLOR,
                                 nImageCount, cGrow);
        ImageList_AddMasked (himl, (HBITMAP32)handle, clrMask);
    }
    else if ((uType == IMAGE_ICON) || (uType == IMAGE_CURSOR)) {
        ICONINFO ii;
        BITMAP32 bmp;

        GetIconInfo (handle, &ii);
        GetObject32A (ii.hbmColor, sizeof(BITMAP32), (LPVOID)&bmp);
        himl = ImageList_Create (bmp.bmWidth, bmp.bmHeight, 
                                 ILC_MASK | ILC_COLOR, 1, cGrow);
        ImageList_Add (himl, ii.hbmColor, ii.hbmMask);
        DeleteObject32 (ii.hbmColor);
        DeleteObject32 (ii.hbmMask);
    }

    DeleteObject32 (handle);
    
    return (himl);
}


/*************************************************************************
 * ImageList_LoadImage32W [COMCTL32.64]
 *
 * Creates an image list from a bitmap, icon or cursor.
 *
 * PARAMS
 *     hi      [I] instance handle
 *     lpbmp   [I] name or id of the image
 *     cx      [I] width of each image
 *     cGrow   [I] number of images to expand
 *     clrMask [I] mask color
 *     uType   [I] type of image to load
 *     uFlags  [I] loading flags
 *
 * RETURNS
 *     Success: handle of the loaded image
 *     Failure: NULL
 *
 * SEE
 *     LoadImage ()
 */

HIMAGELIST WINAPI
ImageList_LoadImage32W (HINSTANCE32 hi, LPCWSTR lpbmp, INT32 cx, INT32 cGrow,
			COLORREF clrMask, UINT32 uType,	UINT32 uFlags)
{
    HIMAGELIST himl = NULL;
    HANDLE32   handle;
    INT32      nImageCount;

    handle = LoadImage32W (hi, lpbmp, uType, 0, 0, uFlags);
    if (!handle) {
        ERR (imagelist, "Error loading image!\n");
        return (NULL);
    }

    if (uType == IMAGE_BITMAP) {
        BITMAP32 bmp;
        GetObject32A (handle, sizeof(BITMAP32), &bmp);
        nImageCount = bmp.bmWidth / cx;

        himl = ImageList_Create (cx, bmp.bmHeight, ILC_MASK | ILC_COLOR,
                                 nImageCount, cGrow);
        ImageList_AddMasked (himl, (HBITMAP32)handle, clrMask);
    }
    else if ((uType == IMAGE_ICON) || (uType == IMAGE_CURSOR)) {
        ICONINFO ii;
        BITMAP32 bmp;

        GetIconInfo (handle, &ii);
        GetObject32A (ii.hbmMask, sizeof(BITMAP32), (LPVOID)&bmp);
        himl = ImageList_Create (bmp.bmWidth, bmp.bmHeight, 
                                 ILC_MASK | ILC_COLOR, 1, cGrow);
        ImageList_Add (himl, ii.hbmColor, ii.hbmMask);
        DeleteObject32 (ii.hbmColor);
        DeleteObject32 (ii.hbmMask);
    }

    DeleteObject32 (handle);
    
    return (himl);
}


/*************************************************************************
 * ImageList_Merge [COMCTL32.65] 
 *
 * Creates a new image list that contains a merged image from the specified
 * images of both source image lists.
 *
 * PARAMS
 *     himl1 [I] first image list handle
 *     i1    [I] first image index
 *     himl2 [I] second image list handle
 *     i2    [I] second image index
 *     dx    [I] X offset of the second image relative to the first.
 *     dy    [I] Y offset of the second image relative to the first.
 *
 * RETURNS
 *     Success: handle of the merged image list.
 *     Failure: NULL
 */

HIMAGELIST WINAPI
ImageList_Merge (HIMAGELIST himl1, INT32 i1, HIMAGELIST himl2, INT32 i2,
		 INT32 dx, INT32 dy)
{
    HIMAGELIST himlDst = NULL;
    HDC32      hdcSrcImage, hdcDstImage;
    INT32      cxDst, cyDst;
    INT32      xOff1, yOff1, xOff2, yOff2;
    INT32      nX1, nX2;

    if ((himl1 == NULL) || (himl2 == NULL)) return (NULL);

    /* check indices */
    if ((i1 < 0) || (i1 >= himl1->cCurImage)) {
        ERR (imagelist, "Index 1 out of range! %d\n", i1);
        return (NULL);
    }

    if ((i2 < 0) || (i2 >= himl2->cCurImage)) {
        ERR (imagelist, "Index 2 out of range! %d\n", i2);
        return (NULL);
    }

    if (dx > 0) {
        cxDst = _MAX (himl1->cx, dx + himl2->cx);
        xOff1 = 0;
        xOff2 = dx;
    }
    else if (dx < 0) {
        cxDst = _MAX (himl2->cx, himl1->cx - dx);
        xOff1 = -dx;
        xOff2 = 0;
    }
    else {
        cxDst = _MAX (himl1->cx, himl2->cx);
        xOff1 = 0;
        xOff2 = 0;
    }

    if (dy > 0) {
        cyDst = _MAX (himl1->cy, dy + himl2->cy);
        yOff1 = 0;
        yOff2 = dy;
    }
    else if (dy < 0) {
        cyDst = _MAX (himl2->cy, himl1->cy - dy);
        yOff1 = -dy;
        yOff2 = 0;
    }
    else {
        cyDst = _MAX (himl1->cy, himl2->cy);
        yOff1 = 0;
        yOff2 = 0;
    }

    himlDst = ImageList_Create (cxDst, cyDst, ILC_MASK | ILC_COLOR, 1, 1);

    if (himlDst) {
        hdcSrcImage = CreateCompatibleDC32 (0);
        hdcDstImage = CreateCompatibleDC32 (0);
        nX1 = i1 * himl1->cx;
        nX2 = i2 * himl2->cx;
        
        /* copy image */
        SelectObject32 (hdcSrcImage, himl1->hbmImage);
        SelectObject32 (hdcDstImage, himlDst->hbmImage);
        BitBlt32 (hdcDstImage, 0, 0, cxDst, cyDst, 
                  hdcSrcImage, 0, 0, BLACKNESS);
        BitBlt32 (hdcDstImage, xOff1, yOff1, himl1->cx, himl1->cy, 
                  hdcSrcImage, nX1, 0, SRCCOPY);

        SelectObject32 (hdcSrcImage, himl2->hbmMask);
        BitBlt32 (hdcDstImage, xOff2, yOff2, himl2->cx, himl2->cy, 
                  hdcSrcImage, nX2, 0, SRCAND);

        SelectObject32 (hdcSrcImage, himl2->hbmImage);
        BitBlt32 (hdcDstImage, xOff2, yOff2, himl2->cx, himl2->cy, 
                  hdcSrcImage, nX2, 0, SRCPAINT);

        /* copy mask */
        SelectObject32 (hdcSrcImage, himl1->hbmMask);
        SelectObject32 (hdcDstImage, himlDst->hbmMask);
        BitBlt32 (hdcDstImage, 0, 0, cxDst, cyDst, 
                  hdcSrcImage, 0, 0, WHITENESS);
        BitBlt32 (hdcDstImage, xOff1, yOff1, himl1->cx, himl1->cy, 
                  hdcSrcImage, nX1, 0, SRCCOPY);

        SelectObject32 (hdcSrcImage, himl2->hbmMask);
        BitBlt32 (hdcDstImage, xOff2, yOff2, himl2->cx, himl2->cy, 
                  hdcSrcImage, nX2, 0, SRCAND);

        DeleteDC32 (hdcSrcImage);
        DeleteDC32 (hdcDstImage);
    }
   
    return (himlDst);
}


/*************************************************************************
 * ImageList_Read [COMCTL32.66]
 *
 * Reads an image list from a stream.
 *
 * PARAMS
 *     pstm [I] pointer to a stream
 *
 * RETURNS
 *     Success: image list handle
 *     Failure: NULL
 *
 * NOTES
 *     This function can not be implemented yet, because
 *     IStream32::Read is not implemented.
 *
 * BUGS
 *     empty stub.
 */

HIMAGELIST WINAPI ImageList_Read (LPSTREAM32 pstm)
{
    FIXME (imagelist, "empty stub!\n");


    return (NULL);
}


/*************************************************************************
 * ImageList_Remove [COMCTL32.67] Removes an image from an image list
 *
 * PARAMS
 *     himl [I] image list handle
 *     i    [I] image index
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL32 WINAPI
ImageList_Remove (HIMAGELIST himl, INT32 i)
{
    HBITMAP32 hbmNewImage, hbmNewMask;
    HDC32     hdcSrc, hdcDst;
    INT32     cxNew, nCount;

    if ((i < -1) || (i >= himl->cCurImage)) {
        ERR (imagelist, "index out of range! %d\n", i);
        return (FALSE);
    }

    if (himl->cCurImage == 0) {
        ERR (imagelist, "image list is already empty!\n");
        return (FALSE);
    }

    if (i == -1) {
        /* remove all */
        TRACE (imagelist, "remove all!\n");

        himl->cMaxImage = himl->cInitial + himl->cGrow;
        himl->cCurImage = 0;
        for (nCount = 0; nCount <= MAX_OVERLAYIMAGE; nCount++)
             himl->nOvlIdx[nCount] = -1;

        DeleteObject32 (himl->hbmImage);
        himl->hbmImage =
            CreateBitmap32 (himl->cMaxImage * himl->cx, himl->cy,
                            1, himl->uBitsPixel, NULL);

        if (himl->hbmMask) {
            DeleteObject32 (himl->hbmMask);
            himl->hbmMask =
                CreateBitmap32 (himl->cMaxImage * himl->cx, himl->cy,
                                1, 1, NULL);
        }
    }
    else {
        /* delete one image */
        TRACE (imagelist, "Remove single image! %d\n", i);

        /* create new bitmap(s) */
        cxNew = (himl->cCurImage + himl->cGrow - 1) * himl->cx;

        TRACE(imagelist, " - Number of images: %d / %d (Old/New)\n",
                 himl->cCurImage, himl->cCurImage - 1);
        TRACE(imagelist, " - Max. number of images: %d / %d (Old/New)\n",
                 himl->cMaxImage, himl->cCurImage + himl->cGrow - 1);
        
        hbmNewImage =
            CreateBitmap32 (cxNew, himl->cy, 1, himl->uBitsPixel, NULL);

        if (himl->hbmMask)
            hbmNewMask = CreateBitmap32 (cxNew, himl->cy, 1, 1, NULL);
        else
            hbmNewMask = 0;  /* Just to keep compiler happy! */

        hdcSrc = CreateCompatibleDC32 (0);
        hdcDst = CreateCompatibleDC32 (0);

        /* copy all images and masks prior to the "removed" image */
        if (i > 0) {
            TRACE (imagelist, "Pre image copy: Copy %d images\n", i);
       
            SelectObject32 (hdcSrc, himl->hbmImage);
            SelectObject32 (hdcDst, hbmNewImage);
            BitBlt32 (hdcDst, 0, 0, i * himl->cx, himl->cy,
                      hdcSrc, 0, 0, SRCCOPY);

            if (himl->hbmMask) {
                SelectObject32 (hdcSrc, himl->hbmMask);
                SelectObject32 (hdcDst, hbmNewMask);
                BitBlt32 (hdcDst, 0, 0, i * himl->cx, himl->cy,
                          hdcSrc, 0, 0, SRCCOPY);
            }
        }

        /* copy all images and masks behind the removed image */
        if (i < himl->cCurImage - 1) {
            TRACE (imagelist, "Post image copy!\n");
            SelectObject32 (hdcSrc, himl->hbmImage);
            SelectObject32 (hdcDst, hbmNewImage);
            BitBlt32 (hdcDst, i * himl->cx, 0, (himl->cCurImage - i - 1) * himl->cx,
                      himl->cy, hdcSrc, (i + 1) * himl->cx, 0, SRCCOPY);

            if (himl->hbmMask) {
                SelectObject32 (hdcSrc, himl->hbmMask);
                SelectObject32 (hdcDst, hbmNewMask);
                BitBlt32 (hdcDst, i * himl->cx, 0,
                          (himl->cCurImage - i - 1) * himl->cx,
                          himl->cy, hdcSrc, (i + 1) * himl->cx, 0, SRCCOPY);
            }
        }

        DeleteDC32 (hdcSrc);
        DeleteDC32 (hdcDst);

        /* delete old images and insert new ones */
        DeleteObject32 (himl->hbmImage);
        himl->hbmImage = hbmNewImage;
        if (himl->hbmMask) {
            DeleteObject32 (himl->hbmMask);
            himl->hbmMask = hbmNewMask;
        }

        himl->cCurImage--;
        himl->cMaxImage = himl->cCurImage + himl->cGrow;
    }

    return (TRUE);
}


/*************************************************************************
 * ImageList_Replace [COMCTL32.68] 
 *
 * Replaces an image in an image list with a new image.
 *
 * PARAMS
 *     himl     [I] image list handle
 *     i        [I] image index
 *     hbmImage [I] image bitmap handle
 *     hbmMask  [I] mask bitmap handle. Can be NULL.
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL32 WINAPI
ImageList_Replace (HIMAGELIST himl, INT32 i, HBITMAP32 hbmImage,
		   HBITMAP32 hbmMask)
{
    HDC32 hdcImageList, hdcImage;
    BITMAP32 bmp;

    if (himl == NULL) {
        ERR (imagelist, "Invalid image list handle!\n");
        return (FALSE);
    }
    
    if ((i >= himl->cCurImage) || (i < 0)) {
        ERR (imagelist, "Invalid image index!\n");
        return (FALSE);
    }

    hdcImageList = CreateCompatibleDC32 (0);
    hdcImage = CreateCompatibleDC32 (0);
    GetObject32A (hbmImage, sizeof(BITMAP32), (LPVOID)&bmp);

    /* Replace Image */
    SelectObject32 (hdcImageList, himl->hbmImage);
    SelectObject32 (hdcImage, hbmImage);

    StretchBlt32 (hdcImageList, i * himl->cx, 0, himl->cx, himl->cy,
                  hdcImage, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);

    if (himl->hbmMask)
    {
        /* Replace Mask */
        SelectObject32 (hdcImageList, himl->hbmMask);
        SelectObject32 (hdcImage, hbmMask);

        StretchBlt32 (hdcImageList, i * himl->cx, 0, himl->cx, himl->cy,
                      hdcImage, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);
    }

    DeleteDC32 (hdcImage);
    DeleteDC32 (hdcImageList);

    return (TRUE);
}


/*************************************************************************
 * ImageList_ReplaceIcon [COMCTL32.69]
 *
 * Replaces an image in an image list using an icon.
 *
 * PARAMS
 *     himl  [I] image list handle
 *     i     [I] image index
 *     hIcon [I] icon handle
 *
 * RETURNS
 *     Success: index of the replaced image
 *     Failure: -1
 */

INT32 WINAPI
ImageList_ReplaceIcon (HIMAGELIST himl, INT32 i, HICON32 hIcon)
{
    HDC32     hdcImageList, hdcImage;
    INT32     nIndex;
    HBITMAP32 hbmOldSrc, hbmOldDst;
    ICONINFO ii;
    BITMAP32 bmp;

    TRACE (imagelist, "(0x%lx 0x%x 0x%x)\n", (DWORD)himl, i, hIcon);

    if (himl == NULL) return (-1);
    if ((i >= himl->cCurImage) || (i < -1)) return (-1);

    GetIconInfo (hIcon, &ii);
    if (ii.hbmMask == 0)
	ERR (imagelist, "no mask!\n");
    if (ii.hbmColor == 0)
	ERR (imagelist, "no color!\n");
    GetObject32A (ii.hbmMask, sizeof(BITMAP32), (LPVOID)&bmp);

    if (i == -1) {
        if (himl->cCurImage + 1 >= himl->cMaxImage)
            IMAGELIST_InternalExpandBitmaps (himl, 1);

        nIndex = himl->cCurImage;
        himl->cCurImage++;
    }
    else
        nIndex = i;

    hdcImageList = CreateCompatibleDC32 (0);
    TRACE (imagelist, "hdcImageList=0x%x!\n", hdcImageList);
    if (hdcImageList == 0)
	ERR (imagelist, "invalid hdcImageList!\n");

    hdcImage = CreateCompatibleDC32 (0);
    TRACE (imagelist, "hdcImage=0x%x!\n", hdcImage);
    if (hdcImage == 0)
	ERR (imagelist, "invalid hdcImage!\n");

    hbmOldDst = SelectObject32 (hdcImageList, himl->hbmImage);
    SetTextColor32( hdcImageList, RGB(0,0,0));
    SetBkColor32( hdcImageList, RGB(255,255,255));
    hbmOldSrc = SelectObject32 (hdcImage, ii.hbmColor);
    StretchBlt32 (hdcImageList, nIndex * himl->cx, 0, himl->cx, himl->cy,
                  hdcImage, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);

    if (himl->hbmMask) {
        SelectObject32 (hdcImageList, himl->hbmMask);
        SelectObject32 (hdcImage, ii.hbmMask);
        StretchBlt32 (hdcImageList, nIndex * himl->cx, 0, himl->cx, himl->cy,
                      hdcImage, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);
    }

    SelectObject32 (hdcImage, hbmOldSrc);
    SelectObject32 (hdcImageList, hbmOldDst);

    if (hdcImageList)
	DeleteDC32 (hdcImageList);
    if (hdcImage)
	DeleteDC32 (hdcImage);

//    FIXME (imagelist, "deleting hbmColor!\n");
    DeleteObject32 (ii.hbmColor);
//    FIXME (imagelist, "deleted hbmColor!\n");
    DeleteObject32 (ii.hbmMask);

    return (nIndex);
}


/*************************************************************************
 * ImageList_SetBkColor [COMCTL32.70] 
 *
 * Sets the background color of an image list.
 *
 * PARAMS
 *     himl  [I] image list handle
 *     clrBk [I] background color
 *
 * RETURNS
 *     Success: previous background color
 *     Failure: CLR_NONE
 */

COLORREF WINAPI
ImageList_SetBkColor (HIMAGELIST himl, COLORREF clrBk)
{
    COLORREF clrOldBk;

    if (himl == NULL)
	return (CLR_NONE);

    clrOldBk = himl->clrBk;
    himl->clrBk = clrBk;
    return (clrOldBk);
}


/*************************************************************************
 * ImageList_SetDragCursorImage [COMCTL32.75]
 *
 * Combines the specified image with the current drag image
 *
 * PARAMS
 *     himlDrag  [I] drag image list handle
 *     iDrag     [I] drag image index
 *     dxHotspot [I] X position of the hot spot
 *     dyHotspot [I] Y position of the hot spot
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 *
 * BUGS
 *     semi-stub.
 */

BOOL32 WINAPI
ImageList_SetDragCursorImage (HIMAGELIST himlDrag, INT32 iDrag,
			      INT32 dxHotspot, INT32 dyHotspot)
{
    HIMAGELIST himlTemp;

    FIXME (imagelist, "semi-stub!\n");

    if (himlInternalDrag == NULL) return (FALSE);

    TRACE (imagelist, " dxH=%d dyH=%d nX=%d nY=%d\n",
	   dxHotspot, dyHotspot, nInternalDragHotspotX, nInternalDragHotspotY);

    himlTemp = ImageList_Merge (himlInternalDrag, 0, himlDrag, iDrag,
				dxHotspot, dyHotspot);

    ImageList_Destroy (himlInternalDrag);
    himlInternalDrag = himlTemp;

    nInternalDragHotspotX = dxHotspot;
    nInternalDragHotspotY = dyHotspot;

    return (FALSE);
}


/*************************************************************************
 * ImageList_SetFilter [COMCTL32.76] 
 *
 * Sets a filter (or does something completely different)!!???
 *
 * PARAMS
 *     himl     [I] ???
 *     i        [I] ???
 *     dwFilter [I] ???
 *
 * RETURNS
 *     Success: TRUE ???
 *     Failure: FALSE ???
 *
 * BUGS
 *     This is an UNDOCUMENTED function!!!!
 *     empty stub.
 */

BOOL32 WINAPI
ImageList_SetFilter (HIMAGELIST himl, INT32 i, DWORD dwFilter)
{
    FIXME (imagelist, "(%p 0x%x 0x%lx):empty stub!\n",
	   himl, i, dwFilter);

    return FALSE;
}


/*************************************************************************
 * ImageList_SetIconSize [COMCTL32.77]
 *
 * Sets the image size of the bitmap and deletes all images.
 *
 * PARAMS
 *     himl [I] image list handle
 *     cx   [I] image width
 *     cy   [I] image height
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL32 WINAPI
ImageList_SetIconSize (HIMAGELIST himl, INT32 cx, INT32 cy)
{
    INT32 nCount;

    if (himl == NULL) return (FALSE);

    /* remove all images*/
    himl->cMaxImage  = himl->cInitial + himl->cGrow;
    himl->cCurImage  = 0;
    himl->cx         = cx;
    himl->cy         = cy;

    /* initialize overlay mask indices */
    for (nCount = 0; nCount < MAX_OVERLAYIMAGE; nCount++)
        himl->nOvlIdx[nCount] = -1;

    DeleteObject32 (himl->hbmImage);
    himl->hbmImage =
        CreateBitmap32 (himl->cMaxImage * himl->cx, himl->cy,
                        1, himl->uBitsPixel, NULL);

    if (himl->hbmMask) {
        DeleteObject32 (himl->hbmMask);
        himl->hbmMask =
            CreateBitmap32 (himl->cMaxImage * himl->cx, himl->cy,
                            1, 1, NULL);
    }

    return (TRUE);
}


/*************************************************************************
 * ImageList_SetImageCount [COMCTL32.78]
 *
 * Resizes an image list to the specified number of images.
 *
 * PARAMS
 *     himl        [I] image list handle
 *     iImageCount [I] number of images in the image list
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL32 WINAPI
ImageList_SetImageCount (HIMAGELIST himl, INT32 iImageCount)
{
    HDC32     hdcImageList, hdcBitmap;
    HBITMAP32 hbmNewBitmap;
    INT32     nNewCount, nCopyCount;

    if (himl == NULL) return (FALSE);
    if (himl->cCurImage <= iImageCount) return (FALSE);
    if (himl->cMaxImage > iImageCount) return (TRUE);

    nNewCount = iImageCount + himl->cGrow;
    nCopyCount = _MIN(himl->cCurImage, iImageCount);

    hdcImageList = CreateCompatibleDC32 (0);
    hdcBitmap = CreateCompatibleDC32 (0);

    hbmNewBitmap = CreateBitmap32 (nNewCount * himl->cx, himl->cy,
                                   1, himl->uBitsPixel, NULL);
    if (hbmNewBitmap == 0)
    {
        SelectObject32 (hdcImageList, himl->hbmImage);
        SelectObject32 (hdcBitmap, hbmNewBitmap);
        BitBlt32 (hdcBitmap, 0, 0, nCopyCount * himl->cx, himl->cy,
                  hdcImageList, 0, 0, SRCCOPY);
        DeleteObject32 (himl->hbmImage);
        himl->hbmImage = hbmNewBitmap;
    }
    else
    {
        WARN (imagelist, "Could not create new image bitmap !\n");
    }

    if (himl->hbmMask)
    {
        hbmNewBitmap = CreateBitmap32 (nNewCount * himl->cx, himl->cy,
                                       1, 1, NULL);
        if (hbmNewBitmap != 0)
        {
            SelectObject32 (hdcImageList, himl->hbmMask);
            SelectObject32 (hdcBitmap, hbmNewBitmap);
            BitBlt32 (hdcBitmap, 0, 0, nCopyCount * himl->cx, himl->cy,
                      hdcImageList, 0, 0, SRCCOPY);
            DeleteObject32 (himl->hbmMask);
            himl->hbmMask = hbmNewBitmap;
        }
        else
        {
            WARN (imagelist, "Could not create new mask bitmap!\n");
        }
    }

    DeleteDC32 (hdcImageList);
    DeleteDC32 (hdcBitmap);

    /* Update max image count and current image count */
    himl->cMaxImage = nNewCount;
    if (himl->cCurImage > nCopyCount)
        himl->cCurImage = nCopyCount;

    return (TRUE);
}


/*************************************************************************
 * ImageList_SetOverlayImage [COMCTL32.79]
 *
 * Assigns an overlay mask index to an existing image in an image list.
 *
 * PARAMS
 *     himl     [I] image list handle
 *     iImage   [I] image index
 *     iOverlay [I] overlay mask index
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL32 WINAPI
ImageList_SetOverlayImage (HIMAGELIST himl, INT32 iImage, INT32 iOverlay)
{
    if ((iOverlay < 1) || (iOverlay > MAX_OVERLAYIMAGE)) return (FALSE);
    if ((iImage < 0) || (iImage > himl->cCurImage)) return (FALSE);
    
    himl->nOvlIdx[iOverlay - 1] = iImage;
    return TRUE;
}


/*************************************************************************
 * ImageList_Write [COMCTL32.80]
 *
 * Writes an image list to a stream.
 *
 * PARAMS
 *     himl [I] Image list handle.
 *     pstm [O] Pointer to a stream.
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 *
 * NOTES
 *     This function can not be implemented yet, because
 *     IStream32::Write is not implemented.
 *
 * BUGS
 *     empty stub.
 */

BOOL32 WINAPI
ImageList_Write (HIMAGELIST himl, LPSTREAM32 pstm)
{
    FIXME (imagelist, "empty stub!\n");

    if (himl == NULL) return (FALSE);

    return FALSE;
}

