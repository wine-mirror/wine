/*
 * Rectangle-related functions
 *
 * Copyright 1993 Alexandre Julliard
 *
 */

#include "windows.h"


/***********************************************************************
 *           SetRect    (USER.72)
 */
void SetRect( LPRECT rect, short left, short top, short right, short bottom )
{
    rect->left   = left;
    rect->right  = right;
    rect->top    = top;
    rect->bottom = bottom;
}


/***********************************************************************
 *           SetRectEmpty    (USER.73)
 */
void SetRectEmpty( LPRECT rect )
{
    rect->left = rect->right = rect->top = rect->bottom = 0;
}


/***********************************************************************
 *           CopyRect    (USER.74)
 */
BOOL CopyRect( LPRECT dest, LPRECT src )
{
    *dest = *src;
    return TRUE;
}


/***********************************************************************
 *           IsRectEmpty    (USER.75)
 */
BOOL IsRectEmpty( LPRECT rect )
{
    return ((rect->left == rect->right) || (rect->top == rect->bottom));
}


/***********************************************************************
 *           PtInRect    (USER.76)
 */
BOOL PtInRect( LPRECT rect, POINT pt )
{
    return ((pt.x >= rect->left) && (pt.x < rect->right) &&
	    (pt.y >= rect->top) && (pt.y < rect->bottom));
}


/***********************************************************************
 *           OffsetRect    (USER.77)
 */
void OffsetRect( LPRECT rect, short x, short y )
{
    rect->left   += x;
    rect->right  += x;
    rect->top    += y;
    rect->bottom += y;    
}


/***********************************************************************
 *           InflateRect    (USER.78)
 */
void InflateRect( LPRECT rect, short x, short y )
{
    rect->left   -= x;
    rect->top 	 -= y;
    rect->right  += x;
    rect->bottom += y;
}


/***********************************************************************
 *           IntersectRect    (USER.79)
 */
BOOL IntersectRect( LPRECT dest, LPRECT src1, LPRECT src2 )
{
    if (IsRectEmpty(src1) || IsRectEmpty(src2) ||
	(src1->left >= src2->right) || (src2->left >= src1->right) ||
	(src1->top >= src2->bottom) || (src2->top >= src1->bottom))
    {
	SetRectEmpty( dest );
	return FALSE;
    }
    dest->left   = MAX( src1->left, src2->left );
    dest->right  = MIN( src1->right, src2->right );
    dest->top    = MAX( src1->top, src2->top );
    dest->bottom = MIN( src1->bottom, src2->bottom );
    return TRUE;
}


/***********************************************************************
 *           UnionRect    (USER.80)
 */
BOOL UnionRect( LPRECT dest, LPRECT src1, LPRECT src2 )
{
    if (IsRectEmpty(src1))
    {
	if (IsRectEmpty(src2))
	{
	    SetRectEmpty( dest );
	    return FALSE;
	}
	else *dest = *src2;
    }
    else
    {
	if (IsRectEmpty(src2)) *dest = *src1;
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
 *           EqualRect    (USER.244)
 */
BOOL EqualRect( const RECT* rect1, const RECT* rect2 )
{
    return ((rect1->left == rect2->left) && (rect1->right == rect2->right) &&
	    (rect1->top == rect2->top) && (rect1->bottom == rect2->bottom));
}


/***********************************************************************
 *           SubtractRect    (USER.373)
 */
BOOL SubtractRect( LPRECT dest, LPRECT src1, LPRECT src2 )
{
    RECT tmp;

    if (IsRectEmpty( src1 ))
    {
	SetRectEmpty( dest );
	return FALSE;
    }
    *dest = *src1;
    if (IntersectRect( &tmp, src1, src2 ))
    {
	if (EqualRect( &tmp, dest ))
	{
	    SetRectEmpty( dest );
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

