/*
 * Graphics paths (BeginPath, EndPath etc.)
 *
 * Copyright 1997, 1998 Martin Boehme
 */

#include <assert.h>
#include <malloc.h>
#include <math.h>

#include "windows.h"
#include "winerror.h"

#include "dc.h"
#include "debug.h"
#include "path.h"

/* Notes on the implementation
 *
 * The implementation is based on dynamically resizable arrays of points and
 * flags. I dithered for a bit before deciding on this implementation, and
 * I had even done a bit of work on a linked list version before switching
 * to arrays. It's a bit of a tradeoff. When you use linked lists, the
 * implementation of FlattenPath is easier, because you can rip the
 * PT_BEZIERTO entries out of the middle of the list and link the
 * corresponding PT_LINETO entries in. However, when you use arrays,
 * PathToRegion becomes easier, since you can essentially just pass your array
 * of points to CreatePolyPolygonRgn. Also, if I'd used linked lists, I would
 * have had the extra effort of creating a chunk-based allocation scheme
 * in order to use memory effectively. That's why I finally decided to use
 * arrays. Note by the way that the array based implementation has the same
 * linear time complexity that linked lists would have since the arrays grow
 * exponentially.
 *
 * The points are stored in the path in device coordinates. This is
 * consistent with the way Windows does things (for instance, see the Win32
 * SDK documentation for GetPath).
 *
 * The word "stroke" appears in several places (e.g. in the flag
 * GdiPath.newStroke). A stroke consists of a PT_MOVETO followed by one or
 * more PT_LINETOs or PT_BEZIERTOs, up to, but not including, the next
 * PT_MOVETO. Note that this is not the same as the definition of a figure;
 * a figure can contain several strokes.
 *
 * I modified the drawing functions (MoveTo, LineTo etc.) to test whether
 * the path is open and to call the corresponding function in path.c if this
 * is the case. A more elegant approach would be to modify the function
 * pointers in the DC_FUNCTIONS structure; however, this would be a lot more
 * complex. Also, the performance degradation caused by my approach in the
 * case where no path is open is so small that it cannot be measured.
 *
 * Martin Boehme
 */

/* FIXME: A lot of stuff isn't implemented yet. There is much more to come. */

#define NUM_ENTRIES_INITIAL 16  /* Initial size of points / flags arrays  */
#define GROW_FACTOR_NUMER    2  /* Numerator of grow factor for the array */
#define GROW_FACTOR_DENOM    1  /* Denominator of grow factor             */


static BOOL32 PATH_PathToRegion(const GdiPath *pPath, INT32 nPolyFillMode,
   HRGN32 *pHrgn);
static void   PATH_EmptyPath(GdiPath *pPath);
static BOOL32 PATH_AddEntry(GdiPath *pPath, const POINT32 *pPoint,
   BYTE flags);
static BOOL32 PATH_ReserveEntries(GdiPath *pPath, INT32 numEntries);
static BOOL32 PATH_GetPathFromHDC(HDC32 hdc, GdiPath **ppPath);
static BOOL32 PATH_DoArcPart(GdiPath *pPath, FLOAT_POINT corners[],
   double angleStart, double angleEnd, BOOL32 addMoveTo);
static void PATH_ScaleNormalizedPoint(FLOAT_POINT corners[], double x,
   double y, POINT32 *pPoint);
static void PATH_NormalizePoint(FLOAT_POINT corners[], const FLOAT_POINT
   *pPoint, double *pX, double *pY);


/***********************************************************************
 *           BeginPath16    (GDI.512)
 */
BOOL16 WINAPI BeginPath16(HDC16 hdc)
{
   return (BOOL16)BeginPath32((HDC32)hdc);
}


/***********************************************************************
 *           BeginPath32    (GDI32.9)
 */
BOOL32 WINAPI BeginPath32(HDC32 hdc)
{
   GdiPath *pPath;
   
   /* Get pointer to path */
   if(!PATH_GetPathFromHDC(hdc, &pPath))
   {
      SetLastError(ERROR_INVALID_HANDLE);
      return FALSE;
   }
   
   /* If path is already open, do nothing */
   if(pPath->state==PATH_Open)
      return TRUE;

   /* Make sure that path is empty */
   PATH_EmptyPath(pPath);

   /* Initialize variables for new path */
   pPath->newStroke=TRUE;
   pPath->state=PATH_Open;
   
   return TRUE;
}


/***********************************************************************
 *           EndPath16    (GDI.514)
 */
BOOL16 WINAPI EndPath16(HDC16 hdc)
{
   return (BOOL16)EndPath32((HDC32)hdc);
}


/***********************************************************************
 *           EndPath32    (GDI32.78)
 */
BOOL32 WINAPI EndPath32(HDC32 hdc)
{
   GdiPath *pPath;
   
   /* Get pointer to path */
   if(!PATH_GetPathFromHDC(hdc, &pPath))
   {
      SetLastError(ERROR_INVALID_HANDLE);
      return FALSE;
   }
   
   /* Check that path is currently being constructed */
   if(pPath->state!=PATH_Open)
   {
      SetLastError(ERROR_CAN_NOT_COMPLETE);
      return FALSE;
   }
   
   /* Set flag to indicate that path is finished */
   pPath->state=PATH_Closed;
   
   return TRUE;
}


/***********************************************************************
 *           AbortPath16    (GDI.511)
 */
BOOL16 WINAPI AbortPath16(HDC16 hdc)
{
   return (BOOL16)AbortPath32((HDC32)hdc);
}


/******************************************************************************
 * AbortPath32 [GDI32.1]
 * Closes and discards paths from device context
 *
 * NOTES
 *    Check that SetLastError is being called correctly
 *
 * PARAMS
 *    hdc [I] Handle to device context
 *
 * RETURNS STD
 */
BOOL32 WINAPI AbortPath32( HDC32 hdc )
{
   GdiPath *pPath;
   
   /* Get pointer to path */
   if(!PATH_GetPathFromHDC(hdc, &pPath))
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
   }
   
   /* Remove all entries from the path */
   PATH_EmptyPath(pPath);

   return TRUE;
}


/***********************************************************************
 *           CloseFigure16    (GDI.513)
 */
BOOL16 WINAPI CloseFigure16(HDC16 hdc)
{
   return (BOOL16)CloseFigure32((HDC32)hdc);
}


/***********************************************************************
 *           CloseFigure32    (GDI32.16)
 *
 * FIXME: Check that SetLastError is being called correctly 
 */
BOOL32 WINAPI CloseFigure32(HDC32 hdc)
{
   GdiPath *pPath;
   
   /* Get pointer to path */
   if(!PATH_GetPathFromHDC(hdc, &pPath))
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
   }
   
   /* Check that path is open */
   if(pPath->state!=PATH_Open)
   {
      SetLastError(ERROR_CAN_NOT_COMPLETE);
      return FALSE;
   }
   
   /* FIXME: Shouldn't we draw a line to the beginning of the figure? */
   
   /* Set PT_CLOSEFIGURE on the last entry and start a new stroke */
   if(pPath->numEntriesUsed)
   {
      pPath->pFlags[pPath->numEntriesUsed-1]|=PT_CLOSEFIGURE;
      pPath->newStroke=TRUE;
   }

   return TRUE;
}


/***********************************************************************
 *           GetPath16    (GDI.517)
 */
INT16 WINAPI GetPath16(HDC16 hdc, LPPOINT16 pPoints, LPBYTE pTypes,
   INT16 nSize)
{
   FIXME(gdi, "(%d,%p,%p): stub\n",hdc,pPoints,pTypes);

   return 0;
}


/***********************************************************************
 *           GetPath32    (GDI32.210)
 */
INT32 WINAPI GetPath32(HDC32 hdc, LPPOINT32 pPoints, LPBYTE pTypes,
   INT32 nSize)
{
   GdiPath *pPath;
   
   /* Get pointer to path */
   if(!PATH_GetPathFromHDC(hdc, &pPath))
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return -1;
   }
   
   /* Check that path is closed */
   if(pPath->state!=PATH_Closed)
   {
      SetLastError(ERROR_CAN_NOT_COMPLETE);
      return -1;
   }
   
   if(nSize==0)
      return pPath->numEntriesUsed;
   else if(nSize<pPath->numEntriesUsed)
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return -1;
   }
   else
   {
      memcpy(pPoints, pPath->pPoints, sizeof(POINT32)*pPath->numEntriesUsed);
      memcpy(pTypes, pPath->pFlags, sizeof(BYTE)*pPath->numEntriesUsed);

      /* Convert the points to logical coordinates */
      if(!DPtoLP32(hdc, pPoints, pPath->numEntriesUsed))
      {
	 /* FIXME: Is this the correct value? */
         SetLastError(ERROR_CAN_NOT_COMPLETE);
         return -1;
      }
      
      return pPath->numEntriesUsed;
   }
}


/***********************************************************************
 *           PathToRegion32    (GDI32.261)
 *
 * FIXME 
 *   Check that SetLastError is being called correctly 
 *
 * The documentation does not state this explicitly, but a test under Windows
 * shows that the region which is returned should be in device coordinates.
 */
HRGN32 WINAPI PathToRegion32(HDC32 hdc)
{
   GdiPath *pPath;
   HRGN32  hrgnRval;

   /* Get pointer to path */
   if(!PATH_GetPathFromHDC(hdc, &pPath))
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return 0;
   }
   
   /* Check that path is closed */
   if(pPath->state!=PATH_Closed)
   {
      SetLastError(ERROR_CAN_NOT_COMPLETE);
      return 0;
   }
   
   /* FIXME: Should we empty the path even if conversion failed? */
   if(PATH_PathToRegion(pPath, GetPolyFillMode32(hdc), &hrgnRval))
      PATH_EmptyPath(pPath);
   else
      hrgnRval=0;

   return hrgnRval;
}


/***********************************************************************
 *           FillPath32    (GDI32.100)
 *
 * FIXME
 *    Check that SetLastError is being called correctly 
 */
BOOL32 WINAPI FillPath32(HDC32 hdc)
{
   GdiPath *pPath;
   INT32   mapMode, graphicsMode;
   POINT32 ptViewportExt, ptViewportOrg, ptWindowExt, ptWindowOrg;
   XFORM   xform;
   HRGN32  hrgn;
   
   /* Get pointer to path */
   if(!PATH_GetPathFromHDC(hdc, &pPath))
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
   }
   
   /* Check that path is closed */
   if(pPath->state!=PATH_Closed)
   {
      SetLastError(ERROR_CAN_NOT_COMPLETE);
      return FALSE;
   }
   
   /* Construct a region from the path and fill it */
   if(PATH_PathToRegion(pPath, GetPolyFillMode32(hdc), &hrgn))
   {
      /* Since PaintRgn interprets the region as being in logical coordinates
       * but the points we store for the path are already in device
       * coordinates, we have to set the mapping mode to MM_TEXT temporarily.
       * Using SaveDC to save information about the mapping mode / world
       * transform would be easier but would require more overhead, especially
       * now that SaveDC saves the current path.
       */
       
      /* Save the information about the old mapping mode */
      mapMode=GetMapMode32(hdc);
      GetViewportExtEx32(hdc, &ptViewportExt);
      GetViewportOrgEx32(hdc, &ptViewportOrg);
      GetWindowExtEx32(hdc, &ptWindowExt);
      GetWindowOrgEx32(hdc, &ptWindowOrg);
      
      /* Save world transform
       * NB: The Windows documentation on world transforms would lead one to
       * believe that this has to be done only in GM_ADVANCED; however, my
       * tests show that resetting the graphics mode to GM_COMPATIBLE does
       * not reset the world transform.
       */
      GetWorldTransform(hdc, &xform);
      
      /* Set MM_TEXT */
      SetMapMode32(hdc, MM_TEXT);
      
      /* Paint the region */
      PaintRgn32(hdc, hrgn);

      /* Restore the old mapping mode */
      SetMapMode32(hdc, mapMode);
      SetViewportExtEx32(hdc, ptViewportExt.x, ptViewportExt.y, NULL);
      SetViewportOrgEx32(hdc, ptViewportOrg.x, ptViewportOrg.y, NULL);
      SetWindowExtEx32(hdc, ptWindowExt.x, ptWindowExt.y, NULL);
      SetWindowOrgEx32(hdc, ptWindowOrg.x, ptWindowOrg.y, NULL);

      /* Go to GM_ADVANCED temporarily to restore the world transform */
      graphicsMode=GetGraphicsMode(hdc);
      SetGraphicsMode(hdc, GM_ADVANCED);
      SetWorldTransform(hdc, &xform);
      SetGraphicsMode(hdc, graphicsMode);

      /* Empty the path */
      PATH_EmptyPath(pPath);
      return TRUE;
   }
   else
   {
      /* FIXME: Should the path be emptied even if conversion failed? */
      /* PATH_EmptyPath(pPath); */
      return FALSE;
   }
}


/***********************************************************************
 *           SelectClipPath32    (GDI32.296)
 * FIXME 
 *  Check that SetLastError is being called correctly 
 */
BOOL32 WINAPI SelectClipPath32(HDC32 hdc, INT32 iMode)
{
   GdiPath *pPath;
   HRGN32  hrgnPath;
   BOOL32  success;
   
   /* Get pointer to path */
   if(!PATH_GetPathFromHDC(hdc, &pPath))
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
   }
   
   /* Check that path is closed */
   if(pPath->state!=PATH_Closed)
   {
      SetLastError(ERROR_CAN_NOT_COMPLETE);
      return FALSE;
   }
   
   /* Construct a region from the path */
   if(PATH_PathToRegion(pPath, GetPolyFillMode32(hdc), &hrgnPath))
   {
      success = ExtSelectClipRgn( hdc, hrgnPath, iMode ) != ERROR;
      DeleteObject32(hrgnPath);

      /* Empty the path */
      if(success)
         PATH_EmptyPath(pPath);
      /* FIXME: Should this function delete the path even if it failed? */

      return success;
   }
   else
      return FALSE;
}


/***********************************************************************
 * Exported functions
 */

/* PATH_InitGdiPath
 *
 * Initializes the GdiPath structure.
 */
void PATH_InitGdiPath(GdiPath *pPath)
{
   assert(pPath!=NULL);

   pPath->state=PATH_Null;
   pPath->pPoints=NULL;
   pPath->pFlags=NULL;
   pPath->numEntriesUsed=0;
   pPath->numEntriesAllocated=0;
}

/* PATH_DestroyGdiPath
 *
 * Destroys a GdiPath structure (frees the memory in the arrays).
 */
void PATH_DestroyGdiPath(GdiPath *pPath)
{
   assert(pPath!=NULL);

   free(pPath->pPoints);
   free(pPath->pFlags);
}

/* PATH_AssignGdiPath
 *
 * Copies the GdiPath structure "pPathSrc" to "pPathDest". A deep copy is
 * performed, i.e. the contents of the pPoints and pFlags arrays are copied,
 * not just the pointers. Since this means that the arrays in pPathDest may
 * need to be resized, pPathDest should have been initialized using
 * PATH_InitGdiPath (in C++, this function would be an assignment operator,
 * not a copy constructor).
 * Returns TRUE if successful, else FALSE.
 */
BOOL32 PATH_AssignGdiPath(GdiPath *pPathDest, const GdiPath *pPathSrc)
{
   assert(pPathDest!=NULL && pPathSrc!=NULL);

   /* Make sure destination arrays are big enough */
   if(!PATH_ReserveEntries(pPathDest, pPathSrc->numEntriesUsed))
      return FALSE;

   /* Perform the copy operation */
   memcpy(pPathDest->pPoints, pPathSrc->pPoints,
      sizeof(POINT32)*pPathSrc->numEntriesUsed);
   memcpy(pPathDest->pFlags, pPathSrc->pFlags,
      sizeof(INT32)*pPathSrc->numEntriesUsed);
   pPathDest->state=pPathSrc->state;
   pPathDest->numEntriesUsed=pPathSrc->numEntriesUsed;
   pPathDest->newStroke=pPathSrc->newStroke;

   return TRUE;
}

/* PATH_MoveTo
 *
 * Should be called when a MoveTo is performed on a DC that has an
 * open path. This starts a new stroke. Returns TRUE if successful, else
 * FALSE.
 */
BOOL32 PATH_MoveTo(HDC32 hdc)
{
   GdiPath *pPath;
   
   /* Get pointer to path */
   if(!PATH_GetPathFromHDC(hdc, &pPath))
      return FALSE;
   
   /* Check that path is open */
   if(pPath->state!=PATH_Open)
      /* FIXME: Do we have to call SetLastError? */
      return FALSE;

   /* Start a new stroke */
   pPath->newStroke=TRUE;

   return TRUE;
}

/* PATH_LineTo
 *
 * Should be called when a LineTo is performed on a DC that has an
 * open path. This adds a PT_LINETO entry to the path (and possibly
 * a PT_MOVETO entry, if this is the first LineTo in a stroke).
 * Returns TRUE if successful, else FALSE.
 */
BOOL32 PATH_LineTo(HDC32 hdc, INT32 x, INT32 y)
{
   GdiPath *pPath;
   POINT32 point, pointCurPos;
   
   /* Get pointer to path */
   if(!PATH_GetPathFromHDC(hdc, &pPath))
      return FALSE;
   
   /* Check that path is open */
   if(pPath->state!=PATH_Open)
      return FALSE;

   /* Convert point to device coordinates */
   point.x=x;
   point.y=y;
   if(!LPtoDP32(hdc, &point, 1))
      return FALSE;
   
   /* Add a PT_MOVETO if necessary */
   if(pPath->newStroke)
   {
      pPath->newStroke=FALSE;
      if(!GetCurrentPositionEx32(hdc, &pointCurPos) ||
         !LPtoDP32(hdc, &pointCurPos, 1))
         return FALSE;
      if(!PATH_AddEntry(pPath, &pointCurPos, PT_MOVETO))
         return FALSE;
   }
   
   /* Add a PT_LINETO entry */
   return PATH_AddEntry(pPath, &point, PT_LINETO);
}

/* PATH_Rectangle
 *
 * Should be called when a call to Rectangle is performed on a DC that has
 * an open path. Returns TRUE if successful, else FALSE.
 */
BOOL32 PATH_Rectangle(HDC32 hdc, INT32 x1, INT32 y1, INT32 x2, INT32 y2)
{
   GdiPath *pPath;
   POINT32 corners[2], pointTemp;
   INT32   temp;

   /* Get pointer to path */
   if(!PATH_GetPathFromHDC(hdc, &pPath))
      return FALSE;
   
   /* Check that path is open */
   if(pPath->state!=PATH_Open)
      return FALSE;

   /* Convert points to device coordinates */
   corners[0].x=x1;
   corners[0].y=y1;
   corners[1].x=x2;
   corners[1].y=y2;
   if(!LPtoDP32(hdc, corners, 2))
      return FALSE;
   
   /* Make sure first corner is top left and second corner is bottom right */
   if(corners[0].x>corners[1].x)
   {
      temp=corners[0].x;
      corners[0].x=corners[1].x;
      corners[1].x=temp;
   }
   if(corners[0].y>corners[1].y)
   {
      temp=corners[0].y;
      corners[0].y=corners[1].y;
      corners[1].y=temp;
   }
   
   /* In GM_COMPATIBLE, don't include bottom and right edges */
   if(GetGraphicsMode(hdc)==GM_COMPATIBLE)
   {
      corners[1].x--;
      corners[1].y--;
   }

   /* Close any previous figure */
   if(!CloseFigure32(hdc))
   {
      /* The CloseFigure call shouldn't have failed */
      assert(FALSE);
      return FALSE;
   }

   /* Add four points to the path */
   pointTemp.x=corners[1].x;
   pointTemp.y=corners[0].y;
   if(!PATH_AddEntry(pPath, &pointTemp, PT_MOVETO))
      return FALSE;
   if(!PATH_AddEntry(pPath, corners, PT_LINETO))
      return FALSE;
   pointTemp.x=corners[0].x;
   pointTemp.y=corners[1].y;
   if(!PATH_AddEntry(pPath, &pointTemp, PT_LINETO))
      return FALSE;
   if(!PATH_AddEntry(pPath, corners+1, PT_LINETO))
      return FALSE;

   /* Close the rectangle figure */
   if(!CloseFigure32(hdc))
   {
      /* The CloseFigure call shouldn't have failed */
      assert(FALSE);
      return FALSE;
   }

   return TRUE;
}

/* PATH_Ellipse
 * 
 * Should be called when a call to Ellipse is performed on a DC that has
 * an open path. This adds four Bezier splines representing the ellipse
 * to the path. Returns TRUE if successful, else FALSE.
 */
BOOL32 PATH_Ellipse(HDC32 hdc, INT32 x1, INT32 y1, INT32 x2, INT32 y2)
{
   // TODO: This should probably be revised to call PATH_AngleArc
   // (once it exists)
   return PATH_Arc(hdc, x1, y1, x2, y2, x1, (y1+y2)/2, x1, (y1+y2)/2);
}

/* PATH_Arc
 *
 * Should be called when a call to Arc is performed on a DC that has
 * an open path. This adds up to five Bezier splines representing the arc
 * to the path. Returns TRUE if successful, else FALSE.
 */
BOOL32 PATH_Arc(HDC32 hdc, INT32 x1, INT32 y1, INT32 x2, INT32 y2,
   INT32 xStart, INT32 yStart, INT32 xEnd, INT32 yEnd)
{
   GdiPath     *pPath;
   DC          *pDC;
   double      angleStart, angleEnd, angleStartQuadrant, angleEndQuadrant=0.0;
               /* Initialize angleEndQuadrant to silence gcc's warning */
   double      x, y;
   FLOAT_POINT corners[2], pointStart, pointEnd;
   BOOL32      start, end;
   INT32       temp;

   /* FIXME: This function should check for all possible error returns */
   /* FIXME: Do we have to respect newStroke? */
   
   /* Get pointer to DC */
   pDC=DC_GetDCPtr(hdc);
   if(pDC==NULL)
      return FALSE;

   /* Get pointer to path */
   if(!PATH_GetPathFromHDC(hdc, &pPath))
      return FALSE;
   
   /* Check that path is open */
   if(pPath->state!=PATH_Open)
      return FALSE;

   /* FIXME: Do we have to close the current figure? */
   
   /* Check for zero height / width */
   /* FIXME: Only in GM_COMPATIBLE? */
   if(x1==x2 || y1==y2)
      return TRUE;
   
   /* Convert points to device coordinates */
   corners[0].x=(FLOAT)x1;
   corners[0].y=(FLOAT)y1;
   corners[1].x=(FLOAT)x2;
   corners[1].y=(FLOAT)y2;
   pointStart.x=(FLOAT)xStart;
   pointStart.y=(FLOAT)yStart;
   pointEnd.x=(FLOAT)xEnd;
   pointEnd.y=(FLOAT)yEnd;
   INTERNAL_LPTODP_FLOAT(pDC, corners);
   INTERNAL_LPTODP_FLOAT(pDC, corners+1);
   INTERNAL_LPTODP_FLOAT(pDC, &pointStart);
   INTERNAL_LPTODP_FLOAT(pDC, &pointEnd);

   /* Make sure first corner is top left and second corner is bottom right */
   if(corners[0].x>corners[1].x)
   {
      temp=corners[0].x;
      corners[0].x=corners[1].x;
      corners[1].x=temp;
   }
   if(corners[0].y>corners[1].y)
   {
      temp=corners[0].y;
      corners[0].y=corners[1].y;
      corners[1].y=temp;
   }

   /* Compute start and end angle */
   PATH_NormalizePoint(corners, &pointStart, &x, &y);
   angleStart=atan2(y, x);
   PATH_NormalizePoint(corners, &pointEnd, &x, &y);
   angleEnd=atan2(y, x);

   /* Make sure the end angle is "on the right side" of the start angle */
   if(GetArcDirection32(hdc)==AD_CLOCKWISE)
   {
      if(angleEnd<=angleStart)
      {
         angleEnd+=2*M_PI;
	 assert(angleEnd>=angleStart);
      }
   }
   else
   {
      if(angleEnd>=angleStart)
      {
         angleEnd-=2*M_PI;
	 assert(angleEnd<=angleStart);
      }
   }

   /* In GM_COMPATIBLE, don't include bottom and right edges */
   if(GetGraphicsMode(hdc)==GM_COMPATIBLE)
   {
      corners[1].x--;
      corners[1].y--;
   }
   
   /* Add the arc to the path with one Bezier spline per quadrant that the
    * arc spans */
   start=TRUE;
   end=FALSE;
   do
   {
      /* Determine the start and end angles for this quadrant */
      if(start)
      {
         angleStartQuadrant=angleStart;
	 if(GetArcDirection32(hdc)==AD_CLOCKWISE)
	    angleEndQuadrant=(floor(angleStart/M_PI_2)+1.0)*M_PI_2;
	 else
	    angleEndQuadrant=(ceil(angleStart/M_PI_2)-1.0)*M_PI_2;
      }
      else
      {
	 angleStartQuadrant=angleEndQuadrant;
	 if(GetArcDirection32(hdc)==AD_CLOCKWISE)
	    angleEndQuadrant+=M_PI_2;
	 else
	    angleEndQuadrant-=M_PI_2;
      }

      /* Have we reached the last part of the arc? */
      if((GetArcDirection32(hdc)==AD_CLOCKWISE &&
         angleEnd<angleEndQuadrant) ||
	 (GetArcDirection32(hdc)==AD_COUNTERCLOCKWISE &&
	 angleEnd>angleEndQuadrant))
      {
	 /* Adjust the end angle for this quadrant */
         angleEndQuadrant=angleEnd;
	 end=TRUE;
      }

      /* Add the Bezier spline to the path */
      PATH_DoArcPart(pPath, corners, angleStartQuadrant, angleEndQuadrant,
         start);
      start=FALSE;
   }  while(!end);

   return TRUE;
}

/***********************************************************************
 * Internal functions
 */

/* PATH_PathToRegion
 *
 * Creates a region from the specified path using the specified polygon
 * filling mode. The path is left unchanged. A handle to the region that
 * was created is stored in *pHrgn. If successful, TRUE is returned; if an
 * error occurs, SetLastError is called with the appropriate value and
 * FALSE is returned.
 */
static BOOL32 PATH_PathToRegion(const GdiPath *pPath, INT32 nPolyFillMode,
   HRGN32 *pHrgn)
{
   int    numStrokes, iStroke, i;
   INT32  *pNumPointsInStroke;
   HRGN32 hrgn;

   assert(pPath!=NULL);
   assert(pHrgn!=NULL);
   
   /* FIXME: What happens when number of points is zero? */
   
   /* First pass: Find out how many strokes there are in the path */
   /* FIXME: We could eliminate this with some bookkeeping in GdiPath */
   numStrokes=0;
   for(i=0; i<pPath->numEntriesUsed; i++)
      if((pPath->pFlags[i] & ~PT_CLOSEFIGURE) == PT_MOVETO)
         numStrokes++;

   /* Allocate memory for number-of-points-in-stroke array */
   pNumPointsInStroke=(int *)malloc(sizeof(int)*numStrokes);
   if(!pNumPointsInStroke)
   {
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
   }
   
   /* Second pass: remember number of points in each polygon */
   iStroke=-1;  /* Will get incremented to 0 at beginning of first stroke */
   for(i=0; i<pPath->numEntriesUsed; i++)
   {
      /* Is this the beginning of a new stroke? */
      if((pPath->pFlags[i] & ~PT_CLOSEFIGURE) == PT_MOVETO)
      {
         iStroke++;
	 pNumPointsInStroke[iStroke]=0;
      }

      pNumPointsInStroke[iStroke]++;
   }

   /* Create a region from the strokes */
   hrgn=CreatePolyPolygonRgn32(pPath->pPoints, pNumPointsInStroke,
      numStrokes, nPolyFillMode);
   if(hrgn==(HRGN32)0)
   {
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
   }

   /* Free memory for number-of-points-in-stroke array */
   free(pNumPointsInStroke);

   /* Success! */
   *pHrgn=hrgn;
   return TRUE;
}

/* PATH_EmptyPath
 *
 * Removes all entries from the path and sets the path state to PATH_Null.
 */
static void PATH_EmptyPath(GdiPath *pPath)
{
   assert(pPath!=NULL);

   pPath->state=PATH_Null;
   pPath->numEntriesUsed=0;
}

/* PATH_AddEntry
 *
 * Adds an entry to the path. For "flags", pass either PT_MOVETO, PT_LINETO
 * or PT_BEZIERTO, optionally ORed with PT_CLOSEFIGURE. Returns TRUE if
 * successful, FALSE otherwise (e.g. if not enough memory was available).
 */
BOOL32 PATH_AddEntry(GdiPath *pPath, const POINT32 *pPoint, BYTE flags)
{
   assert(pPath!=NULL);
   
   /* FIXME: If newStroke is true, perhaps we want to check that we're
    * getting a PT_MOVETO
    */

   /* Check that path is open */
   if(pPath->state!=PATH_Open)
      return FALSE;
   
   /* Reserve enough memory for an extra path entry */
   if(!PATH_ReserveEntries(pPath, pPath->numEntriesUsed+1))
      return FALSE;

   /* Store information in path entry */
   pPath->pPoints[pPath->numEntriesUsed]=*pPoint;
   pPath->pFlags[pPath->numEntriesUsed]=flags;

   /* If this is PT_CLOSEFIGURE, we have to start a new stroke next time */
   if((flags & PT_CLOSEFIGURE) == PT_CLOSEFIGURE)
      pPath->newStroke=TRUE;

   /* Increment entry count */
   pPath->numEntriesUsed++;

   return TRUE;
}

/* PATH_ReserveEntries
 *
 * Ensures that at least "numEntries" entries (for points and flags) have
 * been allocated; allocates larger arrays and copies the existing entries
 * to those arrays, if necessary. Returns TRUE if successful, else FALSE.
 */
static BOOL32 PATH_ReserveEntries(GdiPath *pPath, INT32 numEntries)
{
   INT32   numEntriesToAllocate;
   POINT32 *pPointsNew;
   BYTE    *pFlagsNew;
   
   assert(pPath!=NULL);
   assert(numEntries>=0);

   /* Do we have to allocate more memory? */
   if(numEntries > pPath->numEntriesAllocated)
   {
      /* Find number of entries to allocate. We let the size of the array
       * grow exponentially, since that will guarantee linear time
       * complexity. */
      if(pPath->numEntriesAllocated)
      {
	 numEntriesToAllocate=pPath->numEntriesAllocated;
	 while(numEntriesToAllocate<numEntries)
	    numEntriesToAllocate=numEntriesToAllocate*GROW_FACTOR_NUMER/
	       GROW_FACTOR_DENOM;
      }
      else
         numEntriesToAllocate=NUM_ENTRIES_INITIAL;

      /* Allocate new arrays */
      pPointsNew=(POINT32 *)malloc(numEntriesToAllocate * sizeof(POINT32));
      if(!pPointsNew)
         return FALSE;
      pFlagsNew=(BYTE *)malloc(numEntriesToAllocate * sizeof(BYTE));
      if(!pFlagsNew)
      {
         free(pPointsNew);
	 return FALSE;
      }

      /* Copy old arrays to new arrays and discard old arrays */
      if(pPath->pPoints)
      {
         assert(pPath->pFlags);

	 memcpy(pPointsNew, pPath->pPoints,
	     sizeof(POINT32)*pPath->numEntriesUsed);
	 memcpy(pFlagsNew, pPath->pFlags,
	     sizeof(BYTE)*pPath->numEntriesUsed);

	 free(pPath->pPoints);
	 free(pPath->pFlags);
      }
      pPath->pPoints=pPointsNew;
      pPath->pFlags=pFlagsNew;
      pPath->numEntriesAllocated=numEntriesToAllocate;
   }

   return TRUE;
}

/* PATH_GetPathFromHDC
 *
 * Retrieves a pointer to the GdiPath structure contained in an HDC and
 * places it in *ppPath. TRUE is returned if successful, FALSE otherwise.
 */
static BOOL32 PATH_GetPathFromHDC(HDC32 hdc, GdiPath **ppPath)
{
   DC *pDC;

   pDC=DC_GetDCPtr(hdc);
   if(pDC)
   {
      *ppPath=&pDC->w.path;
      return TRUE;
   }
   else
      return FALSE;
}

/* PATH_DoArcPart
 *
 * Creates a Bezier spline that corresponds to part of an arc and appends the
 * corresponding points to the path. The start and end angles are passed in
 * "angleStart" and "angleEnd"; these angles should span a quarter circle
 * at most. If "addMoveTo" is true, a PT_MOVETO entry for the first control
 * point is added to the path; otherwise, it is assumed that the current
 * position is equal to the first control point.
 */
static BOOL32 PATH_DoArcPart(GdiPath *pPath, FLOAT_POINT corners[],
   double angleStart, double angleEnd, BOOL32 addMoveTo)
{
   double  halfAngle, a;
   double  xNorm[4], yNorm[4];
   POINT32 point;
   int     i;

   assert(fabs(angleEnd-angleStart)<=M_PI_2);

   /* FIXME: Is there an easier way of computing this? */

   /* Compute control points */
   halfAngle=(angleEnd-angleStart)/2.0;
   if(fabs(halfAngle)>1e-8)
   {
      a=4.0/3.0*(1-cos(halfAngle))/sin(halfAngle);
      xNorm[0]=cos(angleStart);
      yNorm[0]=sin(angleStart);
      xNorm[1]=xNorm[0] - a*yNorm[0];
      yNorm[1]=yNorm[0] + a*xNorm[0];
      xNorm[3]=cos(angleEnd);
      yNorm[3]=sin(angleEnd);
      xNorm[2]=xNorm[3] + a*yNorm[3];
      yNorm[2]=yNorm[3] - a*xNorm[3];
   }
   else
      for(i=0; i<4; i++)
      {
	 xNorm[i]=cos(angleStart);
	 yNorm[i]=sin(angleStart);
      }
   
   /* Add starting point to path if desired */
   if(addMoveTo)
   {
      PATH_ScaleNormalizedPoint(corners, xNorm[0], yNorm[0], &point);
      if(!PATH_AddEntry(pPath, &point, PT_MOVETO))
         return FALSE;
   }

   /* Add remaining control points */
   for(i=1; i<4; i++)
   {
      PATH_ScaleNormalizedPoint(corners, xNorm[i], yNorm[i], &point);
      if(!PATH_AddEntry(pPath, &point, PT_BEZIERTO))
         return FALSE;
   }

   return TRUE;
}

/* PATH_ScaleNormalizedPoint
 *
 * Scales a normalized point (x, y) with respect to the box whose corners are
 * passed in "corners". The point is stored in "*pPoint". The normalized
 * coordinates (-1.0, -1.0) correspond to corners[0], the coordinates
 * (1.0, 1.0) correspond to corners[1].
 */
static void PATH_ScaleNormalizedPoint(FLOAT_POINT corners[], double x,
   double y, POINT32 *pPoint)
{
   pPoint->x=GDI_ROUND( (double)corners[0].x +
      (double)(corners[1].x-corners[0].x)*0.5*(x+1.0) );
   pPoint->y=GDI_ROUND( (double)corners[0].y +
      (double)(corners[1].y-corners[0].y)*0.5*(y+1.0) );
}

/* PATH_NormalizePoint
 *
 * Normalizes a point with respect to the box whose corners are passed in
 * "corners". The normalized coordinates are stored in "*pX" and "*pY".
 */
static void PATH_NormalizePoint(FLOAT_POINT corners[],
   const FLOAT_POINT *pPoint,
   double *pX, double *pY)
{
   *pX=(double)(pPoint->x-corners[0].x)/(double)(corners[1].x-corners[0].x) *
      2.0 - 1.0;
   *pY=(double)(pPoint->y-corners[0].y)/(double)(corners[1].y-corners[0].y) *
      2.0 - 1.0;
}
