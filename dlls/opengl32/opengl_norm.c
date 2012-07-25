
/* Auto-generated file... Do not edit ! */

#include "config.h"
#include <stdarg.h>
#include "opengl_ext.h"
#include "winternl.h"
#include "wine/wgl_driver.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(opengl);

/***********************************************************************
 *              glAccum (OPENGL32.@)
 */
void WINAPI wine_glAccum( GLenum op, GLfloat value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f)\n", op, value );
  funcs->gl.p_glAccum( op, value );
}

/***********************************************************************
 *              glAlphaFunc (OPENGL32.@)
 */
void WINAPI wine_glAlphaFunc( GLenum func, GLfloat ref ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f)\n", func, ref );
  funcs->gl.p_glAlphaFunc( func, ref );
}

/***********************************************************************
 *              glAreTexturesResident (OPENGL32.@)
 */
GLboolean WINAPI wine_glAreTexturesResident( GLsizei n, const GLuint* textures, GLboolean* residences ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p, %p)\n", n, textures, residences );
  return funcs->gl.p_glAreTexturesResident( n, textures, residences );
}

/***********************************************************************
 *              glArrayElement (OPENGL32.@)
 */
void WINAPI wine_glArrayElement( GLint i ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", i );
  funcs->gl.p_glArrayElement( i );
}

/***********************************************************************
 *              glBegin (OPENGL32.@)
 */
void WINAPI wine_glBegin( GLenum mode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", mode );
  funcs->gl.p_glBegin( mode );
}

/***********************************************************************
 *              glBindTexture (OPENGL32.@)
 */
void WINAPI wine_glBindTexture( GLenum target, GLuint texture ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", target, texture );
  funcs->gl.p_glBindTexture( target, texture );
}

/***********************************************************************
 *              glBitmap (OPENGL32.@)
 */
void WINAPI wine_glBitmap( GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte* bitmap ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f, %f, %f, %f, %p)\n", width, height, xorig, yorig, xmove, ymove, bitmap );
  funcs->gl.p_glBitmap( width, height, xorig, yorig, xmove, ymove, bitmap );
}

/***********************************************************************
 *              glBlendFunc (OPENGL32.@)
 */
void WINAPI wine_glBlendFunc( GLenum sfactor, GLenum dfactor ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", sfactor, dfactor );
  funcs->gl.p_glBlendFunc( sfactor, dfactor );
}

/***********************************************************************
 *              glCallList (OPENGL32.@)
 */
void WINAPI wine_glCallList( GLuint list ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", list );
  funcs->gl.p_glCallList( list );
}

/***********************************************************************
 *              glCallLists (OPENGL32.@)
 */
void WINAPI wine_glCallLists( GLsizei n, GLenum type, const GLvoid* lists ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", n, type, lists );
  funcs->gl.p_glCallLists( n, type, lists );
}

/***********************************************************************
 *              glClear (OPENGL32.@)
 */
void WINAPI wine_glClear( GLbitfield mask ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", mask );
  funcs->gl.p_glClear( mask );
}

/***********************************************************************
 *              glClearAccum (OPENGL32.@)
 */
void WINAPI wine_glClearAccum( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f, %f)\n", red, green, blue, alpha );
  funcs->gl.p_glClearAccum( red, green, blue, alpha );
}

/***********************************************************************
 *              glClearColor (OPENGL32.@)
 */
void WINAPI wine_glClearColor( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f, %f)\n", red, green, blue, alpha );
  funcs->gl.p_glClearColor( red, green, blue, alpha );
}

/***********************************************************************
 *              glClearDepth (OPENGL32.@)
 */
void WINAPI wine_glClearDepth( GLdouble depth ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f)\n", depth );
  funcs->gl.p_glClearDepth( depth );
}

/***********************************************************************
 *              glClearIndex (OPENGL32.@)
 */
void WINAPI wine_glClearIndex( GLfloat c ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f)\n", c );
  funcs->gl.p_glClearIndex( c );
}

/***********************************************************************
 *              glClearStencil (OPENGL32.@)
 */
void WINAPI wine_glClearStencil( GLint s ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", s );
  funcs->gl.p_glClearStencil( s );
}

/***********************************************************************
 *              glClipPlane (OPENGL32.@)
 */
void WINAPI wine_glClipPlane( GLenum plane, const GLdouble* equation ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", plane, equation );
  funcs->gl.p_glClipPlane( plane, equation );
}

/***********************************************************************
 *              glColor3b (OPENGL32.@)
 */
void WINAPI wine_glColor3b( GLbyte red, GLbyte green, GLbyte blue ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", red, green, blue );
  funcs->gl.p_glColor3b( red, green, blue );
}

/***********************************************************************
 *              glColor3bv (OPENGL32.@)
 */
void WINAPI wine_glColor3bv( const GLbyte* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glColor3bv( v );
}

/***********************************************************************
 *              glColor3d (OPENGL32.@)
 */
void WINAPI wine_glColor3d( GLdouble red, GLdouble green, GLdouble blue ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f)\n", red, green, blue );
  funcs->gl.p_glColor3d( red, green, blue );
}

/***********************************************************************
 *              glColor3dv (OPENGL32.@)
 */
void WINAPI wine_glColor3dv( const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glColor3dv( v );
}

/***********************************************************************
 *              glColor3f (OPENGL32.@)
 */
void WINAPI wine_glColor3f( GLfloat red, GLfloat green, GLfloat blue ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f)\n", red, green, blue );
  funcs->gl.p_glColor3f( red, green, blue );
}

/***********************************************************************
 *              glColor3fv (OPENGL32.@)
 */
void WINAPI wine_glColor3fv( const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glColor3fv( v );
}

/***********************************************************************
 *              glColor3i (OPENGL32.@)
 */
void WINAPI wine_glColor3i( GLint red, GLint green, GLint blue ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", red, green, blue );
  funcs->gl.p_glColor3i( red, green, blue );
}

/***********************************************************************
 *              glColor3iv (OPENGL32.@)
 */
void WINAPI wine_glColor3iv( const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glColor3iv( v );
}

/***********************************************************************
 *              glColor3s (OPENGL32.@)
 */
void WINAPI wine_glColor3s( GLshort red, GLshort green, GLshort blue ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", red, green, blue );
  funcs->gl.p_glColor3s( red, green, blue );
}

/***********************************************************************
 *              glColor3sv (OPENGL32.@)
 */
void WINAPI wine_glColor3sv( const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glColor3sv( v );
}

/***********************************************************************
 *              glColor3ub (OPENGL32.@)
 */
void WINAPI wine_glColor3ub( GLubyte red, GLubyte green, GLubyte blue ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", red, green, blue );
  funcs->gl.p_glColor3ub( red, green, blue );
}

/***********************************************************************
 *              glColor3ubv (OPENGL32.@)
 */
void WINAPI wine_glColor3ubv( const GLubyte* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glColor3ubv( v );
}

/***********************************************************************
 *              glColor3ui (OPENGL32.@)
 */
void WINAPI wine_glColor3ui( GLuint red, GLuint green, GLuint blue ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", red, green, blue );
  funcs->gl.p_glColor3ui( red, green, blue );
}

/***********************************************************************
 *              glColor3uiv (OPENGL32.@)
 */
void WINAPI wine_glColor3uiv( const GLuint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glColor3uiv( v );
}

/***********************************************************************
 *              glColor3us (OPENGL32.@)
 */
void WINAPI wine_glColor3us( GLushort red, GLushort green, GLushort blue ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", red, green, blue );
  funcs->gl.p_glColor3us( red, green, blue );
}

/***********************************************************************
 *              glColor3usv (OPENGL32.@)
 */
void WINAPI wine_glColor3usv( const GLushort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glColor3usv( v );
}

/***********************************************************************
 *              glColor4b (OPENGL32.@)
 */
void WINAPI wine_glColor4b( GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", red, green, blue, alpha );
  funcs->gl.p_glColor4b( red, green, blue, alpha );
}

/***********************************************************************
 *              glColor4bv (OPENGL32.@)
 */
void WINAPI wine_glColor4bv( const GLbyte* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glColor4bv( v );
}

/***********************************************************************
 *              glColor4d (OPENGL32.@)
 */
void WINAPI wine_glColor4d( GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f, %f)\n", red, green, blue, alpha );
  funcs->gl.p_glColor4d( red, green, blue, alpha );
}

/***********************************************************************
 *              glColor4dv (OPENGL32.@)
 */
void WINAPI wine_glColor4dv( const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glColor4dv( v );
}

/***********************************************************************
 *              glColor4f (OPENGL32.@)
 */
void WINAPI wine_glColor4f( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f, %f)\n", red, green, blue, alpha );
  funcs->gl.p_glColor4f( red, green, blue, alpha );
}

/***********************************************************************
 *              glColor4fv (OPENGL32.@)
 */
void WINAPI wine_glColor4fv( const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glColor4fv( v );
}

/***********************************************************************
 *              glColor4i (OPENGL32.@)
 */
void WINAPI wine_glColor4i( GLint red, GLint green, GLint blue, GLint alpha ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", red, green, blue, alpha );
  funcs->gl.p_glColor4i( red, green, blue, alpha );
}

/***********************************************************************
 *              glColor4iv (OPENGL32.@)
 */
void WINAPI wine_glColor4iv( const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glColor4iv( v );
}

/***********************************************************************
 *              glColor4s (OPENGL32.@)
 */
void WINAPI wine_glColor4s( GLshort red, GLshort green, GLshort blue, GLshort alpha ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", red, green, blue, alpha );
  funcs->gl.p_glColor4s( red, green, blue, alpha );
}

/***********************************************************************
 *              glColor4sv (OPENGL32.@)
 */
void WINAPI wine_glColor4sv( const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glColor4sv( v );
}

/***********************************************************************
 *              glColor4ub (OPENGL32.@)
 */
void WINAPI wine_glColor4ub( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", red, green, blue, alpha );
  funcs->gl.p_glColor4ub( red, green, blue, alpha );
}

/***********************************************************************
 *              glColor4ubv (OPENGL32.@)
 */
void WINAPI wine_glColor4ubv( const GLubyte* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glColor4ubv( v );
}

/***********************************************************************
 *              glColor4ui (OPENGL32.@)
 */
void WINAPI wine_glColor4ui( GLuint red, GLuint green, GLuint blue, GLuint alpha ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", red, green, blue, alpha );
  funcs->gl.p_glColor4ui( red, green, blue, alpha );
}

/***********************************************************************
 *              glColor4uiv (OPENGL32.@)
 */
void WINAPI wine_glColor4uiv( const GLuint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glColor4uiv( v );
}

/***********************************************************************
 *              glColor4us (OPENGL32.@)
 */
void WINAPI wine_glColor4us( GLushort red, GLushort green, GLushort blue, GLushort alpha ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", red, green, blue, alpha );
  funcs->gl.p_glColor4us( red, green, blue, alpha );
}

/***********************************************************************
 *              glColor4usv (OPENGL32.@)
 */
void WINAPI wine_glColor4usv( const GLushort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glColor4usv( v );
}

/***********************************************************************
 *              glColorMask (OPENGL32.@)
 */
void WINAPI wine_glColorMask( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", red, green, blue, alpha );
  funcs->gl.p_glColorMask( red, green, blue, alpha );
}

/***********************************************************************
 *              glColorMaterial (OPENGL32.@)
 */
void WINAPI wine_glColorMaterial( GLenum face, GLenum mode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", face, mode );
  funcs->gl.p_glColorMaterial( face, mode );
}

/***********************************************************************
 *              glColorPointer (OPENGL32.@)
 */
void WINAPI wine_glColorPointer( GLint size, GLenum type, GLsizei stride, const GLvoid* pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", size, type, stride, pointer );
  funcs->gl.p_glColorPointer( size, type, stride, pointer );
}

/***********************************************************************
 *              glCopyPixels (OPENGL32.@)
 */
void WINAPI wine_glCopyPixels( GLint x, GLint y, GLsizei width, GLsizei height, GLenum type ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", x, y, width, height, type );
  funcs->gl.p_glCopyPixels( x, y, width, height, type );
}

/***********************************************************************
 *              glCopyTexImage1D (OPENGL32.@)
 */
void WINAPI wine_glCopyTexImage1D( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d)\n", target, level, internalformat, x, y, width, border );
  funcs->gl.p_glCopyTexImage1D( target, level, internalformat, x, y, width, border );
}

/***********************************************************************
 *              glCopyTexImage2D (OPENGL32.@)
 */
void WINAPI wine_glCopyTexImage2D( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d)\n", target, level, internalformat, x, y, width, height, border );
  funcs->gl.p_glCopyTexImage2D( target, level, internalformat, x, y, width, height, border );
}

/***********************************************************************
 *              glCopyTexSubImage1D (OPENGL32.@)
 */
void WINAPI wine_glCopyTexSubImage1D( GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d)\n", target, level, xoffset, x, y, width );
  funcs->gl.p_glCopyTexSubImage1D( target, level, xoffset, x, y, width );
}

/***********************************************************************
 *              glCopyTexSubImage2D (OPENGL32.@)
 */
void WINAPI wine_glCopyTexSubImage2D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d)\n", target, level, xoffset, yoffset, x, y, width, height );
  funcs->gl.p_glCopyTexSubImage2D( target, level, xoffset, yoffset, x, y, width, height );
}

/***********************************************************************
 *              glCullFace (OPENGL32.@)
 */
void WINAPI wine_glCullFace( GLenum mode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", mode );
  funcs->gl.p_glCullFace( mode );
}

/***********************************************************************
 *              glDeleteLists (OPENGL32.@)
 */
void WINAPI wine_glDeleteLists( GLuint list, GLsizei range ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", list, range );
  funcs->gl.p_glDeleteLists( list, range );
}

/***********************************************************************
 *              glDeleteTextures (OPENGL32.@)
 */
void WINAPI wine_glDeleteTextures( GLsizei n, const GLuint* textures ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, textures );
  funcs->gl.p_glDeleteTextures( n, textures );
}

/***********************************************************************
 *              glDepthFunc (OPENGL32.@)
 */
void WINAPI wine_glDepthFunc( GLenum func ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", func );
  funcs->gl.p_glDepthFunc( func );
}

/***********************************************************************
 *              glDepthMask (OPENGL32.@)
 */
void WINAPI wine_glDepthMask( GLboolean flag ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", flag );
  funcs->gl.p_glDepthMask( flag );
}

/***********************************************************************
 *              glDepthRange (OPENGL32.@)
 */
void WINAPI wine_glDepthRange( GLdouble nearParam, GLdouble farParam ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f)\n", nearParam, farParam );
  funcs->gl.p_glDepthRange( nearParam, farParam );
}

/***********************************************************************
 *              glDisable (OPENGL32.@)
 */
void WINAPI wine_glDisable( GLenum cap ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", cap );
  funcs->gl.p_glDisable( cap );
}

/***********************************************************************
 *              glDisableClientState (OPENGL32.@)
 */
void WINAPI wine_glDisableClientState( GLenum array ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", array );
  funcs->gl.p_glDisableClientState( array );
}

/***********************************************************************
 *              glDrawArrays (OPENGL32.@)
 */
void WINAPI wine_glDrawArrays( GLenum mode, GLint first, GLsizei count ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", mode, first, count );
  funcs->gl.p_glDrawArrays( mode, first, count );
}

/***********************************************************************
 *              glDrawBuffer (OPENGL32.@)
 */
void WINAPI wine_glDrawBuffer( GLenum mode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", mode );
  funcs->gl.p_glDrawBuffer( mode );
}

/***********************************************************************
 *              glDrawElements (OPENGL32.@)
 */
void WINAPI wine_glDrawElements( GLenum mode, GLsizei count, GLenum type, const GLvoid* indices ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", mode, count, type, indices );
  funcs->gl.p_glDrawElements( mode, count, type, indices );
}

/***********************************************************************
 *              glDrawPixels (OPENGL32.@)
 */
void WINAPI wine_glDrawPixels( GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", width, height, format, type, pixels );
  funcs->gl.p_glDrawPixels( width, height, format, type, pixels );
}

/***********************************************************************
 *              glEdgeFlag (OPENGL32.@)
 */
void WINAPI wine_glEdgeFlag( GLboolean flag ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", flag );
  funcs->gl.p_glEdgeFlag( flag );
}

/***********************************************************************
 *              glEdgeFlagPointer (OPENGL32.@)
 */
void WINAPI wine_glEdgeFlagPointer( GLsizei stride, const GLvoid* pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", stride, pointer );
  funcs->gl.p_glEdgeFlagPointer( stride, pointer );
}

/***********************************************************************
 *              glEdgeFlagv (OPENGL32.@)
 */
void WINAPI wine_glEdgeFlagv( const GLboolean* flag ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", flag );
  funcs->gl.p_glEdgeFlagv( flag );
}

/***********************************************************************
 *              glEnable (OPENGL32.@)
 */
void WINAPI wine_glEnable( GLenum cap ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", cap );
  funcs->gl.p_glEnable( cap );
}

/***********************************************************************
 *              glEnableClientState (OPENGL32.@)
 */
void WINAPI wine_glEnableClientState( GLenum array ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", array );
  funcs->gl.p_glEnableClientState( array );
}

/***********************************************************************
 *              glEnd (OPENGL32.@)
 */
void WINAPI wine_glEnd( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  funcs->gl.p_glEnd( );
}

/***********************************************************************
 *              glEndList (OPENGL32.@)
 */
void WINAPI wine_glEndList( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  funcs->gl.p_glEndList( );
}

/***********************************************************************
 *              glEvalCoord1d (OPENGL32.@)
 */
void WINAPI wine_glEvalCoord1d( GLdouble u ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f)\n", u );
  funcs->gl.p_glEvalCoord1d( u );
}

/***********************************************************************
 *              glEvalCoord1dv (OPENGL32.@)
 */
void WINAPI wine_glEvalCoord1dv( const GLdouble* u ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", u );
  funcs->gl.p_glEvalCoord1dv( u );
}

/***********************************************************************
 *              glEvalCoord1f (OPENGL32.@)
 */
void WINAPI wine_glEvalCoord1f( GLfloat u ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f)\n", u );
  funcs->gl.p_glEvalCoord1f( u );
}

/***********************************************************************
 *              glEvalCoord1fv (OPENGL32.@)
 */
void WINAPI wine_glEvalCoord1fv( const GLfloat* u ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", u );
  funcs->gl.p_glEvalCoord1fv( u );
}

/***********************************************************************
 *              glEvalCoord2d (OPENGL32.@)
 */
void WINAPI wine_glEvalCoord2d( GLdouble u, GLdouble v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f)\n", u, v );
  funcs->gl.p_glEvalCoord2d( u, v );
}

/***********************************************************************
 *              glEvalCoord2dv (OPENGL32.@)
 */
void WINAPI wine_glEvalCoord2dv( const GLdouble* u ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", u );
  funcs->gl.p_glEvalCoord2dv( u );
}

/***********************************************************************
 *              glEvalCoord2f (OPENGL32.@)
 */
void WINAPI wine_glEvalCoord2f( GLfloat u, GLfloat v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f)\n", u, v );
  funcs->gl.p_glEvalCoord2f( u, v );
}

/***********************************************************************
 *              glEvalCoord2fv (OPENGL32.@)
 */
void WINAPI wine_glEvalCoord2fv( const GLfloat* u ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", u );
  funcs->gl.p_glEvalCoord2fv( u );
}

/***********************************************************************
 *              glEvalMesh1 (OPENGL32.@)
 */
void WINAPI wine_glEvalMesh1( GLenum mode, GLint i1, GLint i2 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", mode, i1, i2 );
  funcs->gl.p_glEvalMesh1( mode, i1, i2 );
}

/***********************************************************************
 *              glEvalMesh2 (OPENGL32.@)
 */
void WINAPI wine_glEvalMesh2( GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", mode, i1, i2, j1, j2 );
  funcs->gl.p_glEvalMesh2( mode, i1, i2, j1, j2 );
}

/***********************************************************************
 *              glEvalPoint1 (OPENGL32.@)
 */
void WINAPI wine_glEvalPoint1( GLint i ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", i );
  funcs->gl.p_glEvalPoint1( i );
}

/***********************************************************************
 *              glEvalPoint2 (OPENGL32.@)
 */
void WINAPI wine_glEvalPoint2( GLint i, GLint j ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", i, j );
  funcs->gl.p_glEvalPoint2( i, j );
}

/***********************************************************************
 *              glFeedbackBuffer (OPENGL32.@)
 */
void WINAPI wine_glFeedbackBuffer( GLsizei size, GLenum type, GLfloat* buffer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", size, type, buffer );
  funcs->gl.p_glFeedbackBuffer( size, type, buffer );
}

/***********************************************************************
 *              glFinish (OPENGL32.@)
 */
void WINAPI wine_glFinish( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  funcs->gl.p_glFinish( );
}

/***********************************************************************
 *              glFlush (OPENGL32.@)
 */
void WINAPI wine_glFlush( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  funcs->gl.p_glFlush( );
}

/***********************************************************************
 *              glFogf (OPENGL32.@)
 */
void WINAPI wine_glFogf( GLenum pname, GLfloat param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f)\n", pname, param );
  funcs->gl.p_glFogf( pname, param );
}

/***********************************************************************
 *              glFogfv (OPENGL32.@)
 */
void WINAPI wine_glFogfv( GLenum pname, const GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", pname, params );
  funcs->gl.p_glFogfv( pname, params );
}

/***********************************************************************
 *              glFogi (OPENGL32.@)
 */
void WINAPI wine_glFogi( GLenum pname, GLint param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", pname, param );
  funcs->gl.p_glFogi( pname, param );
}

/***********************************************************************
 *              glFogiv (OPENGL32.@)
 */
void WINAPI wine_glFogiv( GLenum pname, const GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", pname, params );
  funcs->gl.p_glFogiv( pname, params );
}

/***********************************************************************
 *              glFrontFace (OPENGL32.@)
 */
void WINAPI wine_glFrontFace( GLenum mode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", mode );
  funcs->gl.p_glFrontFace( mode );
}

/***********************************************************************
 *              glFrustum (OPENGL32.@)
 */
void WINAPI wine_glFrustum( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f, %f, %f, %f)\n", left, right, bottom, top, zNear, zFar );
  funcs->gl.p_glFrustum( left, right, bottom, top, zNear, zFar );
}

/***********************************************************************
 *              glGenLists (OPENGL32.@)
 */
GLuint WINAPI wine_glGenLists( GLsizei range ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", range );
  return funcs->gl.p_glGenLists( range );
}

/***********************************************************************
 *              glGenTextures (OPENGL32.@)
 */
void WINAPI wine_glGenTextures( GLsizei n, GLuint* textures ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, textures );
  funcs->gl.p_glGenTextures( n, textures );
}

/***********************************************************************
 *              glGetBooleanv (OPENGL32.@)
 */
void WINAPI wine_glGetBooleanv( GLenum pname, GLboolean* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", pname, params );
  funcs->gl.p_glGetBooleanv( pname, params );
}

/***********************************************************************
 *              glGetClipPlane (OPENGL32.@)
 */
void WINAPI wine_glGetClipPlane( GLenum plane, GLdouble* equation ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", plane, equation );
  funcs->gl.p_glGetClipPlane( plane, equation );
}

/***********************************************************************
 *              glGetDoublev (OPENGL32.@)
 */
void WINAPI wine_glGetDoublev( GLenum pname, GLdouble* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", pname, params );
  funcs->gl.p_glGetDoublev( pname, params );
}

/***********************************************************************
 *              glGetError (OPENGL32.@)
 */
GLenum WINAPI wine_glGetError( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  return funcs->gl.p_glGetError( );
}

/***********************************************************************
 *              glGetFloatv (OPENGL32.@)
 */
void WINAPI wine_glGetFloatv( GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", pname, params );
  funcs->gl.p_glGetFloatv( pname, params );
}

/***********************************************************************
 *              glGetIntegerv (OPENGL32.@)
 */
void WINAPI wine_glGetIntegerv( GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", pname, params );
  funcs->gl.p_glGetIntegerv( pname, params );
}

/***********************************************************************
 *              glGetLightfv (OPENGL32.@)
 */
void WINAPI wine_glGetLightfv( GLenum light, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", light, pname, params );
  funcs->gl.p_glGetLightfv( light, pname, params );
}

/***********************************************************************
 *              glGetLightiv (OPENGL32.@)
 */
void WINAPI wine_glGetLightiv( GLenum light, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", light, pname, params );
  funcs->gl.p_glGetLightiv( light, pname, params );
}

/***********************************************************************
 *              glGetMapdv (OPENGL32.@)
 */
void WINAPI wine_glGetMapdv( GLenum target, GLenum query, GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, query, v );
  funcs->gl.p_glGetMapdv( target, query, v );
}

/***********************************************************************
 *              glGetMapfv (OPENGL32.@)
 */
void WINAPI wine_glGetMapfv( GLenum target, GLenum query, GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, query, v );
  funcs->gl.p_glGetMapfv( target, query, v );
}

/***********************************************************************
 *              glGetMapiv (OPENGL32.@)
 */
void WINAPI wine_glGetMapiv( GLenum target, GLenum query, GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, query, v );
  funcs->gl.p_glGetMapiv( target, query, v );
}

/***********************************************************************
 *              glGetMaterialfv (OPENGL32.@)
 */
void WINAPI wine_glGetMaterialfv( GLenum face, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", face, pname, params );
  funcs->gl.p_glGetMaterialfv( face, pname, params );
}

/***********************************************************************
 *              glGetMaterialiv (OPENGL32.@)
 */
void WINAPI wine_glGetMaterialiv( GLenum face, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", face, pname, params );
  funcs->gl.p_glGetMaterialiv( face, pname, params );
}

/***********************************************************************
 *              glGetPixelMapfv (OPENGL32.@)
 */
void WINAPI wine_glGetPixelMapfv( GLenum map, GLfloat* values ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", map, values );
  funcs->gl.p_glGetPixelMapfv( map, values );
}

/***********************************************************************
 *              glGetPixelMapuiv (OPENGL32.@)
 */
void WINAPI wine_glGetPixelMapuiv( GLenum map, GLuint* values ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", map, values );
  funcs->gl.p_glGetPixelMapuiv( map, values );
}

/***********************************************************************
 *              glGetPixelMapusv (OPENGL32.@)
 */
void WINAPI wine_glGetPixelMapusv( GLenum map, GLushort* values ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", map, values );
  funcs->gl.p_glGetPixelMapusv( map, values );
}

/***********************************************************************
 *              glGetPointerv (OPENGL32.@)
 */
void WINAPI wine_glGetPointerv( GLenum pname, GLvoid** params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", pname, params );
  funcs->gl.p_glGetPointerv( pname, params );
}

/***********************************************************************
 *              glGetPolygonStipple (OPENGL32.@)
 */
void WINAPI wine_glGetPolygonStipple( GLubyte* mask ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", mask );
  funcs->gl.p_glGetPolygonStipple( mask );
}

/***********************************************************************
 *              glGetTexEnvfv (OPENGL32.@)
 */
void WINAPI wine_glGetTexEnvfv( GLenum target, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->gl.p_glGetTexEnvfv( target, pname, params );
}

/***********************************************************************
 *              glGetTexEnviv (OPENGL32.@)
 */
void WINAPI wine_glGetTexEnviv( GLenum target, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->gl.p_glGetTexEnviv( target, pname, params );
}

/***********************************************************************
 *              glGetTexGendv (OPENGL32.@)
 */
void WINAPI wine_glGetTexGendv( GLenum coord, GLenum pname, GLdouble* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", coord, pname, params );
  funcs->gl.p_glGetTexGendv( coord, pname, params );
}

/***********************************************************************
 *              glGetTexGenfv (OPENGL32.@)
 */
void WINAPI wine_glGetTexGenfv( GLenum coord, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", coord, pname, params );
  funcs->gl.p_glGetTexGenfv( coord, pname, params );
}

/***********************************************************************
 *              glGetTexGeniv (OPENGL32.@)
 */
void WINAPI wine_glGetTexGeniv( GLenum coord, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", coord, pname, params );
  funcs->gl.p_glGetTexGeniv( coord, pname, params );
}

/***********************************************************************
 *              glGetTexImage (OPENGL32.@)
 */
void WINAPI wine_glGetTexImage( GLenum target, GLint level, GLenum format, GLenum type, GLvoid* pixels ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", target, level, format, type, pixels );
  funcs->gl.p_glGetTexImage( target, level, format, type, pixels );
}

/***********************************************************************
 *              glGetTexLevelParameterfv (OPENGL32.@)
 */
void WINAPI wine_glGetTexLevelParameterfv( GLenum target, GLint level, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", target, level, pname, params );
  funcs->gl.p_glGetTexLevelParameterfv( target, level, pname, params );
}

/***********************************************************************
 *              glGetTexLevelParameteriv (OPENGL32.@)
 */
void WINAPI wine_glGetTexLevelParameteriv( GLenum target, GLint level, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", target, level, pname, params );
  funcs->gl.p_glGetTexLevelParameteriv( target, level, pname, params );
}

/***********************************************************************
 *              glGetTexParameterfv (OPENGL32.@)
 */
void WINAPI wine_glGetTexParameterfv( GLenum target, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->gl.p_glGetTexParameterfv( target, pname, params );
}

/***********************************************************************
 *              glGetTexParameteriv (OPENGL32.@)
 */
void WINAPI wine_glGetTexParameteriv( GLenum target, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->gl.p_glGetTexParameteriv( target, pname, params );
}

/***********************************************************************
 *              glHint (OPENGL32.@)
 */
void WINAPI wine_glHint( GLenum target, GLenum mode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", target, mode );
  funcs->gl.p_glHint( target, mode );
}

/***********************************************************************
 *              glIndexMask (OPENGL32.@)
 */
void WINAPI wine_glIndexMask( GLuint mask ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", mask );
  funcs->gl.p_glIndexMask( mask );
}

/***********************************************************************
 *              glIndexPointer (OPENGL32.@)
 */
void WINAPI wine_glIndexPointer( GLenum type, GLsizei stride, const GLvoid* pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", type, stride, pointer );
  funcs->gl.p_glIndexPointer( type, stride, pointer );
}

/***********************************************************************
 *              glIndexd (OPENGL32.@)
 */
void WINAPI wine_glIndexd( GLdouble c ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f)\n", c );
  funcs->gl.p_glIndexd( c );
}

/***********************************************************************
 *              glIndexdv (OPENGL32.@)
 */
void WINAPI wine_glIndexdv( const GLdouble* c ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", c );
  funcs->gl.p_glIndexdv( c );
}

/***********************************************************************
 *              glIndexf (OPENGL32.@)
 */
void WINAPI wine_glIndexf( GLfloat c ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f)\n", c );
  funcs->gl.p_glIndexf( c );
}

/***********************************************************************
 *              glIndexfv (OPENGL32.@)
 */
void WINAPI wine_glIndexfv( const GLfloat* c ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", c );
  funcs->gl.p_glIndexfv( c );
}

/***********************************************************************
 *              glIndexi (OPENGL32.@)
 */
void WINAPI wine_glIndexi( GLint c ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", c );
  funcs->gl.p_glIndexi( c );
}

/***********************************************************************
 *              glIndexiv (OPENGL32.@)
 */
void WINAPI wine_glIndexiv( const GLint* c ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", c );
  funcs->gl.p_glIndexiv( c );
}

/***********************************************************************
 *              glIndexs (OPENGL32.@)
 */
void WINAPI wine_glIndexs( GLshort c ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", c );
  funcs->gl.p_glIndexs( c );
}

/***********************************************************************
 *              glIndexsv (OPENGL32.@)
 */
void WINAPI wine_glIndexsv( const GLshort* c ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", c );
  funcs->gl.p_glIndexsv( c );
}

/***********************************************************************
 *              glIndexub (OPENGL32.@)
 */
void WINAPI wine_glIndexub( GLubyte c ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", c );
  funcs->gl.p_glIndexub( c );
}

/***********************************************************************
 *              glIndexubv (OPENGL32.@)
 */
void WINAPI wine_glIndexubv( const GLubyte* c ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", c );
  funcs->gl.p_glIndexubv( c );
}

/***********************************************************************
 *              glInitNames (OPENGL32.@)
 */
void WINAPI wine_glInitNames( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  funcs->gl.p_glInitNames( );
}

/***********************************************************************
 *              glInterleavedArrays (OPENGL32.@)
 */
void WINAPI wine_glInterleavedArrays( GLenum format, GLsizei stride, const GLvoid* pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", format, stride, pointer );
  funcs->gl.p_glInterleavedArrays( format, stride, pointer );
}

/***********************************************************************
 *              glIsEnabled (OPENGL32.@)
 */
GLboolean WINAPI wine_glIsEnabled( GLenum cap ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", cap );
  return funcs->gl.p_glIsEnabled( cap );
}

/***********************************************************************
 *              glIsList (OPENGL32.@)
 */
GLboolean WINAPI wine_glIsList( GLuint list ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", list );
  return funcs->gl.p_glIsList( list );
}

/***********************************************************************
 *              glIsTexture (OPENGL32.@)
 */
GLboolean WINAPI wine_glIsTexture( GLuint texture ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", texture );
  return funcs->gl.p_glIsTexture( texture );
}

/***********************************************************************
 *              glLightModelf (OPENGL32.@)
 */
void WINAPI wine_glLightModelf( GLenum pname, GLfloat param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f)\n", pname, param );
  funcs->gl.p_glLightModelf( pname, param );
}

/***********************************************************************
 *              glLightModelfv (OPENGL32.@)
 */
void WINAPI wine_glLightModelfv( GLenum pname, const GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", pname, params );
  funcs->gl.p_glLightModelfv( pname, params );
}

/***********************************************************************
 *              glLightModeli (OPENGL32.@)
 */
void WINAPI wine_glLightModeli( GLenum pname, GLint param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", pname, param );
  funcs->gl.p_glLightModeli( pname, param );
}

/***********************************************************************
 *              glLightModeliv (OPENGL32.@)
 */
void WINAPI wine_glLightModeliv( GLenum pname, const GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", pname, params );
  funcs->gl.p_glLightModeliv( pname, params );
}

/***********************************************************************
 *              glLightf (OPENGL32.@)
 */
void WINAPI wine_glLightf( GLenum light, GLenum pname, GLfloat param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f)\n", light, pname, param );
  funcs->gl.p_glLightf( light, pname, param );
}

/***********************************************************************
 *              glLightfv (OPENGL32.@)
 */
void WINAPI wine_glLightfv( GLenum light, GLenum pname, const GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", light, pname, params );
  funcs->gl.p_glLightfv( light, pname, params );
}

/***********************************************************************
 *              glLighti (OPENGL32.@)
 */
void WINAPI wine_glLighti( GLenum light, GLenum pname, GLint param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", light, pname, param );
  funcs->gl.p_glLighti( light, pname, param );
}

/***********************************************************************
 *              glLightiv (OPENGL32.@)
 */
void WINAPI wine_glLightiv( GLenum light, GLenum pname, const GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", light, pname, params );
  funcs->gl.p_glLightiv( light, pname, params );
}

/***********************************************************************
 *              glLineStipple (OPENGL32.@)
 */
void WINAPI wine_glLineStipple( GLint factor, GLushort pattern ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", factor, pattern );
  funcs->gl.p_glLineStipple( factor, pattern );
}

/***********************************************************************
 *              glLineWidth (OPENGL32.@)
 */
void WINAPI wine_glLineWidth( GLfloat width ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f)\n", width );
  funcs->gl.p_glLineWidth( width );
}

/***********************************************************************
 *              glListBase (OPENGL32.@)
 */
void WINAPI wine_glListBase( GLuint base ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", base );
  funcs->gl.p_glListBase( base );
}

/***********************************************************************
 *              glLoadIdentity (OPENGL32.@)
 */
void WINAPI wine_glLoadIdentity( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  funcs->gl.p_glLoadIdentity( );
}

/***********************************************************************
 *              glLoadMatrixd (OPENGL32.@)
 */
void WINAPI wine_glLoadMatrixd( const GLdouble* m ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", m );
  funcs->gl.p_glLoadMatrixd( m );
}

/***********************************************************************
 *              glLoadMatrixf (OPENGL32.@)
 */
void WINAPI wine_glLoadMatrixf( const GLfloat* m ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", m );
  funcs->gl.p_glLoadMatrixf( m );
}

/***********************************************************************
 *              glLoadName (OPENGL32.@)
 */
void WINAPI wine_glLoadName( GLuint name ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", name );
  funcs->gl.p_glLoadName( name );
}

/***********************************************************************
 *              glLogicOp (OPENGL32.@)
 */
void WINAPI wine_glLogicOp( GLenum opcode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", opcode );
  funcs->gl.p_glLogicOp( opcode );
}

/***********************************************************************
 *              glMap1d (OPENGL32.@)
 */
void WINAPI wine_glMap1d( GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble* points ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %d, %d, %p)\n", target, u1, u2, stride, order, points );
  funcs->gl.p_glMap1d( target, u1, u2, stride, order, points );
}

/***********************************************************************
 *              glMap1f (OPENGL32.@)
 */
void WINAPI wine_glMap1f( GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat* points ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %d, %d, %p)\n", target, u1, u2, stride, order, points );
  funcs->gl.p_glMap1f( target, u1, u2, stride, order, points );
}

/***********************************************************************
 *              glMap2d (OPENGL32.@)
 */
void WINAPI wine_glMap2d( GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble* points ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %d, %d, %f, %f, %d, %d, %p)\n", target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points );
  funcs->gl.p_glMap2d( target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points );
}

/***********************************************************************
 *              glMap2f (OPENGL32.@)
 */
void WINAPI wine_glMap2f( GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat* points ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %d, %d, %f, %f, %d, %d, %p)\n", target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points );
  funcs->gl.p_glMap2f( target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points );
}

/***********************************************************************
 *              glMapGrid1d (OPENGL32.@)
 */
void WINAPI wine_glMapGrid1d( GLint un, GLdouble u1, GLdouble u2 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f)\n", un, u1, u2 );
  funcs->gl.p_glMapGrid1d( un, u1, u2 );
}

/***********************************************************************
 *              glMapGrid1f (OPENGL32.@)
 */
void WINAPI wine_glMapGrid1f( GLint un, GLfloat u1, GLfloat u2 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f)\n", un, u1, u2 );
  funcs->gl.p_glMapGrid1f( un, u1, u2 );
}

/***********************************************************************
 *              glMapGrid2d (OPENGL32.@)
 */
void WINAPI wine_glMapGrid2d( GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %d, %f, %f)\n", un, u1, u2, vn, v1, v2 );
  funcs->gl.p_glMapGrid2d( un, u1, u2, vn, v1, v2 );
}

/***********************************************************************
 *              glMapGrid2f (OPENGL32.@)
 */
void WINAPI wine_glMapGrid2f( GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %d, %f, %f)\n", un, u1, u2, vn, v1, v2 );
  funcs->gl.p_glMapGrid2f( un, u1, u2, vn, v1, v2 );
}

/***********************************************************************
 *              glMaterialf (OPENGL32.@)
 */
void WINAPI wine_glMaterialf( GLenum face, GLenum pname, GLfloat param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f)\n", face, pname, param );
  funcs->gl.p_glMaterialf( face, pname, param );
}

/***********************************************************************
 *              glMaterialfv (OPENGL32.@)
 */
void WINAPI wine_glMaterialfv( GLenum face, GLenum pname, const GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", face, pname, params );
  funcs->gl.p_glMaterialfv( face, pname, params );
}

/***********************************************************************
 *              glMateriali (OPENGL32.@)
 */
void WINAPI wine_glMateriali( GLenum face, GLenum pname, GLint param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", face, pname, param );
  funcs->gl.p_glMateriali( face, pname, param );
}

/***********************************************************************
 *              glMaterialiv (OPENGL32.@)
 */
void WINAPI wine_glMaterialiv( GLenum face, GLenum pname, const GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", face, pname, params );
  funcs->gl.p_glMaterialiv( face, pname, params );
}

/***********************************************************************
 *              glMatrixMode (OPENGL32.@)
 */
void WINAPI wine_glMatrixMode( GLenum mode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", mode );
  funcs->gl.p_glMatrixMode( mode );
}

/***********************************************************************
 *              glMultMatrixd (OPENGL32.@)
 */
void WINAPI wine_glMultMatrixd( const GLdouble* m ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", m );
  funcs->gl.p_glMultMatrixd( m );
}

/***********************************************************************
 *              glMultMatrixf (OPENGL32.@)
 */
void WINAPI wine_glMultMatrixf( const GLfloat* m ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", m );
  funcs->gl.p_glMultMatrixf( m );
}

/***********************************************************************
 *              glNewList (OPENGL32.@)
 */
void WINAPI wine_glNewList( GLuint list, GLenum mode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", list, mode );
  funcs->gl.p_glNewList( list, mode );
}

/***********************************************************************
 *              glNormal3b (OPENGL32.@)
 */
void WINAPI wine_glNormal3b( GLbyte nx, GLbyte ny, GLbyte nz ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", nx, ny, nz );
  funcs->gl.p_glNormal3b( nx, ny, nz );
}

/***********************************************************************
 *              glNormal3bv (OPENGL32.@)
 */
void WINAPI wine_glNormal3bv( const GLbyte* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glNormal3bv( v );
}

/***********************************************************************
 *              glNormal3d (OPENGL32.@)
 */
void WINAPI wine_glNormal3d( GLdouble nx, GLdouble ny, GLdouble nz ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f)\n", nx, ny, nz );
  funcs->gl.p_glNormal3d( nx, ny, nz );
}

/***********************************************************************
 *              glNormal3dv (OPENGL32.@)
 */
void WINAPI wine_glNormal3dv( const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glNormal3dv( v );
}

/***********************************************************************
 *              glNormal3f (OPENGL32.@)
 */
void WINAPI wine_glNormal3f( GLfloat nx, GLfloat ny, GLfloat nz ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f)\n", nx, ny, nz );
  funcs->gl.p_glNormal3f( nx, ny, nz );
}

/***********************************************************************
 *              glNormal3fv (OPENGL32.@)
 */
void WINAPI wine_glNormal3fv( const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glNormal3fv( v );
}

/***********************************************************************
 *              glNormal3i (OPENGL32.@)
 */
void WINAPI wine_glNormal3i( GLint nx, GLint ny, GLint nz ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", nx, ny, nz );
  funcs->gl.p_glNormal3i( nx, ny, nz );
}

/***********************************************************************
 *              glNormal3iv (OPENGL32.@)
 */
void WINAPI wine_glNormal3iv( const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glNormal3iv( v );
}

/***********************************************************************
 *              glNormal3s (OPENGL32.@)
 */
void WINAPI wine_glNormal3s( GLshort nx, GLshort ny, GLshort nz ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", nx, ny, nz );
  funcs->gl.p_glNormal3s( nx, ny, nz );
}

/***********************************************************************
 *              glNormal3sv (OPENGL32.@)
 */
void WINAPI wine_glNormal3sv( const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glNormal3sv( v );
}

/***********************************************************************
 *              glNormalPointer (OPENGL32.@)
 */
void WINAPI wine_glNormalPointer( GLenum type, GLsizei stride, const GLvoid* pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", type, stride, pointer );
  funcs->gl.p_glNormalPointer( type, stride, pointer );
}

/***********************************************************************
 *              glOrtho (OPENGL32.@)
 */
void WINAPI wine_glOrtho( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f, %f, %f, %f)\n", left, right, bottom, top, zNear, zFar );
  funcs->gl.p_glOrtho( left, right, bottom, top, zNear, zFar );
}

/***********************************************************************
 *              glPassThrough (OPENGL32.@)
 */
void WINAPI wine_glPassThrough( GLfloat token ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f)\n", token );
  funcs->gl.p_glPassThrough( token );
}

/***********************************************************************
 *              glPixelMapfv (OPENGL32.@)
 */
void WINAPI wine_glPixelMapfv( GLenum map, GLint mapsize, const GLfloat* values ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", map, mapsize, values );
  funcs->gl.p_glPixelMapfv( map, mapsize, values );
}

/***********************************************************************
 *              glPixelMapuiv (OPENGL32.@)
 */
void WINAPI wine_glPixelMapuiv( GLenum map, GLint mapsize, const GLuint* values ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", map, mapsize, values );
  funcs->gl.p_glPixelMapuiv( map, mapsize, values );
}

/***********************************************************************
 *              glPixelMapusv (OPENGL32.@)
 */
void WINAPI wine_glPixelMapusv( GLenum map, GLint mapsize, const GLushort* values ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", map, mapsize, values );
  funcs->gl.p_glPixelMapusv( map, mapsize, values );
}

/***********************************************************************
 *              glPixelStoref (OPENGL32.@)
 */
void WINAPI wine_glPixelStoref( GLenum pname, GLfloat param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f)\n", pname, param );
  funcs->gl.p_glPixelStoref( pname, param );
}

/***********************************************************************
 *              glPixelStorei (OPENGL32.@)
 */
void WINAPI wine_glPixelStorei( GLenum pname, GLint param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", pname, param );
  funcs->gl.p_glPixelStorei( pname, param );
}

/***********************************************************************
 *              glPixelTransferf (OPENGL32.@)
 */
void WINAPI wine_glPixelTransferf( GLenum pname, GLfloat param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f)\n", pname, param );
  funcs->gl.p_glPixelTransferf( pname, param );
}

/***********************************************************************
 *              glPixelTransferi (OPENGL32.@)
 */
void WINAPI wine_glPixelTransferi( GLenum pname, GLint param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", pname, param );
  funcs->gl.p_glPixelTransferi( pname, param );
}

/***********************************************************************
 *              glPixelZoom (OPENGL32.@)
 */
void WINAPI wine_glPixelZoom( GLfloat xfactor, GLfloat yfactor ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f)\n", xfactor, yfactor );
  funcs->gl.p_glPixelZoom( xfactor, yfactor );
}

/***********************************************************************
 *              glPointSize (OPENGL32.@)
 */
void WINAPI wine_glPointSize( GLfloat size ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f)\n", size );
  funcs->gl.p_glPointSize( size );
}

/***********************************************************************
 *              glPolygonMode (OPENGL32.@)
 */
void WINAPI wine_glPolygonMode( GLenum face, GLenum mode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", face, mode );
  funcs->gl.p_glPolygonMode( face, mode );
}

/***********************************************************************
 *              glPolygonOffset (OPENGL32.@)
 */
void WINAPI wine_glPolygonOffset( GLfloat factor, GLfloat units ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f)\n", factor, units );
  funcs->gl.p_glPolygonOffset( factor, units );
}

/***********************************************************************
 *              glPolygonStipple (OPENGL32.@)
 */
void WINAPI wine_glPolygonStipple( const GLubyte* mask ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", mask );
  funcs->gl.p_glPolygonStipple( mask );
}

/***********************************************************************
 *              glPopAttrib (OPENGL32.@)
 */
void WINAPI wine_glPopAttrib( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  funcs->gl.p_glPopAttrib( );
}

/***********************************************************************
 *              glPopClientAttrib (OPENGL32.@)
 */
void WINAPI wine_glPopClientAttrib( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  funcs->gl.p_glPopClientAttrib( );
}

/***********************************************************************
 *              glPopMatrix (OPENGL32.@)
 */
void WINAPI wine_glPopMatrix( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  funcs->gl.p_glPopMatrix( );
}

/***********************************************************************
 *              glPopName (OPENGL32.@)
 */
void WINAPI wine_glPopName( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  funcs->gl.p_glPopName( );
}

/***********************************************************************
 *              glPrioritizeTextures (OPENGL32.@)
 */
void WINAPI wine_glPrioritizeTextures( GLsizei n, const GLuint* textures, const GLfloat* priorities ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p, %p)\n", n, textures, priorities );
  funcs->gl.p_glPrioritizeTextures( n, textures, priorities );
}

/***********************************************************************
 *              glPushAttrib (OPENGL32.@)
 */
void WINAPI wine_glPushAttrib( GLbitfield mask ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", mask );
  funcs->gl.p_glPushAttrib( mask );
}

/***********************************************************************
 *              glPushClientAttrib (OPENGL32.@)
 */
void WINAPI wine_glPushClientAttrib( GLbitfield mask ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", mask );
  funcs->gl.p_glPushClientAttrib( mask );
}

/***********************************************************************
 *              glPushMatrix (OPENGL32.@)
 */
void WINAPI wine_glPushMatrix( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  funcs->gl.p_glPushMatrix( );
}

/***********************************************************************
 *              glPushName (OPENGL32.@)
 */
void WINAPI wine_glPushName( GLuint name ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", name );
  funcs->gl.p_glPushName( name );
}

/***********************************************************************
 *              glRasterPos2d (OPENGL32.@)
 */
void WINAPI wine_glRasterPos2d( GLdouble x, GLdouble y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f)\n", x, y );
  funcs->gl.p_glRasterPos2d( x, y );
}

/***********************************************************************
 *              glRasterPos2dv (OPENGL32.@)
 */
void WINAPI wine_glRasterPos2dv( const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glRasterPos2dv( v );
}

/***********************************************************************
 *              glRasterPos2f (OPENGL32.@)
 */
void WINAPI wine_glRasterPos2f( GLfloat x, GLfloat y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f)\n", x, y );
  funcs->gl.p_glRasterPos2f( x, y );
}

/***********************************************************************
 *              glRasterPos2fv (OPENGL32.@)
 */
void WINAPI wine_glRasterPos2fv( const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glRasterPos2fv( v );
}

/***********************************************************************
 *              glRasterPos2i (OPENGL32.@)
 */
void WINAPI wine_glRasterPos2i( GLint x, GLint y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", x, y );
  funcs->gl.p_glRasterPos2i( x, y );
}

/***********************************************************************
 *              glRasterPos2iv (OPENGL32.@)
 */
void WINAPI wine_glRasterPos2iv( const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glRasterPos2iv( v );
}

/***********************************************************************
 *              glRasterPos2s (OPENGL32.@)
 */
void WINAPI wine_glRasterPos2s( GLshort x, GLshort y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", x, y );
  funcs->gl.p_glRasterPos2s( x, y );
}

/***********************************************************************
 *              glRasterPos2sv (OPENGL32.@)
 */
void WINAPI wine_glRasterPos2sv( const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glRasterPos2sv( v );
}

/***********************************************************************
 *              glRasterPos3d (OPENGL32.@)
 */
void WINAPI wine_glRasterPos3d( GLdouble x, GLdouble y, GLdouble z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f)\n", x, y, z );
  funcs->gl.p_glRasterPos3d( x, y, z );
}

/***********************************************************************
 *              glRasterPos3dv (OPENGL32.@)
 */
void WINAPI wine_glRasterPos3dv( const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glRasterPos3dv( v );
}

/***********************************************************************
 *              glRasterPos3f (OPENGL32.@)
 */
void WINAPI wine_glRasterPos3f( GLfloat x, GLfloat y, GLfloat z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f)\n", x, y, z );
  funcs->gl.p_glRasterPos3f( x, y, z );
}

/***********************************************************************
 *              glRasterPos3fv (OPENGL32.@)
 */
void WINAPI wine_glRasterPos3fv( const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glRasterPos3fv( v );
}

/***********************************************************************
 *              glRasterPos3i (OPENGL32.@)
 */
void WINAPI wine_glRasterPos3i( GLint x, GLint y, GLint z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", x, y, z );
  funcs->gl.p_glRasterPos3i( x, y, z );
}

/***********************************************************************
 *              glRasterPos3iv (OPENGL32.@)
 */
void WINAPI wine_glRasterPos3iv( const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glRasterPos3iv( v );
}

/***********************************************************************
 *              glRasterPos3s (OPENGL32.@)
 */
void WINAPI wine_glRasterPos3s( GLshort x, GLshort y, GLshort z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", x, y, z );
  funcs->gl.p_glRasterPos3s( x, y, z );
}

/***********************************************************************
 *              glRasterPos3sv (OPENGL32.@)
 */
void WINAPI wine_glRasterPos3sv( const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glRasterPos3sv( v );
}

/***********************************************************************
 *              glRasterPos4d (OPENGL32.@)
 */
void WINAPI wine_glRasterPos4d( GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f, %f)\n", x, y, z, w );
  funcs->gl.p_glRasterPos4d( x, y, z, w );
}

/***********************************************************************
 *              glRasterPos4dv (OPENGL32.@)
 */
void WINAPI wine_glRasterPos4dv( const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glRasterPos4dv( v );
}

/***********************************************************************
 *              glRasterPos4f (OPENGL32.@)
 */
void WINAPI wine_glRasterPos4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f, %f)\n", x, y, z, w );
  funcs->gl.p_glRasterPos4f( x, y, z, w );
}

/***********************************************************************
 *              glRasterPos4fv (OPENGL32.@)
 */
void WINAPI wine_glRasterPos4fv( const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glRasterPos4fv( v );
}

/***********************************************************************
 *              glRasterPos4i (OPENGL32.@)
 */
void WINAPI wine_glRasterPos4i( GLint x, GLint y, GLint z, GLint w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", x, y, z, w );
  funcs->gl.p_glRasterPos4i( x, y, z, w );
}

/***********************************************************************
 *              glRasterPos4iv (OPENGL32.@)
 */
void WINAPI wine_glRasterPos4iv( const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glRasterPos4iv( v );
}

/***********************************************************************
 *              glRasterPos4s (OPENGL32.@)
 */
void WINAPI wine_glRasterPos4s( GLshort x, GLshort y, GLshort z, GLshort w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", x, y, z, w );
  funcs->gl.p_glRasterPos4s( x, y, z, w );
}

/***********************************************************************
 *              glRasterPos4sv (OPENGL32.@)
 */
void WINAPI wine_glRasterPos4sv( const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glRasterPos4sv( v );
}

/***********************************************************************
 *              glReadBuffer (OPENGL32.@)
 */
void WINAPI wine_glReadBuffer( GLenum mode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", mode );
  funcs->gl.p_glReadBuffer( mode );
}

/***********************************************************************
 *              glReadPixels (OPENGL32.@)
 */
void WINAPI wine_glReadPixels( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %p)\n", x, y, width, height, format, type, pixels );
  funcs->gl.p_glReadPixels( x, y, width, height, format, type, pixels );
}

/***********************************************************************
 *              glRectd (OPENGL32.@)
 */
void WINAPI wine_glRectd( GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f, %f)\n", x1, y1, x2, y2 );
  funcs->gl.p_glRectd( x1, y1, x2, y2 );
}

/***********************************************************************
 *              glRectdv (OPENGL32.@)
 */
void WINAPI wine_glRectdv( const GLdouble* v1, const GLdouble* v2 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p, %p)\n", v1, v2 );
  funcs->gl.p_glRectdv( v1, v2 );
}

/***********************************************************************
 *              glRectf (OPENGL32.@)
 */
void WINAPI wine_glRectf( GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f, %f)\n", x1, y1, x2, y2 );
  funcs->gl.p_glRectf( x1, y1, x2, y2 );
}

/***********************************************************************
 *              glRectfv (OPENGL32.@)
 */
void WINAPI wine_glRectfv( const GLfloat* v1, const GLfloat* v2 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p, %p)\n", v1, v2 );
  funcs->gl.p_glRectfv( v1, v2 );
}

/***********************************************************************
 *              glRecti (OPENGL32.@)
 */
void WINAPI wine_glRecti( GLint x1, GLint y1, GLint x2, GLint y2 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", x1, y1, x2, y2 );
  funcs->gl.p_glRecti( x1, y1, x2, y2 );
}

/***********************************************************************
 *              glRectiv (OPENGL32.@)
 */
void WINAPI wine_glRectiv( const GLint* v1, const GLint* v2 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p, %p)\n", v1, v2 );
  funcs->gl.p_glRectiv( v1, v2 );
}

/***********************************************************************
 *              glRects (OPENGL32.@)
 */
void WINAPI wine_glRects( GLshort x1, GLshort y1, GLshort x2, GLshort y2 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", x1, y1, x2, y2 );
  funcs->gl.p_glRects( x1, y1, x2, y2 );
}

/***********************************************************************
 *              glRectsv (OPENGL32.@)
 */
void WINAPI wine_glRectsv( const GLshort* v1, const GLshort* v2 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p, %p)\n", v1, v2 );
  funcs->gl.p_glRectsv( v1, v2 );
}

/***********************************************************************
 *              glRenderMode (OPENGL32.@)
 */
GLint WINAPI wine_glRenderMode( GLenum mode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", mode );
  return funcs->gl.p_glRenderMode( mode );
}

/***********************************************************************
 *              glRotated (OPENGL32.@)
 */
void WINAPI wine_glRotated( GLdouble angle, GLdouble x, GLdouble y, GLdouble z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f, %f)\n", angle, x, y, z );
  funcs->gl.p_glRotated( angle, x, y, z );
}

/***********************************************************************
 *              glRotatef (OPENGL32.@)
 */
void WINAPI wine_glRotatef( GLfloat angle, GLfloat x, GLfloat y, GLfloat z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f, %f)\n", angle, x, y, z );
  funcs->gl.p_glRotatef( angle, x, y, z );
}

/***********************************************************************
 *              glScaled (OPENGL32.@)
 */
void WINAPI wine_glScaled( GLdouble x, GLdouble y, GLdouble z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f)\n", x, y, z );
  funcs->gl.p_glScaled( x, y, z );
}

/***********************************************************************
 *              glScalef (OPENGL32.@)
 */
void WINAPI wine_glScalef( GLfloat x, GLfloat y, GLfloat z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f)\n", x, y, z );
  funcs->gl.p_glScalef( x, y, z );
}

/***********************************************************************
 *              glScissor (OPENGL32.@)
 */
void WINAPI wine_glScissor( GLint x, GLint y, GLsizei width, GLsizei height ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", x, y, width, height );
  funcs->gl.p_glScissor( x, y, width, height );
}

/***********************************************************************
 *              glSelectBuffer (OPENGL32.@)
 */
void WINAPI wine_glSelectBuffer( GLsizei size, GLuint* buffer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", size, buffer );
  funcs->gl.p_glSelectBuffer( size, buffer );
}

/***********************************************************************
 *              glShadeModel (OPENGL32.@)
 */
void WINAPI wine_glShadeModel( GLenum mode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", mode );
  funcs->gl.p_glShadeModel( mode );
}

/***********************************************************************
 *              glStencilFunc (OPENGL32.@)
 */
void WINAPI wine_glStencilFunc( GLenum func, GLint ref, GLuint mask ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", func, ref, mask );
  funcs->gl.p_glStencilFunc( func, ref, mask );
}

/***********************************************************************
 *              glStencilMask (OPENGL32.@)
 */
void WINAPI wine_glStencilMask( GLuint mask ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", mask );
  funcs->gl.p_glStencilMask( mask );
}

/***********************************************************************
 *              glStencilOp (OPENGL32.@)
 */
void WINAPI wine_glStencilOp( GLenum fail, GLenum zfail, GLenum zpass ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", fail, zfail, zpass );
  funcs->gl.p_glStencilOp( fail, zfail, zpass );
}

/***********************************************************************
 *              glTexCoord1d (OPENGL32.@)
 */
void WINAPI wine_glTexCoord1d( GLdouble s ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f)\n", s );
  funcs->gl.p_glTexCoord1d( s );
}

/***********************************************************************
 *              glTexCoord1dv (OPENGL32.@)
 */
void WINAPI wine_glTexCoord1dv( const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glTexCoord1dv( v );
}

/***********************************************************************
 *              glTexCoord1f (OPENGL32.@)
 */
void WINAPI wine_glTexCoord1f( GLfloat s ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f)\n", s );
  funcs->gl.p_glTexCoord1f( s );
}

/***********************************************************************
 *              glTexCoord1fv (OPENGL32.@)
 */
void WINAPI wine_glTexCoord1fv( const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glTexCoord1fv( v );
}

/***********************************************************************
 *              glTexCoord1i (OPENGL32.@)
 */
void WINAPI wine_glTexCoord1i( GLint s ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", s );
  funcs->gl.p_glTexCoord1i( s );
}

/***********************************************************************
 *              glTexCoord1iv (OPENGL32.@)
 */
void WINAPI wine_glTexCoord1iv( const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glTexCoord1iv( v );
}

/***********************************************************************
 *              glTexCoord1s (OPENGL32.@)
 */
void WINAPI wine_glTexCoord1s( GLshort s ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", s );
  funcs->gl.p_glTexCoord1s( s );
}

/***********************************************************************
 *              glTexCoord1sv (OPENGL32.@)
 */
void WINAPI wine_glTexCoord1sv( const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glTexCoord1sv( v );
}

/***********************************************************************
 *              glTexCoord2d (OPENGL32.@)
 */
void WINAPI wine_glTexCoord2d( GLdouble s, GLdouble t ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f)\n", s, t );
  funcs->gl.p_glTexCoord2d( s, t );
}

/***********************************************************************
 *              glTexCoord2dv (OPENGL32.@)
 */
void WINAPI wine_glTexCoord2dv( const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glTexCoord2dv( v );
}

/***********************************************************************
 *              glTexCoord2f (OPENGL32.@)
 */
void WINAPI wine_glTexCoord2f( GLfloat s, GLfloat t ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f)\n", s, t );
  funcs->gl.p_glTexCoord2f( s, t );
}

/***********************************************************************
 *              glTexCoord2fv (OPENGL32.@)
 */
void WINAPI wine_glTexCoord2fv( const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glTexCoord2fv( v );
}

/***********************************************************************
 *              glTexCoord2i (OPENGL32.@)
 */
void WINAPI wine_glTexCoord2i( GLint s, GLint t ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", s, t );
  funcs->gl.p_glTexCoord2i( s, t );
}

/***********************************************************************
 *              glTexCoord2iv (OPENGL32.@)
 */
void WINAPI wine_glTexCoord2iv( const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glTexCoord2iv( v );
}

/***********************************************************************
 *              glTexCoord2s (OPENGL32.@)
 */
void WINAPI wine_glTexCoord2s( GLshort s, GLshort t ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", s, t );
  funcs->gl.p_glTexCoord2s( s, t );
}

/***********************************************************************
 *              glTexCoord2sv (OPENGL32.@)
 */
void WINAPI wine_glTexCoord2sv( const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glTexCoord2sv( v );
}

/***********************************************************************
 *              glTexCoord3d (OPENGL32.@)
 */
void WINAPI wine_glTexCoord3d( GLdouble s, GLdouble t, GLdouble r ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f)\n", s, t, r );
  funcs->gl.p_glTexCoord3d( s, t, r );
}

/***********************************************************************
 *              glTexCoord3dv (OPENGL32.@)
 */
void WINAPI wine_glTexCoord3dv( const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glTexCoord3dv( v );
}

/***********************************************************************
 *              glTexCoord3f (OPENGL32.@)
 */
void WINAPI wine_glTexCoord3f( GLfloat s, GLfloat t, GLfloat r ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f)\n", s, t, r );
  funcs->gl.p_glTexCoord3f( s, t, r );
}

/***********************************************************************
 *              glTexCoord3fv (OPENGL32.@)
 */
void WINAPI wine_glTexCoord3fv( const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glTexCoord3fv( v );
}

/***********************************************************************
 *              glTexCoord3i (OPENGL32.@)
 */
void WINAPI wine_glTexCoord3i( GLint s, GLint t, GLint r ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", s, t, r );
  funcs->gl.p_glTexCoord3i( s, t, r );
}

/***********************************************************************
 *              glTexCoord3iv (OPENGL32.@)
 */
void WINAPI wine_glTexCoord3iv( const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glTexCoord3iv( v );
}

/***********************************************************************
 *              glTexCoord3s (OPENGL32.@)
 */
void WINAPI wine_glTexCoord3s( GLshort s, GLshort t, GLshort r ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", s, t, r );
  funcs->gl.p_glTexCoord3s( s, t, r );
}

/***********************************************************************
 *              glTexCoord3sv (OPENGL32.@)
 */
void WINAPI wine_glTexCoord3sv( const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glTexCoord3sv( v );
}

/***********************************************************************
 *              glTexCoord4d (OPENGL32.@)
 */
void WINAPI wine_glTexCoord4d( GLdouble s, GLdouble t, GLdouble r, GLdouble q ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f, %f)\n", s, t, r, q );
  funcs->gl.p_glTexCoord4d( s, t, r, q );
}

/***********************************************************************
 *              glTexCoord4dv (OPENGL32.@)
 */
void WINAPI wine_glTexCoord4dv( const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glTexCoord4dv( v );
}

/***********************************************************************
 *              glTexCoord4f (OPENGL32.@)
 */
void WINAPI wine_glTexCoord4f( GLfloat s, GLfloat t, GLfloat r, GLfloat q ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f, %f)\n", s, t, r, q );
  funcs->gl.p_glTexCoord4f( s, t, r, q );
}

/***********************************************************************
 *              glTexCoord4fv (OPENGL32.@)
 */
void WINAPI wine_glTexCoord4fv( const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glTexCoord4fv( v );
}

/***********************************************************************
 *              glTexCoord4i (OPENGL32.@)
 */
void WINAPI wine_glTexCoord4i( GLint s, GLint t, GLint r, GLint q ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", s, t, r, q );
  funcs->gl.p_glTexCoord4i( s, t, r, q );
}

/***********************************************************************
 *              glTexCoord4iv (OPENGL32.@)
 */
void WINAPI wine_glTexCoord4iv( const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glTexCoord4iv( v );
}

/***********************************************************************
 *              glTexCoord4s (OPENGL32.@)
 */
void WINAPI wine_glTexCoord4s( GLshort s, GLshort t, GLshort r, GLshort q ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", s, t, r, q );
  funcs->gl.p_glTexCoord4s( s, t, r, q );
}

/***********************************************************************
 *              glTexCoord4sv (OPENGL32.@)
 */
void WINAPI wine_glTexCoord4sv( const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glTexCoord4sv( v );
}

/***********************************************************************
 *              glTexCoordPointer (OPENGL32.@)
 */
void WINAPI wine_glTexCoordPointer( GLint size, GLenum type, GLsizei stride, const GLvoid* pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", size, type, stride, pointer );
  funcs->gl.p_glTexCoordPointer( size, type, stride, pointer );
}

/***********************************************************************
 *              glTexEnvf (OPENGL32.@)
 */
void WINAPI wine_glTexEnvf( GLenum target, GLenum pname, GLfloat param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f)\n", target, pname, param );
  funcs->gl.p_glTexEnvf( target, pname, param );
}

/***********************************************************************
 *              glTexEnvfv (OPENGL32.@)
 */
void WINAPI wine_glTexEnvfv( GLenum target, GLenum pname, const GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->gl.p_glTexEnvfv( target, pname, params );
}

/***********************************************************************
 *              glTexEnvi (OPENGL32.@)
 */
void WINAPI wine_glTexEnvi( GLenum target, GLenum pname, GLint param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", target, pname, param );
  funcs->gl.p_glTexEnvi( target, pname, param );
}

/***********************************************************************
 *              glTexEnviv (OPENGL32.@)
 */
void WINAPI wine_glTexEnviv( GLenum target, GLenum pname, const GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->gl.p_glTexEnviv( target, pname, params );
}

/***********************************************************************
 *              glTexGend (OPENGL32.@)
 */
void WINAPI wine_glTexGend( GLenum coord, GLenum pname, GLdouble param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f)\n", coord, pname, param );
  funcs->gl.p_glTexGend( coord, pname, param );
}

/***********************************************************************
 *              glTexGendv (OPENGL32.@)
 */
void WINAPI wine_glTexGendv( GLenum coord, GLenum pname, const GLdouble* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", coord, pname, params );
  funcs->gl.p_glTexGendv( coord, pname, params );
}

/***********************************************************************
 *              glTexGenf (OPENGL32.@)
 */
void WINAPI wine_glTexGenf( GLenum coord, GLenum pname, GLfloat param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f)\n", coord, pname, param );
  funcs->gl.p_glTexGenf( coord, pname, param );
}

/***********************************************************************
 *              glTexGenfv (OPENGL32.@)
 */
void WINAPI wine_glTexGenfv( GLenum coord, GLenum pname, const GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", coord, pname, params );
  funcs->gl.p_glTexGenfv( coord, pname, params );
}

/***********************************************************************
 *              glTexGeni (OPENGL32.@)
 */
void WINAPI wine_glTexGeni( GLenum coord, GLenum pname, GLint param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", coord, pname, param );
  funcs->gl.p_glTexGeni( coord, pname, param );
}

/***********************************************************************
 *              glTexGeniv (OPENGL32.@)
 */
void WINAPI wine_glTexGeniv( GLenum coord, GLenum pname, const GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", coord, pname, params );
  funcs->gl.p_glTexGeniv( coord, pname, params );
}

/***********************************************************************
 *              glTexImage1D (OPENGL32.@)
 */
void WINAPI wine_glTexImage1D( GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid* pixels ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, border, format, type, pixels );
  funcs->gl.p_glTexImage1D( target, level, internalformat, width, border, format, type, pixels );
}

/***********************************************************************
 *              glTexImage2D (OPENGL32.@)
 */
void WINAPI wine_glTexImage2D( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, height, border, format, type, pixels );
  funcs->gl.p_glTexImage2D( target, level, internalformat, width, height, border, format, type, pixels );
}

/***********************************************************************
 *              glTexParameterf (OPENGL32.@)
 */
void WINAPI wine_glTexParameterf( GLenum target, GLenum pname, GLfloat param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f)\n", target, pname, param );
  funcs->gl.p_glTexParameterf( target, pname, param );
}

/***********************************************************************
 *              glTexParameterfv (OPENGL32.@)
 */
void WINAPI wine_glTexParameterfv( GLenum target, GLenum pname, const GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->gl.p_glTexParameterfv( target, pname, params );
}

/***********************************************************************
 *              glTexParameteri (OPENGL32.@)
 */
void WINAPI wine_glTexParameteri( GLenum target, GLenum pname, GLint param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", target, pname, param );
  funcs->gl.p_glTexParameteri( target, pname, param );
}

/***********************************************************************
 *              glTexParameteriv (OPENGL32.@)
 */
void WINAPI wine_glTexParameteriv( GLenum target, GLenum pname, const GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->gl.p_glTexParameteriv( target, pname, params );
}

/***********************************************************************
 *              glTexSubImage1D (OPENGL32.@)
 */
void WINAPI wine_glTexSubImage1D( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid* pixels ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, width, format, type, pixels );
  funcs->gl.p_glTexSubImage1D( target, level, xoffset, width, format, type, pixels );
}

/***********************************************************************
 *              glTexSubImage2D (OPENGL32.@)
 */
void WINAPI wine_glTexSubImage2D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, width, height, format, type, pixels );
  funcs->gl.p_glTexSubImage2D( target, level, xoffset, yoffset, width, height, format, type, pixels );
}

/***********************************************************************
 *              glTranslated (OPENGL32.@)
 */
void WINAPI wine_glTranslated( GLdouble x, GLdouble y, GLdouble z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f)\n", x, y, z );
  funcs->gl.p_glTranslated( x, y, z );
}

/***********************************************************************
 *              glTranslatef (OPENGL32.@)
 */
void WINAPI wine_glTranslatef( GLfloat x, GLfloat y, GLfloat z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f)\n", x, y, z );
  funcs->gl.p_glTranslatef( x, y, z );
}

/***********************************************************************
 *              glVertex2d (OPENGL32.@)
 */
void WINAPI wine_glVertex2d( GLdouble x, GLdouble y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f)\n", x, y );
  funcs->gl.p_glVertex2d( x, y );
}

/***********************************************************************
 *              glVertex2dv (OPENGL32.@)
 */
void WINAPI wine_glVertex2dv( const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glVertex2dv( v );
}

/***********************************************************************
 *              glVertex2f (OPENGL32.@)
 */
void WINAPI wine_glVertex2f( GLfloat x, GLfloat y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f)\n", x, y );
  funcs->gl.p_glVertex2f( x, y );
}

/***********************************************************************
 *              glVertex2fv (OPENGL32.@)
 */
void WINAPI wine_glVertex2fv( const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glVertex2fv( v );
}

/***********************************************************************
 *              glVertex2i (OPENGL32.@)
 */
void WINAPI wine_glVertex2i( GLint x, GLint y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", x, y );
  funcs->gl.p_glVertex2i( x, y );
}

/***********************************************************************
 *              glVertex2iv (OPENGL32.@)
 */
void WINAPI wine_glVertex2iv( const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glVertex2iv( v );
}

/***********************************************************************
 *              glVertex2s (OPENGL32.@)
 */
void WINAPI wine_glVertex2s( GLshort x, GLshort y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", x, y );
  funcs->gl.p_glVertex2s( x, y );
}

/***********************************************************************
 *              glVertex2sv (OPENGL32.@)
 */
void WINAPI wine_glVertex2sv( const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glVertex2sv( v );
}

/***********************************************************************
 *              glVertex3d (OPENGL32.@)
 */
void WINAPI wine_glVertex3d( GLdouble x, GLdouble y, GLdouble z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f)\n", x, y, z );
  funcs->gl.p_glVertex3d( x, y, z );
}

/***********************************************************************
 *              glVertex3dv (OPENGL32.@)
 */
void WINAPI wine_glVertex3dv( const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glVertex3dv( v );
}

/***********************************************************************
 *              glVertex3f (OPENGL32.@)
 */
void WINAPI wine_glVertex3f( GLfloat x, GLfloat y, GLfloat z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f)\n", x, y, z );
  funcs->gl.p_glVertex3f( x, y, z );
}

/***********************************************************************
 *              glVertex3fv (OPENGL32.@)
 */
void WINAPI wine_glVertex3fv( const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glVertex3fv( v );
}

/***********************************************************************
 *              glVertex3i (OPENGL32.@)
 */
void WINAPI wine_glVertex3i( GLint x, GLint y, GLint z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", x, y, z );
  funcs->gl.p_glVertex3i( x, y, z );
}

/***********************************************************************
 *              glVertex3iv (OPENGL32.@)
 */
void WINAPI wine_glVertex3iv( const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glVertex3iv( v );
}

/***********************************************************************
 *              glVertex3s (OPENGL32.@)
 */
void WINAPI wine_glVertex3s( GLshort x, GLshort y, GLshort z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", x, y, z );
  funcs->gl.p_glVertex3s( x, y, z );
}

/***********************************************************************
 *              glVertex3sv (OPENGL32.@)
 */
void WINAPI wine_glVertex3sv( const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glVertex3sv( v );
}

/***********************************************************************
 *              glVertex4d (OPENGL32.@)
 */
void WINAPI wine_glVertex4d( GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f, %f)\n", x, y, z, w );
  funcs->gl.p_glVertex4d( x, y, z, w );
}

/***********************************************************************
 *              glVertex4dv (OPENGL32.@)
 */
void WINAPI wine_glVertex4dv( const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glVertex4dv( v );
}

/***********************************************************************
 *              glVertex4f (OPENGL32.@)
 */
void WINAPI wine_glVertex4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f, %f)\n", x, y, z, w );
  funcs->gl.p_glVertex4f( x, y, z, w );
}

/***********************************************************************
 *              glVertex4fv (OPENGL32.@)
 */
void WINAPI wine_glVertex4fv( const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glVertex4fv( v );
}

/***********************************************************************
 *              glVertex4i (OPENGL32.@)
 */
void WINAPI wine_glVertex4i( GLint x, GLint y, GLint z, GLint w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", x, y, z, w );
  funcs->gl.p_glVertex4i( x, y, z, w );
}

/***********************************************************************
 *              glVertex4iv (OPENGL32.@)
 */
void WINAPI wine_glVertex4iv( const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glVertex4iv( v );
}

/***********************************************************************
 *              glVertex4s (OPENGL32.@)
 */
void WINAPI wine_glVertex4s( GLshort x, GLshort y, GLshort z, GLshort w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", x, y, z, w );
  funcs->gl.p_glVertex4s( x, y, z, w );
}

/***********************************************************************
 *              glVertex4sv (OPENGL32.@)
 */
void WINAPI wine_glVertex4sv( const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->gl.p_glVertex4sv( v );
}

/***********************************************************************
 *              glVertexPointer (OPENGL32.@)
 */
void WINAPI wine_glVertexPointer( GLint size, GLenum type, GLsizei stride, const GLvoid* pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", size, type, stride, pointer );
  funcs->gl.p_glVertexPointer( size, type, stride, pointer );
}

/***********************************************************************
 *              glViewport (OPENGL32.@)
 */
void WINAPI wine_glViewport( GLint x, GLint y, GLsizei width, GLsizei height ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", x, y, width, height );
  funcs->gl.p_glViewport( x, y, width, height );
}
static BOOL null_wglCopyContext( struct wgl_context * src, struct wgl_context * dst, UINT mask ) { return 0; }
static struct wgl_context * null_wglCreateContext( HDC hdc ) { return 0; }
static void null_wglDeleteContext( struct wgl_context * context ) { }
static INT null_wglGetPixelFormat( HDC hdc ) { return 0; }
static PROC null_wglGetProcAddress( LPCSTR name ) { return 0; }
static BOOL null_wglMakeCurrent( HDC hdc, struct wgl_context * context ) { return 0; }
static BOOL null_wglShareLists( struct wgl_context * org, struct wgl_context * dst ) { return 0; }
static void null_glAccum( GLenum op, GLfloat value ) { }
static void null_glAlphaFunc( GLenum func, GLfloat ref ) { }
static GLboolean null_glAreTexturesResident( GLsizei n, const GLuint* textures, GLboolean* residences ) { return 0; }
static void null_glArrayElement( GLint i ) { }
static void null_glBegin( GLenum mode ) { }
static void null_glBindTexture( GLenum target, GLuint texture ) { }
static void null_glBitmap( GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte* bitmap ) { }
static void null_glBlendFunc( GLenum sfactor, GLenum dfactor ) { }
static void null_glCallList( GLuint list ) { }
static void null_glCallLists( GLsizei n, GLenum type, const GLvoid* lists ) { }
static void null_glClear( GLbitfield mask ) { }
static void null_glClearAccum( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha ) { }
static void null_glClearColor( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha ) { }
static void null_glClearDepth( GLdouble depth ) { }
static void null_glClearIndex( GLfloat c ) { }
static void null_glClearStencil( GLint s ) { }
static void null_glClipPlane( GLenum plane, const GLdouble* equation ) { }
static void null_glColor3b( GLbyte red, GLbyte green, GLbyte blue ) { }
static void null_glColor3bv( const GLbyte* v ) { }
static void null_glColor3d( GLdouble red, GLdouble green, GLdouble blue ) { }
static void null_glColor3dv( const GLdouble* v ) { }
static void null_glColor3f( GLfloat red, GLfloat green, GLfloat blue ) { }
static void null_glColor3fv( const GLfloat* v ) { }
static void null_glColor3i( GLint red, GLint green, GLint blue ) { }
static void null_glColor3iv( const GLint* v ) { }
static void null_glColor3s( GLshort red, GLshort green, GLshort blue ) { }
static void null_glColor3sv( const GLshort* v ) { }
static void null_glColor3ub( GLubyte red, GLubyte green, GLubyte blue ) { }
static void null_glColor3ubv( const GLubyte* v ) { }
static void null_glColor3ui( GLuint red, GLuint green, GLuint blue ) { }
static void null_glColor3uiv( const GLuint* v ) { }
static void null_glColor3us( GLushort red, GLushort green, GLushort blue ) { }
static void null_glColor3usv( const GLushort* v ) { }
static void null_glColor4b( GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha ) { }
static void null_glColor4bv( const GLbyte* v ) { }
static void null_glColor4d( GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha ) { }
static void null_glColor4dv( const GLdouble* v ) { }
static void null_glColor4f( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha ) { }
static void null_glColor4fv( const GLfloat* v ) { }
static void null_glColor4i( GLint red, GLint green, GLint blue, GLint alpha ) { }
static void null_glColor4iv( const GLint* v ) { }
static void null_glColor4s( GLshort red, GLshort green, GLshort blue, GLshort alpha ) { }
static void null_glColor4sv( const GLshort* v ) { }
static void null_glColor4ub( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha ) { }
static void null_glColor4ubv( const GLubyte* v ) { }
static void null_glColor4ui( GLuint red, GLuint green, GLuint blue, GLuint alpha ) { }
static void null_glColor4uiv( const GLuint* v ) { }
static void null_glColor4us( GLushort red, GLushort green, GLushort blue, GLushort alpha ) { }
static void null_glColor4usv( const GLushort* v ) { }
static void null_glColorMask( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha ) { }
static void null_glColorMaterial( GLenum face, GLenum mode ) { }
static void null_glColorPointer( GLint size, GLenum type, GLsizei stride, const GLvoid* pointer ) { }
static void null_glCopyPixels( GLint x, GLint y, GLsizei width, GLsizei height, GLenum type ) { }
static void null_glCopyTexImage1D( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border ) { }
static void null_glCopyTexImage2D( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border ) { }
static void null_glCopyTexSubImage1D( GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width ) { }
static void null_glCopyTexSubImage2D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height ) { }
static void null_glCullFace( GLenum mode ) { }
static void null_glDeleteLists( GLuint list, GLsizei range ) { }
static void null_glDeleteTextures( GLsizei n, const GLuint* textures ) { }
static void null_glDepthFunc( GLenum func ) { }
static void null_glDepthMask( GLboolean flag ) { }
static void null_glDepthRange( GLdouble nearParam, GLdouble farParam ) { }
static void null_glDisable( GLenum cap ) { }
static void null_glDisableClientState( GLenum array ) { }
static void null_glDrawArrays( GLenum mode, GLint first, GLsizei count ) { }
static void null_glDrawBuffer( GLenum mode ) { }
static void null_glDrawElements( GLenum mode, GLsizei count, GLenum type, const GLvoid* indices ) { }
static void null_glDrawPixels( GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels ) { }
static void null_glEdgeFlag( GLboolean flag ) { }
static void null_glEdgeFlagPointer( GLsizei stride, const GLvoid* pointer ) { }
static void null_glEdgeFlagv( const GLboolean* flag ) { }
static void null_glEnable( GLenum cap ) { }
static void null_glEnableClientState( GLenum array ) { }
static void null_glEnd( void ) { }
static void null_glEndList( void ) { }
static void null_glEvalCoord1d( GLdouble u ) { }
static void null_glEvalCoord1dv( const GLdouble* u ) { }
static void null_glEvalCoord1f( GLfloat u ) { }
static void null_glEvalCoord1fv( const GLfloat* u ) { }
static void null_glEvalCoord2d( GLdouble u, GLdouble v ) { }
static void null_glEvalCoord2dv( const GLdouble* u ) { }
static void null_glEvalCoord2f( GLfloat u, GLfloat v ) { }
static void null_glEvalCoord2fv( const GLfloat* u ) { }
static void null_glEvalMesh1( GLenum mode, GLint i1, GLint i2 ) { }
static void null_glEvalMesh2( GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2 ) { }
static void null_glEvalPoint1( GLint i ) { }
static void null_glEvalPoint2( GLint i, GLint j ) { }
static void null_glFeedbackBuffer( GLsizei size, GLenum type, GLfloat* buffer ) { }
static void null_glFinish( void ) { }
static void null_glFlush( void ) { }
static void null_glFogf( GLenum pname, GLfloat param ) { }
static void null_glFogfv( GLenum pname, const GLfloat* params ) { }
static void null_glFogi( GLenum pname, GLint param ) { }
static void null_glFogiv( GLenum pname, const GLint* params ) { }
static void null_glFrontFace( GLenum mode ) { }
static void null_glFrustum( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar ) { }
static GLuint null_glGenLists( GLsizei range ) { return 0; }
static void null_glGenTextures( GLsizei n, GLuint* textures ) { }
static void null_glGetBooleanv( GLenum pname, GLboolean* params ) { }
static void null_glGetClipPlane( GLenum plane, GLdouble* equation ) { }
static void null_glGetDoublev( GLenum pname, GLdouble* params ) { }
static GLenum null_glGetError( void ) { return GL_INVALID_OPERATION; }
static void null_glGetFloatv( GLenum pname, GLfloat* params ) { }
static void null_glGetIntegerv( GLenum pname, GLint* params ) { }
static void null_glGetLightfv( GLenum light, GLenum pname, GLfloat* params ) { }
static void null_glGetLightiv( GLenum light, GLenum pname, GLint* params ) { }
static void null_glGetMapdv( GLenum target, GLenum query, GLdouble* v ) { }
static void null_glGetMapfv( GLenum target, GLenum query, GLfloat* v ) { }
static void null_glGetMapiv( GLenum target, GLenum query, GLint* v ) { }
static void null_glGetMaterialfv( GLenum face, GLenum pname, GLfloat* params ) { }
static void null_glGetMaterialiv( GLenum face, GLenum pname, GLint* params ) { }
static void null_glGetPixelMapfv( GLenum map, GLfloat* values ) { }
static void null_glGetPixelMapuiv( GLenum map, GLuint* values ) { }
static void null_glGetPixelMapusv( GLenum map, GLushort* values ) { }
static void null_glGetPointerv( GLenum pname, GLvoid** params ) { }
static void null_glGetPolygonStipple( GLubyte* mask ) { }
static const GLubyte * null_glGetString( GLenum name ) { return 0; }
static void null_glGetTexEnvfv( GLenum target, GLenum pname, GLfloat* params ) { }
static void null_glGetTexEnviv( GLenum target, GLenum pname, GLint* params ) { }
static void null_glGetTexGendv( GLenum coord, GLenum pname, GLdouble* params ) { }
static void null_glGetTexGenfv( GLenum coord, GLenum pname, GLfloat* params ) { }
static void null_glGetTexGeniv( GLenum coord, GLenum pname, GLint* params ) { }
static void null_glGetTexImage( GLenum target, GLint level, GLenum format, GLenum type, GLvoid* pixels ) { }
static void null_glGetTexLevelParameterfv( GLenum target, GLint level, GLenum pname, GLfloat* params ) { }
static void null_glGetTexLevelParameteriv( GLenum target, GLint level, GLenum pname, GLint* params ) { }
static void null_glGetTexParameterfv( GLenum target, GLenum pname, GLfloat* params ) { }
static void null_glGetTexParameteriv( GLenum target, GLenum pname, GLint* params ) { }
static void null_glHint( GLenum target, GLenum mode ) { }
static void null_glIndexMask( GLuint mask ) { }
static void null_glIndexPointer( GLenum type, GLsizei stride, const GLvoid* pointer ) { }
static void null_glIndexd( GLdouble c ) { }
static void null_glIndexdv( const GLdouble* c ) { }
static void null_glIndexf( GLfloat c ) { }
static void null_glIndexfv( const GLfloat* c ) { }
static void null_glIndexi( GLint c ) { }
static void null_glIndexiv( const GLint* c ) { }
static void null_glIndexs( GLshort c ) { }
static void null_glIndexsv( const GLshort* c ) { }
static void null_glIndexub( GLubyte c ) { }
static void null_glIndexubv( const GLubyte* c ) { }
static void null_glInitNames( void ) { }
static void null_glInterleavedArrays( GLenum format, GLsizei stride, const GLvoid* pointer ) { }
static GLboolean null_glIsEnabled( GLenum cap ) { return 0; }
static GLboolean null_glIsList( GLuint list ) { return 0; }
static GLboolean null_glIsTexture( GLuint texture ) { return 0; }
static void null_glLightModelf( GLenum pname, GLfloat param ) { }
static void null_glLightModelfv( GLenum pname, const GLfloat* params ) { }
static void null_glLightModeli( GLenum pname, GLint param ) { }
static void null_glLightModeliv( GLenum pname, const GLint* params ) { }
static void null_glLightf( GLenum light, GLenum pname, GLfloat param ) { }
static void null_glLightfv( GLenum light, GLenum pname, const GLfloat* params ) { }
static void null_glLighti( GLenum light, GLenum pname, GLint param ) { }
static void null_glLightiv( GLenum light, GLenum pname, const GLint* params ) { }
static void null_glLineStipple( GLint factor, GLushort pattern ) { }
static void null_glLineWidth( GLfloat width ) { }
static void null_glListBase( GLuint base ) { }
static void null_glLoadIdentity( void ) { }
static void null_glLoadMatrixd( const GLdouble* m ) { }
static void null_glLoadMatrixf( const GLfloat* m ) { }
static void null_glLoadName( GLuint name ) { }
static void null_glLogicOp( GLenum opcode ) { }
static void null_glMap1d( GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble* points ) { }
static void null_glMap1f( GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat* points ) { }
static void null_glMap2d( GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble* points ) { }
static void null_glMap2f( GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat* points ) { }
static void null_glMapGrid1d( GLint un, GLdouble u1, GLdouble u2 ) { }
static void null_glMapGrid1f( GLint un, GLfloat u1, GLfloat u2 ) { }
static void null_glMapGrid2d( GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2 ) { }
static void null_glMapGrid2f( GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2 ) { }
static void null_glMaterialf( GLenum face, GLenum pname, GLfloat param ) { }
static void null_glMaterialfv( GLenum face, GLenum pname, const GLfloat* params ) { }
static void null_glMateriali( GLenum face, GLenum pname, GLint param ) { }
static void null_glMaterialiv( GLenum face, GLenum pname, const GLint* params ) { }
static void null_glMatrixMode( GLenum mode ) { }
static void null_glMultMatrixd( const GLdouble* m ) { }
static void null_glMultMatrixf( const GLfloat* m ) { }
static void null_glNewList( GLuint list, GLenum mode ) { }
static void null_glNormal3b( GLbyte nx, GLbyte ny, GLbyte nz ) { }
static void null_glNormal3bv( const GLbyte* v ) { }
static void null_glNormal3d( GLdouble nx, GLdouble ny, GLdouble nz ) { }
static void null_glNormal3dv( const GLdouble* v ) { }
static void null_glNormal3f( GLfloat nx, GLfloat ny, GLfloat nz ) { }
static void null_glNormal3fv( const GLfloat* v ) { }
static void null_glNormal3i( GLint nx, GLint ny, GLint nz ) { }
static void null_glNormal3iv( const GLint* v ) { }
static void null_glNormal3s( GLshort nx, GLshort ny, GLshort nz ) { }
static void null_glNormal3sv( const GLshort* v ) { }
static void null_glNormalPointer( GLenum type, GLsizei stride, const GLvoid* pointer ) { }
static void null_glOrtho( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar ) { }
static void null_glPassThrough( GLfloat token ) { }
static void null_glPixelMapfv( GLenum map, GLint mapsize, const GLfloat* values ) { }
static void null_glPixelMapuiv( GLenum map, GLint mapsize, const GLuint* values ) { }
static void null_glPixelMapusv( GLenum map, GLint mapsize, const GLushort* values ) { }
static void null_glPixelStoref( GLenum pname, GLfloat param ) { }
static void null_glPixelStorei( GLenum pname, GLint param ) { }
static void null_glPixelTransferf( GLenum pname, GLfloat param ) { }
static void null_glPixelTransferi( GLenum pname, GLint param ) { }
static void null_glPixelZoom( GLfloat xfactor, GLfloat yfactor ) { }
static void null_glPointSize( GLfloat size ) { }
static void null_glPolygonMode( GLenum face, GLenum mode ) { }
static void null_glPolygonOffset( GLfloat factor, GLfloat units ) { }
static void null_glPolygonStipple( const GLubyte* mask ) { }
static void null_glPopAttrib( void ) { }
static void null_glPopClientAttrib( void ) { }
static void null_glPopMatrix( void ) { }
static void null_glPopName( void ) { }
static void null_glPrioritizeTextures( GLsizei n, const GLuint* textures, const GLfloat* priorities ) { }
static void null_glPushAttrib( GLbitfield mask ) { }
static void null_glPushClientAttrib( GLbitfield mask ) { }
static void null_glPushMatrix( void ) { }
static void null_glPushName( GLuint name ) { }
static void null_glRasterPos2d( GLdouble x, GLdouble y ) { }
static void null_glRasterPos2dv( const GLdouble* v ) { }
static void null_glRasterPos2f( GLfloat x, GLfloat y ) { }
static void null_glRasterPos2fv( const GLfloat* v ) { }
static void null_glRasterPos2i( GLint x, GLint y ) { }
static void null_glRasterPos2iv( const GLint* v ) { }
static void null_glRasterPos2s( GLshort x, GLshort y ) { }
static void null_glRasterPos2sv( const GLshort* v ) { }
static void null_glRasterPos3d( GLdouble x, GLdouble y, GLdouble z ) { }
static void null_glRasterPos3dv( const GLdouble* v ) { }
static void null_glRasterPos3f( GLfloat x, GLfloat y, GLfloat z ) { }
static void null_glRasterPos3fv( const GLfloat* v ) { }
static void null_glRasterPos3i( GLint x, GLint y, GLint z ) { }
static void null_glRasterPos3iv( const GLint* v ) { }
static void null_glRasterPos3s( GLshort x, GLshort y, GLshort z ) { }
static void null_glRasterPos3sv( const GLshort* v ) { }
static void null_glRasterPos4d( GLdouble x, GLdouble y, GLdouble z, GLdouble w ) { }
static void null_glRasterPos4dv( const GLdouble* v ) { }
static void null_glRasterPos4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w ) { }
static void null_glRasterPos4fv( const GLfloat* v ) { }
static void null_glRasterPos4i( GLint x, GLint y, GLint z, GLint w ) { }
static void null_glRasterPos4iv( const GLint* v ) { }
static void null_glRasterPos4s( GLshort x, GLshort y, GLshort z, GLshort w ) { }
static void null_glRasterPos4sv( const GLshort* v ) { }
static void null_glReadBuffer( GLenum mode ) { }
static void null_glReadPixels( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels ) { }
static void null_glRectd( GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2 ) { }
static void null_glRectdv( const GLdouble* v1, const GLdouble* v2 ) { }
static void null_glRectf( GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 ) { }
static void null_glRectfv( const GLfloat* v1, const GLfloat* v2 ) { }
static void null_glRecti( GLint x1, GLint y1, GLint x2, GLint y2 ) { }
static void null_glRectiv( const GLint* v1, const GLint* v2 ) { }
static void null_glRects( GLshort x1, GLshort y1, GLshort x2, GLshort y2 ) { }
static void null_glRectsv( const GLshort* v1, const GLshort* v2 ) { }
static GLint null_glRenderMode( GLenum mode ) { return 0; }
static void null_glRotated( GLdouble angle, GLdouble x, GLdouble y, GLdouble z ) { }
static void null_glRotatef( GLfloat angle, GLfloat x, GLfloat y, GLfloat z ) { }
static void null_glScaled( GLdouble x, GLdouble y, GLdouble z ) { }
static void null_glScalef( GLfloat x, GLfloat y, GLfloat z ) { }
static void null_glScissor( GLint x, GLint y, GLsizei width, GLsizei height ) { }
static void null_glSelectBuffer( GLsizei size, GLuint* buffer ) { }
static void null_glShadeModel( GLenum mode ) { }
static void null_glStencilFunc( GLenum func, GLint ref, GLuint mask ) { }
static void null_glStencilMask( GLuint mask ) { }
static void null_glStencilOp( GLenum fail, GLenum zfail, GLenum zpass ) { }
static void null_glTexCoord1d( GLdouble s ) { }
static void null_glTexCoord1dv( const GLdouble* v ) { }
static void null_glTexCoord1f( GLfloat s ) { }
static void null_glTexCoord1fv( const GLfloat* v ) { }
static void null_glTexCoord1i( GLint s ) { }
static void null_glTexCoord1iv( const GLint* v ) { }
static void null_glTexCoord1s( GLshort s ) { }
static void null_glTexCoord1sv( const GLshort* v ) { }
static void null_glTexCoord2d( GLdouble s, GLdouble t ) { }
static void null_glTexCoord2dv( const GLdouble* v ) { }
static void null_glTexCoord2f( GLfloat s, GLfloat t ) { }
static void null_glTexCoord2fv( const GLfloat* v ) { }
static void null_glTexCoord2i( GLint s, GLint t ) { }
static void null_glTexCoord2iv( const GLint* v ) { }
static void null_glTexCoord2s( GLshort s, GLshort t ) { }
static void null_glTexCoord2sv( const GLshort* v ) { }
static void null_glTexCoord3d( GLdouble s, GLdouble t, GLdouble r ) { }
static void null_glTexCoord3dv( const GLdouble* v ) { }
static void null_glTexCoord3f( GLfloat s, GLfloat t, GLfloat r ) { }
static void null_glTexCoord3fv( const GLfloat* v ) { }
static void null_glTexCoord3i( GLint s, GLint t, GLint r ) { }
static void null_glTexCoord3iv( const GLint* v ) { }
static void null_glTexCoord3s( GLshort s, GLshort t, GLshort r ) { }
static void null_glTexCoord3sv( const GLshort* v ) { }
static void null_glTexCoord4d( GLdouble s, GLdouble t, GLdouble r, GLdouble q ) { }
static void null_glTexCoord4dv( const GLdouble* v ) { }
static void null_glTexCoord4f( GLfloat s, GLfloat t, GLfloat r, GLfloat q ) { }
static void null_glTexCoord4fv( const GLfloat* v ) { }
static void null_glTexCoord4i( GLint s, GLint t, GLint r, GLint q ) { }
static void null_glTexCoord4iv( const GLint* v ) { }
static void null_glTexCoord4s( GLshort s, GLshort t, GLshort r, GLshort q ) { }
static void null_glTexCoord4sv( const GLshort* v ) { }
static void null_glTexCoordPointer( GLint size, GLenum type, GLsizei stride, const GLvoid* pointer ) { }
static void null_glTexEnvf( GLenum target, GLenum pname, GLfloat param ) { }
static void null_glTexEnvfv( GLenum target, GLenum pname, const GLfloat* params ) { }
static void null_glTexEnvi( GLenum target, GLenum pname, GLint param ) { }
static void null_glTexEnviv( GLenum target, GLenum pname, const GLint* params ) { }
static void null_glTexGend( GLenum coord, GLenum pname, GLdouble param ) { }
static void null_glTexGendv( GLenum coord, GLenum pname, const GLdouble* params ) { }
static void null_glTexGenf( GLenum coord, GLenum pname, GLfloat param ) { }
static void null_glTexGenfv( GLenum coord, GLenum pname, const GLfloat* params ) { }
static void null_glTexGeni( GLenum coord, GLenum pname, GLint param ) { }
static void null_glTexGeniv( GLenum coord, GLenum pname, const GLint* params ) { }
static void null_glTexImage1D( GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid* pixels ) { }
static void null_glTexImage2D( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels ) { }
static void null_glTexParameterf( GLenum target, GLenum pname, GLfloat param ) { }
static void null_glTexParameterfv( GLenum target, GLenum pname, const GLfloat* params ) { }
static void null_glTexParameteri( GLenum target, GLenum pname, GLint param ) { }
static void null_glTexParameteriv( GLenum target, GLenum pname, const GLint* params ) { }
static void null_glTexSubImage1D( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid* pixels ) { }
static void null_glTexSubImage2D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels ) { }
static void null_glTranslated( GLdouble x, GLdouble y, GLdouble z ) { }
static void null_glTranslatef( GLfloat x, GLfloat y, GLfloat z ) { }
static void null_glVertex2d( GLdouble x, GLdouble y ) { }
static void null_glVertex2dv( const GLdouble* v ) { }
static void null_glVertex2f( GLfloat x, GLfloat y ) { }
static void null_glVertex2fv( const GLfloat* v ) { }
static void null_glVertex2i( GLint x, GLint y ) { }
static void null_glVertex2iv( const GLint* v ) { }
static void null_glVertex2s( GLshort x, GLshort y ) { }
static void null_glVertex2sv( const GLshort* v ) { }
static void null_glVertex3d( GLdouble x, GLdouble y, GLdouble z ) { }
static void null_glVertex3dv( const GLdouble* v ) { }
static void null_glVertex3f( GLfloat x, GLfloat y, GLfloat z ) { }
static void null_glVertex3fv( const GLfloat* v ) { }
static void null_glVertex3i( GLint x, GLint y, GLint z ) { }
static void null_glVertex3iv( const GLint* v ) { }
static void null_glVertex3s( GLshort x, GLshort y, GLshort z ) { }
static void null_glVertex3sv( const GLshort* v ) { }
static void null_glVertex4d( GLdouble x, GLdouble y, GLdouble z, GLdouble w ) { }
static void null_glVertex4dv( const GLdouble* v ) { }
static void null_glVertex4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w ) { }
static void null_glVertex4fv( const GLfloat* v ) { }
static void null_glVertex4i( GLint x, GLint y, GLint z, GLint w ) { }
static void null_glVertex4iv( const GLint* v ) { }
static void null_glVertex4s( GLshort x, GLshort y, GLshort z, GLshort w ) { }
static void null_glVertex4sv( const GLshort* v ) { }
static void null_glVertexPointer( GLint size, GLenum type, GLsizei stride, const GLvoid* pointer ) { }
static void null_glViewport( GLint x, GLint y, GLsizei width, GLsizei height ) { }

struct opengl_funcs null_opengl_funcs =
{
    {
        null_wglCopyContext,
        null_wglCreateContext,
        null_wglDeleteContext,
        null_wglGetPixelFormat,
        null_wglGetProcAddress,
        null_wglMakeCurrent,
        null_wglShareLists,
    },
#define USE_GL_FUNC(name) null_##name,
    { ALL_WGL_FUNCS }
#undef USE_GL_FUNC
};
