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
#include "wine/glu.h"
#include "wine/library.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(glu);

static const struct { GLuint err; const char *str; } errors[] =
{
    { GL_NO_ERROR, "no error" },
    { GL_INVALID_ENUM, "invalid enumerant" },
    { GL_INVALID_VALUE, "invalid value" },
    { GL_INVALID_OPERATION, "invalid operation" },
    { GL_STACK_OVERFLOW, "stack overflow" },
    { GL_STACK_UNDERFLOW, "stack underflow" },
    { GL_OUT_OF_MEMORY, "out of memory" },
    { GL_TABLE_TOO_LARGE, "table too large" },
    { GL_INVALID_FRAMEBUFFER_OPERATION_EXT, "invalid framebuffer operation" },
    { GLU_INVALID_ENUM, "invalid enumerant" },
    { GLU_INVALID_VALUE, "invalid value" },
    { GLU_OUT_OF_MEMORY, "out of memory" },
    { GLU_INCOMPATIBLE_GL_VERSION, "incompatible gl version" },
    { GLU_TESS_ERROR1, "gluTessBeginPolygon() must precede a gluTessEndPolygon()" },
    { GLU_TESS_ERROR2, "gluTessBeginContour() must precede a gluTessEndContour()" },
    { GLU_TESS_ERROR3, "gluTessEndPolygon() must follow a gluTessBeginPolygon()" },
    { GLU_TESS_ERROR4, "gluTessEndContour() must follow a gluTessBeginContour()" },
    { GLU_TESS_ERROR5, "a coordinate is too large" },
    { GLU_TESS_ERROR6, "need combine callback" },
    { GLU_NURBS_ERROR1, "spline order un-supported" },
    { GLU_NURBS_ERROR2, "too few knots" },
    { GLU_NURBS_ERROR3, "valid knot range is empty" },
    { GLU_NURBS_ERROR4, "decreasing knot sequence knot" },
    { GLU_NURBS_ERROR5, "knot multiplicity greater than order of spline" },
    { GLU_NURBS_ERROR6, "gluEndCurve() must follow gluBeginCurve()" },
    { GLU_NURBS_ERROR7, "gluBeginCurve() must precede gluEndCurve()" },
    { GLU_NURBS_ERROR8, "missing or extra geometric data" },
    { GLU_NURBS_ERROR9, "can't draw piecewise linear trimming curves" },
    { GLU_NURBS_ERROR10, "missing or extra domain data" },
    { GLU_NURBS_ERROR11, "missing or extra domain data" },
    { GLU_NURBS_ERROR12, "gluEndTrim() must precede gluEndSurface()" },
    { GLU_NURBS_ERROR13, "gluBeginSurface() must precede gluEndSurface()" },
    { GLU_NURBS_ERROR14, "curve of improper type passed as trim curve" },
    { GLU_NURBS_ERROR15, "gluBeginSurface() must precede gluBeginTrim()" },
    { GLU_NURBS_ERROR16, "gluEndTrim() must follow gluBeginTrim()" },
    { GLU_NURBS_ERROR17, "gluBeginTrim() must precede gluEndTrim()" },
    { GLU_NURBS_ERROR18, "invalid or missing trim curve" },
    { GLU_NURBS_ERROR19, "gluBeginTrim() must precede gluPwlCurve()" },
    { GLU_NURBS_ERROR20, "piecewise linear trimming curve referenced twice" },
    { GLU_NURBS_ERROR21, "piecewise linear trimming curve and nurbs curve mixed" },
    { GLU_NURBS_ERROR22, "improper usage of trim data type" },
    { GLU_NURBS_ERROR23, "nurbs curve referenced twice" },
    { GLU_NURBS_ERROR24, "nurbs curve and piecewise linear trimming curve mixed" },
    { GLU_NURBS_ERROR25, "nurbs surface referenced twice" },
    { GLU_NURBS_ERROR26, "invalid property" },
    { GLU_NURBS_ERROR27, "gluEndSurface() must follow gluBeginSurface()" },
    { GLU_NURBS_ERROR28, "intersecting or misoriented trim curves" },
    { GLU_NURBS_ERROR29, "intersecting trim curves" },
    { GLU_NURBS_ERROR30, "UNUSED" },
    { GLU_NURBS_ERROR31, "unconnected trim curves" },
    { GLU_NURBS_ERROR32, "unknown knot error" },
    { GLU_NURBS_ERROR33, "negative vertex count encountered" },
    { GLU_NURBS_ERROR34, "negative byte-stride encountered" },
    { GLU_NURBS_ERROR35, "unknown type descriptor" },
    { GLU_NURBS_ERROR36, "null control point reference" },
    { GLU_NURBS_ERROR37, "duplicate point on piecewise linear trimming curve" },
};
#define NB_ERRORS (sizeof(errors) / sizeof(errors[0]))

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
static void  (*p_gluGetNurbsProperty)( GLUnurbs* nurb, GLenum property, GLfloat* data );
static void  (*p_gluGetTessProperty)( GLUtesselator* tess, GLenum which, GLdouble* data );
static void  (*p_gluLoadSamplingMatrices)( GLUnurbs* nurb, const GLfloat *model, const GLfloat *perspective, const GLint *view );
static GLUnurbs* (*p_gluNewNurbsRenderer)(void);
static GLUquadric* (*p_gluNewQuadric)(void);
static GLUtesselator* (*p_gluNewTess)(void);
static void  (*p_gluNurbsCallback)( GLUnurbs* nurb, GLenum which, _GLUfuncptr CallBackFunc );
static void  (*p_gluNurbsCurve)( GLUnurbs* nurb, GLint knotCount, GLfloat *knots, GLint stride, GLfloat *control, GLint order, GLenum type );
static void  (*p_gluNurbsProperty)( GLUnurbs* nurb, GLenum property, GLfloat value );
static void  (*p_gluNurbsSurface)( GLUnurbs* nurb, GLint sKnotCount, GLfloat* sKnots, GLint tKnotCount, GLfloat* tKnots, GLint sStride, GLint tStride, GLfloat* control, GLint sOrder, GLint tOrder, GLenum type );
static void  (*p_gluPartialDisk)( GLUquadric* quad, GLdouble inner, GLdouble outer, GLint slices, GLint loops, GLdouble start, GLdouble sweep );
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

static void *libglu_handle;
static INIT_ONCE init_once = INIT_ONCE_STATIC_INIT;

static BOOL WINAPI load_libglu( INIT_ONCE *once, void *param, void **context )
{
#ifdef SONAME_LIBGLU
    char error[256];

    if ((libglu_handle = wine_dlopen( SONAME_LIBGLU, RTLD_NOW, error, sizeof(error) )))
        TRACE( "loaded %s\n", SONAME_LIBGLU );
    else
        ERR( "Failed to load %s: %s\n", SONAME_LIBGLU, error );
#else
    ERR( "libGLU is needed but support was not included at build time\n" );
#endif
    return libglu_handle != NULL;
}

static void *load_glufunc( const char *name )
{
    void *ret;

    if (!InitOnceExecuteOnce( &init_once, load_libglu, NULL, NULL )) return NULL;
    if (!(ret = wine_dlsym( libglu_handle, name, NULL, 0 ))) ERR( "Can't find %s\n", name );
    return ret;
}

#define LOAD_FUNCPTR(f) (p_##f || (p_##f = load_glufunc( #f )))


/***********************************************************************
 *		gluErrorString (GLU32.@)
 */
const GLubyte * WINAPI wine_gluErrorString( GLenum errCode )
{
    unsigned int i;

    for (i = 0; i < NB_ERRORS; i++)
        if (errors[i].err == errCode) return (const GLubyte *)errors[i].str;

    return NULL;
}

/***********************************************************************
 *		gluErrorUnicodeStringEXT (GLU32.@)
 */
const WCHAR * WINAPI wine_gluErrorUnicodeStringEXT( GLenum errCode )
{
    static WCHAR errorsW[NB_ERRORS][64];
    unsigned int i, j;

    for (i = 0; i < NB_ERRORS; i++)
    {
        if (errors[i].err != errCode) continue;
        if (!errorsW[i][0])  /* errors use only ASCII, do a simple mapping */
            for (j = 0; errors[i].str[j]; j++) errorsW[i][j] = (WCHAR)errors[i].str[j];
        return errorsW[i];
    }
    return NULL;
}

/***********************************************************************
 *		gluScaleImage (GLU32.@)
 */
int WINAPI wine_gluScaleImage( GLenum format, GLint widthin, GLint heightin, GLenum typein, const void *datain,
                               GLint widthout, GLint heightout, GLenum typeout, void *dataout )
{
    if (!LOAD_FUNCPTR( gluScaleImage )) return GLU_OUT_OF_MEMORY;
    return p_gluScaleImage( format, widthin, heightin, typein, datain,
                            widthout, heightout, typeout, dataout );
}

/***********************************************************************
 *		gluBuild1DMipmaps (GLU32.@)
 */
int WINAPI wine_gluBuild1DMipmaps( GLenum target, GLint components, GLint width,
                                   GLenum format, GLenum type, const void *data )
{
    if (!LOAD_FUNCPTR( gluBuild1DMipmaps )) return GLU_OUT_OF_MEMORY;
    return p_gluBuild1DMipmaps( target, components, width, format, type, data );
}

/***********************************************************************
 *		gluBuild2DMipmaps (GLU32.@)
 */
int WINAPI wine_gluBuild2DMipmaps( GLenum target, GLint components, GLint width, GLint height,
                                   GLenum format, GLenum type, const void *data )
{
    if (!LOAD_FUNCPTR( gluBuild2DMipmaps )) return GLU_OUT_OF_MEMORY;
    return p_gluBuild2DMipmaps( target, components, width, height, format, type, data );
}

/***********************************************************************
 *		gluNewQuadric (GLU32.@)
 */
GLUquadric * WINAPI wine_gluNewQuadric(void)
{
    if (!LOAD_FUNCPTR( gluNewQuadric )) return NULL;
    return p_gluNewQuadric();
}

/***********************************************************************
 *		gluDeleteQuadric (GLU32.@)
 */
void WINAPI wine_gluDeleteQuadric( GLUquadric *state )
{
    if (!LOAD_FUNCPTR( gluDeleteQuadric )) return;
    p_gluDeleteQuadric( state );
}

/***********************************************************************
 *		gluQuadricDrawStyle (GLU32.@)
 */
void WINAPI wine_gluQuadricDrawStyle( GLUquadric *quadObject, GLenum drawStyle )
{
    if (!LOAD_FUNCPTR( gluQuadricDrawStyle )) return;
    p_gluQuadricDrawStyle( quadObject, drawStyle );
}

/***********************************************************************
 *		gluQuadricOrientation (GLU32.@)
 */
void WINAPI wine_gluQuadricOrientation( GLUquadric *quadObject, GLenum orientation )
{
    if (!LOAD_FUNCPTR( gluQuadricOrientation )) return;
    p_gluQuadricOrientation( quadObject, orientation );
}

/***********************************************************************
 *		gluQuadricNormals (GLU32.@)
 */
void WINAPI wine_gluQuadricNormals( GLUquadric *quadObject, GLenum normals )
{
    if (!LOAD_FUNCPTR( gluQuadricNormals )) return;
    p_gluQuadricNormals( quadObject, normals );
}

/***********************************************************************
 *		gluQuadricTexture (GLU32.@)
 */
void WINAPI wine_gluQuadricTexture( GLUquadric *quadObject, GLboolean textureCoords )
{
    if (!LOAD_FUNCPTR( gluQuadricTexture )) return;
    p_gluQuadricTexture( quadObject, textureCoords );
}

/***********************************************************************
 *		gluQuadricCallback (GLU32.@)
 */
void WINAPI wine_gluQuadricCallback( GLUquadric *qobj, GLenum which, void (CALLBACK *fn)(void) )
{
    if (!LOAD_FUNCPTR( gluQuadricCallback )) return;
    /* FIXME: callback calling convention */
    p_gluQuadricCallback( qobj, which, (_GLUfuncptr)fn );
}

/***********************************************************************
 *		gluCylinder (GLU32.@)
 */
void WINAPI wine_gluCylinder( GLUquadric *qobj, GLdouble baseRadius, GLdouble topRadius,
                              GLdouble height, GLint slices, GLint stacks )
{
    if (!LOAD_FUNCPTR( gluCylinder )) return;
    p_gluCylinder( qobj, baseRadius, topRadius, height, slices, stacks );
}

/***********************************************************************
 *		gluSphere (GLU32.@)
 */
void WINAPI wine_gluSphere( GLUquadric *qobj, GLdouble radius, GLint slices, GLint stacks )
{
    if (!LOAD_FUNCPTR( gluSphere )) return;
    p_gluSphere( qobj, radius, slices, stacks );
}

/***********************************************************************
 *		gluDisk (GLU32.@)
 */
void WINAPI wine_gluDisk( GLUquadric *qobj, GLdouble innerRadius, GLdouble outerRadius,
                          GLint slices, GLint loops )
{
    if (!LOAD_FUNCPTR( gluDisk )) return;
    p_gluDisk( qobj, innerRadius, outerRadius, slices, loops );
}

/***********************************************************************
 *		gluPartialDisk (GLU32.@)
 */
void WINAPI wine_gluPartialDisk( GLUquadric *qobj, GLdouble innerRadius, GLdouble outerRadius,
                                 GLint slices, GLint loops, GLdouble startAngle, GLdouble sweepAngle )
{
    if (!LOAD_FUNCPTR( gluPartialDisk )) return;
    p_gluPartialDisk( qobj, innerRadius, outerRadius, slices, loops, startAngle, sweepAngle );
}

/***********************************************************************
 *		gluNewNurbsRenderer (GLU32.@)
 */
GLUnurbs * WINAPI wine_gluNewNurbsRenderer(void)
{
    if (!LOAD_FUNCPTR( gluNewNurbsRenderer )) return NULL;
    return p_gluNewNurbsRenderer();
}

/***********************************************************************
 *		gluDeleteNurbsRenderer (GLU32.@)
 */
void WINAPI wine_gluDeleteNurbsRenderer( GLUnurbs *nobj )
{
    if (!LOAD_FUNCPTR( gluDeleteNurbsRenderer )) return;
    p_gluDeleteNurbsRenderer( nobj );
}

/***********************************************************************
 *		gluLoadSamplingMatrices (GLU32.@)
 */
void WINAPI wine_gluLoadSamplingMatrices( GLUnurbs *nobj, const GLfloat modelMatrix[16],
                                          const GLfloat projMatrix[16], const GLint viewport[4] )
{
    if (!LOAD_FUNCPTR( gluLoadSamplingMatrices )) return;
    p_gluLoadSamplingMatrices( nobj, modelMatrix, projMatrix, viewport );
}

/***********************************************************************
 *		gluNurbsProperty (GLU32.@)
 */
void WINAPI wine_gluNurbsProperty( GLUnurbs *nobj, GLenum property, GLfloat value )
{
    if (!LOAD_FUNCPTR( gluNurbsProperty )) return;
    p_gluNurbsProperty( nobj, property, value );
}

/***********************************************************************
 *		gluGetNurbsProperty (GLU32.@)
 */
void WINAPI wine_gluGetNurbsProperty( GLUnurbs *nobj, GLenum property, GLfloat *value )
{
    if (!LOAD_FUNCPTR( gluGetNurbsProperty )) return;
    p_gluGetNurbsProperty( nobj, property, value );
}

/***********************************************************************
 *		gluBeginCurve (GLU32.@)
 */
void WINAPI wine_gluBeginCurve( GLUnurbs *nobj )
{
    if (!LOAD_FUNCPTR( gluBeginCurve )) return;
    p_gluBeginCurve( nobj );
}

/***********************************************************************
 *		gluEndCurve (GLU32.@)
 */
void WINAPI wine_gluEndCurve( GLUnurbs *nobj )
{
    if (!LOAD_FUNCPTR( gluEndCurve )) return;
    p_gluEndCurve( nobj );
}

/***********************************************************************
 *		gluNurbsCurve (GLU32.@)
 */
void WINAPI wine_gluNurbsCurve( GLUnurbs *nobj, GLint nknots, GLfloat *knot,
                                GLint stride, GLfloat *ctlarray, GLint order, GLenum type )
{
    if (!LOAD_FUNCPTR( gluNurbsCurve )) return;
    p_gluNurbsCurve( nobj, nknots, knot, stride, ctlarray, order, type );
}

/***********************************************************************
 *		gluBeginSurface (GLU32.@)
 */
void WINAPI wine_gluBeginSurface( GLUnurbs *nobj )
{
    if (!LOAD_FUNCPTR( gluBeginSurface )) return;
    p_gluBeginSurface( nobj );
}

/***********************************************************************
 *		gluEndSurface (GLU32.@)
 */
void WINAPI wine_gluEndSurface( GLUnurbs *nobj )
{
    if (!LOAD_FUNCPTR( gluEndSurface )) return;
    p_gluEndSurface( nobj );
}

/***********************************************************************
 *		gluNurbsSurface (GLU32.@)
 */
void WINAPI wine_gluNurbsSurface( GLUnurbs *nobj, GLint sknot_count, float *sknot, GLint tknot_count,
                                  GLfloat *tknot, GLint s_stride, GLint t_stride, GLfloat *ctlarray,
                                  GLint sorder, GLint torder, GLenum type )
{
    if (!LOAD_FUNCPTR( gluNurbsSurface )) return;
    p_gluNurbsSurface( nobj, sknot_count, sknot, tknot_count, tknot,
                       s_stride, t_stride, ctlarray, sorder, torder, type );
}

/***********************************************************************
 *		gluBeginTrim (GLU32.@)
 */
void WINAPI wine_gluBeginTrim( GLUnurbs *nobj )
{
    if (!LOAD_FUNCPTR( gluBeginTrim )) return;
    p_gluBeginTrim( nobj );
}

/***********************************************************************
 *		gluEndTrim (GLU32.@)
 */
void WINAPI wine_gluEndTrim( GLUnurbs *nobj )
{
    if (!LOAD_FUNCPTR( gluEndTrim )) return;
    p_gluEndTrim( nobj );
}

/***********************************************************************
 *		gluPwlCurve (GLU32.@)
 */
void WINAPI wine_gluPwlCurve( GLUnurbs *nobj, GLint count, GLfloat *array, GLint stride, GLenum type )
{
    if (!LOAD_FUNCPTR( gluPwlCurve )) return;
    p_gluPwlCurve( nobj, count, array, stride, type );
}

/***********************************************************************
 *		gluNurbsCallback (GLU32.@)
 */
void WINAPI wine_gluNurbsCallback( GLUnurbs *nobj, GLenum which, void (CALLBACK *fn)(void) )
{
    if (!LOAD_FUNCPTR( gluNurbsCallback )) return;
    /* FIXME: callback calling convention */
    p_gluNurbsCallback( nobj, which, (_GLUfuncptr)fn );
}

/***********************************************************************
 *		gluGetString (GLU32.@)
 */
const GLubyte * WINAPI wine_gluGetString( GLenum name )
{
    switch (name)
    {
    case GLU_VERSION: return (const GLubyte *)"1.2.2.0 Microsoft Corporation"; /* sic */
    case GLU_EXTENSIONS: return (const GLubyte *)"";
    }
    return NULL;
}

/***********************************************************************
 *		gluCheckExtension (GLU32.@)
 */
GLboolean WINAPI wine_gluCheckExtension( const GLubyte *extName, const GLubyte *extString )
{
    const char *list = (const char *)extString;
    const char *ext = (const char *)extName;
    size_t len = strlen( ext );

    while (list)
    {
        while (*list == ' ') list++;
        if (!strncmp( list, ext, len ) && (!list[len] || list[len] == ' ')) return GLU_TRUE;
        list = strchr( list, ' ' );
    }
    return GLU_FALSE;
}

/***********************************************************************
 *		gluNewTess (GLU32.@)
 */
wine_GLUtesselator * WINAPI wine_gluNewTess(void)
{
    void *tess;
    wine_GLUtesselator *ret;

    if (!LOAD_FUNCPTR( gluNewTess ) || !LOAD_FUNCPTR( gluDeleteTess )) return NULL;

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
    if (!LOAD_FUNCPTR( gluDeleteTess )) return;
    p_gluDeleteTess(wine_tess->tess);
    HeapFree(GetProcessHeap(), 0, wine_tess);
}

/***********************************************************************
 *		gluTessBeginPolygon (GLU32.@)
 */
void WINAPI wine_gluTessBeginPolygon( wine_GLUtesselator *wine_tess, void *polygon_data )
{
    if (!LOAD_FUNCPTR( gluTessBeginPolygon )) return;
    wine_tess->polygon_data = polygon_data;
    p_gluTessBeginPolygon(wine_tess->tess, wine_tess);
}

/***********************************************************************
 *		gluTessEndPolygon (GLU32.@)
 */
void WINAPI wine_gluTessEndPolygon( wine_GLUtesselator *wine_tess )
{
    if (!LOAD_FUNCPTR( gluTessEndPolygon )) return;
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

    if (!LOAD_FUNCPTR( gluTessCallback )) return;
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
    if (!LOAD_FUNCPTR( gluTessBeginContour )) return;
    p_gluTessBeginContour(tess->tess);
}

/***********************************************************************
 *		gluTessEndContour (GLU32.@)
 */
void WINAPI wine_gluTessEndContour( wine_GLUtesselator *tess )
{
    if (!LOAD_FUNCPTR( gluTessEndContour )) return;
    p_gluTessEndContour(tess->tess);
}

/***********************************************************************
 *		gluTessVertex (GLU32.@)
 */
void WINAPI wine_gluTessVertex( wine_GLUtesselator *tess, GLdouble coords[3], void *data )
{
    if (!LOAD_FUNCPTR( gluTessVertex )) return;
    p_gluTessVertex( tess->tess, coords, data );
}

/***********************************************************************
 *		gluGetTessProperty (GLU32.@)
 */
void WINAPI wine_gluGetTessProperty( wine_GLUtesselator *tess, GLenum which, GLdouble *value )
{
    if (!LOAD_FUNCPTR( gluGetTessProperty )) return;
    p_gluGetTessProperty( tess->tess, which, value );
}

/***********************************************************************
 *		gluTessProperty (GLU32.@)
 */
void WINAPI wine_gluTessProperty( wine_GLUtesselator *tess, GLenum which, GLdouble value )
{
    if (!LOAD_FUNCPTR( gluTessProperty )) return;
    p_gluTessProperty( tess->tess, which, value );
}

/***********************************************************************
 *		gluTessNormal (GLU32.@)
 */
void WINAPI wine_gluTessNormal( wine_GLUtesselator *tess, GLdouble x, GLdouble y, GLdouble z )
{
    if (!LOAD_FUNCPTR( gluTessNormal )) return;
    p_gluTessNormal( tess->tess, x, y, z );
}

/***********************************************************************
 *      gluBeginPolygon (GLU32.@)
 */
void WINAPI wine_gluBeginPolygon( wine_GLUtesselator *tess )
{
    if (!LOAD_FUNCPTR( gluTessBeginPolygon ) || !LOAD_FUNCPTR( gluTessBeginContour )) return;
    tess->polygon_data = NULL;
    p_gluTessBeginPolygon(tess->tess, tess);
    p_gluTessBeginContour(tess->tess);
}

/***********************************************************************
 *      gluEndPolygon (GLU32.@)
 */
void WINAPI wine_gluEndPolygon( wine_GLUtesselator *tess )
{
    if (!LOAD_FUNCPTR( gluTessEndPolygon ) || !LOAD_FUNCPTR( gluTessEndContour )) return;
    p_gluTessEndContour(tess->tess);
    p_gluTessEndPolygon(tess->tess);
}

/***********************************************************************
 *      gluNextContour (GLU32.@)
 */
void WINAPI wine_gluNextContour( wine_GLUtesselator *tess, GLenum type )
{
    if (!LOAD_FUNCPTR( gluTessEndContour ) || !LOAD_FUNCPTR( gluTessBeginContour )) return;
    p_gluTessEndContour(tess->tess);
    p_gluTessBeginContour(tess->tess);
}
