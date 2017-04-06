/*
 * Copyright 2001 Marcus Meissner
 * Copyright 2017 Alexandre Julliard
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"

#include "wine/wgl.h"
#include "wine/library.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(glu);

typedef struct GLUnurbs GLUnurbs;
typedef struct GLUquadric GLUquadric;
typedef struct GLUtesselator GLUtesselator;
typedef void (*_GLUfuncptr)(void);

/* The only non-trivial bit of this is the *Tess* functions.  Here we
   need to wrap the callbacks up in a thunk to switch calling
   conventions, so we use our own tesselator type to store the
   application's callbacks.  wine_gluTessCallback always sets the
   *_DATA type of callback so that we have access to the polygon_data
   (which is in fact just wine_GLUtesselator), in the thunk itself we can
   check whether we should call the _DATA or non _DATA type. */

typedef struct {
    GLUtesselator *tess;
    void *polygon_data;
    void (CALLBACK *cb_tess_begin)(int);
    void (CALLBACK *cb_tess_begin_data)(int, void *);
    void (CALLBACK *cb_tess_vertex)(void *);
    void (CALLBACK *cb_tess_vertex_data)(void *, void *);
    void (CALLBACK *cb_tess_end)(void);
    void (CALLBACK *cb_tess_end_data)(void *);
    void (CALLBACK *cb_tess_error)(int);
    void (CALLBACK *cb_tess_error_data)(int, void *);
    void (CALLBACK *cb_tess_edge_flag)(int);
    void (CALLBACK *cb_tess_edge_flag_data)(int, void *);
    void (CALLBACK *cb_tess_combine)(double *, void *, float *, void **);
    void (CALLBACK *cb_tess_combine_data)(double *, void *, float *, void **, void *);
} wine_GLUtesselator;

#define GLU_TESS_BEGIN          100100
#define GLU_TESS_VERTEX         100101
#define GLU_TESS_END            100102
#define GLU_TESS_ERROR          100103
#define GLU_TESS_EDGE_FLAG      100104
#define GLU_TESS_COMBINE        100105
#define GLU_TESS_BEGIN_DATA     100106
#define GLU_TESS_VERTEX_DATA    100107
#define GLU_TESS_END_DATA       100108
#define GLU_TESS_ERROR_DATA     100109
#define GLU_TESS_EDGE_FLAG_DATA 100110
#define GLU_TESS_COMBINE_DATA   100111

static void  (*p_gluBeginCurve)( GLUnurbs* nurb );
static void  (*p_gluBeginSurface)( GLUnurbs* nurb );
static void  (*p_gluBeginTrim)( GLUnurbs* nurb );
static GLint (*p_gluBuild1DMipmaps)( GLenum target, GLint internalFormat, GLsizei width, GLenum format, GLenum type, const void *data );
static GLint (*p_gluBuild2DMipmaps)( GLenum target, GLint internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *data );
static void  (*p_gluCylinder)( GLUquadric* quad, GLdouble base, GLdouble top, GLdouble height, GLint slices, GLint stacks );
static void  (*p_gluDeleteNurbsRenderer)( GLUnurbs* nurb );
static void  (*p_gluDeleteQuadric)( GLUquadric* quad );
static void  (*p_gluDeleteTess)( GLUtesselator* tess );
static void  (*p_gluDisk)( GLUquadric* quad, GLdouble inner, GLdouble outer, GLint slices, GLint loops );
static void  (*p_gluEndCurve)( GLUnurbs* nurb );
static void  (*p_gluEndSurface)( GLUnurbs* nurb );
static void  (*p_gluEndTrim)( GLUnurbs* nurb );
static const GLubyte * (*p_gluErrorString)( GLenum error );
static void  (*p_gluGetNurbsProperty)( GLUnurbs* nurb, GLenum property, GLfloat* data );
static const GLubyte * (*p_gluGetString)( GLenum name );
static void  (*p_gluLoadSamplingMatrices)( GLUnurbs* nurb, const GLfloat *model, const GLfloat *perspective, const GLint *view );
static void  (*p_gluLookAt)( GLdouble eyeX, GLdouble eyeY, GLdouble eyeZ, GLdouble centerX, GLdouble centerY, GLdouble centerZ, GLdouble upX, GLdouble upY, GLdouble upZ );
static GLUnurbs* (*p_gluNewNurbsRenderer)(void);
static GLUquadric* (*p_gluNewQuadric)(void);
static GLUtesselator* (*p_gluNewTess)(void);
static void  (*p_gluNurbsCallback)( GLUnurbs* nurb, GLenum which, _GLUfuncptr CallBackFunc );
static void  (*p_gluNurbsCurve)( GLUnurbs* nurb, GLint knotCount, GLfloat *knots, GLint stride, GLfloat *control, GLint order, GLenum type );
static void  (*p_gluNurbsProperty)( GLUnurbs* nurb, GLenum property, GLfloat value );
static void  (*p_gluNurbsSurface)( GLUnurbs* nurb, GLint sKnotCount, GLfloat* sKnots, GLint tKnotCount, GLfloat* tKnots, GLint sStride, GLint tStride, GLfloat* control, GLint sOrder, GLint tOrder, GLenum type );
static void  (*p_gluOrtho2D)( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top );
static void  (*p_gluPartialDisk)( GLUquadric* quad, GLdouble inner, GLdouble outer, GLint slices, GLint loops, GLdouble start, GLdouble sweep );
static void  (*p_gluPerspective)( GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar );
static void  (*p_gluPickMatrix)( GLdouble x, GLdouble y, GLdouble delX, GLdouble delY, GLint *viewport );
static GLint (*p_gluProject)( GLdouble objX, GLdouble objY, GLdouble objZ, const GLdouble *model, const GLdouble *proj, const GLint *view, GLdouble* winX, GLdouble* winY, GLdouble* winZ );
static void  (*p_gluPwlCurve)( GLUnurbs* nurb, GLint count, GLfloat* data, GLint stride, GLenum type );
static void  (*p_gluQuadricCallback)( GLUquadric* quad, GLenum which, _GLUfuncptr CallBackFunc );
static void  (*p_gluQuadricDrawStyle)( GLUquadric* quad, GLenum draw );
static void  (*p_gluQuadricNormals)( GLUquadric* quad, GLenum normal );
static void  (*p_gluQuadricOrientation)( GLUquadric* quad, GLenum orientation );
static void  (*p_gluQuadricTexture)( GLUquadric* quad, GLboolean texture );
static GLint (*p_gluScaleImage)( GLenum format, GLsizei wIn, GLsizei hIn, GLenum typeIn, const void *dataIn, GLsizei wOut, GLsizei hOut, GLenum typeOut, GLvoid* dataOut );
static void  (*p_gluSphere)( GLUquadric* quad, GLdouble radius, GLint slices, GLint stacks );
static void  (*p_gluTessBeginContour)( GLUtesselator* tess );
static void  (*p_gluTessBeginPolygon)( GLUtesselator* tess, GLvoid* data );
static void  (*p_gluTessCallback)( GLUtesselator* tess, GLenum which, _GLUfuncptr CallBackFunc );
static void  (*p_gluTessEndContour)( GLUtesselator* tess );
static void  (*p_gluTessEndPolygon)( GLUtesselator* tess );
static void  (*p_gluTessNormal)( GLUtesselator* tess, GLdouble valueX, GLdouble valueY, GLdouble valueZ );
static void  (*p_gluTessProperty)( GLUtesselator* tess, GLenum which, GLdouble data );
static void  (*p_gluTessVertex)( GLUtesselator* tess, GLdouble *location, GLvoid* data );
static GLint (*p_gluUnProject)( GLdouble winX, GLdouble winY, GLdouble winZ, const GLdouble *model, const GLdouble *proj, const GLint *view, GLdouble* objX, GLdouble* objY, GLdouble* objZ );

static BOOL load_libglu(void)
{
    char error[256];
    void *handle = wine_dlopen( SONAME_LIBGLU, RTLD_NOW, error, sizeof(error) );

    if (!handle)
    {
        ERR( "Failed to load %s: %s\n", SONAME_LIBGLU, error );
        return FALSE;
    }
#define LOAD_FUNCPTR(f) if (!(p_##f = wine_dlsym( handle, #f, NULL, 0 ))) { ERR( "Can't find %s in %s\n", #f, SONAME_LIBGLU ); return FALSE; }
    LOAD_FUNCPTR(gluBeginCurve);
    LOAD_FUNCPTR(gluBeginSurface);
    LOAD_FUNCPTR(gluBeginTrim);
    LOAD_FUNCPTR(gluBuild1DMipmaps);
    LOAD_FUNCPTR(gluBuild2DMipmaps);
    LOAD_FUNCPTR(gluCylinder);
    LOAD_FUNCPTR(gluDeleteNurbsRenderer);
    LOAD_FUNCPTR(gluDeleteQuadric);
    LOAD_FUNCPTR(gluDeleteTess);
    LOAD_FUNCPTR(gluDisk);
    LOAD_FUNCPTR(gluEndCurve);
    LOAD_FUNCPTR(gluEndSurface);
    LOAD_FUNCPTR(gluEndTrim);
    LOAD_FUNCPTR(gluErrorString);
    LOAD_FUNCPTR(gluGetNurbsProperty);
    LOAD_FUNCPTR(gluGetString);
    LOAD_FUNCPTR(gluLoadSamplingMatrices);
    LOAD_FUNCPTR(gluLookAt);
    LOAD_FUNCPTR(gluNewNurbsRenderer);
    LOAD_FUNCPTR(gluNewQuadric);
    LOAD_FUNCPTR(gluNewTess);
    LOAD_FUNCPTR(gluNurbsCallback);
    LOAD_FUNCPTR(gluNurbsCurve);
    LOAD_FUNCPTR(gluNurbsProperty);
    LOAD_FUNCPTR(gluNurbsSurface);
    LOAD_FUNCPTR(gluOrtho2D);
    LOAD_FUNCPTR(gluPartialDisk);
    LOAD_FUNCPTR(gluPerspective);
    LOAD_FUNCPTR(gluPickMatrix);
    LOAD_FUNCPTR(gluProject);
    LOAD_FUNCPTR(gluPwlCurve);
    LOAD_FUNCPTR(gluQuadricCallback);
    LOAD_FUNCPTR(gluQuadricDrawStyle);
    LOAD_FUNCPTR(gluQuadricNormals);
    LOAD_FUNCPTR(gluQuadricOrientation);
    LOAD_FUNCPTR(gluQuadricTexture);
    LOAD_FUNCPTR(gluScaleImage);
    LOAD_FUNCPTR(gluSphere);
    LOAD_FUNCPTR(gluTessBeginContour);
    LOAD_FUNCPTR(gluTessBeginPolygon);
    LOAD_FUNCPTR(gluTessCallback);
    LOAD_FUNCPTR(gluTessEndContour);
    LOAD_FUNCPTR(gluTessEndPolygon);
    LOAD_FUNCPTR(gluTessNormal);
    LOAD_FUNCPTR(gluTessProperty);
    LOAD_FUNCPTR(gluTessVertex);
    LOAD_FUNCPTR(gluUnProject);
#undef LOAD_FUNCPTR
    TRACE( "loaded %s\n", SONAME_LIBGLU );
    return TRUE;
}


/***********************************************************************
 *		DllMain
 */
BOOL WINAPI DllMain( HINSTANCE inst, DWORD reason, LPVOID reserved )
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( inst );
        return load_libglu();
    }
    return TRUE;
}

/***********************************************************************
 *		gluLookAt (GLU32.@)
 */
void WINAPI wine_gluLookAt( GLdouble eyex, GLdouble eyey, GLdouble eyez,
                            GLdouble centerx, GLdouble centery, GLdouble centerz,
                            GLdouble upx, GLdouble upy, GLdouble upz )
{
    p_gluLookAt( eyex, eyey, eyez, centerx, centery, centerz, upx, upy, upz );
}

/***********************************************************************
 *		gluOrtho2D (GLU32.@)
 */
void WINAPI wine_gluOrtho2D( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top )
{
    p_gluOrtho2D( left, right, bottom, top );
}

/***********************************************************************
 *		gluPerspective (GLU32.@)
 */
void WINAPI wine_gluPerspective( GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar )
{
    p_gluPerspective( fovy, aspect, zNear, zFar );
}

/***********************************************************************
 *		gluPickMatrix (GLU32.@)
 */
void WINAPI wine_gluPickMatrix( GLdouble x, GLdouble y, GLdouble width, GLdouble height, GLint viewport[4])
{
    p_gluPickMatrix( x, y, width, height, viewport );
}

/***********************************************************************
 *		gluProject (GLU32.@)
 */
int WINAPI wine_gluProject( GLdouble objx, GLdouble objy, GLdouble objz, const GLdouble modelMatrix[16],
                            const GLdouble projMatrix[16], const GLint viewport[4],
                            GLdouble *winx, GLdouble *winy, GLdouble *winz )
{
    return p_gluProject( objx, objy, objz, modelMatrix, projMatrix, viewport, winx, winy, winz );
}

/***********************************************************************
 *		gluUnProject (GLU32.@)
 */
int WINAPI wine_gluUnProject( GLdouble winx, GLdouble winy, GLdouble winz, const GLdouble modelMatrix[16],
                              const GLdouble projMatrix[16], const GLint viewport[4],
                              GLdouble *objx, GLdouble *objy, GLdouble *objz )
{
    return p_gluUnProject( winx, winy, winz, modelMatrix, projMatrix, viewport, objx, objy, objz );
}

/***********************************************************************
 *		gluErrorString (GLU32.@)
 */
const GLubyte * WINAPI wine_gluErrorString( GLenum errCode )
{
    return p_gluErrorString( errCode );
}

/***********************************************************************
 *		gluScaleImage (GLU32.@)
 */
int WINAPI wine_gluScaleImage( GLenum format, GLint widthin, GLint heightin, GLenum typein, const void *datain,
                               GLint widthout, GLint heightout, GLenum typeout, void *dataout )
{
    return p_gluScaleImage( format, widthin, heightin, typein, datain,
                            widthout, heightout, typeout, dataout );
}

/***********************************************************************
 *		gluBuild1DMipmaps (GLU32.@)
 */
int WINAPI wine_gluBuild1DMipmaps( GLenum target, GLint components, GLint width,
                                   GLenum format, GLenum type, const void *data )
{
    return p_gluBuild1DMipmaps( target, components, width, format, type, data );
}

/***********************************************************************
 *		gluBuild2DMipmaps (GLU32.@)
 */
int WINAPI wine_gluBuild2DMipmaps( GLenum target, GLint components, GLint width, GLint height,
                                   GLenum format, GLenum type, const void *data )
{
    return p_gluBuild2DMipmaps( target, components, width, height, format, type, data );
}

/***********************************************************************
 *		gluNewQuadric (GLU32.@)
 */
GLUquadric * WINAPI wine_gluNewQuadric(void)
{
    return p_gluNewQuadric();
}

/***********************************************************************
 *		gluDeleteQuadric (GLU32.@)
 */
void WINAPI wine_gluDeleteQuadric( GLUquadric *state )
{
    p_gluDeleteQuadric( state );
}

/***********************************************************************
 *		gluQuadricDrawStyle (GLU32.@)
 */
void WINAPI wine_gluQuadricDrawStyle( GLUquadric *quadObject, GLenum drawStyle )
{
    p_gluQuadricDrawStyle( quadObject, drawStyle );
}

/***********************************************************************
 *		gluQuadricOrientation (GLU32.@)
 */
void WINAPI wine_gluQuadricOrientation( GLUquadric *quadObject, GLenum orientation )
{
    p_gluQuadricOrientation( quadObject, orientation );
}

/***********************************************************************
 *		gluQuadricNormals (GLU32.@)
 */
void WINAPI wine_gluQuadricNormals( GLUquadric *quadObject, GLenum normals )
{
    p_gluQuadricNormals( quadObject, normals );
}

/***********************************************************************
 *		gluQuadricTexture (GLU32.@)
 */
void WINAPI wine_gluQuadricTexture( GLUquadric *quadObject, GLboolean textureCoords )
{
    p_gluQuadricTexture( quadObject, textureCoords );
}

/***********************************************************************
 *		gluQuadricCallback (GLU32.@)
 */
void WINAPI wine_gluQuadricCallback( GLUquadric *qobj, GLenum which, void (CALLBACK *fn)(void) )
{
    /* FIXME: callback calling convention */
    p_gluQuadricCallback( qobj, which, (_GLUfuncptr)fn );
}

/***********************************************************************
 *		gluCylinder (GLU32.@)
 */
void WINAPI wine_gluCylinder( GLUquadric *qobj, GLdouble baseRadius, GLdouble topRadius,
                              GLdouble height, GLint slices, GLint stacks )
{
    p_gluCylinder( qobj, baseRadius, topRadius, height, slices, stacks );
}

/***********************************************************************
 *		gluSphere (GLU32.@)
 */
void WINAPI wine_gluSphere( GLUquadric *qobj, GLdouble radius, GLint slices, GLint stacks )
{
    p_gluSphere( qobj, radius, slices, stacks );
}

/***********************************************************************
 *		gluDisk (GLU32.@)
 */
void WINAPI wine_gluDisk( GLUquadric *qobj, GLdouble innerRadius, GLdouble outerRadius,
                          GLint slices, GLint loops )
{
    p_gluDisk( qobj, innerRadius, outerRadius, slices, loops );
}

/***********************************************************************
 *		gluPartialDisk (GLU32.@)
 */
void WINAPI wine_gluPartialDisk( GLUquadric *qobj, GLdouble innerRadius, GLdouble outerRadius,
                                 GLint slices, GLint loops, GLdouble startAngle, GLdouble sweepAngle )
{
    p_gluPartialDisk( qobj, innerRadius, outerRadius, slices, loops, startAngle, sweepAngle );
}

/***********************************************************************
 *		gluNewNurbsRenderer (GLU32.@)
 */
GLUnurbs * WINAPI wine_gluNewNurbsRenderer(void)
{
    return p_gluNewNurbsRenderer();
}

/***********************************************************************
 *		gluDeleteNurbsRenderer (GLU32.@)
 */
void WINAPI wine_gluDeleteNurbsRenderer( GLUnurbs *nobj )
{
    p_gluDeleteNurbsRenderer( nobj );
}

/***********************************************************************
 *		gluLoadSamplingMatrices (GLU32.@)
 */
void WINAPI wine_gluLoadSamplingMatrices( GLUnurbs *nobj, const GLfloat modelMatrix[16],
                                          const GLfloat projMatrix[16], const GLint viewport[4] )
{
    p_gluLoadSamplingMatrices( nobj, modelMatrix, projMatrix, viewport );
}

/***********************************************************************
 *		gluNurbsProperty (GLU32.@)
 */
void WINAPI wine_gluNurbsProperty( GLUnurbs *nobj, GLenum property, GLfloat value )
{
    p_gluNurbsProperty( nobj, property, value );
}

/***********************************************************************
 *		gluGetNurbsProperty (GLU32.@)
 */
void WINAPI wine_gluGetNurbsProperty( GLUnurbs *nobj, GLenum property, GLfloat *value )
{
    p_gluGetNurbsProperty( nobj, property, value );
}

/***********************************************************************
 *		gluBeginCurve (GLU32.@)
 */
void WINAPI wine_gluBeginCurve( GLUnurbs *nobj )
{
    p_gluBeginCurve( nobj );
}

/***********************************************************************
 *		gluEndCurve (GLU32.@)
 */
void WINAPI wine_gluEndCurve( GLUnurbs *nobj )
{
    p_gluEndCurve( nobj );
}

/***********************************************************************
 *		gluNurbsCurve (GLU32.@)
 */
void WINAPI wine_gluNurbsCurve( GLUnurbs *nobj, GLint nknots, GLfloat *knot,
                                GLint stride, GLfloat *ctlarray, GLint order, GLenum type )
{
    p_gluNurbsCurve( nobj, nknots, knot, stride, ctlarray, order, type );
}

/***********************************************************************
 *		gluBeginSurface (GLU32.@)
 */
void WINAPI wine_gluBeginSurface( GLUnurbs *nobj )
{
    p_gluBeginSurface( nobj );
}

/***********************************************************************
 *		gluEndSurface (GLU32.@)
 */
void WINAPI wine_gluEndSurface( GLUnurbs *nobj )
{
    p_gluEndSurface( nobj );
}

/***********************************************************************
 *		gluNurbsSurface (GLU32.@)
 */
void WINAPI wine_gluNurbsSurface( GLUnurbs *nobj, GLint sknot_count, float *sknot, GLint tknot_count,
                                  GLfloat *tknot, GLint s_stride, GLint t_stride, GLfloat *ctlarray,
                                  GLint sorder, GLint torder, GLenum type )
{
    p_gluNurbsSurface( nobj, sknot_count, sknot, tknot_count, tknot,
                       s_stride, t_stride, ctlarray, sorder, torder, type );
}

/***********************************************************************
 *		gluBeginTrim (GLU32.@)
 */
void WINAPI wine_gluBeginTrim( GLUnurbs *nobj )
{
    p_gluBeginTrim( nobj );
}

/***********************************************************************
 *		gluEndTrim (GLU32.@)
 */
void WINAPI wine_gluEndTrim( GLUnurbs *nobj )
{
    p_gluEndTrim( nobj );
}

/***********************************************************************
 *		gluPwlCurve (GLU32.@)
 */
void WINAPI wine_gluPwlCurve( GLUnurbs *nobj, GLint count, GLfloat *array, GLint stride, GLenum type )
{
    p_gluPwlCurve( nobj, count, array, stride, type );
}

/***********************************************************************
 *		gluNurbsCallback (GLU32.@)
 */
void WINAPI wine_gluNurbsCallback( GLUnurbs *nobj, GLenum which, void (CALLBACK *fn)(void) )
{
    /* FIXME: callback calling convention */
    p_gluNurbsCallback( nobj, which, (_GLUfuncptr)fn );
}

/***********************************************************************
 *		gluGetString (GLU32.@)
 */
const GLubyte * WINAPI wine_gluGetString( GLenum name )
{
    return p_gluGetString( name );
}

/***********************************************************************
 *		gluCheckExtension (GLU32.@)
 */
GLboolean WINAPI wine_gluCheckExtension( const GLubyte *extName, const GLubyte *extString )
{
    return 0;
}

/***********************************************************************
 *		gluNewTess (GLU32.@)
 */
wine_GLUtesselator * WINAPI wine_gluNewTess(void)
{
    void *tess;
    wine_GLUtesselator *ret;

    if((tess = p_gluNewTess()) == NULL)
       return NULL;

    ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ret));
    if(!ret) {
        p_gluDeleteTess(tess);
        return NULL;
    }
    ret->tess = tess;
    return ret;
}

/***********************************************************************
 *		gluDeleteTess (GLU32.@)
 */
void WINAPI wine_gluDeleteTess( wine_GLUtesselator *wine_tess )
{
    p_gluDeleteTess(wine_tess->tess);
    HeapFree(GetProcessHeap(), 0, wine_tess);
    return;
}

/***********************************************************************
 *		gluTessBeginPolygon (GLU32.@)
 */
void WINAPI wine_gluTessBeginPolygon( wine_GLUtesselator *wine_tess, void *polygon_data )
{
    wine_tess->polygon_data = polygon_data;

    p_gluTessBeginPolygon(wine_tess->tess, wine_tess);
}

/***********************************************************************
 *		gluTessEndPolygon (GLU32.@)
 */
void WINAPI wine_gluTessEndPolygon( wine_GLUtesselator *wine_tess )
{
    p_gluTessEndPolygon(wine_tess->tess);
}


static void wine_glu_tess_begin_data(int type, wine_GLUtesselator *wine_tess)
{
    if(wine_tess->cb_tess_begin_data)
        wine_tess->cb_tess_begin_data(type, wine_tess->polygon_data);
    else
        wine_tess->cb_tess_begin(type);
}

static void wine_glu_tess_vertex_data(void *vertex_data, wine_GLUtesselator *wine_tess)
{
    if(wine_tess->cb_tess_vertex_data)
        wine_tess->cb_tess_vertex_data(vertex_data, wine_tess->polygon_data);
    else
        wine_tess->cb_tess_vertex(vertex_data);
}

static void wine_glu_tess_end_data(wine_GLUtesselator *wine_tess)
{
    if(wine_tess->cb_tess_end_data)
        wine_tess->cb_tess_end_data(wine_tess->polygon_data);
    else
        wine_tess->cb_tess_end();
}

static void wine_glu_tess_error_data(int error, wine_GLUtesselator *wine_tess)
{
    if(wine_tess->cb_tess_error_data)
        wine_tess->cb_tess_error_data(error, wine_tess->polygon_data);
    else
        wine_tess->cb_tess_error(error);
}

static void wine_glu_tess_edge_flag_data(int flag, wine_GLUtesselator *wine_tess)
{
    if(wine_tess->cb_tess_edge_flag_data)
        wine_tess->cb_tess_edge_flag_data(flag, wine_tess->polygon_data);
    else
        wine_tess->cb_tess_edge_flag(flag);
}

static void wine_glu_tess_combine_data(double *coords, void *vertex_data, float *weight, void **outData,
                                       wine_GLUtesselator *wine_tess)
{
    if(wine_tess->cb_tess_combine_data)
        wine_tess->cb_tess_combine_data(coords, vertex_data, weight, outData, wine_tess->polygon_data);
    else
        wine_tess->cb_tess_combine(coords, vertex_data, weight, outData);
}


/***********************************************************************
 *		gluTessCallback (GLU32.@)
 */
void WINAPI wine_gluTessCallback( wine_GLUtesselator *tess, GLenum which, void (CALLBACK *fn)(void))
{
    void *new_fn;

    switch(which) {
    case GLU_TESS_BEGIN:
        tess->cb_tess_begin = (void *)fn;
        new_fn = wine_glu_tess_begin_data;
        which += 6;
        break;
    case GLU_TESS_VERTEX:
        tess->cb_tess_vertex = (void *)fn;
        new_fn = wine_glu_tess_vertex_data;
        which += 6;
        break;
    case GLU_TESS_END:
        tess->cb_tess_end = (void *)fn;
        new_fn = wine_glu_tess_end_data;
        which += 6;
        break;
    case GLU_TESS_ERROR:
        tess->cb_tess_error = (void *)fn;
        new_fn = wine_glu_tess_error_data;
        which += 6;
        break;
    case GLU_TESS_EDGE_FLAG:
        tess->cb_tess_edge_flag = (void *)fn;
        new_fn = wine_glu_tess_edge_flag_data;
        which += 6;
        break;
    case GLU_TESS_COMBINE:
        tess->cb_tess_combine = (void *)fn;
        new_fn = wine_glu_tess_combine_data;
        which += 6;
        break;
    case GLU_TESS_BEGIN_DATA:
        tess->cb_tess_begin_data = (void *)fn;
        new_fn = wine_glu_tess_begin_data;
        break;
    case GLU_TESS_VERTEX_DATA:
        tess->cb_tess_vertex_data = (void *)fn;
        new_fn = wine_glu_tess_vertex_data;
        break;
    case GLU_TESS_END_DATA:
        tess->cb_tess_end_data = (void *)fn;
        new_fn = wine_glu_tess_end_data;
        break;
    case GLU_TESS_ERROR_DATA:
        tess->cb_tess_error_data = (void *)fn;
        new_fn = wine_glu_tess_error_data;
        break;
    case GLU_TESS_EDGE_FLAG_DATA:
        tess->cb_tess_edge_flag_data = (void *)fn;
        new_fn = wine_glu_tess_edge_flag_data;
        break;
    case GLU_TESS_COMBINE_DATA:
        tess->cb_tess_combine_data = (void *)fn;
        new_fn = wine_glu_tess_combine_data;
        break;
    default:
        ERR("Unknown callback %d\n", which);
        return;
    }
    p_gluTessCallback(tess->tess, which, new_fn);
}

/***********************************************************************
 *		gluTessBeginContour (GLU32.@)
 */
void WINAPI wine_gluTessBeginContour( wine_GLUtesselator *tess )
{
    p_gluTessBeginContour(tess->tess);
}

/***********************************************************************
 *		gluTessEndContour (GLU32.@)
 */
void WINAPI wine_gluTessEndContour( wine_GLUtesselator *tess )
{
    p_gluTessEndContour(tess->tess);
}

/***********************************************************************
 *		gluTessVertex (GLU32.@)
 */
void WINAPI wine_gluTessVertex( wine_GLUtesselator *tess, GLdouble coords[3], void *data )
{
    p_gluTessVertex( tess->tess, coords, data );
}


/***********************************************************************
 *		gluTessProperty (GLU32.@)
 */
void WINAPI wine_gluTessProperty( wine_GLUtesselator *tess, GLenum which, GLdouble value )
{
    p_gluTessProperty( tess->tess, which, value );
}

/***********************************************************************
 *		gluTessNormal (GLU32.@)
 */
void WINAPI wine_gluTessNormal( wine_GLUtesselator *tess, GLdouble x, GLdouble y, GLdouble z )
{
    p_gluTessNormal( tess->tess, x, y, z );
}

/***********************************************************************
 *      gluBeginPolygon (GLU32.@)
 */
void WINAPI wine_gluBeginPolygon( wine_GLUtesselator *tess )
{
    tess->polygon_data = NULL;
    p_gluTessBeginPolygon(tess->tess, tess);
    p_gluTessBeginContour(tess->tess);
}

/***********************************************************************
 *      gluEndPolygon (GLU32.@)
 */
void WINAPI wine_gluEndPolygon( wine_GLUtesselator *tess )
{
    p_gluTessEndContour(tess->tess);
    p_gluTessEndPolygon(tess->tess);
}

/***********************************************************************
 *      gluNextContour (GLU32.@)
 */
void WINAPI wine_gluNextContour( wine_GLUtesselator *tess, GLenum type )
{
    p_gluTessEndContour(tess->tess);
    p_gluTessBeginContour(tess->tess);
}
