/*
 * Graphics paths (BeginPath, EndPath etc.)
 *
 * Copyright 1997 Martin Boehme
 */

#include <assert.h>
#include <malloc.h>

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
static BOOL32 PATH_AddEntry(GdiPath *pPath, POINT32 point, BYTE flags);
static BOOL32 PATH_ReserveEntries(GdiPath *pPath, INT32 numEntries);
static BOOL32 PATH_GetPathFromHDC(HDC32 hdc, GdiPath **ppPath);

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
 *           AbortPath32    (GDI32.1)
 */
BOOL32 WINAPI AbortPath32(HDC32 hdc)
/* FIXME: Check that SetLastError is being called correctly */
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
 *           CloseFigure32    (GDI32.16)
 */
BOOL32 WINAPI CloseFigure32(HDC32 hdc)
/* FIXME: Check that SetLastError is being called correctly */
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
   
   /* Set PT_CLOSEFIGURE on the last entry and start a new stroke */
   if(pPath->numEntriesUsed)
   {
      pPath->pFlags[pPath->numEntriesUsed-1]|=PT_CLOSEFIGURE;
      pPath->newStroke=TRUE;
   }

   return TRUE;
}


/***********************************************************************
 *           GetPath32    (GDI32.210)
 */
INT32 WINAPI GetPath32(HDC32 hdc, LPPOINT32 pPoints, LPBYTE pTypes,
   INT32 nSize)
{
   GdiPath *pPath;
   BOOL32  temp_flag;
   
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
      temp_flag=DPtoLP32(hdc, pPoints, pPath->numEntriesUsed);

      /* Since hdc is valid, conversion should never fail */
      assert(temp_flag);
      
      return pPath->numEntriesUsed;
   }
}


/***********************************************************************
 *           PathToRegion32    (GDI32.261)
 */
HRGN32 WINAPI PathToRegion32(HDC32 hdc)
/* FIXME: Check that SetLastError is being called correctly */
/* The documentation does not state this explicitly, but a test under Windows
 * shows that the region which is returned should be in device coordinates.
 */
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
 */
BOOL32 WINAPI FillPath32(HDC32 hdc)
/* FIXME: Check that SetLastError is being called correctly */
{
   GdiPath *pPath;
   INT32   mapMode;
   POINT32 ptViewportExt, ptViewportOrg, ptWindowExt, ptWindowOrg;
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
       */
       
      /* Save the information about the old mapping mode */
      mapMode=GetMapMode32(hdc);
      GetViewportExtEx32(hdc, &ptViewportExt);
      GetViewportOrgEx32(hdc, &ptViewportOrg);
      GetWindowExtEx32(hdc, &ptWindowExt);
      GetWindowOrgEx32(hdc, &ptWindowOrg);
      
      /* FIXME: Once world transforms become available, we will have to do
       * a GetWorldTransform, too (along with a SetWorldTransform later on).
       * Moral: Perhaps I should have used SaveDC right away. The reason why
       * I didn't is that I wanted to avoid the overhead of a full SaveDC
       * (especially since SaveDC now saves the current path as well).
       */

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
 */
BOOL32 WINAPI SelectClipPath32(HDC32 hdc, int iMode)
/* FIXME: Check that SetLastError is being called correctly */
{
   GdiPath *pPath;
   HRGN32  hrgnPath, hrgnClip;
   BOOL32  success = FALSE;
   
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
      hrgnClip=CreateRectRgn32(0, 0, 0, 0);
      if(hrgnClip!=NULL)
      {
         success=(GetClipRgn32(hdc, hrgnClip)!=-1) &&
	    (CombineRgn32(hrgnClip, hrgnClip, hrgnPath, iMode)!=ERROR) &&
	    (SelectClipRgn32(hdc, hrgnClip)!=ERROR);
	 DeleteObject32(hrgnClip);
      }

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
      /* FIXME: Do we have to call SetLastError? */
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
      if(!PATH_AddEntry(pPath, pointCurPos, PT_MOVETO))
         return FALSE;
   }
   
   /* Add a PT_LINETO entry */
   return PATH_AddEntry(pPath, point, PT_LINETO);
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
   if(hrgn==NULL)
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
BOOL32 PATH_AddEntry(GdiPath *pPath, POINT32 point, BYTE flags)
{
   assert(pPath!=NULL);
   
   /* Check that path is open */
   if(pPath->state!=PATH_Open)
      return FALSE;
   
   /* Reserve enough memory for an extra path entry */
   if(!PATH_ReserveEntries(pPath, pPath->numEntriesUsed+1))
      return FALSE;

   /* Store information in path entry */
   pPath->pPoints[pPath->numEntriesUsed]=point;
   pPath->pFlags[pPath->numEntriesUsed]=flags;

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
