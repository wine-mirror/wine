/* Automatically generated from http://www.opengl.org/registry files; DO NOT EDIT! */

typedef ULONG PTR32;

extern BOOL wrap_wglCopyContext( TEB *teb, HGLRC hglrcSrc, HGLRC hglrcDst, UINT mask );
extern HGLRC wrap_wglCreateContext( TEB *teb, HDC hDc, HGLRC handle );
extern BOOL wrap_wglDeleteContext( TEB *teb, HGLRC oldContext );
extern BOOL wrap_wglMakeCurrent( TEB *teb, HDC hDc, HGLRC newContext );
extern BOOL wrap_wglShareLists( TEB *teb, HGLRC hrcSrvShare, HGLRC hrcSrvSource );
extern BOOL wrap_wglSwapBuffers( TEB *teb, HDC hdc );
extern void wrap_glClear( TEB *teb, GLbitfield mask );
extern void wrap_glDrawBuffer( TEB *teb, GLenum buf );
extern void wrap_glDrawPixels( TEB *teb, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels );
extern void wrap_glFinish( TEB *teb );
extern void wrap_glFlush( TEB *teb );
extern void wrap_glGetBooleanv( TEB *teb, GLenum pname, GLboolean *data );
extern void wrap_glGetDoublev( TEB *teb, GLenum pname, GLdouble *data );
extern GLenum wrap_glGetError( TEB *teb );
extern void wrap_glGetFloatv( TEB *teb, GLenum pname, GLfloat *data );
extern void wrap_glGetIntegerv( TEB *teb, GLenum pname, GLint *data );
extern const GLubyte *wrap_glGetString( TEB *teb, GLenum name );
extern void wrap_glReadBuffer( TEB *teb, GLenum src );
extern void wrap_glReadPixels( TEB *teb, GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels );
extern void wrap_glViewport( TEB *teb, GLint x, GLint y, GLsizei width, GLsizei height );
extern GLsync wrap_glCreateSyncFromCLeventARB( TEB *teb, struct _cl_context *context, struct _cl_event *event, GLbitfield flags, GLsync handle );
extern void wrap_glDebugMessageCallback( TEB *teb, GLDEBUGPROC callback, const void *userParam );
extern void wrap_glDebugMessageCallbackAMD( TEB *teb, GLDEBUGPROCAMD callback, void *userParam );
extern void wrap_glDebugMessageCallbackARB( TEB *teb, GLDEBUGPROCARB callback, const void *userParam );
extern void wrap_glDrawBuffers( TEB *teb, GLsizei n, const GLenum *bufs );
extern GLsync wrap_glFenceSync( TEB *teb, GLenum condition, GLbitfield flags, GLsync handle );
extern void wrap_glFramebufferDrawBufferEXT( TEB *teb, GLuint framebuffer, GLenum mode );
extern void wrap_glFramebufferDrawBuffersEXT( TEB *teb, GLuint framebuffer, GLsizei n, const GLenum *bufs );
extern void wrap_glFramebufferReadBufferEXT( TEB *teb, GLuint framebuffer, GLenum mode );
extern void wrap_glGetFramebufferParameterivEXT( TEB *teb, GLuint framebuffer, GLenum pname, GLint *params );
extern void wrap_glGetInteger64v( TEB *teb, GLenum pname, GLint64 *data );
extern void wrap_glGetUnsignedBytevEXT( TEB *teb, GLenum pname, GLubyte *data );
extern GLsync wrap_glImportSyncEXT( TEB *teb, GLenum external_sync_type, GLintptr external_sync, GLbitfield flags, GLsync handle );
extern void wrap_glNamedFramebufferDrawBuffer( TEB *teb, GLuint framebuffer, GLenum buf );
extern void wrap_glNamedFramebufferDrawBuffers( TEB *teb, GLuint framebuffer, GLsizei n, const GLenum *bufs );
extern void wrap_glNamedFramebufferReadBuffer( TEB *teb, GLuint framebuffer, GLenum src );
extern HGLRC wrap_wglCreateContextAttribsARB( TEB *teb, HDC hDC, HGLRC hShareContext, const int *attribList, HGLRC handle );
extern HPBUFFERARB wrap_wglCreatePbufferARB( TEB *teb, HDC hDC, int iPixelFormat, int iWidth, int iHeight, const int *piAttribList, HPBUFFERARB handle );
extern BOOL wrap_wglMakeContextCurrentARB( TEB *teb, HDC hDrawDC, HDC hReadDC, HGLRC hglrc );

#ifdef _WIN64
extern void wow64_glBufferStorage( TEB *teb, GLenum target, GLsizeiptr size, const void *data, GLbitfield flags, PFN_glBufferStorage func );
extern void wow64_glDeleteBuffers( TEB *teb, GLsizei n, const GLuint *buffers, PFN_glDeleteBuffers func );
extern void wow64_glFlushMappedBufferRange( TEB *teb, GLenum target, GLintptr offset, GLsizeiptr length, PFN_glFlushMappedBufferRange func );
extern void wow64_glFlushMappedNamedBufferRange( TEB *teb, GLuint buffer, GLintptr offset, GLsizeiptr length, PFN_glFlushMappedNamedBufferRange func );
extern void wow64_glGetBufferPointerv( TEB *teb, GLenum target, GLenum pname, PTR32 *params, PFN_glGetBufferPointerv func );
extern void wow64_glGetNamedBufferPointerv( TEB *teb, GLuint buffer, GLenum pname, PTR32 *params, PFN_glGetNamedBufferPointerv func );
extern void *wow64_glMapBuffer( TEB *teb, GLenum target, GLenum access, PFN_glMapBuffer func );
extern void *wow64_glMapBufferRange( TEB *teb, GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access, PFN_glMapBufferRange func );
extern void *wow64_glMapNamedBuffer( TEB *teb, GLuint buffer, GLenum access, PFN_glMapNamedBuffer func );
extern void *wow64_glMapNamedBufferRange( TEB *teb, GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access, PFN_glMapNamedBufferRange func );
extern void wow64_glNamedBufferStorage( TEB *teb, GLuint buffer, GLsizeiptr size, const void *data, GLbitfield flags, PFN_glNamedBufferStorage func );
extern GLboolean wow64_glUnmapBuffer( TEB *teb, GLenum target, PFN_glUnmapBuffer func );
extern GLboolean wow64_glUnmapNamedBuffer( TEB *teb, GLuint buffer, PFN_glUnmapNamedBuffer func );
#endif
