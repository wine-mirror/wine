/*
 *  ImageList implementation
 *
 *  Copyright 1998 Eric Kohl
 *
 *  TODO:
 *    - Improve the documentation.
 *    - Improve error checking in some functions.
 *    - Fix ILD_TRANSPARENT error in ImageList_DrawIndirect.
 *    - Fix offsets in ImageList_DrawIndirect.
 *    - Fix ImageList_GetIcon (might be a result of the
 *      ILD_TRANSPARENT error in ImageList_DrawIndirect).
 *    - Fix drag functions.
 *    - Fix all other stubs.
 *    - Add ImageList_SetFilter (undocumented).
 *      BTW does anybody know anything about this function???
 *        - It removes 12 Bytes from the stack (3 Parameters).
 *        - First parameter SHOULD be a HIMAGELIST.
 *        - Second parameter COULD be an index?????
 *        - Third parameter.... ?????????????????????
 *
 *  Testing:
 *    - Test ImageList_GetImageRect (undocumented).
 *    - Test all the other functions.
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
 
/* This must be defined until "GetIconInfo" is implemented completely.
 * To do that the Cursor and Icon code in objects/cursoricon.c must
 * be rewritten.
 */
#define __GET_ICON_INFO_HACK__ 
 
#include "windows.h"
#include "imagelist.h"
#include "commctrl.h"
#include "debug.h"

#ifdef __GET_ICON_INFO_HACK__
#include "bitmap.h"
#endif


#define _MAX(a,b) (((a)>(b))?(a):(b))
#define _MIN(a,b) (((a)>(b))?(b):(a))

#define MAX_OVERLAYIMAGE 15


/*
 * internal ImageList data used for dragging
 */
static HIMAGELIST himlInternalDrag = NULL;
static INT32      nInternalDragHotspotX = 0;
static INT32      nInternalDragHotspotY = 0;
static HCURSOR32  hcurInternal = 0;


/*************************************************************************
 *	 		 IMAGELIST_InternalGrowBitmaps   [Internal]
 *
 *  Grows the bitmaps of the given image list by the given number of 
 *  images. Can NOT be used to reduce the number of images.
 */

static void IMAGELIST_InternalGrowBitmaps (
	HIMAGELIST himl,    /* image list handle */
	INT32 nImageCount)  /* number of images to grow by */
{
    HDC32     hdcImageList, hdcBitmap;
    HBITMAP32 hbmNewBitmap;
    INT32     nNewWidth, nNewCount;

    TRACE(imagelist, "Create grown bitmaps!\n");

    nNewCount = himl->cCurImage + nImageCount + himl->cGrow;
    nNewWidth = nNewCount * himl->cx;

    hdcImageList = CreateCompatibleDC32 (0);
    hdcBitmap = CreateCompatibleDC32 (0);

    hbmNewBitmap =
        CreateBitmap32 (nNewWidth, himl->cy, 1, himl->uBitsPixel, NULL);
    if (hbmNewBitmap == 0)
        ERR (imagelist, "Error creating new image bitmap!\n");

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
            ERR (imagelist, "Error creating new mask bitmap!");

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
 *	 		 ImageList_Add   [COMCTL32.39]
 *
 *  Add an image (and a mask) to an image list.
 *
 *  RETURNS
 *    Index of the first image that was added.
 *    -1 if an error occurred.
 */

INT32 WINAPI ImageList_Add (
	HIMAGELIST himl,     /* imagelist handle */
	HBITMAP32 hbmImage,  /* image bitmap */
	HBITMAP32 hbmMask)   /* mask bitmap */
{
    HDC32    hdcImageList, hdcImage, hdcMask;
    INT32    nFirstIndex, nImageCount;
    INT32    nStartX, nRunX, nRunY;
    BITMAP32 bmp;

    if (himl == NULL) return (-1);

    hdcMask = 0; /* to keep compiler happy ;-) */

    GetObject32A (hbmImage, sizeof(BITMAP32), (LPVOID)&bmp);
    nImageCount = bmp.bmWidth / himl->cx;

    if (himl->cCurImage + nImageCount >= himl->cMaxImage)
        IMAGELIST_InternalGrowBitmaps (himl, nImageCount);

    hdcImageList = CreateCompatibleDC32 (0);
    hdcImage = CreateCompatibleDC32 (0);

    SelectObject32 (hdcImageList, himl->hbmImage);
    SelectObject32 (hdcImage, hbmImage);

    BitBlt32 (hdcImageList, himl->cCurImage * himl->cx, 0,
              bmp.bmWidth, himl->cy, hdcImage, 0, 0, SRCCOPY);
          
    if (himl->hbmMask)
    {
        if (hbmMask)
        {
            SelectObject32 (hdcImageList, himl->hbmMask);
            SelectObject32 (hdcImage, hbmMask);
            BitBlt32 (hdcImageList, himl->cCurImage * himl->cx, 0,
                      bmp.bmWidth, himl->cy, hdcImage, 0, 0, SRCCOPY);

            /* fix transparent areas of the image bitmap*/
            SelectObject32 (hdcMask, himl->hbmMask);
            SelectObject32 (hdcImage, himl->hbmImage);
            nStartX = himl->cCurImage * himl->cx;
            for (nRunY = 0; nRunY < himl->cy; nRunY++)
            {
                for (nRunX = 0; nRunX < bmp.bmWidth; nRunX++)
                {
                    if (GetPixel32 (hdcMask, nStartX + nRunX, nRunY) ==
                        RGB(255, 255, 255))
                        SetPixel32 (hdcImage, nStartX + nRunX, nRunY, 
                                    RGB(0, 0, 0));
                }
            }
        }
        else
        {
            /* create mask from the imagelist's background color */
            hdcMask = CreateCompatibleDC32 (0);
            SelectObject32 (hdcMask, himl->hbmMask);
            nStartX = himl->cCurImage * himl->cx;
            for (nRunY = 0; nRunY < himl->cy; nRunY++)
            {
                for (nRunX = 0; nRunX < bmp.bmWidth; nRunX++)
                {
                    if (GetPixel32 (hdcImageList, nStartX + nRunX, nRunY) ==
                        himl->clrBk)
                    {
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


/*************************************************************************
 *	 		 ImageList_AddMasked   [COMCTL32.41]
 *
 *  Adds an image to an imagelist and creates a mask from the given
 *  mask color.
 *
 *  RETURNS
 *    Index of the first image that was added.
 *    -1 if an error occurred.
 */

INT32 WINAPI ImageList_AddMasked (
	HIMAGELIST himl,    /* image list handle */
	HBITMAP32 hbmImage, /* bitmap handle */
	COLORREF clrMask)   /* backround color of the image */
{
    HDC32    hdcImageList, hdcImage, hdcMask;
    INT32    nIndex, nImageCount;
    BITMAP32 bmp;
    INT32    nStartX, nRunX, nRunY;

    if (himl == NULL) return (-1);

    GetObject32A (hbmImage, sizeof(BITMAP32), &bmp);
    nImageCount = bmp.bmWidth / himl->cx;

    if (himl->cCurImage + nImageCount >= himl->cMaxImage)
        IMAGELIST_InternalGrowBitmaps (himl, nImageCount);

    nIndex = himl->cCurImage;
    himl->cCurImage += nImageCount;

    hdcImageList = CreateCompatibleDC32 (0);
    hdcImage = CreateCompatibleDC32 (0);

    SelectObject32 (hdcImageList, himl->hbmImage);
    SelectObject32 (hdcImage, hbmImage);
    BitBlt32 (hdcImageList, nIndex * himl->cx, 0, bmp.bmWidth, himl->cy,
              hdcImage, 0, 0, SRCCOPY);

    if (himl->hbmMask)
    {
        /* create Mask */
        hdcMask = CreateCompatibleDC32 (0);
        SelectObject32 (hdcMask, himl->hbmMask);
        nStartX = nIndex * himl->cx;
        for (nRunY = 0; nRunY < himl->cy; nRunY++)
        {
            for (nRunX = 0; nRunX < bmp.bmWidth; nRunX++)
            {
                if (GetPixel32 (hdcImageList, nStartX + nRunX, nRunY) ==
                    clrMask)
                {
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
 *	 		 ImageList_BeginDrag   [COMCTL32.42]
 *
 *  Creates a temporary imagelist with an image in it, which will be used
 *  as a drag image.
 *
 *  RETURNS
 *    ...
 */

BOOL32 WINAPI ImageList_BeginDrag (
	HIMAGELIST himlTrack, /* Handle of the source imagelist */
	INT32 iTrack,         /* Index of the image in the source imagelist */
	INT32 dxHotspot,      /* Position of the hot spot of the */
	INT32 dyHotspot)      /* drag image */
{
    HDC32 hdcSrc, hdcDst;

    FIXME(imagelist, "ImageList_BeginDrag: partially implemented!\n");

    if (himlTrack == NULL) return (FALSE);
    if (himlInternalDrag)
        ImageList_EndDrag ();

    himlInternalDrag = ImageList_Create (himlTrack->cx, himlTrack->cy,
                                         himlTrack->flags, 1, 1);
    if (himlInternalDrag == NULL)
    {
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
 *	 		 ImageList_Copy   [COMCTL32.43]
 *
 *  Copies an image of the source imagelist to an image of the 
 *  destination imagelist. Images can be copied or swapped.
 *  Copying from one imagelist to another is allowed, in contrary to
 *  M$'s original implementation. They just allow copying or swapping
 *  within one imagelist (himlDst and himlSrc must be the same).
 *
 *  RETURNS
 *    ...
 */

BOOL32 WINAPI ImageList_Copy (
	HIMAGELIST himlDst,  /* Handle of the destination imagelist */
	INT32 iDst,          /* Index of the destination image */
	HIMAGELIST himlSrc,  /* Handel od the source imagelist */
	INT32 iSrc,          /* Index of the source image */
	INT32 uFlags)        /* Flags used for the copy operation */
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

    if (uFlags & ILCF_SWAP)
    {
        /* swap */
        HBITMAP32 hbmTempImage, hbmTempMask;

        /* create temporary bitmaps */
        hbmTempImage = CreateBitmap32 (himlSrc->cx, himlSrc->cy, 1,
                                       himlSrc->uBitsPixel, NULL);
        hbmTempMask = CreateBitmap32 (himlSrc->cx, himlSrc->cy, 1, 1, NULL);

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
    else
    {
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
 *	 		 ImageList_Create   [COMCTL32.44]
 *
 *  Creates an imagelist of the given image size and number of images.
 *
 *  RETURNS
 *    Handle of the created image list.
 *    0 if an error occurred.
 */

HIMAGELIST WINAPI ImageList_Create (
	INT32 cx,        /* Width of an image */
	INT32 cy,        /* Height of an image */
	UINT32 flags,    /* Flags for imagelist creation */
	INT32 cInitial,  /* Initial number of images in the imaglist */
	INT32 cGrow)     /* Number of images that is added to the */
	                 /* imagelist when it grows */
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
    if (himl->hbmImage == 0)
    {
        ERR(imagelist, "Error creating image bitmap!\n");
        return (0);
    }

    if (himl->flags & ILC_MASK)
    {
        himl->hbmMask = 
            CreateBitmap32 (himl->cx * himl->cMaxImage, himl->cy, 1, 1, NULL);
        if (himl->hbmMask == 0)
        {
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
 *	 		 ImageList_Destroy   [COMCTL32.45]
 *
 *  Destroy the given imagelist.
 *
 *  RETURNS
 *    TRUE if the image list was destroyed.
 *    FALSE if an error occurred.
 */

BOOL32 WINAPI ImageList_Destroy (
	HIMAGELIST himl)  /* Handle of the imagelist */
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
 *	 		 ImageList_DragEnter   [COMCTL32.46]
 *
 *  FIXME
 *    This is still an empty stub.
 */

BOOL32 WINAPI ImageList_DragEnter (
	HWND32 hwndLock, 
	INT32 x, 
	INT32 y)
{
    FIXME (imagelist, "empty stub!\n");

    hcurInternal = GetCursor32 ();


    ShowCursor32 (TRUE);

    return (FALSE);
}


/*************************************************************************
 *	 		 ImageList_DragLeave   [COMCTL32.47]
 */

BOOL32 WINAPI ImageList_DragLeave (
	HWND32 hwndLock)
{
    FIXME (imagelist, "empty stub!\n");


    SetCursor32 (hcurInternal);
    hcurInternal = 0;
 
    ShowCursor32 (FALSE);

    return (FALSE);
}


/*************************************************************************
 *	 		 ImageList_DragMove   [COMCTL32.48]
 */

BOOL32 WINAPI ImageList_DragMove (
	INT32 x,
	INT32 y)
{
    FIXME (imagelist, "empty stub!\n");

//    if (hcurInternal)
//        SetCursor32 (hcurInternal);
//    ImageList_Draw (himlInternalDrag, 0, x, y, 0);


    return (FALSE);
}


/*************************************************************************
 *	 		 ImageList_DragShowNolock   [COMCTL32.49]
 */

BOOL32 WINAPI
ImageList_DragShowNolock (BOOL32 bShow)
{
    FIXME (imagelist, "empty stub!\n");

    return (FALSE);
}


/*************************************************************************
 *	 		 ImageList_Draw   [COMCTL32.50]
 */

BOOL32 WINAPI ImageList_Draw (
	HIMAGELIST himl,
	INT32 i,
	HDC32 hdc, 
	INT32 x,
	INT32 y,
	UINT32 fStyle)
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
 *	 		 ImageList_DrawEx   [COMCTL32.51]
 */

BOOL32 WINAPI ImageList_DrawEx (
	HIMAGELIST himl,
	INT32 i,
	HDC32 hdc,
	INT32 x,
	INT32 y,
	INT32 xOffs,
	INT32 yOffs,
	COLORREF rgbBk,
	COLORREF rgbFg,
	UINT32 fStyle)
{
    IMAGELISTDRAWPARAMS imldp;

    imldp.cbSize  = sizeof(IMAGELISTDRAWPARAMS);
    imldp.himl    = himl;
    imldp.i       = i;
    imldp.hdcDst  = hdc,
    imldp.x       = x;
    imldp.y       = y;
    imldp.cx      = xOffs;
    imldp.cy      = yOffs;
    imldp.xBitmap = 0;
    imldp.yBitmap = 0;
    imldp.rgbBk   = rgbBk;
    imldp.rgbFg   = rgbFg;
    imldp.fStyle  = fStyle;
    imldp.dwRop   = 0;

    return (ImageList_DrawIndirect (&imldp));
}


/*************************************************************************
 *	 		 ImageList_DrawIndirect   [COMCTL32.52]
 */

BOOL32 WINAPI ImageList_DrawIndirect (
	IMAGELISTDRAWPARAMS *pimldp)
{
    HIMAGELIST himlLocal;
    HDC32      hdcImageList, hdcTempImage;
    HBITMAP32  hbmTempImage;
    HBRUSH32   hBrush, hOldBrush;
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

    himlLocal = pimldp->himl;
    
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
        
        BitBlt32 (pimldp->hdcDst, pimldp->x, pimldp->y,
                  himlLocal->cx, himlLocal->cy, hdcImageList,
                  himlLocal->cx * pimldp->i, 0,
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
                      himlLocal->cx, himlLocal->cy, PATCOPY);
            DeleteObject32 (SelectObject32 (pimldp->hdcDst, hOldBrush));
        }

        BitBlt32 (pimldp->hdcDst, pimldp->x, pimldp->y, himlLocal->cx,
                  himlLocal->cy, hdcImageList, himlLocal->cx * pimldp->i, 0,
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

            BitBlt32 (pimldp->hdcDst, pimldp->x, pimldp->y, himlLocal->cx,
                      himlLocal->cy, hdcTempImage, 0, 0, SRCAND);

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

            BitBlt32 (pimldp->hdcDst, pimldp->x, pimldp->y, himlLocal->cx,
                      himlLocal->cy, hdcTempImage, 0, 0, SRCPAINT);

            DeleteObject32 (hbmTempImage);
            DeleteDC32 (hdcTempImage);
        }
    }   

    /* Draw overlay image */
    if (pimldp->fStyle & 0x0700)
    {
        nOvlIdx = (pimldp->fStyle & 0x0700) >> 8;
        if ((nOvlIdx >= 1) && (nOvlIdx <= MAX_OVERLAYIMAGE))
        {
            nOvlIdx = pimldp->himl->nOvlIdx[nOvlIdx - 1];
            if ((nOvlIdx >= 0) && (nOvlIdx <= pimldp->himl->cCurImage))
            {
                if (pimldp->himl->hbmMask)
                {  
                    SelectObject32 (hdcImageList, pimldp->himl->hbmMask);
                    BitBlt32 (pimldp->hdcDst, pimldp->x, pimldp->y, 
                              pimldp->himl->cx, pimldp->himl->cy, hdcImageList,
                              pimldp->himl->cx * nOvlIdx, 0, SRCAND);  
                }  
                SelectObject32 (hdcImageList, pimldp->himl->hbmImage);
                BitBlt32 (pimldp->hdcDst, pimldp->x, pimldp->y, 
                          pimldp->himl->cx, pimldp->himl->cy, hdcImageList,
                          pimldp->himl->cx * nOvlIdx, 0, SRCPAINT);
            }
        }
    }

    DeleteDC32 (hdcImageList);
  
    return (TRUE);
}


/*************************************************************************
 *	 		 ImageList_Duplicate   [COMCTL32.53]
 *
 *  Duplicates an image list.
 *
 *  RETURNS
 *    Handle of duplicate image list, 0 if an error occurred.
 */

HIMAGELIST WINAPI ImageList_Duplicate (
	HIMAGELIST himlSrc)
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
 *	 		 ImageList_EndDrag   [COMCTL32.54]
 *
 *  Finishes a drag operation.
 *
 *  FIXME
 *    semi-stub.
 */

BOOL32 WINAPI ImageList_EndDrag (VOID)
{
    FIXME (imagelist, "partially implemented!\n");

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
 *	 		 ImageList_GetBkColor   [COMCTL32.55]
 *
 *  Returns the background color of an image list.
 *
 *  RETURNS
 *    Background color.
 */

COLORREF WINAPI ImageList_GetBkColor (
	HIMAGELIST himl)
{
    return (himl->clrBk);
}


/*************************************************************************
 *	 		 ImageList_GetDragImage   [COMCTL32.56]
 *
 *  Returns the handle to the internal drag image list.
 *
 *  FIXME
 *    semi-stub.
 */

HIMAGELIST WINAPI ImageList_GetDragImage (
	POINT32 *ppt,
	POINT32 *pptHotspot)
{
    FIXME (imagelist, "partially imlemented!\n");

    if (himlInternalDrag)
        return (himlInternalDrag);

    return (NULL);
}


/*************************************************************************
 *	 		 ImageList_GetIcon   [COMCTL32.57]
 */

HICON32 WINAPI ImageList_GetIcon (
	HIMAGELIST himl, 
	INT32 i, 
	UINT32 fStyle)
{
    ICONINFO ii;
    HICON32  hIcon;
    HDC32    hdc;
    INT32    nWidth, nHeight;

    nWidth = GetSystemMetrics32 (SM_CXICON);
    nHeight = GetSystemMetrics32 (SM_CYICON);

    ii.fIcon = TRUE;
    ii.xHotspot = 0;
    ii.yHotspot = 0;
    ii.hbmMask  = CreateBitmap32 (nWidth, nHeight, 1, 1, NULL);
    ii.hbmColor = CreateBitmap32 (nWidth, nHeight, 1, 1, NULL);

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
 *	 		 ImageList_GetIconSize   [COMCTL32.58]
 */

BOOL32 WINAPI ImageList_GetIconSize (
	HIMAGELIST himl,
	INT32 *cx,
	INT32 *cy)
{

    if (himl == NULL) return (FALSE);

    if (cx)
        *cx = himl->cx;
    
    if (cy)
        *cy = himl->cy;
    return (TRUE);
}


/*************************************************************************
 *	 		 ImageList_GetIconCount   [COMCTL32.59]
 */

INT32 WINAPI ImageList_GetImageCount (
	HIMAGELIST himl)
{
    return (himl->cCurImage);
}


/*************************************************************************
 *	 		 ImageList_GetImageInfo   [COMCTL32.60]
 */

BOOL32 WINAPI ImageList_GetImageInfo (
	HIMAGELIST himl,
	INT32 i,
	IMAGEINFO *pImageInfo)
{
    if ((himl == NULL) || (pImageInfo == NULL)) return (FALSE);

    pImageInfo->hbmImage = himl->hbmImage;
    pImageInfo->hbmMask  = himl->hbmMask;
    
    pImageInfo->rcImage.top    = 0;
    pImageInfo->rcImage.bottom = himl->cy;
    pImageInfo->rcImage.left   = i * himl->cx;
    pImageInfo->rcImage.right  = (i+1) * himl->cx;
    
    return (TRUE);
}


/*************************************************************************
 *	 		 ImageList_GetImageRect   [COMCTL32.61]
 *
 *  COMMENTS
 *    I don't know if it really returns a BOOL32 or something else!!!??
 */

BOOL32 WINAPI ImageList_GetImageRect (
	HIMAGELIST himl,
	INT32 i,
	LPRECT32 lpRect)
{
    if (himl == NULL) return (FALSE);
    if ((i < 0) || (i >= himl->cCurImage)) return (FALSE);
    if (lpRect == NULL) return (FALSE);

    lpRect->left = i * himl->cx;
    lpRect->top = 0;
    lpRect->right = lpRect->left + himl->cx;
    lpRect->bottom = himl->cy;

    return (TRUE);
}


/*************************************************************************
 *	 		 ImageList_LoadImage32A   [COMCTL32.63]
 */

HIMAGELIST WINAPI ImageList_LoadImage32A (
	HINSTANCE32 hi,
	LPCSTR lpbmp,
	INT32 cx,
	INT32 cGrow, 
	COLORREF clrMask,
	UINT32 uType,
	UINT32 uFlags)
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


/*************************************************************************
 *	 		 ImageList_LoadImage32W   [COMCTL32.64]
 */

HIMAGELIST WINAPI ImageList_LoadImage32W (
	HINSTANCE32 hi,
	LPCWSTR lpbmp,
	INT32 cx,
	INT32 cGrow,
	COLORREF clrMask,
	UINT32 uType,
	UINT32 uFlags)
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


/*************************************************************************
 *	 		 ImageList_Merge   [COMCTL32.65]
 */

HIMAGELIST WINAPI ImageList_Merge (
	HIMAGELIST himl1,
	INT32 i1,
	HIMAGELIST himl2,
	INT32 i2,
	INT32 xOffs,
	INT32 yOffs)
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

    if (xOffs > 0) {
        cxDst = _MAX (himl1->cx, xOffs + himl2->cx);
        xOff1 = 0;
        xOff2 = xOffs;
    }
    else if (xOffs < 0) {
        cxDst = _MAX (himl2->cx, himl1->cx - xOffs);
        xOff1 = -xOffs;
        xOff2 = 0;
    }
    else {
        cxDst = 0;
        xOff1 = 0;
        xOff2 = 0;
    }

    if (yOffs > 0) {
        cyDst = _MAX (himl1->cy, yOffs + himl2->cy);
        yOff1 = 0;
        yOff2 = yOffs;
    }
    else if (yOffs < 0) {
        cyDst = _MAX (himl2->cy, himl1->cy - yOffs);
        yOff1 = -yOffs;
        yOff2 = 0;
    }
    else {
        cyDst = 0;
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


#if 0
#if __IStream_INTERFACE_DEFINED__
HIMAGELIST WINAPI ImageList_Read (
	LPSTREAM pstm)
{
    FIXME (imagelist, "empty stub!\n");


    return (NULL);
}
#endif /* __IStream_INTERFACE_DEFINED__ */
#endif /* 0 */


BOOL32 WINAPI ImageList_Remove (
	HIMAGELIST himl,
	INT32 i)
{
    HBITMAP32 hbmNewImage, hbmNewMask;
    HDC32     hdcSrc, hdcDst;
    INT32     cxNew, nCount;

    if ((i < -1) || (i >= himl->cCurImage)) {
        ERR (imagelist, "Index out of range! %d\n", i);
        return (FALSE);
    }

    if (himl->cCurImage == 0) {
        ERR (imagelist, "List is already empty!\n");
        return (FALSE);
    }

    if (i == -1) {
        /* remove all */
        TRACE (imagelist, "Remove all!\n");

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


BOOL32 WINAPI ImageList_Replace (
	HIMAGELIST himl,
	INT32 i,
	HBITMAP32 hbmImage, 
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


INT32 WINAPI ImageList_ReplaceIcon (
	HIMAGELIST himl,
	INT32 i,
	HICON32 hIcon)
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

    if (himl == NULL) return (-1);
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
            IMAGELIST_InternalGrowBitmaps (himl, 1);

        nIndex = himl->cCurImage;
        himl->cCurImage++;
    }
    else
        nIndex = i;

    hdcImageList = CreateCompatibleDC32 (0);
    hdcImage = CreateCompatibleDC32 (0);

#ifdef __GET_ICON_INFO_HACK__
    SelectObject32 (hdcImageList, himl->hbmImage);
    SelectObject32 (hdcImage, hbmImage);
    StretchBlt32 (hdcImageList, nIndex * himl->cx, 0, himl->cx, himl->cy,
                  hdcImage, 0, 0, ptr->nWidth, ptr->nHeight, SRCCOPY);
#else
    SelectObject32 (hdcImage, ii->hbmColor);
    StretchBlt32 (hdcImageList, nIndex * himl->cx, 0, himl->cx, himl->cy,
                  hdcImage, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);
#endif

    if (himl->hbmMask) {
#ifdef __GET_ICON_INFO_HACK__
        SelectObject32 (hdcImageList, himl->hbmMask);
        SelectObject32 (hdcImage, hbmMask);
        StretchBlt32 (hdcImageList, nIndex * himl->cx, 0, himl->cx, himl->cy,
                      hdcImage, 0, 0, ptr->nWidth, ptr->nHeight, SRCCOPY);
#else
        SelectObject32 (hdcImage, ii->hbmMask);
        StretchBlt32 (hdcImageList, nIndex * himl->cx, 0, himl->cx, himl->cy,
                      hdcImage, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);
#endif
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


COLORREF WINAPI ImageList_SetBkColor (
	HIMAGELIST himl,
	COLORREF clrBk)
{
    COLORREF clrOldBk;

    clrOldBk = himl->clrBk;
    himl->clrBk = clrBk;
    return (clrOldBk);
}


BOOL32 WINAPI ImageList_SetDragCursorImage (
	HIMAGELIST himlDrag,
	INT32 iDrag,
	INT32 dxHotspot,
	INT32 dyHotspot)
{
    FIXME (imagelist, "empty stub!\n");

    return (FALSE);
}


#if 0
BOOL32 WINAPI ImageList_SetFilter (
	HIMAGELIST himl, 
	INT32 iIndex, 
	INT32 iFilter)
{
    FIXME (imagelist, "empty stub!\n");


}
#endif /* 0 */


BOOL32 WINAPI ImageList_SetIconSize (
	HIMAGELIST himl, 
	INT32 cx, 
	INT32 cy)
{
    INT32 nCount;

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


BOOL32 WINAPI ImageList_SetImageCount (
	HIMAGELIST himl, 
	INT32 iImageCount)
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


BOOL32 WINAPI ImageList_SetOverlayImage (
	HIMAGELIST himl, 
	INT32 iImage, 
	INT32 iOverlay)
{
    if ((iOverlay < 1) || (iOverlay > MAX_OVERLAYIMAGE)) return (FALSE);
    if ((iImage < 0) || (iImage > himl->cCurImage)) return (FALSE);
    
    himl->nOvlIdx[iOverlay - 1] = iImage;
    return (TRUE);
}


#if 0
#if __IStream_INTERFACE_DEFINED__
BOOL32 WINAPI ImageList_Write (
	HIMAGELIST himl, 
	LPSTREAM pstm)
{
    FIXME (imagelist, "empty stub!\n");


    return (FALSE);
}
#endif  /* __IStream_INTERFACE_DEFINED__ */
#endif  /* 0 */
