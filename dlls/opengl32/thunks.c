/* Automatically generated from http://www.opengl.org/registry files; DO NOT EDIT! */

#include <stdarg.h>
#include <stddef.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"

#include "unixlib.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(opengl);

void WINAPI glAccum( GLenum op, GLfloat value )
{
    struct glAccum_params args = { .op = op, .value = value, };
    NTSTATUS status;
    TRACE( "op %d, value %f\n", op, value );
    if ((status = UNIX_CALL( glAccum, &args ))) WARN( "glAccum returned %#x\n", status );
}

void WINAPI glAlphaFunc( GLenum func, GLfloat ref )
{
    struct glAlphaFunc_params args = { .func = func, .ref = ref, };
    NTSTATUS status;
    TRACE( "func %d, ref %f\n", func, ref );
    if ((status = UNIX_CALL( glAlphaFunc, &args ))) WARN( "glAlphaFunc returned %#x\n", status );
}

GLboolean WINAPI glAreTexturesResident( GLsizei n, const GLuint *textures, GLboolean *residences )
{
    struct glAreTexturesResident_params args = { .n = n, .textures = textures, .residences = residences, };
    NTSTATUS status;
    TRACE( "n %d, textures %p, residences %p\n", n, textures, residences );
    if ((status = UNIX_CALL( glAreTexturesResident, &args ))) WARN( "glAreTexturesResident returned %#x\n", status );
    return args.ret;
}

void WINAPI glArrayElement( GLint i )
{
    struct glArrayElement_params args = { .i = i, };
    NTSTATUS status;
    TRACE( "i %d\n", i );
    if ((status = UNIX_CALL( glArrayElement, &args ))) WARN( "glArrayElement returned %#x\n", status );
}

void WINAPI glBegin( GLenum mode )
{
    struct glBegin_params args = { .mode = mode, };
    NTSTATUS status;
    TRACE( "mode %d\n", mode );
    if ((status = UNIX_CALL( glBegin, &args ))) WARN( "glBegin returned %#x\n", status );
}

void WINAPI glBindTexture( GLenum target, GLuint texture )
{
    struct glBindTexture_params args = { .target = target, .texture = texture, };
    NTSTATUS status;
    TRACE( "target %d, texture %d\n", target, texture );
    if ((status = UNIX_CALL( glBindTexture, &args ))) WARN( "glBindTexture returned %#x\n", status );
}

void WINAPI glBitmap( GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap )
{
    struct glBitmap_params args = { .width = width, .height = height, .xorig = xorig, .yorig = yorig, .xmove = xmove, .ymove = ymove, .bitmap = bitmap, };
    NTSTATUS status;
    TRACE( "width %d, height %d, xorig %f, yorig %f, xmove %f, ymove %f, bitmap %p\n", width, height, xorig, yorig, xmove, ymove, bitmap );
    if ((status = UNIX_CALL( glBitmap, &args ))) WARN( "glBitmap returned %#x\n", status );
}

void WINAPI glBlendFunc( GLenum sfactor, GLenum dfactor )
{
    struct glBlendFunc_params args = { .sfactor = sfactor, .dfactor = dfactor, };
    NTSTATUS status;
    TRACE( "sfactor %d, dfactor %d\n", sfactor, dfactor );
    if ((status = UNIX_CALL( glBlendFunc, &args ))) WARN( "glBlendFunc returned %#x\n", status );
}

void WINAPI glCallList( GLuint list )
{
    struct glCallList_params args = { .list = list, };
    NTSTATUS status;
    TRACE( "list %d\n", list );
    if ((status = UNIX_CALL( glCallList, &args ))) WARN( "glCallList returned %#x\n", status );
}

void WINAPI glCallLists( GLsizei n, GLenum type, const void *lists )
{
    struct glCallLists_params args = { .n = n, .type = type, .lists = lists, };
    NTSTATUS status;
    TRACE( "n %d, type %d, lists %p\n", n, type, lists );
    if ((status = UNIX_CALL( glCallLists, &args ))) WARN( "glCallLists returned %#x\n", status );
}

void WINAPI glClear( GLbitfield mask )
{
    struct glClear_params args = { .mask = mask, };
    NTSTATUS status;
    TRACE( "mask %d\n", mask );
    if ((status = UNIX_CALL( glClear, &args ))) WARN( "glClear returned %#x\n", status );
}

void WINAPI glClearAccum( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha )
{
    struct glClearAccum_params args = { .red = red, .green = green, .blue = blue, .alpha = alpha, };
    NTSTATUS status;
    TRACE( "red %f, green %f, blue %f, alpha %f\n", red, green, blue, alpha );
    if ((status = UNIX_CALL( glClearAccum, &args ))) WARN( "glClearAccum returned %#x\n", status );
}

void WINAPI glClearColor( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha )
{
    struct glClearColor_params args = { .red = red, .green = green, .blue = blue, .alpha = alpha, };
    NTSTATUS status;
    TRACE( "red %f, green %f, blue %f, alpha %f\n", red, green, blue, alpha );
    if ((status = UNIX_CALL( glClearColor, &args ))) WARN( "glClearColor returned %#x\n", status );
}

void WINAPI glClearDepth( GLdouble depth )
{
    struct glClearDepth_params args = { .depth = depth, };
    NTSTATUS status;
    TRACE( "depth %f\n", depth );
    if ((status = UNIX_CALL( glClearDepth, &args ))) WARN( "glClearDepth returned %#x\n", status );
}

void WINAPI glClearIndex( GLfloat c )
{
    struct glClearIndex_params args = { .c = c, };
    NTSTATUS status;
    TRACE( "c %f\n", c );
    if ((status = UNIX_CALL( glClearIndex, &args ))) WARN( "glClearIndex returned %#x\n", status );
}

void WINAPI glClearStencil( GLint s )
{
    struct glClearStencil_params args = { .s = s, };
    NTSTATUS status;
    TRACE( "s %d\n", s );
    if ((status = UNIX_CALL( glClearStencil, &args ))) WARN( "glClearStencil returned %#x\n", status );
}

void WINAPI glClipPlane( GLenum plane, const GLdouble *equation )
{
    struct glClipPlane_params args = { .plane = plane, .equation = equation, };
    NTSTATUS status;
    TRACE( "plane %d, equation %p\n", plane, equation );
    if ((status = UNIX_CALL( glClipPlane, &args ))) WARN( "glClipPlane returned %#x\n", status );
}

void WINAPI glColor3b( GLbyte red, GLbyte green, GLbyte blue )
{
    struct glColor3b_params args = { .red = red, .green = green, .blue = blue, };
    NTSTATUS status;
    TRACE( "red %d, green %d, blue %d\n", red, green, blue );
    if ((status = UNIX_CALL( glColor3b, &args ))) WARN( "glColor3b returned %#x\n", status );
}

void WINAPI glColor3bv( const GLbyte *v )
{
    struct glColor3bv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glColor3bv, &args ))) WARN( "glColor3bv returned %#x\n", status );
}

void WINAPI glColor3d( GLdouble red, GLdouble green, GLdouble blue )
{
    struct glColor3d_params args = { .red = red, .green = green, .blue = blue, };
    NTSTATUS status;
    TRACE( "red %f, green %f, blue %f\n", red, green, blue );
    if ((status = UNIX_CALL( glColor3d, &args ))) WARN( "glColor3d returned %#x\n", status );
}

void WINAPI glColor3dv( const GLdouble *v )
{
    struct glColor3dv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glColor3dv, &args ))) WARN( "glColor3dv returned %#x\n", status );
}

void WINAPI glColor3f( GLfloat red, GLfloat green, GLfloat blue )
{
    struct glColor3f_params args = { .red = red, .green = green, .blue = blue, };
    NTSTATUS status;
    TRACE( "red %f, green %f, blue %f\n", red, green, blue );
    if ((status = UNIX_CALL( glColor3f, &args ))) WARN( "glColor3f returned %#x\n", status );
}

void WINAPI glColor3fv( const GLfloat *v )
{
    struct glColor3fv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glColor3fv, &args ))) WARN( "glColor3fv returned %#x\n", status );
}

void WINAPI glColor3i( GLint red, GLint green, GLint blue )
{
    struct glColor3i_params args = { .red = red, .green = green, .blue = blue, };
    NTSTATUS status;
    TRACE( "red %d, green %d, blue %d\n", red, green, blue );
    if ((status = UNIX_CALL( glColor3i, &args ))) WARN( "glColor3i returned %#x\n", status );
}

void WINAPI glColor3iv( const GLint *v )
{
    struct glColor3iv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glColor3iv, &args ))) WARN( "glColor3iv returned %#x\n", status );
}

void WINAPI glColor3s( GLshort red, GLshort green, GLshort blue )
{
    struct glColor3s_params args = { .red = red, .green = green, .blue = blue, };
    NTSTATUS status;
    TRACE( "red %d, green %d, blue %d\n", red, green, blue );
    if ((status = UNIX_CALL( glColor3s, &args ))) WARN( "glColor3s returned %#x\n", status );
}

void WINAPI glColor3sv( const GLshort *v )
{
    struct glColor3sv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glColor3sv, &args ))) WARN( "glColor3sv returned %#x\n", status );
}

void WINAPI glColor3ub( GLubyte red, GLubyte green, GLubyte blue )
{
    struct glColor3ub_params args = { .red = red, .green = green, .blue = blue, };
    NTSTATUS status;
    TRACE( "red %d, green %d, blue %d\n", red, green, blue );
    if ((status = UNIX_CALL( glColor3ub, &args ))) WARN( "glColor3ub returned %#x\n", status );
}

void WINAPI glColor3ubv( const GLubyte *v )
{
    struct glColor3ubv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glColor3ubv, &args ))) WARN( "glColor3ubv returned %#x\n", status );
}

void WINAPI glColor3ui( GLuint red, GLuint green, GLuint blue )
{
    struct glColor3ui_params args = { .red = red, .green = green, .blue = blue, };
    NTSTATUS status;
    TRACE( "red %d, green %d, blue %d\n", red, green, blue );
    if ((status = UNIX_CALL( glColor3ui, &args ))) WARN( "glColor3ui returned %#x\n", status );
}

void WINAPI glColor3uiv( const GLuint *v )
{
    struct glColor3uiv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glColor3uiv, &args ))) WARN( "glColor3uiv returned %#x\n", status );
}

void WINAPI glColor3us( GLushort red, GLushort green, GLushort blue )
{
    struct glColor3us_params args = { .red = red, .green = green, .blue = blue, };
    NTSTATUS status;
    TRACE( "red %d, green %d, blue %d\n", red, green, blue );
    if ((status = UNIX_CALL( glColor3us, &args ))) WARN( "glColor3us returned %#x\n", status );
}

void WINAPI glColor3usv( const GLushort *v )
{
    struct glColor3usv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glColor3usv, &args ))) WARN( "glColor3usv returned %#x\n", status );
}

void WINAPI glColor4b( GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha )
{
    struct glColor4b_params args = { .red = red, .green = green, .blue = blue, .alpha = alpha, };
    NTSTATUS status;
    TRACE( "red %d, green %d, blue %d, alpha %d\n", red, green, blue, alpha );
    if ((status = UNIX_CALL( glColor4b, &args ))) WARN( "glColor4b returned %#x\n", status );
}

void WINAPI glColor4bv( const GLbyte *v )
{
    struct glColor4bv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glColor4bv, &args ))) WARN( "glColor4bv returned %#x\n", status );
}

void WINAPI glColor4d( GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha )
{
    struct glColor4d_params args = { .red = red, .green = green, .blue = blue, .alpha = alpha, };
    NTSTATUS status;
    TRACE( "red %f, green %f, blue %f, alpha %f\n", red, green, blue, alpha );
    if ((status = UNIX_CALL( glColor4d, &args ))) WARN( "glColor4d returned %#x\n", status );
}

void WINAPI glColor4dv( const GLdouble *v )
{
    struct glColor4dv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glColor4dv, &args ))) WARN( "glColor4dv returned %#x\n", status );
}

void WINAPI glColor4f( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha )
{
    struct glColor4f_params args = { .red = red, .green = green, .blue = blue, .alpha = alpha, };
    NTSTATUS status;
    TRACE( "red %f, green %f, blue %f, alpha %f\n", red, green, blue, alpha );
    if ((status = UNIX_CALL( glColor4f, &args ))) WARN( "glColor4f returned %#x\n", status );
}

void WINAPI glColor4fv( const GLfloat *v )
{
    struct glColor4fv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glColor4fv, &args ))) WARN( "glColor4fv returned %#x\n", status );
}

void WINAPI glColor4i( GLint red, GLint green, GLint blue, GLint alpha )
{
    struct glColor4i_params args = { .red = red, .green = green, .blue = blue, .alpha = alpha, };
    NTSTATUS status;
    TRACE( "red %d, green %d, blue %d, alpha %d\n", red, green, blue, alpha );
    if ((status = UNIX_CALL( glColor4i, &args ))) WARN( "glColor4i returned %#x\n", status );
}

void WINAPI glColor4iv( const GLint *v )
{
    struct glColor4iv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glColor4iv, &args ))) WARN( "glColor4iv returned %#x\n", status );
}

void WINAPI glColor4s( GLshort red, GLshort green, GLshort blue, GLshort alpha )
{
    struct glColor4s_params args = { .red = red, .green = green, .blue = blue, .alpha = alpha, };
    NTSTATUS status;
    TRACE( "red %d, green %d, blue %d, alpha %d\n", red, green, blue, alpha );
    if ((status = UNIX_CALL( glColor4s, &args ))) WARN( "glColor4s returned %#x\n", status );
}

void WINAPI glColor4sv( const GLshort *v )
{
    struct glColor4sv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glColor4sv, &args ))) WARN( "glColor4sv returned %#x\n", status );
}

void WINAPI glColor4ub( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha )
{
    struct glColor4ub_params args = { .red = red, .green = green, .blue = blue, .alpha = alpha, };
    NTSTATUS status;
    TRACE( "red %d, green %d, blue %d, alpha %d\n", red, green, blue, alpha );
    if ((status = UNIX_CALL( glColor4ub, &args ))) WARN( "glColor4ub returned %#x\n", status );
}

void WINAPI glColor4ubv( const GLubyte *v )
{
    struct glColor4ubv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glColor4ubv, &args ))) WARN( "glColor4ubv returned %#x\n", status );
}

void WINAPI glColor4ui( GLuint red, GLuint green, GLuint blue, GLuint alpha )
{
    struct glColor4ui_params args = { .red = red, .green = green, .blue = blue, .alpha = alpha, };
    NTSTATUS status;
    TRACE( "red %d, green %d, blue %d, alpha %d\n", red, green, blue, alpha );
    if ((status = UNIX_CALL( glColor4ui, &args ))) WARN( "glColor4ui returned %#x\n", status );
}

void WINAPI glColor4uiv( const GLuint *v )
{
    struct glColor4uiv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glColor4uiv, &args ))) WARN( "glColor4uiv returned %#x\n", status );
}

void WINAPI glColor4us( GLushort red, GLushort green, GLushort blue, GLushort alpha )
{
    struct glColor4us_params args = { .red = red, .green = green, .blue = blue, .alpha = alpha, };
    NTSTATUS status;
    TRACE( "red %d, green %d, blue %d, alpha %d\n", red, green, blue, alpha );
    if ((status = UNIX_CALL( glColor4us, &args ))) WARN( "glColor4us returned %#x\n", status );
}

void WINAPI glColor4usv( const GLushort *v )
{
    struct glColor4usv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glColor4usv, &args ))) WARN( "glColor4usv returned %#x\n", status );
}

void WINAPI glColorMask( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha )
{
    struct glColorMask_params args = { .red = red, .green = green, .blue = blue, .alpha = alpha, };
    NTSTATUS status;
    TRACE( "red %d, green %d, blue %d, alpha %d\n", red, green, blue, alpha );
    if ((status = UNIX_CALL( glColorMask, &args ))) WARN( "glColorMask returned %#x\n", status );
}

void WINAPI glColorMaterial( GLenum face, GLenum mode )
{
    struct glColorMaterial_params args = { .face = face, .mode = mode, };
    NTSTATUS status;
    TRACE( "face %d, mode %d\n", face, mode );
    if ((status = UNIX_CALL( glColorMaterial, &args ))) WARN( "glColorMaterial returned %#x\n", status );
}

void WINAPI glColorPointer( GLint size, GLenum type, GLsizei stride, const void *pointer )
{
    struct glColorPointer_params args = { .size = size, .type = type, .stride = stride, .pointer = pointer, };
    NTSTATUS status;
    TRACE( "size %d, type %d, stride %d, pointer %p\n", size, type, stride, pointer );
    if ((status = UNIX_CALL( glColorPointer, &args ))) WARN( "glColorPointer returned %#x\n", status );
}

void WINAPI glCopyPixels( GLint x, GLint y, GLsizei width, GLsizei height, GLenum type )
{
    struct glCopyPixels_params args = { .x = x, .y = y, .width = width, .height = height, .type = type, };
    NTSTATUS status;
    TRACE( "x %d, y %d, width %d, height %d, type %d\n", x, y, width, height, type );
    if ((status = UNIX_CALL( glCopyPixels, &args ))) WARN( "glCopyPixels returned %#x\n", status );
}

void WINAPI glCopyTexImage1D( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border )
{
    struct glCopyTexImage1D_params args = { .target = target, .level = level, .internalformat = internalformat, .x = x, .y = y, .width = width, .border = border, };
    NTSTATUS status;
    TRACE( "target %d, level %d, internalformat %d, x %d, y %d, width %d, border %d\n", target, level, internalformat, x, y, width, border );
    if ((status = UNIX_CALL( glCopyTexImage1D, &args ))) WARN( "glCopyTexImage1D returned %#x\n", status );
}

void WINAPI glCopyTexImage2D( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border )
{
    struct glCopyTexImage2D_params args = { .target = target, .level = level, .internalformat = internalformat, .x = x, .y = y, .width = width, .height = height, .border = border, };
    NTSTATUS status;
    TRACE( "target %d, level %d, internalformat %d, x %d, y %d, width %d, height %d, border %d\n", target, level, internalformat, x, y, width, height, border );
    if ((status = UNIX_CALL( glCopyTexImage2D, &args ))) WARN( "glCopyTexImage2D returned %#x\n", status );
}

void WINAPI glCopyTexSubImage1D( GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width )
{
    struct glCopyTexSubImage1D_params args = { .target = target, .level = level, .xoffset = xoffset, .x = x, .y = y, .width = width, };
    NTSTATUS status;
    TRACE( "target %d, level %d, xoffset %d, x %d, y %d, width %d\n", target, level, xoffset, x, y, width );
    if ((status = UNIX_CALL( glCopyTexSubImage1D, &args ))) WARN( "glCopyTexSubImage1D returned %#x\n", status );
}

void WINAPI glCopyTexSubImage2D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height )
{
    struct glCopyTexSubImage2D_params args = { .target = target, .level = level, .xoffset = xoffset, .yoffset = yoffset, .x = x, .y = y, .width = width, .height = height, };
    NTSTATUS status;
    TRACE( "target %d, level %d, xoffset %d, yoffset %d, x %d, y %d, width %d, height %d\n", target, level, xoffset, yoffset, x, y, width, height );
    if ((status = UNIX_CALL( glCopyTexSubImage2D, &args ))) WARN( "glCopyTexSubImage2D returned %#x\n", status );
}

void WINAPI glCullFace( GLenum mode )
{
    struct glCullFace_params args = { .mode = mode, };
    NTSTATUS status;
    TRACE( "mode %d\n", mode );
    if ((status = UNIX_CALL( glCullFace, &args ))) WARN( "glCullFace returned %#x\n", status );
}

void WINAPI glDeleteLists( GLuint list, GLsizei range )
{
    struct glDeleteLists_params args = { .list = list, .range = range, };
    NTSTATUS status;
    TRACE( "list %d, range %d\n", list, range );
    if ((status = UNIX_CALL( glDeleteLists, &args ))) WARN( "glDeleteLists returned %#x\n", status );
}

void WINAPI glDeleteTextures( GLsizei n, const GLuint *textures )
{
    struct glDeleteTextures_params args = { .n = n, .textures = textures, };
    NTSTATUS status;
    TRACE( "n %d, textures %p\n", n, textures );
    if ((status = UNIX_CALL( glDeleteTextures, &args ))) WARN( "glDeleteTextures returned %#x\n", status );
}

void WINAPI glDepthFunc( GLenum func )
{
    struct glDepthFunc_params args = { .func = func, };
    NTSTATUS status;
    TRACE( "func %d\n", func );
    if ((status = UNIX_CALL( glDepthFunc, &args ))) WARN( "glDepthFunc returned %#x\n", status );
}

void WINAPI glDepthMask( GLboolean flag )
{
    struct glDepthMask_params args = { .flag = flag, };
    NTSTATUS status;
    TRACE( "flag %d\n", flag );
    if ((status = UNIX_CALL( glDepthMask, &args ))) WARN( "glDepthMask returned %#x\n", status );
}

void WINAPI glDepthRange( GLdouble n, GLdouble f )
{
    struct glDepthRange_params args = { .n = n, .f = f, };
    NTSTATUS status;
    TRACE( "n %f, f %f\n", n, f );
    if ((status = UNIX_CALL( glDepthRange, &args ))) WARN( "glDepthRange returned %#x\n", status );
}

void WINAPI glDisable( GLenum cap )
{
    struct glDisable_params args = { .cap = cap, };
    NTSTATUS status;
    TRACE( "cap %d\n", cap );
    if ((status = UNIX_CALL( glDisable, &args ))) WARN( "glDisable returned %#x\n", status );
}

void WINAPI glDisableClientState( GLenum array )
{
    struct glDisableClientState_params args = { .array = array, };
    NTSTATUS status;
    TRACE( "array %d\n", array );
    if ((status = UNIX_CALL( glDisableClientState, &args ))) WARN( "glDisableClientState returned %#x\n", status );
}

void WINAPI glDrawArrays( GLenum mode, GLint first, GLsizei count )
{
    struct glDrawArrays_params args = { .mode = mode, .first = first, .count = count, };
    NTSTATUS status;
    TRACE( "mode %d, first %d, count %d\n", mode, first, count );
    if ((status = UNIX_CALL( glDrawArrays, &args ))) WARN( "glDrawArrays returned %#x\n", status );
}

void WINAPI glDrawBuffer( GLenum buf )
{
    struct glDrawBuffer_params args = { .buf = buf, };
    NTSTATUS status;
    TRACE( "buf %d\n", buf );
    if ((status = UNIX_CALL( glDrawBuffer, &args ))) WARN( "glDrawBuffer returned %#x\n", status );
}

void WINAPI glDrawElements( GLenum mode, GLsizei count, GLenum type, const void *indices )
{
    struct glDrawElements_params args = { .mode = mode, .count = count, .type = type, .indices = indices, };
    NTSTATUS status;
    TRACE( "mode %d, count %d, type %d, indices %p\n", mode, count, type, indices );
    if ((status = UNIX_CALL( glDrawElements, &args ))) WARN( "glDrawElements returned %#x\n", status );
}

void WINAPI glDrawPixels( GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels )
{
    struct glDrawPixels_params args = { .width = width, .height = height, .format = format, .type = type, .pixels = pixels, };
    NTSTATUS status;
    TRACE( "width %d, height %d, format %d, type %d, pixels %p\n", width, height, format, type, pixels );
    if ((status = UNIX_CALL( glDrawPixels, &args ))) WARN( "glDrawPixels returned %#x\n", status );
}

void WINAPI glEdgeFlag( GLboolean flag )
{
    struct glEdgeFlag_params args = { .flag = flag, };
    NTSTATUS status;
    TRACE( "flag %d\n", flag );
    if ((status = UNIX_CALL( glEdgeFlag, &args ))) WARN( "glEdgeFlag returned %#x\n", status );
}

void WINAPI glEdgeFlagPointer( GLsizei stride, const void *pointer )
{
    struct glEdgeFlagPointer_params args = { .stride = stride, .pointer = pointer, };
    NTSTATUS status;
    TRACE( "stride %d, pointer %p\n", stride, pointer );
    if ((status = UNIX_CALL( glEdgeFlagPointer, &args ))) WARN( "glEdgeFlagPointer returned %#x\n", status );
}

void WINAPI glEdgeFlagv( const GLboolean *flag )
{
    struct glEdgeFlagv_params args = { .flag = flag, };
    NTSTATUS status;
    TRACE( "flag %p\n", flag );
    if ((status = UNIX_CALL( glEdgeFlagv, &args ))) WARN( "glEdgeFlagv returned %#x\n", status );
}

void WINAPI glEnable( GLenum cap )
{
    struct glEnable_params args = { .cap = cap, };
    NTSTATUS status;
    TRACE( "cap %d\n", cap );
    if ((status = UNIX_CALL( glEnable, &args ))) WARN( "glEnable returned %#x\n", status );
}

void WINAPI glEnableClientState( GLenum array )
{
    struct glEnableClientState_params args = { .array = array, };
    NTSTATUS status;
    TRACE( "array %d\n", array );
    if ((status = UNIX_CALL( glEnableClientState, &args ))) WARN( "glEnableClientState returned %#x\n", status );
}

void WINAPI glEnd(void)
{
    struct glEnd_params args;
    NTSTATUS status;
    TRACE( "\n" );
    if ((status = UNIX_CALL( glEnd, &args ))) WARN( "glEnd returned %#x\n", status );
}

void WINAPI glEndList(void)
{
    struct glEndList_params args;
    NTSTATUS status;
    TRACE( "\n" );
    if ((status = UNIX_CALL( glEndList, &args ))) WARN( "glEndList returned %#x\n", status );
}

void WINAPI glEvalCoord1d( GLdouble u )
{
    struct glEvalCoord1d_params args = { .u = u, };
    NTSTATUS status;
    TRACE( "u %f\n", u );
    if ((status = UNIX_CALL( glEvalCoord1d, &args ))) WARN( "glEvalCoord1d returned %#x\n", status );
}

void WINAPI glEvalCoord1dv( const GLdouble *u )
{
    struct glEvalCoord1dv_params args = { .u = u, };
    NTSTATUS status;
    TRACE( "u %p\n", u );
    if ((status = UNIX_CALL( glEvalCoord1dv, &args ))) WARN( "glEvalCoord1dv returned %#x\n", status );
}

void WINAPI glEvalCoord1f( GLfloat u )
{
    struct glEvalCoord1f_params args = { .u = u, };
    NTSTATUS status;
    TRACE( "u %f\n", u );
    if ((status = UNIX_CALL( glEvalCoord1f, &args ))) WARN( "glEvalCoord1f returned %#x\n", status );
}

void WINAPI glEvalCoord1fv( const GLfloat *u )
{
    struct glEvalCoord1fv_params args = { .u = u, };
    NTSTATUS status;
    TRACE( "u %p\n", u );
    if ((status = UNIX_CALL( glEvalCoord1fv, &args ))) WARN( "glEvalCoord1fv returned %#x\n", status );
}

void WINAPI glEvalCoord2d( GLdouble u, GLdouble v )
{
    struct glEvalCoord2d_params args = { .u = u, .v = v, };
    NTSTATUS status;
    TRACE( "u %f, v %f\n", u, v );
    if ((status = UNIX_CALL( glEvalCoord2d, &args ))) WARN( "glEvalCoord2d returned %#x\n", status );
}

void WINAPI glEvalCoord2dv( const GLdouble *u )
{
    struct glEvalCoord2dv_params args = { .u = u, };
    NTSTATUS status;
    TRACE( "u %p\n", u );
    if ((status = UNIX_CALL( glEvalCoord2dv, &args ))) WARN( "glEvalCoord2dv returned %#x\n", status );
}

void WINAPI glEvalCoord2f( GLfloat u, GLfloat v )
{
    struct glEvalCoord2f_params args = { .u = u, .v = v, };
    NTSTATUS status;
    TRACE( "u %f, v %f\n", u, v );
    if ((status = UNIX_CALL( glEvalCoord2f, &args ))) WARN( "glEvalCoord2f returned %#x\n", status );
}

void WINAPI glEvalCoord2fv( const GLfloat *u )
{
    struct glEvalCoord2fv_params args = { .u = u, };
    NTSTATUS status;
    TRACE( "u %p\n", u );
    if ((status = UNIX_CALL( glEvalCoord2fv, &args ))) WARN( "glEvalCoord2fv returned %#x\n", status );
}

void WINAPI glEvalMesh1( GLenum mode, GLint i1, GLint i2 )
{
    struct glEvalMesh1_params args = { .mode = mode, .i1 = i1, .i2 = i2, };
    NTSTATUS status;
    TRACE( "mode %d, i1 %d, i2 %d\n", mode, i1, i2 );
    if ((status = UNIX_CALL( glEvalMesh1, &args ))) WARN( "glEvalMesh1 returned %#x\n", status );
}

void WINAPI glEvalMesh2( GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2 )
{
    struct glEvalMesh2_params args = { .mode = mode, .i1 = i1, .i2 = i2, .j1 = j1, .j2 = j2, };
    NTSTATUS status;
    TRACE( "mode %d, i1 %d, i2 %d, j1 %d, j2 %d\n", mode, i1, i2, j1, j2 );
    if ((status = UNIX_CALL( glEvalMesh2, &args ))) WARN( "glEvalMesh2 returned %#x\n", status );
}

void WINAPI glEvalPoint1( GLint i )
{
    struct glEvalPoint1_params args = { .i = i, };
    NTSTATUS status;
    TRACE( "i %d\n", i );
    if ((status = UNIX_CALL( glEvalPoint1, &args ))) WARN( "glEvalPoint1 returned %#x\n", status );
}

void WINAPI glEvalPoint2( GLint i, GLint j )
{
    struct glEvalPoint2_params args = { .i = i, .j = j, };
    NTSTATUS status;
    TRACE( "i %d, j %d\n", i, j );
    if ((status = UNIX_CALL( glEvalPoint2, &args ))) WARN( "glEvalPoint2 returned %#x\n", status );
}

void WINAPI glFeedbackBuffer( GLsizei size, GLenum type, GLfloat *buffer )
{
    struct glFeedbackBuffer_params args = { .size = size, .type = type, .buffer = buffer, };
    NTSTATUS status;
    TRACE( "size %d, type %d, buffer %p\n", size, type, buffer );
    if ((status = UNIX_CALL( glFeedbackBuffer, &args ))) WARN( "glFeedbackBuffer returned %#x\n", status );
}

void WINAPI glFinish(void)
{
    struct glFinish_params args;
    NTSTATUS status;
    TRACE( "\n" );
    if ((status = UNIX_CALL( glFinish, &args ))) WARN( "glFinish returned %#x\n", status );
}

void WINAPI glFlush(void)
{
    struct glFlush_params args;
    NTSTATUS status;
    TRACE( "\n" );
    if ((status = UNIX_CALL( glFlush, &args ))) WARN( "glFlush returned %#x\n", status );
}

void WINAPI glFogf( GLenum pname, GLfloat param )
{
    struct glFogf_params args = { .pname = pname, .param = param, };
    NTSTATUS status;
    TRACE( "pname %d, param %f\n", pname, param );
    if ((status = UNIX_CALL( glFogf, &args ))) WARN( "glFogf returned %#x\n", status );
}

void WINAPI glFogfv( GLenum pname, const GLfloat *params )
{
    struct glFogfv_params args = { .pname = pname, .params = params, };
    NTSTATUS status;
    TRACE( "pname %d, params %p\n", pname, params );
    if ((status = UNIX_CALL( glFogfv, &args ))) WARN( "glFogfv returned %#x\n", status );
}

void WINAPI glFogi( GLenum pname, GLint param )
{
    struct glFogi_params args = { .pname = pname, .param = param, };
    NTSTATUS status;
    TRACE( "pname %d, param %d\n", pname, param );
    if ((status = UNIX_CALL( glFogi, &args ))) WARN( "glFogi returned %#x\n", status );
}

void WINAPI glFogiv( GLenum pname, const GLint *params )
{
    struct glFogiv_params args = { .pname = pname, .params = params, };
    NTSTATUS status;
    TRACE( "pname %d, params %p\n", pname, params );
    if ((status = UNIX_CALL( glFogiv, &args ))) WARN( "glFogiv returned %#x\n", status );
}

void WINAPI glFrontFace( GLenum mode )
{
    struct glFrontFace_params args = { .mode = mode, };
    NTSTATUS status;
    TRACE( "mode %d\n", mode );
    if ((status = UNIX_CALL( glFrontFace, &args ))) WARN( "glFrontFace returned %#x\n", status );
}

void WINAPI glFrustum( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar )
{
    struct glFrustum_params args = { .left = left, .right = right, .bottom = bottom, .top = top, .zNear = zNear, .zFar = zFar, };
    NTSTATUS status;
    TRACE( "left %f, right %f, bottom %f, top %f, zNear %f, zFar %f\n", left, right, bottom, top, zNear, zFar );
    if ((status = UNIX_CALL( glFrustum, &args ))) WARN( "glFrustum returned %#x\n", status );
}

GLuint WINAPI glGenLists( GLsizei range )
{
    struct glGenLists_params args = { .range = range, };
    NTSTATUS status;
    TRACE( "range %d\n", range );
    if ((status = UNIX_CALL( glGenLists, &args ))) WARN( "glGenLists returned %#x\n", status );
    return args.ret;
}

void WINAPI glGenTextures( GLsizei n, GLuint *textures )
{
    struct glGenTextures_params args = { .n = n, .textures = textures, };
    NTSTATUS status;
    TRACE( "n %d, textures %p\n", n, textures );
    if ((status = UNIX_CALL( glGenTextures, &args ))) WARN( "glGenTextures returned %#x\n", status );
}

void WINAPI glGetBooleanv( GLenum pname, GLboolean *data )
{
    struct glGetBooleanv_params args = { .pname = pname, .data = data, };
    NTSTATUS status;
    TRACE( "pname %d, data %p\n", pname, data );
    if ((status = UNIX_CALL( glGetBooleanv, &args ))) WARN( "glGetBooleanv returned %#x\n", status );
}

void WINAPI glGetClipPlane( GLenum plane, GLdouble *equation )
{
    struct glGetClipPlane_params args = { .plane = plane, .equation = equation, };
    NTSTATUS status;
    TRACE( "plane %d, equation %p\n", plane, equation );
    if ((status = UNIX_CALL( glGetClipPlane, &args ))) WARN( "glGetClipPlane returned %#x\n", status );
}

void WINAPI glGetDoublev( GLenum pname, GLdouble *data )
{
    struct glGetDoublev_params args = { .pname = pname, .data = data, };
    NTSTATUS status;
    TRACE( "pname %d, data %p\n", pname, data );
    if ((status = UNIX_CALL( glGetDoublev, &args ))) WARN( "glGetDoublev returned %#x\n", status );
}

GLenum WINAPI glGetError(void)
{
    struct glGetError_params args = {0};
    NTSTATUS status;
    TRACE( "\n" );
    if ((status = UNIX_CALL( glGetError, &args ))) WARN( "glGetError returned %#x\n", status );
    return args.ret;
}

void WINAPI glGetFloatv( GLenum pname, GLfloat *data )
{
    struct glGetFloatv_params args = { .pname = pname, .data = data, };
    NTSTATUS status;
    TRACE( "pname %d, data %p\n", pname, data );
    if ((status = UNIX_CALL( glGetFloatv, &args ))) WARN( "glGetFloatv returned %#x\n", status );
}

void WINAPI glGetLightfv( GLenum light, GLenum pname, GLfloat *params )
{
    struct glGetLightfv_params args = { .light = light, .pname = pname, .params = params, };
    NTSTATUS status;
    TRACE( "light %d, pname %d, params %p\n", light, pname, params );
    if ((status = UNIX_CALL( glGetLightfv, &args ))) WARN( "glGetLightfv returned %#x\n", status );
}

void WINAPI glGetLightiv( GLenum light, GLenum pname, GLint *params )
{
    struct glGetLightiv_params args = { .light = light, .pname = pname, .params = params, };
    NTSTATUS status;
    TRACE( "light %d, pname %d, params %p\n", light, pname, params );
    if ((status = UNIX_CALL( glGetLightiv, &args ))) WARN( "glGetLightiv returned %#x\n", status );
}

void WINAPI glGetMapdv( GLenum target, GLenum query, GLdouble *v )
{
    struct glGetMapdv_params args = { .target = target, .query = query, .v = v, };
    NTSTATUS status;
    TRACE( "target %d, query %d, v %p\n", target, query, v );
    if ((status = UNIX_CALL( glGetMapdv, &args ))) WARN( "glGetMapdv returned %#x\n", status );
}

void WINAPI glGetMapfv( GLenum target, GLenum query, GLfloat *v )
{
    struct glGetMapfv_params args = { .target = target, .query = query, .v = v, };
    NTSTATUS status;
    TRACE( "target %d, query %d, v %p\n", target, query, v );
    if ((status = UNIX_CALL( glGetMapfv, &args ))) WARN( "glGetMapfv returned %#x\n", status );
}

void WINAPI glGetMapiv( GLenum target, GLenum query, GLint *v )
{
    struct glGetMapiv_params args = { .target = target, .query = query, .v = v, };
    NTSTATUS status;
    TRACE( "target %d, query %d, v %p\n", target, query, v );
    if ((status = UNIX_CALL( glGetMapiv, &args ))) WARN( "glGetMapiv returned %#x\n", status );
}

void WINAPI glGetMaterialfv( GLenum face, GLenum pname, GLfloat *params )
{
    struct glGetMaterialfv_params args = { .face = face, .pname = pname, .params = params, };
    NTSTATUS status;
    TRACE( "face %d, pname %d, params %p\n", face, pname, params );
    if ((status = UNIX_CALL( glGetMaterialfv, &args ))) WARN( "glGetMaterialfv returned %#x\n", status );
}

void WINAPI glGetMaterialiv( GLenum face, GLenum pname, GLint *params )
{
    struct glGetMaterialiv_params args = { .face = face, .pname = pname, .params = params, };
    NTSTATUS status;
    TRACE( "face %d, pname %d, params %p\n", face, pname, params );
    if ((status = UNIX_CALL( glGetMaterialiv, &args ))) WARN( "glGetMaterialiv returned %#x\n", status );
}

void WINAPI glGetPixelMapfv( GLenum map, GLfloat *values )
{
    struct glGetPixelMapfv_params args = { .map = map, .values = values, };
    NTSTATUS status;
    TRACE( "map %d, values %p\n", map, values );
    if ((status = UNIX_CALL( glGetPixelMapfv, &args ))) WARN( "glGetPixelMapfv returned %#x\n", status );
}

void WINAPI glGetPixelMapuiv( GLenum map, GLuint *values )
{
    struct glGetPixelMapuiv_params args = { .map = map, .values = values, };
    NTSTATUS status;
    TRACE( "map %d, values %p\n", map, values );
    if ((status = UNIX_CALL( glGetPixelMapuiv, &args ))) WARN( "glGetPixelMapuiv returned %#x\n", status );
}

void WINAPI glGetPixelMapusv( GLenum map, GLushort *values )
{
    struct glGetPixelMapusv_params args = { .map = map, .values = values, };
    NTSTATUS status;
    TRACE( "map %d, values %p\n", map, values );
    if ((status = UNIX_CALL( glGetPixelMapusv, &args ))) WARN( "glGetPixelMapusv returned %#x\n", status );
}

void WINAPI glGetPointerv( GLenum pname, void **params )
{
    struct glGetPointerv_params args = { .pname = pname, .params = params, };
    NTSTATUS status;
    TRACE( "pname %d, params %p\n", pname, params );
    if ((status = UNIX_CALL( glGetPointerv, &args ))) WARN( "glGetPointerv returned %#x\n", status );
}

void WINAPI glGetPolygonStipple( GLubyte *mask )
{
    struct glGetPolygonStipple_params args = { .mask = mask, };
    NTSTATUS status;
    TRACE( "mask %p\n", mask );
    if ((status = UNIX_CALL( glGetPolygonStipple, &args ))) WARN( "glGetPolygonStipple returned %#x\n", status );
}

void WINAPI glGetTexEnvfv( GLenum target, GLenum pname, GLfloat *params )
{
    struct glGetTexEnvfv_params args = { .target = target, .pname = pname, .params = params, };
    NTSTATUS status;
    TRACE( "target %d, pname %d, params %p\n", target, pname, params );
    if ((status = UNIX_CALL( glGetTexEnvfv, &args ))) WARN( "glGetTexEnvfv returned %#x\n", status );
}

void WINAPI glGetTexEnviv( GLenum target, GLenum pname, GLint *params )
{
    struct glGetTexEnviv_params args = { .target = target, .pname = pname, .params = params, };
    NTSTATUS status;
    TRACE( "target %d, pname %d, params %p\n", target, pname, params );
    if ((status = UNIX_CALL( glGetTexEnviv, &args ))) WARN( "glGetTexEnviv returned %#x\n", status );
}

void WINAPI glGetTexGendv( GLenum coord, GLenum pname, GLdouble *params )
{
    struct glGetTexGendv_params args = { .coord = coord, .pname = pname, .params = params, };
    NTSTATUS status;
    TRACE( "coord %d, pname %d, params %p\n", coord, pname, params );
    if ((status = UNIX_CALL( glGetTexGendv, &args ))) WARN( "glGetTexGendv returned %#x\n", status );
}

void WINAPI glGetTexGenfv( GLenum coord, GLenum pname, GLfloat *params )
{
    struct glGetTexGenfv_params args = { .coord = coord, .pname = pname, .params = params, };
    NTSTATUS status;
    TRACE( "coord %d, pname %d, params %p\n", coord, pname, params );
    if ((status = UNIX_CALL( glGetTexGenfv, &args ))) WARN( "glGetTexGenfv returned %#x\n", status );
}

void WINAPI glGetTexGeniv( GLenum coord, GLenum pname, GLint *params )
{
    struct glGetTexGeniv_params args = { .coord = coord, .pname = pname, .params = params, };
    NTSTATUS status;
    TRACE( "coord %d, pname %d, params %p\n", coord, pname, params );
    if ((status = UNIX_CALL( glGetTexGeniv, &args ))) WARN( "glGetTexGeniv returned %#x\n", status );
}

void WINAPI glGetTexImage( GLenum target, GLint level, GLenum format, GLenum type, void *pixels )
{
    struct glGetTexImage_params args = { .target = target, .level = level, .format = format, .type = type, .pixels = pixels, };
    NTSTATUS status;
    TRACE( "target %d, level %d, format %d, type %d, pixels %p\n", target, level, format, type, pixels );
    if ((status = UNIX_CALL( glGetTexImage, &args ))) WARN( "glGetTexImage returned %#x\n", status );
}

void WINAPI glGetTexLevelParameterfv( GLenum target, GLint level, GLenum pname, GLfloat *params )
{
    struct glGetTexLevelParameterfv_params args = { .target = target, .level = level, .pname = pname, .params = params, };
    NTSTATUS status;
    TRACE( "target %d, level %d, pname %d, params %p\n", target, level, pname, params );
    if ((status = UNIX_CALL( glGetTexLevelParameterfv, &args ))) WARN( "glGetTexLevelParameterfv returned %#x\n", status );
}

void WINAPI glGetTexLevelParameteriv( GLenum target, GLint level, GLenum pname, GLint *params )
{
    struct glGetTexLevelParameteriv_params args = { .target = target, .level = level, .pname = pname, .params = params, };
    NTSTATUS status;
    TRACE( "target %d, level %d, pname %d, params %p\n", target, level, pname, params );
    if ((status = UNIX_CALL( glGetTexLevelParameteriv, &args ))) WARN( "glGetTexLevelParameteriv returned %#x\n", status );
}

void WINAPI glGetTexParameterfv( GLenum target, GLenum pname, GLfloat *params )
{
    struct glGetTexParameterfv_params args = { .target = target, .pname = pname, .params = params, };
    NTSTATUS status;
    TRACE( "target %d, pname %d, params %p\n", target, pname, params );
    if ((status = UNIX_CALL( glGetTexParameterfv, &args ))) WARN( "glGetTexParameterfv returned %#x\n", status );
}

void WINAPI glGetTexParameteriv( GLenum target, GLenum pname, GLint *params )
{
    struct glGetTexParameteriv_params args = { .target = target, .pname = pname, .params = params, };
    NTSTATUS status;
    TRACE( "target %d, pname %d, params %p\n", target, pname, params );
    if ((status = UNIX_CALL( glGetTexParameteriv, &args ))) WARN( "glGetTexParameteriv returned %#x\n", status );
}

void WINAPI glHint( GLenum target, GLenum mode )
{
    struct glHint_params args = { .target = target, .mode = mode, };
    NTSTATUS status;
    TRACE( "target %d, mode %d\n", target, mode );
    if ((status = UNIX_CALL( glHint, &args ))) WARN( "glHint returned %#x\n", status );
}

void WINAPI glIndexMask( GLuint mask )
{
    struct glIndexMask_params args = { .mask = mask, };
    NTSTATUS status;
    TRACE( "mask %d\n", mask );
    if ((status = UNIX_CALL( glIndexMask, &args ))) WARN( "glIndexMask returned %#x\n", status );
}

void WINAPI glIndexPointer( GLenum type, GLsizei stride, const void *pointer )
{
    struct glIndexPointer_params args = { .type = type, .stride = stride, .pointer = pointer, };
    NTSTATUS status;
    TRACE( "type %d, stride %d, pointer %p\n", type, stride, pointer );
    if ((status = UNIX_CALL( glIndexPointer, &args ))) WARN( "glIndexPointer returned %#x\n", status );
}

void WINAPI glIndexd( GLdouble c )
{
    struct glIndexd_params args = { .c = c, };
    NTSTATUS status;
    TRACE( "c %f\n", c );
    if ((status = UNIX_CALL( glIndexd, &args ))) WARN( "glIndexd returned %#x\n", status );
}

void WINAPI glIndexdv( const GLdouble *c )
{
    struct glIndexdv_params args = { .c = c, };
    NTSTATUS status;
    TRACE( "c %p\n", c );
    if ((status = UNIX_CALL( glIndexdv, &args ))) WARN( "glIndexdv returned %#x\n", status );
}

void WINAPI glIndexf( GLfloat c )
{
    struct glIndexf_params args = { .c = c, };
    NTSTATUS status;
    TRACE( "c %f\n", c );
    if ((status = UNIX_CALL( glIndexf, &args ))) WARN( "glIndexf returned %#x\n", status );
}

void WINAPI glIndexfv( const GLfloat *c )
{
    struct glIndexfv_params args = { .c = c, };
    NTSTATUS status;
    TRACE( "c %p\n", c );
    if ((status = UNIX_CALL( glIndexfv, &args ))) WARN( "glIndexfv returned %#x\n", status );
}

void WINAPI glIndexi( GLint c )
{
    struct glIndexi_params args = { .c = c, };
    NTSTATUS status;
    TRACE( "c %d\n", c );
    if ((status = UNIX_CALL( glIndexi, &args ))) WARN( "glIndexi returned %#x\n", status );
}

void WINAPI glIndexiv( const GLint *c )
{
    struct glIndexiv_params args = { .c = c, };
    NTSTATUS status;
    TRACE( "c %p\n", c );
    if ((status = UNIX_CALL( glIndexiv, &args ))) WARN( "glIndexiv returned %#x\n", status );
}

void WINAPI glIndexs( GLshort c )
{
    struct glIndexs_params args = { .c = c, };
    NTSTATUS status;
    TRACE( "c %d\n", c );
    if ((status = UNIX_CALL( glIndexs, &args ))) WARN( "glIndexs returned %#x\n", status );
}

void WINAPI glIndexsv( const GLshort *c )
{
    struct glIndexsv_params args = { .c = c, };
    NTSTATUS status;
    TRACE( "c %p\n", c );
    if ((status = UNIX_CALL( glIndexsv, &args ))) WARN( "glIndexsv returned %#x\n", status );
}

void WINAPI glIndexub( GLubyte c )
{
    struct glIndexub_params args = { .c = c, };
    NTSTATUS status;
    TRACE( "c %d\n", c );
    if ((status = UNIX_CALL( glIndexub, &args ))) WARN( "glIndexub returned %#x\n", status );
}

void WINAPI glIndexubv( const GLubyte *c )
{
    struct glIndexubv_params args = { .c = c, };
    NTSTATUS status;
    TRACE( "c %p\n", c );
    if ((status = UNIX_CALL( glIndexubv, &args ))) WARN( "glIndexubv returned %#x\n", status );
}

void WINAPI glInitNames(void)
{
    struct glInitNames_params args;
    NTSTATUS status;
    TRACE( "\n" );
    if ((status = UNIX_CALL( glInitNames, &args ))) WARN( "glInitNames returned %#x\n", status );
}

void WINAPI glInterleavedArrays( GLenum format, GLsizei stride, const void *pointer )
{
    struct glInterleavedArrays_params args = { .format = format, .stride = stride, .pointer = pointer, };
    NTSTATUS status;
    TRACE( "format %d, stride %d, pointer %p\n", format, stride, pointer );
    if ((status = UNIX_CALL( glInterleavedArrays, &args ))) WARN( "glInterleavedArrays returned %#x\n", status );
}

GLboolean WINAPI glIsEnabled( GLenum cap )
{
    struct glIsEnabled_params args = { .cap = cap, };
    NTSTATUS status;
    TRACE( "cap %d\n", cap );
    if ((status = UNIX_CALL( glIsEnabled, &args ))) WARN( "glIsEnabled returned %#x\n", status );
    return args.ret;
}

GLboolean WINAPI glIsList( GLuint list )
{
    struct glIsList_params args = { .list = list, };
    NTSTATUS status;
    TRACE( "list %d\n", list );
    if ((status = UNIX_CALL( glIsList, &args ))) WARN( "glIsList returned %#x\n", status );
    return args.ret;
}

GLboolean WINAPI glIsTexture( GLuint texture )
{
    struct glIsTexture_params args = { .texture = texture, };
    NTSTATUS status;
    TRACE( "texture %d\n", texture );
    if ((status = UNIX_CALL( glIsTexture, &args ))) WARN( "glIsTexture returned %#x\n", status );
    return args.ret;
}

void WINAPI glLightModelf( GLenum pname, GLfloat param )
{
    struct glLightModelf_params args = { .pname = pname, .param = param, };
    NTSTATUS status;
    TRACE( "pname %d, param %f\n", pname, param );
    if ((status = UNIX_CALL( glLightModelf, &args ))) WARN( "glLightModelf returned %#x\n", status );
}

void WINAPI glLightModelfv( GLenum pname, const GLfloat *params )
{
    struct glLightModelfv_params args = { .pname = pname, .params = params, };
    NTSTATUS status;
    TRACE( "pname %d, params %p\n", pname, params );
    if ((status = UNIX_CALL( glLightModelfv, &args ))) WARN( "glLightModelfv returned %#x\n", status );
}

void WINAPI glLightModeli( GLenum pname, GLint param )
{
    struct glLightModeli_params args = { .pname = pname, .param = param, };
    NTSTATUS status;
    TRACE( "pname %d, param %d\n", pname, param );
    if ((status = UNIX_CALL( glLightModeli, &args ))) WARN( "glLightModeli returned %#x\n", status );
}

void WINAPI glLightModeliv( GLenum pname, const GLint *params )
{
    struct glLightModeliv_params args = { .pname = pname, .params = params, };
    NTSTATUS status;
    TRACE( "pname %d, params %p\n", pname, params );
    if ((status = UNIX_CALL( glLightModeliv, &args ))) WARN( "glLightModeliv returned %#x\n", status );
}

void WINAPI glLightf( GLenum light, GLenum pname, GLfloat param )
{
    struct glLightf_params args = { .light = light, .pname = pname, .param = param, };
    NTSTATUS status;
    TRACE( "light %d, pname %d, param %f\n", light, pname, param );
    if ((status = UNIX_CALL( glLightf, &args ))) WARN( "glLightf returned %#x\n", status );
}

void WINAPI glLightfv( GLenum light, GLenum pname, const GLfloat *params )
{
    struct glLightfv_params args = { .light = light, .pname = pname, .params = params, };
    NTSTATUS status;
    TRACE( "light %d, pname %d, params %p\n", light, pname, params );
    if ((status = UNIX_CALL( glLightfv, &args ))) WARN( "glLightfv returned %#x\n", status );
}

void WINAPI glLighti( GLenum light, GLenum pname, GLint param )
{
    struct glLighti_params args = { .light = light, .pname = pname, .param = param, };
    NTSTATUS status;
    TRACE( "light %d, pname %d, param %d\n", light, pname, param );
    if ((status = UNIX_CALL( glLighti, &args ))) WARN( "glLighti returned %#x\n", status );
}

void WINAPI glLightiv( GLenum light, GLenum pname, const GLint *params )
{
    struct glLightiv_params args = { .light = light, .pname = pname, .params = params, };
    NTSTATUS status;
    TRACE( "light %d, pname %d, params %p\n", light, pname, params );
    if ((status = UNIX_CALL( glLightiv, &args ))) WARN( "glLightiv returned %#x\n", status );
}

void WINAPI glLineStipple( GLint factor, GLushort pattern )
{
    struct glLineStipple_params args = { .factor = factor, .pattern = pattern, };
    NTSTATUS status;
    TRACE( "factor %d, pattern %d\n", factor, pattern );
    if ((status = UNIX_CALL( glLineStipple, &args ))) WARN( "glLineStipple returned %#x\n", status );
}

void WINAPI glLineWidth( GLfloat width )
{
    struct glLineWidth_params args = { .width = width, };
    NTSTATUS status;
    TRACE( "width %f\n", width );
    if ((status = UNIX_CALL( glLineWidth, &args ))) WARN( "glLineWidth returned %#x\n", status );
}

void WINAPI glListBase( GLuint base )
{
    struct glListBase_params args = { .base = base, };
    NTSTATUS status;
    TRACE( "base %d\n", base );
    if ((status = UNIX_CALL( glListBase, &args ))) WARN( "glListBase returned %#x\n", status );
}

void WINAPI glLoadIdentity(void)
{
    struct glLoadIdentity_params args;
    NTSTATUS status;
    TRACE( "\n" );
    if ((status = UNIX_CALL( glLoadIdentity, &args ))) WARN( "glLoadIdentity returned %#x\n", status );
}

void WINAPI glLoadMatrixd( const GLdouble *m )
{
    struct glLoadMatrixd_params args = { .m = m, };
    NTSTATUS status;
    TRACE( "m %p\n", m );
    if ((status = UNIX_CALL( glLoadMatrixd, &args ))) WARN( "glLoadMatrixd returned %#x\n", status );
}

void WINAPI glLoadMatrixf( const GLfloat *m )
{
    struct glLoadMatrixf_params args = { .m = m, };
    NTSTATUS status;
    TRACE( "m %p\n", m );
    if ((status = UNIX_CALL( glLoadMatrixf, &args ))) WARN( "glLoadMatrixf returned %#x\n", status );
}

void WINAPI glLoadName( GLuint name )
{
    struct glLoadName_params args = { .name = name, };
    NTSTATUS status;
    TRACE( "name %d\n", name );
    if ((status = UNIX_CALL( glLoadName, &args ))) WARN( "glLoadName returned %#x\n", status );
}

void WINAPI glLogicOp( GLenum opcode )
{
    struct glLogicOp_params args = { .opcode = opcode, };
    NTSTATUS status;
    TRACE( "opcode %d\n", opcode );
    if ((status = UNIX_CALL( glLogicOp, &args ))) WARN( "glLogicOp returned %#x\n", status );
}

void WINAPI glMap1d( GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points )
{
    struct glMap1d_params args = { .target = target, .u1 = u1, .u2 = u2, .stride = stride, .order = order, .points = points, };
    NTSTATUS status;
    TRACE( "target %d, u1 %f, u2 %f, stride %d, order %d, points %p\n", target, u1, u2, stride, order, points );
    if ((status = UNIX_CALL( glMap1d, &args ))) WARN( "glMap1d returned %#x\n", status );
}

void WINAPI glMap1f( GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points )
{
    struct glMap1f_params args = { .target = target, .u1 = u1, .u2 = u2, .stride = stride, .order = order, .points = points, };
    NTSTATUS status;
    TRACE( "target %d, u1 %f, u2 %f, stride %d, order %d, points %p\n", target, u1, u2, stride, order, points );
    if ((status = UNIX_CALL( glMap1f, &args ))) WARN( "glMap1f returned %#x\n", status );
}

void WINAPI glMap2d( GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points )
{
    struct glMap2d_params args = { .target = target, .u1 = u1, .u2 = u2, .ustride = ustride, .uorder = uorder, .v1 = v1, .v2 = v2, .vstride = vstride, .vorder = vorder, .points = points, };
    NTSTATUS status;
    TRACE( "target %d, u1 %f, u2 %f, ustride %d, uorder %d, v1 %f, v2 %f, vstride %d, vorder %d, points %p\n", target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points );
    if ((status = UNIX_CALL( glMap2d, &args ))) WARN( "glMap2d returned %#x\n", status );
}

void WINAPI glMap2f( GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points )
{
    struct glMap2f_params args = { .target = target, .u1 = u1, .u2 = u2, .ustride = ustride, .uorder = uorder, .v1 = v1, .v2 = v2, .vstride = vstride, .vorder = vorder, .points = points, };
    NTSTATUS status;
    TRACE( "target %d, u1 %f, u2 %f, ustride %d, uorder %d, v1 %f, v2 %f, vstride %d, vorder %d, points %p\n", target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points );
    if ((status = UNIX_CALL( glMap2f, &args ))) WARN( "glMap2f returned %#x\n", status );
}

void WINAPI glMapGrid1d( GLint un, GLdouble u1, GLdouble u2 )
{
    struct glMapGrid1d_params args = { .un = un, .u1 = u1, .u2 = u2, };
    NTSTATUS status;
    TRACE( "un %d, u1 %f, u2 %f\n", un, u1, u2 );
    if ((status = UNIX_CALL( glMapGrid1d, &args ))) WARN( "glMapGrid1d returned %#x\n", status );
}

void WINAPI glMapGrid1f( GLint un, GLfloat u1, GLfloat u2 )
{
    struct glMapGrid1f_params args = { .un = un, .u1 = u1, .u2 = u2, };
    NTSTATUS status;
    TRACE( "un %d, u1 %f, u2 %f\n", un, u1, u2 );
    if ((status = UNIX_CALL( glMapGrid1f, &args ))) WARN( "glMapGrid1f returned %#x\n", status );
}

void WINAPI glMapGrid2d( GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2 )
{
    struct glMapGrid2d_params args = { .un = un, .u1 = u1, .u2 = u2, .vn = vn, .v1 = v1, .v2 = v2, };
    NTSTATUS status;
    TRACE( "un %d, u1 %f, u2 %f, vn %d, v1 %f, v2 %f\n", un, u1, u2, vn, v1, v2 );
    if ((status = UNIX_CALL( glMapGrid2d, &args ))) WARN( "glMapGrid2d returned %#x\n", status );
}

void WINAPI glMapGrid2f( GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2 )
{
    struct glMapGrid2f_params args = { .un = un, .u1 = u1, .u2 = u2, .vn = vn, .v1 = v1, .v2 = v2, };
    NTSTATUS status;
    TRACE( "un %d, u1 %f, u2 %f, vn %d, v1 %f, v2 %f\n", un, u1, u2, vn, v1, v2 );
    if ((status = UNIX_CALL( glMapGrid2f, &args ))) WARN( "glMapGrid2f returned %#x\n", status );
}

void WINAPI glMaterialf( GLenum face, GLenum pname, GLfloat param )
{
    struct glMaterialf_params args = { .face = face, .pname = pname, .param = param, };
    NTSTATUS status;
    TRACE( "face %d, pname %d, param %f\n", face, pname, param );
    if ((status = UNIX_CALL( glMaterialf, &args ))) WARN( "glMaterialf returned %#x\n", status );
}

void WINAPI glMaterialfv( GLenum face, GLenum pname, const GLfloat *params )
{
    struct glMaterialfv_params args = { .face = face, .pname = pname, .params = params, };
    NTSTATUS status;
    TRACE( "face %d, pname %d, params %p\n", face, pname, params );
    if ((status = UNIX_CALL( glMaterialfv, &args ))) WARN( "glMaterialfv returned %#x\n", status );
}

void WINAPI glMateriali( GLenum face, GLenum pname, GLint param )
{
    struct glMateriali_params args = { .face = face, .pname = pname, .param = param, };
    NTSTATUS status;
    TRACE( "face %d, pname %d, param %d\n", face, pname, param );
    if ((status = UNIX_CALL( glMateriali, &args ))) WARN( "glMateriali returned %#x\n", status );
}

void WINAPI glMaterialiv( GLenum face, GLenum pname, const GLint *params )
{
    struct glMaterialiv_params args = { .face = face, .pname = pname, .params = params, };
    NTSTATUS status;
    TRACE( "face %d, pname %d, params %p\n", face, pname, params );
    if ((status = UNIX_CALL( glMaterialiv, &args ))) WARN( "glMaterialiv returned %#x\n", status );
}

void WINAPI glMatrixMode( GLenum mode )
{
    struct glMatrixMode_params args = { .mode = mode, };
    NTSTATUS status;
    TRACE( "mode %d\n", mode );
    if ((status = UNIX_CALL( glMatrixMode, &args ))) WARN( "glMatrixMode returned %#x\n", status );
}

void WINAPI glMultMatrixd( const GLdouble *m )
{
    struct glMultMatrixd_params args = { .m = m, };
    NTSTATUS status;
    TRACE( "m %p\n", m );
    if ((status = UNIX_CALL( glMultMatrixd, &args ))) WARN( "glMultMatrixd returned %#x\n", status );
}

void WINAPI glMultMatrixf( const GLfloat *m )
{
    struct glMultMatrixf_params args = { .m = m, };
    NTSTATUS status;
    TRACE( "m %p\n", m );
    if ((status = UNIX_CALL( glMultMatrixf, &args ))) WARN( "glMultMatrixf returned %#x\n", status );
}

void WINAPI glNewList( GLuint list, GLenum mode )
{
    struct glNewList_params args = { .list = list, .mode = mode, };
    NTSTATUS status;
    TRACE( "list %d, mode %d\n", list, mode );
    if ((status = UNIX_CALL( glNewList, &args ))) WARN( "glNewList returned %#x\n", status );
}

void WINAPI glNormal3b( GLbyte nx, GLbyte ny, GLbyte nz )
{
    struct glNormal3b_params args = { .nx = nx, .ny = ny, .nz = nz, };
    NTSTATUS status;
    TRACE( "nx %d, ny %d, nz %d\n", nx, ny, nz );
    if ((status = UNIX_CALL( glNormal3b, &args ))) WARN( "glNormal3b returned %#x\n", status );
}

void WINAPI glNormal3bv( const GLbyte *v )
{
    struct glNormal3bv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glNormal3bv, &args ))) WARN( "glNormal3bv returned %#x\n", status );
}

void WINAPI glNormal3d( GLdouble nx, GLdouble ny, GLdouble nz )
{
    struct glNormal3d_params args = { .nx = nx, .ny = ny, .nz = nz, };
    NTSTATUS status;
    TRACE( "nx %f, ny %f, nz %f\n", nx, ny, nz );
    if ((status = UNIX_CALL( glNormal3d, &args ))) WARN( "glNormal3d returned %#x\n", status );
}

void WINAPI glNormal3dv( const GLdouble *v )
{
    struct glNormal3dv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glNormal3dv, &args ))) WARN( "glNormal3dv returned %#x\n", status );
}

void WINAPI glNormal3f( GLfloat nx, GLfloat ny, GLfloat nz )
{
    struct glNormal3f_params args = { .nx = nx, .ny = ny, .nz = nz, };
    NTSTATUS status;
    TRACE( "nx %f, ny %f, nz %f\n", nx, ny, nz );
    if ((status = UNIX_CALL( glNormal3f, &args ))) WARN( "glNormal3f returned %#x\n", status );
}

void WINAPI glNormal3fv( const GLfloat *v )
{
    struct glNormal3fv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glNormal3fv, &args ))) WARN( "glNormal3fv returned %#x\n", status );
}

void WINAPI glNormal3i( GLint nx, GLint ny, GLint nz )
{
    struct glNormal3i_params args = { .nx = nx, .ny = ny, .nz = nz, };
    NTSTATUS status;
    TRACE( "nx %d, ny %d, nz %d\n", nx, ny, nz );
    if ((status = UNIX_CALL( glNormal3i, &args ))) WARN( "glNormal3i returned %#x\n", status );
}

void WINAPI glNormal3iv( const GLint *v )
{
    struct glNormal3iv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glNormal3iv, &args ))) WARN( "glNormal3iv returned %#x\n", status );
}

void WINAPI glNormal3s( GLshort nx, GLshort ny, GLshort nz )
{
    struct glNormal3s_params args = { .nx = nx, .ny = ny, .nz = nz, };
    NTSTATUS status;
    TRACE( "nx %d, ny %d, nz %d\n", nx, ny, nz );
    if ((status = UNIX_CALL( glNormal3s, &args ))) WARN( "glNormal3s returned %#x\n", status );
}

void WINAPI glNormal3sv( const GLshort *v )
{
    struct glNormal3sv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glNormal3sv, &args ))) WARN( "glNormal3sv returned %#x\n", status );
}

void WINAPI glNormalPointer( GLenum type, GLsizei stride, const void *pointer )
{
    struct glNormalPointer_params args = { .type = type, .stride = stride, .pointer = pointer, };
    NTSTATUS status;
    TRACE( "type %d, stride %d, pointer %p\n", type, stride, pointer );
    if ((status = UNIX_CALL( glNormalPointer, &args ))) WARN( "glNormalPointer returned %#x\n", status );
}

void WINAPI glOrtho( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar )
{
    struct glOrtho_params args = { .left = left, .right = right, .bottom = bottom, .top = top, .zNear = zNear, .zFar = zFar, };
    NTSTATUS status;
    TRACE( "left %f, right %f, bottom %f, top %f, zNear %f, zFar %f\n", left, right, bottom, top, zNear, zFar );
    if ((status = UNIX_CALL( glOrtho, &args ))) WARN( "glOrtho returned %#x\n", status );
}

void WINAPI glPassThrough( GLfloat token )
{
    struct glPassThrough_params args = { .token = token, };
    NTSTATUS status;
    TRACE( "token %f\n", token );
    if ((status = UNIX_CALL( glPassThrough, &args ))) WARN( "glPassThrough returned %#x\n", status );
}

void WINAPI glPixelMapfv( GLenum map, GLsizei mapsize, const GLfloat *values )
{
    struct glPixelMapfv_params args = { .map = map, .mapsize = mapsize, .values = values, };
    NTSTATUS status;
    TRACE( "map %d, mapsize %d, values %p\n", map, mapsize, values );
    if ((status = UNIX_CALL( glPixelMapfv, &args ))) WARN( "glPixelMapfv returned %#x\n", status );
}

void WINAPI glPixelMapuiv( GLenum map, GLsizei mapsize, const GLuint *values )
{
    struct glPixelMapuiv_params args = { .map = map, .mapsize = mapsize, .values = values, };
    NTSTATUS status;
    TRACE( "map %d, mapsize %d, values %p\n", map, mapsize, values );
    if ((status = UNIX_CALL( glPixelMapuiv, &args ))) WARN( "glPixelMapuiv returned %#x\n", status );
}

void WINAPI glPixelMapusv( GLenum map, GLsizei mapsize, const GLushort *values )
{
    struct glPixelMapusv_params args = { .map = map, .mapsize = mapsize, .values = values, };
    NTSTATUS status;
    TRACE( "map %d, mapsize %d, values %p\n", map, mapsize, values );
    if ((status = UNIX_CALL( glPixelMapusv, &args ))) WARN( "glPixelMapusv returned %#x\n", status );
}

void WINAPI glPixelStoref( GLenum pname, GLfloat param )
{
    struct glPixelStoref_params args = { .pname = pname, .param = param, };
    NTSTATUS status;
    TRACE( "pname %d, param %f\n", pname, param );
    if ((status = UNIX_CALL( glPixelStoref, &args ))) WARN( "glPixelStoref returned %#x\n", status );
}

void WINAPI glPixelStorei( GLenum pname, GLint param )
{
    struct glPixelStorei_params args = { .pname = pname, .param = param, };
    NTSTATUS status;
    TRACE( "pname %d, param %d\n", pname, param );
    if ((status = UNIX_CALL( glPixelStorei, &args ))) WARN( "glPixelStorei returned %#x\n", status );
}

void WINAPI glPixelTransferf( GLenum pname, GLfloat param )
{
    struct glPixelTransferf_params args = { .pname = pname, .param = param, };
    NTSTATUS status;
    TRACE( "pname %d, param %f\n", pname, param );
    if ((status = UNIX_CALL( glPixelTransferf, &args ))) WARN( "glPixelTransferf returned %#x\n", status );
}

void WINAPI glPixelTransferi( GLenum pname, GLint param )
{
    struct glPixelTransferi_params args = { .pname = pname, .param = param, };
    NTSTATUS status;
    TRACE( "pname %d, param %d\n", pname, param );
    if ((status = UNIX_CALL( glPixelTransferi, &args ))) WARN( "glPixelTransferi returned %#x\n", status );
}

void WINAPI glPixelZoom( GLfloat xfactor, GLfloat yfactor )
{
    struct glPixelZoom_params args = { .xfactor = xfactor, .yfactor = yfactor, };
    NTSTATUS status;
    TRACE( "xfactor %f, yfactor %f\n", xfactor, yfactor );
    if ((status = UNIX_CALL( glPixelZoom, &args ))) WARN( "glPixelZoom returned %#x\n", status );
}

void WINAPI glPointSize( GLfloat size )
{
    struct glPointSize_params args = { .size = size, };
    NTSTATUS status;
    TRACE( "size %f\n", size );
    if ((status = UNIX_CALL( glPointSize, &args ))) WARN( "glPointSize returned %#x\n", status );
}

void WINAPI glPolygonMode( GLenum face, GLenum mode )
{
    struct glPolygonMode_params args = { .face = face, .mode = mode, };
    NTSTATUS status;
    TRACE( "face %d, mode %d\n", face, mode );
    if ((status = UNIX_CALL( glPolygonMode, &args ))) WARN( "glPolygonMode returned %#x\n", status );
}

void WINAPI glPolygonOffset( GLfloat factor, GLfloat units )
{
    struct glPolygonOffset_params args = { .factor = factor, .units = units, };
    NTSTATUS status;
    TRACE( "factor %f, units %f\n", factor, units );
    if ((status = UNIX_CALL( glPolygonOffset, &args ))) WARN( "glPolygonOffset returned %#x\n", status );
}

void WINAPI glPolygonStipple( const GLubyte *mask )
{
    struct glPolygonStipple_params args = { .mask = mask, };
    NTSTATUS status;
    TRACE( "mask %p\n", mask );
    if ((status = UNIX_CALL( glPolygonStipple, &args ))) WARN( "glPolygonStipple returned %#x\n", status );
}

void WINAPI glPopAttrib(void)
{
    struct glPopAttrib_params args;
    NTSTATUS status;
    TRACE( "\n" );
    if ((status = UNIX_CALL( glPopAttrib, &args ))) WARN( "glPopAttrib returned %#x\n", status );
}

void WINAPI glPopClientAttrib(void)
{
    struct glPopClientAttrib_params args;
    NTSTATUS status;
    TRACE( "\n" );
    if ((status = UNIX_CALL( glPopClientAttrib, &args ))) WARN( "glPopClientAttrib returned %#x\n", status );
}

void WINAPI glPopMatrix(void)
{
    struct glPopMatrix_params args;
    NTSTATUS status;
    TRACE( "\n" );
    if ((status = UNIX_CALL( glPopMatrix, &args ))) WARN( "glPopMatrix returned %#x\n", status );
}

void WINAPI glPopName(void)
{
    struct glPopName_params args;
    NTSTATUS status;
    TRACE( "\n" );
    if ((status = UNIX_CALL( glPopName, &args ))) WARN( "glPopName returned %#x\n", status );
}

void WINAPI glPrioritizeTextures( GLsizei n, const GLuint *textures, const GLfloat *priorities )
{
    struct glPrioritizeTextures_params args = { .n = n, .textures = textures, .priorities = priorities, };
    NTSTATUS status;
    TRACE( "n %d, textures %p, priorities %p\n", n, textures, priorities );
    if ((status = UNIX_CALL( glPrioritizeTextures, &args ))) WARN( "glPrioritizeTextures returned %#x\n", status );
}

void WINAPI glPushAttrib( GLbitfield mask )
{
    struct glPushAttrib_params args = { .mask = mask, };
    NTSTATUS status;
    TRACE( "mask %d\n", mask );
    if ((status = UNIX_CALL( glPushAttrib, &args ))) WARN( "glPushAttrib returned %#x\n", status );
}

void WINAPI glPushClientAttrib( GLbitfield mask )
{
    struct glPushClientAttrib_params args = { .mask = mask, };
    NTSTATUS status;
    TRACE( "mask %d\n", mask );
    if ((status = UNIX_CALL( glPushClientAttrib, &args ))) WARN( "glPushClientAttrib returned %#x\n", status );
}

void WINAPI glPushMatrix(void)
{
    struct glPushMatrix_params args;
    NTSTATUS status;
    TRACE( "\n" );
    if ((status = UNIX_CALL( glPushMatrix, &args ))) WARN( "glPushMatrix returned %#x\n", status );
}

void WINAPI glPushName( GLuint name )
{
    struct glPushName_params args = { .name = name, };
    NTSTATUS status;
    TRACE( "name %d\n", name );
    if ((status = UNIX_CALL( glPushName, &args ))) WARN( "glPushName returned %#x\n", status );
}

void WINAPI glRasterPos2d( GLdouble x, GLdouble y )
{
    struct glRasterPos2d_params args = { .x = x, .y = y, };
    NTSTATUS status;
    TRACE( "x %f, y %f\n", x, y );
    if ((status = UNIX_CALL( glRasterPos2d, &args ))) WARN( "glRasterPos2d returned %#x\n", status );
}

void WINAPI glRasterPos2dv( const GLdouble *v )
{
    struct glRasterPos2dv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glRasterPos2dv, &args ))) WARN( "glRasterPos2dv returned %#x\n", status );
}

void WINAPI glRasterPos2f( GLfloat x, GLfloat y )
{
    struct glRasterPos2f_params args = { .x = x, .y = y, };
    NTSTATUS status;
    TRACE( "x %f, y %f\n", x, y );
    if ((status = UNIX_CALL( glRasterPos2f, &args ))) WARN( "glRasterPos2f returned %#x\n", status );
}

void WINAPI glRasterPos2fv( const GLfloat *v )
{
    struct glRasterPos2fv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glRasterPos2fv, &args ))) WARN( "glRasterPos2fv returned %#x\n", status );
}

void WINAPI glRasterPos2i( GLint x, GLint y )
{
    struct glRasterPos2i_params args = { .x = x, .y = y, };
    NTSTATUS status;
    TRACE( "x %d, y %d\n", x, y );
    if ((status = UNIX_CALL( glRasterPos2i, &args ))) WARN( "glRasterPos2i returned %#x\n", status );
}

void WINAPI glRasterPos2iv( const GLint *v )
{
    struct glRasterPos2iv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glRasterPos2iv, &args ))) WARN( "glRasterPos2iv returned %#x\n", status );
}

void WINAPI glRasterPos2s( GLshort x, GLshort y )
{
    struct glRasterPos2s_params args = { .x = x, .y = y, };
    NTSTATUS status;
    TRACE( "x %d, y %d\n", x, y );
    if ((status = UNIX_CALL( glRasterPos2s, &args ))) WARN( "glRasterPos2s returned %#x\n", status );
}

void WINAPI glRasterPos2sv( const GLshort *v )
{
    struct glRasterPos2sv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glRasterPos2sv, &args ))) WARN( "glRasterPos2sv returned %#x\n", status );
}

void WINAPI glRasterPos3d( GLdouble x, GLdouble y, GLdouble z )
{
    struct glRasterPos3d_params args = { .x = x, .y = y, .z = z, };
    NTSTATUS status;
    TRACE( "x %f, y %f, z %f\n", x, y, z );
    if ((status = UNIX_CALL( glRasterPos3d, &args ))) WARN( "glRasterPos3d returned %#x\n", status );
}

void WINAPI glRasterPos3dv( const GLdouble *v )
{
    struct glRasterPos3dv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glRasterPos3dv, &args ))) WARN( "glRasterPos3dv returned %#x\n", status );
}

void WINAPI glRasterPos3f( GLfloat x, GLfloat y, GLfloat z )
{
    struct glRasterPos3f_params args = { .x = x, .y = y, .z = z, };
    NTSTATUS status;
    TRACE( "x %f, y %f, z %f\n", x, y, z );
    if ((status = UNIX_CALL( glRasterPos3f, &args ))) WARN( "glRasterPos3f returned %#x\n", status );
}

void WINAPI glRasterPos3fv( const GLfloat *v )
{
    struct glRasterPos3fv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glRasterPos3fv, &args ))) WARN( "glRasterPos3fv returned %#x\n", status );
}

void WINAPI glRasterPos3i( GLint x, GLint y, GLint z )
{
    struct glRasterPos3i_params args = { .x = x, .y = y, .z = z, };
    NTSTATUS status;
    TRACE( "x %d, y %d, z %d\n", x, y, z );
    if ((status = UNIX_CALL( glRasterPos3i, &args ))) WARN( "glRasterPos3i returned %#x\n", status );
}

void WINAPI glRasterPos3iv( const GLint *v )
{
    struct glRasterPos3iv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glRasterPos3iv, &args ))) WARN( "glRasterPos3iv returned %#x\n", status );
}

void WINAPI glRasterPos3s( GLshort x, GLshort y, GLshort z )
{
    struct glRasterPos3s_params args = { .x = x, .y = y, .z = z, };
    NTSTATUS status;
    TRACE( "x %d, y %d, z %d\n", x, y, z );
    if ((status = UNIX_CALL( glRasterPos3s, &args ))) WARN( "glRasterPos3s returned %#x\n", status );
}

void WINAPI glRasterPos3sv( const GLshort *v )
{
    struct glRasterPos3sv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glRasterPos3sv, &args ))) WARN( "glRasterPos3sv returned %#x\n", status );
}

void WINAPI glRasterPos4d( GLdouble x, GLdouble y, GLdouble z, GLdouble w )
{
    struct glRasterPos4d_params args = { .x = x, .y = y, .z = z, .w = w, };
    NTSTATUS status;
    TRACE( "x %f, y %f, z %f, w %f\n", x, y, z, w );
    if ((status = UNIX_CALL( glRasterPos4d, &args ))) WARN( "glRasterPos4d returned %#x\n", status );
}

void WINAPI glRasterPos4dv( const GLdouble *v )
{
    struct glRasterPos4dv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glRasterPos4dv, &args ))) WARN( "glRasterPos4dv returned %#x\n", status );
}

void WINAPI glRasterPos4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
    struct glRasterPos4f_params args = { .x = x, .y = y, .z = z, .w = w, };
    NTSTATUS status;
    TRACE( "x %f, y %f, z %f, w %f\n", x, y, z, w );
    if ((status = UNIX_CALL( glRasterPos4f, &args ))) WARN( "glRasterPos4f returned %#x\n", status );
}

void WINAPI glRasterPos4fv( const GLfloat *v )
{
    struct glRasterPos4fv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glRasterPos4fv, &args ))) WARN( "glRasterPos4fv returned %#x\n", status );
}

void WINAPI glRasterPos4i( GLint x, GLint y, GLint z, GLint w )
{
    struct glRasterPos4i_params args = { .x = x, .y = y, .z = z, .w = w, };
    NTSTATUS status;
    TRACE( "x %d, y %d, z %d, w %d\n", x, y, z, w );
    if ((status = UNIX_CALL( glRasterPos4i, &args ))) WARN( "glRasterPos4i returned %#x\n", status );
}

void WINAPI glRasterPos4iv( const GLint *v )
{
    struct glRasterPos4iv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glRasterPos4iv, &args ))) WARN( "glRasterPos4iv returned %#x\n", status );
}

void WINAPI glRasterPos4s( GLshort x, GLshort y, GLshort z, GLshort w )
{
    struct glRasterPos4s_params args = { .x = x, .y = y, .z = z, .w = w, };
    NTSTATUS status;
    TRACE( "x %d, y %d, z %d, w %d\n", x, y, z, w );
    if ((status = UNIX_CALL( glRasterPos4s, &args ))) WARN( "glRasterPos4s returned %#x\n", status );
}

void WINAPI glRasterPos4sv( const GLshort *v )
{
    struct glRasterPos4sv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glRasterPos4sv, &args ))) WARN( "glRasterPos4sv returned %#x\n", status );
}

void WINAPI glReadBuffer( GLenum src )
{
    struct glReadBuffer_params args = { .src = src, };
    NTSTATUS status;
    TRACE( "src %d\n", src );
    if ((status = UNIX_CALL( glReadBuffer, &args ))) WARN( "glReadBuffer returned %#x\n", status );
}

void WINAPI glReadPixels( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels )
{
    struct glReadPixels_params args = { .x = x, .y = y, .width = width, .height = height, .format = format, .type = type, .pixels = pixels, };
    NTSTATUS status;
    TRACE( "x %d, y %d, width %d, height %d, format %d, type %d, pixels %p\n", x, y, width, height, format, type, pixels );
    if ((status = UNIX_CALL( glReadPixels, &args ))) WARN( "glReadPixels returned %#x\n", status );
}

void WINAPI glRectd( GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2 )
{
    struct glRectd_params args = { .x1 = x1, .y1 = y1, .x2 = x2, .y2 = y2, };
    NTSTATUS status;
    TRACE( "x1 %f, y1 %f, x2 %f, y2 %f\n", x1, y1, x2, y2 );
    if ((status = UNIX_CALL( glRectd, &args ))) WARN( "glRectd returned %#x\n", status );
}

void WINAPI glRectdv( const GLdouble *v1, const GLdouble *v2 )
{
    struct glRectdv_params args = { .v1 = v1, .v2 = v2, };
    NTSTATUS status;
    TRACE( "v1 %p, v2 %p\n", v1, v2 );
    if ((status = UNIX_CALL( glRectdv, &args ))) WARN( "glRectdv returned %#x\n", status );
}

void WINAPI glRectf( GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 )
{
    struct glRectf_params args = { .x1 = x1, .y1 = y1, .x2 = x2, .y2 = y2, };
    NTSTATUS status;
    TRACE( "x1 %f, y1 %f, x2 %f, y2 %f\n", x1, y1, x2, y2 );
    if ((status = UNIX_CALL( glRectf, &args ))) WARN( "glRectf returned %#x\n", status );
}

void WINAPI glRectfv( const GLfloat *v1, const GLfloat *v2 )
{
    struct glRectfv_params args = { .v1 = v1, .v2 = v2, };
    NTSTATUS status;
    TRACE( "v1 %p, v2 %p\n", v1, v2 );
    if ((status = UNIX_CALL( glRectfv, &args ))) WARN( "glRectfv returned %#x\n", status );
}

void WINAPI glRecti( GLint x1, GLint y1, GLint x2, GLint y2 )
{
    struct glRecti_params args = { .x1 = x1, .y1 = y1, .x2 = x2, .y2 = y2, };
    NTSTATUS status;
    TRACE( "x1 %d, y1 %d, x2 %d, y2 %d\n", x1, y1, x2, y2 );
    if ((status = UNIX_CALL( glRecti, &args ))) WARN( "glRecti returned %#x\n", status );
}

void WINAPI glRectiv( const GLint *v1, const GLint *v2 )
{
    struct glRectiv_params args = { .v1 = v1, .v2 = v2, };
    NTSTATUS status;
    TRACE( "v1 %p, v2 %p\n", v1, v2 );
    if ((status = UNIX_CALL( glRectiv, &args ))) WARN( "glRectiv returned %#x\n", status );
}

void WINAPI glRects( GLshort x1, GLshort y1, GLshort x2, GLshort y2 )
{
    struct glRects_params args = { .x1 = x1, .y1 = y1, .x2 = x2, .y2 = y2, };
    NTSTATUS status;
    TRACE( "x1 %d, y1 %d, x2 %d, y2 %d\n", x1, y1, x2, y2 );
    if ((status = UNIX_CALL( glRects, &args ))) WARN( "glRects returned %#x\n", status );
}

void WINAPI glRectsv( const GLshort *v1, const GLshort *v2 )
{
    struct glRectsv_params args = { .v1 = v1, .v2 = v2, };
    NTSTATUS status;
    TRACE( "v1 %p, v2 %p\n", v1, v2 );
    if ((status = UNIX_CALL( glRectsv, &args ))) WARN( "glRectsv returned %#x\n", status );
}

GLint WINAPI glRenderMode( GLenum mode )
{
    struct glRenderMode_params args = { .mode = mode, };
    NTSTATUS status;
    TRACE( "mode %d\n", mode );
    if ((status = UNIX_CALL( glRenderMode, &args ))) WARN( "glRenderMode returned %#x\n", status );
    return args.ret;
}

void WINAPI glRotated( GLdouble angle, GLdouble x, GLdouble y, GLdouble z )
{
    struct glRotated_params args = { .angle = angle, .x = x, .y = y, .z = z, };
    NTSTATUS status;
    TRACE( "angle %f, x %f, y %f, z %f\n", angle, x, y, z );
    if ((status = UNIX_CALL( glRotated, &args ))) WARN( "glRotated returned %#x\n", status );
}

void WINAPI glRotatef( GLfloat angle, GLfloat x, GLfloat y, GLfloat z )
{
    struct glRotatef_params args = { .angle = angle, .x = x, .y = y, .z = z, };
    NTSTATUS status;
    TRACE( "angle %f, x %f, y %f, z %f\n", angle, x, y, z );
    if ((status = UNIX_CALL( glRotatef, &args ))) WARN( "glRotatef returned %#x\n", status );
}

void WINAPI glScaled( GLdouble x, GLdouble y, GLdouble z )
{
    struct glScaled_params args = { .x = x, .y = y, .z = z, };
    NTSTATUS status;
    TRACE( "x %f, y %f, z %f\n", x, y, z );
    if ((status = UNIX_CALL( glScaled, &args ))) WARN( "glScaled returned %#x\n", status );
}

void WINAPI glScalef( GLfloat x, GLfloat y, GLfloat z )
{
    struct glScalef_params args = { .x = x, .y = y, .z = z, };
    NTSTATUS status;
    TRACE( "x %f, y %f, z %f\n", x, y, z );
    if ((status = UNIX_CALL( glScalef, &args ))) WARN( "glScalef returned %#x\n", status );
}

void WINAPI glScissor( GLint x, GLint y, GLsizei width, GLsizei height )
{
    struct glScissor_params args = { .x = x, .y = y, .width = width, .height = height, };
    NTSTATUS status;
    TRACE( "x %d, y %d, width %d, height %d\n", x, y, width, height );
    if ((status = UNIX_CALL( glScissor, &args ))) WARN( "glScissor returned %#x\n", status );
}

void WINAPI glSelectBuffer( GLsizei size, GLuint *buffer )
{
    struct glSelectBuffer_params args = { .size = size, .buffer = buffer, };
    NTSTATUS status;
    TRACE( "size %d, buffer %p\n", size, buffer );
    if ((status = UNIX_CALL( glSelectBuffer, &args ))) WARN( "glSelectBuffer returned %#x\n", status );
}

void WINAPI glShadeModel( GLenum mode )
{
    struct glShadeModel_params args = { .mode = mode, };
    NTSTATUS status;
    TRACE( "mode %d\n", mode );
    if ((status = UNIX_CALL( glShadeModel, &args ))) WARN( "glShadeModel returned %#x\n", status );
}

void WINAPI glStencilFunc( GLenum func, GLint ref, GLuint mask )
{
    struct glStencilFunc_params args = { .func = func, .ref = ref, .mask = mask, };
    NTSTATUS status;
    TRACE( "func %d, ref %d, mask %d\n", func, ref, mask );
    if ((status = UNIX_CALL( glStencilFunc, &args ))) WARN( "glStencilFunc returned %#x\n", status );
}

void WINAPI glStencilMask( GLuint mask )
{
    struct glStencilMask_params args = { .mask = mask, };
    NTSTATUS status;
    TRACE( "mask %d\n", mask );
    if ((status = UNIX_CALL( glStencilMask, &args ))) WARN( "glStencilMask returned %#x\n", status );
}

void WINAPI glStencilOp( GLenum fail, GLenum zfail, GLenum zpass )
{
    struct glStencilOp_params args = { .fail = fail, .zfail = zfail, .zpass = zpass, };
    NTSTATUS status;
    TRACE( "fail %d, zfail %d, zpass %d\n", fail, zfail, zpass );
    if ((status = UNIX_CALL( glStencilOp, &args ))) WARN( "glStencilOp returned %#x\n", status );
}

void WINAPI glTexCoord1d( GLdouble s )
{
    struct glTexCoord1d_params args = { .s = s, };
    NTSTATUS status;
    TRACE( "s %f\n", s );
    if ((status = UNIX_CALL( glTexCoord1d, &args ))) WARN( "glTexCoord1d returned %#x\n", status );
}

void WINAPI glTexCoord1dv( const GLdouble *v )
{
    struct glTexCoord1dv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glTexCoord1dv, &args ))) WARN( "glTexCoord1dv returned %#x\n", status );
}

void WINAPI glTexCoord1f( GLfloat s )
{
    struct glTexCoord1f_params args = { .s = s, };
    NTSTATUS status;
    TRACE( "s %f\n", s );
    if ((status = UNIX_CALL( glTexCoord1f, &args ))) WARN( "glTexCoord1f returned %#x\n", status );
}

void WINAPI glTexCoord1fv( const GLfloat *v )
{
    struct glTexCoord1fv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glTexCoord1fv, &args ))) WARN( "glTexCoord1fv returned %#x\n", status );
}

void WINAPI glTexCoord1i( GLint s )
{
    struct glTexCoord1i_params args = { .s = s, };
    NTSTATUS status;
    TRACE( "s %d\n", s );
    if ((status = UNIX_CALL( glTexCoord1i, &args ))) WARN( "glTexCoord1i returned %#x\n", status );
}

void WINAPI glTexCoord1iv( const GLint *v )
{
    struct glTexCoord1iv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glTexCoord1iv, &args ))) WARN( "glTexCoord1iv returned %#x\n", status );
}

void WINAPI glTexCoord1s( GLshort s )
{
    struct glTexCoord1s_params args = { .s = s, };
    NTSTATUS status;
    TRACE( "s %d\n", s );
    if ((status = UNIX_CALL( glTexCoord1s, &args ))) WARN( "glTexCoord1s returned %#x\n", status );
}

void WINAPI glTexCoord1sv( const GLshort *v )
{
    struct glTexCoord1sv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glTexCoord1sv, &args ))) WARN( "glTexCoord1sv returned %#x\n", status );
}

void WINAPI glTexCoord2d( GLdouble s, GLdouble t )
{
    struct glTexCoord2d_params args = { .s = s, .t = t, };
    NTSTATUS status;
    TRACE( "s %f, t %f\n", s, t );
    if ((status = UNIX_CALL( glTexCoord2d, &args ))) WARN( "glTexCoord2d returned %#x\n", status );
}

void WINAPI glTexCoord2dv( const GLdouble *v )
{
    struct glTexCoord2dv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glTexCoord2dv, &args ))) WARN( "glTexCoord2dv returned %#x\n", status );
}

void WINAPI glTexCoord2f( GLfloat s, GLfloat t )
{
    struct glTexCoord2f_params args = { .s = s, .t = t, };
    NTSTATUS status;
    TRACE( "s %f, t %f\n", s, t );
    if ((status = UNIX_CALL( glTexCoord2f, &args ))) WARN( "glTexCoord2f returned %#x\n", status );
}

void WINAPI glTexCoord2fv( const GLfloat *v )
{
    struct glTexCoord2fv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glTexCoord2fv, &args ))) WARN( "glTexCoord2fv returned %#x\n", status );
}

void WINAPI glTexCoord2i( GLint s, GLint t )
{
    struct glTexCoord2i_params args = { .s = s, .t = t, };
    NTSTATUS status;
    TRACE( "s %d, t %d\n", s, t );
    if ((status = UNIX_CALL( glTexCoord2i, &args ))) WARN( "glTexCoord2i returned %#x\n", status );
}

void WINAPI glTexCoord2iv( const GLint *v )
{
    struct glTexCoord2iv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glTexCoord2iv, &args ))) WARN( "glTexCoord2iv returned %#x\n", status );
}

void WINAPI glTexCoord2s( GLshort s, GLshort t )
{
    struct glTexCoord2s_params args = { .s = s, .t = t, };
    NTSTATUS status;
    TRACE( "s %d, t %d\n", s, t );
    if ((status = UNIX_CALL( glTexCoord2s, &args ))) WARN( "glTexCoord2s returned %#x\n", status );
}

void WINAPI glTexCoord2sv( const GLshort *v )
{
    struct glTexCoord2sv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glTexCoord2sv, &args ))) WARN( "glTexCoord2sv returned %#x\n", status );
}

void WINAPI glTexCoord3d( GLdouble s, GLdouble t, GLdouble r )
{
    struct glTexCoord3d_params args = { .s = s, .t = t, .r = r, };
    NTSTATUS status;
    TRACE( "s %f, t %f, r %f\n", s, t, r );
    if ((status = UNIX_CALL( glTexCoord3d, &args ))) WARN( "glTexCoord3d returned %#x\n", status );
}

void WINAPI glTexCoord3dv( const GLdouble *v )
{
    struct glTexCoord3dv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glTexCoord3dv, &args ))) WARN( "glTexCoord3dv returned %#x\n", status );
}

void WINAPI glTexCoord3f( GLfloat s, GLfloat t, GLfloat r )
{
    struct glTexCoord3f_params args = { .s = s, .t = t, .r = r, };
    NTSTATUS status;
    TRACE( "s %f, t %f, r %f\n", s, t, r );
    if ((status = UNIX_CALL( glTexCoord3f, &args ))) WARN( "glTexCoord3f returned %#x\n", status );
}

void WINAPI glTexCoord3fv( const GLfloat *v )
{
    struct glTexCoord3fv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glTexCoord3fv, &args ))) WARN( "glTexCoord3fv returned %#x\n", status );
}

void WINAPI glTexCoord3i( GLint s, GLint t, GLint r )
{
    struct glTexCoord3i_params args = { .s = s, .t = t, .r = r, };
    NTSTATUS status;
    TRACE( "s %d, t %d, r %d\n", s, t, r );
    if ((status = UNIX_CALL( glTexCoord3i, &args ))) WARN( "glTexCoord3i returned %#x\n", status );
}

void WINAPI glTexCoord3iv( const GLint *v )
{
    struct glTexCoord3iv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glTexCoord3iv, &args ))) WARN( "glTexCoord3iv returned %#x\n", status );
}

void WINAPI glTexCoord3s( GLshort s, GLshort t, GLshort r )
{
    struct glTexCoord3s_params args = { .s = s, .t = t, .r = r, };
    NTSTATUS status;
    TRACE( "s %d, t %d, r %d\n", s, t, r );
    if ((status = UNIX_CALL( glTexCoord3s, &args ))) WARN( "glTexCoord3s returned %#x\n", status );
}

void WINAPI glTexCoord3sv( const GLshort *v )
{
    struct glTexCoord3sv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glTexCoord3sv, &args ))) WARN( "glTexCoord3sv returned %#x\n", status );
}

void WINAPI glTexCoord4d( GLdouble s, GLdouble t, GLdouble r, GLdouble q )
{
    struct glTexCoord4d_params args = { .s = s, .t = t, .r = r, .q = q, };
    NTSTATUS status;
    TRACE( "s %f, t %f, r %f, q %f\n", s, t, r, q );
    if ((status = UNIX_CALL( glTexCoord4d, &args ))) WARN( "glTexCoord4d returned %#x\n", status );
}

void WINAPI glTexCoord4dv( const GLdouble *v )
{
    struct glTexCoord4dv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glTexCoord4dv, &args ))) WARN( "glTexCoord4dv returned %#x\n", status );
}

void WINAPI glTexCoord4f( GLfloat s, GLfloat t, GLfloat r, GLfloat q )
{
    struct glTexCoord4f_params args = { .s = s, .t = t, .r = r, .q = q, };
    NTSTATUS status;
    TRACE( "s %f, t %f, r %f, q %f\n", s, t, r, q );
    if ((status = UNIX_CALL( glTexCoord4f, &args ))) WARN( "glTexCoord4f returned %#x\n", status );
}

void WINAPI glTexCoord4fv( const GLfloat *v )
{
    struct glTexCoord4fv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glTexCoord4fv, &args ))) WARN( "glTexCoord4fv returned %#x\n", status );
}

void WINAPI glTexCoord4i( GLint s, GLint t, GLint r, GLint q )
{
    struct glTexCoord4i_params args = { .s = s, .t = t, .r = r, .q = q, };
    NTSTATUS status;
    TRACE( "s %d, t %d, r %d, q %d\n", s, t, r, q );
    if ((status = UNIX_CALL( glTexCoord4i, &args ))) WARN( "glTexCoord4i returned %#x\n", status );
}

void WINAPI glTexCoord4iv( const GLint *v )
{
    struct glTexCoord4iv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glTexCoord4iv, &args ))) WARN( "glTexCoord4iv returned %#x\n", status );
}

void WINAPI glTexCoord4s( GLshort s, GLshort t, GLshort r, GLshort q )
{
    struct glTexCoord4s_params args = { .s = s, .t = t, .r = r, .q = q, };
    NTSTATUS status;
    TRACE( "s %d, t %d, r %d, q %d\n", s, t, r, q );
    if ((status = UNIX_CALL( glTexCoord4s, &args ))) WARN( "glTexCoord4s returned %#x\n", status );
}

void WINAPI glTexCoord4sv( const GLshort *v )
{
    struct glTexCoord4sv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glTexCoord4sv, &args ))) WARN( "glTexCoord4sv returned %#x\n", status );
}

void WINAPI glTexCoordPointer( GLint size, GLenum type, GLsizei stride, const void *pointer )
{
    struct glTexCoordPointer_params args = { .size = size, .type = type, .stride = stride, .pointer = pointer, };
    NTSTATUS status;
    TRACE( "size %d, type %d, stride %d, pointer %p\n", size, type, stride, pointer );
    if ((status = UNIX_CALL( glTexCoordPointer, &args ))) WARN( "glTexCoordPointer returned %#x\n", status );
}

void WINAPI glTexEnvf( GLenum target, GLenum pname, GLfloat param )
{
    struct glTexEnvf_params args = { .target = target, .pname = pname, .param = param, };
    NTSTATUS status;
    TRACE( "target %d, pname %d, param %f\n", target, pname, param );
    if ((status = UNIX_CALL( glTexEnvf, &args ))) WARN( "glTexEnvf returned %#x\n", status );
}

void WINAPI glTexEnvfv( GLenum target, GLenum pname, const GLfloat *params )
{
    struct glTexEnvfv_params args = { .target = target, .pname = pname, .params = params, };
    NTSTATUS status;
    TRACE( "target %d, pname %d, params %p\n", target, pname, params );
    if ((status = UNIX_CALL( glTexEnvfv, &args ))) WARN( "glTexEnvfv returned %#x\n", status );
}

void WINAPI glTexEnvi( GLenum target, GLenum pname, GLint param )
{
    struct glTexEnvi_params args = { .target = target, .pname = pname, .param = param, };
    NTSTATUS status;
    TRACE( "target %d, pname %d, param %d\n", target, pname, param );
    if ((status = UNIX_CALL( glTexEnvi, &args ))) WARN( "glTexEnvi returned %#x\n", status );
}

void WINAPI glTexEnviv( GLenum target, GLenum pname, const GLint *params )
{
    struct glTexEnviv_params args = { .target = target, .pname = pname, .params = params, };
    NTSTATUS status;
    TRACE( "target %d, pname %d, params %p\n", target, pname, params );
    if ((status = UNIX_CALL( glTexEnviv, &args ))) WARN( "glTexEnviv returned %#x\n", status );
}

void WINAPI glTexGend( GLenum coord, GLenum pname, GLdouble param )
{
    struct glTexGend_params args = { .coord = coord, .pname = pname, .param = param, };
    NTSTATUS status;
    TRACE( "coord %d, pname %d, param %f\n", coord, pname, param );
    if ((status = UNIX_CALL( glTexGend, &args ))) WARN( "glTexGend returned %#x\n", status );
}

void WINAPI glTexGendv( GLenum coord, GLenum pname, const GLdouble *params )
{
    struct glTexGendv_params args = { .coord = coord, .pname = pname, .params = params, };
    NTSTATUS status;
    TRACE( "coord %d, pname %d, params %p\n", coord, pname, params );
    if ((status = UNIX_CALL( glTexGendv, &args ))) WARN( "glTexGendv returned %#x\n", status );
}

void WINAPI glTexGenf( GLenum coord, GLenum pname, GLfloat param )
{
    struct glTexGenf_params args = { .coord = coord, .pname = pname, .param = param, };
    NTSTATUS status;
    TRACE( "coord %d, pname %d, param %f\n", coord, pname, param );
    if ((status = UNIX_CALL( glTexGenf, &args ))) WARN( "glTexGenf returned %#x\n", status );
}

void WINAPI glTexGenfv( GLenum coord, GLenum pname, const GLfloat *params )
{
    struct glTexGenfv_params args = { .coord = coord, .pname = pname, .params = params, };
    NTSTATUS status;
    TRACE( "coord %d, pname %d, params %p\n", coord, pname, params );
    if ((status = UNIX_CALL( glTexGenfv, &args ))) WARN( "glTexGenfv returned %#x\n", status );
}

void WINAPI glTexGeni( GLenum coord, GLenum pname, GLint param )
{
    struct glTexGeni_params args = { .coord = coord, .pname = pname, .param = param, };
    NTSTATUS status;
    TRACE( "coord %d, pname %d, param %d\n", coord, pname, param );
    if ((status = UNIX_CALL( glTexGeni, &args ))) WARN( "glTexGeni returned %#x\n", status );
}

void WINAPI glTexGeniv( GLenum coord, GLenum pname, const GLint *params )
{
    struct glTexGeniv_params args = { .coord = coord, .pname = pname, .params = params, };
    NTSTATUS status;
    TRACE( "coord %d, pname %d, params %p\n", coord, pname, params );
    if ((status = UNIX_CALL( glTexGeniv, &args ))) WARN( "glTexGeniv returned %#x\n", status );
}

void WINAPI glTexImage1D( GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels )
{
    struct glTexImage1D_params args = { .target = target, .level = level, .internalformat = internalformat, .width = width, .border = border, .format = format, .type = type, .pixels = pixels, };
    NTSTATUS status;
    TRACE( "target %d, level %d, internalformat %d, width %d, border %d, format %d, type %d, pixels %p\n", target, level, internalformat, width, border, format, type, pixels );
    if ((status = UNIX_CALL( glTexImage1D, &args ))) WARN( "glTexImage1D returned %#x\n", status );
}

void WINAPI glTexImage2D( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels )
{
    struct glTexImage2D_params args = { .target = target, .level = level, .internalformat = internalformat, .width = width, .height = height, .border = border, .format = format, .type = type, .pixels = pixels, };
    NTSTATUS status;
    TRACE( "target %d, level %d, internalformat %d, width %d, height %d, border %d, format %d, type %d, pixels %p\n", target, level, internalformat, width, height, border, format, type, pixels );
    if ((status = UNIX_CALL( glTexImage2D, &args ))) WARN( "glTexImage2D returned %#x\n", status );
}

void WINAPI glTexParameterf( GLenum target, GLenum pname, GLfloat param )
{
    struct glTexParameterf_params args = { .target = target, .pname = pname, .param = param, };
    NTSTATUS status;
    TRACE( "target %d, pname %d, param %f\n", target, pname, param );
    if ((status = UNIX_CALL( glTexParameterf, &args ))) WARN( "glTexParameterf returned %#x\n", status );
}

void WINAPI glTexParameterfv( GLenum target, GLenum pname, const GLfloat *params )
{
    struct glTexParameterfv_params args = { .target = target, .pname = pname, .params = params, };
    NTSTATUS status;
    TRACE( "target %d, pname %d, params %p\n", target, pname, params );
    if ((status = UNIX_CALL( glTexParameterfv, &args ))) WARN( "glTexParameterfv returned %#x\n", status );
}

void WINAPI glTexParameteri( GLenum target, GLenum pname, GLint param )
{
    struct glTexParameteri_params args = { .target = target, .pname = pname, .param = param, };
    NTSTATUS status;
    TRACE( "target %d, pname %d, param %d\n", target, pname, param );
    if ((status = UNIX_CALL( glTexParameteri, &args ))) WARN( "glTexParameteri returned %#x\n", status );
}

void WINAPI glTexParameteriv( GLenum target, GLenum pname, const GLint *params )
{
    struct glTexParameteriv_params args = { .target = target, .pname = pname, .params = params, };
    NTSTATUS status;
    TRACE( "target %d, pname %d, params %p\n", target, pname, params );
    if ((status = UNIX_CALL( glTexParameteriv, &args ))) WARN( "glTexParameteriv returned %#x\n", status );
}

void WINAPI glTexSubImage1D( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels )
{
    struct glTexSubImage1D_params args = { .target = target, .level = level, .xoffset = xoffset, .width = width, .format = format, .type = type, .pixels = pixels, };
    NTSTATUS status;
    TRACE( "target %d, level %d, xoffset %d, width %d, format %d, type %d, pixels %p\n", target, level, xoffset, width, format, type, pixels );
    if ((status = UNIX_CALL( glTexSubImage1D, &args ))) WARN( "glTexSubImage1D returned %#x\n", status );
}

void WINAPI glTexSubImage2D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels )
{
    struct glTexSubImage2D_params args = { .target = target, .level = level, .xoffset = xoffset, .yoffset = yoffset, .width = width, .height = height, .format = format, .type = type, .pixels = pixels, };
    NTSTATUS status;
    TRACE( "target %d, level %d, xoffset %d, yoffset %d, width %d, height %d, format %d, type %d, pixels %p\n", target, level, xoffset, yoffset, width, height, format, type, pixels );
    if ((status = UNIX_CALL( glTexSubImage2D, &args ))) WARN( "glTexSubImage2D returned %#x\n", status );
}

void WINAPI glTranslated( GLdouble x, GLdouble y, GLdouble z )
{
    struct glTranslated_params args = { .x = x, .y = y, .z = z, };
    NTSTATUS status;
    TRACE( "x %f, y %f, z %f\n", x, y, z );
    if ((status = UNIX_CALL( glTranslated, &args ))) WARN( "glTranslated returned %#x\n", status );
}

void WINAPI glTranslatef( GLfloat x, GLfloat y, GLfloat z )
{
    struct glTranslatef_params args = { .x = x, .y = y, .z = z, };
    NTSTATUS status;
    TRACE( "x %f, y %f, z %f\n", x, y, z );
    if ((status = UNIX_CALL( glTranslatef, &args ))) WARN( "glTranslatef returned %#x\n", status );
}

void WINAPI glVertex2d( GLdouble x, GLdouble y )
{
    struct glVertex2d_params args = { .x = x, .y = y, };
    NTSTATUS status;
    TRACE( "x %f, y %f\n", x, y );
    if ((status = UNIX_CALL( glVertex2d, &args ))) WARN( "glVertex2d returned %#x\n", status );
}

void WINAPI glVertex2dv( const GLdouble *v )
{
    struct glVertex2dv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glVertex2dv, &args ))) WARN( "glVertex2dv returned %#x\n", status );
}

void WINAPI glVertex2f( GLfloat x, GLfloat y )
{
    struct glVertex2f_params args = { .x = x, .y = y, };
    NTSTATUS status;
    TRACE( "x %f, y %f\n", x, y );
    if ((status = UNIX_CALL( glVertex2f, &args ))) WARN( "glVertex2f returned %#x\n", status );
}

void WINAPI glVertex2fv( const GLfloat *v )
{
    struct glVertex2fv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glVertex2fv, &args ))) WARN( "glVertex2fv returned %#x\n", status );
}

void WINAPI glVertex2i( GLint x, GLint y )
{
    struct glVertex2i_params args = { .x = x, .y = y, };
    NTSTATUS status;
    TRACE( "x %d, y %d\n", x, y );
    if ((status = UNIX_CALL( glVertex2i, &args ))) WARN( "glVertex2i returned %#x\n", status );
}

void WINAPI glVertex2iv( const GLint *v )
{
    struct glVertex2iv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glVertex2iv, &args ))) WARN( "glVertex2iv returned %#x\n", status );
}

void WINAPI glVertex2s( GLshort x, GLshort y )
{
    struct glVertex2s_params args = { .x = x, .y = y, };
    NTSTATUS status;
    TRACE( "x %d, y %d\n", x, y );
    if ((status = UNIX_CALL( glVertex2s, &args ))) WARN( "glVertex2s returned %#x\n", status );
}

void WINAPI glVertex2sv( const GLshort *v )
{
    struct glVertex2sv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glVertex2sv, &args ))) WARN( "glVertex2sv returned %#x\n", status );
}

void WINAPI glVertex3d( GLdouble x, GLdouble y, GLdouble z )
{
    struct glVertex3d_params args = { .x = x, .y = y, .z = z, };
    NTSTATUS status;
    TRACE( "x %f, y %f, z %f\n", x, y, z );
    if ((status = UNIX_CALL( glVertex3d, &args ))) WARN( "glVertex3d returned %#x\n", status );
}

void WINAPI glVertex3dv( const GLdouble *v )
{
    struct glVertex3dv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glVertex3dv, &args ))) WARN( "glVertex3dv returned %#x\n", status );
}

void WINAPI glVertex3f( GLfloat x, GLfloat y, GLfloat z )
{
    struct glVertex3f_params args = { .x = x, .y = y, .z = z, };
    NTSTATUS status;
    TRACE( "x %f, y %f, z %f\n", x, y, z );
    if ((status = UNIX_CALL( glVertex3f, &args ))) WARN( "glVertex3f returned %#x\n", status );
}

void WINAPI glVertex3fv( const GLfloat *v )
{
    struct glVertex3fv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glVertex3fv, &args ))) WARN( "glVertex3fv returned %#x\n", status );
}

void WINAPI glVertex3i( GLint x, GLint y, GLint z )
{
    struct glVertex3i_params args = { .x = x, .y = y, .z = z, };
    NTSTATUS status;
    TRACE( "x %d, y %d, z %d\n", x, y, z );
    if ((status = UNIX_CALL( glVertex3i, &args ))) WARN( "glVertex3i returned %#x\n", status );
}

void WINAPI glVertex3iv( const GLint *v )
{
    struct glVertex3iv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glVertex3iv, &args ))) WARN( "glVertex3iv returned %#x\n", status );
}

void WINAPI glVertex3s( GLshort x, GLshort y, GLshort z )
{
    struct glVertex3s_params args = { .x = x, .y = y, .z = z, };
    NTSTATUS status;
    TRACE( "x %d, y %d, z %d\n", x, y, z );
    if ((status = UNIX_CALL( glVertex3s, &args ))) WARN( "glVertex3s returned %#x\n", status );
}

void WINAPI glVertex3sv( const GLshort *v )
{
    struct glVertex3sv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glVertex3sv, &args ))) WARN( "glVertex3sv returned %#x\n", status );
}

void WINAPI glVertex4d( GLdouble x, GLdouble y, GLdouble z, GLdouble w )
{
    struct glVertex4d_params args = { .x = x, .y = y, .z = z, .w = w, };
    NTSTATUS status;
    TRACE( "x %f, y %f, z %f, w %f\n", x, y, z, w );
    if ((status = UNIX_CALL( glVertex4d, &args ))) WARN( "glVertex4d returned %#x\n", status );
}

void WINAPI glVertex4dv( const GLdouble *v )
{
    struct glVertex4dv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glVertex4dv, &args ))) WARN( "glVertex4dv returned %#x\n", status );
}

void WINAPI glVertex4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
    struct glVertex4f_params args = { .x = x, .y = y, .z = z, .w = w, };
    NTSTATUS status;
    TRACE( "x %f, y %f, z %f, w %f\n", x, y, z, w );
    if ((status = UNIX_CALL( glVertex4f, &args ))) WARN( "glVertex4f returned %#x\n", status );
}

void WINAPI glVertex4fv( const GLfloat *v )
{
    struct glVertex4fv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glVertex4fv, &args ))) WARN( "glVertex4fv returned %#x\n", status );
}

void WINAPI glVertex4i( GLint x, GLint y, GLint z, GLint w )
{
    struct glVertex4i_params args = { .x = x, .y = y, .z = z, .w = w, };
    NTSTATUS status;
    TRACE( "x %d, y %d, z %d, w %d\n", x, y, z, w );
    if ((status = UNIX_CALL( glVertex4i, &args ))) WARN( "glVertex4i returned %#x\n", status );
}

void WINAPI glVertex4iv( const GLint *v )
{
    struct glVertex4iv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glVertex4iv, &args ))) WARN( "glVertex4iv returned %#x\n", status );
}

void WINAPI glVertex4s( GLshort x, GLshort y, GLshort z, GLshort w )
{
    struct glVertex4s_params args = { .x = x, .y = y, .z = z, .w = w, };
    NTSTATUS status;
    TRACE( "x %d, y %d, z %d, w %d\n", x, y, z, w );
    if ((status = UNIX_CALL( glVertex4s, &args ))) WARN( "glVertex4s returned %#x\n", status );
}

void WINAPI glVertex4sv( const GLshort *v )
{
    struct glVertex4sv_params args = { .v = v, };
    NTSTATUS status;
    TRACE( "v %p\n", v );
    if ((status = UNIX_CALL( glVertex4sv, &args ))) WARN( "glVertex4sv returned %#x\n", status );
}

void WINAPI glVertexPointer( GLint size, GLenum type, GLsizei stride, const void *pointer )
{
    struct glVertexPointer_params args = { .size = size, .type = type, .stride = stride, .pointer = pointer, };
    NTSTATUS status;
    TRACE( "size %d, type %d, stride %d, pointer %p\n", size, type, stride, pointer );
    if ((status = UNIX_CALL( glVertexPointer, &args ))) WARN( "glVertexPointer returned %#x\n", status );
}

void WINAPI glViewport( GLint x, GLint y, GLsizei width, GLsizei height )
{
    struct glViewport_params args = { .x = x, .y = y, .width = width, .height = height, };
    NTSTATUS status;
    TRACE( "x %d, y %d, width %d, height %d\n", x, y, width, height );
    if ((status = UNIX_CALL( glViewport, &args ))) WARN( "glViewport returned %#x\n", status );
}
