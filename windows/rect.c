/*
 * Rectangle-related functions
 *
 * Copyright 1993, 1996 Alexandre Julliard
 *
 */

#include "windows.h"


/***********************************************************************
 *           SetRect16    (USER.72)
 */
void WINAPI SetRect16( LPRECT16 rect, INT16 left, INT16 top,
                       INT16 right, INT16 bottom )
{
    rect->left   = left;
    rect->right  = right;
    rect->top    = top;
    rect->bottom = bottom;
}


/***********************************************************************
 *           SetRect32    (USER32.499)
 */
BOOL32 WINAPI SetRect32( LPRECT32 rect, INT32 left, INT32 top,
                       INT32 right, INT32 bottom )
{
    rect->left   = left;
    rect->right  = right;
    rect->top    = top;
    rect->bottom = bottom;
    return TRUE;
}


/***********************************************************************
 *           SetRectEmpty16    (USER.73)
 */
void WINAPI SetRectEmpty16( LPRECT16 rect )
{
    rect->left = rect->right = rect->top = rect->bottom = 0;
}


/***********************************************************************
 *           SetRectEmpty32    (USER32.500)
 */
BOOL32 WINAPI SetRectEmpty32( LPRECT32 rect )
{
    rect->left = rect->right = rect->top = rect->bottom = 0;
    return TRUE;
}


/***********************************************************************
 *           CopyRect16    (USER.74)
 */
BOOL16 WINAPI CopyRect16( RECT16 *dest, const RECT16 *src )
{
    *dest = *src;
    return TRUE;
}


/***********************************************************************
 *           CopyRect32    (USER32.62)
 */
BOOL32 WINAPI CopyRect32( RECT32 *dest, const RECT32 *src )
{
    *dest = *src;
    return TRUE;
}


/***********************************************************************
 *           IsRectEmpty16    (USER.75)
 */
BOOL16 WINAPI IsRectEmpty16( const RECT16 *rect )
{
    return ((rect->left == rect->right) || (rect->top == rect->bottom));
}


/***********************************************************************
 *           IsRectEmpty32    (USER32.347)
 */
BOOL32 WINAPI IsRectEmpty32( const RECT32 *rect )
{
    return ((rect->left == rect->right) || (rect->top == rect->bottom));
}


/***********************************************************************
 *           PtInRect16    (USER.76)
 */
BOOL16 WINAPI PtInRect16( const RECT16 *rect, POINT16 pt )
{
    return ((pt.x >= rect->left) && (pt.x < rect->right) &&
	    (pt.y >= rect->top) && (pt.y < rect->bottom));
}


/***********************************************************************
 *           PtInRect32    (USER32.424)
 */
BOOL32 WINAPI PtInRect32( const RECT32 *rect, POINT32 pt )
{
    return ((pt.x >= rect->left) && (pt.x < rect->right) &&
	    (pt.y >= rect->top) && (pt.y < rect->bottom));
}


/***********************************************************************
 *           OffsetRect16    (USER.77)
 */
void WINAPI OffsetRect16( LPRECT16 rect, INT16 x, INT16 y )
{
    rect->left   += x;
    rect->right  += x;
    rect->top    += y;
    rect->bottom += y;    
}


/***********************************************************************
 *           OffsetRect32    (USER32.406)
 */
BOOL32 WINAPI OffsetRect32( LPRECT32 rect, INT32 x, INT32 y )
{
    rect->left   += x;
    rect->right  += x;
    rect->top    += y;
    rect->bottom += y;    
    return TRUE;
}


/***********************************************************************
 *           InflateRect16    (USER.78)
 */
void WINAPI InflateRect16( LPRECT16 rect, INT16 x, INT16 y )
{
    rect->left   -= x;
    rect->top 	 -= y;
    rect->right  += x;
    rect->bottom += y;
}


/***********************************************************************
 *           InflateRect32    (USER32.321)
 */
BOOL32 WINAPI InflateRect32( LPRECT32 rect, INT32 x, INT32 y )
{
    rect->left   -= x;
    rect->top 	 -= y;
    rect->right  += x;
    rect->bottom += y;
    return TRUE;
}


/***********************************************************************
 *           IntersectRect16    (USER.79)
 */
BOOL16 WINAPI IntersectRect16( LPRECT16 dest, const RECT16 *src1,
                               const RECT16 *src2 )
{
    if (IsRectEmpty16(src1) || IsRectEmpty16(src2) ||
	(src1->left >= src2->right) || (src2->left >= src1->right) ||
	(src1->top >= src2->bottom) || (src2->top >= src1->bottom))
    {
	SetRectEmpty16( dest );
	return FALSE;
    }
    dest->left   = MAX( src1->left, src2->left );
    dest->right  = MIN( src1->right, src2->right );
    dest->top    = MAX( src1->top, src2->top );
    dest->bottom = MIN( src1->bottom, src2->bottom );
    return TRUE;
}


/***********************************************************************
 *           IntersectRect32    (USER32.327)
 */
BOOL32 WINAPI IntersectRect32( LPRECT32 dest, const RECT32 *src1,
                               const RECT32 *src2 )
{
    if (IsRectEmpty32(src1) || IsRectEmpty32(src2) ||
	(src1->left >= src2->right) || (src2->left >= src1->right) ||
	(src1->top >= src2->bottom) || (src2->top >= src1->bottom))
    {
	SetRectEmpty32( dest );
	return FALSE;
    }
    dest->left   = MAX( src1->left, src2->left );
    dest->right  = MIN( src1->right, src2->right );
    dest->top    = MAX( src1->top, src2->top );
    dest->bottom = MIN( src1->bottom, src2->bottom );
    return TRUE;
}


/***********************************************************************
 *           UnionRect16    (USER.80)
 */
BOOL16 WINAPI UnionRect16( LPRECT16 dest, const RECT16 *src1,
                           const RECT16 *src2 )
{
    if (IsRectEmpty16(src1))
    {
	if (IsRectEmpty16(src2))
	{
	    SetRectEmpty16( dest );
	    return FALSE;
	}
	else *dest = *src2;
    }
    else
    {
	if (IsRectEmpty16(src2)) *dest = *src1;
	else
	{
	    dest->left   = MIN( src1->left, src2->left );
	    dest->right  = MAX( src1->right, src2->right );
	    dest->top    = MIN( src1->top, src2->top );
	    dest->bottom = MAX( src1->bottom, src2->bottom );	    
	}
    }
    return TRUE;
}


/***********************************************************************
 *           UnionRect32    (USER32.559)
 */
BOOL32 WINAPI UnionRect32( LPRECT32 dest, const RECT32 *src1,
                           const RECT32 *src2 )
{
    if (IsRectEmpty32(src1))
    {
	if (IsRectEmpty32(src2))
	{
	    SetRectEmpty32( dest );
	    return FALSE;
	}
	else *dest = *src2;
    }
    else
    {
	if (IsRectEmpty32(src2)) *dest = *src1;
	else
	{
	    dest->left   = MIN( src1->left, src2->left );
	    dest->right  = MAX( src1->right, src2->right );
	    dest->top    = MIN( src1->top, src2->top );
	    dest->bottom = MAX( src1->bottom, src2->bottom );	    
	}
    }
    return TRUE;
}


/***********************************************************************
 *           EqualRect16    (USER.244)
 */
BOOL16 WINAPI EqualRect16( const RECT16* rect1, const RECT16* rect2 )
{
    return ((rect1->left == rect2->left) && (rect1->right == rect2->right) &&
	    (rect1->top == rect2->top) && (rect1->bottom == rect2->bottom));
}


/***********************************************************************
 *           EqualRect32    (USER32.194)
 */
BOOL32 WINAPI EqualRect32( const RECT32* rect1, const RECT32* rect2 )
{
    return ((rect1->left == rect2->left) && (rect1->right == rect2->right) &&
	    (rect1->top == rect2->top) && (rect1->bottom == rect2->bottom));
}


/***********************************************************************
 *           SubtractRect16    (USER.373)
 */
BOOL16 WINAPI SubtractRect16( LPRECT16 dest, const RECT16 *src1,
                              const RECT16 *src2 )
{
    RECT16 tmp;

    if (IsRectEmpty16( src1 ))
    {
	SetRectEmpty16( dest );
	return FALSE;
    }
    *dest = *src1;
    if (IntersectRect16( &tmp, src1, src2 ))
    {
	if (EqualRect16( &tmp, dest ))
	{
	    SetRectEmpty16( dest );
	    return FALSE;
	}
	if ((tmp.top == dest->top) && (tmp.bottom == dest->bottom))
	{
	    if (tmp.left == dest->left) dest->left = tmp.right;
	    else if (tmp.right == dest->right) dest->right = tmp.left;
	}
	else if ((tmp.left == dest->left) && (tmp.right == dest->right))
	{
	    if (tmp.top == dest->top) dest->top = tmp.bottom;
	    else if (tmp.bottom == dest->bottom) dest->bottom = tmp.top;
	}
    }
    return TRUE;
}


/***********************************************************************
 *           SubtractRect32    (USER32.536)
 */
BOOL32 WINAPI SubtractRect32( LPRECT32 dest, const RECT32 *src1,
                              const RECT32 *src2 )
{
    RECT32 tmp;

    if (IsRectEmpty32( src1 ))
    {
	SetRectEmpty32( dest );
	return FALSE;
    }
    *dest = *src1;
    if (IntersectRect32( &tmp, src1, src2 ))
    {
	if (EqualRect32( &tmp, dest ))
	{
	    SetRectEmpty32( dest );
	    return FALSE;
	}
	if ((tmp.top == dest->top) && (tmp.bottom == dest->bottom))
	{
	    if (tmp.left == dest->left) dest->left = tmp.right;
	    else if (tmp.right == dest->right) dest->right = tmp.left;
	}
	else if ((tmp.left == dest->left) && (tmp.right == dest->right))
	{
	    if (tmp.top == dest->top) dest->top = tmp.bottom;
	    else if (tmp.bottom == dest->bottom) dest->bottom = tmp.top;
	}
    }
    return TRUE;
}
