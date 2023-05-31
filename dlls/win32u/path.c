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

#if 0
#pragma makedep unix
#endif

#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winerror.h"

#include "ntgdi_private.h"
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
    POINT        pos;         /* current cursor position */
    POINT        points_buf[NUM_ENTRIES_INITIAL];
    BYTE         flags_buf[NUM_ENTRIES_INITIAL];
};

struct path_physdev
{
    struct gdi_physdev dev;
    struct gdi_path   *path;
};

static inline struct path_physdev *get_path_physdev( PHYSDEV dev )
{
    return CONTAINING_RECORD( dev, struct path_physdev, dev );
}

void free_gdi_path( struct gdi_path *path )
{
    if (path->points != path->points_buf)
        free( path->points );
    free( path );
}

static struct gdi_path *alloc_gdi_path( int count )
{
    struct gdi_path *path = malloc( sizeof(*path) );

    if (!path)
    {
        RtlSetLastWin32Error( ERROR_NOT_ENOUGH_MEMORY );
        return NULL;
    }
    count = max( NUM_ENTRIES_INITIAL, count );
    if (count > NUM_ENTRIES_INITIAL)
    {
        path->points = malloc( count * (sizeof(path->points[0]) + sizeof(path->flags[0])) );
        if (!path->points)
        {
            free( path );
            RtlSetLastWin32Error( ERROR_NOT_ENOUGH_MEMORY );
            return NULL;
        }
        path->flags = (BYTE *)(path->points + count);
    }
    else
    {
        path->points = path->points_buf;
        path->flags = path->flags_buf;
    }
    path->count = 0;
    path->allocated = count;
    path->newStroke = TRUE;
    path->pos.x = path->pos.y = 0;
    return path;
}

static struct gdi_path *copy_gdi_path( const struct gdi_path *src_path )
{
    struct gdi_path *path = alloc_gdi_path( src_path->count );

    if (!path) return NULL;

    path->count = src_path->count;
    path->newStroke = src_path->newStroke;
    path->pos = src_path->pos;
    memcpy( path->points, src_path->points, path->count * sizeof(*path->points) );
    memcpy( path->flags, src_path->flags, path->count * sizeof(*path->flags) );
    return path;
}

/* Performs a world-to-viewport transformation on the specified point (which
 * is in floating point format).
 */
static inline void INTERNAL_LPTODP_FLOAT( DC *dc, FLOAT_POINT *point, int count )
{
    double x, y;

    while (count--)
    {
        x = point->x;
        y = point->y;
        point->x = x * dc->xformWorld2Vport.eM11 + y * dc->xformWorld2Vport.eM21 + dc->xformWorld2Vport.eDx;
        point->y = x * dc->xformWorld2Vport.eM12 + y * dc->xformWorld2Vport.eM22 + dc->xformWorld2Vport.eDy;
        point++;
    }
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
static BOOL PATH_ReserveEntries(struct gdi_path *path, INT count)
{
    POINT *pts_new;
    int size;

    assert(count>=0);

    /* Do we have to allocate more memory? */
    if (count > path->allocated)
    {
        /* Find number of entries to allocate. We let the size of the array
         * grow exponentially, since that will guarantee linear time
         * complexity. */
        count = max( path->allocated * 2, count );
        size = count * (sizeof(path->points[0]) + sizeof(path->flags[0]));

        if (path->points == path->points_buf)
        {
            pts_new = malloc( size );
            if (!pts_new) return FALSE;
            memcpy( pts_new, path->points, path->count * sizeof(path->points[0]) );
            memcpy( pts_new + count, path->flags, path->count * sizeof(path->flags[0]) );
        }
        else
        {
            pts_new = realloc( path->points, size );
            if (!pts_new) return FALSE;
            memmove( pts_new + count, pts_new + path->allocated, path->count * sizeof(path->flags[0]) );
        }

        path->points = pts_new;
        path->flags = (BYTE *)(pts_new + count);
        path->allocated = count;
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
    TRACE("(%d,%d) - %d\n", (int)pPoint->x, (int)pPoint->y, flags);

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
static BYTE *add_log_points( DC *dc, struct gdi_path *path, const POINT *points,
                             DWORD count, BYTE type )
{
    BYTE *ret;

    if (!PATH_ReserveEntries( path, path->count + count )) return NULL;

    ret = &path->flags[path->count];
    memcpy( &path->points[path->count], points, count * sizeof(*points) );
    lp_to_dp( dc, &path->points[path->count], count );
    memset( ret, type, count );
    path->count += count;
    return ret;
}

/* add a number of points that are already in device coords */
/* return a pointer to the first type byte so it can be fixed up if necessary */
static BYTE *add_points( struct gdi_path *path, const POINT *points, DWORD count, BYTE type )
{
    BYTE *ret;

    if (!PATH_ReserveEntries( path, path->count + count )) return NULL;

    ret = &path->flags[path->count];
    memcpy( &path->points[path->count], points, count * sizeof(*points) );
    memset( ret, type, count );
    path->count += count;
    return ret;
}

/* reverse the order of an array of points */
static void reverse_points( POINT *points, UINT count )
{
    UINT i;
    for (i = 0; i < count / 2; i++)
    {
        POINT pt = points[i];
        points[i] = points[count - i - 1];
        points[count - i - 1] = pt;
    }
}

/* start a new path stroke if necessary */
static BOOL start_new_stroke( struct gdi_path *path )
{
    if (!path->newStroke && path->count &&
        !(path->flags[path->count - 1] & PT_CLOSEFIGURE) &&
        path->points[path->count - 1].x == path->pos.x &&
        path->points[path->count - 1].y == path->pos.y)
        return TRUE;

    path->newStroke = FALSE;
    return add_points( path, &path->pos, 1, PT_MOVETO ) != NULL;
}

/* set current position to the last point that was added to the path */
static void update_current_pos( struct gdi_path *path )
{
    assert( path->count );
    path->pos = path->points[path->count - 1];
}

/* close the current figure */
static void close_figure( struct gdi_path *path )
{
    assert( path->count );
    path->flags[path->count - 1] |= PT_CLOSEFIGURE;
}

/* add a number of points, starting a new stroke if necessary */
static BOOL add_log_points_new_stroke( DC *dc, struct gdi_path *path, const POINT *points,
                                       DWORD count, BYTE type )
{
    if (!start_new_stroke( path )) return FALSE;
    if (!add_log_points( dc, path, points, count, type )) return FALSE;
    update_current_pos( path );
    return TRUE;
}

/* convert a (flattened) path to a region */
static HRGN path_to_region( const struct gdi_path *path, int mode )
{
    int i, pos, polygons, *counts;
    HRGN hrgn;

    if (!path->count) return 0;

    if (!(counts = malloc( (path->count / 2) * sizeof(*counts) ))) return 0;

    pos = polygons = 0;
    assert( path->flags[0] == PT_MOVETO );
    for (i = 1; i < path->count; i++)
    {
        if (path->flags[i] != PT_MOVETO) continue;
        counts[polygons++] = i - pos;
        pos = i;
    }
    if (i > pos + 1) counts[polygons++] = i - pos;

    assert( polygons <= path->count / 2 );
    hrgn = create_polypolygon_region( path->points, counts, polygons, mode, NULL );
    free( counts );
    return hrgn;
}

/* PATH_CheckCorners
 *
 * Helper function for RoundRect() and Rectangle()
 */
static BOOL PATH_CheckCorners( DC *dc, POINT corners[], INT x1, INT y1, INT x2, INT y2 )
{
    INT temp;

    /* Convert points to device coordinates */
    corners[0].x=x1;
    corners[0].y=y1;
    corners[1].x=x2;
    corners[1].y=y2;
    lp_to_dp( dc, corners, 2 );

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
    if (dc->attr->graphics_mode == GM_COMPATIBLE)
    {
        if (corners[0].x == corners[1].x) return FALSE;
        if (corners[0].y == corners[1].y) return FALSE;
        corners[1].x--;
        corners[1].y--;
    }
    return TRUE;
}

/* PATH_AddFlatBezier
 */
static BOOL PATH_AddFlatBezier(struct gdi_path *pPath, POINT *pt, BOOL closed)
{
    POINT *pts;
    BOOL ret;
    INT no;

    pts = GDI_Bezier( pt, 4, &no );
    if(!pts) return FALSE;

    ret = (add_points( pPath, pts + 1, no - 1, PT_LINETO ) != NULL);
    if (ret && closed) close_figure( pPath );
    free( pts );
    return ret;
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
    pPoint->x = GDI_ROUND( corners[0].x + (corners[1].x-corners[0].x)*0.5*(x+1.0) );
    pPoint->y = GDI_ROUND( corners[0].y + (corners[1].y-corners[0].y)*0.5*(y+1.0) );
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
    *pX = (pPoint->x-corners[0].x)/(corners[1].x-corners[0].x) * 2.0 - 1.0;
    *pY = (pPoint->y-corners[0].y)/(corners[1].y-corners[0].y) * 2.0 - 1.0;
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
    POINT points[4];
    BYTE *type;
    int i, start;

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
    start = !startEntryType;
    for (i = start; i < 4; i++) PATH_ScaleNormalizedPoint(corners, xNorm[i], yNorm[i], &points[i]);
    if (!(type = add_points( pPath, points + start, 4 - start, PT_BEZIERTO ))) return FALSE;
    if (!start) type[0] = startEntryType;
    return TRUE;
}

/* retrieve a flattened path in device coordinates, and optionally its region */
/* the DC path is deleted; the returned data must be freed by caller using free_gdi_path() */
/* helper for stroke_and_fill_path in the DIB driver */
struct gdi_path *get_gdi_flat_path( DC *dc, HRGN *rgn )
{
    struct gdi_path *ret = NULL;

    if (dc->path)
    {
        ret = PATH_FlattenPath( dc->path );

        free_gdi_path( dc->path );
        dc->path = NULL;
        if (ret && rgn) *rgn = path_to_region( ret, dc->attr->poly_fill_mode );
    }
    else RtlSetLastWin32Error( ERROR_CAN_NOT_COMPLETE );

    return ret;
}

int get_gdi_path_data( struct gdi_path *path, POINT **pts, BYTE **flags )
{
    *pts = path->points;
    *flags = path->flags;
    return path->count;
}

/***********************************************************************
 *           NtGdiBeginPath    (win32u.@)
 */
BOOL WINAPI NtGdiBeginPath( HDC hdc )
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
 *           NtGdiEndPath    (win32u.@)
 */
BOOL WINAPI NtGdiEndPath( HDC hdc )
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
 *           NtGdiAbortPath    (win32u.@)
 */
BOOL WINAPI NtGdiAbortPath( HDC hdc )
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
 *           NtGdiCloseFigure    (win32u.@)
 */
BOOL WINAPI NtGdiCloseFigure( HDC hdc )
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
 *           NtGdiGetPath    (win32u.@)
 */
INT WINAPI NtGdiGetPath( HDC hdc, POINT *points, BYTE *types, INT size )
{
    INT ret = -1;
    DC *dc = get_dc_ptr( hdc );

    if (!dc) return -1;

    if (!dc->path)
    {
        RtlSetLastWin32Error( ERROR_CAN_NOT_COMPLETE );
    }
    else if (size == 0)
    {
        ret = dc->path->count;
    }
    else if (size < dc->path->count)
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
    }
    else
    {
        memcpy( points, dc->path->points, sizeof(POINT) * dc->path->count );
        memcpy( types, dc->path->flags, sizeof(BYTE) * dc->path->count );

        /* Convert the points to logical coordinates */
        if (dp_to_lp( dc, points, dc->path->count ))
            ret = dc->path->count;
        else
            /* FIXME: Is this the correct value? */
            RtlSetLastWin32Error( ERROR_CAN_NOT_COMPLETE );
    }

    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           NtGdiPathToRegion    (win32u.@)
 */
HRGN WINAPI NtGdiPathToRegion( HDC hdc )
{
    HRGN ret = 0;
    DC *dc = get_dc_ptr( hdc );

    if (!dc) return 0;

    if (dc->path)
    {
        struct gdi_path *path = PATH_FlattenPath( dc->path );

        free_gdi_path( dc->path );
        dc->path = NULL;
        if (path)
        {
            ret = path_to_region( path, dc->attr->poly_fill_mode );
            free_gdi_path( path );
        }
    }
    else RtlSetLastWin32Error( ERROR_CAN_NOT_COMPLETE );

    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           NtGdiFillPath    (win32u.@)
 */
BOOL WINAPI NtGdiFillPath( HDC hdc )
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
 *           NtGdiSelectClipPath    (win32u.@)
 */
BOOL WINAPI NtGdiSelectClipPath( HDC hdc, INT mode )
{
    BOOL ret = FALSE;
    HRGN rgn;

    if ((rgn = NtGdiPathToRegion( hdc )))
    {
        ret = NtGdiExtSelectClipRgn( hdc, rgn, mode ) != ERROR;
        NtGdiDeleteObjectApp( rgn );
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
    DC *dc = get_physdev_dc( dev );

    path_driver.pDeleteDC( pop_dc_driver( dc, &path_driver ));
    return TRUE;
}


/***********************************************************************
 *           pathdrv_EndPath
 */
static BOOL pathdrv_EndPath( PHYSDEV dev )
{
    struct path_physdev *physdev = get_path_physdev( dev );
    DC *dc = get_physdev_dc( dev );

    dc->path = physdev->path;
    pop_dc_driver( dc, &path_driver );
    free( physdev );
    return TRUE;
}


/***********************************************************************
 *           pathdrv_CreateDC
 */
static BOOL pathdrv_CreateDC( PHYSDEV *dev, LPCWSTR device, LPCWSTR output, const DEVMODEW *devmode )
{
    struct path_physdev *physdev = malloc( sizeof(*physdev) );

    if (!physdev) return FALSE;
    push_dc_driver( dev, &physdev->dev, &path_driver );
    return TRUE;
}


/*************************************************************
 *           pathdrv_DeleteDC
 */
static BOOL pathdrv_DeleteDC( PHYSDEV dev )
{
    struct path_physdev *physdev = get_path_physdev( dev );

    free_gdi_path( physdev->path );
    free( physdev );
    return TRUE;
}


BOOL PATH_SavePath( DC *dst, DC *src )
{
    PHYSDEV dev;

    if (src->path)
    {
        if (!(dst->path = copy_gdi_path( src->path ))) return FALSE;
    }
    else if ((dev = find_dc_driver( src, &path_driver )))
    {
        struct path_physdev *physdev = get_path_physdev( dev );
        if (!(dst->path = copy_gdi_path( physdev->path ))) return FALSE;
        dst->path_open = TRUE;
    }
    else dst->path = NULL;
    return TRUE;
}

BOOL PATH_RestorePath( DC *dst, DC *src )
{
    PHYSDEV dev;
    struct path_physdev *physdev;

    if ((dev = pop_dc_driver( dst, &path_driver )))
    {
        physdev = get_path_physdev( dev );
        free_gdi_path( physdev->path );
        free( physdev );
    }

    if (src->path && src->path_open)
    {
        if (!path_driver.pCreateDC( &dst->physDev, NULL, NULL, NULL )) return FALSE;
        physdev = get_path_physdev( find_dc_driver( dst, &path_driver ));
        physdev->path = src->path;
        src->path_open = FALSE;
        src->path = NULL;
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
    DC *dc = get_physdev_dc( dev );

    physdev->path->newStroke = TRUE;
    physdev->path->pos.x = x;
    physdev->path->pos.y = y;
    lp_to_dp( dc, &physdev->path->pos, 1 );
    return TRUE;
}


/*************************************************************
 *           pathdrv_LineTo
 */
static BOOL pathdrv_LineTo( PHYSDEV dev, INT x, INT y )
{
    struct path_physdev *physdev = get_path_physdev( dev );
    DC *dc = get_physdev_dc( dev );
    POINT point;

    point.x = x;
    point.y = y;
    return add_log_points_new_stroke( dc, physdev->path, &point, 1, PT_LINETO );
}


/*************************************************************
 *           pathdrv_Rectangle
 */
static BOOL pathdrv_Rectangle( PHYSDEV dev, INT x1, INT y1, INT x2, INT y2 )
{
    struct path_physdev *physdev = get_path_physdev( dev );
    DC *dc = get_physdev_dc( dev );
    POINT corners[2], points[4];
    BYTE *type;

    if (!PATH_CheckCorners( dc, corners, x1, y1, x2, y2 )) return TRUE;

    points[0].x = corners[1].x;
    points[0].y = corners[0].y;
    points[1]   = corners[0];
    points[2].x = corners[0].x;
    points[2].y = corners[1].y;
    points[3]   = corners[1];
    if (dc->attr->arc_direction == AD_CLOCKWISE) reverse_points( points, 4 );

    if (!(type = add_points( physdev->path, points, 4, PT_LINETO ))) return FALSE;
    type[0] = PT_MOVETO;
    close_figure( physdev->path );
    return TRUE;
}


/*************************************************************
 *           pathdrv_RoundRect
 */
static BOOL pathdrv_RoundRect( PHYSDEV dev, INT x1, INT y1, INT x2, INT y2, INT ell_width, INT ell_height )
{
    const double factor = 0.55428475; /* 4 / 3 * (sqrt(2) - 1) */
    struct path_physdev *physdev = get_path_physdev( dev );
    DC *dc = get_physdev_dc( dev );
    POINT corners[2], ellipse[2], points[16];
    BYTE *type;
    double width, height;

    if (!ell_width || !ell_height) return pathdrv_Rectangle( dev, x1, y1, x2, y2 );

    if (!PATH_CheckCorners( dc, corners, x1, y1, x2, y2 )) return TRUE;

    ellipse[0].x = ellipse[0].y = 0;
    ellipse[1].x = ell_width;
    ellipse[1].y = ell_height;
    lp_to_dp( dc, (POINT *)&ellipse, 2 );
    ell_width  = min( abs( ellipse[1].x - ellipse[0].x ), corners[1].x - corners[0].x );
    ell_height = min( abs( ellipse[1].y - ellipse[0].y ), corners[1].y - corners[0].y );
    width  = ell_width / 2.0;
    height = ell_height / 2.0;

    /* starting point */
    points[0].x  = corners[1].x;
    points[0].y  = corners[0].y + GDI_ROUND( height );
    /* first curve */
    points[1].x  = corners[1].x;
    points[1].y  = corners[0].y + GDI_ROUND( height * (1 - factor) );
    points[2].x  = corners[1].x - GDI_ROUND( width * (1 - factor) );
    points[2].y  = corners[0].y;
    points[3].x  = corners[1].x - GDI_ROUND( width );
    points[3].y  = corners[0].y;
    /* horizontal line */
    points[4].x  = corners[0].x + GDI_ROUND( width );
    points[4].y  = corners[0].y;
    /* second curve */
    points[5].x  = corners[0].x + GDI_ROUND( width * (1 - factor) );
    points[5].y  = corners[0].y;
    points[6].x  = corners[0].x;
    points[6].y  = corners[0].y + GDI_ROUND( height * (1 - factor) );
    points[7].x  = corners[0].x;
    points[7].y  = corners[0].y + GDI_ROUND( height );
    /* vertical line */
    points[8].x  = corners[0].x;
    points[8].y  = corners[1].y - GDI_ROUND( height );
    /* third curve */
    points[9].x  = corners[0].x;
    points[9].y  = corners[1].y - GDI_ROUND( height * (1 - factor) );
    points[10].x = corners[0].x + GDI_ROUND( width * (1 - factor) );
    points[10].y = corners[1].y;
    points[11].x = corners[0].x + GDI_ROUND( width );
    points[11].y = corners[1].y;
    /* horizontal line */
    points[12].x = corners[1].x - GDI_ROUND( width );
    points[12].y = corners[1].y;
    /* fourth curve */
    points[13].x = corners[1].x - GDI_ROUND( width * (1 - factor) );
    points[13].y = corners[1].y;
    points[14].x = corners[1].x;
    points[14].y = corners[1].y - GDI_ROUND( height * (1 - factor) );
    points[15].x = corners[1].x;
    points[15].y = corners[1].y - GDI_ROUND( height );

    if (dc->attr->arc_direction == AD_CLOCKWISE) reverse_points( points, 16 );
    if (!(type = add_points( physdev->path, points, 16, PT_BEZIERTO ))) return FALSE;
    type[0] = PT_MOVETO;
    type[4] = type[8] = type[12] = PT_LINETO;
    close_figure( physdev->path );
    return TRUE;
}


/*************************************************************
 *           pathdrv_Ellipse
 */
static BOOL pathdrv_Ellipse( PHYSDEV dev, INT x1, INT y1, INT x2, INT y2 )
{
    const double factor = 0.55428475; /* 4 / 3 * (sqrt(2) - 1) */
    struct path_physdev *physdev = get_path_physdev( dev );
    DC *dc = get_physdev_dc( dev );
    POINT corners[2], points[13];
    BYTE *type;
    double width, height;

    if (!PATH_CheckCorners( dc, corners, x1, y1, x2, y2 )) return TRUE;

    width  = (corners[1].x - corners[0].x) / 2.0;
    height = (corners[1].y - corners[0].y) / 2.0;

    /* starting point */
    points[0].x = corners[1].x;
    points[0].y = corners[0].y + GDI_ROUND( height );
    /* first curve */
    points[1].x = corners[1].x;
    points[1].y = corners[0].y + GDI_ROUND( height * (1 - factor) );
    points[2].x = corners[1].x - GDI_ROUND( width * (1 - factor) );
    points[2].y = corners[0].y;
    points[3].x = corners[0].x + GDI_ROUND( width );
    points[3].y = corners[0].y;
    /* second curve */
    points[4].x = corners[0].x + GDI_ROUND( width * (1 - factor) );
    points[4].y = corners[0].y;
    points[5].x = corners[0].x;
    points[5].y = corners[0].y + GDI_ROUND( height * (1 - factor) );
    points[6].x = corners[0].x;
    points[6].y = corners[0].y + GDI_ROUND( height );
    /* third curve */
    points[7].x = corners[0].x;
    points[7].y = corners[1].y - GDI_ROUND( height * (1 - factor) );
    points[8].x = corners[0].x + GDI_ROUND( width * (1 - factor) );
    points[8].y = corners[1].y;
    points[9].x = corners[0].x + GDI_ROUND( width );
    points[9].y = corners[1].y;
    /* fourth curve */
    points[10].x = corners[1].x - GDI_ROUND( width * (1 - factor) );
    points[10].y = corners[1].y;
    points[11].x = corners[1].x;
    points[11].y = corners[1].y - GDI_ROUND( height * (1 - factor) );
    points[12].x = corners[1].x;
    points[12].y = corners[1].y - GDI_ROUND( height );

    if (dc->attr->arc_direction == AD_CLOCKWISE) reverse_points( points, 13 );
    if (!(type = add_points( physdev->path, points, 13, PT_BEZIERTO ))) return FALSE;
    type[0] = PT_MOVETO;
    close_figure( physdev->path );
    return TRUE;
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
                      INT xStart, INT yStart, INT xEnd, INT yEnd, int direction, int lines )
{
    DC *dc = get_physdev_dc( dev );
    struct path_physdev *physdev = get_path_physdev( dev );
    double angleStart, angleEnd, angleStartQuadrant, angleEndQuadrant=0.0;
               /* Initialize angleEndQuadrant to silence gcc's warning */
    double x, y;
    FLOAT_POINT corners[2], pointStart, pointEnd;
    POINT centre;
    BOOL start, end;
    INT temp;

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
   INTERNAL_LPTODP_FLOAT(dc, corners, 2);
   INTERNAL_LPTODP_FLOAT(dc, &pointStart, 1);
   INTERNAL_LPTODP_FLOAT(dc, &pointEnd, 1);

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
   if (dc->attr->graphics_mode == GM_COMPATIBLE)
   {
      corners[1].x--;
      corners[1].y--;
   }

   /* arcto: Add a PT_MOVETO only if this is the first entry in a stroke */
   if (lines == -1 && !start_new_stroke( physdev->path )) return FALSE;

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
         start ? (lines==-1 ? PT_LINETO : PT_MOVETO) : 0);
      start=FALSE;
   }  while(!end);

   /* chord: close figure. pie: add line and close figure */
   switch (lines)
   {
   case -1:
       update_current_pos( physdev->path );
       break;
   case 1:
       close_figure( physdev->path );
       break;
   case 2:
      centre.x = (corners[0].x+corners[1].x)/2;
      centre.y = (corners[0].y+corners[1].y)/2;
      if(!PATH_AddEntry(physdev->path, &centre, PT_LINETO | PT_CLOSEFIGURE))
         return FALSE;
      break;
   }
   return TRUE;
}


/*************************************************************
 *           pathdrv_AngleArc
 */
static BOOL pathdrv_AngleArc( PHYSDEV dev, INT x, INT y, DWORD radius, FLOAT eStartAngle, FLOAT eSweepAngle)
{
    int x1 = GDI_ROUND( x + cos(eStartAngle*M_PI/180) * radius );
    int y1 = GDI_ROUND( y - sin(eStartAngle*M_PI/180) * radius );
    int x2 = GDI_ROUND( x + cos((eStartAngle+eSweepAngle)*M_PI/180) * radius );
    int y2 = GDI_ROUND( y - sin((eStartAngle+eSweepAngle)*M_PI/180) * radius );
    return PATH_Arc( dev, x-radius, y-radius, x+radius, y+radius, x1, y1, x2, y2,
                     eSweepAngle >= 0 ? AD_COUNTERCLOCKWISE : AD_CLOCKWISE, -1 );
}


/*************************************************************
 *           pathdrv_Arc
 */
static BOOL pathdrv_Arc( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                         INT xstart, INT ystart, INT xend, INT yend )
{
    DC *dc = get_physdev_dc( dev );
    return PATH_Arc( dev, left, top, right, bottom, xstart, ystart, xend, yend,
                     dc->attr->arc_direction, 0 );
}


/*************************************************************
 *           pathdrv_ArcTo
 */
static BOOL pathdrv_ArcTo( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                           INT xstart, INT ystart, INT xend, INT yend )
{
    DC *dc = get_physdev_dc( dev );
    return PATH_Arc( dev, left, top, right, bottom, xstart, ystart, xend, yend,
                     dc->attr->arc_direction, -1 );
}


/*************************************************************
 *           pathdrv_Chord
 */
static BOOL pathdrv_Chord( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                           INT xstart, INT ystart, INT xend, INT yend )
{
    DC *dc = get_physdev_dc( dev );
    return PATH_Arc( dev, left, top, right, bottom, xstart, ystart, xend, yend,
                     dc->attr->arc_direction, 1 );
}


/*************************************************************
 *           pathdrv_Pie
 */
static BOOL pathdrv_Pie( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                         INT xstart, INT ystart, INT xend, INT yend )
{
    DC *dc = get_physdev_dc( dev );
    return PATH_Arc( dev, left, top, right, bottom, xstart, ystart, xend, yend,
                     dc->attr->arc_direction, 2 );
}


/*************************************************************
 *           pathdrv_PolyBezierTo
 */
static BOOL pathdrv_PolyBezierTo( PHYSDEV dev, const POINT *pts, DWORD cbPoints )
{
    struct path_physdev *physdev = get_path_physdev( dev );
    DC *dc = get_physdev_dc( dev );

    return add_log_points_new_stroke( dc, physdev->path, pts, cbPoints, PT_BEZIERTO );
}


/*************************************************************
 *           pathdrv_PolyBezier
 */
static BOOL pathdrv_PolyBezier( PHYSDEV dev, const POINT *pts, DWORD cbPoints )
{
    struct path_physdev *physdev = get_path_physdev( dev );
    DC *dc = get_physdev_dc( dev );
    BYTE *type = add_log_points( dc, physdev->path, pts, cbPoints, PT_BEZIERTO );

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
    struct gdi_path *path = physdev->path;
    DC *dc = get_physdev_dc( dev );
    POINT orig_pos;
    INT i, lastmove = 0;

    for (i = 0; i < path->count; i++) if (path->flags[i] == PT_MOVETO) lastmove = i;
    orig_pos = path->pos;

    for(i = 0; i < cbPoints; i++)
    {
        switch (types[i])
        {
        case PT_MOVETO:
            path->newStroke = TRUE;
            path->pos = pts[i];
            lp_to_dp( dc, &path->pos, 1 );
            lastmove = path->count;
            break;
        case PT_LINETO:
        case PT_LINETO | PT_CLOSEFIGURE:
            if (!add_log_points_new_stroke( dc, path, &pts[i], 1, PT_LINETO )) return FALSE;
            break;
        case PT_BEZIERTO:
            if ((i + 2 < cbPoints) && (types[i + 1] == PT_BEZIERTO) &&
                (types[i + 2] & ~PT_CLOSEFIGURE) == PT_BEZIERTO)
            {
                if (!add_log_points_new_stroke( dc, path, &pts[i], 3, PT_BEZIERTO )) return FALSE;
                i += 2;
                break;
            }
            /* fall through */
        default:
            /* restore original position */
            path->pos = orig_pos;
            return FALSE;
        }

        if (types[i] & PT_CLOSEFIGURE)
        {
            close_figure( path );
            path->pos = path->points[lastmove];
        }
    }
    return TRUE;
}


/*************************************************************
 *           pathdrv_PolylineTo
 */
static BOOL pathdrv_PolylineTo( PHYSDEV dev, const POINT *pts, INT count )
{
    struct path_physdev *physdev = get_path_physdev( dev );
    DC *dc = get_physdev_dc( dev );

    if (count < 1) return FALSE;
    return add_log_points_new_stroke( dc, physdev->path, pts, count, PT_LINETO );
}


/*************************************************************
 *           pathdrv_PolyPolygon
 */
static BOOL pathdrv_PolyPolygon( PHYSDEV dev, const POINT* pts, const INT* counts, UINT polygons )
{
    struct path_physdev *physdev = get_path_physdev( dev );
    DC *dc = get_physdev_dc( dev );
    UINT poly, count;
    BYTE *type;

    if (!polygons) return FALSE;
    for (poly = count = 0; poly < polygons; poly++)
    {
        if (counts[poly] < 2) return FALSE;
        count += counts[poly];
    }

    type = add_log_points( dc, physdev->path, pts, count, PT_LINETO );
    if (!type) return FALSE;

    /* make the first point of each polyline a PT_MOVETO, and close the last one */
    for (poly = 0; poly < polygons; type += counts[poly++])
    {
        type[0] = PT_MOVETO;
        type[counts[poly] - 1] = PT_LINETO | PT_CLOSEFIGURE;
    }
    return TRUE;
}


/*************************************************************
 *           pathdrv_PolyPolyline
 */
static BOOL pathdrv_PolyPolyline( PHYSDEV dev, const POINT* pts, const DWORD* counts, DWORD polylines )
{
    struct path_physdev *physdev = get_path_physdev( dev );
    DC *dc = get_physdev_dc( dev );
    UINT poly, count;
    BYTE *type;

    if (!polylines) return FALSE;
    for (poly = count = 0; poly < polylines; poly++)
    {
        if (counts[poly] < 2) return FALSE;
        count += counts[poly];
    }

    type = add_log_points( dc, physdev->path, pts, count, PT_LINETO );
    if (!type) return FALSE;

    /* make the first point of each polyline a PT_MOVETO */
    for (poly = 0; poly < polylines; type += counts[poly++]) *type = PT_MOVETO;
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
        add_points( pPath, lppt, 3, PT_BEZIERTO );
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
            add_points( pPath, pt, 3, PT_BEZIERTO );
            n--;
            i++;
        }

        pt[0] = pt[2];
        pt[1] = lppt[i+1];
        pt[2] = lppt[i+2];
        add_points( pPath, pt, 3, PT_BEZIERTO );
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
            FIXME("Unknown header type %d\n", (int)header->dwType);
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
                POINT *pts = malloc( (curve->cpfx + 1) * sizeof(POINT) );

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

                free( pts );
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

    close_figure( physdev->path );
    return TRUE;
}

/*************************************************************
 *           pathdrv_ExtTextOut
 */
static BOOL pathdrv_ExtTextOut( PHYSDEV dev, INT x, INT y, UINT flags, const RECT *lprc,
                                LPCWSTR str, UINT count, const INT *dx )
{
    struct path_physdev *physdev = get_path_physdev( dev );
    unsigned int idx, ggo_flags = GGO_NATIVE;
    POINT offset = {0, 0};

    if (!count) return TRUE;
    if (flags & ETO_GLYPH_INDEX) ggo_flags |= GGO_GLYPH_INDEX;

    for (idx = 0; idx < count; idx++)
    {
        static const MAT2 identity = { {0,1},{0,0},{0,0},{0,1} };
        GLYPHMETRICS gm;
        DWORD dwSize;
        void *outline;

        dwSize = NtGdiGetGlyphOutline( dev->hdc, str[idx], ggo_flags, &gm, 0, NULL, &identity, FALSE );
        if (dwSize == GDI_ERROR) continue;

        /* add outline only if char is printable */
        if(dwSize)
        {
            outline = malloc( dwSize );
            if (!outline) return FALSE;

            NtGdiGetGlyphOutline( dev->hdc, str[idx], ggo_flags, &gm, dwSize, outline, &identity, FALSE );
            PATH_add_outline(physdev, x + offset.x, y + offset.y, outline, dwSize);

            free( outline );
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
    if (physdev->path->count) close_figure( physdev->path );
    return TRUE;
}


/*******************************************************************
 *           NtGdiFlattenPath   (win32u.@)
 */
BOOL WINAPI NtGdiFlattenPath( HDC hdc )
{
    struct gdi_path *path;
    BOOL ret = FALSE;
    DC *dc;

    if (!(dc = get_dc_ptr( hdc ))) return FALSE;

    if (!dc->path) RtlSetLastWin32Error( ERROR_CAN_NOT_COMPLETE );
    else if ((path = PATH_FlattenPath( dc->path )))
    {
        free_gdi_path( dc->path );
        dc->path = path;
        ret = TRUE;
    }

    release_dc_ptr( dc );
    return ret;
}


#define round(x) ((int)((x)>0?(x)+0.5:(x)-0.5))

static struct gdi_path *PATH_WidenPath(DC *dc)
{
    INT i, j, numStrokes, penWidth, penWidthIn, penWidthOut, size, penStyle;
    struct gdi_path *flat_path, *pNewPath, **pStrokes = NULL, **new_strokes, *pUpPath, *pDownPath;
    EXTLOGPEN *elp;
    BYTE *type;
    DWORD obj_type, joint, endcap, penType;

    size = NtGdiExtGetObjectW( dc->hPen, 0, NULL );
    if (!size) {
        RtlSetLastWin32Error(ERROR_CAN_NOT_COMPLETE);
        return NULL;
    }

    elp = malloc( size );
    NtGdiExtGetObjectW( dc->hPen, size, elp );

    obj_type = get_gdi_object_type(dc->hPen);
    switch (obj_type)
    {
    case NTGDI_OBJ_PEN:
        penStyle = ((LOGPEN*)elp)->lopnStyle;
        break;
    case NTGDI_OBJ_EXTPEN:
        penStyle = elp->elpPenStyle;
        break;
    default:
        RtlSetLastWin32Error(ERROR_CAN_NOT_COMPLETE);
        free( elp );
        return NULL;
    }

    penWidth = elp->elpWidth;
    free( elp );

    endcap = (PS_ENDCAP_MASK & penStyle);
    joint = (PS_JOIN_MASK & penStyle);
    penType = (PS_TYPE_MASK & penStyle);

    /* The function cannot apply to cosmetic pens */
    if(obj_type == OBJ_EXTPEN && penType == PS_COSMETIC) {
        RtlSetLastWin32Error(ERROR_CAN_NOT_COMPLETE);
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
                new_strokes = realloc( pStrokes, numStrokes * sizeof(*pStrokes) );
                if (!new_strokes)
                {
                    free_gdi_path(flat_path);
                    free(pStrokes);
                    return NULL;
                }
                pStrokes = new_strokes;
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
                for(i = 0; i < numStrokes; i++) free_gdi_path(pStrokes[i]);
                free( pStrokes );
                free_gdi_path(flat_path);
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
                        PATH_DoArcPart(pUpPath ,corners, theta + M_PI_2 , theta + 3 * M_PI_4, (j == 0 ? PT_MOVETO : 0));
                        PATH_DoArcPart(pUpPath ,corners, theta + 3 * M_PI_4 , theta + M_PI, 0);
                        PATH_DoArcPart(pUpPath ,corners, theta + M_PI, theta +  5 * M_PI_4, 0);
                        PATH_DoArcPart(pUpPath ,corners, theta + 5 * M_PI_4 , theta + 3 * M_PI_2, 0);
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
                if(_joint == PS_JOIN_MITER && dc->attr->miter_limit < fabs(1 / sin(alpha/2))) {
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
        type = add_points( pNewPath, pUpPath->points, pUpPath->count, PT_LINETO );
        type[0] = PT_MOVETO;
        reverse_points( pDownPath->points, pDownPath->count );
        type = add_points( pNewPath, pDownPath->points, pDownPath->count, PT_LINETO );
        if (pStrokes[i]->flags[pStrokes[i]->count - 1] & PT_CLOSEFIGURE) type[0] = PT_MOVETO;

        free_gdi_path( pStrokes[i] );
        free_gdi_path( pUpPath );
        free_gdi_path( pDownPath );
    }
    free( pStrokes );
    free_gdi_path( flat_path );
    return pNewPath;
}


/*******************************************************************
 *           NtGdiStrokeAndFillPath   (win32u.@)
 */
BOOL WINAPI NtGdiStrokeAndFillPath( HDC hdc )
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
 *           NtGdiStrokePath   (win32u.@)
 */
BOOL WINAPI NtGdiStrokePath( HDC hdc )
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
 *           NtGdiWidenPath   (win32u.@)
 */
BOOL WINAPI NtGdiWidenPath( HDC hdc )
{
    struct gdi_path *path;
    BOOL ret = FALSE;
    DC *dc;

    if (!(dc = get_dc_ptr( hdc ))) return FALSE;

    if (!dc->path) RtlSetLastWin32Error( ERROR_CAN_NOT_COMPLETE );
    else if ((path = PATH_WidenPath( dc )))
    {
        free_gdi_path( dc->path );
        dc->path = path;
        ret = TRUE;
    }

    release_dc_ptr( dc );
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
    if (!path_driver.pCreateDC( &dc->physDev, NULL, NULL, NULL ))
    {
        free_gdi_path( path );
        return FALSE;
    }
    physdev = get_path_physdev( find_dc_driver( dc, &path_driver ));
    physdev->path = path;
    path->pos = dc->attr->cur_pos;
    lp_to_dp( dc, &path->pos, 1 );
    if (dc->path) free_gdi_path( dc->path );
    dc->path = NULL;
    return TRUE;
}

BOOL nulldrv_EndPath( PHYSDEV dev )
{
    RtlSetLastWin32Error( ERROR_CAN_NOT_COMPLETE );
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
    RtlSetLastWin32Error( ERROR_CAN_NOT_COMPLETE );
    return FALSE;
}

BOOL nulldrv_FillPath( PHYSDEV dev )
{
    if (NtGdiGetPath( dev->hdc, NULL, NULL, 0 ) == -1) return FALSE;
    NtGdiAbortPath( dev->hdc );
    return TRUE;
}

BOOL nulldrv_StrokeAndFillPath( PHYSDEV dev )
{
    if (NtGdiGetPath( dev->hdc, NULL, NULL, 0 ) == -1) return FALSE;
    NtGdiAbortPath( dev->hdc );
    return TRUE;
}

BOOL nulldrv_StrokePath( PHYSDEV dev )
{
    if (NtGdiGetPath( dev->hdc, NULL, NULL, 0 ) == -1) return FALSE;
    NtGdiAbortPath( dev->hdc );
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
    pathdrv_Ellipse,                    /* pEllipse */
    NULL,                               /* pEndDoc */
    NULL,                               /* pEndPage */
    pathdrv_EndPath,                    /* pEndPath */
    NULL,                               /* pEnumFonts */
    NULL,                               /* pExtEscape */
    NULL,                               /* pExtFloodFill */
    pathdrv_ExtTextOut,                 /* pExtTextOut */
    NULL,                               /* pFillPath */
    NULL,                               /* pFillRgn */
    NULL,                               /* pFontIsLinked */
    NULL,                               /* pFrameRgn */
    NULL,                               /* pGetBoundsRect */
    NULL,                               /* pGetCharABCWidths */
    NULL,                               /* pGetCharABCWidthsI */
    NULL,                               /* pGetCharWidth */
    NULL,                               /* pGetCharWidthInfo */
    NULL,                               /* pGetDeviceCaps */
    NULL,                               /* pGetDeviceGammaRamp */
    NULL,                               /* pGetFontData */
    NULL,                               /* pGetFontRealizationInfo */
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
    NULL,                               /* pInvertRgn */
    pathdrv_LineTo,                     /* pLineTo */
    pathdrv_MoveTo,                     /* pMoveTo */
    NULL,                               /* pPaintRgn */
    NULL,                               /* pPatBlt */
    pathdrv_Pie,                        /* pPie */
    pathdrv_PolyBezier,                 /* pPolyBezier */
    pathdrv_PolyBezierTo,               /* pPolyBezierTo */
    pathdrv_PolyDraw,                   /* pPolyDraw */
    pathdrv_PolyPolygon,                /* pPolyPolygon */
    pathdrv_PolyPolyline,               /* pPolyPolyline */
    pathdrv_PolylineTo,                 /* pPolylineTo */
    NULL,                               /* pPutImage */
    NULL,                               /* pRealizeDefaultPalette */
    NULL,                               /* pRealizePalette */
    pathdrv_Rectangle,                  /* pRectangle */
    NULL,                               /* pResetDC */
    pathdrv_RoundRect,                  /* pRoundRect */
    NULL,                               /* pSelectBitmap */
    NULL,                               /* pSelectBrush */
    NULL,                               /* pSelectFont */
    NULL,                               /* pSelectPen */
    NULL,                               /* pSetBkColor */
    NULL,                               /* pSetBoundsRect */
    NULL,                               /* pSetDCBrushColor */
    NULL,                               /* pSetDCPenColor */
    NULL,                               /* pSetDIBitsToDevice */
    NULL,                               /* pSetDeviceClipping */
    NULL,                               /* pSetDeviceGammaRamp */
    NULL,                               /* pSetPixel */
    NULL,                               /* pSetTextColor */
    NULL,                               /* pStartDoc */
    NULL,                               /* pStartPage */
    NULL,                               /* pStretchBlt */
    NULL,                               /* pStretchDIBits */
    NULL,                               /* pStrokeAndFillPath */
    NULL,                               /* pStrokePath */
    NULL,                               /* pUnrealizePalette */
    NULL,                               /* pD3DKMTCheckVidPnExclusiveOwnership */
    NULL,                               /* pD3DKMTCloseAdapter */
    NULL,                               /* pD3DKMTOpenAdapterFromLuid */
    NULL,                               /* pD3DKMTQueryVideoMemoryInfo */
    NULL,                               /* pD3DKMTSetVidPnSourceOwner */
    GDI_PRIORITY_PATH_DRV               /* priority */
};
