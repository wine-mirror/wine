/* Automatically generated from http://www.opengl.org/registry files; DO NOT EDIT! */

#ifndef __WINE_WGL_DRIVER_H
#define __WINE_WGL_DRIVER_H

#define WINE_WGL_DRIVER_VERSION 29

struct wgl_context;
struct wgl_pbuffer;

struct wgl_pixel_format
{
    PIXELFORMATDESCRIPTOR pfd;
    int swap_method;
    int transparent;
    int pixel_type;
    int draw_to_pbuffer;
    int max_pbuffer_pixels;
    int max_pbuffer_width;
    int max_pbuffer_height;
    int transparent_red_value;
    int transparent_red_value_valid;
    int transparent_green_value;
    int transparent_green_value_valid;
    int transparent_blue_value;
    int transparent_blue_value_valid;
    int transparent_alpha_value;
    int transparent_alpha_value_valid;
    int transparent_index_value;
    int transparent_index_value_valid;
    int sample_buffers;
    int samples;
    int bind_to_texture_rgb;
    int bind_to_texture_rgba;
    int bind_to_texture_rectangle_rgb;
    int bind_to_texture_rectangle_rgba;
    int framebuffer_srgb_capable;
    int float_components;
};

struct opengl_funcs
{
    BOOL       (*p_wglCopyContext)( struct wgl_context * hglrcSrc, struct wgl_context * hglrcDst, UINT mask );
    struct wgl_context * (*p_wglCreateContext)( HDC hDc );
    BOOL       (*p_wglDeleteContext)( struct wgl_context * oldContext );
    int        (*p_wglGetPixelFormat)( HDC hdc );
    PROC       (*p_wglGetProcAddress)( LPCSTR lpszProc );
    BOOL       (*p_wglMakeCurrent)( HDC hDc, struct wgl_context * newContext );
    BOOL       (*p_wglSetPixelFormat)( HDC hdc, int ipfd, const PIXELFORMATDESCRIPTOR *ppfd );
    BOOL       (*p_wglShareLists)( struct wgl_context * hrcSrvShare, struct wgl_context * hrcSrvSource );
    BOOL       (*p_wglSwapBuffers)( HDC hdc );
    void       (*p_get_pixel_formats)( struct wgl_pixel_format *formats, UINT max_formats, UINT *num_formats, UINT *num_onscreen_formats );
    void       (*p_glAccum)( GLenum op, GLfloat value );
    void       (*p_glAlphaFunc)( GLenum func, GLfloat ref );
    GLboolean  (*p_glAreTexturesResident)( GLsizei n, const GLuint *textures, GLboolean *residences );
    void       (*p_glArrayElement)( GLint i );
    void       (*p_glBegin)( GLenum mode );
    void       (*p_glBindTexture)( GLenum target, GLuint texture );
    void       (*p_glBitmap)( GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap );
    void       (*p_glBlendFunc)( GLenum sfactor, GLenum dfactor );
    void       (*p_glCallList)( GLuint list );
    void       (*p_glCallLists)( GLsizei n, GLenum type, const void *lists );
    void       (*p_glClear)( GLbitfield mask );
    void       (*p_glClearAccum)( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha );
    void       (*p_glClearColor)( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha );
    void       (*p_glClearDepth)( GLdouble depth );
    void       (*p_glClearIndex)( GLfloat c );
    void       (*p_glClearStencil)( GLint s );
    void       (*p_glClipPlane)( GLenum plane, const GLdouble *equation );
    void       (*p_glColor3b)( GLbyte red, GLbyte green, GLbyte blue );
    void       (*p_glColor3bv)( const GLbyte *v );
    void       (*p_glColor3d)( GLdouble red, GLdouble green, GLdouble blue );
    void       (*p_glColor3dv)( const GLdouble *v );
    void       (*p_glColor3f)( GLfloat red, GLfloat green, GLfloat blue );
    void       (*p_glColor3fv)( const GLfloat *v );
    void       (*p_glColor3i)( GLint red, GLint green, GLint blue );
    void       (*p_glColor3iv)( const GLint *v );
    void       (*p_glColor3s)( GLshort red, GLshort green, GLshort blue );
    void       (*p_glColor3sv)( const GLshort *v );
    void       (*p_glColor3ub)( GLubyte red, GLubyte green, GLubyte blue );
    void       (*p_glColor3ubv)( const GLubyte *v );
    void       (*p_glColor3ui)( GLuint red, GLuint green, GLuint blue );
    void       (*p_glColor3uiv)( const GLuint *v );
    void       (*p_glColor3us)( GLushort red, GLushort green, GLushort blue );
    void       (*p_glColor3usv)( const GLushort *v );
    void       (*p_glColor4b)( GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha );
    void       (*p_glColor4bv)( const GLbyte *v );
    void       (*p_glColor4d)( GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha );
    void       (*p_glColor4dv)( const GLdouble *v );
    void       (*p_glColor4f)( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha );
    void       (*p_glColor4fv)( const GLfloat *v );
    void       (*p_glColor4i)( GLint red, GLint green, GLint blue, GLint alpha );
    void       (*p_glColor4iv)( const GLint *v );
    void       (*p_glColor4s)( GLshort red, GLshort green, GLshort blue, GLshort alpha );
    void       (*p_glColor4sv)( const GLshort *v );
    void       (*p_glColor4ub)( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha );
    void       (*p_glColor4ubv)( const GLubyte *v );
    void       (*p_glColor4ui)( GLuint red, GLuint green, GLuint blue, GLuint alpha );
    void       (*p_glColor4uiv)( const GLuint *v );
    void       (*p_glColor4us)( GLushort red, GLushort green, GLushort blue, GLushort alpha );
    void       (*p_glColor4usv)( const GLushort *v );
    void       (*p_glColorMask)( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha );
    void       (*p_glColorMaterial)( GLenum face, GLenum mode );
    void       (*p_glColorPointer)( GLint size, GLenum type, GLsizei stride, const void *pointer );
    void       (*p_glCopyPixels)( GLint x, GLint y, GLsizei width, GLsizei height, GLenum type );
    void       (*p_glCopyTexImage1D)( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border );
    void       (*p_glCopyTexImage2D)( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border );
    void       (*p_glCopyTexSubImage1D)( GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width );
    void       (*p_glCopyTexSubImage2D)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height );
    void       (*p_glCullFace)( GLenum mode );
    void       (*p_glDeleteLists)( GLuint list, GLsizei range );
    void       (*p_glDeleteTextures)( GLsizei n, const GLuint *textures );
    void       (*p_glDepthFunc)( GLenum func );
    void       (*p_glDepthMask)( GLboolean flag );
    void       (*p_glDepthRange)( GLdouble n, GLdouble f );
    void       (*p_glDisable)( GLenum cap );
    void       (*p_glDisableClientState)( GLenum array );
    void       (*p_glDrawArrays)( GLenum mode, GLint first, GLsizei count );
    void       (*p_glDrawBuffer)( GLenum buf );
    void       (*p_glDrawElements)( GLenum mode, GLsizei count, GLenum type, const void *indices );
    void       (*p_glDrawPixels)( GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels );
    void       (*p_glEdgeFlag)( GLboolean flag );
    void       (*p_glEdgeFlagPointer)( GLsizei stride, const void *pointer );
    void       (*p_glEdgeFlagv)( const GLboolean *flag );
    void       (*p_glEnable)( GLenum cap );
    void       (*p_glEnableClientState)( GLenum array );
    void       (*p_glEnd)(void);
    void       (*p_glEndList)(void);
    void       (*p_glEvalCoord1d)( GLdouble u );
    void       (*p_glEvalCoord1dv)( const GLdouble *u );
    void       (*p_glEvalCoord1f)( GLfloat u );
    void       (*p_glEvalCoord1fv)( const GLfloat *u );
    void       (*p_glEvalCoord2d)( GLdouble u, GLdouble v );
    void       (*p_glEvalCoord2dv)( const GLdouble *u );
    void       (*p_glEvalCoord2f)( GLfloat u, GLfloat v );
    void       (*p_glEvalCoord2fv)( const GLfloat *u );
    void       (*p_glEvalMesh1)( GLenum mode, GLint i1, GLint i2 );
    void       (*p_glEvalMesh2)( GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2 );
    void       (*p_glEvalPoint1)( GLint i );
    void       (*p_glEvalPoint2)( GLint i, GLint j );
    void       (*p_glFeedbackBuffer)( GLsizei size, GLenum type, GLfloat *buffer );
    void       (*p_glFinish)(void);
    void       (*p_glFlush)(void);
    void       (*p_glFogf)( GLenum pname, GLfloat param );
    void       (*p_glFogfv)( GLenum pname, const GLfloat *params );
    void       (*p_glFogi)( GLenum pname, GLint param );
    void       (*p_glFogiv)( GLenum pname, const GLint *params );
    void       (*p_glFrontFace)( GLenum mode );
    void       (*p_glFrustum)( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar );
    GLuint     (*p_glGenLists)( GLsizei range );
    void       (*p_glGenTextures)( GLsizei n, GLuint *textures );
    void       (*p_glGetBooleanv)( GLenum pname, GLboolean *data );
    void       (*p_glGetClipPlane)( GLenum plane, GLdouble *equation );
    void       (*p_glGetDoublev)( GLenum pname, GLdouble *data );
    GLenum     (*p_glGetError)(void);
    void       (*p_glGetFloatv)( GLenum pname, GLfloat *data );
    void       (*p_glGetIntegerv)( GLenum pname, GLint *data );
    void       (*p_glGetLightfv)( GLenum light, GLenum pname, GLfloat *params );
    void       (*p_glGetLightiv)( GLenum light, GLenum pname, GLint *params );
    void       (*p_glGetMapdv)( GLenum target, GLenum query, GLdouble *v );
    void       (*p_glGetMapfv)( GLenum target, GLenum query, GLfloat *v );
    void       (*p_glGetMapiv)( GLenum target, GLenum query, GLint *v );
    void       (*p_glGetMaterialfv)( GLenum face, GLenum pname, GLfloat *params );
    void       (*p_glGetMaterialiv)( GLenum face, GLenum pname, GLint *params );
    void       (*p_glGetPixelMapfv)( GLenum map, GLfloat *values );
    void       (*p_glGetPixelMapuiv)( GLenum map, GLuint *values );
    void       (*p_glGetPixelMapusv)( GLenum map, GLushort *values );
    void       (*p_glGetPointerv)( GLenum pname, void **params );
    void       (*p_glGetPolygonStipple)( GLubyte *mask );
    const GLubyte * (*p_glGetString)( GLenum name );
    void       (*p_glGetTexEnvfv)( GLenum target, GLenum pname, GLfloat *params );
    void       (*p_glGetTexEnviv)( GLenum target, GLenum pname, GLint *params );
    void       (*p_glGetTexGendv)( GLenum coord, GLenum pname, GLdouble *params );
    void       (*p_glGetTexGenfv)( GLenum coord, GLenum pname, GLfloat *params );
    void       (*p_glGetTexGeniv)( GLenum coord, GLenum pname, GLint *params );
    void       (*p_glGetTexImage)( GLenum target, GLint level, GLenum format, GLenum type, void *pixels );
    void       (*p_glGetTexLevelParameterfv)( GLenum target, GLint level, GLenum pname, GLfloat *params );
    void       (*p_glGetTexLevelParameteriv)( GLenum target, GLint level, GLenum pname, GLint *params );
    void       (*p_glGetTexParameterfv)( GLenum target, GLenum pname, GLfloat *params );
    void       (*p_glGetTexParameteriv)( GLenum target, GLenum pname, GLint *params );
    void       (*p_glHint)( GLenum target, GLenum mode );
    void       (*p_glIndexMask)( GLuint mask );
    void       (*p_glIndexPointer)( GLenum type, GLsizei stride, const void *pointer );
    void       (*p_glIndexd)( GLdouble c );
    void       (*p_glIndexdv)( const GLdouble *c );
    void       (*p_glIndexf)( GLfloat c );
    void       (*p_glIndexfv)( const GLfloat *c );
    void       (*p_glIndexi)( GLint c );
    void       (*p_glIndexiv)( const GLint *c );
    void       (*p_glIndexs)( GLshort c );
    void       (*p_glIndexsv)( const GLshort *c );
    void       (*p_glIndexub)( GLubyte c );
    void       (*p_glIndexubv)( const GLubyte *c );
    void       (*p_glInitNames)(void);
    void       (*p_glInterleavedArrays)( GLenum format, GLsizei stride, const void *pointer );
    GLboolean  (*p_glIsEnabled)( GLenum cap );
    GLboolean  (*p_glIsList)( GLuint list );
    GLboolean  (*p_glIsTexture)( GLuint texture );
    void       (*p_glLightModelf)( GLenum pname, GLfloat param );
    void       (*p_glLightModelfv)( GLenum pname, const GLfloat *params );
    void       (*p_glLightModeli)( GLenum pname, GLint param );
    void       (*p_glLightModeliv)( GLenum pname, const GLint *params );
    void       (*p_glLightf)( GLenum light, GLenum pname, GLfloat param );
    void       (*p_glLightfv)( GLenum light, GLenum pname, const GLfloat *params );
    void       (*p_glLighti)( GLenum light, GLenum pname, GLint param );
    void       (*p_glLightiv)( GLenum light, GLenum pname, const GLint *params );
    void       (*p_glLineStipple)( GLint factor, GLushort pattern );
    void       (*p_glLineWidth)( GLfloat width );
    void       (*p_glListBase)( GLuint base );
    void       (*p_glLoadIdentity)(void);
    void       (*p_glLoadMatrixd)( const GLdouble *m );
    void       (*p_glLoadMatrixf)( const GLfloat *m );
    void       (*p_glLoadName)( GLuint name );
    void       (*p_glLogicOp)( GLenum opcode );
    void       (*p_glMap1d)( GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points );
    void       (*p_glMap1f)( GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points );
    void       (*p_glMap2d)( GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points );
    void       (*p_glMap2f)( GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points );
    void       (*p_glMapGrid1d)( GLint un, GLdouble u1, GLdouble u2 );
    void       (*p_glMapGrid1f)( GLint un, GLfloat u1, GLfloat u2 );
    void       (*p_glMapGrid2d)( GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2 );
    void       (*p_glMapGrid2f)( GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2 );
    void       (*p_glMaterialf)( GLenum face, GLenum pname, GLfloat param );
    void       (*p_glMaterialfv)( GLenum face, GLenum pname, const GLfloat *params );
    void       (*p_glMateriali)( GLenum face, GLenum pname, GLint param );
    void       (*p_glMaterialiv)( GLenum face, GLenum pname, const GLint *params );
    void       (*p_glMatrixMode)( GLenum mode );
    void       (*p_glMultMatrixd)( const GLdouble *m );
    void       (*p_glMultMatrixf)( const GLfloat *m );
    void       (*p_glNewList)( GLuint list, GLenum mode );
    void       (*p_glNormal3b)( GLbyte nx, GLbyte ny, GLbyte nz );
    void       (*p_glNormal3bv)( const GLbyte *v );
    void       (*p_glNormal3d)( GLdouble nx, GLdouble ny, GLdouble nz );
    void       (*p_glNormal3dv)( const GLdouble *v );
    void       (*p_glNormal3f)( GLfloat nx, GLfloat ny, GLfloat nz );
    void       (*p_glNormal3fv)( const GLfloat *v );
    void       (*p_glNormal3i)( GLint nx, GLint ny, GLint nz );
    void       (*p_glNormal3iv)( const GLint *v );
    void       (*p_glNormal3s)( GLshort nx, GLshort ny, GLshort nz );
    void       (*p_glNormal3sv)( const GLshort *v );
    void       (*p_glNormalPointer)( GLenum type, GLsizei stride, const void *pointer );
    void       (*p_glOrtho)( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar );
    void       (*p_glPassThrough)( GLfloat token );
    void       (*p_glPixelMapfv)( GLenum map, GLsizei mapsize, const GLfloat *values );
    void       (*p_glPixelMapuiv)( GLenum map, GLsizei mapsize, const GLuint *values );
    void       (*p_glPixelMapusv)( GLenum map, GLsizei mapsize, const GLushort *values );
    void       (*p_glPixelStoref)( GLenum pname, GLfloat param );
    void       (*p_glPixelStorei)( GLenum pname, GLint param );
    void       (*p_glPixelTransferf)( GLenum pname, GLfloat param );
    void       (*p_glPixelTransferi)( GLenum pname, GLint param );
    void       (*p_glPixelZoom)( GLfloat xfactor, GLfloat yfactor );
    void       (*p_glPointSize)( GLfloat size );
    void       (*p_glPolygonMode)( GLenum face, GLenum mode );
    void       (*p_glPolygonOffset)( GLfloat factor, GLfloat units );
    void       (*p_glPolygonStipple)( const GLubyte *mask );
    void       (*p_glPopAttrib)(void);
    void       (*p_glPopClientAttrib)(void);
    void       (*p_glPopMatrix)(void);
    void       (*p_glPopName)(void);
    void       (*p_glPrioritizeTextures)( GLsizei n, const GLuint *textures, const GLfloat *priorities );
    void       (*p_glPushAttrib)( GLbitfield mask );
    void       (*p_glPushClientAttrib)( GLbitfield mask );
    void       (*p_glPushMatrix)(void);
    void       (*p_glPushName)( GLuint name );
    void       (*p_glRasterPos2d)( GLdouble x, GLdouble y );
    void       (*p_glRasterPos2dv)( const GLdouble *v );
    void       (*p_glRasterPos2f)( GLfloat x, GLfloat y );
    void       (*p_glRasterPos2fv)( const GLfloat *v );
    void       (*p_glRasterPos2i)( GLint x, GLint y );
    void       (*p_glRasterPos2iv)( const GLint *v );
    void       (*p_glRasterPos2s)( GLshort x, GLshort y );
    void       (*p_glRasterPos2sv)( const GLshort *v );
    void       (*p_glRasterPos3d)( GLdouble x, GLdouble y, GLdouble z );
    void       (*p_glRasterPos3dv)( const GLdouble *v );
    void       (*p_glRasterPos3f)( GLfloat x, GLfloat y, GLfloat z );
    void       (*p_glRasterPos3fv)( const GLfloat *v );
    void       (*p_glRasterPos3i)( GLint x, GLint y, GLint z );
    void       (*p_glRasterPos3iv)( const GLint *v );
    void       (*p_glRasterPos3s)( GLshort x, GLshort y, GLshort z );
    void       (*p_glRasterPos3sv)( const GLshort *v );
    void       (*p_glRasterPos4d)( GLdouble x, GLdouble y, GLdouble z, GLdouble w );
    void       (*p_glRasterPos4dv)( const GLdouble *v );
    void       (*p_glRasterPos4f)( GLfloat x, GLfloat y, GLfloat z, GLfloat w );
    void       (*p_glRasterPos4fv)( const GLfloat *v );
    void       (*p_glRasterPos4i)( GLint x, GLint y, GLint z, GLint w );
    void       (*p_glRasterPos4iv)( const GLint *v );
    void       (*p_glRasterPos4s)( GLshort x, GLshort y, GLshort z, GLshort w );
    void       (*p_glRasterPos4sv)( const GLshort *v );
    void       (*p_glReadBuffer)( GLenum src );
    void       (*p_glReadPixels)( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels );
    void       (*p_glRectd)( GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2 );
    void       (*p_glRectdv)( const GLdouble *v1, const GLdouble *v2 );
    void       (*p_glRectf)( GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 );
    void       (*p_glRectfv)( const GLfloat *v1, const GLfloat *v2 );
    void       (*p_glRecti)( GLint x1, GLint y1, GLint x2, GLint y2 );
    void       (*p_glRectiv)( const GLint *v1, const GLint *v2 );
    void       (*p_glRects)( GLshort x1, GLshort y1, GLshort x2, GLshort y2 );
    void       (*p_glRectsv)( const GLshort *v1, const GLshort *v2 );
    GLint      (*p_glRenderMode)( GLenum mode );
    void       (*p_glRotated)( GLdouble angle, GLdouble x, GLdouble y, GLdouble z );
    void       (*p_glRotatef)( GLfloat angle, GLfloat x, GLfloat y, GLfloat z );
    void       (*p_glScaled)( GLdouble x, GLdouble y, GLdouble z );
    void       (*p_glScalef)( GLfloat x, GLfloat y, GLfloat z );
    void       (*p_glScissor)( GLint x, GLint y, GLsizei width, GLsizei height );
    void       (*p_glSelectBuffer)( GLsizei size, GLuint *buffer );
    void       (*p_glShadeModel)( GLenum mode );
    void       (*p_glStencilFunc)( GLenum func, GLint ref, GLuint mask );
    void       (*p_glStencilMask)( GLuint mask );
    void       (*p_glStencilOp)( GLenum fail, GLenum zfail, GLenum zpass );
    void       (*p_glTexCoord1d)( GLdouble s );
    void       (*p_glTexCoord1dv)( const GLdouble *v );
    void       (*p_glTexCoord1f)( GLfloat s );
    void       (*p_glTexCoord1fv)( const GLfloat *v );
    void       (*p_glTexCoord1i)( GLint s );
    void       (*p_glTexCoord1iv)( const GLint *v );
    void       (*p_glTexCoord1s)( GLshort s );
    void       (*p_glTexCoord1sv)( const GLshort *v );
    void       (*p_glTexCoord2d)( GLdouble s, GLdouble t );
    void       (*p_glTexCoord2dv)( const GLdouble *v );
    void       (*p_glTexCoord2f)( GLfloat s, GLfloat t );
    void       (*p_glTexCoord2fv)( const GLfloat *v );
    void       (*p_glTexCoord2i)( GLint s, GLint t );
    void       (*p_glTexCoord2iv)( const GLint *v );
    void       (*p_glTexCoord2s)( GLshort s, GLshort t );
    void       (*p_glTexCoord2sv)( const GLshort *v );
    void       (*p_glTexCoord3d)( GLdouble s, GLdouble t, GLdouble r );
    void       (*p_glTexCoord3dv)( const GLdouble *v );
    void       (*p_glTexCoord3f)( GLfloat s, GLfloat t, GLfloat r );
    void       (*p_glTexCoord3fv)( const GLfloat *v );
    void       (*p_glTexCoord3i)( GLint s, GLint t, GLint r );
    void       (*p_glTexCoord3iv)( const GLint *v );
    void       (*p_glTexCoord3s)( GLshort s, GLshort t, GLshort r );
    void       (*p_glTexCoord3sv)( const GLshort *v );
    void       (*p_glTexCoord4d)( GLdouble s, GLdouble t, GLdouble r, GLdouble q );
    void       (*p_glTexCoord4dv)( const GLdouble *v );
    void       (*p_glTexCoord4f)( GLfloat s, GLfloat t, GLfloat r, GLfloat q );
    void       (*p_glTexCoord4fv)( const GLfloat *v );
    void       (*p_glTexCoord4i)( GLint s, GLint t, GLint r, GLint q );
    void       (*p_glTexCoord4iv)( const GLint *v );
    void       (*p_glTexCoord4s)( GLshort s, GLshort t, GLshort r, GLshort q );
    void       (*p_glTexCoord4sv)( const GLshort *v );
    void       (*p_glTexCoordPointer)( GLint size, GLenum type, GLsizei stride, const void *pointer );
    void       (*p_glTexEnvf)( GLenum target, GLenum pname, GLfloat param );
    void       (*p_glTexEnvfv)( GLenum target, GLenum pname, const GLfloat *params );
    void       (*p_glTexEnvi)( GLenum target, GLenum pname, GLint param );
    void       (*p_glTexEnviv)( GLenum target, GLenum pname, const GLint *params );
    void       (*p_glTexGend)( GLenum coord, GLenum pname, GLdouble param );
    void       (*p_glTexGendv)( GLenum coord, GLenum pname, const GLdouble *params );
    void       (*p_glTexGenf)( GLenum coord, GLenum pname, GLfloat param );
    void       (*p_glTexGenfv)( GLenum coord, GLenum pname, const GLfloat *params );
    void       (*p_glTexGeni)( GLenum coord, GLenum pname, GLint param );
    void       (*p_glTexGeniv)( GLenum coord, GLenum pname, const GLint *params );
    void       (*p_glTexImage1D)( GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels );
    void       (*p_glTexImage2D)( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels );
    void       (*p_glTexParameterf)( GLenum target, GLenum pname, GLfloat param );
    void       (*p_glTexParameterfv)( GLenum target, GLenum pname, const GLfloat *params );
    void       (*p_glTexParameteri)( GLenum target, GLenum pname, GLint param );
    void       (*p_glTexParameteriv)( GLenum target, GLenum pname, const GLint *params );
    void       (*p_glTexSubImage1D)( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels );
    void       (*p_glTexSubImage2D)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels );
    void       (*p_glTranslated)( GLdouble x, GLdouble y, GLdouble z );
    void       (*p_glTranslatef)( GLfloat x, GLfloat y, GLfloat z );
    void       (*p_glVertex2d)( GLdouble x, GLdouble y );
    void       (*p_glVertex2dv)( const GLdouble *v );
    void       (*p_glVertex2f)( GLfloat x, GLfloat y );
    void       (*p_glVertex2fv)( const GLfloat *v );
    void       (*p_glVertex2i)( GLint x, GLint y );
    void       (*p_glVertex2iv)( const GLint *v );
    void       (*p_glVertex2s)( GLshort x, GLshort y );
    void       (*p_glVertex2sv)( const GLshort *v );
    void       (*p_glVertex3d)( GLdouble x, GLdouble y, GLdouble z );
    void       (*p_glVertex3dv)( const GLdouble *v );
    void       (*p_glVertex3f)( GLfloat x, GLfloat y, GLfloat z );
    void       (*p_glVertex3fv)( const GLfloat *v );
    void       (*p_glVertex3i)( GLint x, GLint y, GLint z );
    void       (*p_glVertex3iv)( const GLint *v );
    void       (*p_glVertex3s)( GLshort x, GLshort y, GLshort z );
    void       (*p_glVertex3sv)( const GLshort *v );
    void       (*p_glVertex4d)( GLdouble x, GLdouble y, GLdouble z, GLdouble w );
    void       (*p_glVertex4dv)( const GLdouble *v );
    void       (*p_glVertex4f)( GLfloat x, GLfloat y, GLfloat z, GLfloat w );
    void       (*p_glVertex4fv)( const GLfloat *v );
    void       (*p_glVertex4i)( GLint x, GLint y, GLint z, GLint w );
    void       (*p_glVertex4iv)( const GLint *v );
    void       (*p_glVertex4s)( GLshort x, GLshort y, GLshort z, GLshort w );
    void       (*p_glVertex4sv)( const GLshort *v );
    void       (*p_glVertexPointer)( GLint size, GLenum type, GLsizei stride, const void *pointer );
    void       (*p_glViewport)( GLint x, GLint y, GLsizei width, GLsizei height );
    void       (*p_glAccumxOES)( GLenum op, GLfixed value );
    GLboolean  (*p_glAcquireKeyedMutexWin32EXT)( GLuint memory, GLuint64 key, GLuint timeout );
    void       (*p_glActiveProgramEXT)( GLuint program );
    void       (*p_glActiveShaderProgram)( GLuint pipeline, GLuint program );
    void       (*p_glActiveStencilFaceEXT)( GLenum face );
    void       (*p_glActiveTexture)( GLenum texture );
    void       (*p_glActiveTextureARB)( GLenum texture );
    void       (*p_glActiveVaryingNV)( GLuint program, const GLchar *name );
    void       (*p_glAlphaFragmentOp1ATI)( GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod );
    void       (*p_glAlphaFragmentOp2ATI)( GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod );
    void       (*p_glAlphaFragmentOp3ATI)( GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod );
    void       (*p_glAlphaFuncxOES)( GLenum func, GLfixed ref );
    void       (*p_glAlphaToCoverageDitherControlNV)( GLenum mode );
    void       (*p_glApplyFramebufferAttachmentCMAAINTEL)(void);
    void       (*p_glApplyTextureEXT)( GLenum mode );
    GLboolean  (*p_glAreProgramsResidentNV)( GLsizei n, const GLuint *programs, GLboolean *residences );
    GLboolean  (*p_glAreTexturesResidentEXT)( GLsizei n, const GLuint *textures, GLboolean *residences );
    void       (*p_glArrayElementEXT)( GLint i );
    void       (*p_glArrayObjectATI)( GLenum array, GLint size, GLenum type, GLsizei stride, GLuint buffer, GLuint offset );
    GLuint     (*p_glAsyncCopyBufferSubDataNVX)( GLsizei waitSemaphoreCount, const GLuint *waitSemaphoreArray, const GLuint64 *fenceValueArray, GLuint readGpu, GLbitfield writeGpuMask, GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size, GLsizei signalSemaphoreCount, const GLuint *signalSemaphoreArray, const GLuint64 *signalValueArray );
    GLuint     (*p_glAsyncCopyImageSubDataNVX)( GLsizei waitSemaphoreCount, const GLuint *waitSemaphoreArray, const GLuint64 *waitValueArray, GLuint srcGpu, GLbitfield dstGpuMask, GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth, GLsizei signalSemaphoreCount, const GLuint *signalSemaphoreArray, const GLuint64 *signalValueArray );
    void       (*p_glAsyncMarkerSGIX)( GLuint marker );
    void       (*p_glAttachObjectARB)( GLhandleARB containerObj, GLhandleARB obj );
    void       (*p_glAttachShader)( GLuint program, GLuint shader );
    void       (*p_glBeginConditionalRender)( GLuint id, GLenum mode );
    void       (*p_glBeginConditionalRenderNV)( GLuint id, GLenum mode );
    void       (*p_glBeginConditionalRenderNVX)( GLuint id );
    void       (*p_glBeginFragmentShaderATI)(void);
    void       (*p_glBeginOcclusionQueryNV)( GLuint id );
    void       (*p_glBeginPerfMonitorAMD)( GLuint monitor );
    void       (*p_glBeginPerfQueryINTEL)( GLuint queryHandle );
    void       (*p_glBeginQuery)( GLenum target, GLuint id );
    void       (*p_glBeginQueryARB)( GLenum target, GLuint id );
    void       (*p_glBeginQueryIndexed)( GLenum target, GLuint index, GLuint id );
    void       (*p_glBeginTransformFeedback)( GLenum primitiveMode );
    void       (*p_glBeginTransformFeedbackEXT)( GLenum primitiveMode );
    void       (*p_glBeginTransformFeedbackNV)( GLenum primitiveMode );
    void       (*p_glBeginVertexShaderEXT)(void);
    void       (*p_glBeginVideoCaptureNV)( GLuint video_capture_slot );
    void       (*p_glBindAttribLocation)( GLuint program, GLuint index, const GLchar *name );
    void       (*p_glBindAttribLocationARB)( GLhandleARB programObj, GLuint index, const GLcharARB *name );
    void       (*p_glBindBuffer)( GLenum target, GLuint buffer );
    void       (*p_glBindBufferARB)( GLenum target, GLuint buffer );
    void       (*p_glBindBufferBase)( GLenum target, GLuint index, GLuint buffer );
    void       (*p_glBindBufferBaseEXT)( GLenum target, GLuint index, GLuint buffer );
    void       (*p_glBindBufferBaseNV)( GLenum target, GLuint index, GLuint buffer );
    void       (*p_glBindBufferOffsetEXT)( GLenum target, GLuint index, GLuint buffer, GLintptr offset );
    void       (*p_glBindBufferOffsetNV)( GLenum target, GLuint index, GLuint buffer, GLintptr offset );
    void       (*p_glBindBufferRange)( GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size );
    void       (*p_glBindBufferRangeEXT)( GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size );
    void       (*p_glBindBufferRangeNV)( GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size );
    void       (*p_glBindBuffersBase)( GLenum target, GLuint first, GLsizei count, const GLuint *buffers );
    void       (*p_glBindBuffersRange)( GLenum target, GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizeiptr *sizes );
    void       (*p_glBindFragDataLocation)( GLuint program, GLuint color, const GLchar *name );
    void       (*p_glBindFragDataLocationEXT)( GLuint program, GLuint color, const GLchar *name );
    void       (*p_glBindFragDataLocationIndexed)( GLuint program, GLuint colorNumber, GLuint index, const GLchar *name );
    void       (*p_glBindFragmentShaderATI)( GLuint id );
    void       (*p_glBindFramebuffer)( GLenum target, GLuint framebuffer );
    void       (*p_glBindFramebufferEXT)( GLenum target, GLuint framebuffer );
    void       (*p_glBindImageTexture)( GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format );
    void       (*p_glBindImageTextureEXT)( GLuint index, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLint format );
    void       (*p_glBindImageTextures)( GLuint first, GLsizei count, const GLuint *textures );
    GLuint     (*p_glBindLightParameterEXT)( GLenum light, GLenum value );
    GLuint     (*p_glBindMaterialParameterEXT)( GLenum face, GLenum value );
    void       (*p_glBindMultiTextureEXT)( GLenum texunit, GLenum target, GLuint texture );
    GLuint     (*p_glBindParameterEXT)( GLenum value );
    void       (*p_glBindProgramARB)( GLenum target, GLuint program );
    void       (*p_glBindProgramNV)( GLenum target, GLuint id );
    void       (*p_glBindProgramPipeline)( GLuint pipeline );
    void       (*p_glBindRenderbuffer)( GLenum target, GLuint renderbuffer );
    void       (*p_glBindRenderbufferEXT)( GLenum target, GLuint renderbuffer );
    void       (*p_glBindSampler)( GLuint unit, GLuint sampler );
    void       (*p_glBindSamplers)( GLuint first, GLsizei count, const GLuint *samplers );
    void       (*p_glBindShadingRateImageNV)( GLuint texture );
    GLuint     (*p_glBindTexGenParameterEXT)( GLenum unit, GLenum coord, GLenum value );
    void       (*p_glBindTextureEXT)( GLenum target, GLuint texture );
    void       (*p_glBindTextureUnit)( GLuint unit, GLuint texture );
    GLuint     (*p_glBindTextureUnitParameterEXT)( GLenum unit, GLenum value );
    void       (*p_glBindTextures)( GLuint first, GLsizei count, const GLuint *textures );
    void       (*p_glBindTransformFeedback)( GLenum target, GLuint id );
    void       (*p_glBindTransformFeedbackNV)( GLenum target, GLuint id );
    void       (*p_glBindVertexArray)( GLuint array );
    void       (*p_glBindVertexArrayAPPLE)( GLuint array );
    void       (*p_glBindVertexBuffer)( GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride );
    void       (*p_glBindVertexBuffers)( GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizei *strides );
    void       (*p_glBindVertexShaderEXT)( GLuint id );
    void       (*p_glBindVideoCaptureStreamBufferNV)( GLuint video_capture_slot, GLuint stream, GLenum frame_region, GLintptrARB offset );
    void       (*p_glBindVideoCaptureStreamTextureNV)( GLuint video_capture_slot, GLuint stream, GLenum frame_region, GLenum target, GLuint texture );
    void       (*p_glBinormal3bEXT)( GLbyte bx, GLbyte by, GLbyte bz );
    void       (*p_glBinormal3bvEXT)( const GLbyte *v );
    void       (*p_glBinormal3dEXT)( GLdouble bx, GLdouble by, GLdouble bz );
    void       (*p_glBinormal3dvEXT)( const GLdouble *v );
    void       (*p_glBinormal3fEXT)( GLfloat bx, GLfloat by, GLfloat bz );
    void       (*p_glBinormal3fvEXT)( const GLfloat *v );
    void       (*p_glBinormal3iEXT)( GLint bx, GLint by, GLint bz );
    void       (*p_glBinormal3ivEXT)( const GLint *v );
    void       (*p_glBinormal3sEXT)( GLshort bx, GLshort by, GLshort bz );
    void       (*p_glBinormal3svEXT)( const GLshort *v );
    void       (*p_glBinormalPointerEXT)( GLenum type, GLsizei stride, const void *pointer );
    void       (*p_glBitmapxOES)( GLsizei width, GLsizei height, GLfixed xorig, GLfixed yorig, GLfixed xmove, GLfixed ymove, const GLubyte *bitmap );
    void       (*p_glBlendBarrierKHR)(void);
    void       (*p_glBlendBarrierNV)(void);
    void       (*p_glBlendColor)( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha );
    void       (*p_glBlendColorEXT)( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha );
    void       (*p_glBlendColorxOES)( GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha );
    void       (*p_glBlendEquation)( GLenum mode );
    void       (*p_glBlendEquationEXT)( GLenum mode );
    void       (*p_glBlendEquationIndexedAMD)( GLuint buf, GLenum mode );
    void       (*p_glBlendEquationSeparate)( GLenum modeRGB, GLenum modeAlpha );
    void       (*p_glBlendEquationSeparateEXT)( GLenum modeRGB, GLenum modeAlpha );
    void       (*p_glBlendEquationSeparateIndexedAMD)( GLuint buf, GLenum modeRGB, GLenum modeAlpha );
    void       (*p_glBlendEquationSeparatei)( GLuint buf, GLenum modeRGB, GLenum modeAlpha );
    void       (*p_glBlendEquationSeparateiARB)( GLuint buf, GLenum modeRGB, GLenum modeAlpha );
    void       (*p_glBlendEquationi)( GLuint buf, GLenum mode );
    void       (*p_glBlendEquationiARB)( GLuint buf, GLenum mode );
    void       (*p_glBlendFuncIndexedAMD)( GLuint buf, GLenum src, GLenum dst );
    void       (*p_glBlendFuncSeparate)( GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha );
    void       (*p_glBlendFuncSeparateEXT)( GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha );
    void       (*p_glBlendFuncSeparateINGR)( GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha );
    void       (*p_glBlendFuncSeparateIndexedAMD)( GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha );
    void       (*p_glBlendFuncSeparatei)( GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha );
    void       (*p_glBlendFuncSeparateiARB)( GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha );
    void       (*p_glBlendFunci)( GLuint buf, GLenum src, GLenum dst );
    void       (*p_glBlendFunciARB)( GLuint buf, GLenum src, GLenum dst );
    void       (*p_glBlendParameteriNV)( GLenum pname, GLint value );
    void       (*p_glBlitFramebuffer)( GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter );
    void       (*p_glBlitFramebufferEXT)( GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter );
    void       (*p_glBlitNamedFramebuffer)( GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter );
    void       (*p_glBufferAddressRangeNV)( GLenum pname, GLuint index, GLuint64EXT address, GLsizeiptr length );
    void       (*p_glBufferAttachMemoryNV)( GLenum target, GLuint memory, GLuint64 offset );
    void       (*p_glBufferData)( GLenum target, GLsizeiptr size, const void *data, GLenum usage );
    void       (*p_glBufferDataARB)( GLenum target, GLsizeiptrARB size, const void *data, GLenum usage );
    void       (*p_glBufferPageCommitmentARB)( GLenum target, GLintptr offset, GLsizeiptr size, GLboolean commit );
    void       (*p_glBufferParameteriAPPLE)( GLenum target, GLenum pname, GLint param );
    GLuint     (*p_glBufferRegionEnabled)(void);
    void       (*p_glBufferStorage)( GLenum target, GLsizeiptr size, const void *data, GLbitfield flags );
    void       (*p_glBufferStorageExternalEXT)( GLenum target, GLintptr offset, GLsizeiptr size, GLeglClientBufferEXT clientBuffer, GLbitfield flags );
    void       (*p_glBufferStorageMemEXT)( GLenum target, GLsizeiptr size, GLuint memory, GLuint64 offset );
    void       (*p_glBufferSubData)( GLenum target, GLintptr offset, GLsizeiptr size, const void *data );
    void       (*p_glBufferSubDataARB)( GLenum target, GLintptrARB offset, GLsizeiptrARB size, const void *data );
    void       (*p_glCallCommandListNV)( GLuint list );
    GLenum     (*p_glCheckFramebufferStatus)( GLenum target );
    GLenum     (*p_glCheckFramebufferStatusEXT)( GLenum target );
    GLenum     (*p_glCheckNamedFramebufferStatus)( GLuint framebuffer, GLenum target );
    GLenum     (*p_glCheckNamedFramebufferStatusEXT)( GLuint framebuffer, GLenum target );
    void       (*p_glClampColor)( GLenum target, GLenum clamp );
    void       (*p_glClampColorARB)( GLenum target, GLenum clamp );
    void       (*p_glClearAccumxOES)( GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha );
    void       (*p_glClearBufferData)( GLenum target, GLenum internalformat, GLenum format, GLenum type, const void *data );
    void       (*p_glClearBufferSubData)( GLenum target, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data );
    void       (*p_glClearBufferfi)( GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil );
    void       (*p_glClearBufferfv)( GLenum buffer, GLint drawbuffer, const GLfloat *value );
    void       (*p_glClearBufferiv)( GLenum buffer, GLint drawbuffer, const GLint *value );
    void       (*p_glClearBufferuiv)( GLenum buffer, GLint drawbuffer, const GLuint *value );
    void       (*p_glClearColorIiEXT)( GLint red, GLint green, GLint blue, GLint alpha );
    void       (*p_glClearColorIuiEXT)( GLuint red, GLuint green, GLuint blue, GLuint alpha );
    void       (*p_glClearColorxOES)( GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha );
    void       (*p_glClearDepthdNV)( GLdouble depth );
    void       (*p_glClearDepthf)( GLfloat d );
    void       (*p_glClearDepthfOES)( GLclampf depth );
    void       (*p_glClearDepthxOES)( GLfixed depth );
    void       (*p_glClearNamedBufferData)( GLuint buffer, GLenum internalformat, GLenum format, GLenum type, const void *data );
    void       (*p_glClearNamedBufferDataEXT)( GLuint buffer, GLenum internalformat, GLenum format, GLenum type, const void *data );
    void       (*p_glClearNamedBufferSubData)( GLuint buffer, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data );
    void       (*p_glClearNamedBufferSubDataEXT)( GLuint buffer, GLenum internalformat, GLsizeiptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data );
    void       (*p_glClearNamedFramebufferfi)( GLuint framebuffer, GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil );
    void       (*p_glClearNamedFramebufferfv)( GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLfloat *value );
    void       (*p_glClearNamedFramebufferiv)( GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLint *value );
    void       (*p_glClearNamedFramebufferuiv)( GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLuint *value );
    void       (*p_glClearTexImage)( GLuint texture, GLint level, GLenum format, GLenum type, const void *data );
    void       (*p_glClearTexSubImage)( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *data );
    void       (*p_glClientActiveTexture)( GLenum texture );
    void       (*p_glClientActiveTextureARB)( GLenum texture );
    void       (*p_glClientActiveVertexStreamATI)( GLenum stream );
    void       (*p_glClientAttribDefaultEXT)( GLbitfield mask );
    void       (*p_glClientWaitSemaphoreui64NVX)( GLsizei fenceObjectCount, const GLuint *semaphoreArray, const GLuint64 *fenceValueArray );
    GLenum     (*p_glClientWaitSync)( GLsync sync, GLbitfield flags, GLuint64 timeout );
    void       (*p_glClipControl)( GLenum origin, GLenum depth );
    void       (*p_glClipPlanefOES)( GLenum plane, const GLfloat *equation );
    void       (*p_glClipPlanexOES)( GLenum plane, const GLfixed *equation );
    void       (*p_glColor3fVertex3fSUN)( GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z );
    void       (*p_glColor3fVertex3fvSUN)( const GLfloat *c, const GLfloat *v );
    void       (*p_glColor3hNV)( GLhalfNV red, GLhalfNV green, GLhalfNV blue );
    void       (*p_glColor3hvNV)( const GLhalfNV *v );
    void       (*p_glColor3xOES)( GLfixed red, GLfixed green, GLfixed blue );
    void       (*p_glColor3xvOES)( const GLfixed *components );
    void       (*p_glColor4fNormal3fVertex3fSUN)( GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z );
    void       (*p_glColor4fNormal3fVertex3fvSUN)( const GLfloat *c, const GLfloat *n, const GLfloat *v );
    void       (*p_glColor4hNV)( GLhalfNV red, GLhalfNV green, GLhalfNV blue, GLhalfNV alpha );
    void       (*p_glColor4hvNV)( const GLhalfNV *v );
    void       (*p_glColor4ubVertex2fSUN)( GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y );
    void       (*p_glColor4ubVertex2fvSUN)( const GLubyte *c, const GLfloat *v );
    void       (*p_glColor4ubVertex3fSUN)( GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z );
    void       (*p_glColor4ubVertex3fvSUN)( const GLubyte *c, const GLfloat *v );
    void       (*p_glColor4xOES)( GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha );
    void       (*p_glColor4xvOES)( const GLfixed *components );
    void       (*p_glColorFormatNV)( GLint size, GLenum type, GLsizei stride );
    void       (*p_glColorFragmentOp1ATI)( GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod );
    void       (*p_glColorFragmentOp2ATI)( GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod );
    void       (*p_glColorFragmentOp3ATI)( GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod );
    void       (*p_glColorMaskIndexedEXT)( GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a );
    void       (*p_glColorMaski)( GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a );
    void       (*p_glColorP3ui)( GLenum type, GLuint color );
    void       (*p_glColorP3uiv)( GLenum type, const GLuint *color );
    void       (*p_glColorP4ui)( GLenum type, GLuint color );
    void       (*p_glColorP4uiv)( GLenum type, const GLuint *color );
    void       (*p_glColorPointerEXT)( GLint size, GLenum type, GLsizei stride, GLsizei count, const void *pointer );
    void       (*p_glColorPointerListIBM)( GLint size, GLenum type, GLint stride, const void **pointer, GLint ptrstride );
    void       (*p_glColorPointervINTEL)( GLint size, GLenum type, const void **pointer );
    void       (*p_glColorSubTable)( GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const void *data );
    void       (*p_glColorSubTableEXT)( GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const void *data );
    void       (*p_glColorTable)( GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const void *table );
    void       (*p_glColorTableEXT)( GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type, const void *table );
    void       (*p_glColorTableParameterfv)( GLenum target, GLenum pname, const GLfloat *params );
    void       (*p_glColorTableParameterfvSGI)( GLenum target, GLenum pname, const GLfloat *params );
    void       (*p_glColorTableParameteriv)( GLenum target, GLenum pname, const GLint *params );
    void       (*p_glColorTableParameterivSGI)( GLenum target, GLenum pname, const GLint *params );
    void       (*p_glColorTableSGI)( GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const void *table );
    void       (*p_glCombinerInputNV)( GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage );
    void       (*p_glCombinerOutputNV)( GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct, GLboolean cdDotProduct, GLboolean muxSum );
    void       (*p_glCombinerParameterfNV)( GLenum pname, GLfloat param );
    void       (*p_glCombinerParameterfvNV)( GLenum pname, const GLfloat *params );
    void       (*p_glCombinerParameteriNV)( GLenum pname, GLint param );
    void       (*p_glCombinerParameterivNV)( GLenum pname, const GLint *params );
    void       (*p_glCombinerStageParameterfvNV)( GLenum stage, GLenum pname, const GLfloat *params );
    void       (*p_glCommandListSegmentsNV)( GLuint list, GLuint segments );
    void       (*p_glCompileCommandListNV)( GLuint list );
    void       (*p_glCompileShader)( GLuint shader );
    void       (*p_glCompileShaderARB)( GLhandleARB shaderObj );
    void       (*p_glCompileShaderIncludeARB)( GLuint shader, GLsizei count, const GLchar *const*path, const GLint *length );
    void       (*p_glCompressedMultiTexImage1DEXT)( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *bits );
    void       (*p_glCompressedMultiTexImage2DEXT)( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *bits );
    void       (*p_glCompressedMultiTexImage3DEXT)( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *bits );
    void       (*p_glCompressedMultiTexSubImage1DEXT)( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *bits );
    void       (*p_glCompressedMultiTexSubImage2DEXT)( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *bits );
    void       (*p_glCompressedMultiTexSubImage3DEXT)( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *bits );
    void       (*p_glCompressedTexImage1D)( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *data );
    void       (*p_glCompressedTexImage1DARB)( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *data );
    void       (*p_glCompressedTexImage2D)( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data );
    void       (*p_glCompressedTexImage2DARB)( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data );
    void       (*p_glCompressedTexImage3D)( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data );
    void       (*p_glCompressedTexImage3DARB)( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data );
    void       (*p_glCompressedTexSubImage1D)( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data );
    void       (*p_glCompressedTexSubImage1DARB)( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data );
    void       (*p_glCompressedTexSubImage2D)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data );
    void       (*p_glCompressedTexSubImage2DARB)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data );
    void       (*p_glCompressedTexSubImage3D)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data );
    void       (*p_glCompressedTexSubImage3DARB)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data );
    void       (*p_glCompressedTextureImage1DEXT)( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *bits );
    void       (*p_glCompressedTextureImage2DEXT)( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *bits );
    void       (*p_glCompressedTextureImage3DEXT)( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *bits );
    void       (*p_glCompressedTextureSubImage1D)( GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data );
    void       (*p_glCompressedTextureSubImage1DEXT)( GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *bits );
    void       (*p_glCompressedTextureSubImage2D)( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data );
    void       (*p_glCompressedTextureSubImage2DEXT)( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *bits );
    void       (*p_glCompressedTextureSubImage3D)( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data );
    void       (*p_glCompressedTextureSubImage3DEXT)( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *bits );
    void       (*p_glConservativeRasterParameterfNV)( GLenum pname, GLfloat value );
    void       (*p_glConservativeRasterParameteriNV)( GLenum pname, GLint param );
    void       (*p_glConvolutionFilter1D)( GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const void *image );
    void       (*p_glConvolutionFilter1DEXT)( GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const void *image );
    void       (*p_glConvolutionFilter2D)( GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *image );
    void       (*p_glConvolutionFilter2DEXT)( GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *image );
    void       (*p_glConvolutionParameterf)( GLenum target, GLenum pname, GLfloat params );
    void       (*p_glConvolutionParameterfEXT)( GLenum target, GLenum pname, GLfloat params );
    void       (*p_glConvolutionParameterfv)( GLenum target, GLenum pname, const GLfloat *params );
    void       (*p_glConvolutionParameterfvEXT)( GLenum target, GLenum pname, const GLfloat *params );
    void       (*p_glConvolutionParameteri)( GLenum target, GLenum pname, GLint params );
    void       (*p_glConvolutionParameteriEXT)( GLenum target, GLenum pname, GLint params );
    void       (*p_glConvolutionParameteriv)( GLenum target, GLenum pname, const GLint *params );
    void       (*p_glConvolutionParameterivEXT)( GLenum target, GLenum pname, const GLint *params );
    void       (*p_glConvolutionParameterxOES)( GLenum target, GLenum pname, GLfixed param );
    void       (*p_glConvolutionParameterxvOES)( GLenum target, GLenum pname, const GLfixed *params );
    void       (*p_glCopyBufferSubData)( GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size );
    void       (*p_glCopyColorSubTable)( GLenum target, GLsizei start, GLint x, GLint y, GLsizei width );
    void       (*p_glCopyColorSubTableEXT)( GLenum target, GLsizei start, GLint x, GLint y, GLsizei width );
    void       (*p_glCopyColorTable)( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width );
    void       (*p_glCopyColorTableSGI)( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width );
    void       (*p_glCopyConvolutionFilter1D)( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width );
    void       (*p_glCopyConvolutionFilter1DEXT)( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width );
    void       (*p_glCopyConvolutionFilter2D)( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height );
    void       (*p_glCopyConvolutionFilter2DEXT)( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height );
    void       (*p_glCopyImageSubData)( GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth );
    void       (*p_glCopyImageSubDataNV)( GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei width, GLsizei height, GLsizei depth );
    void       (*p_glCopyMultiTexImage1DEXT)( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border );
    void       (*p_glCopyMultiTexImage2DEXT)( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border );
    void       (*p_glCopyMultiTexSubImage1DEXT)( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width );
    void       (*p_glCopyMultiTexSubImage2DEXT)( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height );
    void       (*p_glCopyMultiTexSubImage3DEXT)( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height );
    void       (*p_glCopyNamedBufferSubData)( GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size );
    void       (*p_glCopyPathNV)( GLuint resultPath, GLuint srcPath );
    void       (*p_glCopyTexImage1DEXT)( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border );
    void       (*p_glCopyTexImage2DEXT)( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border );
    void       (*p_glCopyTexSubImage1DEXT)( GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width );
    void       (*p_glCopyTexSubImage2DEXT)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height );
    void       (*p_glCopyTexSubImage3D)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height );
    void       (*p_glCopyTexSubImage3DEXT)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height );
    void       (*p_glCopyTextureImage1DEXT)( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border );
    void       (*p_glCopyTextureImage2DEXT)( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border );
    void       (*p_glCopyTextureSubImage1D)( GLuint texture, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width );
    void       (*p_glCopyTextureSubImage1DEXT)( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width );
    void       (*p_glCopyTextureSubImage2D)( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height );
    void       (*p_glCopyTextureSubImage2DEXT)( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height );
    void       (*p_glCopyTextureSubImage3D)( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height );
    void       (*p_glCopyTextureSubImage3DEXT)( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height );
    void       (*p_glCoverFillPathInstancedNV)( GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum coverMode, GLenum transformType, const GLfloat *transformValues );
    void       (*p_glCoverFillPathNV)( GLuint path, GLenum coverMode );
    void       (*p_glCoverStrokePathInstancedNV)( GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum coverMode, GLenum transformType, const GLfloat *transformValues );
    void       (*p_glCoverStrokePathNV)( GLuint path, GLenum coverMode );
    void       (*p_glCoverageModulationNV)( GLenum components );
    void       (*p_glCoverageModulationTableNV)( GLsizei n, const GLfloat *v );
    void       (*p_glCreateBuffers)( GLsizei n, GLuint *buffers );
    void       (*p_glCreateCommandListsNV)( GLsizei n, GLuint *lists );
    void       (*p_glCreateFramebuffers)( GLsizei n, GLuint *framebuffers );
    void       (*p_glCreateMemoryObjectsEXT)( GLsizei n, GLuint *memoryObjects );
    void       (*p_glCreatePerfQueryINTEL)( GLuint queryId, GLuint *queryHandle );
    GLuint     (*p_glCreateProgram)(void);
    GLhandleARB (*p_glCreateProgramObjectARB)(void);
    void       (*p_glCreateProgramPipelines)( GLsizei n, GLuint *pipelines );
    GLuint     (*p_glCreateProgressFenceNVX)(void);
    void       (*p_glCreateQueries)( GLenum target, GLsizei n, GLuint *ids );
    void       (*p_glCreateRenderbuffers)( GLsizei n, GLuint *renderbuffers );
    void       (*p_glCreateSamplers)( GLsizei n, GLuint *samplers );
    GLuint     (*p_glCreateShader)( GLenum type );
    GLhandleARB (*p_glCreateShaderObjectARB)( GLenum shaderType );
    GLuint     (*p_glCreateShaderProgramEXT)( GLenum type, const GLchar *string );
    GLuint     (*p_glCreateShaderProgramv)( GLenum type, GLsizei count, const GLchar *const*strings );
    void       (*p_glCreateStatesNV)( GLsizei n, GLuint *states );
    GLsync     (*p_glCreateSyncFromCLeventARB)( struct _cl_context *context, struct _cl_event *event, GLbitfield flags );
    void       (*p_glCreateTextures)( GLenum target, GLsizei n, GLuint *textures );
    void       (*p_glCreateTransformFeedbacks)( GLsizei n, GLuint *ids );
    void       (*p_glCreateVertexArrays)( GLsizei n, GLuint *arrays );
    void       (*p_glCullParameterdvEXT)( GLenum pname, GLdouble *params );
    void       (*p_glCullParameterfvEXT)( GLenum pname, GLfloat *params );
    void       (*p_glCurrentPaletteMatrixARB)( GLint index );
    void       (*p_glDebugMessageCallback)( GLDEBUGPROC callback, const void *userParam );
    void       (*p_glDebugMessageCallbackAMD)( GLDEBUGPROCAMD callback, void *userParam );
    void       (*p_glDebugMessageCallbackARB)( GLDEBUGPROCARB callback, const void *userParam );
    void       (*p_glDebugMessageControl)( GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled );
    void       (*p_glDebugMessageControlARB)( GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled );
    void       (*p_glDebugMessageEnableAMD)( GLenum category, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled );
    void       (*p_glDebugMessageInsert)( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *buf );
    void       (*p_glDebugMessageInsertAMD)( GLenum category, GLenum severity, GLuint id, GLsizei length, const GLchar *buf );
    void       (*p_glDebugMessageInsertARB)( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *buf );
    void       (*p_glDeformSGIX)( GLbitfield mask );
    void       (*p_glDeformationMap3dSGIX)( GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, GLdouble w1, GLdouble w2, GLint wstride, GLint worder, const GLdouble *points );
    void       (*p_glDeformationMap3fSGIX)( GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, GLfloat w1, GLfloat w2, GLint wstride, GLint worder, const GLfloat *points );
    void       (*p_glDeleteAsyncMarkersSGIX)( GLuint marker, GLsizei range );
    void       (*p_glDeleteBufferRegion)( GLenum region );
    void       (*p_glDeleteBuffers)( GLsizei n, const GLuint *buffers );
    void       (*p_glDeleteBuffersARB)( GLsizei n, const GLuint *buffers );
    void       (*p_glDeleteCommandListsNV)( GLsizei n, const GLuint *lists );
    void       (*p_glDeleteFencesAPPLE)( GLsizei n, const GLuint *fences );
    void       (*p_glDeleteFencesNV)( GLsizei n, const GLuint *fences );
    void       (*p_glDeleteFragmentShaderATI)( GLuint id );
    void       (*p_glDeleteFramebuffers)( GLsizei n, const GLuint *framebuffers );
    void       (*p_glDeleteFramebuffersEXT)( GLsizei n, const GLuint *framebuffers );
    void       (*p_glDeleteMemoryObjectsEXT)( GLsizei n, const GLuint *memoryObjects );
    void       (*p_glDeleteNamedStringARB)( GLint namelen, const GLchar *name );
    void       (*p_glDeleteNamesAMD)( GLenum identifier, GLuint num, const GLuint *names );
    void       (*p_glDeleteObjectARB)( GLhandleARB obj );
    void       (*p_glDeleteObjectBufferATI)( GLuint buffer );
    void       (*p_glDeleteOcclusionQueriesNV)( GLsizei n, const GLuint *ids );
    void       (*p_glDeletePathsNV)( GLuint path, GLsizei range );
    void       (*p_glDeletePerfMonitorsAMD)( GLsizei n, GLuint *monitors );
    void       (*p_glDeletePerfQueryINTEL)( GLuint queryHandle );
    void       (*p_glDeleteProgram)( GLuint program );
    void       (*p_glDeleteProgramPipelines)( GLsizei n, const GLuint *pipelines );
    void       (*p_glDeleteProgramsARB)( GLsizei n, const GLuint *programs );
    void       (*p_glDeleteProgramsNV)( GLsizei n, const GLuint *programs );
    void       (*p_glDeleteQueries)( GLsizei n, const GLuint *ids );
    void       (*p_glDeleteQueriesARB)( GLsizei n, const GLuint *ids );
    void       (*p_glDeleteQueryResourceTagNV)( GLsizei n, const GLint *tagIds );
    void       (*p_glDeleteRenderbuffers)( GLsizei n, const GLuint *renderbuffers );
    void       (*p_glDeleteRenderbuffersEXT)( GLsizei n, const GLuint *renderbuffers );
    void       (*p_glDeleteSamplers)( GLsizei count, const GLuint *samplers );
    void       (*p_glDeleteSemaphoresEXT)( GLsizei n, const GLuint *semaphores );
    void       (*p_glDeleteShader)( GLuint shader );
    void       (*p_glDeleteStatesNV)( GLsizei n, const GLuint *states );
    void       (*p_glDeleteSync)( GLsync sync );
    void       (*p_glDeleteTexturesEXT)( GLsizei n, const GLuint *textures );
    void       (*p_glDeleteTransformFeedbacks)( GLsizei n, const GLuint *ids );
    void       (*p_glDeleteTransformFeedbacksNV)( GLsizei n, const GLuint *ids );
    void       (*p_glDeleteVertexArrays)( GLsizei n, const GLuint *arrays );
    void       (*p_glDeleteVertexArraysAPPLE)( GLsizei n, const GLuint *arrays );
    void       (*p_glDeleteVertexShaderEXT)( GLuint id );
    void       (*p_glDepthBoundsEXT)( GLclampd zmin, GLclampd zmax );
    void       (*p_glDepthBoundsdNV)( GLdouble zmin, GLdouble zmax );
    void       (*p_glDepthRangeArraydvNV)( GLuint first, GLsizei count, const GLdouble *v );
    void       (*p_glDepthRangeArrayv)( GLuint first, GLsizei count, const GLdouble *v );
    void       (*p_glDepthRangeIndexed)( GLuint index, GLdouble n, GLdouble f );
    void       (*p_glDepthRangeIndexeddNV)( GLuint index, GLdouble n, GLdouble f );
    void       (*p_glDepthRangedNV)( GLdouble zNear, GLdouble zFar );
    void       (*p_glDepthRangef)( GLfloat n, GLfloat f );
    void       (*p_glDepthRangefOES)( GLclampf n, GLclampf f );
    void       (*p_glDepthRangexOES)( GLfixed n, GLfixed f );
    void       (*p_glDetachObjectARB)( GLhandleARB containerObj, GLhandleARB attachedObj );
    void       (*p_glDetachShader)( GLuint program, GLuint shader );
    void       (*p_glDetailTexFuncSGIS)( GLenum target, GLsizei n, const GLfloat *points );
    void       (*p_glDisableClientStateIndexedEXT)( GLenum array, GLuint index );
    void       (*p_glDisableClientStateiEXT)( GLenum array, GLuint index );
    void       (*p_glDisableIndexedEXT)( GLenum target, GLuint index );
    void       (*p_glDisableVariantClientStateEXT)( GLuint id );
    void       (*p_glDisableVertexArrayAttrib)( GLuint vaobj, GLuint index );
    void       (*p_glDisableVertexArrayAttribEXT)( GLuint vaobj, GLuint index );
    void       (*p_glDisableVertexArrayEXT)( GLuint vaobj, GLenum array );
    void       (*p_glDisableVertexAttribAPPLE)( GLuint index, GLenum pname );
    void       (*p_glDisableVertexAttribArray)( GLuint index );
    void       (*p_glDisableVertexAttribArrayARB)( GLuint index );
    void       (*p_glDisablei)( GLenum target, GLuint index );
    void       (*p_glDispatchCompute)( GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z );
    void       (*p_glDispatchComputeGroupSizeARB)( GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z, GLuint group_size_x, GLuint group_size_y, GLuint group_size_z );
    void       (*p_glDispatchComputeIndirect)( GLintptr indirect );
    void       (*p_glDrawArraysEXT)( GLenum mode, GLint first, GLsizei count );
    void       (*p_glDrawArraysIndirect)( GLenum mode, const void *indirect );
    void       (*p_glDrawArraysInstanced)( GLenum mode, GLint first, GLsizei count, GLsizei instancecount );
    void       (*p_glDrawArraysInstancedARB)( GLenum mode, GLint first, GLsizei count, GLsizei primcount );
    void       (*p_glDrawArraysInstancedBaseInstance)( GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance );
    void       (*p_glDrawArraysInstancedEXT)( GLenum mode, GLint start, GLsizei count, GLsizei primcount );
    void       (*p_glDrawBufferRegion)( GLenum region, GLint x, GLint y, GLsizei width, GLsizei height, GLint xDest, GLint yDest );
    void       (*p_glDrawBuffers)( GLsizei n, const GLenum *bufs );
    void       (*p_glDrawBuffersARB)( GLsizei n, const GLenum *bufs );
    void       (*p_glDrawBuffersATI)( GLsizei n, const GLenum *bufs );
    void       (*p_glDrawCommandsAddressNV)( GLenum primitiveMode, const GLuint64 *indirects, const GLsizei *sizes, GLuint count );
    void       (*p_glDrawCommandsNV)( GLenum primitiveMode, GLuint buffer, const GLintptr *indirects, const GLsizei *sizes, GLuint count );
    void       (*p_glDrawCommandsStatesAddressNV)( const GLuint64 *indirects, const GLsizei *sizes, const GLuint *states, const GLuint *fbos, GLuint count );
    void       (*p_glDrawCommandsStatesNV)( GLuint buffer, const GLintptr *indirects, const GLsizei *sizes, const GLuint *states, const GLuint *fbos, GLuint count );
    void       (*p_glDrawElementArrayAPPLE)( GLenum mode, GLint first, GLsizei count );
    void       (*p_glDrawElementArrayATI)( GLenum mode, GLsizei count );
    void       (*p_glDrawElementsBaseVertex)( GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex );
    void       (*p_glDrawElementsIndirect)( GLenum mode, GLenum type, const void *indirect );
    void       (*p_glDrawElementsInstanced)( GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount );
    void       (*p_glDrawElementsInstancedARB)( GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount );
    void       (*p_glDrawElementsInstancedBaseInstance)( GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLuint baseinstance );
    void       (*p_glDrawElementsInstancedBaseVertex)( GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex );
    void       (*p_glDrawElementsInstancedBaseVertexBaseInstance)( GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex, GLuint baseinstance );
    void       (*p_glDrawElementsInstancedEXT)( GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount );
    void       (*p_glDrawMeshArraysSUN)( GLenum mode, GLint first, GLsizei count, GLsizei width );
    void       (*p_glDrawMeshTasksIndirectNV)( GLintptr indirect );
    void       (*p_glDrawMeshTasksNV)( GLuint first, GLuint count );
    void       (*p_glDrawRangeElementArrayAPPLE)( GLenum mode, GLuint start, GLuint end, GLint first, GLsizei count );
    void       (*p_glDrawRangeElementArrayATI)( GLenum mode, GLuint start, GLuint end, GLsizei count );
    void       (*p_glDrawRangeElements)( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices );
    void       (*p_glDrawRangeElementsBaseVertex)( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices, GLint basevertex );
    void       (*p_glDrawRangeElementsEXT)( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices );
    void       (*p_glDrawTextureNV)( GLuint texture, GLuint sampler, GLfloat x0, GLfloat y0, GLfloat x1, GLfloat y1, GLfloat z, GLfloat s0, GLfloat t0, GLfloat s1, GLfloat t1 );
    void       (*p_glDrawTransformFeedback)( GLenum mode, GLuint id );
    void       (*p_glDrawTransformFeedbackInstanced)( GLenum mode, GLuint id, GLsizei instancecount );
    void       (*p_glDrawTransformFeedbackNV)( GLenum mode, GLuint id );
    void       (*p_glDrawTransformFeedbackStream)( GLenum mode, GLuint id, GLuint stream );
    void       (*p_glDrawTransformFeedbackStreamInstanced)( GLenum mode, GLuint id, GLuint stream, GLsizei instancecount );
    void       (*p_glDrawVkImageNV)( GLuint64 vkImage, GLuint sampler, GLfloat x0, GLfloat y0, GLfloat x1, GLfloat y1, GLfloat z, GLfloat s0, GLfloat t0, GLfloat s1, GLfloat t1 );
    void       (*p_glEGLImageTargetTexStorageEXT)( GLenum target, GLeglImageOES image, const GLint* attrib_list );
    void       (*p_glEGLImageTargetTextureStorageEXT)( GLuint texture, GLeglImageOES image, const GLint* attrib_list );
    void       (*p_glEdgeFlagFormatNV)( GLsizei stride );
    void       (*p_glEdgeFlagPointerEXT)( GLsizei stride, GLsizei count, const GLboolean *pointer );
    void       (*p_glEdgeFlagPointerListIBM)( GLint stride, const GLboolean **pointer, GLint ptrstride );
    void       (*p_glElementPointerAPPLE)( GLenum type, const void *pointer );
    void       (*p_glElementPointerATI)( GLenum type, const void *pointer );
    void       (*p_glEnableClientStateIndexedEXT)( GLenum array, GLuint index );
    void       (*p_glEnableClientStateiEXT)( GLenum array, GLuint index );
    void       (*p_glEnableIndexedEXT)( GLenum target, GLuint index );
    void       (*p_glEnableVariantClientStateEXT)( GLuint id );
    void       (*p_glEnableVertexArrayAttrib)( GLuint vaobj, GLuint index );
    void       (*p_glEnableVertexArrayAttribEXT)( GLuint vaobj, GLuint index );
    void       (*p_glEnableVertexArrayEXT)( GLuint vaobj, GLenum array );
    void       (*p_glEnableVertexAttribAPPLE)( GLuint index, GLenum pname );
    void       (*p_glEnableVertexAttribArray)( GLuint index );
    void       (*p_glEnableVertexAttribArrayARB)( GLuint index );
    void       (*p_glEnablei)( GLenum target, GLuint index );
    void       (*p_glEndConditionalRender)(void);
    void       (*p_glEndConditionalRenderNV)(void);
    void       (*p_glEndConditionalRenderNVX)(void);
    void       (*p_glEndFragmentShaderATI)(void);
    void       (*p_glEndOcclusionQueryNV)(void);
    void       (*p_glEndPerfMonitorAMD)( GLuint monitor );
    void       (*p_glEndPerfQueryINTEL)( GLuint queryHandle );
    void       (*p_glEndQuery)( GLenum target );
    void       (*p_glEndQueryARB)( GLenum target );
    void       (*p_glEndQueryIndexed)( GLenum target, GLuint index );
    void       (*p_glEndTransformFeedback)(void);
    void       (*p_glEndTransformFeedbackEXT)(void);
    void       (*p_glEndTransformFeedbackNV)(void);
    void       (*p_glEndVertexShaderEXT)(void);
    void       (*p_glEndVideoCaptureNV)( GLuint video_capture_slot );
    void       (*p_glEvalCoord1xOES)( GLfixed u );
    void       (*p_glEvalCoord1xvOES)( const GLfixed *coords );
    void       (*p_glEvalCoord2xOES)( GLfixed u, GLfixed v );
    void       (*p_glEvalCoord2xvOES)( const GLfixed *coords );
    void       (*p_glEvalMapsNV)( GLenum target, GLenum mode );
    void       (*p_glEvaluateDepthValuesARB)(void);
    void       (*p_glExecuteProgramNV)( GLenum target, GLuint id, const GLfloat *params );
    void       (*p_glExtractComponentEXT)( GLuint res, GLuint src, GLuint num );
    void       (*p_glFeedbackBufferxOES)( GLsizei n, GLenum type, const GLfixed *buffer );
    GLsync     (*p_glFenceSync)( GLenum condition, GLbitfield flags );
    void       (*p_glFinalCombinerInputNV)( GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage );
    GLint      (*p_glFinishAsyncSGIX)( GLuint *markerp );
    void       (*p_glFinishFenceAPPLE)( GLuint fence );
    void       (*p_glFinishFenceNV)( GLuint fence );
    void       (*p_glFinishObjectAPPLE)( GLenum object, GLint name );
    void       (*p_glFinishTextureSUNX)(void);
    void       (*p_glFlushMappedBufferRange)( GLenum target, GLintptr offset, GLsizeiptr length );
    void       (*p_glFlushMappedBufferRangeAPPLE)( GLenum target, GLintptr offset, GLsizeiptr size );
    void       (*p_glFlushMappedNamedBufferRange)( GLuint buffer, GLintptr offset, GLsizeiptr length );
    void       (*p_glFlushMappedNamedBufferRangeEXT)( GLuint buffer, GLintptr offset, GLsizeiptr length );
    void       (*p_glFlushPixelDataRangeNV)( GLenum target );
    void       (*p_glFlushRasterSGIX)(void);
    void       (*p_glFlushStaticDataIBM)( GLenum target );
    void       (*p_glFlushVertexArrayRangeAPPLE)( GLsizei length, void *pointer );
    void       (*p_glFlushVertexArrayRangeNV)(void);
    void       (*p_glFogCoordFormatNV)( GLenum type, GLsizei stride );
    void       (*p_glFogCoordPointer)( GLenum type, GLsizei stride, const void *pointer );
    void       (*p_glFogCoordPointerEXT)( GLenum type, GLsizei stride, const void *pointer );
    void       (*p_glFogCoordPointerListIBM)( GLenum type, GLint stride, const void **pointer, GLint ptrstride );
    void       (*p_glFogCoordd)( GLdouble coord );
    void       (*p_glFogCoorddEXT)( GLdouble coord );
    void       (*p_glFogCoorddv)( const GLdouble *coord );
    void       (*p_glFogCoorddvEXT)( const GLdouble *coord );
    void       (*p_glFogCoordf)( GLfloat coord );
    void       (*p_glFogCoordfEXT)( GLfloat coord );
    void       (*p_glFogCoordfv)( const GLfloat *coord );
    void       (*p_glFogCoordfvEXT)( const GLfloat *coord );
    void       (*p_glFogCoordhNV)( GLhalfNV fog );
    void       (*p_glFogCoordhvNV)( const GLhalfNV *fog );
    void       (*p_glFogFuncSGIS)( GLsizei n, const GLfloat *points );
    void       (*p_glFogxOES)( GLenum pname, GLfixed param );
    void       (*p_glFogxvOES)( GLenum pname, const GLfixed *param );
    void       (*p_glFragmentColorMaterialSGIX)( GLenum face, GLenum mode );
    void       (*p_glFragmentCoverageColorNV)( GLuint color );
    void       (*p_glFragmentLightModelfSGIX)( GLenum pname, GLfloat param );
    void       (*p_glFragmentLightModelfvSGIX)( GLenum pname, const GLfloat *params );
    void       (*p_glFragmentLightModeliSGIX)( GLenum pname, GLint param );
    void       (*p_glFragmentLightModelivSGIX)( GLenum pname, const GLint *params );
    void       (*p_glFragmentLightfSGIX)( GLenum light, GLenum pname, GLfloat param );
    void       (*p_glFragmentLightfvSGIX)( GLenum light, GLenum pname, const GLfloat *params );
    void       (*p_glFragmentLightiSGIX)( GLenum light, GLenum pname, GLint param );
    void       (*p_glFragmentLightivSGIX)( GLenum light, GLenum pname, const GLint *params );
    void       (*p_glFragmentMaterialfSGIX)( GLenum face, GLenum pname, GLfloat param );
    void       (*p_glFragmentMaterialfvSGIX)( GLenum face, GLenum pname, const GLfloat *params );
    void       (*p_glFragmentMaterialiSGIX)( GLenum face, GLenum pname, GLint param );
    void       (*p_glFragmentMaterialivSGIX)( GLenum face, GLenum pname, const GLint *params );
    void       (*p_glFrameTerminatorGREMEDY)(void);
    void       (*p_glFrameZoomSGIX)( GLint factor );
    void       (*p_glFramebufferDrawBufferEXT)( GLuint framebuffer, GLenum mode );
    void       (*p_glFramebufferDrawBuffersEXT)( GLuint framebuffer, GLsizei n, const GLenum *bufs );
    void       (*p_glFramebufferFetchBarrierEXT)(void);
    void       (*p_glFramebufferParameteri)( GLenum target, GLenum pname, GLint param );
    void       (*p_glFramebufferParameteriMESA)( GLenum target, GLenum pname, GLint param );
    void       (*p_glFramebufferReadBufferEXT)( GLuint framebuffer, GLenum mode );
    void       (*p_glFramebufferRenderbuffer)( GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer );
    void       (*p_glFramebufferRenderbufferEXT)( GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer );
    void       (*p_glFramebufferSampleLocationsfvARB)( GLenum target, GLuint start, GLsizei count, const GLfloat *v );
    void       (*p_glFramebufferSampleLocationsfvNV)( GLenum target, GLuint start, GLsizei count, const GLfloat *v );
    void       (*p_glFramebufferSamplePositionsfvAMD)( GLenum target, GLuint numsamples, GLuint pixelindex, const GLfloat *values );
    void       (*p_glFramebufferTexture)( GLenum target, GLenum attachment, GLuint texture, GLint level );
    void       (*p_glFramebufferTexture1D)( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level );
    void       (*p_glFramebufferTexture1DEXT)( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level );
    void       (*p_glFramebufferTexture2D)( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level );
    void       (*p_glFramebufferTexture2DEXT)( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level );
    void       (*p_glFramebufferTexture3D)( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset );
    void       (*p_glFramebufferTexture3DEXT)( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset );
    void       (*p_glFramebufferTextureARB)( GLenum target, GLenum attachment, GLuint texture, GLint level );
    void       (*p_glFramebufferTextureEXT)( GLenum target, GLenum attachment, GLuint texture, GLint level );
    void       (*p_glFramebufferTextureFaceARB)( GLenum target, GLenum attachment, GLuint texture, GLint level, GLenum face );
    void       (*p_glFramebufferTextureFaceEXT)( GLenum target, GLenum attachment, GLuint texture, GLint level, GLenum face );
    void       (*p_glFramebufferTextureLayer)( GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer );
    void       (*p_glFramebufferTextureLayerARB)( GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer );
    void       (*p_glFramebufferTextureLayerEXT)( GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer );
    void       (*p_glFramebufferTextureMultiviewOVR)( GLenum target, GLenum attachment, GLuint texture, GLint level, GLint baseViewIndex, GLsizei numViews );
    void       (*p_glFreeObjectBufferATI)( GLuint buffer );
    void       (*p_glFrustumfOES)( GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f );
    void       (*p_glFrustumxOES)( GLfixed l, GLfixed r, GLfixed b, GLfixed t, GLfixed n, GLfixed f );
    GLuint     (*p_glGenAsyncMarkersSGIX)( GLsizei range );
    void       (*p_glGenBuffers)( GLsizei n, GLuint *buffers );
    void       (*p_glGenBuffersARB)( GLsizei n, GLuint *buffers );
    void       (*p_glGenFencesAPPLE)( GLsizei n, GLuint *fences );
    void       (*p_glGenFencesNV)( GLsizei n, GLuint *fences );
    GLuint     (*p_glGenFragmentShadersATI)( GLuint range );
    void       (*p_glGenFramebuffers)( GLsizei n, GLuint *framebuffers );
    void       (*p_glGenFramebuffersEXT)( GLsizei n, GLuint *framebuffers );
    void       (*p_glGenNamesAMD)( GLenum identifier, GLuint num, GLuint *names );
    void       (*p_glGenOcclusionQueriesNV)( GLsizei n, GLuint *ids );
    GLuint     (*p_glGenPathsNV)( GLsizei range );
    void       (*p_glGenPerfMonitorsAMD)( GLsizei n, GLuint *monitors );
    void       (*p_glGenProgramPipelines)( GLsizei n, GLuint *pipelines );
    void       (*p_glGenProgramsARB)( GLsizei n, GLuint *programs );
    void       (*p_glGenProgramsNV)( GLsizei n, GLuint *programs );
    void       (*p_glGenQueries)( GLsizei n, GLuint *ids );
    void       (*p_glGenQueriesARB)( GLsizei n, GLuint *ids );
    void       (*p_glGenQueryResourceTagNV)( GLsizei n, GLint *tagIds );
    void       (*p_glGenRenderbuffers)( GLsizei n, GLuint *renderbuffers );
    void       (*p_glGenRenderbuffersEXT)( GLsizei n, GLuint *renderbuffers );
    void       (*p_glGenSamplers)( GLsizei count, GLuint *samplers );
    void       (*p_glGenSemaphoresEXT)( GLsizei n, GLuint *semaphores );
    GLuint     (*p_glGenSymbolsEXT)( GLenum datatype, GLenum storagetype, GLenum range, GLuint components );
    void       (*p_glGenTexturesEXT)( GLsizei n, GLuint *textures );
    void       (*p_glGenTransformFeedbacks)( GLsizei n, GLuint *ids );
    void       (*p_glGenTransformFeedbacksNV)( GLsizei n, GLuint *ids );
    void       (*p_glGenVertexArrays)( GLsizei n, GLuint *arrays );
    void       (*p_glGenVertexArraysAPPLE)( GLsizei n, GLuint *arrays );
    GLuint     (*p_glGenVertexShadersEXT)( GLuint range );
    void       (*p_glGenerateMipmap)( GLenum target );
    void       (*p_glGenerateMipmapEXT)( GLenum target );
    void       (*p_glGenerateMultiTexMipmapEXT)( GLenum texunit, GLenum target );
    void       (*p_glGenerateTextureMipmap)( GLuint texture );
    void       (*p_glGenerateTextureMipmapEXT)( GLuint texture, GLenum target );
    void       (*p_glGetActiveAtomicCounterBufferiv)( GLuint program, GLuint bufferIndex, GLenum pname, GLint *params );
    void       (*p_glGetActiveAttrib)( GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name );
    void       (*p_glGetActiveAttribARB)( GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name );
    void       (*p_glGetActiveSubroutineName)( GLuint program, GLenum shadertype, GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name );
    void       (*p_glGetActiveSubroutineUniformName)( GLuint program, GLenum shadertype, GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name );
    void       (*p_glGetActiveSubroutineUniformiv)( GLuint program, GLenum shadertype, GLuint index, GLenum pname, GLint *values );
    void       (*p_glGetActiveUniform)( GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name );
    void       (*p_glGetActiveUniformARB)( GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name );
    void       (*p_glGetActiveUniformBlockName)( GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformBlockName );
    void       (*p_glGetActiveUniformBlockiv)( GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint *params );
    void       (*p_glGetActiveUniformName)( GLuint program, GLuint uniformIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformName );
    void       (*p_glGetActiveUniformsiv)( GLuint program, GLsizei uniformCount, const GLuint *uniformIndices, GLenum pname, GLint *params );
    void       (*p_glGetActiveVaryingNV)( GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name );
    void       (*p_glGetArrayObjectfvATI)( GLenum array, GLenum pname, GLfloat *params );
    void       (*p_glGetArrayObjectivATI)( GLenum array, GLenum pname, GLint *params );
    void       (*p_glGetAttachedObjectsARB)( GLhandleARB containerObj, GLsizei maxCount, GLsizei *count, GLhandleARB *obj );
    void       (*p_glGetAttachedShaders)( GLuint program, GLsizei maxCount, GLsizei *count, GLuint *shaders );
    GLint      (*p_glGetAttribLocation)( GLuint program, const GLchar *name );
    GLint      (*p_glGetAttribLocationARB)( GLhandleARB programObj, const GLcharARB *name );
    void       (*p_glGetBooleanIndexedvEXT)( GLenum target, GLuint index, GLboolean *data );
    void       (*p_glGetBooleani_v)( GLenum target, GLuint index, GLboolean *data );
    void       (*p_glGetBufferParameteri64v)( GLenum target, GLenum pname, GLint64 *params );
    void       (*p_glGetBufferParameteriv)( GLenum target, GLenum pname, GLint *params );
    void       (*p_glGetBufferParameterivARB)( GLenum target, GLenum pname, GLint *params );
    void       (*p_glGetBufferParameterui64vNV)( GLenum target, GLenum pname, GLuint64EXT *params );
    void       (*p_glGetBufferPointerv)( GLenum target, GLenum pname, void **params );
    void       (*p_glGetBufferPointervARB)( GLenum target, GLenum pname, void **params );
    void       (*p_glGetBufferSubData)( GLenum target, GLintptr offset, GLsizeiptr size, void *data );
    void       (*p_glGetBufferSubDataARB)( GLenum target, GLintptrARB offset, GLsizeiptrARB size, void *data );
    void       (*p_glGetClipPlanefOES)( GLenum plane, GLfloat *equation );
    void       (*p_glGetClipPlanexOES)( GLenum plane, GLfixed *equation );
    void       (*p_glGetColorTable)( GLenum target, GLenum format, GLenum type, void *table );
    void       (*p_glGetColorTableEXT)( GLenum target, GLenum format, GLenum type, void *data );
    void       (*p_glGetColorTableParameterfv)( GLenum target, GLenum pname, GLfloat *params );
    void       (*p_glGetColorTableParameterfvEXT)( GLenum target, GLenum pname, GLfloat *params );
    void       (*p_glGetColorTableParameterfvSGI)( GLenum target, GLenum pname, GLfloat *params );
    void       (*p_glGetColorTableParameteriv)( GLenum target, GLenum pname, GLint *params );
    void       (*p_glGetColorTableParameterivEXT)( GLenum target, GLenum pname, GLint *params );
    void       (*p_glGetColorTableParameterivSGI)( GLenum target, GLenum pname, GLint *params );
    void       (*p_glGetColorTableSGI)( GLenum target, GLenum format, GLenum type, void *table );
    void       (*p_glGetCombinerInputParameterfvNV)( GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLfloat *params );
    void       (*p_glGetCombinerInputParameterivNV)( GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLint *params );
    void       (*p_glGetCombinerOutputParameterfvNV)( GLenum stage, GLenum portion, GLenum pname, GLfloat *params );
    void       (*p_glGetCombinerOutputParameterivNV)( GLenum stage, GLenum portion, GLenum pname, GLint *params );
    void       (*p_glGetCombinerStageParameterfvNV)( GLenum stage, GLenum pname, GLfloat *params );
    GLuint     (*p_glGetCommandHeaderNV)( GLenum tokenID, GLuint size );
    void       (*p_glGetCompressedMultiTexImageEXT)( GLenum texunit, GLenum target, GLint lod, void *img );
    void       (*p_glGetCompressedTexImage)( GLenum target, GLint level, void *img );
    void       (*p_glGetCompressedTexImageARB)( GLenum target, GLint level, void *img );
    void       (*p_glGetCompressedTextureImage)( GLuint texture, GLint level, GLsizei bufSize, void *pixels );
    void       (*p_glGetCompressedTextureImageEXT)( GLuint texture, GLenum target, GLint lod, void *img );
    void       (*p_glGetCompressedTextureSubImage)( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei bufSize, void *pixels );
    void       (*p_glGetConvolutionFilter)( GLenum target, GLenum format, GLenum type, void *image );
    void       (*p_glGetConvolutionFilterEXT)( GLenum target, GLenum format, GLenum type, void *image );
    void       (*p_glGetConvolutionParameterfv)( GLenum target, GLenum pname, GLfloat *params );
    void       (*p_glGetConvolutionParameterfvEXT)( GLenum target, GLenum pname, GLfloat *params );
    void       (*p_glGetConvolutionParameteriv)( GLenum target, GLenum pname, GLint *params );
    void       (*p_glGetConvolutionParameterivEXT)( GLenum target, GLenum pname, GLint *params );
    void       (*p_glGetConvolutionParameterxvOES)( GLenum target, GLenum pname, GLfixed *params );
    void       (*p_glGetCoverageModulationTableNV)( GLsizei bufSize, GLfloat *v );
    GLuint     (*p_glGetDebugMessageLog)( GLuint count, GLsizei bufSize, GLenum *sources, GLenum *types, GLuint *ids, GLenum *severities, GLsizei *lengths, GLchar *messageLog );
    GLuint     (*p_glGetDebugMessageLogAMD)( GLuint count, GLsizei bufSize, GLenum *categories, GLuint *severities, GLuint *ids, GLsizei *lengths, GLchar *message );
    GLuint     (*p_glGetDebugMessageLogARB)( GLuint count, GLsizei bufSize, GLenum *sources, GLenum *types, GLuint *ids, GLenum *severities, GLsizei *lengths, GLchar *messageLog );
    void       (*p_glGetDetailTexFuncSGIS)( GLenum target, GLfloat *points );
    void       (*p_glGetDoubleIndexedvEXT)( GLenum target, GLuint index, GLdouble *data );
    void       (*p_glGetDoublei_v)( GLenum target, GLuint index, GLdouble *data );
    void       (*p_glGetDoublei_vEXT)( GLenum pname, GLuint index, GLdouble *params );
    void       (*p_glGetFenceivNV)( GLuint fence, GLenum pname, GLint *params );
    void       (*p_glGetFinalCombinerInputParameterfvNV)( GLenum variable, GLenum pname, GLfloat *params );
    void       (*p_glGetFinalCombinerInputParameterivNV)( GLenum variable, GLenum pname, GLint *params );
    void       (*p_glGetFirstPerfQueryIdINTEL)( GLuint *queryId );
    void       (*p_glGetFixedvOES)( GLenum pname, GLfixed *params );
    void       (*p_glGetFloatIndexedvEXT)( GLenum target, GLuint index, GLfloat *data );
    void       (*p_glGetFloati_v)( GLenum target, GLuint index, GLfloat *data );
    void       (*p_glGetFloati_vEXT)( GLenum pname, GLuint index, GLfloat *params );
    void       (*p_glGetFogFuncSGIS)( GLfloat *points );
    GLint      (*p_glGetFragDataIndex)( GLuint program, const GLchar *name );
    GLint      (*p_glGetFragDataLocation)( GLuint program, const GLchar *name );
    GLint      (*p_glGetFragDataLocationEXT)( GLuint program, const GLchar *name );
    void       (*p_glGetFragmentLightfvSGIX)( GLenum light, GLenum pname, GLfloat *params );
    void       (*p_glGetFragmentLightivSGIX)( GLenum light, GLenum pname, GLint *params );
    void       (*p_glGetFragmentMaterialfvSGIX)( GLenum face, GLenum pname, GLfloat *params );
    void       (*p_glGetFragmentMaterialivSGIX)( GLenum face, GLenum pname, GLint *params );
    void       (*p_glGetFramebufferAttachmentParameteriv)( GLenum target, GLenum attachment, GLenum pname, GLint *params );
    void       (*p_glGetFramebufferAttachmentParameterivEXT)( GLenum target, GLenum attachment, GLenum pname, GLint *params );
    void       (*p_glGetFramebufferParameterfvAMD)( GLenum target, GLenum pname, GLuint numsamples, GLuint pixelindex, GLsizei size, GLfloat *values );
    void       (*p_glGetFramebufferParameteriv)( GLenum target, GLenum pname, GLint *params );
    void       (*p_glGetFramebufferParameterivEXT)( GLuint framebuffer, GLenum pname, GLint *params );
    void       (*p_glGetFramebufferParameterivMESA)( GLenum target, GLenum pname, GLint *params );
    GLenum     (*p_glGetGraphicsResetStatus)(void);
    GLenum     (*p_glGetGraphicsResetStatusARB)(void);
    GLhandleARB (*p_glGetHandleARB)( GLenum pname );
    void       (*p_glGetHistogram)( GLenum target, GLboolean reset, GLenum format, GLenum type, void *values );
    void       (*p_glGetHistogramEXT)( GLenum target, GLboolean reset, GLenum format, GLenum type, void *values );
    void       (*p_glGetHistogramParameterfv)( GLenum target, GLenum pname, GLfloat *params );
    void       (*p_glGetHistogramParameterfvEXT)( GLenum target, GLenum pname, GLfloat *params );
    void       (*p_glGetHistogramParameteriv)( GLenum target, GLenum pname, GLint *params );
    void       (*p_glGetHistogramParameterivEXT)( GLenum target, GLenum pname, GLint *params );
    void       (*p_glGetHistogramParameterxvOES)( GLenum target, GLenum pname, GLfixed *params );
    GLuint64   (*p_glGetImageHandleARB)( GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum format );
    GLuint64   (*p_glGetImageHandleNV)( GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum format );
    void       (*p_glGetImageTransformParameterfvHP)( GLenum target, GLenum pname, GLfloat *params );
    void       (*p_glGetImageTransformParameterivHP)( GLenum target, GLenum pname, GLint *params );
    void       (*p_glGetInfoLogARB)( GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *infoLog );
    GLint      (*p_glGetInstrumentsSGIX)(void);
    void       (*p_glGetInteger64i_v)( GLenum target, GLuint index, GLint64 *data );
    void       (*p_glGetInteger64v)( GLenum pname, GLint64 *data );
    void       (*p_glGetIntegerIndexedvEXT)( GLenum target, GLuint index, GLint *data );
    void       (*p_glGetIntegeri_v)( GLenum target, GLuint index, GLint *data );
    void       (*p_glGetIntegerui64i_vNV)( GLenum value, GLuint index, GLuint64EXT *result );
    void       (*p_glGetIntegerui64vNV)( GLenum value, GLuint64EXT *result );
    void       (*p_glGetInternalformatSampleivNV)( GLenum target, GLenum internalformat, GLsizei samples, GLenum pname, GLsizei count, GLint *params );
    void       (*p_glGetInternalformati64v)( GLenum target, GLenum internalformat, GLenum pname, GLsizei count, GLint64 *params );
    void       (*p_glGetInternalformativ)( GLenum target, GLenum internalformat, GLenum pname, GLsizei count, GLint *params );
    void       (*p_glGetInvariantBooleanvEXT)( GLuint id, GLenum value, GLboolean *data );
    void       (*p_glGetInvariantFloatvEXT)( GLuint id, GLenum value, GLfloat *data );
    void       (*p_glGetInvariantIntegervEXT)( GLuint id, GLenum value, GLint *data );
    void       (*p_glGetLightxOES)( GLenum light, GLenum pname, GLfixed *params );
    void       (*p_glGetListParameterfvSGIX)( GLuint list, GLenum pname, GLfloat *params );
    void       (*p_glGetListParameterivSGIX)( GLuint list, GLenum pname, GLint *params );
    void       (*p_glGetLocalConstantBooleanvEXT)( GLuint id, GLenum value, GLboolean *data );
    void       (*p_glGetLocalConstantFloatvEXT)( GLuint id, GLenum value, GLfloat *data );
    void       (*p_glGetLocalConstantIntegervEXT)( GLuint id, GLenum value, GLint *data );
    void       (*p_glGetMapAttribParameterfvNV)( GLenum target, GLuint index, GLenum pname, GLfloat *params );
    void       (*p_glGetMapAttribParameterivNV)( GLenum target, GLuint index, GLenum pname, GLint *params );
    void       (*p_glGetMapControlPointsNV)( GLenum target, GLuint index, GLenum type, GLsizei ustride, GLsizei vstride, GLboolean packed, void *points );
    void       (*p_glGetMapParameterfvNV)( GLenum target, GLenum pname, GLfloat *params );
    void       (*p_glGetMapParameterivNV)( GLenum target, GLenum pname, GLint *params );
    void       (*p_glGetMapxvOES)( GLenum target, GLenum query, GLfixed *v );
    void       (*p_glGetMaterialxOES)( GLenum face, GLenum pname, GLfixed param );
    void       (*p_glGetMemoryObjectDetachedResourcesuivNV)( GLuint memory, GLenum pname, GLint first, GLsizei count, GLuint *params );
    void       (*p_glGetMemoryObjectParameterivEXT)( GLuint memoryObject, GLenum pname, GLint *params );
    void       (*p_glGetMinmax)( GLenum target, GLboolean reset, GLenum format, GLenum type, void *values );
    void       (*p_glGetMinmaxEXT)( GLenum target, GLboolean reset, GLenum format, GLenum type, void *values );
    void       (*p_glGetMinmaxParameterfv)( GLenum target, GLenum pname, GLfloat *params );
    void       (*p_glGetMinmaxParameterfvEXT)( GLenum target, GLenum pname, GLfloat *params );
    void       (*p_glGetMinmaxParameteriv)( GLenum target, GLenum pname, GLint *params );
    void       (*p_glGetMinmaxParameterivEXT)( GLenum target, GLenum pname, GLint *params );
    void       (*p_glGetMultiTexEnvfvEXT)( GLenum texunit, GLenum target, GLenum pname, GLfloat *params );
    void       (*p_glGetMultiTexEnvivEXT)( GLenum texunit, GLenum target, GLenum pname, GLint *params );
    void       (*p_glGetMultiTexGendvEXT)( GLenum texunit, GLenum coord, GLenum pname, GLdouble *params );
    void       (*p_glGetMultiTexGenfvEXT)( GLenum texunit, GLenum coord, GLenum pname, GLfloat *params );
    void       (*p_glGetMultiTexGenivEXT)( GLenum texunit, GLenum coord, GLenum pname, GLint *params );
    void       (*p_glGetMultiTexImageEXT)( GLenum texunit, GLenum target, GLint level, GLenum format, GLenum type, void *pixels );
    void       (*p_glGetMultiTexLevelParameterfvEXT)( GLenum texunit, GLenum target, GLint level, GLenum pname, GLfloat *params );
    void       (*p_glGetMultiTexLevelParameterivEXT)( GLenum texunit, GLenum target, GLint level, GLenum pname, GLint *params );
    void       (*p_glGetMultiTexParameterIivEXT)( GLenum texunit, GLenum target, GLenum pname, GLint *params );
    void       (*p_glGetMultiTexParameterIuivEXT)( GLenum texunit, GLenum target, GLenum pname, GLuint *params );
    void       (*p_glGetMultiTexParameterfvEXT)( GLenum texunit, GLenum target, GLenum pname, GLfloat *params );
    void       (*p_glGetMultiTexParameterivEXT)( GLenum texunit, GLenum target, GLenum pname, GLint *params );
    void       (*p_glGetMultisamplefv)( GLenum pname, GLuint index, GLfloat *val );
    void       (*p_glGetMultisamplefvNV)( GLenum pname, GLuint index, GLfloat *val );
    void       (*p_glGetNamedBufferParameteri64v)( GLuint buffer, GLenum pname, GLint64 *params );
    void       (*p_glGetNamedBufferParameteriv)( GLuint buffer, GLenum pname, GLint *params );
    void       (*p_glGetNamedBufferParameterivEXT)( GLuint buffer, GLenum pname, GLint *params );
    void       (*p_glGetNamedBufferParameterui64vNV)( GLuint buffer, GLenum pname, GLuint64EXT *params );
    void       (*p_glGetNamedBufferPointerv)( GLuint buffer, GLenum pname, void **params );
    void       (*p_glGetNamedBufferPointervEXT)( GLuint buffer, GLenum pname, void **params );
    void       (*p_glGetNamedBufferSubData)( GLuint buffer, GLintptr offset, GLsizeiptr size, void *data );
    void       (*p_glGetNamedBufferSubDataEXT)( GLuint buffer, GLintptr offset, GLsizeiptr size, void *data );
    void       (*p_glGetNamedFramebufferAttachmentParameteriv)( GLuint framebuffer, GLenum attachment, GLenum pname, GLint *params );
    void       (*p_glGetNamedFramebufferAttachmentParameterivEXT)( GLuint framebuffer, GLenum attachment, GLenum pname, GLint *params );
    void       (*p_glGetNamedFramebufferParameterfvAMD)( GLuint framebuffer, GLenum pname, GLuint numsamples, GLuint pixelindex, GLsizei size, GLfloat *values );
    void       (*p_glGetNamedFramebufferParameteriv)( GLuint framebuffer, GLenum pname, GLint *param );
    void       (*p_glGetNamedFramebufferParameterivEXT)( GLuint framebuffer, GLenum pname, GLint *params );
    void       (*p_glGetNamedProgramLocalParameterIivEXT)( GLuint program, GLenum target, GLuint index, GLint *params );
    void       (*p_glGetNamedProgramLocalParameterIuivEXT)( GLuint program, GLenum target, GLuint index, GLuint *params );
    void       (*p_glGetNamedProgramLocalParameterdvEXT)( GLuint program, GLenum target, GLuint index, GLdouble *params );
    void       (*p_glGetNamedProgramLocalParameterfvEXT)( GLuint program, GLenum target, GLuint index, GLfloat *params );
    void       (*p_glGetNamedProgramStringEXT)( GLuint program, GLenum target, GLenum pname, void *string );
    void       (*p_glGetNamedProgramivEXT)( GLuint program, GLenum target, GLenum pname, GLint *params );
    void       (*p_glGetNamedRenderbufferParameteriv)( GLuint renderbuffer, GLenum pname, GLint *params );
    void       (*p_glGetNamedRenderbufferParameterivEXT)( GLuint renderbuffer, GLenum pname, GLint *params );
    void       (*p_glGetNamedStringARB)( GLint namelen, const GLchar *name, GLsizei bufSize, GLint *stringlen, GLchar *string );
    void       (*p_glGetNamedStringivARB)( GLint namelen, const GLchar *name, GLenum pname, GLint *params );
    void       (*p_glGetNextPerfQueryIdINTEL)( GLuint queryId, GLuint *nextQueryId );
    void       (*p_glGetObjectBufferfvATI)( GLuint buffer, GLenum pname, GLfloat *params );
    void       (*p_glGetObjectBufferivATI)( GLuint buffer, GLenum pname, GLint *params );
    void       (*p_glGetObjectLabel)( GLenum identifier, GLuint name, GLsizei bufSize, GLsizei *length, GLchar *label );
    void       (*p_glGetObjectLabelEXT)( GLenum type, GLuint object, GLsizei bufSize, GLsizei *length, GLchar *label );
    void       (*p_glGetObjectParameterfvARB)( GLhandleARB obj, GLenum pname, GLfloat *params );
    void       (*p_glGetObjectParameterivAPPLE)( GLenum objectType, GLuint name, GLenum pname, GLint *params );
    void       (*p_glGetObjectParameterivARB)( GLhandleARB obj, GLenum pname, GLint *params );
    void       (*p_glGetObjectPtrLabel)( const void *ptr, GLsizei bufSize, GLsizei *length, GLchar *label );
    void       (*p_glGetOcclusionQueryivNV)( GLuint id, GLenum pname, GLint *params );
    void       (*p_glGetOcclusionQueryuivNV)( GLuint id, GLenum pname, GLuint *params );
    void       (*p_glGetPathColorGenfvNV)( GLenum color, GLenum pname, GLfloat *value );
    void       (*p_glGetPathColorGenivNV)( GLenum color, GLenum pname, GLint *value );
    void       (*p_glGetPathCommandsNV)( GLuint path, GLubyte *commands );
    void       (*p_glGetPathCoordsNV)( GLuint path, GLfloat *coords );
    void       (*p_glGetPathDashArrayNV)( GLuint path, GLfloat *dashArray );
    GLfloat    (*p_glGetPathLengthNV)( GLuint path, GLsizei startSegment, GLsizei numSegments );
    void       (*p_glGetPathMetricRangeNV)( GLbitfield metricQueryMask, GLuint firstPathName, GLsizei numPaths, GLsizei stride, GLfloat *metrics );
    void       (*p_glGetPathMetricsNV)( GLbitfield metricQueryMask, GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLsizei stride, GLfloat *metrics );
    void       (*p_glGetPathParameterfvNV)( GLuint path, GLenum pname, GLfloat *value );
    void       (*p_glGetPathParameterivNV)( GLuint path, GLenum pname, GLint *value );
    void       (*p_glGetPathSpacingNV)( GLenum pathListMode, GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLfloat advanceScale, GLfloat kerningScale, GLenum transformType, GLfloat *returnedSpacing );
    void       (*p_glGetPathTexGenfvNV)( GLenum texCoordSet, GLenum pname, GLfloat *value );
    void       (*p_glGetPathTexGenivNV)( GLenum texCoordSet, GLenum pname, GLint *value );
    void       (*p_glGetPerfCounterInfoINTEL)( GLuint queryId, GLuint counterId, GLuint counterNameLength, GLchar *counterName, GLuint counterDescLength, GLchar *counterDesc, GLuint *counterOffset, GLuint *counterDataSize, GLuint *counterTypeEnum, GLuint *counterDataTypeEnum, GLuint64 *rawCounterMaxValue );
    void       (*p_glGetPerfMonitorCounterDataAMD)( GLuint monitor, GLenum pname, GLsizei dataSize, GLuint *data, GLint *bytesWritten );
    void       (*p_glGetPerfMonitorCounterInfoAMD)( GLuint group, GLuint counter, GLenum pname, void *data );
    void       (*p_glGetPerfMonitorCounterStringAMD)( GLuint group, GLuint counter, GLsizei bufSize, GLsizei *length, GLchar *counterString );
    void       (*p_glGetPerfMonitorCountersAMD)( GLuint group, GLint *numCounters, GLint *maxActiveCounters, GLsizei counterSize, GLuint *counters );
    void       (*p_glGetPerfMonitorGroupStringAMD)( GLuint group, GLsizei bufSize, GLsizei *length, GLchar *groupString );
    void       (*p_glGetPerfMonitorGroupsAMD)( GLint *numGroups, GLsizei groupsSize, GLuint *groups );
    void       (*p_glGetPerfQueryDataINTEL)( GLuint queryHandle, GLuint flags, GLsizei dataSize, void *data, GLuint *bytesWritten );
    void       (*p_glGetPerfQueryIdByNameINTEL)( GLchar *queryName, GLuint *queryId );
    void       (*p_glGetPerfQueryInfoINTEL)( GLuint queryId, GLuint queryNameLength, GLchar *queryName, GLuint *dataSize, GLuint *noCounters, GLuint *noInstances, GLuint *capsMask );
    void       (*p_glGetPixelMapxv)( GLenum map, GLint size, GLfixed *values );
    void       (*p_glGetPixelTexGenParameterfvSGIS)( GLenum pname, GLfloat *params );
    void       (*p_glGetPixelTexGenParameterivSGIS)( GLenum pname, GLint *params );
    void       (*p_glGetPixelTransformParameterfvEXT)( GLenum target, GLenum pname, GLfloat *params );
    void       (*p_glGetPixelTransformParameterivEXT)( GLenum target, GLenum pname, GLint *params );
    void       (*p_glGetPointerIndexedvEXT)( GLenum target, GLuint index, void **data );
    void       (*p_glGetPointeri_vEXT)( GLenum pname, GLuint index, void **params );
    void       (*p_glGetPointervEXT)( GLenum pname, void **params );
    void       (*p_glGetProgramBinary)( GLuint program, GLsizei bufSize, GLsizei *length, GLenum *binaryFormat, void *binary );
    void       (*p_glGetProgramEnvParameterIivNV)( GLenum target, GLuint index, GLint *params );
    void       (*p_glGetProgramEnvParameterIuivNV)( GLenum target, GLuint index, GLuint *params );
    void       (*p_glGetProgramEnvParameterdvARB)( GLenum target, GLuint index, GLdouble *params );
    void       (*p_glGetProgramEnvParameterfvARB)( GLenum target, GLuint index, GLfloat *params );
    void       (*p_glGetProgramInfoLog)( GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog );
    void       (*p_glGetProgramInterfaceiv)( GLuint program, GLenum programInterface, GLenum pname, GLint *params );
    void       (*p_glGetProgramLocalParameterIivNV)( GLenum target, GLuint index, GLint *params );
    void       (*p_glGetProgramLocalParameterIuivNV)( GLenum target, GLuint index, GLuint *params );
    void       (*p_glGetProgramLocalParameterdvARB)( GLenum target, GLuint index, GLdouble *params );
    void       (*p_glGetProgramLocalParameterfvARB)( GLenum target, GLuint index, GLfloat *params );
    void       (*p_glGetProgramNamedParameterdvNV)( GLuint id, GLsizei len, const GLubyte *name, GLdouble *params );
    void       (*p_glGetProgramNamedParameterfvNV)( GLuint id, GLsizei len, const GLubyte *name, GLfloat *params );
    void       (*p_glGetProgramParameterdvNV)( GLenum target, GLuint index, GLenum pname, GLdouble *params );
    void       (*p_glGetProgramParameterfvNV)( GLenum target, GLuint index, GLenum pname, GLfloat *params );
    void       (*p_glGetProgramPipelineInfoLog)( GLuint pipeline, GLsizei bufSize, GLsizei *length, GLchar *infoLog );
    void       (*p_glGetProgramPipelineiv)( GLuint pipeline, GLenum pname, GLint *params );
    GLuint     (*p_glGetProgramResourceIndex)( GLuint program, GLenum programInterface, const GLchar *name );
    GLint      (*p_glGetProgramResourceLocation)( GLuint program, GLenum programInterface, const GLchar *name );
    GLint      (*p_glGetProgramResourceLocationIndex)( GLuint program, GLenum programInterface, const GLchar *name );
    void       (*p_glGetProgramResourceName)( GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name );
    void       (*p_glGetProgramResourcefvNV)( GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum *props, GLsizei count, GLsizei *length, GLfloat *params );
    void       (*p_glGetProgramResourceiv)( GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum *props, GLsizei count, GLsizei *length, GLint *params );
    void       (*p_glGetProgramStageiv)( GLuint program, GLenum shadertype, GLenum pname, GLint *values );
    void       (*p_glGetProgramStringARB)( GLenum target, GLenum pname, void *string );
    void       (*p_glGetProgramStringNV)( GLuint id, GLenum pname, GLubyte *program );
    void       (*p_glGetProgramSubroutineParameteruivNV)( GLenum target, GLuint index, GLuint *param );
    void       (*p_glGetProgramiv)( GLuint program, GLenum pname, GLint *params );
    void       (*p_glGetProgramivARB)( GLenum target, GLenum pname, GLint *params );
    void       (*p_glGetProgramivNV)( GLuint id, GLenum pname, GLint *params );
    void       (*p_glGetQueryBufferObjecti64v)( GLuint id, GLuint buffer, GLenum pname, GLintptr offset );
    void       (*p_glGetQueryBufferObjectiv)( GLuint id, GLuint buffer, GLenum pname, GLintptr offset );
    void       (*p_glGetQueryBufferObjectui64v)( GLuint id, GLuint buffer, GLenum pname, GLintptr offset );
    void       (*p_glGetQueryBufferObjectuiv)( GLuint id, GLuint buffer, GLenum pname, GLintptr offset );
    void       (*p_glGetQueryIndexediv)( GLenum target, GLuint index, GLenum pname, GLint *params );
    void       (*p_glGetQueryObjecti64v)( GLuint id, GLenum pname, GLint64 *params );
    void       (*p_glGetQueryObjecti64vEXT)( GLuint id, GLenum pname, GLint64 *params );
    void       (*p_glGetQueryObjectiv)( GLuint id, GLenum pname, GLint *params );
    void       (*p_glGetQueryObjectivARB)( GLuint id, GLenum pname, GLint *params );
    void       (*p_glGetQueryObjectui64v)( GLuint id, GLenum pname, GLuint64 *params );
    void       (*p_glGetQueryObjectui64vEXT)( GLuint id, GLenum pname, GLuint64 *params );
    void       (*p_glGetQueryObjectuiv)( GLuint id, GLenum pname, GLuint *params );
    void       (*p_glGetQueryObjectuivARB)( GLuint id, GLenum pname, GLuint *params );
    void       (*p_glGetQueryiv)( GLenum target, GLenum pname, GLint *params );
    void       (*p_glGetQueryivARB)( GLenum target, GLenum pname, GLint *params );
    void       (*p_glGetRenderbufferParameteriv)( GLenum target, GLenum pname, GLint *params );
    void       (*p_glGetRenderbufferParameterivEXT)( GLenum target, GLenum pname, GLint *params );
    void       (*p_glGetSamplerParameterIiv)( GLuint sampler, GLenum pname, GLint *params );
    void       (*p_glGetSamplerParameterIuiv)( GLuint sampler, GLenum pname, GLuint *params );
    void       (*p_glGetSamplerParameterfv)( GLuint sampler, GLenum pname, GLfloat *params );
    void       (*p_glGetSamplerParameteriv)( GLuint sampler, GLenum pname, GLint *params );
    void       (*p_glGetSemaphoreParameterui64vEXT)( GLuint semaphore, GLenum pname, GLuint64 *params );
    void       (*p_glGetSeparableFilter)( GLenum target, GLenum format, GLenum type, void *row, void *column, void *span );
    void       (*p_glGetSeparableFilterEXT)( GLenum target, GLenum format, GLenum type, void *row, void *column, void *span );
    void       (*p_glGetShaderInfoLog)( GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog );
    void       (*p_glGetShaderPrecisionFormat)( GLenum shadertype, GLenum precisiontype, GLint *range, GLint *precision );
    void       (*p_glGetShaderSource)( GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *source );
    void       (*p_glGetShaderSourceARB)( GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *source );
    void       (*p_glGetShaderiv)( GLuint shader, GLenum pname, GLint *params );
    void       (*p_glGetShadingRateImagePaletteNV)( GLuint viewport, GLuint entry, GLenum *rate );
    void       (*p_glGetShadingRateSampleLocationivNV)( GLenum rate, GLuint samples, GLuint index, GLint *location );
    void       (*p_glGetSharpenTexFuncSGIS)( GLenum target, GLfloat *points );
    GLushort   (*p_glGetStageIndexNV)( GLenum shadertype );
    const GLubyte * (*p_glGetStringi)( GLenum name, GLuint index );
    GLuint     (*p_glGetSubroutineIndex)( GLuint program, GLenum shadertype, const GLchar *name );
    GLint      (*p_glGetSubroutineUniformLocation)( GLuint program, GLenum shadertype, const GLchar *name );
    void       (*p_glGetSynciv)( GLsync sync, GLenum pname, GLsizei count, GLsizei *length, GLint *values );
    void       (*p_glGetTexBumpParameterfvATI)( GLenum pname, GLfloat *param );
    void       (*p_glGetTexBumpParameterivATI)( GLenum pname, GLint *param );
    void       (*p_glGetTexEnvxvOES)( GLenum target, GLenum pname, GLfixed *params );
    void       (*p_glGetTexFilterFuncSGIS)( GLenum target, GLenum filter, GLfloat *weights );
    void       (*p_glGetTexGenxvOES)( GLenum coord, GLenum pname, GLfixed *params );
    void       (*p_glGetTexLevelParameterxvOES)( GLenum target, GLint level, GLenum pname, GLfixed *params );
    void       (*p_glGetTexParameterIiv)( GLenum target, GLenum pname, GLint *params );
    void       (*p_glGetTexParameterIivEXT)( GLenum target, GLenum pname, GLint *params );
    void       (*p_glGetTexParameterIuiv)( GLenum target, GLenum pname, GLuint *params );
    void       (*p_glGetTexParameterIuivEXT)( GLenum target, GLenum pname, GLuint *params );
    void       (*p_glGetTexParameterPointervAPPLE)( GLenum target, GLenum pname, void **params );
    void       (*p_glGetTexParameterxvOES)( GLenum target, GLenum pname, GLfixed *params );
    GLuint64   (*p_glGetTextureHandleARB)( GLuint texture );
    GLuint64   (*p_glGetTextureHandleNV)( GLuint texture );
    void       (*p_glGetTextureImage)( GLuint texture, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *pixels );
    void       (*p_glGetTextureImageEXT)( GLuint texture, GLenum target, GLint level, GLenum format, GLenum type, void *pixels );
    void       (*p_glGetTextureLevelParameterfv)( GLuint texture, GLint level, GLenum pname, GLfloat *params );
    void       (*p_glGetTextureLevelParameterfvEXT)( GLuint texture, GLenum target, GLint level, GLenum pname, GLfloat *params );
    void       (*p_glGetTextureLevelParameteriv)( GLuint texture, GLint level, GLenum pname, GLint *params );
    void       (*p_glGetTextureLevelParameterivEXT)( GLuint texture, GLenum target, GLint level, GLenum pname, GLint *params );
    void       (*p_glGetTextureParameterIiv)( GLuint texture, GLenum pname, GLint *params );
    void       (*p_glGetTextureParameterIivEXT)( GLuint texture, GLenum target, GLenum pname, GLint *params );
    void       (*p_glGetTextureParameterIuiv)( GLuint texture, GLenum pname, GLuint *params );
    void       (*p_glGetTextureParameterIuivEXT)( GLuint texture, GLenum target, GLenum pname, GLuint *params );
    void       (*p_glGetTextureParameterfv)( GLuint texture, GLenum pname, GLfloat *params );
    void       (*p_glGetTextureParameterfvEXT)( GLuint texture, GLenum target, GLenum pname, GLfloat *params );
    void       (*p_glGetTextureParameteriv)( GLuint texture, GLenum pname, GLint *params );
    void       (*p_glGetTextureParameterivEXT)( GLuint texture, GLenum target, GLenum pname, GLint *params );
    GLuint64   (*p_glGetTextureSamplerHandleARB)( GLuint texture, GLuint sampler );
    GLuint64   (*p_glGetTextureSamplerHandleNV)( GLuint texture, GLuint sampler );
    void       (*p_glGetTextureSubImage)( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, GLsizei bufSize, void *pixels );
    void       (*p_glGetTrackMatrixivNV)( GLenum target, GLuint address, GLenum pname, GLint *params );
    void       (*p_glGetTransformFeedbackVarying)( GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name );
    void       (*p_glGetTransformFeedbackVaryingEXT)( GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name );
    void       (*p_glGetTransformFeedbackVaryingNV)( GLuint program, GLuint index, GLint *location );
    void       (*p_glGetTransformFeedbacki64_v)( GLuint xfb, GLenum pname, GLuint index, GLint64 *param );
    void       (*p_glGetTransformFeedbacki_v)( GLuint xfb, GLenum pname, GLuint index, GLint *param );
    void       (*p_glGetTransformFeedbackiv)( GLuint xfb, GLenum pname, GLint *param );
    GLuint     (*p_glGetUniformBlockIndex)( GLuint program, const GLchar *uniformBlockName );
    GLint      (*p_glGetUniformBufferSizeEXT)( GLuint program, GLint location );
    void       (*p_glGetUniformIndices)( GLuint program, GLsizei uniformCount, const GLchar *const*uniformNames, GLuint *uniformIndices );
    GLint      (*p_glGetUniformLocation)( GLuint program, const GLchar *name );
    GLint      (*p_glGetUniformLocationARB)( GLhandleARB programObj, const GLcharARB *name );
    GLintptr   (*p_glGetUniformOffsetEXT)( GLuint program, GLint location );
    void       (*p_glGetUniformSubroutineuiv)( GLenum shadertype, GLint location, GLuint *params );
    void       (*p_glGetUniformdv)( GLuint program, GLint location, GLdouble *params );
    void       (*p_glGetUniformfv)( GLuint program, GLint location, GLfloat *params );
    void       (*p_glGetUniformfvARB)( GLhandleARB programObj, GLint location, GLfloat *params );
    void       (*p_glGetUniformi64vARB)( GLuint program, GLint location, GLint64 *params );
    void       (*p_glGetUniformi64vNV)( GLuint program, GLint location, GLint64EXT *params );
    void       (*p_glGetUniformiv)( GLuint program, GLint location, GLint *params );
    void       (*p_glGetUniformivARB)( GLhandleARB programObj, GLint location, GLint *params );
    void       (*p_glGetUniformui64vARB)( GLuint program, GLint location, GLuint64 *params );
    void       (*p_glGetUniformui64vNV)( GLuint program, GLint location, GLuint64EXT *params );
    void       (*p_glGetUniformuiv)( GLuint program, GLint location, GLuint *params );
    void       (*p_glGetUniformuivEXT)( GLuint program, GLint location, GLuint *params );
    void       (*p_glGetUnsignedBytei_vEXT)( GLenum target, GLuint index, GLubyte *data );
    void       (*p_glGetUnsignedBytevEXT)( GLenum pname, GLubyte *data );
    void       (*p_glGetVariantArrayObjectfvATI)( GLuint id, GLenum pname, GLfloat *params );
    void       (*p_glGetVariantArrayObjectivATI)( GLuint id, GLenum pname, GLint *params );
    void       (*p_glGetVariantBooleanvEXT)( GLuint id, GLenum value, GLboolean *data );
    void       (*p_glGetVariantFloatvEXT)( GLuint id, GLenum value, GLfloat *data );
    void       (*p_glGetVariantIntegervEXT)( GLuint id, GLenum value, GLint *data );
    void       (*p_glGetVariantPointervEXT)( GLuint id, GLenum value, void **data );
    GLint      (*p_glGetVaryingLocationNV)( GLuint program, const GLchar *name );
    void       (*p_glGetVertexArrayIndexed64iv)( GLuint vaobj, GLuint index, GLenum pname, GLint64 *param );
    void       (*p_glGetVertexArrayIndexediv)( GLuint vaobj, GLuint index, GLenum pname, GLint *param );
    void       (*p_glGetVertexArrayIntegeri_vEXT)( GLuint vaobj, GLuint index, GLenum pname, GLint *param );
    void       (*p_glGetVertexArrayIntegervEXT)( GLuint vaobj, GLenum pname, GLint *param );
    void       (*p_glGetVertexArrayPointeri_vEXT)( GLuint vaobj, GLuint index, GLenum pname, void **param );
    void       (*p_glGetVertexArrayPointervEXT)( GLuint vaobj, GLenum pname, void **param );
    void       (*p_glGetVertexArrayiv)( GLuint vaobj, GLenum pname, GLint *param );
    void       (*p_glGetVertexAttribArrayObjectfvATI)( GLuint index, GLenum pname, GLfloat *params );
    void       (*p_glGetVertexAttribArrayObjectivATI)( GLuint index, GLenum pname, GLint *params );
    void       (*p_glGetVertexAttribIiv)( GLuint index, GLenum pname, GLint *params );
    void       (*p_glGetVertexAttribIivEXT)( GLuint index, GLenum pname, GLint *params );
    void       (*p_glGetVertexAttribIuiv)( GLuint index, GLenum pname, GLuint *params );
    void       (*p_glGetVertexAttribIuivEXT)( GLuint index, GLenum pname, GLuint *params );
    void       (*p_glGetVertexAttribLdv)( GLuint index, GLenum pname, GLdouble *params );
    void       (*p_glGetVertexAttribLdvEXT)( GLuint index, GLenum pname, GLdouble *params );
    void       (*p_glGetVertexAttribLi64vNV)( GLuint index, GLenum pname, GLint64EXT *params );
    void       (*p_glGetVertexAttribLui64vARB)( GLuint index, GLenum pname, GLuint64EXT *params );
    void       (*p_glGetVertexAttribLui64vNV)( GLuint index, GLenum pname, GLuint64EXT *params );
    void       (*p_glGetVertexAttribPointerv)( GLuint index, GLenum pname, void **pointer );
    void       (*p_glGetVertexAttribPointervARB)( GLuint index, GLenum pname, void **pointer );
    void       (*p_glGetVertexAttribPointervNV)( GLuint index, GLenum pname, void **pointer );
    void       (*p_glGetVertexAttribdv)( GLuint index, GLenum pname, GLdouble *params );
    void       (*p_glGetVertexAttribdvARB)( GLuint index, GLenum pname, GLdouble *params );
    void       (*p_glGetVertexAttribdvNV)( GLuint index, GLenum pname, GLdouble *params );
    void       (*p_glGetVertexAttribfv)( GLuint index, GLenum pname, GLfloat *params );
    void       (*p_glGetVertexAttribfvARB)( GLuint index, GLenum pname, GLfloat *params );
    void       (*p_glGetVertexAttribfvNV)( GLuint index, GLenum pname, GLfloat *params );
    void       (*p_glGetVertexAttribiv)( GLuint index, GLenum pname, GLint *params );
    void       (*p_glGetVertexAttribivARB)( GLuint index, GLenum pname, GLint *params );
    void       (*p_glGetVertexAttribivNV)( GLuint index, GLenum pname, GLint *params );
    void       (*p_glGetVideoCaptureStreamdvNV)( GLuint video_capture_slot, GLuint stream, GLenum pname, GLdouble *params );
    void       (*p_glGetVideoCaptureStreamfvNV)( GLuint video_capture_slot, GLuint stream, GLenum pname, GLfloat *params );
    void       (*p_glGetVideoCaptureStreamivNV)( GLuint video_capture_slot, GLuint stream, GLenum pname, GLint *params );
    void       (*p_glGetVideoCaptureivNV)( GLuint video_capture_slot, GLenum pname, GLint *params );
    void       (*p_glGetVideoi64vNV)( GLuint video_slot, GLenum pname, GLint64EXT *params );
    void       (*p_glGetVideoivNV)( GLuint video_slot, GLenum pname, GLint *params );
    void       (*p_glGetVideoui64vNV)( GLuint video_slot, GLenum pname, GLuint64EXT *params );
    void       (*p_glGetVideouivNV)( GLuint video_slot, GLenum pname, GLuint *params );
    GLVULKANPROCNV (*p_glGetVkProcAddrNV)( const GLchar *name );
    void       (*p_glGetnColorTable)( GLenum target, GLenum format, GLenum type, GLsizei bufSize, void *table );
    void       (*p_glGetnColorTableARB)( GLenum target, GLenum format, GLenum type, GLsizei bufSize, void *table );
    void       (*p_glGetnCompressedTexImage)( GLenum target, GLint lod, GLsizei bufSize, void *pixels );
    void       (*p_glGetnCompressedTexImageARB)( GLenum target, GLint lod, GLsizei bufSize, void *img );
    void       (*p_glGetnConvolutionFilter)( GLenum target, GLenum format, GLenum type, GLsizei bufSize, void *image );
    void       (*p_glGetnConvolutionFilterARB)( GLenum target, GLenum format, GLenum type, GLsizei bufSize, void *image );
    void       (*p_glGetnHistogram)( GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, void *values );
    void       (*p_glGetnHistogramARB)( GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, void *values );
    void       (*p_glGetnMapdv)( GLenum target, GLenum query, GLsizei bufSize, GLdouble *v );
    void       (*p_glGetnMapdvARB)( GLenum target, GLenum query, GLsizei bufSize, GLdouble *v );
    void       (*p_glGetnMapfv)( GLenum target, GLenum query, GLsizei bufSize, GLfloat *v );
    void       (*p_glGetnMapfvARB)( GLenum target, GLenum query, GLsizei bufSize, GLfloat *v );
    void       (*p_glGetnMapiv)( GLenum target, GLenum query, GLsizei bufSize, GLint *v );
    void       (*p_glGetnMapivARB)( GLenum target, GLenum query, GLsizei bufSize, GLint *v );
    void       (*p_glGetnMinmax)( GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, void *values );
    void       (*p_glGetnMinmaxARB)( GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, void *values );
    void       (*p_glGetnPixelMapfv)( GLenum map, GLsizei bufSize, GLfloat *values );
    void       (*p_glGetnPixelMapfvARB)( GLenum map, GLsizei bufSize, GLfloat *values );
    void       (*p_glGetnPixelMapuiv)( GLenum map, GLsizei bufSize, GLuint *values );
    void       (*p_glGetnPixelMapuivARB)( GLenum map, GLsizei bufSize, GLuint *values );
    void       (*p_glGetnPixelMapusv)( GLenum map, GLsizei bufSize, GLushort *values );
    void       (*p_glGetnPixelMapusvARB)( GLenum map, GLsizei bufSize, GLushort *values );
    void       (*p_glGetnPolygonStipple)( GLsizei bufSize, GLubyte *pattern );
    void       (*p_glGetnPolygonStippleARB)( GLsizei bufSize, GLubyte *pattern );
    void       (*p_glGetnSeparableFilter)( GLenum target, GLenum format, GLenum type, GLsizei rowBufSize, void *row, GLsizei columnBufSize, void *column, void *span );
    void       (*p_glGetnSeparableFilterARB)( GLenum target, GLenum format, GLenum type, GLsizei rowBufSize, void *row, GLsizei columnBufSize, void *column, void *span );
    void       (*p_glGetnTexImage)( GLenum target, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *pixels );
    void       (*p_glGetnTexImageARB)( GLenum target, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *img );
    void       (*p_glGetnUniformdv)( GLuint program, GLint location, GLsizei bufSize, GLdouble *params );
    void       (*p_glGetnUniformdvARB)( GLuint program, GLint location, GLsizei bufSize, GLdouble *params );
    void       (*p_glGetnUniformfv)( GLuint program, GLint location, GLsizei bufSize, GLfloat *params );
    void       (*p_glGetnUniformfvARB)( GLuint program, GLint location, GLsizei bufSize, GLfloat *params );
    void       (*p_glGetnUniformi64vARB)( GLuint program, GLint location, GLsizei bufSize, GLint64 *params );
    void       (*p_glGetnUniformiv)( GLuint program, GLint location, GLsizei bufSize, GLint *params );
    void       (*p_glGetnUniformivARB)( GLuint program, GLint location, GLsizei bufSize, GLint *params );
    void       (*p_glGetnUniformui64vARB)( GLuint program, GLint location, GLsizei bufSize, GLuint64 *params );
    void       (*p_glGetnUniformuiv)( GLuint program, GLint location, GLsizei bufSize, GLuint *params );
    void       (*p_glGetnUniformuivARB)( GLuint program, GLint location, GLsizei bufSize, GLuint *params );
    void       (*p_glGlobalAlphaFactorbSUN)( GLbyte factor );
    void       (*p_glGlobalAlphaFactordSUN)( GLdouble factor );
    void       (*p_glGlobalAlphaFactorfSUN)( GLfloat factor );
    void       (*p_glGlobalAlphaFactoriSUN)( GLint factor );
    void       (*p_glGlobalAlphaFactorsSUN)( GLshort factor );
    void       (*p_glGlobalAlphaFactorubSUN)( GLubyte factor );
    void       (*p_glGlobalAlphaFactoruiSUN)( GLuint factor );
    void       (*p_glGlobalAlphaFactorusSUN)( GLushort factor );
    void       (*p_glHintPGI)( GLenum target, GLint mode );
    void       (*p_glHistogram)( GLenum target, GLsizei width, GLenum internalformat, GLboolean sink );
    void       (*p_glHistogramEXT)( GLenum target, GLsizei width, GLenum internalformat, GLboolean sink );
    void       (*p_glIglooInterfaceSGIX)( GLenum pname, const void *params );
    void       (*p_glImageTransformParameterfHP)( GLenum target, GLenum pname, GLfloat param );
    void       (*p_glImageTransformParameterfvHP)( GLenum target, GLenum pname, const GLfloat *params );
    void       (*p_glImageTransformParameteriHP)( GLenum target, GLenum pname, GLint param );
    void       (*p_glImageTransformParameterivHP)( GLenum target, GLenum pname, const GLint *params );
    void       (*p_glImportMemoryFdEXT)( GLuint memory, GLuint64 size, GLenum handleType, GLint fd );
    void       (*p_glImportMemoryWin32HandleEXT)( GLuint memory, GLuint64 size, GLenum handleType, void *handle );
    void       (*p_glImportMemoryWin32NameEXT)( GLuint memory, GLuint64 size, GLenum handleType, const void *name );
    void       (*p_glImportSemaphoreFdEXT)( GLuint semaphore, GLenum handleType, GLint fd );
    void       (*p_glImportSemaphoreWin32HandleEXT)( GLuint semaphore, GLenum handleType, void *handle );
    void       (*p_glImportSemaphoreWin32NameEXT)( GLuint semaphore, GLenum handleType, const void *name );
    GLsync     (*p_glImportSyncEXT)( GLenum external_sync_type, GLintptr external_sync, GLbitfield flags );
    void       (*p_glIndexFormatNV)( GLenum type, GLsizei stride );
    void       (*p_glIndexFuncEXT)( GLenum func, GLclampf ref );
    void       (*p_glIndexMaterialEXT)( GLenum face, GLenum mode );
    void       (*p_glIndexPointerEXT)( GLenum type, GLsizei stride, GLsizei count, const void *pointer );
    void       (*p_glIndexPointerListIBM)( GLenum type, GLint stride, const void **pointer, GLint ptrstride );
    void       (*p_glIndexxOES)( GLfixed component );
    void       (*p_glIndexxvOES)( const GLfixed *component );
    void       (*p_glInsertComponentEXT)( GLuint res, GLuint src, GLuint num );
    void       (*p_glInsertEventMarkerEXT)( GLsizei length, const GLchar *marker );
    void       (*p_glInstrumentsBufferSGIX)( GLsizei size, GLint *buffer );
    void       (*p_glInterpolatePathsNV)( GLuint resultPath, GLuint pathA, GLuint pathB, GLfloat weight );
    void       (*p_glInvalidateBufferData)( GLuint buffer );
    void       (*p_glInvalidateBufferSubData)( GLuint buffer, GLintptr offset, GLsizeiptr length );
    void       (*p_glInvalidateFramebuffer)( GLenum target, GLsizei numAttachments, const GLenum *attachments );
    void       (*p_glInvalidateNamedFramebufferData)( GLuint framebuffer, GLsizei numAttachments, const GLenum *attachments );
    void       (*p_glInvalidateNamedFramebufferSubData)( GLuint framebuffer, GLsizei numAttachments, const GLenum *attachments, GLint x, GLint y, GLsizei width, GLsizei height );
    void       (*p_glInvalidateSubFramebuffer)( GLenum target, GLsizei numAttachments, const GLenum *attachments, GLint x, GLint y, GLsizei width, GLsizei height );
    void       (*p_glInvalidateTexImage)( GLuint texture, GLint level );
    void       (*p_glInvalidateTexSubImage)( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth );
    GLboolean  (*p_glIsAsyncMarkerSGIX)( GLuint marker );
    GLboolean  (*p_glIsBuffer)( GLuint buffer );
    GLboolean  (*p_glIsBufferARB)( GLuint buffer );
    GLboolean  (*p_glIsBufferResidentNV)( GLenum target );
    GLboolean  (*p_glIsCommandListNV)( GLuint list );
    GLboolean  (*p_glIsEnabledIndexedEXT)( GLenum target, GLuint index );
    GLboolean  (*p_glIsEnabledi)( GLenum target, GLuint index );
    GLboolean  (*p_glIsFenceAPPLE)( GLuint fence );
    GLboolean  (*p_glIsFenceNV)( GLuint fence );
    GLboolean  (*p_glIsFramebuffer)( GLuint framebuffer );
    GLboolean  (*p_glIsFramebufferEXT)( GLuint framebuffer );
    GLboolean  (*p_glIsImageHandleResidentARB)( GLuint64 handle );
    GLboolean  (*p_glIsImageHandleResidentNV)( GLuint64 handle );
    GLboolean  (*p_glIsMemoryObjectEXT)( GLuint memoryObject );
    GLboolean  (*p_glIsNameAMD)( GLenum identifier, GLuint name );
    GLboolean  (*p_glIsNamedBufferResidentNV)( GLuint buffer );
    GLboolean  (*p_glIsNamedStringARB)( GLint namelen, const GLchar *name );
    GLboolean  (*p_glIsObjectBufferATI)( GLuint buffer );
    GLboolean  (*p_glIsOcclusionQueryNV)( GLuint id );
    GLboolean  (*p_glIsPathNV)( GLuint path );
    GLboolean  (*p_glIsPointInFillPathNV)( GLuint path, GLuint mask, GLfloat x, GLfloat y );
    GLboolean  (*p_glIsPointInStrokePathNV)( GLuint path, GLfloat x, GLfloat y );
    GLboolean  (*p_glIsProgram)( GLuint program );
    GLboolean  (*p_glIsProgramARB)( GLuint program );
    GLboolean  (*p_glIsProgramNV)( GLuint id );
    GLboolean  (*p_glIsProgramPipeline)( GLuint pipeline );
    GLboolean  (*p_glIsQuery)( GLuint id );
    GLboolean  (*p_glIsQueryARB)( GLuint id );
    GLboolean  (*p_glIsRenderbuffer)( GLuint renderbuffer );
    GLboolean  (*p_glIsRenderbufferEXT)( GLuint renderbuffer );
    GLboolean  (*p_glIsSampler)( GLuint sampler );
    GLboolean  (*p_glIsSemaphoreEXT)( GLuint semaphore );
    GLboolean  (*p_glIsShader)( GLuint shader );
    GLboolean  (*p_glIsStateNV)( GLuint state );
    GLboolean  (*p_glIsSync)( GLsync sync );
    GLboolean  (*p_glIsTextureEXT)( GLuint texture );
    GLboolean  (*p_glIsTextureHandleResidentARB)( GLuint64 handle );
    GLboolean  (*p_glIsTextureHandleResidentNV)( GLuint64 handle );
    GLboolean  (*p_glIsTransformFeedback)( GLuint id );
    GLboolean  (*p_glIsTransformFeedbackNV)( GLuint id );
    GLboolean  (*p_glIsVariantEnabledEXT)( GLuint id, GLenum cap );
    GLboolean  (*p_glIsVertexArray)( GLuint array );
    GLboolean  (*p_glIsVertexArrayAPPLE)( GLuint array );
    GLboolean  (*p_glIsVertexAttribEnabledAPPLE)( GLuint index, GLenum pname );
    void       (*p_glLGPUCopyImageSubDataNVX)( GLuint sourceGpu, GLbitfield destinationGpuMask, GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srxY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei width, GLsizei height, GLsizei depth );
    void       (*p_glLGPUInterlockNVX)(void);
    void       (*p_glLGPUNamedBufferSubDataNVX)( GLbitfield gpuMask, GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data );
    void       (*p_glLabelObjectEXT)( GLenum type, GLuint object, GLsizei length, const GLchar *label );
    void       (*p_glLightEnviSGIX)( GLenum pname, GLint param );
    void       (*p_glLightModelxOES)( GLenum pname, GLfixed param );
    void       (*p_glLightModelxvOES)( GLenum pname, const GLfixed *param );
    void       (*p_glLightxOES)( GLenum light, GLenum pname, GLfixed param );
    void       (*p_glLightxvOES)( GLenum light, GLenum pname, const GLfixed *params );
    void       (*p_glLineWidthxOES)( GLfixed width );
    void       (*p_glLinkProgram)( GLuint program );
    void       (*p_glLinkProgramARB)( GLhandleARB programObj );
    void       (*p_glListDrawCommandsStatesClientNV)( GLuint list, GLuint segment, const void **indirects, const GLsizei *sizes, const GLuint *states, const GLuint *fbos, GLuint count );
    void       (*p_glListParameterfSGIX)( GLuint list, GLenum pname, GLfloat param );
    void       (*p_glListParameterfvSGIX)( GLuint list, GLenum pname, const GLfloat *params );
    void       (*p_glListParameteriSGIX)( GLuint list, GLenum pname, GLint param );
    void       (*p_glListParameterivSGIX)( GLuint list, GLenum pname, const GLint *params );
    void       (*p_glLoadIdentityDeformationMapSGIX)( GLbitfield mask );
    void       (*p_glLoadMatrixxOES)( const GLfixed *m );
    void       (*p_glLoadProgramNV)( GLenum target, GLuint id, GLsizei len, const GLubyte *program );
    void       (*p_glLoadTransposeMatrixd)( const GLdouble *m );
    void       (*p_glLoadTransposeMatrixdARB)( const GLdouble *m );
    void       (*p_glLoadTransposeMatrixf)( const GLfloat *m );
    void       (*p_glLoadTransposeMatrixfARB)( const GLfloat *m );
    void       (*p_glLoadTransposeMatrixxOES)( const GLfixed *m );
    void       (*p_glLockArraysEXT)( GLint first, GLsizei count );
    void       (*p_glMTexCoord2fSGIS)( GLenum target, GLfloat s, GLfloat t );
    void       (*p_glMTexCoord2fvSGIS)( GLenum target, GLfloat * v );
    void       (*p_glMakeBufferNonResidentNV)( GLenum target );
    void       (*p_glMakeBufferResidentNV)( GLenum target, GLenum access );
    void       (*p_glMakeImageHandleNonResidentARB)( GLuint64 handle );
    void       (*p_glMakeImageHandleNonResidentNV)( GLuint64 handle );
    void       (*p_glMakeImageHandleResidentARB)( GLuint64 handle, GLenum access );
    void       (*p_glMakeImageHandleResidentNV)( GLuint64 handle, GLenum access );
    void       (*p_glMakeNamedBufferNonResidentNV)( GLuint buffer );
    void       (*p_glMakeNamedBufferResidentNV)( GLuint buffer, GLenum access );
    void       (*p_glMakeTextureHandleNonResidentARB)( GLuint64 handle );
    void       (*p_glMakeTextureHandleNonResidentNV)( GLuint64 handle );
    void       (*p_glMakeTextureHandleResidentARB)( GLuint64 handle );
    void       (*p_glMakeTextureHandleResidentNV)( GLuint64 handle );
    void       (*p_glMap1xOES)( GLenum target, GLfixed u1, GLfixed u2, GLint stride, GLint order, GLfixed points );
    void       (*p_glMap2xOES)( GLenum target, GLfixed u1, GLfixed u2, GLint ustride, GLint uorder, GLfixed v1, GLfixed v2, GLint vstride, GLint vorder, GLfixed points );
    void *     (*p_glMapBuffer)( GLenum target, GLenum access );
    void *     (*p_glMapBufferARB)( GLenum target, GLenum access );
    void *     (*p_glMapBufferRange)( GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access );
    void       (*p_glMapControlPointsNV)( GLenum target, GLuint index, GLenum type, GLsizei ustride, GLsizei vstride, GLint uorder, GLint vorder, GLboolean packed, const void *points );
    void       (*p_glMapGrid1xOES)( GLint n, GLfixed u1, GLfixed u2 );
    void       (*p_glMapGrid2xOES)( GLint n, GLfixed u1, GLfixed u2, GLfixed v1, GLfixed v2 );
    void *     (*p_glMapNamedBuffer)( GLuint buffer, GLenum access );
    void *     (*p_glMapNamedBufferEXT)( GLuint buffer, GLenum access );
    void *     (*p_glMapNamedBufferRange)( GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access );
    void *     (*p_glMapNamedBufferRangeEXT)( GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access );
    void *     (*p_glMapObjectBufferATI)( GLuint buffer );
    void       (*p_glMapParameterfvNV)( GLenum target, GLenum pname, const GLfloat *params );
    void       (*p_glMapParameterivNV)( GLenum target, GLenum pname, const GLint *params );
    void *     (*p_glMapTexture2DINTEL)( GLuint texture, GLint level, GLbitfield access, GLint *stride, GLenum *layout );
    void       (*p_glMapVertexAttrib1dAPPLE)( GLuint index, GLuint size, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points );
    void       (*p_glMapVertexAttrib1fAPPLE)( GLuint index, GLuint size, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points );
    void       (*p_glMapVertexAttrib2dAPPLE)( GLuint index, GLuint size, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points );
    void       (*p_glMapVertexAttrib2fAPPLE)( GLuint index, GLuint size, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points );
    void       (*p_glMaterialxOES)( GLenum face, GLenum pname, GLfixed param );
    void       (*p_glMaterialxvOES)( GLenum face, GLenum pname, const GLfixed *param );
    void       (*p_glMatrixFrustumEXT)( GLenum mode, GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar );
    void       (*p_glMatrixIndexPointerARB)( GLint size, GLenum type, GLsizei stride, const void *pointer );
    void       (*p_glMatrixIndexubvARB)( GLint size, const GLubyte *indices );
    void       (*p_glMatrixIndexuivARB)( GLint size, const GLuint *indices );
    void       (*p_glMatrixIndexusvARB)( GLint size, const GLushort *indices );
    void       (*p_glMatrixLoad3x2fNV)( GLenum matrixMode, const GLfloat *m );
    void       (*p_glMatrixLoad3x3fNV)( GLenum matrixMode, const GLfloat *m );
    void       (*p_glMatrixLoadIdentityEXT)( GLenum mode );
    void       (*p_glMatrixLoadTranspose3x3fNV)( GLenum matrixMode, const GLfloat *m );
    void       (*p_glMatrixLoadTransposedEXT)( GLenum mode, const GLdouble *m );
    void       (*p_glMatrixLoadTransposefEXT)( GLenum mode, const GLfloat *m );
    void       (*p_glMatrixLoaddEXT)( GLenum mode, const GLdouble *m );
    void       (*p_glMatrixLoadfEXT)( GLenum mode, const GLfloat *m );
    void       (*p_glMatrixMult3x2fNV)( GLenum matrixMode, const GLfloat *m );
    void       (*p_glMatrixMult3x3fNV)( GLenum matrixMode, const GLfloat *m );
    void       (*p_glMatrixMultTranspose3x3fNV)( GLenum matrixMode, const GLfloat *m );
    void       (*p_glMatrixMultTransposedEXT)( GLenum mode, const GLdouble *m );
    void       (*p_glMatrixMultTransposefEXT)( GLenum mode, const GLfloat *m );
    void       (*p_glMatrixMultdEXT)( GLenum mode, const GLdouble *m );
    void       (*p_glMatrixMultfEXT)( GLenum mode, const GLfloat *m );
    void       (*p_glMatrixOrthoEXT)( GLenum mode, GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar );
    void       (*p_glMatrixPopEXT)( GLenum mode );
    void       (*p_glMatrixPushEXT)( GLenum mode );
    void       (*p_glMatrixRotatedEXT)( GLenum mode, GLdouble angle, GLdouble x, GLdouble y, GLdouble z );
    void       (*p_glMatrixRotatefEXT)( GLenum mode, GLfloat angle, GLfloat x, GLfloat y, GLfloat z );
    void       (*p_glMatrixScaledEXT)( GLenum mode, GLdouble x, GLdouble y, GLdouble z );
    void       (*p_glMatrixScalefEXT)( GLenum mode, GLfloat x, GLfloat y, GLfloat z );
    void       (*p_glMatrixTranslatedEXT)( GLenum mode, GLdouble x, GLdouble y, GLdouble z );
    void       (*p_glMatrixTranslatefEXT)( GLenum mode, GLfloat x, GLfloat y, GLfloat z );
    void       (*p_glMaxShaderCompilerThreadsARB)( GLuint count );
    void       (*p_glMaxShaderCompilerThreadsKHR)( GLuint count );
    void       (*p_glMemoryBarrier)( GLbitfield barriers );
    void       (*p_glMemoryBarrierByRegion)( GLbitfield barriers );
    void       (*p_glMemoryBarrierEXT)( GLbitfield barriers );
    void       (*p_glMemoryObjectParameterivEXT)( GLuint memoryObject, GLenum pname, const GLint *params );
    void       (*p_glMinSampleShading)( GLfloat value );
    void       (*p_glMinSampleShadingARB)( GLfloat value );
    void       (*p_glMinmax)( GLenum target, GLenum internalformat, GLboolean sink );
    void       (*p_glMinmaxEXT)( GLenum target, GLenum internalformat, GLboolean sink );
    void       (*p_glMultMatrixxOES)( const GLfixed *m );
    void       (*p_glMultTransposeMatrixd)( const GLdouble *m );
    void       (*p_glMultTransposeMatrixdARB)( const GLdouble *m );
    void       (*p_glMultTransposeMatrixf)( const GLfloat *m );
    void       (*p_glMultTransposeMatrixfARB)( const GLfloat *m );
    void       (*p_glMultTransposeMatrixxOES)( const GLfixed *m );
    void       (*p_glMultiDrawArrays)( GLenum mode, const GLint *first, const GLsizei *count, GLsizei drawcount );
    void       (*p_glMultiDrawArraysEXT)( GLenum mode, const GLint *first, const GLsizei *count, GLsizei primcount );
    void       (*p_glMultiDrawArraysIndirect)( GLenum mode, const void *indirect, GLsizei drawcount, GLsizei stride );
    void       (*p_glMultiDrawArraysIndirectAMD)( GLenum mode, const void *indirect, GLsizei primcount, GLsizei stride );
    void       (*p_glMultiDrawArraysIndirectBindlessCountNV)( GLenum mode, const void *indirect, GLsizei drawCount, GLsizei maxDrawCount, GLsizei stride, GLint vertexBufferCount );
    void       (*p_glMultiDrawArraysIndirectBindlessNV)( GLenum mode, const void *indirect, GLsizei drawCount, GLsizei stride, GLint vertexBufferCount );
    void       (*p_glMultiDrawArraysIndirectCount)( GLenum mode, const void *indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride );
    void       (*p_glMultiDrawArraysIndirectCountARB)( GLenum mode, const void *indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride );
    void       (*p_glMultiDrawElementArrayAPPLE)( GLenum mode, const GLint *first, const GLsizei *count, GLsizei primcount );
    void       (*p_glMultiDrawElements)( GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei drawcount );
    void       (*p_glMultiDrawElementsBaseVertex)( GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei drawcount, const GLint *basevertex );
    void       (*p_glMultiDrawElementsEXT)( GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei primcount );
    void       (*p_glMultiDrawElementsIndirect)( GLenum mode, GLenum type, const void *indirect, GLsizei drawcount, GLsizei stride );
    void       (*p_glMultiDrawElementsIndirectAMD)( GLenum mode, GLenum type, const void *indirect, GLsizei primcount, GLsizei stride );
    void       (*p_glMultiDrawElementsIndirectBindlessCountNV)( GLenum mode, GLenum type, const void *indirect, GLsizei drawCount, GLsizei maxDrawCount, GLsizei stride, GLint vertexBufferCount );
    void       (*p_glMultiDrawElementsIndirectBindlessNV)( GLenum mode, GLenum type, const void *indirect, GLsizei drawCount, GLsizei stride, GLint vertexBufferCount );
    void       (*p_glMultiDrawElementsIndirectCount)( GLenum mode, GLenum type, const void *indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride );
    void       (*p_glMultiDrawElementsIndirectCountARB)( GLenum mode, GLenum type, const void *indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride );
    void       (*p_glMultiDrawMeshTasksIndirectCountNV)( GLintptr indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride );
    void       (*p_glMultiDrawMeshTasksIndirectNV)( GLintptr indirect, GLsizei drawcount, GLsizei stride );
    void       (*p_glMultiDrawRangeElementArrayAPPLE)( GLenum mode, GLuint start, GLuint end, const GLint *first, const GLsizei *count, GLsizei primcount );
    void       (*p_glMultiModeDrawArraysIBM)( const GLenum *mode, const GLint *first, const GLsizei *count, GLsizei primcount, GLint modestride );
    void       (*p_glMultiModeDrawElementsIBM)( const GLenum *mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei primcount, GLint modestride );
    void       (*p_glMultiTexBufferEXT)( GLenum texunit, GLenum target, GLenum internalformat, GLuint buffer );
    void       (*p_glMultiTexCoord1bOES)( GLenum texture, GLbyte s );
    void       (*p_glMultiTexCoord1bvOES)( GLenum texture, const GLbyte *coords );
    void       (*p_glMultiTexCoord1d)( GLenum target, GLdouble s );
    void       (*p_glMultiTexCoord1dARB)( GLenum target, GLdouble s );
    void       (*p_glMultiTexCoord1dSGIS)( GLenum target, GLdouble s );
    void       (*p_glMultiTexCoord1dv)( GLenum target, const GLdouble *v );
    void       (*p_glMultiTexCoord1dvARB)( GLenum target, const GLdouble *v );
    void       (*p_glMultiTexCoord1dvSGIS)( GLenum target, GLdouble * v );
    void       (*p_glMultiTexCoord1f)( GLenum target, GLfloat s );
    void       (*p_glMultiTexCoord1fARB)( GLenum target, GLfloat s );
    void       (*p_glMultiTexCoord1fSGIS)( GLenum target, GLfloat s );
    void       (*p_glMultiTexCoord1fv)( GLenum target, const GLfloat *v );
    void       (*p_glMultiTexCoord1fvARB)( GLenum target, const GLfloat *v );
    void       (*p_glMultiTexCoord1fvSGIS)( GLenum target, const GLfloat * v );
    void       (*p_glMultiTexCoord1hNV)( GLenum target, GLhalfNV s );
    void       (*p_glMultiTexCoord1hvNV)( GLenum target, const GLhalfNV *v );
    void       (*p_glMultiTexCoord1i)( GLenum target, GLint s );
    void       (*p_glMultiTexCoord1iARB)( GLenum target, GLint s );
    void       (*p_glMultiTexCoord1iSGIS)( GLenum target, GLint s );
    void       (*p_glMultiTexCoord1iv)( GLenum target, const GLint *v );
    void       (*p_glMultiTexCoord1ivARB)( GLenum target, const GLint *v );
    void       (*p_glMultiTexCoord1ivSGIS)( GLenum target, GLint * v );
    void       (*p_glMultiTexCoord1s)( GLenum target, GLshort s );
    void       (*p_glMultiTexCoord1sARB)( GLenum target, GLshort s );
    void       (*p_glMultiTexCoord1sSGIS)( GLenum target, GLshort s );
    void       (*p_glMultiTexCoord1sv)( GLenum target, const GLshort *v );
    void       (*p_glMultiTexCoord1svARB)( GLenum target, const GLshort *v );
    void       (*p_glMultiTexCoord1svSGIS)( GLenum target, GLshort * v );
    void       (*p_glMultiTexCoord1xOES)( GLenum texture, GLfixed s );
    void       (*p_glMultiTexCoord1xvOES)( GLenum texture, const GLfixed *coords );
    void       (*p_glMultiTexCoord2bOES)( GLenum texture, GLbyte s, GLbyte t );
    void       (*p_glMultiTexCoord2bvOES)( GLenum texture, const GLbyte *coords );
    void       (*p_glMultiTexCoord2d)( GLenum target, GLdouble s, GLdouble t );
    void       (*p_glMultiTexCoord2dARB)( GLenum target, GLdouble s, GLdouble t );
    void       (*p_glMultiTexCoord2dSGIS)( GLenum target, GLdouble s, GLdouble t );
    void       (*p_glMultiTexCoord2dv)( GLenum target, const GLdouble *v );
    void       (*p_glMultiTexCoord2dvARB)( GLenum target, const GLdouble *v );
    void       (*p_glMultiTexCoord2dvSGIS)( GLenum target, GLdouble * v );
    void       (*p_glMultiTexCoord2f)( GLenum target, GLfloat s, GLfloat t );
    void       (*p_glMultiTexCoord2fARB)( GLenum target, GLfloat s, GLfloat t );
    void       (*p_glMultiTexCoord2fSGIS)( GLenum target, GLfloat s, GLfloat t );
    void       (*p_glMultiTexCoord2fv)( GLenum target, const GLfloat *v );
    void       (*p_glMultiTexCoord2fvARB)( GLenum target, const GLfloat *v );
    void       (*p_glMultiTexCoord2fvSGIS)( GLenum target, GLfloat * v );
    void       (*p_glMultiTexCoord2hNV)( GLenum target, GLhalfNV s, GLhalfNV t );
    void       (*p_glMultiTexCoord2hvNV)( GLenum target, const GLhalfNV *v );
    void       (*p_glMultiTexCoord2i)( GLenum target, GLint s, GLint t );
    void       (*p_glMultiTexCoord2iARB)( GLenum target, GLint s, GLint t );
    void       (*p_glMultiTexCoord2iSGIS)( GLenum target, GLint s, GLint t );
    void       (*p_glMultiTexCoord2iv)( GLenum target, const GLint *v );
    void       (*p_glMultiTexCoord2ivARB)( GLenum target, const GLint *v );
    void       (*p_glMultiTexCoord2ivSGIS)( GLenum target, GLint * v );
    void       (*p_glMultiTexCoord2s)( GLenum target, GLshort s, GLshort t );
    void       (*p_glMultiTexCoord2sARB)( GLenum target, GLshort s, GLshort t );
    void       (*p_glMultiTexCoord2sSGIS)( GLenum target, GLshort s, GLshort t );
    void       (*p_glMultiTexCoord2sv)( GLenum target, const GLshort *v );
    void       (*p_glMultiTexCoord2svARB)( GLenum target, const GLshort *v );
    void       (*p_glMultiTexCoord2svSGIS)( GLenum target, GLshort * v );
    void       (*p_glMultiTexCoord2xOES)( GLenum texture, GLfixed s, GLfixed t );
    void       (*p_glMultiTexCoord2xvOES)( GLenum texture, const GLfixed *coords );
    void       (*p_glMultiTexCoord3bOES)( GLenum texture, GLbyte s, GLbyte t, GLbyte r );
    void       (*p_glMultiTexCoord3bvOES)( GLenum texture, const GLbyte *coords );
    void       (*p_glMultiTexCoord3d)( GLenum target, GLdouble s, GLdouble t, GLdouble r );
    void       (*p_glMultiTexCoord3dARB)( GLenum target, GLdouble s, GLdouble t, GLdouble r );
    void       (*p_glMultiTexCoord3dSGIS)( GLenum target, GLdouble s, GLdouble t, GLdouble r );
    void       (*p_glMultiTexCoord3dv)( GLenum target, const GLdouble *v );
    void       (*p_glMultiTexCoord3dvARB)( GLenum target, const GLdouble *v );
    void       (*p_glMultiTexCoord3dvSGIS)( GLenum target, GLdouble * v );
    void       (*p_glMultiTexCoord3f)( GLenum target, GLfloat s, GLfloat t, GLfloat r );
    void       (*p_glMultiTexCoord3fARB)( GLenum target, GLfloat s, GLfloat t, GLfloat r );
    void       (*p_glMultiTexCoord3fSGIS)( GLenum target, GLfloat s, GLfloat t, GLfloat r );
    void       (*p_glMultiTexCoord3fv)( GLenum target, const GLfloat *v );
    void       (*p_glMultiTexCoord3fvARB)( GLenum target, const GLfloat *v );
    void       (*p_glMultiTexCoord3fvSGIS)( GLenum target, GLfloat * v );
    void       (*p_glMultiTexCoord3hNV)( GLenum target, GLhalfNV s, GLhalfNV t, GLhalfNV r );
    void       (*p_glMultiTexCoord3hvNV)( GLenum target, const GLhalfNV *v );
    void       (*p_glMultiTexCoord3i)( GLenum target, GLint s, GLint t, GLint r );
    void       (*p_glMultiTexCoord3iARB)( GLenum target, GLint s, GLint t, GLint r );
    void       (*p_glMultiTexCoord3iSGIS)( GLenum target, GLint s, GLint t, GLint r );
    void       (*p_glMultiTexCoord3iv)( GLenum target, const GLint *v );
    void       (*p_glMultiTexCoord3ivARB)( GLenum target, const GLint *v );
    void       (*p_glMultiTexCoord3ivSGIS)( GLenum target, GLint * v );
    void       (*p_glMultiTexCoord3s)( GLenum target, GLshort s, GLshort t, GLshort r );
    void       (*p_glMultiTexCoord3sARB)( GLenum target, GLshort s, GLshort t, GLshort r );
    void       (*p_glMultiTexCoord3sSGIS)( GLenum target, GLshort s, GLshort t, GLshort r );
    void       (*p_glMultiTexCoord3sv)( GLenum target, const GLshort *v );
    void       (*p_glMultiTexCoord3svARB)( GLenum target, const GLshort *v );
    void       (*p_glMultiTexCoord3svSGIS)( GLenum target, GLshort * v );
    void       (*p_glMultiTexCoord3xOES)( GLenum texture, GLfixed s, GLfixed t, GLfixed r );
    void       (*p_glMultiTexCoord3xvOES)( GLenum texture, const GLfixed *coords );
    void       (*p_glMultiTexCoord4bOES)( GLenum texture, GLbyte s, GLbyte t, GLbyte r, GLbyte q );
    void       (*p_glMultiTexCoord4bvOES)( GLenum texture, const GLbyte *coords );
    void       (*p_glMultiTexCoord4d)( GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q );
    void       (*p_glMultiTexCoord4dARB)( GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q );
    void       (*p_glMultiTexCoord4dSGIS)( GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q );
    void       (*p_glMultiTexCoord4dv)( GLenum target, const GLdouble *v );
    void       (*p_glMultiTexCoord4dvARB)( GLenum target, const GLdouble *v );
    void       (*p_glMultiTexCoord4dvSGIS)( GLenum target, GLdouble * v );
    void       (*p_glMultiTexCoord4f)( GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q );
    void       (*p_glMultiTexCoord4fARB)( GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q );
    void       (*p_glMultiTexCoord4fSGIS)( GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q );
    void       (*p_glMultiTexCoord4fv)( GLenum target, const GLfloat *v );
    void       (*p_glMultiTexCoord4fvARB)( GLenum target, const GLfloat *v );
    void       (*p_glMultiTexCoord4fvSGIS)( GLenum target, GLfloat * v );
    void       (*p_glMultiTexCoord4hNV)( GLenum target, GLhalfNV s, GLhalfNV t, GLhalfNV r, GLhalfNV q );
    void       (*p_glMultiTexCoord4hvNV)( GLenum target, const GLhalfNV *v );
    void       (*p_glMultiTexCoord4i)( GLenum target, GLint s, GLint t, GLint r, GLint q );
    void       (*p_glMultiTexCoord4iARB)( GLenum target, GLint s, GLint t, GLint r, GLint q );
    void       (*p_glMultiTexCoord4iSGIS)( GLenum target, GLint s, GLint t, GLint r, GLint q );
    void       (*p_glMultiTexCoord4iv)( GLenum target, const GLint *v );
    void       (*p_glMultiTexCoord4ivARB)( GLenum target, const GLint *v );
    void       (*p_glMultiTexCoord4ivSGIS)( GLenum target, GLint * v );
    void       (*p_glMultiTexCoord4s)( GLenum target, GLshort s, GLshort t, GLshort r, GLshort q );
    void       (*p_glMultiTexCoord4sARB)( GLenum target, GLshort s, GLshort t, GLshort r, GLshort q );
    void       (*p_glMultiTexCoord4sSGIS)( GLenum target, GLshort s, GLshort t, GLshort r, GLshort q );
    void       (*p_glMultiTexCoord4sv)( GLenum target, const GLshort *v );
    void       (*p_glMultiTexCoord4svARB)( GLenum target, const GLshort *v );
    void       (*p_glMultiTexCoord4svSGIS)( GLenum target, GLshort * v );
    void       (*p_glMultiTexCoord4xOES)( GLenum texture, GLfixed s, GLfixed t, GLfixed r, GLfixed q );
    void       (*p_glMultiTexCoord4xvOES)( GLenum texture, const GLfixed *coords );
    void       (*p_glMultiTexCoordP1ui)( GLenum texture, GLenum type, GLuint coords );
    void       (*p_glMultiTexCoordP1uiv)( GLenum texture, GLenum type, const GLuint *coords );
    void       (*p_glMultiTexCoordP2ui)( GLenum texture, GLenum type, GLuint coords );
    void       (*p_glMultiTexCoordP2uiv)( GLenum texture, GLenum type, const GLuint *coords );
    void       (*p_glMultiTexCoordP3ui)( GLenum texture, GLenum type, GLuint coords );
    void       (*p_glMultiTexCoordP3uiv)( GLenum texture, GLenum type, const GLuint *coords );
    void       (*p_glMultiTexCoordP4ui)( GLenum texture, GLenum type, GLuint coords );
    void       (*p_glMultiTexCoordP4uiv)( GLenum texture, GLenum type, const GLuint *coords );
    void       (*p_glMultiTexCoordPointerEXT)( GLenum texunit, GLint size, GLenum type, GLsizei stride, const void *pointer );
    void       (*p_glMultiTexCoordPointerSGIS)( GLenum target, GLint size, GLenum type, GLsizei stride, GLvoid * pointer );
    void       (*p_glMultiTexEnvfEXT)( GLenum texunit, GLenum target, GLenum pname, GLfloat param );
    void       (*p_glMultiTexEnvfvEXT)( GLenum texunit, GLenum target, GLenum pname, const GLfloat *params );
    void       (*p_glMultiTexEnviEXT)( GLenum texunit, GLenum target, GLenum pname, GLint param );
    void       (*p_glMultiTexEnvivEXT)( GLenum texunit, GLenum target, GLenum pname, const GLint *params );
    void       (*p_glMultiTexGendEXT)( GLenum texunit, GLenum coord, GLenum pname, GLdouble param );
    void       (*p_glMultiTexGendvEXT)( GLenum texunit, GLenum coord, GLenum pname, const GLdouble *params );
    void       (*p_glMultiTexGenfEXT)( GLenum texunit, GLenum coord, GLenum pname, GLfloat param );
    void       (*p_glMultiTexGenfvEXT)( GLenum texunit, GLenum coord, GLenum pname, const GLfloat *params );
    void       (*p_glMultiTexGeniEXT)( GLenum texunit, GLenum coord, GLenum pname, GLint param );
    void       (*p_glMultiTexGenivEXT)( GLenum texunit, GLenum coord, GLenum pname, const GLint *params );
    void       (*p_glMultiTexImage1DEXT)( GLenum texunit, GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels );
    void       (*p_glMultiTexImage2DEXT)( GLenum texunit, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels );
    void       (*p_glMultiTexImage3DEXT)( GLenum texunit, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels );
    void       (*p_glMultiTexParameterIivEXT)( GLenum texunit, GLenum target, GLenum pname, const GLint *params );
    void       (*p_glMultiTexParameterIuivEXT)( GLenum texunit, GLenum target, GLenum pname, const GLuint *params );
    void       (*p_glMultiTexParameterfEXT)( GLenum texunit, GLenum target, GLenum pname, GLfloat param );
    void       (*p_glMultiTexParameterfvEXT)( GLenum texunit, GLenum target, GLenum pname, const GLfloat *params );
    void       (*p_glMultiTexParameteriEXT)( GLenum texunit, GLenum target, GLenum pname, GLint param );
    void       (*p_glMultiTexParameterivEXT)( GLenum texunit, GLenum target, GLenum pname, const GLint *params );
    void       (*p_glMultiTexRenderbufferEXT)( GLenum texunit, GLenum target, GLuint renderbuffer );
    void       (*p_glMultiTexSubImage1DEXT)( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels );
    void       (*p_glMultiTexSubImage2DEXT)( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels );
    void       (*p_glMultiTexSubImage3DEXT)( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels );
    void       (*p_glMulticastBarrierNV)(void);
    void       (*p_glMulticastBlitFramebufferNV)( GLuint srcGpu, GLuint dstGpu, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter );
    void       (*p_glMulticastBufferSubDataNV)( GLbitfield gpuMask, GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data );
    void       (*p_glMulticastCopyBufferSubDataNV)( GLuint readGpu, GLbitfield writeGpuMask, GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size );
    void       (*p_glMulticastCopyImageSubDataNV)( GLuint srcGpu, GLbitfield dstGpuMask, GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth );
    void       (*p_glMulticastFramebufferSampleLocationsfvNV)( GLuint gpu, GLuint framebuffer, GLuint start, GLsizei count, const GLfloat *v );
    void       (*p_glMulticastGetQueryObjecti64vNV)( GLuint gpu, GLuint id, GLenum pname, GLint64 *params );
    void       (*p_glMulticastGetQueryObjectivNV)( GLuint gpu, GLuint id, GLenum pname, GLint *params );
    void       (*p_glMulticastGetQueryObjectui64vNV)( GLuint gpu, GLuint id, GLenum pname, GLuint64 *params );
    void       (*p_glMulticastGetQueryObjectuivNV)( GLuint gpu, GLuint id, GLenum pname, GLuint *params );
    void       (*p_glMulticastScissorArrayvNVX)( GLuint gpu, GLuint first, GLsizei count, const GLint *v );
    void       (*p_glMulticastViewportArrayvNVX)( GLuint gpu, GLuint first, GLsizei count, const GLfloat *v );
    void       (*p_glMulticastViewportPositionWScaleNVX)( GLuint gpu, GLuint index, GLfloat xcoeff, GLfloat ycoeff );
    void       (*p_glMulticastWaitSyncNV)( GLuint signalGpu, GLbitfield waitGpuMask );
    void       (*p_glNamedBufferAttachMemoryNV)( GLuint buffer, GLuint memory, GLuint64 offset );
    void       (*p_glNamedBufferData)( GLuint buffer, GLsizeiptr size, const void *data, GLenum usage );
    void       (*p_glNamedBufferDataEXT)( GLuint buffer, GLsizeiptr size, const void *data, GLenum usage );
    void       (*p_glNamedBufferPageCommitmentARB)( GLuint buffer, GLintptr offset, GLsizeiptr size, GLboolean commit );
    void       (*p_glNamedBufferPageCommitmentEXT)( GLuint buffer, GLintptr offset, GLsizeiptr size, GLboolean commit );
    void       (*p_glNamedBufferStorage)( GLuint buffer, GLsizeiptr size, const void *data, GLbitfield flags );
    void       (*p_glNamedBufferStorageEXT)( GLuint buffer, GLsizeiptr size, const void *data, GLbitfield flags );
    void       (*p_glNamedBufferStorageExternalEXT)( GLuint buffer, GLintptr offset, GLsizeiptr size, GLeglClientBufferEXT clientBuffer, GLbitfield flags );
    void       (*p_glNamedBufferStorageMemEXT)( GLuint buffer, GLsizeiptr size, GLuint memory, GLuint64 offset );
    void       (*p_glNamedBufferSubData)( GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data );
    void       (*p_glNamedBufferSubDataEXT)( GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data );
    void       (*p_glNamedCopyBufferSubDataEXT)( GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size );
    void       (*p_glNamedFramebufferDrawBuffer)( GLuint framebuffer, GLenum buf );
    void       (*p_glNamedFramebufferDrawBuffers)( GLuint framebuffer, GLsizei n, const GLenum *bufs );
    void       (*p_glNamedFramebufferParameteri)( GLuint framebuffer, GLenum pname, GLint param );
    void       (*p_glNamedFramebufferParameteriEXT)( GLuint framebuffer, GLenum pname, GLint param );
    void       (*p_glNamedFramebufferReadBuffer)( GLuint framebuffer, GLenum src );
    void       (*p_glNamedFramebufferRenderbuffer)( GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer );
    void       (*p_glNamedFramebufferRenderbufferEXT)( GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer );
    void       (*p_glNamedFramebufferSampleLocationsfvARB)( GLuint framebuffer, GLuint start, GLsizei count, const GLfloat *v );
    void       (*p_glNamedFramebufferSampleLocationsfvNV)( GLuint framebuffer, GLuint start, GLsizei count, const GLfloat *v );
    void       (*p_glNamedFramebufferSamplePositionsfvAMD)( GLuint framebuffer, GLuint numsamples, GLuint pixelindex, const GLfloat *values );
    void       (*p_glNamedFramebufferTexture)( GLuint framebuffer, GLenum attachment, GLuint texture, GLint level );
    void       (*p_glNamedFramebufferTexture1DEXT)( GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level );
    void       (*p_glNamedFramebufferTexture2DEXT)( GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level );
    void       (*p_glNamedFramebufferTexture3DEXT)( GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset );
    void       (*p_glNamedFramebufferTextureEXT)( GLuint framebuffer, GLenum attachment, GLuint texture, GLint level );
    void       (*p_glNamedFramebufferTextureFaceEXT)( GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLenum face );
    void       (*p_glNamedFramebufferTextureLayer)( GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer );
    void       (*p_glNamedFramebufferTextureLayerEXT)( GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer );
    void       (*p_glNamedProgramLocalParameter4dEXT)( GLuint program, GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
    void       (*p_glNamedProgramLocalParameter4dvEXT)( GLuint program, GLenum target, GLuint index, const GLdouble *params );
    void       (*p_glNamedProgramLocalParameter4fEXT)( GLuint program, GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
    void       (*p_glNamedProgramLocalParameter4fvEXT)( GLuint program, GLenum target, GLuint index, const GLfloat *params );
    void       (*p_glNamedProgramLocalParameterI4iEXT)( GLuint program, GLenum target, GLuint index, GLint x, GLint y, GLint z, GLint w );
    void       (*p_glNamedProgramLocalParameterI4ivEXT)( GLuint program, GLenum target, GLuint index, const GLint *params );
    void       (*p_glNamedProgramLocalParameterI4uiEXT)( GLuint program, GLenum target, GLuint index, GLuint x, GLuint y, GLuint z, GLuint w );
    void       (*p_glNamedProgramLocalParameterI4uivEXT)( GLuint program, GLenum target, GLuint index, const GLuint *params );
    void       (*p_glNamedProgramLocalParameters4fvEXT)( GLuint program, GLenum target, GLuint index, GLsizei count, const GLfloat *params );
    void       (*p_glNamedProgramLocalParametersI4ivEXT)( GLuint program, GLenum target, GLuint index, GLsizei count, const GLint *params );
    void       (*p_glNamedProgramLocalParametersI4uivEXT)( GLuint program, GLenum target, GLuint index, GLsizei count, const GLuint *params );
    void       (*p_glNamedProgramStringEXT)( GLuint program, GLenum target, GLenum format, GLsizei len, const void *string );
    void       (*p_glNamedRenderbufferStorage)( GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height );
    void       (*p_glNamedRenderbufferStorageEXT)( GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height );
    void       (*p_glNamedRenderbufferStorageMultisample)( GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height );
    void       (*p_glNamedRenderbufferStorageMultisampleAdvancedAMD)( GLuint renderbuffer, GLsizei samples, GLsizei storageSamples, GLenum internalformat, GLsizei width, GLsizei height );
    void       (*p_glNamedRenderbufferStorageMultisampleCoverageEXT)( GLuint renderbuffer, GLsizei coverageSamples, GLsizei colorSamples, GLenum internalformat, GLsizei width, GLsizei height );
    void       (*p_glNamedRenderbufferStorageMultisampleEXT)( GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height );
    void       (*p_glNamedStringARB)( GLenum type, GLint namelen, const GLchar *name, GLint stringlen, const GLchar *string );
    GLuint     (*p_glNewBufferRegion)( GLenum type );
    GLuint     (*p_glNewObjectBufferATI)( GLsizei size, const void *pointer, GLenum usage );
    void       (*p_glNormal3fVertex3fSUN)( GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z );
    void       (*p_glNormal3fVertex3fvSUN)( const GLfloat *n, const GLfloat *v );
    void       (*p_glNormal3hNV)( GLhalfNV nx, GLhalfNV ny, GLhalfNV nz );
    void       (*p_glNormal3hvNV)( const GLhalfNV *v );
    void       (*p_glNormal3xOES)( GLfixed nx, GLfixed ny, GLfixed nz );
    void       (*p_glNormal3xvOES)( const GLfixed *coords );
    void       (*p_glNormalFormatNV)( GLenum type, GLsizei stride );
    void       (*p_glNormalP3ui)( GLenum type, GLuint coords );
    void       (*p_glNormalP3uiv)( GLenum type, const GLuint *coords );
    void       (*p_glNormalPointerEXT)( GLenum type, GLsizei stride, GLsizei count, const void *pointer );
    void       (*p_glNormalPointerListIBM)( GLenum type, GLint stride, const void **pointer, GLint ptrstride );
    void       (*p_glNormalPointervINTEL)( GLenum type, const void **pointer );
    void       (*p_glNormalStream3bATI)( GLenum stream, GLbyte nx, GLbyte ny, GLbyte nz );
    void       (*p_glNormalStream3bvATI)( GLenum stream, const GLbyte *coords );
    void       (*p_glNormalStream3dATI)( GLenum stream, GLdouble nx, GLdouble ny, GLdouble nz );
    void       (*p_glNormalStream3dvATI)( GLenum stream, const GLdouble *coords );
    void       (*p_glNormalStream3fATI)( GLenum stream, GLfloat nx, GLfloat ny, GLfloat nz );
    void       (*p_glNormalStream3fvATI)( GLenum stream, const GLfloat *coords );
    void       (*p_glNormalStream3iATI)( GLenum stream, GLint nx, GLint ny, GLint nz );
    void       (*p_glNormalStream3ivATI)( GLenum stream, const GLint *coords );
    void       (*p_glNormalStream3sATI)( GLenum stream, GLshort nx, GLshort ny, GLshort nz );
    void       (*p_glNormalStream3svATI)( GLenum stream, const GLshort *coords );
    void       (*p_glObjectLabel)( GLenum identifier, GLuint name, GLsizei length, const GLchar *label );
    void       (*p_glObjectPtrLabel)( const void *ptr, GLsizei length, const GLchar *label );
    GLenum     (*p_glObjectPurgeableAPPLE)( GLenum objectType, GLuint name, GLenum option );
    GLenum     (*p_glObjectUnpurgeableAPPLE)( GLenum objectType, GLuint name, GLenum option );
    void       (*p_glOrthofOES)( GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f );
    void       (*p_glOrthoxOES)( GLfixed l, GLfixed r, GLfixed b, GLfixed t, GLfixed n, GLfixed f );
    void       (*p_glPNTrianglesfATI)( GLenum pname, GLfloat param );
    void       (*p_glPNTrianglesiATI)( GLenum pname, GLint param );
    void       (*p_glPassTexCoordATI)( GLuint dst, GLuint coord, GLenum swizzle );
    void       (*p_glPassThroughxOES)( GLfixed token );
    void       (*p_glPatchParameterfv)( GLenum pname, const GLfloat *values );
    void       (*p_glPatchParameteri)( GLenum pname, GLint value );
    void       (*p_glPathColorGenNV)( GLenum color, GLenum genMode, GLenum colorFormat, const GLfloat *coeffs );
    void       (*p_glPathCommandsNV)( GLuint path, GLsizei numCommands, const GLubyte *commands, GLsizei numCoords, GLenum coordType, const void *coords );
    void       (*p_glPathCoordsNV)( GLuint path, GLsizei numCoords, GLenum coordType, const void *coords );
    void       (*p_glPathCoverDepthFuncNV)( GLenum func );
    void       (*p_glPathDashArrayNV)( GLuint path, GLsizei dashCount, const GLfloat *dashArray );
    void       (*p_glPathFogGenNV)( GLenum genMode );
    GLenum     (*p_glPathGlyphIndexArrayNV)( GLuint firstPathName, GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLuint firstGlyphIndex, GLsizei numGlyphs, GLuint pathParameterTemplate, GLfloat emScale );
    GLenum     (*p_glPathGlyphIndexRangeNV)( GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLuint pathParameterTemplate, GLfloat emScale, GLuint baseAndCount[2] );
    void       (*p_glPathGlyphRangeNV)( GLuint firstPathName, GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLuint firstGlyph, GLsizei numGlyphs, GLenum handleMissingGlyphs, GLuint pathParameterTemplate, GLfloat emScale );
    void       (*p_glPathGlyphsNV)( GLuint firstPathName, GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLsizei numGlyphs, GLenum type, const void *charcodes, GLenum handleMissingGlyphs, GLuint pathParameterTemplate, GLfloat emScale );
    GLenum     (*p_glPathMemoryGlyphIndexArrayNV)( GLuint firstPathName, GLenum fontTarget, GLsizeiptr fontSize, const void *fontData, GLsizei faceIndex, GLuint firstGlyphIndex, GLsizei numGlyphs, GLuint pathParameterTemplate, GLfloat emScale );
    void       (*p_glPathParameterfNV)( GLuint path, GLenum pname, GLfloat value );
    void       (*p_glPathParameterfvNV)( GLuint path, GLenum pname, const GLfloat *value );
    void       (*p_glPathParameteriNV)( GLuint path, GLenum pname, GLint value );
    void       (*p_glPathParameterivNV)( GLuint path, GLenum pname, const GLint *value );
    void       (*p_glPathStencilDepthOffsetNV)( GLfloat factor, GLfloat units );
    void       (*p_glPathStencilFuncNV)( GLenum func, GLint ref, GLuint mask );
    void       (*p_glPathStringNV)( GLuint path, GLenum format, GLsizei length, const void *pathString );
    void       (*p_glPathSubCommandsNV)( GLuint path, GLsizei commandStart, GLsizei commandsToDelete, GLsizei numCommands, const GLubyte *commands, GLsizei numCoords, GLenum coordType, const void *coords );
    void       (*p_glPathSubCoordsNV)( GLuint path, GLsizei coordStart, GLsizei numCoords, GLenum coordType, const void *coords );
    void       (*p_glPathTexGenNV)( GLenum texCoordSet, GLenum genMode, GLint components, const GLfloat *coeffs );
    void       (*p_glPauseTransformFeedback)(void);
    void       (*p_glPauseTransformFeedbackNV)(void);
    void       (*p_glPixelDataRangeNV)( GLenum target, GLsizei length, const void *pointer );
    void       (*p_glPixelMapx)( GLenum map, GLint size, const GLfixed *values );
    void       (*p_glPixelStorex)( GLenum pname, GLfixed param );
    void       (*p_glPixelTexGenParameterfSGIS)( GLenum pname, GLfloat param );
    void       (*p_glPixelTexGenParameterfvSGIS)( GLenum pname, const GLfloat *params );
    void       (*p_glPixelTexGenParameteriSGIS)( GLenum pname, GLint param );
    void       (*p_glPixelTexGenParameterivSGIS)( GLenum pname, const GLint *params );
    void       (*p_glPixelTexGenSGIX)( GLenum mode );
    void       (*p_glPixelTransferxOES)( GLenum pname, GLfixed param );
    void       (*p_glPixelTransformParameterfEXT)( GLenum target, GLenum pname, GLfloat param );
    void       (*p_glPixelTransformParameterfvEXT)( GLenum target, GLenum pname, const GLfloat *params );
    void       (*p_glPixelTransformParameteriEXT)( GLenum target, GLenum pname, GLint param );
    void       (*p_glPixelTransformParameterivEXT)( GLenum target, GLenum pname, const GLint *params );
    void       (*p_glPixelZoomxOES)( GLfixed xfactor, GLfixed yfactor );
    GLboolean  (*p_glPointAlongPathNV)( GLuint path, GLsizei startSegment, GLsizei numSegments, GLfloat distance, GLfloat *x, GLfloat *y, GLfloat *tangentX, GLfloat *tangentY );
    void       (*p_glPointParameterf)( GLenum pname, GLfloat param );
    void       (*p_glPointParameterfARB)( GLenum pname, GLfloat param );
    void       (*p_glPointParameterfEXT)( GLenum pname, GLfloat param );
    void       (*p_glPointParameterfSGIS)( GLenum pname, GLfloat param );
    void       (*p_glPointParameterfv)( GLenum pname, const GLfloat *params );
    void       (*p_glPointParameterfvARB)( GLenum pname, const GLfloat *params );
    void       (*p_glPointParameterfvEXT)( GLenum pname, const GLfloat *params );
    void       (*p_glPointParameterfvSGIS)( GLenum pname, const GLfloat *params );
    void       (*p_glPointParameteri)( GLenum pname, GLint param );
    void       (*p_glPointParameteriNV)( GLenum pname, GLint param );
    void       (*p_glPointParameteriv)( GLenum pname, const GLint *params );
    void       (*p_glPointParameterivNV)( GLenum pname, const GLint *params );
    void       (*p_glPointParameterxvOES)( GLenum pname, const GLfixed *params );
    void       (*p_glPointSizexOES)( GLfixed size );
    GLint      (*p_glPollAsyncSGIX)( GLuint *markerp );
    GLint      (*p_glPollInstrumentsSGIX)( GLint *marker_p );
    void       (*p_glPolygonOffsetClamp)( GLfloat factor, GLfloat units, GLfloat clamp );
    void       (*p_glPolygonOffsetClampEXT)( GLfloat factor, GLfloat units, GLfloat clamp );
    void       (*p_glPolygonOffsetEXT)( GLfloat factor, GLfloat bias );
    void       (*p_glPolygonOffsetxOES)( GLfixed factor, GLfixed units );
    void       (*p_glPopDebugGroup)(void);
    void       (*p_glPopGroupMarkerEXT)(void);
    void       (*p_glPresentFrameDualFillNV)( GLuint video_slot, GLuint64EXT minPresentTime, GLuint beginPresentTimeId, GLuint presentDurationId, GLenum type, GLenum target0, GLuint fill0, GLenum target1, GLuint fill1, GLenum target2, GLuint fill2, GLenum target3, GLuint fill3 );
    void       (*p_glPresentFrameKeyedNV)( GLuint video_slot, GLuint64EXT minPresentTime, GLuint beginPresentTimeId, GLuint presentDurationId, GLenum type, GLenum target0, GLuint fill0, GLuint key0, GLenum target1, GLuint fill1, GLuint key1 );
    void       (*p_glPrimitiveBoundingBoxARB)( GLfloat minX, GLfloat minY, GLfloat minZ, GLfloat minW, GLfloat maxX, GLfloat maxY, GLfloat maxZ, GLfloat maxW );
    void       (*p_glPrimitiveRestartIndex)( GLuint index );
    void       (*p_glPrimitiveRestartIndexNV)( GLuint index );
    void       (*p_glPrimitiveRestartNV)(void);
    void       (*p_glPrioritizeTexturesEXT)( GLsizei n, const GLuint *textures, const GLclampf *priorities );
    void       (*p_glPrioritizeTexturesxOES)( GLsizei n, const GLuint *textures, const GLfixed *priorities );
    void       (*p_glProgramBinary)( GLuint program, GLenum binaryFormat, const void *binary, GLsizei length );
    void       (*p_glProgramBufferParametersIivNV)( GLenum target, GLuint bindingIndex, GLuint wordIndex, GLsizei count, const GLint *params );
    void       (*p_glProgramBufferParametersIuivNV)( GLenum target, GLuint bindingIndex, GLuint wordIndex, GLsizei count, const GLuint *params );
    void       (*p_glProgramBufferParametersfvNV)( GLenum target, GLuint bindingIndex, GLuint wordIndex, GLsizei count, const GLfloat *params );
    void       (*p_glProgramEnvParameter4dARB)( GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
    void       (*p_glProgramEnvParameter4dvARB)( GLenum target, GLuint index, const GLdouble *params );
    void       (*p_glProgramEnvParameter4fARB)( GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
    void       (*p_glProgramEnvParameter4fvARB)( GLenum target, GLuint index, const GLfloat *params );
    void       (*p_glProgramEnvParameterI4iNV)( GLenum target, GLuint index, GLint x, GLint y, GLint z, GLint w );
    void       (*p_glProgramEnvParameterI4ivNV)( GLenum target, GLuint index, const GLint *params );
    void       (*p_glProgramEnvParameterI4uiNV)( GLenum target, GLuint index, GLuint x, GLuint y, GLuint z, GLuint w );
    void       (*p_glProgramEnvParameterI4uivNV)( GLenum target, GLuint index, const GLuint *params );
    void       (*p_glProgramEnvParameters4fvEXT)( GLenum target, GLuint index, GLsizei count, const GLfloat *params );
    void       (*p_glProgramEnvParametersI4ivNV)( GLenum target, GLuint index, GLsizei count, const GLint *params );
    void       (*p_glProgramEnvParametersI4uivNV)( GLenum target, GLuint index, GLsizei count, const GLuint *params );
    void       (*p_glProgramLocalParameter4dARB)( GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
    void       (*p_glProgramLocalParameter4dvARB)( GLenum target, GLuint index, const GLdouble *params );
    void       (*p_glProgramLocalParameter4fARB)( GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
    void       (*p_glProgramLocalParameter4fvARB)( GLenum target, GLuint index, const GLfloat *params );
    void       (*p_glProgramLocalParameterI4iNV)( GLenum target, GLuint index, GLint x, GLint y, GLint z, GLint w );
    void       (*p_glProgramLocalParameterI4ivNV)( GLenum target, GLuint index, const GLint *params );
    void       (*p_glProgramLocalParameterI4uiNV)( GLenum target, GLuint index, GLuint x, GLuint y, GLuint z, GLuint w );
    void       (*p_glProgramLocalParameterI4uivNV)( GLenum target, GLuint index, const GLuint *params );
    void       (*p_glProgramLocalParameters4fvEXT)( GLenum target, GLuint index, GLsizei count, const GLfloat *params );
    void       (*p_glProgramLocalParametersI4ivNV)( GLenum target, GLuint index, GLsizei count, const GLint *params );
    void       (*p_glProgramLocalParametersI4uivNV)( GLenum target, GLuint index, GLsizei count, const GLuint *params );
    void       (*p_glProgramNamedParameter4dNV)( GLuint id, GLsizei len, const GLubyte *name, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
    void       (*p_glProgramNamedParameter4dvNV)( GLuint id, GLsizei len, const GLubyte *name, const GLdouble *v );
    void       (*p_glProgramNamedParameter4fNV)( GLuint id, GLsizei len, const GLubyte *name, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
    void       (*p_glProgramNamedParameter4fvNV)( GLuint id, GLsizei len, const GLubyte *name, const GLfloat *v );
    void       (*p_glProgramParameter4dNV)( GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
    void       (*p_glProgramParameter4dvNV)( GLenum target, GLuint index, const GLdouble *v );
    void       (*p_glProgramParameter4fNV)( GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
    void       (*p_glProgramParameter4fvNV)( GLenum target, GLuint index, const GLfloat *v );
    void       (*p_glProgramParameteri)( GLuint program, GLenum pname, GLint value );
    void       (*p_glProgramParameteriARB)( GLuint program, GLenum pname, GLint value );
    void       (*p_glProgramParameteriEXT)( GLuint program, GLenum pname, GLint value );
    void       (*p_glProgramParameters4dvNV)( GLenum target, GLuint index, GLsizei count, const GLdouble *v );
    void       (*p_glProgramParameters4fvNV)( GLenum target, GLuint index, GLsizei count, const GLfloat *v );
    void       (*p_glProgramPathFragmentInputGenNV)( GLuint program, GLint location, GLenum genMode, GLint components, const GLfloat *coeffs );
    void       (*p_glProgramStringARB)( GLenum target, GLenum format, GLsizei len, const void *string );
    void       (*p_glProgramSubroutineParametersuivNV)( GLenum target, GLsizei count, const GLuint *params );
    void       (*p_glProgramUniform1d)( GLuint program, GLint location, GLdouble v0 );
    void       (*p_glProgramUniform1dEXT)( GLuint program, GLint location, GLdouble x );
    void       (*p_glProgramUniform1dv)( GLuint program, GLint location, GLsizei count, const GLdouble *value );
    void       (*p_glProgramUniform1dvEXT)( GLuint program, GLint location, GLsizei count, const GLdouble *value );
    void       (*p_glProgramUniform1f)( GLuint program, GLint location, GLfloat v0 );
    void       (*p_glProgramUniform1fEXT)( GLuint program, GLint location, GLfloat v0 );
    void       (*p_glProgramUniform1fv)( GLuint program, GLint location, GLsizei count, const GLfloat *value );
    void       (*p_glProgramUniform1fvEXT)( GLuint program, GLint location, GLsizei count, const GLfloat *value );
    void       (*p_glProgramUniform1i)( GLuint program, GLint location, GLint v0 );
    void       (*p_glProgramUniform1i64ARB)( GLuint program, GLint location, GLint64 x );
    void       (*p_glProgramUniform1i64NV)( GLuint program, GLint location, GLint64EXT x );
    void       (*p_glProgramUniform1i64vARB)( GLuint program, GLint location, GLsizei count, const GLint64 *value );
    void       (*p_glProgramUniform1i64vNV)( GLuint program, GLint location, GLsizei count, const GLint64EXT *value );
    void       (*p_glProgramUniform1iEXT)( GLuint program, GLint location, GLint v0 );
    void       (*p_glProgramUniform1iv)( GLuint program, GLint location, GLsizei count, const GLint *value );
    void       (*p_glProgramUniform1ivEXT)( GLuint program, GLint location, GLsizei count, const GLint *value );
    void       (*p_glProgramUniform1ui)( GLuint program, GLint location, GLuint v0 );
    void       (*p_glProgramUniform1ui64ARB)( GLuint program, GLint location, GLuint64 x );
    void       (*p_glProgramUniform1ui64NV)( GLuint program, GLint location, GLuint64EXT x );
    void       (*p_glProgramUniform1ui64vARB)( GLuint program, GLint location, GLsizei count, const GLuint64 *value );
    void       (*p_glProgramUniform1ui64vNV)( GLuint program, GLint location, GLsizei count, const GLuint64EXT *value );
    void       (*p_glProgramUniform1uiEXT)( GLuint program, GLint location, GLuint v0 );
    void       (*p_glProgramUniform1uiv)( GLuint program, GLint location, GLsizei count, const GLuint *value );
    void       (*p_glProgramUniform1uivEXT)( GLuint program, GLint location, GLsizei count, const GLuint *value );
    void       (*p_glProgramUniform2d)( GLuint program, GLint location, GLdouble v0, GLdouble v1 );
    void       (*p_glProgramUniform2dEXT)( GLuint program, GLint location, GLdouble x, GLdouble y );
    void       (*p_glProgramUniform2dv)( GLuint program, GLint location, GLsizei count, const GLdouble *value );
    void       (*p_glProgramUniform2dvEXT)( GLuint program, GLint location, GLsizei count, const GLdouble *value );
    void       (*p_glProgramUniform2f)( GLuint program, GLint location, GLfloat v0, GLfloat v1 );
    void       (*p_glProgramUniform2fEXT)( GLuint program, GLint location, GLfloat v0, GLfloat v1 );
    void       (*p_glProgramUniform2fv)( GLuint program, GLint location, GLsizei count, const GLfloat *value );
    void       (*p_glProgramUniform2fvEXT)( GLuint program, GLint location, GLsizei count, const GLfloat *value );
    void       (*p_glProgramUniform2i)( GLuint program, GLint location, GLint v0, GLint v1 );
    void       (*p_glProgramUniform2i64ARB)( GLuint program, GLint location, GLint64 x, GLint64 y );
    void       (*p_glProgramUniform2i64NV)( GLuint program, GLint location, GLint64EXT x, GLint64EXT y );
    void       (*p_glProgramUniform2i64vARB)( GLuint program, GLint location, GLsizei count, const GLint64 *value );
    void       (*p_glProgramUniform2i64vNV)( GLuint program, GLint location, GLsizei count, const GLint64EXT *value );
    void       (*p_glProgramUniform2iEXT)( GLuint program, GLint location, GLint v0, GLint v1 );
    void       (*p_glProgramUniform2iv)( GLuint program, GLint location, GLsizei count, const GLint *value );
    void       (*p_glProgramUniform2ivEXT)( GLuint program, GLint location, GLsizei count, const GLint *value );
    void       (*p_glProgramUniform2ui)( GLuint program, GLint location, GLuint v0, GLuint v1 );
    void       (*p_glProgramUniform2ui64ARB)( GLuint program, GLint location, GLuint64 x, GLuint64 y );
    void       (*p_glProgramUniform2ui64NV)( GLuint program, GLint location, GLuint64EXT x, GLuint64EXT y );
    void       (*p_glProgramUniform2ui64vARB)( GLuint program, GLint location, GLsizei count, const GLuint64 *value );
    void       (*p_glProgramUniform2ui64vNV)( GLuint program, GLint location, GLsizei count, const GLuint64EXT *value );
    void       (*p_glProgramUniform2uiEXT)( GLuint program, GLint location, GLuint v0, GLuint v1 );
    void       (*p_glProgramUniform2uiv)( GLuint program, GLint location, GLsizei count, const GLuint *value );
    void       (*p_glProgramUniform2uivEXT)( GLuint program, GLint location, GLsizei count, const GLuint *value );
    void       (*p_glProgramUniform3d)( GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2 );
    void       (*p_glProgramUniform3dEXT)( GLuint program, GLint location, GLdouble x, GLdouble y, GLdouble z );
    void       (*p_glProgramUniform3dv)( GLuint program, GLint location, GLsizei count, const GLdouble *value );
    void       (*p_glProgramUniform3dvEXT)( GLuint program, GLint location, GLsizei count, const GLdouble *value );
    void       (*p_glProgramUniform3f)( GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2 );
    void       (*p_glProgramUniform3fEXT)( GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2 );
    void       (*p_glProgramUniform3fv)( GLuint program, GLint location, GLsizei count, const GLfloat *value );
    void       (*p_glProgramUniform3fvEXT)( GLuint program, GLint location, GLsizei count, const GLfloat *value );
    void       (*p_glProgramUniform3i)( GLuint program, GLint location, GLint v0, GLint v1, GLint v2 );
    void       (*p_glProgramUniform3i64ARB)( GLuint program, GLint location, GLint64 x, GLint64 y, GLint64 z );
    void       (*p_glProgramUniform3i64NV)( GLuint program, GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z );
    void       (*p_glProgramUniform3i64vARB)( GLuint program, GLint location, GLsizei count, const GLint64 *value );
    void       (*p_glProgramUniform3i64vNV)( GLuint program, GLint location, GLsizei count, const GLint64EXT *value );
    void       (*p_glProgramUniform3iEXT)( GLuint program, GLint location, GLint v0, GLint v1, GLint v2 );
    void       (*p_glProgramUniform3iv)( GLuint program, GLint location, GLsizei count, const GLint *value );
    void       (*p_glProgramUniform3ivEXT)( GLuint program, GLint location, GLsizei count, const GLint *value );
    void       (*p_glProgramUniform3ui)( GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2 );
    void       (*p_glProgramUniform3ui64ARB)( GLuint program, GLint location, GLuint64 x, GLuint64 y, GLuint64 z );
    void       (*p_glProgramUniform3ui64NV)( GLuint program, GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z );
    void       (*p_glProgramUniform3ui64vARB)( GLuint program, GLint location, GLsizei count, const GLuint64 *value );
    void       (*p_glProgramUniform3ui64vNV)( GLuint program, GLint location, GLsizei count, const GLuint64EXT *value );
    void       (*p_glProgramUniform3uiEXT)( GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2 );
    void       (*p_glProgramUniform3uiv)( GLuint program, GLint location, GLsizei count, const GLuint *value );
    void       (*p_glProgramUniform3uivEXT)( GLuint program, GLint location, GLsizei count, const GLuint *value );
    void       (*p_glProgramUniform4d)( GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2, GLdouble v3 );
    void       (*p_glProgramUniform4dEXT)( GLuint program, GLint location, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
    void       (*p_glProgramUniform4dv)( GLuint program, GLint location, GLsizei count, const GLdouble *value );
    void       (*p_glProgramUniform4dvEXT)( GLuint program, GLint location, GLsizei count, const GLdouble *value );
    void       (*p_glProgramUniform4f)( GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 );
    void       (*p_glProgramUniform4fEXT)( GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 );
    void       (*p_glProgramUniform4fv)( GLuint program, GLint location, GLsizei count, const GLfloat *value );
    void       (*p_glProgramUniform4fvEXT)( GLuint program, GLint location, GLsizei count, const GLfloat *value );
    void       (*p_glProgramUniform4i)( GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3 );
    void       (*p_glProgramUniform4i64ARB)( GLuint program, GLint location, GLint64 x, GLint64 y, GLint64 z, GLint64 w );
    void       (*p_glProgramUniform4i64NV)( GLuint program, GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z, GLint64EXT w );
    void       (*p_glProgramUniform4i64vARB)( GLuint program, GLint location, GLsizei count, const GLint64 *value );
    void       (*p_glProgramUniform4i64vNV)( GLuint program, GLint location, GLsizei count, const GLint64EXT *value );
    void       (*p_glProgramUniform4iEXT)( GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3 );
    void       (*p_glProgramUniform4iv)( GLuint program, GLint location, GLsizei count, const GLint *value );
    void       (*p_glProgramUniform4ivEXT)( GLuint program, GLint location, GLsizei count, const GLint *value );
    void       (*p_glProgramUniform4ui)( GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3 );
    void       (*p_glProgramUniform4ui64ARB)( GLuint program, GLint location, GLuint64 x, GLuint64 y, GLuint64 z, GLuint64 w );
    void       (*p_glProgramUniform4ui64NV)( GLuint program, GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z, GLuint64EXT w );
    void       (*p_glProgramUniform4ui64vARB)( GLuint program, GLint location, GLsizei count, const GLuint64 *value );
    void       (*p_glProgramUniform4ui64vNV)( GLuint program, GLint location, GLsizei count, const GLuint64EXT *value );
    void       (*p_glProgramUniform4uiEXT)( GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3 );
    void       (*p_glProgramUniform4uiv)( GLuint program, GLint location, GLsizei count, const GLuint *value );
    void       (*p_glProgramUniform4uivEXT)( GLuint program, GLint location, GLsizei count, const GLuint *value );
    void       (*p_glProgramUniformHandleui64ARB)( GLuint program, GLint location, GLuint64 value );
    void       (*p_glProgramUniformHandleui64NV)( GLuint program, GLint location, GLuint64 value );
    void       (*p_glProgramUniformHandleui64vARB)( GLuint program, GLint location, GLsizei count, const GLuint64 *values );
    void       (*p_glProgramUniformHandleui64vNV)( GLuint program, GLint location, GLsizei count, const GLuint64 *values );
    void       (*p_glProgramUniformMatrix2dv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
    void       (*p_glProgramUniformMatrix2dvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
    void       (*p_glProgramUniformMatrix2fv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
    void       (*p_glProgramUniformMatrix2fvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
    void       (*p_glProgramUniformMatrix2x3dv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
    void       (*p_glProgramUniformMatrix2x3dvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
    void       (*p_glProgramUniformMatrix2x3fv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
    void       (*p_glProgramUniformMatrix2x3fvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
    void       (*p_glProgramUniformMatrix2x4dv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
    void       (*p_glProgramUniformMatrix2x4dvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
    void       (*p_glProgramUniformMatrix2x4fv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
    void       (*p_glProgramUniformMatrix2x4fvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
    void       (*p_glProgramUniformMatrix3dv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
    void       (*p_glProgramUniformMatrix3dvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
    void       (*p_glProgramUniformMatrix3fv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
    void       (*p_glProgramUniformMatrix3fvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
    void       (*p_glProgramUniformMatrix3x2dv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
    void       (*p_glProgramUniformMatrix3x2dvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
    void       (*p_glProgramUniformMatrix3x2fv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
    void       (*p_glProgramUniformMatrix3x2fvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
    void       (*p_glProgramUniformMatrix3x4dv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
    void       (*p_glProgramUniformMatrix3x4dvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
    void       (*p_glProgramUniformMatrix3x4fv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
    void       (*p_glProgramUniformMatrix3x4fvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
    void       (*p_glProgramUniformMatrix4dv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
    void       (*p_glProgramUniformMatrix4dvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
    void       (*p_glProgramUniformMatrix4fv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
    void       (*p_glProgramUniformMatrix4fvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
    void       (*p_glProgramUniformMatrix4x2dv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
    void       (*p_glProgramUniformMatrix4x2dvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
    void       (*p_glProgramUniformMatrix4x2fv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
    void       (*p_glProgramUniformMatrix4x2fvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
    void       (*p_glProgramUniformMatrix4x3dv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
    void       (*p_glProgramUniformMatrix4x3dvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
    void       (*p_glProgramUniformMatrix4x3fv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
    void       (*p_glProgramUniformMatrix4x3fvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
    void       (*p_glProgramUniformui64NV)( GLuint program, GLint location, GLuint64EXT value );
    void       (*p_glProgramUniformui64vNV)( GLuint program, GLint location, GLsizei count, const GLuint64EXT *value );
    void       (*p_glProgramVertexLimitNV)( GLenum target, GLint limit );
    void       (*p_glProvokingVertex)( GLenum mode );
    void       (*p_glProvokingVertexEXT)( GLenum mode );
    void       (*p_glPushClientAttribDefaultEXT)( GLbitfield mask );
    void       (*p_glPushDebugGroup)( GLenum source, GLuint id, GLsizei length, const GLchar *message );
    void       (*p_glPushGroupMarkerEXT)( GLsizei length, const GLchar *marker );
    void       (*p_glQueryCounter)( GLuint id, GLenum target );
    GLbitfield (*p_glQueryMatrixxOES)( GLfixed *mantissa, GLint *exponent );
    void       (*p_glQueryObjectParameteruiAMD)( GLenum target, GLuint id, GLenum pname, GLuint param );
    GLint      (*p_glQueryResourceNV)( GLenum queryType, GLint tagId, GLuint count, GLint *buffer );
    void       (*p_glQueryResourceTagNV)( GLint tagId, const GLchar *tagString );
    void       (*p_glRasterPos2xOES)( GLfixed x, GLfixed y );
    void       (*p_glRasterPos2xvOES)( const GLfixed *coords );
    void       (*p_glRasterPos3xOES)( GLfixed x, GLfixed y, GLfixed z );
    void       (*p_glRasterPos3xvOES)( const GLfixed *coords );
    void       (*p_glRasterPos4xOES)( GLfixed x, GLfixed y, GLfixed z, GLfixed w );
    void       (*p_glRasterPos4xvOES)( const GLfixed *coords );
    void       (*p_glRasterSamplesEXT)( GLuint samples, GLboolean fixedsamplelocations );
    void       (*p_glReadBufferRegion)( GLenum region, GLint x, GLint y, GLsizei width, GLsizei height );
    void       (*p_glReadInstrumentsSGIX)( GLint marker );
    void       (*p_glReadnPixels)( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, void *data );
    void       (*p_glReadnPixelsARB)( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, void *data );
    void       (*p_glRectxOES)( GLfixed x1, GLfixed y1, GLfixed x2, GLfixed y2 );
    void       (*p_glRectxvOES)( const GLfixed *v1, const GLfixed *v2 );
    void       (*p_glReferencePlaneSGIX)( const GLdouble *equation );
    GLboolean  (*p_glReleaseKeyedMutexWin32EXT)( GLuint memory, GLuint64 key );
    void       (*p_glReleaseShaderCompiler)(void);
    void       (*p_glRenderGpuMaskNV)( GLbitfield mask );
    void       (*p_glRenderbufferStorage)( GLenum target, GLenum internalformat, GLsizei width, GLsizei height );
    void       (*p_glRenderbufferStorageEXT)( GLenum target, GLenum internalformat, GLsizei width, GLsizei height );
    void       (*p_glRenderbufferStorageMultisample)( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height );
    void       (*p_glRenderbufferStorageMultisampleAdvancedAMD)( GLenum target, GLsizei samples, GLsizei storageSamples, GLenum internalformat, GLsizei width, GLsizei height );
    void       (*p_glRenderbufferStorageMultisampleCoverageNV)( GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLenum internalformat, GLsizei width, GLsizei height );
    void       (*p_glRenderbufferStorageMultisampleEXT)( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height );
    void       (*p_glReplacementCodePointerSUN)( GLenum type, GLsizei stride, const void **pointer );
    void       (*p_glReplacementCodeubSUN)( GLubyte code );
    void       (*p_glReplacementCodeubvSUN)( const GLubyte *code );
    void       (*p_glReplacementCodeuiColor3fVertex3fSUN)( GLuint rc, GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z );
    void       (*p_glReplacementCodeuiColor3fVertex3fvSUN)( const GLuint *rc, const GLfloat *c, const GLfloat *v );
    void       (*p_glReplacementCodeuiColor4fNormal3fVertex3fSUN)( GLuint rc, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z );
    void       (*p_glReplacementCodeuiColor4fNormal3fVertex3fvSUN)( const GLuint *rc, const GLfloat *c, const GLfloat *n, const GLfloat *v );
    void       (*p_glReplacementCodeuiColor4ubVertex3fSUN)( GLuint rc, GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z );
    void       (*p_glReplacementCodeuiColor4ubVertex3fvSUN)( const GLuint *rc, const GLubyte *c, const GLfloat *v );
    void       (*p_glReplacementCodeuiNormal3fVertex3fSUN)( GLuint rc, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z );
    void       (*p_glReplacementCodeuiNormal3fVertex3fvSUN)( const GLuint *rc, const GLfloat *n, const GLfloat *v );
    void       (*p_glReplacementCodeuiSUN)( GLuint code );
    void       (*p_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN)( GLuint rc, GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z );
    void       (*p_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN)( const GLuint *rc, const GLfloat *tc, const GLfloat *c, const GLfloat *n, const GLfloat *v );
    void       (*p_glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN)( GLuint rc, GLfloat s, GLfloat t, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z );
    void       (*p_glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN)( const GLuint *rc, const GLfloat *tc, const GLfloat *n, const GLfloat *v );
    void       (*p_glReplacementCodeuiTexCoord2fVertex3fSUN)( GLuint rc, GLfloat s, GLfloat t, GLfloat x, GLfloat y, GLfloat z );
    void       (*p_glReplacementCodeuiTexCoord2fVertex3fvSUN)( const GLuint *rc, const GLfloat *tc, const GLfloat *v );
    void       (*p_glReplacementCodeuiVertex3fSUN)( GLuint rc, GLfloat x, GLfloat y, GLfloat z );
    void       (*p_glReplacementCodeuiVertex3fvSUN)( const GLuint *rc, const GLfloat *v );
    void       (*p_glReplacementCodeuivSUN)( const GLuint *code );
    void       (*p_glReplacementCodeusSUN)( GLushort code );
    void       (*p_glReplacementCodeusvSUN)( const GLushort *code );
    void       (*p_glRequestResidentProgramsNV)( GLsizei n, const GLuint *programs );
    void       (*p_glResetHistogram)( GLenum target );
    void       (*p_glResetHistogramEXT)( GLenum target );
    void       (*p_glResetMemoryObjectParameterNV)( GLuint memory, GLenum pname );
    void       (*p_glResetMinmax)( GLenum target );
    void       (*p_glResetMinmaxEXT)( GLenum target );
    void       (*p_glResizeBuffersMESA)(void);
    void       (*p_glResolveDepthValuesNV)(void);
    void       (*p_glResumeTransformFeedback)(void);
    void       (*p_glResumeTransformFeedbackNV)(void);
    void       (*p_glRotatexOES)( GLfixed angle, GLfixed x, GLfixed y, GLfixed z );
    void       (*p_glSampleCoverage)( GLfloat value, GLboolean invert );
    void       (*p_glSampleCoverageARB)( GLfloat value, GLboolean invert );
    void       (*p_glSampleMapATI)( GLuint dst, GLuint interp, GLenum swizzle );
    void       (*p_glSampleMaskEXT)( GLclampf value, GLboolean invert );
    void       (*p_glSampleMaskIndexedNV)( GLuint index, GLbitfield mask );
    void       (*p_glSampleMaskSGIS)( GLclampf value, GLboolean invert );
    void       (*p_glSampleMaski)( GLuint maskNumber, GLbitfield mask );
    void       (*p_glSamplePatternEXT)( GLenum pattern );
    void       (*p_glSamplePatternSGIS)( GLenum pattern );
    void       (*p_glSamplerParameterIiv)( GLuint sampler, GLenum pname, const GLint *param );
    void       (*p_glSamplerParameterIuiv)( GLuint sampler, GLenum pname, const GLuint *param );
    void       (*p_glSamplerParameterf)( GLuint sampler, GLenum pname, GLfloat param );
    void       (*p_glSamplerParameterfv)( GLuint sampler, GLenum pname, const GLfloat *param );
    void       (*p_glSamplerParameteri)( GLuint sampler, GLenum pname, GLint param );
    void       (*p_glSamplerParameteriv)( GLuint sampler, GLenum pname, const GLint *param );
    void       (*p_glScalexOES)( GLfixed x, GLfixed y, GLfixed z );
    void       (*p_glScissorArrayv)( GLuint first, GLsizei count, const GLint *v );
    void       (*p_glScissorExclusiveArrayvNV)( GLuint first, GLsizei count, const GLint *v );
    void       (*p_glScissorExclusiveNV)( GLint x, GLint y, GLsizei width, GLsizei height );
    void       (*p_glScissorIndexed)( GLuint index, GLint left, GLint bottom, GLsizei width, GLsizei height );
    void       (*p_glScissorIndexedv)( GLuint index, const GLint *v );
    void       (*p_glSecondaryColor3b)( GLbyte red, GLbyte green, GLbyte blue );
    void       (*p_glSecondaryColor3bEXT)( GLbyte red, GLbyte green, GLbyte blue );
    void       (*p_glSecondaryColor3bv)( const GLbyte *v );
    void       (*p_glSecondaryColor3bvEXT)( const GLbyte *v );
    void       (*p_glSecondaryColor3d)( GLdouble red, GLdouble green, GLdouble blue );
    void       (*p_glSecondaryColor3dEXT)( GLdouble red, GLdouble green, GLdouble blue );
    void       (*p_glSecondaryColor3dv)( const GLdouble *v );
    void       (*p_glSecondaryColor3dvEXT)( const GLdouble *v );
    void       (*p_glSecondaryColor3f)( GLfloat red, GLfloat green, GLfloat blue );
    void       (*p_glSecondaryColor3fEXT)( GLfloat red, GLfloat green, GLfloat blue );
    void       (*p_glSecondaryColor3fv)( const GLfloat *v );
    void       (*p_glSecondaryColor3fvEXT)( const GLfloat *v );
    void       (*p_glSecondaryColor3hNV)( GLhalfNV red, GLhalfNV green, GLhalfNV blue );
    void       (*p_glSecondaryColor3hvNV)( const GLhalfNV *v );
    void       (*p_glSecondaryColor3i)( GLint red, GLint green, GLint blue );
    void       (*p_glSecondaryColor3iEXT)( GLint red, GLint green, GLint blue );
    void       (*p_glSecondaryColor3iv)( const GLint *v );
    void       (*p_glSecondaryColor3ivEXT)( const GLint *v );
    void       (*p_glSecondaryColor3s)( GLshort red, GLshort green, GLshort blue );
    void       (*p_glSecondaryColor3sEXT)( GLshort red, GLshort green, GLshort blue );
    void       (*p_glSecondaryColor3sv)( const GLshort *v );
    void       (*p_glSecondaryColor3svEXT)( const GLshort *v );
    void       (*p_glSecondaryColor3ub)( GLubyte red, GLubyte green, GLubyte blue );
    void       (*p_glSecondaryColor3ubEXT)( GLubyte red, GLubyte green, GLubyte blue );
    void       (*p_glSecondaryColor3ubv)( const GLubyte *v );
    void       (*p_glSecondaryColor3ubvEXT)( const GLubyte *v );
    void       (*p_glSecondaryColor3ui)( GLuint red, GLuint green, GLuint blue );
    void       (*p_glSecondaryColor3uiEXT)( GLuint red, GLuint green, GLuint blue );
    void       (*p_glSecondaryColor3uiv)( const GLuint *v );
    void       (*p_glSecondaryColor3uivEXT)( const GLuint *v );
    void       (*p_glSecondaryColor3us)( GLushort red, GLushort green, GLushort blue );
    void       (*p_glSecondaryColor3usEXT)( GLushort red, GLushort green, GLushort blue );
    void       (*p_glSecondaryColor3usv)( const GLushort *v );
    void       (*p_glSecondaryColor3usvEXT)( const GLushort *v );
    void       (*p_glSecondaryColorFormatNV)( GLint size, GLenum type, GLsizei stride );
    void       (*p_glSecondaryColorP3ui)( GLenum type, GLuint color );
    void       (*p_glSecondaryColorP3uiv)( GLenum type, const GLuint *color );
    void       (*p_glSecondaryColorPointer)( GLint size, GLenum type, GLsizei stride, const void *pointer );
    void       (*p_glSecondaryColorPointerEXT)( GLint size, GLenum type, GLsizei stride, const void *pointer );
    void       (*p_glSecondaryColorPointerListIBM)( GLint size, GLenum type, GLint stride, const void **pointer, GLint ptrstride );
    void       (*p_glSelectPerfMonitorCountersAMD)( GLuint monitor, GLboolean enable, GLuint group, GLint numCounters, GLuint *counterList );
    void       (*p_glSelectTextureCoordSetSGIS)( GLenum target );
    void       (*p_glSelectTextureSGIS)( GLenum target );
    void       (*p_glSemaphoreParameterui64vEXT)( GLuint semaphore, GLenum pname, const GLuint64 *params );
    void       (*p_glSeparableFilter2D)( GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *row, const void *column );
    void       (*p_glSeparableFilter2DEXT)( GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *row, const void *column );
    void       (*p_glSetFenceAPPLE)( GLuint fence );
    void       (*p_glSetFenceNV)( GLuint fence, GLenum condition );
    void       (*p_glSetFragmentShaderConstantATI)( GLuint dst, const GLfloat *value );
    void       (*p_glSetInvariantEXT)( GLuint id, GLenum type, const void *addr );
    void       (*p_glSetLocalConstantEXT)( GLuint id, GLenum type, const void *addr );
    void       (*p_glSetMultisamplefvAMD)( GLenum pname, GLuint index, const GLfloat *val );
    void       (*p_glShaderBinary)( GLsizei count, const GLuint *shaders, GLenum binaryformat, const void *binary, GLsizei length );
    void       (*p_glShaderOp1EXT)( GLenum op, GLuint res, GLuint arg1 );
    void       (*p_glShaderOp2EXT)( GLenum op, GLuint res, GLuint arg1, GLuint arg2 );
    void       (*p_glShaderOp3EXT)( GLenum op, GLuint res, GLuint arg1, GLuint arg2, GLuint arg3 );
    void       (*p_glShaderSource)( GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length );
    void       (*p_glShaderSourceARB)( GLhandleARB shaderObj, GLsizei count, const GLcharARB **string, const GLint *length );
    void       (*p_glShaderStorageBlockBinding)( GLuint program, GLuint storageBlockIndex, GLuint storageBlockBinding );
    void       (*p_glShadingRateImageBarrierNV)( GLboolean synchronize );
    void       (*p_glShadingRateImagePaletteNV)( GLuint viewport, GLuint first, GLsizei count, const GLenum *rates );
    void       (*p_glShadingRateSampleOrderCustomNV)( GLenum rate, GLuint samples, const GLint *locations );
    void       (*p_glShadingRateSampleOrderNV)( GLenum order );
    void       (*p_glSharpenTexFuncSGIS)( GLenum target, GLsizei n, const GLfloat *points );
    void       (*p_glSignalSemaphoreEXT)( GLuint semaphore, GLuint numBufferBarriers, const GLuint *buffers, GLuint numTextureBarriers, const GLuint *textures, const GLenum *dstLayouts );
    void       (*p_glSignalSemaphoreui64NVX)( GLuint signalGpu, GLsizei fenceObjectCount, const GLuint *semaphoreArray, const GLuint64 *fenceValueArray );
    void       (*p_glSignalVkFenceNV)( GLuint64 vkFence );
    void       (*p_glSignalVkSemaphoreNV)( GLuint64 vkSemaphore );
    void       (*p_glSpecializeShader)( GLuint shader, const GLchar *pEntryPoint, GLuint numSpecializationConstants, const GLuint *pConstantIndex, const GLuint *pConstantValue );
    void       (*p_glSpecializeShaderARB)( GLuint shader, const GLchar *pEntryPoint, GLuint numSpecializationConstants, const GLuint *pConstantIndex, const GLuint *pConstantValue );
    void       (*p_glSpriteParameterfSGIX)( GLenum pname, GLfloat param );
    void       (*p_glSpriteParameterfvSGIX)( GLenum pname, const GLfloat *params );
    void       (*p_glSpriteParameteriSGIX)( GLenum pname, GLint param );
    void       (*p_glSpriteParameterivSGIX)( GLenum pname, const GLint *params );
    void       (*p_glStartInstrumentsSGIX)(void);
    void       (*p_glStateCaptureNV)( GLuint state, GLenum mode );
    void       (*p_glStencilClearTagEXT)( GLsizei stencilTagBits, GLuint stencilClearTag );
    void       (*p_glStencilFillPathInstancedNV)( GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum fillMode, GLuint mask, GLenum transformType, const GLfloat *transformValues );
    void       (*p_glStencilFillPathNV)( GLuint path, GLenum fillMode, GLuint mask );
    void       (*p_glStencilFuncSeparate)( GLenum face, GLenum func, GLint ref, GLuint mask );
    void       (*p_glStencilFuncSeparateATI)( GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask );
    void       (*p_glStencilMaskSeparate)( GLenum face, GLuint mask );
    void       (*p_glStencilOpSeparate)( GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass );
    void       (*p_glStencilOpSeparateATI)( GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass );
    void       (*p_glStencilOpValueAMD)( GLenum face, GLuint value );
    void       (*p_glStencilStrokePathInstancedNV)( GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLint reference, GLuint mask, GLenum transformType, const GLfloat *transformValues );
    void       (*p_glStencilStrokePathNV)( GLuint path, GLint reference, GLuint mask );
    void       (*p_glStencilThenCoverFillPathInstancedNV)( GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum fillMode, GLuint mask, GLenum coverMode, GLenum transformType, const GLfloat *transformValues );
    void       (*p_glStencilThenCoverFillPathNV)( GLuint path, GLenum fillMode, GLuint mask, GLenum coverMode );
    void       (*p_glStencilThenCoverStrokePathInstancedNV)( GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLint reference, GLuint mask, GLenum coverMode, GLenum transformType, const GLfloat *transformValues );
    void       (*p_glStencilThenCoverStrokePathNV)( GLuint path, GLint reference, GLuint mask, GLenum coverMode );
    void       (*p_glStopInstrumentsSGIX)( GLint marker );
    void       (*p_glStringMarkerGREMEDY)( GLsizei len, const void *string );
    void       (*p_glSubpixelPrecisionBiasNV)( GLuint xbits, GLuint ybits );
    void       (*p_glSwizzleEXT)( GLuint res, GLuint in, GLenum outX, GLenum outY, GLenum outZ, GLenum outW );
    void       (*p_glSyncTextureINTEL)( GLuint texture );
    void       (*p_glTagSampleBufferSGIX)(void);
    void       (*p_glTangent3bEXT)( GLbyte tx, GLbyte ty, GLbyte tz );
    void       (*p_glTangent3bvEXT)( const GLbyte *v );
    void       (*p_glTangent3dEXT)( GLdouble tx, GLdouble ty, GLdouble tz );
    void       (*p_glTangent3dvEXT)( const GLdouble *v );
    void       (*p_glTangent3fEXT)( GLfloat tx, GLfloat ty, GLfloat tz );
    void       (*p_glTangent3fvEXT)( const GLfloat *v );
    void       (*p_glTangent3iEXT)( GLint tx, GLint ty, GLint tz );
    void       (*p_glTangent3ivEXT)( const GLint *v );
    void       (*p_glTangent3sEXT)( GLshort tx, GLshort ty, GLshort tz );
    void       (*p_glTangent3svEXT)( const GLshort *v );
    void       (*p_glTangentPointerEXT)( GLenum type, GLsizei stride, const void *pointer );
    void       (*p_glTbufferMask3DFX)( GLuint mask );
    void       (*p_glTessellationFactorAMD)( GLfloat factor );
    void       (*p_glTessellationModeAMD)( GLenum mode );
    GLboolean  (*p_glTestFenceAPPLE)( GLuint fence );
    GLboolean  (*p_glTestFenceNV)( GLuint fence );
    GLboolean  (*p_glTestObjectAPPLE)( GLenum object, GLuint name );
    void       (*p_glTexAttachMemoryNV)( GLenum target, GLuint memory, GLuint64 offset );
    void       (*p_glTexBuffer)( GLenum target, GLenum internalformat, GLuint buffer );
    void       (*p_glTexBufferARB)( GLenum target, GLenum internalformat, GLuint buffer );
    void       (*p_glTexBufferEXT)( GLenum target, GLenum internalformat, GLuint buffer );
    void       (*p_glTexBufferRange)( GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size );
    void       (*p_glTexBumpParameterfvATI)( GLenum pname, const GLfloat *param );
    void       (*p_glTexBumpParameterivATI)( GLenum pname, const GLint *param );
    void       (*p_glTexCoord1bOES)( GLbyte s );
    void       (*p_glTexCoord1bvOES)( const GLbyte *coords );
    void       (*p_glTexCoord1hNV)( GLhalfNV s );
    void       (*p_glTexCoord1hvNV)( const GLhalfNV *v );
    void       (*p_glTexCoord1xOES)( GLfixed s );
    void       (*p_glTexCoord1xvOES)( const GLfixed *coords );
    void       (*p_glTexCoord2bOES)( GLbyte s, GLbyte t );
    void       (*p_glTexCoord2bvOES)( const GLbyte *coords );
    void       (*p_glTexCoord2fColor3fVertex3fSUN)( GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z );
    void       (*p_glTexCoord2fColor3fVertex3fvSUN)( const GLfloat *tc, const GLfloat *c, const GLfloat *v );
    void       (*p_glTexCoord2fColor4fNormal3fVertex3fSUN)( GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z );
    void       (*p_glTexCoord2fColor4fNormal3fVertex3fvSUN)( const GLfloat *tc, const GLfloat *c, const GLfloat *n, const GLfloat *v );
    void       (*p_glTexCoord2fColor4ubVertex3fSUN)( GLfloat s, GLfloat t, GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z );
    void       (*p_glTexCoord2fColor4ubVertex3fvSUN)( const GLfloat *tc, const GLubyte *c, const GLfloat *v );
    void       (*p_glTexCoord2fNormal3fVertex3fSUN)( GLfloat s, GLfloat t, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z );
    void       (*p_glTexCoord2fNormal3fVertex3fvSUN)( const GLfloat *tc, const GLfloat *n, const GLfloat *v );
    void       (*p_glTexCoord2fVertex3fSUN)( GLfloat s, GLfloat t, GLfloat x, GLfloat y, GLfloat z );
    void       (*p_glTexCoord2fVertex3fvSUN)( const GLfloat *tc, const GLfloat *v );
    void       (*p_glTexCoord2hNV)( GLhalfNV s, GLhalfNV t );
    void       (*p_glTexCoord2hvNV)( const GLhalfNV *v );
    void       (*p_glTexCoord2xOES)( GLfixed s, GLfixed t );
    void       (*p_glTexCoord2xvOES)( const GLfixed *coords );
    void       (*p_glTexCoord3bOES)( GLbyte s, GLbyte t, GLbyte r );
    void       (*p_glTexCoord3bvOES)( const GLbyte *coords );
    void       (*p_glTexCoord3hNV)( GLhalfNV s, GLhalfNV t, GLhalfNV r );
    void       (*p_glTexCoord3hvNV)( const GLhalfNV *v );
    void       (*p_glTexCoord3xOES)( GLfixed s, GLfixed t, GLfixed r );
    void       (*p_glTexCoord3xvOES)( const GLfixed *coords );
    void       (*p_glTexCoord4bOES)( GLbyte s, GLbyte t, GLbyte r, GLbyte q );
    void       (*p_glTexCoord4bvOES)( const GLbyte *coords );
    void       (*p_glTexCoord4fColor4fNormal3fVertex4fSUN)( GLfloat s, GLfloat t, GLfloat p, GLfloat q, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
    void       (*p_glTexCoord4fColor4fNormal3fVertex4fvSUN)( const GLfloat *tc, const GLfloat *c, const GLfloat *n, const GLfloat *v );
    void       (*p_glTexCoord4fVertex4fSUN)( GLfloat s, GLfloat t, GLfloat p, GLfloat q, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
    void       (*p_glTexCoord4fVertex4fvSUN)( const GLfloat *tc, const GLfloat *v );
    void       (*p_glTexCoord4hNV)( GLhalfNV s, GLhalfNV t, GLhalfNV r, GLhalfNV q );
    void       (*p_glTexCoord4hvNV)( const GLhalfNV *v );
    void       (*p_glTexCoord4xOES)( GLfixed s, GLfixed t, GLfixed r, GLfixed q );
    void       (*p_glTexCoord4xvOES)( const GLfixed *coords );
    void       (*p_glTexCoordFormatNV)( GLint size, GLenum type, GLsizei stride );
    void       (*p_glTexCoordP1ui)( GLenum type, GLuint coords );
    void       (*p_glTexCoordP1uiv)( GLenum type, const GLuint *coords );
    void       (*p_glTexCoordP2ui)( GLenum type, GLuint coords );
    void       (*p_glTexCoordP2uiv)( GLenum type, const GLuint *coords );
    void       (*p_glTexCoordP3ui)( GLenum type, GLuint coords );
    void       (*p_glTexCoordP3uiv)( GLenum type, const GLuint *coords );
    void       (*p_glTexCoordP4ui)( GLenum type, GLuint coords );
    void       (*p_glTexCoordP4uiv)( GLenum type, const GLuint *coords );
    void       (*p_glTexCoordPointerEXT)( GLint size, GLenum type, GLsizei stride, GLsizei count, const void *pointer );
    void       (*p_glTexCoordPointerListIBM)( GLint size, GLenum type, GLint stride, const void **pointer, GLint ptrstride );
    void       (*p_glTexCoordPointervINTEL)( GLint size, GLenum type, const void **pointer );
    void       (*p_glTexEnvxOES)( GLenum target, GLenum pname, GLfixed param );
    void       (*p_glTexEnvxvOES)( GLenum target, GLenum pname, const GLfixed *params );
    void       (*p_glTexFilterFuncSGIS)( GLenum target, GLenum filter, GLsizei n, const GLfloat *weights );
    void       (*p_glTexGenxOES)( GLenum coord, GLenum pname, GLfixed param );
    void       (*p_glTexGenxvOES)( GLenum coord, GLenum pname, const GLfixed *params );
    void       (*p_glTexImage2DMultisample)( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations );
    void       (*p_glTexImage2DMultisampleCoverageNV)( GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLint internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations );
    void       (*p_glTexImage3D)( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels );
    void       (*p_glTexImage3DEXT)( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels );
    void       (*p_glTexImage3DMultisample)( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations );
    void       (*p_glTexImage3DMultisampleCoverageNV)( GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations );
    void       (*p_glTexImage4DSGIS)( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLint border, GLenum format, GLenum type, const void *pixels );
    void       (*p_glTexPageCommitmentARB)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLboolean commit );
    void       (*p_glTexParameterIiv)( GLenum target, GLenum pname, const GLint *params );
    void       (*p_glTexParameterIivEXT)( GLenum target, GLenum pname, const GLint *params );
    void       (*p_glTexParameterIuiv)( GLenum target, GLenum pname, const GLuint *params );
    void       (*p_glTexParameterIuivEXT)( GLenum target, GLenum pname, const GLuint *params );
    void       (*p_glTexParameterxOES)( GLenum target, GLenum pname, GLfixed param );
    void       (*p_glTexParameterxvOES)( GLenum target, GLenum pname, const GLfixed *params );
    void       (*p_glTexRenderbufferNV)( GLenum target, GLuint renderbuffer );
    void       (*p_glTexStorage1D)( GLenum target, GLsizei levels, GLenum internalformat, GLsizei width );
    void       (*p_glTexStorage2D)( GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height );
    void       (*p_glTexStorage2DMultisample)( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations );
    void       (*p_glTexStorage3D)( GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth );
    void       (*p_glTexStorage3DMultisample)( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations );
    void       (*p_glTexStorageMem1DEXT)( GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLuint memory, GLuint64 offset );
    void       (*p_glTexStorageMem2DEXT)( GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLuint memory, GLuint64 offset );
    void       (*p_glTexStorageMem2DMultisampleEXT)( GLenum target, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset );
    void       (*p_glTexStorageMem3DEXT)( GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLuint memory, GLuint64 offset );
    void       (*p_glTexStorageMem3DMultisampleEXT)( GLenum target, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset );
    void       (*p_glTexStorageSparseAMD)( GLenum target, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLsizei layers, GLbitfield flags );
    void       (*p_glTexSubImage1DEXT)( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels );
    void       (*p_glTexSubImage2DEXT)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels );
    void       (*p_glTexSubImage3D)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels );
    void       (*p_glTexSubImage3DEXT)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels );
    void       (*p_glTexSubImage4DSGIS)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint woffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLenum format, GLenum type, const void *pixels );
    void       (*p_glTextureAttachMemoryNV)( GLuint texture, GLuint memory, GLuint64 offset );
    void       (*p_glTextureBarrier)(void);
    void       (*p_glTextureBarrierNV)(void);
    void       (*p_glTextureBuffer)( GLuint texture, GLenum internalformat, GLuint buffer );
    void       (*p_glTextureBufferEXT)( GLuint texture, GLenum target, GLenum internalformat, GLuint buffer );
    void       (*p_glTextureBufferRange)( GLuint texture, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size );
    void       (*p_glTextureBufferRangeEXT)( GLuint texture, GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size );
    void       (*p_glTextureColorMaskSGIS)( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha );
    void       (*p_glTextureImage1DEXT)( GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels );
    void       (*p_glTextureImage2DEXT)( GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels );
    void       (*p_glTextureImage2DMultisampleCoverageNV)( GLuint texture, GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLint internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations );
    void       (*p_glTextureImage2DMultisampleNV)( GLuint texture, GLenum target, GLsizei samples, GLint internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations );
    void       (*p_glTextureImage3DEXT)( GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels );
    void       (*p_glTextureImage3DMultisampleCoverageNV)( GLuint texture, GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations );
    void       (*p_glTextureImage3DMultisampleNV)( GLuint texture, GLenum target, GLsizei samples, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations );
    void       (*p_glTextureLightEXT)( GLenum pname );
    void       (*p_glTextureMaterialEXT)( GLenum face, GLenum mode );
    void       (*p_glTextureNormalEXT)( GLenum mode );
    void       (*p_glTexturePageCommitmentEXT)( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLboolean commit );
    void       (*p_glTextureParameterIiv)( GLuint texture, GLenum pname, const GLint *params );
    void       (*p_glTextureParameterIivEXT)( GLuint texture, GLenum target, GLenum pname, const GLint *params );
    void       (*p_glTextureParameterIuiv)( GLuint texture, GLenum pname, const GLuint *params );
    void       (*p_glTextureParameterIuivEXT)( GLuint texture, GLenum target, GLenum pname, const GLuint *params );
    void       (*p_glTextureParameterf)( GLuint texture, GLenum pname, GLfloat param );
    void       (*p_glTextureParameterfEXT)( GLuint texture, GLenum target, GLenum pname, GLfloat param );
    void       (*p_glTextureParameterfv)( GLuint texture, GLenum pname, const GLfloat *param );
    void       (*p_glTextureParameterfvEXT)( GLuint texture, GLenum target, GLenum pname, const GLfloat *params );
    void       (*p_glTextureParameteri)( GLuint texture, GLenum pname, GLint param );
    void       (*p_glTextureParameteriEXT)( GLuint texture, GLenum target, GLenum pname, GLint param );
    void       (*p_glTextureParameteriv)( GLuint texture, GLenum pname, const GLint *param );
    void       (*p_glTextureParameterivEXT)( GLuint texture, GLenum target, GLenum pname, const GLint *params );
    void       (*p_glTextureRangeAPPLE)( GLenum target, GLsizei length, const void *pointer );
    void       (*p_glTextureRenderbufferEXT)( GLuint texture, GLenum target, GLuint renderbuffer );
    void       (*p_glTextureStorage1D)( GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width );
    void       (*p_glTextureStorage1DEXT)( GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width );
    void       (*p_glTextureStorage2D)( GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height );
    void       (*p_glTextureStorage2DEXT)( GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height );
    void       (*p_glTextureStorage2DMultisample)( GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations );
    void       (*p_glTextureStorage2DMultisampleEXT)( GLuint texture, GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations );
    void       (*p_glTextureStorage3D)( GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth );
    void       (*p_glTextureStorage3DEXT)( GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth );
    void       (*p_glTextureStorage3DMultisample)( GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations );
    void       (*p_glTextureStorage3DMultisampleEXT)( GLuint texture, GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations );
    void       (*p_glTextureStorageMem1DEXT)( GLuint texture, GLsizei levels, GLenum internalFormat, GLsizei width, GLuint memory, GLuint64 offset );
    void       (*p_glTextureStorageMem2DEXT)( GLuint texture, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLuint memory, GLuint64 offset );
    void       (*p_glTextureStorageMem2DMultisampleEXT)( GLuint texture, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset );
    void       (*p_glTextureStorageMem3DEXT)( GLuint texture, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLuint memory, GLuint64 offset );
    void       (*p_glTextureStorageMem3DMultisampleEXT)( GLuint texture, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset );
    void       (*p_glTextureStorageSparseAMD)( GLuint texture, GLenum target, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLsizei layers, GLbitfield flags );
    void       (*p_glTextureSubImage1D)( GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels );
    void       (*p_glTextureSubImage1DEXT)( GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels );
    void       (*p_glTextureSubImage2D)( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels );
    void       (*p_glTextureSubImage2DEXT)( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels );
    void       (*p_glTextureSubImage3D)( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels );
    void       (*p_glTextureSubImage3DEXT)( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels );
    void       (*p_glTextureView)( GLuint texture, GLenum target, GLuint origtexture, GLenum internalformat, GLuint minlevel, GLuint numlevels, GLuint minlayer, GLuint numlayers );
    void       (*p_glTrackMatrixNV)( GLenum target, GLuint address, GLenum matrix, GLenum transform );
    void       (*p_glTransformFeedbackAttribsNV)( GLsizei count, const GLint *attribs, GLenum bufferMode );
    void       (*p_glTransformFeedbackBufferBase)( GLuint xfb, GLuint index, GLuint buffer );
    void       (*p_glTransformFeedbackBufferRange)( GLuint xfb, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size );
    void       (*p_glTransformFeedbackStreamAttribsNV)( GLsizei count, const GLint *attribs, GLsizei nbuffers, const GLint *bufstreams, GLenum bufferMode );
    void       (*p_glTransformFeedbackVaryings)( GLuint program, GLsizei count, const GLchar *const*varyings, GLenum bufferMode );
    void       (*p_glTransformFeedbackVaryingsEXT)( GLuint program, GLsizei count, const GLchar *const*varyings, GLenum bufferMode );
    void       (*p_glTransformFeedbackVaryingsNV)( GLuint program, GLsizei count, const GLint *locations, GLenum bufferMode );
    void       (*p_glTransformPathNV)( GLuint resultPath, GLuint srcPath, GLenum transformType, const GLfloat *transformValues );
    void       (*p_glTranslatexOES)( GLfixed x, GLfixed y, GLfixed z );
    void       (*p_glUniform1d)( GLint location, GLdouble x );
    void       (*p_glUniform1dv)( GLint location, GLsizei count, const GLdouble *value );
    void       (*p_glUniform1f)( GLint location, GLfloat v0 );
    void       (*p_glUniform1fARB)( GLint location, GLfloat v0 );
    void       (*p_glUniform1fv)( GLint location, GLsizei count, const GLfloat *value );
    void       (*p_glUniform1fvARB)( GLint location, GLsizei count, const GLfloat *value );
    void       (*p_glUniform1i)( GLint location, GLint v0 );
    void       (*p_glUniform1i64ARB)( GLint location, GLint64 x );
    void       (*p_glUniform1i64NV)( GLint location, GLint64EXT x );
    void       (*p_glUniform1i64vARB)( GLint location, GLsizei count, const GLint64 *value );
    void       (*p_glUniform1i64vNV)( GLint location, GLsizei count, const GLint64EXT *value );
    void       (*p_glUniform1iARB)( GLint location, GLint v0 );
    void       (*p_glUniform1iv)( GLint location, GLsizei count, const GLint *value );
    void       (*p_glUniform1ivARB)( GLint location, GLsizei count, const GLint *value );
    void       (*p_glUniform1ui)( GLint location, GLuint v0 );
    void       (*p_glUniform1ui64ARB)( GLint location, GLuint64 x );
    void       (*p_glUniform1ui64NV)( GLint location, GLuint64EXT x );
    void       (*p_glUniform1ui64vARB)( GLint location, GLsizei count, const GLuint64 *value );
    void       (*p_glUniform1ui64vNV)( GLint location, GLsizei count, const GLuint64EXT *value );
    void       (*p_glUniform1uiEXT)( GLint location, GLuint v0 );
    void       (*p_glUniform1uiv)( GLint location, GLsizei count, const GLuint *value );
    void       (*p_glUniform1uivEXT)( GLint location, GLsizei count, const GLuint *value );
    void       (*p_glUniform2d)( GLint location, GLdouble x, GLdouble y );
    void       (*p_glUniform2dv)( GLint location, GLsizei count, const GLdouble *value );
    void       (*p_glUniform2f)( GLint location, GLfloat v0, GLfloat v1 );
    void       (*p_glUniform2fARB)( GLint location, GLfloat v0, GLfloat v1 );
    void       (*p_glUniform2fv)( GLint location, GLsizei count, const GLfloat *value );
    void       (*p_glUniform2fvARB)( GLint location, GLsizei count, const GLfloat *value );
    void       (*p_glUniform2i)( GLint location, GLint v0, GLint v1 );
    void       (*p_glUniform2i64ARB)( GLint location, GLint64 x, GLint64 y );
    void       (*p_glUniform2i64NV)( GLint location, GLint64EXT x, GLint64EXT y );
    void       (*p_glUniform2i64vARB)( GLint location, GLsizei count, const GLint64 *value );
    void       (*p_glUniform2i64vNV)( GLint location, GLsizei count, const GLint64EXT *value );
    void       (*p_glUniform2iARB)( GLint location, GLint v0, GLint v1 );
    void       (*p_glUniform2iv)( GLint location, GLsizei count, const GLint *value );
    void       (*p_glUniform2ivARB)( GLint location, GLsizei count, const GLint *value );
    void       (*p_glUniform2ui)( GLint location, GLuint v0, GLuint v1 );
    void       (*p_glUniform2ui64ARB)( GLint location, GLuint64 x, GLuint64 y );
    void       (*p_glUniform2ui64NV)( GLint location, GLuint64EXT x, GLuint64EXT y );
    void       (*p_glUniform2ui64vARB)( GLint location, GLsizei count, const GLuint64 *value );
    void       (*p_glUniform2ui64vNV)( GLint location, GLsizei count, const GLuint64EXT *value );
    void       (*p_glUniform2uiEXT)( GLint location, GLuint v0, GLuint v1 );
    void       (*p_glUniform2uiv)( GLint location, GLsizei count, const GLuint *value );
    void       (*p_glUniform2uivEXT)( GLint location, GLsizei count, const GLuint *value );
    void       (*p_glUniform3d)( GLint location, GLdouble x, GLdouble y, GLdouble z );
    void       (*p_glUniform3dv)( GLint location, GLsizei count, const GLdouble *value );
    void       (*p_glUniform3f)( GLint location, GLfloat v0, GLfloat v1, GLfloat v2 );
    void       (*p_glUniform3fARB)( GLint location, GLfloat v0, GLfloat v1, GLfloat v2 );
    void       (*p_glUniform3fv)( GLint location, GLsizei count, const GLfloat *value );
    void       (*p_glUniform3fvARB)( GLint location, GLsizei count, const GLfloat *value );
    void       (*p_glUniform3i)( GLint location, GLint v0, GLint v1, GLint v2 );
    void       (*p_glUniform3i64ARB)( GLint location, GLint64 x, GLint64 y, GLint64 z );
    void       (*p_glUniform3i64NV)( GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z );
    void       (*p_glUniform3i64vARB)( GLint location, GLsizei count, const GLint64 *value );
    void       (*p_glUniform3i64vNV)( GLint location, GLsizei count, const GLint64EXT *value );
    void       (*p_glUniform3iARB)( GLint location, GLint v0, GLint v1, GLint v2 );
    void       (*p_glUniform3iv)( GLint location, GLsizei count, const GLint *value );
    void       (*p_glUniform3ivARB)( GLint location, GLsizei count, const GLint *value );
    void       (*p_glUniform3ui)( GLint location, GLuint v0, GLuint v1, GLuint v2 );
    void       (*p_glUniform3ui64ARB)( GLint location, GLuint64 x, GLuint64 y, GLuint64 z );
    void       (*p_glUniform3ui64NV)( GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z );
    void       (*p_glUniform3ui64vARB)( GLint location, GLsizei count, const GLuint64 *value );
    void       (*p_glUniform3ui64vNV)( GLint location, GLsizei count, const GLuint64EXT *value );
    void       (*p_glUniform3uiEXT)( GLint location, GLuint v0, GLuint v1, GLuint v2 );
    void       (*p_glUniform3uiv)( GLint location, GLsizei count, const GLuint *value );
    void       (*p_glUniform3uivEXT)( GLint location, GLsizei count, const GLuint *value );
    void       (*p_glUniform4d)( GLint location, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
    void       (*p_glUniform4dv)( GLint location, GLsizei count, const GLdouble *value );
    void       (*p_glUniform4f)( GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 );
    void       (*p_glUniform4fARB)( GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 );
    void       (*p_glUniform4fv)( GLint location, GLsizei count, const GLfloat *value );
    void       (*p_glUniform4fvARB)( GLint location, GLsizei count, const GLfloat *value );
    void       (*p_glUniform4i)( GLint location, GLint v0, GLint v1, GLint v2, GLint v3 );
    void       (*p_glUniform4i64ARB)( GLint location, GLint64 x, GLint64 y, GLint64 z, GLint64 w );
    void       (*p_glUniform4i64NV)( GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z, GLint64EXT w );
    void       (*p_glUniform4i64vARB)( GLint location, GLsizei count, const GLint64 *value );
    void       (*p_glUniform4i64vNV)( GLint location, GLsizei count, const GLint64EXT *value );
    void       (*p_glUniform4iARB)( GLint location, GLint v0, GLint v1, GLint v2, GLint v3 );
    void       (*p_glUniform4iv)( GLint location, GLsizei count, const GLint *value );
    void       (*p_glUniform4ivARB)( GLint location, GLsizei count, const GLint *value );
    void       (*p_glUniform4ui)( GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3 );
    void       (*p_glUniform4ui64ARB)( GLint location, GLuint64 x, GLuint64 y, GLuint64 z, GLuint64 w );
    void       (*p_glUniform4ui64NV)( GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z, GLuint64EXT w );
    void       (*p_glUniform4ui64vARB)( GLint location, GLsizei count, const GLuint64 *value );
    void       (*p_glUniform4ui64vNV)( GLint location, GLsizei count, const GLuint64EXT *value );
    void       (*p_glUniform4uiEXT)( GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3 );
    void       (*p_glUniform4uiv)( GLint location, GLsizei count, const GLuint *value );
    void       (*p_glUniform4uivEXT)( GLint location, GLsizei count, const GLuint *value );
    void       (*p_glUniformBlockBinding)( GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding );
    void       (*p_glUniformBufferEXT)( GLuint program, GLint location, GLuint buffer );
    void       (*p_glUniformHandleui64ARB)( GLint location, GLuint64 value );
    void       (*p_glUniformHandleui64NV)( GLint location, GLuint64 value );
    void       (*p_glUniformHandleui64vARB)( GLint location, GLsizei count, const GLuint64 *value );
    void       (*p_glUniformHandleui64vNV)( GLint location, GLsizei count, const GLuint64 *value );
    void       (*p_glUniformMatrix2dv)( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
    void       (*p_glUniformMatrix2fv)( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
    void       (*p_glUniformMatrix2fvARB)( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
    void       (*p_glUniformMatrix2x3dv)( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
    void       (*p_glUniformMatrix2x3fv)( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
    void       (*p_glUniformMatrix2x4dv)( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
    void       (*p_glUniformMatrix2x4fv)( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
    void       (*p_glUniformMatrix3dv)( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
    void       (*p_glUniformMatrix3fv)( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
    void       (*p_glUniformMatrix3fvARB)( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
    void       (*p_glUniformMatrix3x2dv)( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
    void       (*p_glUniformMatrix3x2fv)( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
    void       (*p_glUniformMatrix3x4dv)( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
    void       (*p_glUniformMatrix3x4fv)( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
    void       (*p_glUniformMatrix4dv)( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
    void       (*p_glUniformMatrix4fv)( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
    void       (*p_glUniformMatrix4fvARB)( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
    void       (*p_glUniformMatrix4x2dv)( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
    void       (*p_glUniformMatrix4x2fv)( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
    void       (*p_glUniformMatrix4x3dv)( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
    void       (*p_glUniformMatrix4x3fv)( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
    void       (*p_glUniformSubroutinesuiv)( GLenum shadertype, GLsizei count, const GLuint *indices );
    void       (*p_glUniformui64NV)( GLint location, GLuint64EXT value );
    void       (*p_glUniformui64vNV)( GLint location, GLsizei count, const GLuint64EXT *value );
    void       (*p_glUnlockArraysEXT)(void);
    GLboolean  (*p_glUnmapBuffer)( GLenum target );
    GLboolean  (*p_glUnmapBufferARB)( GLenum target );
    GLboolean  (*p_glUnmapNamedBuffer)( GLuint buffer );
    GLboolean  (*p_glUnmapNamedBufferEXT)( GLuint buffer );
    void       (*p_glUnmapObjectBufferATI)( GLuint buffer );
    void       (*p_glUnmapTexture2DINTEL)( GLuint texture, GLint level );
    void       (*p_glUpdateObjectBufferATI)( GLuint buffer, GLuint offset, GLsizei size, const void *pointer, GLenum preserve );
    void       (*p_glUploadGpuMaskNVX)( GLbitfield mask );
    void       (*p_glUseProgram)( GLuint program );
    void       (*p_glUseProgramObjectARB)( GLhandleARB programObj );
    void       (*p_glUseProgramStages)( GLuint pipeline, GLbitfield stages, GLuint program );
    void       (*p_glUseShaderProgramEXT)( GLenum type, GLuint program );
    void       (*p_glVDPAUFiniNV)(void);
    void       (*p_glVDPAUGetSurfaceivNV)( GLvdpauSurfaceNV surface, GLenum pname, GLsizei count, GLsizei *length, GLint *values );
    void       (*p_glVDPAUInitNV)( const void *vdpDevice, const void *getProcAddress );
    GLboolean  (*p_glVDPAUIsSurfaceNV)( GLvdpauSurfaceNV surface );
    void       (*p_glVDPAUMapSurfacesNV)( GLsizei numSurfaces, const GLvdpauSurfaceNV *surfaces );
    GLvdpauSurfaceNV (*p_glVDPAURegisterOutputSurfaceNV)( const void *vdpSurface, GLenum target, GLsizei numTextureNames, const GLuint *textureNames );
    GLvdpauSurfaceNV (*p_glVDPAURegisterVideoSurfaceNV)( const void *vdpSurface, GLenum target, GLsizei numTextureNames, const GLuint *textureNames );
    GLvdpauSurfaceNV (*p_glVDPAURegisterVideoSurfaceWithPictureStructureNV)( const void *vdpSurface, GLenum target, GLsizei numTextureNames, const GLuint *textureNames, GLboolean isFrameStructure );
    void       (*p_glVDPAUSurfaceAccessNV)( GLvdpauSurfaceNV surface, GLenum access );
    void       (*p_glVDPAUUnmapSurfacesNV)( GLsizei numSurface, const GLvdpauSurfaceNV *surfaces );
    void       (*p_glVDPAUUnregisterSurfaceNV)( GLvdpauSurfaceNV surface );
    void       (*p_glValidateProgram)( GLuint program );
    void       (*p_glValidateProgramARB)( GLhandleARB programObj );
    void       (*p_glValidateProgramPipeline)( GLuint pipeline );
    void       (*p_glVariantArrayObjectATI)( GLuint id, GLenum type, GLsizei stride, GLuint buffer, GLuint offset );
    void       (*p_glVariantPointerEXT)( GLuint id, GLenum type, GLuint stride, const void *addr );
    void       (*p_glVariantbvEXT)( GLuint id, const GLbyte *addr );
    void       (*p_glVariantdvEXT)( GLuint id, const GLdouble *addr );
    void       (*p_glVariantfvEXT)( GLuint id, const GLfloat *addr );
    void       (*p_glVariantivEXT)( GLuint id, const GLint *addr );
    void       (*p_glVariantsvEXT)( GLuint id, const GLshort *addr );
    void       (*p_glVariantubvEXT)( GLuint id, const GLubyte *addr );
    void       (*p_glVariantuivEXT)( GLuint id, const GLuint *addr );
    void       (*p_glVariantusvEXT)( GLuint id, const GLushort *addr );
    void       (*p_glVertex2bOES)( GLbyte x, GLbyte y );
    void       (*p_glVertex2bvOES)( const GLbyte *coords );
    void       (*p_glVertex2hNV)( GLhalfNV x, GLhalfNV y );
    void       (*p_glVertex2hvNV)( const GLhalfNV *v );
    void       (*p_glVertex2xOES)( GLfixed x );
    void       (*p_glVertex2xvOES)( const GLfixed *coords );
    void       (*p_glVertex3bOES)( GLbyte x, GLbyte y, GLbyte z );
    void       (*p_glVertex3bvOES)( const GLbyte *coords );
    void       (*p_glVertex3hNV)( GLhalfNV x, GLhalfNV y, GLhalfNV z );
    void       (*p_glVertex3hvNV)( const GLhalfNV *v );
    void       (*p_glVertex3xOES)( GLfixed x, GLfixed y );
    void       (*p_glVertex3xvOES)( const GLfixed *coords );
    void       (*p_glVertex4bOES)( GLbyte x, GLbyte y, GLbyte z, GLbyte w );
    void       (*p_glVertex4bvOES)( const GLbyte *coords );
    void       (*p_glVertex4hNV)( GLhalfNV x, GLhalfNV y, GLhalfNV z, GLhalfNV w );
    void       (*p_glVertex4hvNV)( const GLhalfNV *v );
    void       (*p_glVertex4xOES)( GLfixed x, GLfixed y, GLfixed z );
    void       (*p_glVertex4xvOES)( const GLfixed *coords );
    void       (*p_glVertexArrayAttribBinding)( GLuint vaobj, GLuint attribindex, GLuint bindingindex );
    void       (*p_glVertexArrayAttribFormat)( GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset );
    void       (*p_glVertexArrayAttribIFormat)( GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset );
    void       (*p_glVertexArrayAttribLFormat)( GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset );
    void       (*p_glVertexArrayBindVertexBufferEXT)( GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride );
    void       (*p_glVertexArrayBindingDivisor)( GLuint vaobj, GLuint bindingindex, GLuint divisor );
    void       (*p_glVertexArrayColorOffsetEXT)( GLuint vaobj, GLuint buffer, GLint size, GLenum type, GLsizei stride, GLintptr offset );
    void       (*p_glVertexArrayEdgeFlagOffsetEXT)( GLuint vaobj, GLuint buffer, GLsizei stride, GLintptr offset );
    void       (*p_glVertexArrayElementBuffer)( GLuint vaobj, GLuint buffer );
    void       (*p_glVertexArrayFogCoordOffsetEXT)( GLuint vaobj, GLuint buffer, GLenum type, GLsizei stride, GLintptr offset );
    void       (*p_glVertexArrayIndexOffsetEXT)( GLuint vaobj, GLuint buffer, GLenum type, GLsizei stride, GLintptr offset );
    void       (*p_glVertexArrayMultiTexCoordOffsetEXT)( GLuint vaobj, GLuint buffer, GLenum texunit, GLint size, GLenum type, GLsizei stride, GLintptr offset );
    void       (*p_glVertexArrayNormalOffsetEXT)( GLuint vaobj, GLuint buffer, GLenum type, GLsizei stride, GLintptr offset );
    void       (*p_glVertexArrayParameteriAPPLE)( GLenum pname, GLint param );
    void       (*p_glVertexArrayRangeAPPLE)( GLsizei length, void *pointer );
    void       (*p_glVertexArrayRangeNV)( GLsizei length, const void *pointer );
    void       (*p_glVertexArraySecondaryColorOffsetEXT)( GLuint vaobj, GLuint buffer, GLint size, GLenum type, GLsizei stride, GLintptr offset );
    void       (*p_glVertexArrayTexCoordOffsetEXT)( GLuint vaobj, GLuint buffer, GLint size, GLenum type, GLsizei stride, GLintptr offset );
    void       (*p_glVertexArrayVertexAttribBindingEXT)( GLuint vaobj, GLuint attribindex, GLuint bindingindex );
    void       (*p_glVertexArrayVertexAttribDivisorEXT)( GLuint vaobj, GLuint index, GLuint divisor );
    void       (*p_glVertexArrayVertexAttribFormatEXT)( GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset );
    void       (*p_glVertexArrayVertexAttribIFormatEXT)( GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset );
    void       (*p_glVertexArrayVertexAttribIOffsetEXT)( GLuint vaobj, GLuint buffer, GLuint index, GLint size, GLenum type, GLsizei stride, GLintptr offset );
    void       (*p_glVertexArrayVertexAttribLFormatEXT)( GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset );
    void       (*p_glVertexArrayVertexAttribLOffsetEXT)( GLuint vaobj, GLuint buffer, GLuint index, GLint size, GLenum type, GLsizei stride, GLintptr offset );
    void       (*p_glVertexArrayVertexAttribOffsetEXT)( GLuint vaobj, GLuint buffer, GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, GLintptr offset );
    void       (*p_glVertexArrayVertexBindingDivisorEXT)( GLuint vaobj, GLuint bindingindex, GLuint divisor );
    void       (*p_glVertexArrayVertexBuffer)( GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride );
    void       (*p_glVertexArrayVertexBuffers)( GLuint vaobj, GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizei *strides );
    void       (*p_glVertexArrayVertexOffsetEXT)( GLuint vaobj, GLuint buffer, GLint size, GLenum type, GLsizei stride, GLintptr offset );
    void       (*p_glVertexAttrib1d)( GLuint index, GLdouble x );
    void       (*p_glVertexAttrib1dARB)( GLuint index, GLdouble x );
    void       (*p_glVertexAttrib1dNV)( GLuint index, GLdouble x );
    void       (*p_glVertexAttrib1dv)( GLuint index, const GLdouble *v );
    void       (*p_glVertexAttrib1dvARB)( GLuint index, const GLdouble *v );
    void       (*p_glVertexAttrib1dvNV)( GLuint index, const GLdouble *v );
    void       (*p_glVertexAttrib1f)( GLuint index, GLfloat x );
    void       (*p_glVertexAttrib1fARB)( GLuint index, GLfloat x );
    void       (*p_glVertexAttrib1fNV)( GLuint index, GLfloat x );
    void       (*p_glVertexAttrib1fv)( GLuint index, const GLfloat *v );
    void       (*p_glVertexAttrib1fvARB)( GLuint index, const GLfloat *v );
    void       (*p_glVertexAttrib1fvNV)( GLuint index, const GLfloat *v );
    void       (*p_glVertexAttrib1hNV)( GLuint index, GLhalfNV x );
    void       (*p_glVertexAttrib1hvNV)( GLuint index, const GLhalfNV *v );
    void       (*p_glVertexAttrib1s)( GLuint index, GLshort x );
    void       (*p_glVertexAttrib1sARB)( GLuint index, GLshort x );
    void       (*p_glVertexAttrib1sNV)( GLuint index, GLshort x );
    void       (*p_glVertexAttrib1sv)( GLuint index, const GLshort *v );
    void       (*p_glVertexAttrib1svARB)( GLuint index, const GLshort *v );
    void       (*p_glVertexAttrib1svNV)( GLuint index, const GLshort *v );
    void       (*p_glVertexAttrib2d)( GLuint index, GLdouble x, GLdouble y );
    void       (*p_glVertexAttrib2dARB)( GLuint index, GLdouble x, GLdouble y );
    void       (*p_glVertexAttrib2dNV)( GLuint index, GLdouble x, GLdouble y );
    void       (*p_glVertexAttrib2dv)( GLuint index, const GLdouble *v );
    void       (*p_glVertexAttrib2dvARB)( GLuint index, const GLdouble *v );
    void       (*p_glVertexAttrib2dvNV)( GLuint index, const GLdouble *v );
    void       (*p_glVertexAttrib2f)( GLuint index, GLfloat x, GLfloat y );
    void       (*p_glVertexAttrib2fARB)( GLuint index, GLfloat x, GLfloat y );
    void       (*p_glVertexAttrib2fNV)( GLuint index, GLfloat x, GLfloat y );
    void       (*p_glVertexAttrib2fv)( GLuint index, const GLfloat *v );
    void       (*p_glVertexAttrib2fvARB)( GLuint index, const GLfloat *v );
    void       (*p_glVertexAttrib2fvNV)( GLuint index, const GLfloat *v );
    void       (*p_glVertexAttrib2hNV)( GLuint index, GLhalfNV x, GLhalfNV y );
    void       (*p_glVertexAttrib2hvNV)( GLuint index, const GLhalfNV *v );
    void       (*p_glVertexAttrib2s)( GLuint index, GLshort x, GLshort y );
    void       (*p_glVertexAttrib2sARB)( GLuint index, GLshort x, GLshort y );
    void       (*p_glVertexAttrib2sNV)( GLuint index, GLshort x, GLshort y );
    void       (*p_glVertexAttrib2sv)( GLuint index, const GLshort *v );
    void       (*p_glVertexAttrib2svARB)( GLuint index, const GLshort *v );
    void       (*p_glVertexAttrib2svNV)( GLuint index, const GLshort *v );
    void       (*p_glVertexAttrib3d)( GLuint index, GLdouble x, GLdouble y, GLdouble z );
    void       (*p_glVertexAttrib3dARB)( GLuint index, GLdouble x, GLdouble y, GLdouble z );
    void       (*p_glVertexAttrib3dNV)( GLuint index, GLdouble x, GLdouble y, GLdouble z );
    void       (*p_glVertexAttrib3dv)( GLuint index, const GLdouble *v );
    void       (*p_glVertexAttrib3dvARB)( GLuint index, const GLdouble *v );
    void       (*p_glVertexAttrib3dvNV)( GLuint index, const GLdouble *v );
    void       (*p_glVertexAttrib3f)( GLuint index, GLfloat x, GLfloat y, GLfloat z );
    void       (*p_glVertexAttrib3fARB)( GLuint index, GLfloat x, GLfloat y, GLfloat z );
    void       (*p_glVertexAttrib3fNV)( GLuint index, GLfloat x, GLfloat y, GLfloat z );
    void       (*p_glVertexAttrib3fv)( GLuint index, const GLfloat *v );
    void       (*p_glVertexAttrib3fvARB)( GLuint index, const GLfloat *v );
    void       (*p_glVertexAttrib3fvNV)( GLuint index, const GLfloat *v );
    void       (*p_glVertexAttrib3hNV)( GLuint index, GLhalfNV x, GLhalfNV y, GLhalfNV z );
    void       (*p_glVertexAttrib3hvNV)( GLuint index, const GLhalfNV *v );
    void       (*p_glVertexAttrib3s)( GLuint index, GLshort x, GLshort y, GLshort z );
    void       (*p_glVertexAttrib3sARB)( GLuint index, GLshort x, GLshort y, GLshort z );
    void       (*p_glVertexAttrib3sNV)( GLuint index, GLshort x, GLshort y, GLshort z );
    void       (*p_glVertexAttrib3sv)( GLuint index, const GLshort *v );
    void       (*p_glVertexAttrib3svARB)( GLuint index, const GLshort *v );
    void       (*p_glVertexAttrib3svNV)( GLuint index, const GLshort *v );
    void       (*p_glVertexAttrib4Nbv)( GLuint index, const GLbyte *v );
    void       (*p_glVertexAttrib4NbvARB)( GLuint index, const GLbyte *v );
    void       (*p_glVertexAttrib4Niv)( GLuint index, const GLint *v );
    void       (*p_glVertexAttrib4NivARB)( GLuint index, const GLint *v );
    void       (*p_glVertexAttrib4Nsv)( GLuint index, const GLshort *v );
    void       (*p_glVertexAttrib4NsvARB)( GLuint index, const GLshort *v );
    void       (*p_glVertexAttrib4Nub)( GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w );
    void       (*p_glVertexAttrib4NubARB)( GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w );
    void       (*p_glVertexAttrib4Nubv)( GLuint index, const GLubyte *v );
    void       (*p_glVertexAttrib4NubvARB)( GLuint index, const GLubyte *v );
    void       (*p_glVertexAttrib4Nuiv)( GLuint index, const GLuint *v );
    void       (*p_glVertexAttrib4NuivARB)( GLuint index, const GLuint *v );
    void       (*p_glVertexAttrib4Nusv)( GLuint index, const GLushort *v );
    void       (*p_glVertexAttrib4NusvARB)( GLuint index, const GLushort *v );
    void       (*p_glVertexAttrib4bv)( GLuint index, const GLbyte *v );
    void       (*p_glVertexAttrib4bvARB)( GLuint index, const GLbyte *v );
    void       (*p_glVertexAttrib4d)( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
    void       (*p_glVertexAttrib4dARB)( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
    void       (*p_glVertexAttrib4dNV)( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
    void       (*p_glVertexAttrib4dv)( GLuint index, const GLdouble *v );
    void       (*p_glVertexAttrib4dvARB)( GLuint index, const GLdouble *v );
    void       (*p_glVertexAttrib4dvNV)( GLuint index, const GLdouble *v );
    void       (*p_glVertexAttrib4f)( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
    void       (*p_glVertexAttrib4fARB)( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
    void       (*p_glVertexAttrib4fNV)( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
    void       (*p_glVertexAttrib4fv)( GLuint index, const GLfloat *v );
    void       (*p_glVertexAttrib4fvARB)( GLuint index, const GLfloat *v );
    void       (*p_glVertexAttrib4fvNV)( GLuint index, const GLfloat *v );
    void       (*p_glVertexAttrib4hNV)( GLuint index, GLhalfNV x, GLhalfNV y, GLhalfNV z, GLhalfNV w );
    void       (*p_glVertexAttrib4hvNV)( GLuint index, const GLhalfNV *v );
    void       (*p_glVertexAttrib4iv)( GLuint index, const GLint *v );
    void       (*p_glVertexAttrib4ivARB)( GLuint index, const GLint *v );
    void       (*p_glVertexAttrib4s)( GLuint index, GLshort x, GLshort y, GLshort z, GLshort w );
    void       (*p_glVertexAttrib4sARB)( GLuint index, GLshort x, GLshort y, GLshort z, GLshort w );
    void       (*p_glVertexAttrib4sNV)( GLuint index, GLshort x, GLshort y, GLshort z, GLshort w );
    void       (*p_glVertexAttrib4sv)( GLuint index, const GLshort *v );
    void       (*p_glVertexAttrib4svARB)( GLuint index, const GLshort *v );
    void       (*p_glVertexAttrib4svNV)( GLuint index, const GLshort *v );
    void       (*p_glVertexAttrib4ubNV)( GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w );
    void       (*p_glVertexAttrib4ubv)( GLuint index, const GLubyte *v );
    void       (*p_glVertexAttrib4ubvARB)( GLuint index, const GLubyte *v );
    void       (*p_glVertexAttrib4ubvNV)( GLuint index, const GLubyte *v );
    void       (*p_glVertexAttrib4uiv)( GLuint index, const GLuint *v );
    void       (*p_glVertexAttrib4uivARB)( GLuint index, const GLuint *v );
    void       (*p_glVertexAttrib4usv)( GLuint index, const GLushort *v );
    void       (*p_glVertexAttrib4usvARB)( GLuint index, const GLushort *v );
    void       (*p_glVertexAttribArrayObjectATI)( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, GLuint buffer, GLuint offset );
    void       (*p_glVertexAttribBinding)( GLuint attribindex, GLuint bindingindex );
    void       (*p_glVertexAttribDivisor)( GLuint index, GLuint divisor );
    void       (*p_glVertexAttribDivisorARB)( GLuint index, GLuint divisor );
    void       (*p_glVertexAttribFormat)( GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset );
    void       (*p_glVertexAttribFormatNV)( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride );
    void       (*p_glVertexAttribI1i)( GLuint index, GLint x );
    void       (*p_glVertexAttribI1iEXT)( GLuint index, GLint x );
    void       (*p_glVertexAttribI1iv)( GLuint index, const GLint *v );
    void       (*p_glVertexAttribI1ivEXT)( GLuint index, const GLint *v );
    void       (*p_glVertexAttribI1ui)( GLuint index, GLuint x );
    void       (*p_glVertexAttribI1uiEXT)( GLuint index, GLuint x );
    void       (*p_glVertexAttribI1uiv)( GLuint index, const GLuint *v );
    void       (*p_glVertexAttribI1uivEXT)( GLuint index, const GLuint *v );
    void       (*p_glVertexAttribI2i)( GLuint index, GLint x, GLint y );
    void       (*p_glVertexAttribI2iEXT)( GLuint index, GLint x, GLint y );
    void       (*p_glVertexAttribI2iv)( GLuint index, const GLint *v );
    void       (*p_glVertexAttribI2ivEXT)( GLuint index, const GLint *v );
    void       (*p_glVertexAttribI2ui)( GLuint index, GLuint x, GLuint y );
    void       (*p_glVertexAttribI2uiEXT)( GLuint index, GLuint x, GLuint y );
    void       (*p_glVertexAttribI2uiv)( GLuint index, const GLuint *v );
    void       (*p_glVertexAttribI2uivEXT)( GLuint index, const GLuint *v );
    void       (*p_glVertexAttribI3i)( GLuint index, GLint x, GLint y, GLint z );
    void       (*p_glVertexAttribI3iEXT)( GLuint index, GLint x, GLint y, GLint z );
    void       (*p_glVertexAttribI3iv)( GLuint index, const GLint *v );
    void       (*p_glVertexAttribI3ivEXT)( GLuint index, const GLint *v );
    void       (*p_glVertexAttribI3ui)( GLuint index, GLuint x, GLuint y, GLuint z );
    void       (*p_glVertexAttribI3uiEXT)( GLuint index, GLuint x, GLuint y, GLuint z );
    void       (*p_glVertexAttribI3uiv)( GLuint index, const GLuint *v );
    void       (*p_glVertexAttribI3uivEXT)( GLuint index, const GLuint *v );
    void       (*p_glVertexAttribI4bv)( GLuint index, const GLbyte *v );
    void       (*p_glVertexAttribI4bvEXT)( GLuint index, const GLbyte *v );
    void       (*p_glVertexAttribI4i)( GLuint index, GLint x, GLint y, GLint z, GLint w );
    void       (*p_glVertexAttribI4iEXT)( GLuint index, GLint x, GLint y, GLint z, GLint w );
    void       (*p_glVertexAttribI4iv)( GLuint index, const GLint *v );
    void       (*p_glVertexAttribI4ivEXT)( GLuint index, const GLint *v );
    void       (*p_glVertexAttribI4sv)( GLuint index, const GLshort *v );
    void       (*p_glVertexAttribI4svEXT)( GLuint index, const GLshort *v );
    void       (*p_glVertexAttribI4ubv)( GLuint index, const GLubyte *v );
    void       (*p_glVertexAttribI4ubvEXT)( GLuint index, const GLubyte *v );
    void       (*p_glVertexAttribI4ui)( GLuint index, GLuint x, GLuint y, GLuint z, GLuint w );
    void       (*p_glVertexAttribI4uiEXT)( GLuint index, GLuint x, GLuint y, GLuint z, GLuint w );
    void       (*p_glVertexAttribI4uiv)( GLuint index, const GLuint *v );
    void       (*p_glVertexAttribI4uivEXT)( GLuint index, const GLuint *v );
    void       (*p_glVertexAttribI4usv)( GLuint index, const GLushort *v );
    void       (*p_glVertexAttribI4usvEXT)( GLuint index, const GLushort *v );
    void       (*p_glVertexAttribIFormat)( GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset );
    void       (*p_glVertexAttribIFormatNV)( GLuint index, GLint size, GLenum type, GLsizei stride );
    void       (*p_glVertexAttribIPointer)( GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer );
    void       (*p_glVertexAttribIPointerEXT)( GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer );
    void       (*p_glVertexAttribL1d)( GLuint index, GLdouble x );
    void       (*p_glVertexAttribL1dEXT)( GLuint index, GLdouble x );
    void       (*p_glVertexAttribL1dv)( GLuint index, const GLdouble *v );
    void       (*p_glVertexAttribL1dvEXT)( GLuint index, const GLdouble *v );
    void       (*p_glVertexAttribL1i64NV)( GLuint index, GLint64EXT x );
    void       (*p_glVertexAttribL1i64vNV)( GLuint index, const GLint64EXT *v );
    void       (*p_glVertexAttribL1ui64ARB)( GLuint index, GLuint64EXT x );
    void       (*p_glVertexAttribL1ui64NV)( GLuint index, GLuint64EXT x );
    void       (*p_glVertexAttribL1ui64vARB)( GLuint index, const GLuint64EXT *v );
    void       (*p_glVertexAttribL1ui64vNV)( GLuint index, const GLuint64EXT *v );
    void       (*p_glVertexAttribL2d)( GLuint index, GLdouble x, GLdouble y );
    void       (*p_glVertexAttribL2dEXT)( GLuint index, GLdouble x, GLdouble y );
    void       (*p_glVertexAttribL2dv)( GLuint index, const GLdouble *v );
    void       (*p_glVertexAttribL2dvEXT)( GLuint index, const GLdouble *v );
    void       (*p_glVertexAttribL2i64NV)( GLuint index, GLint64EXT x, GLint64EXT y );
    void       (*p_glVertexAttribL2i64vNV)( GLuint index, const GLint64EXT *v );
    void       (*p_glVertexAttribL2ui64NV)( GLuint index, GLuint64EXT x, GLuint64EXT y );
    void       (*p_glVertexAttribL2ui64vNV)( GLuint index, const GLuint64EXT *v );
    void       (*p_glVertexAttribL3d)( GLuint index, GLdouble x, GLdouble y, GLdouble z );
    void       (*p_glVertexAttribL3dEXT)( GLuint index, GLdouble x, GLdouble y, GLdouble z );
    void       (*p_glVertexAttribL3dv)( GLuint index, const GLdouble *v );
    void       (*p_glVertexAttribL3dvEXT)( GLuint index, const GLdouble *v );
    void       (*p_glVertexAttribL3i64NV)( GLuint index, GLint64EXT x, GLint64EXT y, GLint64EXT z );
    void       (*p_glVertexAttribL3i64vNV)( GLuint index, const GLint64EXT *v );
    void       (*p_glVertexAttribL3ui64NV)( GLuint index, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z );
    void       (*p_glVertexAttribL3ui64vNV)( GLuint index, const GLuint64EXT *v );
    void       (*p_glVertexAttribL4d)( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
    void       (*p_glVertexAttribL4dEXT)( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
    void       (*p_glVertexAttribL4dv)( GLuint index, const GLdouble *v );
    void       (*p_glVertexAttribL4dvEXT)( GLuint index, const GLdouble *v );
    void       (*p_glVertexAttribL4i64NV)( GLuint index, GLint64EXT x, GLint64EXT y, GLint64EXT z, GLint64EXT w );
    void       (*p_glVertexAttribL4i64vNV)( GLuint index, const GLint64EXT *v );
    void       (*p_glVertexAttribL4ui64NV)( GLuint index, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z, GLuint64EXT w );
    void       (*p_glVertexAttribL4ui64vNV)( GLuint index, const GLuint64EXT *v );
    void       (*p_glVertexAttribLFormat)( GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset );
    void       (*p_glVertexAttribLFormatNV)( GLuint index, GLint size, GLenum type, GLsizei stride );
    void       (*p_glVertexAttribLPointer)( GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer );
    void       (*p_glVertexAttribLPointerEXT)( GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer );
    void       (*p_glVertexAttribP1ui)( GLuint index, GLenum type, GLboolean normalized, GLuint value );
    void       (*p_glVertexAttribP1uiv)( GLuint index, GLenum type, GLboolean normalized, const GLuint *value );
    void       (*p_glVertexAttribP2ui)( GLuint index, GLenum type, GLboolean normalized, GLuint value );
    void       (*p_glVertexAttribP2uiv)( GLuint index, GLenum type, GLboolean normalized, const GLuint *value );
    void       (*p_glVertexAttribP3ui)( GLuint index, GLenum type, GLboolean normalized, GLuint value );
    void       (*p_glVertexAttribP3uiv)( GLuint index, GLenum type, GLboolean normalized, const GLuint *value );
    void       (*p_glVertexAttribP4ui)( GLuint index, GLenum type, GLboolean normalized, GLuint value );
    void       (*p_glVertexAttribP4uiv)( GLuint index, GLenum type, GLboolean normalized, const GLuint *value );
    void       (*p_glVertexAttribParameteriAMD)( GLuint index, GLenum pname, GLint param );
    void       (*p_glVertexAttribPointer)( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer );
    void       (*p_glVertexAttribPointerARB)( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer );
    void       (*p_glVertexAttribPointerNV)( GLuint index, GLint fsize, GLenum type, GLsizei stride, const void *pointer );
    void       (*p_glVertexAttribs1dvNV)( GLuint index, GLsizei count, const GLdouble *v );
    void       (*p_glVertexAttribs1fvNV)( GLuint index, GLsizei count, const GLfloat *v );
    void       (*p_glVertexAttribs1hvNV)( GLuint index, GLsizei n, const GLhalfNV *v );
    void       (*p_glVertexAttribs1svNV)( GLuint index, GLsizei count, const GLshort *v );
    void       (*p_glVertexAttribs2dvNV)( GLuint index, GLsizei count, const GLdouble *v );
    void       (*p_glVertexAttribs2fvNV)( GLuint index, GLsizei count, const GLfloat *v );
    void       (*p_glVertexAttribs2hvNV)( GLuint index, GLsizei n, const GLhalfNV *v );
    void       (*p_glVertexAttribs2svNV)( GLuint index, GLsizei count, const GLshort *v );
    void       (*p_glVertexAttribs3dvNV)( GLuint index, GLsizei count, const GLdouble *v );
    void       (*p_glVertexAttribs3fvNV)( GLuint index, GLsizei count, const GLfloat *v );
    void       (*p_glVertexAttribs3hvNV)( GLuint index, GLsizei n, const GLhalfNV *v );
    void       (*p_glVertexAttribs3svNV)( GLuint index, GLsizei count, const GLshort *v );
    void       (*p_glVertexAttribs4dvNV)( GLuint index, GLsizei count, const GLdouble *v );
    void       (*p_glVertexAttribs4fvNV)( GLuint index, GLsizei count, const GLfloat *v );
    void       (*p_glVertexAttribs4hvNV)( GLuint index, GLsizei n, const GLhalfNV *v );
    void       (*p_glVertexAttribs4svNV)( GLuint index, GLsizei count, const GLshort *v );
    void       (*p_glVertexAttribs4ubvNV)( GLuint index, GLsizei count, const GLubyte *v );
    void       (*p_glVertexBindingDivisor)( GLuint bindingindex, GLuint divisor );
    void       (*p_glVertexBlendARB)( GLint count );
    void       (*p_glVertexBlendEnvfATI)( GLenum pname, GLfloat param );
    void       (*p_glVertexBlendEnviATI)( GLenum pname, GLint param );
    void       (*p_glVertexFormatNV)( GLint size, GLenum type, GLsizei stride );
    void       (*p_glVertexP2ui)( GLenum type, GLuint value );
    void       (*p_glVertexP2uiv)( GLenum type, const GLuint *value );
    void       (*p_glVertexP3ui)( GLenum type, GLuint value );
    void       (*p_glVertexP3uiv)( GLenum type, const GLuint *value );
    void       (*p_glVertexP4ui)( GLenum type, GLuint value );
    void       (*p_glVertexP4uiv)( GLenum type, const GLuint *value );
    void       (*p_glVertexPointerEXT)( GLint size, GLenum type, GLsizei stride, GLsizei count, const void *pointer );
    void       (*p_glVertexPointerListIBM)( GLint size, GLenum type, GLint stride, const void **pointer, GLint ptrstride );
    void       (*p_glVertexPointervINTEL)( GLint size, GLenum type, const void **pointer );
    void       (*p_glVertexStream1dATI)( GLenum stream, GLdouble x );
    void       (*p_glVertexStream1dvATI)( GLenum stream, const GLdouble *coords );
    void       (*p_glVertexStream1fATI)( GLenum stream, GLfloat x );
    void       (*p_glVertexStream1fvATI)( GLenum stream, const GLfloat *coords );
    void       (*p_glVertexStream1iATI)( GLenum stream, GLint x );
    void       (*p_glVertexStream1ivATI)( GLenum stream, const GLint *coords );
    void       (*p_glVertexStream1sATI)( GLenum stream, GLshort x );
    void       (*p_glVertexStream1svATI)( GLenum stream, const GLshort *coords );
    void       (*p_glVertexStream2dATI)( GLenum stream, GLdouble x, GLdouble y );
    void       (*p_glVertexStream2dvATI)( GLenum stream, const GLdouble *coords );
    void       (*p_glVertexStream2fATI)( GLenum stream, GLfloat x, GLfloat y );
    void       (*p_glVertexStream2fvATI)( GLenum stream, const GLfloat *coords );
    void       (*p_glVertexStream2iATI)( GLenum stream, GLint x, GLint y );
    void       (*p_glVertexStream2ivATI)( GLenum stream, const GLint *coords );
    void       (*p_glVertexStream2sATI)( GLenum stream, GLshort x, GLshort y );
    void       (*p_glVertexStream2svATI)( GLenum stream, const GLshort *coords );
    void       (*p_glVertexStream3dATI)( GLenum stream, GLdouble x, GLdouble y, GLdouble z );
    void       (*p_glVertexStream3dvATI)( GLenum stream, const GLdouble *coords );
    void       (*p_glVertexStream3fATI)( GLenum stream, GLfloat x, GLfloat y, GLfloat z );
    void       (*p_glVertexStream3fvATI)( GLenum stream, const GLfloat *coords );
    void       (*p_glVertexStream3iATI)( GLenum stream, GLint x, GLint y, GLint z );
    void       (*p_glVertexStream3ivATI)( GLenum stream, const GLint *coords );
    void       (*p_glVertexStream3sATI)( GLenum stream, GLshort x, GLshort y, GLshort z );
    void       (*p_glVertexStream3svATI)( GLenum stream, const GLshort *coords );
    void       (*p_glVertexStream4dATI)( GLenum stream, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
    void       (*p_glVertexStream4dvATI)( GLenum stream, const GLdouble *coords );
    void       (*p_glVertexStream4fATI)( GLenum stream, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
    void       (*p_glVertexStream4fvATI)( GLenum stream, const GLfloat *coords );
    void       (*p_glVertexStream4iATI)( GLenum stream, GLint x, GLint y, GLint z, GLint w );
    void       (*p_glVertexStream4ivATI)( GLenum stream, const GLint *coords );
    void       (*p_glVertexStream4sATI)( GLenum stream, GLshort x, GLshort y, GLshort z, GLshort w );
    void       (*p_glVertexStream4svATI)( GLenum stream, const GLshort *coords );
    void       (*p_glVertexWeightPointerEXT)( GLint size, GLenum type, GLsizei stride, const void *pointer );
    void       (*p_glVertexWeightfEXT)( GLfloat weight );
    void       (*p_glVertexWeightfvEXT)( const GLfloat *weight );
    void       (*p_glVertexWeighthNV)( GLhalfNV weight );
    void       (*p_glVertexWeighthvNV)( const GLhalfNV *weight );
    GLenum     (*p_glVideoCaptureNV)( GLuint video_capture_slot, GLuint *sequence_num, GLuint64EXT *capture_time );
    void       (*p_glVideoCaptureStreamParameterdvNV)( GLuint video_capture_slot, GLuint stream, GLenum pname, const GLdouble *params );
    void       (*p_glVideoCaptureStreamParameterfvNV)( GLuint video_capture_slot, GLuint stream, GLenum pname, const GLfloat *params );
    void       (*p_glVideoCaptureStreamParameterivNV)( GLuint video_capture_slot, GLuint stream, GLenum pname, const GLint *params );
    void       (*p_glViewportArrayv)( GLuint first, GLsizei count, const GLfloat *v );
    void       (*p_glViewportIndexedf)( GLuint index, GLfloat x, GLfloat y, GLfloat w, GLfloat h );
    void       (*p_glViewportIndexedfv)( GLuint index, const GLfloat *v );
    void       (*p_glViewportPositionWScaleNV)( GLuint index, GLfloat xcoeff, GLfloat ycoeff );
    void       (*p_glViewportSwizzleNV)( GLuint index, GLenum swizzlex, GLenum swizzley, GLenum swizzlez, GLenum swizzlew );
    void       (*p_glWaitSemaphoreEXT)( GLuint semaphore, GLuint numBufferBarriers, const GLuint *buffers, GLuint numTextureBarriers, const GLuint *textures, const GLenum *srcLayouts );
    void       (*p_glWaitSemaphoreui64NVX)( GLuint waitGpu, GLsizei fenceObjectCount, const GLuint *semaphoreArray, const GLuint64 *fenceValueArray );
    void       (*p_glWaitSync)( GLsync sync, GLbitfield flags, GLuint64 timeout );
    void       (*p_glWaitVkSemaphoreNV)( GLuint64 vkSemaphore );
    void       (*p_glWeightPathsNV)( GLuint resultPath, GLsizei numPaths, const GLuint *paths, const GLfloat *weights );
    void       (*p_glWeightPointerARB)( GLint size, GLenum type, GLsizei stride, const void *pointer );
    void       (*p_glWeightbvARB)( GLint size, const GLbyte *weights );
    void       (*p_glWeightdvARB)( GLint size, const GLdouble *weights );
    void       (*p_glWeightfvARB)( GLint size, const GLfloat *weights );
    void       (*p_glWeightivARB)( GLint size, const GLint *weights );
    void       (*p_glWeightsvARB)( GLint size, const GLshort *weights );
    void       (*p_glWeightubvARB)( GLint size, const GLubyte *weights );
    void       (*p_glWeightuivARB)( GLint size, const GLuint *weights );
    void       (*p_glWeightusvARB)( GLint size, const GLushort *weights );
    void       (*p_glWindowPos2d)( GLdouble x, GLdouble y );
    void       (*p_glWindowPos2dARB)( GLdouble x, GLdouble y );
    void       (*p_glWindowPos2dMESA)( GLdouble x, GLdouble y );
    void       (*p_glWindowPos2dv)( const GLdouble *v );
    void       (*p_glWindowPos2dvARB)( const GLdouble *v );
    void       (*p_glWindowPos2dvMESA)( const GLdouble *v );
    void       (*p_glWindowPos2f)( GLfloat x, GLfloat y );
    void       (*p_glWindowPos2fARB)( GLfloat x, GLfloat y );
    void       (*p_glWindowPos2fMESA)( GLfloat x, GLfloat y );
    void       (*p_glWindowPos2fv)( const GLfloat *v );
    void       (*p_glWindowPos2fvARB)( const GLfloat *v );
    void       (*p_glWindowPos2fvMESA)( const GLfloat *v );
    void       (*p_glWindowPos2i)( GLint x, GLint y );
    void       (*p_glWindowPos2iARB)( GLint x, GLint y );
    void       (*p_glWindowPos2iMESA)( GLint x, GLint y );
    void       (*p_glWindowPos2iv)( const GLint *v );
    void       (*p_glWindowPos2ivARB)( const GLint *v );
    void       (*p_glWindowPos2ivMESA)( const GLint *v );
    void       (*p_glWindowPos2s)( GLshort x, GLshort y );
    void       (*p_glWindowPos2sARB)( GLshort x, GLshort y );
    void       (*p_glWindowPos2sMESA)( GLshort x, GLshort y );
    void       (*p_glWindowPos2sv)( const GLshort *v );
    void       (*p_glWindowPos2svARB)( const GLshort *v );
    void       (*p_glWindowPos2svMESA)( const GLshort *v );
    void       (*p_glWindowPos3d)( GLdouble x, GLdouble y, GLdouble z );
    void       (*p_glWindowPos3dARB)( GLdouble x, GLdouble y, GLdouble z );
    void       (*p_glWindowPos3dMESA)( GLdouble x, GLdouble y, GLdouble z );
    void       (*p_glWindowPos3dv)( const GLdouble *v );
    void       (*p_glWindowPos3dvARB)( const GLdouble *v );
    void       (*p_glWindowPos3dvMESA)( const GLdouble *v );
    void       (*p_glWindowPos3f)( GLfloat x, GLfloat y, GLfloat z );
    void       (*p_glWindowPos3fARB)( GLfloat x, GLfloat y, GLfloat z );
    void       (*p_glWindowPos3fMESA)( GLfloat x, GLfloat y, GLfloat z );
    void       (*p_glWindowPos3fv)( const GLfloat *v );
    void       (*p_glWindowPos3fvARB)( const GLfloat *v );
    void       (*p_glWindowPos3fvMESA)( const GLfloat *v );
    void       (*p_glWindowPos3i)( GLint x, GLint y, GLint z );
    void       (*p_glWindowPos3iARB)( GLint x, GLint y, GLint z );
    void       (*p_glWindowPos3iMESA)( GLint x, GLint y, GLint z );
    void       (*p_glWindowPos3iv)( const GLint *v );
    void       (*p_glWindowPos3ivARB)( const GLint *v );
    void       (*p_glWindowPos3ivMESA)( const GLint *v );
    void       (*p_glWindowPos3s)( GLshort x, GLshort y, GLshort z );
    void       (*p_glWindowPos3sARB)( GLshort x, GLshort y, GLshort z );
    void       (*p_glWindowPos3sMESA)( GLshort x, GLshort y, GLshort z );
    void       (*p_glWindowPos3sv)( const GLshort *v );
    void       (*p_glWindowPos3svARB)( const GLshort *v );
    void       (*p_glWindowPos3svMESA)( const GLshort *v );
    void       (*p_glWindowPos4dMESA)( GLdouble x, GLdouble y, GLdouble z, GLdouble w );
    void       (*p_glWindowPos4dvMESA)( const GLdouble *v );
    void       (*p_glWindowPos4fMESA)( GLfloat x, GLfloat y, GLfloat z, GLfloat w );
    void       (*p_glWindowPos4fvMESA)( const GLfloat *v );
    void       (*p_glWindowPos4iMESA)( GLint x, GLint y, GLint z, GLint w );
    void       (*p_glWindowPos4ivMESA)( const GLint *v );
    void       (*p_glWindowPos4sMESA)( GLshort x, GLshort y, GLshort z, GLshort w );
    void       (*p_glWindowPos4svMESA)( const GLshort *v );
    void       (*p_glWindowRectanglesEXT)( GLenum mode, GLsizei count, const GLint *box );
    void       (*p_glWriteMaskEXT)( GLuint res, GLuint in, GLenum outX, GLenum outY, GLenum outZ, GLenum outW );
    void *     (*p_wglAllocateMemoryNV)( GLsizei size, GLfloat readfreq, GLfloat writefreq, GLfloat priority );
    BOOL       (*p_wglBindTexImageARB)( struct wgl_pbuffer * hPbuffer, int iBuffer );
    BOOL       (*p_wglChoosePixelFormatARB)( HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats );
    struct wgl_context * (*p_wglCreateContextAttribsARB)( HDC hDC, struct wgl_context * hShareContext, const int *attribList );
    struct wgl_pbuffer * (*p_wglCreatePbufferARB)( HDC hDC, int iPixelFormat, int iWidth, int iHeight, const int *piAttribList );
    BOOL       (*p_wglDestroyPbufferARB)( struct wgl_pbuffer * hPbuffer );
    void       (*p_wglFreeMemoryNV)( void *pointer );
    HDC        (*p_wglGetCurrentReadDCARB)(void);
    const char * (*p_wglGetExtensionsStringARB)( HDC hdc );
    const char * (*p_wglGetExtensionsStringEXT)(void);
    HDC        (*p_wglGetPbufferDCARB)( struct wgl_pbuffer * hPbuffer );
    BOOL       (*p_wglGetPixelFormatAttribfvARB)( HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, FLOAT *pfValues );
    BOOL       (*p_wglGetPixelFormatAttribivARB)( HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, int *piValues );
    int        (*p_wglGetSwapIntervalEXT)(void);
    BOOL       (*p_wglMakeContextCurrentARB)( HDC hDrawDC, HDC hReadDC, struct wgl_context * hglrc );
    BOOL       (*p_wglQueryCurrentRendererIntegerWINE)( GLenum attribute, GLuint *value );
    const GLchar * (*p_wglQueryCurrentRendererStringWINE)( GLenum attribute );
    BOOL       (*p_wglQueryPbufferARB)( struct wgl_pbuffer * hPbuffer, int iAttribute, int *piValue );
    BOOL       (*p_wglQueryRendererIntegerWINE)( HDC dc, GLint renderer, GLenum attribute, GLuint *value );
    const GLchar * (*p_wglQueryRendererStringWINE)( HDC dc, GLint renderer, GLenum attribute );
    int        (*p_wglReleasePbufferDCARB)( struct wgl_pbuffer * hPbuffer, HDC hDC );
    BOOL       (*p_wglReleaseTexImageARB)( struct wgl_pbuffer * hPbuffer, int iBuffer );
    BOOL       (*p_wglSetPbufferAttribARB)( struct wgl_pbuffer * hPbuffer, const int *piAttribList );
    BOOL       (*p_wglSetPixelFormatWINE)( HDC hdc, int format );
    BOOL       (*p_wglSwapIntervalEXT)( int interval );
};

#define ALL_GL_UNIX_FUNCS \
    USE_GL_FUNC(glAccum) \
    USE_GL_FUNC(glAlphaFunc) \
    USE_GL_FUNC(glAreTexturesResident) \
    USE_GL_FUNC(glArrayElement) \
    USE_GL_FUNC(glBegin) \
    USE_GL_FUNC(glBindTexture) \
    USE_GL_FUNC(glBitmap) \
    USE_GL_FUNC(glBlendFunc) \
    USE_GL_FUNC(glCallList) \
    USE_GL_FUNC(glCallLists) \
    USE_GL_FUNC(glClear) \
    USE_GL_FUNC(glClearAccum) \
    USE_GL_FUNC(glClearColor) \
    USE_GL_FUNC(glClearDepth) \
    USE_GL_FUNC(glClearIndex) \
    USE_GL_FUNC(glClearStencil) \
    USE_GL_FUNC(glClipPlane) \
    USE_GL_FUNC(glColor3b) \
    USE_GL_FUNC(glColor3bv) \
    USE_GL_FUNC(glColor3d) \
    USE_GL_FUNC(glColor3dv) \
    USE_GL_FUNC(glColor3f) \
    USE_GL_FUNC(glColor3fv) \
    USE_GL_FUNC(glColor3i) \
    USE_GL_FUNC(glColor3iv) \
    USE_GL_FUNC(glColor3s) \
    USE_GL_FUNC(glColor3sv) \
    USE_GL_FUNC(glColor3ub) \
    USE_GL_FUNC(glColor3ubv) \
    USE_GL_FUNC(glColor3ui) \
    USE_GL_FUNC(glColor3uiv) \
    USE_GL_FUNC(glColor3us) \
    USE_GL_FUNC(glColor3usv) \
    USE_GL_FUNC(glColor4b) \
    USE_GL_FUNC(glColor4bv) \
    USE_GL_FUNC(glColor4d) \
    USE_GL_FUNC(glColor4dv) \
    USE_GL_FUNC(glColor4f) \
    USE_GL_FUNC(glColor4fv) \
    USE_GL_FUNC(glColor4i) \
    USE_GL_FUNC(glColor4iv) \
    USE_GL_FUNC(glColor4s) \
    USE_GL_FUNC(glColor4sv) \
    USE_GL_FUNC(glColor4ub) \
    USE_GL_FUNC(glColor4ubv) \
    USE_GL_FUNC(glColor4ui) \
    USE_GL_FUNC(glColor4uiv) \
    USE_GL_FUNC(glColor4us) \
    USE_GL_FUNC(glColor4usv) \
    USE_GL_FUNC(glColorMask) \
    USE_GL_FUNC(glColorMaterial) \
    USE_GL_FUNC(glColorPointer) \
    USE_GL_FUNC(glCopyPixels) \
    USE_GL_FUNC(glCopyTexImage1D) \
    USE_GL_FUNC(glCopyTexImage2D) \
    USE_GL_FUNC(glCopyTexSubImage1D) \
    USE_GL_FUNC(glCopyTexSubImage2D) \
    USE_GL_FUNC(glCullFace) \
    USE_GL_FUNC(glDeleteLists) \
    USE_GL_FUNC(glDeleteTextures) \
    USE_GL_FUNC(glDepthFunc) \
    USE_GL_FUNC(glDepthMask) \
    USE_GL_FUNC(glDepthRange) \
    USE_GL_FUNC(glDisable) \
    USE_GL_FUNC(glDisableClientState) \
    USE_GL_FUNC(glDrawArrays) \
    USE_GL_FUNC(glDrawBuffer) \
    USE_GL_FUNC(glDrawElements) \
    USE_GL_FUNC(glDrawPixels) \
    USE_GL_FUNC(glEdgeFlag) \
    USE_GL_FUNC(glEdgeFlagPointer) \
    USE_GL_FUNC(glEdgeFlagv) \
    USE_GL_FUNC(glEnable) \
    USE_GL_FUNC(glEnableClientState) \
    USE_GL_FUNC(glEnd) \
    USE_GL_FUNC(glEndList) \
    USE_GL_FUNC(glEvalCoord1d) \
    USE_GL_FUNC(glEvalCoord1dv) \
    USE_GL_FUNC(glEvalCoord1f) \
    USE_GL_FUNC(glEvalCoord1fv) \
    USE_GL_FUNC(glEvalCoord2d) \
    USE_GL_FUNC(glEvalCoord2dv) \
    USE_GL_FUNC(glEvalCoord2f) \
    USE_GL_FUNC(glEvalCoord2fv) \
    USE_GL_FUNC(glEvalMesh1) \
    USE_GL_FUNC(glEvalMesh2) \
    USE_GL_FUNC(glEvalPoint1) \
    USE_GL_FUNC(glEvalPoint2) \
    USE_GL_FUNC(glFeedbackBuffer) \
    USE_GL_FUNC(glFinish) \
    USE_GL_FUNC(glFlush) \
    USE_GL_FUNC(glFogf) \
    USE_GL_FUNC(glFogfv) \
    USE_GL_FUNC(glFogi) \
    USE_GL_FUNC(glFogiv) \
    USE_GL_FUNC(glFrontFace) \
    USE_GL_FUNC(glFrustum) \
    USE_GL_FUNC(glGenLists) \
    USE_GL_FUNC(glGenTextures) \
    USE_GL_FUNC(glGetBooleanv) \
    USE_GL_FUNC(glGetClipPlane) \
    USE_GL_FUNC(glGetDoublev) \
    USE_GL_FUNC(glGetError) \
    USE_GL_FUNC(glGetFloatv) \
    USE_GL_FUNC(glGetIntegerv) \
    USE_GL_FUNC(glGetLightfv) \
    USE_GL_FUNC(glGetLightiv) \
    USE_GL_FUNC(glGetMapdv) \
    USE_GL_FUNC(glGetMapfv) \
    USE_GL_FUNC(glGetMapiv) \
    USE_GL_FUNC(glGetMaterialfv) \
    USE_GL_FUNC(glGetMaterialiv) \
    USE_GL_FUNC(glGetPixelMapfv) \
    USE_GL_FUNC(glGetPixelMapuiv) \
    USE_GL_FUNC(glGetPixelMapusv) \
    USE_GL_FUNC(glGetPointerv) \
    USE_GL_FUNC(glGetPolygonStipple) \
    USE_GL_FUNC(glGetString) \
    USE_GL_FUNC(glGetTexEnvfv) \
    USE_GL_FUNC(glGetTexEnviv) \
    USE_GL_FUNC(glGetTexGendv) \
    USE_GL_FUNC(glGetTexGenfv) \
    USE_GL_FUNC(glGetTexGeniv) \
    USE_GL_FUNC(glGetTexImage) \
    USE_GL_FUNC(glGetTexLevelParameterfv) \
    USE_GL_FUNC(glGetTexLevelParameteriv) \
    USE_GL_FUNC(glGetTexParameterfv) \
    USE_GL_FUNC(glGetTexParameteriv) \
    USE_GL_FUNC(glHint) \
    USE_GL_FUNC(glIndexMask) \
    USE_GL_FUNC(glIndexPointer) \
    USE_GL_FUNC(glIndexd) \
    USE_GL_FUNC(glIndexdv) \
    USE_GL_FUNC(glIndexf) \
    USE_GL_FUNC(glIndexfv) \
    USE_GL_FUNC(glIndexi) \
    USE_GL_FUNC(glIndexiv) \
    USE_GL_FUNC(glIndexs) \
    USE_GL_FUNC(glIndexsv) \
    USE_GL_FUNC(glIndexub) \
    USE_GL_FUNC(glIndexubv) \
    USE_GL_FUNC(glInitNames) \
    USE_GL_FUNC(glInterleavedArrays) \
    USE_GL_FUNC(glIsEnabled) \
    USE_GL_FUNC(glIsList) \
    USE_GL_FUNC(glIsTexture) \
    USE_GL_FUNC(glLightModelf) \
    USE_GL_FUNC(glLightModelfv) \
    USE_GL_FUNC(glLightModeli) \
    USE_GL_FUNC(glLightModeliv) \
    USE_GL_FUNC(glLightf) \
    USE_GL_FUNC(glLightfv) \
    USE_GL_FUNC(glLighti) \
    USE_GL_FUNC(glLightiv) \
    USE_GL_FUNC(glLineStipple) \
    USE_GL_FUNC(glLineWidth) \
    USE_GL_FUNC(glListBase) \
    USE_GL_FUNC(glLoadIdentity) \
    USE_GL_FUNC(glLoadMatrixd) \
    USE_GL_FUNC(glLoadMatrixf) \
    USE_GL_FUNC(glLoadName) \
    USE_GL_FUNC(glLogicOp) \
    USE_GL_FUNC(glMap1d) \
    USE_GL_FUNC(glMap1f) \
    USE_GL_FUNC(glMap2d) \
    USE_GL_FUNC(glMap2f) \
    USE_GL_FUNC(glMapGrid1d) \
    USE_GL_FUNC(glMapGrid1f) \
    USE_GL_FUNC(glMapGrid2d) \
    USE_GL_FUNC(glMapGrid2f) \
    USE_GL_FUNC(glMaterialf) \
    USE_GL_FUNC(glMaterialfv) \
    USE_GL_FUNC(glMateriali) \
    USE_GL_FUNC(glMaterialiv) \
    USE_GL_FUNC(glMatrixMode) \
    USE_GL_FUNC(glMultMatrixd) \
    USE_GL_FUNC(glMultMatrixf) \
    USE_GL_FUNC(glNewList) \
    USE_GL_FUNC(glNormal3b) \
    USE_GL_FUNC(glNormal3bv) \
    USE_GL_FUNC(glNormal3d) \
    USE_GL_FUNC(glNormal3dv) \
    USE_GL_FUNC(glNormal3f) \
    USE_GL_FUNC(glNormal3fv) \
    USE_GL_FUNC(glNormal3i) \
    USE_GL_FUNC(glNormal3iv) \
    USE_GL_FUNC(glNormal3s) \
    USE_GL_FUNC(glNormal3sv) \
    USE_GL_FUNC(glNormalPointer) \
    USE_GL_FUNC(glOrtho) \
    USE_GL_FUNC(glPassThrough) \
    USE_GL_FUNC(glPixelMapfv) \
    USE_GL_FUNC(glPixelMapuiv) \
    USE_GL_FUNC(glPixelMapusv) \
    USE_GL_FUNC(glPixelStoref) \
    USE_GL_FUNC(glPixelStorei) \
    USE_GL_FUNC(glPixelTransferf) \
    USE_GL_FUNC(glPixelTransferi) \
    USE_GL_FUNC(glPixelZoom) \
    USE_GL_FUNC(glPointSize) \
    USE_GL_FUNC(glPolygonMode) \
    USE_GL_FUNC(glPolygonOffset) \
    USE_GL_FUNC(glPolygonStipple) \
    USE_GL_FUNC(glPopAttrib) \
    USE_GL_FUNC(glPopClientAttrib) \
    USE_GL_FUNC(glPopMatrix) \
    USE_GL_FUNC(glPopName) \
    USE_GL_FUNC(glPrioritizeTextures) \
    USE_GL_FUNC(glPushAttrib) \
    USE_GL_FUNC(glPushClientAttrib) \
    USE_GL_FUNC(glPushMatrix) \
    USE_GL_FUNC(glPushName) \
    USE_GL_FUNC(glRasterPos2d) \
    USE_GL_FUNC(glRasterPos2dv) \
    USE_GL_FUNC(glRasterPos2f) \
    USE_GL_FUNC(glRasterPos2fv) \
    USE_GL_FUNC(glRasterPos2i) \
    USE_GL_FUNC(glRasterPos2iv) \
    USE_GL_FUNC(glRasterPos2s) \
    USE_GL_FUNC(glRasterPos2sv) \
    USE_GL_FUNC(glRasterPos3d) \
    USE_GL_FUNC(glRasterPos3dv) \
    USE_GL_FUNC(glRasterPos3f) \
    USE_GL_FUNC(glRasterPos3fv) \
    USE_GL_FUNC(glRasterPos3i) \
    USE_GL_FUNC(glRasterPos3iv) \
    USE_GL_FUNC(glRasterPos3s) \
    USE_GL_FUNC(glRasterPos3sv) \
    USE_GL_FUNC(glRasterPos4d) \
    USE_GL_FUNC(glRasterPos4dv) \
    USE_GL_FUNC(glRasterPos4f) \
    USE_GL_FUNC(glRasterPos4fv) \
    USE_GL_FUNC(glRasterPos4i) \
    USE_GL_FUNC(glRasterPos4iv) \
    USE_GL_FUNC(glRasterPos4s) \
    USE_GL_FUNC(glRasterPos4sv) \
    USE_GL_FUNC(glReadBuffer) \
    USE_GL_FUNC(glReadPixels) \
    USE_GL_FUNC(glRectd) \
    USE_GL_FUNC(glRectdv) \
    USE_GL_FUNC(glRectf) \
    USE_GL_FUNC(glRectfv) \
    USE_GL_FUNC(glRecti) \
    USE_GL_FUNC(glRectiv) \
    USE_GL_FUNC(glRects) \
    USE_GL_FUNC(glRectsv) \
    USE_GL_FUNC(glRenderMode) \
    USE_GL_FUNC(glRotated) \
    USE_GL_FUNC(glRotatef) \
    USE_GL_FUNC(glScaled) \
    USE_GL_FUNC(glScalef) \
    USE_GL_FUNC(glScissor) \
    USE_GL_FUNC(glSelectBuffer) \
    USE_GL_FUNC(glShadeModel) \
    USE_GL_FUNC(glStencilFunc) \
    USE_GL_FUNC(glStencilMask) \
    USE_GL_FUNC(glStencilOp) \
    USE_GL_FUNC(glTexCoord1d) \
    USE_GL_FUNC(glTexCoord1dv) \
    USE_GL_FUNC(glTexCoord1f) \
    USE_GL_FUNC(glTexCoord1fv) \
    USE_GL_FUNC(glTexCoord1i) \
    USE_GL_FUNC(glTexCoord1iv) \
    USE_GL_FUNC(glTexCoord1s) \
    USE_GL_FUNC(glTexCoord1sv) \
    USE_GL_FUNC(glTexCoord2d) \
    USE_GL_FUNC(glTexCoord2dv) \
    USE_GL_FUNC(glTexCoord2f) \
    USE_GL_FUNC(glTexCoord2fv) \
    USE_GL_FUNC(glTexCoord2i) \
    USE_GL_FUNC(glTexCoord2iv) \
    USE_GL_FUNC(glTexCoord2s) \
    USE_GL_FUNC(glTexCoord2sv) \
    USE_GL_FUNC(glTexCoord3d) \
    USE_GL_FUNC(glTexCoord3dv) \
    USE_GL_FUNC(glTexCoord3f) \
    USE_GL_FUNC(glTexCoord3fv) \
    USE_GL_FUNC(glTexCoord3i) \
    USE_GL_FUNC(glTexCoord3iv) \
    USE_GL_FUNC(glTexCoord3s) \
    USE_GL_FUNC(glTexCoord3sv) \
    USE_GL_FUNC(glTexCoord4d) \
    USE_GL_FUNC(glTexCoord4dv) \
    USE_GL_FUNC(glTexCoord4f) \
    USE_GL_FUNC(glTexCoord4fv) \
    USE_GL_FUNC(glTexCoord4i) \
    USE_GL_FUNC(glTexCoord4iv) \
    USE_GL_FUNC(glTexCoord4s) \
    USE_GL_FUNC(glTexCoord4sv) \
    USE_GL_FUNC(glTexCoordPointer) \
    USE_GL_FUNC(glTexEnvf) \
    USE_GL_FUNC(glTexEnvfv) \
    USE_GL_FUNC(glTexEnvi) \
    USE_GL_FUNC(glTexEnviv) \
    USE_GL_FUNC(glTexGend) \
    USE_GL_FUNC(glTexGendv) \
    USE_GL_FUNC(glTexGenf) \
    USE_GL_FUNC(glTexGenfv) \
    USE_GL_FUNC(glTexGeni) \
    USE_GL_FUNC(glTexGeniv) \
    USE_GL_FUNC(glTexImage1D) \
    USE_GL_FUNC(glTexImage2D) \
    USE_GL_FUNC(glTexParameterf) \
    USE_GL_FUNC(glTexParameterfv) \
    USE_GL_FUNC(glTexParameteri) \
    USE_GL_FUNC(glTexParameteriv) \
    USE_GL_FUNC(glTexSubImage1D) \
    USE_GL_FUNC(glTexSubImage2D) \
    USE_GL_FUNC(glTranslated) \
    USE_GL_FUNC(glTranslatef) \
    USE_GL_FUNC(glVertex2d) \
    USE_GL_FUNC(glVertex2dv) \
    USE_GL_FUNC(glVertex2f) \
    USE_GL_FUNC(glVertex2fv) \
    USE_GL_FUNC(glVertex2i) \
    USE_GL_FUNC(glVertex2iv) \
    USE_GL_FUNC(glVertex2s) \
    USE_GL_FUNC(glVertex2sv) \
    USE_GL_FUNC(glVertex3d) \
    USE_GL_FUNC(glVertex3dv) \
    USE_GL_FUNC(glVertex3f) \
    USE_GL_FUNC(glVertex3fv) \
    USE_GL_FUNC(glVertex3i) \
    USE_GL_FUNC(glVertex3iv) \
    USE_GL_FUNC(glVertex3s) \
    USE_GL_FUNC(glVertex3sv) \
    USE_GL_FUNC(glVertex4d) \
    USE_GL_FUNC(glVertex4dv) \
    USE_GL_FUNC(glVertex4f) \
    USE_GL_FUNC(glVertex4fv) \
    USE_GL_FUNC(glVertex4i) \
    USE_GL_FUNC(glVertex4iv) \
    USE_GL_FUNC(glVertex4s) \
    USE_GL_FUNC(glVertex4sv) \
    USE_GL_FUNC(glVertexPointer) \
    USE_GL_FUNC(glViewport)

#endif /* __WINE_WGL_DRIVER_H */
