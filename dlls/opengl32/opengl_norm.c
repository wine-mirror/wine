
/* Auto-generated file... Do not edit ! */

#include "config.h"
#include "wine_gl.h"

typedef const GLubyte * GLstring;

/***********************************************************************
 *              glAccum
 */
void WINAPI wine_glAccum( GLenum op, GLfloat value ) {
  ENTER_GL();
  glAccum( op, value );
  LEAVE_GL();
}

/***********************************************************************
 *              glActiveTextureARB
 */
void WINAPI wine_glActiveTextureARB( GLenum texture ) {
  ENTER_GL();
  glActiveTextureARB( texture );
  LEAVE_GL();
}

/***********************************************************************
 *              glAlphaFunc
 */
void WINAPI wine_glAlphaFunc( GLenum func, GLclampf ref ) {
  ENTER_GL();
  glAlphaFunc( func, ref );
  LEAVE_GL();
}

/***********************************************************************
 *              glAreTexturesResident
 */
GLboolean WINAPI wine_glAreTexturesResident( GLsizei n, GLuint* textures, GLboolean* residences ) {
  GLboolean ret_value;
  ENTER_GL();
  ret_value = glAreTexturesResident( n, textures, residences );
  LEAVE_GL();
  return ret_value;
}

/***********************************************************************
 *              glArrayElement
 */
void WINAPI wine_glArrayElement( GLint i ) {
  ENTER_GL();
  glArrayElement( i );
  LEAVE_GL();
}

/***********************************************************************
 *              glBegin
 */
void WINAPI wine_glBegin( GLenum mode ) {
  ENTER_GL();
  glBegin( mode );
  LEAVE_GL();
}

/***********************************************************************
 *              glBindTexture
 */
void WINAPI wine_glBindTexture( GLenum target, GLuint texture ) {
  ENTER_GL();
  glBindTexture( target, texture );
  LEAVE_GL();
}

/***********************************************************************
 *              glBitmap
 */
void WINAPI wine_glBitmap( GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, GLubyte* bitmap ) {
  ENTER_GL();
  glBitmap( width, height, xorig, yorig, xmove, ymove, bitmap );
  LEAVE_GL();
}

/***********************************************************************
 *              glBlendColor
 */
void WINAPI wine_glBlendColor( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha ) {
  ENTER_GL();
  glBlendColor( red, green, blue, alpha );
  LEAVE_GL();
}

/***********************************************************************
 *              glBlendEquation
 */
void WINAPI wine_glBlendEquation( GLenum mode ) {
  ENTER_GL();
  glBlendEquation( mode );
  LEAVE_GL();
}

/***********************************************************************
 *              glBlendFunc
 */
void WINAPI wine_glBlendFunc( GLenum sfactor, GLenum dfactor ) {
  ENTER_GL();
  glBlendFunc( sfactor, dfactor );
  LEAVE_GL();
}

/***********************************************************************
 *              glCallList
 */
void WINAPI wine_glCallList( GLuint list ) {
  ENTER_GL();
  glCallList( list );
  LEAVE_GL();
}

/***********************************************************************
 *              glCallLists
 */
void WINAPI wine_glCallLists( GLsizei n, GLenum type, GLvoid* lists ) {
  ENTER_GL();
  glCallLists( n, type, lists );
  LEAVE_GL();
}

/***********************************************************************
 *              glClear
 */
void WINAPI wine_glClear( GLbitfield mask ) {
  ENTER_GL();
  glClear( mask );
  LEAVE_GL();
}

/***********************************************************************
 *              glClearAccum
 */
void WINAPI wine_glClearAccum( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha ) {
  ENTER_GL();
  glClearAccum( red, green, blue, alpha );
  LEAVE_GL();
}

/***********************************************************************
 *              glClearColor
 */
void WINAPI wine_glClearColor( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha ) {
  ENTER_GL();
  glClearColor( red, green, blue, alpha );
  LEAVE_GL();
}

/***********************************************************************
 *              glClearDepth
 */
void WINAPI wine_glClearDepth( GLclampd depth ) {
  ENTER_GL();
  glClearDepth( depth );
  LEAVE_GL();
}

/***********************************************************************
 *              glClearIndex
 */
void WINAPI wine_glClearIndex( GLfloat c ) {
  ENTER_GL();
  glClearIndex( c );
  LEAVE_GL();
}

/***********************************************************************
 *              glClearStencil
 */
void WINAPI wine_glClearStencil( GLint s ) {
  ENTER_GL();
  glClearStencil( s );
  LEAVE_GL();
}

/***********************************************************************
 *              glClientActiveTextureARB
 */
void WINAPI wine_glClientActiveTextureARB( GLenum texture ) {
  ENTER_GL();
  glClientActiveTextureARB( texture );
  LEAVE_GL();
}

/***********************************************************************
 *              glClipPlane
 */
void WINAPI wine_glClipPlane( GLenum plane, GLdouble* equation ) {
  ENTER_GL();
  glClipPlane( plane, equation );
  LEAVE_GL();
}

/***********************************************************************
 *              glColor3b
 */
void WINAPI wine_glColor3b( GLbyte red, GLbyte green, GLbyte blue ) {
  ENTER_GL();
  glColor3b( red, green, blue );
  LEAVE_GL();
}

/***********************************************************************
 *              glColor3bv
 */
void WINAPI wine_glColor3bv( GLbyte* v ) {
  ENTER_GL();
  glColor3bv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glColor3d
 */
void WINAPI wine_glColor3d( GLdouble red, GLdouble green, GLdouble blue ) {
  ENTER_GL();
  glColor3d( red, green, blue );
  LEAVE_GL();
}

/***********************************************************************
 *              glColor3dv
 */
void WINAPI wine_glColor3dv( GLdouble* v ) {
  ENTER_GL();
  glColor3dv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glColor3f
 */
void WINAPI wine_glColor3f( GLfloat red, GLfloat green, GLfloat blue ) {
  ENTER_GL();
  glColor3f( red, green, blue );
  LEAVE_GL();
}

/***********************************************************************
 *              glColor3fv
 */
void WINAPI wine_glColor3fv( GLfloat* v ) {
  ENTER_GL();
  glColor3fv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glColor3i
 */
void WINAPI wine_glColor3i( GLint red, GLint green, GLint blue ) {
  ENTER_GL();
  glColor3i( red, green, blue );
  LEAVE_GL();
}

/***********************************************************************
 *              glColor3iv
 */
void WINAPI wine_glColor3iv( GLint* v ) {
  ENTER_GL();
  glColor3iv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glColor3s
 */
void WINAPI wine_glColor3s( GLshort red, GLshort green, GLshort blue ) {
  ENTER_GL();
  glColor3s( red, green, blue );
  LEAVE_GL();
}

/***********************************************************************
 *              glColor3sv
 */
void WINAPI wine_glColor3sv( GLshort* v ) {
  ENTER_GL();
  glColor3sv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glColor3ub
 */
void WINAPI wine_glColor3ub( GLubyte red, GLubyte green, GLubyte blue ) {
  ENTER_GL();
  glColor3ub( red, green, blue );
  LEAVE_GL();
}

/***********************************************************************
 *              glColor3ubv
 */
void WINAPI wine_glColor3ubv( GLubyte* v ) {
  ENTER_GL();
  glColor3ubv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glColor3ui
 */
void WINAPI wine_glColor3ui( GLuint red, GLuint green, GLuint blue ) {
  ENTER_GL();
  glColor3ui( red, green, blue );
  LEAVE_GL();
}

/***********************************************************************
 *              glColor3uiv
 */
void WINAPI wine_glColor3uiv( GLuint* v ) {
  ENTER_GL();
  glColor3uiv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glColor3us
 */
void WINAPI wine_glColor3us( GLushort red, GLushort green, GLushort blue ) {
  ENTER_GL();
  glColor3us( red, green, blue );
  LEAVE_GL();
}

/***********************************************************************
 *              glColor3usv
 */
void WINAPI wine_glColor3usv( GLushort* v ) {
  ENTER_GL();
  glColor3usv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glColor4b
 */
void WINAPI wine_glColor4b( GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha ) {
  ENTER_GL();
  glColor4b( red, green, blue, alpha );
  LEAVE_GL();
}

/***********************************************************************
 *              glColor4bv
 */
void WINAPI wine_glColor4bv( GLbyte* v ) {
  ENTER_GL();
  glColor4bv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glColor4d
 */
void WINAPI wine_glColor4d( GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha ) {
  ENTER_GL();
  glColor4d( red, green, blue, alpha );
  LEAVE_GL();
}

/***********************************************************************
 *              glColor4dv
 */
void WINAPI wine_glColor4dv( GLdouble* v ) {
  ENTER_GL();
  glColor4dv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glColor4f
 */
void WINAPI wine_glColor4f( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha ) {
  ENTER_GL();
  glColor4f( red, green, blue, alpha );
  LEAVE_GL();
}

/***********************************************************************
 *              glColor4fv
 */
void WINAPI wine_glColor4fv( GLfloat* v ) {
  ENTER_GL();
  glColor4fv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glColor4i
 */
void WINAPI wine_glColor4i( GLint red, GLint green, GLint blue, GLint alpha ) {
  ENTER_GL();
  glColor4i( red, green, blue, alpha );
  LEAVE_GL();
}

/***********************************************************************
 *              glColor4iv
 */
void WINAPI wine_glColor4iv( GLint* v ) {
  ENTER_GL();
  glColor4iv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glColor4s
 */
void WINAPI wine_glColor4s( GLshort red, GLshort green, GLshort blue, GLshort alpha ) {
  ENTER_GL();
  glColor4s( red, green, blue, alpha );
  LEAVE_GL();
}

/***********************************************************************
 *              glColor4sv
 */
void WINAPI wine_glColor4sv( GLshort* v ) {
  ENTER_GL();
  glColor4sv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glColor4ub
 */
void WINAPI wine_glColor4ub( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha ) {
  ENTER_GL();
  glColor4ub( red, green, blue, alpha );
  LEAVE_GL();
}

/***********************************************************************
 *              glColor4ubv
 */
void WINAPI wine_glColor4ubv( GLubyte* v ) {
  ENTER_GL();
  glColor4ubv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glColor4ui
 */
void WINAPI wine_glColor4ui( GLuint red, GLuint green, GLuint blue, GLuint alpha ) {
  ENTER_GL();
  glColor4ui( red, green, blue, alpha );
  LEAVE_GL();
}

/***********************************************************************
 *              glColor4uiv
 */
void WINAPI wine_glColor4uiv( GLuint* v ) {
  ENTER_GL();
  glColor4uiv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glColor4us
 */
void WINAPI wine_glColor4us( GLushort red, GLushort green, GLushort blue, GLushort alpha ) {
  ENTER_GL();
  glColor4us( red, green, blue, alpha );
  LEAVE_GL();
}

/***********************************************************************
 *              glColor4usv
 */
void WINAPI wine_glColor4usv( GLushort* v ) {
  ENTER_GL();
  glColor4usv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glColorMask
 */
void WINAPI wine_glColorMask( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha ) {
  ENTER_GL();
  glColorMask( red, green, blue, alpha );
  LEAVE_GL();
}

/***********************************************************************
 *              glColorMaterial
 */
void WINAPI wine_glColorMaterial( GLenum face, GLenum mode ) {
  ENTER_GL();
  glColorMaterial( face, mode );
  LEAVE_GL();
}

/***********************************************************************
 *              glColorPointer
 */
void WINAPI wine_glColorPointer( GLint size, GLenum type, GLsizei stride, GLvoid* pointer ) {
  ENTER_GL();
  glColorPointer( size, type, stride, pointer );
  LEAVE_GL();
}

/***********************************************************************
 *              glColorSubTable
 */
void WINAPI wine_glColorSubTable( GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, GLvoid* data ) {
  ENTER_GL();
  glColorSubTable( target, start, count, format, type, data );
  LEAVE_GL();
}

/***********************************************************************
 *              glColorTable
 */
void WINAPI wine_glColorTable( GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, GLvoid* table ) {
  ENTER_GL();
  glColorTable( target, internalformat, width, format, type, table );
  LEAVE_GL();
}

/***********************************************************************
 *              glColorTableParameterfv
 */
void WINAPI wine_glColorTableParameterfv( GLenum target, GLenum pname, GLfloat* params ) {
  ENTER_GL();
  glColorTableParameterfv( target, pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glColorTableParameteriv
 */
void WINAPI wine_glColorTableParameteriv( GLenum target, GLenum pname, GLint* params ) {
  ENTER_GL();
  glColorTableParameteriv( target, pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glConvolutionFilter1D
 */
void WINAPI wine_glConvolutionFilter1D( GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, GLvoid* image ) {
  ENTER_GL();
  glConvolutionFilter1D( target, internalformat, width, format, type, image );
  LEAVE_GL();
}

/***********************************************************************
 *              glConvolutionFilter2D
 */
void WINAPI wine_glConvolutionFilter2D( GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* image ) {
  ENTER_GL();
  glConvolutionFilter2D( target, internalformat, width, height, format, type, image );
  LEAVE_GL();
}

/***********************************************************************
 *              glConvolutionParameterf
 */
void WINAPI wine_glConvolutionParameterf( GLenum target, GLenum pname, GLfloat params ) {
  ENTER_GL();
  glConvolutionParameterf( target, pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glConvolutionParameterfv
 */
void WINAPI wine_glConvolutionParameterfv( GLenum target, GLenum pname, GLfloat* params ) {
  ENTER_GL();
  glConvolutionParameterfv( target, pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glConvolutionParameteri
 */
void WINAPI wine_glConvolutionParameteri( GLenum target, GLenum pname, GLint params ) {
  ENTER_GL();
  glConvolutionParameteri( target, pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glConvolutionParameteriv
 */
void WINAPI wine_glConvolutionParameteriv( GLenum target, GLenum pname, GLint* params ) {
  ENTER_GL();
  glConvolutionParameteriv( target, pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glCopyColorSubTable
 */
void WINAPI wine_glCopyColorSubTable( GLenum target, GLsizei start, GLint x, GLint y, GLsizei width ) {
  ENTER_GL();
  glCopyColorSubTable( target, start, x, y, width );
  LEAVE_GL();
}

/***********************************************************************
 *              glCopyColorTable
 */
void WINAPI wine_glCopyColorTable( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width ) {
  ENTER_GL();
  glCopyColorTable( target, internalformat, x, y, width );
  LEAVE_GL();
}

/***********************************************************************
 *              glCopyConvolutionFilter1D
 */
void WINAPI wine_glCopyConvolutionFilter1D( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width ) {
  ENTER_GL();
  glCopyConvolutionFilter1D( target, internalformat, x, y, width );
  LEAVE_GL();
}

/***********************************************************************
 *              glCopyConvolutionFilter2D
 */
void WINAPI wine_glCopyConvolutionFilter2D( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height ) {
  ENTER_GL();
  glCopyConvolutionFilter2D( target, internalformat, x, y, width, height );
  LEAVE_GL();
}

/***********************************************************************
 *              glCopyPixels
 */
void WINAPI wine_glCopyPixels( GLint x, GLint y, GLsizei width, GLsizei height, GLenum type ) {
  ENTER_GL();
  glCopyPixels( x, y, width, height, type );
  LEAVE_GL();
}

/***********************************************************************
 *              glCopyTexImage1D
 */
void WINAPI wine_glCopyTexImage1D( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border ) {
  ENTER_GL();
  glCopyTexImage1D( target, level, internalformat, x, y, width, border );
  LEAVE_GL();
}

/***********************************************************************
 *              glCopyTexImage2D
 */
void WINAPI wine_glCopyTexImage2D( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border ) {
  ENTER_GL();
  glCopyTexImage2D( target, level, internalformat, x, y, width, height, border );
  LEAVE_GL();
}

/***********************************************************************
 *              glCopyTexSubImage1D
 */
void WINAPI wine_glCopyTexSubImage1D( GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width ) {
  ENTER_GL();
  glCopyTexSubImage1D( target, level, xoffset, x, y, width );
  LEAVE_GL();
}

/***********************************************************************
 *              glCopyTexSubImage2D
 */
void WINAPI wine_glCopyTexSubImage2D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height ) {
  ENTER_GL();
  glCopyTexSubImage2D( target, level, xoffset, yoffset, x, y, width, height );
  LEAVE_GL();
}

/***********************************************************************
 *              glCopyTexSubImage3D
 */
void WINAPI wine_glCopyTexSubImage3D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height ) {
  ENTER_GL();
  glCopyTexSubImage3D( target, level, xoffset, yoffset, zoffset, x, y, width, height );
  LEAVE_GL();
}

/***********************************************************************
 *              glCullFace
 */
void WINAPI wine_glCullFace( GLenum mode ) {
  ENTER_GL();
  glCullFace( mode );
  LEAVE_GL();
}

/***********************************************************************
 *              glDeleteLists
 */
void WINAPI wine_glDeleteLists( GLuint list, GLsizei range ) {
  ENTER_GL();
  glDeleteLists( list, range );
  LEAVE_GL();
}

/***********************************************************************
 *              glDeleteTextures
 */
void WINAPI wine_glDeleteTextures( GLsizei n, GLuint* textures ) {
  ENTER_GL();
  glDeleteTextures( n, textures );
  LEAVE_GL();
}

/***********************************************************************
 *              glDepthFunc
 */
void WINAPI wine_glDepthFunc( GLenum func ) {
  ENTER_GL();
  glDepthFunc( func );
  LEAVE_GL();
}

/***********************************************************************
 *              glDepthMask
 */
void WINAPI wine_glDepthMask( GLboolean flag ) {
  ENTER_GL();
  glDepthMask( flag );
  LEAVE_GL();
}

/***********************************************************************
 *              glDepthRange
 */
void WINAPI wine_glDepthRange( GLclampd near, GLclampd far ) {
  ENTER_GL();
  glDepthRange( near, far );
  LEAVE_GL();
}

/***********************************************************************
 *              glDisable
 */
void WINAPI wine_glDisable( GLenum cap ) {
  ENTER_GL();
  glDisable( cap );
  LEAVE_GL();
}

/***********************************************************************
 *              glDisableClientState
 */
void WINAPI wine_glDisableClientState( GLenum array ) {
  ENTER_GL();
  glDisableClientState( array );
  LEAVE_GL();
}

/***********************************************************************
 *              glDrawArrays
 */
void WINAPI wine_glDrawArrays( GLenum mode, GLint first, GLsizei count ) {
  ENTER_GL();
  glDrawArrays( mode, first, count );
  LEAVE_GL();
}

/***********************************************************************
 *              glDrawBuffer
 */
void WINAPI wine_glDrawBuffer( GLenum mode ) {
  ENTER_GL();
  glDrawBuffer( mode );
  LEAVE_GL();
}

/***********************************************************************
 *              glDrawElements
 */
void WINAPI wine_glDrawElements( GLenum mode, GLsizei count, GLenum type, GLvoid* indices ) {
  ENTER_GL();
  glDrawElements( mode, count, type, indices );
  LEAVE_GL();
}

/***********************************************************************
 *              glDrawPixels
 */
void WINAPI wine_glDrawPixels( GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels ) {
  ENTER_GL();
  glDrawPixels( width, height, format, type, pixels );
  LEAVE_GL();
}

/***********************************************************************
 *              glDrawRangeElements
 */
void WINAPI wine_glDrawRangeElements( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, GLvoid* indices ) {
  ENTER_GL();
  glDrawRangeElements( mode, start, end, count, type, indices );
  LEAVE_GL();
}

/***********************************************************************
 *              glEdgeFlag
 */
void WINAPI wine_glEdgeFlag( GLboolean flag ) {
  ENTER_GL();
  glEdgeFlag( flag );
  LEAVE_GL();
}

/***********************************************************************
 *              glEdgeFlagPointer
 */
void WINAPI wine_glEdgeFlagPointer( GLsizei stride, GLvoid* pointer ) {
  ENTER_GL();
  glEdgeFlagPointer( stride, pointer );
  LEAVE_GL();
}

/***********************************************************************
 *              glEdgeFlagv
 */
void WINAPI wine_glEdgeFlagv( GLboolean* flag ) {
  ENTER_GL();
  glEdgeFlagv( flag );
  LEAVE_GL();
}

/***********************************************************************
 *              glEnable
 */
void WINAPI wine_glEnable( GLenum cap ) {
  ENTER_GL();
  glEnable( cap );
  LEAVE_GL();
}

/***********************************************************************
 *              glEnableClientState
 */
void WINAPI wine_glEnableClientState( GLenum array ) {
  ENTER_GL();
  glEnableClientState( array );
  LEAVE_GL();
}

/***********************************************************************
 *              glEnd
 */
void WINAPI wine_glEnd( ) {
  ENTER_GL();
  glEnd( );
  LEAVE_GL();
}

/***********************************************************************
 *              glEndList
 */
void WINAPI wine_glEndList( ) {
  ENTER_GL();
  glEndList( );
  LEAVE_GL();
}

/***********************************************************************
 *              glEvalCoord1d
 */
void WINAPI wine_glEvalCoord1d( GLdouble u ) {
  ENTER_GL();
  glEvalCoord1d( u );
  LEAVE_GL();
}

/***********************************************************************
 *              glEvalCoord1dv
 */
void WINAPI wine_glEvalCoord1dv( GLdouble* u ) {
  ENTER_GL();
  glEvalCoord1dv( u );
  LEAVE_GL();
}

/***********************************************************************
 *              glEvalCoord1f
 */
void WINAPI wine_glEvalCoord1f( GLfloat u ) {
  ENTER_GL();
  glEvalCoord1f( u );
  LEAVE_GL();
}

/***********************************************************************
 *              glEvalCoord1fv
 */
void WINAPI wine_glEvalCoord1fv( GLfloat* u ) {
  ENTER_GL();
  glEvalCoord1fv( u );
  LEAVE_GL();
}

/***********************************************************************
 *              glEvalCoord2d
 */
void WINAPI wine_glEvalCoord2d( GLdouble u, GLdouble v ) {
  ENTER_GL();
  glEvalCoord2d( u, v );
  LEAVE_GL();
}

/***********************************************************************
 *              glEvalCoord2dv
 */
void WINAPI wine_glEvalCoord2dv( GLdouble* u ) {
  ENTER_GL();
  glEvalCoord2dv( u );
  LEAVE_GL();
}

/***********************************************************************
 *              glEvalCoord2f
 */
void WINAPI wine_glEvalCoord2f( GLfloat u, GLfloat v ) {
  ENTER_GL();
  glEvalCoord2f( u, v );
  LEAVE_GL();
}

/***********************************************************************
 *              glEvalCoord2fv
 */
void WINAPI wine_glEvalCoord2fv( GLfloat* u ) {
  ENTER_GL();
  glEvalCoord2fv( u );
  LEAVE_GL();
}

/***********************************************************************
 *              glEvalMesh1
 */
void WINAPI wine_glEvalMesh1( GLenum mode, GLint i1, GLint i2 ) {
  ENTER_GL();
  glEvalMesh1( mode, i1, i2 );
  LEAVE_GL();
}

/***********************************************************************
 *              glEvalMesh2
 */
void WINAPI wine_glEvalMesh2( GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2 ) {
  ENTER_GL();
  glEvalMesh2( mode, i1, i2, j1, j2 );
  LEAVE_GL();
}

/***********************************************************************
 *              glEvalPoint1
 */
void WINAPI wine_glEvalPoint1( GLint i ) {
  ENTER_GL();
  glEvalPoint1( i );
  LEAVE_GL();
}

/***********************************************************************
 *              glEvalPoint2
 */
void WINAPI wine_glEvalPoint2( GLint i, GLint j ) {
  ENTER_GL();
  glEvalPoint2( i, j );
  LEAVE_GL();
}

/***********************************************************************
 *              glFeedbackBuffer
 */
void WINAPI wine_glFeedbackBuffer( GLsizei size, GLenum type, GLfloat* buffer ) {
  ENTER_GL();
  glFeedbackBuffer( size, type, buffer );
  LEAVE_GL();
}

/***********************************************************************
 *              glFinish
 */
void WINAPI wine_glFinish( ) {
  ENTER_GL();
  glFinish( );
  LEAVE_GL();
}

/***********************************************************************
 *              glFlush
 */
void WINAPI wine_glFlush( ) {
  ENTER_GL();
  glFlush( );
  LEAVE_GL();
}

/***********************************************************************
 *              glFogf
 */
void WINAPI wine_glFogf( GLenum pname, GLfloat param ) {
  ENTER_GL();
  glFogf( pname, param );
  LEAVE_GL();
}

/***********************************************************************
 *              glFogfv
 */
void WINAPI wine_glFogfv( GLenum pname, GLfloat* params ) {
  ENTER_GL();
  glFogfv( pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glFogi
 */
void WINAPI wine_glFogi( GLenum pname, GLint param ) {
  ENTER_GL();
  glFogi( pname, param );
  LEAVE_GL();
}

/***********************************************************************
 *              glFogiv
 */
void WINAPI wine_glFogiv( GLenum pname, GLint* params ) {
  ENTER_GL();
  glFogiv( pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glFrontFace
 */
void WINAPI wine_glFrontFace( GLenum mode ) {
  ENTER_GL();
  glFrontFace( mode );
  LEAVE_GL();
}

/***********************************************************************
 *              glFrustum
 */
void WINAPI wine_glFrustum( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar ) {
  ENTER_GL();
  glFrustum( left, right, bottom, top, zNear, zFar );
  LEAVE_GL();
}

/***********************************************************************
 *              glGenLists
 */
GLuint WINAPI wine_glGenLists( GLsizei range ) {
  GLuint ret_value;
  ENTER_GL();
  ret_value = glGenLists( range );
  LEAVE_GL();
  return ret_value;
}

/***********************************************************************
 *              glGenTextures
 */
void WINAPI wine_glGenTextures( GLsizei n, GLuint* textures ) {
  ENTER_GL();
  glGenTextures( n, textures );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetBooleanv
 */
void WINAPI wine_glGetBooleanv( GLenum pname, GLboolean* params ) {
  ENTER_GL();
  glGetBooleanv( pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetClipPlane
 */
void WINAPI wine_glGetClipPlane( GLenum plane, GLdouble* equation ) {
  ENTER_GL();
  glGetClipPlane( plane, equation );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetColorTable
 */
void WINAPI wine_glGetColorTable( GLenum target, GLenum format, GLenum type, GLvoid* table ) {
  ENTER_GL();
  glGetColorTable( target, format, type, table );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetColorTableParameterfv
 */
void WINAPI wine_glGetColorTableParameterfv( GLenum target, GLenum pname, GLfloat* params ) {
  ENTER_GL();
  glGetColorTableParameterfv( target, pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetColorTableParameteriv
 */
void WINAPI wine_glGetColorTableParameteriv( GLenum target, GLenum pname, GLint* params ) {
  ENTER_GL();
  glGetColorTableParameteriv( target, pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetConvolutionFilter
 */
void WINAPI wine_glGetConvolutionFilter( GLenum target, GLenum format, GLenum type, GLvoid* image ) {
  ENTER_GL();
  glGetConvolutionFilter( target, format, type, image );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetConvolutionParameterfv
 */
void WINAPI wine_glGetConvolutionParameterfv( GLenum target, GLenum pname, GLfloat* params ) {
  ENTER_GL();
  glGetConvolutionParameterfv( target, pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetConvolutionParameteriv
 */
void WINAPI wine_glGetConvolutionParameteriv( GLenum target, GLenum pname, GLint* params ) {
  ENTER_GL();
  glGetConvolutionParameteriv( target, pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetDoublev
 */
void WINAPI wine_glGetDoublev( GLenum pname, GLdouble* params ) {
  ENTER_GL();
  glGetDoublev( pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetError
 */
GLenum WINAPI wine_glGetError( ) {
  GLenum ret_value;
  ENTER_GL();
  ret_value = glGetError( );
  LEAVE_GL();
  return ret_value;
}

/***********************************************************************
 *              glGetFloatv
 */
void WINAPI wine_glGetFloatv( GLenum pname, GLfloat* params ) {
  ENTER_GL();
  glGetFloatv( pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetHistogram
 */
void WINAPI wine_glGetHistogram( GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid* values ) {
  ENTER_GL();
  glGetHistogram( target, reset, format, type, values );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetHistogramParameterfv
 */
void WINAPI wine_glGetHistogramParameterfv( GLenum target, GLenum pname, GLfloat* params ) {
  ENTER_GL();
  glGetHistogramParameterfv( target, pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetHistogramParameteriv
 */
void WINAPI wine_glGetHistogramParameteriv( GLenum target, GLenum pname, GLint* params ) {
  ENTER_GL();
  glGetHistogramParameteriv( target, pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetIntegerv
 */
void WINAPI wine_glGetIntegerv( GLenum pname, GLint* params ) {
  ENTER_GL();
  glGetIntegerv( pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetLightfv
 */
void WINAPI wine_glGetLightfv( GLenum light, GLenum pname, GLfloat* params ) {
  ENTER_GL();
  glGetLightfv( light, pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetLightiv
 */
void WINAPI wine_glGetLightiv( GLenum light, GLenum pname, GLint* params ) {
  ENTER_GL();
  glGetLightiv( light, pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetMapdv
 */
void WINAPI wine_glGetMapdv( GLenum target, GLenum query, GLdouble* v ) {
  ENTER_GL();
  glGetMapdv( target, query, v );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetMapfv
 */
void WINAPI wine_glGetMapfv( GLenum target, GLenum query, GLfloat* v ) {
  ENTER_GL();
  glGetMapfv( target, query, v );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetMapiv
 */
void WINAPI wine_glGetMapiv( GLenum target, GLenum query, GLint* v ) {
  ENTER_GL();
  glGetMapiv( target, query, v );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetMaterialfv
 */
void WINAPI wine_glGetMaterialfv( GLenum face, GLenum pname, GLfloat* params ) {
  ENTER_GL();
  glGetMaterialfv( face, pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetMaterialiv
 */
void WINAPI wine_glGetMaterialiv( GLenum face, GLenum pname, GLint* params ) {
  ENTER_GL();
  glGetMaterialiv( face, pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetMinmax
 */
void WINAPI wine_glGetMinmax( GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid* values ) {
  ENTER_GL();
  glGetMinmax( target, reset, format, type, values );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetMinmaxParameterfv
 */
void WINAPI wine_glGetMinmaxParameterfv( GLenum target, GLenum pname, GLfloat* params ) {
  ENTER_GL();
  glGetMinmaxParameterfv( target, pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetMinmaxParameteriv
 */
void WINAPI wine_glGetMinmaxParameteriv( GLenum target, GLenum pname, GLint* params ) {
  ENTER_GL();
  glGetMinmaxParameteriv( target, pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetPixelMapfv
 */
void WINAPI wine_glGetPixelMapfv( GLenum map, GLfloat* values ) {
  ENTER_GL();
  glGetPixelMapfv( map, values );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetPixelMapuiv
 */
void WINAPI wine_glGetPixelMapuiv( GLenum map, GLuint* values ) {
  ENTER_GL();
  glGetPixelMapuiv( map, values );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetPixelMapusv
 */
void WINAPI wine_glGetPixelMapusv( GLenum map, GLushort* values ) {
  ENTER_GL();
  glGetPixelMapusv( map, values );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetPointerv
 */
void WINAPI wine_glGetPointerv( GLenum pname, GLvoid** params ) {
  ENTER_GL();
  glGetPointerv( pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetPolygonStipple
 */
void WINAPI wine_glGetPolygonStipple( GLubyte* mask ) {
  ENTER_GL();
  glGetPolygonStipple( mask );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetSeparableFilter
 */
void WINAPI wine_glGetSeparableFilter( GLenum target, GLenum format, GLenum type, GLvoid* row, GLvoid* column, GLvoid* span ) {
  ENTER_GL();
  glGetSeparableFilter( target, format, type, row, column, span );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetString
 */
GLstring WINAPI wine_glGetString( GLenum name ) {
  GLstring ret_value;
  ENTER_GL();
  ret_value = glGetString( name );
  LEAVE_GL();
  return ret_value;
}

/***********************************************************************
 *              glGetTexEnvfv
 */
void WINAPI wine_glGetTexEnvfv( GLenum target, GLenum pname, GLfloat* params ) {
  ENTER_GL();
  glGetTexEnvfv( target, pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetTexEnviv
 */
void WINAPI wine_glGetTexEnviv( GLenum target, GLenum pname, GLint* params ) {
  ENTER_GL();
  glGetTexEnviv( target, pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetTexGendv
 */
void WINAPI wine_glGetTexGendv( GLenum coord, GLenum pname, GLdouble* params ) {
  ENTER_GL();
  glGetTexGendv( coord, pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetTexGenfv
 */
void WINAPI wine_glGetTexGenfv( GLenum coord, GLenum pname, GLfloat* params ) {
  ENTER_GL();
  glGetTexGenfv( coord, pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetTexGeniv
 */
void WINAPI wine_glGetTexGeniv( GLenum coord, GLenum pname, GLint* params ) {
  ENTER_GL();
  glGetTexGeniv( coord, pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetTexImage
 */
void WINAPI wine_glGetTexImage( GLenum target, GLint level, GLenum format, GLenum type, GLvoid* pixels ) {
  ENTER_GL();
  glGetTexImage( target, level, format, type, pixels );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetTexLevelParameterfv
 */
void WINAPI wine_glGetTexLevelParameterfv( GLenum target, GLint level, GLenum pname, GLfloat* params ) {
  ENTER_GL();
  glGetTexLevelParameterfv( target, level, pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetTexLevelParameteriv
 */
void WINAPI wine_glGetTexLevelParameteriv( GLenum target, GLint level, GLenum pname, GLint* params ) {
  ENTER_GL();
  glGetTexLevelParameteriv( target, level, pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetTexParameterfv
 */
void WINAPI wine_glGetTexParameterfv( GLenum target, GLenum pname, GLfloat* params ) {
  ENTER_GL();
  glGetTexParameterfv( target, pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glGetTexParameteriv
 */
void WINAPI wine_glGetTexParameteriv( GLenum target, GLenum pname, GLint* params ) {
  ENTER_GL();
  glGetTexParameteriv( target, pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glHint
 */
void WINAPI wine_glHint( GLenum target, GLenum mode ) {
  ENTER_GL();
  glHint( target, mode );
  LEAVE_GL();
}

/***********************************************************************
 *              glHistogram
 */
void WINAPI wine_glHistogram( GLenum target, GLsizei width, GLenum internalformat, GLboolean sink ) {
  ENTER_GL();
  glHistogram( target, width, internalformat, sink );
  LEAVE_GL();
}

/***********************************************************************
 *              glIndexMask
 */
void WINAPI wine_glIndexMask( GLuint mask ) {
  ENTER_GL();
  glIndexMask( mask );
  LEAVE_GL();
}

/***********************************************************************
 *              glIndexPointer
 */
void WINAPI wine_glIndexPointer( GLenum type, GLsizei stride, GLvoid* pointer ) {
  ENTER_GL();
  glIndexPointer( type, stride, pointer );
  LEAVE_GL();
}

/***********************************************************************
 *              glIndexd
 */
void WINAPI wine_glIndexd( GLdouble c ) {
  ENTER_GL();
  glIndexd( c );
  LEAVE_GL();
}

/***********************************************************************
 *              glIndexdv
 */
void WINAPI wine_glIndexdv( GLdouble* c ) {
  ENTER_GL();
  glIndexdv( c );
  LEAVE_GL();
}

/***********************************************************************
 *              glIndexf
 */
void WINAPI wine_glIndexf( GLfloat c ) {
  ENTER_GL();
  glIndexf( c );
  LEAVE_GL();
}

/***********************************************************************
 *              glIndexfv
 */
void WINAPI wine_glIndexfv( GLfloat* c ) {
  ENTER_GL();
  glIndexfv( c );
  LEAVE_GL();
}

/***********************************************************************
 *              glIndexi
 */
void WINAPI wine_glIndexi( GLint c ) {
  ENTER_GL();
  glIndexi( c );
  LEAVE_GL();
}

/***********************************************************************
 *              glIndexiv
 */
void WINAPI wine_glIndexiv( GLint* c ) {
  ENTER_GL();
  glIndexiv( c );
  LEAVE_GL();
}

/***********************************************************************
 *              glIndexs
 */
void WINAPI wine_glIndexs( GLshort c ) {
  ENTER_GL();
  glIndexs( c );
  LEAVE_GL();
}

/***********************************************************************
 *              glIndexsv
 */
void WINAPI wine_glIndexsv( GLshort* c ) {
  ENTER_GL();
  glIndexsv( c );
  LEAVE_GL();
}

/***********************************************************************
 *              glIndexub
 */
void WINAPI wine_glIndexub( GLubyte c ) {
  ENTER_GL();
  glIndexub( c );
  LEAVE_GL();
}

/***********************************************************************
 *              glIndexubv
 */
void WINAPI wine_glIndexubv( GLubyte* c ) {
  ENTER_GL();
  glIndexubv( c );
  LEAVE_GL();
}

/***********************************************************************
 *              glInitNames
 */
void WINAPI wine_glInitNames( ) {
  ENTER_GL();
  glInitNames( );
  LEAVE_GL();
}

/***********************************************************************
 *              glInterleavedArrays
 */
void WINAPI wine_glInterleavedArrays( GLenum format, GLsizei stride, GLvoid* pointer ) {
  ENTER_GL();
  glInterleavedArrays( format, stride, pointer );
  LEAVE_GL();
}

/***********************************************************************
 *              glIsEnabled
 */
GLboolean WINAPI wine_glIsEnabled( GLenum cap ) {
  GLboolean ret_value;
  ENTER_GL();
  ret_value = glIsEnabled( cap );
  LEAVE_GL();
  return ret_value;
}

/***********************************************************************
 *              glIsList
 */
GLboolean WINAPI wine_glIsList( GLuint list ) {
  GLboolean ret_value;
  ENTER_GL();
  ret_value = glIsList( list );
  LEAVE_GL();
  return ret_value;
}

/***********************************************************************
 *              glIsTexture
 */
GLboolean WINAPI wine_glIsTexture( GLuint texture ) {
  GLboolean ret_value;
  ENTER_GL();
  ret_value = glIsTexture( texture );
  LEAVE_GL();
  return ret_value;
}

/***********************************************************************
 *              glLightModelf
 */
void WINAPI wine_glLightModelf( GLenum pname, GLfloat param ) {
  ENTER_GL();
  glLightModelf( pname, param );
  LEAVE_GL();
}

/***********************************************************************
 *              glLightModelfv
 */
void WINAPI wine_glLightModelfv( GLenum pname, GLfloat* params ) {
  ENTER_GL();
  glLightModelfv( pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glLightModeli
 */
void WINAPI wine_glLightModeli( GLenum pname, GLint param ) {
  ENTER_GL();
  glLightModeli( pname, param );
  LEAVE_GL();
}

/***********************************************************************
 *              glLightModeliv
 */
void WINAPI wine_glLightModeliv( GLenum pname, GLint* params ) {
  ENTER_GL();
  glLightModeliv( pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glLightf
 */
void WINAPI wine_glLightf( GLenum light, GLenum pname, GLfloat param ) {
  ENTER_GL();
  glLightf( light, pname, param );
  LEAVE_GL();
}

/***********************************************************************
 *              glLightfv
 */
void WINAPI wine_glLightfv( GLenum light, GLenum pname, GLfloat* params ) {
  ENTER_GL();
  glLightfv( light, pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glLighti
 */
void WINAPI wine_glLighti( GLenum light, GLenum pname, GLint param ) {
  ENTER_GL();
  glLighti( light, pname, param );
  LEAVE_GL();
}

/***********************************************************************
 *              glLightiv
 */
void WINAPI wine_glLightiv( GLenum light, GLenum pname, GLint* params ) {
  ENTER_GL();
  glLightiv( light, pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glLineStipple
 */
void WINAPI wine_glLineStipple( GLint factor, GLushort pattern ) {
  ENTER_GL();
  glLineStipple( factor, pattern );
  LEAVE_GL();
}

/***********************************************************************
 *              glLineWidth
 */
void WINAPI wine_glLineWidth( GLfloat width ) {
  ENTER_GL();
  glLineWidth( width );
  LEAVE_GL();
}

/***********************************************************************
 *              glListBase
 */
void WINAPI wine_glListBase( GLuint base ) {
  ENTER_GL();
  glListBase( base );
  LEAVE_GL();
}

/***********************************************************************
 *              glLoadIdentity
 */
void WINAPI wine_glLoadIdentity( ) {
  ENTER_GL();
  glLoadIdentity( );
  LEAVE_GL();
}

/***********************************************************************
 *              glLoadMatrixd
 */
void WINAPI wine_glLoadMatrixd( GLdouble* m ) {
  ENTER_GL();
  glLoadMatrixd( m );
  LEAVE_GL();
}

/***********************************************************************
 *              glLoadMatrixf
 */
void WINAPI wine_glLoadMatrixf( GLfloat* m ) {
  ENTER_GL();
  glLoadMatrixf( m );
  LEAVE_GL();
}

/***********************************************************************
 *              glLoadName
 */
void WINAPI wine_glLoadName( GLuint name ) {
  ENTER_GL();
  glLoadName( name );
  LEAVE_GL();
}

/***********************************************************************
 *              glLogicOp
 */
void WINAPI wine_glLogicOp( GLenum opcode ) {
  ENTER_GL();
  glLogicOp( opcode );
  LEAVE_GL();
}

/***********************************************************************
 *              glMap1d
 */
void WINAPI wine_glMap1d( GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, GLdouble* points ) {
  ENTER_GL();
  glMap1d( target, u1, u2, stride, order, points );
  LEAVE_GL();
}

/***********************************************************************
 *              glMap1f
 */
void WINAPI wine_glMap1f( GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, GLfloat* points ) {
  ENTER_GL();
  glMap1f( target, u1, u2, stride, order, points );
  LEAVE_GL();
}

/***********************************************************************
 *              glMap2d
 */
void WINAPI wine_glMap2d( GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, GLdouble* points ) {
  ENTER_GL();
  glMap2d( target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points );
  LEAVE_GL();
}

/***********************************************************************
 *              glMap2f
 */
void WINAPI wine_glMap2f( GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, GLfloat* points ) {
  ENTER_GL();
  glMap2f( target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points );
  LEAVE_GL();
}

/***********************************************************************
 *              glMapGrid1d
 */
void WINAPI wine_glMapGrid1d( GLint un, GLdouble u1, GLdouble u2 ) {
  ENTER_GL();
  glMapGrid1d( un, u1, u2 );
  LEAVE_GL();
}

/***********************************************************************
 *              glMapGrid1f
 */
void WINAPI wine_glMapGrid1f( GLint un, GLfloat u1, GLfloat u2 ) {
  ENTER_GL();
  glMapGrid1f( un, u1, u2 );
  LEAVE_GL();
}

/***********************************************************************
 *              glMapGrid2d
 */
void WINAPI wine_glMapGrid2d( GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2 ) {
  ENTER_GL();
  glMapGrid2d( un, u1, u2, vn, v1, v2 );
  LEAVE_GL();
}

/***********************************************************************
 *              glMapGrid2f
 */
void WINAPI wine_glMapGrid2f( GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2 ) {
  ENTER_GL();
  glMapGrid2f( un, u1, u2, vn, v1, v2 );
  LEAVE_GL();
}

/***********************************************************************
 *              glMaterialf
 */
void WINAPI wine_glMaterialf( GLenum face, GLenum pname, GLfloat param ) {
  ENTER_GL();
  glMaterialf( face, pname, param );
  LEAVE_GL();
}

/***********************************************************************
 *              glMaterialfv
 */
void WINAPI wine_glMaterialfv( GLenum face, GLenum pname, GLfloat* params ) {
  ENTER_GL();
  glMaterialfv( face, pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glMateriali
 */
void WINAPI wine_glMateriali( GLenum face, GLenum pname, GLint param ) {
  ENTER_GL();
  glMateriali( face, pname, param );
  LEAVE_GL();
}

/***********************************************************************
 *              glMaterialiv
 */
void WINAPI wine_glMaterialiv( GLenum face, GLenum pname, GLint* params ) {
  ENTER_GL();
  glMaterialiv( face, pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glMatrixMode
 */
void WINAPI wine_glMatrixMode( GLenum mode ) {
  ENTER_GL();
  glMatrixMode( mode );
  LEAVE_GL();
}

/***********************************************************************
 *              glMinmax
 */
void WINAPI wine_glMinmax( GLenum target, GLenum internalformat, GLboolean sink ) {
  ENTER_GL();
  glMinmax( target, internalformat, sink );
  LEAVE_GL();
}

/***********************************************************************
 *              glMultMatrixd
 */
void WINAPI wine_glMultMatrixd( GLdouble* m ) {
  ENTER_GL();
  glMultMatrixd( m );
  LEAVE_GL();
}

/***********************************************************************
 *              glMultMatrixf
 */
void WINAPI wine_glMultMatrixf( GLfloat* m ) {
  ENTER_GL();
  glMultMatrixf( m );
  LEAVE_GL();
}

/***********************************************************************
 *              glMultiTexCoord1dARB
 */
void WINAPI wine_glMultiTexCoord1dARB( GLenum target, GLdouble s ) {
  ENTER_GL();
  glMultiTexCoord1dARB( target, s );
  LEAVE_GL();
}

/***********************************************************************
 *              glMultiTexCoord1dvARB
 */
void WINAPI wine_glMultiTexCoord1dvARB( GLenum target, GLdouble* v ) {
  ENTER_GL();
  glMultiTexCoord1dvARB( target, v );
  LEAVE_GL();
}

/***********************************************************************
 *              glMultiTexCoord1fARB
 */
void WINAPI wine_glMultiTexCoord1fARB( GLenum target, GLfloat s ) {
  ENTER_GL();
  glMultiTexCoord1fARB( target, s );
  LEAVE_GL();
}

/***********************************************************************
 *              glMultiTexCoord1fvARB
 */
void WINAPI wine_glMultiTexCoord1fvARB( GLenum target, GLfloat* v ) {
  ENTER_GL();
  glMultiTexCoord1fvARB( target, v );
  LEAVE_GL();
}

/***********************************************************************
 *              glMultiTexCoord1iARB
 */
void WINAPI wine_glMultiTexCoord1iARB( GLenum target, GLint s ) {
  ENTER_GL();
  glMultiTexCoord1iARB( target, s );
  LEAVE_GL();
}

/***********************************************************************
 *              glMultiTexCoord1ivARB
 */
void WINAPI wine_glMultiTexCoord1ivARB( GLenum target, GLint* v ) {
  ENTER_GL();
  glMultiTexCoord1ivARB( target, v );
  LEAVE_GL();
}

/***********************************************************************
 *              glMultiTexCoord1sARB
 */
void WINAPI wine_glMultiTexCoord1sARB( GLenum target, GLshort s ) {
  ENTER_GL();
  glMultiTexCoord1sARB( target, s );
  LEAVE_GL();
}

/***********************************************************************
 *              glMultiTexCoord1svARB
 */
void WINAPI wine_glMultiTexCoord1svARB( GLenum target, GLshort* v ) {
  ENTER_GL();
  glMultiTexCoord1svARB( target, v );
  LEAVE_GL();
}

/***********************************************************************
 *              glMultiTexCoord2dARB
 */
void WINAPI wine_glMultiTexCoord2dARB( GLenum target, GLdouble s, GLdouble t ) {
  ENTER_GL();
  glMultiTexCoord2dARB( target, s, t );
  LEAVE_GL();
}

/***********************************************************************
 *              glMultiTexCoord2dvARB
 */
void WINAPI wine_glMultiTexCoord2dvARB( GLenum target, GLdouble* v ) {
  ENTER_GL();
  glMultiTexCoord2dvARB( target, v );
  LEAVE_GL();
}

/***********************************************************************
 *              glMultiTexCoord2fARB
 */
void WINAPI wine_glMultiTexCoord2fARB( GLenum target, GLfloat s, GLfloat t ) {
  ENTER_GL();
  glMultiTexCoord2fARB( target, s, t );
  LEAVE_GL();
}

/***********************************************************************
 *              glMultiTexCoord2fvARB
 */
void WINAPI wine_glMultiTexCoord2fvARB( GLenum target, GLfloat* v ) {
  ENTER_GL();
  glMultiTexCoord2fvARB( target, v );
  LEAVE_GL();
}

/***********************************************************************
 *              glMultiTexCoord2iARB
 */
void WINAPI wine_glMultiTexCoord2iARB( GLenum target, GLint s, GLint t ) {
  ENTER_GL();
  glMultiTexCoord2iARB( target, s, t );
  LEAVE_GL();
}

/***********************************************************************
 *              glMultiTexCoord2ivARB
 */
void WINAPI wine_glMultiTexCoord2ivARB( GLenum target, GLint* v ) {
  ENTER_GL();
  glMultiTexCoord2ivARB( target, v );
  LEAVE_GL();
}

/***********************************************************************
 *              glMultiTexCoord2sARB
 */
void WINAPI wine_glMultiTexCoord2sARB( GLenum target, GLshort s, GLshort t ) {
  ENTER_GL();
  glMultiTexCoord2sARB( target, s, t );
  LEAVE_GL();
}

/***********************************************************************
 *              glMultiTexCoord2svARB
 */
void WINAPI wine_glMultiTexCoord2svARB( GLenum target, GLshort* v ) {
  ENTER_GL();
  glMultiTexCoord2svARB( target, v );
  LEAVE_GL();
}

/***********************************************************************
 *              glMultiTexCoord3dARB
 */
void WINAPI wine_glMultiTexCoord3dARB( GLenum target, GLdouble s, GLdouble t, GLdouble r ) {
  ENTER_GL();
  glMultiTexCoord3dARB( target, s, t, r );
  LEAVE_GL();
}

/***********************************************************************
 *              glMultiTexCoord3dvARB
 */
void WINAPI wine_glMultiTexCoord3dvARB( GLenum target, GLdouble* v ) {
  ENTER_GL();
  glMultiTexCoord3dvARB( target, v );
  LEAVE_GL();
}

/***********************************************************************
 *              glMultiTexCoord3fARB
 */
void WINAPI wine_glMultiTexCoord3fARB( GLenum target, GLfloat s, GLfloat t, GLfloat r ) {
  ENTER_GL();
  glMultiTexCoord3fARB( target, s, t, r );
  LEAVE_GL();
}

/***********************************************************************
 *              glMultiTexCoord3fvARB
 */
void WINAPI wine_glMultiTexCoord3fvARB( GLenum target, GLfloat* v ) {
  ENTER_GL();
  glMultiTexCoord3fvARB( target, v );
  LEAVE_GL();
}

/***********************************************************************
 *              glMultiTexCoord3iARB
 */
void WINAPI wine_glMultiTexCoord3iARB( GLenum target, GLint s, GLint t, GLint r ) {
  ENTER_GL();
  glMultiTexCoord3iARB( target, s, t, r );
  LEAVE_GL();
}

/***********************************************************************
 *              glMultiTexCoord3ivARB
 */
void WINAPI wine_glMultiTexCoord3ivARB( GLenum target, GLint* v ) {
  ENTER_GL();
  glMultiTexCoord3ivARB( target, v );
  LEAVE_GL();
}

/***********************************************************************
 *              glMultiTexCoord3sARB
 */
void WINAPI wine_glMultiTexCoord3sARB( GLenum target, GLshort s, GLshort t, GLshort r ) {
  ENTER_GL();
  glMultiTexCoord3sARB( target, s, t, r );
  LEAVE_GL();
}

/***********************************************************************
 *              glMultiTexCoord3svARB
 */
void WINAPI wine_glMultiTexCoord3svARB( GLenum target, GLshort* v ) {
  ENTER_GL();
  glMultiTexCoord3svARB( target, v );
  LEAVE_GL();
}

/***********************************************************************
 *              glMultiTexCoord4dARB
 */
void WINAPI wine_glMultiTexCoord4dARB( GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q ) {
  ENTER_GL();
  glMultiTexCoord4dARB( target, s, t, r, q );
  LEAVE_GL();
}

/***********************************************************************
 *              glMultiTexCoord4dvARB
 */
void WINAPI wine_glMultiTexCoord4dvARB( GLenum target, GLdouble* v ) {
  ENTER_GL();
  glMultiTexCoord4dvARB( target, v );
  LEAVE_GL();
}

/***********************************************************************
 *              glMultiTexCoord4fARB
 */
void WINAPI wine_glMultiTexCoord4fARB( GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q ) {
  ENTER_GL();
  glMultiTexCoord4fARB( target, s, t, r, q );
  LEAVE_GL();
}

/***********************************************************************
 *              glMultiTexCoord4fvARB
 */
void WINAPI wine_glMultiTexCoord4fvARB( GLenum target, GLfloat* v ) {
  ENTER_GL();
  glMultiTexCoord4fvARB( target, v );
  LEAVE_GL();
}

/***********************************************************************
 *              glMultiTexCoord4iARB
 */
void WINAPI wine_glMultiTexCoord4iARB( GLenum target, GLint s, GLint t, GLint r, GLint q ) {
  ENTER_GL();
  glMultiTexCoord4iARB( target, s, t, r, q );
  LEAVE_GL();
}

/***********************************************************************
 *              glMultiTexCoord4ivARB
 */
void WINAPI wine_glMultiTexCoord4ivARB( GLenum target, GLint* v ) {
  ENTER_GL();
  glMultiTexCoord4ivARB( target, v );
  LEAVE_GL();
}

/***********************************************************************
 *              glMultiTexCoord4sARB
 */
void WINAPI wine_glMultiTexCoord4sARB( GLenum target, GLshort s, GLshort t, GLshort r, GLshort q ) {
  ENTER_GL();
  glMultiTexCoord4sARB( target, s, t, r, q );
  LEAVE_GL();
}

/***********************************************************************
 *              glMultiTexCoord4svARB
 */
void WINAPI wine_glMultiTexCoord4svARB( GLenum target, GLshort* v ) {
  ENTER_GL();
  glMultiTexCoord4svARB( target, v );
  LEAVE_GL();
}

/***********************************************************************
 *              glNewList
 */
void WINAPI wine_glNewList( GLuint list, GLenum mode ) {
  ENTER_GL();
  glNewList( list, mode );
  LEAVE_GL();
}

/***********************************************************************
 *              glNormal3b
 */
void WINAPI wine_glNormal3b( GLbyte nx, GLbyte ny, GLbyte nz ) {
  ENTER_GL();
  glNormal3b( nx, ny, nz );
  LEAVE_GL();
}

/***********************************************************************
 *              glNormal3bv
 */
void WINAPI wine_glNormal3bv( GLbyte* v ) {
  ENTER_GL();
  glNormal3bv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glNormal3d
 */
void WINAPI wine_glNormal3d( GLdouble nx, GLdouble ny, GLdouble nz ) {
  ENTER_GL();
  glNormal3d( nx, ny, nz );
  LEAVE_GL();
}

/***********************************************************************
 *              glNormal3dv
 */
void WINAPI wine_glNormal3dv( GLdouble* v ) {
  ENTER_GL();
  glNormal3dv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glNormal3f
 */
void WINAPI wine_glNormal3f( GLfloat nx, GLfloat ny, GLfloat nz ) {
  ENTER_GL();
  glNormal3f( nx, ny, nz );
  LEAVE_GL();
}

/***********************************************************************
 *              glNormal3fv
 */
void WINAPI wine_glNormal3fv( GLfloat* v ) {
  ENTER_GL();
  glNormal3fv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glNormal3i
 */
void WINAPI wine_glNormal3i( GLint nx, GLint ny, GLint nz ) {
  ENTER_GL();
  glNormal3i( nx, ny, nz );
  LEAVE_GL();
}

/***********************************************************************
 *              glNormal3iv
 */
void WINAPI wine_glNormal3iv( GLint* v ) {
  ENTER_GL();
  glNormal3iv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glNormal3s
 */
void WINAPI wine_glNormal3s( GLshort nx, GLshort ny, GLshort nz ) {
  ENTER_GL();
  glNormal3s( nx, ny, nz );
  LEAVE_GL();
}

/***********************************************************************
 *              glNormal3sv
 */
void WINAPI wine_glNormal3sv( GLshort* v ) {
  ENTER_GL();
  glNormal3sv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glNormalPointer
 */
void WINAPI wine_glNormalPointer( GLenum type, GLsizei stride, GLvoid* pointer ) {
  ENTER_GL();
  glNormalPointer( type, stride, pointer );
  LEAVE_GL();
}

/***********************************************************************
 *              glOrtho
 */
void WINAPI wine_glOrtho( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar ) {
  ENTER_GL();
  glOrtho( left, right, bottom, top, zNear, zFar );
  LEAVE_GL();
}

/***********************************************************************
 *              glPassThrough
 */
void WINAPI wine_glPassThrough( GLfloat token ) {
  ENTER_GL();
  glPassThrough( token );
  LEAVE_GL();
}

/***********************************************************************
 *              glPixelMapfv
 */
void WINAPI wine_glPixelMapfv( GLenum map, GLint mapsize, GLfloat* values ) {
  ENTER_GL();
  glPixelMapfv( map, mapsize, values );
  LEAVE_GL();
}

/***********************************************************************
 *              glPixelMapuiv
 */
void WINAPI wine_glPixelMapuiv( GLenum map, GLint mapsize, GLuint* values ) {
  ENTER_GL();
  glPixelMapuiv( map, mapsize, values );
  LEAVE_GL();
}

/***********************************************************************
 *              glPixelMapusv
 */
void WINAPI wine_glPixelMapusv( GLenum map, GLint mapsize, GLushort* values ) {
  ENTER_GL();
  glPixelMapusv( map, mapsize, values );
  LEAVE_GL();
}

/***********************************************************************
 *              glPixelStoref
 */
void WINAPI wine_glPixelStoref( GLenum pname, GLfloat param ) {
  ENTER_GL();
  glPixelStoref( pname, param );
  LEAVE_GL();
}

/***********************************************************************
 *              glPixelStorei
 */
void WINAPI wine_glPixelStorei( GLenum pname, GLint param ) {
  ENTER_GL();
  glPixelStorei( pname, param );
  LEAVE_GL();
}

/***********************************************************************
 *              glPixelTransferf
 */
void WINAPI wine_glPixelTransferf( GLenum pname, GLfloat param ) {
  ENTER_GL();
  glPixelTransferf( pname, param );
  LEAVE_GL();
}

/***********************************************************************
 *              glPixelTransferi
 */
void WINAPI wine_glPixelTransferi( GLenum pname, GLint param ) {
  ENTER_GL();
  glPixelTransferi( pname, param );
  LEAVE_GL();
}

/***********************************************************************
 *              glPixelZoom
 */
void WINAPI wine_glPixelZoom( GLfloat xfactor, GLfloat yfactor ) {
  ENTER_GL();
  glPixelZoom( xfactor, yfactor );
  LEAVE_GL();
}

/***********************************************************************
 *              glPointSize
 */
void WINAPI wine_glPointSize( GLfloat size ) {
  ENTER_GL();
  glPointSize( size );
  LEAVE_GL();
}

/***********************************************************************
 *              glPolygonMode
 */
void WINAPI wine_glPolygonMode( GLenum face, GLenum mode ) {
  ENTER_GL();
  glPolygonMode( face, mode );
  LEAVE_GL();
}

/***********************************************************************
 *              glPolygonOffset
 */
void WINAPI wine_glPolygonOffset( GLfloat factor, GLfloat units ) {
  ENTER_GL();
  glPolygonOffset( factor, units );
  LEAVE_GL();
}

/***********************************************************************
 *              glPolygonStipple
 */
void WINAPI wine_glPolygonStipple( GLubyte* mask ) {
  ENTER_GL();
  glPolygonStipple( mask );
  LEAVE_GL();
}

/***********************************************************************
 *              glPopAttrib
 */
void WINAPI wine_glPopAttrib( ) {
  ENTER_GL();
  glPopAttrib( );
  LEAVE_GL();
}

/***********************************************************************
 *              glPopClientAttrib
 */
void WINAPI wine_glPopClientAttrib( ) {
  ENTER_GL();
  glPopClientAttrib( );
  LEAVE_GL();
}

/***********************************************************************
 *              glPopMatrix
 */
void WINAPI wine_glPopMatrix( ) {
  ENTER_GL();
  glPopMatrix( );
  LEAVE_GL();
}

/***********************************************************************
 *              glPopName
 */
void WINAPI wine_glPopName( ) {
  ENTER_GL();
  glPopName( );
  LEAVE_GL();
}

/***********************************************************************
 *              glPrioritizeTextures
 */
void WINAPI wine_glPrioritizeTextures( GLsizei n, GLuint* textures, GLclampf* priorities ) {
  ENTER_GL();
  glPrioritizeTextures( n, textures, priorities );
  LEAVE_GL();
}

/***********************************************************************
 *              glPushAttrib
 */
void WINAPI wine_glPushAttrib( GLbitfield mask ) {
  ENTER_GL();
  glPushAttrib( mask );
  LEAVE_GL();
}

/***********************************************************************
 *              glPushClientAttrib
 */
void WINAPI wine_glPushClientAttrib( GLbitfield mask ) {
  ENTER_GL();
  glPushClientAttrib( mask );
  LEAVE_GL();
}

/***********************************************************************
 *              glPushMatrix
 */
void WINAPI wine_glPushMatrix( ) {
  ENTER_GL();
  glPushMatrix( );
  LEAVE_GL();
}

/***********************************************************************
 *              glPushName
 */
void WINAPI wine_glPushName( GLuint name ) {
  ENTER_GL();
  glPushName( name );
  LEAVE_GL();
}

/***********************************************************************
 *              glRasterPos2d
 */
void WINAPI wine_glRasterPos2d( GLdouble x, GLdouble y ) {
  ENTER_GL();
  glRasterPos2d( x, y );
  LEAVE_GL();
}

/***********************************************************************
 *              glRasterPos2dv
 */
void WINAPI wine_glRasterPos2dv( GLdouble* v ) {
  ENTER_GL();
  glRasterPos2dv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glRasterPos2f
 */
void WINAPI wine_glRasterPos2f( GLfloat x, GLfloat y ) {
  ENTER_GL();
  glRasterPos2f( x, y );
  LEAVE_GL();
}

/***********************************************************************
 *              glRasterPos2fv
 */
void WINAPI wine_glRasterPos2fv( GLfloat* v ) {
  ENTER_GL();
  glRasterPos2fv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glRasterPos2i
 */
void WINAPI wine_glRasterPos2i( GLint x, GLint y ) {
  ENTER_GL();
  glRasterPos2i( x, y );
  LEAVE_GL();
}

/***********************************************************************
 *              glRasterPos2iv
 */
void WINAPI wine_glRasterPos2iv( GLint* v ) {
  ENTER_GL();
  glRasterPos2iv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glRasterPos2s
 */
void WINAPI wine_glRasterPos2s( GLshort x, GLshort y ) {
  ENTER_GL();
  glRasterPos2s( x, y );
  LEAVE_GL();
}

/***********************************************************************
 *              glRasterPos2sv
 */
void WINAPI wine_glRasterPos2sv( GLshort* v ) {
  ENTER_GL();
  glRasterPos2sv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glRasterPos3d
 */
void WINAPI wine_glRasterPos3d( GLdouble x, GLdouble y, GLdouble z ) {
  ENTER_GL();
  glRasterPos3d( x, y, z );
  LEAVE_GL();
}

/***********************************************************************
 *              glRasterPos3dv
 */
void WINAPI wine_glRasterPos3dv( GLdouble* v ) {
  ENTER_GL();
  glRasterPos3dv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glRasterPos3f
 */
void WINAPI wine_glRasterPos3f( GLfloat x, GLfloat y, GLfloat z ) {
  ENTER_GL();
  glRasterPos3f( x, y, z );
  LEAVE_GL();
}

/***********************************************************************
 *              glRasterPos3fv
 */
void WINAPI wine_glRasterPos3fv( GLfloat* v ) {
  ENTER_GL();
  glRasterPos3fv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glRasterPos3i
 */
void WINAPI wine_glRasterPos3i( GLint x, GLint y, GLint z ) {
  ENTER_GL();
  glRasterPos3i( x, y, z );
  LEAVE_GL();
}

/***********************************************************************
 *              glRasterPos3iv
 */
void WINAPI wine_glRasterPos3iv( GLint* v ) {
  ENTER_GL();
  glRasterPos3iv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glRasterPos3s
 */
void WINAPI wine_glRasterPos3s( GLshort x, GLshort y, GLshort z ) {
  ENTER_GL();
  glRasterPos3s( x, y, z );
  LEAVE_GL();
}

/***********************************************************************
 *              glRasterPos3sv
 */
void WINAPI wine_glRasterPos3sv( GLshort* v ) {
  ENTER_GL();
  glRasterPos3sv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glRasterPos4d
 */
void WINAPI wine_glRasterPos4d( GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  ENTER_GL();
  glRasterPos4d( x, y, z, w );
  LEAVE_GL();
}

/***********************************************************************
 *              glRasterPos4dv
 */
void WINAPI wine_glRasterPos4dv( GLdouble* v ) {
  ENTER_GL();
  glRasterPos4dv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glRasterPos4f
 */
void WINAPI wine_glRasterPos4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  ENTER_GL();
  glRasterPos4f( x, y, z, w );
  LEAVE_GL();
}

/***********************************************************************
 *              glRasterPos4fv
 */
void WINAPI wine_glRasterPos4fv( GLfloat* v ) {
  ENTER_GL();
  glRasterPos4fv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glRasterPos4i
 */
void WINAPI wine_glRasterPos4i( GLint x, GLint y, GLint z, GLint w ) {
  ENTER_GL();
  glRasterPos4i( x, y, z, w );
  LEAVE_GL();
}

/***********************************************************************
 *              glRasterPos4iv
 */
void WINAPI wine_glRasterPos4iv( GLint* v ) {
  ENTER_GL();
  glRasterPos4iv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glRasterPos4s
 */
void WINAPI wine_glRasterPos4s( GLshort x, GLshort y, GLshort z, GLshort w ) {
  ENTER_GL();
  glRasterPos4s( x, y, z, w );
  LEAVE_GL();
}

/***********************************************************************
 *              glRasterPos4sv
 */
void WINAPI wine_glRasterPos4sv( GLshort* v ) {
  ENTER_GL();
  glRasterPos4sv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glReadBuffer
 */
void WINAPI wine_glReadBuffer( GLenum mode ) {
  ENTER_GL();
  glReadBuffer( mode );
  LEAVE_GL();
}

/***********************************************************************
 *              glReadPixels
 */
void WINAPI wine_glReadPixels( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels ) {
  ENTER_GL();
  glReadPixels( x, y, width, height, format, type, pixels );
  LEAVE_GL();
}

/***********************************************************************
 *              glRectd
 */
void WINAPI wine_glRectd( GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2 ) {
  ENTER_GL();
  glRectd( x1, y1, x2, y2 );
  LEAVE_GL();
}

/***********************************************************************
 *              glRectdv
 */
void WINAPI wine_glRectdv( GLdouble* v1, GLdouble* v2 ) {
  ENTER_GL();
  glRectdv( v1, v2 );
  LEAVE_GL();
}

/***********************************************************************
 *              glRectf
 */
void WINAPI wine_glRectf( GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 ) {
  ENTER_GL();
  glRectf( x1, y1, x2, y2 );
  LEAVE_GL();
}

/***********************************************************************
 *              glRectfv
 */
void WINAPI wine_glRectfv( GLfloat* v1, GLfloat* v2 ) {
  ENTER_GL();
  glRectfv( v1, v2 );
  LEAVE_GL();
}

/***********************************************************************
 *              glRecti
 */
void WINAPI wine_glRecti( GLint x1, GLint y1, GLint x2, GLint y2 ) {
  ENTER_GL();
  glRecti( x1, y1, x2, y2 );
  LEAVE_GL();
}

/***********************************************************************
 *              glRectiv
 */
void WINAPI wine_glRectiv( GLint* v1, GLint* v2 ) {
  ENTER_GL();
  glRectiv( v1, v2 );
  LEAVE_GL();
}

/***********************************************************************
 *              glRects
 */
void WINAPI wine_glRects( GLshort x1, GLshort y1, GLshort x2, GLshort y2 ) {
  ENTER_GL();
  glRects( x1, y1, x2, y2 );
  LEAVE_GL();
}

/***********************************************************************
 *              glRectsv
 */
void WINAPI wine_glRectsv( GLshort* v1, GLshort* v2 ) {
  ENTER_GL();
  glRectsv( v1, v2 );
  LEAVE_GL();
}

/***********************************************************************
 *              glRenderMode
 */
GLint WINAPI wine_glRenderMode( GLenum mode ) {
  GLint ret_value;
  ENTER_GL();
  ret_value = glRenderMode( mode );
  LEAVE_GL();
  return ret_value;
}

/***********************************************************************
 *              glResetHistogram
 */
void WINAPI wine_glResetHistogram( GLenum target ) {
  ENTER_GL();
  glResetHistogram( target );
  LEAVE_GL();
}

/***********************************************************************
 *              glResetMinmax
 */
void WINAPI wine_glResetMinmax( GLenum target ) {
  ENTER_GL();
  glResetMinmax( target );
  LEAVE_GL();
}

/***********************************************************************
 *              glRotated
 */
void WINAPI wine_glRotated( GLdouble angle, GLdouble x, GLdouble y, GLdouble z ) {
  ENTER_GL();
  glRotated( angle, x, y, z );
  LEAVE_GL();
}

/***********************************************************************
 *              glRotatef
 */
void WINAPI wine_glRotatef( GLfloat angle, GLfloat x, GLfloat y, GLfloat z ) {
  ENTER_GL();
  glRotatef( angle, x, y, z );
  LEAVE_GL();
}

/***********************************************************************
 *              glScaled
 */
void WINAPI wine_glScaled( GLdouble x, GLdouble y, GLdouble z ) {
  ENTER_GL();
  glScaled( x, y, z );
  LEAVE_GL();
}

/***********************************************************************
 *              glScalef
 */
void WINAPI wine_glScalef( GLfloat x, GLfloat y, GLfloat z ) {
  ENTER_GL();
  glScalef( x, y, z );
  LEAVE_GL();
}

/***********************************************************************
 *              glScissor
 */
void WINAPI wine_glScissor( GLint x, GLint y, GLsizei width, GLsizei height ) {
  ENTER_GL();
  glScissor( x, y, width, height );
  LEAVE_GL();
}

/***********************************************************************
 *              glSelectBuffer
 */
void WINAPI wine_glSelectBuffer( GLsizei size, GLuint* buffer ) {
  ENTER_GL();
  glSelectBuffer( size, buffer );
  LEAVE_GL();
}

/***********************************************************************
 *              glSeparableFilter2D
 */
void WINAPI wine_glSeparableFilter2D( GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* row, GLvoid* column ) {
  ENTER_GL();
  glSeparableFilter2D( target, internalformat, width, height, format, type, row, column );
  LEAVE_GL();
}

/***********************************************************************
 *              glShadeModel
 */
void WINAPI wine_glShadeModel( GLenum mode ) {
  ENTER_GL();
  glShadeModel( mode );
  LEAVE_GL();
}

/***********************************************************************
 *              glStencilFunc
 */
void WINAPI wine_glStencilFunc( GLenum func, GLint ref, GLuint mask ) {
  ENTER_GL();
  glStencilFunc( func, ref, mask );
  LEAVE_GL();
}

/***********************************************************************
 *              glStencilMask
 */
void WINAPI wine_glStencilMask( GLuint mask ) {
  ENTER_GL();
  glStencilMask( mask );
  LEAVE_GL();
}

/***********************************************************************
 *              glStencilOp
 */
void WINAPI wine_glStencilOp( GLenum fail, GLenum zfail, GLenum zpass ) {
  ENTER_GL();
  glStencilOp( fail, zfail, zpass );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexCoord1d
 */
void WINAPI wine_glTexCoord1d( GLdouble s ) {
  ENTER_GL();
  glTexCoord1d( s );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexCoord1dv
 */
void WINAPI wine_glTexCoord1dv( GLdouble* v ) {
  ENTER_GL();
  glTexCoord1dv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexCoord1f
 */
void WINAPI wine_glTexCoord1f( GLfloat s ) {
  ENTER_GL();
  glTexCoord1f( s );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexCoord1fv
 */
void WINAPI wine_glTexCoord1fv( GLfloat* v ) {
  ENTER_GL();
  glTexCoord1fv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexCoord1i
 */
void WINAPI wine_glTexCoord1i( GLint s ) {
  ENTER_GL();
  glTexCoord1i( s );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexCoord1iv
 */
void WINAPI wine_glTexCoord1iv( GLint* v ) {
  ENTER_GL();
  glTexCoord1iv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexCoord1s
 */
void WINAPI wine_glTexCoord1s( GLshort s ) {
  ENTER_GL();
  glTexCoord1s( s );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexCoord1sv
 */
void WINAPI wine_glTexCoord1sv( GLshort* v ) {
  ENTER_GL();
  glTexCoord1sv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexCoord2d
 */
void WINAPI wine_glTexCoord2d( GLdouble s, GLdouble t ) {
  ENTER_GL();
  glTexCoord2d( s, t );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexCoord2dv
 */
void WINAPI wine_glTexCoord2dv( GLdouble* v ) {
  ENTER_GL();
  glTexCoord2dv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexCoord2f
 */
void WINAPI wine_glTexCoord2f( GLfloat s, GLfloat t ) {
  ENTER_GL();
  glTexCoord2f( s, t );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexCoord2fv
 */
void WINAPI wine_glTexCoord2fv( GLfloat* v ) {
  ENTER_GL();
  glTexCoord2fv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexCoord2i
 */
void WINAPI wine_glTexCoord2i( GLint s, GLint t ) {
  ENTER_GL();
  glTexCoord2i( s, t );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexCoord2iv
 */
void WINAPI wine_glTexCoord2iv( GLint* v ) {
  ENTER_GL();
  glTexCoord2iv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexCoord2s
 */
void WINAPI wine_glTexCoord2s( GLshort s, GLshort t ) {
  ENTER_GL();
  glTexCoord2s( s, t );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexCoord2sv
 */
void WINAPI wine_glTexCoord2sv( GLshort* v ) {
  ENTER_GL();
  glTexCoord2sv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexCoord3d
 */
void WINAPI wine_glTexCoord3d( GLdouble s, GLdouble t, GLdouble r ) {
  ENTER_GL();
  glTexCoord3d( s, t, r );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexCoord3dv
 */
void WINAPI wine_glTexCoord3dv( GLdouble* v ) {
  ENTER_GL();
  glTexCoord3dv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexCoord3f
 */
void WINAPI wine_glTexCoord3f( GLfloat s, GLfloat t, GLfloat r ) {
  ENTER_GL();
  glTexCoord3f( s, t, r );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexCoord3fv
 */
void WINAPI wine_glTexCoord3fv( GLfloat* v ) {
  ENTER_GL();
  glTexCoord3fv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexCoord3i
 */
void WINAPI wine_glTexCoord3i( GLint s, GLint t, GLint r ) {
  ENTER_GL();
  glTexCoord3i( s, t, r );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexCoord3iv
 */
void WINAPI wine_glTexCoord3iv( GLint* v ) {
  ENTER_GL();
  glTexCoord3iv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexCoord3s
 */
void WINAPI wine_glTexCoord3s( GLshort s, GLshort t, GLshort r ) {
  ENTER_GL();
  glTexCoord3s( s, t, r );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexCoord3sv
 */
void WINAPI wine_glTexCoord3sv( GLshort* v ) {
  ENTER_GL();
  glTexCoord3sv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexCoord4d
 */
void WINAPI wine_glTexCoord4d( GLdouble s, GLdouble t, GLdouble r, GLdouble q ) {
  ENTER_GL();
  glTexCoord4d( s, t, r, q );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexCoord4dv
 */
void WINAPI wine_glTexCoord4dv( GLdouble* v ) {
  ENTER_GL();
  glTexCoord4dv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexCoord4f
 */
void WINAPI wine_glTexCoord4f( GLfloat s, GLfloat t, GLfloat r, GLfloat q ) {
  ENTER_GL();
  glTexCoord4f( s, t, r, q );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexCoord4fv
 */
void WINAPI wine_glTexCoord4fv( GLfloat* v ) {
  ENTER_GL();
  glTexCoord4fv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexCoord4i
 */
void WINAPI wine_glTexCoord4i( GLint s, GLint t, GLint r, GLint q ) {
  ENTER_GL();
  glTexCoord4i( s, t, r, q );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexCoord4iv
 */
void WINAPI wine_glTexCoord4iv( GLint* v ) {
  ENTER_GL();
  glTexCoord4iv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexCoord4s
 */
void WINAPI wine_glTexCoord4s( GLshort s, GLshort t, GLshort r, GLshort q ) {
  ENTER_GL();
  glTexCoord4s( s, t, r, q );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexCoord4sv
 */
void WINAPI wine_glTexCoord4sv( GLshort* v ) {
  ENTER_GL();
  glTexCoord4sv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexCoordPointer
 */
void WINAPI wine_glTexCoordPointer( GLint size, GLenum type, GLsizei stride, GLvoid* pointer ) {
  ENTER_GL();
  glTexCoordPointer( size, type, stride, pointer );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexEnvf
 */
void WINAPI wine_glTexEnvf( GLenum target, GLenum pname, GLfloat param ) {
  ENTER_GL();
  glTexEnvf( target, pname, param );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexEnvfv
 */
void WINAPI wine_glTexEnvfv( GLenum target, GLenum pname, GLfloat* params ) {
  ENTER_GL();
  glTexEnvfv( target, pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexEnvi
 */
void WINAPI wine_glTexEnvi( GLenum target, GLenum pname, GLint param ) {
  ENTER_GL();
  glTexEnvi( target, pname, param );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexEnviv
 */
void WINAPI wine_glTexEnviv( GLenum target, GLenum pname, GLint* params ) {
  ENTER_GL();
  glTexEnviv( target, pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexGend
 */
void WINAPI wine_glTexGend( GLenum coord, GLenum pname, GLdouble param ) {
  ENTER_GL();
  glTexGend( coord, pname, param );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexGendv
 */
void WINAPI wine_glTexGendv( GLenum coord, GLenum pname, GLdouble* params ) {
  ENTER_GL();
  glTexGendv( coord, pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexGenf
 */
void WINAPI wine_glTexGenf( GLenum coord, GLenum pname, GLfloat param ) {
  ENTER_GL();
  glTexGenf( coord, pname, param );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexGenfv
 */
void WINAPI wine_glTexGenfv( GLenum coord, GLenum pname, GLfloat* params ) {
  ENTER_GL();
  glTexGenfv( coord, pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexGeni
 */
void WINAPI wine_glTexGeni( GLenum coord, GLenum pname, GLint param ) {
  ENTER_GL();
  glTexGeni( coord, pname, param );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexGeniv
 */
void WINAPI wine_glTexGeniv( GLenum coord, GLenum pname, GLint* params ) {
  ENTER_GL();
  glTexGeniv( coord, pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexImage1D
 */
void WINAPI wine_glTexImage1D( GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, GLvoid* pixels ) {
  ENTER_GL();
  glTexImage1D( target, level, internalformat, width, border, format, type, pixels );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexImage2D
 */
void WINAPI wine_glTexImage2D( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, GLvoid* pixels ) {
  ENTER_GL();
  glTexImage2D( target, level, internalformat, width, height, border, format, type, pixels );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexImage3D
 */
void WINAPI wine_glTexImage3D( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, GLvoid* pixels ) {
  ENTER_GL();
  glTexImage3D( target, level, internalformat, width, height, depth, border, format, type, pixels );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexParameterf
 */
void WINAPI wine_glTexParameterf( GLenum target, GLenum pname, GLfloat param ) {
  ENTER_GL();
  glTexParameterf( target, pname, param );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexParameterfv
 */
void WINAPI wine_glTexParameterfv( GLenum target, GLenum pname, GLfloat* params ) {
  ENTER_GL();
  glTexParameterfv( target, pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexParameteri
 */
void WINAPI wine_glTexParameteri( GLenum target, GLenum pname, GLint param ) {
  ENTER_GL();
  glTexParameteri( target, pname, param );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexParameteriv
 */
void WINAPI wine_glTexParameteriv( GLenum target, GLenum pname, GLint* params ) {
  ENTER_GL();
  glTexParameteriv( target, pname, params );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexSubImage1D
 */
void WINAPI wine_glTexSubImage1D( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, GLvoid* pixels ) {
  ENTER_GL();
  glTexSubImage1D( target, level, xoffset, width, format, type, pixels );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexSubImage2D
 */
void WINAPI wine_glTexSubImage2D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels ) {
  ENTER_GL();
  glTexSubImage2D( target, level, xoffset, yoffset, width, height, format, type, pixels );
  LEAVE_GL();
}

/***********************************************************************
 *              glTexSubImage3D
 */
void WINAPI wine_glTexSubImage3D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, GLvoid* pixels ) {
  ENTER_GL();
  glTexSubImage3D( target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels );
  LEAVE_GL();
}

/***********************************************************************
 *              glTranslated
 */
void WINAPI wine_glTranslated( GLdouble x, GLdouble y, GLdouble z ) {
  ENTER_GL();
  glTranslated( x, y, z );
  LEAVE_GL();
}

/***********************************************************************
 *              glTranslatef
 */
void WINAPI wine_glTranslatef( GLfloat x, GLfloat y, GLfloat z ) {
  ENTER_GL();
  glTranslatef( x, y, z );
  LEAVE_GL();
}

/***********************************************************************
 *              glVertex2d
 */
void WINAPI wine_glVertex2d( GLdouble x, GLdouble y ) {
  ENTER_GL();
  glVertex2d( x, y );
  LEAVE_GL();
}

/***********************************************************************
 *              glVertex2dv
 */
void WINAPI wine_glVertex2dv( GLdouble* v ) {
  ENTER_GL();
  glVertex2dv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glVertex2f
 */
void WINAPI wine_glVertex2f( GLfloat x, GLfloat y ) {
  ENTER_GL();
  glVertex2f( x, y );
  LEAVE_GL();
}

/***********************************************************************
 *              glVertex2fv
 */
void WINAPI wine_glVertex2fv( GLfloat* v ) {
  ENTER_GL();
  glVertex2fv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glVertex2i
 */
void WINAPI wine_glVertex2i( GLint x, GLint y ) {
  ENTER_GL();
  glVertex2i( x, y );
  LEAVE_GL();
}

/***********************************************************************
 *              glVertex2iv
 */
void WINAPI wine_glVertex2iv( GLint* v ) {
  ENTER_GL();
  glVertex2iv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glVertex2s
 */
void WINAPI wine_glVertex2s( GLshort x, GLshort y ) {
  ENTER_GL();
  glVertex2s( x, y );
  LEAVE_GL();
}

/***********************************************************************
 *              glVertex2sv
 */
void WINAPI wine_glVertex2sv( GLshort* v ) {
  ENTER_GL();
  glVertex2sv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glVertex3d
 */
void WINAPI wine_glVertex3d( GLdouble x, GLdouble y, GLdouble z ) {
  ENTER_GL();
  glVertex3d( x, y, z );
  LEAVE_GL();
}

/***********************************************************************
 *              glVertex3dv
 */
void WINAPI wine_glVertex3dv( GLdouble* v ) {
  ENTER_GL();
  glVertex3dv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glVertex3f
 */
void WINAPI wine_glVertex3f( GLfloat x, GLfloat y, GLfloat z ) {
  ENTER_GL();
  glVertex3f( x, y, z );
  LEAVE_GL();
}

/***********************************************************************
 *              glVertex3fv
 */
void WINAPI wine_glVertex3fv( GLfloat* v ) {
  ENTER_GL();
  glVertex3fv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glVertex3i
 */
void WINAPI wine_glVertex3i( GLint x, GLint y, GLint z ) {
  ENTER_GL();
  glVertex3i( x, y, z );
  LEAVE_GL();
}

/***********************************************************************
 *              glVertex3iv
 */
void WINAPI wine_glVertex3iv( GLint* v ) {
  ENTER_GL();
  glVertex3iv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glVertex3s
 */
void WINAPI wine_glVertex3s( GLshort x, GLshort y, GLshort z ) {
  ENTER_GL();
  glVertex3s( x, y, z );
  LEAVE_GL();
}

/***********************************************************************
 *              glVertex3sv
 */
void WINAPI wine_glVertex3sv( GLshort* v ) {
  ENTER_GL();
  glVertex3sv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glVertex4d
 */
void WINAPI wine_glVertex4d( GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  ENTER_GL();
  glVertex4d( x, y, z, w );
  LEAVE_GL();
}

/***********************************************************************
 *              glVertex4dv
 */
void WINAPI wine_glVertex4dv( GLdouble* v ) {
  ENTER_GL();
  glVertex4dv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glVertex4f
 */
void WINAPI wine_glVertex4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  ENTER_GL();
  glVertex4f( x, y, z, w );
  LEAVE_GL();
}

/***********************************************************************
 *              glVertex4fv
 */
void WINAPI wine_glVertex4fv( GLfloat* v ) {
  ENTER_GL();
  glVertex4fv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glVertex4i
 */
void WINAPI wine_glVertex4i( GLint x, GLint y, GLint z, GLint w ) {
  ENTER_GL();
  glVertex4i( x, y, z, w );
  LEAVE_GL();
}

/***********************************************************************
 *              glVertex4iv
 */
void WINAPI wine_glVertex4iv( GLint* v ) {
  ENTER_GL();
  glVertex4iv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glVertex4s
 */
void WINAPI wine_glVertex4s( GLshort x, GLshort y, GLshort z, GLshort w ) {
  ENTER_GL();
  glVertex4s( x, y, z, w );
  LEAVE_GL();
}

/***********************************************************************
 *              glVertex4sv
 */
void WINAPI wine_glVertex4sv( GLshort* v ) {
  ENTER_GL();
  glVertex4sv( v );
  LEAVE_GL();
}

/***********************************************************************
 *              glVertexPointer
 */
void WINAPI wine_glVertexPointer( GLint size, GLenum type, GLsizei stride, GLvoid* pointer ) {
  ENTER_GL();
  glVertexPointer( size, type, stride, pointer );
  LEAVE_GL();
}

/***********************************************************************
 *              glViewport
 */
void WINAPI wine_glViewport( GLint x, GLint y, GLsizei width, GLsizei height ) {
  ENTER_GL();
  glViewport( x, y, width, height );
  LEAVE_GL();
}

