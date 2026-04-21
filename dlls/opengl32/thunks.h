/* Automatically generated from http://www.opengl.org/registry files; DO NOT EDIT! */

extern GLsync WINAPI glCreateSyncFromCLeventARB( struct _cl_context *context, struct _cl_event *event, GLbitfield flags );
extern void WINAPI glDeleteSync( GLsync sync );
extern GLsync WINAPI glFenceSync( GLenum condition, GLbitfield flags );
extern const GLubyte * WINAPI glGetStringi( GLenum name, GLuint index );
extern GLsync WINAPI glImportSyncEXT( GLenum external_sync_type, GLintptr external_sync, GLbitfield flags );
extern BOOL WINAPI wglChoosePixelFormatARB( HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats );
extern HGLRC WINAPI wglCreateContextAttribsARB( HDC hDC, HGLRC hShareContext, const int *attribList );
extern HPBUFFERARB WINAPI wglCreatePbufferARB( HDC hDC, int iPixelFormat, int iWidth, int iHeight, const int *piAttribList );
extern BOOL WINAPI wglDestroyPbufferARB( HPBUFFERARB hPbuffer );
extern HDC WINAPI wglGetCurrentReadDCARB(void);
extern const char * WINAPI wglGetExtensionsStringARB( HDC hdc );
extern const char * WINAPI wglGetExtensionsStringEXT(void);
extern BOOL WINAPI wglGetPixelFormatAttribfvARB( HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, FLOAT *pfValues );
extern BOOL WINAPI wglGetPixelFormatAttribivARB( HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, int *piValues );
extern BOOL WINAPI wglMakeContextCurrentARB( HDC hDrawDC, HDC hReadDC, HGLRC hglrc );
extern const GLchar * WINAPI wglQueryCurrentRendererStringWINE( GLenum attribute );
extern const GLchar * WINAPI wglQueryRendererStringWINE( HDC dc, GLint renderer, GLenum attribute );
