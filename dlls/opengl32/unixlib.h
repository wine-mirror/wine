/* Automatically generated from http://www.opengl.org/registry files; DO NOT EDIT! */

#ifndef __WINE_OPENGL32_UNIXLIB_H
#define __WINE_OPENGL32_UNIXLIB_H

#include <stdarg.h>
#include <stddef.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wingdi.h"
#include "ntuser.h"

#include "wine/wgl.h"
#include "wine/unixlib.h"

struct process_attach_params
{
    UINT64 call_gl_debug_message_callback;
};

struct wglCopyContext_params
{
    TEB *teb;
    HGLRC hglrcSrc;
    HGLRC hglrcDst;
    UINT mask;
    BOOL ret;
};

struct wglCreateContext_params
{
    TEB *teb;
    HDC hDc;
    HGLRC ret;
};

struct wglDeleteContext_params
{
    TEB *teb;
    HGLRC oldContext;
    BOOL ret;
};

struct wglGetPixelFormat_params
{
    TEB *teb;
    HDC hdc;
    int ret;
};

struct wglGetProcAddress_params
{
    TEB *teb;
    LPCSTR lpszProc;
    PROC ret;
};

struct wglMakeCurrent_params
{
    TEB *teb;
    HDC hDc;
    HGLRC newContext;
    BOOL ret;
};

struct wglSetPixelFormat_params
{
    TEB *teb;
    HDC hdc;
    int ipfd;
    const PIXELFORMATDESCRIPTOR *ppfd;
    BOOL ret;
};

struct wglShareLists_params
{
    TEB *teb;
    HGLRC hrcSrvShare;
    HGLRC hrcSrvSource;
    BOOL ret;
};

struct wglSwapBuffers_params
{
    TEB *teb;
    HDC hdc;
    BOOL ret;
};

struct glAccum_params
{
    TEB *teb;
    GLenum op;
    GLfloat value;
};

struct glAlphaFunc_params
{
    TEB *teb;
    GLenum func;
    GLfloat ref;
};

struct glAreTexturesResident_params
{
    TEB *teb;
    GLsizei n;
    const GLuint *textures;
    GLboolean *residences;
    GLboolean ret;
};

struct glArrayElement_params
{
    TEB *teb;
    GLint i;
};

struct glBegin_params
{
    TEB *teb;
    GLenum mode;
};

struct glBindTexture_params
{
    TEB *teb;
    GLenum target;
    GLuint texture;
};

struct glBitmap_params
{
    TEB *teb;
    GLsizei width;
    GLsizei height;
    GLfloat xorig;
    GLfloat yorig;
    GLfloat xmove;
    GLfloat ymove;
    const GLubyte *bitmap;
};

struct glBlendFunc_params
{
    TEB *teb;
    GLenum sfactor;
    GLenum dfactor;
};

struct glCallList_params
{
    TEB *teb;
    GLuint list;
};

struct glCallLists_params
{
    TEB *teb;
    GLsizei n;
    GLenum type;
    const void *lists;
};

struct glClear_params
{
    TEB *teb;
    GLbitfield mask;
};

struct glClearAccum_params
{
    TEB *teb;
    GLfloat red;
    GLfloat green;
    GLfloat blue;
    GLfloat alpha;
};

struct glClearColor_params
{
    TEB *teb;
    GLfloat red;
    GLfloat green;
    GLfloat blue;
    GLfloat alpha;
};

struct glClearDepth_params
{
    TEB *teb;
    GLdouble depth;
};

struct glClearIndex_params
{
    TEB *teb;
    GLfloat c;
};

struct glClearStencil_params
{
    TEB *teb;
    GLint s;
};

struct glClipPlane_params
{
    TEB *teb;
    GLenum plane;
    const GLdouble *equation;
};

struct glColor3b_params
{
    TEB *teb;
    GLbyte red;
    GLbyte green;
    GLbyte blue;
};

struct glColor3bv_params
{
    TEB *teb;
    const GLbyte *v;
};

struct glColor3d_params
{
    TEB *teb;
    GLdouble red;
    GLdouble green;
    GLdouble blue;
};

struct glColor3dv_params
{
    TEB *teb;
    const GLdouble *v;
};

struct glColor3f_params
{
    TEB *teb;
    GLfloat red;
    GLfloat green;
    GLfloat blue;
};

struct glColor3fv_params
{
    TEB *teb;
    const GLfloat *v;
};

struct glColor3i_params
{
    TEB *teb;
    GLint red;
    GLint green;
    GLint blue;
};

struct glColor3iv_params
{
    TEB *teb;
    const GLint *v;
};

struct glColor3s_params
{
    TEB *teb;
    GLshort red;
    GLshort green;
    GLshort blue;
};

struct glColor3sv_params
{
    TEB *teb;
    const GLshort *v;
};

struct glColor3ub_params
{
    TEB *teb;
    GLubyte red;
    GLubyte green;
    GLubyte blue;
};

struct glColor3ubv_params
{
    TEB *teb;
    const GLubyte *v;
};

struct glColor3ui_params
{
    TEB *teb;
    GLuint red;
    GLuint green;
    GLuint blue;
};

struct glColor3uiv_params
{
    TEB *teb;
    const GLuint *v;
};

struct glColor3us_params
{
    TEB *teb;
    GLushort red;
    GLushort green;
    GLushort blue;
};

struct glColor3usv_params
{
    TEB *teb;
    const GLushort *v;
};

struct glColor4b_params
{
    TEB *teb;
    GLbyte red;
    GLbyte green;
    GLbyte blue;
    GLbyte alpha;
};

struct glColor4bv_params
{
    TEB *teb;
    const GLbyte *v;
};

struct glColor4d_params
{
    TEB *teb;
    GLdouble red;
    GLdouble green;
    GLdouble blue;
    GLdouble alpha;
};

struct glColor4dv_params
{
    TEB *teb;
    const GLdouble *v;
};

struct glColor4f_params
{
    TEB *teb;
    GLfloat red;
    GLfloat green;
    GLfloat blue;
    GLfloat alpha;
};

struct glColor4fv_params
{
    TEB *teb;
    const GLfloat *v;
};

struct glColor4i_params
{
    TEB *teb;
    GLint red;
    GLint green;
    GLint blue;
    GLint alpha;
};

struct glColor4iv_params
{
    TEB *teb;
    const GLint *v;
};

struct glColor4s_params
{
    TEB *teb;
    GLshort red;
    GLshort green;
    GLshort blue;
    GLshort alpha;
};

struct glColor4sv_params
{
    TEB *teb;
    const GLshort *v;
};

struct glColor4ub_params
{
    TEB *teb;
    GLubyte red;
    GLubyte green;
    GLubyte blue;
    GLubyte alpha;
};

struct glColor4ubv_params
{
    TEB *teb;
    const GLubyte *v;
};

struct glColor4ui_params
{
    TEB *teb;
    GLuint red;
    GLuint green;
    GLuint blue;
    GLuint alpha;
};

struct glColor4uiv_params
{
    TEB *teb;
    const GLuint *v;
};

struct glColor4us_params
{
    TEB *teb;
    GLushort red;
    GLushort green;
    GLushort blue;
    GLushort alpha;
};

struct glColor4usv_params
{
    TEB *teb;
    const GLushort *v;
};

struct glColorMask_params
{
    TEB *teb;
    GLboolean red;
    GLboolean green;
    GLboolean blue;
    GLboolean alpha;
};

struct glColorMaterial_params
{
    TEB *teb;
    GLenum face;
    GLenum mode;
};

struct glColorPointer_params
{
    TEB *teb;
    GLint size;
    GLenum type;
    GLsizei stride;
    const void *pointer;
};

struct glCopyPixels_params
{
    TEB *teb;
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
    GLenum type;
};

struct glCopyTexImage1D_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLenum internalformat;
    GLint x;
    GLint y;
    GLsizei width;
    GLint border;
};

struct glCopyTexImage2D_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLenum internalformat;
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
    GLint border;
};

struct glCopyTexSubImage1D_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint x;
    GLint y;
    GLsizei width;
};

struct glCopyTexSubImage2D_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
};

struct glCullFace_params
{
    TEB *teb;
    GLenum mode;
};

struct glDeleteLists_params
{
    TEB *teb;
    GLuint list;
    GLsizei range;
};

struct glDeleteTextures_params
{
    TEB *teb;
    GLsizei n;
    const GLuint *textures;
};

struct glDepthFunc_params
{
    TEB *teb;
    GLenum func;
};

struct glDepthMask_params
{
    TEB *teb;
    GLboolean flag;
};

struct glDepthRange_params
{
    TEB *teb;
    GLdouble n;
    GLdouble f;
};

struct glDisable_params
{
    TEB *teb;
    GLenum cap;
};

struct glDisableClientState_params
{
    TEB *teb;
    GLenum array;
};

struct glDrawArrays_params
{
    TEB *teb;
    GLenum mode;
    GLint first;
    GLsizei count;
};

struct glDrawBuffer_params
{
    TEB *teb;
    GLenum buf;
};

struct glDrawElements_params
{
    TEB *teb;
    GLenum mode;
    GLsizei count;
    GLenum type;
    const void *indices;
};

struct glDrawPixels_params
{
    TEB *teb;
    GLsizei width;
    GLsizei height;
    GLenum format;
    GLenum type;
    const void *pixels;
};

struct glEdgeFlag_params
{
    TEB *teb;
    GLboolean flag;
};

struct glEdgeFlagPointer_params
{
    TEB *teb;
    GLsizei stride;
    const void *pointer;
};

struct glEdgeFlagv_params
{
    TEB *teb;
    const GLboolean *flag;
};

struct glEnable_params
{
    TEB *teb;
    GLenum cap;
};

struct glEnableClientState_params
{
    TEB *teb;
    GLenum array;
};

struct glEnd_params
{
    TEB *teb;
};

struct glEndList_params
{
    TEB *teb;
};

struct glEvalCoord1d_params
{
    TEB *teb;
    GLdouble u;
};

struct glEvalCoord1dv_params
{
    TEB *teb;
    const GLdouble *u;
};

struct glEvalCoord1f_params
{
    TEB *teb;
    GLfloat u;
};

struct glEvalCoord1fv_params
{
    TEB *teb;
    const GLfloat *u;
};

struct glEvalCoord2d_params
{
    TEB *teb;
    GLdouble u;
    GLdouble v;
};

struct glEvalCoord2dv_params
{
    TEB *teb;
    const GLdouble *u;
};

struct glEvalCoord2f_params
{
    TEB *teb;
    GLfloat u;
    GLfloat v;
};

struct glEvalCoord2fv_params
{
    TEB *teb;
    const GLfloat *u;
};

struct glEvalMesh1_params
{
    TEB *teb;
    GLenum mode;
    GLint i1;
    GLint i2;
};

struct glEvalMesh2_params
{
    TEB *teb;
    GLenum mode;
    GLint i1;
    GLint i2;
    GLint j1;
    GLint j2;
};

struct glEvalPoint1_params
{
    TEB *teb;
    GLint i;
};

struct glEvalPoint2_params
{
    TEB *teb;
    GLint i;
    GLint j;
};

struct glFeedbackBuffer_params
{
    TEB *teb;
    GLsizei size;
    GLenum type;
    GLfloat *buffer;
};

struct glFinish_params
{
    TEB *teb;
};

struct glFlush_params
{
    TEB *teb;
};

struct glFogf_params
{
    TEB *teb;
    GLenum pname;
    GLfloat param;
};

struct glFogfv_params
{
    TEB *teb;
    GLenum pname;
    const GLfloat *params;
};

struct glFogi_params
{
    TEB *teb;
    GLenum pname;
    GLint param;
};

struct glFogiv_params
{
    TEB *teb;
    GLenum pname;
    const GLint *params;
};

struct glFrontFace_params
{
    TEB *teb;
    GLenum mode;
};

struct glFrustum_params
{
    TEB *teb;
    GLdouble left;
    GLdouble right;
    GLdouble bottom;
    GLdouble top;
    GLdouble zNear;
    GLdouble zFar;
};

struct glGenLists_params
{
    TEB *teb;
    GLsizei range;
    GLuint ret;
};

struct glGenTextures_params
{
    TEB *teb;
    GLsizei n;
    GLuint *textures;
};

struct glGetBooleanv_params
{
    TEB *teb;
    GLenum pname;
    GLboolean *data;
};

struct glGetClipPlane_params
{
    TEB *teb;
    GLenum plane;
    GLdouble *equation;
};

struct glGetDoublev_params
{
    TEB *teb;
    GLenum pname;
    GLdouble *data;
};

struct glGetError_params
{
    TEB *teb;
    GLenum ret;
};

struct glGetFloatv_params
{
    TEB *teb;
    GLenum pname;
    GLfloat *data;
};

struct glGetIntegerv_params
{
    TEB *teb;
    GLenum pname;
    GLint *data;
};

struct glGetLightfv_params
{
    TEB *teb;
    GLenum light;
    GLenum pname;
    GLfloat *params;
};

struct glGetLightiv_params
{
    TEB *teb;
    GLenum light;
    GLenum pname;
    GLint *params;
};

struct glGetMapdv_params
{
    TEB *teb;
    GLenum target;
    GLenum query;
    GLdouble *v;
};

struct glGetMapfv_params
{
    TEB *teb;
    GLenum target;
    GLenum query;
    GLfloat *v;
};

struct glGetMapiv_params
{
    TEB *teb;
    GLenum target;
    GLenum query;
    GLint *v;
};

struct glGetMaterialfv_params
{
    TEB *teb;
    GLenum face;
    GLenum pname;
    GLfloat *params;
};

struct glGetMaterialiv_params
{
    TEB *teb;
    GLenum face;
    GLenum pname;
    GLint *params;
};

struct glGetPixelMapfv_params
{
    TEB *teb;
    GLenum map;
    GLfloat *values;
};

struct glGetPixelMapuiv_params
{
    TEB *teb;
    GLenum map;
    GLuint *values;
};

struct glGetPixelMapusv_params
{
    TEB *teb;
    GLenum map;
    GLushort *values;
};

struct glGetPointerv_params
{
    TEB *teb;
    GLenum pname;
    void **params;
};

struct glGetPolygonStipple_params
{
    TEB *teb;
    GLubyte *mask;
};

struct glGetString_params
{
    TEB *teb;
    GLenum name;
    const GLubyte *ret;
};

struct glGetTexEnvfv_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLfloat *params;
};

struct glGetTexEnviv_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLint *params;
};

struct glGetTexGendv_params
{
    TEB *teb;
    GLenum coord;
    GLenum pname;
    GLdouble *params;
};

struct glGetTexGenfv_params
{
    TEB *teb;
    GLenum coord;
    GLenum pname;
    GLfloat *params;
};

struct glGetTexGeniv_params
{
    TEB *teb;
    GLenum coord;
    GLenum pname;
    GLint *params;
};

struct glGetTexImage_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLenum format;
    GLenum type;
    void *pixels;
};

struct glGetTexLevelParameterfv_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLenum pname;
    GLfloat *params;
};

struct glGetTexLevelParameteriv_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLenum pname;
    GLint *params;
};

struct glGetTexParameterfv_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLfloat *params;
};

struct glGetTexParameteriv_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLint *params;
};

struct glHint_params
{
    TEB *teb;
    GLenum target;
    GLenum mode;
};

struct glIndexMask_params
{
    TEB *teb;
    GLuint mask;
};

struct glIndexPointer_params
{
    TEB *teb;
    GLenum type;
    GLsizei stride;
    const void *pointer;
};

struct glIndexd_params
{
    TEB *teb;
    GLdouble c;
};

struct glIndexdv_params
{
    TEB *teb;
    const GLdouble *c;
};

struct glIndexf_params
{
    TEB *teb;
    GLfloat c;
};

struct glIndexfv_params
{
    TEB *teb;
    const GLfloat *c;
};

struct glIndexi_params
{
    TEB *teb;
    GLint c;
};

struct glIndexiv_params
{
    TEB *teb;
    const GLint *c;
};

struct glIndexs_params
{
    TEB *teb;
    GLshort c;
};

struct glIndexsv_params
{
    TEB *teb;
    const GLshort *c;
};

struct glIndexub_params
{
    TEB *teb;
    GLubyte c;
};

struct glIndexubv_params
{
    TEB *teb;
    const GLubyte *c;
};

struct glInitNames_params
{
    TEB *teb;
};

struct glInterleavedArrays_params
{
    TEB *teb;
    GLenum format;
    GLsizei stride;
    const void *pointer;
};

struct glIsEnabled_params
{
    TEB *teb;
    GLenum cap;
    GLboolean ret;
};

struct glIsList_params
{
    TEB *teb;
    GLuint list;
    GLboolean ret;
};

struct glIsTexture_params
{
    TEB *teb;
    GLuint texture;
    GLboolean ret;
};

struct glLightModelf_params
{
    TEB *teb;
    GLenum pname;
    GLfloat param;
};

struct glLightModelfv_params
{
    TEB *teb;
    GLenum pname;
    const GLfloat *params;
};

struct glLightModeli_params
{
    TEB *teb;
    GLenum pname;
    GLint param;
};

struct glLightModeliv_params
{
    TEB *teb;
    GLenum pname;
    const GLint *params;
};

struct glLightf_params
{
    TEB *teb;
    GLenum light;
    GLenum pname;
    GLfloat param;
};

struct glLightfv_params
{
    TEB *teb;
    GLenum light;
    GLenum pname;
    const GLfloat *params;
};

struct glLighti_params
{
    TEB *teb;
    GLenum light;
    GLenum pname;
    GLint param;
};

struct glLightiv_params
{
    TEB *teb;
    GLenum light;
    GLenum pname;
    const GLint *params;
};

struct glLineStipple_params
{
    TEB *teb;
    GLint factor;
    GLushort pattern;
};

struct glLineWidth_params
{
    TEB *teb;
    GLfloat width;
};

struct glListBase_params
{
    TEB *teb;
    GLuint base;
};

struct glLoadIdentity_params
{
    TEB *teb;
};

struct glLoadMatrixd_params
{
    TEB *teb;
    const GLdouble *m;
};

struct glLoadMatrixf_params
{
    TEB *teb;
    const GLfloat *m;
};

struct glLoadName_params
{
    TEB *teb;
    GLuint name;
};

struct glLogicOp_params
{
    TEB *teb;
    GLenum opcode;
};

struct glMap1d_params
{
    TEB *teb;
    GLenum target;
    GLdouble u1;
    GLdouble u2;
    GLint stride;
    GLint order;
    const GLdouble *points;
};

struct glMap1f_params
{
    TEB *teb;
    GLenum target;
    GLfloat u1;
    GLfloat u2;
    GLint stride;
    GLint order;
    const GLfloat *points;
};

struct glMap2d_params
{
    TEB *teb;
    GLenum target;
    GLdouble u1;
    GLdouble u2;
    GLint ustride;
    GLint uorder;
    GLdouble v1;
    GLdouble v2;
    GLint vstride;
    GLint vorder;
    const GLdouble *points;
};

struct glMap2f_params
{
    TEB *teb;
    GLenum target;
    GLfloat u1;
    GLfloat u2;
    GLint ustride;
    GLint uorder;
    GLfloat v1;
    GLfloat v2;
    GLint vstride;
    GLint vorder;
    const GLfloat *points;
};

struct glMapGrid1d_params
{
    TEB *teb;
    GLint un;
    GLdouble u1;
    GLdouble u2;
};

struct glMapGrid1f_params
{
    TEB *teb;
    GLint un;
    GLfloat u1;
    GLfloat u2;
};

struct glMapGrid2d_params
{
    TEB *teb;
    GLint un;
    GLdouble u1;
    GLdouble u2;
    GLint vn;
    GLdouble v1;
    GLdouble v2;
};

struct glMapGrid2f_params
{
    TEB *teb;
    GLint un;
    GLfloat u1;
    GLfloat u2;
    GLint vn;
    GLfloat v1;
    GLfloat v2;
};

struct glMaterialf_params
{
    TEB *teb;
    GLenum face;
    GLenum pname;
    GLfloat param;
};

struct glMaterialfv_params
{
    TEB *teb;
    GLenum face;
    GLenum pname;
    const GLfloat *params;
};

struct glMateriali_params
{
    TEB *teb;
    GLenum face;
    GLenum pname;
    GLint param;
};

struct glMaterialiv_params
{
    TEB *teb;
    GLenum face;
    GLenum pname;
    const GLint *params;
};

struct glMatrixMode_params
{
    TEB *teb;
    GLenum mode;
};

struct glMultMatrixd_params
{
    TEB *teb;
    const GLdouble *m;
};

struct glMultMatrixf_params
{
    TEB *teb;
    const GLfloat *m;
};

struct glNewList_params
{
    TEB *teb;
    GLuint list;
    GLenum mode;
};

struct glNormal3b_params
{
    TEB *teb;
    GLbyte nx;
    GLbyte ny;
    GLbyte nz;
};

struct glNormal3bv_params
{
    TEB *teb;
    const GLbyte *v;
};

struct glNormal3d_params
{
    TEB *teb;
    GLdouble nx;
    GLdouble ny;
    GLdouble nz;
};

struct glNormal3dv_params
{
    TEB *teb;
    const GLdouble *v;
};

struct glNormal3f_params
{
    TEB *teb;
    GLfloat nx;
    GLfloat ny;
    GLfloat nz;
};

struct glNormal3fv_params
{
    TEB *teb;
    const GLfloat *v;
};

struct glNormal3i_params
{
    TEB *teb;
    GLint nx;
    GLint ny;
    GLint nz;
};

struct glNormal3iv_params
{
    TEB *teb;
    const GLint *v;
};

struct glNormal3s_params
{
    TEB *teb;
    GLshort nx;
    GLshort ny;
    GLshort nz;
};

struct glNormal3sv_params
{
    TEB *teb;
    const GLshort *v;
};

struct glNormalPointer_params
{
    TEB *teb;
    GLenum type;
    GLsizei stride;
    const void *pointer;
};

struct glOrtho_params
{
    TEB *teb;
    GLdouble left;
    GLdouble right;
    GLdouble bottom;
    GLdouble top;
    GLdouble zNear;
    GLdouble zFar;
};

struct glPassThrough_params
{
    TEB *teb;
    GLfloat token;
};

struct glPixelMapfv_params
{
    TEB *teb;
    GLenum map;
    GLsizei mapsize;
    const GLfloat *values;
};

struct glPixelMapuiv_params
{
    TEB *teb;
    GLenum map;
    GLsizei mapsize;
    const GLuint *values;
};

struct glPixelMapusv_params
{
    TEB *teb;
    GLenum map;
    GLsizei mapsize;
    const GLushort *values;
};

struct glPixelStoref_params
{
    TEB *teb;
    GLenum pname;
    GLfloat param;
};

struct glPixelStorei_params
{
    TEB *teb;
    GLenum pname;
    GLint param;
};

struct glPixelTransferf_params
{
    TEB *teb;
    GLenum pname;
    GLfloat param;
};

struct glPixelTransferi_params
{
    TEB *teb;
    GLenum pname;
    GLint param;
};

struct glPixelZoom_params
{
    TEB *teb;
    GLfloat xfactor;
    GLfloat yfactor;
};

struct glPointSize_params
{
    TEB *teb;
    GLfloat size;
};

struct glPolygonMode_params
{
    TEB *teb;
    GLenum face;
    GLenum mode;
};

struct glPolygonOffset_params
{
    TEB *teb;
    GLfloat factor;
    GLfloat units;
};

struct glPolygonStipple_params
{
    TEB *teb;
    const GLubyte *mask;
};

struct glPopAttrib_params
{
    TEB *teb;
};

struct glPopClientAttrib_params
{
    TEB *teb;
};

struct glPopMatrix_params
{
    TEB *teb;
};

struct glPopName_params
{
    TEB *teb;
};

struct glPrioritizeTextures_params
{
    TEB *teb;
    GLsizei n;
    const GLuint *textures;
    const GLfloat *priorities;
};

struct glPushAttrib_params
{
    TEB *teb;
    GLbitfield mask;
};

struct glPushClientAttrib_params
{
    TEB *teb;
    GLbitfield mask;
};

struct glPushMatrix_params
{
    TEB *teb;
};

struct glPushName_params
{
    TEB *teb;
    GLuint name;
};

struct glRasterPos2d_params
{
    TEB *teb;
    GLdouble x;
    GLdouble y;
};

struct glRasterPos2dv_params
{
    TEB *teb;
    const GLdouble *v;
};

struct glRasterPos2f_params
{
    TEB *teb;
    GLfloat x;
    GLfloat y;
};

struct glRasterPos2fv_params
{
    TEB *teb;
    const GLfloat *v;
};

struct glRasterPos2i_params
{
    TEB *teb;
    GLint x;
    GLint y;
};

struct glRasterPos2iv_params
{
    TEB *teb;
    const GLint *v;
};

struct glRasterPos2s_params
{
    TEB *teb;
    GLshort x;
    GLshort y;
};

struct glRasterPos2sv_params
{
    TEB *teb;
    const GLshort *v;
};

struct glRasterPos3d_params
{
    TEB *teb;
    GLdouble x;
    GLdouble y;
    GLdouble z;
};

struct glRasterPos3dv_params
{
    TEB *teb;
    const GLdouble *v;
};

struct glRasterPos3f_params
{
    TEB *teb;
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

struct glRasterPos3fv_params
{
    TEB *teb;
    const GLfloat *v;
};

struct glRasterPos3i_params
{
    TEB *teb;
    GLint x;
    GLint y;
    GLint z;
};

struct glRasterPos3iv_params
{
    TEB *teb;
    const GLint *v;
};

struct glRasterPos3s_params
{
    TEB *teb;
    GLshort x;
    GLshort y;
    GLshort z;
};

struct glRasterPos3sv_params
{
    TEB *teb;
    const GLshort *v;
};

struct glRasterPos4d_params
{
    TEB *teb;
    GLdouble x;
    GLdouble y;
    GLdouble z;
    GLdouble w;
};

struct glRasterPos4dv_params
{
    TEB *teb;
    const GLdouble *v;
};

struct glRasterPos4f_params
{
    TEB *teb;
    GLfloat x;
    GLfloat y;
    GLfloat z;
    GLfloat w;
};

struct glRasterPos4fv_params
{
    TEB *teb;
    const GLfloat *v;
};

struct glRasterPos4i_params
{
    TEB *teb;
    GLint x;
    GLint y;
    GLint z;
    GLint w;
};

struct glRasterPos4iv_params
{
    TEB *teb;
    const GLint *v;
};

struct glRasterPos4s_params
{
    TEB *teb;
    GLshort x;
    GLshort y;
    GLshort z;
    GLshort w;
};

struct glRasterPos4sv_params
{
    TEB *teb;
    const GLshort *v;
};

struct glReadBuffer_params
{
    TEB *teb;
    GLenum src;
};

struct glReadPixels_params
{
    TEB *teb;
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
    GLenum format;
    GLenum type;
    void *pixels;
};

struct glRectd_params
{
    TEB *teb;
    GLdouble x1;
    GLdouble y1;
    GLdouble x2;
    GLdouble y2;
};

struct glRectdv_params
{
    TEB *teb;
    const GLdouble *v1;
    const GLdouble *v2;
};

struct glRectf_params
{
    TEB *teb;
    GLfloat x1;
    GLfloat y1;
    GLfloat x2;
    GLfloat y2;
};

struct glRectfv_params
{
    TEB *teb;
    const GLfloat *v1;
    const GLfloat *v2;
};

struct glRecti_params
{
    TEB *teb;
    GLint x1;
    GLint y1;
    GLint x2;
    GLint y2;
};

struct glRectiv_params
{
    TEB *teb;
    const GLint *v1;
    const GLint *v2;
};

struct glRects_params
{
    TEB *teb;
    GLshort x1;
    GLshort y1;
    GLshort x2;
    GLshort y2;
};

struct glRectsv_params
{
    TEB *teb;
    const GLshort *v1;
    const GLshort *v2;
};

struct glRenderMode_params
{
    TEB *teb;
    GLenum mode;
    GLint ret;
};

struct glRotated_params
{
    TEB *teb;
    GLdouble angle;
    GLdouble x;
    GLdouble y;
    GLdouble z;
};

struct glRotatef_params
{
    TEB *teb;
    GLfloat angle;
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

struct glScaled_params
{
    TEB *teb;
    GLdouble x;
    GLdouble y;
    GLdouble z;
};

struct glScalef_params
{
    TEB *teb;
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

struct glScissor_params
{
    TEB *teb;
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
};

struct glSelectBuffer_params
{
    TEB *teb;
    GLsizei size;
    GLuint *buffer;
};

struct glShadeModel_params
{
    TEB *teb;
    GLenum mode;
};

struct glStencilFunc_params
{
    TEB *teb;
    GLenum func;
    GLint ref;
    GLuint mask;
};

struct glStencilMask_params
{
    TEB *teb;
    GLuint mask;
};

struct glStencilOp_params
{
    TEB *teb;
    GLenum fail;
    GLenum zfail;
    GLenum zpass;
};

struct glTexCoord1d_params
{
    TEB *teb;
    GLdouble s;
};

struct glTexCoord1dv_params
{
    TEB *teb;
    const GLdouble *v;
};

struct glTexCoord1f_params
{
    TEB *teb;
    GLfloat s;
};

struct glTexCoord1fv_params
{
    TEB *teb;
    const GLfloat *v;
};

struct glTexCoord1i_params
{
    TEB *teb;
    GLint s;
};

struct glTexCoord1iv_params
{
    TEB *teb;
    const GLint *v;
};

struct glTexCoord1s_params
{
    TEB *teb;
    GLshort s;
};

struct glTexCoord1sv_params
{
    TEB *teb;
    const GLshort *v;
};

struct glTexCoord2d_params
{
    TEB *teb;
    GLdouble s;
    GLdouble t;
};

struct glTexCoord2dv_params
{
    TEB *teb;
    const GLdouble *v;
};

struct glTexCoord2f_params
{
    TEB *teb;
    GLfloat s;
    GLfloat t;
};

struct glTexCoord2fv_params
{
    TEB *teb;
    const GLfloat *v;
};

struct glTexCoord2i_params
{
    TEB *teb;
    GLint s;
    GLint t;
};

struct glTexCoord2iv_params
{
    TEB *teb;
    const GLint *v;
};

struct glTexCoord2s_params
{
    TEB *teb;
    GLshort s;
    GLshort t;
};

struct glTexCoord2sv_params
{
    TEB *teb;
    const GLshort *v;
};

struct glTexCoord3d_params
{
    TEB *teb;
    GLdouble s;
    GLdouble t;
    GLdouble r;
};

struct glTexCoord3dv_params
{
    TEB *teb;
    const GLdouble *v;
};

struct glTexCoord3f_params
{
    TEB *teb;
    GLfloat s;
    GLfloat t;
    GLfloat r;
};

struct glTexCoord3fv_params
{
    TEB *teb;
    const GLfloat *v;
};

struct glTexCoord3i_params
{
    TEB *teb;
    GLint s;
    GLint t;
    GLint r;
};

struct glTexCoord3iv_params
{
    TEB *teb;
    const GLint *v;
};

struct glTexCoord3s_params
{
    TEB *teb;
    GLshort s;
    GLshort t;
    GLshort r;
};

struct glTexCoord3sv_params
{
    TEB *teb;
    const GLshort *v;
};

struct glTexCoord4d_params
{
    TEB *teb;
    GLdouble s;
    GLdouble t;
    GLdouble r;
    GLdouble q;
};

struct glTexCoord4dv_params
{
    TEB *teb;
    const GLdouble *v;
};

struct glTexCoord4f_params
{
    TEB *teb;
    GLfloat s;
    GLfloat t;
    GLfloat r;
    GLfloat q;
};

struct glTexCoord4fv_params
{
    TEB *teb;
    const GLfloat *v;
};

struct glTexCoord4i_params
{
    TEB *teb;
    GLint s;
    GLint t;
    GLint r;
    GLint q;
};

struct glTexCoord4iv_params
{
    TEB *teb;
    const GLint *v;
};

struct glTexCoord4s_params
{
    TEB *teb;
    GLshort s;
    GLshort t;
    GLshort r;
    GLshort q;
};

struct glTexCoord4sv_params
{
    TEB *teb;
    const GLshort *v;
};

struct glTexCoordPointer_params
{
    TEB *teb;
    GLint size;
    GLenum type;
    GLsizei stride;
    const void *pointer;
};

struct glTexEnvf_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLfloat param;
};

struct glTexEnvfv_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    const GLfloat *params;
};

struct glTexEnvi_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLint param;
};

struct glTexEnviv_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    const GLint *params;
};

struct glTexGend_params
{
    TEB *teb;
    GLenum coord;
    GLenum pname;
    GLdouble param;
};

struct glTexGendv_params
{
    TEB *teb;
    GLenum coord;
    GLenum pname;
    const GLdouble *params;
};

struct glTexGenf_params
{
    TEB *teb;
    GLenum coord;
    GLenum pname;
    GLfloat param;
};

struct glTexGenfv_params
{
    TEB *teb;
    GLenum coord;
    GLenum pname;
    const GLfloat *params;
};

struct glTexGeni_params
{
    TEB *teb;
    GLenum coord;
    GLenum pname;
    GLint param;
};

struct glTexGeniv_params
{
    TEB *teb;
    GLenum coord;
    GLenum pname;
    const GLint *params;
};

struct glTexImage1D_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLint internalformat;
    GLsizei width;
    GLint border;
    GLenum format;
    GLenum type;
    const void *pixels;
};

struct glTexImage2D_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLint internalformat;
    GLsizei width;
    GLsizei height;
    GLint border;
    GLenum format;
    GLenum type;
    const void *pixels;
};

struct glTexParameterf_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLfloat param;
};

struct glTexParameterfv_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    const GLfloat *params;
};

struct glTexParameteri_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLint param;
};

struct glTexParameteriv_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    const GLint *params;
};

struct glTexSubImage1D_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLsizei width;
    GLenum format;
    GLenum type;
    const void *pixels;
};

struct glTexSubImage2D_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLsizei width;
    GLsizei height;
    GLenum format;
    GLenum type;
    const void *pixels;
};

struct glTranslated_params
{
    TEB *teb;
    GLdouble x;
    GLdouble y;
    GLdouble z;
};

struct glTranslatef_params
{
    TEB *teb;
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

struct glVertex2d_params
{
    TEB *teb;
    GLdouble x;
    GLdouble y;
};

struct glVertex2dv_params
{
    TEB *teb;
    const GLdouble *v;
};

struct glVertex2f_params
{
    TEB *teb;
    GLfloat x;
    GLfloat y;
};

struct glVertex2fv_params
{
    TEB *teb;
    const GLfloat *v;
};

struct glVertex2i_params
{
    TEB *teb;
    GLint x;
    GLint y;
};

struct glVertex2iv_params
{
    TEB *teb;
    const GLint *v;
};

struct glVertex2s_params
{
    TEB *teb;
    GLshort x;
    GLshort y;
};

struct glVertex2sv_params
{
    TEB *teb;
    const GLshort *v;
};

struct glVertex3d_params
{
    TEB *teb;
    GLdouble x;
    GLdouble y;
    GLdouble z;
};

struct glVertex3dv_params
{
    TEB *teb;
    const GLdouble *v;
};

struct glVertex3f_params
{
    TEB *teb;
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

struct glVertex3fv_params
{
    TEB *teb;
    const GLfloat *v;
};

struct glVertex3i_params
{
    TEB *teb;
    GLint x;
    GLint y;
    GLint z;
};

struct glVertex3iv_params
{
    TEB *teb;
    const GLint *v;
};

struct glVertex3s_params
{
    TEB *teb;
    GLshort x;
    GLshort y;
    GLshort z;
};

struct glVertex3sv_params
{
    TEB *teb;
    const GLshort *v;
};

struct glVertex4d_params
{
    TEB *teb;
    GLdouble x;
    GLdouble y;
    GLdouble z;
    GLdouble w;
};

struct glVertex4dv_params
{
    TEB *teb;
    const GLdouble *v;
};

struct glVertex4f_params
{
    TEB *teb;
    GLfloat x;
    GLfloat y;
    GLfloat z;
    GLfloat w;
};

struct glVertex4fv_params
{
    TEB *teb;
    const GLfloat *v;
};

struct glVertex4i_params
{
    TEB *teb;
    GLint x;
    GLint y;
    GLint z;
    GLint w;
};

struct glVertex4iv_params
{
    TEB *teb;
    const GLint *v;
};

struct glVertex4s_params
{
    TEB *teb;
    GLshort x;
    GLshort y;
    GLshort z;
    GLshort w;
};

struct glVertex4sv_params
{
    TEB *teb;
    const GLshort *v;
};

struct glVertexPointer_params
{
    TEB *teb;
    GLint size;
    GLenum type;
    GLsizei stride;
    const void *pointer;
};

struct glViewport_params
{
    TEB *teb;
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
};

struct glAccumxOES_params
{
    TEB *teb;
    GLenum op;
    GLfixed value;
};

struct glAcquireKeyedMutexWin32EXT_params
{
    TEB *teb;
    GLuint memory;
    GLuint64 key;
    GLuint timeout;
    GLboolean ret;
};

struct glActiveProgramEXT_params
{
    TEB *teb;
    GLuint program;
};

struct glActiveShaderProgram_params
{
    TEB *teb;
    GLuint pipeline;
    GLuint program;
};

struct glActiveStencilFaceEXT_params
{
    TEB *teb;
    GLenum face;
};

struct glActiveTexture_params
{
    TEB *teb;
    GLenum texture;
};

struct glActiveTextureARB_params
{
    TEB *teb;
    GLenum texture;
};

struct glActiveVaryingNV_params
{
    TEB *teb;
    GLuint program;
    const GLchar *name;
};

struct glAlphaFragmentOp1ATI_params
{
    TEB *teb;
    GLenum op;
    GLuint dst;
    GLuint dstMod;
    GLuint arg1;
    GLuint arg1Rep;
    GLuint arg1Mod;
};

struct glAlphaFragmentOp2ATI_params
{
    TEB *teb;
    GLenum op;
    GLuint dst;
    GLuint dstMod;
    GLuint arg1;
    GLuint arg1Rep;
    GLuint arg1Mod;
    GLuint arg2;
    GLuint arg2Rep;
    GLuint arg2Mod;
};

struct glAlphaFragmentOp3ATI_params
{
    TEB *teb;
    GLenum op;
    GLuint dst;
    GLuint dstMod;
    GLuint arg1;
    GLuint arg1Rep;
    GLuint arg1Mod;
    GLuint arg2;
    GLuint arg2Rep;
    GLuint arg2Mod;
    GLuint arg3;
    GLuint arg3Rep;
    GLuint arg3Mod;
};

struct glAlphaFuncxOES_params
{
    TEB *teb;
    GLenum func;
    GLfixed ref;
};

struct glAlphaToCoverageDitherControlNV_params
{
    TEB *teb;
    GLenum mode;
};

struct glApplyFramebufferAttachmentCMAAINTEL_params
{
    TEB *teb;
};

struct glApplyTextureEXT_params
{
    TEB *teb;
    GLenum mode;
};

struct glAreProgramsResidentNV_params
{
    TEB *teb;
    GLsizei n;
    const GLuint *programs;
    GLboolean *residences;
    GLboolean ret;
};

struct glAreTexturesResidentEXT_params
{
    TEB *teb;
    GLsizei n;
    const GLuint *textures;
    GLboolean *residences;
    GLboolean ret;
};

struct glArrayElementEXT_params
{
    TEB *teb;
    GLint i;
};

struct glArrayObjectATI_params
{
    TEB *teb;
    GLenum array;
    GLint size;
    GLenum type;
    GLsizei stride;
    GLuint buffer;
    GLuint offset;
};

struct glAsyncCopyBufferSubDataNVX_params
{
    TEB *teb;
    GLsizei waitSemaphoreCount;
    const GLuint *waitSemaphoreArray;
    const GLuint64 *fenceValueArray;
    GLuint readGpu;
    GLbitfield writeGpuMask;
    GLuint readBuffer;
    GLuint writeBuffer;
    GLintptr readOffset;
    GLintptr writeOffset;
    GLsizeiptr size;
    GLsizei signalSemaphoreCount;
    const GLuint *signalSemaphoreArray;
    const GLuint64 *signalValueArray;
    GLuint ret;
};

struct glAsyncCopyImageSubDataNVX_params
{
    TEB *teb;
    GLsizei waitSemaphoreCount;
    const GLuint *waitSemaphoreArray;
    const GLuint64 *waitValueArray;
    GLuint srcGpu;
    GLbitfield dstGpuMask;
    GLuint srcName;
    GLenum srcTarget;
    GLint srcLevel;
    GLint srcX;
    GLint srcY;
    GLint srcZ;
    GLuint dstName;
    GLenum dstTarget;
    GLint dstLevel;
    GLint dstX;
    GLint dstY;
    GLint dstZ;
    GLsizei srcWidth;
    GLsizei srcHeight;
    GLsizei srcDepth;
    GLsizei signalSemaphoreCount;
    const GLuint *signalSemaphoreArray;
    const GLuint64 *signalValueArray;
    GLuint ret;
};

struct glAsyncMarkerSGIX_params
{
    TEB *teb;
    GLuint marker;
};

struct glAttachObjectARB_params
{
    TEB *teb;
    GLhandleARB containerObj;
    GLhandleARB obj;
};

struct glAttachShader_params
{
    TEB *teb;
    GLuint program;
    GLuint shader;
};

struct glBeginConditionalRender_params
{
    TEB *teb;
    GLuint id;
    GLenum mode;
};

struct glBeginConditionalRenderNV_params
{
    TEB *teb;
    GLuint id;
    GLenum mode;
};

struct glBeginConditionalRenderNVX_params
{
    TEB *teb;
    GLuint id;
};

struct glBeginFragmentShaderATI_params
{
    TEB *teb;
};

struct glBeginOcclusionQueryNV_params
{
    TEB *teb;
    GLuint id;
};

struct glBeginPerfMonitorAMD_params
{
    TEB *teb;
    GLuint monitor;
};

struct glBeginPerfQueryINTEL_params
{
    TEB *teb;
    GLuint queryHandle;
};

struct glBeginQuery_params
{
    TEB *teb;
    GLenum target;
    GLuint id;
};

struct glBeginQueryARB_params
{
    TEB *teb;
    GLenum target;
    GLuint id;
};

struct glBeginQueryIndexed_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLuint id;
};

struct glBeginTransformFeedback_params
{
    TEB *teb;
    GLenum primitiveMode;
};

struct glBeginTransformFeedbackEXT_params
{
    TEB *teb;
    GLenum primitiveMode;
};

struct glBeginTransformFeedbackNV_params
{
    TEB *teb;
    GLenum primitiveMode;
};

struct glBeginVertexShaderEXT_params
{
    TEB *teb;
};

struct glBeginVideoCaptureNV_params
{
    TEB *teb;
    GLuint video_capture_slot;
};

struct glBindAttribLocation_params
{
    TEB *teb;
    GLuint program;
    GLuint index;
    const GLchar *name;
};

struct glBindAttribLocationARB_params
{
    TEB *teb;
    GLhandleARB programObj;
    GLuint index;
    const GLcharARB *name;
};

struct glBindBuffer_params
{
    TEB *teb;
    GLenum target;
    GLuint buffer;
};

struct glBindBufferARB_params
{
    TEB *teb;
    GLenum target;
    GLuint buffer;
};

struct glBindBufferBase_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLuint buffer;
};

struct glBindBufferBaseEXT_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLuint buffer;
};

struct glBindBufferBaseNV_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLuint buffer;
};

struct glBindBufferOffsetEXT_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLuint buffer;
    GLintptr offset;
};

struct glBindBufferOffsetNV_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLuint buffer;
    GLintptr offset;
};

struct glBindBufferRange_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLuint buffer;
    GLintptr offset;
    GLsizeiptr size;
};

struct glBindBufferRangeEXT_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLuint buffer;
    GLintptr offset;
    GLsizeiptr size;
};

struct glBindBufferRangeNV_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLuint buffer;
    GLintptr offset;
    GLsizeiptr size;
};

struct glBindBuffersBase_params
{
    TEB *teb;
    GLenum target;
    GLuint first;
    GLsizei count;
    const GLuint *buffers;
};

struct glBindBuffersRange_params
{
    TEB *teb;
    GLenum target;
    GLuint first;
    GLsizei count;
    const GLuint *buffers;
    const GLintptr *offsets;
    const GLsizeiptr *sizes;
};

struct glBindFragDataLocation_params
{
    TEB *teb;
    GLuint program;
    GLuint color;
    const GLchar *name;
};

struct glBindFragDataLocationEXT_params
{
    TEB *teb;
    GLuint program;
    GLuint color;
    const GLchar *name;
};

struct glBindFragDataLocationIndexed_params
{
    TEB *teb;
    GLuint program;
    GLuint colorNumber;
    GLuint index;
    const GLchar *name;
};

struct glBindFragmentShaderATI_params
{
    TEB *teb;
    GLuint id;
};

struct glBindFramebuffer_params
{
    TEB *teb;
    GLenum target;
    GLuint framebuffer;
};

struct glBindFramebufferEXT_params
{
    TEB *teb;
    GLenum target;
    GLuint framebuffer;
};

struct glBindImageTexture_params
{
    TEB *teb;
    GLuint unit;
    GLuint texture;
    GLint level;
    GLboolean layered;
    GLint layer;
    GLenum access;
    GLenum format;
};

struct glBindImageTextureEXT_params
{
    TEB *teb;
    GLuint index;
    GLuint texture;
    GLint level;
    GLboolean layered;
    GLint layer;
    GLenum access;
    GLint format;
};

struct glBindImageTextures_params
{
    TEB *teb;
    GLuint first;
    GLsizei count;
    const GLuint *textures;
};

struct glBindLightParameterEXT_params
{
    TEB *teb;
    GLenum light;
    GLenum value;
    GLuint ret;
};

struct glBindMaterialParameterEXT_params
{
    TEB *teb;
    GLenum face;
    GLenum value;
    GLuint ret;
};

struct glBindMultiTextureEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLuint texture;
};

struct glBindParameterEXT_params
{
    TEB *teb;
    GLenum value;
    GLuint ret;
};

struct glBindProgramARB_params
{
    TEB *teb;
    GLenum target;
    GLuint program;
};

struct glBindProgramNV_params
{
    TEB *teb;
    GLenum target;
    GLuint id;
};

struct glBindProgramPipeline_params
{
    TEB *teb;
    GLuint pipeline;
};

struct glBindRenderbuffer_params
{
    TEB *teb;
    GLenum target;
    GLuint renderbuffer;
};

struct glBindRenderbufferEXT_params
{
    TEB *teb;
    GLenum target;
    GLuint renderbuffer;
};

struct glBindSampler_params
{
    TEB *teb;
    GLuint unit;
    GLuint sampler;
};

struct glBindSamplers_params
{
    TEB *teb;
    GLuint first;
    GLsizei count;
    const GLuint *samplers;
};

struct glBindShadingRateImageNV_params
{
    TEB *teb;
    GLuint texture;
};

struct glBindTexGenParameterEXT_params
{
    TEB *teb;
    GLenum unit;
    GLenum coord;
    GLenum value;
    GLuint ret;
};

struct glBindTextureEXT_params
{
    TEB *teb;
    GLenum target;
    GLuint texture;
};

struct glBindTextureUnit_params
{
    TEB *teb;
    GLuint unit;
    GLuint texture;
};

struct glBindTextureUnitParameterEXT_params
{
    TEB *teb;
    GLenum unit;
    GLenum value;
    GLuint ret;
};

struct glBindTextures_params
{
    TEB *teb;
    GLuint first;
    GLsizei count;
    const GLuint *textures;
};

struct glBindTransformFeedback_params
{
    TEB *teb;
    GLenum target;
    GLuint id;
};

struct glBindTransformFeedbackNV_params
{
    TEB *teb;
    GLenum target;
    GLuint id;
};

struct glBindVertexArray_params
{
    TEB *teb;
    GLuint array;
};

struct glBindVertexArrayAPPLE_params
{
    TEB *teb;
    GLuint array;
};

struct glBindVertexBuffer_params
{
    TEB *teb;
    GLuint bindingindex;
    GLuint buffer;
    GLintptr offset;
    GLsizei stride;
};

struct glBindVertexBuffers_params
{
    TEB *teb;
    GLuint first;
    GLsizei count;
    const GLuint *buffers;
    const GLintptr *offsets;
    const GLsizei *strides;
};

struct glBindVertexShaderEXT_params
{
    TEB *teb;
    GLuint id;
};

struct glBindVideoCaptureStreamBufferNV_params
{
    TEB *teb;
    GLuint video_capture_slot;
    GLuint stream;
    GLenum frame_region;
    GLintptrARB offset;
};

struct glBindVideoCaptureStreamTextureNV_params
{
    TEB *teb;
    GLuint video_capture_slot;
    GLuint stream;
    GLenum frame_region;
    GLenum target;
    GLuint texture;
};

struct glBinormal3bEXT_params
{
    TEB *teb;
    GLbyte bx;
    GLbyte by;
    GLbyte bz;
};

struct glBinormal3bvEXT_params
{
    TEB *teb;
    const GLbyte *v;
};

struct glBinormal3dEXT_params
{
    TEB *teb;
    GLdouble bx;
    GLdouble by;
    GLdouble bz;
};

struct glBinormal3dvEXT_params
{
    TEB *teb;
    const GLdouble *v;
};

struct glBinormal3fEXT_params
{
    TEB *teb;
    GLfloat bx;
    GLfloat by;
    GLfloat bz;
};

struct glBinormal3fvEXT_params
{
    TEB *teb;
    const GLfloat *v;
};

struct glBinormal3iEXT_params
{
    TEB *teb;
    GLint bx;
    GLint by;
    GLint bz;
};

struct glBinormal3ivEXT_params
{
    TEB *teb;
    const GLint *v;
};

struct glBinormal3sEXT_params
{
    TEB *teb;
    GLshort bx;
    GLshort by;
    GLshort bz;
};

struct glBinormal3svEXT_params
{
    TEB *teb;
    const GLshort *v;
};

struct glBinormalPointerEXT_params
{
    TEB *teb;
    GLenum type;
    GLsizei stride;
    const void *pointer;
};

struct glBitmapxOES_params
{
    TEB *teb;
    GLsizei width;
    GLsizei height;
    GLfixed xorig;
    GLfixed yorig;
    GLfixed xmove;
    GLfixed ymove;
    const GLubyte *bitmap;
};

struct glBlendBarrierKHR_params
{
    TEB *teb;
};

struct glBlendBarrierNV_params
{
    TEB *teb;
};

struct glBlendColor_params
{
    TEB *teb;
    GLfloat red;
    GLfloat green;
    GLfloat blue;
    GLfloat alpha;
};

struct glBlendColorEXT_params
{
    TEB *teb;
    GLfloat red;
    GLfloat green;
    GLfloat blue;
    GLfloat alpha;
};

struct glBlendColorxOES_params
{
    TEB *teb;
    GLfixed red;
    GLfixed green;
    GLfixed blue;
    GLfixed alpha;
};

struct glBlendEquation_params
{
    TEB *teb;
    GLenum mode;
};

struct glBlendEquationEXT_params
{
    TEB *teb;
    GLenum mode;
};

struct glBlendEquationIndexedAMD_params
{
    TEB *teb;
    GLuint buf;
    GLenum mode;
};

struct glBlendEquationSeparate_params
{
    TEB *teb;
    GLenum modeRGB;
    GLenum modeAlpha;
};

struct glBlendEquationSeparateEXT_params
{
    TEB *teb;
    GLenum modeRGB;
    GLenum modeAlpha;
};

struct glBlendEquationSeparateIndexedAMD_params
{
    TEB *teb;
    GLuint buf;
    GLenum modeRGB;
    GLenum modeAlpha;
};

struct glBlendEquationSeparatei_params
{
    TEB *teb;
    GLuint buf;
    GLenum modeRGB;
    GLenum modeAlpha;
};

struct glBlendEquationSeparateiARB_params
{
    TEB *teb;
    GLuint buf;
    GLenum modeRGB;
    GLenum modeAlpha;
};

struct glBlendEquationi_params
{
    TEB *teb;
    GLuint buf;
    GLenum mode;
};

struct glBlendEquationiARB_params
{
    TEB *teb;
    GLuint buf;
    GLenum mode;
};

struct glBlendFuncIndexedAMD_params
{
    TEB *teb;
    GLuint buf;
    GLenum src;
    GLenum dst;
};

struct glBlendFuncSeparate_params
{
    TEB *teb;
    GLenum sfactorRGB;
    GLenum dfactorRGB;
    GLenum sfactorAlpha;
    GLenum dfactorAlpha;
};

struct glBlendFuncSeparateEXT_params
{
    TEB *teb;
    GLenum sfactorRGB;
    GLenum dfactorRGB;
    GLenum sfactorAlpha;
    GLenum dfactorAlpha;
};

struct glBlendFuncSeparateINGR_params
{
    TEB *teb;
    GLenum sfactorRGB;
    GLenum dfactorRGB;
    GLenum sfactorAlpha;
    GLenum dfactorAlpha;
};

struct glBlendFuncSeparateIndexedAMD_params
{
    TEB *teb;
    GLuint buf;
    GLenum srcRGB;
    GLenum dstRGB;
    GLenum srcAlpha;
    GLenum dstAlpha;
};

struct glBlendFuncSeparatei_params
{
    TEB *teb;
    GLuint buf;
    GLenum srcRGB;
    GLenum dstRGB;
    GLenum srcAlpha;
    GLenum dstAlpha;
};

struct glBlendFuncSeparateiARB_params
{
    TEB *teb;
    GLuint buf;
    GLenum srcRGB;
    GLenum dstRGB;
    GLenum srcAlpha;
    GLenum dstAlpha;
};

struct glBlendFunci_params
{
    TEB *teb;
    GLuint buf;
    GLenum src;
    GLenum dst;
};

struct glBlendFunciARB_params
{
    TEB *teb;
    GLuint buf;
    GLenum src;
    GLenum dst;
};

struct glBlendParameteriNV_params
{
    TEB *teb;
    GLenum pname;
    GLint value;
};

struct glBlitFramebuffer_params
{
    TEB *teb;
    GLint srcX0;
    GLint srcY0;
    GLint srcX1;
    GLint srcY1;
    GLint dstX0;
    GLint dstY0;
    GLint dstX1;
    GLint dstY1;
    GLbitfield mask;
    GLenum filter;
};

struct glBlitFramebufferEXT_params
{
    TEB *teb;
    GLint srcX0;
    GLint srcY0;
    GLint srcX1;
    GLint srcY1;
    GLint dstX0;
    GLint dstY0;
    GLint dstX1;
    GLint dstY1;
    GLbitfield mask;
    GLenum filter;
};

struct glBlitNamedFramebuffer_params
{
    TEB *teb;
    GLuint readFramebuffer;
    GLuint drawFramebuffer;
    GLint srcX0;
    GLint srcY0;
    GLint srcX1;
    GLint srcY1;
    GLint dstX0;
    GLint dstY0;
    GLint dstX1;
    GLint dstY1;
    GLbitfield mask;
    GLenum filter;
};

struct glBufferAddressRangeNV_params
{
    TEB *teb;
    GLenum pname;
    GLuint index;
    GLuint64EXT address;
    GLsizeiptr length;
};

struct glBufferAttachMemoryNV_params
{
    TEB *teb;
    GLenum target;
    GLuint memory;
    GLuint64 offset;
};

struct glBufferData_params
{
    TEB *teb;
    GLenum target;
    GLsizeiptr size;
    const void *data;
    GLenum usage;
};

struct glBufferDataARB_params
{
    TEB *teb;
    GLenum target;
    GLsizeiptrARB size;
    const void *data;
    GLenum usage;
};

struct glBufferPageCommitmentARB_params
{
    TEB *teb;
    GLenum target;
    GLintptr offset;
    GLsizeiptr size;
    GLboolean commit;
};

struct glBufferParameteriAPPLE_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLint param;
};

struct glBufferRegionEnabled_params
{
    TEB *teb;
    GLuint ret;
};

struct glBufferStorage_params
{
    TEB *teb;
    GLenum target;
    GLsizeiptr size;
    const void *data;
    GLbitfield flags;
};

struct glBufferStorageExternalEXT_params
{
    TEB *teb;
    GLenum target;
    GLintptr offset;
    GLsizeiptr size;
    GLeglClientBufferEXT clientBuffer;
    GLbitfield flags;
};

struct glBufferStorageMemEXT_params
{
    TEB *teb;
    GLenum target;
    GLsizeiptr size;
    GLuint memory;
    GLuint64 offset;
};

struct glBufferSubData_params
{
    TEB *teb;
    GLenum target;
    GLintptr offset;
    GLsizeiptr size;
    const void *data;
};

struct glBufferSubDataARB_params
{
    TEB *teb;
    GLenum target;
    GLintptrARB offset;
    GLsizeiptrARB size;
    const void *data;
};

struct glCallCommandListNV_params
{
    TEB *teb;
    GLuint list;
};

struct glCheckFramebufferStatus_params
{
    TEB *teb;
    GLenum target;
    GLenum ret;
};

struct glCheckFramebufferStatusEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum ret;
};

struct glCheckNamedFramebufferStatus_params
{
    TEB *teb;
    GLuint framebuffer;
    GLenum target;
    GLenum ret;
};

struct glCheckNamedFramebufferStatusEXT_params
{
    TEB *teb;
    GLuint framebuffer;
    GLenum target;
    GLenum ret;
};

struct glClampColor_params
{
    TEB *teb;
    GLenum target;
    GLenum clamp;
};

struct glClampColorARB_params
{
    TEB *teb;
    GLenum target;
    GLenum clamp;
};

struct glClearAccumxOES_params
{
    TEB *teb;
    GLfixed red;
    GLfixed green;
    GLfixed blue;
    GLfixed alpha;
};

struct glClearBufferData_params
{
    TEB *teb;
    GLenum target;
    GLenum internalformat;
    GLenum format;
    GLenum type;
    const void *data;
};

struct glClearBufferSubData_params
{
    TEB *teb;
    GLenum target;
    GLenum internalformat;
    GLintptr offset;
    GLsizeiptr size;
    GLenum format;
    GLenum type;
    const void *data;
};

struct glClearBufferfi_params
{
    TEB *teb;
    GLenum buffer;
    GLint drawbuffer;
    GLfloat depth;
    GLint stencil;
};

struct glClearBufferfv_params
{
    TEB *teb;
    GLenum buffer;
    GLint drawbuffer;
    const GLfloat *value;
};

struct glClearBufferiv_params
{
    TEB *teb;
    GLenum buffer;
    GLint drawbuffer;
    const GLint *value;
};

struct glClearBufferuiv_params
{
    TEB *teb;
    GLenum buffer;
    GLint drawbuffer;
    const GLuint *value;
};

struct glClearColorIiEXT_params
{
    TEB *teb;
    GLint red;
    GLint green;
    GLint blue;
    GLint alpha;
};

struct glClearColorIuiEXT_params
{
    TEB *teb;
    GLuint red;
    GLuint green;
    GLuint blue;
    GLuint alpha;
};

struct glClearColorxOES_params
{
    TEB *teb;
    GLfixed red;
    GLfixed green;
    GLfixed blue;
    GLfixed alpha;
};

struct glClearDepthdNV_params
{
    TEB *teb;
    GLdouble depth;
};

struct glClearDepthf_params
{
    TEB *teb;
    GLfloat d;
};

struct glClearDepthfOES_params
{
    TEB *teb;
    GLclampf depth;
};

struct glClearDepthxOES_params
{
    TEB *teb;
    GLfixed depth;
};

struct glClearNamedBufferData_params
{
    TEB *teb;
    GLuint buffer;
    GLenum internalformat;
    GLenum format;
    GLenum type;
    const void *data;
};

struct glClearNamedBufferDataEXT_params
{
    TEB *teb;
    GLuint buffer;
    GLenum internalformat;
    GLenum format;
    GLenum type;
    const void *data;
};

struct glClearNamedBufferSubData_params
{
    TEB *teb;
    GLuint buffer;
    GLenum internalformat;
    GLintptr offset;
    GLsizeiptr size;
    GLenum format;
    GLenum type;
    const void *data;
};

struct glClearNamedBufferSubDataEXT_params
{
    TEB *teb;
    GLuint buffer;
    GLenum internalformat;
    GLsizeiptr offset;
    GLsizeiptr size;
    GLenum format;
    GLenum type;
    const void *data;
};

struct glClearNamedFramebufferfi_params
{
    TEB *teb;
    GLuint framebuffer;
    GLenum buffer;
    GLint drawbuffer;
    GLfloat depth;
    GLint stencil;
};

struct glClearNamedFramebufferfv_params
{
    TEB *teb;
    GLuint framebuffer;
    GLenum buffer;
    GLint drawbuffer;
    const GLfloat *value;
};

struct glClearNamedFramebufferiv_params
{
    TEB *teb;
    GLuint framebuffer;
    GLenum buffer;
    GLint drawbuffer;
    const GLint *value;
};

struct glClearNamedFramebufferuiv_params
{
    TEB *teb;
    GLuint framebuffer;
    GLenum buffer;
    GLint drawbuffer;
    const GLuint *value;
};

struct glClearTexImage_params
{
    TEB *teb;
    GLuint texture;
    GLint level;
    GLenum format;
    GLenum type;
    const void *data;
};

struct glClearTexSubImage_params
{
    TEB *teb;
    GLuint texture;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLint zoffset;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLenum format;
    GLenum type;
    const void *data;
};

struct glClientActiveTexture_params
{
    TEB *teb;
    GLenum texture;
};

struct glClientActiveTextureARB_params
{
    TEB *teb;
    GLenum texture;
};

struct glClientActiveVertexStreamATI_params
{
    TEB *teb;
    GLenum stream;
};

struct glClientAttribDefaultEXT_params
{
    TEB *teb;
    GLbitfield mask;
};

struct glClientWaitSemaphoreui64NVX_params
{
    TEB *teb;
    GLsizei fenceObjectCount;
    const GLuint *semaphoreArray;
    const GLuint64 *fenceValueArray;
};

struct glClientWaitSync_params
{
    TEB *teb;
    GLsync sync;
    GLbitfield flags;
    GLuint64 timeout;
    GLenum ret;
};

struct glClipControl_params
{
    TEB *teb;
    GLenum origin;
    GLenum depth;
};

struct glClipPlanefOES_params
{
    TEB *teb;
    GLenum plane;
    const GLfloat *equation;
};

struct glClipPlanexOES_params
{
    TEB *teb;
    GLenum plane;
    const GLfixed *equation;
};

struct glColor3fVertex3fSUN_params
{
    TEB *teb;
    GLfloat r;
    GLfloat g;
    GLfloat b;
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

struct glColor3fVertex3fvSUN_params
{
    TEB *teb;
    const GLfloat *c;
    const GLfloat *v;
};

struct glColor3hNV_params
{
    TEB *teb;
    GLhalfNV red;
    GLhalfNV green;
    GLhalfNV blue;
};

struct glColor3hvNV_params
{
    TEB *teb;
    const GLhalfNV *v;
};

struct glColor3xOES_params
{
    TEB *teb;
    GLfixed red;
    GLfixed green;
    GLfixed blue;
};

struct glColor3xvOES_params
{
    TEB *teb;
    const GLfixed *components;
};

struct glColor4fNormal3fVertex3fSUN_params
{
    TEB *teb;
    GLfloat r;
    GLfloat g;
    GLfloat b;
    GLfloat a;
    GLfloat nx;
    GLfloat ny;
    GLfloat nz;
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

struct glColor4fNormal3fVertex3fvSUN_params
{
    TEB *teb;
    const GLfloat *c;
    const GLfloat *n;
    const GLfloat *v;
};

struct glColor4hNV_params
{
    TEB *teb;
    GLhalfNV red;
    GLhalfNV green;
    GLhalfNV blue;
    GLhalfNV alpha;
};

struct glColor4hvNV_params
{
    TEB *teb;
    const GLhalfNV *v;
};

struct glColor4ubVertex2fSUN_params
{
    TEB *teb;
    GLubyte r;
    GLubyte g;
    GLubyte b;
    GLubyte a;
    GLfloat x;
    GLfloat y;
};

struct glColor4ubVertex2fvSUN_params
{
    TEB *teb;
    const GLubyte *c;
    const GLfloat *v;
};

struct glColor4ubVertex3fSUN_params
{
    TEB *teb;
    GLubyte r;
    GLubyte g;
    GLubyte b;
    GLubyte a;
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

struct glColor4ubVertex3fvSUN_params
{
    TEB *teb;
    const GLubyte *c;
    const GLfloat *v;
};

struct glColor4xOES_params
{
    TEB *teb;
    GLfixed red;
    GLfixed green;
    GLfixed blue;
    GLfixed alpha;
};

struct glColor4xvOES_params
{
    TEB *teb;
    const GLfixed *components;
};

struct glColorFormatNV_params
{
    TEB *teb;
    GLint size;
    GLenum type;
    GLsizei stride;
};

struct glColorFragmentOp1ATI_params
{
    TEB *teb;
    GLenum op;
    GLuint dst;
    GLuint dstMask;
    GLuint dstMod;
    GLuint arg1;
    GLuint arg1Rep;
    GLuint arg1Mod;
};

struct glColorFragmentOp2ATI_params
{
    TEB *teb;
    GLenum op;
    GLuint dst;
    GLuint dstMask;
    GLuint dstMod;
    GLuint arg1;
    GLuint arg1Rep;
    GLuint arg1Mod;
    GLuint arg2;
    GLuint arg2Rep;
    GLuint arg2Mod;
};

struct glColorFragmentOp3ATI_params
{
    TEB *teb;
    GLenum op;
    GLuint dst;
    GLuint dstMask;
    GLuint dstMod;
    GLuint arg1;
    GLuint arg1Rep;
    GLuint arg1Mod;
    GLuint arg2;
    GLuint arg2Rep;
    GLuint arg2Mod;
    GLuint arg3;
    GLuint arg3Rep;
    GLuint arg3Mod;
};

struct glColorMaskIndexedEXT_params
{
    TEB *teb;
    GLuint index;
    GLboolean r;
    GLboolean g;
    GLboolean b;
    GLboolean a;
};

struct glColorMaski_params
{
    TEB *teb;
    GLuint index;
    GLboolean r;
    GLboolean g;
    GLboolean b;
    GLboolean a;
};

struct glColorP3ui_params
{
    TEB *teb;
    GLenum type;
    GLuint color;
};

struct glColorP3uiv_params
{
    TEB *teb;
    GLenum type;
    const GLuint *color;
};

struct glColorP4ui_params
{
    TEB *teb;
    GLenum type;
    GLuint color;
};

struct glColorP4uiv_params
{
    TEB *teb;
    GLenum type;
    const GLuint *color;
};

struct glColorPointerEXT_params
{
    TEB *teb;
    GLint size;
    GLenum type;
    GLsizei stride;
    GLsizei count;
    const void *pointer;
};

struct glColorPointerListIBM_params
{
    TEB *teb;
    GLint size;
    GLenum type;
    GLint stride;
    const void **pointer;
    GLint ptrstride;
};

struct glColorPointervINTEL_params
{
    TEB *teb;
    GLint size;
    GLenum type;
    const void **pointer;
};

struct glColorSubTable_params
{
    TEB *teb;
    GLenum target;
    GLsizei start;
    GLsizei count;
    GLenum format;
    GLenum type;
    const void *data;
};

struct glColorSubTableEXT_params
{
    TEB *teb;
    GLenum target;
    GLsizei start;
    GLsizei count;
    GLenum format;
    GLenum type;
    const void *data;
};

struct glColorTable_params
{
    TEB *teb;
    GLenum target;
    GLenum internalformat;
    GLsizei width;
    GLenum format;
    GLenum type;
    const void *table;
};

struct glColorTableEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum internalFormat;
    GLsizei width;
    GLenum format;
    GLenum type;
    const void *table;
};

struct glColorTableParameterfv_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    const GLfloat *params;
};

struct glColorTableParameterfvSGI_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    const GLfloat *params;
};

struct glColorTableParameteriv_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    const GLint *params;
};

struct glColorTableParameterivSGI_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    const GLint *params;
};

struct glColorTableSGI_params
{
    TEB *teb;
    GLenum target;
    GLenum internalformat;
    GLsizei width;
    GLenum format;
    GLenum type;
    const void *table;
};

struct glCombinerInputNV_params
{
    TEB *teb;
    GLenum stage;
    GLenum portion;
    GLenum variable;
    GLenum input;
    GLenum mapping;
    GLenum componentUsage;
};

struct glCombinerOutputNV_params
{
    TEB *teb;
    GLenum stage;
    GLenum portion;
    GLenum abOutput;
    GLenum cdOutput;
    GLenum sumOutput;
    GLenum scale;
    GLenum bias;
    GLboolean abDotProduct;
    GLboolean cdDotProduct;
    GLboolean muxSum;
};

struct glCombinerParameterfNV_params
{
    TEB *teb;
    GLenum pname;
    GLfloat param;
};

struct glCombinerParameterfvNV_params
{
    TEB *teb;
    GLenum pname;
    const GLfloat *params;
};

struct glCombinerParameteriNV_params
{
    TEB *teb;
    GLenum pname;
    GLint param;
};

struct glCombinerParameterivNV_params
{
    TEB *teb;
    GLenum pname;
    const GLint *params;
};

struct glCombinerStageParameterfvNV_params
{
    TEB *teb;
    GLenum stage;
    GLenum pname;
    const GLfloat *params;
};

struct glCommandListSegmentsNV_params
{
    TEB *teb;
    GLuint list;
    GLuint segments;
};

struct glCompileCommandListNV_params
{
    TEB *teb;
    GLuint list;
};

struct glCompileShader_params
{
    TEB *teb;
    GLuint shader;
};

struct glCompileShaderARB_params
{
    TEB *teb;
    GLhandleARB shaderObj;
};

struct glCompileShaderIncludeARB_params
{
    TEB *teb;
    GLuint shader;
    GLsizei count;
    const GLchar *const*path;
    const GLint *length;
};

struct glCompressedMultiTexImage1DEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLint level;
    GLenum internalformat;
    GLsizei width;
    GLint border;
    GLsizei imageSize;
    const void *bits;
};

struct glCompressedMultiTexImage2DEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLint level;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
    GLint border;
    GLsizei imageSize;
    const void *bits;
};

struct glCompressedMultiTexImage3DEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLint level;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLint border;
    GLsizei imageSize;
    const void *bits;
};

struct glCompressedMultiTexSubImage1DEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLsizei width;
    GLenum format;
    GLsizei imageSize;
    const void *bits;
};

struct glCompressedMultiTexSubImage2DEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLsizei width;
    GLsizei height;
    GLenum format;
    GLsizei imageSize;
    const void *bits;
};

struct glCompressedMultiTexSubImage3DEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLint zoffset;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLenum format;
    GLsizei imageSize;
    const void *bits;
};

struct glCompressedTexImage1D_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLenum internalformat;
    GLsizei width;
    GLint border;
    GLsizei imageSize;
    const void *data;
};

struct glCompressedTexImage1DARB_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLenum internalformat;
    GLsizei width;
    GLint border;
    GLsizei imageSize;
    const void *data;
};

struct glCompressedTexImage2D_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
    GLint border;
    GLsizei imageSize;
    const void *data;
};

struct glCompressedTexImage2DARB_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
    GLint border;
    GLsizei imageSize;
    const void *data;
};

struct glCompressedTexImage3D_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLint border;
    GLsizei imageSize;
    const void *data;
};

struct glCompressedTexImage3DARB_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLint border;
    GLsizei imageSize;
    const void *data;
};

struct glCompressedTexSubImage1D_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLsizei width;
    GLenum format;
    GLsizei imageSize;
    const void *data;
};

struct glCompressedTexSubImage1DARB_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLsizei width;
    GLenum format;
    GLsizei imageSize;
    const void *data;
};

struct glCompressedTexSubImage2D_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLsizei width;
    GLsizei height;
    GLenum format;
    GLsizei imageSize;
    const void *data;
};

struct glCompressedTexSubImage2DARB_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLsizei width;
    GLsizei height;
    GLenum format;
    GLsizei imageSize;
    const void *data;
};

struct glCompressedTexSubImage3D_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLint zoffset;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLenum format;
    GLsizei imageSize;
    const void *data;
};

struct glCompressedTexSubImage3DARB_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLint zoffset;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLenum format;
    GLsizei imageSize;
    const void *data;
};

struct glCompressedTextureImage1DEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLint level;
    GLenum internalformat;
    GLsizei width;
    GLint border;
    GLsizei imageSize;
    const void *bits;
};

struct glCompressedTextureImage2DEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLint level;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
    GLint border;
    GLsizei imageSize;
    const void *bits;
};

struct glCompressedTextureImage3DEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLint level;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLint border;
    GLsizei imageSize;
    const void *bits;
};

struct glCompressedTextureSubImage1D_params
{
    TEB *teb;
    GLuint texture;
    GLint level;
    GLint xoffset;
    GLsizei width;
    GLenum format;
    GLsizei imageSize;
    const void *data;
};

struct glCompressedTextureSubImage1DEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLsizei width;
    GLenum format;
    GLsizei imageSize;
    const void *bits;
};

struct glCompressedTextureSubImage2D_params
{
    TEB *teb;
    GLuint texture;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLsizei width;
    GLsizei height;
    GLenum format;
    GLsizei imageSize;
    const void *data;
};

struct glCompressedTextureSubImage2DEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLsizei width;
    GLsizei height;
    GLenum format;
    GLsizei imageSize;
    const void *bits;
};

struct glCompressedTextureSubImage3D_params
{
    TEB *teb;
    GLuint texture;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLint zoffset;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLenum format;
    GLsizei imageSize;
    const void *data;
};

struct glCompressedTextureSubImage3DEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLint zoffset;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLenum format;
    GLsizei imageSize;
    const void *bits;
};

struct glConservativeRasterParameterfNV_params
{
    TEB *teb;
    GLenum pname;
    GLfloat value;
};

struct glConservativeRasterParameteriNV_params
{
    TEB *teb;
    GLenum pname;
    GLint param;
};

struct glConvolutionFilter1D_params
{
    TEB *teb;
    GLenum target;
    GLenum internalformat;
    GLsizei width;
    GLenum format;
    GLenum type;
    const void *image;
};

struct glConvolutionFilter1DEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum internalformat;
    GLsizei width;
    GLenum format;
    GLenum type;
    const void *image;
};

struct glConvolutionFilter2D_params
{
    TEB *teb;
    GLenum target;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
    GLenum format;
    GLenum type;
    const void *image;
};

struct glConvolutionFilter2DEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
    GLenum format;
    GLenum type;
    const void *image;
};

struct glConvolutionParameterf_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLfloat params;
};

struct glConvolutionParameterfEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLfloat params;
};

struct glConvolutionParameterfv_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    const GLfloat *params;
};

struct glConvolutionParameterfvEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    const GLfloat *params;
};

struct glConvolutionParameteri_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLint params;
};

struct glConvolutionParameteriEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLint params;
};

struct glConvolutionParameteriv_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    const GLint *params;
};

struct glConvolutionParameterivEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    const GLint *params;
};

struct glConvolutionParameterxOES_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLfixed param;
};

struct glConvolutionParameterxvOES_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    const GLfixed *params;
};

struct glCopyBufferSubData_params
{
    TEB *teb;
    GLenum readTarget;
    GLenum writeTarget;
    GLintptr readOffset;
    GLintptr writeOffset;
    GLsizeiptr size;
};

struct glCopyColorSubTable_params
{
    TEB *teb;
    GLenum target;
    GLsizei start;
    GLint x;
    GLint y;
    GLsizei width;
};

struct glCopyColorSubTableEXT_params
{
    TEB *teb;
    GLenum target;
    GLsizei start;
    GLint x;
    GLint y;
    GLsizei width;
};

struct glCopyColorTable_params
{
    TEB *teb;
    GLenum target;
    GLenum internalformat;
    GLint x;
    GLint y;
    GLsizei width;
};

struct glCopyColorTableSGI_params
{
    TEB *teb;
    GLenum target;
    GLenum internalformat;
    GLint x;
    GLint y;
    GLsizei width;
};

struct glCopyConvolutionFilter1D_params
{
    TEB *teb;
    GLenum target;
    GLenum internalformat;
    GLint x;
    GLint y;
    GLsizei width;
};

struct glCopyConvolutionFilter1DEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum internalformat;
    GLint x;
    GLint y;
    GLsizei width;
};

struct glCopyConvolutionFilter2D_params
{
    TEB *teb;
    GLenum target;
    GLenum internalformat;
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
};

struct glCopyConvolutionFilter2DEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum internalformat;
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
};

struct glCopyImageSubData_params
{
    TEB *teb;
    GLuint srcName;
    GLenum srcTarget;
    GLint srcLevel;
    GLint srcX;
    GLint srcY;
    GLint srcZ;
    GLuint dstName;
    GLenum dstTarget;
    GLint dstLevel;
    GLint dstX;
    GLint dstY;
    GLint dstZ;
    GLsizei srcWidth;
    GLsizei srcHeight;
    GLsizei srcDepth;
};

struct glCopyImageSubDataNV_params
{
    TEB *teb;
    GLuint srcName;
    GLenum srcTarget;
    GLint srcLevel;
    GLint srcX;
    GLint srcY;
    GLint srcZ;
    GLuint dstName;
    GLenum dstTarget;
    GLint dstLevel;
    GLint dstX;
    GLint dstY;
    GLint dstZ;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
};

struct glCopyMultiTexImage1DEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLint level;
    GLenum internalformat;
    GLint x;
    GLint y;
    GLsizei width;
    GLint border;
};

struct glCopyMultiTexImage2DEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLint level;
    GLenum internalformat;
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
    GLint border;
};

struct glCopyMultiTexSubImage1DEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint x;
    GLint y;
    GLsizei width;
};

struct glCopyMultiTexSubImage2DEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
};

struct glCopyMultiTexSubImage3DEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLint zoffset;
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
};

struct glCopyNamedBufferSubData_params
{
    TEB *teb;
    GLuint readBuffer;
    GLuint writeBuffer;
    GLintptr readOffset;
    GLintptr writeOffset;
    GLsizeiptr size;
};

struct glCopyPathNV_params
{
    TEB *teb;
    GLuint resultPath;
    GLuint srcPath;
};

struct glCopyTexImage1DEXT_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLenum internalformat;
    GLint x;
    GLint y;
    GLsizei width;
    GLint border;
};

struct glCopyTexImage2DEXT_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLenum internalformat;
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
    GLint border;
};

struct glCopyTexSubImage1DEXT_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint x;
    GLint y;
    GLsizei width;
};

struct glCopyTexSubImage2DEXT_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
};

struct glCopyTexSubImage3D_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLint zoffset;
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
};

struct glCopyTexSubImage3DEXT_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLint zoffset;
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
};

struct glCopyTextureImage1DEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLint level;
    GLenum internalformat;
    GLint x;
    GLint y;
    GLsizei width;
    GLint border;
};

struct glCopyTextureImage2DEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLint level;
    GLenum internalformat;
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
    GLint border;
};

struct glCopyTextureSubImage1D_params
{
    TEB *teb;
    GLuint texture;
    GLint level;
    GLint xoffset;
    GLint x;
    GLint y;
    GLsizei width;
};

struct glCopyTextureSubImage1DEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint x;
    GLint y;
    GLsizei width;
};

struct glCopyTextureSubImage2D_params
{
    TEB *teb;
    GLuint texture;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
};

struct glCopyTextureSubImage2DEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
};

struct glCopyTextureSubImage3D_params
{
    TEB *teb;
    GLuint texture;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLint zoffset;
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
};

struct glCopyTextureSubImage3DEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLint zoffset;
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
};

struct glCoverFillPathInstancedNV_params
{
    TEB *teb;
    GLsizei numPaths;
    GLenum pathNameType;
    const void *paths;
    GLuint pathBase;
    GLenum coverMode;
    GLenum transformType;
    const GLfloat *transformValues;
};

struct glCoverFillPathNV_params
{
    TEB *teb;
    GLuint path;
    GLenum coverMode;
};

struct glCoverStrokePathInstancedNV_params
{
    TEB *teb;
    GLsizei numPaths;
    GLenum pathNameType;
    const void *paths;
    GLuint pathBase;
    GLenum coverMode;
    GLenum transformType;
    const GLfloat *transformValues;
};

struct glCoverStrokePathNV_params
{
    TEB *teb;
    GLuint path;
    GLenum coverMode;
};

struct glCoverageModulationNV_params
{
    TEB *teb;
    GLenum components;
};

struct glCoverageModulationTableNV_params
{
    TEB *teb;
    GLsizei n;
    const GLfloat *v;
};

struct glCreateBuffers_params
{
    TEB *teb;
    GLsizei n;
    GLuint *buffers;
};

struct glCreateCommandListsNV_params
{
    TEB *teb;
    GLsizei n;
    GLuint *lists;
};

struct glCreateFramebuffers_params
{
    TEB *teb;
    GLsizei n;
    GLuint *framebuffers;
};

struct glCreateMemoryObjectsEXT_params
{
    TEB *teb;
    GLsizei n;
    GLuint *memoryObjects;
};

struct glCreatePerfQueryINTEL_params
{
    TEB *teb;
    GLuint queryId;
    GLuint *queryHandle;
};

struct glCreateProgram_params
{
    TEB *teb;
    GLuint ret;
};

struct glCreateProgramObjectARB_params
{
    TEB *teb;
    GLhandleARB ret;
};

struct glCreateProgramPipelines_params
{
    TEB *teb;
    GLsizei n;
    GLuint *pipelines;
};

struct glCreateProgressFenceNVX_params
{
    TEB *teb;
    GLuint ret;
};

struct glCreateQueries_params
{
    TEB *teb;
    GLenum target;
    GLsizei n;
    GLuint *ids;
};

struct glCreateRenderbuffers_params
{
    TEB *teb;
    GLsizei n;
    GLuint *renderbuffers;
};

struct glCreateSamplers_params
{
    TEB *teb;
    GLsizei n;
    GLuint *samplers;
};

struct glCreateShader_params
{
    TEB *teb;
    GLenum type;
    GLuint ret;
};

struct glCreateShaderObjectARB_params
{
    TEB *teb;
    GLenum shaderType;
    GLhandleARB ret;
};

struct glCreateShaderProgramEXT_params
{
    TEB *teb;
    GLenum type;
    const GLchar *string;
    GLuint ret;
};

struct glCreateShaderProgramv_params
{
    TEB *teb;
    GLenum type;
    GLsizei count;
    const GLchar *const*strings;
    GLuint ret;
};

struct glCreateStatesNV_params
{
    TEB *teb;
    GLsizei n;
    GLuint *states;
};

struct glCreateSyncFromCLeventARB_params
{
    TEB *teb;
    struct _cl_context *context;
    struct _cl_event *event;
    GLbitfield flags;
    GLsync ret;
};

struct glCreateTextures_params
{
    TEB *teb;
    GLenum target;
    GLsizei n;
    GLuint *textures;
};

struct glCreateTransformFeedbacks_params
{
    TEB *teb;
    GLsizei n;
    GLuint *ids;
};

struct glCreateVertexArrays_params
{
    TEB *teb;
    GLsizei n;
    GLuint *arrays;
};

struct glCullParameterdvEXT_params
{
    TEB *teb;
    GLenum pname;
    GLdouble *params;
};

struct glCullParameterfvEXT_params
{
    TEB *teb;
    GLenum pname;
    GLfloat *params;
};

struct glCurrentPaletteMatrixARB_params
{
    TEB *teb;
    GLint index;
};

struct glDebugMessageCallback_params
{
    TEB *teb;
    GLDEBUGPROC callback;
    const void *userParam;
};

struct glDebugMessageCallbackAMD_params
{
    TEB *teb;
    GLDEBUGPROCAMD callback;
    void *userParam;
};

struct glDebugMessageCallbackARB_params
{
    TEB *teb;
    GLDEBUGPROCARB callback;
    const void *userParam;
};

struct glDebugMessageControl_params
{
    TEB *teb;
    GLenum source;
    GLenum type;
    GLenum severity;
    GLsizei count;
    const GLuint *ids;
    GLboolean enabled;
};

struct glDebugMessageControlARB_params
{
    TEB *teb;
    GLenum source;
    GLenum type;
    GLenum severity;
    GLsizei count;
    const GLuint *ids;
    GLboolean enabled;
};

struct glDebugMessageEnableAMD_params
{
    TEB *teb;
    GLenum category;
    GLenum severity;
    GLsizei count;
    const GLuint *ids;
    GLboolean enabled;
};

struct glDebugMessageInsert_params
{
    TEB *teb;
    GLenum source;
    GLenum type;
    GLuint id;
    GLenum severity;
    GLsizei length;
    const GLchar *buf;
};

struct glDebugMessageInsertAMD_params
{
    TEB *teb;
    GLenum category;
    GLenum severity;
    GLuint id;
    GLsizei length;
    const GLchar *buf;
};

struct glDebugMessageInsertARB_params
{
    TEB *teb;
    GLenum source;
    GLenum type;
    GLuint id;
    GLenum severity;
    GLsizei length;
    const GLchar *buf;
};

struct glDeformSGIX_params
{
    TEB *teb;
    GLbitfield mask;
};

struct glDeformationMap3dSGIX_params
{
    TEB *teb;
    GLenum target;
    GLdouble u1;
    GLdouble u2;
    GLint ustride;
    GLint uorder;
    GLdouble v1;
    GLdouble v2;
    GLint vstride;
    GLint vorder;
    GLdouble w1;
    GLdouble w2;
    GLint wstride;
    GLint worder;
    const GLdouble *points;
};

struct glDeformationMap3fSGIX_params
{
    TEB *teb;
    GLenum target;
    GLfloat u1;
    GLfloat u2;
    GLint ustride;
    GLint uorder;
    GLfloat v1;
    GLfloat v2;
    GLint vstride;
    GLint vorder;
    GLfloat w1;
    GLfloat w2;
    GLint wstride;
    GLint worder;
    const GLfloat *points;
};

struct glDeleteAsyncMarkersSGIX_params
{
    TEB *teb;
    GLuint marker;
    GLsizei range;
};

struct glDeleteBufferRegion_params
{
    TEB *teb;
    GLenum region;
};

struct glDeleteBuffers_params
{
    TEB *teb;
    GLsizei n;
    const GLuint *buffers;
};

struct glDeleteBuffersARB_params
{
    TEB *teb;
    GLsizei n;
    const GLuint *buffers;
};

struct glDeleteCommandListsNV_params
{
    TEB *teb;
    GLsizei n;
    const GLuint *lists;
};

struct glDeleteFencesAPPLE_params
{
    TEB *teb;
    GLsizei n;
    const GLuint *fences;
};

struct glDeleteFencesNV_params
{
    TEB *teb;
    GLsizei n;
    const GLuint *fences;
};

struct glDeleteFragmentShaderATI_params
{
    TEB *teb;
    GLuint id;
};

struct glDeleteFramebuffers_params
{
    TEB *teb;
    GLsizei n;
    const GLuint *framebuffers;
};

struct glDeleteFramebuffersEXT_params
{
    TEB *teb;
    GLsizei n;
    const GLuint *framebuffers;
};

struct glDeleteMemoryObjectsEXT_params
{
    TEB *teb;
    GLsizei n;
    const GLuint *memoryObjects;
};

struct glDeleteNamedStringARB_params
{
    TEB *teb;
    GLint namelen;
    const GLchar *name;
};

struct glDeleteNamesAMD_params
{
    TEB *teb;
    GLenum identifier;
    GLuint num;
    const GLuint *names;
};

struct glDeleteObjectARB_params
{
    TEB *teb;
    GLhandleARB obj;
};

struct glDeleteObjectBufferATI_params
{
    TEB *teb;
    GLuint buffer;
};

struct glDeleteOcclusionQueriesNV_params
{
    TEB *teb;
    GLsizei n;
    const GLuint *ids;
};

struct glDeletePathsNV_params
{
    TEB *teb;
    GLuint path;
    GLsizei range;
};

struct glDeletePerfMonitorsAMD_params
{
    TEB *teb;
    GLsizei n;
    GLuint *monitors;
};

struct glDeletePerfQueryINTEL_params
{
    TEB *teb;
    GLuint queryHandle;
};

struct glDeleteProgram_params
{
    TEB *teb;
    GLuint program;
};

struct glDeleteProgramPipelines_params
{
    TEB *teb;
    GLsizei n;
    const GLuint *pipelines;
};

struct glDeleteProgramsARB_params
{
    TEB *teb;
    GLsizei n;
    const GLuint *programs;
};

struct glDeleteProgramsNV_params
{
    TEB *teb;
    GLsizei n;
    const GLuint *programs;
};

struct glDeleteQueries_params
{
    TEB *teb;
    GLsizei n;
    const GLuint *ids;
};

struct glDeleteQueriesARB_params
{
    TEB *teb;
    GLsizei n;
    const GLuint *ids;
};

struct glDeleteQueryResourceTagNV_params
{
    TEB *teb;
    GLsizei n;
    const GLint *tagIds;
};

struct glDeleteRenderbuffers_params
{
    TEB *teb;
    GLsizei n;
    const GLuint *renderbuffers;
};

struct glDeleteRenderbuffersEXT_params
{
    TEB *teb;
    GLsizei n;
    const GLuint *renderbuffers;
};

struct glDeleteSamplers_params
{
    TEB *teb;
    GLsizei count;
    const GLuint *samplers;
};

struct glDeleteSemaphoresEXT_params
{
    TEB *teb;
    GLsizei n;
    const GLuint *semaphores;
};

struct glDeleteShader_params
{
    TEB *teb;
    GLuint shader;
};

struct glDeleteStatesNV_params
{
    TEB *teb;
    GLsizei n;
    const GLuint *states;
};

struct glDeleteSync_params
{
    TEB *teb;
    GLsync sync;
};

struct glDeleteTexturesEXT_params
{
    TEB *teb;
    GLsizei n;
    const GLuint *textures;
};

struct glDeleteTransformFeedbacks_params
{
    TEB *teb;
    GLsizei n;
    const GLuint *ids;
};

struct glDeleteTransformFeedbacksNV_params
{
    TEB *teb;
    GLsizei n;
    const GLuint *ids;
};

struct glDeleteVertexArrays_params
{
    TEB *teb;
    GLsizei n;
    const GLuint *arrays;
};

struct glDeleteVertexArraysAPPLE_params
{
    TEB *teb;
    GLsizei n;
    const GLuint *arrays;
};

struct glDeleteVertexShaderEXT_params
{
    TEB *teb;
    GLuint id;
};

struct glDepthBoundsEXT_params
{
    TEB *teb;
    GLclampd zmin;
    GLclampd zmax;
};

struct glDepthBoundsdNV_params
{
    TEB *teb;
    GLdouble zmin;
    GLdouble zmax;
};

struct glDepthRangeArraydvNV_params
{
    TEB *teb;
    GLuint first;
    GLsizei count;
    const GLdouble *v;
};

struct glDepthRangeArrayv_params
{
    TEB *teb;
    GLuint first;
    GLsizei count;
    const GLdouble *v;
};

struct glDepthRangeIndexed_params
{
    TEB *teb;
    GLuint index;
    GLdouble n;
    GLdouble f;
};

struct glDepthRangeIndexeddNV_params
{
    TEB *teb;
    GLuint index;
    GLdouble n;
    GLdouble f;
};

struct glDepthRangedNV_params
{
    TEB *teb;
    GLdouble zNear;
    GLdouble zFar;
};

struct glDepthRangef_params
{
    TEB *teb;
    GLfloat n;
    GLfloat f;
};

struct glDepthRangefOES_params
{
    TEB *teb;
    GLclampf n;
    GLclampf f;
};

struct glDepthRangexOES_params
{
    TEB *teb;
    GLfixed n;
    GLfixed f;
};

struct glDetachObjectARB_params
{
    TEB *teb;
    GLhandleARB containerObj;
    GLhandleARB attachedObj;
};

struct glDetachShader_params
{
    TEB *teb;
    GLuint program;
    GLuint shader;
};

struct glDetailTexFuncSGIS_params
{
    TEB *teb;
    GLenum target;
    GLsizei n;
    const GLfloat *points;
};

struct glDisableClientStateIndexedEXT_params
{
    TEB *teb;
    GLenum array;
    GLuint index;
};

struct glDisableClientStateiEXT_params
{
    TEB *teb;
    GLenum array;
    GLuint index;
};

struct glDisableIndexedEXT_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
};

struct glDisableVariantClientStateEXT_params
{
    TEB *teb;
    GLuint id;
};

struct glDisableVertexArrayAttrib_params
{
    TEB *teb;
    GLuint vaobj;
    GLuint index;
};

struct glDisableVertexArrayAttribEXT_params
{
    TEB *teb;
    GLuint vaobj;
    GLuint index;
};

struct glDisableVertexArrayEXT_params
{
    TEB *teb;
    GLuint vaobj;
    GLenum array;
};

struct glDisableVertexAttribAPPLE_params
{
    TEB *teb;
    GLuint index;
    GLenum pname;
};

struct glDisableVertexAttribArray_params
{
    TEB *teb;
    GLuint index;
};

struct glDisableVertexAttribArrayARB_params
{
    TEB *teb;
    GLuint index;
};

struct glDisablei_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
};

struct glDispatchCompute_params
{
    TEB *teb;
    GLuint num_groups_x;
    GLuint num_groups_y;
    GLuint num_groups_z;
};

struct glDispatchComputeGroupSizeARB_params
{
    TEB *teb;
    GLuint num_groups_x;
    GLuint num_groups_y;
    GLuint num_groups_z;
    GLuint group_size_x;
    GLuint group_size_y;
    GLuint group_size_z;
};

struct glDispatchComputeIndirect_params
{
    TEB *teb;
    GLintptr indirect;
};

struct glDrawArraysEXT_params
{
    TEB *teb;
    GLenum mode;
    GLint first;
    GLsizei count;
};

struct glDrawArraysIndirect_params
{
    TEB *teb;
    GLenum mode;
    const void *indirect;
};

struct glDrawArraysInstanced_params
{
    TEB *teb;
    GLenum mode;
    GLint first;
    GLsizei count;
    GLsizei instancecount;
};

struct glDrawArraysInstancedARB_params
{
    TEB *teb;
    GLenum mode;
    GLint first;
    GLsizei count;
    GLsizei primcount;
};

struct glDrawArraysInstancedBaseInstance_params
{
    TEB *teb;
    GLenum mode;
    GLint first;
    GLsizei count;
    GLsizei instancecount;
    GLuint baseinstance;
};

struct glDrawArraysInstancedEXT_params
{
    TEB *teb;
    GLenum mode;
    GLint start;
    GLsizei count;
    GLsizei primcount;
};

struct glDrawBufferRegion_params
{
    TEB *teb;
    GLenum region;
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
    GLint xDest;
    GLint yDest;
};

struct glDrawBuffers_params
{
    TEB *teb;
    GLsizei n;
    const GLenum *bufs;
};

struct glDrawBuffersARB_params
{
    TEB *teb;
    GLsizei n;
    const GLenum *bufs;
};

struct glDrawBuffersATI_params
{
    TEB *teb;
    GLsizei n;
    const GLenum *bufs;
};

struct glDrawCommandsAddressNV_params
{
    TEB *teb;
    GLenum primitiveMode;
    const GLuint64 *indirects;
    const GLsizei *sizes;
    GLuint count;
};

struct glDrawCommandsNV_params
{
    TEB *teb;
    GLenum primitiveMode;
    GLuint buffer;
    const GLintptr *indirects;
    const GLsizei *sizes;
    GLuint count;
};

struct glDrawCommandsStatesAddressNV_params
{
    TEB *teb;
    const GLuint64 *indirects;
    const GLsizei *sizes;
    const GLuint *states;
    const GLuint *fbos;
    GLuint count;
};

struct glDrawCommandsStatesNV_params
{
    TEB *teb;
    GLuint buffer;
    const GLintptr *indirects;
    const GLsizei *sizes;
    const GLuint *states;
    const GLuint *fbos;
    GLuint count;
};

struct glDrawElementArrayAPPLE_params
{
    TEB *teb;
    GLenum mode;
    GLint first;
    GLsizei count;
};

struct glDrawElementArrayATI_params
{
    TEB *teb;
    GLenum mode;
    GLsizei count;
};

struct glDrawElementsBaseVertex_params
{
    TEB *teb;
    GLenum mode;
    GLsizei count;
    GLenum type;
    const void *indices;
    GLint basevertex;
};

struct glDrawElementsIndirect_params
{
    TEB *teb;
    GLenum mode;
    GLenum type;
    const void *indirect;
};

struct glDrawElementsInstanced_params
{
    TEB *teb;
    GLenum mode;
    GLsizei count;
    GLenum type;
    const void *indices;
    GLsizei instancecount;
};

struct glDrawElementsInstancedARB_params
{
    TEB *teb;
    GLenum mode;
    GLsizei count;
    GLenum type;
    const void *indices;
    GLsizei primcount;
};

struct glDrawElementsInstancedBaseInstance_params
{
    TEB *teb;
    GLenum mode;
    GLsizei count;
    GLenum type;
    const void *indices;
    GLsizei instancecount;
    GLuint baseinstance;
};

struct glDrawElementsInstancedBaseVertex_params
{
    TEB *teb;
    GLenum mode;
    GLsizei count;
    GLenum type;
    const void *indices;
    GLsizei instancecount;
    GLint basevertex;
};

struct glDrawElementsInstancedBaseVertexBaseInstance_params
{
    TEB *teb;
    GLenum mode;
    GLsizei count;
    GLenum type;
    const void *indices;
    GLsizei instancecount;
    GLint basevertex;
    GLuint baseinstance;
};

struct glDrawElementsInstancedEXT_params
{
    TEB *teb;
    GLenum mode;
    GLsizei count;
    GLenum type;
    const void *indices;
    GLsizei primcount;
};

struct glDrawMeshArraysSUN_params
{
    TEB *teb;
    GLenum mode;
    GLint first;
    GLsizei count;
    GLsizei width;
};

struct glDrawMeshTasksIndirectNV_params
{
    TEB *teb;
    GLintptr indirect;
};

struct glDrawMeshTasksNV_params
{
    TEB *teb;
    GLuint first;
    GLuint count;
};

struct glDrawRangeElementArrayAPPLE_params
{
    TEB *teb;
    GLenum mode;
    GLuint start;
    GLuint end;
    GLint first;
    GLsizei count;
};

struct glDrawRangeElementArrayATI_params
{
    TEB *teb;
    GLenum mode;
    GLuint start;
    GLuint end;
    GLsizei count;
};

struct glDrawRangeElements_params
{
    TEB *teb;
    GLenum mode;
    GLuint start;
    GLuint end;
    GLsizei count;
    GLenum type;
    const void *indices;
};

struct glDrawRangeElementsBaseVertex_params
{
    TEB *teb;
    GLenum mode;
    GLuint start;
    GLuint end;
    GLsizei count;
    GLenum type;
    const void *indices;
    GLint basevertex;
};

struct glDrawRangeElementsEXT_params
{
    TEB *teb;
    GLenum mode;
    GLuint start;
    GLuint end;
    GLsizei count;
    GLenum type;
    const void *indices;
};

struct glDrawTextureNV_params
{
    TEB *teb;
    GLuint texture;
    GLuint sampler;
    GLfloat x0;
    GLfloat y0;
    GLfloat x1;
    GLfloat y1;
    GLfloat z;
    GLfloat s0;
    GLfloat t0;
    GLfloat s1;
    GLfloat t1;
};

struct glDrawTransformFeedback_params
{
    TEB *teb;
    GLenum mode;
    GLuint id;
};

struct glDrawTransformFeedbackInstanced_params
{
    TEB *teb;
    GLenum mode;
    GLuint id;
    GLsizei instancecount;
};

struct glDrawTransformFeedbackNV_params
{
    TEB *teb;
    GLenum mode;
    GLuint id;
};

struct glDrawTransformFeedbackStream_params
{
    TEB *teb;
    GLenum mode;
    GLuint id;
    GLuint stream;
};

struct glDrawTransformFeedbackStreamInstanced_params
{
    TEB *teb;
    GLenum mode;
    GLuint id;
    GLuint stream;
    GLsizei instancecount;
};

struct glDrawVkImageNV_params
{
    TEB *teb;
    GLuint64 vkImage;
    GLuint sampler;
    GLfloat x0;
    GLfloat y0;
    GLfloat x1;
    GLfloat y1;
    GLfloat z;
    GLfloat s0;
    GLfloat t0;
    GLfloat s1;
    GLfloat t1;
};

struct glEGLImageTargetTexStorageEXT_params
{
    TEB *teb;
    GLenum target;
    GLeglImageOES image;
    const GLint* attrib_list;
};

struct glEGLImageTargetTextureStorageEXT_params
{
    TEB *teb;
    GLuint texture;
    GLeglImageOES image;
    const GLint* attrib_list;
};

struct glEdgeFlagFormatNV_params
{
    TEB *teb;
    GLsizei stride;
};

struct glEdgeFlagPointerEXT_params
{
    TEB *teb;
    GLsizei stride;
    GLsizei count;
    const GLboolean *pointer;
};

struct glEdgeFlagPointerListIBM_params
{
    TEB *teb;
    GLint stride;
    const GLboolean **pointer;
    GLint ptrstride;
};

struct glElementPointerAPPLE_params
{
    TEB *teb;
    GLenum type;
    const void *pointer;
};

struct glElementPointerATI_params
{
    TEB *teb;
    GLenum type;
    const void *pointer;
};

struct glEnableClientStateIndexedEXT_params
{
    TEB *teb;
    GLenum array;
    GLuint index;
};

struct glEnableClientStateiEXT_params
{
    TEB *teb;
    GLenum array;
    GLuint index;
};

struct glEnableIndexedEXT_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
};

struct glEnableVariantClientStateEXT_params
{
    TEB *teb;
    GLuint id;
};

struct glEnableVertexArrayAttrib_params
{
    TEB *teb;
    GLuint vaobj;
    GLuint index;
};

struct glEnableVertexArrayAttribEXT_params
{
    TEB *teb;
    GLuint vaobj;
    GLuint index;
};

struct glEnableVertexArrayEXT_params
{
    TEB *teb;
    GLuint vaobj;
    GLenum array;
};

struct glEnableVertexAttribAPPLE_params
{
    TEB *teb;
    GLuint index;
    GLenum pname;
};

struct glEnableVertexAttribArray_params
{
    TEB *teb;
    GLuint index;
};

struct glEnableVertexAttribArrayARB_params
{
    TEB *teb;
    GLuint index;
};

struct glEnablei_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
};

struct glEndConditionalRender_params
{
    TEB *teb;
};

struct glEndConditionalRenderNV_params
{
    TEB *teb;
};

struct glEndConditionalRenderNVX_params
{
    TEB *teb;
};

struct glEndFragmentShaderATI_params
{
    TEB *teb;
};

struct glEndOcclusionQueryNV_params
{
    TEB *teb;
};

struct glEndPerfMonitorAMD_params
{
    TEB *teb;
    GLuint monitor;
};

struct glEndPerfQueryINTEL_params
{
    TEB *teb;
    GLuint queryHandle;
};

struct glEndQuery_params
{
    TEB *teb;
    GLenum target;
};

struct glEndQueryARB_params
{
    TEB *teb;
    GLenum target;
};

struct glEndQueryIndexed_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
};

struct glEndTransformFeedback_params
{
    TEB *teb;
};

struct glEndTransformFeedbackEXT_params
{
    TEB *teb;
};

struct glEndTransformFeedbackNV_params
{
    TEB *teb;
};

struct glEndVertexShaderEXT_params
{
    TEB *teb;
};

struct glEndVideoCaptureNV_params
{
    TEB *teb;
    GLuint video_capture_slot;
};

struct glEvalCoord1xOES_params
{
    TEB *teb;
    GLfixed u;
};

struct glEvalCoord1xvOES_params
{
    TEB *teb;
    const GLfixed *coords;
};

struct glEvalCoord2xOES_params
{
    TEB *teb;
    GLfixed u;
    GLfixed v;
};

struct glEvalCoord2xvOES_params
{
    TEB *teb;
    const GLfixed *coords;
};

struct glEvalMapsNV_params
{
    TEB *teb;
    GLenum target;
    GLenum mode;
};

struct glEvaluateDepthValuesARB_params
{
    TEB *teb;
};

struct glExecuteProgramNV_params
{
    TEB *teb;
    GLenum target;
    GLuint id;
    const GLfloat *params;
};

struct glExtractComponentEXT_params
{
    TEB *teb;
    GLuint res;
    GLuint src;
    GLuint num;
};

struct glFeedbackBufferxOES_params
{
    TEB *teb;
    GLsizei n;
    GLenum type;
    const GLfixed *buffer;
};

struct glFenceSync_params
{
    TEB *teb;
    GLenum condition;
    GLbitfield flags;
    GLsync ret;
};

struct glFinalCombinerInputNV_params
{
    TEB *teb;
    GLenum variable;
    GLenum input;
    GLenum mapping;
    GLenum componentUsage;
};

struct glFinishAsyncSGIX_params
{
    TEB *teb;
    GLuint *markerp;
    GLint ret;
};

struct glFinishFenceAPPLE_params
{
    TEB *teb;
    GLuint fence;
};

struct glFinishFenceNV_params
{
    TEB *teb;
    GLuint fence;
};

struct glFinishObjectAPPLE_params
{
    TEB *teb;
    GLenum object;
    GLint name;
};

struct glFinishTextureSUNX_params
{
    TEB *teb;
};

struct glFlushMappedBufferRange_params
{
    TEB *teb;
    GLenum target;
    GLintptr offset;
    GLsizeiptr length;
};

struct glFlushMappedBufferRangeAPPLE_params
{
    TEB *teb;
    GLenum target;
    GLintptr offset;
    GLsizeiptr size;
};

struct glFlushMappedNamedBufferRange_params
{
    TEB *teb;
    GLuint buffer;
    GLintptr offset;
    GLsizeiptr length;
};

struct glFlushMappedNamedBufferRangeEXT_params
{
    TEB *teb;
    GLuint buffer;
    GLintptr offset;
    GLsizeiptr length;
};

struct glFlushPixelDataRangeNV_params
{
    TEB *teb;
    GLenum target;
};

struct glFlushRasterSGIX_params
{
    TEB *teb;
};

struct glFlushStaticDataIBM_params
{
    TEB *teb;
    GLenum target;
};

struct glFlushVertexArrayRangeAPPLE_params
{
    TEB *teb;
    GLsizei length;
    void *pointer;
};

struct glFlushVertexArrayRangeNV_params
{
    TEB *teb;
};

struct glFogCoordFormatNV_params
{
    TEB *teb;
    GLenum type;
    GLsizei stride;
};

struct glFogCoordPointer_params
{
    TEB *teb;
    GLenum type;
    GLsizei stride;
    const void *pointer;
};

struct glFogCoordPointerEXT_params
{
    TEB *teb;
    GLenum type;
    GLsizei stride;
    const void *pointer;
};

struct glFogCoordPointerListIBM_params
{
    TEB *teb;
    GLenum type;
    GLint stride;
    const void **pointer;
    GLint ptrstride;
};

struct glFogCoordd_params
{
    TEB *teb;
    GLdouble coord;
};

struct glFogCoorddEXT_params
{
    TEB *teb;
    GLdouble coord;
};

struct glFogCoorddv_params
{
    TEB *teb;
    const GLdouble *coord;
};

struct glFogCoorddvEXT_params
{
    TEB *teb;
    const GLdouble *coord;
};

struct glFogCoordf_params
{
    TEB *teb;
    GLfloat coord;
};

struct glFogCoordfEXT_params
{
    TEB *teb;
    GLfloat coord;
};

struct glFogCoordfv_params
{
    TEB *teb;
    const GLfloat *coord;
};

struct glFogCoordfvEXT_params
{
    TEB *teb;
    const GLfloat *coord;
};

struct glFogCoordhNV_params
{
    TEB *teb;
    GLhalfNV fog;
};

struct glFogCoordhvNV_params
{
    TEB *teb;
    const GLhalfNV *fog;
};

struct glFogFuncSGIS_params
{
    TEB *teb;
    GLsizei n;
    const GLfloat *points;
};

struct glFogxOES_params
{
    TEB *teb;
    GLenum pname;
    GLfixed param;
};

struct glFogxvOES_params
{
    TEB *teb;
    GLenum pname;
    const GLfixed *param;
};

struct glFragmentColorMaterialSGIX_params
{
    TEB *teb;
    GLenum face;
    GLenum mode;
};

struct glFragmentCoverageColorNV_params
{
    TEB *teb;
    GLuint color;
};

struct glFragmentLightModelfSGIX_params
{
    TEB *teb;
    GLenum pname;
    GLfloat param;
};

struct glFragmentLightModelfvSGIX_params
{
    TEB *teb;
    GLenum pname;
    const GLfloat *params;
};

struct glFragmentLightModeliSGIX_params
{
    TEB *teb;
    GLenum pname;
    GLint param;
};

struct glFragmentLightModelivSGIX_params
{
    TEB *teb;
    GLenum pname;
    const GLint *params;
};

struct glFragmentLightfSGIX_params
{
    TEB *teb;
    GLenum light;
    GLenum pname;
    GLfloat param;
};

struct glFragmentLightfvSGIX_params
{
    TEB *teb;
    GLenum light;
    GLenum pname;
    const GLfloat *params;
};

struct glFragmentLightiSGIX_params
{
    TEB *teb;
    GLenum light;
    GLenum pname;
    GLint param;
};

struct glFragmentLightivSGIX_params
{
    TEB *teb;
    GLenum light;
    GLenum pname;
    const GLint *params;
};

struct glFragmentMaterialfSGIX_params
{
    TEB *teb;
    GLenum face;
    GLenum pname;
    GLfloat param;
};

struct glFragmentMaterialfvSGIX_params
{
    TEB *teb;
    GLenum face;
    GLenum pname;
    const GLfloat *params;
};

struct glFragmentMaterialiSGIX_params
{
    TEB *teb;
    GLenum face;
    GLenum pname;
    GLint param;
};

struct glFragmentMaterialivSGIX_params
{
    TEB *teb;
    GLenum face;
    GLenum pname;
    const GLint *params;
};

struct glFrameTerminatorGREMEDY_params
{
    TEB *teb;
};

struct glFrameZoomSGIX_params
{
    TEB *teb;
    GLint factor;
};

struct glFramebufferDrawBufferEXT_params
{
    TEB *teb;
    GLuint framebuffer;
    GLenum mode;
};

struct glFramebufferDrawBuffersEXT_params
{
    TEB *teb;
    GLuint framebuffer;
    GLsizei n;
    const GLenum *bufs;
};

struct glFramebufferFetchBarrierEXT_params
{
    TEB *teb;
};

struct glFramebufferParameteri_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLint param;
};

struct glFramebufferParameteriMESA_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLint param;
};

struct glFramebufferReadBufferEXT_params
{
    TEB *teb;
    GLuint framebuffer;
    GLenum mode;
};

struct glFramebufferRenderbuffer_params
{
    TEB *teb;
    GLenum target;
    GLenum attachment;
    GLenum renderbuffertarget;
    GLuint renderbuffer;
};

struct glFramebufferRenderbufferEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum attachment;
    GLenum renderbuffertarget;
    GLuint renderbuffer;
};

struct glFramebufferSampleLocationsfvARB_params
{
    TEB *teb;
    GLenum target;
    GLuint start;
    GLsizei count;
    const GLfloat *v;
};

struct glFramebufferSampleLocationsfvNV_params
{
    TEB *teb;
    GLenum target;
    GLuint start;
    GLsizei count;
    const GLfloat *v;
};

struct glFramebufferSamplePositionsfvAMD_params
{
    TEB *teb;
    GLenum target;
    GLuint numsamples;
    GLuint pixelindex;
    const GLfloat *values;
};

struct glFramebufferTexture_params
{
    TEB *teb;
    GLenum target;
    GLenum attachment;
    GLuint texture;
    GLint level;
};

struct glFramebufferTexture1D_params
{
    TEB *teb;
    GLenum target;
    GLenum attachment;
    GLenum textarget;
    GLuint texture;
    GLint level;
};

struct glFramebufferTexture1DEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum attachment;
    GLenum textarget;
    GLuint texture;
    GLint level;
};

struct glFramebufferTexture2D_params
{
    TEB *teb;
    GLenum target;
    GLenum attachment;
    GLenum textarget;
    GLuint texture;
    GLint level;
};

struct glFramebufferTexture2DEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum attachment;
    GLenum textarget;
    GLuint texture;
    GLint level;
};

struct glFramebufferTexture3D_params
{
    TEB *teb;
    GLenum target;
    GLenum attachment;
    GLenum textarget;
    GLuint texture;
    GLint level;
    GLint zoffset;
};

struct glFramebufferTexture3DEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum attachment;
    GLenum textarget;
    GLuint texture;
    GLint level;
    GLint zoffset;
};

struct glFramebufferTextureARB_params
{
    TEB *teb;
    GLenum target;
    GLenum attachment;
    GLuint texture;
    GLint level;
};

struct glFramebufferTextureEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum attachment;
    GLuint texture;
    GLint level;
};

struct glFramebufferTextureFaceARB_params
{
    TEB *teb;
    GLenum target;
    GLenum attachment;
    GLuint texture;
    GLint level;
    GLenum face;
};

struct glFramebufferTextureFaceEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum attachment;
    GLuint texture;
    GLint level;
    GLenum face;
};

struct glFramebufferTextureLayer_params
{
    TEB *teb;
    GLenum target;
    GLenum attachment;
    GLuint texture;
    GLint level;
    GLint layer;
};

struct glFramebufferTextureLayerARB_params
{
    TEB *teb;
    GLenum target;
    GLenum attachment;
    GLuint texture;
    GLint level;
    GLint layer;
};

struct glFramebufferTextureLayerEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum attachment;
    GLuint texture;
    GLint level;
    GLint layer;
};

struct glFramebufferTextureMultiviewOVR_params
{
    TEB *teb;
    GLenum target;
    GLenum attachment;
    GLuint texture;
    GLint level;
    GLint baseViewIndex;
    GLsizei numViews;
};

struct glFreeObjectBufferATI_params
{
    TEB *teb;
    GLuint buffer;
};

struct glFrustumfOES_params
{
    TEB *teb;
    GLfloat l;
    GLfloat r;
    GLfloat b;
    GLfloat t;
    GLfloat n;
    GLfloat f;
};

struct glFrustumxOES_params
{
    TEB *teb;
    GLfixed l;
    GLfixed r;
    GLfixed b;
    GLfixed t;
    GLfixed n;
    GLfixed f;
};

struct glGenAsyncMarkersSGIX_params
{
    TEB *teb;
    GLsizei range;
    GLuint ret;
};

struct glGenBuffers_params
{
    TEB *teb;
    GLsizei n;
    GLuint *buffers;
};

struct glGenBuffersARB_params
{
    TEB *teb;
    GLsizei n;
    GLuint *buffers;
};

struct glGenFencesAPPLE_params
{
    TEB *teb;
    GLsizei n;
    GLuint *fences;
};

struct glGenFencesNV_params
{
    TEB *teb;
    GLsizei n;
    GLuint *fences;
};

struct glGenFragmentShadersATI_params
{
    TEB *teb;
    GLuint range;
    GLuint ret;
};

struct glGenFramebuffers_params
{
    TEB *teb;
    GLsizei n;
    GLuint *framebuffers;
};

struct glGenFramebuffersEXT_params
{
    TEB *teb;
    GLsizei n;
    GLuint *framebuffers;
};

struct glGenNamesAMD_params
{
    TEB *teb;
    GLenum identifier;
    GLuint num;
    GLuint *names;
};

struct glGenOcclusionQueriesNV_params
{
    TEB *teb;
    GLsizei n;
    GLuint *ids;
};

struct glGenPathsNV_params
{
    TEB *teb;
    GLsizei range;
    GLuint ret;
};

struct glGenPerfMonitorsAMD_params
{
    TEB *teb;
    GLsizei n;
    GLuint *monitors;
};

struct glGenProgramPipelines_params
{
    TEB *teb;
    GLsizei n;
    GLuint *pipelines;
};

struct glGenProgramsARB_params
{
    TEB *teb;
    GLsizei n;
    GLuint *programs;
};

struct glGenProgramsNV_params
{
    TEB *teb;
    GLsizei n;
    GLuint *programs;
};

struct glGenQueries_params
{
    TEB *teb;
    GLsizei n;
    GLuint *ids;
};

struct glGenQueriesARB_params
{
    TEB *teb;
    GLsizei n;
    GLuint *ids;
};

struct glGenQueryResourceTagNV_params
{
    TEB *teb;
    GLsizei n;
    GLint *tagIds;
};

struct glGenRenderbuffers_params
{
    TEB *teb;
    GLsizei n;
    GLuint *renderbuffers;
};

struct glGenRenderbuffersEXT_params
{
    TEB *teb;
    GLsizei n;
    GLuint *renderbuffers;
};

struct glGenSamplers_params
{
    TEB *teb;
    GLsizei count;
    GLuint *samplers;
};

struct glGenSemaphoresEXT_params
{
    TEB *teb;
    GLsizei n;
    GLuint *semaphores;
};

struct glGenSymbolsEXT_params
{
    TEB *teb;
    GLenum datatype;
    GLenum storagetype;
    GLenum range;
    GLuint components;
    GLuint ret;
};

struct glGenTexturesEXT_params
{
    TEB *teb;
    GLsizei n;
    GLuint *textures;
};

struct glGenTransformFeedbacks_params
{
    TEB *teb;
    GLsizei n;
    GLuint *ids;
};

struct glGenTransformFeedbacksNV_params
{
    TEB *teb;
    GLsizei n;
    GLuint *ids;
};

struct glGenVertexArrays_params
{
    TEB *teb;
    GLsizei n;
    GLuint *arrays;
};

struct glGenVertexArraysAPPLE_params
{
    TEB *teb;
    GLsizei n;
    GLuint *arrays;
};

struct glGenVertexShadersEXT_params
{
    TEB *teb;
    GLuint range;
    GLuint ret;
};

struct glGenerateMipmap_params
{
    TEB *teb;
    GLenum target;
};

struct glGenerateMipmapEXT_params
{
    TEB *teb;
    GLenum target;
};

struct glGenerateMultiTexMipmapEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
};

struct glGenerateTextureMipmap_params
{
    TEB *teb;
    GLuint texture;
};

struct glGenerateTextureMipmapEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
};

struct glGetActiveAtomicCounterBufferiv_params
{
    TEB *teb;
    GLuint program;
    GLuint bufferIndex;
    GLenum pname;
    GLint *params;
};

struct glGetActiveAttrib_params
{
    TEB *teb;
    GLuint program;
    GLuint index;
    GLsizei bufSize;
    GLsizei *length;
    GLint *size;
    GLenum *type;
    GLchar *name;
};

struct glGetActiveAttribARB_params
{
    TEB *teb;
    GLhandleARB programObj;
    GLuint index;
    GLsizei maxLength;
    GLsizei *length;
    GLint *size;
    GLenum *type;
    GLcharARB *name;
};

struct glGetActiveSubroutineName_params
{
    TEB *teb;
    GLuint program;
    GLenum shadertype;
    GLuint index;
    GLsizei bufSize;
    GLsizei *length;
    GLchar *name;
};

struct glGetActiveSubroutineUniformName_params
{
    TEB *teb;
    GLuint program;
    GLenum shadertype;
    GLuint index;
    GLsizei bufSize;
    GLsizei *length;
    GLchar *name;
};

struct glGetActiveSubroutineUniformiv_params
{
    TEB *teb;
    GLuint program;
    GLenum shadertype;
    GLuint index;
    GLenum pname;
    GLint *values;
};

struct glGetActiveUniform_params
{
    TEB *teb;
    GLuint program;
    GLuint index;
    GLsizei bufSize;
    GLsizei *length;
    GLint *size;
    GLenum *type;
    GLchar *name;
};

struct glGetActiveUniformARB_params
{
    TEB *teb;
    GLhandleARB programObj;
    GLuint index;
    GLsizei maxLength;
    GLsizei *length;
    GLint *size;
    GLenum *type;
    GLcharARB *name;
};

struct glGetActiveUniformBlockName_params
{
    TEB *teb;
    GLuint program;
    GLuint uniformBlockIndex;
    GLsizei bufSize;
    GLsizei *length;
    GLchar *uniformBlockName;
};

struct glGetActiveUniformBlockiv_params
{
    TEB *teb;
    GLuint program;
    GLuint uniformBlockIndex;
    GLenum pname;
    GLint *params;
};

struct glGetActiveUniformName_params
{
    TEB *teb;
    GLuint program;
    GLuint uniformIndex;
    GLsizei bufSize;
    GLsizei *length;
    GLchar *uniformName;
};

struct glGetActiveUniformsiv_params
{
    TEB *teb;
    GLuint program;
    GLsizei uniformCount;
    const GLuint *uniformIndices;
    GLenum pname;
    GLint *params;
};

struct glGetActiveVaryingNV_params
{
    TEB *teb;
    GLuint program;
    GLuint index;
    GLsizei bufSize;
    GLsizei *length;
    GLsizei *size;
    GLenum *type;
    GLchar *name;
};

struct glGetArrayObjectfvATI_params
{
    TEB *teb;
    GLenum array;
    GLenum pname;
    GLfloat *params;
};

struct glGetArrayObjectivATI_params
{
    TEB *teb;
    GLenum array;
    GLenum pname;
    GLint *params;
};

struct glGetAttachedObjectsARB_params
{
    TEB *teb;
    GLhandleARB containerObj;
    GLsizei maxCount;
    GLsizei *count;
    GLhandleARB *obj;
};

struct glGetAttachedShaders_params
{
    TEB *teb;
    GLuint program;
    GLsizei maxCount;
    GLsizei *count;
    GLuint *shaders;
};

struct glGetAttribLocation_params
{
    TEB *teb;
    GLuint program;
    const GLchar *name;
    GLint ret;
};

struct glGetAttribLocationARB_params
{
    TEB *teb;
    GLhandleARB programObj;
    const GLcharARB *name;
    GLint ret;
};

struct glGetBooleanIndexedvEXT_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLboolean *data;
};

struct glGetBooleani_v_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLboolean *data;
};

struct glGetBufferParameteri64v_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLint64 *params;
};

struct glGetBufferParameteriv_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLint *params;
};

struct glGetBufferParameterivARB_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLint *params;
};

struct glGetBufferParameterui64vNV_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLuint64EXT *params;
};

struct glGetBufferPointerv_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    void **params;
};

struct glGetBufferPointervARB_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    void **params;
};

struct glGetBufferSubData_params
{
    TEB *teb;
    GLenum target;
    GLintptr offset;
    GLsizeiptr size;
    void *data;
};

struct glGetBufferSubDataARB_params
{
    TEB *teb;
    GLenum target;
    GLintptrARB offset;
    GLsizeiptrARB size;
    void *data;
};

struct glGetClipPlanefOES_params
{
    TEB *teb;
    GLenum plane;
    GLfloat *equation;
};

struct glGetClipPlanexOES_params
{
    TEB *teb;
    GLenum plane;
    GLfixed *equation;
};

struct glGetColorTable_params
{
    TEB *teb;
    GLenum target;
    GLenum format;
    GLenum type;
    void *table;
};

struct glGetColorTableEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum format;
    GLenum type;
    void *data;
};

struct glGetColorTableParameterfv_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLfloat *params;
};

struct glGetColorTableParameterfvEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLfloat *params;
};

struct glGetColorTableParameterfvSGI_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLfloat *params;
};

struct glGetColorTableParameteriv_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLint *params;
};

struct glGetColorTableParameterivEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLint *params;
};

struct glGetColorTableParameterivSGI_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLint *params;
};

struct glGetColorTableSGI_params
{
    TEB *teb;
    GLenum target;
    GLenum format;
    GLenum type;
    void *table;
};

struct glGetCombinerInputParameterfvNV_params
{
    TEB *teb;
    GLenum stage;
    GLenum portion;
    GLenum variable;
    GLenum pname;
    GLfloat *params;
};

struct glGetCombinerInputParameterivNV_params
{
    TEB *teb;
    GLenum stage;
    GLenum portion;
    GLenum variable;
    GLenum pname;
    GLint *params;
};

struct glGetCombinerOutputParameterfvNV_params
{
    TEB *teb;
    GLenum stage;
    GLenum portion;
    GLenum pname;
    GLfloat *params;
};

struct glGetCombinerOutputParameterivNV_params
{
    TEB *teb;
    GLenum stage;
    GLenum portion;
    GLenum pname;
    GLint *params;
};

struct glGetCombinerStageParameterfvNV_params
{
    TEB *teb;
    GLenum stage;
    GLenum pname;
    GLfloat *params;
};

struct glGetCommandHeaderNV_params
{
    TEB *teb;
    GLenum tokenID;
    GLuint size;
    GLuint ret;
};

struct glGetCompressedMultiTexImageEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLint lod;
    void *img;
};

struct glGetCompressedTexImage_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    void *img;
};

struct glGetCompressedTexImageARB_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    void *img;
};

struct glGetCompressedTextureImage_params
{
    TEB *teb;
    GLuint texture;
    GLint level;
    GLsizei bufSize;
    void *pixels;
};

struct glGetCompressedTextureImageEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLint lod;
    void *img;
};

struct glGetCompressedTextureSubImage_params
{
    TEB *teb;
    GLuint texture;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLint zoffset;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLsizei bufSize;
    void *pixels;
};

struct glGetConvolutionFilter_params
{
    TEB *teb;
    GLenum target;
    GLenum format;
    GLenum type;
    void *image;
};

struct glGetConvolutionFilterEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum format;
    GLenum type;
    void *image;
};

struct glGetConvolutionParameterfv_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLfloat *params;
};

struct glGetConvolutionParameterfvEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLfloat *params;
};

struct glGetConvolutionParameteriv_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLint *params;
};

struct glGetConvolutionParameterivEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLint *params;
};

struct glGetConvolutionParameterxvOES_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLfixed *params;
};

struct glGetCoverageModulationTableNV_params
{
    TEB *teb;
    GLsizei bufSize;
    GLfloat *v;
};

struct glGetDebugMessageLog_params
{
    TEB *teb;
    GLuint count;
    GLsizei bufSize;
    GLenum *sources;
    GLenum *types;
    GLuint *ids;
    GLenum *severities;
    GLsizei *lengths;
    GLchar *messageLog;
    GLuint ret;
};

struct glGetDebugMessageLogAMD_params
{
    TEB *teb;
    GLuint count;
    GLsizei bufSize;
    GLenum *categories;
    GLuint *severities;
    GLuint *ids;
    GLsizei *lengths;
    GLchar *message;
    GLuint ret;
};

struct glGetDebugMessageLogARB_params
{
    TEB *teb;
    GLuint count;
    GLsizei bufSize;
    GLenum *sources;
    GLenum *types;
    GLuint *ids;
    GLenum *severities;
    GLsizei *lengths;
    GLchar *messageLog;
    GLuint ret;
};

struct glGetDetailTexFuncSGIS_params
{
    TEB *teb;
    GLenum target;
    GLfloat *points;
};

struct glGetDoubleIndexedvEXT_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLdouble *data;
};

struct glGetDoublei_v_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLdouble *data;
};

struct glGetDoublei_vEXT_params
{
    TEB *teb;
    GLenum pname;
    GLuint index;
    GLdouble *params;
};

struct glGetFenceivNV_params
{
    TEB *teb;
    GLuint fence;
    GLenum pname;
    GLint *params;
};

struct glGetFinalCombinerInputParameterfvNV_params
{
    TEB *teb;
    GLenum variable;
    GLenum pname;
    GLfloat *params;
};

struct glGetFinalCombinerInputParameterivNV_params
{
    TEB *teb;
    GLenum variable;
    GLenum pname;
    GLint *params;
};

struct glGetFirstPerfQueryIdINTEL_params
{
    TEB *teb;
    GLuint *queryId;
};

struct glGetFixedvOES_params
{
    TEB *teb;
    GLenum pname;
    GLfixed *params;
};

struct glGetFloatIndexedvEXT_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLfloat *data;
};

struct glGetFloati_v_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLfloat *data;
};

struct glGetFloati_vEXT_params
{
    TEB *teb;
    GLenum pname;
    GLuint index;
    GLfloat *params;
};

struct glGetFogFuncSGIS_params
{
    TEB *teb;
    GLfloat *points;
};

struct glGetFragDataIndex_params
{
    TEB *teb;
    GLuint program;
    const GLchar *name;
    GLint ret;
};

struct glGetFragDataLocation_params
{
    TEB *teb;
    GLuint program;
    const GLchar *name;
    GLint ret;
};

struct glGetFragDataLocationEXT_params
{
    TEB *teb;
    GLuint program;
    const GLchar *name;
    GLint ret;
};

struct glGetFragmentLightfvSGIX_params
{
    TEB *teb;
    GLenum light;
    GLenum pname;
    GLfloat *params;
};

struct glGetFragmentLightivSGIX_params
{
    TEB *teb;
    GLenum light;
    GLenum pname;
    GLint *params;
};

struct glGetFragmentMaterialfvSGIX_params
{
    TEB *teb;
    GLenum face;
    GLenum pname;
    GLfloat *params;
};

struct glGetFragmentMaterialivSGIX_params
{
    TEB *teb;
    GLenum face;
    GLenum pname;
    GLint *params;
};

struct glGetFramebufferAttachmentParameteriv_params
{
    TEB *teb;
    GLenum target;
    GLenum attachment;
    GLenum pname;
    GLint *params;
};

struct glGetFramebufferAttachmentParameterivEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum attachment;
    GLenum pname;
    GLint *params;
};

struct glGetFramebufferParameterfvAMD_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLuint numsamples;
    GLuint pixelindex;
    GLsizei size;
    GLfloat *values;
};

struct glGetFramebufferParameteriv_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLint *params;
};

struct glGetFramebufferParameterivEXT_params
{
    TEB *teb;
    GLuint framebuffer;
    GLenum pname;
    GLint *params;
};

struct glGetFramebufferParameterivMESA_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLint *params;
};

struct glGetGraphicsResetStatus_params
{
    TEB *teb;
    GLenum ret;
};

struct glGetGraphicsResetStatusARB_params
{
    TEB *teb;
    GLenum ret;
};

struct glGetHandleARB_params
{
    TEB *teb;
    GLenum pname;
    GLhandleARB ret;
};

struct glGetHistogram_params
{
    TEB *teb;
    GLenum target;
    GLboolean reset;
    GLenum format;
    GLenum type;
    void *values;
};

struct glGetHistogramEXT_params
{
    TEB *teb;
    GLenum target;
    GLboolean reset;
    GLenum format;
    GLenum type;
    void *values;
};

struct glGetHistogramParameterfv_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLfloat *params;
};

struct glGetHistogramParameterfvEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLfloat *params;
};

struct glGetHistogramParameteriv_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLint *params;
};

struct glGetHistogramParameterivEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLint *params;
};

struct glGetHistogramParameterxvOES_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLfixed *params;
};

struct glGetImageHandleARB_params
{
    TEB *teb;
    GLuint texture;
    GLint level;
    GLboolean layered;
    GLint layer;
    GLenum format;
    GLuint64 ret;
};

struct glGetImageHandleNV_params
{
    TEB *teb;
    GLuint texture;
    GLint level;
    GLboolean layered;
    GLint layer;
    GLenum format;
    GLuint64 ret;
};

struct glGetImageTransformParameterfvHP_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLfloat *params;
};

struct glGetImageTransformParameterivHP_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLint *params;
};

struct glGetInfoLogARB_params
{
    TEB *teb;
    GLhandleARB obj;
    GLsizei maxLength;
    GLsizei *length;
    GLcharARB *infoLog;
};

struct glGetInstrumentsSGIX_params
{
    TEB *teb;
    GLint ret;
};

struct glGetInteger64i_v_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLint64 *data;
};

struct glGetInteger64v_params
{
    TEB *teb;
    GLenum pname;
    GLint64 *data;
};

struct glGetIntegerIndexedvEXT_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLint *data;
};

struct glGetIntegeri_v_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLint *data;
};

struct glGetIntegerui64i_vNV_params
{
    TEB *teb;
    GLenum value;
    GLuint index;
    GLuint64EXT *result;
};

struct glGetIntegerui64vNV_params
{
    TEB *teb;
    GLenum value;
    GLuint64EXT *result;
};

struct glGetInternalformatSampleivNV_params
{
    TEB *teb;
    GLenum target;
    GLenum internalformat;
    GLsizei samples;
    GLenum pname;
    GLsizei count;
    GLint *params;
};

struct glGetInternalformati64v_params
{
    TEB *teb;
    GLenum target;
    GLenum internalformat;
    GLenum pname;
    GLsizei count;
    GLint64 *params;
};

struct glGetInternalformativ_params
{
    TEB *teb;
    GLenum target;
    GLenum internalformat;
    GLenum pname;
    GLsizei count;
    GLint *params;
};

struct glGetInvariantBooleanvEXT_params
{
    TEB *teb;
    GLuint id;
    GLenum value;
    GLboolean *data;
};

struct glGetInvariantFloatvEXT_params
{
    TEB *teb;
    GLuint id;
    GLenum value;
    GLfloat *data;
};

struct glGetInvariantIntegervEXT_params
{
    TEB *teb;
    GLuint id;
    GLenum value;
    GLint *data;
};

struct glGetLightxOES_params
{
    TEB *teb;
    GLenum light;
    GLenum pname;
    GLfixed *params;
};

struct glGetListParameterfvSGIX_params
{
    TEB *teb;
    GLuint list;
    GLenum pname;
    GLfloat *params;
};

struct glGetListParameterivSGIX_params
{
    TEB *teb;
    GLuint list;
    GLenum pname;
    GLint *params;
};

struct glGetLocalConstantBooleanvEXT_params
{
    TEB *teb;
    GLuint id;
    GLenum value;
    GLboolean *data;
};

struct glGetLocalConstantFloatvEXT_params
{
    TEB *teb;
    GLuint id;
    GLenum value;
    GLfloat *data;
};

struct glGetLocalConstantIntegervEXT_params
{
    TEB *teb;
    GLuint id;
    GLenum value;
    GLint *data;
};

struct glGetMapAttribParameterfvNV_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLenum pname;
    GLfloat *params;
};

struct glGetMapAttribParameterivNV_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLenum pname;
    GLint *params;
};

struct glGetMapControlPointsNV_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLenum type;
    GLsizei ustride;
    GLsizei vstride;
    GLboolean packed;
    void *points;
};

struct glGetMapParameterfvNV_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLfloat *params;
};

struct glGetMapParameterivNV_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLint *params;
};

struct glGetMapxvOES_params
{
    TEB *teb;
    GLenum target;
    GLenum query;
    GLfixed *v;
};

struct glGetMaterialxOES_params
{
    TEB *teb;
    GLenum face;
    GLenum pname;
    GLfixed param;
};

struct glGetMemoryObjectDetachedResourcesuivNV_params
{
    TEB *teb;
    GLuint memory;
    GLenum pname;
    GLint first;
    GLsizei count;
    GLuint *params;
};

struct glGetMemoryObjectParameterivEXT_params
{
    TEB *teb;
    GLuint memoryObject;
    GLenum pname;
    GLint *params;
};

struct glGetMinmax_params
{
    TEB *teb;
    GLenum target;
    GLboolean reset;
    GLenum format;
    GLenum type;
    void *values;
};

struct glGetMinmaxEXT_params
{
    TEB *teb;
    GLenum target;
    GLboolean reset;
    GLenum format;
    GLenum type;
    void *values;
};

struct glGetMinmaxParameterfv_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLfloat *params;
};

struct glGetMinmaxParameterfvEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLfloat *params;
};

struct glGetMinmaxParameteriv_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLint *params;
};

struct glGetMinmaxParameterivEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLint *params;
};

struct glGetMultiTexEnvfvEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLenum pname;
    GLfloat *params;
};

struct glGetMultiTexEnvivEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLenum pname;
    GLint *params;
};

struct glGetMultiTexGendvEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum coord;
    GLenum pname;
    GLdouble *params;
};

struct glGetMultiTexGenfvEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum coord;
    GLenum pname;
    GLfloat *params;
};

struct glGetMultiTexGenivEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum coord;
    GLenum pname;
    GLint *params;
};

struct glGetMultiTexImageEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLint level;
    GLenum format;
    GLenum type;
    void *pixels;
};

struct glGetMultiTexLevelParameterfvEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLint level;
    GLenum pname;
    GLfloat *params;
};

struct glGetMultiTexLevelParameterivEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLint level;
    GLenum pname;
    GLint *params;
};

struct glGetMultiTexParameterIivEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLenum pname;
    GLint *params;
};

struct glGetMultiTexParameterIuivEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLenum pname;
    GLuint *params;
};

struct glGetMultiTexParameterfvEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLenum pname;
    GLfloat *params;
};

struct glGetMultiTexParameterivEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLenum pname;
    GLint *params;
};

struct glGetMultisamplefv_params
{
    TEB *teb;
    GLenum pname;
    GLuint index;
    GLfloat *val;
};

struct glGetMultisamplefvNV_params
{
    TEB *teb;
    GLenum pname;
    GLuint index;
    GLfloat *val;
};

struct glGetNamedBufferParameteri64v_params
{
    TEB *teb;
    GLuint buffer;
    GLenum pname;
    GLint64 *params;
};

struct glGetNamedBufferParameteriv_params
{
    TEB *teb;
    GLuint buffer;
    GLenum pname;
    GLint *params;
};

struct glGetNamedBufferParameterivEXT_params
{
    TEB *teb;
    GLuint buffer;
    GLenum pname;
    GLint *params;
};

struct glGetNamedBufferParameterui64vNV_params
{
    TEB *teb;
    GLuint buffer;
    GLenum pname;
    GLuint64EXT *params;
};

struct glGetNamedBufferPointerv_params
{
    TEB *teb;
    GLuint buffer;
    GLenum pname;
    void **params;
};

struct glGetNamedBufferPointervEXT_params
{
    TEB *teb;
    GLuint buffer;
    GLenum pname;
    void **params;
};

struct glGetNamedBufferSubData_params
{
    TEB *teb;
    GLuint buffer;
    GLintptr offset;
    GLsizeiptr size;
    void *data;
};

struct glGetNamedBufferSubDataEXT_params
{
    TEB *teb;
    GLuint buffer;
    GLintptr offset;
    GLsizeiptr size;
    void *data;
};

struct glGetNamedFramebufferAttachmentParameteriv_params
{
    TEB *teb;
    GLuint framebuffer;
    GLenum attachment;
    GLenum pname;
    GLint *params;
};

struct glGetNamedFramebufferAttachmentParameterivEXT_params
{
    TEB *teb;
    GLuint framebuffer;
    GLenum attachment;
    GLenum pname;
    GLint *params;
};

struct glGetNamedFramebufferParameterfvAMD_params
{
    TEB *teb;
    GLuint framebuffer;
    GLenum pname;
    GLuint numsamples;
    GLuint pixelindex;
    GLsizei size;
    GLfloat *values;
};

struct glGetNamedFramebufferParameteriv_params
{
    TEB *teb;
    GLuint framebuffer;
    GLenum pname;
    GLint *param;
};

struct glGetNamedFramebufferParameterivEXT_params
{
    TEB *teb;
    GLuint framebuffer;
    GLenum pname;
    GLint *params;
};

struct glGetNamedProgramLocalParameterIivEXT_params
{
    TEB *teb;
    GLuint program;
    GLenum target;
    GLuint index;
    GLint *params;
};

struct glGetNamedProgramLocalParameterIuivEXT_params
{
    TEB *teb;
    GLuint program;
    GLenum target;
    GLuint index;
    GLuint *params;
};

struct glGetNamedProgramLocalParameterdvEXT_params
{
    TEB *teb;
    GLuint program;
    GLenum target;
    GLuint index;
    GLdouble *params;
};

struct glGetNamedProgramLocalParameterfvEXT_params
{
    TEB *teb;
    GLuint program;
    GLenum target;
    GLuint index;
    GLfloat *params;
};

struct glGetNamedProgramStringEXT_params
{
    TEB *teb;
    GLuint program;
    GLenum target;
    GLenum pname;
    void *string;
};

struct glGetNamedProgramivEXT_params
{
    TEB *teb;
    GLuint program;
    GLenum target;
    GLenum pname;
    GLint *params;
};

struct glGetNamedRenderbufferParameteriv_params
{
    TEB *teb;
    GLuint renderbuffer;
    GLenum pname;
    GLint *params;
};

struct glGetNamedRenderbufferParameterivEXT_params
{
    TEB *teb;
    GLuint renderbuffer;
    GLenum pname;
    GLint *params;
};

struct glGetNamedStringARB_params
{
    TEB *teb;
    GLint namelen;
    const GLchar *name;
    GLsizei bufSize;
    GLint *stringlen;
    GLchar *string;
};

struct glGetNamedStringivARB_params
{
    TEB *teb;
    GLint namelen;
    const GLchar *name;
    GLenum pname;
    GLint *params;
};

struct glGetNextPerfQueryIdINTEL_params
{
    TEB *teb;
    GLuint queryId;
    GLuint *nextQueryId;
};

struct glGetObjectBufferfvATI_params
{
    TEB *teb;
    GLuint buffer;
    GLenum pname;
    GLfloat *params;
};

struct glGetObjectBufferivATI_params
{
    TEB *teb;
    GLuint buffer;
    GLenum pname;
    GLint *params;
};

struct glGetObjectLabel_params
{
    TEB *teb;
    GLenum identifier;
    GLuint name;
    GLsizei bufSize;
    GLsizei *length;
    GLchar *label;
};

struct glGetObjectLabelEXT_params
{
    TEB *teb;
    GLenum type;
    GLuint object;
    GLsizei bufSize;
    GLsizei *length;
    GLchar *label;
};

struct glGetObjectParameterfvARB_params
{
    TEB *teb;
    GLhandleARB obj;
    GLenum pname;
    GLfloat *params;
};

struct glGetObjectParameterivAPPLE_params
{
    TEB *teb;
    GLenum objectType;
    GLuint name;
    GLenum pname;
    GLint *params;
};

struct glGetObjectParameterivARB_params
{
    TEB *teb;
    GLhandleARB obj;
    GLenum pname;
    GLint *params;
};

struct glGetObjectPtrLabel_params
{
    TEB *teb;
    const void *ptr;
    GLsizei bufSize;
    GLsizei *length;
    GLchar *label;
};

struct glGetOcclusionQueryivNV_params
{
    TEB *teb;
    GLuint id;
    GLenum pname;
    GLint *params;
};

struct glGetOcclusionQueryuivNV_params
{
    TEB *teb;
    GLuint id;
    GLenum pname;
    GLuint *params;
};

struct glGetPathColorGenfvNV_params
{
    TEB *teb;
    GLenum color;
    GLenum pname;
    GLfloat *value;
};

struct glGetPathColorGenivNV_params
{
    TEB *teb;
    GLenum color;
    GLenum pname;
    GLint *value;
};

struct glGetPathCommandsNV_params
{
    TEB *teb;
    GLuint path;
    GLubyte *commands;
};

struct glGetPathCoordsNV_params
{
    TEB *teb;
    GLuint path;
    GLfloat *coords;
};

struct glGetPathDashArrayNV_params
{
    TEB *teb;
    GLuint path;
    GLfloat *dashArray;
};

struct glGetPathLengthNV_params
{
    TEB *teb;
    GLuint path;
    GLsizei startSegment;
    GLsizei numSegments;
    GLfloat ret;
};

struct glGetPathMetricRangeNV_params
{
    TEB *teb;
    GLbitfield metricQueryMask;
    GLuint firstPathName;
    GLsizei numPaths;
    GLsizei stride;
    GLfloat *metrics;
};

struct glGetPathMetricsNV_params
{
    TEB *teb;
    GLbitfield metricQueryMask;
    GLsizei numPaths;
    GLenum pathNameType;
    const void *paths;
    GLuint pathBase;
    GLsizei stride;
    GLfloat *metrics;
};

struct glGetPathParameterfvNV_params
{
    TEB *teb;
    GLuint path;
    GLenum pname;
    GLfloat *value;
};

struct glGetPathParameterivNV_params
{
    TEB *teb;
    GLuint path;
    GLenum pname;
    GLint *value;
};

struct glGetPathSpacingNV_params
{
    TEB *teb;
    GLenum pathListMode;
    GLsizei numPaths;
    GLenum pathNameType;
    const void *paths;
    GLuint pathBase;
    GLfloat advanceScale;
    GLfloat kerningScale;
    GLenum transformType;
    GLfloat *returnedSpacing;
};

struct glGetPathTexGenfvNV_params
{
    TEB *teb;
    GLenum texCoordSet;
    GLenum pname;
    GLfloat *value;
};

struct glGetPathTexGenivNV_params
{
    TEB *teb;
    GLenum texCoordSet;
    GLenum pname;
    GLint *value;
};

struct glGetPerfCounterInfoINTEL_params
{
    TEB *teb;
    GLuint queryId;
    GLuint counterId;
    GLuint counterNameLength;
    GLchar *counterName;
    GLuint counterDescLength;
    GLchar *counterDesc;
    GLuint *counterOffset;
    GLuint *counterDataSize;
    GLuint *counterTypeEnum;
    GLuint *counterDataTypeEnum;
    GLuint64 *rawCounterMaxValue;
};

struct glGetPerfMonitorCounterDataAMD_params
{
    TEB *teb;
    GLuint monitor;
    GLenum pname;
    GLsizei dataSize;
    GLuint *data;
    GLint *bytesWritten;
};

struct glGetPerfMonitorCounterInfoAMD_params
{
    TEB *teb;
    GLuint group;
    GLuint counter;
    GLenum pname;
    void *data;
};

struct glGetPerfMonitorCounterStringAMD_params
{
    TEB *teb;
    GLuint group;
    GLuint counter;
    GLsizei bufSize;
    GLsizei *length;
    GLchar *counterString;
};

struct glGetPerfMonitorCountersAMD_params
{
    TEB *teb;
    GLuint group;
    GLint *numCounters;
    GLint *maxActiveCounters;
    GLsizei counterSize;
    GLuint *counters;
};

struct glGetPerfMonitorGroupStringAMD_params
{
    TEB *teb;
    GLuint group;
    GLsizei bufSize;
    GLsizei *length;
    GLchar *groupString;
};

struct glGetPerfMonitorGroupsAMD_params
{
    TEB *teb;
    GLint *numGroups;
    GLsizei groupsSize;
    GLuint *groups;
};

struct glGetPerfQueryDataINTEL_params
{
    TEB *teb;
    GLuint queryHandle;
    GLuint flags;
    GLsizei dataSize;
    void *data;
    GLuint *bytesWritten;
};

struct glGetPerfQueryIdByNameINTEL_params
{
    TEB *teb;
    GLchar *queryName;
    GLuint *queryId;
};

struct glGetPerfQueryInfoINTEL_params
{
    TEB *teb;
    GLuint queryId;
    GLuint queryNameLength;
    GLchar *queryName;
    GLuint *dataSize;
    GLuint *noCounters;
    GLuint *noInstances;
    GLuint *capsMask;
};

struct glGetPixelMapxv_params
{
    TEB *teb;
    GLenum map;
    GLint size;
    GLfixed *values;
};

struct glGetPixelTexGenParameterfvSGIS_params
{
    TEB *teb;
    GLenum pname;
    GLfloat *params;
};

struct glGetPixelTexGenParameterivSGIS_params
{
    TEB *teb;
    GLenum pname;
    GLint *params;
};

struct glGetPixelTransformParameterfvEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLfloat *params;
};

struct glGetPixelTransformParameterivEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLint *params;
};

struct glGetPointerIndexedvEXT_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    void **data;
};

struct glGetPointeri_vEXT_params
{
    TEB *teb;
    GLenum pname;
    GLuint index;
    void **params;
};

struct glGetPointervEXT_params
{
    TEB *teb;
    GLenum pname;
    void **params;
};

struct glGetProgramBinary_params
{
    TEB *teb;
    GLuint program;
    GLsizei bufSize;
    GLsizei *length;
    GLenum *binaryFormat;
    void *binary;
};

struct glGetProgramEnvParameterIivNV_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLint *params;
};

struct glGetProgramEnvParameterIuivNV_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLuint *params;
};

struct glGetProgramEnvParameterdvARB_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLdouble *params;
};

struct glGetProgramEnvParameterfvARB_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLfloat *params;
};

struct glGetProgramInfoLog_params
{
    TEB *teb;
    GLuint program;
    GLsizei bufSize;
    GLsizei *length;
    GLchar *infoLog;
};

struct glGetProgramInterfaceiv_params
{
    TEB *teb;
    GLuint program;
    GLenum programInterface;
    GLenum pname;
    GLint *params;
};

struct glGetProgramLocalParameterIivNV_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLint *params;
};

struct glGetProgramLocalParameterIuivNV_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLuint *params;
};

struct glGetProgramLocalParameterdvARB_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLdouble *params;
};

struct glGetProgramLocalParameterfvARB_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLfloat *params;
};

struct glGetProgramNamedParameterdvNV_params
{
    TEB *teb;
    GLuint id;
    GLsizei len;
    const GLubyte *name;
    GLdouble *params;
};

struct glGetProgramNamedParameterfvNV_params
{
    TEB *teb;
    GLuint id;
    GLsizei len;
    const GLubyte *name;
    GLfloat *params;
};

struct glGetProgramParameterdvNV_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLenum pname;
    GLdouble *params;
};

struct glGetProgramParameterfvNV_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLenum pname;
    GLfloat *params;
};

struct glGetProgramPipelineInfoLog_params
{
    TEB *teb;
    GLuint pipeline;
    GLsizei bufSize;
    GLsizei *length;
    GLchar *infoLog;
};

struct glGetProgramPipelineiv_params
{
    TEB *teb;
    GLuint pipeline;
    GLenum pname;
    GLint *params;
};

struct glGetProgramResourceIndex_params
{
    TEB *teb;
    GLuint program;
    GLenum programInterface;
    const GLchar *name;
    GLuint ret;
};

struct glGetProgramResourceLocation_params
{
    TEB *teb;
    GLuint program;
    GLenum programInterface;
    const GLchar *name;
    GLint ret;
};

struct glGetProgramResourceLocationIndex_params
{
    TEB *teb;
    GLuint program;
    GLenum programInterface;
    const GLchar *name;
    GLint ret;
};

struct glGetProgramResourceName_params
{
    TEB *teb;
    GLuint program;
    GLenum programInterface;
    GLuint index;
    GLsizei bufSize;
    GLsizei *length;
    GLchar *name;
};

struct glGetProgramResourcefvNV_params
{
    TEB *teb;
    GLuint program;
    GLenum programInterface;
    GLuint index;
    GLsizei propCount;
    const GLenum *props;
    GLsizei count;
    GLsizei *length;
    GLfloat *params;
};

struct glGetProgramResourceiv_params
{
    TEB *teb;
    GLuint program;
    GLenum programInterface;
    GLuint index;
    GLsizei propCount;
    const GLenum *props;
    GLsizei count;
    GLsizei *length;
    GLint *params;
};

struct glGetProgramStageiv_params
{
    TEB *teb;
    GLuint program;
    GLenum shadertype;
    GLenum pname;
    GLint *values;
};

struct glGetProgramStringARB_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    void *string;
};

struct glGetProgramStringNV_params
{
    TEB *teb;
    GLuint id;
    GLenum pname;
    GLubyte *program;
};

struct glGetProgramSubroutineParameteruivNV_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLuint *param;
};

struct glGetProgramiv_params
{
    TEB *teb;
    GLuint program;
    GLenum pname;
    GLint *params;
};

struct glGetProgramivARB_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLint *params;
};

struct glGetProgramivNV_params
{
    TEB *teb;
    GLuint id;
    GLenum pname;
    GLint *params;
};

struct glGetQueryBufferObjecti64v_params
{
    TEB *teb;
    GLuint id;
    GLuint buffer;
    GLenum pname;
    GLintptr offset;
};

struct glGetQueryBufferObjectiv_params
{
    TEB *teb;
    GLuint id;
    GLuint buffer;
    GLenum pname;
    GLintptr offset;
};

struct glGetQueryBufferObjectui64v_params
{
    TEB *teb;
    GLuint id;
    GLuint buffer;
    GLenum pname;
    GLintptr offset;
};

struct glGetQueryBufferObjectuiv_params
{
    TEB *teb;
    GLuint id;
    GLuint buffer;
    GLenum pname;
    GLintptr offset;
};

struct glGetQueryIndexediv_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLenum pname;
    GLint *params;
};

struct glGetQueryObjecti64v_params
{
    TEB *teb;
    GLuint id;
    GLenum pname;
    GLint64 *params;
};

struct glGetQueryObjecti64vEXT_params
{
    TEB *teb;
    GLuint id;
    GLenum pname;
    GLint64 *params;
};

struct glGetQueryObjectiv_params
{
    TEB *teb;
    GLuint id;
    GLenum pname;
    GLint *params;
};

struct glGetQueryObjectivARB_params
{
    TEB *teb;
    GLuint id;
    GLenum pname;
    GLint *params;
};

struct glGetQueryObjectui64v_params
{
    TEB *teb;
    GLuint id;
    GLenum pname;
    GLuint64 *params;
};

struct glGetQueryObjectui64vEXT_params
{
    TEB *teb;
    GLuint id;
    GLenum pname;
    GLuint64 *params;
};

struct glGetQueryObjectuiv_params
{
    TEB *teb;
    GLuint id;
    GLenum pname;
    GLuint *params;
};

struct glGetQueryObjectuivARB_params
{
    TEB *teb;
    GLuint id;
    GLenum pname;
    GLuint *params;
};

struct glGetQueryiv_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLint *params;
};

struct glGetQueryivARB_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLint *params;
};

struct glGetRenderbufferParameteriv_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLint *params;
};

struct glGetRenderbufferParameterivEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLint *params;
};

struct glGetSamplerParameterIiv_params
{
    TEB *teb;
    GLuint sampler;
    GLenum pname;
    GLint *params;
};

struct glGetSamplerParameterIuiv_params
{
    TEB *teb;
    GLuint sampler;
    GLenum pname;
    GLuint *params;
};

struct glGetSamplerParameterfv_params
{
    TEB *teb;
    GLuint sampler;
    GLenum pname;
    GLfloat *params;
};

struct glGetSamplerParameteriv_params
{
    TEB *teb;
    GLuint sampler;
    GLenum pname;
    GLint *params;
};

struct glGetSemaphoreParameterui64vEXT_params
{
    TEB *teb;
    GLuint semaphore;
    GLenum pname;
    GLuint64 *params;
};

struct glGetSeparableFilter_params
{
    TEB *teb;
    GLenum target;
    GLenum format;
    GLenum type;
    void *row;
    void *column;
    void *span;
};

struct glGetSeparableFilterEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum format;
    GLenum type;
    void *row;
    void *column;
    void *span;
};

struct glGetShaderInfoLog_params
{
    TEB *teb;
    GLuint shader;
    GLsizei bufSize;
    GLsizei *length;
    GLchar *infoLog;
};

struct glGetShaderPrecisionFormat_params
{
    TEB *teb;
    GLenum shadertype;
    GLenum precisiontype;
    GLint *range;
    GLint *precision;
};

struct glGetShaderSource_params
{
    TEB *teb;
    GLuint shader;
    GLsizei bufSize;
    GLsizei *length;
    GLchar *source;
};

struct glGetShaderSourceARB_params
{
    TEB *teb;
    GLhandleARB obj;
    GLsizei maxLength;
    GLsizei *length;
    GLcharARB *source;
};

struct glGetShaderiv_params
{
    TEB *teb;
    GLuint shader;
    GLenum pname;
    GLint *params;
};

struct glGetShadingRateImagePaletteNV_params
{
    TEB *teb;
    GLuint viewport;
    GLuint entry;
    GLenum *rate;
};

struct glGetShadingRateSampleLocationivNV_params
{
    TEB *teb;
    GLenum rate;
    GLuint samples;
    GLuint index;
    GLint *location;
};

struct glGetSharpenTexFuncSGIS_params
{
    TEB *teb;
    GLenum target;
    GLfloat *points;
};

struct glGetStageIndexNV_params
{
    TEB *teb;
    GLenum shadertype;
    GLushort ret;
};

struct glGetStringi_params
{
    TEB *teb;
    GLenum name;
    GLuint index;
    const GLubyte *ret;
};

struct glGetSubroutineIndex_params
{
    TEB *teb;
    GLuint program;
    GLenum shadertype;
    const GLchar *name;
    GLuint ret;
};

struct glGetSubroutineUniformLocation_params
{
    TEB *teb;
    GLuint program;
    GLenum shadertype;
    const GLchar *name;
    GLint ret;
};

struct glGetSynciv_params
{
    TEB *teb;
    GLsync sync;
    GLenum pname;
    GLsizei count;
    GLsizei *length;
    GLint *values;
};

struct glGetTexBumpParameterfvATI_params
{
    TEB *teb;
    GLenum pname;
    GLfloat *param;
};

struct glGetTexBumpParameterivATI_params
{
    TEB *teb;
    GLenum pname;
    GLint *param;
};

struct glGetTexEnvxvOES_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLfixed *params;
};

struct glGetTexFilterFuncSGIS_params
{
    TEB *teb;
    GLenum target;
    GLenum filter;
    GLfloat *weights;
};

struct glGetTexGenxvOES_params
{
    TEB *teb;
    GLenum coord;
    GLenum pname;
    GLfixed *params;
};

struct glGetTexLevelParameterxvOES_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLenum pname;
    GLfixed *params;
};

struct glGetTexParameterIiv_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLint *params;
};

struct glGetTexParameterIivEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLint *params;
};

struct glGetTexParameterIuiv_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLuint *params;
};

struct glGetTexParameterIuivEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLuint *params;
};

struct glGetTexParameterPointervAPPLE_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    void **params;
};

struct glGetTexParameterxvOES_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLfixed *params;
};

struct glGetTextureHandleARB_params
{
    TEB *teb;
    GLuint texture;
    GLuint64 ret;
};

struct glGetTextureHandleNV_params
{
    TEB *teb;
    GLuint texture;
    GLuint64 ret;
};

struct glGetTextureImage_params
{
    TEB *teb;
    GLuint texture;
    GLint level;
    GLenum format;
    GLenum type;
    GLsizei bufSize;
    void *pixels;
};

struct glGetTextureImageEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLint level;
    GLenum format;
    GLenum type;
    void *pixels;
};

struct glGetTextureLevelParameterfv_params
{
    TEB *teb;
    GLuint texture;
    GLint level;
    GLenum pname;
    GLfloat *params;
};

struct glGetTextureLevelParameterfvEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLint level;
    GLenum pname;
    GLfloat *params;
};

struct glGetTextureLevelParameteriv_params
{
    TEB *teb;
    GLuint texture;
    GLint level;
    GLenum pname;
    GLint *params;
};

struct glGetTextureLevelParameterivEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLint level;
    GLenum pname;
    GLint *params;
};

struct glGetTextureParameterIiv_params
{
    TEB *teb;
    GLuint texture;
    GLenum pname;
    GLint *params;
};

struct glGetTextureParameterIivEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLenum pname;
    GLint *params;
};

struct glGetTextureParameterIuiv_params
{
    TEB *teb;
    GLuint texture;
    GLenum pname;
    GLuint *params;
};

struct glGetTextureParameterIuivEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLenum pname;
    GLuint *params;
};

struct glGetTextureParameterfv_params
{
    TEB *teb;
    GLuint texture;
    GLenum pname;
    GLfloat *params;
};

struct glGetTextureParameterfvEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLenum pname;
    GLfloat *params;
};

struct glGetTextureParameteriv_params
{
    TEB *teb;
    GLuint texture;
    GLenum pname;
    GLint *params;
};

struct glGetTextureParameterivEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLenum pname;
    GLint *params;
};

struct glGetTextureSamplerHandleARB_params
{
    TEB *teb;
    GLuint texture;
    GLuint sampler;
    GLuint64 ret;
};

struct glGetTextureSamplerHandleNV_params
{
    TEB *teb;
    GLuint texture;
    GLuint sampler;
    GLuint64 ret;
};

struct glGetTextureSubImage_params
{
    TEB *teb;
    GLuint texture;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLint zoffset;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLenum format;
    GLenum type;
    GLsizei bufSize;
    void *pixels;
};

struct glGetTrackMatrixivNV_params
{
    TEB *teb;
    GLenum target;
    GLuint address;
    GLenum pname;
    GLint *params;
};

struct glGetTransformFeedbackVarying_params
{
    TEB *teb;
    GLuint program;
    GLuint index;
    GLsizei bufSize;
    GLsizei *length;
    GLsizei *size;
    GLenum *type;
    GLchar *name;
};

struct glGetTransformFeedbackVaryingEXT_params
{
    TEB *teb;
    GLuint program;
    GLuint index;
    GLsizei bufSize;
    GLsizei *length;
    GLsizei *size;
    GLenum *type;
    GLchar *name;
};

struct glGetTransformFeedbackVaryingNV_params
{
    TEB *teb;
    GLuint program;
    GLuint index;
    GLint *location;
};

struct glGetTransformFeedbacki64_v_params
{
    TEB *teb;
    GLuint xfb;
    GLenum pname;
    GLuint index;
    GLint64 *param;
};

struct glGetTransformFeedbacki_v_params
{
    TEB *teb;
    GLuint xfb;
    GLenum pname;
    GLuint index;
    GLint *param;
};

struct glGetTransformFeedbackiv_params
{
    TEB *teb;
    GLuint xfb;
    GLenum pname;
    GLint *param;
};

struct glGetUniformBlockIndex_params
{
    TEB *teb;
    GLuint program;
    const GLchar *uniformBlockName;
    GLuint ret;
};

struct glGetUniformBufferSizeEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLint ret;
};

struct glGetUniformIndices_params
{
    TEB *teb;
    GLuint program;
    GLsizei uniformCount;
    const GLchar *const*uniformNames;
    GLuint *uniformIndices;
};

struct glGetUniformLocation_params
{
    TEB *teb;
    GLuint program;
    const GLchar *name;
    GLint ret;
};

struct glGetUniformLocationARB_params
{
    TEB *teb;
    GLhandleARB programObj;
    const GLcharARB *name;
    GLint ret;
};

struct glGetUniformOffsetEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLintptr ret;
};

struct glGetUniformSubroutineuiv_params
{
    TEB *teb;
    GLenum shadertype;
    GLint location;
    GLuint *params;
};

struct glGetUniformdv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLdouble *params;
};

struct glGetUniformfv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLfloat *params;
};

struct glGetUniformfvARB_params
{
    TEB *teb;
    GLhandleARB programObj;
    GLint location;
    GLfloat *params;
};

struct glGetUniformi64vARB_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLint64 *params;
};

struct glGetUniformi64vNV_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLint64EXT *params;
};

struct glGetUniformiv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLint *params;
};

struct glGetUniformivARB_params
{
    TEB *teb;
    GLhandleARB programObj;
    GLint location;
    GLint *params;
};

struct glGetUniformui64vARB_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLuint64 *params;
};

struct glGetUniformui64vNV_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLuint64EXT *params;
};

struct glGetUniformuiv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLuint *params;
};

struct glGetUniformuivEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLuint *params;
};

struct glGetUnsignedBytei_vEXT_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLubyte *data;
};

struct glGetUnsignedBytevEXT_params
{
    TEB *teb;
    GLenum pname;
    GLubyte *data;
};

struct glGetVariantArrayObjectfvATI_params
{
    TEB *teb;
    GLuint id;
    GLenum pname;
    GLfloat *params;
};

struct glGetVariantArrayObjectivATI_params
{
    TEB *teb;
    GLuint id;
    GLenum pname;
    GLint *params;
};

struct glGetVariantBooleanvEXT_params
{
    TEB *teb;
    GLuint id;
    GLenum value;
    GLboolean *data;
};

struct glGetVariantFloatvEXT_params
{
    TEB *teb;
    GLuint id;
    GLenum value;
    GLfloat *data;
};

struct glGetVariantIntegervEXT_params
{
    TEB *teb;
    GLuint id;
    GLenum value;
    GLint *data;
};

struct glGetVariantPointervEXT_params
{
    TEB *teb;
    GLuint id;
    GLenum value;
    void **data;
};

struct glGetVaryingLocationNV_params
{
    TEB *teb;
    GLuint program;
    const GLchar *name;
    GLint ret;
};

struct glGetVertexArrayIndexed64iv_params
{
    TEB *teb;
    GLuint vaobj;
    GLuint index;
    GLenum pname;
    GLint64 *param;
};

struct glGetVertexArrayIndexediv_params
{
    TEB *teb;
    GLuint vaobj;
    GLuint index;
    GLenum pname;
    GLint *param;
};

struct glGetVertexArrayIntegeri_vEXT_params
{
    TEB *teb;
    GLuint vaobj;
    GLuint index;
    GLenum pname;
    GLint *param;
};

struct glGetVertexArrayIntegervEXT_params
{
    TEB *teb;
    GLuint vaobj;
    GLenum pname;
    GLint *param;
};

struct glGetVertexArrayPointeri_vEXT_params
{
    TEB *teb;
    GLuint vaobj;
    GLuint index;
    GLenum pname;
    void **param;
};

struct glGetVertexArrayPointervEXT_params
{
    TEB *teb;
    GLuint vaobj;
    GLenum pname;
    void **param;
};

struct glGetVertexArrayiv_params
{
    TEB *teb;
    GLuint vaobj;
    GLenum pname;
    GLint *param;
};

struct glGetVertexAttribArrayObjectfvATI_params
{
    TEB *teb;
    GLuint index;
    GLenum pname;
    GLfloat *params;
};

struct glGetVertexAttribArrayObjectivATI_params
{
    TEB *teb;
    GLuint index;
    GLenum pname;
    GLint *params;
};

struct glGetVertexAttribIiv_params
{
    TEB *teb;
    GLuint index;
    GLenum pname;
    GLint *params;
};

struct glGetVertexAttribIivEXT_params
{
    TEB *teb;
    GLuint index;
    GLenum pname;
    GLint *params;
};

struct glGetVertexAttribIuiv_params
{
    TEB *teb;
    GLuint index;
    GLenum pname;
    GLuint *params;
};

struct glGetVertexAttribIuivEXT_params
{
    TEB *teb;
    GLuint index;
    GLenum pname;
    GLuint *params;
};

struct glGetVertexAttribLdv_params
{
    TEB *teb;
    GLuint index;
    GLenum pname;
    GLdouble *params;
};

struct glGetVertexAttribLdvEXT_params
{
    TEB *teb;
    GLuint index;
    GLenum pname;
    GLdouble *params;
};

struct glGetVertexAttribLi64vNV_params
{
    TEB *teb;
    GLuint index;
    GLenum pname;
    GLint64EXT *params;
};

struct glGetVertexAttribLui64vARB_params
{
    TEB *teb;
    GLuint index;
    GLenum pname;
    GLuint64EXT *params;
};

struct glGetVertexAttribLui64vNV_params
{
    TEB *teb;
    GLuint index;
    GLenum pname;
    GLuint64EXT *params;
};

struct glGetVertexAttribPointerv_params
{
    TEB *teb;
    GLuint index;
    GLenum pname;
    void **pointer;
};

struct glGetVertexAttribPointervARB_params
{
    TEB *teb;
    GLuint index;
    GLenum pname;
    void **pointer;
};

struct glGetVertexAttribPointervNV_params
{
    TEB *teb;
    GLuint index;
    GLenum pname;
    void **pointer;
};

struct glGetVertexAttribdv_params
{
    TEB *teb;
    GLuint index;
    GLenum pname;
    GLdouble *params;
};

struct glGetVertexAttribdvARB_params
{
    TEB *teb;
    GLuint index;
    GLenum pname;
    GLdouble *params;
};

struct glGetVertexAttribdvNV_params
{
    TEB *teb;
    GLuint index;
    GLenum pname;
    GLdouble *params;
};

struct glGetVertexAttribfv_params
{
    TEB *teb;
    GLuint index;
    GLenum pname;
    GLfloat *params;
};

struct glGetVertexAttribfvARB_params
{
    TEB *teb;
    GLuint index;
    GLenum pname;
    GLfloat *params;
};

struct glGetVertexAttribfvNV_params
{
    TEB *teb;
    GLuint index;
    GLenum pname;
    GLfloat *params;
};

struct glGetVertexAttribiv_params
{
    TEB *teb;
    GLuint index;
    GLenum pname;
    GLint *params;
};

struct glGetVertexAttribivARB_params
{
    TEB *teb;
    GLuint index;
    GLenum pname;
    GLint *params;
};

struct glGetVertexAttribivNV_params
{
    TEB *teb;
    GLuint index;
    GLenum pname;
    GLint *params;
};

struct glGetVideoCaptureStreamdvNV_params
{
    TEB *teb;
    GLuint video_capture_slot;
    GLuint stream;
    GLenum pname;
    GLdouble *params;
};

struct glGetVideoCaptureStreamfvNV_params
{
    TEB *teb;
    GLuint video_capture_slot;
    GLuint stream;
    GLenum pname;
    GLfloat *params;
};

struct glGetVideoCaptureStreamivNV_params
{
    TEB *teb;
    GLuint video_capture_slot;
    GLuint stream;
    GLenum pname;
    GLint *params;
};

struct glGetVideoCaptureivNV_params
{
    TEB *teb;
    GLuint video_capture_slot;
    GLenum pname;
    GLint *params;
};

struct glGetVideoi64vNV_params
{
    TEB *teb;
    GLuint video_slot;
    GLenum pname;
    GLint64EXT *params;
};

struct glGetVideoivNV_params
{
    TEB *teb;
    GLuint video_slot;
    GLenum pname;
    GLint *params;
};

struct glGetVideoui64vNV_params
{
    TEB *teb;
    GLuint video_slot;
    GLenum pname;
    GLuint64EXT *params;
};

struct glGetVideouivNV_params
{
    TEB *teb;
    GLuint video_slot;
    GLenum pname;
    GLuint *params;
};

struct glGetVkProcAddrNV_params
{
    TEB *teb;
    const GLchar *name;
    GLVULKANPROCNV ret;
};

struct glGetnColorTable_params
{
    TEB *teb;
    GLenum target;
    GLenum format;
    GLenum type;
    GLsizei bufSize;
    void *table;
};

struct glGetnColorTableARB_params
{
    TEB *teb;
    GLenum target;
    GLenum format;
    GLenum type;
    GLsizei bufSize;
    void *table;
};

struct glGetnCompressedTexImage_params
{
    TEB *teb;
    GLenum target;
    GLint lod;
    GLsizei bufSize;
    void *pixels;
};

struct glGetnCompressedTexImageARB_params
{
    TEB *teb;
    GLenum target;
    GLint lod;
    GLsizei bufSize;
    void *img;
};

struct glGetnConvolutionFilter_params
{
    TEB *teb;
    GLenum target;
    GLenum format;
    GLenum type;
    GLsizei bufSize;
    void *image;
};

struct glGetnConvolutionFilterARB_params
{
    TEB *teb;
    GLenum target;
    GLenum format;
    GLenum type;
    GLsizei bufSize;
    void *image;
};

struct glGetnHistogram_params
{
    TEB *teb;
    GLenum target;
    GLboolean reset;
    GLenum format;
    GLenum type;
    GLsizei bufSize;
    void *values;
};

struct glGetnHistogramARB_params
{
    TEB *teb;
    GLenum target;
    GLboolean reset;
    GLenum format;
    GLenum type;
    GLsizei bufSize;
    void *values;
};

struct glGetnMapdv_params
{
    TEB *teb;
    GLenum target;
    GLenum query;
    GLsizei bufSize;
    GLdouble *v;
};

struct glGetnMapdvARB_params
{
    TEB *teb;
    GLenum target;
    GLenum query;
    GLsizei bufSize;
    GLdouble *v;
};

struct glGetnMapfv_params
{
    TEB *teb;
    GLenum target;
    GLenum query;
    GLsizei bufSize;
    GLfloat *v;
};

struct glGetnMapfvARB_params
{
    TEB *teb;
    GLenum target;
    GLenum query;
    GLsizei bufSize;
    GLfloat *v;
};

struct glGetnMapiv_params
{
    TEB *teb;
    GLenum target;
    GLenum query;
    GLsizei bufSize;
    GLint *v;
};

struct glGetnMapivARB_params
{
    TEB *teb;
    GLenum target;
    GLenum query;
    GLsizei bufSize;
    GLint *v;
};

struct glGetnMinmax_params
{
    TEB *teb;
    GLenum target;
    GLboolean reset;
    GLenum format;
    GLenum type;
    GLsizei bufSize;
    void *values;
};

struct glGetnMinmaxARB_params
{
    TEB *teb;
    GLenum target;
    GLboolean reset;
    GLenum format;
    GLenum type;
    GLsizei bufSize;
    void *values;
};

struct glGetnPixelMapfv_params
{
    TEB *teb;
    GLenum map;
    GLsizei bufSize;
    GLfloat *values;
};

struct glGetnPixelMapfvARB_params
{
    TEB *teb;
    GLenum map;
    GLsizei bufSize;
    GLfloat *values;
};

struct glGetnPixelMapuiv_params
{
    TEB *teb;
    GLenum map;
    GLsizei bufSize;
    GLuint *values;
};

struct glGetnPixelMapuivARB_params
{
    TEB *teb;
    GLenum map;
    GLsizei bufSize;
    GLuint *values;
};

struct glGetnPixelMapusv_params
{
    TEB *teb;
    GLenum map;
    GLsizei bufSize;
    GLushort *values;
};

struct glGetnPixelMapusvARB_params
{
    TEB *teb;
    GLenum map;
    GLsizei bufSize;
    GLushort *values;
};

struct glGetnPolygonStipple_params
{
    TEB *teb;
    GLsizei bufSize;
    GLubyte *pattern;
};

struct glGetnPolygonStippleARB_params
{
    TEB *teb;
    GLsizei bufSize;
    GLubyte *pattern;
};

struct glGetnSeparableFilter_params
{
    TEB *teb;
    GLenum target;
    GLenum format;
    GLenum type;
    GLsizei rowBufSize;
    void *row;
    GLsizei columnBufSize;
    void *column;
    void *span;
};

struct glGetnSeparableFilterARB_params
{
    TEB *teb;
    GLenum target;
    GLenum format;
    GLenum type;
    GLsizei rowBufSize;
    void *row;
    GLsizei columnBufSize;
    void *column;
    void *span;
};

struct glGetnTexImage_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLenum format;
    GLenum type;
    GLsizei bufSize;
    void *pixels;
};

struct glGetnTexImageARB_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLenum format;
    GLenum type;
    GLsizei bufSize;
    void *img;
};

struct glGetnUniformdv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei bufSize;
    GLdouble *params;
};

struct glGetnUniformdvARB_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei bufSize;
    GLdouble *params;
};

struct glGetnUniformfv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei bufSize;
    GLfloat *params;
};

struct glGetnUniformfvARB_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei bufSize;
    GLfloat *params;
};

struct glGetnUniformi64vARB_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei bufSize;
    GLint64 *params;
};

struct glGetnUniformiv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei bufSize;
    GLint *params;
};

struct glGetnUniformivARB_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei bufSize;
    GLint *params;
};

struct glGetnUniformui64vARB_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei bufSize;
    GLuint64 *params;
};

struct glGetnUniformuiv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei bufSize;
    GLuint *params;
};

struct glGetnUniformuivARB_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei bufSize;
    GLuint *params;
};

struct glGlobalAlphaFactorbSUN_params
{
    TEB *teb;
    GLbyte factor;
};

struct glGlobalAlphaFactordSUN_params
{
    TEB *teb;
    GLdouble factor;
};

struct glGlobalAlphaFactorfSUN_params
{
    TEB *teb;
    GLfloat factor;
};

struct glGlobalAlphaFactoriSUN_params
{
    TEB *teb;
    GLint factor;
};

struct glGlobalAlphaFactorsSUN_params
{
    TEB *teb;
    GLshort factor;
};

struct glGlobalAlphaFactorubSUN_params
{
    TEB *teb;
    GLubyte factor;
};

struct glGlobalAlphaFactoruiSUN_params
{
    TEB *teb;
    GLuint factor;
};

struct glGlobalAlphaFactorusSUN_params
{
    TEB *teb;
    GLushort factor;
};

struct glHintPGI_params
{
    TEB *teb;
    GLenum target;
    GLint mode;
};

struct glHistogram_params
{
    TEB *teb;
    GLenum target;
    GLsizei width;
    GLenum internalformat;
    GLboolean sink;
};

struct glHistogramEXT_params
{
    TEB *teb;
    GLenum target;
    GLsizei width;
    GLenum internalformat;
    GLboolean sink;
};

struct glIglooInterfaceSGIX_params
{
    TEB *teb;
    GLenum pname;
    const void *params;
};

struct glImageTransformParameterfHP_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLfloat param;
};

struct glImageTransformParameterfvHP_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    const GLfloat *params;
};

struct glImageTransformParameteriHP_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLint param;
};

struct glImageTransformParameterivHP_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    const GLint *params;
};

struct glImportMemoryFdEXT_params
{
    TEB *teb;
    GLuint memory;
    GLuint64 size;
    GLenum handleType;
    GLint fd;
};

struct glImportMemoryWin32HandleEXT_params
{
    TEB *teb;
    GLuint memory;
    GLuint64 size;
    GLenum handleType;
    void *handle;
};

struct glImportMemoryWin32NameEXT_params
{
    TEB *teb;
    GLuint memory;
    GLuint64 size;
    GLenum handleType;
    const void *name;
};

struct glImportSemaphoreFdEXT_params
{
    TEB *teb;
    GLuint semaphore;
    GLenum handleType;
    GLint fd;
};

struct glImportSemaphoreWin32HandleEXT_params
{
    TEB *teb;
    GLuint semaphore;
    GLenum handleType;
    void *handle;
};

struct glImportSemaphoreWin32NameEXT_params
{
    TEB *teb;
    GLuint semaphore;
    GLenum handleType;
    const void *name;
};

struct glImportSyncEXT_params
{
    TEB *teb;
    GLenum external_sync_type;
    GLintptr external_sync;
    GLbitfield flags;
    GLsync ret;
};

struct glIndexFormatNV_params
{
    TEB *teb;
    GLenum type;
    GLsizei stride;
};

struct glIndexFuncEXT_params
{
    TEB *teb;
    GLenum func;
    GLclampf ref;
};

struct glIndexMaterialEXT_params
{
    TEB *teb;
    GLenum face;
    GLenum mode;
};

struct glIndexPointerEXT_params
{
    TEB *teb;
    GLenum type;
    GLsizei stride;
    GLsizei count;
    const void *pointer;
};

struct glIndexPointerListIBM_params
{
    TEB *teb;
    GLenum type;
    GLint stride;
    const void **pointer;
    GLint ptrstride;
};

struct glIndexxOES_params
{
    TEB *teb;
    GLfixed component;
};

struct glIndexxvOES_params
{
    TEB *teb;
    const GLfixed *component;
};

struct glInsertComponentEXT_params
{
    TEB *teb;
    GLuint res;
    GLuint src;
    GLuint num;
};

struct glInsertEventMarkerEXT_params
{
    TEB *teb;
    GLsizei length;
    const GLchar *marker;
};

struct glInstrumentsBufferSGIX_params
{
    TEB *teb;
    GLsizei size;
    GLint *buffer;
};

struct glInterpolatePathsNV_params
{
    TEB *teb;
    GLuint resultPath;
    GLuint pathA;
    GLuint pathB;
    GLfloat weight;
};

struct glInvalidateBufferData_params
{
    TEB *teb;
    GLuint buffer;
};

struct glInvalidateBufferSubData_params
{
    TEB *teb;
    GLuint buffer;
    GLintptr offset;
    GLsizeiptr length;
};

struct glInvalidateFramebuffer_params
{
    TEB *teb;
    GLenum target;
    GLsizei numAttachments;
    const GLenum *attachments;
};

struct glInvalidateNamedFramebufferData_params
{
    TEB *teb;
    GLuint framebuffer;
    GLsizei numAttachments;
    const GLenum *attachments;
};

struct glInvalidateNamedFramebufferSubData_params
{
    TEB *teb;
    GLuint framebuffer;
    GLsizei numAttachments;
    const GLenum *attachments;
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
};

struct glInvalidateSubFramebuffer_params
{
    TEB *teb;
    GLenum target;
    GLsizei numAttachments;
    const GLenum *attachments;
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
};

struct glInvalidateTexImage_params
{
    TEB *teb;
    GLuint texture;
    GLint level;
};

struct glInvalidateTexSubImage_params
{
    TEB *teb;
    GLuint texture;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLint zoffset;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
};

struct glIsAsyncMarkerSGIX_params
{
    TEB *teb;
    GLuint marker;
    GLboolean ret;
};

struct glIsBuffer_params
{
    TEB *teb;
    GLuint buffer;
    GLboolean ret;
};

struct glIsBufferARB_params
{
    TEB *teb;
    GLuint buffer;
    GLboolean ret;
};

struct glIsBufferResidentNV_params
{
    TEB *teb;
    GLenum target;
    GLboolean ret;
};

struct glIsCommandListNV_params
{
    TEB *teb;
    GLuint list;
    GLboolean ret;
};

struct glIsEnabledIndexedEXT_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLboolean ret;
};

struct glIsEnabledi_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLboolean ret;
};

struct glIsFenceAPPLE_params
{
    TEB *teb;
    GLuint fence;
    GLboolean ret;
};

struct glIsFenceNV_params
{
    TEB *teb;
    GLuint fence;
    GLboolean ret;
};

struct glIsFramebuffer_params
{
    TEB *teb;
    GLuint framebuffer;
    GLboolean ret;
};

struct glIsFramebufferEXT_params
{
    TEB *teb;
    GLuint framebuffer;
    GLboolean ret;
};

struct glIsImageHandleResidentARB_params
{
    TEB *teb;
    GLuint64 handle;
    GLboolean ret;
};

struct glIsImageHandleResidentNV_params
{
    TEB *teb;
    GLuint64 handle;
    GLboolean ret;
};

struct glIsMemoryObjectEXT_params
{
    TEB *teb;
    GLuint memoryObject;
    GLboolean ret;
};

struct glIsNameAMD_params
{
    TEB *teb;
    GLenum identifier;
    GLuint name;
    GLboolean ret;
};

struct glIsNamedBufferResidentNV_params
{
    TEB *teb;
    GLuint buffer;
    GLboolean ret;
};

struct glIsNamedStringARB_params
{
    TEB *teb;
    GLint namelen;
    const GLchar *name;
    GLboolean ret;
};

struct glIsObjectBufferATI_params
{
    TEB *teb;
    GLuint buffer;
    GLboolean ret;
};

struct glIsOcclusionQueryNV_params
{
    TEB *teb;
    GLuint id;
    GLboolean ret;
};

struct glIsPathNV_params
{
    TEB *teb;
    GLuint path;
    GLboolean ret;
};

struct glIsPointInFillPathNV_params
{
    TEB *teb;
    GLuint path;
    GLuint mask;
    GLfloat x;
    GLfloat y;
    GLboolean ret;
};

struct glIsPointInStrokePathNV_params
{
    TEB *teb;
    GLuint path;
    GLfloat x;
    GLfloat y;
    GLboolean ret;
};

struct glIsProgram_params
{
    TEB *teb;
    GLuint program;
    GLboolean ret;
};

struct glIsProgramARB_params
{
    TEB *teb;
    GLuint program;
    GLboolean ret;
};

struct glIsProgramNV_params
{
    TEB *teb;
    GLuint id;
    GLboolean ret;
};

struct glIsProgramPipeline_params
{
    TEB *teb;
    GLuint pipeline;
    GLboolean ret;
};

struct glIsQuery_params
{
    TEB *teb;
    GLuint id;
    GLboolean ret;
};

struct glIsQueryARB_params
{
    TEB *teb;
    GLuint id;
    GLboolean ret;
};

struct glIsRenderbuffer_params
{
    TEB *teb;
    GLuint renderbuffer;
    GLboolean ret;
};

struct glIsRenderbufferEXT_params
{
    TEB *teb;
    GLuint renderbuffer;
    GLboolean ret;
};

struct glIsSampler_params
{
    TEB *teb;
    GLuint sampler;
    GLboolean ret;
};

struct glIsSemaphoreEXT_params
{
    TEB *teb;
    GLuint semaphore;
    GLboolean ret;
};

struct glIsShader_params
{
    TEB *teb;
    GLuint shader;
    GLboolean ret;
};

struct glIsStateNV_params
{
    TEB *teb;
    GLuint state;
    GLboolean ret;
};

struct glIsSync_params
{
    TEB *teb;
    GLsync sync;
    GLboolean ret;
};

struct glIsTextureEXT_params
{
    TEB *teb;
    GLuint texture;
    GLboolean ret;
};

struct glIsTextureHandleResidentARB_params
{
    TEB *teb;
    GLuint64 handle;
    GLboolean ret;
};

struct glIsTextureHandleResidentNV_params
{
    TEB *teb;
    GLuint64 handle;
    GLboolean ret;
};

struct glIsTransformFeedback_params
{
    TEB *teb;
    GLuint id;
    GLboolean ret;
};

struct glIsTransformFeedbackNV_params
{
    TEB *teb;
    GLuint id;
    GLboolean ret;
};

struct glIsVariantEnabledEXT_params
{
    TEB *teb;
    GLuint id;
    GLenum cap;
    GLboolean ret;
};

struct glIsVertexArray_params
{
    TEB *teb;
    GLuint array;
    GLboolean ret;
};

struct glIsVertexArrayAPPLE_params
{
    TEB *teb;
    GLuint array;
    GLboolean ret;
};

struct glIsVertexAttribEnabledAPPLE_params
{
    TEB *teb;
    GLuint index;
    GLenum pname;
    GLboolean ret;
};

struct glLGPUCopyImageSubDataNVX_params
{
    TEB *teb;
    GLuint sourceGpu;
    GLbitfield destinationGpuMask;
    GLuint srcName;
    GLenum srcTarget;
    GLint srcLevel;
    GLint srcX;
    GLint srxY;
    GLint srcZ;
    GLuint dstName;
    GLenum dstTarget;
    GLint dstLevel;
    GLint dstX;
    GLint dstY;
    GLint dstZ;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
};

struct glLGPUInterlockNVX_params
{
    TEB *teb;
};

struct glLGPUNamedBufferSubDataNVX_params
{
    TEB *teb;
    GLbitfield gpuMask;
    GLuint buffer;
    GLintptr offset;
    GLsizeiptr size;
    const void *data;
};

struct glLabelObjectEXT_params
{
    TEB *teb;
    GLenum type;
    GLuint object;
    GLsizei length;
    const GLchar *label;
};

struct glLightEnviSGIX_params
{
    TEB *teb;
    GLenum pname;
    GLint param;
};

struct glLightModelxOES_params
{
    TEB *teb;
    GLenum pname;
    GLfixed param;
};

struct glLightModelxvOES_params
{
    TEB *teb;
    GLenum pname;
    const GLfixed *param;
};

struct glLightxOES_params
{
    TEB *teb;
    GLenum light;
    GLenum pname;
    GLfixed param;
};

struct glLightxvOES_params
{
    TEB *teb;
    GLenum light;
    GLenum pname;
    const GLfixed *params;
};

struct glLineWidthxOES_params
{
    TEB *teb;
    GLfixed width;
};

struct glLinkProgram_params
{
    TEB *teb;
    GLuint program;
};

struct glLinkProgramARB_params
{
    TEB *teb;
    GLhandleARB programObj;
};

struct glListDrawCommandsStatesClientNV_params
{
    TEB *teb;
    GLuint list;
    GLuint segment;
    const void **indirects;
    const GLsizei *sizes;
    const GLuint *states;
    const GLuint *fbos;
    GLuint count;
};

struct glListParameterfSGIX_params
{
    TEB *teb;
    GLuint list;
    GLenum pname;
    GLfloat param;
};

struct glListParameterfvSGIX_params
{
    TEB *teb;
    GLuint list;
    GLenum pname;
    const GLfloat *params;
};

struct glListParameteriSGIX_params
{
    TEB *teb;
    GLuint list;
    GLenum pname;
    GLint param;
};

struct glListParameterivSGIX_params
{
    TEB *teb;
    GLuint list;
    GLenum pname;
    const GLint *params;
};

struct glLoadIdentityDeformationMapSGIX_params
{
    TEB *teb;
    GLbitfield mask;
};

struct glLoadMatrixxOES_params
{
    TEB *teb;
    const GLfixed *m;
};

struct glLoadProgramNV_params
{
    TEB *teb;
    GLenum target;
    GLuint id;
    GLsizei len;
    const GLubyte *program;
};

struct glLoadTransposeMatrixd_params
{
    TEB *teb;
    const GLdouble *m;
};

struct glLoadTransposeMatrixdARB_params
{
    TEB *teb;
    const GLdouble *m;
};

struct glLoadTransposeMatrixf_params
{
    TEB *teb;
    const GLfloat *m;
};

struct glLoadTransposeMatrixfARB_params
{
    TEB *teb;
    const GLfloat *m;
};

struct glLoadTransposeMatrixxOES_params
{
    TEB *teb;
    const GLfixed *m;
};

struct glLockArraysEXT_params
{
    TEB *teb;
    GLint first;
    GLsizei count;
};

struct glMTexCoord2fSGIS_params
{
    TEB *teb;
    GLenum target;
    GLfloat s;
    GLfloat t;
};

struct glMTexCoord2fvSGIS_params
{
    TEB *teb;
    GLenum target;
    GLfloat * v;
};

struct glMakeBufferNonResidentNV_params
{
    TEB *teb;
    GLenum target;
};

struct glMakeBufferResidentNV_params
{
    TEB *teb;
    GLenum target;
    GLenum access;
};

struct glMakeImageHandleNonResidentARB_params
{
    TEB *teb;
    GLuint64 handle;
};

struct glMakeImageHandleNonResidentNV_params
{
    TEB *teb;
    GLuint64 handle;
};

struct glMakeImageHandleResidentARB_params
{
    TEB *teb;
    GLuint64 handle;
    GLenum access;
};

struct glMakeImageHandleResidentNV_params
{
    TEB *teb;
    GLuint64 handle;
    GLenum access;
};

struct glMakeNamedBufferNonResidentNV_params
{
    TEB *teb;
    GLuint buffer;
};

struct glMakeNamedBufferResidentNV_params
{
    TEB *teb;
    GLuint buffer;
    GLenum access;
};

struct glMakeTextureHandleNonResidentARB_params
{
    TEB *teb;
    GLuint64 handle;
};

struct glMakeTextureHandleNonResidentNV_params
{
    TEB *teb;
    GLuint64 handle;
};

struct glMakeTextureHandleResidentARB_params
{
    TEB *teb;
    GLuint64 handle;
};

struct glMakeTextureHandleResidentNV_params
{
    TEB *teb;
    GLuint64 handle;
};

struct glMap1xOES_params
{
    TEB *teb;
    GLenum target;
    GLfixed u1;
    GLfixed u2;
    GLint stride;
    GLint order;
    GLfixed points;
};

struct glMap2xOES_params
{
    TEB *teb;
    GLenum target;
    GLfixed u1;
    GLfixed u2;
    GLint ustride;
    GLint uorder;
    GLfixed v1;
    GLfixed v2;
    GLint vstride;
    GLint vorder;
    GLfixed points;
};

struct glMapBuffer_params
{
    TEB *teb;
    GLenum target;
    GLenum access;
    void *ret;
};

struct glMapBufferARB_params
{
    TEB *teb;
    GLenum target;
    GLenum access;
    void *ret;
};

struct glMapBufferRange_params
{
    TEB *teb;
    GLenum target;
    GLintptr offset;
    GLsizeiptr length;
    GLbitfield access;
    void *ret;
};

struct glMapControlPointsNV_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLenum type;
    GLsizei ustride;
    GLsizei vstride;
    GLint uorder;
    GLint vorder;
    GLboolean packed;
    const void *points;
};

struct glMapGrid1xOES_params
{
    TEB *teb;
    GLint n;
    GLfixed u1;
    GLfixed u2;
};

struct glMapGrid2xOES_params
{
    TEB *teb;
    GLint n;
    GLfixed u1;
    GLfixed u2;
    GLfixed v1;
    GLfixed v2;
};

struct glMapNamedBuffer_params
{
    TEB *teb;
    GLuint buffer;
    GLenum access;
    void *ret;
};

struct glMapNamedBufferEXT_params
{
    TEB *teb;
    GLuint buffer;
    GLenum access;
    void *ret;
};

struct glMapNamedBufferRange_params
{
    TEB *teb;
    GLuint buffer;
    GLintptr offset;
    GLsizeiptr length;
    GLbitfield access;
    void *ret;
};

struct glMapNamedBufferRangeEXT_params
{
    TEB *teb;
    GLuint buffer;
    GLintptr offset;
    GLsizeiptr length;
    GLbitfield access;
    void *ret;
};

struct glMapObjectBufferATI_params
{
    TEB *teb;
    GLuint buffer;
    void *ret;
};

struct glMapParameterfvNV_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    const GLfloat *params;
};

struct glMapParameterivNV_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    const GLint *params;
};

struct glMapTexture2DINTEL_params
{
    TEB *teb;
    GLuint texture;
    GLint level;
    GLbitfield access;
    GLint *stride;
    GLenum *layout;
    void *ret;
};

struct glMapVertexAttrib1dAPPLE_params
{
    TEB *teb;
    GLuint index;
    GLuint size;
    GLdouble u1;
    GLdouble u2;
    GLint stride;
    GLint order;
    const GLdouble *points;
};

struct glMapVertexAttrib1fAPPLE_params
{
    TEB *teb;
    GLuint index;
    GLuint size;
    GLfloat u1;
    GLfloat u2;
    GLint stride;
    GLint order;
    const GLfloat *points;
};

struct glMapVertexAttrib2dAPPLE_params
{
    TEB *teb;
    GLuint index;
    GLuint size;
    GLdouble u1;
    GLdouble u2;
    GLint ustride;
    GLint uorder;
    GLdouble v1;
    GLdouble v2;
    GLint vstride;
    GLint vorder;
    const GLdouble *points;
};

struct glMapVertexAttrib2fAPPLE_params
{
    TEB *teb;
    GLuint index;
    GLuint size;
    GLfloat u1;
    GLfloat u2;
    GLint ustride;
    GLint uorder;
    GLfloat v1;
    GLfloat v2;
    GLint vstride;
    GLint vorder;
    const GLfloat *points;
};

struct glMaterialxOES_params
{
    TEB *teb;
    GLenum face;
    GLenum pname;
    GLfixed param;
};

struct glMaterialxvOES_params
{
    TEB *teb;
    GLenum face;
    GLenum pname;
    const GLfixed *param;
};

struct glMatrixFrustumEXT_params
{
    TEB *teb;
    GLenum mode;
    GLdouble left;
    GLdouble right;
    GLdouble bottom;
    GLdouble top;
    GLdouble zNear;
    GLdouble zFar;
};

struct glMatrixIndexPointerARB_params
{
    TEB *teb;
    GLint size;
    GLenum type;
    GLsizei stride;
    const void *pointer;
};

struct glMatrixIndexubvARB_params
{
    TEB *teb;
    GLint size;
    const GLubyte *indices;
};

struct glMatrixIndexuivARB_params
{
    TEB *teb;
    GLint size;
    const GLuint *indices;
};

struct glMatrixIndexusvARB_params
{
    TEB *teb;
    GLint size;
    const GLushort *indices;
};

struct glMatrixLoad3x2fNV_params
{
    TEB *teb;
    GLenum matrixMode;
    const GLfloat *m;
};

struct glMatrixLoad3x3fNV_params
{
    TEB *teb;
    GLenum matrixMode;
    const GLfloat *m;
};

struct glMatrixLoadIdentityEXT_params
{
    TEB *teb;
    GLenum mode;
};

struct glMatrixLoadTranspose3x3fNV_params
{
    TEB *teb;
    GLenum matrixMode;
    const GLfloat *m;
};

struct glMatrixLoadTransposedEXT_params
{
    TEB *teb;
    GLenum mode;
    const GLdouble *m;
};

struct glMatrixLoadTransposefEXT_params
{
    TEB *teb;
    GLenum mode;
    const GLfloat *m;
};

struct glMatrixLoaddEXT_params
{
    TEB *teb;
    GLenum mode;
    const GLdouble *m;
};

struct glMatrixLoadfEXT_params
{
    TEB *teb;
    GLenum mode;
    const GLfloat *m;
};

struct glMatrixMult3x2fNV_params
{
    TEB *teb;
    GLenum matrixMode;
    const GLfloat *m;
};

struct glMatrixMult3x3fNV_params
{
    TEB *teb;
    GLenum matrixMode;
    const GLfloat *m;
};

struct glMatrixMultTranspose3x3fNV_params
{
    TEB *teb;
    GLenum matrixMode;
    const GLfloat *m;
};

struct glMatrixMultTransposedEXT_params
{
    TEB *teb;
    GLenum mode;
    const GLdouble *m;
};

struct glMatrixMultTransposefEXT_params
{
    TEB *teb;
    GLenum mode;
    const GLfloat *m;
};

struct glMatrixMultdEXT_params
{
    TEB *teb;
    GLenum mode;
    const GLdouble *m;
};

struct glMatrixMultfEXT_params
{
    TEB *teb;
    GLenum mode;
    const GLfloat *m;
};

struct glMatrixOrthoEXT_params
{
    TEB *teb;
    GLenum mode;
    GLdouble left;
    GLdouble right;
    GLdouble bottom;
    GLdouble top;
    GLdouble zNear;
    GLdouble zFar;
};

struct glMatrixPopEXT_params
{
    TEB *teb;
    GLenum mode;
};

struct glMatrixPushEXT_params
{
    TEB *teb;
    GLenum mode;
};

struct glMatrixRotatedEXT_params
{
    TEB *teb;
    GLenum mode;
    GLdouble angle;
    GLdouble x;
    GLdouble y;
    GLdouble z;
};

struct glMatrixRotatefEXT_params
{
    TEB *teb;
    GLenum mode;
    GLfloat angle;
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

struct glMatrixScaledEXT_params
{
    TEB *teb;
    GLenum mode;
    GLdouble x;
    GLdouble y;
    GLdouble z;
};

struct glMatrixScalefEXT_params
{
    TEB *teb;
    GLenum mode;
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

struct glMatrixTranslatedEXT_params
{
    TEB *teb;
    GLenum mode;
    GLdouble x;
    GLdouble y;
    GLdouble z;
};

struct glMatrixTranslatefEXT_params
{
    TEB *teb;
    GLenum mode;
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

struct glMaxShaderCompilerThreadsARB_params
{
    TEB *teb;
    GLuint count;
};

struct glMaxShaderCompilerThreadsKHR_params
{
    TEB *teb;
    GLuint count;
};

struct glMemoryBarrier_params
{
    TEB *teb;
    GLbitfield barriers;
};

struct glMemoryBarrierByRegion_params
{
    TEB *teb;
    GLbitfield barriers;
};

struct glMemoryBarrierEXT_params
{
    TEB *teb;
    GLbitfield barriers;
};

struct glMemoryObjectParameterivEXT_params
{
    TEB *teb;
    GLuint memoryObject;
    GLenum pname;
    const GLint *params;
};

struct glMinSampleShading_params
{
    TEB *teb;
    GLfloat value;
};

struct glMinSampleShadingARB_params
{
    TEB *teb;
    GLfloat value;
};

struct glMinmax_params
{
    TEB *teb;
    GLenum target;
    GLenum internalformat;
    GLboolean sink;
};

struct glMinmaxEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum internalformat;
    GLboolean sink;
};

struct glMultMatrixxOES_params
{
    TEB *teb;
    const GLfixed *m;
};

struct glMultTransposeMatrixd_params
{
    TEB *teb;
    const GLdouble *m;
};

struct glMultTransposeMatrixdARB_params
{
    TEB *teb;
    const GLdouble *m;
};

struct glMultTransposeMatrixf_params
{
    TEB *teb;
    const GLfloat *m;
};

struct glMultTransposeMatrixfARB_params
{
    TEB *teb;
    const GLfloat *m;
};

struct glMultTransposeMatrixxOES_params
{
    TEB *teb;
    const GLfixed *m;
};

struct glMultiDrawArrays_params
{
    TEB *teb;
    GLenum mode;
    const GLint *first;
    const GLsizei *count;
    GLsizei drawcount;
};

struct glMultiDrawArraysEXT_params
{
    TEB *teb;
    GLenum mode;
    const GLint *first;
    const GLsizei *count;
    GLsizei primcount;
};

struct glMultiDrawArraysIndirect_params
{
    TEB *teb;
    GLenum mode;
    const void *indirect;
    GLsizei drawcount;
    GLsizei stride;
};

struct glMultiDrawArraysIndirectAMD_params
{
    TEB *teb;
    GLenum mode;
    const void *indirect;
    GLsizei primcount;
    GLsizei stride;
};

struct glMultiDrawArraysIndirectBindlessCountNV_params
{
    TEB *teb;
    GLenum mode;
    const void *indirect;
    GLsizei drawCount;
    GLsizei maxDrawCount;
    GLsizei stride;
    GLint vertexBufferCount;
};

struct glMultiDrawArraysIndirectBindlessNV_params
{
    TEB *teb;
    GLenum mode;
    const void *indirect;
    GLsizei drawCount;
    GLsizei stride;
    GLint vertexBufferCount;
};

struct glMultiDrawArraysIndirectCount_params
{
    TEB *teb;
    GLenum mode;
    const void *indirect;
    GLintptr drawcount;
    GLsizei maxdrawcount;
    GLsizei stride;
};

struct glMultiDrawArraysIndirectCountARB_params
{
    TEB *teb;
    GLenum mode;
    const void *indirect;
    GLintptr drawcount;
    GLsizei maxdrawcount;
    GLsizei stride;
};

struct glMultiDrawElementArrayAPPLE_params
{
    TEB *teb;
    GLenum mode;
    const GLint *first;
    const GLsizei *count;
    GLsizei primcount;
};

struct glMultiDrawElements_params
{
    TEB *teb;
    GLenum mode;
    const GLsizei *count;
    GLenum type;
    const void *const*indices;
    GLsizei drawcount;
};

struct glMultiDrawElementsBaseVertex_params
{
    TEB *teb;
    GLenum mode;
    const GLsizei *count;
    GLenum type;
    const void *const*indices;
    GLsizei drawcount;
    const GLint *basevertex;
};

struct glMultiDrawElementsEXT_params
{
    TEB *teb;
    GLenum mode;
    const GLsizei *count;
    GLenum type;
    const void *const*indices;
    GLsizei primcount;
};

struct glMultiDrawElementsIndirect_params
{
    TEB *teb;
    GLenum mode;
    GLenum type;
    const void *indirect;
    GLsizei drawcount;
    GLsizei stride;
};

struct glMultiDrawElementsIndirectAMD_params
{
    TEB *teb;
    GLenum mode;
    GLenum type;
    const void *indirect;
    GLsizei primcount;
    GLsizei stride;
};

struct glMultiDrawElementsIndirectBindlessCountNV_params
{
    TEB *teb;
    GLenum mode;
    GLenum type;
    const void *indirect;
    GLsizei drawCount;
    GLsizei maxDrawCount;
    GLsizei stride;
    GLint vertexBufferCount;
};

struct glMultiDrawElementsIndirectBindlessNV_params
{
    TEB *teb;
    GLenum mode;
    GLenum type;
    const void *indirect;
    GLsizei drawCount;
    GLsizei stride;
    GLint vertexBufferCount;
};

struct glMultiDrawElementsIndirectCount_params
{
    TEB *teb;
    GLenum mode;
    GLenum type;
    const void *indirect;
    GLintptr drawcount;
    GLsizei maxdrawcount;
    GLsizei stride;
};

struct glMultiDrawElementsIndirectCountARB_params
{
    TEB *teb;
    GLenum mode;
    GLenum type;
    const void *indirect;
    GLintptr drawcount;
    GLsizei maxdrawcount;
    GLsizei stride;
};

struct glMultiDrawMeshTasksIndirectCountNV_params
{
    TEB *teb;
    GLintptr indirect;
    GLintptr drawcount;
    GLsizei maxdrawcount;
    GLsizei stride;
};

struct glMultiDrawMeshTasksIndirectNV_params
{
    TEB *teb;
    GLintptr indirect;
    GLsizei drawcount;
    GLsizei stride;
};

struct glMultiDrawRangeElementArrayAPPLE_params
{
    TEB *teb;
    GLenum mode;
    GLuint start;
    GLuint end;
    const GLint *first;
    const GLsizei *count;
    GLsizei primcount;
};

struct glMultiModeDrawArraysIBM_params
{
    TEB *teb;
    const GLenum *mode;
    const GLint *first;
    const GLsizei *count;
    GLsizei primcount;
    GLint modestride;
};

struct glMultiModeDrawElementsIBM_params
{
    TEB *teb;
    const GLenum *mode;
    const GLsizei *count;
    GLenum type;
    const void *const*indices;
    GLsizei primcount;
    GLint modestride;
};

struct glMultiTexBufferEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLenum internalformat;
    GLuint buffer;
};

struct glMultiTexCoord1bOES_params
{
    TEB *teb;
    GLenum texture;
    GLbyte s;
};

struct glMultiTexCoord1bvOES_params
{
    TEB *teb;
    GLenum texture;
    const GLbyte *coords;
};

struct glMultiTexCoord1d_params
{
    TEB *teb;
    GLenum target;
    GLdouble s;
};

struct glMultiTexCoord1dARB_params
{
    TEB *teb;
    GLenum target;
    GLdouble s;
};

struct glMultiTexCoord1dSGIS_params
{
    TEB *teb;
    GLenum target;
    GLdouble s;
};

struct glMultiTexCoord1dv_params
{
    TEB *teb;
    GLenum target;
    const GLdouble *v;
};

struct glMultiTexCoord1dvARB_params
{
    TEB *teb;
    GLenum target;
    const GLdouble *v;
};

struct glMultiTexCoord1dvSGIS_params
{
    TEB *teb;
    GLenum target;
    GLdouble * v;
};

struct glMultiTexCoord1f_params
{
    TEB *teb;
    GLenum target;
    GLfloat s;
};

struct glMultiTexCoord1fARB_params
{
    TEB *teb;
    GLenum target;
    GLfloat s;
};

struct glMultiTexCoord1fSGIS_params
{
    TEB *teb;
    GLenum target;
    GLfloat s;
};

struct glMultiTexCoord1fv_params
{
    TEB *teb;
    GLenum target;
    const GLfloat *v;
};

struct glMultiTexCoord1fvARB_params
{
    TEB *teb;
    GLenum target;
    const GLfloat *v;
};

struct glMultiTexCoord1fvSGIS_params
{
    TEB *teb;
    GLenum target;
    const GLfloat * v;
};

struct glMultiTexCoord1hNV_params
{
    TEB *teb;
    GLenum target;
    GLhalfNV s;
};

struct glMultiTexCoord1hvNV_params
{
    TEB *teb;
    GLenum target;
    const GLhalfNV *v;
};

struct glMultiTexCoord1i_params
{
    TEB *teb;
    GLenum target;
    GLint s;
};

struct glMultiTexCoord1iARB_params
{
    TEB *teb;
    GLenum target;
    GLint s;
};

struct glMultiTexCoord1iSGIS_params
{
    TEB *teb;
    GLenum target;
    GLint s;
};

struct glMultiTexCoord1iv_params
{
    TEB *teb;
    GLenum target;
    const GLint *v;
};

struct glMultiTexCoord1ivARB_params
{
    TEB *teb;
    GLenum target;
    const GLint *v;
};

struct glMultiTexCoord1ivSGIS_params
{
    TEB *teb;
    GLenum target;
    GLint * v;
};

struct glMultiTexCoord1s_params
{
    TEB *teb;
    GLenum target;
    GLshort s;
};

struct glMultiTexCoord1sARB_params
{
    TEB *teb;
    GLenum target;
    GLshort s;
};

struct glMultiTexCoord1sSGIS_params
{
    TEB *teb;
    GLenum target;
    GLshort s;
};

struct glMultiTexCoord1sv_params
{
    TEB *teb;
    GLenum target;
    const GLshort *v;
};

struct glMultiTexCoord1svARB_params
{
    TEB *teb;
    GLenum target;
    const GLshort *v;
};

struct glMultiTexCoord1svSGIS_params
{
    TEB *teb;
    GLenum target;
    GLshort * v;
};

struct glMultiTexCoord1xOES_params
{
    TEB *teb;
    GLenum texture;
    GLfixed s;
};

struct glMultiTexCoord1xvOES_params
{
    TEB *teb;
    GLenum texture;
    const GLfixed *coords;
};

struct glMultiTexCoord2bOES_params
{
    TEB *teb;
    GLenum texture;
    GLbyte s;
    GLbyte t;
};

struct glMultiTexCoord2bvOES_params
{
    TEB *teb;
    GLenum texture;
    const GLbyte *coords;
};

struct glMultiTexCoord2d_params
{
    TEB *teb;
    GLenum target;
    GLdouble s;
    GLdouble t;
};

struct glMultiTexCoord2dARB_params
{
    TEB *teb;
    GLenum target;
    GLdouble s;
    GLdouble t;
};

struct glMultiTexCoord2dSGIS_params
{
    TEB *teb;
    GLenum target;
    GLdouble s;
    GLdouble t;
};

struct glMultiTexCoord2dv_params
{
    TEB *teb;
    GLenum target;
    const GLdouble *v;
};

struct glMultiTexCoord2dvARB_params
{
    TEB *teb;
    GLenum target;
    const GLdouble *v;
};

struct glMultiTexCoord2dvSGIS_params
{
    TEB *teb;
    GLenum target;
    GLdouble * v;
};

struct glMultiTexCoord2f_params
{
    TEB *teb;
    GLenum target;
    GLfloat s;
    GLfloat t;
};

struct glMultiTexCoord2fARB_params
{
    TEB *teb;
    GLenum target;
    GLfloat s;
    GLfloat t;
};

struct glMultiTexCoord2fSGIS_params
{
    TEB *teb;
    GLenum target;
    GLfloat s;
    GLfloat t;
};

struct glMultiTexCoord2fv_params
{
    TEB *teb;
    GLenum target;
    const GLfloat *v;
};

struct glMultiTexCoord2fvARB_params
{
    TEB *teb;
    GLenum target;
    const GLfloat *v;
};

struct glMultiTexCoord2fvSGIS_params
{
    TEB *teb;
    GLenum target;
    GLfloat * v;
};

struct glMultiTexCoord2hNV_params
{
    TEB *teb;
    GLenum target;
    GLhalfNV s;
    GLhalfNV t;
};

struct glMultiTexCoord2hvNV_params
{
    TEB *teb;
    GLenum target;
    const GLhalfNV *v;
};

struct glMultiTexCoord2i_params
{
    TEB *teb;
    GLenum target;
    GLint s;
    GLint t;
};

struct glMultiTexCoord2iARB_params
{
    TEB *teb;
    GLenum target;
    GLint s;
    GLint t;
};

struct glMultiTexCoord2iSGIS_params
{
    TEB *teb;
    GLenum target;
    GLint s;
    GLint t;
};

struct glMultiTexCoord2iv_params
{
    TEB *teb;
    GLenum target;
    const GLint *v;
};

struct glMultiTexCoord2ivARB_params
{
    TEB *teb;
    GLenum target;
    const GLint *v;
};

struct glMultiTexCoord2ivSGIS_params
{
    TEB *teb;
    GLenum target;
    GLint * v;
};

struct glMultiTexCoord2s_params
{
    TEB *teb;
    GLenum target;
    GLshort s;
    GLshort t;
};

struct glMultiTexCoord2sARB_params
{
    TEB *teb;
    GLenum target;
    GLshort s;
    GLshort t;
};

struct glMultiTexCoord2sSGIS_params
{
    TEB *teb;
    GLenum target;
    GLshort s;
    GLshort t;
};

struct glMultiTexCoord2sv_params
{
    TEB *teb;
    GLenum target;
    const GLshort *v;
};

struct glMultiTexCoord2svARB_params
{
    TEB *teb;
    GLenum target;
    const GLshort *v;
};

struct glMultiTexCoord2svSGIS_params
{
    TEB *teb;
    GLenum target;
    GLshort * v;
};

struct glMultiTexCoord2xOES_params
{
    TEB *teb;
    GLenum texture;
    GLfixed s;
    GLfixed t;
};

struct glMultiTexCoord2xvOES_params
{
    TEB *teb;
    GLenum texture;
    const GLfixed *coords;
};

struct glMultiTexCoord3bOES_params
{
    TEB *teb;
    GLenum texture;
    GLbyte s;
    GLbyte t;
    GLbyte r;
};

struct glMultiTexCoord3bvOES_params
{
    TEB *teb;
    GLenum texture;
    const GLbyte *coords;
};

struct glMultiTexCoord3d_params
{
    TEB *teb;
    GLenum target;
    GLdouble s;
    GLdouble t;
    GLdouble r;
};

struct glMultiTexCoord3dARB_params
{
    TEB *teb;
    GLenum target;
    GLdouble s;
    GLdouble t;
    GLdouble r;
};

struct glMultiTexCoord3dSGIS_params
{
    TEB *teb;
    GLenum target;
    GLdouble s;
    GLdouble t;
    GLdouble r;
};

struct glMultiTexCoord3dv_params
{
    TEB *teb;
    GLenum target;
    const GLdouble *v;
};

struct glMultiTexCoord3dvARB_params
{
    TEB *teb;
    GLenum target;
    const GLdouble *v;
};

struct glMultiTexCoord3dvSGIS_params
{
    TEB *teb;
    GLenum target;
    GLdouble * v;
};

struct glMultiTexCoord3f_params
{
    TEB *teb;
    GLenum target;
    GLfloat s;
    GLfloat t;
    GLfloat r;
};

struct glMultiTexCoord3fARB_params
{
    TEB *teb;
    GLenum target;
    GLfloat s;
    GLfloat t;
    GLfloat r;
};

struct glMultiTexCoord3fSGIS_params
{
    TEB *teb;
    GLenum target;
    GLfloat s;
    GLfloat t;
    GLfloat r;
};

struct glMultiTexCoord3fv_params
{
    TEB *teb;
    GLenum target;
    const GLfloat *v;
};

struct glMultiTexCoord3fvARB_params
{
    TEB *teb;
    GLenum target;
    const GLfloat *v;
};

struct glMultiTexCoord3fvSGIS_params
{
    TEB *teb;
    GLenum target;
    GLfloat * v;
};

struct glMultiTexCoord3hNV_params
{
    TEB *teb;
    GLenum target;
    GLhalfNV s;
    GLhalfNV t;
    GLhalfNV r;
};

struct glMultiTexCoord3hvNV_params
{
    TEB *teb;
    GLenum target;
    const GLhalfNV *v;
};

struct glMultiTexCoord3i_params
{
    TEB *teb;
    GLenum target;
    GLint s;
    GLint t;
    GLint r;
};

struct glMultiTexCoord3iARB_params
{
    TEB *teb;
    GLenum target;
    GLint s;
    GLint t;
    GLint r;
};

struct glMultiTexCoord3iSGIS_params
{
    TEB *teb;
    GLenum target;
    GLint s;
    GLint t;
    GLint r;
};

struct glMultiTexCoord3iv_params
{
    TEB *teb;
    GLenum target;
    const GLint *v;
};

struct glMultiTexCoord3ivARB_params
{
    TEB *teb;
    GLenum target;
    const GLint *v;
};

struct glMultiTexCoord3ivSGIS_params
{
    TEB *teb;
    GLenum target;
    GLint * v;
};

struct glMultiTexCoord3s_params
{
    TEB *teb;
    GLenum target;
    GLshort s;
    GLshort t;
    GLshort r;
};

struct glMultiTexCoord3sARB_params
{
    TEB *teb;
    GLenum target;
    GLshort s;
    GLshort t;
    GLshort r;
};

struct glMultiTexCoord3sSGIS_params
{
    TEB *teb;
    GLenum target;
    GLshort s;
    GLshort t;
    GLshort r;
};

struct glMultiTexCoord3sv_params
{
    TEB *teb;
    GLenum target;
    const GLshort *v;
};

struct glMultiTexCoord3svARB_params
{
    TEB *teb;
    GLenum target;
    const GLshort *v;
};

struct glMultiTexCoord3svSGIS_params
{
    TEB *teb;
    GLenum target;
    GLshort * v;
};

struct glMultiTexCoord3xOES_params
{
    TEB *teb;
    GLenum texture;
    GLfixed s;
    GLfixed t;
    GLfixed r;
};

struct glMultiTexCoord3xvOES_params
{
    TEB *teb;
    GLenum texture;
    const GLfixed *coords;
};

struct glMultiTexCoord4bOES_params
{
    TEB *teb;
    GLenum texture;
    GLbyte s;
    GLbyte t;
    GLbyte r;
    GLbyte q;
};

struct glMultiTexCoord4bvOES_params
{
    TEB *teb;
    GLenum texture;
    const GLbyte *coords;
};

struct glMultiTexCoord4d_params
{
    TEB *teb;
    GLenum target;
    GLdouble s;
    GLdouble t;
    GLdouble r;
    GLdouble q;
};

struct glMultiTexCoord4dARB_params
{
    TEB *teb;
    GLenum target;
    GLdouble s;
    GLdouble t;
    GLdouble r;
    GLdouble q;
};

struct glMultiTexCoord4dSGIS_params
{
    TEB *teb;
    GLenum target;
    GLdouble s;
    GLdouble t;
    GLdouble r;
    GLdouble q;
};

struct glMultiTexCoord4dv_params
{
    TEB *teb;
    GLenum target;
    const GLdouble *v;
};

struct glMultiTexCoord4dvARB_params
{
    TEB *teb;
    GLenum target;
    const GLdouble *v;
};

struct glMultiTexCoord4dvSGIS_params
{
    TEB *teb;
    GLenum target;
    GLdouble * v;
};

struct glMultiTexCoord4f_params
{
    TEB *teb;
    GLenum target;
    GLfloat s;
    GLfloat t;
    GLfloat r;
    GLfloat q;
};

struct glMultiTexCoord4fARB_params
{
    TEB *teb;
    GLenum target;
    GLfloat s;
    GLfloat t;
    GLfloat r;
    GLfloat q;
};

struct glMultiTexCoord4fSGIS_params
{
    TEB *teb;
    GLenum target;
    GLfloat s;
    GLfloat t;
    GLfloat r;
    GLfloat q;
};

struct glMultiTexCoord4fv_params
{
    TEB *teb;
    GLenum target;
    const GLfloat *v;
};

struct glMultiTexCoord4fvARB_params
{
    TEB *teb;
    GLenum target;
    const GLfloat *v;
};

struct glMultiTexCoord4fvSGIS_params
{
    TEB *teb;
    GLenum target;
    GLfloat * v;
};

struct glMultiTexCoord4hNV_params
{
    TEB *teb;
    GLenum target;
    GLhalfNV s;
    GLhalfNV t;
    GLhalfNV r;
    GLhalfNV q;
};

struct glMultiTexCoord4hvNV_params
{
    TEB *teb;
    GLenum target;
    const GLhalfNV *v;
};

struct glMultiTexCoord4i_params
{
    TEB *teb;
    GLenum target;
    GLint s;
    GLint t;
    GLint r;
    GLint q;
};

struct glMultiTexCoord4iARB_params
{
    TEB *teb;
    GLenum target;
    GLint s;
    GLint t;
    GLint r;
    GLint q;
};

struct glMultiTexCoord4iSGIS_params
{
    TEB *teb;
    GLenum target;
    GLint s;
    GLint t;
    GLint r;
    GLint q;
};

struct glMultiTexCoord4iv_params
{
    TEB *teb;
    GLenum target;
    const GLint *v;
};

struct glMultiTexCoord4ivARB_params
{
    TEB *teb;
    GLenum target;
    const GLint *v;
};

struct glMultiTexCoord4ivSGIS_params
{
    TEB *teb;
    GLenum target;
    GLint * v;
};

struct glMultiTexCoord4s_params
{
    TEB *teb;
    GLenum target;
    GLshort s;
    GLshort t;
    GLshort r;
    GLshort q;
};

struct glMultiTexCoord4sARB_params
{
    TEB *teb;
    GLenum target;
    GLshort s;
    GLshort t;
    GLshort r;
    GLshort q;
};

struct glMultiTexCoord4sSGIS_params
{
    TEB *teb;
    GLenum target;
    GLshort s;
    GLshort t;
    GLshort r;
    GLshort q;
};

struct glMultiTexCoord4sv_params
{
    TEB *teb;
    GLenum target;
    const GLshort *v;
};

struct glMultiTexCoord4svARB_params
{
    TEB *teb;
    GLenum target;
    const GLshort *v;
};

struct glMultiTexCoord4svSGIS_params
{
    TEB *teb;
    GLenum target;
    GLshort * v;
};

struct glMultiTexCoord4xOES_params
{
    TEB *teb;
    GLenum texture;
    GLfixed s;
    GLfixed t;
    GLfixed r;
    GLfixed q;
};

struct glMultiTexCoord4xvOES_params
{
    TEB *teb;
    GLenum texture;
    const GLfixed *coords;
};

struct glMultiTexCoordP1ui_params
{
    TEB *teb;
    GLenum texture;
    GLenum type;
    GLuint coords;
};

struct glMultiTexCoordP1uiv_params
{
    TEB *teb;
    GLenum texture;
    GLenum type;
    const GLuint *coords;
};

struct glMultiTexCoordP2ui_params
{
    TEB *teb;
    GLenum texture;
    GLenum type;
    GLuint coords;
};

struct glMultiTexCoordP2uiv_params
{
    TEB *teb;
    GLenum texture;
    GLenum type;
    const GLuint *coords;
};

struct glMultiTexCoordP3ui_params
{
    TEB *teb;
    GLenum texture;
    GLenum type;
    GLuint coords;
};

struct glMultiTexCoordP3uiv_params
{
    TEB *teb;
    GLenum texture;
    GLenum type;
    const GLuint *coords;
};

struct glMultiTexCoordP4ui_params
{
    TEB *teb;
    GLenum texture;
    GLenum type;
    GLuint coords;
};

struct glMultiTexCoordP4uiv_params
{
    TEB *teb;
    GLenum texture;
    GLenum type;
    const GLuint *coords;
};

struct glMultiTexCoordPointerEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLint size;
    GLenum type;
    GLsizei stride;
    const void *pointer;
};

struct glMultiTexCoordPointerSGIS_params
{
    TEB *teb;
    GLenum target;
    GLint size;
    GLenum type;
    GLsizei stride;
    GLvoid * pointer;
};

struct glMultiTexEnvfEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLenum pname;
    GLfloat param;
};

struct glMultiTexEnvfvEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLenum pname;
    const GLfloat *params;
};

struct glMultiTexEnviEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLenum pname;
    GLint param;
};

struct glMultiTexEnvivEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLenum pname;
    const GLint *params;
};

struct glMultiTexGendEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum coord;
    GLenum pname;
    GLdouble param;
};

struct glMultiTexGendvEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum coord;
    GLenum pname;
    const GLdouble *params;
};

struct glMultiTexGenfEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum coord;
    GLenum pname;
    GLfloat param;
};

struct glMultiTexGenfvEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum coord;
    GLenum pname;
    const GLfloat *params;
};

struct glMultiTexGeniEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum coord;
    GLenum pname;
    GLint param;
};

struct glMultiTexGenivEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum coord;
    GLenum pname;
    const GLint *params;
};

struct glMultiTexImage1DEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLint level;
    GLint internalformat;
    GLsizei width;
    GLint border;
    GLenum format;
    GLenum type;
    const void *pixels;
};

struct glMultiTexImage2DEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLint level;
    GLint internalformat;
    GLsizei width;
    GLsizei height;
    GLint border;
    GLenum format;
    GLenum type;
    const void *pixels;
};

struct glMultiTexImage3DEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLint level;
    GLint internalformat;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLint border;
    GLenum format;
    GLenum type;
    const void *pixels;
};

struct glMultiTexParameterIivEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLenum pname;
    const GLint *params;
};

struct glMultiTexParameterIuivEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLenum pname;
    const GLuint *params;
};

struct glMultiTexParameterfEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLenum pname;
    GLfloat param;
};

struct glMultiTexParameterfvEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLenum pname;
    const GLfloat *params;
};

struct glMultiTexParameteriEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLenum pname;
    GLint param;
};

struct glMultiTexParameterivEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLenum pname;
    const GLint *params;
};

struct glMultiTexRenderbufferEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLuint renderbuffer;
};

struct glMultiTexSubImage1DEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLsizei width;
    GLenum format;
    GLenum type;
    const void *pixels;
};

struct glMultiTexSubImage2DEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLsizei width;
    GLsizei height;
    GLenum format;
    GLenum type;
    const void *pixels;
};

struct glMultiTexSubImage3DEXT_params
{
    TEB *teb;
    GLenum texunit;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLint zoffset;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLenum format;
    GLenum type;
    const void *pixels;
};

struct glMulticastBarrierNV_params
{
    TEB *teb;
};

struct glMulticastBlitFramebufferNV_params
{
    TEB *teb;
    GLuint srcGpu;
    GLuint dstGpu;
    GLint srcX0;
    GLint srcY0;
    GLint srcX1;
    GLint srcY1;
    GLint dstX0;
    GLint dstY0;
    GLint dstX1;
    GLint dstY1;
    GLbitfield mask;
    GLenum filter;
};

struct glMulticastBufferSubDataNV_params
{
    TEB *teb;
    GLbitfield gpuMask;
    GLuint buffer;
    GLintptr offset;
    GLsizeiptr size;
    const void *data;
};

struct glMulticastCopyBufferSubDataNV_params
{
    TEB *teb;
    GLuint readGpu;
    GLbitfield writeGpuMask;
    GLuint readBuffer;
    GLuint writeBuffer;
    GLintptr readOffset;
    GLintptr writeOffset;
    GLsizeiptr size;
};

struct glMulticastCopyImageSubDataNV_params
{
    TEB *teb;
    GLuint srcGpu;
    GLbitfield dstGpuMask;
    GLuint srcName;
    GLenum srcTarget;
    GLint srcLevel;
    GLint srcX;
    GLint srcY;
    GLint srcZ;
    GLuint dstName;
    GLenum dstTarget;
    GLint dstLevel;
    GLint dstX;
    GLint dstY;
    GLint dstZ;
    GLsizei srcWidth;
    GLsizei srcHeight;
    GLsizei srcDepth;
};

struct glMulticastFramebufferSampleLocationsfvNV_params
{
    TEB *teb;
    GLuint gpu;
    GLuint framebuffer;
    GLuint start;
    GLsizei count;
    const GLfloat *v;
};

struct glMulticastGetQueryObjecti64vNV_params
{
    TEB *teb;
    GLuint gpu;
    GLuint id;
    GLenum pname;
    GLint64 *params;
};

struct glMulticastGetQueryObjectivNV_params
{
    TEB *teb;
    GLuint gpu;
    GLuint id;
    GLenum pname;
    GLint *params;
};

struct glMulticastGetQueryObjectui64vNV_params
{
    TEB *teb;
    GLuint gpu;
    GLuint id;
    GLenum pname;
    GLuint64 *params;
};

struct glMulticastGetQueryObjectuivNV_params
{
    TEB *teb;
    GLuint gpu;
    GLuint id;
    GLenum pname;
    GLuint *params;
};

struct glMulticastScissorArrayvNVX_params
{
    TEB *teb;
    GLuint gpu;
    GLuint first;
    GLsizei count;
    const GLint *v;
};

struct glMulticastViewportArrayvNVX_params
{
    TEB *teb;
    GLuint gpu;
    GLuint first;
    GLsizei count;
    const GLfloat *v;
};

struct glMulticastViewportPositionWScaleNVX_params
{
    TEB *teb;
    GLuint gpu;
    GLuint index;
    GLfloat xcoeff;
    GLfloat ycoeff;
};

struct glMulticastWaitSyncNV_params
{
    TEB *teb;
    GLuint signalGpu;
    GLbitfield waitGpuMask;
};

struct glNamedBufferAttachMemoryNV_params
{
    TEB *teb;
    GLuint buffer;
    GLuint memory;
    GLuint64 offset;
};

struct glNamedBufferData_params
{
    TEB *teb;
    GLuint buffer;
    GLsizeiptr size;
    const void *data;
    GLenum usage;
};

struct glNamedBufferDataEXT_params
{
    TEB *teb;
    GLuint buffer;
    GLsizeiptr size;
    const void *data;
    GLenum usage;
};

struct glNamedBufferPageCommitmentARB_params
{
    TEB *teb;
    GLuint buffer;
    GLintptr offset;
    GLsizeiptr size;
    GLboolean commit;
};

struct glNamedBufferPageCommitmentEXT_params
{
    TEB *teb;
    GLuint buffer;
    GLintptr offset;
    GLsizeiptr size;
    GLboolean commit;
};

struct glNamedBufferStorage_params
{
    TEB *teb;
    GLuint buffer;
    GLsizeiptr size;
    const void *data;
    GLbitfield flags;
};

struct glNamedBufferStorageEXT_params
{
    TEB *teb;
    GLuint buffer;
    GLsizeiptr size;
    const void *data;
    GLbitfield flags;
};

struct glNamedBufferStorageExternalEXT_params
{
    TEB *teb;
    GLuint buffer;
    GLintptr offset;
    GLsizeiptr size;
    GLeglClientBufferEXT clientBuffer;
    GLbitfield flags;
};

struct glNamedBufferStorageMemEXT_params
{
    TEB *teb;
    GLuint buffer;
    GLsizeiptr size;
    GLuint memory;
    GLuint64 offset;
};

struct glNamedBufferSubData_params
{
    TEB *teb;
    GLuint buffer;
    GLintptr offset;
    GLsizeiptr size;
    const void *data;
};

struct glNamedBufferSubDataEXT_params
{
    TEB *teb;
    GLuint buffer;
    GLintptr offset;
    GLsizeiptr size;
    const void *data;
};

struct glNamedCopyBufferSubDataEXT_params
{
    TEB *teb;
    GLuint readBuffer;
    GLuint writeBuffer;
    GLintptr readOffset;
    GLintptr writeOffset;
    GLsizeiptr size;
};

struct glNamedFramebufferDrawBuffer_params
{
    TEB *teb;
    GLuint framebuffer;
    GLenum buf;
};

struct glNamedFramebufferDrawBuffers_params
{
    TEB *teb;
    GLuint framebuffer;
    GLsizei n;
    const GLenum *bufs;
};

struct glNamedFramebufferParameteri_params
{
    TEB *teb;
    GLuint framebuffer;
    GLenum pname;
    GLint param;
};

struct glNamedFramebufferParameteriEXT_params
{
    TEB *teb;
    GLuint framebuffer;
    GLenum pname;
    GLint param;
};

struct glNamedFramebufferReadBuffer_params
{
    TEB *teb;
    GLuint framebuffer;
    GLenum src;
};

struct glNamedFramebufferRenderbuffer_params
{
    TEB *teb;
    GLuint framebuffer;
    GLenum attachment;
    GLenum renderbuffertarget;
    GLuint renderbuffer;
};

struct glNamedFramebufferRenderbufferEXT_params
{
    TEB *teb;
    GLuint framebuffer;
    GLenum attachment;
    GLenum renderbuffertarget;
    GLuint renderbuffer;
};

struct glNamedFramebufferSampleLocationsfvARB_params
{
    TEB *teb;
    GLuint framebuffer;
    GLuint start;
    GLsizei count;
    const GLfloat *v;
};

struct glNamedFramebufferSampleLocationsfvNV_params
{
    TEB *teb;
    GLuint framebuffer;
    GLuint start;
    GLsizei count;
    const GLfloat *v;
};

struct glNamedFramebufferSamplePositionsfvAMD_params
{
    TEB *teb;
    GLuint framebuffer;
    GLuint numsamples;
    GLuint pixelindex;
    const GLfloat *values;
};

struct glNamedFramebufferTexture_params
{
    TEB *teb;
    GLuint framebuffer;
    GLenum attachment;
    GLuint texture;
    GLint level;
};

struct glNamedFramebufferTexture1DEXT_params
{
    TEB *teb;
    GLuint framebuffer;
    GLenum attachment;
    GLenum textarget;
    GLuint texture;
    GLint level;
};

struct glNamedFramebufferTexture2DEXT_params
{
    TEB *teb;
    GLuint framebuffer;
    GLenum attachment;
    GLenum textarget;
    GLuint texture;
    GLint level;
};

struct glNamedFramebufferTexture3DEXT_params
{
    TEB *teb;
    GLuint framebuffer;
    GLenum attachment;
    GLenum textarget;
    GLuint texture;
    GLint level;
    GLint zoffset;
};

struct glNamedFramebufferTextureEXT_params
{
    TEB *teb;
    GLuint framebuffer;
    GLenum attachment;
    GLuint texture;
    GLint level;
};

struct glNamedFramebufferTextureFaceEXT_params
{
    TEB *teb;
    GLuint framebuffer;
    GLenum attachment;
    GLuint texture;
    GLint level;
    GLenum face;
};

struct glNamedFramebufferTextureLayer_params
{
    TEB *teb;
    GLuint framebuffer;
    GLenum attachment;
    GLuint texture;
    GLint level;
    GLint layer;
};

struct glNamedFramebufferTextureLayerEXT_params
{
    TEB *teb;
    GLuint framebuffer;
    GLenum attachment;
    GLuint texture;
    GLint level;
    GLint layer;
};

struct glNamedProgramLocalParameter4dEXT_params
{
    TEB *teb;
    GLuint program;
    GLenum target;
    GLuint index;
    GLdouble x;
    GLdouble y;
    GLdouble z;
    GLdouble w;
};

struct glNamedProgramLocalParameter4dvEXT_params
{
    TEB *teb;
    GLuint program;
    GLenum target;
    GLuint index;
    const GLdouble *params;
};

struct glNamedProgramLocalParameter4fEXT_params
{
    TEB *teb;
    GLuint program;
    GLenum target;
    GLuint index;
    GLfloat x;
    GLfloat y;
    GLfloat z;
    GLfloat w;
};

struct glNamedProgramLocalParameter4fvEXT_params
{
    TEB *teb;
    GLuint program;
    GLenum target;
    GLuint index;
    const GLfloat *params;
};

struct glNamedProgramLocalParameterI4iEXT_params
{
    TEB *teb;
    GLuint program;
    GLenum target;
    GLuint index;
    GLint x;
    GLint y;
    GLint z;
    GLint w;
};

struct glNamedProgramLocalParameterI4ivEXT_params
{
    TEB *teb;
    GLuint program;
    GLenum target;
    GLuint index;
    const GLint *params;
};

struct glNamedProgramLocalParameterI4uiEXT_params
{
    TEB *teb;
    GLuint program;
    GLenum target;
    GLuint index;
    GLuint x;
    GLuint y;
    GLuint z;
    GLuint w;
};

struct glNamedProgramLocalParameterI4uivEXT_params
{
    TEB *teb;
    GLuint program;
    GLenum target;
    GLuint index;
    const GLuint *params;
};

struct glNamedProgramLocalParameters4fvEXT_params
{
    TEB *teb;
    GLuint program;
    GLenum target;
    GLuint index;
    GLsizei count;
    const GLfloat *params;
};

struct glNamedProgramLocalParametersI4ivEXT_params
{
    TEB *teb;
    GLuint program;
    GLenum target;
    GLuint index;
    GLsizei count;
    const GLint *params;
};

struct glNamedProgramLocalParametersI4uivEXT_params
{
    TEB *teb;
    GLuint program;
    GLenum target;
    GLuint index;
    GLsizei count;
    const GLuint *params;
};

struct glNamedProgramStringEXT_params
{
    TEB *teb;
    GLuint program;
    GLenum target;
    GLenum format;
    GLsizei len;
    const void *string;
};

struct glNamedRenderbufferStorage_params
{
    TEB *teb;
    GLuint renderbuffer;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
};

struct glNamedRenderbufferStorageEXT_params
{
    TEB *teb;
    GLuint renderbuffer;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
};

struct glNamedRenderbufferStorageMultisample_params
{
    TEB *teb;
    GLuint renderbuffer;
    GLsizei samples;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
};

struct glNamedRenderbufferStorageMultisampleAdvancedAMD_params
{
    TEB *teb;
    GLuint renderbuffer;
    GLsizei samples;
    GLsizei storageSamples;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
};

struct glNamedRenderbufferStorageMultisampleCoverageEXT_params
{
    TEB *teb;
    GLuint renderbuffer;
    GLsizei coverageSamples;
    GLsizei colorSamples;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
};

struct glNamedRenderbufferStorageMultisampleEXT_params
{
    TEB *teb;
    GLuint renderbuffer;
    GLsizei samples;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
};

struct glNamedStringARB_params
{
    TEB *teb;
    GLenum type;
    GLint namelen;
    const GLchar *name;
    GLint stringlen;
    const GLchar *string;
};

struct glNewBufferRegion_params
{
    TEB *teb;
    GLenum type;
    GLuint ret;
};

struct glNewObjectBufferATI_params
{
    TEB *teb;
    GLsizei size;
    const void *pointer;
    GLenum usage;
    GLuint ret;
};

struct glNormal3fVertex3fSUN_params
{
    TEB *teb;
    GLfloat nx;
    GLfloat ny;
    GLfloat nz;
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

struct glNormal3fVertex3fvSUN_params
{
    TEB *teb;
    const GLfloat *n;
    const GLfloat *v;
};

struct glNormal3hNV_params
{
    TEB *teb;
    GLhalfNV nx;
    GLhalfNV ny;
    GLhalfNV nz;
};

struct glNormal3hvNV_params
{
    TEB *teb;
    const GLhalfNV *v;
};

struct glNormal3xOES_params
{
    TEB *teb;
    GLfixed nx;
    GLfixed ny;
    GLfixed nz;
};

struct glNormal3xvOES_params
{
    TEB *teb;
    const GLfixed *coords;
};

struct glNormalFormatNV_params
{
    TEB *teb;
    GLenum type;
    GLsizei stride;
};

struct glNormalP3ui_params
{
    TEB *teb;
    GLenum type;
    GLuint coords;
};

struct glNormalP3uiv_params
{
    TEB *teb;
    GLenum type;
    const GLuint *coords;
};

struct glNormalPointerEXT_params
{
    TEB *teb;
    GLenum type;
    GLsizei stride;
    GLsizei count;
    const void *pointer;
};

struct glNormalPointerListIBM_params
{
    TEB *teb;
    GLenum type;
    GLint stride;
    const void **pointer;
    GLint ptrstride;
};

struct glNormalPointervINTEL_params
{
    TEB *teb;
    GLenum type;
    const void **pointer;
};

struct glNormalStream3bATI_params
{
    TEB *teb;
    GLenum stream;
    GLbyte nx;
    GLbyte ny;
    GLbyte nz;
};

struct glNormalStream3bvATI_params
{
    TEB *teb;
    GLenum stream;
    const GLbyte *coords;
};

struct glNormalStream3dATI_params
{
    TEB *teb;
    GLenum stream;
    GLdouble nx;
    GLdouble ny;
    GLdouble nz;
};

struct glNormalStream3dvATI_params
{
    TEB *teb;
    GLenum stream;
    const GLdouble *coords;
};

struct glNormalStream3fATI_params
{
    TEB *teb;
    GLenum stream;
    GLfloat nx;
    GLfloat ny;
    GLfloat nz;
};

struct glNormalStream3fvATI_params
{
    TEB *teb;
    GLenum stream;
    const GLfloat *coords;
};

struct glNormalStream3iATI_params
{
    TEB *teb;
    GLenum stream;
    GLint nx;
    GLint ny;
    GLint nz;
};

struct glNormalStream3ivATI_params
{
    TEB *teb;
    GLenum stream;
    const GLint *coords;
};

struct glNormalStream3sATI_params
{
    TEB *teb;
    GLenum stream;
    GLshort nx;
    GLshort ny;
    GLshort nz;
};

struct glNormalStream3svATI_params
{
    TEB *teb;
    GLenum stream;
    const GLshort *coords;
};

struct glObjectLabel_params
{
    TEB *teb;
    GLenum identifier;
    GLuint name;
    GLsizei length;
    const GLchar *label;
};

struct glObjectPtrLabel_params
{
    TEB *teb;
    const void *ptr;
    GLsizei length;
    const GLchar *label;
};

struct glObjectPurgeableAPPLE_params
{
    TEB *teb;
    GLenum objectType;
    GLuint name;
    GLenum option;
    GLenum ret;
};

struct glObjectUnpurgeableAPPLE_params
{
    TEB *teb;
    GLenum objectType;
    GLuint name;
    GLenum option;
    GLenum ret;
};

struct glOrthofOES_params
{
    TEB *teb;
    GLfloat l;
    GLfloat r;
    GLfloat b;
    GLfloat t;
    GLfloat n;
    GLfloat f;
};

struct glOrthoxOES_params
{
    TEB *teb;
    GLfixed l;
    GLfixed r;
    GLfixed b;
    GLfixed t;
    GLfixed n;
    GLfixed f;
};

struct glPNTrianglesfATI_params
{
    TEB *teb;
    GLenum pname;
    GLfloat param;
};

struct glPNTrianglesiATI_params
{
    TEB *teb;
    GLenum pname;
    GLint param;
};

struct glPassTexCoordATI_params
{
    TEB *teb;
    GLuint dst;
    GLuint coord;
    GLenum swizzle;
};

struct glPassThroughxOES_params
{
    TEB *teb;
    GLfixed token;
};

struct glPatchParameterfv_params
{
    TEB *teb;
    GLenum pname;
    const GLfloat *values;
};

struct glPatchParameteri_params
{
    TEB *teb;
    GLenum pname;
    GLint value;
};

struct glPathColorGenNV_params
{
    TEB *teb;
    GLenum color;
    GLenum genMode;
    GLenum colorFormat;
    const GLfloat *coeffs;
};

struct glPathCommandsNV_params
{
    TEB *teb;
    GLuint path;
    GLsizei numCommands;
    const GLubyte *commands;
    GLsizei numCoords;
    GLenum coordType;
    const void *coords;
};

struct glPathCoordsNV_params
{
    TEB *teb;
    GLuint path;
    GLsizei numCoords;
    GLenum coordType;
    const void *coords;
};

struct glPathCoverDepthFuncNV_params
{
    TEB *teb;
    GLenum func;
};

struct glPathDashArrayNV_params
{
    TEB *teb;
    GLuint path;
    GLsizei dashCount;
    const GLfloat *dashArray;
};

struct glPathFogGenNV_params
{
    TEB *teb;
    GLenum genMode;
};

struct glPathGlyphIndexArrayNV_params
{
    TEB *teb;
    GLuint firstPathName;
    GLenum fontTarget;
    const void *fontName;
    GLbitfield fontStyle;
    GLuint firstGlyphIndex;
    GLsizei numGlyphs;
    GLuint pathParameterTemplate;
    GLfloat emScale;
    GLenum ret;
};

struct glPathGlyphIndexRangeNV_params
{
    TEB *teb;
    GLenum fontTarget;
    const void *fontName;
    GLbitfield fontStyle;
    GLuint pathParameterTemplate;
    GLfloat emScale;
    GLuint baseAndCount[2];
    GLenum ret;
};

struct glPathGlyphRangeNV_params
{
    TEB *teb;
    GLuint firstPathName;
    GLenum fontTarget;
    const void *fontName;
    GLbitfield fontStyle;
    GLuint firstGlyph;
    GLsizei numGlyphs;
    GLenum handleMissingGlyphs;
    GLuint pathParameterTemplate;
    GLfloat emScale;
};

struct glPathGlyphsNV_params
{
    TEB *teb;
    GLuint firstPathName;
    GLenum fontTarget;
    const void *fontName;
    GLbitfield fontStyle;
    GLsizei numGlyphs;
    GLenum type;
    const void *charcodes;
    GLenum handleMissingGlyphs;
    GLuint pathParameterTemplate;
    GLfloat emScale;
};

struct glPathMemoryGlyphIndexArrayNV_params
{
    TEB *teb;
    GLuint firstPathName;
    GLenum fontTarget;
    GLsizeiptr fontSize;
    const void *fontData;
    GLsizei faceIndex;
    GLuint firstGlyphIndex;
    GLsizei numGlyphs;
    GLuint pathParameterTemplate;
    GLfloat emScale;
    GLenum ret;
};

struct glPathParameterfNV_params
{
    TEB *teb;
    GLuint path;
    GLenum pname;
    GLfloat value;
};

struct glPathParameterfvNV_params
{
    TEB *teb;
    GLuint path;
    GLenum pname;
    const GLfloat *value;
};

struct glPathParameteriNV_params
{
    TEB *teb;
    GLuint path;
    GLenum pname;
    GLint value;
};

struct glPathParameterivNV_params
{
    TEB *teb;
    GLuint path;
    GLenum pname;
    const GLint *value;
};

struct glPathStencilDepthOffsetNV_params
{
    TEB *teb;
    GLfloat factor;
    GLfloat units;
};

struct glPathStencilFuncNV_params
{
    TEB *teb;
    GLenum func;
    GLint ref;
    GLuint mask;
};

struct glPathStringNV_params
{
    TEB *teb;
    GLuint path;
    GLenum format;
    GLsizei length;
    const void *pathString;
};

struct glPathSubCommandsNV_params
{
    TEB *teb;
    GLuint path;
    GLsizei commandStart;
    GLsizei commandsToDelete;
    GLsizei numCommands;
    const GLubyte *commands;
    GLsizei numCoords;
    GLenum coordType;
    const void *coords;
};

struct glPathSubCoordsNV_params
{
    TEB *teb;
    GLuint path;
    GLsizei coordStart;
    GLsizei numCoords;
    GLenum coordType;
    const void *coords;
};

struct glPathTexGenNV_params
{
    TEB *teb;
    GLenum texCoordSet;
    GLenum genMode;
    GLint components;
    const GLfloat *coeffs;
};

struct glPauseTransformFeedback_params
{
    TEB *teb;
};

struct glPauseTransformFeedbackNV_params
{
    TEB *teb;
};

struct glPixelDataRangeNV_params
{
    TEB *teb;
    GLenum target;
    GLsizei length;
    const void *pointer;
};

struct glPixelMapx_params
{
    TEB *teb;
    GLenum map;
    GLint size;
    const GLfixed *values;
};

struct glPixelStorex_params
{
    TEB *teb;
    GLenum pname;
    GLfixed param;
};

struct glPixelTexGenParameterfSGIS_params
{
    TEB *teb;
    GLenum pname;
    GLfloat param;
};

struct glPixelTexGenParameterfvSGIS_params
{
    TEB *teb;
    GLenum pname;
    const GLfloat *params;
};

struct glPixelTexGenParameteriSGIS_params
{
    TEB *teb;
    GLenum pname;
    GLint param;
};

struct glPixelTexGenParameterivSGIS_params
{
    TEB *teb;
    GLenum pname;
    const GLint *params;
};

struct glPixelTexGenSGIX_params
{
    TEB *teb;
    GLenum mode;
};

struct glPixelTransferxOES_params
{
    TEB *teb;
    GLenum pname;
    GLfixed param;
};

struct glPixelTransformParameterfEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLfloat param;
};

struct glPixelTransformParameterfvEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    const GLfloat *params;
};

struct glPixelTransformParameteriEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLint param;
};

struct glPixelTransformParameterivEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    const GLint *params;
};

struct glPixelZoomxOES_params
{
    TEB *teb;
    GLfixed xfactor;
    GLfixed yfactor;
};

struct glPointAlongPathNV_params
{
    TEB *teb;
    GLuint path;
    GLsizei startSegment;
    GLsizei numSegments;
    GLfloat distance;
    GLfloat *x;
    GLfloat *y;
    GLfloat *tangentX;
    GLfloat *tangentY;
    GLboolean ret;
};

struct glPointParameterf_params
{
    TEB *teb;
    GLenum pname;
    GLfloat param;
};

struct glPointParameterfARB_params
{
    TEB *teb;
    GLenum pname;
    GLfloat param;
};

struct glPointParameterfEXT_params
{
    TEB *teb;
    GLenum pname;
    GLfloat param;
};

struct glPointParameterfSGIS_params
{
    TEB *teb;
    GLenum pname;
    GLfloat param;
};

struct glPointParameterfv_params
{
    TEB *teb;
    GLenum pname;
    const GLfloat *params;
};

struct glPointParameterfvARB_params
{
    TEB *teb;
    GLenum pname;
    const GLfloat *params;
};

struct glPointParameterfvEXT_params
{
    TEB *teb;
    GLenum pname;
    const GLfloat *params;
};

struct glPointParameterfvSGIS_params
{
    TEB *teb;
    GLenum pname;
    const GLfloat *params;
};

struct glPointParameteri_params
{
    TEB *teb;
    GLenum pname;
    GLint param;
};

struct glPointParameteriNV_params
{
    TEB *teb;
    GLenum pname;
    GLint param;
};

struct glPointParameteriv_params
{
    TEB *teb;
    GLenum pname;
    const GLint *params;
};

struct glPointParameterivNV_params
{
    TEB *teb;
    GLenum pname;
    const GLint *params;
};

struct glPointParameterxvOES_params
{
    TEB *teb;
    GLenum pname;
    const GLfixed *params;
};

struct glPointSizexOES_params
{
    TEB *teb;
    GLfixed size;
};

struct glPollAsyncSGIX_params
{
    TEB *teb;
    GLuint *markerp;
    GLint ret;
};

struct glPollInstrumentsSGIX_params
{
    TEB *teb;
    GLint *marker_p;
    GLint ret;
};

struct glPolygonOffsetClamp_params
{
    TEB *teb;
    GLfloat factor;
    GLfloat units;
    GLfloat clamp;
};

struct glPolygonOffsetClampEXT_params
{
    TEB *teb;
    GLfloat factor;
    GLfloat units;
    GLfloat clamp;
};

struct glPolygonOffsetEXT_params
{
    TEB *teb;
    GLfloat factor;
    GLfloat bias;
};

struct glPolygonOffsetxOES_params
{
    TEB *teb;
    GLfixed factor;
    GLfixed units;
};

struct glPopDebugGroup_params
{
    TEB *teb;
};

struct glPopGroupMarkerEXT_params
{
    TEB *teb;
};

struct glPresentFrameDualFillNV_params
{
    TEB *teb;
    GLuint video_slot;
    GLuint64EXT minPresentTime;
    GLuint beginPresentTimeId;
    GLuint presentDurationId;
    GLenum type;
    GLenum target0;
    GLuint fill0;
    GLenum target1;
    GLuint fill1;
    GLenum target2;
    GLuint fill2;
    GLenum target3;
    GLuint fill3;
};

struct glPresentFrameKeyedNV_params
{
    TEB *teb;
    GLuint video_slot;
    GLuint64EXT minPresentTime;
    GLuint beginPresentTimeId;
    GLuint presentDurationId;
    GLenum type;
    GLenum target0;
    GLuint fill0;
    GLuint key0;
    GLenum target1;
    GLuint fill1;
    GLuint key1;
};

struct glPrimitiveBoundingBoxARB_params
{
    TEB *teb;
    GLfloat minX;
    GLfloat minY;
    GLfloat minZ;
    GLfloat minW;
    GLfloat maxX;
    GLfloat maxY;
    GLfloat maxZ;
    GLfloat maxW;
};

struct glPrimitiveRestartIndex_params
{
    TEB *teb;
    GLuint index;
};

struct glPrimitiveRestartIndexNV_params
{
    TEB *teb;
    GLuint index;
};

struct glPrimitiveRestartNV_params
{
    TEB *teb;
};

struct glPrioritizeTexturesEXT_params
{
    TEB *teb;
    GLsizei n;
    const GLuint *textures;
    const GLclampf *priorities;
};

struct glPrioritizeTexturesxOES_params
{
    TEB *teb;
    GLsizei n;
    const GLuint *textures;
    const GLfixed *priorities;
};

struct glProgramBinary_params
{
    TEB *teb;
    GLuint program;
    GLenum binaryFormat;
    const void *binary;
    GLsizei length;
};

struct glProgramBufferParametersIivNV_params
{
    TEB *teb;
    GLenum target;
    GLuint bindingIndex;
    GLuint wordIndex;
    GLsizei count;
    const GLint *params;
};

struct glProgramBufferParametersIuivNV_params
{
    TEB *teb;
    GLenum target;
    GLuint bindingIndex;
    GLuint wordIndex;
    GLsizei count;
    const GLuint *params;
};

struct glProgramBufferParametersfvNV_params
{
    TEB *teb;
    GLenum target;
    GLuint bindingIndex;
    GLuint wordIndex;
    GLsizei count;
    const GLfloat *params;
};

struct glProgramEnvParameter4dARB_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLdouble x;
    GLdouble y;
    GLdouble z;
    GLdouble w;
};

struct glProgramEnvParameter4dvARB_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    const GLdouble *params;
};

struct glProgramEnvParameter4fARB_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLfloat x;
    GLfloat y;
    GLfloat z;
    GLfloat w;
};

struct glProgramEnvParameter4fvARB_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    const GLfloat *params;
};

struct glProgramEnvParameterI4iNV_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLint x;
    GLint y;
    GLint z;
    GLint w;
};

struct glProgramEnvParameterI4ivNV_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    const GLint *params;
};

struct glProgramEnvParameterI4uiNV_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLuint x;
    GLuint y;
    GLuint z;
    GLuint w;
};

struct glProgramEnvParameterI4uivNV_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    const GLuint *params;
};

struct glProgramEnvParameters4fvEXT_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLsizei count;
    const GLfloat *params;
};

struct glProgramEnvParametersI4ivNV_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLsizei count;
    const GLint *params;
};

struct glProgramEnvParametersI4uivNV_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLsizei count;
    const GLuint *params;
};

struct glProgramLocalParameter4dARB_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLdouble x;
    GLdouble y;
    GLdouble z;
    GLdouble w;
};

struct glProgramLocalParameter4dvARB_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    const GLdouble *params;
};

struct glProgramLocalParameter4fARB_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLfloat x;
    GLfloat y;
    GLfloat z;
    GLfloat w;
};

struct glProgramLocalParameter4fvARB_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    const GLfloat *params;
};

struct glProgramLocalParameterI4iNV_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLint x;
    GLint y;
    GLint z;
    GLint w;
};

struct glProgramLocalParameterI4ivNV_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    const GLint *params;
};

struct glProgramLocalParameterI4uiNV_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLuint x;
    GLuint y;
    GLuint z;
    GLuint w;
};

struct glProgramLocalParameterI4uivNV_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    const GLuint *params;
};

struct glProgramLocalParameters4fvEXT_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLsizei count;
    const GLfloat *params;
};

struct glProgramLocalParametersI4ivNV_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLsizei count;
    const GLint *params;
};

struct glProgramLocalParametersI4uivNV_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLsizei count;
    const GLuint *params;
};

struct glProgramNamedParameter4dNV_params
{
    TEB *teb;
    GLuint id;
    GLsizei len;
    const GLubyte *name;
    GLdouble x;
    GLdouble y;
    GLdouble z;
    GLdouble w;
};

struct glProgramNamedParameter4dvNV_params
{
    TEB *teb;
    GLuint id;
    GLsizei len;
    const GLubyte *name;
    const GLdouble *v;
};

struct glProgramNamedParameter4fNV_params
{
    TEB *teb;
    GLuint id;
    GLsizei len;
    const GLubyte *name;
    GLfloat x;
    GLfloat y;
    GLfloat z;
    GLfloat w;
};

struct glProgramNamedParameter4fvNV_params
{
    TEB *teb;
    GLuint id;
    GLsizei len;
    const GLubyte *name;
    const GLfloat *v;
};

struct glProgramParameter4dNV_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLdouble x;
    GLdouble y;
    GLdouble z;
    GLdouble w;
};

struct glProgramParameter4dvNV_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    const GLdouble *v;
};

struct glProgramParameter4fNV_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLfloat x;
    GLfloat y;
    GLfloat z;
    GLfloat w;
};

struct glProgramParameter4fvNV_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    const GLfloat *v;
};

struct glProgramParameteri_params
{
    TEB *teb;
    GLuint program;
    GLenum pname;
    GLint value;
};

struct glProgramParameteriARB_params
{
    TEB *teb;
    GLuint program;
    GLenum pname;
    GLint value;
};

struct glProgramParameteriEXT_params
{
    TEB *teb;
    GLuint program;
    GLenum pname;
    GLint value;
};

struct glProgramParameters4dvNV_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLsizei count;
    const GLdouble *v;
};

struct glProgramParameters4fvNV_params
{
    TEB *teb;
    GLenum target;
    GLuint index;
    GLsizei count;
    const GLfloat *v;
};

struct glProgramPathFragmentInputGenNV_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLenum genMode;
    GLint components;
    const GLfloat *coeffs;
};

struct glProgramStringARB_params
{
    TEB *teb;
    GLenum target;
    GLenum format;
    GLsizei len;
    const void *string;
};

struct glProgramSubroutineParametersuivNV_params
{
    TEB *teb;
    GLenum target;
    GLsizei count;
    const GLuint *params;
};

struct glProgramUniform1d_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLdouble v0;
};

struct glProgramUniform1dEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLdouble x;
};

struct glProgramUniform1dv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLdouble *value;
};

struct glProgramUniform1dvEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLdouble *value;
};

struct glProgramUniform1f_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLfloat v0;
};

struct glProgramUniform1fEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLfloat v0;
};

struct glProgramUniform1fv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLfloat *value;
};

struct glProgramUniform1fvEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLfloat *value;
};

struct glProgramUniform1i_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLint v0;
};

struct glProgramUniform1i64ARB_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLint64 x;
};

struct glProgramUniform1i64NV_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLint64EXT x;
};

struct glProgramUniform1i64vARB_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLint64 *value;
};

struct glProgramUniform1i64vNV_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLint64EXT *value;
};

struct glProgramUniform1iEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLint v0;
};

struct glProgramUniform1iv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLint *value;
};

struct glProgramUniform1ivEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLint *value;
};

struct glProgramUniform1ui_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLuint v0;
};

struct glProgramUniform1ui64ARB_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLuint64 x;
};

struct glProgramUniform1ui64NV_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLuint64EXT x;
};

struct glProgramUniform1ui64vARB_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLuint64 *value;
};

struct glProgramUniform1ui64vNV_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLuint64EXT *value;
};

struct glProgramUniform1uiEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLuint v0;
};

struct glProgramUniform1uiv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLuint *value;
};

struct glProgramUniform1uivEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLuint *value;
};

struct glProgramUniform2d_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLdouble v0;
    GLdouble v1;
};

struct glProgramUniform2dEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLdouble x;
    GLdouble y;
};

struct glProgramUniform2dv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLdouble *value;
};

struct glProgramUniform2dvEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLdouble *value;
};

struct glProgramUniform2f_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLfloat v0;
    GLfloat v1;
};

struct glProgramUniform2fEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLfloat v0;
    GLfloat v1;
};

struct glProgramUniform2fv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLfloat *value;
};

struct glProgramUniform2fvEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLfloat *value;
};

struct glProgramUniform2i_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLint v0;
    GLint v1;
};

struct glProgramUniform2i64ARB_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLint64 x;
    GLint64 y;
};

struct glProgramUniform2i64NV_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLint64EXT x;
    GLint64EXT y;
};

struct glProgramUniform2i64vARB_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLint64 *value;
};

struct glProgramUniform2i64vNV_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLint64EXT *value;
};

struct glProgramUniform2iEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLint v0;
    GLint v1;
};

struct glProgramUniform2iv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLint *value;
};

struct glProgramUniform2ivEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLint *value;
};

struct glProgramUniform2ui_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLuint v0;
    GLuint v1;
};

struct glProgramUniform2ui64ARB_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLuint64 x;
    GLuint64 y;
};

struct glProgramUniform2ui64NV_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLuint64EXT x;
    GLuint64EXT y;
};

struct glProgramUniform2ui64vARB_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLuint64 *value;
};

struct glProgramUniform2ui64vNV_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLuint64EXT *value;
};

struct glProgramUniform2uiEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLuint v0;
    GLuint v1;
};

struct glProgramUniform2uiv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLuint *value;
};

struct glProgramUniform2uivEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLuint *value;
};

struct glProgramUniform3d_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLdouble v0;
    GLdouble v1;
    GLdouble v2;
};

struct glProgramUniform3dEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLdouble x;
    GLdouble y;
    GLdouble z;
};

struct glProgramUniform3dv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLdouble *value;
};

struct glProgramUniform3dvEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLdouble *value;
};

struct glProgramUniform3f_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLfloat v0;
    GLfloat v1;
    GLfloat v2;
};

struct glProgramUniform3fEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLfloat v0;
    GLfloat v1;
    GLfloat v2;
};

struct glProgramUniform3fv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLfloat *value;
};

struct glProgramUniform3fvEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLfloat *value;
};

struct glProgramUniform3i_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLint v0;
    GLint v1;
    GLint v2;
};

struct glProgramUniform3i64ARB_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLint64 x;
    GLint64 y;
    GLint64 z;
};

struct glProgramUniform3i64NV_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLint64EXT x;
    GLint64EXT y;
    GLint64EXT z;
};

struct glProgramUniform3i64vARB_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLint64 *value;
};

struct glProgramUniform3i64vNV_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLint64EXT *value;
};

struct glProgramUniform3iEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLint v0;
    GLint v1;
    GLint v2;
};

struct glProgramUniform3iv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLint *value;
};

struct glProgramUniform3ivEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLint *value;
};

struct glProgramUniform3ui_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLuint v0;
    GLuint v1;
    GLuint v2;
};

struct glProgramUniform3ui64ARB_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLuint64 x;
    GLuint64 y;
    GLuint64 z;
};

struct glProgramUniform3ui64NV_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLuint64EXT x;
    GLuint64EXT y;
    GLuint64EXT z;
};

struct glProgramUniform3ui64vARB_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLuint64 *value;
};

struct glProgramUniform3ui64vNV_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLuint64EXT *value;
};

struct glProgramUniform3uiEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLuint v0;
    GLuint v1;
    GLuint v2;
};

struct glProgramUniform3uiv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLuint *value;
};

struct glProgramUniform3uivEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLuint *value;
};

struct glProgramUniform4d_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLdouble v0;
    GLdouble v1;
    GLdouble v2;
    GLdouble v3;
};

struct glProgramUniform4dEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLdouble x;
    GLdouble y;
    GLdouble z;
    GLdouble w;
};

struct glProgramUniform4dv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLdouble *value;
};

struct glProgramUniform4dvEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLdouble *value;
};

struct glProgramUniform4f_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLfloat v0;
    GLfloat v1;
    GLfloat v2;
    GLfloat v3;
};

struct glProgramUniform4fEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLfloat v0;
    GLfloat v1;
    GLfloat v2;
    GLfloat v3;
};

struct glProgramUniform4fv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLfloat *value;
};

struct glProgramUniform4fvEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLfloat *value;
};

struct glProgramUniform4i_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLint v0;
    GLint v1;
    GLint v2;
    GLint v3;
};

struct glProgramUniform4i64ARB_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLint64 x;
    GLint64 y;
    GLint64 z;
    GLint64 w;
};

struct glProgramUniform4i64NV_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLint64EXT x;
    GLint64EXT y;
    GLint64EXT z;
    GLint64EXT w;
};

struct glProgramUniform4i64vARB_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLint64 *value;
};

struct glProgramUniform4i64vNV_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLint64EXT *value;
};

struct glProgramUniform4iEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLint v0;
    GLint v1;
    GLint v2;
    GLint v3;
};

struct glProgramUniform4iv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLint *value;
};

struct glProgramUniform4ivEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLint *value;
};

struct glProgramUniform4ui_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLuint v0;
    GLuint v1;
    GLuint v2;
    GLuint v3;
};

struct glProgramUniform4ui64ARB_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLuint64 x;
    GLuint64 y;
    GLuint64 z;
    GLuint64 w;
};

struct glProgramUniform4ui64NV_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLuint64EXT x;
    GLuint64EXT y;
    GLuint64EXT z;
    GLuint64EXT w;
};

struct glProgramUniform4ui64vARB_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLuint64 *value;
};

struct glProgramUniform4ui64vNV_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLuint64EXT *value;
};

struct glProgramUniform4uiEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLuint v0;
    GLuint v1;
    GLuint v2;
    GLuint v3;
};

struct glProgramUniform4uiv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLuint *value;
};

struct glProgramUniform4uivEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLuint *value;
};

struct glProgramUniformHandleui64ARB_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLuint64 value;
};

struct glProgramUniformHandleui64NV_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLuint64 value;
};

struct glProgramUniformHandleui64vARB_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLuint64 *values;
};

struct glProgramUniformHandleui64vNV_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLuint64 *values;
};

struct glProgramUniformMatrix2dv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLdouble *value;
};

struct glProgramUniformMatrix2dvEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLdouble *value;
};

struct glProgramUniformMatrix2fv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLfloat *value;
};

struct glProgramUniformMatrix2fvEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLfloat *value;
};

struct glProgramUniformMatrix2x3dv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLdouble *value;
};

struct glProgramUniformMatrix2x3dvEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLdouble *value;
};

struct glProgramUniformMatrix2x3fv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLfloat *value;
};

struct glProgramUniformMatrix2x3fvEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLfloat *value;
};

struct glProgramUniformMatrix2x4dv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLdouble *value;
};

struct glProgramUniformMatrix2x4dvEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLdouble *value;
};

struct glProgramUniformMatrix2x4fv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLfloat *value;
};

struct glProgramUniformMatrix2x4fvEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLfloat *value;
};

struct glProgramUniformMatrix3dv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLdouble *value;
};

struct glProgramUniformMatrix3dvEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLdouble *value;
};

struct glProgramUniformMatrix3fv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLfloat *value;
};

struct glProgramUniformMatrix3fvEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLfloat *value;
};

struct glProgramUniformMatrix3x2dv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLdouble *value;
};

struct glProgramUniformMatrix3x2dvEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLdouble *value;
};

struct glProgramUniformMatrix3x2fv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLfloat *value;
};

struct glProgramUniformMatrix3x2fvEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLfloat *value;
};

struct glProgramUniformMatrix3x4dv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLdouble *value;
};

struct glProgramUniformMatrix3x4dvEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLdouble *value;
};

struct glProgramUniformMatrix3x4fv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLfloat *value;
};

struct glProgramUniformMatrix3x4fvEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLfloat *value;
};

struct glProgramUniformMatrix4dv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLdouble *value;
};

struct glProgramUniformMatrix4dvEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLdouble *value;
};

struct glProgramUniformMatrix4fv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLfloat *value;
};

struct glProgramUniformMatrix4fvEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLfloat *value;
};

struct glProgramUniformMatrix4x2dv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLdouble *value;
};

struct glProgramUniformMatrix4x2dvEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLdouble *value;
};

struct glProgramUniformMatrix4x2fv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLfloat *value;
};

struct glProgramUniformMatrix4x2fvEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLfloat *value;
};

struct glProgramUniformMatrix4x3dv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLdouble *value;
};

struct glProgramUniformMatrix4x3dvEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLdouble *value;
};

struct glProgramUniformMatrix4x3fv_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLfloat *value;
};

struct glProgramUniformMatrix4x3fvEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLfloat *value;
};

struct glProgramUniformui64NV_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLuint64EXT value;
};

struct glProgramUniformui64vNV_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLsizei count;
    const GLuint64EXT *value;
};

struct glProgramVertexLimitNV_params
{
    TEB *teb;
    GLenum target;
    GLint limit;
};

struct glProvokingVertex_params
{
    TEB *teb;
    GLenum mode;
};

struct glProvokingVertexEXT_params
{
    TEB *teb;
    GLenum mode;
};

struct glPushClientAttribDefaultEXT_params
{
    TEB *teb;
    GLbitfield mask;
};

struct glPushDebugGroup_params
{
    TEB *teb;
    GLenum source;
    GLuint id;
    GLsizei length;
    const GLchar *message;
};

struct glPushGroupMarkerEXT_params
{
    TEB *teb;
    GLsizei length;
    const GLchar *marker;
};

struct glQueryCounter_params
{
    TEB *teb;
    GLuint id;
    GLenum target;
};

struct glQueryMatrixxOES_params
{
    TEB *teb;
    GLfixed *mantissa;
    GLint *exponent;
    GLbitfield ret;
};

struct glQueryObjectParameteruiAMD_params
{
    TEB *teb;
    GLenum target;
    GLuint id;
    GLenum pname;
    GLuint param;
};

struct glQueryResourceNV_params
{
    TEB *teb;
    GLenum queryType;
    GLint tagId;
    GLuint count;
    GLint *buffer;
    GLint ret;
};

struct glQueryResourceTagNV_params
{
    TEB *teb;
    GLint tagId;
    const GLchar *tagString;
};

struct glRasterPos2xOES_params
{
    TEB *teb;
    GLfixed x;
    GLfixed y;
};

struct glRasterPos2xvOES_params
{
    TEB *teb;
    const GLfixed *coords;
};

struct glRasterPos3xOES_params
{
    TEB *teb;
    GLfixed x;
    GLfixed y;
    GLfixed z;
};

struct glRasterPos3xvOES_params
{
    TEB *teb;
    const GLfixed *coords;
};

struct glRasterPos4xOES_params
{
    TEB *teb;
    GLfixed x;
    GLfixed y;
    GLfixed z;
    GLfixed w;
};

struct glRasterPos4xvOES_params
{
    TEB *teb;
    const GLfixed *coords;
};

struct glRasterSamplesEXT_params
{
    TEB *teb;
    GLuint samples;
    GLboolean fixedsamplelocations;
};

struct glReadBufferRegion_params
{
    TEB *teb;
    GLenum region;
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
};

struct glReadInstrumentsSGIX_params
{
    TEB *teb;
    GLint marker;
};

struct glReadnPixels_params
{
    TEB *teb;
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
    GLenum format;
    GLenum type;
    GLsizei bufSize;
    void *data;
};

struct glReadnPixelsARB_params
{
    TEB *teb;
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
    GLenum format;
    GLenum type;
    GLsizei bufSize;
    void *data;
};

struct glRectxOES_params
{
    TEB *teb;
    GLfixed x1;
    GLfixed y1;
    GLfixed x2;
    GLfixed y2;
};

struct glRectxvOES_params
{
    TEB *teb;
    const GLfixed *v1;
    const GLfixed *v2;
};

struct glReferencePlaneSGIX_params
{
    TEB *teb;
    const GLdouble *equation;
};

struct glReleaseKeyedMutexWin32EXT_params
{
    TEB *teb;
    GLuint memory;
    GLuint64 key;
    GLboolean ret;
};

struct glReleaseShaderCompiler_params
{
    TEB *teb;
};

struct glRenderGpuMaskNV_params
{
    TEB *teb;
    GLbitfield mask;
};

struct glRenderbufferStorage_params
{
    TEB *teb;
    GLenum target;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
};

struct glRenderbufferStorageEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
};

struct glRenderbufferStorageMultisample_params
{
    TEB *teb;
    GLenum target;
    GLsizei samples;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
};

struct glRenderbufferStorageMultisampleAdvancedAMD_params
{
    TEB *teb;
    GLenum target;
    GLsizei samples;
    GLsizei storageSamples;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
};

struct glRenderbufferStorageMultisampleCoverageNV_params
{
    TEB *teb;
    GLenum target;
    GLsizei coverageSamples;
    GLsizei colorSamples;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
};

struct glRenderbufferStorageMultisampleEXT_params
{
    TEB *teb;
    GLenum target;
    GLsizei samples;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
};

struct glReplacementCodePointerSUN_params
{
    TEB *teb;
    GLenum type;
    GLsizei stride;
    const void **pointer;
};

struct glReplacementCodeubSUN_params
{
    TEB *teb;
    GLubyte code;
};

struct glReplacementCodeubvSUN_params
{
    TEB *teb;
    const GLubyte *code;
};

struct glReplacementCodeuiColor3fVertex3fSUN_params
{
    TEB *teb;
    GLuint rc;
    GLfloat r;
    GLfloat g;
    GLfloat b;
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

struct glReplacementCodeuiColor3fVertex3fvSUN_params
{
    TEB *teb;
    const GLuint *rc;
    const GLfloat *c;
    const GLfloat *v;
};

struct glReplacementCodeuiColor4fNormal3fVertex3fSUN_params
{
    TEB *teb;
    GLuint rc;
    GLfloat r;
    GLfloat g;
    GLfloat b;
    GLfloat a;
    GLfloat nx;
    GLfloat ny;
    GLfloat nz;
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

struct glReplacementCodeuiColor4fNormal3fVertex3fvSUN_params
{
    TEB *teb;
    const GLuint *rc;
    const GLfloat *c;
    const GLfloat *n;
    const GLfloat *v;
};

struct glReplacementCodeuiColor4ubVertex3fSUN_params
{
    TEB *teb;
    GLuint rc;
    GLubyte r;
    GLubyte g;
    GLubyte b;
    GLubyte a;
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

struct glReplacementCodeuiColor4ubVertex3fvSUN_params
{
    TEB *teb;
    const GLuint *rc;
    const GLubyte *c;
    const GLfloat *v;
};

struct glReplacementCodeuiNormal3fVertex3fSUN_params
{
    TEB *teb;
    GLuint rc;
    GLfloat nx;
    GLfloat ny;
    GLfloat nz;
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

struct glReplacementCodeuiNormal3fVertex3fvSUN_params
{
    TEB *teb;
    const GLuint *rc;
    const GLfloat *n;
    const GLfloat *v;
};

struct glReplacementCodeuiSUN_params
{
    TEB *teb;
    GLuint code;
};

struct glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN_params
{
    TEB *teb;
    GLuint rc;
    GLfloat s;
    GLfloat t;
    GLfloat r;
    GLfloat g;
    GLfloat b;
    GLfloat a;
    GLfloat nx;
    GLfloat ny;
    GLfloat nz;
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

struct glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN_params
{
    TEB *teb;
    const GLuint *rc;
    const GLfloat *tc;
    const GLfloat *c;
    const GLfloat *n;
    const GLfloat *v;
};

struct glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN_params
{
    TEB *teb;
    GLuint rc;
    GLfloat s;
    GLfloat t;
    GLfloat nx;
    GLfloat ny;
    GLfloat nz;
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

struct glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN_params
{
    TEB *teb;
    const GLuint *rc;
    const GLfloat *tc;
    const GLfloat *n;
    const GLfloat *v;
};

struct glReplacementCodeuiTexCoord2fVertex3fSUN_params
{
    TEB *teb;
    GLuint rc;
    GLfloat s;
    GLfloat t;
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

struct glReplacementCodeuiTexCoord2fVertex3fvSUN_params
{
    TEB *teb;
    const GLuint *rc;
    const GLfloat *tc;
    const GLfloat *v;
};

struct glReplacementCodeuiVertex3fSUN_params
{
    TEB *teb;
    GLuint rc;
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

struct glReplacementCodeuiVertex3fvSUN_params
{
    TEB *teb;
    const GLuint *rc;
    const GLfloat *v;
};

struct glReplacementCodeuivSUN_params
{
    TEB *teb;
    const GLuint *code;
};

struct glReplacementCodeusSUN_params
{
    TEB *teb;
    GLushort code;
};

struct glReplacementCodeusvSUN_params
{
    TEB *teb;
    const GLushort *code;
};

struct glRequestResidentProgramsNV_params
{
    TEB *teb;
    GLsizei n;
    const GLuint *programs;
};

struct glResetHistogram_params
{
    TEB *teb;
    GLenum target;
};

struct glResetHistogramEXT_params
{
    TEB *teb;
    GLenum target;
};

struct glResetMemoryObjectParameterNV_params
{
    TEB *teb;
    GLuint memory;
    GLenum pname;
};

struct glResetMinmax_params
{
    TEB *teb;
    GLenum target;
};

struct glResetMinmaxEXT_params
{
    TEB *teb;
    GLenum target;
};

struct glResizeBuffersMESA_params
{
    TEB *teb;
};

struct glResolveDepthValuesNV_params
{
    TEB *teb;
};

struct glResumeTransformFeedback_params
{
    TEB *teb;
};

struct glResumeTransformFeedbackNV_params
{
    TEB *teb;
};

struct glRotatexOES_params
{
    TEB *teb;
    GLfixed angle;
    GLfixed x;
    GLfixed y;
    GLfixed z;
};

struct glSampleCoverage_params
{
    TEB *teb;
    GLfloat value;
    GLboolean invert;
};

struct glSampleCoverageARB_params
{
    TEB *teb;
    GLfloat value;
    GLboolean invert;
};

struct glSampleMapATI_params
{
    TEB *teb;
    GLuint dst;
    GLuint interp;
    GLenum swizzle;
};

struct glSampleMaskEXT_params
{
    TEB *teb;
    GLclampf value;
    GLboolean invert;
};

struct glSampleMaskIndexedNV_params
{
    TEB *teb;
    GLuint index;
    GLbitfield mask;
};

struct glSampleMaskSGIS_params
{
    TEB *teb;
    GLclampf value;
    GLboolean invert;
};

struct glSampleMaski_params
{
    TEB *teb;
    GLuint maskNumber;
    GLbitfield mask;
};

struct glSamplePatternEXT_params
{
    TEB *teb;
    GLenum pattern;
};

struct glSamplePatternSGIS_params
{
    TEB *teb;
    GLenum pattern;
};

struct glSamplerParameterIiv_params
{
    TEB *teb;
    GLuint sampler;
    GLenum pname;
    const GLint *param;
};

struct glSamplerParameterIuiv_params
{
    TEB *teb;
    GLuint sampler;
    GLenum pname;
    const GLuint *param;
};

struct glSamplerParameterf_params
{
    TEB *teb;
    GLuint sampler;
    GLenum pname;
    GLfloat param;
};

struct glSamplerParameterfv_params
{
    TEB *teb;
    GLuint sampler;
    GLenum pname;
    const GLfloat *param;
};

struct glSamplerParameteri_params
{
    TEB *teb;
    GLuint sampler;
    GLenum pname;
    GLint param;
};

struct glSamplerParameteriv_params
{
    TEB *teb;
    GLuint sampler;
    GLenum pname;
    const GLint *param;
};

struct glScalexOES_params
{
    TEB *teb;
    GLfixed x;
    GLfixed y;
    GLfixed z;
};

struct glScissorArrayv_params
{
    TEB *teb;
    GLuint first;
    GLsizei count;
    const GLint *v;
};

struct glScissorExclusiveArrayvNV_params
{
    TEB *teb;
    GLuint first;
    GLsizei count;
    const GLint *v;
};

struct glScissorExclusiveNV_params
{
    TEB *teb;
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
};

struct glScissorIndexed_params
{
    TEB *teb;
    GLuint index;
    GLint left;
    GLint bottom;
    GLsizei width;
    GLsizei height;
};

struct glScissorIndexedv_params
{
    TEB *teb;
    GLuint index;
    const GLint *v;
};

struct glSecondaryColor3b_params
{
    TEB *teb;
    GLbyte red;
    GLbyte green;
    GLbyte blue;
};

struct glSecondaryColor3bEXT_params
{
    TEB *teb;
    GLbyte red;
    GLbyte green;
    GLbyte blue;
};

struct glSecondaryColor3bv_params
{
    TEB *teb;
    const GLbyte *v;
};

struct glSecondaryColor3bvEXT_params
{
    TEB *teb;
    const GLbyte *v;
};

struct glSecondaryColor3d_params
{
    TEB *teb;
    GLdouble red;
    GLdouble green;
    GLdouble blue;
};

struct glSecondaryColor3dEXT_params
{
    TEB *teb;
    GLdouble red;
    GLdouble green;
    GLdouble blue;
};

struct glSecondaryColor3dv_params
{
    TEB *teb;
    const GLdouble *v;
};

struct glSecondaryColor3dvEXT_params
{
    TEB *teb;
    const GLdouble *v;
};

struct glSecondaryColor3f_params
{
    TEB *teb;
    GLfloat red;
    GLfloat green;
    GLfloat blue;
};

struct glSecondaryColor3fEXT_params
{
    TEB *teb;
    GLfloat red;
    GLfloat green;
    GLfloat blue;
};

struct glSecondaryColor3fv_params
{
    TEB *teb;
    const GLfloat *v;
};

struct glSecondaryColor3fvEXT_params
{
    TEB *teb;
    const GLfloat *v;
};

struct glSecondaryColor3hNV_params
{
    TEB *teb;
    GLhalfNV red;
    GLhalfNV green;
    GLhalfNV blue;
};

struct glSecondaryColor3hvNV_params
{
    TEB *teb;
    const GLhalfNV *v;
};

struct glSecondaryColor3i_params
{
    TEB *teb;
    GLint red;
    GLint green;
    GLint blue;
};

struct glSecondaryColor3iEXT_params
{
    TEB *teb;
    GLint red;
    GLint green;
    GLint blue;
};

struct glSecondaryColor3iv_params
{
    TEB *teb;
    const GLint *v;
};

struct glSecondaryColor3ivEXT_params
{
    TEB *teb;
    const GLint *v;
};

struct glSecondaryColor3s_params
{
    TEB *teb;
    GLshort red;
    GLshort green;
    GLshort blue;
};

struct glSecondaryColor3sEXT_params
{
    TEB *teb;
    GLshort red;
    GLshort green;
    GLshort blue;
};

struct glSecondaryColor3sv_params
{
    TEB *teb;
    const GLshort *v;
};

struct glSecondaryColor3svEXT_params
{
    TEB *teb;
    const GLshort *v;
};

struct glSecondaryColor3ub_params
{
    TEB *teb;
    GLubyte red;
    GLubyte green;
    GLubyte blue;
};

struct glSecondaryColor3ubEXT_params
{
    TEB *teb;
    GLubyte red;
    GLubyte green;
    GLubyte blue;
};

struct glSecondaryColor3ubv_params
{
    TEB *teb;
    const GLubyte *v;
};

struct glSecondaryColor3ubvEXT_params
{
    TEB *teb;
    const GLubyte *v;
};

struct glSecondaryColor3ui_params
{
    TEB *teb;
    GLuint red;
    GLuint green;
    GLuint blue;
};

struct glSecondaryColor3uiEXT_params
{
    TEB *teb;
    GLuint red;
    GLuint green;
    GLuint blue;
};

struct glSecondaryColor3uiv_params
{
    TEB *teb;
    const GLuint *v;
};

struct glSecondaryColor3uivEXT_params
{
    TEB *teb;
    const GLuint *v;
};

struct glSecondaryColor3us_params
{
    TEB *teb;
    GLushort red;
    GLushort green;
    GLushort blue;
};

struct glSecondaryColor3usEXT_params
{
    TEB *teb;
    GLushort red;
    GLushort green;
    GLushort blue;
};

struct glSecondaryColor3usv_params
{
    TEB *teb;
    const GLushort *v;
};

struct glSecondaryColor3usvEXT_params
{
    TEB *teb;
    const GLushort *v;
};

struct glSecondaryColorFormatNV_params
{
    TEB *teb;
    GLint size;
    GLenum type;
    GLsizei stride;
};

struct glSecondaryColorP3ui_params
{
    TEB *teb;
    GLenum type;
    GLuint color;
};

struct glSecondaryColorP3uiv_params
{
    TEB *teb;
    GLenum type;
    const GLuint *color;
};

struct glSecondaryColorPointer_params
{
    TEB *teb;
    GLint size;
    GLenum type;
    GLsizei stride;
    const void *pointer;
};

struct glSecondaryColorPointerEXT_params
{
    TEB *teb;
    GLint size;
    GLenum type;
    GLsizei stride;
    const void *pointer;
};

struct glSecondaryColorPointerListIBM_params
{
    TEB *teb;
    GLint size;
    GLenum type;
    GLint stride;
    const void **pointer;
    GLint ptrstride;
};

struct glSelectPerfMonitorCountersAMD_params
{
    TEB *teb;
    GLuint monitor;
    GLboolean enable;
    GLuint group;
    GLint numCounters;
    GLuint *counterList;
};

struct glSelectTextureCoordSetSGIS_params
{
    TEB *teb;
    GLenum target;
};

struct glSelectTextureSGIS_params
{
    TEB *teb;
    GLenum target;
};

struct glSemaphoreParameterui64vEXT_params
{
    TEB *teb;
    GLuint semaphore;
    GLenum pname;
    const GLuint64 *params;
};

struct glSeparableFilter2D_params
{
    TEB *teb;
    GLenum target;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
    GLenum format;
    GLenum type;
    const void *row;
    const void *column;
};

struct glSeparableFilter2DEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
    GLenum format;
    GLenum type;
    const void *row;
    const void *column;
};

struct glSetFenceAPPLE_params
{
    TEB *teb;
    GLuint fence;
};

struct glSetFenceNV_params
{
    TEB *teb;
    GLuint fence;
    GLenum condition;
};

struct glSetFragmentShaderConstantATI_params
{
    TEB *teb;
    GLuint dst;
    const GLfloat *value;
};

struct glSetInvariantEXT_params
{
    TEB *teb;
    GLuint id;
    GLenum type;
    const void *addr;
};

struct glSetLocalConstantEXT_params
{
    TEB *teb;
    GLuint id;
    GLenum type;
    const void *addr;
};

struct glSetMultisamplefvAMD_params
{
    TEB *teb;
    GLenum pname;
    GLuint index;
    const GLfloat *val;
};

struct glShaderBinary_params
{
    TEB *teb;
    GLsizei count;
    const GLuint *shaders;
    GLenum binaryformat;
    const void *binary;
    GLsizei length;
};

struct glShaderOp1EXT_params
{
    TEB *teb;
    GLenum op;
    GLuint res;
    GLuint arg1;
};

struct glShaderOp2EXT_params
{
    TEB *teb;
    GLenum op;
    GLuint res;
    GLuint arg1;
    GLuint arg2;
};

struct glShaderOp3EXT_params
{
    TEB *teb;
    GLenum op;
    GLuint res;
    GLuint arg1;
    GLuint arg2;
    GLuint arg3;
};

struct glShaderSource_params
{
    TEB *teb;
    GLuint shader;
    GLsizei count;
    const GLchar *const*string;
    const GLint *length;
};

struct glShaderSourceARB_params
{
    TEB *teb;
    GLhandleARB shaderObj;
    GLsizei count;
    const GLcharARB **string;
    const GLint *length;
};

struct glShaderStorageBlockBinding_params
{
    TEB *teb;
    GLuint program;
    GLuint storageBlockIndex;
    GLuint storageBlockBinding;
};

struct glShadingRateImageBarrierNV_params
{
    TEB *teb;
    GLboolean synchronize;
};

struct glShadingRateImagePaletteNV_params
{
    TEB *teb;
    GLuint viewport;
    GLuint first;
    GLsizei count;
    const GLenum *rates;
};

struct glShadingRateSampleOrderCustomNV_params
{
    TEB *teb;
    GLenum rate;
    GLuint samples;
    const GLint *locations;
};

struct glShadingRateSampleOrderNV_params
{
    TEB *teb;
    GLenum order;
};

struct glSharpenTexFuncSGIS_params
{
    TEB *teb;
    GLenum target;
    GLsizei n;
    const GLfloat *points;
};

struct glSignalSemaphoreEXT_params
{
    TEB *teb;
    GLuint semaphore;
    GLuint numBufferBarriers;
    const GLuint *buffers;
    GLuint numTextureBarriers;
    const GLuint *textures;
    const GLenum *dstLayouts;
};

struct glSignalSemaphoreui64NVX_params
{
    TEB *teb;
    GLuint signalGpu;
    GLsizei fenceObjectCount;
    const GLuint *semaphoreArray;
    const GLuint64 *fenceValueArray;
};

struct glSignalVkFenceNV_params
{
    TEB *teb;
    GLuint64 vkFence;
};

struct glSignalVkSemaphoreNV_params
{
    TEB *teb;
    GLuint64 vkSemaphore;
};

struct glSpecializeShader_params
{
    TEB *teb;
    GLuint shader;
    const GLchar *pEntryPoint;
    GLuint numSpecializationConstants;
    const GLuint *pConstantIndex;
    const GLuint *pConstantValue;
};

struct glSpecializeShaderARB_params
{
    TEB *teb;
    GLuint shader;
    const GLchar *pEntryPoint;
    GLuint numSpecializationConstants;
    const GLuint *pConstantIndex;
    const GLuint *pConstantValue;
};

struct glSpriteParameterfSGIX_params
{
    TEB *teb;
    GLenum pname;
    GLfloat param;
};

struct glSpriteParameterfvSGIX_params
{
    TEB *teb;
    GLenum pname;
    const GLfloat *params;
};

struct glSpriteParameteriSGIX_params
{
    TEB *teb;
    GLenum pname;
    GLint param;
};

struct glSpriteParameterivSGIX_params
{
    TEB *teb;
    GLenum pname;
    const GLint *params;
};

struct glStartInstrumentsSGIX_params
{
    TEB *teb;
};

struct glStateCaptureNV_params
{
    TEB *teb;
    GLuint state;
    GLenum mode;
};

struct glStencilClearTagEXT_params
{
    TEB *teb;
    GLsizei stencilTagBits;
    GLuint stencilClearTag;
};

struct glStencilFillPathInstancedNV_params
{
    TEB *teb;
    GLsizei numPaths;
    GLenum pathNameType;
    const void *paths;
    GLuint pathBase;
    GLenum fillMode;
    GLuint mask;
    GLenum transformType;
    const GLfloat *transformValues;
};

struct glStencilFillPathNV_params
{
    TEB *teb;
    GLuint path;
    GLenum fillMode;
    GLuint mask;
};

struct glStencilFuncSeparate_params
{
    TEB *teb;
    GLenum face;
    GLenum func;
    GLint ref;
    GLuint mask;
};

struct glStencilFuncSeparateATI_params
{
    TEB *teb;
    GLenum frontfunc;
    GLenum backfunc;
    GLint ref;
    GLuint mask;
};

struct glStencilMaskSeparate_params
{
    TEB *teb;
    GLenum face;
    GLuint mask;
};

struct glStencilOpSeparate_params
{
    TEB *teb;
    GLenum face;
    GLenum sfail;
    GLenum dpfail;
    GLenum dppass;
};

struct glStencilOpSeparateATI_params
{
    TEB *teb;
    GLenum face;
    GLenum sfail;
    GLenum dpfail;
    GLenum dppass;
};

struct glStencilOpValueAMD_params
{
    TEB *teb;
    GLenum face;
    GLuint value;
};

struct glStencilStrokePathInstancedNV_params
{
    TEB *teb;
    GLsizei numPaths;
    GLenum pathNameType;
    const void *paths;
    GLuint pathBase;
    GLint reference;
    GLuint mask;
    GLenum transformType;
    const GLfloat *transformValues;
};

struct glStencilStrokePathNV_params
{
    TEB *teb;
    GLuint path;
    GLint reference;
    GLuint mask;
};

struct glStencilThenCoverFillPathInstancedNV_params
{
    TEB *teb;
    GLsizei numPaths;
    GLenum pathNameType;
    const void *paths;
    GLuint pathBase;
    GLenum fillMode;
    GLuint mask;
    GLenum coverMode;
    GLenum transformType;
    const GLfloat *transformValues;
};

struct glStencilThenCoverFillPathNV_params
{
    TEB *teb;
    GLuint path;
    GLenum fillMode;
    GLuint mask;
    GLenum coverMode;
};

struct glStencilThenCoverStrokePathInstancedNV_params
{
    TEB *teb;
    GLsizei numPaths;
    GLenum pathNameType;
    const void *paths;
    GLuint pathBase;
    GLint reference;
    GLuint mask;
    GLenum coverMode;
    GLenum transformType;
    const GLfloat *transformValues;
};

struct glStencilThenCoverStrokePathNV_params
{
    TEB *teb;
    GLuint path;
    GLint reference;
    GLuint mask;
    GLenum coverMode;
};

struct glStopInstrumentsSGIX_params
{
    TEB *teb;
    GLint marker;
};

struct glStringMarkerGREMEDY_params
{
    TEB *teb;
    GLsizei len;
    const void *string;
};

struct glSubpixelPrecisionBiasNV_params
{
    TEB *teb;
    GLuint xbits;
    GLuint ybits;
};

struct glSwizzleEXT_params
{
    TEB *teb;
    GLuint res;
    GLuint in;
    GLenum outX;
    GLenum outY;
    GLenum outZ;
    GLenum outW;
};

struct glSyncTextureINTEL_params
{
    TEB *teb;
    GLuint texture;
};

struct glTagSampleBufferSGIX_params
{
    TEB *teb;
};

struct glTangent3bEXT_params
{
    TEB *teb;
    GLbyte tx;
    GLbyte ty;
    GLbyte tz;
};

struct glTangent3bvEXT_params
{
    TEB *teb;
    const GLbyte *v;
};

struct glTangent3dEXT_params
{
    TEB *teb;
    GLdouble tx;
    GLdouble ty;
    GLdouble tz;
};

struct glTangent3dvEXT_params
{
    TEB *teb;
    const GLdouble *v;
};

struct glTangent3fEXT_params
{
    TEB *teb;
    GLfloat tx;
    GLfloat ty;
    GLfloat tz;
};

struct glTangent3fvEXT_params
{
    TEB *teb;
    const GLfloat *v;
};

struct glTangent3iEXT_params
{
    TEB *teb;
    GLint tx;
    GLint ty;
    GLint tz;
};

struct glTangent3ivEXT_params
{
    TEB *teb;
    const GLint *v;
};

struct glTangent3sEXT_params
{
    TEB *teb;
    GLshort tx;
    GLshort ty;
    GLshort tz;
};

struct glTangent3svEXT_params
{
    TEB *teb;
    const GLshort *v;
};

struct glTangentPointerEXT_params
{
    TEB *teb;
    GLenum type;
    GLsizei stride;
    const void *pointer;
};

struct glTbufferMask3DFX_params
{
    TEB *teb;
    GLuint mask;
};

struct glTessellationFactorAMD_params
{
    TEB *teb;
    GLfloat factor;
};

struct glTessellationModeAMD_params
{
    TEB *teb;
    GLenum mode;
};

struct glTestFenceAPPLE_params
{
    TEB *teb;
    GLuint fence;
    GLboolean ret;
};

struct glTestFenceNV_params
{
    TEB *teb;
    GLuint fence;
    GLboolean ret;
};

struct glTestObjectAPPLE_params
{
    TEB *teb;
    GLenum object;
    GLuint name;
    GLboolean ret;
};

struct glTexAttachMemoryNV_params
{
    TEB *teb;
    GLenum target;
    GLuint memory;
    GLuint64 offset;
};

struct glTexBuffer_params
{
    TEB *teb;
    GLenum target;
    GLenum internalformat;
    GLuint buffer;
};

struct glTexBufferARB_params
{
    TEB *teb;
    GLenum target;
    GLenum internalformat;
    GLuint buffer;
};

struct glTexBufferEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum internalformat;
    GLuint buffer;
};

struct glTexBufferRange_params
{
    TEB *teb;
    GLenum target;
    GLenum internalformat;
    GLuint buffer;
    GLintptr offset;
    GLsizeiptr size;
};

struct glTexBumpParameterfvATI_params
{
    TEB *teb;
    GLenum pname;
    const GLfloat *param;
};

struct glTexBumpParameterivATI_params
{
    TEB *teb;
    GLenum pname;
    const GLint *param;
};

struct glTexCoord1bOES_params
{
    TEB *teb;
    GLbyte s;
};

struct glTexCoord1bvOES_params
{
    TEB *teb;
    const GLbyte *coords;
};

struct glTexCoord1hNV_params
{
    TEB *teb;
    GLhalfNV s;
};

struct glTexCoord1hvNV_params
{
    TEB *teb;
    const GLhalfNV *v;
};

struct glTexCoord1xOES_params
{
    TEB *teb;
    GLfixed s;
};

struct glTexCoord1xvOES_params
{
    TEB *teb;
    const GLfixed *coords;
};

struct glTexCoord2bOES_params
{
    TEB *teb;
    GLbyte s;
    GLbyte t;
};

struct glTexCoord2bvOES_params
{
    TEB *teb;
    const GLbyte *coords;
};

struct glTexCoord2fColor3fVertex3fSUN_params
{
    TEB *teb;
    GLfloat s;
    GLfloat t;
    GLfloat r;
    GLfloat g;
    GLfloat b;
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

struct glTexCoord2fColor3fVertex3fvSUN_params
{
    TEB *teb;
    const GLfloat *tc;
    const GLfloat *c;
    const GLfloat *v;
};

struct glTexCoord2fColor4fNormal3fVertex3fSUN_params
{
    TEB *teb;
    GLfloat s;
    GLfloat t;
    GLfloat r;
    GLfloat g;
    GLfloat b;
    GLfloat a;
    GLfloat nx;
    GLfloat ny;
    GLfloat nz;
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

struct glTexCoord2fColor4fNormal3fVertex3fvSUN_params
{
    TEB *teb;
    const GLfloat *tc;
    const GLfloat *c;
    const GLfloat *n;
    const GLfloat *v;
};

struct glTexCoord2fColor4ubVertex3fSUN_params
{
    TEB *teb;
    GLfloat s;
    GLfloat t;
    GLubyte r;
    GLubyte g;
    GLubyte b;
    GLubyte a;
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

struct glTexCoord2fColor4ubVertex3fvSUN_params
{
    TEB *teb;
    const GLfloat *tc;
    const GLubyte *c;
    const GLfloat *v;
};

struct glTexCoord2fNormal3fVertex3fSUN_params
{
    TEB *teb;
    GLfloat s;
    GLfloat t;
    GLfloat nx;
    GLfloat ny;
    GLfloat nz;
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

struct glTexCoord2fNormal3fVertex3fvSUN_params
{
    TEB *teb;
    const GLfloat *tc;
    const GLfloat *n;
    const GLfloat *v;
};

struct glTexCoord2fVertex3fSUN_params
{
    TEB *teb;
    GLfloat s;
    GLfloat t;
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

struct glTexCoord2fVertex3fvSUN_params
{
    TEB *teb;
    const GLfloat *tc;
    const GLfloat *v;
};

struct glTexCoord2hNV_params
{
    TEB *teb;
    GLhalfNV s;
    GLhalfNV t;
};

struct glTexCoord2hvNV_params
{
    TEB *teb;
    const GLhalfNV *v;
};

struct glTexCoord2xOES_params
{
    TEB *teb;
    GLfixed s;
    GLfixed t;
};

struct glTexCoord2xvOES_params
{
    TEB *teb;
    const GLfixed *coords;
};

struct glTexCoord3bOES_params
{
    TEB *teb;
    GLbyte s;
    GLbyte t;
    GLbyte r;
};

struct glTexCoord3bvOES_params
{
    TEB *teb;
    const GLbyte *coords;
};

struct glTexCoord3hNV_params
{
    TEB *teb;
    GLhalfNV s;
    GLhalfNV t;
    GLhalfNV r;
};

struct glTexCoord3hvNV_params
{
    TEB *teb;
    const GLhalfNV *v;
};

struct glTexCoord3xOES_params
{
    TEB *teb;
    GLfixed s;
    GLfixed t;
    GLfixed r;
};

struct glTexCoord3xvOES_params
{
    TEB *teb;
    const GLfixed *coords;
};

struct glTexCoord4bOES_params
{
    TEB *teb;
    GLbyte s;
    GLbyte t;
    GLbyte r;
    GLbyte q;
};

struct glTexCoord4bvOES_params
{
    TEB *teb;
    const GLbyte *coords;
};

struct glTexCoord4fColor4fNormal3fVertex4fSUN_params
{
    TEB *teb;
    GLfloat s;
    GLfloat t;
    GLfloat p;
    GLfloat q;
    GLfloat r;
    GLfloat g;
    GLfloat b;
    GLfloat a;
    GLfloat nx;
    GLfloat ny;
    GLfloat nz;
    GLfloat x;
    GLfloat y;
    GLfloat z;
    GLfloat w;
};

struct glTexCoord4fColor4fNormal3fVertex4fvSUN_params
{
    TEB *teb;
    const GLfloat *tc;
    const GLfloat *c;
    const GLfloat *n;
    const GLfloat *v;
};

struct glTexCoord4fVertex4fSUN_params
{
    TEB *teb;
    GLfloat s;
    GLfloat t;
    GLfloat p;
    GLfloat q;
    GLfloat x;
    GLfloat y;
    GLfloat z;
    GLfloat w;
};

struct glTexCoord4fVertex4fvSUN_params
{
    TEB *teb;
    const GLfloat *tc;
    const GLfloat *v;
};

struct glTexCoord4hNV_params
{
    TEB *teb;
    GLhalfNV s;
    GLhalfNV t;
    GLhalfNV r;
    GLhalfNV q;
};

struct glTexCoord4hvNV_params
{
    TEB *teb;
    const GLhalfNV *v;
};

struct glTexCoord4xOES_params
{
    TEB *teb;
    GLfixed s;
    GLfixed t;
    GLfixed r;
    GLfixed q;
};

struct glTexCoord4xvOES_params
{
    TEB *teb;
    const GLfixed *coords;
};

struct glTexCoordFormatNV_params
{
    TEB *teb;
    GLint size;
    GLenum type;
    GLsizei stride;
};

struct glTexCoordP1ui_params
{
    TEB *teb;
    GLenum type;
    GLuint coords;
};

struct glTexCoordP1uiv_params
{
    TEB *teb;
    GLenum type;
    const GLuint *coords;
};

struct glTexCoordP2ui_params
{
    TEB *teb;
    GLenum type;
    GLuint coords;
};

struct glTexCoordP2uiv_params
{
    TEB *teb;
    GLenum type;
    const GLuint *coords;
};

struct glTexCoordP3ui_params
{
    TEB *teb;
    GLenum type;
    GLuint coords;
};

struct glTexCoordP3uiv_params
{
    TEB *teb;
    GLenum type;
    const GLuint *coords;
};

struct glTexCoordP4ui_params
{
    TEB *teb;
    GLenum type;
    GLuint coords;
};

struct glTexCoordP4uiv_params
{
    TEB *teb;
    GLenum type;
    const GLuint *coords;
};

struct glTexCoordPointerEXT_params
{
    TEB *teb;
    GLint size;
    GLenum type;
    GLsizei stride;
    GLsizei count;
    const void *pointer;
};

struct glTexCoordPointerListIBM_params
{
    TEB *teb;
    GLint size;
    GLenum type;
    GLint stride;
    const void **pointer;
    GLint ptrstride;
};

struct glTexCoordPointervINTEL_params
{
    TEB *teb;
    GLint size;
    GLenum type;
    const void **pointer;
};

struct glTexEnvxOES_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLfixed param;
};

struct glTexEnvxvOES_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    const GLfixed *params;
};

struct glTexFilterFuncSGIS_params
{
    TEB *teb;
    GLenum target;
    GLenum filter;
    GLsizei n;
    const GLfloat *weights;
};

struct glTexGenxOES_params
{
    TEB *teb;
    GLenum coord;
    GLenum pname;
    GLfixed param;
};

struct glTexGenxvOES_params
{
    TEB *teb;
    GLenum coord;
    GLenum pname;
    const GLfixed *params;
};

struct glTexImage2DMultisample_params
{
    TEB *teb;
    GLenum target;
    GLsizei samples;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
    GLboolean fixedsamplelocations;
};

struct glTexImage2DMultisampleCoverageNV_params
{
    TEB *teb;
    GLenum target;
    GLsizei coverageSamples;
    GLsizei colorSamples;
    GLint internalFormat;
    GLsizei width;
    GLsizei height;
    GLboolean fixedSampleLocations;
};

struct glTexImage3D_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLint internalformat;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLint border;
    GLenum format;
    GLenum type;
    const void *pixels;
};

struct glTexImage3DEXT_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLint border;
    GLenum format;
    GLenum type;
    const void *pixels;
};

struct glTexImage3DMultisample_params
{
    TEB *teb;
    GLenum target;
    GLsizei samples;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLboolean fixedsamplelocations;
};

struct glTexImage3DMultisampleCoverageNV_params
{
    TEB *teb;
    GLenum target;
    GLsizei coverageSamples;
    GLsizei colorSamples;
    GLint internalFormat;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLboolean fixedSampleLocations;
};

struct glTexImage4DSGIS_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLsizei size4d;
    GLint border;
    GLenum format;
    GLenum type;
    const void *pixels;
};

struct glTexPageCommitmentARB_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLint zoffset;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLboolean commit;
};

struct glTexParameterIiv_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    const GLint *params;
};

struct glTexParameterIivEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    const GLint *params;
};

struct glTexParameterIuiv_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    const GLuint *params;
};

struct glTexParameterIuivEXT_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    const GLuint *params;
};

struct glTexParameterxOES_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    GLfixed param;
};

struct glTexParameterxvOES_params
{
    TEB *teb;
    GLenum target;
    GLenum pname;
    const GLfixed *params;
};

struct glTexRenderbufferNV_params
{
    TEB *teb;
    GLenum target;
    GLuint renderbuffer;
};

struct glTexStorage1D_params
{
    TEB *teb;
    GLenum target;
    GLsizei levels;
    GLenum internalformat;
    GLsizei width;
};

struct glTexStorage2D_params
{
    TEB *teb;
    GLenum target;
    GLsizei levels;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
};

struct glTexStorage2DMultisample_params
{
    TEB *teb;
    GLenum target;
    GLsizei samples;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
    GLboolean fixedsamplelocations;
};

struct glTexStorage3D_params
{
    TEB *teb;
    GLenum target;
    GLsizei levels;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
};

struct glTexStorage3DMultisample_params
{
    TEB *teb;
    GLenum target;
    GLsizei samples;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLboolean fixedsamplelocations;
};

struct glTexStorageMem1DEXT_params
{
    TEB *teb;
    GLenum target;
    GLsizei levels;
    GLenum internalFormat;
    GLsizei width;
    GLuint memory;
    GLuint64 offset;
};

struct glTexStorageMem2DEXT_params
{
    TEB *teb;
    GLenum target;
    GLsizei levels;
    GLenum internalFormat;
    GLsizei width;
    GLsizei height;
    GLuint memory;
    GLuint64 offset;
};

struct glTexStorageMem2DMultisampleEXT_params
{
    TEB *teb;
    GLenum target;
    GLsizei samples;
    GLenum internalFormat;
    GLsizei width;
    GLsizei height;
    GLboolean fixedSampleLocations;
    GLuint memory;
    GLuint64 offset;
};

struct glTexStorageMem3DEXT_params
{
    TEB *teb;
    GLenum target;
    GLsizei levels;
    GLenum internalFormat;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLuint memory;
    GLuint64 offset;
};

struct glTexStorageMem3DMultisampleEXT_params
{
    TEB *teb;
    GLenum target;
    GLsizei samples;
    GLenum internalFormat;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLboolean fixedSampleLocations;
    GLuint memory;
    GLuint64 offset;
};

struct glTexStorageSparseAMD_params
{
    TEB *teb;
    GLenum target;
    GLenum internalFormat;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLsizei layers;
    GLbitfield flags;
};

struct glTexSubImage1DEXT_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLsizei width;
    GLenum format;
    GLenum type;
    const void *pixels;
};

struct glTexSubImage2DEXT_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLsizei width;
    GLsizei height;
    GLenum format;
    GLenum type;
    const void *pixels;
};

struct glTexSubImage3D_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLint zoffset;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLenum format;
    GLenum type;
    const void *pixels;
};

struct glTexSubImage3DEXT_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLint zoffset;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLenum format;
    GLenum type;
    const void *pixels;
};

struct glTexSubImage4DSGIS_params
{
    TEB *teb;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLint zoffset;
    GLint woffset;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLsizei size4d;
    GLenum format;
    GLenum type;
    const void *pixels;
};

struct glTextureAttachMemoryNV_params
{
    TEB *teb;
    GLuint texture;
    GLuint memory;
    GLuint64 offset;
};

struct glTextureBarrier_params
{
    TEB *teb;
};

struct glTextureBarrierNV_params
{
    TEB *teb;
};

struct glTextureBuffer_params
{
    TEB *teb;
    GLuint texture;
    GLenum internalformat;
    GLuint buffer;
};

struct glTextureBufferEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLenum internalformat;
    GLuint buffer;
};

struct glTextureBufferRange_params
{
    TEB *teb;
    GLuint texture;
    GLenum internalformat;
    GLuint buffer;
    GLintptr offset;
    GLsizeiptr size;
};

struct glTextureBufferRangeEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLenum internalformat;
    GLuint buffer;
    GLintptr offset;
    GLsizeiptr size;
};

struct glTextureColorMaskSGIS_params
{
    TEB *teb;
    GLboolean red;
    GLboolean green;
    GLboolean blue;
    GLboolean alpha;
};

struct glTextureImage1DEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLint level;
    GLint internalformat;
    GLsizei width;
    GLint border;
    GLenum format;
    GLenum type;
    const void *pixels;
};

struct glTextureImage2DEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLint level;
    GLint internalformat;
    GLsizei width;
    GLsizei height;
    GLint border;
    GLenum format;
    GLenum type;
    const void *pixels;
};

struct glTextureImage2DMultisampleCoverageNV_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLsizei coverageSamples;
    GLsizei colorSamples;
    GLint internalFormat;
    GLsizei width;
    GLsizei height;
    GLboolean fixedSampleLocations;
};

struct glTextureImage2DMultisampleNV_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLsizei samples;
    GLint internalFormat;
    GLsizei width;
    GLsizei height;
    GLboolean fixedSampleLocations;
};

struct glTextureImage3DEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLint level;
    GLint internalformat;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLint border;
    GLenum format;
    GLenum type;
    const void *pixels;
};

struct glTextureImage3DMultisampleCoverageNV_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLsizei coverageSamples;
    GLsizei colorSamples;
    GLint internalFormat;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLboolean fixedSampleLocations;
};

struct glTextureImage3DMultisampleNV_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLsizei samples;
    GLint internalFormat;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLboolean fixedSampleLocations;
};

struct glTextureLightEXT_params
{
    TEB *teb;
    GLenum pname;
};

struct glTextureMaterialEXT_params
{
    TEB *teb;
    GLenum face;
    GLenum mode;
};

struct glTextureNormalEXT_params
{
    TEB *teb;
    GLenum mode;
};

struct glTexturePageCommitmentEXT_params
{
    TEB *teb;
    GLuint texture;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLint zoffset;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLboolean commit;
};

struct glTextureParameterIiv_params
{
    TEB *teb;
    GLuint texture;
    GLenum pname;
    const GLint *params;
};

struct glTextureParameterIivEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLenum pname;
    const GLint *params;
};

struct glTextureParameterIuiv_params
{
    TEB *teb;
    GLuint texture;
    GLenum pname;
    const GLuint *params;
};

struct glTextureParameterIuivEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLenum pname;
    const GLuint *params;
};

struct glTextureParameterf_params
{
    TEB *teb;
    GLuint texture;
    GLenum pname;
    GLfloat param;
};

struct glTextureParameterfEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLenum pname;
    GLfloat param;
};

struct glTextureParameterfv_params
{
    TEB *teb;
    GLuint texture;
    GLenum pname;
    const GLfloat *param;
};

struct glTextureParameterfvEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLenum pname;
    const GLfloat *params;
};

struct glTextureParameteri_params
{
    TEB *teb;
    GLuint texture;
    GLenum pname;
    GLint param;
};

struct glTextureParameteriEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLenum pname;
    GLint param;
};

struct glTextureParameteriv_params
{
    TEB *teb;
    GLuint texture;
    GLenum pname;
    const GLint *param;
};

struct glTextureParameterivEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLenum pname;
    const GLint *params;
};

struct glTextureRangeAPPLE_params
{
    TEB *teb;
    GLenum target;
    GLsizei length;
    const void *pointer;
};

struct glTextureRenderbufferEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLuint renderbuffer;
};

struct glTextureStorage1D_params
{
    TEB *teb;
    GLuint texture;
    GLsizei levels;
    GLenum internalformat;
    GLsizei width;
};

struct glTextureStorage1DEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLsizei levels;
    GLenum internalformat;
    GLsizei width;
};

struct glTextureStorage2D_params
{
    TEB *teb;
    GLuint texture;
    GLsizei levels;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
};

struct glTextureStorage2DEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLsizei levels;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
};

struct glTextureStorage2DMultisample_params
{
    TEB *teb;
    GLuint texture;
    GLsizei samples;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
    GLboolean fixedsamplelocations;
};

struct glTextureStorage2DMultisampleEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLsizei samples;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
    GLboolean fixedsamplelocations;
};

struct glTextureStorage3D_params
{
    TEB *teb;
    GLuint texture;
    GLsizei levels;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
};

struct glTextureStorage3DEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLsizei levels;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
};

struct glTextureStorage3DMultisample_params
{
    TEB *teb;
    GLuint texture;
    GLsizei samples;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLboolean fixedsamplelocations;
};

struct glTextureStorage3DMultisampleEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLsizei samples;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLboolean fixedsamplelocations;
};

struct glTextureStorageMem1DEXT_params
{
    TEB *teb;
    GLuint texture;
    GLsizei levels;
    GLenum internalFormat;
    GLsizei width;
    GLuint memory;
    GLuint64 offset;
};

struct glTextureStorageMem2DEXT_params
{
    TEB *teb;
    GLuint texture;
    GLsizei levels;
    GLenum internalFormat;
    GLsizei width;
    GLsizei height;
    GLuint memory;
    GLuint64 offset;
};

struct glTextureStorageMem2DMultisampleEXT_params
{
    TEB *teb;
    GLuint texture;
    GLsizei samples;
    GLenum internalFormat;
    GLsizei width;
    GLsizei height;
    GLboolean fixedSampleLocations;
    GLuint memory;
    GLuint64 offset;
};

struct glTextureStorageMem3DEXT_params
{
    TEB *teb;
    GLuint texture;
    GLsizei levels;
    GLenum internalFormat;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLuint memory;
    GLuint64 offset;
};

struct glTextureStorageMem3DMultisampleEXT_params
{
    TEB *teb;
    GLuint texture;
    GLsizei samples;
    GLenum internalFormat;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLboolean fixedSampleLocations;
    GLuint memory;
    GLuint64 offset;
};

struct glTextureStorageSparseAMD_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLenum internalFormat;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLsizei layers;
    GLbitfield flags;
};

struct glTextureSubImage1D_params
{
    TEB *teb;
    GLuint texture;
    GLint level;
    GLint xoffset;
    GLsizei width;
    GLenum format;
    GLenum type;
    const void *pixels;
};

struct glTextureSubImage1DEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLsizei width;
    GLenum format;
    GLenum type;
    const void *pixels;
};

struct glTextureSubImage2D_params
{
    TEB *teb;
    GLuint texture;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLsizei width;
    GLsizei height;
    GLenum format;
    GLenum type;
    const void *pixels;
};

struct glTextureSubImage2DEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLsizei width;
    GLsizei height;
    GLenum format;
    GLenum type;
    const void *pixels;
};

struct glTextureSubImage3D_params
{
    TEB *teb;
    GLuint texture;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLint zoffset;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLenum format;
    GLenum type;
    const void *pixels;
};

struct glTextureSubImage3DEXT_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLint zoffset;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLenum format;
    GLenum type;
    const void *pixels;
};

struct glTextureView_params
{
    TEB *teb;
    GLuint texture;
    GLenum target;
    GLuint origtexture;
    GLenum internalformat;
    GLuint minlevel;
    GLuint numlevels;
    GLuint minlayer;
    GLuint numlayers;
};

struct glTrackMatrixNV_params
{
    TEB *teb;
    GLenum target;
    GLuint address;
    GLenum matrix;
    GLenum transform;
};

struct glTransformFeedbackAttribsNV_params
{
    TEB *teb;
    GLsizei count;
    const GLint *attribs;
    GLenum bufferMode;
};

struct glTransformFeedbackBufferBase_params
{
    TEB *teb;
    GLuint xfb;
    GLuint index;
    GLuint buffer;
};

struct glTransformFeedbackBufferRange_params
{
    TEB *teb;
    GLuint xfb;
    GLuint index;
    GLuint buffer;
    GLintptr offset;
    GLsizeiptr size;
};

struct glTransformFeedbackStreamAttribsNV_params
{
    TEB *teb;
    GLsizei count;
    const GLint *attribs;
    GLsizei nbuffers;
    const GLint *bufstreams;
    GLenum bufferMode;
};

struct glTransformFeedbackVaryings_params
{
    TEB *teb;
    GLuint program;
    GLsizei count;
    const GLchar *const*varyings;
    GLenum bufferMode;
};

struct glTransformFeedbackVaryingsEXT_params
{
    TEB *teb;
    GLuint program;
    GLsizei count;
    const GLchar *const*varyings;
    GLenum bufferMode;
};

struct glTransformFeedbackVaryingsNV_params
{
    TEB *teb;
    GLuint program;
    GLsizei count;
    const GLint *locations;
    GLenum bufferMode;
};

struct glTransformPathNV_params
{
    TEB *teb;
    GLuint resultPath;
    GLuint srcPath;
    GLenum transformType;
    const GLfloat *transformValues;
};

struct glTranslatexOES_params
{
    TEB *teb;
    GLfixed x;
    GLfixed y;
    GLfixed z;
};

struct glUniform1d_params
{
    TEB *teb;
    GLint location;
    GLdouble x;
};

struct glUniform1dv_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLdouble *value;
};

struct glUniform1f_params
{
    TEB *teb;
    GLint location;
    GLfloat v0;
};

struct glUniform1fARB_params
{
    TEB *teb;
    GLint location;
    GLfloat v0;
};

struct glUniform1fv_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLfloat *value;
};

struct glUniform1fvARB_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLfloat *value;
};

struct glUniform1i_params
{
    TEB *teb;
    GLint location;
    GLint v0;
};

struct glUniform1i64ARB_params
{
    TEB *teb;
    GLint location;
    GLint64 x;
};

struct glUniform1i64NV_params
{
    TEB *teb;
    GLint location;
    GLint64EXT x;
};

struct glUniform1i64vARB_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLint64 *value;
};

struct glUniform1i64vNV_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLint64EXT *value;
};

struct glUniform1iARB_params
{
    TEB *teb;
    GLint location;
    GLint v0;
};

struct glUniform1iv_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLint *value;
};

struct glUniform1ivARB_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLint *value;
};

struct glUniform1ui_params
{
    TEB *teb;
    GLint location;
    GLuint v0;
};

struct glUniform1ui64ARB_params
{
    TEB *teb;
    GLint location;
    GLuint64 x;
};

struct glUniform1ui64NV_params
{
    TEB *teb;
    GLint location;
    GLuint64EXT x;
};

struct glUniform1ui64vARB_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLuint64 *value;
};

struct glUniform1ui64vNV_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLuint64EXT *value;
};

struct glUniform1uiEXT_params
{
    TEB *teb;
    GLint location;
    GLuint v0;
};

struct glUniform1uiv_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLuint *value;
};

struct glUniform1uivEXT_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLuint *value;
};

struct glUniform2d_params
{
    TEB *teb;
    GLint location;
    GLdouble x;
    GLdouble y;
};

struct glUniform2dv_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLdouble *value;
};

struct glUniform2f_params
{
    TEB *teb;
    GLint location;
    GLfloat v0;
    GLfloat v1;
};

struct glUniform2fARB_params
{
    TEB *teb;
    GLint location;
    GLfloat v0;
    GLfloat v1;
};

struct glUniform2fv_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLfloat *value;
};

struct glUniform2fvARB_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLfloat *value;
};

struct glUniform2i_params
{
    TEB *teb;
    GLint location;
    GLint v0;
    GLint v1;
};

struct glUniform2i64ARB_params
{
    TEB *teb;
    GLint location;
    GLint64 x;
    GLint64 y;
};

struct glUniform2i64NV_params
{
    TEB *teb;
    GLint location;
    GLint64EXT x;
    GLint64EXT y;
};

struct glUniform2i64vARB_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLint64 *value;
};

struct glUniform2i64vNV_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLint64EXT *value;
};

struct glUniform2iARB_params
{
    TEB *teb;
    GLint location;
    GLint v0;
    GLint v1;
};

struct glUniform2iv_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLint *value;
};

struct glUniform2ivARB_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLint *value;
};

struct glUniform2ui_params
{
    TEB *teb;
    GLint location;
    GLuint v0;
    GLuint v1;
};

struct glUniform2ui64ARB_params
{
    TEB *teb;
    GLint location;
    GLuint64 x;
    GLuint64 y;
};

struct glUniform2ui64NV_params
{
    TEB *teb;
    GLint location;
    GLuint64EXT x;
    GLuint64EXT y;
};

struct glUniform2ui64vARB_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLuint64 *value;
};

struct glUniform2ui64vNV_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLuint64EXT *value;
};

struct glUniform2uiEXT_params
{
    TEB *teb;
    GLint location;
    GLuint v0;
    GLuint v1;
};

struct glUniform2uiv_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLuint *value;
};

struct glUniform2uivEXT_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLuint *value;
};

struct glUniform3d_params
{
    TEB *teb;
    GLint location;
    GLdouble x;
    GLdouble y;
    GLdouble z;
};

struct glUniform3dv_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLdouble *value;
};

struct glUniform3f_params
{
    TEB *teb;
    GLint location;
    GLfloat v0;
    GLfloat v1;
    GLfloat v2;
};

struct glUniform3fARB_params
{
    TEB *teb;
    GLint location;
    GLfloat v0;
    GLfloat v1;
    GLfloat v2;
};

struct glUniform3fv_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLfloat *value;
};

struct glUniform3fvARB_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLfloat *value;
};

struct glUniform3i_params
{
    TEB *teb;
    GLint location;
    GLint v0;
    GLint v1;
    GLint v2;
};

struct glUniform3i64ARB_params
{
    TEB *teb;
    GLint location;
    GLint64 x;
    GLint64 y;
    GLint64 z;
};

struct glUniform3i64NV_params
{
    TEB *teb;
    GLint location;
    GLint64EXT x;
    GLint64EXT y;
    GLint64EXT z;
};

struct glUniform3i64vARB_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLint64 *value;
};

struct glUniform3i64vNV_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLint64EXT *value;
};

struct glUniform3iARB_params
{
    TEB *teb;
    GLint location;
    GLint v0;
    GLint v1;
    GLint v2;
};

struct glUniform3iv_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLint *value;
};

struct glUniform3ivARB_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLint *value;
};

struct glUniform3ui_params
{
    TEB *teb;
    GLint location;
    GLuint v0;
    GLuint v1;
    GLuint v2;
};

struct glUniform3ui64ARB_params
{
    TEB *teb;
    GLint location;
    GLuint64 x;
    GLuint64 y;
    GLuint64 z;
};

struct glUniform3ui64NV_params
{
    TEB *teb;
    GLint location;
    GLuint64EXT x;
    GLuint64EXT y;
    GLuint64EXT z;
};

struct glUniform3ui64vARB_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLuint64 *value;
};

struct glUniform3ui64vNV_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLuint64EXT *value;
};

struct glUniform3uiEXT_params
{
    TEB *teb;
    GLint location;
    GLuint v0;
    GLuint v1;
    GLuint v2;
};

struct glUniform3uiv_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLuint *value;
};

struct glUniform3uivEXT_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLuint *value;
};

struct glUniform4d_params
{
    TEB *teb;
    GLint location;
    GLdouble x;
    GLdouble y;
    GLdouble z;
    GLdouble w;
};

struct glUniform4dv_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLdouble *value;
};

struct glUniform4f_params
{
    TEB *teb;
    GLint location;
    GLfloat v0;
    GLfloat v1;
    GLfloat v2;
    GLfloat v3;
};

struct glUniform4fARB_params
{
    TEB *teb;
    GLint location;
    GLfloat v0;
    GLfloat v1;
    GLfloat v2;
    GLfloat v3;
};

struct glUniform4fv_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLfloat *value;
};

struct glUniform4fvARB_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLfloat *value;
};

struct glUniform4i_params
{
    TEB *teb;
    GLint location;
    GLint v0;
    GLint v1;
    GLint v2;
    GLint v3;
};

struct glUniform4i64ARB_params
{
    TEB *teb;
    GLint location;
    GLint64 x;
    GLint64 y;
    GLint64 z;
    GLint64 w;
};

struct glUniform4i64NV_params
{
    TEB *teb;
    GLint location;
    GLint64EXT x;
    GLint64EXT y;
    GLint64EXT z;
    GLint64EXT w;
};

struct glUniform4i64vARB_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLint64 *value;
};

struct glUniform4i64vNV_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLint64EXT *value;
};

struct glUniform4iARB_params
{
    TEB *teb;
    GLint location;
    GLint v0;
    GLint v1;
    GLint v2;
    GLint v3;
};

struct glUniform4iv_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLint *value;
};

struct glUniform4ivARB_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLint *value;
};

struct glUniform4ui_params
{
    TEB *teb;
    GLint location;
    GLuint v0;
    GLuint v1;
    GLuint v2;
    GLuint v3;
};

struct glUniform4ui64ARB_params
{
    TEB *teb;
    GLint location;
    GLuint64 x;
    GLuint64 y;
    GLuint64 z;
    GLuint64 w;
};

struct glUniform4ui64NV_params
{
    TEB *teb;
    GLint location;
    GLuint64EXT x;
    GLuint64EXT y;
    GLuint64EXT z;
    GLuint64EXT w;
};

struct glUniform4ui64vARB_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLuint64 *value;
};

struct glUniform4ui64vNV_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLuint64EXT *value;
};

struct glUniform4uiEXT_params
{
    TEB *teb;
    GLint location;
    GLuint v0;
    GLuint v1;
    GLuint v2;
    GLuint v3;
};

struct glUniform4uiv_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLuint *value;
};

struct glUniform4uivEXT_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLuint *value;
};

struct glUniformBlockBinding_params
{
    TEB *teb;
    GLuint program;
    GLuint uniformBlockIndex;
    GLuint uniformBlockBinding;
};

struct glUniformBufferEXT_params
{
    TEB *teb;
    GLuint program;
    GLint location;
    GLuint buffer;
};

struct glUniformHandleui64ARB_params
{
    TEB *teb;
    GLint location;
    GLuint64 value;
};

struct glUniformHandleui64NV_params
{
    TEB *teb;
    GLint location;
    GLuint64 value;
};

struct glUniformHandleui64vARB_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLuint64 *value;
};

struct glUniformHandleui64vNV_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLuint64 *value;
};

struct glUniformMatrix2dv_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLdouble *value;
};

struct glUniformMatrix2fv_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLfloat *value;
};

struct glUniformMatrix2fvARB_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLfloat *value;
};

struct glUniformMatrix2x3dv_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLdouble *value;
};

struct glUniformMatrix2x3fv_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLfloat *value;
};

struct glUniformMatrix2x4dv_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLdouble *value;
};

struct glUniformMatrix2x4fv_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLfloat *value;
};

struct glUniformMatrix3dv_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLdouble *value;
};

struct glUniformMatrix3fv_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLfloat *value;
};

struct glUniformMatrix3fvARB_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLfloat *value;
};

struct glUniformMatrix3x2dv_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLdouble *value;
};

struct glUniformMatrix3x2fv_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLfloat *value;
};

struct glUniformMatrix3x4dv_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLdouble *value;
};

struct glUniformMatrix3x4fv_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLfloat *value;
};

struct glUniformMatrix4dv_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLdouble *value;
};

struct glUniformMatrix4fv_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLfloat *value;
};

struct glUniformMatrix4fvARB_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLfloat *value;
};

struct glUniformMatrix4x2dv_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLdouble *value;
};

struct glUniformMatrix4x2fv_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLfloat *value;
};

struct glUniformMatrix4x3dv_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLdouble *value;
};

struct glUniformMatrix4x3fv_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLfloat *value;
};

struct glUniformSubroutinesuiv_params
{
    TEB *teb;
    GLenum shadertype;
    GLsizei count;
    const GLuint *indices;
};

struct glUniformui64NV_params
{
    TEB *teb;
    GLint location;
    GLuint64EXT value;
};

struct glUniformui64vNV_params
{
    TEB *teb;
    GLint location;
    GLsizei count;
    const GLuint64EXT *value;
};

struct glUnlockArraysEXT_params
{
    TEB *teb;
};

struct glUnmapBuffer_params
{
    TEB *teb;
    GLenum target;
    GLboolean ret;
#ifndef _WIN64
    void *client_ptr;
#endif
};

struct glUnmapBufferARB_params
{
    TEB *teb;
    GLenum target;
    GLboolean ret;
#ifndef _WIN64
    void *client_ptr;
#endif
};

struct glUnmapNamedBuffer_params
{
    TEB *teb;
    GLuint buffer;
    GLboolean ret;
};

struct glUnmapNamedBufferEXT_params
{
    TEB *teb;
    GLuint buffer;
    GLboolean ret;
};

struct glUnmapObjectBufferATI_params
{
    TEB *teb;
    GLuint buffer;
};

struct glUnmapTexture2DINTEL_params
{
    TEB *teb;
    GLuint texture;
    GLint level;
};

struct glUpdateObjectBufferATI_params
{
    TEB *teb;
    GLuint buffer;
    GLuint offset;
    GLsizei size;
    const void *pointer;
    GLenum preserve;
};

struct glUploadGpuMaskNVX_params
{
    TEB *teb;
    GLbitfield mask;
};

struct glUseProgram_params
{
    TEB *teb;
    GLuint program;
};

struct glUseProgramObjectARB_params
{
    TEB *teb;
    GLhandleARB programObj;
};

struct glUseProgramStages_params
{
    TEB *teb;
    GLuint pipeline;
    GLbitfield stages;
    GLuint program;
};

struct glUseShaderProgramEXT_params
{
    TEB *teb;
    GLenum type;
    GLuint program;
};

struct glVDPAUFiniNV_params
{
    TEB *teb;
};

struct glVDPAUGetSurfaceivNV_params
{
    TEB *teb;
    GLvdpauSurfaceNV surface;
    GLenum pname;
    GLsizei count;
    GLsizei *length;
    GLint *values;
};

struct glVDPAUInitNV_params
{
    TEB *teb;
    const void *vdpDevice;
    const void *getProcAddress;
};

struct glVDPAUIsSurfaceNV_params
{
    TEB *teb;
    GLvdpauSurfaceNV surface;
    GLboolean ret;
};

struct glVDPAUMapSurfacesNV_params
{
    TEB *teb;
    GLsizei numSurfaces;
    const GLvdpauSurfaceNV *surfaces;
};

struct glVDPAURegisterOutputSurfaceNV_params
{
    TEB *teb;
    const void *vdpSurface;
    GLenum target;
    GLsizei numTextureNames;
    const GLuint *textureNames;
    GLvdpauSurfaceNV ret;
};

struct glVDPAURegisterVideoSurfaceNV_params
{
    TEB *teb;
    const void *vdpSurface;
    GLenum target;
    GLsizei numTextureNames;
    const GLuint *textureNames;
    GLvdpauSurfaceNV ret;
};

struct glVDPAURegisterVideoSurfaceWithPictureStructureNV_params
{
    TEB *teb;
    const void *vdpSurface;
    GLenum target;
    GLsizei numTextureNames;
    const GLuint *textureNames;
    GLboolean isFrameStructure;
    GLvdpauSurfaceNV ret;
};

struct glVDPAUSurfaceAccessNV_params
{
    TEB *teb;
    GLvdpauSurfaceNV surface;
    GLenum access;
};

struct glVDPAUUnmapSurfacesNV_params
{
    TEB *teb;
    GLsizei numSurface;
    const GLvdpauSurfaceNV *surfaces;
};

struct glVDPAUUnregisterSurfaceNV_params
{
    TEB *teb;
    GLvdpauSurfaceNV surface;
};

struct glValidateProgram_params
{
    TEB *teb;
    GLuint program;
};

struct glValidateProgramARB_params
{
    TEB *teb;
    GLhandleARB programObj;
};

struct glValidateProgramPipeline_params
{
    TEB *teb;
    GLuint pipeline;
};

struct glVariantArrayObjectATI_params
{
    TEB *teb;
    GLuint id;
    GLenum type;
    GLsizei stride;
    GLuint buffer;
    GLuint offset;
};

struct glVariantPointerEXT_params
{
    TEB *teb;
    GLuint id;
    GLenum type;
    GLuint stride;
    const void *addr;
};

struct glVariantbvEXT_params
{
    TEB *teb;
    GLuint id;
    const GLbyte *addr;
};

struct glVariantdvEXT_params
{
    TEB *teb;
    GLuint id;
    const GLdouble *addr;
};

struct glVariantfvEXT_params
{
    TEB *teb;
    GLuint id;
    const GLfloat *addr;
};

struct glVariantivEXT_params
{
    TEB *teb;
    GLuint id;
    const GLint *addr;
};

struct glVariantsvEXT_params
{
    TEB *teb;
    GLuint id;
    const GLshort *addr;
};

struct glVariantubvEXT_params
{
    TEB *teb;
    GLuint id;
    const GLubyte *addr;
};

struct glVariantuivEXT_params
{
    TEB *teb;
    GLuint id;
    const GLuint *addr;
};

struct glVariantusvEXT_params
{
    TEB *teb;
    GLuint id;
    const GLushort *addr;
};

struct glVertex2bOES_params
{
    TEB *teb;
    GLbyte x;
    GLbyte y;
};

struct glVertex2bvOES_params
{
    TEB *teb;
    const GLbyte *coords;
};

struct glVertex2hNV_params
{
    TEB *teb;
    GLhalfNV x;
    GLhalfNV y;
};

struct glVertex2hvNV_params
{
    TEB *teb;
    const GLhalfNV *v;
};

struct glVertex2xOES_params
{
    TEB *teb;
    GLfixed x;
};

struct glVertex2xvOES_params
{
    TEB *teb;
    const GLfixed *coords;
};

struct glVertex3bOES_params
{
    TEB *teb;
    GLbyte x;
    GLbyte y;
    GLbyte z;
};

struct glVertex3bvOES_params
{
    TEB *teb;
    const GLbyte *coords;
};

struct glVertex3hNV_params
{
    TEB *teb;
    GLhalfNV x;
    GLhalfNV y;
    GLhalfNV z;
};

struct glVertex3hvNV_params
{
    TEB *teb;
    const GLhalfNV *v;
};

struct glVertex3xOES_params
{
    TEB *teb;
    GLfixed x;
    GLfixed y;
};

struct glVertex3xvOES_params
{
    TEB *teb;
    const GLfixed *coords;
};

struct glVertex4bOES_params
{
    TEB *teb;
    GLbyte x;
    GLbyte y;
    GLbyte z;
    GLbyte w;
};

struct glVertex4bvOES_params
{
    TEB *teb;
    const GLbyte *coords;
};

struct glVertex4hNV_params
{
    TEB *teb;
    GLhalfNV x;
    GLhalfNV y;
    GLhalfNV z;
    GLhalfNV w;
};

struct glVertex4hvNV_params
{
    TEB *teb;
    const GLhalfNV *v;
};

struct glVertex4xOES_params
{
    TEB *teb;
    GLfixed x;
    GLfixed y;
    GLfixed z;
};

struct glVertex4xvOES_params
{
    TEB *teb;
    const GLfixed *coords;
};

struct glVertexArrayAttribBinding_params
{
    TEB *teb;
    GLuint vaobj;
    GLuint attribindex;
    GLuint bindingindex;
};

struct glVertexArrayAttribFormat_params
{
    TEB *teb;
    GLuint vaobj;
    GLuint attribindex;
    GLint size;
    GLenum type;
    GLboolean normalized;
    GLuint relativeoffset;
};

struct glVertexArrayAttribIFormat_params
{
    TEB *teb;
    GLuint vaobj;
    GLuint attribindex;
    GLint size;
    GLenum type;
    GLuint relativeoffset;
};

struct glVertexArrayAttribLFormat_params
{
    TEB *teb;
    GLuint vaobj;
    GLuint attribindex;
    GLint size;
    GLenum type;
    GLuint relativeoffset;
};

struct glVertexArrayBindVertexBufferEXT_params
{
    TEB *teb;
    GLuint vaobj;
    GLuint bindingindex;
    GLuint buffer;
    GLintptr offset;
    GLsizei stride;
};

struct glVertexArrayBindingDivisor_params
{
    TEB *teb;
    GLuint vaobj;
    GLuint bindingindex;
    GLuint divisor;
};

struct glVertexArrayColorOffsetEXT_params
{
    TEB *teb;
    GLuint vaobj;
    GLuint buffer;
    GLint size;
    GLenum type;
    GLsizei stride;
    GLintptr offset;
};

struct glVertexArrayEdgeFlagOffsetEXT_params
{
    TEB *teb;
    GLuint vaobj;
    GLuint buffer;
    GLsizei stride;
    GLintptr offset;
};

struct glVertexArrayElementBuffer_params
{
    TEB *teb;
    GLuint vaobj;
    GLuint buffer;
};

struct glVertexArrayFogCoordOffsetEXT_params
{
    TEB *teb;
    GLuint vaobj;
    GLuint buffer;
    GLenum type;
    GLsizei stride;
    GLintptr offset;
};

struct glVertexArrayIndexOffsetEXT_params
{
    TEB *teb;
    GLuint vaobj;
    GLuint buffer;
    GLenum type;
    GLsizei stride;
    GLintptr offset;
};

struct glVertexArrayMultiTexCoordOffsetEXT_params
{
    TEB *teb;
    GLuint vaobj;
    GLuint buffer;
    GLenum texunit;
    GLint size;
    GLenum type;
    GLsizei stride;
    GLintptr offset;
};

struct glVertexArrayNormalOffsetEXT_params
{
    TEB *teb;
    GLuint vaobj;
    GLuint buffer;
    GLenum type;
    GLsizei stride;
    GLintptr offset;
};

struct glVertexArrayParameteriAPPLE_params
{
    TEB *teb;
    GLenum pname;
    GLint param;
};

struct glVertexArrayRangeAPPLE_params
{
    TEB *teb;
    GLsizei length;
    void *pointer;
};

struct glVertexArrayRangeNV_params
{
    TEB *teb;
    GLsizei length;
    const void *pointer;
};

struct glVertexArraySecondaryColorOffsetEXT_params
{
    TEB *teb;
    GLuint vaobj;
    GLuint buffer;
    GLint size;
    GLenum type;
    GLsizei stride;
    GLintptr offset;
};

struct glVertexArrayTexCoordOffsetEXT_params
{
    TEB *teb;
    GLuint vaobj;
    GLuint buffer;
    GLint size;
    GLenum type;
    GLsizei stride;
    GLintptr offset;
};

struct glVertexArrayVertexAttribBindingEXT_params
{
    TEB *teb;
    GLuint vaobj;
    GLuint attribindex;
    GLuint bindingindex;
};

struct glVertexArrayVertexAttribDivisorEXT_params
{
    TEB *teb;
    GLuint vaobj;
    GLuint index;
    GLuint divisor;
};

struct glVertexArrayVertexAttribFormatEXT_params
{
    TEB *teb;
    GLuint vaobj;
    GLuint attribindex;
    GLint size;
    GLenum type;
    GLboolean normalized;
    GLuint relativeoffset;
};

struct glVertexArrayVertexAttribIFormatEXT_params
{
    TEB *teb;
    GLuint vaobj;
    GLuint attribindex;
    GLint size;
    GLenum type;
    GLuint relativeoffset;
};

struct glVertexArrayVertexAttribIOffsetEXT_params
{
    TEB *teb;
    GLuint vaobj;
    GLuint buffer;
    GLuint index;
    GLint size;
    GLenum type;
    GLsizei stride;
    GLintptr offset;
};

struct glVertexArrayVertexAttribLFormatEXT_params
{
    TEB *teb;
    GLuint vaobj;
    GLuint attribindex;
    GLint size;
    GLenum type;
    GLuint relativeoffset;
};

struct glVertexArrayVertexAttribLOffsetEXT_params
{
    TEB *teb;
    GLuint vaobj;
    GLuint buffer;
    GLuint index;
    GLint size;
    GLenum type;
    GLsizei stride;
    GLintptr offset;
};

struct glVertexArrayVertexAttribOffsetEXT_params
{
    TEB *teb;
    GLuint vaobj;
    GLuint buffer;
    GLuint index;
    GLint size;
    GLenum type;
    GLboolean normalized;
    GLsizei stride;
    GLintptr offset;
};

struct glVertexArrayVertexBindingDivisorEXT_params
{
    TEB *teb;
    GLuint vaobj;
    GLuint bindingindex;
    GLuint divisor;
};

struct glVertexArrayVertexBuffer_params
{
    TEB *teb;
    GLuint vaobj;
    GLuint bindingindex;
    GLuint buffer;
    GLintptr offset;
    GLsizei stride;
};

struct glVertexArrayVertexBuffers_params
{
    TEB *teb;
    GLuint vaobj;
    GLuint first;
    GLsizei count;
    const GLuint *buffers;
    const GLintptr *offsets;
    const GLsizei *strides;
};

struct glVertexArrayVertexOffsetEXT_params
{
    TEB *teb;
    GLuint vaobj;
    GLuint buffer;
    GLint size;
    GLenum type;
    GLsizei stride;
    GLintptr offset;
};

struct glVertexAttrib1d_params
{
    TEB *teb;
    GLuint index;
    GLdouble x;
};

struct glVertexAttrib1dARB_params
{
    TEB *teb;
    GLuint index;
    GLdouble x;
};

struct glVertexAttrib1dNV_params
{
    TEB *teb;
    GLuint index;
    GLdouble x;
};

struct glVertexAttrib1dv_params
{
    TEB *teb;
    GLuint index;
    const GLdouble *v;
};

struct glVertexAttrib1dvARB_params
{
    TEB *teb;
    GLuint index;
    const GLdouble *v;
};

struct glVertexAttrib1dvNV_params
{
    TEB *teb;
    GLuint index;
    const GLdouble *v;
};

struct glVertexAttrib1f_params
{
    TEB *teb;
    GLuint index;
    GLfloat x;
};

struct glVertexAttrib1fARB_params
{
    TEB *teb;
    GLuint index;
    GLfloat x;
};

struct glVertexAttrib1fNV_params
{
    TEB *teb;
    GLuint index;
    GLfloat x;
};

struct glVertexAttrib1fv_params
{
    TEB *teb;
    GLuint index;
    const GLfloat *v;
};

struct glVertexAttrib1fvARB_params
{
    TEB *teb;
    GLuint index;
    const GLfloat *v;
};

struct glVertexAttrib1fvNV_params
{
    TEB *teb;
    GLuint index;
    const GLfloat *v;
};

struct glVertexAttrib1hNV_params
{
    TEB *teb;
    GLuint index;
    GLhalfNV x;
};

struct glVertexAttrib1hvNV_params
{
    TEB *teb;
    GLuint index;
    const GLhalfNV *v;
};

struct glVertexAttrib1s_params
{
    TEB *teb;
    GLuint index;
    GLshort x;
};

struct glVertexAttrib1sARB_params
{
    TEB *teb;
    GLuint index;
    GLshort x;
};

struct glVertexAttrib1sNV_params
{
    TEB *teb;
    GLuint index;
    GLshort x;
};

struct glVertexAttrib1sv_params
{
    TEB *teb;
    GLuint index;
    const GLshort *v;
};

struct glVertexAttrib1svARB_params
{
    TEB *teb;
    GLuint index;
    const GLshort *v;
};

struct glVertexAttrib1svNV_params
{
    TEB *teb;
    GLuint index;
    const GLshort *v;
};

struct glVertexAttrib2d_params
{
    TEB *teb;
    GLuint index;
    GLdouble x;
    GLdouble y;
};

struct glVertexAttrib2dARB_params
{
    TEB *teb;
    GLuint index;
    GLdouble x;
    GLdouble y;
};

struct glVertexAttrib2dNV_params
{
    TEB *teb;
    GLuint index;
    GLdouble x;
    GLdouble y;
};

struct glVertexAttrib2dv_params
{
    TEB *teb;
    GLuint index;
    const GLdouble *v;
};

struct glVertexAttrib2dvARB_params
{
    TEB *teb;
    GLuint index;
    const GLdouble *v;
};

struct glVertexAttrib2dvNV_params
{
    TEB *teb;
    GLuint index;
    const GLdouble *v;
};

struct glVertexAttrib2f_params
{
    TEB *teb;
    GLuint index;
    GLfloat x;
    GLfloat y;
};

struct glVertexAttrib2fARB_params
{
    TEB *teb;
    GLuint index;
    GLfloat x;
    GLfloat y;
};

struct glVertexAttrib2fNV_params
{
    TEB *teb;
    GLuint index;
    GLfloat x;
    GLfloat y;
};

struct glVertexAttrib2fv_params
{
    TEB *teb;
    GLuint index;
    const GLfloat *v;
};

struct glVertexAttrib2fvARB_params
{
    TEB *teb;
    GLuint index;
    const GLfloat *v;
};

struct glVertexAttrib2fvNV_params
{
    TEB *teb;
    GLuint index;
    const GLfloat *v;
};

struct glVertexAttrib2hNV_params
{
    TEB *teb;
    GLuint index;
    GLhalfNV x;
    GLhalfNV y;
};

struct glVertexAttrib2hvNV_params
{
    TEB *teb;
    GLuint index;
    const GLhalfNV *v;
};

struct glVertexAttrib2s_params
{
    TEB *teb;
    GLuint index;
    GLshort x;
    GLshort y;
};

struct glVertexAttrib2sARB_params
{
    TEB *teb;
    GLuint index;
    GLshort x;
    GLshort y;
};

struct glVertexAttrib2sNV_params
{
    TEB *teb;
    GLuint index;
    GLshort x;
    GLshort y;
};

struct glVertexAttrib2sv_params
{
    TEB *teb;
    GLuint index;
    const GLshort *v;
};

struct glVertexAttrib2svARB_params
{
    TEB *teb;
    GLuint index;
    const GLshort *v;
};

struct glVertexAttrib2svNV_params
{
    TEB *teb;
    GLuint index;
    const GLshort *v;
};

struct glVertexAttrib3d_params
{
    TEB *teb;
    GLuint index;
    GLdouble x;
    GLdouble y;
    GLdouble z;
};

struct glVertexAttrib3dARB_params
{
    TEB *teb;
    GLuint index;
    GLdouble x;
    GLdouble y;
    GLdouble z;
};

struct glVertexAttrib3dNV_params
{
    TEB *teb;
    GLuint index;
    GLdouble x;
    GLdouble y;
    GLdouble z;
};

struct glVertexAttrib3dv_params
{
    TEB *teb;
    GLuint index;
    const GLdouble *v;
};

struct glVertexAttrib3dvARB_params
{
    TEB *teb;
    GLuint index;
    const GLdouble *v;
};

struct glVertexAttrib3dvNV_params
{
    TEB *teb;
    GLuint index;
    const GLdouble *v;
};

struct glVertexAttrib3f_params
{
    TEB *teb;
    GLuint index;
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

struct glVertexAttrib3fARB_params
{
    TEB *teb;
    GLuint index;
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

struct glVertexAttrib3fNV_params
{
    TEB *teb;
    GLuint index;
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

struct glVertexAttrib3fv_params
{
    TEB *teb;
    GLuint index;
    const GLfloat *v;
};

struct glVertexAttrib3fvARB_params
{
    TEB *teb;
    GLuint index;
    const GLfloat *v;
};

struct glVertexAttrib3fvNV_params
{
    TEB *teb;
    GLuint index;
    const GLfloat *v;
};

struct glVertexAttrib3hNV_params
{
    TEB *teb;
    GLuint index;
    GLhalfNV x;
    GLhalfNV y;
    GLhalfNV z;
};

struct glVertexAttrib3hvNV_params
{
    TEB *teb;
    GLuint index;
    const GLhalfNV *v;
};

struct glVertexAttrib3s_params
{
    TEB *teb;
    GLuint index;
    GLshort x;
    GLshort y;
    GLshort z;
};

struct glVertexAttrib3sARB_params
{
    TEB *teb;
    GLuint index;
    GLshort x;
    GLshort y;
    GLshort z;
};

struct glVertexAttrib3sNV_params
{
    TEB *teb;
    GLuint index;
    GLshort x;
    GLshort y;
    GLshort z;
};

struct glVertexAttrib3sv_params
{
    TEB *teb;
    GLuint index;
    const GLshort *v;
};

struct glVertexAttrib3svARB_params
{
    TEB *teb;
    GLuint index;
    const GLshort *v;
};

struct glVertexAttrib3svNV_params
{
    TEB *teb;
    GLuint index;
    const GLshort *v;
};

struct glVertexAttrib4Nbv_params
{
    TEB *teb;
    GLuint index;
    const GLbyte *v;
};

struct glVertexAttrib4NbvARB_params
{
    TEB *teb;
    GLuint index;
    const GLbyte *v;
};

struct glVertexAttrib4Niv_params
{
    TEB *teb;
    GLuint index;
    const GLint *v;
};

struct glVertexAttrib4NivARB_params
{
    TEB *teb;
    GLuint index;
    const GLint *v;
};

struct glVertexAttrib4Nsv_params
{
    TEB *teb;
    GLuint index;
    const GLshort *v;
};

struct glVertexAttrib4NsvARB_params
{
    TEB *teb;
    GLuint index;
    const GLshort *v;
};

struct glVertexAttrib4Nub_params
{
    TEB *teb;
    GLuint index;
    GLubyte x;
    GLubyte y;
    GLubyte z;
    GLubyte w;
};

struct glVertexAttrib4NubARB_params
{
    TEB *teb;
    GLuint index;
    GLubyte x;
    GLubyte y;
    GLubyte z;
    GLubyte w;
};

struct glVertexAttrib4Nubv_params
{
    TEB *teb;
    GLuint index;
    const GLubyte *v;
};

struct glVertexAttrib4NubvARB_params
{
    TEB *teb;
    GLuint index;
    const GLubyte *v;
};

struct glVertexAttrib4Nuiv_params
{
    TEB *teb;
    GLuint index;
    const GLuint *v;
};

struct glVertexAttrib4NuivARB_params
{
    TEB *teb;
    GLuint index;
    const GLuint *v;
};

struct glVertexAttrib4Nusv_params
{
    TEB *teb;
    GLuint index;
    const GLushort *v;
};

struct glVertexAttrib4NusvARB_params
{
    TEB *teb;
    GLuint index;
    const GLushort *v;
};

struct glVertexAttrib4bv_params
{
    TEB *teb;
    GLuint index;
    const GLbyte *v;
};

struct glVertexAttrib4bvARB_params
{
    TEB *teb;
    GLuint index;
    const GLbyte *v;
};

struct glVertexAttrib4d_params
{
    TEB *teb;
    GLuint index;
    GLdouble x;
    GLdouble y;
    GLdouble z;
    GLdouble w;
};

struct glVertexAttrib4dARB_params
{
    TEB *teb;
    GLuint index;
    GLdouble x;
    GLdouble y;
    GLdouble z;
    GLdouble w;
};

struct glVertexAttrib4dNV_params
{
    TEB *teb;
    GLuint index;
    GLdouble x;
    GLdouble y;
    GLdouble z;
    GLdouble w;
};

struct glVertexAttrib4dv_params
{
    TEB *teb;
    GLuint index;
    const GLdouble *v;
};

struct glVertexAttrib4dvARB_params
{
    TEB *teb;
    GLuint index;
    const GLdouble *v;
};

struct glVertexAttrib4dvNV_params
{
    TEB *teb;
    GLuint index;
    const GLdouble *v;
};

struct glVertexAttrib4f_params
{
    TEB *teb;
    GLuint index;
    GLfloat x;
    GLfloat y;
    GLfloat z;
    GLfloat w;
};

struct glVertexAttrib4fARB_params
{
    TEB *teb;
    GLuint index;
    GLfloat x;
    GLfloat y;
    GLfloat z;
    GLfloat w;
};

struct glVertexAttrib4fNV_params
{
    TEB *teb;
    GLuint index;
    GLfloat x;
    GLfloat y;
    GLfloat z;
    GLfloat w;
};

struct glVertexAttrib4fv_params
{
    TEB *teb;
    GLuint index;
    const GLfloat *v;
};

struct glVertexAttrib4fvARB_params
{
    TEB *teb;
    GLuint index;
    const GLfloat *v;
};

struct glVertexAttrib4fvNV_params
{
    TEB *teb;
    GLuint index;
    const GLfloat *v;
};

struct glVertexAttrib4hNV_params
{
    TEB *teb;
    GLuint index;
    GLhalfNV x;
    GLhalfNV y;
    GLhalfNV z;
    GLhalfNV w;
};

struct glVertexAttrib4hvNV_params
{
    TEB *teb;
    GLuint index;
    const GLhalfNV *v;
};

struct glVertexAttrib4iv_params
{
    TEB *teb;
    GLuint index;
    const GLint *v;
};

struct glVertexAttrib4ivARB_params
{
    TEB *teb;
    GLuint index;
    const GLint *v;
};

struct glVertexAttrib4s_params
{
    TEB *teb;
    GLuint index;
    GLshort x;
    GLshort y;
    GLshort z;
    GLshort w;
};

struct glVertexAttrib4sARB_params
{
    TEB *teb;
    GLuint index;
    GLshort x;
    GLshort y;
    GLshort z;
    GLshort w;
};

struct glVertexAttrib4sNV_params
{
    TEB *teb;
    GLuint index;
    GLshort x;
    GLshort y;
    GLshort z;
    GLshort w;
};

struct glVertexAttrib4sv_params
{
    TEB *teb;
    GLuint index;
    const GLshort *v;
};

struct glVertexAttrib4svARB_params
{
    TEB *teb;
    GLuint index;
    const GLshort *v;
};

struct glVertexAttrib4svNV_params
{
    TEB *teb;
    GLuint index;
    const GLshort *v;
};

struct glVertexAttrib4ubNV_params
{
    TEB *teb;
    GLuint index;
    GLubyte x;
    GLubyte y;
    GLubyte z;
    GLubyte w;
};

struct glVertexAttrib4ubv_params
{
    TEB *teb;
    GLuint index;
    const GLubyte *v;
};

struct glVertexAttrib4ubvARB_params
{
    TEB *teb;
    GLuint index;
    const GLubyte *v;
};

struct glVertexAttrib4ubvNV_params
{
    TEB *teb;
    GLuint index;
    const GLubyte *v;
};

struct glVertexAttrib4uiv_params
{
    TEB *teb;
    GLuint index;
    const GLuint *v;
};

struct glVertexAttrib4uivARB_params
{
    TEB *teb;
    GLuint index;
    const GLuint *v;
};

struct glVertexAttrib4usv_params
{
    TEB *teb;
    GLuint index;
    const GLushort *v;
};

struct glVertexAttrib4usvARB_params
{
    TEB *teb;
    GLuint index;
    const GLushort *v;
};

struct glVertexAttribArrayObjectATI_params
{
    TEB *teb;
    GLuint index;
    GLint size;
    GLenum type;
    GLboolean normalized;
    GLsizei stride;
    GLuint buffer;
    GLuint offset;
};

struct glVertexAttribBinding_params
{
    TEB *teb;
    GLuint attribindex;
    GLuint bindingindex;
};

struct glVertexAttribDivisor_params
{
    TEB *teb;
    GLuint index;
    GLuint divisor;
};

struct glVertexAttribDivisorARB_params
{
    TEB *teb;
    GLuint index;
    GLuint divisor;
};

struct glVertexAttribFormat_params
{
    TEB *teb;
    GLuint attribindex;
    GLint size;
    GLenum type;
    GLboolean normalized;
    GLuint relativeoffset;
};

struct glVertexAttribFormatNV_params
{
    TEB *teb;
    GLuint index;
    GLint size;
    GLenum type;
    GLboolean normalized;
    GLsizei stride;
};

struct glVertexAttribI1i_params
{
    TEB *teb;
    GLuint index;
    GLint x;
};

struct glVertexAttribI1iEXT_params
{
    TEB *teb;
    GLuint index;
    GLint x;
};

struct glVertexAttribI1iv_params
{
    TEB *teb;
    GLuint index;
    const GLint *v;
};

struct glVertexAttribI1ivEXT_params
{
    TEB *teb;
    GLuint index;
    const GLint *v;
};

struct glVertexAttribI1ui_params
{
    TEB *teb;
    GLuint index;
    GLuint x;
};

struct glVertexAttribI1uiEXT_params
{
    TEB *teb;
    GLuint index;
    GLuint x;
};

struct glVertexAttribI1uiv_params
{
    TEB *teb;
    GLuint index;
    const GLuint *v;
};

struct glVertexAttribI1uivEXT_params
{
    TEB *teb;
    GLuint index;
    const GLuint *v;
};

struct glVertexAttribI2i_params
{
    TEB *teb;
    GLuint index;
    GLint x;
    GLint y;
};

struct glVertexAttribI2iEXT_params
{
    TEB *teb;
    GLuint index;
    GLint x;
    GLint y;
};

struct glVertexAttribI2iv_params
{
    TEB *teb;
    GLuint index;
    const GLint *v;
};

struct glVertexAttribI2ivEXT_params
{
    TEB *teb;
    GLuint index;
    const GLint *v;
};

struct glVertexAttribI2ui_params
{
    TEB *teb;
    GLuint index;
    GLuint x;
    GLuint y;
};

struct glVertexAttribI2uiEXT_params
{
    TEB *teb;
    GLuint index;
    GLuint x;
    GLuint y;
};

struct glVertexAttribI2uiv_params
{
    TEB *teb;
    GLuint index;
    const GLuint *v;
};

struct glVertexAttribI2uivEXT_params
{
    TEB *teb;
    GLuint index;
    const GLuint *v;
};

struct glVertexAttribI3i_params
{
    TEB *teb;
    GLuint index;
    GLint x;
    GLint y;
    GLint z;
};

struct glVertexAttribI3iEXT_params
{
    TEB *teb;
    GLuint index;
    GLint x;
    GLint y;
    GLint z;
};

struct glVertexAttribI3iv_params
{
    TEB *teb;
    GLuint index;
    const GLint *v;
};

struct glVertexAttribI3ivEXT_params
{
    TEB *teb;
    GLuint index;
    const GLint *v;
};

struct glVertexAttribI3ui_params
{
    TEB *teb;
    GLuint index;
    GLuint x;
    GLuint y;
    GLuint z;
};

struct glVertexAttribI3uiEXT_params
{
    TEB *teb;
    GLuint index;
    GLuint x;
    GLuint y;
    GLuint z;
};

struct glVertexAttribI3uiv_params
{
    TEB *teb;
    GLuint index;
    const GLuint *v;
};

struct glVertexAttribI3uivEXT_params
{
    TEB *teb;
    GLuint index;
    const GLuint *v;
};

struct glVertexAttribI4bv_params
{
    TEB *teb;
    GLuint index;
    const GLbyte *v;
};

struct glVertexAttribI4bvEXT_params
{
    TEB *teb;
    GLuint index;
    const GLbyte *v;
};

struct glVertexAttribI4i_params
{
    TEB *teb;
    GLuint index;
    GLint x;
    GLint y;
    GLint z;
    GLint w;
};

struct glVertexAttribI4iEXT_params
{
    TEB *teb;
    GLuint index;
    GLint x;
    GLint y;
    GLint z;
    GLint w;
};

struct glVertexAttribI4iv_params
{
    TEB *teb;
    GLuint index;
    const GLint *v;
};

struct glVertexAttribI4ivEXT_params
{
    TEB *teb;
    GLuint index;
    const GLint *v;
};

struct glVertexAttribI4sv_params
{
    TEB *teb;
    GLuint index;
    const GLshort *v;
};

struct glVertexAttribI4svEXT_params
{
    TEB *teb;
    GLuint index;
    const GLshort *v;
};

struct glVertexAttribI4ubv_params
{
    TEB *teb;
    GLuint index;
    const GLubyte *v;
};

struct glVertexAttribI4ubvEXT_params
{
    TEB *teb;
    GLuint index;
    const GLubyte *v;
};

struct glVertexAttribI4ui_params
{
    TEB *teb;
    GLuint index;
    GLuint x;
    GLuint y;
    GLuint z;
    GLuint w;
};

struct glVertexAttribI4uiEXT_params
{
    TEB *teb;
    GLuint index;
    GLuint x;
    GLuint y;
    GLuint z;
    GLuint w;
};

struct glVertexAttribI4uiv_params
{
    TEB *teb;
    GLuint index;
    const GLuint *v;
};

struct glVertexAttribI4uivEXT_params
{
    TEB *teb;
    GLuint index;
    const GLuint *v;
};

struct glVertexAttribI4usv_params
{
    TEB *teb;
    GLuint index;
    const GLushort *v;
};

struct glVertexAttribI4usvEXT_params
{
    TEB *teb;
    GLuint index;
    const GLushort *v;
};

struct glVertexAttribIFormat_params
{
    TEB *teb;
    GLuint attribindex;
    GLint size;
    GLenum type;
    GLuint relativeoffset;
};

struct glVertexAttribIFormatNV_params
{
    TEB *teb;
    GLuint index;
    GLint size;
    GLenum type;
    GLsizei stride;
};

struct glVertexAttribIPointer_params
{
    TEB *teb;
    GLuint index;
    GLint size;
    GLenum type;
    GLsizei stride;
    const void *pointer;
};

struct glVertexAttribIPointerEXT_params
{
    TEB *teb;
    GLuint index;
    GLint size;
    GLenum type;
    GLsizei stride;
    const void *pointer;
};

struct glVertexAttribL1d_params
{
    TEB *teb;
    GLuint index;
    GLdouble x;
};

struct glVertexAttribL1dEXT_params
{
    TEB *teb;
    GLuint index;
    GLdouble x;
};

struct glVertexAttribL1dv_params
{
    TEB *teb;
    GLuint index;
    const GLdouble *v;
};

struct glVertexAttribL1dvEXT_params
{
    TEB *teb;
    GLuint index;
    const GLdouble *v;
};

struct glVertexAttribL1i64NV_params
{
    TEB *teb;
    GLuint index;
    GLint64EXT x;
};

struct glVertexAttribL1i64vNV_params
{
    TEB *teb;
    GLuint index;
    const GLint64EXT *v;
};

struct glVertexAttribL1ui64ARB_params
{
    TEB *teb;
    GLuint index;
    GLuint64EXT x;
};

struct glVertexAttribL1ui64NV_params
{
    TEB *teb;
    GLuint index;
    GLuint64EXT x;
};

struct glVertexAttribL1ui64vARB_params
{
    TEB *teb;
    GLuint index;
    const GLuint64EXT *v;
};

struct glVertexAttribL1ui64vNV_params
{
    TEB *teb;
    GLuint index;
    const GLuint64EXT *v;
};

struct glVertexAttribL2d_params
{
    TEB *teb;
    GLuint index;
    GLdouble x;
    GLdouble y;
};

struct glVertexAttribL2dEXT_params
{
    TEB *teb;
    GLuint index;
    GLdouble x;
    GLdouble y;
};

struct glVertexAttribL2dv_params
{
    TEB *teb;
    GLuint index;
    const GLdouble *v;
};

struct glVertexAttribL2dvEXT_params
{
    TEB *teb;
    GLuint index;
    const GLdouble *v;
};

struct glVertexAttribL2i64NV_params
{
    TEB *teb;
    GLuint index;
    GLint64EXT x;
    GLint64EXT y;
};

struct glVertexAttribL2i64vNV_params
{
    TEB *teb;
    GLuint index;
    const GLint64EXT *v;
};

struct glVertexAttribL2ui64NV_params
{
    TEB *teb;
    GLuint index;
    GLuint64EXT x;
    GLuint64EXT y;
};

struct glVertexAttribL2ui64vNV_params
{
    TEB *teb;
    GLuint index;
    const GLuint64EXT *v;
};

struct glVertexAttribL3d_params
{
    TEB *teb;
    GLuint index;
    GLdouble x;
    GLdouble y;
    GLdouble z;
};

struct glVertexAttribL3dEXT_params
{
    TEB *teb;
    GLuint index;
    GLdouble x;
    GLdouble y;
    GLdouble z;
};

struct glVertexAttribL3dv_params
{
    TEB *teb;
    GLuint index;
    const GLdouble *v;
};

struct glVertexAttribL3dvEXT_params
{
    TEB *teb;
    GLuint index;
    const GLdouble *v;
};

struct glVertexAttribL3i64NV_params
{
    TEB *teb;
    GLuint index;
    GLint64EXT x;
    GLint64EXT y;
    GLint64EXT z;
};

struct glVertexAttribL3i64vNV_params
{
    TEB *teb;
    GLuint index;
    const GLint64EXT *v;
};

struct glVertexAttribL3ui64NV_params
{
    TEB *teb;
    GLuint index;
    GLuint64EXT x;
    GLuint64EXT y;
    GLuint64EXT z;
};

struct glVertexAttribL3ui64vNV_params
{
    TEB *teb;
    GLuint index;
    const GLuint64EXT *v;
};

struct glVertexAttribL4d_params
{
    TEB *teb;
    GLuint index;
    GLdouble x;
    GLdouble y;
    GLdouble z;
    GLdouble w;
};

struct glVertexAttribL4dEXT_params
{
    TEB *teb;
    GLuint index;
    GLdouble x;
    GLdouble y;
    GLdouble z;
    GLdouble w;
};

struct glVertexAttribL4dv_params
{
    TEB *teb;
    GLuint index;
    const GLdouble *v;
};

struct glVertexAttribL4dvEXT_params
{
    TEB *teb;
    GLuint index;
    const GLdouble *v;
};

struct glVertexAttribL4i64NV_params
{
    TEB *teb;
    GLuint index;
    GLint64EXT x;
    GLint64EXT y;
    GLint64EXT z;
    GLint64EXT w;
};

struct glVertexAttribL4i64vNV_params
{
    TEB *teb;
    GLuint index;
    const GLint64EXT *v;
};

struct glVertexAttribL4ui64NV_params
{
    TEB *teb;
    GLuint index;
    GLuint64EXT x;
    GLuint64EXT y;
    GLuint64EXT z;
    GLuint64EXT w;
};

struct glVertexAttribL4ui64vNV_params
{
    TEB *teb;
    GLuint index;
    const GLuint64EXT *v;
};

struct glVertexAttribLFormat_params
{
    TEB *teb;
    GLuint attribindex;
    GLint size;
    GLenum type;
    GLuint relativeoffset;
};

struct glVertexAttribLFormatNV_params
{
    TEB *teb;
    GLuint index;
    GLint size;
    GLenum type;
    GLsizei stride;
};

struct glVertexAttribLPointer_params
{
    TEB *teb;
    GLuint index;
    GLint size;
    GLenum type;
    GLsizei stride;
    const void *pointer;
};

struct glVertexAttribLPointerEXT_params
{
    TEB *teb;
    GLuint index;
    GLint size;
    GLenum type;
    GLsizei stride;
    const void *pointer;
};

struct glVertexAttribP1ui_params
{
    TEB *teb;
    GLuint index;
    GLenum type;
    GLboolean normalized;
    GLuint value;
};

struct glVertexAttribP1uiv_params
{
    TEB *teb;
    GLuint index;
    GLenum type;
    GLboolean normalized;
    const GLuint *value;
};

struct glVertexAttribP2ui_params
{
    TEB *teb;
    GLuint index;
    GLenum type;
    GLboolean normalized;
    GLuint value;
};

struct glVertexAttribP2uiv_params
{
    TEB *teb;
    GLuint index;
    GLenum type;
    GLboolean normalized;
    const GLuint *value;
};

struct glVertexAttribP3ui_params
{
    TEB *teb;
    GLuint index;
    GLenum type;
    GLboolean normalized;
    GLuint value;
};

struct glVertexAttribP3uiv_params
{
    TEB *teb;
    GLuint index;
    GLenum type;
    GLboolean normalized;
    const GLuint *value;
};

struct glVertexAttribP4ui_params
{
    TEB *teb;
    GLuint index;
    GLenum type;
    GLboolean normalized;
    GLuint value;
};

struct glVertexAttribP4uiv_params
{
    TEB *teb;
    GLuint index;
    GLenum type;
    GLboolean normalized;
    const GLuint *value;
};

struct glVertexAttribParameteriAMD_params
{
    TEB *teb;
    GLuint index;
    GLenum pname;
    GLint param;
};

struct glVertexAttribPointer_params
{
    TEB *teb;
    GLuint index;
    GLint size;
    GLenum type;
    GLboolean normalized;
    GLsizei stride;
    const void *pointer;
};

struct glVertexAttribPointerARB_params
{
    TEB *teb;
    GLuint index;
    GLint size;
    GLenum type;
    GLboolean normalized;
    GLsizei stride;
    const void *pointer;
};

struct glVertexAttribPointerNV_params
{
    TEB *teb;
    GLuint index;
    GLint fsize;
    GLenum type;
    GLsizei stride;
    const void *pointer;
};

struct glVertexAttribs1dvNV_params
{
    TEB *teb;
    GLuint index;
    GLsizei count;
    const GLdouble *v;
};

struct glVertexAttribs1fvNV_params
{
    TEB *teb;
    GLuint index;
    GLsizei count;
    const GLfloat *v;
};

struct glVertexAttribs1hvNV_params
{
    TEB *teb;
    GLuint index;
    GLsizei n;
    const GLhalfNV *v;
};

struct glVertexAttribs1svNV_params
{
    TEB *teb;
    GLuint index;
    GLsizei count;
    const GLshort *v;
};

struct glVertexAttribs2dvNV_params
{
    TEB *teb;
    GLuint index;
    GLsizei count;
    const GLdouble *v;
};

struct glVertexAttribs2fvNV_params
{
    TEB *teb;
    GLuint index;
    GLsizei count;
    const GLfloat *v;
};

struct glVertexAttribs2hvNV_params
{
    TEB *teb;
    GLuint index;
    GLsizei n;
    const GLhalfNV *v;
};

struct glVertexAttribs2svNV_params
{
    TEB *teb;
    GLuint index;
    GLsizei count;
    const GLshort *v;
};

struct glVertexAttribs3dvNV_params
{
    TEB *teb;
    GLuint index;
    GLsizei count;
    const GLdouble *v;
};

struct glVertexAttribs3fvNV_params
{
    TEB *teb;
    GLuint index;
    GLsizei count;
    const GLfloat *v;
};

struct glVertexAttribs3hvNV_params
{
    TEB *teb;
    GLuint index;
    GLsizei n;
    const GLhalfNV *v;
};

struct glVertexAttribs3svNV_params
{
    TEB *teb;
    GLuint index;
    GLsizei count;
    const GLshort *v;
};

struct glVertexAttribs4dvNV_params
{
    TEB *teb;
    GLuint index;
    GLsizei count;
    const GLdouble *v;
};

struct glVertexAttribs4fvNV_params
{
    TEB *teb;
    GLuint index;
    GLsizei count;
    const GLfloat *v;
};

struct glVertexAttribs4hvNV_params
{
    TEB *teb;
    GLuint index;
    GLsizei n;
    const GLhalfNV *v;
};

struct glVertexAttribs4svNV_params
{
    TEB *teb;
    GLuint index;
    GLsizei count;
    const GLshort *v;
};

struct glVertexAttribs4ubvNV_params
{
    TEB *teb;
    GLuint index;
    GLsizei count;
    const GLubyte *v;
};

struct glVertexBindingDivisor_params
{
    TEB *teb;
    GLuint bindingindex;
    GLuint divisor;
};

struct glVertexBlendARB_params
{
    TEB *teb;
    GLint count;
};

struct glVertexBlendEnvfATI_params
{
    TEB *teb;
    GLenum pname;
    GLfloat param;
};

struct glVertexBlendEnviATI_params
{
    TEB *teb;
    GLenum pname;
    GLint param;
};

struct glVertexFormatNV_params
{
    TEB *teb;
    GLint size;
    GLenum type;
    GLsizei stride;
};

struct glVertexP2ui_params
{
    TEB *teb;
    GLenum type;
    GLuint value;
};

struct glVertexP2uiv_params
{
    TEB *teb;
    GLenum type;
    const GLuint *value;
};

struct glVertexP3ui_params
{
    TEB *teb;
    GLenum type;
    GLuint value;
};

struct glVertexP3uiv_params
{
    TEB *teb;
    GLenum type;
    const GLuint *value;
};

struct glVertexP4ui_params
{
    TEB *teb;
    GLenum type;
    GLuint value;
};

struct glVertexP4uiv_params
{
    TEB *teb;
    GLenum type;
    const GLuint *value;
};

struct glVertexPointerEXT_params
{
    TEB *teb;
    GLint size;
    GLenum type;
    GLsizei stride;
    GLsizei count;
    const void *pointer;
};

struct glVertexPointerListIBM_params
{
    TEB *teb;
    GLint size;
    GLenum type;
    GLint stride;
    const void **pointer;
    GLint ptrstride;
};

struct glVertexPointervINTEL_params
{
    TEB *teb;
    GLint size;
    GLenum type;
    const void **pointer;
};

struct glVertexStream1dATI_params
{
    TEB *teb;
    GLenum stream;
    GLdouble x;
};

struct glVertexStream1dvATI_params
{
    TEB *teb;
    GLenum stream;
    const GLdouble *coords;
};

struct glVertexStream1fATI_params
{
    TEB *teb;
    GLenum stream;
    GLfloat x;
};

struct glVertexStream1fvATI_params
{
    TEB *teb;
    GLenum stream;
    const GLfloat *coords;
};

struct glVertexStream1iATI_params
{
    TEB *teb;
    GLenum stream;
    GLint x;
};

struct glVertexStream1ivATI_params
{
    TEB *teb;
    GLenum stream;
    const GLint *coords;
};

struct glVertexStream1sATI_params
{
    TEB *teb;
    GLenum stream;
    GLshort x;
};

struct glVertexStream1svATI_params
{
    TEB *teb;
    GLenum stream;
    const GLshort *coords;
};

struct glVertexStream2dATI_params
{
    TEB *teb;
    GLenum stream;
    GLdouble x;
    GLdouble y;
};

struct glVertexStream2dvATI_params
{
    TEB *teb;
    GLenum stream;
    const GLdouble *coords;
};

struct glVertexStream2fATI_params
{
    TEB *teb;
    GLenum stream;
    GLfloat x;
    GLfloat y;
};

struct glVertexStream2fvATI_params
{
    TEB *teb;
    GLenum stream;
    const GLfloat *coords;
};

struct glVertexStream2iATI_params
{
    TEB *teb;
    GLenum stream;
    GLint x;
    GLint y;
};

struct glVertexStream2ivATI_params
{
    TEB *teb;
    GLenum stream;
    const GLint *coords;
};

struct glVertexStream2sATI_params
{
    TEB *teb;
    GLenum stream;
    GLshort x;
    GLshort y;
};

struct glVertexStream2svATI_params
{
    TEB *teb;
    GLenum stream;
    const GLshort *coords;
};

struct glVertexStream3dATI_params
{
    TEB *teb;
    GLenum stream;
    GLdouble x;
    GLdouble y;
    GLdouble z;
};

struct glVertexStream3dvATI_params
{
    TEB *teb;
    GLenum stream;
    const GLdouble *coords;
};

struct glVertexStream3fATI_params
{
    TEB *teb;
    GLenum stream;
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

struct glVertexStream3fvATI_params
{
    TEB *teb;
    GLenum stream;
    const GLfloat *coords;
};

struct glVertexStream3iATI_params
{
    TEB *teb;
    GLenum stream;
    GLint x;
    GLint y;
    GLint z;
};

struct glVertexStream3ivATI_params
{
    TEB *teb;
    GLenum stream;
    const GLint *coords;
};

struct glVertexStream3sATI_params
{
    TEB *teb;
    GLenum stream;
    GLshort x;
    GLshort y;
    GLshort z;
};

struct glVertexStream3svATI_params
{
    TEB *teb;
    GLenum stream;
    const GLshort *coords;
};

struct glVertexStream4dATI_params
{
    TEB *teb;
    GLenum stream;
    GLdouble x;
    GLdouble y;
    GLdouble z;
    GLdouble w;
};

struct glVertexStream4dvATI_params
{
    TEB *teb;
    GLenum stream;
    const GLdouble *coords;
};

struct glVertexStream4fATI_params
{
    TEB *teb;
    GLenum stream;
    GLfloat x;
    GLfloat y;
    GLfloat z;
    GLfloat w;
};

struct glVertexStream4fvATI_params
{
    TEB *teb;
    GLenum stream;
    const GLfloat *coords;
};

struct glVertexStream4iATI_params
{
    TEB *teb;
    GLenum stream;
    GLint x;
    GLint y;
    GLint z;
    GLint w;
};

struct glVertexStream4ivATI_params
{
    TEB *teb;
    GLenum stream;
    const GLint *coords;
};

struct glVertexStream4sATI_params
{
    TEB *teb;
    GLenum stream;
    GLshort x;
    GLshort y;
    GLshort z;
    GLshort w;
};

struct glVertexStream4svATI_params
{
    TEB *teb;
    GLenum stream;
    const GLshort *coords;
};

struct glVertexWeightPointerEXT_params
{
    TEB *teb;
    GLint size;
    GLenum type;
    GLsizei stride;
    const void *pointer;
};

struct glVertexWeightfEXT_params
{
    TEB *teb;
    GLfloat weight;
};

struct glVertexWeightfvEXT_params
{
    TEB *teb;
    const GLfloat *weight;
};

struct glVertexWeighthNV_params
{
    TEB *teb;
    GLhalfNV weight;
};

struct glVertexWeighthvNV_params
{
    TEB *teb;
    const GLhalfNV *weight;
};

struct glVideoCaptureNV_params
{
    TEB *teb;
    GLuint video_capture_slot;
    GLuint *sequence_num;
    GLuint64EXT *capture_time;
    GLenum ret;
};

struct glVideoCaptureStreamParameterdvNV_params
{
    TEB *teb;
    GLuint video_capture_slot;
    GLuint stream;
    GLenum pname;
    const GLdouble *params;
};

struct glVideoCaptureStreamParameterfvNV_params
{
    TEB *teb;
    GLuint video_capture_slot;
    GLuint stream;
    GLenum pname;
    const GLfloat *params;
};

struct glVideoCaptureStreamParameterivNV_params
{
    TEB *teb;
    GLuint video_capture_slot;
    GLuint stream;
    GLenum pname;
    const GLint *params;
};

struct glViewportArrayv_params
{
    TEB *teb;
    GLuint first;
    GLsizei count;
    const GLfloat *v;
};

struct glViewportIndexedf_params
{
    TEB *teb;
    GLuint index;
    GLfloat x;
    GLfloat y;
    GLfloat w;
    GLfloat h;
};

struct glViewportIndexedfv_params
{
    TEB *teb;
    GLuint index;
    const GLfloat *v;
};

struct glViewportPositionWScaleNV_params
{
    TEB *teb;
    GLuint index;
    GLfloat xcoeff;
    GLfloat ycoeff;
};

struct glViewportSwizzleNV_params
{
    TEB *teb;
    GLuint index;
    GLenum swizzlex;
    GLenum swizzley;
    GLenum swizzlez;
    GLenum swizzlew;
};

struct glWaitSemaphoreEXT_params
{
    TEB *teb;
    GLuint semaphore;
    GLuint numBufferBarriers;
    const GLuint *buffers;
    GLuint numTextureBarriers;
    const GLuint *textures;
    const GLenum *srcLayouts;
};

struct glWaitSemaphoreui64NVX_params
{
    TEB *teb;
    GLuint waitGpu;
    GLsizei fenceObjectCount;
    const GLuint *semaphoreArray;
    const GLuint64 *fenceValueArray;
};

struct glWaitSync_params
{
    TEB *teb;
    GLsync sync;
    GLbitfield flags;
    GLuint64 timeout;
};

struct glWaitVkSemaphoreNV_params
{
    TEB *teb;
    GLuint64 vkSemaphore;
};

struct glWeightPathsNV_params
{
    TEB *teb;
    GLuint resultPath;
    GLsizei numPaths;
    const GLuint *paths;
    const GLfloat *weights;
};

struct glWeightPointerARB_params
{
    TEB *teb;
    GLint size;
    GLenum type;
    GLsizei stride;
    const void *pointer;
};

struct glWeightbvARB_params
{
    TEB *teb;
    GLint size;
    const GLbyte *weights;
};

struct glWeightdvARB_params
{
    TEB *teb;
    GLint size;
    const GLdouble *weights;
};

struct glWeightfvARB_params
{
    TEB *teb;
    GLint size;
    const GLfloat *weights;
};

struct glWeightivARB_params
{
    TEB *teb;
    GLint size;
    const GLint *weights;
};

struct glWeightsvARB_params
{
    TEB *teb;
    GLint size;
    const GLshort *weights;
};

struct glWeightubvARB_params
{
    TEB *teb;
    GLint size;
    const GLubyte *weights;
};

struct glWeightuivARB_params
{
    TEB *teb;
    GLint size;
    const GLuint *weights;
};

struct glWeightusvARB_params
{
    TEB *teb;
    GLint size;
    const GLushort *weights;
};

struct glWindowPos2d_params
{
    TEB *teb;
    GLdouble x;
    GLdouble y;
};

struct glWindowPos2dARB_params
{
    TEB *teb;
    GLdouble x;
    GLdouble y;
};

struct glWindowPos2dMESA_params
{
    TEB *teb;
    GLdouble x;
    GLdouble y;
};

struct glWindowPos2dv_params
{
    TEB *teb;
    const GLdouble *v;
};

struct glWindowPos2dvARB_params
{
    TEB *teb;
    const GLdouble *v;
};

struct glWindowPos2dvMESA_params
{
    TEB *teb;
    const GLdouble *v;
};

struct glWindowPos2f_params
{
    TEB *teb;
    GLfloat x;
    GLfloat y;
};

struct glWindowPos2fARB_params
{
    TEB *teb;
    GLfloat x;
    GLfloat y;
};

struct glWindowPos2fMESA_params
{
    TEB *teb;
    GLfloat x;
    GLfloat y;
};

struct glWindowPos2fv_params
{
    TEB *teb;
    const GLfloat *v;
};

struct glWindowPos2fvARB_params
{
    TEB *teb;
    const GLfloat *v;
};

struct glWindowPos2fvMESA_params
{
    TEB *teb;
    const GLfloat *v;
};

struct glWindowPos2i_params
{
    TEB *teb;
    GLint x;
    GLint y;
};

struct glWindowPos2iARB_params
{
    TEB *teb;
    GLint x;
    GLint y;
};

struct glWindowPos2iMESA_params
{
    TEB *teb;
    GLint x;
    GLint y;
};

struct glWindowPos2iv_params
{
    TEB *teb;
    const GLint *v;
};

struct glWindowPos2ivARB_params
{
    TEB *teb;
    const GLint *v;
};

struct glWindowPos2ivMESA_params
{
    TEB *teb;
    const GLint *v;
};

struct glWindowPos2s_params
{
    TEB *teb;
    GLshort x;
    GLshort y;
};

struct glWindowPos2sARB_params
{
    TEB *teb;
    GLshort x;
    GLshort y;
};

struct glWindowPos2sMESA_params
{
    TEB *teb;
    GLshort x;
    GLshort y;
};

struct glWindowPos2sv_params
{
    TEB *teb;
    const GLshort *v;
};

struct glWindowPos2svARB_params
{
    TEB *teb;
    const GLshort *v;
};

struct glWindowPos2svMESA_params
{
    TEB *teb;
    const GLshort *v;
};

struct glWindowPos3d_params
{
    TEB *teb;
    GLdouble x;
    GLdouble y;
    GLdouble z;
};

struct glWindowPos3dARB_params
{
    TEB *teb;
    GLdouble x;
    GLdouble y;
    GLdouble z;
};

struct glWindowPos3dMESA_params
{
    TEB *teb;
    GLdouble x;
    GLdouble y;
    GLdouble z;
};

struct glWindowPos3dv_params
{
    TEB *teb;
    const GLdouble *v;
};

struct glWindowPos3dvARB_params
{
    TEB *teb;
    const GLdouble *v;
};

struct glWindowPos3dvMESA_params
{
    TEB *teb;
    const GLdouble *v;
};

struct glWindowPos3f_params
{
    TEB *teb;
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

struct glWindowPos3fARB_params
{
    TEB *teb;
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

struct glWindowPos3fMESA_params
{
    TEB *teb;
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

struct glWindowPos3fv_params
{
    TEB *teb;
    const GLfloat *v;
};

struct glWindowPos3fvARB_params
{
    TEB *teb;
    const GLfloat *v;
};

struct glWindowPos3fvMESA_params
{
    TEB *teb;
    const GLfloat *v;
};

struct glWindowPos3i_params
{
    TEB *teb;
    GLint x;
    GLint y;
    GLint z;
};

struct glWindowPos3iARB_params
{
    TEB *teb;
    GLint x;
    GLint y;
    GLint z;
};

struct glWindowPos3iMESA_params
{
    TEB *teb;
    GLint x;
    GLint y;
    GLint z;
};

struct glWindowPos3iv_params
{
    TEB *teb;
    const GLint *v;
};

struct glWindowPos3ivARB_params
{
    TEB *teb;
    const GLint *v;
};

struct glWindowPos3ivMESA_params
{
    TEB *teb;
    const GLint *v;
};

struct glWindowPos3s_params
{
    TEB *teb;
    GLshort x;
    GLshort y;
    GLshort z;
};

struct glWindowPos3sARB_params
{
    TEB *teb;
    GLshort x;
    GLshort y;
    GLshort z;
};

struct glWindowPos3sMESA_params
{
    TEB *teb;
    GLshort x;
    GLshort y;
    GLshort z;
};

struct glWindowPos3sv_params
{
    TEB *teb;
    const GLshort *v;
};

struct glWindowPos3svARB_params
{
    TEB *teb;
    const GLshort *v;
};

struct glWindowPos3svMESA_params
{
    TEB *teb;
    const GLshort *v;
};

struct glWindowPos4dMESA_params
{
    TEB *teb;
    GLdouble x;
    GLdouble y;
    GLdouble z;
    GLdouble w;
};

struct glWindowPos4dvMESA_params
{
    TEB *teb;
    const GLdouble *v;
};

struct glWindowPos4fMESA_params
{
    TEB *teb;
    GLfloat x;
    GLfloat y;
    GLfloat z;
    GLfloat w;
};

struct glWindowPos4fvMESA_params
{
    TEB *teb;
    const GLfloat *v;
};

struct glWindowPos4iMESA_params
{
    TEB *teb;
    GLint x;
    GLint y;
    GLint z;
    GLint w;
};

struct glWindowPos4ivMESA_params
{
    TEB *teb;
    const GLint *v;
};

struct glWindowPos4sMESA_params
{
    TEB *teb;
    GLshort x;
    GLshort y;
    GLshort z;
    GLshort w;
};

struct glWindowPos4svMESA_params
{
    TEB *teb;
    const GLshort *v;
};

struct glWindowRectanglesEXT_params
{
    TEB *teb;
    GLenum mode;
    GLsizei count;
    const GLint *box;
};

struct glWriteMaskEXT_params
{
    TEB *teb;
    GLuint res;
    GLuint in;
    GLenum outX;
    GLenum outY;
    GLenum outZ;
    GLenum outW;
};

struct wglAllocateMemoryNV_params
{
    TEB *teb;
    GLsizei size;
    GLfloat readfreq;
    GLfloat writefreq;
    GLfloat priority;
    void *ret;
};

struct wglBindTexImageARB_params
{
    TEB *teb;
    HPBUFFERARB hPbuffer;
    int iBuffer;
    BOOL ret;
};

struct wglChoosePixelFormatARB_params
{
    TEB *teb;
    HDC hdc;
    const int *piAttribIList;
    const FLOAT *pfAttribFList;
    UINT nMaxFormats;
    int *piFormats;
    UINT *nNumFormats;
    BOOL ret;
};

struct wglCreateContextAttribsARB_params
{
    TEB *teb;
    HDC hDC;
    HGLRC hShareContext;
    const int *attribList;
    HGLRC ret;
};

struct wglCreatePbufferARB_params
{
    TEB *teb;
    HDC hDC;
    int iPixelFormat;
    int iWidth;
    int iHeight;
    const int *piAttribList;
    HPBUFFERARB ret;
};

struct wglDestroyPbufferARB_params
{
    TEB *teb;
    HPBUFFERARB hPbuffer;
    BOOL ret;
};

struct wglFreeMemoryNV_params
{
    TEB *teb;
    void *pointer;
};

struct wglGetExtensionsStringARB_params
{
    TEB *teb;
    HDC hdc;
    const char *ret;
};

struct wglGetExtensionsStringEXT_params
{
    TEB *teb;
    const char *ret;
};

struct wglGetPbufferDCARB_params
{
    TEB *teb;
    HPBUFFERARB hPbuffer;
    HDC ret;
};

struct wglGetPixelFormatAttribfvARB_params
{
    TEB *teb;
    HDC hdc;
    int iPixelFormat;
    int iLayerPlane;
    UINT nAttributes;
    const int *piAttributes;
    FLOAT *pfValues;
    BOOL ret;
};

struct wglGetPixelFormatAttribivARB_params
{
    TEB *teb;
    HDC hdc;
    int iPixelFormat;
    int iLayerPlane;
    UINT nAttributes;
    const int *piAttributes;
    int *piValues;
    BOOL ret;
};

struct wglGetSwapIntervalEXT_params
{
    TEB *teb;
    int ret;
};

struct wglMakeContextCurrentARB_params
{
    TEB *teb;
    HDC hDrawDC;
    HDC hReadDC;
    HGLRC hglrc;
    BOOL ret;
};

struct wglQueryCurrentRendererIntegerWINE_params
{
    TEB *teb;
    GLenum attribute;
    GLuint *value;
    BOOL ret;
};

struct wglQueryCurrentRendererStringWINE_params
{
    TEB *teb;
    GLenum attribute;
    const GLchar *ret;
};

struct wglQueryPbufferARB_params
{
    TEB *teb;
    HPBUFFERARB hPbuffer;
    int iAttribute;
    int *piValue;
    BOOL ret;
};

struct wglQueryRendererIntegerWINE_params
{
    TEB *teb;
    HDC dc;
    GLint renderer;
    GLenum attribute;
    GLuint *value;
    BOOL ret;
};

struct wglQueryRendererStringWINE_params
{
    TEB *teb;
    HDC dc;
    GLint renderer;
    GLenum attribute;
    const GLchar *ret;
};

struct wglReleasePbufferDCARB_params
{
    TEB *teb;
    HPBUFFERARB hPbuffer;
    HDC hDC;
    int ret;
};

struct wglReleaseTexImageARB_params
{
    TEB *teb;
    HPBUFFERARB hPbuffer;
    int iBuffer;
    BOOL ret;
};

struct wglSetPbufferAttribARB_params
{
    TEB *teb;
    HPBUFFERARB hPbuffer;
    const int *piAttribList;
    BOOL ret;
};

struct wglSetPixelFormatWINE_params
{
    TEB *teb;
    HDC hdc;
    int format;
    BOOL ret;
};

struct wglSwapIntervalEXT_params
{
    TEB *teb;
    int interval;
    BOOL ret;
};

struct get_pixel_formats_params
{
    TEB *teb;
    HDC hdc;
    struct wgl_pixel_format *formats;
    unsigned int max_formats;
    unsigned int num_formats;
    unsigned int num_onscreen_formats;
};

enum unix_funcs
{
    unix_process_attach,
    unix_thread_attach,
    unix_process_detach,
    unix_get_pixel_formats,
    unix_wglCopyContext,
    unix_wglCreateContext,
    unix_wglDeleteContext,
    unix_wglGetPixelFormat,
    unix_wglGetProcAddress,
    unix_wglMakeCurrent,
    unix_wglSetPixelFormat,
    unix_wglShareLists,
    unix_wglSwapBuffers,
    unix_glAccum,
    unix_glAlphaFunc,
    unix_glAreTexturesResident,
    unix_glArrayElement,
    unix_glBegin,
    unix_glBindTexture,
    unix_glBitmap,
    unix_glBlendFunc,
    unix_glCallList,
    unix_glCallLists,
    unix_glClear,
    unix_glClearAccum,
    unix_glClearColor,
    unix_glClearDepth,
    unix_glClearIndex,
    unix_glClearStencil,
    unix_glClipPlane,
    unix_glColor3b,
    unix_glColor3bv,
    unix_glColor3d,
    unix_glColor3dv,
    unix_glColor3f,
    unix_glColor3fv,
    unix_glColor3i,
    unix_glColor3iv,
    unix_glColor3s,
    unix_glColor3sv,
    unix_glColor3ub,
    unix_glColor3ubv,
    unix_glColor3ui,
    unix_glColor3uiv,
    unix_glColor3us,
    unix_glColor3usv,
    unix_glColor4b,
    unix_glColor4bv,
    unix_glColor4d,
    unix_glColor4dv,
    unix_glColor4f,
    unix_glColor4fv,
    unix_glColor4i,
    unix_glColor4iv,
    unix_glColor4s,
    unix_glColor4sv,
    unix_glColor4ub,
    unix_glColor4ubv,
    unix_glColor4ui,
    unix_glColor4uiv,
    unix_glColor4us,
    unix_glColor4usv,
    unix_glColorMask,
    unix_glColorMaterial,
    unix_glColorPointer,
    unix_glCopyPixels,
    unix_glCopyTexImage1D,
    unix_glCopyTexImage2D,
    unix_glCopyTexSubImage1D,
    unix_glCopyTexSubImage2D,
    unix_glCullFace,
    unix_glDeleteLists,
    unix_glDeleteTextures,
    unix_glDepthFunc,
    unix_glDepthMask,
    unix_glDepthRange,
    unix_glDisable,
    unix_glDisableClientState,
    unix_glDrawArrays,
    unix_glDrawBuffer,
    unix_glDrawElements,
    unix_glDrawPixels,
    unix_glEdgeFlag,
    unix_glEdgeFlagPointer,
    unix_glEdgeFlagv,
    unix_glEnable,
    unix_glEnableClientState,
    unix_glEnd,
    unix_glEndList,
    unix_glEvalCoord1d,
    unix_glEvalCoord1dv,
    unix_glEvalCoord1f,
    unix_glEvalCoord1fv,
    unix_glEvalCoord2d,
    unix_glEvalCoord2dv,
    unix_glEvalCoord2f,
    unix_glEvalCoord2fv,
    unix_glEvalMesh1,
    unix_glEvalMesh2,
    unix_glEvalPoint1,
    unix_glEvalPoint2,
    unix_glFeedbackBuffer,
    unix_glFinish,
    unix_glFlush,
    unix_glFogf,
    unix_glFogfv,
    unix_glFogi,
    unix_glFogiv,
    unix_glFrontFace,
    unix_glFrustum,
    unix_glGenLists,
    unix_glGenTextures,
    unix_glGetBooleanv,
    unix_glGetClipPlane,
    unix_glGetDoublev,
    unix_glGetError,
    unix_glGetFloatv,
    unix_glGetIntegerv,
    unix_glGetLightfv,
    unix_glGetLightiv,
    unix_glGetMapdv,
    unix_glGetMapfv,
    unix_glGetMapiv,
    unix_glGetMaterialfv,
    unix_glGetMaterialiv,
    unix_glGetPixelMapfv,
    unix_glGetPixelMapuiv,
    unix_glGetPixelMapusv,
    unix_glGetPointerv,
    unix_glGetPolygonStipple,
    unix_glGetString,
    unix_glGetTexEnvfv,
    unix_glGetTexEnviv,
    unix_glGetTexGendv,
    unix_glGetTexGenfv,
    unix_glGetTexGeniv,
    unix_glGetTexImage,
    unix_glGetTexLevelParameterfv,
    unix_glGetTexLevelParameteriv,
    unix_glGetTexParameterfv,
    unix_glGetTexParameteriv,
    unix_glHint,
    unix_glIndexMask,
    unix_glIndexPointer,
    unix_glIndexd,
    unix_glIndexdv,
    unix_glIndexf,
    unix_glIndexfv,
    unix_glIndexi,
    unix_glIndexiv,
    unix_glIndexs,
    unix_glIndexsv,
    unix_glIndexub,
    unix_glIndexubv,
    unix_glInitNames,
    unix_glInterleavedArrays,
    unix_glIsEnabled,
    unix_glIsList,
    unix_glIsTexture,
    unix_glLightModelf,
    unix_glLightModelfv,
    unix_glLightModeli,
    unix_glLightModeliv,
    unix_glLightf,
    unix_glLightfv,
    unix_glLighti,
    unix_glLightiv,
    unix_glLineStipple,
    unix_glLineWidth,
    unix_glListBase,
    unix_glLoadIdentity,
    unix_glLoadMatrixd,
    unix_glLoadMatrixf,
    unix_glLoadName,
    unix_glLogicOp,
    unix_glMap1d,
    unix_glMap1f,
    unix_glMap2d,
    unix_glMap2f,
    unix_glMapGrid1d,
    unix_glMapGrid1f,
    unix_glMapGrid2d,
    unix_glMapGrid2f,
    unix_glMaterialf,
    unix_glMaterialfv,
    unix_glMateriali,
    unix_glMaterialiv,
    unix_glMatrixMode,
    unix_glMultMatrixd,
    unix_glMultMatrixf,
    unix_glNewList,
    unix_glNormal3b,
    unix_glNormal3bv,
    unix_glNormal3d,
    unix_glNormal3dv,
    unix_glNormal3f,
    unix_glNormal3fv,
    unix_glNormal3i,
    unix_glNormal3iv,
    unix_glNormal3s,
    unix_glNormal3sv,
    unix_glNormalPointer,
    unix_glOrtho,
    unix_glPassThrough,
    unix_glPixelMapfv,
    unix_glPixelMapuiv,
    unix_glPixelMapusv,
    unix_glPixelStoref,
    unix_glPixelStorei,
    unix_glPixelTransferf,
    unix_glPixelTransferi,
    unix_glPixelZoom,
    unix_glPointSize,
    unix_glPolygonMode,
    unix_glPolygonOffset,
    unix_glPolygonStipple,
    unix_glPopAttrib,
    unix_glPopClientAttrib,
    unix_glPopMatrix,
    unix_glPopName,
    unix_glPrioritizeTextures,
    unix_glPushAttrib,
    unix_glPushClientAttrib,
    unix_glPushMatrix,
    unix_glPushName,
    unix_glRasterPos2d,
    unix_glRasterPos2dv,
    unix_glRasterPos2f,
    unix_glRasterPos2fv,
    unix_glRasterPos2i,
    unix_glRasterPos2iv,
    unix_glRasterPos2s,
    unix_glRasterPos2sv,
    unix_glRasterPos3d,
    unix_glRasterPos3dv,
    unix_glRasterPos3f,
    unix_glRasterPos3fv,
    unix_glRasterPos3i,
    unix_glRasterPos3iv,
    unix_glRasterPos3s,
    unix_glRasterPos3sv,
    unix_glRasterPos4d,
    unix_glRasterPos4dv,
    unix_glRasterPos4f,
    unix_glRasterPos4fv,
    unix_glRasterPos4i,
    unix_glRasterPos4iv,
    unix_glRasterPos4s,
    unix_glRasterPos4sv,
    unix_glReadBuffer,
    unix_glReadPixels,
    unix_glRectd,
    unix_glRectdv,
    unix_glRectf,
    unix_glRectfv,
    unix_glRecti,
    unix_glRectiv,
    unix_glRects,
    unix_glRectsv,
    unix_glRenderMode,
    unix_glRotated,
    unix_glRotatef,
    unix_glScaled,
    unix_glScalef,
    unix_glScissor,
    unix_glSelectBuffer,
    unix_glShadeModel,
    unix_glStencilFunc,
    unix_glStencilMask,
    unix_glStencilOp,
    unix_glTexCoord1d,
    unix_glTexCoord1dv,
    unix_glTexCoord1f,
    unix_glTexCoord1fv,
    unix_glTexCoord1i,
    unix_glTexCoord1iv,
    unix_glTexCoord1s,
    unix_glTexCoord1sv,
    unix_glTexCoord2d,
    unix_glTexCoord2dv,
    unix_glTexCoord2f,
    unix_glTexCoord2fv,
    unix_glTexCoord2i,
    unix_glTexCoord2iv,
    unix_glTexCoord2s,
    unix_glTexCoord2sv,
    unix_glTexCoord3d,
    unix_glTexCoord3dv,
    unix_glTexCoord3f,
    unix_glTexCoord3fv,
    unix_glTexCoord3i,
    unix_glTexCoord3iv,
    unix_glTexCoord3s,
    unix_glTexCoord3sv,
    unix_glTexCoord4d,
    unix_glTexCoord4dv,
    unix_glTexCoord4f,
    unix_glTexCoord4fv,
    unix_glTexCoord4i,
    unix_glTexCoord4iv,
    unix_glTexCoord4s,
    unix_glTexCoord4sv,
    unix_glTexCoordPointer,
    unix_glTexEnvf,
    unix_glTexEnvfv,
    unix_glTexEnvi,
    unix_glTexEnviv,
    unix_glTexGend,
    unix_glTexGendv,
    unix_glTexGenf,
    unix_glTexGenfv,
    unix_glTexGeni,
    unix_glTexGeniv,
    unix_glTexImage1D,
    unix_glTexImage2D,
    unix_glTexParameterf,
    unix_glTexParameterfv,
    unix_glTexParameteri,
    unix_glTexParameteriv,
    unix_glTexSubImage1D,
    unix_glTexSubImage2D,
    unix_glTranslated,
    unix_glTranslatef,
    unix_glVertex2d,
    unix_glVertex2dv,
    unix_glVertex2f,
    unix_glVertex2fv,
    unix_glVertex2i,
    unix_glVertex2iv,
    unix_glVertex2s,
    unix_glVertex2sv,
    unix_glVertex3d,
    unix_glVertex3dv,
    unix_glVertex3f,
    unix_glVertex3fv,
    unix_glVertex3i,
    unix_glVertex3iv,
    unix_glVertex3s,
    unix_glVertex3sv,
    unix_glVertex4d,
    unix_glVertex4dv,
    unix_glVertex4f,
    unix_glVertex4fv,
    unix_glVertex4i,
    unix_glVertex4iv,
    unix_glVertex4s,
    unix_glVertex4sv,
    unix_glVertexPointer,
    unix_glViewport,
    unix_glAccumxOES,
    unix_glAcquireKeyedMutexWin32EXT,
    unix_glActiveProgramEXT,
    unix_glActiveShaderProgram,
    unix_glActiveStencilFaceEXT,
    unix_glActiveTexture,
    unix_glActiveTextureARB,
    unix_glActiveVaryingNV,
    unix_glAlphaFragmentOp1ATI,
    unix_glAlphaFragmentOp2ATI,
    unix_glAlphaFragmentOp3ATI,
    unix_glAlphaFuncxOES,
    unix_glAlphaToCoverageDitherControlNV,
    unix_glApplyFramebufferAttachmentCMAAINTEL,
    unix_glApplyTextureEXT,
    unix_glAreProgramsResidentNV,
    unix_glAreTexturesResidentEXT,
    unix_glArrayElementEXT,
    unix_glArrayObjectATI,
    unix_glAsyncCopyBufferSubDataNVX,
    unix_glAsyncCopyImageSubDataNVX,
    unix_glAsyncMarkerSGIX,
    unix_glAttachObjectARB,
    unix_glAttachShader,
    unix_glBeginConditionalRender,
    unix_glBeginConditionalRenderNV,
    unix_glBeginConditionalRenderNVX,
    unix_glBeginFragmentShaderATI,
    unix_glBeginOcclusionQueryNV,
    unix_glBeginPerfMonitorAMD,
    unix_glBeginPerfQueryINTEL,
    unix_glBeginQuery,
    unix_glBeginQueryARB,
    unix_glBeginQueryIndexed,
    unix_glBeginTransformFeedback,
    unix_glBeginTransformFeedbackEXT,
    unix_glBeginTransformFeedbackNV,
    unix_glBeginVertexShaderEXT,
    unix_glBeginVideoCaptureNV,
    unix_glBindAttribLocation,
    unix_glBindAttribLocationARB,
    unix_glBindBuffer,
    unix_glBindBufferARB,
    unix_glBindBufferBase,
    unix_glBindBufferBaseEXT,
    unix_glBindBufferBaseNV,
    unix_glBindBufferOffsetEXT,
    unix_glBindBufferOffsetNV,
    unix_glBindBufferRange,
    unix_glBindBufferRangeEXT,
    unix_glBindBufferRangeNV,
    unix_glBindBuffersBase,
    unix_glBindBuffersRange,
    unix_glBindFragDataLocation,
    unix_glBindFragDataLocationEXT,
    unix_glBindFragDataLocationIndexed,
    unix_glBindFragmentShaderATI,
    unix_glBindFramebuffer,
    unix_glBindFramebufferEXT,
    unix_glBindImageTexture,
    unix_glBindImageTextureEXT,
    unix_glBindImageTextures,
    unix_glBindLightParameterEXT,
    unix_glBindMaterialParameterEXT,
    unix_glBindMultiTextureEXT,
    unix_glBindParameterEXT,
    unix_glBindProgramARB,
    unix_glBindProgramNV,
    unix_glBindProgramPipeline,
    unix_glBindRenderbuffer,
    unix_glBindRenderbufferEXT,
    unix_glBindSampler,
    unix_glBindSamplers,
    unix_glBindShadingRateImageNV,
    unix_glBindTexGenParameterEXT,
    unix_glBindTextureEXT,
    unix_glBindTextureUnit,
    unix_glBindTextureUnitParameterEXT,
    unix_glBindTextures,
    unix_glBindTransformFeedback,
    unix_glBindTransformFeedbackNV,
    unix_glBindVertexArray,
    unix_glBindVertexArrayAPPLE,
    unix_glBindVertexBuffer,
    unix_glBindVertexBuffers,
    unix_glBindVertexShaderEXT,
    unix_glBindVideoCaptureStreamBufferNV,
    unix_glBindVideoCaptureStreamTextureNV,
    unix_glBinormal3bEXT,
    unix_glBinormal3bvEXT,
    unix_glBinormal3dEXT,
    unix_glBinormal3dvEXT,
    unix_glBinormal3fEXT,
    unix_glBinormal3fvEXT,
    unix_glBinormal3iEXT,
    unix_glBinormal3ivEXT,
    unix_glBinormal3sEXT,
    unix_glBinormal3svEXT,
    unix_glBinormalPointerEXT,
    unix_glBitmapxOES,
    unix_glBlendBarrierKHR,
    unix_glBlendBarrierNV,
    unix_glBlendColor,
    unix_glBlendColorEXT,
    unix_glBlendColorxOES,
    unix_glBlendEquation,
    unix_glBlendEquationEXT,
    unix_glBlendEquationIndexedAMD,
    unix_glBlendEquationSeparate,
    unix_glBlendEquationSeparateEXT,
    unix_glBlendEquationSeparateIndexedAMD,
    unix_glBlendEquationSeparatei,
    unix_glBlendEquationSeparateiARB,
    unix_glBlendEquationi,
    unix_glBlendEquationiARB,
    unix_glBlendFuncIndexedAMD,
    unix_glBlendFuncSeparate,
    unix_glBlendFuncSeparateEXT,
    unix_glBlendFuncSeparateINGR,
    unix_glBlendFuncSeparateIndexedAMD,
    unix_glBlendFuncSeparatei,
    unix_glBlendFuncSeparateiARB,
    unix_glBlendFunci,
    unix_glBlendFunciARB,
    unix_glBlendParameteriNV,
    unix_glBlitFramebuffer,
    unix_glBlitFramebufferEXT,
    unix_glBlitNamedFramebuffer,
    unix_glBufferAddressRangeNV,
    unix_glBufferAttachMemoryNV,
    unix_glBufferData,
    unix_glBufferDataARB,
    unix_glBufferPageCommitmentARB,
    unix_glBufferParameteriAPPLE,
    unix_glBufferRegionEnabled,
    unix_glBufferStorage,
    unix_glBufferStorageExternalEXT,
    unix_glBufferStorageMemEXT,
    unix_glBufferSubData,
    unix_glBufferSubDataARB,
    unix_glCallCommandListNV,
    unix_glCheckFramebufferStatus,
    unix_glCheckFramebufferStatusEXT,
    unix_glCheckNamedFramebufferStatus,
    unix_glCheckNamedFramebufferStatusEXT,
    unix_glClampColor,
    unix_glClampColorARB,
    unix_glClearAccumxOES,
    unix_glClearBufferData,
    unix_glClearBufferSubData,
    unix_glClearBufferfi,
    unix_glClearBufferfv,
    unix_glClearBufferiv,
    unix_glClearBufferuiv,
    unix_glClearColorIiEXT,
    unix_glClearColorIuiEXT,
    unix_glClearColorxOES,
    unix_glClearDepthdNV,
    unix_glClearDepthf,
    unix_glClearDepthfOES,
    unix_glClearDepthxOES,
    unix_glClearNamedBufferData,
    unix_glClearNamedBufferDataEXT,
    unix_glClearNamedBufferSubData,
    unix_glClearNamedBufferSubDataEXT,
    unix_glClearNamedFramebufferfi,
    unix_glClearNamedFramebufferfv,
    unix_glClearNamedFramebufferiv,
    unix_glClearNamedFramebufferuiv,
    unix_glClearTexImage,
    unix_glClearTexSubImage,
    unix_glClientActiveTexture,
    unix_glClientActiveTextureARB,
    unix_glClientActiveVertexStreamATI,
    unix_glClientAttribDefaultEXT,
    unix_glClientWaitSemaphoreui64NVX,
    unix_glClientWaitSync,
    unix_glClipControl,
    unix_glClipPlanefOES,
    unix_glClipPlanexOES,
    unix_glColor3fVertex3fSUN,
    unix_glColor3fVertex3fvSUN,
    unix_glColor3hNV,
    unix_glColor3hvNV,
    unix_glColor3xOES,
    unix_glColor3xvOES,
    unix_glColor4fNormal3fVertex3fSUN,
    unix_glColor4fNormal3fVertex3fvSUN,
    unix_glColor4hNV,
    unix_glColor4hvNV,
    unix_glColor4ubVertex2fSUN,
    unix_glColor4ubVertex2fvSUN,
    unix_glColor4ubVertex3fSUN,
    unix_glColor4ubVertex3fvSUN,
    unix_glColor4xOES,
    unix_glColor4xvOES,
    unix_glColorFormatNV,
    unix_glColorFragmentOp1ATI,
    unix_glColorFragmentOp2ATI,
    unix_glColorFragmentOp3ATI,
    unix_glColorMaskIndexedEXT,
    unix_glColorMaski,
    unix_glColorP3ui,
    unix_glColorP3uiv,
    unix_glColorP4ui,
    unix_glColorP4uiv,
    unix_glColorPointerEXT,
    unix_glColorPointerListIBM,
    unix_glColorPointervINTEL,
    unix_glColorSubTable,
    unix_glColorSubTableEXT,
    unix_glColorTable,
    unix_glColorTableEXT,
    unix_glColorTableParameterfv,
    unix_glColorTableParameterfvSGI,
    unix_glColorTableParameteriv,
    unix_glColorTableParameterivSGI,
    unix_glColorTableSGI,
    unix_glCombinerInputNV,
    unix_glCombinerOutputNV,
    unix_glCombinerParameterfNV,
    unix_glCombinerParameterfvNV,
    unix_glCombinerParameteriNV,
    unix_glCombinerParameterivNV,
    unix_glCombinerStageParameterfvNV,
    unix_glCommandListSegmentsNV,
    unix_glCompileCommandListNV,
    unix_glCompileShader,
    unix_glCompileShaderARB,
    unix_glCompileShaderIncludeARB,
    unix_glCompressedMultiTexImage1DEXT,
    unix_glCompressedMultiTexImage2DEXT,
    unix_glCompressedMultiTexImage3DEXT,
    unix_glCompressedMultiTexSubImage1DEXT,
    unix_glCompressedMultiTexSubImage2DEXT,
    unix_glCompressedMultiTexSubImage3DEXT,
    unix_glCompressedTexImage1D,
    unix_glCompressedTexImage1DARB,
    unix_glCompressedTexImage2D,
    unix_glCompressedTexImage2DARB,
    unix_glCompressedTexImage3D,
    unix_glCompressedTexImage3DARB,
    unix_glCompressedTexSubImage1D,
    unix_glCompressedTexSubImage1DARB,
    unix_glCompressedTexSubImage2D,
    unix_glCompressedTexSubImage2DARB,
    unix_glCompressedTexSubImage3D,
    unix_glCompressedTexSubImage3DARB,
    unix_glCompressedTextureImage1DEXT,
    unix_glCompressedTextureImage2DEXT,
    unix_glCompressedTextureImage3DEXT,
    unix_glCompressedTextureSubImage1D,
    unix_glCompressedTextureSubImage1DEXT,
    unix_glCompressedTextureSubImage2D,
    unix_glCompressedTextureSubImage2DEXT,
    unix_glCompressedTextureSubImage3D,
    unix_glCompressedTextureSubImage3DEXT,
    unix_glConservativeRasterParameterfNV,
    unix_glConservativeRasterParameteriNV,
    unix_glConvolutionFilter1D,
    unix_glConvolutionFilter1DEXT,
    unix_glConvolutionFilter2D,
    unix_glConvolutionFilter2DEXT,
    unix_glConvolutionParameterf,
    unix_glConvolutionParameterfEXT,
    unix_glConvolutionParameterfv,
    unix_glConvolutionParameterfvEXT,
    unix_glConvolutionParameteri,
    unix_glConvolutionParameteriEXT,
    unix_glConvolutionParameteriv,
    unix_glConvolutionParameterivEXT,
    unix_glConvolutionParameterxOES,
    unix_glConvolutionParameterxvOES,
    unix_glCopyBufferSubData,
    unix_glCopyColorSubTable,
    unix_glCopyColorSubTableEXT,
    unix_glCopyColorTable,
    unix_glCopyColorTableSGI,
    unix_glCopyConvolutionFilter1D,
    unix_glCopyConvolutionFilter1DEXT,
    unix_glCopyConvolutionFilter2D,
    unix_glCopyConvolutionFilter2DEXT,
    unix_glCopyImageSubData,
    unix_glCopyImageSubDataNV,
    unix_glCopyMultiTexImage1DEXT,
    unix_glCopyMultiTexImage2DEXT,
    unix_glCopyMultiTexSubImage1DEXT,
    unix_glCopyMultiTexSubImage2DEXT,
    unix_glCopyMultiTexSubImage3DEXT,
    unix_glCopyNamedBufferSubData,
    unix_glCopyPathNV,
    unix_glCopyTexImage1DEXT,
    unix_glCopyTexImage2DEXT,
    unix_glCopyTexSubImage1DEXT,
    unix_glCopyTexSubImage2DEXT,
    unix_glCopyTexSubImage3D,
    unix_glCopyTexSubImage3DEXT,
    unix_glCopyTextureImage1DEXT,
    unix_glCopyTextureImage2DEXT,
    unix_glCopyTextureSubImage1D,
    unix_glCopyTextureSubImage1DEXT,
    unix_glCopyTextureSubImage2D,
    unix_glCopyTextureSubImage2DEXT,
    unix_glCopyTextureSubImage3D,
    unix_glCopyTextureSubImage3DEXT,
    unix_glCoverFillPathInstancedNV,
    unix_glCoverFillPathNV,
    unix_glCoverStrokePathInstancedNV,
    unix_glCoverStrokePathNV,
    unix_glCoverageModulationNV,
    unix_glCoverageModulationTableNV,
    unix_glCreateBuffers,
    unix_glCreateCommandListsNV,
    unix_glCreateFramebuffers,
    unix_glCreateMemoryObjectsEXT,
    unix_glCreatePerfQueryINTEL,
    unix_glCreateProgram,
    unix_glCreateProgramObjectARB,
    unix_glCreateProgramPipelines,
    unix_glCreateProgressFenceNVX,
    unix_glCreateQueries,
    unix_glCreateRenderbuffers,
    unix_glCreateSamplers,
    unix_glCreateShader,
    unix_glCreateShaderObjectARB,
    unix_glCreateShaderProgramEXT,
    unix_glCreateShaderProgramv,
    unix_glCreateStatesNV,
    unix_glCreateSyncFromCLeventARB,
    unix_glCreateTextures,
    unix_glCreateTransformFeedbacks,
    unix_glCreateVertexArrays,
    unix_glCullParameterdvEXT,
    unix_glCullParameterfvEXT,
    unix_glCurrentPaletteMatrixARB,
    unix_glDebugMessageCallback,
    unix_glDebugMessageCallbackAMD,
    unix_glDebugMessageCallbackARB,
    unix_glDebugMessageControl,
    unix_glDebugMessageControlARB,
    unix_glDebugMessageEnableAMD,
    unix_glDebugMessageInsert,
    unix_glDebugMessageInsertAMD,
    unix_glDebugMessageInsertARB,
    unix_glDeformSGIX,
    unix_glDeformationMap3dSGIX,
    unix_glDeformationMap3fSGIX,
    unix_glDeleteAsyncMarkersSGIX,
    unix_glDeleteBufferRegion,
    unix_glDeleteBuffers,
    unix_glDeleteBuffersARB,
    unix_glDeleteCommandListsNV,
    unix_glDeleteFencesAPPLE,
    unix_glDeleteFencesNV,
    unix_glDeleteFragmentShaderATI,
    unix_glDeleteFramebuffers,
    unix_glDeleteFramebuffersEXT,
    unix_glDeleteMemoryObjectsEXT,
    unix_glDeleteNamedStringARB,
    unix_glDeleteNamesAMD,
    unix_glDeleteObjectARB,
    unix_glDeleteObjectBufferATI,
    unix_glDeleteOcclusionQueriesNV,
    unix_glDeletePathsNV,
    unix_glDeletePerfMonitorsAMD,
    unix_glDeletePerfQueryINTEL,
    unix_glDeleteProgram,
    unix_glDeleteProgramPipelines,
    unix_glDeleteProgramsARB,
    unix_glDeleteProgramsNV,
    unix_glDeleteQueries,
    unix_glDeleteQueriesARB,
    unix_glDeleteQueryResourceTagNV,
    unix_glDeleteRenderbuffers,
    unix_glDeleteRenderbuffersEXT,
    unix_glDeleteSamplers,
    unix_glDeleteSemaphoresEXT,
    unix_glDeleteShader,
    unix_glDeleteStatesNV,
    unix_glDeleteSync,
    unix_glDeleteTexturesEXT,
    unix_glDeleteTransformFeedbacks,
    unix_glDeleteTransformFeedbacksNV,
    unix_glDeleteVertexArrays,
    unix_glDeleteVertexArraysAPPLE,
    unix_glDeleteVertexShaderEXT,
    unix_glDepthBoundsEXT,
    unix_glDepthBoundsdNV,
    unix_glDepthRangeArraydvNV,
    unix_glDepthRangeArrayv,
    unix_glDepthRangeIndexed,
    unix_glDepthRangeIndexeddNV,
    unix_glDepthRangedNV,
    unix_glDepthRangef,
    unix_glDepthRangefOES,
    unix_glDepthRangexOES,
    unix_glDetachObjectARB,
    unix_glDetachShader,
    unix_glDetailTexFuncSGIS,
    unix_glDisableClientStateIndexedEXT,
    unix_glDisableClientStateiEXT,
    unix_glDisableIndexedEXT,
    unix_glDisableVariantClientStateEXT,
    unix_glDisableVertexArrayAttrib,
    unix_glDisableVertexArrayAttribEXT,
    unix_glDisableVertexArrayEXT,
    unix_glDisableVertexAttribAPPLE,
    unix_glDisableVertexAttribArray,
    unix_glDisableVertexAttribArrayARB,
    unix_glDisablei,
    unix_glDispatchCompute,
    unix_glDispatchComputeGroupSizeARB,
    unix_glDispatchComputeIndirect,
    unix_glDrawArraysEXT,
    unix_glDrawArraysIndirect,
    unix_glDrawArraysInstanced,
    unix_glDrawArraysInstancedARB,
    unix_glDrawArraysInstancedBaseInstance,
    unix_glDrawArraysInstancedEXT,
    unix_glDrawBufferRegion,
    unix_glDrawBuffers,
    unix_glDrawBuffersARB,
    unix_glDrawBuffersATI,
    unix_glDrawCommandsAddressNV,
    unix_glDrawCommandsNV,
    unix_glDrawCommandsStatesAddressNV,
    unix_glDrawCommandsStatesNV,
    unix_glDrawElementArrayAPPLE,
    unix_glDrawElementArrayATI,
    unix_glDrawElementsBaseVertex,
    unix_glDrawElementsIndirect,
    unix_glDrawElementsInstanced,
    unix_glDrawElementsInstancedARB,
    unix_glDrawElementsInstancedBaseInstance,
    unix_glDrawElementsInstancedBaseVertex,
    unix_glDrawElementsInstancedBaseVertexBaseInstance,
    unix_glDrawElementsInstancedEXT,
    unix_glDrawMeshArraysSUN,
    unix_glDrawMeshTasksIndirectNV,
    unix_glDrawMeshTasksNV,
    unix_glDrawRangeElementArrayAPPLE,
    unix_glDrawRangeElementArrayATI,
    unix_glDrawRangeElements,
    unix_glDrawRangeElementsBaseVertex,
    unix_glDrawRangeElementsEXT,
    unix_glDrawTextureNV,
    unix_glDrawTransformFeedback,
    unix_glDrawTransformFeedbackInstanced,
    unix_glDrawTransformFeedbackNV,
    unix_glDrawTransformFeedbackStream,
    unix_glDrawTransformFeedbackStreamInstanced,
    unix_glDrawVkImageNV,
    unix_glEGLImageTargetTexStorageEXT,
    unix_glEGLImageTargetTextureStorageEXT,
    unix_glEdgeFlagFormatNV,
    unix_glEdgeFlagPointerEXT,
    unix_glEdgeFlagPointerListIBM,
    unix_glElementPointerAPPLE,
    unix_glElementPointerATI,
    unix_glEnableClientStateIndexedEXT,
    unix_glEnableClientStateiEXT,
    unix_glEnableIndexedEXT,
    unix_glEnableVariantClientStateEXT,
    unix_glEnableVertexArrayAttrib,
    unix_glEnableVertexArrayAttribEXT,
    unix_glEnableVertexArrayEXT,
    unix_glEnableVertexAttribAPPLE,
    unix_glEnableVertexAttribArray,
    unix_glEnableVertexAttribArrayARB,
    unix_glEnablei,
    unix_glEndConditionalRender,
    unix_glEndConditionalRenderNV,
    unix_glEndConditionalRenderNVX,
    unix_glEndFragmentShaderATI,
    unix_glEndOcclusionQueryNV,
    unix_glEndPerfMonitorAMD,
    unix_glEndPerfQueryINTEL,
    unix_glEndQuery,
    unix_glEndQueryARB,
    unix_glEndQueryIndexed,
    unix_glEndTransformFeedback,
    unix_glEndTransformFeedbackEXT,
    unix_glEndTransformFeedbackNV,
    unix_glEndVertexShaderEXT,
    unix_glEndVideoCaptureNV,
    unix_glEvalCoord1xOES,
    unix_glEvalCoord1xvOES,
    unix_glEvalCoord2xOES,
    unix_glEvalCoord2xvOES,
    unix_glEvalMapsNV,
    unix_glEvaluateDepthValuesARB,
    unix_glExecuteProgramNV,
    unix_glExtractComponentEXT,
    unix_glFeedbackBufferxOES,
    unix_glFenceSync,
    unix_glFinalCombinerInputNV,
    unix_glFinishAsyncSGIX,
    unix_glFinishFenceAPPLE,
    unix_glFinishFenceNV,
    unix_glFinishObjectAPPLE,
    unix_glFinishTextureSUNX,
    unix_glFlushMappedBufferRange,
    unix_glFlushMappedBufferRangeAPPLE,
    unix_glFlushMappedNamedBufferRange,
    unix_glFlushMappedNamedBufferRangeEXT,
    unix_glFlushPixelDataRangeNV,
    unix_glFlushRasterSGIX,
    unix_glFlushStaticDataIBM,
    unix_glFlushVertexArrayRangeAPPLE,
    unix_glFlushVertexArrayRangeNV,
    unix_glFogCoordFormatNV,
    unix_glFogCoordPointer,
    unix_glFogCoordPointerEXT,
    unix_glFogCoordPointerListIBM,
    unix_glFogCoordd,
    unix_glFogCoorddEXT,
    unix_glFogCoorddv,
    unix_glFogCoorddvEXT,
    unix_glFogCoordf,
    unix_glFogCoordfEXT,
    unix_glFogCoordfv,
    unix_glFogCoordfvEXT,
    unix_glFogCoordhNV,
    unix_glFogCoordhvNV,
    unix_glFogFuncSGIS,
    unix_glFogxOES,
    unix_glFogxvOES,
    unix_glFragmentColorMaterialSGIX,
    unix_glFragmentCoverageColorNV,
    unix_glFragmentLightModelfSGIX,
    unix_glFragmentLightModelfvSGIX,
    unix_glFragmentLightModeliSGIX,
    unix_glFragmentLightModelivSGIX,
    unix_glFragmentLightfSGIX,
    unix_glFragmentLightfvSGIX,
    unix_glFragmentLightiSGIX,
    unix_glFragmentLightivSGIX,
    unix_glFragmentMaterialfSGIX,
    unix_glFragmentMaterialfvSGIX,
    unix_glFragmentMaterialiSGIX,
    unix_glFragmentMaterialivSGIX,
    unix_glFrameTerminatorGREMEDY,
    unix_glFrameZoomSGIX,
    unix_glFramebufferDrawBufferEXT,
    unix_glFramebufferDrawBuffersEXT,
    unix_glFramebufferFetchBarrierEXT,
    unix_glFramebufferParameteri,
    unix_glFramebufferParameteriMESA,
    unix_glFramebufferReadBufferEXT,
    unix_glFramebufferRenderbuffer,
    unix_glFramebufferRenderbufferEXT,
    unix_glFramebufferSampleLocationsfvARB,
    unix_glFramebufferSampleLocationsfvNV,
    unix_glFramebufferSamplePositionsfvAMD,
    unix_glFramebufferTexture,
    unix_glFramebufferTexture1D,
    unix_glFramebufferTexture1DEXT,
    unix_glFramebufferTexture2D,
    unix_glFramebufferTexture2DEXT,
    unix_glFramebufferTexture3D,
    unix_glFramebufferTexture3DEXT,
    unix_glFramebufferTextureARB,
    unix_glFramebufferTextureEXT,
    unix_glFramebufferTextureFaceARB,
    unix_glFramebufferTextureFaceEXT,
    unix_glFramebufferTextureLayer,
    unix_glFramebufferTextureLayerARB,
    unix_glFramebufferTextureLayerEXT,
    unix_glFramebufferTextureMultiviewOVR,
    unix_glFreeObjectBufferATI,
    unix_glFrustumfOES,
    unix_glFrustumxOES,
    unix_glGenAsyncMarkersSGIX,
    unix_glGenBuffers,
    unix_glGenBuffersARB,
    unix_glGenFencesAPPLE,
    unix_glGenFencesNV,
    unix_glGenFragmentShadersATI,
    unix_glGenFramebuffers,
    unix_glGenFramebuffersEXT,
    unix_glGenNamesAMD,
    unix_glGenOcclusionQueriesNV,
    unix_glGenPathsNV,
    unix_glGenPerfMonitorsAMD,
    unix_glGenProgramPipelines,
    unix_glGenProgramsARB,
    unix_glGenProgramsNV,
    unix_glGenQueries,
    unix_glGenQueriesARB,
    unix_glGenQueryResourceTagNV,
    unix_glGenRenderbuffers,
    unix_glGenRenderbuffersEXT,
    unix_glGenSamplers,
    unix_glGenSemaphoresEXT,
    unix_glGenSymbolsEXT,
    unix_glGenTexturesEXT,
    unix_glGenTransformFeedbacks,
    unix_glGenTransformFeedbacksNV,
    unix_glGenVertexArrays,
    unix_glGenVertexArraysAPPLE,
    unix_glGenVertexShadersEXT,
    unix_glGenerateMipmap,
    unix_glGenerateMipmapEXT,
    unix_glGenerateMultiTexMipmapEXT,
    unix_glGenerateTextureMipmap,
    unix_glGenerateTextureMipmapEXT,
    unix_glGetActiveAtomicCounterBufferiv,
    unix_glGetActiveAttrib,
    unix_glGetActiveAttribARB,
    unix_glGetActiveSubroutineName,
    unix_glGetActiveSubroutineUniformName,
    unix_glGetActiveSubroutineUniformiv,
    unix_glGetActiveUniform,
    unix_glGetActiveUniformARB,
    unix_glGetActiveUniformBlockName,
    unix_glGetActiveUniformBlockiv,
    unix_glGetActiveUniformName,
    unix_glGetActiveUniformsiv,
    unix_glGetActiveVaryingNV,
    unix_glGetArrayObjectfvATI,
    unix_glGetArrayObjectivATI,
    unix_glGetAttachedObjectsARB,
    unix_glGetAttachedShaders,
    unix_glGetAttribLocation,
    unix_glGetAttribLocationARB,
    unix_glGetBooleanIndexedvEXT,
    unix_glGetBooleani_v,
    unix_glGetBufferParameteri64v,
    unix_glGetBufferParameteriv,
    unix_glGetBufferParameterivARB,
    unix_glGetBufferParameterui64vNV,
    unix_glGetBufferPointerv,
    unix_glGetBufferPointervARB,
    unix_glGetBufferSubData,
    unix_glGetBufferSubDataARB,
    unix_glGetClipPlanefOES,
    unix_glGetClipPlanexOES,
    unix_glGetColorTable,
    unix_glGetColorTableEXT,
    unix_glGetColorTableParameterfv,
    unix_glGetColorTableParameterfvEXT,
    unix_glGetColorTableParameterfvSGI,
    unix_glGetColorTableParameteriv,
    unix_glGetColorTableParameterivEXT,
    unix_glGetColorTableParameterivSGI,
    unix_glGetColorTableSGI,
    unix_glGetCombinerInputParameterfvNV,
    unix_glGetCombinerInputParameterivNV,
    unix_glGetCombinerOutputParameterfvNV,
    unix_glGetCombinerOutputParameterivNV,
    unix_glGetCombinerStageParameterfvNV,
    unix_glGetCommandHeaderNV,
    unix_glGetCompressedMultiTexImageEXT,
    unix_glGetCompressedTexImage,
    unix_glGetCompressedTexImageARB,
    unix_glGetCompressedTextureImage,
    unix_glGetCompressedTextureImageEXT,
    unix_glGetCompressedTextureSubImage,
    unix_glGetConvolutionFilter,
    unix_glGetConvolutionFilterEXT,
    unix_glGetConvolutionParameterfv,
    unix_glGetConvolutionParameterfvEXT,
    unix_glGetConvolutionParameteriv,
    unix_glGetConvolutionParameterivEXT,
    unix_glGetConvolutionParameterxvOES,
    unix_glGetCoverageModulationTableNV,
    unix_glGetDebugMessageLog,
    unix_glGetDebugMessageLogAMD,
    unix_glGetDebugMessageLogARB,
    unix_glGetDetailTexFuncSGIS,
    unix_glGetDoubleIndexedvEXT,
    unix_glGetDoublei_v,
    unix_glGetDoublei_vEXT,
    unix_glGetFenceivNV,
    unix_glGetFinalCombinerInputParameterfvNV,
    unix_glGetFinalCombinerInputParameterivNV,
    unix_glGetFirstPerfQueryIdINTEL,
    unix_glGetFixedvOES,
    unix_glGetFloatIndexedvEXT,
    unix_glGetFloati_v,
    unix_glGetFloati_vEXT,
    unix_glGetFogFuncSGIS,
    unix_glGetFragDataIndex,
    unix_glGetFragDataLocation,
    unix_glGetFragDataLocationEXT,
    unix_glGetFragmentLightfvSGIX,
    unix_glGetFragmentLightivSGIX,
    unix_glGetFragmentMaterialfvSGIX,
    unix_glGetFragmentMaterialivSGIX,
    unix_glGetFramebufferAttachmentParameteriv,
    unix_glGetFramebufferAttachmentParameterivEXT,
    unix_glGetFramebufferParameterfvAMD,
    unix_glGetFramebufferParameteriv,
    unix_glGetFramebufferParameterivEXT,
    unix_glGetFramebufferParameterivMESA,
    unix_glGetGraphicsResetStatus,
    unix_glGetGraphicsResetStatusARB,
    unix_glGetHandleARB,
    unix_glGetHistogram,
    unix_glGetHistogramEXT,
    unix_glGetHistogramParameterfv,
    unix_glGetHistogramParameterfvEXT,
    unix_glGetHistogramParameteriv,
    unix_glGetHistogramParameterivEXT,
    unix_glGetHistogramParameterxvOES,
    unix_glGetImageHandleARB,
    unix_glGetImageHandleNV,
    unix_glGetImageTransformParameterfvHP,
    unix_glGetImageTransformParameterivHP,
    unix_glGetInfoLogARB,
    unix_glGetInstrumentsSGIX,
    unix_glGetInteger64i_v,
    unix_glGetInteger64v,
    unix_glGetIntegerIndexedvEXT,
    unix_glGetIntegeri_v,
    unix_glGetIntegerui64i_vNV,
    unix_glGetIntegerui64vNV,
    unix_glGetInternalformatSampleivNV,
    unix_glGetInternalformati64v,
    unix_glGetInternalformativ,
    unix_glGetInvariantBooleanvEXT,
    unix_glGetInvariantFloatvEXT,
    unix_glGetInvariantIntegervEXT,
    unix_glGetLightxOES,
    unix_glGetListParameterfvSGIX,
    unix_glGetListParameterivSGIX,
    unix_glGetLocalConstantBooleanvEXT,
    unix_glGetLocalConstantFloatvEXT,
    unix_glGetLocalConstantIntegervEXT,
    unix_glGetMapAttribParameterfvNV,
    unix_glGetMapAttribParameterivNV,
    unix_glGetMapControlPointsNV,
    unix_glGetMapParameterfvNV,
    unix_glGetMapParameterivNV,
    unix_glGetMapxvOES,
    unix_glGetMaterialxOES,
    unix_glGetMemoryObjectDetachedResourcesuivNV,
    unix_glGetMemoryObjectParameterivEXT,
    unix_glGetMinmax,
    unix_glGetMinmaxEXT,
    unix_glGetMinmaxParameterfv,
    unix_glGetMinmaxParameterfvEXT,
    unix_glGetMinmaxParameteriv,
    unix_glGetMinmaxParameterivEXT,
    unix_glGetMultiTexEnvfvEXT,
    unix_glGetMultiTexEnvivEXT,
    unix_glGetMultiTexGendvEXT,
    unix_glGetMultiTexGenfvEXT,
    unix_glGetMultiTexGenivEXT,
    unix_glGetMultiTexImageEXT,
    unix_glGetMultiTexLevelParameterfvEXT,
    unix_glGetMultiTexLevelParameterivEXT,
    unix_glGetMultiTexParameterIivEXT,
    unix_glGetMultiTexParameterIuivEXT,
    unix_glGetMultiTexParameterfvEXT,
    unix_glGetMultiTexParameterivEXT,
    unix_glGetMultisamplefv,
    unix_glGetMultisamplefvNV,
    unix_glGetNamedBufferParameteri64v,
    unix_glGetNamedBufferParameteriv,
    unix_glGetNamedBufferParameterivEXT,
    unix_glGetNamedBufferParameterui64vNV,
    unix_glGetNamedBufferPointerv,
    unix_glGetNamedBufferPointervEXT,
    unix_glGetNamedBufferSubData,
    unix_glGetNamedBufferSubDataEXT,
    unix_glGetNamedFramebufferAttachmentParameteriv,
    unix_glGetNamedFramebufferAttachmentParameterivEXT,
    unix_glGetNamedFramebufferParameterfvAMD,
    unix_glGetNamedFramebufferParameteriv,
    unix_glGetNamedFramebufferParameterivEXT,
    unix_glGetNamedProgramLocalParameterIivEXT,
    unix_glGetNamedProgramLocalParameterIuivEXT,
    unix_glGetNamedProgramLocalParameterdvEXT,
    unix_glGetNamedProgramLocalParameterfvEXT,
    unix_glGetNamedProgramStringEXT,
    unix_glGetNamedProgramivEXT,
    unix_glGetNamedRenderbufferParameteriv,
    unix_glGetNamedRenderbufferParameterivEXT,
    unix_glGetNamedStringARB,
    unix_glGetNamedStringivARB,
    unix_glGetNextPerfQueryIdINTEL,
    unix_glGetObjectBufferfvATI,
    unix_glGetObjectBufferivATI,
    unix_glGetObjectLabel,
    unix_glGetObjectLabelEXT,
    unix_glGetObjectParameterfvARB,
    unix_glGetObjectParameterivAPPLE,
    unix_glGetObjectParameterivARB,
    unix_glGetObjectPtrLabel,
    unix_glGetOcclusionQueryivNV,
    unix_glGetOcclusionQueryuivNV,
    unix_glGetPathColorGenfvNV,
    unix_glGetPathColorGenivNV,
    unix_glGetPathCommandsNV,
    unix_glGetPathCoordsNV,
    unix_glGetPathDashArrayNV,
    unix_glGetPathLengthNV,
    unix_glGetPathMetricRangeNV,
    unix_glGetPathMetricsNV,
    unix_glGetPathParameterfvNV,
    unix_glGetPathParameterivNV,
    unix_glGetPathSpacingNV,
    unix_glGetPathTexGenfvNV,
    unix_glGetPathTexGenivNV,
    unix_glGetPerfCounterInfoINTEL,
    unix_glGetPerfMonitorCounterDataAMD,
    unix_glGetPerfMonitorCounterInfoAMD,
    unix_glGetPerfMonitorCounterStringAMD,
    unix_glGetPerfMonitorCountersAMD,
    unix_glGetPerfMonitorGroupStringAMD,
    unix_glGetPerfMonitorGroupsAMD,
    unix_glGetPerfQueryDataINTEL,
    unix_glGetPerfQueryIdByNameINTEL,
    unix_glGetPerfQueryInfoINTEL,
    unix_glGetPixelMapxv,
    unix_glGetPixelTexGenParameterfvSGIS,
    unix_glGetPixelTexGenParameterivSGIS,
    unix_glGetPixelTransformParameterfvEXT,
    unix_glGetPixelTransformParameterivEXT,
    unix_glGetPointerIndexedvEXT,
    unix_glGetPointeri_vEXT,
    unix_glGetPointervEXT,
    unix_glGetProgramBinary,
    unix_glGetProgramEnvParameterIivNV,
    unix_glGetProgramEnvParameterIuivNV,
    unix_glGetProgramEnvParameterdvARB,
    unix_glGetProgramEnvParameterfvARB,
    unix_glGetProgramInfoLog,
    unix_glGetProgramInterfaceiv,
    unix_glGetProgramLocalParameterIivNV,
    unix_glGetProgramLocalParameterIuivNV,
    unix_glGetProgramLocalParameterdvARB,
    unix_glGetProgramLocalParameterfvARB,
    unix_glGetProgramNamedParameterdvNV,
    unix_glGetProgramNamedParameterfvNV,
    unix_glGetProgramParameterdvNV,
    unix_glGetProgramParameterfvNV,
    unix_glGetProgramPipelineInfoLog,
    unix_glGetProgramPipelineiv,
    unix_glGetProgramResourceIndex,
    unix_glGetProgramResourceLocation,
    unix_glGetProgramResourceLocationIndex,
    unix_glGetProgramResourceName,
    unix_glGetProgramResourcefvNV,
    unix_glGetProgramResourceiv,
    unix_glGetProgramStageiv,
    unix_glGetProgramStringARB,
    unix_glGetProgramStringNV,
    unix_glGetProgramSubroutineParameteruivNV,
    unix_glGetProgramiv,
    unix_glGetProgramivARB,
    unix_glGetProgramivNV,
    unix_glGetQueryBufferObjecti64v,
    unix_glGetQueryBufferObjectiv,
    unix_glGetQueryBufferObjectui64v,
    unix_glGetQueryBufferObjectuiv,
    unix_glGetQueryIndexediv,
    unix_glGetQueryObjecti64v,
    unix_glGetQueryObjecti64vEXT,
    unix_glGetQueryObjectiv,
    unix_glGetQueryObjectivARB,
    unix_glGetQueryObjectui64v,
    unix_glGetQueryObjectui64vEXT,
    unix_glGetQueryObjectuiv,
    unix_glGetQueryObjectuivARB,
    unix_glGetQueryiv,
    unix_glGetQueryivARB,
    unix_glGetRenderbufferParameteriv,
    unix_glGetRenderbufferParameterivEXT,
    unix_glGetSamplerParameterIiv,
    unix_glGetSamplerParameterIuiv,
    unix_glGetSamplerParameterfv,
    unix_glGetSamplerParameteriv,
    unix_glGetSemaphoreParameterui64vEXT,
    unix_glGetSeparableFilter,
    unix_glGetSeparableFilterEXT,
    unix_glGetShaderInfoLog,
    unix_glGetShaderPrecisionFormat,
    unix_glGetShaderSource,
    unix_glGetShaderSourceARB,
    unix_glGetShaderiv,
    unix_glGetShadingRateImagePaletteNV,
    unix_glGetShadingRateSampleLocationivNV,
    unix_glGetSharpenTexFuncSGIS,
    unix_glGetStageIndexNV,
    unix_glGetStringi,
    unix_glGetSubroutineIndex,
    unix_glGetSubroutineUniformLocation,
    unix_glGetSynciv,
    unix_glGetTexBumpParameterfvATI,
    unix_glGetTexBumpParameterivATI,
    unix_glGetTexEnvxvOES,
    unix_glGetTexFilterFuncSGIS,
    unix_glGetTexGenxvOES,
    unix_glGetTexLevelParameterxvOES,
    unix_glGetTexParameterIiv,
    unix_glGetTexParameterIivEXT,
    unix_glGetTexParameterIuiv,
    unix_glGetTexParameterIuivEXT,
    unix_glGetTexParameterPointervAPPLE,
    unix_glGetTexParameterxvOES,
    unix_glGetTextureHandleARB,
    unix_glGetTextureHandleNV,
    unix_glGetTextureImage,
    unix_glGetTextureImageEXT,
    unix_glGetTextureLevelParameterfv,
    unix_glGetTextureLevelParameterfvEXT,
    unix_glGetTextureLevelParameteriv,
    unix_glGetTextureLevelParameterivEXT,
    unix_glGetTextureParameterIiv,
    unix_glGetTextureParameterIivEXT,
    unix_glGetTextureParameterIuiv,
    unix_glGetTextureParameterIuivEXT,
    unix_glGetTextureParameterfv,
    unix_glGetTextureParameterfvEXT,
    unix_glGetTextureParameteriv,
    unix_glGetTextureParameterivEXT,
    unix_glGetTextureSamplerHandleARB,
    unix_glGetTextureSamplerHandleNV,
    unix_glGetTextureSubImage,
    unix_glGetTrackMatrixivNV,
    unix_glGetTransformFeedbackVarying,
    unix_glGetTransformFeedbackVaryingEXT,
    unix_glGetTransformFeedbackVaryingNV,
    unix_glGetTransformFeedbacki64_v,
    unix_glGetTransformFeedbacki_v,
    unix_glGetTransformFeedbackiv,
    unix_glGetUniformBlockIndex,
    unix_glGetUniformBufferSizeEXT,
    unix_glGetUniformIndices,
    unix_glGetUniformLocation,
    unix_glGetUniformLocationARB,
    unix_glGetUniformOffsetEXT,
    unix_glGetUniformSubroutineuiv,
    unix_glGetUniformdv,
    unix_glGetUniformfv,
    unix_glGetUniformfvARB,
    unix_glGetUniformi64vARB,
    unix_glGetUniformi64vNV,
    unix_glGetUniformiv,
    unix_glGetUniformivARB,
    unix_glGetUniformui64vARB,
    unix_glGetUniformui64vNV,
    unix_glGetUniformuiv,
    unix_glGetUniformuivEXT,
    unix_glGetUnsignedBytei_vEXT,
    unix_glGetUnsignedBytevEXT,
    unix_glGetVariantArrayObjectfvATI,
    unix_glGetVariantArrayObjectivATI,
    unix_glGetVariantBooleanvEXT,
    unix_glGetVariantFloatvEXT,
    unix_glGetVariantIntegervEXT,
    unix_glGetVariantPointervEXT,
    unix_glGetVaryingLocationNV,
    unix_glGetVertexArrayIndexed64iv,
    unix_glGetVertexArrayIndexediv,
    unix_glGetVertexArrayIntegeri_vEXT,
    unix_glGetVertexArrayIntegervEXT,
    unix_glGetVertexArrayPointeri_vEXT,
    unix_glGetVertexArrayPointervEXT,
    unix_glGetVertexArrayiv,
    unix_glGetVertexAttribArrayObjectfvATI,
    unix_glGetVertexAttribArrayObjectivATI,
    unix_glGetVertexAttribIiv,
    unix_glGetVertexAttribIivEXT,
    unix_glGetVertexAttribIuiv,
    unix_glGetVertexAttribIuivEXT,
    unix_glGetVertexAttribLdv,
    unix_glGetVertexAttribLdvEXT,
    unix_glGetVertexAttribLi64vNV,
    unix_glGetVertexAttribLui64vARB,
    unix_glGetVertexAttribLui64vNV,
    unix_glGetVertexAttribPointerv,
    unix_glGetVertexAttribPointervARB,
    unix_glGetVertexAttribPointervNV,
    unix_glGetVertexAttribdv,
    unix_glGetVertexAttribdvARB,
    unix_glGetVertexAttribdvNV,
    unix_glGetVertexAttribfv,
    unix_glGetVertexAttribfvARB,
    unix_glGetVertexAttribfvNV,
    unix_glGetVertexAttribiv,
    unix_glGetVertexAttribivARB,
    unix_glGetVertexAttribivNV,
    unix_glGetVideoCaptureStreamdvNV,
    unix_glGetVideoCaptureStreamfvNV,
    unix_glGetVideoCaptureStreamivNV,
    unix_glGetVideoCaptureivNV,
    unix_glGetVideoi64vNV,
    unix_glGetVideoivNV,
    unix_glGetVideoui64vNV,
    unix_glGetVideouivNV,
    unix_glGetVkProcAddrNV,
    unix_glGetnColorTable,
    unix_glGetnColorTableARB,
    unix_glGetnCompressedTexImage,
    unix_glGetnCompressedTexImageARB,
    unix_glGetnConvolutionFilter,
    unix_glGetnConvolutionFilterARB,
    unix_glGetnHistogram,
    unix_glGetnHistogramARB,
    unix_glGetnMapdv,
    unix_glGetnMapdvARB,
    unix_glGetnMapfv,
    unix_glGetnMapfvARB,
    unix_glGetnMapiv,
    unix_glGetnMapivARB,
    unix_glGetnMinmax,
    unix_glGetnMinmaxARB,
    unix_glGetnPixelMapfv,
    unix_glGetnPixelMapfvARB,
    unix_glGetnPixelMapuiv,
    unix_glGetnPixelMapuivARB,
    unix_glGetnPixelMapusv,
    unix_glGetnPixelMapusvARB,
    unix_glGetnPolygonStipple,
    unix_glGetnPolygonStippleARB,
    unix_glGetnSeparableFilter,
    unix_glGetnSeparableFilterARB,
    unix_glGetnTexImage,
    unix_glGetnTexImageARB,
    unix_glGetnUniformdv,
    unix_glGetnUniformdvARB,
    unix_glGetnUniformfv,
    unix_glGetnUniformfvARB,
    unix_glGetnUniformi64vARB,
    unix_glGetnUniformiv,
    unix_glGetnUniformivARB,
    unix_glGetnUniformui64vARB,
    unix_glGetnUniformuiv,
    unix_glGetnUniformuivARB,
    unix_glGlobalAlphaFactorbSUN,
    unix_glGlobalAlphaFactordSUN,
    unix_glGlobalAlphaFactorfSUN,
    unix_glGlobalAlphaFactoriSUN,
    unix_glGlobalAlphaFactorsSUN,
    unix_glGlobalAlphaFactorubSUN,
    unix_glGlobalAlphaFactoruiSUN,
    unix_glGlobalAlphaFactorusSUN,
    unix_glHintPGI,
    unix_glHistogram,
    unix_glHistogramEXT,
    unix_glIglooInterfaceSGIX,
    unix_glImageTransformParameterfHP,
    unix_glImageTransformParameterfvHP,
    unix_glImageTransformParameteriHP,
    unix_glImageTransformParameterivHP,
    unix_glImportMemoryFdEXT,
    unix_glImportMemoryWin32HandleEXT,
    unix_glImportMemoryWin32NameEXT,
    unix_glImportSemaphoreFdEXT,
    unix_glImportSemaphoreWin32HandleEXT,
    unix_glImportSemaphoreWin32NameEXT,
    unix_glImportSyncEXT,
    unix_glIndexFormatNV,
    unix_glIndexFuncEXT,
    unix_glIndexMaterialEXT,
    unix_glIndexPointerEXT,
    unix_glIndexPointerListIBM,
    unix_glIndexxOES,
    unix_glIndexxvOES,
    unix_glInsertComponentEXT,
    unix_glInsertEventMarkerEXT,
    unix_glInstrumentsBufferSGIX,
    unix_glInterpolatePathsNV,
    unix_glInvalidateBufferData,
    unix_glInvalidateBufferSubData,
    unix_glInvalidateFramebuffer,
    unix_glInvalidateNamedFramebufferData,
    unix_glInvalidateNamedFramebufferSubData,
    unix_glInvalidateSubFramebuffer,
    unix_glInvalidateTexImage,
    unix_glInvalidateTexSubImage,
    unix_glIsAsyncMarkerSGIX,
    unix_glIsBuffer,
    unix_glIsBufferARB,
    unix_glIsBufferResidentNV,
    unix_glIsCommandListNV,
    unix_glIsEnabledIndexedEXT,
    unix_glIsEnabledi,
    unix_glIsFenceAPPLE,
    unix_glIsFenceNV,
    unix_glIsFramebuffer,
    unix_glIsFramebufferEXT,
    unix_glIsImageHandleResidentARB,
    unix_glIsImageHandleResidentNV,
    unix_glIsMemoryObjectEXT,
    unix_glIsNameAMD,
    unix_glIsNamedBufferResidentNV,
    unix_glIsNamedStringARB,
    unix_glIsObjectBufferATI,
    unix_glIsOcclusionQueryNV,
    unix_glIsPathNV,
    unix_glIsPointInFillPathNV,
    unix_glIsPointInStrokePathNV,
    unix_glIsProgram,
    unix_glIsProgramARB,
    unix_glIsProgramNV,
    unix_glIsProgramPipeline,
    unix_glIsQuery,
    unix_glIsQueryARB,
    unix_glIsRenderbuffer,
    unix_glIsRenderbufferEXT,
    unix_glIsSampler,
    unix_glIsSemaphoreEXT,
    unix_glIsShader,
    unix_glIsStateNV,
    unix_glIsSync,
    unix_glIsTextureEXT,
    unix_glIsTextureHandleResidentARB,
    unix_glIsTextureHandleResidentNV,
    unix_glIsTransformFeedback,
    unix_glIsTransformFeedbackNV,
    unix_glIsVariantEnabledEXT,
    unix_glIsVertexArray,
    unix_glIsVertexArrayAPPLE,
    unix_glIsVertexAttribEnabledAPPLE,
    unix_glLGPUCopyImageSubDataNVX,
    unix_glLGPUInterlockNVX,
    unix_glLGPUNamedBufferSubDataNVX,
    unix_glLabelObjectEXT,
    unix_glLightEnviSGIX,
    unix_glLightModelxOES,
    unix_glLightModelxvOES,
    unix_glLightxOES,
    unix_glLightxvOES,
    unix_glLineWidthxOES,
    unix_glLinkProgram,
    unix_glLinkProgramARB,
    unix_glListDrawCommandsStatesClientNV,
    unix_glListParameterfSGIX,
    unix_glListParameterfvSGIX,
    unix_glListParameteriSGIX,
    unix_glListParameterivSGIX,
    unix_glLoadIdentityDeformationMapSGIX,
    unix_glLoadMatrixxOES,
    unix_glLoadProgramNV,
    unix_glLoadTransposeMatrixd,
    unix_glLoadTransposeMatrixdARB,
    unix_glLoadTransposeMatrixf,
    unix_glLoadTransposeMatrixfARB,
    unix_glLoadTransposeMatrixxOES,
    unix_glLockArraysEXT,
    unix_glMTexCoord2fSGIS,
    unix_glMTexCoord2fvSGIS,
    unix_glMakeBufferNonResidentNV,
    unix_glMakeBufferResidentNV,
    unix_glMakeImageHandleNonResidentARB,
    unix_glMakeImageHandleNonResidentNV,
    unix_glMakeImageHandleResidentARB,
    unix_glMakeImageHandleResidentNV,
    unix_glMakeNamedBufferNonResidentNV,
    unix_glMakeNamedBufferResidentNV,
    unix_glMakeTextureHandleNonResidentARB,
    unix_glMakeTextureHandleNonResidentNV,
    unix_glMakeTextureHandleResidentARB,
    unix_glMakeTextureHandleResidentNV,
    unix_glMap1xOES,
    unix_glMap2xOES,
    unix_glMapBuffer,
    unix_glMapBufferARB,
    unix_glMapBufferRange,
    unix_glMapControlPointsNV,
    unix_glMapGrid1xOES,
    unix_glMapGrid2xOES,
    unix_glMapNamedBuffer,
    unix_glMapNamedBufferEXT,
    unix_glMapNamedBufferRange,
    unix_glMapNamedBufferRangeEXT,
    unix_glMapObjectBufferATI,
    unix_glMapParameterfvNV,
    unix_glMapParameterivNV,
    unix_glMapTexture2DINTEL,
    unix_glMapVertexAttrib1dAPPLE,
    unix_glMapVertexAttrib1fAPPLE,
    unix_glMapVertexAttrib2dAPPLE,
    unix_glMapVertexAttrib2fAPPLE,
    unix_glMaterialxOES,
    unix_glMaterialxvOES,
    unix_glMatrixFrustumEXT,
    unix_glMatrixIndexPointerARB,
    unix_glMatrixIndexubvARB,
    unix_glMatrixIndexuivARB,
    unix_glMatrixIndexusvARB,
    unix_glMatrixLoad3x2fNV,
    unix_glMatrixLoad3x3fNV,
    unix_glMatrixLoadIdentityEXT,
    unix_glMatrixLoadTranspose3x3fNV,
    unix_glMatrixLoadTransposedEXT,
    unix_glMatrixLoadTransposefEXT,
    unix_glMatrixLoaddEXT,
    unix_glMatrixLoadfEXT,
    unix_glMatrixMult3x2fNV,
    unix_glMatrixMult3x3fNV,
    unix_glMatrixMultTranspose3x3fNV,
    unix_glMatrixMultTransposedEXT,
    unix_glMatrixMultTransposefEXT,
    unix_glMatrixMultdEXT,
    unix_glMatrixMultfEXT,
    unix_glMatrixOrthoEXT,
    unix_glMatrixPopEXT,
    unix_glMatrixPushEXT,
    unix_glMatrixRotatedEXT,
    unix_glMatrixRotatefEXT,
    unix_glMatrixScaledEXT,
    unix_glMatrixScalefEXT,
    unix_glMatrixTranslatedEXT,
    unix_glMatrixTranslatefEXT,
    unix_glMaxShaderCompilerThreadsARB,
    unix_glMaxShaderCompilerThreadsKHR,
    unix_glMemoryBarrier,
    unix_glMemoryBarrierByRegion,
    unix_glMemoryBarrierEXT,
    unix_glMemoryObjectParameterivEXT,
    unix_glMinSampleShading,
    unix_glMinSampleShadingARB,
    unix_glMinmax,
    unix_glMinmaxEXT,
    unix_glMultMatrixxOES,
    unix_glMultTransposeMatrixd,
    unix_glMultTransposeMatrixdARB,
    unix_glMultTransposeMatrixf,
    unix_glMultTransposeMatrixfARB,
    unix_glMultTransposeMatrixxOES,
    unix_glMultiDrawArrays,
    unix_glMultiDrawArraysEXT,
    unix_glMultiDrawArraysIndirect,
    unix_glMultiDrawArraysIndirectAMD,
    unix_glMultiDrawArraysIndirectBindlessCountNV,
    unix_glMultiDrawArraysIndirectBindlessNV,
    unix_glMultiDrawArraysIndirectCount,
    unix_glMultiDrawArraysIndirectCountARB,
    unix_glMultiDrawElementArrayAPPLE,
    unix_glMultiDrawElements,
    unix_glMultiDrawElementsBaseVertex,
    unix_glMultiDrawElementsEXT,
    unix_glMultiDrawElementsIndirect,
    unix_glMultiDrawElementsIndirectAMD,
    unix_glMultiDrawElementsIndirectBindlessCountNV,
    unix_glMultiDrawElementsIndirectBindlessNV,
    unix_glMultiDrawElementsIndirectCount,
    unix_glMultiDrawElementsIndirectCountARB,
    unix_glMultiDrawMeshTasksIndirectCountNV,
    unix_glMultiDrawMeshTasksIndirectNV,
    unix_glMultiDrawRangeElementArrayAPPLE,
    unix_glMultiModeDrawArraysIBM,
    unix_glMultiModeDrawElementsIBM,
    unix_glMultiTexBufferEXT,
    unix_glMultiTexCoord1bOES,
    unix_glMultiTexCoord1bvOES,
    unix_glMultiTexCoord1d,
    unix_glMultiTexCoord1dARB,
    unix_glMultiTexCoord1dSGIS,
    unix_glMultiTexCoord1dv,
    unix_glMultiTexCoord1dvARB,
    unix_glMultiTexCoord1dvSGIS,
    unix_glMultiTexCoord1f,
    unix_glMultiTexCoord1fARB,
    unix_glMultiTexCoord1fSGIS,
    unix_glMultiTexCoord1fv,
    unix_glMultiTexCoord1fvARB,
    unix_glMultiTexCoord1fvSGIS,
    unix_glMultiTexCoord1hNV,
    unix_glMultiTexCoord1hvNV,
    unix_glMultiTexCoord1i,
    unix_glMultiTexCoord1iARB,
    unix_glMultiTexCoord1iSGIS,
    unix_glMultiTexCoord1iv,
    unix_glMultiTexCoord1ivARB,
    unix_glMultiTexCoord1ivSGIS,
    unix_glMultiTexCoord1s,
    unix_glMultiTexCoord1sARB,
    unix_glMultiTexCoord1sSGIS,
    unix_glMultiTexCoord1sv,
    unix_glMultiTexCoord1svARB,
    unix_glMultiTexCoord1svSGIS,
    unix_glMultiTexCoord1xOES,
    unix_glMultiTexCoord1xvOES,
    unix_glMultiTexCoord2bOES,
    unix_glMultiTexCoord2bvOES,
    unix_glMultiTexCoord2d,
    unix_glMultiTexCoord2dARB,
    unix_glMultiTexCoord2dSGIS,
    unix_glMultiTexCoord2dv,
    unix_glMultiTexCoord2dvARB,
    unix_glMultiTexCoord2dvSGIS,
    unix_glMultiTexCoord2f,
    unix_glMultiTexCoord2fARB,
    unix_glMultiTexCoord2fSGIS,
    unix_glMultiTexCoord2fv,
    unix_glMultiTexCoord2fvARB,
    unix_glMultiTexCoord2fvSGIS,
    unix_glMultiTexCoord2hNV,
    unix_glMultiTexCoord2hvNV,
    unix_glMultiTexCoord2i,
    unix_glMultiTexCoord2iARB,
    unix_glMultiTexCoord2iSGIS,
    unix_glMultiTexCoord2iv,
    unix_glMultiTexCoord2ivARB,
    unix_glMultiTexCoord2ivSGIS,
    unix_glMultiTexCoord2s,
    unix_glMultiTexCoord2sARB,
    unix_glMultiTexCoord2sSGIS,
    unix_glMultiTexCoord2sv,
    unix_glMultiTexCoord2svARB,
    unix_glMultiTexCoord2svSGIS,
    unix_glMultiTexCoord2xOES,
    unix_glMultiTexCoord2xvOES,
    unix_glMultiTexCoord3bOES,
    unix_glMultiTexCoord3bvOES,
    unix_glMultiTexCoord3d,
    unix_glMultiTexCoord3dARB,
    unix_glMultiTexCoord3dSGIS,
    unix_glMultiTexCoord3dv,
    unix_glMultiTexCoord3dvARB,
    unix_glMultiTexCoord3dvSGIS,
    unix_glMultiTexCoord3f,
    unix_glMultiTexCoord3fARB,
    unix_glMultiTexCoord3fSGIS,
    unix_glMultiTexCoord3fv,
    unix_glMultiTexCoord3fvARB,
    unix_glMultiTexCoord3fvSGIS,
    unix_glMultiTexCoord3hNV,
    unix_glMultiTexCoord3hvNV,
    unix_glMultiTexCoord3i,
    unix_glMultiTexCoord3iARB,
    unix_glMultiTexCoord3iSGIS,
    unix_glMultiTexCoord3iv,
    unix_glMultiTexCoord3ivARB,
    unix_glMultiTexCoord3ivSGIS,
    unix_glMultiTexCoord3s,
    unix_glMultiTexCoord3sARB,
    unix_glMultiTexCoord3sSGIS,
    unix_glMultiTexCoord3sv,
    unix_glMultiTexCoord3svARB,
    unix_glMultiTexCoord3svSGIS,
    unix_glMultiTexCoord3xOES,
    unix_glMultiTexCoord3xvOES,
    unix_glMultiTexCoord4bOES,
    unix_glMultiTexCoord4bvOES,
    unix_glMultiTexCoord4d,
    unix_glMultiTexCoord4dARB,
    unix_glMultiTexCoord4dSGIS,
    unix_glMultiTexCoord4dv,
    unix_glMultiTexCoord4dvARB,
    unix_glMultiTexCoord4dvSGIS,
    unix_glMultiTexCoord4f,
    unix_glMultiTexCoord4fARB,
    unix_glMultiTexCoord4fSGIS,
    unix_glMultiTexCoord4fv,
    unix_glMultiTexCoord4fvARB,
    unix_glMultiTexCoord4fvSGIS,
    unix_glMultiTexCoord4hNV,
    unix_glMultiTexCoord4hvNV,
    unix_glMultiTexCoord4i,
    unix_glMultiTexCoord4iARB,
    unix_glMultiTexCoord4iSGIS,
    unix_glMultiTexCoord4iv,
    unix_glMultiTexCoord4ivARB,
    unix_glMultiTexCoord4ivSGIS,
    unix_glMultiTexCoord4s,
    unix_glMultiTexCoord4sARB,
    unix_glMultiTexCoord4sSGIS,
    unix_glMultiTexCoord4sv,
    unix_glMultiTexCoord4svARB,
    unix_glMultiTexCoord4svSGIS,
    unix_glMultiTexCoord4xOES,
    unix_glMultiTexCoord4xvOES,
    unix_glMultiTexCoordP1ui,
    unix_glMultiTexCoordP1uiv,
    unix_glMultiTexCoordP2ui,
    unix_glMultiTexCoordP2uiv,
    unix_glMultiTexCoordP3ui,
    unix_glMultiTexCoordP3uiv,
    unix_glMultiTexCoordP4ui,
    unix_glMultiTexCoordP4uiv,
    unix_glMultiTexCoordPointerEXT,
    unix_glMultiTexCoordPointerSGIS,
    unix_glMultiTexEnvfEXT,
    unix_glMultiTexEnvfvEXT,
    unix_glMultiTexEnviEXT,
    unix_glMultiTexEnvivEXT,
    unix_glMultiTexGendEXT,
    unix_glMultiTexGendvEXT,
    unix_glMultiTexGenfEXT,
    unix_glMultiTexGenfvEXT,
    unix_glMultiTexGeniEXT,
    unix_glMultiTexGenivEXT,
    unix_glMultiTexImage1DEXT,
    unix_glMultiTexImage2DEXT,
    unix_glMultiTexImage3DEXT,
    unix_glMultiTexParameterIivEXT,
    unix_glMultiTexParameterIuivEXT,
    unix_glMultiTexParameterfEXT,
    unix_glMultiTexParameterfvEXT,
    unix_glMultiTexParameteriEXT,
    unix_glMultiTexParameterivEXT,
    unix_glMultiTexRenderbufferEXT,
    unix_glMultiTexSubImage1DEXT,
    unix_glMultiTexSubImage2DEXT,
    unix_glMultiTexSubImage3DEXT,
    unix_glMulticastBarrierNV,
    unix_glMulticastBlitFramebufferNV,
    unix_glMulticastBufferSubDataNV,
    unix_glMulticastCopyBufferSubDataNV,
    unix_glMulticastCopyImageSubDataNV,
    unix_glMulticastFramebufferSampleLocationsfvNV,
    unix_glMulticastGetQueryObjecti64vNV,
    unix_glMulticastGetQueryObjectivNV,
    unix_glMulticastGetQueryObjectui64vNV,
    unix_glMulticastGetQueryObjectuivNV,
    unix_glMulticastScissorArrayvNVX,
    unix_glMulticastViewportArrayvNVX,
    unix_glMulticastViewportPositionWScaleNVX,
    unix_glMulticastWaitSyncNV,
    unix_glNamedBufferAttachMemoryNV,
    unix_glNamedBufferData,
    unix_glNamedBufferDataEXT,
    unix_glNamedBufferPageCommitmentARB,
    unix_glNamedBufferPageCommitmentEXT,
    unix_glNamedBufferStorage,
    unix_glNamedBufferStorageEXT,
    unix_glNamedBufferStorageExternalEXT,
    unix_glNamedBufferStorageMemEXT,
    unix_glNamedBufferSubData,
    unix_glNamedBufferSubDataEXT,
    unix_glNamedCopyBufferSubDataEXT,
    unix_glNamedFramebufferDrawBuffer,
    unix_glNamedFramebufferDrawBuffers,
    unix_glNamedFramebufferParameteri,
    unix_glNamedFramebufferParameteriEXT,
    unix_glNamedFramebufferReadBuffer,
    unix_glNamedFramebufferRenderbuffer,
    unix_glNamedFramebufferRenderbufferEXT,
    unix_glNamedFramebufferSampleLocationsfvARB,
    unix_glNamedFramebufferSampleLocationsfvNV,
    unix_glNamedFramebufferSamplePositionsfvAMD,
    unix_glNamedFramebufferTexture,
    unix_glNamedFramebufferTexture1DEXT,
    unix_glNamedFramebufferTexture2DEXT,
    unix_glNamedFramebufferTexture3DEXT,
    unix_glNamedFramebufferTextureEXT,
    unix_glNamedFramebufferTextureFaceEXT,
    unix_glNamedFramebufferTextureLayer,
    unix_glNamedFramebufferTextureLayerEXT,
    unix_glNamedProgramLocalParameter4dEXT,
    unix_glNamedProgramLocalParameter4dvEXT,
    unix_glNamedProgramLocalParameter4fEXT,
    unix_glNamedProgramLocalParameter4fvEXT,
    unix_glNamedProgramLocalParameterI4iEXT,
    unix_glNamedProgramLocalParameterI4ivEXT,
    unix_glNamedProgramLocalParameterI4uiEXT,
    unix_glNamedProgramLocalParameterI4uivEXT,
    unix_glNamedProgramLocalParameters4fvEXT,
    unix_glNamedProgramLocalParametersI4ivEXT,
    unix_glNamedProgramLocalParametersI4uivEXT,
    unix_glNamedProgramStringEXT,
    unix_glNamedRenderbufferStorage,
    unix_glNamedRenderbufferStorageEXT,
    unix_glNamedRenderbufferStorageMultisample,
    unix_glNamedRenderbufferStorageMultisampleAdvancedAMD,
    unix_glNamedRenderbufferStorageMultisampleCoverageEXT,
    unix_glNamedRenderbufferStorageMultisampleEXT,
    unix_glNamedStringARB,
    unix_glNewBufferRegion,
    unix_glNewObjectBufferATI,
    unix_glNormal3fVertex3fSUN,
    unix_glNormal3fVertex3fvSUN,
    unix_glNormal3hNV,
    unix_glNormal3hvNV,
    unix_glNormal3xOES,
    unix_glNormal3xvOES,
    unix_glNormalFormatNV,
    unix_glNormalP3ui,
    unix_glNormalP3uiv,
    unix_glNormalPointerEXT,
    unix_glNormalPointerListIBM,
    unix_glNormalPointervINTEL,
    unix_glNormalStream3bATI,
    unix_glNormalStream3bvATI,
    unix_glNormalStream3dATI,
    unix_glNormalStream3dvATI,
    unix_glNormalStream3fATI,
    unix_glNormalStream3fvATI,
    unix_glNormalStream3iATI,
    unix_glNormalStream3ivATI,
    unix_glNormalStream3sATI,
    unix_glNormalStream3svATI,
    unix_glObjectLabel,
    unix_glObjectPtrLabel,
    unix_glObjectPurgeableAPPLE,
    unix_glObjectUnpurgeableAPPLE,
    unix_glOrthofOES,
    unix_glOrthoxOES,
    unix_glPNTrianglesfATI,
    unix_glPNTrianglesiATI,
    unix_glPassTexCoordATI,
    unix_glPassThroughxOES,
    unix_glPatchParameterfv,
    unix_glPatchParameteri,
    unix_glPathColorGenNV,
    unix_glPathCommandsNV,
    unix_glPathCoordsNV,
    unix_glPathCoverDepthFuncNV,
    unix_glPathDashArrayNV,
    unix_glPathFogGenNV,
    unix_glPathGlyphIndexArrayNV,
    unix_glPathGlyphIndexRangeNV,
    unix_glPathGlyphRangeNV,
    unix_glPathGlyphsNV,
    unix_glPathMemoryGlyphIndexArrayNV,
    unix_glPathParameterfNV,
    unix_glPathParameterfvNV,
    unix_glPathParameteriNV,
    unix_glPathParameterivNV,
    unix_glPathStencilDepthOffsetNV,
    unix_glPathStencilFuncNV,
    unix_glPathStringNV,
    unix_glPathSubCommandsNV,
    unix_glPathSubCoordsNV,
    unix_glPathTexGenNV,
    unix_glPauseTransformFeedback,
    unix_glPauseTransformFeedbackNV,
    unix_glPixelDataRangeNV,
    unix_glPixelMapx,
    unix_glPixelStorex,
    unix_glPixelTexGenParameterfSGIS,
    unix_glPixelTexGenParameterfvSGIS,
    unix_glPixelTexGenParameteriSGIS,
    unix_glPixelTexGenParameterivSGIS,
    unix_glPixelTexGenSGIX,
    unix_glPixelTransferxOES,
    unix_glPixelTransformParameterfEXT,
    unix_glPixelTransformParameterfvEXT,
    unix_glPixelTransformParameteriEXT,
    unix_glPixelTransformParameterivEXT,
    unix_glPixelZoomxOES,
    unix_glPointAlongPathNV,
    unix_glPointParameterf,
    unix_glPointParameterfARB,
    unix_glPointParameterfEXT,
    unix_glPointParameterfSGIS,
    unix_glPointParameterfv,
    unix_glPointParameterfvARB,
    unix_glPointParameterfvEXT,
    unix_glPointParameterfvSGIS,
    unix_glPointParameteri,
    unix_glPointParameteriNV,
    unix_glPointParameteriv,
    unix_glPointParameterivNV,
    unix_glPointParameterxvOES,
    unix_glPointSizexOES,
    unix_glPollAsyncSGIX,
    unix_glPollInstrumentsSGIX,
    unix_glPolygonOffsetClamp,
    unix_glPolygonOffsetClampEXT,
    unix_glPolygonOffsetEXT,
    unix_glPolygonOffsetxOES,
    unix_glPopDebugGroup,
    unix_glPopGroupMarkerEXT,
    unix_glPresentFrameDualFillNV,
    unix_glPresentFrameKeyedNV,
    unix_glPrimitiveBoundingBoxARB,
    unix_glPrimitiveRestartIndex,
    unix_glPrimitiveRestartIndexNV,
    unix_glPrimitiveRestartNV,
    unix_glPrioritizeTexturesEXT,
    unix_glPrioritizeTexturesxOES,
    unix_glProgramBinary,
    unix_glProgramBufferParametersIivNV,
    unix_glProgramBufferParametersIuivNV,
    unix_glProgramBufferParametersfvNV,
    unix_glProgramEnvParameter4dARB,
    unix_glProgramEnvParameter4dvARB,
    unix_glProgramEnvParameter4fARB,
    unix_glProgramEnvParameter4fvARB,
    unix_glProgramEnvParameterI4iNV,
    unix_glProgramEnvParameterI4ivNV,
    unix_glProgramEnvParameterI4uiNV,
    unix_glProgramEnvParameterI4uivNV,
    unix_glProgramEnvParameters4fvEXT,
    unix_glProgramEnvParametersI4ivNV,
    unix_glProgramEnvParametersI4uivNV,
    unix_glProgramLocalParameter4dARB,
    unix_glProgramLocalParameter4dvARB,
    unix_glProgramLocalParameter4fARB,
    unix_glProgramLocalParameter4fvARB,
    unix_glProgramLocalParameterI4iNV,
    unix_glProgramLocalParameterI4ivNV,
    unix_glProgramLocalParameterI4uiNV,
    unix_glProgramLocalParameterI4uivNV,
    unix_glProgramLocalParameters4fvEXT,
    unix_glProgramLocalParametersI4ivNV,
    unix_glProgramLocalParametersI4uivNV,
    unix_glProgramNamedParameter4dNV,
    unix_glProgramNamedParameter4dvNV,
    unix_glProgramNamedParameter4fNV,
    unix_glProgramNamedParameter4fvNV,
    unix_glProgramParameter4dNV,
    unix_glProgramParameter4dvNV,
    unix_glProgramParameter4fNV,
    unix_glProgramParameter4fvNV,
    unix_glProgramParameteri,
    unix_glProgramParameteriARB,
    unix_glProgramParameteriEXT,
    unix_glProgramParameters4dvNV,
    unix_glProgramParameters4fvNV,
    unix_glProgramPathFragmentInputGenNV,
    unix_glProgramStringARB,
    unix_glProgramSubroutineParametersuivNV,
    unix_glProgramUniform1d,
    unix_glProgramUniform1dEXT,
    unix_glProgramUniform1dv,
    unix_glProgramUniform1dvEXT,
    unix_glProgramUniform1f,
    unix_glProgramUniform1fEXT,
    unix_glProgramUniform1fv,
    unix_glProgramUniform1fvEXT,
    unix_glProgramUniform1i,
    unix_glProgramUniform1i64ARB,
    unix_glProgramUniform1i64NV,
    unix_glProgramUniform1i64vARB,
    unix_glProgramUniform1i64vNV,
    unix_glProgramUniform1iEXT,
    unix_glProgramUniform1iv,
    unix_glProgramUniform1ivEXT,
    unix_glProgramUniform1ui,
    unix_glProgramUniform1ui64ARB,
    unix_glProgramUniform1ui64NV,
    unix_glProgramUniform1ui64vARB,
    unix_glProgramUniform1ui64vNV,
    unix_glProgramUniform1uiEXT,
    unix_glProgramUniform1uiv,
    unix_glProgramUniform1uivEXT,
    unix_glProgramUniform2d,
    unix_glProgramUniform2dEXT,
    unix_glProgramUniform2dv,
    unix_glProgramUniform2dvEXT,
    unix_glProgramUniform2f,
    unix_glProgramUniform2fEXT,
    unix_glProgramUniform2fv,
    unix_glProgramUniform2fvEXT,
    unix_glProgramUniform2i,
    unix_glProgramUniform2i64ARB,
    unix_glProgramUniform2i64NV,
    unix_glProgramUniform2i64vARB,
    unix_glProgramUniform2i64vNV,
    unix_glProgramUniform2iEXT,
    unix_glProgramUniform2iv,
    unix_glProgramUniform2ivEXT,
    unix_glProgramUniform2ui,
    unix_glProgramUniform2ui64ARB,
    unix_glProgramUniform2ui64NV,
    unix_glProgramUniform2ui64vARB,
    unix_glProgramUniform2ui64vNV,
    unix_glProgramUniform2uiEXT,
    unix_glProgramUniform2uiv,
    unix_glProgramUniform2uivEXT,
    unix_glProgramUniform3d,
    unix_glProgramUniform3dEXT,
    unix_glProgramUniform3dv,
    unix_glProgramUniform3dvEXT,
    unix_glProgramUniform3f,
    unix_glProgramUniform3fEXT,
    unix_glProgramUniform3fv,
    unix_glProgramUniform3fvEXT,
    unix_glProgramUniform3i,
    unix_glProgramUniform3i64ARB,
    unix_glProgramUniform3i64NV,
    unix_glProgramUniform3i64vARB,
    unix_glProgramUniform3i64vNV,
    unix_glProgramUniform3iEXT,
    unix_glProgramUniform3iv,
    unix_glProgramUniform3ivEXT,
    unix_glProgramUniform3ui,
    unix_glProgramUniform3ui64ARB,
    unix_glProgramUniform3ui64NV,
    unix_glProgramUniform3ui64vARB,
    unix_glProgramUniform3ui64vNV,
    unix_glProgramUniform3uiEXT,
    unix_glProgramUniform3uiv,
    unix_glProgramUniform3uivEXT,
    unix_glProgramUniform4d,
    unix_glProgramUniform4dEXT,
    unix_glProgramUniform4dv,
    unix_glProgramUniform4dvEXT,
    unix_glProgramUniform4f,
    unix_glProgramUniform4fEXT,
    unix_glProgramUniform4fv,
    unix_glProgramUniform4fvEXT,
    unix_glProgramUniform4i,
    unix_glProgramUniform4i64ARB,
    unix_glProgramUniform4i64NV,
    unix_glProgramUniform4i64vARB,
    unix_glProgramUniform4i64vNV,
    unix_glProgramUniform4iEXT,
    unix_glProgramUniform4iv,
    unix_glProgramUniform4ivEXT,
    unix_glProgramUniform4ui,
    unix_glProgramUniform4ui64ARB,
    unix_glProgramUniform4ui64NV,
    unix_glProgramUniform4ui64vARB,
    unix_glProgramUniform4ui64vNV,
    unix_glProgramUniform4uiEXT,
    unix_glProgramUniform4uiv,
    unix_glProgramUniform4uivEXT,
    unix_glProgramUniformHandleui64ARB,
    unix_glProgramUniformHandleui64NV,
    unix_glProgramUniformHandleui64vARB,
    unix_glProgramUniformHandleui64vNV,
    unix_glProgramUniformMatrix2dv,
    unix_glProgramUniformMatrix2dvEXT,
    unix_glProgramUniformMatrix2fv,
    unix_glProgramUniformMatrix2fvEXT,
    unix_glProgramUniformMatrix2x3dv,
    unix_glProgramUniformMatrix2x3dvEXT,
    unix_glProgramUniformMatrix2x3fv,
    unix_glProgramUniformMatrix2x3fvEXT,
    unix_glProgramUniformMatrix2x4dv,
    unix_glProgramUniformMatrix2x4dvEXT,
    unix_glProgramUniformMatrix2x4fv,
    unix_glProgramUniformMatrix2x4fvEXT,
    unix_glProgramUniformMatrix3dv,
    unix_glProgramUniformMatrix3dvEXT,
    unix_glProgramUniformMatrix3fv,
    unix_glProgramUniformMatrix3fvEXT,
    unix_glProgramUniformMatrix3x2dv,
    unix_glProgramUniformMatrix3x2dvEXT,
    unix_glProgramUniformMatrix3x2fv,
    unix_glProgramUniformMatrix3x2fvEXT,
    unix_glProgramUniformMatrix3x4dv,
    unix_glProgramUniformMatrix3x4dvEXT,
    unix_glProgramUniformMatrix3x4fv,
    unix_glProgramUniformMatrix3x4fvEXT,
    unix_glProgramUniformMatrix4dv,
    unix_glProgramUniformMatrix4dvEXT,
    unix_glProgramUniformMatrix4fv,
    unix_glProgramUniformMatrix4fvEXT,
    unix_glProgramUniformMatrix4x2dv,
    unix_glProgramUniformMatrix4x2dvEXT,
    unix_glProgramUniformMatrix4x2fv,
    unix_glProgramUniformMatrix4x2fvEXT,
    unix_glProgramUniformMatrix4x3dv,
    unix_glProgramUniformMatrix4x3dvEXT,
    unix_glProgramUniformMatrix4x3fv,
    unix_glProgramUniformMatrix4x3fvEXT,
    unix_glProgramUniformui64NV,
    unix_glProgramUniformui64vNV,
    unix_glProgramVertexLimitNV,
    unix_glProvokingVertex,
    unix_glProvokingVertexEXT,
    unix_glPushClientAttribDefaultEXT,
    unix_glPushDebugGroup,
    unix_glPushGroupMarkerEXT,
    unix_glQueryCounter,
    unix_glQueryMatrixxOES,
    unix_glQueryObjectParameteruiAMD,
    unix_glQueryResourceNV,
    unix_glQueryResourceTagNV,
    unix_glRasterPos2xOES,
    unix_glRasterPos2xvOES,
    unix_glRasterPos3xOES,
    unix_glRasterPos3xvOES,
    unix_glRasterPos4xOES,
    unix_glRasterPos4xvOES,
    unix_glRasterSamplesEXT,
    unix_glReadBufferRegion,
    unix_glReadInstrumentsSGIX,
    unix_glReadnPixels,
    unix_glReadnPixelsARB,
    unix_glRectxOES,
    unix_glRectxvOES,
    unix_glReferencePlaneSGIX,
    unix_glReleaseKeyedMutexWin32EXT,
    unix_glReleaseShaderCompiler,
    unix_glRenderGpuMaskNV,
    unix_glRenderbufferStorage,
    unix_glRenderbufferStorageEXT,
    unix_glRenderbufferStorageMultisample,
    unix_glRenderbufferStorageMultisampleAdvancedAMD,
    unix_glRenderbufferStorageMultisampleCoverageNV,
    unix_glRenderbufferStorageMultisampleEXT,
    unix_glReplacementCodePointerSUN,
    unix_glReplacementCodeubSUN,
    unix_glReplacementCodeubvSUN,
    unix_glReplacementCodeuiColor3fVertex3fSUN,
    unix_glReplacementCodeuiColor3fVertex3fvSUN,
    unix_glReplacementCodeuiColor4fNormal3fVertex3fSUN,
    unix_glReplacementCodeuiColor4fNormal3fVertex3fvSUN,
    unix_glReplacementCodeuiColor4ubVertex3fSUN,
    unix_glReplacementCodeuiColor4ubVertex3fvSUN,
    unix_glReplacementCodeuiNormal3fVertex3fSUN,
    unix_glReplacementCodeuiNormal3fVertex3fvSUN,
    unix_glReplacementCodeuiSUN,
    unix_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN,
    unix_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN,
    unix_glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN,
    unix_glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN,
    unix_glReplacementCodeuiTexCoord2fVertex3fSUN,
    unix_glReplacementCodeuiTexCoord2fVertex3fvSUN,
    unix_glReplacementCodeuiVertex3fSUN,
    unix_glReplacementCodeuiVertex3fvSUN,
    unix_glReplacementCodeuivSUN,
    unix_glReplacementCodeusSUN,
    unix_glReplacementCodeusvSUN,
    unix_glRequestResidentProgramsNV,
    unix_glResetHistogram,
    unix_glResetHistogramEXT,
    unix_glResetMemoryObjectParameterNV,
    unix_glResetMinmax,
    unix_glResetMinmaxEXT,
    unix_glResizeBuffersMESA,
    unix_glResolveDepthValuesNV,
    unix_glResumeTransformFeedback,
    unix_glResumeTransformFeedbackNV,
    unix_glRotatexOES,
    unix_glSampleCoverage,
    unix_glSampleCoverageARB,
    unix_glSampleMapATI,
    unix_glSampleMaskEXT,
    unix_glSampleMaskIndexedNV,
    unix_glSampleMaskSGIS,
    unix_glSampleMaski,
    unix_glSamplePatternEXT,
    unix_glSamplePatternSGIS,
    unix_glSamplerParameterIiv,
    unix_glSamplerParameterIuiv,
    unix_glSamplerParameterf,
    unix_glSamplerParameterfv,
    unix_glSamplerParameteri,
    unix_glSamplerParameteriv,
    unix_glScalexOES,
    unix_glScissorArrayv,
    unix_glScissorExclusiveArrayvNV,
    unix_glScissorExclusiveNV,
    unix_glScissorIndexed,
    unix_glScissorIndexedv,
    unix_glSecondaryColor3b,
    unix_glSecondaryColor3bEXT,
    unix_glSecondaryColor3bv,
    unix_glSecondaryColor3bvEXT,
    unix_glSecondaryColor3d,
    unix_glSecondaryColor3dEXT,
    unix_glSecondaryColor3dv,
    unix_glSecondaryColor3dvEXT,
    unix_glSecondaryColor3f,
    unix_glSecondaryColor3fEXT,
    unix_glSecondaryColor3fv,
    unix_glSecondaryColor3fvEXT,
    unix_glSecondaryColor3hNV,
    unix_glSecondaryColor3hvNV,
    unix_glSecondaryColor3i,
    unix_glSecondaryColor3iEXT,
    unix_glSecondaryColor3iv,
    unix_glSecondaryColor3ivEXT,
    unix_glSecondaryColor3s,
    unix_glSecondaryColor3sEXT,
    unix_glSecondaryColor3sv,
    unix_glSecondaryColor3svEXT,
    unix_glSecondaryColor3ub,
    unix_glSecondaryColor3ubEXT,
    unix_glSecondaryColor3ubv,
    unix_glSecondaryColor3ubvEXT,
    unix_glSecondaryColor3ui,
    unix_glSecondaryColor3uiEXT,
    unix_glSecondaryColor3uiv,
    unix_glSecondaryColor3uivEXT,
    unix_glSecondaryColor3us,
    unix_glSecondaryColor3usEXT,
    unix_glSecondaryColor3usv,
    unix_glSecondaryColor3usvEXT,
    unix_glSecondaryColorFormatNV,
    unix_glSecondaryColorP3ui,
    unix_glSecondaryColorP3uiv,
    unix_glSecondaryColorPointer,
    unix_glSecondaryColorPointerEXT,
    unix_glSecondaryColorPointerListIBM,
    unix_glSelectPerfMonitorCountersAMD,
    unix_glSelectTextureCoordSetSGIS,
    unix_glSelectTextureSGIS,
    unix_glSemaphoreParameterui64vEXT,
    unix_glSeparableFilter2D,
    unix_glSeparableFilter2DEXT,
    unix_glSetFenceAPPLE,
    unix_glSetFenceNV,
    unix_glSetFragmentShaderConstantATI,
    unix_glSetInvariantEXT,
    unix_glSetLocalConstantEXT,
    unix_glSetMultisamplefvAMD,
    unix_glShaderBinary,
    unix_glShaderOp1EXT,
    unix_glShaderOp2EXT,
    unix_glShaderOp3EXT,
    unix_glShaderSource,
    unix_glShaderSourceARB,
    unix_glShaderStorageBlockBinding,
    unix_glShadingRateImageBarrierNV,
    unix_glShadingRateImagePaletteNV,
    unix_glShadingRateSampleOrderCustomNV,
    unix_glShadingRateSampleOrderNV,
    unix_glSharpenTexFuncSGIS,
    unix_glSignalSemaphoreEXT,
    unix_glSignalSemaphoreui64NVX,
    unix_glSignalVkFenceNV,
    unix_glSignalVkSemaphoreNV,
    unix_glSpecializeShader,
    unix_glSpecializeShaderARB,
    unix_glSpriteParameterfSGIX,
    unix_glSpriteParameterfvSGIX,
    unix_glSpriteParameteriSGIX,
    unix_glSpriteParameterivSGIX,
    unix_glStartInstrumentsSGIX,
    unix_glStateCaptureNV,
    unix_glStencilClearTagEXT,
    unix_glStencilFillPathInstancedNV,
    unix_glStencilFillPathNV,
    unix_glStencilFuncSeparate,
    unix_glStencilFuncSeparateATI,
    unix_glStencilMaskSeparate,
    unix_glStencilOpSeparate,
    unix_glStencilOpSeparateATI,
    unix_glStencilOpValueAMD,
    unix_glStencilStrokePathInstancedNV,
    unix_glStencilStrokePathNV,
    unix_glStencilThenCoverFillPathInstancedNV,
    unix_glStencilThenCoverFillPathNV,
    unix_glStencilThenCoverStrokePathInstancedNV,
    unix_glStencilThenCoverStrokePathNV,
    unix_glStopInstrumentsSGIX,
    unix_glStringMarkerGREMEDY,
    unix_glSubpixelPrecisionBiasNV,
    unix_glSwizzleEXT,
    unix_glSyncTextureINTEL,
    unix_glTagSampleBufferSGIX,
    unix_glTangent3bEXT,
    unix_glTangent3bvEXT,
    unix_glTangent3dEXT,
    unix_glTangent3dvEXT,
    unix_glTangent3fEXT,
    unix_glTangent3fvEXT,
    unix_glTangent3iEXT,
    unix_glTangent3ivEXT,
    unix_glTangent3sEXT,
    unix_glTangent3svEXT,
    unix_glTangentPointerEXT,
    unix_glTbufferMask3DFX,
    unix_glTessellationFactorAMD,
    unix_glTessellationModeAMD,
    unix_glTestFenceAPPLE,
    unix_glTestFenceNV,
    unix_glTestObjectAPPLE,
    unix_glTexAttachMemoryNV,
    unix_glTexBuffer,
    unix_glTexBufferARB,
    unix_glTexBufferEXT,
    unix_glTexBufferRange,
    unix_glTexBumpParameterfvATI,
    unix_glTexBumpParameterivATI,
    unix_glTexCoord1bOES,
    unix_glTexCoord1bvOES,
    unix_glTexCoord1hNV,
    unix_glTexCoord1hvNV,
    unix_glTexCoord1xOES,
    unix_glTexCoord1xvOES,
    unix_glTexCoord2bOES,
    unix_glTexCoord2bvOES,
    unix_glTexCoord2fColor3fVertex3fSUN,
    unix_glTexCoord2fColor3fVertex3fvSUN,
    unix_glTexCoord2fColor4fNormal3fVertex3fSUN,
    unix_glTexCoord2fColor4fNormal3fVertex3fvSUN,
    unix_glTexCoord2fColor4ubVertex3fSUN,
    unix_glTexCoord2fColor4ubVertex3fvSUN,
    unix_glTexCoord2fNormal3fVertex3fSUN,
    unix_glTexCoord2fNormal3fVertex3fvSUN,
    unix_glTexCoord2fVertex3fSUN,
    unix_glTexCoord2fVertex3fvSUN,
    unix_glTexCoord2hNV,
    unix_glTexCoord2hvNV,
    unix_glTexCoord2xOES,
    unix_glTexCoord2xvOES,
    unix_glTexCoord3bOES,
    unix_glTexCoord3bvOES,
    unix_glTexCoord3hNV,
    unix_glTexCoord3hvNV,
    unix_glTexCoord3xOES,
    unix_glTexCoord3xvOES,
    unix_glTexCoord4bOES,
    unix_glTexCoord4bvOES,
    unix_glTexCoord4fColor4fNormal3fVertex4fSUN,
    unix_glTexCoord4fColor4fNormal3fVertex4fvSUN,
    unix_glTexCoord4fVertex4fSUN,
    unix_glTexCoord4fVertex4fvSUN,
    unix_glTexCoord4hNV,
    unix_glTexCoord4hvNV,
    unix_glTexCoord4xOES,
    unix_glTexCoord4xvOES,
    unix_glTexCoordFormatNV,
    unix_glTexCoordP1ui,
    unix_glTexCoordP1uiv,
    unix_glTexCoordP2ui,
    unix_glTexCoordP2uiv,
    unix_glTexCoordP3ui,
    unix_glTexCoordP3uiv,
    unix_glTexCoordP4ui,
    unix_glTexCoordP4uiv,
    unix_glTexCoordPointerEXT,
    unix_glTexCoordPointerListIBM,
    unix_glTexCoordPointervINTEL,
    unix_glTexEnvxOES,
    unix_glTexEnvxvOES,
    unix_glTexFilterFuncSGIS,
    unix_glTexGenxOES,
    unix_glTexGenxvOES,
    unix_glTexImage2DMultisample,
    unix_glTexImage2DMultisampleCoverageNV,
    unix_glTexImage3D,
    unix_glTexImage3DEXT,
    unix_glTexImage3DMultisample,
    unix_glTexImage3DMultisampleCoverageNV,
    unix_glTexImage4DSGIS,
    unix_glTexPageCommitmentARB,
    unix_glTexParameterIiv,
    unix_glTexParameterIivEXT,
    unix_glTexParameterIuiv,
    unix_glTexParameterIuivEXT,
    unix_glTexParameterxOES,
    unix_glTexParameterxvOES,
    unix_glTexRenderbufferNV,
    unix_glTexStorage1D,
    unix_glTexStorage2D,
    unix_glTexStorage2DMultisample,
    unix_glTexStorage3D,
    unix_glTexStorage3DMultisample,
    unix_glTexStorageMem1DEXT,
    unix_glTexStorageMem2DEXT,
    unix_glTexStorageMem2DMultisampleEXT,
    unix_glTexStorageMem3DEXT,
    unix_glTexStorageMem3DMultisampleEXT,
    unix_glTexStorageSparseAMD,
    unix_glTexSubImage1DEXT,
    unix_glTexSubImage2DEXT,
    unix_glTexSubImage3D,
    unix_glTexSubImage3DEXT,
    unix_glTexSubImage4DSGIS,
    unix_glTextureAttachMemoryNV,
    unix_glTextureBarrier,
    unix_glTextureBarrierNV,
    unix_glTextureBuffer,
    unix_glTextureBufferEXT,
    unix_glTextureBufferRange,
    unix_glTextureBufferRangeEXT,
    unix_glTextureColorMaskSGIS,
    unix_glTextureImage1DEXT,
    unix_glTextureImage2DEXT,
    unix_glTextureImage2DMultisampleCoverageNV,
    unix_glTextureImage2DMultisampleNV,
    unix_glTextureImage3DEXT,
    unix_glTextureImage3DMultisampleCoverageNV,
    unix_glTextureImage3DMultisampleNV,
    unix_glTextureLightEXT,
    unix_glTextureMaterialEXT,
    unix_glTextureNormalEXT,
    unix_glTexturePageCommitmentEXT,
    unix_glTextureParameterIiv,
    unix_glTextureParameterIivEXT,
    unix_glTextureParameterIuiv,
    unix_glTextureParameterIuivEXT,
    unix_glTextureParameterf,
    unix_glTextureParameterfEXT,
    unix_glTextureParameterfv,
    unix_glTextureParameterfvEXT,
    unix_glTextureParameteri,
    unix_glTextureParameteriEXT,
    unix_glTextureParameteriv,
    unix_glTextureParameterivEXT,
    unix_glTextureRangeAPPLE,
    unix_glTextureRenderbufferEXT,
    unix_glTextureStorage1D,
    unix_glTextureStorage1DEXT,
    unix_glTextureStorage2D,
    unix_glTextureStorage2DEXT,
    unix_glTextureStorage2DMultisample,
    unix_glTextureStorage2DMultisampleEXT,
    unix_glTextureStorage3D,
    unix_glTextureStorage3DEXT,
    unix_glTextureStorage3DMultisample,
    unix_glTextureStorage3DMultisampleEXT,
    unix_glTextureStorageMem1DEXT,
    unix_glTextureStorageMem2DEXT,
    unix_glTextureStorageMem2DMultisampleEXT,
    unix_glTextureStorageMem3DEXT,
    unix_glTextureStorageMem3DMultisampleEXT,
    unix_glTextureStorageSparseAMD,
    unix_glTextureSubImage1D,
    unix_glTextureSubImage1DEXT,
    unix_glTextureSubImage2D,
    unix_glTextureSubImage2DEXT,
    unix_glTextureSubImage3D,
    unix_glTextureSubImage3DEXT,
    unix_glTextureView,
    unix_glTrackMatrixNV,
    unix_glTransformFeedbackAttribsNV,
    unix_glTransformFeedbackBufferBase,
    unix_glTransformFeedbackBufferRange,
    unix_glTransformFeedbackStreamAttribsNV,
    unix_glTransformFeedbackVaryings,
    unix_glTransformFeedbackVaryingsEXT,
    unix_glTransformFeedbackVaryingsNV,
    unix_glTransformPathNV,
    unix_glTranslatexOES,
    unix_glUniform1d,
    unix_glUniform1dv,
    unix_glUniform1f,
    unix_glUniform1fARB,
    unix_glUniform1fv,
    unix_glUniform1fvARB,
    unix_glUniform1i,
    unix_glUniform1i64ARB,
    unix_glUniform1i64NV,
    unix_glUniform1i64vARB,
    unix_glUniform1i64vNV,
    unix_glUniform1iARB,
    unix_glUniform1iv,
    unix_glUniform1ivARB,
    unix_glUniform1ui,
    unix_glUniform1ui64ARB,
    unix_glUniform1ui64NV,
    unix_glUniform1ui64vARB,
    unix_glUniform1ui64vNV,
    unix_glUniform1uiEXT,
    unix_glUniform1uiv,
    unix_glUniform1uivEXT,
    unix_glUniform2d,
    unix_glUniform2dv,
    unix_glUniform2f,
    unix_glUniform2fARB,
    unix_glUniform2fv,
    unix_glUniform2fvARB,
    unix_glUniform2i,
    unix_glUniform2i64ARB,
    unix_glUniform2i64NV,
    unix_glUniform2i64vARB,
    unix_glUniform2i64vNV,
    unix_glUniform2iARB,
    unix_glUniform2iv,
    unix_glUniform2ivARB,
    unix_glUniform2ui,
    unix_glUniform2ui64ARB,
    unix_glUniform2ui64NV,
    unix_glUniform2ui64vARB,
    unix_glUniform2ui64vNV,
    unix_glUniform2uiEXT,
    unix_glUniform2uiv,
    unix_glUniform2uivEXT,
    unix_glUniform3d,
    unix_glUniform3dv,
    unix_glUniform3f,
    unix_glUniform3fARB,
    unix_glUniform3fv,
    unix_glUniform3fvARB,
    unix_glUniform3i,
    unix_glUniform3i64ARB,
    unix_glUniform3i64NV,
    unix_glUniform3i64vARB,
    unix_glUniform3i64vNV,
    unix_glUniform3iARB,
    unix_glUniform3iv,
    unix_glUniform3ivARB,
    unix_glUniform3ui,
    unix_glUniform3ui64ARB,
    unix_glUniform3ui64NV,
    unix_glUniform3ui64vARB,
    unix_glUniform3ui64vNV,
    unix_glUniform3uiEXT,
    unix_glUniform3uiv,
    unix_glUniform3uivEXT,
    unix_glUniform4d,
    unix_glUniform4dv,
    unix_glUniform4f,
    unix_glUniform4fARB,
    unix_glUniform4fv,
    unix_glUniform4fvARB,
    unix_glUniform4i,
    unix_glUniform4i64ARB,
    unix_glUniform4i64NV,
    unix_glUniform4i64vARB,
    unix_glUniform4i64vNV,
    unix_glUniform4iARB,
    unix_glUniform4iv,
    unix_glUniform4ivARB,
    unix_glUniform4ui,
    unix_glUniform4ui64ARB,
    unix_glUniform4ui64NV,
    unix_glUniform4ui64vARB,
    unix_glUniform4ui64vNV,
    unix_glUniform4uiEXT,
    unix_glUniform4uiv,
    unix_glUniform4uivEXT,
    unix_glUniformBlockBinding,
    unix_glUniformBufferEXT,
    unix_glUniformHandleui64ARB,
    unix_glUniformHandleui64NV,
    unix_glUniformHandleui64vARB,
    unix_glUniformHandleui64vNV,
    unix_glUniformMatrix2dv,
    unix_glUniformMatrix2fv,
    unix_glUniformMatrix2fvARB,
    unix_glUniformMatrix2x3dv,
    unix_glUniformMatrix2x3fv,
    unix_glUniformMatrix2x4dv,
    unix_glUniformMatrix2x4fv,
    unix_glUniformMatrix3dv,
    unix_glUniformMatrix3fv,
    unix_glUniformMatrix3fvARB,
    unix_glUniformMatrix3x2dv,
    unix_glUniformMatrix3x2fv,
    unix_glUniformMatrix3x4dv,
    unix_glUniformMatrix3x4fv,
    unix_glUniformMatrix4dv,
    unix_glUniformMatrix4fv,
    unix_glUniformMatrix4fvARB,
    unix_glUniformMatrix4x2dv,
    unix_glUniformMatrix4x2fv,
    unix_glUniformMatrix4x3dv,
    unix_glUniformMatrix4x3fv,
    unix_glUniformSubroutinesuiv,
    unix_glUniformui64NV,
    unix_glUniformui64vNV,
    unix_glUnlockArraysEXT,
    unix_glUnmapBuffer,
    unix_glUnmapBufferARB,
    unix_glUnmapNamedBuffer,
    unix_glUnmapNamedBufferEXT,
    unix_glUnmapObjectBufferATI,
    unix_glUnmapTexture2DINTEL,
    unix_glUpdateObjectBufferATI,
    unix_glUploadGpuMaskNVX,
    unix_glUseProgram,
    unix_glUseProgramObjectARB,
    unix_glUseProgramStages,
    unix_glUseShaderProgramEXT,
    unix_glVDPAUFiniNV,
    unix_glVDPAUGetSurfaceivNV,
    unix_glVDPAUInitNV,
    unix_glVDPAUIsSurfaceNV,
    unix_glVDPAUMapSurfacesNV,
    unix_glVDPAURegisterOutputSurfaceNV,
    unix_glVDPAURegisterVideoSurfaceNV,
    unix_glVDPAURegisterVideoSurfaceWithPictureStructureNV,
    unix_glVDPAUSurfaceAccessNV,
    unix_glVDPAUUnmapSurfacesNV,
    unix_glVDPAUUnregisterSurfaceNV,
    unix_glValidateProgram,
    unix_glValidateProgramARB,
    unix_glValidateProgramPipeline,
    unix_glVariantArrayObjectATI,
    unix_glVariantPointerEXT,
    unix_glVariantbvEXT,
    unix_glVariantdvEXT,
    unix_glVariantfvEXT,
    unix_glVariantivEXT,
    unix_glVariantsvEXT,
    unix_glVariantubvEXT,
    unix_glVariantuivEXT,
    unix_glVariantusvEXT,
    unix_glVertex2bOES,
    unix_glVertex2bvOES,
    unix_glVertex2hNV,
    unix_glVertex2hvNV,
    unix_glVertex2xOES,
    unix_glVertex2xvOES,
    unix_glVertex3bOES,
    unix_glVertex3bvOES,
    unix_glVertex3hNV,
    unix_glVertex3hvNV,
    unix_glVertex3xOES,
    unix_glVertex3xvOES,
    unix_glVertex4bOES,
    unix_glVertex4bvOES,
    unix_glVertex4hNV,
    unix_glVertex4hvNV,
    unix_glVertex4xOES,
    unix_glVertex4xvOES,
    unix_glVertexArrayAttribBinding,
    unix_glVertexArrayAttribFormat,
    unix_glVertexArrayAttribIFormat,
    unix_glVertexArrayAttribLFormat,
    unix_glVertexArrayBindVertexBufferEXT,
    unix_glVertexArrayBindingDivisor,
    unix_glVertexArrayColorOffsetEXT,
    unix_glVertexArrayEdgeFlagOffsetEXT,
    unix_glVertexArrayElementBuffer,
    unix_glVertexArrayFogCoordOffsetEXT,
    unix_glVertexArrayIndexOffsetEXT,
    unix_glVertexArrayMultiTexCoordOffsetEXT,
    unix_glVertexArrayNormalOffsetEXT,
    unix_glVertexArrayParameteriAPPLE,
    unix_glVertexArrayRangeAPPLE,
    unix_glVertexArrayRangeNV,
    unix_glVertexArraySecondaryColorOffsetEXT,
    unix_glVertexArrayTexCoordOffsetEXT,
    unix_glVertexArrayVertexAttribBindingEXT,
    unix_glVertexArrayVertexAttribDivisorEXT,
    unix_glVertexArrayVertexAttribFormatEXT,
    unix_glVertexArrayVertexAttribIFormatEXT,
    unix_glVertexArrayVertexAttribIOffsetEXT,
    unix_glVertexArrayVertexAttribLFormatEXT,
    unix_glVertexArrayVertexAttribLOffsetEXT,
    unix_glVertexArrayVertexAttribOffsetEXT,
    unix_glVertexArrayVertexBindingDivisorEXT,
    unix_glVertexArrayVertexBuffer,
    unix_glVertexArrayVertexBuffers,
    unix_glVertexArrayVertexOffsetEXT,
    unix_glVertexAttrib1d,
    unix_glVertexAttrib1dARB,
    unix_glVertexAttrib1dNV,
    unix_glVertexAttrib1dv,
    unix_glVertexAttrib1dvARB,
    unix_glVertexAttrib1dvNV,
    unix_glVertexAttrib1f,
    unix_glVertexAttrib1fARB,
    unix_glVertexAttrib1fNV,
    unix_glVertexAttrib1fv,
    unix_glVertexAttrib1fvARB,
    unix_glVertexAttrib1fvNV,
    unix_glVertexAttrib1hNV,
    unix_glVertexAttrib1hvNV,
    unix_glVertexAttrib1s,
    unix_glVertexAttrib1sARB,
    unix_glVertexAttrib1sNV,
    unix_glVertexAttrib1sv,
    unix_glVertexAttrib1svARB,
    unix_glVertexAttrib1svNV,
    unix_glVertexAttrib2d,
    unix_glVertexAttrib2dARB,
    unix_glVertexAttrib2dNV,
    unix_glVertexAttrib2dv,
    unix_glVertexAttrib2dvARB,
    unix_glVertexAttrib2dvNV,
    unix_glVertexAttrib2f,
    unix_glVertexAttrib2fARB,
    unix_glVertexAttrib2fNV,
    unix_glVertexAttrib2fv,
    unix_glVertexAttrib2fvARB,
    unix_glVertexAttrib2fvNV,
    unix_glVertexAttrib2hNV,
    unix_glVertexAttrib2hvNV,
    unix_glVertexAttrib2s,
    unix_glVertexAttrib2sARB,
    unix_glVertexAttrib2sNV,
    unix_glVertexAttrib2sv,
    unix_glVertexAttrib2svARB,
    unix_glVertexAttrib2svNV,
    unix_glVertexAttrib3d,
    unix_glVertexAttrib3dARB,
    unix_glVertexAttrib3dNV,
    unix_glVertexAttrib3dv,
    unix_glVertexAttrib3dvARB,
    unix_glVertexAttrib3dvNV,
    unix_glVertexAttrib3f,
    unix_glVertexAttrib3fARB,
    unix_glVertexAttrib3fNV,
    unix_glVertexAttrib3fv,
    unix_glVertexAttrib3fvARB,
    unix_glVertexAttrib3fvNV,
    unix_glVertexAttrib3hNV,
    unix_glVertexAttrib3hvNV,
    unix_glVertexAttrib3s,
    unix_glVertexAttrib3sARB,
    unix_glVertexAttrib3sNV,
    unix_glVertexAttrib3sv,
    unix_glVertexAttrib3svARB,
    unix_glVertexAttrib3svNV,
    unix_glVertexAttrib4Nbv,
    unix_glVertexAttrib4NbvARB,
    unix_glVertexAttrib4Niv,
    unix_glVertexAttrib4NivARB,
    unix_glVertexAttrib4Nsv,
    unix_glVertexAttrib4NsvARB,
    unix_glVertexAttrib4Nub,
    unix_glVertexAttrib4NubARB,
    unix_glVertexAttrib4Nubv,
    unix_glVertexAttrib4NubvARB,
    unix_glVertexAttrib4Nuiv,
    unix_glVertexAttrib4NuivARB,
    unix_glVertexAttrib4Nusv,
    unix_glVertexAttrib4NusvARB,
    unix_glVertexAttrib4bv,
    unix_glVertexAttrib4bvARB,
    unix_glVertexAttrib4d,
    unix_glVertexAttrib4dARB,
    unix_glVertexAttrib4dNV,
    unix_glVertexAttrib4dv,
    unix_glVertexAttrib4dvARB,
    unix_glVertexAttrib4dvNV,
    unix_glVertexAttrib4f,
    unix_glVertexAttrib4fARB,
    unix_glVertexAttrib4fNV,
    unix_glVertexAttrib4fv,
    unix_glVertexAttrib4fvARB,
    unix_glVertexAttrib4fvNV,
    unix_glVertexAttrib4hNV,
    unix_glVertexAttrib4hvNV,
    unix_glVertexAttrib4iv,
    unix_glVertexAttrib4ivARB,
    unix_glVertexAttrib4s,
    unix_glVertexAttrib4sARB,
    unix_glVertexAttrib4sNV,
    unix_glVertexAttrib4sv,
    unix_glVertexAttrib4svARB,
    unix_glVertexAttrib4svNV,
    unix_glVertexAttrib4ubNV,
    unix_glVertexAttrib4ubv,
    unix_glVertexAttrib4ubvARB,
    unix_glVertexAttrib4ubvNV,
    unix_glVertexAttrib4uiv,
    unix_glVertexAttrib4uivARB,
    unix_glVertexAttrib4usv,
    unix_glVertexAttrib4usvARB,
    unix_glVertexAttribArrayObjectATI,
    unix_glVertexAttribBinding,
    unix_glVertexAttribDivisor,
    unix_glVertexAttribDivisorARB,
    unix_glVertexAttribFormat,
    unix_glVertexAttribFormatNV,
    unix_glVertexAttribI1i,
    unix_glVertexAttribI1iEXT,
    unix_glVertexAttribI1iv,
    unix_glVertexAttribI1ivEXT,
    unix_glVertexAttribI1ui,
    unix_glVertexAttribI1uiEXT,
    unix_glVertexAttribI1uiv,
    unix_glVertexAttribI1uivEXT,
    unix_glVertexAttribI2i,
    unix_glVertexAttribI2iEXT,
    unix_glVertexAttribI2iv,
    unix_glVertexAttribI2ivEXT,
    unix_glVertexAttribI2ui,
    unix_glVertexAttribI2uiEXT,
    unix_glVertexAttribI2uiv,
    unix_glVertexAttribI2uivEXT,
    unix_glVertexAttribI3i,
    unix_glVertexAttribI3iEXT,
    unix_glVertexAttribI3iv,
    unix_glVertexAttribI3ivEXT,
    unix_glVertexAttribI3ui,
    unix_glVertexAttribI3uiEXT,
    unix_glVertexAttribI3uiv,
    unix_glVertexAttribI3uivEXT,
    unix_glVertexAttribI4bv,
    unix_glVertexAttribI4bvEXT,
    unix_glVertexAttribI4i,
    unix_glVertexAttribI4iEXT,
    unix_glVertexAttribI4iv,
    unix_glVertexAttribI4ivEXT,
    unix_glVertexAttribI4sv,
    unix_glVertexAttribI4svEXT,
    unix_glVertexAttribI4ubv,
    unix_glVertexAttribI4ubvEXT,
    unix_glVertexAttribI4ui,
    unix_glVertexAttribI4uiEXT,
    unix_glVertexAttribI4uiv,
    unix_glVertexAttribI4uivEXT,
    unix_glVertexAttribI4usv,
    unix_glVertexAttribI4usvEXT,
    unix_glVertexAttribIFormat,
    unix_glVertexAttribIFormatNV,
    unix_glVertexAttribIPointer,
    unix_glVertexAttribIPointerEXT,
    unix_glVertexAttribL1d,
    unix_glVertexAttribL1dEXT,
    unix_glVertexAttribL1dv,
    unix_glVertexAttribL1dvEXT,
    unix_glVertexAttribL1i64NV,
    unix_glVertexAttribL1i64vNV,
    unix_glVertexAttribL1ui64ARB,
    unix_glVertexAttribL1ui64NV,
    unix_glVertexAttribL1ui64vARB,
    unix_glVertexAttribL1ui64vNV,
    unix_glVertexAttribL2d,
    unix_glVertexAttribL2dEXT,
    unix_glVertexAttribL2dv,
    unix_glVertexAttribL2dvEXT,
    unix_glVertexAttribL2i64NV,
    unix_glVertexAttribL2i64vNV,
    unix_glVertexAttribL2ui64NV,
    unix_glVertexAttribL2ui64vNV,
    unix_glVertexAttribL3d,
    unix_glVertexAttribL3dEXT,
    unix_glVertexAttribL3dv,
    unix_glVertexAttribL3dvEXT,
    unix_glVertexAttribL3i64NV,
    unix_glVertexAttribL3i64vNV,
    unix_glVertexAttribL3ui64NV,
    unix_glVertexAttribL3ui64vNV,
    unix_glVertexAttribL4d,
    unix_glVertexAttribL4dEXT,
    unix_glVertexAttribL4dv,
    unix_glVertexAttribL4dvEXT,
    unix_glVertexAttribL4i64NV,
    unix_glVertexAttribL4i64vNV,
    unix_glVertexAttribL4ui64NV,
    unix_glVertexAttribL4ui64vNV,
    unix_glVertexAttribLFormat,
    unix_glVertexAttribLFormatNV,
    unix_glVertexAttribLPointer,
    unix_glVertexAttribLPointerEXT,
    unix_glVertexAttribP1ui,
    unix_glVertexAttribP1uiv,
    unix_glVertexAttribP2ui,
    unix_glVertexAttribP2uiv,
    unix_glVertexAttribP3ui,
    unix_glVertexAttribP3uiv,
    unix_glVertexAttribP4ui,
    unix_glVertexAttribP4uiv,
    unix_glVertexAttribParameteriAMD,
    unix_glVertexAttribPointer,
    unix_glVertexAttribPointerARB,
    unix_glVertexAttribPointerNV,
    unix_glVertexAttribs1dvNV,
    unix_glVertexAttribs1fvNV,
    unix_glVertexAttribs1hvNV,
    unix_glVertexAttribs1svNV,
    unix_glVertexAttribs2dvNV,
    unix_glVertexAttribs2fvNV,
    unix_glVertexAttribs2hvNV,
    unix_glVertexAttribs2svNV,
    unix_glVertexAttribs3dvNV,
    unix_glVertexAttribs3fvNV,
    unix_glVertexAttribs3hvNV,
    unix_glVertexAttribs3svNV,
    unix_glVertexAttribs4dvNV,
    unix_glVertexAttribs4fvNV,
    unix_glVertexAttribs4hvNV,
    unix_glVertexAttribs4svNV,
    unix_glVertexAttribs4ubvNV,
    unix_glVertexBindingDivisor,
    unix_glVertexBlendARB,
    unix_glVertexBlendEnvfATI,
    unix_glVertexBlendEnviATI,
    unix_glVertexFormatNV,
    unix_glVertexP2ui,
    unix_glVertexP2uiv,
    unix_glVertexP3ui,
    unix_glVertexP3uiv,
    unix_glVertexP4ui,
    unix_glVertexP4uiv,
    unix_glVertexPointerEXT,
    unix_glVertexPointerListIBM,
    unix_glVertexPointervINTEL,
    unix_glVertexStream1dATI,
    unix_glVertexStream1dvATI,
    unix_glVertexStream1fATI,
    unix_glVertexStream1fvATI,
    unix_glVertexStream1iATI,
    unix_glVertexStream1ivATI,
    unix_glVertexStream1sATI,
    unix_glVertexStream1svATI,
    unix_glVertexStream2dATI,
    unix_glVertexStream2dvATI,
    unix_glVertexStream2fATI,
    unix_glVertexStream2fvATI,
    unix_glVertexStream2iATI,
    unix_glVertexStream2ivATI,
    unix_glVertexStream2sATI,
    unix_glVertexStream2svATI,
    unix_glVertexStream3dATI,
    unix_glVertexStream3dvATI,
    unix_glVertexStream3fATI,
    unix_glVertexStream3fvATI,
    unix_glVertexStream3iATI,
    unix_glVertexStream3ivATI,
    unix_glVertexStream3sATI,
    unix_glVertexStream3svATI,
    unix_glVertexStream4dATI,
    unix_glVertexStream4dvATI,
    unix_glVertexStream4fATI,
    unix_glVertexStream4fvATI,
    unix_glVertexStream4iATI,
    unix_glVertexStream4ivATI,
    unix_glVertexStream4sATI,
    unix_glVertexStream4svATI,
    unix_glVertexWeightPointerEXT,
    unix_glVertexWeightfEXT,
    unix_glVertexWeightfvEXT,
    unix_glVertexWeighthNV,
    unix_glVertexWeighthvNV,
    unix_glVideoCaptureNV,
    unix_glVideoCaptureStreamParameterdvNV,
    unix_glVideoCaptureStreamParameterfvNV,
    unix_glVideoCaptureStreamParameterivNV,
    unix_glViewportArrayv,
    unix_glViewportIndexedf,
    unix_glViewportIndexedfv,
    unix_glViewportPositionWScaleNV,
    unix_glViewportSwizzleNV,
    unix_glWaitSemaphoreEXT,
    unix_glWaitSemaphoreui64NVX,
    unix_glWaitSync,
    unix_glWaitVkSemaphoreNV,
    unix_glWeightPathsNV,
    unix_glWeightPointerARB,
    unix_glWeightbvARB,
    unix_glWeightdvARB,
    unix_glWeightfvARB,
    unix_glWeightivARB,
    unix_glWeightsvARB,
    unix_glWeightubvARB,
    unix_glWeightuivARB,
    unix_glWeightusvARB,
    unix_glWindowPos2d,
    unix_glWindowPos2dARB,
    unix_glWindowPos2dMESA,
    unix_glWindowPos2dv,
    unix_glWindowPos2dvARB,
    unix_glWindowPos2dvMESA,
    unix_glWindowPos2f,
    unix_glWindowPos2fARB,
    unix_glWindowPos2fMESA,
    unix_glWindowPos2fv,
    unix_glWindowPos2fvARB,
    unix_glWindowPos2fvMESA,
    unix_glWindowPos2i,
    unix_glWindowPos2iARB,
    unix_glWindowPos2iMESA,
    unix_glWindowPos2iv,
    unix_glWindowPos2ivARB,
    unix_glWindowPos2ivMESA,
    unix_glWindowPos2s,
    unix_glWindowPos2sARB,
    unix_glWindowPos2sMESA,
    unix_glWindowPos2sv,
    unix_glWindowPos2svARB,
    unix_glWindowPos2svMESA,
    unix_glWindowPos3d,
    unix_glWindowPos3dARB,
    unix_glWindowPos3dMESA,
    unix_glWindowPos3dv,
    unix_glWindowPos3dvARB,
    unix_glWindowPos3dvMESA,
    unix_glWindowPos3f,
    unix_glWindowPos3fARB,
    unix_glWindowPos3fMESA,
    unix_glWindowPos3fv,
    unix_glWindowPos3fvARB,
    unix_glWindowPos3fvMESA,
    unix_glWindowPos3i,
    unix_glWindowPos3iARB,
    unix_glWindowPos3iMESA,
    unix_glWindowPos3iv,
    unix_glWindowPos3ivARB,
    unix_glWindowPos3ivMESA,
    unix_glWindowPos3s,
    unix_glWindowPos3sARB,
    unix_glWindowPos3sMESA,
    unix_glWindowPos3sv,
    unix_glWindowPos3svARB,
    unix_glWindowPos3svMESA,
    unix_glWindowPos4dMESA,
    unix_glWindowPos4dvMESA,
    unix_glWindowPos4fMESA,
    unix_glWindowPos4fvMESA,
    unix_glWindowPos4iMESA,
    unix_glWindowPos4ivMESA,
    unix_glWindowPos4sMESA,
    unix_glWindowPos4svMESA,
    unix_glWindowRectanglesEXT,
    unix_glWriteMaskEXT,
    unix_wglAllocateMemoryNV,
    unix_wglBindTexImageARB,
    unix_wglChoosePixelFormatARB,
    unix_wglCreateContextAttribsARB,
    unix_wglCreatePbufferARB,
    unix_wglDestroyPbufferARB,
    unix_wglFreeMemoryNV,
    unix_wglGetExtensionsStringARB,
    unix_wglGetExtensionsStringEXT,
    unix_wglGetPbufferDCARB,
    unix_wglGetPixelFormatAttribfvARB,
    unix_wglGetPixelFormatAttribivARB,
    unix_wglGetSwapIntervalEXT,
    unix_wglMakeContextCurrentARB,
    unix_wglQueryCurrentRendererIntegerWINE,
    unix_wglQueryCurrentRendererStringWINE,
    unix_wglQueryPbufferARB,
    unix_wglQueryRendererIntegerWINE,
    unix_wglQueryRendererStringWINE,
    unix_wglReleasePbufferDCARB,
    unix_wglReleaseTexImageARB,
    unix_wglSetPbufferAttribARB,
    unix_wglSetPixelFormatWINE,
    unix_wglSwapIntervalEXT,
    funcs_count
};

struct gl_debug_message_callback_params
{
    struct dispatch_callback_params dispatch;
    UINT64 debug_callback; /* client pointer */
    UINT64 debug_user; /* client pointer */
    UINT32 source;
    UINT32 type;
    UINT32 id;
    UINT32 severity;
    UINT32 length;
    char message[1];
};

#define UNIX_CALL( func, params ) WINE_UNIX_CALL( unix_ ## func, params )

#endif /* __WINE_OPENGL32_UNIXLIB_H */
