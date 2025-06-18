/*
 * SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
 * Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice including the dates of first publication and
 * either this permission notice or a reference to
 * http://oss.sgi.com/projects/FreeB/
 * shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * SILICON GRAPHICS, INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of Silicon Graphics, Inc.
 * shall not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization from
 * Silicon Graphics, Inc.
 */
/*
** Author: Eric Veach, July 1994.
**
*/

#include <stdarg.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"

#include "tess.h"
#include "mesh.h"

#define GLU_TESS_DEFAULT_TOLERANCE 0.0
#define GLU_TESS_MESH		100112	/* void (*)(GLUmesh *mesh)	    */

/*ARGSUSED*/ static void GLAPIENTRY noBegin( GLenum type ) {}
/*ARGSUSED*/ static void GLAPIENTRY noEdgeFlag( GLboolean boundaryEdge ) {}
/*ARGSUSED*/ static void GLAPIENTRY noVertex( void *data ) {}
/*ARGSUSED*/ static void GLAPIENTRY noEnd( void ) {}
/*ARGSUSED*/ static void GLAPIENTRY noError( GLenum errnum ) {}
/*ARGSUSED*/ static void GLAPIENTRY noCombine( GLdouble coords[3], void *data[4],
				    GLfloat weight[4], void **dataOut ) {}
/*ARGSUSED*/ static void GLAPIENTRY noMesh( GLUmesh *mesh ) {}


/*ARGSUSED*/ void GLAPIENTRY __gl_noBeginData( GLenum type,
					     void *polygonData ) {}
/*ARGSUSED*/ void GLAPIENTRY __gl_noEdgeFlagData( GLboolean boundaryEdge,
				       void *polygonData ) {}
/*ARGSUSED*/ void GLAPIENTRY __gl_noVertexData( void *data,
					      void *polygonData ) {}
/*ARGSUSED*/ void GLAPIENTRY __gl_noEndData( void *polygonData ) {}
/*ARGSUSED*/ void GLAPIENTRY __gl_noErrorData( GLenum errnum,
					     void *polygonData ) {}
/*ARGSUSED*/ void GLAPIENTRY __gl_noCombineData( GLdouble coords[3],
					       void *data[4],
					       GLfloat weight[4],
					       void **outData,
					       void *polygonData ) {}

/* Half-edges are allocated in pairs (see mesh.c) */
typedef struct { GLUhalfEdge e, eSym; } EdgePair;

#undef	MAX
#define MAX(a,b)	((a) > (b) ? (a) : (b))
#define MAX_FAST_ALLOC	(MAX(sizeof(EdgePair), \
                         MAX(sizeof(GLUvertex),sizeof(GLUface))))

/* normal support (used to be in normal.c) */

#define Dot(u,v)	(u[0]*v[0] + u[1]*v[1] + u[2]*v[2])

#undef	ABS
#define ABS(x)	((x) < 0 ? -(x) : (x))

static int LongAxis( GLdouble v[3] )
{
  int i = 0;

  if( ABS(v[1]) > ABS(v[0]) ) { i = 1; }
  if( ABS(v[2]) > ABS(v[i]) ) { i = 2; }
  return i;
}

static void ComputeNormal( GLUtesselator *tess, GLdouble norm[3] )
{
  GLUvertex *v, *v1, *v2;
  GLdouble c, tLen2, maxLen2;
  GLdouble maxVal[3], minVal[3], d1[3], d2[3], tNorm[3];
  GLUvertex *maxVert[3], *minVert[3];
  GLUvertex *vHead = &tess->mesh->vHead;
  int i;

  maxVal[0] = maxVal[1] = maxVal[2] = -2 * GLU_TESS_MAX_COORD;
  minVal[0] = minVal[1] = minVal[2] = 2 * GLU_TESS_MAX_COORD;

  for( v = vHead->next; v != vHead; v = v->next ) {
    for( i = 0; i < 3; ++i ) {
      c = v->coords[i];
      if( c < minVal[i] ) { minVal[i] = c; minVert[i] = v; }
      if( c > maxVal[i] ) { maxVal[i] = c; maxVert[i] = v; }
    }
  }

  /* Find two vertices separated by at least 1/sqrt(3) of the maximum
   * distance between any two vertices
   */
  i = 0;
  if( maxVal[1] - minVal[1] > maxVal[0] - minVal[0] ) { i = 1; }
  if( maxVal[2] - minVal[2] > maxVal[i] - minVal[i] ) { i = 2; }
  if( minVal[i] >= maxVal[i] ) {
    /* All vertices are the same -- normal doesn't matter */
    norm[0] = 0; norm[1] = 0; norm[2] = 1;
    return;
  }

  /* Look for a third vertex which forms the triangle with maximum area
   * (Length of normal == twice the triangle area)
   */
  maxLen2 = 0;
  v1 = minVert[i];
  v2 = maxVert[i];
  d1[0] = v1->coords[0] - v2->coords[0];
  d1[1] = v1->coords[1] - v2->coords[1];
  d1[2] = v1->coords[2] - v2->coords[2];
  for( v = vHead->next; v != vHead; v = v->next ) {
    d2[0] = v->coords[0] - v2->coords[0];
    d2[1] = v->coords[1] - v2->coords[1];
    d2[2] = v->coords[2] - v2->coords[2];
    tNorm[0] = d1[1]*d2[2] - d1[2]*d2[1];
    tNorm[1] = d1[2]*d2[0] - d1[0]*d2[2];
    tNorm[2] = d1[0]*d2[1] - d1[1]*d2[0];
    tLen2 = tNorm[0]*tNorm[0] + tNorm[1]*tNorm[1] + tNorm[2]*tNorm[2];
    if( tLen2 > maxLen2 ) {
      maxLen2 = tLen2;
      norm[0] = tNorm[0];
      norm[1] = tNorm[1];
      norm[2] = tNorm[2];
    }
  }

  if( maxLen2 <= 0 ) {
    /* All points lie on a single line -- any decent normal will do */
    norm[0] = norm[1] = norm[2] = 0;
    norm[LongAxis(d1)] = 1;
  }
}


static void CheckOrientation( GLUtesselator *tess )
{
  GLdouble area;
  GLUface *f, *fHead = &tess->mesh->fHead;
  GLUvertex *v, *vHead = &tess->mesh->vHead;
  GLUhalfEdge *e;

  /* When we compute the normal automatically, we choose the orientation
   * so that the sum of the signed areas of all contours is non-negative.
   */
  area = 0;
  for( f = fHead->next; f != fHead; f = f->next ) {
    e = f->anEdge;
    if( e->winding <= 0 ) continue;
    do {
      area += (e->Org->s - e->Dst->s) * (e->Org->t + e->Dst->t);
      e = e->Lnext;
    } while( e != f->anEdge );
  }
  if( area < 0 ) {
    /* Reverse the orientation by flipping all the t-coordinates */
    for( v = vHead->next; v != vHead; v = v->next ) {
      v->t = - v->t;
    }
    tess->tUnit[0] = - tess->tUnit[0];
    tess->tUnit[1] = - tess->tUnit[1];
    tess->tUnit[2] = - tess->tUnit[2];
  }
}

#if defined(SLANTED_SWEEP)
/* The "feature merging" is not intended to be complete.  There are
 * special cases where edges are nearly parallel to the sweep line
 * which are not implemented.  The algorithm should still behave
 * robustly (ie. produce a reasonable tessellation) in the presence
 * of such edges, however it may miss features which could have been
 * merged.  We could minimize this effect by choosing the sweep line
 * direction to be something unusual (ie. not parallel to one of the
 * coordinate axes).
 */
#define S_UNIT_X	0.50941539564955385	/* Pre-normalized */
#define S_UNIT_Y	0.86052074622010633
#else
#define S_UNIT_X	1.0
#define S_UNIT_Y	0.0
#endif

/* Determine the polygon normal and project vertices onto the plane
 * of the polygon.
 */
static void __gl_projectPolygon( GLUtesselator *tess )
{
  GLUvertex *v, *vHead = &tess->mesh->vHead;
  GLdouble norm[3];
  GLdouble *sUnit, *tUnit;
  int i, computedNormal = FALSE;

  norm[0] = tess->normal[0];
  norm[1] = tess->normal[1];
  norm[2] = tess->normal[2];
  if( norm[0] == 0 && norm[1] == 0 && norm[2] == 0 ) {
    ComputeNormal( tess, norm );
    computedNormal = TRUE;
  }
  sUnit = tess->sUnit;
  tUnit = tess->tUnit;
  i = LongAxis( norm );

  /* Project perpendicular to a coordinate axis -- better numerically */
  sUnit[i] = 0;
  sUnit[(i+1)%3] = S_UNIT_X;
  sUnit[(i+2)%3] = S_UNIT_Y;

  tUnit[i] = 0;
  tUnit[(i+1)%3] = (norm[i] > 0) ? -S_UNIT_Y : S_UNIT_Y;
  tUnit[(i+2)%3] = (norm[i] > 0) ? S_UNIT_X : -S_UNIT_X;

  /* Project the vertices onto the sweep plane */
  for( v = vHead->next; v != vHead; v = v->next ) {
    v->s = Dot( v->coords, sUnit );
    v->t = Dot( v->coords, tUnit );
  }
  if( computedNormal ) {
    CheckOrientation( tess );
  }
}


/***********************************************************************
 *		gluNewTess (GLU32.@)
 */
GLUtesselator * WINAPI gluNewTess(void)
{
  GLUtesselator *tess;

  /* Only initialize fields which can be changed by the api.  Other fields
   * are initialized where they are used.
   */

  tess = HeapAlloc(GetProcessHeap(), 0, sizeof( GLUtesselator ));
  if (tess == NULL) {
     return 0;			/* out of memory */
  }

  tess->state = T_DORMANT;

  tess->normal[0] = 0;
  tess->normal[1] = 0;
  tess->normal[2] = 0;

  tess->relTolerance = GLU_TESS_DEFAULT_TOLERANCE;
  tess->windingRule = GLU_TESS_WINDING_ODD;
  tess->flagBoundary = FALSE;
  tess->boundaryOnly = FALSE;

  tess->callBegin = &noBegin;
  tess->callEdgeFlag = &noEdgeFlag;
  tess->callVertex = &noVertex;
  tess->callEnd = &noEnd;

  tess->callError = &noError;
  tess->callCombine = &noCombine;
  tess->callMesh = &noMesh;

  tess->callBeginData= &__gl_noBeginData;
  tess->callEdgeFlagData= &__gl_noEdgeFlagData;
  tess->callVertexData= &__gl_noVertexData;
  tess->callEndData= &__gl_noEndData;
  tess->callErrorData= &__gl_noErrorData;
  tess->callCombineData= &__gl_noCombineData;

  tess->polygonData= NULL;

  return tess;
}

static void MakeDormant( GLUtesselator *tess )
{
  /* Return the tessellator to its original dormant state. */

  if( tess->mesh != NULL ) {
    __gl_meshDeleteMesh( tess->mesh );
  }
  tess->state = T_DORMANT;
  tess->lastEdge = NULL;
  tess->mesh = NULL;
}

#define RequireState( tess, s )   if( tess->state != s ) GotoState(tess,s)

static void GotoState( GLUtesselator *tess, enum TessState newState )
{
  while( tess->state != newState ) {
    /* We change the current state one level at a time, to get to
     * the desired state.
     */
    if( tess->state < newState ) {
      switch( tess->state ) {
      case T_DORMANT:
	CALL_ERROR_OR_ERROR_DATA( GLU_TESS_MISSING_BEGIN_POLYGON );
	gluTessBeginPolygon( tess, NULL );
	break;
      case T_IN_POLYGON:
	CALL_ERROR_OR_ERROR_DATA( GLU_TESS_MISSING_BEGIN_CONTOUR );
	gluTessBeginContour( tess );
	break;
      default:
	 ;
      }
    } else {
      switch( tess->state ) {
      case T_IN_CONTOUR:
	CALL_ERROR_OR_ERROR_DATA( GLU_TESS_MISSING_END_CONTOUR );
	gluTessEndContour( tess );
	break;
      case T_IN_POLYGON:
	CALL_ERROR_OR_ERROR_DATA( GLU_TESS_MISSING_END_POLYGON );
	/* gluTessEndPolygon( tess ) is too much work! */
	MakeDormant( tess );
	break;
      default:
	 ;
      }
    }
  }
}


/***********************************************************************
 *		gluDeleteTess (GLU32.@)
 */
void WINAPI gluDeleteTess( GLUtesselator *tess )
{
  RequireState( tess, T_DORMANT );
  HeapFree( GetProcessHeap(), 0, tess );
}


/***********************************************************************
 *		gluTessProperty (GLU32.@)
 */
void WINAPI gluTessProperty( GLUtesselator *tess, GLenum which, GLdouble value )
{
  GLenum windingRule;

  switch( which ) {
  case GLU_TESS_TOLERANCE:
    if( value < 0.0 || value > 1.0 ) break;
    tess->relTolerance = value;
    return;

  case GLU_TESS_WINDING_RULE:
    windingRule = (GLenum) value;
    if( windingRule != value ) break;	/* not an integer */

    switch( windingRule ) {
    case GLU_TESS_WINDING_ODD:
    case GLU_TESS_WINDING_NONZERO:
    case GLU_TESS_WINDING_POSITIVE:
    case GLU_TESS_WINDING_NEGATIVE:
    case GLU_TESS_WINDING_ABS_GEQ_TWO:
      tess->windingRule = windingRule;
      return;
    default:
      break;
    }

  case GLU_TESS_BOUNDARY_ONLY:
    tess->boundaryOnly = (value != 0);
    return;

  default:
    CALL_ERROR_OR_ERROR_DATA( GLU_INVALID_ENUM );
    return;
  }
  CALL_ERROR_OR_ERROR_DATA( GLU_INVALID_VALUE );
}

/* Returns tessellator property */
/***********************************************************************
 *		gluGetTessProperty (GLU32.@)
 */
void WINAPI gluGetTessProperty( GLUtesselator *tess, GLenum which, GLdouble *value )
{
   switch (which) {
   case GLU_TESS_TOLERANCE:
      /* tolerance should be in range [0..1] */
      assert(0.0 <= tess->relTolerance && tess->relTolerance <= 1.0);
      *value= tess->relTolerance;
      break;
   case GLU_TESS_WINDING_RULE:
      assert(tess->windingRule == GLU_TESS_WINDING_ODD ||
	     tess->windingRule == GLU_TESS_WINDING_NONZERO ||
	     tess->windingRule == GLU_TESS_WINDING_POSITIVE ||
	     tess->windingRule == GLU_TESS_WINDING_NEGATIVE ||
	     tess->windingRule == GLU_TESS_WINDING_ABS_GEQ_TWO);
      *value= tess->windingRule;
      break;
   case GLU_TESS_BOUNDARY_ONLY:
      assert(tess->boundaryOnly == TRUE || tess->boundaryOnly == FALSE);
      *value= tess->boundaryOnly;
      break;
   default:
      *value= 0.0;
      CALL_ERROR_OR_ERROR_DATA( GLU_INVALID_ENUM );
      break;
   }
} /* gluGetTessProperty() */

/***********************************************************************
 *		gluTessNormal (GLU32.@)
 */
void WINAPI gluTessNormal( GLUtesselator *tess, GLdouble x, GLdouble y, GLdouble z )
{
  tess->normal[0] = x;
  tess->normal[1] = y;
  tess->normal[2] = z;
}

/***********************************************************************
 *		gluTessCallback (GLU32.@)
 */
void WINAPI gluTessCallback( GLUtesselator *tess, GLenum which, void (CALLBACK *fn)(void))
{
  switch( which ) {
  case GLU_TESS_BEGIN:
    tess->callBegin = (fn == NULL) ? &noBegin : (void (GLAPIENTRY *)(GLenum)) fn;
    return;
  case GLU_TESS_BEGIN_DATA:
    tess->callBeginData = (fn == NULL) ?
	&__gl_noBeginData : (void (GLAPIENTRY *)(GLenum, void *)) fn;
    return;
  case GLU_TESS_EDGE_FLAG:
    tess->callEdgeFlag = (fn == NULL) ? &noEdgeFlag :
					(void (GLAPIENTRY *)(GLboolean)) fn;
    /* If the client wants boundary edges to be flagged,
     * we render everything as separate triangles (no strips or fans).
     */
    tess->flagBoundary = (fn != NULL);
    return;
  case GLU_TESS_EDGE_FLAG_DATA:
    tess->callEdgeFlagData= (fn == NULL) ?
	&__gl_noEdgeFlagData : (void (GLAPIENTRY *)(GLboolean, void *)) fn;
    /* If the client wants boundary edges to be flagged,
     * we render everything as separate triangles (no strips or fans).
     */
    tess->flagBoundary = (fn != NULL);
    return;
  case GLU_TESS_VERTEX:
    tess->callVertex = (fn == NULL) ? &noVertex :
				      (void (GLAPIENTRY *)(void *)) fn;
    return;
  case GLU_TESS_VERTEX_DATA:
    tess->callVertexData = (fn == NULL) ?
	&__gl_noVertexData : (void (GLAPIENTRY *)(void *, void *)) fn;
    return;
  case GLU_TESS_END:
    tess->callEnd = (fn == NULL) ? &noEnd : (void (GLAPIENTRY *)(void)) fn;
    return;
  case GLU_TESS_END_DATA:
    tess->callEndData = (fn == NULL) ? &__gl_noEndData :
				       (void (GLAPIENTRY *)(void *)) fn;
    return;
  case GLU_TESS_ERROR:
    tess->callError = (fn == NULL) ? &noError : (void (GLAPIENTRY *)(GLenum)) fn;
    return;
  case GLU_TESS_ERROR_DATA:
    tess->callErrorData = (fn == NULL) ?
	&__gl_noErrorData : (void (GLAPIENTRY *)(GLenum, void *)) fn;
    return;
  case GLU_TESS_COMBINE:
    tess->callCombine = (fn == NULL) ? &noCombine :
	(void (GLAPIENTRY *)(GLdouble [3],void *[4], GLfloat [4], void ** )) fn;
    return;
  case GLU_TESS_COMBINE_DATA:
    tess->callCombineData = (fn == NULL) ? &__gl_noCombineData :
					   (void (GLAPIENTRY *)(GLdouble [3],
						     void *[4],
						     GLfloat [4],
						     void **,
						     void *)) fn;
    return;
  case GLU_TESS_MESH:
    tess->callMesh = (fn == NULL) ? &noMesh : (void (GLAPIENTRY *)(GLUmesh *)) fn;
    return;
  default:
    CALL_ERROR_OR_ERROR_DATA( GLU_INVALID_ENUM );
    return;
  }
}

static int AddVertex( GLUtesselator *tess, GLdouble coords[3], void *data )
{
  GLUhalfEdge *e;

  e = tess->lastEdge;
  if( e == NULL ) {
    /* Make a self-loop (one vertex, one edge). */

    e = __gl_meshMakeEdge( tess->mesh );
    if (e == NULL) return 0;
    if ( !__gl_meshSplice( e, e->Sym ) ) return 0;
  } else {
    /* Create a new vertex and edge which immediately follow e
     * in the ordering around the left face.
     */
    if (__gl_meshSplitEdge( e ) == NULL) return 0;
    e = e->Lnext;
  }

  /* The new vertex is now e->Org. */
  e->Org->data = data;
  e->Org->coords[0] = coords[0];
  e->Org->coords[1] = coords[1];
  e->Org->coords[2] = coords[2];

  /* The winding of an edge says how the winding number changes as we
   * cross from the edge''s right face to its left face.  We add the
   * vertices in such an order that a CCW contour will add +1 to
   * the winding number of the region inside the contour.
   */
  e->winding = 1;
  e->Sym->winding = -1;

  tess->lastEdge = e;

  return 1;
}


static void CacheVertex( GLUtesselator *tess, GLdouble coords[3], void *data )
{
  CachedVertex *v = &tess->cache[tess->cacheCount];

  v->data = data;
  v->coords[0] = coords[0];
  v->coords[1] = coords[1];
  v->coords[2] = coords[2];
  ++tess->cacheCount;
}


static int EmptyCache( GLUtesselator *tess )
{
  CachedVertex *v = tess->cache;
  CachedVertex *vLast;

  tess->mesh = __gl_meshNewMesh();
  if (tess->mesh == NULL) return 0;

  for( vLast = v + tess->cacheCount; v < vLast; ++v ) {
    if ( !AddVertex( tess, v->coords, v->data ) ) return 0;
  }
  tess->cacheCount = 0;
  tess->emptyCache = FALSE;

  return 1;
}


/***********************************************************************
 *		gluTessVertex (GLU32.@)
 */
void WINAPI gluTessVertex( GLUtesselator *tess, GLdouble coords[3], void *data )
{
  int i, tooLarge = FALSE;
  GLdouble x, clamped[3];

  RequireState( tess, T_IN_CONTOUR );

  if( tess->emptyCache ) {
    if ( !EmptyCache( tess ) ) {
       CALL_ERROR_OR_ERROR_DATA( GLU_OUT_OF_MEMORY );
       return;
    }
    tess->lastEdge = NULL;
  }
  for( i = 0; i < 3; ++i ) {
    x = coords[i];
    if( x < - GLU_TESS_MAX_COORD ) {
      x = - GLU_TESS_MAX_COORD;
      tooLarge = TRUE;
    }
    if( x > GLU_TESS_MAX_COORD ) {
      x = GLU_TESS_MAX_COORD;
      tooLarge = TRUE;
    }
    clamped[i] = x;
  }
  if( tooLarge ) {
    CALL_ERROR_OR_ERROR_DATA( GLU_TESS_COORD_TOO_LARGE );
  }

  if( tess->mesh == NULL ) {
    if( tess->cacheCount < TESS_MAX_CACHE ) {
      CacheVertex( tess, clamped, data );
      return;
    }
    if ( !EmptyCache( tess ) ) {
       CALL_ERROR_OR_ERROR_DATA( GLU_OUT_OF_MEMORY );
       return;
    }
  }
  if ( !AddVertex( tess, clamped, data ) ) {
       CALL_ERROR_OR_ERROR_DATA( GLU_OUT_OF_MEMORY );
  }
}


/***********************************************************************
 *		gluTessBeginPolygon (GLU32.@)
 */
void WINAPI gluTessBeginPolygon( GLUtesselator *tess, void *data )
{
  RequireState( tess, T_DORMANT );

  tess->state = T_IN_POLYGON;
  tess->cacheCount = 0;
  tess->emptyCache = FALSE;
  tess->mesh = NULL;

  tess->polygonData= data;
}


/***********************************************************************
 *		gluTessBeginContour (GLU32.@)
 */
void WINAPI gluTessBeginContour( GLUtesselator *tess )
{
  RequireState( tess, T_IN_POLYGON );

  tess->state = T_IN_CONTOUR;
  tess->lastEdge = NULL;
  if( tess->cacheCount > 0 ) {
    /* Just set a flag so we don't get confused by empty contours
     * -- these can be generated accidentally with the obsolete
     * NextContour() interface.
     */
    tess->emptyCache = TRUE;
  }
}


/***********************************************************************
 *		gluTessEndContour (GLU32.@)
 */
void WINAPI gluTessEndContour( GLUtesselator *tess )
{
  RequireState( tess, T_IN_CONTOUR );
  tess->state = T_IN_POLYGON;
}

/***********************************************************************
 *		gluTessEndPolygon (GLU32.@)
 */
void WINAPI gluTessEndPolygon( GLUtesselator *tess )
{
  GLUmesh *mesh;

  if (setjmp(tess->env) != 0) {
     /* come back here if out of memory */
     CALL_ERROR_OR_ERROR_DATA( GLU_OUT_OF_MEMORY );
     return;
  }

  RequireState( tess, T_IN_POLYGON );
  tess->state = T_DORMANT;

  if( tess->mesh == NULL ) {
    if( ! tess->flagBoundary && tess->callMesh == &noMesh ) {

      /* Try some special code to make the easy cases go quickly
       * (eg. convex polygons).  This code does NOT handle multiple contours,
       * intersections, edge flags, and of course it does not generate
       * an explicit mesh either.
       */
      if( __gl_renderCache( tess )) {
	tess->polygonData= NULL;
	return;
      }
    }
    if ( !EmptyCache( tess ) ) longjmp(tess->env,1); /* could've used a label*/
  }

  /* Determine the polygon normal and project vertices onto the plane
   * of the polygon.
   */
  __gl_projectPolygon( tess );

  /* __gl_computeInterior( tess ) computes the planar arrangement specified
   * by the given contours, and further subdivides this arrangement
   * into regions.  Each region is marked "inside" if it belongs
   * to the polygon, according to the rule given by tess->windingRule.
   * Each interior region is guaranteed be monotone.
   */
  if ( !__gl_computeInterior( tess ) ) {
     longjmp(tess->env,1);	/* could've used a label */
  }

  mesh = tess->mesh;
  if( ! tess->fatalError ) {
    int rc = 1;

    /* If the user wants only the boundary contours, we throw away all edges
     * except those which separate the interior from the exterior.
     * Otherwise we tessellate all the regions marked "inside".
     */
    if( tess->boundaryOnly ) {
      rc = __gl_meshSetWindingNumber( mesh, 1, TRUE );
    } else {
      rc = __gl_meshTessellateInterior( mesh );
    }
    if (rc == 0) longjmp(tess->env,1);	/* could've used a label */

    __gl_meshCheckMesh( mesh );

    if( tess->callBegin != &noBegin || tess->callEnd != &noEnd
       || tess->callVertex != &noVertex || tess->callEdgeFlag != &noEdgeFlag
       || tess->callBeginData != &__gl_noBeginData
       || tess->callEndData != &__gl_noEndData
       || tess->callVertexData != &__gl_noVertexData
       || tess->callEdgeFlagData != &__gl_noEdgeFlagData )
    {
      if( tess->boundaryOnly ) {
	__gl_renderBoundary( tess, mesh );  /* output boundary contours */
      } else {
	__gl_renderMesh( tess, mesh );	   /* output strips and fans */
      }
    }
    if( tess->callMesh != &noMesh ) {

      /* Throw away the exterior faces, so that all faces are interior.
       * This way the user doesn't have to check the "inside" flag,
       * and we don't need to even reveal its existence.  It also leaves
       * the freedom for an implementation to not generate the exterior
       * faces in the first place.
       */
      __gl_meshDiscardExterior( mesh );
      (*tess->callMesh)( mesh );		/* user wants the mesh itself */
      tess->mesh = NULL;
      tess->polygonData= NULL;
      return;
    }
  }
  __gl_meshDeleteMesh( mesh );
  tess->polygonData= NULL;
  tess->mesh = NULL;
}


/*XXXblythe unused function*/
#if 0
void GLAPIENTRY
gluDeleteMesh( GLUmesh *mesh )
{
  __gl_meshDeleteMesh( mesh );
}
#endif



/*******************************************************/

/* Obsolete calls -- for backward compatibility */

/***********************************************************************
 *		gluBeginPolygon (GLU32.@)
 */
void WINAPI gluBeginPolygon( GLUtesselator *tess )
{
  gluTessBeginPolygon( tess, NULL );
  gluTessBeginContour( tess );
}

/***********************************************************************
 *		gluNextContour (GLU32.@)
 */
void WINAPI gluNextContour( GLUtesselator *tess, GLenum type )
{
  gluTessEndContour( tess );
  gluTessBeginContour( tess );
}

/***********************************************************************
 *		gluEndPolygon (GLU32.@)
 */
void WINAPI gluEndPolygon( GLUtesselator *tess )
{
  gluTessEndContour( tess );
  gluTessEndPolygon( tess );
}
