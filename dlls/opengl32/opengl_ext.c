
/* Auto-generated file... Do not edit ! */

#include "config.h"
#include "opengl_ext.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(opengl);

static void (*func_glActiveStencilFaceEXT)( GLenum );
static void (*func_glActiveTexture)( GLenum );
static void (*func_glActiveTextureARB)( GLenum );
static void (*func_glAlphaFragmentOp1ATI)( GLenum, GLuint, GLuint, GLuint, GLuint, GLuint );
static void (*func_glAlphaFragmentOp2ATI)( GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint );
static void (*func_glAlphaFragmentOp3ATI)( GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint );
static void (*func_glApplyTextureEXT)( GLenum );
static GLboolean (*func_glAreProgramsResidentNV)( GLsizei, GLuint*, GLboolean* );
static GLboolean (*func_glAreTexturesResidentEXT)( GLsizei, GLuint*, GLboolean* );
static void (*func_glArrayElementEXT)( GLint );
static void (*func_glArrayObjectATI)( GLenum, GLint, GLenum, GLsizei, GLuint, GLuint );
static void (*func_glAsyncMarkerSGIX)( GLuint );
static void (*func_glAttachObjectARB)( unsigned int, unsigned int );
static void (*func_glAttachShader)( GLuint, GLuint );
static void (*func_glBeginFragmentShaderATI)( void );
static void (*func_glBeginOcclusionQueryNV)( GLuint );
static void (*func_glBeginQuery)( GLenum, GLuint );
static void (*func_glBeginQueryARB)( GLenum, GLuint );
static void (*func_glBeginVertexShaderEXT)( void );
static void (*func_glBindAttribLocation)( GLuint, GLuint, char* );
static void (*func_glBindAttribLocationARB)( unsigned int, GLuint, char* );
static void (*func_glBindBuffer)( GLenum, GLuint );
static void (*func_glBindBufferARB)( GLenum, GLuint );
static void (*func_glBindFragmentShaderATI)( GLuint );
static void (*func_glBindFramebufferEXT)( GLenum, GLuint );
static GLuint (*func_glBindLightParameterEXT)( GLenum, GLenum );
static GLuint (*func_glBindMaterialParameterEXT)( GLenum, GLenum );
static GLuint (*func_glBindParameterEXT)( GLenum );
static void (*func_glBindProgramARB)( GLenum, GLuint );
static void (*func_glBindProgramNV)( GLenum, GLuint );
static void (*func_glBindRenderbufferEXT)( GLenum, GLuint );
static GLuint (*func_glBindTexGenParameterEXT)( GLenum, GLenum, GLenum );
static void (*func_glBindTextureEXT)( GLenum, GLuint );
static GLuint (*func_glBindTextureUnitParameterEXT)( GLenum, GLenum );
static void (*func_glBindVertexArrayAPPLE)( GLuint );
static void (*func_glBindVertexShaderEXT)( GLuint );
static void (*func_glBinormal3bEXT)( GLbyte, GLbyte, GLbyte );
static void (*func_glBinormal3bvEXT)( GLbyte* );
static void (*func_glBinormal3dEXT)( GLdouble, GLdouble, GLdouble );
static void (*func_glBinormal3dvEXT)( GLdouble* );
static void (*func_glBinormal3fEXT)( GLfloat, GLfloat, GLfloat );
static void (*func_glBinormal3fvEXT)( GLfloat* );
static void (*func_glBinormal3iEXT)( GLint, GLint, GLint );
static void (*func_glBinormal3ivEXT)( GLint* );
static void (*func_glBinormal3sEXT)( GLshort, GLshort, GLshort );
static void (*func_glBinormal3svEXT)( GLshort* );
static void (*func_glBinormalPointerEXT)( GLenum, GLsizei, GLvoid* );
static void (*func_glBlendColorEXT)( GLclampf, GLclampf, GLclampf, GLclampf );
static void (*func_glBlendEquationEXT)( GLenum );
static void (*func_glBlendEquationSeparate)( GLenum, GLenum );
static void (*func_glBlendEquationSeparateEXT)( GLenum, GLenum );
static void (*func_glBlendFuncSeparate)( GLenum, GLenum, GLenum, GLenum );
static void (*func_glBlendFuncSeparateEXT)( GLenum, GLenum, GLenum, GLenum );
static void (*func_glBlendFuncSeparateINGR)( GLenum, GLenum, GLenum, GLenum );
static void (*func_glBufferData)( GLenum, ptrdiff_t, GLvoid*, GLenum );
static void (*func_glBufferDataARB)( GLenum, ptrdiff_t, GLvoid*, GLenum );
static GLuint (*func_glBufferRegionEnabled)( void );
static void (*func_glBufferSubData)( GLenum, ptrdiff_t, ptrdiff_t, GLvoid* );
static void (*func_glBufferSubDataARB)( GLenum, ptrdiff_t, ptrdiff_t, GLvoid* );
static GLenum (*func_glCheckFramebufferStatusEXT)( GLenum );
static void (*func_glClampColorARB)( GLenum, GLenum );
static void (*func_glClientActiveTexture)( GLenum );
static void (*func_glClientActiveTextureARB)( GLenum );
static void (*func_glClientActiveVertexStreamATI)( GLenum );
static void (*func_glColor3fVertex3fSUN)( GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat );
static void (*func_glColor3fVertex3fvSUN)( GLfloat*, GLfloat* );
static void (*func_glColor3hNV)( unsigned short, unsigned short, unsigned short );
static void (*func_glColor3hvNV)( unsigned short* );
static void (*func_glColor4fNormal3fVertex3fSUN)( GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat );
static void (*func_glColor4fNormal3fVertex3fvSUN)( GLfloat*, GLfloat*, GLfloat* );
static void (*func_glColor4hNV)( unsigned short, unsigned short, unsigned short, unsigned short );
static void (*func_glColor4hvNV)( unsigned short* );
static void (*func_glColor4ubVertex2fSUN)( GLubyte, GLubyte, GLubyte, GLubyte, GLfloat, GLfloat );
static void (*func_glColor4ubVertex2fvSUN)( GLubyte*, GLfloat* );
static void (*func_glColor4ubVertex3fSUN)( GLubyte, GLubyte, GLubyte, GLubyte, GLfloat, GLfloat, GLfloat );
static void (*func_glColor4ubVertex3fvSUN)( GLubyte*, GLfloat* );
static void (*func_glColorFragmentOp1ATI)( GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint );
static void (*func_glColorFragmentOp2ATI)( GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint );
static void (*func_glColorFragmentOp3ATI)( GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint );
static void (*func_glColorPointerEXT)( GLint, GLenum, GLsizei, GLsizei, GLvoid* );
static void (*func_glColorPointerListIBM)( GLint, GLenum, GLint, GLvoid**, GLint );
static void (*func_glColorPointervINTEL)( GLint, GLenum, GLvoid** );
static void (*func_glColorSubTableEXT)( GLenum, GLsizei, GLsizei, GLenum, GLenum, GLvoid* );
static void (*func_glColorTableEXT)( GLenum, GLenum, GLsizei, GLenum, GLenum, GLvoid* );
static void (*func_glColorTableParameterfvSGI)( GLenum, GLenum, GLfloat* );
static void (*func_glColorTableParameterivSGI)( GLenum, GLenum, GLint* );
static void (*func_glColorTableSGI)( GLenum, GLenum, GLsizei, GLenum, GLenum, GLvoid* );
static void (*func_glCombinerInputNV)( GLenum, GLenum, GLenum, GLenum, GLenum, GLenum );
static void (*func_glCombinerOutputNV)( GLenum, GLenum, GLenum, GLenum, GLenum, GLenum, GLenum, GLboolean, GLboolean, GLboolean );
static void (*func_glCombinerParameterfNV)( GLenum, GLfloat );
static void (*func_glCombinerParameterfvNV)( GLenum, GLfloat* );
static void (*func_glCombinerParameteriNV)( GLenum, GLint );
static void (*func_glCombinerParameterivNV)( GLenum, GLint* );
static void (*func_glCombinerStageParameterfvNV)( GLenum, GLenum, GLfloat* );
static void (*func_glCompileShader)( GLuint );
static void (*func_glCompileShaderARB)( unsigned int );
static void (*func_glCompressedTexImage1D)( GLenum, GLint, GLenum, GLsizei, GLint, GLsizei, GLvoid* );
static void (*func_glCompressedTexImage1DARB)( GLenum, GLint, GLenum, GLsizei, GLint, GLsizei, GLvoid* );
static void (*func_glCompressedTexImage2D)( GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, GLvoid* );
static void (*func_glCompressedTexImage2DARB)( GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, GLvoid* );
static void (*func_glCompressedTexImage3D)( GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLsizei, GLvoid* );
static void (*func_glCompressedTexImage3DARB)( GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLsizei, GLvoid* );
static void (*func_glCompressedTexSubImage1D)( GLenum, GLint, GLint, GLsizei, GLenum, GLsizei, GLvoid* );
static void (*func_glCompressedTexSubImage1DARB)( GLenum, GLint, GLint, GLsizei, GLenum, GLsizei, GLvoid* );
static void (*func_glCompressedTexSubImage2D)( GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, GLvoid* );
static void (*func_glCompressedTexSubImage2DARB)( GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, GLvoid* );
static void (*func_glCompressedTexSubImage3D)( GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, GLvoid* );
static void (*func_glCompressedTexSubImage3DARB)( GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, GLvoid* );
static void (*func_glConvolutionFilter1DEXT)( GLenum, GLenum, GLsizei, GLenum, GLenum, GLvoid* );
static void (*func_glConvolutionFilter2DEXT)( GLenum, GLenum, GLsizei, GLsizei, GLenum, GLenum, GLvoid* );
static void (*func_glConvolutionParameterfEXT)( GLenum, GLenum, GLfloat );
static void (*func_glConvolutionParameterfvEXT)( GLenum, GLenum, GLfloat* );
static void (*func_glConvolutionParameteriEXT)( GLenum, GLenum, GLint );
static void (*func_glConvolutionParameterivEXT)( GLenum, GLenum, GLint* );
static void (*func_glCopyColorSubTableEXT)( GLenum, GLsizei, GLint, GLint, GLsizei );
static void (*func_glCopyColorTableSGI)( GLenum, GLenum, GLint, GLint, GLsizei );
static void (*func_glCopyConvolutionFilter1DEXT)( GLenum, GLenum, GLint, GLint, GLsizei );
static void (*func_glCopyConvolutionFilter2DEXT)( GLenum, GLenum, GLint, GLint, GLsizei, GLsizei );
static void (*func_glCopyTexImage1DEXT)( GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLint );
static void (*func_glCopyTexImage2DEXT)( GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint );
static void (*func_glCopyTexSubImage1DEXT)( GLenum, GLint, GLint, GLint, GLint, GLsizei );
static void (*func_glCopyTexSubImage2DEXT)( GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei );
static void (*func_glCopyTexSubImage3DEXT)( GLenum, GLint, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei );
static GLuint (*func_glCreateProgram)( void );
static unsigned int (*func_glCreateProgramObjectARB)( void );
static GLuint (*func_glCreateShader)( GLenum );
static unsigned int (*func_glCreateShaderObjectARB)( GLenum );
static void (*func_glCullParameterdvEXT)( GLenum, GLdouble* );
static void (*func_glCullParameterfvEXT)( GLenum, GLfloat* );
static void (*func_glCurrentPaletteMatrixARB)( GLint );
static void (*func_glDeformSGIX)( GLint );
static void (*func_glDeformationMap3dSGIX)( GLint, GLdouble, GLdouble, GLint, GLint, GLdouble, GLdouble, GLint, GLint, GLdouble, GLdouble, GLint, GLint, GLdouble* );
static void (*func_glDeformationMap3fSGIX)( GLint, GLfloat, GLfloat, GLint, GLint, GLfloat, GLfloat, GLint, GLint, GLfloat, GLfloat, GLint, GLint, GLfloat* );
static void (*func_glDeleteAsyncMarkersSGIX)( GLuint, GLsizei );
static void (*func_glDeleteBufferRegion)( GLenum );
static void (*func_glDeleteBuffers)( GLsizei, GLuint* );
static void (*func_glDeleteBuffersARB)( GLsizei, GLuint* );
static void (*func_glDeleteFencesAPPLE)( GLsizei, GLuint* );
static void (*func_glDeleteFencesNV)( GLsizei, GLuint* );
static void (*func_glDeleteFragmentShaderATI)( GLuint );
static void (*func_glDeleteFramebuffersEXT)( GLsizei, GLuint* );
static void (*func_glDeleteObjectARB)( unsigned int );
static void (*func_glDeleteObjectBufferATI)( GLuint );
static void (*func_glDeleteOcclusionQueriesNV)( GLsizei, GLuint* );
static void (*func_glDeleteProgram)( GLuint );
static void (*func_glDeleteProgramsARB)( GLsizei, GLuint* );
static void (*func_glDeleteProgramsNV)( GLsizei, GLuint* );
static void (*func_glDeleteQueries)( GLsizei, GLuint* );
static void (*func_glDeleteQueriesARB)( GLsizei, GLuint* );
static void (*func_glDeleteRenderbuffersEXT)( GLsizei, GLuint* );
static void (*func_glDeleteShader)( GLuint );
static void (*func_glDeleteTexturesEXT)( GLsizei, GLuint* );
static void (*func_glDeleteVertexArraysAPPLE)( GLsizei, GLuint* );
static void (*func_glDeleteVertexShaderEXT)( GLuint );
static void (*func_glDepthBoundsEXT)( GLclampd, GLclampd );
static void (*func_glDetachObjectARB)( unsigned int, unsigned int );
static void (*func_glDetachShader)( GLuint, GLuint );
static void (*func_glDetailTexFuncSGIS)( GLenum, GLsizei, GLfloat* );
static void (*func_glDisableVariantClientStateEXT)( GLuint );
static void (*func_glDisableVertexAttribArray)( GLuint );
static void (*func_glDisableVertexAttribArrayARB)( GLuint );
static void (*func_glDrawArraysEXT)( GLenum, GLint, GLsizei );
static void (*func_glDrawBufferRegion)( GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLint );
static void (*func_glDrawBuffers)( GLsizei, GLenum* );
static void (*func_glDrawBuffersARB)( GLsizei, GLenum* );
static void (*func_glDrawBuffersATI)( GLsizei, GLenum* );
static void (*func_glDrawElementArrayAPPLE)( GLenum, GLint, GLsizei );
static void (*func_glDrawElementArrayATI)( GLenum, GLsizei );
static void (*func_glDrawMeshArraysSUN)( GLenum, GLint, GLsizei, GLsizei );
static void (*func_glDrawRangeElementArrayAPPLE)( GLenum, GLuint, GLuint, GLint, GLsizei );
static void (*func_glDrawRangeElementArrayATI)( GLenum, GLuint, GLuint, GLsizei );
static void (*func_glDrawRangeElementsEXT)( GLenum, GLuint, GLuint, GLsizei, GLenum, GLvoid* );
static void (*func_glEdgeFlagPointerEXT)( GLsizei, GLsizei, GLboolean* );
static void (*func_glEdgeFlagPointerListIBM)( GLint, GLboolean**, GLint );
static void (*func_glElementPointerAPPLE)( GLenum, GLvoid* );
static void (*func_glElementPointerATI)( GLenum, GLvoid* );
static void (*func_glEnableVariantClientStateEXT)( GLuint );
static void (*func_glEnableVertexAttribArray)( GLuint );
static void (*func_glEnableVertexAttribArrayARB)( GLuint );
static void (*func_glEndFragmentShaderATI)( void );
static void (*func_glEndOcclusionQueryNV)( void );
static void (*func_glEndQuery)( GLenum );
static void (*func_glEndQueryARB)( GLenum );
static void (*func_glEndVertexShaderEXT)( void );
static void (*func_glEvalMapsNV)( GLenum, GLenum );
static void (*func_glExecuteProgramNV)( GLenum, GLuint, GLfloat* );
static void (*func_glExtractComponentEXT)( GLuint, GLuint, GLuint );
static void (*func_glFinalCombinerInputNV)( GLenum, GLenum, GLenum, GLenum );
static GLint (*func_glFinishAsyncSGIX)( GLuint* );
static void (*func_glFinishFenceAPPLE)( GLuint );
static void (*func_glFinishFenceNV)( GLuint );
static void (*func_glFinishObjectAPPLE)( GLenum, GLint );
static void (*func_glFinishTextureSUNX)( void );
static void (*func_glFlushPixelDataRangeNV)( GLenum );
static void (*func_glFlushRasterSGIX)( void );
static void (*func_glFlushVertexArrayRangeAPPLE)( GLsizei, GLvoid* );
static void (*func_glFlushVertexArrayRangeNV)( void );
static void (*func_glFogCoordPointer)( GLenum, GLsizei, GLvoid* );
static void (*func_glFogCoordPointerEXT)( GLenum, GLsizei, GLvoid* );
static void (*func_glFogCoordPointerListIBM)( GLenum, GLint, GLvoid**, GLint );
static void (*func_glFogCoordd)( GLdouble );
static void (*func_glFogCoorddEXT)( GLdouble );
static void (*func_glFogCoorddv)( GLdouble* );
static void (*func_glFogCoorddvEXT)( GLdouble* );
static void (*func_glFogCoordf)( GLfloat );
static void (*func_glFogCoordfEXT)( GLfloat );
static void (*func_glFogCoordfv)( GLfloat* );
static void (*func_glFogCoordfvEXT)( GLfloat* );
static void (*func_glFogCoordhNV)( unsigned short );
static void (*func_glFogCoordhvNV)( unsigned short* );
static void (*func_glFogFuncSGIS)( GLsizei, GLfloat* );
static void (*func_glFragmentColorMaterialSGIX)( GLenum, GLenum );
static void (*func_glFragmentLightModelfSGIX)( GLenum, GLfloat );
static void (*func_glFragmentLightModelfvSGIX)( GLenum, GLfloat* );
static void (*func_glFragmentLightModeliSGIX)( GLenum, GLint );
static void (*func_glFragmentLightModelivSGIX)( GLenum, GLint* );
static void (*func_glFragmentLightfSGIX)( GLenum, GLenum, GLfloat );
static void (*func_glFragmentLightfvSGIX)( GLenum, GLenum, GLfloat* );
static void (*func_glFragmentLightiSGIX)( GLenum, GLenum, GLint );
static void (*func_glFragmentLightivSGIX)( GLenum, GLenum, GLint* );
static void (*func_glFragmentMaterialfSGIX)( GLenum, GLenum, GLfloat );
static void (*func_glFragmentMaterialfvSGIX)( GLenum, GLenum, GLfloat* );
static void (*func_glFragmentMaterialiSGIX)( GLenum, GLenum, GLint );
static void (*func_glFragmentMaterialivSGIX)( GLenum, GLenum, GLint* );
static void (*func_glFrameZoomSGIX)( GLint );
static void (*func_glFramebufferRenderbufferEXT)( GLenum, GLenum, GLenum, GLuint );
static void (*func_glFramebufferTexture1DEXT)( GLenum, GLenum, GLenum, GLuint, GLint );
static void (*func_glFramebufferTexture2DEXT)( GLenum, GLenum, GLenum, GLuint, GLint );
static void (*func_glFramebufferTexture3DEXT)( GLenum, GLenum, GLenum, GLuint, GLint, GLint );
static void (*func_glFreeObjectBufferATI)( GLuint );
static GLuint (*func_glGenAsyncMarkersSGIX)( GLsizei );
static void (*func_glGenBuffers)( GLsizei, GLuint* );
static void (*func_glGenBuffersARB)( GLsizei, GLuint* );
static void (*func_glGenFencesAPPLE)( GLsizei, GLuint* );
static void (*func_glGenFencesNV)( GLsizei, GLuint* );
static GLuint (*func_glGenFragmentShadersATI)( GLuint );
static void (*func_glGenFramebuffersEXT)( GLsizei, GLuint* );
static void (*func_glGenOcclusionQueriesNV)( GLsizei, GLuint* );
static void (*func_glGenProgramsARB)( GLsizei, GLuint* );
static void (*func_glGenProgramsNV)( GLsizei, GLuint* );
static void (*func_glGenQueries)( GLsizei, GLuint* );
static void (*func_glGenQueriesARB)( GLsizei, GLuint* );
static void (*func_glGenRenderbuffersEXT)( GLsizei, GLuint* );
static GLuint (*func_glGenSymbolsEXT)( GLenum, GLenum, GLenum, GLuint );
static void (*func_glGenTexturesEXT)( GLsizei, GLuint* );
static void (*func_glGenVertexArraysAPPLE)( GLsizei, GLuint* );
static GLuint (*func_glGenVertexShadersEXT)( GLuint );
static void (*func_glGenerateMipmapEXT)( GLenum );
static void (*func_glGetActiveAttrib)( GLuint, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, char* );
static void (*func_glGetActiveAttribARB)( unsigned int, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, char* );
static void (*func_glGetActiveUniform)( GLuint, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, char* );
static void (*func_glGetActiveUniformARB)( unsigned int, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, char* );
static void (*func_glGetArrayObjectfvATI)( GLenum, GLenum, GLfloat* );
static void (*func_glGetArrayObjectivATI)( GLenum, GLenum, GLint* );
static void (*func_glGetAttachedObjectsARB)( unsigned int, GLsizei, GLsizei*, unsigned int* );
static void (*func_glGetAttachedShaders)( GLuint, GLsizei, GLsizei*, GLuint* );
static GLint (*func_glGetAttribLocation)( GLuint, char* );
static GLint (*func_glGetAttribLocationARB)( unsigned int, char* );
static void (*func_glGetBufferParameteriv)( GLenum, GLenum, GLint* );
static void (*func_glGetBufferParameterivARB)( GLenum, GLenum, GLint* );
static void (*func_glGetBufferPointerv)( GLenum, GLenum, GLvoid** );
static void (*func_glGetBufferPointervARB)( GLenum, GLenum, GLvoid** );
static void (*func_glGetBufferSubData)( GLenum, ptrdiff_t, ptrdiff_t, GLvoid* );
static void (*func_glGetBufferSubDataARB)( GLenum, ptrdiff_t, ptrdiff_t, GLvoid* );
static void (*func_glGetColorTableEXT)( GLenum, GLenum, GLenum, GLvoid* );
static void (*func_glGetColorTableParameterfvEXT)( GLenum, GLenum, GLfloat* );
static void (*func_glGetColorTableParameterfvSGI)( GLenum, GLenum, GLfloat* );
static void (*func_glGetColorTableParameterivEXT)( GLenum, GLenum, GLint* );
static void (*func_glGetColorTableParameterivSGI)( GLenum, GLenum, GLint* );
static void (*func_glGetColorTableSGI)( GLenum, GLenum, GLenum, GLvoid* );
static void (*func_glGetCombinerInputParameterfvNV)( GLenum, GLenum, GLenum, GLenum, GLfloat* );
static void (*func_glGetCombinerInputParameterivNV)( GLenum, GLenum, GLenum, GLenum, GLint* );
static void (*func_glGetCombinerOutputParameterfvNV)( GLenum, GLenum, GLenum, GLfloat* );
static void (*func_glGetCombinerOutputParameterivNV)( GLenum, GLenum, GLenum, GLint* );
static void (*func_glGetCombinerStageParameterfvNV)( GLenum, GLenum, GLfloat* );
static void (*func_glGetCompressedTexImage)( GLenum, GLint, GLvoid* );
static void (*func_glGetCompressedTexImageARB)( GLenum, GLint, GLvoid* );
static void (*func_glGetConvolutionFilterEXT)( GLenum, GLenum, GLenum, GLvoid* );
static void (*func_glGetConvolutionParameterfvEXT)( GLenum, GLenum, GLfloat* );
static void (*func_glGetConvolutionParameterivEXT)( GLenum, GLenum, GLint* );
static void (*func_glGetDetailTexFuncSGIS)( GLenum, GLfloat* );
static void (*func_glGetFenceivNV)( GLuint, GLenum, GLint* );
static void (*func_glGetFinalCombinerInputParameterfvNV)( GLenum, GLenum, GLfloat* );
static void (*func_glGetFinalCombinerInputParameterivNV)( GLenum, GLenum, GLint* );
static void (*func_glGetFogFuncSGIS)( GLfloat* );
static void (*func_glGetFragmentLightfvSGIX)( GLenum, GLenum, GLfloat* );
static void (*func_glGetFragmentLightivSGIX)( GLenum, GLenum, GLint* );
static void (*func_glGetFragmentMaterialfvSGIX)( GLenum, GLenum, GLfloat* );
static void (*func_glGetFragmentMaterialivSGIX)( GLenum, GLenum, GLint* );
static void (*func_glGetFramebufferAttachmentParameterivEXT)( GLenum, GLenum, GLenum, GLint* );
static unsigned int (*func_glGetHandleARB)( GLenum );
static void (*func_glGetHistogramEXT)( GLenum, GLboolean, GLenum, GLenum, GLvoid* );
static void (*func_glGetHistogramParameterfvEXT)( GLenum, GLenum, GLfloat* );
static void (*func_glGetHistogramParameterivEXT)( GLenum, GLenum, GLint* );
static void (*func_glGetImageTransformParameterfvHP)( GLenum, GLenum, GLfloat* );
static void (*func_glGetImageTransformParameterivHP)( GLenum, GLenum, GLint* );
static void (*func_glGetInfoLogARB)( unsigned int, GLsizei, GLsizei*, char* );
static GLint (*func_glGetInstrumentsSGIX)( void );
static void (*func_glGetInvariantBooleanvEXT)( GLuint, GLenum, GLboolean* );
static void (*func_glGetInvariantFloatvEXT)( GLuint, GLenum, GLfloat* );
static void (*func_glGetInvariantIntegervEXT)( GLuint, GLenum, GLint* );
static void (*func_glGetListParameterfvSGIX)( GLuint, GLenum, GLfloat* );
static void (*func_glGetListParameterivSGIX)( GLuint, GLenum, GLint* );
static void (*func_glGetLocalConstantBooleanvEXT)( GLuint, GLenum, GLboolean* );
static void (*func_glGetLocalConstantFloatvEXT)( GLuint, GLenum, GLfloat* );
static void (*func_glGetLocalConstantIntegervEXT)( GLuint, GLenum, GLint* );
static void (*func_glGetMapAttribParameterfvNV)( GLenum, GLuint, GLenum, GLfloat* );
static void (*func_glGetMapAttribParameterivNV)( GLenum, GLuint, GLenum, GLint* );
static void (*func_glGetMapControlPointsNV)( GLenum, GLuint, GLenum, GLsizei, GLsizei, GLboolean, GLvoid* );
static void (*func_glGetMapParameterfvNV)( GLenum, GLenum, GLfloat* );
static void (*func_glGetMapParameterivNV)( GLenum, GLenum, GLint* );
static void (*func_glGetMinmaxEXT)( GLenum, GLboolean, GLenum, GLenum, GLvoid* );
static void (*func_glGetMinmaxParameterfvEXT)( GLenum, GLenum, GLfloat* );
static void (*func_glGetMinmaxParameterivEXT)( GLenum, GLenum, GLint* );
static void (*func_glGetObjectBufferfvATI)( GLuint, GLenum, GLfloat* );
static void (*func_glGetObjectBufferivATI)( GLuint, GLenum, GLint* );
static void (*func_glGetObjectParameterfvARB)( unsigned int, GLenum, GLfloat* );
static void (*func_glGetObjectParameterivARB)( unsigned int, GLenum, GLint* );
static void (*func_glGetOcclusionQueryivNV)( GLuint, GLenum, GLint* );
static void (*func_glGetOcclusionQueryuivNV)( GLuint, GLenum, GLuint* );
static void (*func_glGetPixelTexGenParameterfvSGIS)( GLenum, GLfloat* );
static void (*func_glGetPixelTexGenParameterivSGIS)( GLenum, GLint* );
static void (*func_glGetPointervEXT)( GLenum, GLvoid** );
static void (*func_glGetProgramEnvParameterdvARB)( GLenum, GLuint, GLdouble* );
static void (*func_glGetProgramEnvParameterfvARB)( GLenum, GLuint, GLfloat* );
static void (*func_glGetProgramInfoLog)( GLuint, GLsizei, GLsizei*, char* );
static void (*func_glGetProgramLocalParameterdvARB)( GLenum, GLuint, GLdouble* );
static void (*func_glGetProgramLocalParameterfvARB)( GLenum, GLuint, GLfloat* );
static void (*func_glGetProgramNamedParameterdvNV)( GLuint, GLsizei, GLubyte*, GLdouble* );
static void (*func_glGetProgramNamedParameterfvNV)( GLuint, GLsizei, GLubyte*, GLfloat* );
static void (*func_glGetProgramParameterdvNV)( GLenum, GLuint, GLenum, GLdouble* );
static void (*func_glGetProgramParameterfvNV)( GLenum, GLuint, GLenum, GLfloat* );
static void (*func_glGetProgramStringARB)( GLenum, GLenum, GLvoid* );
static void (*func_glGetProgramStringNV)( GLuint, GLenum, GLubyte* );
static void (*func_glGetProgramiv)( GLuint, GLenum, GLint* );
static void (*func_glGetProgramivARB)( GLenum, GLenum, GLint* );
static void (*func_glGetProgramivNV)( GLuint, GLenum, GLint* );
static void (*func_glGetQueryObjectiv)( GLuint, GLenum, GLint* );
static void (*func_glGetQueryObjectivARB)( GLuint, GLenum, GLint* );
static void (*func_glGetQueryObjectuiv)( GLuint, GLenum, GLuint* );
static void (*func_glGetQueryObjectuivARB)( GLuint, GLenum, GLuint* );
static void (*func_glGetQueryiv)( GLenum, GLenum, GLint* );
static void (*func_glGetQueryivARB)( GLenum, GLenum, GLint* );
static void (*func_glGetRenderbufferParameterivEXT)( GLenum, GLenum, GLint* );
static void (*func_glGetSeparableFilterEXT)( GLenum, GLenum, GLenum, GLvoid*, GLvoid*, GLvoid* );
static void (*func_glGetShaderInfoLog)( GLuint, GLsizei, GLsizei*, char* );
static void (*func_glGetShaderSource)( GLuint, GLsizei, GLsizei*, char* );
static void (*func_glGetShaderSourceARB)( unsigned int, GLsizei, GLsizei*, char* );
static void (*func_glGetShaderiv)( GLuint, GLenum, GLint* );
static void (*func_glGetSharpenTexFuncSGIS)( GLenum, GLfloat* );
static void (*func_glGetTexBumpParameterfvATI)( GLenum, GLfloat* );
static void (*func_glGetTexBumpParameterivATI)( GLenum, GLint* );
static void (*func_glGetTexFilterFuncSGIS)( GLenum, GLenum, GLfloat* );
static void (*func_glGetTrackMatrixivNV)( GLenum, GLuint, GLenum, GLint* );
static GLint (*func_glGetUniformLocation)( GLuint, char* );
static GLint (*func_glGetUniformLocationARB)( unsigned int, char* );
static void (*func_glGetUniformfv)( GLuint, GLint, GLfloat* );
static void (*func_glGetUniformfvARB)( unsigned int, GLint, GLfloat* );
static void (*func_glGetUniformiv)( GLuint, GLint, GLint* );
static void (*func_glGetUniformivARB)( unsigned int, GLint, GLint* );
static void (*func_glGetVariantArrayObjectfvATI)( GLuint, GLenum, GLfloat* );
static void (*func_glGetVariantArrayObjectivATI)( GLuint, GLenum, GLint* );
static void (*func_glGetVariantBooleanvEXT)( GLuint, GLenum, GLboolean* );
static void (*func_glGetVariantFloatvEXT)( GLuint, GLenum, GLfloat* );
static void (*func_glGetVariantIntegervEXT)( GLuint, GLenum, GLint* );
static void (*func_glGetVariantPointervEXT)( GLuint, GLenum, GLvoid** );
static void (*func_glGetVertexAttribArrayObjectfvATI)( GLuint, GLenum, GLfloat* );
static void (*func_glGetVertexAttribArrayObjectivATI)( GLuint, GLenum, GLint* );
static void (*func_glGetVertexAttribPointerv)( GLuint, GLenum, GLvoid** );
static void (*func_glGetVertexAttribPointervARB)( GLuint, GLenum, GLvoid** );
static void (*func_glGetVertexAttribPointervNV)( GLuint, GLenum, GLvoid** );
static void (*func_glGetVertexAttribdv)( GLuint, GLenum, GLdouble* );
static void (*func_glGetVertexAttribdvARB)( GLuint, GLenum, GLdouble* );
static void (*func_glGetVertexAttribdvNV)( GLuint, GLenum, GLdouble* );
static void (*func_glGetVertexAttribfv)( GLuint, GLenum, GLfloat* );
static void (*func_glGetVertexAttribfvARB)( GLuint, GLenum, GLfloat* );
static void (*func_glGetVertexAttribfvNV)( GLuint, GLenum, GLfloat* );
static void (*func_glGetVertexAttribiv)( GLuint, GLenum, GLint* );
static void (*func_glGetVertexAttribivARB)( GLuint, GLenum, GLint* );
static void (*func_glGetVertexAttribivNV)( GLuint, GLenum, GLint* );
static void (*func_glGlobalAlphaFactorbSUN)( GLbyte );
static void (*func_glGlobalAlphaFactordSUN)( GLdouble );
static void (*func_glGlobalAlphaFactorfSUN)( GLfloat );
static void (*func_glGlobalAlphaFactoriSUN)( GLint );
static void (*func_glGlobalAlphaFactorsSUN)( GLshort );
static void (*func_glGlobalAlphaFactorubSUN)( GLubyte );
static void (*func_glGlobalAlphaFactoruiSUN)( GLuint );
static void (*func_glGlobalAlphaFactorusSUN)( GLushort );
static void (*func_glHintPGI)( GLenum, GLint );
static void (*func_glHistogramEXT)( GLenum, GLsizei, GLenum, GLboolean );
static void (*func_glIglooInterfaceSGIX)( GLint, GLint* );
static void (*func_glImageTransformParameterfHP)( GLenum, GLenum, GLfloat );
static void (*func_glImageTransformParameterfvHP)( GLenum, GLenum, GLfloat* );
static void (*func_glImageTransformParameteriHP)( GLenum, GLenum, GLint );
static void (*func_glImageTransformParameterivHP)( GLenum, GLenum, GLint* );
static void (*func_glIndexFuncEXT)( GLenum, GLclampf );
static void (*func_glIndexMaterialEXT)( GLenum, GLenum );
static void (*func_glIndexPointerEXT)( GLenum, GLsizei, GLsizei, GLvoid* );
static void (*func_glIndexPointerListIBM)( GLenum, GLint, GLvoid**, GLint );
static void (*func_glInsertComponentEXT)( GLuint, GLuint, GLuint );
static void (*func_glInstrumentsBufferSGIX)( GLsizei, GLint* );
static GLboolean (*func_glIsAsyncMarkerSGIX)( GLuint );
static GLboolean (*func_glIsBuffer)( GLuint );
static GLboolean (*func_glIsBufferARB)( GLuint );
static GLboolean (*func_glIsFenceAPPLE)( GLuint );
static GLboolean (*func_glIsFenceNV)( GLuint );
static GLboolean (*func_glIsFramebufferEXT)( GLuint );
static GLboolean (*func_glIsObjectBufferATI)( GLuint );
static GLboolean (*func_glIsOcclusionQueryNV)( GLuint );
static GLboolean (*func_glIsProgram)( GLuint );
static GLboolean (*func_glIsProgramARB)( GLuint );
static GLboolean (*func_glIsProgramNV)( GLuint );
static GLboolean (*func_glIsQuery)( GLuint );
static GLboolean (*func_glIsQueryARB)( GLuint );
static GLboolean (*func_glIsRenderbufferEXT)( GLuint );
static GLboolean (*func_glIsShader)( GLuint );
static GLboolean (*func_glIsTextureEXT)( GLuint );
static GLboolean (*func_glIsVariantEnabledEXT)( GLuint, GLenum );
static GLboolean (*func_glIsVertexArrayAPPLE)( GLuint );
static void (*func_glLightEnviSGIX)( GLenum, GLint );
static void (*func_glLinkProgram)( GLuint );
static void (*func_glLinkProgramARB)( unsigned int );
static void (*func_glListParameterfSGIX)( GLuint, GLenum, GLfloat );
static void (*func_glListParameterfvSGIX)( GLuint, GLenum, GLfloat* );
static void (*func_glListParameteriSGIX)( GLuint, GLenum, GLint );
static void (*func_glListParameterivSGIX)( GLuint, GLenum, GLint* );
static void (*func_glLoadIdentityDeformationMapSGIX)( GLint );
static void (*func_glLoadProgramNV)( GLenum, GLuint, GLsizei, GLubyte* );
static void (*func_glLoadTransposeMatrixd)( GLdouble* );
static void (*func_glLoadTransposeMatrixdARB)( GLdouble* );
static void (*func_glLoadTransposeMatrixf)( GLfloat* );
static void (*func_glLoadTransposeMatrixfARB)( GLfloat* );
static void (*func_glLockArraysEXT)( GLint, GLsizei );
static void (*func_glMTexCoord2fSGIS)( GLenum, GLfloat, GLfloat );
static void (*func_glMTexCoord2fvSGIS)( GLenum, GLfloat * );
static GLvoid* (*func_glMapBuffer)( GLenum, GLenum );
static GLvoid* (*func_glMapBufferARB)( GLenum, GLenum );
static void (*func_glMapControlPointsNV)( GLenum, GLuint, GLenum, GLsizei, GLsizei, GLint, GLint, GLboolean, GLvoid* );
static GLvoid* (*func_glMapObjectBufferATI)( GLuint );
static void (*func_glMapParameterfvNV)( GLenum, GLenum, GLfloat* );
static void (*func_glMapParameterivNV)( GLenum, GLenum, GLint* );
static void (*func_glMatrixIndexPointerARB)( GLint, GLenum, GLsizei, GLvoid* );
static void (*func_glMatrixIndexubvARB)( GLint, GLubyte* );
static void (*func_glMatrixIndexuivARB)( GLint, GLuint* );
static void (*func_glMatrixIndexusvARB)( GLint, GLushort* );
static void (*func_glMinmaxEXT)( GLenum, GLenum, GLboolean );
static void (*func_glMultTransposeMatrixd)( GLdouble* );
static void (*func_glMultTransposeMatrixdARB)( GLdouble* );
static void (*func_glMultTransposeMatrixf)( GLfloat* );
static void (*func_glMultTransposeMatrixfARB)( GLfloat* );
static void (*func_glMultiDrawArrays)( GLenum, GLint*, GLsizei*, GLsizei );
static void (*func_glMultiDrawArraysEXT)( GLenum, GLint*, GLsizei*, GLsizei );
static void (*func_glMultiDrawElementArrayAPPLE)( GLenum, GLint*, GLsizei*, GLsizei );
static void (*func_glMultiDrawElements)( GLenum, GLsizei*, GLenum, GLvoid**, GLsizei );
static void (*func_glMultiDrawElementsEXT)( GLenum, GLsizei*, GLenum, GLvoid**, GLsizei );
static void (*func_glMultiDrawRangeElementArrayAPPLE)( GLenum, GLuint, GLuint, GLint*, GLsizei*, GLsizei );
static void (*func_glMultiModeDrawArraysIBM)( GLenum*, GLint*, GLsizei*, GLsizei, GLint );
static void (*func_glMultiModeDrawElementsIBM)( GLenum*, GLsizei*, GLenum, GLvoid* const*, GLsizei, GLint );
static void (*func_glMultiTexCoord1d)( GLenum, GLdouble );
static void (*func_glMultiTexCoord1dARB)( GLenum, GLdouble );
static void (*func_glMultiTexCoord1dSGIS)( GLenum, GLdouble );
static void (*func_glMultiTexCoord1dv)( GLenum, GLdouble* );
static void (*func_glMultiTexCoord1dvARB)( GLenum, GLdouble* );
static void (*func_glMultiTexCoord1dvSGIS)( GLenum, GLdouble * );
static void (*func_glMultiTexCoord1f)( GLenum, GLfloat );
static void (*func_glMultiTexCoord1fARB)( GLenum, GLfloat );
static void (*func_glMultiTexCoord1fSGIS)( GLenum, GLfloat );
static void (*func_glMultiTexCoord1fv)( GLenum, GLfloat* );
static void (*func_glMultiTexCoord1fvARB)( GLenum, GLfloat* );
static void (*func_glMultiTexCoord1fvSGIS)( GLenum, const GLfloat * );
static void (*func_glMultiTexCoord1hNV)( GLenum, unsigned short );
static void (*func_glMultiTexCoord1hvNV)( GLenum, unsigned short* );
static void (*func_glMultiTexCoord1i)( GLenum, GLint );
static void (*func_glMultiTexCoord1iARB)( GLenum, GLint );
static void (*func_glMultiTexCoord1iSGIS)( GLenum, GLint );
static void (*func_glMultiTexCoord1iv)( GLenum, GLint* );
static void (*func_glMultiTexCoord1ivARB)( GLenum, GLint* );
static void (*func_glMultiTexCoord1ivSGIS)( GLenum, GLint * );
static void (*func_glMultiTexCoord1s)( GLenum, GLshort );
static void (*func_glMultiTexCoord1sARB)( GLenum, GLshort );
static void (*func_glMultiTexCoord1sSGIS)( GLenum, GLshort );
static void (*func_glMultiTexCoord1sv)( GLenum, GLshort* );
static void (*func_glMultiTexCoord1svARB)( GLenum, GLshort* );
static void (*func_glMultiTexCoord1svSGIS)( GLenum, GLshort * );
static void (*func_glMultiTexCoord2d)( GLenum, GLdouble, GLdouble );
static void (*func_glMultiTexCoord2dARB)( GLenum, GLdouble, GLdouble );
static void (*func_glMultiTexCoord2dSGIS)( GLenum, GLdouble, GLdouble );
static void (*func_glMultiTexCoord2dv)( GLenum, GLdouble* );
static void (*func_glMultiTexCoord2dvARB)( GLenum, GLdouble* );
static void (*func_glMultiTexCoord2dvSGIS)( GLenum, GLdouble * );
static void (*func_glMultiTexCoord2f)( GLenum, GLfloat, GLfloat );
static void (*func_glMultiTexCoord2fARB)( GLenum, GLfloat, GLfloat );
static void (*func_glMultiTexCoord2fSGIS)( GLenum, GLfloat, GLfloat );
static void (*func_glMultiTexCoord2fv)( GLenum, GLfloat* );
static void (*func_glMultiTexCoord2fvARB)( GLenum, GLfloat* );
static void (*func_glMultiTexCoord2fvSGIS)( GLenum, GLfloat * );
static void (*func_glMultiTexCoord2hNV)( GLenum, unsigned short, unsigned short );
static void (*func_glMultiTexCoord2hvNV)( GLenum, unsigned short* );
static void (*func_glMultiTexCoord2i)( GLenum, GLint, GLint );
static void (*func_glMultiTexCoord2iARB)( GLenum, GLint, GLint );
static void (*func_glMultiTexCoord2iSGIS)( GLenum, GLint, GLint );
static void (*func_glMultiTexCoord2iv)( GLenum, GLint* );
static void (*func_glMultiTexCoord2ivARB)( GLenum, GLint* );
static void (*func_glMultiTexCoord2ivSGIS)( GLenum, GLint * );
static void (*func_glMultiTexCoord2s)( GLenum, GLshort, GLshort );
static void (*func_glMultiTexCoord2sARB)( GLenum, GLshort, GLshort );
static void (*func_glMultiTexCoord2sSGIS)( GLenum, GLshort, GLshort );
static void (*func_glMultiTexCoord2sv)( GLenum, GLshort* );
static void (*func_glMultiTexCoord2svARB)( GLenum, GLshort* );
static void (*func_glMultiTexCoord2svSGIS)( GLenum, GLshort * );
static void (*func_glMultiTexCoord3d)( GLenum, GLdouble, GLdouble, GLdouble );
static void (*func_glMultiTexCoord3dARB)( GLenum, GLdouble, GLdouble, GLdouble );
static void (*func_glMultiTexCoord3dSGIS)( GLenum, GLdouble, GLdouble, GLdouble );
static void (*func_glMultiTexCoord3dv)( GLenum, GLdouble* );
static void (*func_glMultiTexCoord3dvARB)( GLenum, GLdouble* );
static void (*func_glMultiTexCoord3dvSGIS)( GLenum, GLdouble * );
static void (*func_glMultiTexCoord3f)( GLenum, GLfloat, GLfloat, GLfloat );
static void (*func_glMultiTexCoord3fARB)( GLenum, GLfloat, GLfloat, GLfloat );
static void (*func_glMultiTexCoord3fSGIS)( GLenum, GLfloat, GLfloat, GLfloat );
static void (*func_glMultiTexCoord3fv)( GLenum, GLfloat* );
static void (*func_glMultiTexCoord3fvARB)( GLenum, GLfloat* );
static void (*func_glMultiTexCoord3fvSGIS)( GLenum, GLfloat * );
static void (*func_glMultiTexCoord3hNV)( GLenum, unsigned short, unsigned short, unsigned short );
static void (*func_glMultiTexCoord3hvNV)( GLenum, unsigned short* );
static void (*func_glMultiTexCoord3i)( GLenum, GLint, GLint, GLint );
static void (*func_glMultiTexCoord3iARB)( GLenum, GLint, GLint, GLint );
static void (*func_glMultiTexCoord3iSGIS)( GLenum, GLint, GLint, GLint );
static void (*func_glMultiTexCoord3iv)( GLenum, GLint* );
static void (*func_glMultiTexCoord3ivARB)( GLenum, GLint* );
static void (*func_glMultiTexCoord3ivSGIS)( GLenum, GLint * );
static void (*func_glMultiTexCoord3s)( GLenum, GLshort, GLshort, GLshort );
static void (*func_glMultiTexCoord3sARB)( GLenum, GLshort, GLshort, GLshort );
static void (*func_glMultiTexCoord3sSGIS)( GLenum, GLshort, GLshort, GLshort );
static void (*func_glMultiTexCoord3sv)( GLenum, GLshort* );
static void (*func_glMultiTexCoord3svARB)( GLenum, GLshort* );
static void (*func_glMultiTexCoord3svSGIS)( GLenum, GLshort * );
static void (*func_glMultiTexCoord4d)( GLenum, GLdouble, GLdouble, GLdouble, GLdouble );
static void (*func_glMultiTexCoord4dARB)( GLenum, GLdouble, GLdouble, GLdouble, GLdouble );
static void (*func_glMultiTexCoord4dSGIS)( GLenum, GLdouble, GLdouble, GLdouble, GLdouble );
static void (*func_glMultiTexCoord4dv)( GLenum, GLdouble* );
static void (*func_glMultiTexCoord4dvARB)( GLenum, GLdouble* );
static void (*func_glMultiTexCoord4dvSGIS)( GLenum, GLdouble * );
static void (*func_glMultiTexCoord4f)( GLenum, GLfloat, GLfloat, GLfloat, GLfloat );
static void (*func_glMultiTexCoord4fARB)( GLenum, GLfloat, GLfloat, GLfloat, GLfloat );
static void (*func_glMultiTexCoord4fSGIS)( GLenum, GLfloat, GLfloat, GLfloat, GLfloat );
static void (*func_glMultiTexCoord4fv)( GLenum, GLfloat* );
static void (*func_glMultiTexCoord4fvARB)( GLenum, GLfloat* );
static void (*func_glMultiTexCoord4fvSGIS)( GLenum, GLfloat * );
static void (*func_glMultiTexCoord4hNV)( GLenum, unsigned short, unsigned short, unsigned short, unsigned short );
static void (*func_glMultiTexCoord4hvNV)( GLenum, unsigned short* );
static void (*func_glMultiTexCoord4i)( GLenum, GLint, GLint, GLint, GLint );
static void (*func_glMultiTexCoord4iARB)( GLenum, GLint, GLint, GLint, GLint );
static void (*func_glMultiTexCoord4iSGIS)( GLenum, GLint, GLint, GLint, GLint );
static void (*func_glMultiTexCoord4iv)( GLenum, GLint* );
static void (*func_glMultiTexCoord4ivARB)( GLenum, GLint* );
static void (*func_glMultiTexCoord4ivSGIS)( GLenum, GLint * );
static void (*func_glMultiTexCoord4s)( GLenum, GLshort, GLshort, GLshort, GLshort );
static void (*func_glMultiTexCoord4sARB)( GLenum, GLshort, GLshort, GLshort, GLshort );
static void (*func_glMultiTexCoord4sSGIS)( GLenum, GLshort, GLshort, GLshort, GLshort );
static void (*func_glMultiTexCoord4sv)( GLenum, GLshort* );
static void (*func_glMultiTexCoord4svARB)( GLenum, GLshort* );
static void (*func_glMultiTexCoord4svSGIS)( GLenum, GLshort * );
static void (*func_glMultiTexCoordPointerSGIS)( GLenum, GLint, GLenum, GLsizei, GLvoid * );
static GLuint (*func_glNewBufferRegion)( GLenum );
static GLuint (*func_glNewObjectBufferATI)( GLsizei, GLvoid*, GLenum );
static void (*func_glNormal3fVertex3fSUN)( GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat );
static void (*func_glNormal3fVertex3fvSUN)( GLfloat*, GLfloat* );
static void (*func_glNormal3hNV)( unsigned short, unsigned short, unsigned short );
static void (*func_glNormal3hvNV)( unsigned short* );
static void (*func_glNormalPointerEXT)( GLenum, GLsizei, GLsizei, GLvoid* );
static void (*func_glNormalPointerListIBM)( GLenum, GLint, GLvoid**, GLint );
static void (*func_glNormalPointervINTEL)( GLenum, GLvoid** );
static void (*func_glNormalStream3bATI)( GLenum, GLbyte, GLbyte, GLbyte );
static void (*func_glNormalStream3bvATI)( GLenum, GLbyte* );
static void (*func_glNormalStream3dATI)( GLenum, GLdouble, GLdouble, GLdouble );
static void (*func_glNormalStream3dvATI)( GLenum, GLdouble* );
static void (*func_glNormalStream3fATI)( GLenum, GLfloat, GLfloat, GLfloat );
static void (*func_glNormalStream3fvATI)( GLenum, GLfloat* );
static void (*func_glNormalStream3iATI)( GLenum, GLint, GLint, GLint );
static void (*func_glNormalStream3ivATI)( GLenum, GLint* );
static void (*func_glNormalStream3sATI)( GLenum, GLshort, GLshort, GLshort );
static void (*func_glNormalStream3svATI)( GLenum, GLshort* );
static void (*func_glPNTrianglesfATI)( GLenum, GLfloat );
static void (*func_glPNTrianglesiATI)( GLenum, GLint );
static void (*func_glPassTexCoordATI)( GLuint, GLuint, GLenum );
static void (*func_glPixelDataRangeNV)( GLenum, GLsizei, GLvoid* );
static void (*func_glPixelTexGenParameterfSGIS)( GLenum, GLfloat );
static void (*func_glPixelTexGenParameterfvSGIS)( GLenum, GLfloat* );
static void (*func_glPixelTexGenParameteriSGIS)( GLenum, GLint );
static void (*func_glPixelTexGenParameterivSGIS)( GLenum, GLint* );
static void (*func_glPixelTexGenSGIX)( GLenum );
static void (*func_glPixelTransformParameterfEXT)( GLenum, GLenum, GLfloat );
static void (*func_glPixelTransformParameterfvEXT)( GLenum, GLenum, GLfloat* );
static void (*func_glPixelTransformParameteriEXT)( GLenum, GLenum, GLint );
static void (*func_glPixelTransformParameterivEXT)( GLenum, GLenum, GLint* );
static void (*func_glPointParameterf)( GLenum, GLfloat );
static void (*func_glPointParameterfARB)( GLenum, GLfloat );
static void (*func_glPointParameterfEXT)( GLenum, GLfloat );
static void (*func_glPointParameterfSGIS)( GLenum, GLfloat );
static void (*func_glPointParameterfv)( GLenum, GLfloat* );
static void (*func_glPointParameterfvARB)( GLenum, GLfloat* );
static void (*func_glPointParameterfvEXT)( GLenum, GLfloat* );
static void (*func_glPointParameterfvSGIS)( GLenum, GLfloat* );
static void (*func_glPointParameteri)( GLenum, GLint );
static void (*func_glPointParameteriNV)( GLenum, GLint );
static void (*func_glPointParameteriv)( GLenum, GLint* );
static void (*func_glPointParameterivNV)( GLenum, GLint* );
static GLint (*func_glPollAsyncSGIX)( GLuint* );
static GLint (*func_glPollInstrumentsSGIX)( GLint* );
static void (*func_glPolygonOffsetEXT)( GLfloat, GLfloat );
static void (*func_glPrimitiveRestartIndexNV)( GLuint );
static void (*func_glPrimitiveRestartNV)( void );
static void (*func_glPrioritizeTexturesEXT)( GLsizei, GLuint*, GLclampf* );
static void (*func_glProgramEnvParameter4dARB)( GLenum, GLuint, GLdouble, GLdouble, GLdouble, GLdouble );
static void (*func_glProgramEnvParameter4dvARB)( GLenum, GLuint, GLdouble* );
static void (*func_glProgramEnvParameter4fARB)( GLenum, GLuint, GLfloat, GLfloat, GLfloat, GLfloat );
static void (*func_glProgramEnvParameter4fvARB)( GLenum, GLuint, GLfloat* );
static void (*func_glProgramLocalParameter4dARB)( GLenum, GLuint, GLdouble, GLdouble, GLdouble, GLdouble );
static void (*func_glProgramLocalParameter4dvARB)( GLenum, GLuint, GLdouble* );
static void (*func_glProgramLocalParameter4fARB)( GLenum, GLuint, GLfloat, GLfloat, GLfloat, GLfloat );
static void (*func_glProgramLocalParameter4fvARB)( GLenum, GLuint, GLfloat* );
static void (*func_glProgramNamedParameter4dNV)( GLuint, GLsizei, GLubyte*, GLdouble, GLdouble, GLdouble, GLdouble );
static void (*func_glProgramNamedParameter4dvNV)( GLuint, GLsizei, GLubyte*, GLdouble* );
static void (*func_glProgramNamedParameter4fNV)( GLuint, GLsizei, GLubyte*, GLfloat, GLfloat, GLfloat, GLfloat );
static void (*func_glProgramNamedParameter4fvNV)( GLuint, GLsizei, GLubyte*, GLfloat* );
static void (*func_glProgramParameter4dNV)( GLenum, GLuint, GLdouble, GLdouble, GLdouble, GLdouble );
static void (*func_glProgramParameter4dvNV)( GLenum, GLuint, GLdouble* );
static void (*func_glProgramParameter4fNV)( GLenum, GLuint, GLfloat, GLfloat, GLfloat, GLfloat );
static void (*func_glProgramParameter4fvNV)( GLenum, GLuint, GLfloat* );
static void (*func_glProgramParameters4dvNV)( GLenum, GLuint, GLuint, GLdouble* );
static void (*func_glProgramParameters4fvNV)( GLenum, GLuint, GLuint, GLfloat* );
static void (*func_glProgramStringARB)( GLenum, GLenum, GLsizei, GLvoid* );
static void (*func_glReadBufferRegion)( GLenum, GLint, GLint, GLsizei, GLsizei );
static void (*func_glReadInstrumentsSGIX)( GLint );
static void (*func_glReferencePlaneSGIX)( GLdouble* );
static void (*func_glRenderbufferStorageEXT)( GLenum, GLenum, GLsizei, GLsizei );
static void (*func_glReplacementCodePointerSUN)( GLenum, GLsizei, GLvoid** );
static void (*func_glReplacementCodeubSUN)( GLubyte );
static void (*func_glReplacementCodeubvSUN)( GLubyte* );
static void (*func_glReplacementCodeuiColor3fVertex3fSUN)( GLuint, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat );
static void (*func_glReplacementCodeuiColor3fVertex3fvSUN)( GLuint*, GLfloat*, GLfloat* );
static void (*func_glReplacementCodeuiColor4fNormal3fVertex3fSUN)( GLuint, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat );
static void (*func_glReplacementCodeuiColor4fNormal3fVertex3fvSUN)( GLuint*, GLfloat*, GLfloat*, GLfloat* );
static void (*func_glReplacementCodeuiColor4ubVertex3fSUN)( GLuint, GLubyte, GLubyte, GLubyte, GLubyte, GLfloat, GLfloat, GLfloat );
static void (*func_glReplacementCodeuiColor4ubVertex3fvSUN)( GLuint*, GLubyte*, GLfloat* );
static void (*func_glReplacementCodeuiNormal3fVertex3fSUN)( GLuint, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat );
static void (*func_glReplacementCodeuiNormal3fVertex3fvSUN)( GLuint*, GLfloat*, GLfloat* );
static void (*func_glReplacementCodeuiSUN)( GLuint );
static void (*func_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN)( GLuint, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat );
static void (*func_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN)( GLuint*, GLfloat*, GLfloat*, GLfloat*, GLfloat* );
static void (*func_glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN)( GLuint, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat );
static void (*func_glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN)( GLuint*, GLfloat*, GLfloat*, GLfloat* );
static void (*func_glReplacementCodeuiTexCoord2fVertex3fSUN)( GLuint, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat );
static void (*func_glReplacementCodeuiTexCoord2fVertex3fvSUN)( GLuint*, GLfloat*, GLfloat* );
static void (*func_glReplacementCodeuiVertex3fSUN)( GLuint, GLfloat, GLfloat, GLfloat );
static void (*func_glReplacementCodeuiVertex3fvSUN)( GLuint*, GLfloat* );
static void (*func_glReplacementCodeuivSUN)( GLuint* );
static void (*func_glReplacementCodeusSUN)( GLushort );
static void (*func_glReplacementCodeusvSUN)( GLushort* );
static void (*func_glRequestResidentProgramsNV)( GLsizei, GLuint* );
static void (*func_glResetHistogramEXT)( GLenum );
static void (*func_glResetMinmaxEXT)( GLenum );
static void (*func_glResizeBuffersMESA)( void );
static void (*func_glSampleCoverage)( GLclampf, GLboolean );
static void (*func_glSampleCoverageARB)( GLclampf, GLboolean );
static void (*func_glSampleMapATI)( GLuint, GLuint, GLenum );
static void (*func_glSampleMaskEXT)( GLclampf, GLboolean );
static void (*func_glSampleMaskSGIS)( GLclampf, GLboolean );
static void (*func_glSamplePatternEXT)( GLenum );
static void (*func_glSamplePatternSGIS)( GLenum );
static void (*func_glSecondaryColor3b)( GLbyte, GLbyte, GLbyte );
static void (*func_glSecondaryColor3bEXT)( GLbyte, GLbyte, GLbyte );
static void (*func_glSecondaryColor3bv)( GLbyte* );
static void (*func_glSecondaryColor3bvEXT)( GLbyte* );
static void (*func_glSecondaryColor3d)( GLdouble, GLdouble, GLdouble );
static void (*func_glSecondaryColor3dEXT)( GLdouble, GLdouble, GLdouble );
static void (*func_glSecondaryColor3dv)( GLdouble* );
static void (*func_glSecondaryColor3dvEXT)( GLdouble* );
static void (*func_glSecondaryColor3f)( GLfloat, GLfloat, GLfloat );
static void (*func_glSecondaryColor3fEXT)( GLfloat, GLfloat, GLfloat );
static void (*func_glSecondaryColor3fv)( GLfloat* );
static void (*func_glSecondaryColor3fvEXT)( GLfloat* );
static void (*func_glSecondaryColor3hNV)( unsigned short, unsigned short, unsigned short );
static void (*func_glSecondaryColor3hvNV)( unsigned short* );
static void (*func_glSecondaryColor3i)( GLint, GLint, GLint );
static void (*func_glSecondaryColor3iEXT)( GLint, GLint, GLint );
static void (*func_glSecondaryColor3iv)( GLint* );
static void (*func_glSecondaryColor3ivEXT)( GLint* );
static void (*func_glSecondaryColor3s)( GLshort, GLshort, GLshort );
static void (*func_glSecondaryColor3sEXT)( GLshort, GLshort, GLshort );
static void (*func_glSecondaryColor3sv)( GLshort* );
static void (*func_glSecondaryColor3svEXT)( GLshort* );
static void (*func_glSecondaryColor3ub)( GLubyte, GLubyte, GLubyte );
static void (*func_glSecondaryColor3ubEXT)( GLubyte, GLubyte, GLubyte );
static void (*func_glSecondaryColor3ubv)( GLubyte* );
static void (*func_glSecondaryColor3ubvEXT)( GLubyte* );
static void (*func_glSecondaryColor3ui)( GLuint, GLuint, GLuint );
static void (*func_glSecondaryColor3uiEXT)( GLuint, GLuint, GLuint );
static void (*func_glSecondaryColor3uiv)( GLuint* );
static void (*func_glSecondaryColor3uivEXT)( GLuint* );
static void (*func_glSecondaryColor3us)( GLushort, GLushort, GLushort );
static void (*func_glSecondaryColor3usEXT)( GLushort, GLushort, GLushort );
static void (*func_glSecondaryColor3usv)( GLushort* );
static void (*func_glSecondaryColor3usvEXT)( GLushort* );
static void (*func_glSecondaryColorPointer)( GLint, GLenum, GLsizei, GLvoid* );
static void (*func_glSecondaryColorPointerEXT)( GLint, GLenum, GLsizei, GLvoid* );
static void (*func_glSecondaryColorPointerListIBM)( GLint, GLenum, GLint, GLvoid**, GLint );
static void (*func_glSelectTextureCoordSetSGIS)( GLenum );
static void (*func_glSelectTextureSGIS)( GLenum );
static void (*func_glSeparableFilter2DEXT)( GLenum, GLenum, GLsizei, GLsizei, GLenum, GLenum, GLvoid*, GLvoid* );
static void (*func_glSetFenceAPPLE)( GLuint );
static void (*func_glSetFenceNV)( GLuint, GLenum );
static void (*func_glSetFragmentShaderConstantATI)( GLuint, GLfloat* );
static void (*func_glSetInvariantEXT)( GLuint, GLenum, GLvoid* );
static void (*func_glSetLocalConstantEXT)( GLuint, GLenum, GLvoid* );
static void (*func_glShaderOp1EXT)( GLenum, GLuint, GLuint );
static void (*func_glShaderOp2EXT)( GLenum, GLuint, GLuint, GLuint );
static void (*func_glShaderOp3EXT)( GLenum, GLuint, GLuint, GLuint, GLuint );
static void (*func_glShaderSource)( GLuint, GLsizei, char**, GLint* );
static void (*func_glShaderSourceARB)( unsigned int, GLsizei, char**, GLint* );
static void (*func_glSharpenTexFuncSGIS)( GLenum, GLsizei, GLfloat* );
static void (*func_glSpriteParameterfSGIX)( GLenum, GLfloat );
static void (*func_glSpriteParameterfvSGIX)( GLenum, GLfloat* );
static void (*func_glSpriteParameteriSGIX)( GLenum, GLint );
static void (*func_glSpriteParameterivSGIX)( GLenum, GLint* );
static void (*func_glStartInstrumentsSGIX)( void );
static void (*func_glStencilFuncSeparate)( GLenum, GLenum, GLint, GLuint );
static void (*func_glStencilFuncSeparateATI)( GLenum, GLenum, GLint, GLuint );
static void (*func_glStencilMaskSeparate)( GLenum, GLuint );
static void (*func_glStencilOpSeparate)( GLenum, GLenum, GLenum, GLenum );
static void (*func_glStencilOpSeparateATI)( GLenum, GLenum, GLenum, GLenum );
static void (*func_glStopInstrumentsSGIX)( GLint );
static void (*func_glStringMarkerGREMEDY)( GLsizei, GLvoid* );
static void (*func_glSwizzleEXT)( GLuint, GLuint, GLenum, GLenum, GLenum, GLenum );
static void (*func_glTagSampleBufferSGIX)( void );
static void (*func_glTangent3bEXT)( GLbyte, GLbyte, GLbyte );
static void (*func_glTangent3bvEXT)( GLbyte* );
static void (*func_glTangent3dEXT)( GLdouble, GLdouble, GLdouble );
static void (*func_glTangent3dvEXT)( GLdouble* );
static void (*func_glTangent3fEXT)( GLfloat, GLfloat, GLfloat );
static void (*func_glTangent3fvEXT)( GLfloat* );
static void (*func_glTangent3iEXT)( GLint, GLint, GLint );
static void (*func_glTangent3ivEXT)( GLint* );
static void (*func_glTangent3sEXT)( GLshort, GLshort, GLshort );
static void (*func_glTangent3svEXT)( GLshort* );
static void (*func_glTangentPointerEXT)( GLenum, GLsizei, GLvoid* );
static void (*func_glTbufferMask3DFX)( GLuint );
static GLboolean (*func_glTestFenceAPPLE)( GLuint );
static GLboolean (*func_glTestFenceNV)( GLuint );
static GLboolean (*func_glTestObjectAPPLE)( GLenum, GLuint );
static void (*func_glTexBumpParameterfvATI)( GLenum, GLfloat* );
static void (*func_glTexBumpParameterivATI)( GLenum, GLint* );
static void (*func_glTexCoord1hNV)( unsigned short );
static void (*func_glTexCoord1hvNV)( unsigned short* );
static void (*func_glTexCoord2fColor3fVertex3fSUN)( GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat );
static void (*func_glTexCoord2fColor3fVertex3fvSUN)( GLfloat*, GLfloat*, GLfloat* );
static void (*func_glTexCoord2fColor4fNormal3fVertex3fSUN)( GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat );
static void (*func_glTexCoord2fColor4fNormal3fVertex3fvSUN)( GLfloat*, GLfloat*, GLfloat*, GLfloat* );
static void (*func_glTexCoord2fColor4ubVertex3fSUN)( GLfloat, GLfloat, GLubyte, GLubyte, GLubyte, GLubyte, GLfloat, GLfloat, GLfloat );
static void (*func_glTexCoord2fColor4ubVertex3fvSUN)( GLfloat*, GLubyte*, GLfloat* );
static void (*func_glTexCoord2fNormal3fVertex3fSUN)( GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat );
static void (*func_glTexCoord2fNormal3fVertex3fvSUN)( GLfloat*, GLfloat*, GLfloat* );
static void (*func_glTexCoord2fVertex3fSUN)( GLfloat, GLfloat, GLfloat, GLfloat, GLfloat );
static void (*func_glTexCoord2fVertex3fvSUN)( GLfloat*, GLfloat* );
static void (*func_glTexCoord2hNV)( unsigned short, unsigned short );
static void (*func_glTexCoord2hvNV)( unsigned short* );
static void (*func_glTexCoord3hNV)( unsigned short, unsigned short, unsigned short );
static void (*func_glTexCoord3hvNV)( unsigned short* );
static void (*func_glTexCoord4fColor4fNormal3fVertex4fSUN)( GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat );
static void (*func_glTexCoord4fColor4fNormal3fVertex4fvSUN)( GLfloat*, GLfloat*, GLfloat*, GLfloat* );
static void (*func_glTexCoord4fVertex4fSUN)( GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat );
static void (*func_glTexCoord4fVertex4fvSUN)( GLfloat*, GLfloat* );
static void (*func_glTexCoord4hNV)( unsigned short, unsigned short, unsigned short, unsigned short );
static void (*func_glTexCoord4hvNV)( unsigned short* );
static void (*func_glTexCoordPointerEXT)( GLint, GLenum, GLsizei, GLsizei, GLvoid* );
static void (*func_glTexCoordPointerListIBM)( GLint, GLenum, GLint, GLvoid**, GLint );
static void (*func_glTexCoordPointervINTEL)( GLint, GLenum, GLvoid** );
static void (*func_glTexFilterFuncSGIS)( GLenum, GLenum, GLsizei, GLfloat* );
static void (*func_glTexImage3DEXT)( GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, GLvoid* );
static void (*func_glTexImage4DSGIS)( GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, GLvoid* );
static void (*func_glTexSubImage1DEXT)( GLenum, GLint, GLint, GLsizei, GLenum, GLenum, GLvoid* );
static void (*func_glTexSubImage2DEXT)( GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid* );
static void (*func_glTexSubImage3DEXT)( GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, GLvoid* );
static void (*func_glTexSubImage4DSGIS)( GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLsizei, GLenum, GLenum, GLvoid* );
static void (*func_glTextureColorMaskSGIS)( GLboolean, GLboolean, GLboolean, GLboolean );
static void (*func_glTextureLightEXT)( GLenum );
static void (*func_glTextureMaterialEXT)( GLenum, GLenum );
static void (*func_glTextureNormalEXT)( GLenum );
static void (*func_glTrackMatrixNV)( GLenum, GLuint, GLenum, GLenum );
static void (*func_glUniform1f)( GLint, GLfloat );
static void (*func_glUniform1fARB)( GLint, GLfloat );
static void (*func_glUniform1fv)( GLint, GLsizei, GLfloat* );
static void (*func_glUniform1fvARB)( GLint, GLsizei, GLfloat* );
static void (*func_glUniform1i)( GLint, GLint );
static void (*func_glUniform1iARB)( GLint, GLint );
static void (*func_glUniform1iv)( GLint, GLsizei, GLint* );
static void (*func_glUniform1ivARB)( GLint, GLsizei, GLint* );
static void (*func_glUniform2f)( GLint, GLfloat, GLfloat );
static void (*func_glUniform2fARB)( GLint, GLfloat, GLfloat );
static void (*func_glUniform2fv)( GLint, GLsizei, GLfloat* );
static void (*func_glUniform2fvARB)( GLint, GLsizei, GLfloat* );
static void (*func_glUniform2i)( GLint, GLint, GLint );
static void (*func_glUniform2iARB)( GLint, GLint, GLint );
static void (*func_glUniform2iv)( GLint, GLsizei, GLint* );
static void (*func_glUniform2ivARB)( GLint, GLsizei, GLint* );
static void (*func_glUniform3f)( GLint, GLfloat, GLfloat, GLfloat );
static void (*func_glUniform3fARB)( GLint, GLfloat, GLfloat, GLfloat );
static void (*func_glUniform3fv)( GLint, GLsizei, GLfloat* );
static void (*func_glUniform3fvARB)( GLint, GLsizei, GLfloat* );
static void (*func_glUniform3i)( GLint, GLint, GLint, GLint );
static void (*func_glUniform3iARB)( GLint, GLint, GLint, GLint );
static void (*func_glUniform3iv)( GLint, GLsizei, GLint* );
static void (*func_glUniform3ivARB)( GLint, GLsizei, GLint* );
static void (*func_glUniform4f)( GLint, GLfloat, GLfloat, GLfloat, GLfloat );
static void (*func_glUniform4fARB)( GLint, GLfloat, GLfloat, GLfloat, GLfloat );
static void (*func_glUniform4fv)( GLint, GLsizei, GLfloat* );
static void (*func_glUniform4fvARB)( GLint, GLsizei, GLfloat* );
static void (*func_glUniform4i)( GLint, GLint, GLint, GLint, GLint );
static void (*func_glUniform4iARB)( GLint, GLint, GLint, GLint, GLint );
static void (*func_glUniform4iv)( GLint, GLsizei, GLint* );
static void (*func_glUniform4ivARB)( GLint, GLsizei, GLint* );
static void (*func_glUniformMatrix2fv)( GLint, GLsizei, GLboolean, GLfloat* );
static void (*func_glUniformMatrix2fvARB)( GLint, GLsizei, GLboolean, GLfloat* );
static void (*func_glUniformMatrix3fv)( GLint, GLsizei, GLboolean, GLfloat* );
static void (*func_glUniformMatrix3fvARB)( GLint, GLsizei, GLboolean, GLfloat* );
static void (*func_glUniformMatrix4fv)( GLint, GLsizei, GLboolean, GLfloat* );
static void (*func_glUniformMatrix4fvARB)( GLint, GLsizei, GLboolean, GLfloat* );
static void (*func_glUnlockArraysEXT)( void );
static GLboolean (*func_glUnmapBuffer)( GLenum );
static GLboolean (*func_glUnmapBufferARB)( GLenum );
static void (*func_glUnmapObjectBufferATI)( GLuint );
static void (*func_glUpdateObjectBufferATI)( GLuint, GLuint, GLsizei, GLvoid*, GLenum );
static void (*func_glUseProgram)( GLuint );
static void (*func_glUseProgramObjectARB)( unsigned int );
static void (*func_glValidateProgram)( GLuint );
static void (*func_glValidateProgramARB)( unsigned int );
static void (*func_glVariantArrayObjectATI)( GLuint, GLenum, GLsizei, GLuint, GLuint );
static void (*func_glVariantPointerEXT)( GLuint, GLenum, GLuint, GLvoid* );
static void (*func_glVariantbvEXT)( GLuint, GLbyte* );
static void (*func_glVariantdvEXT)( GLuint, GLdouble* );
static void (*func_glVariantfvEXT)( GLuint, GLfloat* );
static void (*func_glVariantivEXT)( GLuint, GLint* );
static void (*func_glVariantsvEXT)( GLuint, GLshort* );
static void (*func_glVariantubvEXT)( GLuint, GLubyte* );
static void (*func_glVariantuivEXT)( GLuint, GLuint* );
static void (*func_glVariantusvEXT)( GLuint, GLushort* );
static void (*func_glVertex2hNV)( unsigned short, unsigned short );
static void (*func_glVertex2hvNV)( unsigned short* );
static void (*func_glVertex3hNV)( unsigned short, unsigned short, unsigned short );
static void (*func_glVertex3hvNV)( unsigned short* );
static void (*func_glVertex4hNV)( unsigned short, unsigned short, unsigned short, unsigned short );
static void (*func_glVertex4hvNV)( unsigned short* );
static void (*func_glVertexArrayParameteriAPPLE)( GLenum, GLint );
static void (*func_glVertexArrayRangeAPPLE)( GLsizei, GLvoid* );
static void (*func_glVertexArrayRangeNV)( GLsizei, GLvoid* );
static void (*func_glVertexAttrib1d)( GLuint, GLdouble );
static void (*func_glVertexAttrib1dARB)( GLuint, GLdouble );
static void (*func_glVertexAttrib1dNV)( GLuint, GLdouble );
static void (*func_glVertexAttrib1dv)( GLuint, GLdouble* );
static void (*func_glVertexAttrib1dvARB)( GLuint, GLdouble* );
static void (*func_glVertexAttrib1dvNV)( GLuint, GLdouble* );
static void (*func_glVertexAttrib1f)( GLuint, GLfloat );
static void (*func_glVertexAttrib1fARB)( GLuint, GLfloat );
static void (*func_glVertexAttrib1fNV)( GLuint, GLfloat );
static void (*func_glVertexAttrib1fv)( GLuint, GLfloat* );
static void (*func_glVertexAttrib1fvARB)( GLuint, GLfloat* );
static void (*func_glVertexAttrib1fvNV)( GLuint, GLfloat* );
static void (*func_glVertexAttrib1hNV)( GLuint, unsigned short );
static void (*func_glVertexAttrib1hvNV)( GLuint, unsigned short* );
static void (*func_glVertexAttrib1s)( GLuint, GLshort );
static void (*func_glVertexAttrib1sARB)( GLuint, GLshort );
static void (*func_glVertexAttrib1sNV)( GLuint, GLshort );
static void (*func_glVertexAttrib1sv)( GLuint, GLshort* );
static void (*func_glVertexAttrib1svARB)( GLuint, GLshort* );
static void (*func_glVertexAttrib1svNV)( GLuint, GLshort* );
static void (*func_glVertexAttrib2d)( GLuint, GLdouble, GLdouble );
static void (*func_glVertexAttrib2dARB)( GLuint, GLdouble, GLdouble );
static void (*func_glVertexAttrib2dNV)( GLuint, GLdouble, GLdouble );
static void (*func_glVertexAttrib2dv)( GLuint, GLdouble* );
static void (*func_glVertexAttrib2dvARB)( GLuint, GLdouble* );
static void (*func_glVertexAttrib2dvNV)( GLuint, GLdouble* );
static void (*func_glVertexAttrib2f)( GLuint, GLfloat, GLfloat );
static void (*func_glVertexAttrib2fARB)( GLuint, GLfloat, GLfloat );
static void (*func_glVertexAttrib2fNV)( GLuint, GLfloat, GLfloat );
static void (*func_glVertexAttrib2fv)( GLuint, GLfloat* );
static void (*func_glVertexAttrib2fvARB)( GLuint, GLfloat* );
static void (*func_glVertexAttrib2fvNV)( GLuint, GLfloat* );
static void (*func_glVertexAttrib2hNV)( GLuint, unsigned short, unsigned short );
static void (*func_glVertexAttrib2hvNV)( GLuint, unsigned short* );
static void (*func_glVertexAttrib2s)( GLuint, GLshort, GLshort );
static void (*func_glVertexAttrib2sARB)( GLuint, GLshort, GLshort );
static void (*func_glVertexAttrib2sNV)( GLuint, GLshort, GLshort );
static void (*func_glVertexAttrib2sv)( GLuint, GLshort* );
static void (*func_glVertexAttrib2svARB)( GLuint, GLshort* );
static void (*func_glVertexAttrib2svNV)( GLuint, GLshort* );
static void (*func_glVertexAttrib3d)( GLuint, GLdouble, GLdouble, GLdouble );
static void (*func_glVertexAttrib3dARB)( GLuint, GLdouble, GLdouble, GLdouble );
static void (*func_glVertexAttrib3dNV)( GLuint, GLdouble, GLdouble, GLdouble );
static void (*func_glVertexAttrib3dv)( GLuint, GLdouble* );
static void (*func_glVertexAttrib3dvARB)( GLuint, GLdouble* );
static void (*func_glVertexAttrib3dvNV)( GLuint, GLdouble* );
static void (*func_glVertexAttrib3f)( GLuint, GLfloat, GLfloat, GLfloat );
static void (*func_glVertexAttrib3fARB)( GLuint, GLfloat, GLfloat, GLfloat );
static void (*func_glVertexAttrib3fNV)( GLuint, GLfloat, GLfloat, GLfloat );
static void (*func_glVertexAttrib3fv)( GLuint, GLfloat* );
static void (*func_glVertexAttrib3fvARB)( GLuint, GLfloat* );
static void (*func_glVertexAttrib3fvNV)( GLuint, GLfloat* );
static void (*func_glVertexAttrib3hNV)( GLuint, unsigned short, unsigned short, unsigned short );
static void (*func_glVertexAttrib3hvNV)( GLuint, unsigned short* );
static void (*func_glVertexAttrib3s)( GLuint, GLshort, GLshort, GLshort );
static void (*func_glVertexAttrib3sARB)( GLuint, GLshort, GLshort, GLshort );
static void (*func_glVertexAttrib3sNV)( GLuint, GLshort, GLshort, GLshort );
static void (*func_glVertexAttrib3sv)( GLuint, GLshort* );
static void (*func_glVertexAttrib3svARB)( GLuint, GLshort* );
static void (*func_glVertexAttrib3svNV)( GLuint, GLshort* );
static void (*func_glVertexAttrib4Nbv)( GLuint, GLbyte* );
static void (*func_glVertexAttrib4NbvARB)( GLuint, GLbyte* );
static void (*func_glVertexAttrib4Niv)( GLuint, GLint* );
static void (*func_glVertexAttrib4NivARB)( GLuint, GLint* );
static void (*func_glVertexAttrib4Nsv)( GLuint, GLshort* );
static void (*func_glVertexAttrib4NsvARB)( GLuint, GLshort* );
static void (*func_glVertexAttrib4Nub)( GLuint, GLubyte, GLubyte, GLubyte, GLubyte );
static void (*func_glVertexAttrib4NubARB)( GLuint, GLubyte, GLubyte, GLubyte, GLubyte );
static void (*func_glVertexAttrib4Nubv)( GLuint, GLubyte* );
static void (*func_glVertexAttrib4NubvARB)( GLuint, GLubyte* );
static void (*func_glVertexAttrib4Nuiv)( GLuint, GLuint* );
static void (*func_glVertexAttrib4NuivARB)( GLuint, GLuint* );
static void (*func_glVertexAttrib4Nusv)( GLuint, GLushort* );
static void (*func_glVertexAttrib4NusvARB)( GLuint, GLushort* );
static void (*func_glVertexAttrib4bv)( GLuint, GLbyte* );
static void (*func_glVertexAttrib4bvARB)( GLuint, GLbyte* );
static void (*func_glVertexAttrib4d)( GLuint, GLdouble, GLdouble, GLdouble, GLdouble );
static void (*func_glVertexAttrib4dARB)( GLuint, GLdouble, GLdouble, GLdouble, GLdouble );
static void (*func_glVertexAttrib4dNV)( GLuint, GLdouble, GLdouble, GLdouble, GLdouble );
static void (*func_glVertexAttrib4dv)( GLuint, GLdouble* );
static void (*func_glVertexAttrib4dvARB)( GLuint, GLdouble* );
static void (*func_glVertexAttrib4dvNV)( GLuint, GLdouble* );
static void (*func_glVertexAttrib4f)( GLuint, GLfloat, GLfloat, GLfloat, GLfloat );
static void (*func_glVertexAttrib4fARB)( GLuint, GLfloat, GLfloat, GLfloat, GLfloat );
static void (*func_glVertexAttrib4fNV)( GLuint, GLfloat, GLfloat, GLfloat, GLfloat );
static void (*func_glVertexAttrib4fv)( GLuint, GLfloat* );
static void (*func_glVertexAttrib4fvARB)( GLuint, GLfloat* );
static void (*func_glVertexAttrib4fvNV)( GLuint, GLfloat* );
static void (*func_glVertexAttrib4hNV)( GLuint, unsigned short, unsigned short, unsigned short, unsigned short );
static void (*func_glVertexAttrib4hvNV)( GLuint, unsigned short* );
static void (*func_glVertexAttrib4iv)( GLuint, GLint* );
static void (*func_glVertexAttrib4ivARB)( GLuint, GLint* );
static void (*func_glVertexAttrib4s)( GLuint, GLshort, GLshort, GLshort, GLshort );
static void (*func_glVertexAttrib4sARB)( GLuint, GLshort, GLshort, GLshort, GLshort );
static void (*func_glVertexAttrib4sNV)( GLuint, GLshort, GLshort, GLshort, GLshort );
static void (*func_glVertexAttrib4sv)( GLuint, GLshort* );
static void (*func_glVertexAttrib4svARB)( GLuint, GLshort* );
static void (*func_glVertexAttrib4svNV)( GLuint, GLshort* );
static void (*func_glVertexAttrib4ubNV)( GLuint, GLubyte, GLubyte, GLubyte, GLubyte );
static void (*func_glVertexAttrib4ubv)( GLuint, GLubyte* );
static void (*func_glVertexAttrib4ubvARB)( GLuint, GLubyte* );
static void (*func_glVertexAttrib4ubvNV)( GLuint, GLubyte* );
static void (*func_glVertexAttrib4uiv)( GLuint, GLuint* );
static void (*func_glVertexAttrib4uivARB)( GLuint, GLuint* );
static void (*func_glVertexAttrib4usv)( GLuint, GLushort* );
static void (*func_glVertexAttrib4usvARB)( GLuint, GLushort* );
static void (*func_glVertexAttribArrayObjectATI)( GLuint, GLint, GLenum, GLboolean, GLsizei, GLuint, GLuint );
static void (*func_glVertexAttribPointer)( GLuint, GLint, GLenum, GLboolean, GLsizei, GLvoid* );
static void (*func_glVertexAttribPointerARB)( GLuint, GLint, GLenum, GLboolean, GLsizei, GLvoid* );
static void (*func_glVertexAttribPointerNV)( GLuint, GLint, GLenum, GLsizei, GLvoid* );
static void (*func_glVertexAttribs1dvNV)( GLuint, GLsizei, GLdouble* );
static void (*func_glVertexAttribs1fvNV)( GLuint, GLsizei, GLfloat* );
static void (*func_glVertexAttribs1hvNV)( GLuint, GLsizei, unsigned short* );
static void (*func_glVertexAttribs1svNV)( GLuint, GLsizei, GLshort* );
static void (*func_glVertexAttribs2dvNV)( GLuint, GLsizei, GLdouble* );
static void (*func_glVertexAttribs2fvNV)( GLuint, GLsizei, GLfloat* );
static void (*func_glVertexAttribs2hvNV)( GLuint, GLsizei, unsigned short* );
static void (*func_glVertexAttribs2svNV)( GLuint, GLsizei, GLshort* );
static void (*func_glVertexAttribs3dvNV)( GLuint, GLsizei, GLdouble* );
static void (*func_glVertexAttribs3fvNV)( GLuint, GLsizei, GLfloat* );
static void (*func_glVertexAttribs3hvNV)( GLuint, GLsizei, unsigned short* );
static void (*func_glVertexAttribs3svNV)( GLuint, GLsizei, GLshort* );
static void (*func_glVertexAttribs4dvNV)( GLuint, GLsizei, GLdouble* );
static void (*func_glVertexAttribs4fvNV)( GLuint, GLsizei, GLfloat* );
static void (*func_glVertexAttribs4hvNV)( GLuint, GLsizei, unsigned short* );
static void (*func_glVertexAttribs4svNV)( GLuint, GLsizei, GLshort* );
static void (*func_glVertexAttribs4ubvNV)( GLuint, GLsizei, GLubyte* );
static void (*func_glVertexBlendARB)( GLint );
static void (*func_glVertexBlendEnvfATI)( GLenum, GLfloat );
static void (*func_glVertexBlendEnviATI)( GLenum, GLint );
static void (*func_glVertexPointerEXT)( GLint, GLenum, GLsizei, GLsizei, GLvoid* );
static void (*func_glVertexPointerListIBM)( GLint, GLenum, GLint, GLvoid**, GLint );
static void (*func_glVertexPointervINTEL)( GLint, GLenum, GLvoid** );
static void (*func_glVertexStream1dATI)( GLenum, GLdouble );
static void (*func_glVertexStream1dvATI)( GLenum, GLdouble* );
static void (*func_glVertexStream1fATI)( GLenum, GLfloat );
static void (*func_glVertexStream1fvATI)( GLenum, GLfloat* );
static void (*func_glVertexStream1iATI)( GLenum, GLint );
static void (*func_glVertexStream1ivATI)( GLenum, GLint* );
static void (*func_glVertexStream1sATI)( GLenum, GLshort );
static void (*func_glVertexStream1svATI)( GLenum, GLshort* );
static void (*func_glVertexStream2dATI)( GLenum, GLdouble, GLdouble );
static void (*func_glVertexStream2dvATI)( GLenum, GLdouble* );
static void (*func_glVertexStream2fATI)( GLenum, GLfloat, GLfloat );
static void (*func_glVertexStream2fvATI)( GLenum, GLfloat* );
static void (*func_glVertexStream2iATI)( GLenum, GLint, GLint );
static void (*func_glVertexStream2ivATI)( GLenum, GLint* );
static void (*func_glVertexStream2sATI)( GLenum, GLshort, GLshort );
static void (*func_glVertexStream2svATI)( GLenum, GLshort* );
static void (*func_glVertexStream3dATI)( GLenum, GLdouble, GLdouble, GLdouble );
static void (*func_glVertexStream3dvATI)( GLenum, GLdouble* );
static void (*func_glVertexStream3fATI)( GLenum, GLfloat, GLfloat, GLfloat );
static void (*func_glVertexStream3fvATI)( GLenum, GLfloat* );
static void (*func_glVertexStream3iATI)( GLenum, GLint, GLint, GLint );
static void (*func_glVertexStream3ivATI)( GLenum, GLint* );
static void (*func_glVertexStream3sATI)( GLenum, GLshort, GLshort, GLshort );
static void (*func_glVertexStream3svATI)( GLenum, GLshort* );
static void (*func_glVertexStream4dATI)( GLenum, GLdouble, GLdouble, GLdouble, GLdouble );
static void (*func_glVertexStream4dvATI)( GLenum, GLdouble* );
static void (*func_glVertexStream4fATI)( GLenum, GLfloat, GLfloat, GLfloat, GLfloat );
static void (*func_glVertexStream4fvATI)( GLenum, GLfloat* );
static void (*func_glVertexStream4iATI)( GLenum, GLint, GLint, GLint, GLint );
static void (*func_glVertexStream4ivATI)( GLenum, GLint* );
static void (*func_glVertexStream4sATI)( GLenum, GLshort, GLshort, GLshort, GLshort );
static void (*func_glVertexStream4svATI)( GLenum, GLshort* );
static void (*func_glVertexWeightPointerEXT)( GLsizei, GLenum, GLsizei, GLvoid* );
static void (*func_glVertexWeightfEXT)( GLfloat );
static void (*func_glVertexWeightfvEXT)( GLfloat* );
static void (*func_glVertexWeighthNV)( unsigned short );
static void (*func_glVertexWeighthvNV)( unsigned short* );
static void (*func_glWeightPointerARB)( GLint, GLenum, GLsizei, GLvoid* );
static void (*func_glWeightbvARB)( GLint, GLbyte* );
static void (*func_glWeightdvARB)( GLint, GLdouble* );
static void (*func_glWeightfvARB)( GLint, GLfloat* );
static void (*func_glWeightivARB)( GLint, GLint* );
static void (*func_glWeightsvARB)( GLint, GLshort* );
static void (*func_glWeightubvARB)( GLint, GLubyte* );
static void (*func_glWeightuivARB)( GLint, GLuint* );
static void (*func_glWeightusvARB)( GLint, GLushort* );
static void (*func_glWindowPos2d)( GLdouble, GLdouble );
static void (*func_glWindowPos2dARB)( GLdouble, GLdouble );
static void (*func_glWindowPos2dMESA)( GLdouble, GLdouble );
static void (*func_glWindowPos2dv)( GLdouble* );
static void (*func_glWindowPos2dvARB)( GLdouble* );
static void (*func_glWindowPos2dvMESA)( GLdouble* );
static void (*func_glWindowPos2f)( GLfloat, GLfloat );
static void (*func_glWindowPos2fARB)( GLfloat, GLfloat );
static void (*func_glWindowPos2fMESA)( GLfloat, GLfloat );
static void (*func_glWindowPos2fv)( GLfloat* );
static void (*func_glWindowPos2fvARB)( GLfloat* );
static void (*func_glWindowPos2fvMESA)( GLfloat* );
static void (*func_glWindowPos2i)( GLint, GLint );
static void (*func_glWindowPos2iARB)( GLint, GLint );
static void (*func_glWindowPos2iMESA)( GLint, GLint );
static void (*func_glWindowPos2iv)( GLint* );
static void (*func_glWindowPos2ivARB)( GLint* );
static void (*func_glWindowPos2ivMESA)( GLint* );
static void (*func_glWindowPos2s)( GLshort, GLshort );
static void (*func_glWindowPos2sARB)( GLshort, GLshort );
static void (*func_glWindowPos2sMESA)( GLshort, GLshort );
static void (*func_glWindowPos2sv)( GLshort* );
static void (*func_glWindowPos2svARB)( GLshort* );
static void (*func_glWindowPos2svMESA)( GLshort* );
static void (*func_glWindowPos3d)( GLdouble, GLdouble, GLdouble );
static void (*func_glWindowPos3dARB)( GLdouble, GLdouble, GLdouble );
static void (*func_glWindowPos3dMESA)( GLdouble, GLdouble, GLdouble );
static void (*func_glWindowPos3dv)( GLdouble* );
static void (*func_glWindowPos3dvARB)( GLdouble* );
static void (*func_glWindowPos3dvMESA)( GLdouble* );
static void (*func_glWindowPos3f)( GLfloat, GLfloat, GLfloat );
static void (*func_glWindowPos3fARB)( GLfloat, GLfloat, GLfloat );
static void (*func_glWindowPos3fMESA)( GLfloat, GLfloat, GLfloat );
static void (*func_glWindowPos3fv)( GLfloat* );
static void (*func_glWindowPos3fvARB)( GLfloat* );
static void (*func_glWindowPos3fvMESA)( GLfloat* );
static void (*func_glWindowPos3i)( GLint, GLint, GLint );
static void (*func_glWindowPos3iARB)( GLint, GLint, GLint );
static void (*func_glWindowPos3iMESA)( GLint, GLint, GLint );
static void (*func_glWindowPos3iv)( GLint* );
static void (*func_glWindowPos3ivARB)( GLint* );
static void (*func_glWindowPos3ivMESA)( GLint* );
static void (*func_glWindowPos3s)( GLshort, GLshort, GLshort );
static void (*func_glWindowPos3sARB)( GLshort, GLshort, GLshort );
static void (*func_glWindowPos3sMESA)( GLshort, GLshort, GLshort );
static void (*func_glWindowPos3sv)( GLshort* );
static void (*func_glWindowPos3svARB)( GLshort* );
static void (*func_glWindowPos3svMESA)( GLshort* );
static void (*func_glWindowPos4dMESA)( GLdouble, GLdouble, GLdouble, GLdouble );
static void (*func_glWindowPos4dvMESA)( GLdouble* );
static void (*func_glWindowPos4fMESA)( GLfloat, GLfloat, GLfloat, GLfloat );
static void (*func_glWindowPos4fvMESA)( GLfloat* );
static void (*func_glWindowPos4iMESA)( GLint, GLint, GLint, GLint );
static void (*func_glWindowPos4ivMESA)( GLint* );
static void (*func_glWindowPos4sMESA)( GLshort, GLshort, GLshort, GLshort );
static void (*func_glWindowPos4svMESA)( GLshort* );
static void (*func_glWriteMaskEXT)( GLuint, GLuint, GLenum, GLenum, GLenum, GLenum );
static void * (*func_wglAllocateMemoryNV)( GLsizei, GLfloat, GLfloat, GLfloat );
static void (*func_wglFreeMemoryNV)( GLvoid * );

/* The thunks themselves....*/
static void WINAPI wine_glActiveStencilFaceEXT( GLenum face ) {
  TRACE("(%d)\n", face );
  ENTER_GL();
  func_glActiveStencilFaceEXT( face );
  LEAVE_GL();
}

static void WINAPI wine_glActiveTexture( GLenum texture ) {
  TRACE("(%d)\n", texture );
  ENTER_GL();
  func_glActiveTexture( texture );
  LEAVE_GL();
}

static void WINAPI wine_glActiveTextureARB( GLenum texture ) {
  TRACE("(%d)\n", texture );
  ENTER_GL();
  func_glActiveTextureARB( texture );
  LEAVE_GL();
}

static void WINAPI wine_glAlphaFragmentOp1ATI( GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod ) {
  TRACE("(%d, %d, %d, %d, %d, %d)\n", op, dst, dstMod, arg1, arg1Rep, arg1Mod );
  ENTER_GL();
  func_glAlphaFragmentOp1ATI( op, dst, dstMod, arg1, arg1Rep, arg1Mod );
  LEAVE_GL();
}

static void WINAPI wine_glAlphaFragmentOp2ATI( GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d)\n", op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod );
  ENTER_GL();
  func_glAlphaFragmentOp2ATI( op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod );
  LEAVE_GL();
}

static void WINAPI wine_glAlphaFragmentOp3ATI( GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod );
  ENTER_GL();
  func_glAlphaFragmentOp3ATI( op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod );
  LEAVE_GL();
}

static void WINAPI wine_glApplyTextureEXT( GLenum mode ) {
  TRACE("(%d)\n", mode );
  ENTER_GL();
  func_glApplyTextureEXT( mode );
  LEAVE_GL();
}

static GLboolean WINAPI wine_glAreProgramsResidentNV( GLsizei n, GLuint* programs, GLboolean* residences ) {
  GLboolean ret_value;
  TRACE("(%d, %p, %p)\n", n, programs, residences );
  ENTER_GL();
  ret_value = func_glAreProgramsResidentNV( n, programs, residences );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glAreTexturesResidentEXT( GLsizei n, GLuint* textures, GLboolean* residences ) {
  GLboolean ret_value;
  TRACE("(%d, %p, %p)\n", n, textures, residences );
  ENTER_GL();
  ret_value = func_glAreTexturesResidentEXT( n, textures, residences );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glArrayElementEXT( GLint i ) {
  TRACE("(%d)\n", i );
  ENTER_GL();
  func_glArrayElementEXT( i );
  LEAVE_GL();
}

static void WINAPI wine_glArrayObjectATI( GLenum array, GLint size, GLenum type, GLsizei stride, GLuint buffer, GLuint offset ) {
  TRACE("(%d, %d, %d, %d, %d, %d)\n", array, size, type, stride, buffer, offset );
  ENTER_GL();
  func_glArrayObjectATI( array, size, type, stride, buffer, offset );
  LEAVE_GL();
}

static void WINAPI wine_glAsyncMarkerSGIX( GLuint marker ) {
  TRACE("(%d)\n", marker );
  ENTER_GL();
  func_glAsyncMarkerSGIX( marker );
  LEAVE_GL();
}

static void WINAPI wine_glAttachObjectARB( unsigned int containerObj, unsigned int obj ) {
  TRACE("(%d, %d)\n", containerObj, obj );
  ENTER_GL();
  func_glAttachObjectARB( containerObj, obj );
  LEAVE_GL();
}

static void WINAPI wine_glAttachShader( GLuint program, GLuint shader ) {
  TRACE("(%d, %d)\n", program, shader );
  ENTER_GL();
  func_glAttachShader( program, shader );
  LEAVE_GL();
}

static void WINAPI wine_glBeginFragmentShaderATI( void ) {
  TRACE("()\n");
  ENTER_GL();
  func_glBeginFragmentShaderATI( );
  LEAVE_GL();
}

static void WINAPI wine_glBeginOcclusionQueryNV( GLuint id ) {
  TRACE("(%d)\n", id );
  ENTER_GL();
  func_glBeginOcclusionQueryNV( id );
  LEAVE_GL();
}

static void WINAPI wine_glBeginQuery( GLenum target, GLuint id ) {
  TRACE("(%d, %d)\n", target, id );
  ENTER_GL();
  func_glBeginQuery( target, id );
  LEAVE_GL();
}

static void WINAPI wine_glBeginQueryARB( GLenum target, GLuint id ) {
  TRACE("(%d, %d)\n", target, id );
  ENTER_GL();
  func_glBeginQueryARB( target, id );
  LEAVE_GL();
}

static void WINAPI wine_glBeginVertexShaderEXT( void ) {
  TRACE("()\n");
  ENTER_GL();
  func_glBeginVertexShaderEXT( );
  LEAVE_GL();
}

static void WINAPI wine_glBindAttribLocation( GLuint program, GLuint index, char* name ) {
  TRACE("(%d, %d, %p)\n", program, index, name );
  ENTER_GL();
  func_glBindAttribLocation( program, index, name );
  LEAVE_GL();
}

static void WINAPI wine_glBindAttribLocationARB( unsigned int programObj, GLuint index, char* name ) {
  TRACE("(%d, %d, %p)\n", programObj, index, name );
  ENTER_GL();
  func_glBindAttribLocationARB( programObj, index, name );
  LEAVE_GL();
}

static void WINAPI wine_glBindBuffer( GLenum target, GLuint buffer ) {
  TRACE("(%d, %d)\n", target, buffer );
  ENTER_GL();
  func_glBindBuffer( target, buffer );
  LEAVE_GL();
}

static void WINAPI wine_glBindBufferARB( GLenum target, GLuint buffer ) {
  TRACE("(%d, %d)\n", target, buffer );
  ENTER_GL();
  func_glBindBufferARB( target, buffer );
  LEAVE_GL();
}

static void WINAPI wine_glBindFragmentShaderATI( GLuint id ) {
  TRACE("(%d)\n", id );
  ENTER_GL();
  func_glBindFragmentShaderATI( id );
  LEAVE_GL();
}

static void WINAPI wine_glBindFramebufferEXT( GLenum target, GLuint framebuffer ) {
  TRACE("(%d, %d)\n", target, framebuffer );
  ENTER_GL();
  func_glBindFramebufferEXT( target, framebuffer );
  LEAVE_GL();
}

static GLuint WINAPI wine_glBindLightParameterEXT( GLenum light, GLenum value ) {
  GLuint ret_value;
  TRACE("(%d, %d)\n", light, value );
  ENTER_GL();
  ret_value = func_glBindLightParameterEXT( light, value );
  LEAVE_GL();
  return ret_value;
}

static GLuint WINAPI wine_glBindMaterialParameterEXT( GLenum face, GLenum value ) {
  GLuint ret_value;
  TRACE("(%d, %d)\n", face, value );
  ENTER_GL();
  ret_value = func_glBindMaterialParameterEXT( face, value );
  LEAVE_GL();
  return ret_value;
}

static GLuint WINAPI wine_glBindParameterEXT( GLenum value ) {
  GLuint ret_value;
  TRACE("(%d)\n", value );
  ENTER_GL();
  ret_value = func_glBindParameterEXT( value );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glBindProgramARB( GLenum target, GLuint program ) {
  TRACE("(%d, %d)\n", target, program );
  ENTER_GL();
  func_glBindProgramARB( target, program );
  LEAVE_GL();
}

static void WINAPI wine_glBindProgramNV( GLenum target, GLuint id ) {
  TRACE("(%d, %d)\n", target, id );
  ENTER_GL();
  func_glBindProgramNV( target, id );
  LEAVE_GL();
}

static void WINAPI wine_glBindRenderbufferEXT( GLenum target, GLuint renderbuffer ) {
  TRACE("(%d, %d)\n", target, renderbuffer );
  ENTER_GL();
  func_glBindRenderbufferEXT( target, renderbuffer );
  LEAVE_GL();
}

static GLuint WINAPI wine_glBindTexGenParameterEXT( GLenum unit, GLenum coord, GLenum value ) {
  GLuint ret_value;
  TRACE("(%d, %d, %d)\n", unit, coord, value );
  ENTER_GL();
  ret_value = func_glBindTexGenParameterEXT( unit, coord, value );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glBindTextureEXT( GLenum target, GLuint texture ) {
  TRACE("(%d, %d)\n", target, texture );
  ENTER_GL();
  func_glBindTextureEXT( target, texture );
  LEAVE_GL();
}

static GLuint WINAPI wine_glBindTextureUnitParameterEXT( GLenum unit, GLenum value ) {
  GLuint ret_value;
  TRACE("(%d, %d)\n", unit, value );
  ENTER_GL();
  ret_value = func_glBindTextureUnitParameterEXT( unit, value );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glBindVertexArrayAPPLE( GLuint array ) {
  TRACE("(%d)\n", array );
  ENTER_GL();
  func_glBindVertexArrayAPPLE( array );
  LEAVE_GL();
}

static void WINAPI wine_glBindVertexShaderEXT( GLuint id ) {
  TRACE("(%d)\n", id );
  ENTER_GL();
  func_glBindVertexShaderEXT( id );
  LEAVE_GL();
}

static void WINAPI wine_glBinormal3bEXT( GLbyte bx, GLbyte by, GLbyte bz ) {
  TRACE("(%d, %d, %d)\n", bx, by, bz );
  ENTER_GL();
  func_glBinormal3bEXT( bx, by, bz );
  LEAVE_GL();
}

static void WINAPI wine_glBinormal3bvEXT( GLbyte* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glBinormal3bvEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glBinormal3dEXT( GLdouble bx, GLdouble by, GLdouble bz ) {
  TRACE("(%f, %f, %f)\n", bx, by, bz );
  ENTER_GL();
  func_glBinormal3dEXT( bx, by, bz );
  LEAVE_GL();
}

static void WINAPI wine_glBinormal3dvEXT( GLdouble* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glBinormal3dvEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glBinormal3fEXT( GLfloat bx, GLfloat by, GLfloat bz ) {
  TRACE("(%f, %f, %f)\n", bx, by, bz );
  ENTER_GL();
  func_glBinormal3fEXT( bx, by, bz );
  LEAVE_GL();
}

static void WINAPI wine_glBinormal3fvEXT( GLfloat* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glBinormal3fvEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glBinormal3iEXT( GLint bx, GLint by, GLint bz ) {
  TRACE("(%d, %d, %d)\n", bx, by, bz );
  ENTER_GL();
  func_glBinormal3iEXT( bx, by, bz );
  LEAVE_GL();
}

static void WINAPI wine_glBinormal3ivEXT( GLint* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glBinormal3ivEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glBinormal3sEXT( GLshort bx, GLshort by, GLshort bz ) {
  TRACE("(%d, %d, %d)\n", bx, by, bz );
  ENTER_GL();
  func_glBinormal3sEXT( bx, by, bz );
  LEAVE_GL();
}

static void WINAPI wine_glBinormal3svEXT( GLshort* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glBinormal3svEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glBinormalPointerEXT( GLenum type, GLsizei stride, GLvoid* pointer ) {
  TRACE("(%d, %d, %p)\n", type, stride, pointer );
  ENTER_GL();
  func_glBinormalPointerEXT( type, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glBlendColorEXT( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha ) {
  TRACE("(%f, %f, %f, %f)\n", red, green, blue, alpha );
  ENTER_GL();
  func_glBlendColorEXT( red, green, blue, alpha );
  LEAVE_GL();
}

static void WINAPI wine_glBlendEquationEXT( GLenum mode ) {
  TRACE("(%d)\n", mode );
  ENTER_GL();
  func_glBlendEquationEXT( mode );
  LEAVE_GL();
}

static void WINAPI wine_glBlendEquationSeparate( GLenum modeRGB, GLenum modeAlpha ) {
  TRACE("(%d, %d)\n", modeRGB, modeAlpha );
  ENTER_GL();
  func_glBlendEquationSeparate( modeRGB, modeAlpha );
  LEAVE_GL();
}

static void WINAPI wine_glBlendEquationSeparateEXT( GLenum modeRGB, GLenum modeAlpha ) {
  TRACE("(%d, %d)\n", modeRGB, modeAlpha );
  ENTER_GL();
  func_glBlendEquationSeparateEXT( modeRGB, modeAlpha );
  LEAVE_GL();
}

static void WINAPI wine_glBlendFuncSeparate( GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha ) {
  TRACE("(%d, %d, %d, %d)\n", sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha );
  ENTER_GL();
  func_glBlendFuncSeparate( sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha );
  LEAVE_GL();
}

static void WINAPI wine_glBlendFuncSeparateEXT( GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha ) {
  TRACE("(%d, %d, %d, %d)\n", sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha );
  ENTER_GL();
  func_glBlendFuncSeparateEXT( sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha );
  LEAVE_GL();
}

static void WINAPI wine_glBlendFuncSeparateINGR( GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha ) {
  TRACE("(%d, %d, %d, %d)\n", sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha );
  ENTER_GL();
  func_glBlendFuncSeparateINGR( sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha );
  LEAVE_GL();
}

static void WINAPI wine_glBufferData( GLenum target, ptrdiff_t size, GLvoid* data, GLenum usage ) {
  TRACE("(%d, %d, %p, %d)\n", target, size, data, usage );
  ENTER_GL();
  func_glBufferData( target, size, data, usage );
  LEAVE_GL();
}

static void WINAPI wine_glBufferDataARB( GLenum target, ptrdiff_t size, GLvoid* data, GLenum usage ) {
  TRACE("(%d, %d, %p, %d)\n", target, size, data, usage );
  ENTER_GL();
  func_glBufferDataARB( target, size, data, usage );
  LEAVE_GL();
}

static GLuint WINAPI wine_glBufferRegionEnabled( void ) {
  GLuint ret_value;
  TRACE("()\n");
  ENTER_GL();
  ret_value = func_glBufferRegionEnabled( );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glBufferSubData( GLenum target, ptrdiff_t offset, ptrdiff_t size, GLvoid* data ) {
  TRACE("(%d, %d, %d, %p)\n", target, offset, size, data );
  ENTER_GL();
  func_glBufferSubData( target, offset, size, data );
  LEAVE_GL();
}

static void WINAPI wine_glBufferSubDataARB( GLenum target, ptrdiff_t offset, ptrdiff_t size, GLvoid* data ) {
  TRACE("(%d, %d, %d, %p)\n", target, offset, size, data );
  ENTER_GL();
  func_glBufferSubDataARB( target, offset, size, data );
  LEAVE_GL();
}

static GLenum WINAPI wine_glCheckFramebufferStatusEXT( GLenum target ) {
  GLenum ret_value;
  TRACE("(%d)\n", target );
  ENTER_GL();
  ret_value = func_glCheckFramebufferStatusEXT( target );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glClampColorARB( GLenum target, GLenum clamp ) {
  TRACE("(%d, %d)\n", target, clamp );
  ENTER_GL();
  func_glClampColorARB( target, clamp );
  LEAVE_GL();
}

static void WINAPI wine_glClientActiveTexture( GLenum texture ) {
  TRACE("(%d)\n", texture );
  ENTER_GL();
  func_glClientActiveTexture( texture );
  LEAVE_GL();
}

static void WINAPI wine_glClientActiveTextureARB( GLenum texture ) {
  TRACE("(%d)\n", texture );
  ENTER_GL();
  func_glClientActiveTextureARB( texture );
  LEAVE_GL();
}

static void WINAPI wine_glClientActiveVertexStreamATI( GLenum stream ) {
  TRACE("(%d)\n", stream );
  ENTER_GL();
  func_glClientActiveVertexStreamATI( stream );
  LEAVE_GL();
}

static void WINAPI wine_glColor3fVertex3fSUN( GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z ) {
  TRACE("(%f, %f, %f, %f, %f, %f)\n", r, g, b, x, y, z );
  ENTER_GL();
  func_glColor3fVertex3fSUN( r, g, b, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glColor3fVertex3fvSUN( GLfloat* c, GLfloat* v ) {
  TRACE("(%p, %p)\n", c, v );
  ENTER_GL();
  func_glColor3fVertex3fvSUN( c, v );
  LEAVE_GL();
}

static void WINAPI wine_glColor3hNV( unsigned short red, unsigned short green, unsigned short blue ) {
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glColor3hNV( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glColor3hvNV( unsigned short* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glColor3hvNV( v );
  LEAVE_GL();
}

static void WINAPI wine_glColor4fNormal3fVertex3fSUN( GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) {
  TRACE("(%f, %f, %f, %f, %f, %f, %f, %f, %f, %f)\n", r, g, b, a, nx, ny, nz, x, y, z );
  ENTER_GL();
  func_glColor4fNormal3fVertex3fSUN( r, g, b, a, nx, ny, nz, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glColor4fNormal3fVertex3fvSUN( GLfloat* c, GLfloat* n, GLfloat* v ) {
  TRACE("(%p, %p, %p)\n", c, n, v );
  ENTER_GL();
  func_glColor4fNormal3fVertex3fvSUN( c, n, v );
  LEAVE_GL();
}

static void WINAPI wine_glColor4hNV( unsigned short red, unsigned short green, unsigned short blue, unsigned short alpha ) {
  TRACE("(%d, %d, %d, %d)\n", red, green, blue, alpha );
  ENTER_GL();
  func_glColor4hNV( red, green, blue, alpha );
  LEAVE_GL();
}

static void WINAPI wine_glColor4hvNV( unsigned short* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glColor4hvNV( v );
  LEAVE_GL();
}

static void WINAPI wine_glColor4ubVertex2fSUN( GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y ) {
  TRACE("(%d, %d, %d, %d, %f, %f)\n", r, g, b, a, x, y );
  ENTER_GL();
  func_glColor4ubVertex2fSUN( r, g, b, a, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glColor4ubVertex2fvSUN( GLubyte* c, GLfloat* v ) {
  TRACE("(%p, %p)\n", c, v );
  ENTER_GL();
  func_glColor4ubVertex2fvSUN( c, v );
  LEAVE_GL();
}

static void WINAPI wine_glColor4ubVertex3fSUN( GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z ) {
  TRACE("(%d, %d, %d, %d, %f, %f, %f)\n", r, g, b, a, x, y, z );
  ENTER_GL();
  func_glColor4ubVertex3fSUN( r, g, b, a, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glColor4ubVertex3fvSUN( GLubyte* c, GLfloat* v ) {
  TRACE("(%p, %p)\n", c, v );
  ENTER_GL();
  func_glColor4ubVertex3fvSUN( c, v );
  LEAVE_GL();
}

static void WINAPI wine_glColorFragmentOp1ATI( GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %d)\n", op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod );
  ENTER_GL();
  func_glColorFragmentOp1ATI( op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod );
  LEAVE_GL();
}

static void WINAPI wine_glColorFragmentOp2ATI( GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod );
  ENTER_GL();
  func_glColorFragmentOp2ATI( op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod );
  LEAVE_GL();
}

static void WINAPI wine_glColorFragmentOp3ATI( GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod );
  ENTER_GL();
  func_glColorFragmentOp3ATI( op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod );
  LEAVE_GL();
}

static void WINAPI wine_glColorPointerEXT( GLint size, GLenum type, GLsizei stride, GLsizei count, GLvoid* pointer ) {
  TRACE("(%d, %d, %d, %d, %p)\n", size, type, stride, count, pointer );
  ENTER_GL();
  func_glColorPointerEXT( size, type, stride, count, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glColorPointerListIBM( GLint size, GLenum type, GLint stride, GLvoid** pointer, GLint ptrstride ) {
  TRACE("(%d, %d, %d, %p, %d)\n", size, type, stride, pointer, ptrstride );
  ENTER_GL();
  func_glColorPointerListIBM( size, type, stride, pointer, ptrstride );
  LEAVE_GL();
}

static void WINAPI wine_glColorPointervINTEL( GLint size, GLenum type, GLvoid** pointer ) {
  TRACE("(%d, %d, %p)\n", size, type, pointer );
  ENTER_GL();
  func_glColorPointervINTEL( size, type, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glColorSubTableEXT( GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, GLvoid* data ) {
  TRACE("(%d, %d, %d, %d, %d, %p)\n", target, start, count, format, type, data );
  ENTER_GL();
  func_glColorSubTableEXT( target, start, count, format, type, data );
  LEAVE_GL();
}

static void WINAPI wine_glColorTableEXT( GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type, GLvoid* table ) {
  TRACE("(%d, %d, %d, %d, %d, %p)\n", target, internalFormat, width, format, type, table );
  ENTER_GL();
  func_glColorTableEXT( target, internalFormat, width, format, type, table );
  LEAVE_GL();
}

static void WINAPI wine_glColorTableParameterfvSGI( GLenum target, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glColorTableParameterfvSGI( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glColorTableParameterivSGI( GLenum target, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glColorTableParameterivSGI( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glColorTableSGI( GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, GLvoid* table ) {
  TRACE("(%d, %d, %d, %d, %d, %p)\n", target, internalformat, width, format, type, table );
  ENTER_GL();
  func_glColorTableSGI( target, internalformat, width, format, type, table );
  LEAVE_GL();
}

static void WINAPI wine_glCombinerInputNV( GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage ) {
  TRACE("(%d, %d, %d, %d, %d, %d)\n", stage, portion, variable, input, mapping, componentUsage );
  ENTER_GL();
  func_glCombinerInputNV( stage, portion, variable, input, mapping, componentUsage );
  LEAVE_GL();
}

static void WINAPI wine_glCombinerOutputNV( GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct, GLboolean cdDotProduct, GLboolean muxSum ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", stage, portion, abOutput, cdOutput, sumOutput, scale, bias, abDotProduct, cdDotProduct, muxSum );
  ENTER_GL();
  func_glCombinerOutputNV( stage, portion, abOutput, cdOutput, sumOutput, scale, bias, abDotProduct, cdDotProduct, muxSum );
  LEAVE_GL();
}

static void WINAPI wine_glCombinerParameterfNV( GLenum pname, GLfloat param ) {
  TRACE("(%d, %f)\n", pname, param );
  ENTER_GL();
  func_glCombinerParameterfNV( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glCombinerParameterfvNV( GLenum pname, GLfloat* params ) {
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glCombinerParameterfvNV( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glCombinerParameteriNV( GLenum pname, GLint param ) {
  TRACE("(%d, %d)\n", pname, param );
  ENTER_GL();
  func_glCombinerParameteriNV( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glCombinerParameterivNV( GLenum pname, GLint* params ) {
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glCombinerParameterivNV( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glCombinerStageParameterfvNV( GLenum stage, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", stage, pname, params );
  ENTER_GL();
  func_glCombinerStageParameterfvNV( stage, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glCompileShader( GLuint shader ) {
  TRACE("(%d)\n", shader );
  ENTER_GL();
  func_glCompileShader( shader );
  LEAVE_GL();
}

static void WINAPI wine_glCompileShaderARB( unsigned int shaderObj ) {
  TRACE("(%d)\n", shaderObj );
  ENTER_GL();
  func_glCompileShaderARB( shaderObj );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTexImage1D( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, GLvoid* data ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, border, imageSize, data );
  ENTER_GL();
  func_glCompressedTexImage1D( target, level, internalformat, width, border, imageSize, data );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTexImage1DARB( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, GLvoid* data ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, border, imageSize, data );
  ENTER_GL();
  func_glCompressedTexImage1DARB( target, level, internalformat, width, border, imageSize, data );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTexImage2D( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, GLvoid* data ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, height, border, imageSize, data );
  ENTER_GL();
  func_glCompressedTexImage2D( target, level, internalformat, width, height, border, imageSize, data );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTexImage2DARB( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, GLvoid* data ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, height, border, imageSize, data );
  ENTER_GL();
  func_glCompressedTexImage2DARB( target, level, internalformat, width, height, border, imageSize, data );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTexImage3D( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, GLvoid* data ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, height, depth, border, imageSize, data );
  ENTER_GL();
  func_glCompressedTexImage3D( target, level, internalformat, width, height, depth, border, imageSize, data );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTexImage3DARB( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, GLvoid* data ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, height, depth, border, imageSize, data );
  ENTER_GL();
  func_glCompressedTexImage3DARB( target, level, internalformat, width, height, depth, border, imageSize, data );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTexSubImage1D( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, GLvoid* data ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, width, format, imageSize, data );
  ENTER_GL();
  func_glCompressedTexSubImage1D( target, level, xoffset, width, format, imageSize, data );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTexSubImage1DARB( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, GLvoid* data ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, width, format, imageSize, data );
  ENTER_GL();
  func_glCompressedTexSubImage1DARB( target, level, xoffset, width, format, imageSize, data );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTexSubImage2D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, GLvoid* data ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, width, height, format, imageSize, data );
  ENTER_GL();
  func_glCompressedTexSubImage2D( target, level, xoffset, yoffset, width, height, format, imageSize, data );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTexSubImage2DARB( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, GLvoid* data ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, width, height, format, imageSize, data );
  ENTER_GL();
  func_glCompressedTexSubImage2DARB( target, level, xoffset, yoffset, width, height, format, imageSize, data );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTexSubImage3D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, GLvoid* data ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data );
  ENTER_GL();
  func_glCompressedTexSubImage3D( target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTexSubImage3DARB( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, GLvoid* data ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data );
  ENTER_GL();
  func_glCompressedTexSubImage3DARB( target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data );
  LEAVE_GL();
}

static void WINAPI wine_glConvolutionFilter1DEXT( GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, GLvoid* image ) {
  TRACE("(%d, %d, %d, %d, %d, %p)\n", target, internalformat, width, format, type, image );
  ENTER_GL();
  func_glConvolutionFilter1DEXT( target, internalformat, width, format, type, image );
  LEAVE_GL();
}

static void WINAPI wine_glConvolutionFilter2DEXT( GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* image ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %p)\n", target, internalformat, width, height, format, type, image );
  ENTER_GL();
  func_glConvolutionFilter2DEXT( target, internalformat, width, height, format, type, image );
  LEAVE_GL();
}

static void WINAPI wine_glConvolutionParameterfEXT( GLenum target, GLenum pname, GLfloat params ) {
  TRACE("(%d, %d, %f)\n", target, pname, params );
  ENTER_GL();
  func_glConvolutionParameterfEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glConvolutionParameterfvEXT( GLenum target, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glConvolutionParameterfvEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glConvolutionParameteriEXT( GLenum target, GLenum pname, GLint params ) {
  TRACE("(%d, %d, %d)\n", target, pname, params );
  ENTER_GL();
  func_glConvolutionParameteriEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glConvolutionParameterivEXT( GLenum target, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glConvolutionParameterivEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glCopyColorSubTableEXT( GLenum target, GLsizei start, GLint x, GLint y, GLsizei width ) {
  TRACE("(%d, %d, %d, %d, %d)\n", target, start, x, y, width );
  ENTER_GL();
  func_glCopyColorSubTableEXT( target, start, x, y, width );
  LEAVE_GL();
}

static void WINAPI wine_glCopyColorTableSGI( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width ) {
  TRACE("(%d, %d, %d, %d, %d)\n", target, internalformat, x, y, width );
  ENTER_GL();
  func_glCopyColorTableSGI( target, internalformat, x, y, width );
  LEAVE_GL();
}

static void WINAPI wine_glCopyConvolutionFilter1DEXT( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width ) {
  TRACE("(%d, %d, %d, %d, %d)\n", target, internalformat, x, y, width );
  ENTER_GL();
  func_glCopyConvolutionFilter1DEXT( target, internalformat, x, y, width );
  LEAVE_GL();
}

static void WINAPI wine_glCopyConvolutionFilter2DEXT( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height ) {
  TRACE("(%d, %d, %d, %d, %d, %d)\n", target, internalformat, x, y, width, height );
  ENTER_GL();
  func_glCopyConvolutionFilter2DEXT( target, internalformat, x, y, width, height );
  LEAVE_GL();
}

static void WINAPI wine_glCopyTexImage1DEXT( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %d)\n", target, level, internalformat, x, y, width, border );
  ENTER_GL();
  func_glCopyTexImage1DEXT( target, level, internalformat, x, y, width, border );
  LEAVE_GL();
}

static void WINAPI wine_glCopyTexImage2DEXT( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d)\n", target, level, internalformat, x, y, width, height, border );
  ENTER_GL();
  func_glCopyTexImage2DEXT( target, level, internalformat, x, y, width, height, border );
  LEAVE_GL();
}

static void WINAPI wine_glCopyTexSubImage1DEXT( GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width ) {
  TRACE("(%d, %d, %d, %d, %d, %d)\n", target, level, xoffset, x, y, width );
  ENTER_GL();
  func_glCopyTexSubImage1DEXT( target, level, xoffset, x, y, width );
  LEAVE_GL();
}

static void WINAPI wine_glCopyTexSubImage2DEXT( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d)\n", target, level, xoffset, yoffset, x, y, width, height );
  ENTER_GL();
  func_glCopyTexSubImage2DEXT( target, level, xoffset, yoffset, x, y, width, height );
  LEAVE_GL();
}

static void WINAPI wine_glCopyTexSubImage3DEXT( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d)\n", target, level, xoffset, yoffset, zoffset, x, y, width, height );
  ENTER_GL();
  func_glCopyTexSubImage3DEXT( target, level, xoffset, yoffset, zoffset, x, y, width, height );
  LEAVE_GL();
}

static GLuint WINAPI wine_glCreateProgram( void ) {
  GLuint ret_value;
  TRACE("()\n");
  ENTER_GL();
  ret_value = func_glCreateProgram( );
  LEAVE_GL();
  return ret_value;
}

static unsigned int WINAPI wine_glCreateProgramObjectARB( void ) {
  unsigned int ret_value;
  TRACE("()\n");
  ENTER_GL();
  ret_value = func_glCreateProgramObjectARB( );
  LEAVE_GL();
  return ret_value;
}

static GLuint WINAPI wine_glCreateShader( GLenum type ) {
  GLuint ret_value;
  TRACE("(%d)\n", type );
  ENTER_GL();
  ret_value = func_glCreateShader( type );
  LEAVE_GL();
  return ret_value;
}

static unsigned int WINAPI wine_glCreateShaderObjectARB( GLenum shaderType ) {
  unsigned int ret_value;
  TRACE("(%d)\n", shaderType );
  ENTER_GL();
  ret_value = func_glCreateShaderObjectARB( shaderType );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glCullParameterdvEXT( GLenum pname, GLdouble* params ) {
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glCullParameterdvEXT( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glCullParameterfvEXT( GLenum pname, GLfloat* params ) {
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glCullParameterfvEXT( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glCurrentPaletteMatrixARB( GLint index ) {
  TRACE("(%d)\n", index );
  ENTER_GL();
  func_glCurrentPaletteMatrixARB( index );
  LEAVE_GL();
}

static void WINAPI wine_glDeformSGIX( GLint mask ) {
  TRACE("(%d)\n", mask );
  ENTER_GL();
  func_glDeformSGIX( mask );
  LEAVE_GL();
}

static void WINAPI wine_glDeformationMap3dSGIX( GLint target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, GLdouble w1, GLdouble w2, GLint wstride, GLint worder, GLdouble* points ) {
  TRACE("(%d, %f, %f, %d, %d, %f, %f, %d, %d, %f, %f, %d, %d, %p)\n", target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points );
  ENTER_GL();
  func_glDeformationMap3dSGIX( target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points );
  LEAVE_GL();
}

static void WINAPI wine_glDeformationMap3fSGIX( GLint target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, GLfloat w1, GLfloat w2, GLint wstride, GLint worder, GLfloat* points ) {
  TRACE("(%d, %f, %f, %d, %d, %f, %f, %d, %d, %f, %f, %d, %d, %p)\n", target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points );
  ENTER_GL();
  func_glDeformationMap3fSGIX( target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteAsyncMarkersSGIX( GLuint marker, GLsizei range ) {
  TRACE("(%d, %d)\n", marker, range );
  ENTER_GL();
  func_glDeleteAsyncMarkersSGIX( marker, range );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteBufferRegion( GLenum region ) {
  TRACE("(%d)\n", region );
  ENTER_GL();
  func_glDeleteBufferRegion( region );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteBuffers( GLsizei n, GLuint* buffers ) {
  TRACE("(%d, %p)\n", n, buffers );
  ENTER_GL();
  func_glDeleteBuffers( n, buffers );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteBuffersARB( GLsizei n, GLuint* buffers ) {
  TRACE("(%d, %p)\n", n, buffers );
  ENTER_GL();
  func_glDeleteBuffersARB( n, buffers );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteFencesAPPLE( GLsizei n, GLuint* fences ) {
  TRACE("(%d, %p)\n", n, fences );
  ENTER_GL();
  func_glDeleteFencesAPPLE( n, fences );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteFencesNV( GLsizei n, GLuint* fences ) {
  TRACE("(%d, %p)\n", n, fences );
  ENTER_GL();
  func_glDeleteFencesNV( n, fences );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteFragmentShaderATI( GLuint id ) {
  TRACE("(%d)\n", id );
  ENTER_GL();
  func_glDeleteFragmentShaderATI( id );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteFramebuffersEXT( GLsizei n, GLuint* framebuffers ) {
  TRACE("(%d, %p)\n", n, framebuffers );
  ENTER_GL();
  func_glDeleteFramebuffersEXT( n, framebuffers );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteObjectARB( unsigned int obj ) {
  TRACE("(%d)\n", obj );
  ENTER_GL();
  func_glDeleteObjectARB( obj );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteObjectBufferATI( GLuint buffer ) {
  TRACE("(%d)\n", buffer );
  ENTER_GL();
  func_glDeleteObjectBufferATI( buffer );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteOcclusionQueriesNV( GLsizei n, GLuint* ids ) {
  TRACE("(%d, %p)\n", n, ids );
  ENTER_GL();
  func_glDeleteOcclusionQueriesNV( n, ids );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteProgram( GLuint program ) {
  TRACE("(%d)\n", program );
  ENTER_GL();
  func_glDeleteProgram( program );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteProgramsARB( GLsizei n, GLuint* programs ) {
  TRACE("(%d, %p)\n", n, programs );
  ENTER_GL();
  func_glDeleteProgramsARB( n, programs );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteProgramsNV( GLsizei n, GLuint* programs ) {
  TRACE("(%d, %p)\n", n, programs );
  ENTER_GL();
  func_glDeleteProgramsNV( n, programs );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteQueries( GLsizei n, GLuint* ids ) {
  TRACE("(%d, %p)\n", n, ids );
  ENTER_GL();
  func_glDeleteQueries( n, ids );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteQueriesARB( GLsizei n, GLuint* ids ) {
  TRACE("(%d, %p)\n", n, ids );
  ENTER_GL();
  func_glDeleteQueriesARB( n, ids );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteRenderbuffersEXT( GLsizei n, GLuint* renderbuffers ) {
  TRACE("(%d, %p)\n", n, renderbuffers );
  ENTER_GL();
  func_glDeleteRenderbuffersEXT( n, renderbuffers );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteShader( GLuint shader ) {
  TRACE("(%d)\n", shader );
  ENTER_GL();
  func_glDeleteShader( shader );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteTexturesEXT( GLsizei n, GLuint* textures ) {
  TRACE("(%d, %p)\n", n, textures );
  ENTER_GL();
  func_glDeleteTexturesEXT( n, textures );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteVertexArraysAPPLE( GLsizei n, GLuint* arrays ) {
  TRACE("(%d, %p)\n", n, arrays );
  ENTER_GL();
  func_glDeleteVertexArraysAPPLE( n, arrays );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteVertexShaderEXT( GLuint id ) {
  TRACE("(%d)\n", id );
  ENTER_GL();
  func_glDeleteVertexShaderEXT( id );
  LEAVE_GL();
}

static void WINAPI wine_glDepthBoundsEXT( GLclampd zmin, GLclampd zmax ) {
  TRACE("(%f, %f)\n", zmin, zmax );
  ENTER_GL();
  func_glDepthBoundsEXT( zmin, zmax );
  LEAVE_GL();
}

static void WINAPI wine_glDetachObjectARB( unsigned int containerObj, unsigned int attachedObj ) {
  TRACE("(%d, %d)\n", containerObj, attachedObj );
  ENTER_GL();
  func_glDetachObjectARB( containerObj, attachedObj );
  LEAVE_GL();
}

static void WINAPI wine_glDetachShader( GLuint program, GLuint shader ) {
  TRACE("(%d, %d)\n", program, shader );
  ENTER_GL();
  func_glDetachShader( program, shader );
  LEAVE_GL();
}

static void WINAPI wine_glDetailTexFuncSGIS( GLenum target, GLsizei n, GLfloat* points ) {
  TRACE("(%d, %d, %p)\n", target, n, points );
  ENTER_GL();
  func_glDetailTexFuncSGIS( target, n, points );
  LEAVE_GL();
}

static void WINAPI wine_glDisableVariantClientStateEXT( GLuint id ) {
  TRACE("(%d)\n", id );
  ENTER_GL();
  func_glDisableVariantClientStateEXT( id );
  LEAVE_GL();
}

static void WINAPI wine_glDisableVertexAttribArray( GLuint index ) {
  TRACE("(%d)\n", index );
  ENTER_GL();
  func_glDisableVertexAttribArray( index );
  LEAVE_GL();
}

static void WINAPI wine_glDisableVertexAttribArrayARB( GLuint index ) {
  TRACE("(%d)\n", index );
  ENTER_GL();
  func_glDisableVertexAttribArrayARB( index );
  LEAVE_GL();
}

static void WINAPI wine_glDrawArraysEXT( GLenum mode, GLint first, GLsizei count ) {
  TRACE("(%d, %d, %d)\n", mode, first, count );
  ENTER_GL();
  func_glDrawArraysEXT( mode, first, count );
  LEAVE_GL();
}

static void WINAPI wine_glDrawBufferRegion( GLenum region, GLint x, GLint y, GLsizei width, GLsizei height, GLint xDest, GLint yDest ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %d)\n", region, x, y, width, height, xDest, yDest );
  ENTER_GL();
  func_glDrawBufferRegion( region, x, y, width, height, xDest, yDest );
  LEAVE_GL();
}

static void WINAPI wine_glDrawBuffers( GLsizei n, GLenum* bufs ) {
  TRACE("(%d, %p)\n", n, bufs );
  ENTER_GL();
  func_glDrawBuffers( n, bufs );
  LEAVE_GL();
}

static void WINAPI wine_glDrawBuffersARB( GLsizei n, GLenum* bufs ) {
  TRACE("(%d, %p)\n", n, bufs );
  ENTER_GL();
  func_glDrawBuffersARB( n, bufs );
  LEAVE_GL();
}

static void WINAPI wine_glDrawBuffersATI( GLsizei n, GLenum* bufs ) {
  TRACE("(%d, %p)\n", n, bufs );
  ENTER_GL();
  func_glDrawBuffersATI( n, bufs );
  LEAVE_GL();
}

static void WINAPI wine_glDrawElementArrayAPPLE( GLenum mode, GLint first, GLsizei count ) {
  TRACE("(%d, %d, %d)\n", mode, first, count );
  ENTER_GL();
  func_glDrawElementArrayAPPLE( mode, first, count );
  LEAVE_GL();
}

static void WINAPI wine_glDrawElementArrayATI( GLenum mode, GLsizei count ) {
  TRACE("(%d, %d)\n", mode, count );
  ENTER_GL();
  func_glDrawElementArrayATI( mode, count );
  LEAVE_GL();
}

static void WINAPI wine_glDrawMeshArraysSUN( GLenum mode, GLint first, GLsizei count, GLsizei width ) {
  TRACE("(%d, %d, %d, %d)\n", mode, first, count, width );
  ENTER_GL();
  func_glDrawMeshArraysSUN( mode, first, count, width );
  LEAVE_GL();
}

static void WINAPI wine_glDrawRangeElementArrayAPPLE( GLenum mode, GLuint start, GLuint end, GLint first, GLsizei count ) {
  TRACE("(%d, %d, %d, %d, %d)\n", mode, start, end, first, count );
  ENTER_GL();
  func_glDrawRangeElementArrayAPPLE( mode, start, end, first, count );
  LEAVE_GL();
}

static void WINAPI wine_glDrawRangeElementArrayATI( GLenum mode, GLuint start, GLuint end, GLsizei count ) {
  TRACE("(%d, %d, %d, %d)\n", mode, start, end, count );
  ENTER_GL();
  func_glDrawRangeElementArrayATI( mode, start, end, count );
  LEAVE_GL();
}

static void WINAPI wine_glDrawRangeElementsEXT( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, GLvoid* indices ) {
  TRACE("(%d, %d, %d, %d, %d, %p)\n", mode, start, end, count, type, indices );
  ENTER_GL();
  func_glDrawRangeElementsEXT( mode, start, end, count, type, indices );
  LEAVE_GL();
}

static void WINAPI wine_glEdgeFlagPointerEXT( GLsizei stride, GLsizei count, GLboolean* pointer ) {
  TRACE("(%d, %d, %p)\n", stride, count, pointer );
  ENTER_GL();
  func_glEdgeFlagPointerEXT( stride, count, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glEdgeFlagPointerListIBM( GLint stride, GLboolean** pointer, GLint ptrstride ) {
  TRACE("(%d, %p, %d)\n", stride, pointer, ptrstride );
  ENTER_GL();
  func_glEdgeFlagPointerListIBM( stride, pointer, ptrstride );
  LEAVE_GL();
}

static void WINAPI wine_glElementPointerAPPLE( GLenum type, GLvoid* pointer ) {
  TRACE("(%d, %p)\n", type, pointer );
  ENTER_GL();
  func_glElementPointerAPPLE( type, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glElementPointerATI( GLenum type, GLvoid* pointer ) {
  TRACE("(%d, %p)\n", type, pointer );
  ENTER_GL();
  func_glElementPointerATI( type, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glEnableVariantClientStateEXT( GLuint id ) {
  TRACE("(%d)\n", id );
  ENTER_GL();
  func_glEnableVariantClientStateEXT( id );
  LEAVE_GL();
}

static void WINAPI wine_glEnableVertexAttribArray( GLuint index ) {
  TRACE("(%d)\n", index );
  ENTER_GL();
  func_glEnableVertexAttribArray( index );
  LEAVE_GL();
}

static void WINAPI wine_glEnableVertexAttribArrayARB( GLuint index ) {
  TRACE("(%d)\n", index );
  ENTER_GL();
  func_glEnableVertexAttribArrayARB( index );
  LEAVE_GL();
}

static void WINAPI wine_glEndFragmentShaderATI( void ) {
  TRACE("()\n");
  ENTER_GL();
  func_glEndFragmentShaderATI( );
  LEAVE_GL();
}

static void WINAPI wine_glEndOcclusionQueryNV( void ) {
  TRACE("()\n");
  ENTER_GL();
  func_glEndOcclusionQueryNV( );
  LEAVE_GL();
}

static void WINAPI wine_glEndQuery( GLenum target ) {
  TRACE("(%d)\n", target );
  ENTER_GL();
  func_glEndQuery( target );
  LEAVE_GL();
}

static void WINAPI wine_glEndQueryARB( GLenum target ) {
  TRACE("(%d)\n", target );
  ENTER_GL();
  func_glEndQueryARB( target );
  LEAVE_GL();
}

static void WINAPI wine_glEndVertexShaderEXT( void ) {
  TRACE("()\n");
  ENTER_GL();
  func_glEndVertexShaderEXT( );
  LEAVE_GL();
}

static void WINAPI wine_glEvalMapsNV( GLenum target, GLenum mode ) {
  TRACE("(%d, %d)\n", target, mode );
  ENTER_GL();
  func_glEvalMapsNV( target, mode );
  LEAVE_GL();
}

static void WINAPI wine_glExecuteProgramNV( GLenum target, GLuint id, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", target, id, params );
  ENTER_GL();
  func_glExecuteProgramNV( target, id, params );
  LEAVE_GL();
}

static void WINAPI wine_glExtractComponentEXT( GLuint res, GLuint src, GLuint num ) {
  TRACE("(%d, %d, %d)\n", res, src, num );
  ENTER_GL();
  func_glExtractComponentEXT( res, src, num );
  LEAVE_GL();
}

static void WINAPI wine_glFinalCombinerInputNV( GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage ) {
  TRACE("(%d, %d, %d, %d)\n", variable, input, mapping, componentUsage );
  ENTER_GL();
  func_glFinalCombinerInputNV( variable, input, mapping, componentUsage );
  LEAVE_GL();
}

static GLint WINAPI wine_glFinishAsyncSGIX( GLuint* markerp ) {
  GLint ret_value;
  TRACE("(%p)\n", markerp );
  ENTER_GL();
  ret_value = func_glFinishAsyncSGIX( markerp );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glFinishFenceAPPLE( GLuint fence ) {
  TRACE("(%d)\n", fence );
  ENTER_GL();
  func_glFinishFenceAPPLE( fence );
  LEAVE_GL();
}

static void WINAPI wine_glFinishFenceNV( GLuint fence ) {
  TRACE("(%d)\n", fence );
  ENTER_GL();
  func_glFinishFenceNV( fence );
  LEAVE_GL();
}

static void WINAPI wine_glFinishObjectAPPLE( GLenum object, GLint name ) {
  TRACE("(%d, %d)\n", object, name );
  ENTER_GL();
  func_glFinishObjectAPPLE( object, name );
  LEAVE_GL();
}

static void WINAPI wine_glFinishTextureSUNX( void ) {
  TRACE("()\n");
  ENTER_GL();
  func_glFinishTextureSUNX( );
  LEAVE_GL();
}

static void WINAPI wine_glFlushPixelDataRangeNV( GLenum target ) {
  TRACE("(%d)\n", target );
  ENTER_GL();
  func_glFlushPixelDataRangeNV( target );
  LEAVE_GL();
}

static void WINAPI wine_glFlushRasterSGIX( void ) {
  TRACE("()\n");
  ENTER_GL();
  func_glFlushRasterSGIX( );
  LEAVE_GL();
}

static void WINAPI wine_glFlushVertexArrayRangeAPPLE( GLsizei length, GLvoid* pointer ) {
  TRACE("(%d, %p)\n", length, pointer );
  ENTER_GL();
  func_glFlushVertexArrayRangeAPPLE( length, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glFlushVertexArrayRangeNV( void ) {
  TRACE("()\n");
  ENTER_GL();
  func_glFlushVertexArrayRangeNV( );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoordPointer( GLenum type, GLsizei stride, GLvoid* pointer ) {
  TRACE("(%d, %d, %p)\n", type, stride, pointer );
  ENTER_GL();
  func_glFogCoordPointer( type, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoordPointerEXT( GLenum type, GLsizei stride, GLvoid* pointer ) {
  TRACE("(%d, %d, %p)\n", type, stride, pointer );
  ENTER_GL();
  func_glFogCoordPointerEXT( type, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoordPointerListIBM( GLenum type, GLint stride, GLvoid** pointer, GLint ptrstride ) {
  TRACE("(%d, %d, %p, %d)\n", type, stride, pointer, ptrstride );
  ENTER_GL();
  func_glFogCoordPointerListIBM( type, stride, pointer, ptrstride );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoordd( GLdouble coord ) {
  TRACE("(%f)\n", coord );
  ENTER_GL();
  func_glFogCoordd( coord );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoorddEXT( GLdouble coord ) {
  TRACE("(%f)\n", coord );
  ENTER_GL();
  func_glFogCoorddEXT( coord );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoorddv( GLdouble* coord ) {
  TRACE("(%p)\n", coord );
  ENTER_GL();
  func_glFogCoorddv( coord );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoorddvEXT( GLdouble* coord ) {
  TRACE("(%p)\n", coord );
  ENTER_GL();
  func_glFogCoorddvEXT( coord );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoordf( GLfloat coord ) {
  TRACE("(%f)\n", coord );
  ENTER_GL();
  func_glFogCoordf( coord );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoordfEXT( GLfloat coord ) {
  TRACE("(%f)\n", coord );
  ENTER_GL();
  func_glFogCoordfEXT( coord );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoordfv( GLfloat* coord ) {
  TRACE("(%p)\n", coord );
  ENTER_GL();
  func_glFogCoordfv( coord );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoordfvEXT( GLfloat* coord ) {
  TRACE("(%p)\n", coord );
  ENTER_GL();
  func_glFogCoordfvEXT( coord );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoordhNV( unsigned short fog ) {
  TRACE("(%d)\n", fog );
  ENTER_GL();
  func_glFogCoordhNV( fog );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoordhvNV( unsigned short* fog ) {
  TRACE("(%p)\n", fog );
  ENTER_GL();
  func_glFogCoordhvNV( fog );
  LEAVE_GL();
}

static void WINAPI wine_glFogFuncSGIS( GLsizei n, GLfloat* points ) {
  TRACE("(%d, %p)\n", n, points );
  ENTER_GL();
  func_glFogFuncSGIS( n, points );
  LEAVE_GL();
}

static void WINAPI wine_glFragmentColorMaterialSGIX( GLenum face, GLenum mode ) {
  TRACE("(%d, %d)\n", face, mode );
  ENTER_GL();
  func_glFragmentColorMaterialSGIX( face, mode );
  LEAVE_GL();
}

static void WINAPI wine_glFragmentLightModelfSGIX( GLenum pname, GLfloat param ) {
  TRACE("(%d, %f)\n", pname, param );
  ENTER_GL();
  func_glFragmentLightModelfSGIX( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glFragmentLightModelfvSGIX( GLenum pname, GLfloat* params ) {
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glFragmentLightModelfvSGIX( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glFragmentLightModeliSGIX( GLenum pname, GLint param ) {
  TRACE("(%d, %d)\n", pname, param );
  ENTER_GL();
  func_glFragmentLightModeliSGIX( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glFragmentLightModelivSGIX( GLenum pname, GLint* params ) {
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glFragmentLightModelivSGIX( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glFragmentLightfSGIX( GLenum light, GLenum pname, GLfloat param ) {
  TRACE("(%d, %d, %f)\n", light, pname, param );
  ENTER_GL();
  func_glFragmentLightfSGIX( light, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glFragmentLightfvSGIX( GLenum light, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", light, pname, params );
  ENTER_GL();
  func_glFragmentLightfvSGIX( light, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glFragmentLightiSGIX( GLenum light, GLenum pname, GLint param ) {
  TRACE("(%d, %d, %d)\n", light, pname, param );
  ENTER_GL();
  func_glFragmentLightiSGIX( light, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glFragmentLightivSGIX( GLenum light, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", light, pname, params );
  ENTER_GL();
  func_glFragmentLightivSGIX( light, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glFragmentMaterialfSGIX( GLenum face, GLenum pname, GLfloat param ) {
  TRACE("(%d, %d, %f)\n", face, pname, param );
  ENTER_GL();
  func_glFragmentMaterialfSGIX( face, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glFragmentMaterialfvSGIX( GLenum face, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", face, pname, params );
  ENTER_GL();
  func_glFragmentMaterialfvSGIX( face, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glFragmentMaterialiSGIX( GLenum face, GLenum pname, GLint param ) {
  TRACE("(%d, %d, %d)\n", face, pname, param );
  ENTER_GL();
  func_glFragmentMaterialiSGIX( face, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glFragmentMaterialivSGIX( GLenum face, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", face, pname, params );
  ENTER_GL();
  func_glFragmentMaterialivSGIX( face, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glFrameZoomSGIX( GLint factor ) {
  TRACE("(%d)\n", factor );
  ENTER_GL();
  func_glFrameZoomSGIX( factor );
  LEAVE_GL();
}

static void WINAPI wine_glFramebufferRenderbufferEXT( GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer ) {
  TRACE("(%d, %d, %d, %d)\n", target, attachment, renderbuffertarget, renderbuffer );
  ENTER_GL();
  func_glFramebufferRenderbufferEXT( target, attachment, renderbuffertarget, renderbuffer );
  LEAVE_GL();
}

static void WINAPI wine_glFramebufferTexture1DEXT( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level ) {
  TRACE("(%d, %d, %d, %d, %d)\n", target, attachment, textarget, texture, level );
  ENTER_GL();
  func_glFramebufferTexture1DEXT( target, attachment, textarget, texture, level );
  LEAVE_GL();
}

static void WINAPI wine_glFramebufferTexture2DEXT( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level ) {
  TRACE("(%d, %d, %d, %d, %d)\n", target, attachment, textarget, texture, level );
  ENTER_GL();
  func_glFramebufferTexture2DEXT( target, attachment, textarget, texture, level );
  LEAVE_GL();
}

static void WINAPI wine_glFramebufferTexture3DEXT( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset ) {
  TRACE("(%d, %d, %d, %d, %d, %d)\n", target, attachment, textarget, texture, level, zoffset );
  ENTER_GL();
  func_glFramebufferTexture3DEXT( target, attachment, textarget, texture, level, zoffset );
  LEAVE_GL();
}

static void WINAPI wine_glFreeObjectBufferATI( GLuint buffer ) {
  TRACE("(%d)\n", buffer );
  ENTER_GL();
  func_glFreeObjectBufferATI( buffer );
  LEAVE_GL();
}

static GLuint WINAPI wine_glGenAsyncMarkersSGIX( GLsizei range ) {
  GLuint ret_value;
  TRACE("(%d)\n", range );
  ENTER_GL();
  ret_value = func_glGenAsyncMarkersSGIX( range );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glGenBuffers( GLsizei n, GLuint* buffers ) {
  TRACE("(%d, %p)\n", n, buffers );
  ENTER_GL();
  func_glGenBuffers( n, buffers );
  LEAVE_GL();
}

static void WINAPI wine_glGenBuffersARB( GLsizei n, GLuint* buffers ) {
  TRACE("(%d, %p)\n", n, buffers );
  ENTER_GL();
  func_glGenBuffersARB( n, buffers );
  LEAVE_GL();
}

static void WINAPI wine_glGenFencesAPPLE( GLsizei n, GLuint* fences ) {
  TRACE("(%d, %p)\n", n, fences );
  ENTER_GL();
  func_glGenFencesAPPLE( n, fences );
  LEAVE_GL();
}

static void WINAPI wine_glGenFencesNV( GLsizei n, GLuint* fences ) {
  TRACE("(%d, %p)\n", n, fences );
  ENTER_GL();
  func_glGenFencesNV( n, fences );
  LEAVE_GL();
}

static GLuint WINAPI wine_glGenFragmentShadersATI( GLuint range ) {
  GLuint ret_value;
  TRACE("(%d)\n", range );
  ENTER_GL();
  ret_value = func_glGenFragmentShadersATI( range );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glGenFramebuffersEXT( GLsizei n, GLuint* framebuffers ) {
  TRACE("(%d, %p)\n", n, framebuffers );
  ENTER_GL();
  func_glGenFramebuffersEXT( n, framebuffers );
  LEAVE_GL();
}

static void WINAPI wine_glGenOcclusionQueriesNV( GLsizei n, GLuint* ids ) {
  TRACE("(%d, %p)\n", n, ids );
  ENTER_GL();
  func_glGenOcclusionQueriesNV( n, ids );
  LEAVE_GL();
}

static void WINAPI wine_glGenProgramsARB( GLsizei n, GLuint* programs ) {
  TRACE("(%d, %p)\n", n, programs );
  ENTER_GL();
  func_glGenProgramsARB( n, programs );
  LEAVE_GL();
}

static void WINAPI wine_glGenProgramsNV( GLsizei n, GLuint* programs ) {
  TRACE("(%d, %p)\n", n, programs );
  ENTER_GL();
  func_glGenProgramsNV( n, programs );
  LEAVE_GL();
}

static void WINAPI wine_glGenQueries( GLsizei n, GLuint* ids ) {
  TRACE("(%d, %p)\n", n, ids );
  ENTER_GL();
  func_glGenQueries( n, ids );
  LEAVE_GL();
}

static void WINAPI wine_glGenQueriesARB( GLsizei n, GLuint* ids ) {
  TRACE("(%d, %p)\n", n, ids );
  ENTER_GL();
  func_glGenQueriesARB( n, ids );
  LEAVE_GL();
}

static void WINAPI wine_glGenRenderbuffersEXT( GLsizei n, GLuint* renderbuffers ) {
  TRACE("(%d, %p)\n", n, renderbuffers );
  ENTER_GL();
  func_glGenRenderbuffersEXT( n, renderbuffers );
  LEAVE_GL();
}

static GLuint WINAPI wine_glGenSymbolsEXT( GLenum datatype, GLenum storagetype, GLenum range, GLuint components ) {
  GLuint ret_value;
  TRACE("(%d, %d, %d, %d)\n", datatype, storagetype, range, components );
  ENTER_GL();
  ret_value = func_glGenSymbolsEXT( datatype, storagetype, range, components );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glGenTexturesEXT( GLsizei n, GLuint* textures ) {
  TRACE("(%d, %p)\n", n, textures );
  ENTER_GL();
  func_glGenTexturesEXT( n, textures );
  LEAVE_GL();
}

static void WINAPI wine_glGenVertexArraysAPPLE( GLsizei n, GLuint* arrays ) {
  TRACE("(%d, %p)\n", n, arrays );
  ENTER_GL();
  func_glGenVertexArraysAPPLE( n, arrays );
  LEAVE_GL();
}

static GLuint WINAPI wine_glGenVertexShadersEXT( GLuint range ) {
  GLuint ret_value;
  TRACE("(%d)\n", range );
  ENTER_GL();
  ret_value = func_glGenVertexShadersEXT( range );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glGenerateMipmapEXT( GLenum target ) {
  TRACE("(%d)\n", target );
  ENTER_GL();
  func_glGenerateMipmapEXT( target );
  LEAVE_GL();
}

static void WINAPI wine_glGetActiveAttrib( GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, char* name ) {
  TRACE("(%d, %d, %d, %p, %p, %p, %p)\n", program, index, bufSize, length, size, type, name );
  ENTER_GL();
  func_glGetActiveAttrib( program, index, bufSize, length, size, type, name );
  LEAVE_GL();
}

static void WINAPI wine_glGetActiveAttribARB( unsigned int programObj, GLuint index, GLsizei maxLength, GLsizei* length, GLint* size, GLenum* type, char* name ) {
  TRACE("(%d, %d, %d, %p, %p, %p, %p)\n", programObj, index, maxLength, length, size, type, name );
  ENTER_GL();
  func_glGetActiveAttribARB( programObj, index, maxLength, length, size, type, name );
  LEAVE_GL();
}

static void WINAPI wine_glGetActiveUniform( GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, char* name ) {
  TRACE("(%d, %d, %d, %p, %p, %p, %p)\n", program, index, bufSize, length, size, type, name );
  ENTER_GL();
  func_glGetActiveUniform( program, index, bufSize, length, size, type, name );
  LEAVE_GL();
}

static void WINAPI wine_glGetActiveUniformARB( unsigned int programObj, GLuint index, GLsizei maxLength, GLsizei* length, GLint* size, GLenum* type, char* name ) {
  TRACE("(%d, %d, %d, %p, %p, %p, %p)\n", programObj, index, maxLength, length, size, type, name );
  ENTER_GL();
  func_glGetActiveUniformARB( programObj, index, maxLength, length, size, type, name );
  LEAVE_GL();
}

static void WINAPI wine_glGetArrayObjectfvATI( GLenum array, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", array, pname, params );
  ENTER_GL();
  func_glGetArrayObjectfvATI( array, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetArrayObjectivATI( GLenum array, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", array, pname, params );
  ENTER_GL();
  func_glGetArrayObjectivATI( array, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetAttachedObjectsARB( unsigned int containerObj, GLsizei maxCount, GLsizei* count, unsigned int* obj ) {
  TRACE("(%d, %d, %p, %p)\n", containerObj, maxCount, count, obj );
  ENTER_GL();
  func_glGetAttachedObjectsARB( containerObj, maxCount, count, obj );
  LEAVE_GL();
}

static void WINAPI wine_glGetAttachedShaders( GLuint program, GLsizei maxCount, GLsizei* count, GLuint* obj ) {
  TRACE("(%d, %d, %p, %p)\n", program, maxCount, count, obj );
  ENTER_GL();
  func_glGetAttachedShaders( program, maxCount, count, obj );
  LEAVE_GL();
}

static GLint WINAPI wine_glGetAttribLocation( GLuint program, char* name ) {
  GLint ret_value;
  TRACE("(%d, %p)\n", program, name );
  ENTER_GL();
  ret_value = func_glGetAttribLocation( program, name );
  LEAVE_GL();
  return ret_value;
}

static GLint WINAPI wine_glGetAttribLocationARB( unsigned int programObj, char* name ) {
  GLint ret_value;
  TRACE("(%d, %p)\n", programObj, name );
  ENTER_GL();
  ret_value = func_glGetAttribLocationARB( programObj, name );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glGetBufferParameteriv( GLenum target, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetBufferParameteriv( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetBufferParameterivARB( GLenum target, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetBufferParameterivARB( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetBufferPointerv( GLenum target, GLenum pname, GLvoid** params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetBufferPointerv( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetBufferPointervARB( GLenum target, GLenum pname, GLvoid** params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetBufferPointervARB( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetBufferSubData( GLenum target, ptrdiff_t offset, ptrdiff_t size, GLvoid* data ) {
  TRACE("(%d, %d, %d, %p)\n", target, offset, size, data );
  ENTER_GL();
  func_glGetBufferSubData( target, offset, size, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetBufferSubDataARB( GLenum target, ptrdiff_t offset, ptrdiff_t size, GLvoid* data ) {
  TRACE("(%d, %d, %d, %p)\n", target, offset, size, data );
  ENTER_GL();
  func_glGetBufferSubDataARB( target, offset, size, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetColorTableEXT( GLenum target, GLenum format, GLenum type, GLvoid* data ) {
  TRACE("(%d, %d, %d, %p)\n", target, format, type, data );
  ENTER_GL();
  func_glGetColorTableEXT( target, format, type, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetColorTableParameterfvEXT( GLenum target, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetColorTableParameterfvEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetColorTableParameterfvSGI( GLenum target, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetColorTableParameterfvSGI( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetColorTableParameterivEXT( GLenum target, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetColorTableParameterivEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetColorTableParameterivSGI( GLenum target, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetColorTableParameterivSGI( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetColorTableSGI( GLenum target, GLenum format, GLenum type, GLvoid* table ) {
  TRACE("(%d, %d, %d, %p)\n", target, format, type, table );
  ENTER_GL();
  func_glGetColorTableSGI( target, format, type, table );
  LEAVE_GL();
}

static void WINAPI wine_glGetCombinerInputParameterfvNV( GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %d, %d, %p)\n", stage, portion, variable, pname, params );
  ENTER_GL();
  func_glGetCombinerInputParameterfvNV( stage, portion, variable, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetCombinerInputParameterivNV( GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %d, %d, %p)\n", stage, portion, variable, pname, params );
  ENTER_GL();
  func_glGetCombinerInputParameterivNV( stage, portion, variable, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetCombinerOutputParameterfvNV( GLenum stage, GLenum portion, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %d, %p)\n", stage, portion, pname, params );
  ENTER_GL();
  func_glGetCombinerOutputParameterfvNV( stage, portion, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetCombinerOutputParameterivNV( GLenum stage, GLenum portion, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %d, %p)\n", stage, portion, pname, params );
  ENTER_GL();
  func_glGetCombinerOutputParameterivNV( stage, portion, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetCombinerStageParameterfvNV( GLenum stage, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", stage, pname, params );
  ENTER_GL();
  func_glGetCombinerStageParameterfvNV( stage, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetCompressedTexImage( GLenum target, GLint level, GLvoid* img ) {
  TRACE("(%d, %d, %p)\n", target, level, img );
  ENTER_GL();
  func_glGetCompressedTexImage( target, level, img );
  LEAVE_GL();
}

static void WINAPI wine_glGetCompressedTexImageARB( GLenum target, GLint level, GLvoid* img ) {
  TRACE("(%d, %d, %p)\n", target, level, img );
  ENTER_GL();
  func_glGetCompressedTexImageARB( target, level, img );
  LEAVE_GL();
}

static void WINAPI wine_glGetConvolutionFilterEXT( GLenum target, GLenum format, GLenum type, GLvoid* image ) {
  TRACE("(%d, %d, %d, %p)\n", target, format, type, image );
  ENTER_GL();
  func_glGetConvolutionFilterEXT( target, format, type, image );
  LEAVE_GL();
}

static void WINAPI wine_glGetConvolutionParameterfvEXT( GLenum target, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetConvolutionParameterfvEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetConvolutionParameterivEXT( GLenum target, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetConvolutionParameterivEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetDetailTexFuncSGIS( GLenum target, GLfloat* points ) {
  TRACE("(%d, %p)\n", target, points );
  ENTER_GL();
  func_glGetDetailTexFuncSGIS( target, points );
  LEAVE_GL();
}

static void WINAPI wine_glGetFenceivNV( GLuint fence, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", fence, pname, params );
  ENTER_GL();
  func_glGetFenceivNV( fence, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetFinalCombinerInputParameterfvNV( GLenum variable, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", variable, pname, params );
  ENTER_GL();
  func_glGetFinalCombinerInputParameterfvNV( variable, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetFinalCombinerInputParameterivNV( GLenum variable, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", variable, pname, params );
  ENTER_GL();
  func_glGetFinalCombinerInputParameterivNV( variable, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetFogFuncSGIS( GLfloat* points ) {
  TRACE("(%p)\n", points );
  ENTER_GL();
  func_glGetFogFuncSGIS( points );
  LEAVE_GL();
}

static void WINAPI wine_glGetFragmentLightfvSGIX( GLenum light, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", light, pname, params );
  ENTER_GL();
  func_glGetFragmentLightfvSGIX( light, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetFragmentLightivSGIX( GLenum light, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", light, pname, params );
  ENTER_GL();
  func_glGetFragmentLightivSGIX( light, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetFragmentMaterialfvSGIX( GLenum face, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", face, pname, params );
  ENTER_GL();
  func_glGetFragmentMaterialfvSGIX( face, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetFragmentMaterialivSGIX( GLenum face, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", face, pname, params );
  ENTER_GL();
  func_glGetFragmentMaterialivSGIX( face, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetFramebufferAttachmentParameterivEXT( GLenum target, GLenum attachment, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %d, %p)\n", target, attachment, pname, params );
  ENTER_GL();
  func_glGetFramebufferAttachmentParameterivEXT( target, attachment, pname, params );
  LEAVE_GL();
}

static unsigned int WINAPI wine_glGetHandleARB( GLenum pname ) {
  unsigned int ret_value;
  TRACE("(%d)\n", pname );
  ENTER_GL();
  ret_value = func_glGetHandleARB( pname );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glGetHistogramEXT( GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid* values ) {
  TRACE("(%d, %d, %d, %d, %p)\n", target, reset, format, type, values );
  ENTER_GL();
  func_glGetHistogramEXT( target, reset, format, type, values );
  LEAVE_GL();
}

static void WINAPI wine_glGetHistogramParameterfvEXT( GLenum target, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetHistogramParameterfvEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetHistogramParameterivEXT( GLenum target, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetHistogramParameterivEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetImageTransformParameterfvHP( GLenum target, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetImageTransformParameterfvHP( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetImageTransformParameterivHP( GLenum target, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetImageTransformParameterivHP( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetInfoLogARB( unsigned int obj, GLsizei maxLength, GLsizei* length, char* infoLog ) {
  TRACE("(%d, %d, %p, %p)\n", obj, maxLength, length, infoLog );
  ENTER_GL();
  func_glGetInfoLogARB( obj, maxLength, length, infoLog );
  LEAVE_GL();
}

static GLint WINAPI wine_glGetInstrumentsSGIX( void ) {
  GLint ret_value;
  TRACE("()\n");
  ENTER_GL();
  ret_value = func_glGetInstrumentsSGIX( );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glGetInvariantBooleanvEXT( GLuint id, GLenum value, GLboolean* data ) {
  TRACE("(%d, %d, %p)\n", id, value, data );
  ENTER_GL();
  func_glGetInvariantBooleanvEXT( id, value, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetInvariantFloatvEXT( GLuint id, GLenum value, GLfloat* data ) {
  TRACE("(%d, %d, %p)\n", id, value, data );
  ENTER_GL();
  func_glGetInvariantFloatvEXT( id, value, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetInvariantIntegervEXT( GLuint id, GLenum value, GLint* data ) {
  TRACE("(%d, %d, %p)\n", id, value, data );
  ENTER_GL();
  func_glGetInvariantIntegervEXT( id, value, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetListParameterfvSGIX( GLuint list, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", list, pname, params );
  ENTER_GL();
  func_glGetListParameterfvSGIX( list, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetListParameterivSGIX( GLuint list, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", list, pname, params );
  ENTER_GL();
  func_glGetListParameterivSGIX( list, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetLocalConstantBooleanvEXT( GLuint id, GLenum value, GLboolean* data ) {
  TRACE("(%d, %d, %p)\n", id, value, data );
  ENTER_GL();
  func_glGetLocalConstantBooleanvEXT( id, value, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetLocalConstantFloatvEXT( GLuint id, GLenum value, GLfloat* data ) {
  TRACE("(%d, %d, %p)\n", id, value, data );
  ENTER_GL();
  func_glGetLocalConstantFloatvEXT( id, value, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetLocalConstantIntegervEXT( GLuint id, GLenum value, GLint* data ) {
  TRACE("(%d, %d, %p)\n", id, value, data );
  ENTER_GL();
  func_glGetLocalConstantIntegervEXT( id, value, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetMapAttribParameterfvNV( GLenum target, GLuint index, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %d, %p)\n", target, index, pname, params );
  ENTER_GL();
  func_glGetMapAttribParameterfvNV( target, index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetMapAttribParameterivNV( GLenum target, GLuint index, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %d, %p)\n", target, index, pname, params );
  ENTER_GL();
  func_glGetMapAttribParameterivNV( target, index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetMapControlPointsNV( GLenum target, GLuint index, GLenum type, GLsizei ustride, GLsizei vstride, GLboolean packed, GLvoid* points ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %p)\n", target, index, type, ustride, vstride, packed, points );
  ENTER_GL();
  func_glGetMapControlPointsNV( target, index, type, ustride, vstride, packed, points );
  LEAVE_GL();
}

static void WINAPI wine_glGetMapParameterfvNV( GLenum target, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetMapParameterfvNV( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetMapParameterivNV( GLenum target, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetMapParameterivNV( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetMinmaxEXT( GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid* values ) {
  TRACE("(%d, %d, %d, %d, %p)\n", target, reset, format, type, values );
  ENTER_GL();
  func_glGetMinmaxEXT( target, reset, format, type, values );
  LEAVE_GL();
}

static void WINAPI wine_glGetMinmaxParameterfvEXT( GLenum target, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetMinmaxParameterfvEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetMinmaxParameterivEXT( GLenum target, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetMinmaxParameterivEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetObjectBufferfvATI( GLuint buffer, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", buffer, pname, params );
  ENTER_GL();
  func_glGetObjectBufferfvATI( buffer, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetObjectBufferivATI( GLuint buffer, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", buffer, pname, params );
  ENTER_GL();
  func_glGetObjectBufferivATI( buffer, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetObjectParameterfvARB( unsigned int obj, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", obj, pname, params );
  ENTER_GL();
  func_glGetObjectParameterfvARB( obj, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetObjectParameterivARB( unsigned int obj, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", obj, pname, params );
  ENTER_GL();
  func_glGetObjectParameterivARB( obj, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetOcclusionQueryivNV( GLuint id, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", id, pname, params );
  ENTER_GL();
  func_glGetOcclusionQueryivNV( id, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetOcclusionQueryuivNV( GLuint id, GLenum pname, GLuint* params ) {
  TRACE("(%d, %d, %p)\n", id, pname, params );
  ENTER_GL();
  func_glGetOcclusionQueryuivNV( id, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetPixelTexGenParameterfvSGIS( GLenum pname, GLfloat* params ) {
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glGetPixelTexGenParameterfvSGIS( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetPixelTexGenParameterivSGIS( GLenum pname, GLint* params ) {
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glGetPixelTexGenParameterivSGIS( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetPointervEXT( GLenum pname, GLvoid** params ) {
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glGetPointervEXT( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramEnvParameterdvARB( GLenum target, GLuint index, GLdouble* params ) {
  TRACE("(%d, %d, %p)\n", target, index, params );
  ENTER_GL();
  func_glGetProgramEnvParameterdvARB( target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramEnvParameterfvARB( GLenum target, GLuint index, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", target, index, params );
  ENTER_GL();
  func_glGetProgramEnvParameterfvARB( target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramInfoLog( GLuint program, GLsizei bufSize, GLsizei* length, char* infoLog ) {
  TRACE("(%d, %d, %p, %p)\n", program, bufSize, length, infoLog );
  ENTER_GL();
  func_glGetProgramInfoLog( program, bufSize, length, infoLog );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramLocalParameterdvARB( GLenum target, GLuint index, GLdouble* params ) {
  TRACE("(%d, %d, %p)\n", target, index, params );
  ENTER_GL();
  func_glGetProgramLocalParameterdvARB( target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramLocalParameterfvARB( GLenum target, GLuint index, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", target, index, params );
  ENTER_GL();
  func_glGetProgramLocalParameterfvARB( target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramNamedParameterdvNV( GLuint id, GLsizei len, GLubyte* name, GLdouble* params ) {
  TRACE("(%d, %d, %p, %p)\n", id, len, name, params );
  ENTER_GL();
  func_glGetProgramNamedParameterdvNV( id, len, name, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramNamedParameterfvNV( GLuint id, GLsizei len, GLubyte* name, GLfloat* params ) {
  TRACE("(%d, %d, %p, %p)\n", id, len, name, params );
  ENTER_GL();
  func_glGetProgramNamedParameterfvNV( id, len, name, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramParameterdvNV( GLenum target, GLuint index, GLenum pname, GLdouble* params ) {
  TRACE("(%d, %d, %d, %p)\n", target, index, pname, params );
  ENTER_GL();
  func_glGetProgramParameterdvNV( target, index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramParameterfvNV( GLenum target, GLuint index, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %d, %p)\n", target, index, pname, params );
  ENTER_GL();
  func_glGetProgramParameterfvNV( target, index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramStringARB( GLenum target, GLenum pname, GLvoid* string ) {
  TRACE("(%d, %d, %p)\n", target, pname, string );
  ENTER_GL();
  func_glGetProgramStringARB( target, pname, string );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramStringNV( GLuint id, GLenum pname, GLubyte* program ) {
  TRACE("(%d, %d, %p)\n", id, pname, program );
  ENTER_GL();
  func_glGetProgramStringNV( id, pname, program );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramiv( GLuint program, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", program, pname, params );
  ENTER_GL();
  func_glGetProgramiv( program, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramivARB( GLenum target, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetProgramivARB( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramivNV( GLuint id, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", id, pname, params );
  ENTER_GL();
  func_glGetProgramivNV( id, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetQueryObjectiv( GLuint id, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", id, pname, params );
  ENTER_GL();
  func_glGetQueryObjectiv( id, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetQueryObjectivARB( GLuint id, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", id, pname, params );
  ENTER_GL();
  func_glGetQueryObjectivARB( id, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetQueryObjectuiv( GLuint id, GLenum pname, GLuint* params ) {
  TRACE("(%d, %d, %p)\n", id, pname, params );
  ENTER_GL();
  func_glGetQueryObjectuiv( id, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetQueryObjectuivARB( GLuint id, GLenum pname, GLuint* params ) {
  TRACE("(%d, %d, %p)\n", id, pname, params );
  ENTER_GL();
  func_glGetQueryObjectuivARB( id, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetQueryiv( GLenum target, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetQueryiv( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetQueryivARB( GLenum target, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetQueryivARB( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetRenderbufferParameterivEXT( GLenum target, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetRenderbufferParameterivEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetSeparableFilterEXT( GLenum target, GLenum format, GLenum type, GLvoid* row, GLvoid* column, GLvoid* span ) {
  TRACE("(%d, %d, %d, %p, %p, %p)\n", target, format, type, row, column, span );
  ENTER_GL();
  func_glGetSeparableFilterEXT( target, format, type, row, column, span );
  LEAVE_GL();
}

static void WINAPI wine_glGetShaderInfoLog( GLuint shader, GLsizei bufSize, GLsizei* length, char* infoLog ) {
  TRACE("(%d, %d, %p, %p)\n", shader, bufSize, length, infoLog );
  ENTER_GL();
  func_glGetShaderInfoLog( shader, bufSize, length, infoLog );
  LEAVE_GL();
}

static void WINAPI wine_glGetShaderSource( GLuint shader, GLsizei bufSize, GLsizei* length, char* source ) {
  TRACE("(%d, %d, %p, %p)\n", shader, bufSize, length, source );
  ENTER_GL();
  func_glGetShaderSource( shader, bufSize, length, source );
  LEAVE_GL();
}

static void WINAPI wine_glGetShaderSourceARB( unsigned int obj, GLsizei maxLength, GLsizei* length, char* source ) {
  TRACE("(%d, %d, %p, %p)\n", obj, maxLength, length, source );
  ENTER_GL();
  func_glGetShaderSourceARB( obj, maxLength, length, source );
  LEAVE_GL();
}

static void WINAPI wine_glGetShaderiv( GLuint shader, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", shader, pname, params );
  ENTER_GL();
  func_glGetShaderiv( shader, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetSharpenTexFuncSGIS( GLenum target, GLfloat* points ) {
  TRACE("(%d, %p)\n", target, points );
  ENTER_GL();
  func_glGetSharpenTexFuncSGIS( target, points );
  LEAVE_GL();
}

static void WINAPI wine_glGetTexBumpParameterfvATI( GLenum pname, GLfloat* param ) {
  TRACE("(%d, %p)\n", pname, param );
  ENTER_GL();
  func_glGetTexBumpParameterfvATI( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glGetTexBumpParameterivATI( GLenum pname, GLint* param ) {
  TRACE("(%d, %p)\n", pname, param );
  ENTER_GL();
  func_glGetTexBumpParameterivATI( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glGetTexFilterFuncSGIS( GLenum target, GLenum filter, GLfloat* weights ) {
  TRACE("(%d, %d, %p)\n", target, filter, weights );
  ENTER_GL();
  func_glGetTexFilterFuncSGIS( target, filter, weights );
  LEAVE_GL();
}

static void WINAPI wine_glGetTrackMatrixivNV( GLenum target, GLuint address, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %d, %p)\n", target, address, pname, params );
  ENTER_GL();
  func_glGetTrackMatrixivNV( target, address, pname, params );
  LEAVE_GL();
}

static GLint WINAPI wine_glGetUniformLocation( GLuint program, char* name ) {
  GLint ret_value;
  TRACE("(%d, %p)\n", program, name );
  ENTER_GL();
  ret_value = func_glGetUniformLocation( program, name );
  LEAVE_GL();
  return ret_value;
}

static GLint WINAPI wine_glGetUniformLocationARB( unsigned int programObj, char* name ) {
  GLint ret_value;
  TRACE("(%d, %p)\n", programObj, name );
  ENTER_GL();
  ret_value = func_glGetUniformLocationARB( programObj, name );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glGetUniformfv( GLuint program, GLint location, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", program, location, params );
  ENTER_GL();
  func_glGetUniformfv( program, location, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetUniformfvARB( unsigned int programObj, GLint location, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", programObj, location, params );
  ENTER_GL();
  func_glGetUniformfvARB( programObj, location, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetUniformiv( GLuint program, GLint location, GLint* params ) {
  TRACE("(%d, %d, %p)\n", program, location, params );
  ENTER_GL();
  func_glGetUniformiv( program, location, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetUniformivARB( unsigned int programObj, GLint location, GLint* params ) {
  TRACE("(%d, %d, %p)\n", programObj, location, params );
  ENTER_GL();
  func_glGetUniformivARB( programObj, location, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVariantArrayObjectfvATI( GLuint id, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", id, pname, params );
  ENTER_GL();
  func_glGetVariantArrayObjectfvATI( id, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVariantArrayObjectivATI( GLuint id, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", id, pname, params );
  ENTER_GL();
  func_glGetVariantArrayObjectivATI( id, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVariantBooleanvEXT( GLuint id, GLenum value, GLboolean* data ) {
  TRACE("(%d, %d, %p)\n", id, value, data );
  ENTER_GL();
  func_glGetVariantBooleanvEXT( id, value, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetVariantFloatvEXT( GLuint id, GLenum value, GLfloat* data ) {
  TRACE("(%d, %d, %p)\n", id, value, data );
  ENTER_GL();
  func_glGetVariantFloatvEXT( id, value, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetVariantIntegervEXT( GLuint id, GLenum value, GLint* data ) {
  TRACE("(%d, %d, %p)\n", id, value, data );
  ENTER_GL();
  func_glGetVariantIntegervEXT( id, value, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetVariantPointervEXT( GLuint id, GLenum value, GLvoid** data ) {
  TRACE("(%d, %d, %p)\n", id, value, data );
  ENTER_GL();
  func_glGetVariantPointervEXT( id, value, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribArrayObjectfvATI( GLuint index, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribArrayObjectfvATI( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribArrayObjectivATI( GLuint index, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribArrayObjectivATI( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribPointerv( GLuint index, GLenum pname, GLvoid** pointer ) {
  TRACE("(%d, %d, %p)\n", index, pname, pointer );
  ENTER_GL();
  func_glGetVertexAttribPointerv( index, pname, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribPointervARB( GLuint index, GLenum pname, GLvoid** pointer ) {
  TRACE("(%d, %d, %p)\n", index, pname, pointer );
  ENTER_GL();
  func_glGetVertexAttribPointervARB( index, pname, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribPointervNV( GLuint index, GLenum pname, GLvoid** pointer ) {
  TRACE("(%d, %d, %p)\n", index, pname, pointer );
  ENTER_GL();
  func_glGetVertexAttribPointervNV( index, pname, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribdv( GLuint index, GLenum pname, GLdouble* params ) {
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribdv( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribdvARB( GLuint index, GLenum pname, GLdouble* params ) {
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribdvARB( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribdvNV( GLuint index, GLenum pname, GLdouble* params ) {
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribdvNV( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribfv( GLuint index, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribfv( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribfvARB( GLuint index, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribfvARB( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribfvNV( GLuint index, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribfvNV( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribiv( GLuint index, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribiv( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribivARB( GLuint index, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribivARB( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribivNV( GLuint index, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribivNV( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGlobalAlphaFactorbSUN( GLbyte factor ) {
  TRACE("(%d)\n", factor );
  ENTER_GL();
  func_glGlobalAlphaFactorbSUN( factor );
  LEAVE_GL();
}

static void WINAPI wine_glGlobalAlphaFactordSUN( GLdouble factor ) {
  TRACE("(%f)\n", factor );
  ENTER_GL();
  func_glGlobalAlphaFactordSUN( factor );
  LEAVE_GL();
}

static void WINAPI wine_glGlobalAlphaFactorfSUN( GLfloat factor ) {
  TRACE("(%f)\n", factor );
  ENTER_GL();
  func_glGlobalAlphaFactorfSUN( factor );
  LEAVE_GL();
}

static void WINAPI wine_glGlobalAlphaFactoriSUN( GLint factor ) {
  TRACE("(%d)\n", factor );
  ENTER_GL();
  func_glGlobalAlphaFactoriSUN( factor );
  LEAVE_GL();
}

static void WINAPI wine_glGlobalAlphaFactorsSUN( GLshort factor ) {
  TRACE("(%d)\n", factor );
  ENTER_GL();
  func_glGlobalAlphaFactorsSUN( factor );
  LEAVE_GL();
}

static void WINAPI wine_glGlobalAlphaFactorubSUN( GLubyte factor ) {
  TRACE("(%d)\n", factor );
  ENTER_GL();
  func_glGlobalAlphaFactorubSUN( factor );
  LEAVE_GL();
}

static void WINAPI wine_glGlobalAlphaFactoruiSUN( GLuint factor ) {
  TRACE("(%d)\n", factor );
  ENTER_GL();
  func_glGlobalAlphaFactoruiSUN( factor );
  LEAVE_GL();
}

static void WINAPI wine_glGlobalAlphaFactorusSUN( GLushort factor ) {
  TRACE("(%d)\n", factor );
  ENTER_GL();
  func_glGlobalAlphaFactorusSUN( factor );
  LEAVE_GL();
}

static void WINAPI wine_glHintPGI( GLenum target, GLint mode ) {
  TRACE("(%d, %d)\n", target, mode );
  ENTER_GL();
  func_glHintPGI( target, mode );
  LEAVE_GL();
}

static void WINAPI wine_glHistogramEXT( GLenum target, GLsizei width, GLenum internalformat, GLboolean sink ) {
  TRACE("(%d, %d, %d, %d)\n", target, width, internalformat, sink );
  ENTER_GL();
  func_glHistogramEXT( target, width, internalformat, sink );
  LEAVE_GL();
}

static void WINAPI wine_glIglooInterfaceSGIX( GLint pname, GLint* params ) {
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glIglooInterfaceSGIX( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glImageTransformParameterfHP( GLenum target, GLenum pname, GLfloat param ) {
  TRACE("(%d, %d, %f)\n", target, pname, param );
  ENTER_GL();
  func_glImageTransformParameterfHP( target, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glImageTransformParameterfvHP( GLenum target, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glImageTransformParameterfvHP( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glImageTransformParameteriHP( GLenum target, GLenum pname, GLint param ) {
  TRACE("(%d, %d, %d)\n", target, pname, param );
  ENTER_GL();
  func_glImageTransformParameteriHP( target, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glImageTransformParameterivHP( GLenum target, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glImageTransformParameterivHP( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glIndexFuncEXT( GLenum func, GLclampf ref ) {
  TRACE("(%d, %f)\n", func, ref );
  ENTER_GL();
  func_glIndexFuncEXT( func, ref );
  LEAVE_GL();
}

static void WINAPI wine_glIndexMaterialEXT( GLenum face, GLenum mode ) {
  TRACE("(%d, %d)\n", face, mode );
  ENTER_GL();
  func_glIndexMaterialEXT( face, mode );
  LEAVE_GL();
}

static void WINAPI wine_glIndexPointerEXT( GLenum type, GLsizei stride, GLsizei count, GLvoid* pointer ) {
  TRACE("(%d, %d, %d, %p)\n", type, stride, count, pointer );
  ENTER_GL();
  func_glIndexPointerEXT( type, stride, count, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glIndexPointerListIBM( GLenum type, GLint stride, GLvoid** pointer, GLint ptrstride ) {
  TRACE("(%d, %d, %p, %d)\n", type, stride, pointer, ptrstride );
  ENTER_GL();
  func_glIndexPointerListIBM( type, stride, pointer, ptrstride );
  LEAVE_GL();
}

static void WINAPI wine_glInsertComponentEXT( GLuint res, GLuint src, GLuint num ) {
  TRACE("(%d, %d, %d)\n", res, src, num );
  ENTER_GL();
  func_glInsertComponentEXT( res, src, num );
  LEAVE_GL();
}

static void WINAPI wine_glInstrumentsBufferSGIX( GLsizei size, GLint* buffer ) {
  TRACE("(%d, %p)\n", size, buffer );
  ENTER_GL();
  func_glInstrumentsBufferSGIX( size, buffer );
  LEAVE_GL();
}

static GLboolean WINAPI wine_glIsAsyncMarkerSGIX( GLuint marker ) {
  GLboolean ret_value;
  TRACE("(%d)\n", marker );
  ENTER_GL();
  ret_value = func_glIsAsyncMarkerSGIX( marker );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsBuffer( GLuint buffer ) {
  GLboolean ret_value;
  TRACE("(%d)\n", buffer );
  ENTER_GL();
  ret_value = func_glIsBuffer( buffer );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsBufferARB( GLuint buffer ) {
  GLboolean ret_value;
  TRACE("(%d)\n", buffer );
  ENTER_GL();
  ret_value = func_glIsBufferARB( buffer );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsFenceAPPLE( GLuint fence ) {
  GLboolean ret_value;
  TRACE("(%d)\n", fence );
  ENTER_GL();
  ret_value = func_glIsFenceAPPLE( fence );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsFenceNV( GLuint fence ) {
  GLboolean ret_value;
  TRACE("(%d)\n", fence );
  ENTER_GL();
  ret_value = func_glIsFenceNV( fence );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsFramebufferEXT( GLuint framebuffer ) {
  GLboolean ret_value;
  TRACE("(%d)\n", framebuffer );
  ENTER_GL();
  ret_value = func_glIsFramebufferEXT( framebuffer );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsObjectBufferATI( GLuint buffer ) {
  GLboolean ret_value;
  TRACE("(%d)\n", buffer );
  ENTER_GL();
  ret_value = func_glIsObjectBufferATI( buffer );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsOcclusionQueryNV( GLuint id ) {
  GLboolean ret_value;
  TRACE("(%d)\n", id );
  ENTER_GL();
  ret_value = func_glIsOcclusionQueryNV( id );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsProgram( GLuint program ) {
  GLboolean ret_value;
  TRACE("(%d)\n", program );
  ENTER_GL();
  ret_value = func_glIsProgram( program );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsProgramARB( GLuint program ) {
  GLboolean ret_value;
  TRACE("(%d)\n", program );
  ENTER_GL();
  ret_value = func_glIsProgramARB( program );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsProgramNV( GLuint id ) {
  GLboolean ret_value;
  TRACE("(%d)\n", id );
  ENTER_GL();
  ret_value = func_glIsProgramNV( id );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsQuery( GLuint id ) {
  GLboolean ret_value;
  TRACE("(%d)\n", id );
  ENTER_GL();
  ret_value = func_glIsQuery( id );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsQueryARB( GLuint id ) {
  GLboolean ret_value;
  TRACE("(%d)\n", id );
  ENTER_GL();
  ret_value = func_glIsQueryARB( id );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsRenderbufferEXT( GLuint renderbuffer ) {
  GLboolean ret_value;
  TRACE("(%d)\n", renderbuffer );
  ENTER_GL();
  ret_value = func_glIsRenderbufferEXT( renderbuffer );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsShader( GLuint shader ) {
  GLboolean ret_value;
  TRACE("(%d)\n", shader );
  ENTER_GL();
  ret_value = func_glIsShader( shader );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsTextureEXT( GLuint texture ) {
  GLboolean ret_value;
  TRACE("(%d)\n", texture );
  ENTER_GL();
  ret_value = func_glIsTextureEXT( texture );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsVariantEnabledEXT( GLuint id, GLenum cap ) {
  GLboolean ret_value;
  TRACE("(%d, %d)\n", id, cap );
  ENTER_GL();
  ret_value = func_glIsVariantEnabledEXT( id, cap );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsVertexArrayAPPLE( GLuint array ) {
  GLboolean ret_value;
  TRACE("(%d)\n", array );
  ENTER_GL();
  ret_value = func_glIsVertexArrayAPPLE( array );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glLightEnviSGIX( GLenum pname, GLint param ) {
  TRACE("(%d, %d)\n", pname, param );
  ENTER_GL();
  func_glLightEnviSGIX( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glLinkProgram( GLuint program ) {
  TRACE("(%d)\n", program );
  ENTER_GL();
  func_glLinkProgram( program );
  LEAVE_GL();
}

static void WINAPI wine_glLinkProgramARB( unsigned int programObj ) {
  TRACE("(%d)\n", programObj );
  ENTER_GL();
  func_glLinkProgramARB( programObj );
  LEAVE_GL();
}

static void WINAPI wine_glListParameterfSGIX( GLuint list, GLenum pname, GLfloat param ) {
  TRACE("(%d, %d, %f)\n", list, pname, param );
  ENTER_GL();
  func_glListParameterfSGIX( list, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glListParameterfvSGIX( GLuint list, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", list, pname, params );
  ENTER_GL();
  func_glListParameterfvSGIX( list, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glListParameteriSGIX( GLuint list, GLenum pname, GLint param ) {
  TRACE("(%d, %d, %d)\n", list, pname, param );
  ENTER_GL();
  func_glListParameteriSGIX( list, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glListParameterivSGIX( GLuint list, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", list, pname, params );
  ENTER_GL();
  func_glListParameterivSGIX( list, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glLoadIdentityDeformationMapSGIX( GLint mask ) {
  TRACE("(%d)\n", mask );
  ENTER_GL();
  func_glLoadIdentityDeformationMapSGIX( mask );
  LEAVE_GL();
}

static void WINAPI wine_glLoadProgramNV( GLenum target, GLuint id, GLsizei len, GLubyte* program ) {
  TRACE("(%d, %d, %d, %p)\n", target, id, len, program );
  ENTER_GL();
  func_glLoadProgramNV( target, id, len, program );
  LEAVE_GL();
}

static void WINAPI wine_glLoadTransposeMatrixd( GLdouble* m ) {
  TRACE("(%p)\n", m );
  ENTER_GL();
  func_glLoadTransposeMatrixd( m );
  LEAVE_GL();
}

static void WINAPI wine_glLoadTransposeMatrixdARB( GLdouble* m ) {
  TRACE("(%p)\n", m );
  ENTER_GL();
  func_glLoadTransposeMatrixdARB( m );
  LEAVE_GL();
}

static void WINAPI wine_glLoadTransposeMatrixf( GLfloat* m ) {
  TRACE("(%p)\n", m );
  ENTER_GL();
  func_glLoadTransposeMatrixf( m );
  LEAVE_GL();
}

static void WINAPI wine_glLoadTransposeMatrixfARB( GLfloat* m ) {
  TRACE("(%p)\n", m );
  ENTER_GL();
  func_glLoadTransposeMatrixfARB( m );
  LEAVE_GL();
}

static void WINAPI wine_glLockArraysEXT( GLint first, GLsizei count ) {
  TRACE("(%d, %d)\n", first, count );
  ENTER_GL();
  func_glLockArraysEXT( first, count );
  LEAVE_GL();
}

static void WINAPI wine_glMTexCoord2fSGIS( GLenum target, GLfloat s, GLfloat t ) {
  TRACE("(%d, %f, %f)\n", target, s, t );
  ENTER_GL();
  func_glMTexCoord2fSGIS( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMTexCoord2fvSGIS( GLenum target, GLfloat * v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMTexCoord2fvSGIS( target, v );
  LEAVE_GL();
}

static GLvoid* WINAPI wine_glMapBuffer( GLenum target, GLenum access ) {
  GLvoid* ret_value;
  TRACE("(%d, %d)\n", target, access );
  ENTER_GL();
  ret_value = func_glMapBuffer( target, access );
  LEAVE_GL();
  return ret_value;
}

static GLvoid* WINAPI wine_glMapBufferARB( GLenum target, GLenum access ) {
  GLvoid* ret_value;
  TRACE("(%d, %d)\n", target, access );
  ENTER_GL();
  ret_value = func_glMapBufferARB( target, access );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glMapControlPointsNV( GLenum target, GLuint index, GLenum type, GLsizei ustride, GLsizei vstride, GLint uorder, GLint vorder, GLboolean packed, GLvoid* points ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, index, type, ustride, vstride, uorder, vorder, packed, points );
  ENTER_GL();
  func_glMapControlPointsNV( target, index, type, ustride, vstride, uorder, vorder, packed, points );
  LEAVE_GL();
}

static GLvoid* WINAPI wine_glMapObjectBufferATI( GLuint buffer ) {
  GLvoid* ret_value;
  TRACE("(%d)\n", buffer );
  ENTER_GL();
  ret_value = func_glMapObjectBufferATI( buffer );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glMapParameterfvNV( GLenum target, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glMapParameterfvNV( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glMapParameterivNV( GLenum target, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glMapParameterivNV( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glMatrixIndexPointerARB( GLint size, GLenum type, GLsizei stride, GLvoid* pointer ) {
  TRACE("(%d, %d, %d, %p)\n", size, type, stride, pointer );
  ENTER_GL();
  func_glMatrixIndexPointerARB( size, type, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glMatrixIndexubvARB( GLint size, GLubyte* indices ) {
  TRACE("(%d, %p)\n", size, indices );
  ENTER_GL();
  func_glMatrixIndexubvARB( size, indices );
  LEAVE_GL();
}

static void WINAPI wine_glMatrixIndexuivARB( GLint size, GLuint* indices ) {
  TRACE("(%d, %p)\n", size, indices );
  ENTER_GL();
  func_glMatrixIndexuivARB( size, indices );
  LEAVE_GL();
}

static void WINAPI wine_glMatrixIndexusvARB( GLint size, GLushort* indices ) {
  TRACE("(%d, %p)\n", size, indices );
  ENTER_GL();
  func_glMatrixIndexusvARB( size, indices );
  LEAVE_GL();
}

static void WINAPI wine_glMinmaxEXT( GLenum target, GLenum internalformat, GLboolean sink ) {
  TRACE("(%d, %d, %d)\n", target, internalformat, sink );
  ENTER_GL();
  func_glMinmaxEXT( target, internalformat, sink );
  LEAVE_GL();
}

static void WINAPI wine_glMultTransposeMatrixd( GLdouble* m ) {
  TRACE("(%p)\n", m );
  ENTER_GL();
  func_glMultTransposeMatrixd( m );
  LEAVE_GL();
}

static void WINAPI wine_glMultTransposeMatrixdARB( GLdouble* m ) {
  TRACE("(%p)\n", m );
  ENTER_GL();
  func_glMultTransposeMatrixdARB( m );
  LEAVE_GL();
}

static void WINAPI wine_glMultTransposeMatrixf( GLfloat* m ) {
  TRACE("(%p)\n", m );
  ENTER_GL();
  func_glMultTransposeMatrixf( m );
  LEAVE_GL();
}

static void WINAPI wine_glMultTransposeMatrixfARB( GLfloat* m ) {
  TRACE("(%p)\n", m );
  ENTER_GL();
  func_glMultTransposeMatrixfARB( m );
  LEAVE_GL();
}

static void WINAPI wine_glMultiDrawArrays( GLenum mode, GLint* first, GLsizei* count, GLsizei primcount ) {
  TRACE("(%d, %p, %p, %d)\n", mode, first, count, primcount );
  ENTER_GL();
  func_glMultiDrawArrays( mode, first, count, primcount );
  LEAVE_GL();
}

static void WINAPI wine_glMultiDrawArraysEXT( GLenum mode, GLint* first, GLsizei* count, GLsizei primcount ) {
  TRACE("(%d, %p, %p, %d)\n", mode, first, count, primcount );
  ENTER_GL();
  func_glMultiDrawArraysEXT( mode, first, count, primcount );
  LEAVE_GL();
}

static void WINAPI wine_glMultiDrawElementArrayAPPLE( GLenum mode, GLint* first, GLsizei* count, GLsizei primcount ) {
  TRACE("(%d, %p, %p, %d)\n", mode, first, count, primcount );
  ENTER_GL();
  func_glMultiDrawElementArrayAPPLE( mode, first, count, primcount );
  LEAVE_GL();
}

static void WINAPI wine_glMultiDrawElements( GLenum mode, GLsizei* count, GLenum type, GLvoid** indices, GLsizei primcount ) {
  TRACE("(%d, %p, %d, %p, %d)\n", mode, count, type, indices, primcount );
  ENTER_GL();
  func_glMultiDrawElements( mode, count, type, indices, primcount );
  LEAVE_GL();
}

static void WINAPI wine_glMultiDrawElementsEXT( GLenum mode, GLsizei* count, GLenum type, GLvoid** indices, GLsizei primcount ) {
  TRACE("(%d, %p, %d, %p, %d)\n", mode, count, type, indices, primcount );
  ENTER_GL();
  func_glMultiDrawElementsEXT( mode, count, type, indices, primcount );
  LEAVE_GL();
}

static void WINAPI wine_glMultiDrawRangeElementArrayAPPLE( GLenum mode, GLuint start, GLuint end, GLint* first, GLsizei* count, GLsizei primcount ) {
  TRACE("(%d, %d, %d, %p, %p, %d)\n", mode, start, end, first, count, primcount );
  ENTER_GL();
  func_glMultiDrawRangeElementArrayAPPLE( mode, start, end, first, count, primcount );
  LEAVE_GL();
}

static void WINAPI wine_glMultiModeDrawArraysIBM( GLenum* mode, GLint* first, GLsizei* count, GLsizei primcount, GLint modestride ) {
  TRACE("(%p, %p, %p, %d, %d)\n", mode, first, count, primcount, modestride );
  ENTER_GL();
  func_glMultiModeDrawArraysIBM( mode, first, count, primcount, modestride );
  LEAVE_GL();
}

static void WINAPI wine_glMultiModeDrawElementsIBM( GLenum* mode, GLsizei* count, GLenum type, GLvoid* const* indices, GLsizei primcount, GLint modestride ) {
  TRACE("(%p, %p, %d, %p, %d, %d)\n", mode, count, type, indices, primcount, modestride );
  ENTER_GL();
  func_glMultiModeDrawElementsIBM( mode, count, type, indices, primcount, modestride );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1d( GLenum target, GLdouble s ) {
  TRACE("(%d, %f)\n", target, s );
  ENTER_GL();
  func_glMultiTexCoord1d( target, s );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1dARB( GLenum target, GLdouble s ) {
  TRACE("(%d, %f)\n", target, s );
  ENTER_GL();
  func_glMultiTexCoord1dARB( target, s );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1dSGIS( GLenum target, GLdouble s ) {
  TRACE("(%d, %f)\n", target, s );
  ENTER_GL();
  func_glMultiTexCoord1dSGIS( target, s );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1dv( GLenum target, GLdouble* v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord1dv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1dvARB( GLenum target, GLdouble* v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord1dvARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1dvSGIS( GLenum target, GLdouble * v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord1dvSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1f( GLenum target, GLfloat s ) {
  TRACE("(%d, %f)\n", target, s );
  ENTER_GL();
  func_glMultiTexCoord1f( target, s );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1fARB( GLenum target, GLfloat s ) {
  TRACE("(%d, %f)\n", target, s );
  ENTER_GL();
  func_glMultiTexCoord1fARB( target, s );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1fSGIS( GLenum target, GLfloat s ) {
  TRACE("(%d, %f)\n", target, s );
  ENTER_GL();
  func_glMultiTexCoord1fSGIS( target, s );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1fv( GLenum target, GLfloat* v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord1fv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1fvARB( GLenum target, GLfloat* v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord1fvARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1fvSGIS( GLenum target, const GLfloat * v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord1fvSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1hNV( GLenum target, unsigned short s ) {
  TRACE("(%d, %d)\n", target, s );
  ENTER_GL();
  func_glMultiTexCoord1hNV( target, s );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1hvNV( GLenum target, unsigned short* v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord1hvNV( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1i( GLenum target, GLint s ) {
  TRACE("(%d, %d)\n", target, s );
  ENTER_GL();
  func_glMultiTexCoord1i( target, s );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1iARB( GLenum target, GLint s ) {
  TRACE("(%d, %d)\n", target, s );
  ENTER_GL();
  func_glMultiTexCoord1iARB( target, s );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1iSGIS( GLenum target, GLint s ) {
  TRACE("(%d, %d)\n", target, s );
  ENTER_GL();
  func_glMultiTexCoord1iSGIS( target, s );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1iv( GLenum target, GLint* v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord1iv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1ivARB( GLenum target, GLint* v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord1ivARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1ivSGIS( GLenum target, GLint * v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord1ivSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1s( GLenum target, GLshort s ) {
  TRACE("(%d, %d)\n", target, s );
  ENTER_GL();
  func_glMultiTexCoord1s( target, s );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1sARB( GLenum target, GLshort s ) {
  TRACE("(%d, %d)\n", target, s );
  ENTER_GL();
  func_glMultiTexCoord1sARB( target, s );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1sSGIS( GLenum target, GLshort s ) {
  TRACE("(%d, %d)\n", target, s );
  ENTER_GL();
  func_glMultiTexCoord1sSGIS( target, s );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1sv( GLenum target, GLshort* v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord1sv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1svARB( GLenum target, GLshort* v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord1svARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1svSGIS( GLenum target, GLshort * v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord1svSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2d( GLenum target, GLdouble s, GLdouble t ) {
  TRACE("(%d, %f, %f)\n", target, s, t );
  ENTER_GL();
  func_glMultiTexCoord2d( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2dARB( GLenum target, GLdouble s, GLdouble t ) {
  TRACE("(%d, %f, %f)\n", target, s, t );
  ENTER_GL();
  func_glMultiTexCoord2dARB( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2dSGIS( GLenum target, GLdouble s, GLdouble t ) {
  TRACE("(%d, %f, %f)\n", target, s, t );
  ENTER_GL();
  func_glMultiTexCoord2dSGIS( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2dv( GLenum target, GLdouble* v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord2dv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2dvARB( GLenum target, GLdouble* v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord2dvARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2dvSGIS( GLenum target, GLdouble * v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord2dvSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2f( GLenum target, GLfloat s, GLfloat t ) {
  TRACE("(%d, %f, %f)\n", target, s, t );
  ENTER_GL();
  func_glMultiTexCoord2f( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2fARB( GLenum target, GLfloat s, GLfloat t ) {
  TRACE("(%d, %f, %f)\n", target, s, t );
  ENTER_GL();
  func_glMultiTexCoord2fARB( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2fSGIS( GLenum target, GLfloat s, GLfloat t ) {
  TRACE("(%d, %f, %f)\n", target, s, t );
  ENTER_GL();
  func_glMultiTexCoord2fSGIS( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2fv( GLenum target, GLfloat* v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord2fv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2fvARB( GLenum target, GLfloat* v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord2fvARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2fvSGIS( GLenum target, GLfloat * v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord2fvSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2hNV( GLenum target, unsigned short s, unsigned short t ) {
  TRACE("(%d, %d, %d)\n", target, s, t );
  ENTER_GL();
  func_glMultiTexCoord2hNV( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2hvNV( GLenum target, unsigned short* v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord2hvNV( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2i( GLenum target, GLint s, GLint t ) {
  TRACE("(%d, %d, %d)\n", target, s, t );
  ENTER_GL();
  func_glMultiTexCoord2i( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2iARB( GLenum target, GLint s, GLint t ) {
  TRACE("(%d, %d, %d)\n", target, s, t );
  ENTER_GL();
  func_glMultiTexCoord2iARB( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2iSGIS( GLenum target, GLint s, GLint t ) {
  TRACE("(%d, %d, %d)\n", target, s, t );
  ENTER_GL();
  func_glMultiTexCoord2iSGIS( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2iv( GLenum target, GLint* v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord2iv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2ivARB( GLenum target, GLint* v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord2ivARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2ivSGIS( GLenum target, GLint * v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord2ivSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2s( GLenum target, GLshort s, GLshort t ) {
  TRACE("(%d, %d, %d)\n", target, s, t );
  ENTER_GL();
  func_glMultiTexCoord2s( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2sARB( GLenum target, GLshort s, GLshort t ) {
  TRACE("(%d, %d, %d)\n", target, s, t );
  ENTER_GL();
  func_glMultiTexCoord2sARB( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2sSGIS( GLenum target, GLshort s, GLshort t ) {
  TRACE("(%d, %d, %d)\n", target, s, t );
  ENTER_GL();
  func_glMultiTexCoord2sSGIS( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2sv( GLenum target, GLshort* v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord2sv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2svARB( GLenum target, GLshort* v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord2svARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2svSGIS( GLenum target, GLshort * v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord2svSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3d( GLenum target, GLdouble s, GLdouble t, GLdouble r ) {
  TRACE("(%d, %f, %f, %f)\n", target, s, t, r );
  ENTER_GL();
  func_glMultiTexCoord3d( target, s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3dARB( GLenum target, GLdouble s, GLdouble t, GLdouble r ) {
  TRACE("(%d, %f, %f, %f)\n", target, s, t, r );
  ENTER_GL();
  func_glMultiTexCoord3dARB( target, s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3dSGIS( GLenum target, GLdouble s, GLdouble t, GLdouble r ) {
  TRACE("(%d, %f, %f, %f)\n", target, s, t, r );
  ENTER_GL();
  func_glMultiTexCoord3dSGIS( target, s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3dv( GLenum target, GLdouble* v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord3dv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3dvARB( GLenum target, GLdouble* v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord3dvARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3dvSGIS( GLenum target, GLdouble * v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord3dvSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3f( GLenum target, GLfloat s, GLfloat t, GLfloat r ) {
  TRACE("(%d, %f, %f, %f)\n", target, s, t, r );
  ENTER_GL();
  func_glMultiTexCoord3f( target, s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3fARB( GLenum target, GLfloat s, GLfloat t, GLfloat r ) {
  TRACE("(%d, %f, %f, %f)\n", target, s, t, r );
  ENTER_GL();
  func_glMultiTexCoord3fARB( target, s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3fSGIS( GLenum target, GLfloat s, GLfloat t, GLfloat r ) {
  TRACE("(%d, %f, %f, %f)\n", target, s, t, r );
  ENTER_GL();
  func_glMultiTexCoord3fSGIS( target, s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3fv( GLenum target, GLfloat* v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord3fv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3fvARB( GLenum target, GLfloat* v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord3fvARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3fvSGIS( GLenum target, GLfloat * v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord3fvSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3hNV( GLenum target, unsigned short s, unsigned short t, unsigned short r ) {
  TRACE("(%d, %d, %d, %d)\n", target, s, t, r );
  ENTER_GL();
  func_glMultiTexCoord3hNV( target, s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3hvNV( GLenum target, unsigned short* v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord3hvNV( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3i( GLenum target, GLint s, GLint t, GLint r ) {
  TRACE("(%d, %d, %d, %d)\n", target, s, t, r );
  ENTER_GL();
  func_glMultiTexCoord3i( target, s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3iARB( GLenum target, GLint s, GLint t, GLint r ) {
  TRACE("(%d, %d, %d, %d)\n", target, s, t, r );
  ENTER_GL();
  func_glMultiTexCoord3iARB( target, s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3iSGIS( GLenum target, GLint s, GLint t, GLint r ) {
  TRACE("(%d, %d, %d, %d)\n", target, s, t, r );
  ENTER_GL();
  func_glMultiTexCoord3iSGIS( target, s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3iv( GLenum target, GLint* v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord3iv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3ivARB( GLenum target, GLint* v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord3ivARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3ivSGIS( GLenum target, GLint * v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord3ivSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3s( GLenum target, GLshort s, GLshort t, GLshort r ) {
  TRACE("(%d, %d, %d, %d)\n", target, s, t, r );
  ENTER_GL();
  func_glMultiTexCoord3s( target, s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3sARB( GLenum target, GLshort s, GLshort t, GLshort r ) {
  TRACE("(%d, %d, %d, %d)\n", target, s, t, r );
  ENTER_GL();
  func_glMultiTexCoord3sARB( target, s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3sSGIS( GLenum target, GLshort s, GLshort t, GLshort r ) {
  TRACE("(%d, %d, %d, %d)\n", target, s, t, r );
  ENTER_GL();
  func_glMultiTexCoord3sSGIS( target, s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3sv( GLenum target, GLshort* v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord3sv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3svARB( GLenum target, GLshort* v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord3svARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3svSGIS( GLenum target, GLshort * v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord3svSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4d( GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q ) {
  TRACE("(%d, %f, %f, %f, %f)\n", target, s, t, r, q );
  ENTER_GL();
  func_glMultiTexCoord4d( target, s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4dARB( GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q ) {
  TRACE("(%d, %f, %f, %f, %f)\n", target, s, t, r, q );
  ENTER_GL();
  func_glMultiTexCoord4dARB( target, s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4dSGIS( GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q ) {
  TRACE("(%d, %f, %f, %f, %f)\n", target, s, t, r, q );
  ENTER_GL();
  func_glMultiTexCoord4dSGIS( target, s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4dv( GLenum target, GLdouble* v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord4dv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4dvARB( GLenum target, GLdouble* v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord4dvARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4dvSGIS( GLenum target, GLdouble * v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord4dvSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4f( GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q ) {
  TRACE("(%d, %f, %f, %f, %f)\n", target, s, t, r, q );
  ENTER_GL();
  func_glMultiTexCoord4f( target, s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4fARB( GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q ) {
  TRACE("(%d, %f, %f, %f, %f)\n", target, s, t, r, q );
  ENTER_GL();
  func_glMultiTexCoord4fARB( target, s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4fSGIS( GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q ) {
  TRACE("(%d, %f, %f, %f, %f)\n", target, s, t, r, q );
  ENTER_GL();
  func_glMultiTexCoord4fSGIS( target, s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4fv( GLenum target, GLfloat* v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord4fv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4fvARB( GLenum target, GLfloat* v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord4fvARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4fvSGIS( GLenum target, GLfloat * v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord4fvSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4hNV( GLenum target, unsigned short s, unsigned short t, unsigned short r, unsigned short q ) {
  TRACE("(%d, %d, %d, %d, %d)\n", target, s, t, r, q );
  ENTER_GL();
  func_glMultiTexCoord4hNV( target, s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4hvNV( GLenum target, unsigned short* v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord4hvNV( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4i( GLenum target, GLint s, GLint t, GLint r, GLint q ) {
  TRACE("(%d, %d, %d, %d, %d)\n", target, s, t, r, q );
  ENTER_GL();
  func_glMultiTexCoord4i( target, s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4iARB( GLenum target, GLint s, GLint t, GLint r, GLint q ) {
  TRACE("(%d, %d, %d, %d, %d)\n", target, s, t, r, q );
  ENTER_GL();
  func_glMultiTexCoord4iARB( target, s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4iSGIS( GLenum target, GLint s, GLint t, GLint r, GLint q ) {
  TRACE("(%d, %d, %d, %d, %d)\n", target, s, t, r, q );
  ENTER_GL();
  func_glMultiTexCoord4iSGIS( target, s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4iv( GLenum target, GLint* v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord4iv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4ivARB( GLenum target, GLint* v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord4ivARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4ivSGIS( GLenum target, GLint * v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord4ivSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4s( GLenum target, GLshort s, GLshort t, GLshort r, GLshort q ) {
  TRACE("(%d, %d, %d, %d, %d)\n", target, s, t, r, q );
  ENTER_GL();
  func_glMultiTexCoord4s( target, s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4sARB( GLenum target, GLshort s, GLshort t, GLshort r, GLshort q ) {
  TRACE("(%d, %d, %d, %d, %d)\n", target, s, t, r, q );
  ENTER_GL();
  func_glMultiTexCoord4sARB( target, s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4sSGIS( GLenum target, GLshort s, GLshort t, GLshort r, GLshort q ) {
  TRACE("(%d, %d, %d, %d, %d)\n", target, s, t, r, q );
  ENTER_GL();
  func_glMultiTexCoord4sSGIS( target, s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4sv( GLenum target, GLshort* v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord4sv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4svARB( GLenum target, GLshort* v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord4svARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4svSGIS( GLenum target, GLshort * v ) {
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord4svSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoordPointerSGIS( GLenum target, GLint size, GLenum type, GLsizei stride, GLvoid * pointer ) {
  TRACE("(%d, %d, %d, %d, %p)\n", target, size, type, stride, pointer );
  ENTER_GL();
  func_glMultiTexCoordPointerSGIS( target, size, type, stride, pointer );
  LEAVE_GL();
}

static GLuint WINAPI wine_glNewBufferRegion( GLenum type ) {
  GLuint ret_value;
  TRACE("(%d)\n", type );
  ENTER_GL();
  ret_value = func_glNewBufferRegion( type );
  LEAVE_GL();
  return ret_value;
}

static GLuint WINAPI wine_glNewObjectBufferATI( GLsizei size, GLvoid* pointer, GLenum usage ) {
  GLuint ret_value;
  TRACE("(%d, %p, %d)\n", size, pointer, usage );
  ENTER_GL();
  ret_value = func_glNewObjectBufferATI( size, pointer, usage );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glNormal3fVertex3fSUN( GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) {
  TRACE("(%f, %f, %f, %f, %f, %f)\n", nx, ny, nz, x, y, z );
  ENTER_GL();
  func_glNormal3fVertex3fSUN( nx, ny, nz, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glNormal3fVertex3fvSUN( GLfloat* n, GLfloat* v ) {
  TRACE("(%p, %p)\n", n, v );
  ENTER_GL();
  func_glNormal3fVertex3fvSUN( n, v );
  LEAVE_GL();
}

static void WINAPI wine_glNormal3hNV( unsigned short nx, unsigned short ny, unsigned short nz ) {
  TRACE("(%d, %d, %d)\n", nx, ny, nz );
  ENTER_GL();
  func_glNormal3hNV( nx, ny, nz );
  LEAVE_GL();
}

static void WINAPI wine_glNormal3hvNV( unsigned short* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glNormal3hvNV( v );
  LEAVE_GL();
}

static void WINAPI wine_glNormalPointerEXT( GLenum type, GLsizei stride, GLsizei count, GLvoid* pointer ) {
  TRACE("(%d, %d, %d, %p)\n", type, stride, count, pointer );
  ENTER_GL();
  func_glNormalPointerEXT( type, stride, count, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glNormalPointerListIBM( GLenum type, GLint stride, GLvoid** pointer, GLint ptrstride ) {
  TRACE("(%d, %d, %p, %d)\n", type, stride, pointer, ptrstride );
  ENTER_GL();
  func_glNormalPointerListIBM( type, stride, pointer, ptrstride );
  LEAVE_GL();
}

static void WINAPI wine_glNormalPointervINTEL( GLenum type, GLvoid** pointer ) {
  TRACE("(%d, %p)\n", type, pointer );
  ENTER_GL();
  func_glNormalPointervINTEL( type, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glNormalStream3bATI( GLenum stream, GLbyte nx, GLbyte ny, GLbyte nz ) {
  TRACE("(%d, %d, %d, %d)\n", stream, nx, ny, nz );
  ENTER_GL();
  func_glNormalStream3bATI( stream, nx, ny, nz );
  LEAVE_GL();
}

static void WINAPI wine_glNormalStream3bvATI( GLenum stream, GLbyte* coords ) {
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glNormalStream3bvATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glNormalStream3dATI( GLenum stream, GLdouble nx, GLdouble ny, GLdouble nz ) {
  TRACE("(%d, %f, %f, %f)\n", stream, nx, ny, nz );
  ENTER_GL();
  func_glNormalStream3dATI( stream, nx, ny, nz );
  LEAVE_GL();
}

static void WINAPI wine_glNormalStream3dvATI( GLenum stream, GLdouble* coords ) {
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glNormalStream3dvATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glNormalStream3fATI( GLenum stream, GLfloat nx, GLfloat ny, GLfloat nz ) {
  TRACE("(%d, %f, %f, %f)\n", stream, nx, ny, nz );
  ENTER_GL();
  func_glNormalStream3fATI( stream, nx, ny, nz );
  LEAVE_GL();
}

static void WINAPI wine_glNormalStream3fvATI( GLenum stream, GLfloat* coords ) {
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glNormalStream3fvATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glNormalStream3iATI( GLenum stream, GLint nx, GLint ny, GLint nz ) {
  TRACE("(%d, %d, %d, %d)\n", stream, nx, ny, nz );
  ENTER_GL();
  func_glNormalStream3iATI( stream, nx, ny, nz );
  LEAVE_GL();
}

static void WINAPI wine_glNormalStream3ivATI( GLenum stream, GLint* coords ) {
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glNormalStream3ivATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glNormalStream3sATI( GLenum stream, GLshort nx, GLshort ny, GLshort nz ) {
  TRACE("(%d, %d, %d, %d)\n", stream, nx, ny, nz );
  ENTER_GL();
  func_glNormalStream3sATI( stream, nx, ny, nz );
  LEAVE_GL();
}

static void WINAPI wine_glNormalStream3svATI( GLenum stream, GLshort* coords ) {
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glNormalStream3svATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glPNTrianglesfATI( GLenum pname, GLfloat param ) {
  TRACE("(%d, %f)\n", pname, param );
  ENTER_GL();
  func_glPNTrianglesfATI( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glPNTrianglesiATI( GLenum pname, GLint param ) {
  TRACE("(%d, %d)\n", pname, param );
  ENTER_GL();
  func_glPNTrianglesiATI( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glPassTexCoordATI( GLuint dst, GLuint coord, GLenum swizzle ) {
  TRACE("(%d, %d, %d)\n", dst, coord, swizzle );
  ENTER_GL();
  func_glPassTexCoordATI( dst, coord, swizzle );
  LEAVE_GL();
}

static void WINAPI wine_glPixelDataRangeNV( GLenum target, GLsizei length, GLvoid* pointer ) {
  TRACE("(%d, %d, %p)\n", target, length, pointer );
  ENTER_GL();
  func_glPixelDataRangeNV( target, length, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glPixelTexGenParameterfSGIS( GLenum pname, GLfloat param ) {
  TRACE("(%d, %f)\n", pname, param );
  ENTER_GL();
  func_glPixelTexGenParameterfSGIS( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glPixelTexGenParameterfvSGIS( GLenum pname, GLfloat* params ) {
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glPixelTexGenParameterfvSGIS( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glPixelTexGenParameteriSGIS( GLenum pname, GLint param ) {
  TRACE("(%d, %d)\n", pname, param );
  ENTER_GL();
  func_glPixelTexGenParameteriSGIS( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glPixelTexGenParameterivSGIS( GLenum pname, GLint* params ) {
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glPixelTexGenParameterivSGIS( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glPixelTexGenSGIX( GLenum mode ) {
  TRACE("(%d)\n", mode );
  ENTER_GL();
  func_glPixelTexGenSGIX( mode );
  LEAVE_GL();
}

static void WINAPI wine_glPixelTransformParameterfEXT( GLenum target, GLenum pname, GLfloat param ) {
  TRACE("(%d, %d, %f)\n", target, pname, param );
  ENTER_GL();
  func_glPixelTransformParameterfEXT( target, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glPixelTransformParameterfvEXT( GLenum target, GLenum pname, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glPixelTransformParameterfvEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glPixelTransformParameteriEXT( GLenum target, GLenum pname, GLint param ) {
  TRACE("(%d, %d, %d)\n", target, pname, param );
  ENTER_GL();
  func_glPixelTransformParameteriEXT( target, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glPixelTransformParameterivEXT( GLenum target, GLenum pname, GLint* params ) {
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glPixelTransformParameterivEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glPointParameterf( GLenum pname, GLfloat param ) {
  TRACE("(%d, %f)\n", pname, param );
  ENTER_GL();
  func_glPointParameterf( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glPointParameterfARB( GLenum pname, GLfloat param ) {
  TRACE("(%d, %f)\n", pname, param );
  ENTER_GL();
  func_glPointParameterfARB( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glPointParameterfEXT( GLenum pname, GLfloat param ) {
  TRACE("(%d, %f)\n", pname, param );
  ENTER_GL();
  func_glPointParameterfEXT( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glPointParameterfSGIS( GLenum pname, GLfloat param ) {
  TRACE("(%d, %f)\n", pname, param );
  ENTER_GL();
  func_glPointParameterfSGIS( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glPointParameterfv( GLenum pname, GLfloat* params ) {
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glPointParameterfv( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glPointParameterfvARB( GLenum pname, GLfloat* params ) {
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glPointParameterfvARB( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glPointParameterfvEXT( GLenum pname, GLfloat* params ) {
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glPointParameterfvEXT( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glPointParameterfvSGIS( GLenum pname, GLfloat* params ) {
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glPointParameterfvSGIS( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glPointParameteri( GLenum pname, GLint param ) {
  TRACE("(%d, %d)\n", pname, param );
  ENTER_GL();
  func_glPointParameteri( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glPointParameteriNV( GLenum pname, GLint param ) {
  TRACE("(%d, %d)\n", pname, param );
  ENTER_GL();
  func_glPointParameteriNV( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glPointParameteriv( GLenum pname, GLint* params ) {
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glPointParameteriv( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glPointParameterivNV( GLenum pname, GLint* params ) {
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glPointParameterivNV( pname, params );
  LEAVE_GL();
}

static GLint WINAPI wine_glPollAsyncSGIX( GLuint* markerp ) {
  GLint ret_value;
  TRACE("(%p)\n", markerp );
  ENTER_GL();
  ret_value = func_glPollAsyncSGIX( markerp );
  LEAVE_GL();
  return ret_value;
}

static GLint WINAPI wine_glPollInstrumentsSGIX( GLint* marker_p ) {
  GLint ret_value;
  TRACE("(%p)\n", marker_p );
  ENTER_GL();
  ret_value = func_glPollInstrumentsSGIX( marker_p );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glPolygonOffsetEXT( GLfloat factor, GLfloat bias ) {
  TRACE("(%f, %f)\n", factor, bias );
  ENTER_GL();
  func_glPolygonOffsetEXT( factor, bias );
  LEAVE_GL();
}

static void WINAPI wine_glPrimitiveRestartIndexNV( GLuint index ) {
  TRACE("(%d)\n", index );
  ENTER_GL();
  func_glPrimitiveRestartIndexNV( index );
  LEAVE_GL();
}

static void WINAPI wine_glPrimitiveRestartNV( void ) {
  TRACE("()\n");
  ENTER_GL();
  func_glPrimitiveRestartNV( );
  LEAVE_GL();
}

static void WINAPI wine_glPrioritizeTexturesEXT( GLsizei n, GLuint* textures, GLclampf* priorities ) {
  TRACE("(%d, %p, %p)\n", n, textures, priorities );
  ENTER_GL();
  func_glPrioritizeTexturesEXT( n, textures, priorities );
  LEAVE_GL();
}

static void WINAPI wine_glProgramEnvParameter4dARB( GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  TRACE("(%d, %d, %f, %f, %f, %f)\n", target, index, x, y, z, w );
  ENTER_GL();
  func_glProgramEnvParameter4dARB( target, index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glProgramEnvParameter4dvARB( GLenum target, GLuint index, GLdouble* params ) {
  TRACE("(%d, %d, %p)\n", target, index, params );
  ENTER_GL();
  func_glProgramEnvParameter4dvARB( target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glProgramEnvParameter4fARB( GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  TRACE("(%d, %d, %f, %f, %f, %f)\n", target, index, x, y, z, w );
  ENTER_GL();
  func_glProgramEnvParameter4fARB( target, index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glProgramEnvParameter4fvARB( GLenum target, GLuint index, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", target, index, params );
  ENTER_GL();
  func_glProgramEnvParameter4fvARB( target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glProgramLocalParameter4dARB( GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  TRACE("(%d, %d, %f, %f, %f, %f)\n", target, index, x, y, z, w );
  ENTER_GL();
  func_glProgramLocalParameter4dARB( target, index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glProgramLocalParameter4dvARB( GLenum target, GLuint index, GLdouble* params ) {
  TRACE("(%d, %d, %p)\n", target, index, params );
  ENTER_GL();
  func_glProgramLocalParameter4dvARB( target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glProgramLocalParameter4fARB( GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  TRACE("(%d, %d, %f, %f, %f, %f)\n", target, index, x, y, z, w );
  ENTER_GL();
  func_glProgramLocalParameter4fARB( target, index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glProgramLocalParameter4fvARB( GLenum target, GLuint index, GLfloat* params ) {
  TRACE("(%d, %d, %p)\n", target, index, params );
  ENTER_GL();
  func_glProgramLocalParameter4fvARB( target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glProgramNamedParameter4dNV( GLuint id, GLsizei len, GLubyte* name, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  TRACE("(%d, %d, %p, %f, %f, %f, %f)\n", id, len, name, x, y, z, w );
  ENTER_GL();
  func_glProgramNamedParameter4dNV( id, len, name, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glProgramNamedParameter4dvNV( GLuint id, GLsizei len, GLubyte* name, GLdouble* v ) {
  TRACE("(%d, %d, %p, %p)\n", id, len, name, v );
  ENTER_GL();
  func_glProgramNamedParameter4dvNV( id, len, name, v );
  LEAVE_GL();
}

static void WINAPI wine_glProgramNamedParameter4fNV( GLuint id, GLsizei len, GLubyte* name, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  TRACE("(%d, %d, %p, %f, %f, %f, %f)\n", id, len, name, x, y, z, w );
  ENTER_GL();
  func_glProgramNamedParameter4fNV( id, len, name, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glProgramNamedParameter4fvNV( GLuint id, GLsizei len, GLubyte* name, GLfloat* v ) {
  TRACE("(%d, %d, %p, %p)\n", id, len, name, v );
  ENTER_GL();
  func_glProgramNamedParameter4fvNV( id, len, name, v );
  LEAVE_GL();
}

static void WINAPI wine_glProgramParameter4dNV( GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  TRACE("(%d, %d, %f, %f, %f, %f)\n", target, index, x, y, z, w );
  ENTER_GL();
  func_glProgramParameter4dNV( target, index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glProgramParameter4dvNV( GLenum target, GLuint index, GLdouble* v ) {
  TRACE("(%d, %d, %p)\n", target, index, v );
  ENTER_GL();
  func_glProgramParameter4dvNV( target, index, v );
  LEAVE_GL();
}

static void WINAPI wine_glProgramParameter4fNV( GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  TRACE("(%d, %d, %f, %f, %f, %f)\n", target, index, x, y, z, w );
  ENTER_GL();
  func_glProgramParameter4fNV( target, index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glProgramParameter4fvNV( GLenum target, GLuint index, GLfloat* v ) {
  TRACE("(%d, %d, %p)\n", target, index, v );
  ENTER_GL();
  func_glProgramParameter4fvNV( target, index, v );
  LEAVE_GL();
}

static void WINAPI wine_glProgramParameters4dvNV( GLenum target, GLuint index, GLuint count, GLdouble* v ) {
  TRACE("(%d, %d, %d, %p)\n", target, index, count, v );
  ENTER_GL();
  func_glProgramParameters4dvNV( target, index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glProgramParameters4fvNV( GLenum target, GLuint index, GLuint count, GLfloat* v ) {
  TRACE("(%d, %d, %d, %p)\n", target, index, count, v );
  ENTER_GL();
  func_glProgramParameters4fvNV( target, index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glProgramStringARB( GLenum target, GLenum format, GLsizei len, GLvoid* string ) {
  TRACE("(%d, %d, %d, %p)\n", target, format, len, string );
  ENTER_GL();
  func_glProgramStringARB( target, format, len, string );
  LEAVE_GL();
}

static void WINAPI wine_glReadBufferRegion( GLenum region, GLint x, GLint y, GLsizei width, GLsizei height ) {
  TRACE("(%d, %d, %d, %d, %d)\n", region, x, y, width, height );
  ENTER_GL();
  func_glReadBufferRegion( region, x, y, width, height );
  LEAVE_GL();
}

static void WINAPI wine_glReadInstrumentsSGIX( GLint marker ) {
  TRACE("(%d)\n", marker );
  ENTER_GL();
  func_glReadInstrumentsSGIX( marker );
  LEAVE_GL();
}

static void WINAPI wine_glReferencePlaneSGIX( GLdouble* equation ) {
  TRACE("(%p)\n", equation );
  ENTER_GL();
  func_glReferencePlaneSGIX( equation );
  LEAVE_GL();
}

static void WINAPI wine_glRenderbufferStorageEXT( GLenum target, GLenum internalformat, GLsizei width, GLsizei height ) {
  TRACE("(%d, %d, %d, %d)\n", target, internalformat, width, height );
  ENTER_GL();
  func_glRenderbufferStorageEXT( target, internalformat, width, height );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodePointerSUN( GLenum type, GLsizei stride, GLvoid** pointer ) {
  TRACE("(%d, %d, %p)\n", type, stride, pointer );
  ENTER_GL();
  func_glReplacementCodePointerSUN( type, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeubSUN( GLubyte code ) {
  TRACE("(%d)\n", code );
  ENTER_GL();
  func_glReplacementCodeubSUN( code );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeubvSUN( GLubyte* code ) {
  TRACE("(%p)\n", code );
  ENTER_GL();
  func_glReplacementCodeubvSUN( code );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiColor3fVertex3fSUN( GLuint rc, GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z ) {
  TRACE("(%d, %f, %f, %f, %f, %f, %f)\n", rc, r, g, b, x, y, z );
  ENTER_GL();
  func_glReplacementCodeuiColor3fVertex3fSUN( rc, r, g, b, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiColor3fVertex3fvSUN( GLuint* rc, GLfloat* c, GLfloat* v ) {
  TRACE("(%p, %p, %p)\n", rc, c, v );
  ENTER_GL();
  func_glReplacementCodeuiColor3fVertex3fvSUN( rc, c, v );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiColor4fNormal3fVertex3fSUN( GLuint rc, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) {
  TRACE("(%d, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f)\n", rc, r, g, b, a, nx, ny, nz, x, y, z );
  ENTER_GL();
  func_glReplacementCodeuiColor4fNormal3fVertex3fSUN( rc, r, g, b, a, nx, ny, nz, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiColor4fNormal3fVertex3fvSUN( GLuint* rc, GLfloat* c, GLfloat* n, GLfloat* v ) {
  TRACE("(%p, %p, %p, %p)\n", rc, c, n, v );
  ENTER_GL();
  func_glReplacementCodeuiColor4fNormal3fVertex3fvSUN( rc, c, n, v );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiColor4ubVertex3fSUN( GLuint rc, GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z ) {
  TRACE("(%d, %d, %d, %d, %d, %f, %f, %f)\n", rc, r, g, b, a, x, y, z );
  ENTER_GL();
  func_glReplacementCodeuiColor4ubVertex3fSUN( rc, r, g, b, a, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiColor4ubVertex3fvSUN( GLuint* rc, GLubyte* c, GLfloat* v ) {
  TRACE("(%p, %p, %p)\n", rc, c, v );
  ENTER_GL();
  func_glReplacementCodeuiColor4ubVertex3fvSUN( rc, c, v );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiNormal3fVertex3fSUN( GLuint rc, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) {
  TRACE("(%d, %f, %f, %f, %f, %f, %f)\n", rc, nx, ny, nz, x, y, z );
  ENTER_GL();
  func_glReplacementCodeuiNormal3fVertex3fSUN( rc, nx, ny, nz, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiNormal3fVertex3fvSUN( GLuint* rc, GLfloat* n, GLfloat* v ) {
  TRACE("(%p, %p, %p)\n", rc, n, v );
  ENTER_GL();
  func_glReplacementCodeuiNormal3fVertex3fvSUN( rc, n, v );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiSUN( GLuint code ) {
  TRACE("(%d)\n", code );
  ENTER_GL();
  func_glReplacementCodeuiSUN( code );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN( GLuint rc, GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) {
  TRACE("(%d, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f)\n", rc, s, t, r, g, b, a, nx, ny, nz, x, y, z );
  ENTER_GL();
  func_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN( rc, s, t, r, g, b, a, nx, ny, nz, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN( GLuint* rc, GLfloat* tc, GLfloat* c, GLfloat* n, GLfloat* v ) {
  TRACE("(%p, %p, %p, %p, %p)\n", rc, tc, c, n, v );
  ENTER_GL();
  func_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN( rc, tc, c, n, v );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN( GLuint rc, GLfloat s, GLfloat t, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) {
  TRACE("(%d, %f, %f, %f, %f, %f, %f, %f, %f)\n", rc, s, t, nx, ny, nz, x, y, z );
  ENTER_GL();
  func_glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN( rc, s, t, nx, ny, nz, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN( GLuint* rc, GLfloat* tc, GLfloat* n, GLfloat* v ) {
  TRACE("(%p, %p, %p, %p)\n", rc, tc, n, v );
  ENTER_GL();
  func_glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN( rc, tc, n, v );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiTexCoord2fVertex3fSUN( GLuint rc, GLfloat s, GLfloat t, GLfloat x, GLfloat y, GLfloat z ) {
  TRACE("(%d, %f, %f, %f, %f, %f)\n", rc, s, t, x, y, z );
  ENTER_GL();
  func_glReplacementCodeuiTexCoord2fVertex3fSUN( rc, s, t, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiTexCoord2fVertex3fvSUN( GLuint* rc, GLfloat* tc, GLfloat* v ) {
  TRACE("(%p, %p, %p)\n", rc, tc, v );
  ENTER_GL();
  func_glReplacementCodeuiTexCoord2fVertex3fvSUN( rc, tc, v );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiVertex3fSUN( GLuint rc, GLfloat x, GLfloat y, GLfloat z ) {
  TRACE("(%d, %f, %f, %f)\n", rc, x, y, z );
  ENTER_GL();
  func_glReplacementCodeuiVertex3fSUN( rc, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiVertex3fvSUN( GLuint* rc, GLfloat* v ) {
  TRACE("(%p, %p)\n", rc, v );
  ENTER_GL();
  func_glReplacementCodeuiVertex3fvSUN( rc, v );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuivSUN( GLuint* code ) {
  TRACE("(%p)\n", code );
  ENTER_GL();
  func_glReplacementCodeuivSUN( code );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeusSUN( GLushort code ) {
  TRACE("(%d)\n", code );
  ENTER_GL();
  func_glReplacementCodeusSUN( code );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeusvSUN( GLushort* code ) {
  TRACE("(%p)\n", code );
  ENTER_GL();
  func_glReplacementCodeusvSUN( code );
  LEAVE_GL();
}

static void WINAPI wine_glRequestResidentProgramsNV( GLsizei n, GLuint* programs ) {
  TRACE("(%d, %p)\n", n, programs );
  ENTER_GL();
  func_glRequestResidentProgramsNV( n, programs );
  LEAVE_GL();
}

static void WINAPI wine_glResetHistogramEXT( GLenum target ) {
  TRACE("(%d)\n", target );
  ENTER_GL();
  func_glResetHistogramEXT( target );
  LEAVE_GL();
}

static void WINAPI wine_glResetMinmaxEXT( GLenum target ) {
  TRACE("(%d)\n", target );
  ENTER_GL();
  func_glResetMinmaxEXT( target );
  LEAVE_GL();
}

static void WINAPI wine_glResizeBuffersMESA( void ) {
  TRACE("()\n");
  ENTER_GL();
  func_glResizeBuffersMESA( );
  LEAVE_GL();
}

static void WINAPI wine_glSampleCoverage( GLclampf value, GLboolean invert ) {
  TRACE("(%f, %d)\n", value, invert );
  ENTER_GL();
  func_glSampleCoverage( value, invert );
  LEAVE_GL();
}

static void WINAPI wine_glSampleCoverageARB( GLclampf value, GLboolean invert ) {
  TRACE("(%f, %d)\n", value, invert );
  ENTER_GL();
  func_glSampleCoverageARB( value, invert );
  LEAVE_GL();
}

static void WINAPI wine_glSampleMapATI( GLuint dst, GLuint interp, GLenum swizzle ) {
  TRACE("(%d, %d, %d)\n", dst, interp, swizzle );
  ENTER_GL();
  func_glSampleMapATI( dst, interp, swizzle );
  LEAVE_GL();
}

static void WINAPI wine_glSampleMaskEXT( GLclampf value, GLboolean invert ) {
  TRACE("(%f, %d)\n", value, invert );
  ENTER_GL();
  func_glSampleMaskEXT( value, invert );
  LEAVE_GL();
}

static void WINAPI wine_glSampleMaskSGIS( GLclampf value, GLboolean invert ) {
  TRACE("(%f, %d)\n", value, invert );
  ENTER_GL();
  func_glSampleMaskSGIS( value, invert );
  LEAVE_GL();
}

static void WINAPI wine_glSamplePatternEXT( GLenum pattern ) {
  TRACE("(%d)\n", pattern );
  ENTER_GL();
  func_glSamplePatternEXT( pattern );
  LEAVE_GL();
}

static void WINAPI wine_glSamplePatternSGIS( GLenum pattern ) {
  TRACE("(%d)\n", pattern );
  ENTER_GL();
  func_glSamplePatternSGIS( pattern );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3b( GLbyte red, GLbyte green, GLbyte blue ) {
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3b( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3bEXT( GLbyte red, GLbyte green, GLbyte blue ) {
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3bEXT( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3bv( GLbyte* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3bv( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3bvEXT( GLbyte* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3bvEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3d( GLdouble red, GLdouble green, GLdouble blue ) {
  TRACE("(%f, %f, %f)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3d( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3dEXT( GLdouble red, GLdouble green, GLdouble blue ) {
  TRACE("(%f, %f, %f)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3dEXT( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3dv( GLdouble* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3dv( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3dvEXT( GLdouble* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3dvEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3f( GLfloat red, GLfloat green, GLfloat blue ) {
  TRACE("(%f, %f, %f)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3f( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3fEXT( GLfloat red, GLfloat green, GLfloat blue ) {
  TRACE("(%f, %f, %f)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3fEXT( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3fv( GLfloat* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3fv( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3fvEXT( GLfloat* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3fvEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3hNV( unsigned short red, unsigned short green, unsigned short blue ) {
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3hNV( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3hvNV( unsigned short* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3hvNV( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3i( GLint red, GLint green, GLint blue ) {
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3i( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3iEXT( GLint red, GLint green, GLint blue ) {
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3iEXT( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3iv( GLint* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3iv( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3ivEXT( GLint* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3ivEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3s( GLshort red, GLshort green, GLshort blue ) {
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3s( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3sEXT( GLshort red, GLshort green, GLshort blue ) {
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3sEXT( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3sv( GLshort* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3sv( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3svEXT( GLshort* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3svEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3ub( GLubyte red, GLubyte green, GLubyte blue ) {
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3ub( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3ubEXT( GLubyte red, GLubyte green, GLubyte blue ) {
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3ubEXT( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3ubv( GLubyte* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3ubv( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3ubvEXT( GLubyte* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3ubvEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3ui( GLuint red, GLuint green, GLuint blue ) {
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3ui( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3uiEXT( GLuint red, GLuint green, GLuint blue ) {
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3uiEXT( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3uiv( GLuint* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3uiv( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3uivEXT( GLuint* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3uivEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3us( GLushort red, GLushort green, GLushort blue ) {
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3us( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3usEXT( GLushort red, GLushort green, GLushort blue ) {
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3usEXT( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3usv( GLushort* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3usv( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3usvEXT( GLushort* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3usvEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColorPointer( GLint size, GLenum type, GLsizei stride, GLvoid* pointer ) {
  TRACE("(%d, %d, %d, %p)\n", size, type, stride, pointer );
  ENTER_GL();
  func_glSecondaryColorPointer( size, type, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColorPointerEXT( GLint size, GLenum type, GLsizei stride, GLvoid* pointer ) {
  TRACE("(%d, %d, %d, %p)\n", size, type, stride, pointer );
  ENTER_GL();
  func_glSecondaryColorPointerEXT( size, type, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColorPointerListIBM( GLint size, GLenum type, GLint stride, GLvoid** pointer, GLint ptrstride ) {
  TRACE("(%d, %d, %d, %p, %d)\n", size, type, stride, pointer, ptrstride );
  ENTER_GL();
  func_glSecondaryColorPointerListIBM( size, type, stride, pointer, ptrstride );
  LEAVE_GL();
}

static void WINAPI wine_glSelectTextureCoordSetSGIS( GLenum target ) {
  TRACE("(%d)\n", target );
  ENTER_GL();
  func_glSelectTextureCoordSetSGIS( target );
  LEAVE_GL();
}

static void WINAPI wine_glSelectTextureSGIS( GLenum target ) {
  TRACE("(%d)\n", target );
  ENTER_GL();
  func_glSelectTextureSGIS( target );
  LEAVE_GL();
}

static void WINAPI wine_glSeparableFilter2DEXT( GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* row, GLvoid* column ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %p, %p)\n", target, internalformat, width, height, format, type, row, column );
  ENTER_GL();
  func_glSeparableFilter2DEXT( target, internalformat, width, height, format, type, row, column );
  LEAVE_GL();
}

static void WINAPI wine_glSetFenceAPPLE( GLuint fence ) {
  TRACE("(%d)\n", fence );
  ENTER_GL();
  func_glSetFenceAPPLE( fence );
  LEAVE_GL();
}

static void WINAPI wine_glSetFenceNV( GLuint fence, GLenum condition ) {
  TRACE("(%d, %d)\n", fence, condition );
  ENTER_GL();
  func_glSetFenceNV( fence, condition );
  LEAVE_GL();
}

static void WINAPI wine_glSetFragmentShaderConstantATI( GLuint dst, GLfloat* value ) {
  TRACE("(%d, %p)\n", dst, value );
  ENTER_GL();
  func_glSetFragmentShaderConstantATI( dst, value );
  LEAVE_GL();
}

static void WINAPI wine_glSetInvariantEXT( GLuint id, GLenum type, GLvoid* addr ) {
  TRACE("(%d, %d, %p)\n", id, type, addr );
  ENTER_GL();
  func_glSetInvariantEXT( id, type, addr );
  LEAVE_GL();
}

static void WINAPI wine_glSetLocalConstantEXT( GLuint id, GLenum type, GLvoid* addr ) {
  TRACE("(%d, %d, %p)\n", id, type, addr );
  ENTER_GL();
  func_glSetLocalConstantEXT( id, type, addr );
  LEAVE_GL();
}

static void WINAPI wine_glShaderOp1EXT( GLenum op, GLuint res, GLuint arg1 ) {
  TRACE("(%d, %d, %d)\n", op, res, arg1 );
  ENTER_GL();
  func_glShaderOp1EXT( op, res, arg1 );
  LEAVE_GL();
}

static void WINAPI wine_glShaderOp2EXT( GLenum op, GLuint res, GLuint arg1, GLuint arg2 ) {
  TRACE("(%d, %d, %d, %d)\n", op, res, arg1, arg2 );
  ENTER_GL();
  func_glShaderOp2EXT( op, res, arg1, arg2 );
  LEAVE_GL();
}

static void WINAPI wine_glShaderOp3EXT( GLenum op, GLuint res, GLuint arg1, GLuint arg2, GLuint arg3 ) {
  TRACE("(%d, %d, %d, %d, %d)\n", op, res, arg1, arg2, arg3 );
  ENTER_GL();
  func_glShaderOp3EXT( op, res, arg1, arg2, arg3 );
  LEAVE_GL();
}

static void WINAPI wine_glShaderSource( GLuint shader, GLsizei count, char** string, GLint* length ) {
  TRACE("(%d, %d, %p, %p)\n", shader, count, string, length );
  ENTER_GL();
  func_glShaderSource( shader, count, string, length );
  LEAVE_GL();
}

static void WINAPI wine_glShaderSourceARB( unsigned int shaderObj, GLsizei count, char** string, GLint* length ) {
  TRACE("(%d, %d, %p, %p)\n", shaderObj, count, string, length );
  ENTER_GL();
  func_glShaderSourceARB( shaderObj, count, string, length );
  LEAVE_GL();
}

static void WINAPI wine_glSharpenTexFuncSGIS( GLenum target, GLsizei n, GLfloat* points ) {
  TRACE("(%d, %d, %p)\n", target, n, points );
  ENTER_GL();
  func_glSharpenTexFuncSGIS( target, n, points );
  LEAVE_GL();
}

static void WINAPI wine_glSpriteParameterfSGIX( GLenum pname, GLfloat param ) {
  TRACE("(%d, %f)\n", pname, param );
  ENTER_GL();
  func_glSpriteParameterfSGIX( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glSpriteParameterfvSGIX( GLenum pname, GLfloat* params ) {
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glSpriteParameterfvSGIX( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glSpriteParameteriSGIX( GLenum pname, GLint param ) {
  TRACE("(%d, %d)\n", pname, param );
  ENTER_GL();
  func_glSpriteParameteriSGIX( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glSpriteParameterivSGIX( GLenum pname, GLint* params ) {
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glSpriteParameterivSGIX( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glStartInstrumentsSGIX( void ) {
  TRACE("()\n");
  ENTER_GL();
  func_glStartInstrumentsSGIX( );
  LEAVE_GL();
}

static void WINAPI wine_glStencilFuncSeparate( GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask ) {
  TRACE("(%d, %d, %d, %d)\n", frontfunc, backfunc, ref, mask );
  ENTER_GL();
  func_glStencilFuncSeparate( frontfunc, backfunc, ref, mask );
  LEAVE_GL();
}

static void WINAPI wine_glStencilFuncSeparateATI( GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask ) {
  TRACE("(%d, %d, %d, %d)\n", frontfunc, backfunc, ref, mask );
  ENTER_GL();
  func_glStencilFuncSeparateATI( frontfunc, backfunc, ref, mask );
  LEAVE_GL();
}

static void WINAPI wine_glStencilMaskSeparate( GLenum face, GLuint mask ) {
  TRACE("(%d, %d)\n", face, mask );
  ENTER_GL();
  func_glStencilMaskSeparate( face, mask );
  LEAVE_GL();
}

static void WINAPI wine_glStencilOpSeparate( GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass ) {
  TRACE("(%d, %d, %d, %d)\n", face, sfail, dpfail, dppass );
  ENTER_GL();
  func_glStencilOpSeparate( face, sfail, dpfail, dppass );
  LEAVE_GL();
}

static void WINAPI wine_glStencilOpSeparateATI( GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass ) {
  TRACE("(%d, %d, %d, %d)\n", face, sfail, dpfail, dppass );
  ENTER_GL();
  func_glStencilOpSeparateATI( face, sfail, dpfail, dppass );
  LEAVE_GL();
}

static void WINAPI wine_glStopInstrumentsSGIX( GLint marker ) {
  TRACE("(%d)\n", marker );
  ENTER_GL();
  func_glStopInstrumentsSGIX( marker );
  LEAVE_GL();
}

static void WINAPI wine_glStringMarkerGREMEDY( GLsizei len, GLvoid* string ) {
  TRACE("(%d, %p)\n", len, string );
  ENTER_GL();
  func_glStringMarkerGREMEDY( len, string );
  LEAVE_GL();
}

static void WINAPI wine_glSwizzleEXT( GLuint res, GLuint in, GLenum outX, GLenum outY, GLenum outZ, GLenum outW ) {
  TRACE("(%d, %d, %d, %d, %d, %d)\n", res, in, outX, outY, outZ, outW );
  ENTER_GL();
  func_glSwizzleEXT( res, in, outX, outY, outZ, outW );
  LEAVE_GL();
}

static void WINAPI wine_glTagSampleBufferSGIX( void ) {
  TRACE("()\n");
  ENTER_GL();
  func_glTagSampleBufferSGIX( );
  LEAVE_GL();
}

static void WINAPI wine_glTangent3bEXT( GLbyte tx, GLbyte ty, GLbyte tz ) {
  TRACE("(%d, %d, %d)\n", tx, ty, tz );
  ENTER_GL();
  func_glTangent3bEXT( tx, ty, tz );
  LEAVE_GL();
}

static void WINAPI wine_glTangent3bvEXT( GLbyte* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glTangent3bvEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glTangent3dEXT( GLdouble tx, GLdouble ty, GLdouble tz ) {
  TRACE("(%f, %f, %f)\n", tx, ty, tz );
  ENTER_GL();
  func_glTangent3dEXT( tx, ty, tz );
  LEAVE_GL();
}

static void WINAPI wine_glTangent3dvEXT( GLdouble* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glTangent3dvEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glTangent3fEXT( GLfloat tx, GLfloat ty, GLfloat tz ) {
  TRACE("(%f, %f, %f)\n", tx, ty, tz );
  ENTER_GL();
  func_glTangent3fEXT( tx, ty, tz );
  LEAVE_GL();
}

static void WINAPI wine_glTangent3fvEXT( GLfloat* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glTangent3fvEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glTangent3iEXT( GLint tx, GLint ty, GLint tz ) {
  TRACE("(%d, %d, %d)\n", tx, ty, tz );
  ENTER_GL();
  func_glTangent3iEXT( tx, ty, tz );
  LEAVE_GL();
}

static void WINAPI wine_glTangent3ivEXT( GLint* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glTangent3ivEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glTangent3sEXT( GLshort tx, GLshort ty, GLshort tz ) {
  TRACE("(%d, %d, %d)\n", tx, ty, tz );
  ENTER_GL();
  func_glTangent3sEXT( tx, ty, tz );
  LEAVE_GL();
}

static void WINAPI wine_glTangent3svEXT( GLshort* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glTangent3svEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glTangentPointerEXT( GLenum type, GLsizei stride, GLvoid* pointer ) {
  TRACE("(%d, %d, %p)\n", type, stride, pointer );
  ENTER_GL();
  func_glTangentPointerEXT( type, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glTbufferMask3DFX( GLuint mask ) {
  TRACE("(%d)\n", mask );
  ENTER_GL();
  func_glTbufferMask3DFX( mask );
  LEAVE_GL();
}

static GLboolean WINAPI wine_glTestFenceAPPLE( GLuint fence ) {
  GLboolean ret_value;
  TRACE("(%d)\n", fence );
  ENTER_GL();
  ret_value = func_glTestFenceAPPLE( fence );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glTestFenceNV( GLuint fence ) {
  GLboolean ret_value;
  TRACE("(%d)\n", fence );
  ENTER_GL();
  ret_value = func_glTestFenceNV( fence );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glTestObjectAPPLE( GLenum object, GLuint name ) {
  GLboolean ret_value;
  TRACE("(%d, %d)\n", object, name );
  ENTER_GL();
  ret_value = func_glTestObjectAPPLE( object, name );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glTexBumpParameterfvATI( GLenum pname, GLfloat* param ) {
  TRACE("(%d, %p)\n", pname, param );
  ENTER_GL();
  func_glTexBumpParameterfvATI( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glTexBumpParameterivATI( GLenum pname, GLint* param ) {
  TRACE("(%d, %p)\n", pname, param );
  ENTER_GL();
  func_glTexBumpParameterivATI( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord1hNV( unsigned short s ) {
  TRACE("(%d)\n", s );
  ENTER_GL();
  func_glTexCoord1hNV( s );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord1hvNV( unsigned short* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glTexCoord1hvNV( v );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord2fColor3fVertex3fSUN( GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z ) {
  TRACE("(%f, %f, %f, %f, %f, %f, %f, %f)\n", s, t, r, g, b, x, y, z );
  ENTER_GL();
  func_glTexCoord2fColor3fVertex3fSUN( s, t, r, g, b, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord2fColor3fVertex3fvSUN( GLfloat* tc, GLfloat* c, GLfloat* v ) {
  TRACE("(%p, %p, %p)\n", tc, c, v );
  ENTER_GL();
  func_glTexCoord2fColor3fVertex3fvSUN( tc, c, v );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord2fColor4fNormal3fVertex3fSUN( GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) {
  TRACE("(%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f)\n", s, t, r, g, b, a, nx, ny, nz, x, y, z );
  ENTER_GL();
  func_glTexCoord2fColor4fNormal3fVertex3fSUN( s, t, r, g, b, a, nx, ny, nz, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord2fColor4fNormal3fVertex3fvSUN( GLfloat* tc, GLfloat* c, GLfloat* n, GLfloat* v ) {
  TRACE("(%p, %p, %p, %p)\n", tc, c, n, v );
  ENTER_GL();
  func_glTexCoord2fColor4fNormal3fVertex3fvSUN( tc, c, n, v );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord2fColor4ubVertex3fSUN( GLfloat s, GLfloat t, GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z ) {
  TRACE("(%f, %f, %d, %d, %d, %d, %f, %f, %f)\n", s, t, r, g, b, a, x, y, z );
  ENTER_GL();
  func_glTexCoord2fColor4ubVertex3fSUN( s, t, r, g, b, a, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord2fColor4ubVertex3fvSUN( GLfloat* tc, GLubyte* c, GLfloat* v ) {
  TRACE("(%p, %p, %p)\n", tc, c, v );
  ENTER_GL();
  func_glTexCoord2fColor4ubVertex3fvSUN( tc, c, v );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord2fNormal3fVertex3fSUN( GLfloat s, GLfloat t, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) {
  TRACE("(%f, %f, %f, %f, %f, %f, %f, %f)\n", s, t, nx, ny, nz, x, y, z );
  ENTER_GL();
  func_glTexCoord2fNormal3fVertex3fSUN( s, t, nx, ny, nz, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord2fNormal3fVertex3fvSUN( GLfloat* tc, GLfloat* n, GLfloat* v ) {
  TRACE("(%p, %p, %p)\n", tc, n, v );
  ENTER_GL();
  func_glTexCoord2fNormal3fVertex3fvSUN( tc, n, v );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord2fVertex3fSUN( GLfloat s, GLfloat t, GLfloat x, GLfloat y, GLfloat z ) {
  TRACE("(%f, %f, %f, %f, %f)\n", s, t, x, y, z );
  ENTER_GL();
  func_glTexCoord2fVertex3fSUN( s, t, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord2fVertex3fvSUN( GLfloat* tc, GLfloat* v ) {
  TRACE("(%p, %p)\n", tc, v );
  ENTER_GL();
  func_glTexCoord2fVertex3fvSUN( tc, v );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord2hNV( unsigned short s, unsigned short t ) {
  TRACE("(%d, %d)\n", s, t );
  ENTER_GL();
  func_glTexCoord2hNV( s, t );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord2hvNV( unsigned short* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glTexCoord2hvNV( v );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord3hNV( unsigned short s, unsigned short t, unsigned short r ) {
  TRACE("(%d, %d, %d)\n", s, t, r );
  ENTER_GL();
  func_glTexCoord3hNV( s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord3hvNV( unsigned short* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glTexCoord3hvNV( v );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord4fColor4fNormal3fVertex4fSUN( GLfloat s, GLfloat t, GLfloat p, GLfloat q, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  TRACE("(%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f)\n", s, t, p, q, r, g, b, a, nx, ny, nz, x, y, z, w );
  ENTER_GL();
  func_glTexCoord4fColor4fNormal3fVertex4fSUN( s, t, p, q, r, g, b, a, nx, ny, nz, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord4fColor4fNormal3fVertex4fvSUN( GLfloat* tc, GLfloat* c, GLfloat* n, GLfloat* v ) {
  TRACE("(%p, %p, %p, %p)\n", tc, c, n, v );
  ENTER_GL();
  func_glTexCoord4fColor4fNormal3fVertex4fvSUN( tc, c, n, v );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord4fVertex4fSUN( GLfloat s, GLfloat t, GLfloat p, GLfloat q, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  TRACE("(%f, %f, %f, %f, %f, %f, %f, %f)\n", s, t, p, q, x, y, z, w );
  ENTER_GL();
  func_glTexCoord4fVertex4fSUN( s, t, p, q, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord4fVertex4fvSUN( GLfloat* tc, GLfloat* v ) {
  TRACE("(%p, %p)\n", tc, v );
  ENTER_GL();
  func_glTexCoord4fVertex4fvSUN( tc, v );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord4hNV( unsigned short s, unsigned short t, unsigned short r, unsigned short q ) {
  TRACE("(%d, %d, %d, %d)\n", s, t, r, q );
  ENTER_GL();
  func_glTexCoord4hNV( s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord4hvNV( unsigned short* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glTexCoord4hvNV( v );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoordPointerEXT( GLint size, GLenum type, GLsizei stride, GLsizei count, GLvoid* pointer ) {
  TRACE("(%d, %d, %d, %d, %p)\n", size, type, stride, count, pointer );
  ENTER_GL();
  func_glTexCoordPointerEXT( size, type, stride, count, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoordPointerListIBM( GLint size, GLenum type, GLint stride, GLvoid** pointer, GLint ptrstride ) {
  TRACE("(%d, %d, %d, %p, %d)\n", size, type, stride, pointer, ptrstride );
  ENTER_GL();
  func_glTexCoordPointerListIBM( size, type, stride, pointer, ptrstride );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoordPointervINTEL( GLint size, GLenum type, GLvoid** pointer ) {
  TRACE("(%d, %d, %p)\n", size, type, pointer );
  ENTER_GL();
  func_glTexCoordPointervINTEL( size, type, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glTexFilterFuncSGIS( GLenum target, GLenum filter, GLsizei n, GLfloat* weights ) {
  TRACE("(%d, %d, %d, %p)\n", target, filter, n, weights );
  ENTER_GL();
  func_glTexFilterFuncSGIS( target, filter, n, weights );
  LEAVE_GL();
}

static void WINAPI wine_glTexImage3DEXT( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, GLvoid* pixels ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, height, depth, border, format, type, pixels );
  ENTER_GL();
  func_glTexImage3DEXT( target, level, internalformat, width, height, depth, border, format, type, pixels );
  LEAVE_GL();
}

static void WINAPI wine_glTexImage4DSGIS( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLint border, GLenum format, GLenum type, GLvoid* pixels ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, height, depth, size4d, border, format, type, pixels );
  ENTER_GL();
  func_glTexImage4DSGIS( target, level, internalformat, width, height, depth, size4d, border, format, type, pixels );
  LEAVE_GL();
}

static void WINAPI wine_glTexSubImage1DEXT( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, GLvoid* pixels ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, width, format, type, pixels );
  ENTER_GL();
  func_glTexSubImage1DEXT( target, level, xoffset, width, format, type, pixels );
  LEAVE_GL();
}

static void WINAPI wine_glTexSubImage2DEXT( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, width, height, format, type, pixels );
  ENTER_GL();
  func_glTexSubImage2DEXT( target, level, xoffset, yoffset, width, height, format, type, pixels );
  LEAVE_GL();
}

static void WINAPI wine_glTexSubImage3DEXT( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, GLvoid* pixels ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels );
  ENTER_GL();
  func_glTexSubImage3DEXT( target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels );
  LEAVE_GL();
}

static void WINAPI wine_glTexSubImage4DSGIS( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint woffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLenum format, GLenum type, GLvoid* pixels ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, zoffset, woffset, width, height, depth, size4d, format, type, pixels );
  ENTER_GL();
  func_glTexSubImage4DSGIS( target, level, xoffset, yoffset, zoffset, woffset, width, height, depth, size4d, format, type, pixels );
  LEAVE_GL();
}

static void WINAPI wine_glTextureColorMaskSGIS( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha ) {
  TRACE("(%d, %d, %d, %d)\n", red, green, blue, alpha );
  ENTER_GL();
  func_glTextureColorMaskSGIS( red, green, blue, alpha );
  LEAVE_GL();
}

static void WINAPI wine_glTextureLightEXT( GLenum pname ) {
  TRACE("(%d)\n", pname );
  ENTER_GL();
  func_glTextureLightEXT( pname );
  LEAVE_GL();
}

static void WINAPI wine_glTextureMaterialEXT( GLenum face, GLenum mode ) {
  TRACE("(%d, %d)\n", face, mode );
  ENTER_GL();
  func_glTextureMaterialEXT( face, mode );
  LEAVE_GL();
}

static void WINAPI wine_glTextureNormalEXT( GLenum mode ) {
  TRACE("(%d)\n", mode );
  ENTER_GL();
  func_glTextureNormalEXT( mode );
  LEAVE_GL();
}

static void WINAPI wine_glTrackMatrixNV( GLenum target, GLuint address, GLenum matrix, GLenum transform ) {
  TRACE("(%d, %d, %d, %d)\n", target, address, matrix, transform );
  ENTER_GL();
  func_glTrackMatrixNV( target, address, matrix, transform );
  LEAVE_GL();
}

static void WINAPI wine_glUniform1f( GLint location, GLfloat v0 ) {
  TRACE("(%d, %f)\n", location, v0 );
  ENTER_GL();
  func_glUniform1f( location, v0 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform1fARB( GLint location, GLfloat v0 ) {
  TRACE("(%d, %f)\n", location, v0 );
  ENTER_GL();
  func_glUniform1fARB( location, v0 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform1fv( GLint location, GLsizei count, GLfloat* value ) {
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform1fv( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform1fvARB( GLint location, GLsizei count, GLfloat* value ) {
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform1fvARB( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform1i( GLint location, GLint v0 ) {
  TRACE("(%d, %d)\n", location, v0 );
  ENTER_GL();
  func_glUniform1i( location, v0 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform1iARB( GLint location, GLint v0 ) {
  TRACE("(%d, %d)\n", location, v0 );
  ENTER_GL();
  func_glUniform1iARB( location, v0 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform1iv( GLint location, GLsizei count, GLint* value ) {
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform1iv( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform1ivARB( GLint location, GLsizei count, GLint* value ) {
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform1ivARB( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform2f( GLint location, GLfloat v0, GLfloat v1 ) {
  TRACE("(%d, %f, %f)\n", location, v0, v1 );
  ENTER_GL();
  func_glUniform2f( location, v0, v1 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform2fARB( GLint location, GLfloat v0, GLfloat v1 ) {
  TRACE("(%d, %f, %f)\n", location, v0, v1 );
  ENTER_GL();
  func_glUniform2fARB( location, v0, v1 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform2fv( GLint location, GLsizei count, GLfloat* value ) {
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform2fv( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform2fvARB( GLint location, GLsizei count, GLfloat* value ) {
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform2fvARB( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform2i( GLint location, GLint v0, GLint v1 ) {
  TRACE("(%d, %d, %d)\n", location, v0, v1 );
  ENTER_GL();
  func_glUniform2i( location, v0, v1 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform2iARB( GLint location, GLint v0, GLint v1 ) {
  TRACE("(%d, %d, %d)\n", location, v0, v1 );
  ENTER_GL();
  func_glUniform2iARB( location, v0, v1 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform2iv( GLint location, GLsizei count, GLint* value ) {
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform2iv( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform2ivARB( GLint location, GLsizei count, GLint* value ) {
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform2ivARB( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform3f( GLint location, GLfloat v0, GLfloat v1, GLfloat v2 ) {
  TRACE("(%d, %f, %f, %f)\n", location, v0, v1, v2 );
  ENTER_GL();
  func_glUniform3f( location, v0, v1, v2 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform3fARB( GLint location, GLfloat v0, GLfloat v1, GLfloat v2 ) {
  TRACE("(%d, %f, %f, %f)\n", location, v0, v1, v2 );
  ENTER_GL();
  func_glUniform3fARB( location, v0, v1, v2 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform3fv( GLint location, GLsizei count, GLfloat* value ) {
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform3fv( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform3fvARB( GLint location, GLsizei count, GLfloat* value ) {
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform3fvARB( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform3i( GLint location, GLint v0, GLint v1, GLint v2 ) {
  TRACE("(%d, %d, %d, %d)\n", location, v0, v1, v2 );
  ENTER_GL();
  func_glUniform3i( location, v0, v1, v2 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform3iARB( GLint location, GLint v0, GLint v1, GLint v2 ) {
  TRACE("(%d, %d, %d, %d)\n", location, v0, v1, v2 );
  ENTER_GL();
  func_glUniform3iARB( location, v0, v1, v2 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform3iv( GLint location, GLsizei count, GLint* value ) {
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform3iv( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform3ivARB( GLint location, GLsizei count, GLint* value ) {
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform3ivARB( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform4f( GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 ) {
  TRACE("(%d, %f, %f, %f, %f)\n", location, v0, v1, v2, v3 );
  ENTER_GL();
  func_glUniform4f( location, v0, v1, v2, v3 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform4fARB( GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 ) {
  TRACE("(%d, %f, %f, %f, %f)\n", location, v0, v1, v2, v3 );
  ENTER_GL();
  func_glUniform4fARB( location, v0, v1, v2, v3 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform4fv( GLint location, GLsizei count, GLfloat* value ) {
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform4fv( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform4fvARB( GLint location, GLsizei count, GLfloat* value ) {
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform4fvARB( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform4i( GLint location, GLint v0, GLint v1, GLint v2, GLint v3 ) {
  TRACE("(%d, %d, %d, %d, %d)\n", location, v0, v1, v2, v3 );
  ENTER_GL();
  func_glUniform4i( location, v0, v1, v2, v3 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform4iARB( GLint location, GLint v0, GLint v1, GLint v2, GLint v3 ) {
  TRACE("(%d, %d, %d, %d, %d)\n", location, v0, v1, v2, v3 );
  ENTER_GL();
  func_glUniform4iARB( location, v0, v1, v2, v3 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform4iv( GLint location, GLsizei count, GLint* value ) {
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform4iv( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform4ivARB( GLint location, GLsizei count, GLint* value ) {
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform4ivARB( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniformMatrix2fv( GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  ENTER_GL();
  func_glUniformMatrix2fv( location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniformMatrix2fvARB( GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  ENTER_GL();
  func_glUniformMatrix2fvARB( location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniformMatrix3fv( GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  ENTER_GL();
  func_glUniformMatrix3fv( location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniformMatrix3fvARB( GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  ENTER_GL();
  func_glUniformMatrix3fvARB( location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniformMatrix4fv( GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  ENTER_GL();
  func_glUniformMatrix4fv( location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniformMatrix4fvARB( GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  ENTER_GL();
  func_glUniformMatrix4fvARB( location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glUnlockArraysEXT( void ) {
  TRACE("()\n");
  ENTER_GL();
  func_glUnlockArraysEXT( );
  LEAVE_GL();
}

static GLboolean WINAPI wine_glUnmapBuffer( GLenum target ) {
  GLboolean ret_value;
  TRACE("(%d)\n", target );
  ENTER_GL();
  ret_value = func_glUnmapBuffer( target );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glUnmapBufferARB( GLenum target ) {
  GLboolean ret_value;
  TRACE("(%d)\n", target );
  ENTER_GL();
  ret_value = func_glUnmapBufferARB( target );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glUnmapObjectBufferATI( GLuint buffer ) {
  TRACE("(%d)\n", buffer );
  ENTER_GL();
  func_glUnmapObjectBufferATI( buffer );
  LEAVE_GL();
}

static void WINAPI wine_glUpdateObjectBufferATI( GLuint buffer, GLuint offset, GLsizei size, GLvoid* pointer, GLenum preserve ) {
  TRACE("(%d, %d, %d, %p, %d)\n", buffer, offset, size, pointer, preserve );
  ENTER_GL();
  func_glUpdateObjectBufferATI( buffer, offset, size, pointer, preserve );
  LEAVE_GL();
}

static void WINAPI wine_glUseProgram( GLuint program ) {
  TRACE("(%d)\n", program );
  ENTER_GL();
  func_glUseProgram( program );
  LEAVE_GL();
}

static void WINAPI wine_glUseProgramObjectARB( unsigned int programObj ) {
  TRACE("(%d)\n", programObj );
  ENTER_GL();
  func_glUseProgramObjectARB( programObj );
  LEAVE_GL();
}

static void WINAPI wine_glValidateProgram( GLuint program ) {
  TRACE("(%d)\n", program );
  ENTER_GL();
  func_glValidateProgram( program );
  LEAVE_GL();
}

static void WINAPI wine_glValidateProgramARB( unsigned int programObj ) {
  TRACE("(%d)\n", programObj );
  ENTER_GL();
  func_glValidateProgramARB( programObj );
  LEAVE_GL();
}

static void WINAPI wine_glVariantArrayObjectATI( GLuint id, GLenum type, GLsizei stride, GLuint buffer, GLuint offset ) {
  TRACE("(%d, %d, %d, %d, %d)\n", id, type, stride, buffer, offset );
  ENTER_GL();
  func_glVariantArrayObjectATI( id, type, stride, buffer, offset );
  LEAVE_GL();
}

static void WINAPI wine_glVariantPointerEXT( GLuint id, GLenum type, GLuint stride, GLvoid* addr ) {
  TRACE("(%d, %d, %d, %p)\n", id, type, stride, addr );
  ENTER_GL();
  func_glVariantPointerEXT( id, type, stride, addr );
  LEAVE_GL();
}

static void WINAPI wine_glVariantbvEXT( GLuint id, GLbyte* addr ) {
  TRACE("(%d, %p)\n", id, addr );
  ENTER_GL();
  func_glVariantbvEXT( id, addr );
  LEAVE_GL();
}

static void WINAPI wine_glVariantdvEXT( GLuint id, GLdouble* addr ) {
  TRACE("(%d, %p)\n", id, addr );
  ENTER_GL();
  func_glVariantdvEXT( id, addr );
  LEAVE_GL();
}

static void WINAPI wine_glVariantfvEXT( GLuint id, GLfloat* addr ) {
  TRACE("(%d, %p)\n", id, addr );
  ENTER_GL();
  func_glVariantfvEXT( id, addr );
  LEAVE_GL();
}

static void WINAPI wine_glVariantivEXT( GLuint id, GLint* addr ) {
  TRACE("(%d, %p)\n", id, addr );
  ENTER_GL();
  func_glVariantivEXT( id, addr );
  LEAVE_GL();
}

static void WINAPI wine_glVariantsvEXT( GLuint id, GLshort* addr ) {
  TRACE("(%d, %p)\n", id, addr );
  ENTER_GL();
  func_glVariantsvEXT( id, addr );
  LEAVE_GL();
}

static void WINAPI wine_glVariantubvEXT( GLuint id, GLubyte* addr ) {
  TRACE("(%d, %p)\n", id, addr );
  ENTER_GL();
  func_glVariantubvEXT( id, addr );
  LEAVE_GL();
}

static void WINAPI wine_glVariantuivEXT( GLuint id, GLuint* addr ) {
  TRACE("(%d, %p)\n", id, addr );
  ENTER_GL();
  func_glVariantuivEXT( id, addr );
  LEAVE_GL();
}

static void WINAPI wine_glVariantusvEXT( GLuint id, GLushort* addr ) {
  TRACE("(%d, %p)\n", id, addr );
  ENTER_GL();
  func_glVariantusvEXT( id, addr );
  LEAVE_GL();
}

static void WINAPI wine_glVertex2hNV( unsigned short x, unsigned short y ) {
  TRACE("(%d, %d)\n", x, y );
  ENTER_GL();
  func_glVertex2hNV( x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertex2hvNV( unsigned short* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glVertex2hvNV( v );
  LEAVE_GL();
}

static void WINAPI wine_glVertex3hNV( unsigned short x, unsigned short y, unsigned short z ) {
  TRACE("(%d, %d, %d)\n", x, y, z );
  ENTER_GL();
  func_glVertex3hNV( x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertex3hvNV( unsigned short* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glVertex3hvNV( v );
  LEAVE_GL();
}

static void WINAPI wine_glVertex4hNV( unsigned short x, unsigned short y, unsigned short z, unsigned short w ) {
  TRACE("(%d, %d, %d, %d)\n", x, y, z, w );
  ENTER_GL();
  func_glVertex4hNV( x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertex4hvNV( unsigned short* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glVertex4hvNV( v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexArrayParameteriAPPLE( GLenum pname, GLint param ) {
  TRACE("(%d, %d)\n", pname, param );
  ENTER_GL();
  func_glVertexArrayParameteriAPPLE( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glVertexArrayRangeAPPLE( GLsizei length, GLvoid* pointer ) {
  TRACE("(%d, %p)\n", length, pointer );
  ENTER_GL();
  func_glVertexArrayRangeAPPLE( length, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glVertexArrayRangeNV( GLsizei length, GLvoid* pointer ) {
  TRACE("(%d, %p)\n", length, pointer );
  ENTER_GL();
  func_glVertexArrayRangeNV( length, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1d( GLuint index, GLdouble x ) {
  TRACE("(%d, %f)\n", index, x );
  ENTER_GL();
  func_glVertexAttrib1d( index, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1dARB( GLuint index, GLdouble x ) {
  TRACE("(%d, %f)\n", index, x );
  ENTER_GL();
  func_glVertexAttrib1dARB( index, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1dNV( GLuint index, GLdouble x ) {
  TRACE("(%d, %f)\n", index, x );
  ENTER_GL();
  func_glVertexAttrib1dNV( index, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1dv( GLuint index, GLdouble* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib1dv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1dvARB( GLuint index, GLdouble* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib1dvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1dvNV( GLuint index, GLdouble* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib1dvNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1f( GLuint index, GLfloat x ) {
  TRACE("(%d, %f)\n", index, x );
  ENTER_GL();
  func_glVertexAttrib1f( index, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1fARB( GLuint index, GLfloat x ) {
  TRACE("(%d, %f)\n", index, x );
  ENTER_GL();
  func_glVertexAttrib1fARB( index, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1fNV( GLuint index, GLfloat x ) {
  TRACE("(%d, %f)\n", index, x );
  ENTER_GL();
  func_glVertexAttrib1fNV( index, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1fv( GLuint index, GLfloat* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib1fv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1fvARB( GLuint index, GLfloat* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib1fvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1fvNV( GLuint index, GLfloat* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib1fvNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1hNV( GLuint index, unsigned short x ) {
  TRACE("(%d, %d)\n", index, x );
  ENTER_GL();
  func_glVertexAttrib1hNV( index, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1hvNV( GLuint index, unsigned short* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib1hvNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1s( GLuint index, GLshort x ) {
  TRACE("(%d, %d)\n", index, x );
  ENTER_GL();
  func_glVertexAttrib1s( index, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1sARB( GLuint index, GLshort x ) {
  TRACE("(%d, %d)\n", index, x );
  ENTER_GL();
  func_glVertexAttrib1sARB( index, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1sNV( GLuint index, GLshort x ) {
  TRACE("(%d, %d)\n", index, x );
  ENTER_GL();
  func_glVertexAttrib1sNV( index, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1sv( GLuint index, GLshort* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib1sv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1svARB( GLuint index, GLshort* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib1svARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1svNV( GLuint index, GLshort* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib1svNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2d( GLuint index, GLdouble x, GLdouble y ) {
  TRACE("(%d, %f, %f)\n", index, x, y );
  ENTER_GL();
  func_glVertexAttrib2d( index, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2dARB( GLuint index, GLdouble x, GLdouble y ) {
  TRACE("(%d, %f, %f)\n", index, x, y );
  ENTER_GL();
  func_glVertexAttrib2dARB( index, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2dNV( GLuint index, GLdouble x, GLdouble y ) {
  TRACE("(%d, %f, %f)\n", index, x, y );
  ENTER_GL();
  func_glVertexAttrib2dNV( index, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2dv( GLuint index, GLdouble* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib2dv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2dvARB( GLuint index, GLdouble* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib2dvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2dvNV( GLuint index, GLdouble* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib2dvNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2f( GLuint index, GLfloat x, GLfloat y ) {
  TRACE("(%d, %f, %f)\n", index, x, y );
  ENTER_GL();
  func_glVertexAttrib2f( index, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2fARB( GLuint index, GLfloat x, GLfloat y ) {
  TRACE("(%d, %f, %f)\n", index, x, y );
  ENTER_GL();
  func_glVertexAttrib2fARB( index, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2fNV( GLuint index, GLfloat x, GLfloat y ) {
  TRACE("(%d, %f, %f)\n", index, x, y );
  ENTER_GL();
  func_glVertexAttrib2fNV( index, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2fv( GLuint index, GLfloat* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib2fv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2fvARB( GLuint index, GLfloat* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib2fvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2fvNV( GLuint index, GLfloat* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib2fvNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2hNV( GLuint index, unsigned short x, unsigned short y ) {
  TRACE("(%d, %d, %d)\n", index, x, y );
  ENTER_GL();
  func_glVertexAttrib2hNV( index, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2hvNV( GLuint index, unsigned short* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib2hvNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2s( GLuint index, GLshort x, GLshort y ) {
  TRACE("(%d, %d, %d)\n", index, x, y );
  ENTER_GL();
  func_glVertexAttrib2s( index, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2sARB( GLuint index, GLshort x, GLshort y ) {
  TRACE("(%d, %d, %d)\n", index, x, y );
  ENTER_GL();
  func_glVertexAttrib2sARB( index, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2sNV( GLuint index, GLshort x, GLshort y ) {
  TRACE("(%d, %d, %d)\n", index, x, y );
  ENTER_GL();
  func_glVertexAttrib2sNV( index, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2sv( GLuint index, GLshort* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib2sv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2svARB( GLuint index, GLshort* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib2svARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2svNV( GLuint index, GLshort* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib2svNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3d( GLuint index, GLdouble x, GLdouble y, GLdouble z ) {
  TRACE("(%d, %f, %f, %f)\n", index, x, y, z );
  ENTER_GL();
  func_glVertexAttrib3d( index, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3dARB( GLuint index, GLdouble x, GLdouble y, GLdouble z ) {
  TRACE("(%d, %f, %f, %f)\n", index, x, y, z );
  ENTER_GL();
  func_glVertexAttrib3dARB( index, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3dNV( GLuint index, GLdouble x, GLdouble y, GLdouble z ) {
  TRACE("(%d, %f, %f, %f)\n", index, x, y, z );
  ENTER_GL();
  func_glVertexAttrib3dNV( index, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3dv( GLuint index, GLdouble* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib3dv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3dvARB( GLuint index, GLdouble* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib3dvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3dvNV( GLuint index, GLdouble* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib3dvNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3f( GLuint index, GLfloat x, GLfloat y, GLfloat z ) {
  TRACE("(%d, %f, %f, %f)\n", index, x, y, z );
  ENTER_GL();
  func_glVertexAttrib3f( index, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3fARB( GLuint index, GLfloat x, GLfloat y, GLfloat z ) {
  TRACE("(%d, %f, %f, %f)\n", index, x, y, z );
  ENTER_GL();
  func_glVertexAttrib3fARB( index, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3fNV( GLuint index, GLfloat x, GLfloat y, GLfloat z ) {
  TRACE("(%d, %f, %f, %f)\n", index, x, y, z );
  ENTER_GL();
  func_glVertexAttrib3fNV( index, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3fv( GLuint index, GLfloat* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib3fv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3fvARB( GLuint index, GLfloat* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib3fvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3fvNV( GLuint index, GLfloat* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib3fvNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3hNV( GLuint index, unsigned short x, unsigned short y, unsigned short z ) {
  TRACE("(%d, %d, %d, %d)\n", index, x, y, z );
  ENTER_GL();
  func_glVertexAttrib3hNV( index, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3hvNV( GLuint index, unsigned short* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib3hvNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3s( GLuint index, GLshort x, GLshort y, GLshort z ) {
  TRACE("(%d, %d, %d, %d)\n", index, x, y, z );
  ENTER_GL();
  func_glVertexAttrib3s( index, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3sARB( GLuint index, GLshort x, GLshort y, GLshort z ) {
  TRACE("(%d, %d, %d, %d)\n", index, x, y, z );
  ENTER_GL();
  func_glVertexAttrib3sARB( index, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3sNV( GLuint index, GLshort x, GLshort y, GLshort z ) {
  TRACE("(%d, %d, %d, %d)\n", index, x, y, z );
  ENTER_GL();
  func_glVertexAttrib3sNV( index, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3sv( GLuint index, GLshort* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib3sv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3svARB( GLuint index, GLshort* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib3svARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3svNV( GLuint index, GLshort* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib3svNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4Nbv( GLuint index, GLbyte* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4Nbv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4NbvARB( GLuint index, GLbyte* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4NbvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4Niv( GLuint index, GLint* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4Niv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4NivARB( GLuint index, GLint* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4NivARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4Nsv( GLuint index, GLshort* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4Nsv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4NsvARB( GLuint index, GLshort* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4NsvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4Nub( GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w ) {
  TRACE("(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttrib4Nub( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4NubARB( GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w ) {
  TRACE("(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttrib4NubARB( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4Nubv( GLuint index, GLubyte* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4Nubv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4NubvARB( GLuint index, GLubyte* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4NubvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4Nuiv( GLuint index, GLuint* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4Nuiv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4NuivARB( GLuint index, GLuint* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4NuivARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4Nusv( GLuint index, GLushort* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4Nusv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4NusvARB( GLuint index, GLushort* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4NusvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4bv( GLuint index, GLbyte* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4bv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4bvARB( GLuint index, GLbyte* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4bvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4d( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  TRACE("(%d, %f, %f, %f, %f)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttrib4d( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4dARB( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  TRACE("(%d, %f, %f, %f, %f)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttrib4dARB( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4dNV( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  TRACE("(%d, %f, %f, %f, %f)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttrib4dNV( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4dv( GLuint index, GLdouble* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4dv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4dvARB( GLuint index, GLdouble* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4dvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4dvNV( GLuint index, GLdouble* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4dvNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4f( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  TRACE("(%d, %f, %f, %f, %f)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttrib4f( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4fARB( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  TRACE("(%d, %f, %f, %f, %f)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttrib4fARB( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4fNV( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  TRACE("(%d, %f, %f, %f, %f)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttrib4fNV( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4fv( GLuint index, GLfloat* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4fv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4fvARB( GLuint index, GLfloat* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4fvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4fvNV( GLuint index, GLfloat* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4fvNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4hNV( GLuint index, unsigned short x, unsigned short y, unsigned short z, unsigned short w ) {
  TRACE("(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttrib4hNV( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4hvNV( GLuint index, unsigned short* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4hvNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4iv( GLuint index, GLint* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4iv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4ivARB( GLuint index, GLint* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4ivARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4s( GLuint index, GLshort x, GLshort y, GLshort z, GLshort w ) {
  TRACE("(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttrib4s( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4sARB( GLuint index, GLshort x, GLshort y, GLshort z, GLshort w ) {
  TRACE("(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttrib4sARB( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4sNV( GLuint index, GLshort x, GLshort y, GLshort z, GLshort w ) {
  TRACE("(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttrib4sNV( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4sv( GLuint index, GLshort* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4sv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4svARB( GLuint index, GLshort* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4svARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4svNV( GLuint index, GLshort* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4svNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4ubNV( GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w ) {
  TRACE("(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttrib4ubNV( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4ubv( GLuint index, GLubyte* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4ubv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4ubvARB( GLuint index, GLubyte* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4ubvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4ubvNV( GLuint index, GLubyte* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4ubvNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4uiv( GLuint index, GLuint* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4uiv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4uivARB( GLuint index, GLuint* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4uivARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4usv( GLuint index, GLushort* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4usv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4usvARB( GLuint index, GLushort* v ) {
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4usvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribArrayObjectATI( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, GLuint buffer, GLuint offset ) {
  TRACE("(%d, %d, %d, %d, %d, %d, %d)\n", index, size, type, normalized, stride, buffer, offset );
  ENTER_GL();
  func_glVertexAttribArrayObjectATI( index, size, type, normalized, stride, buffer, offset );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribPointer( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, GLvoid* pointer ) {
  TRACE("(%d, %d, %d, %d, %d, %p)\n", index, size, type, normalized, stride, pointer );
  ENTER_GL();
  func_glVertexAttribPointer( index, size, type, normalized, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribPointerARB( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, GLvoid* pointer ) {
  TRACE("(%d, %d, %d, %d, %d, %p)\n", index, size, type, normalized, stride, pointer );
  ENTER_GL();
  func_glVertexAttribPointerARB( index, size, type, normalized, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribPointerNV( GLuint index, GLint fsize, GLenum type, GLsizei stride, GLvoid* pointer ) {
  TRACE("(%d, %d, %d, %d, %p)\n", index, fsize, type, stride, pointer );
  ENTER_GL();
  func_glVertexAttribPointerNV( index, fsize, type, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs1dvNV( GLuint index, GLsizei count, GLdouble* v ) {
  TRACE("(%d, %d, %p)\n", index, count, v );
  ENTER_GL();
  func_glVertexAttribs1dvNV( index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs1fvNV( GLuint index, GLsizei count, GLfloat* v ) {
  TRACE("(%d, %d, %p)\n", index, count, v );
  ENTER_GL();
  func_glVertexAttribs1fvNV( index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs1hvNV( GLuint index, GLsizei n, unsigned short* v ) {
  TRACE("(%d, %d, %p)\n", index, n, v );
  ENTER_GL();
  func_glVertexAttribs1hvNV( index, n, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs1svNV( GLuint index, GLsizei count, GLshort* v ) {
  TRACE("(%d, %d, %p)\n", index, count, v );
  ENTER_GL();
  func_glVertexAttribs1svNV( index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs2dvNV( GLuint index, GLsizei count, GLdouble* v ) {
  TRACE("(%d, %d, %p)\n", index, count, v );
  ENTER_GL();
  func_glVertexAttribs2dvNV( index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs2fvNV( GLuint index, GLsizei count, GLfloat* v ) {
  TRACE("(%d, %d, %p)\n", index, count, v );
  ENTER_GL();
  func_glVertexAttribs2fvNV( index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs2hvNV( GLuint index, GLsizei n, unsigned short* v ) {
  TRACE("(%d, %d, %p)\n", index, n, v );
  ENTER_GL();
  func_glVertexAttribs2hvNV( index, n, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs2svNV( GLuint index, GLsizei count, GLshort* v ) {
  TRACE("(%d, %d, %p)\n", index, count, v );
  ENTER_GL();
  func_glVertexAttribs2svNV( index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs3dvNV( GLuint index, GLsizei count, GLdouble* v ) {
  TRACE("(%d, %d, %p)\n", index, count, v );
  ENTER_GL();
  func_glVertexAttribs3dvNV( index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs3fvNV( GLuint index, GLsizei count, GLfloat* v ) {
  TRACE("(%d, %d, %p)\n", index, count, v );
  ENTER_GL();
  func_glVertexAttribs3fvNV( index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs3hvNV( GLuint index, GLsizei n, unsigned short* v ) {
  TRACE("(%d, %d, %p)\n", index, n, v );
  ENTER_GL();
  func_glVertexAttribs3hvNV( index, n, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs3svNV( GLuint index, GLsizei count, GLshort* v ) {
  TRACE("(%d, %d, %p)\n", index, count, v );
  ENTER_GL();
  func_glVertexAttribs3svNV( index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs4dvNV( GLuint index, GLsizei count, GLdouble* v ) {
  TRACE("(%d, %d, %p)\n", index, count, v );
  ENTER_GL();
  func_glVertexAttribs4dvNV( index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs4fvNV( GLuint index, GLsizei count, GLfloat* v ) {
  TRACE("(%d, %d, %p)\n", index, count, v );
  ENTER_GL();
  func_glVertexAttribs4fvNV( index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs4hvNV( GLuint index, GLsizei n, unsigned short* v ) {
  TRACE("(%d, %d, %p)\n", index, n, v );
  ENTER_GL();
  func_glVertexAttribs4hvNV( index, n, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs4svNV( GLuint index, GLsizei count, GLshort* v ) {
  TRACE("(%d, %d, %p)\n", index, count, v );
  ENTER_GL();
  func_glVertexAttribs4svNV( index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs4ubvNV( GLuint index, GLsizei count, GLubyte* v ) {
  TRACE("(%d, %d, %p)\n", index, count, v );
  ENTER_GL();
  func_glVertexAttribs4ubvNV( index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexBlendARB( GLint count ) {
  TRACE("(%d)\n", count );
  ENTER_GL();
  func_glVertexBlendARB( count );
  LEAVE_GL();
}

static void WINAPI wine_glVertexBlendEnvfATI( GLenum pname, GLfloat param ) {
  TRACE("(%d, %f)\n", pname, param );
  ENTER_GL();
  func_glVertexBlendEnvfATI( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glVertexBlendEnviATI( GLenum pname, GLint param ) {
  TRACE("(%d, %d)\n", pname, param );
  ENTER_GL();
  func_glVertexBlendEnviATI( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glVertexPointerEXT( GLint size, GLenum type, GLsizei stride, GLsizei count, GLvoid* pointer ) {
  TRACE("(%d, %d, %d, %d, %p)\n", size, type, stride, count, pointer );
  ENTER_GL();
  func_glVertexPointerEXT( size, type, stride, count, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glVertexPointerListIBM( GLint size, GLenum type, GLint stride, GLvoid** pointer, GLint ptrstride ) {
  TRACE("(%d, %d, %d, %p, %d)\n", size, type, stride, pointer, ptrstride );
  ENTER_GL();
  func_glVertexPointerListIBM( size, type, stride, pointer, ptrstride );
  LEAVE_GL();
}

static void WINAPI wine_glVertexPointervINTEL( GLint size, GLenum type, GLvoid** pointer ) {
  TRACE("(%d, %d, %p)\n", size, type, pointer );
  ENTER_GL();
  func_glVertexPointervINTEL( size, type, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream1dATI( GLenum stream, GLdouble x ) {
  TRACE("(%d, %f)\n", stream, x );
  ENTER_GL();
  func_glVertexStream1dATI( stream, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream1dvATI( GLenum stream, GLdouble* coords ) {
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream1dvATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream1fATI( GLenum stream, GLfloat x ) {
  TRACE("(%d, %f)\n", stream, x );
  ENTER_GL();
  func_glVertexStream1fATI( stream, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream1fvATI( GLenum stream, GLfloat* coords ) {
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream1fvATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream1iATI( GLenum stream, GLint x ) {
  TRACE("(%d, %d)\n", stream, x );
  ENTER_GL();
  func_glVertexStream1iATI( stream, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream1ivATI( GLenum stream, GLint* coords ) {
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream1ivATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream1sATI( GLenum stream, GLshort x ) {
  TRACE("(%d, %d)\n", stream, x );
  ENTER_GL();
  func_glVertexStream1sATI( stream, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream1svATI( GLenum stream, GLshort* coords ) {
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream1svATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream2dATI( GLenum stream, GLdouble x, GLdouble y ) {
  TRACE("(%d, %f, %f)\n", stream, x, y );
  ENTER_GL();
  func_glVertexStream2dATI( stream, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream2dvATI( GLenum stream, GLdouble* coords ) {
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream2dvATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream2fATI( GLenum stream, GLfloat x, GLfloat y ) {
  TRACE("(%d, %f, %f)\n", stream, x, y );
  ENTER_GL();
  func_glVertexStream2fATI( stream, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream2fvATI( GLenum stream, GLfloat* coords ) {
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream2fvATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream2iATI( GLenum stream, GLint x, GLint y ) {
  TRACE("(%d, %d, %d)\n", stream, x, y );
  ENTER_GL();
  func_glVertexStream2iATI( stream, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream2ivATI( GLenum stream, GLint* coords ) {
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream2ivATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream2sATI( GLenum stream, GLshort x, GLshort y ) {
  TRACE("(%d, %d, %d)\n", stream, x, y );
  ENTER_GL();
  func_glVertexStream2sATI( stream, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream2svATI( GLenum stream, GLshort* coords ) {
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream2svATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream3dATI( GLenum stream, GLdouble x, GLdouble y, GLdouble z ) {
  TRACE("(%d, %f, %f, %f)\n", stream, x, y, z );
  ENTER_GL();
  func_glVertexStream3dATI( stream, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream3dvATI( GLenum stream, GLdouble* coords ) {
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream3dvATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream3fATI( GLenum stream, GLfloat x, GLfloat y, GLfloat z ) {
  TRACE("(%d, %f, %f, %f)\n", stream, x, y, z );
  ENTER_GL();
  func_glVertexStream3fATI( stream, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream3fvATI( GLenum stream, GLfloat* coords ) {
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream3fvATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream3iATI( GLenum stream, GLint x, GLint y, GLint z ) {
  TRACE("(%d, %d, %d, %d)\n", stream, x, y, z );
  ENTER_GL();
  func_glVertexStream3iATI( stream, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream3ivATI( GLenum stream, GLint* coords ) {
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream3ivATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream3sATI( GLenum stream, GLshort x, GLshort y, GLshort z ) {
  TRACE("(%d, %d, %d, %d)\n", stream, x, y, z );
  ENTER_GL();
  func_glVertexStream3sATI( stream, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream3svATI( GLenum stream, GLshort* coords ) {
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream3svATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream4dATI( GLenum stream, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  TRACE("(%d, %f, %f, %f, %f)\n", stream, x, y, z, w );
  ENTER_GL();
  func_glVertexStream4dATI( stream, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream4dvATI( GLenum stream, GLdouble* coords ) {
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream4dvATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream4fATI( GLenum stream, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  TRACE("(%d, %f, %f, %f, %f)\n", stream, x, y, z, w );
  ENTER_GL();
  func_glVertexStream4fATI( stream, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream4fvATI( GLenum stream, GLfloat* coords ) {
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream4fvATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream4iATI( GLenum stream, GLint x, GLint y, GLint z, GLint w ) {
  TRACE("(%d, %d, %d, %d, %d)\n", stream, x, y, z, w );
  ENTER_GL();
  func_glVertexStream4iATI( stream, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream4ivATI( GLenum stream, GLint* coords ) {
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream4ivATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream4sATI( GLenum stream, GLshort x, GLshort y, GLshort z, GLshort w ) {
  TRACE("(%d, %d, %d, %d, %d)\n", stream, x, y, z, w );
  ENTER_GL();
  func_glVertexStream4sATI( stream, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream4svATI( GLenum stream, GLshort* coords ) {
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream4svATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexWeightPointerEXT( GLsizei size, GLenum type, GLsizei stride, GLvoid* pointer ) {
  TRACE("(%d, %d, %d, %p)\n", size, type, stride, pointer );
  ENTER_GL();
  func_glVertexWeightPointerEXT( size, type, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glVertexWeightfEXT( GLfloat weight ) {
  TRACE("(%f)\n", weight );
  ENTER_GL();
  func_glVertexWeightfEXT( weight );
  LEAVE_GL();
}

static void WINAPI wine_glVertexWeightfvEXT( GLfloat* weight ) {
  TRACE("(%p)\n", weight );
  ENTER_GL();
  func_glVertexWeightfvEXT( weight );
  LEAVE_GL();
}

static void WINAPI wine_glVertexWeighthNV( unsigned short weight ) {
  TRACE("(%d)\n", weight );
  ENTER_GL();
  func_glVertexWeighthNV( weight );
  LEAVE_GL();
}

static void WINAPI wine_glVertexWeighthvNV( unsigned short* weight ) {
  TRACE("(%p)\n", weight );
  ENTER_GL();
  func_glVertexWeighthvNV( weight );
  LEAVE_GL();
}

static void WINAPI wine_glWeightPointerARB( GLint size, GLenum type, GLsizei stride, GLvoid* pointer ) {
  TRACE("(%d, %d, %d, %p)\n", size, type, stride, pointer );
  ENTER_GL();
  func_glWeightPointerARB( size, type, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glWeightbvARB( GLint size, GLbyte* weights ) {
  TRACE("(%d, %p)\n", size, weights );
  ENTER_GL();
  func_glWeightbvARB( size, weights );
  LEAVE_GL();
}

static void WINAPI wine_glWeightdvARB( GLint size, GLdouble* weights ) {
  TRACE("(%d, %p)\n", size, weights );
  ENTER_GL();
  func_glWeightdvARB( size, weights );
  LEAVE_GL();
}

static void WINAPI wine_glWeightfvARB( GLint size, GLfloat* weights ) {
  TRACE("(%d, %p)\n", size, weights );
  ENTER_GL();
  func_glWeightfvARB( size, weights );
  LEAVE_GL();
}

static void WINAPI wine_glWeightivARB( GLint size, GLint* weights ) {
  TRACE("(%d, %p)\n", size, weights );
  ENTER_GL();
  func_glWeightivARB( size, weights );
  LEAVE_GL();
}

static void WINAPI wine_glWeightsvARB( GLint size, GLshort* weights ) {
  TRACE("(%d, %p)\n", size, weights );
  ENTER_GL();
  func_glWeightsvARB( size, weights );
  LEAVE_GL();
}

static void WINAPI wine_glWeightubvARB( GLint size, GLubyte* weights ) {
  TRACE("(%d, %p)\n", size, weights );
  ENTER_GL();
  func_glWeightubvARB( size, weights );
  LEAVE_GL();
}

static void WINAPI wine_glWeightuivARB( GLint size, GLuint* weights ) {
  TRACE("(%d, %p)\n", size, weights );
  ENTER_GL();
  func_glWeightuivARB( size, weights );
  LEAVE_GL();
}

static void WINAPI wine_glWeightusvARB( GLint size, GLushort* weights ) {
  TRACE("(%d, %p)\n", size, weights );
  ENTER_GL();
  func_glWeightusvARB( size, weights );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2d( GLdouble x, GLdouble y ) {
  TRACE("(%f, %f)\n", x, y );
  ENTER_GL();
  func_glWindowPos2d( x, y );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2dARB( GLdouble x, GLdouble y ) {
  TRACE("(%f, %f)\n", x, y );
  ENTER_GL();
  func_glWindowPos2dARB( x, y );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2dMESA( GLdouble x, GLdouble y ) {
  TRACE("(%f, %f)\n", x, y );
  ENTER_GL();
  func_glWindowPos2dMESA( x, y );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2dv( GLdouble* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos2dv( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2dvARB( GLdouble* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos2dvARB( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2dvMESA( GLdouble* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos2dvMESA( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2f( GLfloat x, GLfloat y ) {
  TRACE("(%f, %f)\n", x, y );
  ENTER_GL();
  func_glWindowPos2f( x, y );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2fARB( GLfloat x, GLfloat y ) {
  TRACE("(%f, %f)\n", x, y );
  ENTER_GL();
  func_glWindowPos2fARB( x, y );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2fMESA( GLfloat x, GLfloat y ) {
  TRACE("(%f, %f)\n", x, y );
  ENTER_GL();
  func_glWindowPos2fMESA( x, y );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2fv( GLfloat* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos2fv( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2fvARB( GLfloat* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos2fvARB( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2fvMESA( GLfloat* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos2fvMESA( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2i( GLint x, GLint y ) {
  TRACE("(%d, %d)\n", x, y );
  ENTER_GL();
  func_glWindowPos2i( x, y );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2iARB( GLint x, GLint y ) {
  TRACE("(%d, %d)\n", x, y );
  ENTER_GL();
  func_glWindowPos2iARB( x, y );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2iMESA( GLint x, GLint y ) {
  TRACE("(%d, %d)\n", x, y );
  ENTER_GL();
  func_glWindowPos2iMESA( x, y );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2iv( GLint* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos2iv( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2ivARB( GLint* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos2ivARB( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2ivMESA( GLint* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos2ivMESA( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2s( GLshort x, GLshort y ) {
  TRACE("(%d, %d)\n", x, y );
  ENTER_GL();
  func_glWindowPos2s( x, y );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2sARB( GLshort x, GLshort y ) {
  TRACE("(%d, %d)\n", x, y );
  ENTER_GL();
  func_glWindowPos2sARB( x, y );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2sMESA( GLshort x, GLshort y ) {
  TRACE("(%d, %d)\n", x, y );
  ENTER_GL();
  func_glWindowPos2sMESA( x, y );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2sv( GLshort* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos2sv( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2svARB( GLshort* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos2svARB( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2svMESA( GLshort* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos2svMESA( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3d( GLdouble x, GLdouble y, GLdouble z ) {
  TRACE("(%f, %f, %f)\n", x, y, z );
  ENTER_GL();
  func_glWindowPos3d( x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3dARB( GLdouble x, GLdouble y, GLdouble z ) {
  TRACE("(%f, %f, %f)\n", x, y, z );
  ENTER_GL();
  func_glWindowPos3dARB( x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3dMESA( GLdouble x, GLdouble y, GLdouble z ) {
  TRACE("(%f, %f, %f)\n", x, y, z );
  ENTER_GL();
  func_glWindowPos3dMESA( x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3dv( GLdouble* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos3dv( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3dvARB( GLdouble* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos3dvARB( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3dvMESA( GLdouble* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos3dvMESA( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3f( GLfloat x, GLfloat y, GLfloat z ) {
  TRACE("(%f, %f, %f)\n", x, y, z );
  ENTER_GL();
  func_glWindowPos3f( x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3fARB( GLfloat x, GLfloat y, GLfloat z ) {
  TRACE("(%f, %f, %f)\n", x, y, z );
  ENTER_GL();
  func_glWindowPos3fARB( x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3fMESA( GLfloat x, GLfloat y, GLfloat z ) {
  TRACE("(%f, %f, %f)\n", x, y, z );
  ENTER_GL();
  func_glWindowPos3fMESA( x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3fv( GLfloat* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos3fv( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3fvARB( GLfloat* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos3fvARB( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3fvMESA( GLfloat* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos3fvMESA( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3i( GLint x, GLint y, GLint z ) {
  TRACE("(%d, %d, %d)\n", x, y, z );
  ENTER_GL();
  func_glWindowPos3i( x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3iARB( GLint x, GLint y, GLint z ) {
  TRACE("(%d, %d, %d)\n", x, y, z );
  ENTER_GL();
  func_glWindowPos3iARB( x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3iMESA( GLint x, GLint y, GLint z ) {
  TRACE("(%d, %d, %d)\n", x, y, z );
  ENTER_GL();
  func_glWindowPos3iMESA( x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3iv( GLint* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos3iv( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3ivARB( GLint* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos3ivARB( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3ivMESA( GLint* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos3ivMESA( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3s( GLshort x, GLshort y, GLshort z ) {
  TRACE("(%d, %d, %d)\n", x, y, z );
  ENTER_GL();
  func_glWindowPos3s( x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3sARB( GLshort x, GLshort y, GLshort z ) {
  TRACE("(%d, %d, %d)\n", x, y, z );
  ENTER_GL();
  func_glWindowPos3sARB( x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3sMESA( GLshort x, GLshort y, GLshort z ) {
  TRACE("(%d, %d, %d)\n", x, y, z );
  ENTER_GL();
  func_glWindowPos3sMESA( x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3sv( GLshort* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos3sv( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3svARB( GLshort* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos3svARB( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3svMESA( GLshort* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos3svMESA( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos4dMESA( GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  TRACE("(%f, %f, %f, %f)\n", x, y, z, w );
  ENTER_GL();
  func_glWindowPos4dMESA( x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos4dvMESA( GLdouble* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos4dvMESA( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos4fMESA( GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  TRACE("(%f, %f, %f, %f)\n", x, y, z, w );
  ENTER_GL();
  func_glWindowPos4fMESA( x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos4fvMESA( GLfloat* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos4fvMESA( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos4iMESA( GLint x, GLint y, GLint z, GLint w ) {
  TRACE("(%d, %d, %d, %d)\n", x, y, z, w );
  ENTER_GL();
  func_glWindowPos4iMESA( x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos4ivMESA( GLint* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos4ivMESA( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos4sMESA( GLshort x, GLshort y, GLshort z, GLshort w ) {
  TRACE("(%d, %d, %d, %d)\n", x, y, z, w );
  ENTER_GL();
  func_glWindowPos4sMESA( x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos4svMESA( GLshort* v ) {
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos4svMESA( v );
  LEAVE_GL();
}

static void WINAPI wine_glWriteMaskEXT( GLuint res, GLuint in, GLenum outX, GLenum outY, GLenum outZ, GLenum outW ) {
  TRACE("(%d, %d, %d, %d, %d, %d)\n", res, in, outX, outY, outZ, outW );
  ENTER_GL();
  func_glWriteMaskEXT( res, in, outX, outY, outZ, outW );
  LEAVE_GL();
}

static void * WINAPI wine_wglAllocateMemoryNV( GLsizei size, GLfloat readfreq, GLfloat writefreq, GLfloat priority ) {
  void * ret_value;
  TRACE("(%d, %f, %f, %f)\n", size, readfreq, writefreq, priority );
  ENTER_GL();
  ret_value = func_wglAllocateMemoryNV( size, readfreq, writefreq, priority );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_wglFreeMemoryNV( GLvoid * pointer ) {
  TRACE("(%p)\n", pointer );
  ENTER_GL();
  func_wglFreeMemoryNV( pointer );
  LEAVE_GL();
}


/* The table giving the correspondance between names and functions */
const int extension_registry_size = 1093;
const OpenGL_extension extension_registry[1093] = {
  { "glActiveStencilFaceEXT", NULL, (void *) wine_glActiveStencilFaceEXT, (void **) &func_glActiveStencilFaceEXT },
  { "glActiveTexture", NULL, (void *) wine_glActiveTexture, (void **) &func_glActiveTexture },
  { "glActiveTextureARB", NULL, (void *) wine_glActiveTextureARB, (void **) &func_glActiveTextureARB },
  { "glAlphaFragmentOp1ATI", NULL, (void *) wine_glAlphaFragmentOp1ATI, (void **) &func_glAlphaFragmentOp1ATI },
  { "glAlphaFragmentOp2ATI", NULL, (void *) wine_glAlphaFragmentOp2ATI, (void **) &func_glAlphaFragmentOp2ATI },
  { "glAlphaFragmentOp3ATI", NULL, (void *) wine_glAlphaFragmentOp3ATI, (void **) &func_glAlphaFragmentOp3ATI },
  { "glApplyTextureEXT", NULL, (void *) wine_glApplyTextureEXT, (void **) &func_glApplyTextureEXT },
  { "glAreProgramsResidentNV", NULL, (void *) wine_glAreProgramsResidentNV, (void **) &func_glAreProgramsResidentNV },
  { "glAreTexturesResidentEXT", NULL, (void *) wine_glAreTexturesResidentEXT, (void **) &func_glAreTexturesResidentEXT },
  { "glArrayElementEXT", NULL, (void *) wine_glArrayElementEXT, (void **) &func_glArrayElementEXT },
  { "glArrayObjectATI", NULL, (void *) wine_glArrayObjectATI, (void **) &func_glArrayObjectATI },
  { "glAsyncMarkerSGIX", NULL, (void *) wine_glAsyncMarkerSGIX, (void **) &func_glAsyncMarkerSGIX },
  { "glAttachObjectARB", NULL, (void *) wine_glAttachObjectARB, (void **) &func_glAttachObjectARB },
  { "glAttachShader", NULL, (void *) wine_glAttachShader, (void **) &func_glAttachShader },
  { "glBeginFragmentShaderATI", NULL, (void *) wine_glBeginFragmentShaderATI, (void **) &func_glBeginFragmentShaderATI },
  { "glBeginOcclusionQueryNV", NULL, (void *) wine_glBeginOcclusionQueryNV, (void **) &func_glBeginOcclusionQueryNV },
  { "glBeginQuery", NULL, (void *) wine_glBeginQuery, (void **) &func_glBeginQuery },
  { "glBeginQueryARB", NULL, (void *) wine_glBeginQueryARB, (void **) &func_glBeginQueryARB },
  { "glBeginVertexShaderEXT", NULL, (void *) wine_glBeginVertexShaderEXT, (void **) &func_glBeginVertexShaderEXT },
  { "glBindAttribLocation", NULL, (void *) wine_glBindAttribLocation, (void **) &func_glBindAttribLocation },
  { "glBindAttribLocationARB", NULL, (void *) wine_glBindAttribLocationARB, (void **) &func_glBindAttribLocationARB },
  { "glBindBuffer", NULL, (void *) wine_glBindBuffer, (void **) &func_glBindBuffer },
  { "glBindBufferARB", NULL, (void *) wine_glBindBufferARB, (void **) &func_glBindBufferARB },
  { "glBindFragmentShaderATI", NULL, (void *) wine_glBindFragmentShaderATI, (void **) &func_glBindFragmentShaderATI },
  { "glBindFramebufferEXT", NULL, (void *) wine_glBindFramebufferEXT, (void **) &func_glBindFramebufferEXT },
  { "glBindLightParameterEXT", NULL, (void *) wine_glBindLightParameterEXT, (void **) &func_glBindLightParameterEXT },
  { "glBindMaterialParameterEXT", NULL, (void *) wine_glBindMaterialParameterEXT, (void **) &func_glBindMaterialParameterEXT },
  { "glBindParameterEXT", NULL, (void *) wine_glBindParameterEXT, (void **) &func_glBindParameterEXT },
  { "glBindProgramARB", NULL, (void *) wine_glBindProgramARB, (void **) &func_glBindProgramARB },
  { "glBindProgramNV", NULL, (void *) wine_glBindProgramNV, (void **) &func_glBindProgramNV },
  { "glBindRenderbufferEXT", NULL, (void *) wine_glBindRenderbufferEXT, (void **) &func_glBindRenderbufferEXT },
  { "glBindTexGenParameterEXT", NULL, (void *) wine_glBindTexGenParameterEXT, (void **) &func_glBindTexGenParameterEXT },
  { "glBindTextureEXT", NULL, (void *) wine_glBindTextureEXT, (void **) &func_glBindTextureEXT },
  { "glBindTextureUnitParameterEXT", NULL, (void *) wine_glBindTextureUnitParameterEXT, (void **) &func_glBindTextureUnitParameterEXT },
  { "glBindVertexArrayAPPLE", NULL, (void *) wine_glBindVertexArrayAPPLE, (void **) &func_glBindVertexArrayAPPLE },
  { "glBindVertexShaderEXT", NULL, (void *) wine_glBindVertexShaderEXT, (void **) &func_glBindVertexShaderEXT },
  { "glBinormal3bEXT", NULL, (void *) wine_glBinormal3bEXT, (void **) &func_glBinormal3bEXT },
  { "glBinormal3bvEXT", NULL, (void *) wine_glBinormal3bvEXT, (void **) &func_glBinormal3bvEXT },
  { "glBinormal3dEXT", NULL, (void *) wine_glBinormal3dEXT, (void **) &func_glBinormal3dEXT },
  { "glBinormal3dvEXT", NULL, (void *) wine_glBinormal3dvEXT, (void **) &func_glBinormal3dvEXT },
  { "glBinormal3fEXT", NULL, (void *) wine_glBinormal3fEXT, (void **) &func_glBinormal3fEXT },
  { "glBinormal3fvEXT", NULL, (void *) wine_glBinormal3fvEXT, (void **) &func_glBinormal3fvEXT },
  { "glBinormal3iEXT", NULL, (void *) wine_glBinormal3iEXT, (void **) &func_glBinormal3iEXT },
  { "glBinormal3ivEXT", NULL, (void *) wine_glBinormal3ivEXT, (void **) &func_glBinormal3ivEXT },
  { "glBinormal3sEXT", NULL, (void *) wine_glBinormal3sEXT, (void **) &func_glBinormal3sEXT },
  { "glBinormal3svEXT", NULL, (void *) wine_glBinormal3svEXT, (void **) &func_glBinormal3svEXT },
  { "glBinormalPointerEXT", NULL, (void *) wine_glBinormalPointerEXT, (void **) &func_glBinormalPointerEXT },
  { "glBlendColorEXT", NULL, (void *) wine_glBlendColorEXT, (void **) &func_glBlendColorEXT },
  { "glBlendEquationEXT", NULL, (void *) wine_glBlendEquationEXT, (void **) &func_glBlendEquationEXT },
  { "glBlendEquationSeparate", NULL, (void *) wine_glBlendEquationSeparate, (void **) &func_glBlendEquationSeparate },
  { "glBlendEquationSeparateEXT", NULL, (void *) wine_glBlendEquationSeparateEXT, (void **) &func_glBlendEquationSeparateEXT },
  { "glBlendFuncSeparate", NULL, (void *) wine_glBlendFuncSeparate, (void **) &func_glBlendFuncSeparate },
  { "glBlendFuncSeparateEXT", NULL, (void *) wine_glBlendFuncSeparateEXT, (void **) &func_glBlendFuncSeparateEXT },
  { "glBlendFuncSeparateINGR", NULL, (void *) wine_glBlendFuncSeparateINGR, (void **) &func_glBlendFuncSeparateINGR },
  { "glBufferData", NULL, (void *) wine_glBufferData, (void **) &func_glBufferData },
  { "glBufferDataARB", NULL, (void *) wine_glBufferDataARB, (void **) &func_glBufferDataARB },
  { "glBufferRegionEnabled", NULL, (void *) wine_glBufferRegionEnabled, (void **) &func_glBufferRegionEnabled },
  { "glBufferSubData", NULL, (void *) wine_glBufferSubData, (void **) &func_glBufferSubData },
  { "glBufferSubDataARB", NULL, (void *) wine_glBufferSubDataARB, (void **) &func_glBufferSubDataARB },
  { "glCheckFramebufferStatusEXT", NULL, (void *) wine_glCheckFramebufferStatusEXT, (void **) &func_glCheckFramebufferStatusEXT },
  { "glClampColorARB", NULL, (void *) wine_glClampColorARB, (void **) &func_glClampColorARB },
  { "glClientActiveTexture", NULL, (void *) wine_glClientActiveTexture, (void **) &func_glClientActiveTexture },
  { "glClientActiveTextureARB", NULL, (void *) wine_glClientActiveTextureARB, (void **) &func_glClientActiveTextureARB },
  { "glClientActiveVertexStreamATI", NULL, (void *) wine_glClientActiveVertexStreamATI, (void **) &func_glClientActiveVertexStreamATI },
  { "glColor3fVertex3fSUN", NULL, (void *) wine_glColor3fVertex3fSUN, (void **) &func_glColor3fVertex3fSUN },
  { "glColor3fVertex3fvSUN", NULL, (void *) wine_glColor3fVertex3fvSUN, (void **) &func_glColor3fVertex3fvSUN },
  { "glColor3hNV", NULL, (void *) wine_glColor3hNV, (void **) &func_glColor3hNV },
  { "glColor3hvNV", NULL, (void *) wine_glColor3hvNV, (void **) &func_glColor3hvNV },
  { "glColor4fNormal3fVertex3fSUN", NULL, (void *) wine_glColor4fNormal3fVertex3fSUN, (void **) &func_glColor4fNormal3fVertex3fSUN },
  { "glColor4fNormal3fVertex3fvSUN", NULL, (void *) wine_glColor4fNormal3fVertex3fvSUN, (void **) &func_glColor4fNormal3fVertex3fvSUN },
  { "glColor4hNV", NULL, (void *) wine_glColor4hNV, (void **) &func_glColor4hNV },
  { "glColor4hvNV", NULL, (void *) wine_glColor4hvNV, (void **) &func_glColor4hvNV },
  { "glColor4ubVertex2fSUN", NULL, (void *) wine_glColor4ubVertex2fSUN, (void **) &func_glColor4ubVertex2fSUN },
  { "glColor4ubVertex2fvSUN", NULL, (void *) wine_glColor4ubVertex2fvSUN, (void **) &func_glColor4ubVertex2fvSUN },
  { "glColor4ubVertex3fSUN", NULL, (void *) wine_glColor4ubVertex3fSUN, (void **) &func_glColor4ubVertex3fSUN },
  { "glColor4ubVertex3fvSUN", NULL, (void *) wine_glColor4ubVertex3fvSUN, (void **) &func_glColor4ubVertex3fvSUN },
  { "glColorFragmentOp1ATI", NULL, (void *) wine_glColorFragmentOp1ATI, (void **) &func_glColorFragmentOp1ATI },
  { "glColorFragmentOp2ATI", NULL, (void *) wine_glColorFragmentOp2ATI, (void **) &func_glColorFragmentOp2ATI },
  { "glColorFragmentOp3ATI", NULL, (void *) wine_glColorFragmentOp3ATI, (void **) &func_glColorFragmentOp3ATI },
  { "glColorPointerEXT", NULL, (void *) wine_glColorPointerEXT, (void **) &func_glColorPointerEXT },
  { "glColorPointerListIBM", NULL, (void *) wine_glColorPointerListIBM, (void **) &func_glColorPointerListIBM },
  { "glColorPointervINTEL", NULL, (void *) wine_glColorPointervINTEL, (void **) &func_glColorPointervINTEL },
  { "glColorSubTableEXT", NULL, (void *) wine_glColorSubTableEXT, (void **) &func_glColorSubTableEXT },
  { "glColorTableEXT", NULL, (void *) wine_glColorTableEXT, (void **) &func_glColorTableEXT },
  { "glColorTableParameterfvSGI", NULL, (void *) wine_glColorTableParameterfvSGI, (void **) &func_glColorTableParameterfvSGI },
  { "glColorTableParameterivSGI", NULL, (void *) wine_glColorTableParameterivSGI, (void **) &func_glColorTableParameterivSGI },
  { "glColorTableSGI", NULL, (void *) wine_glColorTableSGI, (void **) &func_glColorTableSGI },
  { "glCombinerInputNV", NULL, (void *) wine_glCombinerInputNV, (void **) &func_glCombinerInputNV },
  { "glCombinerOutputNV", NULL, (void *) wine_glCombinerOutputNV, (void **) &func_glCombinerOutputNV },
  { "glCombinerParameterfNV", NULL, (void *) wine_glCombinerParameterfNV, (void **) &func_glCombinerParameterfNV },
  { "glCombinerParameterfvNV", NULL, (void *) wine_glCombinerParameterfvNV, (void **) &func_glCombinerParameterfvNV },
  { "glCombinerParameteriNV", NULL, (void *) wine_glCombinerParameteriNV, (void **) &func_glCombinerParameteriNV },
  { "glCombinerParameterivNV", NULL, (void *) wine_glCombinerParameterivNV, (void **) &func_glCombinerParameterivNV },
  { "glCombinerStageParameterfvNV", NULL, (void *) wine_glCombinerStageParameterfvNV, (void **) &func_glCombinerStageParameterfvNV },
  { "glCompileShader", NULL, (void *) wine_glCompileShader, (void **) &func_glCompileShader },
  { "glCompileShaderARB", NULL, (void *) wine_glCompileShaderARB, (void **) &func_glCompileShaderARB },
  { "glCompressedTexImage1D", NULL, (void *) wine_glCompressedTexImage1D, (void **) &func_glCompressedTexImage1D },
  { "glCompressedTexImage1DARB", NULL, (void *) wine_glCompressedTexImage1DARB, (void **) &func_glCompressedTexImage1DARB },
  { "glCompressedTexImage2D", NULL, (void *) wine_glCompressedTexImage2D, (void **) &func_glCompressedTexImage2D },
  { "glCompressedTexImage2DARB", NULL, (void *) wine_glCompressedTexImage2DARB, (void **) &func_glCompressedTexImage2DARB },
  { "glCompressedTexImage3D", NULL, (void *) wine_glCompressedTexImage3D, (void **) &func_glCompressedTexImage3D },
  { "glCompressedTexImage3DARB", NULL, (void *) wine_glCompressedTexImage3DARB, (void **) &func_glCompressedTexImage3DARB },
  { "glCompressedTexSubImage1D", NULL, (void *) wine_glCompressedTexSubImage1D, (void **) &func_glCompressedTexSubImage1D },
  { "glCompressedTexSubImage1DARB", NULL, (void *) wine_glCompressedTexSubImage1DARB, (void **) &func_glCompressedTexSubImage1DARB },
  { "glCompressedTexSubImage2D", NULL, (void *) wine_glCompressedTexSubImage2D, (void **) &func_glCompressedTexSubImage2D },
  { "glCompressedTexSubImage2DARB", NULL, (void *) wine_glCompressedTexSubImage2DARB, (void **) &func_glCompressedTexSubImage2DARB },
  { "glCompressedTexSubImage3D", NULL, (void *) wine_glCompressedTexSubImage3D, (void **) &func_glCompressedTexSubImage3D },
  { "glCompressedTexSubImage3DARB", NULL, (void *) wine_glCompressedTexSubImage3DARB, (void **) &func_glCompressedTexSubImage3DARB },
  { "glConvolutionFilter1DEXT", NULL, (void *) wine_glConvolutionFilter1DEXT, (void **) &func_glConvolutionFilter1DEXT },
  { "glConvolutionFilter2DEXT", NULL, (void *) wine_glConvolutionFilter2DEXT, (void **) &func_glConvolutionFilter2DEXT },
  { "glConvolutionParameterfEXT", NULL, (void *) wine_glConvolutionParameterfEXT, (void **) &func_glConvolutionParameterfEXT },
  { "glConvolutionParameterfvEXT", NULL, (void *) wine_glConvolutionParameterfvEXT, (void **) &func_glConvolutionParameterfvEXT },
  { "glConvolutionParameteriEXT", NULL, (void *) wine_glConvolutionParameteriEXT, (void **) &func_glConvolutionParameteriEXT },
  { "glConvolutionParameterivEXT", NULL, (void *) wine_glConvolutionParameterivEXT, (void **) &func_glConvolutionParameterivEXT },
  { "glCopyColorSubTableEXT", NULL, (void *) wine_glCopyColorSubTableEXT, (void **) &func_glCopyColorSubTableEXT },
  { "glCopyColorTableSGI", NULL, (void *) wine_glCopyColorTableSGI, (void **) &func_glCopyColorTableSGI },
  { "glCopyConvolutionFilter1DEXT", NULL, (void *) wine_glCopyConvolutionFilter1DEXT, (void **) &func_glCopyConvolutionFilter1DEXT },
  { "glCopyConvolutionFilter2DEXT", NULL, (void *) wine_glCopyConvolutionFilter2DEXT, (void **) &func_glCopyConvolutionFilter2DEXT },
  { "glCopyTexImage1DEXT", NULL, (void *) wine_glCopyTexImage1DEXT, (void **) &func_glCopyTexImage1DEXT },
  { "glCopyTexImage2DEXT", NULL, (void *) wine_glCopyTexImage2DEXT, (void **) &func_glCopyTexImage2DEXT },
  { "glCopyTexSubImage1DEXT", NULL, (void *) wine_glCopyTexSubImage1DEXT, (void **) &func_glCopyTexSubImage1DEXT },
  { "glCopyTexSubImage2DEXT", NULL, (void *) wine_glCopyTexSubImage2DEXT, (void **) &func_glCopyTexSubImage2DEXT },
  { "glCopyTexSubImage3DEXT", NULL, (void *) wine_glCopyTexSubImage3DEXT, (void **) &func_glCopyTexSubImage3DEXT },
  { "glCreateProgram", NULL, (void *) wine_glCreateProgram, (void **) &func_glCreateProgram },
  { "glCreateProgramObjectARB", NULL, (void *) wine_glCreateProgramObjectARB, (void **) &func_glCreateProgramObjectARB },
  { "glCreateShader", NULL, (void *) wine_glCreateShader, (void **) &func_glCreateShader },
  { "glCreateShaderObjectARB", NULL, (void *) wine_glCreateShaderObjectARB, (void **) &func_glCreateShaderObjectARB },
  { "glCullParameterdvEXT", NULL, (void *) wine_glCullParameterdvEXT, (void **) &func_glCullParameterdvEXT },
  { "glCullParameterfvEXT", NULL, (void *) wine_glCullParameterfvEXT, (void **) &func_glCullParameterfvEXT },
  { "glCurrentPaletteMatrixARB", NULL, (void *) wine_glCurrentPaletteMatrixARB, (void **) &func_glCurrentPaletteMatrixARB },
  { "glDeformSGIX", NULL, (void *) wine_glDeformSGIX, (void **) &func_glDeformSGIX },
  { "glDeformationMap3dSGIX", NULL, (void *) wine_glDeformationMap3dSGIX, (void **) &func_glDeformationMap3dSGIX },
  { "glDeformationMap3fSGIX", NULL, (void *) wine_glDeformationMap3fSGIX, (void **) &func_glDeformationMap3fSGIX },
  { "glDeleteAsyncMarkersSGIX", NULL, (void *) wine_glDeleteAsyncMarkersSGIX, (void **) &func_glDeleteAsyncMarkersSGIX },
  { "glDeleteBufferRegion", NULL, (void *) wine_glDeleteBufferRegion, (void **) &func_glDeleteBufferRegion },
  { "glDeleteBuffers", NULL, (void *) wine_glDeleteBuffers, (void **) &func_glDeleteBuffers },
  { "glDeleteBuffersARB", NULL, (void *) wine_glDeleteBuffersARB, (void **) &func_glDeleteBuffersARB },
  { "glDeleteFencesAPPLE", NULL, (void *) wine_glDeleteFencesAPPLE, (void **) &func_glDeleteFencesAPPLE },
  { "glDeleteFencesNV", NULL, (void *) wine_glDeleteFencesNV, (void **) &func_glDeleteFencesNV },
  { "glDeleteFragmentShaderATI", NULL, (void *) wine_glDeleteFragmentShaderATI, (void **) &func_glDeleteFragmentShaderATI },
  { "glDeleteFramebuffersEXT", NULL, (void *) wine_glDeleteFramebuffersEXT, (void **) &func_glDeleteFramebuffersEXT },
  { "glDeleteObjectARB", NULL, (void *) wine_glDeleteObjectARB, (void **) &func_glDeleteObjectARB },
  { "glDeleteObjectBufferATI", NULL, (void *) wine_glDeleteObjectBufferATI, (void **) &func_glDeleteObjectBufferATI },
  { "glDeleteOcclusionQueriesNV", NULL, (void *) wine_glDeleteOcclusionQueriesNV, (void **) &func_glDeleteOcclusionQueriesNV },
  { "glDeleteProgram", NULL, (void *) wine_glDeleteProgram, (void **) &func_glDeleteProgram },
  { "glDeleteProgramsARB", NULL, (void *) wine_glDeleteProgramsARB, (void **) &func_glDeleteProgramsARB },
  { "glDeleteProgramsNV", NULL, (void *) wine_glDeleteProgramsNV, (void **) &func_glDeleteProgramsNV },
  { "glDeleteQueries", NULL, (void *) wine_glDeleteQueries, (void **) &func_glDeleteQueries },
  { "glDeleteQueriesARB", NULL, (void *) wine_glDeleteQueriesARB, (void **) &func_glDeleteQueriesARB },
  { "glDeleteRenderbuffersEXT", NULL, (void *) wine_glDeleteRenderbuffersEXT, (void **) &func_glDeleteRenderbuffersEXT },
  { "glDeleteShader", NULL, (void *) wine_glDeleteShader, (void **) &func_glDeleteShader },
  { "glDeleteTexturesEXT", NULL, (void *) wine_glDeleteTexturesEXT, (void **) &func_glDeleteTexturesEXT },
  { "glDeleteVertexArraysAPPLE", NULL, (void *) wine_glDeleteVertexArraysAPPLE, (void **) &func_glDeleteVertexArraysAPPLE },
  { "glDeleteVertexShaderEXT", NULL, (void *) wine_glDeleteVertexShaderEXT, (void **) &func_glDeleteVertexShaderEXT },
  { "glDepthBoundsEXT", NULL, (void *) wine_glDepthBoundsEXT, (void **) &func_glDepthBoundsEXT },
  { "glDetachObjectARB", NULL, (void *) wine_glDetachObjectARB, (void **) &func_glDetachObjectARB },
  { "glDetachShader", NULL, (void *) wine_glDetachShader, (void **) &func_glDetachShader },
  { "glDetailTexFuncSGIS", NULL, (void *) wine_glDetailTexFuncSGIS, (void **) &func_glDetailTexFuncSGIS },
  { "glDisableVariantClientStateEXT", NULL, (void *) wine_glDisableVariantClientStateEXT, (void **) &func_glDisableVariantClientStateEXT },
  { "glDisableVertexAttribArray", NULL, (void *) wine_glDisableVertexAttribArray, (void **) &func_glDisableVertexAttribArray },
  { "glDisableVertexAttribArrayARB", NULL, (void *) wine_glDisableVertexAttribArrayARB, (void **) &func_glDisableVertexAttribArrayARB },
  { "glDrawArraysEXT", NULL, (void *) wine_glDrawArraysEXT, (void **) &func_glDrawArraysEXT },
  { "glDrawBufferRegion", NULL, (void *) wine_glDrawBufferRegion, (void **) &func_glDrawBufferRegion },
  { "glDrawBuffers", NULL, (void *) wine_glDrawBuffers, (void **) &func_glDrawBuffers },
  { "glDrawBuffersARB", NULL, (void *) wine_glDrawBuffersARB, (void **) &func_glDrawBuffersARB },
  { "glDrawBuffersATI", NULL, (void *) wine_glDrawBuffersATI, (void **) &func_glDrawBuffersATI },
  { "glDrawElementArrayAPPLE", NULL, (void *) wine_glDrawElementArrayAPPLE, (void **) &func_glDrawElementArrayAPPLE },
  { "glDrawElementArrayATI", NULL, (void *) wine_glDrawElementArrayATI, (void **) &func_glDrawElementArrayATI },
  { "glDrawMeshArraysSUN", NULL, (void *) wine_glDrawMeshArraysSUN, (void **) &func_glDrawMeshArraysSUN },
  { "glDrawRangeElementArrayAPPLE", NULL, (void *) wine_glDrawRangeElementArrayAPPLE, (void **) &func_glDrawRangeElementArrayAPPLE },
  { "glDrawRangeElementArrayATI", NULL, (void *) wine_glDrawRangeElementArrayATI, (void **) &func_glDrawRangeElementArrayATI },
  { "glDrawRangeElementsEXT", NULL, (void *) wine_glDrawRangeElementsEXT, (void **) &func_glDrawRangeElementsEXT },
  { "glEdgeFlagPointerEXT", NULL, (void *) wine_glEdgeFlagPointerEXT, (void **) &func_glEdgeFlagPointerEXT },
  { "glEdgeFlagPointerListIBM", NULL, (void *) wine_glEdgeFlagPointerListIBM, (void **) &func_glEdgeFlagPointerListIBM },
  { "glElementPointerAPPLE", NULL, (void *) wine_glElementPointerAPPLE, (void **) &func_glElementPointerAPPLE },
  { "glElementPointerATI", NULL, (void *) wine_glElementPointerATI, (void **) &func_glElementPointerATI },
  { "glEnableVariantClientStateEXT", NULL, (void *) wine_glEnableVariantClientStateEXT, (void **) &func_glEnableVariantClientStateEXT },
  { "glEnableVertexAttribArray", NULL, (void *) wine_glEnableVertexAttribArray, (void **) &func_glEnableVertexAttribArray },
  { "glEnableVertexAttribArrayARB", NULL, (void *) wine_glEnableVertexAttribArrayARB, (void **) &func_glEnableVertexAttribArrayARB },
  { "glEndFragmentShaderATI", NULL, (void *) wine_glEndFragmentShaderATI, (void **) &func_glEndFragmentShaderATI },
  { "glEndOcclusionQueryNV", NULL, (void *) wine_glEndOcclusionQueryNV, (void **) &func_glEndOcclusionQueryNV },
  { "glEndQuery", NULL, (void *) wine_glEndQuery, (void **) &func_glEndQuery },
  { "glEndQueryARB", NULL, (void *) wine_glEndQueryARB, (void **) &func_glEndQueryARB },
  { "glEndVertexShaderEXT", NULL, (void *) wine_glEndVertexShaderEXT, (void **) &func_glEndVertexShaderEXT },
  { "glEvalMapsNV", NULL, (void *) wine_glEvalMapsNV, (void **) &func_glEvalMapsNV },
  { "glExecuteProgramNV", NULL, (void *) wine_glExecuteProgramNV, (void **) &func_glExecuteProgramNV },
  { "glExtractComponentEXT", NULL, (void *) wine_glExtractComponentEXT, (void **) &func_glExtractComponentEXT },
  { "glFinalCombinerInputNV", NULL, (void *) wine_glFinalCombinerInputNV, (void **) &func_glFinalCombinerInputNV },
  { "glFinishAsyncSGIX", NULL, (void *) wine_glFinishAsyncSGIX, (void **) &func_glFinishAsyncSGIX },
  { "glFinishFenceAPPLE", NULL, (void *) wine_glFinishFenceAPPLE, (void **) &func_glFinishFenceAPPLE },
  { "glFinishFenceNV", NULL, (void *) wine_glFinishFenceNV, (void **) &func_glFinishFenceNV },
  { "glFinishObjectAPPLE", NULL, (void *) wine_glFinishObjectAPPLE, (void **) &func_glFinishObjectAPPLE },
  { "glFinishTextureSUNX", NULL, (void *) wine_glFinishTextureSUNX, (void **) &func_glFinishTextureSUNX },
  { "glFlushPixelDataRangeNV", NULL, (void *) wine_glFlushPixelDataRangeNV, (void **) &func_glFlushPixelDataRangeNV },
  { "glFlushRasterSGIX", NULL, (void *) wine_glFlushRasterSGIX, (void **) &func_glFlushRasterSGIX },
  { "glFlushVertexArrayRangeAPPLE", NULL, (void *) wine_glFlushVertexArrayRangeAPPLE, (void **) &func_glFlushVertexArrayRangeAPPLE },
  { "glFlushVertexArrayRangeNV", NULL, (void *) wine_glFlushVertexArrayRangeNV, (void **) &func_glFlushVertexArrayRangeNV },
  { "glFogCoordPointer", NULL, (void *) wine_glFogCoordPointer, (void **) &func_glFogCoordPointer },
  { "glFogCoordPointerEXT", NULL, (void *) wine_glFogCoordPointerEXT, (void **) &func_glFogCoordPointerEXT },
  { "glFogCoordPointerListIBM", NULL, (void *) wine_glFogCoordPointerListIBM, (void **) &func_glFogCoordPointerListIBM },
  { "glFogCoordd", NULL, (void *) wine_glFogCoordd, (void **) &func_glFogCoordd },
  { "glFogCoorddEXT", NULL, (void *) wine_glFogCoorddEXT, (void **) &func_glFogCoorddEXT },
  { "glFogCoorddv", NULL, (void *) wine_glFogCoorddv, (void **) &func_glFogCoorddv },
  { "glFogCoorddvEXT", NULL, (void *) wine_glFogCoorddvEXT, (void **) &func_glFogCoorddvEXT },
  { "glFogCoordf", NULL, (void *) wine_glFogCoordf, (void **) &func_glFogCoordf },
  { "glFogCoordfEXT", NULL, (void *) wine_glFogCoordfEXT, (void **) &func_glFogCoordfEXT },
  { "glFogCoordfv", NULL, (void *) wine_glFogCoordfv, (void **) &func_glFogCoordfv },
  { "glFogCoordfvEXT", NULL, (void *) wine_glFogCoordfvEXT, (void **) &func_glFogCoordfvEXT },
  { "glFogCoordhNV", NULL, (void *) wine_glFogCoordhNV, (void **) &func_glFogCoordhNV },
  { "glFogCoordhvNV", NULL, (void *) wine_glFogCoordhvNV, (void **) &func_glFogCoordhvNV },
  { "glFogFuncSGIS", NULL, (void *) wine_glFogFuncSGIS, (void **) &func_glFogFuncSGIS },
  { "glFragmentColorMaterialSGIX", NULL, (void *) wine_glFragmentColorMaterialSGIX, (void **) &func_glFragmentColorMaterialSGIX },
  { "glFragmentLightModelfSGIX", NULL, (void *) wine_glFragmentLightModelfSGIX, (void **) &func_glFragmentLightModelfSGIX },
  { "glFragmentLightModelfvSGIX", NULL, (void *) wine_glFragmentLightModelfvSGIX, (void **) &func_glFragmentLightModelfvSGIX },
  { "glFragmentLightModeliSGIX", NULL, (void *) wine_glFragmentLightModeliSGIX, (void **) &func_glFragmentLightModeliSGIX },
  { "glFragmentLightModelivSGIX", NULL, (void *) wine_glFragmentLightModelivSGIX, (void **) &func_glFragmentLightModelivSGIX },
  { "glFragmentLightfSGIX", NULL, (void *) wine_glFragmentLightfSGIX, (void **) &func_glFragmentLightfSGIX },
  { "glFragmentLightfvSGIX", NULL, (void *) wine_glFragmentLightfvSGIX, (void **) &func_glFragmentLightfvSGIX },
  { "glFragmentLightiSGIX", NULL, (void *) wine_glFragmentLightiSGIX, (void **) &func_glFragmentLightiSGIX },
  { "glFragmentLightivSGIX", NULL, (void *) wine_glFragmentLightivSGIX, (void **) &func_glFragmentLightivSGIX },
  { "glFragmentMaterialfSGIX", NULL, (void *) wine_glFragmentMaterialfSGIX, (void **) &func_glFragmentMaterialfSGIX },
  { "glFragmentMaterialfvSGIX", NULL, (void *) wine_glFragmentMaterialfvSGIX, (void **) &func_glFragmentMaterialfvSGIX },
  { "glFragmentMaterialiSGIX", NULL, (void *) wine_glFragmentMaterialiSGIX, (void **) &func_glFragmentMaterialiSGIX },
  { "glFragmentMaterialivSGIX", NULL, (void *) wine_glFragmentMaterialivSGIX, (void **) &func_glFragmentMaterialivSGIX },
  { "glFrameZoomSGIX", NULL, (void *) wine_glFrameZoomSGIX, (void **) &func_glFrameZoomSGIX },
  { "glFramebufferRenderbufferEXT", NULL, (void *) wine_glFramebufferRenderbufferEXT, (void **) &func_glFramebufferRenderbufferEXT },
  { "glFramebufferTexture1DEXT", NULL, (void *) wine_glFramebufferTexture1DEXT, (void **) &func_glFramebufferTexture1DEXT },
  { "glFramebufferTexture2DEXT", NULL, (void *) wine_glFramebufferTexture2DEXT, (void **) &func_glFramebufferTexture2DEXT },
  { "glFramebufferTexture3DEXT", NULL, (void *) wine_glFramebufferTexture3DEXT, (void **) &func_glFramebufferTexture3DEXT },
  { "glFreeObjectBufferATI", NULL, (void *) wine_glFreeObjectBufferATI, (void **) &func_glFreeObjectBufferATI },
  { "glGenAsyncMarkersSGIX", NULL, (void *) wine_glGenAsyncMarkersSGIX, (void **) &func_glGenAsyncMarkersSGIX },
  { "glGenBuffers", NULL, (void *) wine_glGenBuffers, (void **) &func_glGenBuffers },
  { "glGenBuffersARB", NULL, (void *) wine_glGenBuffersARB, (void **) &func_glGenBuffersARB },
  { "glGenFencesAPPLE", NULL, (void *) wine_glGenFencesAPPLE, (void **) &func_glGenFencesAPPLE },
  { "glGenFencesNV", NULL, (void *) wine_glGenFencesNV, (void **) &func_glGenFencesNV },
  { "glGenFragmentShadersATI", NULL, (void *) wine_glGenFragmentShadersATI, (void **) &func_glGenFragmentShadersATI },
  { "glGenFramebuffersEXT", NULL, (void *) wine_glGenFramebuffersEXT, (void **) &func_glGenFramebuffersEXT },
  { "glGenOcclusionQueriesNV", NULL, (void *) wine_glGenOcclusionQueriesNV, (void **) &func_glGenOcclusionQueriesNV },
  { "glGenProgramsARB", NULL, (void *) wine_glGenProgramsARB, (void **) &func_glGenProgramsARB },
  { "glGenProgramsNV", NULL, (void *) wine_glGenProgramsNV, (void **) &func_glGenProgramsNV },
  { "glGenQueries", NULL, (void *) wine_glGenQueries, (void **) &func_glGenQueries },
  { "glGenQueriesARB", NULL, (void *) wine_glGenQueriesARB, (void **) &func_glGenQueriesARB },
  { "glGenRenderbuffersEXT", NULL, (void *) wine_glGenRenderbuffersEXT, (void **) &func_glGenRenderbuffersEXT },
  { "glGenSymbolsEXT", NULL, (void *) wine_glGenSymbolsEXT, (void **) &func_glGenSymbolsEXT },
  { "glGenTexturesEXT", NULL, (void *) wine_glGenTexturesEXT, (void **) &func_glGenTexturesEXT },
  { "glGenVertexArraysAPPLE", NULL, (void *) wine_glGenVertexArraysAPPLE, (void **) &func_glGenVertexArraysAPPLE },
  { "glGenVertexShadersEXT", NULL, (void *) wine_glGenVertexShadersEXT, (void **) &func_glGenVertexShadersEXT },
  { "glGenerateMipmapEXT", NULL, (void *) wine_glGenerateMipmapEXT, (void **) &func_glGenerateMipmapEXT },
  { "glGetActiveAttrib", NULL, (void *) wine_glGetActiveAttrib, (void **) &func_glGetActiveAttrib },
  { "glGetActiveAttribARB", NULL, (void *) wine_glGetActiveAttribARB, (void **) &func_glGetActiveAttribARB },
  { "glGetActiveUniform", NULL, (void *) wine_glGetActiveUniform, (void **) &func_glGetActiveUniform },
  { "glGetActiveUniformARB", NULL, (void *) wine_glGetActiveUniformARB, (void **) &func_glGetActiveUniformARB },
  { "glGetArrayObjectfvATI", NULL, (void *) wine_glGetArrayObjectfvATI, (void **) &func_glGetArrayObjectfvATI },
  { "glGetArrayObjectivATI", NULL, (void *) wine_glGetArrayObjectivATI, (void **) &func_glGetArrayObjectivATI },
  { "glGetAttachedObjectsARB", NULL, (void *) wine_glGetAttachedObjectsARB, (void **) &func_glGetAttachedObjectsARB },
  { "glGetAttachedShaders", NULL, (void *) wine_glGetAttachedShaders, (void **) &func_glGetAttachedShaders },
  { "glGetAttribLocation", NULL, (void *) wine_glGetAttribLocation, (void **) &func_glGetAttribLocation },
  { "glGetAttribLocationARB", NULL, (void *) wine_glGetAttribLocationARB, (void **) &func_glGetAttribLocationARB },
  { "glGetBufferParameteriv", NULL, (void *) wine_glGetBufferParameteriv, (void **) &func_glGetBufferParameteriv },
  { "glGetBufferParameterivARB", NULL, (void *) wine_glGetBufferParameterivARB, (void **) &func_glGetBufferParameterivARB },
  { "glGetBufferPointerv", NULL, (void *) wine_glGetBufferPointerv, (void **) &func_glGetBufferPointerv },
  { "glGetBufferPointervARB", NULL, (void *) wine_glGetBufferPointervARB, (void **) &func_glGetBufferPointervARB },
  { "glGetBufferSubData", NULL, (void *) wine_glGetBufferSubData, (void **) &func_glGetBufferSubData },
  { "glGetBufferSubDataARB", NULL, (void *) wine_glGetBufferSubDataARB, (void **) &func_glGetBufferSubDataARB },
  { "glGetColorTableEXT", NULL, (void *) wine_glGetColorTableEXT, (void **) &func_glGetColorTableEXT },
  { "glGetColorTableParameterfvEXT", NULL, (void *) wine_glGetColorTableParameterfvEXT, (void **) &func_glGetColorTableParameterfvEXT },
  { "glGetColorTableParameterfvSGI", NULL, (void *) wine_glGetColorTableParameterfvSGI, (void **) &func_glGetColorTableParameterfvSGI },
  { "glGetColorTableParameterivEXT", NULL, (void *) wine_glGetColorTableParameterivEXT, (void **) &func_glGetColorTableParameterivEXT },
  { "glGetColorTableParameterivSGI", NULL, (void *) wine_glGetColorTableParameterivSGI, (void **) &func_glGetColorTableParameterivSGI },
  { "glGetColorTableSGI", NULL, (void *) wine_glGetColorTableSGI, (void **) &func_glGetColorTableSGI },
  { "glGetCombinerInputParameterfvNV", NULL, (void *) wine_glGetCombinerInputParameterfvNV, (void **) &func_glGetCombinerInputParameterfvNV },
  { "glGetCombinerInputParameterivNV", NULL, (void *) wine_glGetCombinerInputParameterivNV, (void **) &func_glGetCombinerInputParameterivNV },
  { "glGetCombinerOutputParameterfvNV", NULL, (void *) wine_glGetCombinerOutputParameterfvNV, (void **) &func_glGetCombinerOutputParameterfvNV },
  { "glGetCombinerOutputParameterivNV", NULL, (void *) wine_glGetCombinerOutputParameterivNV, (void **) &func_glGetCombinerOutputParameterivNV },
  { "glGetCombinerStageParameterfvNV", NULL, (void *) wine_glGetCombinerStageParameterfvNV, (void **) &func_glGetCombinerStageParameterfvNV },
  { "glGetCompressedTexImage", NULL, (void *) wine_glGetCompressedTexImage, (void **) &func_glGetCompressedTexImage },
  { "glGetCompressedTexImageARB", NULL, (void *) wine_glGetCompressedTexImageARB, (void **) &func_glGetCompressedTexImageARB },
  { "glGetConvolutionFilterEXT", NULL, (void *) wine_glGetConvolutionFilterEXT, (void **) &func_glGetConvolutionFilterEXT },
  { "glGetConvolutionParameterfvEXT", NULL, (void *) wine_glGetConvolutionParameterfvEXT, (void **) &func_glGetConvolutionParameterfvEXT },
  { "glGetConvolutionParameterivEXT", NULL, (void *) wine_glGetConvolutionParameterivEXT, (void **) &func_glGetConvolutionParameterivEXT },
  { "glGetDetailTexFuncSGIS", NULL, (void *) wine_glGetDetailTexFuncSGIS, (void **) &func_glGetDetailTexFuncSGIS },
  { "glGetFenceivNV", NULL, (void *) wine_glGetFenceivNV, (void **) &func_glGetFenceivNV },
  { "glGetFinalCombinerInputParameterfvNV", NULL, (void *) wine_glGetFinalCombinerInputParameterfvNV, (void **) &func_glGetFinalCombinerInputParameterfvNV },
  { "glGetFinalCombinerInputParameterivNV", NULL, (void *) wine_glGetFinalCombinerInputParameterivNV, (void **) &func_glGetFinalCombinerInputParameterivNV },
  { "glGetFogFuncSGIS", NULL, (void *) wine_glGetFogFuncSGIS, (void **) &func_glGetFogFuncSGIS },
  { "glGetFragmentLightfvSGIX", NULL, (void *) wine_glGetFragmentLightfvSGIX, (void **) &func_glGetFragmentLightfvSGIX },
  { "glGetFragmentLightivSGIX", NULL, (void *) wine_glGetFragmentLightivSGIX, (void **) &func_glGetFragmentLightivSGIX },
  { "glGetFragmentMaterialfvSGIX", NULL, (void *) wine_glGetFragmentMaterialfvSGIX, (void **) &func_glGetFragmentMaterialfvSGIX },
  { "glGetFragmentMaterialivSGIX", NULL, (void *) wine_glGetFragmentMaterialivSGIX, (void **) &func_glGetFragmentMaterialivSGIX },
  { "glGetFramebufferAttachmentParameterivEXT", NULL, (void *) wine_glGetFramebufferAttachmentParameterivEXT, (void **) &func_glGetFramebufferAttachmentParameterivEXT },
  { "glGetHandleARB", NULL, (void *) wine_glGetHandleARB, (void **) &func_glGetHandleARB },
  { "glGetHistogramEXT", NULL, (void *) wine_glGetHistogramEXT, (void **) &func_glGetHistogramEXT },
  { "glGetHistogramParameterfvEXT", NULL, (void *) wine_glGetHistogramParameterfvEXT, (void **) &func_glGetHistogramParameterfvEXT },
  { "glGetHistogramParameterivEXT", NULL, (void *) wine_glGetHistogramParameterivEXT, (void **) &func_glGetHistogramParameterivEXT },
  { "glGetImageTransformParameterfvHP", NULL, (void *) wine_glGetImageTransformParameterfvHP, (void **) &func_glGetImageTransformParameterfvHP },
  { "glGetImageTransformParameterivHP", NULL, (void *) wine_glGetImageTransformParameterivHP, (void **) &func_glGetImageTransformParameterivHP },
  { "glGetInfoLogARB", NULL, (void *) wine_glGetInfoLogARB, (void **) &func_glGetInfoLogARB },
  { "glGetInstrumentsSGIX", NULL, (void *) wine_glGetInstrumentsSGIX, (void **) &func_glGetInstrumentsSGIX },
  { "glGetInvariantBooleanvEXT", NULL, (void *) wine_glGetInvariantBooleanvEXT, (void **) &func_glGetInvariantBooleanvEXT },
  { "glGetInvariantFloatvEXT", NULL, (void *) wine_glGetInvariantFloatvEXT, (void **) &func_glGetInvariantFloatvEXT },
  { "glGetInvariantIntegervEXT", NULL, (void *) wine_glGetInvariantIntegervEXT, (void **) &func_glGetInvariantIntegervEXT },
  { "glGetListParameterfvSGIX", NULL, (void *) wine_glGetListParameterfvSGIX, (void **) &func_glGetListParameterfvSGIX },
  { "glGetListParameterivSGIX", NULL, (void *) wine_glGetListParameterivSGIX, (void **) &func_glGetListParameterivSGIX },
  { "glGetLocalConstantBooleanvEXT", NULL, (void *) wine_glGetLocalConstantBooleanvEXT, (void **) &func_glGetLocalConstantBooleanvEXT },
  { "glGetLocalConstantFloatvEXT", NULL, (void *) wine_glGetLocalConstantFloatvEXT, (void **) &func_glGetLocalConstantFloatvEXT },
  { "glGetLocalConstantIntegervEXT", NULL, (void *) wine_glGetLocalConstantIntegervEXT, (void **) &func_glGetLocalConstantIntegervEXT },
  { "glGetMapAttribParameterfvNV", NULL, (void *) wine_glGetMapAttribParameterfvNV, (void **) &func_glGetMapAttribParameterfvNV },
  { "glGetMapAttribParameterivNV", NULL, (void *) wine_glGetMapAttribParameterivNV, (void **) &func_glGetMapAttribParameterivNV },
  { "glGetMapControlPointsNV", NULL, (void *) wine_glGetMapControlPointsNV, (void **) &func_glGetMapControlPointsNV },
  { "glGetMapParameterfvNV", NULL, (void *) wine_glGetMapParameterfvNV, (void **) &func_glGetMapParameterfvNV },
  { "glGetMapParameterivNV", NULL, (void *) wine_glGetMapParameterivNV, (void **) &func_glGetMapParameterivNV },
  { "glGetMinmaxEXT", NULL, (void *) wine_glGetMinmaxEXT, (void **) &func_glGetMinmaxEXT },
  { "glGetMinmaxParameterfvEXT", NULL, (void *) wine_glGetMinmaxParameterfvEXT, (void **) &func_glGetMinmaxParameterfvEXT },
  { "glGetMinmaxParameterivEXT", NULL, (void *) wine_glGetMinmaxParameterivEXT, (void **) &func_glGetMinmaxParameterivEXT },
  { "glGetObjectBufferfvATI", NULL, (void *) wine_glGetObjectBufferfvATI, (void **) &func_glGetObjectBufferfvATI },
  { "glGetObjectBufferivATI", NULL, (void *) wine_glGetObjectBufferivATI, (void **) &func_glGetObjectBufferivATI },
  { "glGetObjectParameterfvARB", NULL, (void *) wine_glGetObjectParameterfvARB, (void **) &func_glGetObjectParameterfvARB },
  { "glGetObjectParameterivARB", NULL, (void *) wine_glGetObjectParameterivARB, (void **) &func_glGetObjectParameterivARB },
  { "glGetOcclusionQueryivNV", NULL, (void *) wine_glGetOcclusionQueryivNV, (void **) &func_glGetOcclusionQueryivNV },
  { "glGetOcclusionQueryuivNV", NULL, (void *) wine_glGetOcclusionQueryuivNV, (void **) &func_glGetOcclusionQueryuivNV },
  { "glGetPixelTexGenParameterfvSGIS", NULL, (void *) wine_glGetPixelTexGenParameterfvSGIS, (void **) &func_glGetPixelTexGenParameterfvSGIS },
  { "glGetPixelTexGenParameterivSGIS", NULL, (void *) wine_glGetPixelTexGenParameterivSGIS, (void **) &func_glGetPixelTexGenParameterivSGIS },
  { "glGetPointervEXT", NULL, (void *) wine_glGetPointervEXT, (void **) &func_glGetPointervEXT },
  { "glGetProgramEnvParameterdvARB", NULL, (void *) wine_glGetProgramEnvParameterdvARB, (void **) &func_glGetProgramEnvParameterdvARB },
  { "glGetProgramEnvParameterfvARB", NULL, (void *) wine_glGetProgramEnvParameterfvARB, (void **) &func_glGetProgramEnvParameterfvARB },
  { "glGetProgramInfoLog", NULL, (void *) wine_glGetProgramInfoLog, (void **) &func_glGetProgramInfoLog },
  { "glGetProgramLocalParameterdvARB", NULL, (void *) wine_glGetProgramLocalParameterdvARB, (void **) &func_glGetProgramLocalParameterdvARB },
  { "glGetProgramLocalParameterfvARB", NULL, (void *) wine_glGetProgramLocalParameterfvARB, (void **) &func_glGetProgramLocalParameterfvARB },
  { "glGetProgramNamedParameterdvNV", NULL, (void *) wine_glGetProgramNamedParameterdvNV, (void **) &func_glGetProgramNamedParameterdvNV },
  { "glGetProgramNamedParameterfvNV", NULL, (void *) wine_glGetProgramNamedParameterfvNV, (void **) &func_glGetProgramNamedParameterfvNV },
  { "glGetProgramParameterdvNV", NULL, (void *) wine_glGetProgramParameterdvNV, (void **) &func_glGetProgramParameterdvNV },
  { "glGetProgramParameterfvNV", NULL, (void *) wine_glGetProgramParameterfvNV, (void **) &func_glGetProgramParameterfvNV },
  { "glGetProgramStringARB", NULL, (void *) wine_glGetProgramStringARB, (void **) &func_glGetProgramStringARB },
  { "glGetProgramStringNV", NULL, (void *) wine_glGetProgramStringNV, (void **) &func_glGetProgramStringNV },
  { "glGetProgramiv", NULL, (void *) wine_glGetProgramiv, (void **) &func_glGetProgramiv },
  { "glGetProgramivARB", NULL, (void *) wine_glGetProgramivARB, (void **) &func_glGetProgramivARB },
  { "glGetProgramivNV", NULL, (void *) wine_glGetProgramivNV, (void **) &func_glGetProgramivNV },
  { "glGetQueryObjectiv", NULL, (void *) wine_glGetQueryObjectiv, (void **) &func_glGetQueryObjectiv },
  { "glGetQueryObjectivARB", NULL, (void *) wine_glGetQueryObjectivARB, (void **) &func_glGetQueryObjectivARB },
  { "glGetQueryObjectuiv", NULL, (void *) wine_glGetQueryObjectuiv, (void **) &func_glGetQueryObjectuiv },
  { "glGetQueryObjectuivARB", NULL, (void *) wine_glGetQueryObjectuivARB, (void **) &func_glGetQueryObjectuivARB },
  { "glGetQueryiv", NULL, (void *) wine_glGetQueryiv, (void **) &func_glGetQueryiv },
  { "glGetQueryivARB", NULL, (void *) wine_glGetQueryivARB, (void **) &func_glGetQueryivARB },
  { "glGetRenderbufferParameterivEXT", NULL, (void *) wine_glGetRenderbufferParameterivEXT, (void **) &func_glGetRenderbufferParameterivEXT },
  { "glGetSeparableFilterEXT", NULL, (void *) wine_glGetSeparableFilterEXT, (void **) &func_glGetSeparableFilterEXT },
  { "glGetShaderInfoLog", NULL, (void *) wine_glGetShaderInfoLog, (void **) &func_glGetShaderInfoLog },
  { "glGetShaderSource", NULL, (void *) wine_glGetShaderSource, (void **) &func_glGetShaderSource },
  { "glGetShaderSourceARB", NULL, (void *) wine_glGetShaderSourceARB, (void **) &func_glGetShaderSourceARB },
  { "glGetShaderiv", NULL, (void *) wine_glGetShaderiv, (void **) &func_glGetShaderiv },
  { "glGetSharpenTexFuncSGIS", NULL, (void *) wine_glGetSharpenTexFuncSGIS, (void **) &func_glGetSharpenTexFuncSGIS },
  { "glGetTexBumpParameterfvATI", NULL, (void *) wine_glGetTexBumpParameterfvATI, (void **) &func_glGetTexBumpParameterfvATI },
  { "glGetTexBumpParameterivATI", NULL, (void *) wine_glGetTexBumpParameterivATI, (void **) &func_glGetTexBumpParameterivATI },
  { "glGetTexFilterFuncSGIS", NULL, (void *) wine_glGetTexFilterFuncSGIS, (void **) &func_glGetTexFilterFuncSGIS },
  { "glGetTrackMatrixivNV", NULL, (void *) wine_glGetTrackMatrixivNV, (void **) &func_glGetTrackMatrixivNV },
  { "glGetUniformLocation", NULL, (void *) wine_glGetUniformLocation, (void **) &func_glGetUniformLocation },
  { "glGetUniformLocationARB", NULL, (void *) wine_glGetUniformLocationARB, (void **) &func_glGetUniformLocationARB },
  { "glGetUniformfv", NULL, (void *) wine_glGetUniformfv, (void **) &func_glGetUniformfv },
  { "glGetUniformfvARB", NULL, (void *) wine_glGetUniformfvARB, (void **) &func_glGetUniformfvARB },
  { "glGetUniformiv", NULL, (void *) wine_glGetUniformiv, (void **) &func_glGetUniformiv },
  { "glGetUniformivARB", NULL, (void *) wine_glGetUniformivARB, (void **) &func_glGetUniformivARB },
  { "glGetVariantArrayObjectfvATI", NULL, (void *) wine_glGetVariantArrayObjectfvATI, (void **) &func_glGetVariantArrayObjectfvATI },
  { "glGetVariantArrayObjectivATI", NULL, (void *) wine_glGetVariantArrayObjectivATI, (void **) &func_glGetVariantArrayObjectivATI },
  { "glGetVariantBooleanvEXT", NULL, (void *) wine_glGetVariantBooleanvEXT, (void **) &func_glGetVariantBooleanvEXT },
  { "glGetVariantFloatvEXT", NULL, (void *) wine_glGetVariantFloatvEXT, (void **) &func_glGetVariantFloatvEXT },
  { "glGetVariantIntegervEXT", NULL, (void *) wine_glGetVariantIntegervEXT, (void **) &func_glGetVariantIntegervEXT },
  { "glGetVariantPointervEXT", NULL, (void *) wine_glGetVariantPointervEXT, (void **) &func_glGetVariantPointervEXT },
  { "glGetVertexAttribArrayObjectfvATI", NULL, (void *) wine_glGetVertexAttribArrayObjectfvATI, (void **) &func_glGetVertexAttribArrayObjectfvATI },
  { "glGetVertexAttribArrayObjectivATI", NULL, (void *) wine_glGetVertexAttribArrayObjectivATI, (void **) &func_glGetVertexAttribArrayObjectivATI },
  { "glGetVertexAttribPointerv", NULL, (void *) wine_glGetVertexAttribPointerv, (void **) &func_glGetVertexAttribPointerv },
  { "glGetVertexAttribPointervARB", NULL, (void *) wine_glGetVertexAttribPointervARB, (void **) &func_glGetVertexAttribPointervARB },
  { "glGetVertexAttribPointervNV", NULL, (void *) wine_glGetVertexAttribPointervNV, (void **) &func_glGetVertexAttribPointervNV },
  { "glGetVertexAttribdv", NULL, (void *) wine_glGetVertexAttribdv, (void **) &func_glGetVertexAttribdv },
  { "glGetVertexAttribdvARB", NULL, (void *) wine_glGetVertexAttribdvARB, (void **) &func_glGetVertexAttribdvARB },
  { "glGetVertexAttribdvNV", NULL, (void *) wine_glGetVertexAttribdvNV, (void **) &func_glGetVertexAttribdvNV },
  { "glGetVertexAttribfv", NULL, (void *) wine_glGetVertexAttribfv, (void **) &func_glGetVertexAttribfv },
  { "glGetVertexAttribfvARB", NULL, (void *) wine_glGetVertexAttribfvARB, (void **) &func_glGetVertexAttribfvARB },
  { "glGetVertexAttribfvNV", NULL, (void *) wine_glGetVertexAttribfvNV, (void **) &func_glGetVertexAttribfvNV },
  { "glGetVertexAttribiv", NULL, (void *) wine_glGetVertexAttribiv, (void **) &func_glGetVertexAttribiv },
  { "glGetVertexAttribivARB", NULL, (void *) wine_glGetVertexAttribivARB, (void **) &func_glGetVertexAttribivARB },
  { "glGetVertexAttribivNV", NULL, (void *) wine_glGetVertexAttribivNV, (void **) &func_glGetVertexAttribivNV },
  { "glGlobalAlphaFactorbSUN", NULL, (void *) wine_glGlobalAlphaFactorbSUN, (void **) &func_glGlobalAlphaFactorbSUN },
  { "glGlobalAlphaFactordSUN", NULL, (void *) wine_glGlobalAlphaFactordSUN, (void **) &func_glGlobalAlphaFactordSUN },
  { "glGlobalAlphaFactorfSUN", NULL, (void *) wine_glGlobalAlphaFactorfSUN, (void **) &func_glGlobalAlphaFactorfSUN },
  { "glGlobalAlphaFactoriSUN", NULL, (void *) wine_glGlobalAlphaFactoriSUN, (void **) &func_glGlobalAlphaFactoriSUN },
  { "glGlobalAlphaFactorsSUN", NULL, (void *) wine_glGlobalAlphaFactorsSUN, (void **) &func_glGlobalAlphaFactorsSUN },
  { "glGlobalAlphaFactorubSUN", NULL, (void *) wine_glGlobalAlphaFactorubSUN, (void **) &func_glGlobalAlphaFactorubSUN },
  { "glGlobalAlphaFactoruiSUN", NULL, (void *) wine_glGlobalAlphaFactoruiSUN, (void **) &func_glGlobalAlphaFactoruiSUN },
  { "glGlobalAlphaFactorusSUN", NULL, (void *) wine_glGlobalAlphaFactorusSUN, (void **) &func_glGlobalAlphaFactorusSUN },
  { "glHintPGI", NULL, (void *) wine_glHintPGI, (void **) &func_glHintPGI },
  { "glHistogramEXT", NULL, (void *) wine_glHistogramEXT, (void **) &func_glHistogramEXT },
  { "glIglooInterfaceSGIX", NULL, (void *) wine_glIglooInterfaceSGIX, (void **) &func_glIglooInterfaceSGIX },
  { "glImageTransformParameterfHP", NULL, (void *) wine_glImageTransformParameterfHP, (void **) &func_glImageTransformParameterfHP },
  { "glImageTransformParameterfvHP", NULL, (void *) wine_glImageTransformParameterfvHP, (void **) &func_glImageTransformParameterfvHP },
  { "glImageTransformParameteriHP", NULL, (void *) wine_glImageTransformParameteriHP, (void **) &func_glImageTransformParameteriHP },
  { "glImageTransformParameterivHP", NULL, (void *) wine_glImageTransformParameterivHP, (void **) &func_glImageTransformParameterivHP },
  { "glIndexFuncEXT", NULL, (void *) wine_glIndexFuncEXT, (void **) &func_glIndexFuncEXT },
  { "glIndexMaterialEXT", NULL, (void *) wine_glIndexMaterialEXT, (void **) &func_glIndexMaterialEXT },
  { "glIndexPointerEXT", NULL, (void *) wine_glIndexPointerEXT, (void **) &func_glIndexPointerEXT },
  { "glIndexPointerListIBM", NULL, (void *) wine_glIndexPointerListIBM, (void **) &func_glIndexPointerListIBM },
  { "glInsertComponentEXT", NULL, (void *) wine_glInsertComponentEXT, (void **) &func_glInsertComponentEXT },
  { "glInstrumentsBufferSGIX", NULL, (void *) wine_glInstrumentsBufferSGIX, (void **) &func_glInstrumentsBufferSGIX },
  { "glIsAsyncMarkerSGIX", NULL, (void *) wine_glIsAsyncMarkerSGIX, (void **) &func_glIsAsyncMarkerSGIX },
  { "glIsBuffer", NULL, (void *) wine_glIsBuffer, (void **) &func_glIsBuffer },
  { "glIsBufferARB", NULL, (void *) wine_glIsBufferARB, (void **) &func_glIsBufferARB },
  { "glIsFenceAPPLE", NULL, (void *) wine_glIsFenceAPPLE, (void **) &func_glIsFenceAPPLE },
  { "glIsFenceNV", NULL, (void *) wine_glIsFenceNV, (void **) &func_glIsFenceNV },
  { "glIsFramebufferEXT", NULL, (void *) wine_glIsFramebufferEXT, (void **) &func_glIsFramebufferEXT },
  { "glIsObjectBufferATI", NULL, (void *) wine_glIsObjectBufferATI, (void **) &func_glIsObjectBufferATI },
  { "glIsOcclusionQueryNV", NULL, (void *) wine_glIsOcclusionQueryNV, (void **) &func_glIsOcclusionQueryNV },
  { "glIsProgram", NULL, (void *) wine_glIsProgram, (void **) &func_glIsProgram },
  { "glIsProgramARB", NULL, (void *) wine_glIsProgramARB, (void **) &func_glIsProgramARB },
  { "glIsProgramNV", NULL, (void *) wine_glIsProgramNV, (void **) &func_glIsProgramNV },
  { "glIsQuery", NULL, (void *) wine_glIsQuery, (void **) &func_glIsQuery },
  { "glIsQueryARB", NULL, (void *) wine_glIsQueryARB, (void **) &func_glIsQueryARB },
  { "glIsRenderbufferEXT", NULL, (void *) wine_glIsRenderbufferEXT, (void **) &func_glIsRenderbufferEXT },
  { "glIsShader", NULL, (void *) wine_glIsShader, (void **) &func_glIsShader },
  { "glIsTextureEXT", NULL, (void *) wine_glIsTextureEXT, (void **) &func_glIsTextureEXT },
  { "glIsVariantEnabledEXT", NULL, (void *) wine_glIsVariantEnabledEXT, (void **) &func_glIsVariantEnabledEXT },
  { "glIsVertexArrayAPPLE", NULL, (void *) wine_glIsVertexArrayAPPLE, (void **) &func_glIsVertexArrayAPPLE },
  { "glLightEnviSGIX", NULL, (void *) wine_glLightEnviSGIX, (void **) &func_glLightEnviSGIX },
  { "glLinkProgram", NULL, (void *) wine_glLinkProgram, (void **) &func_glLinkProgram },
  { "glLinkProgramARB", NULL, (void *) wine_glLinkProgramARB, (void **) &func_glLinkProgramARB },
  { "glListParameterfSGIX", NULL, (void *) wine_glListParameterfSGIX, (void **) &func_glListParameterfSGIX },
  { "glListParameterfvSGIX", NULL, (void *) wine_glListParameterfvSGIX, (void **) &func_glListParameterfvSGIX },
  { "glListParameteriSGIX", NULL, (void *) wine_glListParameteriSGIX, (void **) &func_glListParameteriSGIX },
  { "glListParameterivSGIX", NULL, (void *) wine_glListParameterivSGIX, (void **) &func_glListParameterivSGIX },
  { "glLoadIdentityDeformationMapSGIX", NULL, (void *) wine_glLoadIdentityDeformationMapSGIX, (void **) &func_glLoadIdentityDeformationMapSGIX },
  { "glLoadProgramNV", NULL, (void *) wine_glLoadProgramNV, (void **) &func_glLoadProgramNV },
  { "glLoadTransposeMatrixd", NULL, (void *) wine_glLoadTransposeMatrixd, (void **) &func_glLoadTransposeMatrixd },
  { "glLoadTransposeMatrixdARB", NULL, (void *) wine_glLoadTransposeMatrixdARB, (void **) &func_glLoadTransposeMatrixdARB },
  { "glLoadTransposeMatrixf", NULL, (void *) wine_glLoadTransposeMatrixf, (void **) &func_glLoadTransposeMatrixf },
  { "glLoadTransposeMatrixfARB", NULL, (void *) wine_glLoadTransposeMatrixfARB, (void **) &func_glLoadTransposeMatrixfARB },
  { "glLockArraysEXT", NULL, (void *) wine_glLockArraysEXT, (void **) &func_glLockArraysEXT },
  { "glMTexCoord2fSGIS", NULL, (void *) wine_glMTexCoord2fSGIS, (void **) &func_glMTexCoord2fSGIS },
  { "glMTexCoord2fvSGIS", NULL, (void *) wine_glMTexCoord2fvSGIS, (void **) &func_glMTexCoord2fvSGIS },
  { "glMapBuffer", NULL, (void *) wine_glMapBuffer, (void **) &func_glMapBuffer },
  { "glMapBufferARB", NULL, (void *) wine_glMapBufferARB, (void **) &func_glMapBufferARB },
  { "glMapControlPointsNV", NULL, (void *) wine_glMapControlPointsNV, (void **) &func_glMapControlPointsNV },
  { "glMapObjectBufferATI", NULL, (void *) wine_glMapObjectBufferATI, (void **) &func_glMapObjectBufferATI },
  { "glMapParameterfvNV", NULL, (void *) wine_glMapParameterfvNV, (void **) &func_glMapParameterfvNV },
  { "glMapParameterivNV", NULL, (void *) wine_glMapParameterivNV, (void **) &func_glMapParameterivNV },
  { "glMatrixIndexPointerARB", NULL, (void *) wine_glMatrixIndexPointerARB, (void **) &func_glMatrixIndexPointerARB },
  { "glMatrixIndexubvARB", NULL, (void *) wine_glMatrixIndexubvARB, (void **) &func_glMatrixIndexubvARB },
  { "glMatrixIndexuivARB", NULL, (void *) wine_glMatrixIndexuivARB, (void **) &func_glMatrixIndexuivARB },
  { "glMatrixIndexusvARB", NULL, (void *) wine_glMatrixIndexusvARB, (void **) &func_glMatrixIndexusvARB },
  { "glMinmaxEXT", NULL, (void *) wine_glMinmaxEXT, (void **) &func_glMinmaxEXT },
  { "glMultTransposeMatrixd", NULL, (void *) wine_glMultTransposeMatrixd, (void **) &func_glMultTransposeMatrixd },
  { "glMultTransposeMatrixdARB", NULL, (void *) wine_glMultTransposeMatrixdARB, (void **) &func_glMultTransposeMatrixdARB },
  { "glMultTransposeMatrixf", NULL, (void *) wine_glMultTransposeMatrixf, (void **) &func_glMultTransposeMatrixf },
  { "glMultTransposeMatrixfARB", NULL, (void *) wine_glMultTransposeMatrixfARB, (void **) &func_glMultTransposeMatrixfARB },
  { "glMultiDrawArrays", NULL, (void *) wine_glMultiDrawArrays, (void **) &func_glMultiDrawArrays },
  { "glMultiDrawArraysEXT", NULL, (void *) wine_glMultiDrawArraysEXT, (void **) &func_glMultiDrawArraysEXT },
  { "glMultiDrawElementArrayAPPLE", NULL, (void *) wine_glMultiDrawElementArrayAPPLE, (void **) &func_glMultiDrawElementArrayAPPLE },
  { "glMultiDrawElements", NULL, (void *) wine_glMultiDrawElements, (void **) &func_glMultiDrawElements },
  { "glMultiDrawElementsEXT", NULL, (void *) wine_glMultiDrawElementsEXT, (void **) &func_glMultiDrawElementsEXT },
  { "glMultiDrawRangeElementArrayAPPLE", NULL, (void *) wine_glMultiDrawRangeElementArrayAPPLE, (void **) &func_glMultiDrawRangeElementArrayAPPLE },
  { "glMultiModeDrawArraysIBM", NULL, (void *) wine_glMultiModeDrawArraysIBM, (void **) &func_glMultiModeDrawArraysIBM },
  { "glMultiModeDrawElementsIBM", NULL, (void *) wine_glMultiModeDrawElementsIBM, (void **) &func_glMultiModeDrawElementsIBM },
  { "glMultiTexCoord1d", NULL, (void *) wine_glMultiTexCoord1d, (void **) &func_glMultiTexCoord1d },
  { "glMultiTexCoord1dARB", NULL, (void *) wine_glMultiTexCoord1dARB, (void **) &func_glMultiTexCoord1dARB },
  { "glMultiTexCoord1dSGIS", NULL, (void *) wine_glMultiTexCoord1dSGIS, (void **) &func_glMultiTexCoord1dSGIS },
  { "glMultiTexCoord1dv", NULL, (void *) wine_glMultiTexCoord1dv, (void **) &func_glMultiTexCoord1dv },
  { "glMultiTexCoord1dvARB", NULL, (void *) wine_glMultiTexCoord1dvARB, (void **) &func_glMultiTexCoord1dvARB },
  { "glMultiTexCoord1dvSGIS", NULL, (void *) wine_glMultiTexCoord1dvSGIS, (void **) &func_glMultiTexCoord1dvSGIS },
  { "glMultiTexCoord1f", NULL, (void *) wine_glMultiTexCoord1f, (void **) &func_glMultiTexCoord1f },
  { "glMultiTexCoord1fARB", NULL, (void *) wine_glMultiTexCoord1fARB, (void **) &func_glMultiTexCoord1fARB },
  { "glMultiTexCoord1fSGIS", NULL, (void *) wine_glMultiTexCoord1fSGIS, (void **) &func_glMultiTexCoord1fSGIS },
  { "glMultiTexCoord1fv", NULL, (void *) wine_glMultiTexCoord1fv, (void **) &func_glMultiTexCoord1fv },
  { "glMultiTexCoord1fvARB", NULL, (void *) wine_glMultiTexCoord1fvARB, (void **) &func_glMultiTexCoord1fvARB },
  { "glMultiTexCoord1fvSGIS", NULL, (void *) wine_glMultiTexCoord1fvSGIS, (void **) &func_glMultiTexCoord1fvSGIS },
  { "glMultiTexCoord1hNV", NULL, (void *) wine_glMultiTexCoord1hNV, (void **) &func_glMultiTexCoord1hNV },
  { "glMultiTexCoord1hvNV", NULL, (void *) wine_glMultiTexCoord1hvNV, (void **) &func_glMultiTexCoord1hvNV },
  { "glMultiTexCoord1i", NULL, (void *) wine_glMultiTexCoord1i, (void **) &func_glMultiTexCoord1i },
  { "glMultiTexCoord1iARB", NULL, (void *) wine_glMultiTexCoord1iARB, (void **) &func_glMultiTexCoord1iARB },
  { "glMultiTexCoord1iSGIS", NULL, (void *) wine_glMultiTexCoord1iSGIS, (void **) &func_glMultiTexCoord1iSGIS },
  { "glMultiTexCoord1iv", NULL, (void *) wine_glMultiTexCoord1iv, (void **) &func_glMultiTexCoord1iv },
  { "glMultiTexCoord1ivARB", NULL, (void *) wine_glMultiTexCoord1ivARB, (void **) &func_glMultiTexCoord1ivARB },
  { "glMultiTexCoord1ivSGIS", NULL, (void *) wine_glMultiTexCoord1ivSGIS, (void **) &func_glMultiTexCoord1ivSGIS },
  { "glMultiTexCoord1s", NULL, (void *) wine_glMultiTexCoord1s, (void **) &func_glMultiTexCoord1s },
  { "glMultiTexCoord1sARB", NULL, (void *) wine_glMultiTexCoord1sARB, (void **) &func_glMultiTexCoord1sARB },
  { "glMultiTexCoord1sSGIS", NULL, (void *) wine_glMultiTexCoord1sSGIS, (void **) &func_glMultiTexCoord1sSGIS },
  { "glMultiTexCoord1sv", NULL, (void *) wine_glMultiTexCoord1sv, (void **) &func_glMultiTexCoord1sv },
  { "glMultiTexCoord1svARB", NULL, (void *) wine_glMultiTexCoord1svARB, (void **) &func_glMultiTexCoord1svARB },
  { "glMultiTexCoord1svSGIS", NULL, (void *) wine_glMultiTexCoord1svSGIS, (void **) &func_glMultiTexCoord1svSGIS },
  { "glMultiTexCoord2d", NULL, (void *) wine_glMultiTexCoord2d, (void **) &func_glMultiTexCoord2d },
  { "glMultiTexCoord2dARB", NULL, (void *) wine_glMultiTexCoord2dARB, (void **) &func_glMultiTexCoord2dARB },
  { "glMultiTexCoord2dSGIS", NULL, (void *) wine_glMultiTexCoord2dSGIS, (void **) &func_glMultiTexCoord2dSGIS },
  { "glMultiTexCoord2dv", NULL, (void *) wine_glMultiTexCoord2dv, (void **) &func_glMultiTexCoord2dv },
  { "glMultiTexCoord2dvARB", NULL, (void *) wine_glMultiTexCoord2dvARB, (void **) &func_glMultiTexCoord2dvARB },
  { "glMultiTexCoord2dvSGIS", NULL, (void *) wine_glMultiTexCoord2dvSGIS, (void **) &func_glMultiTexCoord2dvSGIS },
  { "glMultiTexCoord2f", NULL, (void *) wine_glMultiTexCoord2f, (void **) &func_glMultiTexCoord2f },
  { "glMultiTexCoord2fARB", NULL, (void *) wine_glMultiTexCoord2fARB, (void **) &func_glMultiTexCoord2fARB },
  { "glMultiTexCoord2fSGIS", NULL, (void *) wine_glMultiTexCoord2fSGIS, (void **) &func_glMultiTexCoord2fSGIS },
  { "glMultiTexCoord2fv", NULL, (void *) wine_glMultiTexCoord2fv, (void **) &func_glMultiTexCoord2fv },
  { "glMultiTexCoord2fvARB", NULL, (void *) wine_glMultiTexCoord2fvARB, (void **) &func_glMultiTexCoord2fvARB },
  { "glMultiTexCoord2fvSGIS", NULL, (void *) wine_glMultiTexCoord2fvSGIS, (void **) &func_glMultiTexCoord2fvSGIS },
  { "glMultiTexCoord2hNV", NULL, (void *) wine_glMultiTexCoord2hNV, (void **) &func_glMultiTexCoord2hNV },
  { "glMultiTexCoord2hvNV", NULL, (void *) wine_glMultiTexCoord2hvNV, (void **) &func_glMultiTexCoord2hvNV },
  { "glMultiTexCoord2i", NULL, (void *) wine_glMultiTexCoord2i, (void **) &func_glMultiTexCoord2i },
  { "glMultiTexCoord2iARB", NULL, (void *) wine_glMultiTexCoord2iARB, (void **) &func_glMultiTexCoord2iARB },
  { "glMultiTexCoord2iSGIS", NULL, (void *) wine_glMultiTexCoord2iSGIS, (void **) &func_glMultiTexCoord2iSGIS },
  { "glMultiTexCoord2iv", NULL, (void *) wine_glMultiTexCoord2iv, (void **) &func_glMultiTexCoord2iv },
  { "glMultiTexCoord2ivARB", NULL, (void *) wine_glMultiTexCoord2ivARB, (void **) &func_glMultiTexCoord2ivARB },
  { "glMultiTexCoord2ivSGIS", NULL, (void *) wine_glMultiTexCoord2ivSGIS, (void **) &func_glMultiTexCoord2ivSGIS },
  { "glMultiTexCoord2s", NULL, (void *) wine_glMultiTexCoord2s, (void **) &func_glMultiTexCoord2s },
  { "glMultiTexCoord2sARB", NULL, (void *) wine_glMultiTexCoord2sARB, (void **) &func_glMultiTexCoord2sARB },
  { "glMultiTexCoord2sSGIS", NULL, (void *) wine_glMultiTexCoord2sSGIS, (void **) &func_glMultiTexCoord2sSGIS },
  { "glMultiTexCoord2sv", NULL, (void *) wine_glMultiTexCoord2sv, (void **) &func_glMultiTexCoord2sv },
  { "glMultiTexCoord2svARB", NULL, (void *) wine_glMultiTexCoord2svARB, (void **) &func_glMultiTexCoord2svARB },
  { "glMultiTexCoord2svSGIS", NULL, (void *) wine_glMultiTexCoord2svSGIS, (void **) &func_glMultiTexCoord2svSGIS },
  { "glMultiTexCoord3d", NULL, (void *) wine_glMultiTexCoord3d, (void **) &func_glMultiTexCoord3d },
  { "glMultiTexCoord3dARB", NULL, (void *) wine_glMultiTexCoord3dARB, (void **) &func_glMultiTexCoord3dARB },
  { "glMultiTexCoord3dSGIS", NULL, (void *) wine_glMultiTexCoord3dSGIS, (void **) &func_glMultiTexCoord3dSGIS },
  { "glMultiTexCoord3dv", NULL, (void *) wine_glMultiTexCoord3dv, (void **) &func_glMultiTexCoord3dv },
  { "glMultiTexCoord3dvARB", NULL, (void *) wine_glMultiTexCoord3dvARB, (void **) &func_glMultiTexCoord3dvARB },
  { "glMultiTexCoord3dvSGIS", NULL, (void *) wine_glMultiTexCoord3dvSGIS, (void **) &func_glMultiTexCoord3dvSGIS },
  { "glMultiTexCoord3f", NULL, (void *) wine_glMultiTexCoord3f, (void **) &func_glMultiTexCoord3f },
  { "glMultiTexCoord3fARB", NULL, (void *) wine_glMultiTexCoord3fARB, (void **) &func_glMultiTexCoord3fARB },
  { "glMultiTexCoord3fSGIS", NULL, (void *) wine_glMultiTexCoord3fSGIS, (void **) &func_glMultiTexCoord3fSGIS },
  { "glMultiTexCoord3fv", NULL, (void *) wine_glMultiTexCoord3fv, (void **) &func_glMultiTexCoord3fv },
  { "glMultiTexCoord3fvARB", NULL, (void *) wine_glMultiTexCoord3fvARB, (void **) &func_glMultiTexCoord3fvARB },
  { "glMultiTexCoord3fvSGIS", NULL, (void *) wine_glMultiTexCoord3fvSGIS, (void **) &func_glMultiTexCoord3fvSGIS },
  { "glMultiTexCoord3hNV", NULL, (void *) wine_glMultiTexCoord3hNV, (void **) &func_glMultiTexCoord3hNV },
  { "glMultiTexCoord3hvNV", NULL, (void *) wine_glMultiTexCoord3hvNV, (void **) &func_glMultiTexCoord3hvNV },
  { "glMultiTexCoord3i", NULL, (void *) wine_glMultiTexCoord3i, (void **) &func_glMultiTexCoord3i },
  { "glMultiTexCoord3iARB", NULL, (void *) wine_glMultiTexCoord3iARB, (void **) &func_glMultiTexCoord3iARB },
  { "glMultiTexCoord3iSGIS", NULL, (void *) wine_glMultiTexCoord3iSGIS, (void **) &func_glMultiTexCoord3iSGIS },
  { "glMultiTexCoord3iv", NULL, (void *) wine_glMultiTexCoord3iv, (void **) &func_glMultiTexCoord3iv },
  { "glMultiTexCoord3ivARB", NULL, (void *) wine_glMultiTexCoord3ivARB, (void **) &func_glMultiTexCoord3ivARB },
  { "glMultiTexCoord3ivSGIS", NULL, (void *) wine_glMultiTexCoord3ivSGIS, (void **) &func_glMultiTexCoord3ivSGIS },
  { "glMultiTexCoord3s", NULL, (void *) wine_glMultiTexCoord3s, (void **) &func_glMultiTexCoord3s },
  { "glMultiTexCoord3sARB", NULL, (void *) wine_glMultiTexCoord3sARB, (void **) &func_glMultiTexCoord3sARB },
  { "glMultiTexCoord3sSGIS", NULL, (void *) wine_glMultiTexCoord3sSGIS, (void **) &func_glMultiTexCoord3sSGIS },
  { "glMultiTexCoord3sv", NULL, (void *) wine_glMultiTexCoord3sv, (void **) &func_glMultiTexCoord3sv },
  { "glMultiTexCoord3svARB", NULL, (void *) wine_glMultiTexCoord3svARB, (void **) &func_glMultiTexCoord3svARB },
  { "glMultiTexCoord3svSGIS", NULL, (void *) wine_glMultiTexCoord3svSGIS, (void **) &func_glMultiTexCoord3svSGIS },
  { "glMultiTexCoord4d", NULL, (void *) wine_glMultiTexCoord4d, (void **) &func_glMultiTexCoord4d },
  { "glMultiTexCoord4dARB", NULL, (void *) wine_glMultiTexCoord4dARB, (void **) &func_glMultiTexCoord4dARB },
  { "glMultiTexCoord4dSGIS", NULL, (void *) wine_glMultiTexCoord4dSGIS, (void **) &func_glMultiTexCoord4dSGIS },
  { "glMultiTexCoord4dv", NULL, (void *) wine_glMultiTexCoord4dv, (void **) &func_glMultiTexCoord4dv },
  { "glMultiTexCoord4dvARB", NULL, (void *) wine_glMultiTexCoord4dvARB, (void **) &func_glMultiTexCoord4dvARB },
  { "glMultiTexCoord4dvSGIS", NULL, (void *) wine_glMultiTexCoord4dvSGIS, (void **) &func_glMultiTexCoord4dvSGIS },
  { "glMultiTexCoord4f", NULL, (void *) wine_glMultiTexCoord4f, (void **) &func_glMultiTexCoord4f },
  { "glMultiTexCoord4fARB", NULL, (void *) wine_glMultiTexCoord4fARB, (void **) &func_glMultiTexCoord4fARB },
  { "glMultiTexCoord4fSGIS", NULL, (void *) wine_glMultiTexCoord4fSGIS, (void **) &func_glMultiTexCoord4fSGIS },
  { "glMultiTexCoord4fv", NULL, (void *) wine_glMultiTexCoord4fv, (void **) &func_glMultiTexCoord4fv },
  { "glMultiTexCoord4fvARB", NULL, (void *) wine_glMultiTexCoord4fvARB, (void **) &func_glMultiTexCoord4fvARB },
  { "glMultiTexCoord4fvSGIS", NULL, (void *) wine_glMultiTexCoord4fvSGIS, (void **) &func_glMultiTexCoord4fvSGIS },
  { "glMultiTexCoord4hNV", NULL, (void *) wine_glMultiTexCoord4hNV, (void **) &func_glMultiTexCoord4hNV },
  { "glMultiTexCoord4hvNV", NULL, (void *) wine_glMultiTexCoord4hvNV, (void **) &func_glMultiTexCoord4hvNV },
  { "glMultiTexCoord4i", NULL, (void *) wine_glMultiTexCoord4i, (void **) &func_glMultiTexCoord4i },
  { "glMultiTexCoord4iARB", NULL, (void *) wine_glMultiTexCoord4iARB, (void **) &func_glMultiTexCoord4iARB },
  { "glMultiTexCoord4iSGIS", NULL, (void *) wine_glMultiTexCoord4iSGIS, (void **) &func_glMultiTexCoord4iSGIS },
  { "glMultiTexCoord4iv", NULL, (void *) wine_glMultiTexCoord4iv, (void **) &func_glMultiTexCoord4iv },
  { "glMultiTexCoord4ivARB", NULL, (void *) wine_glMultiTexCoord4ivARB, (void **) &func_glMultiTexCoord4ivARB },
  { "glMultiTexCoord4ivSGIS", NULL, (void *) wine_glMultiTexCoord4ivSGIS, (void **) &func_glMultiTexCoord4ivSGIS },
  { "glMultiTexCoord4s", NULL, (void *) wine_glMultiTexCoord4s, (void **) &func_glMultiTexCoord4s },
  { "glMultiTexCoord4sARB", NULL, (void *) wine_glMultiTexCoord4sARB, (void **) &func_glMultiTexCoord4sARB },
  { "glMultiTexCoord4sSGIS", NULL, (void *) wine_glMultiTexCoord4sSGIS, (void **) &func_glMultiTexCoord4sSGIS },
  { "glMultiTexCoord4sv", NULL, (void *) wine_glMultiTexCoord4sv, (void **) &func_glMultiTexCoord4sv },
  { "glMultiTexCoord4svARB", NULL, (void *) wine_glMultiTexCoord4svARB, (void **) &func_glMultiTexCoord4svARB },
  { "glMultiTexCoord4svSGIS", NULL, (void *) wine_glMultiTexCoord4svSGIS, (void **) &func_glMultiTexCoord4svSGIS },
  { "glMultiTexCoordPointerSGIS", NULL, (void *) wine_glMultiTexCoordPointerSGIS, (void **) &func_glMultiTexCoordPointerSGIS },
  { "glNewBufferRegion", NULL, (void *) wine_glNewBufferRegion, (void **) &func_glNewBufferRegion },
  { "glNewObjectBufferATI", NULL, (void *) wine_glNewObjectBufferATI, (void **) &func_glNewObjectBufferATI },
  { "glNormal3fVertex3fSUN", NULL, (void *) wine_glNormal3fVertex3fSUN, (void **) &func_glNormal3fVertex3fSUN },
  { "glNormal3fVertex3fvSUN", NULL, (void *) wine_glNormal3fVertex3fvSUN, (void **) &func_glNormal3fVertex3fvSUN },
  { "glNormal3hNV", NULL, (void *) wine_glNormal3hNV, (void **) &func_glNormal3hNV },
  { "glNormal3hvNV", NULL, (void *) wine_glNormal3hvNV, (void **) &func_glNormal3hvNV },
  { "glNormalPointerEXT", NULL, (void *) wine_glNormalPointerEXT, (void **) &func_glNormalPointerEXT },
  { "glNormalPointerListIBM", NULL, (void *) wine_glNormalPointerListIBM, (void **) &func_glNormalPointerListIBM },
  { "glNormalPointervINTEL", NULL, (void *) wine_glNormalPointervINTEL, (void **) &func_glNormalPointervINTEL },
  { "glNormalStream3bATI", NULL, (void *) wine_glNormalStream3bATI, (void **) &func_glNormalStream3bATI },
  { "glNormalStream3bvATI", NULL, (void *) wine_glNormalStream3bvATI, (void **) &func_glNormalStream3bvATI },
  { "glNormalStream3dATI", NULL, (void *) wine_glNormalStream3dATI, (void **) &func_glNormalStream3dATI },
  { "glNormalStream3dvATI", NULL, (void *) wine_glNormalStream3dvATI, (void **) &func_glNormalStream3dvATI },
  { "glNormalStream3fATI", NULL, (void *) wine_glNormalStream3fATI, (void **) &func_glNormalStream3fATI },
  { "glNormalStream3fvATI", NULL, (void *) wine_glNormalStream3fvATI, (void **) &func_glNormalStream3fvATI },
  { "glNormalStream3iATI", NULL, (void *) wine_glNormalStream3iATI, (void **) &func_glNormalStream3iATI },
  { "glNormalStream3ivATI", NULL, (void *) wine_glNormalStream3ivATI, (void **) &func_glNormalStream3ivATI },
  { "glNormalStream3sATI", NULL, (void *) wine_glNormalStream3sATI, (void **) &func_glNormalStream3sATI },
  { "glNormalStream3svATI", NULL, (void *) wine_glNormalStream3svATI, (void **) &func_glNormalStream3svATI },
  { "glPNTrianglesfATI", NULL, (void *) wine_glPNTrianglesfATI, (void **) &func_glPNTrianglesfATI },
  { "glPNTrianglesiATI", NULL, (void *) wine_glPNTrianglesiATI, (void **) &func_glPNTrianglesiATI },
  { "glPassTexCoordATI", NULL, (void *) wine_glPassTexCoordATI, (void **) &func_glPassTexCoordATI },
  { "glPixelDataRangeNV", NULL, (void *) wine_glPixelDataRangeNV, (void **) &func_glPixelDataRangeNV },
  { "glPixelTexGenParameterfSGIS", NULL, (void *) wine_glPixelTexGenParameterfSGIS, (void **) &func_glPixelTexGenParameterfSGIS },
  { "glPixelTexGenParameterfvSGIS", NULL, (void *) wine_glPixelTexGenParameterfvSGIS, (void **) &func_glPixelTexGenParameterfvSGIS },
  { "glPixelTexGenParameteriSGIS", NULL, (void *) wine_glPixelTexGenParameteriSGIS, (void **) &func_glPixelTexGenParameteriSGIS },
  { "glPixelTexGenParameterivSGIS", NULL, (void *) wine_glPixelTexGenParameterivSGIS, (void **) &func_glPixelTexGenParameterivSGIS },
  { "glPixelTexGenSGIX", NULL, (void *) wine_glPixelTexGenSGIX, (void **) &func_glPixelTexGenSGIX },
  { "glPixelTransformParameterfEXT", NULL, (void *) wine_glPixelTransformParameterfEXT, (void **) &func_glPixelTransformParameterfEXT },
  { "glPixelTransformParameterfvEXT", NULL, (void *) wine_glPixelTransformParameterfvEXT, (void **) &func_glPixelTransformParameterfvEXT },
  { "glPixelTransformParameteriEXT", NULL, (void *) wine_glPixelTransformParameteriEXT, (void **) &func_glPixelTransformParameteriEXT },
  { "glPixelTransformParameterivEXT", NULL, (void *) wine_glPixelTransformParameterivEXT, (void **) &func_glPixelTransformParameterivEXT },
  { "glPointParameterf", NULL, (void *) wine_glPointParameterf, (void **) &func_glPointParameterf },
  { "glPointParameterfARB", NULL, (void *) wine_glPointParameterfARB, (void **) &func_glPointParameterfARB },
  { "glPointParameterfEXT", NULL, (void *) wine_glPointParameterfEXT, (void **) &func_glPointParameterfEXT },
  { "glPointParameterfSGIS", NULL, (void *) wine_glPointParameterfSGIS, (void **) &func_glPointParameterfSGIS },
  { "glPointParameterfv", NULL, (void *) wine_glPointParameterfv, (void **) &func_glPointParameterfv },
  { "glPointParameterfvARB", NULL, (void *) wine_glPointParameterfvARB, (void **) &func_glPointParameterfvARB },
  { "glPointParameterfvEXT", NULL, (void *) wine_glPointParameterfvEXT, (void **) &func_glPointParameterfvEXT },
  { "glPointParameterfvSGIS", NULL, (void *) wine_glPointParameterfvSGIS, (void **) &func_glPointParameterfvSGIS },
  { "glPointParameteri", NULL, (void *) wine_glPointParameteri, (void **) &func_glPointParameteri },
  { "glPointParameteriNV", NULL, (void *) wine_glPointParameteriNV, (void **) &func_glPointParameteriNV },
  { "glPointParameteriv", NULL, (void *) wine_glPointParameteriv, (void **) &func_glPointParameteriv },
  { "glPointParameterivNV", NULL, (void *) wine_glPointParameterivNV, (void **) &func_glPointParameterivNV },
  { "glPollAsyncSGIX", NULL, (void *) wine_glPollAsyncSGIX, (void **) &func_glPollAsyncSGIX },
  { "glPollInstrumentsSGIX", NULL, (void *) wine_glPollInstrumentsSGIX, (void **) &func_glPollInstrumentsSGIX },
  { "glPolygonOffsetEXT", NULL, (void *) wine_glPolygonOffsetEXT, (void **) &func_glPolygonOffsetEXT },
  { "glPrimitiveRestartIndexNV", NULL, (void *) wine_glPrimitiveRestartIndexNV, (void **) &func_glPrimitiveRestartIndexNV },
  { "glPrimitiveRestartNV", NULL, (void *) wine_glPrimitiveRestartNV, (void **) &func_glPrimitiveRestartNV },
  { "glPrioritizeTexturesEXT", NULL, (void *) wine_glPrioritizeTexturesEXT, (void **) &func_glPrioritizeTexturesEXT },
  { "glProgramEnvParameter4dARB", NULL, (void *) wine_glProgramEnvParameter4dARB, (void **) &func_glProgramEnvParameter4dARB },
  { "glProgramEnvParameter4dvARB", NULL, (void *) wine_glProgramEnvParameter4dvARB, (void **) &func_glProgramEnvParameter4dvARB },
  { "glProgramEnvParameter4fARB", NULL, (void *) wine_glProgramEnvParameter4fARB, (void **) &func_glProgramEnvParameter4fARB },
  { "glProgramEnvParameter4fvARB", NULL, (void *) wine_glProgramEnvParameter4fvARB, (void **) &func_glProgramEnvParameter4fvARB },
  { "glProgramLocalParameter4dARB", NULL, (void *) wine_glProgramLocalParameter4dARB, (void **) &func_glProgramLocalParameter4dARB },
  { "glProgramLocalParameter4dvARB", NULL, (void *) wine_glProgramLocalParameter4dvARB, (void **) &func_glProgramLocalParameter4dvARB },
  { "glProgramLocalParameter4fARB", NULL, (void *) wine_glProgramLocalParameter4fARB, (void **) &func_glProgramLocalParameter4fARB },
  { "glProgramLocalParameter4fvARB", NULL, (void *) wine_glProgramLocalParameter4fvARB, (void **) &func_glProgramLocalParameter4fvARB },
  { "glProgramNamedParameter4dNV", NULL, (void *) wine_glProgramNamedParameter4dNV, (void **) &func_glProgramNamedParameter4dNV },
  { "glProgramNamedParameter4dvNV", NULL, (void *) wine_glProgramNamedParameter4dvNV, (void **) &func_glProgramNamedParameter4dvNV },
  { "glProgramNamedParameter4fNV", NULL, (void *) wine_glProgramNamedParameter4fNV, (void **) &func_glProgramNamedParameter4fNV },
  { "glProgramNamedParameter4fvNV", NULL, (void *) wine_glProgramNamedParameter4fvNV, (void **) &func_glProgramNamedParameter4fvNV },
  { "glProgramParameter4dNV", NULL, (void *) wine_glProgramParameter4dNV, (void **) &func_glProgramParameter4dNV },
  { "glProgramParameter4dvNV", NULL, (void *) wine_glProgramParameter4dvNV, (void **) &func_glProgramParameter4dvNV },
  { "glProgramParameter4fNV", NULL, (void *) wine_glProgramParameter4fNV, (void **) &func_glProgramParameter4fNV },
  { "glProgramParameter4fvNV", NULL, (void *) wine_glProgramParameter4fvNV, (void **) &func_glProgramParameter4fvNV },
  { "glProgramParameters4dvNV", NULL, (void *) wine_glProgramParameters4dvNV, (void **) &func_glProgramParameters4dvNV },
  { "glProgramParameters4fvNV", NULL, (void *) wine_glProgramParameters4fvNV, (void **) &func_glProgramParameters4fvNV },
  { "glProgramStringARB", NULL, (void *) wine_glProgramStringARB, (void **) &func_glProgramStringARB },
  { "glReadBufferRegion", NULL, (void *) wine_glReadBufferRegion, (void **) &func_glReadBufferRegion },
  { "glReadInstrumentsSGIX", NULL, (void *) wine_glReadInstrumentsSGIX, (void **) &func_glReadInstrumentsSGIX },
  { "glReferencePlaneSGIX", NULL, (void *) wine_glReferencePlaneSGIX, (void **) &func_glReferencePlaneSGIX },
  { "glRenderbufferStorageEXT", NULL, (void *) wine_glRenderbufferStorageEXT, (void **) &func_glRenderbufferStorageEXT },
  { "glReplacementCodePointerSUN", NULL, (void *) wine_glReplacementCodePointerSUN, (void **) &func_glReplacementCodePointerSUN },
  { "glReplacementCodeubSUN", NULL, (void *) wine_glReplacementCodeubSUN, (void **) &func_glReplacementCodeubSUN },
  { "glReplacementCodeubvSUN", NULL, (void *) wine_glReplacementCodeubvSUN, (void **) &func_glReplacementCodeubvSUN },
  { "glReplacementCodeuiColor3fVertex3fSUN", NULL, (void *) wine_glReplacementCodeuiColor3fVertex3fSUN, (void **) &func_glReplacementCodeuiColor3fVertex3fSUN },
  { "glReplacementCodeuiColor3fVertex3fvSUN", NULL, (void *) wine_glReplacementCodeuiColor3fVertex3fvSUN, (void **) &func_glReplacementCodeuiColor3fVertex3fvSUN },
  { "glReplacementCodeuiColor4fNormal3fVertex3fSUN", NULL, (void *) wine_glReplacementCodeuiColor4fNormal3fVertex3fSUN, (void **) &func_glReplacementCodeuiColor4fNormal3fVertex3fSUN },
  { "glReplacementCodeuiColor4fNormal3fVertex3fvSUN", NULL, (void *) wine_glReplacementCodeuiColor4fNormal3fVertex3fvSUN, (void **) &func_glReplacementCodeuiColor4fNormal3fVertex3fvSUN },
  { "glReplacementCodeuiColor4ubVertex3fSUN", NULL, (void *) wine_glReplacementCodeuiColor4ubVertex3fSUN, (void **) &func_glReplacementCodeuiColor4ubVertex3fSUN },
  { "glReplacementCodeuiColor4ubVertex3fvSUN", NULL, (void *) wine_glReplacementCodeuiColor4ubVertex3fvSUN, (void **) &func_glReplacementCodeuiColor4ubVertex3fvSUN },
  { "glReplacementCodeuiNormal3fVertex3fSUN", NULL, (void *) wine_glReplacementCodeuiNormal3fVertex3fSUN, (void **) &func_glReplacementCodeuiNormal3fVertex3fSUN },
  { "glReplacementCodeuiNormal3fVertex3fvSUN", NULL, (void *) wine_glReplacementCodeuiNormal3fVertex3fvSUN, (void **) &func_glReplacementCodeuiNormal3fVertex3fvSUN },
  { "glReplacementCodeuiSUN", NULL, (void *) wine_glReplacementCodeuiSUN, (void **) &func_glReplacementCodeuiSUN },
  { "glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN", NULL, (void *) wine_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN, (void **) &func_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN },
  { "glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN", NULL, (void *) wine_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN, (void **) &func_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN },
  { "glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN", NULL, (void *) wine_glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN, (void **) &func_glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN },
  { "glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN", NULL, (void *) wine_glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN, (void **) &func_glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN },
  { "glReplacementCodeuiTexCoord2fVertex3fSUN", NULL, (void *) wine_glReplacementCodeuiTexCoord2fVertex3fSUN, (void **) &func_glReplacementCodeuiTexCoord2fVertex3fSUN },
  { "glReplacementCodeuiTexCoord2fVertex3fvSUN", NULL, (void *) wine_glReplacementCodeuiTexCoord2fVertex3fvSUN, (void **) &func_glReplacementCodeuiTexCoord2fVertex3fvSUN },
  { "glReplacementCodeuiVertex3fSUN", NULL, (void *) wine_glReplacementCodeuiVertex3fSUN, (void **) &func_glReplacementCodeuiVertex3fSUN },
  { "glReplacementCodeuiVertex3fvSUN", NULL, (void *) wine_glReplacementCodeuiVertex3fvSUN, (void **) &func_glReplacementCodeuiVertex3fvSUN },
  { "glReplacementCodeuivSUN", NULL, (void *) wine_glReplacementCodeuivSUN, (void **) &func_glReplacementCodeuivSUN },
  { "glReplacementCodeusSUN", NULL, (void *) wine_glReplacementCodeusSUN, (void **) &func_glReplacementCodeusSUN },
  { "glReplacementCodeusvSUN", NULL, (void *) wine_glReplacementCodeusvSUN, (void **) &func_glReplacementCodeusvSUN },
  { "glRequestResidentProgramsNV", NULL, (void *) wine_glRequestResidentProgramsNV, (void **) &func_glRequestResidentProgramsNV },
  { "glResetHistogramEXT", NULL, (void *) wine_glResetHistogramEXT, (void **) &func_glResetHistogramEXT },
  { "glResetMinmaxEXT", NULL, (void *) wine_glResetMinmaxEXT, (void **) &func_glResetMinmaxEXT },
  { "glResizeBuffersMESA", NULL, (void *) wine_glResizeBuffersMESA, (void **) &func_glResizeBuffersMESA },
  { "glSampleCoverage", NULL, (void *) wine_glSampleCoverage, (void **) &func_glSampleCoverage },
  { "glSampleCoverageARB", NULL, (void *) wine_glSampleCoverageARB, (void **) &func_glSampleCoverageARB },
  { "glSampleMapATI", NULL, (void *) wine_glSampleMapATI, (void **) &func_glSampleMapATI },
  { "glSampleMaskEXT", NULL, (void *) wine_glSampleMaskEXT, (void **) &func_glSampleMaskEXT },
  { "glSampleMaskSGIS", NULL, (void *) wine_glSampleMaskSGIS, (void **) &func_glSampleMaskSGIS },
  { "glSamplePatternEXT", NULL, (void *) wine_glSamplePatternEXT, (void **) &func_glSamplePatternEXT },
  { "glSamplePatternSGIS", NULL, (void *) wine_glSamplePatternSGIS, (void **) &func_glSamplePatternSGIS },
  { "glSecondaryColor3b", NULL, (void *) wine_glSecondaryColor3b, (void **) &func_glSecondaryColor3b },
  { "glSecondaryColor3bEXT", NULL, (void *) wine_glSecondaryColor3bEXT, (void **) &func_glSecondaryColor3bEXT },
  { "glSecondaryColor3bv", NULL, (void *) wine_glSecondaryColor3bv, (void **) &func_glSecondaryColor3bv },
  { "glSecondaryColor3bvEXT", NULL, (void *) wine_glSecondaryColor3bvEXT, (void **) &func_glSecondaryColor3bvEXT },
  { "glSecondaryColor3d", NULL, (void *) wine_glSecondaryColor3d, (void **) &func_glSecondaryColor3d },
  { "glSecondaryColor3dEXT", NULL, (void *) wine_glSecondaryColor3dEXT, (void **) &func_glSecondaryColor3dEXT },
  { "glSecondaryColor3dv", NULL, (void *) wine_glSecondaryColor3dv, (void **) &func_glSecondaryColor3dv },
  { "glSecondaryColor3dvEXT", NULL, (void *) wine_glSecondaryColor3dvEXT, (void **) &func_glSecondaryColor3dvEXT },
  { "glSecondaryColor3f", NULL, (void *) wine_glSecondaryColor3f, (void **) &func_glSecondaryColor3f },
  { "glSecondaryColor3fEXT", NULL, (void *) wine_glSecondaryColor3fEXT, (void **) &func_glSecondaryColor3fEXT },
  { "glSecondaryColor3fv", NULL, (void *) wine_glSecondaryColor3fv, (void **) &func_glSecondaryColor3fv },
  { "glSecondaryColor3fvEXT", NULL, (void *) wine_glSecondaryColor3fvEXT, (void **) &func_glSecondaryColor3fvEXT },
  { "glSecondaryColor3hNV", NULL, (void *) wine_glSecondaryColor3hNV, (void **) &func_glSecondaryColor3hNV },
  { "glSecondaryColor3hvNV", NULL, (void *) wine_glSecondaryColor3hvNV, (void **) &func_glSecondaryColor3hvNV },
  { "glSecondaryColor3i", NULL, (void *) wine_glSecondaryColor3i, (void **) &func_glSecondaryColor3i },
  { "glSecondaryColor3iEXT", NULL, (void *) wine_glSecondaryColor3iEXT, (void **) &func_glSecondaryColor3iEXT },
  { "glSecondaryColor3iv", NULL, (void *) wine_glSecondaryColor3iv, (void **) &func_glSecondaryColor3iv },
  { "glSecondaryColor3ivEXT", NULL, (void *) wine_glSecondaryColor3ivEXT, (void **) &func_glSecondaryColor3ivEXT },
  { "glSecondaryColor3s", NULL, (void *) wine_glSecondaryColor3s, (void **) &func_glSecondaryColor3s },
  { "glSecondaryColor3sEXT", NULL, (void *) wine_glSecondaryColor3sEXT, (void **) &func_glSecondaryColor3sEXT },
  { "glSecondaryColor3sv", NULL, (void *) wine_glSecondaryColor3sv, (void **) &func_glSecondaryColor3sv },
  { "glSecondaryColor3svEXT", NULL, (void *) wine_glSecondaryColor3svEXT, (void **) &func_glSecondaryColor3svEXT },
  { "glSecondaryColor3ub", NULL, (void *) wine_glSecondaryColor3ub, (void **) &func_glSecondaryColor3ub },
  { "glSecondaryColor3ubEXT", NULL, (void *) wine_glSecondaryColor3ubEXT, (void **) &func_glSecondaryColor3ubEXT },
  { "glSecondaryColor3ubv", NULL, (void *) wine_glSecondaryColor3ubv, (void **) &func_glSecondaryColor3ubv },
  { "glSecondaryColor3ubvEXT", NULL, (void *) wine_glSecondaryColor3ubvEXT, (void **) &func_glSecondaryColor3ubvEXT },
  { "glSecondaryColor3ui", NULL, (void *) wine_glSecondaryColor3ui, (void **) &func_glSecondaryColor3ui },
  { "glSecondaryColor3uiEXT", NULL, (void *) wine_glSecondaryColor3uiEXT, (void **) &func_glSecondaryColor3uiEXT },
  { "glSecondaryColor3uiv", NULL, (void *) wine_glSecondaryColor3uiv, (void **) &func_glSecondaryColor3uiv },
  { "glSecondaryColor3uivEXT", NULL, (void *) wine_glSecondaryColor3uivEXT, (void **) &func_glSecondaryColor3uivEXT },
  { "glSecondaryColor3us", NULL, (void *) wine_glSecondaryColor3us, (void **) &func_glSecondaryColor3us },
  { "glSecondaryColor3usEXT", NULL, (void *) wine_glSecondaryColor3usEXT, (void **) &func_glSecondaryColor3usEXT },
  { "glSecondaryColor3usv", NULL, (void *) wine_glSecondaryColor3usv, (void **) &func_glSecondaryColor3usv },
  { "glSecondaryColor3usvEXT", NULL, (void *) wine_glSecondaryColor3usvEXT, (void **) &func_glSecondaryColor3usvEXT },
  { "glSecondaryColorPointer", NULL, (void *) wine_glSecondaryColorPointer, (void **) &func_glSecondaryColorPointer },
  { "glSecondaryColorPointerEXT", NULL, (void *) wine_glSecondaryColorPointerEXT, (void **) &func_glSecondaryColorPointerEXT },
  { "glSecondaryColorPointerListIBM", NULL, (void *) wine_glSecondaryColorPointerListIBM, (void **) &func_glSecondaryColorPointerListIBM },
  { "glSelectTextureCoordSetSGIS", NULL, (void *) wine_glSelectTextureCoordSetSGIS, (void **) &func_glSelectTextureCoordSetSGIS },
  { "glSelectTextureSGIS", NULL, (void *) wine_glSelectTextureSGIS, (void **) &func_glSelectTextureSGIS },
  { "glSeparableFilter2DEXT", NULL, (void *) wine_glSeparableFilter2DEXT, (void **) &func_glSeparableFilter2DEXT },
  { "glSetFenceAPPLE", NULL, (void *) wine_glSetFenceAPPLE, (void **) &func_glSetFenceAPPLE },
  { "glSetFenceNV", NULL, (void *) wine_glSetFenceNV, (void **) &func_glSetFenceNV },
  { "glSetFragmentShaderConstantATI", NULL, (void *) wine_glSetFragmentShaderConstantATI, (void **) &func_glSetFragmentShaderConstantATI },
  { "glSetInvariantEXT", NULL, (void *) wine_glSetInvariantEXT, (void **) &func_glSetInvariantEXT },
  { "glSetLocalConstantEXT", NULL, (void *) wine_glSetLocalConstantEXT, (void **) &func_glSetLocalConstantEXT },
  { "glShaderOp1EXT", NULL, (void *) wine_glShaderOp1EXT, (void **) &func_glShaderOp1EXT },
  { "glShaderOp2EXT", NULL, (void *) wine_glShaderOp2EXT, (void **) &func_glShaderOp2EXT },
  { "glShaderOp3EXT", NULL, (void *) wine_glShaderOp3EXT, (void **) &func_glShaderOp3EXT },
  { "glShaderSource", NULL, (void *) wine_glShaderSource, (void **) &func_glShaderSource },
  { "glShaderSourceARB", NULL, (void *) wine_glShaderSourceARB, (void **) &func_glShaderSourceARB },
  { "glSharpenTexFuncSGIS", NULL, (void *) wine_glSharpenTexFuncSGIS, (void **) &func_glSharpenTexFuncSGIS },
  { "glSpriteParameterfSGIX", NULL, (void *) wine_glSpriteParameterfSGIX, (void **) &func_glSpriteParameterfSGIX },
  { "glSpriteParameterfvSGIX", NULL, (void *) wine_glSpriteParameterfvSGIX, (void **) &func_glSpriteParameterfvSGIX },
  { "glSpriteParameteriSGIX", NULL, (void *) wine_glSpriteParameteriSGIX, (void **) &func_glSpriteParameteriSGIX },
  { "glSpriteParameterivSGIX", NULL, (void *) wine_glSpriteParameterivSGIX, (void **) &func_glSpriteParameterivSGIX },
  { "glStartInstrumentsSGIX", NULL, (void *) wine_glStartInstrumentsSGIX, (void **) &func_glStartInstrumentsSGIX },
  { "glStencilFuncSeparate", NULL, (void *) wine_glStencilFuncSeparate, (void **) &func_glStencilFuncSeparate },
  { "glStencilFuncSeparateATI", NULL, (void *) wine_glStencilFuncSeparateATI, (void **) &func_glStencilFuncSeparateATI },
  { "glStencilMaskSeparate", NULL, (void *) wine_glStencilMaskSeparate, (void **) &func_glStencilMaskSeparate },
  { "glStencilOpSeparate", NULL, (void *) wine_glStencilOpSeparate, (void **) &func_glStencilOpSeparate },
  { "glStencilOpSeparateATI", NULL, (void *) wine_glStencilOpSeparateATI, (void **) &func_glStencilOpSeparateATI },
  { "glStopInstrumentsSGIX", NULL, (void *) wine_glStopInstrumentsSGIX, (void **) &func_glStopInstrumentsSGIX },
  { "glStringMarkerGREMEDY", NULL, (void *) wine_glStringMarkerGREMEDY, (void **) &func_glStringMarkerGREMEDY },
  { "glSwizzleEXT", NULL, (void *) wine_glSwizzleEXT, (void **) &func_glSwizzleEXT },
  { "glTagSampleBufferSGIX", NULL, (void *) wine_glTagSampleBufferSGIX, (void **) &func_glTagSampleBufferSGIX },
  { "glTangent3bEXT", NULL, (void *) wine_glTangent3bEXT, (void **) &func_glTangent3bEXT },
  { "glTangent3bvEXT", NULL, (void *) wine_glTangent3bvEXT, (void **) &func_glTangent3bvEXT },
  { "glTangent3dEXT", NULL, (void *) wine_glTangent3dEXT, (void **) &func_glTangent3dEXT },
  { "glTangent3dvEXT", NULL, (void *) wine_glTangent3dvEXT, (void **) &func_glTangent3dvEXT },
  { "glTangent3fEXT", NULL, (void *) wine_glTangent3fEXT, (void **) &func_glTangent3fEXT },
  { "glTangent3fvEXT", NULL, (void *) wine_glTangent3fvEXT, (void **) &func_glTangent3fvEXT },
  { "glTangent3iEXT", NULL, (void *) wine_glTangent3iEXT, (void **) &func_glTangent3iEXT },
  { "glTangent3ivEXT", NULL, (void *) wine_glTangent3ivEXT, (void **) &func_glTangent3ivEXT },
  { "glTangent3sEXT", NULL, (void *) wine_glTangent3sEXT, (void **) &func_glTangent3sEXT },
  { "glTangent3svEXT", NULL, (void *) wine_glTangent3svEXT, (void **) &func_glTangent3svEXT },
  { "glTangentPointerEXT", NULL, (void *) wine_glTangentPointerEXT, (void **) &func_glTangentPointerEXT },
  { "glTbufferMask3DFX", NULL, (void *) wine_glTbufferMask3DFX, (void **) &func_glTbufferMask3DFX },
  { "glTestFenceAPPLE", NULL, (void *) wine_glTestFenceAPPLE, (void **) &func_glTestFenceAPPLE },
  { "glTestFenceNV", NULL, (void *) wine_glTestFenceNV, (void **) &func_glTestFenceNV },
  { "glTestObjectAPPLE", NULL, (void *) wine_glTestObjectAPPLE, (void **) &func_glTestObjectAPPLE },
  { "glTexBumpParameterfvATI", NULL, (void *) wine_glTexBumpParameterfvATI, (void **) &func_glTexBumpParameterfvATI },
  { "glTexBumpParameterivATI", NULL, (void *) wine_glTexBumpParameterivATI, (void **) &func_glTexBumpParameterivATI },
  { "glTexCoord1hNV", NULL, (void *) wine_glTexCoord1hNV, (void **) &func_glTexCoord1hNV },
  { "glTexCoord1hvNV", NULL, (void *) wine_glTexCoord1hvNV, (void **) &func_glTexCoord1hvNV },
  { "glTexCoord2fColor3fVertex3fSUN", NULL, (void *) wine_glTexCoord2fColor3fVertex3fSUN, (void **) &func_glTexCoord2fColor3fVertex3fSUN },
  { "glTexCoord2fColor3fVertex3fvSUN", NULL, (void *) wine_glTexCoord2fColor3fVertex3fvSUN, (void **) &func_glTexCoord2fColor3fVertex3fvSUN },
  { "glTexCoord2fColor4fNormal3fVertex3fSUN", NULL, (void *) wine_glTexCoord2fColor4fNormal3fVertex3fSUN, (void **) &func_glTexCoord2fColor4fNormal3fVertex3fSUN },
  { "glTexCoord2fColor4fNormal3fVertex3fvSUN", NULL, (void *) wine_glTexCoord2fColor4fNormal3fVertex3fvSUN, (void **) &func_glTexCoord2fColor4fNormal3fVertex3fvSUN },
  { "glTexCoord2fColor4ubVertex3fSUN", NULL, (void *) wine_glTexCoord2fColor4ubVertex3fSUN, (void **) &func_glTexCoord2fColor4ubVertex3fSUN },
  { "glTexCoord2fColor4ubVertex3fvSUN", NULL, (void *) wine_glTexCoord2fColor4ubVertex3fvSUN, (void **) &func_glTexCoord2fColor4ubVertex3fvSUN },
  { "glTexCoord2fNormal3fVertex3fSUN", NULL, (void *) wine_glTexCoord2fNormal3fVertex3fSUN, (void **) &func_glTexCoord2fNormal3fVertex3fSUN },
  { "glTexCoord2fNormal3fVertex3fvSUN", NULL, (void *) wine_glTexCoord2fNormal3fVertex3fvSUN, (void **) &func_glTexCoord2fNormal3fVertex3fvSUN },
  { "glTexCoord2fVertex3fSUN", NULL, (void *) wine_glTexCoord2fVertex3fSUN, (void **) &func_glTexCoord2fVertex3fSUN },
  { "glTexCoord2fVertex3fvSUN", NULL, (void *) wine_glTexCoord2fVertex3fvSUN, (void **) &func_glTexCoord2fVertex3fvSUN },
  { "glTexCoord2hNV", NULL, (void *) wine_glTexCoord2hNV, (void **) &func_glTexCoord2hNV },
  { "glTexCoord2hvNV", NULL, (void *) wine_glTexCoord2hvNV, (void **) &func_glTexCoord2hvNV },
  { "glTexCoord3hNV", NULL, (void *) wine_glTexCoord3hNV, (void **) &func_glTexCoord3hNV },
  { "glTexCoord3hvNV", NULL, (void *) wine_glTexCoord3hvNV, (void **) &func_glTexCoord3hvNV },
  { "glTexCoord4fColor4fNormal3fVertex4fSUN", NULL, (void *) wine_glTexCoord4fColor4fNormal3fVertex4fSUN, (void **) &func_glTexCoord4fColor4fNormal3fVertex4fSUN },
  { "glTexCoord4fColor4fNormal3fVertex4fvSUN", NULL, (void *) wine_glTexCoord4fColor4fNormal3fVertex4fvSUN, (void **) &func_glTexCoord4fColor4fNormal3fVertex4fvSUN },
  { "glTexCoord4fVertex4fSUN", NULL, (void *) wine_glTexCoord4fVertex4fSUN, (void **) &func_glTexCoord4fVertex4fSUN },
  { "glTexCoord4fVertex4fvSUN", NULL, (void *) wine_glTexCoord4fVertex4fvSUN, (void **) &func_glTexCoord4fVertex4fvSUN },
  { "glTexCoord4hNV", NULL, (void *) wine_glTexCoord4hNV, (void **) &func_glTexCoord4hNV },
  { "glTexCoord4hvNV", NULL, (void *) wine_glTexCoord4hvNV, (void **) &func_glTexCoord4hvNV },
  { "glTexCoordPointerEXT", NULL, (void *) wine_glTexCoordPointerEXT, (void **) &func_glTexCoordPointerEXT },
  { "glTexCoordPointerListIBM", NULL, (void *) wine_glTexCoordPointerListIBM, (void **) &func_glTexCoordPointerListIBM },
  { "glTexCoordPointervINTEL", NULL, (void *) wine_glTexCoordPointervINTEL, (void **) &func_glTexCoordPointervINTEL },
  { "glTexFilterFuncSGIS", NULL, (void *) wine_glTexFilterFuncSGIS, (void **) &func_glTexFilterFuncSGIS },
  { "glTexImage3DEXT", NULL, (void *) wine_glTexImage3DEXT, (void **) &func_glTexImage3DEXT },
  { "glTexImage4DSGIS", NULL, (void *) wine_glTexImage4DSGIS, (void **) &func_glTexImage4DSGIS },
  { "glTexSubImage1DEXT", NULL, (void *) wine_glTexSubImage1DEXT, (void **) &func_glTexSubImage1DEXT },
  { "glTexSubImage2DEXT", NULL, (void *) wine_glTexSubImage2DEXT, (void **) &func_glTexSubImage2DEXT },
  { "glTexSubImage3DEXT", NULL, (void *) wine_glTexSubImage3DEXT, (void **) &func_glTexSubImage3DEXT },
  { "glTexSubImage4DSGIS", NULL, (void *) wine_glTexSubImage4DSGIS, (void **) &func_glTexSubImage4DSGIS },
  { "glTextureColorMaskSGIS", NULL, (void *) wine_glTextureColorMaskSGIS, (void **) &func_glTextureColorMaskSGIS },
  { "glTextureLightEXT", NULL, (void *) wine_glTextureLightEXT, (void **) &func_glTextureLightEXT },
  { "glTextureMaterialEXT", NULL, (void *) wine_glTextureMaterialEXT, (void **) &func_glTextureMaterialEXT },
  { "glTextureNormalEXT", NULL, (void *) wine_glTextureNormalEXT, (void **) &func_glTextureNormalEXT },
  { "glTrackMatrixNV", NULL, (void *) wine_glTrackMatrixNV, (void **) &func_glTrackMatrixNV },
  { "glUniform1f", NULL, (void *) wine_glUniform1f, (void **) &func_glUniform1f },
  { "glUniform1fARB", NULL, (void *) wine_glUniform1fARB, (void **) &func_glUniform1fARB },
  { "glUniform1fv", NULL, (void *) wine_glUniform1fv, (void **) &func_glUniform1fv },
  { "glUniform1fvARB", NULL, (void *) wine_glUniform1fvARB, (void **) &func_glUniform1fvARB },
  { "glUniform1i", NULL, (void *) wine_glUniform1i, (void **) &func_glUniform1i },
  { "glUniform1iARB", NULL, (void *) wine_glUniform1iARB, (void **) &func_glUniform1iARB },
  { "glUniform1iv", NULL, (void *) wine_glUniform1iv, (void **) &func_glUniform1iv },
  { "glUniform1ivARB", NULL, (void *) wine_glUniform1ivARB, (void **) &func_glUniform1ivARB },
  { "glUniform2f", NULL, (void *) wine_glUniform2f, (void **) &func_glUniform2f },
  { "glUniform2fARB", NULL, (void *) wine_glUniform2fARB, (void **) &func_glUniform2fARB },
  { "glUniform2fv", NULL, (void *) wine_glUniform2fv, (void **) &func_glUniform2fv },
  { "glUniform2fvARB", NULL, (void *) wine_glUniform2fvARB, (void **) &func_glUniform2fvARB },
  { "glUniform2i", NULL, (void *) wine_glUniform2i, (void **) &func_glUniform2i },
  { "glUniform2iARB", NULL, (void *) wine_glUniform2iARB, (void **) &func_glUniform2iARB },
  { "glUniform2iv", NULL, (void *) wine_glUniform2iv, (void **) &func_glUniform2iv },
  { "glUniform2ivARB", NULL, (void *) wine_glUniform2ivARB, (void **) &func_glUniform2ivARB },
  { "glUniform3f", NULL, (void *) wine_glUniform3f, (void **) &func_glUniform3f },
  { "glUniform3fARB", NULL, (void *) wine_glUniform3fARB, (void **) &func_glUniform3fARB },
  { "glUniform3fv", NULL, (void *) wine_glUniform3fv, (void **) &func_glUniform3fv },
  { "glUniform3fvARB", NULL, (void *) wine_glUniform3fvARB, (void **) &func_glUniform3fvARB },
  { "glUniform3i", NULL, (void *) wine_glUniform3i, (void **) &func_glUniform3i },
  { "glUniform3iARB", NULL, (void *) wine_glUniform3iARB, (void **) &func_glUniform3iARB },
  { "glUniform3iv", NULL, (void *) wine_glUniform3iv, (void **) &func_glUniform3iv },
  { "glUniform3ivARB", NULL, (void *) wine_glUniform3ivARB, (void **) &func_glUniform3ivARB },
  { "glUniform4f", NULL, (void *) wine_glUniform4f, (void **) &func_glUniform4f },
  { "glUniform4fARB", NULL, (void *) wine_glUniform4fARB, (void **) &func_glUniform4fARB },
  { "glUniform4fv", NULL, (void *) wine_glUniform4fv, (void **) &func_glUniform4fv },
  { "glUniform4fvARB", NULL, (void *) wine_glUniform4fvARB, (void **) &func_glUniform4fvARB },
  { "glUniform4i", NULL, (void *) wine_glUniform4i, (void **) &func_glUniform4i },
  { "glUniform4iARB", NULL, (void *) wine_glUniform4iARB, (void **) &func_glUniform4iARB },
  { "glUniform4iv", NULL, (void *) wine_glUniform4iv, (void **) &func_glUniform4iv },
  { "glUniform4ivARB", NULL, (void *) wine_glUniform4ivARB, (void **) &func_glUniform4ivARB },
  { "glUniformMatrix2fv", NULL, (void *) wine_glUniformMatrix2fv, (void **) &func_glUniformMatrix2fv },
  { "glUniformMatrix2fvARB", NULL, (void *) wine_glUniformMatrix2fvARB, (void **) &func_glUniformMatrix2fvARB },
  { "glUniformMatrix3fv", NULL, (void *) wine_glUniformMatrix3fv, (void **) &func_glUniformMatrix3fv },
  { "glUniformMatrix3fvARB", NULL, (void *) wine_glUniformMatrix3fvARB, (void **) &func_glUniformMatrix3fvARB },
  { "glUniformMatrix4fv", NULL, (void *) wine_glUniformMatrix4fv, (void **) &func_glUniformMatrix4fv },
  { "glUniformMatrix4fvARB", NULL, (void *) wine_glUniformMatrix4fvARB, (void **) &func_glUniformMatrix4fvARB },
  { "glUnlockArraysEXT", NULL, (void *) wine_glUnlockArraysEXT, (void **) &func_glUnlockArraysEXT },
  { "glUnmapBuffer", NULL, (void *) wine_glUnmapBuffer, (void **) &func_glUnmapBuffer },
  { "glUnmapBufferARB", NULL, (void *) wine_glUnmapBufferARB, (void **) &func_glUnmapBufferARB },
  { "glUnmapObjectBufferATI", NULL, (void *) wine_glUnmapObjectBufferATI, (void **) &func_glUnmapObjectBufferATI },
  { "glUpdateObjectBufferATI", NULL, (void *) wine_glUpdateObjectBufferATI, (void **) &func_glUpdateObjectBufferATI },
  { "glUseProgram", NULL, (void *) wine_glUseProgram, (void **) &func_glUseProgram },
  { "glUseProgramObjectARB", NULL, (void *) wine_glUseProgramObjectARB, (void **) &func_glUseProgramObjectARB },
  { "glValidateProgram", NULL, (void *) wine_glValidateProgram, (void **) &func_glValidateProgram },
  { "glValidateProgramARB", NULL, (void *) wine_glValidateProgramARB, (void **) &func_glValidateProgramARB },
  { "glVariantArrayObjectATI", NULL, (void *) wine_glVariantArrayObjectATI, (void **) &func_glVariantArrayObjectATI },
  { "glVariantPointerEXT", NULL, (void *) wine_glVariantPointerEXT, (void **) &func_glVariantPointerEXT },
  { "glVariantbvEXT", NULL, (void *) wine_glVariantbvEXT, (void **) &func_glVariantbvEXT },
  { "glVariantdvEXT", NULL, (void *) wine_glVariantdvEXT, (void **) &func_glVariantdvEXT },
  { "glVariantfvEXT", NULL, (void *) wine_glVariantfvEXT, (void **) &func_glVariantfvEXT },
  { "glVariantivEXT", NULL, (void *) wine_glVariantivEXT, (void **) &func_glVariantivEXT },
  { "glVariantsvEXT", NULL, (void *) wine_glVariantsvEXT, (void **) &func_glVariantsvEXT },
  { "glVariantubvEXT", NULL, (void *) wine_glVariantubvEXT, (void **) &func_glVariantubvEXT },
  { "glVariantuivEXT", NULL, (void *) wine_glVariantuivEXT, (void **) &func_glVariantuivEXT },
  { "glVariantusvEXT", NULL, (void *) wine_glVariantusvEXT, (void **) &func_glVariantusvEXT },
  { "glVertex2hNV", NULL, (void *) wine_glVertex2hNV, (void **) &func_glVertex2hNV },
  { "glVertex2hvNV", NULL, (void *) wine_glVertex2hvNV, (void **) &func_glVertex2hvNV },
  { "glVertex3hNV", NULL, (void *) wine_glVertex3hNV, (void **) &func_glVertex3hNV },
  { "glVertex3hvNV", NULL, (void *) wine_glVertex3hvNV, (void **) &func_glVertex3hvNV },
  { "glVertex4hNV", NULL, (void *) wine_glVertex4hNV, (void **) &func_glVertex4hNV },
  { "glVertex4hvNV", NULL, (void *) wine_glVertex4hvNV, (void **) &func_glVertex4hvNV },
  { "glVertexArrayParameteriAPPLE", NULL, (void *) wine_glVertexArrayParameteriAPPLE, (void **) &func_glVertexArrayParameteriAPPLE },
  { "glVertexArrayRangeAPPLE", NULL, (void *) wine_glVertexArrayRangeAPPLE, (void **) &func_glVertexArrayRangeAPPLE },
  { "glVertexArrayRangeNV", NULL, (void *) wine_glVertexArrayRangeNV, (void **) &func_glVertexArrayRangeNV },
  { "glVertexAttrib1d", NULL, (void *) wine_glVertexAttrib1d, (void **) &func_glVertexAttrib1d },
  { "glVertexAttrib1dARB", NULL, (void *) wine_glVertexAttrib1dARB, (void **) &func_glVertexAttrib1dARB },
  { "glVertexAttrib1dNV", NULL, (void *) wine_glVertexAttrib1dNV, (void **) &func_glVertexAttrib1dNV },
  { "glVertexAttrib1dv", NULL, (void *) wine_glVertexAttrib1dv, (void **) &func_glVertexAttrib1dv },
  { "glVertexAttrib1dvARB", NULL, (void *) wine_glVertexAttrib1dvARB, (void **) &func_glVertexAttrib1dvARB },
  { "glVertexAttrib1dvNV", NULL, (void *) wine_glVertexAttrib1dvNV, (void **) &func_glVertexAttrib1dvNV },
  { "glVertexAttrib1f", NULL, (void *) wine_glVertexAttrib1f, (void **) &func_glVertexAttrib1f },
  { "glVertexAttrib1fARB", NULL, (void *) wine_glVertexAttrib1fARB, (void **) &func_glVertexAttrib1fARB },
  { "glVertexAttrib1fNV", NULL, (void *) wine_glVertexAttrib1fNV, (void **) &func_glVertexAttrib1fNV },
  { "glVertexAttrib1fv", NULL, (void *) wine_glVertexAttrib1fv, (void **) &func_glVertexAttrib1fv },
  { "glVertexAttrib1fvARB", NULL, (void *) wine_glVertexAttrib1fvARB, (void **) &func_glVertexAttrib1fvARB },
  { "glVertexAttrib1fvNV", NULL, (void *) wine_glVertexAttrib1fvNV, (void **) &func_glVertexAttrib1fvNV },
  { "glVertexAttrib1hNV", NULL, (void *) wine_glVertexAttrib1hNV, (void **) &func_glVertexAttrib1hNV },
  { "glVertexAttrib1hvNV", NULL, (void *) wine_glVertexAttrib1hvNV, (void **) &func_glVertexAttrib1hvNV },
  { "glVertexAttrib1s", NULL, (void *) wine_glVertexAttrib1s, (void **) &func_glVertexAttrib1s },
  { "glVertexAttrib1sARB", NULL, (void *) wine_glVertexAttrib1sARB, (void **) &func_glVertexAttrib1sARB },
  { "glVertexAttrib1sNV", NULL, (void *) wine_glVertexAttrib1sNV, (void **) &func_glVertexAttrib1sNV },
  { "glVertexAttrib1sv", NULL, (void *) wine_glVertexAttrib1sv, (void **) &func_glVertexAttrib1sv },
  { "glVertexAttrib1svARB", NULL, (void *) wine_glVertexAttrib1svARB, (void **) &func_glVertexAttrib1svARB },
  { "glVertexAttrib1svNV", NULL, (void *) wine_glVertexAttrib1svNV, (void **) &func_glVertexAttrib1svNV },
  { "glVertexAttrib2d", NULL, (void *) wine_glVertexAttrib2d, (void **) &func_glVertexAttrib2d },
  { "glVertexAttrib2dARB", NULL, (void *) wine_glVertexAttrib2dARB, (void **) &func_glVertexAttrib2dARB },
  { "glVertexAttrib2dNV", NULL, (void *) wine_glVertexAttrib2dNV, (void **) &func_glVertexAttrib2dNV },
  { "glVertexAttrib2dv", NULL, (void *) wine_glVertexAttrib2dv, (void **) &func_glVertexAttrib2dv },
  { "glVertexAttrib2dvARB", NULL, (void *) wine_glVertexAttrib2dvARB, (void **) &func_glVertexAttrib2dvARB },
  { "glVertexAttrib2dvNV", NULL, (void *) wine_glVertexAttrib2dvNV, (void **) &func_glVertexAttrib2dvNV },
  { "glVertexAttrib2f", NULL, (void *) wine_glVertexAttrib2f, (void **) &func_glVertexAttrib2f },
  { "glVertexAttrib2fARB", NULL, (void *) wine_glVertexAttrib2fARB, (void **) &func_glVertexAttrib2fARB },
  { "glVertexAttrib2fNV", NULL, (void *) wine_glVertexAttrib2fNV, (void **) &func_glVertexAttrib2fNV },
  { "glVertexAttrib2fv", NULL, (void *) wine_glVertexAttrib2fv, (void **) &func_glVertexAttrib2fv },
  { "glVertexAttrib2fvARB", NULL, (void *) wine_glVertexAttrib2fvARB, (void **) &func_glVertexAttrib2fvARB },
  { "glVertexAttrib2fvNV", NULL, (void *) wine_glVertexAttrib2fvNV, (void **) &func_glVertexAttrib2fvNV },
  { "glVertexAttrib2hNV", NULL, (void *) wine_glVertexAttrib2hNV, (void **) &func_glVertexAttrib2hNV },
  { "glVertexAttrib2hvNV", NULL, (void *) wine_glVertexAttrib2hvNV, (void **) &func_glVertexAttrib2hvNV },
  { "glVertexAttrib2s", NULL, (void *) wine_glVertexAttrib2s, (void **) &func_glVertexAttrib2s },
  { "glVertexAttrib2sARB", NULL, (void *) wine_glVertexAttrib2sARB, (void **) &func_glVertexAttrib2sARB },
  { "glVertexAttrib2sNV", NULL, (void *) wine_glVertexAttrib2sNV, (void **) &func_glVertexAttrib2sNV },
  { "glVertexAttrib2sv", NULL, (void *) wine_glVertexAttrib2sv, (void **) &func_glVertexAttrib2sv },
  { "glVertexAttrib2svARB", NULL, (void *) wine_glVertexAttrib2svARB, (void **) &func_glVertexAttrib2svARB },
  { "glVertexAttrib2svNV", NULL, (void *) wine_glVertexAttrib2svNV, (void **) &func_glVertexAttrib2svNV },
  { "glVertexAttrib3d", NULL, (void *) wine_glVertexAttrib3d, (void **) &func_glVertexAttrib3d },
  { "glVertexAttrib3dARB", NULL, (void *) wine_glVertexAttrib3dARB, (void **) &func_glVertexAttrib3dARB },
  { "glVertexAttrib3dNV", NULL, (void *) wine_glVertexAttrib3dNV, (void **) &func_glVertexAttrib3dNV },
  { "glVertexAttrib3dv", NULL, (void *) wine_glVertexAttrib3dv, (void **) &func_glVertexAttrib3dv },
  { "glVertexAttrib3dvARB", NULL, (void *) wine_glVertexAttrib3dvARB, (void **) &func_glVertexAttrib3dvARB },
  { "glVertexAttrib3dvNV", NULL, (void *) wine_glVertexAttrib3dvNV, (void **) &func_glVertexAttrib3dvNV },
  { "glVertexAttrib3f", NULL, (void *) wine_glVertexAttrib3f, (void **) &func_glVertexAttrib3f },
  { "glVertexAttrib3fARB", NULL, (void *) wine_glVertexAttrib3fARB, (void **) &func_glVertexAttrib3fARB },
  { "glVertexAttrib3fNV", NULL, (void *) wine_glVertexAttrib3fNV, (void **) &func_glVertexAttrib3fNV },
  { "glVertexAttrib3fv", NULL, (void *) wine_glVertexAttrib3fv, (void **) &func_glVertexAttrib3fv },
  { "glVertexAttrib3fvARB", NULL, (void *) wine_glVertexAttrib3fvARB, (void **) &func_glVertexAttrib3fvARB },
  { "glVertexAttrib3fvNV", NULL, (void *) wine_glVertexAttrib3fvNV, (void **) &func_glVertexAttrib3fvNV },
  { "glVertexAttrib3hNV", NULL, (void *) wine_glVertexAttrib3hNV, (void **) &func_glVertexAttrib3hNV },
  { "glVertexAttrib3hvNV", NULL, (void *) wine_glVertexAttrib3hvNV, (void **) &func_glVertexAttrib3hvNV },
  { "glVertexAttrib3s", NULL, (void *) wine_glVertexAttrib3s, (void **) &func_glVertexAttrib3s },
  { "glVertexAttrib3sARB", NULL, (void *) wine_glVertexAttrib3sARB, (void **) &func_glVertexAttrib3sARB },
  { "glVertexAttrib3sNV", NULL, (void *) wine_glVertexAttrib3sNV, (void **) &func_glVertexAttrib3sNV },
  { "glVertexAttrib3sv", NULL, (void *) wine_glVertexAttrib3sv, (void **) &func_glVertexAttrib3sv },
  { "glVertexAttrib3svARB", NULL, (void *) wine_glVertexAttrib3svARB, (void **) &func_glVertexAttrib3svARB },
  { "glVertexAttrib3svNV", NULL, (void *) wine_glVertexAttrib3svNV, (void **) &func_glVertexAttrib3svNV },
  { "glVertexAttrib4Nbv", NULL, (void *) wine_glVertexAttrib4Nbv, (void **) &func_glVertexAttrib4Nbv },
  { "glVertexAttrib4NbvARB", NULL, (void *) wine_glVertexAttrib4NbvARB, (void **) &func_glVertexAttrib4NbvARB },
  { "glVertexAttrib4Niv", NULL, (void *) wine_glVertexAttrib4Niv, (void **) &func_glVertexAttrib4Niv },
  { "glVertexAttrib4NivARB", NULL, (void *) wine_glVertexAttrib4NivARB, (void **) &func_glVertexAttrib4NivARB },
  { "glVertexAttrib4Nsv", NULL, (void *) wine_glVertexAttrib4Nsv, (void **) &func_glVertexAttrib4Nsv },
  { "glVertexAttrib4NsvARB", NULL, (void *) wine_glVertexAttrib4NsvARB, (void **) &func_glVertexAttrib4NsvARB },
  { "glVertexAttrib4Nub", NULL, (void *) wine_glVertexAttrib4Nub, (void **) &func_glVertexAttrib4Nub },
  { "glVertexAttrib4NubARB", NULL, (void *) wine_glVertexAttrib4NubARB, (void **) &func_glVertexAttrib4NubARB },
  { "glVertexAttrib4Nubv", NULL, (void *) wine_glVertexAttrib4Nubv, (void **) &func_glVertexAttrib4Nubv },
  { "glVertexAttrib4NubvARB", NULL, (void *) wine_glVertexAttrib4NubvARB, (void **) &func_glVertexAttrib4NubvARB },
  { "glVertexAttrib4Nuiv", NULL, (void *) wine_glVertexAttrib4Nuiv, (void **) &func_glVertexAttrib4Nuiv },
  { "glVertexAttrib4NuivARB", NULL, (void *) wine_glVertexAttrib4NuivARB, (void **) &func_glVertexAttrib4NuivARB },
  { "glVertexAttrib4Nusv", NULL, (void *) wine_glVertexAttrib4Nusv, (void **) &func_glVertexAttrib4Nusv },
  { "glVertexAttrib4NusvARB", NULL, (void *) wine_glVertexAttrib4NusvARB, (void **) &func_glVertexAttrib4NusvARB },
  { "glVertexAttrib4bv", NULL, (void *) wine_glVertexAttrib4bv, (void **) &func_glVertexAttrib4bv },
  { "glVertexAttrib4bvARB", NULL, (void *) wine_glVertexAttrib4bvARB, (void **) &func_glVertexAttrib4bvARB },
  { "glVertexAttrib4d", NULL, (void *) wine_glVertexAttrib4d, (void **) &func_glVertexAttrib4d },
  { "glVertexAttrib4dARB", NULL, (void *) wine_glVertexAttrib4dARB, (void **) &func_glVertexAttrib4dARB },
  { "glVertexAttrib4dNV", NULL, (void *) wine_glVertexAttrib4dNV, (void **) &func_glVertexAttrib4dNV },
  { "glVertexAttrib4dv", NULL, (void *) wine_glVertexAttrib4dv, (void **) &func_glVertexAttrib4dv },
  { "glVertexAttrib4dvARB", NULL, (void *) wine_glVertexAttrib4dvARB, (void **) &func_glVertexAttrib4dvARB },
  { "glVertexAttrib4dvNV", NULL, (void *) wine_glVertexAttrib4dvNV, (void **) &func_glVertexAttrib4dvNV },
  { "glVertexAttrib4f", NULL, (void *) wine_glVertexAttrib4f, (void **) &func_glVertexAttrib4f },
  { "glVertexAttrib4fARB", NULL, (void *) wine_glVertexAttrib4fARB, (void **) &func_glVertexAttrib4fARB },
  { "glVertexAttrib4fNV", NULL, (void *) wine_glVertexAttrib4fNV, (void **) &func_glVertexAttrib4fNV },
  { "glVertexAttrib4fv", NULL, (void *) wine_glVertexAttrib4fv, (void **) &func_glVertexAttrib4fv },
  { "glVertexAttrib4fvARB", NULL, (void *) wine_glVertexAttrib4fvARB, (void **) &func_glVertexAttrib4fvARB },
  { "glVertexAttrib4fvNV", NULL, (void *) wine_glVertexAttrib4fvNV, (void **) &func_glVertexAttrib4fvNV },
  { "glVertexAttrib4hNV", NULL, (void *) wine_glVertexAttrib4hNV, (void **) &func_glVertexAttrib4hNV },
  { "glVertexAttrib4hvNV", NULL, (void *) wine_glVertexAttrib4hvNV, (void **) &func_glVertexAttrib4hvNV },
  { "glVertexAttrib4iv", NULL, (void *) wine_glVertexAttrib4iv, (void **) &func_glVertexAttrib4iv },
  { "glVertexAttrib4ivARB", NULL, (void *) wine_glVertexAttrib4ivARB, (void **) &func_glVertexAttrib4ivARB },
  { "glVertexAttrib4s", NULL, (void *) wine_glVertexAttrib4s, (void **) &func_glVertexAttrib4s },
  { "glVertexAttrib4sARB", NULL, (void *) wine_glVertexAttrib4sARB, (void **) &func_glVertexAttrib4sARB },
  { "glVertexAttrib4sNV", NULL, (void *) wine_glVertexAttrib4sNV, (void **) &func_glVertexAttrib4sNV },
  { "glVertexAttrib4sv", NULL, (void *) wine_glVertexAttrib4sv, (void **) &func_glVertexAttrib4sv },
  { "glVertexAttrib4svARB", NULL, (void *) wine_glVertexAttrib4svARB, (void **) &func_glVertexAttrib4svARB },
  { "glVertexAttrib4svNV", NULL, (void *) wine_glVertexAttrib4svNV, (void **) &func_glVertexAttrib4svNV },
  { "glVertexAttrib4ubNV", NULL, (void *) wine_glVertexAttrib4ubNV, (void **) &func_glVertexAttrib4ubNV },
  { "glVertexAttrib4ubv", NULL, (void *) wine_glVertexAttrib4ubv, (void **) &func_glVertexAttrib4ubv },
  { "glVertexAttrib4ubvARB", NULL, (void *) wine_glVertexAttrib4ubvARB, (void **) &func_glVertexAttrib4ubvARB },
  { "glVertexAttrib4ubvNV", NULL, (void *) wine_glVertexAttrib4ubvNV, (void **) &func_glVertexAttrib4ubvNV },
  { "glVertexAttrib4uiv", NULL, (void *) wine_glVertexAttrib4uiv, (void **) &func_glVertexAttrib4uiv },
  { "glVertexAttrib4uivARB", NULL, (void *) wine_glVertexAttrib4uivARB, (void **) &func_glVertexAttrib4uivARB },
  { "glVertexAttrib4usv", NULL, (void *) wine_glVertexAttrib4usv, (void **) &func_glVertexAttrib4usv },
  { "glVertexAttrib4usvARB", NULL, (void *) wine_glVertexAttrib4usvARB, (void **) &func_glVertexAttrib4usvARB },
  { "glVertexAttribArrayObjectATI", NULL, (void *) wine_glVertexAttribArrayObjectATI, (void **) &func_glVertexAttribArrayObjectATI },
  { "glVertexAttribPointer", NULL, (void *) wine_glVertexAttribPointer, (void **) &func_glVertexAttribPointer },
  { "glVertexAttribPointerARB", NULL, (void *) wine_glVertexAttribPointerARB, (void **) &func_glVertexAttribPointerARB },
  { "glVertexAttribPointerNV", NULL, (void *) wine_glVertexAttribPointerNV, (void **) &func_glVertexAttribPointerNV },
  { "glVertexAttribs1dvNV", NULL, (void *) wine_glVertexAttribs1dvNV, (void **) &func_glVertexAttribs1dvNV },
  { "glVertexAttribs1fvNV", NULL, (void *) wine_glVertexAttribs1fvNV, (void **) &func_glVertexAttribs1fvNV },
  { "glVertexAttribs1hvNV", NULL, (void *) wine_glVertexAttribs1hvNV, (void **) &func_glVertexAttribs1hvNV },
  { "glVertexAttribs1svNV", NULL, (void *) wine_glVertexAttribs1svNV, (void **) &func_glVertexAttribs1svNV },
  { "glVertexAttribs2dvNV", NULL, (void *) wine_glVertexAttribs2dvNV, (void **) &func_glVertexAttribs2dvNV },
  { "glVertexAttribs2fvNV", NULL, (void *) wine_glVertexAttribs2fvNV, (void **) &func_glVertexAttribs2fvNV },
  { "glVertexAttribs2hvNV", NULL, (void *) wine_glVertexAttribs2hvNV, (void **) &func_glVertexAttribs2hvNV },
  { "glVertexAttribs2svNV", NULL, (void *) wine_glVertexAttribs2svNV, (void **) &func_glVertexAttribs2svNV },
  { "glVertexAttribs3dvNV", NULL, (void *) wine_glVertexAttribs3dvNV, (void **) &func_glVertexAttribs3dvNV },
  { "glVertexAttribs3fvNV", NULL, (void *) wine_glVertexAttribs3fvNV, (void **) &func_glVertexAttribs3fvNV },
  { "glVertexAttribs3hvNV", NULL, (void *) wine_glVertexAttribs3hvNV, (void **) &func_glVertexAttribs3hvNV },
  { "glVertexAttribs3svNV", NULL, (void *) wine_glVertexAttribs3svNV, (void **) &func_glVertexAttribs3svNV },
  { "glVertexAttribs4dvNV", NULL, (void *) wine_glVertexAttribs4dvNV, (void **) &func_glVertexAttribs4dvNV },
  { "glVertexAttribs4fvNV", NULL, (void *) wine_glVertexAttribs4fvNV, (void **) &func_glVertexAttribs4fvNV },
  { "glVertexAttribs4hvNV", NULL, (void *) wine_glVertexAttribs4hvNV, (void **) &func_glVertexAttribs4hvNV },
  { "glVertexAttribs4svNV", NULL, (void *) wine_glVertexAttribs4svNV, (void **) &func_glVertexAttribs4svNV },
  { "glVertexAttribs4ubvNV", NULL, (void *) wine_glVertexAttribs4ubvNV, (void **) &func_glVertexAttribs4ubvNV },
  { "glVertexBlendARB", NULL, (void *) wine_glVertexBlendARB, (void **) &func_glVertexBlendARB },
  { "glVertexBlendEnvfATI", NULL, (void *) wine_glVertexBlendEnvfATI, (void **) &func_glVertexBlendEnvfATI },
  { "glVertexBlendEnviATI", NULL, (void *) wine_glVertexBlendEnviATI, (void **) &func_glVertexBlendEnviATI },
  { "glVertexPointerEXT", NULL, (void *) wine_glVertexPointerEXT, (void **) &func_glVertexPointerEXT },
  { "glVertexPointerListIBM", NULL, (void *) wine_glVertexPointerListIBM, (void **) &func_glVertexPointerListIBM },
  { "glVertexPointervINTEL", NULL, (void *) wine_glVertexPointervINTEL, (void **) &func_glVertexPointervINTEL },
  { "glVertexStream1dATI", NULL, (void *) wine_glVertexStream1dATI, (void **) &func_glVertexStream1dATI },
  { "glVertexStream1dvATI", NULL, (void *) wine_glVertexStream1dvATI, (void **) &func_glVertexStream1dvATI },
  { "glVertexStream1fATI", NULL, (void *) wine_glVertexStream1fATI, (void **) &func_glVertexStream1fATI },
  { "glVertexStream1fvATI", NULL, (void *) wine_glVertexStream1fvATI, (void **) &func_glVertexStream1fvATI },
  { "glVertexStream1iATI", NULL, (void *) wine_glVertexStream1iATI, (void **) &func_glVertexStream1iATI },
  { "glVertexStream1ivATI", NULL, (void *) wine_glVertexStream1ivATI, (void **) &func_glVertexStream1ivATI },
  { "glVertexStream1sATI", NULL, (void *) wine_glVertexStream1sATI, (void **) &func_glVertexStream1sATI },
  { "glVertexStream1svATI", NULL, (void *) wine_glVertexStream1svATI, (void **) &func_glVertexStream1svATI },
  { "glVertexStream2dATI", NULL, (void *) wine_glVertexStream2dATI, (void **) &func_glVertexStream2dATI },
  { "glVertexStream2dvATI", NULL, (void *) wine_glVertexStream2dvATI, (void **) &func_glVertexStream2dvATI },
  { "glVertexStream2fATI", NULL, (void *) wine_glVertexStream2fATI, (void **) &func_glVertexStream2fATI },
  { "glVertexStream2fvATI", NULL, (void *) wine_glVertexStream2fvATI, (void **) &func_glVertexStream2fvATI },
  { "glVertexStream2iATI", NULL, (void *) wine_glVertexStream2iATI, (void **) &func_glVertexStream2iATI },
  { "glVertexStream2ivATI", NULL, (void *) wine_glVertexStream2ivATI, (void **) &func_glVertexStream2ivATI },
  { "glVertexStream2sATI", NULL, (void *) wine_glVertexStream2sATI, (void **) &func_glVertexStream2sATI },
  { "glVertexStream2svATI", NULL, (void *) wine_glVertexStream2svATI, (void **) &func_glVertexStream2svATI },
  { "glVertexStream3dATI", NULL, (void *) wine_glVertexStream3dATI, (void **) &func_glVertexStream3dATI },
  { "glVertexStream3dvATI", NULL, (void *) wine_glVertexStream3dvATI, (void **) &func_glVertexStream3dvATI },
  { "glVertexStream3fATI", NULL, (void *) wine_glVertexStream3fATI, (void **) &func_glVertexStream3fATI },
  { "glVertexStream3fvATI", NULL, (void *) wine_glVertexStream3fvATI, (void **) &func_glVertexStream3fvATI },
  { "glVertexStream3iATI", NULL, (void *) wine_glVertexStream3iATI, (void **) &func_glVertexStream3iATI },
  { "glVertexStream3ivATI", NULL, (void *) wine_glVertexStream3ivATI, (void **) &func_glVertexStream3ivATI },
  { "glVertexStream3sATI", NULL, (void *) wine_glVertexStream3sATI, (void **) &func_glVertexStream3sATI },
  { "glVertexStream3svATI", NULL, (void *) wine_glVertexStream3svATI, (void **) &func_glVertexStream3svATI },
  { "glVertexStream4dATI", NULL, (void *) wine_glVertexStream4dATI, (void **) &func_glVertexStream4dATI },
  { "glVertexStream4dvATI", NULL, (void *) wine_glVertexStream4dvATI, (void **) &func_glVertexStream4dvATI },
  { "glVertexStream4fATI", NULL, (void *) wine_glVertexStream4fATI, (void **) &func_glVertexStream4fATI },
  { "glVertexStream4fvATI", NULL, (void *) wine_glVertexStream4fvATI, (void **) &func_glVertexStream4fvATI },
  { "glVertexStream4iATI", NULL, (void *) wine_glVertexStream4iATI, (void **) &func_glVertexStream4iATI },
  { "glVertexStream4ivATI", NULL, (void *) wine_glVertexStream4ivATI, (void **) &func_glVertexStream4ivATI },
  { "glVertexStream4sATI", NULL, (void *) wine_glVertexStream4sATI, (void **) &func_glVertexStream4sATI },
  { "glVertexStream4svATI", NULL, (void *) wine_glVertexStream4svATI, (void **) &func_glVertexStream4svATI },
  { "glVertexWeightPointerEXT", NULL, (void *) wine_glVertexWeightPointerEXT, (void **) &func_glVertexWeightPointerEXT },
  { "glVertexWeightfEXT", NULL, (void *) wine_glVertexWeightfEXT, (void **) &func_glVertexWeightfEXT },
  { "glVertexWeightfvEXT", NULL, (void *) wine_glVertexWeightfvEXT, (void **) &func_glVertexWeightfvEXT },
  { "glVertexWeighthNV", NULL, (void *) wine_glVertexWeighthNV, (void **) &func_glVertexWeighthNV },
  { "glVertexWeighthvNV", NULL, (void *) wine_glVertexWeighthvNV, (void **) &func_glVertexWeighthvNV },
  { "glWeightPointerARB", NULL, (void *) wine_glWeightPointerARB, (void **) &func_glWeightPointerARB },
  { "glWeightbvARB", NULL, (void *) wine_glWeightbvARB, (void **) &func_glWeightbvARB },
  { "glWeightdvARB", NULL, (void *) wine_glWeightdvARB, (void **) &func_glWeightdvARB },
  { "glWeightfvARB", NULL, (void *) wine_glWeightfvARB, (void **) &func_glWeightfvARB },
  { "glWeightivARB", NULL, (void *) wine_glWeightivARB, (void **) &func_glWeightivARB },
  { "glWeightsvARB", NULL, (void *) wine_glWeightsvARB, (void **) &func_glWeightsvARB },
  { "glWeightubvARB", NULL, (void *) wine_glWeightubvARB, (void **) &func_glWeightubvARB },
  { "glWeightuivARB", NULL, (void *) wine_glWeightuivARB, (void **) &func_glWeightuivARB },
  { "glWeightusvARB", NULL, (void *) wine_glWeightusvARB, (void **) &func_glWeightusvARB },
  { "glWindowPos2d", NULL, (void *) wine_glWindowPos2d, (void **) &func_glWindowPos2d },
  { "glWindowPos2dARB", NULL, (void *) wine_glWindowPos2dARB, (void **) &func_glWindowPos2dARB },
  { "glWindowPos2dMESA", NULL, (void *) wine_glWindowPos2dMESA, (void **) &func_glWindowPos2dMESA },
  { "glWindowPos2dv", NULL, (void *) wine_glWindowPos2dv, (void **) &func_glWindowPos2dv },
  { "glWindowPos2dvARB", NULL, (void *) wine_glWindowPos2dvARB, (void **) &func_glWindowPos2dvARB },
  { "glWindowPos2dvMESA", NULL, (void *) wine_glWindowPos2dvMESA, (void **) &func_glWindowPos2dvMESA },
  { "glWindowPos2f", NULL, (void *) wine_glWindowPos2f, (void **) &func_glWindowPos2f },
  { "glWindowPos2fARB", NULL, (void *) wine_glWindowPos2fARB, (void **) &func_glWindowPos2fARB },
  { "glWindowPos2fMESA", NULL, (void *) wine_glWindowPos2fMESA, (void **) &func_glWindowPos2fMESA },
  { "glWindowPos2fv", NULL, (void *) wine_glWindowPos2fv, (void **) &func_glWindowPos2fv },
  { "glWindowPos2fvARB", NULL, (void *) wine_glWindowPos2fvARB, (void **) &func_glWindowPos2fvARB },
  { "glWindowPos2fvMESA", NULL, (void *) wine_glWindowPos2fvMESA, (void **) &func_glWindowPos2fvMESA },
  { "glWindowPos2i", NULL, (void *) wine_glWindowPos2i, (void **) &func_glWindowPos2i },
  { "glWindowPos2iARB", NULL, (void *) wine_glWindowPos2iARB, (void **) &func_glWindowPos2iARB },
  { "glWindowPos2iMESA", NULL, (void *) wine_glWindowPos2iMESA, (void **) &func_glWindowPos2iMESA },
  { "glWindowPos2iv", NULL, (void *) wine_glWindowPos2iv, (void **) &func_glWindowPos2iv },
  { "glWindowPos2ivARB", NULL, (void *) wine_glWindowPos2ivARB, (void **) &func_glWindowPos2ivARB },
  { "glWindowPos2ivMESA", NULL, (void *) wine_glWindowPos2ivMESA, (void **) &func_glWindowPos2ivMESA },
  { "glWindowPos2s", NULL, (void *) wine_glWindowPos2s, (void **) &func_glWindowPos2s },
  { "glWindowPos2sARB", NULL, (void *) wine_glWindowPos2sARB, (void **) &func_glWindowPos2sARB },
  { "glWindowPos2sMESA", NULL, (void *) wine_glWindowPos2sMESA, (void **) &func_glWindowPos2sMESA },
  { "glWindowPos2sv", NULL, (void *) wine_glWindowPos2sv, (void **) &func_glWindowPos2sv },
  { "glWindowPos2svARB", NULL, (void *) wine_glWindowPos2svARB, (void **) &func_glWindowPos2svARB },
  { "glWindowPos2svMESA", NULL, (void *) wine_glWindowPos2svMESA, (void **) &func_glWindowPos2svMESA },
  { "glWindowPos3d", NULL, (void *) wine_glWindowPos3d, (void **) &func_glWindowPos3d },
  { "glWindowPos3dARB", NULL, (void *) wine_glWindowPos3dARB, (void **) &func_glWindowPos3dARB },
  { "glWindowPos3dMESA", NULL, (void *) wine_glWindowPos3dMESA, (void **) &func_glWindowPos3dMESA },
  { "glWindowPos3dv", NULL, (void *) wine_glWindowPos3dv, (void **) &func_glWindowPos3dv },
  { "glWindowPos3dvARB", NULL, (void *) wine_glWindowPos3dvARB, (void **) &func_glWindowPos3dvARB },
  { "glWindowPos3dvMESA", NULL, (void *) wine_glWindowPos3dvMESA, (void **) &func_glWindowPos3dvMESA },
  { "glWindowPos3f", NULL, (void *) wine_glWindowPos3f, (void **) &func_glWindowPos3f },
  { "glWindowPos3fARB", NULL, (void *) wine_glWindowPos3fARB, (void **) &func_glWindowPos3fARB },
  { "glWindowPos3fMESA", NULL, (void *) wine_glWindowPos3fMESA, (void **) &func_glWindowPos3fMESA },
  { "glWindowPos3fv", NULL, (void *) wine_glWindowPos3fv, (void **) &func_glWindowPos3fv },
  { "glWindowPos3fvARB", NULL, (void *) wine_glWindowPos3fvARB, (void **) &func_glWindowPos3fvARB },
  { "glWindowPos3fvMESA", NULL, (void *) wine_glWindowPos3fvMESA, (void **) &func_glWindowPos3fvMESA },
  { "glWindowPos3i", NULL, (void *) wine_glWindowPos3i, (void **) &func_glWindowPos3i },
  { "glWindowPos3iARB", NULL, (void *) wine_glWindowPos3iARB, (void **) &func_glWindowPos3iARB },
  { "glWindowPos3iMESA", NULL, (void *) wine_glWindowPos3iMESA, (void **) &func_glWindowPos3iMESA },
  { "glWindowPos3iv", NULL, (void *) wine_glWindowPos3iv, (void **) &func_glWindowPos3iv },
  { "glWindowPos3ivARB", NULL, (void *) wine_glWindowPos3ivARB, (void **) &func_glWindowPos3ivARB },
  { "glWindowPos3ivMESA", NULL, (void *) wine_glWindowPos3ivMESA, (void **) &func_glWindowPos3ivMESA },
  { "glWindowPos3s", NULL, (void *) wine_glWindowPos3s, (void **) &func_glWindowPos3s },
  { "glWindowPos3sARB", NULL, (void *) wine_glWindowPos3sARB, (void **) &func_glWindowPos3sARB },
  { "glWindowPos3sMESA", NULL, (void *) wine_glWindowPos3sMESA, (void **) &func_glWindowPos3sMESA },
  { "glWindowPos3sv", NULL, (void *) wine_glWindowPos3sv, (void **) &func_glWindowPos3sv },
  { "glWindowPos3svARB", NULL, (void *) wine_glWindowPos3svARB, (void **) &func_glWindowPos3svARB },
  { "glWindowPos3svMESA", NULL, (void *) wine_glWindowPos3svMESA, (void **) &func_glWindowPos3svMESA },
  { "glWindowPos4dMESA", NULL, (void *) wine_glWindowPos4dMESA, (void **) &func_glWindowPos4dMESA },
  { "glWindowPos4dvMESA", NULL, (void *) wine_glWindowPos4dvMESA, (void **) &func_glWindowPos4dvMESA },
  { "glWindowPos4fMESA", NULL, (void *) wine_glWindowPos4fMESA, (void **) &func_glWindowPos4fMESA },
  { "glWindowPos4fvMESA", NULL, (void *) wine_glWindowPos4fvMESA, (void **) &func_glWindowPos4fvMESA },
  { "glWindowPos4iMESA", NULL, (void *) wine_glWindowPos4iMESA, (void **) &func_glWindowPos4iMESA },
  { "glWindowPos4ivMESA", NULL, (void *) wine_glWindowPos4ivMESA, (void **) &func_glWindowPos4ivMESA },
  { "glWindowPos4sMESA", NULL, (void *) wine_glWindowPos4sMESA, (void **) &func_glWindowPos4sMESA },
  { "glWindowPos4svMESA", NULL, (void *) wine_glWindowPos4svMESA, (void **) &func_glWindowPos4svMESA },
  { "glWriteMaskEXT", NULL, (void *) wine_glWriteMaskEXT, (void **) &func_glWriteMaskEXT },
  { "wglAllocateMemoryNV", "glXAllocateMemoryNV", (void *) wine_wglAllocateMemoryNV, (void **) &func_wglAllocateMemoryNV },
  { "wglFreeMemoryNV", "glXFreeMemoryNV", (void *) wine_wglFreeMemoryNV, (void **) &func_wglFreeMemoryNV }
};
