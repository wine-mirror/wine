
/* Auto-generated file... Do not edit ! */

#include "config.h"
#include "wine_gl.h"


void WINAPI wine_glClearIndex(GLfloat c ) {
  ENTER_GL();
  glClearIndex(c);
  LEAVE_GL();
}

void WINAPI wine_glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha ) {
  ENTER_GL();
  glClearColor(red, green, blue, alpha);
  LEAVE_GL();
}

void WINAPI wine_glClear(GLbitfield mask ) {
  ENTER_GL();
  glClear(mask);
  LEAVE_GL();
}

void WINAPI wine_glIndexMask(GLuint mask ) {
  ENTER_GL();
  glIndexMask(mask);
  LEAVE_GL();
}

void WINAPI wine_glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha ) {
  ENTER_GL();
  glColorMask(red, green, blue, alpha);
  LEAVE_GL();
}

void WINAPI wine_glAlphaFunc(GLenum func, GLclampf ref ) {
  ENTER_GL();
  glAlphaFunc(func, ref);
  LEAVE_GL();
}

void WINAPI wine_glBlendFunc(GLenum sfactor, GLenum dfactor ) {
  ENTER_GL();
  glBlendFunc(sfactor, dfactor);
  LEAVE_GL();
}

void WINAPI wine_glLogicOp(GLenum opcode ) {
  ENTER_GL();
  glLogicOp(opcode);
  LEAVE_GL();
}

void WINAPI wine_glCullFace(GLenum mode ) {
  ENTER_GL();
  glCullFace(mode);
  LEAVE_GL();
}

void WINAPI wine_glFrontFace(GLenum mode ) {
  ENTER_GL();
  glFrontFace(mode);
  LEAVE_GL();
}

void WINAPI wine_glPointSize(GLfloat size ) {
  ENTER_GL();
  glPointSize(size);
  LEAVE_GL();
}

void WINAPI wine_glLineWidth(GLfloat width ) {
  ENTER_GL();
  glLineWidth(width);
  LEAVE_GL();
}

void WINAPI wine_glLineStipple(GLint factor, GLushort pattern ) {
  ENTER_GL();
  glLineStipple(factor, pattern);
  LEAVE_GL();
}

void WINAPI wine_glPolygonMode(GLenum face, GLenum mode ) {
  ENTER_GL();
  glPolygonMode(face, mode);
  LEAVE_GL();
}

void WINAPI wine_glPolygonOffset(GLfloat factor, GLfloat units ) {
  ENTER_GL();
  glPolygonOffset(factor, units);
  LEAVE_GL();
}

void WINAPI wine_glPolygonStipple(const GLubyte *mask ) {
  ENTER_GL();
  glPolygonStipple(mask);
  LEAVE_GL();
}

void WINAPI wine_glGetPolygonStipple(GLubyte *mask ) {
  ENTER_GL();
  glGetPolygonStipple(mask);
  LEAVE_GL();
}

void WINAPI wine_glEdgeFlag(GLboolean flag ) {
  ENTER_GL();
  glEdgeFlag(flag);
  LEAVE_GL();
}

void WINAPI wine_glEdgeFlagv(const GLboolean *flag ) {
  ENTER_GL();
  glEdgeFlagv(flag);
  LEAVE_GL();
}

void WINAPI wine_glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
  ENTER_GL();
  glScissor(x, y, width, height);
  LEAVE_GL();
}

void WINAPI wine_glClipPlane(GLenum plane, const GLdouble *equation ) {
  ENTER_GL();
  glClipPlane(plane, equation);
  LEAVE_GL();
}

void WINAPI wine_glGetClipPlane(GLenum plane, GLdouble *equation ) {
  ENTER_GL();
  glGetClipPlane(plane, equation);
  LEAVE_GL();
}

void WINAPI wine_glDrawBuffer(GLenum mode ) {
  ENTER_GL();
  glDrawBuffer(mode);
  LEAVE_GL();
}

void WINAPI wine_glReadBuffer(GLenum mode ) {
  ENTER_GL();
  glReadBuffer(mode);
  LEAVE_GL();
}

void WINAPI wine_glEnable(GLenum cap ) {
  ENTER_GL();
  glEnable(cap);
  LEAVE_GL();
}

void WINAPI wine_glDisable(GLenum cap ) {
  ENTER_GL();
  glDisable(cap);
  LEAVE_GL();
}

GLboolean WINAPI wine_glIsEnabled(GLenum cap ) {
  GLboolean ret;
  ENTER_GL();
  ret = glIsEnabled(cap);
  LEAVE_GL();
  return ret;
}

void WINAPI wine_glEnableClientState(GLenum cap ) {
  ENTER_GL();
  glEnableClientState(cap);
  LEAVE_GL();
}

void WINAPI wine_glDisableClientState(GLenum cap ) {
  ENTER_GL();
  glDisableClientState(cap);
  LEAVE_GL();
}

void WINAPI wine_glGetBooleanv(GLenum pname, GLboolean *params ) {
  ENTER_GL();
  glGetBooleanv(pname, params);
  LEAVE_GL();
}

void WINAPI wine_glGetDoublev(GLenum pname, GLdouble *params ) {
  ENTER_GL();
  glGetDoublev(pname, params);
  LEAVE_GL();
}

void WINAPI wine_glGetFloatv(GLenum pname, GLfloat *params ) {
  ENTER_GL();
  glGetFloatv(pname, params);
  LEAVE_GL();
}

void WINAPI wine_glGetIntegerv(GLenum pname, GLint *params ) {
  ENTER_GL();
  glGetIntegerv(pname, params);
  LEAVE_GL();
}

void WINAPI wine_glPushAttrib(GLbitfield mask ) {
  ENTER_GL();
  glPushAttrib(mask);
  LEAVE_GL();
}

void WINAPI wine_glPopAttrib(void ) {
  ENTER_GL();
  glPopAttrib();
  LEAVE_GL();
}

void WINAPI wine_glPushClientAttrib(GLbitfield mask ) {
  ENTER_GL();
  glPushClientAttrib(mask);
  LEAVE_GL();
}

void WINAPI wine_glPopClientAttrib(void ) {
  ENTER_GL();
  glPopClientAttrib();
  LEAVE_GL();
}

GLint WINAPI wine_glRenderMode(GLenum mode ) {
  GLint ret;
  ENTER_GL();
  ret = glRenderMode(mode);
  LEAVE_GL();
  return ret;
}

GLenum WINAPI wine_glGetError(void ) {
  GLenum ret;
  ENTER_GL();
  ret = glGetError();
  LEAVE_GL();
  return ret;
}

const GLubyte* WINAPI wine_glGetString(GLenum name ) {
  const GLubyte* ret;
  ENTER_GL();
  ret = glGetString(name);
  LEAVE_GL();
  return ret;
}

void WINAPI wine_glFinish(void ) {
  ENTER_GL();
  glFinish();
  LEAVE_GL();
}

void WINAPI wine_glFlush(void ) {
  ENTER_GL();
  glFlush();
  LEAVE_GL();
}

void WINAPI wine_glHint(GLenum target, GLenum mode ) {
  ENTER_GL();
  glHint(target, mode);
  LEAVE_GL();
}

void WINAPI wine_glClearDepth(GLclampd depth ) {
  ENTER_GL();
  glClearDepth(depth);
  LEAVE_GL();
}

void WINAPI wine_glDepthFunc(GLenum func ) {
  ENTER_GL();
  glDepthFunc(func);
  LEAVE_GL();
}

void WINAPI wine_glDepthMask(GLboolean flag ) {
  ENTER_GL();
  glDepthMask(flag);
  LEAVE_GL();
}

void WINAPI wine_glDepthRange(GLclampd near_val, GLclampd far_val ) {
  ENTER_GL();
  glDepthRange(near_val, far_val);
  LEAVE_GL();
}

void WINAPI wine_glClearAccum(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha ) {
  ENTER_GL();
  glClearAccum(red, green, blue, alpha);
  LEAVE_GL();
}

void WINAPI wine_glAccum(GLenum op, GLfloat value ) {
  ENTER_GL();
  glAccum(op, value);
  LEAVE_GL();
}

void WINAPI wine_glMatrixMode(GLenum mode ) {
  ENTER_GL();
  glMatrixMode(mode);
  LEAVE_GL();
}

void WINAPI wine_glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val ) {
  ENTER_GL();
  glOrtho(left, right, bottom, top, near_val, far_val);
  LEAVE_GL();
}

void WINAPI wine_glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val ) {
  ENTER_GL();
  glFrustum(left, right, bottom, top, near_val, far_val);
  LEAVE_GL();
}

void WINAPI wine_glViewport(GLint x, GLint y, GLsizei width, GLsizei height ) {
  ENTER_GL();
  glViewport(x, y, width, height);
  LEAVE_GL();
}

void WINAPI wine_glPushMatrix(void ) {
  ENTER_GL();
  glPushMatrix();
  LEAVE_GL();
}

void WINAPI wine_glPopMatrix(void ) {
  ENTER_GL();
  glPopMatrix();
  LEAVE_GL();
}

void WINAPI wine_glLoadIdentity(void ) {
  ENTER_GL();
  glLoadIdentity();
  LEAVE_GL();
}

void WINAPI wine_glLoadMatrixd(const GLdouble *m ) {
  ENTER_GL();
  glLoadMatrixd(m);
  LEAVE_GL();
}

void WINAPI wine_glLoadMatrixf(const GLfloat *m ) {
  ENTER_GL();
  glLoadMatrixf(m);
  LEAVE_GL();
}

void WINAPI wine_glMultMatrixd(const GLdouble *m ) {
  ENTER_GL();
  glMultMatrixd(m);
  LEAVE_GL();
}

void WINAPI wine_glMultMatrixf(const GLfloat *m ) {
  ENTER_GL();
  glMultMatrixf(m);
  LEAVE_GL();
}

void WINAPI wine_glRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z ) {
  ENTER_GL();
  glRotated(angle, x, y, z);
  LEAVE_GL();
}

void WINAPI wine_glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z ) {
  ENTER_GL();
  glRotatef(angle, x, y, z);
  LEAVE_GL();
}

void WINAPI wine_glScaled(GLdouble x, GLdouble y, GLdouble z ) {
  ENTER_GL();
  glScaled(x, y, z);
  LEAVE_GL();
}

void WINAPI wine_glScalef(GLfloat x, GLfloat y, GLfloat z ) {
  ENTER_GL();
  glScalef(x, y, z);
  LEAVE_GL();
}

void WINAPI wine_glTranslated(GLdouble x, GLdouble y, GLdouble z ) {
  ENTER_GL();
  glTranslated(x, y, z);
  LEAVE_GL();
}

void WINAPI wine_glTranslatef(GLfloat x, GLfloat y, GLfloat z ) {
  ENTER_GL();
  glTranslatef(x, y, z);
  LEAVE_GL();
}

GLboolean WINAPI wine_glIsList(GLuint list ) {
  GLboolean ret;
  ENTER_GL();
  ret = glIsList(list);
  LEAVE_GL();
  return ret;
}

void WINAPI wine_glDeleteLists(GLuint list, GLsizei range ) {
  ENTER_GL();
  glDeleteLists(list, range);
  LEAVE_GL();
}

GLuint WINAPI wine_glGenLists(GLsizei range ) {
  GLuint ret;
  ENTER_GL();
  ret = glGenLists(range);
  LEAVE_GL();
  return ret;
}

void WINAPI wine_glNewList(GLuint list, GLenum mode ) {
  ENTER_GL();
  glNewList(list, mode);
  LEAVE_GL();
}

void WINAPI wine_glEndList(void ) {
  ENTER_GL();
  glEndList();
  LEAVE_GL();
}

void WINAPI wine_glCallList(GLuint list ) {
  ENTER_GL();
  glCallList(list);
  LEAVE_GL();
}

void WINAPI wine_glCallLists(GLsizei n, GLenum type, const GLvoid *lists ) {
  ENTER_GL();
  glCallLists(n, type, lists);
  LEAVE_GL();
}

void WINAPI wine_glListBase(GLuint base ) {
  ENTER_GL();
  glListBase(base);
  LEAVE_GL();
}

void WINAPI wine_glBegin(GLenum mode ) {
  ENTER_GL();
  glBegin(mode);
  LEAVE_GL();
}

void WINAPI wine_glEnd(void ) {
  ENTER_GL();
  glEnd();
  LEAVE_GL();
}

void WINAPI wine_glVertex2d(GLdouble x, GLdouble y ) {
  ENTER_GL();
  glVertex2d(x, y);
  LEAVE_GL();
}

void WINAPI wine_glVertex2f(GLfloat x, GLfloat y ) {
  ENTER_GL();
  glVertex2f(x, y);
  LEAVE_GL();
}

void WINAPI wine_glVertex2i(GLint x, GLint y ) {
  ENTER_GL();
  glVertex2i(x, y);
  LEAVE_GL();
}

void WINAPI wine_glVertex2s(GLshort x, GLshort y ) {
  ENTER_GL();
  glVertex2s(x, y);
  LEAVE_GL();
}

void WINAPI wine_glVertex3d(GLdouble x, GLdouble y, GLdouble z ) {
  ENTER_GL();
  glVertex3d(x, y, z);
  LEAVE_GL();
}

void WINAPI wine_glVertex3f(GLfloat x, GLfloat y, GLfloat z ) {
  ENTER_GL();
  glVertex3f(x, y, z);
  LEAVE_GL();
}

void WINAPI wine_glVertex3i(GLint x, GLint y, GLint z ) {
  ENTER_GL();
  glVertex3i(x, y, z);
  LEAVE_GL();
}

void WINAPI wine_glVertex3s(GLshort x, GLshort y, GLshort z ) {
  ENTER_GL();
  glVertex3s(x, y, z);
  LEAVE_GL();
}

void WINAPI wine_glVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  ENTER_GL();
  glVertex4d(x, y, z, w);
  LEAVE_GL();
}

void WINAPI wine_glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  ENTER_GL();
  glVertex4f(x, y, z, w);
  LEAVE_GL();
}

void WINAPI wine_glVertex4i(GLint x, GLint y, GLint z, GLint w ) {
  ENTER_GL();
  glVertex4i(x, y, z, w);
  LEAVE_GL();
}

void WINAPI wine_glVertex4s(GLshort x, GLshort y, GLshort z, GLshort w ) {
  ENTER_GL();
  glVertex4s(x, y, z, w);
  LEAVE_GL();
}

void WINAPI wine_glVertex2dv(const GLdouble *v ) {
  ENTER_GL();
  glVertex2dv(v);
  LEAVE_GL();
}

void WINAPI wine_glVertex2fv(const GLfloat *v ) {
  ENTER_GL();
  glVertex2fv(v);
  LEAVE_GL();
}

void WINAPI wine_glVertex2iv(const GLint *v ) {
  ENTER_GL();
  glVertex2iv(v);
  LEAVE_GL();
}

void WINAPI wine_glVertex2sv(const GLshort *v ) {
  ENTER_GL();
  glVertex2sv(v);
  LEAVE_GL();
}

void WINAPI wine_glVertex3dv(const GLdouble *v ) {
  ENTER_GL();
  glVertex3dv(v);
  LEAVE_GL();
}

void WINAPI wine_glVertex3fv(const GLfloat *v ) {
  ENTER_GL();
  glVertex3fv(v);
  LEAVE_GL();
}

void WINAPI wine_glVertex3iv(const GLint *v ) {
  ENTER_GL();
  glVertex3iv(v);
  LEAVE_GL();
}

void WINAPI wine_glVertex3sv(const GLshort *v ) {
  ENTER_GL();
  glVertex3sv(v);
  LEAVE_GL();
}

void WINAPI wine_glVertex4dv(const GLdouble *v ) {
  ENTER_GL();
  glVertex4dv(v);
  LEAVE_GL();
}

void WINAPI wine_glVertex4fv(const GLfloat *v ) {
  ENTER_GL();
  glVertex4fv(v);
  LEAVE_GL();
}

void WINAPI wine_glVertex4iv(const GLint *v ) {
  ENTER_GL();
  glVertex4iv(v);
  LEAVE_GL();
}

void WINAPI wine_glVertex4sv(const GLshort *v ) {
  ENTER_GL();
  glVertex4sv(v);
  LEAVE_GL();
}

void WINAPI wine_glNormal3b(GLbyte nx, GLbyte ny, GLbyte nz ) {
  ENTER_GL();
  glNormal3b(nx, ny, nz);
  LEAVE_GL();
}

void WINAPI wine_glNormal3d(GLdouble nx, GLdouble ny, GLdouble nz ) {
  ENTER_GL();
  glNormal3d(nx, ny, nz);
  LEAVE_GL();
}

void WINAPI wine_glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz ) {
  ENTER_GL();
  glNormal3f(nx, ny, nz);
  LEAVE_GL();
}

void WINAPI wine_glNormal3i(GLint nx, GLint ny, GLint nz ) {
  ENTER_GL();
  glNormal3i(nx, ny, nz);
  LEAVE_GL();
}

void WINAPI wine_glNormal3s(GLshort nx, GLshort ny, GLshort nz ) {
  ENTER_GL();
  glNormal3s(nx, ny, nz);
  LEAVE_GL();
}

void WINAPI wine_glNormal3bv(const GLbyte *v ) {
  ENTER_GL();
  glNormal3bv(v);
  LEAVE_GL();
}

void WINAPI wine_glNormal3dv(const GLdouble *v ) {
  ENTER_GL();
  glNormal3dv(v);
  LEAVE_GL();
}

void WINAPI wine_glNormal3fv(const GLfloat *v ) {
  ENTER_GL();
  glNormal3fv(v);
  LEAVE_GL();
}

void WINAPI wine_glNormal3iv(const GLint *v ) {
  ENTER_GL();
  glNormal3iv(v);
  LEAVE_GL();
}

void WINAPI wine_glNormal3sv(const GLshort *v ) {
  ENTER_GL();
  glNormal3sv(v);
  LEAVE_GL();
}

void WINAPI wine_glIndexd(GLdouble c ) {
  ENTER_GL();
  glIndexd(c);
  LEAVE_GL();
}

void WINAPI wine_glIndexf(GLfloat c ) {
  ENTER_GL();
  glIndexf(c);
  LEAVE_GL();
}

void WINAPI wine_glIndexi(GLint c ) {
  ENTER_GL();
  glIndexi(c);
  LEAVE_GL();
}

void WINAPI wine_glIndexs(GLshort c ) {
  ENTER_GL();
  glIndexs(c);
  LEAVE_GL();
}

void WINAPI wine_glIndexub(GLubyte c ) {
  ENTER_GL();
  glIndexub(c);
  LEAVE_GL();
}

void WINAPI wine_glIndexdv(const GLdouble *c ) {
  ENTER_GL();
  glIndexdv(c);
  LEAVE_GL();
}

void WINAPI wine_glIndexfv(const GLfloat *c ) {
  ENTER_GL();
  glIndexfv(c);
  LEAVE_GL();
}

void WINAPI wine_glIndexiv(const GLint *c ) {
  ENTER_GL();
  glIndexiv(c);
  LEAVE_GL();
}

void WINAPI wine_glIndexsv(const GLshort *c ) {
  ENTER_GL();
  glIndexsv(c);
  LEAVE_GL();
}

void WINAPI wine_glIndexubv(const GLubyte *c ) {
  ENTER_GL();
  glIndexubv(c);
  LEAVE_GL();
}

void WINAPI wine_glColor3b(GLbyte red, GLbyte green, GLbyte blue ) {
  ENTER_GL();
  glColor3b(red, green, blue);
  LEAVE_GL();
}

void WINAPI wine_glColor3d(GLdouble red, GLdouble green, GLdouble blue ) {
  ENTER_GL();
  glColor3d(red, green, blue);
  LEAVE_GL();
}

void WINAPI wine_glColor3f(GLfloat red, GLfloat green, GLfloat blue ) {
  ENTER_GL();
  glColor3f(red, green, blue);
  LEAVE_GL();
}

void WINAPI wine_glColor3i(GLint red, GLint green, GLint blue ) {
  ENTER_GL();
  glColor3i(red, green, blue);
  LEAVE_GL();
}

void WINAPI wine_glColor3s(GLshort red, GLshort green, GLshort blue ) {
  ENTER_GL();
  glColor3s(red, green, blue);
  LEAVE_GL();
}

void WINAPI wine_glColor3ub(GLubyte red, GLubyte green, GLubyte blue ) {
  ENTER_GL();
  glColor3ub(red, green, blue);
  LEAVE_GL();
}

void WINAPI wine_glColor3ui(GLuint red, GLuint green, GLuint blue ) {
  ENTER_GL();
  glColor3ui(red, green, blue);
  LEAVE_GL();
}

void WINAPI wine_glColor3us(GLushort red, GLushort green, GLushort blue ) {
  ENTER_GL();
  glColor3us(red, green, blue);
  LEAVE_GL();
}

void WINAPI wine_glColor4b(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha ) {
  ENTER_GL();
  glColor4b(red, green, blue, alpha);
  LEAVE_GL();
}

void WINAPI wine_glColor4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha ) {
  ENTER_GL();
  glColor4d(red, green, blue, alpha);
  LEAVE_GL();
}

void WINAPI wine_glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha ) {
  ENTER_GL();
  glColor4f(red, green, blue, alpha);
  LEAVE_GL();
}

void WINAPI wine_glColor4i(GLint red, GLint green, GLint blue, GLint alpha ) {
  ENTER_GL();
  glColor4i(red, green, blue, alpha);
  LEAVE_GL();
}

void WINAPI wine_glColor4s(GLshort red, GLshort green, GLshort blue, GLshort alpha ) {
  ENTER_GL();
  glColor4s(red, green, blue, alpha);
  LEAVE_GL();
}

void WINAPI wine_glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha ) {
  ENTER_GL();
  glColor4ub(red, green, blue, alpha);
  LEAVE_GL();
}

void WINAPI wine_glColor4ui(GLuint red, GLuint green, GLuint blue, GLuint alpha ) {
  ENTER_GL();
  glColor4ui(red, green, blue, alpha);
  LEAVE_GL();
}

void WINAPI wine_glColor4us(GLushort red, GLushort green, GLushort blue, GLushort alpha ) {
  ENTER_GL();
  glColor4us(red, green, blue, alpha);
  LEAVE_GL();
}

void WINAPI wine_glColor3bv(const GLbyte *v ) {
  ENTER_GL();
  glColor3bv(v);
  LEAVE_GL();
}

void WINAPI wine_glColor3dv(const GLdouble *v ) {
  ENTER_GL();
  glColor3dv(v);
  LEAVE_GL();
}

void WINAPI wine_glColor3fv(const GLfloat *v ) {
  ENTER_GL();
  glColor3fv(v);
  LEAVE_GL();
}

void WINAPI wine_glColor3iv(const GLint *v ) {
  ENTER_GL();
  glColor3iv(v);
  LEAVE_GL();
}

void WINAPI wine_glColor3sv(const GLshort *v ) {
  ENTER_GL();
  glColor3sv(v);
  LEAVE_GL();
}

void WINAPI wine_glColor3ubv(const GLubyte *v ) {
  ENTER_GL();
  glColor3ubv(v);
  LEAVE_GL();
}

void WINAPI wine_glColor3uiv(const GLuint *v ) {
  ENTER_GL();
  glColor3uiv(v);
  LEAVE_GL();
}

void WINAPI wine_glColor3usv(const GLushort *v ) {
  ENTER_GL();
  glColor3usv(v);
  LEAVE_GL();
}

void WINAPI wine_glColor4bv(const GLbyte *v ) {
  ENTER_GL();
  glColor4bv(v);
  LEAVE_GL();
}

void WINAPI wine_glColor4dv(const GLdouble *v ) {
  ENTER_GL();
  glColor4dv(v);
  LEAVE_GL();
}

void WINAPI wine_glColor4fv(const GLfloat *v ) {
  ENTER_GL();
  glColor4fv(v);
  LEAVE_GL();
}

void WINAPI wine_glColor4iv(const GLint *v ) {
  ENTER_GL();
  glColor4iv(v);
  LEAVE_GL();
}

void WINAPI wine_glColor4sv(const GLshort *v ) {
  ENTER_GL();
  glColor4sv(v);
  LEAVE_GL();
}

void WINAPI wine_glColor4ubv(const GLubyte *v ) {
  ENTER_GL();
  glColor4ubv(v);
  LEAVE_GL();
}

void WINAPI wine_glColor4uiv(const GLuint *v ) {
  ENTER_GL();
  glColor4uiv(v);
  LEAVE_GL();
}

void WINAPI wine_glColor4usv(const GLushort *v ) {
  ENTER_GL();
  glColor4usv(v);
  LEAVE_GL();
}

void WINAPI wine_glTexCoord1d(GLdouble s ) {
  ENTER_GL();
  glTexCoord1d(s);
  LEAVE_GL();
}

void WINAPI wine_glTexCoord1f(GLfloat s ) {
  ENTER_GL();
  glTexCoord1f(s);
  LEAVE_GL();
}

void WINAPI wine_glTexCoord1i(GLint s ) {
  ENTER_GL();
  glTexCoord1i(s);
  LEAVE_GL();
}

void WINAPI wine_glTexCoord1s(GLshort s ) {
  ENTER_GL();
  glTexCoord1s(s);
  LEAVE_GL();
}

void WINAPI wine_glTexCoord2d(GLdouble s, GLdouble t ) {
  ENTER_GL();
  glTexCoord2d(s, t);
  LEAVE_GL();
}

void WINAPI wine_glTexCoord2f(GLfloat s, GLfloat t ) {
  ENTER_GL();
  glTexCoord2f(s, t);
  LEAVE_GL();
}

void WINAPI wine_glTexCoord2i(GLint s, GLint t ) {
  ENTER_GL();
  glTexCoord2i(s, t);
  LEAVE_GL();
}

void WINAPI wine_glTexCoord2s(GLshort s, GLshort t ) {
  ENTER_GL();
  glTexCoord2s(s, t);
  LEAVE_GL();
}

void WINAPI wine_glTexCoord3d(GLdouble s, GLdouble t, GLdouble r ) {
  ENTER_GL();
  glTexCoord3d(s, t, r);
  LEAVE_GL();
}

void WINAPI wine_glTexCoord3f(GLfloat s, GLfloat t, GLfloat r ) {
  ENTER_GL();
  glTexCoord3f(s, t, r);
  LEAVE_GL();
}

void WINAPI wine_glTexCoord3i(GLint s, GLint t, GLint r ) {
  ENTER_GL();
  glTexCoord3i(s, t, r);
  LEAVE_GL();
}

void WINAPI wine_glTexCoord3s(GLshort s, GLshort t, GLshort r ) {
  ENTER_GL();
  glTexCoord3s(s, t, r);
  LEAVE_GL();
}

void WINAPI wine_glTexCoord4d(GLdouble s, GLdouble t, GLdouble r, GLdouble q ) {
  ENTER_GL();
  glTexCoord4d(s, t, r, q);
  LEAVE_GL();
}

void WINAPI wine_glTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q ) {
  ENTER_GL();
  glTexCoord4f(s, t, r, q);
  LEAVE_GL();
}

void WINAPI wine_glTexCoord4i(GLint s, GLint t, GLint r, GLint q ) {
  ENTER_GL();
  glTexCoord4i(s, t, r, q);
  LEAVE_GL();
}

void WINAPI wine_glTexCoord4s(GLshort s, GLshort t, GLshort r, GLshort q ) {
  ENTER_GL();
  glTexCoord4s(s, t, r, q);
  LEAVE_GL();
}

void WINAPI wine_glTexCoord1dv(const GLdouble *v ) {
  ENTER_GL();
  glTexCoord1dv(v);
  LEAVE_GL();
}

void WINAPI wine_glTexCoord1fv(const GLfloat *v ) {
  ENTER_GL();
  glTexCoord1fv(v);
  LEAVE_GL();
}

void WINAPI wine_glTexCoord1iv(const GLint *v ) {
  ENTER_GL();
  glTexCoord1iv(v);
  LEAVE_GL();
}

void WINAPI wine_glTexCoord1sv(const GLshort *v ) {
  ENTER_GL();
  glTexCoord1sv(v);
  LEAVE_GL();
}

void WINAPI wine_glTexCoord2dv(const GLdouble *v ) {
  ENTER_GL();
  glTexCoord2dv(v);
  LEAVE_GL();
}

void WINAPI wine_glTexCoord2fv(const GLfloat *v ) {
  ENTER_GL();
  glTexCoord2fv(v);
  LEAVE_GL();
}

void WINAPI wine_glTexCoord2iv(const GLint *v ) {
  ENTER_GL();
  glTexCoord2iv(v);
  LEAVE_GL();
}

void WINAPI wine_glTexCoord2sv(const GLshort *v ) {
  ENTER_GL();
  glTexCoord2sv(v);
  LEAVE_GL();
}

void WINAPI wine_glTexCoord3dv(const GLdouble *v ) {
  ENTER_GL();
  glTexCoord3dv(v);
  LEAVE_GL();
}

void WINAPI wine_glTexCoord3fv(const GLfloat *v ) {
  ENTER_GL();
  glTexCoord3fv(v);
  LEAVE_GL();
}

void WINAPI wine_glTexCoord3iv(const GLint *v ) {
  ENTER_GL();
  glTexCoord3iv(v);
  LEAVE_GL();
}

void WINAPI wine_glTexCoord3sv(const GLshort *v ) {
  ENTER_GL();
  glTexCoord3sv(v);
  LEAVE_GL();
}

void WINAPI wine_glTexCoord4dv(const GLdouble *v ) {
  ENTER_GL();
  glTexCoord4dv(v);
  LEAVE_GL();
}

void WINAPI wine_glTexCoord4fv(const GLfloat *v ) {
  ENTER_GL();
  glTexCoord4fv(v);
  LEAVE_GL();
}

void WINAPI wine_glTexCoord4iv(const GLint *v ) {
  ENTER_GL();
  glTexCoord4iv(v);
  LEAVE_GL();
}

void WINAPI wine_glTexCoord4sv(const GLshort *v ) {
  ENTER_GL();
  glTexCoord4sv(v);
  LEAVE_GL();
}

void WINAPI wine_glRasterPos2d(GLdouble x, GLdouble y ) {
  ENTER_GL();
  glRasterPos2d(x, y);
  LEAVE_GL();
}

void WINAPI wine_glRasterPos2f(GLfloat x, GLfloat y ) {
  ENTER_GL();
  glRasterPos2f(x, y);
  LEAVE_GL();
}

void WINAPI wine_glRasterPos2i(GLint x, GLint y ) {
  ENTER_GL();
  glRasterPos2i(x, y);
  LEAVE_GL();
}

void WINAPI wine_glRasterPos2s(GLshort x, GLshort y ) {
  ENTER_GL();
  glRasterPos2s(x, y);
  LEAVE_GL();
}

void WINAPI wine_glRasterPos3d(GLdouble x, GLdouble y, GLdouble z ) {
  ENTER_GL();
  glRasterPos3d(x, y, z);
  LEAVE_GL();
}

void WINAPI wine_glRasterPos3f(GLfloat x, GLfloat y, GLfloat z ) {
  ENTER_GL();
  glRasterPos3f(x, y, z);
  LEAVE_GL();
}

void WINAPI wine_glRasterPos3i(GLint x, GLint y, GLint z ) {
  ENTER_GL();
  glRasterPos3i(x, y, z);
  LEAVE_GL();
}

void WINAPI wine_glRasterPos3s(GLshort x, GLshort y, GLshort z ) {
  ENTER_GL();
  glRasterPos3s(x, y, z);
  LEAVE_GL();
}

void WINAPI wine_glRasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  ENTER_GL();
  glRasterPos4d(x, y, z, w);
  LEAVE_GL();
}

void WINAPI wine_glRasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  ENTER_GL();
  glRasterPos4f(x, y, z, w);
  LEAVE_GL();
}

void WINAPI wine_glRasterPos4i(GLint x, GLint y, GLint z, GLint w ) {
  ENTER_GL();
  glRasterPos4i(x, y, z, w);
  LEAVE_GL();
}

void WINAPI wine_glRasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w ) {
  ENTER_GL();
  glRasterPos4s(x, y, z, w);
  LEAVE_GL();
}

void WINAPI wine_glRasterPos2dv(const GLdouble *v ) {
  ENTER_GL();
  glRasterPos2dv(v);
  LEAVE_GL();
}

void WINAPI wine_glRasterPos2fv(const GLfloat *v ) {
  ENTER_GL();
  glRasterPos2fv(v);
  LEAVE_GL();
}

void WINAPI wine_glRasterPos2iv(const GLint *v ) {
  ENTER_GL();
  glRasterPos2iv(v);
  LEAVE_GL();
}

void WINAPI wine_glRasterPos2sv(const GLshort *v ) {
  ENTER_GL();
  glRasterPos2sv(v);
  LEAVE_GL();
}

void WINAPI wine_glRasterPos3dv(const GLdouble *v ) {
  ENTER_GL();
  glRasterPos3dv(v);
  LEAVE_GL();
}

void WINAPI wine_glRasterPos3fv(const GLfloat *v ) {
  ENTER_GL();
  glRasterPos3fv(v);
  LEAVE_GL();
}

void WINAPI wine_glRasterPos3iv(const GLint *v ) {
  ENTER_GL();
  glRasterPos3iv(v);
  LEAVE_GL();
}

void WINAPI wine_glRasterPos3sv(const GLshort *v ) {
  ENTER_GL();
  glRasterPos3sv(v);
  LEAVE_GL();
}

void WINAPI wine_glRasterPos4dv(const GLdouble *v ) {
  ENTER_GL();
  glRasterPos4dv(v);
  LEAVE_GL();
}

void WINAPI wine_glRasterPos4fv(const GLfloat *v ) {
  ENTER_GL();
  glRasterPos4fv(v);
  LEAVE_GL();
}

void WINAPI wine_glRasterPos4iv(const GLint *v ) {
  ENTER_GL();
  glRasterPos4iv(v);
  LEAVE_GL();
}

void WINAPI wine_glRasterPos4sv(const GLshort *v ) {
  ENTER_GL();
  glRasterPos4sv(v);
  LEAVE_GL();
}

void WINAPI wine_glRectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2 ) {
  ENTER_GL();
  glRectd(x1, y1, x2, y2);
  LEAVE_GL();
}

void WINAPI wine_glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 ) {
  ENTER_GL();
  glRectf(x1, y1, x2, y2);
  LEAVE_GL();
}

void WINAPI wine_glRecti(GLint x1, GLint y1, GLint x2, GLint y2 ) {
  ENTER_GL();
  glRecti(x1, y1, x2, y2);
  LEAVE_GL();
}

void WINAPI wine_glRects(GLshort x1, GLshort y1, GLshort x2, GLshort y2 ) {
  ENTER_GL();
  glRects(x1, y1, x2, y2);
  LEAVE_GL();
}

void WINAPI wine_glRectdv(const GLdouble *v1, const GLdouble *v2 ) {
  ENTER_GL();
  glRectdv(v1, v2);
  LEAVE_GL();
}

void WINAPI wine_glRectfv(const GLfloat *v1, const GLfloat *v2 ) {
  ENTER_GL();
  glRectfv(v1, v2);
  LEAVE_GL();
}

void WINAPI wine_glRectiv(const GLint *v1, const GLint *v2 ) {
  ENTER_GL();
  glRectiv(v1, v2);
  LEAVE_GL();
}

void WINAPI wine_glRectsv(const GLshort *v1, const GLshort *v2 ) {
  ENTER_GL();
  glRectsv(v1, v2);
  LEAVE_GL();
}

void WINAPI wine_glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr ) {
  ENTER_GL();
  glVertexPointer(size, type, stride, ptr);
  LEAVE_GL();
}

void WINAPI wine_glNormalPointer(GLenum type, GLsizei stride, const GLvoid *ptr ) {
  ENTER_GL();
  glNormalPointer(type, stride, ptr);
  LEAVE_GL();
}

void WINAPI wine_glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr ) {
  ENTER_GL();
  glColorPointer(size, type, stride, ptr);
  LEAVE_GL();
}

void WINAPI wine_glIndexPointer(GLenum type, GLsizei stride, const GLvoid *ptr ) {
  ENTER_GL();
  glIndexPointer(type, stride, ptr);
  LEAVE_GL();
}

void WINAPI wine_glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr ) {
  ENTER_GL();
  glTexCoordPointer(size, type, stride, ptr);
  LEAVE_GL();
}

void WINAPI wine_glEdgeFlagPointer(GLsizei stride, const GLvoid *ptr ) {
  ENTER_GL();
  glEdgeFlagPointer(stride, ptr);
  LEAVE_GL();
}

void WINAPI wine_glGetPointerv(GLenum pname, void **params ) {
  ENTER_GL();
  glGetPointerv(pname, params);
  LEAVE_GL();
}

void WINAPI wine_glArrayElement(GLint i ) {
  ENTER_GL();
  glArrayElement(i);
  LEAVE_GL();
}

void WINAPI wine_glDrawArrays(GLenum mode, GLint first, GLsizei count ) {
  ENTER_GL();
  glDrawArrays(mode, first, count);
  LEAVE_GL();
}

void WINAPI wine_glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices ) {
  ENTER_GL();
  glDrawElements(mode, count, type, indices);
  LEAVE_GL();
}

void WINAPI wine_glInterleavedArrays(GLenum format, GLsizei stride, const GLvoid *pointer ) {
  ENTER_GL();
  glInterleavedArrays(format, stride, pointer);
  LEAVE_GL();
}

void WINAPI wine_glShadeModel(GLenum mode ) {
  ENTER_GL();
  glShadeModel(mode);
  LEAVE_GL();
}

void WINAPI wine_glLightf(GLenum light, GLenum pname, GLfloat param ) {
  ENTER_GL();
  glLightf(light, pname, param);
  LEAVE_GL();
}

void WINAPI wine_glLighti(GLenum light, GLenum pname, GLint param ) {
  ENTER_GL();
  glLighti(light, pname, param);
  LEAVE_GL();
}

void WINAPI wine_glLightfv(GLenum light, GLenum pname, const GLfloat *params ) {
  ENTER_GL();
  glLightfv(light, pname, params);
  LEAVE_GL();
}

void WINAPI wine_glLightiv(GLenum light, GLenum pname, const GLint *params ) {
  ENTER_GL();
  glLightiv(light, pname, params);
  LEAVE_GL();
}

void WINAPI wine_glGetLightfv(GLenum light, GLenum pname, GLfloat *params ) {
  ENTER_GL();
  glGetLightfv(light, pname, params);
  LEAVE_GL();
}

void WINAPI wine_glGetLightiv(GLenum light, GLenum pname, GLint *params ) {
  ENTER_GL();
  glGetLightiv(light, pname, params);
  LEAVE_GL();
}

void WINAPI wine_glLightModelf(GLenum pname, GLfloat param ) {
  ENTER_GL();
  glLightModelf(pname, param);
  LEAVE_GL();
}

void WINAPI wine_glLightModeli(GLenum pname, GLint param ) {
  ENTER_GL();
  glLightModeli(pname, param);
  LEAVE_GL();
}

void WINAPI wine_glLightModelfv(GLenum pname, const GLfloat *params ) {
  ENTER_GL();
  glLightModelfv(pname, params);
  LEAVE_GL();
}

void WINAPI wine_glLightModeliv(GLenum pname, const GLint *params ) {
  ENTER_GL();
  glLightModeliv(pname, params);
  LEAVE_GL();
}

void WINAPI wine_glMaterialf(GLenum face, GLenum pname, GLfloat param ) {
  ENTER_GL();
  glMaterialf(face, pname, param);
  LEAVE_GL();
}

void WINAPI wine_glMateriali(GLenum face, GLenum pname, GLint param ) {
  ENTER_GL();
  glMateriali(face, pname, param);
  LEAVE_GL();
}

void WINAPI wine_glMaterialfv(GLenum face, GLenum pname, const GLfloat *params ) {
  ENTER_GL();
  glMaterialfv(face, pname, params);
  LEAVE_GL();
}

void WINAPI wine_glMaterialiv(GLenum face, GLenum pname, const GLint *params ) {
  ENTER_GL();
  glMaterialiv(face, pname, params);
  LEAVE_GL();
}

void WINAPI wine_glGetMaterialfv(GLenum face, GLenum pname, GLfloat *params ) {
  ENTER_GL();
  glGetMaterialfv(face, pname, params);
  LEAVE_GL();
}

void WINAPI wine_glGetMaterialiv(GLenum face, GLenum pname, GLint *params ) {
  ENTER_GL();
  glGetMaterialiv(face, pname, params);
  LEAVE_GL();
}

void WINAPI wine_glColorMaterial(GLenum face, GLenum mode ) {
  ENTER_GL();
  glColorMaterial(face, mode);
  LEAVE_GL();
}

void WINAPI wine_glPixelZoom(GLfloat xfactor, GLfloat yfactor ) {
  ENTER_GL();
  glPixelZoom(xfactor, yfactor);
  LEAVE_GL();
}

void WINAPI wine_glPixelStoref(GLenum pname, GLfloat param ) {
  ENTER_GL();
  glPixelStoref(pname, param);
  LEAVE_GL();
}

void WINAPI wine_glPixelStorei(GLenum pname, GLint param ) {
  ENTER_GL();
  glPixelStorei(pname, param);
  LEAVE_GL();
}

void WINAPI wine_glPixelTransferf(GLenum pname, GLfloat param ) {
  ENTER_GL();
  glPixelTransferf(pname, param);
  LEAVE_GL();
}

void WINAPI wine_glPixelTransferi(GLenum pname, GLint param ) {
  ENTER_GL();
  glPixelTransferi(pname, param);
  LEAVE_GL();
}

void WINAPI wine_glPixelMapfv(GLenum map, GLint mapsize, const GLfloat *values ) {
  ENTER_GL();
  glPixelMapfv(map, mapsize, values);
  LEAVE_GL();
}

void WINAPI wine_glPixelMapuiv(GLenum map, GLint mapsize, const GLuint *values ) {
  ENTER_GL();
  glPixelMapuiv(map, mapsize, values);
  LEAVE_GL();
}

void WINAPI wine_glPixelMapusv(GLenum map, GLint mapsize, const GLushort *values ) {
  ENTER_GL();
  glPixelMapusv(map, mapsize, values);
  LEAVE_GL();
}

void WINAPI wine_glGetPixelMapfv(GLenum map, GLfloat *values ) {
  ENTER_GL();
  glGetPixelMapfv(map, values);
  LEAVE_GL();
}

void WINAPI wine_glGetPixelMapuiv(GLenum map, GLuint *values ) {
  ENTER_GL();
  glGetPixelMapuiv(map, values);
  LEAVE_GL();
}

void WINAPI wine_glGetPixelMapusv(GLenum map, GLushort *values ) {
  ENTER_GL();
  glGetPixelMapusv(map, values);
  LEAVE_GL();
}

void WINAPI wine_glBitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap ) {
  ENTER_GL();
  glBitmap(width, height, xorig, yorig, xmove, ymove, bitmap);
  LEAVE_GL();
}

void WINAPI wine_glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels ) {
  ENTER_GL();
  glReadPixels(x, y, width, height, format, type, pixels);
  LEAVE_GL();
}

void WINAPI wine_glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels ) {
  ENTER_GL();
  glDrawPixels(width, height, format, type, pixels);
  LEAVE_GL();
}

void WINAPI wine_glCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type ) {
  ENTER_GL();
  glCopyPixels(x, y, width, height, type);
  LEAVE_GL();
}

void WINAPI wine_glStencilFunc(GLenum func, GLint ref, GLuint mask ) {
  ENTER_GL();
  glStencilFunc(func, ref, mask);
  LEAVE_GL();
}

void WINAPI wine_glStencilMask(GLuint mask ) {
  ENTER_GL();
  glStencilMask(mask);
  LEAVE_GL();
}

void WINAPI wine_glStencilOp(GLenum fail, GLenum zfail, GLenum zpass ) {
  ENTER_GL();
  glStencilOp(fail, zfail, zpass);
  LEAVE_GL();
}

void WINAPI wine_glClearStencil(GLint s ) {
  ENTER_GL();
  glClearStencil(s);
  LEAVE_GL();
}

void WINAPI wine_glTexGend(GLenum coord, GLenum pname, GLdouble param ) {
  ENTER_GL();
  glTexGend(coord, pname, param);
  LEAVE_GL();
}

void WINAPI wine_glTexGenf(GLenum coord, GLenum pname, GLfloat param ) {
  ENTER_GL();
  glTexGenf(coord, pname, param);
  LEAVE_GL();
}

void WINAPI wine_glTexGeni(GLenum coord, GLenum pname, GLint param ) {
  ENTER_GL();
  glTexGeni(coord, pname, param);
  LEAVE_GL();
}

void WINAPI wine_glTexGendv(GLenum coord, GLenum pname, const GLdouble *params ) {
  ENTER_GL();
  glTexGendv(coord, pname, params);
  LEAVE_GL();
}

void WINAPI wine_glTexGenfv(GLenum coord, GLenum pname, const GLfloat *params ) {
  ENTER_GL();
  glTexGenfv(coord, pname, params);
  LEAVE_GL();
}

void WINAPI wine_glTexGeniv(GLenum coord, GLenum pname, const GLint *params ) {
  ENTER_GL();
  glTexGeniv(coord, pname, params);
  LEAVE_GL();
}

void WINAPI wine_glGetTexGendv(GLenum coord, GLenum pname, GLdouble *params ) {
  ENTER_GL();
  glGetTexGendv(coord, pname, params);
  LEAVE_GL();
}

void WINAPI wine_glGetTexGenfv(GLenum coord, GLenum pname, GLfloat *params ) {
  ENTER_GL();
  glGetTexGenfv(coord, pname, params);
  LEAVE_GL();
}

void WINAPI wine_glGetTexGeniv(GLenum coord, GLenum pname, GLint *params ) {
  ENTER_GL();
  glGetTexGeniv(coord, pname, params);
  LEAVE_GL();
}

void WINAPI wine_glTexEnvf(GLenum target, GLenum pname, GLfloat param ) {
  ENTER_GL();
  glTexEnvf(target, pname, param);
  LEAVE_GL();
}

void WINAPI wine_glTexEnvi(GLenum target, GLenum pname, GLint param ) {
  ENTER_GL();
  glTexEnvi(target, pname, param);
  LEAVE_GL();
}

void WINAPI wine_glTexEnvfv(GLenum target, GLenum pname, const GLfloat *params ) {
  ENTER_GL();
  glTexEnvfv(target, pname, params);
  LEAVE_GL();
}

void WINAPI wine_glTexEnviv(GLenum target, GLenum pname, const GLint *params ) {
  ENTER_GL();
  glTexEnviv(target, pname, params);
  LEAVE_GL();
}

void WINAPI wine_glGetTexEnvfv(GLenum target, GLenum pname, GLfloat *params ) {
  ENTER_GL();
  glGetTexEnvfv(target, pname, params);
  LEAVE_GL();
}

void WINAPI wine_glGetTexEnviv(GLenum target, GLenum pname, GLint *params ) {
  ENTER_GL();
  glGetTexEnviv(target, pname, params);
  LEAVE_GL();
}

void WINAPI wine_glTexParameterf(GLenum target, GLenum pname, GLfloat param ) {
  ENTER_GL();
  glTexParameterf(target, pname, param);
  LEAVE_GL();
}

void WINAPI wine_glTexParameteri(GLenum target, GLenum pname, GLint param ) {
  ENTER_GL();
  glTexParameteri(target, pname, param);
  LEAVE_GL();
}

void WINAPI wine_glTexParameterfv(GLenum target, GLenum pname, const GLfloat *params ) {
  ENTER_GL();
  glTexParameterfv(target, pname, params);
  LEAVE_GL();
}

void WINAPI wine_glTexParameteriv(GLenum target, GLenum pname, const GLint *params ) {
  ENTER_GL();
  glTexParameteriv(target, pname, params);
  LEAVE_GL();
}

void WINAPI wine_glGetTexParameterfv(GLenum target, GLenum pname, GLfloat *params) {
  ENTER_GL();
  glGetTexParameterfv(target, pname, params);
  LEAVE_GL();
}

void WINAPI wine_glGetTexParameteriv(GLenum target, GLenum pname, GLint *params ) {
  ENTER_GL();
  glGetTexParameteriv(target, pname, params);
  LEAVE_GL();
}

void WINAPI wine_glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat *params ) {
  ENTER_GL();
  glGetTexLevelParameterfv(target, level, pname, params);
  LEAVE_GL();
}

void WINAPI wine_glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params ) {
  ENTER_GL();
  glGetTexLevelParameteriv(target, level, pname, params);
  LEAVE_GL();
}

void WINAPI wine_glTexImage1D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels ) {
  ENTER_GL();
  glTexImage1D(target, level, internalFormat, width, border, format, type, pixels);
  LEAVE_GL();
}

void WINAPI wine_glTexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels ) {
  ENTER_GL();
  glTexImage2D(target, level, internalFormat, width, height, border, format, type, pixels);
  LEAVE_GL();
}

void WINAPI wine_glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels ) {
  ENTER_GL();
  glGetTexImage(target, level, format, type, pixels);
  LEAVE_GL();
}

void WINAPI wine_glGenTextures(GLsizei n, GLuint *textures ) {
  ENTER_GL();
  glGenTextures(n, textures);
  LEAVE_GL();
}

void WINAPI wine_glDeleteTextures(GLsizei n, const GLuint *textures) {
  ENTER_GL();
  glDeleteTextures(n, textures);
  LEAVE_GL();
}

void WINAPI wine_glBindTexture(GLenum target, GLuint texture ) {
  ENTER_GL();
  glBindTexture(target, texture);
  LEAVE_GL();
}

void WINAPI wine_glPrioritizeTextures(GLsizei n, const GLuint *textures, const GLclampf *priorities ) {
  ENTER_GL();
  glPrioritizeTextures(n, textures, priorities);
  LEAVE_GL();
}

GLboolean WINAPI wine_glAreTexturesResident(GLsizei n, const GLuint *textures, GLboolean *residences ) {
  GLboolean ret;
  ENTER_GL();
  ret = glAreTexturesResident(n, textures, residences);
  LEAVE_GL();
  return ret;
}

GLboolean WINAPI wine_glIsTexture(GLuint texture ) {
  GLboolean ret;
  ENTER_GL();
  ret = glIsTexture(texture);
  LEAVE_GL();
  return ret;
}

void WINAPI wine_glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels ) {
  ENTER_GL();
  glTexSubImage1D(target, level, xoffset, width, format, type, pixels);
  LEAVE_GL();
}

void WINAPI wine_glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels ) {
  ENTER_GL();
  glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
  LEAVE_GL();
}

void WINAPI wine_glCopyTexImage1D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border ) {
  ENTER_GL();
  glCopyTexImage1D(target, level, internalformat, x, y, width, border);
  LEAVE_GL();
}

void WINAPI wine_glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border ) {
  ENTER_GL();
  glCopyTexImage2D(target, level, internalformat, x, y, width, height, border);
  LEAVE_GL();
}

void WINAPI wine_glCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width ) {
  ENTER_GL();
  glCopyTexSubImage1D(target, level, xoffset, x, y, width);
  LEAVE_GL();
}

void WINAPI wine_glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height ) {
  ENTER_GL();
  glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
  LEAVE_GL();
}

void WINAPI wine_glMap1d(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points ) {
  ENTER_GL();
  glMap1d(target, u1, u2, stride, order, points);
  LEAVE_GL();
}

void WINAPI wine_glMap1f(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points ) {
  ENTER_GL();
  glMap1f(target, u1, u2, stride, order, points);
  LEAVE_GL();
}

void WINAPI wine_glMap2d(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points ) {
  ENTER_GL();
  glMap2d(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
  LEAVE_GL();
}

void WINAPI wine_glMap2f(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points ) {
  ENTER_GL();
  glMap2f(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
  LEAVE_GL();
}

void WINAPI wine_glGetMapdv(GLenum target, GLenum query, GLdouble *v ) {
  ENTER_GL();
  glGetMapdv(target, query, v);
  LEAVE_GL();
}

void WINAPI wine_glGetMapfv(GLenum target, GLenum query, GLfloat *v ) {
  ENTER_GL();
  glGetMapfv(target, query, v);
  LEAVE_GL();
}

void WINAPI wine_glGetMapiv(GLenum target, GLenum query, GLint *v ) {
  ENTER_GL();
  glGetMapiv(target, query, v);
  LEAVE_GL();
}

void WINAPI wine_glEvalCoord1d(GLdouble u ) {
  ENTER_GL();
  glEvalCoord1d(u);
  LEAVE_GL();
}

void WINAPI wine_glEvalCoord1f(GLfloat u ) {
  ENTER_GL();
  glEvalCoord1f(u);
  LEAVE_GL();
}

void WINAPI wine_glEvalCoord1dv(const GLdouble *u ) {
  ENTER_GL();
  glEvalCoord1dv(u);
  LEAVE_GL();
}

void WINAPI wine_glEvalCoord1fv(const GLfloat *u ) {
  ENTER_GL();
  glEvalCoord1fv(u);
  LEAVE_GL();
}

void WINAPI wine_glEvalCoord2d(GLdouble u, GLdouble v ) {
  ENTER_GL();
  glEvalCoord2d(u, v);
  LEAVE_GL();
}

void WINAPI wine_glEvalCoord2f(GLfloat u, GLfloat v ) {
  ENTER_GL();
  glEvalCoord2f(u, v);
  LEAVE_GL();
}

void WINAPI wine_glEvalCoord2dv(const GLdouble *u ) {
  ENTER_GL();
  glEvalCoord2dv(u);
  LEAVE_GL();
}

void WINAPI wine_glEvalCoord2fv(const GLfloat *u ) {
  ENTER_GL();
  glEvalCoord2fv(u);
  LEAVE_GL();
}

void WINAPI wine_glMapGrid1d(GLint un, GLdouble u1, GLdouble u2 ) {
  ENTER_GL();
  glMapGrid1d(un, u1, u2);
  LEAVE_GL();
}

void WINAPI wine_glMapGrid1f(GLint un, GLfloat u1, GLfloat u2 ) {
  ENTER_GL();
  glMapGrid1f(un, u1, u2);
  LEAVE_GL();
}

void WINAPI wine_glMapGrid2d(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2 ) {
  ENTER_GL();
  glMapGrid2d(un, u1, u2, vn, v1, v2);
  LEAVE_GL();
}

void WINAPI wine_glMapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2 ) {
  ENTER_GL();
  glMapGrid2f(un, u1, u2, vn, v1, v2);
  LEAVE_GL();
}

void WINAPI wine_glEvalPoint1(GLint i ) {
  ENTER_GL();
  glEvalPoint1(i);
  LEAVE_GL();
}

void WINAPI wine_glEvalPoint2(GLint i, GLint j ) {
  ENTER_GL();
  glEvalPoint2(i, j);
  LEAVE_GL();
}

void WINAPI wine_glEvalMesh1(GLenum mode, GLint i1, GLint i2 ) {
  ENTER_GL();
  glEvalMesh1(mode, i1, i2);
  LEAVE_GL();
}

void WINAPI wine_glEvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2 ) {
  ENTER_GL();
  glEvalMesh2(mode, i1, i2, j1, j2);
  LEAVE_GL();
}

void WINAPI wine_glFogf(GLenum pname, GLfloat param ) {
  ENTER_GL();
  glFogf(pname, param);
  LEAVE_GL();
}

void WINAPI wine_glFogi(GLenum pname, GLint param ) {
  ENTER_GL();
  glFogi(pname, param);
  LEAVE_GL();
}

void WINAPI wine_glFogfv(GLenum pname, const GLfloat *params ) {
  ENTER_GL();
  glFogfv(pname, params);
  LEAVE_GL();
}

void WINAPI wine_glFogiv(GLenum pname, const GLint *params ) {
  ENTER_GL();
  glFogiv(pname, params);
  LEAVE_GL();
}

void WINAPI wine_glFeedbackBuffer(GLsizei size, GLenum type, GLfloat *buffer ) {
  ENTER_GL();
  glFeedbackBuffer(size, type, buffer);
  LEAVE_GL();
}

void WINAPI wine_glPassThrough(GLfloat token ) {
  ENTER_GL();
  glPassThrough(token);
  LEAVE_GL();
}

void WINAPI wine_glSelectBuffer(GLsizei size, GLuint *buffer ) {
  ENTER_GL();
  glSelectBuffer(size, buffer);
  LEAVE_GL();
}

void WINAPI wine_glInitNames(void ) {
  ENTER_GL();
  glInitNames();
  LEAVE_GL();
}

void WINAPI wine_glLoadName(GLuint name ) {
  ENTER_GL();
  glLoadName(name);
  LEAVE_GL();
}

void WINAPI wine_glPushName(GLuint name ) {
  ENTER_GL();
  glPushName(name);
  LEAVE_GL();
}

void WINAPI wine_glPopName(void ) {
  ENTER_GL();
  glPopName();
  LEAVE_GL();
}

void WINAPI wine_glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices ) {
  ENTER_GL();
  glDrawRangeElements(mode, start, end, count, type, indices);
  LEAVE_GL();
}

void WINAPI wine_glTexImage3D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels ) {
  ENTER_GL();
  glTexImage3D(target, level, internalFormat, width, height, depth, border, format, type, pixels);
  LEAVE_GL();
}

void WINAPI wine_glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels) {
  ENTER_GL();
  glTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
  LEAVE_GL();
}

void WINAPI wine_glCopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height ) {
  ENTER_GL();
  glCopyTexSubImage3D(target, level, xoffset, yoffset, zoffset, x, y, width, height);
  LEAVE_GL();
}

void WINAPI wine_glColorTable(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *table ) {
  ENTER_GL();
  glColorTable(target, internalformat, width, format, type, table);
  LEAVE_GL();
}

void WINAPI wine_glColorSubTable(GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid *data ) {
  ENTER_GL();
  glColorSubTable(target, start, count, format, type, data);
  LEAVE_GL();
}

void WINAPI wine_glColorTableParameteriv(GLenum target, GLenum pname, const GLint *params) {
  ENTER_GL();
  glColorTableParameteriv(target, pname, params);
  LEAVE_GL();
}

void WINAPI wine_glColorTableParameterfv(GLenum target, GLenum pname, const GLfloat *params) {
  ENTER_GL();
  glColorTableParameterfv(target, pname, params);
  LEAVE_GL();
}

void WINAPI wine_glCopyColorSubTable(GLenum target, GLsizei start, GLint x, GLint y, GLsizei width ) {
  ENTER_GL();
  glCopyColorSubTable(target, start, x, y, width);
  LEAVE_GL();
}

void WINAPI wine_glCopyColorTable(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width ) {
  ENTER_GL();
  glCopyColorTable(target, internalformat, x, y, width);
  LEAVE_GL();
}

void WINAPI wine_glGetColorTable(GLenum target, GLenum format, GLenum type, GLvoid *table ) {
  ENTER_GL();
  glGetColorTable(target, format, type, table);
  LEAVE_GL();
}

void WINAPI wine_glGetColorTableParameterfv(GLenum target, GLenum pname, GLfloat *params ) {
  ENTER_GL();
  glGetColorTableParameterfv(target, pname, params);
  LEAVE_GL();
}

void WINAPI wine_glGetColorTableParameteriv(GLenum target, GLenum pname, GLint *params ) {
  ENTER_GL();
  glGetColorTableParameteriv(target, pname, params);
  LEAVE_GL();
}

void WINAPI wine_glBlendEquation(GLenum mode ) {
  ENTER_GL();
  glBlendEquation(mode);
  LEAVE_GL();
}

void WINAPI wine_glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha ) {
  ENTER_GL();
  glBlendColor(red, green, blue, alpha);
  LEAVE_GL();
}

void WINAPI wine_glHistogram(GLenum target, GLsizei width, GLenum internalformat, GLboolean sink ) {
  ENTER_GL();
  glHistogram(target, width, internalformat, sink);
  LEAVE_GL();
}

void WINAPI wine_glResetHistogram(GLenum target ) {
  ENTER_GL();
  glResetHistogram(target);
  LEAVE_GL();
}

void WINAPI wine_glGetHistogram(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values ) {
  ENTER_GL();
  glGetHistogram(target, reset, format, type, values);
  LEAVE_GL();
}

void WINAPI wine_glGetHistogramParameterfv(GLenum target, GLenum pname, GLfloat *params ) {
  ENTER_GL();
  glGetHistogramParameterfv(target, pname, params);
  LEAVE_GL();
}

void WINAPI wine_glGetHistogramParameteriv(GLenum target, GLenum pname, GLint *params ) {
  ENTER_GL();
  glGetHistogramParameteriv(target, pname, params);
  LEAVE_GL();
}

void WINAPI wine_glMinmax(GLenum target, GLenum internalformat, GLboolean sink ) {
  ENTER_GL();
  glMinmax(target, internalformat, sink);
  LEAVE_GL();
}

void WINAPI wine_glResetMinmax(GLenum target ) {
  ENTER_GL();
  glResetMinmax(target);
  LEAVE_GL();
}

void WINAPI wine_glGetMinmax(GLenum target, GLboolean reset, GLenum format, GLenum types, GLvoid *values ) {
  ENTER_GL();
  glGetMinmax(target, reset, format, types, values);
  LEAVE_GL();
}

void WINAPI wine_glGetMinmaxParameterfv(GLenum target, GLenum pname, GLfloat *params ) {
  ENTER_GL();
  glGetMinmaxParameterfv(target, pname, params);
  LEAVE_GL();
}

void WINAPI wine_glGetMinmaxParameteriv(GLenum target, GLenum pname, GLint *params ) {
  ENTER_GL();
  glGetMinmaxParameteriv(target, pname, params);
  LEAVE_GL();
}

void WINAPI wine_glConvolutionFilter1D(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *image ) {
  ENTER_GL();
  glConvolutionFilter1D(target, internalformat, width, format, type, image);
  LEAVE_GL();
}

void WINAPI wine_glConvolutionFilter2D(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *image ) {
  ENTER_GL();
  glConvolutionFilter2D(target, internalformat, width, height, format, type, image);
  LEAVE_GL();
}

void WINAPI wine_glConvolutionParameterf(GLenum target, GLenum pname, GLfloat params ) {
  ENTER_GL();
  glConvolutionParameterf(target, pname, params);
  LEAVE_GL();
}

void WINAPI wine_glConvolutionParameterfv(GLenum target, GLenum pname, const GLfloat *params ) {
  ENTER_GL();
  glConvolutionParameterfv(target, pname, params);
  LEAVE_GL();
}

void WINAPI wine_glConvolutionParameteri(GLenum target, GLenum pname, GLint params ) {
  ENTER_GL();
  glConvolutionParameteri(target, pname, params);
  LEAVE_GL();
}

void WINAPI wine_glConvolutionParameteriv(GLenum target, GLenum pname, const GLint *params ) {
  ENTER_GL();
  glConvolutionParameteriv(target, pname, params);
  LEAVE_GL();
}

void WINAPI wine_glCopyConvolutionFilter1D(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width ) {
  ENTER_GL();
  glCopyConvolutionFilter1D(target, internalformat, x, y, width);
  LEAVE_GL();
}

void WINAPI wine_glCopyConvolutionFilter2D(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height) {
  ENTER_GL();
  glCopyConvolutionFilter2D(target, internalformat, x, y, width, height);
  LEAVE_GL();
}

void WINAPI wine_glGetConvolutionFilter(GLenum target, GLenum format, GLenum type, GLvoid *image ) {
  ENTER_GL();
  glGetConvolutionFilter(target, format, type, image);
  LEAVE_GL();
}

void WINAPI wine_glGetConvolutionParameterfv(GLenum target, GLenum pname, GLfloat *params ) {
  ENTER_GL();
  glGetConvolutionParameterfv(target, pname, params);
  LEAVE_GL();
}

void WINAPI wine_glGetConvolutionParameteriv(GLenum target, GLenum pname, GLint *params ) {
  ENTER_GL();
  glGetConvolutionParameteriv(target, pname, params);
  LEAVE_GL();
}

void WINAPI wine_glSeparableFilter2D(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *row, const GLvoid *column ) {
  ENTER_GL();
  glSeparableFilter2D(target, internalformat, width, height, format, type, row, column);
  LEAVE_GL();
}

void WINAPI wine_glGetSeparableFilter(GLenum target, GLenum format, GLenum type, GLvoid *row, GLvoid *column, GLvoid *span ) {
  ENTER_GL();
  glGetSeparableFilter(target, format, type, row, column, span);
  LEAVE_GL();
}

void WINAPI wine_glActiveTextureARB(GLenum texture) {
  ENTER_GL();
  glActiveTextureARB(texture);
  LEAVE_GL();
}

void WINAPI wine_glClientActiveTextureARB(GLenum texture) {
  ENTER_GL();
  glClientActiveTextureARB(texture);
  LEAVE_GL();
}

void WINAPI wine_glMultiTexCoord1dARB(GLenum target, GLdouble s) {
  ENTER_GL();
  glMultiTexCoord1dARB(target, s);
  LEAVE_GL();
}

void WINAPI wine_glMultiTexCoord1dvARB(GLenum target, const GLdouble *v) {
  ENTER_GL();
  glMultiTexCoord1dvARB(target, v);
  LEAVE_GL();
}

void WINAPI wine_glMultiTexCoord1fARB(GLenum target, GLfloat s) {
  ENTER_GL();
  glMultiTexCoord1fARB(target, s);
  LEAVE_GL();
}

void WINAPI wine_glMultiTexCoord1fvARB(GLenum target, const GLfloat *v) {
  ENTER_GL();
  glMultiTexCoord1fvARB(target, v);
  LEAVE_GL();
}

void WINAPI wine_glMultiTexCoord1iARB(GLenum target, GLint s) {
  ENTER_GL();
  glMultiTexCoord1iARB(target, s);
  LEAVE_GL();
}

void WINAPI wine_glMultiTexCoord1ivARB(GLenum target, const GLint *v) {
  ENTER_GL();
  glMultiTexCoord1ivARB(target, v);
  LEAVE_GL();
}

void WINAPI wine_glMultiTexCoord1sARB(GLenum target, GLshort s) {
  ENTER_GL();
  glMultiTexCoord1sARB(target, s);
  LEAVE_GL();
}

void WINAPI wine_glMultiTexCoord1svARB(GLenum target, const GLshort *v) {
  ENTER_GL();
  glMultiTexCoord1svARB(target, v);
  LEAVE_GL();
}

void WINAPI wine_glMultiTexCoord2dARB(GLenum target, GLdouble s, GLdouble t) {
  ENTER_GL();
  glMultiTexCoord2dARB(target, s, t);
  LEAVE_GL();
}

void WINAPI wine_glMultiTexCoord2dvARB(GLenum target, const GLdouble *v) {
  ENTER_GL();
  glMultiTexCoord2dvARB(target, v);
  LEAVE_GL();
}

void WINAPI wine_glMultiTexCoord2fARB(GLenum target, GLfloat s, GLfloat t) {
  ENTER_GL();
  glMultiTexCoord2fARB(target, s, t);
  LEAVE_GL();
}

void WINAPI wine_glMultiTexCoord2fvARB(GLenum target, const GLfloat *v) {
  ENTER_GL();
  glMultiTexCoord2fvARB(target, v);
  LEAVE_GL();
}

void WINAPI wine_glMultiTexCoord2iARB(GLenum target, GLint s, GLint t) {
  ENTER_GL();
  glMultiTexCoord2iARB(target, s, t);
  LEAVE_GL();
}

void WINAPI wine_glMultiTexCoord2ivARB(GLenum target, const GLint *v) {
  ENTER_GL();
  glMultiTexCoord2ivARB(target, v);
  LEAVE_GL();
}

void WINAPI wine_glMultiTexCoord2sARB(GLenum target, GLshort s, GLshort t) {
  ENTER_GL();
  glMultiTexCoord2sARB(target, s, t);
  LEAVE_GL();
}

void WINAPI wine_glMultiTexCoord2svARB(GLenum target, const GLshort *v) {
  ENTER_GL();
  glMultiTexCoord2svARB(target, v);
  LEAVE_GL();
}

void WINAPI wine_glMultiTexCoord3dARB(GLenum target, GLdouble s, GLdouble t, GLdouble r) {
  ENTER_GL();
  glMultiTexCoord3dARB(target, s, t, r);
  LEAVE_GL();
}

void WINAPI wine_glMultiTexCoord3dvARB(GLenum target, const GLdouble *v) {
  ENTER_GL();
  glMultiTexCoord3dvARB(target, v);
  LEAVE_GL();
}

void WINAPI wine_glMultiTexCoord3fARB(GLenum target, GLfloat s, GLfloat t, GLfloat r) {
  ENTER_GL();
  glMultiTexCoord3fARB(target, s, t, r);
  LEAVE_GL();
}

void WINAPI wine_glMultiTexCoord3fvARB(GLenum target, const GLfloat *v) {
  ENTER_GL();
  glMultiTexCoord3fvARB(target, v);
  LEAVE_GL();
}

void WINAPI wine_glMultiTexCoord3iARB(GLenum target, GLint s, GLint t, GLint r) {
  ENTER_GL();
  glMultiTexCoord3iARB(target, s, t, r);
  LEAVE_GL();
}

void WINAPI wine_glMultiTexCoord3ivARB(GLenum target, const GLint *v) {
  ENTER_GL();
  glMultiTexCoord3ivARB(target, v);
  LEAVE_GL();
}

void WINAPI wine_glMultiTexCoord3sARB(GLenum target, GLshort s, GLshort t, GLshort r) {
  ENTER_GL();
  glMultiTexCoord3sARB(target, s, t, r);
  LEAVE_GL();
}

void WINAPI wine_glMultiTexCoord3svARB(GLenum target, const GLshort *v) {
  ENTER_GL();
  glMultiTexCoord3svARB(target, v);
  LEAVE_GL();
}

void WINAPI wine_glMultiTexCoord4dARB(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q) {
  ENTER_GL();
  glMultiTexCoord4dARB(target, s, t, r, q);
  LEAVE_GL();
}

void WINAPI wine_glMultiTexCoord4dvARB(GLenum target, const GLdouble *v) {
  ENTER_GL();
  glMultiTexCoord4dvARB(target, v);
  LEAVE_GL();
}

void WINAPI wine_glMultiTexCoord4fARB(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q) {
  ENTER_GL();
  glMultiTexCoord4fARB(target, s, t, r, q);
  LEAVE_GL();
}

void WINAPI wine_glMultiTexCoord4fvARB(GLenum target, const GLfloat *v) {
  ENTER_GL();
  glMultiTexCoord4fvARB(target, v);
  LEAVE_GL();
}

void WINAPI wine_glMultiTexCoord4iARB(GLenum target, GLint s, GLint t, GLint r, GLint q) {
  ENTER_GL();
  glMultiTexCoord4iARB(target, s, t, r, q);
  LEAVE_GL();
}

void WINAPI wine_glMultiTexCoord4ivARB(GLenum target, const GLint *v) {
  ENTER_GL();
  glMultiTexCoord4ivARB(target, v);
  LEAVE_GL();
}

void WINAPI wine_glMultiTexCoord4sARB(GLenum target, GLshort s, GLshort t, GLshort r, GLshort q) {
  ENTER_GL();
  glMultiTexCoord4sARB(target, s, t, r, q);
  LEAVE_GL();
}

void WINAPI wine_glMultiTexCoord4svARB(GLenum target, const GLshort *v) {
  ENTER_GL();
  glMultiTexCoord4svARB(target, v);
  LEAVE_GL();
}

