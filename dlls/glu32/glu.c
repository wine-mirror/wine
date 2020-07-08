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

typedef void (*_GLUfuncptr)(void);

static void  (*p_gluBeginCurve)( GLUnurbs* nurb );
static void  (*p_gluBeginSurface)( GLUnurbs* nurb );
static void  (*p_gluBeginTrim)( GLUnurbs* nurb );
static void  (*p_gluDeleteNurbsRenderer)( GLUnurbs* nurb );
static void  (*p_gluEndCurve)( GLUnurbs* nurb );
static void  (*p_gluEndSurface)( GLUnurbs* nurb );
static void  (*p_gluEndTrim)( GLUnurbs* nurb );
static void  (*p_gluGetNurbsProperty)( GLUnurbs* nurb, GLenum property, GLfloat* data );
static void  (*p_gluLoadSamplingMatrices)( GLUnurbs* nurb, const GLfloat *model, const GLfloat *perspective, const GLint *view );
static GLUnurbs* (*p_gluNewNurbsRenderer)(void);
static void  (*p_gluNurbsCallback)( GLUnurbs* nurb, GLenum which, _GLUfuncptr CallBackFunc );
static void  (*p_gluNurbsCurve)( GLUnurbs* nurb, GLint knotCount, GLfloat *knots, GLint stride, GLfloat *control, GLint order, GLenum type );
static void  (*p_gluNurbsProperty)( GLUnurbs* nurb, GLenum property, GLfloat value );
static void  (*p_gluNurbsSurface)( GLUnurbs* nurb, GLint sKnotCount, GLfloat* sKnots, GLint tKnotCount, GLfloat* tKnots, GLint sStride, GLint tStride, GLfloat* control, GLint sOrder, GLint tOrder, GLenum type );
static void  (*p_gluPwlCurve)( GLUnurbs* nurb, GLint count, GLfloat* data, GLint stride, GLenum type );

static void *libglu_handle;
static INIT_ONCE init_once = INIT_ONCE_STATIC_INIT;

static BOOL WINAPI load_libglu( INIT_ONCE *once, void *param, void **context )
{
#ifdef SONAME_LIBGLU
    if ((libglu_handle = dlopen( SONAME_LIBGLU, RTLD_NOW )))
        TRACE( "loaded %s\n", SONAME_LIBGLU );
    else
        ERR( "Failed to load %s: %s\n", SONAME_LIBGLU, dlerror() );
#else
    ERR( "libGLU is needed but support was not included at build time\n" );
#endif
    return libglu_handle != NULL;
}

static void *load_glufunc( const char *name )
{
    void *ret;

    if (!InitOnceExecuteOnce( &init_once, load_libglu, NULL, NULL )) return NULL;
    if (!(ret = dlsym( libglu_handle, name ))) ERR( "Can't find %s\n", name );
    return ret;
}

#define LOAD_FUNCPTR(f) (p_##f || (p_##f = load_glufunc( #f )))


/***********************************************************************
 *		gluErrorString (GLU32.@)
 */
const GLubyte * WINAPI wine_gluErrorString( GLenum errCode )
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(errors); i++)
        if (errors[i].err == errCode) return (const GLubyte *)errors[i].str;

    return NULL;
}

/***********************************************************************
 *		gluErrorUnicodeStringEXT (GLU32.@)
 */
const WCHAR * WINAPI wine_gluErrorUnicodeStringEXT( GLenum errCode )
{
    static WCHAR errorsW[ARRAY_SIZE(errors)][64];
    unsigned int i, j;

    for (i = 0; i < ARRAY_SIZE(errors); i++)
    {
        if (errors[i].err != errCode) continue;
        if (!errorsW[i][0])  /* errors use only ASCII, do a simple mapping */
            for (j = 0; errors[i].str[j]; j++) errorsW[i][j] = (WCHAR)errors[i].str[j];
        return errorsW[i];
    }
    return NULL;
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
