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

#include "wine/wgl.h"
#include "wine/unixlib.h"

struct glAccum_params
{
    GLenum op;
    GLfloat value;
};

struct glAlphaFunc_params
{
    GLenum func;
    GLfloat ref;
};

struct glAreTexturesResident_params
{
    GLsizei n;
    const GLuint *textures;
    GLboolean *residences;
    GLboolean ret;
};

struct glArrayElement_params
{
    GLint i;
};

struct glBegin_params
{
    GLenum mode;
};

struct glBindTexture_params
{
    GLenum target;
    GLuint texture;
};

struct glBitmap_params
{
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
    GLenum sfactor;
    GLenum dfactor;
};

struct glCallList_params
{
    GLuint list;
};

struct glCallLists_params
{
    GLsizei n;
    GLenum type;
    const void *lists;
};

struct glClear_params
{
    GLbitfield mask;
};

struct glClearAccum_params
{
    GLfloat red;
    GLfloat green;
    GLfloat blue;
    GLfloat alpha;
};

struct glClearColor_params
{
    GLfloat red;
    GLfloat green;
    GLfloat blue;
    GLfloat alpha;
};

struct glClearDepth_params
{
    GLdouble depth;
};

struct glClearIndex_params
{
    GLfloat c;
};

struct glClearStencil_params
{
    GLint s;
};

struct glClipPlane_params
{
    GLenum plane;
    const GLdouble *equation;
};

struct glColor3b_params
{
    GLbyte red;
    GLbyte green;
    GLbyte blue;
};

struct glColor3bv_params
{
    const GLbyte *v;
};

struct glColor3d_params
{
    GLdouble red;
    GLdouble green;
    GLdouble blue;
};

struct glColor3dv_params
{
    const GLdouble *v;
};

struct glColor3f_params
{
    GLfloat red;
    GLfloat green;
    GLfloat blue;
};

struct glColor3fv_params
{
    const GLfloat *v;
};

struct glColor3i_params
{
    GLint red;
    GLint green;
    GLint blue;
};

struct glColor3iv_params
{
    const GLint *v;
};

struct glColor3s_params
{
    GLshort red;
    GLshort green;
    GLshort blue;
};

struct glColor3sv_params
{
    const GLshort *v;
};

struct glColor3ub_params
{
    GLubyte red;
    GLubyte green;
    GLubyte blue;
};

struct glColor3ubv_params
{
    const GLubyte *v;
};

struct glColor3ui_params
{
    GLuint red;
    GLuint green;
    GLuint blue;
};

struct glColor3uiv_params
{
    const GLuint *v;
};

struct glColor3us_params
{
    GLushort red;
    GLushort green;
    GLushort blue;
};

struct glColor3usv_params
{
    const GLushort *v;
};

struct glColor4b_params
{
    GLbyte red;
    GLbyte green;
    GLbyte blue;
    GLbyte alpha;
};

struct glColor4bv_params
{
    const GLbyte *v;
};

struct glColor4d_params
{
    GLdouble red;
    GLdouble green;
    GLdouble blue;
    GLdouble alpha;
};

struct glColor4dv_params
{
    const GLdouble *v;
};

struct glColor4f_params
{
    GLfloat red;
    GLfloat green;
    GLfloat blue;
    GLfloat alpha;
};

struct glColor4fv_params
{
    const GLfloat *v;
};

struct glColor4i_params
{
    GLint red;
    GLint green;
    GLint blue;
    GLint alpha;
};

struct glColor4iv_params
{
    const GLint *v;
};

struct glColor4s_params
{
    GLshort red;
    GLshort green;
    GLshort blue;
    GLshort alpha;
};

struct glColor4sv_params
{
    const GLshort *v;
};

struct glColor4ub_params
{
    GLubyte red;
    GLubyte green;
    GLubyte blue;
    GLubyte alpha;
};

struct glColor4ubv_params
{
    const GLubyte *v;
};

struct glColor4ui_params
{
    GLuint red;
    GLuint green;
    GLuint blue;
    GLuint alpha;
};

struct glColor4uiv_params
{
    const GLuint *v;
};

struct glColor4us_params
{
    GLushort red;
    GLushort green;
    GLushort blue;
    GLushort alpha;
};

struct glColor4usv_params
{
    const GLushort *v;
};

struct glColorMask_params
{
    GLboolean red;
    GLboolean green;
    GLboolean blue;
    GLboolean alpha;
};

struct glColorMaterial_params
{
    GLenum face;
    GLenum mode;
};

struct glColorPointer_params
{
    GLint size;
    GLenum type;
    GLsizei stride;
    const void *pointer;
};

struct glCopyPixels_params
{
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
    GLenum type;
};

struct glCopyTexImage1D_params
{
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
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint x;
    GLint y;
    GLsizei width;
};

struct glCopyTexSubImage2D_params
{
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
    GLenum mode;
};

struct glDeleteLists_params
{
    GLuint list;
    GLsizei range;
};

struct glDeleteTextures_params
{
    GLsizei n;
    const GLuint *textures;
};

struct glDepthFunc_params
{
    GLenum func;
};

struct glDepthMask_params
{
    GLboolean flag;
};

struct glDepthRange_params
{
    GLdouble n;
    GLdouble f;
};

struct glDisable_params
{
    GLenum cap;
};

struct glDisableClientState_params
{
    GLenum array;
};

struct glDrawArrays_params
{
    GLenum mode;
    GLint first;
    GLsizei count;
};

struct glDrawBuffer_params
{
    GLenum buf;
};

struct glDrawElements_params
{
    GLenum mode;
    GLsizei count;
    GLenum type;
    const void *indices;
};

struct glDrawPixels_params
{
    GLsizei width;
    GLsizei height;
    GLenum format;
    GLenum type;
    const void *pixels;
};

struct glEdgeFlag_params
{
    GLboolean flag;
};

struct glEdgeFlagPointer_params
{
    GLsizei stride;
    const void *pointer;
};

struct glEdgeFlagv_params
{
    const GLboolean *flag;
};

struct glEnable_params
{
    GLenum cap;
};

struct glEnableClientState_params
{
    GLenum array;
};

struct glEnd_params
{
};

struct glEndList_params
{
};

struct glEvalCoord1d_params
{
    GLdouble u;
};

struct glEvalCoord1dv_params
{
    const GLdouble *u;
};

struct glEvalCoord1f_params
{
    GLfloat u;
};

struct glEvalCoord1fv_params
{
    const GLfloat *u;
};

struct glEvalCoord2d_params
{
    GLdouble u;
    GLdouble v;
};

struct glEvalCoord2dv_params
{
    const GLdouble *u;
};

struct glEvalCoord2f_params
{
    GLfloat u;
    GLfloat v;
};

struct glEvalCoord2fv_params
{
    const GLfloat *u;
};

struct glEvalMesh1_params
{
    GLenum mode;
    GLint i1;
    GLint i2;
};

struct glEvalMesh2_params
{
    GLenum mode;
    GLint i1;
    GLint i2;
    GLint j1;
    GLint j2;
};

struct glEvalPoint1_params
{
    GLint i;
};

struct glEvalPoint2_params
{
    GLint i;
    GLint j;
};

struct glFeedbackBuffer_params
{
    GLsizei size;
    GLenum type;
    GLfloat *buffer;
};

struct glFinish_params
{
};

struct glFlush_params
{
};

struct glFogf_params
{
    GLenum pname;
    GLfloat param;
};

struct glFogfv_params
{
    GLenum pname;
    const GLfloat *params;
};

struct glFogi_params
{
    GLenum pname;
    GLint param;
};

struct glFogiv_params
{
    GLenum pname;
    const GLint *params;
};

struct glFrontFace_params
{
    GLenum mode;
};

struct glFrustum_params
{
    GLdouble left;
    GLdouble right;
    GLdouble bottom;
    GLdouble top;
    GLdouble zNear;
    GLdouble zFar;
};

struct glGenLists_params
{
    GLsizei range;
    GLuint ret;
};

struct glGenTextures_params
{
    GLsizei n;
    GLuint *textures;
};

struct glGetBooleanv_params
{
    GLenum pname;
    GLboolean *data;
};

struct glGetClipPlane_params
{
    GLenum plane;
    GLdouble *equation;
};

struct glGetDoublev_params
{
    GLenum pname;
    GLdouble *data;
};

struct glGetError_params
{
    GLenum ret;
};

struct glGetFloatv_params
{
    GLenum pname;
    GLfloat *data;
};

struct glGetIntegerv_params
{
    GLenum pname;
    GLint *data;
};

struct glGetLightfv_params
{
    GLenum light;
    GLenum pname;
    GLfloat *params;
};

struct glGetLightiv_params
{
    GLenum light;
    GLenum pname;
    GLint *params;
};

struct glGetMapdv_params
{
    GLenum target;
    GLenum query;
    GLdouble *v;
};

struct glGetMapfv_params
{
    GLenum target;
    GLenum query;
    GLfloat *v;
};

struct glGetMapiv_params
{
    GLenum target;
    GLenum query;
    GLint *v;
};

struct glGetMaterialfv_params
{
    GLenum face;
    GLenum pname;
    GLfloat *params;
};

struct glGetMaterialiv_params
{
    GLenum face;
    GLenum pname;
    GLint *params;
};

struct glGetPixelMapfv_params
{
    GLenum map;
    GLfloat *values;
};

struct glGetPixelMapuiv_params
{
    GLenum map;
    GLuint *values;
};

struct glGetPixelMapusv_params
{
    GLenum map;
    GLushort *values;
};

struct glGetPointerv_params
{
    GLenum pname;
    void **params;
};

struct glGetPolygonStipple_params
{
    GLubyte *mask;
};

struct glGetString_params
{
    GLenum name;
    const GLubyte *ret;
};

struct glGetTexEnvfv_params
{
    GLenum target;
    GLenum pname;
    GLfloat *params;
};

struct glGetTexEnviv_params
{
    GLenum target;
    GLenum pname;
    GLint *params;
};

struct glGetTexGendv_params
{
    GLenum coord;
    GLenum pname;
    GLdouble *params;
};

struct glGetTexGenfv_params
{
    GLenum coord;
    GLenum pname;
    GLfloat *params;
};

struct glGetTexGeniv_params
{
    GLenum coord;
    GLenum pname;
    GLint *params;
};

struct glGetTexImage_params
{
    GLenum target;
    GLint level;
    GLenum format;
    GLenum type;
    void *pixels;
};

struct glGetTexLevelParameterfv_params
{
    GLenum target;
    GLint level;
    GLenum pname;
    GLfloat *params;
};

struct glGetTexLevelParameteriv_params
{
    GLenum target;
    GLint level;
    GLenum pname;
    GLint *params;
};

struct glGetTexParameterfv_params
{
    GLenum target;
    GLenum pname;
    GLfloat *params;
};

struct glGetTexParameteriv_params
{
    GLenum target;
    GLenum pname;
    GLint *params;
};

struct glHint_params
{
    GLenum target;
    GLenum mode;
};

struct glIndexMask_params
{
    GLuint mask;
};

struct glIndexPointer_params
{
    GLenum type;
    GLsizei stride;
    const void *pointer;
};

struct glIndexd_params
{
    GLdouble c;
};

struct glIndexdv_params
{
    const GLdouble *c;
};

struct glIndexf_params
{
    GLfloat c;
};

struct glIndexfv_params
{
    const GLfloat *c;
};

struct glIndexi_params
{
    GLint c;
};

struct glIndexiv_params
{
    const GLint *c;
};

struct glIndexs_params
{
    GLshort c;
};

struct glIndexsv_params
{
    const GLshort *c;
};

struct glIndexub_params
{
    GLubyte c;
};

struct glIndexubv_params
{
    const GLubyte *c;
};

struct glInitNames_params
{
};

struct glInterleavedArrays_params
{
    GLenum format;
    GLsizei stride;
    const void *pointer;
};

struct glIsEnabled_params
{
    GLenum cap;
    GLboolean ret;
};

struct glIsList_params
{
    GLuint list;
    GLboolean ret;
};

struct glIsTexture_params
{
    GLuint texture;
    GLboolean ret;
};

struct glLightModelf_params
{
    GLenum pname;
    GLfloat param;
};

struct glLightModelfv_params
{
    GLenum pname;
    const GLfloat *params;
};

struct glLightModeli_params
{
    GLenum pname;
    GLint param;
};

struct glLightModeliv_params
{
    GLenum pname;
    const GLint *params;
};

struct glLightf_params
{
    GLenum light;
    GLenum pname;
    GLfloat param;
};

struct glLightfv_params
{
    GLenum light;
    GLenum pname;
    const GLfloat *params;
};

struct glLighti_params
{
    GLenum light;
    GLenum pname;
    GLint param;
};

struct glLightiv_params
{
    GLenum light;
    GLenum pname;
    const GLint *params;
};

struct glLineStipple_params
{
    GLint factor;
    GLushort pattern;
};

struct glLineWidth_params
{
    GLfloat width;
};

struct glListBase_params
{
    GLuint base;
};

struct glLoadIdentity_params
{
};

struct glLoadMatrixd_params
{
    const GLdouble *m;
};

struct glLoadMatrixf_params
{
    const GLfloat *m;
};

struct glLoadName_params
{
    GLuint name;
};

struct glLogicOp_params
{
    GLenum opcode;
};

struct glMap1d_params
{
    GLenum target;
    GLdouble u1;
    GLdouble u2;
    GLint stride;
    GLint order;
    const GLdouble *points;
};

struct glMap1f_params
{
    GLenum target;
    GLfloat u1;
    GLfloat u2;
    GLint stride;
    GLint order;
    const GLfloat *points;
};

struct glMap2d_params
{
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
    GLint un;
    GLdouble u1;
    GLdouble u2;
};

struct glMapGrid1f_params
{
    GLint un;
    GLfloat u1;
    GLfloat u2;
};

struct glMapGrid2d_params
{
    GLint un;
    GLdouble u1;
    GLdouble u2;
    GLint vn;
    GLdouble v1;
    GLdouble v2;
};

struct glMapGrid2f_params
{
    GLint un;
    GLfloat u1;
    GLfloat u2;
    GLint vn;
    GLfloat v1;
    GLfloat v2;
};

struct glMaterialf_params
{
    GLenum face;
    GLenum pname;
    GLfloat param;
};

struct glMaterialfv_params
{
    GLenum face;
    GLenum pname;
    const GLfloat *params;
};

struct glMateriali_params
{
    GLenum face;
    GLenum pname;
    GLint param;
};

struct glMaterialiv_params
{
    GLenum face;
    GLenum pname;
    const GLint *params;
};

struct glMatrixMode_params
{
    GLenum mode;
};

struct glMultMatrixd_params
{
    const GLdouble *m;
};

struct glMultMatrixf_params
{
    const GLfloat *m;
};

struct glNewList_params
{
    GLuint list;
    GLenum mode;
};

struct glNormal3b_params
{
    GLbyte nx;
    GLbyte ny;
    GLbyte nz;
};

struct glNormal3bv_params
{
    const GLbyte *v;
};

struct glNormal3d_params
{
    GLdouble nx;
    GLdouble ny;
    GLdouble nz;
};

struct glNormal3dv_params
{
    const GLdouble *v;
};

struct glNormal3f_params
{
    GLfloat nx;
    GLfloat ny;
    GLfloat nz;
};

struct glNormal3fv_params
{
    const GLfloat *v;
};

struct glNormal3i_params
{
    GLint nx;
    GLint ny;
    GLint nz;
};

struct glNormal3iv_params
{
    const GLint *v;
};

struct glNormal3s_params
{
    GLshort nx;
    GLshort ny;
    GLshort nz;
};

struct glNormal3sv_params
{
    const GLshort *v;
};

struct glNormalPointer_params
{
    GLenum type;
    GLsizei stride;
    const void *pointer;
};

struct glOrtho_params
{
    GLdouble left;
    GLdouble right;
    GLdouble bottom;
    GLdouble top;
    GLdouble zNear;
    GLdouble zFar;
};

struct glPassThrough_params
{
    GLfloat token;
};

struct glPixelMapfv_params
{
    GLenum map;
    GLsizei mapsize;
    const GLfloat *values;
};

struct glPixelMapuiv_params
{
    GLenum map;
    GLsizei mapsize;
    const GLuint *values;
};

struct glPixelMapusv_params
{
    GLenum map;
    GLsizei mapsize;
    const GLushort *values;
};

struct glPixelStoref_params
{
    GLenum pname;
    GLfloat param;
};

struct glPixelStorei_params
{
    GLenum pname;
    GLint param;
};

struct glPixelTransferf_params
{
    GLenum pname;
    GLfloat param;
};

struct glPixelTransferi_params
{
    GLenum pname;
    GLint param;
};

struct glPixelZoom_params
{
    GLfloat xfactor;
    GLfloat yfactor;
};

struct glPointSize_params
{
    GLfloat size;
};

struct glPolygonMode_params
{
    GLenum face;
    GLenum mode;
};

struct glPolygonOffset_params
{
    GLfloat factor;
    GLfloat units;
};

struct glPolygonStipple_params
{
    const GLubyte *mask;
};

struct glPopAttrib_params
{
};

struct glPopClientAttrib_params
{
};

struct glPopMatrix_params
{
};

struct glPopName_params
{
};

struct glPrioritizeTextures_params
{
    GLsizei n;
    const GLuint *textures;
    const GLfloat *priorities;
};

struct glPushAttrib_params
{
    GLbitfield mask;
};

struct glPushClientAttrib_params
{
    GLbitfield mask;
};

struct glPushMatrix_params
{
};

struct glPushName_params
{
    GLuint name;
};

struct glRasterPos2d_params
{
    GLdouble x;
    GLdouble y;
};

struct glRasterPos2dv_params
{
    const GLdouble *v;
};

struct glRasterPos2f_params
{
    GLfloat x;
    GLfloat y;
};

struct glRasterPos2fv_params
{
    const GLfloat *v;
};

struct glRasterPos2i_params
{
    GLint x;
    GLint y;
};

struct glRasterPos2iv_params
{
    const GLint *v;
};

struct glRasterPos2s_params
{
    GLshort x;
    GLshort y;
};

struct glRasterPos2sv_params
{
    const GLshort *v;
};

struct glRasterPos3d_params
{
    GLdouble x;
    GLdouble y;
    GLdouble z;
};

struct glRasterPos3dv_params
{
    const GLdouble *v;
};

struct glRasterPos3f_params
{
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

struct glRasterPos3fv_params
{
    const GLfloat *v;
};

struct glRasterPos3i_params
{
    GLint x;
    GLint y;
    GLint z;
};

struct glRasterPos3iv_params
{
    const GLint *v;
};

struct glRasterPos3s_params
{
    GLshort x;
    GLshort y;
    GLshort z;
};

struct glRasterPos3sv_params
{
    const GLshort *v;
};

struct glRasterPos4d_params
{
    GLdouble x;
    GLdouble y;
    GLdouble z;
    GLdouble w;
};

struct glRasterPos4dv_params
{
    const GLdouble *v;
};

struct glRasterPos4f_params
{
    GLfloat x;
    GLfloat y;
    GLfloat z;
    GLfloat w;
};

struct glRasterPos4fv_params
{
    const GLfloat *v;
};

struct glRasterPos4i_params
{
    GLint x;
    GLint y;
    GLint z;
    GLint w;
};

struct glRasterPos4iv_params
{
    const GLint *v;
};

struct glRasterPos4s_params
{
    GLshort x;
    GLshort y;
    GLshort z;
    GLshort w;
};

struct glRasterPos4sv_params
{
    const GLshort *v;
};

struct glReadBuffer_params
{
    GLenum src;
};

struct glReadPixels_params
{
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
    GLdouble x1;
    GLdouble y1;
    GLdouble x2;
    GLdouble y2;
};

struct glRectdv_params
{
    const GLdouble *v1;
    const GLdouble *v2;
};

struct glRectf_params
{
    GLfloat x1;
    GLfloat y1;
    GLfloat x2;
    GLfloat y2;
};

struct glRectfv_params
{
    const GLfloat *v1;
    const GLfloat *v2;
};

struct glRecti_params
{
    GLint x1;
    GLint y1;
    GLint x2;
    GLint y2;
};

struct glRectiv_params
{
    const GLint *v1;
    const GLint *v2;
};

struct glRects_params
{
    GLshort x1;
    GLshort y1;
    GLshort x2;
    GLshort y2;
};

struct glRectsv_params
{
    const GLshort *v1;
    const GLshort *v2;
};

struct glRenderMode_params
{
    GLenum mode;
    GLint ret;
};

struct glRotated_params
{
    GLdouble angle;
    GLdouble x;
    GLdouble y;
    GLdouble z;
};

struct glRotatef_params
{
    GLfloat angle;
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

struct glScaled_params
{
    GLdouble x;
    GLdouble y;
    GLdouble z;
};

struct glScalef_params
{
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

struct glScissor_params
{
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
};

struct glSelectBuffer_params
{
    GLsizei size;
    GLuint *buffer;
};

struct glShadeModel_params
{
    GLenum mode;
};

struct glStencilFunc_params
{
    GLenum func;
    GLint ref;
    GLuint mask;
};

struct glStencilMask_params
{
    GLuint mask;
};

struct glStencilOp_params
{
    GLenum fail;
    GLenum zfail;
    GLenum zpass;
};

struct glTexCoord1d_params
{
    GLdouble s;
};

struct glTexCoord1dv_params
{
    const GLdouble *v;
};

struct glTexCoord1f_params
{
    GLfloat s;
};

struct glTexCoord1fv_params
{
    const GLfloat *v;
};

struct glTexCoord1i_params
{
    GLint s;
};

struct glTexCoord1iv_params
{
    const GLint *v;
};

struct glTexCoord1s_params
{
    GLshort s;
};

struct glTexCoord1sv_params
{
    const GLshort *v;
};

struct glTexCoord2d_params
{
    GLdouble s;
    GLdouble t;
};

struct glTexCoord2dv_params
{
    const GLdouble *v;
};

struct glTexCoord2f_params
{
    GLfloat s;
    GLfloat t;
};

struct glTexCoord2fv_params
{
    const GLfloat *v;
};

struct glTexCoord2i_params
{
    GLint s;
    GLint t;
};

struct glTexCoord2iv_params
{
    const GLint *v;
};

struct glTexCoord2s_params
{
    GLshort s;
    GLshort t;
};

struct glTexCoord2sv_params
{
    const GLshort *v;
};

struct glTexCoord3d_params
{
    GLdouble s;
    GLdouble t;
    GLdouble r;
};

struct glTexCoord3dv_params
{
    const GLdouble *v;
};

struct glTexCoord3f_params
{
    GLfloat s;
    GLfloat t;
    GLfloat r;
};

struct glTexCoord3fv_params
{
    const GLfloat *v;
};

struct glTexCoord3i_params
{
    GLint s;
    GLint t;
    GLint r;
};

struct glTexCoord3iv_params
{
    const GLint *v;
};

struct glTexCoord3s_params
{
    GLshort s;
    GLshort t;
    GLshort r;
};

struct glTexCoord3sv_params
{
    const GLshort *v;
};

struct glTexCoord4d_params
{
    GLdouble s;
    GLdouble t;
    GLdouble r;
    GLdouble q;
};

struct glTexCoord4dv_params
{
    const GLdouble *v;
};

struct glTexCoord4f_params
{
    GLfloat s;
    GLfloat t;
    GLfloat r;
    GLfloat q;
};

struct glTexCoord4fv_params
{
    const GLfloat *v;
};

struct glTexCoord4i_params
{
    GLint s;
    GLint t;
    GLint r;
    GLint q;
};

struct glTexCoord4iv_params
{
    const GLint *v;
};

struct glTexCoord4s_params
{
    GLshort s;
    GLshort t;
    GLshort r;
    GLshort q;
};

struct glTexCoord4sv_params
{
    const GLshort *v;
};

struct glTexCoordPointer_params
{
    GLint size;
    GLenum type;
    GLsizei stride;
    const void *pointer;
};

struct glTexEnvf_params
{
    GLenum target;
    GLenum pname;
    GLfloat param;
};

struct glTexEnvfv_params
{
    GLenum target;
    GLenum pname;
    const GLfloat *params;
};

struct glTexEnvi_params
{
    GLenum target;
    GLenum pname;
    GLint param;
};

struct glTexEnviv_params
{
    GLenum target;
    GLenum pname;
    const GLint *params;
};

struct glTexGend_params
{
    GLenum coord;
    GLenum pname;
    GLdouble param;
};

struct glTexGendv_params
{
    GLenum coord;
    GLenum pname;
    const GLdouble *params;
};

struct glTexGenf_params
{
    GLenum coord;
    GLenum pname;
    GLfloat param;
};

struct glTexGenfv_params
{
    GLenum coord;
    GLenum pname;
    const GLfloat *params;
};

struct glTexGeni_params
{
    GLenum coord;
    GLenum pname;
    GLint param;
};

struct glTexGeniv_params
{
    GLenum coord;
    GLenum pname;
    const GLint *params;
};

struct glTexImage1D_params
{
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
    GLenum target;
    GLenum pname;
    GLfloat param;
};

struct glTexParameterfv_params
{
    GLenum target;
    GLenum pname;
    const GLfloat *params;
};

struct glTexParameteri_params
{
    GLenum target;
    GLenum pname;
    GLint param;
};

struct glTexParameteriv_params
{
    GLenum target;
    GLenum pname;
    const GLint *params;
};

struct glTexSubImage1D_params
{
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
    GLdouble x;
    GLdouble y;
    GLdouble z;
};

struct glTranslatef_params
{
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

struct glVertex2d_params
{
    GLdouble x;
    GLdouble y;
};

struct glVertex2dv_params
{
    const GLdouble *v;
};

struct glVertex2f_params
{
    GLfloat x;
    GLfloat y;
};

struct glVertex2fv_params
{
    const GLfloat *v;
};

struct glVertex2i_params
{
    GLint x;
    GLint y;
};

struct glVertex2iv_params
{
    const GLint *v;
};

struct glVertex2s_params
{
    GLshort x;
    GLshort y;
};

struct glVertex2sv_params
{
    const GLshort *v;
};

struct glVertex3d_params
{
    GLdouble x;
    GLdouble y;
    GLdouble z;
};

struct glVertex3dv_params
{
    const GLdouble *v;
};

struct glVertex3f_params
{
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

struct glVertex3fv_params
{
    const GLfloat *v;
};

struct glVertex3i_params
{
    GLint x;
    GLint y;
    GLint z;
};

struct glVertex3iv_params
{
    const GLint *v;
};

struct glVertex3s_params
{
    GLshort x;
    GLshort y;
    GLshort z;
};

struct glVertex3sv_params
{
    const GLshort *v;
};

struct glVertex4d_params
{
    GLdouble x;
    GLdouble y;
    GLdouble z;
    GLdouble w;
};

struct glVertex4dv_params
{
    const GLdouble *v;
};

struct glVertex4f_params
{
    GLfloat x;
    GLfloat y;
    GLfloat z;
    GLfloat w;
};

struct glVertex4fv_params
{
    const GLfloat *v;
};

struct glVertex4i_params
{
    GLint x;
    GLint y;
    GLint z;
    GLint w;
};

struct glVertex4iv_params
{
    const GLint *v;
};

struct glVertex4s_params
{
    GLshort x;
    GLshort y;
    GLshort z;
    GLshort w;
};

struct glVertex4sv_params
{
    const GLshort *v;
};

struct glVertexPointer_params
{
    GLint size;
    GLenum type;
    GLsizei stride;
    const void *pointer;
};

struct glViewport_params
{
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
};

enum unix_funcs
{
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
};

typedef NTSTATUS (*unixlib_function_t)( void *args );
extern const unixlib_function_t __wine_unix_call_funcs[] DECLSPEC_HIDDEN;
#define UNIX_CALL( func, params ) __wine_unix_call_funcs[unix_ ## func]( params )

#endif /* __WINE_OPENGL32_UNIXLIB_H */
