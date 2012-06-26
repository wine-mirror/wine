
/* Auto-generated file... Do not edit ! */

#include "config.h"
#include "opengl_ext.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(opengl);

/***********************************************************************
 *              glAccum (OPENGL32.@)
 */
void WINAPI wine_glAccum( GLenum op, GLfloat value ) {
  TRACE("(%d, %f)\n", op, value );
  glAccum( op, value );
}

/***********************************************************************
 *              glAlphaFunc (OPENGL32.@)
 */
void WINAPI wine_glAlphaFunc( GLenum func, GLfloat ref ) {
  TRACE("(%d, %f)\n", func, ref );
  glAlphaFunc( func, ref );
}

/***********************************************************************
 *              glAreTexturesResident (OPENGL32.@)
 */
GLboolean WINAPI wine_glAreTexturesResident( GLsizei n, GLuint* textures, GLboolean* residences ) {
  TRACE("(%d, %p, %p)\n", n, textures, residences );
  return glAreTexturesResident( n, textures, residences );
}

/***********************************************************************
 *              glArrayElement (OPENGL32.@)
 */
void WINAPI wine_glArrayElement( GLint i ) {
  TRACE("(%d)\n", i );
  glArrayElement( i );
}

/***********************************************************************
 *              glBegin (OPENGL32.@)
 */
void WINAPI wine_glBegin( GLenum mode ) {
  TRACE("(%d)\n", mode );
  glBegin( mode );
}

/***********************************************************************
 *              glBindTexture (OPENGL32.@)
 */
void WINAPI wine_glBindTexture( GLenum target, GLuint texture ) {
  TRACE("(%d, %d)\n", target, texture );
  glBindTexture( target, texture );
}

/***********************************************************************
 *              glBitmap (OPENGL32.@)
 */
void WINAPI wine_glBitmap( GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, GLubyte* bitmap ) {
  TRACE("(%d, %d, %f, %f, %f, %f, %p)\n", width, height, xorig, yorig, xmove, ymove, bitmap );
  glBitmap( width, height, xorig, yorig, xmove, ymove, bitmap );
}

/***********************************************************************
 *              glBlendFunc (OPENGL32.@)
 */
void WINAPI wine_glBlendFunc( GLenum sfactor, GLenum dfactor ) {
  TRACE("(%d, %d)\n", sfactor, dfactor );
  glBlendFunc( sfactor, dfactor );
}

/***********************************************************************
 *              glCallList (OPENGL32.@)
 */
void WINAPI wine_glCallList( GLuint list ) {
  TRACE("(%d)\n", list );
  glCallList( list );
}

/***********************************************************************
 *              glCallLists (OPENGL32.@)
 */
void WINAPI wine_glCallLists( GLsizei n, GLenum type, GLvoid* lists ) {
  TRACE("(%d, %d, %p)\n", n, type, lists );
  glCallLists( n, type, lists );
}

/***********************************************************************
 *              glClear (OPENGL32.@)
 */
void WINAPI wine_glClear( GLbitfield mask ) {
  TRACE("(%d)\n", mask );
  glClear( mask );
}

/***********************************************************************
 *              glClearAccum (OPENGL32.@)
 */
void WINAPI wine_glClearAccum( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha ) {
  TRACE("(%f, %f, %f, %f)\n", red, green, blue, alpha );
  glClearAccum( red, green, blue, alpha );
}

/***********************************************************************
 *              glClearColor (OPENGL32.@)
 */
void WINAPI wine_glClearColor( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha ) {
  TRACE("(%f, %f, %f, %f)\n", red, green, blue, alpha );
  glClearColor( red, green, blue, alpha );
}

/***********************************************************************
 *              glClearDepth (OPENGL32.@)
 */
void WINAPI wine_glClearDepth( GLdouble depth ) {
  TRACE("(%f)\n", depth );
  glClearDepth( depth );
}

/***********************************************************************
 *              glClearIndex (OPENGL32.@)
 */
void WINAPI wine_glClearIndex( GLfloat c ) {
  TRACE("(%f)\n", c );
  glClearIndex( c );
}

/***********************************************************************
 *              glClearStencil (OPENGL32.@)
 */
void WINAPI wine_glClearStencil( GLint s ) {
  TRACE("(%d)\n", s );
  glClearStencil( s );
}

/***********************************************************************
 *              glClipPlane (OPENGL32.@)
 */
void WINAPI wine_glClipPlane( GLenum plane, GLdouble* equation ) {
  TRACE("(%d, %p)\n", plane, equation );
  glClipPlane( plane, equation );
}

/***********************************************************************
 *              glColor3b (OPENGL32.@)
 */
void WINAPI wine_glColor3b( GLbyte red, GLbyte green, GLbyte blue ) {
  TRACE("(%d, %d, %d)\n", red, green, blue );
  glColor3b( red, green, blue );
}

/***********************************************************************
 *              glColor3bv (OPENGL32.@)
 */
void WINAPI wine_glColor3bv( GLbyte* v ) {
  TRACE("(%p)\n", v );
  glColor3bv( v );
}

/***********************************************************************
 *              glColor3d (OPENGL32.@)
 */
void WINAPI wine_glColor3d( GLdouble red, GLdouble green, GLdouble blue ) {
  TRACE("(%f, %f, %f)\n", red, green, blue );
  glColor3d( red, green, blue );
}

/***********************************************************************
 *              glColor3dv (OPENGL32.@)
 */
void WINAPI wine_glColor3dv( GLdouble* v ) {
  TRACE("(%p)\n", v );
  glColor3dv( v );
}

/***********************************************************************
 *              glColor3f (OPENGL32.@)
 */
void WINAPI wine_glColor3f( GLfloat red, GLfloat green, GLfloat blue ) {
  TRACE("(%f, %f, %f)\n", red, green, blue );
  glColor3f( red, green, blue );
}

/***********************************************************************
 *              glColor3fv (OPENGL32.@)
 */
void WINAPI wine_glColor3fv( GLfloat* v ) {
  TRACE("(%p)\n", v );
  glColor3fv( v );
}

/***********************************************************************
 *              glColor3i (OPENGL32.@)
 */
void WINAPI wine_glColor3i( GLint red, GLint green, GLint blue ) {
  TRACE("(%d, %d, %d)\n", red, green, blue );
  glColor3i( red, green, blue );
}

/***********************************************************************
 *              glColor3iv (OPENGL32.@)
 */
void WINAPI wine_glColor3iv( GLint* v ) {
  TRACE("(%p)\n", v );
  glColor3iv( v );
}

/***********************************************************************
 *              glColor3s (OPENGL32.@)
 */
void WINAPI wine_glColor3s( GLshort red, GLshort green, GLshort blue ) {
  TRACE("(%d, %d, %d)\n", red, green, blue );
  glColor3s( red, green, blue );
}

/***********************************************************************
 *              glColor3sv (OPENGL32.@)
 */
void WINAPI wine_glColor3sv( GLshort* v ) {
  TRACE("(%p)\n", v );
  glColor3sv( v );
}

/***********************************************************************
 *              glColor3ub (OPENGL32.@)
 */
void WINAPI wine_glColor3ub( GLubyte red, GLubyte green, GLubyte blue ) {
  TRACE("(%d, %d, %d)\n", red, green, blue );
  glColor3ub( red, green, blue );
}

/***********************************************************************
 *              glColor3ubv (OPENGL32.@)
 */
void WINAPI wine_glColor3ubv( GLubyte* v ) {
  TRACE("(%p)\n", v );
  glColor3ubv( v );
}

/***********************************************************************
 *              glColor3ui (OPENGL32.@)
 */
void WINAPI wine_glColor3ui( GLuint red, GLuint green, GLuint blue ) {
  TRACE("(%d, %d, %d)\n", red, green, blue );
  glColor3ui( red, green, blue );
}

/***********************************************************************
 *              glColor3uiv (OPENGL32.@)
 */
void WINAPI wine_glColor3uiv( GLuint* v ) {
  TRACE("(%p)\n", v );
  glColor3uiv( v );
}

/***********************************************************************
 *              glColor3us (OPENGL32.@)
 */
void WINAPI wine_glColor3us( GLushort red, GLushort green, GLushort blue ) {
  TRACE("(%d, %d, %d)\n", red, green, blue );
  glColor3us( red, green, blue );
}

/***********************************************************************
 *              glColor3usv (OPENGL32.@)
 */
void WINAPI wine_glColor3usv( GLushort* v ) {
  TRACE("(%p)\n", v );
  glColor3usv( v );
}

/***********************************************************************
 *              glColor4b (OPENGL32.@)
 */
void WINAPI wine_glColor4b( GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha ) {
  TRACE("(%d, %d, %d, %d)\n", red, green, blue, alpha );
  glColor4b( red, green, blue, alpha );
}

/***********************************************************************
 *              glColor4bv (OPENGL32.@)
 */
void WINAPI wine_glColor4bv( GLbyte* v ) {
  TRACE("(%p)\n", v );
  glColor4bv( v );
}

/***********************************************************************
 *              glColor4d (OPENGL32.@)
 */
void WINAPI wine_glColor4d( GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha ) {
  TRACE("(%f, %f, %f, %f)\n", red, green, blue, alpha );
  glColor4d( red, green, blue, alpha );
}

/***********************************************************************
 *              glColor4dv (OPENGL32.@)
 */
void WINAPI wine_glColor4dv( GLdouble* v ) {
  TRACE("(%p)\n", v );
  glColor4dv( v );
}

/***********************************************************************
 *              glColor4f (OPENGL32.@)
 */
void WINAPI wine_glColor4f( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha ) {
  TRACE("(%f, %f, %f, %f)\n", red, green, blue, alpha );
  glColor4f( red, green, blue, alpha );
}

/***********************************************************************
 *              glColor4fv (OPENGL32.@)
 */
void WINAPI wine_glColor4fv( GLfloat* v ) {
  TRACE("(%p)\n", v );
  glColor4fv( v );
}

/***********************************************************************
 *              glColor4i (OPENGL32.@)
 */
void WINAPI wine_glColor4i( GLint red, GLint green, GLint blue, GLint alpha ) {
  TRACE("(%d, %d, %d, %d)\n", red, green, blue, alpha );
  glColor4i( red, green, blue, alpha );
}

/***********************************************************************
 *              glColor4iv (OPENGL32.@)
 */
void WINAPI wine_glColor4iv( GLint* v ) {
  TRACE("(%p)\n", v );
  glColor4iv( v );
}

/***********************************************************************
 *              glColor4s (OPENGL32.@)
 */
void WINAPI wine_glColor4s( GLshort red, GLshort green, GLshort blue, GLshort alpha ) {
  TRACE("(%d, %d, %d, %d)\n", red, green, blue, alpha );
  glColor4s( red, green, blue, alpha );
}

/***********************************************************************
 *              glColor4sv (OPENGL32.@)
 */
void WINAPI wine_glColor4sv( GLshort* v ) {
  TRACE("(%p)\n", v );
  glColor4sv( v );
}

/***********************************************************************
 *              glColor4ub (OPENGL32.@)
 */
void WINAPI wine_glColor4ub( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha ) {
  TRACE("(%d, %d, %d, %d)\n", red, green, blue, alpha );
  glColor4ub( red, green, blue, alpha );
}

/***********************************************************************
 *              glColor4ubv (OPENGL32.@)
 */
void WINAPI wine_glColor4ubv( GLubyte* v ) {
  TRACE("(%p)\n", v );
  glColor4ubv( v );
}

/***********************************************************************
 *              glColor4ui (OPENGL32.@)
 */
void WINAPI wine_glColor4ui( GLuint red, GLuint green, GLuint blue, GLuint alpha ) {
  TRACE("(%d, %d, %d, %d)\n", red, green, blue, alpha );
  glColor4ui( red, green, blue, alpha );
}

/***********************************************************************
 *              glColor4uiv (OPENGL32.@)
 */
void WINAPI wine_glColor4uiv( GLuint* v ) {
  TRACE("(%p)\n", v );
  glColor4uiv( v );
}

/***********************************************************************
 *              glColor4us (OPENGL32.@)
 */
void WINAPI wine_glColor4us( GLushort red, GLushort green, GLushort blue, GLushort alpha ) {
  TRACE("(%d, %d, %d, %d)\n", red, green, blue, alpha );
  glColor4us( red, green, blue, alpha );
}

/***********************************************************************
 *              glColor4usv (OPENGL32.@)
 */
void WINAPI wine_glColor4usv( GLushort* v ) {
  TRACE("(%p)\n", v );
  glColor4usv( v );
}

/***********************************************************************
 *              glColorMask (OPENGL32.@)
 */
void WINAPI wine_glColorMask( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha ) {
  TRACE("(%d, %d, %d, %d)\n", red, green, blue, alpha );
  glColorMask( red, green, blue, alpha );
}

/***********************************************************************
 *              glColorMaterial (OPENGL32.@)
 */
void WINAPI wine_glColorMaterial( GLenum face, GLenum mode ) {
  TRACE("(%d, %d)\n", face, mode );
  glColorMaterial( face, mode );
}

/***********************************************************************
 *              glColorPointer (OPENGL32.@)
 */
void WINAPI wine_glColorPointer( GLint size, GLenum type, GLsizei stride, GLvoid* pointer ) {
  TRACE("(%d, %d, %d, %p)\n", size, type, stride, pointer );
  glColorPointer( size, type, stride, pointer );
}

/***********************************************************************
 *              glCopyPixels (OPENGL32.@)
 */
void WINAPI wine_glCopyPixels( GLint x, GLint y, GLsizei width, GLsizei height, GLenum type ) {
  TRACE("(%d, %d, %d, %d, %d)\n", x, y, width, height, type );
  glCopyPixels( x, y, width, height, type );
}

/***********************************************************************
 *              glCopyTexImage1D (OPENGL32.@)
 */
void WINAPI wine_glCopyTexImage1D( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %d)\n", target, level, internalformat, x, y, width, border );
  glCopyTexImage1D( target, level, internalformat, x, y, width, border );
}

/***********************************************************************
 *              glCopyTexImage2D (OPENGL32.@)
 */
void WINAPI wine_glCopyTexImage2D( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d)\n", target, level, internalformat, x, y, width, height, border );
  glCopyTexImage2D( target, level, internalformat, x, y, width, height, border );
}

/***********************************************************************
 *              glCopyTexSubImage1D (OPENGL32.@)
 */
void WINAPI wine_glCopyTexSubImage1D( GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width ) {
  TRACE("(%d, %d, %d, %d, %d, %d)\n", target, level, xoffset, x, y, width );
  glCopyTexSubImage1D( target, level, xoffset, x, y, width );
}

/***********************************************************************
 *              glCopyTexSubImage2D (OPENGL32.@)
 */
void WINAPI wine_glCopyTexSubImage2D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d)\n", target, level, xoffset, yoffset, x, y, width, height );
  glCopyTexSubImage2D( target, level, xoffset, yoffset, x, y, width, height );
}

/***********************************************************************
 *              glCullFace (OPENGL32.@)
 */
void WINAPI wine_glCullFace( GLenum mode ) {
  TRACE("(%d)\n", mode );
  glCullFace( mode );
}

/***********************************************************************
 *              glDeleteLists (OPENGL32.@)
 */
void WINAPI wine_glDeleteLists( GLuint list, GLsizei range ) {
  TRACE("(%d, %d)\n", list, range );
  glDeleteLists( list, range );
}

/***********************************************************************
 *              glDeleteTextures (OPENGL32.@)
 */
void WINAPI wine_glDeleteTextures( GLsizei n, GLuint* textures ) {
  TRACE("(%d, %p)\n", n, textures );
  glDeleteTextures( n, textures );
}

/***********************************************************************
 *              glDepthFunc (OPENGL32.@)
 */
void WINAPI wine_glDepthFunc( GLenum func ) {
  TRACE("(%d)\n", func );
  glDepthFunc( func );
}

/***********************************************************************
 *              glDepthMask (OPENGL32.@)
 */
void WINAPI wine_glDepthMask( GLboolean flag ) {
  TRACE("(%d)\n", flag );
  glDepthMask( flag );
}

/***********************************************************************
 *              glDepthRange (OPENGL32.@)
 */
void WINAPI wine_glDepthRange( GLdouble nearParam, GLdouble farParam ) {
  TRACE("(%f, %f)\n", nearParam, farParam );
  glDepthRange( nearParam, farParam );
}

/***********************************************************************
 *              glDisable (OPENGL32.@)
 */
void WINAPI wine_glDisable( GLenum cap ) {
  TRACE("(%d)\n", cap );
  glDisable( cap );
}

/***********************************************************************
 *              glDisableClientState (OPENGL32.@)
 */
void WINAPI wine_glDisableClientState( GLenum array ) {
  TRACE("(%d)\n", array );
  glDisableClientState( array );
}

/***********************************************************************
 *              glDrawArrays (OPENGL32.@)
 */
void WINAPI wine_glDrawArrays( GLenum mode, GLint first, GLsizei count ) {
  TRACE("(%d, %d, %d)\n", mode, first, count );
  glDrawArrays( mode, first, count );
}

/***********************************************************************
 *              glDrawBuffer (OPENGL32.@)
 */
void WINAPI wine_glDrawBuffer( GLenum mode ) {
  TRACE("(%d)\n", mode );
  glDrawBuffer( mode );
}

/***********************************************************************
 *              glDrawElements (OPENGL32.@)
 */
void WINAPI wine_glDrawElements( GLenum mode, GLsizei count, GLenum type, GLvoid* indices ) {
  TRACE("(%d, %d, %d, %p)\n", mode, count, type, indices );
  glDrawElements( mode, count, type, indices );
}

/***********************************************************************
 *              glDrawPixels (OPENGL32.@)
 */
void WINAPI wine_glDrawPixels( GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels ) {
  TRACE("(%d, %d, %d, %d, %p)\n", width, height, format, type, pixels );
  glDrawPixels( width, height, format, type, pixels );
}

/***********************************************************************
 *              glEdgeFlag (OPENGL32.@)
 */
void WINAPI wine_glEdgeFlag( GLboolean flag ) {
  TRACE("(%d)\n", flag );
  glEdgeFlag( flag );
}

/***********************************************************************
 *              glEdgeFlagPointer (OPENGL32.@)
 */
void WINAPI wine_glEdgeFlagPointer( GLsizei stride, GLvoid* pointer ) {
  TRACE("(%d, %p)\n", stride, pointer );
  glEdgeFlagPointer( stride, pointer );
}

/***********************************************************************
 *              glEdgeFlagv (OPENGL32.@)
 */
void WINAPI wine_glEdgeFlagv( GLboolean* flag ) {
  TRACE("(%p)\n", flag );
  glEdgeFlagv( flag );
}

/***********************************************************************
 *              glEnable (OPENGL32.@)
 */
void WINAPI wine_glEnable( GLenum cap ) {
  TRACE("(%d)\n", cap );
  glEnable( cap );
}

/***********************************************************************
 *              glEnableClientState (OPENGL32.@)
 */
void WINAPI wine_glEnableClientState( GLenum array ) {
  TRACE("(%d)\n", array );
  glEnableClientState( array );
}

/***********************************************************************
 *              glEnd (OPENGL32.@)
 */
void WINAPI wine_glEnd( void ) {
  TRACE("()\n");
  glEnd( );
}

/***********************************************************************
 *              glEndList (OPENGL32.@)
 */
void WINAPI wine_glEndList( void ) {
  TRACE("()\n");
  glEndList( );
}

/***********************************************************************
 *              glEvalCoord1d (OPENGL32.@)
 */
void WINAPI wine_glEvalCoord1d( GLdouble u ) {
  TRACE("(%f)\n", u );
  glEvalCoord1d( u );
}

/***********************************************************************
 *              glEvalCoord1dv (OPENGL32.@)
 */
void WINAPI wine_glEvalCoord1dv( GLdouble* u ) {
  TRACE("(%p)\n", u );
  glEvalCoord1dv( u );
}

/***********************************************************************
 *              glEvalCoord1f (OPENGL32.@)
 */
void WINAPI wine_glEvalCoord1f( GLfloat u ) {
  TRACE("(%f)\n", u );
  glEvalCoord1f( u );
}

/***********************************************************************
 *              glEvalCoord1fv (OPENGL32.@)
 */
void WINAPI wine_glEvalCoord1fv( GLfloat* u ) {
  TRACE("(%p)\n", u );
  glEvalCoord1fv( u );
}

/***********************************************************************
 *              glEvalCoord2d (OPENGL32.@)
 */
void WINAPI wine_glEvalCoord2d( GLdouble u, GLdouble v ) {
  TRACE("(%f, %f)\n", u, v );
  glEvalCoord2d( u, v );
}

/***********************************************************************
 *              glEvalCoord2dv (OPENGL32.@)
 */
void WINAPI wine_glEvalCoord2dv( GLdouble* u ) {
  TRACE("(%p)\n", u );
  glEvalCoord2dv( u );
}

/***********************************************************************
 *              glEvalCoord2f (OPENGL32.@)
 */
void WINAPI wine_glEvalCoord2f( GLfloat u, GLfloat v ) {
  TRACE("(%f, %f)\n", u, v );
  glEvalCoord2f( u, v );
}

/***********************************************************************
 *              glEvalCoord2fv (OPENGL32.@)
 */
void WINAPI wine_glEvalCoord2fv( GLfloat* u ) {
  TRACE("(%p)\n", u );
  glEvalCoord2fv( u );
}

/***********************************************************************
 *              glEvalMesh1 (OPENGL32.@)
 */
void WINAPI wine_glEvalMesh1( GLenum mode, GLint i1, GLint i2 ) {
  TRACE("(%d, %d, %d)\n", mode, i1, i2 );
  glEvalMesh1( mode, i1, i2 );
}

/***********************************************************************
 *              glEvalMesh2 (OPENGL32.@)
 */
void WINAPI wine_glEvalMesh2( GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2 ) {
  TRACE("(%d, %d, %d, %d, %d)\n", mode, i1, i2, j1, j2 );
  glEvalMesh2( mode, i1, i2, j1, j2 );
}

/***********************************************************************
 *              glEvalPoint1 (OPENGL32.@)
 */
void WINAPI wine_glEvalPoint1( GLint i ) {
  TRACE("(%d)\n", i );
  glEvalPoint1( i );
}

/***********************************************************************
 *              glEvalPoint2 (OPENGL32.@)
 */
void WINAPI wine_glEvalPoint2( GLint i, GLint j ) {
  TRACE("(%d, %d)\n", i, j );
  glEvalPoint2( i, j );
}

/***********************************************************************
 *              glFeedbackBuffer (OPENGL32.@)
 */
void WINAPI wine_glFeedbackBuffer( GLsizei size, GLenum type, GLfloat* buffer ) {
  TRACE("(%d, %d, %p)\n", size, type, buffer );
  glFeedbackBuffer( size, type, buffer );
}

/***********************************************************************
 *              glFogf (OPENGL32.@)
 */
void WINAPI wine_glFogf( GLenum pname, GLfloat param ) {
  TRACE("(%d, %f)\n", pname, param );
  glFogf( pname, param );
}

/***********************************************************************
 *              glFogfv (OPENGL32.@)
 */
void WINAPI wine_glFogfv( GLenum pname, GLfloat* params ) {
  TRACE("(%d, %p)\n", pname, params );
  glFogfv( pname, params );
}

/***********************************************************************
 *              glFogi (OPENGL32.@)
 */
void WINAPI wine_glFogi( GLenum pname, GLint param ) {
  TRACE("(%d, %d)\n", pname, param );
  glFogi( pname, param );
}

/***********************************************************************
 *              glFogiv (OPENGL32.@)
 */
void WINAPI wine_glFogiv( GLenum pname, GLint* params ) {
  TRACE("(%d, %p)\n", pname, params );
  glFogiv( pname, params );
}

/***********************************************************************
 *              glFrontFace (OPENGL32.@)
 */
void WINAPI wine_glFrontFace( GLenum mode ) {
  TRACE("(%d)\n", mode );
  glFrontFace( mode );
}

/***********************************************************************
 *              glFrustum (OPENGL32.@)
 */
void WINAPI wine_glFrustum( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar ) {
  TRACE("(%f, %f, %f, %f, %f, %f)\n", left, right, bottom, top, zNear, zFar );
  glFrustum( left, right, bottom, top, zNear, zFar );
}

/***********************************************************************
 *              glGenLists (OPENGL32.@)
 */
GLuint WINAPI wine_glGenLists( GLsizei range ) {
  TRACE("(%d)\n", range );
  return glGenLists( range );
}

/***********************************************************************
 *              glGenTextures (OPENGL32.@)
 */
void WINAPI wine_glGenTextures( GLsizei n, GLuint* textures ) {
  TRACE("(%d, %p)\n", n, textures );
  glGenTextures( n, textures );
}

/***********************************************************************
 *              glGetBooleanv (OPENGL32.@)
 */
void WINAPI wine_glGetBooleanv( GLenum pname, GLboolean* params ) {
  TRACE("(%d, %p)\n", pname, params );
  glGetBooleanv( pname, params );
}

/***********************************************************************
 *              glGetClipPlane (OPENGL32.@)
 */
void WINAPI wine_glGetClipPlane( GLenum plane, GLdouble* equation ) {
  TRACE("(%d, %p)\n", plane, equation );
  glGetClipPlane( plane, equation );
}

/***********************************************************************
 *              glGetDoublev (OPENGL32.@)
 */
void WINAPI wine_glGetDoublev( GLenum pname, GLdouble* params ) {
  TRACE("(%d, %p)\n", pname, params );
  glGetDoublev( pname, params );
}

/***********************************************************************
 *              glGetError (OPENGL32.@)
 */
GLenum WINAPI wine_glGetError( void ) {
  TRACE("()\n");
  return glGetError( );
}

/***********************************************************************
 *              glGetFloatv (OPENGL32.@)
 */
void WINAPI wine_glGetFloatv( GLenum pname, GLfloat* params ) {
  TRACE("(%d, %p)\n", pname, params );
  glGetFloatv( pname, params );
}

/***********************************************************************
 *              glGetLightfv (OPENGL32.@)
 */
void WINAPI wine_glGetLightfv( GLenum light, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", light, pname, params );
  glGetLightfv( light, pname, params );
}

/***********************************************************************
 *              glGetLightiv (OPENGL32.@)
 */
void WINAPI wine_glGetLightiv( GLenum light, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", light, pname, params );
  glGetLightiv( light, pname, params );
}

/***********************************************************************
 *              glGetMapdv (OPENGL32.@)
 */
void WINAPI wine_glGetMapdv( GLenum target, GLenum query, GLdouble* v ) {
  TRACE("(%d, %d, %p)\n", target, query, v );
  glGetMapdv( target, query, v );
}

/***********************************************************************
 *              glGetMapfv (OPENGL32.@)
 */
void WINAPI wine_glGetMapfv( GLenum target, GLenum query, GLfloat* v ) {
  TRACE("(%d, %d, %p)\n", target, query, v );
  glGetMapfv( target, query, v );
}

/***********************************************************************
 *              glGetMapiv (OPENGL32.@)
 */
void WINAPI wine_glGetMapiv( GLenum target, GLenum query, GLint* v ) {
  TRACE("(%d, %d, %p)\n", target, query, v );
  glGetMapiv( target, query, v );
}

/***********************************************************************
 *              glGetMaterialfv (OPENGL32.@)
 */
void WINAPI wine_glGetMaterialfv( GLenum face, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", face, pname, params );
  glGetMaterialfv( face, pname, params );
}

/***********************************************************************
 *              glGetMaterialiv (OPENGL32.@)
 */
void WINAPI wine_glGetMaterialiv( GLenum face, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", face, pname, params );
  glGetMaterialiv( face, pname, params );
}

/***********************************************************************
 *              glGetPixelMapfv (OPENGL32.@)
 */
void WINAPI wine_glGetPixelMapfv( GLenum map, GLfloat* values ) {
  TRACE("(%d, %p)\n", map, values );
  glGetPixelMapfv( map, values );
}

/***********************************************************************
 *              glGetPixelMapuiv (OPENGL32.@)
 */
void WINAPI wine_glGetPixelMapuiv( GLenum map, GLuint* values ) {
  TRACE("(%d, %p)\n", map, values );
  glGetPixelMapuiv( map, values );
}

/***********************************************************************
 *              glGetPixelMapusv (OPENGL32.@)
 */
void WINAPI wine_glGetPixelMapusv( GLenum map, GLushort* values ) {
  TRACE("(%d, %p)\n", map, values );
  glGetPixelMapusv( map, values );
}

/***********************************************************************
 *              glGetPointerv (OPENGL32.@)
 */
void WINAPI wine_glGetPointerv( GLenum pname, GLvoid** params ) {
  TRACE("(%d, %p)\n", pname, params );
  glGetPointerv( pname, params );
}

/***********************************************************************
 *              glGetPolygonStipple (OPENGL32.@)
 */
void WINAPI wine_glGetPolygonStipple( GLubyte* mask ) {
  TRACE("(%p)\n", mask );
  glGetPolygonStipple( mask );
}

/***********************************************************************
 *              glGetTexEnvfv (OPENGL32.@)
 */
void WINAPI wine_glGetTexEnvfv( GLenum target, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  glGetTexEnvfv( target, pname, params );
}

/***********************************************************************
 *              glGetTexEnviv (OPENGL32.@)
 */
void WINAPI wine_glGetTexEnviv( GLenum target, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  glGetTexEnviv( target, pname, params );
}

/***********************************************************************
 *              glGetTexGendv (OPENGL32.@)
 */
void WINAPI wine_glGetTexGendv( GLenum coord, GLenum pname, GLdouble* params ) {
  TRACE("(%d, %d, %p)\n", coord, pname, params );
  glGetTexGendv( coord, pname, params );
}

/***********************************************************************
 *              glGetTexGenfv (OPENGL32.@)
 */
void WINAPI wine_glGetTexGenfv( GLenum coord, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", coord, pname, params );
  glGetTexGenfv( coord, pname, params );
}

/***********************************************************************
 *              glGetTexGeniv (OPENGL32.@)
 */
void WINAPI wine_glGetTexGeniv( GLenum coord, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", coord, pname, params );
  glGetTexGeniv( coord, pname, params );
}

/***********************************************************************
 *              glGetTexImage (OPENGL32.@)
 */
void WINAPI wine_glGetTexImage( GLenum target, GLint level, GLenum format, GLenum type, GLvoid* pixels ) {
  TRACE("(%d, %d, %d, %d, %p)\n", target, level, format, type, pixels );
  glGetTexImage( target, level, format, type, pixels );
}

/***********************************************************************
 *              glGetTexLevelParameterfv (OPENGL32.@)
 */
void WINAPI wine_glGetTexLevelParameterfv( GLenum target, GLint level, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %d, %p)\n", target, level, pname, params );
  glGetTexLevelParameterfv( target, level, pname, params );
}

/***********************************************************************
 *              glGetTexLevelParameteriv (OPENGL32.@)
 */
void WINAPI wine_glGetTexLevelParameteriv( GLenum target, GLint level, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %d, %p)\n", target, level, pname, params );
  glGetTexLevelParameteriv( target, level, pname, params );
}

/***********************************************************************
 *              glGetTexParameterfv (OPENGL32.@)
 */
void WINAPI wine_glGetTexParameterfv( GLenum target, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  glGetTexParameterfv( target, pname, params );
}

/***********************************************************************
 *              glGetTexParameteriv (OPENGL32.@)
 */
void WINAPI wine_glGetTexParameteriv( GLenum target, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  glGetTexParameteriv( target, pname, params );
}

/***********************************************************************
 *              glHint (OPENGL32.@)
 */
void WINAPI wine_glHint( GLenum target, GLenum mode ) {
  TRACE("(%d, %d)\n", target, mode );
  glHint( target, mode );
}

/***********************************************************************
 *              glIndexMask (OPENGL32.@)
 */
void WINAPI wine_glIndexMask( GLuint mask ) {
  TRACE("(%d)\n", mask );
  glIndexMask( mask );
}

/***********************************************************************
 *              glIndexPointer (OPENGL32.@)
 */
void WINAPI wine_glIndexPointer( GLenum type, GLsizei stride, GLvoid* pointer ) {
  TRACE("(%d, %d, %p)\n", type, stride, pointer );
  glIndexPointer( type, stride, pointer );
}

/***********************************************************************
 *              glIndexd (OPENGL32.@)
 */
void WINAPI wine_glIndexd( GLdouble c ) {
  TRACE("(%f)\n", c );
  glIndexd( c );
}

/***********************************************************************
 *              glIndexdv (OPENGL32.@)
 */
void WINAPI wine_glIndexdv( GLdouble* c ) {
  TRACE("(%p)\n", c );
  glIndexdv( c );
}

/***********************************************************************
 *              glIndexf (OPENGL32.@)
 */
void WINAPI wine_glIndexf( GLfloat c ) {
  TRACE("(%f)\n", c );
  glIndexf( c );
}

/***********************************************************************
 *              glIndexfv (OPENGL32.@)
 */
void WINAPI wine_glIndexfv( GLfloat* c ) {
  TRACE("(%p)\n", c );
  glIndexfv( c );
}

/***********************************************************************
 *              glIndexi (OPENGL32.@)
 */
void WINAPI wine_glIndexi( GLint c ) {
  TRACE("(%d)\n", c );
  glIndexi( c );
}

/***********************************************************************
 *              glIndexiv (OPENGL32.@)
 */
void WINAPI wine_glIndexiv( GLint* c ) {
  TRACE("(%p)\n", c );
  glIndexiv( c );
}

/***********************************************************************
 *              glIndexs (OPENGL32.@)
 */
void WINAPI wine_glIndexs( GLshort c ) {
  TRACE("(%d)\n", c );
  glIndexs( c );
}

/***********************************************************************
 *              glIndexsv (OPENGL32.@)
 */
void WINAPI wine_glIndexsv( GLshort* c ) {
  TRACE("(%p)\n", c );
  glIndexsv( c );
}

/***********************************************************************
 *              glIndexub (OPENGL32.@)
 */
void WINAPI wine_glIndexub( GLubyte c ) {
  TRACE("(%d)\n", c );
  glIndexub( c );
}

/***********************************************************************
 *              glIndexubv (OPENGL32.@)
 */
void WINAPI wine_glIndexubv( GLubyte* c ) {
  TRACE("(%p)\n", c );
  glIndexubv( c );
}

/***********************************************************************
 *              glInitNames (OPENGL32.@)
 */
void WINAPI wine_glInitNames( void ) {
  TRACE("()\n");
  glInitNames( );
}

/***********************************************************************
 *              glInterleavedArrays (OPENGL32.@)
 */
void WINAPI wine_glInterleavedArrays( GLenum format, GLsizei stride, GLvoid* pointer ) {
  TRACE("(%d, %d, %p)\n", format, stride, pointer );
  glInterleavedArrays( format, stride, pointer );
}

/***********************************************************************
 *              glIsEnabled (OPENGL32.@)
 */
GLboolean WINAPI wine_glIsEnabled( GLenum cap ) {
  TRACE("(%d)\n", cap );
  return glIsEnabled( cap );
}

/***********************************************************************
 *              glIsList (OPENGL32.@)
 */
GLboolean WINAPI wine_glIsList( GLuint list ) {
  TRACE("(%d)\n", list );
  return glIsList( list );
}

/***********************************************************************
 *              glIsTexture (OPENGL32.@)
 */
GLboolean WINAPI wine_glIsTexture( GLuint texture ) {
  TRACE("(%d)\n", texture );
  return glIsTexture( texture );
}

/***********************************************************************
 *              glLightModelf (OPENGL32.@)
 */
void WINAPI wine_glLightModelf( GLenum pname, GLfloat param ) {
  TRACE("(%d, %f)\n", pname, param );
  glLightModelf( pname, param );
}

/***********************************************************************
 *              glLightModelfv (OPENGL32.@)
 */
void WINAPI wine_glLightModelfv( GLenum pname, GLfloat* params ) {
  TRACE("(%d, %p)\n", pname, params );
  glLightModelfv( pname, params );
}

/***********************************************************************
 *              glLightModeli (OPENGL32.@)
 */
void WINAPI wine_glLightModeli( GLenum pname, GLint param ) {
  TRACE("(%d, %d)\n", pname, param );
  glLightModeli( pname, param );
}

/***********************************************************************
 *              glLightModeliv (OPENGL32.@)
 */
void WINAPI wine_glLightModeliv( GLenum pname, GLint* params ) {
  TRACE("(%d, %p)\n", pname, params );
  glLightModeliv( pname, params );
}

/***********************************************************************
 *              glLightf (OPENGL32.@)
 */
void WINAPI wine_glLightf( GLenum light, GLenum pname, GLfloat param ) {
  TRACE("(%d, %d, %f)\n", light, pname, param );
  glLightf( light, pname, param );
}

/***********************************************************************
 *              glLightfv (OPENGL32.@)
 */
void WINAPI wine_glLightfv( GLenum light, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", light, pname, params );
  glLightfv( light, pname, params );
}

/***********************************************************************
 *              glLighti (OPENGL32.@)
 */
void WINAPI wine_glLighti( GLenum light, GLenum pname, GLint param ) {
  TRACE("(%d, %d, %d)\n", light, pname, param );
  glLighti( light, pname, param );
}

/***********************************************************************
 *              glLightiv (OPENGL32.@)
 */
void WINAPI wine_glLightiv( GLenum light, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", light, pname, params );
  glLightiv( light, pname, params );
}

/***********************************************************************
 *              glLineStipple (OPENGL32.@)
 */
void WINAPI wine_glLineStipple( GLint factor, GLushort pattern ) {
  TRACE("(%d, %d)\n", factor, pattern );
  glLineStipple( factor, pattern );
}

/***********************************************************************
 *              glLineWidth (OPENGL32.@)
 */
void WINAPI wine_glLineWidth( GLfloat width ) {
  TRACE("(%f)\n", width );
  glLineWidth( width );
}

/***********************************************************************
 *              glListBase (OPENGL32.@)
 */
void WINAPI wine_glListBase( GLuint base ) {
  TRACE("(%d)\n", base );
  glListBase( base );
}

/***********************************************************************
 *              glLoadIdentity (OPENGL32.@)
 */
void WINAPI wine_glLoadIdentity( void ) {
  TRACE("()\n");
  glLoadIdentity( );
}

/***********************************************************************
 *              glLoadMatrixd (OPENGL32.@)
 */
void WINAPI wine_glLoadMatrixd( GLdouble* m ) {
  TRACE("(%p)\n", m );
  glLoadMatrixd( m );
}

/***********************************************************************
 *              glLoadMatrixf (OPENGL32.@)
 */
void WINAPI wine_glLoadMatrixf( GLfloat* m ) {
  TRACE("(%p)\n", m );
  glLoadMatrixf( m );
}

/***********************************************************************
 *              glLoadName (OPENGL32.@)
 */
void WINAPI wine_glLoadName( GLuint name ) {
  TRACE("(%d)\n", name );
  glLoadName( name );
}

/***********************************************************************
 *              glLogicOp (OPENGL32.@)
 */
void WINAPI wine_glLogicOp( GLenum opcode ) {
  TRACE("(%d)\n", opcode );
  glLogicOp( opcode );
}

/***********************************************************************
 *              glMap1d (OPENGL32.@)
 */
void WINAPI wine_glMap1d( GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, GLdouble* points ) {
  TRACE("(%d, %f, %f, %d, %d, %p)\n", target, u1, u2, stride, order, points );
  glMap1d( target, u1, u2, stride, order, points );
}

/***********************************************************************
 *              glMap1f (OPENGL32.@)
 */
void WINAPI wine_glMap1f( GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, GLfloat* points ) {
  TRACE("(%d, %f, %f, %d, %d, %p)\n", target, u1, u2, stride, order, points );
  glMap1f( target, u1, u2, stride, order, points );
}

/***********************************************************************
 *              glMap2d (OPENGL32.@)
 */
void WINAPI wine_glMap2d( GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, GLdouble* points ) {
  TRACE("(%d, %f, %f, %d, %d, %f, %f, %d, %d, %p)\n", target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points );
  glMap2d( target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points );
}

/***********************************************************************
 *              glMap2f (OPENGL32.@)
 */
void WINAPI wine_glMap2f( GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, GLfloat* points ) {
  TRACE("(%d, %f, %f, %d, %d, %f, %f, %d, %d, %p)\n", target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points );
  glMap2f( target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points );
}

/***********************************************************************
 *              glMapGrid1d (OPENGL32.@)
 */
void WINAPI wine_glMapGrid1d( GLint un, GLdouble u1, GLdouble u2 ) {
  TRACE("(%d, %f, %f)\n", un, u1, u2 );
  glMapGrid1d( un, u1, u2 );
}

/***********************************************************************
 *              glMapGrid1f (OPENGL32.@)
 */
void WINAPI wine_glMapGrid1f( GLint un, GLfloat u1, GLfloat u2 ) {
  TRACE("(%d, %f, %f)\n", un, u1, u2 );
  glMapGrid1f( un, u1, u2 );
}

/***********************************************************************
 *              glMapGrid2d (OPENGL32.@)
 */
void WINAPI wine_glMapGrid2d( GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2 ) {
  TRACE("(%d, %f, %f, %d, %f, %f)\n", un, u1, u2, vn, v1, v2 );
  glMapGrid2d( un, u1, u2, vn, v1, v2 );
}

/***********************************************************************
 *              glMapGrid2f (OPENGL32.@)
 */
void WINAPI wine_glMapGrid2f( GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2 ) {
  TRACE("(%d, %f, %f, %d, %f, %f)\n", un, u1, u2, vn, v1, v2 );
  glMapGrid2f( un, u1, u2, vn, v1, v2 );
}

/***********************************************************************
 *              glMaterialf (OPENGL32.@)
 */
void WINAPI wine_glMaterialf( GLenum face, GLenum pname, GLfloat param ) {
  TRACE("(%d, %d, %f)\n", face, pname, param );
  glMaterialf( face, pname, param );
}

/***********************************************************************
 *              glMaterialfv (OPENGL32.@)
 */
void WINAPI wine_glMaterialfv( GLenum face, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", face, pname, params );
  glMaterialfv( face, pname, params );
}

/***********************************************************************
 *              glMateriali (OPENGL32.@)
 */
void WINAPI wine_glMateriali( GLenum face, GLenum pname, GLint param ) {
  TRACE("(%d, %d, %d)\n", face, pname, param );
  glMateriali( face, pname, param );
}

/***********************************************************************
 *              glMaterialiv (OPENGL32.@)
 */
void WINAPI wine_glMaterialiv( GLenum face, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", face, pname, params );
  glMaterialiv( face, pname, params );
}

/***********************************************************************
 *              glMatrixMode (OPENGL32.@)
 */
void WINAPI wine_glMatrixMode( GLenum mode ) {
  TRACE("(%d)\n", mode );
  glMatrixMode( mode );
}

/***********************************************************************
 *              glMultMatrixd (OPENGL32.@)
 */
void WINAPI wine_glMultMatrixd( GLdouble* m ) {
  TRACE("(%p)\n", m );
  glMultMatrixd( m );
}

/***********************************************************************
 *              glMultMatrixf (OPENGL32.@)
 */
void WINAPI wine_glMultMatrixf( GLfloat* m ) {
  TRACE("(%p)\n", m );
  glMultMatrixf( m );
}

/***********************************************************************
 *              glNewList (OPENGL32.@)
 */
void WINAPI wine_glNewList( GLuint list, GLenum mode ) {
  TRACE("(%d, %d)\n", list, mode );
  glNewList( list, mode );
}

/***********************************************************************
 *              glNormal3b (OPENGL32.@)
 */
void WINAPI wine_glNormal3b( GLbyte nx, GLbyte ny, GLbyte nz ) {
  TRACE("(%d, %d, %d)\n", nx, ny, nz );
  glNormal3b( nx, ny, nz );
}

/***********************************************************************
 *              glNormal3bv (OPENGL32.@)
 */
void WINAPI wine_glNormal3bv( GLbyte* v ) {
  TRACE("(%p)\n", v );
  glNormal3bv( v );
}

/***********************************************************************
 *              glNormal3d (OPENGL32.@)
 */
void WINAPI wine_glNormal3d( GLdouble nx, GLdouble ny, GLdouble nz ) {
  TRACE("(%f, %f, %f)\n", nx, ny, nz );
  glNormal3d( nx, ny, nz );
}

/***********************************************************************
 *              glNormal3dv (OPENGL32.@)
 */
void WINAPI wine_glNormal3dv( GLdouble* v ) {
  TRACE("(%p)\n", v );
  glNormal3dv( v );
}

/***********************************************************************
 *              glNormal3f (OPENGL32.@)
 */
void WINAPI wine_glNormal3f( GLfloat nx, GLfloat ny, GLfloat nz ) {
  TRACE("(%f, %f, %f)\n", nx, ny, nz );
  glNormal3f( nx, ny, nz );
}

/***********************************************************************
 *              glNormal3fv (OPENGL32.@)
 */
void WINAPI wine_glNormal3fv( GLfloat* v ) {
  TRACE("(%p)\n", v );
  glNormal3fv( v );
}

/***********************************************************************
 *              glNormal3i (OPENGL32.@)
 */
void WINAPI wine_glNormal3i( GLint nx, GLint ny, GLint nz ) {
  TRACE("(%d, %d, %d)\n", nx, ny, nz );
  glNormal3i( nx, ny, nz );
}

/***********************************************************************
 *              glNormal3iv (OPENGL32.@)
 */
void WINAPI wine_glNormal3iv( GLint* v ) {
  TRACE("(%p)\n", v );
  glNormal3iv( v );
}

/***********************************************************************
 *              glNormal3s (OPENGL32.@)
 */
void WINAPI wine_glNormal3s( GLshort nx, GLshort ny, GLshort nz ) {
  TRACE("(%d, %d, %d)\n", nx, ny, nz );
  glNormal3s( nx, ny, nz );
}

/***********************************************************************
 *              glNormal3sv (OPENGL32.@)
 */
void WINAPI wine_glNormal3sv( GLshort* v ) {
  TRACE("(%p)\n", v );
  glNormal3sv( v );
}

/***********************************************************************
 *              glNormalPointer (OPENGL32.@)
 */
void WINAPI wine_glNormalPointer( GLenum type, GLsizei stride, GLvoid* pointer ) {
  TRACE("(%d, %d, %p)\n", type, stride, pointer );
  glNormalPointer( type, stride, pointer );
}

/***********************************************************************
 *              glOrtho (OPENGL32.@)
 */
void WINAPI wine_glOrtho( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar ) {
  TRACE("(%f, %f, %f, %f, %f, %f)\n", left, right, bottom, top, zNear, zFar );
  glOrtho( left, right, bottom, top, zNear, zFar );
}

/***********************************************************************
 *              glPassThrough (OPENGL32.@)
 */
void WINAPI wine_glPassThrough( GLfloat token ) {
  TRACE("(%f)\n", token );
  glPassThrough( token );
}

/***********************************************************************
 *              glPixelMapfv (OPENGL32.@)
 */
void WINAPI wine_glPixelMapfv( GLenum map, GLint mapsize, GLfloat* values ) {
  TRACE("(%d, %d, %p)\n", map, mapsize, values );
  glPixelMapfv( map, mapsize, values );
}

/***********************************************************************
 *              glPixelMapuiv (OPENGL32.@)
 */
void WINAPI wine_glPixelMapuiv( GLenum map, GLint mapsize, GLuint* values ) {
  TRACE("(%d, %d, %p)\n", map, mapsize, values );
  glPixelMapuiv( map, mapsize, values );
}

/***********************************************************************
 *              glPixelMapusv (OPENGL32.@)
 */
void WINAPI wine_glPixelMapusv( GLenum map, GLint mapsize, GLushort* values ) {
  TRACE("(%d, %d, %p)\n", map, mapsize, values );
  glPixelMapusv( map, mapsize, values );
}

/***********************************************************************
 *              glPixelStoref (OPENGL32.@)
 */
void WINAPI wine_glPixelStoref( GLenum pname, GLfloat param ) {
  TRACE("(%d, %f)\n", pname, param );
  glPixelStoref( pname, param );
}

/***********************************************************************
 *              glPixelStorei (OPENGL32.@)
 */
void WINAPI wine_glPixelStorei( GLenum pname, GLint param ) {
  TRACE("(%d, %d)\n", pname, param );
  glPixelStorei( pname, param );
}

/***********************************************************************
 *              glPixelTransferf (OPENGL32.@)
 */
void WINAPI wine_glPixelTransferf( GLenum pname, GLfloat param ) {
  TRACE("(%d, %f)\n", pname, param );
  glPixelTransferf( pname, param );
}

/***********************************************************************
 *              glPixelTransferi (OPENGL32.@)
 */
void WINAPI wine_glPixelTransferi( GLenum pname, GLint param ) {
  TRACE("(%d, %d)\n", pname, param );
  glPixelTransferi( pname, param );
}

/***********************************************************************
 *              glPixelZoom (OPENGL32.@)
 */
void WINAPI wine_glPixelZoom( GLfloat xfactor, GLfloat yfactor ) {
  TRACE("(%f, %f)\n", xfactor, yfactor );
  glPixelZoom( xfactor, yfactor );
}

/***********************************************************************
 *              glPointSize (OPENGL32.@)
 */
void WINAPI wine_glPointSize( GLfloat size ) {
  TRACE("(%f)\n", size );
  glPointSize( size );
}

/***********************************************************************
 *              glPolygonMode (OPENGL32.@)
 */
void WINAPI wine_glPolygonMode( GLenum face, GLenum mode ) {
  TRACE("(%d, %d)\n", face, mode );
  glPolygonMode( face, mode );
}

/***********************************************************************
 *              glPolygonOffset (OPENGL32.@)
 */
void WINAPI wine_glPolygonOffset( GLfloat factor, GLfloat units ) {
  TRACE("(%f, %f)\n", factor, units );
  glPolygonOffset( factor, units );
}

/***********************************************************************
 *              glPolygonStipple (OPENGL32.@)
 */
void WINAPI wine_glPolygonStipple( GLubyte* mask ) {
  TRACE("(%p)\n", mask );
  glPolygonStipple( mask );
}

/***********************************************************************
 *              glPopAttrib (OPENGL32.@)
 */
void WINAPI wine_glPopAttrib( void ) {
  TRACE("()\n");
  glPopAttrib( );
}

/***********************************************************************
 *              glPopClientAttrib (OPENGL32.@)
 */
void WINAPI wine_glPopClientAttrib( void ) {
  TRACE("()\n");
  glPopClientAttrib( );
}

/***********************************************************************
 *              glPopMatrix (OPENGL32.@)
 */
void WINAPI wine_glPopMatrix( void ) {
  TRACE("()\n");
  glPopMatrix( );
}

/***********************************************************************
 *              glPopName (OPENGL32.@)
 */
void WINAPI wine_glPopName( void ) {
  TRACE("()\n");
  glPopName( );
}

/***********************************************************************
 *              glPrioritizeTextures (OPENGL32.@)
 */
void WINAPI wine_glPrioritizeTextures( GLsizei n, GLuint* textures, GLfloat* priorities ) {
  TRACE("(%d, %p, %p)\n", n, textures, priorities );
  glPrioritizeTextures( n, textures, priorities );
}

/***********************************************************************
 *              glPushAttrib (OPENGL32.@)
 */
void WINAPI wine_glPushAttrib( GLbitfield mask ) {
  TRACE("(%d)\n", mask );
  glPushAttrib( mask );
}

/***********************************************************************
 *              glPushClientAttrib (OPENGL32.@)
 */
void WINAPI wine_glPushClientAttrib( GLbitfield mask ) {
  TRACE("(%d)\n", mask );
  glPushClientAttrib( mask );
}

/***********************************************************************
 *              glPushMatrix (OPENGL32.@)
 */
void WINAPI wine_glPushMatrix( void ) {
  TRACE("()\n");
  glPushMatrix( );
}

/***********************************************************************
 *              glPushName (OPENGL32.@)
 */
void WINAPI wine_glPushName( GLuint name ) {
  TRACE("(%d)\n", name );
  glPushName( name );
}

/***********************************************************************
 *              glRasterPos2d (OPENGL32.@)
 */
void WINAPI wine_glRasterPos2d( GLdouble x, GLdouble y ) {
  TRACE("(%f, %f)\n", x, y );
  glRasterPos2d( x, y );
}

/***********************************************************************
 *              glRasterPos2dv (OPENGL32.@)
 */
void WINAPI wine_glRasterPos2dv( GLdouble* v ) {
  TRACE("(%p)\n", v );
  glRasterPos2dv( v );
}

/***********************************************************************
 *              glRasterPos2f (OPENGL32.@)
 */
void WINAPI wine_glRasterPos2f( GLfloat x, GLfloat y ) {
  TRACE("(%f, %f)\n", x, y );
  glRasterPos2f( x, y );
}

/***********************************************************************
 *              glRasterPos2fv (OPENGL32.@)
 */
void WINAPI wine_glRasterPos2fv( GLfloat* v ) {
  TRACE("(%p)\n", v );
  glRasterPos2fv( v );
}

/***********************************************************************
 *              glRasterPos2i (OPENGL32.@)
 */
void WINAPI wine_glRasterPos2i( GLint x, GLint y ) {
  TRACE("(%d, %d)\n", x, y );
  glRasterPos2i( x, y );
}

/***********************************************************************
 *              glRasterPos2iv (OPENGL32.@)
 */
void WINAPI wine_glRasterPos2iv( GLint* v ) {
  TRACE("(%p)\n", v );
  glRasterPos2iv( v );
}

/***********************************************************************
 *              glRasterPos2s (OPENGL32.@)
 */
void WINAPI wine_glRasterPos2s( GLshort x, GLshort y ) {
  TRACE("(%d, %d)\n", x, y );
  glRasterPos2s( x, y );
}

/***********************************************************************
 *              glRasterPos2sv (OPENGL32.@)
 */
void WINAPI wine_glRasterPos2sv( GLshort* v ) {
  TRACE("(%p)\n", v );
  glRasterPos2sv( v );
}

/***********************************************************************
 *              glRasterPos3d (OPENGL32.@)
 */
void WINAPI wine_glRasterPos3d( GLdouble x, GLdouble y, GLdouble z ) {
  TRACE("(%f, %f, %f)\n", x, y, z );
  glRasterPos3d( x, y, z );
}

/***********************************************************************
 *              glRasterPos3dv (OPENGL32.@)
 */
void WINAPI wine_glRasterPos3dv( GLdouble* v ) {
  TRACE("(%p)\n", v );
  glRasterPos3dv( v );
}

/***********************************************************************
 *              glRasterPos3f (OPENGL32.@)
 */
void WINAPI wine_glRasterPos3f( GLfloat x, GLfloat y, GLfloat z ) {
  TRACE("(%f, %f, %f)\n", x, y, z );
  glRasterPos3f( x, y, z );
}

/***********************************************************************
 *              glRasterPos3fv (OPENGL32.@)
 */
void WINAPI wine_glRasterPos3fv( GLfloat* v ) {
  TRACE("(%p)\n", v );
  glRasterPos3fv( v );
}

/***********************************************************************
 *              glRasterPos3i (OPENGL32.@)
 */
void WINAPI wine_glRasterPos3i( GLint x, GLint y, GLint z ) {
  TRACE("(%d, %d, %d)\n", x, y, z );
  glRasterPos3i( x, y, z );
}

/***********************************************************************
 *              glRasterPos3iv (OPENGL32.@)
 */
void WINAPI wine_glRasterPos3iv( GLint* v ) {
  TRACE("(%p)\n", v );
  glRasterPos3iv( v );
}

/***********************************************************************
 *              glRasterPos3s (OPENGL32.@)
 */
void WINAPI wine_glRasterPos3s( GLshort x, GLshort y, GLshort z ) {
  TRACE("(%d, %d, %d)\n", x, y, z );
  glRasterPos3s( x, y, z );
}

/***********************************************************************
 *              glRasterPos3sv (OPENGL32.@)
 */
void WINAPI wine_glRasterPos3sv( GLshort* v ) {
  TRACE("(%p)\n", v );
  glRasterPos3sv( v );
}

/***********************************************************************
 *              glRasterPos4d (OPENGL32.@)
 */
void WINAPI wine_glRasterPos4d( GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  TRACE("(%f, %f, %f, %f)\n", x, y, z, w );
  glRasterPos4d( x, y, z, w );
}

/***********************************************************************
 *              glRasterPos4dv (OPENGL32.@)
 */
void WINAPI wine_glRasterPos4dv( GLdouble* v ) {
  TRACE("(%p)\n", v );
  glRasterPos4dv( v );
}

/***********************************************************************
 *              glRasterPos4f (OPENGL32.@)
 */
void WINAPI wine_glRasterPos4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  TRACE("(%f, %f, %f, %f)\n", x, y, z, w );
  glRasterPos4f( x, y, z, w );
}

/***********************************************************************
 *              glRasterPos4fv (OPENGL32.@)
 */
void WINAPI wine_glRasterPos4fv( GLfloat* v ) {
  TRACE("(%p)\n", v );
  glRasterPos4fv( v );
}

/***********************************************************************
 *              glRasterPos4i (OPENGL32.@)
 */
void WINAPI wine_glRasterPos4i( GLint x, GLint y, GLint z, GLint w ) {
  TRACE("(%d, %d, %d, %d)\n", x, y, z, w );
  glRasterPos4i( x, y, z, w );
}

/***********************************************************************
 *              glRasterPos4iv (OPENGL32.@)
 */
void WINAPI wine_glRasterPos4iv( GLint* v ) {
  TRACE("(%p)\n", v );
  glRasterPos4iv( v );
}

/***********************************************************************
 *              glRasterPos4s (OPENGL32.@)
 */
void WINAPI wine_glRasterPos4s( GLshort x, GLshort y, GLshort z, GLshort w ) {
  TRACE("(%d, %d, %d, %d)\n", x, y, z, w );
  glRasterPos4s( x, y, z, w );
}

/***********************************************************************
 *              glRasterPos4sv (OPENGL32.@)
 */
void WINAPI wine_glRasterPos4sv( GLshort* v ) {
  TRACE("(%p)\n", v );
  glRasterPos4sv( v );
}

/***********************************************************************
 *              glReadBuffer (OPENGL32.@)
 */
void WINAPI wine_glReadBuffer( GLenum mode ) {
  TRACE("(%d)\n", mode );
  glReadBuffer( mode );
}

/***********************************************************************
 *              glReadPixels (OPENGL32.@)
 */
void WINAPI wine_glReadPixels( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %p)\n", x, y, width, height, format, type, pixels );
  glReadPixels( x, y, width, height, format, type, pixels );
}

/***********************************************************************
 *              glRectd (OPENGL32.@)
 */
void WINAPI wine_glRectd( GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2 ) {
  TRACE("(%f, %f, %f, %f)\n", x1, y1, x2, y2 );
  glRectd( x1, y1, x2, y2 );
}

/***********************************************************************
 *              glRectdv (OPENGL32.@)
 */
void WINAPI wine_glRectdv( GLdouble* v1, GLdouble* v2 ) {
  TRACE("(%p, %p)\n", v1, v2 );
  glRectdv( v1, v2 );
}

/***********************************************************************
 *              glRectf (OPENGL32.@)
 */
void WINAPI wine_glRectf( GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 ) {
  TRACE("(%f, %f, %f, %f)\n", x1, y1, x2, y2 );
  glRectf( x1, y1, x2, y2 );
}

/***********************************************************************
 *              glRectfv (OPENGL32.@)
 */
void WINAPI wine_glRectfv( GLfloat* v1, GLfloat* v2 ) {
  TRACE("(%p, %p)\n", v1, v2 );
  glRectfv( v1, v2 );
}

/***********************************************************************
 *              glRecti (OPENGL32.@)
 */
void WINAPI wine_glRecti( GLint x1, GLint y1, GLint x2, GLint y2 ) {
  TRACE("(%d, %d, %d, %d)\n", x1, y1, x2, y2 );
  glRecti( x1, y1, x2, y2 );
}

/***********************************************************************
 *              glRectiv (OPENGL32.@)
 */
void WINAPI wine_glRectiv( GLint* v1, GLint* v2 ) {
  TRACE("(%p, %p)\n", v1, v2 );
  glRectiv( v1, v2 );
}

/***********************************************************************
 *              glRects (OPENGL32.@)
 */
void WINAPI wine_glRects( GLshort x1, GLshort y1, GLshort x2, GLshort y2 ) {
  TRACE("(%d, %d, %d, %d)\n", x1, y1, x2, y2 );
  glRects( x1, y1, x2, y2 );
}

/***********************************************************************
 *              glRectsv (OPENGL32.@)
 */
void WINAPI wine_glRectsv( GLshort* v1, GLshort* v2 ) {
  TRACE("(%p, %p)\n", v1, v2 );
  glRectsv( v1, v2 );
}

/***********************************************************************
 *              glRenderMode (OPENGL32.@)
 */
GLint WINAPI wine_glRenderMode( GLenum mode ) {
  TRACE("(%d)\n", mode );
  return glRenderMode( mode );
}

/***********************************************************************
 *              glRotated (OPENGL32.@)
 */
void WINAPI wine_glRotated( GLdouble angle, GLdouble x, GLdouble y, GLdouble z ) {
  TRACE("(%f, %f, %f, %f)\n", angle, x, y, z );
  glRotated( angle, x, y, z );
}

/***********************************************************************
 *              glRotatef (OPENGL32.@)
 */
void WINAPI wine_glRotatef( GLfloat angle, GLfloat x, GLfloat y, GLfloat z ) {
  TRACE("(%f, %f, %f, %f)\n", angle, x, y, z );
  glRotatef( angle, x, y, z );
}

/***********************************************************************
 *              glScaled (OPENGL32.@)
 */
void WINAPI wine_glScaled( GLdouble x, GLdouble y, GLdouble z ) {
  TRACE("(%f, %f, %f)\n", x, y, z );
  glScaled( x, y, z );
}

/***********************************************************************
 *              glScalef (OPENGL32.@)
 */
void WINAPI wine_glScalef( GLfloat x, GLfloat y, GLfloat z ) {
  TRACE("(%f, %f, %f)\n", x, y, z );
  glScalef( x, y, z );
}

/***********************************************************************
 *              glScissor (OPENGL32.@)
 */
void WINAPI wine_glScissor( GLint x, GLint y, GLsizei width, GLsizei height ) {
  TRACE("(%d, %d, %d, %d)\n", x, y, width, height );
  glScissor( x, y, width, height );
}

/***********************************************************************
 *              glSelectBuffer (OPENGL32.@)
 */
void WINAPI wine_glSelectBuffer( GLsizei size, GLuint* buffer ) {
  TRACE("(%d, %p)\n", size, buffer );
  glSelectBuffer( size, buffer );
}

/***********************************************************************
 *              glShadeModel (OPENGL32.@)
 */
void WINAPI wine_glShadeModel( GLenum mode ) {
  TRACE("(%d)\n", mode );
  glShadeModel( mode );
}

/***********************************************************************
 *              glStencilFunc (OPENGL32.@)
 */
void WINAPI wine_glStencilFunc( GLenum func, GLint ref, GLuint mask ) {
  TRACE("(%d, %d, %d)\n", func, ref, mask );
  glStencilFunc( func, ref, mask );
}

/***********************************************************************
 *              glStencilMask (OPENGL32.@)
 */
void WINAPI wine_glStencilMask( GLuint mask ) {
  TRACE("(%d)\n", mask );
  glStencilMask( mask );
}

/***********************************************************************
 *              glStencilOp (OPENGL32.@)
 */
void WINAPI wine_glStencilOp( GLenum fail, GLenum zfail, GLenum zpass ) {
  TRACE("(%d, %d, %d)\n", fail, zfail, zpass );
  glStencilOp( fail, zfail, zpass );
}

/***********************************************************************
 *              glTexCoord1d (OPENGL32.@)
 */
void WINAPI wine_glTexCoord1d( GLdouble s ) {
  TRACE("(%f)\n", s );
  glTexCoord1d( s );
}

/***********************************************************************
 *              glTexCoord1dv (OPENGL32.@)
 */
void WINAPI wine_glTexCoord1dv( GLdouble* v ) {
  TRACE("(%p)\n", v );
  glTexCoord1dv( v );
}

/***********************************************************************
 *              glTexCoord1f (OPENGL32.@)
 */
void WINAPI wine_glTexCoord1f( GLfloat s ) {
  TRACE("(%f)\n", s );
  glTexCoord1f( s );
}

/***********************************************************************
 *              glTexCoord1fv (OPENGL32.@)
 */
void WINAPI wine_glTexCoord1fv( GLfloat* v ) {
  TRACE("(%p)\n", v );
  glTexCoord1fv( v );
}

/***********************************************************************
 *              glTexCoord1i (OPENGL32.@)
 */
void WINAPI wine_glTexCoord1i( GLint s ) {
  TRACE("(%d)\n", s );
  glTexCoord1i( s );
}

/***********************************************************************
 *              glTexCoord1iv (OPENGL32.@)
 */
void WINAPI wine_glTexCoord1iv( GLint* v ) {
  TRACE("(%p)\n", v );
  glTexCoord1iv( v );
}

/***********************************************************************
 *              glTexCoord1s (OPENGL32.@)
 */
void WINAPI wine_glTexCoord1s( GLshort s ) {
  TRACE("(%d)\n", s );
  glTexCoord1s( s );
}

/***********************************************************************
 *              glTexCoord1sv (OPENGL32.@)
 */
void WINAPI wine_glTexCoord1sv( GLshort* v ) {
  TRACE("(%p)\n", v );
  glTexCoord1sv( v );
}

/***********************************************************************
 *              glTexCoord2d (OPENGL32.@)
 */
void WINAPI wine_glTexCoord2d( GLdouble s, GLdouble t ) {
  TRACE("(%f, %f)\n", s, t );
  glTexCoord2d( s, t );
}

/***********************************************************************
 *              glTexCoord2dv (OPENGL32.@)
 */
void WINAPI wine_glTexCoord2dv( GLdouble* v ) {
  TRACE("(%p)\n", v );
  glTexCoord2dv( v );
}

/***********************************************************************
 *              glTexCoord2f (OPENGL32.@)
 */
void WINAPI wine_glTexCoord2f( GLfloat s, GLfloat t ) {
  TRACE("(%f, %f)\n", s, t );
  glTexCoord2f( s, t );
}

/***********************************************************************
 *              glTexCoord2fv (OPENGL32.@)
 */
void WINAPI wine_glTexCoord2fv( GLfloat* v ) {
  TRACE("(%p)\n", v );
  glTexCoord2fv( v );
}

/***********************************************************************
 *              glTexCoord2i (OPENGL32.@)
 */
void WINAPI wine_glTexCoord2i( GLint s, GLint t ) {
  TRACE("(%d, %d)\n", s, t );
  glTexCoord2i( s, t );
}

/***********************************************************************
 *              glTexCoord2iv (OPENGL32.@)
 */
void WINAPI wine_glTexCoord2iv( GLint* v ) {
  TRACE("(%p)\n", v );
  glTexCoord2iv( v );
}

/***********************************************************************
 *              glTexCoord2s (OPENGL32.@)
 */
void WINAPI wine_glTexCoord2s( GLshort s, GLshort t ) {
  TRACE("(%d, %d)\n", s, t );
  glTexCoord2s( s, t );
}

/***********************************************************************
 *              glTexCoord2sv (OPENGL32.@)
 */
void WINAPI wine_glTexCoord2sv( GLshort* v ) {
  TRACE("(%p)\n", v );
  glTexCoord2sv( v );
}

/***********************************************************************
 *              glTexCoord3d (OPENGL32.@)
 */
void WINAPI wine_glTexCoord3d( GLdouble s, GLdouble t, GLdouble r ) {
  TRACE("(%f, %f, %f)\n", s, t, r );
  glTexCoord3d( s, t, r );
}

/***********************************************************************
 *              glTexCoord3dv (OPENGL32.@)
 */
void WINAPI wine_glTexCoord3dv( GLdouble* v ) {
  TRACE("(%p)\n", v );
  glTexCoord3dv( v );
}

/***********************************************************************
 *              glTexCoord3f (OPENGL32.@)
 */
void WINAPI wine_glTexCoord3f( GLfloat s, GLfloat t, GLfloat r ) {
  TRACE("(%f, %f, %f)\n", s, t, r );
  glTexCoord3f( s, t, r );
}

/***********************************************************************
 *              glTexCoord3fv (OPENGL32.@)
 */
void WINAPI wine_glTexCoord3fv( GLfloat* v ) {
  TRACE("(%p)\n", v );
  glTexCoord3fv( v );
}

/***********************************************************************
 *              glTexCoord3i (OPENGL32.@)
 */
void WINAPI wine_glTexCoord3i( GLint s, GLint t, GLint r ) {
  TRACE("(%d, %d, %d)\n", s, t, r );
  glTexCoord3i( s, t, r );
}

/***********************************************************************
 *              glTexCoord3iv (OPENGL32.@)
 */
void WINAPI wine_glTexCoord3iv( GLint* v ) {
  TRACE("(%p)\n", v );
  glTexCoord3iv( v );
}

/***********************************************************************
 *              glTexCoord3s (OPENGL32.@)
 */
void WINAPI wine_glTexCoord3s( GLshort s, GLshort t, GLshort r ) {
  TRACE("(%d, %d, %d)\n", s, t, r );
  glTexCoord3s( s, t, r );
}

/***********************************************************************
 *              glTexCoord3sv (OPENGL32.@)
 */
void WINAPI wine_glTexCoord3sv( GLshort* v ) {
  TRACE("(%p)\n", v );
  glTexCoord3sv( v );
}

/***********************************************************************
 *              glTexCoord4d (OPENGL32.@)
 */
void WINAPI wine_glTexCoord4d( GLdouble s, GLdouble t, GLdouble r, GLdouble q ) {
  TRACE("(%f, %f, %f, %f)\n", s, t, r, q );
  glTexCoord4d( s, t, r, q );
}

/***********************************************************************
 *              glTexCoord4dv (OPENGL32.@)
 */
void WINAPI wine_glTexCoord4dv( GLdouble* v ) {
  TRACE("(%p)\n", v );
  glTexCoord4dv( v );
}

/***********************************************************************
 *              glTexCoord4f (OPENGL32.@)
 */
void WINAPI wine_glTexCoord4f( GLfloat s, GLfloat t, GLfloat r, GLfloat q ) {
  TRACE("(%f, %f, %f, %f)\n", s, t, r, q );
  glTexCoord4f( s, t, r, q );
}

/***********************************************************************
 *              glTexCoord4fv (OPENGL32.@)
 */
void WINAPI wine_glTexCoord4fv( GLfloat* v ) {
  TRACE("(%p)\n", v );
  glTexCoord4fv( v );
}

/***********************************************************************
 *              glTexCoord4i (OPENGL32.@)
 */
void WINAPI wine_glTexCoord4i( GLint s, GLint t, GLint r, GLint q ) {
  TRACE("(%d, %d, %d, %d)\n", s, t, r, q );
  glTexCoord4i( s, t, r, q );
}

/***********************************************************************
 *              glTexCoord4iv (OPENGL32.@)
 */
void WINAPI wine_glTexCoord4iv( GLint* v ) {
  TRACE("(%p)\n", v );
  glTexCoord4iv( v );
}

/***********************************************************************
 *              glTexCoord4s (OPENGL32.@)
 */
void WINAPI wine_glTexCoord4s( GLshort s, GLshort t, GLshort r, GLshort q ) {
  TRACE("(%d, %d, %d, %d)\n", s, t, r, q );
  glTexCoord4s( s, t, r, q );
}

/***********************************************************************
 *              glTexCoord4sv (OPENGL32.@)
 */
void WINAPI wine_glTexCoord4sv( GLshort* v ) {
  TRACE("(%p)\n", v );
  glTexCoord4sv( v );
}

/***********************************************************************
 *              glTexCoordPointer (OPENGL32.@)
 */
void WINAPI wine_glTexCoordPointer( GLint size, GLenum type, GLsizei stride, GLvoid* pointer ) {
  TRACE("(%d, %d, %d, %p)\n", size, type, stride, pointer );
  glTexCoordPointer( size, type, stride, pointer );
}

/***********************************************************************
 *              glTexEnvf (OPENGL32.@)
 */
void WINAPI wine_glTexEnvf( GLenum target, GLenum pname, GLfloat param ) {
  TRACE("(%d, %d, %f)\n", target, pname, param );
  glTexEnvf( target, pname, param );
}

/***********************************************************************
 *              glTexEnvfv (OPENGL32.@)
 */
void WINAPI wine_glTexEnvfv( GLenum target, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  glTexEnvfv( target, pname, params );
}

/***********************************************************************
 *              glTexEnvi (OPENGL32.@)
 */
void WINAPI wine_glTexEnvi( GLenum target, GLenum pname, GLint param ) {
  TRACE("(%d, %d, %d)\n", target, pname, param );
  glTexEnvi( target, pname, param );
}

/***********************************************************************
 *              glTexEnviv (OPENGL32.@)
 */
void WINAPI wine_glTexEnviv( GLenum target, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  glTexEnviv( target, pname, params );
}

/***********************************************************************
 *              glTexGend (OPENGL32.@)
 */
void WINAPI wine_glTexGend( GLenum coord, GLenum pname, GLdouble param ) {
  TRACE("(%d, %d, %f)\n", coord, pname, param );
  glTexGend( coord, pname, param );
}

/***********************************************************************
 *              glTexGendv (OPENGL32.@)
 */
void WINAPI wine_glTexGendv( GLenum coord, GLenum pname, GLdouble* params ) {
  TRACE("(%d, %d, %p)\n", coord, pname, params );
  glTexGendv( coord, pname, params );
}

/***********************************************************************
 *              glTexGenf (OPENGL32.@)
 */
void WINAPI wine_glTexGenf( GLenum coord, GLenum pname, GLfloat param ) {
  TRACE("(%d, %d, %f)\n", coord, pname, param );
  glTexGenf( coord, pname, param );
}

/***********************************************************************
 *              glTexGenfv (OPENGL32.@)
 */
void WINAPI wine_glTexGenfv( GLenum coord, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", coord, pname, params );
  glTexGenfv( coord, pname, params );
}

/***********************************************************************
 *              glTexGeni (OPENGL32.@)
 */
void WINAPI wine_glTexGeni( GLenum coord, GLenum pname, GLint param ) {
  TRACE("(%d, %d, %d)\n", coord, pname, param );
  glTexGeni( coord, pname, param );
}

/***********************************************************************
 *              glTexGeniv (OPENGL32.@)
 */
void WINAPI wine_glTexGeniv( GLenum coord, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", coord, pname, params );
  glTexGeniv( coord, pname, params );
}

/***********************************************************************
 *              glTexImage1D (OPENGL32.@)
 */
void WINAPI wine_glTexImage1D( GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, GLvoid* pixels ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, border, format, type, pixels );
  glTexImage1D( target, level, internalformat, width, border, format, type, pixels );
}

/***********************************************************************
 *              glTexImage2D (OPENGL32.@)
 */
void WINAPI wine_glTexImage2D( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, GLvoid* pixels ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, height, border, format, type, pixels );
  glTexImage2D( target, level, internalformat, width, height, border, format, type, pixels );
}

/***********************************************************************
 *              glTexParameterf (OPENGL32.@)
 */
void WINAPI wine_glTexParameterf( GLenum target, GLenum pname, GLfloat param ) {
  TRACE("(%d, %d, %f)\n", target, pname, param );
  glTexParameterf( target, pname, param );
}

/***********************************************************************
 *              glTexParameterfv (OPENGL32.@)
 */
void WINAPI wine_glTexParameterfv( GLenum target, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  glTexParameterfv( target, pname, params );
}

/***********************************************************************
 *              glTexParameteri (OPENGL32.@)
 */
void WINAPI wine_glTexParameteri( GLenum target, GLenum pname, GLint param ) {
  TRACE("(%d, %d, %d)\n", target, pname, param );
  glTexParameteri( target, pname, param );
}

/***********************************************************************
 *              glTexParameteriv (OPENGL32.@)
 */
void WINAPI wine_glTexParameteriv( GLenum target, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  glTexParameteriv( target, pname, params );
}

/***********************************************************************
 *              glTexSubImage1D (OPENGL32.@)
 */
void WINAPI wine_glTexSubImage1D( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, GLvoid* pixels ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, width, format, type, pixels );
  glTexSubImage1D( target, level, xoffset, width, format, type, pixels );
}

/***********************************************************************
 *              glTexSubImage2D (OPENGL32.@)
 */
void WINAPI wine_glTexSubImage2D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, width, height, format, type, pixels );
  glTexSubImage2D( target, level, xoffset, yoffset, width, height, format, type, pixels );
}

/***********************************************************************
 *              glTranslated (OPENGL32.@)
 */
void WINAPI wine_glTranslated( GLdouble x, GLdouble y, GLdouble z ) {
  TRACE("(%f, %f, %f)\n", x, y, z );
  glTranslated( x, y, z );
}

/***********************************************************************
 *              glTranslatef (OPENGL32.@)
 */
void WINAPI wine_glTranslatef( GLfloat x, GLfloat y, GLfloat z ) {
  TRACE("(%f, %f, %f)\n", x, y, z );
  glTranslatef( x, y, z );
}

/***********************************************************************
 *              glVertex2d (OPENGL32.@)
 */
void WINAPI wine_glVertex2d( GLdouble x, GLdouble y ) {
  TRACE("(%f, %f)\n", x, y );
  glVertex2d( x, y );
}

/***********************************************************************
 *              glVertex2dv (OPENGL32.@)
 */
void WINAPI wine_glVertex2dv( GLdouble* v ) {
  TRACE("(%p)\n", v );
  glVertex2dv( v );
}

/***********************************************************************
 *              glVertex2f (OPENGL32.@)
 */
void WINAPI wine_glVertex2f( GLfloat x, GLfloat y ) {
  TRACE("(%f, %f)\n", x, y );
  glVertex2f( x, y );
}

/***********************************************************************
 *              glVertex2fv (OPENGL32.@)
 */
void WINAPI wine_glVertex2fv( GLfloat* v ) {
  TRACE("(%p)\n", v );
  glVertex2fv( v );
}

/***********************************************************************
 *              glVertex2i (OPENGL32.@)
 */
void WINAPI wine_glVertex2i( GLint x, GLint y ) {
  TRACE("(%d, %d)\n", x, y );
  glVertex2i( x, y );
}

/***********************************************************************
 *              glVertex2iv (OPENGL32.@)
 */
void WINAPI wine_glVertex2iv( GLint* v ) {
  TRACE("(%p)\n", v );
  glVertex2iv( v );
}

/***********************************************************************
 *              glVertex2s (OPENGL32.@)
 */
void WINAPI wine_glVertex2s( GLshort x, GLshort y ) {
  TRACE("(%d, %d)\n", x, y );
  glVertex2s( x, y );
}

/***********************************************************************
 *              glVertex2sv (OPENGL32.@)
 */
void WINAPI wine_glVertex2sv( GLshort* v ) {
  TRACE("(%p)\n", v );
  glVertex2sv( v );
}

/***********************************************************************
 *              glVertex3d (OPENGL32.@)
 */
void WINAPI wine_glVertex3d( GLdouble x, GLdouble y, GLdouble z ) {
  TRACE("(%f, %f, %f)\n", x, y, z );
  glVertex3d( x, y, z );
}

/***********************************************************************
 *              glVertex3dv (OPENGL32.@)
 */
void WINAPI wine_glVertex3dv( GLdouble* v ) {
  TRACE("(%p)\n", v );
  glVertex3dv( v );
}

/***********************************************************************
 *              glVertex3f (OPENGL32.@)
 */
void WINAPI wine_glVertex3f( GLfloat x, GLfloat y, GLfloat z ) {
  TRACE("(%f, %f, %f)\n", x, y, z );
  glVertex3f( x, y, z );
}

/***********************************************************************
 *              glVertex3fv (OPENGL32.@)
 */
void WINAPI wine_glVertex3fv( GLfloat* v ) {
  TRACE("(%p)\n", v );
  glVertex3fv( v );
}

/***********************************************************************
 *              glVertex3i (OPENGL32.@)
 */
void WINAPI wine_glVertex3i( GLint x, GLint y, GLint z ) {
  TRACE("(%d, %d, %d)\n", x, y, z );
  glVertex3i( x, y, z );
}

/***********************************************************************
 *              glVertex3iv (OPENGL32.@)
 */
void WINAPI wine_glVertex3iv( GLint* v ) {
  TRACE("(%p)\n", v );
  glVertex3iv( v );
}

/***********************************************************************
 *              glVertex3s (OPENGL32.@)
 */
void WINAPI wine_glVertex3s( GLshort x, GLshort y, GLshort z ) {
  TRACE("(%d, %d, %d)\n", x, y, z );
  glVertex3s( x, y, z );
}

/***********************************************************************
 *              glVertex3sv (OPENGL32.@)
 */
void WINAPI wine_glVertex3sv( GLshort* v ) {
  TRACE("(%p)\n", v );
  glVertex3sv( v );
}

/***********************************************************************
 *              glVertex4d (OPENGL32.@)
 */
void WINAPI wine_glVertex4d( GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  TRACE("(%f, %f, %f, %f)\n", x, y, z, w );
  glVertex4d( x, y, z, w );
}

/***********************************************************************
 *              glVertex4dv (OPENGL32.@)
 */
void WINAPI wine_glVertex4dv( GLdouble* v ) {
  TRACE("(%p)\n", v );
  glVertex4dv( v );
}

/***********************************************************************
 *              glVertex4f (OPENGL32.@)
 */
void WINAPI wine_glVertex4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  TRACE("(%f, %f, %f, %f)\n", x, y, z, w );
  glVertex4f( x, y, z, w );
}

/***********************************************************************
 *              glVertex4fv (OPENGL32.@)
 */
void WINAPI wine_glVertex4fv( GLfloat* v ) {
  TRACE("(%p)\n", v );
  glVertex4fv( v );
}

/***********************************************************************
 *              glVertex4i (OPENGL32.@)
 */
void WINAPI wine_glVertex4i( GLint x, GLint y, GLint z, GLint w ) {
  TRACE("(%d, %d, %d, %d)\n", x, y, z, w );
  glVertex4i( x, y, z, w );
}

/***********************************************************************
 *              glVertex4iv (OPENGL32.@)
 */
void WINAPI wine_glVertex4iv( GLint* v ) {
  TRACE("(%p)\n", v );
  glVertex4iv( v );
}

/***********************************************************************
 *              glVertex4s (OPENGL32.@)
 */
void WINAPI wine_glVertex4s( GLshort x, GLshort y, GLshort z, GLshort w ) {
  TRACE("(%d, %d, %d, %d)\n", x, y, z, w );
  glVertex4s( x, y, z, w );
}

/***********************************************************************
 *              glVertex4sv (OPENGL32.@)
 */
void WINAPI wine_glVertex4sv( GLshort* v ) {
  TRACE("(%p)\n", v );
  glVertex4sv( v );
}

/***********************************************************************
 *              glVertexPointer (OPENGL32.@)
 */
void WINAPI wine_glVertexPointer( GLint size, GLenum type, GLsizei stride, GLvoid* pointer ) {
  TRACE("(%d, %d, %d, %p)\n", size, type, stride, pointer );
  glVertexPointer( size, type, stride, pointer );
}

/***********************************************************************
 *              glViewport (OPENGL32.@)
 */
void WINAPI wine_glViewport( GLint x, GLint y, GLsizei width, GLsizei height ) {
  TRACE("(%d, %d, %d, %d)\n", x, y, width, height );
  glViewport( x, y, width, height );
}
