
name opengl32
type win32

@  stdcall wglCreateContext(long) wglCreateContext
@  stdcall wglCreateLayerContext(long long) wglCreateLayerContext
@  stdcall wglCopyContext(long long long) wglCopyContext
@  stdcall wglDeleteContext(long) wglDeleteContext
@  stdcall wglDescribeLayerPlane(long long long long ptr) wglDescribeLayerPlane
@  stdcall wglGetCurrentContext() wglGetCurrentContext
@  stdcall wglGetCurrentDC() wglGetCurrentDC
@  stdcall wglGetLayerPaletteEntries(long long long long ptr) wglGetLayerPaletteEntries
@  stdcall wglGetProcAddress(str) wglGetProcAddress
@  stdcall wglMakeCurrent(long long) wglMakeCurrent
@  stdcall wglRealizeLayerPalette(long long long) wglRealizeLayerPalette
@  stdcall wglSetLayerPaletteEntries(long long long long ptr) wglSetLayerPaletteEntries
@  stdcall wglShareLists(long long) wglShareLists
@  stdcall wglSwapLayerBuffers(long long) wglSwapLayerBuffers
@  stdcall wglUseFontBitmaps(long long long long) wglUseFontBitmaps
@  stdcall wglUseFontOutlines(long long long long long long long) wglUseFontOutlines
@  stub    glGetLevelParameterfv
@  stub    glGetLevelParameteriv
@  stub    wglUseFontBitmapsA
@  stub    wglUseFontOutlinesA
@  forward wglChoosePixelFormat GDI32.ChoosePixelFormat
@  forward wglDescribePixelFormat GDI32.DescribePixelFormat
@  forward wglGetPixelFormat GDI32.GetPixelFormat
@  forward wglSetPixelFormat GDI32.SetPixelFormat
@  forward wglSwapBuffers GDI32.SwapBuffers
@  stdcall glClearIndex(long ) wine_glClearIndex
@  stdcall glClearColor(long long long long ) wine_glClearColor
@  stdcall glClear(long ) wine_glClear
@  stdcall glIndexMask(long ) wine_glIndexMask
@  stdcall glColorMask(long long long long ) wine_glColorMask
@  stdcall glAlphaFunc(long long ) wine_glAlphaFunc
@  stdcall glBlendFunc(long long ) wine_glBlendFunc
@  stdcall glLogicOp(long ) wine_glLogicOp
@  stdcall glCullFace(long ) wine_glCullFace
@  stdcall glFrontFace(long ) wine_glFrontFace
@  stdcall glPointSize(long ) wine_glPointSize
@  stdcall glLineWidth(long ) wine_glLineWidth
@  stdcall glLineStipple(long long ) wine_glLineStipple
@  stdcall glPolygonMode(long long ) wine_glPolygonMode
@  stdcall glPolygonOffset(long long ) wine_glPolygonOffset
@  stdcall glPolygonStipple(ptr ) wine_glPolygonStipple
@  stdcall glGetPolygonStipple(ptr ) wine_glGetPolygonStipple
@  stdcall glEdgeFlag(long ) wine_glEdgeFlag
@  stdcall glEdgeFlagv(ptr ) wine_glEdgeFlagv
@  stdcall glScissor(long long long long ) wine_glScissor
@  stdcall glClipPlane(long ptr ) wine_glClipPlane
@  stdcall glGetClipPlane(long ptr ) wine_glGetClipPlane
@  stdcall glDrawBuffer(long ) wine_glDrawBuffer
@  stdcall glReadBuffer(long ) wine_glReadBuffer
@  stdcall glEnable(long ) wine_glEnable
@  stdcall glDisable(long ) wine_glDisable
@  stdcall glIsEnabled(long ) wine_glIsEnabled
@  stdcall glEnableClientState(long ) wine_glEnableClientState
@  stdcall glDisableClientState(long ) wine_glDisableClientState
@  stdcall glGetBooleanv(long ptr ) wine_glGetBooleanv
@  stdcall glGetDoublev(long ptr ) wine_glGetDoublev
@  stdcall glGetFloatv(long ptr ) wine_glGetFloatv
@  stdcall glGetIntegerv(long ptr ) wine_glGetIntegerv
@  stdcall glPushAttrib(long ) wine_glPushAttrib
@  stdcall glPopAttrib() wine_glPopAttrib
@  stdcall glPushClientAttrib(long ) wine_glPushClientAttrib
@  stdcall glPopClientAttrib() wine_glPopClientAttrib
@  stdcall glRenderMode(long ) wine_glRenderMode
@  stdcall glGetError() wine_glGetError
@  stdcall glGetString(long ) wine_glGetString
@  stdcall glFinish() wine_glFinish
@  stdcall glFlush() wine_glFlush
@  stdcall glHint(long long ) wine_glHint
@  stdcall glClearDepth(long ) wine_glClearDepth
@  stdcall glDepthFunc(long ) wine_glDepthFunc
@  stdcall glDepthMask(long ) wine_glDepthMask
@  stdcall glDepthRange(long long ) wine_glDepthRange
@  stdcall glClearAccum(long long long long ) wine_glClearAccum
@  stdcall glAccum(long long ) wine_glAccum
@  stdcall glMatrixMode(long ) wine_glMatrixMode
@  stdcall glOrtho(double double double double double double ) wine_glOrtho
@  stdcall glFrustum(double double double double double double ) wine_glFrustum
@  stdcall glViewport(long long long long ) wine_glViewport
@  stdcall glPushMatrix() wine_glPushMatrix
@  stdcall glPopMatrix() wine_glPopMatrix
@  stdcall glLoadIdentity() wine_glLoadIdentity
@  stdcall glLoadMatrixd(ptr ) wine_glLoadMatrixd
@  stdcall glLoadMatrixf(ptr ) wine_glLoadMatrixf
@  stdcall glMultMatrixd(ptr ) wine_glMultMatrixd
@  stdcall glMultMatrixf(ptr ) wine_glMultMatrixf
@  stdcall glRotated(double double double double ) wine_glRotated
@  stdcall glRotatef(long long long long ) wine_glRotatef
@  stdcall glScaled(double double double ) wine_glScaled
@  stdcall glScalef(long long long ) wine_glScalef
@  stdcall glTranslated(double double double ) wine_glTranslated
@  stdcall glTranslatef(long long long ) wine_glTranslatef
@  stdcall glIsList(long ) wine_glIsList
@  stdcall glDeleteLists(long long ) wine_glDeleteLists
@  stdcall glGenLists(long ) wine_glGenLists
@  stdcall glNewList(long long ) wine_glNewList
@  stdcall glEndList() wine_glEndList
@  stdcall glCallList(long ) wine_glCallList
@  stdcall glCallLists(long long ptr ) wine_glCallLists
@  stdcall glListBase(long ) wine_glListBase
@  stdcall glBegin(long ) wine_glBegin
@  stdcall glEnd() wine_glEnd
@  stdcall glVertex2d(double double ) wine_glVertex2d
@  stdcall glVertex2f(long long ) wine_glVertex2f
@  stdcall glVertex2i(long long ) wine_glVertex2i
@  stdcall glVertex2s(long long ) wine_glVertex2s
@  stdcall glVertex3d(double double double ) wine_glVertex3d
@  stdcall glVertex3f(long long long ) wine_glVertex3f
@  stdcall glVertex3i(long long long ) wine_glVertex3i
@  stdcall glVertex3s(long long long ) wine_glVertex3s
@  stdcall glVertex4d(double double double double ) wine_glVertex4d
@  stdcall glVertex4f(long long long long ) wine_glVertex4f
@  stdcall glVertex4i(long long long long ) wine_glVertex4i
@  stdcall glVertex4s(long long long long ) wine_glVertex4s
@  stdcall glVertex2dv(ptr ) wine_glVertex2dv
@  stdcall glVertex2fv(ptr ) wine_glVertex2fv
@  stdcall glVertex2iv(ptr ) wine_glVertex2iv
@  stdcall glVertex2sv(ptr ) wine_glVertex2sv
@  stdcall glVertex3dv(ptr ) wine_glVertex3dv
@  stdcall glVertex3fv(ptr ) wine_glVertex3fv
@  stdcall glVertex3iv(ptr ) wine_glVertex3iv
@  stdcall glVertex3sv(ptr ) wine_glVertex3sv
@  stdcall glVertex4dv(ptr ) wine_glVertex4dv
@  stdcall glVertex4fv(ptr ) wine_glVertex4fv
@  stdcall glVertex4iv(ptr ) wine_glVertex4iv
@  stdcall glVertex4sv(ptr ) wine_glVertex4sv
@  stdcall glNormal3b(long long long ) wine_glNormal3b
@  stdcall glNormal3d(double double double ) wine_glNormal3d
@  stdcall glNormal3f(long long long ) wine_glNormal3f
@  stdcall glNormal3i(long long long ) wine_glNormal3i
@  stdcall glNormal3s(long long long ) wine_glNormal3s
@  stdcall glNormal3bv(ptr ) wine_glNormal3bv
@  stdcall glNormal3dv(ptr ) wine_glNormal3dv
@  stdcall glNormal3fv(ptr ) wine_glNormal3fv
@  stdcall glNormal3iv(ptr ) wine_glNormal3iv
@  stdcall glNormal3sv(ptr ) wine_glNormal3sv
@  stdcall glIndexd(double ) wine_glIndexd
@  stdcall glIndexf(long ) wine_glIndexf
@  stdcall glIndexi(long ) wine_glIndexi
@  stdcall glIndexs(long ) wine_glIndexs
@  stdcall glIndexub(long ) wine_glIndexub
@  stdcall glIndexdv(ptr ) wine_glIndexdv
@  stdcall glIndexfv(ptr ) wine_glIndexfv
@  stdcall glIndexiv(ptr ) wine_glIndexiv
@  stdcall glIndexsv(ptr ) wine_glIndexsv
@  stdcall glIndexubv(ptr ) wine_glIndexubv
@  stdcall glColor3b(long long long ) wine_glColor3b
@  stdcall glColor3d(double double double ) wine_glColor3d
@  stdcall glColor3f(long long long ) wine_glColor3f
@  stdcall glColor3i(long long long ) wine_glColor3i
@  stdcall glColor3s(long long long ) wine_glColor3s
@  stdcall glColor3ub(long long long ) wine_glColor3ub
@  stdcall glColor3ui(long long long ) wine_glColor3ui
@  stdcall glColor3us(long long long ) wine_glColor3us
@  stdcall glColor4b(long long long long ) wine_glColor4b
@  stdcall glColor4d(double double double double ) wine_glColor4d
@  stdcall glColor4f(long long long long ) wine_glColor4f
@  stdcall glColor4i(long long long long ) wine_glColor4i
@  stdcall glColor4s(long long long long ) wine_glColor4s
@  stdcall glColor4ub(long long long long ) wine_glColor4ub
@  stdcall glColor4ui(long long long long ) wine_glColor4ui
@  stdcall glColor4us(long long long long ) wine_glColor4us
@  stdcall glColor3bv(ptr ) wine_glColor3bv
@  stdcall glColor3dv(ptr ) wine_glColor3dv
@  stdcall glColor3fv(ptr ) wine_glColor3fv
@  stdcall glColor3iv(ptr ) wine_glColor3iv
@  stdcall glColor3sv(ptr ) wine_glColor3sv
@  stdcall glColor3ubv(ptr ) wine_glColor3ubv
@  stdcall glColor3uiv(ptr ) wine_glColor3uiv
@  stdcall glColor3usv(ptr ) wine_glColor3usv
@  stdcall glColor4bv(ptr ) wine_glColor4bv
@  stdcall glColor4dv(ptr ) wine_glColor4dv
@  stdcall glColor4fv(ptr ) wine_glColor4fv
@  stdcall glColor4iv(ptr ) wine_glColor4iv
@  stdcall glColor4sv(ptr ) wine_glColor4sv
@  stdcall glColor4ubv(ptr ) wine_glColor4ubv
@  stdcall glColor4uiv(ptr ) wine_glColor4uiv
@  stdcall glColor4usv(ptr ) wine_glColor4usv
@  stdcall glTexCoord1d(double ) wine_glTexCoord1d
@  stdcall glTexCoord1f(long ) wine_glTexCoord1f
@  stdcall glTexCoord1i(long ) wine_glTexCoord1i
@  stdcall glTexCoord1s(long ) wine_glTexCoord1s
@  stdcall glTexCoord2d(double double ) wine_glTexCoord2d
@  stdcall glTexCoord2f(long long ) wine_glTexCoord2f
@  stdcall glTexCoord2i(long long ) wine_glTexCoord2i
@  stdcall glTexCoord2s(long long ) wine_glTexCoord2s
@  stdcall glTexCoord3d(double double double ) wine_glTexCoord3d
@  stdcall glTexCoord3f(long long long ) wine_glTexCoord3f
@  stdcall glTexCoord3i(long long long ) wine_glTexCoord3i
@  stdcall glTexCoord3s(long long long ) wine_glTexCoord3s
@  stdcall glTexCoord4d(double double double double ) wine_glTexCoord4d
@  stdcall glTexCoord4f(long long long long ) wine_glTexCoord4f
@  stdcall glTexCoord4i(long long long long ) wine_glTexCoord4i
@  stdcall glTexCoord4s(long long long long ) wine_glTexCoord4s
@  stdcall glTexCoord1dv(ptr ) wine_glTexCoord1dv
@  stdcall glTexCoord1fv(ptr ) wine_glTexCoord1fv
@  stdcall glTexCoord1iv(ptr ) wine_glTexCoord1iv
@  stdcall glTexCoord1sv(ptr ) wine_glTexCoord1sv
@  stdcall glTexCoord2dv(ptr ) wine_glTexCoord2dv
@  stdcall glTexCoord2fv(ptr ) wine_glTexCoord2fv
@  stdcall glTexCoord2iv(ptr ) wine_glTexCoord2iv
@  stdcall glTexCoord2sv(ptr ) wine_glTexCoord2sv
@  stdcall glTexCoord3dv(ptr ) wine_glTexCoord3dv
@  stdcall glTexCoord3fv(ptr ) wine_glTexCoord3fv
@  stdcall glTexCoord3iv(ptr ) wine_glTexCoord3iv
@  stdcall glTexCoord3sv(ptr ) wine_glTexCoord3sv
@  stdcall glTexCoord4dv(ptr ) wine_glTexCoord4dv
@  stdcall glTexCoord4fv(ptr ) wine_glTexCoord4fv
@  stdcall glTexCoord4iv(ptr ) wine_glTexCoord4iv
@  stdcall glTexCoord4sv(ptr ) wine_glTexCoord4sv
@  stdcall glRasterPos2d(double double ) wine_glRasterPos2d
@  stdcall glRasterPos2f(long long ) wine_glRasterPos2f
@  stdcall glRasterPos2i(long long ) wine_glRasterPos2i
@  stdcall glRasterPos2s(long long ) wine_glRasterPos2s
@  stdcall glRasterPos3d(double double double ) wine_glRasterPos3d
@  stdcall glRasterPos3f(long long long ) wine_glRasterPos3f
@  stdcall glRasterPos3i(long long long ) wine_glRasterPos3i
@  stdcall glRasterPos3s(long long long ) wine_glRasterPos3s
@  stdcall glRasterPos4d(double double double double ) wine_glRasterPos4d
@  stdcall glRasterPos4f(long long long long ) wine_glRasterPos4f
@  stdcall glRasterPos4i(long long long long ) wine_glRasterPos4i
@  stdcall glRasterPos4s(long long long long ) wine_glRasterPos4s
@  stdcall glRasterPos2dv(ptr ) wine_glRasterPos2dv
@  stdcall glRasterPos2fv(ptr ) wine_glRasterPos2fv
@  stdcall glRasterPos2iv(ptr ) wine_glRasterPos2iv
@  stdcall glRasterPos2sv(ptr ) wine_glRasterPos2sv
@  stdcall glRasterPos3dv(ptr ) wine_glRasterPos3dv
@  stdcall glRasterPos3fv(ptr ) wine_glRasterPos3fv
@  stdcall glRasterPos3iv(ptr ) wine_glRasterPos3iv
@  stdcall glRasterPos3sv(ptr ) wine_glRasterPos3sv
@  stdcall glRasterPos4dv(ptr ) wine_glRasterPos4dv
@  stdcall glRasterPos4fv(ptr ) wine_glRasterPos4fv
@  stdcall glRasterPos4iv(ptr ) wine_glRasterPos4iv
@  stdcall glRasterPos4sv(ptr ) wine_glRasterPos4sv
@  stdcall glRectd(double double double double ) wine_glRectd
@  stdcall glRectf(long long long long ) wine_glRectf
@  stdcall glRecti(long long long long ) wine_glRecti
@  stdcall glRects(long long long long ) wine_glRects
@  stdcall glRectdv(ptr ptr ) wine_glRectdv
@  stdcall glRectfv(ptr ptr ) wine_glRectfv
@  stdcall glRectiv(ptr ptr ) wine_glRectiv
@  stdcall glRectsv(ptr ptr ) wine_glRectsv
@  stdcall glVertexPointer(long long long ptr ) wine_glVertexPointer
@  stdcall glNormalPointer(long long ptr ) wine_glNormalPointer
@  stdcall glColorPointer(long long long ptr ) wine_glColorPointer
@  stdcall glIndexPointer(long long ptr ) wine_glIndexPointer
@  stdcall glTexCoordPointer(long long long ptr ) wine_glTexCoordPointer
@  stdcall glEdgeFlagPointer(long ptr ) wine_glEdgeFlagPointer
@  stdcall glGetPointerv(long ptr ) wine_glGetPointerv
@  stdcall glArrayElement(long ) wine_glArrayElement
@  stdcall glDrawArrays(long long long ) wine_glDrawArrays
@  stdcall glDrawElements(long long long ptr ) wine_glDrawElements
@  stdcall glInterleavedArrays(long long ptr ) wine_glInterleavedArrays
@  stdcall glShadeModel(long ) wine_glShadeModel
@  stdcall glLightf(long long long ) wine_glLightf
@  stdcall glLighti(long long long ) wine_glLighti
@  stdcall glLightfv(long long ptr ) wine_glLightfv
@  stdcall glLightiv(long long ptr ) wine_glLightiv
@  stdcall glGetLightfv(long long ptr ) wine_glGetLightfv
@  stdcall glGetLightiv(long long ptr ) wine_glGetLightiv
@  stdcall glLightModelf(long long ) wine_glLightModelf
@  stdcall glLightModeli(long long ) wine_glLightModeli
@  stdcall glLightModelfv(long ptr ) wine_glLightModelfv
@  stdcall glLightModeliv(long ptr ) wine_glLightModeliv
@  stdcall glMaterialf(long long long ) wine_glMaterialf
@  stdcall glMateriali(long long long ) wine_glMateriali
@  stdcall glMaterialfv(long long ptr ) wine_glMaterialfv
@  stdcall glMaterialiv(long long ptr ) wine_glMaterialiv
@  stdcall glGetMaterialfv(long long ptr ) wine_glGetMaterialfv
@  stdcall glGetMaterialiv(long long ptr ) wine_glGetMaterialiv
@  stdcall glColorMaterial(long long ) wine_glColorMaterial
@  stdcall glPixelZoom(long long ) wine_glPixelZoom
@  stdcall glPixelStoref(long long ) wine_glPixelStoref
@  stdcall glPixelStorei(long long ) wine_glPixelStorei
@  stdcall glPixelTransferf(long long ) wine_glPixelTransferf
@  stdcall glPixelTransferi(long long ) wine_glPixelTransferi
@  stdcall glPixelMapfv(long long ptr ) wine_glPixelMapfv
@  stdcall glPixelMapuiv(long long ptr ) wine_glPixelMapuiv
@  stdcall glPixelMapusv(long long ptr ) wine_glPixelMapusv
@  stdcall glGetPixelMapfv(long ptr ) wine_glGetPixelMapfv
@  stdcall glGetPixelMapuiv(long ptr ) wine_glGetPixelMapuiv
@  stdcall glGetPixelMapusv(long ptr ) wine_glGetPixelMapusv
@  stdcall glBitmap(long long long long long long ptr ) wine_glBitmap
@  stdcall glReadPixels(long long long long long long ptr ) wine_glReadPixels
@  stdcall glDrawPixels(long long long long ptr ) wine_glDrawPixels
@  stdcall glCopyPixels(long long long long long ) wine_glCopyPixels
@  stdcall glStencilFunc(long long long ) wine_glStencilFunc
@  stdcall glStencilMask(long ) wine_glStencilMask
@  stdcall glStencilOp(long long long ) wine_glStencilOp
@  stdcall glClearStencil(long ) wine_glClearStencil
@  stdcall glTexGend(long long double ) wine_glTexGend
@  stdcall glTexGenf(long long long ) wine_glTexGenf
@  stdcall glTexGeni(long long long ) wine_glTexGeni
@  stdcall glTexGendv(long long ptr ) wine_glTexGendv
@  stdcall glTexGenfv(long long ptr ) wine_glTexGenfv
@  stdcall glTexGeniv(long long ptr ) wine_glTexGeniv
@  stdcall glGetTexGendv(long long ptr ) wine_glGetTexGendv
@  stdcall glGetTexGenfv(long long ptr ) wine_glGetTexGenfv
@  stdcall glGetTexGeniv(long long ptr ) wine_glGetTexGeniv
@  stdcall glTexEnvf(long long long ) wine_glTexEnvf
@  stdcall glTexEnvi(long long long ) wine_glTexEnvi
@  stdcall glTexEnvfv(long long ptr ) wine_glTexEnvfv
@  stdcall glTexEnviv(long long ptr ) wine_glTexEnviv
@  stdcall glGetTexEnvfv(long long ptr ) wine_glGetTexEnvfv
@  stdcall glGetTexEnviv(long long ptr ) wine_glGetTexEnviv
@  stdcall glTexParameterf(long long long ) wine_glTexParameterf
@  stdcall glTexParameteri(long long long ) wine_glTexParameteri
@  stdcall glTexParameterfv(long long ptr ) wine_glTexParameterfv
@  stdcall glTexParameteriv(long long ptr ) wine_glTexParameteriv
@  stdcall glGetTexParameterfv(long long ptr ) wine_glGetTexParameterfv
@  stdcall glGetTexParameteriv(long long ptr ) wine_glGetTexParameteriv
@  stdcall glGetTexLevelParameterfv(long long long ptr ) wine_glGetTexLevelParameterfv
@  stdcall glGetTexLevelParameteriv(long long long ptr ) wine_glGetTexLevelParameteriv
@  stdcall glTexImage1D(long long long long long long long ptr ) wine_glTexImage1D
@  stdcall glTexImage2D(long long long long long long long long ptr ) wine_glTexImage2D
@  stdcall glGetTexImage(long long long long ptr ) wine_glGetTexImage
@  stdcall glGenTextures(long ptr ) wine_glGenTextures
@  stdcall glDeleteTextures(long ptr ) wine_glDeleteTextures
@  stdcall glBindTexture(long long ) wine_glBindTexture
@  stdcall glPrioritizeTextures(long ptr ptr ) wine_glPrioritizeTextures
@  stdcall glAreTexturesResident(long ptr ptr ) wine_glAreTexturesResident
@  stdcall glIsTexture(long ) wine_glIsTexture
@  stdcall glTexSubImage1D(long long long long long long ptr ) wine_glTexSubImage1D
@  stdcall glTexSubImage2D(long long long long long long long long ptr ) wine_glTexSubImage2D
@  stdcall glCopyTexImage1D(long long long long long long long ) wine_glCopyTexImage1D
@  stdcall glCopyTexImage2D(long long long long long long long long ) wine_glCopyTexImage2D
@  stdcall glCopyTexSubImage1D(long long long long long long ) wine_glCopyTexSubImage1D
@  stdcall glCopyTexSubImage2D(long long long long long long long long ) wine_glCopyTexSubImage2D
@  stdcall glMap1d(long double double long long ptr ) wine_glMap1d
@  stdcall glMap1f(long long long long long ptr ) wine_glMap1f
@  stdcall glMap2d(long double double long long double double long long ptr ) wine_glMap2d
@  stdcall glMap2f(long long long long long long long long long ptr ) wine_glMap2f
@  stdcall glGetMapdv(long long ptr ) wine_glGetMapdv
@  stdcall glGetMapfv(long long ptr ) wine_glGetMapfv
@  stdcall glGetMapiv(long long ptr ) wine_glGetMapiv
@  stdcall glEvalCoord1d(double ) wine_glEvalCoord1d
@  stdcall glEvalCoord1f(long ) wine_glEvalCoord1f
@  stdcall glEvalCoord1dv(ptr ) wine_glEvalCoord1dv
@  stdcall glEvalCoord1fv(ptr ) wine_glEvalCoord1fv
@  stdcall glEvalCoord2d(double double ) wine_glEvalCoord2d
@  stdcall glEvalCoord2f(long long ) wine_glEvalCoord2f
@  stdcall glEvalCoord2dv(ptr ) wine_glEvalCoord2dv
@  stdcall glEvalCoord2fv(ptr ) wine_glEvalCoord2fv
@  stdcall glMapGrid1d(long double double ) wine_glMapGrid1d
@  stdcall glMapGrid1f(long long long ) wine_glMapGrid1f
@  stdcall glMapGrid2d(long double double long double double ) wine_glMapGrid2d
@  stdcall glMapGrid2f(long long long long long long ) wine_glMapGrid2f
@  stdcall glEvalPoint1(long ) wine_glEvalPoint1
@  stdcall glEvalPoint2(long long ) wine_glEvalPoint2
@  stdcall glEvalMesh1(long long long ) wine_glEvalMesh1
@  stdcall glEvalMesh2(long long long long long ) wine_glEvalMesh2
@  stdcall glFogf(long long ) wine_glFogf
@  stdcall glFogi(long long ) wine_glFogi
@  stdcall glFogfv(long ptr ) wine_glFogfv
@  stdcall glFogiv(long ptr ) wine_glFogiv
@  stdcall glFeedbackBuffer(long long ptr ) wine_glFeedbackBuffer
@  stdcall glPassThrough(long ) wine_glPassThrough
@  stdcall glSelectBuffer(long ptr ) wine_glSelectBuffer
@  stdcall glInitNames() wine_glInitNames
@  stdcall glLoadName(long ) wine_glLoadName
@  stdcall glPushName(long ) wine_glPushName
@  stdcall glPopName() wine_glPopName
@  stdcall glDrawRangeElements(long long long long long ptr ) wine_glDrawRangeElements
@  stdcall glTexImage3D(long long long long long long long long long ptr ) wine_glTexImage3D
@  stdcall glTexSubImage3D(long long long long long long long long long long ptr ) wine_glTexSubImage3D
@  stdcall glCopyTexSubImage3D(long long long long long long long long long ) wine_glCopyTexSubImage3D
@  stdcall glColorTable(long long long long long ptr ) wine_glColorTable
@  stdcall glColorSubTable(long long long long long ptr ) wine_glColorSubTable
@  stdcall glColorTableParameteriv(long long ptr ) wine_glColorTableParameteriv
@  stdcall glColorTableParameterfv(long long ptr ) wine_glColorTableParameterfv
@  stdcall glCopyColorSubTable(long long long long long ) wine_glCopyColorSubTable
@  stdcall glCopyColorTable(long long long long long ) wine_glCopyColorTable
@  stdcall glGetColorTable(long long long ptr ) wine_glGetColorTable
@  stdcall glGetColorTableParameterfv(long long ptr ) wine_glGetColorTableParameterfv
@  stdcall glGetColorTableParameteriv(long long ptr ) wine_glGetColorTableParameteriv
@  stdcall glBlendEquation(long ) wine_glBlendEquation
@  stdcall glBlendColor(long long long long ) wine_glBlendColor
@  stdcall glHistogram(long long long long ) wine_glHistogram
@  stdcall glResetHistogram(long ) wine_glResetHistogram
@  stdcall glGetHistogram(long long long long ptr ) wine_glGetHistogram
@  stdcall glGetHistogramParameterfv(long long ptr ) wine_glGetHistogramParameterfv
@  stdcall glGetHistogramParameteriv(long long ptr ) wine_glGetHistogramParameteriv
@  stdcall glMinmax(long long long ) wine_glMinmax
@  stdcall glResetMinmax(long ) wine_glResetMinmax
@  stdcall glGetMinmax(long long long long ptr ) wine_glGetMinmax
@  stdcall glGetMinmaxParameterfv(long long ptr ) wine_glGetMinmaxParameterfv
@  stdcall glGetMinmaxParameteriv(long long ptr ) wine_glGetMinmaxParameteriv
@  stdcall glConvolutionFilter1D(long long long long long ptr ) wine_glConvolutionFilter1D
@  stdcall glConvolutionFilter2D(long long long long long long ptr ) wine_glConvolutionFilter2D
@  stdcall glConvolutionParameterf(long long long ) wine_glConvolutionParameterf
@  stdcall glConvolutionParameterfv(long long ptr ) wine_glConvolutionParameterfv
@  stdcall glConvolutionParameteri(long long long ) wine_glConvolutionParameteri
@  stdcall glConvolutionParameteriv(long long ptr ) wine_glConvolutionParameteriv
@  stdcall glCopyConvolutionFilter1D(long long long long long ) wine_glCopyConvolutionFilter1D
@  stdcall glCopyConvolutionFilter2D(long long long long long long ) wine_glCopyConvolutionFilter2D
@  stdcall glGetConvolutionFilter(long long long ptr ) wine_glGetConvolutionFilter
@  stdcall glGetConvolutionParameterfv(long long ptr ) wine_glGetConvolutionParameterfv
@  stdcall glGetConvolutionParameteriv(long long ptr ) wine_glGetConvolutionParameteriv
@  stdcall glSeparableFilter2D(long long long long long long ptr ptr ) wine_glSeparableFilter2D
@  stdcall glGetSeparableFilter(long long long ptr ptr ptr ) wine_glGetSeparableFilter
@  stdcall glActiveTextureARB(long ) wine_glActiveTextureARB
@  stdcall glClientActiveTextureARB(long ) wine_glClientActiveTextureARB
@  stdcall glMultiTexCoord1dARB(long double ) wine_glMultiTexCoord1dARB
@  stdcall glMultiTexCoord1dvARB(long ptr ) wine_glMultiTexCoord1dvARB
@  stdcall glMultiTexCoord1fARB(long long ) wine_glMultiTexCoord1fARB
@  stdcall glMultiTexCoord1fvARB(long ptr ) wine_glMultiTexCoord1fvARB
@  stdcall glMultiTexCoord1iARB(long long ) wine_glMultiTexCoord1iARB
@  stdcall glMultiTexCoord1ivARB(long ptr ) wine_glMultiTexCoord1ivARB
@  stdcall glMultiTexCoord1sARB(long long ) wine_glMultiTexCoord1sARB
@  stdcall glMultiTexCoord1svARB(long ptr ) wine_glMultiTexCoord1svARB
@  stdcall glMultiTexCoord2dARB(long double double ) wine_glMultiTexCoord2dARB
@  stdcall glMultiTexCoord2dvARB(long ptr ) wine_glMultiTexCoord2dvARB
@  stdcall glMultiTexCoord2fARB(long long long ) wine_glMultiTexCoord2fARB
@  stdcall glMultiTexCoord2fvARB(long ptr ) wine_glMultiTexCoord2fvARB
@  stdcall glMultiTexCoord2iARB(long long long ) wine_glMultiTexCoord2iARB
@  stdcall glMultiTexCoord2ivARB(long ptr ) wine_glMultiTexCoord2ivARB
@  stdcall glMultiTexCoord2sARB(long long long ) wine_glMultiTexCoord2sARB
@  stdcall glMultiTexCoord2svARB(long ptr ) wine_glMultiTexCoord2svARB
@  stdcall glMultiTexCoord3dARB(long double double double ) wine_glMultiTexCoord3dARB
@  stdcall glMultiTexCoord3dvARB(long ptr ) wine_glMultiTexCoord3dvARB
@  stdcall glMultiTexCoord3fARB(long long long long ) wine_glMultiTexCoord3fARB
@  stdcall glMultiTexCoord3fvARB(long ptr ) wine_glMultiTexCoord3fvARB
@  stdcall glMultiTexCoord3iARB(long long long long ) wine_glMultiTexCoord3iARB
@  stdcall glMultiTexCoord3ivARB(long ptr ) wine_glMultiTexCoord3ivARB
@  stdcall glMultiTexCoord3sARB(long long long long ) wine_glMultiTexCoord3sARB
@  stdcall glMultiTexCoord3svARB(long ptr ) wine_glMultiTexCoord3svARB
@  stdcall glMultiTexCoord4dARB(long double double double double ) wine_glMultiTexCoord4dARB
@  stdcall glMultiTexCoord4dvARB(long ptr ) wine_glMultiTexCoord4dvARB
@  stdcall glMultiTexCoord4fARB(long long long long long ) wine_glMultiTexCoord4fARB
@  stdcall glMultiTexCoord4fvARB(long ptr ) wine_glMultiTexCoord4fvARB
@  stdcall glMultiTexCoord4iARB(long long long long long ) wine_glMultiTexCoord4iARB
@  stdcall glMultiTexCoord4ivARB(long ptr ) wine_glMultiTexCoord4ivARB
@  stdcall glMultiTexCoord4sARB(long long long long long ) wine_glMultiTexCoord4sARB
@  stdcall glMultiTexCoord4svARB(long ptr ) wine_glMultiTexCoord4svARB
