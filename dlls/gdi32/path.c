/*
 * Graphics paths (BeginPath, EndPath etc.)
 *
 * Copyright 1997, 1998 Martin Boehme
 *                 1999 Huw D M Davies
 * Copyright 2005 Dmitry Timoshkov
 * Copyright 2011 Alexandre Julliard
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

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#if defined(HAVE_FLOAT_H)
#include <float.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winerror.h"

#include "gdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdi);

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
 * Martin Boehme
 */

#define NUM_ENTRIES_INITIAL 16  /* Initial size of points / flags arrays  */

/* A floating point version of the POINT structure */
typedef struct tagFLOAT_POINT
{
   double x, y;
} FLOAT_POINT;

struct gdi_path
{
    POINT       *points;
    BYTE        *flags;
    int          count;
    int          allocated;
    BOOL         newStroke;
};

struct path_physdev
{
    struct gdi_physdev dev;
    struct gdi_path   *path;
};

static inline struct path_physdev *get_path_physdev( PHYSDEV dev )
{
    return (struct path_physdev *)dev;
}

static inline void pop_path_driver( DC *dc, struct path_physdev *physdev )
{
    pop_dc_driver( dc, &physdev->dev );
    HeapFree( GetProcessHeap(), 0, physdev );
}

static inline struct path_physdev *find_path_physdev( DC *dc )
{
    PHYSDEV dev;

    for (dev = dc->physDev; dev->funcs != &null_driver; dev = dev->next)
        if (dev->funcs == &path_driver) return get_path_physdev( dev );
    return NULL;
}

void free_gdi_path( struct gdi_path *path )
{
    HeapFree( GetProcessHeap(), 0, path->points );
    HeapFree( GetProcessHeap(), 0, path->flags );
    HeapFree( GetProcessHeap(), 0, path );
}

static struct gdi_path *alloc_gdi_path( int count )
{
    struct gdi_path *path = HeapAlloc( GetProcessHeap(), 0, sizeof(*path) );

    if (!path)
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return NULL;
    }
    count = max( NUM_ENTRIES_INITIAL, count );
    path->points = HeapAlloc( GetProcessHeap(), 0, count * sizeof(*path->points) );
    path->flags = HeapAlloc( GetProcessHeap(), 0, count * sizeof(*path->flags) );
    if (!path->points || !path->flags)
    {
        free_gdi_path( path );
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return NULL;
    }
    path->count = 0;
    path->allocated = count;
    path->newStroke = TRUE;
    return path;
}

static struct gdi_path *copy_gdi_path( const struct gdi_path *src_path )
{
    struct gdi_path *path = HeapAlloc( GetProcessHeap(), 0, sizeof(*path) );

    if (!path)
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return NULL;
    }
    path->count = path->allocated = src_path->count;
    path->newStroke = src_path->newStroke;
    path->points = HeapAlloc( GetProcessHeap(), 0, path->count * sizeof(*path->points) );
    path->flags = HeapAlloc( GetProcessHeap(), 0, path->count * sizeof(*path->flags) );
    if (!path->points || !path->flags)
    {
        free_gdi_path( path );
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return NULL;
    }
    memcpy( path->points, src_path->points, path->count * sizeof(*path->points) );
    memcpy( path->flags, src_path->flags, path->count * sizeof(*path->flags) );
    return path;
}

/* Performs a world-to-viewport transformation on the specified point (which
 * is in floating point format).
 */
static inline void INTERNAL_LPTODP_FLOAT( HDC hdc, FLOAT_POINT *point, int count )
{
    DC *dc = get_dc_ptr( hdc );
    double x, y;

    while (count--)
    {
        x = point->x;
        y = point->y;
        point->x = x * dc->xformWorld2Vport.eM11 + y * dc->xformWorld2Vport.eM21 + dc->xformWorld2Vport.eDx;
        point->y = x * dc->xformWorld2Vport.eM12 + y * dc->xformWorld2Vport.eM22 + dc->xformWorld2Vport.eDy;
        point++;
    }
    release_dc_ptr( dc );
}

static inline INT int_from_fixed(FIXED f)
{
    return (f.fract >= 0x8000) ? (f.value + 1) : f.value;
}


/* PATH_ReserveEntries
 *
 * Ensures that at least "numEntries" entries (for points and flags) have
 * been allocated; allocates larger arrays and copies the existing entries
 * to those arrays, if necessary. Returns TRUE if successful, else FALSE.
 */
static BOOL PATH_ReserveEntries(struct gdi_path *pPath, INT count)
{
    POINT *pPointsNew;
    BYTE    *pFlagsNew;

    assert(count>=0);

    /* Do we have to allocate more memory? */
    if(count > pPath->allocated)
    {
        /* Find number of entries to allocate. We let the size of the array
         * grow exponentially, since that will guarantee linear time
         * complexity. */
        count = max( pPath->allocated * 2, count );

        pPointsNew = HeapReAlloc( GetProcessHeap(), 0, pPath->points, count * sizeof(POINT) );
        if (!pPointsNew) return FALSE;
        pPath->points = pPointsNew;

        pFlagsNew = HeapReAlloc( GetProcessHeap(), 0, pPath->flags, count * sizeof(BYTE) );
        if (!pFlagsNew) return FALSE;
        pPath->flags = pFlagsNew;

        pPath->allocated = count;
    }
    return TRUE;
}

/* PATH_AddEntry
 *
 * Adds an entry to the path. For "flags", pass either PT_MOVETO, PT_LINETO
 * or PT_BEZIERTO, optionally ORed with PT_CLOSEFIGURE. Returns TRUE if
 * successful, FALSE otherwise (e.g. if not enough memory was available).
 */
static BOOL PATH_AddEntry(struct gdi_path *pPath, const POINT *pPoint, BYTE flags)
{
    /* FIXME: If newStroke is true, perhaps we want to check that we're
     * getting a PT_MOVETO
     */
    TRACE("(%d,%d) - %d\n", pPoint->x, pPoint->y, flags);

    /* Reserve enough memory for an extra path entry */
    if(!PATH_ReserveEntries(pPath, pPath->count+1))
        return FALSE;

    /* Store information in path entry */
    pPath->points[pPath->count]=*pPoint;
    pPath->flags[pPath->count]=flags;

    pPath->count++;

    return TRUE;
}

/* add a number of points, converting them to device coords */
/* return a pointer to the first type byte so it can be fixed up if necessary */
static BYTE *add_log_points( struct path_physdev *physdev, const POINT *points, DWORD count, BYTE type )
{
    BYTE *ret;
    struct gdi_path *path = physdev->path;

    if (!PATH_ReserveEntries( path, path->count + count )) return NULL;

    ret = &path->flags[path->count];
    memcpy( &path->points[path->count], points, count * sizeof(*points) );
    LPtoDP( physdev->dev.hdc, &path->points[path->count], count );
    memset( ret, type, count );
    path->count += count;
    return ret;
}

/* start a new path stroke if necessary */
static BOOL start_new_stroke( struct path_physdev *physdev )
{
    POINT pos;
    struct gdi_path *path = physdev->path;

    if (!path->newStroke && path->count &&
        !(path->flags[path->count - 1] & PT_CLOSEFIGURE))
        return TRUE;

    path->newStroke = FALSE;
    GetCurrentPositionEx( physdev->dev.hdc, &pos );
    return add_log_points( physdev, &pos, 1, PT_MOVETO ) != NULL;
}

/* PATH_CheckCorners
 *
 * Helper function for RoundRect() and Rectangle()
 */
static void PATH_CheckCorners( HDC hdc, POINT corners[], INT x1, INT y1, INT x2, INT y2 )
{
    INT temp;

    /* Convert points to device coordinates */
    corners[0].x=x1;
    corners[0].y=y1;
    corners[1].x=x2;
    corners[1].y=y2;
    LPtoDP( hdc, corners, 2 );

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
    if (GetGraphicsMode( hdc ) == GM_COMPATIBLE)
    {
        corners[1].x--;
        corners[1].y--;
    }
}

/* PATH_AddFlatBezier
 */
static BOOL PATH_AddFlatBezier(struct gdi_path *pPath, POINT *pt, BOOL closed)
{
    POINT *pts;
    INT no, i;

    pts = GDI_Bezier( pt, 4, &no );
    if(!pts) return FALSE;

    for(i = 1; i < no; i++)
        PATH_AddEntry(pPath, &pts[i], (i == no-1 && closed) ? PT_LINETO | PT_CLOSEFIGURE : PT_LINETO);
    HeapFree( GetProcessHeap(), 0, pts );
    return TRUE;
}

/* PATH_FlattenPath
 *
 * Replaces Beziers with line segments
 *
 */
static struct gdi_path *PATH_FlattenPath(const struct gdi_path *pPath)
{
    struct gdi_path *new_path;
    INT srcpt;

    if (!(new_path = alloc_gdi_path( pPath->count ))) return NULL;

    for(srcpt = 0; srcpt < pPath->count; srcpt++) {
        switch(pPath->flags[srcpt] & ~PT_CLOSEFIGURE) {
	case PT_MOVETO:
	case PT_LINETO:
	    if (!PATH_AddEntry(new_path, &pPath->points[srcpt], pPath->flags[srcpt]))
            {
                free_gdi_path( new_path );
                return NULL;
            }
	    break;
	case PT_BEZIERTO:
            if (!PATH_AddFlatBezier(new_path, &pPath->points[srcpt-1],
                                    pPath->flags[srcpt+2] & PT_CLOSEFIGURE))
            {
                free_gdi_path( new_path );
                return NULL;
            }
	    srcpt += 2;
	    break;
	}
    }
    return new_path;
}

/* PATH_PathToRegion
 *
 * Creates a region from the specified path using the specified polygon
 * filling mode. The path is left unchanged.
 */
static HRGN PATH_PathToRegion(const struct gdi_path *pPath, INT nPolyFillMode)
{
    struct gdi_path *rgn_path;
    int    numStrokes, iStroke, i;
    INT  *pNumPointsInStroke;
    HRGN hrgn;

    if (!(rgn_path = PATH_FlattenPath( pPath ))) return 0;

    /* FIXME: What happens when number of points is zero? */

    /* First pass: Find out how many strokes there are in the path */
    /* FIXME: We could eliminate this with some bookkeeping in GdiPath */
    numStrokes=0;
    for(i=0; i<rgn_path->count; i++)
        if((rgn_path->flags[i] & ~PT_CLOSEFIGURE) == PT_MOVETO)
            numStrokes++;

    /* Allocate memory for number-of-points-in-stroke array */
    pNumPointsInStroke=HeapAlloc( GetProcessHeap(), 0, sizeof(int) * numStrokes );
    if(!pNumPointsInStroke)
    {
        free_gdi_path( rgn_path );
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    /* Second pass: remember number of points in each polygon */
    iStroke=-1;  /* Will get incremented to 0 at beginning of first stroke */
    for(i=0; i<rgn_path->count; i++)
    {
        /* Is this the beginning of a new stroke? */
        if((rgn_path->flags[i] & ~PT_CLOSEFIGURE) == PT_MOVETO)
        {
            iStroke++;
            pNumPointsInStroke[iStroke]=0;
        }

        pNumPointsInStroke[iStroke]++;
    }

    /* Create a region from the strokes */
    hrgn=CreatePolyPolygonRgn(rgn_path->points, pNumPointsInStroke,
                              numStrokes, nPolyFillMode);

    HeapFree( GetProcessHeap(), 0, pNumPointsInStroke );
    free_gdi_path( rgn_path );
    return hrgn;
}

/* PATH_ScaleNormalizedPoint
 *
 * Scales a normalized point (x, y) with respect to the box whose corners are
 * passed in "corners". The point is stored in "*pPoint". The normalized
 * coordinates (-1.0, -1.0) correspond to corners[0], the coordinates
 * (1.0, 1.0) correspond to corners[1].
 */
static void PATH_ScaleNormalizedPoint(FLOAT_POINT corners[], double x,
   double y, POINT *pPoint)
{
    pPoint->x=GDI_ROUND( (double)corners[0].x + (double)(corners[1].x-corners[0].x)*0.5*(x+1.0) );
    pPoint->y=GDI_ROUND( (double)corners[0].y + (double)(corners[1].y-corners[0].y)*0.5*(y+1.0) );
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
    *pX=(double)(pPoint->x-corners[0].x)/(double)(corners[1].x-corners[0].x) * 2.0 - 1.0;
    *pY=(double)(pPoint->y-corners[0].y)/(double)(corners[1].y-corners[0].y) * 2.0 - 1.0;
}

/* PATH_DoArcPart
 *
 * Creates a Bezier spline that corresponds to part of an arc and appends the
 * corresponding points to the path. The start and end angles are passed in
 * "angleStart" and "angleEnd"; these angles should span a quarter circle
 * at most. If "startEntryType" is non-zero, an entry of that type for the first
 * control point is added to the path; otherwise, it is assumed that the current
 * position is equal to the first control point.
 */
static BOOL PATH_DoArcPart(struct gdi_path *pPath, FLOAT_POINT corners[],
   double angleStart, double angleEnd, BYTE startEntryType)
{
    double  halfAngle, a;
    double  xNorm[4], yNorm[4];
    POINT point;
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
    if(startEntryType)
    {
        PATH_ScaleNormalizedPoint(corners, xNorm[0], yNorm[0], &point);
        if(!PATH_AddEntry(pPath, &point, startEntryType))
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


/***********************************************************************
 *           BeginPath    (GDI32.@)
 */
BOOL WINAPI BeginPath(HDC hdc)
{
    BOOL ret = FALSE;
    DC *dc = get_dc_ptr( hdc );

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pBeginPath );
        ret = physdev->funcs->pBeginPath( physdev );
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           EndPath    (GDI32.@)
 */
BOOL WINAPI EndPath(HDC hdc)
{
    BOOL ret = FALSE;
    DC *dc = get_dc_ptr( hdc );

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pEndPath );
        ret = physdev->funcs->pEndPath( physdev );
        release_dc_ptr( dc );
    }
    return ret;
}


/******************************************************************************
 * AbortPath [GDI32.@]
 * Closes and discards paths from device context
 *
 * NOTES
 *    Check that SetLastError is being called correctly
 *
 * PARAMS
 *    hdc [I] Handle to device context
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI AbortPath( HDC hdc )
{
    BOOL ret = FALSE;
    DC *dc = get_dc_ptr( hdc );

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pAbortPath );
        ret = physdev->funcs->pAbortPath( physdev );
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           CloseFigure    (GDI32.@)
 *
 * FIXME: Check that SetLastError is being called correctly
 */
BOOL WINAPI CloseFigure(HDC hdc)
{
    BOOL ret = FALSE;
    DC *dc = get_dc_ptr( hdc );

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pCloseFigure );
        ret = physdev->funcs->pCloseFigure( physdev );
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           GetPath    (GDI32.@)
 */
INT WINAPI GetPath(HDC hdc, LPPOINT pPoints, LPBYTE pTypes, INT nSize)
{
   INT ret = -1;
   DC *dc = get_dc_ptr( hdc );

   if(!dc) return -1;

   if (!dc->path)
   {
      SetLastError(ERROR_CAN_NOT_COMPLETE);
      goto done;
   }

   if(nSize==0)
      ret = dc->path->count;
   else if(nSize<dc->path->count)
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      goto done;
   }
   else
   {
      memcpy(pPoints, dc->path->points, sizeof(POINT)*dc->path->count);
      memcpy(pTypes, dc->path->flags, sizeof(BYTE)*dc->path->count);

      /* Convert the points to logical coordinates */
      if(!DPtoLP(hdc, pPoints, dc->path->count))
      {
	 /* FIXME: Is this the correct value? */
         SetLastError(ERROR_CAN_NOT_COMPLETE);
	goto done;
      }
     else ret = dc->path->count;
   }
 done:
   release_dc_ptr( dc );
   return ret;
}


/***********************************************************************
 *           PathToRegion    (GDI32.@)
 *
 * FIXME
 *   Check that SetLastError is being called correctly
 *
 * The documentation does not state this explicitly, but a test under Windows
 * shows that the region which is returned should be in device coordinates.
 */
HRGN WINAPI PathToRegion(HDC hdc)
{
   HRGN  hrgnRval = 0;
   DC *dc = get_dc_ptr( hdc );

   /* Get pointer to path */
   if(!dc) return 0;

   if (!dc->path) SetLastError(ERROR_CAN_NOT_COMPLETE);
   else
   {
       if ((hrgnRval = PATH_PathToRegion(dc->path, GetPolyFillMode(hdc))))
       {
           /* FIXME: Should we empty the path even if conversion failed? */
           free_gdi_path( dc->path );
           dc->path = NULL;
       }
   }
   release_dc_ptr( dc );
   return hrgnRval;
}

static BOOL PATH_FillPath( HDC hdc, const struct gdi_path *pPath )
{
   INT   mapMode, graphicsMode;
   SIZE  ptViewportExt, ptWindowExt;
   POINT ptViewportOrg, ptWindowOrg;
   XFORM xform;
   HRGN  hrgn;

   /* Construct a region from the path and fill it */
   if ((hrgn = PATH_PathToRegion(pPath, GetPolyFillMode(hdc))))
   {
      /* Since PaintRgn interprets the region as being in logical coordinates
       * but the points we store for the path are already in device
       * coordinates, we have to set the mapping mode to MM_TEXT temporarily.
       * Using SaveDC to save information about the mapping mode / world
       * transform would be easier but would require more overhead, especially
       * now that SaveDC saves the current path.
       */

      /* Save the information about the old mapping mode */
      mapMode=GetMapMode(hdc);
      GetViewportExtEx(hdc, &ptViewportExt);
      GetViewportOrgEx(hdc, &ptViewportOrg);
      GetWindowExtEx(hdc, &ptWindowExt);
      GetWindowOrgEx(hdc, &ptWindowOrg);

      /* Save world transform
       * NB: The Windows documentation on world transforms would lead one to
       * believe that this has to be done only in GM_ADVANCED; however, my
       * tests show that resetting the graphics mode to GM_COMPATIBLE does
       * not reset the world transform.
       */
      GetWorldTransform(hdc, &xform);

      /* Set MM_TEXT */
      SetMapMode(hdc, MM_TEXT);
      SetViewportOrgEx(hdc, 0, 0, NULL);
      SetWindowOrgEx(hdc, 0, 0, NULL);
      graphicsMode=GetGraphicsMode(hdc);
      SetGraphicsMode(hdc, GM_ADVANCED);
      ModifyWorldTransform(hdc, &xform, MWT_IDENTITY);
      SetGraphicsMode(hdc, graphicsMode);

      /* Paint the region */
      PaintRgn(hdc, hrgn);
      DeleteObject(hrgn);
      /* Restore the old mapping mode */
      SetMapMode(hdc, mapMode);
      SetViewportExtEx(hdc, ptViewportExt.cx, ptViewportExt.cy, NULL);
      SetViewportOrgEx(hdc, ptViewportOrg.x, ptViewportOrg.y, NULL);
      SetWindowExtEx(hdc, ptWindowExt.cx, ptWindowExt.cy, NULL);
      SetWindowOrgEx(hdc, ptWindowOrg.x, ptWindowOrg.y, NULL);

      /* Go to GM_ADVANCED temporarily to restore the world transform */
      graphicsMode=GetGraphicsMode(hdc);
      SetGraphicsMode(hdc, GM_ADVANCED);
      SetWorldTransform(hdc, &xform);
      SetGraphicsMode(hdc, graphicsMode);
      return TRUE;
   }
   return FALSE;
}


/***********************************************************************
 *           FillPath    (GDI32.@)
 *
 * FIXME
 *    Check that SetLastError is being called correctly
 */
BOOL WINAPI FillPath(HDC hdc)
{
    BOOL ret = FALSE;
    DC *dc = get_dc_ptr( hdc );

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pFillPath );
        ret = physdev->funcs->pFillPath( physdev );
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           SelectClipPath    (GDI32.@)
 * FIXME
 *  Check that SetLastError is being called correctly
 */
BOOL WINAPI SelectClipPath(HDC hdc, INT iMode)
{
    BOOL ret = FALSE;
    DC *dc = get_dc_ptr( hdc );

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pSelectClipPath );
        ret = physdev->funcs->pSelectClipPath( physdev, iMode );
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           pathdrv_BeginPath
 */
static BOOL pathdrv_BeginPath( PHYSDEV dev )
{
    /* path already open, nothing to do */
    return TRUE;
}


/***********************************************************************
 *           pathdrv_AbortPath
 */
static BOOL pathdrv_AbortPath( PHYSDEV dev )
{
    struct path_physdev *physdev = get_path_physdev( dev );
    DC *dc = get_dc_ptr( dev->hdc );

    if (!dc) return FALSE;
    free_gdi_path( physdev->path );
    pop_path_driver( dc, physdev );
    release_dc_ptr( dc );
    return TRUE;
}


/***********************************************************************
 *           pathdrv_EndPath
 */
static BOOL pathdrv_EndPath( PHYSDEV dev )
{
    struct path_physdev *physdev = get_path_physdev( dev );
    DC *dc = get_dc_ptr( dev->hdc );

    if (!dc) return FALSE;
    dc->path = physdev->path;
    pop_path_driver( dc, physdev );
    release_dc_ptr( dc );
    return TRUE;
}


/***********************************************************************
 *           pathdrv_CreateDC
 */
static BOOL pathdrv_CreateDC( PHYSDEV *dev, LPCWSTR driver, LPCWSTR device,
                              LPCWSTR output, const DEVMODEW *devmode )
{
    struct path_physdev *physdev = HeapAlloc( GetProcessHeap(), 0, sizeof(*physdev) );
    DC *dc;

    if (!physdev) return FALSE;
    dc = get_dc_ptr( (*dev)->hdc );
    push_dc_driver( dev, &physdev->dev, &path_driver );
    release_dc_ptr( dc );
    return TRUE;
}


/*************************************************************
 *           pathdrv_DeleteDC
 */
static BOOL pathdrv_DeleteDC( PHYSDEV dev )
{
    assert( 0 );  /* should never be called */
    return TRUE;
}


BOOL PATH_SavePath( DC *dst, DC *src )
{
    struct path_physdev *physdev;

    if (src->path)
    {
        if (!(dst->path = copy_gdi_path( src->path ))) return FALSE;
    }
    else if ((physdev = find_path_physdev( src )))
    {
        if (!(dst->path = copy_gdi_path( physdev->path ))) return FALSE;
        dst->path_open = TRUE;
    }
    else dst->path = NULL;
    return TRUE;
}

BOOL PATH_RestorePath( DC *dst, DC *src )
{
    struct path_physdev *physdev = find_path_physdev( dst );

    if (src->path && src->path_open)
    {
        if (!physdev)
        {
            if (!path_driver.pCreateDC( &dst->physDev, NULL, NULL, NULL, NULL )) return FALSE;
            physdev = get_path_physdev( dst->physDev );
        }
        else free_gdi_path( physdev->path );

        physdev->path = src->path;
        src->path_open = FALSE;
        src->path = NULL;
    }
    else if (physdev)
    {
        free_gdi_path( physdev->path );
        pop_path_driver( dst, physdev );
    }
    if (dst->path) free_gdi_path( dst->path );
    dst->path = src->path;
    src->path = NULL;
    return TRUE;
}


/*************************************************************
 *           pathdrv_MoveTo
 */
static BOOL pathdrv_MoveTo( PHYSDEV dev, INT x, INT y )
{
    struct path_physdev *physdev = get_path_physdev( dev );
    physdev->path->newStroke = TRUE;
    return TRUE;
}


/*************************************************************
 *           pathdrv_LineTo
 */
static BOOL pathdrv_LineTo( PHYSDEV dev, INT x, INT y )
{
    struct path_physdev *physdev = get_path_physdev( dev );
    POINT point;

    if (!start_new_stroke( physdev )) return FALSE;
    point.x = x;
    point.y = y;
    return add_log_points( physdev, &point, 1, PT_LINETO ) != NULL;
}


/*************************************************************
 *           pathdrv_RoundRect
 *
 * FIXME: it adds the same entries to the path as windows does, but there
 * is an error in the bezier drawing code so that there are small pixel-size
 * gaps when the resulting path is drawn by StrokePath()
 */
static BOOL pathdrv_RoundRect( PHYSDEV dev, INT x1, INT y1, INT x2, INT y2, INT ell_width, INT ell_height )
{
    struct path_physdev *physdev = get_path_physdev( dev );
    POINT corners[2], pointTemp;
    FLOAT_POINT ellCorners[2];

    PATH_CheckCorners(dev->hdc,corners,x1,y1,x2,y2);

   /* Add points to the roundrect path */
   ellCorners[0].x = corners[1].x-ell_width;
   ellCorners[0].y = corners[0].y;
   ellCorners[1].x = corners[1].x;
   ellCorners[1].y = corners[0].y+ell_height;
   if(!PATH_DoArcPart(physdev->path, ellCorners, 0, -M_PI_2, PT_MOVETO))
      return FALSE;
   pointTemp.x = corners[0].x+ell_width/2;
   pointTemp.y = corners[0].y;
   if(!PATH_AddEntry(physdev->path, &pointTemp, PT_LINETO))
      return FALSE;
   ellCorners[0].x = corners[0].x;
   ellCorners[1].x = corners[0].x+ell_width;
   if(!PATH_DoArcPart(physdev->path, ellCorners, -M_PI_2, -M_PI, FALSE))
      return FALSE;
   pointTemp.x = corners[0].x;
   pointTemp.y = corners[1].y-ell_height/2;
   if(!PATH_AddEntry(physdev->path, &pointTemp, PT_LINETO))
      return FALSE;
   ellCorners[0].y = corners[1].y-ell_height;
   ellCorners[1].y = corners[1].y;
   if(!PATH_DoArcPart(physdev->path, ellCorners, M_PI, M_PI_2, FALSE))
      return FALSE;
   pointTemp.x = corners[1].x-ell_width/2;
   pointTemp.y = corners[1].y;
   if(!PATH_AddEntry(physdev->path, &pointTemp, PT_LINETO))
      return FALSE;
   ellCorners[0].x = corners[1].x-ell_width;
   ellCorners[1].x = corners[1].x;
   if(!PATH_DoArcPart(physdev->path, ellCorners, M_PI_2, 0, FALSE))
      return FALSE;

   /* Close the roundrect figure */
   return CloseFigure( dev->hdc );
}


/*************************************************************
 *           pathdrv_Rectangle
 */
static BOOL pathdrv_Rectangle( PHYSDEV dev, INT x1, INT y1, INT x2, INT y2 )
{
    struct path_physdev *physdev = get_path_physdev( dev );
    POINT corners[2], pointTemp;

    PATH_CheckCorners(dev->hdc,corners,x1,y1,x2,y2);

   /* Add four points to the path */
   pointTemp.x=corners[1].x;
   pointTemp.y=corners[0].y;
   if(!PATH_AddEntry(physdev->path, &pointTemp, PT_MOVETO))
      return FALSE;
   if(!PATH_AddEntry(physdev->path, corners, PT_LINETO))
      return FALSE;
   pointTemp.x=corners[0].x;
   pointTemp.y=corners[1].y;
   if(!PATH_AddEntry(physdev->path, &pointTemp, PT_LINETO))
      return FALSE;
   if(!PATH_AddEntry(physdev->path, corners+1, PT_LINETO))
      return FALSE;

   /* Close the rectangle figure */
   return CloseFigure( dev->hdc );
}


/* PATH_Arc
 *
 * Should be called when a call to Arc is performed on a DC that has
 * an open path. This adds up to five Bezier splines representing the arc
 * to the path. When 'lines' is 1, we add 1 extra line to get a chord,
 * when 'lines' is 2, we add 2 extra lines to get a pie, and when 'lines' is
 * -1 we add 1 extra line from the current DC position to the starting position
 * of the arc before drawing the arc itself (arcto). Returns TRUE if successful,
 * else FALSE.
 */
static BOOL PATH_Arc( PHYSDEV dev, INT x1, INT y1, INT x2, INT y2,
                      INT xStart, INT yStart, INT xEnd, INT yEnd, INT lines )
{
    struct path_physdev *physdev = get_path_physdev( dev );
    double angleStart, angleEnd, angleStartQuadrant, angleEndQuadrant=0.0;
               /* Initialize angleEndQuadrant to silence gcc's warning */
    double x, y;
    FLOAT_POINT corners[2], pointStart, pointEnd;
    POINT centre;
    BOOL start, end;
    INT temp, direction = GetArcDirection(dev->hdc);

   /* FIXME: Do we have to respect newStroke? */

   /* Check for zero height / width */
   /* FIXME: Only in GM_COMPATIBLE? */
   if(x1==x2 || y1==y2)
      return TRUE;

   /* Convert points to device coordinates */
   corners[0].x = x1;
   corners[0].y = y1;
   corners[1].x = x2;
   corners[1].y = y2;
   pointStart.x = xStart;
   pointStart.y = yStart;
   pointEnd.x = xEnd;
   pointEnd.y = yEnd;
   INTERNAL_LPTODP_FLOAT(dev->hdc, corners, 2);
   INTERNAL_LPTODP_FLOAT(dev->hdc, &pointStart, 1);
   INTERNAL_LPTODP_FLOAT(dev->hdc, &pointEnd, 1);

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
   if (direction == AD_CLOCKWISE)
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
   if (GetGraphicsMode(dev->hdc) == GM_COMPATIBLE)
   {
      corners[1].x--;
      corners[1].y--;
   }

   /* arcto: Add a PT_MOVETO only if this is the first entry in a stroke */
   if (lines==-1 && !start_new_stroke( physdev )) return FALSE;

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
	 if (direction == AD_CLOCKWISE)
	    angleEndQuadrant=(floor(angleStart/M_PI_2)+1.0)*M_PI_2;
	 else
	    angleEndQuadrant=(ceil(angleStart/M_PI_2)-1.0)*M_PI_2;
      }
      else
      {
	 angleStartQuadrant=angleEndQuadrant;
	 if (direction == AD_CLOCKWISE)
	    angleEndQuadrant+=M_PI_2;
	 else
	    angleEndQuadrant-=M_PI_2;
      }

      /* Have we reached the last part of the arc? */
      if((direction == AD_CLOCKWISE && angleEnd<angleEndQuadrant) ||
	 (direction == AD_COUNTERCLOCKWISE && angleEnd>angleEndQuadrant))
      {
	 /* Adjust the end angle for this quadrant */
         angleEndQuadrant=angleEnd;
	 end=TRUE;
      }

      /* Add the Bezier spline to the path */
      PATH_DoArcPart(physdev->path, corners, angleStartQuadrant, angleEndQuadrant,
         start ? (lines==-1 ? PT_LINETO : PT_MOVETO) : FALSE);
      start=FALSE;
   }  while(!end);

   /* chord: close figure. pie: add line and close figure */
   if(lines==1)
   {
      return CloseFigure(dev->hdc);
   }
   else if(lines==2)
   {
      centre.x = (corners[0].x+corners[1].x)/2;
      centre.y = (corners[0].y+corners[1].y)/2;
      if(!PATH_AddEntry(physdev->path, &centre, PT_LINETO | PT_CLOSEFIGURE))
         return FALSE;
   }

   return TRUE;
}


/*************************************************************
 *           pathdrv_AngleArc
 */
static BOOL pathdrv_AngleArc( PHYSDEV dev, INT x, INT y, DWORD radius, FLOAT eStartAngle, FLOAT eSweepAngle)
{
    INT x1, y1, x2, y2, arcdir;
    BOOL ret;

    x1 = GDI_ROUND( x + cos(eStartAngle*M_PI/180) * radius );
    y1 = GDI_ROUND( y - sin(eStartAngle*M_PI/180) * radius );
    x2 = GDI_ROUND( x + cos((eStartAngle+eSweepAngle)*M_PI/180) * radius );
    y2 = GDI_ROUND( y - sin((eStartAngle+eSweepAngle)*M_PI/180) * radius );
    arcdir = SetArcDirection( dev->hdc, eSweepAngle >= 0 ? AD_COUNTERCLOCKWISE : AD_CLOCKWISE);
    ret = PATH_Arc( dev, x-radius, y-radius, x+radius, y+radius, x1, y1, x2, y2, -1 );
    SetArcDirection( dev->hdc, arcdir );
    return ret;
}


/*************************************************************
 *           pathdrv_Arc
 */
static BOOL pathdrv_Arc( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                         INT xstart, INT ystart, INT xend, INT yend )
{
    return PATH_Arc( dev, left, top, right, bottom, xstart, ystart, xend, yend, 0 );
}


/*************************************************************
 *           pathdrv_ArcTo
 */
static BOOL pathdrv_ArcTo( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                           INT xstart, INT ystart, INT xend, INT yend )
{
    return PATH_Arc( dev, left, top, right, bottom, xstart, ystart, xend, yend, -1 );
}


/*************************************************************
 *           pathdrv_Chord
 */
static BOOL pathdrv_Chord( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                           INT xstart, INT ystart, INT xend, INT yend )
{
    return PATH_Arc( dev, left, top, right, bottom, xstart, ystart, xend, yend, 1);
}


/*************************************************************
 *           pathdrv_Pie
 */
static BOOL pathdrv_Pie( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                         INT xstart, INT ystart, INT xend, INT yend )
{
    return PATH_Arc( dev, left, top, right, bottom, xstart, ystart, xend, yend, 2 );
}


/*************************************************************
 *           pathdrv_Ellipse
 */
static BOOL pathdrv_Ellipse( PHYSDEV dev, INT x1, INT y1, INT x2, INT y2 )
{
    return PATH_Arc( dev, x1, y1, x2, y2, x1, (y1+y2)/2, x1, (y1+y2)/2, 0 ) && CloseFigure( dev->hdc );
}


/*************************************************************
 *           pathdrv_PolyBezierTo
 */
static BOOL pathdrv_PolyBezierTo( PHYSDEV dev, const POINT *pts, DWORD cbPoints )
{
    struct path_physdev *physdev = get_path_physdev( dev );

    if (!start_new_stroke( physdev )) return FALSE;
    return add_log_points( physdev, pts, cbPoints, PT_BEZIERTO ) != NULL;
}


/*************************************************************
 *           pathdrv_PolyBezier
 */
static BOOL pathdrv_PolyBezier( PHYSDEV dev, const POINT *pts, DWORD cbPoints )
{
    struct path_physdev *physdev = get_path_physdev( dev );
    BYTE *type = add_log_points( physdev, pts, cbPoints, PT_BEZIERTO );

    if (!type) return FALSE;
    type[0] = PT_MOVETO;
    return TRUE;
}


/*************************************************************
 *           pathdrv_PolyDraw
 */
static BOOL pathdrv_PolyDraw( PHYSDEV dev, const POINT *pts, const BYTE *types, DWORD cbPoints )
{
    struct path_physdev *physdev = get_path_physdev( dev );
    POINT lastmove, orig_pos;
    INT i;

    GetCurrentPositionEx( dev->hdc, &orig_pos );
    lastmove = orig_pos;

    for(i = physdev->path->count - 1; i >= 0; i--){
        if(physdev->path->flags[i] == PT_MOVETO){
            lastmove = physdev->path->points[i];
            DPtoLP(dev->hdc, &lastmove, 1);
            break;
        }
    }

    for(i = 0; i < cbPoints; i++)
    {
        switch (types[i])
        {
        case PT_MOVETO:
            MoveToEx( dev->hdc, pts[i].x, pts[i].y, NULL );
            break;
        case PT_LINETO:
        case PT_LINETO | PT_CLOSEFIGURE:
            LineTo( dev->hdc, pts[i].x, pts[i].y );
            break;
        case PT_BEZIERTO:
            if ((i + 2 < cbPoints) && (types[i + 1] == PT_BEZIERTO) &&
                (types[i + 2] & ~PT_CLOSEFIGURE) == PT_BEZIERTO)
            {
                PolyBezierTo( dev->hdc, &pts[i], 3 );
                i += 2;
                break;
            }
            /* fall through */
        default:
            if (i)  /* restore original position */
            {
                if (!(types[i - 1] & PT_CLOSEFIGURE)) lastmove = pts[i - 1];
                if (lastmove.x != orig_pos.x || lastmove.y != orig_pos.y)
                    MoveToEx( dev->hdc, orig_pos.x, orig_pos.y, NULL );
            }
            return FALSE;
        }

        if(types[i] & PT_CLOSEFIGURE){
            physdev->path->flags[physdev->path->count-1] |= PT_CLOSEFIGURE;
            MoveToEx( dev->hdc, lastmove.x, lastmove.y, NULL );
        }
    }

    return TRUE;
}


/*************************************************************
 *           pathdrv_Polyline
 */
static BOOL pathdrv_Polyline( PHYSDEV dev, const POINT *pts, INT cbPoints )
{
    struct path_physdev *physdev = get_path_physdev( dev );
    BYTE *type = add_log_points( physdev, pts, cbPoints, PT_LINETO );

    if (!type) return FALSE;
    if (cbPoints) type[0] = PT_MOVETO;
    return TRUE;
}


/*************************************************************
 *           pathdrv_PolylineTo
 */
static BOOL pathdrv_PolylineTo( PHYSDEV dev, const POINT *pts, INT cbPoints )
{
    struct path_physdev *physdev = get_path_physdev( dev );

    if (!start_new_stroke( physdev )) return FALSE;
    return add_log_points( physdev, pts, cbPoints, PT_LINETO ) != NULL;
}


/*************************************************************
 *           pathdrv_Polygon
 */
static BOOL pathdrv_Polygon( PHYSDEV dev, const POINT *pts, INT cbPoints )
{
    struct path_physdev *physdev = get_path_physdev( dev );
    BYTE *type = add_log_points( physdev, pts, cbPoints, PT_LINETO );

    if (!type) return FALSE;
    if (cbPoints) type[0] = PT_MOVETO;
    if (cbPoints > 1) type[cbPoints - 1] = PT_LINETO | PT_CLOSEFIGURE;
    return TRUE;
}


/*************************************************************
 *           pathdrv_PolyPolygon
 */
static BOOL pathdrv_PolyPolygon( PHYSDEV dev, const POINT* pts, const INT* counts, UINT polygons )
{
    struct path_physdev *physdev = get_path_physdev( dev );
    UINT poly;
    BYTE *type;

    for(poly = 0; poly < polygons; poly++) {
        type = add_log_points( physdev, pts, counts[poly], PT_LINETO );
        if (!type) return FALSE;
        type[0] = PT_MOVETO;
        /* win98 adds an extra line to close the figure for some reason */
        add_log_points( physdev, pts, 1, PT_LINETO | PT_CLOSEFIGURE );
        pts += counts[poly];
    }
    return TRUE;
}


/*************************************************************
 *           pathdrv_PolyPolyline
 */
static BOOL pathdrv_PolyPolyline( PHYSDEV dev, const POINT* pts, const DWORD* counts, DWORD polylines )
{
    struct path_physdev *physdev = get_path_physdev( dev );
    UINT poly, count;
    BYTE *type;

    for (poly = count = 0; poly < polylines; poly++) count += counts[poly];

    type = add_log_points( physdev, pts, count, PT_LINETO );
    if (!type) return FALSE;

    /* make the first point of each polyline a PT_MOVETO */
    for (poly = 0; poly < polylines; poly++, type += counts[poly]) *type = PT_MOVETO;
    return TRUE;
}


/**********************************************************************
 *      PATH_BezierTo
 *
 * internally used by PATH_add_outline
 */
static void PATH_BezierTo(struct gdi_path *pPath, POINT *lppt, INT n)
{
    if (n < 2) return;

    if (n == 2)
    {
        PATH_AddEntry(pPath, &lppt[1], PT_LINETO);
    }
    else if (n == 3)
    {
        PATH_AddEntry(pPath, &lppt[0], PT_BEZIERTO);
        PATH_AddEntry(pPath, &lppt[1], PT_BEZIERTO);
        PATH_AddEntry(pPath, &lppt[2], PT_BEZIERTO);
    }
    else
    {
        POINT pt[3];
        INT i = 0;

        pt[2] = lppt[0];
        n--;

        while (n > 2)
        {
            pt[0] = pt[2];
            pt[1] = lppt[i+1];
            pt[2].x = (lppt[i+2].x + lppt[i+1].x) / 2;
            pt[2].y = (lppt[i+2].y + lppt[i+1].y) / 2;
            PATH_BezierTo(pPath, pt, 3);
            n--;
            i++;
        }

        pt[0] = pt[2];
        pt[1] = lppt[i+1];
        pt[2] = lppt[i+2];
        PATH_BezierTo(pPath, pt, 3);
    }
}

static BOOL PATH_add_outline(struct path_physdev *physdev, INT x, INT y,
                             TTPOLYGONHEADER *header, DWORD size)
{
    TTPOLYGONHEADER *start;
    POINT pt;

    start = header;

    while ((char *)header < (char *)start + size)
    {
        TTPOLYCURVE *curve;

        if (header->dwType != TT_POLYGON_TYPE)
        {
            FIXME("Unknown header type %d\n", header->dwType);
            return FALSE;
        }

        pt.x = x + int_from_fixed(header->pfxStart.x);
        pt.y = y - int_from_fixed(header->pfxStart.y);
        PATH_AddEntry(physdev->path, &pt, PT_MOVETO);

        curve = (TTPOLYCURVE *)(header + 1);

        while ((char *)curve < (char *)header + header->cb)
        {
            /*TRACE("curve->wType %d\n", curve->wType);*/

            switch(curve->wType)
            {
            case TT_PRIM_LINE:
            {
                WORD i;

                for (i = 0; i < curve->cpfx; i++)
                {
                    pt.x = x + int_from_fixed(curve->apfx[i].x);
                    pt.y = y - int_from_fixed(curve->apfx[i].y);
                    PATH_AddEntry(physdev->path, &pt, PT_LINETO);
                }
                break;
            }

            case TT_PRIM_QSPLINE:
            case TT_PRIM_CSPLINE:
            {
                WORD i;
                POINTFX ptfx;
                POINT *pts = HeapAlloc(GetProcessHeap(), 0, (curve->cpfx + 1) * sizeof(POINT));

                if (!pts) return FALSE;

                ptfx = *(POINTFX *)((char *)curve - sizeof(POINTFX));

                pts[0].x = x + int_from_fixed(ptfx.x);
                pts[0].y = y - int_from_fixed(ptfx.y);

                for(i = 0; i < curve->cpfx; i++)
                {
                    pts[i + 1].x = x + int_from_fixed(curve->apfx[i].x);
                    pts[i + 1].y = y - int_from_fixed(curve->apfx[i].y);
                }

                PATH_BezierTo(physdev->path, pts, curve->cpfx + 1);

                HeapFree(GetProcessHeap(), 0, pts);
                break;
            }

            default:
                FIXME("Unknown curve type %04x\n", curve->wType);
                return FALSE;
            }

            curve = (TTPOLYCURVE *)&curve->apfx[curve->cpfx];
        }

        header = (TTPOLYGONHEADER *)((char *)header + header->cb);
    }

    return CloseFigure(physdev->dev.hdc);
}

/*************************************************************
 *           pathdrv_ExtTextOut
 */
static BOOL pathdrv_ExtTextOut( PHYSDEV dev, INT x, INT y, UINT flags, const RECT *lprc,
                                LPCWSTR str, UINT count, const INT *dx )
{
    struct path_physdev *physdev = get_path_physdev( dev );
    unsigned int idx;
    POINT offset = {0, 0};

    if (!count) return TRUE;

    for (idx = 0; idx < count; idx++)
    {
        static const MAT2 identity = { {0,1},{0,0},{0,0},{0,1} };
        GLYPHMETRICS gm;
        DWORD dwSize;
        void *outline;

        dwSize = GetGlyphOutlineW(dev->hdc, str[idx], GGO_GLYPH_INDEX | GGO_NATIVE,
                                  &gm, 0, NULL, &identity);
        if (dwSize == GDI_ERROR) return FALSE;

        /* add outline only if char is printable */
        if(dwSize)
        {
            outline = HeapAlloc(GetProcessHeap(), 0, dwSize);
            if (!outline) return FALSE;

            GetGlyphOutlineW(dev->hdc, str[idx], GGO_GLYPH_INDEX | GGO_NATIVE,
                             &gm, dwSize, outline, &identity);

            PATH_add_outline(physdev, x + offset.x, y + offset.y, outline, dwSize);

            HeapFree(GetProcessHeap(), 0, outline);
        }

        if (dx)
        {
            if(flags & ETO_PDY)
            {
                offset.x += dx[idx * 2];
                offset.y += dx[idx * 2 + 1];
            }
            else
                offset.x += dx[idx];
        }
        else
        {
            offset.x += gm.gmCellIncX;
            offset.y += gm.gmCellIncY;
        }
    }
    return TRUE;
}


/*************************************************************
 *           pathdrv_CloseFigure
 */
static BOOL pathdrv_CloseFigure( PHYSDEV dev )
{
    struct path_physdev *physdev = get_path_physdev( dev );

    /* Set PT_CLOSEFIGURE on the last entry and start a new stroke */
    /* It is not necessary to draw a line, PT_CLOSEFIGURE is a virtual closing line itself */
    if (physdev->path->count)
        physdev->path->flags[physdev->path->count - 1] |= PT_CLOSEFIGURE;
    return TRUE;
}


/*******************************************************************
 *      FlattenPath [GDI32.@]
 *
 *
 */
BOOL WINAPI FlattenPath(HDC hdc)
{
    BOOL ret = FALSE;
    DC *dc = get_dc_ptr( hdc );

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pFlattenPath );
        ret = physdev->funcs->pFlattenPath( physdev );
        release_dc_ptr( dc );
    }
    return ret;
}


static BOOL PATH_StrokePath( HDC hdc, const struct gdi_path *pPath )
{
    INT i, nLinePts, nAlloc;
    POINT *pLinePts;
    POINT ptViewportOrg, ptWindowOrg;
    SIZE szViewportExt, szWindowExt;
    DWORD mapMode, graphicsMode;
    XFORM xform;
    BOOL ret = TRUE;

    /* Save the mapping mode info */
    mapMode=GetMapMode(hdc);
    GetViewportExtEx(hdc, &szViewportExt);
    GetViewportOrgEx(hdc, &ptViewportOrg);
    GetWindowExtEx(hdc, &szWindowExt);
    GetWindowOrgEx(hdc, &ptWindowOrg);
    GetWorldTransform(hdc, &xform);

    /* Set MM_TEXT */
    SetMapMode(hdc, MM_TEXT);
    SetViewportOrgEx(hdc, 0, 0, NULL);
    SetWindowOrgEx(hdc, 0, 0, NULL);
    graphicsMode=GetGraphicsMode(hdc);
    SetGraphicsMode(hdc, GM_ADVANCED);
    ModifyWorldTransform(hdc, &xform, MWT_IDENTITY);
    SetGraphicsMode(hdc, graphicsMode);

    /* Allocate enough memory for the worst case without beziers (one PT_MOVETO
     * and the rest PT_LINETO with PT_CLOSEFIGURE at the end) plus some buffer 
     * space in case we get one to keep the number of reallocations small. */
    nAlloc = pPath->count + 1 + 300;
    pLinePts = HeapAlloc(GetProcessHeap(), 0, nAlloc * sizeof(POINT));
    nLinePts = 0;
    
    for(i = 0; i < pPath->count; i++) {
        if((i == 0 || (pPath->flags[i-1] & PT_CLOSEFIGURE)) &&
	   (pPath->flags[i] != PT_MOVETO)) {
	    ERR("Expected PT_MOVETO %s, got path flag %d\n", 
	        i == 0 ? "as first point" : "after PT_CLOSEFIGURE",
		pPath->flags[i]);
	    ret = FALSE;
	    goto end;
	}
        switch(pPath->flags[i]) {
	case PT_MOVETO:
            TRACE("Got PT_MOVETO (%d, %d)\n",
		  pPath->points[i].x, pPath->points[i].y);
	    if(nLinePts >= 2)
	        Polyline(hdc, pLinePts, nLinePts);
	    nLinePts = 0;
	    pLinePts[nLinePts++] = pPath->points[i];
	    break;
	case PT_LINETO:
	case (PT_LINETO | PT_CLOSEFIGURE):
            TRACE("Got PT_LINETO (%d, %d)\n",
		  pPath->points[i].x, pPath->points[i].y);
	    pLinePts[nLinePts++] = pPath->points[i];
	    break;
	case PT_BEZIERTO:
	    TRACE("Got PT_BEZIERTO\n");
	    if(pPath->flags[i+1] != PT_BEZIERTO ||
	       (pPath->flags[i+2] & ~PT_CLOSEFIGURE) != PT_BEZIERTO) {
	        ERR("Path didn't contain 3 successive PT_BEZIERTOs\n");
		ret = FALSE;
		goto end;
	    } else {
	        INT nBzrPts, nMinAlloc;
	        POINT *pBzrPts = GDI_Bezier(&pPath->points[i-1], 4, &nBzrPts);
		/* Make sure we have allocated enough memory for the lines of 
		 * this bezier and the rest of the path, assuming we won't get
		 * another one (since we won't reallocate again then). */
		nMinAlloc = nLinePts + (pPath->count - i) + nBzrPts;
		if(nAlloc < nMinAlloc)
		{
		    nAlloc = nMinAlloc * 2;
		    pLinePts = HeapReAlloc(GetProcessHeap(), 0, pLinePts,
		                           nAlloc * sizeof(POINT));
		}
		memcpy(&pLinePts[nLinePts], &pBzrPts[1],
		       (nBzrPts - 1) * sizeof(POINT));
		nLinePts += nBzrPts - 1;
		HeapFree(GetProcessHeap(), 0, pBzrPts);
		i += 2;
	    }
	    break;
	default:
	    ERR("Got path flag %d\n", pPath->flags[i]);
	    ret = FALSE;
	    goto end;
	}
	if(pPath->flags[i] & PT_CLOSEFIGURE)
	    pLinePts[nLinePts++] = pLinePts[0];
    }
    if(nLinePts >= 2)
        Polyline(hdc, pLinePts, nLinePts);

 end:
    HeapFree(GetProcessHeap(), 0, pLinePts);

    /* Restore the old mapping mode */
    SetMapMode(hdc, mapMode);
    SetWindowExtEx(hdc, szWindowExt.cx, szWindowExt.cy, NULL);
    SetWindowOrgEx(hdc, ptWindowOrg.x, ptWindowOrg.y, NULL);
    SetViewportExtEx(hdc, szViewportExt.cx, szViewportExt.cy, NULL);
    SetViewportOrgEx(hdc, ptViewportOrg.x, ptViewportOrg.y, NULL);

    /* Go to GM_ADVANCED temporarily to restore the world transform */
    graphicsMode=GetGraphicsMode(hdc);
    SetGraphicsMode(hdc, GM_ADVANCED);
    SetWorldTransform(hdc, &xform);
    SetGraphicsMode(hdc, graphicsMode);

    /* If we've moved the current point then get its new position
       which will be in device (MM_TEXT) co-ords, convert it to
       logical co-ords and re-set it.  This basically updates
       dc->CurPosX|Y so that their values are in the correct mapping
       mode.
    */
    if(i > 0) {
        POINT pt;
        GetCurrentPositionEx(hdc, &pt);
        DPtoLP(hdc, &pt, 1);
        MoveToEx(hdc, pt.x, pt.y, NULL);
    }

    return ret;
}

#define round(x) ((int)((x)>0?(x)+0.5:(x)-0.5))

static struct gdi_path *PATH_WidenPath(DC *dc)
{
    INT i, j, numStrokes, penWidth, penWidthIn, penWidthOut, size, penStyle;
    struct gdi_path *flat_path, *pNewPath, **pStrokes = NULL, *pUpPath, *pDownPath;
    EXTLOGPEN *elp;
    DWORD obj_type, joint, endcap, penType;

    size = GetObjectW( dc->hPen, 0, NULL );
    if (!size) {
        SetLastError(ERROR_CAN_NOT_COMPLETE);
        return NULL;
    }

    elp = HeapAlloc( GetProcessHeap(), 0, size );
    GetObjectW( dc->hPen, size, elp );

    obj_type = GetObjectType(dc->hPen);
    if(obj_type == OBJ_PEN) {
        penStyle = ((LOGPEN*)elp)->lopnStyle;
    }
    else if(obj_type == OBJ_EXTPEN) {
        penStyle = elp->elpPenStyle;
    }
    else {
        SetLastError(ERROR_CAN_NOT_COMPLETE);
        HeapFree( GetProcessHeap(), 0, elp );
        return NULL;
    }

    penWidth = elp->elpWidth;
    HeapFree( GetProcessHeap(), 0, elp );

    endcap = (PS_ENDCAP_MASK & penStyle);
    joint = (PS_JOIN_MASK & penStyle);
    penType = (PS_TYPE_MASK & penStyle);

    /* The function cannot apply to cosmetic pens */
    if(obj_type == OBJ_EXTPEN && penType == PS_COSMETIC) {
        SetLastError(ERROR_CAN_NOT_COMPLETE);
        return NULL;
    }

    if (!(flat_path = PATH_FlattenPath( dc->path ))) return NULL;

    penWidthIn = penWidth / 2;
    penWidthOut = penWidth / 2;
    if(penWidthIn + penWidthOut < penWidth)
        penWidthOut++;

    numStrokes = 0;

    for(i = 0, j = 0; i < flat_path->count; i++, j++) {
        POINT point;
        if((i == 0 || (flat_path->flags[i-1] & PT_CLOSEFIGURE)) &&
            (flat_path->flags[i] != PT_MOVETO)) {
            ERR("Expected PT_MOVETO %s, got path flag %c\n",
                i == 0 ? "as first point" : "after PT_CLOSEFIGURE",
                flat_path->flags[i]);
            free_gdi_path( flat_path );
            return NULL;
        }
        switch(flat_path->flags[i]) {
            case PT_MOVETO:
                numStrokes++;
                j = 0;
                if(numStrokes == 1)
                    pStrokes = HeapAlloc(GetProcessHeap(), 0, sizeof(*pStrokes));
                else
                    pStrokes = HeapReAlloc(GetProcessHeap(), 0, pStrokes, numStrokes * sizeof(*pStrokes));
                if(!pStrokes) return NULL;
                pStrokes[numStrokes - 1] = alloc_gdi_path(0);
                /* fall through */
            case PT_LINETO:
            case (PT_LINETO | PT_CLOSEFIGURE):
                point.x = flat_path->points[i].x;
                point.y = flat_path->points[i].y;
                PATH_AddEntry(pStrokes[numStrokes - 1], &point, flat_path->flags[i]);
                break;
            case PT_BEZIERTO:
                /* should never happen because of the FlattenPath call */
                ERR("Should never happen\n");
                break;
            default:
                ERR("Got path flag %c\n", flat_path->flags[i]);
                return NULL;
        }
    }

    pNewPath = alloc_gdi_path( flat_path->count );

    for(i = 0; i < numStrokes; i++) {
        pUpPath = alloc_gdi_path( pStrokes[i]->count );
        pDownPath = alloc_gdi_path( pStrokes[i]->count );

        for(j = 0; j < pStrokes[i]->count; j++) {
            /* Beginning or end of the path if not closed */
            if((!(pStrokes[i]->flags[pStrokes[i]->count - 1] & PT_CLOSEFIGURE)) && (j == 0 || j == pStrokes[i]->count - 1) ) {
                /* Compute segment angle */
                double xo, yo, xa, ya, theta;
                POINT pt;
                FLOAT_POINT corners[2];
                if(j == 0) {
                    xo = pStrokes[i]->points[j].x;
                    yo = pStrokes[i]->points[j].y;
                    xa = pStrokes[i]->points[1].x;
                    ya = pStrokes[i]->points[1].y;
                }
                else {
                    xa = pStrokes[i]->points[j - 1].x;
                    ya = pStrokes[i]->points[j - 1].y;
                    xo = pStrokes[i]->points[j].x;
                    yo = pStrokes[i]->points[j].y;
                }
                theta = atan2( ya - yo, xa - xo );
                switch(endcap) {
                    case PS_ENDCAP_SQUARE :
                        pt.x = xo + round(sqrt(2) * penWidthOut * cos(M_PI_4 + theta));
                        pt.y = yo + round(sqrt(2) * penWidthOut * sin(M_PI_4 + theta));
                        PATH_AddEntry(pUpPath, &pt, (j == 0 ? PT_MOVETO : PT_LINETO) );
                        pt.x = xo + round(sqrt(2) * penWidthIn * cos(- M_PI_4 + theta));
                        pt.y = yo + round(sqrt(2) * penWidthIn * sin(- M_PI_4 + theta));
                        PATH_AddEntry(pUpPath, &pt, PT_LINETO);
                        break;
                    case PS_ENDCAP_FLAT :
                        pt.x = xo + round( penWidthOut * cos(theta + M_PI_2) );
                        pt.y = yo + round( penWidthOut * sin(theta + M_PI_2) );
                        PATH_AddEntry(pUpPath, &pt, (j == 0 ? PT_MOVETO : PT_LINETO));
                        pt.x = xo - round( penWidthIn * cos(theta + M_PI_2) );
                        pt.y = yo - round( penWidthIn * sin(theta + M_PI_2) );
                        PATH_AddEntry(pUpPath, &pt, PT_LINETO);
                        break;
                    case PS_ENDCAP_ROUND :
                    default :
                        corners[0].x = xo - penWidthIn;
                        corners[0].y = yo - penWidthIn;
                        corners[1].x = xo + penWidthOut;
                        corners[1].y = yo + penWidthOut;
                        PATH_DoArcPart(pUpPath ,corners, theta + M_PI_2 , theta + 3 * M_PI_4, (j == 0 ? PT_MOVETO : FALSE));
                        PATH_DoArcPart(pUpPath ,corners, theta + 3 * M_PI_4 , theta + M_PI, FALSE);
                        PATH_DoArcPart(pUpPath ,corners, theta + M_PI, theta +  5 * M_PI_4, FALSE);
                        PATH_DoArcPart(pUpPath ,corners, theta + 5 * M_PI_4 , theta + 3 * M_PI_2, FALSE);
                        break;
                }
            }
            /* Corpse of the path */
            else {
                /* Compute angle */
                INT previous, next;
                double xa, ya, xb, yb, xo, yo;
                double alpha, theta, miterWidth;
                DWORD _joint = joint;
                POINT pt;
		struct gdi_path *pInsidePath, *pOutsidePath;
                if(j > 0 && j < pStrokes[i]->count - 1) {
                    previous = j - 1;
                    next = j + 1;
                }
                else if (j == 0) {
                    previous = pStrokes[i]->count - 1;
                    next = j + 1;
                }
                else {
                    previous = j - 1;
                    next = 0;
                }
                xo = pStrokes[i]->points[j].x;
                yo = pStrokes[i]->points[j].y;
                xa = pStrokes[i]->points[previous].x;
                ya = pStrokes[i]->points[previous].y;
                xb = pStrokes[i]->points[next].x;
                yb = pStrokes[i]->points[next].y;
                theta = atan2( yo - ya, xo - xa );
                alpha = atan2( yb - yo, xb - xo ) - theta;
                if (alpha > 0) alpha -= M_PI;
                else alpha += M_PI;
                if(_joint == PS_JOIN_MITER && dc->miterLimit < fabs(1 / sin(alpha/2))) {
                    _joint = PS_JOIN_BEVEL;
                }
                if(alpha > 0) {
                    pInsidePath = pUpPath;
                    pOutsidePath = pDownPath;
                }
                else if(alpha < 0) {
                    pInsidePath = pDownPath;
                    pOutsidePath = pUpPath;
                }
                else {
                    continue;
                }
                /* Inside angle points */
                if(alpha > 0) {
                    pt.x = xo - round( penWidthIn * cos(theta + M_PI_2) );
                    pt.y = yo - round( penWidthIn * sin(theta + M_PI_2) );
                }
                else {
                    pt.x = xo + round( penWidthIn * cos(theta + M_PI_2) );
                    pt.y = yo + round( penWidthIn * sin(theta + M_PI_2) );
                }
                PATH_AddEntry(pInsidePath, &pt, PT_LINETO);
                if(alpha > 0) {
                    pt.x = xo + round( penWidthIn * cos(M_PI_2 + alpha + theta) );
                    pt.y = yo + round( penWidthIn * sin(M_PI_2 + alpha + theta) );
                }
                else {
                    pt.x = xo - round( penWidthIn * cos(M_PI_2 + alpha + theta) );
                    pt.y = yo - round( penWidthIn * sin(M_PI_2 + alpha + theta) );
                }
                PATH_AddEntry(pInsidePath, &pt, PT_LINETO);
                /* Outside angle point */
                switch(_joint) {
                     case PS_JOIN_MITER :
                        miterWidth = fabs(penWidthOut / cos(M_PI_2 - fabs(alpha) / 2));
                        pt.x = xo + round( miterWidth * cos(theta + alpha / 2) );
                        pt.y = yo + round( miterWidth * sin(theta + alpha / 2) );
                        PATH_AddEntry(pOutsidePath, &pt, PT_LINETO);
                        break;
                    case PS_JOIN_BEVEL :
                        if(alpha > 0) {
                            pt.x = xo + round( penWidthOut * cos(theta + M_PI_2) );
                            pt.y = yo + round( penWidthOut * sin(theta + M_PI_2) );
                        }
                        else {
                            pt.x = xo - round( penWidthOut * cos(theta + M_PI_2) );
                            pt.y = yo - round( penWidthOut * sin(theta + M_PI_2) );
                        }
                        PATH_AddEntry(pOutsidePath, &pt, PT_LINETO);
                        if(alpha > 0) {
                            pt.x = xo - round( penWidthOut * cos(M_PI_2 + alpha + theta) );
                            pt.y = yo - round( penWidthOut * sin(M_PI_2 + alpha + theta) );
                        }
                        else {
                            pt.x = xo + round( penWidthOut * cos(M_PI_2 + alpha + theta) );
                            pt.y = yo + round( penWidthOut * sin(M_PI_2 + alpha + theta) );
                        }
                        PATH_AddEntry(pOutsidePath, &pt, PT_LINETO);
                        break;
                    case PS_JOIN_ROUND :
                    default :
                        if(alpha > 0) {
                            pt.x = xo + round( penWidthOut * cos(theta + M_PI_2) );
                            pt.y = yo + round( penWidthOut * sin(theta + M_PI_2) );
                        }
                        else {
                            pt.x = xo - round( penWidthOut * cos(theta + M_PI_2) );
                            pt.y = yo - round( penWidthOut * sin(theta + M_PI_2) );
                        }
                        PATH_AddEntry(pOutsidePath, &pt, PT_BEZIERTO);
                        pt.x = xo + round( penWidthOut * cos(theta + alpha / 2) );
                        pt.y = yo + round( penWidthOut * sin(theta + alpha / 2) );
                        PATH_AddEntry(pOutsidePath, &pt, PT_BEZIERTO);
                        if(alpha > 0) {
                            pt.x = xo - round( penWidthOut * cos(M_PI_2 + alpha + theta) );
                            pt.y = yo - round( penWidthOut * sin(M_PI_2 + alpha + theta) );
                        }
                        else {
                            pt.x = xo + round( penWidthOut * cos(M_PI_2 + alpha + theta) );
                            pt.y = yo + round( penWidthOut * sin(M_PI_2 + alpha + theta) );
                        }
                        PATH_AddEntry(pOutsidePath, &pt, PT_BEZIERTO);
                        break;
                }
            }
        }
        for(j = 0; j < pUpPath->count; j++) {
            POINT pt;
            pt.x = pUpPath->points[j].x;
            pt.y = pUpPath->points[j].y;
            PATH_AddEntry(pNewPath, &pt, (j == 0 ? PT_MOVETO : PT_LINETO));
        }
        for(j = 0; j < pDownPath->count; j++) {
            POINT pt;
            pt.x = pDownPath->points[pDownPath->count - j - 1].x;
            pt.y = pDownPath->points[pDownPath->count - j - 1].y;
            PATH_AddEntry(pNewPath, &pt, ( (j == 0 && (pStrokes[i]->flags[pStrokes[i]->count - 1] & PT_CLOSEFIGURE)) ? PT_MOVETO : PT_LINETO));
        }

        free_gdi_path( pStrokes[i] );
        free_gdi_path( pUpPath );
        free_gdi_path( pDownPath );
    }
    HeapFree(GetProcessHeap(), 0, pStrokes);
    free_gdi_path( flat_path );
    return pNewPath;
}


/*******************************************************************
 *      StrokeAndFillPath [GDI32.@]
 *
 *
 */
BOOL WINAPI StrokeAndFillPath(HDC hdc)
{
    BOOL ret = FALSE;
    DC *dc = get_dc_ptr( hdc );

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pStrokeAndFillPath );
        ret = physdev->funcs->pStrokeAndFillPath( physdev );
        release_dc_ptr( dc );
    }
    return ret;
}


/*******************************************************************
 *      StrokePath [GDI32.@]
 *
 *
 */
BOOL WINAPI StrokePath(HDC hdc)
{
    BOOL ret = FALSE;
    DC *dc = get_dc_ptr( hdc );

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pStrokePath );
        ret = physdev->funcs->pStrokePath( physdev );
        release_dc_ptr( dc );
    }
    return ret;
}


/*******************************************************************
 *      WidenPath [GDI32.@]
 *
 *
 */
BOOL WINAPI WidenPath(HDC hdc)
{
    BOOL ret = FALSE;
    DC *dc = get_dc_ptr( hdc );

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pWidenPath );
        ret = physdev->funcs->pWidenPath( physdev );
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           null driver fallback implementations
 */

BOOL nulldrv_BeginPath( PHYSDEV dev )
{
    DC *dc = get_nulldrv_dc( dev );
    struct path_physdev *physdev;
    struct gdi_path *path = alloc_gdi_path(0);

    if (!path) return FALSE;
    if (!path_driver.pCreateDC( &dc->physDev, NULL, NULL, NULL, NULL ))
    {
        free_gdi_path( path );
        return FALSE;
    }
    physdev = get_path_physdev( dc->physDev );
    physdev->path = path;
    if (dc->path) free_gdi_path( dc->path );
    dc->path = NULL;
    return TRUE;
}

BOOL nulldrv_EndPath( PHYSDEV dev )
{
    SetLastError( ERROR_CAN_NOT_COMPLETE );
    return FALSE;
}

BOOL nulldrv_AbortPath( PHYSDEV dev )
{
    DC *dc = get_nulldrv_dc( dev );

    if (dc->path) free_gdi_path( dc->path );
    dc->path = NULL;
    return TRUE;
}

BOOL nulldrv_CloseFigure( PHYSDEV dev )
{
    SetLastError( ERROR_CAN_NOT_COMPLETE );
    return FALSE;
}

BOOL nulldrv_SelectClipPath( PHYSDEV dev, INT mode )
{
    BOOL ret;
    HRGN hrgn;
    DC *dc = get_nulldrv_dc( dev );

    if (!dc->path)
    {
        SetLastError( ERROR_CAN_NOT_COMPLETE );
        return FALSE;
    }
    if (!(hrgn = PATH_PathToRegion( dc->path, GetPolyFillMode(dev->hdc)))) return FALSE;
    ret = ExtSelectClipRgn( dev->hdc, hrgn, mode ) != ERROR;
    if (ret)
    {
        free_gdi_path( dc->path );
        dc->path = NULL;
    }
    /* FIXME: Should this function delete the path even if it failed? */
    DeleteObject( hrgn );
    return ret;
}

BOOL nulldrv_FillPath( PHYSDEV dev )
{
    DC *dc = get_nulldrv_dc( dev );

    if (!dc->path)
    {
        SetLastError( ERROR_CAN_NOT_COMPLETE );
        return FALSE;
    }
    if (!PATH_FillPath( dev->hdc, dc->path )) return FALSE;
    /* FIXME: Should the path be emptied even if conversion failed? */
    free_gdi_path( dc->path );
    dc->path = NULL;
    return TRUE;
}

BOOL nulldrv_StrokeAndFillPath( PHYSDEV dev )
{
    DC *dc = get_nulldrv_dc( dev );

    if (!dc->path)
    {
        SetLastError( ERROR_CAN_NOT_COMPLETE );
        return FALSE;
    }
    if (!PATH_FillPath( dev->hdc, dc->path )) return FALSE;
    if (!PATH_StrokePath( dev->hdc, dc->path )) return FALSE;
    free_gdi_path( dc->path );
    dc->path = NULL;
    return TRUE;
}

BOOL nulldrv_StrokePath( PHYSDEV dev )
{
    DC *dc = get_nulldrv_dc( dev );

    if (!dc->path)
    {
        SetLastError( ERROR_CAN_NOT_COMPLETE );
        return FALSE;
    }
    if (!PATH_StrokePath( dev->hdc, dc->path )) return FALSE;
    free_gdi_path( dc->path );
    dc->path = NULL;
    return TRUE;
}

BOOL nulldrv_FlattenPath( PHYSDEV dev )
{
    DC *dc = get_nulldrv_dc( dev );
    struct gdi_path *path;

    if (!dc->path)
    {
        SetLastError( ERROR_CAN_NOT_COMPLETE );
        return FALSE;
    }
    if (!(path = PATH_FlattenPath( dc->path ))) return FALSE;
    free_gdi_path( dc->path );
    dc->path = path;
    return TRUE;
}

BOOL nulldrv_WidenPath( PHYSDEV dev )
{
    DC *dc = get_nulldrv_dc( dev );
    struct gdi_path *path;

    if (!dc->path)
    {
        SetLastError( ERROR_CAN_NOT_COMPLETE );
        return FALSE;
    }
    if (!(path = PATH_WidenPath( dc ))) return FALSE;
    free_gdi_path( dc->path );
    dc->path = path;
    return TRUE;
}

const struct gdi_dc_funcs path_driver =
{
    NULL,                               /* pAbortDoc */
    pathdrv_AbortPath,                  /* pAbortPath */
    NULL,                               /* pAlphaBlend */
    pathdrv_AngleArc,                   /* pAngleArc */
    pathdrv_Arc,                        /* pArc */
    pathdrv_ArcTo,                      /* pArcTo */
    pathdrv_BeginPath,                  /* pBeginPath */
    NULL,                               /* pBlendImage */
    pathdrv_Chord,                      /* pChord */
    pathdrv_CloseFigure,                /* pCloseFigure */
    NULL,                               /* pCreateCompatibleDC */
    pathdrv_CreateDC,                   /* pCreateDC */
    pathdrv_DeleteDC,                   /* pDeleteDC */
    NULL,                               /* pDeleteObject */
    NULL,                               /* pDescribePixelFormat */
    NULL,                               /* pDeviceCapabilities */
    pathdrv_Ellipse,                    /* pEllipse */
    NULL,                               /* pEndDoc */
    NULL,                               /* pEndPage */
    pathdrv_EndPath,                    /* pEndPath */
    NULL,                               /* pEnumFonts */
    NULL,                               /* pEnumICMProfiles */
    NULL,                               /* pExcludeClipRect */
    NULL,                               /* pExtDeviceMode */
    NULL,                               /* pExtEscape */
    NULL,                               /* pExtFloodFill */
    NULL,                               /* pExtSelectClipRgn */
    pathdrv_ExtTextOut,                 /* pExtTextOut */
    NULL,                               /* pFillPath */
    NULL,                               /* pFillRgn */
    NULL,                               /* pFlattenPath */
    NULL,                               /* pFontIsLinked */
    NULL,                               /* pFrameRgn */
    NULL,                               /* pGdiComment */
    NULL,                               /* pGdiRealizationInfo */
    NULL,                               /* pGetBoundsRect */
    NULL,                               /* pGetCharABCWidths */
    NULL,                               /* pGetCharABCWidthsI */
    NULL,                               /* pGetCharWidth */
    NULL,                               /* pGetDeviceCaps */
    NULL,                               /* pGetDeviceGammaRamp */
    NULL,                               /* pGetFontData */
    NULL,                               /* pGetFontUnicodeRanges */
    NULL,                               /* pGetGlyphIndices */
    NULL,                               /* pGetGlyphOutline */
    NULL,                               /* pGetICMProfile */
    NULL,                               /* pGetImage */
    NULL,                               /* pGetKerningPairs */
    NULL,                               /* pGetNearestColor */
    NULL,                               /* pGetOutlineTextMetrics */
    NULL,                               /* pGetPixel */
    NULL,                               /* pGetSystemPaletteEntries */
    NULL,                               /* pGetTextCharsetInfo */
    NULL,                               /* pGetTextExtentExPoint */
    NULL,                               /* pGetTextExtentExPointI */
    NULL,                               /* pGetTextFace */
    NULL,                               /* pGetTextMetrics */
    NULL,                               /* pGradientFill */
    NULL,                               /* pIntersectClipRect */
    NULL,                               /* pInvertRgn */
    pathdrv_LineTo,                     /* pLineTo */
    NULL,                               /* pModifyWorldTransform */
    pathdrv_MoveTo,                     /* pMoveTo */
    NULL,                               /* pOffsetClipRgn */
    NULL,                               /* pOffsetViewportOrg */
    NULL,                               /* pOffsetWindowOrg */
    NULL,                               /* pPaintRgn */
    NULL,                               /* pPatBlt */
    pathdrv_Pie,                        /* pPie */
    pathdrv_PolyBezier,                 /* pPolyBezier */
    pathdrv_PolyBezierTo,               /* pPolyBezierTo */
    pathdrv_PolyDraw,                   /* pPolyDraw */
    pathdrv_PolyPolygon,                /* pPolyPolygon */
    pathdrv_PolyPolyline,               /* pPolyPolyline */
    pathdrv_Polygon,                    /* pPolygon */
    pathdrv_Polyline,                   /* pPolyline */
    pathdrv_PolylineTo,                 /* pPolylineTo */
    NULL,                               /* pPutImage */
    NULL,                               /* pRealizeDefaultPalette */
    NULL,                               /* pRealizePalette */
    pathdrv_Rectangle,                  /* pRectangle */
    NULL,                               /* pResetDC */
    NULL,                               /* pRestoreDC */
    pathdrv_RoundRect,                  /* pRoundRect */
    NULL,                               /* pSaveDC */
    NULL,                               /* pScaleViewportExt */
    NULL,                               /* pScaleWindowExt */
    NULL,                               /* pSelectBitmap */
    NULL,                               /* pSelectBrush */
    NULL,                               /* pSelectClipPath */
    NULL,                               /* pSelectFont */
    NULL,                               /* pSelectPalette */
    NULL,                               /* pSelectPen */
    NULL,                               /* pSetArcDirection */
    NULL,                               /* pSetBkColor */
    NULL,                               /* pSetBkMode */
    NULL,                               /* pSetDCBrushColor */
    NULL,                               /* pSetDCPenColor */
    NULL,                               /* pSetDIBColorTable */
    NULL,                               /* pSetDIBitsToDevice */
    NULL,                               /* pSetDeviceClipping */
    NULL,                               /* pSetDeviceGammaRamp */
    NULL,                               /* pSetLayout */
    NULL,                               /* pSetMapMode */
    NULL,                               /* pSetMapperFlags */
    NULL,                               /* pSetPixel */
    NULL,                               /* pSetPixelFormat */
    NULL,                               /* pSetPolyFillMode */
    NULL,                               /* pSetROP2 */
    NULL,                               /* pSetRelAbs */
    NULL,                               /* pSetStretchBltMode */
    NULL,                               /* pSetTextAlign */
    NULL,                               /* pSetTextCharacterExtra */
    NULL,                               /* pSetTextColor */
    NULL,                               /* pSetTextJustification */
    NULL,                               /* pSetViewportExt */
    NULL,                               /* pSetViewportOrg */
    NULL,                               /* pSetWindowExt */
    NULL,                               /* pSetWindowOrg */
    NULL,                               /* pSetWorldTransform */
    NULL,                               /* pStartDoc */
    NULL,                               /* pStartPage */
    NULL,                               /* pStretchBlt */
    NULL,                               /* pStretchDIBits */
    NULL,                               /* pStrokeAndFillPath */
    NULL,                               /* pStrokePath */
    NULL,                               /* pSwapBuffers */
    NULL,                               /* pUnrealizePalette */
    NULL,                               /* pWidenPath */
    NULL,                               /* wine_get_wgl_driver */
    GDI_PRIORITY_PATH_DRV               /* priority */
};
