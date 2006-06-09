
/* Auto-generated file... Do not edit ! */

#include "config.h"
#include "opengl_ext.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(opengl);

const int extension_registry_size = 1093;
void *extension_funcs[1093];

/* The thunks themselves....*/
static void WINAPI wine_glActiveStencilFaceEXT( GLenum face ) {
  void (*func_glActiveStencilFaceEXT)( GLenum ) = extension_funcs[0];
  TRACE("(%d)\n", face );
  ENTER_GL();
  func_glActiveStencilFaceEXT( face );
  LEAVE_GL();
}

static void WINAPI wine_glActiveTexture( GLenum texture ) {
  void (*func_glActiveTexture)( GLenum ) = extension_funcs[1];
  TRACE("(%d)\n", texture );
  ENTER_GL();
  func_glActiveTexture( texture );
  LEAVE_GL();
}

static void WINAPI wine_glActiveTextureARB( GLenum texture ) {
  void (*func_glActiveTextureARB)( GLenum ) = extension_funcs[2];
  TRACE("(%d)\n", texture );
  ENTER_GL();
  func_glActiveTextureARB( texture );
  LEAVE_GL();
}

static void WINAPI wine_glAlphaFragmentOp1ATI( GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod ) {
  void (*func_glAlphaFragmentOp1ATI)( GLenum, GLuint, GLuint, GLuint, GLuint, GLuint ) = extension_funcs[3];
  TRACE("(%d, %d, %d, %d, %d, %d)\n", op, dst, dstMod, arg1, arg1Rep, arg1Mod );
  ENTER_GL();
  func_glAlphaFragmentOp1ATI( op, dst, dstMod, arg1, arg1Rep, arg1Mod );
  LEAVE_GL();
}

static void WINAPI wine_glAlphaFragmentOp2ATI( GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod ) {
  void (*func_glAlphaFragmentOp2ATI)( GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint ) = extension_funcs[4];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d)\n", op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod );
  ENTER_GL();
  func_glAlphaFragmentOp2ATI( op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod );
  LEAVE_GL();
}

static void WINAPI wine_glAlphaFragmentOp3ATI( GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod ) {
  void (*func_glAlphaFragmentOp3ATI)( GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint ) = extension_funcs[5];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod );
  ENTER_GL();
  func_glAlphaFragmentOp3ATI( op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod );
  LEAVE_GL();
}

static void WINAPI wine_glApplyTextureEXT( GLenum mode ) {
  void (*func_glApplyTextureEXT)( GLenum ) = extension_funcs[6];
  TRACE("(%d)\n", mode );
  ENTER_GL();
  func_glApplyTextureEXT( mode );
  LEAVE_GL();
}

static GLboolean WINAPI wine_glAreProgramsResidentNV( GLsizei n, GLuint* programs, GLboolean* residences ) {
  GLboolean ret_value;
  GLboolean (*func_glAreProgramsResidentNV)( GLsizei, GLuint*, GLboolean* ) = extension_funcs[7];
  TRACE("(%d, %p, %p)\n", n, programs, residences );
  ENTER_GL();
  ret_value = func_glAreProgramsResidentNV( n, programs, residences );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glAreTexturesResidentEXT( GLsizei n, GLuint* textures, GLboolean* residences ) {
  GLboolean ret_value;
  GLboolean (*func_glAreTexturesResidentEXT)( GLsizei, GLuint*, GLboolean* ) = extension_funcs[8];
  TRACE("(%d, %p, %p)\n", n, textures, residences );
  ENTER_GL();
  ret_value = func_glAreTexturesResidentEXT( n, textures, residences );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glArrayElementEXT( GLint i ) {
  void (*func_glArrayElementEXT)( GLint ) = extension_funcs[9];
  TRACE("(%d)\n", i );
  ENTER_GL();
  func_glArrayElementEXT( i );
  LEAVE_GL();
}

static void WINAPI wine_glArrayObjectATI( GLenum array, GLint size, GLenum type, GLsizei stride, GLuint buffer, GLuint offset ) {
  void (*func_glArrayObjectATI)( GLenum, GLint, GLenum, GLsizei, GLuint, GLuint ) = extension_funcs[10];
  TRACE("(%d, %d, %d, %d, %d, %d)\n", array, size, type, stride, buffer, offset );
  ENTER_GL();
  func_glArrayObjectATI( array, size, type, stride, buffer, offset );
  LEAVE_GL();
}

static void WINAPI wine_glAsyncMarkerSGIX( GLuint marker ) {
  void (*func_glAsyncMarkerSGIX)( GLuint ) = extension_funcs[11];
  TRACE("(%d)\n", marker );
  ENTER_GL();
  func_glAsyncMarkerSGIX( marker );
  LEAVE_GL();
}

static void WINAPI wine_glAttachObjectARB( unsigned int containerObj, unsigned int obj ) {
  void (*func_glAttachObjectARB)( unsigned int, unsigned int ) = extension_funcs[12];
  TRACE("(%d, %d)\n", containerObj, obj );
  ENTER_GL();
  func_glAttachObjectARB( containerObj, obj );
  LEAVE_GL();
}

static void WINAPI wine_glAttachShader( GLuint program, GLuint shader ) {
  void (*func_glAttachShader)( GLuint, GLuint ) = extension_funcs[13];
  TRACE("(%d, %d)\n", program, shader );
  ENTER_GL();
  func_glAttachShader( program, shader );
  LEAVE_GL();
}

static void WINAPI wine_glBeginFragmentShaderATI( void ) {
  void (*func_glBeginFragmentShaderATI)( void ) = extension_funcs[14];
  TRACE("()\n");
  ENTER_GL();
  func_glBeginFragmentShaderATI( );
  LEAVE_GL();
}

static void WINAPI wine_glBeginOcclusionQueryNV( GLuint id ) {
  void (*func_glBeginOcclusionQueryNV)( GLuint ) = extension_funcs[15];
  TRACE("(%d)\n", id );
  ENTER_GL();
  func_glBeginOcclusionQueryNV( id );
  LEAVE_GL();
}

static void WINAPI wine_glBeginQuery( GLenum target, GLuint id ) {
  void (*func_glBeginQuery)( GLenum, GLuint ) = extension_funcs[16];
  TRACE("(%d, %d)\n", target, id );
  ENTER_GL();
  func_glBeginQuery( target, id );
  LEAVE_GL();
}

static void WINAPI wine_glBeginQueryARB( GLenum target, GLuint id ) {
  void (*func_glBeginQueryARB)( GLenum, GLuint ) = extension_funcs[17];
  TRACE("(%d, %d)\n", target, id );
  ENTER_GL();
  func_glBeginQueryARB( target, id );
  LEAVE_GL();
}

static void WINAPI wine_glBeginVertexShaderEXT( void ) {
  void (*func_glBeginVertexShaderEXT)( void ) = extension_funcs[18];
  TRACE("()\n");
  ENTER_GL();
  func_glBeginVertexShaderEXT( );
  LEAVE_GL();
}

static void WINAPI wine_glBindAttribLocation( GLuint program, GLuint index, char* name ) {
  void (*func_glBindAttribLocation)( GLuint, GLuint, char* ) = extension_funcs[19];
  TRACE("(%d, %d, %p)\n", program, index, name );
  ENTER_GL();
  func_glBindAttribLocation( program, index, name );
  LEAVE_GL();
}

static void WINAPI wine_glBindAttribLocationARB( unsigned int programObj, GLuint index, char* name ) {
  void (*func_glBindAttribLocationARB)( unsigned int, GLuint, char* ) = extension_funcs[20];
  TRACE("(%d, %d, %p)\n", programObj, index, name );
  ENTER_GL();
  func_glBindAttribLocationARB( programObj, index, name );
  LEAVE_GL();
}

static void WINAPI wine_glBindBuffer( GLenum target, GLuint buffer ) {
  void (*func_glBindBuffer)( GLenum, GLuint ) = extension_funcs[21];
  TRACE("(%d, %d)\n", target, buffer );
  ENTER_GL();
  func_glBindBuffer( target, buffer );
  LEAVE_GL();
}

static void WINAPI wine_glBindBufferARB( GLenum target, GLuint buffer ) {
  void (*func_glBindBufferARB)( GLenum, GLuint ) = extension_funcs[22];
  TRACE("(%d, %d)\n", target, buffer );
  ENTER_GL();
  func_glBindBufferARB( target, buffer );
  LEAVE_GL();
}

static void WINAPI wine_glBindFragmentShaderATI( GLuint id ) {
  void (*func_glBindFragmentShaderATI)( GLuint ) = extension_funcs[23];
  TRACE("(%d)\n", id );
  ENTER_GL();
  func_glBindFragmentShaderATI( id );
  LEAVE_GL();
}

static void WINAPI wine_glBindFramebufferEXT( GLenum target, GLuint framebuffer ) {
  void (*func_glBindFramebufferEXT)( GLenum, GLuint ) = extension_funcs[24];
  TRACE("(%d, %d)\n", target, framebuffer );
  ENTER_GL();
  func_glBindFramebufferEXT( target, framebuffer );
  LEAVE_GL();
}

static GLuint WINAPI wine_glBindLightParameterEXT( GLenum light, GLenum value ) {
  GLuint ret_value;
  GLuint (*func_glBindLightParameterEXT)( GLenum, GLenum ) = extension_funcs[25];
  TRACE("(%d, %d)\n", light, value );
  ENTER_GL();
  ret_value = func_glBindLightParameterEXT( light, value );
  LEAVE_GL();
  return ret_value;
}

static GLuint WINAPI wine_glBindMaterialParameterEXT( GLenum face, GLenum value ) {
  GLuint ret_value;
  GLuint (*func_glBindMaterialParameterEXT)( GLenum, GLenum ) = extension_funcs[26];
  TRACE("(%d, %d)\n", face, value );
  ENTER_GL();
  ret_value = func_glBindMaterialParameterEXT( face, value );
  LEAVE_GL();
  return ret_value;
}

static GLuint WINAPI wine_glBindParameterEXT( GLenum value ) {
  GLuint ret_value;
  GLuint (*func_glBindParameterEXT)( GLenum ) = extension_funcs[27];
  TRACE("(%d)\n", value );
  ENTER_GL();
  ret_value = func_glBindParameterEXT( value );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glBindProgramARB( GLenum target, GLuint program ) {
  void (*func_glBindProgramARB)( GLenum, GLuint ) = extension_funcs[28];
  TRACE("(%d, %d)\n", target, program );
  ENTER_GL();
  func_glBindProgramARB( target, program );
  LEAVE_GL();
}

static void WINAPI wine_glBindProgramNV( GLenum target, GLuint id ) {
  void (*func_glBindProgramNV)( GLenum, GLuint ) = extension_funcs[29];
  TRACE("(%d, %d)\n", target, id );
  ENTER_GL();
  func_glBindProgramNV( target, id );
  LEAVE_GL();
}

static void WINAPI wine_glBindRenderbufferEXT( GLenum target, GLuint renderbuffer ) {
  void (*func_glBindRenderbufferEXT)( GLenum, GLuint ) = extension_funcs[30];
  TRACE("(%d, %d)\n", target, renderbuffer );
  ENTER_GL();
  func_glBindRenderbufferEXT( target, renderbuffer );
  LEAVE_GL();
}

static GLuint WINAPI wine_glBindTexGenParameterEXT( GLenum unit, GLenum coord, GLenum value ) {
  GLuint ret_value;
  GLuint (*func_glBindTexGenParameterEXT)( GLenum, GLenum, GLenum ) = extension_funcs[31];
  TRACE("(%d, %d, %d)\n", unit, coord, value );
  ENTER_GL();
  ret_value = func_glBindTexGenParameterEXT( unit, coord, value );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glBindTextureEXT( GLenum target, GLuint texture ) {
  void (*func_glBindTextureEXT)( GLenum, GLuint ) = extension_funcs[32];
  TRACE("(%d, %d)\n", target, texture );
  ENTER_GL();
  func_glBindTextureEXT( target, texture );
  LEAVE_GL();
}

static GLuint WINAPI wine_glBindTextureUnitParameterEXT( GLenum unit, GLenum value ) {
  GLuint ret_value;
  GLuint (*func_glBindTextureUnitParameterEXT)( GLenum, GLenum ) = extension_funcs[33];
  TRACE("(%d, %d)\n", unit, value );
  ENTER_GL();
  ret_value = func_glBindTextureUnitParameterEXT( unit, value );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glBindVertexArrayAPPLE( GLuint array ) {
  void (*func_glBindVertexArrayAPPLE)( GLuint ) = extension_funcs[34];
  TRACE("(%d)\n", array );
  ENTER_GL();
  func_glBindVertexArrayAPPLE( array );
  LEAVE_GL();
}

static void WINAPI wine_glBindVertexShaderEXT( GLuint id ) {
  void (*func_glBindVertexShaderEXT)( GLuint ) = extension_funcs[35];
  TRACE("(%d)\n", id );
  ENTER_GL();
  func_glBindVertexShaderEXT( id );
  LEAVE_GL();
}

static void WINAPI wine_glBinormal3bEXT( GLbyte bx, GLbyte by, GLbyte bz ) {
  void (*func_glBinormal3bEXT)( GLbyte, GLbyte, GLbyte ) = extension_funcs[36];
  TRACE("(%d, %d, %d)\n", bx, by, bz );
  ENTER_GL();
  func_glBinormal3bEXT( bx, by, bz );
  LEAVE_GL();
}

static void WINAPI wine_glBinormal3bvEXT( GLbyte* v ) {
  void (*func_glBinormal3bvEXT)( GLbyte* ) = extension_funcs[37];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glBinormal3bvEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glBinormal3dEXT( GLdouble bx, GLdouble by, GLdouble bz ) {
  void (*func_glBinormal3dEXT)( GLdouble, GLdouble, GLdouble ) = extension_funcs[38];
  TRACE("(%f, %f, %f)\n", bx, by, bz );
  ENTER_GL();
  func_glBinormal3dEXT( bx, by, bz );
  LEAVE_GL();
}

static void WINAPI wine_glBinormal3dvEXT( GLdouble* v ) {
  void (*func_glBinormal3dvEXT)( GLdouble* ) = extension_funcs[39];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glBinormal3dvEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glBinormal3fEXT( GLfloat bx, GLfloat by, GLfloat bz ) {
  void (*func_glBinormal3fEXT)( GLfloat, GLfloat, GLfloat ) = extension_funcs[40];
  TRACE("(%f, %f, %f)\n", bx, by, bz );
  ENTER_GL();
  func_glBinormal3fEXT( bx, by, bz );
  LEAVE_GL();
}

static void WINAPI wine_glBinormal3fvEXT( GLfloat* v ) {
  void (*func_glBinormal3fvEXT)( GLfloat* ) = extension_funcs[41];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glBinormal3fvEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glBinormal3iEXT( GLint bx, GLint by, GLint bz ) {
  void (*func_glBinormal3iEXT)( GLint, GLint, GLint ) = extension_funcs[42];
  TRACE("(%d, %d, %d)\n", bx, by, bz );
  ENTER_GL();
  func_glBinormal3iEXT( bx, by, bz );
  LEAVE_GL();
}

static void WINAPI wine_glBinormal3ivEXT( GLint* v ) {
  void (*func_glBinormal3ivEXT)( GLint* ) = extension_funcs[43];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glBinormal3ivEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glBinormal3sEXT( GLshort bx, GLshort by, GLshort bz ) {
  void (*func_glBinormal3sEXT)( GLshort, GLshort, GLshort ) = extension_funcs[44];
  TRACE("(%d, %d, %d)\n", bx, by, bz );
  ENTER_GL();
  func_glBinormal3sEXT( bx, by, bz );
  LEAVE_GL();
}

static void WINAPI wine_glBinormal3svEXT( GLshort* v ) {
  void (*func_glBinormal3svEXT)( GLshort* ) = extension_funcs[45];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glBinormal3svEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glBinormalPointerEXT( GLenum type, GLsizei stride, GLvoid* pointer ) {
  void (*func_glBinormalPointerEXT)( GLenum, GLsizei, GLvoid* ) = extension_funcs[46];
  TRACE("(%d, %d, %p)\n", type, stride, pointer );
  ENTER_GL();
  func_glBinormalPointerEXT( type, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glBlendColorEXT( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha ) {
  void (*func_glBlendColorEXT)( GLclampf, GLclampf, GLclampf, GLclampf ) = extension_funcs[47];
  TRACE("(%f, %f, %f, %f)\n", red, green, blue, alpha );
  ENTER_GL();
  func_glBlendColorEXT( red, green, blue, alpha );
  LEAVE_GL();
}

static void WINAPI wine_glBlendEquationEXT( GLenum mode ) {
  void (*func_glBlendEquationEXT)( GLenum ) = extension_funcs[48];
  TRACE("(%d)\n", mode );
  ENTER_GL();
  func_glBlendEquationEXT( mode );
  LEAVE_GL();
}

static void WINAPI wine_glBlendEquationSeparate( GLenum modeRGB, GLenum modeAlpha ) {
  void (*func_glBlendEquationSeparate)( GLenum, GLenum ) = extension_funcs[49];
  TRACE("(%d, %d)\n", modeRGB, modeAlpha );
  ENTER_GL();
  func_glBlendEquationSeparate( modeRGB, modeAlpha );
  LEAVE_GL();
}

static void WINAPI wine_glBlendEquationSeparateEXT( GLenum modeRGB, GLenum modeAlpha ) {
  void (*func_glBlendEquationSeparateEXT)( GLenum, GLenum ) = extension_funcs[50];
  TRACE("(%d, %d)\n", modeRGB, modeAlpha );
  ENTER_GL();
  func_glBlendEquationSeparateEXT( modeRGB, modeAlpha );
  LEAVE_GL();
}

static void WINAPI wine_glBlendFuncSeparate( GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha ) {
  void (*func_glBlendFuncSeparate)( GLenum, GLenum, GLenum, GLenum ) = extension_funcs[51];
  TRACE("(%d, %d, %d, %d)\n", sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha );
  ENTER_GL();
  func_glBlendFuncSeparate( sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha );
  LEAVE_GL();
}

static void WINAPI wine_glBlendFuncSeparateEXT( GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha ) {
  void (*func_glBlendFuncSeparateEXT)( GLenum, GLenum, GLenum, GLenum ) = extension_funcs[52];
  TRACE("(%d, %d, %d, %d)\n", sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha );
  ENTER_GL();
  func_glBlendFuncSeparateEXT( sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha );
  LEAVE_GL();
}

static void WINAPI wine_glBlendFuncSeparateINGR( GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha ) {
  void (*func_glBlendFuncSeparateINGR)( GLenum, GLenum, GLenum, GLenum ) = extension_funcs[53];
  TRACE("(%d, %d, %d, %d)\n", sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha );
  ENTER_GL();
  func_glBlendFuncSeparateINGR( sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha );
  LEAVE_GL();
}

static void WINAPI wine_glBufferData( GLenum target, ptrdiff_t size, GLvoid* data, GLenum usage ) {
  void (*func_glBufferData)( GLenum, ptrdiff_t, GLvoid*, GLenum ) = extension_funcs[54];
  TRACE("(%d, %d, %p, %d)\n", target, size, data, usage );
  ENTER_GL();
  func_glBufferData( target, size, data, usage );
  LEAVE_GL();
}

static void WINAPI wine_glBufferDataARB( GLenum target, ptrdiff_t size, GLvoid* data, GLenum usage ) {
  void (*func_glBufferDataARB)( GLenum, ptrdiff_t, GLvoid*, GLenum ) = extension_funcs[55];
  TRACE("(%d, %d, %p, %d)\n", target, size, data, usage );
  ENTER_GL();
  func_glBufferDataARB( target, size, data, usage );
  LEAVE_GL();
}

static GLuint WINAPI wine_glBufferRegionEnabled( void ) {
  GLuint ret_value;
  GLuint (*func_glBufferRegionEnabled)( void ) = extension_funcs[56];
  TRACE("()\n");
  ENTER_GL();
  ret_value = func_glBufferRegionEnabled( );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glBufferSubData( GLenum target, ptrdiff_t offset, ptrdiff_t size, GLvoid* data ) {
  void (*func_glBufferSubData)( GLenum, ptrdiff_t, ptrdiff_t, GLvoid* ) = extension_funcs[57];
  TRACE("(%d, %d, %d, %p)\n", target, offset, size, data );
  ENTER_GL();
  func_glBufferSubData( target, offset, size, data );
  LEAVE_GL();
}

static void WINAPI wine_glBufferSubDataARB( GLenum target, ptrdiff_t offset, ptrdiff_t size, GLvoid* data ) {
  void (*func_glBufferSubDataARB)( GLenum, ptrdiff_t, ptrdiff_t, GLvoid* ) = extension_funcs[58];
  TRACE("(%d, %d, %d, %p)\n", target, offset, size, data );
  ENTER_GL();
  func_glBufferSubDataARB( target, offset, size, data );
  LEAVE_GL();
}

static GLenum WINAPI wine_glCheckFramebufferStatusEXT( GLenum target ) {
  GLenum ret_value;
  GLenum (*func_glCheckFramebufferStatusEXT)( GLenum ) = extension_funcs[59];
  TRACE("(%d)\n", target );
  ENTER_GL();
  ret_value = func_glCheckFramebufferStatusEXT( target );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glClampColorARB( GLenum target, GLenum clamp ) {
  void (*func_glClampColorARB)( GLenum, GLenum ) = extension_funcs[60];
  TRACE("(%d, %d)\n", target, clamp );
  ENTER_GL();
  func_glClampColorARB( target, clamp );
  LEAVE_GL();
}

static void WINAPI wine_glClientActiveTexture( GLenum texture ) {
  void (*func_glClientActiveTexture)( GLenum ) = extension_funcs[61];
  TRACE("(%d)\n", texture );
  ENTER_GL();
  func_glClientActiveTexture( texture );
  LEAVE_GL();
}

static void WINAPI wine_glClientActiveTextureARB( GLenum texture ) {
  void (*func_glClientActiveTextureARB)( GLenum ) = extension_funcs[62];
  TRACE("(%d)\n", texture );
  ENTER_GL();
  func_glClientActiveTextureARB( texture );
  LEAVE_GL();
}

static void WINAPI wine_glClientActiveVertexStreamATI( GLenum stream ) {
  void (*func_glClientActiveVertexStreamATI)( GLenum ) = extension_funcs[63];
  TRACE("(%d)\n", stream );
  ENTER_GL();
  func_glClientActiveVertexStreamATI( stream );
  LEAVE_GL();
}

static void WINAPI wine_glColor3fVertex3fSUN( GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glColor3fVertex3fSUN)( GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[64];
  TRACE("(%f, %f, %f, %f, %f, %f)\n", r, g, b, x, y, z );
  ENTER_GL();
  func_glColor3fVertex3fSUN( r, g, b, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glColor3fVertex3fvSUN( GLfloat* c, GLfloat* v ) {
  void (*func_glColor3fVertex3fvSUN)( GLfloat*, GLfloat* ) = extension_funcs[65];
  TRACE("(%p, %p)\n", c, v );
  ENTER_GL();
  func_glColor3fVertex3fvSUN( c, v );
  LEAVE_GL();
}

static void WINAPI wine_glColor3hNV( unsigned short red, unsigned short green, unsigned short blue ) {
  void (*func_glColor3hNV)( unsigned short, unsigned short, unsigned short ) = extension_funcs[66];
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glColor3hNV( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glColor3hvNV( unsigned short* v ) {
  void (*func_glColor3hvNV)( unsigned short* ) = extension_funcs[67];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glColor3hvNV( v );
  LEAVE_GL();
}

static void WINAPI wine_glColor4fNormal3fVertex3fSUN( GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glColor4fNormal3fVertex3fSUN)( GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[68];
  TRACE("(%f, %f, %f, %f, %f, %f, %f, %f, %f, %f)\n", r, g, b, a, nx, ny, nz, x, y, z );
  ENTER_GL();
  func_glColor4fNormal3fVertex3fSUN( r, g, b, a, nx, ny, nz, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glColor4fNormal3fVertex3fvSUN( GLfloat* c, GLfloat* n, GLfloat* v ) {
  void (*func_glColor4fNormal3fVertex3fvSUN)( GLfloat*, GLfloat*, GLfloat* ) = extension_funcs[69];
  TRACE("(%p, %p, %p)\n", c, n, v );
  ENTER_GL();
  func_glColor4fNormal3fVertex3fvSUN( c, n, v );
  LEAVE_GL();
}

static void WINAPI wine_glColor4hNV( unsigned short red, unsigned short green, unsigned short blue, unsigned short alpha ) {
  void (*func_glColor4hNV)( unsigned short, unsigned short, unsigned short, unsigned short ) = extension_funcs[70];
  TRACE("(%d, %d, %d, %d)\n", red, green, blue, alpha );
  ENTER_GL();
  func_glColor4hNV( red, green, blue, alpha );
  LEAVE_GL();
}

static void WINAPI wine_glColor4hvNV( unsigned short* v ) {
  void (*func_glColor4hvNV)( unsigned short* ) = extension_funcs[71];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glColor4hvNV( v );
  LEAVE_GL();
}

static void WINAPI wine_glColor4ubVertex2fSUN( GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y ) {
  void (*func_glColor4ubVertex2fSUN)( GLubyte, GLubyte, GLubyte, GLubyte, GLfloat, GLfloat ) = extension_funcs[72];
  TRACE("(%d, %d, %d, %d, %f, %f)\n", r, g, b, a, x, y );
  ENTER_GL();
  func_glColor4ubVertex2fSUN( r, g, b, a, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glColor4ubVertex2fvSUN( GLubyte* c, GLfloat* v ) {
  void (*func_glColor4ubVertex2fvSUN)( GLubyte*, GLfloat* ) = extension_funcs[73];
  TRACE("(%p, %p)\n", c, v );
  ENTER_GL();
  func_glColor4ubVertex2fvSUN( c, v );
  LEAVE_GL();
}

static void WINAPI wine_glColor4ubVertex3fSUN( GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glColor4ubVertex3fSUN)( GLubyte, GLubyte, GLubyte, GLubyte, GLfloat, GLfloat, GLfloat ) = extension_funcs[74];
  TRACE("(%d, %d, %d, %d, %f, %f, %f)\n", r, g, b, a, x, y, z );
  ENTER_GL();
  func_glColor4ubVertex3fSUN( r, g, b, a, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glColor4ubVertex3fvSUN( GLubyte* c, GLfloat* v ) {
  void (*func_glColor4ubVertex3fvSUN)( GLubyte*, GLfloat* ) = extension_funcs[75];
  TRACE("(%p, %p)\n", c, v );
  ENTER_GL();
  func_glColor4ubVertex3fvSUN( c, v );
  LEAVE_GL();
}

static void WINAPI wine_glColorFragmentOp1ATI( GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod ) {
  void (*func_glColorFragmentOp1ATI)( GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint ) = extension_funcs[76];
  TRACE("(%d, %d, %d, %d, %d, %d, %d)\n", op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod );
  ENTER_GL();
  func_glColorFragmentOp1ATI( op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod );
  LEAVE_GL();
}

static void WINAPI wine_glColorFragmentOp2ATI( GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod ) {
  void (*func_glColorFragmentOp2ATI)( GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint ) = extension_funcs[77];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod );
  ENTER_GL();
  func_glColorFragmentOp2ATI( op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod );
  LEAVE_GL();
}

static void WINAPI wine_glColorFragmentOp3ATI( GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod ) {
  void (*func_glColorFragmentOp3ATI)( GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint ) = extension_funcs[78];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod );
  ENTER_GL();
  func_glColorFragmentOp3ATI( op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod );
  LEAVE_GL();
}

static void WINAPI wine_glColorPointerEXT( GLint size, GLenum type, GLsizei stride, GLsizei count, GLvoid* pointer ) {
  void (*func_glColorPointerEXT)( GLint, GLenum, GLsizei, GLsizei, GLvoid* ) = extension_funcs[79];
  TRACE("(%d, %d, %d, %d, %p)\n", size, type, stride, count, pointer );
  ENTER_GL();
  func_glColorPointerEXT( size, type, stride, count, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glColorPointerListIBM( GLint size, GLenum type, GLint stride, GLvoid** pointer, GLint ptrstride ) {
  void (*func_glColorPointerListIBM)( GLint, GLenum, GLint, GLvoid**, GLint ) = extension_funcs[80];
  TRACE("(%d, %d, %d, %p, %d)\n", size, type, stride, pointer, ptrstride );
  ENTER_GL();
  func_glColorPointerListIBM( size, type, stride, pointer, ptrstride );
  LEAVE_GL();
}

static void WINAPI wine_glColorPointervINTEL( GLint size, GLenum type, GLvoid** pointer ) {
  void (*func_glColorPointervINTEL)( GLint, GLenum, GLvoid** ) = extension_funcs[81];
  TRACE("(%d, %d, %p)\n", size, type, pointer );
  ENTER_GL();
  func_glColorPointervINTEL( size, type, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glColorSubTableEXT( GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, GLvoid* data ) {
  void (*func_glColorSubTableEXT)( GLenum, GLsizei, GLsizei, GLenum, GLenum, GLvoid* ) = extension_funcs[82];
  TRACE("(%d, %d, %d, %d, %d, %p)\n", target, start, count, format, type, data );
  ENTER_GL();
  func_glColorSubTableEXT( target, start, count, format, type, data );
  LEAVE_GL();
}

static void WINAPI wine_glColorTableEXT( GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type, GLvoid* table ) {
  void (*func_glColorTableEXT)( GLenum, GLenum, GLsizei, GLenum, GLenum, GLvoid* ) = extension_funcs[83];
  TRACE("(%d, %d, %d, %d, %d, %p)\n", target, internalFormat, width, format, type, table );
  ENTER_GL();
  func_glColorTableEXT( target, internalFormat, width, format, type, table );
  LEAVE_GL();
}

static void WINAPI wine_glColorTableParameterfvSGI( GLenum target, GLenum pname, GLfloat* params ) {
  void (*func_glColorTableParameterfvSGI)( GLenum, GLenum, GLfloat* ) = extension_funcs[84];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glColorTableParameterfvSGI( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glColorTableParameterivSGI( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glColorTableParameterivSGI)( GLenum, GLenum, GLint* ) = extension_funcs[85];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glColorTableParameterivSGI( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glColorTableSGI( GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, GLvoid* table ) {
  void (*func_glColorTableSGI)( GLenum, GLenum, GLsizei, GLenum, GLenum, GLvoid* ) = extension_funcs[86];
  TRACE("(%d, %d, %d, %d, %d, %p)\n", target, internalformat, width, format, type, table );
  ENTER_GL();
  func_glColorTableSGI( target, internalformat, width, format, type, table );
  LEAVE_GL();
}

static void WINAPI wine_glCombinerInputNV( GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage ) {
  void (*func_glCombinerInputNV)( GLenum, GLenum, GLenum, GLenum, GLenum, GLenum ) = extension_funcs[87];
  TRACE("(%d, %d, %d, %d, %d, %d)\n", stage, portion, variable, input, mapping, componentUsage );
  ENTER_GL();
  func_glCombinerInputNV( stage, portion, variable, input, mapping, componentUsage );
  LEAVE_GL();
}

static void WINAPI wine_glCombinerOutputNV( GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct, GLboolean cdDotProduct, GLboolean muxSum ) {
  void (*func_glCombinerOutputNV)( GLenum, GLenum, GLenum, GLenum, GLenum, GLenum, GLenum, GLboolean, GLboolean, GLboolean ) = extension_funcs[88];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", stage, portion, abOutput, cdOutput, sumOutput, scale, bias, abDotProduct, cdDotProduct, muxSum );
  ENTER_GL();
  func_glCombinerOutputNV( stage, portion, abOutput, cdOutput, sumOutput, scale, bias, abDotProduct, cdDotProduct, muxSum );
  LEAVE_GL();
}

static void WINAPI wine_glCombinerParameterfNV( GLenum pname, GLfloat param ) {
  void (*func_glCombinerParameterfNV)( GLenum, GLfloat ) = extension_funcs[89];
  TRACE("(%d, %f)\n", pname, param );
  ENTER_GL();
  func_glCombinerParameterfNV( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glCombinerParameterfvNV( GLenum pname, GLfloat* params ) {
  void (*func_glCombinerParameterfvNV)( GLenum, GLfloat* ) = extension_funcs[90];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glCombinerParameterfvNV( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glCombinerParameteriNV( GLenum pname, GLint param ) {
  void (*func_glCombinerParameteriNV)( GLenum, GLint ) = extension_funcs[91];
  TRACE("(%d, %d)\n", pname, param );
  ENTER_GL();
  func_glCombinerParameteriNV( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glCombinerParameterivNV( GLenum pname, GLint* params ) {
  void (*func_glCombinerParameterivNV)( GLenum, GLint* ) = extension_funcs[92];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glCombinerParameterivNV( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glCombinerStageParameterfvNV( GLenum stage, GLenum pname, GLfloat* params ) {
  void (*func_glCombinerStageParameterfvNV)( GLenum, GLenum, GLfloat* ) = extension_funcs[93];
  TRACE("(%d, %d, %p)\n", stage, pname, params );
  ENTER_GL();
  func_glCombinerStageParameterfvNV( stage, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glCompileShader( GLuint shader ) {
  void (*func_glCompileShader)( GLuint ) = extension_funcs[94];
  TRACE("(%d)\n", shader );
  ENTER_GL();
  func_glCompileShader( shader );
  LEAVE_GL();
}

static void WINAPI wine_glCompileShaderARB( unsigned int shaderObj ) {
  void (*func_glCompileShaderARB)( unsigned int ) = extension_funcs[95];
  TRACE("(%d)\n", shaderObj );
  ENTER_GL();
  func_glCompileShaderARB( shaderObj );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTexImage1D( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, GLvoid* data ) {
  void (*func_glCompressedTexImage1D)( GLenum, GLint, GLenum, GLsizei, GLint, GLsizei, GLvoid* ) = extension_funcs[96];
  TRACE("(%d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, border, imageSize, data );
  ENTER_GL();
  func_glCompressedTexImage1D( target, level, internalformat, width, border, imageSize, data );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTexImage1DARB( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, GLvoid* data ) {
  void (*func_glCompressedTexImage1DARB)( GLenum, GLint, GLenum, GLsizei, GLint, GLsizei, GLvoid* ) = extension_funcs[97];
  TRACE("(%d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, border, imageSize, data );
  ENTER_GL();
  func_glCompressedTexImage1DARB( target, level, internalformat, width, border, imageSize, data );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTexImage2D( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, GLvoid* data ) {
  void (*func_glCompressedTexImage2D)( GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, GLvoid* ) = extension_funcs[98];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, height, border, imageSize, data );
  ENTER_GL();
  func_glCompressedTexImage2D( target, level, internalformat, width, height, border, imageSize, data );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTexImage2DARB( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, GLvoid* data ) {
  void (*func_glCompressedTexImage2DARB)( GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, GLvoid* ) = extension_funcs[99];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, height, border, imageSize, data );
  ENTER_GL();
  func_glCompressedTexImage2DARB( target, level, internalformat, width, height, border, imageSize, data );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTexImage3D( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, GLvoid* data ) {
  void (*func_glCompressedTexImage3D)( GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLsizei, GLvoid* ) = extension_funcs[100];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, height, depth, border, imageSize, data );
  ENTER_GL();
  func_glCompressedTexImage3D( target, level, internalformat, width, height, depth, border, imageSize, data );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTexImage3DARB( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, GLvoid* data ) {
  void (*func_glCompressedTexImage3DARB)( GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLsizei, GLvoid* ) = extension_funcs[101];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, height, depth, border, imageSize, data );
  ENTER_GL();
  func_glCompressedTexImage3DARB( target, level, internalformat, width, height, depth, border, imageSize, data );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTexSubImage1D( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, GLvoid* data ) {
  void (*func_glCompressedTexSubImage1D)( GLenum, GLint, GLint, GLsizei, GLenum, GLsizei, GLvoid* ) = extension_funcs[102];
  TRACE("(%d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, width, format, imageSize, data );
  ENTER_GL();
  func_glCompressedTexSubImage1D( target, level, xoffset, width, format, imageSize, data );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTexSubImage1DARB( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, GLvoid* data ) {
  void (*func_glCompressedTexSubImage1DARB)( GLenum, GLint, GLint, GLsizei, GLenum, GLsizei, GLvoid* ) = extension_funcs[103];
  TRACE("(%d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, width, format, imageSize, data );
  ENTER_GL();
  func_glCompressedTexSubImage1DARB( target, level, xoffset, width, format, imageSize, data );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTexSubImage2D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, GLvoid* data ) {
  void (*func_glCompressedTexSubImage2D)( GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, GLvoid* ) = extension_funcs[104];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, width, height, format, imageSize, data );
  ENTER_GL();
  func_glCompressedTexSubImage2D( target, level, xoffset, yoffset, width, height, format, imageSize, data );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTexSubImage2DARB( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, GLvoid* data ) {
  void (*func_glCompressedTexSubImage2DARB)( GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, GLvoid* ) = extension_funcs[105];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, width, height, format, imageSize, data );
  ENTER_GL();
  func_glCompressedTexSubImage2DARB( target, level, xoffset, yoffset, width, height, format, imageSize, data );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTexSubImage3D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, GLvoid* data ) {
  void (*func_glCompressedTexSubImage3D)( GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, GLvoid* ) = extension_funcs[106];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data );
  ENTER_GL();
  func_glCompressedTexSubImage3D( target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTexSubImage3DARB( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, GLvoid* data ) {
  void (*func_glCompressedTexSubImage3DARB)( GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, GLvoid* ) = extension_funcs[107];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data );
  ENTER_GL();
  func_glCompressedTexSubImage3DARB( target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data );
  LEAVE_GL();
}

static void WINAPI wine_glConvolutionFilter1DEXT( GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, GLvoid* image ) {
  void (*func_glConvolutionFilter1DEXT)( GLenum, GLenum, GLsizei, GLenum, GLenum, GLvoid* ) = extension_funcs[108];
  TRACE("(%d, %d, %d, %d, %d, %p)\n", target, internalformat, width, format, type, image );
  ENTER_GL();
  func_glConvolutionFilter1DEXT( target, internalformat, width, format, type, image );
  LEAVE_GL();
}

static void WINAPI wine_glConvolutionFilter2DEXT( GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* image ) {
  void (*func_glConvolutionFilter2DEXT)( GLenum, GLenum, GLsizei, GLsizei, GLenum, GLenum, GLvoid* ) = extension_funcs[109];
  TRACE("(%d, %d, %d, %d, %d, %d, %p)\n", target, internalformat, width, height, format, type, image );
  ENTER_GL();
  func_glConvolutionFilter2DEXT( target, internalformat, width, height, format, type, image );
  LEAVE_GL();
}

static void WINAPI wine_glConvolutionParameterfEXT( GLenum target, GLenum pname, GLfloat params ) {
  void (*func_glConvolutionParameterfEXT)( GLenum, GLenum, GLfloat ) = extension_funcs[110];
  TRACE("(%d, %d, %f)\n", target, pname, params );
  ENTER_GL();
  func_glConvolutionParameterfEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glConvolutionParameterfvEXT( GLenum target, GLenum pname, GLfloat* params ) {
  void (*func_glConvolutionParameterfvEXT)( GLenum, GLenum, GLfloat* ) = extension_funcs[111];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glConvolutionParameterfvEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glConvolutionParameteriEXT( GLenum target, GLenum pname, GLint params ) {
  void (*func_glConvolutionParameteriEXT)( GLenum, GLenum, GLint ) = extension_funcs[112];
  TRACE("(%d, %d, %d)\n", target, pname, params );
  ENTER_GL();
  func_glConvolutionParameteriEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glConvolutionParameterivEXT( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glConvolutionParameterivEXT)( GLenum, GLenum, GLint* ) = extension_funcs[113];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glConvolutionParameterivEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glCopyColorSubTableEXT( GLenum target, GLsizei start, GLint x, GLint y, GLsizei width ) {
  void (*func_glCopyColorSubTableEXT)( GLenum, GLsizei, GLint, GLint, GLsizei ) = extension_funcs[114];
  TRACE("(%d, %d, %d, %d, %d)\n", target, start, x, y, width );
  ENTER_GL();
  func_glCopyColorSubTableEXT( target, start, x, y, width );
  LEAVE_GL();
}

static void WINAPI wine_glCopyColorTableSGI( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width ) {
  void (*func_glCopyColorTableSGI)( GLenum, GLenum, GLint, GLint, GLsizei ) = extension_funcs[115];
  TRACE("(%d, %d, %d, %d, %d)\n", target, internalformat, x, y, width );
  ENTER_GL();
  func_glCopyColorTableSGI( target, internalformat, x, y, width );
  LEAVE_GL();
}

static void WINAPI wine_glCopyConvolutionFilter1DEXT( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width ) {
  void (*func_glCopyConvolutionFilter1DEXT)( GLenum, GLenum, GLint, GLint, GLsizei ) = extension_funcs[116];
  TRACE("(%d, %d, %d, %d, %d)\n", target, internalformat, x, y, width );
  ENTER_GL();
  func_glCopyConvolutionFilter1DEXT( target, internalformat, x, y, width );
  LEAVE_GL();
}

static void WINAPI wine_glCopyConvolutionFilter2DEXT( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height ) {
  void (*func_glCopyConvolutionFilter2DEXT)( GLenum, GLenum, GLint, GLint, GLsizei, GLsizei ) = extension_funcs[117];
  TRACE("(%d, %d, %d, %d, %d, %d)\n", target, internalformat, x, y, width, height );
  ENTER_GL();
  func_glCopyConvolutionFilter2DEXT( target, internalformat, x, y, width, height );
  LEAVE_GL();
}

static void WINAPI wine_glCopyTexImage1DEXT( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border ) {
  void (*func_glCopyTexImage1DEXT)( GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLint ) = extension_funcs[118];
  TRACE("(%d, %d, %d, %d, %d, %d, %d)\n", target, level, internalformat, x, y, width, border );
  ENTER_GL();
  func_glCopyTexImage1DEXT( target, level, internalformat, x, y, width, border );
  LEAVE_GL();
}

static void WINAPI wine_glCopyTexImage2DEXT( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border ) {
  void (*func_glCopyTexImage2DEXT)( GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint ) = extension_funcs[119];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d)\n", target, level, internalformat, x, y, width, height, border );
  ENTER_GL();
  func_glCopyTexImage2DEXT( target, level, internalformat, x, y, width, height, border );
  LEAVE_GL();
}

static void WINAPI wine_glCopyTexSubImage1DEXT( GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width ) {
  void (*func_glCopyTexSubImage1DEXT)( GLenum, GLint, GLint, GLint, GLint, GLsizei ) = extension_funcs[120];
  TRACE("(%d, %d, %d, %d, %d, %d)\n", target, level, xoffset, x, y, width );
  ENTER_GL();
  func_glCopyTexSubImage1DEXT( target, level, xoffset, x, y, width );
  LEAVE_GL();
}

static void WINAPI wine_glCopyTexSubImage2DEXT( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height ) {
  void (*func_glCopyTexSubImage2DEXT)( GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei ) = extension_funcs[121];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d)\n", target, level, xoffset, yoffset, x, y, width, height );
  ENTER_GL();
  func_glCopyTexSubImage2DEXT( target, level, xoffset, yoffset, x, y, width, height );
  LEAVE_GL();
}

static void WINAPI wine_glCopyTexSubImage3DEXT( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height ) {
  void (*func_glCopyTexSubImage3DEXT)( GLenum, GLint, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei ) = extension_funcs[122];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d)\n", target, level, xoffset, yoffset, zoffset, x, y, width, height );
  ENTER_GL();
  func_glCopyTexSubImage3DEXT( target, level, xoffset, yoffset, zoffset, x, y, width, height );
  LEAVE_GL();
}

static GLuint WINAPI wine_glCreateProgram( void ) {
  GLuint ret_value;
  GLuint (*func_glCreateProgram)( void ) = extension_funcs[123];
  TRACE("()\n");
  ENTER_GL();
  ret_value = func_glCreateProgram( );
  LEAVE_GL();
  return ret_value;
}

static unsigned int WINAPI wine_glCreateProgramObjectARB( void ) {
  unsigned int ret_value;
  unsigned int (*func_glCreateProgramObjectARB)( void ) = extension_funcs[124];
  TRACE("()\n");
  ENTER_GL();
  ret_value = func_glCreateProgramObjectARB( );
  LEAVE_GL();
  return ret_value;
}

static GLuint WINAPI wine_glCreateShader( GLenum type ) {
  GLuint ret_value;
  GLuint (*func_glCreateShader)( GLenum ) = extension_funcs[125];
  TRACE("(%d)\n", type );
  ENTER_GL();
  ret_value = func_glCreateShader( type );
  LEAVE_GL();
  return ret_value;
}

static unsigned int WINAPI wine_glCreateShaderObjectARB( GLenum shaderType ) {
  unsigned int ret_value;
  unsigned int (*func_glCreateShaderObjectARB)( GLenum ) = extension_funcs[126];
  TRACE("(%d)\n", shaderType );
  ENTER_GL();
  ret_value = func_glCreateShaderObjectARB( shaderType );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glCullParameterdvEXT( GLenum pname, GLdouble* params ) {
  void (*func_glCullParameterdvEXT)( GLenum, GLdouble* ) = extension_funcs[127];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glCullParameterdvEXT( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glCullParameterfvEXT( GLenum pname, GLfloat* params ) {
  void (*func_glCullParameterfvEXT)( GLenum, GLfloat* ) = extension_funcs[128];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glCullParameterfvEXT( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glCurrentPaletteMatrixARB( GLint index ) {
  void (*func_glCurrentPaletteMatrixARB)( GLint ) = extension_funcs[129];
  TRACE("(%d)\n", index );
  ENTER_GL();
  func_glCurrentPaletteMatrixARB( index );
  LEAVE_GL();
}

static void WINAPI wine_glDeformSGIX( GLint mask ) {
  void (*func_glDeformSGIX)( GLint ) = extension_funcs[130];
  TRACE("(%d)\n", mask );
  ENTER_GL();
  func_glDeformSGIX( mask );
  LEAVE_GL();
}

static void WINAPI wine_glDeformationMap3dSGIX( GLint target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, GLdouble w1, GLdouble w2, GLint wstride, GLint worder, GLdouble* points ) {
  void (*func_glDeformationMap3dSGIX)( GLint, GLdouble, GLdouble, GLint, GLint, GLdouble, GLdouble, GLint, GLint, GLdouble, GLdouble, GLint, GLint, GLdouble* ) = extension_funcs[131];
  TRACE("(%d, %f, %f, %d, %d, %f, %f, %d, %d, %f, %f, %d, %d, %p)\n", target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points );
  ENTER_GL();
  func_glDeformationMap3dSGIX( target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points );
  LEAVE_GL();
}

static void WINAPI wine_glDeformationMap3fSGIX( GLint target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, GLfloat w1, GLfloat w2, GLint wstride, GLint worder, GLfloat* points ) {
  void (*func_glDeformationMap3fSGIX)( GLint, GLfloat, GLfloat, GLint, GLint, GLfloat, GLfloat, GLint, GLint, GLfloat, GLfloat, GLint, GLint, GLfloat* ) = extension_funcs[132];
  TRACE("(%d, %f, %f, %d, %d, %f, %f, %d, %d, %f, %f, %d, %d, %p)\n", target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points );
  ENTER_GL();
  func_glDeformationMap3fSGIX( target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteAsyncMarkersSGIX( GLuint marker, GLsizei range ) {
  void (*func_glDeleteAsyncMarkersSGIX)( GLuint, GLsizei ) = extension_funcs[133];
  TRACE("(%d, %d)\n", marker, range );
  ENTER_GL();
  func_glDeleteAsyncMarkersSGIX( marker, range );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteBufferRegion( GLenum region ) {
  void (*func_glDeleteBufferRegion)( GLenum ) = extension_funcs[134];
  TRACE("(%d)\n", region );
  ENTER_GL();
  func_glDeleteBufferRegion( region );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteBuffers( GLsizei n, GLuint* buffers ) {
  void (*func_glDeleteBuffers)( GLsizei, GLuint* ) = extension_funcs[135];
  TRACE("(%d, %p)\n", n, buffers );
  ENTER_GL();
  func_glDeleteBuffers( n, buffers );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteBuffersARB( GLsizei n, GLuint* buffers ) {
  void (*func_glDeleteBuffersARB)( GLsizei, GLuint* ) = extension_funcs[136];
  TRACE("(%d, %p)\n", n, buffers );
  ENTER_GL();
  func_glDeleteBuffersARB( n, buffers );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteFencesAPPLE( GLsizei n, GLuint* fences ) {
  void (*func_glDeleteFencesAPPLE)( GLsizei, GLuint* ) = extension_funcs[137];
  TRACE("(%d, %p)\n", n, fences );
  ENTER_GL();
  func_glDeleteFencesAPPLE( n, fences );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteFencesNV( GLsizei n, GLuint* fences ) {
  void (*func_glDeleteFencesNV)( GLsizei, GLuint* ) = extension_funcs[138];
  TRACE("(%d, %p)\n", n, fences );
  ENTER_GL();
  func_glDeleteFencesNV( n, fences );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteFragmentShaderATI( GLuint id ) {
  void (*func_glDeleteFragmentShaderATI)( GLuint ) = extension_funcs[139];
  TRACE("(%d)\n", id );
  ENTER_GL();
  func_glDeleteFragmentShaderATI( id );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteFramebuffersEXT( GLsizei n, GLuint* framebuffers ) {
  void (*func_glDeleteFramebuffersEXT)( GLsizei, GLuint* ) = extension_funcs[140];
  TRACE("(%d, %p)\n", n, framebuffers );
  ENTER_GL();
  func_glDeleteFramebuffersEXT( n, framebuffers );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteObjectARB( unsigned int obj ) {
  void (*func_glDeleteObjectARB)( unsigned int ) = extension_funcs[141];
  TRACE("(%d)\n", obj );
  ENTER_GL();
  func_glDeleteObjectARB( obj );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteObjectBufferATI( GLuint buffer ) {
  void (*func_glDeleteObjectBufferATI)( GLuint ) = extension_funcs[142];
  TRACE("(%d)\n", buffer );
  ENTER_GL();
  func_glDeleteObjectBufferATI( buffer );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteOcclusionQueriesNV( GLsizei n, GLuint* ids ) {
  void (*func_glDeleteOcclusionQueriesNV)( GLsizei, GLuint* ) = extension_funcs[143];
  TRACE("(%d, %p)\n", n, ids );
  ENTER_GL();
  func_glDeleteOcclusionQueriesNV( n, ids );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteProgram( GLuint program ) {
  void (*func_glDeleteProgram)( GLuint ) = extension_funcs[144];
  TRACE("(%d)\n", program );
  ENTER_GL();
  func_glDeleteProgram( program );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteProgramsARB( GLsizei n, GLuint* programs ) {
  void (*func_glDeleteProgramsARB)( GLsizei, GLuint* ) = extension_funcs[145];
  TRACE("(%d, %p)\n", n, programs );
  ENTER_GL();
  func_glDeleteProgramsARB( n, programs );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteProgramsNV( GLsizei n, GLuint* programs ) {
  void (*func_glDeleteProgramsNV)( GLsizei, GLuint* ) = extension_funcs[146];
  TRACE("(%d, %p)\n", n, programs );
  ENTER_GL();
  func_glDeleteProgramsNV( n, programs );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteQueries( GLsizei n, GLuint* ids ) {
  void (*func_glDeleteQueries)( GLsizei, GLuint* ) = extension_funcs[147];
  TRACE("(%d, %p)\n", n, ids );
  ENTER_GL();
  func_glDeleteQueries( n, ids );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteQueriesARB( GLsizei n, GLuint* ids ) {
  void (*func_glDeleteQueriesARB)( GLsizei, GLuint* ) = extension_funcs[148];
  TRACE("(%d, %p)\n", n, ids );
  ENTER_GL();
  func_glDeleteQueriesARB( n, ids );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteRenderbuffersEXT( GLsizei n, GLuint* renderbuffers ) {
  void (*func_glDeleteRenderbuffersEXT)( GLsizei, GLuint* ) = extension_funcs[149];
  TRACE("(%d, %p)\n", n, renderbuffers );
  ENTER_GL();
  func_glDeleteRenderbuffersEXT( n, renderbuffers );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteShader( GLuint shader ) {
  void (*func_glDeleteShader)( GLuint ) = extension_funcs[150];
  TRACE("(%d)\n", shader );
  ENTER_GL();
  func_glDeleteShader( shader );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteTexturesEXT( GLsizei n, GLuint* textures ) {
  void (*func_glDeleteTexturesEXT)( GLsizei, GLuint* ) = extension_funcs[151];
  TRACE("(%d, %p)\n", n, textures );
  ENTER_GL();
  func_glDeleteTexturesEXT( n, textures );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteVertexArraysAPPLE( GLsizei n, GLuint* arrays ) {
  void (*func_glDeleteVertexArraysAPPLE)( GLsizei, GLuint* ) = extension_funcs[152];
  TRACE("(%d, %p)\n", n, arrays );
  ENTER_GL();
  func_glDeleteVertexArraysAPPLE( n, arrays );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteVertexShaderEXT( GLuint id ) {
  void (*func_glDeleteVertexShaderEXT)( GLuint ) = extension_funcs[153];
  TRACE("(%d)\n", id );
  ENTER_GL();
  func_glDeleteVertexShaderEXT( id );
  LEAVE_GL();
}

static void WINAPI wine_glDepthBoundsEXT( GLclampd zmin, GLclampd zmax ) {
  void (*func_glDepthBoundsEXT)( GLclampd, GLclampd ) = extension_funcs[154];
  TRACE("(%f, %f)\n", zmin, zmax );
  ENTER_GL();
  func_glDepthBoundsEXT( zmin, zmax );
  LEAVE_GL();
}

static void WINAPI wine_glDetachObjectARB( unsigned int containerObj, unsigned int attachedObj ) {
  void (*func_glDetachObjectARB)( unsigned int, unsigned int ) = extension_funcs[155];
  TRACE("(%d, %d)\n", containerObj, attachedObj );
  ENTER_GL();
  func_glDetachObjectARB( containerObj, attachedObj );
  LEAVE_GL();
}

static void WINAPI wine_glDetachShader( GLuint program, GLuint shader ) {
  void (*func_glDetachShader)( GLuint, GLuint ) = extension_funcs[156];
  TRACE("(%d, %d)\n", program, shader );
  ENTER_GL();
  func_glDetachShader( program, shader );
  LEAVE_GL();
}

static void WINAPI wine_glDetailTexFuncSGIS( GLenum target, GLsizei n, GLfloat* points ) {
  void (*func_glDetailTexFuncSGIS)( GLenum, GLsizei, GLfloat* ) = extension_funcs[157];
  TRACE("(%d, %d, %p)\n", target, n, points );
  ENTER_GL();
  func_glDetailTexFuncSGIS( target, n, points );
  LEAVE_GL();
}

static void WINAPI wine_glDisableVariantClientStateEXT( GLuint id ) {
  void (*func_glDisableVariantClientStateEXT)( GLuint ) = extension_funcs[158];
  TRACE("(%d)\n", id );
  ENTER_GL();
  func_glDisableVariantClientStateEXT( id );
  LEAVE_GL();
}

static void WINAPI wine_glDisableVertexAttribArray( GLuint index ) {
  void (*func_glDisableVertexAttribArray)( GLuint ) = extension_funcs[159];
  TRACE("(%d)\n", index );
  ENTER_GL();
  func_glDisableVertexAttribArray( index );
  LEAVE_GL();
}

static void WINAPI wine_glDisableVertexAttribArrayARB( GLuint index ) {
  void (*func_glDisableVertexAttribArrayARB)( GLuint ) = extension_funcs[160];
  TRACE("(%d)\n", index );
  ENTER_GL();
  func_glDisableVertexAttribArrayARB( index );
  LEAVE_GL();
}

static void WINAPI wine_glDrawArraysEXT( GLenum mode, GLint first, GLsizei count ) {
  void (*func_glDrawArraysEXT)( GLenum, GLint, GLsizei ) = extension_funcs[161];
  TRACE("(%d, %d, %d)\n", mode, first, count );
  ENTER_GL();
  func_glDrawArraysEXT( mode, first, count );
  LEAVE_GL();
}

static void WINAPI wine_glDrawBufferRegion( GLenum region, GLint x, GLint y, GLsizei width, GLsizei height, GLint xDest, GLint yDest ) {
  void (*func_glDrawBufferRegion)( GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLint ) = extension_funcs[162];
  TRACE("(%d, %d, %d, %d, %d, %d, %d)\n", region, x, y, width, height, xDest, yDest );
  ENTER_GL();
  func_glDrawBufferRegion( region, x, y, width, height, xDest, yDest );
  LEAVE_GL();
}

static void WINAPI wine_glDrawBuffers( GLsizei n, GLenum* bufs ) {
  void (*func_glDrawBuffers)( GLsizei, GLenum* ) = extension_funcs[163];
  TRACE("(%d, %p)\n", n, bufs );
  ENTER_GL();
  func_glDrawBuffers( n, bufs );
  LEAVE_GL();
}

static void WINAPI wine_glDrawBuffersARB( GLsizei n, GLenum* bufs ) {
  void (*func_glDrawBuffersARB)( GLsizei, GLenum* ) = extension_funcs[164];
  TRACE("(%d, %p)\n", n, bufs );
  ENTER_GL();
  func_glDrawBuffersARB( n, bufs );
  LEAVE_GL();
}

static void WINAPI wine_glDrawBuffersATI( GLsizei n, GLenum* bufs ) {
  void (*func_glDrawBuffersATI)( GLsizei, GLenum* ) = extension_funcs[165];
  TRACE("(%d, %p)\n", n, bufs );
  ENTER_GL();
  func_glDrawBuffersATI( n, bufs );
  LEAVE_GL();
}

static void WINAPI wine_glDrawElementArrayAPPLE( GLenum mode, GLint first, GLsizei count ) {
  void (*func_glDrawElementArrayAPPLE)( GLenum, GLint, GLsizei ) = extension_funcs[166];
  TRACE("(%d, %d, %d)\n", mode, first, count );
  ENTER_GL();
  func_glDrawElementArrayAPPLE( mode, first, count );
  LEAVE_GL();
}

static void WINAPI wine_glDrawElementArrayATI( GLenum mode, GLsizei count ) {
  void (*func_glDrawElementArrayATI)( GLenum, GLsizei ) = extension_funcs[167];
  TRACE("(%d, %d)\n", mode, count );
  ENTER_GL();
  func_glDrawElementArrayATI( mode, count );
  LEAVE_GL();
}

static void WINAPI wine_glDrawMeshArraysSUN( GLenum mode, GLint first, GLsizei count, GLsizei width ) {
  void (*func_glDrawMeshArraysSUN)( GLenum, GLint, GLsizei, GLsizei ) = extension_funcs[168];
  TRACE("(%d, %d, %d, %d)\n", mode, first, count, width );
  ENTER_GL();
  func_glDrawMeshArraysSUN( mode, first, count, width );
  LEAVE_GL();
}

static void WINAPI wine_glDrawRangeElementArrayAPPLE( GLenum mode, GLuint start, GLuint end, GLint first, GLsizei count ) {
  void (*func_glDrawRangeElementArrayAPPLE)( GLenum, GLuint, GLuint, GLint, GLsizei ) = extension_funcs[169];
  TRACE("(%d, %d, %d, %d, %d)\n", mode, start, end, first, count );
  ENTER_GL();
  func_glDrawRangeElementArrayAPPLE( mode, start, end, first, count );
  LEAVE_GL();
}

static void WINAPI wine_glDrawRangeElementArrayATI( GLenum mode, GLuint start, GLuint end, GLsizei count ) {
  void (*func_glDrawRangeElementArrayATI)( GLenum, GLuint, GLuint, GLsizei ) = extension_funcs[170];
  TRACE("(%d, %d, %d, %d)\n", mode, start, end, count );
  ENTER_GL();
  func_glDrawRangeElementArrayATI( mode, start, end, count );
  LEAVE_GL();
}

static void WINAPI wine_glDrawRangeElementsEXT( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, GLvoid* indices ) {
  void (*func_glDrawRangeElementsEXT)( GLenum, GLuint, GLuint, GLsizei, GLenum, GLvoid* ) = extension_funcs[171];
  TRACE("(%d, %d, %d, %d, %d, %p)\n", mode, start, end, count, type, indices );
  ENTER_GL();
  func_glDrawRangeElementsEXT( mode, start, end, count, type, indices );
  LEAVE_GL();
}

static void WINAPI wine_glEdgeFlagPointerEXT( GLsizei stride, GLsizei count, GLboolean* pointer ) {
  void (*func_glEdgeFlagPointerEXT)( GLsizei, GLsizei, GLboolean* ) = extension_funcs[172];
  TRACE("(%d, %d, %p)\n", stride, count, pointer );
  ENTER_GL();
  func_glEdgeFlagPointerEXT( stride, count, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glEdgeFlagPointerListIBM( GLint stride, GLboolean** pointer, GLint ptrstride ) {
  void (*func_glEdgeFlagPointerListIBM)( GLint, GLboolean**, GLint ) = extension_funcs[173];
  TRACE("(%d, %p, %d)\n", stride, pointer, ptrstride );
  ENTER_GL();
  func_glEdgeFlagPointerListIBM( stride, pointer, ptrstride );
  LEAVE_GL();
}

static void WINAPI wine_glElementPointerAPPLE( GLenum type, GLvoid* pointer ) {
  void (*func_glElementPointerAPPLE)( GLenum, GLvoid* ) = extension_funcs[174];
  TRACE("(%d, %p)\n", type, pointer );
  ENTER_GL();
  func_glElementPointerAPPLE( type, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glElementPointerATI( GLenum type, GLvoid* pointer ) {
  void (*func_glElementPointerATI)( GLenum, GLvoid* ) = extension_funcs[175];
  TRACE("(%d, %p)\n", type, pointer );
  ENTER_GL();
  func_glElementPointerATI( type, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glEnableVariantClientStateEXT( GLuint id ) {
  void (*func_glEnableVariantClientStateEXT)( GLuint ) = extension_funcs[176];
  TRACE("(%d)\n", id );
  ENTER_GL();
  func_glEnableVariantClientStateEXT( id );
  LEAVE_GL();
}

static void WINAPI wine_glEnableVertexAttribArray( GLuint index ) {
  void (*func_glEnableVertexAttribArray)( GLuint ) = extension_funcs[177];
  TRACE("(%d)\n", index );
  ENTER_GL();
  func_glEnableVertexAttribArray( index );
  LEAVE_GL();
}

static void WINAPI wine_glEnableVertexAttribArrayARB( GLuint index ) {
  void (*func_glEnableVertexAttribArrayARB)( GLuint ) = extension_funcs[178];
  TRACE("(%d)\n", index );
  ENTER_GL();
  func_glEnableVertexAttribArrayARB( index );
  LEAVE_GL();
}

static void WINAPI wine_glEndFragmentShaderATI( void ) {
  void (*func_glEndFragmentShaderATI)( void ) = extension_funcs[179];
  TRACE("()\n");
  ENTER_GL();
  func_glEndFragmentShaderATI( );
  LEAVE_GL();
}

static void WINAPI wine_glEndOcclusionQueryNV( void ) {
  void (*func_glEndOcclusionQueryNV)( void ) = extension_funcs[180];
  TRACE("()\n");
  ENTER_GL();
  func_glEndOcclusionQueryNV( );
  LEAVE_GL();
}

static void WINAPI wine_glEndQuery( GLenum target ) {
  void (*func_glEndQuery)( GLenum ) = extension_funcs[181];
  TRACE("(%d)\n", target );
  ENTER_GL();
  func_glEndQuery( target );
  LEAVE_GL();
}

static void WINAPI wine_glEndQueryARB( GLenum target ) {
  void (*func_glEndQueryARB)( GLenum ) = extension_funcs[182];
  TRACE("(%d)\n", target );
  ENTER_GL();
  func_glEndQueryARB( target );
  LEAVE_GL();
}

static void WINAPI wine_glEndVertexShaderEXT( void ) {
  void (*func_glEndVertexShaderEXT)( void ) = extension_funcs[183];
  TRACE("()\n");
  ENTER_GL();
  func_glEndVertexShaderEXT( );
  LEAVE_GL();
}

static void WINAPI wine_glEvalMapsNV( GLenum target, GLenum mode ) {
  void (*func_glEvalMapsNV)( GLenum, GLenum ) = extension_funcs[184];
  TRACE("(%d, %d)\n", target, mode );
  ENTER_GL();
  func_glEvalMapsNV( target, mode );
  LEAVE_GL();
}

static void WINAPI wine_glExecuteProgramNV( GLenum target, GLuint id, GLfloat* params ) {
  void (*func_glExecuteProgramNV)( GLenum, GLuint, GLfloat* ) = extension_funcs[185];
  TRACE("(%d, %d, %p)\n", target, id, params );
  ENTER_GL();
  func_glExecuteProgramNV( target, id, params );
  LEAVE_GL();
}

static void WINAPI wine_glExtractComponentEXT( GLuint res, GLuint src, GLuint num ) {
  void (*func_glExtractComponentEXT)( GLuint, GLuint, GLuint ) = extension_funcs[186];
  TRACE("(%d, %d, %d)\n", res, src, num );
  ENTER_GL();
  func_glExtractComponentEXT( res, src, num );
  LEAVE_GL();
}

static void WINAPI wine_glFinalCombinerInputNV( GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage ) {
  void (*func_glFinalCombinerInputNV)( GLenum, GLenum, GLenum, GLenum ) = extension_funcs[187];
  TRACE("(%d, %d, %d, %d)\n", variable, input, mapping, componentUsage );
  ENTER_GL();
  func_glFinalCombinerInputNV( variable, input, mapping, componentUsage );
  LEAVE_GL();
}

static GLint WINAPI wine_glFinishAsyncSGIX( GLuint* markerp ) {
  GLint ret_value;
  GLint (*func_glFinishAsyncSGIX)( GLuint* ) = extension_funcs[188];
  TRACE("(%p)\n", markerp );
  ENTER_GL();
  ret_value = func_glFinishAsyncSGIX( markerp );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glFinishFenceAPPLE( GLuint fence ) {
  void (*func_glFinishFenceAPPLE)( GLuint ) = extension_funcs[189];
  TRACE("(%d)\n", fence );
  ENTER_GL();
  func_glFinishFenceAPPLE( fence );
  LEAVE_GL();
}

static void WINAPI wine_glFinishFenceNV( GLuint fence ) {
  void (*func_glFinishFenceNV)( GLuint ) = extension_funcs[190];
  TRACE("(%d)\n", fence );
  ENTER_GL();
  func_glFinishFenceNV( fence );
  LEAVE_GL();
}

static void WINAPI wine_glFinishObjectAPPLE( GLenum object, GLint name ) {
  void (*func_glFinishObjectAPPLE)( GLenum, GLint ) = extension_funcs[191];
  TRACE("(%d, %d)\n", object, name );
  ENTER_GL();
  func_glFinishObjectAPPLE( object, name );
  LEAVE_GL();
}

static void WINAPI wine_glFinishTextureSUNX( void ) {
  void (*func_glFinishTextureSUNX)( void ) = extension_funcs[192];
  TRACE("()\n");
  ENTER_GL();
  func_glFinishTextureSUNX( );
  LEAVE_GL();
}

static void WINAPI wine_glFlushPixelDataRangeNV( GLenum target ) {
  void (*func_glFlushPixelDataRangeNV)( GLenum ) = extension_funcs[193];
  TRACE("(%d)\n", target );
  ENTER_GL();
  func_glFlushPixelDataRangeNV( target );
  LEAVE_GL();
}

static void WINAPI wine_glFlushRasterSGIX( void ) {
  void (*func_glFlushRasterSGIX)( void ) = extension_funcs[194];
  TRACE("()\n");
  ENTER_GL();
  func_glFlushRasterSGIX( );
  LEAVE_GL();
}

static void WINAPI wine_glFlushVertexArrayRangeAPPLE( GLsizei length, GLvoid* pointer ) {
  void (*func_glFlushVertexArrayRangeAPPLE)( GLsizei, GLvoid* ) = extension_funcs[195];
  TRACE("(%d, %p)\n", length, pointer );
  ENTER_GL();
  func_glFlushVertexArrayRangeAPPLE( length, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glFlushVertexArrayRangeNV( void ) {
  void (*func_glFlushVertexArrayRangeNV)( void ) = extension_funcs[196];
  TRACE("()\n");
  ENTER_GL();
  func_glFlushVertexArrayRangeNV( );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoordPointer( GLenum type, GLsizei stride, GLvoid* pointer ) {
  void (*func_glFogCoordPointer)( GLenum, GLsizei, GLvoid* ) = extension_funcs[197];
  TRACE("(%d, %d, %p)\n", type, stride, pointer );
  ENTER_GL();
  func_glFogCoordPointer( type, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoordPointerEXT( GLenum type, GLsizei stride, GLvoid* pointer ) {
  void (*func_glFogCoordPointerEXT)( GLenum, GLsizei, GLvoid* ) = extension_funcs[198];
  TRACE("(%d, %d, %p)\n", type, stride, pointer );
  ENTER_GL();
  func_glFogCoordPointerEXT( type, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoordPointerListIBM( GLenum type, GLint stride, GLvoid** pointer, GLint ptrstride ) {
  void (*func_glFogCoordPointerListIBM)( GLenum, GLint, GLvoid**, GLint ) = extension_funcs[199];
  TRACE("(%d, %d, %p, %d)\n", type, stride, pointer, ptrstride );
  ENTER_GL();
  func_glFogCoordPointerListIBM( type, stride, pointer, ptrstride );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoordd( GLdouble coord ) {
  void (*func_glFogCoordd)( GLdouble ) = extension_funcs[200];
  TRACE("(%f)\n", coord );
  ENTER_GL();
  func_glFogCoordd( coord );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoorddEXT( GLdouble coord ) {
  void (*func_glFogCoorddEXT)( GLdouble ) = extension_funcs[201];
  TRACE("(%f)\n", coord );
  ENTER_GL();
  func_glFogCoorddEXT( coord );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoorddv( GLdouble* coord ) {
  void (*func_glFogCoorddv)( GLdouble* ) = extension_funcs[202];
  TRACE("(%p)\n", coord );
  ENTER_GL();
  func_glFogCoorddv( coord );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoorddvEXT( GLdouble* coord ) {
  void (*func_glFogCoorddvEXT)( GLdouble* ) = extension_funcs[203];
  TRACE("(%p)\n", coord );
  ENTER_GL();
  func_glFogCoorddvEXT( coord );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoordf( GLfloat coord ) {
  void (*func_glFogCoordf)( GLfloat ) = extension_funcs[204];
  TRACE("(%f)\n", coord );
  ENTER_GL();
  func_glFogCoordf( coord );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoordfEXT( GLfloat coord ) {
  void (*func_glFogCoordfEXT)( GLfloat ) = extension_funcs[205];
  TRACE("(%f)\n", coord );
  ENTER_GL();
  func_glFogCoordfEXT( coord );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoordfv( GLfloat* coord ) {
  void (*func_glFogCoordfv)( GLfloat* ) = extension_funcs[206];
  TRACE("(%p)\n", coord );
  ENTER_GL();
  func_glFogCoordfv( coord );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoordfvEXT( GLfloat* coord ) {
  void (*func_glFogCoordfvEXT)( GLfloat* ) = extension_funcs[207];
  TRACE("(%p)\n", coord );
  ENTER_GL();
  func_glFogCoordfvEXT( coord );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoordhNV( unsigned short fog ) {
  void (*func_glFogCoordhNV)( unsigned short ) = extension_funcs[208];
  TRACE("(%d)\n", fog );
  ENTER_GL();
  func_glFogCoordhNV( fog );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoordhvNV( unsigned short* fog ) {
  void (*func_glFogCoordhvNV)( unsigned short* ) = extension_funcs[209];
  TRACE("(%p)\n", fog );
  ENTER_GL();
  func_glFogCoordhvNV( fog );
  LEAVE_GL();
}

static void WINAPI wine_glFogFuncSGIS( GLsizei n, GLfloat* points ) {
  void (*func_glFogFuncSGIS)( GLsizei, GLfloat* ) = extension_funcs[210];
  TRACE("(%d, %p)\n", n, points );
  ENTER_GL();
  func_glFogFuncSGIS( n, points );
  LEAVE_GL();
}

static void WINAPI wine_glFragmentColorMaterialSGIX( GLenum face, GLenum mode ) {
  void (*func_glFragmentColorMaterialSGIX)( GLenum, GLenum ) = extension_funcs[211];
  TRACE("(%d, %d)\n", face, mode );
  ENTER_GL();
  func_glFragmentColorMaterialSGIX( face, mode );
  LEAVE_GL();
}

static void WINAPI wine_glFragmentLightModelfSGIX( GLenum pname, GLfloat param ) {
  void (*func_glFragmentLightModelfSGIX)( GLenum, GLfloat ) = extension_funcs[212];
  TRACE("(%d, %f)\n", pname, param );
  ENTER_GL();
  func_glFragmentLightModelfSGIX( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glFragmentLightModelfvSGIX( GLenum pname, GLfloat* params ) {
  void (*func_glFragmentLightModelfvSGIX)( GLenum, GLfloat* ) = extension_funcs[213];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glFragmentLightModelfvSGIX( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glFragmentLightModeliSGIX( GLenum pname, GLint param ) {
  void (*func_glFragmentLightModeliSGIX)( GLenum, GLint ) = extension_funcs[214];
  TRACE("(%d, %d)\n", pname, param );
  ENTER_GL();
  func_glFragmentLightModeliSGIX( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glFragmentLightModelivSGIX( GLenum pname, GLint* params ) {
  void (*func_glFragmentLightModelivSGIX)( GLenum, GLint* ) = extension_funcs[215];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glFragmentLightModelivSGIX( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glFragmentLightfSGIX( GLenum light, GLenum pname, GLfloat param ) {
  void (*func_glFragmentLightfSGIX)( GLenum, GLenum, GLfloat ) = extension_funcs[216];
  TRACE("(%d, %d, %f)\n", light, pname, param );
  ENTER_GL();
  func_glFragmentLightfSGIX( light, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glFragmentLightfvSGIX( GLenum light, GLenum pname, GLfloat* params ) {
  void (*func_glFragmentLightfvSGIX)( GLenum, GLenum, GLfloat* ) = extension_funcs[217];
  TRACE("(%d, %d, %p)\n", light, pname, params );
  ENTER_GL();
  func_glFragmentLightfvSGIX( light, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glFragmentLightiSGIX( GLenum light, GLenum pname, GLint param ) {
  void (*func_glFragmentLightiSGIX)( GLenum, GLenum, GLint ) = extension_funcs[218];
  TRACE("(%d, %d, %d)\n", light, pname, param );
  ENTER_GL();
  func_glFragmentLightiSGIX( light, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glFragmentLightivSGIX( GLenum light, GLenum pname, GLint* params ) {
  void (*func_glFragmentLightivSGIX)( GLenum, GLenum, GLint* ) = extension_funcs[219];
  TRACE("(%d, %d, %p)\n", light, pname, params );
  ENTER_GL();
  func_glFragmentLightivSGIX( light, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glFragmentMaterialfSGIX( GLenum face, GLenum pname, GLfloat param ) {
  void (*func_glFragmentMaterialfSGIX)( GLenum, GLenum, GLfloat ) = extension_funcs[220];
  TRACE("(%d, %d, %f)\n", face, pname, param );
  ENTER_GL();
  func_glFragmentMaterialfSGIX( face, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glFragmentMaterialfvSGIX( GLenum face, GLenum pname, GLfloat* params ) {
  void (*func_glFragmentMaterialfvSGIX)( GLenum, GLenum, GLfloat* ) = extension_funcs[221];
  TRACE("(%d, %d, %p)\n", face, pname, params );
  ENTER_GL();
  func_glFragmentMaterialfvSGIX( face, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glFragmentMaterialiSGIX( GLenum face, GLenum pname, GLint param ) {
  void (*func_glFragmentMaterialiSGIX)( GLenum, GLenum, GLint ) = extension_funcs[222];
  TRACE("(%d, %d, %d)\n", face, pname, param );
  ENTER_GL();
  func_glFragmentMaterialiSGIX( face, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glFragmentMaterialivSGIX( GLenum face, GLenum pname, GLint* params ) {
  void (*func_glFragmentMaterialivSGIX)( GLenum, GLenum, GLint* ) = extension_funcs[223];
  TRACE("(%d, %d, %p)\n", face, pname, params );
  ENTER_GL();
  func_glFragmentMaterialivSGIX( face, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glFrameZoomSGIX( GLint factor ) {
  void (*func_glFrameZoomSGIX)( GLint ) = extension_funcs[224];
  TRACE("(%d)\n", factor );
  ENTER_GL();
  func_glFrameZoomSGIX( factor );
  LEAVE_GL();
}

static void WINAPI wine_glFramebufferRenderbufferEXT( GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer ) {
  void (*func_glFramebufferRenderbufferEXT)( GLenum, GLenum, GLenum, GLuint ) = extension_funcs[225];
  TRACE("(%d, %d, %d, %d)\n", target, attachment, renderbuffertarget, renderbuffer );
  ENTER_GL();
  func_glFramebufferRenderbufferEXT( target, attachment, renderbuffertarget, renderbuffer );
  LEAVE_GL();
}

static void WINAPI wine_glFramebufferTexture1DEXT( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level ) {
  void (*func_glFramebufferTexture1DEXT)( GLenum, GLenum, GLenum, GLuint, GLint ) = extension_funcs[226];
  TRACE("(%d, %d, %d, %d, %d)\n", target, attachment, textarget, texture, level );
  ENTER_GL();
  func_glFramebufferTexture1DEXT( target, attachment, textarget, texture, level );
  LEAVE_GL();
}

static void WINAPI wine_glFramebufferTexture2DEXT( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level ) {
  void (*func_glFramebufferTexture2DEXT)( GLenum, GLenum, GLenum, GLuint, GLint ) = extension_funcs[227];
  TRACE("(%d, %d, %d, %d, %d)\n", target, attachment, textarget, texture, level );
  ENTER_GL();
  func_glFramebufferTexture2DEXT( target, attachment, textarget, texture, level );
  LEAVE_GL();
}

static void WINAPI wine_glFramebufferTexture3DEXT( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset ) {
  void (*func_glFramebufferTexture3DEXT)( GLenum, GLenum, GLenum, GLuint, GLint, GLint ) = extension_funcs[228];
  TRACE("(%d, %d, %d, %d, %d, %d)\n", target, attachment, textarget, texture, level, zoffset );
  ENTER_GL();
  func_glFramebufferTexture3DEXT( target, attachment, textarget, texture, level, zoffset );
  LEAVE_GL();
}

static void WINAPI wine_glFreeObjectBufferATI( GLuint buffer ) {
  void (*func_glFreeObjectBufferATI)( GLuint ) = extension_funcs[229];
  TRACE("(%d)\n", buffer );
  ENTER_GL();
  func_glFreeObjectBufferATI( buffer );
  LEAVE_GL();
}

static GLuint WINAPI wine_glGenAsyncMarkersSGIX( GLsizei range ) {
  GLuint ret_value;
  GLuint (*func_glGenAsyncMarkersSGIX)( GLsizei ) = extension_funcs[230];
  TRACE("(%d)\n", range );
  ENTER_GL();
  ret_value = func_glGenAsyncMarkersSGIX( range );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glGenBuffers( GLsizei n, GLuint* buffers ) {
  void (*func_glGenBuffers)( GLsizei, GLuint* ) = extension_funcs[231];
  TRACE("(%d, %p)\n", n, buffers );
  ENTER_GL();
  func_glGenBuffers( n, buffers );
  LEAVE_GL();
}

static void WINAPI wine_glGenBuffersARB( GLsizei n, GLuint* buffers ) {
  void (*func_glGenBuffersARB)( GLsizei, GLuint* ) = extension_funcs[232];
  TRACE("(%d, %p)\n", n, buffers );
  ENTER_GL();
  func_glGenBuffersARB( n, buffers );
  LEAVE_GL();
}

static void WINAPI wine_glGenFencesAPPLE( GLsizei n, GLuint* fences ) {
  void (*func_glGenFencesAPPLE)( GLsizei, GLuint* ) = extension_funcs[233];
  TRACE("(%d, %p)\n", n, fences );
  ENTER_GL();
  func_glGenFencesAPPLE( n, fences );
  LEAVE_GL();
}

static void WINAPI wine_glGenFencesNV( GLsizei n, GLuint* fences ) {
  void (*func_glGenFencesNV)( GLsizei, GLuint* ) = extension_funcs[234];
  TRACE("(%d, %p)\n", n, fences );
  ENTER_GL();
  func_glGenFencesNV( n, fences );
  LEAVE_GL();
}

static GLuint WINAPI wine_glGenFragmentShadersATI( GLuint range ) {
  GLuint ret_value;
  GLuint (*func_glGenFragmentShadersATI)( GLuint ) = extension_funcs[235];
  TRACE("(%d)\n", range );
  ENTER_GL();
  ret_value = func_glGenFragmentShadersATI( range );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glGenFramebuffersEXT( GLsizei n, GLuint* framebuffers ) {
  void (*func_glGenFramebuffersEXT)( GLsizei, GLuint* ) = extension_funcs[236];
  TRACE("(%d, %p)\n", n, framebuffers );
  ENTER_GL();
  func_glGenFramebuffersEXT( n, framebuffers );
  LEAVE_GL();
}

static void WINAPI wine_glGenOcclusionQueriesNV( GLsizei n, GLuint* ids ) {
  void (*func_glGenOcclusionQueriesNV)( GLsizei, GLuint* ) = extension_funcs[237];
  TRACE("(%d, %p)\n", n, ids );
  ENTER_GL();
  func_glGenOcclusionQueriesNV( n, ids );
  LEAVE_GL();
}

static void WINAPI wine_glGenProgramsARB( GLsizei n, GLuint* programs ) {
  void (*func_glGenProgramsARB)( GLsizei, GLuint* ) = extension_funcs[238];
  TRACE("(%d, %p)\n", n, programs );
  ENTER_GL();
  func_glGenProgramsARB( n, programs );
  LEAVE_GL();
}

static void WINAPI wine_glGenProgramsNV( GLsizei n, GLuint* programs ) {
  void (*func_glGenProgramsNV)( GLsizei, GLuint* ) = extension_funcs[239];
  TRACE("(%d, %p)\n", n, programs );
  ENTER_GL();
  func_glGenProgramsNV( n, programs );
  LEAVE_GL();
}

static void WINAPI wine_glGenQueries( GLsizei n, GLuint* ids ) {
  void (*func_glGenQueries)( GLsizei, GLuint* ) = extension_funcs[240];
  TRACE("(%d, %p)\n", n, ids );
  ENTER_GL();
  func_glGenQueries( n, ids );
  LEAVE_GL();
}

static void WINAPI wine_glGenQueriesARB( GLsizei n, GLuint* ids ) {
  void (*func_glGenQueriesARB)( GLsizei, GLuint* ) = extension_funcs[241];
  TRACE("(%d, %p)\n", n, ids );
  ENTER_GL();
  func_glGenQueriesARB( n, ids );
  LEAVE_GL();
}

static void WINAPI wine_glGenRenderbuffersEXT( GLsizei n, GLuint* renderbuffers ) {
  void (*func_glGenRenderbuffersEXT)( GLsizei, GLuint* ) = extension_funcs[242];
  TRACE("(%d, %p)\n", n, renderbuffers );
  ENTER_GL();
  func_glGenRenderbuffersEXT( n, renderbuffers );
  LEAVE_GL();
}

static GLuint WINAPI wine_glGenSymbolsEXT( GLenum datatype, GLenum storagetype, GLenum range, GLuint components ) {
  GLuint ret_value;
  GLuint (*func_glGenSymbolsEXT)( GLenum, GLenum, GLenum, GLuint ) = extension_funcs[243];
  TRACE("(%d, %d, %d, %d)\n", datatype, storagetype, range, components );
  ENTER_GL();
  ret_value = func_glGenSymbolsEXT( datatype, storagetype, range, components );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glGenTexturesEXT( GLsizei n, GLuint* textures ) {
  void (*func_glGenTexturesEXT)( GLsizei, GLuint* ) = extension_funcs[244];
  TRACE("(%d, %p)\n", n, textures );
  ENTER_GL();
  func_glGenTexturesEXT( n, textures );
  LEAVE_GL();
}

static void WINAPI wine_glGenVertexArraysAPPLE( GLsizei n, GLuint* arrays ) {
  void (*func_glGenVertexArraysAPPLE)( GLsizei, GLuint* ) = extension_funcs[245];
  TRACE("(%d, %p)\n", n, arrays );
  ENTER_GL();
  func_glGenVertexArraysAPPLE( n, arrays );
  LEAVE_GL();
}

static GLuint WINAPI wine_glGenVertexShadersEXT( GLuint range ) {
  GLuint ret_value;
  GLuint (*func_glGenVertexShadersEXT)( GLuint ) = extension_funcs[246];
  TRACE("(%d)\n", range );
  ENTER_GL();
  ret_value = func_glGenVertexShadersEXT( range );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glGenerateMipmapEXT( GLenum target ) {
  void (*func_glGenerateMipmapEXT)( GLenum ) = extension_funcs[247];
  TRACE("(%d)\n", target );
  ENTER_GL();
  func_glGenerateMipmapEXT( target );
  LEAVE_GL();
}

static void WINAPI wine_glGetActiveAttrib( GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, char* name ) {
  void (*func_glGetActiveAttrib)( GLuint, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, char* ) = extension_funcs[248];
  TRACE("(%d, %d, %d, %p, %p, %p, %p)\n", program, index, bufSize, length, size, type, name );
  ENTER_GL();
  func_glGetActiveAttrib( program, index, bufSize, length, size, type, name );
  LEAVE_GL();
}

static void WINAPI wine_glGetActiveAttribARB( unsigned int programObj, GLuint index, GLsizei maxLength, GLsizei* length, GLint* size, GLenum* type, char* name ) {
  void (*func_glGetActiveAttribARB)( unsigned int, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, char* ) = extension_funcs[249];
  TRACE("(%d, %d, %d, %p, %p, %p, %p)\n", programObj, index, maxLength, length, size, type, name );
  ENTER_GL();
  func_glGetActiveAttribARB( programObj, index, maxLength, length, size, type, name );
  LEAVE_GL();
}

static void WINAPI wine_glGetActiveUniform( GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, char* name ) {
  void (*func_glGetActiveUniform)( GLuint, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, char* ) = extension_funcs[250];
  TRACE("(%d, %d, %d, %p, %p, %p, %p)\n", program, index, bufSize, length, size, type, name );
  ENTER_GL();
  func_glGetActiveUniform( program, index, bufSize, length, size, type, name );
  LEAVE_GL();
}

static void WINAPI wine_glGetActiveUniformARB( unsigned int programObj, GLuint index, GLsizei maxLength, GLsizei* length, GLint* size, GLenum* type, char* name ) {
  void (*func_glGetActiveUniformARB)( unsigned int, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, char* ) = extension_funcs[251];
  TRACE("(%d, %d, %d, %p, %p, %p, %p)\n", programObj, index, maxLength, length, size, type, name );
  ENTER_GL();
  func_glGetActiveUniformARB( programObj, index, maxLength, length, size, type, name );
  LEAVE_GL();
}

static void WINAPI wine_glGetArrayObjectfvATI( GLenum array, GLenum pname, GLfloat* params ) {
  void (*func_glGetArrayObjectfvATI)( GLenum, GLenum, GLfloat* ) = extension_funcs[252];
  TRACE("(%d, %d, %p)\n", array, pname, params );
  ENTER_GL();
  func_glGetArrayObjectfvATI( array, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetArrayObjectivATI( GLenum array, GLenum pname, GLint* params ) {
  void (*func_glGetArrayObjectivATI)( GLenum, GLenum, GLint* ) = extension_funcs[253];
  TRACE("(%d, %d, %p)\n", array, pname, params );
  ENTER_GL();
  func_glGetArrayObjectivATI( array, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetAttachedObjectsARB( unsigned int containerObj, GLsizei maxCount, GLsizei* count, unsigned int* obj ) {
  void (*func_glGetAttachedObjectsARB)( unsigned int, GLsizei, GLsizei*, unsigned int* ) = extension_funcs[254];
  TRACE("(%d, %d, %p, %p)\n", containerObj, maxCount, count, obj );
  ENTER_GL();
  func_glGetAttachedObjectsARB( containerObj, maxCount, count, obj );
  LEAVE_GL();
}

static void WINAPI wine_glGetAttachedShaders( GLuint program, GLsizei maxCount, GLsizei* count, GLuint* obj ) {
  void (*func_glGetAttachedShaders)( GLuint, GLsizei, GLsizei*, GLuint* ) = extension_funcs[255];
  TRACE("(%d, %d, %p, %p)\n", program, maxCount, count, obj );
  ENTER_GL();
  func_glGetAttachedShaders( program, maxCount, count, obj );
  LEAVE_GL();
}

static GLint WINAPI wine_glGetAttribLocation( GLuint program, char* name ) {
  GLint ret_value;
  GLint (*func_glGetAttribLocation)( GLuint, char* ) = extension_funcs[256];
  TRACE("(%d, %p)\n", program, name );
  ENTER_GL();
  ret_value = func_glGetAttribLocation( program, name );
  LEAVE_GL();
  return ret_value;
}

static GLint WINAPI wine_glGetAttribLocationARB( unsigned int programObj, char* name ) {
  GLint ret_value;
  GLint (*func_glGetAttribLocationARB)( unsigned int, char* ) = extension_funcs[257];
  TRACE("(%d, %p)\n", programObj, name );
  ENTER_GL();
  ret_value = func_glGetAttribLocationARB( programObj, name );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glGetBufferParameteriv( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glGetBufferParameteriv)( GLenum, GLenum, GLint* ) = extension_funcs[258];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetBufferParameteriv( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetBufferParameterivARB( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glGetBufferParameterivARB)( GLenum, GLenum, GLint* ) = extension_funcs[259];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetBufferParameterivARB( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetBufferPointerv( GLenum target, GLenum pname, GLvoid** params ) {
  void (*func_glGetBufferPointerv)( GLenum, GLenum, GLvoid** ) = extension_funcs[260];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetBufferPointerv( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetBufferPointervARB( GLenum target, GLenum pname, GLvoid** params ) {
  void (*func_glGetBufferPointervARB)( GLenum, GLenum, GLvoid** ) = extension_funcs[261];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetBufferPointervARB( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetBufferSubData( GLenum target, ptrdiff_t offset, ptrdiff_t size, GLvoid* data ) {
  void (*func_glGetBufferSubData)( GLenum, ptrdiff_t, ptrdiff_t, GLvoid* ) = extension_funcs[262];
  TRACE("(%d, %d, %d, %p)\n", target, offset, size, data );
  ENTER_GL();
  func_glGetBufferSubData( target, offset, size, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetBufferSubDataARB( GLenum target, ptrdiff_t offset, ptrdiff_t size, GLvoid* data ) {
  void (*func_glGetBufferSubDataARB)( GLenum, ptrdiff_t, ptrdiff_t, GLvoid* ) = extension_funcs[263];
  TRACE("(%d, %d, %d, %p)\n", target, offset, size, data );
  ENTER_GL();
  func_glGetBufferSubDataARB( target, offset, size, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetColorTableEXT( GLenum target, GLenum format, GLenum type, GLvoid* data ) {
  void (*func_glGetColorTableEXT)( GLenum, GLenum, GLenum, GLvoid* ) = extension_funcs[264];
  TRACE("(%d, %d, %d, %p)\n", target, format, type, data );
  ENTER_GL();
  func_glGetColorTableEXT( target, format, type, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetColorTableParameterfvEXT( GLenum target, GLenum pname, GLfloat* params ) {
  void (*func_glGetColorTableParameterfvEXT)( GLenum, GLenum, GLfloat* ) = extension_funcs[265];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetColorTableParameterfvEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetColorTableParameterfvSGI( GLenum target, GLenum pname, GLfloat* params ) {
  void (*func_glGetColorTableParameterfvSGI)( GLenum, GLenum, GLfloat* ) = extension_funcs[266];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetColorTableParameterfvSGI( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetColorTableParameterivEXT( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glGetColorTableParameterivEXT)( GLenum, GLenum, GLint* ) = extension_funcs[267];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetColorTableParameterivEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetColorTableParameterivSGI( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glGetColorTableParameterivSGI)( GLenum, GLenum, GLint* ) = extension_funcs[268];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetColorTableParameterivSGI( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetColorTableSGI( GLenum target, GLenum format, GLenum type, GLvoid* table ) {
  void (*func_glGetColorTableSGI)( GLenum, GLenum, GLenum, GLvoid* ) = extension_funcs[269];
  TRACE("(%d, %d, %d, %p)\n", target, format, type, table );
  ENTER_GL();
  func_glGetColorTableSGI( target, format, type, table );
  LEAVE_GL();
}

static void WINAPI wine_glGetCombinerInputParameterfvNV( GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLfloat* params ) {
  void (*func_glGetCombinerInputParameterfvNV)( GLenum, GLenum, GLenum, GLenum, GLfloat* ) = extension_funcs[270];
  TRACE("(%d, %d, %d, %d, %p)\n", stage, portion, variable, pname, params );
  ENTER_GL();
  func_glGetCombinerInputParameterfvNV( stage, portion, variable, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetCombinerInputParameterivNV( GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLint* params ) {
  void (*func_glGetCombinerInputParameterivNV)( GLenum, GLenum, GLenum, GLenum, GLint* ) = extension_funcs[271];
  TRACE("(%d, %d, %d, %d, %p)\n", stage, portion, variable, pname, params );
  ENTER_GL();
  func_glGetCombinerInputParameterivNV( stage, portion, variable, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetCombinerOutputParameterfvNV( GLenum stage, GLenum portion, GLenum pname, GLfloat* params ) {
  void (*func_glGetCombinerOutputParameterfvNV)( GLenum, GLenum, GLenum, GLfloat* ) = extension_funcs[272];
  TRACE("(%d, %d, %d, %p)\n", stage, portion, pname, params );
  ENTER_GL();
  func_glGetCombinerOutputParameterfvNV( stage, portion, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetCombinerOutputParameterivNV( GLenum stage, GLenum portion, GLenum pname, GLint* params ) {
  void (*func_glGetCombinerOutputParameterivNV)( GLenum, GLenum, GLenum, GLint* ) = extension_funcs[273];
  TRACE("(%d, %d, %d, %p)\n", stage, portion, pname, params );
  ENTER_GL();
  func_glGetCombinerOutputParameterivNV( stage, portion, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetCombinerStageParameterfvNV( GLenum stage, GLenum pname, GLfloat* params ) {
  void (*func_glGetCombinerStageParameterfvNV)( GLenum, GLenum, GLfloat* ) = extension_funcs[274];
  TRACE("(%d, %d, %p)\n", stage, pname, params );
  ENTER_GL();
  func_glGetCombinerStageParameterfvNV( stage, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetCompressedTexImage( GLenum target, GLint level, GLvoid* img ) {
  void (*func_glGetCompressedTexImage)( GLenum, GLint, GLvoid* ) = extension_funcs[275];
  TRACE("(%d, %d, %p)\n", target, level, img );
  ENTER_GL();
  func_glGetCompressedTexImage( target, level, img );
  LEAVE_GL();
}

static void WINAPI wine_glGetCompressedTexImageARB( GLenum target, GLint level, GLvoid* img ) {
  void (*func_glGetCompressedTexImageARB)( GLenum, GLint, GLvoid* ) = extension_funcs[276];
  TRACE("(%d, %d, %p)\n", target, level, img );
  ENTER_GL();
  func_glGetCompressedTexImageARB( target, level, img );
  LEAVE_GL();
}

static void WINAPI wine_glGetConvolutionFilterEXT( GLenum target, GLenum format, GLenum type, GLvoid* image ) {
  void (*func_glGetConvolutionFilterEXT)( GLenum, GLenum, GLenum, GLvoid* ) = extension_funcs[277];
  TRACE("(%d, %d, %d, %p)\n", target, format, type, image );
  ENTER_GL();
  func_glGetConvolutionFilterEXT( target, format, type, image );
  LEAVE_GL();
}

static void WINAPI wine_glGetConvolutionParameterfvEXT( GLenum target, GLenum pname, GLfloat* params ) {
  void (*func_glGetConvolutionParameterfvEXT)( GLenum, GLenum, GLfloat* ) = extension_funcs[278];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetConvolutionParameterfvEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetConvolutionParameterivEXT( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glGetConvolutionParameterivEXT)( GLenum, GLenum, GLint* ) = extension_funcs[279];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetConvolutionParameterivEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetDetailTexFuncSGIS( GLenum target, GLfloat* points ) {
  void (*func_glGetDetailTexFuncSGIS)( GLenum, GLfloat* ) = extension_funcs[280];
  TRACE("(%d, %p)\n", target, points );
  ENTER_GL();
  func_glGetDetailTexFuncSGIS( target, points );
  LEAVE_GL();
}

static void WINAPI wine_glGetFenceivNV( GLuint fence, GLenum pname, GLint* params ) {
  void (*func_glGetFenceivNV)( GLuint, GLenum, GLint* ) = extension_funcs[281];
  TRACE("(%d, %d, %p)\n", fence, pname, params );
  ENTER_GL();
  func_glGetFenceivNV( fence, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetFinalCombinerInputParameterfvNV( GLenum variable, GLenum pname, GLfloat* params ) {
  void (*func_glGetFinalCombinerInputParameterfvNV)( GLenum, GLenum, GLfloat* ) = extension_funcs[282];
  TRACE("(%d, %d, %p)\n", variable, pname, params );
  ENTER_GL();
  func_glGetFinalCombinerInputParameterfvNV( variable, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetFinalCombinerInputParameterivNV( GLenum variable, GLenum pname, GLint* params ) {
  void (*func_glGetFinalCombinerInputParameterivNV)( GLenum, GLenum, GLint* ) = extension_funcs[283];
  TRACE("(%d, %d, %p)\n", variable, pname, params );
  ENTER_GL();
  func_glGetFinalCombinerInputParameterivNV( variable, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetFogFuncSGIS( GLfloat* points ) {
  void (*func_glGetFogFuncSGIS)( GLfloat* ) = extension_funcs[284];
  TRACE("(%p)\n", points );
  ENTER_GL();
  func_glGetFogFuncSGIS( points );
  LEAVE_GL();
}

static void WINAPI wine_glGetFragmentLightfvSGIX( GLenum light, GLenum pname, GLfloat* params ) {
  void (*func_glGetFragmentLightfvSGIX)( GLenum, GLenum, GLfloat* ) = extension_funcs[285];
  TRACE("(%d, %d, %p)\n", light, pname, params );
  ENTER_GL();
  func_glGetFragmentLightfvSGIX( light, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetFragmentLightivSGIX( GLenum light, GLenum pname, GLint* params ) {
  void (*func_glGetFragmentLightivSGIX)( GLenum, GLenum, GLint* ) = extension_funcs[286];
  TRACE("(%d, %d, %p)\n", light, pname, params );
  ENTER_GL();
  func_glGetFragmentLightivSGIX( light, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetFragmentMaterialfvSGIX( GLenum face, GLenum pname, GLfloat* params ) {
  void (*func_glGetFragmentMaterialfvSGIX)( GLenum, GLenum, GLfloat* ) = extension_funcs[287];
  TRACE("(%d, %d, %p)\n", face, pname, params );
  ENTER_GL();
  func_glGetFragmentMaterialfvSGIX( face, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetFragmentMaterialivSGIX( GLenum face, GLenum pname, GLint* params ) {
  void (*func_glGetFragmentMaterialivSGIX)( GLenum, GLenum, GLint* ) = extension_funcs[288];
  TRACE("(%d, %d, %p)\n", face, pname, params );
  ENTER_GL();
  func_glGetFragmentMaterialivSGIX( face, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetFramebufferAttachmentParameterivEXT( GLenum target, GLenum attachment, GLenum pname, GLint* params ) {
  void (*func_glGetFramebufferAttachmentParameterivEXT)( GLenum, GLenum, GLenum, GLint* ) = extension_funcs[289];
  TRACE("(%d, %d, %d, %p)\n", target, attachment, pname, params );
  ENTER_GL();
  func_glGetFramebufferAttachmentParameterivEXT( target, attachment, pname, params );
  LEAVE_GL();
}

static unsigned int WINAPI wine_glGetHandleARB( GLenum pname ) {
  unsigned int ret_value;
  unsigned int (*func_glGetHandleARB)( GLenum ) = extension_funcs[290];
  TRACE("(%d)\n", pname );
  ENTER_GL();
  ret_value = func_glGetHandleARB( pname );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glGetHistogramEXT( GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid* values ) {
  void (*func_glGetHistogramEXT)( GLenum, GLboolean, GLenum, GLenum, GLvoid* ) = extension_funcs[291];
  TRACE("(%d, %d, %d, %d, %p)\n", target, reset, format, type, values );
  ENTER_GL();
  func_glGetHistogramEXT( target, reset, format, type, values );
  LEAVE_GL();
}

static void WINAPI wine_glGetHistogramParameterfvEXT( GLenum target, GLenum pname, GLfloat* params ) {
  void (*func_glGetHistogramParameterfvEXT)( GLenum, GLenum, GLfloat* ) = extension_funcs[292];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetHistogramParameterfvEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetHistogramParameterivEXT( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glGetHistogramParameterivEXT)( GLenum, GLenum, GLint* ) = extension_funcs[293];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetHistogramParameterivEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetImageTransformParameterfvHP( GLenum target, GLenum pname, GLfloat* params ) {
  void (*func_glGetImageTransformParameterfvHP)( GLenum, GLenum, GLfloat* ) = extension_funcs[294];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetImageTransformParameterfvHP( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetImageTransformParameterivHP( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glGetImageTransformParameterivHP)( GLenum, GLenum, GLint* ) = extension_funcs[295];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetImageTransformParameterivHP( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetInfoLogARB( unsigned int obj, GLsizei maxLength, GLsizei* length, char* infoLog ) {
  void (*func_glGetInfoLogARB)( unsigned int, GLsizei, GLsizei*, char* ) = extension_funcs[296];
  TRACE("(%d, %d, %p, %p)\n", obj, maxLength, length, infoLog );
  ENTER_GL();
  func_glGetInfoLogARB( obj, maxLength, length, infoLog );
  LEAVE_GL();
}

static GLint WINAPI wine_glGetInstrumentsSGIX( void ) {
  GLint ret_value;
  GLint (*func_glGetInstrumentsSGIX)( void ) = extension_funcs[297];
  TRACE("()\n");
  ENTER_GL();
  ret_value = func_glGetInstrumentsSGIX( );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glGetInvariantBooleanvEXT( GLuint id, GLenum value, GLboolean* data ) {
  void (*func_glGetInvariantBooleanvEXT)( GLuint, GLenum, GLboolean* ) = extension_funcs[298];
  TRACE("(%d, %d, %p)\n", id, value, data );
  ENTER_GL();
  func_glGetInvariantBooleanvEXT( id, value, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetInvariantFloatvEXT( GLuint id, GLenum value, GLfloat* data ) {
  void (*func_glGetInvariantFloatvEXT)( GLuint, GLenum, GLfloat* ) = extension_funcs[299];
  TRACE("(%d, %d, %p)\n", id, value, data );
  ENTER_GL();
  func_glGetInvariantFloatvEXT( id, value, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetInvariantIntegervEXT( GLuint id, GLenum value, GLint* data ) {
  void (*func_glGetInvariantIntegervEXT)( GLuint, GLenum, GLint* ) = extension_funcs[300];
  TRACE("(%d, %d, %p)\n", id, value, data );
  ENTER_GL();
  func_glGetInvariantIntegervEXT( id, value, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetListParameterfvSGIX( GLuint list, GLenum pname, GLfloat* params ) {
  void (*func_glGetListParameterfvSGIX)( GLuint, GLenum, GLfloat* ) = extension_funcs[301];
  TRACE("(%d, %d, %p)\n", list, pname, params );
  ENTER_GL();
  func_glGetListParameterfvSGIX( list, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetListParameterivSGIX( GLuint list, GLenum pname, GLint* params ) {
  void (*func_glGetListParameterivSGIX)( GLuint, GLenum, GLint* ) = extension_funcs[302];
  TRACE("(%d, %d, %p)\n", list, pname, params );
  ENTER_GL();
  func_glGetListParameterivSGIX( list, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetLocalConstantBooleanvEXT( GLuint id, GLenum value, GLboolean* data ) {
  void (*func_glGetLocalConstantBooleanvEXT)( GLuint, GLenum, GLboolean* ) = extension_funcs[303];
  TRACE("(%d, %d, %p)\n", id, value, data );
  ENTER_GL();
  func_glGetLocalConstantBooleanvEXT( id, value, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetLocalConstantFloatvEXT( GLuint id, GLenum value, GLfloat* data ) {
  void (*func_glGetLocalConstantFloatvEXT)( GLuint, GLenum, GLfloat* ) = extension_funcs[304];
  TRACE("(%d, %d, %p)\n", id, value, data );
  ENTER_GL();
  func_glGetLocalConstantFloatvEXT( id, value, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetLocalConstantIntegervEXT( GLuint id, GLenum value, GLint* data ) {
  void (*func_glGetLocalConstantIntegervEXT)( GLuint, GLenum, GLint* ) = extension_funcs[305];
  TRACE("(%d, %d, %p)\n", id, value, data );
  ENTER_GL();
  func_glGetLocalConstantIntegervEXT( id, value, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetMapAttribParameterfvNV( GLenum target, GLuint index, GLenum pname, GLfloat* params ) {
  void (*func_glGetMapAttribParameterfvNV)( GLenum, GLuint, GLenum, GLfloat* ) = extension_funcs[306];
  TRACE("(%d, %d, %d, %p)\n", target, index, pname, params );
  ENTER_GL();
  func_glGetMapAttribParameterfvNV( target, index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetMapAttribParameterivNV( GLenum target, GLuint index, GLenum pname, GLint* params ) {
  void (*func_glGetMapAttribParameterivNV)( GLenum, GLuint, GLenum, GLint* ) = extension_funcs[307];
  TRACE("(%d, %d, %d, %p)\n", target, index, pname, params );
  ENTER_GL();
  func_glGetMapAttribParameterivNV( target, index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetMapControlPointsNV( GLenum target, GLuint index, GLenum type, GLsizei ustride, GLsizei vstride, GLboolean packed, GLvoid* points ) {
  void (*func_glGetMapControlPointsNV)( GLenum, GLuint, GLenum, GLsizei, GLsizei, GLboolean, GLvoid* ) = extension_funcs[308];
  TRACE("(%d, %d, %d, %d, %d, %d, %p)\n", target, index, type, ustride, vstride, packed, points );
  ENTER_GL();
  func_glGetMapControlPointsNV( target, index, type, ustride, vstride, packed, points );
  LEAVE_GL();
}

static void WINAPI wine_glGetMapParameterfvNV( GLenum target, GLenum pname, GLfloat* params ) {
  void (*func_glGetMapParameterfvNV)( GLenum, GLenum, GLfloat* ) = extension_funcs[309];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetMapParameterfvNV( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetMapParameterivNV( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glGetMapParameterivNV)( GLenum, GLenum, GLint* ) = extension_funcs[310];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetMapParameterivNV( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetMinmaxEXT( GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid* values ) {
  void (*func_glGetMinmaxEXT)( GLenum, GLboolean, GLenum, GLenum, GLvoid* ) = extension_funcs[311];
  TRACE("(%d, %d, %d, %d, %p)\n", target, reset, format, type, values );
  ENTER_GL();
  func_glGetMinmaxEXT( target, reset, format, type, values );
  LEAVE_GL();
}

static void WINAPI wine_glGetMinmaxParameterfvEXT( GLenum target, GLenum pname, GLfloat* params ) {
  void (*func_glGetMinmaxParameterfvEXT)( GLenum, GLenum, GLfloat* ) = extension_funcs[312];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetMinmaxParameterfvEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetMinmaxParameterivEXT( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glGetMinmaxParameterivEXT)( GLenum, GLenum, GLint* ) = extension_funcs[313];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetMinmaxParameterivEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetObjectBufferfvATI( GLuint buffer, GLenum pname, GLfloat* params ) {
  void (*func_glGetObjectBufferfvATI)( GLuint, GLenum, GLfloat* ) = extension_funcs[314];
  TRACE("(%d, %d, %p)\n", buffer, pname, params );
  ENTER_GL();
  func_glGetObjectBufferfvATI( buffer, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetObjectBufferivATI( GLuint buffer, GLenum pname, GLint* params ) {
  void (*func_glGetObjectBufferivATI)( GLuint, GLenum, GLint* ) = extension_funcs[315];
  TRACE("(%d, %d, %p)\n", buffer, pname, params );
  ENTER_GL();
  func_glGetObjectBufferivATI( buffer, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetObjectParameterfvARB( unsigned int obj, GLenum pname, GLfloat* params ) {
  void (*func_glGetObjectParameterfvARB)( unsigned int, GLenum, GLfloat* ) = extension_funcs[316];
  TRACE("(%d, %d, %p)\n", obj, pname, params );
  ENTER_GL();
  func_glGetObjectParameterfvARB( obj, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetObjectParameterivARB( unsigned int obj, GLenum pname, GLint* params ) {
  void (*func_glGetObjectParameterivARB)( unsigned int, GLenum, GLint* ) = extension_funcs[317];
  TRACE("(%d, %d, %p)\n", obj, pname, params );
  ENTER_GL();
  func_glGetObjectParameterivARB( obj, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetOcclusionQueryivNV( GLuint id, GLenum pname, GLint* params ) {
  void (*func_glGetOcclusionQueryivNV)( GLuint, GLenum, GLint* ) = extension_funcs[318];
  TRACE("(%d, %d, %p)\n", id, pname, params );
  ENTER_GL();
  func_glGetOcclusionQueryivNV( id, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetOcclusionQueryuivNV( GLuint id, GLenum pname, GLuint* params ) {
  void (*func_glGetOcclusionQueryuivNV)( GLuint, GLenum, GLuint* ) = extension_funcs[319];
  TRACE("(%d, %d, %p)\n", id, pname, params );
  ENTER_GL();
  func_glGetOcclusionQueryuivNV( id, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetPixelTexGenParameterfvSGIS( GLenum pname, GLfloat* params ) {
  void (*func_glGetPixelTexGenParameterfvSGIS)( GLenum, GLfloat* ) = extension_funcs[320];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glGetPixelTexGenParameterfvSGIS( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetPixelTexGenParameterivSGIS( GLenum pname, GLint* params ) {
  void (*func_glGetPixelTexGenParameterivSGIS)( GLenum, GLint* ) = extension_funcs[321];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glGetPixelTexGenParameterivSGIS( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetPointervEXT( GLenum pname, GLvoid** params ) {
  void (*func_glGetPointervEXT)( GLenum, GLvoid** ) = extension_funcs[322];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glGetPointervEXT( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramEnvParameterdvARB( GLenum target, GLuint index, GLdouble* params ) {
  void (*func_glGetProgramEnvParameterdvARB)( GLenum, GLuint, GLdouble* ) = extension_funcs[323];
  TRACE("(%d, %d, %p)\n", target, index, params );
  ENTER_GL();
  func_glGetProgramEnvParameterdvARB( target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramEnvParameterfvARB( GLenum target, GLuint index, GLfloat* params ) {
  void (*func_glGetProgramEnvParameterfvARB)( GLenum, GLuint, GLfloat* ) = extension_funcs[324];
  TRACE("(%d, %d, %p)\n", target, index, params );
  ENTER_GL();
  func_glGetProgramEnvParameterfvARB( target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramInfoLog( GLuint program, GLsizei bufSize, GLsizei* length, char* infoLog ) {
  void (*func_glGetProgramInfoLog)( GLuint, GLsizei, GLsizei*, char* ) = extension_funcs[325];
  TRACE("(%d, %d, %p, %p)\n", program, bufSize, length, infoLog );
  ENTER_GL();
  func_glGetProgramInfoLog( program, bufSize, length, infoLog );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramLocalParameterdvARB( GLenum target, GLuint index, GLdouble* params ) {
  void (*func_glGetProgramLocalParameterdvARB)( GLenum, GLuint, GLdouble* ) = extension_funcs[326];
  TRACE("(%d, %d, %p)\n", target, index, params );
  ENTER_GL();
  func_glGetProgramLocalParameterdvARB( target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramLocalParameterfvARB( GLenum target, GLuint index, GLfloat* params ) {
  void (*func_glGetProgramLocalParameterfvARB)( GLenum, GLuint, GLfloat* ) = extension_funcs[327];
  TRACE("(%d, %d, %p)\n", target, index, params );
  ENTER_GL();
  func_glGetProgramLocalParameterfvARB( target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramNamedParameterdvNV( GLuint id, GLsizei len, GLubyte* name, GLdouble* params ) {
  void (*func_glGetProgramNamedParameterdvNV)( GLuint, GLsizei, GLubyte*, GLdouble* ) = extension_funcs[328];
  TRACE("(%d, %d, %p, %p)\n", id, len, name, params );
  ENTER_GL();
  func_glGetProgramNamedParameterdvNV( id, len, name, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramNamedParameterfvNV( GLuint id, GLsizei len, GLubyte* name, GLfloat* params ) {
  void (*func_glGetProgramNamedParameterfvNV)( GLuint, GLsizei, GLubyte*, GLfloat* ) = extension_funcs[329];
  TRACE("(%d, %d, %p, %p)\n", id, len, name, params );
  ENTER_GL();
  func_glGetProgramNamedParameterfvNV( id, len, name, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramParameterdvNV( GLenum target, GLuint index, GLenum pname, GLdouble* params ) {
  void (*func_glGetProgramParameterdvNV)( GLenum, GLuint, GLenum, GLdouble* ) = extension_funcs[330];
  TRACE("(%d, %d, %d, %p)\n", target, index, pname, params );
  ENTER_GL();
  func_glGetProgramParameterdvNV( target, index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramParameterfvNV( GLenum target, GLuint index, GLenum pname, GLfloat* params ) {
  void (*func_glGetProgramParameterfvNV)( GLenum, GLuint, GLenum, GLfloat* ) = extension_funcs[331];
  TRACE("(%d, %d, %d, %p)\n", target, index, pname, params );
  ENTER_GL();
  func_glGetProgramParameterfvNV( target, index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramStringARB( GLenum target, GLenum pname, GLvoid* string ) {
  void (*func_glGetProgramStringARB)( GLenum, GLenum, GLvoid* ) = extension_funcs[332];
  TRACE("(%d, %d, %p)\n", target, pname, string );
  ENTER_GL();
  func_glGetProgramStringARB( target, pname, string );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramStringNV( GLuint id, GLenum pname, GLubyte* program ) {
  void (*func_glGetProgramStringNV)( GLuint, GLenum, GLubyte* ) = extension_funcs[333];
  TRACE("(%d, %d, %p)\n", id, pname, program );
  ENTER_GL();
  func_glGetProgramStringNV( id, pname, program );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramiv( GLuint program, GLenum pname, GLint* params ) {
  void (*func_glGetProgramiv)( GLuint, GLenum, GLint* ) = extension_funcs[334];
  TRACE("(%d, %d, %p)\n", program, pname, params );
  ENTER_GL();
  func_glGetProgramiv( program, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramivARB( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glGetProgramivARB)( GLenum, GLenum, GLint* ) = extension_funcs[335];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetProgramivARB( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramivNV( GLuint id, GLenum pname, GLint* params ) {
  void (*func_glGetProgramivNV)( GLuint, GLenum, GLint* ) = extension_funcs[336];
  TRACE("(%d, %d, %p)\n", id, pname, params );
  ENTER_GL();
  func_glGetProgramivNV( id, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetQueryObjectiv( GLuint id, GLenum pname, GLint* params ) {
  void (*func_glGetQueryObjectiv)( GLuint, GLenum, GLint* ) = extension_funcs[337];
  TRACE("(%d, %d, %p)\n", id, pname, params );
  ENTER_GL();
  func_glGetQueryObjectiv( id, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetQueryObjectivARB( GLuint id, GLenum pname, GLint* params ) {
  void (*func_glGetQueryObjectivARB)( GLuint, GLenum, GLint* ) = extension_funcs[338];
  TRACE("(%d, %d, %p)\n", id, pname, params );
  ENTER_GL();
  func_glGetQueryObjectivARB( id, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetQueryObjectuiv( GLuint id, GLenum pname, GLuint* params ) {
  void (*func_glGetQueryObjectuiv)( GLuint, GLenum, GLuint* ) = extension_funcs[339];
  TRACE("(%d, %d, %p)\n", id, pname, params );
  ENTER_GL();
  func_glGetQueryObjectuiv( id, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetQueryObjectuivARB( GLuint id, GLenum pname, GLuint* params ) {
  void (*func_glGetQueryObjectuivARB)( GLuint, GLenum, GLuint* ) = extension_funcs[340];
  TRACE("(%d, %d, %p)\n", id, pname, params );
  ENTER_GL();
  func_glGetQueryObjectuivARB( id, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetQueryiv( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glGetQueryiv)( GLenum, GLenum, GLint* ) = extension_funcs[341];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetQueryiv( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetQueryivARB( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glGetQueryivARB)( GLenum, GLenum, GLint* ) = extension_funcs[342];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetQueryivARB( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetRenderbufferParameterivEXT( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glGetRenderbufferParameterivEXT)( GLenum, GLenum, GLint* ) = extension_funcs[343];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetRenderbufferParameterivEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetSeparableFilterEXT( GLenum target, GLenum format, GLenum type, GLvoid* row, GLvoid* column, GLvoid* span ) {
  void (*func_glGetSeparableFilterEXT)( GLenum, GLenum, GLenum, GLvoid*, GLvoid*, GLvoid* ) = extension_funcs[344];
  TRACE("(%d, %d, %d, %p, %p, %p)\n", target, format, type, row, column, span );
  ENTER_GL();
  func_glGetSeparableFilterEXT( target, format, type, row, column, span );
  LEAVE_GL();
}

static void WINAPI wine_glGetShaderInfoLog( GLuint shader, GLsizei bufSize, GLsizei* length, char* infoLog ) {
  void (*func_glGetShaderInfoLog)( GLuint, GLsizei, GLsizei*, char* ) = extension_funcs[345];
  TRACE("(%d, %d, %p, %p)\n", shader, bufSize, length, infoLog );
  ENTER_GL();
  func_glGetShaderInfoLog( shader, bufSize, length, infoLog );
  LEAVE_GL();
}

static void WINAPI wine_glGetShaderSource( GLuint shader, GLsizei bufSize, GLsizei* length, char* source ) {
  void (*func_glGetShaderSource)( GLuint, GLsizei, GLsizei*, char* ) = extension_funcs[346];
  TRACE("(%d, %d, %p, %p)\n", shader, bufSize, length, source );
  ENTER_GL();
  func_glGetShaderSource( shader, bufSize, length, source );
  LEAVE_GL();
}

static void WINAPI wine_glGetShaderSourceARB( unsigned int obj, GLsizei maxLength, GLsizei* length, char* source ) {
  void (*func_glGetShaderSourceARB)( unsigned int, GLsizei, GLsizei*, char* ) = extension_funcs[347];
  TRACE("(%d, %d, %p, %p)\n", obj, maxLength, length, source );
  ENTER_GL();
  func_glGetShaderSourceARB( obj, maxLength, length, source );
  LEAVE_GL();
}

static void WINAPI wine_glGetShaderiv( GLuint shader, GLenum pname, GLint* params ) {
  void (*func_glGetShaderiv)( GLuint, GLenum, GLint* ) = extension_funcs[348];
  TRACE("(%d, %d, %p)\n", shader, pname, params );
  ENTER_GL();
  func_glGetShaderiv( shader, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetSharpenTexFuncSGIS( GLenum target, GLfloat* points ) {
  void (*func_glGetSharpenTexFuncSGIS)( GLenum, GLfloat* ) = extension_funcs[349];
  TRACE("(%d, %p)\n", target, points );
  ENTER_GL();
  func_glGetSharpenTexFuncSGIS( target, points );
  LEAVE_GL();
}

static void WINAPI wine_glGetTexBumpParameterfvATI( GLenum pname, GLfloat* param ) {
  void (*func_glGetTexBumpParameterfvATI)( GLenum, GLfloat* ) = extension_funcs[350];
  TRACE("(%d, %p)\n", pname, param );
  ENTER_GL();
  func_glGetTexBumpParameterfvATI( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glGetTexBumpParameterivATI( GLenum pname, GLint* param ) {
  void (*func_glGetTexBumpParameterivATI)( GLenum, GLint* ) = extension_funcs[351];
  TRACE("(%d, %p)\n", pname, param );
  ENTER_GL();
  func_glGetTexBumpParameterivATI( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glGetTexFilterFuncSGIS( GLenum target, GLenum filter, GLfloat* weights ) {
  void (*func_glGetTexFilterFuncSGIS)( GLenum, GLenum, GLfloat* ) = extension_funcs[352];
  TRACE("(%d, %d, %p)\n", target, filter, weights );
  ENTER_GL();
  func_glGetTexFilterFuncSGIS( target, filter, weights );
  LEAVE_GL();
}

static void WINAPI wine_glGetTrackMatrixivNV( GLenum target, GLuint address, GLenum pname, GLint* params ) {
  void (*func_glGetTrackMatrixivNV)( GLenum, GLuint, GLenum, GLint* ) = extension_funcs[353];
  TRACE("(%d, %d, %d, %p)\n", target, address, pname, params );
  ENTER_GL();
  func_glGetTrackMatrixivNV( target, address, pname, params );
  LEAVE_GL();
}

static GLint WINAPI wine_glGetUniformLocation( GLuint program, char* name ) {
  GLint ret_value;
  GLint (*func_glGetUniformLocation)( GLuint, char* ) = extension_funcs[354];
  TRACE("(%d, %p)\n", program, name );
  ENTER_GL();
  ret_value = func_glGetUniformLocation( program, name );
  LEAVE_GL();
  return ret_value;
}

static GLint WINAPI wine_glGetUniformLocationARB( unsigned int programObj, char* name ) {
  GLint ret_value;
  GLint (*func_glGetUniformLocationARB)( unsigned int, char* ) = extension_funcs[355];
  TRACE("(%d, %p)\n", programObj, name );
  ENTER_GL();
  ret_value = func_glGetUniformLocationARB( programObj, name );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glGetUniformfv( GLuint program, GLint location, GLfloat* params ) {
  void (*func_glGetUniformfv)( GLuint, GLint, GLfloat* ) = extension_funcs[356];
  TRACE("(%d, %d, %p)\n", program, location, params );
  ENTER_GL();
  func_glGetUniformfv( program, location, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetUniformfvARB( unsigned int programObj, GLint location, GLfloat* params ) {
  void (*func_glGetUniformfvARB)( unsigned int, GLint, GLfloat* ) = extension_funcs[357];
  TRACE("(%d, %d, %p)\n", programObj, location, params );
  ENTER_GL();
  func_glGetUniformfvARB( programObj, location, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetUniformiv( GLuint program, GLint location, GLint* params ) {
  void (*func_glGetUniformiv)( GLuint, GLint, GLint* ) = extension_funcs[358];
  TRACE("(%d, %d, %p)\n", program, location, params );
  ENTER_GL();
  func_glGetUniformiv( program, location, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetUniformivARB( unsigned int programObj, GLint location, GLint* params ) {
  void (*func_glGetUniformivARB)( unsigned int, GLint, GLint* ) = extension_funcs[359];
  TRACE("(%d, %d, %p)\n", programObj, location, params );
  ENTER_GL();
  func_glGetUniformivARB( programObj, location, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVariantArrayObjectfvATI( GLuint id, GLenum pname, GLfloat* params ) {
  void (*func_glGetVariantArrayObjectfvATI)( GLuint, GLenum, GLfloat* ) = extension_funcs[360];
  TRACE("(%d, %d, %p)\n", id, pname, params );
  ENTER_GL();
  func_glGetVariantArrayObjectfvATI( id, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVariantArrayObjectivATI( GLuint id, GLenum pname, GLint* params ) {
  void (*func_glGetVariantArrayObjectivATI)( GLuint, GLenum, GLint* ) = extension_funcs[361];
  TRACE("(%d, %d, %p)\n", id, pname, params );
  ENTER_GL();
  func_glGetVariantArrayObjectivATI( id, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVariantBooleanvEXT( GLuint id, GLenum value, GLboolean* data ) {
  void (*func_glGetVariantBooleanvEXT)( GLuint, GLenum, GLboolean* ) = extension_funcs[362];
  TRACE("(%d, %d, %p)\n", id, value, data );
  ENTER_GL();
  func_glGetVariantBooleanvEXT( id, value, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetVariantFloatvEXT( GLuint id, GLenum value, GLfloat* data ) {
  void (*func_glGetVariantFloatvEXT)( GLuint, GLenum, GLfloat* ) = extension_funcs[363];
  TRACE("(%d, %d, %p)\n", id, value, data );
  ENTER_GL();
  func_glGetVariantFloatvEXT( id, value, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetVariantIntegervEXT( GLuint id, GLenum value, GLint* data ) {
  void (*func_glGetVariantIntegervEXT)( GLuint, GLenum, GLint* ) = extension_funcs[364];
  TRACE("(%d, %d, %p)\n", id, value, data );
  ENTER_GL();
  func_glGetVariantIntegervEXT( id, value, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetVariantPointervEXT( GLuint id, GLenum value, GLvoid** data ) {
  void (*func_glGetVariantPointervEXT)( GLuint, GLenum, GLvoid** ) = extension_funcs[365];
  TRACE("(%d, %d, %p)\n", id, value, data );
  ENTER_GL();
  func_glGetVariantPointervEXT( id, value, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribArrayObjectfvATI( GLuint index, GLenum pname, GLfloat* params ) {
  void (*func_glGetVertexAttribArrayObjectfvATI)( GLuint, GLenum, GLfloat* ) = extension_funcs[366];
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribArrayObjectfvATI( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribArrayObjectivATI( GLuint index, GLenum pname, GLint* params ) {
  void (*func_glGetVertexAttribArrayObjectivATI)( GLuint, GLenum, GLint* ) = extension_funcs[367];
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribArrayObjectivATI( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribPointerv( GLuint index, GLenum pname, GLvoid** pointer ) {
  void (*func_glGetVertexAttribPointerv)( GLuint, GLenum, GLvoid** ) = extension_funcs[368];
  TRACE("(%d, %d, %p)\n", index, pname, pointer );
  ENTER_GL();
  func_glGetVertexAttribPointerv( index, pname, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribPointervARB( GLuint index, GLenum pname, GLvoid** pointer ) {
  void (*func_glGetVertexAttribPointervARB)( GLuint, GLenum, GLvoid** ) = extension_funcs[369];
  TRACE("(%d, %d, %p)\n", index, pname, pointer );
  ENTER_GL();
  func_glGetVertexAttribPointervARB( index, pname, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribPointervNV( GLuint index, GLenum pname, GLvoid** pointer ) {
  void (*func_glGetVertexAttribPointervNV)( GLuint, GLenum, GLvoid** ) = extension_funcs[370];
  TRACE("(%d, %d, %p)\n", index, pname, pointer );
  ENTER_GL();
  func_glGetVertexAttribPointervNV( index, pname, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribdv( GLuint index, GLenum pname, GLdouble* params ) {
  void (*func_glGetVertexAttribdv)( GLuint, GLenum, GLdouble* ) = extension_funcs[371];
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribdv( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribdvARB( GLuint index, GLenum pname, GLdouble* params ) {
  void (*func_glGetVertexAttribdvARB)( GLuint, GLenum, GLdouble* ) = extension_funcs[372];
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribdvARB( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribdvNV( GLuint index, GLenum pname, GLdouble* params ) {
  void (*func_glGetVertexAttribdvNV)( GLuint, GLenum, GLdouble* ) = extension_funcs[373];
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribdvNV( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribfv( GLuint index, GLenum pname, GLfloat* params ) {
  void (*func_glGetVertexAttribfv)( GLuint, GLenum, GLfloat* ) = extension_funcs[374];
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribfv( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribfvARB( GLuint index, GLenum pname, GLfloat* params ) {
  void (*func_glGetVertexAttribfvARB)( GLuint, GLenum, GLfloat* ) = extension_funcs[375];
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribfvARB( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribfvNV( GLuint index, GLenum pname, GLfloat* params ) {
  void (*func_glGetVertexAttribfvNV)( GLuint, GLenum, GLfloat* ) = extension_funcs[376];
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribfvNV( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribiv( GLuint index, GLenum pname, GLint* params ) {
  void (*func_glGetVertexAttribiv)( GLuint, GLenum, GLint* ) = extension_funcs[377];
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribiv( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribivARB( GLuint index, GLenum pname, GLint* params ) {
  void (*func_glGetVertexAttribivARB)( GLuint, GLenum, GLint* ) = extension_funcs[378];
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribivARB( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribivNV( GLuint index, GLenum pname, GLint* params ) {
  void (*func_glGetVertexAttribivNV)( GLuint, GLenum, GLint* ) = extension_funcs[379];
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribivNV( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGlobalAlphaFactorbSUN( GLbyte factor ) {
  void (*func_glGlobalAlphaFactorbSUN)( GLbyte ) = extension_funcs[380];
  TRACE("(%d)\n", factor );
  ENTER_GL();
  func_glGlobalAlphaFactorbSUN( factor );
  LEAVE_GL();
}

static void WINAPI wine_glGlobalAlphaFactordSUN( GLdouble factor ) {
  void (*func_glGlobalAlphaFactordSUN)( GLdouble ) = extension_funcs[381];
  TRACE("(%f)\n", factor );
  ENTER_GL();
  func_glGlobalAlphaFactordSUN( factor );
  LEAVE_GL();
}

static void WINAPI wine_glGlobalAlphaFactorfSUN( GLfloat factor ) {
  void (*func_glGlobalAlphaFactorfSUN)( GLfloat ) = extension_funcs[382];
  TRACE("(%f)\n", factor );
  ENTER_GL();
  func_glGlobalAlphaFactorfSUN( factor );
  LEAVE_GL();
}

static void WINAPI wine_glGlobalAlphaFactoriSUN( GLint factor ) {
  void (*func_glGlobalAlphaFactoriSUN)( GLint ) = extension_funcs[383];
  TRACE("(%d)\n", factor );
  ENTER_GL();
  func_glGlobalAlphaFactoriSUN( factor );
  LEAVE_GL();
}

static void WINAPI wine_glGlobalAlphaFactorsSUN( GLshort factor ) {
  void (*func_glGlobalAlphaFactorsSUN)( GLshort ) = extension_funcs[384];
  TRACE("(%d)\n", factor );
  ENTER_GL();
  func_glGlobalAlphaFactorsSUN( factor );
  LEAVE_GL();
}

static void WINAPI wine_glGlobalAlphaFactorubSUN( GLubyte factor ) {
  void (*func_glGlobalAlphaFactorubSUN)( GLubyte ) = extension_funcs[385];
  TRACE("(%d)\n", factor );
  ENTER_GL();
  func_glGlobalAlphaFactorubSUN( factor );
  LEAVE_GL();
}

static void WINAPI wine_glGlobalAlphaFactoruiSUN( GLuint factor ) {
  void (*func_glGlobalAlphaFactoruiSUN)( GLuint ) = extension_funcs[386];
  TRACE("(%d)\n", factor );
  ENTER_GL();
  func_glGlobalAlphaFactoruiSUN( factor );
  LEAVE_GL();
}

static void WINAPI wine_glGlobalAlphaFactorusSUN( GLushort factor ) {
  void (*func_glGlobalAlphaFactorusSUN)( GLushort ) = extension_funcs[387];
  TRACE("(%d)\n", factor );
  ENTER_GL();
  func_glGlobalAlphaFactorusSUN( factor );
  LEAVE_GL();
}

static void WINAPI wine_glHintPGI( GLenum target, GLint mode ) {
  void (*func_glHintPGI)( GLenum, GLint ) = extension_funcs[388];
  TRACE("(%d, %d)\n", target, mode );
  ENTER_GL();
  func_glHintPGI( target, mode );
  LEAVE_GL();
}

static void WINAPI wine_glHistogramEXT( GLenum target, GLsizei width, GLenum internalformat, GLboolean sink ) {
  void (*func_glHistogramEXT)( GLenum, GLsizei, GLenum, GLboolean ) = extension_funcs[389];
  TRACE("(%d, %d, %d, %d)\n", target, width, internalformat, sink );
  ENTER_GL();
  func_glHistogramEXT( target, width, internalformat, sink );
  LEAVE_GL();
}

static void WINAPI wine_glIglooInterfaceSGIX( GLint pname, GLint* params ) {
  void (*func_glIglooInterfaceSGIX)( GLint, GLint* ) = extension_funcs[390];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glIglooInterfaceSGIX( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glImageTransformParameterfHP( GLenum target, GLenum pname, GLfloat param ) {
  void (*func_glImageTransformParameterfHP)( GLenum, GLenum, GLfloat ) = extension_funcs[391];
  TRACE("(%d, %d, %f)\n", target, pname, param );
  ENTER_GL();
  func_glImageTransformParameterfHP( target, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glImageTransformParameterfvHP( GLenum target, GLenum pname, GLfloat* params ) {
  void (*func_glImageTransformParameterfvHP)( GLenum, GLenum, GLfloat* ) = extension_funcs[392];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glImageTransformParameterfvHP( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glImageTransformParameteriHP( GLenum target, GLenum pname, GLint param ) {
  void (*func_glImageTransformParameteriHP)( GLenum, GLenum, GLint ) = extension_funcs[393];
  TRACE("(%d, %d, %d)\n", target, pname, param );
  ENTER_GL();
  func_glImageTransformParameteriHP( target, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glImageTransformParameterivHP( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glImageTransformParameterivHP)( GLenum, GLenum, GLint* ) = extension_funcs[394];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glImageTransformParameterivHP( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glIndexFuncEXT( GLenum func, GLclampf ref ) {
  void (*func_glIndexFuncEXT)( GLenum, GLclampf ) = extension_funcs[395];
  TRACE("(%d, %f)\n", func, ref );
  ENTER_GL();
  func_glIndexFuncEXT( func, ref );
  LEAVE_GL();
}

static void WINAPI wine_glIndexMaterialEXT( GLenum face, GLenum mode ) {
  void (*func_glIndexMaterialEXT)( GLenum, GLenum ) = extension_funcs[396];
  TRACE("(%d, %d)\n", face, mode );
  ENTER_GL();
  func_glIndexMaterialEXT( face, mode );
  LEAVE_GL();
}

static void WINAPI wine_glIndexPointerEXT( GLenum type, GLsizei stride, GLsizei count, GLvoid* pointer ) {
  void (*func_glIndexPointerEXT)( GLenum, GLsizei, GLsizei, GLvoid* ) = extension_funcs[397];
  TRACE("(%d, %d, %d, %p)\n", type, stride, count, pointer );
  ENTER_GL();
  func_glIndexPointerEXT( type, stride, count, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glIndexPointerListIBM( GLenum type, GLint stride, GLvoid** pointer, GLint ptrstride ) {
  void (*func_glIndexPointerListIBM)( GLenum, GLint, GLvoid**, GLint ) = extension_funcs[398];
  TRACE("(%d, %d, %p, %d)\n", type, stride, pointer, ptrstride );
  ENTER_GL();
  func_glIndexPointerListIBM( type, stride, pointer, ptrstride );
  LEAVE_GL();
}

static void WINAPI wine_glInsertComponentEXT( GLuint res, GLuint src, GLuint num ) {
  void (*func_glInsertComponentEXT)( GLuint, GLuint, GLuint ) = extension_funcs[399];
  TRACE("(%d, %d, %d)\n", res, src, num );
  ENTER_GL();
  func_glInsertComponentEXT( res, src, num );
  LEAVE_GL();
}

static void WINAPI wine_glInstrumentsBufferSGIX( GLsizei size, GLint* buffer ) {
  void (*func_glInstrumentsBufferSGIX)( GLsizei, GLint* ) = extension_funcs[400];
  TRACE("(%d, %p)\n", size, buffer );
  ENTER_GL();
  func_glInstrumentsBufferSGIX( size, buffer );
  LEAVE_GL();
}

static GLboolean WINAPI wine_glIsAsyncMarkerSGIX( GLuint marker ) {
  GLboolean ret_value;
  GLboolean (*func_glIsAsyncMarkerSGIX)( GLuint ) = extension_funcs[401];
  TRACE("(%d)\n", marker );
  ENTER_GL();
  ret_value = func_glIsAsyncMarkerSGIX( marker );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsBuffer( GLuint buffer ) {
  GLboolean ret_value;
  GLboolean (*func_glIsBuffer)( GLuint ) = extension_funcs[402];
  TRACE("(%d)\n", buffer );
  ENTER_GL();
  ret_value = func_glIsBuffer( buffer );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsBufferARB( GLuint buffer ) {
  GLboolean ret_value;
  GLboolean (*func_glIsBufferARB)( GLuint ) = extension_funcs[403];
  TRACE("(%d)\n", buffer );
  ENTER_GL();
  ret_value = func_glIsBufferARB( buffer );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsFenceAPPLE( GLuint fence ) {
  GLboolean ret_value;
  GLboolean (*func_glIsFenceAPPLE)( GLuint ) = extension_funcs[404];
  TRACE("(%d)\n", fence );
  ENTER_GL();
  ret_value = func_glIsFenceAPPLE( fence );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsFenceNV( GLuint fence ) {
  GLboolean ret_value;
  GLboolean (*func_glIsFenceNV)( GLuint ) = extension_funcs[405];
  TRACE("(%d)\n", fence );
  ENTER_GL();
  ret_value = func_glIsFenceNV( fence );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsFramebufferEXT( GLuint framebuffer ) {
  GLboolean ret_value;
  GLboolean (*func_glIsFramebufferEXT)( GLuint ) = extension_funcs[406];
  TRACE("(%d)\n", framebuffer );
  ENTER_GL();
  ret_value = func_glIsFramebufferEXT( framebuffer );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsObjectBufferATI( GLuint buffer ) {
  GLboolean ret_value;
  GLboolean (*func_glIsObjectBufferATI)( GLuint ) = extension_funcs[407];
  TRACE("(%d)\n", buffer );
  ENTER_GL();
  ret_value = func_glIsObjectBufferATI( buffer );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsOcclusionQueryNV( GLuint id ) {
  GLboolean ret_value;
  GLboolean (*func_glIsOcclusionQueryNV)( GLuint ) = extension_funcs[408];
  TRACE("(%d)\n", id );
  ENTER_GL();
  ret_value = func_glIsOcclusionQueryNV( id );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsProgram( GLuint program ) {
  GLboolean ret_value;
  GLboolean (*func_glIsProgram)( GLuint ) = extension_funcs[409];
  TRACE("(%d)\n", program );
  ENTER_GL();
  ret_value = func_glIsProgram( program );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsProgramARB( GLuint program ) {
  GLboolean ret_value;
  GLboolean (*func_glIsProgramARB)( GLuint ) = extension_funcs[410];
  TRACE("(%d)\n", program );
  ENTER_GL();
  ret_value = func_glIsProgramARB( program );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsProgramNV( GLuint id ) {
  GLboolean ret_value;
  GLboolean (*func_glIsProgramNV)( GLuint ) = extension_funcs[411];
  TRACE("(%d)\n", id );
  ENTER_GL();
  ret_value = func_glIsProgramNV( id );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsQuery( GLuint id ) {
  GLboolean ret_value;
  GLboolean (*func_glIsQuery)( GLuint ) = extension_funcs[412];
  TRACE("(%d)\n", id );
  ENTER_GL();
  ret_value = func_glIsQuery( id );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsQueryARB( GLuint id ) {
  GLboolean ret_value;
  GLboolean (*func_glIsQueryARB)( GLuint ) = extension_funcs[413];
  TRACE("(%d)\n", id );
  ENTER_GL();
  ret_value = func_glIsQueryARB( id );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsRenderbufferEXT( GLuint renderbuffer ) {
  GLboolean ret_value;
  GLboolean (*func_glIsRenderbufferEXT)( GLuint ) = extension_funcs[414];
  TRACE("(%d)\n", renderbuffer );
  ENTER_GL();
  ret_value = func_glIsRenderbufferEXT( renderbuffer );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsShader( GLuint shader ) {
  GLboolean ret_value;
  GLboolean (*func_glIsShader)( GLuint ) = extension_funcs[415];
  TRACE("(%d)\n", shader );
  ENTER_GL();
  ret_value = func_glIsShader( shader );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsTextureEXT( GLuint texture ) {
  GLboolean ret_value;
  GLboolean (*func_glIsTextureEXT)( GLuint ) = extension_funcs[416];
  TRACE("(%d)\n", texture );
  ENTER_GL();
  ret_value = func_glIsTextureEXT( texture );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsVariantEnabledEXT( GLuint id, GLenum cap ) {
  GLboolean ret_value;
  GLboolean (*func_glIsVariantEnabledEXT)( GLuint, GLenum ) = extension_funcs[417];
  TRACE("(%d, %d)\n", id, cap );
  ENTER_GL();
  ret_value = func_glIsVariantEnabledEXT( id, cap );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsVertexArrayAPPLE( GLuint array ) {
  GLboolean ret_value;
  GLboolean (*func_glIsVertexArrayAPPLE)( GLuint ) = extension_funcs[418];
  TRACE("(%d)\n", array );
  ENTER_GL();
  ret_value = func_glIsVertexArrayAPPLE( array );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glLightEnviSGIX( GLenum pname, GLint param ) {
  void (*func_glLightEnviSGIX)( GLenum, GLint ) = extension_funcs[419];
  TRACE("(%d, %d)\n", pname, param );
  ENTER_GL();
  func_glLightEnviSGIX( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glLinkProgram( GLuint program ) {
  void (*func_glLinkProgram)( GLuint ) = extension_funcs[420];
  TRACE("(%d)\n", program );
  ENTER_GL();
  func_glLinkProgram( program );
  LEAVE_GL();
}

static void WINAPI wine_glLinkProgramARB( unsigned int programObj ) {
  void (*func_glLinkProgramARB)( unsigned int ) = extension_funcs[421];
  TRACE("(%d)\n", programObj );
  ENTER_GL();
  func_glLinkProgramARB( programObj );
  LEAVE_GL();
}

static void WINAPI wine_glListParameterfSGIX( GLuint list, GLenum pname, GLfloat param ) {
  void (*func_glListParameterfSGIX)( GLuint, GLenum, GLfloat ) = extension_funcs[422];
  TRACE("(%d, %d, %f)\n", list, pname, param );
  ENTER_GL();
  func_glListParameterfSGIX( list, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glListParameterfvSGIX( GLuint list, GLenum pname, GLfloat* params ) {
  void (*func_glListParameterfvSGIX)( GLuint, GLenum, GLfloat* ) = extension_funcs[423];
  TRACE("(%d, %d, %p)\n", list, pname, params );
  ENTER_GL();
  func_glListParameterfvSGIX( list, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glListParameteriSGIX( GLuint list, GLenum pname, GLint param ) {
  void (*func_glListParameteriSGIX)( GLuint, GLenum, GLint ) = extension_funcs[424];
  TRACE("(%d, %d, %d)\n", list, pname, param );
  ENTER_GL();
  func_glListParameteriSGIX( list, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glListParameterivSGIX( GLuint list, GLenum pname, GLint* params ) {
  void (*func_glListParameterivSGIX)( GLuint, GLenum, GLint* ) = extension_funcs[425];
  TRACE("(%d, %d, %p)\n", list, pname, params );
  ENTER_GL();
  func_glListParameterivSGIX( list, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glLoadIdentityDeformationMapSGIX( GLint mask ) {
  void (*func_glLoadIdentityDeformationMapSGIX)( GLint ) = extension_funcs[426];
  TRACE("(%d)\n", mask );
  ENTER_GL();
  func_glLoadIdentityDeformationMapSGIX( mask );
  LEAVE_GL();
}

static void WINAPI wine_glLoadProgramNV( GLenum target, GLuint id, GLsizei len, GLubyte* program ) {
  void (*func_glLoadProgramNV)( GLenum, GLuint, GLsizei, GLubyte* ) = extension_funcs[427];
  TRACE("(%d, %d, %d, %p)\n", target, id, len, program );
  ENTER_GL();
  func_glLoadProgramNV( target, id, len, program );
  LEAVE_GL();
}

static void WINAPI wine_glLoadTransposeMatrixd( GLdouble* m ) {
  void (*func_glLoadTransposeMatrixd)( GLdouble* ) = extension_funcs[428];
  TRACE("(%p)\n", m );
  ENTER_GL();
  func_glLoadTransposeMatrixd( m );
  LEAVE_GL();
}

static void WINAPI wine_glLoadTransposeMatrixdARB( GLdouble* m ) {
  void (*func_glLoadTransposeMatrixdARB)( GLdouble* ) = extension_funcs[429];
  TRACE("(%p)\n", m );
  ENTER_GL();
  func_glLoadTransposeMatrixdARB( m );
  LEAVE_GL();
}

static void WINAPI wine_glLoadTransposeMatrixf( GLfloat* m ) {
  void (*func_glLoadTransposeMatrixf)( GLfloat* ) = extension_funcs[430];
  TRACE("(%p)\n", m );
  ENTER_GL();
  func_glLoadTransposeMatrixf( m );
  LEAVE_GL();
}

static void WINAPI wine_glLoadTransposeMatrixfARB( GLfloat* m ) {
  void (*func_glLoadTransposeMatrixfARB)( GLfloat* ) = extension_funcs[431];
  TRACE("(%p)\n", m );
  ENTER_GL();
  func_glLoadTransposeMatrixfARB( m );
  LEAVE_GL();
}

static void WINAPI wine_glLockArraysEXT( GLint first, GLsizei count ) {
  void (*func_glLockArraysEXT)( GLint, GLsizei ) = extension_funcs[432];
  TRACE("(%d, %d)\n", first, count );
  ENTER_GL();
  func_glLockArraysEXT( first, count );
  LEAVE_GL();
}

static void WINAPI wine_glMTexCoord2fSGIS( GLenum target, GLfloat s, GLfloat t ) {
  void (*func_glMTexCoord2fSGIS)( GLenum, GLfloat, GLfloat ) = extension_funcs[433];
  TRACE("(%d, %f, %f)\n", target, s, t );
  ENTER_GL();
  func_glMTexCoord2fSGIS( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMTexCoord2fvSGIS( GLenum target, GLfloat * v ) {
  void (*func_glMTexCoord2fvSGIS)( GLenum, GLfloat * ) = extension_funcs[434];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMTexCoord2fvSGIS( target, v );
  LEAVE_GL();
}

static GLvoid* WINAPI wine_glMapBuffer( GLenum target, GLenum access ) {
  GLvoid* ret_value;
  GLvoid* (*func_glMapBuffer)( GLenum, GLenum ) = extension_funcs[435];
  TRACE("(%d, %d)\n", target, access );
  ENTER_GL();
  ret_value = func_glMapBuffer( target, access );
  LEAVE_GL();
  return ret_value;
}

static GLvoid* WINAPI wine_glMapBufferARB( GLenum target, GLenum access ) {
  GLvoid* ret_value;
  GLvoid* (*func_glMapBufferARB)( GLenum, GLenum ) = extension_funcs[436];
  TRACE("(%d, %d)\n", target, access );
  ENTER_GL();
  ret_value = func_glMapBufferARB( target, access );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glMapControlPointsNV( GLenum target, GLuint index, GLenum type, GLsizei ustride, GLsizei vstride, GLint uorder, GLint vorder, GLboolean packed, GLvoid* points ) {
  void (*func_glMapControlPointsNV)( GLenum, GLuint, GLenum, GLsizei, GLsizei, GLint, GLint, GLboolean, GLvoid* ) = extension_funcs[437];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, index, type, ustride, vstride, uorder, vorder, packed, points );
  ENTER_GL();
  func_glMapControlPointsNV( target, index, type, ustride, vstride, uorder, vorder, packed, points );
  LEAVE_GL();
}

static GLvoid* WINAPI wine_glMapObjectBufferATI( GLuint buffer ) {
  GLvoid* ret_value;
  GLvoid* (*func_glMapObjectBufferATI)( GLuint ) = extension_funcs[438];
  TRACE("(%d)\n", buffer );
  ENTER_GL();
  ret_value = func_glMapObjectBufferATI( buffer );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glMapParameterfvNV( GLenum target, GLenum pname, GLfloat* params ) {
  void (*func_glMapParameterfvNV)( GLenum, GLenum, GLfloat* ) = extension_funcs[439];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glMapParameterfvNV( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glMapParameterivNV( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glMapParameterivNV)( GLenum, GLenum, GLint* ) = extension_funcs[440];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glMapParameterivNV( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glMatrixIndexPointerARB( GLint size, GLenum type, GLsizei stride, GLvoid* pointer ) {
  void (*func_glMatrixIndexPointerARB)( GLint, GLenum, GLsizei, GLvoid* ) = extension_funcs[441];
  TRACE("(%d, %d, %d, %p)\n", size, type, stride, pointer );
  ENTER_GL();
  func_glMatrixIndexPointerARB( size, type, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glMatrixIndexubvARB( GLint size, GLubyte* indices ) {
  void (*func_glMatrixIndexubvARB)( GLint, GLubyte* ) = extension_funcs[442];
  TRACE("(%d, %p)\n", size, indices );
  ENTER_GL();
  func_glMatrixIndexubvARB( size, indices );
  LEAVE_GL();
}

static void WINAPI wine_glMatrixIndexuivARB( GLint size, GLuint* indices ) {
  void (*func_glMatrixIndexuivARB)( GLint, GLuint* ) = extension_funcs[443];
  TRACE("(%d, %p)\n", size, indices );
  ENTER_GL();
  func_glMatrixIndexuivARB( size, indices );
  LEAVE_GL();
}

static void WINAPI wine_glMatrixIndexusvARB( GLint size, GLushort* indices ) {
  void (*func_glMatrixIndexusvARB)( GLint, GLushort* ) = extension_funcs[444];
  TRACE("(%d, %p)\n", size, indices );
  ENTER_GL();
  func_glMatrixIndexusvARB( size, indices );
  LEAVE_GL();
}

static void WINAPI wine_glMinmaxEXT( GLenum target, GLenum internalformat, GLboolean sink ) {
  void (*func_glMinmaxEXT)( GLenum, GLenum, GLboolean ) = extension_funcs[445];
  TRACE("(%d, %d, %d)\n", target, internalformat, sink );
  ENTER_GL();
  func_glMinmaxEXT( target, internalformat, sink );
  LEAVE_GL();
}

static void WINAPI wine_glMultTransposeMatrixd( GLdouble* m ) {
  void (*func_glMultTransposeMatrixd)( GLdouble* ) = extension_funcs[446];
  TRACE("(%p)\n", m );
  ENTER_GL();
  func_glMultTransposeMatrixd( m );
  LEAVE_GL();
}

static void WINAPI wine_glMultTransposeMatrixdARB( GLdouble* m ) {
  void (*func_glMultTransposeMatrixdARB)( GLdouble* ) = extension_funcs[447];
  TRACE("(%p)\n", m );
  ENTER_GL();
  func_glMultTransposeMatrixdARB( m );
  LEAVE_GL();
}

static void WINAPI wine_glMultTransposeMatrixf( GLfloat* m ) {
  void (*func_glMultTransposeMatrixf)( GLfloat* ) = extension_funcs[448];
  TRACE("(%p)\n", m );
  ENTER_GL();
  func_glMultTransposeMatrixf( m );
  LEAVE_GL();
}

static void WINAPI wine_glMultTransposeMatrixfARB( GLfloat* m ) {
  void (*func_glMultTransposeMatrixfARB)( GLfloat* ) = extension_funcs[449];
  TRACE("(%p)\n", m );
  ENTER_GL();
  func_glMultTransposeMatrixfARB( m );
  LEAVE_GL();
}

static void WINAPI wine_glMultiDrawArrays( GLenum mode, GLint* first, GLsizei* count, GLsizei primcount ) {
  void (*func_glMultiDrawArrays)( GLenum, GLint*, GLsizei*, GLsizei ) = extension_funcs[450];
  TRACE("(%d, %p, %p, %d)\n", mode, first, count, primcount );
  ENTER_GL();
  func_glMultiDrawArrays( mode, first, count, primcount );
  LEAVE_GL();
}

static void WINAPI wine_glMultiDrawArraysEXT( GLenum mode, GLint* first, GLsizei* count, GLsizei primcount ) {
  void (*func_glMultiDrawArraysEXT)( GLenum, GLint*, GLsizei*, GLsizei ) = extension_funcs[451];
  TRACE("(%d, %p, %p, %d)\n", mode, first, count, primcount );
  ENTER_GL();
  func_glMultiDrawArraysEXT( mode, first, count, primcount );
  LEAVE_GL();
}

static void WINAPI wine_glMultiDrawElementArrayAPPLE( GLenum mode, GLint* first, GLsizei* count, GLsizei primcount ) {
  void (*func_glMultiDrawElementArrayAPPLE)( GLenum, GLint*, GLsizei*, GLsizei ) = extension_funcs[452];
  TRACE("(%d, %p, %p, %d)\n", mode, first, count, primcount );
  ENTER_GL();
  func_glMultiDrawElementArrayAPPLE( mode, first, count, primcount );
  LEAVE_GL();
}

static void WINAPI wine_glMultiDrawElements( GLenum mode, GLsizei* count, GLenum type, GLvoid** indices, GLsizei primcount ) {
  void (*func_glMultiDrawElements)( GLenum, GLsizei*, GLenum, GLvoid**, GLsizei ) = extension_funcs[453];
  TRACE("(%d, %p, %d, %p, %d)\n", mode, count, type, indices, primcount );
  ENTER_GL();
  func_glMultiDrawElements( mode, count, type, indices, primcount );
  LEAVE_GL();
}

static void WINAPI wine_glMultiDrawElementsEXT( GLenum mode, GLsizei* count, GLenum type, GLvoid** indices, GLsizei primcount ) {
  void (*func_glMultiDrawElementsEXT)( GLenum, GLsizei*, GLenum, GLvoid**, GLsizei ) = extension_funcs[454];
  TRACE("(%d, %p, %d, %p, %d)\n", mode, count, type, indices, primcount );
  ENTER_GL();
  func_glMultiDrawElementsEXT( mode, count, type, indices, primcount );
  LEAVE_GL();
}

static void WINAPI wine_glMultiDrawRangeElementArrayAPPLE( GLenum mode, GLuint start, GLuint end, GLint* first, GLsizei* count, GLsizei primcount ) {
  void (*func_glMultiDrawRangeElementArrayAPPLE)( GLenum, GLuint, GLuint, GLint*, GLsizei*, GLsizei ) = extension_funcs[455];
  TRACE("(%d, %d, %d, %p, %p, %d)\n", mode, start, end, first, count, primcount );
  ENTER_GL();
  func_glMultiDrawRangeElementArrayAPPLE( mode, start, end, first, count, primcount );
  LEAVE_GL();
}

static void WINAPI wine_glMultiModeDrawArraysIBM( GLenum* mode, GLint* first, GLsizei* count, GLsizei primcount, GLint modestride ) {
  void (*func_glMultiModeDrawArraysIBM)( GLenum*, GLint*, GLsizei*, GLsizei, GLint ) = extension_funcs[456];
  TRACE("(%p, %p, %p, %d, %d)\n", mode, first, count, primcount, modestride );
  ENTER_GL();
  func_glMultiModeDrawArraysIBM( mode, first, count, primcount, modestride );
  LEAVE_GL();
}

static void WINAPI wine_glMultiModeDrawElementsIBM( GLenum* mode, GLsizei* count, GLenum type, GLvoid* const* indices, GLsizei primcount, GLint modestride ) {
  void (*func_glMultiModeDrawElementsIBM)( GLenum*, GLsizei*, GLenum, GLvoid* const*, GLsizei, GLint ) = extension_funcs[457];
  TRACE("(%p, %p, %d, %p, %d, %d)\n", mode, count, type, indices, primcount, modestride );
  ENTER_GL();
  func_glMultiModeDrawElementsIBM( mode, count, type, indices, primcount, modestride );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1d( GLenum target, GLdouble s ) {
  void (*func_glMultiTexCoord1d)( GLenum, GLdouble ) = extension_funcs[458];
  TRACE("(%d, %f)\n", target, s );
  ENTER_GL();
  func_glMultiTexCoord1d( target, s );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1dARB( GLenum target, GLdouble s ) {
  void (*func_glMultiTexCoord1dARB)( GLenum, GLdouble ) = extension_funcs[459];
  TRACE("(%d, %f)\n", target, s );
  ENTER_GL();
  func_glMultiTexCoord1dARB( target, s );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1dSGIS( GLenum target, GLdouble s ) {
  void (*func_glMultiTexCoord1dSGIS)( GLenum, GLdouble ) = extension_funcs[460];
  TRACE("(%d, %f)\n", target, s );
  ENTER_GL();
  func_glMultiTexCoord1dSGIS( target, s );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1dv( GLenum target, GLdouble* v ) {
  void (*func_glMultiTexCoord1dv)( GLenum, GLdouble* ) = extension_funcs[461];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord1dv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1dvARB( GLenum target, GLdouble* v ) {
  void (*func_glMultiTexCoord1dvARB)( GLenum, GLdouble* ) = extension_funcs[462];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord1dvARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1dvSGIS( GLenum target, GLdouble * v ) {
  void (*func_glMultiTexCoord1dvSGIS)( GLenum, GLdouble * ) = extension_funcs[463];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord1dvSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1f( GLenum target, GLfloat s ) {
  void (*func_glMultiTexCoord1f)( GLenum, GLfloat ) = extension_funcs[464];
  TRACE("(%d, %f)\n", target, s );
  ENTER_GL();
  func_glMultiTexCoord1f( target, s );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1fARB( GLenum target, GLfloat s ) {
  void (*func_glMultiTexCoord1fARB)( GLenum, GLfloat ) = extension_funcs[465];
  TRACE("(%d, %f)\n", target, s );
  ENTER_GL();
  func_glMultiTexCoord1fARB( target, s );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1fSGIS( GLenum target, GLfloat s ) {
  void (*func_glMultiTexCoord1fSGIS)( GLenum, GLfloat ) = extension_funcs[466];
  TRACE("(%d, %f)\n", target, s );
  ENTER_GL();
  func_glMultiTexCoord1fSGIS( target, s );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1fv( GLenum target, GLfloat* v ) {
  void (*func_glMultiTexCoord1fv)( GLenum, GLfloat* ) = extension_funcs[467];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord1fv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1fvARB( GLenum target, GLfloat* v ) {
  void (*func_glMultiTexCoord1fvARB)( GLenum, GLfloat* ) = extension_funcs[468];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord1fvARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1fvSGIS( GLenum target, const GLfloat * v ) {
  void (*func_glMultiTexCoord1fvSGIS)( GLenum, const GLfloat * ) = extension_funcs[469];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord1fvSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1hNV( GLenum target, unsigned short s ) {
  void (*func_glMultiTexCoord1hNV)( GLenum, unsigned short ) = extension_funcs[470];
  TRACE("(%d, %d)\n", target, s );
  ENTER_GL();
  func_glMultiTexCoord1hNV( target, s );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1hvNV( GLenum target, unsigned short* v ) {
  void (*func_glMultiTexCoord1hvNV)( GLenum, unsigned short* ) = extension_funcs[471];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord1hvNV( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1i( GLenum target, GLint s ) {
  void (*func_glMultiTexCoord1i)( GLenum, GLint ) = extension_funcs[472];
  TRACE("(%d, %d)\n", target, s );
  ENTER_GL();
  func_glMultiTexCoord1i( target, s );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1iARB( GLenum target, GLint s ) {
  void (*func_glMultiTexCoord1iARB)( GLenum, GLint ) = extension_funcs[473];
  TRACE("(%d, %d)\n", target, s );
  ENTER_GL();
  func_glMultiTexCoord1iARB( target, s );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1iSGIS( GLenum target, GLint s ) {
  void (*func_glMultiTexCoord1iSGIS)( GLenum, GLint ) = extension_funcs[474];
  TRACE("(%d, %d)\n", target, s );
  ENTER_GL();
  func_glMultiTexCoord1iSGIS( target, s );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1iv( GLenum target, GLint* v ) {
  void (*func_glMultiTexCoord1iv)( GLenum, GLint* ) = extension_funcs[475];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord1iv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1ivARB( GLenum target, GLint* v ) {
  void (*func_glMultiTexCoord1ivARB)( GLenum, GLint* ) = extension_funcs[476];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord1ivARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1ivSGIS( GLenum target, GLint * v ) {
  void (*func_glMultiTexCoord1ivSGIS)( GLenum, GLint * ) = extension_funcs[477];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord1ivSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1s( GLenum target, GLshort s ) {
  void (*func_glMultiTexCoord1s)( GLenum, GLshort ) = extension_funcs[478];
  TRACE("(%d, %d)\n", target, s );
  ENTER_GL();
  func_glMultiTexCoord1s( target, s );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1sARB( GLenum target, GLshort s ) {
  void (*func_glMultiTexCoord1sARB)( GLenum, GLshort ) = extension_funcs[479];
  TRACE("(%d, %d)\n", target, s );
  ENTER_GL();
  func_glMultiTexCoord1sARB( target, s );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1sSGIS( GLenum target, GLshort s ) {
  void (*func_glMultiTexCoord1sSGIS)( GLenum, GLshort ) = extension_funcs[480];
  TRACE("(%d, %d)\n", target, s );
  ENTER_GL();
  func_glMultiTexCoord1sSGIS( target, s );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1sv( GLenum target, GLshort* v ) {
  void (*func_glMultiTexCoord1sv)( GLenum, GLshort* ) = extension_funcs[481];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord1sv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1svARB( GLenum target, GLshort* v ) {
  void (*func_glMultiTexCoord1svARB)( GLenum, GLshort* ) = extension_funcs[482];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord1svARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1svSGIS( GLenum target, GLshort * v ) {
  void (*func_glMultiTexCoord1svSGIS)( GLenum, GLshort * ) = extension_funcs[483];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord1svSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2d( GLenum target, GLdouble s, GLdouble t ) {
  void (*func_glMultiTexCoord2d)( GLenum, GLdouble, GLdouble ) = extension_funcs[484];
  TRACE("(%d, %f, %f)\n", target, s, t );
  ENTER_GL();
  func_glMultiTexCoord2d( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2dARB( GLenum target, GLdouble s, GLdouble t ) {
  void (*func_glMultiTexCoord2dARB)( GLenum, GLdouble, GLdouble ) = extension_funcs[485];
  TRACE("(%d, %f, %f)\n", target, s, t );
  ENTER_GL();
  func_glMultiTexCoord2dARB( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2dSGIS( GLenum target, GLdouble s, GLdouble t ) {
  void (*func_glMultiTexCoord2dSGIS)( GLenum, GLdouble, GLdouble ) = extension_funcs[486];
  TRACE("(%d, %f, %f)\n", target, s, t );
  ENTER_GL();
  func_glMultiTexCoord2dSGIS( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2dv( GLenum target, GLdouble* v ) {
  void (*func_glMultiTexCoord2dv)( GLenum, GLdouble* ) = extension_funcs[487];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord2dv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2dvARB( GLenum target, GLdouble* v ) {
  void (*func_glMultiTexCoord2dvARB)( GLenum, GLdouble* ) = extension_funcs[488];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord2dvARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2dvSGIS( GLenum target, GLdouble * v ) {
  void (*func_glMultiTexCoord2dvSGIS)( GLenum, GLdouble * ) = extension_funcs[489];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord2dvSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2f( GLenum target, GLfloat s, GLfloat t ) {
  void (*func_glMultiTexCoord2f)( GLenum, GLfloat, GLfloat ) = extension_funcs[490];
  TRACE("(%d, %f, %f)\n", target, s, t );
  ENTER_GL();
  func_glMultiTexCoord2f( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2fARB( GLenum target, GLfloat s, GLfloat t ) {
  void (*func_glMultiTexCoord2fARB)( GLenum, GLfloat, GLfloat ) = extension_funcs[491];
  TRACE("(%d, %f, %f)\n", target, s, t );
  ENTER_GL();
  func_glMultiTexCoord2fARB( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2fSGIS( GLenum target, GLfloat s, GLfloat t ) {
  void (*func_glMultiTexCoord2fSGIS)( GLenum, GLfloat, GLfloat ) = extension_funcs[492];
  TRACE("(%d, %f, %f)\n", target, s, t );
  ENTER_GL();
  func_glMultiTexCoord2fSGIS( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2fv( GLenum target, GLfloat* v ) {
  void (*func_glMultiTexCoord2fv)( GLenum, GLfloat* ) = extension_funcs[493];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord2fv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2fvARB( GLenum target, GLfloat* v ) {
  void (*func_glMultiTexCoord2fvARB)( GLenum, GLfloat* ) = extension_funcs[494];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord2fvARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2fvSGIS( GLenum target, GLfloat * v ) {
  void (*func_glMultiTexCoord2fvSGIS)( GLenum, GLfloat * ) = extension_funcs[495];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord2fvSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2hNV( GLenum target, unsigned short s, unsigned short t ) {
  void (*func_glMultiTexCoord2hNV)( GLenum, unsigned short, unsigned short ) = extension_funcs[496];
  TRACE("(%d, %d, %d)\n", target, s, t );
  ENTER_GL();
  func_glMultiTexCoord2hNV( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2hvNV( GLenum target, unsigned short* v ) {
  void (*func_glMultiTexCoord2hvNV)( GLenum, unsigned short* ) = extension_funcs[497];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord2hvNV( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2i( GLenum target, GLint s, GLint t ) {
  void (*func_glMultiTexCoord2i)( GLenum, GLint, GLint ) = extension_funcs[498];
  TRACE("(%d, %d, %d)\n", target, s, t );
  ENTER_GL();
  func_glMultiTexCoord2i( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2iARB( GLenum target, GLint s, GLint t ) {
  void (*func_glMultiTexCoord2iARB)( GLenum, GLint, GLint ) = extension_funcs[499];
  TRACE("(%d, %d, %d)\n", target, s, t );
  ENTER_GL();
  func_glMultiTexCoord2iARB( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2iSGIS( GLenum target, GLint s, GLint t ) {
  void (*func_glMultiTexCoord2iSGIS)( GLenum, GLint, GLint ) = extension_funcs[500];
  TRACE("(%d, %d, %d)\n", target, s, t );
  ENTER_GL();
  func_glMultiTexCoord2iSGIS( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2iv( GLenum target, GLint* v ) {
  void (*func_glMultiTexCoord2iv)( GLenum, GLint* ) = extension_funcs[501];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord2iv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2ivARB( GLenum target, GLint* v ) {
  void (*func_glMultiTexCoord2ivARB)( GLenum, GLint* ) = extension_funcs[502];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord2ivARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2ivSGIS( GLenum target, GLint * v ) {
  void (*func_glMultiTexCoord2ivSGIS)( GLenum, GLint * ) = extension_funcs[503];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord2ivSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2s( GLenum target, GLshort s, GLshort t ) {
  void (*func_glMultiTexCoord2s)( GLenum, GLshort, GLshort ) = extension_funcs[504];
  TRACE("(%d, %d, %d)\n", target, s, t );
  ENTER_GL();
  func_glMultiTexCoord2s( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2sARB( GLenum target, GLshort s, GLshort t ) {
  void (*func_glMultiTexCoord2sARB)( GLenum, GLshort, GLshort ) = extension_funcs[505];
  TRACE("(%d, %d, %d)\n", target, s, t );
  ENTER_GL();
  func_glMultiTexCoord2sARB( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2sSGIS( GLenum target, GLshort s, GLshort t ) {
  void (*func_glMultiTexCoord2sSGIS)( GLenum, GLshort, GLshort ) = extension_funcs[506];
  TRACE("(%d, %d, %d)\n", target, s, t );
  ENTER_GL();
  func_glMultiTexCoord2sSGIS( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2sv( GLenum target, GLshort* v ) {
  void (*func_glMultiTexCoord2sv)( GLenum, GLshort* ) = extension_funcs[507];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord2sv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2svARB( GLenum target, GLshort* v ) {
  void (*func_glMultiTexCoord2svARB)( GLenum, GLshort* ) = extension_funcs[508];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord2svARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2svSGIS( GLenum target, GLshort * v ) {
  void (*func_glMultiTexCoord2svSGIS)( GLenum, GLshort * ) = extension_funcs[509];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord2svSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3d( GLenum target, GLdouble s, GLdouble t, GLdouble r ) {
  void (*func_glMultiTexCoord3d)( GLenum, GLdouble, GLdouble, GLdouble ) = extension_funcs[510];
  TRACE("(%d, %f, %f, %f)\n", target, s, t, r );
  ENTER_GL();
  func_glMultiTexCoord3d( target, s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3dARB( GLenum target, GLdouble s, GLdouble t, GLdouble r ) {
  void (*func_glMultiTexCoord3dARB)( GLenum, GLdouble, GLdouble, GLdouble ) = extension_funcs[511];
  TRACE("(%d, %f, %f, %f)\n", target, s, t, r );
  ENTER_GL();
  func_glMultiTexCoord3dARB( target, s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3dSGIS( GLenum target, GLdouble s, GLdouble t, GLdouble r ) {
  void (*func_glMultiTexCoord3dSGIS)( GLenum, GLdouble, GLdouble, GLdouble ) = extension_funcs[512];
  TRACE("(%d, %f, %f, %f)\n", target, s, t, r );
  ENTER_GL();
  func_glMultiTexCoord3dSGIS( target, s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3dv( GLenum target, GLdouble* v ) {
  void (*func_glMultiTexCoord3dv)( GLenum, GLdouble* ) = extension_funcs[513];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord3dv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3dvARB( GLenum target, GLdouble* v ) {
  void (*func_glMultiTexCoord3dvARB)( GLenum, GLdouble* ) = extension_funcs[514];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord3dvARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3dvSGIS( GLenum target, GLdouble * v ) {
  void (*func_glMultiTexCoord3dvSGIS)( GLenum, GLdouble * ) = extension_funcs[515];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord3dvSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3f( GLenum target, GLfloat s, GLfloat t, GLfloat r ) {
  void (*func_glMultiTexCoord3f)( GLenum, GLfloat, GLfloat, GLfloat ) = extension_funcs[516];
  TRACE("(%d, %f, %f, %f)\n", target, s, t, r );
  ENTER_GL();
  func_glMultiTexCoord3f( target, s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3fARB( GLenum target, GLfloat s, GLfloat t, GLfloat r ) {
  void (*func_glMultiTexCoord3fARB)( GLenum, GLfloat, GLfloat, GLfloat ) = extension_funcs[517];
  TRACE("(%d, %f, %f, %f)\n", target, s, t, r );
  ENTER_GL();
  func_glMultiTexCoord3fARB( target, s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3fSGIS( GLenum target, GLfloat s, GLfloat t, GLfloat r ) {
  void (*func_glMultiTexCoord3fSGIS)( GLenum, GLfloat, GLfloat, GLfloat ) = extension_funcs[518];
  TRACE("(%d, %f, %f, %f)\n", target, s, t, r );
  ENTER_GL();
  func_glMultiTexCoord3fSGIS( target, s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3fv( GLenum target, GLfloat* v ) {
  void (*func_glMultiTexCoord3fv)( GLenum, GLfloat* ) = extension_funcs[519];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord3fv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3fvARB( GLenum target, GLfloat* v ) {
  void (*func_glMultiTexCoord3fvARB)( GLenum, GLfloat* ) = extension_funcs[520];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord3fvARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3fvSGIS( GLenum target, GLfloat * v ) {
  void (*func_glMultiTexCoord3fvSGIS)( GLenum, GLfloat * ) = extension_funcs[521];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord3fvSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3hNV( GLenum target, unsigned short s, unsigned short t, unsigned short r ) {
  void (*func_glMultiTexCoord3hNV)( GLenum, unsigned short, unsigned short, unsigned short ) = extension_funcs[522];
  TRACE("(%d, %d, %d, %d)\n", target, s, t, r );
  ENTER_GL();
  func_glMultiTexCoord3hNV( target, s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3hvNV( GLenum target, unsigned short* v ) {
  void (*func_glMultiTexCoord3hvNV)( GLenum, unsigned short* ) = extension_funcs[523];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord3hvNV( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3i( GLenum target, GLint s, GLint t, GLint r ) {
  void (*func_glMultiTexCoord3i)( GLenum, GLint, GLint, GLint ) = extension_funcs[524];
  TRACE("(%d, %d, %d, %d)\n", target, s, t, r );
  ENTER_GL();
  func_glMultiTexCoord3i( target, s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3iARB( GLenum target, GLint s, GLint t, GLint r ) {
  void (*func_glMultiTexCoord3iARB)( GLenum, GLint, GLint, GLint ) = extension_funcs[525];
  TRACE("(%d, %d, %d, %d)\n", target, s, t, r );
  ENTER_GL();
  func_glMultiTexCoord3iARB( target, s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3iSGIS( GLenum target, GLint s, GLint t, GLint r ) {
  void (*func_glMultiTexCoord3iSGIS)( GLenum, GLint, GLint, GLint ) = extension_funcs[526];
  TRACE("(%d, %d, %d, %d)\n", target, s, t, r );
  ENTER_GL();
  func_glMultiTexCoord3iSGIS( target, s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3iv( GLenum target, GLint* v ) {
  void (*func_glMultiTexCoord3iv)( GLenum, GLint* ) = extension_funcs[527];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord3iv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3ivARB( GLenum target, GLint* v ) {
  void (*func_glMultiTexCoord3ivARB)( GLenum, GLint* ) = extension_funcs[528];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord3ivARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3ivSGIS( GLenum target, GLint * v ) {
  void (*func_glMultiTexCoord3ivSGIS)( GLenum, GLint * ) = extension_funcs[529];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord3ivSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3s( GLenum target, GLshort s, GLshort t, GLshort r ) {
  void (*func_glMultiTexCoord3s)( GLenum, GLshort, GLshort, GLshort ) = extension_funcs[530];
  TRACE("(%d, %d, %d, %d)\n", target, s, t, r );
  ENTER_GL();
  func_glMultiTexCoord3s( target, s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3sARB( GLenum target, GLshort s, GLshort t, GLshort r ) {
  void (*func_glMultiTexCoord3sARB)( GLenum, GLshort, GLshort, GLshort ) = extension_funcs[531];
  TRACE("(%d, %d, %d, %d)\n", target, s, t, r );
  ENTER_GL();
  func_glMultiTexCoord3sARB( target, s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3sSGIS( GLenum target, GLshort s, GLshort t, GLshort r ) {
  void (*func_glMultiTexCoord3sSGIS)( GLenum, GLshort, GLshort, GLshort ) = extension_funcs[532];
  TRACE("(%d, %d, %d, %d)\n", target, s, t, r );
  ENTER_GL();
  func_glMultiTexCoord3sSGIS( target, s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3sv( GLenum target, GLshort* v ) {
  void (*func_glMultiTexCoord3sv)( GLenum, GLshort* ) = extension_funcs[533];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord3sv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3svARB( GLenum target, GLshort* v ) {
  void (*func_glMultiTexCoord3svARB)( GLenum, GLshort* ) = extension_funcs[534];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord3svARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3svSGIS( GLenum target, GLshort * v ) {
  void (*func_glMultiTexCoord3svSGIS)( GLenum, GLshort * ) = extension_funcs[535];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord3svSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4d( GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q ) {
  void (*func_glMultiTexCoord4d)( GLenum, GLdouble, GLdouble, GLdouble, GLdouble ) = extension_funcs[536];
  TRACE("(%d, %f, %f, %f, %f)\n", target, s, t, r, q );
  ENTER_GL();
  func_glMultiTexCoord4d( target, s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4dARB( GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q ) {
  void (*func_glMultiTexCoord4dARB)( GLenum, GLdouble, GLdouble, GLdouble, GLdouble ) = extension_funcs[537];
  TRACE("(%d, %f, %f, %f, %f)\n", target, s, t, r, q );
  ENTER_GL();
  func_glMultiTexCoord4dARB( target, s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4dSGIS( GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q ) {
  void (*func_glMultiTexCoord4dSGIS)( GLenum, GLdouble, GLdouble, GLdouble, GLdouble ) = extension_funcs[538];
  TRACE("(%d, %f, %f, %f, %f)\n", target, s, t, r, q );
  ENTER_GL();
  func_glMultiTexCoord4dSGIS( target, s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4dv( GLenum target, GLdouble* v ) {
  void (*func_glMultiTexCoord4dv)( GLenum, GLdouble* ) = extension_funcs[539];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord4dv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4dvARB( GLenum target, GLdouble* v ) {
  void (*func_glMultiTexCoord4dvARB)( GLenum, GLdouble* ) = extension_funcs[540];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord4dvARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4dvSGIS( GLenum target, GLdouble * v ) {
  void (*func_glMultiTexCoord4dvSGIS)( GLenum, GLdouble * ) = extension_funcs[541];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord4dvSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4f( GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q ) {
  void (*func_glMultiTexCoord4f)( GLenum, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[542];
  TRACE("(%d, %f, %f, %f, %f)\n", target, s, t, r, q );
  ENTER_GL();
  func_glMultiTexCoord4f( target, s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4fARB( GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q ) {
  void (*func_glMultiTexCoord4fARB)( GLenum, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[543];
  TRACE("(%d, %f, %f, %f, %f)\n", target, s, t, r, q );
  ENTER_GL();
  func_glMultiTexCoord4fARB( target, s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4fSGIS( GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q ) {
  void (*func_glMultiTexCoord4fSGIS)( GLenum, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[544];
  TRACE("(%d, %f, %f, %f, %f)\n", target, s, t, r, q );
  ENTER_GL();
  func_glMultiTexCoord4fSGIS( target, s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4fv( GLenum target, GLfloat* v ) {
  void (*func_glMultiTexCoord4fv)( GLenum, GLfloat* ) = extension_funcs[545];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord4fv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4fvARB( GLenum target, GLfloat* v ) {
  void (*func_glMultiTexCoord4fvARB)( GLenum, GLfloat* ) = extension_funcs[546];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord4fvARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4fvSGIS( GLenum target, GLfloat * v ) {
  void (*func_glMultiTexCoord4fvSGIS)( GLenum, GLfloat * ) = extension_funcs[547];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord4fvSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4hNV( GLenum target, unsigned short s, unsigned short t, unsigned short r, unsigned short q ) {
  void (*func_glMultiTexCoord4hNV)( GLenum, unsigned short, unsigned short, unsigned short, unsigned short ) = extension_funcs[548];
  TRACE("(%d, %d, %d, %d, %d)\n", target, s, t, r, q );
  ENTER_GL();
  func_glMultiTexCoord4hNV( target, s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4hvNV( GLenum target, unsigned short* v ) {
  void (*func_glMultiTexCoord4hvNV)( GLenum, unsigned short* ) = extension_funcs[549];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord4hvNV( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4i( GLenum target, GLint s, GLint t, GLint r, GLint q ) {
  void (*func_glMultiTexCoord4i)( GLenum, GLint, GLint, GLint, GLint ) = extension_funcs[550];
  TRACE("(%d, %d, %d, %d, %d)\n", target, s, t, r, q );
  ENTER_GL();
  func_glMultiTexCoord4i( target, s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4iARB( GLenum target, GLint s, GLint t, GLint r, GLint q ) {
  void (*func_glMultiTexCoord4iARB)( GLenum, GLint, GLint, GLint, GLint ) = extension_funcs[551];
  TRACE("(%d, %d, %d, %d, %d)\n", target, s, t, r, q );
  ENTER_GL();
  func_glMultiTexCoord4iARB( target, s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4iSGIS( GLenum target, GLint s, GLint t, GLint r, GLint q ) {
  void (*func_glMultiTexCoord4iSGIS)( GLenum, GLint, GLint, GLint, GLint ) = extension_funcs[552];
  TRACE("(%d, %d, %d, %d, %d)\n", target, s, t, r, q );
  ENTER_GL();
  func_glMultiTexCoord4iSGIS( target, s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4iv( GLenum target, GLint* v ) {
  void (*func_glMultiTexCoord4iv)( GLenum, GLint* ) = extension_funcs[553];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord4iv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4ivARB( GLenum target, GLint* v ) {
  void (*func_glMultiTexCoord4ivARB)( GLenum, GLint* ) = extension_funcs[554];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord4ivARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4ivSGIS( GLenum target, GLint * v ) {
  void (*func_glMultiTexCoord4ivSGIS)( GLenum, GLint * ) = extension_funcs[555];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord4ivSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4s( GLenum target, GLshort s, GLshort t, GLshort r, GLshort q ) {
  void (*func_glMultiTexCoord4s)( GLenum, GLshort, GLshort, GLshort, GLshort ) = extension_funcs[556];
  TRACE("(%d, %d, %d, %d, %d)\n", target, s, t, r, q );
  ENTER_GL();
  func_glMultiTexCoord4s( target, s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4sARB( GLenum target, GLshort s, GLshort t, GLshort r, GLshort q ) {
  void (*func_glMultiTexCoord4sARB)( GLenum, GLshort, GLshort, GLshort, GLshort ) = extension_funcs[557];
  TRACE("(%d, %d, %d, %d, %d)\n", target, s, t, r, q );
  ENTER_GL();
  func_glMultiTexCoord4sARB( target, s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4sSGIS( GLenum target, GLshort s, GLshort t, GLshort r, GLshort q ) {
  void (*func_glMultiTexCoord4sSGIS)( GLenum, GLshort, GLshort, GLshort, GLshort ) = extension_funcs[558];
  TRACE("(%d, %d, %d, %d, %d)\n", target, s, t, r, q );
  ENTER_GL();
  func_glMultiTexCoord4sSGIS( target, s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4sv( GLenum target, GLshort* v ) {
  void (*func_glMultiTexCoord4sv)( GLenum, GLshort* ) = extension_funcs[559];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord4sv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4svARB( GLenum target, GLshort* v ) {
  void (*func_glMultiTexCoord4svARB)( GLenum, GLshort* ) = extension_funcs[560];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord4svARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4svSGIS( GLenum target, GLshort * v ) {
  void (*func_glMultiTexCoord4svSGIS)( GLenum, GLshort * ) = extension_funcs[561];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord4svSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoordPointerSGIS( GLenum target, GLint size, GLenum type, GLsizei stride, GLvoid * pointer ) {
  void (*func_glMultiTexCoordPointerSGIS)( GLenum, GLint, GLenum, GLsizei, GLvoid * ) = extension_funcs[562];
  TRACE("(%d, %d, %d, %d, %p)\n", target, size, type, stride, pointer );
  ENTER_GL();
  func_glMultiTexCoordPointerSGIS( target, size, type, stride, pointer );
  LEAVE_GL();
}

static GLuint WINAPI wine_glNewBufferRegion( GLenum type ) {
  GLuint ret_value;
  GLuint (*func_glNewBufferRegion)( GLenum ) = extension_funcs[563];
  TRACE("(%d)\n", type );
  ENTER_GL();
  ret_value = func_glNewBufferRegion( type );
  LEAVE_GL();
  return ret_value;
}

static GLuint WINAPI wine_glNewObjectBufferATI( GLsizei size, GLvoid* pointer, GLenum usage ) {
  GLuint ret_value;
  GLuint (*func_glNewObjectBufferATI)( GLsizei, GLvoid*, GLenum ) = extension_funcs[564];
  TRACE("(%d, %p, %d)\n", size, pointer, usage );
  ENTER_GL();
  ret_value = func_glNewObjectBufferATI( size, pointer, usage );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glNormal3fVertex3fSUN( GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glNormal3fVertex3fSUN)( GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[565];
  TRACE("(%f, %f, %f, %f, %f, %f)\n", nx, ny, nz, x, y, z );
  ENTER_GL();
  func_glNormal3fVertex3fSUN( nx, ny, nz, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glNormal3fVertex3fvSUN( GLfloat* n, GLfloat* v ) {
  void (*func_glNormal3fVertex3fvSUN)( GLfloat*, GLfloat* ) = extension_funcs[566];
  TRACE("(%p, %p)\n", n, v );
  ENTER_GL();
  func_glNormal3fVertex3fvSUN( n, v );
  LEAVE_GL();
}

static void WINAPI wine_glNormal3hNV( unsigned short nx, unsigned short ny, unsigned short nz ) {
  void (*func_glNormal3hNV)( unsigned short, unsigned short, unsigned short ) = extension_funcs[567];
  TRACE("(%d, %d, %d)\n", nx, ny, nz );
  ENTER_GL();
  func_glNormal3hNV( nx, ny, nz );
  LEAVE_GL();
}

static void WINAPI wine_glNormal3hvNV( unsigned short* v ) {
  void (*func_glNormal3hvNV)( unsigned short* ) = extension_funcs[568];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glNormal3hvNV( v );
  LEAVE_GL();
}

static void WINAPI wine_glNormalPointerEXT( GLenum type, GLsizei stride, GLsizei count, GLvoid* pointer ) {
  void (*func_glNormalPointerEXT)( GLenum, GLsizei, GLsizei, GLvoid* ) = extension_funcs[569];
  TRACE("(%d, %d, %d, %p)\n", type, stride, count, pointer );
  ENTER_GL();
  func_glNormalPointerEXT( type, stride, count, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glNormalPointerListIBM( GLenum type, GLint stride, GLvoid** pointer, GLint ptrstride ) {
  void (*func_glNormalPointerListIBM)( GLenum, GLint, GLvoid**, GLint ) = extension_funcs[570];
  TRACE("(%d, %d, %p, %d)\n", type, stride, pointer, ptrstride );
  ENTER_GL();
  func_glNormalPointerListIBM( type, stride, pointer, ptrstride );
  LEAVE_GL();
}

static void WINAPI wine_glNormalPointervINTEL( GLenum type, GLvoid** pointer ) {
  void (*func_glNormalPointervINTEL)( GLenum, GLvoid** ) = extension_funcs[571];
  TRACE("(%d, %p)\n", type, pointer );
  ENTER_GL();
  func_glNormalPointervINTEL( type, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glNormalStream3bATI( GLenum stream, GLbyte nx, GLbyte ny, GLbyte nz ) {
  void (*func_glNormalStream3bATI)( GLenum, GLbyte, GLbyte, GLbyte ) = extension_funcs[572];
  TRACE("(%d, %d, %d, %d)\n", stream, nx, ny, nz );
  ENTER_GL();
  func_glNormalStream3bATI( stream, nx, ny, nz );
  LEAVE_GL();
}

static void WINAPI wine_glNormalStream3bvATI( GLenum stream, GLbyte* coords ) {
  void (*func_glNormalStream3bvATI)( GLenum, GLbyte* ) = extension_funcs[573];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glNormalStream3bvATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glNormalStream3dATI( GLenum stream, GLdouble nx, GLdouble ny, GLdouble nz ) {
  void (*func_glNormalStream3dATI)( GLenum, GLdouble, GLdouble, GLdouble ) = extension_funcs[574];
  TRACE("(%d, %f, %f, %f)\n", stream, nx, ny, nz );
  ENTER_GL();
  func_glNormalStream3dATI( stream, nx, ny, nz );
  LEAVE_GL();
}

static void WINAPI wine_glNormalStream3dvATI( GLenum stream, GLdouble* coords ) {
  void (*func_glNormalStream3dvATI)( GLenum, GLdouble* ) = extension_funcs[575];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glNormalStream3dvATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glNormalStream3fATI( GLenum stream, GLfloat nx, GLfloat ny, GLfloat nz ) {
  void (*func_glNormalStream3fATI)( GLenum, GLfloat, GLfloat, GLfloat ) = extension_funcs[576];
  TRACE("(%d, %f, %f, %f)\n", stream, nx, ny, nz );
  ENTER_GL();
  func_glNormalStream3fATI( stream, nx, ny, nz );
  LEAVE_GL();
}

static void WINAPI wine_glNormalStream3fvATI( GLenum stream, GLfloat* coords ) {
  void (*func_glNormalStream3fvATI)( GLenum, GLfloat* ) = extension_funcs[577];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glNormalStream3fvATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glNormalStream3iATI( GLenum stream, GLint nx, GLint ny, GLint nz ) {
  void (*func_glNormalStream3iATI)( GLenum, GLint, GLint, GLint ) = extension_funcs[578];
  TRACE("(%d, %d, %d, %d)\n", stream, nx, ny, nz );
  ENTER_GL();
  func_glNormalStream3iATI( stream, nx, ny, nz );
  LEAVE_GL();
}

static void WINAPI wine_glNormalStream3ivATI( GLenum stream, GLint* coords ) {
  void (*func_glNormalStream3ivATI)( GLenum, GLint* ) = extension_funcs[579];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glNormalStream3ivATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glNormalStream3sATI( GLenum stream, GLshort nx, GLshort ny, GLshort nz ) {
  void (*func_glNormalStream3sATI)( GLenum, GLshort, GLshort, GLshort ) = extension_funcs[580];
  TRACE("(%d, %d, %d, %d)\n", stream, nx, ny, nz );
  ENTER_GL();
  func_glNormalStream3sATI( stream, nx, ny, nz );
  LEAVE_GL();
}

static void WINAPI wine_glNormalStream3svATI( GLenum stream, GLshort* coords ) {
  void (*func_glNormalStream3svATI)( GLenum, GLshort* ) = extension_funcs[581];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glNormalStream3svATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glPNTrianglesfATI( GLenum pname, GLfloat param ) {
  void (*func_glPNTrianglesfATI)( GLenum, GLfloat ) = extension_funcs[582];
  TRACE("(%d, %f)\n", pname, param );
  ENTER_GL();
  func_glPNTrianglesfATI( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glPNTrianglesiATI( GLenum pname, GLint param ) {
  void (*func_glPNTrianglesiATI)( GLenum, GLint ) = extension_funcs[583];
  TRACE("(%d, %d)\n", pname, param );
  ENTER_GL();
  func_glPNTrianglesiATI( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glPassTexCoordATI( GLuint dst, GLuint coord, GLenum swizzle ) {
  void (*func_glPassTexCoordATI)( GLuint, GLuint, GLenum ) = extension_funcs[584];
  TRACE("(%d, %d, %d)\n", dst, coord, swizzle );
  ENTER_GL();
  func_glPassTexCoordATI( dst, coord, swizzle );
  LEAVE_GL();
}

static void WINAPI wine_glPixelDataRangeNV( GLenum target, GLsizei length, GLvoid* pointer ) {
  void (*func_glPixelDataRangeNV)( GLenum, GLsizei, GLvoid* ) = extension_funcs[585];
  TRACE("(%d, %d, %p)\n", target, length, pointer );
  ENTER_GL();
  func_glPixelDataRangeNV( target, length, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glPixelTexGenParameterfSGIS( GLenum pname, GLfloat param ) {
  void (*func_glPixelTexGenParameterfSGIS)( GLenum, GLfloat ) = extension_funcs[586];
  TRACE("(%d, %f)\n", pname, param );
  ENTER_GL();
  func_glPixelTexGenParameterfSGIS( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glPixelTexGenParameterfvSGIS( GLenum pname, GLfloat* params ) {
  void (*func_glPixelTexGenParameterfvSGIS)( GLenum, GLfloat* ) = extension_funcs[587];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glPixelTexGenParameterfvSGIS( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glPixelTexGenParameteriSGIS( GLenum pname, GLint param ) {
  void (*func_glPixelTexGenParameteriSGIS)( GLenum, GLint ) = extension_funcs[588];
  TRACE("(%d, %d)\n", pname, param );
  ENTER_GL();
  func_glPixelTexGenParameteriSGIS( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glPixelTexGenParameterivSGIS( GLenum pname, GLint* params ) {
  void (*func_glPixelTexGenParameterivSGIS)( GLenum, GLint* ) = extension_funcs[589];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glPixelTexGenParameterivSGIS( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glPixelTexGenSGIX( GLenum mode ) {
  void (*func_glPixelTexGenSGIX)( GLenum ) = extension_funcs[590];
  TRACE("(%d)\n", mode );
  ENTER_GL();
  func_glPixelTexGenSGIX( mode );
  LEAVE_GL();
}

static void WINAPI wine_glPixelTransformParameterfEXT( GLenum target, GLenum pname, GLfloat param ) {
  void (*func_glPixelTransformParameterfEXT)( GLenum, GLenum, GLfloat ) = extension_funcs[591];
  TRACE("(%d, %d, %f)\n", target, pname, param );
  ENTER_GL();
  func_glPixelTransformParameterfEXT( target, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glPixelTransformParameterfvEXT( GLenum target, GLenum pname, GLfloat* params ) {
  void (*func_glPixelTransformParameterfvEXT)( GLenum, GLenum, GLfloat* ) = extension_funcs[592];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glPixelTransformParameterfvEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glPixelTransformParameteriEXT( GLenum target, GLenum pname, GLint param ) {
  void (*func_glPixelTransformParameteriEXT)( GLenum, GLenum, GLint ) = extension_funcs[593];
  TRACE("(%d, %d, %d)\n", target, pname, param );
  ENTER_GL();
  func_glPixelTransformParameteriEXT( target, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glPixelTransformParameterivEXT( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glPixelTransformParameterivEXT)( GLenum, GLenum, GLint* ) = extension_funcs[594];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glPixelTransformParameterivEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glPointParameterf( GLenum pname, GLfloat param ) {
  void (*func_glPointParameterf)( GLenum, GLfloat ) = extension_funcs[595];
  TRACE("(%d, %f)\n", pname, param );
  ENTER_GL();
  func_glPointParameterf( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glPointParameterfARB( GLenum pname, GLfloat param ) {
  void (*func_glPointParameterfARB)( GLenum, GLfloat ) = extension_funcs[596];
  TRACE("(%d, %f)\n", pname, param );
  ENTER_GL();
  func_glPointParameterfARB( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glPointParameterfEXT( GLenum pname, GLfloat param ) {
  void (*func_glPointParameterfEXT)( GLenum, GLfloat ) = extension_funcs[597];
  TRACE("(%d, %f)\n", pname, param );
  ENTER_GL();
  func_glPointParameterfEXT( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glPointParameterfSGIS( GLenum pname, GLfloat param ) {
  void (*func_glPointParameterfSGIS)( GLenum, GLfloat ) = extension_funcs[598];
  TRACE("(%d, %f)\n", pname, param );
  ENTER_GL();
  func_glPointParameterfSGIS( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glPointParameterfv( GLenum pname, GLfloat* params ) {
  void (*func_glPointParameterfv)( GLenum, GLfloat* ) = extension_funcs[599];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glPointParameterfv( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glPointParameterfvARB( GLenum pname, GLfloat* params ) {
  void (*func_glPointParameterfvARB)( GLenum, GLfloat* ) = extension_funcs[600];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glPointParameterfvARB( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glPointParameterfvEXT( GLenum pname, GLfloat* params ) {
  void (*func_glPointParameterfvEXT)( GLenum, GLfloat* ) = extension_funcs[601];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glPointParameterfvEXT( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glPointParameterfvSGIS( GLenum pname, GLfloat* params ) {
  void (*func_glPointParameterfvSGIS)( GLenum, GLfloat* ) = extension_funcs[602];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glPointParameterfvSGIS( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glPointParameteri( GLenum pname, GLint param ) {
  void (*func_glPointParameteri)( GLenum, GLint ) = extension_funcs[603];
  TRACE("(%d, %d)\n", pname, param );
  ENTER_GL();
  func_glPointParameteri( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glPointParameteriNV( GLenum pname, GLint param ) {
  void (*func_glPointParameteriNV)( GLenum, GLint ) = extension_funcs[604];
  TRACE("(%d, %d)\n", pname, param );
  ENTER_GL();
  func_glPointParameteriNV( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glPointParameteriv( GLenum pname, GLint* params ) {
  void (*func_glPointParameteriv)( GLenum, GLint* ) = extension_funcs[605];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glPointParameteriv( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glPointParameterivNV( GLenum pname, GLint* params ) {
  void (*func_glPointParameterivNV)( GLenum, GLint* ) = extension_funcs[606];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glPointParameterivNV( pname, params );
  LEAVE_GL();
}

static GLint WINAPI wine_glPollAsyncSGIX( GLuint* markerp ) {
  GLint ret_value;
  GLint (*func_glPollAsyncSGIX)( GLuint* ) = extension_funcs[607];
  TRACE("(%p)\n", markerp );
  ENTER_GL();
  ret_value = func_glPollAsyncSGIX( markerp );
  LEAVE_GL();
  return ret_value;
}

static GLint WINAPI wine_glPollInstrumentsSGIX( GLint* marker_p ) {
  GLint ret_value;
  GLint (*func_glPollInstrumentsSGIX)( GLint* ) = extension_funcs[608];
  TRACE("(%p)\n", marker_p );
  ENTER_GL();
  ret_value = func_glPollInstrumentsSGIX( marker_p );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glPolygonOffsetEXT( GLfloat factor, GLfloat bias ) {
  void (*func_glPolygonOffsetEXT)( GLfloat, GLfloat ) = extension_funcs[609];
  TRACE("(%f, %f)\n", factor, bias );
  ENTER_GL();
  func_glPolygonOffsetEXT( factor, bias );
  LEAVE_GL();
}

static void WINAPI wine_glPrimitiveRestartIndexNV( GLuint index ) {
  void (*func_glPrimitiveRestartIndexNV)( GLuint ) = extension_funcs[610];
  TRACE("(%d)\n", index );
  ENTER_GL();
  func_glPrimitiveRestartIndexNV( index );
  LEAVE_GL();
}

static void WINAPI wine_glPrimitiveRestartNV( void ) {
  void (*func_glPrimitiveRestartNV)( void ) = extension_funcs[611];
  TRACE("()\n");
  ENTER_GL();
  func_glPrimitiveRestartNV( );
  LEAVE_GL();
}

static void WINAPI wine_glPrioritizeTexturesEXT( GLsizei n, GLuint* textures, GLclampf* priorities ) {
  void (*func_glPrioritizeTexturesEXT)( GLsizei, GLuint*, GLclampf* ) = extension_funcs[612];
  TRACE("(%d, %p, %p)\n", n, textures, priorities );
  ENTER_GL();
  func_glPrioritizeTexturesEXT( n, textures, priorities );
  LEAVE_GL();
}

static void WINAPI wine_glProgramEnvParameter4dARB( GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  void (*func_glProgramEnvParameter4dARB)( GLenum, GLuint, GLdouble, GLdouble, GLdouble, GLdouble ) = extension_funcs[613];
  TRACE("(%d, %d, %f, %f, %f, %f)\n", target, index, x, y, z, w );
  ENTER_GL();
  func_glProgramEnvParameter4dARB( target, index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glProgramEnvParameter4dvARB( GLenum target, GLuint index, GLdouble* params ) {
  void (*func_glProgramEnvParameter4dvARB)( GLenum, GLuint, GLdouble* ) = extension_funcs[614];
  TRACE("(%d, %d, %p)\n", target, index, params );
  ENTER_GL();
  func_glProgramEnvParameter4dvARB( target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glProgramEnvParameter4fARB( GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  void (*func_glProgramEnvParameter4fARB)( GLenum, GLuint, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[615];
  TRACE("(%d, %d, %f, %f, %f, %f)\n", target, index, x, y, z, w );
  ENTER_GL();
  func_glProgramEnvParameter4fARB( target, index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glProgramEnvParameter4fvARB( GLenum target, GLuint index, GLfloat* params ) {
  void (*func_glProgramEnvParameter4fvARB)( GLenum, GLuint, GLfloat* ) = extension_funcs[616];
  TRACE("(%d, %d, %p)\n", target, index, params );
  ENTER_GL();
  func_glProgramEnvParameter4fvARB( target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glProgramLocalParameter4dARB( GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  void (*func_glProgramLocalParameter4dARB)( GLenum, GLuint, GLdouble, GLdouble, GLdouble, GLdouble ) = extension_funcs[617];
  TRACE("(%d, %d, %f, %f, %f, %f)\n", target, index, x, y, z, w );
  ENTER_GL();
  func_glProgramLocalParameter4dARB( target, index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glProgramLocalParameter4dvARB( GLenum target, GLuint index, GLdouble* params ) {
  void (*func_glProgramLocalParameter4dvARB)( GLenum, GLuint, GLdouble* ) = extension_funcs[618];
  TRACE("(%d, %d, %p)\n", target, index, params );
  ENTER_GL();
  func_glProgramLocalParameter4dvARB( target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glProgramLocalParameter4fARB( GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  void (*func_glProgramLocalParameter4fARB)( GLenum, GLuint, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[619];
  TRACE("(%d, %d, %f, %f, %f, %f)\n", target, index, x, y, z, w );
  ENTER_GL();
  func_glProgramLocalParameter4fARB( target, index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glProgramLocalParameter4fvARB( GLenum target, GLuint index, GLfloat* params ) {
  void (*func_glProgramLocalParameter4fvARB)( GLenum, GLuint, GLfloat* ) = extension_funcs[620];
  TRACE("(%d, %d, %p)\n", target, index, params );
  ENTER_GL();
  func_glProgramLocalParameter4fvARB( target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glProgramNamedParameter4dNV( GLuint id, GLsizei len, GLubyte* name, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  void (*func_glProgramNamedParameter4dNV)( GLuint, GLsizei, GLubyte*, GLdouble, GLdouble, GLdouble, GLdouble ) = extension_funcs[621];
  TRACE("(%d, %d, %p, %f, %f, %f, %f)\n", id, len, name, x, y, z, w );
  ENTER_GL();
  func_glProgramNamedParameter4dNV( id, len, name, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glProgramNamedParameter4dvNV( GLuint id, GLsizei len, GLubyte* name, GLdouble* v ) {
  void (*func_glProgramNamedParameter4dvNV)( GLuint, GLsizei, GLubyte*, GLdouble* ) = extension_funcs[622];
  TRACE("(%d, %d, %p, %p)\n", id, len, name, v );
  ENTER_GL();
  func_glProgramNamedParameter4dvNV( id, len, name, v );
  LEAVE_GL();
}

static void WINAPI wine_glProgramNamedParameter4fNV( GLuint id, GLsizei len, GLubyte* name, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  void (*func_glProgramNamedParameter4fNV)( GLuint, GLsizei, GLubyte*, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[623];
  TRACE("(%d, %d, %p, %f, %f, %f, %f)\n", id, len, name, x, y, z, w );
  ENTER_GL();
  func_glProgramNamedParameter4fNV( id, len, name, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glProgramNamedParameter4fvNV( GLuint id, GLsizei len, GLubyte* name, GLfloat* v ) {
  void (*func_glProgramNamedParameter4fvNV)( GLuint, GLsizei, GLubyte*, GLfloat* ) = extension_funcs[624];
  TRACE("(%d, %d, %p, %p)\n", id, len, name, v );
  ENTER_GL();
  func_glProgramNamedParameter4fvNV( id, len, name, v );
  LEAVE_GL();
}

static void WINAPI wine_glProgramParameter4dNV( GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  void (*func_glProgramParameter4dNV)( GLenum, GLuint, GLdouble, GLdouble, GLdouble, GLdouble ) = extension_funcs[625];
  TRACE("(%d, %d, %f, %f, %f, %f)\n", target, index, x, y, z, w );
  ENTER_GL();
  func_glProgramParameter4dNV( target, index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glProgramParameter4dvNV( GLenum target, GLuint index, GLdouble* v ) {
  void (*func_glProgramParameter4dvNV)( GLenum, GLuint, GLdouble* ) = extension_funcs[626];
  TRACE("(%d, %d, %p)\n", target, index, v );
  ENTER_GL();
  func_glProgramParameter4dvNV( target, index, v );
  LEAVE_GL();
}

static void WINAPI wine_glProgramParameter4fNV( GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  void (*func_glProgramParameter4fNV)( GLenum, GLuint, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[627];
  TRACE("(%d, %d, %f, %f, %f, %f)\n", target, index, x, y, z, w );
  ENTER_GL();
  func_glProgramParameter4fNV( target, index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glProgramParameter4fvNV( GLenum target, GLuint index, GLfloat* v ) {
  void (*func_glProgramParameter4fvNV)( GLenum, GLuint, GLfloat* ) = extension_funcs[628];
  TRACE("(%d, %d, %p)\n", target, index, v );
  ENTER_GL();
  func_glProgramParameter4fvNV( target, index, v );
  LEAVE_GL();
}

static void WINAPI wine_glProgramParameters4dvNV( GLenum target, GLuint index, GLuint count, GLdouble* v ) {
  void (*func_glProgramParameters4dvNV)( GLenum, GLuint, GLuint, GLdouble* ) = extension_funcs[629];
  TRACE("(%d, %d, %d, %p)\n", target, index, count, v );
  ENTER_GL();
  func_glProgramParameters4dvNV( target, index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glProgramParameters4fvNV( GLenum target, GLuint index, GLuint count, GLfloat* v ) {
  void (*func_glProgramParameters4fvNV)( GLenum, GLuint, GLuint, GLfloat* ) = extension_funcs[630];
  TRACE("(%d, %d, %d, %p)\n", target, index, count, v );
  ENTER_GL();
  func_glProgramParameters4fvNV( target, index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glProgramStringARB( GLenum target, GLenum format, GLsizei len, GLvoid* string ) {
  void (*func_glProgramStringARB)( GLenum, GLenum, GLsizei, GLvoid* ) = extension_funcs[631];
  TRACE("(%d, %d, %d, %p)\n", target, format, len, string );
  ENTER_GL();
  func_glProgramStringARB( target, format, len, string );
  LEAVE_GL();
}

static void WINAPI wine_glReadBufferRegion( GLenum region, GLint x, GLint y, GLsizei width, GLsizei height ) {
  void (*func_glReadBufferRegion)( GLenum, GLint, GLint, GLsizei, GLsizei ) = extension_funcs[632];
  TRACE("(%d, %d, %d, %d, %d)\n", region, x, y, width, height );
  ENTER_GL();
  func_glReadBufferRegion( region, x, y, width, height );
  LEAVE_GL();
}

static void WINAPI wine_glReadInstrumentsSGIX( GLint marker ) {
  void (*func_glReadInstrumentsSGIX)( GLint ) = extension_funcs[633];
  TRACE("(%d)\n", marker );
  ENTER_GL();
  func_glReadInstrumentsSGIX( marker );
  LEAVE_GL();
}

static void WINAPI wine_glReferencePlaneSGIX( GLdouble* equation ) {
  void (*func_glReferencePlaneSGIX)( GLdouble* ) = extension_funcs[634];
  TRACE("(%p)\n", equation );
  ENTER_GL();
  func_glReferencePlaneSGIX( equation );
  LEAVE_GL();
}

static void WINAPI wine_glRenderbufferStorageEXT( GLenum target, GLenum internalformat, GLsizei width, GLsizei height ) {
  void (*func_glRenderbufferStorageEXT)( GLenum, GLenum, GLsizei, GLsizei ) = extension_funcs[635];
  TRACE("(%d, %d, %d, %d)\n", target, internalformat, width, height );
  ENTER_GL();
  func_glRenderbufferStorageEXT( target, internalformat, width, height );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodePointerSUN( GLenum type, GLsizei stride, GLvoid** pointer ) {
  void (*func_glReplacementCodePointerSUN)( GLenum, GLsizei, GLvoid** ) = extension_funcs[636];
  TRACE("(%d, %d, %p)\n", type, stride, pointer );
  ENTER_GL();
  func_glReplacementCodePointerSUN( type, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeubSUN( GLubyte code ) {
  void (*func_glReplacementCodeubSUN)( GLubyte ) = extension_funcs[637];
  TRACE("(%d)\n", code );
  ENTER_GL();
  func_glReplacementCodeubSUN( code );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeubvSUN( GLubyte* code ) {
  void (*func_glReplacementCodeubvSUN)( GLubyte* ) = extension_funcs[638];
  TRACE("(%p)\n", code );
  ENTER_GL();
  func_glReplacementCodeubvSUN( code );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiColor3fVertex3fSUN( GLuint rc, GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glReplacementCodeuiColor3fVertex3fSUN)( GLuint, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[639];
  TRACE("(%d, %f, %f, %f, %f, %f, %f)\n", rc, r, g, b, x, y, z );
  ENTER_GL();
  func_glReplacementCodeuiColor3fVertex3fSUN( rc, r, g, b, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiColor3fVertex3fvSUN( GLuint* rc, GLfloat* c, GLfloat* v ) {
  void (*func_glReplacementCodeuiColor3fVertex3fvSUN)( GLuint*, GLfloat*, GLfloat* ) = extension_funcs[640];
  TRACE("(%p, %p, %p)\n", rc, c, v );
  ENTER_GL();
  func_glReplacementCodeuiColor3fVertex3fvSUN( rc, c, v );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiColor4fNormal3fVertex3fSUN( GLuint rc, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glReplacementCodeuiColor4fNormal3fVertex3fSUN)( GLuint, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[641];
  TRACE("(%d, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f)\n", rc, r, g, b, a, nx, ny, nz, x, y, z );
  ENTER_GL();
  func_glReplacementCodeuiColor4fNormal3fVertex3fSUN( rc, r, g, b, a, nx, ny, nz, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiColor4fNormal3fVertex3fvSUN( GLuint* rc, GLfloat* c, GLfloat* n, GLfloat* v ) {
  void (*func_glReplacementCodeuiColor4fNormal3fVertex3fvSUN)( GLuint*, GLfloat*, GLfloat*, GLfloat* ) = extension_funcs[642];
  TRACE("(%p, %p, %p, %p)\n", rc, c, n, v );
  ENTER_GL();
  func_glReplacementCodeuiColor4fNormal3fVertex3fvSUN( rc, c, n, v );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiColor4ubVertex3fSUN( GLuint rc, GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glReplacementCodeuiColor4ubVertex3fSUN)( GLuint, GLubyte, GLubyte, GLubyte, GLubyte, GLfloat, GLfloat, GLfloat ) = extension_funcs[643];
  TRACE("(%d, %d, %d, %d, %d, %f, %f, %f)\n", rc, r, g, b, a, x, y, z );
  ENTER_GL();
  func_glReplacementCodeuiColor4ubVertex3fSUN( rc, r, g, b, a, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiColor4ubVertex3fvSUN( GLuint* rc, GLubyte* c, GLfloat* v ) {
  void (*func_glReplacementCodeuiColor4ubVertex3fvSUN)( GLuint*, GLubyte*, GLfloat* ) = extension_funcs[644];
  TRACE("(%p, %p, %p)\n", rc, c, v );
  ENTER_GL();
  func_glReplacementCodeuiColor4ubVertex3fvSUN( rc, c, v );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiNormal3fVertex3fSUN( GLuint rc, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glReplacementCodeuiNormal3fVertex3fSUN)( GLuint, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[645];
  TRACE("(%d, %f, %f, %f, %f, %f, %f)\n", rc, nx, ny, nz, x, y, z );
  ENTER_GL();
  func_glReplacementCodeuiNormal3fVertex3fSUN( rc, nx, ny, nz, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiNormal3fVertex3fvSUN( GLuint* rc, GLfloat* n, GLfloat* v ) {
  void (*func_glReplacementCodeuiNormal3fVertex3fvSUN)( GLuint*, GLfloat*, GLfloat* ) = extension_funcs[646];
  TRACE("(%p, %p, %p)\n", rc, n, v );
  ENTER_GL();
  func_glReplacementCodeuiNormal3fVertex3fvSUN( rc, n, v );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiSUN( GLuint code ) {
  void (*func_glReplacementCodeuiSUN)( GLuint ) = extension_funcs[647];
  TRACE("(%d)\n", code );
  ENTER_GL();
  func_glReplacementCodeuiSUN( code );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN( GLuint rc, GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN)( GLuint, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[648];
  TRACE("(%d, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f)\n", rc, s, t, r, g, b, a, nx, ny, nz, x, y, z );
  ENTER_GL();
  func_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN( rc, s, t, r, g, b, a, nx, ny, nz, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN( GLuint* rc, GLfloat* tc, GLfloat* c, GLfloat* n, GLfloat* v ) {
  void (*func_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN)( GLuint*, GLfloat*, GLfloat*, GLfloat*, GLfloat* ) = extension_funcs[649];
  TRACE("(%p, %p, %p, %p, %p)\n", rc, tc, c, n, v );
  ENTER_GL();
  func_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN( rc, tc, c, n, v );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN( GLuint rc, GLfloat s, GLfloat t, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN)( GLuint, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[650];
  TRACE("(%d, %f, %f, %f, %f, %f, %f, %f, %f)\n", rc, s, t, nx, ny, nz, x, y, z );
  ENTER_GL();
  func_glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN( rc, s, t, nx, ny, nz, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN( GLuint* rc, GLfloat* tc, GLfloat* n, GLfloat* v ) {
  void (*func_glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN)( GLuint*, GLfloat*, GLfloat*, GLfloat* ) = extension_funcs[651];
  TRACE("(%p, %p, %p, %p)\n", rc, tc, n, v );
  ENTER_GL();
  func_glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN( rc, tc, n, v );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiTexCoord2fVertex3fSUN( GLuint rc, GLfloat s, GLfloat t, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glReplacementCodeuiTexCoord2fVertex3fSUN)( GLuint, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[652];
  TRACE("(%d, %f, %f, %f, %f, %f)\n", rc, s, t, x, y, z );
  ENTER_GL();
  func_glReplacementCodeuiTexCoord2fVertex3fSUN( rc, s, t, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiTexCoord2fVertex3fvSUN( GLuint* rc, GLfloat* tc, GLfloat* v ) {
  void (*func_glReplacementCodeuiTexCoord2fVertex3fvSUN)( GLuint*, GLfloat*, GLfloat* ) = extension_funcs[653];
  TRACE("(%p, %p, %p)\n", rc, tc, v );
  ENTER_GL();
  func_glReplacementCodeuiTexCoord2fVertex3fvSUN( rc, tc, v );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiVertex3fSUN( GLuint rc, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glReplacementCodeuiVertex3fSUN)( GLuint, GLfloat, GLfloat, GLfloat ) = extension_funcs[654];
  TRACE("(%d, %f, %f, %f)\n", rc, x, y, z );
  ENTER_GL();
  func_glReplacementCodeuiVertex3fSUN( rc, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiVertex3fvSUN( GLuint* rc, GLfloat* v ) {
  void (*func_glReplacementCodeuiVertex3fvSUN)( GLuint*, GLfloat* ) = extension_funcs[655];
  TRACE("(%p, %p)\n", rc, v );
  ENTER_GL();
  func_glReplacementCodeuiVertex3fvSUN( rc, v );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuivSUN( GLuint* code ) {
  void (*func_glReplacementCodeuivSUN)( GLuint* ) = extension_funcs[656];
  TRACE("(%p)\n", code );
  ENTER_GL();
  func_glReplacementCodeuivSUN( code );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeusSUN( GLushort code ) {
  void (*func_glReplacementCodeusSUN)( GLushort ) = extension_funcs[657];
  TRACE("(%d)\n", code );
  ENTER_GL();
  func_glReplacementCodeusSUN( code );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeusvSUN( GLushort* code ) {
  void (*func_glReplacementCodeusvSUN)( GLushort* ) = extension_funcs[658];
  TRACE("(%p)\n", code );
  ENTER_GL();
  func_glReplacementCodeusvSUN( code );
  LEAVE_GL();
}

static void WINAPI wine_glRequestResidentProgramsNV( GLsizei n, GLuint* programs ) {
  void (*func_glRequestResidentProgramsNV)( GLsizei, GLuint* ) = extension_funcs[659];
  TRACE("(%d, %p)\n", n, programs );
  ENTER_GL();
  func_glRequestResidentProgramsNV( n, programs );
  LEAVE_GL();
}

static void WINAPI wine_glResetHistogramEXT( GLenum target ) {
  void (*func_glResetHistogramEXT)( GLenum ) = extension_funcs[660];
  TRACE("(%d)\n", target );
  ENTER_GL();
  func_glResetHistogramEXT( target );
  LEAVE_GL();
}

static void WINAPI wine_glResetMinmaxEXT( GLenum target ) {
  void (*func_glResetMinmaxEXT)( GLenum ) = extension_funcs[661];
  TRACE("(%d)\n", target );
  ENTER_GL();
  func_glResetMinmaxEXT( target );
  LEAVE_GL();
}

static void WINAPI wine_glResizeBuffersMESA( void ) {
  void (*func_glResizeBuffersMESA)( void ) = extension_funcs[662];
  TRACE("()\n");
  ENTER_GL();
  func_glResizeBuffersMESA( );
  LEAVE_GL();
}

static void WINAPI wine_glSampleCoverage( GLclampf value, GLboolean invert ) {
  void (*func_glSampleCoverage)( GLclampf, GLboolean ) = extension_funcs[663];
  TRACE("(%f, %d)\n", value, invert );
  ENTER_GL();
  func_glSampleCoverage( value, invert );
  LEAVE_GL();
}

static void WINAPI wine_glSampleCoverageARB( GLclampf value, GLboolean invert ) {
  void (*func_glSampleCoverageARB)( GLclampf, GLboolean ) = extension_funcs[664];
  TRACE("(%f, %d)\n", value, invert );
  ENTER_GL();
  func_glSampleCoverageARB( value, invert );
  LEAVE_GL();
}

static void WINAPI wine_glSampleMapATI( GLuint dst, GLuint interp, GLenum swizzle ) {
  void (*func_glSampleMapATI)( GLuint, GLuint, GLenum ) = extension_funcs[665];
  TRACE("(%d, %d, %d)\n", dst, interp, swizzle );
  ENTER_GL();
  func_glSampleMapATI( dst, interp, swizzle );
  LEAVE_GL();
}

static void WINAPI wine_glSampleMaskEXT( GLclampf value, GLboolean invert ) {
  void (*func_glSampleMaskEXT)( GLclampf, GLboolean ) = extension_funcs[666];
  TRACE("(%f, %d)\n", value, invert );
  ENTER_GL();
  func_glSampleMaskEXT( value, invert );
  LEAVE_GL();
}

static void WINAPI wine_glSampleMaskSGIS( GLclampf value, GLboolean invert ) {
  void (*func_glSampleMaskSGIS)( GLclampf, GLboolean ) = extension_funcs[667];
  TRACE("(%f, %d)\n", value, invert );
  ENTER_GL();
  func_glSampleMaskSGIS( value, invert );
  LEAVE_GL();
}

static void WINAPI wine_glSamplePatternEXT( GLenum pattern ) {
  void (*func_glSamplePatternEXT)( GLenum ) = extension_funcs[668];
  TRACE("(%d)\n", pattern );
  ENTER_GL();
  func_glSamplePatternEXT( pattern );
  LEAVE_GL();
}

static void WINAPI wine_glSamplePatternSGIS( GLenum pattern ) {
  void (*func_glSamplePatternSGIS)( GLenum ) = extension_funcs[669];
  TRACE("(%d)\n", pattern );
  ENTER_GL();
  func_glSamplePatternSGIS( pattern );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3b( GLbyte red, GLbyte green, GLbyte blue ) {
  void (*func_glSecondaryColor3b)( GLbyte, GLbyte, GLbyte ) = extension_funcs[670];
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3b( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3bEXT( GLbyte red, GLbyte green, GLbyte blue ) {
  void (*func_glSecondaryColor3bEXT)( GLbyte, GLbyte, GLbyte ) = extension_funcs[671];
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3bEXT( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3bv( GLbyte* v ) {
  void (*func_glSecondaryColor3bv)( GLbyte* ) = extension_funcs[672];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3bv( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3bvEXT( GLbyte* v ) {
  void (*func_glSecondaryColor3bvEXT)( GLbyte* ) = extension_funcs[673];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3bvEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3d( GLdouble red, GLdouble green, GLdouble blue ) {
  void (*func_glSecondaryColor3d)( GLdouble, GLdouble, GLdouble ) = extension_funcs[674];
  TRACE("(%f, %f, %f)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3d( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3dEXT( GLdouble red, GLdouble green, GLdouble blue ) {
  void (*func_glSecondaryColor3dEXT)( GLdouble, GLdouble, GLdouble ) = extension_funcs[675];
  TRACE("(%f, %f, %f)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3dEXT( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3dv( GLdouble* v ) {
  void (*func_glSecondaryColor3dv)( GLdouble* ) = extension_funcs[676];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3dv( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3dvEXT( GLdouble* v ) {
  void (*func_glSecondaryColor3dvEXT)( GLdouble* ) = extension_funcs[677];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3dvEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3f( GLfloat red, GLfloat green, GLfloat blue ) {
  void (*func_glSecondaryColor3f)( GLfloat, GLfloat, GLfloat ) = extension_funcs[678];
  TRACE("(%f, %f, %f)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3f( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3fEXT( GLfloat red, GLfloat green, GLfloat blue ) {
  void (*func_glSecondaryColor3fEXT)( GLfloat, GLfloat, GLfloat ) = extension_funcs[679];
  TRACE("(%f, %f, %f)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3fEXT( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3fv( GLfloat* v ) {
  void (*func_glSecondaryColor3fv)( GLfloat* ) = extension_funcs[680];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3fv( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3fvEXT( GLfloat* v ) {
  void (*func_glSecondaryColor3fvEXT)( GLfloat* ) = extension_funcs[681];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3fvEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3hNV( unsigned short red, unsigned short green, unsigned short blue ) {
  void (*func_glSecondaryColor3hNV)( unsigned short, unsigned short, unsigned short ) = extension_funcs[682];
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3hNV( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3hvNV( unsigned short* v ) {
  void (*func_glSecondaryColor3hvNV)( unsigned short* ) = extension_funcs[683];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3hvNV( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3i( GLint red, GLint green, GLint blue ) {
  void (*func_glSecondaryColor3i)( GLint, GLint, GLint ) = extension_funcs[684];
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3i( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3iEXT( GLint red, GLint green, GLint blue ) {
  void (*func_glSecondaryColor3iEXT)( GLint, GLint, GLint ) = extension_funcs[685];
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3iEXT( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3iv( GLint* v ) {
  void (*func_glSecondaryColor3iv)( GLint* ) = extension_funcs[686];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3iv( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3ivEXT( GLint* v ) {
  void (*func_glSecondaryColor3ivEXT)( GLint* ) = extension_funcs[687];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3ivEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3s( GLshort red, GLshort green, GLshort blue ) {
  void (*func_glSecondaryColor3s)( GLshort, GLshort, GLshort ) = extension_funcs[688];
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3s( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3sEXT( GLshort red, GLshort green, GLshort blue ) {
  void (*func_glSecondaryColor3sEXT)( GLshort, GLshort, GLshort ) = extension_funcs[689];
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3sEXT( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3sv( GLshort* v ) {
  void (*func_glSecondaryColor3sv)( GLshort* ) = extension_funcs[690];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3sv( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3svEXT( GLshort* v ) {
  void (*func_glSecondaryColor3svEXT)( GLshort* ) = extension_funcs[691];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3svEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3ub( GLubyte red, GLubyte green, GLubyte blue ) {
  void (*func_glSecondaryColor3ub)( GLubyte, GLubyte, GLubyte ) = extension_funcs[692];
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3ub( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3ubEXT( GLubyte red, GLubyte green, GLubyte blue ) {
  void (*func_glSecondaryColor3ubEXT)( GLubyte, GLubyte, GLubyte ) = extension_funcs[693];
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3ubEXT( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3ubv( GLubyte* v ) {
  void (*func_glSecondaryColor3ubv)( GLubyte* ) = extension_funcs[694];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3ubv( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3ubvEXT( GLubyte* v ) {
  void (*func_glSecondaryColor3ubvEXT)( GLubyte* ) = extension_funcs[695];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3ubvEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3ui( GLuint red, GLuint green, GLuint blue ) {
  void (*func_glSecondaryColor3ui)( GLuint, GLuint, GLuint ) = extension_funcs[696];
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3ui( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3uiEXT( GLuint red, GLuint green, GLuint blue ) {
  void (*func_glSecondaryColor3uiEXT)( GLuint, GLuint, GLuint ) = extension_funcs[697];
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3uiEXT( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3uiv( GLuint* v ) {
  void (*func_glSecondaryColor3uiv)( GLuint* ) = extension_funcs[698];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3uiv( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3uivEXT( GLuint* v ) {
  void (*func_glSecondaryColor3uivEXT)( GLuint* ) = extension_funcs[699];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3uivEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3us( GLushort red, GLushort green, GLushort blue ) {
  void (*func_glSecondaryColor3us)( GLushort, GLushort, GLushort ) = extension_funcs[700];
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3us( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3usEXT( GLushort red, GLushort green, GLushort blue ) {
  void (*func_glSecondaryColor3usEXT)( GLushort, GLushort, GLushort ) = extension_funcs[701];
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3usEXT( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3usv( GLushort* v ) {
  void (*func_glSecondaryColor3usv)( GLushort* ) = extension_funcs[702];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3usv( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3usvEXT( GLushort* v ) {
  void (*func_glSecondaryColor3usvEXT)( GLushort* ) = extension_funcs[703];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3usvEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColorPointer( GLint size, GLenum type, GLsizei stride, GLvoid* pointer ) {
  void (*func_glSecondaryColorPointer)( GLint, GLenum, GLsizei, GLvoid* ) = extension_funcs[704];
  TRACE("(%d, %d, %d, %p)\n", size, type, stride, pointer );
  ENTER_GL();
  func_glSecondaryColorPointer( size, type, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColorPointerEXT( GLint size, GLenum type, GLsizei stride, GLvoid* pointer ) {
  void (*func_glSecondaryColorPointerEXT)( GLint, GLenum, GLsizei, GLvoid* ) = extension_funcs[705];
  TRACE("(%d, %d, %d, %p)\n", size, type, stride, pointer );
  ENTER_GL();
  func_glSecondaryColorPointerEXT( size, type, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColorPointerListIBM( GLint size, GLenum type, GLint stride, GLvoid** pointer, GLint ptrstride ) {
  void (*func_glSecondaryColorPointerListIBM)( GLint, GLenum, GLint, GLvoid**, GLint ) = extension_funcs[706];
  TRACE("(%d, %d, %d, %p, %d)\n", size, type, stride, pointer, ptrstride );
  ENTER_GL();
  func_glSecondaryColorPointerListIBM( size, type, stride, pointer, ptrstride );
  LEAVE_GL();
}

static void WINAPI wine_glSelectTextureCoordSetSGIS( GLenum target ) {
  void (*func_glSelectTextureCoordSetSGIS)( GLenum ) = extension_funcs[707];
  TRACE("(%d)\n", target );
  ENTER_GL();
  func_glSelectTextureCoordSetSGIS( target );
  LEAVE_GL();
}

static void WINAPI wine_glSelectTextureSGIS( GLenum target ) {
  void (*func_glSelectTextureSGIS)( GLenum ) = extension_funcs[708];
  TRACE("(%d)\n", target );
  ENTER_GL();
  func_glSelectTextureSGIS( target );
  LEAVE_GL();
}

static void WINAPI wine_glSeparableFilter2DEXT( GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* row, GLvoid* column ) {
  void (*func_glSeparableFilter2DEXT)( GLenum, GLenum, GLsizei, GLsizei, GLenum, GLenum, GLvoid*, GLvoid* ) = extension_funcs[709];
  TRACE("(%d, %d, %d, %d, %d, %d, %p, %p)\n", target, internalformat, width, height, format, type, row, column );
  ENTER_GL();
  func_glSeparableFilter2DEXT( target, internalformat, width, height, format, type, row, column );
  LEAVE_GL();
}

static void WINAPI wine_glSetFenceAPPLE( GLuint fence ) {
  void (*func_glSetFenceAPPLE)( GLuint ) = extension_funcs[710];
  TRACE("(%d)\n", fence );
  ENTER_GL();
  func_glSetFenceAPPLE( fence );
  LEAVE_GL();
}

static void WINAPI wine_glSetFenceNV( GLuint fence, GLenum condition ) {
  void (*func_glSetFenceNV)( GLuint, GLenum ) = extension_funcs[711];
  TRACE("(%d, %d)\n", fence, condition );
  ENTER_GL();
  func_glSetFenceNV( fence, condition );
  LEAVE_GL();
}

static void WINAPI wine_glSetFragmentShaderConstantATI( GLuint dst, GLfloat* value ) {
  void (*func_glSetFragmentShaderConstantATI)( GLuint, GLfloat* ) = extension_funcs[712];
  TRACE("(%d, %p)\n", dst, value );
  ENTER_GL();
  func_glSetFragmentShaderConstantATI( dst, value );
  LEAVE_GL();
}

static void WINAPI wine_glSetInvariantEXT( GLuint id, GLenum type, GLvoid* addr ) {
  void (*func_glSetInvariantEXT)( GLuint, GLenum, GLvoid* ) = extension_funcs[713];
  TRACE("(%d, %d, %p)\n", id, type, addr );
  ENTER_GL();
  func_glSetInvariantEXT( id, type, addr );
  LEAVE_GL();
}

static void WINAPI wine_glSetLocalConstantEXT( GLuint id, GLenum type, GLvoid* addr ) {
  void (*func_glSetLocalConstantEXT)( GLuint, GLenum, GLvoid* ) = extension_funcs[714];
  TRACE("(%d, %d, %p)\n", id, type, addr );
  ENTER_GL();
  func_glSetLocalConstantEXT( id, type, addr );
  LEAVE_GL();
}

static void WINAPI wine_glShaderOp1EXT( GLenum op, GLuint res, GLuint arg1 ) {
  void (*func_glShaderOp1EXT)( GLenum, GLuint, GLuint ) = extension_funcs[715];
  TRACE("(%d, %d, %d)\n", op, res, arg1 );
  ENTER_GL();
  func_glShaderOp1EXT( op, res, arg1 );
  LEAVE_GL();
}

static void WINAPI wine_glShaderOp2EXT( GLenum op, GLuint res, GLuint arg1, GLuint arg2 ) {
  void (*func_glShaderOp2EXT)( GLenum, GLuint, GLuint, GLuint ) = extension_funcs[716];
  TRACE("(%d, %d, %d, %d)\n", op, res, arg1, arg2 );
  ENTER_GL();
  func_glShaderOp2EXT( op, res, arg1, arg2 );
  LEAVE_GL();
}

static void WINAPI wine_glShaderOp3EXT( GLenum op, GLuint res, GLuint arg1, GLuint arg2, GLuint arg3 ) {
  void (*func_glShaderOp3EXT)( GLenum, GLuint, GLuint, GLuint, GLuint ) = extension_funcs[717];
  TRACE("(%d, %d, %d, %d, %d)\n", op, res, arg1, arg2, arg3 );
  ENTER_GL();
  func_glShaderOp3EXT( op, res, arg1, arg2, arg3 );
  LEAVE_GL();
}

static void WINAPI wine_glShaderSource( GLuint shader, GLsizei count, char** string, GLint* length ) {
  void (*func_glShaderSource)( GLuint, GLsizei, char**, GLint* ) = extension_funcs[718];
  TRACE("(%d, %d, %p, %p)\n", shader, count, string, length );
  ENTER_GL();
  func_glShaderSource( shader, count, string, length );
  LEAVE_GL();
}

static void WINAPI wine_glShaderSourceARB( unsigned int shaderObj, GLsizei count, char** string, GLint* length ) {
  void (*func_glShaderSourceARB)( unsigned int, GLsizei, char**, GLint* ) = extension_funcs[719];
  TRACE("(%d, %d, %p, %p)\n", shaderObj, count, string, length );
  ENTER_GL();
  func_glShaderSourceARB( shaderObj, count, string, length );
  LEAVE_GL();
}

static void WINAPI wine_glSharpenTexFuncSGIS( GLenum target, GLsizei n, GLfloat* points ) {
  void (*func_glSharpenTexFuncSGIS)( GLenum, GLsizei, GLfloat* ) = extension_funcs[720];
  TRACE("(%d, %d, %p)\n", target, n, points );
  ENTER_GL();
  func_glSharpenTexFuncSGIS( target, n, points );
  LEAVE_GL();
}

static void WINAPI wine_glSpriteParameterfSGIX( GLenum pname, GLfloat param ) {
  void (*func_glSpriteParameterfSGIX)( GLenum, GLfloat ) = extension_funcs[721];
  TRACE("(%d, %f)\n", pname, param );
  ENTER_GL();
  func_glSpriteParameterfSGIX( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glSpriteParameterfvSGIX( GLenum pname, GLfloat* params ) {
  void (*func_glSpriteParameterfvSGIX)( GLenum, GLfloat* ) = extension_funcs[722];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glSpriteParameterfvSGIX( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glSpriteParameteriSGIX( GLenum pname, GLint param ) {
  void (*func_glSpriteParameteriSGIX)( GLenum, GLint ) = extension_funcs[723];
  TRACE("(%d, %d)\n", pname, param );
  ENTER_GL();
  func_glSpriteParameteriSGIX( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glSpriteParameterivSGIX( GLenum pname, GLint* params ) {
  void (*func_glSpriteParameterivSGIX)( GLenum, GLint* ) = extension_funcs[724];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glSpriteParameterivSGIX( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glStartInstrumentsSGIX( void ) {
  void (*func_glStartInstrumentsSGIX)( void ) = extension_funcs[725];
  TRACE("()\n");
  ENTER_GL();
  func_glStartInstrumentsSGIX( );
  LEAVE_GL();
}

static void WINAPI wine_glStencilFuncSeparate( GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask ) {
  void (*func_glStencilFuncSeparate)( GLenum, GLenum, GLint, GLuint ) = extension_funcs[726];
  TRACE("(%d, %d, %d, %d)\n", frontfunc, backfunc, ref, mask );
  ENTER_GL();
  func_glStencilFuncSeparate( frontfunc, backfunc, ref, mask );
  LEAVE_GL();
}

static void WINAPI wine_glStencilFuncSeparateATI( GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask ) {
  void (*func_glStencilFuncSeparateATI)( GLenum, GLenum, GLint, GLuint ) = extension_funcs[727];
  TRACE("(%d, %d, %d, %d)\n", frontfunc, backfunc, ref, mask );
  ENTER_GL();
  func_glStencilFuncSeparateATI( frontfunc, backfunc, ref, mask );
  LEAVE_GL();
}

static void WINAPI wine_glStencilMaskSeparate( GLenum face, GLuint mask ) {
  void (*func_glStencilMaskSeparate)( GLenum, GLuint ) = extension_funcs[728];
  TRACE("(%d, %d)\n", face, mask );
  ENTER_GL();
  func_glStencilMaskSeparate( face, mask );
  LEAVE_GL();
}

static void WINAPI wine_glStencilOpSeparate( GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass ) {
  void (*func_glStencilOpSeparate)( GLenum, GLenum, GLenum, GLenum ) = extension_funcs[729];
  TRACE("(%d, %d, %d, %d)\n", face, sfail, dpfail, dppass );
  ENTER_GL();
  func_glStencilOpSeparate( face, sfail, dpfail, dppass );
  LEAVE_GL();
}

static void WINAPI wine_glStencilOpSeparateATI( GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass ) {
  void (*func_glStencilOpSeparateATI)( GLenum, GLenum, GLenum, GLenum ) = extension_funcs[730];
  TRACE("(%d, %d, %d, %d)\n", face, sfail, dpfail, dppass );
  ENTER_GL();
  func_glStencilOpSeparateATI( face, sfail, dpfail, dppass );
  LEAVE_GL();
}

static void WINAPI wine_glStopInstrumentsSGIX( GLint marker ) {
  void (*func_glStopInstrumentsSGIX)( GLint ) = extension_funcs[731];
  TRACE("(%d)\n", marker );
  ENTER_GL();
  func_glStopInstrumentsSGIX( marker );
  LEAVE_GL();
}

static void WINAPI wine_glStringMarkerGREMEDY( GLsizei len, GLvoid* string ) {
  void (*func_glStringMarkerGREMEDY)( GLsizei, GLvoid* ) = extension_funcs[732];
  TRACE("(%d, %p)\n", len, string );
  ENTER_GL();
  func_glStringMarkerGREMEDY( len, string );
  LEAVE_GL();
}

static void WINAPI wine_glSwizzleEXT( GLuint res, GLuint in, GLenum outX, GLenum outY, GLenum outZ, GLenum outW ) {
  void (*func_glSwizzleEXT)( GLuint, GLuint, GLenum, GLenum, GLenum, GLenum ) = extension_funcs[733];
  TRACE("(%d, %d, %d, %d, %d, %d)\n", res, in, outX, outY, outZ, outW );
  ENTER_GL();
  func_glSwizzleEXT( res, in, outX, outY, outZ, outW );
  LEAVE_GL();
}

static void WINAPI wine_glTagSampleBufferSGIX( void ) {
  void (*func_glTagSampleBufferSGIX)( void ) = extension_funcs[734];
  TRACE("()\n");
  ENTER_GL();
  func_glTagSampleBufferSGIX( );
  LEAVE_GL();
}

static void WINAPI wine_glTangent3bEXT( GLbyte tx, GLbyte ty, GLbyte tz ) {
  void (*func_glTangent3bEXT)( GLbyte, GLbyte, GLbyte ) = extension_funcs[735];
  TRACE("(%d, %d, %d)\n", tx, ty, tz );
  ENTER_GL();
  func_glTangent3bEXT( tx, ty, tz );
  LEAVE_GL();
}

static void WINAPI wine_glTangent3bvEXT( GLbyte* v ) {
  void (*func_glTangent3bvEXT)( GLbyte* ) = extension_funcs[736];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glTangent3bvEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glTangent3dEXT( GLdouble tx, GLdouble ty, GLdouble tz ) {
  void (*func_glTangent3dEXT)( GLdouble, GLdouble, GLdouble ) = extension_funcs[737];
  TRACE("(%f, %f, %f)\n", tx, ty, tz );
  ENTER_GL();
  func_glTangent3dEXT( tx, ty, tz );
  LEAVE_GL();
}

static void WINAPI wine_glTangent3dvEXT( GLdouble* v ) {
  void (*func_glTangent3dvEXT)( GLdouble* ) = extension_funcs[738];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glTangent3dvEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glTangent3fEXT( GLfloat tx, GLfloat ty, GLfloat tz ) {
  void (*func_glTangent3fEXT)( GLfloat, GLfloat, GLfloat ) = extension_funcs[739];
  TRACE("(%f, %f, %f)\n", tx, ty, tz );
  ENTER_GL();
  func_glTangent3fEXT( tx, ty, tz );
  LEAVE_GL();
}

static void WINAPI wine_glTangent3fvEXT( GLfloat* v ) {
  void (*func_glTangent3fvEXT)( GLfloat* ) = extension_funcs[740];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glTangent3fvEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glTangent3iEXT( GLint tx, GLint ty, GLint tz ) {
  void (*func_glTangent3iEXT)( GLint, GLint, GLint ) = extension_funcs[741];
  TRACE("(%d, %d, %d)\n", tx, ty, tz );
  ENTER_GL();
  func_glTangent3iEXT( tx, ty, tz );
  LEAVE_GL();
}

static void WINAPI wine_glTangent3ivEXT( GLint* v ) {
  void (*func_glTangent3ivEXT)( GLint* ) = extension_funcs[742];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glTangent3ivEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glTangent3sEXT( GLshort tx, GLshort ty, GLshort tz ) {
  void (*func_glTangent3sEXT)( GLshort, GLshort, GLshort ) = extension_funcs[743];
  TRACE("(%d, %d, %d)\n", tx, ty, tz );
  ENTER_GL();
  func_glTangent3sEXT( tx, ty, tz );
  LEAVE_GL();
}

static void WINAPI wine_glTangent3svEXT( GLshort* v ) {
  void (*func_glTangent3svEXT)( GLshort* ) = extension_funcs[744];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glTangent3svEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glTangentPointerEXT( GLenum type, GLsizei stride, GLvoid* pointer ) {
  void (*func_glTangentPointerEXT)( GLenum, GLsizei, GLvoid* ) = extension_funcs[745];
  TRACE("(%d, %d, %p)\n", type, stride, pointer );
  ENTER_GL();
  func_glTangentPointerEXT( type, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glTbufferMask3DFX( GLuint mask ) {
  void (*func_glTbufferMask3DFX)( GLuint ) = extension_funcs[746];
  TRACE("(%d)\n", mask );
  ENTER_GL();
  func_glTbufferMask3DFX( mask );
  LEAVE_GL();
}

static GLboolean WINAPI wine_glTestFenceAPPLE( GLuint fence ) {
  GLboolean ret_value;
  GLboolean (*func_glTestFenceAPPLE)( GLuint ) = extension_funcs[747];
  TRACE("(%d)\n", fence );
  ENTER_GL();
  ret_value = func_glTestFenceAPPLE( fence );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glTestFenceNV( GLuint fence ) {
  GLboolean ret_value;
  GLboolean (*func_glTestFenceNV)( GLuint ) = extension_funcs[748];
  TRACE("(%d)\n", fence );
  ENTER_GL();
  ret_value = func_glTestFenceNV( fence );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glTestObjectAPPLE( GLenum object, GLuint name ) {
  GLboolean ret_value;
  GLboolean (*func_glTestObjectAPPLE)( GLenum, GLuint ) = extension_funcs[749];
  TRACE("(%d, %d)\n", object, name );
  ENTER_GL();
  ret_value = func_glTestObjectAPPLE( object, name );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glTexBumpParameterfvATI( GLenum pname, GLfloat* param ) {
  void (*func_glTexBumpParameterfvATI)( GLenum, GLfloat* ) = extension_funcs[750];
  TRACE("(%d, %p)\n", pname, param );
  ENTER_GL();
  func_glTexBumpParameterfvATI( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glTexBumpParameterivATI( GLenum pname, GLint* param ) {
  void (*func_glTexBumpParameterivATI)( GLenum, GLint* ) = extension_funcs[751];
  TRACE("(%d, %p)\n", pname, param );
  ENTER_GL();
  func_glTexBumpParameterivATI( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord1hNV( unsigned short s ) {
  void (*func_glTexCoord1hNV)( unsigned short ) = extension_funcs[752];
  TRACE("(%d)\n", s );
  ENTER_GL();
  func_glTexCoord1hNV( s );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord1hvNV( unsigned short* v ) {
  void (*func_glTexCoord1hvNV)( unsigned short* ) = extension_funcs[753];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glTexCoord1hvNV( v );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord2fColor3fVertex3fSUN( GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glTexCoord2fColor3fVertex3fSUN)( GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[754];
  TRACE("(%f, %f, %f, %f, %f, %f, %f, %f)\n", s, t, r, g, b, x, y, z );
  ENTER_GL();
  func_glTexCoord2fColor3fVertex3fSUN( s, t, r, g, b, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord2fColor3fVertex3fvSUN( GLfloat* tc, GLfloat* c, GLfloat* v ) {
  void (*func_glTexCoord2fColor3fVertex3fvSUN)( GLfloat*, GLfloat*, GLfloat* ) = extension_funcs[755];
  TRACE("(%p, %p, %p)\n", tc, c, v );
  ENTER_GL();
  func_glTexCoord2fColor3fVertex3fvSUN( tc, c, v );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord2fColor4fNormal3fVertex3fSUN( GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glTexCoord2fColor4fNormal3fVertex3fSUN)( GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[756];
  TRACE("(%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f)\n", s, t, r, g, b, a, nx, ny, nz, x, y, z );
  ENTER_GL();
  func_glTexCoord2fColor4fNormal3fVertex3fSUN( s, t, r, g, b, a, nx, ny, nz, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord2fColor4fNormal3fVertex3fvSUN( GLfloat* tc, GLfloat* c, GLfloat* n, GLfloat* v ) {
  void (*func_glTexCoord2fColor4fNormal3fVertex3fvSUN)( GLfloat*, GLfloat*, GLfloat*, GLfloat* ) = extension_funcs[757];
  TRACE("(%p, %p, %p, %p)\n", tc, c, n, v );
  ENTER_GL();
  func_glTexCoord2fColor4fNormal3fVertex3fvSUN( tc, c, n, v );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord2fColor4ubVertex3fSUN( GLfloat s, GLfloat t, GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glTexCoord2fColor4ubVertex3fSUN)( GLfloat, GLfloat, GLubyte, GLubyte, GLubyte, GLubyte, GLfloat, GLfloat, GLfloat ) = extension_funcs[758];
  TRACE("(%f, %f, %d, %d, %d, %d, %f, %f, %f)\n", s, t, r, g, b, a, x, y, z );
  ENTER_GL();
  func_glTexCoord2fColor4ubVertex3fSUN( s, t, r, g, b, a, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord2fColor4ubVertex3fvSUN( GLfloat* tc, GLubyte* c, GLfloat* v ) {
  void (*func_glTexCoord2fColor4ubVertex3fvSUN)( GLfloat*, GLubyte*, GLfloat* ) = extension_funcs[759];
  TRACE("(%p, %p, %p)\n", tc, c, v );
  ENTER_GL();
  func_glTexCoord2fColor4ubVertex3fvSUN( tc, c, v );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord2fNormal3fVertex3fSUN( GLfloat s, GLfloat t, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glTexCoord2fNormal3fVertex3fSUN)( GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[760];
  TRACE("(%f, %f, %f, %f, %f, %f, %f, %f)\n", s, t, nx, ny, nz, x, y, z );
  ENTER_GL();
  func_glTexCoord2fNormal3fVertex3fSUN( s, t, nx, ny, nz, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord2fNormal3fVertex3fvSUN( GLfloat* tc, GLfloat* n, GLfloat* v ) {
  void (*func_glTexCoord2fNormal3fVertex3fvSUN)( GLfloat*, GLfloat*, GLfloat* ) = extension_funcs[761];
  TRACE("(%p, %p, %p)\n", tc, n, v );
  ENTER_GL();
  func_glTexCoord2fNormal3fVertex3fvSUN( tc, n, v );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord2fVertex3fSUN( GLfloat s, GLfloat t, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glTexCoord2fVertex3fSUN)( GLfloat, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[762];
  TRACE("(%f, %f, %f, %f, %f)\n", s, t, x, y, z );
  ENTER_GL();
  func_glTexCoord2fVertex3fSUN( s, t, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord2fVertex3fvSUN( GLfloat* tc, GLfloat* v ) {
  void (*func_glTexCoord2fVertex3fvSUN)( GLfloat*, GLfloat* ) = extension_funcs[763];
  TRACE("(%p, %p)\n", tc, v );
  ENTER_GL();
  func_glTexCoord2fVertex3fvSUN( tc, v );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord2hNV( unsigned short s, unsigned short t ) {
  void (*func_glTexCoord2hNV)( unsigned short, unsigned short ) = extension_funcs[764];
  TRACE("(%d, %d)\n", s, t );
  ENTER_GL();
  func_glTexCoord2hNV( s, t );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord2hvNV( unsigned short* v ) {
  void (*func_glTexCoord2hvNV)( unsigned short* ) = extension_funcs[765];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glTexCoord2hvNV( v );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord3hNV( unsigned short s, unsigned short t, unsigned short r ) {
  void (*func_glTexCoord3hNV)( unsigned short, unsigned short, unsigned short ) = extension_funcs[766];
  TRACE("(%d, %d, %d)\n", s, t, r );
  ENTER_GL();
  func_glTexCoord3hNV( s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord3hvNV( unsigned short* v ) {
  void (*func_glTexCoord3hvNV)( unsigned short* ) = extension_funcs[767];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glTexCoord3hvNV( v );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord4fColor4fNormal3fVertex4fSUN( GLfloat s, GLfloat t, GLfloat p, GLfloat q, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  void (*func_glTexCoord4fColor4fNormal3fVertex4fSUN)( GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[768];
  TRACE("(%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f)\n", s, t, p, q, r, g, b, a, nx, ny, nz, x, y, z, w );
  ENTER_GL();
  func_glTexCoord4fColor4fNormal3fVertex4fSUN( s, t, p, q, r, g, b, a, nx, ny, nz, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord4fColor4fNormal3fVertex4fvSUN( GLfloat* tc, GLfloat* c, GLfloat* n, GLfloat* v ) {
  void (*func_glTexCoord4fColor4fNormal3fVertex4fvSUN)( GLfloat*, GLfloat*, GLfloat*, GLfloat* ) = extension_funcs[769];
  TRACE("(%p, %p, %p, %p)\n", tc, c, n, v );
  ENTER_GL();
  func_glTexCoord4fColor4fNormal3fVertex4fvSUN( tc, c, n, v );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord4fVertex4fSUN( GLfloat s, GLfloat t, GLfloat p, GLfloat q, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  void (*func_glTexCoord4fVertex4fSUN)( GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[770];
  TRACE("(%f, %f, %f, %f, %f, %f, %f, %f)\n", s, t, p, q, x, y, z, w );
  ENTER_GL();
  func_glTexCoord4fVertex4fSUN( s, t, p, q, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord4fVertex4fvSUN( GLfloat* tc, GLfloat* v ) {
  void (*func_glTexCoord4fVertex4fvSUN)( GLfloat*, GLfloat* ) = extension_funcs[771];
  TRACE("(%p, %p)\n", tc, v );
  ENTER_GL();
  func_glTexCoord4fVertex4fvSUN( tc, v );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord4hNV( unsigned short s, unsigned short t, unsigned short r, unsigned short q ) {
  void (*func_glTexCoord4hNV)( unsigned short, unsigned short, unsigned short, unsigned short ) = extension_funcs[772];
  TRACE("(%d, %d, %d, %d)\n", s, t, r, q );
  ENTER_GL();
  func_glTexCoord4hNV( s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord4hvNV( unsigned short* v ) {
  void (*func_glTexCoord4hvNV)( unsigned short* ) = extension_funcs[773];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glTexCoord4hvNV( v );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoordPointerEXT( GLint size, GLenum type, GLsizei stride, GLsizei count, GLvoid* pointer ) {
  void (*func_glTexCoordPointerEXT)( GLint, GLenum, GLsizei, GLsizei, GLvoid* ) = extension_funcs[774];
  TRACE("(%d, %d, %d, %d, %p)\n", size, type, stride, count, pointer );
  ENTER_GL();
  func_glTexCoordPointerEXT( size, type, stride, count, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoordPointerListIBM( GLint size, GLenum type, GLint stride, GLvoid** pointer, GLint ptrstride ) {
  void (*func_glTexCoordPointerListIBM)( GLint, GLenum, GLint, GLvoid**, GLint ) = extension_funcs[775];
  TRACE("(%d, %d, %d, %p, %d)\n", size, type, stride, pointer, ptrstride );
  ENTER_GL();
  func_glTexCoordPointerListIBM( size, type, stride, pointer, ptrstride );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoordPointervINTEL( GLint size, GLenum type, GLvoid** pointer ) {
  void (*func_glTexCoordPointervINTEL)( GLint, GLenum, GLvoid** ) = extension_funcs[776];
  TRACE("(%d, %d, %p)\n", size, type, pointer );
  ENTER_GL();
  func_glTexCoordPointervINTEL( size, type, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glTexFilterFuncSGIS( GLenum target, GLenum filter, GLsizei n, GLfloat* weights ) {
  void (*func_glTexFilterFuncSGIS)( GLenum, GLenum, GLsizei, GLfloat* ) = extension_funcs[777];
  TRACE("(%d, %d, %d, %p)\n", target, filter, n, weights );
  ENTER_GL();
  func_glTexFilterFuncSGIS( target, filter, n, weights );
  LEAVE_GL();
}

static void WINAPI wine_glTexImage3DEXT( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, GLvoid* pixels ) {
  void (*func_glTexImage3DEXT)( GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, GLvoid* ) = extension_funcs[778];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, height, depth, border, format, type, pixels );
  ENTER_GL();
  func_glTexImage3DEXT( target, level, internalformat, width, height, depth, border, format, type, pixels );
  LEAVE_GL();
}

static void WINAPI wine_glTexImage4DSGIS( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLint border, GLenum format, GLenum type, GLvoid* pixels ) {
  void (*func_glTexImage4DSGIS)( GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, GLvoid* ) = extension_funcs[779];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, height, depth, size4d, border, format, type, pixels );
  ENTER_GL();
  func_glTexImage4DSGIS( target, level, internalformat, width, height, depth, size4d, border, format, type, pixels );
  LEAVE_GL();
}

static void WINAPI wine_glTexSubImage1DEXT( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, GLvoid* pixels ) {
  void (*func_glTexSubImage1DEXT)( GLenum, GLint, GLint, GLsizei, GLenum, GLenum, GLvoid* ) = extension_funcs[780];
  TRACE("(%d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, width, format, type, pixels );
  ENTER_GL();
  func_glTexSubImage1DEXT( target, level, xoffset, width, format, type, pixels );
  LEAVE_GL();
}

static void WINAPI wine_glTexSubImage2DEXT( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels ) {
  void (*func_glTexSubImage2DEXT)( GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid* ) = extension_funcs[781];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, width, height, format, type, pixels );
  ENTER_GL();
  func_glTexSubImage2DEXT( target, level, xoffset, yoffset, width, height, format, type, pixels );
  LEAVE_GL();
}

static void WINAPI wine_glTexSubImage3DEXT( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, GLvoid* pixels ) {
  void (*func_glTexSubImage3DEXT)( GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, GLvoid* ) = extension_funcs[782];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels );
  ENTER_GL();
  func_glTexSubImage3DEXT( target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels );
  LEAVE_GL();
}

static void WINAPI wine_glTexSubImage4DSGIS( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint woffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLenum format, GLenum type, GLvoid* pixels ) {
  void (*func_glTexSubImage4DSGIS)( GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLsizei, GLenum, GLenum, GLvoid* ) = extension_funcs[783];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, zoffset, woffset, width, height, depth, size4d, format, type, pixels );
  ENTER_GL();
  func_glTexSubImage4DSGIS( target, level, xoffset, yoffset, zoffset, woffset, width, height, depth, size4d, format, type, pixels );
  LEAVE_GL();
}

static void WINAPI wine_glTextureColorMaskSGIS( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha ) {
  void (*func_glTextureColorMaskSGIS)( GLboolean, GLboolean, GLboolean, GLboolean ) = extension_funcs[784];
  TRACE("(%d, %d, %d, %d)\n", red, green, blue, alpha );
  ENTER_GL();
  func_glTextureColorMaskSGIS( red, green, blue, alpha );
  LEAVE_GL();
}

static void WINAPI wine_glTextureLightEXT( GLenum pname ) {
  void (*func_glTextureLightEXT)( GLenum ) = extension_funcs[785];
  TRACE("(%d)\n", pname );
  ENTER_GL();
  func_glTextureLightEXT( pname );
  LEAVE_GL();
}

static void WINAPI wine_glTextureMaterialEXT( GLenum face, GLenum mode ) {
  void (*func_glTextureMaterialEXT)( GLenum, GLenum ) = extension_funcs[786];
  TRACE("(%d, %d)\n", face, mode );
  ENTER_GL();
  func_glTextureMaterialEXT( face, mode );
  LEAVE_GL();
}

static void WINAPI wine_glTextureNormalEXT( GLenum mode ) {
  void (*func_glTextureNormalEXT)( GLenum ) = extension_funcs[787];
  TRACE("(%d)\n", mode );
  ENTER_GL();
  func_glTextureNormalEXT( mode );
  LEAVE_GL();
}

static void WINAPI wine_glTrackMatrixNV( GLenum target, GLuint address, GLenum matrix, GLenum transform ) {
  void (*func_glTrackMatrixNV)( GLenum, GLuint, GLenum, GLenum ) = extension_funcs[788];
  TRACE("(%d, %d, %d, %d)\n", target, address, matrix, transform );
  ENTER_GL();
  func_glTrackMatrixNV( target, address, matrix, transform );
  LEAVE_GL();
}

static void WINAPI wine_glUniform1f( GLint location, GLfloat v0 ) {
  void (*func_glUniform1f)( GLint, GLfloat ) = extension_funcs[789];
  TRACE("(%d, %f)\n", location, v0 );
  ENTER_GL();
  func_glUniform1f( location, v0 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform1fARB( GLint location, GLfloat v0 ) {
  void (*func_glUniform1fARB)( GLint, GLfloat ) = extension_funcs[790];
  TRACE("(%d, %f)\n", location, v0 );
  ENTER_GL();
  func_glUniform1fARB( location, v0 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform1fv( GLint location, GLsizei count, GLfloat* value ) {
  void (*func_glUniform1fv)( GLint, GLsizei, GLfloat* ) = extension_funcs[791];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform1fv( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform1fvARB( GLint location, GLsizei count, GLfloat* value ) {
  void (*func_glUniform1fvARB)( GLint, GLsizei, GLfloat* ) = extension_funcs[792];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform1fvARB( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform1i( GLint location, GLint v0 ) {
  void (*func_glUniform1i)( GLint, GLint ) = extension_funcs[793];
  TRACE("(%d, %d)\n", location, v0 );
  ENTER_GL();
  func_glUniform1i( location, v0 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform1iARB( GLint location, GLint v0 ) {
  void (*func_glUniform1iARB)( GLint, GLint ) = extension_funcs[794];
  TRACE("(%d, %d)\n", location, v0 );
  ENTER_GL();
  func_glUniform1iARB( location, v0 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform1iv( GLint location, GLsizei count, GLint* value ) {
  void (*func_glUniform1iv)( GLint, GLsizei, GLint* ) = extension_funcs[795];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform1iv( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform1ivARB( GLint location, GLsizei count, GLint* value ) {
  void (*func_glUniform1ivARB)( GLint, GLsizei, GLint* ) = extension_funcs[796];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform1ivARB( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform2f( GLint location, GLfloat v0, GLfloat v1 ) {
  void (*func_glUniform2f)( GLint, GLfloat, GLfloat ) = extension_funcs[797];
  TRACE("(%d, %f, %f)\n", location, v0, v1 );
  ENTER_GL();
  func_glUniform2f( location, v0, v1 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform2fARB( GLint location, GLfloat v0, GLfloat v1 ) {
  void (*func_glUniform2fARB)( GLint, GLfloat, GLfloat ) = extension_funcs[798];
  TRACE("(%d, %f, %f)\n", location, v0, v1 );
  ENTER_GL();
  func_glUniform2fARB( location, v0, v1 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform2fv( GLint location, GLsizei count, GLfloat* value ) {
  void (*func_glUniform2fv)( GLint, GLsizei, GLfloat* ) = extension_funcs[799];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform2fv( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform2fvARB( GLint location, GLsizei count, GLfloat* value ) {
  void (*func_glUniform2fvARB)( GLint, GLsizei, GLfloat* ) = extension_funcs[800];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform2fvARB( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform2i( GLint location, GLint v0, GLint v1 ) {
  void (*func_glUniform2i)( GLint, GLint, GLint ) = extension_funcs[801];
  TRACE("(%d, %d, %d)\n", location, v0, v1 );
  ENTER_GL();
  func_glUniform2i( location, v0, v1 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform2iARB( GLint location, GLint v0, GLint v1 ) {
  void (*func_glUniform2iARB)( GLint, GLint, GLint ) = extension_funcs[802];
  TRACE("(%d, %d, %d)\n", location, v0, v1 );
  ENTER_GL();
  func_glUniform2iARB( location, v0, v1 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform2iv( GLint location, GLsizei count, GLint* value ) {
  void (*func_glUniform2iv)( GLint, GLsizei, GLint* ) = extension_funcs[803];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform2iv( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform2ivARB( GLint location, GLsizei count, GLint* value ) {
  void (*func_glUniform2ivARB)( GLint, GLsizei, GLint* ) = extension_funcs[804];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform2ivARB( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform3f( GLint location, GLfloat v0, GLfloat v1, GLfloat v2 ) {
  void (*func_glUniform3f)( GLint, GLfloat, GLfloat, GLfloat ) = extension_funcs[805];
  TRACE("(%d, %f, %f, %f)\n", location, v0, v1, v2 );
  ENTER_GL();
  func_glUniform3f( location, v0, v1, v2 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform3fARB( GLint location, GLfloat v0, GLfloat v1, GLfloat v2 ) {
  void (*func_glUniform3fARB)( GLint, GLfloat, GLfloat, GLfloat ) = extension_funcs[806];
  TRACE("(%d, %f, %f, %f)\n", location, v0, v1, v2 );
  ENTER_GL();
  func_glUniform3fARB( location, v0, v1, v2 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform3fv( GLint location, GLsizei count, GLfloat* value ) {
  void (*func_glUniform3fv)( GLint, GLsizei, GLfloat* ) = extension_funcs[807];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform3fv( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform3fvARB( GLint location, GLsizei count, GLfloat* value ) {
  void (*func_glUniform3fvARB)( GLint, GLsizei, GLfloat* ) = extension_funcs[808];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform3fvARB( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform3i( GLint location, GLint v0, GLint v1, GLint v2 ) {
  void (*func_glUniform3i)( GLint, GLint, GLint, GLint ) = extension_funcs[809];
  TRACE("(%d, %d, %d, %d)\n", location, v0, v1, v2 );
  ENTER_GL();
  func_glUniform3i( location, v0, v1, v2 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform3iARB( GLint location, GLint v0, GLint v1, GLint v2 ) {
  void (*func_glUniform3iARB)( GLint, GLint, GLint, GLint ) = extension_funcs[810];
  TRACE("(%d, %d, %d, %d)\n", location, v0, v1, v2 );
  ENTER_GL();
  func_glUniform3iARB( location, v0, v1, v2 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform3iv( GLint location, GLsizei count, GLint* value ) {
  void (*func_glUniform3iv)( GLint, GLsizei, GLint* ) = extension_funcs[811];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform3iv( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform3ivARB( GLint location, GLsizei count, GLint* value ) {
  void (*func_glUniform3ivARB)( GLint, GLsizei, GLint* ) = extension_funcs[812];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform3ivARB( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform4f( GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 ) {
  void (*func_glUniform4f)( GLint, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[813];
  TRACE("(%d, %f, %f, %f, %f)\n", location, v0, v1, v2, v3 );
  ENTER_GL();
  func_glUniform4f( location, v0, v1, v2, v3 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform4fARB( GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 ) {
  void (*func_glUniform4fARB)( GLint, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[814];
  TRACE("(%d, %f, %f, %f, %f)\n", location, v0, v1, v2, v3 );
  ENTER_GL();
  func_glUniform4fARB( location, v0, v1, v2, v3 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform4fv( GLint location, GLsizei count, GLfloat* value ) {
  void (*func_glUniform4fv)( GLint, GLsizei, GLfloat* ) = extension_funcs[815];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform4fv( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform4fvARB( GLint location, GLsizei count, GLfloat* value ) {
  void (*func_glUniform4fvARB)( GLint, GLsizei, GLfloat* ) = extension_funcs[816];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform4fvARB( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform4i( GLint location, GLint v0, GLint v1, GLint v2, GLint v3 ) {
  void (*func_glUniform4i)( GLint, GLint, GLint, GLint, GLint ) = extension_funcs[817];
  TRACE("(%d, %d, %d, %d, %d)\n", location, v0, v1, v2, v3 );
  ENTER_GL();
  func_glUniform4i( location, v0, v1, v2, v3 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform4iARB( GLint location, GLint v0, GLint v1, GLint v2, GLint v3 ) {
  void (*func_glUniform4iARB)( GLint, GLint, GLint, GLint, GLint ) = extension_funcs[818];
  TRACE("(%d, %d, %d, %d, %d)\n", location, v0, v1, v2, v3 );
  ENTER_GL();
  func_glUniform4iARB( location, v0, v1, v2, v3 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform4iv( GLint location, GLsizei count, GLint* value ) {
  void (*func_glUniform4iv)( GLint, GLsizei, GLint* ) = extension_funcs[819];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform4iv( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform4ivARB( GLint location, GLsizei count, GLint* value ) {
  void (*func_glUniform4ivARB)( GLint, GLsizei, GLint* ) = extension_funcs[820];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform4ivARB( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniformMatrix2fv( GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  void (*func_glUniformMatrix2fv)( GLint, GLsizei, GLboolean, GLfloat* ) = extension_funcs[821];
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  ENTER_GL();
  func_glUniformMatrix2fv( location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniformMatrix2fvARB( GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  void (*func_glUniformMatrix2fvARB)( GLint, GLsizei, GLboolean, GLfloat* ) = extension_funcs[822];
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  ENTER_GL();
  func_glUniformMatrix2fvARB( location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniformMatrix3fv( GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  void (*func_glUniformMatrix3fv)( GLint, GLsizei, GLboolean, GLfloat* ) = extension_funcs[823];
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  ENTER_GL();
  func_glUniformMatrix3fv( location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniformMatrix3fvARB( GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  void (*func_glUniformMatrix3fvARB)( GLint, GLsizei, GLboolean, GLfloat* ) = extension_funcs[824];
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  ENTER_GL();
  func_glUniformMatrix3fvARB( location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniformMatrix4fv( GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  void (*func_glUniformMatrix4fv)( GLint, GLsizei, GLboolean, GLfloat* ) = extension_funcs[825];
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  ENTER_GL();
  func_glUniformMatrix4fv( location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniformMatrix4fvARB( GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  void (*func_glUniformMatrix4fvARB)( GLint, GLsizei, GLboolean, GLfloat* ) = extension_funcs[826];
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  ENTER_GL();
  func_glUniformMatrix4fvARB( location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glUnlockArraysEXT( void ) {
  void (*func_glUnlockArraysEXT)( void ) = extension_funcs[827];
  TRACE("()\n");
  ENTER_GL();
  func_glUnlockArraysEXT( );
  LEAVE_GL();
}

static GLboolean WINAPI wine_glUnmapBuffer( GLenum target ) {
  GLboolean ret_value;
  GLboolean (*func_glUnmapBuffer)( GLenum ) = extension_funcs[828];
  TRACE("(%d)\n", target );
  ENTER_GL();
  ret_value = func_glUnmapBuffer( target );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glUnmapBufferARB( GLenum target ) {
  GLboolean ret_value;
  GLboolean (*func_glUnmapBufferARB)( GLenum ) = extension_funcs[829];
  TRACE("(%d)\n", target );
  ENTER_GL();
  ret_value = func_glUnmapBufferARB( target );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glUnmapObjectBufferATI( GLuint buffer ) {
  void (*func_glUnmapObjectBufferATI)( GLuint ) = extension_funcs[830];
  TRACE("(%d)\n", buffer );
  ENTER_GL();
  func_glUnmapObjectBufferATI( buffer );
  LEAVE_GL();
}

static void WINAPI wine_glUpdateObjectBufferATI( GLuint buffer, GLuint offset, GLsizei size, GLvoid* pointer, GLenum preserve ) {
  void (*func_glUpdateObjectBufferATI)( GLuint, GLuint, GLsizei, GLvoid*, GLenum ) = extension_funcs[831];
  TRACE("(%d, %d, %d, %p, %d)\n", buffer, offset, size, pointer, preserve );
  ENTER_GL();
  func_glUpdateObjectBufferATI( buffer, offset, size, pointer, preserve );
  LEAVE_GL();
}

static void WINAPI wine_glUseProgram( GLuint program ) {
  void (*func_glUseProgram)( GLuint ) = extension_funcs[832];
  TRACE("(%d)\n", program );
  ENTER_GL();
  func_glUseProgram( program );
  LEAVE_GL();
}

static void WINAPI wine_glUseProgramObjectARB( unsigned int programObj ) {
  void (*func_glUseProgramObjectARB)( unsigned int ) = extension_funcs[833];
  TRACE("(%d)\n", programObj );
  ENTER_GL();
  func_glUseProgramObjectARB( programObj );
  LEAVE_GL();
}

static void WINAPI wine_glValidateProgram( GLuint program ) {
  void (*func_glValidateProgram)( GLuint ) = extension_funcs[834];
  TRACE("(%d)\n", program );
  ENTER_GL();
  func_glValidateProgram( program );
  LEAVE_GL();
}

static void WINAPI wine_glValidateProgramARB( unsigned int programObj ) {
  void (*func_glValidateProgramARB)( unsigned int ) = extension_funcs[835];
  TRACE("(%d)\n", programObj );
  ENTER_GL();
  func_glValidateProgramARB( programObj );
  LEAVE_GL();
}

static void WINAPI wine_glVariantArrayObjectATI( GLuint id, GLenum type, GLsizei stride, GLuint buffer, GLuint offset ) {
  void (*func_glVariantArrayObjectATI)( GLuint, GLenum, GLsizei, GLuint, GLuint ) = extension_funcs[836];
  TRACE("(%d, %d, %d, %d, %d)\n", id, type, stride, buffer, offset );
  ENTER_GL();
  func_glVariantArrayObjectATI( id, type, stride, buffer, offset );
  LEAVE_GL();
}

static void WINAPI wine_glVariantPointerEXT( GLuint id, GLenum type, GLuint stride, GLvoid* addr ) {
  void (*func_glVariantPointerEXT)( GLuint, GLenum, GLuint, GLvoid* ) = extension_funcs[837];
  TRACE("(%d, %d, %d, %p)\n", id, type, stride, addr );
  ENTER_GL();
  func_glVariantPointerEXT( id, type, stride, addr );
  LEAVE_GL();
}

static void WINAPI wine_glVariantbvEXT( GLuint id, GLbyte* addr ) {
  void (*func_glVariantbvEXT)( GLuint, GLbyte* ) = extension_funcs[838];
  TRACE("(%d, %p)\n", id, addr );
  ENTER_GL();
  func_glVariantbvEXT( id, addr );
  LEAVE_GL();
}

static void WINAPI wine_glVariantdvEXT( GLuint id, GLdouble* addr ) {
  void (*func_glVariantdvEXT)( GLuint, GLdouble* ) = extension_funcs[839];
  TRACE("(%d, %p)\n", id, addr );
  ENTER_GL();
  func_glVariantdvEXT( id, addr );
  LEAVE_GL();
}

static void WINAPI wine_glVariantfvEXT( GLuint id, GLfloat* addr ) {
  void (*func_glVariantfvEXT)( GLuint, GLfloat* ) = extension_funcs[840];
  TRACE("(%d, %p)\n", id, addr );
  ENTER_GL();
  func_glVariantfvEXT( id, addr );
  LEAVE_GL();
}

static void WINAPI wine_glVariantivEXT( GLuint id, GLint* addr ) {
  void (*func_glVariantivEXT)( GLuint, GLint* ) = extension_funcs[841];
  TRACE("(%d, %p)\n", id, addr );
  ENTER_GL();
  func_glVariantivEXT( id, addr );
  LEAVE_GL();
}

static void WINAPI wine_glVariantsvEXT( GLuint id, GLshort* addr ) {
  void (*func_glVariantsvEXT)( GLuint, GLshort* ) = extension_funcs[842];
  TRACE("(%d, %p)\n", id, addr );
  ENTER_GL();
  func_glVariantsvEXT( id, addr );
  LEAVE_GL();
}

static void WINAPI wine_glVariantubvEXT( GLuint id, GLubyte* addr ) {
  void (*func_glVariantubvEXT)( GLuint, GLubyte* ) = extension_funcs[843];
  TRACE("(%d, %p)\n", id, addr );
  ENTER_GL();
  func_glVariantubvEXT( id, addr );
  LEAVE_GL();
}

static void WINAPI wine_glVariantuivEXT( GLuint id, GLuint* addr ) {
  void (*func_glVariantuivEXT)( GLuint, GLuint* ) = extension_funcs[844];
  TRACE("(%d, %p)\n", id, addr );
  ENTER_GL();
  func_glVariantuivEXT( id, addr );
  LEAVE_GL();
}

static void WINAPI wine_glVariantusvEXT( GLuint id, GLushort* addr ) {
  void (*func_glVariantusvEXT)( GLuint, GLushort* ) = extension_funcs[845];
  TRACE("(%d, %p)\n", id, addr );
  ENTER_GL();
  func_glVariantusvEXT( id, addr );
  LEAVE_GL();
}

static void WINAPI wine_glVertex2hNV( unsigned short x, unsigned short y ) {
  void (*func_glVertex2hNV)( unsigned short, unsigned short ) = extension_funcs[846];
  TRACE("(%d, %d)\n", x, y );
  ENTER_GL();
  func_glVertex2hNV( x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertex2hvNV( unsigned short* v ) {
  void (*func_glVertex2hvNV)( unsigned short* ) = extension_funcs[847];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glVertex2hvNV( v );
  LEAVE_GL();
}

static void WINAPI wine_glVertex3hNV( unsigned short x, unsigned short y, unsigned short z ) {
  void (*func_glVertex3hNV)( unsigned short, unsigned short, unsigned short ) = extension_funcs[848];
  TRACE("(%d, %d, %d)\n", x, y, z );
  ENTER_GL();
  func_glVertex3hNV( x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertex3hvNV( unsigned short* v ) {
  void (*func_glVertex3hvNV)( unsigned short* ) = extension_funcs[849];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glVertex3hvNV( v );
  LEAVE_GL();
}

static void WINAPI wine_glVertex4hNV( unsigned short x, unsigned short y, unsigned short z, unsigned short w ) {
  void (*func_glVertex4hNV)( unsigned short, unsigned short, unsigned short, unsigned short ) = extension_funcs[850];
  TRACE("(%d, %d, %d, %d)\n", x, y, z, w );
  ENTER_GL();
  func_glVertex4hNV( x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertex4hvNV( unsigned short* v ) {
  void (*func_glVertex4hvNV)( unsigned short* ) = extension_funcs[851];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glVertex4hvNV( v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexArrayParameteriAPPLE( GLenum pname, GLint param ) {
  void (*func_glVertexArrayParameteriAPPLE)( GLenum, GLint ) = extension_funcs[852];
  TRACE("(%d, %d)\n", pname, param );
  ENTER_GL();
  func_glVertexArrayParameteriAPPLE( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glVertexArrayRangeAPPLE( GLsizei length, GLvoid* pointer ) {
  void (*func_glVertexArrayRangeAPPLE)( GLsizei, GLvoid* ) = extension_funcs[853];
  TRACE("(%d, %p)\n", length, pointer );
  ENTER_GL();
  func_glVertexArrayRangeAPPLE( length, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glVertexArrayRangeNV( GLsizei length, GLvoid* pointer ) {
  void (*func_glVertexArrayRangeNV)( GLsizei, GLvoid* ) = extension_funcs[854];
  TRACE("(%d, %p)\n", length, pointer );
  ENTER_GL();
  func_glVertexArrayRangeNV( length, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1d( GLuint index, GLdouble x ) {
  void (*func_glVertexAttrib1d)( GLuint, GLdouble ) = extension_funcs[855];
  TRACE("(%d, %f)\n", index, x );
  ENTER_GL();
  func_glVertexAttrib1d( index, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1dARB( GLuint index, GLdouble x ) {
  void (*func_glVertexAttrib1dARB)( GLuint, GLdouble ) = extension_funcs[856];
  TRACE("(%d, %f)\n", index, x );
  ENTER_GL();
  func_glVertexAttrib1dARB( index, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1dNV( GLuint index, GLdouble x ) {
  void (*func_glVertexAttrib1dNV)( GLuint, GLdouble ) = extension_funcs[857];
  TRACE("(%d, %f)\n", index, x );
  ENTER_GL();
  func_glVertexAttrib1dNV( index, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1dv( GLuint index, GLdouble* v ) {
  void (*func_glVertexAttrib1dv)( GLuint, GLdouble* ) = extension_funcs[858];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib1dv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1dvARB( GLuint index, GLdouble* v ) {
  void (*func_glVertexAttrib1dvARB)( GLuint, GLdouble* ) = extension_funcs[859];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib1dvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1dvNV( GLuint index, GLdouble* v ) {
  void (*func_glVertexAttrib1dvNV)( GLuint, GLdouble* ) = extension_funcs[860];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib1dvNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1f( GLuint index, GLfloat x ) {
  void (*func_glVertexAttrib1f)( GLuint, GLfloat ) = extension_funcs[861];
  TRACE("(%d, %f)\n", index, x );
  ENTER_GL();
  func_glVertexAttrib1f( index, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1fARB( GLuint index, GLfloat x ) {
  void (*func_glVertexAttrib1fARB)( GLuint, GLfloat ) = extension_funcs[862];
  TRACE("(%d, %f)\n", index, x );
  ENTER_GL();
  func_glVertexAttrib1fARB( index, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1fNV( GLuint index, GLfloat x ) {
  void (*func_glVertexAttrib1fNV)( GLuint, GLfloat ) = extension_funcs[863];
  TRACE("(%d, %f)\n", index, x );
  ENTER_GL();
  func_glVertexAttrib1fNV( index, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1fv( GLuint index, GLfloat* v ) {
  void (*func_glVertexAttrib1fv)( GLuint, GLfloat* ) = extension_funcs[864];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib1fv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1fvARB( GLuint index, GLfloat* v ) {
  void (*func_glVertexAttrib1fvARB)( GLuint, GLfloat* ) = extension_funcs[865];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib1fvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1fvNV( GLuint index, GLfloat* v ) {
  void (*func_glVertexAttrib1fvNV)( GLuint, GLfloat* ) = extension_funcs[866];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib1fvNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1hNV( GLuint index, unsigned short x ) {
  void (*func_glVertexAttrib1hNV)( GLuint, unsigned short ) = extension_funcs[867];
  TRACE("(%d, %d)\n", index, x );
  ENTER_GL();
  func_glVertexAttrib1hNV( index, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1hvNV( GLuint index, unsigned short* v ) {
  void (*func_glVertexAttrib1hvNV)( GLuint, unsigned short* ) = extension_funcs[868];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib1hvNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1s( GLuint index, GLshort x ) {
  void (*func_glVertexAttrib1s)( GLuint, GLshort ) = extension_funcs[869];
  TRACE("(%d, %d)\n", index, x );
  ENTER_GL();
  func_glVertexAttrib1s( index, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1sARB( GLuint index, GLshort x ) {
  void (*func_glVertexAttrib1sARB)( GLuint, GLshort ) = extension_funcs[870];
  TRACE("(%d, %d)\n", index, x );
  ENTER_GL();
  func_glVertexAttrib1sARB( index, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1sNV( GLuint index, GLshort x ) {
  void (*func_glVertexAttrib1sNV)( GLuint, GLshort ) = extension_funcs[871];
  TRACE("(%d, %d)\n", index, x );
  ENTER_GL();
  func_glVertexAttrib1sNV( index, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1sv( GLuint index, GLshort* v ) {
  void (*func_glVertexAttrib1sv)( GLuint, GLshort* ) = extension_funcs[872];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib1sv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1svARB( GLuint index, GLshort* v ) {
  void (*func_glVertexAttrib1svARB)( GLuint, GLshort* ) = extension_funcs[873];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib1svARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1svNV( GLuint index, GLshort* v ) {
  void (*func_glVertexAttrib1svNV)( GLuint, GLshort* ) = extension_funcs[874];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib1svNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2d( GLuint index, GLdouble x, GLdouble y ) {
  void (*func_glVertexAttrib2d)( GLuint, GLdouble, GLdouble ) = extension_funcs[875];
  TRACE("(%d, %f, %f)\n", index, x, y );
  ENTER_GL();
  func_glVertexAttrib2d( index, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2dARB( GLuint index, GLdouble x, GLdouble y ) {
  void (*func_glVertexAttrib2dARB)( GLuint, GLdouble, GLdouble ) = extension_funcs[876];
  TRACE("(%d, %f, %f)\n", index, x, y );
  ENTER_GL();
  func_glVertexAttrib2dARB( index, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2dNV( GLuint index, GLdouble x, GLdouble y ) {
  void (*func_glVertexAttrib2dNV)( GLuint, GLdouble, GLdouble ) = extension_funcs[877];
  TRACE("(%d, %f, %f)\n", index, x, y );
  ENTER_GL();
  func_glVertexAttrib2dNV( index, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2dv( GLuint index, GLdouble* v ) {
  void (*func_glVertexAttrib2dv)( GLuint, GLdouble* ) = extension_funcs[878];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib2dv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2dvARB( GLuint index, GLdouble* v ) {
  void (*func_glVertexAttrib2dvARB)( GLuint, GLdouble* ) = extension_funcs[879];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib2dvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2dvNV( GLuint index, GLdouble* v ) {
  void (*func_glVertexAttrib2dvNV)( GLuint, GLdouble* ) = extension_funcs[880];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib2dvNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2f( GLuint index, GLfloat x, GLfloat y ) {
  void (*func_glVertexAttrib2f)( GLuint, GLfloat, GLfloat ) = extension_funcs[881];
  TRACE("(%d, %f, %f)\n", index, x, y );
  ENTER_GL();
  func_glVertexAttrib2f( index, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2fARB( GLuint index, GLfloat x, GLfloat y ) {
  void (*func_glVertexAttrib2fARB)( GLuint, GLfloat, GLfloat ) = extension_funcs[882];
  TRACE("(%d, %f, %f)\n", index, x, y );
  ENTER_GL();
  func_glVertexAttrib2fARB( index, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2fNV( GLuint index, GLfloat x, GLfloat y ) {
  void (*func_glVertexAttrib2fNV)( GLuint, GLfloat, GLfloat ) = extension_funcs[883];
  TRACE("(%d, %f, %f)\n", index, x, y );
  ENTER_GL();
  func_glVertexAttrib2fNV( index, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2fv( GLuint index, GLfloat* v ) {
  void (*func_glVertexAttrib2fv)( GLuint, GLfloat* ) = extension_funcs[884];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib2fv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2fvARB( GLuint index, GLfloat* v ) {
  void (*func_glVertexAttrib2fvARB)( GLuint, GLfloat* ) = extension_funcs[885];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib2fvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2fvNV( GLuint index, GLfloat* v ) {
  void (*func_glVertexAttrib2fvNV)( GLuint, GLfloat* ) = extension_funcs[886];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib2fvNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2hNV( GLuint index, unsigned short x, unsigned short y ) {
  void (*func_glVertexAttrib2hNV)( GLuint, unsigned short, unsigned short ) = extension_funcs[887];
  TRACE("(%d, %d, %d)\n", index, x, y );
  ENTER_GL();
  func_glVertexAttrib2hNV( index, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2hvNV( GLuint index, unsigned short* v ) {
  void (*func_glVertexAttrib2hvNV)( GLuint, unsigned short* ) = extension_funcs[888];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib2hvNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2s( GLuint index, GLshort x, GLshort y ) {
  void (*func_glVertexAttrib2s)( GLuint, GLshort, GLshort ) = extension_funcs[889];
  TRACE("(%d, %d, %d)\n", index, x, y );
  ENTER_GL();
  func_glVertexAttrib2s( index, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2sARB( GLuint index, GLshort x, GLshort y ) {
  void (*func_glVertexAttrib2sARB)( GLuint, GLshort, GLshort ) = extension_funcs[890];
  TRACE("(%d, %d, %d)\n", index, x, y );
  ENTER_GL();
  func_glVertexAttrib2sARB( index, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2sNV( GLuint index, GLshort x, GLshort y ) {
  void (*func_glVertexAttrib2sNV)( GLuint, GLshort, GLshort ) = extension_funcs[891];
  TRACE("(%d, %d, %d)\n", index, x, y );
  ENTER_GL();
  func_glVertexAttrib2sNV( index, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2sv( GLuint index, GLshort* v ) {
  void (*func_glVertexAttrib2sv)( GLuint, GLshort* ) = extension_funcs[892];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib2sv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2svARB( GLuint index, GLshort* v ) {
  void (*func_glVertexAttrib2svARB)( GLuint, GLshort* ) = extension_funcs[893];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib2svARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2svNV( GLuint index, GLshort* v ) {
  void (*func_glVertexAttrib2svNV)( GLuint, GLshort* ) = extension_funcs[894];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib2svNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3d( GLuint index, GLdouble x, GLdouble y, GLdouble z ) {
  void (*func_glVertexAttrib3d)( GLuint, GLdouble, GLdouble, GLdouble ) = extension_funcs[895];
  TRACE("(%d, %f, %f, %f)\n", index, x, y, z );
  ENTER_GL();
  func_glVertexAttrib3d( index, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3dARB( GLuint index, GLdouble x, GLdouble y, GLdouble z ) {
  void (*func_glVertexAttrib3dARB)( GLuint, GLdouble, GLdouble, GLdouble ) = extension_funcs[896];
  TRACE("(%d, %f, %f, %f)\n", index, x, y, z );
  ENTER_GL();
  func_glVertexAttrib3dARB( index, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3dNV( GLuint index, GLdouble x, GLdouble y, GLdouble z ) {
  void (*func_glVertexAttrib3dNV)( GLuint, GLdouble, GLdouble, GLdouble ) = extension_funcs[897];
  TRACE("(%d, %f, %f, %f)\n", index, x, y, z );
  ENTER_GL();
  func_glVertexAttrib3dNV( index, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3dv( GLuint index, GLdouble* v ) {
  void (*func_glVertexAttrib3dv)( GLuint, GLdouble* ) = extension_funcs[898];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib3dv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3dvARB( GLuint index, GLdouble* v ) {
  void (*func_glVertexAttrib3dvARB)( GLuint, GLdouble* ) = extension_funcs[899];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib3dvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3dvNV( GLuint index, GLdouble* v ) {
  void (*func_glVertexAttrib3dvNV)( GLuint, GLdouble* ) = extension_funcs[900];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib3dvNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3f( GLuint index, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glVertexAttrib3f)( GLuint, GLfloat, GLfloat, GLfloat ) = extension_funcs[901];
  TRACE("(%d, %f, %f, %f)\n", index, x, y, z );
  ENTER_GL();
  func_glVertexAttrib3f( index, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3fARB( GLuint index, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glVertexAttrib3fARB)( GLuint, GLfloat, GLfloat, GLfloat ) = extension_funcs[902];
  TRACE("(%d, %f, %f, %f)\n", index, x, y, z );
  ENTER_GL();
  func_glVertexAttrib3fARB( index, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3fNV( GLuint index, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glVertexAttrib3fNV)( GLuint, GLfloat, GLfloat, GLfloat ) = extension_funcs[903];
  TRACE("(%d, %f, %f, %f)\n", index, x, y, z );
  ENTER_GL();
  func_glVertexAttrib3fNV( index, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3fv( GLuint index, GLfloat* v ) {
  void (*func_glVertexAttrib3fv)( GLuint, GLfloat* ) = extension_funcs[904];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib3fv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3fvARB( GLuint index, GLfloat* v ) {
  void (*func_glVertexAttrib3fvARB)( GLuint, GLfloat* ) = extension_funcs[905];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib3fvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3fvNV( GLuint index, GLfloat* v ) {
  void (*func_glVertexAttrib3fvNV)( GLuint, GLfloat* ) = extension_funcs[906];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib3fvNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3hNV( GLuint index, unsigned short x, unsigned short y, unsigned short z ) {
  void (*func_glVertexAttrib3hNV)( GLuint, unsigned short, unsigned short, unsigned short ) = extension_funcs[907];
  TRACE("(%d, %d, %d, %d)\n", index, x, y, z );
  ENTER_GL();
  func_glVertexAttrib3hNV( index, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3hvNV( GLuint index, unsigned short* v ) {
  void (*func_glVertexAttrib3hvNV)( GLuint, unsigned short* ) = extension_funcs[908];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib3hvNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3s( GLuint index, GLshort x, GLshort y, GLshort z ) {
  void (*func_glVertexAttrib3s)( GLuint, GLshort, GLshort, GLshort ) = extension_funcs[909];
  TRACE("(%d, %d, %d, %d)\n", index, x, y, z );
  ENTER_GL();
  func_glVertexAttrib3s( index, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3sARB( GLuint index, GLshort x, GLshort y, GLshort z ) {
  void (*func_glVertexAttrib3sARB)( GLuint, GLshort, GLshort, GLshort ) = extension_funcs[910];
  TRACE("(%d, %d, %d, %d)\n", index, x, y, z );
  ENTER_GL();
  func_glVertexAttrib3sARB( index, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3sNV( GLuint index, GLshort x, GLshort y, GLshort z ) {
  void (*func_glVertexAttrib3sNV)( GLuint, GLshort, GLshort, GLshort ) = extension_funcs[911];
  TRACE("(%d, %d, %d, %d)\n", index, x, y, z );
  ENTER_GL();
  func_glVertexAttrib3sNV( index, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3sv( GLuint index, GLshort* v ) {
  void (*func_glVertexAttrib3sv)( GLuint, GLshort* ) = extension_funcs[912];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib3sv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3svARB( GLuint index, GLshort* v ) {
  void (*func_glVertexAttrib3svARB)( GLuint, GLshort* ) = extension_funcs[913];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib3svARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3svNV( GLuint index, GLshort* v ) {
  void (*func_glVertexAttrib3svNV)( GLuint, GLshort* ) = extension_funcs[914];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib3svNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4Nbv( GLuint index, GLbyte* v ) {
  void (*func_glVertexAttrib4Nbv)( GLuint, GLbyte* ) = extension_funcs[915];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4Nbv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4NbvARB( GLuint index, GLbyte* v ) {
  void (*func_glVertexAttrib4NbvARB)( GLuint, GLbyte* ) = extension_funcs[916];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4NbvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4Niv( GLuint index, GLint* v ) {
  void (*func_glVertexAttrib4Niv)( GLuint, GLint* ) = extension_funcs[917];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4Niv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4NivARB( GLuint index, GLint* v ) {
  void (*func_glVertexAttrib4NivARB)( GLuint, GLint* ) = extension_funcs[918];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4NivARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4Nsv( GLuint index, GLshort* v ) {
  void (*func_glVertexAttrib4Nsv)( GLuint, GLshort* ) = extension_funcs[919];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4Nsv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4NsvARB( GLuint index, GLshort* v ) {
  void (*func_glVertexAttrib4NsvARB)( GLuint, GLshort* ) = extension_funcs[920];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4NsvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4Nub( GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w ) {
  void (*func_glVertexAttrib4Nub)( GLuint, GLubyte, GLubyte, GLubyte, GLubyte ) = extension_funcs[921];
  TRACE("(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttrib4Nub( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4NubARB( GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w ) {
  void (*func_glVertexAttrib4NubARB)( GLuint, GLubyte, GLubyte, GLubyte, GLubyte ) = extension_funcs[922];
  TRACE("(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttrib4NubARB( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4Nubv( GLuint index, GLubyte* v ) {
  void (*func_glVertexAttrib4Nubv)( GLuint, GLubyte* ) = extension_funcs[923];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4Nubv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4NubvARB( GLuint index, GLubyte* v ) {
  void (*func_glVertexAttrib4NubvARB)( GLuint, GLubyte* ) = extension_funcs[924];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4NubvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4Nuiv( GLuint index, GLuint* v ) {
  void (*func_glVertexAttrib4Nuiv)( GLuint, GLuint* ) = extension_funcs[925];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4Nuiv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4NuivARB( GLuint index, GLuint* v ) {
  void (*func_glVertexAttrib4NuivARB)( GLuint, GLuint* ) = extension_funcs[926];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4NuivARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4Nusv( GLuint index, GLushort* v ) {
  void (*func_glVertexAttrib4Nusv)( GLuint, GLushort* ) = extension_funcs[927];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4Nusv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4NusvARB( GLuint index, GLushort* v ) {
  void (*func_glVertexAttrib4NusvARB)( GLuint, GLushort* ) = extension_funcs[928];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4NusvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4bv( GLuint index, GLbyte* v ) {
  void (*func_glVertexAttrib4bv)( GLuint, GLbyte* ) = extension_funcs[929];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4bv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4bvARB( GLuint index, GLbyte* v ) {
  void (*func_glVertexAttrib4bvARB)( GLuint, GLbyte* ) = extension_funcs[930];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4bvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4d( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  void (*func_glVertexAttrib4d)( GLuint, GLdouble, GLdouble, GLdouble, GLdouble ) = extension_funcs[931];
  TRACE("(%d, %f, %f, %f, %f)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttrib4d( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4dARB( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  void (*func_glVertexAttrib4dARB)( GLuint, GLdouble, GLdouble, GLdouble, GLdouble ) = extension_funcs[932];
  TRACE("(%d, %f, %f, %f, %f)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttrib4dARB( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4dNV( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  void (*func_glVertexAttrib4dNV)( GLuint, GLdouble, GLdouble, GLdouble, GLdouble ) = extension_funcs[933];
  TRACE("(%d, %f, %f, %f, %f)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttrib4dNV( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4dv( GLuint index, GLdouble* v ) {
  void (*func_glVertexAttrib4dv)( GLuint, GLdouble* ) = extension_funcs[934];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4dv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4dvARB( GLuint index, GLdouble* v ) {
  void (*func_glVertexAttrib4dvARB)( GLuint, GLdouble* ) = extension_funcs[935];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4dvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4dvNV( GLuint index, GLdouble* v ) {
  void (*func_glVertexAttrib4dvNV)( GLuint, GLdouble* ) = extension_funcs[936];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4dvNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4f( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  void (*func_glVertexAttrib4f)( GLuint, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[937];
  TRACE("(%d, %f, %f, %f, %f)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttrib4f( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4fARB( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  void (*func_glVertexAttrib4fARB)( GLuint, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[938];
  TRACE("(%d, %f, %f, %f, %f)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttrib4fARB( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4fNV( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  void (*func_glVertexAttrib4fNV)( GLuint, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[939];
  TRACE("(%d, %f, %f, %f, %f)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttrib4fNV( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4fv( GLuint index, GLfloat* v ) {
  void (*func_glVertexAttrib4fv)( GLuint, GLfloat* ) = extension_funcs[940];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4fv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4fvARB( GLuint index, GLfloat* v ) {
  void (*func_glVertexAttrib4fvARB)( GLuint, GLfloat* ) = extension_funcs[941];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4fvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4fvNV( GLuint index, GLfloat* v ) {
  void (*func_glVertexAttrib4fvNV)( GLuint, GLfloat* ) = extension_funcs[942];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4fvNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4hNV( GLuint index, unsigned short x, unsigned short y, unsigned short z, unsigned short w ) {
  void (*func_glVertexAttrib4hNV)( GLuint, unsigned short, unsigned short, unsigned short, unsigned short ) = extension_funcs[943];
  TRACE("(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttrib4hNV( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4hvNV( GLuint index, unsigned short* v ) {
  void (*func_glVertexAttrib4hvNV)( GLuint, unsigned short* ) = extension_funcs[944];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4hvNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4iv( GLuint index, GLint* v ) {
  void (*func_glVertexAttrib4iv)( GLuint, GLint* ) = extension_funcs[945];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4iv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4ivARB( GLuint index, GLint* v ) {
  void (*func_glVertexAttrib4ivARB)( GLuint, GLint* ) = extension_funcs[946];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4ivARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4s( GLuint index, GLshort x, GLshort y, GLshort z, GLshort w ) {
  void (*func_glVertexAttrib4s)( GLuint, GLshort, GLshort, GLshort, GLshort ) = extension_funcs[947];
  TRACE("(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttrib4s( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4sARB( GLuint index, GLshort x, GLshort y, GLshort z, GLshort w ) {
  void (*func_glVertexAttrib4sARB)( GLuint, GLshort, GLshort, GLshort, GLshort ) = extension_funcs[948];
  TRACE("(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttrib4sARB( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4sNV( GLuint index, GLshort x, GLshort y, GLshort z, GLshort w ) {
  void (*func_glVertexAttrib4sNV)( GLuint, GLshort, GLshort, GLshort, GLshort ) = extension_funcs[949];
  TRACE("(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttrib4sNV( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4sv( GLuint index, GLshort* v ) {
  void (*func_glVertexAttrib4sv)( GLuint, GLshort* ) = extension_funcs[950];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4sv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4svARB( GLuint index, GLshort* v ) {
  void (*func_glVertexAttrib4svARB)( GLuint, GLshort* ) = extension_funcs[951];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4svARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4svNV( GLuint index, GLshort* v ) {
  void (*func_glVertexAttrib4svNV)( GLuint, GLshort* ) = extension_funcs[952];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4svNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4ubNV( GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w ) {
  void (*func_glVertexAttrib4ubNV)( GLuint, GLubyte, GLubyte, GLubyte, GLubyte ) = extension_funcs[953];
  TRACE("(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttrib4ubNV( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4ubv( GLuint index, GLubyte* v ) {
  void (*func_glVertexAttrib4ubv)( GLuint, GLubyte* ) = extension_funcs[954];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4ubv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4ubvARB( GLuint index, GLubyte* v ) {
  void (*func_glVertexAttrib4ubvARB)( GLuint, GLubyte* ) = extension_funcs[955];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4ubvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4ubvNV( GLuint index, GLubyte* v ) {
  void (*func_glVertexAttrib4ubvNV)( GLuint, GLubyte* ) = extension_funcs[956];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4ubvNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4uiv( GLuint index, GLuint* v ) {
  void (*func_glVertexAttrib4uiv)( GLuint, GLuint* ) = extension_funcs[957];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4uiv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4uivARB( GLuint index, GLuint* v ) {
  void (*func_glVertexAttrib4uivARB)( GLuint, GLuint* ) = extension_funcs[958];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4uivARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4usv( GLuint index, GLushort* v ) {
  void (*func_glVertexAttrib4usv)( GLuint, GLushort* ) = extension_funcs[959];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4usv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4usvARB( GLuint index, GLushort* v ) {
  void (*func_glVertexAttrib4usvARB)( GLuint, GLushort* ) = extension_funcs[960];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4usvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribArrayObjectATI( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, GLuint buffer, GLuint offset ) {
  void (*func_glVertexAttribArrayObjectATI)( GLuint, GLint, GLenum, GLboolean, GLsizei, GLuint, GLuint ) = extension_funcs[961];
  TRACE("(%d, %d, %d, %d, %d, %d, %d)\n", index, size, type, normalized, stride, buffer, offset );
  ENTER_GL();
  func_glVertexAttribArrayObjectATI( index, size, type, normalized, stride, buffer, offset );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribPointer( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, GLvoid* pointer ) {
  void (*func_glVertexAttribPointer)( GLuint, GLint, GLenum, GLboolean, GLsizei, GLvoid* ) = extension_funcs[962];
  TRACE("(%d, %d, %d, %d, %d, %p)\n", index, size, type, normalized, stride, pointer );
  ENTER_GL();
  func_glVertexAttribPointer( index, size, type, normalized, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribPointerARB( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, GLvoid* pointer ) {
  void (*func_glVertexAttribPointerARB)( GLuint, GLint, GLenum, GLboolean, GLsizei, GLvoid* ) = extension_funcs[963];
  TRACE("(%d, %d, %d, %d, %d, %p)\n", index, size, type, normalized, stride, pointer );
  ENTER_GL();
  func_glVertexAttribPointerARB( index, size, type, normalized, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribPointerNV( GLuint index, GLint fsize, GLenum type, GLsizei stride, GLvoid* pointer ) {
  void (*func_glVertexAttribPointerNV)( GLuint, GLint, GLenum, GLsizei, GLvoid* ) = extension_funcs[964];
  TRACE("(%d, %d, %d, %d, %p)\n", index, fsize, type, stride, pointer );
  ENTER_GL();
  func_glVertexAttribPointerNV( index, fsize, type, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs1dvNV( GLuint index, GLsizei count, GLdouble* v ) {
  void (*func_glVertexAttribs1dvNV)( GLuint, GLsizei, GLdouble* ) = extension_funcs[965];
  TRACE("(%d, %d, %p)\n", index, count, v );
  ENTER_GL();
  func_glVertexAttribs1dvNV( index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs1fvNV( GLuint index, GLsizei count, GLfloat* v ) {
  void (*func_glVertexAttribs1fvNV)( GLuint, GLsizei, GLfloat* ) = extension_funcs[966];
  TRACE("(%d, %d, %p)\n", index, count, v );
  ENTER_GL();
  func_glVertexAttribs1fvNV( index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs1hvNV( GLuint index, GLsizei n, unsigned short* v ) {
  void (*func_glVertexAttribs1hvNV)( GLuint, GLsizei, unsigned short* ) = extension_funcs[967];
  TRACE("(%d, %d, %p)\n", index, n, v );
  ENTER_GL();
  func_glVertexAttribs1hvNV( index, n, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs1svNV( GLuint index, GLsizei count, GLshort* v ) {
  void (*func_glVertexAttribs1svNV)( GLuint, GLsizei, GLshort* ) = extension_funcs[968];
  TRACE("(%d, %d, %p)\n", index, count, v );
  ENTER_GL();
  func_glVertexAttribs1svNV( index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs2dvNV( GLuint index, GLsizei count, GLdouble* v ) {
  void (*func_glVertexAttribs2dvNV)( GLuint, GLsizei, GLdouble* ) = extension_funcs[969];
  TRACE("(%d, %d, %p)\n", index, count, v );
  ENTER_GL();
  func_glVertexAttribs2dvNV( index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs2fvNV( GLuint index, GLsizei count, GLfloat* v ) {
  void (*func_glVertexAttribs2fvNV)( GLuint, GLsizei, GLfloat* ) = extension_funcs[970];
  TRACE("(%d, %d, %p)\n", index, count, v );
  ENTER_GL();
  func_glVertexAttribs2fvNV( index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs2hvNV( GLuint index, GLsizei n, unsigned short* v ) {
  void (*func_glVertexAttribs2hvNV)( GLuint, GLsizei, unsigned short* ) = extension_funcs[971];
  TRACE("(%d, %d, %p)\n", index, n, v );
  ENTER_GL();
  func_glVertexAttribs2hvNV( index, n, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs2svNV( GLuint index, GLsizei count, GLshort* v ) {
  void (*func_glVertexAttribs2svNV)( GLuint, GLsizei, GLshort* ) = extension_funcs[972];
  TRACE("(%d, %d, %p)\n", index, count, v );
  ENTER_GL();
  func_glVertexAttribs2svNV( index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs3dvNV( GLuint index, GLsizei count, GLdouble* v ) {
  void (*func_glVertexAttribs3dvNV)( GLuint, GLsizei, GLdouble* ) = extension_funcs[973];
  TRACE("(%d, %d, %p)\n", index, count, v );
  ENTER_GL();
  func_glVertexAttribs3dvNV( index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs3fvNV( GLuint index, GLsizei count, GLfloat* v ) {
  void (*func_glVertexAttribs3fvNV)( GLuint, GLsizei, GLfloat* ) = extension_funcs[974];
  TRACE("(%d, %d, %p)\n", index, count, v );
  ENTER_GL();
  func_glVertexAttribs3fvNV( index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs3hvNV( GLuint index, GLsizei n, unsigned short* v ) {
  void (*func_glVertexAttribs3hvNV)( GLuint, GLsizei, unsigned short* ) = extension_funcs[975];
  TRACE("(%d, %d, %p)\n", index, n, v );
  ENTER_GL();
  func_glVertexAttribs3hvNV( index, n, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs3svNV( GLuint index, GLsizei count, GLshort* v ) {
  void (*func_glVertexAttribs3svNV)( GLuint, GLsizei, GLshort* ) = extension_funcs[976];
  TRACE("(%d, %d, %p)\n", index, count, v );
  ENTER_GL();
  func_glVertexAttribs3svNV( index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs4dvNV( GLuint index, GLsizei count, GLdouble* v ) {
  void (*func_glVertexAttribs4dvNV)( GLuint, GLsizei, GLdouble* ) = extension_funcs[977];
  TRACE("(%d, %d, %p)\n", index, count, v );
  ENTER_GL();
  func_glVertexAttribs4dvNV( index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs4fvNV( GLuint index, GLsizei count, GLfloat* v ) {
  void (*func_glVertexAttribs4fvNV)( GLuint, GLsizei, GLfloat* ) = extension_funcs[978];
  TRACE("(%d, %d, %p)\n", index, count, v );
  ENTER_GL();
  func_glVertexAttribs4fvNV( index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs4hvNV( GLuint index, GLsizei n, unsigned short* v ) {
  void (*func_glVertexAttribs4hvNV)( GLuint, GLsizei, unsigned short* ) = extension_funcs[979];
  TRACE("(%d, %d, %p)\n", index, n, v );
  ENTER_GL();
  func_glVertexAttribs4hvNV( index, n, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs4svNV( GLuint index, GLsizei count, GLshort* v ) {
  void (*func_glVertexAttribs4svNV)( GLuint, GLsizei, GLshort* ) = extension_funcs[980];
  TRACE("(%d, %d, %p)\n", index, count, v );
  ENTER_GL();
  func_glVertexAttribs4svNV( index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs4ubvNV( GLuint index, GLsizei count, GLubyte* v ) {
  void (*func_glVertexAttribs4ubvNV)( GLuint, GLsizei, GLubyte* ) = extension_funcs[981];
  TRACE("(%d, %d, %p)\n", index, count, v );
  ENTER_GL();
  func_glVertexAttribs4ubvNV( index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexBlendARB( GLint count ) {
  void (*func_glVertexBlendARB)( GLint ) = extension_funcs[982];
  TRACE("(%d)\n", count );
  ENTER_GL();
  func_glVertexBlendARB( count );
  LEAVE_GL();
}

static void WINAPI wine_glVertexBlendEnvfATI( GLenum pname, GLfloat param ) {
  void (*func_glVertexBlendEnvfATI)( GLenum, GLfloat ) = extension_funcs[983];
  TRACE("(%d, %f)\n", pname, param );
  ENTER_GL();
  func_glVertexBlendEnvfATI( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glVertexBlendEnviATI( GLenum pname, GLint param ) {
  void (*func_glVertexBlendEnviATI)( GLenum, GLint ) = extension_funcs[984];
  TRACE("(%d, %d)\n", pname, param );
  ENTER_GL();
  func_glVertexBlendEnviATI( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glVertexPointerEXT( GLint size, GLenum type, GLsizei stride, GLsizei count, GLvoid* pointer ) {
  void (*func_glVertexPointerEXT)( GLint, GLenum, GLsizei, GLsizei, GLvoid* ) = extension_funcs[985];
  TRACE("(%d, %d, %d, %d, %p)\n", size, type, stride, count, pointer );
  ENTER_GL();
  func_glVertexPointerEXT( size, type, stride, count, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glVertexPointerListIBM( GLint size, GLenum type, GLint stride, GLvoid** pointer, GLint ptrstride ) {
  void (*func_glVertexPointerListIBM)( GLint, GLenum, GLint, GLvoid**, GLint ) = extension_funcs[986];
  TRACE("(%d, %d, %d, %p, %d)\n", size, type, stride, pointer, ptrstride );
  ENTER_GL();
  func_glVertexPointerListIBM( size, type, stride, pointer, ptrstride );
  LEAVE_GL();
}

static void WINAPI wine_glVertexPointervINTEL( GLint size, GLenum type, GLvoid** pointer ) {
  void (*func_glVertexPointervINTEL)( GLint, GLenum, GLvoid** ) = extension_funcs[987];
  TRACE("(%d, %d, %p)\n", size, type, pointer );
  ENTER_GL();
  func_glVertexPointervINTEL( size, type, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream1dATI( GLenum stream, GLdouble x ) {
  void (*func_glVertexStream1dATI)( GLenum, GLdouble ) = extension_funcs[988];
  TRACE("(%d, %f)\n", stream, x );
  ENTER_GL();
  func_glVertexStream1dATI( stream, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream1dvATI( GLenum stream, GLdouble* coords ) {
  void (*func_glVertexStream1dvATI)( GLenum, GLdouble* ) = extension_funcs[989];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream1dvATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream1fATI( GLenum stream, GLfloat x ) {
  void (*func_glVertexStream1fATI)( GLenum, GLfloat ) = extension_funcs[990];
  TRACE("(%d, %f)\n", stream, x );
  ENTER_GL();
  func_glVertexStream1fATI( stream, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream1fvATI( GLenum stream, GLfloat* coords ) {
  void (*func_glVertexStream1fvATI)( GLenum, GLfloat* ) = extension_funcs[991];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream1fvATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream1iATI( GLenum stream, GLint x ) {
  void (*func_glVertexStream1iATI)( GLenum, GLint ) = extension_funcs[992];
  TRACE("(%d, %d)\n", stream, x );
  ENTER_GL();
  func_glVertexStream1iATI( stream, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream1ivATI( GLenum stream, GLint* coords ) {
  void (*func_glVertexStream1ivATI)( GLenum, GLint* ) = extension_funcs[993];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream1ivATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream1sATI( GLenum stream, GLshort x ) {
  void (*func_glVertexStream1sATI)( GLenum, GLshort ) = extension_funcs[994];
  TRACE("(%d, %d)\n", stream, x );
  ENTER_GL();
  func_glVertexStream1sATI( stream, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream1svATI( GLenum stream, GLshort* coords ) {
  void (*func_glVertexStream1svATI)( GLenum, GLshort* ) = extension_funcs[995];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream1svATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream2dATI( GLenum stream, GLdouble x, GLdouble y ) {
  void (*func_glVertexStream2dATI)( GLenum, GLdouble, GLdouble ) = extension_funcs[996];
  TRACE("(%d, %f, %f)\n", stream, x, y );
  ENTER_GL();
  func_glVertexStream2dATI( stream, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream2dvATI( GLenum stream, GLdouble* coords ) {
  void (*func_glVertexStream2dvATI)( GLenum, GLdouble* ) = extension_funcs[997];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream2dvATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream2fATI( GLenum stream, GLfloat x, GLfloat y ) {
  void (*func_glVertexStream2fATI)( GLenum, GLfloat, GLfloat ) = extension_funcs[998];
  TRACE("(%d, %f, %f)\n", stream, x, y );
  ENTER_GL();
  func_glVertexStream2fATI( stream, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream2fvATI( GLenum stream, GLfloat* coords ) {
  void (*func_glVertexStream2fvATI)( GLenum, GLfloat* ) = extension_funcs[999];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream2fvATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream2iATI( GLenum stream, GLint x, GLint y ) {
  void (*func_glVertexStream2iATI)( GLenum, GLint, GLint ) = extension_funcs[1000];
  TRACE("(%d, %d, %d)\n", stream, x, y );
  ENTER_GL();
  func_glVertexStream2iATI( stream, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream2ivATI( GLenum stream, GLint* coords ) {
  void (*func_glVertexStream2ivATI)( GLenum, GLint* ) = extension_funcs[1001];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream2ivATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream2sATI( GLenum stream, GLshort x, GLshort y ) {
  void (*func_glVertexStream2sATI)( GLenum, GLshort, GLshort ) = extension_funcs[1002];
  TRACE("(%d, %d, %d)\n", stream, x, y );
  ENTER_GL();
  func_glVertexStream2sATI( stream, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream2svATI( GLenum stream, GLshort* coords ) {
  void (*func_glVertexStream2svATI)( GLenum, GLshort* ) = extension_funcs[1003];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream2svATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream3dATI( GLenum stream, GLdouble x, GLdouble y, GLdouble z ) {
  void (*func_glVertexStream3dATI)( GLenum, GLdouble, GLdouble, GLdouble ) = extension_funcs[1004];
  TRACE("(%d, %f, %f, %f)\n", stream, x, y, z );
  ENTER_GL();
  func_glVertexStream3dATI( stream, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream3dvATI( GLenum stream, GLdouble* coords ) {
  void (*func_glVertexStream3dvATI)( GLenum, GLdouble* ) = extension_funcs[1005];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream3dvATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream3fATI( GLenum stream, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glVertexStream3fATI)( GLenum, GLfloat, GLfloat, GLfloat ) = extension_funcs[1006];
  TRACE("(%d, %f, %f, %f)\n", stream, x, y, z );
  ENTER_GL();
  func_glVertexStream3fATI( stream, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream3fvATI( GLenum stream, GLfloat* coords ) {
  void (*func_glVertexStream3fvATI)( GLenum, GLfloat* ) = extension_funcs[1007];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream3fvATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream3iATI( GLenum stream, GLint x, GLint y, GLint z ) {
  void (*func_glVertexStream3iATI)( GLenum, GLint, GLint, GLint ) = extension_funcs[1008];
  TRACE("(%d, %d, %d, %d)\n", stream, x, y, z );
  ENTER_GL();
  func_glVertexStream3iATI( stream, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream3ivATI( GLenum stream, GLint* coords ) {
  void (*func_glVertexStream3ivATI)( GLenum, GLint* ) = extension_funcs[1009];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream3ivATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream3sATI( GLenum stream, GLshort x, GLshort y, GLshort z ) {
  void (*func_glVertexStream3sATI)( GLenum, GLshort, GLshort, GLshort ) = extension_funcs[1010];
  TRACE("(%d, %d, %d, %d)\n", stream, x, y, z );
  ENTER_GL();
  func_glVertexStream3sATI( stream, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream3svATI( GLenum stream, GLshort* coords ) {
  void (*func_glVertexStream3svATI)( GLenum, GLshort* ) = extension_funcs[1011];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream3svATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream4dATI( GLenum stream, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  void (*func_glVertexStream4dATI)( GLenum, GLdouble, GLdouble, GLdouble, GLdouble ) = extension_funcs[1012];
  TRACE("(%d, %f, %f, %f, %f)\n", stream, x, y, z, w );
  ENTER_GL();
  func_glVertexStream4dATI( stream, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream4dvATI( GLenum stream, GLdouble* coords ) {
  void (*func_glVertexStream4dvATI)( GLenum, GLdouble* ) = extension_funcs[1013];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream4dvATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream4fATI( GLenum stream, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  void (*func_glVertexStream4fATI)( GLenum, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[1014];
  TRACE("(%d, %f, %f, %f, %f)\n", stream, x, y, z, w );
  ENTER_GL();
  func_glVertexStream4fATI( stream, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream4fvATI( GLenum stream, GLfloat* coords ) {
  void (*func_glVertexStream4fvATI)( GLenum, GLfloat* ) = extension_funcs[1015];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream4fvATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream4iATI( GLenum stream, GLint x, GLint y, GLint z, GLint w ) {
  void (*func_glVertexStream4iATI)( GLenum, GLint, GLint, GLint, GLint ) = extension_funcs[1016];
  TRACE("(%d, %d, %d, %d, %d)\n", stream, x, y, z, w );
  ENTER_GL();
  func_glVertexStream4iATI( stream, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream4ivATI( GLenum stream, GLint* coords ) {
  void (*func_glVertexStream4ivATI)( GLenum, GLint* ) = extension_funcs[1017];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream4ivATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream4sATI( GLenum stream, GLshort x, GLshort y, GLshort z, GLshort w ) {
  void (*func_glVertexStream4sATI)( GLenum, GLshort, GLshort, GLshort, GLshort ) = extension_funcs[1018];
  TRACE("(%d, %d, %d, %d, %d)\n", stream, x, y, z, w );
  ENTER_GL();
  func_glVertexStream4sATI( stream, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream4svATI( GLenum stream, GLshort* coords ) {
  void (*func_glVertexStream4svATI)( GLenum, GLshort* ) = extension_funcs[1019];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream4svATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexWeightPointerEXT( GLsizei size, GLenum type, GLsizei stride, GLvoid* pointer ) {
  void (*func_glVertexWeightPointerEXT)( GLsizei, GLenum, GLsizei, GLvoid* ) = extension_funcs[1020];
  TRACE("(%d, %d, %d, %p)\n", size, type, stride, pointer );
  ENTER_GL();
  func_glVertexWeightPointerEXT( size, type, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glVertexWeightfEXT( GLfloat weight ) {
  void (*func_glVertexWeightfEXT)( GLfloat ) = extension_funcs[1021];
  TRACE("(%f)\n", weight );
  ENTER_GL();
  func_glVertexWeightfEXT( weight );
  LEAVE_GL();
}

static void WINAPI wine_glVertexWeightfvEXT( GLfloat* weight ) {
  void (*func_glVertexWeightfvEXT)( GLfloat* ) = extension_funcs[1022];
  TRACE("(%p)\n", weight );
  ENTER_GL();
  func_glVertexWeightfvEXT( weight );
  LEAVE_GL();
}

static void WINAPI wine_glVertexWeighthNV( unsigned short weight ) {
  void (*func_glVertexWeighthNV)( unsigned short ) = extension_funcs[1023];
  TRACE("(%d)\n", weight );
  ENTER_GL();
  func_glVertexWeighthNV( weight );
  LEAVE_GL();
}

static void WINAPI wine_glVertexWeighthvNV( unsigned short* weight ) {
  void (*func_glVertexWeighthvNV)( unsigned short* ) = extension_funcs[1024];
  TRACE("(%p)\n", weight );
  ENTER_GL();
  func_glVertexWeighthvNV( weight );
  LEAVE_GL();
}

static void WINAPI wine_glWeightPointerARB( GLint size, GLenum type, GLsizei stride, GLvoid* pointer ) {
  void (*func_glWeightPointerARB)( GLint, GLenum, GLsizei, GLvoid* ) = extension_funcs[1025];
  TRACE("(%d, %d, %d, %p)\n", size, type, stride, pointer );
  ENTER_GL();
  func_glWeightPointerARB( size, type, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glWeightbvARB( GLint size, GLbyte* weights ) {
  void (*func_glWeightbvARB)( GLint, GLbyte* ) = extension_funcs[1026];
  TRACE("(%d, %p)\n", size, weights );
  ENTER_GL();
  func_glWeightbvARB( size, weights );
  LEAVE_GL();
}

static void WINAPI wine_glWeightdvARB( GLint size, GLdouble* weights ) {
  void (*func_glWeightdvARB)( GLint, GLdouble* ) = extension_funcs[1027];
  TRACE("(%d, %p)\n", size, weights );
  ENTER_GL();
  func_glWeightdvARB( size, weights );
  LEAVE_GL();
}

static void WINAPI wine_glWeightfvARB( GLint size, GLfloat* weights ) {
  void (*func_glWeightfvARB)( GLint, GLfloat* ) = extension_funcs[1028];
  TRACE("(%d, %p)\n", size, weights );
  ENTER_GL();
  func_glWeightfvARB( size, weights );
  LEAVE_GL();
}

static void WINAPI wine_glWeightivARB( GLint size, GLint* weights ) {
  void (*func_glWeightivARB)( GLint, GLint* ) = extension_funcs[1029];
  TRACE("(%d, %p)\n", size, weights );
  ENTER_GL();
  func_glWeightivARB( size, weights );
  LEAVE_GL();
}

static void WINAPI wine_glWeightsvARB( GLint size, GLshort* weights ) {
  void (*func_glWeightsvARB)( GLint, GLshort* ) = extension_funcs[1030];
  TRACE("(%d, %p)\n", size, weights );
  ENTER_GL();
  func_glWeightsvARB( size, weights );
  LEAVE_GL();
}

static void WINAPI wine_glWeightubvARB( GLint size, GLubyte* weights ) {
  void (*func_glWeightubvARB)( GLint, GLubyte* ) = extension_funcs[1031];
  TRACE("(%d, %p)\n", size, weights );
  ENTER_GL();
  func_glWeightubvARB( size, weights );
  LEAVE_GL();
}

static void WINAPI wine_glWeightuivARB( GLint size, GLuint* weights ) {
  void (*func_glWeightuivARB)( GLint, GLuint* ) = extension_funcs[1032];
  TRACE("(%d, %p)\n", size, weights );
  ENTER_GL();
  func_glWeightuivARB( size, weights );
  LEAVE_GL();
}

static void WINAPI wine_glWeightusvARB( GLint size, GLushort* weights ) {
  void (*func_glWeightusvARB)( GLint, GLushort* ) = extension_funcs[1033];
  TRACE("(%d, %p)\n", size, weights );
  ENTER_GL();
  func_glWeightusvARB( size, weights );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2d( GLdouble x, GLdouble y ) {
  void (*func_glWindowPos2d)( GLdouble, GLdouble ) = extension_funcs[1034];
  TRACE("(%f, %f)\n", x, y );
  ENTER_GL();
  func_glWindowPos2d( x, y );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2dARB( GLdouble x, GLdouble y ) {
  void (*func_glWindowPos2dARB)( GLdouble, GLdouble ) = extension_funcs[1035];
  TRACE("(%f, %f)\n", x, y );
  ENTER_GL();
  func_glWindowPos2dARB( x, y );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2dMESA( GLdouble x, GLdouble y ) {
  void (*func_glWindowPos2dMESA)( GLdouble, GLdouble ) = extension_funcs[1036];
  TRACE("(%f, %f)\n", x, y );
  ENTER_GL();
  func_glWindowPos2dMESA( x, y );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2dv( GLdouble* v ) {
  void (*func_glWindowPos2dv)( GLdouble* ) = extension_funcs[1037];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos2dv( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2dvARB( GLdouble* v ) {
  void (*func_glWindowPos2dvARB)( GLdouble* ) = extension_funcs[1038];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos2dvARB( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2dvMESA( GLdouble* v ) {
  void (*func_glWindowPos2dvMESA)( GLdouble* ) = extension_funcs[1039];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos2dvMESA( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2f( GLfloat x, GLfloat y ) {
  void (*func_glWindowPos2f)( GLfloat, GLfloat ) = extension_funcs[1040];
  TRACE("(%f, %f)\n", x, y );
  ENTER_GL();
  func_glWindowPos2f( x, y );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2fARB( GLfloat x, GLfloat y ) {
  void (*func_glWindowPos2fARB)( GLfloat, GLfloat ) = extension_funcs[1041];
  TRACE("(%f, %f)\n", x, y );
  ENTER_GL();
  func_glWindowPos2fARB( x, y );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2fMESA( GLfloat x, GLfloat y ) {
  void (*func_glWindowPos2fMESA)( GLfloat, GLfloat ) = extension_funcs[1042];
  TRACE("(%f, %f)\n", x, y );
  ENTER_GL();
  func_glWindowPos2fMESA( x, y );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2fv( GLfloat* v ) {
  void (*func_glWindowPos2fv)( GLfloat* ) = extension_funcs[1043];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos2fv( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2fvARB( GLfloat* v ) {
  void (*func_glWindowPos2fvARB)( GLfloat* ) = extension_funcs[1044];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos2fvARB( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2fvMESA( GLfloat* v ) {
  void (*func_glWindowPos2fvMESA)( GLfloat* ) = extension_funcs[1045];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos2fvMESA( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2i( GLint x, GLint y ) {
  void (*func_glWindowPos2i)( GLint, GLint ) = extension_funcs[1046];
  TRACE("(%d, %d)\n", x, y );
  ENTER_GL();
  func_glWindowPos2i( x, y );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2iARB( GLint x, GLint y ) {
  void (*func_glWindowPos2iARB)( GLint, GLint ) = extension_funcs[1047];
  TRACE("(%d, %d)\n", x, y );
  ENTER_GL();
  func_glWindowPos2iARB( x, y );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2iMESA( GLint x, GLint y ) {
  void (*func_glWindowPos2iMESA)( GLint, GLint ) = extension_funcs[1048];
  TRACE("(%d, %d)\n", x, y );
  ENTER_GL();
  func_glWindowPos2iMESA( x, y );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2iv( GLint* v ) {
  void (*func_glWindowPos2iv)( GLint* ) = extension_funcs[1049];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos2iv( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2ivARB( GLint* v ) {
  void (*func_glWindowPos2ivARB)( GLint* ) = extension_funcs[1050];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos2ivARB( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2ivMESA( GLint* v ) {
  void (*func_glWindowPos2ivMESA)( GLint* ) = extension_funcs[1051];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos2ivMESA( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2s( GLshort x, GLshort y ) {
  void (*func_glWindowPos2s)( GLshort, GLshort ) = extension_funcs[1052];
  TRACE("(%d, %d)\n", x, y );
  ENTER_GL();
  func_glWindowPos2s( x, y );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2sARB( GLshort x, GLshort y ) {
  void (*func_glWindowPos2sARB)( GLshort, GLshort ) = extension_funcs[1053];
  TRACE("(%d, %d)\n", x, y );
  ENTER_GL();
  func_glWindowPos2sARB( x, y );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2sMESA( GLshort x, GLshort y ) {
  void (*func_glWindowPos2sMESA)( GLshort, GLshort ) = extension_funcs[1054];
  TRACE("(%d, %d)\n", x, y );
  ENTER_GL();
  func_glWindowPos2sMESA( x, y );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2sv( GLshort* v ) {
  void (*func_glWindowPos2sv)( GLshort* ) = extension_funcs[1055];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos2sv( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2svARB( GLshort* v ) {
  void (*func_glWindowPos2svARB)( GLshort* ) = extension_funcs[1056];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos2svARB( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2svMESA( GLshort* v ) {
  void (*func_glWindowPos2svMESA)( GLshort* ) = extension_funcs[1057];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos2svMESA( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3d( GLdouble x, GLdouble y, GLdouble z ) {
  void (*func_glWindowPos3d)( GLdouble, GLdouble, GLdouble ) = extension_funcs[1058];
  TRACE("(%f, %f, %f)\n", x, y, z );
  ENTER_GL();
  func_glWindowPos3d( x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3dARB( GLdouble x, GLdouble y, GLdouble z ) {
  void (*func_glWindowPos3dARB)( GLdouble, GLdouble, GLdouble ) = extension_funcs[1059];
  TRACE("(%f, %f, %f)\n", x, y, z );
  ENTER_GL();
  func_glWindowPos3dARB( x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3dMESA( GLdouble x, GLdouble y, GLdouble z ) {
  void (*func_glWindowPos3dMESA)( GLdouble, GLdouble, GLdouble ) = extension_funcs[1060];
  TRACE("(%f, %f, %f)\n", x, y, z );
  ENTER_GL();
  func_glWindowPos3dMESA( x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3dv( GLdouble* v ) {
  void (*func_glWindowPos3dv)( GLdouble* ) = extension_funcs[1061];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos3dv( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3dvARB( GLdouble* v ) {
  void (*func_glWindowPos3dvARB)( GLdouble* ) = extension_funcs[1062];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos3dvARB( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3dvMESA( GLdouble* v ) {
  void (*func_glWindowPos3dvMESA)( GLdouble* ) = extension_funcs[1063];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos3dvMESA( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3f( GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glWindowPos3f)( GLfloat, GLfloat, GLfloat ) = extension_funcs[1064];
  TRACE("(%f, %f, %f)\n", x, y, z );
  ENTER_GL();
  func_glWindowPos3f( x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3fARB( GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glWindowPos3fARB)( GLfloat, GLfloat, GLfloat ) = extension_funcs[1065];
  TRACE("(%f, %f, %f)\n", x, y, z );
  ENTER_GL();
  func_glWindowPos3fARB( x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3fMESA( GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glWindowPos3fMESA)( GLfloat, GLfloat, GLfloat ) = extension_funcs[1066];
  TRACE("(%f, %f, %f)\n", x, y, z );
  ENTER_GL();
  func_glWindowPos3fMESA( x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3fv( GLfloat* v ) {
  void (*func_glWindowPos3fv)( GLfloat* ) = extension_funcs[1067];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos3fv( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3fvARB( GLfloat* v ) {
  void (*func_glWindowPos3fvARB)( GLfloat* ) = extension_funcs[1068];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos3fvARB( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3fvMESA( GLfloat* v ) {
  void (*func_glWindowPos3fvMESA)( GLfloat* ) = extension_funcs[1069];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos3fvMESA( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3i( GLint x, GLint y, GLint z ) {
  void (*func_glWindowPos3i)( GLint, GLint, GLint ) = extension_funcs[1070];
  TRACE("(%d, %d, %d)\n", x, y, z );
  ENTER_GL();
  func_glWindowPos3i( x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3iARB( GLint x, GLint y, GLint z ) {
  void (*func_glWindowPos3iARB)( GLint, GLint, GLint ) = extension_funcs[1071];
  TRACE("(%d, %d, %d)\n", x, y, z );
  ENTER_GL();
  func_glWindowPos3iARB( x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3iMESA( GLint x, GLint y, GLint z ) {
  void (*func_glWindowPos3iMESA)( GLint, GLint, GLint ) = extension_funcs[1072];
  TRACE("(%d, %d, %d)\n", x, y, z );
  ENTER_GL();
  func_glWindowPos3iMESA( x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3iv( GLint* v ) {
  void (*func_glWindowPos3iv)( GLint* ) = extension_funcs[1073];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos3iv( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3ivARB( GLint* v ) {
  void (*func_glWindowPos3ivARB)( GLint* ) = extension_funcs[1074];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos3ivARB( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3ivMESA( GLint* v ) {
  void (*func_glWindowPos3ivMESA)( GLint* ) = extension_funcs[1075];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos3ivMESA( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3s( GLshort x, GLshort y, GLshort z ) {
  void (*func_glWindowPos3s)( GLshort, GLshort, GLshort ) = extension_funcs[1076];
  TRACE("(%d, %d, %d)\n", x, y, z );
  ENTER_GL();
  func_glWindowPos3s( x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3sARB( GLshort x, GLshort y, GLshort z ) {
  void (*func_glWindowPos3sARB)( GLshort, GLshort, GLshort ) = extension_funcs[1077];
  TRACE("(%d, %d, %d)\n", x, y, z );
  ENTER_GL();
  func_glWindowPos3sARB( x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3sMESA( GLshort x, GLshort y, GLshort z ) {
  void (*func_glWindowPos3sMESA)( GLshort, GLshort, GLshort ) = extension_funcs[1078];
  TRACE("(%d, %d, %d)\n", x, y, z );
  ENTER_GL();
  func_glWindowPos3sMESA( x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3sv( GLshort* v ) {
  void (*func_glWindowPos3sv)( GLshort* ) = extension_funcs[1079];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos3sv( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3svARB( GLshort* v ) {
  void (*func_glWindowPos3svARB)( GLshort* ) = extension_funcs[1080];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos3svARB( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3svMESA( GLshort* v ) {
  void (*func_glWindowPos3svMESA)( GLshort* ) = extension_funcs[1081];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos3svMESA( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos4dMESA( GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  void (*func_glWindowPos4dMESA)( GLdouble, GLdouble, GLdouble, GLdouble ) = extension_funcs[1082];
  TRACE("(%f, %f, %f, %f)\n", x, y, z, w );
  ENTER_GL();
  func_glWindowPos4dMESA( x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos4dvMESA( GLdouble* v ) {
  void (*func_glWindowPos4dvMESA)( GLdouble* ) = extension_funcs[1083];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos4dvMESA( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos4fMESA( GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  void (*func_glWindowPos4fMESA)( GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[1084];
  TRACE("(%f, %f, %f, %f)\n", x, y, z, w );
  ENTER_GL();
  func_glWindowPos4fMESA( x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos4fvMESA( GLfloat* v ) {
  void (*func_glWindowPos4fvMESA)( GLfloat* ) = extension_funcs[1085];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos4fvMESA( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos4iMESA( GLint x, GLint y, GLint z, GLint w ) {
  void (*func_glWindowPos4iMESA)( GLint, GLint, GLint, GLint ) = extension_funcs[1086];
  TRACE("(%d, %d, %d, %d)\n", x, y, z, w );
  ENTER_GL();
  func_glWindowPos4iMESA( x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos4ivMESA( GLint* v ) {
  void (*func_glWindowPos4ivMESA)( GLint* ) = extension_funcs[1087];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos4ivMESA( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos4sMESA( GLshort x, GLshort y, GLshort z, GLshort w ) {
  void (*func_glWindowPos4sMESA)( GLshort, GLshort, GLshort, GLshort ) = extension_funcs[1088];
  TRACE("(%d, %d, %d, %d)\n", x, y, z, w );
  ENTER_GL();
  func_glWindowPos4sMESA( x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos4svMESA( GLshort* v ) {
  void (*func_glWindowPos4svMESA)( GLshort* ) = extension_funcs[1089];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos4svMESA( v );
  LEAVE_GL();
}

static void WINAPI wine_glWriteMaskEXT( GLuint res, GLuint in, GLenum outX, GLenum outY, GLenum outZ, GLenum outW ) {
  void (*func_glWriteMaskEXT)( GLuint, GLuint, GLenum, GLenum, GLenum, GLenum ) = extension_funcs[1090];
  TRACE("(%d, %d, %d, %d, %d, %d)\n", res, in, outX, outY, outZ, outW );
  ENTER_GL();
  func_glWriteMaskEXT( res, in, outX, outY, outZ, outW );
  LEAVE_GL();
}

static void * WINAPI wine_wglAllocateMemoryNV( GLsizei size, GLfloat readfreq, GLfloat writefreq, GLfloat priority ) {
  void * ret_value;
  void * (*func_wglAllocateMemoryNV)( GLsizei, GLfloat, GLfloat, GLfloat ) = extension_funcs[1091];
  TRACE("(%d, %f, %f, %f)\n", size, readfreq, writefreq, priority );
  ENTER_GL();
  ret_value = func_wglAllocateMemoryNV( size, readfreq, writefreq, priority );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_wglFreeMemoryNV( GLvoid * pointer ) {
  void (*func_wglFreeMemoryNV)( GLvoid * ) = extension_funcs[1092];
  TRACE("(%p)\n", pointer );
  ENTER_GL();
  func_wglFreeMemoryNV( pointer );
  LEAVE_GL();
}


/* The table giving the correspondance between names and functions */
const OpenGL_extension extension_registry[1093] = {
  { "glActiveStencilFaceEXT", NULL, (void *) wine_glActiveStencilFaceEXT },
  { "glActiveTexture", NULL, (void *) wine_glActiveTexture },
  { "glActiveTextureARB", NULL, (void *) wine_glActiveTextureARB },
  { "glAlphaFragmentOp1ATI", NULL, (void *) wine_glAlphaFragmentOp1ATI },
  { "glAlphaFragmentOp2ATI", NULL, (void *) wine_glAlphaFragmentOp2ATI },
  { "glAlphaFragmentOp3ATI", NULL, (void *) wine_glAlphaFragmentOp3ATI },
  { "glApplyTextureEXT", NULL, (void *) wine_glApplyTextureEXT },
  { "glAreProgramsResidentNV", NULL, (void *) wine_glAreProgramsResidentNV },
  { "glAreTexturesResidentEXT", NULL, (void *) wine_glAreTexturesResidentEXT },
  { "glArrayElementEXT", NULL, (void *) wine_glArrayElementEXT },
  { "glArrayObjectATI", NULL, (void *) wine_glArrayObjectATI },
  { "glAsyncMarkerSGIX", NULL, (void *) wine_glAsyncMarkerSGIX },
  { "glAttachObjectARB", NULL, (void *) wine_glAttachObjectARB },
  { "glAttachShader", NULL, (void *) wine_glAttachShader },
  { "glBeginFragmentShaderATI", NULL, (void *) wine_glBeginFragmentShaderATI },
  { "glBeginOcclusionQueryNV", NULL, (void *) wine_glBeginOcclusionQueryNV },
  { "glBeginQuery", NULL, (void *) wine_glBeginQuery },
  { "glBeginQueryARB", NULL, (void *) wine_glBeginQueryARB },
  { "glBeginVertexShaderEXT", NULL, (void *) wine_glBeginVertexShaderEXT },
  { "glBindAttribLocation", NULL, (void *) wine_glBindAttribLocation },
  { "glBindAttribLocationARB", NULL, (void *) wine_glBindAttribLocationARB },
  { "glBindBuffer", NULL, (void *) wine_glBindBuffer },
  { "glBindBufferARB", NULL, (void *) wine_glBindBufferARB },
  { "glBindFragmentShaderATI", NULL, (void *) wine_glBindFragmentShaderATI },
  { "glBindFramebufferEXT", NULL, (void *) wine_glBindFramebufferEXT },
  { "glBindLightParameterEXT", NULL, (void *) wine_glBindLightParameterEXT },
  { "glBindMaterialParameterEXT", NULL, (void *) wine_glBindMaterialParameterEXT },
  { "glBindParameterEXT", NULL, (void *) wine_glBindParameterEXT },
  { "glBindProgramARB", NULL, (void *) wine_glBindProgramARB },
  { "glBindProgramNV", NULL, (void *) wine_glBindProgramNV },
  { "glBindRenderbufferEXT", NULL, (void *) wine_glBindRenderbufferEXT },
  { "glBindTexGenParameterEXT", NULL, (void *) wine_glBindTexGenParameterEXT },
  { "glBindTextureEXT", NULL, (void *) wine_glBindTextureEXT },
  { "glBindTextureUnitParameterEXT", NULL, (void *) wine_glBindTextureUnitParameterEXT },
  { "glBindVertexArrayAPPLE", NULL, (void *) wine_glBindVertexArrayAPPLE },
  { "glBindVertexShaderEXT", NULL, (void *) wine_glBindVertexShaderEXT },
  { "glBinormal3bEXT", NULL, (void *) wine_glBinormal3bEXT },
  { "glBinormal3bvEXT", NULL, (void *) wine_glBinormal3bvEXT },
  { "glBinormal3dEXT", NULL, (void *) wine_glBinormal3dEXT },
  { "glBinormal3dvEXT", NULL, (void *) wine_glBinormal3dvEXT },
  { "glBinormal3fEXT", NULL, (void *) wine_glBinormal3fEXT },
  { "glBinormal3fvEXT", NULL, (void *) wine_glBinormal3fvEXT },
  { "glBinormal3iEXT", NULL, (void *) wine_glBinormal3iEXT },
  { "glBinormal3ivEXT", NULL, (void *) wine_glBinormal3ivEXT },
  { "glBinormal3sEXT", NULL, (void *) wine_glBinormal3sEXT },
  { "glBinormal3svEXT", NULL, (void *) wine_glBinormal3svEXT },
  { "glBinormalPointerEXT", NULL, (void *) wine_glBinormalPointerEXT },
  { "glBlendColorEXT", NULL, (void *) wine_glBlendColorEXT },
  { "glBlendEquationEXT", NULL, (void *) wine_glBlendEquationEXT },
  { "glBlendEquationSeparate", NULL, (void *) wine_glBlendEquationSeparate },
  { "glBlendEquationSeparateEXT", NULL, (void *) wine_glBlendEquationSeparateEXT },
  { "glBlendFuncSeparate", NULL, (void *) wine_glBlendFuncSeparate },
  { "glBlendFuncSeparateEXT", NULL, (void *) wine_glBlendFuncSeparateEXT },
  { "glBlendFuncSeparateINGR", NULL, (void *) wine_glBlendFuncSeparateINGR },
  { "glBufferData", NULL, (void *) wine_glBufferData },
  { "glBufferDataARB", NULL, (void *) wine_glBufferDataARB },
  { "glBufferRegionEnabled", NULL, (void *) wine_glBufferRegionEnabled },
  { "glBufferSubData", NULL, (void *) wine_glBufferSubData },
  { "glBufferSubDataARB", NULL, (void *) wine_glBufferSubDataARB },
  { "glCheckFramebufferStatusEXT", NULL, (void *) wine_glCheckFramebufferStatusEXT },
  { "glClampColorARB", NULL, (void *) wine_glClampColorARB },
  { "glClientActiveTexture", NULL, (void *) wine_glClientActiveTexture },
  { "glClientActiveTextureARB", NULL, (void *) wine_glClientActiveTextureARB },
  { "glClientActiveVertexStreamATI", NULL, (void *) wine_glClientActiveVertexStreamATI },
  { "glColor3fVertex3fSUN", NULL, (void *) wine_glColor3fVertex3fSUN },
  { "glColor3fVertex3fvSUN", NULL, (void *) wine_glColor3fVertex3fvSUN },
  { "glColor3hNV", NULL, (void *) wine_glColor3hNV },
  { "glColor3hvNV", NULL, (void *) wine_glColor3hvNV },
  { "glColor4fNormal3fVertex3fSUN", NULL, (void *) wine_glColor4fNormal3fVertex3fSUN },
  { "glColor4fNormal3fVertex3fvSUN", NULL, (void *) wine_glColor4fNormal3fVertex3fvSUN },
  { "glColor4hNV", NULL, (void *) wine_glColor4hNV },
  { "glColor4hvNV", NULL, (void *) wine_glColor4hvNV },
  { "glColor4ubVertex2fSUN", NULL, (void *) wine_glColor4ubVertex2fSUN },
  { "glColor4ubVertex2fvSUN", NULL, (void *) wine_glColor4ubVertex2fvSUN },
  { "glColor4ubVertex3fSUN", NULL, (void *) wine_glColor4ubVertex3fSUN },
  { "glColor4ubVertex3fvSUN", NULL, (void *) wine_glColor4ubVertex3fvSUN },
  { "glColorFragmentOp1ATI", NULL, (void *) wine_glColorFragmentOp1ATI },
  { "glColorFragmentOp2ATI", NULL, (void *) wine_glColorFragmentOp2ATI },
  { "glColorFragmentOp3ATI", NULL, (void *) wine_glColorFragmentOp3ATI },
  { "glColorPointerEXT", NULL, (void *) wine_glColorPointerEXT },
  { "glColorPointerListIBM", NULL, (void *) wine_glColorPointerListIBM },
  { "glColorPointervINTEL", NULL, (void *) wine_glColorPointervINTEL },
  { "glColorSubTableEXT", NULL, (void *) wine_glColorSubTableEXT },
  { "glColorTableEXT", NULL, (void *) wine_glColorTableEXT },
  { "glColorTableParameterfvSGI", NULL, (void *) wine_glColorTableParameterfvSGI },
  { "glColorTableParameterivSGI", NULL, (void *) wine_glColorTableParameterivSGI },
  { "glColorTableSGI", NULL, (void *) wine_glColorTableSGI },
  { "glCombinerInputNV", NULL, (void *) wine_glCombinerInputNV },
  { "glCombinerOutputNV", NULL, (void *) wine_glCombinerOutputNV },
  { "glCombinerParameterfNV", NULL, (void *) wine_glCombinerParameterfNV },
  { "glCombinerParameterfvNV", NULL, (void *) wine_glCombinerParameterfvNV },
  { "glCombinerParameteriNV", NULL, (void *) wine_glCombinerParameteriNV },
  { "glCombinerParameterivNV", NULL, (void *) wine_glCombinerParameterivNV },
  { "glCombinerStageParameterfvNV", NULL, (void *) wine_glCombinerStageParameterfvNV },
  { "glCompileShader", NULL, (void *) wine_glCompileShader },
  { "glCompileShaderARB", NULL, (void *) wine_glCompileShaderARB },
  { "glCompressedTexImage1D", NULL, (void *) wine_glCompressedTexImage1D },
  { "glCompressedTexImage1DARB", NULL, (void *) wine_glCompressedTexImage1DARB },
  { "glCompressedTexImage2D", NULL, (void *) wine_glCompressedTexImage2D },
  { "glCompressedTexImage2DARB", NULL, (void *) wine_glCompressedTexImage2DARB },
  { "glCompressedTexImage3D", NULL, (void *) wine_glCompressedTexImage3D },
  { "glCompressedTexImage3DARB", NULL, (void *) wine_glCompressedTexImage3DARB },
  { "glCompressedTexSubImage1D", NULL, (void *) wine_glCompressedTexSubImage1D },
  { "glCompressedTexSubImage1DARB", NULL, (void *) wine_glCompressedTexSubImage1DARB },
  { "glCompressedTexSubImage2D", NULL, (void *) wine_glCompressedTexSubImage2D },
  { "glCompressedTexSubImage2DARB", NULL, (void *) wine_glCompressedTexSubImage2DARB },
  { "glCompressedTexSubImage3D", NULL, (void *) wine_glCompressedTexSubImage3D },
  { "glCompressedTexSubImage3DARB", NULL, (void *) wine_glCompressedTexSubImage3DARB },
  { "glConvolutionFilter1DEXT", NULL, (void *) wine_glConvolutionFilter1DEXT },
  { "glConvolutionFilter2DEXT", NULL, (void *) wine_glConvolutionFilter2DEXT },
  { "glConvolutionParameterfEXT", NULL, (void *) wine_glConvolutionParameterfEXT },
  { "glConvolutionParameterfvEXT", NULL, (void *) wine_glConvolutionParameterfvEXT },
  { "glConvolutionParameteriEXT", NULL, (void *) wine_glConvolutionParameteriEXT },
  { "glConvolutionParameterivEXT", NULL, (void *) wine_glConvolutionParameterivEXT },
  { "glCopyColorSubTableEXT", NULL, (void *) wine_glCopyColorSubTableEXT },
  { "glCopyColorTableSGI", NULL, (void *) wine_glCopyColorTableSGI },
  { "glCopyConvolutionFilter1DEXT", NULL, (void *) wine_glCopyConvolutionFilter1DEXT },
  { "glCopyConvolutionFilter2DEXT", NULL, (void *) wine_glCopyConvolutionFilter2DEXT },
  { "glCopyTexImage1DEXT", NULL, (void *) wine_glCopyTexImage1DEXT },
  { "glCopyTexImage2DEXT", NULL, (void *) wine_glCopyTexImage2DEXT },
  { "glCopyTexSubImage1DEXT", NULL, (void *) wine_glCopyTexSubImage1DEXT },
  { "glCopyTexSubImage2DEXT", NULL, (void *) wine_glCopyTexSubImage2DEXT },
  { "glCopyTexSubImage3DEXT", NULL, (void *) wine_glCopyTexSubImage3DEXT },
  { "glCreateProgram", NULL, (void *) wine_glCreateProgram },
  { "glCreateProgramObjectARB", NULL, (void *) wine_glCreateProgramObjectARB },
  { "glCreateShader", NULL, (void *) wine_glCreateShader },
  { "glCreateShaderObjectARB", NULL, (void *) wine_glCreateShaderObjectARB },
  { "glCullParameterdvEXT", NULL, (void *) wine_glCullParameterdvEXT },
  { "glCullParameterfvEXT", NULL, (void *) wine_glCullParameterfvEXT },
  { "glCurrentPaletteMatrixARB", NULL, (void *) wine_glCurrentPaletteMatrixARB },
  { "glDeformSGIX", NULL, (void *) wine_glDeformSGIX },
  { "glDeformationMap3dSGIX", NULL, (void *) wine_glDeformationMap3dSGIX },
  { "glDeformationMap3fSGIX", NULL, (void *) wine_glDeformationMap3fSGIX },
  { "glDeleteAsyncMarkersSGIX", NULL, (void *) wine_glDeleteAsyncMarkersSGIX },
  { "glDeleteBufferRegion", NULL, (void *) wine_glDeleteBufferRegion },
  { "glDeleteBuffers", NULL, (void *) wine_glDeleteBuffers },
  { "glDeleteBuffersARB", NULL, (void *) wine_glDeleteBuffersARB },
  { "glDeleteFencesAPPLE", NULL, (void *) wine_glDeleteFencesAPPLE },
  { "glDeleteFencesNV", NULL, (void *) wine_glDeleteFencesNV },
  { "glDeleteFragmentShaderATI", NULL, (void *) wine_glDeleteFragmentShaderATI },
  { "glDeleteFramebuffersEXT", NULL, (void *) wine_glDeleteFramebuffersEXT },
  { "glDeleteObjectARB", NULL, (void *) wine_glDeleteObjectARB },
  { "glDeleteObjectBufferATI", NULL, (void *) wine_glDeleteObjectBufferATI },
  { "glDeleteOcclusionQueriesNV", NULL, (void *) wine_glDeleteOcclusionQueriesNV },
  { "glDeleteProgram", NULL, (void *) wine_glDeleteProgram },
  { "glDeleteProgramsARB", NULL, (void *) wine_glDeleteProgramsARB },
  { "glDeleteProgramsNV", NULL, (void *) wine_glDeleteProgramsNV },
  { "glDeleteQueries", NULL, (void *) wine_glDeleteQueries },
  { "glDeleteQueriesARB", NULL, (void *) wine_glDeleteQueriesARB },
  { "glDeleteRenderbuffersEXT", NULL, (void *) wine_glDeleteRenderbuffersEXT },
  { "glDeleteShader", NULL, (void *) wine_glDeleteShader },
  { "glDeleteTexturesEXT", NULL, (void *) wine_glDeleteTexturesEXT },
  { "glDeleteVertexArraysAPPLE", NULL, (void *) wine_glDeleteVertexArraysAPPLE },
  { "glDeleteVertexShaderEXT", NULL, (void *) wine_glDeleteVertexShaderEXT },
  { "glDepthBoundsEXT", NULL, (void *) wine_glDepthBoundsEXT },
  { "glDetachObjectARB", NULL, (void *) wine_glDetachObjectARB },
  { "glDetachShader", NULL, (void *) wine_glDetachShader },
  { "glDetailTexFuncSGIS", NULL, (void *) wine_glDetailTexFuncSGIS },
  { "glDisableVariantClientStateEXT", NULL, (void *) wine_glDisableVariantClientStateEXT },
  { "glDisableVertexAttribArray", NULL, (void *) wine_glDisableVertexAttribArray },
  { "glDisableVertexAttribArrayARB", NULL, (void *) wine_glDisableVertexAttribArrayARB },
  { "glDrawArraysEXT", NULL, (void *) wine_glDrawArraysEXT },
  { "glDrawBufferRegion", NULL, (void *) wine_glDrawBufferRegion },
  { "glDrawBuffers", NULL, (void *) wine_glDrawBuffers },
  { "glDrawBuffersARB", NULL, (void *) wine_glDrawBuffersARB },
  { "glDrawBuffersATI", NULL, (void *) wine_glDrawBuffersATI },
  { "glDrawElementArrayAPPLE", NULL, (void *) wine_glDrawElementArrayAPPLE },
  { "glDrawElementArrayATI", NULL, (void *) wine_glDrawElementArrayATI },
  { "glDrawMeshArraysSUN", NULL, (void *) wine_glDrawMeshArraysSUN },
  { "glDrawRangeElementArrayAPPLE", NULL, (void *) wine_glDrawRangeElementArrayAPPLE },
  { "glDrawRangeElementArrayATI", NULL, (void *) wine_glDrawRangeElementArrayATI },
  { "glDrawRangeElementsEXT", NULL, (void *) wine_glDrawRangeElementsEXT },
  { "glEdgeFlagPointerEXT", NULL, (void *) wine_glEdgeFlagPointerEXT },
  { "glEdgeFlagPointerListIBM", NULL, (void *) wine_glEdgeFlagPointerListIBM },
  { "glElementPointerAPPLE", NULL, (void *) wine_glElementPointerAPPLE },
  { "glElementPointerATI", NULL, (void *) wine_glElementPointerATI },
  { "glEnableVariantClientStateEXT", NULL, (void *) wine_glEnableVariantClientStateEXT },
  { "glEnableVertexAttribArray", NULL, (void *) wine_glEnableVertexAttribArray },
  { "glEnableVertexAttribArrayARB", NULL, (void *) wine_glEnableVertexAttribArrayARB },
  { "glEndFragmentShaderATI", NULL, (void *) wine_glEndFragmentShaderATI },
  { "glEndOcclusionQueryNV", NULL, (void *) wine_glEndOcclusionQueryNV },
  { "glEndQuery", NULL, (void *) wine_glEndQuery },
  { "glEndQueryARB", NULL, (void *) wine_glEndQueryARB },
  { "glEndVertexShaderEXT", NULL, (void *) wine_glEndVertexShaderEXT },
  { "glEvalMapsNV", NULL, (void *) wine_glEvalMapsNV },
  { "glExecuteProgramNV", NULL, (void *) wine_glExecuteProgramNV },
  { "glExtractComponentEXT", NULL, (void *) wine_glExtractComponentEXT },
  { "glFinalCombinerInputNV", NULL, (void *) wine_glFinalCombinerInputNV },
  { "glFinishAsyncSGIX", NULL, (void *) wine_glFinishAsyncSGIX },
  { "glFinishFenceAPPLE", NULL, (void *) wine_glFinishFenceAPPLE },
  { "glFinishFenceNV", NULL, (void *) wine_glFinishFenceNV },
  { "glFinishObjectAPPLE", NULL, (void *) wine_glFinishObjectAPPLE },
  { "glFinishTextureSUNX", NULL, (void *) wine_glFinishTextureSUNX },
  { "glFlushPixelDataRangeNV", NULL, (void *) wine_glFlushPixelDataRangeNV },
  { "glFlushRasterSGIX", NULL, (void *) wine_glFlushRasterSGIX },
  { "glFlushVertexArrayRangeAPPLE", NULL, (void *) wine_glFlushVertexArrayRangeAPPLE },
  { "glFlushVertexArrayRangeNV", NULL, (void *) wine_glFlushVertexArrayRangeNV },
  { "glFogCoordPointer", NULL, (void *) wine_glFogCoordPointer },
  { "glFogCoordPointerEXT", NULL, (void *) wine_glFogCoordPointerEXT },
  { "glFogCoordPointerListIBM", NULL, (void *) wine_glFogCoordPointerListIBM },
  { "glFogCoordd", NULL, (void *) wine_glFogCoordd },
  { "glFogCoorddEXT", NULL, (void *) wine_glFogCoorddEXT },
  { "glFogCoorddv", NULL, (void *) wine_glFogCoorddv },
  { "glFogCoorddvEXT", NULL, (void *) wine_glFogCoorddvEXT },
  { "glFogCoordf", NULL, (void *) wine_glFogCoordf },
  { "glFogCoordfEXT", NULL, (void *) wine_glFogCoordfEXT },
  { "glFogCoordfv", NULL, (void *) wine_glFogCoordfv },
  { "glFogCoordfvEXT", NULL, (void *) wine_glFogCoordfvEXT },
  { "glFogCoordhNV", NULL, (void *) wine_glFogCoordhNV },
  { "glFogCoordhvNV", NULL, (void *) wine_glFogCoordhvNV },
  { "glFogFuncSGIS", NULL, (void *) wine_glFogFuncSGIS },
  { "glFragmentColorMaterialSGIX", NULL, (void *) wine_glFragmentColorMaterialSGIX },
  { "glFragmentLightModelfSGIX", NULL, (void *) wine_glFragmentLightModelfSGIX },
  { "glFragmentLightModelfvSGIX", NULL, (void *) wine_glFragmentLightModelfvSGIX },
  { "glFragmentLightModeliSGIX", NULL, (void *) wine_glFragmentLightModeliSGIX },
  { "glFragmentLightModelivSGIX", NULL, (void *) wine_glFragmentLightModelivSGIX },
  { "glFragmentLightfSGIX", NULL, (void *) wine_glFragmentLightfSGIX },
  { "glFragmentLightfvSGIX", NULL, (void *) wine_glFragmentLightfvSGIX },
  { "glFragmentLightiSGIX", NULL, (void *) wine_glFragmentLightiSGIX },
  { "glFragmentLightivSGIX", NULL, (void *) wine_glFragmentLightivSGIX },
  { "glFragmentMaterialfSGIX", NULL, (void *) wine_glFragmentMaterialfSGIX },
  { "glFragmentMaterialfvSGIX", NULL, (void *) wine_glFragmentMaterialfvSGIX },
  { "glFragmentMaterialiSGIX", NULL, (void *) wine_glFragmentMaterialiSGIX },
  { "glFragmentMaterialivSGIX", NULL, (void *) wine_glFragmentMaterialivSGIX },
  { "glFrameZoomSGIX", NULL, (void *) wine_glFrameZoomSGIX },
  { "glFramebufferRenderbufferEXT", NULL, (void *) wine_glFramebufferRenderbufferEXT },
  { "glFramebufferTexture1DEXT", NULL, (void *) wine_glFramebufferTexture1DEXT },
  { "glFramebufferTexture2DEXT", NULL, (void *) wine_glFramebufferTexture2DEXT },
  { "glFramebufferTexture3DEXT", NULL, (void *) wine_glFramebufferTexture3DEXT },
  { "glFreeObjectBufferATI", NULL, (void *) wine_glFreeObjectBufferATI },
  { "glGenAsyncMarkersSGIX", NULL, (void *) wine_glGenAsyncMarkersSGIX },
  { "glGenBuffers", NULL, (void *) wine_glGenBuffers },
  { "glGenBuffersARB", NULL, (void *) wine_glGenBuffersARB },
  { "glGenFencesAPPLE", NULL, (void *) wine_glGenFencesAPPLE },
  { "glGenFencesNV", NULL, (void *) wine_glGenFencesNV },
  { "glGenFragmentShadersATI", NULL, (void *) wine_glGenFragmentShadersATI },
  { "glGenFramebuffersEXT", NULL, (void *) wine_glGenFramebuffersEXT },
  { "glGenOcclusionQueriesNV", NULL, (void *) wine_glGenOcclusionQueriesNV },
  { "glGenProgramsARB", NULL, (void *) wine_glGenProgramsARB },
  { "glGenProgramsNV", NULL, (void *) wine_glGenProgramsNV },
  { "glGenQueries", NULL, (void *) wine_glGenQueries },
  { "glGenQueriesARB", NULL, (void *) wine_glGenQueriesARB },
  { "glGenRenderbuffersEXT", NULL, (void *) wine_glGenRenderbuffersEXT },
  { "glGenSymbolsEXT", NULL, (void *) wine_glGenSymbolsEXT },
  { "glGenTexturesEXT", NULL, (void *) wine_glGenTexturesEXT },
  { "glGenVertexArraysAPPLE", NULL, (void *) wine_glGenVertexArraysAPPLE },
  { "glGenVertexShadersEXT", NULL, (void *) wine_glGenVertexShadersEXT },
  { "glGenerateMipmapEXT", NULL, (void *) wine_glGenerateMipmapEXT },
  { "glGetActiveAttrib", NULL, (void *) wine_glGetActiveAttrib },
  { "glGetActiveAttribARB", NULL, (void *) wine_glGetActiveAttribARB },
  { "glGetActiveUniform", NULL, (void *) wine_glGetActiveUniform },
  { "glGetActiveUniformARB", NULL, (void *) wine_glGetActiveUniformARB },
  { "glGetArrayObjectfvATI", NULL, (void *) wine_glGetArrayObjectfvATI },
  { "glGetArrayObjectivATI", NULL, (void *) wine_glGetArrayObjectivATI },
  { "glGetAttachedObjectsARB", NULL, (void *) wine_glGetAttachedObjectsARB },
  { "glGetAttachedShaders", NULL, (void *) wine_glGetAttachedShaders },
  { "glGetAttribLocation", NULL, (void *) wine_glGetAttribLocation },
  { "glGetAttribLocationARB", NULL, (void *) wine_glGetAttribLocationARB },
  { "glGetBufferParameteriv", NULL, (void *) wine_glGetBufferParameteriv },
  { "glGetBufferParameterivARB", NULL, (void *) wine_glGetBufferParameterivARB },
  { "glGetBufferPointerv", NULL, (void *) wine_glGetBufferPointerv },
  { "glGetBufferPointervARB", NULL, (void *) wine_glGetBufferPointervARB },
  { "glGetBufferSubData", NULL, (void *) wine_glGetBufferSubData },
  { "glGetBufferSubDataARB", NULL, (void *) wine_glGetBufferSubDataARB },
  { "glGetColorTableEXT", NULL, (void *) wine_glGetColorTableEXT },
  { "glGetColorTableParameterfvEXT", NULL, (void *) wine_glGetColorTableParameterfvEXT },
  { "glGetColorTableParameterfvSGI", NULL, (void *) wine_glGetColorTableParameterfvSGI },
  { "glGetColorTableParameterivEXT", NULL, (void *) wine_glGetColorTableParameterivEXT },
  { "glGetColorTableParameterivSGI", NULL, (void *) wine_glGetColorTableParameterivSGI },
  { "glGetColorTableSGI", NULL, (void *) wine_glGetColorTableSGI },
  { "glGetCombinerInputParameterfvNV", NULL, (void *) wine_glGetCombinerInputParameterfvNV },
  { "glGetCombinerInputParameterivNV", NULL, (void *) wine_glGetCombinerInputParameterivNV },
  { "glGetCombinerOutputParameterfvNV", NULL, (void *) wine_glGetCombinerOutputParameterfvNV },
  { "glGetCombinerOutputParameterivNV", NULL, (void *) wine_glGetCombinerOutputParameterivNV },
  { "glGetCombinerStageParameterfvNV", NULL, (void *) wine_glGetCombinerStageParameterfvNV },
  { "glGetCompressedTexImage", NULL, (void *) wine_glGetCompressedTexImage },
  { "glGetCompressedTexImageARB", NULL, (void *) wine_glGetCompressedTexImageARB },
  { "glGetConvolutionFilterEXT", NULL, (void *) wine_glGetConvolutionFilterEXT },
  { "glGetConvolutionParameterfvEXT", NULL, (void *) wine_glGetConvolutionParameterfvEXT },
  { "glGetConvolutionParameterivEXT", NULL, (void *) wine_glGetConvolutionParameterivEXT },
  { "glGetDetailTexFuncSGIS", NULL, (void *) wine_glGetDetailTexFuncSGIS },
  { "glGetFenceivNV", NULL, (void *) wine_glGetFenceivNV },
  { "glGetFinalCombinerInputParameterfvNV", NULL, (void *) wine_glGetFinalCombinerInputParameterfvNV },
  { "glGetFinalCombinerInputParameterivNV", NULL, (void *) wine_glGetFinalCombinerInputParameterivNV },
  { "glGetFogFuncSGIS", NULL, (void *) wine_glGetFogFuncSGIS },
  { "glGetFragmentLightfvSGIX", NULL, (void *) wine_glGetFragmentLightfvSGIX },
  { "glGetFragmentLightivSGIX", NULL, (void *) wine_glGetFragmentLightivSGIX },
  { "glGetFragmentMaterialfvSGIX", NULL, (void *) wine_glGetFragmentMaterialfvSGIX },
  { "glGetFragmentMaterialivSGIX", NULL, (void *) wine_glGetFragmentMaterialivSGIX },
  { "glGetFramebufferAttachmentParameterivEXT", NULL, (void *) wine_glGetFramebufferAttachmentParameterivEXT },
  { "glGetHandleARB", NULL, (void *) wine_glGetHandleARB },
  { "glGetHistogramEXT", NULL, (void *) wine_glGetHistogramEXT },
  { "glGetHistogramParameterfvEXT", NULL, (void *) wine_glGetHistogramParameterfvEXT },
  { "glGetHistogramParameterivEXT", NULL, (void *) wine_glGetHistogramParameterivEXT },
  { "glGetImageTransformParameterfvHP", NULL, (void *) wine_glGetImageTransformParameterfvHP },
  { "glGetImageTransformParameterivHP", NULL, (void *) wine_glGetImageTransformParameterivHP },
  { "glGetInfoLogARB", NULL, (void *) wine_glGetInfoLogARB },
  { "glGetInstrumentsSGIX", NULL, (void *) wine_glGetInstrumentsSGIX },
  { "glGetInvariantBooleanvEXT", NULL, (void *) wine_glGetInvariantBooleanvEXT },
  { "glGetInvariantFloatvEXT", NULL, (void *) wine_glGetInvariantFloatvEXT },
  { "glGetInvariantIntegervEXT", NULL, (void *) wine_glGetInvariantIntegervEXT },
  { "glGetListParameterfvSGIX", NULL, (void *) wine_glGetListParameterfvSGIX },
  { "glGetListParameterivSGIX", NULL, (void *) wine_glGetListParameterivSGIX },
  { "glGetLocalConstantBooleanvEXT", NULL, (void *) wine_glGetLocalConstantBooleanvEXT },
  { "glGetLocalConstantFloatvEXT", NULL, (void *) wine_glGetLocalConstantFloatvEXT },
  { "glGetLocalConstantIntegervEXT", NULL, (void *) wine_glGetLocalConstantIntegervEXT },
  { "glGetMapAttribParameterfvNV", NULL, (void *) wine_glGetMapAttribParameterfvNV },
  { "glGetMapAttribParameterivNV", NULL, (void *) wine_glGetMapAttribParameterivNV },
  { "glGetMapControlPointsNV", NULL, (void *) wine_glGetMapControlPointsNV },
  { "glGetMapParameterfvNV", NULL, (void *) wine_glGetMapParameterfvNV },
  { "glGetMapParameterivNV", NULL, (void *) wine_glGetMapParameterivNV },
  { "glGetMinmaxEXT", NULL, (void *) wine_glGetMinmaxEXT },
  { "glGetMinmaxParameterfvEXT", NULL, (void *) wine_glGetMinmaxParameterfvEXT },
  { "glGetMinmaxParameterivEXT", NULL, (void *) wine_glGetMinmaxParameterivEXT },
  { "glGetObjectBufferfvATI", NULL, (void *) wine_glGetObjectBufferfvATI },
  { "glGetObjectBufferivATI", NULL, (void *) wine_glGetObjectBufferivATI },
  { "glGetObjectParameterfvARB", NULL, (void *) wine_glGetObjectParameterfvARB },
  { "glGetObjectParameterivARB", NULL, (void *) wine_glGetObjectParameterivARB },
  { "glGetOcclusionQueryivNV", NULL, (void *) wine_glGetOcclusionQueryivNV },
  { "glGetOcclusionQueryuivNV", NULL, (void *) wine_glGetOcclusionQueryuivNV },
  { "glGetPixelTexGenParameterfvSGIS", NULL, (void *) wine_glGetPixelTexGenParameterfvSGIS },
  { "glGetPixelTexGenParameterivSGIS", NULL, (void *) wine_glGetPixelTexGenParameterivSGIS },
  { "glGetPointervEXT", NULL, (void *) wine_glGetPointervEXT },
  { "glGetProgramEnvParameterdvARB", NULL, (void *) wine_glGetProgramEnvParameterdvARB },
  { "glGetProgramEnvParameterfvARB", NULL, (void *) wine_glGetProgramEnvParameterfvARB },
  { "glGetProgramInfoLog", NULL, (void *) wine_glGetProgramInfoLog },
  { "glGetProgramLocalParameterdvARB", NULL, (void *) wine_glGetProgramLocalParameterdvARB },
  { "glGetProgramLocalParameterfvARB", NULL, (void *) wine_glGetProgramLocalParameterfvARB },
  { "glGetProgramNamedParameterdvNV", NULL, (void *) wine_glGetProgramNamedParameterdvNV },
  { "glGetProgramNamedParameterfvNV", NULL, (void *) wine_glGetProgramNamedParameterfvNV },
  { "glGetProgramParameterdvNV", NULL, (void *) wine_glGetProgramParameterdvNV },
  { "glGetProgramParameterfvNV", NULL, (void *) wine_glGetProgramParameterfvNV },
  { "glGetProgramStringARB", NULL, (void *) wine_glGetProgramStringARB },
  { "glGetProgramStringNV", NULL, (void *) wine_glGetProgramStringNV },
  { "glGetProgramiv", NULL, (void *) wine_glGetProgramiv },
  { "glGetProgramivARB", NULL, (void *) wine_glGetProgramivARB },
  { "glGetProgramivNV", NULL, (void *) wine_glGetProgramivNV },
  { "glGetQueryObjectiv", NULL, (void *) wine_glGetQueryObjectiv },
  { "glGetQueryObjectivARB", NULL, (void *) wine_glGetQueryObjectivARB },
  { "glGetQueryObjectuiv", NULL, (void *) wine_glGetQueryObjectuiv },
  { "glGetQueryObjectuivARB", NULL, (void *) wine_glGetQueryObjectuivARB },
  { "glGetQueryiv", NULL, (void *) wine_glGetQueryiv },
  { "glGetQueryivARB", NULL, (void *) wine_glGetQueryivARB },
  { "glGetRenderbufferParameterivEXT", NULL, (void *) wine_glGetRenderbufferParameterivEXT },
  { "glGetSeparableFilterEXT", NULL, (void *) wine_glGetSeparableFilterEXT },
  { "glGetShaderInfoLog", NULL, (void *) wine_glGetShaderInfoLog },
  { "glGetShaderSource", NULL, (void *) wine_glGetShaderSource },
  { "glGetShaderSourceARB", NULL, (void *) wine_glGetShaderSourceARB },
  { "glGetShaderiv", NULL, (void *) wine_glGetShaderiv },
  { "glGetSharpenTexFuncSGIS", NULL, (void *) wine_glGetSharpenTexFuncSGIS },
  { "glGetTexBumpParameterfvATI", NULL, (void *) wine_glGetTexBumpParameterfvATI },
  { "glGetTexBumpParameterivATI", NULL, (void *) wine_glGetTexBumpParameterivATI },
  { "glGetTexFilterFuncSGIS", NULL, (void *) wine_glGetTexFilterFuncSGIS },
  { "glGetTrackMatrixivNV", NULL, (void *) wine_glGetTrackMatrixivNV },
  { "glGetUniformLocation", NULL, (void *) wine_glGetUniformLocation },
  { "glGetUniformLocationARB", NULL, (void *) wine_glGetUniformLocationARB },
  { "glGetUniformfv", NULL, (void *) wine_glGetUniformfv },
  { "glGetUniformfvARB", NULL, (void *) wine_glGetUniformfvARB },
  { "glGetUniformiv", NULL, (void *) wine_glGetUniformiv },
  { "glGetUniformivARB", NULL, (void *) wine_glGetUniformivARB },
  { "glGetVariantArrayObjectfvATI", NULL, (void *) wine_glGetVariantArrayObjectfvATI },
  { "glGetVariantArrayObjectivATI", NULL, (void *) wine_glGetVariantArrayObjectivATI },
  { "glGetVariantBooleanvEXT", NULL, (void *) wine_glGetVariantBooleanvEXT },
  { "glGetVariantFloatvEXT", NULL, (void *) wine_glGetVariantFloatvEXT },
  { "glGetVariantIntegervEXT", NULL, (void *) wine_glGetVariantIntegervEXT },
  { "glGetVariantPointervEXT", NULL, (void *) wine_glGetVariantPointervEXT },
  { "glGetVertexAttribArrayObjectfvATI", NULL, (void *) wine_glGetVertexAttribArrayObjectfvATI },
  { "glGetVertexAttribArrayObjectivATI", NULL, (void *) wine_glGetVertexAttribArrayObjectivATI },
  { "glGetVertexAttribPointerv", NULL, (void *) wine_glGetVertexAttribPointerv },
  { "glGetVertexAttribPointervARB", NULL, (void *) wine_glGetVertexAttribPointervARB },
  { "glGetVertexAttribPointervNV", NULL, (void *) wine_glGetVertexAttribPointervNV },
  { "glGetVertexAttribdv", NULL, (void *) wine_glGetVertexAttribdv },
  { "glGetVertexAttribdvARB", NULL, (void *) wine_glGetVertexAttribdvARB },
  { "glGetVertexAttribdvNV", NULL, (void *) wine_glGetVertexAttribdvNV },
  { "glGetVertexAttribfv", NULL, (void *) wine_glGetVertexAttribfv },
  { "glGetVertexAttribfvARB", NULL, (void *) wine_glGetVertexAttribfvARB },
  { "glGetVertexAttribfvNV", NULL, (void *) wine_glGetVertexAttribfvNV },
  { "glGetVertexAttribiv", NULL, (void *) wine_glGetVertexAttribiv },
  { "glGetVertexAttribivARB", NULL, (void *) wine_glGetVertexAttribivARB },
  { "glGetVertexAttribivNV", NULL, (void *) wine_glGetVertexAttribivNV },
  { "glGlobalAlphaFactorbSUN", NULL, (void *) wine_glGlobalAlphaFactorbSUN },
  { "glGlobalAlphaFactordSUN", NULL, (void *) wine_glGlobalAlphaFactordSUN },
  { "glGlobalAlphaFactorfSUN", NULL, (void *) wine_glGlobalAlphaFactorfSUN },
  { "glGlobalAlphaFactoriSUN", NULL, (void *) wine_glGlobalAlphaFactoriSUN },
  { "glGlobalAlphaFactorsSUN", NULL, (void *) wine_glGlobalAlphaFactorsSUN },
  { "glGlobalAlphaFactorubSUN", NULL, (void *) wine_glGlobalAlphaFactorubSUN },
  { "glGlobalAlphaFactoruiSUN", NULL, (void *) wine_glGlobalAlphaFactoruiSUN },
  { "glGlobalAlphaFactorusSUN", NULL, (void *) wine_glGlobalAlphaFactorusSUN },
  { "glHintPGI", NULL, (void *) wine_glHintPGI },
  { "glHistogramEXT", NULL, (void *) wine_glHistogramEXT },
  { "glIglooInterfaceSGIX", NULL, (void *) wine_glIglooInterfaceSGIX },
  { "glImageTransformParameterfHP", NULL, (void *) wine_glImageTransformParameterfHP },
  { "glImageTransformParameterfvHP", NULL, (void *) wine_glImageTransformParameterfvHP },
  { "glImageTransformParameteriHP", NULL, (void *) wine_glImageTransformParameteriHP },
  { "glImageTransformParameterivHP", NULL, (void *) wine_glImageTransformParameterivHP },
  { "glIndexFuncEXT", NULL, (void *) wine_glIndexFuncEXT },
  { "glIndexMaterialEXT", NULL, (void *) wine_glIndexMaterialEXT },
  { "glIndexPointerEXT", NULL, (void *) wine_glIndexPointerEXT },
  { "glIndexPointerListIBM", NULL, (void *) wine_glIndexPointerListIBM },
  { "glInsertComponentEXT", NULL, (void *) wine_glInsertComponentEXT },
  { "glInstrumentsBufferSGIX", NULL, (void *) wine_glInstrumentsBufferSGIX },
  { "glIsAsyncMarkerSGIX", NULL, (void *) wine_glIsAsyncMarkerSGIX },
  { "glIsBuffer", NULL, (void *) wine_glIsBuffer },
  { "glIsBufferARB", NULL, (void *) wine_glIsBufferARB },
  { "glIsFenceAPPLE", NULL, (void *) wine_glIsFenceAPPLE },
  { "glIsFenceNV", NULL, (void *) wine_glIsFenceNV },
  { "glIsFramebufferEXT", NULL, (void *) wine_glIsFramebufferEXT },
  { "glIsObjectBufferATI", NULL, (void *) wine_glIsObjectBufferATI },
  { "glIsOcclusionQueryNV", NULL, (void *) wine_glIsOcclusionQueryNV },
  { "glIsProgram", NULL, (void *) wine_glIsProgram },
  { "glIsProgramARB", NULL, (void *) wine_glIsProgramARB },
  { "glIsProgramNV", NULL, (void *) wine_glIsProgramNV },
  { "glIsQuery", NULL, (void *) wine_glIsQuery },
  { "glIsQueryARB", NULL, (void *) wine_glIsQueryARB },
  { "glIsRenderbufferEXT", NULL, (void *) wine_glIsRenderbufferEXT },
  { "glIsShader", NULL, (void *) wine_glIsShader },
  { "glIsTextureEXT", NULL, (void *) wine_glIsTextureEXT },
  { "glIsVariantEnabledEXT", NULL, (void *) wine_glIsVariantEnabledEXT },
  { "glIsVertexArrayAPPLE", NULL, (void *) wine_glIsVertexArrayAPPLE },
  { "glLightEnviSGIX", NULL, (void *) wine_glLightEnviSGIX },
  { "glLinkProgram", NULL, (void *) wine_glLinkProgram },
  { "glLinkProgramARB", NULL, (void *) wine_glLinkProgramARB },
  { "glListParameterfSGIX", NULL, (void *) wine_glListParameterfSGIX },
  { "glListParameterfvSGIX", NULL, (void *) wine_glListParameterfvSGIX },
  { "glListParameteriSGIX", NULL, (void *) wine_glListParameteriSGIX },
  { "glListParameterivSGIX", NULL, (void *) wine_glListParameterivSGIX },
  { "glLoadIdentityDeformationMapSGIX", NULL, (void *) wine_glLoadIdentityDeformationMapSGIX },
  { "glLoadProgramNV", NULL, (void *) wine_glLoadProgramNV },
  { "glLoadTransposeMatrixd", NULL, (void *) wine_glLoadTransposeMatrixd },
  { "glLoadTransposeMatrixdARB", NULL, (void *) wine_glLoadTransposeMatrixdARB },
  { "glLoadTransposeMatrixf", NULL, (void *) wine_glLoadTransposeMatrixf },
  { "glLoadTransposeMatrixfARB", NULL, (void *) wine_glLoadTransposeMatrixfARB },
  { "glLockArraysEXT", NULL, (void *) wine_glLockArraysEXT },
  { "glMTexCoord2fSGIS", NULL, (void *) wine_glMTexCoord2fSGIS },
  { "glMTexCoord2fvSGIS", NULL, (void *) wine_glMTexCoord2fvSGIS },
  { "glMapBuffer", NULL, (void *) wine_glMapBuffer },
  { "glMapBufferARB", NULL, (void *) wine_glMapBufferARB },
  { "glMapControlPointsNV", NULL, (void *) wine_glMapControlPointsNV },
  { "glMapObjectBufferATI", NULL, (void *) wine_glMapObjectBufferATI },
  { "glMapParameterfvNV", NULL, (void *) wine_glMapParameterfvNV },
  { "glMapParameterivNV", NULL, (void *) wine_glMapParameterivNV },
  { "glMatrixIndexPointerARB", NULL, (void *) wine_glMatrixIndexPointerARB },
  { "glMatrixIndexubvARB", NULL, (void *) wine_glMatrixIndexubvARB },
  { "glMatrixIndexuivARB", NULL, (void *) wine_glMatrixIndexuivARB },
  { "glMatrixIndexusvARB", NULL, (void *) wine_glMatrixIndexusvARB },
  { "glMinmaxEXT", NULL, (void *) wine_glMinmaxEXT },
  { "glMultTransposeMatrixd", NULL, (void *) wine_glMultTransposeMatrixd },
  { "glMultTransposeMatrixdARB", NULL, (void *) wine_glMultTransposeMatrixdARB },
  { "glMultTransposeMatrixf", NULL, (void *) wine_glMultTransposeMatrixf },
  { "glMultTransposeMatrixfARB", NULL, (void *) wine_glMultTransposeMatrixfARB },
  { "glMultiDrawArrays", NULL, (void *) wine_glMultiDrawArrays },
  { "glMultiDrawArraysEXT", NULL, (void *) wine_glMultiDrawArraysEXT },
  { "glMultiDrawElementArrayAPPLE", NULL, (void *) wine_glMultiDrawElementArrayAPPLE },
  { "glMultiDrawElements", NULL, (void *) wine_glMultiDrawElements },
  { "glMultiDrawElementsEXT", NULL, (void *) wine_glMultiDrawElementsEXT },
  { "glMultiDrawRangeElementArrayAPPLE", NULL, (void *) wine_glMultiDrawRangeElementArrayAPPLE },
  { "glMultiModeDrawArraysIBM", NULL, (void *) wine_glMultiModeDrawArraysIBM },
  { "glMultiModeDrawElementsIBM", NULL, (void *) wine_glMultiModeDrawElementsIBM },
  { "glMultiTexCoord1d", NULL, (void *) wine_glMultiTexCoord1d },
  { "glMultiTexCoord1dARB", NULL, (void *) wine_glMultiTexCoord1dARB },
  { "glMultiTexCoord1dSGIS", NULL, (void *) wine_glMultiTexCoord1dSGIS },
  { "glMultiTexCoord1dv", NULL, (void *) wine_glMultiTexCoord1dv },
  { "glMultiTexCoord1dvARB", NULL, (void *) wine_glMultiTexCoord1dvARB },
  { "glMultiTexCoord1dvSGIS", NULL, (void *) wine_glMultiTexCoord1dvSGIS },
  { "glMultiTexCoord1f", NULL, (void *) wine_glMultiTexCoord1f },
  { "glMultiTexCoord1fARB", NULL, (void *) wine_glMultiTexCoord1fARB },
  { "glMultiTexCoord1fSGIS", NULL, (void *) wine_glMultiTexCoord1fSGIS },
  { "glMultiTexCoord1fv", NULL, (void *) wine_glMultiTexCoord1fv },
  { "glMultiTexCoord1fvARB", NULL, (void *) wine_glMultiTexCoord1fvARB },
  { "glMultiTexCoord1fvSGIS", NULL, (void *) wine_glMultiTexCoord1fvSGIS },
  { "glMultiTexCoord1hNV", NULL, (void *) wine_glMultiTexCoord1hNV },
  { "glMultiTexCoord1hvNV", NULL, (void *) wine_glMultiTexCoord1hvNV },
  { "glMultiTexCoord1i", NULL, (void *) wine_glMultiTexCoord1i },
  { "glMultiTexCoord1iARB", NULL, (void *) wine_glMultiTexCoord1iARB },
  { "glMultiTexCoord1iSGIS", NULL, (void *) wine_glMultiTexCoord1iSGIS },
  { "glMultiTexCoord1iv", NULL, (void *) wine_glMultiTexCoord1iv },
  { "glMultiTexCoord1ivARB", NULL, (void *) wine_glMultiTexCoord1ivARB },
  { "glMultiTexCoord1ivSGIS", NULL, (void *) wine_glMultiTexCoord1ivSGIS },
  { "glMultiTexCoord1s", NULL, (void *) wine_glMultiTexCoord1s },
  { "glMultiTexCoord1sARB", NULL, (void *) wine_glMultiTexCoord1sARB },
  { "glMultiTexCoord1sSGIS", NULL, (void *) wine_glMultiTexCoord1sSGIS },
  { "glMultiTexCoord1sv", NULL, (void *) wine_glMultiTexCoord1sv },
  { "glMultiTexCoord1svARB", NULL, (void *) wine_glMultiTexCoord1svARB },
  { "glMultiTexCoord1svSGIS", NULL, (void *) wine_glMultiTexCoord1svSGIS },
  { "glMultiTexCoord2d", NULL, (void *) wine_glMultiTexCoord2d },
  { "glMultiTexCoord2dARB", NULL, (void *) wine_glMultiTexCoord2dARB },
  { "glMultiTexCoord2dSGIS", NULL, (void *) wine_glMultiTexCoord2dSGIS },
  { "glMultiTexCoord2dv", NULL, (void *) wine_glMultiTexCoord2dv },
  { "glMultiTexCoord2dvARB", NULL, (void *) wine_glMultiTexCoord2dvARB },
  { "glMultiTexCoord2dvSGIS", NULL, (void *) wine_glMultiTexCoord2dvSGIS },
  { "glMultiTexCoord2f", NULL, (void *) wine_glMultiTexCoord2f },
  { "glMultiTexCoord2fARB", NULL, (void *) wine_glMultiTexCoord2fARB },
  { "glMultiTexCoord2fSGIS", NULL, (void *) wine_glMultiTexCoord2fSGIS },
  { "glMultiTexCoord2fv", NULL, (void *) wine_glMultiTexCoord2fv },
  { "glMultiTexCoord2fvARB", NULL, (void *) wine_glMultiTexCoord2fvARB },
  { "glMultiTexCoord2fvSGIS", NULL, (void *) wine_glMultiTexCoord2fvSGIS },
  { "glMultiTexCoord2hNV", NULL, (void *) wine_glMultiTexCoord2hNV },
  { "glMultiTexCoord2hvNV", NULL, (void *) wine_glMultiTexCoord2hvNV },
  { "glMultiTexCoord2i", NULL, (void *) wine_glMultiTexCoord2i },
  { "glMultiTexCoord2iARB", NULL, (void *) wine_glMultiTexCoord2iARB },
  { "glMultiTexCoord2iSGIS", NULL, (void *) wine_glMultiTexCoord2iSGIS },
  { "glMultiTexCoord2iv", NULL, (void *) wine_glMultiTexCoord2iv },
  { "glMultiTexCoord2ivARB", NULL, (void *) wine_glMultiTexCoord2ivARB },
  { "glMultiTexCoord2ivSGIS", NULL, (void *) wine_glMultiTexCoord2ivSGIS },
  { "glMultiTexCoord2s", NULL, (void *) wine_glMultiTexCoord2s },
  { "glMultiTexCoord2sARB", NULL, (void *) wine_glMultiTexCoord2sARB },
  { "glMultiTexCoord2sSGIS", NULL, (void *) wine_glMultiTexCoord2sSGIS },
  { "glMultiTexCoord2sv", NULL, (void *) wine_glMultiTexCoord2sv },
  { "glMultiTexCoord2svARB", NULL, (void *) wine_glMultiTexCoord2svARB },
  { "glMultiTexCoord2svSGIS", NULL, (void *) wine_glMultiTexCoord2svSGIS },
  { "glMultiTexCoord3d", NULL, (void *) wine_glMultiTexCoord3d },
  { "glMultiTexCoord3dARB", NULL, (void *) wine_glMultiTexCoord3dARB },
  { "glMultiTexCoord3dSGIS", NULL, (void *) wine_glMultiTexCoord3dSGIS },
  { "glMultiTexCoord3dv", NULL, (void *) wine_glMultiTexCoord3dv },
  { "glMultiTexCoord3dvARB", NULL, (void *) wine_glMultiTexCoord3dvARB },
  { "glMultiTexCoord3dvSGIS", NULL, (void *) wine_glMultiTexCoord3dvSGIS },
  { "glMultiTexCoord3f", NULL, (void *) wine_glMultiTexCoord3f },
  { "glMultiTexCoord3fARB", NULL, (void *) wine_glMultiTexCoord3fARB },
  { "glMultiTexCoord3fSGIS", NULL, (void *) wine_glMultiTexCoord3fSGIS },
  { "glMultiTexCoord3fv", NULL, (void *) wine_glMultiTexCoord3fv },
  { "glMultiTexCoord3fvARB", NULL, (void *) wine_glMultiTexCoord3fvARB },
  { "glMultiTexCoord3fvSGIS", NULL, (void *) wine_glMultiTexCoord3fvSGIS },
  { "glMultiTexCoord3hNV", NULL, (void *) wine_glMultiTexCoord3hNV },
  { "glMultiTexCoord3hvNV", NULL, (void *) wine_glMultiTexCoord3hvNV },
  { "glMultiTexCoord3i", NULL, (void *) wine_glMultiTexCoord3i },
  { "glMultiTexCoord3iARB", NULL, (void *) wine_glMultiTexCoord3iARB },
  { "glMultiTexCoord3iSGIS", NULL, (void *) wine_glMultiTexCoord3iSGIS },
  { "glMultiTexCoord3iv", NULL, (void *) wine_glMultiTexCoord3iv },
  { "glMultiTexCoord3ivARB", NULL, (void *) wine_glMultiTexCoord3ivARB },
  { "glMultiTexCoord3ivSGIS", NULL, (void *) wine_glMultiTexCoord3ivSGIS },
  { "glMultiTexCoord3s", NULL, (void *) wine_glMultiTexCoord3s },
  { "glMultiTexCoord3sARB", NULL, (void *) wine_glMultiTexCoord3sARB },
  { "glMultiTexCoord3sSGIS", NULL, (void *) wine_glMultiTexCoord3sSGIS },
  { "glMultiTexCoord3sv", NULL, (void *) wine_glMultiTexCoord3sv },
  { "glMultiTexCoord3svARB", NULL, (void *) wine_glMultiTexCoord3svARB },
  { "glMultiTexCoord3svSGIS", NULL, (void *) wine_glMultiTexCoord3svSGIS },
  { "glMultiTexCoord4d", NULL, (void *) wine_glMultiTexCoord4d },
  { "glMultiTexCoord4dARB", NULL, (void *) wine_glMultiTexCoord4dARB },
  { "glMultiTexCoord4dSGIS", NULL, (void *) wine_glMultiTexCoord4dSGIS },
  { "glMultiTexCoord4dv", NULL, (void *) wine_glMultiTexCoord4dv },
  { "glMultiTexCoord4dvARB", NULL, (void *) wine_glMultiTexCoord4dvARB },
  { "glMultiTexCoord4dvSGIS", NULL, (void *) wine_glMultiTexCoord4dvSGIS },
  { "glMultiTexCoord4f", NULL, (void *) wine_glMultiTexCoord4f },
  { "glMultiTexCoord4fARB", NULL, (void *) wine_glMultiTexCoord4fARB },
  { "glMultiTexCoord4fSGIS", NULL, (void *) wine_glMultiTexCoord4fSGIS },
  { "glMultiTexCoord4fv", NULL, (void *) wine_glMultiTexCoord4fv },
  { "glMultiTexCoord4fvARB", NULL, (void *) wine_glMultiTexCoord4fvARB },
  { "glMultiTexCoord4fvSGIS", NULL, (void *) wine_glMultiTexCoord4fvSGIS },
  { "glMultiTexCoord4hNV", NULL, (void *) wine_glMultiTexCoord4hNV },
  { "glMultiTexCoord4hvNV", NULL, (void *) wine_glMultiTexCoord4hvNV },
  { "glMultiTexCoord4i", NULL, (void *) wine_glMultiTexCoord4i },
  { "glMultiTexCoord4iARB", NULL, (void *) wine_glMultiTexCoord4iARB },
  { "glMultiTexCoord4iSGIS", NULL, (void *) wine_glMultiTexCoord4iSGIS },
  { "glMultiTexCoord4iv", NULL, (void *) wine_glMultiTexCoord4iv },
  { "glMultiTexCoord4ivARB", NULL, (void *) wine_glMultiTexCoord4ivARB },
  { "glMultiTexCoord4ivSGIS", NULL, (void *) wine_glMultiTexCoord4ivSGIS },
  { "glMultiTexCoord4s", NULL, (void *) wine_glMultiTexCoord4s },
  { "glMultiTexCoord4sARB", NULL, (void *) wine_glMultiTexCoord4sARB },
  { "glMultiTexCoord4sSGIS", NULL, (void *) wine_glMultiTexCoord4sSGIS },
  { "glMultiTexCoord4sv", NULL, (void *) wine_glMultiTexCoord4sv },
  { "glMultiTexCoord4svARB", NULL, (void *) wine_glMultiTexCoord4svARB },
  { "glMultiTexCoord4svSGIS", NULL, (void *) wine_glMultiTexCoord4svSGIS },
  { "glMultiTexCoordPointerSGIS", NULL, (void *) wine_glMultiTexCoordPointerSGIS },
  { "glNewBufferRegion", NULL, (void *) wine_glNewBufferRegion },
  { "glNewObjectBufferATI", NULL, (void *) wine_glNewObjectBufferATI },
  { "glNormal3fVertex3fSUN", NULL, (void *) wine_glNormal3fVertex3fSUN },
  { "glNormal3fVertex3fvSUN", NULL, (void *) wine_glNormal3fVertex3fvSUN },
  { "glNormal3hNV", NULL, (void *) wine_glNormal3hNV },
  { "glNormal3hvNV", NULL, (void *) wine_glNormal3hvNV },
  { "glNormalPointerEXT", NULL, (void *) wine_glNormalPointerEXT },
  { "glNormalPointerListIBM", NULL, (void *) wine_glNormalPointerListIBM },
  { "glNormalPointervINTEL", NULL, (void *) wine_glNormalPointervINTEL },
  { "glNormalStream3bATI", NULL, (void *) wine_glNormalStream3bATI },
  { "glNormalStream3bvATI", NULL, (void *) wine_glNormalStream3bvATI },
  { "glNormalStream3dATI", NULL, (void *) wine_glNormalStream3dATI },
  { "glNormalStream3dvATI", NULL, (void *) wine_glNormalStream3dvATI },
  { "glNormalStream3fATI", NULL, (void *) wine_glNormalStream3fATI },
  { "glNormalStream3fvATI", NULL, (void *) wine_glNormalStream3fvATI },
  { "glNormalStream3iATI", NULL, (void *) wine_glNormalStream3iATI },
  { "glNormalStream3ivATI", NULL, (void *) wine_glNormalStream3ivATI },
  { "glNormalStream3sATI", NULL, (void *) wine_glNormalStream3sATI },
  { "glNormalStream3svATI", NULL, (void *) wine_glNormalStream3svATI },
  { "glPNTrianglesfATI", NULL, (void *) wine_glPNTrianglesfATI },
  { "glPNTrianglesiATI", NULL, (void *) wine_glPNTrianglesiATI },
  { "glPassTexCoordATI", NULL, (void *) wine_glPassTexCoordATI },
  { "glPixelDataRangeNV", NULL, (void *) wine_glPixelDataRangeNV },
  { "glPixelTexGenParameterfSGIS", NULL, (void *) wine_glPixelTexGenParameterfSGIS },
  { "glPixelTexGenParameterfvSGIS", NULL, (void *) wine_glPixelTexGenParameterfvSGIS },
  { "glPixelTexGenParameteriSGIS", NULL, (void *) wine_glPixelTexGenParameteriSGIS },
  { "glPixelTexGenParameterivSGIS", NULL, (void *) wine_glPixelTexGenParameterivSGIS },
  { "glPixelTexGenSGIX", NULL, (void *) wine_glPixelTexGenSGIX },
  { "glPixelTransformParameterfEXT", NULL, (void *) wine_glPixelTransformParameterfEXT },
  { "glPixelTransformParameterfvEXT", NULL, (void *) wine_glPixelTransformParameterfvEXT },
  { "glPixelTransformParameteriEXT", NULL, (void *) wine_glPixelTransformParameteriEXT },
  { "glPixelTransformParameterivEXT", NULL, (void *) wine_glPixelTransformParameterivEXT },
  { "glPointParameterf", NULL, (void *) wine_glPointParameterf },
  { "glPointParameterfARB", NULL, (void *) wine_glPointParameterfARB },
  { "glPointParameterfEXT", NULL, (void *) wine_glPointParameterfEXT },
  { "glPointParameterfSGIS", NULL, (void *) wine_glPointParameterfSGIS },
  { "glPointParameterfv", NULL, (void *) wine_glPointParameterfv },
  { "glPointParameterfvARB", NULL, (void *) wine_glPointParameterfvARB },
  { "glPointParameterfvEXT", NULL, (void *) wine_glPointParameterfvEXT },
  { "glPointParameterfvSGIS", NULL, (void *) wine_glPointParameterfvSGIS },
  { "glPointParameteri", NULL, (void *) wine_glPointParameteri },
  { "glPointParameteriNV", NULL, (void *) wine_glPointParameteriNV },
  { "glPointParameteriv", NULL, (void *) wine_glPointParameteriv },
  { "glPointParameterivNV", NULL, (void *) wine_glPointParameterivNV },
  { "glPollAsyncSGIX", NULL, (void *) wine_glPollAsyncSGIX },
  { "glPollInstrumentsSGIX", NULL, (void *) wine_glPollInstrumentsSGIX },
  { "glPolygonOffsetEXT", NULL, (void *) wine_glPolygonOffsetEXT },
  { "glPrimitiveRestartIndexNV", NULL, (void *) wine_glPrimitiveRestartIndexNV },
  { "glPrimitiveRestartNV", NULL, (void *) wine_glPrimitiveRestartNV },
  { "glPrioritizeTexturesEXT", NULL, (void *) wine_glPrioritizeTexturesEXT },
  { "glProgramEnvParameter4dARB", NULL, (void *) wine_glProgramEnvParameter4dARB },
  { "glProgramEnvParameter4dvARB", NULL, (void *) wine_glProgramEnvParameter4dvARB },
  { "glProgramEnvParameter4fARB", NULL, (void *) wine_glProgramEnvParameter4fARB },
  { "glProgramEnvParameter4fvARB", NULL, (void *) wine_glProgramEnvParameter4fvARB },
  { "glProgramLocalParameter4dARB", NULL, (void *) wine_glProgramLocalParameter4dARB },
  { "glProgramLocalParameter4dvARB", NULL, (void *) wine_glProgramLocalParameter4dvARB },
  { "glProgramLocalParameter4fARB", NULL, (void *) wine_glProgramLocalParameter4fARB },
  { "glProgramLocalParameter4fvARB", NULL, (void *) wine_glProgramLocalParameter4fvARB },
  { "glProgramNamedParameter4dNV", NULL, (void *) wine_glProgramNamedParameter4dNV },
  { "glProgramNamedParameter4dvNV", NULL, (void *) wine_glProgramNamedParameter4dvNV },
  { "glProgramNamedParameter4fNV", NULL, (void *) wine_glProgramNamedParameter4fNV },
  { "glProgramNamedParameter4fvNV", NULL, (void *) wine_glProgramNamedParameter4fvNV },
  { "glProgramParameter4dNV", NULL, (void *) wine_glProgramParameter4dNV },
  { "glProgramParameter4dvNV", NULL, (void *) wine_glProgramParameter4dvNV },
  { "glProgramParameter4fNV", NULL, (void *) wine_glProgramParameter4fNV },
  { "glProgramParameter4fvNV", NULL, (void *) wine_glProgramParameter4fvNV },
  { "glProgramParameters4dvNV", NULL, (void *) wine_glProgramParameters4dvNV },
  { "glProgramParameters4fvNV", NULL, (void *) wine_glProgramParameters4fvNV },
  { "glProgramStringARB", NULL, (void *) wine_glProgramStringARB },
  { "glReadBufferRegion", NULL, (void *) wine_glReadBufferRegion },
  { "glReadInstrumentsSGIX", NULL, (void *) wine_glReadInstrumentsSGIX },
  { "glReferencePlaneSGIX", NULL, (void *) wine_glReferencePlaneSGIX },
  { "glRenderbufferStorageEXT", NULL, (void *) wine_glRenderbufferStorageEXT },
  { "glReplacementCodePointerSUN", NULL, (void *) wine_glReplacementCodePointerSUN },
  { "glReplacementCodeubSUN", NULL, (void *) wine_glReplacementCodeubSUN },
  { "glReplacementCodeubvSUN", NULL, (void *) wine_glReplacementCodeubvSUN },
  { "glReplacementCodeuiColor3fVertex3fSUN", NULL, (void *) wine_glReplacementCodeuiColor3fVertex3fSUN },
  { "glReplacementCodeuiColor3fVertex3fvSUN", NULL, (void *) wine_glReplacementCodeuiColor3fVertex3fvSUN },
  { "glReplacementCodeuiColor4fNormal3fVertex3fSUN", NULL, (void *) wine_glReplacementCodeuiColor4fNormal3fVertex3fSUN },
  { "glReplacementCodeuiColor4fNormal3fVertex3fvSUN", NULL, (void *) wine_glReplacementCodeuiColor4fNormal3fVertex3fvSUN },
  { "glReplacementCodeuiColor4ubVertex3fSUN", NULL, (void *) wine_glReplacementCodeuiColor4ubVertex3fSUN },
  { "glReplacementCodeuiColor4ubVertex3fvSUN", NULL, (void *) wine_glReplacementCodeuiColor4ubVertex3fvSUN },
  { "glReplacementCodeuiNormal3fVertex3fSUN", NULL, (void *) wine_glReplacementCodeuiNormal3fVertex3fSUN },
  { "glReplacementCodeuiNormal3fVertex3fvSUN", NULL, (void *) wine_glReplacementCodeuiNormal3fVertex3fvSUN },
  { "glReplacementCodeuiSUN", NULL, (void *) wine_glReplacementCodeuiSUN },
  { "glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN", NULL, (void *) wine_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN },
  { "glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN", NULL, (void *) wine_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN },
  { "glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN", NULL, (void *) wine_glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN },
  { "glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN", NULL, (void *) wine_glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN },
  { "glReplacementCodeuiTexCoord2fVertex3fSUN", NULL, (void *) wine_glReplacementCodeuiTexCoord2fVertex3fSUN },
  { "glReplacementCodeuiTexCoord2fVertex3fvSUN", NULL, (void *) wine_glReplacementCodeuiTexCoord2fVertex3fvSUN },
  { "glReplacementCodeuiVertex3fSUN", NULL, (void *) wine_glReplacementCodeuiVertex3fSUN },
  { "glReplacementCodeuiVertex3fvSUN", NULL, (void *) wine_glReplacementCodeuiVertex3fvSUN },
  { "glReplacementCodeuivSUN", NULL, (void *) wine_glReplacementCodeuivSUN },
  { "glReplacementCodeusSUN", NULL, (void *) wine_glReplacementCodeusSUN },
  { "glReplacementCodeusvSUN", NULL, (void *) wine_glReplacementCodeusvSUN },
  { "glRequestResidentProgramsNV", NULL, (void *) wine_glRequestResidentProgramsNV },
  { "glResetHistogramEXT", NULL, (void *) wine_glResetHistogramEXT },
  { "glResetMinmaxEXT", NULL, (void *) wine_glResetMinmaxEXT },
  { "glResizeBuffersMESA", NULL, (void *) wine_glResizeBuffersMESA },
  { "glSampleCoverage", NULL, (void *) wine_glSampleCoverage },
  { "glSampleCoverageARB", NULL, (void *) wine_glSampleCoverageARB },
  { "glSampleMapATI", NULL, (void *) wine_glSampleMapATI },
  { "glSampleMaskEXT", NULL, (void *) wine_glSampleMaskEXT },
  { "glSampleMaskSGIS", NULL, (void *) wine_glSampleMaskSGIS },
  { "glSamplePatternEXT", NULL, (void *) wine_glSamplePatternEXT },
  { "glSamplePatternSGIS", NULL, (void *) wine_glSamplePatternSGIS },
  { "glSecondaryColor3b", NULL, (void *) wine_glSecondaryColor3b },
  { "glSecondaryColor3bEXT", NULL, (void *) wine_glSecondaryColor3bEXT },
  { "glSecondaryColor3bv", NULL, (void *) wine_glSecondaryColor3bv },
  { "glSecondaryColor3bvEXT", NULL, (void *) wine_glSecondaryColor3bvEXT },
  { "glSecondaryColor3d", NULL, (void *) wine_glSecondaryColor3d },
  { "glSecondaryColor3dEXT", NULL, (void *) wine_glSecondaryColor3dEXT },
  { "glSecondaryColor3dv", NULL, (void *) wine_glSecondaryColor3dv },
  { "glSecondaryColor3dvEXT", NULL, (void *) wine_glSecondaryColor3dvEXT },
  { "glSecondaryColor3f", NULL, (void *) wine_glSecondaryColor3f },
  { "glSecondaryColor3fEXT", NULL, (void *) wine_glSecondaryColor3fEXT },
  { "glSecondaryColor3fv", NULL, (void *) wine_glSecondaryColor3fv },
  { "glSecondaryColor3fvEXT", NULL, (void *) wine_glSecondaryColor3fvEXT },
  { "glSecondaryColor3hNV", NULL, (void *) wine_glSecondaryColor3hNV },
  { "glSecondaryColor3hvNV", NULL, (void *) wine_glSecondaryColor3hvNV },
  { "glSecondaryColor3i", NULL, (void *) wine_glSecondaryColor3i },
  { "glSecondaryColor3iEXT", NULL, (void *) wine_glSecondaryColor3iEXT },
  { "glSecondaryColor3iv", NULL, (void *) wine_glSecondaryColor3iv },
  { "glSecondaryColor3ivEXT", NULL, (void *) wine_glSecondaryColor3ivEXT },
  { "glSecondaryColor3s", NULL, (void *) wine_glSecondaryColor3s },
  { "glSecondaryColor3sEXT", NULL, (void *) wine_glSecondaryColor3sEXT },
  { "glSecondaryColor3sv", NULL, (void *) wine_glSecondaryColor3sv },
  { "glSecondaryColor3svEXT", NULL, (void *) wine_glSecondaryColor3svEXT },
  { "glSecondaryColor3ub", NULL, (void *) wine_glSecondaryColor3ub },
  { "glSecondaryColor3ubEXT", NULL, (void *) wine_glSecondaryColor3ubEXT },
  { "glSecondaryColor3ubv", NULL, (void *) wine_glSecondaryColor3ubv },
  { "glSecondaryColor3ubvEXT", NULL, (void *) wine_glSecondaryColor3ubvEXT },
  { "glSecondaryColor3ui", NULL, (void *) wine_glSecondaryColor3ui },
  { "glSecondaryColor3uiEXT", NULL, (void *) wine_glSecondaryColor3uiEXT },
  { "glSecondaryColor3uiv", NULL, (void *) wine_glSecondaryColor3uiv },
  { "glSecondaryColor3uivEXT", NULL, (void *) wine_glSecondaryColor3uivEXT },
  { "glSecondaryColor3us", NULL, (void *) wine_glSecondaryColor3us },
  { "glSecondaryColor3usEXT", NULL, (void *) wine_glSecondaryColor3usEXT },
  { "glSecondaryColor3usv", NULL, (void *) wine_glSecondaryColor3usv },
  { "glSecondaryColor3usvEXT", NULL, (void *) wine_glSecondaryColor3usvEXT },
  { "glSecondaryColorPointer", NULL, (void *) wine_glSecondaryColorPointer },
  { "glSecondaryColorPointerEXT", NULL, (void *) wine_glSecondaryColorPointerEXT },
  { "glSecondaryColorPointerListIBM", NULL, (void *) wine_glSecondaryColorPointerListIBM },
  { "glSelectTextureCoordSetSGIS", NULL, (void *) wine_glSelectTextureCoordSetSGIS },
  { "glSelectTextureSGIS", NULL, (void *) wine_glSelectTextureSGIS },
  { "glSeparableFilter2DEXT", NULL, (void *) wine_glSeparableFilter2DEXT },
  { "glSetFenceAPPLE", NULL, (void *) wine_glSetFenceAPPLE },
  { "glSetFenceNV", NULL, (void *) wine_glSetFenceNV },
  { "glSetFragmentShaderConstantATI", NULL, (void *) wine_glSetFragmentShaderConstantATI },
  { "glSetInvariantEXT", NULL, (void *) wine_glSetInvariantEXT },
  { "glSetLocalConstantEXT", NULL, (void *) wine_glSetLocalConstantEXT },
  { "glShaderOp1EXT", NULL, (void *) wine_glShaderOp1EXT },
  { "glShaderOp2EXT", NULL, (void *) wine_glShaderOp2EXT },
  { "glShaderOp3EXT", NULL, (void *) wine_glShaderOp3EXT },
  { "glShaderSource", NULL, (void *) wine_glShaderSource },
  { "glShaderSourceARB", NULL, (void *) wine_glShaderSourceARB },
  { "glSharpenTexFuncSGIS", NULL, (void *) wine_glSharpenTexFuncSGIS },
  { "glSpriteParameterfSGIX", NULL, (void *) wine_glSpriteParameterfSGIX },
  { "glSpriteParameterfvSGIX", NULL, (void *) wine_glSpriteParameterfvSGIX },
  { "glSpriteParameteriSGIX", NULL, (void *) wine_glSpriteParameteriSGIX },
  { "glSpriteParameterivSGIX", NULL, (void *) wine_glSpriteParameterivSGIX },
  { "glStartInstrumentsSGIX", NULL, (void *) wine_glStartInstrumentsSGIX },
  { "glStencilFuncSeparate", NULL, (void *) wine_glStencilFuncSeparate },
  { "glStencilFuncSeparateATI", NULL, (void *) wine_glStencilFuncSeparateATI },
  { "glStencilMaskSeparate", NULL, (void *) wine_glStencilMaskSeparate },
  { "glStencilOpSeparate", NULL, (void *) wine_glStencilOpSeparate },
  { "glStencilOpSeparateATI", NULL, (void *) wine_glStencilOpSeparateATI },
  { "glStopInstrumentsSGIX", NULL, (void *) wine_glStopInstrumentsSGIX },
  { "glStringMarkerGREMEDY", NULL, (void *) wine_glStringMarkerGREMEDY },
  { "glSwizzleEXT", NULL, (void *) wine_glSwizzleEXT },
  { "glTagSampleBufferSGIX", NULL, (void *) wine_glTagSampleBufferSGIX },
  { "glTangent3bEXT", NULL, (void *) wine_glTangent3bEXT },
  { "glTangent3bvEXT", NULL, (void *) wine_glTangent3bvEXT },
  { "glTangent3dEXT", NULL, (void *) wine_glTangent3dEXT },
  { "glTangent3dvEXT", NULL, (void *) wine_glTangent3dvEXT },
  { "glTangent3fEXT", NULL, (void *) wine_glTangent3fEXT },
  { "glTangent3fvEXT", NULL, (void *) wine_glTangent3fvEXT },
  { "glTangent3iEXT", NULL, (void *) wine_glTangent3iEXT },
  { "glTangent3ivEXT", NULL, (void *) wine_glTangent3ivEXT },
  { "glTangent3sEXT", NULL, (void *) wine_glTangent3sEXT },
  { "glTangent3svEXT", NULL, (void *) wine_glTangent3svEXT },
  { "glTangentPointerEXT", NULL, (void *) wine_glTangentPointerEXT },
  { "glTbufferMask3DFX", NULL, (void *) wine_glTbufferMask3DFX },
  { "glTestFenceAPPLE", NULL, (void *) wine_glTestFenceAPPLE },
  { "glTestFenceNV", NULL, (void *) wine_glTestFenceNV },
  { "glTestObjectAPPLE", NULL, (void *) wine_glTestObjectAPPLE },
  { "glTexBumpParameterfvATI", NULL, (void *) wine_glTexBumpParameterfvATI },
  { "glTexBumpParameterivATI", NULL, (void *) wine_glTexBumpParameterivATI },
  { "glTexCoord1hNV", NULL, (void *) wine_glTexCoord1hNV },
  { "glTexCoord1hvNV", NULL, (void *) wine_glTexCoord1hvNV },
  { "glTexCoord2fColor3fVertex3fSUN", NULL, (void *) wine_glTexCoord2fColor3fVertex3fSUN },
  { "glTexCoord2fColor3fVertex3fvSUN", NULL, (void *) wine_glTexCoord2fColor3fVertex3fvSUN },
  { "glTexCoord2fColor4fNormal3fVertex3fSUN", NULL, (void *) wine_glTexCoord2fColor4fNormal3fVertex3fSUN },
  { "glTexCoord2fColor4fNormal3fVertex3fvSUN", NULL, (void *) wine_glTexCoord2fColor4fNormal3fVertex3fvSUN },
  { "glTexCoord2fColor4ubVertex3fSUN", NULL, (void *) wine_glTexCoord2fColor4ubVertex3fSUN },
  { "glTexCoord2fColor4ubVertex3fvSUN", NULL, (void *) wine_glTexCoord2fColor4ubVertex3fvSUN },
  { "glTexCoord2fNormal3fVertex3fSUN", NULL, (void *) wine_glTexCoord2fNormal3fVertex3fSUN },
  { "glTexCoord2fNormal3fVertex3fvSUN", NULL, (void *) wine_glTexCoord2fNormal3fVertex3fvSUN },
  { "glTexCoord2fVertex3fSUN", NULL, (void *) wine_glTexCoord2fVertex3fSUN },
  { "glTexCoord2fVertex3fvSUN", NULL, (void *) wine_glTexCoord2fVertex3fvSUN },
  { "glTexCoord2hNV", NULL, (void *) wine_glTexCoord2hNV },
  { "glTexCoord2hvNV", NULL, (void *) wine_glTexCoord2hvNV },
  { "glTexCoord3hNV", NULL, (void *) wine_glTexCoord3hNV },
  { "glTexCoord3hvNV", NULL, (void *) wine_glTexCoord3hvNV },
  { "glTexCoord4fColor4fNormal3fVertex4fSUN", NULL, (void *) wine_glTexCoord4fColor4fNormal3fVertex4fSUN },
  { "glTexCoord4fColor4fNormal3fVertex4fvSUN", NULL, (void *) wine_glTexCoord4fColor4fNormal3fVertex4fvSUN },
  { "glTexCoord4fVertex4fSUN", NULL, (void *) wine_glTexCoord4fVertex4fSUN },
  { "glTexCoord4fVertex4fvSUN", NULL, (void *) wine_glTexCoord4fVertex4fvSUN },
  { "glTexCoord4hNV", NULL, (void *) wine_glTexCoord4hNV },
  { "glTexCoord4hvNV", NULL, (void *) wine_glTexCoord4hvNV },
  { "glTexCoordPointerEXT", NULL, (void *) wine_glTexCoordPointerEXT },
  { "glTexCoordPointerListIBM", NULL, (void *) wine_glTexCoordPointerListIBM },
  { "glTexCoordPointervINTEL", NULL, (void *) wine_glTexCoordPointervINTEL },
  { "glTexFilterFuncSGIS", NULL, (void *) wine_glTexFilterFuncSGIS },
  { "glTexImage3DEXT", NULL, (void *) wine_glTexImage3DEXT },
  { "glTexImage4DSGIS", NULL, (void *) wine_glTexImage4DSGIS },
  { "glTexSubImage1DEXT", NULL, (void *) wine_glTexSubImage1DEXT },
  { "glTexSubImage2DEXT", NULL, (void *) wine_glTexSubImage2DEXT },
  { "glTexSubImage3DEXT", NULL, (void *) wine_glTexSubImage3DEXT },
  { "glTexSubImage4DSGIS", NULL, (void *) wine_glTexSubImage4DSGIS },
  { "glTextureColorMaskSGIS", NULL, (void *) wine_glTextureColorMaskSGIS },
  { "glTextureLightEXT", NULL, (void *) wine_glTextureLightEXT },
  { "glTextureMaterialEXT", NULL, (void *) wine_glTextureMaterialEXT },
  { "glTextureNormalEXT", NULL, (void *) wine_glTextureNormalEXT },
  { "glTrackMatrixNV", NULL, (void *) wine_glTrackMatrixNV },
  { "glUniform1f", NULL, (void *) wine_glUniform1f },
  { "glUniform1fARB", NULL, (void *) wine_glUniform1fARB },
  { "glUniform1fv", NULL, (void *) wine_glUniform1fv },
  { "glUniform1fvARB", NULL, (void *) wine_glUniform1fvARB },
  { "glUniform1i", NULL, (void *) wine_glUniform1i },
  { "glUniform1iARB", NULL, (void *) wine_glUniform1iARB },
  { "glUniform1iv", NULL, (void *) wine_glUniform1iv },
  { "glUniform1ivARB", NULL, (void *) wine_glUniform1ivARB },
  { "glUniform2f", NULL, (void *) wine_glUniform2f },
  { "glUniform2fARB", NULL, (void *) wine_glUniform2fARB },
  { "glUniform2fv", NULL, (void *) wine_glUniform2fv },
  { "glUniform2fvARB", NULL, (void *) wine_glUniform2fvARB },
  { "glUniform2i", NULL, (void *) wine_glUniform2i },
  { "glUniform2iARB", NULL, (void *) wine_glUniform2iARB },
  { "glUniform2iv", NULL, (void *) wine_glUniform2iv },
  { "glUniform2ivARB", NULL, (void *) wine_glUniform2ivARB },
  { "glUniform3f", NULL, (void *) wine_glUniform3f },
  { "glUniform3fARB", NULL, (void *) wine_glUniform3fARB },
  { "glUniform3fv", NULL, (void *) wine_glUniform3fv },
  { "glUniform3fvARB", NULL, (void *) wine_glUniform3fvARB },
  { "glUniform3i", NULL, (void *) wine_glUniform3i },
  { "glUniform3iARB", NULL, (void *) wine_glUniform3iARB },
  { "glUniform3iv", NULL, (void *) wine_glUniform3iv },
  { "glUniform3ivARB", NULL, (void *) wine_glUniform3ivARB },
  { "glUniform4f", NULL, (void *) wine_glUniform4f },
  { "glUniform4fARB", NULL, (void *) wine_glUniform4fARB },
  { "glUniform4fv", NULL, (void *) wine_glUniform4fv },
  { "glUniform4fvARB", NULL, (void *) wine_glUniform4fvARB },
  { "glUniform4i", NULL, (void *) wine_glUniform4i },
  { "glUniform4iARB", NULL, (void *) wine_glUniform4iARB },
  { "glUniform4iv", NULL, (void *) wine_glUniform4iv },
  { "glUniform4ivARB", NULL, (void *) wine_glUniform4ivARB },
  { "glUniformMatrix2fv", NULL, (void *) wine_glUniformMatrix2fv },
  { "glUniformMatrix2fvARB", NULL, (void *) wine_glUniformMatrix2fvARB },
  { "glUniformMatrix3fv", NULL, (void *) wine_glUniformMatrix3fv },
  { "glUniformMatrix3fvARB", NULL, (void *) wine_glUniformMatrix3fvARB },
  { "glUniformMatrix4fv", NULL, (void *) wine_glUniformMatrix4fv },
  { "glUniformMatrix4fvARB", NULL, (void *) wine_glUniformMatrix4fvARB },
  { "glUnlockArraysEXT", NULL, (void *) wine_glUnlockArraysEXT },
  { "glUnmapBuffer", NULL, (void *) wine_glUnmapBuffer },
  { "glUnmapBufferARB", NULL, (void *) wine_glUnmapBufferARB },
  { "glUnmapObjectBufferATI", NULL, (void *) wine_glUnmapObjectBufferATI },
  { "glUpdateObjectBufferATI", NULL, (void *) wine_glUpdateObjectBufferATI },
  { "glUseProgram", NULL, (void *) wine_glUseProgram },
  { "glUseProgramObjectARB", NULL, (void *) wine_glUseProgramObjectARB },
  { "glValidateProgram", NULL, (void *) wine_glValidateProgram },
  { "glValidateProgramARB", NULL, (void *) wine_glValidateProgramARB },
  { "glVariantArrayObjectATI", NULL, (void *) wine_glVariantArrayObjectATI },
  { "glVariantPointerEXT", NULL, (void *) wine_glVariantPointerEXT },
  { "glVariantbvEXT", NULL, (void *) wine_glVariantbvEXT },
  { "glVariantdvEXT", NULL, (void *) wine_glVariantdvEXT },
  { "glVariantfvEXT", NULL, (void *) wine_glVariantfvEXT },
  { "glVariantivEXT", NULL, (void *) wine_glVariantivEXT },
  { "glVariantsvEXT", NULL, (void *) wine_glVariantsvEXT },
  { "glVariantubvEXT", NULL, (void *) wine_glVariantubvEXT },
  { "glVariantuivEXT", NULL, (void *) wine_glVariantuivEXT },
  { "glVariantusvEXT", NULL, (void *) wine_glVariantusvEXT },
  { "glVertex2hNV", NULL, (void *) wine_glVertex2hNV },
  { "glVertex2hvNV", NULL, (void *) wine_glVertex2hvNV },
  { "glVertex3hNV", NULL, (void *) wine_glVertex3hNV },
  { "glVertex3hvNV", NULL, (void *) wine_glVertex3hvNV },
  { "glVertex4hNV", NULL, (void *) wine_glVertex4hNV },
  { "glVertex4hvNV", NULL, (void *) wine_glVertex4hvNV },
  { "glVertexArrayParameteriAPPLE", NULL, (void *) wine_glVertexArrayParameteriAPPLE },
  { "glVertexArrayRangeAPPLE", NULL, (void *) wine_glVertexArrayRangeAPPLE },
  { "glVertexArrayRangeNV", NULL, (void *) wine_glVertexArrayRangeNV },
  { "glVertexAttrib1d", NULL, (void *) wine_glVertexAttrib1d },
  { "glVertexAttrib1dARB", NULL, (void *) wine_glVertexAttrib1dARB },
  { "glVertexAttrib1dNV", NULL, (void *) wine_glVertexAttrib1dNV },
  { "glVertexAttrib1dv", NULL, (void *) wine_glVertexAttrib1dv },
  { "glVertexAttrib1dvARB", NULL, (void *) wine_glVertexAttrib1dvARB },
  { "glVertexAttrib1dvNV", NULL, (void *) wine_glVertexAttrib1dvNV },
  { "glVertexAttrib1f", NULL, (void *) wine_glVertexAttrib1f },
  { "glVertexAttrib1fARB", NULL, (void *) wine_glVertexAttrib1fARB },
  { "glVertexAttrib1fNV", NULL, (void *) wine_glVertexAttrib1fNV },
  { "glVertexAttrib1fv", NULL, (void *) wine_glVertexAttrib1fv },
  { "glVertexAttrib1fvARB", NULL, (void *) wine_glVertexAttrib1fvARB },
  { "glVertexAttrib1fvNV", NULL, (void *) wine_glVertexAttrib1fvNV },
  { "glVertexAttrib1hNV", NULL, (void *) wine_glVertexAttrib1hNV },
  { "glVertexAttrib1hvNV", NULL, (void *) wine_glVertexAttrib1hvNV },
  { "glVertexAttrib1s", NULL, (void *) wine_glVertexAttrib1s },
  { "glVertexAttrib1sARB", NULL, (void *) wine_glVertexAttrib1sARB },
  { "glVertexAttrib1sNV", NULL, (void *) wine_glVertexAttrib1sNV },
  { "glVertexAttrib1sv", NULL, (void *) wine_glVertexAttrib1sv },
  { "glVertexAttrib1svARB", NULL, (void *) wine_glVertexAttrib1svARB },
  { "glVertexAttrib1svNV", NULL, (void *) wine_glVertexAttrib1svNV },
  { "glVertexAttrib2d", NULL, (void *) wine_glVertexAttrib2d },
  { "glVertexAttrib2dARB", NULL, (void *) wine_glVertexAttrib2dARB },
  { "glVertexAttrib2dNV", NULL, (void *) wine_glVertexAttrib2dNV },
  { "glVertexAttrib2dv", NULL, (void *) wine_glVertexAttrib2dv },
  { "glVertexAttrib2dvARB", NULL, (void *) wine_glVertexAttrib2dvARB },
  { "glVertexAttrib2dvNV", NULL, (void *) wine_glVertexAttrib2dvNV },
  { "glVertexAttrib2f", NULL, (void *) wine_glVertexAttrib2f },
  { "glVertexAttrib2fARB", NULL, (void *) wine_glVertexAttrib2fARB },
  { "glVertexAttrib2fNV", NULL, (void *) wine_glVertexAttrib2fNV },
  { "glVertexAttrib2fv", NULL, (void *) wine_glVertexAttrib2fv },
  { "glVertexAttrib2fvARB", NULL, (void *) wine_glVertexAttrib2fvARB },
  { "glVertexAttrib2fvNV", NULL, (void *) wine_glVertexAttrib2fvNV },
  { "glVertexAttrib2hNV", NULL, (void *) wine_glVertexAttrib2hNV },
  { "glVertexAttrib2hvNV", NULL, (void *) wine_glVertexAttrib2hvNV },
  { "glVertexAttrib2s", NULL, (void *) wine_glVertexAttrib2s },
  { "glVertexAttrib2sARB", NULL, (void *) wine_glVertexAttrib2sARB },
  { "glVertexAttrib2sNV", NULL, (void *) wine_glVertexAttrib2sNV },
  { "glVertexAttrib2sv", NULL, (void *) wine_glVertexAttrib2sv },
  { "glVertexAttrib2svARB", NULL, (void *) wine_glVertexAttrib2svARB },
  { "glVertexAttrib2svNV", NULL, (void *) wine_glVertexAttrib2svNV },
  { "glVertexAttrib3d", NULL, (void *) wine_glVertexAttrib3d },
  { "glVertexAttrib3dARB", NULL, (void *) wine_glVertexAttrib3dARB },
  { "glVertexAttrib3dNV", NULL, (void *) wine_glVertexAttrib3dNV },
  { "glVertexAttrib3dv", NULL, (void *) wine_glVertexAttrib3dv },
  { "glVertexAttrib3dvARB", NULL, (void *) wine_glVertexAttrib3dvARB },
  { "glVertexAttrib3dvNV", NULL, (void *) wine_glVertexAttrib3dvNV },
  { "glVertexAttrib3f", NULL, (void *) wine_glVertexAttrib3f },
  { "glVertexAttrib3fARB", NULL, (void *) wine_glVertexAttrib3fARB },
  { "glVertexAttrib3fNV", NULL, (void *) wine_glVertexAttrib3fNV },
  { "glVertexAttrib3fv", NULL, (void *) wine_glVertexAttrib3fv },
  { "glVertexAttrib3fvARB", NULL, (void *) wine_glVertexAttrib3fvARB },
  { "glVertexAttrib3fvNV", NULL, (void *) wine_glVertexAttrib3fvNV },
  { "glVertexAttrib3hNV", NULL, (void *) wine_glVertexAttrib3hNV },
  { "glVertexAttrib3hvNV", NULL, (void *) wine_glVertexAttrib3hvNV },
  { "glVertexAttrib3s", NULL, (void *) wine_glVertexAttrib3s },
  { "glVertexAttrib3sARB", NULL, (void *) wine_glVertexAttrib3sARB },
  { "glVertexAttrib3sNV", NULL, (void *) wine_glVertexAttrib3sNV },
  { "glVertexAttrib3sv", NULL, (void *) wine_glVertexAttrib3sv },
  { "glVertexAttrib3svARB", NULL, (void *) wine_glVertexAttrib3svARB },
  { "glVertexAttrib3svNV", NULL, (void *) wine_glVertexAttrib3svNV },
  { "glVertexAttrib4Nbv", NULL, (void *) wine_glVertexAttrib4Nbv },
  { "glVertexAttrib4NbvARB", NULL, (void *) wine_glVertexAttrib4NbvARB },
  { "glVertexAttrib4Niv", NULL, (void *) wine_glVertexAttrib4Niv },
  { "glVertexAttrib4NivARB", NULL, (void *) wine_glVertexAttrib4NivARB },
  { "glVertexAttrib4Nsv", NULL, (void *) wine_glVertexAttrib4Nsv },
  { "glVertexAttrib4NsvARB", NULL, (void *) wine_glVertexAttrib4NsvARB },
  { "glVertexAttrib4Nub", NULL, (void *) wine_glVertexAttrib4Nub },
  { "glVertexAttrib4NubARB", NULL, (void *) wine_glVertexAttrib4NubARB },
  { "glVertexAttrib4Nubv", NULL, (void *) wine_glVertexAttrib4Nubv },
  { "glVertexAttrib4NubvARB", NULL, (void *) wine_glVertexAttrib4NubvARB },
  { "glVertexAttrib4Nuiv", NULL, (void *) wine_glVertexAttrib4Nuiv },
  { "glVertexAttrib4NuivARB", NULL, (void *) wine_glVertexAttrib4NuivARB },
  { "glVertexAttrib4Nusv", NULL, (void *) wine_glVertexAttrib4Nusv },
  { "glVertexAttrib4NusvARB", NULL, (void *) wine_glVertexAttrib4NusvARB },
  { "glVertexAttrib4bv", NULL, (void *) wine_glVertexAttrib4bv },
  { "glVertexAttrib4bvARB", NULL, (void *) wine_glVertexAttrib4bvARB },
  { "glVertexAttrib4d", NULL, (void *) wine_glVertexAttrib4d },
  { "glVertexAttrib4dARB", NULL, (void *) wine_glVertexAttrib4dARB },
  { "glVertexAttrib4dNV", NULL, (void *) wine_glVertexAttrib4dNV },
  { "glVertexAttrib4dv", NULL, (void *) wine_glVertexAttrib4dv },
  { "glVertexAttrib4dvARB", NULL, (void *) wine_glVertexAttrib4dvARB },
  { "glVertexAttrib4dvNV", NULL, (void *) wine_glVertexAttrib4dvNV },
  { "glVertexAttrib4f", NULL, (void *) wine_glVertexAttrib4f },
  { "glVertexAttrib4fARB", NULL, (void *) wine_glVertexAttrib4fARB },
  { "glVertexAttrib4fNV", NULL, (void *) wine_glVertexAttrib4fNV },
  { "glVertexAttrib4fv", NULL, (void *) wine_glVertexAttrib4fv },
  { "glVertexAttrib4fvARB", NULL, (void *) wine_glVertexAttrib4fvARB },
  { "glVertexAttrib4fvNV", NULL, (void *) wine_glVertexAttrib4fvNV },
  { "glVertexAttrib4hNV", NULL, (void *) wine_glVertexAttrib4hNV },
  { "glVertexAttrib4hvNV", NULL, (void *) wine_glVertexAttrib4hvNV },
  { "glVertexAttrib4iv", NULL, (void *) wine_glVertexAttrib4iv },
  { "glVertexAttrib4ivARB", NULL, (void *) wine_glVertexAttrib4ivARB },
  { "glVertexAttrib4s", NULL, (void *) wine_glVertexAttrib4s },
  { "glVertexAttrib4sARB", NULL, (void *) wine_glVertexAttrib4sARB },
  { "glVertexAttrib4sNV", NULL, (void *) wine_glVertexAttrib4sNV },
  { "glVertexAttrib4sv", NULL, (void *) wine_glVertexAttrib4sv },
  { "glVertexAttrib4svARB", NULL, (void *) wine_glVertexAttrib4svARB },
  { "glVertexAttrib4svNV", NULL, (void *) wine_glVertexAttrib4svNV },
  { "glVertexAttrib4ubNV", NULL, (void *) wine_glVertexAttrib4ubNV },
  { "glVertexAttrib4ubv", NULL, (void *) wine_glVertexAttrib4ubv },
  { "glVertexAttrib4ubvARB", NULL, (void *) wine_glVertexAttrib4ubvARB },
  { "glVertexAttrib4ubvNV", NULL, (void *) wine_glVertexAttrib4ubvNV },
  { "glVertexAttrib4uiv", NULL, (void *) wine_glVertexAttrib4uiv },
  { "glVertexAttrib4uivARB", NULL, (void *) wine_glVertexAttrib4uivARB },
  { "glVertexAttrib4usv", NULL, (void *) wine_glVertexAttrib4usv },
  { "glVertexAttrib4usvARB", NULL, (void *) wine_glVertexAttrib4usvARB },
  { "glVertexAttribArrayObjectATI", NULL, (void *) wine_glVertexAttribArrayObjectATI },
  { "glVertexAttribPointer", NULL, (void *) wine_glVertexAttribPointer },
  { "glVertexAttribPointerARB", NULL, (void *) wine_glVertexAttribPointerARB },
  { "glVertexAttribPointerNV", NULL, (void *) wine_glVertexAttribPointerNV },
  { "glVertexAttribs1dvNV", NULL, (void *) wine_glVertexAttribs1dvNV },
  { "glVertexAttribs1fvNV", NULL, (void *) wine_glVertexAttribs1fvNV },
  { "glVertexAttribs1hvNV", NULL, (void *) wine_glVertexAttribs1hvNV },
  { "glVertexAttribs1svNV", NULL, (void *) wine_glVertexAttribs1svNV },
  { "glVertexAttribs2dvNV", NULL, (void *) wine_glVertexAttribs2dvNV },
  { "glVertexAttribs2fvNV", NULL, (void *) wine_glVertexAttribs2fvNV },
  { "glVertexAttribs2hvNV", NULL, (void *) wine_glVertexAttribs2hvNV },
  { "glVertexAttribs2svNV", NULL, (void *) wine_glVertexAttribs2svNV },
  { "glVertexAttribs3dvNV", NULL, (void *) wine_glVertexAttribs3dvNV },
  { "glVertexAttribs3fvNV", NULL, (void *) wine_glVertexAttribs3fvNV },
  { "glVertexAttribs3hvNV", NULL, (void *) wine_glVertexAttribs3hvNV },
  { "glVertexAttribs3svNV", NULL, (void *) wine_glVertexAttribs3svNV },
  { "glVertexAttribs4dvNV", NULL, (void *) wine_glVertexAttribs4dvNV },
  { "glVertexAttribs4fvNV", NULL, (void *) wine_glVertexAttribs4fvNV },
  { "glVertexAttribs4hvNV", NULL, (void *) wine_glVertexAttribs4hvNV },
  { "glVertexAttribs4svNV", NULL, (void *) wine_glVertexAttribs4svNV },
  { "glVertexAttribs4ubvNV", NULL, (void *) wine_glVertexAttribs4ubvNV },
  { "glVertexBlendARB", NULL, (void *) wine_glVertexBlendARB },
  { "glVertexBlendEnvfATI", NULL, (void *) wine_glVertexBlendEnvfATI },
  { "glVertexBlendEnviATI", NULL, (void *) wine_glVertexBlendEnviATI },
  { "glVertexPointerEXT", NULL, (void *) wine_glVertexPointerEXT },
  { "glVertexPointerListIBM", NULL, (void *) wine_glVertexPointerListIBM },
  { "glVertexPointervINTEL", NULL, (void *) wine_glVertexPointervINTEL },
  { "glVertexStream1dATI", NULL, (void *) wine_glVertexStream1dATI },
  { "glVertexStream1dvATI", NULL, (void *) wine_glVertexStream1dvATI },
  { "glVertexStream1fATI", NULL, (void *) wine_glVertexStream1fATI },
  { "glVertexStream1fvATI", NULL, (void *) wine_glVertexStream1fvATI },
  { "glVertexStream1iATI", NULL, (void *) wine_glVertexStream1iATI },
  { "glVertexStream1ivATI", NULL, (void *) wine_glVertexStream1ivATI },
  { "glVertexStream1sATI", NULL, (void *) wine_glVertexStream1sATI },
  { "glVertexStream1svATI", NULL, (void *) wine_glVertexStream1svATI },
  { "glVertexStream2dATI", NULL, (void *) wine_glVertexStream2dATI },
  { "glVertexStream2dvATI", NULL, (void *) wine_glVertexStream2dvATI },
  { "glVertexStream2fATI", NULL, (void *) wine_glVertexStream2fATI },
  { "glVertexStream2fvATI", NULL, (void *) wine_glVertexStream2fvATI },
  { "glVertexStream2iATI", NULL, (void *) wine_glVertexStream2iATI },
  { "glVertexStream2ivATI", NULL, (void *) wine_glVertexStream2ivATI },
  { "glVertexStream2sATI", NULL, (void *) wine_glVertexStream2sATI },
  { "glVertexStream2svATI", NULL, (void *) wine_glVertexStream2svATI },
  { "glVertexStream3dATI", NULL, (void *) wine_glVertexStream3dATI },
  { "glVertexStream3dvATI", NULL, (void *) wine_glVertexStream3dvATI },
  { "glVertexStream3fATI", NULL, (void *) wine_glVertexStream3fATI },
  { "glVertexStream3fvATI", NULL, (void *) wine_glVertexStream3fvATI },
  { "glVertexStream3iATI", NULL, (void *) wine_glVertexStream3iATI },
  { "glVertexStream3ivATI", NULL, (void *) wine_glVertexStream3ivATI },
  { "glVertexStream3sATI", NULL, (void *) wine_glVertexStream3sATI },
  { "glVertexStream3svATI", NULL, (void *) wine_glVertexStream3svATI },
  { "glVertexStream4dATI", NULL, (void *) wine_glVertexStream4dATI },
  { "glVertexStream4dvATI", NULL, (void *) wine_glVertexStream4dvATI },
  { "glVertexStream4fATI", NULL, (void *) wine_glVertexStream4fATI },
  { "glVertexStream4fvATI", NULL, (void *) wine_glVertexStream4fvATI },
  { "glVertexStream4iATI", NULL, (void *) wine_glVertexStream4iATI },
  { "glVertexStream4ivATI", NULL, (void *) wine_glVertexStream4ivATI },
  { "glVertexStream4sATI", NULL, (void *) wine_glVertexStream4sATI },
  { "glVertexStream4svATI", NULL, (void *) wine_glVertexStream4svATI },
  { "glVertexWeightPointerEXT", NULL, (void *) wine_glVertexWeightPointerEXT },
  { "glVertexWeightfEXT", NULL, (void *) wine_glVertexWeightfEXT },
  { "glVertexWeightfvEXT", NULL, (void *) wine_glVertexWeightfvEXT },
  { "glVertexWeighthNV", NULL, (void *) wine_glVertexWeighthNV },
  { "glVertexWeighthvNV", NULL, (void *) wine_glVertexWeighthvNV },
  { "glWeightPointerARB", NULL, (void *) wine_glWeightPointerARB },
  { "glWeightbvARB", NULL, (void *) wine_glWeightbvARB },
  { "glWeightdvARB", NULL, (void *) wine_glWeightdvARB },
  { "glWeightfvARB", NULL, (void *) wine_glWeightfvARB },
  { "glWeightivARB", NULL, (void *) wine_glWeightivARB },
  { "glWeightsvARB", NULL, (void *) wine_glWeightsvARB },
  { "glWeightubvARB", NULL, (void *) wine_glWeightubvARB },
  { "glWeightuivARB", NULL, (void *) wine_glWeightuivARB },
  { "glWeightusvARB", NULL, (void *) wine_glWeightusvARB },
  { "glWindowPos2d", NULL, (void *) wine_glWindowPos2d },
  { "glWindowPos2dARB", NULL, (void *) wine_glWindowPos2dARB },
  { "glWindowPos2dMESA", NULL, (void *) wine_glWindowPos2dMESA },
  { "glWindowPos2dv", NULL, (void *) wine_glWindowPos2dv },
  { "glWindowPos2dvARB", NULL, (void *) wine_glWindowPos2dvARB },
  { "glWindowPos2dvMESA", NULL, (void *) wine_glWindowPos2dvMESA },
  { "glWindowPos2f", NULL, (void *) wine_glWindowPos2f },
  { "glWindowPos2fARB", NULL, (void *) wine_glWindowPos2fARB },
  { "glWindowPos2fMESA", NULL, (void *) wine_glWindowPos2fMESA },
  { "glWindowPos2fv", NULL, (void *) wine_glWindowPos2fv },
  { "glWindowPos2fvARB", NULL, (void *) wine_glWindowPos2fvARB },
  { "glWindowPos2fvMESA", NULL, (void *) wine_glWindowPos2fvMESA },
  { "glWindowPos2i", NULL, (void *) wine_glWindowPos2i },
  { "glWindowPos2iARB", NULL, (void *) wine_glWindowPos2iARB },
  { "glWindowPos2iMESA", NULL, (void *) wine_glWindowPos2iMESA },
  { "glWindowPos2iv", NULL, (void *) wine_glWindowPos2iv },
  { "glWindowPos2ivARB", NULL, (void *) wine_glWindowPos2ivARB },
  { "glWindowPos2ivMESA", NULL, (void *) wine_glWindowPos2ivMESA },
  { "glWindowPos2s", NULL, (void *) wine_glWindowPos2s },
  { "glWindowPos2sARB", NULL, (void *) wine_glWindowPos2sARB },
  { "glWindowPos2sMESA", NULL, (void *) wine_glWindowPos2sMESA },
  { "glWindowPos2sv", NULL, (void *) wine_glWindowPos2sv },
  { "glWindowPos2svARB", NULL, (void *) wine_glWindowPos2svARB },
  { "glWindowPos2svMESA", NULL, (void *) wine_glWindowPos2svMESA },
  { "glWindowPos3d", NULL, (void *) wine_glWindowPos3d },
  { "glWindowPos3dARB", NULL, (void *) wine_glWindowPos3dARB },
  { "glWindowPos3dMESA", NULL, (void *) wine_glWindowPos3dMESA },
  { "glWindowPos3dv", NULL, (void *) wine_glWindowPos3dv },
  { "glWindowPos3dvARB", NULL, (void *) wine_glWindowPos3dvARB },
  { "glWindowPos3dvMESA", NULL, (void *) wine_glWindowPos3dvMESA },
  { "glWindowPos3f", NULL, (void *) wine_glWindowPos3f },
  { "glWindowPos3fARB", NULL, (void *) wine_glWindowPos3fARB },
  { "glWindowPos3fMESA", NULL, (void *) wine_glWindowPos3fMESA },
  { "glWindowPos3fv", NULL, (void *) wine_glWindowPos3fv },
  { "glWindowPos3fvARB", NULL, (void *) wine_glWindowPos3fvARB },
  { "glWindowPos3fvMESA", NULL, (void *) wine_glWindowPos3fvMESA },
  { "glWindowPos3i", NULL, (void *) wine_glWindowPos3i },
  { "glWindowPos3iARB", NULL, (void *) wine_glWindowPos3iARB },
  { "glWindowPos3iMESA", NULL, (void *) wine_glWindowPos3iMESA },
  { "glWindowPos3iv", NULL, (void *) wine_glWindowPos3iv },
  { "glWindowPos3ivARB", NULL, (void *) wine_glWindowPos3ivARB },
  { "glWindowPos3ivMESA", NULL, (void *) wine_glWindowPos3ivMESA },
  { "glWindowPos3s", NULL, (void *) wine_glWindowPos3s },
  { "glWindowPos3sARB", NULL, (void *) wine_glWindowPos3sARB },
  { "glWindowPos3sMESA", NULL, (void *) wine_glWindowPos3sMESA },
  { "glWindowPos3sv", NULL, (void *) wine_glWindowPos3sv },
  { "glWindowPos3svARB", NULL, (void *) wine_glWindowPos3svARB },
  { "glWindowPos3svMESA", NULL, (void *) wine_glWindowPos3svMESA },
  { "glWindowPos4dMESA", NULL, (void *) wine_glWindowPos4dMESA },
  { "glWindowPos4dvMESA", NULL, (void *) wine_glWindowPos4dvMESA },
  { "glWindowPos4fMESA", NULL, (void *) wine_glWindowPos4fMESA },
  { "glWindowPos4fvMESA", NULL, (void *) wine_glWindowPos4fvMESA },
  { "glWindowPos4iMESA", NULL, (void *) wine_glWindowPos4iMESA },
  { "glWindowPos4ivMESA", NULL, (void *) wine_glWindowPos4ivMESA },
  { "glWindowPos4sMESA", NULL, (void *) wine_glWindowPos4sMESA },
  { "glWindowPos4svMESA", NULL, (void *) wine_glWindowPos4svMESA },
  { "glWriteMaskEXT", NULL, (void *) wine_glWriteMaskEXT },
  { "wglAllocateMemoryNV", "glXAllocateMemoryNV", (void *) wine_wglAllocateMemoryNV },
  { "wglFreeMemoryNV", "glXFreeMemoryNV", (void *) wine_wglFreeMemoryNV }
};
