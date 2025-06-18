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

#include <assert.h>
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
    ERR( "Nurbs renderer is not supported, please report\n" );
    return NULL;
}

/***********************************************************************
 *		gluDeleteNurbsRenderer (GLU32.@)
 */
void WINAPI wine_gluDeleteNurbsRenderer( GLUnurbs *nobj )
{
    assert( 0 );
}

/***********************************************************************
 *		gluLoadSamplingMatrices (GLU32.@)
 */
void WINAPI wine_gluLoadSamplingMatrices( GLUnurbs *nobj, const GLfloat modelMatrix[16],
                                          const GLfloat projMatrix[16], const GLint viewport[4] )
{
    assert( 0 );
}

/***********************************************************************
 *		gluNurbsProperty (GLU32.@)
 */
void WINAPI wine_gluNurbsProperty( GLUnurbs *nobj, GLenum property, GLfloat value )
{
    assert( 0 );
}

/***********************************************************************
 *		gluGetNurbsProperty (GLU32.@)
 */
void WINAPI wine_gluGetNurbsProperty( GLUnurbs *nobj, GLenum property, GLfloat *value )
{
    assert( 0 );
}

/***********************************************************************
 *		gluBeginCurve (GLU32.@)
 */
void WINAPI wine_gluBeginCurve( GLUnurbs *nobj )
{
    assert( 0 );
}

/***********************************************************************
 *		gluEndCurve (GLU32.@)
 */
void WINAPI wine_gluEndCurve( GLUnurbs *nobj )
{
    assert( 0 );
}

/***********************************************************************
 *		gluNurbsCurve (GLU32.@)
 */
void WINAPI wine_gluNurbsCurve( GLUnurbs *nobj, GLint nknots, GLfloat *knot,
                                GLint stride, GLfloat *ctlarray, GLint order, GLenum type )
{
    assert( 0 );
}

/***********************************************************************
 *		gluBeginSurface (GLU32.@)
 */
void WINAPI wine_gluBeginSurface( GLUnurbs *nobj )
{
    assert( 0 );
}

/***********************************************************************
 *		gluEndSurface (GLU32.@)
 */
void WINAPI wine_gluEndSurface( GLUnurbs *nobj )
{
    assert( 0 );
}

/***********************************************************************
 *		gluNurbsSurface (GLU32.@)
 */
void WINAPI wine_gluNurbsSurface( GLUnurbs *nobj, GLint sknot_count, float *sknot, GLint tknot_count,
                                  GLfloat *tknot, GLint s_stride, GLint t_stride, GLfloat *ctlarray,
                                  GLint sorder, GLint torder, GLenum type )
{
    assert( 0 );
}

/***********************************************************************
 *		gluBeginTrim (GLU32.@)
 */
void WINAPI wine_gluBeginTrim( GLUnurbs *nobj )
{
    assert( 0 );
}

/***********************************************************************
 *		gluEndTrim (GLU32.@)
 */
void WINAPI wine_gluEndTrim( GLUnurbs *nobj )
{
    assert( 0 );
}

/***********************************************************************
 *		gluPwlCurve (GLU32.@)
 */
void WINAPI wine_gluPwlCurve( GLUnurbs *nobj, GLint count, GLfloat *array, GLint stride, GLenum type )
{
    assert( 0 );
}

/***********************************************************************
 *		gluNurbsCallback (GLU32.@)
 */
void WINAPI wine_gluNurbsCallback( GLUnurbs *nobj, GLenum which, void (CALLBACK *fn)(void) )
{
    assert( 0 );
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
