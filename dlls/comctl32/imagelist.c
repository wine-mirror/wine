/*
 *  ImageList implementation
 *
 *  Copyright 1998 Eric Kohl
 *
 *  TODO:
 *    - Fix ImageList_DrawIndirect (xBitmap, yBitmap, rgbFg, rgbBk, dwRop).
 *    - Fix ImageList_GetIcon.
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

 
#include "wine/obj_base.h"
#include "wine/obj_storage.h"
#include "imagelist.h"
#include "commctrl.h"
#include "debug.h"


#define _MAX(a,b) (((a)>(b))?(a):(b))
#define _MIN(a,b) (((a)>(b))?(b):(a))

#define MAX_OVERLAYIMAGE 15


/* internal image list data used for Drag & Drop operations */

static HIMAGELIST himlInternalDrag = NULL;
static INT      nInternalDragHotspotX = 0;
static INT      nInternalDragHotspotY = 0;

static HWND     hwndInternalDrag = 0;
static INT      xInternalPos = 0;
static INT      yInternalPos = 0;

static HDC      hdcBackBuffer = 0;
static HBITMAP  hbmBackBuffer = 0;


/*************************************************************************
 * IMAGELIST_InternalExpandBitmaps [Internal] 
 *
 * Expands the bitmaps of an image list by the given number of images.
 *
 * PARAMS
 *     himl        [I] handle to image list
 *     nImageCount [I] number of images to add
 *
 * RETURNS
 *     nothing
 *
 * NOTES
 *     This function can NOT be used to reduce the number of images.
 */

static VOID
IMAGELIST_InternalExpandBitmaps (HIMAGELIST himl, INT nImageCount)
{
    HDC     hdcImageList, hdcBitmap;
    HBITMAP hbmNewBitmap;
    INT     nNewWidth, nNewCount;

    TRACE(imagelist, "Create expanded bitmaps!\n");

    nNewCount = himl->cCurImage + nImageCount + himl->cGrow;
    nNewWidth = nNewCount * himl->cx;

    hdcImageList = CreateCompatibleDC (0);
    hdcBitmap = CreateCompatibleDC (0);

    hbmNewBitmap =
        CreateBitmap (nNewWidth, himl->cy, 1, himl->uBitsPixel, NULL);
    if (hbmNewBitmap == 0)
        ERR (imagelist, "creating new image bitmap!\n");

    SelectObject (hdcImageList, himl->hbmImage);
    SelectObject (hdcBitmap, hbmNewBitmap);
    BitBlt (hdcBitmap, 0, 0, himl->cCurImage * himl->cx, himl->cy,
              hdcImageList, 0, 0, SRCCOPY);

    DeleteObject (himl->hbmImage);
    himl->hbmImage = hbmNewBitmap;

    if (himl->hbmMask) {
        hbmNewBitmap = 
            CreateBitmap (nNewWidth, himl->cy, 1, 1, NULL);

        if (hbmNewBitmap == 0)
            ERR (imagelist, "creating new mask bitmap!");

        SelectObject (hdcImageList, himl->hbmMask);
        SelectObject (hdcBitmap, hbmNewBitmap);
        BitBlt (hdcBitmap, 0, 0, himl->cCurImage * himl->cx, himl->cy,
                  hdcImageList, 0, 0, SRCCOPY);
        DeleteObject (himl->hbmMask);
        himl->hbmMask = hbmNewBitmap;
    }

    himl->cMaxImage = nNewCount;

    DeleteDC (hdcImageList);
    DeleteDC (hdcBitmap);
}


/*************************************************************************
 * ImageList_Add [COMCTL32.39]
 *
 * Add an image or images to an image list.
 *
 * PARAMS
 *     himl     [I] handle to image list
 *     hbmImage [I] handle to image bitmap
 *     hbmMask  [I] handle to mask bitmap
 *
 * RETURNS
 *     Success: Index of the first new image.
 *     Failure: -1
 */

INT WINAPI
ImageList_Add (HIMAGELIST himl,	HBITMAP hbmImage, HBITMAP hbmMask)
{
    HDC    hdcSrc, hdcDst;
    INT    nFirstIndex, nImageCount;
    INT    nStartX;
    BITMAP bmp;

    if (!himl || !hbmImage)
	return -1;

    GetObjectA (hbmImage, sizeof(BITMAP), (LPVOID)&bmp);
    nImageCount = bmp.bmWidth / himl->cx;

    if (himl->cCurImage + nImageCount >= himl->cMaxImage)
        IMAGELIST_InternalExpandBitmaps (himl, nImageCount);

    nStartX = himl->cCurImage * himl->cx;

    hdcSrc = CreateCompatibleDC (0);
    hdcDst = CreateCompatibleDC (0);

    /* copy image bitmap */
    SelectObject (hdcDst, himl->hbmImage);
    SelectObject (hdcSrc, hbmImage);
    BitBlt (hdcDst, himl->cCurImage * himl->cx, 0,
              bmp.bmWidth, himl->cy, hdcSrc, 0, 0, SRCCOPY);
          
    if (himl->hbmMask) {
        if (hbmMask) {
	    /* copy mask bitmap */
            SelectObject (hdcDst, himl->hbmMask);
            SelectObject (hdcSrc, hbmMask);
            BitBlt (hdcDst, nStartX, 0, bmp.bmWidth, himl->cy,
		      hdcSrc, 0, 0, SRCCOPY);
        }
        else {
	    /* copy monochrome image to the mask bitmap */
            SelectObject (hdcDst, himl->hbmMask);
            SelectObject (hdcSrc, hbmImage);
	    SetBkColor (hdcSrc, GetNearestColor (hdcSrc,
			  GetPixel (hdcSrc, 0, 0)));
	    BitBlt (hdcDst, nStartX, 0, bmp.bmWidth, himl->cy,
		      hdcSrc, nStartX, 0, SRCCOPY);
        }
    }

    DeleteDC (hdcSrc);
    DeleteDC (hdcDst);

    nFirstIndex = himl->cCurImage;
    himl->cCurImage += nImageCount;

    return nFirstIndex;
}


/*************************************************************************
 * ImageList_AddIcon [COMCTL32.40]
 *
 * Adds an icon to an image list.
 *
 * PARAMS
 *     himl  [I] handle to image list
 *     hIcon [I] handle to icon
 *
 * RETURNS
 *     Success: index of the new image
 *     Failure: -1
 */

INT WINAPI
ImageList_AddIcon (HIMAGELIST himl, HICON hIcon)
{
    return ImageList_ReplaceIcon (himl, -1, hIcon);
}


/*************************************************************************
 * ImageList_AddMasked [COMCTL32.41] 
 *
 * Adds an image or images to an image list and creates a mask from the
 * specified bitmap using the mask color.
 *
 * PARAMS
 *     himl    [I] handle to image list.
 *     hBitmap [I] handle to bitmap
 *     clrMask [I] mask color.
 *
 * RETURNS
 *     Success: Index of the first new image.
 *     Failure: -1
 */

INT WINAPI
ImageList_AddMasked (HIMAGELIST himl, HBITMAP hBitmap, COLORREF clrMask)
{
    HDC    hdcImage, hdcMask, hdcBitmap;
    INT    nIndex, nImageCount;
    BITMAP bmp;

    if (himl == NULL)
	return -1;

    if (!GetObjectA (hBitmap, sizeof(BITMAP), &bmp))
	return -1;

    nImageCount = bmp.bmWidth / himl->cx;

    if (himl->cCurImage + nImageCount >= himl->cMaxImage)
        IMAGELIST_InternalExpandBitmaps (himl, nImageCount);

    nIndex = himl->cCurImage;
    himl->cCurImage += nImageCount;

    hdcImage  = CreateCompatibleDC (0);
    hdcBitmap = CreateCompatibleDC (0);

    SelectObject (hdcBitmap, hBitmap);
    SelectObject (hdcImage, himl->hbmImage);
    BitBlt (hdcImage, nIndex * himl->cx, 0, bmp.bmWidth, himl->cy,
              hdcBitmap, 0, 0, SRCCOPY);

    if (himl->hbmMask) {
	COLORREF bkColor = (clrMask != CLR_DEFAULT) ? clrMask :
	    GetNearestColor (hdcBitmap, GetPixel (hdcBitmap, 0, 0));

	/* create mask from image */
        hdcMask = CreateCompatibleDC (0);
        SelectObject (hdcMask, himl->hbmMask);

	/* create monochrome image to the mask bitmap */
	SetBkColor (hdcBitmap, bkColor);
	BitBlt (hdcMask, nIndex * himl->cx, 0, bmp.bmWidth, himl->cy,
		  hdcBitmap, 0, 0, SRCCOPY);

        DeleteDC (hdcMask);
    }

    DeleteDC (hdcImage);
    DeleteDC (hdcBitmap);
  
    return nIndex;
}


/*************************************************************************
 * ImageList_BeginDrag [COMCTL32.42] 
 *
 * Creates a temporary image list that contains one image. It will be used
 * as a drag image.
 *
 * PARAMS
 *     himlTrack [I] handle to the source image list
 *     iTrack    [I] index of the drag image in the source image list
 *     dxHotspot [I] X position of the hot spot of the drag image
 *     dyHotspot [I] Y position of the hot spot of the drag image
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
ImageList_BeginDrag (HIMAGELIST himlTrack, INT iTrack,
	             INT dxHotspot, INT dyHotspot)
{
    HDC hdcSrc, hdcDst;

    FIXME(imagelist, "partially implemented!\n");

    if (himlTrack == NULL)
	return FALSE;

    if (himlInternalDrag)
        ImageList_EndDrag ();

    himlInternalDrag =
	ImageList_Create (himlTrack->cx, himlTrack->cy,
			  himlTrack->flags, 1, 1);
    if (himlInternalDrag == NULL) {
        ERR(imagelist, "Error creating drag image list!\n");
        return FALSE;
    }

    nInternalDragHotspotX = dxHotspot;
    nInternalDragHotspotY = dyHotspot;

    hdcSrc = CreateCompatibleDC (0);
    hdcDst = CreateCompatibleDC (0);

    /* copy image */
    SelectObject (hdcSrc, himlTrack->hbmImage);
    SelectObject (hdcDst, himlInternalDrag->hbmImage);
    StretchBlt (hdcDst, 0, 0, himlInternalDrag->cx, himlInternalDrag->cy, hdcSrc,
                  iTrack * himlTrack->cx, 0, himlTrack->cx, himlTrack->cy, SRCCOPY);

    /* copy mask */
    SelectObject (hdcSrc, himlTrack->hbmMask);
    SelectObject (hdcDst, himlInternalDrag->hbmMask);
    StretchBlt (hdcDst, 0, 0, himlInternalDrag->cx, himlInternalDrag->cy, hdcSrc,
                  iTrack * himlTrack->cx, 0, himlTrack->cx, himlTrack->cy, SRCCOPY);

    DeleteDC (hdcSrc);
    DeleteDC (hdcDst);

    himlInternalDrag->cCurImage = 1;

    return TRUE;
}


/*************************************************************************
 * ImageList_Copy [COMCTL32.43] 
 *
 *  Copies an image of the source image list to an image of the 
 *  destination image list. Images can be copied or swapped.
 *
 * PARAMS
 *     himlDst [I] handle to the destination image list
 *     iDst    [I] destination image index.
 *     himlSrc [I] handle to the source image list
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

BOOL WINAPI
ImageList_Copy (HIMAGELIST himlDst, INT iDst,	HIMAGELIST himlSrc,
		INT iSrc, INT uFlags)
{
    HDC hdcSrc, hdcDst;    

    TRACE(imagelist, "iDst=%d  iSrc=%d\n", iDst, iSrc);

    if ((himlSrc == NULL) || (himlDst == NULL))
	return FALSE;
    if ((iDst < 0) || (iDst >= himlDst->cCurImage))
	return FALSE;
    if ((iSrc < 0) || (iSrc >= himlSrc->cCurImage))
	return FALSE;

    hdcSrc = CreateCompatibleDC (0);
    if (himlDst == himlSrc)
        hdcDst = hdcSrc;
    else
        hdcDst = CreateCompatibleDC (0);

    if (uFlags & ILCF_SWAP) {
        /* swap */
        HBITMAP hbmTempImage, hbmTempMask;

        /* create temporary bitmaps */
        hbmTempImage = CreateBitmap (himlSrc->cx, himlSrc->cy, 1,
                                       himlSrc->uBitsPixel, NULL);
        hbmTempMask = CreateBitmap (himlSrc->cx, himlSrc->cy, 1,
				      1, NULL);

        /* copy (and stretch) destination to temporary bitmaps.(save) */
        /* image */
        SelectObject (hdcSrc, himlDst->hbmImage);
        SelectObject (hdcDst, hbmTempImage);
        StretchBlt (hdcDst, 0, 0, himlSrc->cx, himlSrc->cy,
                      hdcSrc, iDst * himlDst->cx, 0, himlDst->cx, himlDst->cy,
                      SRCCOPY);
        /* mask */
        SelectObject (hdcSrc, himlDst->hbmMask);
        SelectObject (hdcDst, hbmTempMask);
        StretchBlt (hdcDst, 0, 0, himlSrc->cx, himlSrc->cy,
                      hdcSrc, iDst * himlDst->cx, 0, himlDst->cx, himlDst->cy,
                      SRCCOPY);

        /* copy (and stretch) source to destination */
        /* image */
        SelectObject (hdcSrc, himlSrc->hbmImage);
        SelectObject (hdcDst, himlDst->hbmImage);
        StretchBlt (hdcDst, iDst * himlDst->cx, 0, himlDst->cx, himlDst->cy,
                      hdcSrc, iSrc * himlSrc->cx, 0, himlSrc->cx, himlSrc->cy,
                      SRCCOPY);
        /* mask */
        SelectObject (hdcSrc, himlSrc->hbmMask);
        SelectObject (hdcDst, himlDst->hbmMask);
        StretchBlt (hdcDst, iDst * himlDst->cx, 0, himlDst->cx, himlDst->cy,
                      hdcSrc, iSrc * himlSrc->cx, 0, himlSrc->cx, himlSrc->cy,
                      SRCCOPY);

        /* copy (without stretching) temporary bitmaps to source (restore) */
        /* image */
        SelectObject (hdcSrc, hbmTempImage);
        SelectObject (hdcDst, himlSrc->hbmImage);
        BitBlt (hdcDst, iSrc * himlSrc->cx, 0, himlSrc->cx, himlSrc->cy,
                  hdcSrc, 0, 0, SRCCOPY);
        /* mask */
        SelectObject (hdcSrc, hbmTempMask);
        SelectObject (hdcDst, himlSrc->hbmMask);
        BitBlt (hdcDst, iSrc * himlSrc->cx, 0, himlSrc->cx, himlSrc->cy,
                  hdcSrc, 0, 0, SRCCOPY);

        /* delete temporary bitmaps */
        DeleteObject (hbmTempMask);
        DeleteObject (hbmTempImage);
    }
    else {
        /* copy image */
        SelectObject (hdcSrc, himlSrc->hbmImage);
        if (himlSrc == himlDst)
            hdcDst = hdcSrc;
        else
            SelectObject (hdcDst, himlDst->hbmImage);
        StretchBlt (hdcDst, iDst * himlDst->cx, 0, himlDst->cx, himlDst->cy,
                      hdcSrc, iSrc * himlSrc->cx, 0, himlSrc->cx, himlSrc->cy,
                      SRCCOPY);

        /* copy mask */
        SelectObject (hdcSrc, himlSrc->hbmMask);
        if (himlSrc == himlDst)
            hdcDst = hdcSrc;
        else
            SelectObject (hdcDst, himlDst->hbmMask);
        StretchBlt (hdcDst, iDst * himlDst->cx, 0, himlDst->cx, himlDst->cy,
                      hdcSrc, iSrc * himlSrc->cx, 0, himlSrc->cx, himlSrc->cy,
                      SRCCOPY);
    }

    DeleteDC (hdcSrc);
    if (himlSrc != himlDst)
        DeleteDC (hdcDst);

    return TRUE;
}


/*************************************************************************
 * ImageList_Create [COMCTL32.44]  Creates a new image list.
 *
 * PARAMS
 *     cx       [I] image height
 *     cy       [I] image width
 *     flags    [I] creation flags
 *     cInitial [I] initial number of images in the image list
 *     cGrow    [I] number of images by which image list grows
 *
 * RETURNS
 *     Success: Handle to the created image list
 *     Failure: NULL
 */

HIMAGELIST WINAPI
ImageList_Create (INT cx, INT cy, UINT flags,
		  INT cInitial, INT cGrow)
{
    HIMAGELIST himl;
    HDC      hdc;
    INT      nCount;
    HBITMAP  hbmTemp;
    static WORD aBitBlend25[] = 
        {0xAA, 0x00, 0x55, 0x00, 0xAA, 0x00, 0x55, 0x00};

    static WORD aBitBlend50[] =
        {0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA};

    TRACE (imagelist, "(%d %d 0x%x %d %d)\n", cx, cy, flags, cInitial, cGrow);

    himl = (HIMAGELIST)COMCTL32_Alloc (sizeof(struct _IMAGELIST));
    if (!himl)
        return NULL;

    himl->cx        = cx;
    himl->cy        = cy;
    himl->flags     = flags;
    himl->cMaxImage = cInitial + cGrow;
    himl->cInitial  = cInitial;
    himl->cGrow     = cGrow;
    himl->cCurImage = 0;
    himl->clrFg     = CLR_DEFAULT;
    himl->clrBk     = CLR_NONE;

    /* initialize overlay mask indices */
    for (nCount = 0; nCount < MAX_OVERLAYIMAGE; nCount++)
        himl->nOvlIdx[nCount] = -1;

    hdc = CreateCompatibleDC (0);
    himl->uBitsPixel = (UINT)GetDeviceCaps (hdc, BITSPIXEL);
    DeleteDC (hdc);

    TRACE(imagelist, "Image: %d Bits per Pixel\n", himl->uBitsPixel);

    himl->hbmImage =
        CreateBitmap (himl->cx * himl->cMaxImage, himl->cy,
                        1, himl->uBitsPixel, NULL);
    if (himl->hbmImage == 0) {
        ERR(imagelist, "Error creating image bitmap!\n");
        return NULL;
    }

    if (himl->flags & ILC_MASK) {
        himl->hbmMask = CreateBitmap (himl->cx * himl->cMaxImage, himl->cy,
					1, 1, NULL);
        if (himl->hbmMask == 0) {
            ERR(imagelist, "Error creating mask bitmap!\n");
            if (himl->hbmImage)
                DeleteObject (himl->hbmImage);
            return NULL;
        }
    }
    else
        himl->hbmMask = 0;

    /* create blending brushes */
    hbmTemp = CreateBitmap (8, 8, 1, 1, &aBitBlend25);
    himl->hbrBlend25 = CreatePatternBrush (hbmTemp);
    DeleteObject (hbmTemp);

    hbmTemp = CreateBitmap (8, 8, 1, 1, &aBitBlend50);
    himl->hbrBlend50 = CreatePatternBrush (hbmTemp);
    DeleteObject (hbmTemp);

    return himl;
}


/*************************************************************************
 * ImageList_Destroy [COMCTL32.45] 
 *
 * Destroys an image list.
 *
 * PARAMS
 *     himl [I] handle to image list
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
ImageList_Destroy (HIMAGELIST himl)
{ 
    if (!himl)
	return FALSE;

    /* delete image bitmaps */
    if (himl->hbmImage)
        DeleteObject (himl->hbmImage);
    if (himl->hbmMask)
        DeleteObject (himl->hbmMask);

    /* delete blending brushes */
    if (himl->hbrBlend25)
        DeleteObject (himl->hbrBlend25);
    if (himl->hbrBlend50)
        DeleteObject (himl->hbrBlend50);
        
    COMCTL32_Free (himl);

    return TRUE;
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

BOOL WINAPI
ImageList_DragEnter (HWND hwndLock, INT x, INT y)
{
    if (himlInternalDrag == NULL)
	return FALSE;

    if (hwndLock)
	hwndInternalDrag = hwndLock;
    else
	hwndInternalDrag = GetDesktopWindow ();

    xInternalPos = x;
    yInternalPos = y;

    hdcBackBuffer = CreateCompatibleDC (0);
    hbmBackBuffer = CreateCompatibleBitmap (hdcBackBuffer,
		himlInternalDrag->cx, himlInternalDrag->cy);

    ImageList_DragShowNolock (TRUE);

    return FALSE;
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

BOOL WINAPI
ImageList_DragLeave (HWND hwndLock)
{
    if (hwndLock)
	hwndInternalDrag = hwndLock;
    else
	hwndInternalDrag = GetDesktopWindow ();

    ImageList_DragShowNolock (FALSE);

    DeleteDC (hdcBackBuffer);
    DeleteObject (hbmBackBuffer);

    return TRUE;
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

BOOL WINAPI
ImageList_DragMove (INT x, INT y)
{
    ImageList_DragShowNolock (FALSE);

    xInternalPos = x;
    yInternalPos = y;

    ImageList_DragShowNolock (TRUE);

    return FALSE;
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

BOOL WINAPI
ImageList_DragShowNolock (BOOL bShow)
{
    HDC hdcDrag;

    FIXME (imagelist, "semi-stub!\n");
    TRACE (imagelist, "bShow=0x%X!\n", bShow);

    hdcDrag = GetDCEx (hwndInternalDrag, 0,
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

    ReleaseDC (hwndInternalDrag, hdcDrag);

    return FALSE;
}


/*************************************************************************
 * ImageList_Draw [COMCTL32.50] Draws an image.
 *
 * PARAMS
 *     himl   [I] handle to image list
 *     i      [I] image index
 *     hdc    [I] handle to device context
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

BOOL WINAPI
ImageList_Draw (HIMAGELIST himl, INT i, HDC hdc,
		INT x, INT y, UINT fStyle)
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

    return ImageList_DrawIndirect (&imldp);
}


/*************************************************************************
 * ImageList_DrawEx [COMCTL32.51]
 *
 * Draws an image and allows to use extended drawing features.
 *
 * PARAMS
 *     himl   [I] handle to image list
 *     i      [I] image index
 *     hdc    [I] handle to device context
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

BOOL WINAPI
ImageList_DrawEx (HIMAGELIST himl, INT i, HDC hdc, INT x, INT y,
		  INT dx, INT dy, COLORREF rgbBk, COLORREF rgbFg,
		  UINT fStyle)
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

    return ImageList_DrawIndirect (&imldp);
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

BOOL WINAPI
ImageList_DrawIndirect (IMAGELISTDRAWPARAMS *pimldp)
{
    HIMAGELIST himlLocal;
    HDC      hdcImageList, hdcTempImage;
    HBITMAP  hbmTempImage;
    HBRUSH   hBrush, hOldBrush;
    INT      cx, cy;
    INT      nOvlIdx;
    COLORREF   clrBlend;
    BOOL     bImage;       /* draw image ? */
    BOOL     bImageTrans;  /* draw image transparent ? */
    BOOL     bMask;        /* draw mask ? */
    BOOL     bMaskTrans;   /* draw mask transparent ? */
    BOOL     bBlend25;
    BOOL     bBlend50;

    if (pimldp == NULL)
	return FALSE;
    if (pimldp->cbSize < sizeof(IMAGELISTDRAWPARAMS))
	return FALSE;
    if (pimldp->himl == NULL)
	return FALSE;
    if ((pimldp->i < 0) || (pimldp->i >= pimldp->himl->cCurImage))
	return FALSE;

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

    hdcImageList = CreateCompatibleDC (0);

    if (bMask)
    {
	/* draw the mask */
	SelectObject (hdcImageList, himlLocal->hbmMask);
	SetBkColor (hdcImageList, RGB(255, 255, 255));
	SetTextColor (hdcImageList, RGB(0, 0, 0));
	BitBlt (pimldp->hdcDst, pimldp->x, pimldp->y, cx, cy,
		  hdcImageList, himlLocal->cx * pimldp->i, 0,
		  bMaskTrans ? SRCAND : SRCCOPY);
    }

    if (bImage)
    {
	/* draw the image */
	SelectObject (hdcImageList, himlLocal->hbmImage);

        if (!bImageTrans)
        {
            hBrush = CreateSolidBrush (himlLocal->clrBk);
            hOldBrush = SelectObject (pimldp->hdcDst, hBrush);
            PatBlt (pimldp->hdcDst, pimldp->x, pimldp->y,
                      cx, cy, PATCOPY);
            DeleteObject (SelectObject (pimldp->hdcDst, hOldBrush));
        }

        BitBlt (pimldp->hdcDst, pimldp->x, pimldp->y, cx, cy,
                  hdcImageList, himlLocal->cx * pimldp->i, 0, SRCPAINT);

        if (bBlend25 || bBlend50)
        {
            if (pimldp->rgbFg == CLR_DEFAULT)
                clrBlend = GetSysColor (COLOR_HIGHLIGHT);
            else
                clrBlend = pimldp->rgbFg;

            hdcTempImage = CreateCompatibleDC (0);
            hbmTempImage = CreateBitmap (himlLocal->cx, himlLocal->cy,
                                           1, himlLocal->uBitsPixel, NULL);
            SelectObject (hdcTempImage, hbmTempImage);


            /* mask */
            SelectObject (hdcTempImage,
                            bBlend50 ? himlLocal->hbrBlend50 : himlLocal->hbrBlend25);
            PatBlt (hdcTempImage, 0, 0, himlLocal->cx, himlLocal->cy, PATCOPY);

            SelectObject (hdcImageList, himlLocal->hbmMask);
            BitBlt (hdcTempImage, 0, 0, himlLocal->cx,
                      himlLocal->cy, hdcImageList, 
                      pimldp->i * himlLocal->cx, 0, SRCPAINT);

            BitBlt (pimldp->hdcDst, pimldp->x, pimldp->y, cx, cy,
		      hdcTempImage, 0, 0, SRCAND);

            /* fill */
            hBrush = CreateSolidBrush (clrBlend);
            SelectObject (hdcTempImage, hBrush);
            PatBlt (hdcTempImage, 0, 0, himlLocal->cx, himlLocal->cy, PATCOPY);
            DeleteObject (hBrush);

            SelectObject (hdcTempImage,
                            bBlend50 ? himlLocal->hbrBlend50 : himlLocal->hbrBlend25);
            PatBlt (hdcTempImage, 0, 0, himlLocal->cx, himlLocal->cy, 0x0A0329);

            SelectObject (hdcImageList, himlLocal->hbmMask);
            BitBlt (hdcTempImage, 0, 0, himlLocal->cx,
                      himlLocal->cy, hdcImageList, 
                      pimldp->i * himlLocal->cx, 0, SRCPAINT);

            BitBlt (pimldp->hdcDst, pimldp->x, pimldp->y, cx, cy,
                      hdcTempImage, 0, 0, SRCPAINT);

            DeleteObject (hbmTempImage);
            DeleteDC (hdcTempImage);
        }
    }   

    /* Draw overlay image */
    if (pimldp->fStyle & 0x0700) {
	nOvlIdx = (pimldp->fStyle & 0x0700) >> 8;
	if ((nOvlIdx >= 1) && (nOvlIdx <= MAX_OVERLAYIMAGE)) {
	    nOvlIdx = pimldp->himl->nOvlIdx[nOvlIdx - 1];
	    if ((nOvlIdx >= 0) && (nOvlIdx <= pimldp->himl->cCurImage)) {
		if (pimldp->himl->hbmMask) {  
                    SelectObject (hdcImageList, pimldp->himl->hbmMask);
                    BitBlt (pimldp->hdcDst, pimldp->x, pimldp->y, cx, cy,
                              hdcImageList, pimldp->himl->cx * nOvlIdx, 0,
			      SRCAND);
		}  
                SelectObject (hdcImageList, pimldp->himl->hbmImage);
                BitBlt (pimldp->hdcDst, pimldp->x, pimldp->y,
                          cx, cy, hdcImageList,
                          pimldp->himl->cx * nOvlIdx, 0, SRCPAINT);
            }
        }
    }

    DeleteDC (hdcImageList);
  
    return TRUE;
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
    HDC hdcSrc, hdcDst;

    if (himlSrc == NULL) {
        ERR (imagelist, "Invalid image list handle!\n");
        return NULL;
    }

    himlDst = ImageList_Create (himlSrc->cx, himlSrc->cy, himlSrc->flags,
                                himlSrc->cInitial, himlSrc->cGrow);

    if (himlDst)
    {
        hdcSrc = CreateCompatibleDC (0);
        hdcDst = CreateCompatibleDC (0);
        SelectObject (hdcSrc, himlSrc->hbmImage);
        SelectObject (hdcDst, himlDst->hbmImage);
        BitBlt (hdcDst, 0, 0, himlSrc->cCurImage * himlSrc->cx, himlSrc->cy,
                  hdcSrc, 0, 0, SRCCOPY);

        if (himlDst->hbmMask)
        {
            SelectObject (hdcSrc, himlSrc->hbmMask);
            SelectObject (hdcDst, himlDst->hbmMask);
            BitBlt (hdcDst, 0, 0, himlSrc->cCurImage * himlSrc->cx,
                      himlSrc->cy, hdcSrc, 0, 0, SRCCOPY);
        }

        DeleteDC (hdcDst);
        DeleteDC (hdcSrc);
    }

    return himlDst;
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

BOOL WINAPI
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

    return TRUE;
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

    return himl->clrBk;
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
ImageList_GetDragImage (POINT *ppt, POINT *pptHotspot)
{
    FIXME (imagelist, "semi-stub!\n");

    if (himlInternalDrag)
        return (himlInternalDrag);

    return NULL;
}


/*************************************************************************
 * ImageList_GetIcon [COMCTL32.57] 
 *
 * Creates an icon from a masked image of an image list.
 *
 * PARAMS
 *     himl  [I] handle to image list
 *     i     [I] image index
 *     flags [I] drawing style flags
 *
 * RETURNS
 *     Success: icon handle
 *     Failure: NULL
 */

HICON WINAPI
ImageList_GetIcon (HIMAGELIST himl, INT i, UINT fStyle)
{
    ICONINFO ii;
    HICON  hIcon;
    HDC    hdcSrc, hdcDst;
    INT    nWidth, nHeight;

    if ((himl == NULL) || (i < 0) || (i >= himl->cCurImage))
	return 0;

    nWidth = GetSystemMetrics (SM_CXICON);
    nHeight = GetSystemMetrics (SM_CYICON);

    hdcSrc = CreateCompatibleDC(0);
    hdcDst = CreateCompatibleDC(0);

    ii.fIcon = TRUE;
    ii.xHotspot = nWidth / 2;
    ii.yHotspot = nHeight / 2;
    ii.hbmMask  = CreateCompatibleBitmap (hdcDst, nWidth, nHeight);
    ii.hbmColor = CreateCompatibleBitmap (hdcDst, nWidth, nHeight);


    /* draw mask*/
    SelectObject (hdcDst, ii.hbmMask);
    if (himl->hbmMask) {
	SelectObject (hdcSrc, himl->hbmMask);
	BitBlt (hdcDst, 0, 0, nWidth, nHeight,
		  hdcSrc, i * himl->cx, 0, SRCCOPY);
    }
    else
	PatBlt (hdcDst, 0, 0, nWidth, nHeight, BLACKNESS);

    /* draw image*/
    SelectObject (hdcDst, ii.hbmColor);
    SelectObject (hdcSrc, himl->hbmImage);
    BitBlt (hdcDst, 0, 0, nWidth, nHeight,
	      hdcSrc, i * himl->cx, 0, SRCCOPY);

    hIcon = CreateIconIndirect (&ii);    

    DeleteDC (hdcSrc);
    DeleteDC (hdcDst);
    DeleteObject (ii.hbmMask);
    DeleteObject (ii.hbmColor);

    return hIcon;
}


/*************************************************************************
 * ImageList_GetIconSize [COMCTL32.58]
 *
 * Retrieves the size of an image in an image list.
 *
 * PARAMS
 *     himl [I] handle to image list
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

BOOL WINAPI
ImageList_GetIconSize (HIMAGELIST himl, INT *cx, INT *cy)
{
    if (himl == NULL)
	return FALSE;
    if ((himl->cx <= 0) || (himl->cy <= 0))
	return FALSE;

    if (cx)
	*cx = himl->cx;
    if (cy)
	*cy = himl->cy;

    return TRUE;
}


/*************************************************************************
 * ImageList_GetImageCount [COMCTL32.59]
 *
 * Returns the number of images in an image list.
 *
 * PARAMS
 *     himl [I] handle to image list
 *
 * RETURNS
 *     Success: Number of images.
 *     Failure: 0
 */

INT WINAPI
ImageList_GetImageCount (HIMAGELIST himl)
{
    if (himl == NULL)
	return 0;

    return himl->cCurImage;
}


/*************************************************************************
 * ImageList_GetImageInfo [COMCTL32.60]
 *
 * Returns information about an image in an image list.
 *
 * PARAMS
 *     himl       [I] handle to image list
 *     i          [I] image index
 *     pImageInfo [O] pointer to the image information
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
ImageList_GetImageInfo (HIMAGELIST himl, INT i, IMAGEINFO *pImageInfo)
{
    if ((himl == NULL) || (pImageInfo == NULL))
	return FALSE;
    if ((i < 0) || (i >= himl->cCurImage))
	return FALSE;

    pImageInfo->hbmImage = himl->hbmImage;
    pImageInfo->hbmMask  = himl->hbmMask;
    
    pImageInfo->rcImage.top    = 0;
    pImageInfo->rcImage.bottom = himl->cy;
    pImageInfo->rcImage.left   = i * himl->cx;
    pImageInfo->rcImage.right  = (i+1) * himl->cx;
    
    return TRUE;
}


/*************************************************************************
 * ImageList_GetImageRect [COMCTL32.61] 
 *
 * Retrieves the rectangle of the specified image in an image list.
 *
 * PARAMS
 *     himl   [I] handle to image list
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

BOOL WINAPI
ImageList_GetImageRect (HIMAGELIST himl, INT i, LPRECT lpRect)
{
    if ((himl == NULL) || (lpRect == NULL))
	return FALSE;
    if ((i < 0) || (i >= himl->cCurImage))
	return FALSE;

    lpRect->left   = i * himl->cx;
    lpRect->top    = 0;
    lpRect->right  = lpRect->left + himl->cx;
    lpRect->bottom = himl->cy;

    return TRUE;
}


/*************************************************************************
 * ImageList_LoadImage32A [COMCTL32.63][COMCTL32.62]
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
 *     Success: handle to the loaded image list
 *     Failure: NULL
 *
 * SEE
 *     LoadImage ()
 */

HIMAGELIST WINAPI
ImageList_LoadImageA (HINSTANCE hi, LPCSTR lpbmp, INT cx,	INT cGrow,
			COLORREF clrMask, UINT uType,	UINT uFlags)
{
    HIMAGELIST himl = NULL;
    HANDLE   handle;
    INT      nImageCount;

    handle = LoadImageA (hi, lpbmp, uType, 0, 0, uFlags);
    if (!handle) {
        ERR (imagelist, "Error loading image!\n");
        return NULL;
    }

    if (uType == IMAGE_BITMAP) {
        BITMAP bmp;
        GetObjectA (handle, sizeof(BITMAP), &bmp);
        nImageCount = bmp.bmWidth / cx;

        himl = ImageList_Create (cx, bmp.bmHeight, ILC_MASK | ILC_COLOR,
                                 nImageCount, cGrow);
        ImageList_AddMasked (himl, (HBITMAP)handle, clrMask);
    }
    else if ((uType == IMAGE_ICON) || (uType == IMAGE_CURSOR)) {
        ICONINFO ii;
        BITMAP bmp;

        GetIconInfo (handle, &ii);
        GetObjectA (ii.hbmColor, sizeof(BITMAP), (LPVOID)&bmp);
        himl = ImageList_Create (bmp.bmWidth, bmp.bmHeight, 
                                 ILC_MASK | ILC_COLOR, 1, cGrow);
        ImageList_Add (himl, ii.hbmColor, ii.hbmMask);
        DeleteObject (ii.hbmColor);
        DeleteObject (ii.hbmMask);
    }

    DeleteObject (handle);
    
    return himl;
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
 *     Success: handle to the loaded image list
 *     Failure: NULL
 *
 * SEE
 *     LoadImage ()
 */

HIMAGELIST WINAPI
ImageList_LoadImageW (HINSTANCE hi, LPCWSTR lpbmp, INT cx, INT cGrow,
			COLORREF clrMask, UINT uType,	UINT uFlags)
{
    HIMAGELIST himl = NULL;
    HANDLE   handle;
    INT      nImageCount;

    handle = LoadImageW (hi, lpbmp, uType, 0, 0, uFlags);
    if (!handle) {
        ERR (imagelist, "Error loading image!\n");
        return NULL;
    }

    if (uType == IMAGE_BITMAP) {
        BITMAP bmp;
        GetObjectA (handle, sizeof(BITMAP), &bmp);
        nImageCount = bmp.bmWidth / cx;

        himl = ImageList_Create (cx, bmp.bmHeight, ILC_MASK | ILC_COLOR,
                                 nImageCount, cGrow);
        ImageList_AddMasked (himl, (HBITMAP)handle, clrMask);
    }
    else if ((uType == IMAGE_ICON) || (uType == IMAGE_CURSOR)) {
        ICONINFO ii;
        BITMAP bmp;

        GetIconInfo (handle, &ii);
        GetObjectA (ii.hbmMask, sizeof(BITMAP), (LPVOID)&bmp);
        himl = ImageList_Create (bmp.bmWidth, bmp.bmHeight, 
                                 ILC_MASK | ILC_COLOR, 1, cGrow);
        ImageList_Add (himl, ii.hbmColor, ii.hbmMask);
        DeleteObject (ii.hbmColor);
        DeleteObject (ii.hbmMask);
    }

    DeleteObject (handle);
    
    return himl;
}


/*************************************************************************
 * ImageList_Merge [COMCTL32.65] 
 *
 * Creates a new image list that contains a merged image from the specified
 * images of both source image lists.
 *
 * PARAMS
 *     himl1 [I] handle to first image list
 *     i1    [I] first image index
 *     himl2 [I] handle to second image list
 *     i2    [I] second image index
 *     dx    [I] X offset of the second image relative to the first.
 *     dy    [I] Y offset of the second image relative to the first.
 *
 * RETURNS
 *     Success: handle of the merged image list.
 *     Failure: NULL
 */

HIMAGELIST WINAPI
ImageList_Merge (HIMAGELIST himl1, INT i1, HIMAGELIST himl2, INT i2,
		 INT dx, INT dy)
{
    HIMAGELIST himlDst = NULL;
    HDC      hdcSrcImage, hdcDstImage;
    INT      cxDst, cyDst;
    INT      xOff1, yOff1, xOff2, yOff2;
    INT      nX1, nX2;

    if ((himl1 == NULL) || (himl2 == NULL))
	return NULL;

    /* check indices */
    if ((i1 < 0) || (i1 >= himl1->cCurImage)) {
        ERR (imagelist, "Index 1 out of range! %d\n", i1);
        return NULL;
    }

    if ((i2 < 0) || (i2 >= himl2->cCurImage)) {
        ERR (imagelist, "Index 2 out of range! %d\n", i2);
        return NULL;
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
        hdcSrcImage = CreateCompatibleDC (0);
        hdcDstImage = CreateCompatibleDC (0);
        nX1 = i1 * himl1->cx;
        nX2 = i2 * himl2->cx;
        
        /* copy image */
        SelectObject (hdcSrcImage, himl1->hbmImage);
        SelectObject (hdcDstImage, himlDst->hbmImage);
        BitBlt (hdcDstImage, 0, 0, cxDst, cyDst, 
                  hdcSrcImage, 0, 0, BLACKNESS);
        BitBlt (hdcDstImage, xOff1, yOff1, himl1->cx, himl1->cy, 
                  hdcSrcImage, nX1, 0, SRCCOPY);

        SelectObject (hdcSrcImage, himl2->hbmMask);
        BitBlt (hdcDstImage, xOff2, yOff2, himl2->cx, himl2->cy, 
                  hdcSrcImage, nX2, 0, SRCAND);

        SelectObject (hdcSrcImage, himl2->hbmImage);
        BitBlt (hdcDstImage, xOff2, yOff2, himl2->cx, himl2->cy, 
                  hdcSrcImage, nX2, 0, SRCPAINT);

        /* copy mask */
        SelectObject (hdcSrcImage, himl1->hbmMask);
        SelectObject (hdcDstImage, himlDst->hbmMask);
        BitBlt (hdcDstImage, 0, 0, cxDst, cyDst, 
                  hdcSrcImage, 0, 0, WHITENESS);
        BitBlt (hdcDstImage, xOff1, yOff1, himl1->cx, himl1->cy, 
                  hdcSrcImage, nX1, 0, SRCCOPY);

        SelectObject (hdcSrcImage, himl2->hbmMask);
        BitBlt (hdcDstImage, xOff2, yOff2, himl2->cx, himl2->cy, 
                  hdcSrcImage, nX2, 0, SRCAND);

        DeleteDC (hdcSrcImage);
        DeleteDC (hdcDstImage);
    }
   
    return himlDst;
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
 *     Success: handle to image list
 *     Failure: NULL
 *
 * NOTES
 *     This function can not be implemented yet, because
 *     IStream32::Read is not implemented yet.
 *
 * BUGS
 *     empty stub.
 */

HIMAGELIST WINAPI ImageList_Read (LPSTREAM pstm)
{
    FIXME (imagelist, "empty stub!\n");


    return NULL;
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

BOOL WINAPI
ImageList_Remove (HIMAGELIST himl, INT i)
{
    HBITMAP hbmNewImage, hbmNewMask;
    HDC     hdcSrc, hdcDst;
    INT     cxNew, nCount;

    if ((i < -1) || (i >= himl->cCurImage)) {
        ERR (imagelist, "index out of range! %d\n", i);
        return FALSE;
    }

    if (himl->cCurImage == 0) {
        ERR (imagelist, "image list is already empty!\n");
        return FALSE;
    }

    if (i == -1) {
        /* remove all */
        TRACE (imagelist, "remove all!\n");

        himl->cMaxImage = himl->cInitial + himl->cGrow;
        himl->cCurImage = 0;
        for (nCount = 0; nCount < MAX_OVERLAYIMAGE; nCount++)
             himl->nOvlIdx[nCount] = -1;

        DeleteObject (himl->hbmImage);
        himl->hbmImage =
            CreateBitmap (himl->cMaxImage * himl->cx, himl->cy,
                            1, himl->uBitsPixel, NULL);

        if (himl->hbmMask) {
            DeleteObject (himl->hbmMask);
            himl->hbmMask =
                CreateBitmap (himl->cMaxImage * himl->cx, himl->cy,
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
            CreateBitmap (cxNew, himl->cy, 1, himl->uBitsPixel, NULL);

        if (himl->hbmMask)
            hbmNewMask = CreateBitmap (cxNew, himl->cy, 1, 1, NULL);
        else
            hbmNewMask = 0;  /* Just to keep compiler happy! */

        hdcSrc = CreateCompatibleDC (0);
        hdcDst = CreateCompatibleDC (0);

        /* copy all images and masks prior to the "removed" image */
        if (i > 0) {
            TRACE (imagelist, "Pre image copy: Copy %d images\n", i);
       
            SelectObject (hdcSrc, himl->hbmImage);
            SelectObject (hdcDst, hbmNewImage);
            BitBlt (hdcDst, 0, 0, i * himl->cx, himl->cy,
                      hdcSrc, 0, 0, SRCCOPY);

            if (himl->hbmMask) {
                SelectObject (hdcSrc, himl->hbmMask);
                SelectObject (hdcDst, hbmNewMask);
                BitBlt (hdcDst, 0, 0, i * himl->cx, himl->cy,
                          hdcSrc, 0, 0, SRCCOPY);
            }
        }

        /* copy all images and masks behind the removed image */
        if (i < himl->cCurImage - 1) {
            TRACE (imagelist, "Post image copy!\n");
            SelectObject (hdcSrc, himl->hbmImage);
            SelectObject (hdcDst, hbmNewImage);
            BitBlt (hdcDst, i * himl->cx, 0, (himl->cCurImage - i - 1) * himl->cx,
                      himl->cy, hdcSrc, (i + 1) * himl->cx, 0, SRCCOPY);

            if (himl->hbmMask) {
                SelectObject (hdcSrc, himl->hbmMask);
                SelectObject (hdcDst, hbmNewMask);
                BitBlt (hdcDst, i * himl->cx, 0,
                          (himl->cCurImage - i - 1) * himl->cx,
                          himl->cy, hdcSrc, (i + 1) * himl->cx, 0, SRCCOPY);
            }
        }

        DeleteDC (hdcSrc);
        DeleteDC (hdcDst);

        /* delete old images and insert new ones */
        DeleteObject (himl->hbmImage);
        himl->hbmImage = hbmNewImage;
        if (himl->hbmMask) {
            DeleteObject (himl->hbmMask);
            himl->hbmMask = hbmNewMask;
        }

        himl->cCurImage--;
        himl->cMaxImage = himl->cCurImage + himl->cGrow;
    }

    return TRUE;
}


/*************************************************************************
 * ImageList_Replace [COMCTL32.68] 
 *
 * Replaces an image in an image list with a new image.
 *
 * PARAMS
 *     himl     [I] handle to image list
 *     i        [I] image index
 *     hbmImage [I] handle to image bitmap
 *     hbmMask  [I] handle to mask bitmap. Can be NULL.
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
ImageList_Replace (HIMAGELIST himl, INT i, HBITMAP hbmImage,
		   HBITMAP hbmMask)
{
    HDC hdcImageList, hdcImage;
    BITMAP bmp;

    if (himl == NULL) {
        ERR (imagelist, "Invalid image list handle!\n");
        return FALSE;
    }
    
    if ((i >= himl->cCurImage) || (i < 0)) {
        ERR (imagelist, "Invalid image index!\n");
        return FALSE;
    }

    hdcImageList = CreateCompatibleDC (0);
    hdcImage = CreateCompatibleDC (0);
    GetObjectA (hbmImage, sizeof(BITMAP), (LPVOID)&bmp);

    /* Replace Image */
    SelectObject (hdcImageList, himl->hbmImage);
    SelectObject (hdcImage, hbmImage);

    StretchBlt (hdcImageList, i * himl->cx, 0, himl->cx, himl->cy,
                  hdcImage, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);

    if (himl->hbmMask)
    {
        /* Replace Mask */
        SelectObject (hdcImageList, himl->hbmMask);
        SelectObject (hdcImage, hbmMask);

        StretchBlt (hdcImageList, i * himl->cx, 0, himl->cx, himl->cy,
                      hdcImage, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);
    }

    DeleteDC (hdcImage);
    DeleteDC (hdcImageList);

    return TRUE;
}


/*************************************************************************
 * ImageList_ReplaceIcon [COMCTL32.69]
 *
 * Replaces an image in an image list using an icon.
 *
 * PARAMS
 *     himl  [I] handle to image list
 *     i     [I] image index
 *     hIcon [I] handle to icon
 *
 * RETURNS
 *     Success: index of the replaced image
 *     Failure: -1
 */

INT WINAPI
ImageList_ReplaceIcon (HIMAGELIST himl, INT i, HICON hIcon)
{
    HDC     hdcImageList, hdcImage;
    INT     nIndex;
    HBITMAP hbmOldSrc, hbmOldDst;
    ICONINFO  ii;
    BITMAP  bmp;

    TRACE (imagelist, "(0x%lx 0x%x 0x%x)\n", (DWORD)himl, i, hIcon);

    if (himl == NULL)
	return -1;
    if ((i >= himl->cCurImage) || (i < -1))
	return -1;

    GetIconInfo (hIcon, &ii);
    if (ii.hbmMask == 0)
	ERR (imagelist, "no mask!\n");
    if (ii.hbmColor == 0)
	ERR (imagelist, "no color!\n");
    GetObjectA (ii.hbmMask, sizeof(BITMAP), (LPVOID)&bmp);

    if (i == -1) {
        if (himl->cCurImage + 1 >= himl->cMaxImage)
            IMAGELIST_InternalExpandBitmaps (himl, 1);

        nIndex = himl->cCurImage;
        himl->cCurImage++;
    }
    else
        nIndex = i;

    hdcImageList = CreateCompatibleDC (0);
    TRACE (imagelist, "hdcImageList=0x%x!\n", hdcImageList);
    if (hdcImageList == 0)
	ERR (imagelist, "invalid hdcImageList!\n");

    hdcImage = CreateCompatibleDC (0);
    TRACE (imagelist, "hdcImage=0x%x!\n", hdcImage);
    if (hdcImage == 0)
	ERR (imagelist, "invalid hdcImage!\n");

    hbmOldDst = SelectObject (hdcImageList, himl->hbmImage);
    SetTextColor( hdcImageList, RGB(0,0,0));
    SetBkColor( hdcImageList, RGB(255,255,255));
    hbmOldSrc = SelectObject (hdcImage, ii.hbmColor);
    StretchBlt (hdcImageList, nIndex * himl->cx, 0, himl->cx, himl->cy,
                  hdcImage, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);

    if (himl->hbmMask) {
        SelectObject (hdcImageList, himl->hbmMask);
        SelectObject (hdcImage, ii.hbmMask);
        StretchBlt (hdcImageList, nIndex * himl->cx, 0, himl->cx, himl->cy,
                      hdcImage, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);
    }

    SelectObject (hdcImage, hbmOldSrc);
    SelectObject (hdcImageList, hbmOldDst);

    if (hdcImageList)
	DeleteDC (hdcImageList);
    if (hdcImage)
	DeleteDC (hdcImage);
    if (ii.hbmColor)
	DeleteObject (ii.hbmColor);
    if (ii.hbmMask)
	DeleteObject (ii.hbmMask);

    return nIndex;
}


/*************************************************************************
 * ImageList_SetBkColor [COMCTL32.70] 
 *
 * Sets the background color of an image list.
 *
 * PARAMS
 *     himl  [I] handle to image list
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
	return CLR_NONE;

    clrOldBk = himl->clrBk;
    himl->clrBk = clrBk;
    return clrOldBk;
}


/*************************************************************************
 * ImageList_SetDragCursorImage [COMCTL32.75]
 *
 * Combines the specified image with the current drag image
 *
 * PARAMS
 *     himlDrag  [I] handle to drag image list
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

BOOL WINAPI
ImageList_SetDragCursorImage (HIMAGELIST himlDrag, INT iDrag,
			      INT dxHotspot, INT dyHotspot)
{
    HIMAGELIST himlTemp;

    FIXME (imagelist, "semi-stub!\n");

    if (himlInternalDrag == NULL)
	return FALSE;

    TRACE (imagelist, " dxH=%d dyH=%d nX=%d nY=%d\n",
	   dxHotspot, dyHotspot, nInternalDragHotspotX, nInternalDragHotspotY);

    himlTemp = ImageList_Merge (himlInternalDrag, 0, himlDrag, iDrag,
				dxHotspot, dyHotspot);

    ImageList_Destroy (himlInternalDrag);
    himlInternalDrag = himlTemp;

    nInternalDragHotspotX = dxHotspot;
    nInternalDragHotspotY = dyHotspot;

    return FALSE;
}


/*************************************************************************
 * ImageList_SetFilter [COMCTL32.76] 
 *
 * Sets a filter (or does something completely different)!!???
 *
 * PARAMS
 *     himl     [I] handle to image list
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

BOOL WINAPI
ImageList_SetFilter (HIMAGELIST himl, INT i, DWORD dwFilter)
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
 *     himl [I] handle to image list
 *     cx   [I] image width
 *     cy   [I] image height
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
ImageList_SetIconSize (HIMAGELIST himl, INT cx, INT cy)
{
    INT nCount;

    if (!himl)
	return FALSE;

    /* remove all images*/
    himl->cMaxImage = himl->cInitial + himl->cGrow;
    himl->cCurImage = 0;
    himl->cx        = cx;
    himl->cy        = cy;

    /* initialize overlay mask indices */
    for (nCount = 0; nCount < MAX_OVERLAYIMAGE; nCount++)
        himl->nOvlIdx[nCount] = -1;

    DeleteObject (himl->hbmImage);
    himl->hbmImage =
        CreateBitmap (himl->cMaxImage * himl->cx, himl->cy,
                        1, himl->uBitsPixel, NULL);

    if (himl->hbmMask) {
        DeleteObject (himl->hbmMask);
        himl->hbmMask =
            CreateBitmap (himl->cMaxImage * himl->cx, himl->cy,
                            1, 1, NULL);
    }

    return TRUE;
}


/*************************************************************************
 * ImageList_SetImageCount [COMCTL32.78]
 *
 * Resizes an image list to the specified number of images.
 *
 * PARAMS
 *     himl        [I] handle to image list
 *     iImageCount [I] number of images in the image list
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
ImageList_SetImageCount (HIMAGELIST himl, INT iImageCount)
{
    HDC     hdcImageList, hdcBitmap;
    HBITMAP hbmNewBitmap;
    INT     nNewCount, nCopyCount;

    if (!himl)
	return FALSE;
    if (himl->cCurImage <= iImageCount)
	return FALSE;
    if (himl->cMaxImage > iImageCount)
	return TRUE;

    nNewCount = iImageCount + himl->cGrow;
    nCopyCount = _MIN(himl->cCurImage, iImageCount);

    hdcImageList = CreateCompatibleDC (0);
    hdcBitmap = CreateCompatibleDC (0);

    hbmNewBitmap = CreateBitmap (nNewCount * himl->cx, himl->cy,
                                   1, himl->uBitsPixel, NULL);
    if (hbmNewBitmap == 0)
    {
        SelectObject (hdcImageList, himl->hbmImage);
        SelectObject (hdcBitmap, hbmNewBitmap);

	/* copy images */
        BitBlt (hdcBitmap, 0, 0, nCopyCount * himl->cx, himl->cy,
                  hdcImageList, 0, 0, SRCCOPY);

	/* delete 'empty' image space */
	SetBkColor (hdcBitmap, RGB(255, 255, 255));
	SetTextColor (hdcBitmap, RGB(0, 0, 0));
	PatBlt (hdcBitmap,  nCopyCount * himl->cx, 0, 
		  (nNewCount - nCopyCount) * himl->cx, himl->cy, BLACKNESS);

	DeleteObject (himl->hbmImage);
	himl->hbmImage = hbmNewBitmap;
    }
    else
	ERR (imagelist, "Could not create new image bitmap !\n");

    if (himl->hbmMask)
    {
        hbmNewBitmap = CreateBitmap (nNewCount * himl->cx, himl->cy,
                                       1, 1, NULL);
        if (hbmNewBitmap != 0)
        {
            SelectObject (hdcImageList, himl->hbmMask);
            SelectObject (hdcBitmap, hbmNewBitmap);

	    /* copy images */
            BitBlt (hdcBitmap, 0, 0, nCopyCount * himl->cx, himl->cy,
                      hdcImageList, 0, 0, SRCCOPY);

	    /* delete 'empty' image space */
	    SetBkColor (hdcBitmap, RGB(255, 255, 255));
	    SetTextColor (hdcBitmap, RGB(0, 0, 0));
            PatBlt (hdcBitmap,  nCopyCount * himl->cx, 0, 
		      (nNewCount - nCopyCount) * himl->cx, himl->cy, BLACKNESS);

            DeleteObject (himl->hbmMask);
            himl->hbmMask = hbmNewBitmap;
        }
        else
            ERR (imagelist, "Could not create new mask bitmap!\n");
    }

    DeleteDC (hdcImageList);
    DeleteDC (hdcBitmap);

    /* Update max image count and current image count */
    himl->cMaxImage = nNewCount;
    if (himl->cCurImage > nCopyCount)
        himl->cCurImage = nCopyCount;

    return TRUE;
}


/*************************************************************************
 * ImageList_SetOverlayImage [COMCTL32.79]
 *
 * Assigns an overlay mask index to an existing image in an image list.
 *
 * PARAMS
 *     himl     [I] handle to image list
 *     iImage   [I] image index
 *     iOverlay [I] overlay mask index
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
ImageList_SetOverlayImage (HIMAGELIST himl, INT iImage, INT iOverlay)
{
    if (!himl)
	return FALSE;
    if ((iOverlay < 1) || (iOverlay > MAX_OVERLAYIMAGE))
	return FALSE;
    if ((iImage < 0) || (iImage > himl->cCurImage))
	return FALSE;
    
    himl->nOvlIdx[iOverlay - 1] = iImage;
    return TRUE;
}


/*************************************************************************
 * ImageList_Write [COMCTL32.80]
 *
 * Writes an image list to a stream.
 *
 * PARAMS
 *     himl [I] handle to image list
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

BOOL WINAPI
ImageList_Write (HIMAGELIST himl, LPSTREAM pstm)
{
    if (!himl)
	return FALSE;

    FIXME (imagelist, "empty stub!\n");

    return FALSE;
}

