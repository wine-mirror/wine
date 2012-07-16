
/* Auto-generated file... Do not edit ! */

#include "config.h"
#include <stdarg.h>
#include "opengl_ext.h"
#include "winternl.h"
#define WGL_WGLEXT_PROTOTYPES
#include "wine/wglext.h"
#include "wine/wgl_driver.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(opengl);

const int extension_registry_size = 2085;

static void WINAPI wine_glActiveProgramEXT( GLuint program ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", program );
  funcs->ext.p_glActiveProgramEXT( program );
}

static void WINAPI wine_glActiveShaderProgram( GLuint pipeline, GLuint program ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", pipeline, program );
  funcs->ext.p_glActiveShaderProgram( pipeline, program );
}

static void WINAPI wine_glActiveStencilFaceEXT( GLenum face ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", face );
  funcs->ext.p_glActiveStencilFaceEXT( face );
}

static void WINAPI wine_glActiveTexture( GLenum texture ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", texture );
  funcs->ext.p_glActiveTexture( texture );
}

static void WINAPI wine_glActiveTextureARB( GLenum texture ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", texture );
  funcs->ext.p_glActiveTextureARB( texture );
}

static void WINAPI wine_glActiveVaryingNV( GLuint program, const char* name ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", program, name );
  funcs->ext.p_glActiveVaryingNV( program, name );
}

static void WINAPI wine_glAlphaFragmentOp1ATI( GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d)\n", op, dst, dstMod, arg1, arg1Rep, arg1Mod );
  funcs->ext.p_glAlphaFragmentOp1ATI( op, dst, dstMod, arg1, arg1Rep, arg1Mod );
}

static void WINAPI wine_glAlphaFragmentOp2ATI( GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d)\n", op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod );
  funcs->ext.p_glAlphaFragmentOp2ATI( op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod );
}

static void WINAPI wine_glAlphaFragmentOp3ATI( GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod );
  funcs->ext.p_glAlphaFragmentOp3ATI( op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod );
}

static void WINAPI wine_glApplyTextureEXT( GLenum mode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", mode );
  funcs->ext.p_glApplyTextureEXT( mode );
}

static GLboolean WINAPI wine_glAreProgramsResidentNV( GLsizei n, const GLuint* programs, GLboolean* residences ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p, %p)\n", n, programs, residences );
  return funcs->ext.p_glAreProgramsResidentNV( n, programs, residences );
}

static GLboolean WINAPI wine_glAreTexturesResidentEXT( GLsizei n, const GLuint* textures, GLboolean* residences ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p, %p)\n", n, textures, residences );
  return funcs->ext.p_glAreTexturesResidentEXT( n, textures, residences );
}

static void WINAPI wine_glArrayElementEXT( GLint i ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", i );
  funcs->ext.p_glArrayElementEXT( i );
}

static void WINAPI wine_glArrayObjectATI( GLenum array, GLint size, GLenum type, GLsizei stride, GLuint buffer, GLuint offset ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d)\n", array, size, type, stride, buffer, offset );
  funcs->ext.p_glArrayObjectATI( array, size, type, stride, buffer, offset );
}

static void WINAPI wine_glAsyncMarkerSGIX( GLuint marker ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", marker );
  funcs->ext.p_glAsyncMarkerSGIX( marker );
}

static void WINAPI wine_glAttachObjectARB( unsigned int containerObj, unsigned int obj ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", containerObj, obj );
  funcs->ext.p_glAttachObjectARB( containerObj, obj );
}

static void WINAPI wine_glAttachShader( GLuint program, GLuint shader ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", program, shader );
  funcs->ext.p_glAttachShader( program, shader );
}

static void WINAPI wine_glBeginConditionalRender( GLuint id, GLenum mode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", id, mode );
  funcs->ext.p_glBeginConditionalRender( id, mode );
}

static void WINAPI wine_glBeginConditionalRenderNV( GLuint id, GLenum mode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", id, mode );
  funcs->ext.p_glBeginConditionalRenderNV( id, mode );
}

static void WINAPI wine_glBeginFragmentShaderATI( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  funcs->ext.p_glBeginFragmentShaderATI( );
}

static void WINAPI wine_glBeginOcclusionQueryNV( GLuint id ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", id );
  funcs->ext.p_glBeginOcclusionQueryNV( id );
}

static void WINAPI wine_glBeginPerfMonitorAMD( GLuint monitor ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", monitor );
  funcs->ext.p_glBeginPerfMonitorAMD( monitor );
}

static void WINAPI wine_glBeginQuery( GLenum target, GLuint id ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", target, id );
  funcs->ext.p_glBeginQuery( target, id );
}

static void WINAPI wine_glBeginQueryARB( GLenum target, GLuint id ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", target, id );
  funcs->ext.p_glBeginQueryARB( target, id );
}

static void WINAPI wine_glBeginQueryIndexed( GLenum target, GLuint index, GLuint id ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", target, index, id );
  funcs->ext.p_glBeginQueryIndexed( target, index, id );
}

static void WINAPI wine_glBeginTransformFeedback( GLenum primitiveMode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", primitiveMode );
  funcs->ext.p_glBeginTransformFeedback( primitiveMode );
}

static void WINAPI wine_glBeginTransformFeedbackEXT( GLenum primitiveMode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", primitiveMode );
  funcs->ext.p_glBeginTransformFeedbackEXT( primitiveMode );
}

static void WINAPI wine_glBeginTransformFeedbackNV( GLenum primitiveMode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", primitiveMode );
  funcs->ext.p_glBeginTransformFeedbackNV( primitiveMode );
}

static void WINAPI wine_glBeginVertexShaderEXT( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  funcs->ext.p_glBeginVertexShaderEXT( );
}

static void WINAPI wine_glBeginVideoCaptureNV( GLuint video_capture_slot ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", video_capture_slot );
  funcs->ext.p_glBeginVideoCaptureNV( video_capture_slot );
}

static void WINAPI wine_glBindAttribLocation( GLuint program, GLuint index, const char* name ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", program, index, name );
  funcs->ext.p_glBindAttribLocation( program, index, name );
}

static void WINAPI wine_glBindAttribLocationARB( unsigned int programObj, GLuint index, const char* name ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", programObj, index, name );
  funcs->ext.p_glBindAttribLocationARB( programObj, index, name );
}

static void WINAPI wine_glBindBuffer( GLenum target, GLuint buffer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", target, buffer );
  funcs->ext.p_glBindBuffer( target, buffer );
}

static void WINAPI wine_glBindBufferARB( GLenum target, GLuint buffer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", target, buffer );
  funcs->ext.p_glBindBufferARB( target, buffer );
}

static void WINAPI wine_glBindBufferBase( GLenum target, GLuint index, GLuint buffer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", target, index, buffer );
  funcs->ext.p_glBindBufferBase( target, index, buffer );
}

static void WINAPI wine_glBindBufferBaseEXT( GLenum target, GLuint index, GLuint buffer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", target, index, buffer );
  funcs->ext.p_glBindBufferBaseEXT( target, index, buffer );
}

static void WINAPI wine_glBindBufferBaseNV( GLenum target, GLuint index, GLuint buffer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", target, index, buffer );
  funcs->ext.p_glBindBufferBaseNV( target, index, buffer );
}

static void WINAPI wine_glBindBufferOffsetEXT( GLenum target, GLuint index, GLuint buffer, INT_PTR offset ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %ld)\n", target, index, buffer, offset );
  funcs->ext.p_glBindBufferOffsetEXT( target, index, buffer, offset );
}

static void WINAPI wine_glBindBufferOffsetNV( GLenum target, GLuint index, GLuint buffer, INT_PTR offset ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %ld)\n", target, index, buffer, offset );
  funcs->ext.p_glBindBufferOffsetNV( target, index, buffer, offset );
}

static void WINAPI wine_glBindBufferRange( GLenum target, GLuint index, GLuint buffer, INT_PTR offset, INT_PTR size ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %ld, %ld)\n", target, index, buffer, offset, size );
  funcs->ext.p_glBindBufferRange( target, index, buffer, offset, size );
}

static void WINAPI wine_glBindBufferRangeEXT( GLenum target, GLuint index, GLuint buffer, INT_PTR offset, INT_PTR size ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %ld, %ld)\n", target, index, buffer, offset, size );
  funcs->ext.p_glBindBufferRangeEXT( target, index, buffer, offset, size );
}

static void WINAPI wine_glBindBufferRangeNV( GLenum target, GLuint index, GLuint buffer, INT_PTR offset, INT_PTR size ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %ld, %ld)\n", target, index, buffer, offset, size );
  funcs->ext.p_glBindBufferRangeNV( target, index, buffer, offset, size );
}

static void WINAPI wine_glBindFragDataLocation( GLuint program, GLuint color, const char* name ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", program, color, name );
  funcs->ext.p_glBindFragDataLocation( program, color, name );
}

static void WINAPI wine_glBindFragDataLocationEXT( GLuint program, GLuint color, const char* name ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", program, color, name );
  funcs->ext.p_glBindFragDataLocationEXT( program, color, name );
}

static void WINAPI wine_glBindFragDataLocationIndexed( GLuint program, GLuint colorNumber, GLuint index, const char* name ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, colorNumber, index, name );
  funcs->ext.p_glBindFragDataLocationIndexed( program, colorNumber, index, name );
}

static void WINAPI wine_glBindFragmentShaderATI( GLuint id ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", id );
  funcs->ext.p_glBindFragmentShaderATI( id );
}

static void WINAPI wine_glBindFramebuffer( GLenum target, GLuint framebuffer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", target, framebuffer );
  funcs->ext.p_glBindFramebuffer( target, framebuffer );
}

static void WINAPI wine_glBindFramebufferEXT( GLenum target, GLuint framebuffer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", target, framebuffer );
  funcs->ext.p_glBindFramebufferEXT( target, framebuffer );
}

static void WINAPI wine_glBindImageTexture( GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d)\n", unit, texture, level, layered, layer, access, format );
  funcs->ext.p_glBindImageTexture( unit, texture, level, layered, layer, access, format );
}

static void WINAPI wine_glBindImageTextureEXT( GLuint index, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLint format ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d)\n", index, texture, level, layered, layer, access, format );
  funcs->ext.p_glBindImageTextureEXT( index, texture, level, layered, layer, access, format );
}

static GLuint WINAPI wine_glBindLightParameterEXT( GLenum light, GLenum value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", light, value );
  return funcs->ext.p_glBindLightParameterEXT( light, value );
}

static GLuint WINAPI wine_glBindMaterialParameterEXT( GLenum face, GLenum value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", face, value );
  return funcs->ext.p_glBindMaterialParameterEXT( face, value );
}

static void WINAPI wine_glBindMultiTextureEXT( GLenum texunit, GLenum target, GLuint texture ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", texunit, target, texture );
  funcs->ext.p_glBindMultiTextureEXT( texunit, target, texture );
}

static GLuint WINAPI wine_glBindParameterEXT( GLenum value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", value );
  return funcs->ext.p_glBindParameterEXT( value );
}

static void WINAPI wine_glBindProgramARB( GLenum target, GLuint program ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", target, program );
  funcs->ext.p_glBindProgramARB( target, program );
}

static void WINAPI wine_glBindProgramNV( GLenum target, GLuint id ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", target, id );
  funcs->ext.p_glBindProgramNV( target, id );
}

static void WINAPI wine_glBindProgramPipeline( GLuint pipeline ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", pipeline );
  funcs->ext.p_glBindProgramPipeline( pipeline );
}

static void WINAPI wine_glBindRenderbuffer( GLenum target, GLuint renderbuffer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", target, renderbuffer );
  funcs->ext.p_glBindRenderbuffer( target, renderbuffer );
}

static void WINAPI wine_glBindRenderbufferEXT( GLenum target, GLuint renderbuffer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", target, renderbuffer );
  funcs->ext.p_glBindRenderbufferEXT( target, renderbuffer );
}

static void WINAPI wine_glBindSampler( GLuint unit, GLuint sampler ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", unit, sampler );
  funcs->ext.p_glBindSampler( unit, sampler );
}

static GLuint WINAPI wine_glBindTexGenParameterEXT( GLenum unit, GLenum coord, GLenum value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", unit, coord, value );
  return funcs->ext.p_glBindTexGenParameterEXT( unit, coord, value );
}

static void WINAPI wine_glBindTextureEXT( GLenum target, GLuint texture ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", target, texture );
  funcs->ext.p_glBindTextureEXT( target, texture );
}

static GLuint WINAPI wine_glBindTextureUnitParameterEXT( GLenum unit, GLenum value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", unit, value );
  return funcs->ext.p_glBindTextureUnitParameterEXT( unit, value );
}

static void WINAPI wine_glBindTransformFeedback( GLenum target, GLuint id ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", target, id );
  funcs->ext.p_glBindTransformFeedback( target, id );
}

static void WINAPI wine_glBindTransformFeedbackNV( GLenum target, GLuint id ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", target, id );
  funcs->ext.p_glBindTransformFeedbackNV( target, id );
}

static void WINAPI wine_glBindVertexArray( GLuint array ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", array );
  funcs->ext.p_glBindVertexArray( array );
}

static void WINAPI wine_glBindVertexArrayAPPLE( GLuint array ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", array );
  funcs->ext.p_glBindVertexArrayAPPLE( array );
}

static void WINAPI wine_glBindVertexShaderEXT( GLuint id ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", id );
  funcs->ext.p_glBindVertexShaderEXT( id );
}

static void WINAPI wine_glBindVideoCaptureStreamBufferNV( GLuint video_capture_slot, GLuint stream, GLenum frame_region, INT_PTR offset ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %ld)\n", video_capture_slot, stream, frame_region, offset );
  funcs->ext.p_glBindVideoCaptureStreamBufferNV( video_capture_slot, stream, frame_region, offset );
}

static void WINAPI wine_glBindVideoCaptureStreamTextureNV( GLuint video_capture_slot, GLuint stream, GLenum frame_region, GLenum target, GLuint texture ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", video_capture_slot, stream, frame_region, target, texture );
  funcs->ext.p_glBindVideoCaptureStreamTextureNV( video_capture_slot, stream, frame_region, target, texture );
}

static void WINAPI wine_glBinormal3bEXT( GLbyte bx, GLbyte by, GLbyte bz ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", bx, by, bz );
  funcs->ext.p_glBinormal3bEXT( bx, by, bz );
}

static void WINAPI wine_glBinormal3bvEXT( const GLbyte* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glBinormal3bvEXT( v );
}

static void WINAPI wine_glBinormal3dEXT( GLdouble bx, GLdouble by, GLdouble bz ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f)\n", bx, by, bz );
  funcs->ext.p_glBinormal3dEXT( bx, by, bz );
}

static void WINAPI wine_glBinormal3dvEXT( const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glBinormal3dvEXT( v );
}

static void WINAPI wine_glBinormal3fEXT( GLfloat bx, GLfloat by, GLfloat bz ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f)\n", bx, by, bz );
  funcs->ext.p_glBinormal3fEXT( bx, by, bz );
}

static void WINAPI wine_glBinormal3fvEXT( const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glBinormal3fvEXT( v );
}

static void WINAPI wine_glBinormal3iEXT( GLint bx, GLint by, GLint bz ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", bx, by, bz );
  funcs->ext.p_glBinormal3iEXT( bx, by, bz );
}

static void WINAPI wine_glBinormal3ivEXT( const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glBinormal3ivEXT( v );
}

static void WINAPI wine_glBinormal3sEXT( GLshort bx, GLshort by, GLshort bz ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", bx, by, bz );
  funcs->ext.p_glBinormal3sEXT( bx, by, bz );
}

static void WINAPI wine_glBinormal3svEXT( const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glBinormal3svEXT( v );
}

static void WINAPI wine_glBinormalPointerEXT( GLenum type, GLsizei stride, const GLvoid* pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", type, stride, pointer );
  funcs->ext.p_glBinormalPointerEXT( type, stride, pointer );
}

static void WINAPI wine_glBlendColor( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f, %f)\n", red, green, blue, alpha );
  funcs->ext.p_glBlendColor( red, green, blue, alpha );
}

static void WINAPI wine_glBlendColorEXT( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f, %f)\n", red, green, blue, alpha );
  funcs->ext.p_glBlendColorEXT( red, green, blue, alpha );
}

static void WINAPI wine_glBlendEquation( GLenum mode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", mode );
  funcs->ext.p_glBlendEquation( mode );
}

static void WINAPI wine_glBlendEquationEXT( GLenum mode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", mode );
  funcs->ext.p_glBlendEquationEXT( mode );
}

static void WINAPI wine_glBlendEquationIndexedAMD( GLuint buf, GLenum mode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", buf, mode );
  funcs->ext.p_glBlendEquationIndexedAMD( buf, mode );
}

static void WINAPI wine_glBlendEquationSeparate( GLenum modeRGB, GLenum modeAlpha ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", modeRGB, modeAlpha );
  funcs->ext.p_glBlendEquationSeparate( modeRGB, modeAlpha );
}

static void WINAPI wine_glBlendEquationSeparateEXT( GLenum modeRGB, GLenum modeAlpha ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", modeRGB, modeAlpha );
  funcs->ext.p_glBlendEquationSeparateEXT( modeRGB, modeAlpha );
}

static void WINAPI wine_glBlendEquationSeparateIndexedAMD( GLuint buf, GLenum modeRGB, GLenum modeAlpha ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", buf, modeRGB, modeAlpha );
  funcs->ext.p_glBlendEquationSeparateIndexedAMD( buf, modeRGB, modeAlpha );
}

static void WINAPI wine_glBlendEquationSeparatei( GLuint buf, GLenum modeRGB, GLenum modeAlpha ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", buf, modeRGB, modeAlpha );
  funcs->ext.p_glBlendEquationSeparatei( buf, modeRGB, modeAlpha );
}

static void WINAPI wine_glBlendEquationSeparateiARB( GLuint buf, GLenum modeRGB, GLenum modeAlpha ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", buf, modeRGB, modeAlpha );
  funcs->ext.p_glBlendEquationSeparateiARB( buf, modeRGB, modeAlpha );
}

static void WINAPI wine_glBlendEquationi( GLuint buf, GLenum mode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", buf, mode );
  funcs->ext.p_glBlendEquationi( buf, mode );
}

static void WINAPI wine_glBlendEquationiARB( GLuint buf, GLenum mode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", buf, mode );
  funcs->ext.p_glBlendEquationiARB( buf, mode );
}

static void WINAPI wine_glBlendFuncIndexedAMD( GLuint buf, GLenum src, GLenum dst ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", buf, src, dst );
  funcs->ext.p_glBlendFuncIndexedAMD( buf, src, dst );
}

static void WINAPI wine_glBlendFuncSeparate( GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha );
  funcs->ext.p_glBlendFuncSeparate( sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha );
}

static void WINAPI wine_glBlendFuncSeparateEXT( GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha );
  funcs->ext.p_glBlendFuncSeparateEXT( sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha );
}

static void WINAPI wine_glBlendFuncSeparateINGR( GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha );
  funcs->ext.p_glBlendFuncSeparateINGR( sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha );
}

static void WINAPI wine_glBlendFuncSeparateIndexedAMD( GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", buf, srcRGB, dstRGB, srcAlpha, dstAlpha );
  funcs->ext.p_glBlendFuncSeparateIndexedAMD( buf, srcRGB, dstRGB, srcAlpha, dstAlpha );
}

static void WINAPI wine_glBlendFuncSeparatei( GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", buf, srcRGB, dstRGB, srcAlpha, dstAlpha );
  funcs->ext.p_glBlendFuncSeparatei( buf, srcRGB, dstRGB, srcAlpha, dstAlpha );
}

static void WINAPI wine_glBlendFuncSeparateiARB( GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", buf, srcRGB, dstRGB, srcAlpha, dstAlpha );
  funcs->ext.p_glBlendFuncSeparateiARB( buf, srcRGB, dstRGB, srcAlpha, dstAlpha );
}

static void WINAPI wine_glBlendFunci( GLuint buf, GLenum src, GLenum dst ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", buf, src, dst );
  funcs->ext.p_glBlendFunci( buf, src, dst );
}

static void WINAPI wine_glBlendFunciARB( GLuint buf, GLenum src, GLenum dst ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", buf, src, dst );
  funcs->ext.p_glBlendFunciARB( buf, src, dst );
}

static void WINAPI wine_glBlitFramebuffer( GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter );
  funcs->ext.p_glBlitFramebuffer( srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter );
}

static void WINAPI wine_glBlitFramebufferEXT( GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter );
  funcs->ext.p_glBlitFramebufferEXT( srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter );
}

static void WINAPI wine_glBufferAddressRangeNV( GLenum pname, GLuint index, UINT64 address, INT_PTR length ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %s, %ld)\n", pname, index, wine_dbgstr_longlong(address), length );
  funcs->ext.p_glBufferAddressRangeNV( pname, index, address, length );
}

static void WINAPI wine_glBufferData( GLenum target, INT_PTR size, const GLvoid* data, GLenum usage ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %ld, %p, %d)\n", target, size, data, usage );
  funcs->ext.p_glBufferData( target, size, data, usage );
}

static void WINAPI wine_glBufferDataARB( GLenum target, INT_PTR size, const GLvoid* data, GLenum usage ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %ld, %p, %d)\n", target, size, data, usage );
  funcs->ext.p_glBufferDataARB( target, size, data, usage );
}

static void WINAPI wine_glBufferParameteriAPPLE( GLenum target, GLenum pname, GLint param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", target, pname, param );
  funcs->ext.p_glBufferParameteriAPPLE( target, pname, param );
}

static GLuint WINAPI wine_glBufferRegionEnabled( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  return funcs->ext.p_glBufferRegionEnabled( );
}

static void WINAPI wine_glBufferSubData( GLenum target, INT_PTR offset, INT_PTR size, const GLvoid* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %ld, %ld, %p)\n", target, offset, size, data );
  funcs->ext.p_glBufferSubData( target, offset, size, data );
}

static void WINAPI wine_glBufferSubDataARB( GLenum target, INT_PTR offset, INT_PTR size, const GLvoid* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %ld, %ld, %p)\n", target, offset, size, data );
  funcs->ext.p_glBufferSubDataARB( target, offset, size, data );
}

static GLenum WINAPI wine_glCheckFramebufferStatus( GLenum target ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", target );
  return funcs->ext.p_glCheckFramebufferStatus( target );
}

static GLenum WINAPI wine_glCheckFramebufferStatusEXT( GLenum target ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", target );
  return funcs->ext.p_glCheckFramebufferStatusEXT( target );
}

static GLenum WINAPI wine_glCheckNamedFramebufferStatusEXT( GLuint framebuffer, GLenum target ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", framebuffer, target );
  return funcs->ext.p_glCheckNamedFramebufferStatusEXT( framebuffer, target );
}

static void WINAPI wine_glClampColor( GLenum target, GLenum clamp ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", target, clamp );
  funcs->ext.p_glClampColor( target, clamp );
}

static void WINAPI wine_glClampColorARB( GLenum target, GLenum clamp ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", target, clamp );
  funcs->ext.p_glClampColorARB( target, clamp );
}

static void WINAPI wine_glClearBufferfi( GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f, %d)\n", buffer, drawbuffer, depth, stencil );
  funcs->ext.p_glClearBufferfi( buffer, drawbuffer, depth, stencil );
}

static void WINAPI wine_glClearBufferfv( GLenum buffer, GLint drawbuffer, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", buffer, drawbuffer, value );
  funcs->ext.p_glClearBufferfv( buffer, drawbuffer, value );
}

static void WINAPI wine_glClearBufferiv( GLenum buffer, GLint drawbuffer, const GLint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", buffer, drawbuffer, value );
  funcs->ext.p_glClearBufferiv( buffer, drawbuffer, value );
}

static void WINAPI wine_glClearBufferuiv( GLenum buffer, GLint drawbuffer, const GLuint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", buffer, drawbuffer, value );
  funcs->ext.p_glClearBufferuiv( buffer, drawbuffer, value );
}

static void WINAPI wine_glClearColorIiEXT( GLint red, GLint green, GLint blue, GLint alpha ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", red, green, blue, alpha );
  funcs->ext.p_glClearColorIiEXT( red, green, blue, alpha );
}

static void WINAPI wine_glClearColorIuiEXT( GLuint red, GLuint green, GLuint blue, GLuint alpha ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", red, green, blue, alpha );
  funcs->ext.p_glClearColorIuiEXT( red, green, blue, alpha );
}

static void WINAPI wine_glClearDepthdNV( GLdouble depth ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f)\n", depth );
  funcs->ext.p_glClearDepthdNV( depth );
}

static void WINAPI wine_glClearDepthf( GLfloat d ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f)\n", d );
  funcs->ext.p_glClearDepthf( d );
}

static void WINAPI wine_glClientActiveTexture( GLenum texture ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", texture );
  funcs->ext.p_glClientActiveTexture( texture );
}

static void WINAPI wine_glClientActiveTextureARB( GLenum texture ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", texture );
  funcs->ext.p_glClientActiveTextureARB( texture );
}

static void WINAPI wine_glClientActiveVertexStreamATI( GLenum stream ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", stream );
  funcs->ext.p_glClientActiveVertexStreamATI( stream );
}

static void WINAPI wine_glClientAttribDefaultEXT( GLbitfield mask ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", mask );
  funcs->ext.p_glClientAttribDefaultEXT( mask );
}

static GLenum WINAPI wine_glClientWaitSync( GLvoid* sync, GLbitfield flags, UINT64 timeout ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p, %d, %s)\n", sync, flags, wine_dbgstr_longlong(timeout) );
  return funcs->ext.p_glClientWaitSync( sync, flags, timeout );
}

static void WINAPI wine_glColor3fVertex3fSUN( GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f, %f, %f, %f)\n", r, g, b, x, y, z );
  funcs->ext.p_glColor3fVertex3fSUN( r, g, b, x, y, z );
}

static void WINAPI wine_glColor3fVertex3fvSUN( const GLfloat* c, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p, %p)\n", c, v );
  funcs->ext.p_glColor3fVertex3fvSUN( c, v );
}

static void WINAPI wine_glColor3hNV( unsigned short red, unsigned short green, unsigned short blue ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", red, green, blue );
  funcs->ext.p_glColor3hNV( red, green, blue );
}

static void WINAPI wine_glColor3hvNV( const unsigned short* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glColor3hvNV( v );
}

static void WINAPI wine_glColor4fNormal3fVertex3fSUN( GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f, %f, %f, %f, %f, %f, %f, %f)\n", r, g, b, a, nx, ny, nz, x, y, z );
  funcs->ext.p_glColor4fNormal3fVertex3fSUN( r, g, b, a, nx, ny, nz, x, y, z );
}

static void WINAPI wine_glColor4fNormal3fVertex3fvSUN( const GLfloat* c, const GLfloat* n, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p, %p, %p)\n", c, n, v );
  funcs->ext.p_glColor4fNormal3fVertex3fvSUN( c, n, v );
}

static void WINAPI wine_glColor4hNV( unsigned short red, unsigned short green, unsigned short blue, unsigned short alpha ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", red, green, blue, alpha );
  funcs->ext.p_glColor4hNV( red, green, blue, alpha );
}

static void WINAPI wine_glColor4hvNV( const unsigned short* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glColor4hvNV( v );
}

static void WINAPI wine_glColor4ubVertex2fSUN( GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %f, %f)\n", r, g, b, a, x, y );
  funcs->ext.p_glColor4ubVertex2fSUN( r, g, b, a, x, y );
}

static void WINAPI wine_glColor4ubVertex2fvSUN( const GLubyte* c, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p, %p)\n", c, v );
  funcs->ext.p_glColor4ubVertex2fvSUN( c, v );
}

static void WINAPI wine_glColor4ubVertex3fSUN( GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %f, %f, %f)\n", r, g, b, a, x, y, z );
  funcs->ext.p_glColor4ubVertex3fSUN( r, g, b, a, x, y, z );
}

static void WINAPI wine_glColor4ubVertex3fvSUN( const GLubyte* c, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p, %p)\n", c, v );
  funcs->ext.p_glColor4ubVertex3fvSUN( c, v );
}

static void WINAPI wine_glColorFormatNV( GLint size, GLenum type, GLsizei stride ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", size, type, stride );
  funcs->ext.p_glColorFormatNV( size, type, stride );
}

static void WINAPI wine_glColorFragmentOp1ATI( GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d)\n", op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod );
  funcs->ext.p_glColorFragmentOp1ATI( op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod );
}

static void WINAPI wine_glColorFragmentOp2ATI( GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod );
  funcs->ext.p_glColorFragmentOp2ATI( op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod );
}

static void WINAPI wine_glColorFragmentOp3ATI( GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod );
  funcs->ext.p_glColorFragmentOp3ATI( op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod );
}

static void WINAPI wine_glColorMaskIndexedEXT( GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", index, r, g, b, a );
  funcs->ext.p_glColorMaskIndexedEXT( index, r, g, b, a );
}

static void WINAPI wine_glColorMaski( GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", index, r, g, b, a );
  funcs->ext.p_glColorMaski( index, r, g, b, a );
}

static void WINAPI wine_glColorP3ui( GLenum type, GLuint color ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", type, color );
  funcs->ext.p_glColorP3ui( type, color );
}

static void WINAPI wine_glColorP3uiv( GLenum type, const GLuint* color ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", type, color );
  funcs->ext.p_glColorP3uiv( type, color );
}

static void WINAPI wine_glColorP4ui( GLenum type, GLuint color ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", type, color );
  funcs->ext.p_glColorP4ui( type, color );
}

static void WINAPI wine_glColorP4uiv( GLenum type, const GLuint* color ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", type, color );
  funcs->ext.p_glColorP4uiv( type, color );
}

static void WINAPI wine_glColorPointerEXT( GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid* pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", size, type, stride, count, pointer );
  funcs->ext.p_glColorPointerEXT( size, type, stride, count, pointer );
}

static void WINAPI wine_glColorPointerListIBM( GLint size, GLenum type, GLint stride, const GLvoid** pointer, GLint ptrstride ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p, %d)\n", size, type, stride, pointer, ptrstride );
  funcs->ext.p_glColorPointerListIBM( size, type, stride, pointer, ptrstride );
}

static void WINAPI wine_glColorPointervINTEL( GLint size, GLenum type, const GLvoid** pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", size, type, pointer );
  funcs->ext.p_glColorPointervINTEL( size, type, pointer );
}

static void WINAPI wine_glColorSubTable( GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %p)\n", target, start, count, format, type, data );
  funcs->ext.p_glColorSubTable( target, start, count, format, type, data );
}

static void WINAPI wine_glColorSubTableEXT( GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %p)\n", target, start, count, format, type, data );
  funcs->ext.p_glColorSubTableEXT( target, start, count, format, type, data );
}

static void WINAPI wine_glColorTable( GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid* table ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %p)\n", target, internalformat, width, format, type, table );
  funcs->ext.p_glColorTable( target, internalformat, width, format, type, table );
}

static void WINAPI wine_glColorTableEXT( GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type, const GLvoid* table ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %p)\n", target, internalFormat, width, format, type, table );
  funcs->ext.p_glColorTableEXT( target, internalFormat, width, format, type, table );
}

static void WINAPI wine_glColorTableParameterfv( GLenum target, GLenum pname, const GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glColorTableParameterfv( target, pname, params );
}

static void WINAPI wine_glColorTableParameterfvSGI( GLenum target, GLenum pname, const GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glColorTableParameterfvSGI( target, pname, params );
}

static void WINAPI wine_glColorTableParameteriv( GLenum target, GLenum pname, const GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glColorTableParameteriv( target, pname, params );
}

static void WINAPI wine_glColorTableParameterivSGI( GLenum target, GLenum pname, const GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glColorTableParameterivSGI( target, pname, params );
}

static void WINAPI wine_glColorTableSGI( GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid* table ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %p)\n", target, internalformat, width, format, type, table );
  funcs->ext.p_glColorTableSGI( target, internalformat, width, format, type, table );
}

static void WINAPI wine_glCombinerInputNV( GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d)\n", stage, portion, variable, input, mapping, componentUsage );
  funcs->ext.p_glCombinerInputNV( stage, portion, variable, input, mapping, componentUsage );
}

static void WINAPI wine_glCombinerOutputNV( GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct, GLboolean cdDotProduct, GLboolean muxSum ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", stage, portion, abOutput, cdOutput, sumOutput, scale, bias, abDotProduct, cdDotProduct, muxSum );
  funcs->ext.p_glCombinerOutputNV( stage, portion, abOutput, cdOutput, sumOutput, scale, bias, abDotProduct, cdDotProduct, muxSum );
}

static void WINAPI wine_glCombinerParameterfNV( GLenum pname, GLfloat param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f)\n", pname, param );
  funcs->ext.p_glCombinerParameterfNV( pname, param );
}

static void WINAPI wine_glCombinerParameterfvNV( GLenum pname, const GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", pname, params );
  funcs->ext.p_glCombinerParameterfvNV( pname, params );
}

static void WINAPI wine_glCombinerParameteriNV( GLenum pname, GLint param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", pname, param );
  funcs->ext.p_glCombinerParameteriNV( pname, param );
}

static void WINAPI wine_glCombinerParameterivNV( GLenum pname, const GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", pname, params );
  funcs->ext.p_glCombinerParameterivNV( pname, params );
}

static void WINAPI wine_glCombinerStageParameterfvNV( GLenum stage, GLenum pname, const GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", stage, pname, params );
  funcs->ext.p_glCombinerStageParameterfvNV( stage, pname, params );
}

static void WINAPI wine_glCompileShader( GLuint shader ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", shader );
  funcs->ext.p_glCompileShader( shader );
}

static void WINAPI wine_glCompileShaderARB( unsigned int shaderObj ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", shaderObj );
  funcs->ext.p_glCompileShaderARB( shaderObj );
}

static void WINAPI wine_glCompileShaderIncludeARB( GLuint shader, GLsizei count, const char** path, const GLint* length ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %p)\n", shader, count, path, length );
  funcs->ext.p_glCompileShaderIncludeARB( shader, count, path, length );
}

static void WINAPI wine_glCompressedMultiTexImage1DEXT( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid* bits ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %p)\n", texunit, target, level, internalformat, width, border, imageSize, bits );
  funcs->ext.p_glCompressedMultiTexImage1DEXT( texunit, target, level, internalformat, width, border, imageSize, bits );
}

static void WINAPI wine_glCompressedMultiTexImage2DEXT( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* bits ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texunit, target, level, internalformat, width, height, border, imageSize, bits );
  funcs->ext.p_glCompressedMultiTexImage2DEXT( texunit, target, level, internalformat, width, height, border, imageSize, bits );
}

static void WINAPI wine_glCompressedMultiTexImage3DEXT( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid* bits ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texunit, target, level, internalformat, width, height, depth, border, imageSize, bits );
  funcs->ext.p_glCompressedMultiTexImage3DEXT( texunit, target, level, internalformat, width, height, depth, border, imageSize, bits );
}

static void WINAPI wine_glCompressedMultiTexSubImage1DEXT( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid* bits ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %p)\n", texunit, target, level, xoffset, width, format, imageSize, bits );
  funcs->ext.p_glCompressedMultiTexSubImage1DEXT( texunit, target, level, xoffset, width, format, imageSize, bits );
}

static void WINAPI wine_glCompressedMultiTexSubImage2DEXT( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid* bits ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texunit, target, level, xoffset, yoffset, width, height, format, imageSize, bits );
  funcs->ext.p_glCompressedMultiTexSubImage2DEXT( texunit, target, level, xoffset, yoffset, width, height, format, imageSize, bits );
}

static void WINAPI wine_glCompressedMultiTexSubImage3DEXT( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid* bits ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texunit, target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, bits );
  funcs->ext.p_glCompressedMultiTexSubImage3DEXT( texunit, target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, bits );
}

static void WINAPI wine_glCompressedTexImage1D( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, border, imageSize, data );
  funcs->ext.p_glCompressedTexImage1D( target, level, internalformat, width, border, imageSize, data );
}

static void WINAPI wine_glCompressedTexImage1DARB( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, border, imageSize, data );
  funcs->ext.p_glCompressedTexImage1DARB( target, level, internalformat, width, border, imageSize, data );
}

static void WINAPI wine_glCompressedTexImage2D( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, height, border, imageSize, data );
  funcs->ext.p_glCompressedTexImage2D( target, level, internalformat, width, height, border, imageSize, data );
}

static void WINAPI wine_glCompressedTexImage2DARB( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, height, border, imageSize, data );
  funcs->ext.p_glCompressedTexImage2DARB( target, level, internalformat, width, height, border, imageSize, data );
}

static void WINAPI wine_glCompressedTexImage3D( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, height, depth, border, imageSize, data );
  funcs->ext.p_glCompressedTexImage3D( target, level, internalformat, width, height, depth, border, imageSize, data );
}

static void WINAPI wine_glCompressedTexImage3DARB( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, height, depth, border, imageSize, data );
  funcs->ext.p_glCompressedTexImage3DARB( target, level, internalformat, width, height, depth, border, imageSize, data );
}

static void WINAPI wine_glCompressedTexSubImage1D( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, width, format, imageSize, data );
  funcs->ext.p_glCompressedTexSubImage1D( target, level, xoffset, width, format, imageSize, data );
}

static void WINAPI wine_glCompressedTexSubImage1DARB( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, width, format, imageSize, data );
  funcs->ext.p_glCompressedTexSubImage1DARB( target, level, xoffset, width, format, imageSize, data );
}

static void WINAPI wine_glCompressedTexSubImage2D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, width, height, format, imageSize, data );
  funcs->ext.p_glCompressedTexSubImage2D( target, level, xoffset, yoffset, width, height, format, imageSize, data );
}

static void WINAPI wine_glCompressedTexSubImage2DARB( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, width, height, format, imageSize, data );
  funcs->ext.p_glCompressedTexSubImage2DARB( target, level, xoffset, yoffset, width, height, format, imageSize, data );
}

static void WINAPI wine_glCompressedTexSubImage3D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data );
  funcs->ext.p_glCompressedTexSubImage3D( target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data );
}

static void WINAPI wine_glCompressedTexSubImage3DARB( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data );
  funcs->ext.p_glCompressedTexSubImage3DARB( target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data );
}

static void WINAPI wine_glCompressedTextureImage1DEXT( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid* bits ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %p)\n", texture, target, level, internalformat, width, border, imageSize, bits );
  funcs->ext.p_glCompressedTextureImage1DEXT( texture, target, level, internalformat, width, border, imageSize, bits );
}

static void WINAPI wine_glCompressedTextureImage2DEXT( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* bits ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texture, target, level, internalformat, width, height, border, imageSize, bits );
  funcs->ext.p_glCompressedTextureImage2DEXT( texture, target, level, internalformat, width, height, border, imageSize, bits );
}

static void WINAPI wine_glCompressedTextureImage3DEXT( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid* bits ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texture, target, level, internalformat, width, height, depth, border, imageSize, bits );
  funcs->ext.p_glCompressedTextureImage3DEXT( texture, target, level, internalformat, width, height, depth, border, imageSize, bits );
}

static void WINAPI wine_glCompressedTextureSubImage1DEXT( GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid* bits ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %p)\n", texture, target, level, xoffset, width, format, imageSize, bits );
  funcs->ext.p_glCompressedTextureSubImage1DEXT( texture, target, level, xoffset, width, format, imageSize, bits );
}

static void WINAPI wine_glCompressedTextureSubImage2DEXT( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid* bits ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texture, target, level, xoffset, yoffset, width, height, format, imageSize, bits );
  funcs->ext.p_glCompressedTextureSubImage2DEXT( texture, target, level, xoffset, yoffset, width, height, format, imageSize, bits );
}

static void WINAPI wine_glCompressedTextureSubImage3DEXT( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid* bits ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texture, target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, bits );
  funcs->ext.p_glCompressedTextureSubImage3DEXT( texture, target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, bits );
}

static void WINAPI wine_glConvolutionFilter1D( GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid* image ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %p)\n", target, internalformat, width, format, type, image );
  funcs->ext.p_glConvolutionFilter1D( target, internalformat, width, format, type, image );
}

static void WINAPI wine_glConvolutionFilter1DEXT( GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid* image ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %p)\n", target, internalformat, width, format, type, image );
  funcs->ext.p_glConvolutionFilter1DEXT( target, internalformat, width, format, type, image );
}

static void WINAPI wine_glConvolutionFilter2D( GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* image ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %p)\n", target, internalformat, width, height, format, type, image );
  funcs->ext.p_glConvolutionFilter2D( target, internalformat, width, height, format, type, image );
}

static void WINAPI wine_glConvolutionFilter2DEXT( GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* image ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %p)\n", target, internalformat, width, height, format, type, image );
  funcs->ext.p_glConvolutionFilter2DEXT( target, internalformat, width, height, format, type, image );
}

static void WINAPI wine_glConvolutionParameterf( GLenum target, GLenum pname, GLfloat params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f)\n", target, pname, params );
  funcs->ext.p_glConvolutionParameterf( target, pname, params );
}

static void WINAPI wine_glConvolutionParameterfEXT( GLenum target, GLenum pname, GLfloat params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f)\n", target, pname, params );
  funcs->ext.p_glConvolutionParameterfEXT( target, pname, params );
}

static void WINAPI wine_glConvolutionParameterfv( GLenum target, GLenum pname, const GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glConvolutionParameterfv( target, pname, params );
}

static void WINAPI wine_glConvolutionParameterfvEXT( GLenum target, GLenum pname, const GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glConvolutionParameterfvEXT( target, pname, params );
}

static void WINAPI wine_glConvolutionParameteri( GLenum target, GLenum pname, GLint params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", target, pname, params );
  funcs->ext.p_glConvolutionParameteri( target, pname, params );
}

static void WINAPI wine_glConvolutionParameteriEXT( GLenum target, GLenum pname, GLint params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", target, pname, params );
  funcs->ext.p_glConvolutionParameteriEXT( target, pname, params );
}

static void WINAPI wine_glConvolutionParameteriv( GLenum target, GLenum pname, const GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glConvolutionParameteriv( target, pname, params );
}

static void WINAPI wine_glConvolutionParameterivEXT( GLenum target, GLenum pname, const GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glConvolutionParameterivEXT( target, pname, params );
}

static void WINAPI wine_glCopyBufferSubData( GLenum readTarget, GLenum writeTarget, INT_PTR readOffset, INT_PTR writeOffset, INT_PTR size ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %ld, %ld, %ld)\n", readTarget, writeTarget, readOffset, writeOffset, size );
  funcs->ext.p_glCopyBufferSubData( readTarget, writeTarget, readOffset, writeOffset, size );
}

static void WINAPI wine_glCopyColorSubTable( GLenum target, GLsizei start, GLint x, GLint y, GLsizei width ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", target, start, x, y, width );
  funcs->ext.p_glCopyColorSubTable( target, start, x, y, width );
}

static void WINAPI wine_glCopyColorSubTableEXT( GLenum target, GLsizei start, GLint x, GLint y, GLsizei width ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", target, start, x, y, width );
  funcs->ext.p_glCopyColorSubTableEXT( target, start, x, y, width );
}

static void WINAPI wine_glCopyColorTable( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", target, internalformat, x, y, width );
  funcs->ext.p_glCopyColorTable( target, internalformat, x, y, width );
}

static void WINAPI wine_glCopyColorTableSGI( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", target, internalformat, x, y, width );
  funcs->ext.p_glCopyColorTableSGI( target, internalformat, x, y, width );
}

static void WINAPI wine_glCopyConvolutionFilter1D( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", target, internalformat, x, y, width );
  funcs->ext.p_glCopyConvolutionFilter1D( target, internalformat, x, y, width );
}

static void WINAPI wine_glCopyConvolutionFilter1DEXT( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", target, internalformat, x, y, width );
  funcs->ext.p_glCopyConvolutionFilter1DEXT( target, internalformat, x, y, width );
}

static void WINAPI wine_glCopyConvolutionFilter2D( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d)\n", target, internalformat, x, y, width, height );
  funcs->ext.p_glCopyConvolutionFilter2D( target, internalformat, x, y, width, height );
}

static void WINAPI wine_glCopyConvolutionFilter2DEXT( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d)\n", target, internalformat, x, y, width, height );
  funcs->ext.p_glCopyConvolutionFilter2DEXT( target, internalformat, x, y, width, height );
}

static void WINAPI wine_glCopyImageSubDataNV( GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei width, GLsizei height, GLsizei depth ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", srcName, srcTarget, srcLevel, srcX, srcY, srcZ, dstName, dstTarget, dstLevel, dstX, dstY, dstZ, width, height, depth );
  funcs->ext.p_glCopyImageSubDataNV( srcName, srcTarget, srcLevel, srcX, srcY, srcZ, dstName, dstTarget, dstLevel, dstX, dstY, dstZ, width, height, depth );
}

static void WINAPI wine_glCopyMultiTexImage1DEXT( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d)\n", texunit, target, level, internalformat, x, y, width, border );
  funcs->ext.p_glCopyMultiTexImage1DEXT( texunit, target, level, internalformat, x, y, width, border );
}

static void WINAPI wine_glCopyMultiTexImage2DEXT( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d)\n", texunit, target, level, internalformat, x, y, width, height, border );
  funcs->ext.p_glCopyMultiTexImage2DEXT( texunit, target, level, internalformat, x, y, width, height, border );
}

static void WINAPI wine_glCopyMultiTexSubImage1DEXT( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d)\n", texunit, target, level, xoffset, x, y, width );
  funcs->ext.p_glCopyMultiTexSubImage1DEXT( texunit, target, level, xoffset, x, y, width );
}

static void WINAPI wine_glCopyMultiTexSubImage2DEXT( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d)\n", texunit, target, level, xoffset, yoffset, x, y, width, height );
  funcs->ext.p_glCopyMultiTexSubImage2DEXT( texunit, target, level, xoffset, yoffset, x, y, width, height );
}

static void WINAPI wine_glCopyMultiTexSubImage3DEXT( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", texunit, target, level, xoffset, yoffset, zoffset, x, y, width, height );
  funcs->ext.p_glCopyMultiTexSubImage3DEXT( texunit, target, level, xoffset, yoffset, zoffset, x, y, width, height );
}

static void WINAPI wine_glCopyPathNV( GLuint resultPath, GLuint srcPath ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", resultPath, srcPath );
  funcs->ext.p_glCopyPathNV( resultPath, srcPath );
}

static void WINAPI wine_glCopyTexImage1DEXT( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d)\n", target, level, internalformat, x, y, width, border );
  funcs->ext.p_glCopyTexImage1DEXT( target, level, internalformat, x, y, width, border );
}

static void WINAPI wine_glCopyTexImage2DEXT( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d)\n", target, level, internalformat, x, y, width, height, border );
  funcs->ext.p_glCopyTexImage2DEXT( target, level, internalformat, x, y, width, height, border );
}

static void WINAPI wine_glCopyTexSubImage1DEXT( GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d)\n", target, level, xoffset, x, y, width );
  funcs->ext.p_glCopyTexSubImage1DEXT( target, level, xoffset, x, y, width );
}

static void WINAPI wine_glCopyTexSubImage2DEXT( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d)\n", target, level, xoffset, yoffset, x, y, width, height );
  funcs->ext.p_glCopyTexSubImage2DEXT( target, level, xoffset, yoffset, x, y, width, height );
}

static void WINAPI wine_glCopyTexSubImage3D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d)\n", target, level, xoffset, yoffset, zoffset, x, y, width, height );
  funcs->ext.p_glCopyTexSubImage3D( target, level, xoffset, yoffset, zoffset, x, y, width, height );
}

static void WINAPI wine_glCopyTexSubImage3DEXT( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d)\n", target, level, xoffset, yoffset, zoffset, x, y, width, height );
  funcs->ext.p_glCopyTexSubImage3DEXT( target, level, xoffset, yoffset, zoffset, x, y, width, height );
}

static void WINAPI wine_glCopyTextureImage1DEXT( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d)\n", texture, target, level, internalformat, x, y, width, border );
  funcs->ext.p_glCopyTextureImage1DEXT( texture, target, level, internalformat, x, y, width, border );
}

static void WINAPI wine_glCopyTextureImage2DEXT( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d)\n", texture, target, level, internalformat, x, y, width, height, border );
  funcs->ext.p_glCopyTextureImage2DEXT( texture, target, level, internalformat, x, y, width, height, border );
}

static void WINAPI wine_glCopyTextureSubImage1DEXT( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d)\n", texture, target, level, xoffset, x, y, width );
  funcs->ext.p_glCopyTextureSubImage1DEXT( texture, target, level, xoffset, x, y, width );
}

static void WINAPI wine_glCopyTextureSubImage2DEXT( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d)\n", texture, target, level, xoffset, yoffset, x, y, width, height );
  funcs->ext.p_glCopyTextureSubImage2DEXT( texture, target, level, xoffset, yoffset, x, y, width, height );
}

static void WINAPI wine_glCopyTextureSubImage3DEXT( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", texture, target, level, xoffset, yoffset, zoffset, x, y, width, height );
  funcs->ext.p_glCopyTextureSubImage3DEXT( texture, target, level, xoffset, yoffset, zoffset, x, y, width, height );
}

static void WINAPI wine_glCoverFillPathInstancedNV( GLsizei numPaths, GLenum pathNameType, const GLvoid* paths, GLuint pathBase, GLenum coverMode, GLenum transformType, const GLfloat* transformValues ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %d, %d, %d, %p)\n", numPaths, pathNameType, paths, pathBase, coverMode, transformType, transformValues );
  funcs->ext.p_glCoverFillPathInstancedNV( numPaths, pathNameType, paths, pathBase, coverMode, transformType, transformValues );
}

static void WINAPI wine_glCoverFillPathNV( GLuint path, GLenum coverMode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", path, coverMode );
  funcs->ext.p_glCoverFillPathNV( path, coverMode );
}

static void WINAPI wine_glCoverStrokePathInstancedNV( GLsizei numPaths, GLenum pathNameType, const GLvoid* paths, GLuint pathBase, GLenum coverMode, GLenum transformType, const GLfloat* transformValues ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %d, %d, %d, %p)\n", numPaths, pathNameType, paths, pathBase, coverMode, transformType, transformValues );
  funcs->ext.p_glCoverStrokePathInstancedNV( numPaths, pathNameType, paths, pathBase, coverMode, transformType, transformValues );
}

static void WINAPI wine_glCoverStrokePathNV( GLuint path, GLenum coverMode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", path, coverMode );
  funcs->ext.p_glCoverStrokePathNV( path, coverMode );
}

static GLuint WINAPI wine_glCreateProgram( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  return funcs->ext.p_glCreateProgram( );
}

static unsigned int WINAPI wine_glCreateProgramObjectARB( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  return funcs->ext.p_glCreateProgramObjectARB( );
}

static GLuint WINAPI wine_glCreateShader( GLenum type ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", type );
  return funcs->ext.p_glCreateShader( type );
}

static unsigned int WINAPI wine_glCreateShaderObjectARB( GLenum shaderType ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", shaderType );
  return funcs->ext.p_glCreateShaderObjectARB( shaderType );
}

static GLuint WINAPI wine_glCreateShaderProgramEXT( GLenum type, const char* string ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", type, string );
  return funcs->ext.p_glCreateShaderProgramEXT( type, string );
}

static GLuint WINAPI wine_glCreateShaderProgramv( GLenum type, GLsizei count, const char* const* strings ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", type, count, strings );
  return funcs->ext.p_glCreateShaderProgramv( type, count, strings );
}

static GLvoid* WINAPI wine_glCreateSyncFromCLeventARB( void * context, void * event, GLbitfield flags ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p, %p, %d)\n", context, event, flags );
  return funcs->ext.p_glCreateSyncFromCLeventARB( context, event, flags );
}

static void WINAPI wine_glCullParameterdvEXT( GLenum pname, GLdouble* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", pname, params );
  funcs->ext.p_glCullParameterdvEXT( pname, params );
}

static void WINAPI wine_glCullParameterfvEXT( GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", pname, params );
  funcs->ext.p_glCullParameterfvEXT( pname, params );
}

static void WINAPI wine_glCurrentPaletteMatrixARB( GLint index ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", index );
  funcs->ext.p_glCurrentPaletteMatrixARB( index );
}

static void WINAPI wine_glDebugMessageCallbackAMD( void * callback, GLvoid* userParam ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p, %p)\n", callback, userParam );
  funcs->ext.p_glDebugMessageCallbackAMD( callback, userParam );
}

static void WINAPI wine_glDebugMessageCallbackARB( void * callback, const GLvoid* userParam ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p, %p)\n", callback, userParam );
  funcs->ext.p_glDebugMessageCallbackARB( callback, userParam );
}

static void WINAPI wine_glDebugMessageControlARB( GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint* ids, GLboolean enabled ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p, %d)\n", source, type, severity, count, ids, enabled );
  funcs->ext.p_glDebugMessageControlARB( source, type, severity, count, ids, enabled );
}

static void WINAPI wine_glDebugMessageEnableAMD( GLenum category, GLenum severity, GLsizei count, const GLuint* ids, GLboolean enabled ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p, %d)\n", category, severity, count, ids, enabled );
  funcs->ext.p_glDebugMessageEnableAMD( category, severity, count, ids, enabled );
}

static void WINAPI wine_glDebugMessageInsertAMD( GLenum category, GLenum severity, GLuint id, GLsizei length, const char* buf ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", category, severity, id, length, buf );
  funcs->ext.p_glDebugMessageInsertAMD( category, severity, id, length, buf );
}

static void WINAPI wine_glDebugMessageInsertARB( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* buf ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %p)\n", source, type, id, severity, length, buf );
  funcs->ext.p_glDebugMessageInsertARB( source, type, id, severity, length, buf );
}

static void WINAPI wine_glDeformSGIX( GLbitfield mask ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", mask );
  funcs->ext.p_glDeformSGIX( mask );
}

static void WINAPI wine_glDeformationMap3dSGIX( GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, GLdouble w1, GLdouble w2, GLint wstride, GLint worder, const GLdouble* points ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %d, %d, %f, %f, %d, %d, %f, %f, %d, %d, %p)\n", target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points );
  funcs->ext.p_glDeformationMap3dSGIX( target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points );
}

static void WINAPI wine_glDeformationMap3fSGIX( GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, GLfloat w1, GLfloat w2, GLint wstride, GLint worder, const GLfloat* points ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %d, %d, %f, %f, %d, %d, %f, %f, %d, %d, %p)\n", target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points );
  funcs->ext.p_glDeformationMap3fSGIX( target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points );
}

static void WINAPI wine_glDeleteAsyncMarkersSGIX( GLuint marker, GLsizei range ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", marker, range );
  funcs->ext.p_glDeleteAsyncMarkersSGIX( marker, range );
}

static void WINAPI wine_glDeleteBufferRegion( GLenum region ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", region );
  funcs->ext.p_glDeleteBufferRegion( region );
}

static void WINAPI wine_glDeleteBuffers( GLsizei n, const GLuint* buffers ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, buffers );
  funcs->ext.p_glDeleteBuffers( n, buffers );
}

static void WINAPI wine_glDeleteBuffersARB( GLsizei n, const GLuint* buffers ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, buffers );
  funcs->ext.p_glDeleteBuffersARB( n, buffers );
}

static void WINAPI wine_glDeleteFencesAPPLE( GLsizei n, const GLuint* fences ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, fences );
  funcs->ext.p_glDeleteFencesAPPLE( n, fences );
}

static void WINAPI wine_glDeleteFencesNV( GLsizei n, const GLuint* fences ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, fences );
  funcs->ext.p_glDeleteFencesNV( n, fences );
}

static void WINAPI wine_glDeleteFragmentShaderATI( GLuint id ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", id );
  funcs->ext.p_glDeleteFragmentShaderATI( id );
}

static void WINAPI wine_glDeleteFramebuffers( GLsizei n, const GLuint* framebuffers ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, framebuffers );
  funcs->ext.p_glDeleteFramebuffers( n, framebuffers );
}

static void WINAPI wine_glDeleteFramebuffersEXT( GLsizei n, const GLuint* framebuffers ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, framebuffers );
  funcs->ext.p_glDeleteFramebuffersEXT( n, framebuffers );
}

static void WINAPI wine_glDeleteNamedStringARB( GLint namelen, const char* name ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", namelen, name );
  funcs->ext.p_glDeleteNamedStringARB( namelen, name );
}

static void WINAPI wine_glDeleteNamesAMD( GLenum identifier, GLuint num, const GLuint* names ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", identifier, num, names );
  funcs->ext.p_glDeleteNamesAMD( identifier, num, names );
}

static void WINAPI wine_glDeleteObjectARB( unsigned int obj ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", obj );
  funcs->ext.p_glDeleteObjectARB( obj );
}

static void WINAPI wine_glDeleteObjectBufferATI( GLuint buffer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", buffer );
  funcs->ext.p_glDeleteObjectBufferATI( buffer );
}

static void WINAPI wine_glDeleteOcclusionQueriesNV( GLsizei n, const GLuint* ids ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, ids );
  funcs->ext.p_glDeleteOcclusionQueriesNV( n, ids );
}

static void WINAPI wine_glDeletePathsNV( GLuint path, GLsizei range ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", path, range );
  funcs->ext.p_glDeletePathsNV( path, range );
}

static void WINAPI wine_glDeletePerfMonitorsAMD( GLsizei n, GLuint* monitors ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, monitors );
  funcs->ext.p_glDeletePerfMonitorsAMD( n, monitors );
}

static void WINAPI wine_glDeleteProgram( GLuint program ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", program );
  funcs->ext.p_glDeleteProgram( program );
}

static void WINAPI wine_glDeleteProgramPipelines( GLsizei n, const GLuint* pipelines ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, pipelines );
  funcs->ext.p_glDeleteProgramPipelines( n, pipelines );
}

static void WINAPI wine_glDeleteProgramsARB( GLsizei n, const GLuint* programs ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, programs );
  funcs->ext.p_glDeleteProgramsARB( n, programs );
}

static void WINAPI wine_glDeleteProgramsNV( GLsizei n, const GLuint* programs ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, programs );
  funcs->ext.p_glDeleteProgramsNV( n, programs );
}

static void WINAPI wine_glDeleteQueries( GLsizei n, const GLuint* ids ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, ids );
  funcs->ext.p_glDeleteQueries( n, ids );
}

static void WINAPI wine_glDeleteQueriesARB( GLsizei n, const GLuint* ids ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, ids );
  funcs->ext.p_glDeleteQueriesARB( n, ids );
}

static void WINAPI wine_glDeleteRenderbuffers( GLsizei n, const GLuint* renderbuffers ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, renderbuffers );
  funcs->ext.p_glDeleteRenderbuffers( n, renderbuffers );
}

static void WINAPI wine_glDeleteRenderbuffersEXT( GLsizei n, const GLuint* renderbuffers ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, renderbuffers );
  funcs->ext.p_glDeleteRenderbuffersEXT( n, renderbuffers );
}

static void WINAPI wine_glDeleteSamplers( GLsizei count, const GLuint* samplers ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", count, samplers );
  funcs->ext.p_glDeleteSamplers( count, samplers );
}

static void WINAPI wine_glDeleteShader( GLuint shader ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", shader );
  funcs->ext.p_glDeleteShader( shader );
}

static void WINAPI wine_glDeleteSync( GLvoid* sync ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", sync );
  funcs->ext.p_glDeleteSync( sync );
}

static void WINAPI wine_glDeleteTexturesEXT( GLsizei n, const GLuint* textures ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, textures );
  funcs->ext.p_glDeleteTexturesEXT( n, textures );
}

static void WINAPI wine_glDeleteTransformFeedbacks( GLsizei n, const GLuint* ids ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, ids );
  funcs->ext.p_glDeleteTransformFeedbacks( n, ids );
}

static void WINAPI wine_glDeleteTransformFeedbacksNV( GLsizei n, const GLuint* ids ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, ids );
  funcs->ext.p_glDeleteTransformFeedbacksNV( n, ids );
}

static void WINAPI wine_glDeleteVertexArrays( GLsizei n, const GLuint* arrays ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, arrays );
  funcs->ext.p_glDeleteVertexArrays( n, arrays );
}

static void WINAPI wine_glDeleteVertexArraysAPPLE( GLsizei n, const GLuint* arrays ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, arrays );
  funcs->ext.p_glDeleteVertexArraysAPPLE( n, arrays );
}

static void WINAPI wine_glDeleteVertexShaderEXT( GLuint id ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", id );
  funcs->ext.p_glDeleteVertexShaderEXT( id );
}

static void WINAPI wine_glDepthBoundsEXT( GLclampd zmin, GLclampd zmax ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f)\n", zmin, zmax );
  funcs->ext.p_glDepthBoundsEXT( zmin, zmax );
}

static void WINAPI wine_glDepthBoundsdNV( GLdouble zmin, GLdouble zmax ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f)\n", zmin, zmax );
  funcs->ext.p_glDepthBoundsdNV( zmin, zmax );
}

static void WINAPI wine_glDepthRangeArrayv( GLuint first, GLsizei count, const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", first, count, v );
  funcs->ext.p_glDepthRangeArrayv( first, count, v );
}

static void WINAPI wine_glDepthRangeIndexed( GLuint index, GLdouble n, GLdouble f ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f)\n", index, n, f );
  funcs->ext.p_glDepthRangeIndexed( index, n, f );
}

static void WINAPI wine_glDepthRangedNV( GLdouble zNear, GLdouble zFar ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f)\n", zNear, zFar );
  funcs->ext.p_glDepthRangedNV( zNear, zFar );
}

static void WINAPI wine_glDepthRangef( GLfloat n, GLfloat f ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f)\n", n, f );
  funcs->ext.p_glDepthRangef( n, f );
}

static void WINAPI wine_glDetachObjectARB( unsigned int containerObj, unsigned int attachedObj ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", containerObj, attachedObj );
  funcs->ext.p_glDetachObjectARB( containerObj, attachedObj );
}

static void WINAPI wine_glDetachShader( GLuint program, GLuint shader ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", program, shader );
  funcs->ext.p_glDetachShader( program, shader );
}

static void WINAPI wine_glDetailTexFuncSGIS( GLenum target, GLsizei n, const GLfloat* points ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, n, points );
  funcs->ext.p_glDetailTexFuncSGIS( target, n, points );
}

static void WINAPI wine_glDisableClientStateIndexedEXT( GLenum array, GLuint index ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", array, index );
  funcs->ext.p_glDisableClientStateIndexedEXT( array, index );
}

static void WINAPI wine_glDisableIndexedEXT( GLenum target, GLuint index ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", target, index );
  funcs->ext.p_glDisableIndexedEXT( target, index );
}

static void WINAPI wine_glDisableVariantClientStateEXT( GLuint id ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", id );
  funcs->ext.p_glDisableVariantClientStateEXT( id );
}

static void WINAPI wine_glDisableVertexAttribAPPLE( GLuint index, GLenum pname ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", index, pname );
  funcs->ext.p_glDisableVertexAttribAPPLE( index, pname );
}

static void WINAPI wine_glDisableVertexAttribArray( GLuint index ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", index );
  funcs->ext.p_glDisableVertexAttribArray( index );
}

static void WINAPI wine_glDisableVertexAttribArrayARB( GLuint index ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", index );
  funcs->ext.p_glDisableVertexAttribArrayARB( index );
}

static void WINAPI wine_glDisablei( GLenum target, GLuint index ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", target, index );
  funcs->ext.p_glDisablei( target, index );
}

static void WINAPI wine_glDrawArraysEXT( GLenum mode, GLint first, GLsizei count ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", mode, first, count );
  funcs->ext.p_glDrawArraysEXT( mode, first, count );
}

static void WINAPI wine_glDrawArraysIndirect( GLenum mode, const GLvoid* indirect ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", mode, indirect );
  funcs->ext.p_glDrawArraysIndirect( mode, indirect );
}

static void WINAPI wine_glDrawArraysInstanced( GLenum mode, GLint first, GLsizei count, GLsizei primcount ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", mode, first, count, primcount );
  funcs->ext.p_glDrawArraysInstanced( mode, first, count, primcount );
}

static void WINAPI wine_glDrawArraysInstancedARB( GLenum mode, GLint first, GLsizei count, GLsizei primcount ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", mode, first, count, primcount );
  funcs->ext.p_glDrawArraysInstancedARB( mode, first, count, primcount );
}

static void WINAPI wine_glDrawArraysInstancedBaseInstance( GLenum mode, GLint first, GLsizei count, GLsizei primcount, GLuint baseinstance ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", mode, first, count, primcount, baseinstance );
  funcs->ext.p_glDrawArraysInstancedBaseInstance( mode, first, count, primcount, baseinstance );
}

static void WINAPI wine_glDrawArraysInstancedEXT( GLenum mode, GLint start, GLsizei count, GLsizei primcount ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", mode, start, count, primcount );
  funcs->ext.p_glDrawArraysInstancedEXT( mode, start, count, primcount );
}

static void WINAPI wine_glDrawBufferRegion( GLenum region, GLint x, GLint y, GLsizei width, GLsizei height, GLint xDest, GLint yDest ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d)\n", region, x, y, width, height, xDest, yDest );
  funcs->ext.p_glDrawBufferRegion( region, x, y, width, height, xDest, yDest );
}

static void WINAPI wine_glDrawBuffers( GLsizei n, const GLenum* bufs ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, bufs );
  funcs->ext.p_glDrawBuffers( n, bufs );
}

static void WINAPI wine_glDrawBuffersARB( GLsizei n, const GLenum* bufs ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, bufs );
  funcs->ext.p_glDrawBuffersARB( n, bufs );
}

static void WINAPI wine_glDrawBuffersATI( GLsizei n, const GLenum* bufs ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, bufs );
  funcs->ext.p_glDrawBuffersATI( n, bufs );
}

static void WINAPI wine_glDrawElementArrayAPPLE( GLenum mode, GLint first, GLsizei count ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", mode, first, count );
  funcs->ext.p_glDrawElementArrayAPPLE( mode, first, count );
}

static void WINAPI wine_glDrawElementArrayATI( GLenum mode, GLsizei count ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", mode, count );
  funcs->ext.p_glDrawElementArrayATI( mode, count );
}

static void WINAPI wine_glDrawElementsBaseVertex( GLenum mode, GLsizei count, GLenum type, const GLvoid* indices, GLint basevertex ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p, %d)\n", mode, count, type, indices, basevertex );
  funcs->ext.p_glDrawElementsBaseVertex( mode, count, type, indices, basevertex );
}

static void WINAPI wine_glDrawElementsIndirect( GLenum mode, GLenum type, const GLvoid* indirect ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", mode, type, indirect );
  funcs->ext.p_glDrawElementsIndirect( mode, type, indirect );
}

static void WINAPI wine_glDrawElementsInstanced( GLenum mode, GLsizei count, GLenum type, const GLvoid* indices, GLsizei primcount ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p, %d)\n", mode, count, type, indices, primcount );
  funcs->ext.p_glDrawElementsInstanced( mode, count, type, indices, primcount );
}

static void WINAPI wine_glDrawElementsInstancedARB( GLenum mode, GLsizei count, GLenum type, const GLvoid* indices, GLsizei primcount ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p, %d)\n", mode, count, type, indices, primcount );
  funcs->ext.p_glDrawElementsInstancedARB( mode, count, type, indices, primcount );
}

static void WINAPI wine_glDrawElementsInstancedBaseInstance( GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei primcount, GLuint baseinstance ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p, %d, %d)\n", mode, count, type, indices, primcount, baseinstance );
  funcs->ext.p_glDrawElementsInstancedBaseInstance( mode, count, type, indices, primcount, baseinstance );
}

static void WINAPI wine_glDrawElementsInstancedBaseVertex( GLenum mode, GLsizei count, GLenum type, const GLvoid* indices, GLsizei primcount, GLint basevertex ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p, %d, %d)\n", mode, count, type, indices, primcount, basevertex );
  funcs->ext.p_glDrawElementsInstancedBaseVertex( mode, count, type, indices, primcount, basevertex );
}

static void WINAPI wine_glDrawElementsInstancedBaseVertexBaseInstance( GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei primcount, GLint basevertex, GLuint baseinstance ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p, %d, %d, %d)\n", mode, count, type, indices, primcount, basevertex, baseinstance );
  funcs->ext.p_glDrawElementsInstancedBaseVertexBaseInstance( mode, count, type, indices, primcount, basevertex, baseinstance );
}

static void WINAPI wine_glDrawElementsInstancedEXT( GLenum mode, GLsizei count, GLenum type, const GLvoid* indices, GLsizei primcount ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p, %d)\n", mode, count, type, indices, primcount );
  funcs->ext.p_glDrawElementsInstancedEXT( mode, count, type, indices, primcount );
}

static void WINAPI wine_glDrawMeshArraysSUN( GLenum mode, GLint first, GLsizei count, GLsizei width ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", mode, first, count, width );
  funcs->ext.p_glDrawMeshArraysSUN( mode, first, count, width );
}

static void WINAPI wine_glDrawRangeElementArrayAPPLE( GLenum mode, GLuint start, GLuint end, GLint first, GLsizei count ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", mode, start, end, first, count );
  funcs->ext.p_glDrawRangeElementArrayAPPLE( mode, start, end, first, count );
}

static void WINAPI wine_glDrawRangeElementArrayATI( GLenum mode, GLuint start, GLuint end, GLsizei count ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", mode, start, end, count );
  funcs->ext.p_glDrawRangeElementArrayATI( mode, start, end, count );
}

static void WINAPI wine_glDrawRangeElements( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid* indices ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %p)\n", mode, start, end, count, type, indices );
  funcs->ext.p_glDrawRangeElements( mode, start, end, count, type, indices );
}

static void WINAPI wine_glDrawRangeElementsBaseVertex( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid* indices, GLint basevertex ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %p, %d)\n", mode, start, end, count, type, indices, basevertex );
  funcs->ext.p_glDrawRangeElementsBaseVertex( mode, start, end, count, type, indices, basevertex );
}

static void WINAPI wine_glDrawRangeElementsEXT( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid* indices ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %p)\n", mode, start, end, count, type, indices );
  funcs->ext.p_glDrawRangeElementsEXT( mode, start, end, count, type, indices );
}

static void WINAPI wine_glDrawTransformFeedback( GLenum mode, GLuint id ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", mode, id );
  funcs->ext.p_glDrawTransformFeedback( mode, id );
}

static void WINAPI wine_glDrawTransformFeedbackInstanced( GLenum mode, GLuint id, GLsizei primcount ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", mode, id, primcount );
  funcs->ext.p_glDrawTransformFeedbackInstanced( mode, id, primcount );
}

static void WINAPI wine_glDrawTransformFeedbackNV( GLenum mode, GLuint id ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", mode, id );
  funcs->ext.p_glDrawTransformFeedbackNV( mode, id );
}

static void WINAPI wine_glDrawTransformFeedbackStream( GLenum mode, GLuint id, GLuint stream ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", mode, id, stream );
  funcs->ext.p_glDrawTransformFeedbackStream( mode, id, stream );
}

static void WINAPI wine_glDrawTransformFeedbackStreamInstanced( GLenum mode, GLuint id, GLuint stream, GLsizei primcount ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", mode, id, stream, primcount );
  funcs->ext.p_glDrawTransformFeedbackStreamInstanced( mode, id, stream, primcount );
}

static void WINAPI wine_glEdgeFlagFormatNV( GLsizei stride ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", stride );
  funcs->ext.p_glEdgeFlagFormatNV( stride );
}

static void WINAPI wine_glEdgeFlagPointerEXT( GLsizei stride, GLsizei count, const GLboolean* pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", stride, count, pointer );
  funcs->ext.p_glEdgeFlagPointerEXT( stride, count, pointer );
}

static void WINAPI wine_glEdgeFlagPointerListIBM( GLint stride, const GLboolean** pointer, GLint ptrstride ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p, %d)\n", stride, pointer, ptrstride );
  funcs->ext.p_glEdgeFlagPointerListIBM( stride, pointer, ptrstride );
}

static void WINAPI wine_glElementPointerAPPLE( GLenum type, const GLvoid* pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", type, pointer );
  funcs->ext.p_glElementPointerAPPLE( type, pointer );
}

static void WINAPI wine_glElementPointerATI( GLenum type, const GLvoid* pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", type, pointer );
  funcs->ext.p_glElementPointerATI( type, pointer );
}

static void WINAPI wine_glEnableClientStateIndexedEXT( GLenum array, GLuint index ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", array, index );
  funcs->ext.p_glEnableClientStateIndexedEXT( array, index );
}

static void WINAPI wine_glEnableIndexedEXT( GLenum target, GLuint index ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", target, index );
  funcs->ext.p_glEnableIndexedEXT( target, index );
}

static void WINAPI wine_glEnableVariantClientStateEXT( GLuint id ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", id );
  funcs->ext.p_glEnableVariantClientStateEXT( id );
}

static void WINAPI wine_glEnableVertexAttribAPPLE( GLuint index, GLenum pname ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", index, pname );
  funcs->ext.p_glEnableVertexAttribAPPLE( index, pname );
}

static void WINAPI wine_glEnableVertexAttribArray( GLuint index ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", index );
  funcs->ext.p_glEnableVertexAttribArray( index );
}

static void WINAPI wine_glEnableVertexAttribArrayARB( GLuint index ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", index );
  funcs->ext.p_glEnableVertexAttribArrayARB( index );
}

static void WINAPI wine_glEnablei( GLenum target, GLuint index ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", target, index );
  funcs->ext.p_glEnablei( target, index );
}

static void WINAPI wine_glEndConditionalRender( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  funcs->ext.p_glEndConditionalRender( );
}

static void WINAPI wine_glEndConditionalRenderNV( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  funcs->ext.p_glEndConditionalRenderNV( );
}

static void WINAPI wine_glEndFragmentShaderATI( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  funcs->ext.p_glEndFragmentShaderATI( );
}

static void WINAPI wine_glEndOcclusionQueryNV( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  funcs->ext.p_glEndOcclusionQueryNV( );
}

static void WINAPI wine_glEndPerfMonitorAMD( GLuint monitor ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", monitor );
  funcs->ext.p_glEndPerfMonitorAMD( monitor );
}

static void WINAPI wine_glEndQuery( GLenum target ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", target );
  funcs->ext.p_glEndQuery( target );
}

static void WINAPI wine_glEndQueryARB( GLenum target ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", target );
  funcs->ext.p_glEndQueryARB( target );
}

static void WINAPI wine_glEndQueryIndexed( GLenum target, GLuint index ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", target, index );
  funcs->ext.p_glEndQueryIndexed( target, index );
}

static void WINAPI wine_glEndTransformFeedback( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  funcs->ext.p_glEndTransformFeedback( );
}

static void WINAPI wine_glEndTransformFeedbackEXT( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  funcs->ext.p_glEndTransformFeedbackEXT( );
}

static void WINAPI wine_glEndTransformFeedbackNV( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  funcs->ext.p_glEndTransformFeedbackNV( );
}

static void WINAPI wine_glEndVertexShaderEXT( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  funcs->ext.p_glEndVertexShaderEXT( );
}

static void WINAPI wine_glEndVideoCaptureNV( GLuint video_capture_slot ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", video_capture_slot );
  funcs->ext.p_glEndVideoCaptureNV( video_capture_slot );
}

static void WINAPI wine_glEvalMapsNV( GLenum target, GLenum mode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", target, mode );
  funcs->ext.p_glEvalMapsNV( target, mode );
}

static void WINAPI wine_glExecuteProgramNV( GLenum target, GLuint id, const GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, id, params );
  funcs->ext.p_glExecuteProgramNV( target, id, params );
}

static void WINAPI wine_glExtractComponentEXT( GLuint res, GLuint src, GLuint num ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", res, src, num );
  funcs->ext.p_glExtractComponentEXT( res, src, num );
}

static GLvoid* WINAPI wine_glFenceSync( GLenum condition, GLbitfield flags ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", condition, flags );
  return funcs->ext.p_glFenceSync( condition, flags );
}

static void WINAPI wine_glFinalCombinerInputNV( GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", variable, input, mapping, componentUsage );
  funcs->ext.p_glFinalCombinerInputNV( variable, input, mapping, componentUsage );
}

static GLint WINAPI wine_glFinishAsyncSGIX( GLuint* markerp ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", markerp );
  return funcs->ext.p_glFinishAsyncSGIX( markerp );
}

static void WINAPI wine_glFinishFenceAPPLE( GLuint fence ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", fence );
  funcs->ext.p_glFinishFenceAPPLE( fence );
}

static void WINAPI wine_glFinishFenceNV( GLuint fence ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", fence );
  funcs->ext.p_glFinishFenceNV( fence );
}

static void WINAPI wine_glFinishObjectAPPLE( GLenum object, GLint name ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", object, name );
  funcs->ext.p_glFinishObjectAPPLE( object, name );
}

static void WINAPI wine_glFinishTextureSUNX( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  funcs->ext.p_glFinishTextureSUNX( );
}

static void WINAPI wine_glFlushMappedBufferRange( GLenum target, INT_PTR offset, INT_PTR length ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %ld, %ld)\n", target, offset, length );
  funcs->ext.p_glFlushMappedBufferRange( target, offset, length );
}

static void WINAPI wine_glFlushMappedBufferRangeAPPLE( GLenum target, INT_PTR offset, INT_PTR size ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %ld, %ld)\n", target, offset, size );
  funcs->ext.p_glFlushMappedBufferRangeAPPLE( target, offset, size );
}

static void WINAPI wine_glFlushMappedNamedBufferRangeEXT( GLuint buffer, INT_PTR offset, INT_PTR length ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %ld, %ld)\n", buffer, offset, length );
  funcs->ext.p_glFlushMappedNamedBufferRangeEXT( buffer, offset, length );
}

static void WINAPI wine_glFlushPixelDataRangeNV( GLenum target ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", target );
  funcs->ext.p_glFlushPixelDataRangeNV( target );
}

static void WINAPI wine_glFlushRasterSGIX( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  funcs->ext.p_glFlushRasterSGIX( );
}

static void WINAPI wine_glFlushVertexArrayRangeAPPLE( GLsizei length, GLvoid* pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", length, pointer );
  funcs->ext.p_glFlushVertexArrayRangeAPPLE( length, pointer );
}

static void WINAPI wine_glFlushVertexArrayRangeNV( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  funcs->ext.p_glFlushVertexArrayRangeNV( );
}

static void WINAPI wine_glFogCoordFormatNV( GLenum type, GLsizei stride ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", type, stride );
  funcs->ext.p_glFogCoordFormatNV( type, stride );
}

static void WINAPI wine_glFogCoordPointer( GLenum type, GLsizei stride, const GLvoid* pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", type, stride, pointer );
  funcs->ext.p_glFogCoordPointer( type, stride, pointer );
}

static void WINAPI wine_glFogCoordPointerEXT( GLenum type, GLsizei stride, const GLvoid* pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", type, stride, pointer );
  funcs->ext.p_glFogCoordPointerEXT( type, stride, pointer );
}

static void WINAPI wine_glFogCoordPointerListIBM( GLenum type, GLint stride, const GLvoid** pointer, GLint ptrstride ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %d)\n", type, stride, pointer, ptrstride );
  funcs->ext.p_glFogCoordPointerListIBM( type, stride, pointer, ptrstride );
}

static void WINAPI wine_glFogCoordd( GLdouble coord ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f)\n", coord );
  funcs->ext.p_glFogCoordd( coord );
}

static void WINAPI wine_glFogCoorddEXT( GLdouble coord ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f)\n", coord );
  funcs->ext.p_glFogCoorddEXT( coord );
}

static void WINAPI wine_glFogCoorddv( const GLdouble* coord ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", coord );
  funcs->ext.p_glFogCoorddv( coord );
}

static void WINAPI wine_glFogCoorddvEXT( const GLdouble* coord ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", coord );
  funcs->ext.p_glFogCoorddvEXT( coord );
}

static void WINAPI wine_glFogCoordf( GLfloat coord ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f)\n", coord );
  funcs->ext.p_glFogCoordf( coord );
}

static void WINAPI wine_glFogCoordfEXT( GLfloat coord ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f)\n", coord );
  funcs->ext.p_glFogCoordfEXT( coord );
}

static void WINAPI wine_glFogCoordfv( const GLfloat* coord ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", coord );
  funcs->ext.p_glFogCoordfv( coord );
}

static void WINAPI wine_glFogCoordfvEXT( const GLfloat* coord ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", coord );
  funcs->ext.p_glFogCoordfvEXT( coord );
}

static void WINAPI wine_glFogCoordhNV( unsigned short fog ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", fog );
  funcs->ext.p_glFogCoordhNV( fog );
}

static void WINAPI wine_glFogCoordhvNV( const unsigned short* fog ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", fog );
  funcs->ext.p_glFogCoordhvNV( fog );
}

static void WINAPI wine_glFogFuncSGIS( GLsizei n, const GLfloat* points ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, points );
  funcs->ext.p_glFogFuncSGIS( n, points );
}

static void WINAPI wine_glFragmentColorMaterialSGIX( GLenum face, GLenum mode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", face, mode );
  funcs->ext.p_glFragmentColorMaterialSGIX( face, mode );
}

static void WINAPI wine_glFragmentLightModelfSGIX( GLenum pname, GLfloat param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f)\n", pname, param );
  funcs->ext.p_glFragmentLightModelfSGIX( pname, param );
}

static void WINAPI wine_glFragmentLightModelfvSGIX( GLenum pname, const GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", pname, params );
  funcs->ext.p_glFragmentLightModelfvSGIX( pname, params );
}

static void WINAPI wine_glFragmentLightModeliSGIX( GLenum pname, GLint param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", pname, param );
  funcs->ext.p_glFragmentLightModeliSGIX( pname, param );
}

static void WINAPI wine_glFragmentLightModelivSGIX( GLenum pname, const GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", pname, params );
  funcs->ext.p_glFragmentLightModelivSGIX( pname, params );
}

static void WINAPI wine_glFragmentLightfSGIX( GLenum light, GLenum pname, GLfloat param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f)\n", light, pname, param );
  funcs->ext.p_glFragmentLightfSGIX( light, pname, param );
}

static void WINAPI wine_glFragmentLightfvSGIX( GLenum light, GLenum pname, const GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", light, pname, params );
  funcs->ext.p_glFragmentLightfvSGIX( light, pname, params );
}

static void WINAPI wine_glFragmentLightiSGIX( GLenum light, GLenum pname, GLint param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", light, pname, param );
  funcs->ext.p_glFragmentLightiSGIX( light, pname, param );
}

static void WINAPI wine_glFragmentLightivSGIX( GLenum light, GLenum pname, const GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", light, pname, params );
  funcs->ext.p_glFragmentLightivSGIX( light, pname, params );
}

static void WINAPI wine_glFragmentMaterialfSGIX( GLenum face, GLenum pname, GLfloat param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f)\n", face, pname, param );
  funcs->ext.p_glFragmentMaterialfSGIX( face, pname, param );
}

static void WINAPI wine_glFragmentMaterialfvSGIX( GLenum face, GLenum pname, const GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", face, pname, params );
  funcs->ext.p_glFragmentMaterialfvSGIX( face, pname, params );
}

static void WINAPI wine_glFragmentMaterialiSGIX( GLenum face, GLenum pname, GLint param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", face, pname, param );
  funcs->ext.p_glFragmentMaterialiSGIX( face, pname, param );
}

static void WINAPI wine_glFragmentMaterialivSGIX( GLenum face, GLenum pname, const GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", face, pname, params );
  funcs->ext.p_glFragmentMaterialivSGIX( face, pname, params );
}

static void WINAPI wine_glFrameTerminatorGREMEDY( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  funcs->ext.p_glFrameTerminatorGREMEDY( );
}

static void WINAPI wine_glFrameZoomSGIX( GLint factor ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", factor );
  funcs->ext.p_glFrameZoomSGIX( factor );
}

static void WINAPI wine_glFramebufferDrawBufferEXT( GLuint framebuffer, GLenum mode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", framebuffer, mode );
  funcs->ext.p_glFramebufferDrawBufferEXT( framebuffer, mode );
}

static void WINAPI wine_glFramebufferDrawBuffersEXT( GLuint framebuffer, GLsizei n, const GLenum* bufs ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", framebuffer, n, bufs );
  funcs->ext.p_glFramebufferDrawBuffersEXT( framebuffer, n, bufs );
}

static void WINAPI wine_glFramebufferReadBufferEXT( GLuint framebuffer, GLenum mode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", framebuffer, mode );
  funcs->ext.p_glFramebufferReadBufferEXT( framebuffer, mode );
}

static void WINAPI wine_glFramebufferRenderbuffer( GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", target, attachment, renderbuffertarget, renderbuffer );
  funcs->ext.p_glFramebufferRenderbuffer( target, attachment, renderbuffertarget, renderbuffer );
}

static void WINAPI wine_glFramebufferRenderbufferEXT( GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", target, attachment, renderbuffertarget, renderbuffer );
  funcs->ext.p_glFramebufferRenderbufferEXT( target, attachment, renderbuffertarget, renderbuffer );
}

static void WINAPI wine_glFramebufferTexture( GLenum target, GLenum attachment, GLuint texture, GLint level ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", target, attachment, texture, level );
  funcs->ext.p_glFramebufferTexture( target, attachment, texture, level );
}

static void WINAPI wine_glFramebufferTexture1D( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", target, attachment, textarget, texture, level );
  funcs->ext.p_glFramebufferTexture1D( target, attachment, textarget, texture, level );
}

static void WINAPI wine_glFramebufferTexture1DEXT( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", target, attachment, textarget, texture, level );
  funcs->ext.p_glFramebufferTexture1DEXT( target, attachment, textarget, texture, level );
}

static void WINAPI wine_glFramebufferTexture2D( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", target, attachment, textarget, texture, level );
  funcs->ext.p_glFramebufferTexture2D( target, attachment, textarget, texture, level );
}

static void WINAPI wine_glFramebufferTexture2DEXT( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", target, attachment, textarget, texture, level );
  funcs->ext.p_glFramebufferTexture2DEXT( target, attachment, textarget, texture, level );
}

static void WINAPI wine_glFramebufferTexture3D( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d)\n", target, attachment, textarget, texture, level, zoffset );
  funcs->ext.p_glFramebufferTexture3D( target, attachment, textarget, texture, level, zoffset );
}

static void WINAPI wine_glFramebufferTexture3DEXT( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d)\n", target, attachment, textarget, texture, level, zoffset );
  funcs->ext.p_glFramebufferTexture3DEXT( target, attachment, textarget, texture, level, zoffset );
}

static void WINAPI wine_glFramebufferTextureARB( GLenum target, GLenum attachment, GLuint texture, GLint level ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", target, attachment, texture, level );
  funcs->ext.p_glFramebufferTextureARB( target, attachment, texture, level );
}

static void WINAPI wine_glFramebufferTextureEXT( GLenum target, GLenum attachment, GLuint texture, GLint level ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", target, attachment, texture, level );
  funcs->ext.p_glFramebufferTextureEXT( target, attachment, texture, level );
}

static void WINAPI wine_glFramebufferTextureFaceARB( GLenum target, GLenum attachment, GLuint texture, GLint level, GLenum face ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", target, attachment, texture, level, face );
  funcs->ext.p_glFramebufferTextureFaceARB( target, attachment, texture, level, face );
}

static void WINAPI wine_glFramebufferTextureFaceEXT( GLenum target, GLenum attachment, GLuint texture, GLint level, GLenum face ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", target, attachment, texture, level, face );
  funcs->ext.p_glFramebufferTextureFaceEXT( target, attachment, texture, level, face );
}

static void WINAPI wine_glFramebufferTextureLayer( GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", target, attachment, texture, level, layer );
  funcs->ext.p_glFramebufferTextureLayer( target, attachment, texture, level, layer );
}

static void WINAPI wine_glFramebufferTextureLayerARB( GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", target, attachment, texture, level, layer );
  funcs->ext.p_glFramebufferTextureLayerARB( target, attachment, texture, level, layer );
}

static void WINAPI wine_glFramebufferTextureLayerEXT( GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", target, attachment, texture, level, layer );
  funcs->ext.p_glFramebufferTextureLayerEXT( target, attachment, texture, level, layer );
}

static void WINAPI wine_glFreeObjectBufferATI( GLuint buffer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", buffer );
  funcs->ext.p_glFreeObjectBufferATI( buffer );
}

static GLuint WINAPI wine_glGenAsyncMarkersSGIX( GLsizei range ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", range );
  return funcs->ext.p_glGenAsyncMarkersSGIX( range );
}

static void WINAPI wine_glGenBuffers( GLsizei n, GLuint* buffers ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, buffers );
  funcs->ext.p_glGenBuffers( n, buffers );
}

static void WINAPI wine_glGenBuffersARB( GLsizei n, GLuint* buffers ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, buffers );
  funcs->ext.p_glGenBuffersARB( n, buffers );
}

static void WINAPI wine_glGenFencesAPPLE( GLsizei n, GLuint* fences ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, fences );
  funcs->ext.p_glGenFencesAPPLE( n, fences );
}

static void WINAPI wine_glGenFencesNV( GLsizei n, GLuint* fences ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, fences );
  funcs->ext.p_glGenFencesNV( n, fences );
}

static GLuint WINAPI wine_glGenFragmentShadersATI( GLuint range ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", range );
  return funcs->ext.p_glGenFragmentShadersATI( range );
}

static void WINAPI wine_glGenFramebuffers( GLsizei n, GLuint* framebuffers ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, framebuffers );
  funcs->ext.p_glGenFramebuffers( n, framebuffers );
}

static void WINAPI wine_glGenFramebuffersEXT( GLsizei n, GLuint* framebuffers ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, framebuffers );
  funcs->ext.p_glGenFramebuffersEXT( n, framebuffers );
}

static void WINAPI wine_glGenNamesAMD( GLenum identifier, GLuint num, GLuint* names ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", identifier, num, names );
  funcs->ext.p_glGenNamesAMD( identifier, num, names );
}

static void WINAPI wine_glGenOcclusionQueriesNV( GLsizei n, GLuint* ids ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, ids );
  funcs->ext.p_glGenOcclusionQueriesNV( n, ids );
}

static GLuint WINAPI wine_glGenPathsNV( GLsizei range ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", range );
  return funcs->ext.p_glGenPathsNV( range );
}

static void WINAPI wine_glGenPerfMonitorsAMD( GLsizei n, GLuint* monitors ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, monitors );
  funcs->ext.p_glGenPerfMonitorsAMD( n, monitors );
}

static void WINAPI wine_glGenProgramPipelines( GLsizei n, GLuint* pipelines ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, pipelines );
  funcs->ext.p_glGenProgramPipelines( n, pipelines );
}

static void WINAPI wine_glGenProgramsARB( GLsizei n, GLuint* programs ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, programs );
  funcs->ext.p_glGenProgramsARB( n, programs );
}

static void WINAPI wine_glGenProgramsNV( GLsizei n, GLuint* programs ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, programs );
  funcs->ext.p_glGenProgramsNV( n, programs );
}

static void WINAPI wine_glGenQueries( GLsizei n, GLuint* ids ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, ids );
  funcs->ext.p_glGenQueries( n, ids );
}

static void WINAPI wine_glGenQueriesARB( GLsizei n, GLuint* ids ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, ids );
  funcs->ext.p_glGenQueriesARB( n, ids );
}

static void WINAPI wine_glGenRenderbuffers( GLsizei n, GLuint* renderbuffers ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, renderbuffers );
  funcs->ext.p_glGenRenderbuffers( n, renderbuffers );
}

static void WINAPI wine_glGenRenderbuffersEXT( GLsizei n, GLuint* renderbuffers ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, renderbuffers );
  funcs->ext.p_glGenRenderbuffersEXT( n, renderbuffers );
}

static void WINAPI wine_glGenSamplers( GLsizei count, GLuint* samplers ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", count, samplers );
  funcs->ext.p_glGenSamplers( count, samplers );
}

static GLuint WINAPI wine_glGenSymbolsEXT( GLenum datatype, GLenum storagetype, GLenum range, GLuint components ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", datatype, storagetype, range, components );
  return funcs->ext.p_glGenSymbolsEXT( datatype, storagetype, range, components );
}

static void WINAPI wine_glGenTexturesEXT( GLsizei n, GLuint* textures ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, textures );
  funcs->ext.p_glGenTexturesEXT( n, textures );
}

static void WINAPI wine_glGenTransformFeedbacks( GLsizei n, GLuint* ids ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, ids );
  funcs->ext.p_glGenTransformFeedbacks( n, ids );
}

static void WINAPI wine_glGenTransformFeedbacksNV( GLsizei n, GLuint* ids ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, ids );
  funcs->ext.p_glGenTransformFeedbacksNV( n, ids );
}

static void WINAPI wine_glGenVertexArrays( GLsizei n, GLuint* arrays ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, arrays );
  funcs->ext.p_glGenVertexArrays( n, arrays );
}

static void WINAPI wine_glGenVertexArraysAPPLE( GLsizei n, GLuint* arrays ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, arrays );
  funcs->ext.p_glGenVertexArraysAPPLE( n, arrays );
}

static GLuint WINAPI wine_glGenVertexShadersEXT( GLuint range ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", range );
  return funcs->ext.p_glGenVertexShadersEXT( range );
}

static void WINAPI wine_glGenerateMipmap( GLenum target ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", target );
  funcs->ext.p_glGenerateMipmap( target );
}

static void WINAPI wine_glGenerateMipmapEXT( GLenum target ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", target );
  funcs->ext.p_glGenerateMipmapEXT( target );
}

static void WINAPI wine_glGenerateMultiTexMipmapEXT( GLenum texunit, GLenum target ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", texunit, target );
  funcs->ext.p_glGenerateMultiTexMipmapEXT( texunit, target );
}

static void WINAPI wine_glGenerateTextureMipmapEXT( GLuint texture, GLenum target ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", texture, target );
  funcs->ext.p_glGenerateTextureMipmapEXT( texture, target );
}

static void WINAPI wine_glGetActiveAtomicCounterBufferiv( GLuint program, GLuint bufferIndex, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, bufferIndex, pname, params );
  funcs->ext.p_glGetActiveAtomicCounterBufferiv( program, bufferIndex, pname, params );
}

static void WINAPI wine_glGetActiveAttrib( GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, char* name ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p, %p, %p, %p)\n", program, index, bufSize, length, size, type, name );
  funcs->ext.p_glGetActiveAttrib( program, index, bufSize, length, size, type, name );
}

static void WINAPI wine_glGetActiveAttribARB( unsigned int programObj, GLuint index, GLsizei maxLength, GLsizei* length, GLint* size, GLenum* type, char* name ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p, %p, %p, %p)\n", programObj, index, maxLength, length, size, type, name );
  funcs->ext.p_glGetActiveAttribARB( programObj, index, maxLength, length, size, type, name );
}

static void WINAPI wine_glGetActiveSubroutineName( GLuint program, GLenum shadertype, GLuint index, GLsizei bufsize, GLsizei* length, char* name ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p, %p)\n", program, shadertype, index, bufsize, length, name );
  funcs->ext.p_glGetActiveSubroutineName( program, shadertype, index, bufsize, length, name );
}

static void WINAPI wine_glGetActiveSubroutineUniformName( GLuint program, GLenum shadertype, GLuint index, GLsizei bufsize, GLsizei* length, char* name ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p, %p)\n", program, shadertype, index, bufsize, length, name );
  funcs->ext.p_glGetActiveSubroutineUniformName( program, shadertype, index, bufsize, length, name );
}

static void WINAPI wine_glGetActiveSubroutineUniformiv( GLuint program, GLenum shadertype, GLuint index, GLenum pname, GLint* values ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, shadertype, index, pname, values );
  funcs->ext.p_glGetActiveSubroutineUniformiv( program, shadertype, index, pname, values );
}

static void WINAPI wine_glGetActiveUniform( GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, char* name ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p, %p, %p, %p)\n", program, index, bufSize, length, size, type, name );
  funcs->ext.p_glGetActiveUniform( program, index, bufSize, length, size, type, name );
}

static void WINAPI wine_glGetActiveUniformARB( unsigned int programObj, GLuint index, GLsizei maxLength, GLsizei* length, GLint* size, GLenum* type, char* name ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p, %p, %p, %p)\n", programObj, index, maxLength, length, size, type, name );
  funcs->ext.p_glGetActiveUniformARB( programObj, index, maxLength, length, size, type, name );
}

static void WINAPI wine_glGetActiveUniformBlockName( GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei* length, char* uniformBlockName ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p, %p)\n", program, uniformBlockIndex, bufSize, length, uniformBlockName );
  funcs->ext.p_glGetActiveUniformBlockName( program, uniformBlockIndex, bufSize, length, uniformBlockName );
}

static void WINAPI wine_glGetActiveUniformBlockiv( GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, uniformBlockIndex, pname, params );
  funcs->ext.p_glGetActiveUniformBlockiv( program, uniformBlockIndex, pname, params );
}

static void WINAPI wine_glGetActiveUniformName( GLuint program, GLuint uniformIndex, GLsizei bufSize, GLsizei* length, char* uniformName ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p, %p)\n", program, uniformIndex, bufSize, length, uniformName );
  funcs->ext.p_glGetActiveUniformName( program, uniformIndex, bufSize, length, uniformName );
}

static void WINAPI wine_glGetActiveUniformsiv( GLuint program, GLsizei uniformCount, const GLuint* uniformIndices, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %d, %p)\n", program, uniformCount, uniformIndices, pname, params );
  funcs->ext.p_glGetActiveUniformsiv( program, uniformCount, uniformIndices, pname, params );
}

static void WINAPI wine_glGetActiveVaryingNV( GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLsizei* size, GLenum* type, char* name ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p, %p, %p, %p)\n", program, index, bufSize, length, size, type, name );
  funcs->ext.p_glGetActiveVaryingNV( program, index, bufSize, length, size, type, name );
}

static void WINAPI wine_glGetArrayObjectfvATI( GLenum array, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", array, pname, params );
  funcs->ext.p_glGetArrayObjectfvATI( array, pname, params );
}

static void WINAPI wine_glGetArrayObjectivATI( GLenum array, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", array, pname, params );
  funcs->ext.p_glGetArrayObjectivATI( array, pname, params );
}

static void WINAPI wine_glGetAttachedObjectsARB( unsigned int containerObj, GLsizei maxCount, GLsizei* count, unsigned int* obj ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %p)\n", containerObj, maxCount, count, obj );
  funcs->ext.p_glGetAttachedObjectsARB( containerObj, maxCount, count, obj );
}

static void WINAPI wine_glGetAttachedShaders( GLuint program, GLsizei maxCount, GLsizei* count, GLuint* obj ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %p)\n", program, maxCount, count, obj );
  funcs->ext.p_glGetAttachedShaders( program, maxCount, count, obj );
}

static GLint WINAPI wine_glGetAttribLocation( GLuint program, const char* name ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", program, name );
  return funcs->ext.p_glGetAttribLocation( program, name );
}

static GLint WINAPI wine_glGetAttribLocationARB( unsigned int programObj, const char* name ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", programObj, name );
  return funcs->ext.p_glGetAttribLocationARB( programObj, name );
}

static void WINAPI wine_glGetBooleanIndexedvEXT( GLenum target, GLuint index, GLboolean* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, index, data );
  funcs->ext.p_glGetBooleanIndexedvEXT( target, index, data );
}

static void WINAPI wine_glGetBooleani_v( GLenum target, GLuint index, GLboolean* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, index, data );
  funcs->ext.p_glGetBooleani_v( target, index, data );
}

static void WINAPI wine_glGetBufferParameteri64v( GLenum target, GLenum pname, INT64* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetBufferParameteri64v( target, pname, params );
}

static void WINAPI wine_glGetBufferParameteriv( GLenum target, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetBufferParameteriv( target, pname, params );
}

static void WINAPI wine_glGetBufferParameterivARB( GLenum target, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetBufferParameterivARB( target, pname, params );
}

static void WINAPI wine_glGetBufferParameterui64vNV( GLenum target, GLenum pname, UINT64* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetBufferParameterui64vNV( target, pname, params );
}

static void WINAPI wine_glGetBufferPointerv( GLenum target, GLenum pname, GLvoid** params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetBufferPointerv( target, pname, params );
}

static void WINAPI wine_glGetBufferPointervARB( GLenum target, GLenum pname, GLvoid** params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetBufferPointervARB( target, pname, params );
}

static void WINAPI wine_glGetBufferSubData( GLenum target, INT_PTR offset, INT_PTR size, GLvoid* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %ld, %ld, %p)\n", target, offset, size, data );
  funcs->ext.p_glGetBufferSubData( target, offset, size, data );
}

static void WINAPI wine_glGetBufferSubDataARB( GLenum target, INT_PTR offset, INT_PTR size, GLvoid* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %ld, %ld, %p)\n", target, offset, size, data );
  funcs->ext.p_glGetBufferSubDataARB( target, offset, size, data );
}

static void WINAPI wine_glGetColorTable( GLenum target, GLenum format, GLenum type, GLvoid* table ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", target, format, type, table );
  funcs->ext.p_glGetColorTable( target, format, type, table );
}

static void WINAPI wine_glGetColorTableEXT( GLenum target, GLenum format, GLenum type, GLvoid* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", target, format, type, data );
  funcs->ext.p_glGetColorTableEXT( target, format, type, data );
}

static void WINAPI wine_glGetColorTableParameterfv( GLenum target, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetColorTableParameterfv( target, pname, params );
}

static void WINAPI wine_glGetColorTableParameterfvEXT( GLenum target, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetColorTableParameterfvEXT( target, pname, params );
}

static void WINAPI wine_glGetColorTableParameterfvSGI( GLenum target, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetColorTableParameterfvSGI( target, pname, params );
}

static void WINAPI wine_glGetColorTableParameteriv( GLenum target, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetColorTableParameteriv( target, pname, params );
}

static void WINAPI wine_glGetColorTableParameterivEXT( GLenum target, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetColorTableParameterivEXT( target, pname, params );
}

static void WINAPI wine_glGetColorTableParameterivSGI( GLenum target, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetColorTableParameterivSGI( target, pname, params );
}

static void WINAPI wine_glGetColorTableSGI( GLenum target, GLenum format, GLenum type, GLvoid* table ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", target, format, type, table );
  funcs->ext.p_glGetColorTableSGI( target, format, type, table );
}

static void WINAPI wine_glGetCombinerInputParameterfvNV( GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", stage, portion, variable, pname, params );
  funcs->ext.p_glGetCombinerInputParameterfvNV( stage, portion, variable, pname, params );
}

static void WINAPI wine_glGetCombinerInputParameterivNV( GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", stage, portion, variable, pname, params );
  funcs->ext.p_glGetCombinerInputParameterivNV( stage, portion, variable, pname, params );
}

static void WINAPI wine_glGetCombinerOutputParameterfvNV( GLenum stage, GLenum portion, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", stage, portion, pname, params );
  funcs->ext.p_glGetCombinerOutputParameterfvNV( stage, portion, pname, params );
}

static void WINAPI wine_glGetCombinerOutputParameterivNV( GLenum stage, GLenum portion, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", stage, portion, pname, params );
  funcs->ext.p_glGetCombinerOutputParameterivNV( stage, portion, pname, params );
}

static void WINAPI wine_glGetCombinerStageParameterfvNV( GLenum stage, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", stage, pname, params );
  funcs->ext.p_glGetCombinerStageParameterfvNV( stage, pname, params );
}

static void WINAPI wine_glGetCompressedMultiTexImageEXT( GLenum texunit, GLenum target, GLint lod, GLvoid* img ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", texunit, target, lod, img );
  funcs->ext.p_glGetCompressedMultiTexImageEXT( texunit, target, lod, img );
}

static void WINAPI wine_glGetCompressedTexImage( GLenum target, GLint level, GLvoid* img ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, level, img );
  funcs->ext.p_glGetCompressedTexImage( target, level, img );
}

static void WINAPI wine_glGetCompressedTexImageARB( GLenum target, GLint level, GLvoid* img ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, level, img );
  funcs->ext.p_glGetCompressedTexImageARB( target, level, img );
}

static void WINAPI wine_glGetCompressedTextureImageEXT( GLuint texture, GLenum target, GLint lod, GLvoid* img ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", texture, target, lod, img );
  funcs->ext.p_glGetCompressedTextureImageEXT( texture, target, lod, img );
}

static void WINAPI wine_glGetConvolutionFilter( GLenum target, GLenum format, GLenum type, GLvoid* image ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", target, format, type, image );
  funcs->ext.p_glGetConvolutionFilter( target, format, type, image );
}

static void WINAPI wine_glGetConvolutionFilterEXT( GLenum target, GLenum format, GLenum type, GLvoid* image ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", target, format, type, image );
  funcs->ext.p_glGetConvolutionFilterEXT( target, format, type, image );
}

static void WINAPI wine_glGetConvolutionParameterfv( GLenum target, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetConvolutionParameterfv( target, pname, params );
}

static void WINAPI wine_glGetConvolutionParameterfvEXT( GLenum target, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetConvolutionParameterfvEXT( target, pname, params );
}

static void WINAPI wine_glGetConvolutionParameteriv( GLenum target, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetConvolutionParameteriv( target, pname, params );
}

static void WINAPI wine_glGetConvolutionParameterivEXT( GLenum target, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetConvolutionParameterivEXT( target, pname, params );
}

static GLuint WINAPI wine_glGetDebugMessageLogAMD( GLuint count, GLsizei bufsize, GLenum* categories, GLuint* severities, GLuint* ids, GLsizei* lengths, char* message ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %p, %p, %p, %p)\n", count, bufsize, categories, severities, ids, lengths, message );
  return funcs->ext.p_glGetDebugMessageLogAMD( count, bufsize, categories, severities, ids, lengths, message );
}

static GLuint WINAPI wine_glGetDebugMessageLogARB( GLuint count, GLsizei bufsize, GLenum* sources, GLenum* types, GLuint* ids, GLenum* severities, GLsizei* lengths, char* messageLog ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %p, %p, %p, %p, %p)\n", count, bufsize, sources, types, ids, severities, lengths, messageLog );
  return funcs->ext.p_glGetDebugMessageLogARB( count, bufsize, sources, types, ids, severities, lengths, messageLog );
}

static void WINAPI wine_glGetDetailTexFuncSGIS( GLenum target, GLfloat* points ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, points );
  funcs->ext.p_glGetDetailTexFuncSGIS( target, points );
}

static void WINAPI wine_glGetDoubleIndexedvEXT( GLenum target, GLuint index, GLdouble* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, index, data );
  funcs->ext.p_glGetDoubleIndexedvEXT( target, index, data );
}

static void WINAPI wine_glGetDoublei_v( GLenum target, GLuint index, GLdouble* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, index, data );
  funcs->ext.p_glGetDoublei_v( target, index, data );
}

static void WINAPI wine_glGetFenceivNV( GLuint fence, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", fence, pname, params );
  funcs->ext.p_glGetFenceivNV( fence, pname, params );
}

static void WINAPI wine_glGetFinalCombinerInputParameterfvNV( GLenum variable, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", variable, pname, params );
  funcs->ext.p_glGetFinalCombinerInputParameterfvNV( variable, pname, params );
}

static void WINAPI wine_glGetFinalCombinerInputParameterivNV( GLenum variable, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", variable, pname, params );
  funcs->ext.p_glGetFinalCombinerInputParameterivNV( variable, pname, params );
}

static void WINAPI wine_glGetFloatIndexedvEXT( GLenum target, GLuint index, GLfloat* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, index, data );
  funcs->ext.p_glGetFloatIndexedvEXT( target, index, data );
}

static void WINAPI wine_glGetFloati_v( GLenum target, GLuint index, GLfloat* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, index, data );
  funcs->ext.p_glGetFloati_v( target, index, data );
}

static void WINAPI wine_glGetFogFuncSGIS( GLfloat* points ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", points );
  funcs->ext.p_glGetFogFuncSGIS( points );
}

static GLint WINAPI wine_glGetFragDataIndex( GLuint program, const char* name ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", program, name );
  return funcs->ext.p_glGetFragDataIndex( program, name );
}

static GLint WINAPI wine_glGetFragDataLocation( GLuint program, const char* name ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", program, name );
  return funcs->ext.p_glGetFragDataLocation( program, name );
}

static GLint WINAPI wine_glGetFragDataLocationEXT( GLuint program, const char* name ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", program, name );
  return funcs->ext.p_glGetFragDataLocationEXT( program, name );
}

static void WINAPI wine_glGetFragmentLightfvSGIX( GLenum light, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", light, pname, params );
  funcs->ext.p_glGetFragmentLightfvSGIX( light, pname, params );
}

static void WINAPI wine_glGetFragmentLightivSGIX( GLenum light, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", light, pname, params );
  funcs->ext.p_glGetFragmentLightivSGIX( light, pname, params );
}

static void WINAPI wine_glGetFragmentMaterialfvSGIX( GLenum face, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", face, pname, params );
  funcs->ext.p_glGetFragmentMaterialfvSGIX( face, pname, params );
}

static void WINAPI wine_glGetFragmentMaterialivSGIX( GLenum face, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", face, pname, params );
  funcs->ext.p_glGetFragmentMaterialivSGIX( face, pname, params );
}

static void WINAPI wine_glGetFramebufferAttachmentParameteriv( GLenum target, GLenum attachment, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", target, attachment, pname, params );
  funcs->ext.p_glGetFramebufferAttachmentParameteriv( target, attachment, pname, params );
}

static void WINAPI wine_glGetFramebufferAttachmentParameterivEXT( GLenum target, GLenum attachment, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", target, attachment, pname, params );
  funcs->ext.p_glGetFramebufferAttachmentParameterivEXT( target, attachment, pname, params );
}

static void WINAPI wine_glGetFramebufferParameterivEXT( GLuint framebuffer, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", framebuffer, pname, params );
  funcs->ext.p_glGetFramebufferParameterivEXT( framebuffer, pname, params );
}

static GLenum WINAPI wine_glGetGraphicsResetStatusARB( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  return funcs->ext.p_glGetGraphicsResetStatusARB( );
}

static unsigned int WINAPI wine_glGetHandleARB( GLenum pname ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", pname );
  return funcs->ext.p_glGetHandleARB( pname );
}

static void WINAPI wine_glGetHistogram( GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid* values ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", target, reset, format, type, values );
  funcs->ext.p_glGetHistogram( target, reset, format, type, values );
}

static void WINAPI wine_glGetHistogramEXT( GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid* values ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", target, reset, format, type, values );
  funcs->ext.p_glGetHistogramEXT( target, reset, format, type, values );
}

static void WINAPI wine_glGetHistogramParameterfv( GLenum target, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetHistogramParameterfv( target, pname, params );
}

static void WINAPI wine_glGetHistogramParameterfvEXT( GLenum target, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetHistogramParameterfvEXT( target, pname, params );
}

static void WINAPI wine_glGetHistogramParameteriv( GLenum target, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetHistogramParameteriv( target, pname, params );
}

static void WINAPI wine_glGetHistogramParameterivEXT( GLenum target, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetHistogramParameterivEXT( target, pname, params );
}

static UINT64 WINAPI wine_glGetImageHandleNV( GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum format ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", texture, level, layered, layer, format );
  return funcs->ext.p_glGetImageHandleNV( texture, level, layered, layer, format );
}

static void WINAPI wine_glGetImageTransformParameterfvHP( GLenum target, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetImageTransformParameterfvHP( target, pname, params );
}

static void WINAPI wine_glGetImageTransformParameterivHP( GLenum target, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetImageTransformParameterivHP( target, pname, params );
}

static void WINAPI wine_glGetInfoLogARB( unsigned int obj, GLsizei maxLength, GLsizei* length, char* infoLog ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %p)\n", obj, maxLength, length, infoLog );
  funcs->ext.p_glGetInfoLogARB( obj, maxLength, length, infoLog );
}

static GLint WINAPI wine_glGetInstrumentsSGIX( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  return funcs->ext.p_glGetInstrumentsSGIX( );
}

static void WINAPI wine_glGetInteger64i_v( GLenum target, GLuint index, INT64* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, index, data );
  funcs->ext.p_glGetInteger64i_v( target, index, data );
}

static void WINAPI wine_glGetInteger64v( GLenum pname, INT64* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", pname, params );
  funcs->ext.p_glGetInteger64v( pname, params );
}

static void WINAPI wine_glGetIntegerIndexedvEXT( GLenum target, GLuint index, GLint* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, index, data );
  funcs->ext.p_glGetIntegerIndexedvEXT( target, index, data );
}

static void WINAPI wine_glGetIntegeri_v( GLenum target, GLuint index, GLint* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, index, data );
  funcs->ext.p_glGetIntegeri_v( target, index, data );
}

static void WINAPI wine_glGetIntegerui64i_vNV( GLenum value, GLuint index, UINT64* result ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", value, index, result );
  funcs->ext.p_glGetIntegerui64i_vNV( value, index, result );
}

static void WINAPI wine_glGetIntegerui64vNV( GLenum value, UINT64* result ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", value, result );
  funcs->ext.p_glGetIntegerui64vNV( value, result );
}

static void WINAPI wine_glGetInternalformativ( GLenum target, GLenum internalformat, GLenum pname, GLsizei bufSize, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", target, internalformat, pname, bufSize, params );
  funcs->ext.p_glGetInternalformativ( target, internalformat, pname, bufSize, params );
}

static void WINAPI wine_glGetInvariantBooleanvEXT( GLuint id, GLenum value, GLboolean* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", id, value, data );
  funcs->ext.p_glGetInvariantBooleanvEXT( id, value, data );
}

static void WINAPI wine_glGetInvariantFloatvEXT( GLuint id, GLenum value, GLfloat* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", id, value, data );
  funcs->ext.p_glGetInvariantFloatvEXT( id, value, data );
}

static void WINAPI wine_glGetInvariantIntegervEXT( GLuint id, GLenum value, GLint* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", id, value, data );
  funcs->ext.p_glGetInvariantIntegervEXT( id, value, data );
}

static void WINAPI wine_glGetListParameterfvSGIX( GLuint list, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", list, pname, params );
  funcs->ext.p_glGetListParameterfvSGIX( list, pname, params );
}

static void WINAPI wine_glGetListParameterivSGIX( GLuint list, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", list, pname, params );
  funcs->ext.p_glGetListParameterivSGIX( list, pname, params );
}

static void WINAPI wine_glGetLocalConstantBooleanvEXT( GLuint id, GLenum value, GLboolean* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", id, value, data );
  funcs->ext.p_glGetLocalConstantBooleanvEXT( id, value, data );
}

static void WINAPI wine_glGetLocalConstantFloatvEXT( GLuint id, GLenum value, GLfloat* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", id, value, data );
  funcs->ext.p_glGetLocalConstantFloatvEXT( id, value, data );
}

static void WINAPI wine_glGetLocalConstantIntegervEXT( GLuint id, GLenum value, GLint* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", id, value, data );
  funcs->ext.p_glGetLocalConstantIntegervEXT( id, value, data );
}

static void WINAPI wine_glGetMapAttribParameterfvNV( GLenum target, GLuint index, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", target, index, pname, params );
  funcs->ext.p_glGetMapAttribParameterfvNV( target, index, pname, params );
}

static void WINAPI wine_glGetMapAttribParameterivNV( GLenum target, GLuint index, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", target, index, pname, params );
  funcs->ext.p_glGetMapAttribParameterivNV( target, index, pname, params );
}

static void WINAPI wine_glGetMapControlPointsNV( GLenum target, GLuint index, GLenum type, GLsizei ustride, GLsizei vstride, GLboolean packed, GLvoid* points ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %p)\n", target, index, type, ustride, vstride, packed, points );
  funcs->ext.p_glGetMapControlPointsNV( target, index, type, ustride, vstride, packed, points );
}

static void WINAPI wine_glGetMapParameterfvNV( GLenum target, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetMapParameterfvNV( target, pname, params );
}

static void WINAPI wine_glGetMapParameterivNV( GLenum target, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetMapParameterivNV( target, pname, params );
}

static void WINAPI wine_glGetMinmax( GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid* values ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", target, reset, format, type, values );
  funcs->ext.p_glGetMinmax( target, reset, format, type, values );
}

static void WINAPI wine_glGetMinmaxEXT( GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid* values ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", target, reset, format, type, values );
  funcs->ext.p_glGetMinmaxEXT( target, reset, format, type, values );
}

static void WINAPI wine_glGetMinmaxParameterfv( GLenum target, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetMinmaxParameterfv( target, pname, params );
}

static void WINAPI wine_glGetMinmaxParameterfvEXT( GLenum target, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetMinmaxParameterfvEXT( target, pname, params );
}

static void WINAPI wine_glGetMinmaxParameteriv( GLenum target, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetMinmaxParameteriv( target, pname, params );
}

static void WINAPI wine_glGetMinmaxParameterivEXT( GLenum target, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetMinmaxParameterivEXT( target, pname, params );
}

static void WINAPI wine_glGetMultiTexEnvfvEXT( GLenum texunit, GLenum target, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", texunit, target, pname, params );
  funcs->ext.p_glGetMultiTexEnvfvEXT( texunit, target, pname, params );
}

static void WINAPI wine_glGetMultiTexEnvivEXT( GLenum texunit, GLenum target, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", texunit, target, pname, params );
  funcs->ext.p_glGetMultiTexEnvivEXT( texunit, target, pname, params );
}

static void WINAPI wine_glGetMultiTexGendvEXT( GLenum texunit, GLenum coord, GLenum pname, GLdouble* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", texunit, coord, pname, params );
  funcs->ext.p_glGetMultiTexGendvEXT( texunit, coord, pname, params );
}

static void WINAPI wine_glGetMultiTexGenfvEXT( GLenum texunit, GLenum coord, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", texunit, coord, pname, params );
  funcs->ext.p_glGetMultiTexGenfvEXT( texunit, coord, pname, params );
}

static void WINAPI wine_glGetMultiTexGenivEXT( GLenum texunit, GLenum coord, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", texunit, coord, pname, params );
  funcs->ext.p_glGetMultiTexGenivEXT( texunit, coord, pname, params );
}

static void WINAPI wine_glGetMultiTexImageEXT( GLenum texunit, GLenum target, GLint level, GLenum format, GLenum type, GLvoid* pixels ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %p)\n", texunit, target, level, format, type, pixels );
  funcs->ext.p_glGetMultiTexImageEXT( texunit, target, level, format, type, pixels );
}

static void WINAPI wine_glGetMultiTexLevelParameterfvEXT( GLenum texunit, GLenum target, GLint level, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", texunit, target, level, pname, params );
  funcs->ext.p_glGetMultiTexLevelParameterfvEXT( texunit, target, level, pname, params );
}

static void WINAPI wine_glGetMultiTexLevelParameterivEXT( GLenum texunit, GLenum target, GLint level, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", texunit, target, level, pname, params );
  funcs->ext.p_glGetMultiTexLevelParameterivEXT( texunit, target, level, pname, params );
}

static void WINAPI wine_glGetMultiTexParameterIivEXT( GLenum texunit, GLenum target, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", texunit, target, pname, params );
  funcs->ext.p_glGetMultiTexParameterIivEXT( texunit, target, pname, params );
}

static void WINAPI wine_glGetMultiTexParameterIuivEXT( GLenum texunit, GLenum target, GLenum pname, GLuint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", texunit, target, pname, params );
  funcs->ext.p_glGetMultiTexParameterIuivEXT( texunit, target, pname, params );
}

static void WINAPI wine_glGetMultiTexParameterfvEXT( GLenum texunit, GLenum target, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", texunit, target, pname, params );
  funcs->ext.p_glGetMultiTexParameterfvEXT( texunit, target, pname, params );
}

static void WINAPI wine_glGetMultiTexParameterivEXT( GLenum texunit, GLenum target, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", texunit, target, pname, params );
  funcs->ext.p_glGetMultiTexParameterivEXT( texunit, target, pname, params );
}

static void WINAPI wine_glGetMultisamplefv( GLenum pname, GLuint index, GLfloat* val ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", pname, index, val );
  funcs->ext.p_glGetMultisamplefv( pname, index, val );
}

static void WINAPI wine_glGetMultisamplefvNV( GLenum pname, GLuint index, GLfloat* val ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", pname, index, val );
  funcs->ext.p_glGetMultisamplefvNV( pname, index, val );
}

static void WINAPI wine_glGetNamedBufferParameterivEXT( GLuint buffer, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", buffer, pname, params );
  funcs->ext.p_glGetNamedBufferParameterivEXT( buffer, pname, params );
}

static void WINAPI wine_glGetNamedBufferParameterui64vNV( GLuint buffer, GLenum pname, UINT64* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", buffer, pname, params );
  funcs->ext.p_glGetNamedBufferParameterui64vNV( buffer, pname, params );
}

static void WINAPI wine_glGetNamedBufferPointervEXT( GLuint buffer, GLenum pname, GLvoid** params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", buffer, pname, params );
  funcs->ext.p_glGetNamedBufferPointervEXT( buffer, pname, params );
}

static void WINAPI wine_glGetNamedBufferSubDataEXT( GLuint buffer, INT_PTR offset, INT_PTR size, GLvoid* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %ld, %ld, %p)\n", buffer, offset, size, data );
  funcs->ext.p_glGetNamedBufferSubDataEXT( buffer, offset, size, data );
}

static void WINAPI wine_glGetNamedFramebufferAttachmentParameterivEXT( GLuint framebuffer, GLenum attachment, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", framebuffer, attachment, pname, params );
  funcs->ext.p_glGetNamedFramebufferAttachmentParameterivEXT( framebuffer, attachment, pname, params );
}

static void WINAPI wine_glGetNamedProgramLocalParameterIivEXT( GLuint program, GLenum target, GLuint index, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, target, index, params );
  funcs->ext.p_glGetNamedProgramLocalParameterIivEXT( program, target, index, params );
}

static void WINAPI wine_glGetNamedProgramLocalParameterIuivEXT( GLuint program, GLenum target, GLuint index, GLuint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, target, index, params );
  funcs->ext.p_glGetNamedProgramLocalParameterIuivEXT( program, target, index, params );
}

static void WINAPI wine_glGetNamedProgramLocalParameterdvEXT( GLuint program, GLenum target, GLuint index, GLdouble* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, target, index, params );
  funcs->ext.p_glGetNamedProgramLocalParameterdvEXT( program, target, index, params );
}

static void WINAPI wine_glGetNamedProgramLocalParameterfvEXT( GLuint program, GLenum target, GLuint index, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, target, index, params );
  funcs->ext.p_glGetNamedProgramLocalParameterfvEXT( program, target, index, params );
}

static void WINAPI wine_glGetNamedProgramStringEXT( GLuint program, GLenum target, GLenum pname, GLvoid* string ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, target, pname, string );
  funcs->ext.p_glGetNamedProgramStringEXT( program, target, pname, string );
}

static void WINAPI wine_glGetNamedProgramivEXT( GLuint program, GLenum target, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, target, pname, params );
  funcs->ext.p_glGetNamedProgramivEXT( program, target, pname, params );
}

static void WINAPI wine_glGetNamedRenderbufferParameterivEXT( GLuint renderbuffer, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", renderbuffer, pname, params );
  funcs->ext.p_glGetNamedRenderbufferParameterivEXT( renderbuffer, pname, params );
}

static void WINAPI wine_glGetNamedStringARB( GLint namelen, const char* name, GLsizei bufSize, GLint* stringlen, char* string ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p, %d, %p, %p)\n", namelen, name, bufSize, stringlen, string );
  funcs->ext.p_glGetNamedStringARB( namelen, name, bufSize, stringlen, string );
}

static void WINAPI wine_glGetNamedStringivARB( GLint namelen, const char* name, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p, %d, %p)\n", namelen, name, pname, params );
  funcs->ext.p_glGetNamedStringivARB( namelen, name, pname, params );
}

static void WINAPI wine_glGetObjectBufferfvATI( GLuint buffer, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", buffer, pname, params );
  funcs->ext.p_glGetObjectBufferfvATI( buffer, pname, params );
}

static void WINAPI wine_glGetObjectBufferivATI( GLuint buffer, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", buffer, pname, params );
  funcs->ext.p_glGetObjectBufferivATI( buffer, pname, params );
}

static void WINAPI wine_glGetObjectParameterfvARB( unsigned int obj, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", obj, pname, params );
  funcs->ext.p_glGetObjectParameterfvARB( obj, pname, params );
}

static void WINAPI wine_glGetObjectParameterivAPPLE( GLenum objectType, GLuint name, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", objectType, name, pname, params );
  funcs->ext.p_glGetObjectParameterivAPPLE( objectType, name, pname, params );
}

static void WINAPI wine_glGetObjectParameterivARB( unsigned int obj, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", obj, pname, params );
  funcs->ext.p_glGetObjectParameterivARB( obj, pname, params );
}

static void WINAPI wine_glGetOcclusionQueryivNV( GLuint id, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", id, pname, params );
  funcs->ext.p_glGetOcclusionQueryivNV( id, pname, params );
}

static void WINAPI wine_glGetOcclusionQueryuivNV( GLuint id, GLenum pname, GLuint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", id, pname, params );
  funcs->ext.p_glGetOcclusionQueryuivNV( id, pname, params );
}

static void WINAPI wine_glGetPathColorGenfvNV( GLenum color, GLenum pname, GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", color, pname, value );
  funcs->ext.p_glGetPathColorGenfvNV( color, pname, value );
}

static void WINAPI wine_glGetPathColorGenivNV( GLenum color, GLenum pname, GLint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", color, pname, value );
  funcs->ext.p_glGetPathColorGenivNV( color, pname, value );
}

static void WINAPI wine_glGetPathCommandsNV( GLuint path, GLubyte* commands ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", path, commands );
  funcs->ext.p_glGetPathCommandsNV( path, commands );
}

static void WINAPI wine_glGetPathCoordsNV( GLuint path, GLfloat* coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", path, coords );
  funcs->ext.p_glGetPathCoordsNV( path, coords );
}

static void WINAPI wine_glGetPathDashArrayNV( GLuint path, GLfloat* dashArray ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", path, dashArray );
  funcs->ext.p_glGetPathDashArrayNV( path, dashArray );
}

static GLfloat WINAPI wine_glGetPathLengthNV( GLuint path, GLsizei startSegment, GLsizei numSegments ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", path, startSegment, numSegments );
  return funcs->ext.p_glGetPathLengthNV( path, startSegment, numSegments );
}

static void WINAPI wine_glGetPathMetricRangeNV( GLbitfield metricQueryMask, GLuint firstPathName, GLsizei numPaths, GLsizei stride, GLfloat* metrics ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", metricQueryMask, firstPathName, numPaths, stride, metrics );
  funcs->ext.p_glGetPathMetricRangeNV( metricQueryMask, firstPathName, numPaths, stride, metrics );
}

static void WINAPI wine_glGetPathMetricsNV( GLbitfield metricQueryMask, GLsizei numPaths, GLenum pathNameType, const GLvoid* paths, GLuint pathBase, GLsizei stride, GLfloat* metrics ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p, %d, %d, %p)\n", metricQueryMask, numPaths, pathNameType, paths, pathBase, stride, metrics );
  funcs->ext.p_glGetPathMetricsNV( metricQueryMask, numPaths, pathNameType, paths, pathBase, stride, metrics );
}

static void WINAPI wine_glGetPathParameterfvNV( GLuint path, GLenum pname, GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", path, pname, value );
  funcs->ext.p_glGetPathParameterfvNV( path, pname, value );
}

static void WINAPI wine_glGetPathParameterivNV( GLuint path, GLenum pname, GLint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", path, pname, value );
  funcs->ext.p_glGetPathParameterivNV( path, pname, value );
}

static void WINAPI wine_glGetPathSpacingNV( GLenum pathListMode, GLsizei numPaths, GLenum pathNameType, const GLvoid* paths, GLuint pathBase, GLfloat advanceScale, GLfloat kerningScale, GLenum transformType, GLfloat* returnedSpacing ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p, %d, %f, %f, %d, %p)\n", pathListMode, numPaths, pathNameType, paths, pathBase, advanceScale, kerningScale, transformType, returnedSpacing );
  funcs->ext.p_glGetPathSpacingNV( pathListMode, numPaths, pathNameType, paths, pathBase, advanceScale, kerningScale, transformType, returnedSpacing );
}

static void WINAPI wine_glGetPathTexGenfvNV( GLenum texCoordSet, GLenum pname, GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", texCoordSet, pname, value );
  funcs->ext.p_glGetPathTexGenfvNV( texCoordSet, pname, value );
}

static void WINAPI wine_glGetPathTexGenivNV( GLenum texCoordSet, GLenum pname, GLint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", texCoordSet, pname, value );
  funcs->ext.p_glGetPathTexGenivNV( texCoordSet, pname, value );
}

static void WINAPI wine_glGetPerfMonitorCounterDataAMD( GLuint monitor, GLenum pname, GLsizei dataSize, GLuint* data, GLint* bytesWritten ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p, %p)\n", monitor, pname, dataSize, data, bytesWritten );
  funcs->ext.p_glGetPerfMonitorCounterDataAMD( monitor, pname, dataSize, data, bytesWritten );
}

static void WINAPI wine_glGetPerfMonitorCounterInfoAMD( GLuint group, GLuint counter, GLenum pname, GLvoid* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", group, counter, pname, data );
  funcs->ext.p_glGetPerfMonitorCounterInfoAMD( group, counter, pname, data );
}

static void WINAPI wine_glGetPerfMonitorCounterStringAMD( GLuint group, GLuint counter, GLsizei bufSize, GLsizei* length, char* counterString ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p, %p)\n", group, counter, bufSize, length, counterString );
  funcs->ext.p_glGetPerfMonitorCounterStringAMD( group, counter, bufSize, length, counterString );
}

static void WINAPI wine_glGetPerfMonitorCountersAMD( GLuint group, GLint* numCounters, GLint* maxActiveCounters, GLsizei counterSize, GLuint* counters ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p, %p, %d, %p)\n", group, numCounters, maxActiveCounters, counterSize, counters );
  funcs->ext.p_glGetPerfMonitorCountersAMD( group, numCounters, maxActiveCounters, counterSize, counters );
}

static void WINAPI wine_glGetPerfMonitorGroupStringAMD( GLuint group, GLsizei bufSize, GLsizei* length, char* groupString ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %p)\n", group, bufSize, length, groupString );
  funcs->ext.p_glGetPerfMonitorGroupStringAMD( group, bufSize, length, groupString );
}

static void WINAPI wine_glGetPerfMonitorGroupsAMD( GLint* numGroups, GLsizei groupsSize, GLuint* groups ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p, %d, %p)\n", numGroups, groupsSize, groups );
  funcs->ext.p_glGetPerfMonitorGroupsAMD( numGroups, groupsSize, groups );
}

static void WINAPI wine_glGetPixelTexGenParameterfvSGIS( GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", pname, params );
  funcs->ext.p_glGetPixelTexGenParameterfvSGIS( pname, params );
}

static void WINAPI wine_glGetPixelTexGenParameterivSGIS( GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", pname, params );
  funcs->ext.p_glGetPixelTexGenParameterivSGIS( pname, params );
}

static void WINAPI wine_glGetPointerIndexedvEXT( GLenum target, GLuint index, GLvoid** data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, index, data );
  funcs->ext.p_glGetPointerIndexedvEXT( target, index, data );
}

static void WINAPI wine_glGetPointervEXT( GLenum pname, GLvoid** params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", pname, params );
  funcs->ext.p_glGetPointervEXT( pname, params );
}

static void WINAPI wine_glGetProgramBinary( GLuint program, GLsizei bufSize, GLsizei* length, GLenum* binaryFormat, GLvoid* binary ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %p, %p)\n", program, bufSize, length, binaryFormat, binary );
  funcs->ext.p_glGetProgramBinary( program, bufSize, length, binaryFormat, binary );
}

static void WINAPI wine_glGetProgramEnvParameterIivNV( GLenum target, GLuint index, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, index, params );
  funcs->ext.p_glGetProgramEnvParameterIivNV( target, index, params );
}

static void WINAPI wine_glGetProgramEnvParameterIuivNV( GLenum target, GLuint index, GLuint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, index, params );
  funcs->ext.p_glGetProgramEnvParameterIuivNV( target, index, params );
}

static void WINAPI wine_glGetProgramEnvParameterdvARB( GLenum target, GLuint index, GLdouble* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, index, params );
  funcs->ext.p_glGetProgramEnvParameterdvARB( target, index, params );
}

static void WINAPI wine_glGetProgramEnvParameterfvARB( GLenum target, GLuint index, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, index, params );
  funcs->ext.p_glGetProgramEnvParameterfvARB( target, index, params );
}

static void WINAPI wine_glGetProgramInfoLog( GLuint program, GLsizei bufSize, GLsizei* length, char* infoLog ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %p)\n", program, bufSize, length, infoLog );
  funcs->ext.p_glGetProgramInfoLog( program, bufSize, length, infoLog );
}

static void WINAPI wine_glGetProgramLocalParameterIivNV( GLenum target, GLuint index, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, index, params );
  funcs->ext.p_glGetProgramLocalParameterIivNV( target, index, params );
}

static void WINAPI wine_glGetProgramLocalParameterIuivNV( GLenum target, GLuint index, GLuint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, index, params );
  funcs->ext.p_glGetProgramLocalParameterIuivNV( target, index, params );
}

static void WINAPI wine_glGetProgramLocalParameterdvARB( GLenum target, GLuint index, GLdouble* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, index, params );
  funcs->ext.p_glGetProgramLocalParameterdvARB( target, index, params );
}

static void WINAPI wine_glGetProgramLocalParameterfvARB( GLenum target, GLuint index, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, index, params );
  funcs->ext.p_glGetProgramLocalParameterfvARB( target, index, params );
}

static void WINAPI wine_glGetProgramNamedParameterdvNV( GLuint id, GLsizei len, const GLubyte* name, GLdouble* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %p)\n", id, len, name, params );
  funcs->ext.p_glGetProgramNamedParameterdvNV( id, len, name, params );
}

static void WINAPI wine_glGetProgramNamedParameterfvNV( GLuint id, GLsizei len, const GLubyte* name, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %p)\n", id, len, name, params );
  funcs->ext.p_glGetProgramNamedParameterfvNV( id, len, name, params );
}

static void WINAPI wine_glGetProgramParameterdvNV( GLenum target, GLuint index, GLenum pname, GLdouble* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", target, index, pname, params );
  funcs->ext.p_glGetProgramParameterdvNV( target, index, pname, params );
}

static void WINAPI wine_glGetProgramParameterfvNV( GLenum target, GLuint index, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", target, index, pname, params );
  funcs->ext.p_glGetProgramParameterfvNV( target, index, pname, params );
}

static void WINAPI wine_glGetProgramPipelineInfoLog( GLuint pipeline, GLsizei bufSize, GLsizei* length, char* infoLog ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %p)\n", pipeline, bufSize, length, infoLog );
  funcs->ext.p_glGetProgramPipelineInfoLog( pipeline, bufSize, length, infoLog );
}

static void WINAPI wine_glGetProgramPipelineiv( GLuint pipeline, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", pipeline, pname, params );
  funcs->ext.p_glGetProgramPipelineiv( pipeline, pname, params );
}

static void WINAPI wine_glGetProgramStageiv( GLuint program, GLenum shadertype, GLenum pname, GLint* values ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, shadertype, pname, values );
  funcs->ext.p_glGetProgramStageiv( program, shadertype, pname, values );
}

static void WINAPI wine_glGetProgramStringARB( GLenum target, GLenum pname, GLvoid* string ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, string );
  funcs->ext.p_glGetProgramStringARB( target, pname, string );
}

static void WINAPI wine_glGetProgramStringNV( GLuint id, GLenum pname, GLubyte* program ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", id, pname, program );
  funcs->ext.p_glGetProgramStringNV( id, pname, program );
}

static void WINAPI wine_glGetProgramSubroutineParameteruivNV( GLenum target, GLuint index, GLuint* param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, index, param );
  funcs->ext.p_glGetProgramSubroutineParameteruivNV( target, index, param );
}

static void WINAPI wine_glGetProgramiv( GLuint program, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", program, pname, params );
  funcs->ext.p_glGetProgramiv( program, pname, params );
}

static void WINAPI wine_glGetProgramivARB( GLenum target, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetProgramivARB( target, pname, params );
}

static void WINAPI wine_glGetProgramivNV( GLuint id, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", id, pname, params );
  funcs->ext.p_glGetProgramivNV( id, pname, params );
}

static void WINAPI wine_glGetQueryIndexediv( GLenum target, GLuint index, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", target, index, pname, params );
  funcs->ext.p_glGetQueryIndexediv( target, index, pname, params );
}

static void WINAPI wine_glGetQueryObjecti64v( GLuint id, GLenum pname, INT64* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", id, pname, params );
  funcs->ext.p_glGetQueryObjecti64v( id, pname, params );
}

static void WINAPI wine_glGetQueryObjecti64vEXT( GLuint id, GLenum pname, INT64* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", id, pname, params );
  funcs->ext.p_glGetQueryObjecti64vEXT( id, pname, params );
}

static void WINAPI wine_glGetQueryObjectiv( GLuint id, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", id, pname, params );
  funcs->ext.p_glGetQueryObjectiv( id, pname, params );
}

static void WINAPI wine_glGetQueryObjectivARB( GLuint id, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", id, pname, params );
  funcs->ext.p_glGetQueryObjectivARB( id, pname, params );
}

static void WINAPI wine_glGetQueryObjectui64v( GLuint id, GLenum pname, UINT64* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", id, pname, params );
  funcs->ext.p_glGetQueryObjectui64v( id, pname, params );
}

static void WINAPI wine_glGetQueryObjectui64vEXT( GLuint id, GLenum pname, UINT64* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", id, pname, params );
  funcs->ext.p_glGetQueryObjectui64vEXT( id, pname, params );
}

static void WINAPI wine_glGetQueryObjectuiv( GLuint id, GLenum pname, GLuint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", id, pname, params );
  funcs->ext.p_glGetQueryObjectuiv( id, pname, params );
}

static void WINAPI wine_glGetQueryObjectuivARB( GLuint id, GLenum pname, GLuint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", id, pname, params );
  funcs->ext.p_glGetQueryObjectuivARB( id, pname, params );
}

static void WINAPI wine_glGetQueryiv( GLenum target, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetQueryiv( target, pname, params );
}

static void WINAPI wine_glGetQueryivARB( GLenum target, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetQueryivARB( target, pname, params );
}

static void WINAPI wine_glGetRenderbufferParameteriv( GLenum target, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetRenderbufferParameteriv( target, pname, params );
}

static void WINAPI wine_glGetRenderbufferParameterivEXT( GLenum target, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetRenderbufferParameterivEXT( target, pname, params );
}

static void WINAPI wine_glGetSamplerParameterIiv( GLuint sampler, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", sampler, pname, params );
  funcs->ext.p_glGetSamplerParameterIiv( sampler, pname, params );
}

static void WINAPI wine_glGetSamplerParameterIuiv( GLuint sampler, GLenum pname, GLuint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", sampler, pname, params );
  funcs->ext.p_glGetSamplerParameterIuiv( sampler, pname, params );
}

static void WINAPI wine_glGetSamplerParameterfv( GLuint sampler, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", sampler, pname, params );
  funcs->ext.p_glGetSamplerParameterfv( sampler, pname, params );
}

static void WINAPI wine_glGetSamplerParameteriv( GLuint sampler, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", sampler, pname, params );
  funcs->ext.p_glGetSamplerParameteriv( sampler, pname, params );
}

static void WINAPI wine_glGetSeparableFilter( GLenum target, GLenum format, GLenum type, GLvoid* row, GLvoid* column, GLvoid* span ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p, %p, %p)\n", target, format, type, row, column, span );
  funcs->ext.p_glGetSeparableFilter( target, format, type, row, column, span );
}

static void WINAPI wine_glGetSeparableFilterEXT( GLenum target, GLenum format, GLenum type, GLvoid* row, GLvoid* column, GLvoid* span ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p, %p, %p)\n", target, format, type, row, column, span );
  funcs->ext.p_glGetSeparableFilterEXT( target, format, type, row, column, span );
}

static void WINAPI wine_glGetShaderInfoLog( GLuint shader, GLsizei bufSize, GLsizei* length, char* infoLog ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %p)\n", shader, bufSize, length, infoLog );
  funcs->ext.p_glGetShaderInfoLog( shader, bufSize, length, infoLog );
}

static void WINAPI wine_glGetShaderPrecisionFormat( GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %p)\n", shadertype, precisiontype, range, precision );
  funcs->ext.p_glGetShaderPrecisionFormat( shadertype, precisiontype, range, precision );
}

static void WINAPI wine_glGetShaderSource( GLuint shader, GLsizei bufSize, GLsizei* length, char* source ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %p)\n", shader, bufSize, length, source );
  funcs->ext.p_glGetShaderSource( shader, bufSize, length, source );
}

static void WINAPI wine_glGetShaderSourceARB( unsigned int obj, GLsizei maxLength, GLsizei* length, char* source ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %p)\n", obj, maxLength, length, source );
  funcs->ext.p_glGetShaderSourceARB( obj, maxLength, length, source );
}

static void WINAPI wine_glGetShaderiv( GLuint shader, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", shader, pname, params );
  funcs->ext.p_glGetShaderiv( shader, pname, params );
}

static void WINAPI wine_glGetSharpenTexFuncSGIS( GLenum target, GLfloat* points ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, points );
  funcs->ext.p_glGetSharpenTexFuncSGIS( target, points );
}

static const GLubyte * WINAPI wine_glGetStringi( GLenum name, GLuint index ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", name, index );
  return funcs->ext.p_glGetStringi( name, index );
}

static GLuint WINAPI wine_glGetSubroutineIndex( GLuint program, GLenum shadertype, const char* name ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", program, shadertype, name );
  return funcs->ext.p_glGetSubroutineIndex( program, shadertype, name );
}

static GLint WINAPI wine_glGetSubroutineUniformLocation( GLuint program, GLenum shadertype, const char* name ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", program, shadertype, name );
  return funcs->ext.p_glGetSubroutineUniformLocation( program, shadertype, name );
}

static void WINAPI wine_glGetSynciv( GLvoid* sync, GLenum pname, GLsizei bufSize, GLsizei* length, GLint* values ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p, %d, %d, %p, %p)\n", sync, pname, bufSize, length, values );
  funcs->ext.p_glGetSynciv( sync, pname, bufSize, length, values );
}

static void WINAPI wine_glGetTexBumpParameterfvATI( GLenum pname, GLfloat* param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", pname, param );
  funcs->ext.p_glGetTexBumpParameterfvATI( pname, param );
}

static void WINAPI wine_glGetTexBumpParameterivATI( GLenum pname, GLint* param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", pname, param );
  funcs->ext.p_glGetTexBumpParameterivATI( pname, param );
}

static void WINAPI wine_glGetTexFilterFuncSGIS( GLenum target, GLenum filter, GLfloat* weights ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, filter, weights );
  funcs->ext.p_glGetTexFilterFuncSGIS( target, filter, weights );
}

static void WINAPI wine_glGetTexParameterIiv( GLenum target, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetTexParameterIiv( target, pname, params );
}

static void WINAPI wine_glGetTexParameterIivEXT( GLenum target, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetTexParameterIivEXT( target, pname, params );
}

static void WINAPI wine_glGetTexParameterIuiv( GLenum target, GLenum pname, GLuint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetTexParameterIuiv( target, pname, params );
}

static void WINAPI wine_glGetTexParameterIuivEXT( GLenum target, GLenum pname, GLuint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetTexParameterIuivEXT( target, pname, params );
}

static void WINAPI wine_glGetTexParameterPointervAPPLE( GLenum target, GLenum pname, GLvoid** params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetTexParameterPointervAPPLE( target, pname, params );
}

static UINT64 WINAPI wine_glGetTextureHandleNV( GLuint texture ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", texture );
  return funcs->ext.p_glGetTextureHandleNV( texture );
}

static void WINAPI wine_glGetTextureImageEXT( GLuint texture, GLenum target, GLint level, GLenum format, GLenum type, GLvoid* pixels ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %p)\n", texture, target, level, format, type, pixels );
  funcs->ext.p_glGetTextureImageEXT( texture, target, level, format, type, pixels );
}

static void WINAPI wine_glGetTextureLevelParameterfvEXT( GLuint texture, GLenum target, GLint level, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", texture, target, level, pname, params );
  funcs->ext.p_glGetTextureLevelParameterfvEXT( texture, target, level, pname, params );
}

static void WINAPI wine_glGetTextureLevelParameterivEXT( GLuint texture, GLenum target, GLint level, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", texture, target, level, pname, params );
  funcs->ext.p_glGetTextureLevelParameterivEXT( texture, target, level, pname, params );
}

static void WINAPI wine_glGetTextureParameterIivEXT( GLuint texture, GLenum target, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", texture, target, pname, params );
  funcs->ext.p_glGetTextureParameterIivEXT( texture, target, pname, params );
}

static void WINAPI wine_glGetTextureParameterIuivEXT( GLuint texture, GLenum target, GLenum pname, GLuint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", texture, target, pname, params );
  funcs->ext.p_glGetTextureParameterIuivEXT( texture, target, pname, params );
}

static void WINAPI wine_glGetTextureParameterfvEXT( GLuint texture, GLenum target, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", texture, target, pname, params );
  funcs->ext.p_glGetTextureParameterfvEXT( texture, target, pname, params );
}

static void WINAPI wine_glGetTextureParameterivEXT( GLuint texture, GLenum target, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", texture, target, pname, params );
  funcs->ext.p_glGetTextureParameterivEXT( texture, target, pname, params );
}

static UINT64 WINAPI wine_glGetTextureSamplerHandleNV( GLuint texture, GLuint sampler ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", texture, sampler );
  return funcs->ext.p_glGetTextureSamplerHandleNV( texture, sampler );
}

static void WINAPI wine_glGetTrackMatrixivNV( GLenum target, GLuint address, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", target, address, pname, params );
  funcs->ext.p_glGetTrackMatrixivNV( target, address, pname, params );
}

static void WINAPI wine_glGetTransformFeedbackVarying( GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLsizei* size, GLenum* type, char* name ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p, %p, %p, %p)\n", program, index, bufSize, length, size, type, name );
  funcs->ext.p_glGetTransformFeedbackVarying( program, index, bufSize, length, size, type, name );
}

static void WINAPI wine_glGetTransformFeedbackVaryingEXT( GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLsizei* size, GLenum* type, char* name ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p, %p, %p, %p)\n", program, index, bufSize, length, size, type, name );
  funcs->ext.p_glGetTransformFeedbackVaryingEXT( program, index, bufSize, length, size, type, name );
}

static void WINAPI wine_glGetTransformFeedbackVaryingNV( GLuint program, GLuint index, GLint* location ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", program, index, location );
  funcs->ext.p_glGetTransformFeedbackVaryingNV( program, index, location );
}

static GLuint WINAPI wine_glGetUniformBlockIndex( GLuint program, const char* uniformBlockName ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", program, uniformBlockName );
  return funcs->ext.p_glGetUniformBlockIndex( program, uniformBlockName );
}

static GLint WINAPI wine_glGetUniformBufferSizeEXT( GLuint program, GLint location ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", program, location );
  return funcs->ext.p_glGetUniformBufferSizeEXT( program, location );
}

static void WINAPI wine_glGetUniformIndices( GLuint program, GLsizei uniformCount, const char* const* uniformNames, GLuint* uniformIndices ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %p)\n", program, uniformCount, uniformNames, uniformIndices );
  funcs->ext.p_glGetUniformIndices( program, uniformCount, uniformNames, uniformIndices );
}

static GLint WINAPI wine_glGetUniformLocation( GLuint program, const char* name ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", program, name );
  return funcs->ext.p_glGetUniformLocation( program, name );
}

static GLint WINAPI wine_glGetUniformLocationARB( unsigned int programObj, const char* name ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", programObj, name );
  return funcs->ext.p_glGetUniformLocationARB( programObj, name );
}

static INT_PTR WINAPI wine_glGetUniformOffsetEXT( GLuint program, GLint location ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", program, location );
  return funcs->ext.p_glGetUniformOffsetEXT( program, location );
}

static void WINAPI wine_glGetUniformSubroutineuiv( GLenum shadertype, GLint location, GLuint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", shadertype, location, params );
  funcs->ext.p_glGetUniformSubroutineuiv( shadertype, location, params );
}

static void WINAPI wine_glGetUniformdv( GLuint program, GLint location, GLdouble* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", program, location, params );
  funcs->ext.p_glGetUniformdv( program, location, params );
}

static void WINAPI wine_glGetUniformfv( GLuint program, GLint location, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", program, location, params );
  funcs->ext.p_glGetUniformfv( program, location, params );
}

static void WINAPI wine_glGetUniformfvARB( unsigned int programObj, GLint location, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", programObj, location, params );
  funcs->ext.p_glGetUniformfvARB( programObj, location, params );
}

static void WINAPI wine_glGetUniformi64vNV( GLuint program, GLint location, INT64* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", program, location, params );
  funcs->ext.p_glGetUniformi64vNV( program, location, params );
}

static void WINAPI wine_glGetUniformiv( GLuint program, GLint location, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", program, location, params );
  funcs->ext.p_glGetUniformiv( program, location, params );
}

static void WINAPI wine_glGetUniformivARB( unsigned int programObj, GLint location, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", programObj, location, params );
  funcs->ext.p_glGetUniformivARB( programObj, location, params );
}

static void WINAPI wine_glGetUniformui64vNV( GLuint program, GLint location, UINT64* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", program, location, params );
  funcs->ext.p_glGetUniformui64vNV( program, location, params );
}

static void WINAPI wine_glGetUniformuiv( GLuint program, GLint location, GLuint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", program, location, params );
  funcs->ext.p_glGetUniformuiv( program, location, params );
}

static void WINAPI wine_glGetUniformuivEXT( GLuint program, GLint location, GLuint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", program, location, params );
  funcs->ext.p_glGetUniformuivEXT( program, location, params );
}

static void WINAPI wine_glGetVariantArrayObjectfvATI( GLuint id, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", id, pname, params );
  funcs->ext.p_glGetVariantArrayObjectfvATI( id, pname, params );
}

static void WINAPI wine_glGetVariantArrayObjectivATI( GLuint id, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", id, pname, params );
  funcs->ext.p_glGetVariantArrayObjectivATI( id, pname, params );
}

static void WINAPI wine_glGetVariantBooleanvEXT( GLuint id, GLenum value, GLboolean* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", id, value, data );
  funcs->ext.p_glGetVariantBooleanvEXT( id, value, data );
}

static void WINAPI wine_glGetVariantFloatvEXT( GLuint id, GLenum value, GLfloat* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", id, value, data );
  funcs->ext.p_glGetVariantFloatvEXT( id, value, data );
}

static void WINAPI wine_glGetVariantIntegervEXT( GLuint id, GLenum value, GLint* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", id, value, data );
  funcs->ext.p_glGetVariantIntegervEXT( id, value, data );
}

static void WINAPI wine_glGetVariantPointervEXT( GLuint id, GLenum value, GLvoid** data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", id, value, data );
  funcs->ext.p_glGetVariantPointervEXT( id, value, data );
}

static GLint WINAPI wine_glGetVaryingLocationNV( GLuint program, const char* name ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", program, name );
  return funcs->ext.p_glGetVaryingLocationNV( program, name );
}

static void WINAPI wine_glGetVertexAttribArrayObjectfvATI( GLuint index, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", index, pname, params );
  funcs->ext.p_glGetVertexAttribArrayObjectfvATI( index, pname, params );
}

static void WINAPI wine_glGetVertexAttribArrayObjectivATI( GLuint index, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", index, pname, params );
  funcs->ext.p_glGetVertexAttribArrayObjectivATI( index, pname, params );
}

static void WINAPI wine_glGetVertexAttribIiv( GLuint index, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", index, pname, params );
  funcs->ext.p_glGetVertexAttribIiv( index, pname, params );
}

static void WINAPI wine_glGetVertexAttribIivEXT( GLuint index, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", index, pname, params );
  funcs->ext.p_glGetVertexAttribIivEXT( index, pname, params );
}

static void WINAPI wine_glGetVertexAttribIuiv( GLuint index, GLenum pname, GLuint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", index, pname, params );
  funcs->ext.p_glGetVertexAttribIuiv( index, pname, params );
}

static void WINAPI wine_glGetVertexAttribIuivEXT( GLuint index, GLenum pname, GLuint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", index, pname, params );
  funcs->ext.p_glGetVertexAttribIuivEXT( index, pname, params );
}

static void WINAPI wine_glGetVertexAttribLdv( GLuint index, GLenum pname, GLdouble* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", index, pname, params );
  funcs->ext.p_glGetVertexAttribLdv( index, pname, params );
}

static void WINAPI wine_glGetVertexAttribLdvEXT( GLuint index, GLenum pname, GLdouble* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", index, pname, params );
  funcs->ext.p_glGetVertexAttribLdvEXT( index, pname, params );
}

static void WINAPI wine_glGetVertexAttribLi64vNV( GLuint index, GLenum pname, INT64* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", index, pname, params );
  funcs->ext.p_glGetVertexAttribLi64vNV( index, pname, params );
}

static void WINAPI wine_glGetVertexAttribLui64vNV( GLuint index, GLenum pname, UINT64* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", index, pname, params );
  funcs->ext.p_glGetVertexAttribLui64vNV( index, pname, params );
}

static void WINAPI wine_glGetVertexAttribPointerv( GLuint index, GLenum pname, GLvoid** pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", index, pname, pointer );
  funcs->ext.p_glGetVertexAttribPointerv( index, pname, pointer );
}

static void WINAPI wine_glGetVertexAttribPointervARB( GLuint index, GLenum pname, GLvoid** pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", index, pname, pointer );
  funcs->ext.p_glGetVertexAttribPointervARB( index, pname, pointer );
}

static void WINAPI wine_glGetVertexAttribPointervNV( GLuint index, GLenum pname, GLvoid** pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", index, pname, pointer );
  funcs->ext.p_glGetVertexAttribPointervNV( index, pname, pointer );
}

static void WINAPI wine_glGetVertexAttribdv( GLuint index, GLenum pname, GLdouble* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", index, pname, params );
  funcs->ext.p_glGetVertexAttribdv( index, pname, params );
}

static void WINAPI wine_glGetVertexAttribdvARB( GLuint index, GLenum pname, GLdouble* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", index, pname, params );
  funcs->ext.p_glGetVertexAttribdvARB( index, pname, params );
}

static void WINAPI wine_glGetVertexAttribdvNV( GLuint index, GLenum pname, GLdouble* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", index, pname, params );
  funcs->ext.p_glGetVertexAttribdvNV( index, pname, params );
}

static void WINAPI wine_glGetVertexAttribfv( GLuint index, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", index, pname, params );
  funcs->ext.p_glGetVertexAttribfv( index, pname, params );
}

static void WINAPI wine_glGetVertexAttribfvARB( GLuint index, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", index, pname, params );
  funcs->ext.p_glGetVertexAttribfvARB( index, pname, params );
}

static void WINAPI wine_glGetVertexAttribfvNV( GLuint index, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", index, pname, params );
  funcs->ext.p_glGetVertexAttribfvNV( index, pname, params );
}

static void WINAPI wine_glGetVertexAttribiv( GLuint index, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", index, pname, params );
  funcs->ext.p_glGetVertexAttribiv( index, pname, params );
}

static void WINAPI wine_glGetVertexAttribivARB( GLuint index, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", index, pname, params );
  funcs->ext.p_glGetVertexAttribivARB( index, pname, params );
}

static void WINAPI wine_glGetVertexAttribivNV( GLuint index, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", index, pname, params );
  funcs->ext.p_glGetVertexAttribivNV( index, pname, params );
}

static void WINAPI wine_glGetVideoCaptureStreamdvNV( GLuint video_capture_slot, GLuint stream, GLenum pname, GLdouble* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", video_capture_slot, stream, pname, params );
  funcs->ext.p_glGetVideoCaptureStreamdvNV( video_capture_slot, stream, pname, params );
}

static void WINAPI wine_glGetVideoCaptureStreamfvNV( GLuint video_capture_slot, GLuint stream, GLenum pname, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", video_capture_slot, stream, pname, params );
  funcs->ext.p_glGetVideoCaptureStreamfvNV( video_capture_slot, stream, pname, params );
}

static void WINAPI wine_glGetVideoCaptureStreamivNV( GLuint video_capture_slot, GLuint stream, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", video_capture_slot, stream, pname, params );
  funcs->ext.p_glGetVideoCaptureStreamivNV( video_capture_slot, stream, pname, params );
}

static void WINAPI wine_glGetVideoCaptureivNV( GLuint video_capture_slot, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", video_capture_slot, pname, params );
  funcs->ext.p_glGetVideoCaptureivNV( video_capture_slot, pname, params );
}

static void WINAPI wine_glGetVideoi64vNV( GLuint video_slot, GLenum pname, INT64* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", video_slot, pname, params );
  funcs->ext.p_glGetVideoi64vNV( video_slot, pname, params );
}

static void WINAPI wine_glGetVideoivNV( GLuint video_slot, GLenum pname, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", video_slot, pname, params );
  funcs->ext.p_glGetVideoivNV( video_slot, pname, params );
}

static void WINAPI wine_glGetVideoui64vNV( GLuint video_slot, GLenum pname, UINT64* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", video_slot, pname, params );
  funcs->ext.p_glGetVideoui64vNV( video_slot, pname, params );
}

static void WINAPI wine_glGetVideouivNV( GLuint video_slot, GLenum pname, GLuint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", video_slot, pname, params );
  funcs->ext.p_glGetVideouivNV( video_slot, pname, params );
}

static void WINAPI wine_glGetnColorTableARB( GLenum target, GLenum format, GLenum type, GLsizei bufSize, GLvoid* table ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", target, format, type, bufSize, table );
  funcs->ext.p_glGetnColorTableARB( target, format, type, bufSize, table );
}

static void WINAPI wine_glGetnCompressedTexImageARB( GLenum target, GLint lod, GLsizei bufSize, GLvoid* img ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", target, lod, bufSize, img );
  funcs->ext.p_glGetnCompressedTexImageARB( target, lod, bufSize, img );
}

static void WINAPI wine_glGetnConvolutionFilterARB( GLenum target, GLenum format, GLenum type, GLsizei bufSize, GLvoid* image ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", target, format, type, bufSize, image );
  funcs->ext.p_glGetnConvolutionFilterARB( target, format, type, bufSize, image );
}

static void WINAPI wine_glGetnHistogramARB( GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, GLvoid* values ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %p)\n", target, reset, format, type, bufSize, values );
  funcs->ext.p_glGetnHistogramARB( target, reset, format, type, bufSize, values );
}

static void WINAPI wine_glGetnMapdvARB( GLenum target, GLenum query, GLsizei bufSize, GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", target, query, bufSize, v );
  funcs->ext.p_glGetnMapdvARB( target, query, bufSize, v );
}

static void WINAPI wine_glGetnMapfvARB( GLenum target, GLenum query, GLsizei bufSize, GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", target, query, bufSize, v );
  funcs->ext.p_glGetnMapfvARB( target, query, bufSize, v );
}

static void WINAPI wine_glGetnMapivARB( GLenum target, GLenum query, GLsizei bufSize, GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", target, query, bufSize, v );
  funcs->ext.p_glGetnMapivARB( target, query, bufSize, v );
}

static void WINAPI wine_glGetnMinmaxARB( GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, GLvoid* values ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %p)\n", target, reset, format, type, bufSize, values );
  funcs->ext.p_glGetnMinmaxARB( target, reset, format, type, bufSize, values );
}

static void WINAPI wine_glGetnPixelMapfvARB( GLenum map, GLsizei bufSize, GLfloat* values ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", map, bufSize, values );
  funcs->ext.p_glGetnPixelMapfvARB( map, bufSize, values );
}

static void WINAPI wine_glGetnPixelMapuivARB( GLenum map, GLsizei bufSize, GLuint* values ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", map, bufSize, values );
  funcs->ext.p_glGetnPixelMapuivARB( map, bufSize, values );
}

static void WINAPI wine_glGetnPixelMapusvARB( GLenum map, GLsizei bufSize, GLushort* values ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", map, bufSize, values );
  funcs->ext.p_glGetnPixelMapusvARB( map, bufSize, values );
}

static void WINAPI wine_glGetnPolygonStippleARB( GLsizei bufSize, GLubyte* pattern ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", bufSize, pattern );
  funcs->ext.p_glGetnPolygonStippleARB( bufSize, pattern );
}

static void WINAPI wine_glGetnSeparableFilterARB( GLenum target, GLenum format, GLenum type, GLsizei rowBufSize, GLvoid* row, GLsizei columnBufSize, GLvoid* column, GLvoid* span ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p, %d, %p, %p)\n", target, format, type, rowBufSize, row, columnBufSize, column, span );
  funcs->ext.p_glGetnSeparableFilterARB( target, format, type, rowBufSize, row, columnBufSize, column, span );
}

static void WINAPI wine_glGetnTexImageARB( GLenum target, GLint level, GLenum format, GLenum type, GLsizei bufSize, GLvoid* img ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %p)\n", target, level, format, type, bufSize, img );
  funcs->ext.p_glGetnTexImageARB( target, level, format, type, bufSize, img );
}

static void WINAPI wine_glGetnUniformdvARB( GLuint program, GLint location, GLsizei bufSize, GLdouble* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, bufSize, params );
  funcs->ext.p_glGetnUniformdvARB( program, location, bufSize, params );
}

static void WINAPI wine_glGetnUniformfvARB( GLuint program, GLint location, GLsizei bufSize, GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, bufSize, params );
  funcs->ext.p_glGetnUniformfvARB( program, location, bufSize, params );
}

static void WINAPI wine_glGetnUniformivARB( GLuint program, GLint location, GLsizei bufSize, GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, bufSize, params );
  funcs->ext.p_glGetnUniformivARB( program, location, bufSize, params );
}

static void WINAPI wine_glGetnUniformuivARB( GLuint program, GLint location, GLsizei bufSize, GLuint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, bufSize, params );
  funcs->ext.p_glGetnUniformuivARB( program, location, bufSize, params );
}

static void WINAPI wine_glGlobalAlphaFactorbSUN( GLbyte factor ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", factor );
  funcs->ext.p_glGlobalAlphaFactorbSUN( factor );
}

static void WINAPI wine_glGlobalAlphaFactordSUN( GLdouble factor ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f)\n", factor );
  funcs->ext.p_glGlobalAlphaFactordSUN( factor );
}

static void WINAPI wine_glGlobalAlphaFactorfSUN( GLfloat factor ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f)\n", factor );
  funcs->ext.p_glGlobalAlphaFactorfSUN( factor );
}

static void WINAPI wine_glGlobalAlphaFactoriSUN( GLint factor ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", factor );
  funcs->ext.p_glGlobalAlphaFactoriSUN( factor );
}

static void WINAPI wine_glGlobalAlphaFactorsSUN( GLshort factor ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", factor );
  funcs->ext.p_glGlobalAlphaFactorsSUN( factor );
}

static void WINAPI wine_glGlobalAlphaFactorubSUN( GLubyte factor ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", factor );
  funcs->ext.p_glGlobalAlphaFactorubSUN( factor );
}

static void WINAPI wine_glGlobalAlphaFactoruiSUN( GLuint factor ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", factor );
  funcs->ext.p_glGlobalAlphaFactoruiSUN( factor );
}

static void WINAPI wine_glGlobalAlphaFactorusSUN( GLushort factor ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", factor );
  funcs->ext.p_glGlobalAlphaFactorusSUN( factor );
}

static void WINAPI wine_glHintPGI( GLenum target, GLint mode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", target, mode );
  funcs->ext.p_glHintPGI( target, mode );
}

static void WINAPI wine_glHistogram( GLenum target, GLsizei width, GLenum internalformat, GLboolean sink ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", target, width, internalformat, sink );
  funcs->ext.p_glHistogram( target, width, internalformat, sink );
}

static void WINAPI wine_glHistogramEXT( GLenum target, GLsizei width, GLenum internalformat, GLboolean sink ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", target, width, internalformat, sink );
  funcs->ext.p_glHistogramEXT( target, width, internalformat, sink );
}

static void WINAPI wine_glIglooInterfaceSGIX( GLenum pname, const GLvoid* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", pname, params );
  funcs->ext.p_glIglooInterfaceSGIX( pname, params );
}

static void WINAPI wine_glImageTransformParameterfHP( GLenum target, GLenum pname, GLfloat param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f)\n", target, pname, param );
  funcs->ext.p_glImageTransformParameterfHP( target, pname, param );
}

static void WINAPI wine_glImageTransformParameterfvHP( GLenum target, GLenum pname, const GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glImageTransformParameterfvHP( target, pname, params );
}

static void WINAPI wine_glImageTransformParameteriHP( GLenum target, GLenum pname, GLint param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", target, pname, param );
  funcs->ext.p_glImageTransformParameteriHP( target, pname, param );
}

static void WINAPI wine_glImageTransformParameterivHP( GLenum target, GLenum pname, const GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glImageTransformParameterivHP( target, pname, params );
}

static GLvoid* WINAPI wine_glImportSyncEXT( GLenum external_sync_type, INT_PTR external_sync, GLbitfield flags ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %ld, %d)\n", external_sync_type, external_sync, flags );
  return funcs->ext.p_glImportSyncEXT( external_sync_type, external_sync, flags );
}

static void WINAPI wine_glIndexFormatNV( GLenum type, GLsizei stride ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", type, stride );
  funcs->ext.p_glIndexFormatNV( type, stride );
}

static void WINAPI wine_glIndexFuncEXT( GLenum func, GLclampf ref ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f)\n", func, ref );
  funcs->ext.p_glIndexFuncEXT( func, ref );
}

static void WINAPI wine_glIndexMaterialEXT( GLenum face, GLenum mode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", face, mode );
  funcs->ext.p_glIndexMaterialEXT( face, mode );
}

static void WINAPI wine_glIndexPointerEXT( GLenum type, GLsizei stride, GLsizei count, const GLvoid* pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", type, stride, count, pointer );
  funcs->ext.p_glIndexPointerEXT( type, stride, count, pointer );
}

static void WINAPI wine_glIndexPointerListIBM( GLenum type, GLint stride, const GLvoid** pointer, GLint ptrstride ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %d)\n", type, stride, pointer, ptrstride );
  funcs->ext.p_glIndexPointerListIBM( type, stride, pointer, ptrstride );
}

static void WINAPI wine_glInsertComponentEXT( GLuint res, GLuint src, GLuint num ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", res, src, num );
  funcs->ext.p_glInsertComponentEXT( res, src, num );
}

static void WINAPI wine_glInstrumentsBufferSGIX( GLsizei size, GLint* buffer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", size, buffer );
  funcs->ext.p_glInstrumentsBufferSGIX( size, buffer );
}

static void WINAPI wine_glInterpolatePathsNV( GLuint resultPath, GLuint pathA, GLuint pathB, GLfloat weight ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %f)\n", resultPath, pathA, pathB, weight );
  funcs->ext.p_glInterpolatePathsNV( resultPath, pathA, pathB, weight );
}

static GLboolean WINAPI wine_glIsAsyncMarkerSGIX( GLuint marker ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", marker );
  return funcs->ext.p_glIsAsyncMarkerSGIX( marker );
}

static GLboolean WINAPI wine_glIsBuffer( GLuint buffer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", buffer );
  return funcs->ext.p_glIsBuffer( buffer );
}

static GLboolean WINAPI wine_glIsBufferARB( GLuint buffer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", buffer );
  return funcs->ext.p_glIsBufferARB( buffer );
}

static GLboolean WINAPI wine_glIsBufferResidentNV( GLenum target ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", target );
  return funcs->ext.p_glIsBufferResidentNV( target );
}

static GLboolean WINAPI wine_glIsEnabledIndexedEXT( GLenum target, GLuint index ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", target, index );
  return funcs->ext.p_glIsEnabledIndexedEXT( target, index );
}

static GLboolean WINAPI wine_glIsEnabledi( GLenum target, GLuint index ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", target, index );
  return funcs->ext.p_glIsEnabledi( target, index );
}

static GLboolean WINAPI wine_glIsFenceAPPLE( GLuint fence ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", fence );
  return funcs->ext.p_glIsFenceAPPLE( fence );
}

static GLboolean WINAPI wine_glIsFenceNV( GLuint fence ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", fence );
  return funcs->ext.p_glIsFenceNV( fence );
}

static GLboolean WINAPI wine_glIsFramebuffer( GLuint framebuffer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", framebuffer );
  return funcs->ext.p_glIsFramebuffer( framebuffer );
}

static GLboolean WINAPI wine_glIsFramebufferEXT( GLuint framebuffer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", framebuffer );
  return funcs->ext.p_glIsFramebufferEXT( framebuffer );
}

static GLboolean WINAPI wine_glIsImageHandleResidentNV( UINT64 handle ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%s)\n", wine_dbgstr_longlong(handle) );
  return funcs->ext.p_glIsImageHandleResidentNV( handle );
}

static GLboolean WINAPI wine_glIsNameAMD( GLenum identifier, GLuint name ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", identifier, name );
  return funcs->ext.p_glIsNameAMD( identifier, name );
}

static GLboolean WINAPI wine_glIsNamedBufferResidentNV( GLuint buffer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", buffer );
  return funcs->ext.p_glIsNamedBufferResidentNV( buffer );
}

static GLboolean WINAPI wine_glIsNamedStringARB( GLint namelen, const char* name ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", namelen, name );
  return funcs->ext.p_glIsNamedStringARB( namelen, name );
}

static GLboolean WINAPI wine_glIsObjectBufferATI( GLuint buffer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", buffer );
  return funcs->ext.p_glIsObjectBufferATI( buffer );
}

static GLboolean WINAPI wine_glIsOcclusionQueryNV( GLuint id ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", id );
  return funcs->ext.p_glIsOcclusionQueryNV( id );
}

static GLboolean WINAPI wine_glIsPathNV( GLuint path ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", path );
  return funcs->ext.p_glIsPathNV( path );
}

static GLboolean WINAPI wine_glIsPointInFillPathNV( GLuint path, GLuint mask, GLfloat x, GLfloat y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f, %f)\n", path, mask, x, y );
  return funcs->ext.p_glIsPointInFillPathNV( path, mask, x, y );
}

static GLboolean WINAPI wine_glIsPointInStrokePathNV( GLuint path, GLfloat x, GLfloat y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f)\n", path, x, y );
  return funcs->ext.p_glIsPointInStrokePathNV( path, x, y );
}

static GLboolean WINAPI wine_glIsProgram( GLuint program ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", program );
  return funcs->ext.p_glIsProgram( program );
}

static GLboolean WINAPI wine_glIsProgramARB( GLuint program ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", program );
  return funcs->ext.p_glIsProgramARB( program );
}

static GLboolean WINAPI wine_glIsProgramNV( GLuint id ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", id );
  return funcs->ext.p_glIsProgramNV( id );
}

static GLboolean WINAPI wine_glIsProgramPipeline( GLuint pipeline ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", pipeline );
  return funcs->ext.p_glIsProgramPipeline( pipeline );
}

static GLboolean WINAPI wine_glIsQuery( GLuint id ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", id );
  return funcs->ext.p_glIsQuery( id );
}

static GLboolean WINAPI wine_glIsQueryARB( GLuint id ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", id );
  return funcs->ext.p_glIsQueryARB( id );
}

static GLboolean WINAPI wine_glIsRenderbuffer( GLuint renderbuffer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", renderbuffer );
  return funcs->ext.p_glIsRenderbuffer( renderbuffer );
}

static GLboolean WINAPI wine_glIsRenderbufferEXT( GLuint renderbuffer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", renderbuffer );
  return funcs->ext.p_glIsRenderbufferEXT( renderbuffer );
}

static GLboolean WINAPI wine_glIsSampler( GLuint sampler ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", sampler );
  return funcs->ext.p_glIsSampler( sampler );
}

static GLboolean WINAPI wine_glIsShader( GLuint shader ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", shader );
  return funcs->ext.p_glIsShader( shader );
}

static GLboolean WINAPI wine_glIsSync( GLvoid* sync ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", sync );
  return funcs->ext.p_glIsSync( sync );
}

static GLboolean WINAPI wine_glIsTextureEXT( GLuint texture ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", texture );
  return funcs->ext.p_glIsTextureEXT( texture );
}

static GLboolean WINAPI wine_glIsTextureHandleResidentNV( UINT64 handle ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%s)\n", wine_dbgstr_longlong(handle) );
  return funcs->ext.p_glIsTextureHandleResidentNV( handle );
}

static GLboolean WINAPI wine_glIsTransformFeedback( GLuint id ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", id );
  return funcs->ext.p_glIsTransformFeedback( id );
}

static GLboolean WINAPI wine_glIsTransformFeedbackNV( GLuint id ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", id );
  return funcs->ext.p_glIsTransformFeedbackNV( id );
}

static GLboolean WINAPI wine_glIsVariantEnabledEXT( GLuint id, GLenum cap ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", id, cap );
  return funcs->ext.p_glIsVariantEnabledEXT( id, cap );
}

static GLboolean WINAPI wine_glIsVertexArray( GLuint array ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", array );
  return funcs->ext.p_glIsVertexArray( array );
}

static GLboolean WINAPI wine_glIsVertexArrayAPPLE( GLuint array ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", array );
  return funcs->ext.p_glIsVertexArrayAPPLE( array );
}

static GLboolean WINAPI wine_glIsVertexAttribEnabledAPPLE( GLuint index, GLenum pname ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", index, pname );
  return funcs->ext.p_glIsVertexAttribEnabledAPPLE( index, pname );
}

static void WINAPI wine_glLightEnviSGIX( GLenum pname, GLint param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", pname, param );
  funcs->ext.p_glLightEnviSGIX( pname, param );
}

static void WINAPI wine_glLinkProgram( GLuint program ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", program );
  funcs->ext.p_glLinkProgram( program );
}

static void WINAPI wine_glLinkProgramARB( unsigned int programObj ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", programObj );
  funcs->ext.p_glLinkProgramARB( programObj );
}

static void WINAPI wine_glListParameterfSGIX( GLuint list, GLenum pname, GLfloat param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f)\n", list, pname, param );
  funcs->ext.p_glListParameterfSGIX( list, pname, param );
}

static void WINAPI wine_glListParameterfvSGIX( GLuint list, GLenum pname, const GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", list, pname, params );
  funcs->ext.p_glListParameterfvSGIX( list, pname, params );
}

static void WINAPI wine_glListParameteriSGIX( GLuint list, GLenum pname, GLint param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", list, pname, param );
  funcs->ext.p_glListParameteriSGIX( list, pname, param );
}

static void WINAPI wine_glListParameterivSGIX( GLuint list, GLenum pname, const GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", list, pname, params );
  funcs->ext.p_glListParameterivSGIX( list, pname, params );
}

static void WINAPI wine_glLoadIdentityDeformationMapSGIX( GLbitfield mask ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", mask );
  funcs->ext.p_glLoadIdentityDeformationMapSGIX( mask );
}

static void WINAPI wine_glLoadProgramNV( GLenum target, GLuint id, GLsizei len, const GLubyte* program ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", target, id, len, program );
  funcs->ext.p_glLoadProgramNV( target, id, len, program );
}

static void WINAPI wine_glLoadTransposeMatrixd( const GLdouble* m ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", m );
  funcs->ext.p_glLoadTransposeMatrixd( m );
}

static void WINAPI wine_glLoadTransposeMatrixdARB( const GLdouble* m ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", m );
  funcs->ext.p_glLoadTransposeMatrixdARB( m );
}

static void WINAPI wine_glLoadTransposeMatrixf( const GLfloat* m ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", m );
  funcs->ext.p_glLoadTransposeMatrixf( m );
}

static void WINAPI wine_glLoadTransposeMatrixfARB( const GLfloat* m ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", m );
  funcs->ext.p_glLoadTransposeMatrixfARB( m );
}

static void WINAPI wine_glLockArraysEXT( GLint first, GLsizei count ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", first, count );
  funcs->ext.p_glLockArraysEXT( first, count );
}

static void WINAPI wine_glMTexCoord2fSGIS( GLenum target, GLfloat s, GLfloat t ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f)\n", target, s, t );
  funcs->ext.p_glMTexCoord2fSGIS( target, s, t );
}

static void WINAPI wine_glMTexCoord2fvSGIS( GLenum target, GLfloat * v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMTexCoord2fvSGIS( target, v );
}

static void WINAPI wine_glMakeBufferNonResidentNV( GLenum target ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", target );
  funcs->ext.p_glMakeBufferNonResidentNV( target );
}

static void WINAPI wine_glMakeBufferResidentNV( GLenum target, GLenum access ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", target, access );
  funcs->ext.p_glMakeBufferResidentNV( target, access );
}

static void WINAPI wine_glMakeImageHandleNonResidentNV( UINT64 handle ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%s)\n", wine_dbgstr_longlong(handle) );
  funcs->ext.p_glMakeImageHandleNonResidentNV( handle );
}

static void WINAPI wine_glMakeImageHandleResidentNV( UINT64 handle, GLenum access ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%s, %d)\n", wine_dbgstr_longlong(handle), access );
  funcs->ext.p_glMakeImageHandleResidentNV( handle, access );
}

static void WINAPI wine_glMakeNamedBufferNonResidentNV( GLuint buffer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", buffer );
  funcs->ext.p_glMakeNamedBufferNonResidentNV( buffer );
}

static void WINAPI wine_glMakeNamedBufferResidentNV( GLuint buffer, GLenum access ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", buffer, access );
  funcs->ext.p_glMakeNamedBufferResidentNV( buffer, access );
}

static void WINAPI wine_glMakeTextureHandleNonResidentNV( UINT64 handle ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%s)\n", wine_dbgstr_longlong(handle) );
  funcs->ext.p_glMakeTextureHandleNonResidentNV( handle );
}

static void WINAPI wine_glMakeTextureHandleResidentNV( UINT64 handle ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%s)\n", wine_dbgstr_longlong(handle) );
  funcs->ext.p_glMakeTextureHandleResidentNV( handle );
}

static GLvoid* WINAPI wine_glMapBuffer( GLenum target, GLenum access ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", target, access );
  return funcs->ext.p_glMapBuffer( target, access );
}

static GLvoid* WINAPI wine_glMapBufferARB( GLenum target, GLenum access ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", target, access );
  return funcs->ext.p_glMapBufferARB( target, access );
}

static GLvoid* WINAPI wine_glMapBufferRange( GLenum target, INT_PTR offset, INT_PTR length, GLbitfield access ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %ld, %ld, %d)\n", target, offset, length, access );
  return funcs->ext.p_glMapBufferRange( target, offset, length, access );
}

static void WINAPI wine_glMapControlPointsNV( GLenum target, GLuint index, GLenum type, GLsizei ustride, GLsizei vstride, GLint uorder, GLint vorder, GLboolean packed, const GLvoid* points ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, index, type, ustride, vstride, uorder, vorder, packed, points );
  funcs->ext.p_glMapControlPointsNV( target, index, type, ustride, vstride, uorder, vorder, packed, points );
}

static GLvoid* WINAPI wine_glMapNamedBufferEXT( GLuint buffer, GLenum access ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", buffer, access );
  return funcs->ext.p_glMapNamedBufferEXT( buffer, access );
}

static GLvoid* WINAPI wine_glMapNamedBufferRangeEXT( GLuint buffer, INT_PTR offset, INT_PTR length, GLbitfield access ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %ld, %ld, %d)\n", buffer, offset, length, access );
  return funcs->ext.p_glMapNamedBufferRangeEXT( buffer, offset, length, access );
}

static GLvoid* WINAPI wine_glMapObjectBufferATI( GLuint buffer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", buffer );
  return funcs->ext.p_glMapObjectBufferATI( buffer );
}

static void WINAPI wine_glMapParameterfvNV( GLenum target, GLenum pname, const GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glMapParameterfvNV( target, pname, params );
}

static void WINAPI wine_glMapParameterivNV( GLenum target, GLenum pname, const GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glMapParameterivNV( target, pname, params );
}

static void WINAPI wine_glMapVertexAttrib1dAPPLE( GLuint index, GLuint size, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble* points ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f, %f, %d, %d, %p)\n", index, size, u1, u2, stride, order, points );
  funcs->ext.p_glMapVertexAttrib1dAPPLE( index, size, u1, u2, stride, order, points );
}

static void WINAPI wine_glMapVertexAttrib1fAPPLE( GLuint index, GLuint size, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat* points ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f, %f, %d, %d, %p)\n", index, size, u1, u2, stride, order, points );
  funcs->ext.p_glMapVertexAttrib1fAPPLE( index, size, u1, u2, stride, order, points );
}

static void WINAPI wine_glMapVertexAttrib2dAPPLE( GLuint index, GLuint size, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble* points ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f, %f, %d, %d, %f, %f, %d, %d, %p)\n", index, size, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points );
  funcs->ext.p_glMapVertexAttrib2dAPPLE( index, size, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points );
}

static void WINAPI wine_glMapVertexAttrib2fAPPLE( GLuint index, GLuint size, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat* points ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f, %f, %d, %d, %f, %f, %d, %d, %p)\n", index, size, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points );
  funcs->ext.p_glMapVertexAttrib2fAPPLE( index, size, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points );
}

static void WINAPI wine_glMatrixFrustumEXT( GLenum mode, GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f, %f, %f, %f)\n", mode, left, right, bottom, top, zNear, zFar );
  funcs->ext.p_glMatrixFrustumEXT( mode, left, right, bottom, top, zNear, zFar );
}

static void WINAPI wine_glMatrixIndexPointerARB( GLint size, GLenum type, GLsizei stride, const GLvoid* pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", size, type, stride, pointer );
  funcs->ext.p_glMatrixIndexPointerARB( size, type, stride, pointer );
}

static void WINAPI wine_glMatrixIndexubvARB( GLint size, const GLubyte* indices ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", size, indices );
  funcs->ext.p_glMatrixIndexubvARB( size, indices );
}

static void WINAPI wine_glMatrixIndexuivARB( GLint size, const GLuint* indices ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", size, indices );
  funcs->ext.p_glMatrixIndexuivARB( size, indices );
}

static void WINAPI wine_glMatrixIndexusvARB( GLint size, const GLushort* indices ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", size, indices );
  funcs->ext.p_glMatrixIndexusvARB( size, indices );
}

static void WINAPI wine_glMatrixLoadIdentityEXT( GLenum mode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", mode );
  funcs->ext.p_glMatrixLoadIdentityEXT( mode );
}

static void WINAPI wine_glMatrixLoadTransposedEXT( GLenum mode, const GLdouble* m ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", mode, m );
  funcs->ext.p_glMatrixLoadTransposedEXT( mode, m );
}

static void WINAPI wine_glMatrixLoadTransposefEXT( GLenum mode, const GLfloat* m ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", mode, m );
  funcs->ext.p_glMatrixLoadTransposefEXT( mode, m );
}

static void WINAPI wine_glMatrixLoaddEXT( GLenum mode, const GLdouble* m ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", mode, m );
  funcs->ext.p_glMatrixLoaddEXT( mode, m );
}

static void WINAPI wine_glMatrixLoadfEXT( GLenum mode, const GLfloat* m ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", mode, m );
  funcs->ext.p_glMatrixLoadfEXT( mode, m );
}

static void WINAPI wine_glMatrixMultTransposedEXT( GLenum mode, const GLdouble* m ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", mode, m );
  funcs->ext.p_glMatrixMultTransposedEXT( mode, m );
}

static void WINAPI wine_glMatrixMultTransposefEXT( GLenum mode, const GLfloat* m ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", mode, m );
  funcs->ext.p_glMatrixMultTransposefEXT( mode, m );
}

static void WINAPI wine_glMatrixMultdEXT( GLenum mode, const GLdouble* m ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", mode, m );
  funcs->ext.p_glMatrixMultdEXT( mode, m );
}

static void WINAPI wine_glMatrixMultfEXT( GLenum mode, const GLfloat* m ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", mode, m );
  funcs->ext.p_glMatrixMultfEXT( mode, m );
}

static void WINAPI wine_glMatrixOrthoEXT( GLenum mode, GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f, %f, %f, %f)\n", mode, left, right, bottom, top, zNear, zFar );
  funcs->ext.p_glMatrixOrthoEXT( mode, left, right, bottom, top, zNear, zFar );
}

static void WINAPI wine_glMatrixPopEXT( GLenum mode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", mode );
  funcs->ext.p_glMatrixPopEXT( mode );
}

static void WINAPI wine_glMatrixPushEXT( GLenum mode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", mode );
  funcs->ext.p_glMatrixPushEXT( mode );
}

static void WINAPI wine_glMatrixRotatedEXT( GLenum mode, GLdouble angle, GLdouble x, GLdouble y, GLdouble z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f, %f)\n", mode, angle, x, y, z );
  funcs->ext.p_glMatrixRotatedEXT( mode, angle, x, y, z );
}

static void WINAPI wine_glMatrixRotatefEXT( GLenum mode, GLfloat angle, GLfloat x, GLfloat y, GLfloat z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f, %f)\n", mode, angle, x, y, z );
  funcs->ext.p_glMatrixRotatefEXT( mode, angle, x, y, z );
}

static void WINAPI wine_glMatrixScaledEXT( GLenum mode, GLdouble x, GLdouble y, GLdouble z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f)\n", mode, x, y, z );
  funcs->ext.p_glMatrixScaledEXT( mode, x, y, z );
}

static void WINAPI wine_glMatrixScalefEXT( GLenum mode, GLfloat x, GLfloat y, GLfloat z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f)\n", mode, x, y, z );
  funcs->ext.p_glMatrixScalefEXT( mode, x, y, z );
}

static void WINAPI wine_glMatrixTranslatedEXT( GLenum mode, GLdouble x, GLdouble y, GLdouble z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f)\n", mode, x, y, z );
  funcs->ext.p_glMatrixTranslatedEXT( mode, x, y, z );
}

static void WINAPI wine_glMatrixTranslatefEXT( GLenum mode, GLfloat x, GLfloat y, GLfloat z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f)\n", mode, x, y, z );
  funcs->ext.p_glMatrixTranslatefEXT( mode, x, y, z );
}

static void WINAPI wine_glMemoryBarrier( GLbitfield barriers ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", barriers );
  funcs->ext.p_glMemoryBarrier( barriers );
}

static void WINAPI wine_glMemoryBarrierEXT( GLbitfield barriers ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", barriers );
  funcs->ext.p_glMemoryBarrierEXT( barriers );
}

static void WINAPI wine_glMinSampleShading( GLfloat value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f)\n", value );
  funcs->ext.p_glMinSampleShading( value );
}

static void WINAPI wine_glMinSampleShadingARB( GLfloat value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f)\n", value );
  funcs->ext.p_glMinSampleShadingARB( value );
}

static void WINAPI wine_glMinmax( GLenum target, GLenum internalformat, GLboolean sink ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", target, internalformat, sink );
  funcs->ext.p_glMinmax( target, internalformat, sink );
}

static void WINAPI wine_glMinmaxEXT( GLenum target, GLenum internalformat, GLboolean sink ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", target, internalformat, sink );
  funcs->ext.p_glMinmaxEXT( target, internalformat, sink );
}

static void WINAPI wine_glMultTransposeMatrixd( const GLdouble* m ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", m );
  funcs->ext.p_glMultTransposeMatrixd( m );
}

static void WINAPI wine_glMultTransposeMatrixdARB( const GLdouble* m ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", m );
  funcs->ext.p_glMultTransposeMatrixdARB( m );
}

static void WINAPI wine_glMultTransposeMatrixf( const GLfloat* m ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", m );
  funcs->ext.p_glMultTransposeMatrixf( m );
}

static void WINAPI wine_glMultTransposeMatrixfARB( const GLfloat* m ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", m );
  funcs->ext.p_glMultTransposeMatrixfARB( m );
}

static void WINAPI wine_glMultiDrawArrays( GLenum mode, const GLint* first, const GLsizei* count, GLsizei primcount ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p, %p, %d)\n", mode, first, count, primcount );
  funcs->ext.p_glMultiDrawArrays( mode, first, count, primcount );
}

static void WINAPI wine_glMultiDrawArraysEXT( GLenum mode, const GLint* first, const GLsizei* count, GLsizei primcount ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p, %p, %d)\n", mode, first, count, primcount );
  funcs->ext.p_glMultiDrawArraysEXT( mode, first, count, primcount );
}

static void WINAPI wine_glMultiDrawArraysIndirectAMD( GLenum mode, const GLvoid* indirect, GLsizei primcount, GLsizei stride ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p, %d, %d)\n", mode, indirect, primcount, stride );
  funcs->ext.p_glMultiDrawArraysIndirectAMD( mode, indirect, primcount, stride );
}

static void WINAPI wine_glMultiDrawElementArrayAPPLE( GLenum mode, const GLint* first, const GLsizei* count, GLsizei primcount ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p, %p, %d)\n", mode, first, count, primcount );
  funcs->ext.p_glMultiDrawElementArrayAPPLE( mode, first, count, primcount );
}

static void WINAPI wine_glMultiDrawElements( GLenum mode, const GLsizei* count, GLenum type, const GLvoid* const* indices, GLsizei primcount ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p, %d, %p, %d)\n", mode, count, type, indices, primcount );
  funcs->ext.p_glMultiDrawElements( mode, count, type, indices, primcount );
}

static void WINAPI wine_glMultiDrawElementsBaseVertex( GLenum mode, const GLsizei* count, GLenum type, const GLvoid* const* indices, GLsizei primcount, const GLint* basevertex ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p, %d, %p, %d, %p)\n", mode, count, type, indices, primcount, basevertex );
  funcs->ext.p_glMultiDrawElementsBaseVertex( mode, count, type, indices, primcount, basevertex );
}

static void WINAPI wine_glMultiDrawElementsEXT( GLenum mode, const GLsizei* count, GLenum type, const GLvoid** indices, GLsizei primcount ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p, %d, %p, %d)\n", mode, count, type, indices, primcount );
  funcs->ext.p_glMultiDrawElementsEXT( mode, count, type, indices, primcount );
}

static void WINAPI wine_glMultiDrawElementsIndirectAMD( GLenum mode, GLenum type, const GLvoid* indirect, GLsizei primcount, GLsizei stride ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %d, %d)\n", mode, type, indirect, primcount, stride );
  funcs->ext.p_glMultiDrawElementsIndirectAMD( mode, type, indirect, primcount, stride );
}

static void WINAPI wine_glMultiDrawRangeElementArrayAPPLE( GLenum mode, GLuint start, GLuint end, const GLint* first, const GLsizei* count, GLsizei primcount ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p, %p, %d)\n", mode, start, end, first, count, primcount );
  funcs->ext.p_glMultiDrawRangeElementArrayAPPLE( mode, start, end, first, count, primcount );
}

static void WINAPI wine_glMultiModeDrawArraysIBM( const GLenum* mode, const GLint* first, const GLsizei* count, GLsizei primcount, GLint modestride ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p, %p, %p, %d, %d)\n", mode, first, count, primcount, modestride );
  funcs->ext.p_glMultiModeDrawArraysIBM( mode, first, count, primcount, modestride );
}

static void WINAPI wine_glMultiModeDrawElementsIBM( const GLenum* mode, const GLsizei* count, GLenum type, const GLvoid* const* indices, GLsizei primcount, GLint modestride ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p, %p, %d, %p, %d, %d)\n", mode, count, type, indices, primcount, modestride );
  funcs->ext.p_glMultiModeDrawElementsIBM( mode, count, type, indices, primcount, modestride );
}

static void WINAPI wine_glMultiTexBufferEXT( GLenum texunit, GLenum target, GLenum internalformat, GLuint buffer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", texunit, target, internalformat, buffer );
  funcs->ext.p_glMultiTexBufferEXT( texunit, target, internalformat, buffer );
}

static void WINAPI wine_glMultiTexCoord1d( GLenum target, GLdouble s ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f)\n", target, s );
  funcs->ext.p_glMultiTexCoord1d( target, s );
}

static void WINAPI wine_glMultiTexCoord1dARB( GLenum target, GLdouble s ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f)\n", target, s );
  funcs->ext.p_glMultiTexCoord1dARB( target, s );
}

static void WINAPI wine_glMultiTexCoord1dSGIS( GLenum target, GLdouble s ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f)\n", target, s );
  funcs->ext.p_glMultiTexCoord1dSGIS( target, s );
}

static void WINAPI wine_glMultiTexCoord1dv( GLenum target, const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord1dv( target, v );
}

static void WINAPI wine_glMultiTexCoord1dvARB( GLenum target, const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord1dvARB( target, v );
}

static void WINAPI wine_glMultiTexCoord1dvSGIS( GLenum target, GLdouble * v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord1dvSGIS( target, v );
}

static void WINAPI wine_glMultiTexCoord1f( GLenum target, GLfloat s ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f)\n", target, s );
  funcs->ext.p_glMultiTexCoord1f( target, s );
}

static void WINAPI wine_glMultiTexCoord1fARB( GLenum target, GLfloat s ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f)\n", target, s );
  funcs->ext.p_glMultiTexCoord1fARB( target, s );
}

static void WINAPI wine_glMultiTexCoord1fSGIS( GLenum target, GLfloat s ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f)\n", target, s );
  funcs->ext.p_glMultiTexCoord1fSGIS( target, s );
}

static void WINAPI wine_glMultiTexCoord1fv( GLenum target, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord1fv( target, v );
}

static void WINAPI wine_glMultiTexCoord1fvARB( GLenum target, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord1fvARB( target, v );
}

static void WINAPI wine_glMultiTexCoord1fvSGIS( GLenum target, const GLfloat * v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord1fvSGIS( target, v );
}

static void WINAPI wine_glMultiTexCoord1hNV( GLenum target, unsigned short s ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", target, s );
  funcs->ext.p_glMultiTexCoord1hNV( target, s );
}

static void WINAPI wine_glMultiTexCoord1hvNV( GLenum target, const unsigned short* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord1hvNV( target, v );
}

static void WINAPI wine_glMultiTexCoord1i( GLenum target, GLint s ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", target, s );
  funcs->ext.p_glMultiTexCoord1i( target, s );
}

static void WINAPI wine_glMultiTexCoord1iARB( GLenum target, GLint s ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", target, s );
  funcs->ext.p_glMultiTexCoord1iARB( target, s );
}

static void WINAPI wine_glMultiTexCoord1iSGIS( GLenum target, GLint s ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", target, s );
  funcs->ext.p_glMultiTexCoord1iSGIS( target, s );
}

static void WINAPI wine_glMultiTexCoord1iv( GLenum target, const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord1iv( target, v );
}

static void WINAPI wine_glMultiTexCoord1ivARB( GLenum target, const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord1ivARB( target, v );
}

static void WINAPI wine_glMultiTexCoord1ivSGIS( GLenum target, GLint * v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord1ivSGIS( target, v );
}

static void WINAPI wine_glMultiTexCoord1s( GLenum target, GLshort s ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", target, s );
  funcs->ext.p_glMultiTexCoord1s( target, s );
}

static void WINAPI wine_glMultiTexCoord1sARB( GLenum target, GLshort s ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", target, s );
  funcs->ext.p_glMultiTexCoord1sARB( target, s );
}

static void WINAPI wine_glMultiTexCoord1sSGIS( GLenum target, GLshort s ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", target, s );
  funcs->ext.p_glMultiTexCoord1sSGIS( target, s );
}

static void WINAPI wine_glMultiTexCoord1sv( GLenum target, const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord1sv( target, v );
}

static void WINAPI wine_glMultiTexCoord1svARB( GLenum target, const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord1svARB( target, v );
}

static void WINAPI wine_glMultiTexCoord1svSGIS( GLenum target, GLshort * v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord1svSGIS( target, v );
}

static void WINAPI wine_glMultiTexCoord2d( GLenum target, GLdouble s, GLdouble t ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f)\n", target, s, t );
  funcs->ext.p_glMultiTexCoord2d( target, s, t );
}

static void WINAPI wine_glMultiTexCoord2dARB( GLenum target, GLdouble s, GLdouble t ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f)\n", target, s, t );
  funcs->ext.p_glMultiTexCoord2dARB( target, s, t );
}

static void WINAPI wine_glMultiTexCoord2dSGIS( GLenum target, GLdouble s, GLdouble t ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f)\n", target, s, t );
  funcs->ext.p_glMultiTexCoord2dSGIS( target, s, t );
}

static void WINAPI wine_glMultiTexCoord2dv( GLenum target, const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord2dv( target, v );
}

static void WINAPI wine_glMultiTexCoord2dvARB( GLenum target, const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord2dvARB( target, v );
}

static void WINAPI wine_glMultiTexCoord2dvSGIS( GLenum target, GLdouble * v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord2dvSGIS( target, v );
}

static void WINAPI wine_glMultiTexCoord2f( GLenum target, GLfloat s, GLfloat t ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f)\n", target, s, t );
  funcs->ext.p_glMultiTexCoord2f( target, s, t );
}

static void WINAPI wine_glMultiTexCoord2fARB( GLenum target, GLfloat s, GLfloat t ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f)\n", target, s, t );
  funcs->ext.p_glMultiTexCoord2fARB( target, s, t );
}

static void WINAPI wine_glMultiTexCoord2fSGIS( GLenum target, GLfloat s, GLfloat t ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f)\n", target, s, t );
  funcs->ext.p_glMultiTexCoord2fSGIS( target, s, t );
}

static void WINAPI wine_glMultiTexCoord2fv( GLenum target, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord2fv( target, v );
}

static void WINAPI wine_glMultiTexCoord2fvARB( GLenum target, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord2fvARB( target, v );
}

static void WINAPI wine_glMultiTexCoord2fvSGIS( GLenum target, GLfloat * v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord2fvSGIS( target, v );
}

static void WINAPI wine_glMultiTexCoord2hNV( GLenum target, unsigned short s, unsigned short t ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", target, s, t );
  funcs->ext.p_glMultiTexCoord2hNV( target, s, t );
}

static void WINAPI wine_glMultiTexCoord2hvNV( GLenum target, const unsigned short* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord2hvNV( target, v );
}

static void WINAPI wine_glMultiTexCoord2i( GLenum target, GLint s, GLint t ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", target, s, t );
  funcs->ext.p_glMultiTexCoord2i( target, s, t );
}

static void WINAPI wine_glMultiTexCoord2iARB( GLenum target, GLint s, GLint t ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", target, s, t );
  funcs->ext.p_glMultiTexCoord2iARB( target, s, t );
}

static void WINAPI wine_glMultiTexCoord2iSGIS( GLenum target, GLint s, GLint t ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", target, s, t );
  funcs->ext.p_glMultiTexCoord2iSGIS( target, s, t );
}

static void WINAPI wine_glMultiTexCoord2iv( GLenum target, const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord2iv( target, v );
}

static void WINAPI wine_glMultiTexCoord2ivARB( GLenum target, const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord2ivARB( target, v );
}

static void WINAPI wine_glMultiTexCoord2ivSGIS( GLenum target, GLint * v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord2ivSGIS( target, v );
}

static void WINAPI wine_glMultiTexCoord2s( GLenum target, GLshort s, GLshort t ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", target, s, t );
  funcs->ext.p_glMultiTexCoord2s( target, s, t );
}

static void WINAPI wine_glMultiTexCoord2sARB( GLenum target, GLshort s, GLshort t ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", target, s, t );
  funcs->ext.p_glMultiTexCoord2sARB( target, s, t );
}

static void WINAPI wine_glMultiTexCoord2sSGIS( GLenum target, GLshort s, GLshort t ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", target, s, t );
  funcs->ext.p_glMultiTexCoord2sSGIS( target, s, t );
}

static void WINAPI wine_glMultiTexCoord2sv( GLenum target, const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord2sv( target, v );
}

static void WINAPI wine_glMultiTexCoord2svARB( GLenum target, const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord2svARB( target, v );
}

static void WINAPI wine_glMultiTexCoord2svSGIS( GLenum target, GLshort * v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord2svSGIS( target, v );
}

static void WINAPI wine_glMultiTexCoord3d( GLenum target, GLdouble s, GLdouble t, GLdouble r ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f)\n", target, s, t, r );
  funcs->ext.p_glMultiTexCoord3d( target, s, t, r );
}

static void WINAPI wine_glMultiTexCoord3dARB( GLenum target, GLdouble s, GLdouble t, GLdouble r ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f)\n", target, s, t, r );
  funcs->ext.p_glMultiTexCoord3dARB( target, s, t, r );
}

static void WINAPI wine_glMultiTexCoord3dSGIS( GLenum target, GLdouble s, GLdouble t, GLdouble r ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f)\n", target, s, t, r );
  funcs->ext.p_glMultiTexCoord3dSGIS( target, s, t, r );
}

static void WINAPI wine_glMultiTexCoord3dv( GLenum target, const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord3dv( target, v );
}

static void WINAPI wine_glMultiTexCoord3dvARB( GLenum target, const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord3dvARB( target, v );
}

static void WINAPI wine_glMultiTexCoord3dvSGIS( GLenum target, GLdouble * v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord3dvSGIS( target, v );
}

static void WINAPI wine_glMultiTexCoord3f( GLenum target, GLfloat s, GLfloat t, GLfloat r ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f)\n", target, s, t, r );
  funcs->ext.p_glMultiTexCoord3f( target, s, t, r );
}

static void WINAPI wine_glMultiTexCoord3fARB( GLenum target, GLfloat s, GLfloat t, GLfloat r ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f)\n", target, s, t, r );
  funcs->ext.p_glMultiTexCoord3fARB( target, s, t, r );
}

static void WINAPI wine_glMultiTexCoord3fSGIS( GLenum target, GLfloat s, GLfloat t, GLfloat r ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f)\n", target, s, t, r );
  funcs->ext.p_glMultiTexCoord3fSGIS( target, s, t, r );
}

static void WINAPI wine_glMultiTexCoord3fv( GLenum target, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord3fv( target, v );
}

static void WINAPI wine_glMultiTexCoord3fvARB( GLenum target, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord3fvARB( target, v );
}

static void WINAPI wine_glMultiTexCoord3fvSGIS( GLenum target, GLfloat * v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord3fvSGIS( target, v );
}

static void WINAPI wine_glMultiTexCoord3hNV( GLenum target, unsigned short s, unsigned short t, unsigned short r ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", target, s, t, r );
  funcs->ext.p_glMultiTexCoord3hNV( target, s, t, r );
}

static void WINAPI wine_glMultiTexCoord3hvNV( GLenum target, const unsigned short* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord3hvNV( target, v );
}

static void WINAPI wine_glMultiTexCoord3i( GLenum target, GLint s, GLint t, GLint r ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", target, s, t, r );
  funcs->ext.p_glMultiTexCoord3i( target, s, t, r );
}

static void WINAPI wine_glMultiTexCoord3iARB( GLenum target, GLint s, GLint t, GLint r ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", target, s, t, r );
  funcs->ext.p_glMultiTexCoord3iARB( target, s, t, r );
}

static void WINAPI wine_glMultiTexCoord3iSGIS( GLenum target, GLint s, GLint t, GLint r ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", target, s, t, r );
  funcs->ext.p_glMultiTexCoord3iSGIS( target, s, t, r );
}

static void WINAPI wine_glMultiTexCoord3iv( GLenum target, const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord3iv( target, v );
}

static void WINAPI wine_glMultiTexCoord3ivARB( GLenum target, const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord3ivARB( target, v );
}

static void WINAPI wine_glMultiTexCoord3ivSGIS( GLenum target, GLint * v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord3ivSGIS( target, v );
}

static void WINAPI wine_glMultiTexCoord3s( GLenum target, GLshort s, GLshort t, GLshort r ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", target, s, t, r );
  funcs->ext.p_glMultiTexCoord3s( target, s, t, r );
}

static void WINAPI wine_glMultiTexCoord3sARB( GLenum target, GLshort s, GLshort t, GLshort r ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", target, s, t, r );
  funcs->ext.p_glMultiTexCoord3sARB( target, s, t, r );
}

static void WINAPI wine_glMultiTexCoord3sSGIS( GLenum target, GLshort s, GLshort t, GLshort r ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", target, s, t, r );
  funcs->ext.p_glMultiTexCoord3sSGIS( target, s, t, r );
}

static void WINAPI wine_glMultiTexCoord3sv( GLenum target, const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord3sv( target, v );
}

static void WINAPI wine_glMultiTexCoord3svARB( GLenum target, const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord3svARB( target, v );
}

static void WINAPI wine_glMultiTexCoord3svSGIS( GLenum target, GLshort * v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord3svSGIS( target, v );
}

static void WINAPI wine_glMultiTexCoord4d( GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f, %f)\n", target, s, t, r, q );
  funcs->ext.p_glMultiTexCoord4d( target, s, t, r, q );
}

static void WINAPI wine_glMultiTexCoord4dARB( GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f, %f)\n", target, s, t, r, q );
  funcs->ext.p_glMultiTexCoord4dARB( target, s, t, r, q );
}

static void WINAPI wine_glMultiTexCoord4dSGIS( GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f, %f)\n", target, s, t, r, q );
  funcs->ext.p_glMultiTexCoord4dSGIS( target, s, t, r, q );
}

static void WINAPI wine_glMultiTexCoord4dv( GLenum target, const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord4dv( target, v );
}

static void WINAPI wine_glMultiTexCoord4dvARB( GLenum target, const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord4dvARB( target, v );
}

static void WINAPI wine_glMultiTexCoord4dvSGIS( GLenum target, GLdouble * v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord4dvSGIS( target, v );
}

static void WINAPI wine_glMultiTexCoord4f( GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f, %f)\n", target, s, t, r, q );
  funcs->ext.p_glMultiTexCoord4f( target, s, t, r, q );
}

static void WINAPI wine_glMultiTexCoord4fARB( GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f, %f)\n", target, s, t, r, q );
  funcs->ext.p_glMultiTexCoord4fARB( target, s, t, r, q );
}

static void WINAPI wine_glMultiTexCoord4fSGIS( GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f, %f)\n", target, s, t, r, q );
  funcs->ext.p_glMultiTexCoord4fSGIS( target, s, t, r, q );
}

static void WINAPI wine_glMultiTexCoord4fv( GLenum target, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord4fv( target, v );
}

static void WINAPI wine_glMultiTexCoord4fvARB( GLenum target, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord4fvARB( target, v );
}

static void WINAPI wine_glMultiTexCoord4fvSGIS( GLenum target, GLfloat * v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord4fvSGIS( target, v );
}

static void WINAPI wine_glMultiTexCoord4hNV( GLenum target, unsigned short s, unsigned short t, unsigned short r, unsigned short q ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", target, s, t, r, q );
  funcs->ext.p_glMultiTexCoord4hNV( target, s, t, r, q );
}

static void WINAPI wine_glMultiTexCoord4hvNV( GLenum target, const unsigned short* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord4hvNV( target, v );
}

static void WINAPI wine_glMultiTexCoord4i( GLenum target, GLint s, GLint t, GLint r, GLint q ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", target, s, t, r, q );
  funcs->ext.p_glMultiTexCoord4i( target, s, t, r, q );
}

static void WINAPI wine_glMultiTexCoord4iARB( GLenum target, GLint s, GLint t, GLint r, GLint q ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", target, s, t, r, q );
  funcs->ext.p_glMultiTexCoord4iARB( target, s, t, r, q );
}

static void WINAPI wine_glMultiTexCoord4iSGIS( GLenum target, GLint s, GLint t, GLint r, GLint q ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", target, s, t, r, q );
  funcs->ext.p_glMultiTexCoord4iSGIS( target, s, t, r, q );
}

static void WINAPI wine_glMultiTexCoord4iv( GLenum target, const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord4iv( target, v );
}

static void WINAPI wine_glMultiTexCoord4ivARB( GLenum target, const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord4ivARB( target, v );
}

static void WINAPI wine_glMultiTexCoord4ivSGIS( GLenum target, GLint * v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord4ivSGIS( target, v );
}

static void WINAPI wine_glMultiTexCoord4s( GLenum target, GLshort s, GLshort t, GLshort r, GLshort q ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", target, s, t, r, q );
  funcs->ext.p_glMultiTexCoord4s( target, s, t, r, q );
}

static void WINAPI wine_glMultiTexCoord4sARB( GLenum target, GLshort s, GLshort t, GLshort r, GLshort q ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", target, s, t, r, q );
  funcs->ext.p_glMultiTexCoord4sARB( target, s, t, r, q );
}

static void WINAPI wine_glMultiTexCoord4sSGIS( GLenum target, GLshort s, GLshort t, GLshort r, GLshort q ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", target, s, t, r, q );
  funcs->ext.p_glMultiTexCoord4sSGIS( target, s, t, r, q );
}

static void WINAPI wine_glMultiTexCoord4sv( GLenum target, const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord4sv( target, v );
}

static void WINAPI wine_glMultiTexCoord4svARB( GLenum target, const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord4svARB( target, v );
}

static void WINAPI wine_glMultiTexCoord4svSGIS( GLenum target, GLshort * v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord4svSGIS( target, v );
}

static void WINAPI wine_glMultiTexCoordP1ui( GLenum texture, GLenum type, GLuint coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", texture, type, coords );
  funcs->ext.p_glMultiTexCoordP1ui( texture, type, coords );
}

static void WINAPI wine_glMultiTexCoordP1uiv( GLenum texture, GLenum type, const GLuint* coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", texture, type, coords );
  funcs->ext.p_glMultiTexCoordP1uiv( texture, type, coords );
}

static void WINAPI wine_glMultiTexCoordP2ui( GLenum texture, GLenum type, GLuint coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", texture, type, coords );
  funcs->ext.p_glMultiTexCoordP2ui( texture, type, coords );
}

static void WINAPI wine_glMultiTexCoordP2uiv( GLenum texture, GLenum type, const GLuint* coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", texture, type, coords );
  funcs->ext.p_glMultiTexCoordP2uiv( texture, type, coords );
}

static void WINAPI wine_glMultiTexCoordP3ui( GLenum texture, GLenum type, GLuint coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", texture, type, coords );
  funcs->ext.p_glMultiTexCoordP3ui( texture, type, coords );
}

static void WINAPI wine_glMultiTexCoordP3uiv( GLenum texture, GLenum type, const GLuint* coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", texture, type, coords );
  funcs->ext.p_glMultiTexCoordP3uiv( texture, type, coords );
}

static void WINAPI wine_glMultiTexCoordP4ui( GLenum texture, GLenum type, GLuint coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", texture, type, coords );
  funcs->ext.p_glMultiTexCoordP4ui( texture, type, coords );
}

static void WINAPI wine_glMultiTexCoordP4uiv( GLenum texture, GLenum type, const GLuint* coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", texture, type, coords );
  funcs->ext.p_glMultiTexCoordP4uiv( texture, type, coords );
}

static void WINAPI wine_glMultiTexCoordPointerEXT( GLenum texunit, GLint size, GLenum type, GLsizei stride, const GLvoid* pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", texunit, size, type, stride, pointer );
  funcs->ext.p_glMultiTexCoordPointerEXT( texunit, size, type, stride, pointer );
}

static void WINAPI wine_glMultiTexCoordPointerSGIS( GLenum target, GLint size, GLenum type, GLsizei stride, GLvoid * pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", target, size, type, stride, pointer );
  funcs->ext.p_glMultiTexCoordPointerSGIS( target, size, type, stride, pointer );
}

static void WINAPI wine_glMultiTexEnvfEXT( GLenum texunit, GLenum target, GLenum pname, GLfloat param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %f)\n", texunit, target, pname, param );
  funcs->ext.p_glMultiTexEnvfEXT( texunit, target, pname, param );
}

static void WINAPI wine_glMultiTexEnvfvEXT( GLenum texunit, GLenum target, GLenum pname, const GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", texunit, target, pname, params );
  funcs->ext.p_glMultiTexEnvfvEXT( texunit, target, pname, params );
}

static void WINAPI wine_glMultiTexEnviEXT( GLenum texunit, GLenum target, GLenum pname, GLint param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", texunit, target, pname, param );
  funcs->ext.p_glMultiTexEnviEXT( texunit, target, pname, param );
}

static void WINAPI wine_glMultiTexEnvivEXT( GLenum texunit, GLenum target, GLenum pname, const GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", texunit, target, pname, params );
  funcs->ext.p_glMultiTexEnvivEXT( texunit, target, pname, params );
}

static void WINAPI wine_glMultiTexGendEXT( GLenum texunit, GLenum coord, GLenum pname, GLdouble param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %f)\n", texunit, coord, pname, param );
  funcs->ext.p_glMultiTexGendEXT( texunit, coord, pname, param );
}

static void WINAPI wine_glMultiTexGendvEXT( GLenum texunit, GLenum coord, GLenum pname, const GLdouble* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", texunit, coord, pname, params );
  funcs->ext.p_glMultiTexGendvEXT( texunit, coord, pname, params );
}

static void WINAPI wine_glMultiTexGenfEXT( GLenum texunit, GLenum coord, GLenum pname, GLfloat param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %f)\n", texunit, coord, pname, param );
  funcs->ext.p_glMultiTexGenfEXT( texunit, coord, pname, param );
}

static void WINAPI wine_glMultiTexGenfvEXT( GLenum texunit, GLenum coord, GLenum pname, const GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", texunit, coord, pname, params );
  funcs->ext.p_glMultiTexGenfvEXT( texunit, coord, pname, params );
}

static void WINAPI wine_glMultiTexGeniEXT( GLenum texunit, GLenum coord, GLenum pname, GLint param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", texunit, coord, pname, param );
  funcs->ext.p_glMultiTexGeniEXT( texunit, coord, pname, param );
}

static void WINAPI wine_glMultiTexGenivEXT( GLenum texunit, GLenum coord, GLenum pname, const GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", texunit, coord, pname, params );
  funcs->ext.p_glMultiTexGenivEXT( texunit, coord, pname, params );
}

static void WINAPI wine_glMultiTexImage1DEXT( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid* pixels ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texunit, target, level, internalformat, width, border, format, type, pixels );
  funcs->ext.p_glMultiTexImage1DEXT( texunit, target, level, internalformat, width, border, format, type, pixels );
}

static void WINAPI wine_glMultiTexImage2DEXT( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texunit, target, level, internalformat, width, height, border, format, type, pixels );
  funcs->ext.p_glMultiTexImage2DEXT( texunit, target, level, internalformat, width, height, border, format, type, pixels );
}

static void WINAPI wine_glMultiTexImage3DEXT( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* pixels ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texunit, target, level, internalformat, width, height, depth, border, format, type, pixels );
  funcs->ext.p_glMultiTexImage3DEXT( texunit, target, level, internalformat, width, height, depth, border, format, type, pixels );
}

static void WINAPI wine_glMultiTexParameterIivEXT( GLenum texunit, GLenum target, GLenum pname, const GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", texunit, target, pname, params );
  funcs->ext.p_glMultiTexParameterIivEXT( texunit, target, pname, params );
}

static void WINAPI wine_glMultiTexParameterIuivEXT( GLenum texunit, GLenum target, GLenum pname, const GLuint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", texunit, target, pname, params );
  funcs->ext.p_glMultiTexParameterIuivEXT( texunit, target, pname, params );
}

static void WINAPI wine_glMultiTexParameterfEXT( GLenum texunit, GLenum target, GLenum pname, GLfloat param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %f)\n", texunit, target, pname, param );
  funcs->ext.p_glMultiTexParameterfEXT( texunit, target, pname, param );
}

static void WINAPI wine_glMultiTexParameterfvEXT( GLenum texunit, GLenum target, GLenum pname, const GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", texunit, target, pname, params );
  funcs->ext.p_glMultiTexParameterfvEXT( texunit, target, pname, params );
}

static void WINAPI wine_glMultiTexParameteriEXT( GLenum texunit, GLenum target, GLenum pname, GLint param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", texunit, target, pname, param );
  funcs->ext.p_glMultiTexParameteriEXT( texunit, target, pname, param );
}

static void WINAPI wine_glMultiTexParameterivEXT( GLenum texunit, GLenum target, GLenum pname, const GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", texunit, target, pname, params );
  funcs->ext.p_glMultiTexParameterivEXT( texunit, target, pname, params );
}

static void WINAPI wine_glMultiTexRenderbufferEXT( GLenum texunit, GLenum target, GLuint renderbuffer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", texunit, target, renderbuffer );
  funcs->ext.p_glMultiTexRenderbufferEXT( texunit, target, renderbuffer );
}

static void WINAPI wine_glMultiTexSubImage1DEXT( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid* pixels ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %p)\n", texunit, target, level, xoffset, width, format, type, pixels );
  funcs->ext.p_glMultiTexSubImage1DEXT( texunit, target, level, xoffset, width, format, type, pixels );
}

static void WINAPI wine_glMultiTexSubImage2DEXT( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texunit, target, level, xoffset, yoffset, width, height, format, type, pixels );
  funcs->ext.p_glMultiTexSubImage2DEXT( texunit, target, level, xoffset, yoffset, width, height, format, type, pixels );
}

static void WINAPI wine_glMultiTexSubImage3DEXT( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid* pixels ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texunit, target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels );
  funcs->ext.p_glMultiTexSubImage3DEXT( texunit, target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels );
}

static void WINAPI wine_glNamedBufferDataEXT( GLuint buffer, INT_PTR size, const GLvoid* data, GLenum usage ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %ld, %p, %d)\n", buffer, size, data, usage );
  funcs->ext.p_glNamedBufferDataEXT( buffer, size, data, usage );
}

static void WINAPI wine_glNamedBufferSubDataEXT( GLuint buffer, INT_PTR offset, INT_PTR size, const GLvoid* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %ld, %ld, %p)\n", buffer, offset, size, data );
  funcs->ext.p_glNamedBufferSubDataEXT( buffer, offset, size, data );
}

static void WINAPI wine_glNamedCopyBufferSubDataEXT( GLuint readBuffer, GLuint writeBuffer, INT_PTR readOffset, INT_PTR writeOffset, INT_PTR size ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %ld, %ld, %ld)\n", readBuffer, writeBuffer, readOffset, writeOffset, size );
  funcs->ext.p_glNamedCopyBufferSubDataEXT( readBuffer, writeBuffer, readOffset, writeOffset, size );
}

static void WINAPI wine_glNamedFramebufferRenderbufferEXT( GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", framebuffer, attachment, renderbuffertarget, renderbuffer );
  funcs->ext.p_glNamedFramebufferRenderbufferEXT( framebuffer, attachment, renderbuffertarget, renderbuffer );
}

static void WINAPI wine_glNamedFramebufferTexture1DEXT( GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", framebuffer, attachment, textarget, texture, level );
  funcs->ext.p_glNamedFramebufferTexture1DEXT( framebuffer, attachment, textarget, texture, level );
}

static void WINAPI wine_glNamedFramebufferTexture2DEXT( GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", framebuffer, attachment, textarget, texture, level );
  funcs->ext.p_glNamedFramebufferTexture2DEXT( framebuffer, attachment, textarget, texture, level );
}

static void WINAPI wine_glNamedFramebufferTexture3DEXT( GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d)\n", framebuffer, attachment, textarget, texture, level, zoffset );
  funcs->ext.p_glNamedFramebufferTexture3DEXT( framebuffer, attachment, textarget, texture, level, zoffset );
}

static void WINAPI wine_glNamedFramebufferTextureEXT( GLuint framebuffer, GLenum attachment, GLuint texture, GLint level ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", framebuffer, attachment, texture, level );
  funcs->ext.p_glNamedFramebufferTextureEXT( framebuffer, attachment, texture, level );
}

static void WINAPI wine_glNamedFramebufferTextureFaceEXT( GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLenum face ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", framebuffer, attachment, texture, level, face );
  funcs->ext.p_glNamedFramebufferTextureFaceEXT( framebuffer, attachment, texture, level, face );
}

static void WINAPI wine_glNamedFramebufferTextureLayerEXT( GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", framebuffer, attachment, texture, level, layer );
  funcs->ext.p_glNamedFramebufferTextureLayerEXT( framebuffer, attachment, texture, level, layer );
}

static void WINAPI wine_glNamedProgramLocalParameter4dEXT( GLuint program, GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %f, %f, %f, %f)\n", program, target, index, x, y, z, w );
  funcs->ext.p_glNamedProgramLocalParameter4dEXT( program, target, index, x, y, z, w );
}

static void WINAPI wine_glNamedProgramLocalParameter4dvEXT( GLuint program, GLenum target, GLuint index, const GLdouble* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, target, index, params );
  funcs->ext.p_glNamedProgramLocalParameter4dvEXT( program, target, index, params );
}

static void WINAPI wine_glNamedProgramLocalParameter4fEXT( GLuint program, GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %f, %f, %f, %f)\n", program, target, index, x, y, z, w );
  funcs->ext.p_glNamedProgramLocalParameter4fEXT( program, target, index, x, y, z, w );
}

static void WINAPI wine_glNamedProgramLocalParameter4fvEXT( GLuint program, GLenum target, GLuint index, const GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, target, index, params );
  funcs->ext.p_glNamedProgramLocalParameter4fvEXT( program, target, index, params );
}

static void WINAPI wine_glNamedProgramLocalParameterI4iEXT( GLuint program, GLenum target, GLuint index, GLint x, GLint y, GLint z, GLint w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d)\n", program, target, index, x, y, z, w );
  funcs->ext.p_glNamedProgramLocalParameterI4iEXT( program, target, index, x, y, z, w );
}

static void WINAPI wine_glNamedProgramLocalParameterI4ivEXT( GLuint program, GLenum target, GLuint index, const GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, target, index, params );
  funcs->ext.p_glNamedProgramLocalParameterI4ivEXT( program, target, index, params );
}

static void WINAPI wine_glNamedProgramLocalParameterI4uiEXT( GLuint program, GLenum target, GLuint index, GLuint x, GLuint y, GLuint z, GLuint w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d)\n", program, target, index, x, y, z, w );
  funcs->ext.p_glNamedProgramLocalParameterI4uiEXT( program, target, index, x, y, z, w );
}

static void WINAPI wine_glNamedProgramLocalParameterI4uivEXT( GLuint program, GLenum target, GLuint index, const GLuint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, target, index, params );
  funcs->ext.p_glNamedProgramLocalParameterI4uivEXT( program, target, index, params );
}

static void WINAPI wine_glNamedProgramLocalParameters4fvEXT( GLuint program, GLenum target, GLuint index, GLsizei count, const GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, target, index, count, params );
  funcs->ext.p_glNamedProgramLocalParameters4fvEXT( program, target, index, count, params );
}

static void WINAPI wine_glNamedProgramLocalParametersI4ivEXT( GLuint program, GLenum target, GLuint index, GLsizei count, const GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, target, index, count, params );
  funcs->ext.p_glNamedProgramLocalParametersI4ivEXT( program, target, index, count, params );
}

static void WINAPI wine_glNamedProgramLocalParametersI4uivEXT( GLuint program, GLenum target, GLuint index, GLsizei count, const GLuint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, target, index, count, params );
  funcs->ext.p_glNamedProgramLocalParametersI4uivEXT( program, target, index, count, params );
}

static void WINAPI wine_glNamedProgramStringEXT( GLuint program, GLenum target, GLenum format, GLsizei len, const GLvoid* string ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, target, format, len, string );
  funcs->ext.p_glNamedProgramStringEXT( program, target, format, len, string );
}

static void WINAPI wine_glNamedRenderbufferStorageEXT( GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", renderbuffer, internalformat, width, height );
  funcs->ext.p_glNamedRenderbufferStorageEXT( renderbuffer, internalformat, width, height );
}

static void WINAPI wine_glNamedRenderbufferStorageMultisampleCoverageEXT( GLuint renderbuffer, GLsizei coverageSamples, GLsizei colorSamples, GLenum internalformat, GLsizei width, GLsizei height ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d)\n", renderbuffer, coverageSamples, colorSamples, internalformat, width, height );
  funcs->ext.p_glNamedRenderbufferStorageMultisampleCoverageEXT( renderbuffer, coverageSamples, colorSamples, internalformat, width, height );
}

static void WINAPI wine_glNamedRenderbufferStorageMultisampleEXT( GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", renderbuffer, samples, internalformat, width, height );
  funcs->ext.p_glNamedRenderbufferStorageMultisampleEXT( renderbuffer, samples, internalformat, width, height );
}

static void WINAPI wine_glNamedStringARB( GLenum type, GLint namelen, const char* name, GLint stringlen, const char* string ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %d, %p)\n", type, namelen, name, stringlen, string );
  funcs->ext.p_glNamedStringARB( type, namelen, name, stringlen, string );
}

static GLuint WINAPI wine_glNewBufferRegion( GLenum type ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", type );
  return funcs->ext.p_glNewBufferRegion( type );
}

static GLuint WINAPI wine_glNewObjectBufferATI( GLsizei size, const GLvoid* pointer, GLenum usage ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p, %d)\n", size, pointer, usage );
  return funcs->ext.p_glNewObjectBufferATI( size, pointer, usage );
}

static void WINAPI wine_glNormal3fVertex3fSUN( GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f, %f, %f, %f)\n", nx, ny, nz, x, y, z );
  funcs->ext.p_glNormal3fVertex3fSUN( nx, ny, nz, x, y, z );
}

static void WINAPI wine_glNormal3fVertex3fvSUN( const GLfloat* n, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p, %p)\n", n, v );
  funcs->ext.p_glNormal3fVertex3fvSUN( n, v );
}

static void WINAPI wine_glNormal3hNV( unsigned short nx, unsigned short ny, unsigned short nz ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", nx, ny, nz );
  funcs->ext.p_glNormal3hNV( nx, ny, nz );
}

static void WINAPI wine_glNormal3hvNV( const unsigned short* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glNormal3hvNV( v );
}

static void WINAPI wine_glNormalFormatNV( GLenum type, GLsizei stride ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", type, stride );
  funcs->ext.p_glNormalFormatNV( type, stride );
}

static void WINAPI wine_glNormalP3ui( GLenum type, GLuint coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", type, coords );
  funcs->ext.p_glNormalP3ui( type, coords );
}

static void WINAPI wine_glNormalP3uiv( GLenum type, const GLuint* coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", type, coords );
  funcs->ext.p_glNormalP3uiv( type, coords );
}

static void WINAPI wine_glNormalPointerEXT( GLenum type, GLsizei stride, GLsizei count, const GLvoid* pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", type, stride, count, pointer );
  funcs->ext.p_glNormalPointerEXT( type, stride, count, pointer );
}

static void WINAPI wine_glNormalPointerListIBM( GLenum type, GLint stride, const GLvoid** pointer, GLint ptrstride ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %d)\n", type, stride, pointer, ptrstride );
  funcs->ext.p_glNormalPointerListIBM( type, stride, pointer, ptrstride );
}

static void WINAPI wine_glNormalPointervINTEL( GLenum type, const GLvoid** pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", type, pointer );
  funcs->ext.p_glNormalPointervINTEL( type, pointer );
}

static void WINAPI wine_glNormalStream3bATI( GLenum stream, GLbyte nx, GLbyte ny, GLbyte nz ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", stream, nx, ny, nz );
  funcs->ext.p_glNormalStream3bATI( stream, nx, ny, nz );
}

static void WINAPI wine_glNormalStream3bvATI( GLenum stream, const GLbyte* coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", stream, coords );
  funcs->ext.p_glNormalStream3bvATI( stream, coords );
}

static void WINAPI wine_glNormalStream3dATI( GLenum stream, GLdouble nx, GLdouble ny, GLdouble nz ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f)\n", stream, nx, ny, nz );
  funcs->ext.p_glNormalStream3dATI( stream, nx, ny, nz );
}

static void WINAPI wine_glNormalStream3dvATI( GLenum stream, const GLdouble* coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", stream, coords );
  funcs->ext.p_glNormalStream3dvATI( stream, coords );
}

static void WINAPI wine_glNormalStream3fATI( GLenum stream, GLfloat nx, GLfloat ny, GLfloat nz ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f)\n", stream, nx, ny, nz );
  funcs->ext.p_glNormalStream3fATI( stream, nx, ny, nz );
}

static void WINAPI wine_glNormalStream3fvATI( GLenum stream, const GLfloat* coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", stream, coords );
  funcs->ext.p_glNormalStream3fvATI( stream, coords );
}

static void WINAPI wine_glNormalStream3iATI( GLenum stream, GLint nx, GLint ny, GLint nz ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", stream, nx, ny, nz );
  funcs->ext.p_glNormalStream3iATI( stream, nx, ny, nz );
}

static void WINAPI wine_glNormalStream3ivATI( GLenum stream, const GLint* coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", stream, coords );
  funcs->ext.p_glNormalStream3ivATI( stream, coords );
}

static void WINAPI wine_glNormalStream3sATI( GLenum stream, GLshort nx, GLshort ny, GLshort nz ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", stream, nx, ny, nz );
  funcs->ext.p_glNormalStream3sATI( stream, nx, ny, nz );
}

static void WINAPI wine_glNormalStream3svATI( GLenum stream, const GLshort* coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", stream, coords );
  funcs->ext.p_glNormalStream3svATI( stream, coords );
}

static GLenum WINAPI wine_glObjectPurgeableAPPLE( GLenum objectType, GLuint name, GLenum option ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", objectType, name, option );
  return funcs->ext.p_glObjectPurgeableAPPLE( objectType, name, option );
}

static GLenum WINAPI wine_glObjectUnpurgeableAPPLE( GLenum objectType, GLuint name, GLenum option ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", objectType, name, option );
  return funcs->ext.p_glObjectUnpurgeableAPPLE( objectType, name, option );
}

static void WINAPI wine_glPNTrianglesfATI( GLenum pname, GLfloat param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f)\n", pname, param );
  funcs->ext.p_glPNTrianglesfATI( pname, param );
}

static void WINAPI wine_glPNTrianglesiATI( GLenum pname, GLint param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", pname, param );
  funcs->ext.p_glPNTrianglesiATI( pname, param );
}

static void WINAPI wine_glPassTexCoordATI( GLuint dst, GLuint coord, GLenum swizzle ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", dst, coord, swizzle );
  funcs->ext.p_glPassTexCoordATI( dst, coord, swizzle );
}

static void WINAPI wine_glPatchParameterfv( GLenum pname, const GLfloat* values ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", pname, values );
  funcs->ext.p_glPatchParameterfv( pname, values );
}

static void WINAPI wine_glPatchParameteri( GLenum pname, GLint value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", pname, value );
  funcs->ext.p_glPatchParameteri( pname, value );
}

static void WINAPI wine_glPathColorGenNV( GLenum color, GLenum genMode, GLenum colorFormat, const GLfloat* coeffs ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", color, genMode, colorFormat, coeffs );
  funcs->ext.p_glPathColorGenNV( color, genMode, colorFormat, coeffs );
}

static void WINAPI wine_glPathCommandsNV( GLuint path, GLsizei numCommands, const GLubyte* commands, GLsizei numCoords, GLenum coordType, const GLvoid* coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %d, %d, %p)\n", path, numCommands, commands, numCoords, coordType, coords );
  funcs->ext.p_glPathCommandsNV( path, numCommands, commands, numCoords, coordType, coords );
}

static void WINAPI wine_glPathCoordsNV( GLuint path, GLsizei numCoords, GLenum coordType, const GLvoid* coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", path, numCoords, coordType, coords );
  funcs->ext.p_glPathCoordsNV( path, numCoords, coordType, coords );
}

static void WINAPI wine_glPathCoverDepthFuncNV( GLenum func ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", func );
  funcs->ext.p_glPathCoverDepthFuncNV( func );
}

static void WINAPI wine_glPathDashArrayNV( GLuint path, GLsizei dashCount, const GLfloat* dashArray ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", path, dashCount, dashArray );
  funcs->ext.p_glPathDashArrayNV( path, dashCount, dashArray );
}

static void WINAPI wine_glPathFogGenNV( GLenum genMode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", genMode );
  funcs->ext.p_glPathFogGenNV( genMode );
}

static void WINAPI wine_glPathGlyphRangeNV( GLuint firstPathName, GLenum fontTarget, const GLvoid* fontName, GLbitfield fontStyle, GLuint firstGlyph, GLsizei numGlyphs, GLenum handleMissingGlyphs, GLuint pathParameterTemplate, GLfloat emScale ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %d, %d, %d, %d, %d, %f)\n", firstPathName, fontTarget, fontName, fontStyle, firstGlyph, numGlyphs, handleMissingGlyphs, pathParameterTemplate, emScale );
  funcs->ext.p_glPathGlyphRangeNV( firstPathName, fontTarget, fontName, fontStyle, firstGlyph, numGlyphs, handleMissingGlyphs, pathParameterTemplate, emScale );
}

static void WINAPI wine_glPathGlyphsNV( GLuint firstPathName, GLenum fontTarget, const GLvoid* fontName, GLbitfield fontStyle, GLsizei numGlyphs, GLenum type, const GLvoid* charcodes, GLenum handleMissingGlyphs, GLuint pathParameterTemplate, GLfloat emScale ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %d, %d, %d, %p, %d, %d, %f)\n", firstPathName, fontTarget, fontName, fontStyle, numGlyphs, type, charcodes, handleMissingGlyphs, pathParameterTemplate, emScale );
  funcs->ext.p_glPathGlyphsNV( firstPathName, fontTarget, fontName, fontStyle, numGlyphs, type, charcodes, handleMissingGlyphs, pathParameterTemplate, emScale );
}

static void WINAPI wine_glPathParameterfNV( GLuint path, GLenum pname, GLfloat value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f)\n", path, pname, value );
  funcs->ext.p_glPathParameterfNV( path, pname, value );
}

static void WINAPI wine_glPathParameterfvNV( GLuint path, GLenum pname, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", path, pname, value );
  funcs->ext.p_glPathParameterfvNV( path, pname, value );
}

static void WINAPI wine_glPathParameteriNV( GLuint path, GLenum pname, GLint value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", path, pname, value );
  funcs->ext.p_glPathParameteriNV( path, pname, value );
}

static void WINAPI wine_glPathParameterivNV( GLuint path, GLenum pname, const GLint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", path, pname, value );
  funcs->ext.p_glPathParameterivNV( path, pname, value );
}

static void WINAPI wine_glPathStencilDepthOffsetNV( GLfloat factor, GLfloat units ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f)\n", factor, units );
  funcs->ext.p_glPathStencilDepthOffsetNV( factor, units );
}

static void WINAPI wine_glPathStencilFuncNV( GLenum func, GLint ref, GLuint mask ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", func, ref, mask );
  funcs->ext.p_glPathStencilFuncNV( func, ref, mask );
}

static void WINAPI wine_glPathStringNV( GLuint path, GLenum format, GLsizei length, const GLvoid* pathString ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", path, format, length, pathString );
  funcs->ext.p_glPathStringNV( path, format, length, pathString );
}

static void WINAPI wine_glPathSubCommandsNV( GLuint path, GLsizei commandStart, GLsizei commandsToDelete, GLsizei numCommands, const GLubyte* commands, GLsizei numCoords, GLenum coordType, const GLvoid* coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p, %d, %d, %p)\n", path, commandStart, commandsToDelete, numCommands, commands, numCoords, coordType, coords );
  funcs->ext.p_glPathSubCommandsNV( path, commandStart, commandsToDelete, numCommands, commands, numCoords, coordType, coords );
}

static void WINAPI wine_glPathSubCoordsNV( GLuint path, GLsizei coordStart, GLsizei numCoords, GLenum coordType, const GLvoid* coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", path, coordStart, numCoords, coordType, coords );
  funcs->ext.p_glPathSubCoordsNV( path, coordStart, numCoords, coordType, coords );
}

static void WINAPI wine_glPathTexGenNV( GLenum texCoordSet, GLenum genMode, GLint components, const GLfloat* coeffs ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", texCoordSet, genMode, components, coeffs );
  funcs->ext.p_glPathTexGenNV( texCoordSet, genMode, components, coeffs );
}

static void WINAPI wine_glPauseTransformFeedback( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  funcs->ext.p_glPauseTransformFeedback( );
}

static void WINAPI wine_glPauseTransformFeedbackNV( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  funcs->ext.p_glPauseTransformFeedbackNV( );
}

static void WINAPI wine_glPixelDataRangeNV( GLenum target, GLsizei length, GLvoid* pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, length, pointer );
  funcs->ext.p_glPixelDataRangeNV( target, length, pointer );
}

static void WINAPI wine_glPixelTexGenParameterfSGIS( GLenum pname, GLfloat param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f)\n", pname, param );
  funcs->ext.p_glPixelTexGenParameterfSGIS( pname, param );
}

static void WINAPI wine_glPixelTexGenParameterfvSGIS( GLenum pname, const GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", pname, params );
  funcs->ext.p_glPixelTexGenParameterfvSGIS( pname, params );
}

static void WINAPI wine_glPixelTexGenParameteriSGIS( GLenum pname, GLint param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", pname, param );
  funcs->ext.p_glPixelTexGenParameteriSGIS( pname, param );
}

static void WINAPI wine_glPixelTexGenParameterivSGIS( GLenum pname, const GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", pname, params );
  funcs->ext.p_glPixelTexGenParameterivSGIS( pname, params );
}

static void WINAPI wine_glPixelTexGenSGIX( GLenum mode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", mode );
  funcs->ext.p_glPixelTexGenSGIX( mode );
}

static void WINAPI wine_glPixelTransformParameterfEXT( GLenum target, GLenum pname, GLfloat param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f)\n", target, pname, param );
  funcs->ext.p_glPixelTransformParameterfEXT( target, pname, param );
}

static void WINAPI wine_glPixelTransformParameterfvEXT( GLenum target, GLenum pname, const GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glPixelTransformParameterfvEXT( target, pname, params );
}

static void WINAPI wine_glPixelTransformParameteriEXT( GLenum target, GLenum pname, GLint param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", target, pname, param );
  funcs->ext.p_glPixelTransformParameteriEXT( target, pname, param );
}

static void WINAPI wine_glPixelTransformParameterivEXT( GLenum target, GLenum pname, const GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glPixelTransformParameterivEXT( target, pname, params );
}

static GLboolean WINAPI wine_glPointAlongPathNV( GLuint path, GLsizei startSegment, GLsizei numSegments, GLfloat distance, GLfloat* x, GLfloat* y, GLfloat* tangentX, GLfloat* tangentY ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %f, %p, %p, %p, %p)\n", path, startSegment, numSegments, distance, x, y, tangentX, tangentY );
  return funcs->ext.p_glPointAlongPathNV( path, startSegment, numSegments, distance, x, y, tangentX, tangentY );
}

static void WINAPI wine_glPointParameterf( GLenum pname, GLfloat param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f)\n", pname, param );
  funcs->ext.p_glPointParameterf( pname, param );
}

static void WINAPI wine_glPointParameterfARB( GLenum pname, GLfloat param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f)\n", pname, param );
  funcs->ext.p_glPointParameterfARB( pname, param );
}

static void WINAPI wine_glPointParameterfEXT( GLenum pname, GLfloat param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f)\n", pname, param );
  funcs->ext.p_glPointParameterfEXT( pname, param );
}

static void WINAPI wine_glPointParameterfSGIS( GLenum pname, GLfloat param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f)\n", pname, param );
  funcs->ext.p_glPointParameterfSGIS( pname, param );
}

static void WINAPI wine_glPointParameterfv( GLenum pname, const GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", pname, params );
  funcs->ext.p_glPointParameterfv( pname, params );
}

static void WINAPI wine_glPointParameterfvARB( GLenum pname, const GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", pname, params );
  funcs->ext.p_glPointParameterfvARB( pname, params );
}

static void WINAPI wine_glPointParameterfvEXT( GLenum pname, const GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", pname, params );
  funcs->ext.p_glPointParameterfvEXT( pname, params );
}

static void WINAPI wine_glPointParameterfvSGIS( GLenum pname, const GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", pname, params );
  funcs->ext.p_glPointParameterfvSGIS( pname, params );
}

static void WINAPI wine_glPointParameteri( GLenum pname, GLint param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", pname, param );
  funcs->ext.p_glPointParameteri( pname, param );
}

static void WINAPI wine_glPointParameteriNV( GLenum pname, GLint param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", pname, param );
  funcs->ext.p_glPointParameteriNV( pname, param );
}

static void WINAPI wine_glPointParameteriv( GLenum pname, const GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", pname, params );
  funcs->ext.p_glPointParameteriv( pname, params );
}

static void WINAPI wine_glPointParameterivNV( GLenum pname, const GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", pname, params );
  funcs->ext.p_glPointParameterivNV( pname, params );
}

static GLint WINAPI wine_glPollAsyncSGIX( GLuint* markerp ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", markerp );
  return funcs->ext.p_glPollAsyncSGIX( markerp );
}

static GLint WINAPI wine_glPollInstrumentsSGIX( GLint* marker_p ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", marker_p );
  return funcs->ext.p_glPollInstrumentsSGIX( marker_p );
}

static void WINAPI wine_glPolygonOffsetEXT( GLfloat factor, GLfloat bias ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f)\n", factor, bias );
  funcs->ext.p_glPolygonOffsetEXT( factor, bias );
}

static void WINAPI wine_glPresentFrameDualFillNV( GLuint video_slot, UINT64 minPresentTime, GLuint beginPresentTimeId, GLuint presentDurationId, GLenum type, GLenum target0, GLuint fill0, GLenum target1, GLuint fill1, GLenum target2, GLuint fill2, GLenum target3, GLuint fill3 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %s, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", video_slot, wine_dbgstr_longlong(minPresentTime), beginPresentTimeId, presentDurationId, type, target0, fill0, target1, fill1, target2, fill2, target3, fill3 );
  funcs->ext.p_glPresentFrameDualFillNV( video_slot, minPresentTime, beginPresentTimeId, presentDurationId, type, target0, fill0, target1, fill1, target2, fill2, target3, fill3 );
}

static void WINAPI wine_glPresentFrameKeyedNV( GLuint video_slot, UINT64 minPresentTime, GLuint beginPresentTimeId, GLuint presentDurationId, GLenum type, GLenum target0, GLuint fill0, GLuint key0, GLenum target1, GLuint fill1, GLuint key1 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %s, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", video_slot, wine_dbgstr_longlong(minPresentTime), beginPresentTimeId, presentDurationId, type, target0, fill0, key0, target1, fill1, key1 );
  funcs->ext.p_glPresentFrameKeyedNV( video_slot, minPresentTime, beginPresentTimeId, presentDurationId, type, target0, fill0, key0, target1, fill1, key1 );
}

static void WINAPI wine_glPrimitiveRestartIndex( GLuint index ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", index );
  funcs->ext.p_glPrimitiveRestartIndex( index );
}

static void WINAPI wine_glPrimitiveRestartIndexNV( GLuint index ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", index );
  funcs->ext.p_glPrimitiveRestartIndexNV( index );
}

static void WINAPI wine_glPrimitiveRestartNV( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  funcs->ext.p_glPrimitiveRestartNV( );
}

static void WINAPI wine_glPrioritizeTexturesEXT( GLsizei n, const GLuint* textures, const GLclampf* priorities ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p, %p)\n", n, textures, priorities );
  funcs->ext.p_glPrioritizeTexturesEXT( n, textures, priorities );
}

static void WINAPI wine_glProgramBinary( GLuint program, GLenum binaryFormat, const GLvoid* binary, GLsizei length ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %d)\n", program, binaryFormat, binary, length );
  funcs->ext.p_glProgramBinary( program, binaryFormat, binary, length );
}

static void WINAPI wine_glProgramBufferParametersIivNV( GLenum target, GLuint buffer, GLuint index, GLsizei count, const GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", target, buffer, index, count, params );
  funcs->ext.p_glProgramBufferParametersIivNV( target, buffer, index, count, params );
}

static void WINAPI wine_glProgramBufferParametersIuivNV( GLenum target, GLuint buffer, GLuint index, GLsizei count, const GLuint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", target, buffer, index, count, params );
  funcs->ext.p_glProgramBufferParametersIuivNV( target, buffer, index, count, params );
}

static void WINAPI wine_glProgramBufferParametersfvNV( GLenum target, GLuint buffer, GLuint index, GLsizei count, const GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", target, buffer, index, count, params );
  funcs->ext.p_glProgramBufferParametersfvNV( target, buffer, index, count, params );
}

static void WINAPI wine_glProgramEnvParameter4dARB( GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f, %f, %f, %f)\n", target, index, x, y, z, w );
  funcs->ext.p_glProgramEnvParameter4dARB( target, index, x, y, z, w );
}

static void WINAPI wine_glProgramEnvParameter4dvARB( GLenum target, GLuint index, const GLdouble* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, index, params );
  funcs->ext.p_glProgramEnvParameter4dvARB( target, index, params );
}

static void WINAPI wine_glProgramEnvParameter4fARB( GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f, %f, %f, %f)\n", target, index, x, y, z, w );
  funcs->ext.p_glProgramEnvParameter4fARB( target, index, x, y, z, w );
}

static void WINAPI wine_glProgramEnvParameter4fvARB( GLenum target, GLuint index, const GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, index, params );
  funcs->ext.p_glProgramEnvParameter4fvARB( target, index, params );
}

static void WINAPI wine_glProgramEnvParameterI4iNV( GLenum target, GLuint index, GLint x, GLint y, GLint z, GLint w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d)\n", target, index, x, y, z, w );
  funcs->ext.p_glProgramEnvParameterI4iNV( target, index, x, y, z, w );
}

static void WINAPI wine_glProgramEnvParameterI4ivNV( GLenum target, GLuint index, const GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, index, params );
  funcs->ext.p_glProgramEnvParameterI4ivNV( target, index, params );
}

static void WINAPI wine_glProgramEnvParameterI4uiNV( GLenum target, GLuint index, GLuint x, GLuint y, GLuint z, GLuint w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d)\n", target, index, x, y, z, w );
  funcs->ext.p_glProgramEnvParameterI4uiNV( target, index, x, y, z, w );
}

static void WINAPI wine_glProgramEnvParameterI4uivNV( GLenum target, GLuint index, const GLuint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, index, params );
  funcs->ext.p_glProgramEnvParameterI4uivNV( target, index, params );
}

static void WINAPI wine_glProgramEnvParameters4fvEXT( GLenum target, GLuint index, GLsizei count, const GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", target, index, count, params );
  funcs->ext.p_glProgramEnvParameters4fvEXT( target, index, count, params );
}

static void WINAPI wine_glProgramEnvParametersI4ivNV( GLenum target, GLuint index, GLsizei count, const GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", target, index, count, params );
  funcs->ext.p_glProgramEnvParametersI4ivNV( target, index, count, params );
}

static void WINAPI wine_glProgramEnvParametersI4uivNV( GLenum target, GLuint index, GLsizei count, const GLuint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", target, index, count, params );
  funcs->ext.p_glProgramEnvParametersI4uivNV( target, index, count, params );
}

static void WINAPI wine_glProgramLocalParameter4dARB( GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f, %f, %f, %f)\n", target, index, x, y, z, w );
  funcs->ext.p_glProgramLocalParameter4dARB( target, index, x, y, z, w );
}

static void WINAPI wine_glProgramLocalParameter4dvARB( GLenum target, GLuint index, const GLdouble* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, index, params );
  funcs->ext.p_glProgramLocalParameter4dvARB( target, index, params );
}

static void WINAPI wine_glProgramLocalParameter4fARB( GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f, %f, %f, %f)\n", target, index, x, y, z, w );
  funcs->ext.p_glProgramLocalParameter4fARB( target, index, x, y, z, w );
}

static void WINAPI wine_glProgramLocalParameter4fvARB( GLenum target, GLuint index, const GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, index, params );
  funcs->ext.p_glProgramLocalParameter4fvARB( target, index, params );
}

static void WINAPI wine_glProgramLocalParameterI4iNV( GLenum target, GLuint index, GLint x, GLint y, GLint z, GLint w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d)\n", target, index, x, y, z, w );
  funcs->ext.p_glProgramLocalParameterI4iNV( target, index, x, y, z, w );
}

static void WINAPI wine_glProgramLocalParameterI4ivNV( GLenum target, GLuint index, const GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, index, params );
  funcs->ext.p_glProgramLocalParameterI4ivNV( target, index, params );
}

static void WINAPI wine_glProgramLocalParameterI4uiNV( GLenum target, GLuint index, GLuint x, GLuint y, GLuint z, GLuint w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d)\n", target, index, x, y, z, w );
  funcs->ext.p_glProgramLocalParameterI4uiNV( target, index, x, y, z, w );
}

static void WINAPI wine_glProgramLocalParameterI4uivNV( GLenum target, GLuint index, const GLuint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, index, params );
  funcs->ext.p_glProgramLocalParameterI4uivNV( target, index, params );
}

static void WINAPI wine_glProgramLocalParameters4fvEXT( GLenum target, GLuint index, GLsizei count, const GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", target, index, count, params );
  funcs->ext.p_glProgramLocalParameters4fvEXT( target, index, count, params );
}

static void WINAPI wine_glProgramLocalParametersI4ivNV( GLenum target, GLuint index, GLsizei count, const GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", target, index, count, params );
  funcs->ext.p_glProgramLocalParametersI4ivNV( target, index, count, params );
}

static void WINAPI wine_glProgramLocalParametersI4uivNV( GLenum target, GLuint index, GLsizei count, const GLuint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", target, index, count, params );
  funcs->ext.p_glProgramLocalParametersI4uivNV( target, index, count, params );
}

static void WINAPI wine_glProgramNamedParameter4dNV( GLuint id, GLsizei len, const GLubyte* name, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %f, %f, %f, %f)\n", id, len, name, x, y, z, w );
  funcs->ext.p_glProgramNamedParameter4dNV( id, len, name, x, y, z, w );
}

static void WINAPI wine_glProgramNamedParameter4dvNV( GLuint id, GLsizei len, const GLubyte* name, const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %p)\n", id, len, name, v );
  funcs->ext.p_glProgramNamedParameter4dvNV( id, len, name, v );
}

static void WINAPI wine_glProgramNamedParameter4fNV( GLuint id, GLsizei len, const GLubyte* name, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %f, %f, %f, %f)\n", id, len, name, x, y, z, w );
  funcs->ext.p_glProgramNamedParameter4fNV( id, len, name, x, y, z, w );
}

static void WINAPI wine_glProgramNamedParameter4fvNV( GLuint id, GLsizei len, const GLubyte* name, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %p)\n", id, len, name, v );
  funcs->ext.p_glProgramNamedParameter4fvNV( id, len, name, v );
}

static void WINAPI wine_glProgramParameter4dNV( GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f, %f, %f, %f)\n", target, index, x, y, z, w );
  funcs->ext.p_glProgramParameter4dNV( target, index, x, y, z, w );
}

static void WINAPI wine_glProgramParameter4dvNV( GLenum target, GLuint index, const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, index, v );
  funcs->ext.p_glProgramParameter4dvNV( target, index, v );
}

static void WINAPI wine_glProgramParameter4fNV( GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f, %f, %f, %f)\n", target, index, x, y, z, w );
  funcs->ext.p_glProgramParameter4fNV( target, index, x, y, z, w );
}

static void WINAPI wine_glProgramParameter4fvNV( GLenum target, GLuint index, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, index, v );
  funcs->ext.p_glProgramParameter4fvNV( target, index, v );
}

static void WINAPI wine_glProgramParameteri( GLuint program, GLenum pname, GLint value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", program, pname, value );
  funcs->ext.p_glProgramParameteri( program, pname, value );
}

static void WINAPI wine_glProgramParameteriARB( GLuint program, GLenum pname, GLint value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", program, pname, value );
  funcs->ext.p_glProgramParameteriARB( program, pname, value );
}

static void WINAPI wine_glProgramParameteriEXT( GLuint program, GLenum pname, GLint value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", program, pname, value );
  funcs->ext.p_glProgramParameteriEXT( program, pname, value );
}

static void WINAPI wine_glProgramParameters4dvNV( GLenum target, GLuint index, GLsizei count, const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", target, index, count, v );
  funcs->ext.p_glProgramParameters4dvNV( target, index, count, v );
}

static void WINAPI wine_glProgramParameters4fvNV( GLenum target, GLuint index, GLsizei count, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", target, index, count, v );
  funcs->ext.p_glProgramParameters4fvNV( target, index, count, v );
}

static void WINAPI wine_glProgramStringARB( GLenum target, GLenum format, GLsizei len, const GLvoid* string ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", target, format, len, string );
  funcs->ext.p_glProgramStringARB( target, format, len, string );
}

static void WINAPI wine_glProgramSubroutineParametersuivNV( GLenum target, GLsizei count, const GLuint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, count, params );
  funcs->ext.p_glProgramSubroutineParametersuivNV( target, count, params );
}

static void WINAPI wine_glProgramUniform1d( GLuint program, GLint location, GLdouble v0 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f)\n", program, location, v0 );
  funcs->ext.p_glProgramUniform1d( program, location, v0 );
}

static void WINAPI wine_glProgramUniform1dEXT( GLuint program, GLint location, GLdouble x ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f)\n", program, location, x );
  funcs->ext.p_glProgramUniform1dEXT( program, location, x );
}

static void WINAPI wine_glProgramUniform1dv( GLuint program, GLint location, GLsizei count, const GLdouble* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform1dv( program, location, count, value );
}

static void WINAPI wine_glProgramUniform1dvEXT( GLuint program, GLint location, GLsizei count, const GLdouble* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform1dvEXT( program, location, count, value );
}

static void WINAPI wine_glProgramUniform1f( GLuint program, GLint location, GLfloat v0 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f)\n", program, location, v0 );
  funcs->ext.p_glProgramUniform1f( program, location, v0 );
}

static void WINAPI wine_glProgramUniform1fEXT( GLuint program, GLint location, GLfloat v0 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f)\n", program, location, v0 );
  funcs->ext.p_glProgramUniform1fEXT( program, location, v0 );
}

static void WINAPI wine_glProgramUniform1fv( GLuint program, GLint location, GLsizei count, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform1fv( program, location, count, value );
}

static void WINAPI wine_glProgramUniform1fvEXT( GLuint program, GLint location, GLsizei count, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform1fvEXT( program, location, count, value );
}

static void WINAPI wine_glProgramUniform1i( GLuint program, GLint location, GLint v0 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", program, location, v0 );
  funcs->ext.p_glProgramUniform1i( program, location, v0 );
}

static void WINAPI wine_glProgramUniform1i64NV( GLuint program, GLint location, INT64 x ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %s)\n", program, location, wine_dbgstr_longlong(x) );
  funcs->ext.p_glProgramUniform1i64NV( program, location, x );
}

static void WINAPI wine_glProgramUniform1i64vNV( GLuint program, GLint location, GLsizei count, const INT64* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform1i64vNV( program, location, count, value );
}

static void WINAPI wine_glProgramUniform1iEXT( GLuint program, GLint location, GLint v0 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", program, location, v0 );
  funcs->ext.p_glProgramUniform1iEXT( program, location, v0 );
}

static void WINAPI wine_glProgramUniform1iv( GLuint program, GLint location, GLsizei count, const GLint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform1iv( program, location, count, value );
}

static void WINAPI wine_glProgramUniform1ivEXT( GLuint program, GLint location, GLsizei count, const GLint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform1ivEXT( program, location, count, value );
}

static void WINAPI wine_glProgramUniform1ui( GLuint program, GLint location, GLuint v0 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", program, location, v0 );
  funcs->ext.p_glProgramUniform1ui( program, location, v0 );
}

static void WINAPI wine_glProgramUniform1ui64NV( GLuint program, GLint location, UINT64 x ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %s)\n", program, location, wine_dbgstr_longlong(x) );
  funcs->ext.p_glProgramUniform1ui64NV( program, location, x );
}

static void WINAPI wine_glProgramUniform1ui64vNV( GLuint program, GLint location, GLsizei count, const UINT64* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform1ui64vNV( program, location, count, value );
}

static void WINAPI wine_glProgramUniform1uiEXT( GLuint program, GLint location, GLuint v0 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", program, location, v0 );
  funcs->ext.p_glProgramUniform1uiEXT( program, location, v0 );
}

static void WINAPI wine_glProgramUniform1uiv( GLuint program, GLint location, GLsizei count, const GLuint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform1uiv( program, location, count, value );
}

static void WINAPI wine_glProgramUniform1uivEXT( GLuint program, GLint location, GLsizei count, const GLuint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform1uivEXT( program, location, count, value );
}

static void WINAPI wine_glProgramUniform2d( GLuint program, GLint location, GLdouble v0, GLdouble v1 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f, %f)\n", program, location, v0, v1 );
  funcs->ext.p_glProgramUniform2d( program, location, v0, v1 );
}

static void WINAPI wine_glProgramUniform2dEXT( GLuint program, GLint location, GLdouble x, GLdouble y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f, %f)\n", program, location, x, y );
  funcs->ext.p_glProgramUniform2dEXT( program, location, x, y );
}

static void WINAPI wine_glProgramUniform2dv( GLuint program, GLint location, GLsizei count, const GLdouble* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform2dv( program, location, count, value );
}

static void WINAPI wine_glProgramUniform2dvEXT( GLuint program, GLint location, GLsizei count, const GLdouble* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform2dvEXT( program, location, count, value );
}

static void WINAPI wine_glProgramUniform2f( GLuint program, GLint location, GLfloat v0, GLfloat v1 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f, %f)\n", program, location, v0, v1 );
  funcs->ext.p_glProgramUniform2f( program, location, v0, v1 );
}

static void WINAPI wine_glProgramUniform2fEXT( GLuint program, GLint location, GLfloat v0, GLfloat v1 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f, %f)\n", program, location, v0, v1 );
  funcs->ext.p_glProgramUniform2fEXT( program, location, v0, v1 );
}

static void WINAPI wine_glProgramUniform2fv( GLuint program, GLint location, GLsizei count, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform2fv( program, location, count, value );
}

static void WINAPI wine_glProgramUniform2fvEXT( GLuint program, GLint location, GLsizei count, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform2fvEXT( program, location, count, value );
}

static void WINAPI wine_glProgramUniform2i( GLuint program, GLint location, GLint v0, GLint v1 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", program, location, v0, v1 );
  funcs->ext.p_glProgramUniform2i( program, location, v0, v1 );
}

static void WINAPI wine_glProgramUniform2i64NV( GLuint program, GLint location, INT64 x, INT64 y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %s, %s)\n", program, location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y) );
  funcs->ext.p_glProgramUniform2i64NV( program, location, x, y );
}

static void WINAPI wine_glProgramUniform2i64vNV( GLuint program, GLint location, GLsizei count, const INT64* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform2i64vNV( program, location, count, value );
}

static void WINAPI wine_glProgramUniform2iEXT( GLuint program, GLint location, GLint v0, GLint v1 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", program, location, v0, v1 );
  funcs->ext.p_glProgramUniform2iEXT( program, location, v0, v1 );
}

static void WINAPI wine_glProgramUniform2iv( GLuint program, GLint location, GLsizei count, const GLint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform2iv( program, location, count, value );
}

static void WINAPI wine_glProgramUniform2ivEXT( GLuint program, GLint location, GLsizei count, const GLint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform2ivEXT( program, location, count, value );
}

static void WINAPI wine_glProgramUniform2ui( GLuint program, GLint location, GLuint v0, GLuint v1 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", program, location, v0, v1 );
  funcs->ext.p_glProgramUniform2ui( program, location, v0, v1 );
}

static void WINAPI wine_glProgramUniform2ui64NV( GLuint program, GLint location, UINT64 x, UINT64 y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %s, %s)\n", program, location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y) );
  funcs->ext.p_glProgramUniform2ui64NV( program, location, x, y );
}

static void WINAPI wine_glProgramUniform2ui64vNV( GLuint program, GLint location, GLsizei count, const UINT64* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform2ui64vNV( program, location, count, value );
}

static void WINAPI wine_glProgramUniform2uiEXT( GLuint program, GLint location, GLuint v0, GLuint v1 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", program, location, v0, v1 );
  funcs->ext.p_glProgramUniform2uiEXT( program, location, v0, v1 );
}

static void WINAPI wine_glProgramUniform2uiv( GLuint program, GLint location, GLsizei count, const GLuint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform2uiv( program, location, count, value );
}

static void WINAPI wine_glProgramUniform2uivEXT( GLuint program, GLint location, GLsizei count, const GLuint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform2uivEXT( program, location, count, value );
}

static void WINAPI wine_glProgramUniform3d( GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f, %f, %f)\n", program, location, v0, v1, v2 );
  funcs->ext.p_glProgramUniform3d( program, location, v0, v1, v2 );
}

static void WINAPI wine_glProgramUniform3dEXT( GLuint program, GLint location, GLdouble x, GLdouble y, GLdouble z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f, %f, %f)\n", program, location, x, y, z );
  funcs->ext.p_glProgramUniform3dEXT( program, location, x, y, z );
}

static void WINAPI wine_glProgramUniform3dv( GLuint program, GLint location, GLsizei count, const GLdouble* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform3dv( program, location, count, value );
}

static void WINAPI wine_glProgramUniform3dvEXT( GLuint program, GLint location, GLsizei count, const GLdouble* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform3dvEXT( program, location, count, value );
}

static void WINAPI wine_glProgramUniform3f( GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f, %f, %f)\n", program, location, v0, v1, v2 );
  funcs->ext.p_glProgramUniform3f( program, location, v0, v1, v2 );
}

static void WINAPI wine_glProgramUniform3fEXT( GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f, %f, %f)\n", program, location, v0, v1, v2 );
  funcs->ext.p_glProgramUniform3fEXT( program, location, v0, v1, v2 );
}

static void WINAPI wine_glProgramUniform3fv( GLuint program, GLint location, GLsizei count, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform3fv( program, location, count, value );
}

static void WINAPI wine_glProgramUniform3fvEXT( GLuint program, GLint location, GLsizei count, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform3fvEXT( program, location, count, value );
}

static void WINAPI wine_glProgramUniform3i( GLuint program, GLint location, GLint v0, GLint v1, GLint v2 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", program, location, v0, v1, v2 );
  funcs->ext.p_glProgramUniform3i( program, location, v0, v1, v2 );
}

static void WINAPI wine_glProgramUniform3i64NV( GLuint program, GLint location, INT64 x, INT64 y, INT64 z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %s, %s, %s)\n", program, location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z) );
  funcs->ext.p_glProgramUniform3i64NV( program, location, x, y, z );
}

static void WINAPI wine_glProgramUniform3i64vNV( GLuint program, GLint location, GLsizei count, const INT64* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform3i64vNV( program, location, count, value );
}

static void WINAPI wine_glProgramUniform3iEXT( GLuint program, GLint location, GLint v0, GLint v1, GLint v2 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", program, location, v0, v1, v2 );
  funcs->ext.p_glProgramUniform3iEXT( program, location, v0, v1, v2 );
}

static void WINAPI wine_glProgramUniform3iv( GLuint program, GLint location, GLsizei count, const GLint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform3iv( program, location, count, value );
}

static void WINAPI wine_glProgramUniform3ivEXT( GLuint program, GLint location, GLsizei count, const GLint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform3ivEXT( program, location, count, value );
}

static void WINAPI wine_glProgramUniform3ui( GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", program, location, v0, v1, v2 );
  funcs->ext.p_glProgramUniform3ui( program, location, v0, v1, v2 );
}

static void WINAPI wine_glProgramUniform3ui64NV( GLuint program, GLint location, UINT64 x, UINT64 y, UINT64 z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %s, %s, %s)\n", program, location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z) );
  funcs->ext.p_glProgramUniform3ui64NV( program, location, x, y, z );
}

static void WINAPI wine_glProgramUniform3ui64vNV( GLuint program, GLint location, GLsizei count, const UINT64* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform3ui64vNV( program, location, count, value );
}

static void WINAPI wine_glProgramUniform3uiEXT( GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", program, location, v0, v1, v2 );
  funcs->ext.p_glProgramUniform3uiEXT( program, location, v0, v1, v2 );
}

static void WINAPI wine_glProgramUniform3uiv( GLuint program, GLint location, GLsizei count, const GLuint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform3uiv( program, location, count, value );
}

static void WINAPI wine_glProgramUniform3uivEXT( GLuint program, GLint location, GLsizei count, const GLuint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform3uivEXT( program, location, count, value );
}

static void WINAPI wine_glProgramUniform4d( GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2, GLdouble v3 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f, %f, %f, %f)\n", program, location, v0, v1, v2, v3 );
  funcs->ext.p_glProgramUniform4d( program, location, v0, v1, v2, v3 );
}

static void WINAPI wine_glProgramUniform4dEXT( GLuint program, GLint location, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f, %f, %f, %f)\n", program, location, x, y, z, w );
  funcs->ext.p_glProgramUniform4dEXT( program, location, x, y, z, w );
}

static void WINAPI wine_glProgramUniform4dv( GLuint program, GLint location, GLsizei count, const GLdouble* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform4dv( program, location, count, value );
}

static void WINAPI wine_glProgramUniform4dvEXT( GLuint program, GLint location, GLsizei count, const GLdouble* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform4dvEXT( program, location, count, value );
}

static void WINAPI wine_glProgramUniform4f( GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f, %f, %f, %f)\n", program, location, v0, v1, v2, v3 );
  funcs->ext.p_glProgramUniform4f( program, location, v0, v1, v2, v3 );
}

static void WINAPI wine_glProgramUniform4fEXT( GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f, %f, %f, %f)\n", program, location, v0, v1, v2, v3 );
  funcs->ext.p_glProgramUniform4fEXT( program, location, v0, v1, v2, v3 );
}

static void WINAPI wine_glProgramUniform4fv( GLuint program, GLint location, GLsizei count, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform4fv( program, location, count, value );
}

static void WINAPI wine_glProgramUniform4fvEXT( GLuint program, GLint location, GLsizei count, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform4fvEXT( program, location, count, value );
}

static void WINAPI wine_glProgramUniform4i( GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d)\n", program, location, v0, v1, v2, v3 );
  funcs->ext.p_glProgramUniform4i( program, location, v0, v1, v2, v3 );
}

static void WINAPI wine_glProgramUniform4i64NV( GLuint program, GLint location, INT64 x, INT64 y, INT64 z, INT64 w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %s, %s, %s, %s)\n", program, location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z), wine_dbgstr_longlong(w) );
  funcs->ext.p_glProgramUniform4i64NV( program, location, x, y, z, w );
}

static void WINAPI wine_glProgramUniform4i64vNV( GLuint program, GLint location, GLsizei count, const INT64* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform4i64vNV( program, location, count, value );
}

static void WINAPI wine_glProgramUniform4iEXT( GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d)\n", program, location, v0, v1, v2, v3 );
  funcs->ext.p_glProgramUniform4iEXT( program, location, v0, v1, v2, v3 );
}

static void WINAPI wine_glProgramUniform4iv( GLuint program, GLint location, GLsizei count, const GLint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform4iv( program, location, count, value );
}

static void WINAPI wine_glProgramUniform4ivEXT( GLuint program, GLint location, GLsizei count, const GLint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform4ivEXT( program, location, count, value );
}

static void WINAPI wine_glProgramUniform4ui( GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d)\n", program, location, v0, v1, v2, v3 );
  funcs->ext.p_glProgramUniform4ui( program, location, v0, v1, v2, v3 );
}

static void WINAPI wine_glProgramUniform4ui64NV( GLuint program, GLint location, UINT64 x, UINT64 y, UINT64 z, UINT64 w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %s, %s, %s, %s)\n", program, location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z), wine_dbgstr_longlong(w) );
  funcs->ext.p_glProgramUniform4ui64NV( program, location, x, y, z, w );
}

static void WINAPI wine_glProgramUniform4ui64vNV( GLuint program, GLint location, GLsizei count, const UINT64* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform4ui64vNV( program, location, count, value );
}

static void WINAPI wine_glProgramUniform4uiEXT( GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d)\n", program, location, v0, v1, v2, v3 );
  funcs->ext.p_glProgramUniform4uiEXT( program, location, v0, v1, v2, v3 );
}

static void WINAPI wine_glProgramUniform4uiv( GLuint program, GLint location, GLsizei count, const GLuint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform4uiv( program, location, count, value );
}

static void WINAPI wine_glProgramUniform4uivEXT( GLuint program, GLint location, GLsizei count, const GLuint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform4uivEXT( program, location, count, value );
}

static void WINAPI wine_glProgramUniformHandleui64NV( GLuint program, GLint location, UINT64 value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %s)\n", program, location, wine_dbgstr_longlong(value) );
  funcs->ext.p_glProgramUniformHandleui64NV( program, location, value );
}

static void WINAPI wine_glProgramUniformHandleui64vNV( GLuint program, GLint location, GLsizei count, const UINT64* values ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, values );
  funcs->ext.p_glProgramUniformHandleui64vNV( program, location, count, values );
}

static void WINAPI wine_glProgramUniformMatrix2dv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix2dv( program, location, count, transpose, value );
}

static void WINAPI wine_glProgramUniformMatrix2dvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix2dvEXT( program, location, count, transpose, value );
}

static void WINAPI wine_glProgramUniformMatrix2fv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix2fv( program, location, count, transpose, value );
}

static void WINAPI wine_glProgramUniformMatrix2fvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix2fvEXT( program, location, count, transpose, value );
}

static void WINAPI wine_glProgramUniformMatrix2x3dv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix2x3dv( program, location, count, transpose, value );
}

static void WINAPI wine_glProgramUniformMatrix2x3dvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix2x3dvEXT( program, location, count, transpose, value );
}

static void WINAPI wine_glProgramUniformMatrix2x3fv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix2x3fv( program, location, count, transpose, value );
}

static void WINAPI wine_glProgramUniformMatrix2x3fvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix2x3fvEXT( program, location, count, transpose, value );
}

static void WINAPI wine_glProgramUniformMatrix2x4dv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix2x4dv( program, location, count, transpose, value );
}

static void WINAPI wine_glProgramUniformMatrix2x4dvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix2x4dvEXT( program, location, count, transpose, value );
}

static void WINAPI wine_glProgramUniformMatrix2x4fv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix2x4fv( program, location, count, transpose, value );
}

static void WINAPI wine_glProgramUniformMatrix2x4fvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix2x4fvEXT( program, location, count, transpose, value );
}

static void WINAPI wine_glProgramUniformMatrix3dv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix3dv( program, location, count, transpose, value );
}

static void WINAPI wine_glProgramUniformMatrix3dvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix3dvEXT( program, location, count, transpose, value );
}

static void WINAPI wine_glProgramUniformMatrix3fv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix3fv( program, location, count, transpose, value );
}

static void WINAPI wine_glProgramUniformMatrix3fvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix3fvEXT( program, location, count, transpose, value );
}

static void WINAPI wine_glProgramUniformMatrix3x2dv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix3x2dv( program, location, count, transpose, value );
}

static void WINAPI wine_glProgramUniformMatrix3x2dvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix3x2dvEXT( program, location, count, transpose, value );
}

static void WINAPI wine_glProgramUniformMatrix3x2fv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix3x2fv( program, location, count, transpose, value );
}

static void WINAPI wine_glProgramUniformMatrix3x2fvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix3x2fvEXT( program, location, count, transpose, value );
}

static void WINAPI wine_glProgramUniformMatrix3x4dv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix3x4dv( program, location, count, transpose, value );
}

static void WINAPI wine_glProgramUniformMatrix3x4dvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix3x4dvEXT( program, location, count, transpose, value );
}

static void WINAPI wine_glProgramUniformMatrix3x4fv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix3x4fv( program, location, count, transpose, value );
}

static void WINAPI wine_glProgramUniformMatrix3x4fvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix3x4fvEXT( program, location, count, transpose, value );
}

static void WINAPI wine_glProgramUniformMatrix4dv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix4dv( program, location, count, transpose, value );
}

static void WINAPI wine_glProgramUniformMatrix4dvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix4dvEXT( program, location, count, transpose, value );
}

static void WINAPI wine_glProgramUniformMatrix4fv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix4fv( program, location, count, transpose, value );
}

static void WINAPI wine_glProgramUniformMatrix4fvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix4fvEXT( program, location, count, transpose, value );
}

static void WINAPI wine_glProgramUniformMatrix4x2dv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix4x2dv( program, location, count, transpose, value );
}

static void WINAPI wine_glProgramUniformMatrix4x2dvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix4x2dvEXT( program, location, count, transpose, value );
}

static void WINAPI wine_glProgramUniformMatrix4x2fv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix4x2fv( program, location, count, transpose, value );
}

static void WINAPI wine_glProgramUniformMatrix4x2fvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix4x2fvEXT( program, location, count, transpose, value );
}

static void WINAPI wine_glProgramUniformMatrix4x3dv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix4x3dv( program, location, count, transpose, value );
}

static void WINAPI wine_glProgramUniformMatrix4x3dvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix4x3dvEXT( program, location, count, transpose, value );
}

static void WINAPI wine_glProgramUniformMatrix4x3fv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix4x3fv( program, location, count, transpose, value );
}

static void WINAPI wine_glProgramUniformMatrix4x3fvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix4x3fvEXT( program, location, count, transpose, value );
}

static void WINAPI wine_glProgramUniformui64NV( GLuint program, GLint location, UINT64 value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %s)\n", program, location, wine_dbgstr_longlong(value) );
  funcs->ext.p_glProgramUniformui64NV( program, location, value );
}

static void WINAPI wine_glProgramUniformui64vNV( GLuint program, GLint location, GLsizei count, const UINT64* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniformui64vNV( program, location, count, value );
}

static void WINAPI wine_glProgramVertexLimitNV( GLenum target, GLint limit ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", target, limit );
  funcs->ext.p_glProgramVertexLimitNV( target, limit );
}

static void WINAPI wine_glProvokingVertex( GLenum mode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", mode );
  funcs->ext.p_glProvokingVertex( mode );
}

static void WINAPI wine_glProvokingVertexEXT( GLenum mode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", mode );
  funcs->ext.p_glProvokingVertexEXT( mode );
}

static void WINAPI wine_glPushClientAttribDefaultEXT( GLbitfield mask ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", mask );
  funcs->ext.p_glPushClientAttribDefaultEXT( mask );
}

static void WINAPI wine_glQueryCounter( GLuint id, GLenum target ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", id, target );
  funcs->ext.p_glQueryCounter( id, target );
}

static void WINAPI wine_glReadBufferRegion( GLenum region, GLint x, GLint y, GLsizei width, GLsizei height ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", region, x, y, width, height );
  funcs->ext.p_glReadBufferRegion( region, x, y, width, height );
}

static void WINAPI wine_glReadInstrumentsSGIX( GLint marker ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", marker );
  funcs->ext.p_glReadInstrumentsSGIX( marker );
}

static void WINAPI wine_glReadnPixelsARB( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, GLvoid* data ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %p)\n", x, y, width, height, format, type, bufSize, data );
  funcs->ext.p_glReadnPixelsARB( x, y, width, height, format, type, bufSize, data );
}

static void WINAPI wine_glReferencePlaneSGIX( const GLdouble* equation ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", equation );
  funcs->ext.p_glReferencePlaneSGIX( equation );
}

static void WINAPI wine_glReleaseShaderCompiler( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  funcs->ext.p_glReleaseShaderCompiler( );
}

static void WINAPI wine_glRenderbufferStorage( GLenum target, GLenum internalformat, GLsizei width, GLsizei height ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", target, internalformat, width, height );
  funcs->ext.p_glRenderbufferStorage( target, internalformat, width, height );
}

static void WINAPI wine_glRenderbufferStorageEXT( GLenum target, GLenum internalformat, GLsizei width, GLsizei height ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", target, internalformat, width, height );
  funcs->ext.p_glRenderbufferStorageEXT( target, internalformat, width, height );
}

static void WINAPI wine_glRenderbufferStorageMultisample( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", target, samples, internalformat, width, height );
  funcs->ext.p_glRenderbufferStorageMultisample( target, samples, internalformat, width, height );
}

static void WINAPI wine_glRenderbufferStorageMultisampleCoverageNV( GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLenum internalformat, GLsizei width, GLsizei height ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d)\n", target, coverageSamples, colorSamples, internalformat, width, height );
  funcs->ext.p_glRenderbufferStorageMultisampleCoverageNV( target, coverageSamples, colorSamples, internalformat, width, height );
}

static void WINAPI wine_glRenderbufferStorageMultisampleEXT( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", target, samples, internalformat, width, height );
  funcs->ext.p_glRenderbufferStorageMultisampleEXT( target, samples, internalformat, width, height );
}

static void WINAPI wine_glReplacementCodePointerSUN( GLenum type, GLsizei stride, const GLvoid** pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", type, stride, pointer );
  funcs->ext.p_glReplacementCodePointerSUN( type, stride, pointer );
}

static void WINAPI wine_glReplacementCodeubSUN( GLubyte code ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", code );
  funcs->ext.p_glReplacementCodeubSUN( code );
}

static void WINAPI wine_glReplacementCodeubvSUN( const GLubyte* code ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", code );
  funcs->ext.p_glReplacementCodeubvSUN( code );
}

static void WINAPI wine_glReplacementCodeuiColor3fVertex3fSUN( GLuint rc, GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f, %f, %f, %f)\n", rc, r, g, b, x, y, z );
  funcs->ext.p_glReplacementCodeuiColor3fVertex3fSUN( rc, r, g, b, x, y, z );
}

static void WINAPI wine_glReplacementCodeuiColor3fVertex3fvSUN( const GLuint* rc, const GLfloat* c, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p, %p, %p)\n", rc, c, v );
  funcs->ext.p_glReplacementCodeuiColor3fVertex3fvSUN( rc, c, v );
}

static void WINAPI wine_glReplacementCodeuiColor4fNormal3fVertex3fSUN( GLuint rc, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f)\n", rc, r, g, b, a, nx, ny, nz, x, y, z );
  funcs->ext.p_glReplacementCodeuiColor4fNormal3fVertex3fSUN( rc, r, g, b, a, nx, ny, nz, x, y, z );
}

static void WINAPI wine_glReplacementCodeuiColor4fNormal3fVertex3fvSUN( const GLuint* rc, const GLfloat* c, const GLfloat* n, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p, %p, %p, %p)\n", rc, c, n, v );
  funcs->ext.p_glReplacementCodeuiColor4fNormal3fVertex3fvSUN( rc, c, n, v );
}

static void WINAPI wine_glReplacementCodeuiColor4ubVertex3fSUN( GLuint rc, GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %f, %f, %f)\n", rc, r, g, b, a, x, y, z );
  funcs->ext.p_glReplacementCodeuiColor4ubVertex3fSUN( rc, r, g, b, a, x, y, z );
}

static void WINAPI wine_glReplacementCodeuiColor4ubVertex3fvSUN( const GLuint* rc, const GLubyte* c, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p, %p, %p)\n", rc, c, v );
  funcs->ext.p_glReplacementCodeuiColor4ubVertex3fvSUN( rc, c, v );
}

static void WINAPI wine_glReplacementCodeuiNormal3fVertex3fSUN( GLuint rc, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f, %f, %f, %f)\n", rc, nx, ny, nz, x, y, z );
  funcs->ext.p_glReplacementCodeuiNormal3fVertex3fSUN( rc, nx, ny, nz, x, y, z );
}

static void WINAPI wine_glReplacementCodeuiNormal3fVertex3fvSUN( const GLuint* rc, const GLfloat* n, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p, %p, %p)\n", rc, n, v );
  funcs->ext.p_glReplacementCodeuiNormal3fVertex3fvSUN( rc, n, v );
}

static void WINAPI wine_glReplacementCodeuiSUN( GLuint code ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", code );
  funcs->ext.p_glReplacementCodeuiSUN( code );
}

static void WINAPI wine_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN( GLuint rc, GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f)\n", rc, s, t, r, g, b, a, nx, ny, nz, x, y, z );
  funcs->ext.p_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN( rc, s, t, r, g, b, a, nx, ny, nz, x, y, z );
}

static void WINAPI wine_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN( const GLuint* rc, const GLfloat* tc, const GLfloat* c, const GLfloat* n, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p, %p, %p, %p, %p)\n", rc, tc, c, n, v );
  funcs->ext.p_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN( rc, tc, c, n, v );
}

static void WINAPI wine_glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN( GLuint rc, GLfloat s, GLfloat t, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f, %f, %f, %f, %f, %f)\n", rc, s, t, nx, ny, nz, x, y, z );
  funcs->ext.p_glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN( rc, s, t, nx, ny, nz, x, y, z );
}

static void WINAPI wine_glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN( const GLuint* rc, const GLfloat* tc, const GLfloat* n, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p, %p, %p, %p)\n", rc, tc, n, v );
  funcs->ext.p_glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN( rc, tc, n, v );
}

static void WINAPI wine_glReplacementCodeuiTexCoord2fVertex3fSUN( GLuint rc, GLfloat s, GLfloat t, GLfloat x, GLfloat y, GLfloat z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f, %f, %f)\n", rc, s, t, x, y, z );
  funcs->ext.p_glReplacementCodeuiTexCoord2fVertex3fSUN( rc, s, t, x, y, z );
}

static void WINAPI wine_glReplacementCodeuiTexCoord2fVertex3fvSUN( const GLuint* rc, const GLfloat* tc, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p, %p, %p)\n", rc, tc, v );
  funcs->ext.p_glReplacementCodeuiTexCoord2fVertex3fvSUN( rc, tc, v );
}

static void WINAPI wine_glReplacementCodeuiVertex3fSUN( GLuint rc, GLfloat x, GLfloat y, GLfloat z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f)\n", rc, x, y, z );
  funcs->ext.p_glReplacementCodeuiVertex3fSUN( rc, x, y, z );
}

static void WINAPI wine_glReplacementCodeuiVertex3fvSUN( const GLuint* rc, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p, %p)\n", rc, v );
  funcs->ext.p_glReplacementCodeuiVertex3fvSUN( rc, v );
}

static void WINAPI wine_glReplacementCodeuivSUN( const GLuint* code ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", code );
  funcs->ext.p_glReplacementCodeuivSUN( code );
}

static void WINAPI wine_glReplacementCodeusSUN( GLushort code ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", code );
  funcs->ext.p_glReplacementCodeusSUN( code );
}

static void WINAPI wine_glReplacementCodeusvSUN( const GLushort* code ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", code );
  funcs->ext.p_glReplacementCodeusvSUN( code );
}

static void WINAPI wine_glRequestResidentProgramsNV( GLsizei n, const GLuint* programs ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", n, programs );
  funcs->ext.p_glRequestResidentProgramsNV( n, programs );
}

static void WINAPI wine_glResetHistogram( GLenum target ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", target );
  funcs->ext.p_glResetHistogram( target );
}

static void WINAPI wine_glResetHistogramEXT( GLenum target ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", target );
  funcs->ext.p_glResetHistogramEXT( target );
}

static void WINAPI wine_glResetMinmax( GLenum target ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", target );
  funcs->ext.p_glResetMinmax( target );
}

static void WINAPI wine_glResetMinmaxEXT( GLenum target ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", target );
  funcs->ext.p_glResetMinmaxEXT( target );
}

static void WINAPI wine_glResizeBuffersMESA( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  funcs->ext.p_glResizeBuffersMESA( );
}

static void WINAPI wine_glResumeTransformFeedback( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  funcs->ext.p_glResumeTransformFeedback( );
}

static void WINAPI wine_glResumeTransformFeedbackNV( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  funcs->ext.p_glResumeTransformFeedbackNV( );
}

static void WINAPI wine_glSampleCoverage( GLfloat value, GLboolean invert ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %d)\n", value, invert );
  funcs->ext.p_glSampleCoverage( value, invert );
}

static void WINAPI wine_glSampleCoverageARB( GLfloat value, GLboolean invert ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %d)\n", value, invert );
  funcs->ext.p_glSampleCoverageARB( value, invert );
}

static void WINAPI wine_glSampleMapATI( GLuint dst, GLuint interp, GLenum swizzle ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", dst, interp, swizzle );
  funcs->ext.p_glSampleMapATI( dst, interp, swizzle );
}

static void WINAPI wine_glSampleMaskEXT( GLclampf value, GLboolean invert ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %d)\n", value, invert );
  funcs->ext.p_glSampleMaskEXT( value, invert );
}

static void WINAPI wine_glSampleMaskIndexedNV( GLuint index, GLbitfield mask ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", index, mask );
  funcs->ext.p_glSampleMaskIndexedNV( index, mask );
}

static void WINAPI wine_glSampleMaskSGIS( GLclampf value, GLboolean invert ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %d)\n", value, invert );
  funcs->ext.p_glSampleMaskSGIS( value, invert );
}

static void WINAPI wine_glSampleMaski( GLuint index, GLbitfield mask ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", index, mask );
  funcs->ext.p_glSampleMaski( index, mask );
}

static void WINAPI wine_glSamplePatternEXT( GLenum pattern ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", pattern );
  funcs->ext.p_glSamplePatternEXT( pattern );
}

static void WINAPI wine_glSamplePatternSGIS( GLenum pattern ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", pattern );
  funcs->ext.p_glSamplePatternSGIS( pattern );
}

static void WINAPI wine_glSamplerParameterIiv( GLuint sampler, GLenum pname, const GLint* param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", sampler, pname, param );
  funcs->ext.p_glSamplerParameterIiv( sampler, pname, param );
}

static void WINAPI wine_glSamplerParameterIuiv( GLuint sampler, GLenum pname, const GLuint* param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", sampler, pname, param );
  funcs->ext.p_glSamplerParameterIuiv( sampler, pname, param );
}

static void WINAPI wine_glSamplerParameterf( GLuint sampler, GLenum pname, GLfloat param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %f)\n", sampler, pname, param );
  funcs->ext.p_glSamplerParameterf( sampler, pname, param );
}

static void WINAPI wine_glSamplerParameterfv( GLuint sampler, GLenum pname, const GLfloat* param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", sampler, pname, param );
  funcs->ext.p_glSamplerParameterfv( sampler, pname, param );
}

static void WINAPI wine_glSamplerParameteri( GLuint sampler, GLenum pname, GLint param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", sampler, pname, param );
  funcs->ext.p_glSamplerParameteri( sampler, pname, param );
}

static void WINAPI wine_glSamplerParameteriv( GLuint sampler, GLenum pname, const GLint* param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", sampler, pname, param );
  funcs->ext.p_glSamplerParameteriv( sampler, pname, param );
}

static void WINAPI wine_glScissorArrayv( GLuint first, GLsizei count, const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", first, count, v );
  funcs->ext.p_glScissorArrayv( first, count, v );
}

static void WINAPI wine_glScissorIndexed( GLuint index, GLint left, GLint bottom, GLsizei width, GLsizei height ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", index, left, bottom, width, height );
  funcs->ext.p_glScissorIndexed( index, left, bottom, width, height );
}

static void WINAPI wine_glScissorIndexedv( GLuint index, const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glScissorIndexedv( index, v );
}

static void WINAPI wine_glSecondaryColor3b( GLbyte red, GLbyte green, GLbyte blue ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", red, green, blue );
  funcs->ext.p_glSecondaryColor3b( red, green, blue );
}

static void WINAPI wine_glSecondaryColor3bEXT( GLbyte red, GLbyte green, GLbyte blue ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", red, green, blue );
  funcs->ext.p_glSecondaryColor3bEXT( red, green, blue );
}

static void WINAPI wine_glSecondaryColor3bv( const GLbyte* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glSecondaryColor3bv( v );
}

static void WINAPI wine_glSecondaryColor3bvEXT( const GLbyte* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glSecondaryColor3bvEXT( v );
}

static void WINAPI wine_glSecondaryColor3d( GLdouble red, GLdouble green, GLdouble blue ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f)\n", red, green, blue );
  funcs->ext.p_glSecondaryColor3d( red, green, blue );
}

static void WINAPI wine_glSecondaryColor3dEXT( GLdouble red, GLdouble green, GLdouble blue ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f)\n", red, green, blue );
  funcs->ext.p_glSecondaryColor3dEXT( red, green, blue );
}

static void WINAPI wine_glSecondaryColor3dv( const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glSecondaryColor3dv( v );
}

static void WINAPI wine_glSecondaryColor3dvEXT( const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glSecondaryColor3dvEXT( v );
}

static void WINAPI wine_glSecondaryColor3f( GLfloat red, GLfloat green, GLfloat blue ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f)\n", red, green, blue );
  funcs->ext.p_glSecondaryColor3f( red, green, blue );
}

static void WINAPI wine_glSecondaryColor3fEXT( GLfloat red, GLfloat green, GLfloat blue ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f)\n", red, green, blue );
  funcs->ext.p_glSecondaryColor3fEXT( red, green, blue );
}

static void WINAPI wine_glSecondaryColor3fv( const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glSecondaryColor3fv( v );
}

static void WINAPI wine_glSecondaryColor3fvEXT( const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glSecondaryColor3fvEXT( v );
}

static void WINAPI wine_glSecondaryColor3hNV( unsigned short red, unsigned short green, unsigned short blue ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", red, green, blue );
  funcs->ext.p_glSecondaryColor3hNV( red, green, blue );
}

static void WINAPI wine_glSecondaryColor3hvNV( const unsigned short* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glSecondaryColor3hvNV( v );
}

static void WINAPI wine_glSecondaryColor3i( GLint red, GLint green, GLint blue ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", red, green, blue );
  funcs->ext.p_glSecondaryColor3i( red, green, blue );
}

static void WINAPI wine_glSecondaryColor3iEXT( GLint red, GLint green, GLint blue ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", red, green, blue );
  funcs->ext.p_glSecondaryColor3iEXT( red, green, blue );
}

static void WINAPI wine_glSecondaryColor3iv( const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glSecondaryColor3iv( v );
}

static void WINAPI wine_glSecondaryColor3ivEXT( const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glSecondaryColor3ivEXT( v );
}

static void WINAPI wine_glSecondaryColor3s( GLshort red, GLshort green, GLshort blue ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", red, green, blue );
  funcs->ext.p_glSecondaryColor3s( red, green, blue );
}

static void WINAPI wine_glSecondaryColor3sEXT( GLshort red, GLshort green, GLshort blue ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", red, green, blue );
  funcs->ext.p_glSecondaryColor3sEXT( red, green, blue );
}

static void WINAPI wine_glSecondaryColor3sv( const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glSecondaryColor3sv( v );
}

static void WINAPI wine_glSecondaryColor3svEXT( const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glSecondaryColor3svEXT( v );
}

static void WINAPI wine_glSecondaryColor3ub( GLubyte red, GLubyte green, GLubyte blue ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", red, green, blue );
  funcs->ext.p_glSecondaryColor3ub( red, green, blue );
}

static void WINAPI wine_glSecondaryColor3ubEXT( GLubyte red, GLubyte green, GLubyte blue ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", red, green, blue );
  funcs->ext.p_glSecondaryColor3ubEXT( red, green, blue );
}

static void WINAPI wine_glSecondaryColor3ubv( const GLubyte* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glSecondaryColor3ubv( v );
}

static void WINAPI wine_glSecondaryColor3ubvEXT( const GLubyte* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glSecondaryColor3ubvEXT( v );
}

static void WINAPI wine_glSecondaryColor3ui( GLuint red, GLuint green, GLuint blue ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", red, green, blue );
  funcs->ext.p_glSecondaryColor3ui( red, green, blue );
}

static void WINAPI wine_glSecondaryColor3uiEXT( GLuint red, GLuint green, GLuint blue ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", red, green, blue );
  funcs->ext.p_glSecondaryColor3uiEXT( red, green, blue );
}

static void WINAPI wine_glSecondaryColor3uiv( const GLuint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glSecondaryColor3uiv( v );
}

static void WINAPI wine_glSecondaryColor3uivEXT( const GLuint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glSecondaryColor3uivEXT( v );
}

static void WINAPI wine_glSecondaryColor3us( GLushort red, GLushort green, GLushort blue ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", red, green, blue );
  funcs->ext.p_glSecondaryColor3us( red, green, blue );
}

static void WINAPI wine_glSecondaryColor3usEXT( GLushort red, GLushort green, GLushort blue ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", red, green, blue );
  funcs->ext.p_glSecondaryColor3usEXT( red, green, blue );
}

static void WINAPI wine_glSecondaryColor3usv( const GLushort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glSecondaryColor3usv( v );
}

static void WINAPI wine_glSecondaryColor3usvEXT( const GLushort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glSecondaryColor3usvEXT( v );
}

static void WINAPI wine_glSecondaryColorFormatNV( GLint size, GLenum type, GLsizei stride ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", size, type, stride );
  funcs->ext.p_glSecondaryColorFormatNV( size, type, stride );
}

static void WINAPI wine_glSecondaryColorP3ui( GLenum type, GLuint color ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", type, color );
  funcs->ext.p_glSecondaryColorP3ui( type, color );
}

static void WINAPI wine_glSecondaryColorP3uiv( GLenum type, const GLuint* color ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", type, color );
  funcs->ext.p_glSecondaryColorP3uiv( type, color );
}

static void WINAPI wine_glSecondaryColorPointer( GLint size, GLenum type, GLsizei stride, const GLvoid* pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", size, type, stride, pointer );
  funcs->ext.p_glSecondaryColorPointer( size, type, stride, pointer );
}

static void WINAPI wine_glSecondaryColorPointerEXT( GLint size, GLenum type, GLsizei stride, const GLvoid* pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", size, type, stride, pointer );
  funcs->ext.p_glSecondaryColorPointerEXT( size, type, stride, pointer );
}

static void WINAPI wine_glSecondaryColorPointerListIBM( GLint size, GLenum type, GLint stride, const GLvoid** pointer, GLint ptrstride ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p, %d)\n", size, type, stride, pointer, ptrstride );
  funcs->ext.p_glSecondaryColorPointerListIBM( size, type, stride, pointer, ptrstride );
}

static void WINAPI wine_glSelectPerfMonitorCountersAMD( GLuint monitor, GLboolean enable, GLuint group, GLint numCounters, GLuint* counterList ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", monitor, enable, group, numCounters, counterList );
  funcs->ext.p_glSelectPerfMonitorCountersAMD( monitor, enable, group, numCounters, counterList );
}

static void WINAPI wine_glSelectTextureCoordSetSGIS( GLenum target ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", target );
  funcs->ext.p_glSelectTextureCoordSetSGIS( target );
}

static void WINAPI wine_glSelectTextureSGIS( GLenum target ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", target );
  funcs->ext.p_glSelectTextureSGIS( target );
}

static void WINAPI wine_glSeparableFilter2D( GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* row, const GLvoid* column ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %p, %p)\n", target, internalformat, width, height, format, type, row, column );
  funcs->ext.p_glSeparableFilter2D( target, internalformat, width, height, format, type, row, column );
}

static void WINAPI wine_glSeparableFilter2DEXT( GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* row, const GLvoid* column ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %p, %p)\n", target, internalformat, width, height, format, type, row, column );
  funcs->ext.p_glSeparableFilter2DEXT( target, internalformat, width, height, format, type, row, column );
}

static void WINAPI wine_glSetFenceAPPLE( GLuint fence ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", fence );
  funcs->ext.p_glSetFenceAPPLE( fence );
}

static void WINAPI wine_glSetFenceNV( GLuint fence, GLenum condition ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", fence, condition );
  funcs->ext.p_glSetFenceNV( fence, condition );
}

static void WINAPI wine_glSetFragmentShaderConstantATI( GLuint dst, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", dst, value );
  funcs->ext.p_glSetFragmentShaderConstantATI( dst, value );
}

static void WINAPI wine_glSetInvariantEXT( GLuint id, GLenum type, const GLvoid* addr ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", id, type, addr );
  funcs->ext.p_glSetInvariantEXT( id, type, addr );
}

static void WINAPI wine_glSetLocalConstantEXT( GLuint id, GLenum type, const GLvoid* addr ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", id, type, addr );
  funcs->ext.p_glSetLocalConstantEXT( id, type, addr );
}

static void WINAPI wine_glSetMultisamplefvAMD( GLenum pname, GLuint index, const GLfloat* val ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", pname, index, val );
  funcs->ext.p_glSetMultisamplefvAMD( pname, index, val );
}

static void WINAPI wine_glShaderBinary( GLsizei count, const GLuint* shaders, GLenum binaryformat, const GLvoid* binary, GLsizei length ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p, %d, %p, %d)\n", count, shaders, binaryformat, binary, length );
  funcs->ext.p_glShaderBinary( count, shaders, binaryformat, binary, length );
}

static void WINAPI wine_glShaderOp1EXT( GLenum op, GLuint res, GLuint arg1 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", op, res, arg1 );
  funcs->ext.p_glShaderOp1EXT( op, res, arg1 );
}

static void WINAPI wine_glShaderOp2EXT( GLenum op, GLuint res, GLuint arg1, GLuint arg2 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", op, res, arg1, arg2 );
  funcs->ext.p_glShaderOp2EXT( op, res, arg1, arg2 );
}

static void WINAPI wine_glShaderOp3EXT( GLenum op, GLuint res, GLuint arg1, GLuint arg2, GLuint arg3 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", op, res, arg1, arg2, arg3 );
  funcs->ext.p_glShaderOp3EXT( op, res, arg1, arg2, arg3 );
}

static void WINAPI wine_glShaderSource( GLuint shader, GLsizei count, const char* const* string, const GLint* length ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %p)\n", shader, count, string, length );
  funcs->ext.p_glShaderSource( shader, count, string, length );
}

static void WINAPI wine_glShaderSourceARB( unsigned int shaderObj, GLsizei count, const char** string, const GLint* length ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %p)\n", shaderObj, count, string, length );
  funcs->ext.p_glShaderSourceARB( shaderObj, count, string, length );
}

static void WINAPI wine_glSharpenTexFuncSGIS( GLenum target, GLsizei n, const GLfloat* points ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, n, points );
  funcs->ext.p_glSharpenTexFuncSGIS( target, n, points );
}

static void WINAPI wine_glSpriteParameterfSGIX( GLenum pname, GLfloat param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f)\n", pname, param );
  funcs->ext.p_glSpriteParameterfSGIX( pname, param );
}

static void WINAPI wine_glSpriteParameterfvSGIX( GLenum pname, const GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", pname, params );
  funcs->ext.p_glSpriteParameterfvSGIX( pname, params );
}

static void WINAPI wine_glSpriteParameteriSGIX( GLenum pname, GLint param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", pname, param );
  funcs->ext.p_glSpriteParameteriSGIX( pname, param );
}

static void WINAPI wine_glSpriteParameterivSGIX( GLenum pname, const GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", pname, params );
  funcs->ext.p_glSpriteParameterivSGIX( pname, params );
}

static void WINAPI wine_glStartInstrumentsSGIX( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  funcs->ext.p_glStartInstrumentsSGIX( );
}

static void WINAPI wine_glStencilClearTagEXT( GLsizei stencilTagBits, GLuint stencilClearTag ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", stencilTagBits, stencilClearTag );
  funcs->ext.p_glStencilClearTagEXT( stencilTagBits, stencilClearTag );
}

static void WINAPI wine_glStencilFillPathInstancedNV( GLsizei numPaths, GLenum pathNameType, const GLvoid* paths, GLuint pathBase, GLenum fillMode, GLuint mask, GLenum transformType, const GLfloat* transformValues ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %d, %d, %d, %d, %p)\n", numPaths, pathNameType, paths, pathBase, fillMode, mask, transformType, transformValues );
  funcs->ext.p_glStencilFillPathInstancedNV( numPaths, pathNameType, paths, pathBase, fillMode, mask, transformType, transformValues );
}

static void WINAPI wine_glStencilFillPathNV( GLuint path, GLenum fillMode, GLuint mask ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", path, fillMode, mask );
  funcs->ext.p_glStencilFillPathNV( path, fillMode, mask );
}

static void WINAPI wine_glStencilFuncSeparate( GLenum face, GLenum func, GLint ref, GLuint mask ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", face, func, ref, mask );
  funcs->ext.p_glStencilFuncSeparate( face, func, ref, mask );
}

static void WINAPI wine_glStencilFuncSeparateATI( GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", frontfunc, backfunc, ref, mask );
  funcs->ext.p_glStencilFuncSeparateATI( frontfunc, backfunc, ref, mask );
}

static void WINAPI wine_glStencilMaskSeparate( GLenum face, GLuint mask ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", face, mask );
  funcs->ext.p_glStencilMaskSeparate( face, mask );
}

static void WINAPI wine_glStencilOpSeparate( GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", face, sfail, dpfail, dppass );
  funcs->ext.p_glStencilOpSeparate( face, sfail, dpfail, dppass );
}

static void WINAPI wine_glStencilOpSeparateATI( GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", face, sfail, dpfail, dppass );
  funcs->ext.p_glStencilOpSeparateATI( face, sfail, dpfail, dppass );
}

static void WINAPI wine_glStencilOpValueAMD( GLenum face, GLuint value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", face, value );
  funcs->ext.p_glStencilOpValueAMD( face, value );
}

static void WINAPI wine_glStencilStrokePathInstancedNV( GLsizei numPaths, GLenum pathNameType, const GLvoid* paths, GLuint pathBase, GLint reference, GLuint mask, GLenum transformType, const GLfloat* transformValues ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %d, %d, %d, %d, %p)\n", numPaths, pathNameType, paths, pathBase, reference, mask, transformType, transformValues );
  funcs->ext.p_glStencilStrokePathInstancedNV( numPaths, pathNameType, paths, pathBase, reference, mask, transformType, transformValues );
}

static void WINAPI wine_glStencilStrokePathNV( GLuint path, GLint reference, GLuint mask ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", path, reference, mask );
  funcs->ext.p_glStencilStrokePathNV( path, reference, mask );
}

static void WINAPI wine_glStopInstrumentsSGIX( GLint marker ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", marker );
  funcs->ext.p_glStopInstrumentsSGIX( marker );
}

static void WINAPI wine_glStringMarkerGREMEDY( GLsizei len, const GLvoid* string ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", len, string );
  funcs->ext.p_glStringMarkerGREMEDY( len, string );
}

static void WINAPI wine_glSwizzleEXT( GLuint res, GLuint in, GLenum outX, GLenum outY, GLenum outZ, GLenum outW ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d)\n", res, in, outX, outY, outZ, outW );
  funcs->ext.p_glSwizzleEXT( res, in, outX, outY, outZ, outW );
}

static void WINAPI wine_glTagSampleBufferSGIX( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  funcs->ext.p_glTagSampleBufferSGIX( );
}

static void WINAPI wine_glTangent3bEXT( GLbyte tx, GLbyte ty, GLbyte tz ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", tx, ty, tz );
  funcs->ext.p_glTangent3bEXT( tx, ty, tz );
}

static void WINAPI wine_glTangent3bvEXT( const GLbyte* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glTangent3bvEXT( v );
}

static void WINAPI wine_glTangent3dEXT( GLdouble tx, GLdouble ty, GLdouble tz ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f)\n", tx, ty, tz );
  funcs->ext.p_glTangent3dEXT( tx, ty, tz );
}

static void WINAPI wine_glTangent3dvEXT( const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glTangent3dvEXT( v );
}

static void WINAPI wine_glTangent3fEXT( GLfloat tx, GLfloat ty, GLfloat tz ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f)\n", tx, ty, tz );
  funcs->ext.p_glTangent3fEXT( tx, ty, tz );
}

static void WINAPI wine_glTangent3fvEXT( const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glTangent3fvEXT( v );
}

static void WINAPI wine_glTangent3iEXT( GLint tx, GLint ty, GLint tz ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", tx, ty, tz );
  funcs->ext.p_glTangent3iEXT( tx, ty, tz );
}

static void WINAPI wine_glTangent3ivEXT( const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glTangent3ivEXT( v );
}

static void WINAPI wine_glTangent3sEXT( GLshort tx, GLshort ty, GLshort tz ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", tx, ty, tz );
  funcs->ext.p_glTangent3sEXT( tx, ty, tz );
}

static void WINAPI wine_glTangent3svEXT( const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glTangent3svEXT( v );
}

static void WINAPI wine_glTangentPointerEXT( GLenum type, GLsizei stride, const GLvoid* pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", type, stride, pointer );
  funcs->ext.p_glTangentPointerEXT( type, stride, pointer );
}

static void WINAPI wine_glTbufferMask3DFX( GLuint mask ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", mask );
  funcs->ext.p_glTbufferMask3DFX( mask );
}

static void WINAPI wine_glTessellationFactorAMD( GLfloat factor ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f)\n", factor );
  funcs->ext.p_glTessellationFactorAMD( factor );
}

static void WINAPI wine_glTessellationModeAMD( GLenum mode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", mode );
  funcs->ext.p_glTessellationModeAMD( mode );
}

static GLboolean WINAPI wine_glTestFenceAPPLE( GLuint fence ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", fence );
  return funcs->ext.p_glTestFenceAPPLE( fence );
}

static GLboolean WINAPI wine_glTestFenceNV( GLuint fence ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", fence );
  return funcs->ext.p_glTestFenceNV( fence );
}

static GLboolean WINAPI wine_glTestObjectAPPLE( GLenum object, GLuint name ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", object, name );
  return funcs->ext.p_glTestObjectAPPLE( object, name );
}

static void WINAPI wine_glTexBuffer( GLenum target, GLenum internalformat, GLuint buffer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", target, internalformat, buffer );
  funcs->ext.p_glTexBuffer( target, internalformat, buffer );
}

static void WINAPI wine_glTexBufferARB( GLenum target, GLenum internalformat, GLuint buffer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", target, internalformat, buffer );
  funcs->ext.p_glTexBufferARB( target, internalformat, buffer );
}

static void WINAPI wine_glTexBufferEXT( GLenum target, GLenum internalformat, GLuint buffer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", target, internalformat, buffer );
  funcs->ext.p_glTexBufferEXT( target, internalformat, buffer );
}

static void WINAPI wine_glTexBumpParameterfvATI( GLenum pname, const GLfloat* param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", pname, param );
  funcs->ext.p_glTexBumpParameterfvATI( pname, param );
}

static void WINAPI wine_glTexBumpParameterivATI( GLenum pname, const GLint* param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", pname, param );
  funcs->ext.p_glTexBumpParameterivATI( pname, param );
}

static void WINAPI wine_glTexCoord1hNV( unsigned short s ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", s );
  funcs->ext.p_glTexCoord1hNV( s );
}

static void WINAPI wine_glTexCoord1hvNV( const unsigned short* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glTexCoord1hvNV( v );
}

static void WINAPI wine_glTexCoord2fColor3fVertex3fSUN( GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f, %f, %f, %f, %f, %f)\n", s, t, r, g, b, x, y, z );
  funcs->ext.p_glTexCoord2fColor3fVertex3fSUN( s, t, r, g, b, x, y, z );
}

static void WINAPI wine_glTexCoord2fColor3fVertex3fvSUN( const GLfloat* tc, const GLfloat* c, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p, %p, %p)\n", tc, c, v );
  funcs->ext.p_glTexCoord2fColor3fVertex3fvSUN( tc, c, v );
}

static void WINAPI wine_glTexCoord2fColor4fNormal3fVertex3fSUN( GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f)\n", s, t, r, g, b, a, nx, ny, nz, x, y, z );
  funcs->ext.p_glTexCoord2fColor4fNormal3fVertex3fSUN( s, t, r, g, b, a, nx, ny, nz, x, y, z );
}

static void WINAPI wine_glTexCoord2fColor4fNormal3fVertex3fvSUN( const GLfloat* tc, const GLfloat* c, const GLfloat* n, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p, %p, %p, %p)\n", tc, c, n, v );
  funcs->ext.p_glTexCoord2fColor4fNormal3fVertex3fvSUN( tc, c, n, v );
}

static void WINAPI wine_glTexCoord2fColor4ubVertex3fSUN( GLfloat s, GLfloat t, GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %d, %d, %d, %d, %f, %f, %f)\n", s, t, r, g, b, a, x, y, z );
  funcs->ext.p_glTexCoord2fColor4ubVertex3fSUN( s, t, r, g, b, a, x, y, z );
}

static void WINAPI wine_glTexCoord2fColor4ubVertex3fvSUN( const GLfloat* tc, const GLubyte* c, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p, %p, %p)\n", tc, c, v );
  funcs->ext.p_glTexCoord2fColor4ubVertex3fvSUN( tc, c, v );
}

static void WINAPI wine_glTexCoord2fNormal3fVertex3fSUN( GLfloat s, GLfloat t, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f, %f, %f, %f, %f, %f)\n", s, t, nx, ny, nz, x, y, z );
  funcs->ext.p_glTexCoord2fNormal3fVertex3fSUN( s, t, nx, ny, nz, x, y, z );
}

static void WINAPI wine_glTexCoord2fNormal3fVertex3fvSUN( const GLfloat* tc, const GLfloat* n, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p, %p, %p)\n", tc, n, v );
  funcs->ext.p_glTexCoord2fNormal3fVertex3fvSUN( tc, n, v );
}

static void WINAPI wine_glTexCoord2fVertex3fSUN( GLfloat s, GLfloat t, GLfloat x, GLfloat y, GLfloat z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f, %f, %f)\n", s, t, x, y, z );
  funcs->ext.p_glTexCoord2fVertex3fSUN( s, t, x, y, z );
}

static void WINAPI wine_glTexCoord2fVertex3fvSUN( const GLfloat* tc, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p, %p)\n", tc, v );
  funcs->ext.p_glTexCoord2fVertex3fvSUN( tc, v );
}

static void WINAPI wine_glTexCoord2hNV( unsigned short s, unsigned short t ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", s, t );
  funcs->ext.p_glTexCoord2hNV( s, t );
}

static void WINAPI wine_glTexCoord2hvNV( const unsigned short* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glTexCoord2hvNV( v );
}

static void WINAPI wine_glTexCoord3hNV( unsigned short s, unsigned short t, unsigned short r ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", s, t, r );
  funcs->ext.p_glTexCoord3hNV( s, t, r );
}

static void WINAPI wine_glTexCoord3hvNV( const unsigned short* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glTexCoord3hvNV( v );
}

static void WINAPI wine_glTexCoord4fColor4fNormal3fVertex4fSUN( GLfloat s, GLfloat t, GLfloat p, GLfloat q, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f)\n", s, t, p, q, r, g, b, a, nx, ny, nz, x, y, z, w );
  funcs->ext.p_glTexCoord4fColor4fNormal3fVertex4fSUN( s, t, p, q, r, g, b, a, nx, ny, nz, x, y, z, w );
}

static void WINAPI wine_glTexCoord4fColor4fNormal3fVertex4fvSUN( const GLfloat* tc, const GLfloat* c, const GLfloat* n, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p, %p, %p, %p)\n", tc, c, n, v );
  funcs->ext.p_glTexCoord4fColor4fNormal3fVertex4fvSUN( tc, c, n, v );
}

static void WINAPI wine_glTexCoord4fVertex4fSUN( GLfloat s, GLfloat t, GLfloat p, GLfloat q, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f, %f, %f, %f, %f, %f)\n", s, t, p, q, x, y, z, w );
  funcs->ext.p_glTexCoord4fVertex4fSUN( s, t, p, q, x, y, z, w );
}

static void WINAPI wine_glTexCoord4fVertex4fvSUN( const GLfloat* tc, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p, %p)\n", tc, v );
  funcs->ext.p_glTexCoord4fVertex4fvSUN( tc, v );
}

static void WINAPI wine_glTexCoord4hNV( unsigned short s, unsigned short t, unsigned short r, unsigned short q ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", s, t, r, q );
  funcs->ext.p_glTexCoord4hNV( s, t, r, q );
}

static void WINAPI wine_glTexCoord4hvNV( const unsigned short* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glTexCoord4hvNV( v );
}

static void WINAPI wine_glTexCoordFormatNV( GLint size, GLenum type, GLsizei stride ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", size, type, stride );
  funcs->ext.p_glTexCoordFormatNV( size, type, stride );
}

static void WINAPI wine_glTexCoordP1ui( GLenum type, GLuint coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", type, coords );
  funcs->ext.p_glTexCoordP1ui( type, coords );
}

static void WINAPI wine_glTexCoordP1uiv( GLenum type, const GLuint* coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", type, coords );
  funcs->ext.p_glTexCoordP1uiv( type, coords );
}

static void WINAPI wine_glTexCoordP2ui( GLenum type, GLuint coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", type, coords );
  funcs->ext.p_glTexCoordP2ui( type, coords );
}

static void WINAPI wine_glTexCoordP2uiv( GLenum type, const GLuint* coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", type, coords );
  funcs->ext.p_glTexCoordP2uiv( type, coords );
}

static void WINAPI wine_glTexCoordP3ui( GLenum type, GLuint coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", type, coords );
  funcs->ext.p_glTexCoordP3ui( type, coords );
}

static void WINAPI wine_glTexCoordP3uiv( GLenum type, const GLuint* coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", type, coords );
  funcs->ext.p_glTexCoordP3uiv( type, coords );
}

static void WINAPI wine_glTexCoordP4ui( GLenum type, GLuint coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", type, coords );
  funcs->ext.p_glTexCoordP4ui( type, coords );
}

static void WINAPI wine_glTexCoordP4uiv( GLenum type, const GLuint* coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", type, coords );
  funcs->ext.p_glTexCoordP4uiv( type, coords );
}

static void WINAPI wine_glTexCoordPointerEXT( GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid* pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", size, type, stride, count, pointer );
  funcs->ext.p_glTexCoordPointerEXT( size, type, stride, count, pointer );
}

static void WINAPI wine_glTexCoordPointerListIBM( GLint size, GLenum type, GLint stride, const GLvoid** pointer, GLint ptrstride ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p, %d)\n", size, type, stride, pointer, ptrstride );
  funcs->ext.p_glTexCoordPointerListIBM( size, type, stride, pointer, ptrstride );
}

static void WINAPI wine_glTexCoordPointervINTEL( GLint size, GLenum type, const GLvoid** pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", size, type, pointer );
  funcs->ext.p_glTexCoordPointervINTEL( size, type, pointer );
}

static void WINAPI wine_glTexFilterFuncSGIS( GLenum target, GLenum filter, GLsizei n, const GLfloat* weights ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", target, filter, n, weights );
  funcs->ext.p_glTexFilterFuncSGIS( target, filter, n, weights );
}

static void WINAPI wine_glTexImage2DMultisample( GLenum target, GLsizei samples, GLint internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d)\n", target, samples, internalformat, width, height, fixedsamplelocations );
  funcs->ext.p_glTexImage2DMultisample( target, samples, internalformat, width, height, fixedsamplelocations );
}

static void WINAPI wine_glTexImage2DMultisampleCoverageNV( GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLint internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d)\n", target, coverageSamples, colorSamples, internalFormat, width, height, fixedSampleLocations );
  funcs->ext.p_glTexImage2DMultisampleCoverageNV( target, coverageSamples, colorSamples, internalFormat, width, height, fixedSampleLocations );
}

static void WINAPI wine_glTexImage3D( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* pixels ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, height, depth, border, format, type, pixels );
  funcs->ext.p_glTexImage3D( target, level, internalformat, width, height, depth, border, format, type, pixels );
}

static void WINAPI wine_glTexImage3DEXT( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* pixels ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, height, depth, border, format, type, pixels );
  funcs->ext.p_glTexImage3DEXT( target, level, internalformat, width, height, depth, border, format, type, pixels );
}

static void WINAPI wine_glTexImage3DMultisample( GLenum target, GLsizei samples, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d)\n", target, samples, internalformat, width, height, depth, fixedsamplelocations );
  funcs->ext.p_glTexImage3DMultisample( target, samples, internalformat, width, height, depth, fixedsamplelocations );
}

static void WINAPI wine_glTexImage3DMultisampleCoverageNV( GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d)\n", target, coverageSamples, colorSamples, internalFormat, width, height, depth, fixedSampleLocations );
  funcs->ext.p_glTexImage3DMultisampleCoverageNV( target, coverageSamples, colorSamples, internalFormat, width, height, depth, fixedSampleLocations );
}

static void WINAPI wine_glTexImage4DSGIS( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLint border, GLenum format, GLenum type, const GLvoid* pixels ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, height, depth, size4d, border, format, type, pixels );
  funcs->ext.p_glTexImage4DSGIS( target, level, internalformat, width, height, depth, size4d, border, format, type, pixels );
}

static void WINAPI wine_glTexParameterIiv( GLenum target, GLenum pname, const GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glTexParameterIiv( target, pname, params );
}

static void WINAPI wine_glTexParameterIivEXT( GLenum target, GLenum pname, const GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glTexParameterIivEXT( target, pname, params );
}

static void WINAPI wine_glTexParameterIuiv( GLenum target, GLenum pname, const GLuint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glTexParameterIuiv( target, pname, params );
}

static void WINAPI wine_glTexParameterIuivEXT( GLenum target, GLenum pname, const GLuint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glTexParameterIuivEXT( target, pname, params );
}

static void WINAPI wine_glTexRenderbufferNV( GLenum target, GLuint renderbuffer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", target, renderbuffer );
  funcs->ext.p_glTexRenderbufferNV( target, renderbuffer );
}

static void WINAPI wine_glTexStorage1D( GLenum target, GLsizei levels, GLenum internalformat, GLsizei width ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", target, levels, internalformat, width );
  funcs->ext.p_glTexStorage1D( target, levels, internalformat, width );
}

static void WINAPI wine_glTexStorage2D( GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", target, levels, internalformat, width, height );
  funcs->ext.p_glTexStorage2D( target, levels, internalformat, width, height );
}

static void WINAPI wine_glTexStorage3D( GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d)\n", target, levels, internalformat, width, height, depth );
  funcs->ext.p_glTexStorage3D( target, levels, internalformat, width, height, depth );
}

static void WINAPI wine_glTexSubImage1DEXT( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid* pixels ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, width, format, type, pixels );
  funcs->ext.p_glTexSubImage1DEXT( target, level, xoffset, width, format, type, pixels );
}

static void WINAPI wine_glTexSubImage2DEXT( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, width, height, format, type, pixels );
  funcs->ext.p_glTexSubImage2DEXT( target, level, xoffset, yoffset, width, height, format, type, pixels );
}

static void WINAPI wine_glTexSubImage3D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid* pixels ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels );
  funcs->ext.p_glTexSubImage3D( target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels );
}

static void WINAPI wine_glTexSubImage3DEXT( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid* pixels ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels );
  funcs->ext.p_glTexSubImage3DEXT( target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels );
}

static void WINAPI wine_glTexSubImage4DSGIS( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint woffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLenum format, GLenum type, const GLvoid* pixels ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, zoffset, woffset, width, height, depth, size4d, format, type, pixels );
  funcs->ext.p_glTexSubImage4DSGIS( target, level, xoffset, yoffset, zoffset, woffset, width, height, depth, size4d, format, type, pixels );
}

static void WINAPI wine_glTextureBarrierNV( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  funcs->ext.p_glTextureBarrierNV( );
}

static void WINAPI wine_glTextureBufferEXT( GLuint texture, GLenum target, GLenum internalformat, GLuint buffer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", texture, target, internalformat, buffer );
  funcs->ext.p_glTextureBufferEXT( texture, target, internalformat, buffer );
}

static void WINAPI wine_glTextureColorMaskSGIS( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", red, green, blue, alpha );
  funcs->ext.p_glTextureColorMaskSGIS( red, green, blue, alpha );
}

static void WINAPI wine_glTextureImage1DEXT( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid* pixels ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texture, target, level, internalformat, width, border, format, type, pixels );
  funcs->ext.p_glTextureImage1DEXT( texture, target, level, internalformat, width, border, format, type, pixels );
}

static void WINAPI wine_glTextureImage2DEXT( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texture, target, level, internalformat, width, height, border, format, type, pixels );
  funcs->ext.p_glTextureImage2DEXT( texture, target, level, internalformat, width, height, border, format, type, pixels );
}

static void WINAPI wine_glTextureImage2DMultisampleCoverageNV( GLuint texture, GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLint internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d)\n", texture, target, coverageSamples, colorSamples, internalFormat, width, height, fixedSampleLocations );
  funcs->ext.p_glTextureImage2DMultisampleCoverageNV( texture, target, coverageSamples, colorSamples, internalFormat, width, height, fixedSampleLocations );
}

static void WINAPI wine_glTextureImage2DMultisampleNV( GLuint texture, GLenum target, GLsizei samples, GLint internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d)\n", texture, target, samples, internalFormat, width, height, fixedSampleLocations );
  funcs->ext.p_glTextureImage2DMultisampleNV( texture, target, samples, internalFormat, width, height, fixedSampleLocations );
}

static void WINAPI wine_glTextureImage3DEXT( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* pixels ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texture, target, level, internalformat, width, height, depth, border, format, type, pixels );
  funcs->ext.p_glTextureImage3DEXT( texture, target, level, internalformat, width, height, depth, border, format, type, pixels );
}

static void WINAPI wine_glTextureImage3DMultisampleCoverageNV( GLuint texture, GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d)\n", texture, target, coverageSamples, colorSamples, internalFormat, width, height, depth, fixedSampleLocations );
  funcs->ext.p_glTextureImage3DMultisampleCoverageNV( texture, target, coverageSamples, colorSamples, internalFormat, width, height, depth, fixedSampleLocations );
}

static void WINAPI wine_glTextureImage3DMultisampleNV( GLuint texture, GLenum target, GLsizei samples, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d)\n", texture, target, samples, internalFormat, width, height, depth, fixedSampleLocations );
  funcs->ext.p_glTextureImage3DMultisampleNV( texture, target, samples, internalFormat, width, height, depth, fixedSampleLocations );
}

static void WINAPI wine_glTextureLightEXT( GLenum pname ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", pname );
  funcs->ext.p_glTextureLightEXT( pname );
}

static void WINAPI wine_glTextureMaterialEXT( GLenum face, GLenum mode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", face, mode );
  funcs->ext.p_glTextureMaterialEXT( face, mode );
}

static void WINAPI wine_glTextureNormalEXT( GLenum mode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", mode );
  funcs->ext.p_glTextureNormalEXT( mode );
}

static void WINAPI wine_glTextureParameterIivEXT( GLuint texture, GLenum target, GLenum pname, const GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", texture, target, pname, params );
  funcs->ext.p_glTextureParameterIivEXT( texture, target, pname, params );
}

static void WINAPI wine_glTextureParameterIuivEXT( GLuint texture, GLenum target, GLenum pname, const GLuint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", texture, target, pname, params );
  funcs->ext.p_glTextureParameterIuivEXT( texture, target, pname, params );
}

static void WINAPI wine_glTextureParameterfEXT( GLuint texture, GLenum target, GLenum pname, GLfloat param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %f)\n", texture, target, pname, param );
  funcs->ext.p_glTextureParameterfEXT( texture, target, pname, param );
}

static void WINAPI wine_glTextureParameterfvEXT( GLuint texture, GLenum target, GLenum pname, const GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", texture, target, pname, params );
  funcs->ext.p_glTextureParameterfvEXT( texture, target, pname, params );
}

static void WINAPI wine_glTextureParameteriEXT( GLuint texture, GLenum target, GLenum pname, GLint param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", texture, target, pname, param );
  funcs->ext.p_glTextureParameteriEXT( texture, target, pname, param );
}

static void WINAPI wine_glTextureParameterivEXT( GLuint texture, GLenum target, GLenum pname, const GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", texture, target, pname, params );
  funcs->ext.p_glTextureParameterivEXT( texture, target, pname, params );
}

static void WINAPI wine_glTextureRangeAPPLE( GLenum target, GLsizei length, const GLvoid* pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", target, length, pointer );
  funcs->ext.p_glTextureRangeAPPLE( target, length, pointer );
}

static void WINAPI wine_glTextureRenderbufferEXT( GLuint texture, GLenum target, GLuint renderbuffer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", texture, target, renderbuffer );
  funcs->ext.p_glTextureRenderbufferEXT( texture, target, renderbuffer );
}

static void WINAPI wine_glTextureStorage1DEXT( GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", texture, target, levels, internalformat, width );
  funcs->ext.p_glTextureStorage1DEXT( texture, target, levels, internalformat, width );
}

static void WINAPI wine_glTextureStorage2DEXT( GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d)\n", texture, target, levels, internalformat, width, height );
  funcs->ext.p_glTextureStorage2DEXT( texture, target, levels, internalformat, width, height );
}

static void WINAPI wine_glTextureStorage3DEXT( GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d)\n", texture, target, levels, internalformat, width, height, depth );
  funcs->ext.p_glTextureStorage3DEXT( texture, target, levels, internalformat, width, height, depth );
}

static void WINAPI wine_glTextureSubImage1DEXT( GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid* pixels ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %p)\n", texture, target, level, xoffset, width, format, type, pixels );
  funcs->ext.p_glTextureSubImage1DEXT( texture, target, level, xoffset, width, format, type, pixels );
}

static void WINAPI wine_glTextureSubImage2DEXT( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texture, target, level, xoffset, yoffset, width, height, format, type, pixels );
  funcs->ext.p_glTextureSubImage2DEXT( texture, target, level, xoffset, yoffset, width, height, format, type, pixels );
}

static void WINAPI wine_glTextureSubImage3DEXT( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid* pixels ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texture, target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels );
  funcs->ext.p_glTextureSubImage3DEXT( texture, target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels );
}

static void WINAPI wine_glTrackMatrixNV( GLenum target, GLuint address, GLenum matrix, GLenum transform ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", target, address, matrix, transform );
  funcs->ext.p_glTrackMatrixNV( target, address, matrix, transform );
}

static void WINAPI wine_glTransformFeedbackAttribsNV( GLuint count, const GLint* attribs, GLenum bufferMode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p, %d)\n", count, attribs, bufferMode );
  funcs->ext.p_glTransformFeedbackAttribsNV( count, attribs, bufferMode );
}

static void WINAPI wine_glTransformFeedbackStreamAttribsNV( GLsizei count, const GLint* attribs, GLsizei nbuffers, const GLint* bufstreams, GLenum bufferMode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p, %d, %p, %d)\n", count, attribs, nbuffers, bufstreams, bufferMode );
  funcs->ext.p_glTransformFeedbackStreamAttribsNV( count, attribs, nbuffers, bufstreams, bufferMode );
}

static void WINAPI wine_glTransformFeedbackVaryings( GLuint program, GLsizei count, const char* const* varyings, GLenum bufferMode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %d)\n", program, count, varyings, bufferMode );
  funcs->ext.p_glTransformFeedbackVaryings( program, count, varyings, bufferMode );
}

static void WINAPI wine_glTransformFeedbackVaryingsEXT( GLuint program, GLsizei count, const char** varyings, GLenum bufferMode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %d)\n", program, count, varyings, bufferMode );
  funcs->ext.p_glTransformFeedbackVaryingsEXT( program, count, varyings, bufferMode );
}

static void WINAPI wine_glTransformFeedbackVaryingsNV( GLuint program, GLsizei count, const GLint* locations, GLenum bufferMode ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %d)\n", program, count, locations, bufferMode );
  funcs->ext.p_glTransformFeedbackVaryingsNV( program, count, locations, bufferMode );
}

static void WINAPI wine_glTransformPathNV( GLuint resultPath, GLuint srcPath, GLenum transformType, const GLfloat* transformValues ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", resultPath, srcPath, transformType, transformValues );
  funcs->ext.p_glTransformPathNV( resultPath, srcPath, transformType, transformValues );
}

static void WINAPI wine_glUniform1d( GLint location, GLdouble x ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f)\n", location, x );
  funcs->ext.p_glUniform1d( location, x );
}

static void WINAPI wine_glUniform1dv( GLint location, GLsizei count, const GLdouble* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform1dv( location, count, value );
}

static void WINAPI wine_glUniform1f( GLint location, GLfloat v0 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f)\n", location, v0 );
  funcs->ext.p_glUniform1f( location, v0 );
}

static void WINAPI wine_glUniform1fARB( GLint location, GLfloat v0 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f)\n", location, v0 );
  funcs->ext.p_glUniform1fARB( location, v0 );
}

static void WINAPI wine_glUniform1fv( GLint location, GLsizei count, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform1fv( location, count, value );
}

static void WINAPI wine_glUniform1fvARB( GLint location, GLsizei count, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform1fvARB( location, count, value );
}

static void WINAPI wine_glUniform1i( GLint location, GLint v0 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", location, v0 );
  funcs->ext.p_glUniform1i( location, v0 );
}

static void WINAPI wine_glUniform1i64NV( GLint location, INT64 x ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %s)\n", location, wine_dbgstr_longlong(x) );
  funcs->ext.p_glUniform1i64NV( location, x );
}

static void WINAPI wine_glUniform1i64vNV( GLint location, GLsizei count, const INT64* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform1i64vNV( location, count, value );
}

static void WINAPI wine_glUniform1iARB( GLint location, GLint v0 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", location, v0 );
  funcs->ext.p_glUniform1iARB( location, v0 );
}

static void WINAPI wine_glUniform1iv( GLint location, GLsizei count, const GLint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform1iv( location, count, value );
}

static void WINAPI wine_glUniform1ivARB( GLint location, GLsizei count, const GLint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform1ivARB( location, count, value );
}

static void WINAPI wine_glUniform1ui( GLint location, GLuint v0 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", location, v0 );
  funcs->ext.p_glUniform1ui( location, v0 );
}

static void WINAPI wine_glUniform1ui64NV( GLint location, UINT64 x ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %s)\n", location, wine_dbgstr_longlong(x) );
  funcs->ext.p_glUniform1ui64NV( location, x );
}

static void WINAPI wine_glUniform1ui64vNV( GLint location, GLsizei count, const UINT64* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform1ui64vNV( location, count, value );
}

static void WINAPI wine_glUniform1uiEXT( GLint location, GLuint v0 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", location, v0 );
  funcs->ext.p_glUniform1uiEXT( location, v0 );
}

static void WINAPI wine_glUniform1uiv( GLint location, GLsizei count, const GLuint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform1uiv( location, count, value );
}

static void WINAPI wine_glUniform1uivEXT( GLint location, GLsizei count, const GLuint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform1uivEXT( location, count, value );
}

static void WINAPI wine_glUniform2d( GLint location, GLdouble x, GLdouble y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f)\n", location, x, y );
  funcs->ext.p_glUniform2d( location, x, y );
}

static void WINAPI wine_glUniform2dv( GLint location, GLsizei count, const GLdouble* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform2dv( location, count, value );
}

static void WINAPI wine_glUniform2f( GLint location, GLfloat v0, GLfloat v1 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f)\n", location, v0, v1 );
  funcs->ext.p_glUniform2f( location, v0, v1 );
}

static void WINAPI wine_glUniform2fARB( GLint location, GLfloat v0, GLfloat v1 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f)\n", location, v0, v1 );
  funcs->ext.p_glUniform2fARB( location, v0, v1 );
}

static void WINAPI wine_glUniform2fv( GLint location, GLsizei count, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform2fv( location, count, value );
}

static void WINAPI wine_glUniform2fvARB( GLint location, GLsizei count, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform2fvARB( location, count, value );
}

static void WINAPI wine_glUniform2i( GLint location, GLint v0, GLint v1 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", location, v0, v1 );
  funcs->ext.p_glUniform2i( location, v0, v1 );
}

static void WINAPI wine_glUniform2i64NV( GLint location, INT64 x, INT64 y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %s, %s)\n", location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y) );
  funcs->ext.p_glUniform2i64NV( location, x, y );
}

static void WINAPI wine_glUniform2i64vNV( GLint location, GLsizei count, const INT64* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform2i64vNV( location, count, value );
}

static void WINAPI wine_glUniform2iARB( GLint location, GLint v0, GLint v1 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", location, v0, v1 );
  funcs->ext.p_glUniform2iARB( location, v0, v1 );
}

static void WINAPI wine_glUniform2iv( GLint location, GLsizei count, const GLint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform2iv( location, count, value );
}

static void WINAPI wine_glUniform2ivARB( GLint location, GLsizei count, const GLint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform2ivARB( location, count, value );
}

static void WINAPI wine_glUniform2ui( GLint location, GLuint v0, GLuint v1 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", location, v0, v1 );
  funcs->ext.p_glUniform2ui( location, v0, v1 );
}

static void WINAPI wine_glUniform2ui64NV( GLint location, UINT64 x, UINT64 y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %s, %s)\n", location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y) );
  funcs->ext.p_glUniform2ui64NV( location, x, y );
}

static void WINAPI wine_glUniform2ui64vNV( GLint location, GLsizei count, const UINT64* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform2ui64vNV( location, count, value );
}

static void WINAPI wine_glUniform2uiEXT( GLint location, GLuint v0, GLuint v1 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", location, v0, v1 );
  funcs->ext.p_glUniform2uiEXT( location, v0, v1 );
}

static void WINAPI wine_glUniform2uiv( GLint location, GLsizei count, const GLuint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform2uiv( location, count, value );
}

static void WINAPI wine_glUniform2uivEXT( GLint location, GLsizei count, const GLuint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform2uivEXT( location, count, value );
}

static void WINAPI wine_glUniform3d( GLint location, GLdouble x, GLdouble y, GLdouble z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f)\n", location, x, y, z );
  funcs->ext.p_glUniform3d( location, x, y, z );
}

static void WINAPI wine_glUniform3dv( GLint location, GLsizei count, const GLdouble* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform3dv( location, count, value );
}

static void WINAPI wine_glUniform3f( GLint location, GLfloat v0, GLfloat v1, GLfloat v2 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f)\n", location, v0, v1, v2 );
  funcs->ext.p_glUniform3f( location, v0, v1, v2 );
}

static void WINAPI wine_glUniform3fARB( GLint location, GLfloat v0, GLfloat v1, GLfloat v2 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f)\n", location, v0, v1, v2 );
  funcs->ext.p_glUniform3fARB( location, v0, v1, v2 );
}

static void WINAPI wine_glUniform3fv( GLint location, GLsizei count, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform3fv( location, count, value );
}

static void WINAPI wine_glUniform3fvARB( GLint location, GLsizei count, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform3fvARB( location, count, value );
}

static void WINAPI wine_glUniform3i( GLint location, GLint v0, GLint v1, GLint v2 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", location, v0, v1, v2 );
  funcs->ext.p_glUniform3i( location, v0, v1, v2 );
}

static void WINAPI wine_glUniform3i64NV( GLint location, INT64 x, INT64 y, INT64 z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %s, %s, %s)\n", location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z) );
  funcs->ext.p_glUniform3i64NV( location, x, y, z );
}

static void WINAPI wine_glUniform3i64vNV( GLint location, GLsizei count, const INT64* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform3i64vNV( location, count, value );
}

static void WINAPI wine_glUniform3iARB( GLint location, GLint v0, GLint v1, GLint v2 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", location, v0, v1, v2 );
  funcs->ext.p_glUniform3iARB( location, v0, v1, v2 );
}

static void WINAPI wine_glUniform3iv( GLint location, GLsizei count, const GLint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform3iv( location, count, value );
}

static void WINAPI wine_glUniform3ivARB( GLint location, GLsizei count, const GLint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform3ivARB( location, count, value );
}

static void WINAPI wine_glUniform3ui( GLint location, GLuint v0, GLuint v1, GLuint v2 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", location, v0, v1, v2 );
  funcs->ext.p_glUniform3ui( location, v0, v1, v2 );
}

static void WINAPI wine_glUniform3ui64NV( GLint location, UINT64 x, UINT64 y, UINT64 z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %s, %s, %s)\n", location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z) );
  funcs->ext.p_glUniform3ui64NV( location, x, y, z );
}

static void WINAPI wine_glUniform3ui64vNV( GLint location, GLsizei count, const UINT64* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform3ui64vNV( location, count, value );
}

static void WINAPI wine_glUniform3uiEXT( GLint location, GLuint v0, GLuint v1, GLuint v2 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", location, v0, v1, v2 );
  funcs->ext.p_glUniform3uiEXT( location, v0, v1, v2 );
}

static void WINAPI wine_glUniform3uiv( GLint location, GLsizei count, const GLuint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform3uiv( location, count, value );
}

static void WINAPI wine_glUniform3uivEXT( GLint location, GLsizei count, const GLuint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform3uivEXT( location, count, value );
}

static void WINAPI wine_glUniform4d( GLint location, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f, %f)\n", location, x, y, z, w );
  funcs->ext.p_glUniform4d( location, x, y, z, w );
}

static void WINAPI wine_glUniform4dv( GLint location, GLsizei count, const GLdouble* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform4dv( location, count, value );
}

static void WINAPI wine_glUniform4f( GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f, %f)\n", location, v0, v1, v2, v3 );
  funcs->ext.p_glUniform4f( location, v0, v1, v2, v3 );
}

static void WINAPI wine_glUniform4fARB( GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f, %f)\n", location, v0, v1, v2, v3 );
  funcs->ext.p_glUniform4fARB( location, v0, v1, v2, v3 );
}

static void WINAPI wine_glUniform4fv( GLint location, GLsizei count, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform4fv( location, count, value );
}

static void WINAPI wine_glUniform4fvARB( GLint location, GLsizei count, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform4fvARB( location, count, value );
}

static void WINAPI wine_glUniform4i( GLint location, GLint v0, GLint v1, GLint v2, GLint v3 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", location, v0, v1, v2, v3 );
  funcs->ext.p_glUniform4i( location, v0, v1, v2, v3 );
}

static void WINAPI wine_glUniform4i64NV( GLint location, INT64 x, INT64 y, INT64 z, INT64 w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %s, %s, %s, %s)\n", location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z), wine_dbgstr_longlong(w) );
  funcs->ext.p_glUniform4i64NV( location, x, y, z, w );
}

static void WINAPI wine_glUniform4i64vNV( GLint location, GLsizei count, const INT64* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform4i64vNV( location, count, value );
}

static void WINAPI wine_glUniform4iARB( GLint location, GLint v0, GLint v1, GLint v2, GLint v3 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", location, v0, v1, v2, v3 );
  funcs->ext.p_glUniform4iARB( location, v0, v1, v2, v3 );
}

static void WINAPI wine_glUniform4iv( GLint location, GLsizei count, const GLint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform4iv( location, count, value );
}

static void WINAPI wine_glUniform4ivARB( GLint location, GLsizei count, const GLint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform4ivARB( location, count, value );
}

static void WINAPI wine_glUniform4ui( GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", location, v0, v1, v2, v3 );
  funcs->ext.p_glUniform4ui( location, v0, v1, v2, v3 );
}

static void WINAPI wine_glUniform4ui64NV( GLint location, UINT64 x, UINT64 y, UINT64 z, UINT64 w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %s, %s, %s, %s)\n", location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z), wine_dbgstr_longlong(w) );
  funcs->ext.p_glUniform4ui64NV( location, x, y, z, w );
}

static void WINAPI wine_glUniform4ui64vNV( GLint location, GLsizei count, const UINT64* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform4ui64vNV( location, count, value );
}

static void WINAPI wine_glUniform4uiEXT( GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3 ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", location, v0, v1, v2, v3 );
  funcs->ext.p_glUniform4uiEXT( location, v0, v1, v2, v3 );
}

static void WINAPI wine_glUniform4uiv( GLint location, GLsizei count, const GLuint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform4uiv( location, count, value );
}

static void WINAPI wine_glUniform4uivEXT( GLint location, GLsizei count, const GLuint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform4uivEXT( location, count, value );
}

static void WINAPI wine_glUniformBlockBinding( GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", program, uniformBlockIndex, uniformBlockBinding );
  funcs->ext.p_glUniformBlockBinding( program, uniformBlockIndex, uniformBlockBinding );
}

static void WINAPI wine_glUniformBufferEXT( GLuint program, GLint location, GLuint buffer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", program, location, buffer );
  funcs->ext.p_glUniformBufferEXT( program, location, buffer );
}

static void WINAPI wine_glUniformHandleui64NV( GLint location, UINT64 value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %s)\n", location, wine_dbgstr_longlong(value) );
  funcs->ext.p_glUniformHandleui64NV( location, value );
}

static void WINAPI wine_glUniformHandleui64vNV( GLint location, GLsizei count, const UINT64* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniformHandleui64vNV( location, count, value );
}

static void WINAPI wine_glUniformMatrix2dv( GLint location, GLsizei count, GLboolean transpose, const GLdouble* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix2dv( location, count, transpose, value );
}

static void WINAPI wine_glUniformMatrix2fv( GLint location, GLsizei count, GLboolean transpose, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix2fv( location, count, transpose, value );
}

static void WINAPI wine_glUniformMatrix2fvARB( GLint location, GLsizei count, GLboolean transpose, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix2fvARB( location, count, transpose, value );
}

static void WINAPI wine_glUniformMatrix2x3dv( GLint location, GLsizei count, GLboolean transpose, const GLdouble* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix2x3dv( location, count, transpose, value );
}

static void WINAPI wine_glUniformMatrix2x3fv( GLint location, GLsizei count, GLboolean transpose, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix2x3fv( location, count, transpose, value );
}

static void WINAPI wine_glUniformMatrix2x4dv( GLint location, GLsizei count, GLboolean transpose, const GLdouble* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix2x4dv( location, count, transpose, value );
}

static void WINAPI wine_glUniformMatrix2x4fv( GLint location, GLsizei count, GLboolean transpose, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix2x4fv( location, count, transpose, value );
}

static void WINAPI wine_glUniformMatrix3dv( GLint location, GLsizei count, GLboolean transpose, const GLdouble* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix3dv( location, count, transpose, value );
}

static void WINAPI wine_glUniformMatrix3fv( GLint location, GLsizei count, GLboolean transpose, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix3fv( location, count, transpose, value );
}

static void WINAPI wine_glUniformMatrix3fvARB( GLint location, GLsizei count, GLboolean transpose, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix3fvARB( location, count, transpose, value );
}

static void WINAPI wine_glUniformMatrix3x2dv( GLint location, GLsizei count, GLboolean transpose, const GLdouble* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix3x2dv( location, count, transpose, value );
}

static void WINAPI wine_glUniformMatrix3x2fv( GLint location, GLsizei count, GLboolean transpose, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix3x2fv( location, count, transpose, value );
}

static void WINAPI wine_glUniformMatrix3x4dv( GLint location, GLsizei count, GLboolean transpose, const GLdouble* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix3x4dv( location, count, transpose, value );
}

static void WINAPI wine_glUniformMatrix3x4fv( GLint location, GLsizei count, GLboolean transpose, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix3x4fv( location, count, transpose, value );
}

static void WINAPI wine_glUniformMatrix4dv( GLint location, GLsizei count, GLboolean transpose, const GLdouble* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix4dv( location, count, transpose, value );
}

static void WINAPI wine_glUniformMatrix4fv( GLint location, GLsizei count, GLboolean transpose, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix4fv( location, count, transpose, value );
}

static void WINAPI wine_glUniformMatrix4fvARB( GLint location, GLsizei count, GLboolean transpose, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix4fvARB( location, count, transpose, value );
}

static void WINAPI wine_glUniformMatrix4x2dv( GLint location, GLsizei count, GLboolean transpose, const GLdouble* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix4x2dv( location, count, transpose, value );
}

static void WINAPI wine_glUniformMatrix4x2fv( GLint location, GLsizei count, GLboolean transpose, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix4x2fv( location, count, transpose, value );
}

static void WINAPI wine_glUniformMatrix4x3dv( GLint location, GLsizei count, GLboolean transpose, const GLdouble* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix4x3dv( location, count, transpose, value );
}

static void WINAPI wine_glUniformMatrix4x3fv( GLint location, GLsizei count, GLboolean transpose, const GLfloat* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix4x3fv( location, count, transpose, value );
}

static void WINAPI wine_glUniformSubroutinesuiv( GLenum shadertype, GLsizei count, const GLuint* indices ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", shadertype, count, indices );
  funcs->ext.p_glUniformSubroutinesuiv( shadertype, count, indices );
}

static void WINAPI wine_glUniformui64NV( GLint location, UINT64 value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %s)\n", location, wine_dbgstr_longlong(value) );
  funcs->ext.p_glUniformui64NV( location, value );
}

static void WINAPI wine_glUniformui64vNV( GLint location, GLsizei count, const UINT64* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniformui64vNV( location, count, value );
}

static void WINAPI wine_glUnlockArraysEXT( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  funcs->ext.p_glUnlockArraysEXT( );
}

static GLboolean WINAPI wine_glUnmapBuffer( GLenum target ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", target );
  return funcs->ext.p_glUnmapBuffer( target );
}

static GLboolean WINAPI wine_glUnmapBufferARB( GLenum target ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", target );
  return funcs->ext.p_glUnmapBufferARB( target );
}

static GLboolean WINAPI wine_glUnmapNamedBufferEXT( GLuint buffer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", buffer );
  return funcs->ext.p_glUnmapNamedBufferEXT( buffer );
}

static void WINAPI wine_glUnmapObjectBufferATI( GLuint buffer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", buffer );
  funcs->ext.p_glUnmapObjectBufferATI( buffer );
}

static void WINAPI wine_glUpdateObjectBufferATI( GLuint buffer, GLuint offset, GLsizei size, const GLvoid* pointer, GLenum preserve ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p, %d)\n", buffer, offset, size, pointer, preserve );
  funcs->ext.p_glUpdateObjectBufferATI( buffer, offset, size, pointer, preserve );
}

static void WINAPI wine_glUseProgram( GLuint program ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", program );
  funcs->ext.p_glUseProgram( program );
}

static void WINAPI wine_glUseProgramObjectARB( unsigned int programObj ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", programObj );
  funcs->ext.p_glUseProgramObjectARB( programObj );
}

static void WINAPI wine_glUseProgramStages( GLuint pipeline, GLbitfield stages, GLuint program ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", pipeline, stages, program );
  funcs->ext.p_glUseProgramStages( pipeline, stages, program );
}

static void WINAPI wine_glUseShaderProgramEXT( GLenum type, GLuint program ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", type, program );
  funcs->ext.p_glUseShaderProgramEXT( type, program );
}

static void WINAPI wine_glVDPAUFiniNV( void ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("()\n");
  funcs->ext.p_glVDPAUFiniNV( );
}

static void WINAPI wine_glVDPAUGetSurfaceivNV( INT_PTR surface, GLenum pname, GLsizei bufSize, GLsizei* length, GLint* values ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%ld, %d, %d, %p, %p)\n", surface, pname, bufSize, length, values );
  funcs->ext.p_glVDPAUGetSurfaceivNV( surface, pname, bufSize, length, values );
}

static void WINAPI wine_glVDPAUInitNV( const GLvoid* vdpDevice, const GLvoid* getProcAddress ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p, %p)\n", vdpDevice, getProcAddress );
  funcs->ext.p_glVDPAUInitNV( vdpDevice, getProcAddress );
}

static void WINAPI wine_glVDPAUIsSurfaceNV( INT_PTR surface ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%ld)\n", surface );
  funcs->ext.p_glVDPAUIsSurfaceNV( surface );
}

static void WINAPI wine_glVDPAUMapSurfacesNV( GLsizei numSurfaces, const INT_PTR* surfaces ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", numSurfaces, surfaces );
  funcs->ext.p_glVDPAUMapSurfacesNV( numSurfaces, surfaces );
}

static INT_PTR WINAPI wine_glVDPAURegisterOutputSurfaceNV( GLvoid* vdpSurface, GLenum target, GLsizei numTextureNames, const GLuint* textureNames ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p, %d, %d, %p)\n", vdpSurface, target, numTextureNames, textureNames );
  return funcs->ext.p_glVDPAURegisterOutputSurfaceNV( vdpSurface, target, numTextureNames, textureNames );
}

static INT_PTR WINAPI wine_glVDPAURegisterVideoSurfaceNV( GLvoid* vdpSurface, GLenum target, GLsizei numTextureNames, const GLuint* textureNames ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p, %d, %d, %p)\n", vdpSurface, target, numTextureNames, textureNames );
  return funcs->ext.p_glVDPAURegisterVideoSurfaceNV( vdpSurface, target, numTextureNames, textureNames );
}

static void WINAPI wine_glVDPAUSurfaceAccessNV( INT_PTR surface, GLenum access ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%ld, %d)\n", surface, access );
  funcs->ext.p_glVDPAUSurfaceAccessNV( surface, access );
}

static void WINAPI wine_glVDPAUUnmapSurfacesNV( GLsizei numSurface, const INT_PTR* surfaces ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", numSurface, surfaces );
  funcs->ext.p_glVDPAUUnmapSurfacesNV( numSurface, surfaces );
}

static void WINAPI wine_glVDPAUUnregisterSurfaceNV( INT_PTR surface ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%ld)\n", surface );
  funcs->ext.p_glVDPAUUnregisterSurfaceNV( surface );
}

static void WINAPI wine_glValidateProgram( GLuint program ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", program );
  funcs->ext.p_glValidateProgram( program );
}

static void WINAPI wine_glValidateProgramARB( unsigned int programObj ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", programObj );
  funcs->ext.p_glValidateProgramARB( programObj );
}

static void WINAPI wine_glValidateProgramPipeline( GLuint pipeline ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", pipeline );
  funcs->ext.p_glValidateProgramPipeline( pipeline );
}

static void WINAPI wine_glVariantArrayObjectATI( GLuint id, GLenum type, GLsizei stride, GLuint buffer, GLuint offset ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", id, type, stride, buffer, offset );
  funcs->ext.p_glVariantArrayObjectATI( id, type, stride, buffer, offset );
}

static void WINAPI wine_glVariantPointerEXT( GLuint id, GLenum type, GLuint stride, const GLvoid* addr ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", id, type, stride, addr );
  funcs->ext.p_glVariantPointerEXT( id, type, stride, addr );
}

static void WINAPI wine_glVariantbvEXT( GLuint id, const GLbyte* addr ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", id, addr );
  funcs->ext.p_glVariantbvEXT( id, addr );
}

static void WINAPI wine_glVariantdvEXT( GLuint id, const GLdouble* addr ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", id, addr );
  funcs->ext.p_glVariantdvEXT( id, addr );
}

static void WINAPI wine_glVariantfvEXT( GLuint id, const GLfloat* addr ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", id, addr );
  funcs->ext.p_glVariantfvEXT( id, addr );
}

static void WINAPI wine_glVariantivEXT( GLuint id, const GLint* addr ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", id, addr );
  funcs->ext.p_glVariantivEXT( id, addr );
}

static void WINAPI wine_glVariantsvEXT( GLuint id, const GLshort* addr ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", id, addr );
  funcs->ext.p_glVariantsvEXT( id, addr );
}

static void WINAPI wine_glVariantubvEXT( GLuint id, const GLubyte* addr ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", id, addr );
  funcs->ext.p_glVariantubvEXT( id, addr );
}

static void WINAPI wine_glVariantuivEXT( GLuint id, const GLuint* addr ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", id, addr );
  funcs->ext.p_glVariantuivEXT( id, addr );
}

static void WINAPI wine_glVariantusvEXT( GLuint id, const GLushort* addr ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", id, addr );
  funcs->ext.p_glVariantusvEXT( id, addr );
}

static void WINAPI wine_glVertex2hNV( unsigned short x, unsigned short y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", x, y );
  funcs->ext.p_glVertex2hNV( x, y );
}

static void WINAPI wine_glVertex2hvNV( const unsigned short* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glVertex2hvNV( v );
}

static void WINAPI wine_glVertex3hNV( unsigned short x, unsigned short y, unsigned short z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", x, y, z );
  funcs->ext.p_glVertex3hNV( x, y, z );
}

static void WINAPI wine_glVertex3hvNV( const unsigned short* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glVertex3hvNV( v );
}

static void WINAPI wine_glVertex4hNV( unsigned short x, unsigned short y, unsigned short z, unsigned short w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", x, y, z, w );
  funcs->ext.p_glVertex4hNV( x, y, z, w );
}

static void WINAPI wine_glVertex4hvNV( const unsigned short* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glVertex4hvNV( v );
}

static void WINAPI wine_glVertexArrayParameteriAPPLE( GLenum pname, GLint param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", pname, param );
  funcs->ext.p_glVertexArrayParameteriAPPLE( pname, param );
}

static void WINAPI wine_glVertexArrayRangeAPPLE( GLsizei length, GLvoid* pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", length, pointer );
  funcs->ext.p_glVertexArrayRangeAPPLE( length, pointer );
}

static void WINAPI wine_glVertexArrayRangeNV( GLsizei length, const GLvoid* pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", length, pointer );
  funcs->ext.p_glVertexArrayRangeNV( length, pointer );
}

static void WINAPI wine_glVertexArrayVertexAttribLOffsetEXT( GLuint vaobj, GLuint buffer, GLuint index, GLint size, GLenum type, GLsizei stride, INT_PTR offset ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %ld)\n", vaobj, buffer, index, size, type, stride, offset );
  funcs->ext.p_glVertexArrayVertexAttribLOffsetEXT( vaobj, buffer, index, size, type, stride, offset );
}

static void WINAPI wine_glVertexAttrib1d( GLuint index, GLdouble x ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f)\n", index, x );
  funcs->ext.p_glVertexAttrib1d( index, x );
}

static void WINAPI wine_glVertexAttrib1dARB( GLuint index, GLdouble x ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f)\n", index, x );
  funcs->ext.p_glVertexAttrib1dARB( index, x );
}

static void WINAPI wine_glVertexAttrib1dNV( GLuint index, GLdouble x ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f)\n", index, x );
  funcs->ext.p_glVertexAttrib1dNV( index, x );
}

static void WINAPI wine_glVertexAttrib1dv( GLuint index, const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib1dv( index, v );
}

static void WINAPI wine_glVertexAttrib1dvARB( GLuint index, const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib1dvARB( index, v );
}

static void WINAPI wine_glVertexAttrib1dvNV( GLuint index, const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib1dvNV( index, v );
}

static void WINAPI wine_glVertexAttrib1f( GLuint index, GLfloat x ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f)\n", index, x );
  funcs->ext.p_glVertexAttrib1f( index, x );
}

static void WINAPI wine_glVertexAttrib1fARB( GLuint index, GLfloat x ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f)\n", index, x );
  funcs->ext.p_glVertexAttrib1fARB( index, x );
}

static void WINAPI wine_glVertexAttrib1fNV( GLuint index, GLfloat x ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f)\n", index, x );
  funcs->ext.p_glVertexAttrib1fNV( index, x );
}

static void WINAPI wine_glVertexAttrib1fv( GLuint index, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib1fv( index, v );
}

static void WINAPI wine_glVertexAttrib1fvARB( GLuint index, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib1fvARB( index, v );
}

static void WINAPI wine_glVertexAttrib1fvNV( GLuint index, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib1fvNV( index, v );
}

static void WINAPI wine_glVertexAttrib1hNV( GLuint index, unsigned short x ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", index, x );
  funcs->ext.p_glVertexAttrib1hNV( index, x );
}

static void WINAPI wine_glVertexAttrib1hvNV( GLuint index, const unsigned short* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib1hvNV( index, v );
}

static void WINAPI wine_glVertexAttrib1s( GLuint index, GLshort x ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", index, x );
  funcs->ext.p_glVertexAttrib1s( index, x );
}

static void WINAPI wine_glVertexAttrib1sARB( GLuint index, GLshort x ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", index, x );
  funcs->ext.p_glVertexAttrib1sARB( index, x );
}

static void WINAPI wine_glVertexAttrib1sNV( GLuint index, GLshort x ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", index, x );
  funcs->ext.p_glVertexAttrib1sNV( index, x );
}

static void WINAPI wine_glVertexAttrib1sv( GLuint index, const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib1sv( index, v );
}

static void WINAPI wine_glVertexAttrib1svARB( GLuint index, const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib1svARB( index, v );
}

static void WINAPI wine_glVertexAttrib1svNV( GLuint index, const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib1svNV( index, v );
}

static void WINAPI wine_glVertexAttrib2d( GLuint index, GLdouble x, GLdouble y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f)\n", index, x, y );
  funcs->ext.p_glVertexAttrib2d( index, x, y );
}

static void WINAPI wine_glVertexAttrib2dARB( GLuint index, GLdouble x, GLdouble y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f)\n", index, x, y );
  funcs->ext.p_glVertexAttrib2dARB( index, x, y );
}

static void WINAPI wine_glVertexAttrib2dNV( GLuint index, GLdouble x, GLdouble y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f)\n", index, x, y );
  funcs->ext.p_glVertexAttrib2dNV( index, x, y );
}

static void WINAPI wine_glVertexAttrib2dv( GLuint index, const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib2dv( index, v );
}

static void WINAPI wine_glVertexAttrib2dvARB( GLuint index, const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib2dvARB( index, v );
}

static void WINAPI wine_glVertexAttrib2dvNV( GLuint index, const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib2dvNV( index, v );
}

static void WINAPI wine_glVertexAttrib2f( GLuint index, GLfloat x, GLfloat y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f)\n", index, x, y );
  funcs->ext.p_glVertexAttrib2f( index, x, y );
}

static void WINAPI wine_glVertexAttrib2fARB( GLuint index, GLfloat x, GLfloat y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f)\n", index, x, y );
  funcs->ext.p_glVertexAttrib2fARB( index, x, y );
}

static void WINAPI wine_glVertexAttrib2fNV( GLuint index, GLfloat x, GLfloat y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f)\n", index, x, y );
  funcs->ext.p_glVertexAttrib2fNV( index, x, y );
}

static void WINAPI wine_glVertexAttrib2fv( GLuint index, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib2fv( index, v );
}

static void WINAPI wine_glVertexAttrib2fvARB( GLuint index, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib2fvARB( index, v );
}

static void WINAPI wine_glVertexAttrib2fvNV( GLuint index, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib2fvNV( index, v );
}

static void WINAPI wine_glVertexAttrib2hNV( GLuint index, unsigned short x, unsigned short y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", index, x, y );
  funcs->ext.p_glVertexAttrib2hNV( index, x, y );
}

static void WINAPI wine_glVertexAttrib2hvNV( GLuint index, const unsigned short* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib2hvNV( index, v );
}

static void WINAPI wine_glVertexAttrib2s( GLuint index, GLshort x, GLshort y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", index, x, y );
  funcs->ext.p_glVertexAttrib2s( index, x, y );
}

static void WINAPI wine_glVertexAttrib2sARB( GLuint index, GLshort x, GLshort y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", index, x, y );
  funcs->ext.p_glVertexAttrib2sARB( index, x, y );
}

static void WINAPI wine_glVertexAttrib2sNV( GLuint index, GLshort x, GLshort y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", index, x, y );
  funcs->ext.p_glVertexAttrib2sNV( index, x, y );
}

static void WINAPI wine_glVertexAttrib2sv( GLuint index, const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib2sv( index, v );
}

static void WINAPI wine_glVertexAttrib2svARB( GLuint index, const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib2svARB( index, v );
}

static void WINAPI wine_glVertexAttrib2svNV( GLuint index, const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib2svNV( index, v );
}

static void WINAPI wine_glVertexAttrib3d( GLuint index, GLdouble x, GLdouble y, GLdouble z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f)\n", index, x, y, z );
  funcs->ext.p_glVertexAttrib3d( index, x, y, z );
}

static void WINAPI wine_glVertexAttrib3dARB( GLuint index, GLdouble x, GLdouble y, GLdouble z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f)\n", index, x, y, z );
  funcs->ext.p_glVertexAttrib3dARB( index, x, y, z );
}

static void WINAPI wine_glVertexAttrib3dNV( GLuint index, GLdouble x, GLdouble y, GLdouble z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f)\n", index, x, y, z );
  funcs->ext.p_glVertexAttrib3dNV( index, x, y, z );
}

static void WINAPI wine_glVertexAttrib3dv( GLuint index, const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib3dv( index, v );
}

static void WINAPI wine_glVertexAttrib3dvARB( GLuint index, const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib3dvARB( index, v );
}

static void WINAPI wine_glVertexAttrib3dvNV( GLuint index, const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib3dvNV( index, v );
}

static void WINAPI wine_glVertexAttrib3f( GLuint index, GLfloat x, GLfloat y, GLfloat z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f)\n", index, x, y, z );
  funcs->ext.p_glVertexAttrib3f( index, x, y, z );
}

static void WINAPI wine_glVertexAttrib3fARB( GLuint index, GLfloat x, GLfloat y, GLfloat z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f)\n", index, x, y, z );
  funcs->ext.p_glVertexAttrib3fARB( index, x, y, z );
}

static void WINAPI wine_glVertexAttrib3fNV( GLuint index, GLfloat x, GLfloat y, GLfloat z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f)\n", index, x, y, z );
  funcs->ext.p_glVertexAttrib3fNV( index, x, y, z );
}

static void WINAPI wine_glVertexAttrib3fv( GLuint index, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib3fv( index, v );
}

static void WINAPI wine_glVertexAttrib3fvARB( GLuint index, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib3fvARB( index, v );
}

static void WINAPI wine_glVertexAttrib3fvNV( GLuint index, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib3fvNV( index, v );
}

static void WINAPI wine_glVertexAttrib3hNV( GLuint index, unsigned short x, unsigned short y, unsigned short z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", index, x, y, z );
  funcs->ext.p_glVertexAttrib3hNV( index, x, y, z );
}

static void WINAPI wine_glVertexAttrib3hvNV( GLuint index, const unsigned short* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib3hvNV( index, v );
}

static void WINAPI wine_glVertexAttrib3s( GLuint index, GLshort x, GLshort y, GLshort z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", index, x, y, z );
  funcs->ext.p_glVertexAttrib3s( index, x, y, z );
}

static void WINAPI wine_glVertexAttrib3sARB( GLuint index, GLshort x, GLshort y, GLshort z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", index, x, y, z );
  funcs->ext.p_glVertexAttrib3sARB( index, x, y, z );
}

static void WINAPI wine_glVertexAttrib3sNV( GLuint index, GLshort x, GLshort y, GLshort z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", index, x, y, z );
  funcs->ext.p_glVertexAttrib3sNV( index, x, y, z );
}

static void WINAPI wine_glVertexAttrib3sv( GLuint index, const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib3sv( index, v );
}

static void WINAPI wine_glVertexAttrib3svARB( GLuint index, const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib3svARB( index, v );
}

static void WINAPI wine_glVertexAttrib3svNV( GLuint index, const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib3svNV( index, v );
}

static void WINAPI wine_glVertexAttrib4Nbv( GLuint index, const GLbyte* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4Nbv( index, v );
}

static void WINAPI wine_glVertexAttrib4NbvARB( GLuint index, const GLbyte* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4NbvARB( index, v );
}

static void WINAPI wine_glVertexAttrib4Niv( GLuint index, const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4Niv( index, v );
}

static void WINAPI wine_glVertexAttrib4NivARB( GLuint index, const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4NivARB( index, v );
}

static void WINAPI wine_glVertexAttrib4Nsv( GLuint index, const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4Nsv( index, v );
}

static void WINAPI wine_glVertexAttrib4NsvARB( GLuint index, const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4NsvARB( index, v );
}

static void WINAPI wine_glVertexAttrib4Nub( GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  funcs->ext.p_glVertexAttrib4Nub( index, x, y, z, w );
}

static void WINAPI wine_glVertexAttrib4NubARB( GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  funcs->ext.p_glVertexAttrib4NubARB( index, x, y, z, w );
}

static void WINAPI wine_glVertexAttrib4Nubv( GLuint index, const GLubyte* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4Nubv( index, v );
}

static void WINAPI wine_glVertexAttrib4NubvARB( GLuint index, const GLubyte* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4NubvARB( index, v );
}

static void WINAPI wine_glVertexAttrib4Nuiv( GLuint index, const GLuint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4Nuiv( index, v );
}

static void WINAPI wine_glVertexAttrib4NuivARB( GLuint index, const GLuint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4NuivARB( index, v );
}

static void WINAPI wine_glVertexAttrib4Nusv( GLuint index, const GLushort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4Nusv( index, v );
}

static void WINAPI wine_glVertexAttrib4NusvARB( GLuint index, const GLushort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4NusvARB( index, v );
}

static void WINAPI wine_glVertexAttrib4bv( GLuint index, const GLbyte* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4bv( index, v );
}

static void WINAPI wine_glVertexAttrib4bvARB( GLuint index, const GLbyte* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4bvARB( index, v );
}

static void WINAPI wine_glVertexAttrib4d( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f, %f)\n", index, x, y, z, w );
  funcs->ext.p_glVertexAttrib4d( index, x, y, z, w );
}

static void WINAPI wine_glVertexAttrib4dARB( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f, %f)\n", index, x, y, z, w );
  funcs->ext.p_glVertexAttrib4dARB( index, x, y, z, w );
}

static void WINAPI wine_glVertexAttrib4dNV( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f, %f)\n", index, x, y, z, w );
  funcs->ext.p_glVertexAttrib4dNV( index, x, y, z, w );
}

static void WINAPI wine_glVertexAttrib4dv( GLuint index, const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4dv( index, v );
}

static void WINAPI wine_glVertexAttrib4dvARB( GLuint index, const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4dvARB( index, v );
}

static void WINAPI wine_glVertexAttrib4dvNV( GLuint index, const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4dvNV( index, v );
}

static void WINAPI wine_glVertexAttrib4f( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f, %f)\n", index, x, y, z, w );
  funcs->ext.p_glVertexAttrib4f( index, x, y, z, w );
}

static void WINAPI wine_glVertexAttrib4fARB( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f, %f)\n", index, x, y, z, w );
  funcs->ext.p_glVertexAttrib4fARB( index, x, y, z, w );
}

static void WINAPI wine_glVertexAttrib4fNV( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f, %f)\n", index, x, y, z, w );
  funcs->ext.p_glVertexAttrib4fNV( index, x, y, z, w );
}

static void WINAPI wine_glVertexAttrib4fv( GLuint index, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4fv( index, v );
}

static void WINAPI wine_glVertexAttrib4fvARB( GLuint index, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4fvARB( index, v );
}

static void WINAPI wine_glVertexAttrib4fvNV( GLuint index, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4fvNV( index, v );
}

static void WINAPI wine_glVertexAttrib4hNV( GLuint index, unsigned short x, unsigned short y, unsigned short z, unsigned short w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  funcs->ext.p_glVertexAttrib4hNV( index, x, y, z, w );
}

static void WINAPI wine_glVertexAttrib4hvNV( GLuint index, const unsigned short* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4hvNV( index, v );
}

static void WINAPI wine_glVertexAttrib4iv( GLuint index, const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4iv( index, v );
}

static void WINAPI wine_glVertexAttrib4ivARB( GLuint index, const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4ivARB( index, v );
}

static void WINAPI wine_glVertexAttrib4s( GLuint index, GLshort x, GLshort y, GLshort z, GLshort w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  funcs->ext.p_glVertexAttrib4s( index, x, y, z, w );
}

static void WINAPI wine_glVertexAttrib4sARB( GLuint index, GLshort x, GLshort y, GLshort z, GLshort w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  funcs->ext.p_glVertexAttrib4sARB( index, x, y, z, w );
}

static void WINAPI wine_glVertexAttrib4sNV( GLuint index, GLshort x, GLshort y, GLshort z, GLshort w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  funcs->ext.p_glVertexAttrib4sNV( index, x, y, z, w );
}

static void WINAPI wine_glVertexAttrib4sv( GLuint index, const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4sv( index, v );
}

static void WINAPI wine_glVertexAttrib4svARB( GLuint index, const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4svARB( index, v );
}

static void WINAPI wine_glVertexAttrib4svNV( GLuint index, const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4svNV( index, v );
}

static void WINAPI wine_glVertexAttrib4ubNV( GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  funcs->ext.p_glVertexAttrib4ubNV( index, x, y, z, w );
}

static void WINAPI wine_glVertexAttrib4ubv( GLuint index, const GLubyte* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4ubv( index, v );
}

static void WINAPI wine_glVertexAttrib4ubvARB( GLuint index, const GLubyte* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4ubvARB( index, v );
}

static void WINAPI wine_glVertexAttrib4ubvNV( GLuint index, const GLubyte* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4ubvNV( index, v );
}

static void WINAPI wine_glVertexAttrib4uiv( GLuint index, const GLuint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4uiv( index, v );
}

static void WINAPI wine_glVertexAttrib4uivARB( GLuint index, const GLuint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4uivARB( index, v );
}

static void WINAPI wine_glVertexAttrib4usv( GLuint index, const GLushort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4usv( index, v );
}

static void WINAPI wine_glVertexAttrib4usvARB( GLuint index, const GLushort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4usvARB( index, v );
}

static void WINAPI wine_glVertexAttribArrayObjectATI( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, GLuint buffer, GLuint offset ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d, %d)\n", index, size, type, normalized, stride, buffer, offset );
  funcs->ext.p_glVertexAttribArrayObjectATI( index, size, type, normalized, stride, buffer, offset );
}

static void WINAPI wine_glVertexAttribDivisor( GLuint index, GLuint divisor ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", index, divisor );
  funcs->ext.p_glVertexAttribDivisor( index, divisor );
}

static void WINAPI wine_glVertexAttribDivisorARB( GLuint index, GLuint divisor ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", index, divisor );
  funcs->ext.p_glVertexAttribDivisorARB( index, divisor );
}

static void WINAPI wine_glVertexAttribFormatNV( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", index, size, type, normalized, stride );
  funcs->ext.p_glVertexAttribFormatNV( index, size, type, normalized, stride );
}

static void WINAPI wine_glVertexAttribI1i( GLuint index, GLint x ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", index, x );
  funcs->ext.p_glVertexAttribI1i( index, x );
}

static void WINAPI wine_glVertexAttribI1iEXT( GLuint index, GLint x ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", index, x );
  funcs->ext.p_glVertexAttribI1iEXT( index, x );
}

static void WINAPI wine_glVertexAttribI1iv( GLuint index, const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI1iv( index, v );
}

static void WINAPI wine_glVertexAttribI1ivEXT( GLuint index, const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI1ivEXT( index, v );
}

static void WINAPI wine_glVertexAttribI1ui( GLuint index, GLuint x ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", index, x );
  funcs->ext.p_glVertexAttribI1ui( index, x );
}

static void WINAPI wine_glVertexAttribI1uiEXT( GLuint index, GLuint x ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", index, x );
  funcs->ext.p_glVertexAttribI1uiEXT( index, x );
}

static void WINAPI wine_glVertexAttribI1uiv( GLuint index, const GLuint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI1uiv( index, v );
}

static void WINAPI wine_glVertexAttribI1uivEXT( GLuint index, const GLuint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI1uivEXT( index, v );
}

static void WINAPI wine_glVertexAttribI2i( GLuint index, GLint x, GLint y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", index, x, y );
  funcs->ext.p_glVertexAttribI2i( index, x, y );
}

static void WINAPI wine_glVertexAttribI2iEXT( GLuint index, GLint x, GLint y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", index, x, y );
  funcs->ext.p_glVertexAttribI2iEXT( index, x, y );
}

static void WINAPI wine_glVertexAttribI2iv( GLuint index, const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI2iv( index, v );
}

static void WINAPI wine_glVertexAttribI2ivEXT( GLuint index, const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI2ivEXT( index, v );
}

static void WINAPI wine_glVertexAttribI2ui( GLuint index, GLuint x, GLuint y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", index, x, y );
  funcs->ext.p_glVertexAttribI2ui( index, x, y );
}

static void WINAPI wine_glVertexAttribI2uiEXT( GLuint index, GLuint x, GLuint y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", index, x, y );
  funcs->ext.p_glVertexAttribI2uiEXT( index, x, y );
}

static void WINAPI wine_glVertexAttribI2uiv( GLuint index, const GLuint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI2uiv( index, v );
}

static void WINAPI wine_glVertexAttribI2uivEXT( GLuint index, const GLuint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI2uivEXT( index, v );
}

static void WINAPI wine_glVertexAttribI3i( GLuint index, GLint x, GLint y, GLint z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", index, x, y, z );
  funcs->ext.p_glVertexAttribI3i( index, x, y, z );
}

static void WINAPI wine_glVertexAttribI3iEXT( GLuint index, GLint x, GLint y, GLint z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", index, x, y, z );
  funcs->ext.p_glVertexAttribI3iEXT( index, x, y, z );
}

static void WINAPI wine_glVertexAttribI3iv( GLuint index, const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI3iv( index, v );
}

static void WINAPI wine_glVertexAttribI3ivEXT( GLuint index, const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI3ivEXT( index, v );
}

static void WINAPI wine_glVertexAttribI3ui( GLuint index, GLuint x, GLuint y, GLuint z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", index, x, y, z );
  funcs->ext.p_glVertexAttribI3ui( index, x, y, z );
}

static void WINAPI wine_glVertexAttribI3uiEXT( GLuint index, GLuint x, GLuint y, GLuint z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", index, x, y, z );
  funcs->ext.p_glVertexAttribI3uiEXT( index, x, y, z );
}

static void WINAPI wine_glVertexAttribI3uiv( GLuint index, const GLuint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI3uiv( index, v );
}

static void WINAPI wine_glVertexAttribI3uivEXT( GLuint index, const GLuint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI3uivEXT( index, v );
}

static void WINAPI wine_glVertexAttribI4bv( GLuint index, const GLbyte* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI4bv( index, v );
}

static void WINAPI wine_glVertexAttribI4bvEXT( GLuint index, const GLbyte* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI4bvEXT( index, v );
}

static void WINAPI wine_glVertexAttribI4i( GLuint index, GLint x, GLint y, GLint z, GLint w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  funcs->ext.p_glVertexAttribI4i( index, x, y, z, w );
}

static void WINAPI wine_glVertexAttribI4iEXT( GLuint index, GLint x, GLint y, GLint z, GLint w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  funcs->ext.p_glVertexAttribI4iEXT( index, x, y, z, w );
}

static void WINAPI wine_glVertexAttribI4iv( GLuint index, const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI4iv( index, v );
}

static void WINAPI wine_glVertexAttribI4ivEXT( GLuint index, const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI4ivEXT( index, v );
}

static void WINAPI wine_glVertexAttribI4sv( GLuint index, const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI4sv( index, v );
}

static void WINAPI wine_glVertexAttribI4svEXT( GLuint index, const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI4svEXT( index, v );
}

static void WINAPI wine_glVertexAttribI4ubv( GLuint index, const GLubyte* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI4ubv( index, v );
}

static void WINAPI wine_glVertexAttribI4ubvEXT( GLuint index, const GLubyte* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI4ubvEXT( index, v );
}

static void WINAPI wine_glVertexAttribI4ui( GLuint index, GLuint x, GLuint y, GLuint z, GLuint w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  funcs->ext.p_glVertexAttribI4ui( index, x, y, z, w );
}

static void WINAPI wine_glVertexAttribI4uiEXT( GLuint index, GLuint x, GLuint y, GLuint z, GLuint w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  funcs->ext.p_glVertexAttribI4uiEXT( index, x, y, z, w );
}

static void WINAPI wine_glVertexAttribI4uiv( GLuint index, const GLuint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI4uiv( index, v );
}

static void WINAPI wine_glVertexAttribI4uivEXT( GLuint index, const GLuint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI4uivEXT( index, v );
}

static void WINAPI wine_glVertexAttribI4usv( GLuint index, const GLushort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI4usv( index, v );
}

static void WINAPI wine_glVertexAttribI4usvEXT( GLuint index, const GLushort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI4usvEXT( index, v );
}

static void WINAPI wine_glVertexAttribIFormatNV( GLuint index, GLint size, GLenum type, GLsizei stride ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", index, size, type, stride );
  funcs->ext.p_glVertexAttribIFormatNV( index, size, type, stride );
}

static void WINAPI wine_glVertexAttribIPointer( GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid* pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", index, size, type, stride, pointer );
  funcs->ext.p_glVertexAttribIPointer( index, size, type, stride, pointer );
}

static void WINAPI wine_glVertexAttribIPointerEXT( GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid* pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", index, size, type, stride, pointer );
  funcs->ext.p_glVertexAttribIPointerEXT( index, size, type, stride, pointer );
}

static void WINAPI wine_glVertexAttribL1d( GLuint index, GLdouble x ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f)\n", index, x );
  funcs->ext.p_glVertexAttribL1d( index, x );
}

static void WINAPI wine_glVertexAttribL1dEXT( GLuint index, GLdouble x ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f)\n", index, x );
  funcs->ext.p_glVertexAttribL1dEXT( index, x );
}

static void WINAPI wine_glVertexAttribL1dv( GLuint index, const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribL1dv( index, v );
}

static void WINAPI wine_glVertexAttribL1dvEXT( GLuint index, const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribL1dvEXT( index, v );
}

static void WINAPI wine_glVertexAttribL1i64NV( GLuint index, INT64 x ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %s)\n", index, wine_dbgstr_longlong(x) );
  funcs->ext.p_glVertexAttribL1i64NV( index, x );
}

static void WINAPI wine_glVertexAttribL1i64vNV( GLuint index, const INT64* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribL1i64vNV( index, v );
}

static void WINAPI wine_glVertexAttribL1ui64NV( GLuint index, UINT64 x ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %s)\n", index, wine_dbgstr_longlong(x) );
  funcs->ext.p_glVertexAttribL1ui64NV( index, x );
}

static void WINAPI wine_glVertexAttribL1ui64vNV( GLuint index, const UINT64* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribL1ui64vNV( index, v );
}

static void WINAPI wine_glVertexAttribL2d( GLuint index, GLdouble x, GLdouble y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f)\n", index, x, y );
  funcs->ext.p_glVertexAttribL2d( index, x, y );
}

static void WINAPI wine_glVertexAttribL2dEXT( GLuint index, GLdouble x, GLdouble y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f)\n", index, x, y );
  funcs->ext.p_glVertexAttribL2dEXT( index, x, y );
}

static void WINAPI wine_glVertexAttribL2dv( GLuint index, const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribL2dv( index, v );
}

static void WINAPI wine_glVertexAttribL2dvEXT( GLuint index, const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribL2dvEXT( index, v );
}

static void WINAPI wine_glVertexAttribL2i64NV( GLuint index, INT64 x, INT64 y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %s, %s)\n", index, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y) );
  funcs->ext.p_glVertexAttribL2i64NV( index, x, y );
}

static void WINAPI wine_glVertexAttribL2i64vNV( GLuint index, const INT64* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribL2i64vNV( index, v );
}

static void WINAPI wine_glVertexAttribL2ui64NV( GLuint index, UINT64 x, UINT64 y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %s, %s)\n", index, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y) );
  funcs->ext.p_glVertexAttribL2ui64NV( index, x, y );
}

static void WINAPI wine_glVertexAttribL2ui64vNV( GLuint index, const UINT64* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribL2ui64vNV( index, v );
}

static void WINAPI wine_glVertexAttribL3d( GLuint index, GLdouble x, GLdouble y, GLdouble z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f)\n", index, x, y, z );
  funcs->ext.p_glVertexAttribL3d( index, x, y, z );
}

static void WINAPI wine_glVertexAttribL3dEXT( GLuint index, GLdouble x, GLdouble y, GLdouble z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f)\n", index, x, y, z );
  funcs->ext.p_glVertexAttribL3dEXT( index, x, y, z );
}

static void WINAPI wine_glVertexAttribL3dv( GLuint index, const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribL3dv( index, v );
}

static void WINAPI wine_glVertexAttribL3dvEXT( GLuint index, const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribL3dvEXT( index, v );
}

static void WINAPI wine_glVertexAttribL3i64NV( GLuint index, INT64 x, INT64 y, INT64 z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %s, %s, %s)\n", index, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z) );
  funcs->ext.p_glVertexAttribL3i64NV( index, x, y, z );
}

static void WINAPI wine_glVertexAttribL3i64vNV( GLuint index, const INT64* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribL3i64vNV( index, v );
}

static void WINAPI wine_glVertexAttribL3ui64NV( GLuint index, UINT64 x, UINT64 y, UINT64 z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %s, %s, %s)\n", index, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z) );
  funcs->ext.p_glVertexAttribL3ui64NV( index, x, y, z );
}

static void WINAPI wine_glVertexAttribL3ui64vNV( GLuint index, const UINT64* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribL3ui64vNV( index, v );
}

static void WINAPI wine_glVertexAttribL4d( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f, %f)\n", index, x, y, z, w );
  funcs->ext.p_glVertexAttribL4d( index, x, y, z, w );
}

static void WINAPI wine_glVertexAttribL4dEXT( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f, %f)\n", index, x, y, z, w );
  funcs->ext.p_glVertexAttribL4dEXT( index, x, y, z, w );
}

static void WINAPI wine_glVertexAttribL4dv( GLuint index, const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribL4dv( index, v );
}

static void WINAPI wine_glVertexAttribL4dvEXT( GLuint index, const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribL4dvEXT( index, v );
}

static void WINAPI wine_glVertexAttribL4i64NV( GLuint index, INT64 x, INT64 y, INT64 z, INT64 w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %s, %s, %s, %s)\n", index, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z), wine_dbgstr_longlong(w) );
  funcs->ext.p_glVertexAttribL4i64NV( index, x, y, z, w );
}

static void WINAPI wine_glVertexAttribL4i64vNV( GLuint index, const INT64* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribL4i64vNV( index, v );
}

static void WINAPI wine_glVertexAttribL4ui64NV( GLuint index, UINT64 x, UINT64 y, UINT64 z, UINT64 w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %s, %s, %s, %s)\n", index, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z), wine_dbgstr_longlong(w) );
  funcs->ext.p_glVertexAttribL4ui64NV( index, x, y, z, w );
}

static void WINAPI wine_glVertexAttribL4ui64vNV( GLuint index, const UINT64* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribL4ui64vNV( index, v );
}

static void WINAPI wine_glVertexAttribLFormatNV( GLuint index, GLint size, GLenum type, GLsizei stride ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", index, size, type, stride );
  funcs->ext.p_glVertexAttribLFormatNV( index, size, type, stride );
}

static void WINAPI wine_glVertexAttribLPointer( GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid* pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", index, size, type, stride, pointer );
  funcs->ext.p_glVertexAttribLPointer( index, size, type, stride, pointer );
}

static void WINAPI wine_glVertexAttribLPointerEXT( GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid* pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", index, size, type, stride, pointer );
  funcs->ext.p_glVertexAttribLPointerEXT( index, size, type, stride, pointer );
}

static void WINAPI wine_glVertexAttribP1ui( GLuint index, GLenum type, GLboolean normalized, GLuint value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", index, type, normalized, value );
  funcs->ext.p_glVertexAttribP1ui( index, type, normalized, value );
}

static void WINAPI wine_glVertexAttribP1uiv( GLuint index, GLenum type, GLboolean normalized, const GLuint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", index, type, normalized, value );
  funcs->ext.p_glVertexAttribP1uiv( index, type, normalized, value );
}

static void WINAPI wine_glVertexAttribP2ui( GLuint index, GLenum type, GLboolean normalized, GLuint value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", index, type, normalized, value );
  funcs->ext.p_glVertexAttribP2ui( index, type, normalized, value );
}

static void WINAPI wine_glVertexAttribP2uiv( GLuint index, GLenum type, GLboolean normalized, const GLuint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", index, type, normalized, value );
  funcs->ext.p_glVertexAttribP2uiv( index, type, normalized, value );
}

static void WINAPI wine_glVertexAttribP3ui( GLuint index, GLenum type, GLboolean normalized, GLuint value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", index, type, normalized, value );
  funcs->ext.p_glVertexAttribP3ui( index, type, normalized, value );
}

static void WINAPI wine_glVertexAttribP3uiv( GLuint index, GLenum type, GLboolean normalized, const GLuint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", index, type, normalized, value );
  funcs->ext.p_glVertexAttribP3uiv( index, type, normalized, value );
}

static void WINAPI wine_glVertexAttribP4ui( GLuint index, GLenum type, GLboolean normalized, GLuint value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", index, type, normalized, value );
  funcs->ext.p_glVertexAttribP4ui( index, type, normalized, value );
}

static void WINAPI wine_glVertexAttribP4uiv( GLuint index, GLenum type, GLboolean normalized, const GLuint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", index, type, normalized, value );
  funcs->ext.p_glVertexAttribP4uiv( index, type, normalized, value );
}

static void WINAPI wine_glVertexAttribPointer( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %p)\n", index, size, type, normalized, stride, pointer );
  funcs->ext.p_glVertexAttribPointer( index, size, type, normalized, stride, pointer );
}

static void WINAPI wine_glVertexAttribPointerARB( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %p)\n", index, size, type, normalized, stride, pointer );
  funcs->ext.p_glVertexAttribPointerARB( index, size, type, normalized, stride, pointer );
}

static void WINAPI wine_glVertexAttribPointerNV( GLuint index, GLint fsize, GLenum type, GLsizei stride, const GLvoid* pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", index, fsize, type, stride, pointer );
  funcs->ext.p_glVertexAttribPointerNV( index, fsize, type, stride, pointer );
}

static void WINAPI wine_glVertexAttribs1dvNV( GLuint index, GLsizei count, const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", index, count, v );
  funcs->ext.p_glVertexAttribs1dvNV( index, count, v );
}

static void WINAPI wine_glVertexAttribs1fvNV( GLuint index, GLsizei count, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", index, count, v );
  funcs->ext.p_glVertexAttribs1fvNV( index, count, v );
}

static void WINAPI wine_glVertexAttribs1hvNV( GLuint index, GLsizei n, const unsigned short* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", index, n, v );
  funcs->ext.p_glVertexAttribs1hvNV( index, n, v );
}

static void WINAPI wine_glVertexAttribs1svNV( GLuint index, GLsizei count, const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", index, count, v );
  funcs->ext.p_glVertexAttribs1svNV( index, count, v );
}

static void WINAPI wine_glVertexAttribs2dvNV( GLuint index, GLsizei count, const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", index, count, v );
  funcs->ext.p_glVertexAttribs2dvNV( index, count, v );
}

static void WINAPI wine_glVertexAttribs2fvNV( GLuint index, GLsizei count, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", index, count, v );
  funcs->ext.p_glVertexAttribs2fvNV( index, count, v );
}

static void WINAPI wine_glVertexAttribs2hvNV( GLuint index, GLsizei n, const unsigned short* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", index, n, v );
  funcs->ext.p_glVertexAttribs2hvNV( index, n, v );
}

static void WINAPI wine_glVertexAttribs2svNV( GLuint index, GLsizei count, const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", index, count, v );
  funcs->ext.p_glVertexAttribs2svNV( index, count, v );
}

static void WINAPI wine_glVertexAttribs3dvNV( GLuint index, GLsizei count, const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", index, count, v );
  funcs->ext.p_glVertexAttribs3dvNV( index, count, v );
}

static void WINAPI wine_glVertexAttribs3fvNV( GLuint index, GLsizei count, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", index, count, v );
  funcs->ext.p_glVertexAttribs3fvNV( index, count, v );
}

static void WINAPI wine_glVertexAttribs3hvNV( GLuint index, GLsizei n, const unsigned short* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", index, n, v );
  funcs->ext.p_glVertexAttribs3hvNV( index, n, v );
}

static void WINAPI wine_glVertexAttribs3svNV( GLuint index, GLsizei count, const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", index, count, v );
  funcs->ext.p_glVertexAttribs3svNV( index, count, v );
}

static void WINAPI wine_glVertexAttribs4dvNV( GLuint index, GLsizei count, const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", index, count, v );
  funcs->ext.p_glVertexAttribs4dvNV( index, count, v );
}

static void WINAPI wine_glVertexAttribs4fvNV( GLuint index, GLsizei count, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", index, count, v );
  funcs->ext.p_glVertexAttribs4fvNV( index, count, v );
}

static void WINAPI wine_glVertexAttribs4hvNV( GLuint index, GLsizei n, const unsigned short* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", index, n, v );
  funcs->ext.p_glVertexAttribs4hvNV( index, n, v );
}

static void WINAPI wine_glVertexAttribs4svNV( GLuint index, GLsizei count, const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", index, count, v );
  funcs->ext.p_glVertexAttribs4svNV( index, count, v );
}

static void WINAPI wine_glVertexAttribs4ubvNV( GLuint index, GLsizei count, const GLubyte* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", index, count, v );
  funcs->ext.p_glVertexAttribs4ubvNV( index, count, v );
}

static void WINAPI wine_glVertexBlendARB( GLint count ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", count );
  funcs->ext.p_glVertexBlendARB( count );
}

static void WINAPI wine_glVertexBlendEnvfATI( GLenum pname, GLfloat param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f)\n", pname, param );
  funcs->ext.p_glVertexBlendEnvfATI( pname, param );
}

static void WINAPI wine_glVertexBlendEnviATI( GLenum pname, GLint param ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", pname, param );
  funcs->ext.p_glVertexBlendEnviATI( pname, param );
}

static void WINAPI wine_glVertexFormatNV( GLint size, GLenum type, GLsizei stride ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", size, type, stride );
  funcs->ext.p_glVertexFormatNV( size, type, stride );
}

static void WINAPI wine_glVertexP2ui( GLenum type, GLuint value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", type, value );
  funcs->ext.p_glVertexP2ui( type, value );
}

static void WINAPI wine_glVertexP2uiv( GLenum type, const GLuint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", type, value );
  funcs->ext.p_glVertexP2uiv( type, value );
}

static void WINAPI wine_glVertexP3ui( GLenum type, GLuint value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", type, value );
  funcs->ext.p_glVertexP3ui( type, value );
}

static void WINAPI wine_glVertexP3uiv( GLenum type, const GLuint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", type, value );
  funcs->ext.p_glVertexP3uiv( type, value );
}

static void WINAPI wine_glVertexP4ui( GLenum type, GLuint value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", type, value );
  funcs->ext.p_glVertexP4ui( type, value );
}

static void WINAPI wine_glVertexP4uiv( GLenum type, const GLuint* value ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", type, value );
  funcs->ext.p_glVertexP4uiv( type, value );
}

static void WINAPI wine_glVertexPointerEXT( GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid* pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %p)\n", size, type, stride, count, pointer );
  funcs->ext.p_glVertexPointerEXT( size, type, stride, count, pointer );
}

static void WINAPI wine_glVertexPointerListIBM( GLint size, GLenum type, GLint stride, const GLvoid** pointer, GLint ptrstride ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p, %d)\n", size, type, stride, pointer, ptrstride );
  funcs->ext.p_glVertexPointerListIBM( size, type, stride, pointer, ptrstride );
}

static void WINAPI wine_glVertexPointervINTEL( GLint size, GLenum type, const GLvoid** pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", size, type, pointer );
  funcs->ext.p_glVertexPointervINTEL( size, type, pointer );
}

static void WINAPI wine_glVertexStream1dATI( GLenum stream, GLdouble x ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f)\n", stream, x );
  funcs->ext.p_glVertexStream1dATI( stream, x );
}

static void WINAPI wine_glVertexStream1dvATI( GLenum stream, const GLdouble* coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", stream, coords );
  funcs->ext.p_glVertexStream1dvATI( stream, coords );
}

static void WINAPI wine_glVertexStream1fATI( GLenum stream, GLfloat x ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f)\n", stream, x );
  funcs->ext.p_glVertexStream1fATI( stream, x );
}

static void WINAPI wine_glVertexStream1fvATI( GLenum stream, const GLfloat* coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", stream, coords );
  funcs->ext.p_glVertexStream1fvATI( stream, coords );
}

static void WINAPI wine_glVertexStream1iATI( GLenum stream, GLint x ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", stream, x );
  funcs->ext.p_glVertexStream1iATI( stream, x );
}

static void WINAPI wine_glVertexStream1ivATI( GLenum stream, const GLint* coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", stream, coords );
  funcs->ext.p_glVertexStream1ivATI( stream, coords );
}

static void WINAPI wine_glVertexStream1sATI( GLenum stream, GLshort x ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", stream, x );
  funcs->ext.p_glVertexStream1sATI( stream, x );
}

static void WINAPI wine_glVertexStream1svATI( GLenum stream, const GLshort* coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", stream, coords );
  funcs->ext.p_glVertexStream1svATI( stream, coords );
}

static void WINAPI wine_glVertexStream2dATI( GLenum stream, GLdouble x, GLdouble y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f)\n", stream, x, y );
  funcs->ext.p_glVertexStream2dATI( stream, x, y );
}

static void WINAPI wine_glVertexStream2dvATI( GLenum stream, const GLdouble* coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", stream, coords );
  funcs->ext.p_glVertexStream2dvATI( stream, coords );
}

static void WINAPI wine_glVertexStream2fATI( GLenum stream, GLfloat x, GLfloat y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f)\n", stream, x, y );
  funcs->ext.p_glVertexStream2fATI( stream, x, y );
}

static void WINAPI wine_glVertexStream2fvATI( GLenum stream, const GLfloat* coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", stream, coords );
  funcs->ext.p_glVertexStream2fvATI( stream, coords );
}

static void WINAPI wine_glVertexStream2iATI( GLenum stream, GLint x, GLint y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", stream, x, y );
  funcs->ext.p_glVertexStream2iATI( stream, x, y );
}

static void WINAPI wine_glVertexStream2ivATI( GLenum stream, const GLint* coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", stream, coords );
  funcs->ext.p_glVertexStream2ivATI( stream, coords );
}

static void WINAPI wine_glVertexStream2sATI( GLenum stream, GLshort x, GLshort y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", stream, x, y );
  funcs->ext.p_glVertexStream2sATI( stream, x, y );
}

static void WINAPI wine_glVertexStream2svATI( GLenum stream, const GLshort* coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", stream, coords );
  funcs->ext.p_glVertexStream2svATI( stream, coords );
}

static void WINAPI wine_glVertexStream3dATI( GLenum stream, GLdouble x, GLdouble y, GLdouble z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f)\n", stream, x, y, z );
  funcs->ext.p_glVertexStream3dATI( stream, x, y, z );
}

static void WINAPI wine_glVertexStream3dvATI( GLenum stream, const GLdouble* coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", stream, coords );
  funcs->ext.p_glVertexStream3dvATI( stream, coords );
}

static void WINAPI wine_glVertexStream3fATI( GLenum stream, GLfloat x, GLfloat y, GLfloat z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f)\n", stream, x, y, z );
  funcs->ext.p_glVertexStream3fATI( stream, x, y, z );
}

static void WINAPI wine_glVertexStream3fvATI( GLenum stream, const GLfloat* coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", stream, coords );
  funcs->ext.p_glVertexStream3fvATI( stream, coords );
}

static void WINAPI wine_glVertexStream3iATI( GLenum stream, GLint x, GLint y, GLint z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", stream, x, y, z );
  funcs->ext.p_glVertexStream3iATI( stream, x, y, z );
}

static void WINAPI wine_glVertexStream3ivATI( GLenum stream, const GLint* coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", stream, coords );
  funcs->ext.p_glVertexStream3ivATI( stream, coords );
}

static void WINAPI wine_glVertexStream3sATI( GLenum stream, GLshort x, GLshort y, GLshort z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", stream, x, y, z );
  funcs->ext.p_glVertexStream3sATI( stream, x, y, z );
}

static void WINAPI wine_glVertexStream3svATI( GLenum stream, const GLshort* coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", stream, coords );
  funcs->ext.p_glVertexStream3svATI( stream, coords );
}

static void WINAPI wine_glVertexStream4dATI( GLenum stream, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f, %f)\n", stream, x, y, z, w );
  funcs->ext.p_glVertexStream4dATI( stream, x, y, z, w );
}

static void WINAPI wine_glVertexStream4dvATI( GLenum stream, const GLdouble* coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", stream, coords );
  funcs->ext.p_glVertexStream4dvATI( stream, coords );
}

static void WINAPI wine_glVertexStream4fATI( GLenum stream, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f, %f)\n", stream, x, y, z, w );
  funcs->ext.p_glVertexStream4fATI( stream, x, y, z, w );
}

static void WINAPI wine_glVertexStream4fvATI( GLenum stream, const GLfloat* coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", stream, coords );
  funcs->ext.p_glVertexStream4fvATI( stream, coords );
}

static void WINAPI wine_glVertexStream4iATI( GLenum stream, GLint x, GLint y, GLint z, GLint w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", stream, x, y, z, w );
  funcs->ext.p_glVertexStream4iATI( stream, x, y, z, w );
}

static void WINAPI wine_glVertexStream4ivATI( GLenum stream, const GLint* coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", stream, coords );
  funcs->ext.p_glVertexStream4ivATI( stream, coords );
}

static void WINAPI wine_glVertexStream4sATI( GLenum stream, GLshort x, GLshort y, GLshort z, GLshort w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d)\n", stream, x, y, z, w );
  funcs->ext.p_glVertexStream4sATI( stream, x, y, z, w );
}

static void WINAPI wine_glVertexStream4svATI( GLenum stream, const GLshort* coords ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", stream, coords );
  funcs->ext.p_glVertexStream4svATI( stream, coords );
}

static void WINAPI wine_glVertexWeightPointerEXT( GLsizei size, GLenum type, GLsizei stride, const GLvoid* pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", size, type, stride, pointer );
  funcs->ext.p_glVertexWeightPointerEXT( size, type, stride, pointer );
}

static void WINAPI wine_glVertexWeightfEXT( GLfloat weight ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f)\n", weight );
  funcs->ext.p_glVertexWeightfEXT( weight );
}

static void WINAPI wine_glVertexWeightfvEXT( const GLfloat* weight ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", weight );
  funcs->ext.p_glVertexWeightfvEXT( weight );
}

static void WINAPI wine_glVertexWeighthNV( unsigned short weight ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d)\n", weight );
  funcs->ext.p_glVertexWeighthNV( weight );
}

static void WINAPI wine_glVertexWeighthvNV( const unsigned short* weight ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", weight );
  funcs->ext.p_glVertexWeighthvNV( weight );
}

static GLenum WINAPI wine_glVideoCaptureNV( GLuint video_capture_slot, GLuint* sequence_num, UINT64* capture_time ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p, %p)\n", video_capture_slot, sequence_num, capture_time );
  return funcs->ext.p_glVideoCaptureNV( video_capture_slot, sequence_num, capture_time );
}

static void WINAPI wine_glVideoCaptureStreamParameterdvNV( GLuint video_capture_slot, GLuint stream, GLenum pname, const GLdouble* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", video_capture_slot, stream, pname, params );
  funcs->ext.p_glVideoCaptureStreamParameterdvNV( video_capture_slot, stream, pname, params );
}

static void WINAPI wine_glVideoCaptureStreamParameterfvNV( GLuint video_capture_slot, GLuint stream, GLenum pname, const GLfloat* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", video_capture_slot, stream, pname, params );
  funcs->ext.p_glVideoCaptureStreamParameterfvNV( video_capture_slot, stream, pname, params );
}

static void WINAPI wine_glVideoCaptureStreamParameterivNV( GLuint video_capture_slot, GLuint stream, GLenum pname, const GLint* params ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", video_capture_slot, stream, pname, params );
  funcs->ext.p_glVideoCaptureStreamParameterivNV( video_capture_slot, stream, pname, params );
}

static void WINAPI wine_glViewportArrayv( GLuint first, GLsizei count, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p)\n", first, count, v );
  funcs->ext.p_glViewportArrayv( first, count, v );
}

static void WINAPI wine_glViewportIndexedf( GLuint index, GLfloat x, GLfloat y, GLfloat w, GLfloat h ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %f, %f, %f, %f)\n", index, x, y, w, h );
  funcs->ext.p_glViewportIndexedf( index, x, y, w, h );
}

static void WINAPI wine_glViewportIndexedfv( GLuint index, const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", index, v );
  funcs->ext.p_glViewportIndexedfv( index, v );
}

static void WINAPI wine_glWaitSync( GLvoid* sync, GLbitfield flags, UINT64 timeout ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p, %d, %s)\n", sync, flags, wine_dbgstr_longlong(timeout) );
  funcs->ext.p_glWaitSync( sync, flags, timeout );
}

static void WINAPI wine_glWeightPathsNV( GLuint resultPath, GLsizei numPaths, const GLuint* paths, const GLfloat* weights ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %p, %p)\n", resultPath, numPaths, paths, weights );
  funcs->ext.p_glWeightPathsNV( resultPath, numPaths, paths, weights );
}

static void WINAPI wine_glWeightPointerARB( GLint size, GLenum type, GLsizei stride, const GLvoid* pointer ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %p)\n", size, type, stride, pointer );
  funcs->ext.p_glWeightPointerARB( size, type, stride, pointer );
}

static void WINAPI wine_glWeightbvARB( GLint size, const GLbyte* weights ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", size, weights );
  funcs->ext.p_glWeightbvARB( size, weights );
}

static void WINAPI wine_glWeightdvARB( GLint size, const GLdouble* weights ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", size, weights );
  funcs->ext.p_glWeightdvARB( size, weights );
}

static void WINAPI wine_glWeightfvARB( GLint size, const GLfloat* weights ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", size, weights );
  funcs->ext.p_glWeightfvARB( size, weights );
}

static void WINAPI wine_glWeightivARB( GLint size, const GLint* weights ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", size, weights );
  funcs->ext.p_glWeightivARB( size, weights );
}

static void WINAPI wine_glWeightsvARB( GLint size, const GLshort* weights ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", size, weights );
  funcs->ext.p_glWeightsvARB( size, weights );
}

static void WINAPI wine_glWeightubvARB( GLint size, const GLubyte* weights ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", size, weights );
  funcs->ext.p_glWeightubvARB( size, weights );
}

static void WINAPI wine_glWeightuivARB( GLint size, const GLuint* weights ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", size, weights );
  funcs->ext.p_glWeightuivARB( size, weights );
}

static void WINAPI wine_glWeightusvARB( GLint size, const GLushort* weights ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %p)\n", size, weights );
  funcs->ext.p_glWeightusvARB( size, weights );
}

static void WINAPI wine_glWindowPos2d( GLdouble x, GLdouble y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f)\n", x, y );
  funcs->ext.p_glWindowPos2d( x, y );
}

static void WINAPI wine_glWindowPos2dARB( GLdouble x, GLdouble y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f)\n", x, y );
  funcs->ext.p_glWindowPos2dARB( x, y );
}

static void WINAPI wine_glWindowPos2dMESA( GLdouble x, GLdouble y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f)\n", x, y );
  funcs->ext.p_glWindowPos2dMESA( x, y );
}

static void WINAPI wine_glWindowPos2dv( const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glWindowPos2dv( v );
}

static void WINAPI wine_glWindowPos2dvARB( const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glWindowPos2dvARB( v );
}

static void WINAPI wine_glWindowPos2dvMESA( const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glWindowPos2dvMESA( v );
}

static void WINAPI wine_glWindowPos2f( GLfloat x, GLfloat y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f)\n", x, y );
  funcs->ext.p_glWindowPos2f( x, y );
}

static void WINAPI wine_glWindowPos2fARB( GLfloat x, GLfloat y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f)\n", x, y );
  funcs->ext.p_glWindowPos2fARB( x, y );
}

static void WINAPI wine_glWindowPos2fMESA( GLfloat x, GLfloat y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f)\n", x, y );
  funcs->ext.p_glWindowPos2fMESA( x, y );
}

static void WINAPI wine_glWindowPos2fv( const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glWindowPos2fv( v );
}

static void WINAPI wine_glWindowPos2fvARB( const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glWindowPos2fvARB( v );
}

static void WINAPI wine_glWindowPos2fvMESA( const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glWindowPos2fvMESA( v );
}

static void WINAPI wine_glWindowPos2i( GLint x, GLint y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", x, y );
  funcs->ext.p_glWindowPos2i( x, y );
}

static void WINAPI wine_glWindowPos2iARB( GLint x, GLint y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", x, y );
  funcs->ext.p_glWindowPos2iARB( x, y );
}

static void WINAPI wine_glWindowPos2iMESA( GLint x, GLint y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", x, y );
  funcs->ext.p_glWindowPos2iMESA( x, y );
}

static void WINAPI wine_glWindowPos2iv( const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glWindowPos2iv( v );
}

static void WINAPI wine_glWindowPos2ivARB( const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glWindowPos2ivARB( v );
}

static void WINAPI wine_glWindowPos2ivMESA( const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glWindowPos2ivMESA( v );
}

static void WINAPI wine_glWindowPos2s( GLshort x, GLshort y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", x, y );
  funcs->ext.p_glWindowPos2s( x, y );
}

static void WINAPI wine_glWindowPos2sARB( GLshort x, GLshort y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", x, y );
  funcs->ext.p_glWindowPos2sARB( x, y );
}

static void WINAPI wine_glWindowPos2sMESA( GLshort x, GLshort y ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d)\n", x, y );
  funcs->ext.p_glWindowPos2sMESA( x, y );
}

static void WINAPI wine_glWindowPos2sv( const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glWindowPos2sv( v );
}

static void WINAPI wine_glWindowPos2svARB( const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glWindowPos2svARB( v );
}

static void WINAPI wine_glWindowPos2svMESA( const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glWindowPos2svMESA( v );
}

static void WINAPI wine_glWindowPos3d( GLdouble x, GLdouble y, GLdouble z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f)\n", x, y, z );
  funcs->ext.p_glWindowPos3d( x, y, z );
}

static void WINAPI wine_glWindowPos3dARB( GLdouble x, GLdouble y, GLdouble z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f)\n", x, y, z );
  funcs->ext.p_glWindowPos3dARB( x, y, z );
}

static void WINAPI wine_glWindowPos3dMESA( GLdouble x, GLdouble y, GLdouble z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f)\n", x, y, z );
  funcs->ext.p_glWindowPos3dMESA( x, y, z );
}

static void WINAPI wine_glWindowPos3dv( const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glWindowPos3dv( v );
}

static void WINAPI wine_glWindowPos3dvARB( const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glWindowPos3dvARB( v );
}

static void WINAPI wine_glWindowPos3dvMESA( const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glWindowPos3dvMESA( v );
}

static void WINAPI wine_glWindowPos3f( GLfloat x, GLfloat y, GLfloat z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f)\n", x, y, z );
  funcs->ext.p_glWindowPos3f( x, y, z );
}

static void WINAPI wine_glWindowPos3fARB( GLfloat x, GLfloat y, GLfloat z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f)\n", x, y, z );
  funcs->ext.p_glWindowPos3fARB( x, y, z );
}

static void WINAPI wine_glWindowPos3fMESA( GLfloat x, GLfloat y, GLfloat z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f)\n", x, y, z );
  funcs->ext.p_glWindowPos3fMESA( x, y, z );
}

static void WINAPI wine_glWindowPos3fv( const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glWindowPos3fv( v );
}

static void WINAPI wine_glWindowPos3fvARB( const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glWindowPos3fvARB( v );
}

static void WINAPI wine_glWindowPos3fvMESA( const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glWindowPos3fvMESA( v );
}

static void WINAPI wine_glWindowPos3i( GLint x, GLint y, GLint z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", x, y, z );
  funcs->ext.p_glWindowPos3i( x, y, z );
}

static void WINAPI wine_glWindowPos3iARB( GLint x, GLint y, GLint z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", x, y, z );
  funcs->ext.p_glWindowPos3iARB( x, y, z );
}

static void WINAPI wine_glWindowPos3iMESA( GLint x, GLint y, GLint z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", x, y, z );
  funcs->ext.p_glWindowPos3iMESA( x, y, z );
}

static void WINAPI wine_glWindowPos3iv( const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glWindowPos3iv( v );
}

static void WINAPI wine_glWindowPos3ivARB( const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glWindowPos3ivARB( v );
}

static void WINAPI wine_glWindowPos3ivMESA( const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glWindowPos3ivMESA( v );
}

static void WINAPI wine_glWindowPos3s( GLshort x, GLshort y, GLshort z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", x, y, z );
  funcs->ext.p_glWindowPos3s( x, y, z );
}

static void WINAPI wine_glWindowPos3sARB( GLshort x, GLshort y, GLshort z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", x, y, z );
  funcs->ext.p_glWindowPos3sARB( x, y, z );
}

static void WINAPI wine_glWindowPos3sMESA( GLshort x, GLshort y, GLshort z ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d)\n", x, y, z );
  funcs->ext.p_glWindowPos3sMESA( x, y, z );
}

static void WINAPI wine_glWindowPos3sv( const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glWindowPos3sv( v );
}

static void WINAPI wine_glWindowPos3svARB( const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glWindowPos3svARB( v );
}

static void WINAPI wine_glWindowPos3svMESA( const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glWindowPos3svMESA( v );
}

static void WINAPI wine_glWindowPos4dMESA( GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f, %f)\n", x, y, z, w );
  funcs->ext.p_glWindowPos4dMESA( x, y, z, w );
}

static void WINAPI wine_glWindowPos4dvMESA( const GLdouble* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glWindowPos4dvMESA( v );
}

static void WINAPI wine_glWindowPos4fMESA( GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%f, %f, %f, %f)\n", x, y, z, w );
  funcs->ext.p_glWindowPos4fMESA( x, y, z, w );
}

static void WINAPI wine_glWindowPos4fvMESA( const GLfloat* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glWindowPos4fvMESA( v );
}

static void WINAPI wine_glWindowPos4iMESA( GLint x, GLint y, GLint z, GLint w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", x, y, z, w );
  funcs->ext.p_glWindowPos4iMESA( x, y, z, w );
}

static void WINAPI wine_glWindowPos4ivMESA( const GLint* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glWindowPos4ivMESA( v );
}

static void WINAPI wine_glWindowPos4sMESA( GLshort x, GLshort y, GLshort z, GLshort w ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d)\n", x, y, z, w );
  funcs->ext.p_glWindowPos4sMESA( x, y, z, w );
}

static void WINAPI wine_glWindowPos4svMESA( const GLshort* v ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%p)\n", v );
  funcs->ext.p_glWindowPos4svMESA( v );
}

static void WINAPI wine_glWriteMaskEXT( GLuint res, GLuint in, GLenum outX, GLenum outY, GLenum outZ, GLenum outW ) {
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE("(%d, %d, %d, %d, %d, %d)\n", res, in, outX, outY, outZ, outW );
  funcs->ext.p_glWriteMaskEXT( res, in, outX, outY, outZ, outW );
}

const OpenGL_extension extension_registry[2085] = {
  { "glActiveProgramEXT", "GL_EXT_separate_shader_objects", wine_glActiveProgramEXT },
  { "glActiveShaderProgram", "GL_ARB_separate_shader_objects", wine_glActiveShaderProgram },
  { "glActiveStencilFaceEXT", "GL_EXT_stencil_two_side", wine_glActiveStencilFaceEXT },
  { "glActiveTexture", "GL_VERSION_1_3", wine_glActiveTexture },
  { "glActiveTextureARB", "GL_ARB_multitexture", wine_glActiveTextureARB },
  { "glActiveVaryingNV", "GL_NV_transform_feedback", wine_glActiveVaryingNV },
  { "glAlphaFragmentOp1ATI", "GL_ATI_fragment_shader", wine_glAlphaFragmentOp1ATI },
  { "glAlphaFragmentOp2ATI", "GL_ATI_fragment_shader", wine_glAlphaFragmentOp2ATI },
  { "glAlphaFragmentOp3ATI", "GL_ATI_fragment_shader", wine_glAlphaFragmentOp3ATI },
  { "glApplyTextureEXT", "GL_EXT_light_texture", wine_glApplyTextureEXT },
  { "glAreProgramsResidentNV", "GL_NV_vertex_program", wine_glAreProgramsResidentNV },
  { "glAreTexturesResidentEXT", "GL_EXT_texture_object", wine_glAreTexturesResidentEXT },
  { "glArrayElementEXT", "GL_EXT_vertex_array", wine_glArrayElementEXT },
  { "glArrayObjectATI", "GL_ATI_vertex_array_object", wine_glArrayObjectATI },
  { "glAsyncMarkerSGIX", "GL_SGIX_async", wine_glAsyncMarkerSGIX },
  { "glAttachObjectARB", "GL_ARB_shader_objects", wine_glAttachObjectARB },
  { "glAttachShader", "GL_VERSION_2_0", wine_glAttachShader },
  { "glBeginConditionalRender", "GL_VERSION_3_0", wine_glBeginConditionalRender },
  { "glBeginConditionalRenderNV", "GL_NV_conditional_render", wine_glBeginConditionalRenderNV },
  { "glBeginFragmentShaderATI", "GL_ATI_fragment_shader", wine_glBeginFragmentShaderATI },
  { "glBeginOcclusionQueryNV", "GL_NV_occlusion_query", wine_glBeginOcclusionQueryNV },
  { "glBeginPerfMonitorAMD", "GL_AMD_performance_monitor", wine_glBeginPerfMonitorAMD },
  { "glBeginQuery", "GL_VERSION_1_5", wine_glBeginQuery },
  { "glBeginQueryARB", "GL_ARB_occlusion_query", wine_glBeginQueryARB },
  { "glBeginQueryIndexed", "GL_ARB_transform_feedback3", wine_glBeginQueryIndexed },
  { "glBeginTransformFeedback", "GL_VERSION_3_0", wine_glBeginTransformFeedback },
  { "glBeginTransformFeedbackEXT", "GL_EXT_transform_feedback", wine_glBeginTransformFeedbackEXT },
  { "glBeginTransformFeedbackNV", "GL_NV_transform_feedback", wine_glBeginTransformFeedbackNV },
  { "glBeginVertexShaderEXT", "GL_EXT_vertex_shader", wine_glBeginVertexShaderEXT },
  { "glBeginVideoCaptureNV", "GL_NV_video_capture", wine_glBeginVideoCaptureNV },
  { "glBindAttribLocation", "GL_VERSION_2_0", wine_glBindAttribLocation },
  { "glBindAttribLocationARB", "GL_ARB_vertex_shader", wine_glBindAttribLocationARB },
  { "glBindBuffer", "GL_VERSION_1_5", wine_glBindBuffer },
  { "glBindBufferARB", "GL_ARB_vertex_buffer_object", wine_glBindBufferARB },
  { "glBindBufferBase", "GL_VERSION_3_0", wine_glBindBufferBase },
  { "glBindBufferBaseEXT", "GL_EXT_transform_feedback", wine_glBindBufferBaseEXT },
  { "glBindBufferBaseNV", "GL_NV_transform_feedback", wine_glBindBufferBaseNV },
  { "glBindBufferOffsetEXT", "GL_EXT_transform_feedback", wine_glBindBufferOffsetEXT },
  { "glBindBufferOffsetNV", "GL_NV_transform_feedback", wine_glBindBufferOffsetNV },
  { "glBindBufferRange", "GL_VERSION_3_0", wine_glBindBufferRange },
  { "glBindBufferRangeEXT", "GL_EXT_transform_feedback", wine_glBindBufferRangeEXT },
  { "glBindBufferRangeNV", "GL_NV_transform_feedback", wine_glBindBufferRangeNV },
  { "glBindFragDataLocation", "GL_VERSION_3_0", wine_glBindFragDataLocation },
  { "glBindFragDataLocationEXT", "GL_EXT_gpu_shader4", wine_glBindFragDataLocationEXT },
  { "glBindFragDataLocationIndexed", "GL_ARB_blend_func_extended", wine_glBindFragDataLocationIndexed },
  { "glBindFragmentShaderATI", "GL_ATI_fragment_shader", wine_glBindFragmentShaderATI },
  { "glBindFramebuffer", "GL_ARB_framebuffer_object", wine_glBindFramebuffer },
  { "glBindFramebufferEXT", "GL_EXT_framebuffer_object", wine_glBindFramebufferEXT },
  { "glBindImageTexture", "GL_ARB_shader_image_load_store", wine_glBindImageTexture },
  { "glBindImageTextureEXT", "GL_EXT_shader_image_load_store", wine_glBindImageTextureEXT },
  { "glBindLightParameterEXT", "GL_EXT_vertex_shader", wine_glBindLightParameterEXT },
  { "glBindMaterialParameterEXT", "GL_EXT_vertex_shader", wine_glBindMaterialParameterEXT },
  { "glBindMultiTextureEXT", "GL_EXT_direct_state_access", wine_glBindMultiTextureEXT },
  { "glBindParameterEXT", "GL_EXT_vertex_shader", wine_glBindParameterEXT },
  { "glBindProgramARB", "GL_ARB_vertex_program", wine_glBindProgramARB },
  { "glBindProgramNV", "GL_NV_vertex_program", wine_glBindProgramNV },
  { "glBindProgramPipeline", "GL_ARB_separate_shader_objects", wine_glBindProgramPipeline },
  { "glBindRenderbuffer", "GL_ARB_framebuffer_object", wine_glBindRenderbuffer },
  { "glBindRenderbufferEXT", "GL_EXT_framebuffer_object", wine_glBindRenderbufferEXT },
  { "glBindSampler", "GL_ARB_sampler_objects", wine_glBindSampler },
  { "glBindTexGenParameterEXT", "GL_EXT_vertex_shader", wine_glBindTexGenParameterEXT },
  { "glBindTextureEXT", "GL_EXT_texture_object", wine_glBindTextureEXT },
  { "glBindTextureUnitParameterEXT", "GL_EXT_vertex_shader", wine_glBindTextureUnitParameterEXT },
  { "glBindTransformFeedback", "GL_ARB_transform_feedback2", wine_glBindTransformFeedback },
  { "glBindTransformFeedbackNV", "GL_NV_transform_feedback2", wine_glBindTransformFeedbackNV },
  { "glBindVertexArray", "GL_ARB_vertex_array_object", wine_glBindVertexArray },
  { "glBindVertexArrayAPPLE", "GL_APPLE_vertex_array_object", wine_glBindVertexArrayAPPLE },
  { "glBindVertexShaderEXT", "GL_EXT_vertex_shader", wine_glBindVertexShaderEXT },
  { "glBindVideoCaptureStreamBufferNV", "GL_NV_video_capture", wine_glBindVideoCaptureStreamBufferNV },
  { "glBindVideoCaptureStreamTextureNV", "GL_NV_video_capture", wine_glBindVideoCaptureStreamTextureNV },
  { "glBinormal3bEXT", "GL_EXT_coordinate_frame", wine_glBinormal3bEXT },
  { "glBinormal3bvEXT", "GL_EXT_coordinate_frame", wine_glBinormal3bvEXT },
  { "glBinormal3dEXT", "GL_EXT_coordinate_frame", wine_glBinormal3dEXT },
  { "glBinormal3dvEXT", "GL_EXT_coordinate_frame", wine_glBinormal3dvEXT },
  { "glBinormal3fEXT", "GL_EXT_coordinate_frame", wine_glBinormal3fEXT },
  { "glBinormal3fvEXT", "GL_EXT_coordinate_frame", wine_glBinormal3fvEXT },
  { "glBinormal3iEXT", "GL_EXT_coordinate_frame", wine_glBinormal3iEXT },
  { "glBinormal3ivEXT", "GL_EXT_coordinate_frame", wine_glBinormal3ivEXT },
  { "glBinormal3sEXT", "GL_EXT_coordinate_frame", wine_glBinormal3sEXT },
  { "glBinormal3svEXT", "GL_EXT_coordinate_frame", wine_glBinormal3svEXT },
  { "glBinormalPointerEXT", "GL_EXT_coordinate_frame", wine_glBinormalPointerEXT },
  { "glBlendColor", "GL_VERSION_1_2", wine_glBlendColor },
  { "glBlendColorEXT", "GL_EXT_blend_color", wine_glBlendColorEXT },
  { "glBlendEquation", "GL_VERSION_1_2", wine_glBlendEquation },
  { "glBlendEquationEXT", "GL_EXT_blend_minmax", wine_glBlendEquationEXT },
  { "glBlendEquationIndexedAMD", "GL_AMD_draw_buffers_blend", wine_glBlendEquationIndexedAMD },
  { "glBlendEquationSeparate", "GL_VERSION_2_0", wine_glBlendEquationSeparate },
  { "glBlendEquationSeparateEXT", "GL_EXT_blend_equation_separate", wine_glBlendEquationSeparateEXT },
  { "glBlendEquationSeparateIndexedAMD", "GL_AMD_draw_buffers_blend", wine_glBlendEquationSeparateIndexedAMD },
  { "glBlendEquationSeparatei", "GL_VERSION_4_0", wine_glBlendEquationSeparatei },
  { "glBlendEquationSeparateiARB", "GL_ARB_draw_buffers_blend", wine_glBlendEquationSeparateiARB },
  { "glBlendEquationi", "GL_VERSION_4_0", wine_glBlendEquationi },
  { "glBlendEquationiARB", "GL_ARB_draw_buffers_blend", wine_glBlendEquationiARB },
  { "glBlendFuncIndexedAMD", "GL_AMD_draw_buffers_blend", wine_glBlendFuncIndexedAMD },
  { "glBlendFuncSeparate", "GL_VERSION_1_4", wine_glBlendFuncSeparate },
  { "glBlendFuncSeparateEXT", "GL_EXT_blend_func_separate", wine_glBlendFuncSeparateEXT },
  { "glBlendFuncSeparateINGR", "GL_INGR_blend_func_separate", wine_glBlendFuncSeparateINGR },
  { "glBlendFuncSeparateIndexedAMD", "GL_AMD_draw_buffers_blend", wine_glBlendFuncSeparateIndexedAMD },
  { "glBlendFuncSeparatei", "GL_VERSION_4_0", wine_glBlendFuncSeparatei },
  { "glBlendFuncSeparateiARB", "GL_ARB_draw_buffers_blend", wine_glBlendFuncSeparateiARB },
  { "glBlendFunci", "GL_VERSION_4_0", wine_glBlendFunci },
  { "glBlendFunciARB", "GL_ARB_draw_buffers_blend", wine_glBlendFunciARB },
  { "glBlitFramebuffer", "GL_ARB_framebuffer_object", wine_glBlitFramebuffer },
  { "glBlitFramebufferEXT", "GL_EXT_framebuffer_blit", wine_glBlitFramebufferEXT },
  { "glBufferAddressRangeNV", "GL_NV_vertex_buffer_unified_memory", wine_glBufferAddressRangeNV },
  { "glBufferData", "GL_VERSION_1_5", wine_glBufferData },
  { "glBufferDataARB", "GL_ARB_vertex_buffer_object", wine_glBufferDataARB },
  { "glBufferParameteriAPPLE", "GL_APPLE_flush_buffer_range", wine_glBufferParameteriAPPLE },
  { "glBufferRegionEnabled", "GL_KTX_buffer_region", wine_glBufferRegionEnabled },
  { "glBufferSubData", "GL_VERSION_1_5", wine_glBufferSubData },
  { "glBufferSubDataARB", "GL_ARB_vertex_buffer_object", wine_glBufferSubDataARB },
  { "glCheckFramebufferStatus", "GL_ARB_framebuffer_object", wine_glCheckFramebufferStatus },
  { "glCheckFramebufferStatusEXT", "GL_EXT_framebuffer_object", wine_glCheckFramebufferStatusEXT },
  { "glCheckNamedFramebufferStatusEXT", "GL_EXT_direct_state_access", wine_glCheckNamedFramebufferStatusEXT },
  { "glClampColor", "GL_VERSION_3_0", wine_glClampColor },
  { "glClampColorARB", "GL_ARB_color_buffer_float", wine_glClampColorARB },
  { "glClearBufferfi", "GL_VERSION_3_0", wine_glClearBufferfi },
  { "glClearBufferfv", "GL_VERSION_3_0", wine_glClearBufferfv },
  { "glClearBufferiv", "GL_VERSION_3_0", wine_glClearBufferiv },
  { "glClearBufferuiv", "GL_VERSION_3_0", wine_glClearBufferuiv },
  { "glClearColorIiEXT", "GL_EXT_texture_integer", wine_glClearColorIiEXT },
  { "glClearColorIuiEXT", "GL_EXT_texture_integer", wine_glClearColorIuiEXT },
  { "glClearDepthdNV", "GL_NV_depth_buffer_float", wine_glClearDepthdNV },
  { "glClearDepthf", "GL_ARB_ES2_compatibility", wine_glClearDepthf },
  { "glClientActiveTexture", "GL_VERSION_1_3_DEPRECATED", wine_glClientActiveTexture },
  { "glClientActiveTextureARB", "GL_ARB_multitexture", wine_glClientActiveTextureARB },
  { "glClientActiveVertexStreamATI", "GL_ATI_vertex_streams", wine_glClientActiveVertexStreamATI },
  { "glClientAttribDefaultEXT", "GL_EXT_direct_state_access", wine_glClientAttribDefaultEXT },
  { "glClientWaitSync", "GL_ARB_sync", wine_glClientWaitSync },
  { "glColor3fVertex3fSUN", "GL_SUN_vertex", wine_glColor3fVertex3fSUN },
  { "glColor3fVertex3fvSUN", "GL_SUN_vertex", wine_glColor3fVertex3fvSUN },
  { "glColor3hNV", "GL_NV_half_float", wine_glColor3hNV },
  { "glColor3hvNV", "GL_NV_half_float", wine_glColor3hvNV },
  { "glColor4fNormal3fVertex3fSUN", "GL_SUN_vertex", wine_glColor4fNormal3fVertex3fSUN },
  { "glColor4fNormal3fVertex3fvSUN", "GL_SUN_vertex", wine_glColor4fNormal3fVertex3fvSUN },
  { "glColor4hNV", "GL_NV_half_float", wine_glColor4hNV },
  { "glColor4hvNV", "GL_NV_half_float", wine_glColor4hvNV },
  { "glColor4ubVertex2fSUN", "GL_SUN_vertex", wine_glColor4ubVertex2fSUN },
  { "glColor4ubVertex2fvSUN", "GL_SUN_vertex", wine_glColor4ubVertex2fvSUN },
  { "glColor4ubVertex3fSUN", "GL_SUN_vertex", wine_glColor4ubVertex3fSUN },
  { "glColor4ubVertex3fvSUN", "GL_SUN_vertex", wine_glColor4ubVertex3fvSUN },
  { "glColorFormatNV", "GL_NV_vertex_buffer_unified_memory", wine_glColorFormatNV },
  { "glColorFragmentOp1ATI", "GL_ATI_fragment_shader", wine_glColorFragmentOp1ATI },
  { "glColorFragmentOp2ATI", "GL_ATI_fragment_shader", wine_glColorFragmentOp2ATI },
  { "glColorFragmentOp3ATI", "GL_ATI_fragment_shader", wine_glColorFragmentOp3ATI },
  { "glColorMaskIndexedEXT", "GL_EXT_draw_buffers2", wine_glColorMaskIndexedEXT },
  { "glColorMaski", "GL_VERSION_3_0", wine_glColorMaski },
  { "glColorP3ui", "GL_ARB_vertex_type_2_10_10_10_rev", wine_glColorP3ui },
  { "glColorP3uiv", "GL_ARB_vertex_type_2_10_10_10_rev", wine_glColorP3uiv },
  { "glColorP4ui", "GL_ARB_vertex_type_2_10_10_10_rev", wine_glColorP4ui },
  { "glColorP4uiv", "GL_ARB_vertex_type_2_10_10_10_rev", wine_glColorP4uiv },
  { "glColorPointerEXT", "GL_EXT_vertex_array", wine_glColorPointerEXT },
  { "glColorPointerListIBM", "GL_IBM_vertex_array_lists", wine_glColorPointerListIBM },
  { "glColorPointervINTEL", "GL_INTEL_parallel_arrays", wine_glColorPointervINTEL },
  { "glColorSubTable", "GL_VERSION_1_2_DEPRECATED", wine_glColorSubTable },
  { "glColorSubTableEXT", "GL_EXT_color_subtable", wine_glColorSubTableEXT },
  { "glColorTable", "GL_VERSION_1_2_DEPRECATED", wine_glColorTable },
  { "glColorTableEXT", "GL_EXT_paletted_texture", wine_glColorTableEXT },
  { "glColorTableParameterfv", "GL_VERSION_1_2_DEPRECATED", wine_glColorTableParameterfv },
  { "glColorTableParameterfvSGI", "GL_SGI_color_table", wine_glColorTableParameterfvSGI },
  { "glColorTableParameteriv", "GL_VERSION_1_2_DEPRECATED", wine_glColorTableParameteriv },
  { "glColorTableParameterivSGI", "GL_SGI_color_table", wine_glColorTableParameterivSGI },
  { "glColorTableSGI", "GL_SGI_color_table", wine_glColorTableSGI },
  { "glCombinerInputNV", "GL_NV_register_combiners", wine_glCombinerInputNV },
  { "glCombinerOutputNV", "GL_NV_register_combiners", wine_glCombinerOutputNV },
  { "glCombinerParameterfNV", "GL_NV_register_combiners", wine_glCombinerParameterfNV },
  { "glCombinerParameterfvNV", "GL_NV_register_combiners", wine_glCombinerParameterfvNV },
  { "glCombinerParameteriNV", "GL_NV_register_combiners", wine_glCombinerParameteriNV },
  { "glCombinerParameterivNV", "GL_NV_register_combiners", wine_glCombinerParameterivNV },
  { "glCombinerStageParameterfvNV", "GL_NV_register_combiners2", wine_glCombinerStageParameterfvNV },
  { "glCompileShader", "GL_VERSION_2_0", wine_glCompileShader },
  { "glCompileShaderARB", "GL_ARB_shader_objects", wine_glCompileShaderARB },
  { "glCompileShaderIncludeARB", "GL_ARB_shading_language_include", wine_glCompileShaderIncludeARB },
  { "glCompressedMultiTexImage1DEXT", "GL_EXT_direct_state_access", wine_glCompressedMultiTexImage1DEXT },
  { "glCompressedMultiTexImage2DEXT", "GL_EXT_direct_state_access", wine_glCompressedMultiTexImage2DEXT },
  { "glCompressedMultiTexImage3DEXT", "GL_EXT_direct_state_access", wine_glCompressedMultiTexImage3DEXT },
  { "glCompressedMultiTexSubImage1DEXT", "GL_EXT_direct_state_access", wine_glCompressedMultiTexSubImage1DEXT },
  { "glCompressedMultiTexSubImage2DEXT", "GL_EXT_direct_state_access", wine_glCompressedMultiTexSubImage2DEXT },
  { "glCompressedMultiTexSubImage3DEXT", "GL_EXT_direct_state_access", wine_glCompressedMultiTexSubImage3DEXT },
  { "glCompressedTexImage1D", "GL_VERSION_1_3", wine_glCompressedTexImage1D },
  { "glCompressedTexImage1DARB", "GL_ARB_texture_compression", wine_glCompressedTexImage1DARB },
  { "glCompressedTexImage2D", "GL_VERSION_1_3", wine_glCompressedTexImage2D },
  { "glCompressedTexImage2DARB", "GL_ARB_texture_compression", wine_glCompressedTexImage2DARB },
  { "glCompressedTexImage3D", "GL_VERSION_1_3", wine_glCompressedTexImage3D },
  { "glCompressedTexImage3DARB", "GL_ARB_texture_compression", wine_glCompressedTexImage3DARB },
  { "glCompressedTexSubImage1D", "GL_VERSION_1_3", wine_glCompressedTexSubImage1D },
  { "glCompressedTexSubImage1DARB", "GL_ARB_texture_compression", wine_glCompressedTexSubImage1DARB },
  { "glCompressedTexSubImage2D", "GL_VERSION_1_3", wine_glCompressedTexSubImage2D },
  { "glCompressedTexSubImage2DARB", "GL_ARB_texture_compression", wine_glCompressedTexSubImage2DARB },
  { "glCompressedTexSubImage3D", "GL_VERSION_1_3", wine_glCompressedTexSubImage3D },
  { "glCompressedTexSubImage3DARB", "GL_ARB_texture_compression", wine_glCompressedTexSubImage3DARB },
  { "glCompressedTextureImage1DEXT", "GL_EXT_direct_state_access", wine_glCompressedTextureImage1DEXT },
  { "glCompressedTextureImage2DEXT", "GL_EXT_direct_state_access", wine_glCompressedTextureImage2DEXT },
  { "glCompressedTextureImage3DEXT", "GL_EXT_direct_state_access", wine_glCompressedTextureImage3DEXT },
  { "glCompressedTextureSubImage1DEXT", "GL_EXT_direct_state_access", wine_glCompressedTextureSubImage1DEXT },
  { "glCompressedTextureSubImage2DEXT", "GL_EXT_direct_state_access", wine_glCompressedTextureSubImage2DEXT },
  { "glCompressedTextureSubImage3DEXT", "GL_EXT_direct_state_access", wine_glCompressedTextureSubImage3DEXT },
  { "glConvolutionFilter1D", "GL_VERSION_1_2_DEPRECATED", wine_glConvolutionFilter1D },
  { "glConvolutionFilter1DEXT", "GL_EXT_convolution", wine_glConvolutionFilter1DEXT },
  { "glConvolutionFilter2D", "GL_VERSION_1_2_DEPRECATED", wine_glConvolutionFilter2D },
  { "glConvolutionFilter2DEXT", "GL_EXT_convolution", wine_glConvolutionFilter2DEXT },
  { "glConvolutionParameterf", "GL_VERSION_1_2_DEPRECATED", wine_glConvolutionParameterf },
  { "glConvolutionParameterfEXT", "GL_EXT_convolution", wine_glConvolutionParameterfEXT },
  { "glConvolutionParameterfv", "GL_VERSION_1_2_DEPRECATED", wine_glConvolutionParameterfv },
  { "glConvolutionParameterfvEXT", "GL_EXT_convolution", wine_glConvolutionParameterfvEXT },
  { "glConvolutionParameteri", "GL_VERSION_1_2_DEPRECATED", wine_glConvolutionParameteri },
  { "glConvolutionParameteriEXT", "GL_EXT_convolution", wine_glConvolutionParameteriEXT },
  { "glConvolutionParameteriv", "GL_VERSION_1_2_DEPRECATED", wine_glConvolutionParameteriv },
  { "glConvolutionParameterivEXT", "GL_EXT_convolution", wine_glConvolutionParameterivEXT },
  { "glCopyBufferSubData", "GL_ARB_copy_buffer", wine_glCopyBufferSubData },
  { "glCopyColorSubTable", "GL_VERSION_1_2_DEPRECATED", wine_glCopyColorSubTable },
  { "glCopyColorSubTableEXT", "GL_EXT_color_subtable", wine_glCopyColorSubTableEXT },
  { "glCopyColorTable", "GL_VERSION_1_2_DEPRECATED", wine_glCopyColorTable },
  { "glCopyColorTableSGI", "GL_SGI_color_table", wine_glCopyColorTableSGI },
  { "glCopyConvolutionFilter1D", "GL_VERSION_1_2_DEPRECATED", wine_glCopyConvolutionFilter1D },
  { "glCopyConvolutionFilter1DEXT", "GL_EXT_convolution", wine_glCopyConvolutionFilter1DEXT },
  { "glCopyConvolutionFilter2D", "GL_VERSION_1_2_DEPRECATED", wine_glCopyConvolutionFilter2D },
  { "glCopyConvolutionFilter2DEXT", "GL_EXT_convolution", wine_glCopyConvolutionFilter2DEXT },
  { "glCopyImageSubDataNV", "GL_NV_copy_image", wine_glCopyImageSubDataNV },
  { "glCopyMultiTexImage1DEXT", "GL_EXT_direct_state_access", wine_glCopyMultiTexImage1DEXT },
  { "glCopyMultiTexImage2DEXT", "GL_EXT_direct_state_access", wine_glCopyMultiTexImage2DEXT },
  { "glCopyMultiTexSubImage1DEXT", "GL_EXT_direct_state_access", wine_glCopyMultiTexSubImage1DEXT },
  { "glCopyMultiTexSubImage2DEXT", "GL_EXT_direct_state_access", wine_glCopyMultiTexSubImage2DEXT },
  { "glCopyMultiTexSubImage3DEXT", "GL_EXT_direct_state_access", wine_glCopyMultiTexSubImage3DEXT },
  { "glCopyPathNV", "GL_NV_path_rendering", wine_glCopyPathNV },
  { "glCopyTexImage1DEXT", "GL_EXT_copy_texture", wine_glCopyTexImage1DEXT },
  { "glCopyTexImage2DEXT", "GL_EXT_copy_texture", wine_glCopyTexImage2DEXT },
  { "glCopyTexSubImage1DEXT", "GL_EXT_copy_texture", wine_glCopyTexSubImage1DEXT },
  { "glCopyTexSubImage2DEXT", "GL_EXT_copy_texture", wine_glCopyTexSubImage2DEXT },
  { "glCopyTexSubImage3D", "GL_VERSION_1_2", wine_glCopyTexSubImage3D },
  { "glCopyTexSubImage3DEXT", "GL_EXT_copy_texture", wine_glCopyTexSubImage3DEXT },
  { "glCopyTextureImage1DEXT", "GL_EXT_direct_state_access", wine_glCopyTextureImage1DEXT },
  { "glCopyTextureImage2DEXT", "GL_EXT_direct_state_access", wine_glCopyTextureImage2DEXT },
  { "glCopyTextureSubImage1DEXT", "GL_EXT_direct_state_access", wine_glCopyTextureSubImage1DEXT },
  { "glCopyTextureSubImage2DEXT", "GL_EXT_direct_state_access", wine_glCopyTextureSubImage2DEXT },
  { "glCopyTextureSubImage3DEXT", "GL_EXT_direct_state_access", wine_glCopyTextureSubImage3DEXT },
  { "glCoverFillPathInstancedNV", "GL_NV_path_rendering", wine_glCoverFillPathInstancedNV },
  { "glCoverFillPathNV", "GL_NV_path_rendering", wine_glCoverFillPathNV },
  { "glCoverStrokePathInstancedNV", "GL_NV_path_rendering", wine_glCoverStrokePathInstancedNV },
  { "glCoverStrokePathNV", "GL_NV_path_rendering", wine_glCoverStrokePathNV },
  { "glCreateProgram", "GL_VERSION_2_0", wine_glCreateProgram },
  { "glCreateProgramObjectARB", "GL_ARB_shader_objects", wine_glCreateProgramObjectARB },
  { "glCreateShader", "GL_VERSION_2_0", wine_glCreateShader },
  { "glCreateShaderObjectARB", "GL_ARB_shader_objects", wine_glCreateShaderObjectARB },
  { "glCreateShaderProgramEXT", "GL_EXT_separate_shader_objects", wine_glCreateShaderProgramEXT },
  { "glCreateShaderProgramv", "GL_ARB_separate_shader_objects", wine_glCreateShaderProgramv },
  { "glCreateSyncFromCLeventARB", "GL_ARB_cl_event", wine_glCreateSyncFromCLeventARB },
  { "glCullParameterdvEXT", "GL_EXT_cull_vertex", wine_glCullParameterdvEXT },
  { "glCullParameterfvEXT", "GL_EXT_cull_vertex", wine_glCullParameterfvEXT },
  { "glCurrentPaletteMatrixARB", "GL_ARB_matrix_palette", wine_glCurrentPaletteMatrixARB },
  { "glDebugMessageCallbackAMD", "GL_AMD_debug_output", wine_glDebugMessageCallbackAMD },
  { "glDebugMessageCallbackARB", "GL_ARB_debug_output", wine_glDebugMessageCallbackARB },
  { "glDebugMessageControlARB", "GL_ARB_debug_output", wine_glDebugMessageControlARB },
  { "glDebugMessageEnableAMD", "GL_AMD_debug_output", wine_glDebugMessageEnableAMD },
  { "glDebugMessageInsertAMD", "GL_AMD_debug_output", wine_glDebugMessageInsertAMD },
  { "glDebugMessageInsertARB", "GL_ARB_debug_output", wine_glDebugMessageInsertARB },
  { "glDeformSGIX", "GL_SGIX_polynomial_ffd", wine_glDeformSGIX },
  { "glDeformationMap3dSGIX", "GL_SGIX_polynomial_ffd", wine_glDeformationMap3dSGIX },
  { "glDeformationMap3fSGIX", "GL_SGIX_polynomial_ffd", wine_glDeformationMap3fSGIX },
  { "glDeleteAsyncMarkersSGIX", "GL_SGIX_async", wine_glDeleteAsyncMarkersSGIX },
  { "glDeleteBufferRegion", "GL_KTX_buffer_region", wine_glDeleteBufferRegion },
  { "glDeleteBuffers", "GL_VERSION_1_5", wine_glDeleteBuffers },
  { "glDeleteBuffersARB", "GL_ARB_vertex_buffer_object", wine_glDeleteBuffersARB },
  { "glDeleteFencesAPPLE", "GL_APPLE_fence", wine_glDeleteFencesAPPLE },
  { "glDeleteFencesNV", "GL_NV_fence", wine_glDeleteFencesNV },
  { "glDeleteFragmentShaderATI", "GL_ATI_fragment_shader", wine_glDeleteFragmentShaderATI },
  { "glDeleteFramebuffers", "GL_ARB_framebuffer_object", wine_glDeleteFramebuffers },
  { "glDeleteFramebuffersEXT", "GL_EXT_framebuffer_object", wine_glDeleteFramebuffersEXT },
  { "glDeleteNamedStringARB", "GL_ARB_shading_language_include", wine_glDeleteNamedStringARB },
  { "glDeleteNamesAMD", "GL_AMD_name_gen_delete", wine_glDeleteNamesAMD },
  { "glDeleteObjectARB", "GL_ARB_shader_objects", wine_glDeleteObjectARB },
  { "glDeleteObjectBufferATI", "GL_ATI_vertex_array_object", wine_glDeleteObjectBufferATI },
  { "glDeleteOcclusionQueriesNV", "GL_NV_occlusion_query", wine_glDeleteOcclusionQueriesNV },
  { "glDeletePathsNV", "GL_NV_path_rendering", wine_glDeletePathsNV },
  { "glDeletePerfMonitorsAMD", "GL_AMD_performance_monitor", wine_glDeletePerfMonitorsAMD },
  { "glDeleteProgram", "GL_VERSION_2_0", wine_glDeleteProgram },
  { "glDeleteProgramPipelines", "GL_ARB_separate_shader_objects", wine_glDeleteProgramPipelines },
  { "glDeleteProgramsARB", "GL_ARB_vertex_program", wine_glDeleteProgramsARB },
  { "glDeleteProgramsNV", "GL_NV_vertex_program", wine_glDeleteProgramsNV },
  { "glDeleteQueries", "GL_VERSION_1_5", wine_glDeleteQueries },
  { "glDeleteQueriesARB", "GL_ARB_occlusion_query", wine_glDeleteQueriesARB },
  { "glDeleteRenderbuffers", "GL_ARB_framebuffer_object", wine_glDeleteRenderbuffers },
  { "glDeleteRenderbuffersEXT", "GL_EXT_framebuffer_object", wine_glDeleteRenderbuffersEXT },
  { "glDeleteSamplers", "GL_ARB_sampler_objects", wine_glDeleteSamplers },
  { "glDeleteShader", "GL_VERSION_2_0", wine_glDeleteShader },
  { "glDeleteSync", "GL_ARB_sync", wine_glDeleteSync },
  { "glDeleteTexturesEXT", "GL_EXT_texture_object", wine_glDeleteTexturesEXT },
  { "glDeleteTransformFeedbacks", "GL_ARB_transform_feedback2", wine_glDeleteTransformFeedbacks },
  { "glDeleteTransformFeedbacksNV", "GL_NV_transform_feedback2", wine_glDeleteTransformFeedbacksNV },
  { "glDeleteVertexArrays", "GL_ARB_vertex_array_object", wine_glDeleteVertexArrays },
  { "glDeleteVertexArraysAPPLE", "GL_APPLE_vertex_array_object", wine_glDeleteVertexArraysAPPLE },
  { "glDeleteVertexShaderEXT", "GL_EXT_vertex_shader", wine_glDeleteVertexShaderEXT },
  { "glDepthBoundsEXT", "GL_EXT_depth_bounds_test", wine_glDepthBoundsEXT },
  { "glDepthBoundsdNV", "GL_NV_depth_buffer_float", wine_glDepthBoundsdNV },
  { "glDepthRangeArrayv", "GL_ARB_viewport_array", wine_glDepthRangeArrayv },
  { "glDepthRangeIndexed", "GL_ARB_viewport_array", wine_glDepthRangeIndexed },
  { "glDepthRangedNV", "GL_NV_depth_buffer_float", wine_glDepthRangedNV },
  { "glDepthRangef", "GL_ARB_ES2_compatibility", wine_glDepthRangef },
  { "glDetachObjectARB", "GL_ARB_shader_objects", wine_glDetachObjectARB },
  { "glDetachShader", "GL_VERSION_2_0", wine_glDetachShader },
  { "glDetailTexFuncSGIS", "GL_SGIS_detail_texture", wine_glDetailTexFuncSGIS },
  { "glDisableClientStateIndexedEXT", "GL_EXT_direct_state_access", wine_glDisableClientStateIndexedEXT },
  { "glDisableIndexedEXT", "GL_EXT_draw_buffers2", wine_glDisableIndexedEXT },
  { "glDisableVariantClientStateEXT", "GL_EXT_vertex_shader", wine_glDisableVariantClientStateEXT },
  { "glDisableVertexAttribAPPLE", "GL_APPLE_vertex_program_evaluators", wine_glDisableVertexAttribAPPLE },
  { "glDisableVertexAttribArray", "GL_VERSION_2_0", wine_glDisableVertexAttribArray },
  { "glDisableVertexAttribArrayARB", "GL_ARB_vertex_program", wine_glDisableVertexAttribArrayARB },
  { "glDisablei", "GL_VERSION_3_0", wine_glDisablei },
  { "glDrawArraysEXT", "GL_EXT_vertex_array", wine_glDrawArraysEXT },
  { "glDrawArraysIndirect", "GL_ARB_draw_indirect", wine_glDrawArraysIndirect },
  { "glDrawArraysInstanced", "GL_VERSION_3_1", wine_glDrawArraysInstanced },
  { "glDrawArraysInstancedARB", "GL_ARB_draw_instanced", wine_glDrawArraysInstancedARB },
  { "glDrawArraysInstancedBaseInstance", "GL_ARB_base_instance", wine_glDrawArraysInstancedBaseInstance },
  { "glDrawArraysInstancedEXT", "GL_EXT_draw_instanced", wine_glDrawArraysInstancedEXT },
  { "glDrawBufferRegion", "GL_KTX_buffer_region", wine_glDrawBufferRegion },
  { "glDrawBuffers", "GL_VERSION_2_0", wine_glDrawBuffers },
  { "glDrawBuffersARB", "GL_ARB_draw_buffers", wine_glDrawBuffersARB },
  { "glDrawBuffersATI", "GL_ATI_draw_buffers", wine_glDrawBuffersATI },
  { "glDrawElementArrayAPPLE", "GL_APPLE_element_array", wine_glDrawElementArrayAPPLE },
  { "glDrawElementArrayATI", "GL_ATI_element_array", wine_glDrawElementArrayATI },
  { "glDrawElementsBaseVertex", "GL_ARB_draw_elements_base_vertex", wine_glDrawElementsBaseVertex },
  { "glDrawElementsIndirect", "GL_ARB_draw_indirect", wine_glDrawElementsIndirect },
  { "glDrawElementsInstanced", "GL_VERSION_3_1", wine_glDrawElementsInstanced },
  { "glDrawElementsInstancedARB", "GL_ARB_draw_instanced", wine_glDrawElementsInstancedARB },
  { "glDrawElementsInstancedBaseInstance", "GL_ARB_base_instance", wine_glDrawElementsInstancedBaseInstance },
  { "glDrawElementsInstancedBaseVertex", "GL_ARB_draw_elements_base_vertex", wine_glDrawElementsInstancedBaseVertex },
  { "glDrawElementsInstancedBaseVertexBaseInstance", "GL_ARB_base_instance", wine_glDrawElementsInstancedBaseVertexBaseInstance },
  { "glDrawElementsInstancedEXT", "GL_EXT_draw_instanced", wine_glDrawElementsInstancedEXT },
  { "glDrawMeshArraysSUN", "GL_SUN_mesh_array", wine_glDrawMeshArraysSUN },
  { "glDrawRangeElementArrayAPPLE", "GL_APPLE_element_array", wine_glDrawRangeElementArrayAPPLE },
  { "glDrawRangeElementArrayATI", "GL_ATI_element_array", wine_glDrawRangeElementArrayATI },
  { "glDrawRangeElements", "GL_VERSION_1_2", wine_glDrawRangeElements },
  { "glDrawRangeElementsBaseVertex", "GL_ARB_draw_elements_base_vertex", wine_glDrawRangeElementsBaseVertex },
  { "glDrawRangeElementsEXT", "GL_EXT_draw_range_elements", wine_glDrawRangeElementsEXT },
  { "glDrawTransformFeedback", "GL_ARB_transform_feedback2", wine_glDrawTransformFeedback },
  { "glDrawTransformFeedbackInstanced", "GL_ARB_transform_feedback_instanced", wine_glDrawTransformFeedbackInstanced },
  { "glDrawTransformFeedbackNV", "GL_NV_transform_feedback2", wine_glDrawTransformFeedbackNV },
  { "glDrawTransformFeedbackStream", "GL_ARB_transform_feedback3", wine_glDrawTransformFeedbackStream },
  { "glDrawTransformFeedbackStreamInstanced", "GL_ARB_transform_feedback_instanced", wine_glDrawTransformFeedbackStreamInstanced },
  { "glEdgeFlagFormatNV", "GL_NV_vertex_buffer_unified_memory", wine_glEdgeFlagFormatNV },
  { "glEdgeFlagPointerEXT", "GL_EXT_vertex_array", wine_glEdgeFlagPointerEXT },
  { "glEdgeFlagPointerListIBM", "GL_IBM_vertex_array_lists", wine_glEdgeFlagPointerListIBM },
  { "glElementPointerAPPLE", "GL_APPLE_element_array", wine_glElementPointerAPPLE },
  { "glElementPointerATI", "GL_ATI_element_array", wine_glElementPointerATI },
  { "glEnableClientStateIndexedEXT", "GL_EXT_direct_state_access", wine_glEnableClientStateIndexedEXT },
  { "glEnableIndexedEXT", "GL_EXT_draw_buffers2", wine_glEnableIndexedEXT },
  { "glEnableVariantClientStateEXT", "GL_EXT_vertex_shader", wine_glEnableVariantClientStateEXT },
  { "glEnableVertexAttribAPPLE", "GL_APPLE_vertex_program_evaluators", wine_glEnableVertexAttribAPPLE },
  { "glEnableVertexAttribArray", "GL_VERSION_2_0", wine_glEnableVertexAttribArray },
  { "glEnableVertexAttribArrayARB", "GL_ARB_vertex_program", wine_glEnableVertexAttribArrayARB },
  { "glEnablei", "GL_VERSION_3_0", wine_glEnablei },
  { "glEndConditionalRender", "GL_VERSION_3_0", wine_glEndConditionalRender },
  { "glEndConditionalRenderNV", "GL_NV_conditional_render", wine_glEndConditionalRenderNV },
  { "glEndFragmentShaderATI", "GL_ATI_fragment_shader", wine_glEndFragmentShaderATI },
  { "glEndOcclusionQueryNV", "GL_NV_occlusion_query", wine_glEndOcclusionQueryNV },
  { "glEndPerfMonitorAMD", "GL_AMD_performance_monitor", wine_glEndPerfMonitorAMD },
  { "glEndQuery", "GL_VERSION_1_5", wine_glEndQuery },
  { "glEndQueryARB", "GL_ARB_occlusion_query", wine_glEndQueryARB },
  { "glEndQueryIndexed", "GL_ARB_transform_feedback3", wine_glEndQueryIndexed },
  { "glEndTransformFeedback", "GL_VERSION_3_0", wine_glEndTransformFeedback },
  { "glEndTransformFeedbackEXT", "GL_EXT_transform_feedback", wine_glEndTransformFeedbackEXT },
  { "glEndTransformFeedbackNV", "GL_NV_transform_feedback", wine_glEndTransformFeedbackNV },
  { "glEndVertexShaderEXT", "GL_EXT_vertex_shader", wine_glEndVertexShaderEXT },
  { "glEndVideoCaptureNV", "GL_NV_video_capture", wine_glEndVideoCaptureNV },
  { "glEvalMapsNV", "GL_NV_evaluators", wine_glEvalMapsNV },
  { "glExecuteProgramNV", "GL_NV_vertex_program", wine_glExecuteProgramNV },
  { "glExtractComponentEXT", "GL_EXT_vertex_shader", wine_glExtractComponentEXT },
  { "glFenceSync", "GL_ARB_sync", wine_glFenceSync },
  { "glFinalCombinerInputNV", "GL_NV_register_combiners", wine_glFinalCombinerInputNV },
  { "glFinishAsyncSGIX", "GL_SGIX_async", wine_glFinishAsyncSGIX },
  { "glFinishFenceAPPLE", "GL_APPLE_fence", wine_glFinishFenceAPPLE },
  { "glFinishFenceNV", "GL_NV_fence", wine_glFinishFenceNV },
  { "glFinishObjectAPPLE", "GL_APPLE_fence", wine_glFinishObjectAPPLE },
  { "glFinishTextureSUNX", "GL_SUNX_constant_data", wine_glFinishTextureSUNX },
  { "glFlushMappedBufferRange", "GL_ARB_map_buffer_range", wine_glFlushMappedBufferRange },
  { "glFlushMappedBufferRangeAPPLE", "GL_APPLE_flush_buffer_range", wine_glFlushMappedBufferRangeAPPLE },
  { "glFlushMappedNamedBufferRangeEXT", "GL_EXT_direct_state_access", wine_glFlushMappedNamedBufferRangeEXT },
  { "glFlushPixelDataRangeNV", "GL_NV_pixel_data_range", wine_glFlushPixelDataRangeNV },
  { "glFlushRasterSGIX", "GL_SGIX_flush_raster", wine_glFlushRasterSGIX },
  { "glFlushVertexArrayRangeAPPLE", "GL_APPLE_vertex_array_range", wine_glFlushVertexArrayRangeAPPLE },
  { "glFlushVertexArrayRangeNV", "GL_NV_vertex_array_range", wine_glFlushVertexArrayRangeNV },
  { "glFogCoordFormatNV", "GL_NV_vertex_buffer_unified_memory", wine_glFogCoordFormatNV },
  { "glFogCoordPointer", "GL_VERSION_1_4_DEPRECATED", wine_glFogCoordPointer },
  { "glFogCoordPointerEXT", "GL_EXT_fog_coord", wine_glFogCoordPointerEXT },
  { "glFogCoordPointerListIBM", "GL_IBM_vertex_array_lists", wine_glFogCoordPointerListIBM },
  { "glFogCoordd", "GL_VERSION_1_4_DEPRECATED", wine_glFogCoordd },
  { "glFogCoorddEXT", "GL_EXT_fog_coord", wine_glFogCoorddEXT },
  { "glFogCoorddv", "GL_VERSION_1_4_DEPRECATED", wine_glFogCoorddv },
  { "glFogCoorddvEXT", "GL_EXT_fog_coord", wine_glFogCoorddvEXT },
  { "glFogCoordf", "GL_VERSION_1_4_DEPRECATED", wine_glFogCoordf },
  { "glFogCoordfEXT", "GL_EXT_fog_coord", wine_glFogCoordfEXT },
  { "glFogCoordfv", "GL_VERSION_1_4_DEPRECATED", wine_glFogCoordfv },
  { "glFogCoordfvEXT", "GL_EXT_fog_coord", wine_glFogCoordfvEXT },
  { "glFogCoordhNV", "GL_NV_half_float", wine_glFogCoordhNV },
  { "glFogCoordhvNV", "GL_NV_half_float", wine_glFogCoordhvNV },
  { "glFogFuncSGIS", "GL_SGIS_fog_function", wine_glFogFuncSGIS },
  { "glFragmentColorMaterialSGIX", "GL_SGIX_fragment_lighting", wine_glFragmentColorMaterialSGIX },
  { "glFragmentLightModelfSGIX", "GL_SGIX_fragment_lighting", wine_glFragmentLightModelfSGIX },
  { "glFragmentLightModelfvSGIX", "GL_SGIX_fragment_lighting", wine_glFragmentLightModelfvSGIX },
  { "glFragmentLightModeliSGIX", "GL_SGIX_fragment_lighting", wine_glFragmentLightModeliSGIX },
  { "glFragmentLightModelivSGIX", "GL_SGIX_fragment_lighting", wine_glFragmentLightModelivSGIX },
  { "glFragmentLightfSGIX", "GL_SGIX_fragment_lighting", wine_glFragmentLightfSGIX },
  { "glFragmentLightfvSGIX", "GL_SGIX_fragment_lighting", wine_glFragmentLightfvSGIX },
  { "glFragmentLightiSGIX", "GL_SGIX_fragment_lighting", wine_glFragmentLightiSGIX },
  { "glFragmentLightivSGIX", "GL_SGIX_fragment_lighting", wine_glFragmentLightivSGIX },
  { "glFragmentMaterialfSGIX", "GL_SGIX_fragment_lighting", wine_glFragmentMaterialfSGIX },
  { "glFragmentMaterialfvSGIX", "GL_SGIX_fragment_lighting", wine_glFragmentMaterialfvSGIX },
  { "glFragmentMaterialiSGIX", "GL_SGIX_fragment_lighting", wine_glFragmentMaterialiSGIX },
  { "glFragmentMaterialivSGIX", "GL_SGIX_fragment_lighting", wine_glFragmentMaterialivSGIX },
  { "glFrameTerminatorGREMEDY", "GL_GREMEDY_frame_terminator", wine_glFrameTerminatorGREMEDY },
  { "glFrameZoomSGIX", "GL_SGIX_framezoom", wine_glFrameZoomSGIX },
  { "glFramebufferDrawBufferEXT", "GL_EXT_direct_state_access", wine_glFramebufferDrawBufferEXT },
  { "glFramebufferDrawBuffersEXT", "GL_EXT_direct_state_access", wine_glFramebufferDrawBuffersEXT },
  { "glFramebufferReadBufferEXT", "GL_EXT_direct_state_access", wine_glFramebufferReadBufferEXT },
  { "glFramebufferRenderbuffer", "GL_ARB_framebuffer_object", wine_glFramebufferRenderbuffer },
  { "glFramebufferRenderbufferEXT", "GL_EXT_framebuffer_object", wine_glFramebufferRenderbufferEXT },
  { "glFramebufferTexture", "GL_VERSION_3_2", wine_glFramebufferTexture },
  { "glFramebufferTexture1D", "GL_ARB_framebuffer_object", wine_glFramebufferTexture1D },
  { "glFramebufferTexture1DEXT", "GL_EXT_framebuffer_object", wine_glFramebufferTexture1DEXT },
  { "glFramebufferTexture2D", "GL_ARB_framebuffer_object", wine_glFramebufferTexture2D },
  { "glFramebufferTexture2DEXT", "GL_EXT_framebuffer_object", wine_glFramebufferTexture2DEXT },
  { "glFramebufferTexture3D", "GL_ARB_framebuffer_object", wine_glFramebufferTexture3D },
  { "glFramebufferTexture3DEXT", "GL_EXT_framebuffer_object", wine_glFramebufferTexture3DEXT },
  { "glFramebufferTextureARB", "GL_ARB_geometry_shader4", wine_glFramebufferTextureARB },
  { "glFramebufferTextureEXT", "GL_NV_geometry_program4", wine_glFramebufferTextureEXT },
  { "glFramebufferTextureFaceARB", "GL_ARB_geometry_shader4", wine_glFramebufferTextureFaceARB },
  { "glFramebufferTextureFaceEXT", "GL_NV_geometry_program4", wine_glFramebufferTextureFaceEXT },
  { "glFramebufferTextureLayer", "GL_ARB_framebuffer_object", wine_glFramebufferTextureLayer },
  { "glFramebufferTextureLayerARB", "GL_ARB_geometry_shader4", wine_glFramebufferTextureLayerARB },
  { "glFramebufferTextureLayerEXT", "GL_NV_geometry_program4", wine_glFramebufferTextureLayerEXT },
  { "glFreeObjectBufferATI", "GL_ATI_vertex_array_object", wine_glFreeObjectBufferATI },
  { "glGenAsyncMarkersSGIX", "GL_SGIX_async", wine_glGenAsyncMarkersSGIX },
  { "glGenBuffers", "GL_VERSION_1_5", wine_glGenBuffers },
  { "glGenBuffersARB", "GL_ARB_vertex_buffer_object", wine_glGenBuffersARB },
  { "glGenFencesAPPLE", "GL_APPLE_fence", wine_glGenFencesAPPLE },
  { "glGenFencesNV", "GL_NV_fence", wine_glGenFencesNV },
  { "glGenFragmentShadersATI", "GL_ATI_fragment_shader", wine_glGenFragmentShadersATI },
  { "glGenFramebuffers", "GL_ARB_framebuffer_object", wine_glGenFramebuffers },
  { "glGenFramebuffersEXT", "GL_EXT_framebuffer_object", wine_glGenFramebuffersEXT },
  { "glGenNamesAMD", "GL_AMD_name_gen_delete", wine_glGenNamesAMD },
  { "glGenOcclusionQueriesNV", "GL_NV_occlusion_query", wine_glGenOcclusionQueriesNV },
  { "glGenPathsNV", "GL_NV_path_rendering", wine_glGenPathsNV },
  { "glGenPerfMonitorsAMD", "GL_AMD_performance_monitor", wine_glGenPerfMonitorsAMD },
  { "glGenProgramPipelines", "GL_ARB_separate_shader_objects", wine_glGenProgramPipelines },
  { "glGenProgramsARB", "GL_ARB_vertex_program", wine_glGenProgramsARB },
  { "glGenProgramsNV", "GL_NV_vertex_program", wine_glGenProgramsNV },
  { "glGenQueries", "GL_VERSION_1_5", wine_glGenQueries },
  { "glGenQueriesARB", "GL_ARB_occlusion_query", wine_glGenQueriesARB },
  { "glGenRenderbuffers", "GL_ARB_framebuffer_object", wine_glGenRenderbuffers },
  { "glGenRenderbuffersEXT", "GL_EXT_framebuffer_object", wine_glGenRenderbuffersEXT },
  { "glGenSamplers", "GL_ARB_sampler_objects", wine_glGenSamplers },
  { "glGenSymbolsEXT", "GL_EXT_vertex_shader", wine_glGenSymbolsEXT },
  { "glGenTexturesEXT", "GL_EXT_texture_object", wine_glGenTexturesEXT },
  { "glGenTransformFeedbacks", "GL_ARB_transform_feedback2", wine_glGenTransformFeedbacks },
  { "glGenTransformFeedbacksNV", "GL_NV_transform_feedback2", wine_glGenTransformFeedbacksNV },
  { "glGenVertexArrays", "GL_ARB_vertex_array_object", wine_glGenVertexArrays },
  { "glGenVertexArraysAPPLE", "GL_APPLE_vertex_array_object", wine_glGenVertexArraysAPPLE },
  { "glGenVertexShadersEXT", "GL_EXT_vertex_shader", wine_glGenVertexShadersEXT },
  { "glGenerateMipmap", "GL_ARB_framebuffer_object", wine_glGenerateMipmap },
  { "glGenerateMipmapEXT", "GL_EXT_framebuffer_object", wine_glGenerateMipmapEXT },
  { "glGenerateMultiTexMipmapEXT", "GL_EXT_direct_state_access", wine_glGenerateMultiTexMipmapEXT },
  { "glGenerateTextureMipmapEXT", "GL_EXT_direct_state_access", wine_glGenerateTextureMipmapEXT },
  { "glGetActiveAtomicCounterBufferiv", "GL_ARB_shader_atomic_counters", wine_glGetActiveAtomicCounterBufferiv },
  { "glGetActiveAttrib", "GL_VERSION_2_0", wine_glGetActiveAttrib },
  { "glGetActiveAttribARB", "GL_ARB_vertex_shader", wine_glGetActiveAttribARB },
  { "glGetActiveSubroutineName", "GL_ARB_shader_subroutine", wine_glGetActiveSubroutineName },
  { "glGetActiveSubroutineUniformName", "GL_ARB_shader_subroutine", wine_glGetActiveSubroutineUniformName },
  { "glGetActiveSubroutineUniformiv", "GL_ARB_shader_subroutine", wine_glGetActiveSubroutineUniformiv },
  { "glGetActiveUniform", "GL_VERSION_2_0", wine_glGetActiveUniform },
  { "glGetActiveUniformARB", "GL_ARB_shader_objects", wine_glGetActiveUniformARB },
  { "glGetActiveUniformBlockName", "GL_ARB_uniform_buffer_object", wine_glGetActiveUniformBlockName },
  { "glGetActiveUniformBlockiv", "GL_ARB_uniform_buffer_object", wine_glGetActiveUniformBlockiv },
  { "glGetActiveUniformName", "GL_ARB_uniform_buffer_object", wine_glGetActiveUniformName },
  { "glGetActiveUniformsiv", "GL_ARB_uniform_buffer_object", wine_glGetActiveUniformsiv },
  { "glGetActiveVaryingNV", "GL_NV_transform_feedback", wine_glGetActiveVaryingNV },
  { "glGetArrayObjectfvATI", "GL_ATI_vertex_array_object", wine_glGetArrayObjectfvATI },
  { "glGetArrayObjectivATI", "GL_ATI_vertex_array_object", wine_glGetArrayObjectivATI },
  { "glGetAttachedObjectsARB", "GL_ARB_shader_objects", wine_glGetAttachedObjectsARB },
  { "glGetAttachedShaders", "GL_VERSION_2_0", wine_glGetAttachedShaders },
  { "glGetAttribLocation", "GL_VERSION_2_0", wine_glGetAttribLocation },
  { "glGetAttribLocationARB", "GL_ARB_vertex_shader", wine_glGetAttribLocationARB },
  { "glGetBooleanIndexedvEXT", "GL_EXT_draw_buffers2", wine_glGetBooleanIndexedvEXT },
  { "glGetBooleani_v", "GL_VERSION_3_0", wine_glGetBooleani_v },
  { "glGetBufferParameteri64v", "GL_VERSION_3_2", wine_glGetBufferParameteri64v },
  { "glGetBufferParameteriv", "GL_VERSION_1_5", wine_glGetBufferParameteriv },
  { "glGetBufferParameterivARB", "GL_ARB_vertex_buffer_object", wine_glGetBufferParameterivARB },
  { "glGetBufferParameterui64vNV", "GL_NV_shader_buffer_load", wine_glGetBufferParameterui64vNV },
  { "glGetBufferPointerv", "GL_VERSION_1_5", wine_glGetBufferPointerv },
  { "glGetBufferPointervARB", "GL_ARB_vertex_buffer_object", wine_glGetBufferPointervARB },
  { "glGetBufferSubData", "GL_VERSION_1_5", wine_glGetBufferSubData },
  { "glGetBufferSubDataARB", "GL_ARB_vertex_buffer_object", wine_glGetBufferSubDataARB },
  { "glGetColorTable", "GL_VERSION_1_2_DEPRECATED", wine_glGetColorTable },
  { "glGetColorTableEXT", "GL_EXT_paletted_texture", wine_glGetColorTableEXT },
  { "glGetColorTableParameterfv", "GL_VERSION_1_2_DEPRECATED", wine_glGetColorTableParameterfv },
  { "glGetColorTableParameterfvEXT", "GL_EXT_paletted_texture", wine_glGetColorTableParameterfvEXT },
  { "glGetColorTableParameterfvSGI", "GL_SGI_color_table", wine_glGetColorTableParameterfvSGI },
  { "glGetColorTableParameteriv", "GL_VERSION_1_2_DEPRECATED", wine_glGetColorTableParameteriv },
  { "glGetColorTableParameterivEXT", "GL_EXT_paletted_texture", wine_glGetColorTableParameterivEXT },
  { "glGetColorTableParameterivSGI", "GL_SGI_color_table", wine_glGetColorTableParameterivSGI },
  { "glGetColorTableSGI", "GL_SGI_color_table", wine_glGetColorTableSGI },
  { "glGetCombinerInputParameterfvNV", "GL_NV_register_combiners", wine_glGetCombinerInputParameterfvNV },
  { "glGetCombinerInputParameterivNV", "GL_NV_register_combiners", wine_glGetCombinerInputParameterivNV },
  { "glGetCombinerOutputParameterfvNV", "GL_NV_register_combiners", wine_glGetCombinerOutputParameterfvNV },
  { "glGetCombinerOutputParameterivNV", "GL_NV_register_combiners", wine_glGetCombinerOutputParameterivNV },
  { "glGetCombinerStageParameterfvNV", "GL_NV_register_combiners2", wine_glGetCombinerStageParameterfvNV },
  { "glGetCompressedMultiTexImageEXT", "GL_EXT_direct_state_access", wine_glGetCompressedMultiTexImageEXT },
  { "glGetCompressedTexImage", "GL_VERSION_1_3", wine_glGetCompressedTexImage },
  { "glGetCompressedTexImageARB", "GL_ARB_texture_compression", wine_glGetCompressedTexImageARB },
  { "glGetCompressedTextureImageEXT", "GL_EXT_direct_state_access", wine_glGetCompressedTextureImageEXT },
  { "glGetConvolutionFilter", "GL_VERSION_1_2_DEPRECATED", wine_glGetConvolutionFilter },
  { "glGetConvolutionFilterEXT", "GL_EXT_convolution", wine_glGetConvolutionFilterEXT },
  { "glGetConvolutionParameterfv", "GL_VERSION_1_2_DEPRECATED", wine_glGetConvolutionParameterfv },
  { "glGetConvolutionParameterfvEXT", "GL_EXT_convolution", wine_glGetConvolutionParameterfvEXT },
  { "glGetConvolutionParameteriv", "GL_VERSION_1_2_DEPRECATED", wine_glGetConvolutionParameteriv },
  { "glGetConvolutionParameterivEXT", "GL_EXT_convolution", wine_glGetConvolutionParameterivEXT },
  { "glGetDebugMessageLogAMD", "GL_AMD_debug_output", wine_glGetDebugMessageLogAMD },
  { "glGetDebugMessageLogARB", "GL_ARB_debug_output", wine_glGetDebugMessageLogARB },
  { "glGetDetailTexFuncSGIS", "GL_SGIS_detail_texture", wine_glGetDetailTexFuncSGIS },
  { "glGetDoubleIndexedvEXT", "GL_EXT_direct_state_access", wine_glGetDoubleIndexedvEXT },
  { "glGetDoublei_v", "GL_ARB_viewport_array", wine_glGetDoublei_v },
  { "glGetFenceivNV", "GL_NV_fence", wine_glGetFenceivNV },
  { "glGetFinalCombinerInputParameterfvNV", "GL_NV_register_combiners", wine_glGetFinalCombinerInputParameterfvNV },
  { "glGetFinalCombinerInputParameterivNV", "GL_NV_register_combiners", wine_glGetFinalCombinerInputParameterivNV },
  { "glGetFloatIndexedvEXT", "GL_EXT_direct_state_access", wine_glGetFloatIndexedvEXT },
  { "glGetFloati_v", "GL_ARB_viewport_array", wine_glGetFloati_v },
  { "glGetFogFuncSGIS", "GL_SGIS_fog_function", wine_glGetFogFuncSGIS },
  { "glGetFragDataIndex", "GL_ARB_blend_func_extended", wine_glGetFragDataIndex },
  { "glGetFragDataLocation", "GL_VERSION_3_0", wine_glGetFragDataLocation },
  { "glGetFragDataLocationEXT", "GL_EXT_gpu_shader4", wine_glGetFragDataLocationEXT },
  { "glGetFragmentLightfvSGIX", "GL_SGIX_fragment_lighting", wine_glGetFragmentLightfvSGIX },
  { "glGetFragmentLightivSGIX", "GL_SGIX_fragment_lighting", wine_glGetFragmentLightivSGIX },
  { "glGetFragmentMaterialfvSGIX", "GL_SGIX_fragment_lighting", wine_glGetFragmentMaterialfvSGIX },
  { "glGetFragmentMaterialivSGIX", "GL_SGIX_fragment_lighting", wine_glGetFragmentMaterialivSGIX },
  { "glGetFramebufferAttachmentParameteriv", "GL_ARB_framebuffer_object", wine_glGetFramebufferAttachmentParameteriv },
  { "glGetFramebufferAttachmentParameterivEXT", "GL_EXT_framebuffer_object", wine_glGetFramebufferAttachmentParameterivEXT },
  { "glGetFramebufferParameterivEXT", "GL_EXT_direct_state_access", wine_glGetFramebufferParameterivEXT },
  { "glGetGraphicsResetStatusARB", "GL_ARB_robustness", wine_glGetGraphicsResetStatusARB },
  { "glGetHandleARB", "GL_ARB_shader_objects", wine_glGetHandleARB },
  { "glGetHistogram", "GL_VERSION_1_2_DEPRECATED", wine_glGetHistogram },
  { "glGetHistogramEXT", "GL_EXT_histogram", wine_glGetHistogramEXT },
  { "glGetHistogramParameterfv", "GL_VERSION_1_2_DEPRECATED", wine_glGetHistogramParameterfv },
  { "glGetHistogramParameterfvEXT", "GL_EXT_histogram", wine_glGetHistogramParameterfvEXT },
  { "glGetHistogramParameteriv", "GL_VERSION_1_2_DEPRECATED", wine_glGetHistogramParameteriv },
  { "glGetHistogramParameterivEXT", "GL_EXT_histogram", wine_glGetHistogramParameterivEXT },
  { "glGetImageHandleNV", "GL_NV_bindless_texture", wine_glGetImageHandleNV },
  { "glGetImageTransformParameterfvHP", "GL_HP_image_transform", wine_glGetImageTransformParameterfvHP },
  { "glGetImageTransformParameterivHP", "GL_HP_image_transform", wine_glGetImageTransformParameterivHP },
  { "glGetInfoLogARB", "GL_ARB_shader_objects", wine_glGetInfoLogARB },
  { "glGetInstrumentsSGIX", "GL_SGIX_instruments", wine_glGetInstrumentsSGIX },
  { "glGetInteger64i_v", "GL_VERSION_3_2", wine_glGetInteger64i_v },
  { "glGetInteger64v", "GL_ARB_sync", wine_glGetInteger64v },
  { "glGetIntegerIndexedvEXT", "GL_EXT_draw_buffers2", wine_glGetIntegerIndexedvEXT },
  { "glGetIntegeri_v", "GL_VERSION_3_0", wine_glGetIntegeri_v },
  { "glGetIntegerui64i_vNV", "GL_NV_vertex_buffer_unified_memory", wine_glGetIntegerui64i_vNV },
  { "glGetIntegerui64vNV", "GL_NV_shader_buffer_load", wine_glGetIntegerui64vNV },
  { "glGetInternalformativ", "GL_ARB_internalformat_query", wine_glGetInternalformativ },
  { "glGetInvariantBooleanvEXT", "GL_EXT_vertex_shader", wine_glGetInvariantBooleanvEXT },
  { "glGetInvariantFloatvEXT", "GL_EXT_vertex_shader", wine_glGetInvariantFloatvEXT },
  { "glGetInvariantIntegervEXT", "GL_EXT_vertex_shader", wine_glGetInvariantIntegervEXT },
  { "glGetListParameterfvSGIX", "GL_SGIX_list_priority", wine_glGetListParameterfvSGIX },
  { "glGetListParameterivSGIX", "GL_SGIX_list_priority", wine_glGetListParameterivSGIX },
  { "glGetLocalConstantBooleanvEXT", "GL_EXT_vertex_shader", wine_glGetLocalConstantBooleanvEXT },
  { "glGetLocalConstantFloatvEXT", "GL_EXT_vertex_shader", wine_glGetLocalConstantFloatvEXT },
  { "glGetLocalConstantIntegervEXT", "GL_EXT_vertex_shader", wine_glGetLocalConstantIntegervEXT },
  { "glGetMapAttribParameterfvNV", "GL_NV_evaluators", wine_glGetMapAttribParameterfvNV },
  { "glGetMapAttribParameterivNV", "GL_NV_evaluators", wine_glGetMapAttribParameterivNV },
  { "glGetMapControlPointsNV", "GL_NV_evaluators", wine_glGetMapControlPointsNV },
  { "glGetMapParameterfvNV", "GL_NV_evaluators", wine_glGetMapParameterfvNV },
  { "glGetMapParameterivNV", "GL_NV_evaluators", wine_glGetMapParameterivNV },
  { "glGetMinmax", "GL_VERSION_1_2_DEPRECATED", wine_glGetMinmax },
  { "glGetMinmaxEXT", "GL_EXT_histogram", wine_glGetMinmaxEXT },
  { "glGetMinmaxParameterfv", "GL_VERSION_1_2_DEPRECATED", wine_glGetMinmaxParameterfv },
  { "glGetMinmaxParameterfvEXT", "GL_EXT_histogram", wine_glGetMinmaxParameterfvEXT },
  { "glGetMinmaxParameteriv", "GL_VERSION_1_2_DEPRECATED", wine_glGetMinmaxParameteriv },
  { "glGetMinmaxParameterivEXT", "GL_EXT_histogram", wine_glGetMinmaxParameterivEXT },
  { "glGetMultiTexEnvfvEXT", "GL_EXT_direct_state_access", wine_glGetMultiTexEnvfvEXT },
  { "glGetMultiTexEnvivEXT", "GL_EXT_direct_state_access", wine_glGetMultiTexEnvivEXT },
  { "glGetMultiTexGendvEXT", "GL_EXT_direct_state_access", wine_glGetMultiTexGendvEXT },
  { "glGetMultiTexGenfvEXT", "GL_EXT_direct_state_access", wine_glGetMultiTexGenfvEXT },
  { "glGetMultiTexGenivEXT", "GL_EXT_direct_state_access", wine_glGetMultiTexGenivEXT },
  { "glGetMultiTexImageEXT", "GL_EXT_direct_state_access", wine_glGetMultiTexImageEXT },
  { "glGetMultiTexLevelParameterfvEXT", "GL_EXT_direct_state_access", wine_glGetMultiTexLevelParameterfvEXT },
  { "glGetMultiTexLevelParameterivEXT", "GL_EXT_direct_state_access", wine_glGetMultiTexLevelParameterivEXT },
  { "glGetMultiTexParameterIivEXT", "GL_EXT_direct_state_access", wine_glGetMultiTexParameterIivEXT },
  { "glGetMultiTexParameterIuivEXT", "GL_EXT_direct_state_access", wine_glGetMultiTexParameterIuivEXT },
  { "glGetMultiTexParameterfvEXT", "GL_EXT_direct_state_access", wine_glGetMultiTexParameterfvEXT },
  { "glGetMultiTexParameterivEXT", "GL_EXT_direct_state_access", wine_glGetMultiTexParameterivEXT },
  { "glGetMultisamplefv", "GL_ARB_texture_multisample", wine_glGetMultisamplefv },
  { "glGetMultisamplefvNV", "GL_NV_explicit_multisample", wine_glGetMultisamplefvNV },
  { "glGetNamedBufferParameterivEXT", "GL_EXT_direct_state_access", wine_glGetNamedBufferParameterivEXT },
  { "glGetNamedBufferParameterui64vNV", "GL_NV_shader_buffer_load", wine_glGetNamedBufferParameterui64vNV },
  { "glGetNamedBufferPointervEXT", "GL_EXT_direct_state_access", wine_glGetNamedBufferPointervEXT },
  { "glGetNamedBufferSubDataEXT", "GL_EXT_direct_state_access", wine_glGetNamedBufferSubDataEXT },
  { "glGetNamedFramebufferAttachmentParameterivEXT", "GL_EXT_direct_state_access", wine_glGetNamedFramebufferAttachmentParameterivEXT },
  { "glGetNamedProgramLocalParameterIivEXT", "GL_EXT_direct_state_access", wine_glGetNamedProgramLocalParameterIivEXT },
  { "glGetNamedProgramLocalParameterIuivEXT", "GL_EXT_direct_state_access", wine_glGetNamedProgramLocalParameterIuivEXT },
  { "glGetNamedProgramLocalParameterdvEXT", "GL_EXT_direct_state_access", wine_glGetNamedProgramLocalParameterdvEXT },
  { "glGetNamedProgramLocalParameterfvEXT", "GL_EXT_direct_state_access", wine_glGetNamedProgramLocalParameterfvEXT },
  { "glGetNamedProgramStringEXT", "GL_EXT_direct_state_access", wine_glGetNamedProgramStringEXT },
  { "glGetNamedProgramivEXT", "GL_EXT_direct_state_access", wine_glGetNamedProgramivEXT },
  { "glGetNamedRenderbufferParameterivEXT", "GL_EXT_direct_state_access", wine_glGetNamedRenderbufferParameterivEXT },
  { "glGetNamedStringARB", "GL_ARB_shading_language_include", wine_glGetNamedStringARB },
  { "glGetNamedStringivARB", "GL_ARB_shading_language_include", wine_glGetNamedStringivARB },
  { "glGetObjectBufferfvATI", "GL_ATI_vertex_array_object", wine_glGetObjectBufferfvATI },
  { "glGetObjectBufferivATI", "GL_ATI_vertex_array_object", wine_glGetObjectBufferivATI },
  { "glGetObjectParameterfvARB", "GL_ARB_shader_objects", wine_glGetObjectParameterfvARB },
  { "glGetObjectParameterivAPPLE", "GL_APPLE_object_purgeable", wine_glGetObjectParameterivAPPLE },
  { "glGetObjectParameterivARB", "GL_ARB_shader_objects", wine_glGetObjectParameterivARB },
  { "glGetOcclusionQueryivNV", "GL_NV_occlusion_query", wine_glGetOcclusionQueryivNV },
  { "glGetOcclusionQueryuivNV", "GL_NV_occlusion_query", wine_glGetOcclusionQueryuivNV },
  { "glGetPathColorGenfvNV", "GL_NV_path_rendering", wine_glGetPathColorGenfvNV },
  { "glGetPathColorGenivNV", "GL_NV_path_rendering", wine_glGetPathColorGenivNV },
  { "glGetPathCommandsNV", "GL_NV_path_rendering", wine_glGetPathCommandsNV },
  { "glGetPathCoordsNV", "GL_NV_path_rendering", wine_glGetPathCoordsNV },
  { "glGetPathDashArrayNV", "GL_NV_path_rendering", wine_glGetPathDashArrayNV },
  { "glGetPathLengthNV", "GL_NV_path_rendering", wine_glGetPathLengthNV },
  { "glGetPathMetricRangeNV", "GL_NV_path_rendering", wine_glGetPathMetricRangeNV },
  { "glGetPathMetricsNV", "GL_NV_path_rendering", wine_glGetPathMetricsNV },
  { "glGetPathParameterfvNV", "GL_NV_path_rendering", wine_glGetPathParameterfvNV },
  { "glGetPathParameterivNV", "GL_NV_path_rendering", wine_glGetPathParameterivNV },
  { "glGetPathSpacingNV", "GL_NV_path_rendering", wine_glGetPathSpacingNV },
  { "glGetPathTexGenfvNV", "GL_NV_path_rendering", wine_glGetPathTexGenfvNV },
  { "glGetPathTexGenivNV", "GL_NV_path_rendering", wine_glGetPathTexGenivNV },
  { "glGetPerfMonitorCounterDataAMD", "GL_AMD_performance_monitor", wine_glGetPerfMonitorCounterDataAMD },
  { "glGetPerfMonitorCounterInfoAMD", "GL_AMD_performance_monitor", wine_glGetPerfMonitorCounterInfoAMD },
  { "glGetPerfMonitorCounterStringAMD", "GL_AMD_performance_monitor", wine_glGetPerfMonitorCounterStringAMD },
  { "glGetPerfMonitorCountersAMD", "GL_AMD_performance_monitor", wine_glGetPerfMonitorCountersAMD },
  { "glGetPerfMonitorGroupStringAMD", "GL_AMD_performance_monitor", wine_glGetPerfMonitorGroupStringAMD },
  { "glGetPerfMonitorGroupsAMD", "GL_AMD_performance_monitor", wine_glGetPerfMonitorGroupsAMD },
  { "glGetPixelTexGenParameterfvSGIS", "GL_SGIS_pixel_texture", wine_glGetPixelTexGenParameterfvSGIS },
  { "glGetPixelTexGenParameterivSGIS", "GL_SGIS_pixel_texture", wine_glGetPixelTexGenParameterivSGIS },
  { "glGetPointerIndexedvEXT", "GL_EXT_direct_state_access", wine_glGetPointerIndexedvEXT },
  { "glGetPointervEXT", "GL_EXT_vertex_array", wine_glGetPointervEXT },
  { "glGetProgramBinary", "GL_ARB_get_program_binary", wine_glGetProgramBinary },
  { "glGetProgramEnvParameterIivNV", "GL_NV_gpu_program4", wine_glGetProgramEnvParameterIivNV },
  { "glGetProgramEnvParameterIuivNV", "GL_NV_gpu_program4", wine_glGetProgramEnvParameterIuivNV },
  { "glGetProgramEnvParameterdvARB", "GL_ARB_vertex_program", wine_glGetProgramEnvParameterdvARB },
  { "glGetProgramEnvParameterfvARB", "GL_ARB_vertex_program", wine_glGetProgramEnvParameterfvARB },
  { "glGetProgramInfoLog", "GL_VERSION_2_0", wine_glGetProgramInfoLog },
  { "glGetProgramLocalParameterIivNV", "GL_NV_gpu_program4", wine_glGetProgramLocalParameterIivNV },
  { "glGetProgramLocalParameterIuivNV", "GL_NV_gpu_program4", wine_glGetProgramLocalParameterIuivNV },
  { "glGetProgramLocalParameterdvARB", "GL_ARB_vertex_program", wine_glGetProgramLocalParameterdvARB },
  { "glGetProgramLocalParameterfvARB", "GL_ARB_vertex_program", wine_glGetProgramLocalParameterfvARB },
  { "glGetProgramNamedParameterdvNV", "GL_NV_fragment_program", wine_glGetProgramNamedParameterdvNV },
  { "glGetProgramNamedParameterfvNV", "GL_NV_fragment_program", wine_glGetProgramNamedParameterfvNV },
  { "glGetProgramParameterdvNV", "GL_NV_vertex_program", wine_glGetProgramParameterdvNV },
  { "glGetProgramParameterfvNV", "GL_NV_vertex_program", wine_glGetProgramParameterfvNV },
  { "glGetProgramPipelineInfoLog", "GL_ARB_separate_shader_objects", wine_glGetProgramPipelineInfoLog },
  { "glGetProgramPipelineiv", "GL_ARB_separate_shader_objects", wine_glGetProgramPipelineiv },
  { "glGetProgramStageiv", "GL_ARB_shader_subroutine", wine_glGetProgramStageiv },
  { "glGetProgramStringARB", "GL_ARB_vertex_program", wine_glGetProgramStringARB },
  { "glGetProgramStringNV", "GL_NV_vertex_program", wine_glGetProgramStringNV },
  { "glGetProgramSubroutineParameteruivNV", "GL_NV_gpu_program5", wine_glGetProgramSubroutineParameteruivNV },
  { "glGetProgramiv", "GL_VERSION_2_0", wine_glGetProgramiv },
  { "glGetProgramivARB", "GL_ARB_vertex_program", wine_glGetProgramivARB },
  { "glGetProgramivNV", "GL_NV_vertex_program", wine_glGetProgramivNV },
  { "glGetQueryIndexediv", "GL_ARB_transform_feedback3", wine_glGetQueryIndexediv },
  { "glGetQueryObjecti64v", "GL_ARB_timer_query", wine_glGetQueryObjecti64v },
  { "glGetQueryObjecti64vEXT", "GL_EXT_timer_query", wine_glGetQueryObjecti64vEXT },
  { "glGetQueryObjectiv", "GL_VERSION_1_5", wine_glGetQueryObjectiv },
  { "glGetQueryObjectivARB", "GL_ARB_occlusion_query", wine_glGetQueryObjectivARB },
  { "glGetQueryObjectui64v", "GL_ARB_timer_query", wine_glGetQueryObjectui64v },
  { "glGetQueryObjectui64vEXT", "GL_EXT_timer_query", wine_glGetQueryObjectui64vEXT },
  { "glGetQueryObjectuiv", "GL_VERSION_1_5", wine_glGetQueryObjectuiv },
  { "glGetQueryObjectuivARB", "GL_ARB_occlusion_query", wine_glGetQueryObjectuivARB },
  { "glGetQueryiv", "GL_VERSION_1_5", wine_glGetQueryiv },
  { "glGetQueryivARB", "GL_ARB_occlusion_query", wine_glGetQueryivARB },
  { "glGetRenderbufferParameteriv", "GL_ARB_framebuffer_object", wine_glGetRenderbufferParameteriv },
  { "glGetRenderbufferParameterivEXT", "GL_EXT_framebuffer_object", wine_glGetRenderbufferParameterivEXT },
  { "glGetSamplerParameterIiv", "GL_ARB_sampler_objects", wine_glGetSamplerParameterIiv },
  { "glGetSamplerParameterIuiv", "GL_ARB_sampler_objects", wine_glGetSamplerParameterIuiv },
  { "glGetSamplerParameterfv", "GL_ARB_sampler_objects", wine_glGetSamplerParameterfv },
  { "glGetSamplerParameteriv", "GL_ARB_sampler_objects", wine_glGetSamplerParameteriv },
  { "glGetSeparableFilter", "GL_VERSION_1_2_DEPRECATED", wine_glGetSeparableFilter },
  { "glGetSeparableFilterEXT", "GL_EXT_convolution", wine_glGetSeparableFilterEXT },
  { "glGetShaderInfoLog", "GL_VERSION_2_0", wine_glGetShaderInfoLog },
  { "glGetShaderPrecisionFormat", "GL_ARB_ES2_compatibility", wine_glGetShaderPrecisionFormat },
  { "glGetShaderSource", "GL_VERSION_2_0", wine_glGetShaderSource },
  { "glGetShaderSourceARB", "GL_ARB_shader_objects", wine_glGetShaderSourceARB },
  { "glGetShaderiv", "GL_VERSION_2_0", wine_glGetShaderiv },
  { "glGetSharpenTexFuncSGIS", "GL_SGIS_sharpen_texture", wine_glGetSharpenTexFuncSGIS },
  { "glGetStringi", "GL_VERSION_3_0", wine_glGetStringi },
  { "glGetSubroutineIndex", "GL_ARB_shader_subroutine", wine_glGetSubroutineIndex },
  { "glGetSubroutineUniformLocation", "GL_ARB_shader_subroutine", wine_glGetSubroutineUniformLocation },
  { "glGetSynciv", "GL_ARB_sync", wine_glGetSynciv },
  { "glGetTexBumpParameterfvATI", "GL_ATI_envmap_bumpmap", wine_glGetTexBumpParameterfvATI },
  { "glGetTexBumpParameterivATI", "GL_ATI_envmap_bumpmap", wine_glGetTexBumpParameterivATI },
  { "glGetTexFilterFuncSGIS", "GL_SGIS_texture_filter4", wine_glGetTexFilterFuncSGIS },
  { "glGetTexParameterIiv", "GL_VERSION_3_0", wine_glGetTexParameterIiv },
  { "glGetTexParameterIivEXT", "GL_EXT_texture_integer", wine_glGetTexParameterIivEXT },
  { "glGetTexParameterIuiv", "GL_VERSION_3_0", wine_glGetTexParameterIuiv },
  { "glGetTexParameterIuivEXT", "GL_EXT_texture_integer", wine_glGetTexParameterIuivEXT },
  { "glGetTexParameterPointervAPPLE", "GL_APPLE_texture_range", wine_glGetTexParameterPointervAPPLE },
  { "glGetTextureHandleNV", "GL_NV_bindless_texture", wine_glGetTextureHandleNV },
  { "glGetTextureImageEXT", "GL_EXT_direct_state_access", wine_glGetTextureImageEXT },
  { "glGetTextureLevelParameterfvEXT", "GL_EXT_direct_state_access", wine_glGetTextureLevelParameterfvEXT },
  { "glGetTextureLevelParameterivEXT", "GL_EXT_direct_state_access", wine_glGetTextureLevelParameterivEXT },
  { "glGetTextureParameterIivEXT", "GL_EXT_direct_state_access", wine_glGetTextureParameterIivEXT },
  { "glGetTextureParameterIuivEXT", "GL_EXT_direct_state_access", wine_glGetTextureParameterIuivEXT },
  { "glGetTextureParameterfvEXT", "GL_EXT_direct_state_access", wine_glGetTextureParameterfvEXT },
  { "glGetTextureParameterivEXT", "GL_EXT_direct_state_access", wine_glGetTextureParameterivEXT },
  { "glGetTextureSamplerHandleNV", "GL_NV_bindless_texture", wine_glGetTextureSamplerHandleNV },
  { "glGetTrackMatrixivNV", "GL_NV_vertex_program", wine_glGetTrackMatrixivNV },
  { "glGetTransformFeedbackVarying", "GL_VERSION_3_0", wine_glGetTransformFeedbackVarying },
  { "glGetTransformFeedbackVaryingEXT", "GL_EXT_transform_feedback", wine_glGetTransformFeedbackVaryingEXT },
  { "glGetTransformFeedbackVaryingNV", "GL_NV_transform_feedback", wine_glGetTransformFeedbackVaryingNV },
  { "glGetUniformBlockIndex", "GL_ARB_uniform_buffer_object", wine_glGetUniformBlockIndex },
  { "glGetUniformBufferSizeEXT", "GL_EXT_bindable_uniform", wine_glGetUniformBufferSizeEXT },
  { "glGetUniformIndices", "GL_ARB_uniform_buffer_object", wine_glGetUniformIndices },
  { "glGetUniformLocation", "GL_VERSION_2_0", wine_glGetUniformLocation },
  { "glGetUniformLocationARB", "GL_ARB_shader_objects", wine_glGetUniformLocationARB },
  { "glGetUniformOffsetEXT", "GL_EXT_bindable_uniform", wine_glGetUniformOffsetEXT },
  { "glGetUniformSubroutineuiv", "GL_ARB_shader_subroutine", wine_glGetUniformSubroutineuiv },
  { "glGetUniformdv", "GL_ARB_gpu_shader_fp64", wine_glGetUniformdv },
  { "glGetUniformfv", "GL_VERSION_2_0", wine_glGetUniformfv },
  { "glGetUniformfvARB", "GL_ARB_shader_objects", wine_glGetUniformfvARB },
  { "glGetUniformi64vNV", "GL_NV_gpu_shader5", wine_glGetUniformi64vNV },
  { "glGetUniformiv", "GL_VERSION_2_0", wine_glGetUniformiv },
  { "glGetUniformivARB", "GL_ARB_shader_objects", wine_glGetUniformivARB },
  { "glGetUniformui64vNV", "GL_NV_shader_buffer_load", wine_glGetUniformui64vNV },
  { "glGetUniformuiv", "GL_VERSION_3_0", wine_glGetUniformuiv },
  { "glGetUniformuivEXT", "GL_EXT_gpu_shader4", wine_glGetUniformuivEXT },
  { "glGetVariantArrayObjectfvATI", "GL_ATI_vertex_array_object", wine_glGetVariantArrayObjectfvATI },
  { "glGetVariantArrayObjectivATI", "GL_ATI_vertex_array_object", wine_glGetVariantArrayObjectivATI },
  { "glGetVariantBooleanvEXT", "GL_EXT_vertex_shader", wine_glGetVariantBooleanvEXT },
  { "glGetVariantFloatvEXT", "GL_EXT_vertex_shader", wine_glGetVariantFloatvEXT },
  { "glGetVariantIntegervEXT", "GL_EXT_vertex_shader", wine_glGetVariantIntegervEXT },
  { "glGetVariantPointervEXT", "GL_EXT_vertex_shader", wine_glGetVariantPointervEXT },
  { "glGetVaryingLocationNV", "GL_NV_transform_feedback", wine_glGetVaryingLocationNV },
  { "glGetVertexAttribArrayObjectfvATI", "GL_ATI_vertex_attrib_array_object", wine_glGetVertexAttribArrayObjectfvATI },
  { "glGetVertexAttribArrayObjectivATI", "GL_ATI_vertex_attrib_array_object", wine_glGetVertexAttribArrayObjectivATI },
  { "glGetVertexAttribIiv", "GL_VERSION_3_0", wine_glGetVertexAttribIiv },
  { "glGetVertexAttribIivEXT", "GL_NV_vertex_program4", wine_glGetVertexAttribIivEXT },
  { "glGetVertexAttribIuiv", "GL_VERSION_3_0", wine_glGetVertexAttribIuiv },
  { "glGetVertexAttribIuivEXT", "GL_NV_vertex_program4", wine_glGetVertexAttribIuivEXT },
  { "glGetVertexAttribLdv", "GL_ARB_vertex_attrib_64bit", wine_glGetVertexAttribLdv },
  { "glGetVertexAttribLdvEXT", "GL_EXT_vertex_attrib_64bit", wine_glGetVertexAttribLdvEXT },
  { "glGetVertexAttribLi64vNV", "GL_NV_vertex_attrib_integer_64bit", wine_glGetVertexAttribLi64vNV },
  { "glGetVertexAttribLui64vNV", "GL_NV_vertex_attrib_integer_64bit", wine_glGetVertexAttribLui64vNV },
  { "glGetVertexAttribPointerv", "GL_VERSION_2_0", wine_glGetVertexAttribPointerv },
  { "glGetVertexAttribPointervARB", "GL_ARB_vertex_program", wine_glGetVertexAttribPointervARB },
  { "glGetVertexAttribPointervNV", "GL_NV_vertex_program", wine_glGetVertexAttribPointervNV },
  { "glGetVertexAttribdv", "GL_VERSION_2_0", wine_glGetVertexAttribdv },
  { "glGetVertexAttribdvARB", "GL_ARB_vertex_program", wine_glGetVertexAttribdvARB },
  { "glGetVertexAttribdvNV", "GL_NV_vertex_program", wine_glGetVertexAttribdvNV },
  { "glGetVertexAttribfv", "GL_VERSION_2_0", wine_glGetVertexAttribfv },
  { "glGetVertexAttribfvARB", "GL_ARB_vertex_program", wine_glGetVertexAttribfvARB },
  { "glGetVertexAttribfvNV", "GL_NV_vertex_program", wine_glGetVertexAttribfvNV },
  { "glGetVertexAttribiv", "GL_VERSION_2_0", wine_glGetVertexAttribiv },
  { "glGetVertexAttribivARB", "GL_ARB_vertex_program", wine_glGetVertexAttribivARB },
  { "glGetVertexAttribivNV", "GL_NV_vertex_program", wine_glGetVertexAttribivNV },
  { "glGetVideoCaptureStreamdvNV", "GL_NV_video_capture", wine_glGetVideoCaptureStreamdvNV },
  { "glGetVideoCaptureStreamfvNV", "GL_NV_video_capture", wine_glGetVideoCaptureStreamfvNV },
  { "glGetVideoCaptureStreamivNV", "GL_NV_video_capture", wine_glGetVideoCaptureStreamivNV },
  { "glGetVideoCaptureivNV", "GL_NV_video_capture", wine_glGetVideoCaptureivNV },
  { "glGetVideoi64vNV", "GL_NV_present_video", wine_glGetVideoi64vNV },
  { "glGetVideoivNV", "GL_NV_present_video", wine_glGetVideoivNV },
  { "glGetVideoui64vNV", "GL_NV_present_video", wine_glGetVideoui64vNV },
  { "glGetVideouivNV", "GL_NV_present_video", wine_glGetVideouivNV },
  { "glGetnColorTableARB", "GL_ARB_robustness", wine_glGetnColorTableARB },
  { "glGetnCompressedTexImageARB", "GL_ARB_robustness", wine_glGetnCompressedTexImageARB },
  { "glGetnConvolutionFilterARB", "GL_ARB_robustness", wine_glGetnConvolutionFilterARB },
  { "glGetnHistogramARB", "GL_ARB_robustness", wine_glGetnHistogramARB },
  { "glGetnMapdvARB", "GL_ARB_robustness", wine_glGetnMapdvARB },
  { "glGetnMapfvARB", "GL_ARB_robustness", wine_glGetnMapfvARB },
  { "glGetnMapivARB", "GL_ARB_robustness", wine_glGetnMapivARB },
  { "glGetnMinmaxARB", "GL_ARB_robustness", wine_glGetnMinmaxARB },
  { "glGetnPixelMapfvARB", "GL_ARB_robustness", wine_glGetnPixelMapfvARB },
  { "glGetnPixelMapuivARB", "GL_ARB_robustness", wine_glGetnPixelMapuivARB },
  { "glGetnPixelMapusvARB", "GL_ARB_robustness", wine_glGetnPixelMapusvARB },
  { "glGetnPolygonStippleARB", "GL_ARB_robustness", wine_glGetnPolygonStippleARB },
  { "glGetnSeparableFilterARB", "GL_ARB_robustness", wine_glGetnSeparableFilterARB },
  { "glGetnTexImageARB", "GL_ARB_robustness", wine_glGetnTexImageARB },
  { "glGetnUniformdvARB", "GL_ARB_robustness", wine_glGetnUniformdvARB },
  { "glGetnUniformfvARB", "GL_ARB_robustness", wine_glGetnUniformfvARB },
  { "glGetnUniformivARB", "GL_ARB_robustness", wine_glGetnUniformivARB },
  { "glGetnUniformuivARB", "GL_ARB_robustness", wine_glGetnUniformuivARB },
  { "glGlobalAlphaFactorbSUN", "GL_SUN_global_alpha", wine_glGlobalAlphaFactorbSUN },
  { "glGlobalAlphaFactordSUN", "GL_SUN_global_alpha", wine_glGlobalAlphaFactordSUN },
  { "glGlobalAlphaFactorfSUN", "GL_SUN_global_alpha", wine_glGlobalAlphaFactorfSUN },
  { "glGlobalAlphaFactoriSUN", "GL_SUN_global_alpha", wine_glGlobalAlphaFactoriSUN },
  { "glGlobalAlphaFactorsSUN", "GL_SUN_global_alpha", wine_glGlobalAlphaFactorsSUN },
  { "glGlobalAlphaFactorubSUN", "GL_SUN_global_alpha", wine_glGlobalAlphaFactorubSUN },
  { "glGlobalAlphaFactoruiSUN", "GL_SUN_global_alpha", wine_glGlobalAlphaFactoruiSUN },
  { "glGlobalAlphaFactorusSUN", "GL_SUN_global_alpha", wine_glGlobalAlphaFactorusSUN },
  { "glHintPGI", "GL_PGI_misc_hints", wine_glHintPGI },
  { "glHistogram", "GL_VERSION_1_2_DEPRECATED", wine_glHistogram },
  { "glHistogramEXT", "GL_EXT_histogram", wine_glHistogramEXT },
  { "glIglooInterfaceSGIX", "GL_SGIX_igloo_interface", wine_glIglooInterfaceSGIX },
  { "glImageTransformParameterfHP", "GL_HP_image_transform", wine_glImageTransformParameterfHP },
  { "glImageTransformParameterfvHP", "GL_HP_image_transform", wine_glImageTransformParameterfvHP },
  { "glImageTransformParameteriHP", "GL_HP_image_transform", wine_glImageTransformParameteriHP },
  { "glImageTransformParameterivHP", "GL_HP_image_transform", wine_glImageTransformParameterivHP },
  { "glImportSyncEXT", "GL_EXT_x11_sync_object", wine_glImportSyncEXT },
  { "glIndexFormatNV", "GL_NV_vertex_buffer_unified_memory", wine_glIndexFormatNV },
  { "glIndexFuncEXT", "GL_EXT_index_func", wine_glIndexFuncEXT },
  { "glIndexMaterialEXT", "GL_EXT_index_material", wine_glIndexMaterialEXT },
  { "glIndexPointerEXT", "GL_EXT_vertex_array", wine_glIndexPointerEXT },
  { "glIndexPointerListIBM", "GL_IBM_vertex_array_lists", wine_glIndexPointerListIBM },
  { "glInsertComponentEXT", "GL_EXT_vertex_shader", wine_glInsertComponentEXT },
  { "glInstrumentsBufferSGIX", "GL_SGIX_instruments", wine_glInstrumentsBufferSGIX },
  { "glInterpolatePathsNV", "GL_NV_path_rendering", wine_glInterpolatePathsNV },
  { "glIsAsyncMarkerSGIX", "GL_SGIX_async", wine_glIsAsyncMarkerSGIX },
  { "glIsBuffer", "GL_VERSION_1_5", wine_glIsBuffer },
  { "glIsBufferARB", "GL_ARB_vertex_buffer_object", wine_glIsBufferARB },
  { "glIsBufferResidentNV", "GL_NV_shader_buffer_load", wine_glIsBufferResidentNV },
  { "glIsEnabledIndexedEXT", "GL_EXT_draw_buffers2", wine_glIsEnabledIndexedEXT },
  { "glIsEnabledi", "GL_VERSION_3_0", wine_glIsEnabledi },
  { "glIsFenceAPPLE", "GL_APPLE_fence", wine_glIsFenceAPPLE },
  { "glIsFenceNV", "GL_NV_fence", wine_glIsFenceNV },
  { "glIsFramebuffer", "GL_ARB_framebuffer_object", wine_glIsFramebuffer },
  { "glIsFramebufferEXT", "GL_EXT_framebuffer_object", wine_glIsFramebufferEXT },
  { "glIsImageHandleResidentNV", "GL_NV_bindless_texture", wine_glIsImageHandleResidentNV },
  { "glIsNameAMD", "GL_AMD_name_gen_delete", wine_glIsNameAMD },
  { "glIsNamedBufferResidentNV", "GL_NV_shader_buffer_load", wine_glIsNamedBufferResidentNV },
  { "glIsNamedStringARB", "GL_ARB_shading_language_include", wine_glIsNamedStringARB },
  { "glIsObjectBufferATI", "GL_ATI_vertex_array_object", wine_glIsObjectBufferATI },
  { "glIsOcclusionQueryNV", "GL_NV_occlusion_query", wine_glIsOcclusionQueryNV },
  { "glIsPathNV", "GL_NV_path_rendering", wine_glIsPathNV },
  { "glIsPointInFillPathNV", "GL_NV_path_rendering", wine_glIsPointInFillPathNV },
  { "glIsPointInStrokePathNV", "GL_NV_path_rendering", wine_glIsPointInStrokePathNV },
  { "glIsProgram", "GL_VERSION_2_0", wine_glIsProgram },
  { "glIsProgramARB", "GL_ARB_vertex_program", wine_glIsProgramARB },
  { "glIsProgramNV", "GL_NV_vertex_program", wine_glIsProgramNV },
  { "glIsProgramPipeline", "GL_ARB_separate_shader_objects", wine_glIsProgramPipeline },
  { "glIsQuery", "GL_VERSION_1_5", wine_glIsQuery },
  { "glIsQueryARB", "GL_ARB_occlusion_query", wine_glIsQueryARB },
  { "glIsRenderbuffer", "GL_ARB_framebuffer_object", wine_glIsRenderbuffer },
  { "glIsRenderbufferEXT", "GL_EXT_framebuffer_object", wine_glIsRenderbufferEXT },
  { "glIsSampler", "GL_ARB_sampler_objects", wine_glIsSampler },
  { "glIsShader", "GL_VERSION_2_0", wine_glIsShader },
  { "glIsSync", "GL_ARB_sync", wine_glIsSync },
  { "glIsTextureEXT", "GL_EXT_texture_object", wine_glIsTextureEXT },
  { "glIsTextureHandleResidentNV", "GL_NV_bindless_texture", wine_glIsTextureHandleResidentNV },
  { "glIsTransformFeedback", "GL_ARB_transform_feedback2", wine_glIsTransformFeedback },
  { "glIsTransformFeedbackNV", "GL_NV_transform_feedback2", wine_glIsTransformFeedbackNV },
  { "glIsVariantEnabledEXT", "GL_EXT_vertex_shader", wine_glIsVariantEnabledEXT },
  { "glIsVertexArray", "GL_ARB_vertex_array_object", wine_glIsVertexArray },
  { "glIsVertexArrayAPPLE", "GL_APPLE_vertex_array_object", wine_glIsVertexArrayAPPLE },
  { "glIsVertexAttribEnabledAPPLE", "GL_APPLE_vertex_program_evaluators", wine_glIsVertexAttribEnabledAPPLE },
  { "glLightEnviSGIX", "GL_SGIX_fragment_lighting", wine_glLightEnviSGIX },
  { "glLinkProgram", "GL_VERSION_2_0", wine_glLinkProgram },
  { "glLinkProgramARB", "GL_ARB_shader_objects", wine_glLinkProgramARB },
  { "glListParameterfSGIX", "GL_SGIX_list_priority", wine_glListParameterfSGIX },
  { "glListParameterfvSGIX", "GL_SGIX_list_priority", wine_glListParameterfvSGIX },
  { "glListParameteriSGIX", "GL_SGIX_list_priority", wine_glListParameteriSGIX },
  { "glListParameterivSGIX", "GL_SGIX_list_priority", wine_glListParameterivSGIX },
  { "glLoadIdentityDeformationMapSGIX", "GL_SGIX_polynomial_ffd", wine_glLoadIdentityDeformationMapSGIX },
  { "glLoadProgramNV", "GL_NV_vertex_program", wine_glLoadProgramNV },
  { "glLoadTransposeMatrixd", "GL_VERSION_1_3_DEPRECATED", wine_glLoadTransposeMatrixd },
  { "glLoadTransposeMatrixdARB", "GL_ARB_transpose_matrix", wine_glLoadTransposeMatrixdARB },
  { "glLoadTransposeMatrixf", "GL_VERSION_1_3_DEPRECATED", wine_glLoadTransposeMatrixf },
  { "glLoadTransposeMatrixfARB", "GL_ARB_transpose_matrix", wine_glLoadTransposeMatrixfARB },
  { "glLockArraysEXT", "GL_EXT_compiled_vertex_array", wine_glLockArraysEXT },
  { "glMTexCoord2fSGIS", "GL_SGIS_multitexture", wine_glMTexCoord2fSGIS },
  { "glMTexCoord2fvSGIS", "GL_SGIS_multitexture", wine_glMTexCoord2fvSGIS },
  { "glMakeBufferNonResidentNV", "GL_NV_shader_buffer_load", wine_glMakeBufferNonResidentNV },
  { "glMakeBufferResidentNV", "GL_NV_shader_buffer_load", wine_glMakeBufferResidentNV },
  { "glMakeImageHandleNonResidentNV", "GL_NV_bindless_texture", wine_glMakeImageHandleNonResidentNV },
  { "glMakeImageHandleResidentNV", "GL_NV_bindless_texture", wine_glMakeImageHandleResidentNV },
  { "glMakeNamedBufferNonResidentNV", "GL_NV_shader_buffer_load", wine_glMakeNamedBufferNonResidentNV },
  { "glMakeNamedBufferResidentNV", "GL_NV_shader_buffer_load", wine_glMakeNamedBufferResidentNV },
  { "glMakeTextureHandleNonResidentNV", "GL_NV_bindless_texture", wine_glMakeTextureHandleNonResidentNV },
  { "glMakeTextureHandleResidentNV", "GL_NV_bindless_texture", wine_glMakeTextureHandleResidentNV },
  { "glMapBuffer", "GL_VERSION_1_5", wine_glMapBuffer },
  { "glMapBufferARB", "GL_ARB_vertex_buffer_object", wine_glMapBufferARB },
  { "glMapBufferRange", "GL_ARB_map_buffer_range", wine_glMapBufferRange },
  { "glMapControlPointsNV", "GL_NV_evaluators", wine_glMapControlPointsNV },
  { "glMapNamedBufferEXT", "GL_EXT_direct_state_access", wine_glMapNamedBufferEXT },
  { "glMapNamedBufferRangeEXT", "GL_EXT_direct_state_access", wine_glMapNamedBufferRangeEXT },
  { "glMapObjectBufferATI", "GL_ATI_map_object_buffer", wine_glMapObjectBufferATI },
  { "glMapParameterfvNV", "GL_NV_evaluators", wine_glMapParameterfvNV },
  { "glMapParameterivNV", "GL_NV_evaluators", wine_glMapParameterivNV },
  { "glMapVertexAttrib1dAPPLE", "GL_APPLE_vertex_program_evaluators", wine_glMapVertexAttrib1dAPPLE },
  { "glMapVertexAttrib1fAPPLE", "GL_APPLE_vertex_program_evaluators", wine_glMapVertexAttrib1fAPPLE },
  { "glMapVertexAttrib2dAPPLE", "GL_APPLE_vertex_program_evaluators", wine_glMapVertexAttrib2dAPPLE },
  { "glMapVertexAttrib2fAPPLE", "GL_APPLE_vertex_program_evaluators", wine_glMapVertexAttrib2fAPPLE },
  { "glMatrixFrustumEXT", "GL_EXT_direct_state_access", wine_glMatrixFrustumEXT },
  { "glMatrixIndexPointerARB", "GL_ARB_matrix_palette", wine_glMatrixIndexPointerARB },
  { "glMatrixIndexubvARB", "GL_ARB_matrix_palette", wine_glMatrixIndexubvARB },
  { "glMatrixIndexuivARB", "GL_ARB_matrix_palette", wine_glMatrixIndexuivARB },
  { "glMatrixIndexusvARB", "GL_ARB_matrix_palette", wine_glMatrixIndexusvARB },
  { "glMatrixLoadIdentityEXT", "GL_EXT_direct_state_access", wine_glMatrixLoadIdentityEXT },
  { "glMatrixLoadTransposedEXT", "GL_EXT_direct_state_access", wine_glMatrixLoadTransposedEXT },
  { "glMatrixLoadTransposefEXT", "GL_EXT_direct_state_access", wine_glMatrixLoadTransposefEXT },
  { "glMatrixLoaddEXT", "GL_EXT_direct_state_access", wine_glMatrixLoaddEXT },
  { "glMatrixLoadfEXT", "GL_EXT_direct_state_access", wine_glMatrixLoadfEXT },
  { "glMatrixMultTransposedEXT", "GL_EXT_direct_state_access", wine_glMatrixMultTransposedEXT },
  { "glMatrixMultTransposefEXT", "GL_EXT_direct_state_access", wine_glMatrixMultTransposefEXT },
  { "glMatrixMultdEXT", "GL_EXT_direct_state_access", wine_glMatrixMultdEXT },
  { "glMatrixMultfEXT", "GL_EXT_direct_state_access", wine_glMatrixMultfEXT },
  { "glMatrixOrthoEXT", "GL_EXT_direct_state_access", wine_glMatrixOrthoEXT },
  { "glMatrixPopEXT", "GL_EXT_direct_state_access", wine_glMatrixPopEXT },
  { "glMatrixPushEXT", "GL_EXT_direct_state_access", wine_glMatrixPushEXT },
  { "glMatrixRotatedEXT", "GL_EXT_direct_state_access", wine_glMatrixRotatedEXT },
  { "glMatrixRotatefEXT", "GL_EXT_direct_state_access", wine_glMatrixRotatefEXT },
  { "glMatrixScaledEXT", "GL_EXT_direct_state_access", wine_glMatrixScaledEXT },
  { "glMatrixScalefEXT", "GL_EXT_direct_state_access", wine_glMatrixScalefEXT },
  { "glMatrixTranslatedEXT", "GL_EXT_direct_state_access", wine_glMatrixTranslatedEXT },
  { "glMatrixTranslatefEXT", "GL_EXT_direct_state_access", wine_glMatrixTranslatefEXT },
  { "glMemoryBarrier", "GL_ARB_shader_image_load_store", wine_glMemoryBarrier },
  { "glMemoryBarrierEXT", "GL_EXT_shader_image_load_store", wine_glMemoryBarrierEXT },
  { "glMinSampleShading", "GL_VERSION_4_0", wine_glMinSampleShading },
  { "glMinSampleShadingARB", "GL_ARB_sample_shading", wine_glMinSampleShadingARB },
  { "glMinmax", "GL_VERSION_1_2_DEPRECATED", wine_glMinmax },
  { "glMinmaxEXT", "GL_EXT_histogram", wine_glMinmaxEXT },
  { "glMultTransposeMatrixd", "GL_VERSION_1_3_DEPRECATED", wine_glMultTransposeMatrixd },
  { "glMultTransposeMatrixdARB", "GL_ARB_transpose_matrix", wine_glMultTransposeMatrixdARB },
  { "glMultTransposeMatrixf", "GL_VERSION_1_3_DEPRECATED", wine_glMultTransposeMatrixf },
  { "glMultTransposeMatrixfARB", "GL_ARB_transpose_matrix", wine_glMultTransposeMatrixfARB },
  { "glMultiDrawArrays", "GL_VERSION_1_4", wine_glMultiDrawArrays },
  { "glMultiDrawArraysEXT", "GL_EXT_multi_draw_arrays", wine_glMultiDrawArraysEXT },
  { "glMultiDrawArraysIndirectAMD", "GL_AMD_multi_draw_indirect", wine_glMultiDrawArraysIndirectAMD },
  { "glMultiDrawElementArrayAPPLE", "GL_APPLE_element_array", wine_glMultiDrawElementArrayAPPLE },
  { "glMultiDrawElements", "GL_VERSION_1_4", wine_glMultiDrawElements },
  { "glMultiDrawElementsBaseVertex", "GL_ARB_draw_elements_base_vertex", wine_glMultiDrawElementsBaseVertex },
  { "glMultiDrawElementsEXT", "GL_EXT_multi_draw_arrays", wine_glMultiDrawElementsEXT },
  { "glMultiDrawElementsIndirectAMD", "GL_AMD_multi_draw_indirect", wine_glMultiDrawElementsIndirectAMD },
  { "glMultiDrawRangeElementArrayAPPLE", "GL_APPLE_element_array", wine_glMultiDrawRangeElementArrayAPPLE },
  { "glMultiModeDrawArraysIBM", "GL_IBM_multimode_draw_arrays", wine_glMultiModeDrawArraysIBM },
  { "glMultiModeDrawElementsIBM", "GL_IBM_multimode_draw_arrays", wine_glMultiModeDrawElementsIBM },
  { "glMultiTexBufferEXT", "GL_EXT_direct_state_access", wine_glMultiTexBufferEXT },
  { "glMultiTexCoord1d", "GL_VERSION_1_3_DEPRECATED", wine_glMultiTexCoord1d },
  { "glMultiTexCoord1dARB", "GL_ARB_multitexture", wine_glMultiTexCoord1dARB },
  { "glMultiTexCoord1dSGIS", "GL_SGIS_multitexture", wine_glMultiTexCoord1dSGIS },
  { "glMultiTexCoord1dv", "GL_VERSION_1_3_DEPRECATED", wine_glMultiTexCoord1dv },
  { "glMultiTexCoord1dvARB", "GL_ARB_multitexture", wine_glMultiTexCoord1dvARB },
  { "glMultiTexCoord1dvSGIS", "GL_SGIS_multitexture", wine_glMultiTexCoord1dvSGIS },
  { "glMultiTexCoord1f", "GL_VERSION_1_3_DEPRECATED", wine_glMultiTexCoord1f },
  { "glMultiTexCoord1fARB", "GL_ARB_multitexture", wine_glMultiTexCoord1fARB },
  { "glMultiTexCoord1fSGIS", "GL_SGIS_multitexture", wine_glMultiTexCoord1fSGIS },
  { "glMultiTexCoord1fv", "GL_VERSION_1_3_DEPRECATED", wine_glMultiTexCoord1fv },
  { "glMultiTexCoord1fvARB", "GL_ARB_multitexture", wine_glMultiTexCoord1fvARB },
  { "glMultiTexCoord1fvSGIS", "GL_SGIS_multitexture", wine_glMultiTexCoord1fvSGIS },
  { "glMultiTexCoord1hNV", "GL_NV_half_float", wine_glMultiTexCoord1hNV },
  { "glMultiTexCoord1hvNV", "GL_NV_half_float", wine_glMultiTexCoord1hvNV },
  { "glMultiTexCoord1i", "GL_VERSION_1_3_DEPRECATED", wine_glMultiTexCoord1i },
  { "glMultiTexCoord1iARB", "GL_ARB_multitexture", wine_glMultiTexCoord1iARB },
  { "glMultiTexCoord1iSGIS", "GL_SGIS_multitexture", wine_glMultiTexCoord1iSGIS },
  { "glMultiTexCoord1iv", "GL_VERSION_1_3_DEPRECATED", wine_glMultiTexCoord1iv },
  { "glMultiTexCoord1ivARB", "GL_ARB_multitexture", wine_glMultiTexCoord1ivARB },
  { "glMultiTexCoord1ivSGIS", "GL_SGIS_multitexture", wine_glMultiTexCoord1ivSGIS },
  { "glMultiTexCoord1s", "GL_VERSION_1_3_DEPRECATED", wine_glMultiTexCoord1s },
  { "glMultiTexCoord1sARB", "GL_ARB_multitexture", wine_glMultiTexCoord1sARB },
  { "glMultiTexCoord1sSGIS", "GL_SGIS_multitexture", wine_glMultiTexCoord1sSGIS },
  { "glMultiTexCoord1sv", "GL_VERSION_1_3_DEPRECATED", wine_glMultiTexCoord1sv },
  { "glMultiTexCoord1svARB", "GL_ARB_multitexture", wine_glMultiTexCoord1svARB },
  { "glMultiTexCoord1svSGIS", "GL_SGIS_multitexture", wine_glMultiTexCoord1svSGIS },
  { "glMultiTexCoord2d", "GL_VERSION_1_3_DEPRECATED", wine_glMultiTexCoord2d },
  { "glMultiTexCoord2dARB", "GL_ARB_multitexture", wine_glMultiTexCoord2dARB },
  { "glMultiTexCoord2dSGIS", "GL_SGIS_multitexture", wine_glMultiTexCoord2dSGIS },
  { "glMultiTexCoord2dv", "GL_VERSION_1_3_DEPRECATED", wine_glMultiTexCoord2dv },
  { "glMultiTexCoord2dvARB", "GL_ARB_multitexture", wine_glMultiTexCoord2dvARB },
  { "glMultiTexCoord2dvSGIS", "GL_SGIS_multitexture", wine_glMultiTexCoord2dvSGIS },
  { "glMultiTexCoord2f", "GL_VERSION_1_3_DEPRECATED", wine_glMultiTexCoord2f },
  { "glMultiTexCoord2fARB", "GL_ARB_multitexture", wine_glMultiTexCoord2fARB },
  { "glMultiTexCoord2fSGIS", "GL_SGIS_multitexture", wine_glMultiTexCoord2fSGIS },
  { "glMultiTexCoord2fv", "GL_VERSION_1_3_DEPRECATED", wine_glMultiTexCoord2fv },
  { "glMultiTexCoord2fvARB", "GL_ARB_multitexture", wine_glMultiTexCoord2fvARB },
  { "glMultiTexCoord2fvSGIS", "GL_SGIS_multitexture", wine_glMultiTexCoord2fvSGIS },
  { "glMultiTexCoord2hNV", "GL_NV_half_float", wine_glMultiTexCoord2hNV },
  { "glMultiTexCoord2hvNV", "GL_NV_half_float", wine_glMultiTexCoord2hvNV },
  { "glMultiTexCoord2i", "GL_VERSION_1_3_DEPRECATED", wine_glMultiTexCoord2i },
  { "glMultiTexCoord2iARB", "GL_ARB_multitexture", wine_glMultiTexCoord2iARB },
  { "glMultiTexCoord2iSGIS", "GL_SGIS_multitexture", wine_glMultiTexCoord2iSGIS },
  { "glMultiTexCoord2iv", "GL_VERSION_1_3_DEPRECATED", wine_glMultiTexCoord2iv },
  { "glMultiTexCoord2ivARB", "GL_ARB_multitexture", wine_glMultiTexCoord2ivARB },
  { "glMultiTexCoord2ivSGIS", "GL_SGIS_multitexture", wine_glMultiTexCoord2ivSGIS },
  { "glMultiTexCoord2s", "GL_VERSION_1_3_DEPRECATED", wine_glMultiTexCoord2s },
  { "glMultiTexCoord2sARB", "GL_ARB_multitexture", wine_glMultiTexCoord2sARB },
  { "glMultiTexCoord2sSGIS", "GL_SGIS_multitexture", wine_glMultiTexCoord2sSGIS },
  { "glMultiTexCoord2sv", "GL_VERSION_1_3_DEPRECATED", wine_glMultiTexCoord2sv },
  { "glMultiTexCoord2svARB", "GL_ARB_multitexture", wine_glMultiTexCoord2svARB },
  { "glMultiTexCoord2svSGIS", "GL_SGIS_multitexture", wine_glMultiTexCoord2svSGIS },
  { "glMultiTexCoord3d", "GL_VERSION_1_3_DEPRECATED", wine_glMultiTexCoord3d },
  { "glMultiTexCoord3dARB", "GL_ARB_multitexture", wine_glMultiTexCoord3dARB },
  { "glMultiTexCoord3dSGIS", "GL_SGIS_multitexture", wine_glMultiTexCoord3dSGIS },
  { "glMultiTexCoord3dv", "GL_VERSION_1_3_DEPRECATED", wine_glMultiTexCoord3dv },
  { "glMultiTexCoord3dvARB", "GL_ARB_multitexture", wine_glMultiTexCoord3dvARB },
  { "glMultiTexCoord3dvSGIS", "GL_SGIS_multitexture", wine_glMultiTexCoord3dvSGIS },
  { "glMultiTexCoord3f", "GL_VERSION_1_3_DEPRECATED", wine_glMultiTexCoord3f },
  { "glMultiTexCoord3fARB", "GL_ARB_multitexture", wine_glMultiTexCoord3fARB },
  { "glMultiTexCoord3fSGIS", "GL_SGIS_multitexture", wine_glMultiTexCoord3fSGIS },
  { "glMultiTexCoord3fv", "GL_VERSION_1_3_DEPRECATED", wine_glMultiTexCoord3fv },
  { "glMultiTexCoord3fvARB", "GL_ARB_multitexture", wine_glMultiTexCoord3fvARB },
  { "glMultiTexCoord3fvSGIS", "GL_SGIS_multitexture", wine_glMultiTexCoord3fvSGIS },
  { "glMultiTexCoord3hNV", "GL_NV_half_float", wine_glMultiTexCoord3hNV },
  { "glMultiTexCoord3hvNV", "GL_NV_half_float", wine_glMultiTexCoord3hvNV },
  { "glMultiTexCoord3i", "GL_VERSION_1_3_DEPRECATED", wine_glMultiTexCoord3i },
  { "glMultiTexCoord3iARB", "GL_ARB_multitexture", wine_glMultiTexCoord3iARB },
  { "glMultiTexCoord3iSGIS", "GL_SGIS_multitexture", wine_glMultiTexCoord3iSGIS },
  { "glMultiTexCoord3iv", "GL_VERSION_1_3_DEPRECATED", wine_glMultiTexCoord3iv },
  { "glMultiTexCoord3ivARB", "GL_ARB_multitexture", wine_glMultiTexCoord3ivARB },
  { "glMultiTexCoord3ivSGIS", "GL_SGIS_multitexture", wine_glMultiTexCoord3ivSGIS },
  { "glMultiTexCoord3s", "GL_VERSION_1_3_DEPRECATED", wine_glMultiTexCoord3s },
  { "glMultiTexCoord3sARB", "GL_ARB_multitexture", wine_glMultiTexCoord3sARB },
  { "glMultiTexCoord3sSGIS", "GL_SGIS_multitexture", wine_glMultiTexCoord3sSGIS },
  { "glMultiTexCoord3sv", "GL_VERSION_1_3_DEPRECATED", wine_glMultiTexCoord3sv },
  { "glMultiTexCoord3svARB", "GL_ARB_multitexture", wine_glMultiTexCoord3svARB },
  { "glMultiTexCoord3svSGIS", "GL_SGIS_multitexture", wine_glMultiTexCoord3svSGIS },
  { "glMultiTexCoord4d", "GL_VERSION_1_3_DEPRECATED", wine_glMultiTexCoord4d },
  { "glMultiTexCoord4dARB", "GL_ARB_multitexture", wine_glMultiTexCoord4dARB },
  { "glMultiTexCoord4dSGIS", "GL_SGIS_multitexture", wine_glMultiTexCoord4dSGIS },
  { "glMultiTexCoord4dv", "GL_VERSION_1_3_DEPRECATED", wine_glMultiTexCoord4dv },
  { "glMultiTexCoord4dvARB", "GL_ARB_multitexture", wine_glMultiTexCoord4dvARB },
  { "glMultiTexCoord4dvSGIS", "GL_SGIS_multitexture", wine_glMultiTexCoord4dvSGIS },
  { "glMultiTexCoord4f", "GL_VERSION_1_3_DEPRECATED", wine_glMultiTexCoord4f },
  { "glMultiTexCoord4fARB", "GL_ARB_multitexture", wine_glMultiTexCoord4fARB },
  { "glMultiTexCoord4fSGIS", "GL_SGIS_multitexture", wine_glMultiTexCoord4fSGIS },
  { "glMultiTexCoord4fv", "GL_VERSION_1_3_DEPRECATED", wine_glMultiTexCoord4fv },
  { "glMultiTexCoord4fvARB", "GL_ARB_multitexture", wine_glMultiTexCoord4fvARB },
  { "glMultiTexCoord4fvSGIS", "GL_SGIS_multitexture", wine_glMultiTexCoord4fvSGIS },
  { "glMultiTexCoord4hNV", "GL_NV_half_float", wine_glMultiTexCoord4hNV },
  { "glMultiTexCoord4hvNV", "GL_NV_half_float", wine_glMultiTexCoord4hvNV },
  { "glMultiTexCoord4i", "GL_VERSION_1_3_DEPRECATED", wine_glMultiTexCoord4i },
  { "glMultiTexCoord4iARB", "GL_ARB_multitexture", wine_glMultiTexCoord4iARB },
  { "glMultiTexCoord4iSGIS", "GL_SGIS_multitexture", wine_glMultiTexCoord4iSGIS },
  { "glMultiTexCoord4iv", "GL_VERSION_1_3_DEPRECATED", wine_glMultiTexCoord4iv },
  { "glMultiTexCoord4ivARB", "GL_ARB_multitexture", wine_glMultiTexCoord4ivARB },
  { "glMultiTexCoord4ivSGIS", "GL_SGIS_multitexture", wine_glMultiTexCoord4ivSGIS },
  { "glMultiTexCoord4s", "GL_VERSION_1_3_DEPRECATED", wine_glMultiTexCoord4s },
  { "glMultiTexCoord4sARB", "GL_ARB_multitexture", wine_glMultiTexCoord4sARB },
  { "glMultiTexCoord4sSGIS", "GL_SGIS_multitexture", wine_glMultiTexCoord4sSGIS },
  { "glMultiTexCoord4sv", "GL_VERSION_1_3_DEPRECATED", wine_glMultiTexCoord4sv },
  { "glMultiTexCoord4svARB", "GL_ARB_multitexture", wine_glMultiTexCoord4svARB },
  { "glMultiTexCoord4svSGIS", "GL_SGIS_multitexture", wine_glMultiTexCoord4svSGIS },
  { "glMultiTexCoordP1ui", "GL_ARB_vertex_type_2_10_10_10_rev", wine_glMultiTexCoordP1ui },
  { "glMultiTexCoordP1uiv", "GL_ARB_vertex_type_2_10_10_10_rev", wine_glMultiTexCoordP1uiv },
  { "glMultiTexCoordP2ui", "GL_ARB_vertex_type_2_10_10_10_rev", wine_glMultiTexCoordP2ui },
  { "glMultiTexCoordP2uiv", "GL_ARB_vertex_type_2_10_10_10_rev", wine_glMultiTexCoordP2uiv },
  { "glMultiTexCoordP3ui", "GL_ARB_vertex_type_2_10_10_10_rev", wine_glMultiTexCoordP3ui },
  { "glMultiTexCoordP3uiv", "GL_ARB_vertex_type_2_10_10_10_rev", wine_glMultiTexCoordP3uiv },
  { "glMultiTexCoordP4ui", "GL_ARB_vertex_type_2_10_10_10_rev", wine_glMultiTexCoordP4ui },
  { "glMultiTexCoordP4uiv", "GL_ARB_vertex_type_2_10_10_10_rev", wine_glMultiTexCoordP4uiv },
  { "glMultiTexCoordPointerEXT", "GL_EXT_direct_state_access", wine_glMultiTexCoordPointerEXT },
  { "glMultiTexCoordPointerSGIS", "GL_SGIS_multitexture", wine_glMultiTexCoordPointerSGIS },
  { "glMultiTexEnvfEXT", "GL_EXT_direct_state_access", wine_glMultiTexEnvfEXT },
  { "glMultiTexEnvfvEXT", "GL_EXT_direct_state_access", wine_glMultiTexEnvfvEXT },
  { "glMultiTexEnviEXT", "GL_EXT_direct_state_access", wine_glMultiTexEnviEXT },
  { "glMultiTexEnvivEXT", "GL_EXT_direct_state_access", wine_glMultiTexEnvivEXT },
  { "glMultiTexGendEXT", "GL_EXT_direct_state_access", wine_glMultiTexGendEXT },
  { "glMultiTexGendvEXT", "GL_EXT_direct_state_access", wine_glMultiTexGendvEXT },
  { "glMultiTexGenfEXT", "GL_EXT_direct_state_access", wine_glMultiTexGenfEXT },
  { "glMultiTexGenfvEXT", "GL_EXT_direct_state_access", wine_glMultiTexGenfvEXT },
  { "glMultiTexGeniEXT", "GL_EXT_direct_state_access", wine_glMultiTexGeniEXT },
  { "glMultiTexGenivEXT", "GL_EXT_direct_state_access", wine_glMultiTexGenivEXT },
  { "glMultiTexImage1DEXT", "GL_EXT_direct_state_access", wine_glMultiTexImage1DEXT },
  { "glMultiTexImage2DEXT", "GL_EXT_direct_state_access", wine_glMultiTexImage2DEXT },
  { "glMultiTexImage3DEXT", "GL_EXT_direct_state_access", wine_glMultiTexImage3DEXT },
  { "glMultiTexParameterIivEXT", "GL_EXT_direct_state_access", wine_glMultiTexParameterIivEXT },
  { "glMultiTexParameterIuivEXT", "GL_EXT_direct_state_access", wine_glMultiTexParameterIuivEXT },
  { "glMultiTexParameterfEXT", "GL_EXT_direct_state_access", wine_glMultiTexParameterfEXT },
  { "glMultiTexParameterfvEXT", "GL_EXT_direct_state_access", wine_glMultiTexParameterfvEXT },
  { "glMultiTexParameteriEXT", "GL_EXT_direct_state_access", wine_glMultiTexParameteriEXT },
  { "glMultiTexParameterivEXT", "GL_EXT_direct_state_access", wine_glMultiTexParameterivEXT },
  { "glMultiTexRenderbufferEXT", "GL_EXT_direct_state_access", wine_glMultiTexRenderbufferEXT },
  { "glMultiTexSubImage1DEXT", "GL_EXT_direct_state_access", wine_glMultiTexSubImage1DEXT },
  { "glMultiTexSubImage2DEXT", "GL_EXT_direct_state_access", wine_glMultiTexSubImage2DEXT },
  { "glMultiTexSubImage3DEXT", "GL_EXT_direct_state_access", wine_glMultiTexSubImage3DEXT },
  { "glNamedBufferDataEXT", "GL_EXT_direct_state_access", wine_glNamedBufferDataEXT },
  { "glNamedBufferSubDataEXT", "GL_EXT_direct_state_access", wine_glNamedBufferSubDataEXT },
  { "glNamedCopyBufferSubDataEXT", "GL_EXT_direct_state_access", wine_glNamedCopyBufferSubDataEXT },
  { "glNamedFramebufferRenderbufferEXT", "GL_EXT_direct_state_access", wine_glNamedFramebufferRenderbufferEXT },
  { "glNamedFramebufferTexture1DEXT", "GL_EXT_direct_state_access", wine_glNamedFramebufferTexture1DEXT },
  { "glNamedFramebufferTexture2DEXT", "GL_EXT_direct_state_access", wine_glNamedFramebufferTexture2DEXT },
  { "glNamedFramebufferTexture3DEXT", "GL_EXT_direct_state_access", wine_glNamedFramebufferTexture3DEXT },
  { "glNamedFramebufferTextureEXT", "GL_EXT_direct_state_access", wine_glNamedFramebufferTextureEXT },
  { "glNamedFramebufferTextureFaceEXT", "GL_EXT_direct_state_access", wine_glNamedFramebufferTextureFaceEXT },
  { "glNamedFramebufferTextureLayerEXT", "GL_EXT_direct_state_access", wine_glNamedFramebufferTextureLayerEXT },
  { "glNamedProgramLocalParameter4dEXT", "GL_EXT_direct_state_access", wine_glNamedProgramLocalParameter4dEXT },
  { "glNamedProgramLocalParameter4dvEXT", "GL_EXT_direct_state_access", wine_glNamedProgramLocalParameter4dvEXT },
  { "glNamedProgramLocalParameter4fEXT", "GL_EXT_direct_state_access", wine_glNamedProgramLocalParameter4fEXT },
  { "glNamedProgramLocalParameter4fvEXT", "GL_EXT_direct_state_access", wine_glNamedProgramLocalParameter4fvEXT },
  { "glNamedProgramLocalParameterI4iEXT", "GL_EXT_direct_state_access", wine_glNamedProgramLocalParameterI4iEXT },
  { "glNamedProgramLocalParameterI4ivEXT", "GL_EXT_direct_state_access", wine_glNamedProgramLocalParameterI4ivEXT },
  { "glNamedProgramLocalParameterI4uiEXT", "GL_EXT_direct_state_access", wine_glNamedProgramLocalParameterI4uiEXT },
  { "glNamedProgramLocalParameterI4uivEXT", "GL_EXT_direct_state_access", wine_glNamedProgramLocalParameterI4uivEXT },
  { "glNamedProgramLocalParameters4fvEXT", "GL_EXT_direct_state_access", wine_glNamedProgramLocalParameters4fvEXT },
  { "glNamedProgramLocalParametersI4ivEXT", "GL_EXT_direct_state_access", wine_glNamedProgramLocalParametersI4ivEXT },
  { "glNamedProgramLocalParametersI4uivEXT", "GL_EXT_direct_state_access", wine_glNamedProgramLocalParametersI4uivEXT },
  { "glNamedProgramStringEXT", "GL_EXT_direct_state_access", wine_glNamedProgramStringEXT },
  { "glNamedRenderbufferStorageEXT", "GL_EXT_direct_state_access", wine_glNamedRenderbufferStorageEXT },
  { "glNamedRenderbufferStorageMultisampleCoverageEXT", "GL_EXT_direct_state_access", wine_glNamedRenderbufferStorageMultisampleCoverageEXT },
  { "glNamedRenderbufferStorageMultisampleEXT", "GL_EXT_direct_state_access", wine_glNamedRenderbufferStorageMultisampleEXT },
  { "glNamedStringARB", "GL_ARB_shading_language_include", wine_glNamedStringARB },
  { "glNewBufferRegion", "GL_KTX_buffer_region", wine_glNewBufferRegion },
  { "glNewObjectBufferATI", "GL_ATI_vertex_array_object", wine_glNewObjectBufferATI },
  { "glNormal3fVertex3fSUN", "GL_SUN_vertex", wine_glNormal3fVertex3fSUN },
  { "glNormal3fVertex3fvSUN", "GL_SUN_vertex", wine_glNormal3fVertex3fvSUN },
  { "glNormal3hNV", "GL_NV_half_float", wine_glNormal3hNV },
  { "glNormal3hvNV", "GL_NV_half_float", wine_glNormal3hvNV },
  { "glNormalFormatNV", "GL_NV_vertex_buffer_unified_memory", wine_glNormalFormatNV },
  { "glNormalP3ui", "GL_ARB_vertex_type_2_10_10_10_rev", wine_glNormalP3ui },
  { "glNormalP3uiv", "GL_ARB_vertex_type_2_10_10_10_rev", wine_glNormalP3uiv },
  { "glNormalPointerEXT", "GL_EXT_vertex_array", wine_glNormalPointerEXT },
  { "glNormalPointerListIBM", "GL_IBM_vertex_array_lists", wine_glNormalPointerListIBM },
  { "glNormalPointervINTEL", "GL_INTEL_parallel_arrays", wine_glNormalPointervINTEL },
  { "glNormalStream3bATI", "GL_ATI_vertex_streams", wine_glNormalStream3bATI },
  { "glNormalStream3bvATI", "GL_ATI_vertex_streams", wine_glNormalStream3bvATI },
  { "glNormalStream3dATI", "GL_ATI_vertex_streams", wine_glNormalStream3dATI },
  { "glNormalStream3dvATI", "GL_ATI_vertex_streams", wine_glNormalStream3dvATI },
  { "glNormalStream3fATI", "GL_ATI_vertex_streams", wine_glNormalStream3fATI },
  { "glNormalStream3fvATI", "GL_ATI_vertex_streams", wine_glNormalStream3fvATI },
  { "glNormalStream3iATI", "GL_ATI_vertex_streams", wine_glNormalStream3iATI },
  { "glNormalStream3ivATI", "GL_ATI_vertex_streams", wine_glNormalStream3ivATI },
  { "glNormalStream3sATI", "GL_ATI_vertex_streams", wine_glNormalStream3sATI },
  { "glNormalStream3svATI", "GL_ATI_vertex_streams", wine_glNormalStream3svATI },
  { "glObjectPurgeableAPPLE", "GL_APPLE_object_purgeable", wine_glObjectPurgeableAPPLE },
  { "glObjectUnpurgeableAPPLE", "GL_APPLE_object_purgeable", wine_glObjectUnpurgeableAPPLE },
  { "glPNTrianglesfATI", "GL_ATI_pn_triangles", wine_glPNTrianglesfATI },
  { "glPNTrianglesiATI", "GL_ATI_pn_triangles", wine_glPNTrianglesiATI },
  { "glPassTexCoordATI", "GL_ATI_fragment_shader", wine_glPassTexCoordATI },
  { "glPatchParameterfv", "GL_ARB_tessellation_shader", wine_glPatchParameterfv },
  { "glPatchParameteri", "GL_ARB_tessellation_shader", wine_glPatchParameteri },
  { "glPathColorGenNV", "GL_NV_path_rendering", wine_glPathColorGenNV },
  { "glPathCommandsNV", "GL_NV_path_rendering", wine_glPathCommandsNV },
  { "glPathCoordsNV", "GL_NV_path_rendering", wine_glPathCoordsNV },
  { "glPathCoverDepthFuncNV", "GL_NV_path_rendering", wine_glPathCoverDepthFuncNV },
  { "glPathDashArrayNV", "GL_NV_path_rendering", wine_glPathDashArrayNV },
  { "glPathFogGenNV", "GL_NV_path_rendering", wine_glPathFogGenNV },
  { "glPathGlyphRangeNV", "GL_NV_path_rendering", wine_glPathGlyphRangeNV },
  { "glPathGlyphsNV", "GL_NV_path_rendering", wine_glPathGlyphsNV },
  { "glPathParameterfNV", "GL_NV_path_rendering", wine_glPathParameterfNV },
  { "glPathParameterfvNV", "GL_NV_path_rendering", wine_glPathParameterfvNV },
  { "glPathParameteriNV", "GL_NV_path_rendering", wine_glPathParameteriNV },
  { "glPathParameterivNV", "GL_NV_path_rendering", wine_glPathParameterivNV },
  { "glPathStencilDepthOffsetNV", "GL_NV_path_rendering", wine_glPathStencilDepthOffsetNV },
  { "glPathStencilFuncNV", "GL_NV_path_rendering", wine_glPathStencilFuncNV },
  { "glPathStringNV", "GL_NV_path_rendering", wine_glPathStringNV },
  { "glPathSubCommandsNV", "GL_NV_path_rendering", wine_glPathSubCommandsNV },
  { "glPathSubCoordsNV", "GL_NV_path_rendering", wine_glPathSubCoordsNV },
  { "glPathTexGenNV", "GL_NV_path_rendering", wine_glPathTexGenNV },
  { "glPauseTransformFeedback", "GL_ARB_transform_feedback2", wine_glPauseTransformFeedback },
  { "glPauseTransformFeedbackNV", "GL_NV_transform_feedback2", wine_glPauseTransformFeedbackNV },
  { "glPixelDataRangeNV", "GL_NV_pixel_data_range", wine_glPixelDataRangeNV },
  { "glPixelTexGenParameterfSGIS", "GL_SGIS_pixel_texture", wine_glPixelTexGenParameterfSGIS },
  { "glPixelTexGenParameterfvSGIS", "GL_SGIS_pixel_texture", wine_glPixelTexGenParameterfvSGIS },
  { "glPixelTexGenParameteriSGIS", "GL_SGIS_pixel_texture", wine_glPixelTexGenParameteriSGIS },
  { "glPixelTexGenParameterivSGIS", "GL_SGIS_pixel_texture", wine_glPixelTexGenParameterivSGIS },
  { "glPixelTexGenSGIX", "GL_SGIX_pixel_texture", wine_glPixelTexGenSGIX },
  { "glPixelTransformParameterfEXT", "GL_EXT_pixel_transform", wine_glPixelTransformParameterfEXT },
  { "glPixelTransformParameterfvEXT", "GL_EXT_pixel_transform", wine_glPixelTransformParameterfvEXT },
  { "glPixelTransformParameteriEXT", "GL_EXT_pixel_transform", wine_glPixelTransformParameteriEXT },
  { "glPixelTransformParameterivEXT", "GL_EXT_pixel_transform", wine_glPixelTransformParameterivEXT },
  { "glPointAlongPathNV", "GL_NV_path_rendering", wine_glPointAlongPathNV },
  { "glPointParameterf", "GL_VERSION_1_4", wine_glPointParameterf },
  { "glPointParameterfARB", "GL_ARB_point_parameters", wine_glPointParameterfARB },
  { "glPointParameterfEXT", "GL_EXT_point_parameters", wine_glPointParameterfEXT },
  { "glPointParameterfSGIS", "GL_SGIS_point_parameters", wine_glPointParameterfSGIS },
  { "glPointParameterfv", "GL_VERSION_1_4", wine_glPointParameterfv },
  { "glPointParameterfvARB", "GL_ARB_point_parameters", wine_glPointParameterfvARB },
  { "glPointParameterfvEXT", "GL_EXT_point_parameters", wine_glPointParameterfvEXT },
  { "glPointParameterfvSGIS", "GL_SGIS_point_parameters", wine_glPointParameterfvSGIS },
  { "glPointParameteri", "GL_VERSION_1_4", wine_glPointParameteri },
  { "glPointParameteriNV", "GL_NV_point_sprite", wine_glPointParameteriNV },
  { "glPointParameteriv", "GL_VERSION_1_4", wine_glPointParameteriv },
  { "glPointParameterivNV", "GL_NV_point_sprite", wine_glPointParameterivNV },
  { "glPollAsyncSGIX", "GL_SGIX_async", wine_glPollAsyncSGIX },
  { "glPollInstrumentsSGIX", "GL_SGIX_instruments", wine_glPollInstrumentsSGIX },
  { "glPolygonOffsetEXT", "GL_EXT_polygon_offset", wine_glPolygonOffsetEXT },
  { "glPresentFrameDualFillNV", "GL_NV_present_video", wine_glPresentFrameDualFillNV },
  { "glPresentFrameKeyedNV", "GL_NV_present_video", wine_glPresentFrameKeyedNV },
  { "glPrimitiveRestartIndex", "GL_VERSION_3_1", wine_glPrimitiveRestartIndex },
  { "glPrimitiveRestartIndexNV", "GL_NV_primitive_restart", wine_glPrimitiveRestartIndexNV },
  { "glPrimitiveRestartNV", "GL_NV_primitive_restart", wine_glPrimitiveRestartNV },
  { "glPrioritizeTexturesEXT", "GL_EXT_texture_object", wine_glPrioritizeTexturesEXT },
  { "glProgramBinary", "GL_ARB_get_program_binary", wine_glProgramBinary },
  { "glProgramBufferParametersIivNV", "GL_NV_parameter_buffer_object", wine_glProgramBufferParametersIivNV },
  { "glProgramBufferParametersIuivNV", "GL_NV_parameter_buffer_object", wine_glProgramBufferParametersIuivNV },
  { "glProgramBufferParametersfvNV", "GL_NV_parameter_buffer_object", wine_glProgramBufferParametersfvNV },
  { "glProgramEnvParameter4dARB", "GL_ARB_vertex_program", wine_glProgramEnvParameter4dARB },
  { "glProgramEnvParameter4dvARB", "GL_ARB_vertex_program", wine_glProgramEnvParameter4dvARB },
  { "glProgramEnvParameter4fARB", "GL_ARB_vertex_program", wine_glProgramEnvParameter4fARB },
  { "glProgramEnvParameter4fvARB", "GL_ARB_vertex_program", wine_glProgramEnvParameter4fvARB },
  { "glProgramEnvParameterI4iNV", "GL_NV_gpu_program4", wine_glProgramEnvParameterI4iNV },
  { "glProgramEnvParameterI4ivNV", "GL_NV_gpu_program4", wine_glProgramEnvParameterI4ivNV },
  { "glProgramEnvParameterI4uiNV", "GL_NV_gpu_program4", wine_glProgramEnvParameterI4uiNV },
  { "glProgramEnvParameterI4uivNV", "GL_NV_gpu_program4", wine_glProgramEnvParameterI4uivNV },
  { "glProgramEnvParameters4fvEXT", "GL_EXT_gpu_program_parameters", wine_glProgramEnvParameters4fvEXT },
  { "glProgramEnvParametersI4ivNV", "GL_NV_gpu_program4", wine_glProgramEnvParametersI4ivNV },
  { "glProgramEnvParametersI4uivNV", "GL_NV_gpu_program4", wine_glProgramEnvParametersI4uivNV },
  { "glProgramLocalParameter4dARB", "GL_ARB_vertex_program", wine_glProgramLocalParameter4dARB },
  { "glProgramLocalParameter4dvARB", "GL_ARB_vertex_program", wine_glProgramLocalParameter4dvARB },
  { "glProgramLocalParameter4fARB", "GL_ARB_vertex_program", wine_glProgramLocalParameter4fARB },
  { "glProgramLocalParameter4fvARB", "GL_ARB_vertex_program", wine_glProgramLocalParameter4fvARB },
  { "glProgramLocalParameterI4iNV", "GL_NV_gpu_program4", wine_glProgramLocalParameterI4iNV },
  { "glProgramLocalParameterI4ivNV", "GL_NV_gpu_program4", wine_glProgramLocalParameterI4ivNV },
  { "glProgramLocalParameterI4uiNV", "GL_NV_gpu_program4", wine_glProgramLocalParameterI4uiNV },
  { "glProgramLocalParameterI4uivNV", "GL_NV_gpu_program4", wine_glProgramLocalParameterI4uivNV },
  { "glProgramLocalParameters4fvEXT", "GL_EXT_gpu_program_parameters", wine_glProgramLocalParameters4fvEXT },
  { "glProgramLocalParametersI4ivNV", "GL_NV_gpu_program4", wine_glProgramLocalParametersI4ivNV },
  { "glProgramLocalParametersI4uivNV", "GL_NV_gpu_program4", wine_glProgramLocalParametersI4uivNV },
  { "glProgramNamedParameter4dNV", "GL_NV_fragment_program", wine_glProgramNamedParameter4dNV },
  { "glProgramNamedParameter4dvNV", "GL_NV_fragment_program", wine_glProgramNamedParameter4dvNV },
  { "glProgramNamedParameter4fNV", "GL_NV_fragment_program", wine_glProgramNamedParameter4fNV },
  { "glProgramNamedParameter4fvNV", "GL_NV_fragment_program", wine_glProgramNamedParameter4fvNV },
  { "glProgramParameter4dNV", "GL_NV_vertex_program", wine_glProgramParameter4dNV },
  { "glProgramParameter4dvNV", "GL_NV_vertex_program", wine_glProgramParameter4dvNV },
  { "glProgramParameter4fNV", "GL_NV_vertex_program", wine_glProgramParameter4fNV },
  { "glProgramParameter4fvNV", "GL_NV_vertex_program", wine_glProgramParameter4fvNV },
  { "glProgramParameteri", "GL_ARB_get_program_binary", wine_glProgramParameteri },
  { "glProgramParameteriARB", "GL_ARB_geometry_shader4", wine_glProgramParameteriARB },
  { "glProgramParameteriEXT", "GL_EXT_geometry_shader4", wine_glProgramParameteriEXT },
  { "glProgramParameters4dvNV", "GL_NV_vertex_program", wine_glProgramParameters4dvNV },
  { "glProgramParameters4fvNV", "GL_NV_vertex_program", wine_glProgramParameters4fvNV },
  { "glProgramStringARB", "GL_ARB_vertex_program", wine_glProgramStringARB },
  { "glProgramSubroutineParametersuivNV", "GL_NV_gpu_program5", wine_glProgramSubroutineParametersuivNV },
  { "glProgramUniform1d", "GL_ARB_separate_shader_objects", wine_glProgramUniform1d },
  { "glProgramUniform1dEXT", "GL_EXT_direct_state_access", wine_glProgramUniform1dEXT },
  { "glProgramUniform1dv", "GL_ARB_separate_shader_objects", wine_glProgramUniform1dv },
  { "glProgramUniform1dvEXT", "GL_EXT_direct_state_access", wine_glProgramUniform1dvEXT },
  { "glProgramUniform1f", "GL_ARB_separate_shader_objects", wine_glProgramUniform1f },
  { "glProgramUniform1fEXT", "GL_EXT_direct_state_access", wine_glProgramUniform1fEXT },
  { "glProgramUniform1fv", "GL_ARB_separate_shader_objects", wine_glProgramUniform1fv },
  { "glProgramUniform1fvEXT", "GL_EXT_direct_state_access", wine_glProgramUniform1fvEXT },
  { "glProgramUniform1i", "GL_ARB_separate_shader_objects", wine_glProgramUniform1i },
  { "glProgramUniform1i64NV", "GL_NV_gpu_shader5", wine_glProgramUniform1i64NV },
  { "glProgramUniform1i64vNV", "GL_NV_gpu_shader5", wine_glProgramUniform1i64vNV },
  { "glProgramUniform1iEXT", "GL_EXT_direct_state_access", wine_glProgramUniform1iEXT },
  { "glProgramUniform1iv", "GL_ARB_separate_shader_objects", wine_glProgramUniform1iv },
  { "glProgramUniform1ivEXT", "GL_EXT_direct_state_access", wine_glProgramUniform1ivEXT },
  { "glProgramUniform1ui", "GL_ARB_separate_shader_objects", wine_glProgramUniform1ui },
  { "glProgramUniform1ui64NV", "GL_NV_gpu_shader5", wine_glProgramUniform1ui64NV },
  { "glProgramUniform1ui64vNV", "GL_NV_gpu_shader5", wine_glProgramUniform1ui64vNV },
  { "glProgramUniform1uiEXT", "GL_EXT_direct_state_access", wine_glProgramUniform1uiEXT },
  { "glProgramUniform1uiv", "GL_ARB_separate_shader_objects", wine_glProgramUniform1uiv },
  { "glProgramUniform1uivEXT", "GL_EXT_direct_state_access", wine_glProgramUniform1uivEXT },
  { "glProgramUniform2d", "GL_ARB_separate_shader_objects", wine_glProgramUniform2d },
  { "glProgramUniform2dEXT", "GL_EXT_direct_state_access", wine_glProgramUniform2dEXT },
  { "glProgramUniform2dv", "GL_ARB_separate_shader_objects", wine_glProgramUniform2dv },
  { "glProgramUniform2dvEXT", "GL_EXT_direct_state_access", wine_glProgramUniform2dvEXT },
  { "glProgramUniform2f", "GL_ARB_separate_shader_objects", wine_glProgramUniform2f },
  { "glProgramUniform2fEXT", "GL_EXT_direct_state_access", wine_glProgramUniform2fEXT },
  { "glProgramUniform2fv", "GL_ARB_separate_shader_objects", wine_glProgramUniform2fv },
  { "glProgramUniform2fvEXT", "GL_EXT_direct_state_access", wine_glProgramUniform2fvEXT },
  { "glProgramUniform2i", "GL_ARB_separate_shader_objects", wine_glProgramUniform2i },
  { "glProgramUniform2i64NV", "GL_NV_gpu_shader5", wine_glProgramUniform2i64NV },
  { "glProgramUniform2i64vNV", "GL_NV_gpu_shader5", wine_glProgramUniform2i64vNV },
  { "glProgramUniform2iEXT", "GL_EXT_direct_state_access", wine_glProgramUniform2iEXT },
  { "glProgramUniform2iv", "GL_ARB_separate_shader_objects", wine_glProgramUniform2iv },
  { "glProgramUniform2ivEXT", "GL_EXT_direct_state_access", wine_glProgramUniform2ivEXT },
  { "glProgramUniform2ui", "GL_ARB_separate_shader_objects", wine_glProgramUniform2ui },
  { "glProgramUniform2ui64NV", "GL_NV_gpu_shader5", wine_glProgramUniform2ui64NV },
  { "glProgramUniform2ui64vNV", "GL_NV_gpu_shader5", wine_glProgramUniform2ui64vNV },
  { "glProgramUniform2uiEXT", "GL_EXT_direct_state_access", wine_glProgramUniform2uiEXT },
  { "glProgramUniform2uiv", "GL_ARB_separate_shader_objects", wine_glProgramUniform2uiv },
  { "glProgramUniform2uivEXT", "GL_EXT_direct_state_access", wine_glProgramUniform2uivEXT },
  { "glProgramUniform3d", "GL_ARB_separate_shader_objects", wine_glProgramUniform3d },
  { "glProgramUniform3dEXT", "GL_EXT_direct_state_access", wine_glProgramUniform3dEXT },
  { "glProgramUniform3dv", "GL_ARB_separate_shader_objects", wine_glProgramUniform3dv },
  { "glProgramUniform3dvEXT", "GL_EXT_direct_state_access", wine_glProgramUniform3dvEXT },
  { "glProgramUniform3f", "GL_ARB_separate_shader_objects", wine_glProgramUniform3f },
  { "glProgramUniform3fEXT", "GL_EXT_direct_state_access", wine_glProgramUniform3fEXT },
  { "glProgramUniform3fv", "GL_ARB_separate_shader_objects", wine_glProgramUniform3fv },
  { "glProgramUniform3fvEXT", "GL_EXT_direct_state_access", wine_glProgramUniform3fvEXT },
  { "glProgramUniform3i", "GL_ARB_separate_shader_objects", wine_glProgramUniform3i },
  { "glProgramUniform3i64NV", "GL_NV_gpu_shader5", wine_glProgramUniform3i64NV },
  { "glProgramUniform3i64vNV", "GL_NV_gpu_shader5", wine_glProgramUniform3i64vNV },
  { "glProgramUniform3iEXT", "GL_EXT_direct_state_access", wine_glProgramUniform3iEXT },
  { "glProgramUniform3iv", "GL_ARB_separate_shader_objects", wine_glProgramUniform3iv },
  { "glProgramUniform3ivEXT", "GL_EXT_direct_state_access", wine_glProgramUniform3ivEXT },
  { "glProgramUniform3ui", "GL_ARB_separate_shader_objects", wine_glProgramUniform3ui },
  { "glProgramUniform3ui64NV", "GL_NV_gpu_shader5", wine_glProgramUniform3ui64NV },
  { "glProgramUniform3ui64vNV", "GL_NV_gpu_shader5", wine_glProgramUniform3ui64vNV },
  { "glProgramUniform3uiEXT", "GL_EXT_direct_state_access", wine_glProgramUniform3uiEXT },
  { "glProgramUniform3uiv", "GL_ARB_separate_shader_objects", wine_glProgramUniform3uiv },
  { "glProgramUniform3uivEXT", "GL_EXT_direct_state_access", wine_glProgramUniform3uivEXT },
  { "glProgramUniform4d", "GL_ARB_separate_shader_objects", wine_glProgramUniform4d },
  { "glProgramUniform4dEXT", "GL_EXT_direct_state_access", wine_glProgramUniform4dEXT },
  { "glProgramUniform4dv", "GL_ARB_separate_shader_objects", wine_glProgramUniform4dv },
  { "glProgramUniform4dvEXT", "GL_EXT_direct_state_access", wine_glProgramUniform4dvEXT },
  { "glProgramUniform4f", "GL_ARB_separate_shader_objects", wine_glProgramUniform4f },
  { "glProgramUniform4fEXT", "GL_EXT_direct_state_access", wine_glProgramUniform4fEXT },
  { "glProgramUniform4fv", "GL_ARB_separate_shader_objects", wine_glProgramUniform4fv },
  { "glProgramUniform4fvEXT", "GL_EXT_direct_state_access", wine_glProgramUniform4fvEXT },
  { "glProgramUniform4i", "GL_ARB_separate_shader_objects", wine_glProgramUniform4i },
  { "glProgramUniform4i64NV", "GL_NV_gpu_shader5", wine_glProgramUniform4i64NV },
  { "glProgramUniform4i64vNV", "GL_NV_gpu_shader5", wine_glProgramUniform4i64vNV },
  { "glProgramUniform4iEXT", "GL_EXT_direct_state_access", wine_glProgramUniform4iEXT },
  { "glProgramUniform4iv", "GL_ARB_separate_shader_objects", wine_glProgramUniform4iv },
  { "glProgramUniform4ivEXT", "GL_EXT_direct_state_access", wine_glProgramUniform4ivEXT },
  { "glProgramUniform4ui", "GL_ARB_separate_shader_objects", wine_glProgramUniform4ui },
  { "glProgramUniform4ui64NV", "GL_NV_gpu_shader5", wine_glProgramUniform4ui64NV },
  { "glProgramUniform4ui64vNV", "GL_NV_gpu_shader5", wine_glProgramUniform4ui64vNV },
  { "glProgramUniform4uiEXT", "GL_EXT_direct_state_access", wine_glProgramUniform4uiEXT },
  { "glProgramUniform4uiv", "GL_ARB_separate_shader_objects", wine_glProgramUniform4uiv },
  { "glProgramUniform4uivEXT", "GL_EXT_direct_state_access", wine_glProgramUniform4uivEXT },
  { "glProgramUniformHandleui64NV", "GL_NV_bindless_texture", wine_glProgramUniformHandleui64NV },
  { "glProgramUniformHandleui64vNV", "GL_NV_bindless_texture", wine_glProgramUniformHandleui64vNV },
  { "glProgramUniformMatrix2dv", "GL_ARB_separate_shader_objects", wine_glProgramUniformMatrix2dv },
  { "glProgramUniformMatrix2dvEXT", "GL_EXT_direct_state_access", wine_glProgramUniformMatrix2dvEXT },
  { "glProgramUniformMatrix2fv", "GL_ARB_separate_shader_objects", wine_glProgramUniformMatrix2fv },
  { "glProgramUniformMatrix2fvEXT", "GL_EXT_direct_state_access", wine_glProgramUniformMatrix2fvEXT },
  { "glProgramUniformMatrix2x3dv", "GL_ARB_separate_shader_objects", wine_glProgramUniformMatrix2x3dv },
  { "glProgramUniformMatrix2x3dvEXT", "GL_EXT_direct_state_access", wine_glProgramUniformMatrix2x3dvEXT },
  { "glProgramUniformMatrix2x3fv", "GL_ARB_separate_shader_objects", wine_glProgramUniformMatrix2x3fv },
  { "glProgramUniformMatrix2x3fvEXT", "GL_EXT_direct_state_access", wine_glProgramUniformMatrix2x3fvEXT },
  { "glProgramUniformMatrix2x4dv", "GL_ARB_separate_shader_objects", wine_glProgramUniformMatrix2x4dv },
  { "glProgramUniformMatrix2x4dvEXT", "GL_EXT_direct_state_access", wine_glProgramUniformMatrix2x4dvEXT },
  { "glProgramUniformMatrix2x4fv", "GL_ARB_separate_shader_objects", wine_glProgramUniformMatrix2x4fv },
  { "glProgramUniformMatrix2x4fvEXT", "GL_EXT_direct_state_access", wine_glProgramUniformMatrix2x4fvEXT },
  { "glProgramUniformMatrix3dv", "GL_ARB_separate_shader_objects", wine_glProgramUniformMatrix3dv },
  { "glProgramUniformMatrix3dvEXT", "GL_EXT_direct_state_access", wine_glProgramUniformMatrix3dvEXT },
  { "glProgramUniformMatrix3fv", "GL_ARB_separate_shader_objects", wine_glProgramUniformMatrix3fv },
  { "glProgramUniformMatrix3fvEXT", "GL_EXT_direct_state_access", wine_glProgramUniformMatrix3fvEXT },
  { "glProgramUniformMatrix3x2dv", "GL_ARB_separate_shader_objects", wine_glProgramUniformMatrix3x2dv },
  { "glProgramUniformMatrix3x2dvEXT", "GL_EXT_direct_state_access", wine_glProgramUniformMatrix3x2dvEXT },
  { "glProgramUniformMatrix3x2fv", "GL_ARB_separate_shader_objects", wine_glProgramUniformMatrix3x2fv },
  { "glProgramUniformMatrix3x2fvEXT", "GL_EXT_direct_state_access", wine_glProgramUniformMatrix3x2fvEXT },
  { "glProgramUniformMatrix3x4dv", "GL_ARB_separate_shader_objects", wine_glProgramUniformMatrix3x4dv },
  { "glProgramUniformMatrix3x4dvEXT", "GL_EXT_direct_state_access", wine_glProgramUniformMatrix3x4dvEXT },
  { "glProgramUniformMatrix3x4fv", "GL_ARB_separate_shader_objects", wine_glProgramUniformMatrix3x4fv },
  { "glProgramUniformMatrix3x4fvEXT", "GL_EXT_direct_state_access", wine_glProgramUniformMatrix3x4fvEXT },
  { "glProgramUniformMatrix4dv", "GL_ARB_separate_shader_objects", wine_glProgramUniformMatrix4dv },
  { "glProgramUniformMatrix4dvEXT", "GL_EXT_direct_state_access", wine_glProgramUniformMatrix4dvEXT },
  { "glProgramUniformMatrix4fv", "GL_ARB_separate_shader_objects", wine_glProgramUniformMatrix4fv },
  { "glProgramUniformMatrix4fvEXT", "GL_EXT_direct_state_access", wine_glProgramUniformMatrix4fvEXT },
  { "glProgramUniformMatrix4x2dv", "GL_ARB_separate_shader_objects", wine_glProgramUniformMatrix4x2dv },
  { "glProgramUniformMatrix4x2dvEXT", "GL_EXT_direct_state_access", wine_glProgramUniformMatrix4x2dvEXT },
  { "glProgramUniformMatrix4x2fv", "GL_ARB_separate_shader_objects", wine_glProgramUniformMatrix4x2fv },
  { "glProgramUniformMatrix4x2fvEXT", "GL_EXT_direct_state_access", wine_glProgramUniformMatrix4x2fvEXT },
  { "glProgramUniformMatrix4x3dv", "GL_ARB_separate_shader_objects", wine_glProgramUniformMatrix4x3dv },
  { "glProgramUniformMatrix4x3dvEXT", "GL_EXT_direct_state_access", wine_glProgramUniformMatrix4x3dvEXT },
  { "glProgramUniformMatrix4x3fv", "GL_ARB_separate_shader_objects", wine_glProgramUniformMatrix4x3fv },
  { "glProgramUniformMatrix4x3fvEXT", "GL_EXT_direct_state_access", wine_glProgramUniformMatrix4x3fvEXT },
  { "glProgramUniformui64NV", "GL_NV_shader_buffer_load", wine_glProgramUniformui64NV },
  { "glProgramUniformui64vNV", "GL_NV_shader_buffer_load", wine_glProgramUniformui64vNV },
  { "glProgramVertexLimitNV", "GL_NV_geometry_program4", wine_glProgramVertexLimitNV },
  { "glProvokingVertex", "GL_ARB_provoking_vertex", wine_glProvokingVertex },
  { "glProvokingVertexEXT", "GL_EXT_provoking_vertex", wine_glProvokingVertexEXT },
  { "glPushClientAttribDefaultEXT", "GL_EXT_direct_state_access", wine_glPushClientAttribDefaultEXT },
  { "glQueryCounter", "GL_ARB_timer_query", wine_glQueryCounter },
  { "glReadBufferRegion", "GL_KTX_buffer_region", wine_glReadBufferRegion },
  { "glReadInstrumentsSGIX", "GL_SGIX_instruments", wine_glReadInstrumentsSGIX },
  { "glReadnPixelsARB", "GL_ARB_robustness", wine_glReadnPixelsARB },
  { "glReferencePlaneSGIX", "GL_SGIX_reference_plane", wine_glReferencePlaneSGIX },
  { "glReleaseShaderCompiler", "GL_ARB_ES2_compatibility", wine_glReleaseShaderCompiler },
  { "glRenderbufferStorage", "GL_ARB_framebuffer_object", wine_glRenderbufferStorage },
  { "glRenderbufferStorageEXT", "GL_EXT_framebuffer_object", wine_glRenderbufferStorageEXT },
  { "glRenderbufferStorageMultisample", "GL_ARB_framebuffer_object", wine_glRenderbufferStorageMultisample },
  { "glRenderbufferStorageMultisampleCoverageNV", "GL_NV_framebuffer_multisample_coverage", wine_glRenderbufferStorageMultisampleCoverageNV },
  { "glRenderbufferStorageMultisampleEXT", "GL_EXT_framebuffer_multisample", wine_glRenderbufferStorageMultisampleEXT },
  { "glReplacementCodePointerSUN", "GL_SUN_triangle_list", wine_glReplacementCodePointerSUN },
  { "glReplacementCodeubSUN", "GL_SUN_triangle_list", wine_glReplacementCodeubSUN },
  { "glReplacementCodeubvSUN", "GL_SUN_triangle_list", wine_glReplacementCodeubvSUN },
  { "glReplacementCodeuiColor3fVertex3fSUN", "GL_SUN_vertex", wine_glReplacementCodeuiColor3fVertex3fSUN },
  { "glReplacementCodeuiColor3fVertex3fvSUN", "GL_SUN_vertex", wine_glReplacementCodeuiColor3fVertex3fvSUN },
  { "glReplacementCodeuiColor4fNormal3fVertex3fSUN", "GL_SUN_vertex", wine_glReplacementCodeuiColor4fNormal3fVertex3fSUN },
  { "glReplacementCodeuiColor4fNormal3fVertex3fvSUN", "GL_SUN_vertex", wine_glReplacementCodeuiColor4fNormal3fVertex3fvSUN },
  { "glReplacementCodeuiColor4ubVertex3fSUN", "GL_SUN_vertex", wine_glReplacementCodeuiColor4ubVertex3fSUN },
  { "glReplacementCodeuiColor4ubVertex3fvSUN", "GL_SUN_vertex", wine_glReplacementCodeuiColor4ubVertex3fvSUN },
  { "glReplacementCodeuiNormal3fVertex3fSUN", "GL_SUN_vertex", wine_glReplacementCodeuiNormal3fVertex3fSUN },
  { "glReplacementCodeuiNormal3fVertex3fvSUN", "GL_SUN_vertex", wine_glReplacementCodeuiNormal3fVertex3fvSUN },
  { "glReplacementCodeuiSUN", "GL_SUN_triangle_list", wine_glReplacementCodeuiSUN },
  { "glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN", "GL_SUN_vertex", wine_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN },
  { "glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN", "GL_SUN_vertex", wine_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN },
  { "glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN", "GL_SUN_vertex", wine_glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN },
  { "glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN", "GL_SUN_vertex", wine_glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN },
  { "glReplacementCodeuiTexCoord2fVertex3fSUN", "GL_SUN_vertex", wine_glReplacementCodeuiTexCoord2fVertex3fSUN },
  { "glReplacementCodeuiTexCoord2fVertex3fvSUN", "GL_SUN_vertex", wine_glReplacementCodeuiTexCoord2fVertex3fvSUN },
  { "glReplacementCodeuiVertex3fSUN", "GL_SUN_vertex", wine_glReplacementCodeuiVertex3fSUN },
  { "glReplacementCodeuiVertex3fvSUN", "GL_SUN_vertex", wine_glReplacementCodeuiVertex3fvSUN },
  { "glReplacementCodeuivSUN", "GL_SUN_triangle_list", wine_glReplacementCodeuivSUN },
  { "glReplacementCodeusSUN", "GL_SUN_triangle_list", wine_glReplacementCodeusSUN },
  { "glReplacementCodeusvSUN", "GL_SUN_triangle_list", wine_glReplacementCodeusvSUN },
  { "glRequestResidentProgramsNV", "GL_NV_vertex_program", wine_glRequestResidentProgramsNV },
  { "glResetHistogram", "GL_VERSION_1_2_DEPRECATED", wine_glResetHistogram },
  { "glResetHistogramEXT", "GL_EXT_histogram", wine_glResetHistogramEXT },
  { "glResetMinmax", "GL_VERSION_1_2_DEPRECATED", wine_glResetMinmax },
  { "glResetMinmaxEXT", "GL_EXT_histogram", wine_glResetMinmaxEXT },
  { "glResizeBuffersMESA", "GL_MESA_resize_buffers", wine_glResizeBuffersMESA },
  { "glResumeTransformFeedback", "GL_ARB_transform_feedback2", wine_glResumeTransformFeedback },
  { "glResumeTransformFeedbackNV", "GL_NV_transform_feedback2", wine_glResumeTransformFeedbackNV },
  { "glSampleCoverage", "GL_VERSION_1_3", wine_glSampleCoverage },
  { "glSampleCoverageARB", "GL_ARB_multisample", wine_glSampleCoverageARB },
  { "glSampleMapATI", "GL_ATI_fragment_shader", wine_glSampleMapATI },
  { "glSampleMaskEXT", "GL_EXT_multisample", wine_glSampleMaskEXT },
  { "glSampleMaskIndexedNV", "GL_NV_explicit_multisample", wine_glSampleMaskIndexedNV },
  { "glSampleMaskSGIS", "GL_SGIS_multisample", wine_glSampleMaskSGIS },
  { "glSampleMaski", "GL_ARB_texture_multisample", wine_glSampleMaski },
  { "glSamplePatternEXT", "GL_EXT_multisample", wine_glSamplePatternEXT },
  { "glSamplePatternSGIS", "GL_SGIS_multisample", wine_glSamplePatternSGIS },
  { "glSamplerParameterIiv", "GL_ARB_sampler_objects", wine_glSamplerParameterIiv },
  { "glSamplerParameterIuiv", "GL_ARB_sampler_objects", wine_glSamplerParameterIuiv },
  { "glSamplerParameterf", "GL_ARB_sampler_objects", wine_glSamplerParameterf },
  { "glSamplerParameterfv", "GL_ARB_sampler_objects", wine_glSamplerParameterfv },
  { "glSamplerParameteri", "GL_ARB_sampler_objects", wine_glSamplerParameteri },
  { "glSamplerParameteriv", "GL_ARB_sampler_objects", wine_glSamplerParameteriv },
  { "glScissorArrayv", "GL_ARB_viewport_array", wine_glScissorArrayv },
  { "glScissorIndexed", "GL_ARB_viewport_array", wine_glScissorIndexed },
  { "glScissorIndexedv", "GL_ARB_viewport_array", wine_glScissorIndexedv },
  { "glSecondaryColor3b", "GL_VERSION_1_4_DEPRECATED", wine_glSecondaryColor3b },
  { "glSecondaryColor3bEXT", "GL_EXT_secondary_color", wine_glSecondaryColor3bEXT },
  { "glSecondaryColor3bv", "GL_VERSION_1_4_DEPRECATED", wine_glSecondaryColor3bv },
  { "glSecondaryColor3bvEXT", "GL_EXT_secondary_color", wine_glSecondaryColor3bvEXT },
  { "glSecondaryColor3d", "GL_VERSION_1_4_DEPRECATED", wine_glSecondaryColor3d },
  { "glSecondaryColor3dEXT", "GL_EXT_secondary_color", wine_glSecondaryColor3dEXT },
  { "glSecondaryColor3dv", "GL_VERSION_1_4_DEPRECATED", wine_glSecondaryColor3dv },
  { "glSecondaryColor3dvEXT", "GL_EXT_secondary_color", wine_glSecondaryColor3dvEXT },
  { "glSecondaryColor3f", "GL_VERSION_1_4_DEPRECATED", wine_glSecondaryColor3f },
  { "glSecondaryColor3fEXT", "GL_EXT_secondary_color", wine_glSecondaryColor3fEXT },
  { "glSecondaryColor3fv", "GL_VERSION_1_4_DEPRECATED", wine_glSecondaryColor3fv },
  { "glSecondaryColor3fvEXT", "GL_EXT_secondary_color", wine_glSecondaryColor3fvEXT },
  { "glSecondaryColor3hNV", "GL_NV_half_float", wine_glSecondaryColor3hNV },
  { "glSecondaryColor3hvNV", "GL_NV_half_float", wine_glSecondaryColor3hvNV },
  { "glSecondaryColor3i", "GL_VERSION_1_4_DEPRECATED", wine_glSecondaryColor3i },
  { "glSecondaryColor3iEXT", "GL_EXT_secondary_color", wine_glSecondaryColor3iEXT },
  { "glSecondaryColor3iv", "GL_VERSION_1_4_DEPRECATED", wine_glSecondaryColor3iv },
  { "glSecondaryColor3ivEXT", "GL_EXT_secondary_color", wine_glSecondaryColor3ivEXT },
  { "glSecondaryColor3s", "GL_VERSION_1_4_DEPRECATED", wine_glSecondaryColor3s },
  { "glSecondaryColor3sEXT", "GL_EXT_secondary_color", wine_glSecondaryColor3sEXT },
  { "glSecondaryColor3sv", "GL_VERSION_1_4_DEPRECATED", wine_glSecondaryColor3sv },
  { "glSecondaryColor3svEXT", "GL_EXT_secondary_color", wine_glSecondaryColor3svEXT },
  { "glSecondaryColor3ub", "GL_VERSION_1_4_DEPRECATED", wine_glSecondaryColor3ub },
  { "glSecondaryColor3ubEXT", "GL_EXT_secondary_color", wine_glSecondaryColor3ubEXT },
  { "glSecondaryColor3ubv", "GL_VERSION_1_4_DEPRECATED", wine_glSecondaryColor3ubv },
  { "glSecondaryColor3ubvEXT", "GL_EXT_secondary_color", wine_glSecondaryColor3ubvEXT },
  { "glSecondaryColor3ui", "GL_VERSION_1_4_DEPRECATED", wine_glSecondaryColor3ui },
  { "glSecondaryColor3uiEXT", "GL_EXT_secondary_color", wine_glSecondaryColor3uiEXT },
  { "glSecondaryColor3uiv", "GL_VERSION_1_4_DEPRECATED", wine_glSecondaryColor3uiv },
  { "glSecondaryColor3uivEXT", "GL_EXT_secondary_color", wine_glSecondaryColor3uivEXT },
  { "glSecondaryColor3us", "GL_VERSION_1_4_DEPRECATED", wine_glSecondaryColor3us },
  { "glSecondaryColor3usEXT", "GL_EXT_secondary_color", wine_glSecondaryColor3usEXT },
  { "glSecondaryColor3usv", "GL_VERSION_1_4_DEPRECATED", wine_glSecondaryColor3usv },
  { "glSecondaryColor3usvEXT", "GL_EXT_secondary_color", wine_glSecondaryColor3usvEXT },
  { "glSecondaryColorFormatNV", "GL_NV_vertex_buffer_unified_memory", wine_glSecondaryColorFormatNV },
  { "glSecondaryColorP3ui", "GL_ARB_vertex_type_2_10_10_10_rev", wine_glSecondaryColorP3ui },
  { "glSecondaryColorP3uiv", "GL_ARB_vertex_type_2_10_10_10_rev", wine_glSecondaryColorP3uiv },
  { "glSecondaryColorPointer", "GL_VERSION_1_4_DEPRECATED", wine_glSecondaryColorPointer },
  { "glSecondaryColorPointerEXT", "GL_EXT_secondary_color", wine_glSecondaryColorPointerEXT },
  { "glSecondaryColorPointerListIBM", "GL_IBM_vertex_array_lists", wine_glSecondaryColorPointerListIBM },
  { "glSelectPerfMonitorCountersAMD", "GL_AMD_performance_monitor", wine_glSelectPerfMonitorCountersAMD },
  { "glSelectTextureCoordSetSGIS", "GL_SGIS_multitexture", wine_glSelectTextureCoordSetSGIS },
  { "glSelectTextureSGIS", "GL_SGIS_multitexture", wine_glSelectTextureSGIS },
  { "glSeparableFilter2D", "GL_VERSION_1_2_DEPRECATED", wine_glSeparableFilter2D },
  { "glSeparableFilter2DEXT", "GL_EXT_convolution", wine_glSeparableFilter2DEXT },
  { "glSetFenceAPPLE", "GL_APPLE_fence", wine_glSetFenceAPPLE },
  { "glSetFenceNV", "GL_NV_fence", wine_glSetFenceNV },
  { "glSetFragmentShaderConstantATI", "GL_ATI_fragment_shader", wine_glSetFragmentShaderConstantATI },
  { "glSetInvariantEXT", "GL_EXT_vertex_shader", wine_glSetInvariantEXT },
  { "glSetLocalConstantEXT", "GL_EXT_vertex_shader", wine_glSetLocalConstantEXT },
  { "glSetMultisamplefvAMD", "GL_AMD_sample_positions", wine_glSetMultisamplefvAMD },
  { "glShaderBinary", "GL_ARB_ES2_compatibility", wine_glShaderBinary },
  { "glShaderOp1EXT", "GL_EXT_vertex_shader", wine_glShaderOp1EXT },
  { "glShaderOp2EXT", "GL_EXT_vertex_shader", wine_glShaderOp2EXT },
  { "glShaderOp3EXT", "GL_EXT_vertex_shader", wine_glShaderOp3EXT },
  { "glShaderSource", "GL_VERSION_2_0", wine_glShaderSource },
  { "glShaderSourceARB", "GL_ARB_shader_objects", wine_glShaderSourceARB },
  { "glSharpenTexFuncSGIS", "GL_SGIS_sharpen_texture", wine_glSharpenTexFuncSGIS },
  { "glSpriteParameterfSGIX", "GL_SGIX_sprite", wine_glSpriteParameterfSGIX },
  { "glSpriteParameterfvSGIX", "GL_SGIX_sprite", wine_glSpriteParameterfvSGIX },
  { "glSpriteParameteriSGIX", "GL_SGIX_sprite", wine_glSpriteParameteriSGIX },
  { "glSpriteParameterivSGIX", "GL_SGIX_sprite", wine_glSpriteParameterivSGIX },
  { "glStartInstrumentsSGIX", "GL_SGIX_instruments", wine_glStartInstrumentsSGIX },
  { "glStencilClearTagEXT", "GL_EXT_stencil_clear_tag", wine_glStencilClearTagEXT },
  { "glStencilFillPathInstancedNV", "GL_NV_path_rendering", wine_glStencilFillPathInstancedNV },
  { "glStencilFillPathNV", "GL_NV_path_rendering", wine_glStencilFillPathNV },
  { "glStencilFuncSeparate", "GL_VERSION_2_0", wine_glStencilFuncSeparate },
  { "glStencilFuncSeparateATI", "GL_ATI_separate_stencil", wine_glStencilFuncSeparateATI },
  { "glStencilMaskSeparate", "GL_VERSION_2_0", wine_glStencilMaskSeparate },
  { "glStencilOpSeparate", "GL_VERSION_2_0", wine_glStencilOpSeparate },
  { "glStencilOpSeparateATI", "GL_ATI_separate_stencil", wine_glStencilOpSeparateATI },
  { "glStencilOpValueAMD", "GL_AMD_stencil_operation_extended", wine_glStencilOpValueAMD },
  { "glStencilStrokePathInstancedNV", "GL_NV_path_rendering", wine_glStencilStrokePathInstancedNV },
  { "glStencilStrokePathNV", "GL_NV_path_rendering", wine_glStencilStrokePathNV },
  { "glStopInstrumentsSGIX", "GL_SGIX_instruments", wine_glStopInstrumentsSGIX },
  { "glStringMarkerGREMEDY", "GL_GREMEDY_string_marker", wine_glStringMarkerGREMEDY },
  { "glSwizzleEXT", "GL_EXT_vertex_shader", wine_glSwizzleEXT },
  { "glTagSampleBufferSGIX", "GL_SGIX_tag_sample_buffer", wine_glTagSampleBufferSGIX },
  { "glTangent3bEXT", "GL_EXT_coordinate_frame", wine_glTangent3bEXT },
  { "glTangent3bvEXT", "GL_EXT_coordinate_frame", wine_glTangent3bvEXT },
  { "glTangent3dEXT", "GL_EXT_coordinate_frame", wine_glTangent3dEXT },
  { "glTangent3dvEXT", "GL_EXT_coordinate_frame", wine_glTangent3dvEXT },
  { "glTangent3fEXT", "GL_EXT_coordinate_frame", wine_glTangent3fEXT },
  { "glTangent3fvEXT", "GL_EXT_coordinate_frame", wine_glTangent3fvEXT },
  { "glTangent3iEXT", "GL_EXT_coordinate_frame", wine_glTangent3iEXT },
  { "glTangent3ivEXT", "GL_EXT_coordinate_frame", wine_glTangent3ivEXT },
  { "glTangent3sEXT", "GL_EXT_coordinate_frame", wine_glTangent3sEXT },
  { "glTangent3svEXT", "GL_EXT_coordinate_frame", wine_glTangent3svEXT },
  { "glTangentPointerEXT", "GL_EXT_coordinate_frame", wine_glTangentPointerEXT },
  { "glTbufferMask3DFX", "GL_3DFX_tbuffer", wine_glTbufferMask3DFX },
  { "glTessellationFactorAMD", "GL_AMD_vertex_shader_tesselator", wine_glTessellationFactorAMD },
  { "glTessellationModeAMD", "GL_AMD_vertex_shader_tesselator", wine_glTessellationModeAMD },
  { "glTestFenceAPPLE", "GL_APPLE_fence", wine_glTestFenceAPPLE },
  { "glTestFenceNV", "GL_NV_fence", wine_glTestFenceNV },
  { "glTestObjectAPPLE", "GL_APPLE_fence", wine_glTestObjectAPPLE },
  { "glTexBuffer", "GL_VERSION_3_1", wine_glTexBuffer },
  { "glTexBufferARB", "GL_ARB_texture_buffer_object", wine_glTexBufferARB },
  { "glTexBufferEXT", "GL_EXT_texture_buffer_object", wine_glTexBufferEXT },
  { "glTexBumpParameterfvATI", "GL_ATI_envmap_bumpmap", wine_glTexBumpParameterfvATI },
  { "glTexBumpParameterivATI", "GL_ATI_envmap_bumpmap", wine_glTexBumpParameterivATI },
  { "glTexCoord1hNV", "GL_NV_half_float", wine_glTexCoord1hNV },
  { "glTexCoord1hvNV", "GL_NV_half_float", wine_glTexCoord1hvNV },
  { "glTexCoord2fColor3fVertex3fSUN", "GL_SUN_vertex", wine_glTexCoord2fColor3fVertex3fSUN },
  { "glTexCoord2fColor3fVertex3fvSUN", "GL_SUN_vertex", wine_glTexCoord2fColor3fVertex3fvSUN },
  { "glTexCoord2fColor4fNormal3fVertex3fSUN", "GL_SUN_vertex", wine_glTexCoord2fColor4fNormal3fVertex3fSUN },
  { "glTexCoord2fColor4fNormal3fVertex3fvSUN", "GL_SUN_vertex", wine_glTexCoord2fColor4fNormal3fVertex3fvSUN },
  { "glTexCoord2fColor4ubVertex3fSUN", "GL_SUN_vertex", wine_glTexCoord2fColor4ubVertex3fSUN },
  { "glTexCoord2fColor4ubVertex3fvSUN", "GL_SUN_vertex", wine_glTexCoord2fColor4ubVertex3fvSUN },
  { "glTexCoord2fNormal3fVertex3fSUN", "GL_SUN_vertex", wine_glTexCoord2fNormal3fVertex3fSUN },
  { "glTexCoord2fNormal3fVertex3fvSUN", "GL_SUN_vertex", wine_glTexCoord2fNormal3fVertex3fvSUN },
  { "glTexCoord2fVertex3fSUN", "GL_SUN_vertex", wine_glTexCoord2fVertex3fSUN },
  { "glTexCoord2fVertex3fvSUN", "GL_SUN_vertex", wine_glTexCoord2fVertex3fvSUN },
  { "glTexCoord2hNV", "GL_NV_half_float", wine_glTexCoord2hNV },
  { "glTexCoord2hvNV", "GL_NV_half_float", wine_glTexCoord2hvNV },
  { "glTexCoord3hNV", "GL_NV_half_float", wine_glTexCoord3hNV },
  { "glTexCoord3hvNV", "GL_NV_half_float", wine_glTexCoord3hvNV },
  { "glTexCoord4fColor4fNormal3fVertex4fSUN", "GL_SUN_vertex", wine_glTexCoord4fColor4fNormal3fVertex4fSUN },
  { "glTexCoord4fColor4fNormal3fVertex4fvSUN", "GL_SUN_vertex", wine_glTexCoord4fColor4fNormal3fVertex4fvSUN },
  { "glTexCoord4fVertex4fSUN", "GL_SUN_vertex", wine_glTexCoord4fVertex4fSUN },
  { "glTexCoord4fVertex4fvSUN", "GL_SUN_vertex", wine_glTexCoord4fVertex4fvSUN },
  { "glTexCoord4hNV", "GL_NV_half_float", wine_glTexCoord4hNV },
  { "glTexCoord4hvNV", "GL_NV_half_float", wine_glTexCoord4hvNV },
  { "glTexCoordFormatNV", "GL_NV_vertex_buffer_unified_memory", wine_glTexCoordFormatNV },
  { "glTexCoordP1ui", "GL_ARB_vertex_type_2_10_10_10_rev", wine_glTexCoordP1ui },
  { "glTexCoordP1uiv", "GL_ARB_vertex_type_2_10_10_10_rev", wine_glTexCoordP1uiv },
  { "glTexCoordP2ui", "GL_ARB_vertex_type_2_10_10_10_rev", wine_glTexCoordP2ui },
  { "glTexCoordP2uiv", "GL_ARB_vertex_type_2_10_10_10_rev", wine_glTexCoordP2uiv },
  { "glTexCoordP3ui", "GL_ARB_vertex_type_2_10_10_10_rev", wine_glTexCoordP3ui },
  { "glTexCoordP3uiv", "GL_ARB_vertex_type_2_10_10_10_rev", wine_glTexCoordP3uiv },
  { "glTexCoordP4ui", "GL_ARB_vertex_type_2_10_10_10_rev", wine_glTexCoordP4ui },
  { "glTexCoordP4uiv", "GL_ARB_vertex_type_2_10_10_10_rev", wine_glTexCoordP4uiv },
  { "glTexCoordPointerEXT", "GL_EXT_vertex_array", wine_glTexCoordPointerEXT },
  { "glTexCoordPointerListIBM", "GL_IBM_vertex_array_lists", wine_glTexCoordPointerListIBM },
  { "glTexCoordPointervINTEL", "GL_INTEL_parallel_arrays", wine_glTexCoordPointervINTEL },
  { "glTexFilterFuncSGIS", "GL_SGIS_texture_filter4", wine_glTexFilterFuncSGIS },
  { "glTexImage2DMultisample", "GL_ARB_texture_multisample", wine_glTexImage2DMultisample },
  { "glTexImage2DMultisampleCoverageNV", "GL_NV_texture_multisample", wine_glTexImage2DMultisampleCoverageNV },
  { "glTexImage3D", "GL_VERSION_1_2", wine_glTexImage3D },
  { "glTexImage3DEXT", "GL_EXT_texture3D", wine_glTexImage3DEXT },
  { "glTexImage3DMultisample", "GL_ARB_texture_multisample", wine_glTexImage3DMultisample },
  { "glTexImage3DMultisampleCoverageNV", "GL_NV_texture_multisample", wine_glTexImage3DMultisampleCoverageNV },
  { "glTexImage4DSGIS", "GL_SGIS_texture4D", wine_glTexImage4DSGIS },
  { "glTexParameterIiv", "GL_VERSION_3_0", wine_glTexParameterIiv },
  { "glTexParameterIivEXT", "GL_EXT_texture_integer", wine_glTexParameterIivEXT },
  { "glTexParameterIuiv", "GL_VERSION_3_0", wine_glTexParameterIuiv },
  { "glTexParameterIuivEXT", "GL_EXT_texture_integer", wine_glTexParameterIuivEXT },
  { "glTexRenderbufferNV", "GL_NV_explicit_multisample", wine_glTexRenderbufferNV },
  { "glTexStorage1D", "GL_ARB_texture_storage", wine_glTexStorage1D },
  { "glTexStorage2D", "GL_ARB_texture_storage", wine_glTexStorage2D },
  { "glTexStorage3D", "GL_ARB_texture_storage", wine_glTexStorage3D },
  { "glTexSubImage1DEXT", "GL_EXT_subtexture", wine_glTexSubImage1DEXT },
  { "glTexSubImage2DEXT", "GL_EXT_subtexture", wine_glTexSubImage2DEXT },
  { "glTexSubImage3D", "GL_VERSION_1_2", wine_glTexSubImage3D },
  { "glTexSubImage3DEXT", "GL_EXT_texture3D", wine_glTexSubImage3DEXT },
  { "glTexSubImage4DSGIS", "GL_SGIS_texture4D", wine_glTexSubImage4DSGIS },
  { "glTextureBarrierNV", "GL_NV_texture_barrier", wine_glTextureBarrierNV },
  { "glTextureBufferEXT", "GL_EXT_direct_state_access", wine_glTextureBufferEXT },
  { "glTextureColorMaskSGIS", "GL_SGIS_texture_color_mask", wine_glTextureColorMaskSGIS },
  { "glTextureImage1DEXT", "GL_EXT_direct_state_access", wine_glTextureImage1DEXT },
  { "glTextureImage2DEXT", "GL_EXT_direct_state_access", wine_glTextureImage2DEXT },
  { "glTextureImage2DMultisampleCoverageNV", "GL_NV_texture_multisample", wine_glTextureImage2DMultisampleCoverageNV },
  { "glTextureImage2DMultisampleNV", "GL_NV_texture_multisample", wine_glTextureImage2DMultisampleNV },
  { "glTextureImage3DEXT", "GL_EXT_direct_state_access", wine_glTextureImage3DEXT },
  { "glTextureImage3DMultisampleCoverageNV", "GL_NV_texture_multisample", wine_glTextureImage3DMultisampleCoverageNV },
  { "glTextureImage3DMultisampleNV", "GL_NV_texture_multisample", wine_glTextureImage3DMultisampleNV },
  { "glTextureLightEXT", "GL_EXT_light_texture", wine_glTextureLightEXT },
  { "glTextureMaterialEXT", "GL_EXT_light_texture", wine_glTextureMaterialEXT },
  { "glTextureNormalEXT", "GL_EXT_texture_perturb_normal", wine_glTextureNormalEXT },
  { "glTextureParameterIivEXT", "GL_EXT_direct_state_access", wine_glTextureParameterIivEXT },
  { "glTextureParameterIuivEXT", "GL_EXT_direct_state_access", wine_glTextureParameterIuivEXT },
  { "glTextureParameterfEXT", "GL_EXT_direct_state_access", wine_glTextureParameterfEXT },
  { "glTextureParameterfvEXT", "GL_EXT_direct_state_access", wine_glTextureParameterfvEXT },
  { "glTextureParameteriEXT", "GL_EXT_direct_state_access", wine_glTextureParameteriEXT },
  { "glTextureParameterivEXT", "GL_EXT_direct_state_access", wine_glTextureParameterivEXT },
  { "glTextureRangeAPPLE", "GL_APPLE_texture_range", wine_glTextureRangeAPPLE },
  { "glTextureRenderbufferEXT", "GL_EXT_direct_state_access", wine_glTextureRenderbufferEXT },
  { "glTextureStorage1DEXT", "GL_ARB_texture_storage", wine_glTextureStorage1DEXT },
  { "glTextureStorage2DEXT", "GL_ARB_texture_storage", wine_glTextureStorage2DEXT },
  { "glTextureStorage3DEXT", "GL_ARB_texture_storage", wine_glTextureStorage3DEXT },
  { "glTextureSubImage1DEXT", "GL_EXT_direct_state_access", wine_glTextureSubImage1DEXT },
  { "glTextureSubImage2DEXT", "GL_EXT_direct_state_access", wine_glTextureSubImage2DEXT },
  { "glTextureSubImage3DEXT", "GL_EXT_direct_state_access", wine_glTextureSubImage3DEXT },
  { "glTrackMatrixNV", "GL_NV_vertex_program", wine_glTrackMatrixNV },
  { "glTransformFeedbackAttribsNV", "GL_NV_transform_feedback", wine_glTransformFeedbackAttribsNV },
  { "glTransformFeedbackStreamAttribsNV", "GL_NV_transform_feedback", wine_glTransformFeedbackStreamAttribsNV },
  { "glTransformFeedbackVaryings", "GL_VERSION_3_0", wine_glTransformFeedbackVaryings },
  { "glTransformFeedbackVaryingsEXT", "GL_EXT_transform_feedback", wine_glTransformFeedbackVaryingsEXT },
  { "glTransformFeedbackVaryingsNV", "GL_NV_transform_feedback", wine_glTransformFeedbackVaryingsNV },
  { "glTransformPathNV", "GL_NV_path_rendering", wine_glTransformPathNV },
  { "glUniform1d", "GL_ARB_gpu_shader_fp64", wine_glUniform1d },
  { "glUniform1dv", "GL_ARB_gpu_shader_fp64", wine_glUniform1dv },
  { "glUniform1f", "GL_VERSION_2_0", wine_glUniform1f },
  { "glUniform1fARB", "GL_ARB_shader_objects", wine_glUniform1fARB },
  { "glUniform1fv", "GL_VERSION_2_0", wine_glUniform1fv },
  { "glUniform1fvARB", "GL_ARB_shader_objects", wine_glUniform1fvARB },
  { "glUniform1i", "GL_VERSION_2_0", wine_glUniform1i },
  { "glUniform1i64NV", "GL_NV_gpu_shader5", wine_glUniform1i64NV },
  { "glUniform1i64vNV", "GL_NV_gpu_shader5", wine_glUniform1i64vNV },
  { "glUniform1iARB", "GL_ARB_shader_objects", wine_glUniform1iARB },
  { "glUniform1iv", "GL_VERSION_2_0", wine_glUniform1iv },
  { "glUniform1ivARB", "GL_ARB_shader_objects", wine_glUniform1ivARB },
  { "glUniform1ui", "GL_VERSION_3_0", wine_glUniform1ui },
  { "glUniform1ui64NV", "GL_NV_gpu_shader5", wine_glUniform1ui64NV },
  { "glUniform1ui64vNV", "GL_NV_gpu_shader5", wine_glUniform1ui64vNV },
  { "glUniform1uiEXT", "GL_EXT_gpu_shader4", wine_glUniform1uiEXT },
  { "glUniform1uiv", "GL_VERSION_3_0", wine_glUniform1uiv },
  { "glUniform1uivEXT", "GL_EXT_gpu_shader4", wine_glUniform1uivEXT },
  { "glUniform2d", "GL_ARB_gpu_shader_fp64", wine_glUniform2d },
  { "glUniform2dv", "GL_ARB_gpu_shader_fp64", wine_glUniform2dv },
  { "glUniform2f", "GL_VERSION_2_0", wine_glUniform2f },
  { "glUniform2fARB", "GL_ARB_shader_objects", wine_glUniform2fARB },
  { "glUniform2fv", "GL_VERSION_2_0", wine_glUniform2fv },
  { "glUniform2fvARB", "GL_ARB_shader_objects", wine_glUniform2fvARB },
  { "glUniform2i", "GL_VERSION_2_0", wine_glUniform2i },
  { "glUniform2i64NV", "GL_NV_gpu_shader5", wine_glUniform2i64NV },
  { "glUniform2i64vNV", "GL_NV_gpu_shader5", wine_glUniform2i64vNV },
  { "glUniform2iARB", "GL_ARB_shader_objects", wine_glUniform2iARB },
  { "glUniform2iv", "GL_VERSION_2_0", wine_glUniform2iv },
  { "glUniform2ivARB", "GL_ARB_shader_objects", wine_glUniform2ivARB },
  { "glUniform2ui", "GL_VERSION_3_0", wine_glUniform2ui },
  { "glUniform2ui64NV", "GL_NV_gpu_shader5", wine_glUniform2ui64NV },
  { "glUniform2ui64vNV", "GL_NV_gpu_shader5", wine_glUniform2ui64vNV },
  { "glUniform2uiEXT", "GL_EXT_gpu_shader4", wine_glUniform2uiEXT },
  { "glUniform2uiv", "GL_VERSION_3_0", wine_glUniform2uiv },
  { "glUniform2uivEXT", "GL_EXT_gpu_shader4", wine_glUniform2uivEXT },
  { "glUniform3d", "GL_ARB_gpu_shader_fp64", wine_glUniform3d },
  { "glUniform3dv", "GL_ARB_gpu_shader_fp64", wine_glUniform3dv },
  { "glUniform3f", "GL_VERSION_2_0", wine_glUniform3f },
  { "glUniform3fARB", "GL_ARB_shader_objects", wine_glUniform3fARB },
  { "glUniform3fv", "GL_VERSION_2_0", wine_glUniform3fv },
  { "glUniform3fvARB", "GL_ARB_shader_objects", wine_glUniform3fvARB },
  { "glUniform3i", "GL_VERSION_2_0", wine_glUniform3i },
  { "glUniform3i64NV", "GL_NV_gpu_shader5", wine_glUniform3i64NV },
  { "glUniform3i64vNV", "GL_NV_gpu_shader5", wine_glUniform3i64vNV },
  { "glUniform3iARB", "GL_ARB_shader_objects", wine_glUniform3iARB },
  { "glUniform3iv", "GL_VERSION_2_0", wine_glUniform3iv },
  { "glUniform3ivARB", "GL_ARB_shader_objects", wine_glUniform3ivARB },
  { "glUniform3ui", "GL_VERSION_3_0", wine_glUniform3ui },
  { "glUniform3ui64NV", "GL_NV_gpu_shader5", wine_glUniform3ui64NV },
  { "glUniform3ui64vNV", "GL_NV_gpu_shader5", wine_glUniform3ui64vNV },
  { "glUniform3uiEXT", "GL_EXT_gpu_shader4", wine_glUniform3uiEXT },
  { "glUniform3uiv", "GL_VERSION_3_0", wine_glUniform3uiv },
  { "glUniform3uivEXT", "GL_EXT_gpu_shader4", wine_glUniform3uivEXT },
  { "glUniform4d", "GL_ARB_gpu_shader_fp64", wine_glUniform4d },
  { "glUniform4dv", "GL_ARB_gpu_shader_fp64", wine_glUniform4dv },
  { "glUniform4f", "GL_VERSION_2_0", wine_glUniform4f },
  { "glUniform4fARB", "GL_ARB_shader_objects", wine_glUniform4fARB },
  { "glUniform4fv", "GL_VERSION_2_0", wine_glUniform4fv },
  { "glUniform4fvARB", "GL_ARB_shader_objects", wine_glUniform4fvARB },
  { "glUniform4i", "GL_VERSION_2_0", wine_glUniform4i },
  { "glUniform4i64NV", "GL_NV_gpu_shader5", wine_glUniform4i64NV },
  { "glUniform4i64vNV", "GL_NV_gpu_shader5", wine_glUniform4i64vNV },
  { "glUniform4iARB", "GL_ARB_shader_objects", wine_glUniform4iARB },
  { "glUniform4iv", "GL_VERSION_2_0", wine_glUniform4iv },
  { "glUniform4ivARB", "GL_ARB_shader_objects", wine_glUniform4ivARB },
  { "glUniform4ui", "GL_VERSION_3_0", wine_glUniform4ui },
  { "glUniform4ui64NV", "GL_NV_gpu_shader5", wine_glUniform4ui64NV },
  { "glUniform4ui64vNV", "GL_NV_gpu_shader5", wine_glUniform4ui64vNV },
  { "glUniform4uiEXT", "GL_EXT_gpu_shader4", wine_glUniform4uiEXT },
  { "glUniform4uiv", "GL_VERSION_3_0", wine_glUniform4uiv },
  { "glUniform4uivEXT", "GL_EXT_gpu_shader4", wine_glUniform4uivEXT },
  { "glUniformBlockBinding", "GL_ARB_uniform_buffer_object", wine_glUniformBlockBinding },
  { "glUniformBufferEXT", "GL_EXT_bindable_uniform", wine_glUniformBufferEXT },
  { "glUniformHandleui64NV", "GL_NV_bindless_texture", wine_glUniformHandleui64NV },
  { "glUniformHandleui64vNV", "GL_NV_bindless_texture", wine_glUniformHandleui64vNV },
  { "glUniformMatrix2dv", "GL_ARB_gpu_shader_fp64", wine_glUniformMatrix2dv },
  { "glUniformMatrix2fv", "GL_VERSION_2_0", wine_glUniformMatrix2fv },
  { "glUniformMatrix2fvARB", "GL_ARB_shader_objects", wine_glUniformMatrix2fvARB },
  { "glUniformMatrix2x3dv", "GL_ARB_gpu_shader_fp64", wine_glUniformMatrix2x3dv },
  { "glUniformMatrix2x3fv", "GL_VERSION_2_1", wine_glUniformMatrix2x3fv },
  { "glUniformMatrix2x4dv", "GL_ARB_gpu_shader_fp64", wine_glUniformMatrix2x4dv },
  { "glUniformMatrix2x4fv", "GL_VERSION_2_1", wine_glUniformMatrix2x4fv },
  { "glUniformMatrix3dv", "GL_ARB_gpu_shader_fp64", wine_glUniformMatrix3dv },
  { "glUniformMatrix3fv", "GL_VERSION_2_0", wine_glUniformMatrix3fv },
  { "glUniformMatrix3fvARB", "GL_ARB_shader_objects", wine_glUniformMatrix3fvARB },
  { "glUniformMatrix3x2dv", "GL_ARB_gpu_shader_fp64", wine_glUniformMatrix3x2dv },
  { "glUniformMatrix3x2fv", "GL_VERSION_2_1", wine_glUniformMatrix3x2fv },
  { "glUniformMatrix3x4dv", "GL_ARB_gpu_shader_fp64", wine_glUniformMatrix3x4dv },
  { "glUniformMatrix3x4fv", "GL_VERSION_2_1", wine_glUniformMatrix3x4fv },
  { "glUniformMatrix4dv", "GL_ARB_gpu_shader_fp64", wine_glUniformMatrix4dv },
  { "glUniformMatrix4fv", "GL_VERSION_2_0", wine_glUniformMatrix4fv },
  { "glUniformMatrix4fvARB", "GL_ARB_shader_objects", wine_glUniformMatrix4fvARB },
  { "glUniformMatrix4x2dv", "GL_ARB_gpu_shader_fp64", wine_glUniformMatrix4x2dv },
  { "glUniformMatrix4x2fv", "GL_VERSION_2_1", wine_glUniformMatrix4x2fv },
  { "glUniformMatrix4x3dv", "GL_ARB_gpu_shader_fp64", wine_glUniformMatrix4x3dv },
  { "glUniformMatrix4x3fv", "GL_VERSION_2_1", wine_glUniformMatrix4x3fv },
  { "glUniformSubroutinesuiv", "GL_ARB_shader_subroutine", wine_glUniformSubroutinesuiv },
  { "glUniformui64NV", "GL_NV_shader_buffer_load", wine_glUniformui64NV },
  { "glUniformui64vNV", "GL_NV_shader_buffer_load", wine_glUniformui64vNV },
  { "glUnlockArraysEXT", "GL_EXT_compiled_vertex_array", wine_glUnlockArraysEXT },
  { "glUnmapBuffer", "GL_VERSION_1_5", wine_glUnmapBuffer },
  { "glUnmapBufferARB", "GL_ARB_vertex_buffer_object", wine_glUnmapBufferARB },
  { "glUnmapNamedBufferEXT", "GL_EXT_direct_state_access", wine_glUnmapNamedBufferEXT },
  { "glUnmapObjectBufferATI", "GL_ATI_map_object_buffer", wine_glUnmapObjectBufferATI },
  { "glUpdateObjectBufferATI", "GL_ATI_vertex_array_object", wine_glUpdateObjectBufferATI },
  { "glUseProgram", "GL_VERSION_2_0", wine_glUseProgram },
  { "glUseProgramObjectARB", "GL_ARB_shader_objects", wine_glUseProgramObjectARB },
  { "glUseProgramStages", "GL_ARB_separate_shader_objects", wine_glUseProgramStages },
  { "glUseShaderProgramEXT", "GL_EXT_separate_shader_objects", wine_glUseShaderProgramEXT },
  { "glVDPAUFiniNV", "GL_NV_vdpau_interop", wine_glVDPAUFiniNV },
  { "glVDPAUGetSurfaceivNV", "GL_NV_vdpau_interop", wine_glVDPAUGetSurfaceivNV },
  { "glVDPAUInitNV", "GL_NV_vdpau_interop", wine_glVDPAUInitNV },
  { "glVDPAUIsSurfaceNV", "GL_NV_vdpau_interop", wine_glVDPAUIsSurfaceNV },
  { "glVDPAUMapSurfacesNV", "GL_NV_vdpau_interop", wine_glVDPAUMapSurfacesNV },
  { "glVDPAURegisterOutputSurfaceNV", "GL_NV_vdpau_interop", wine_glVDPAURegisterOutputSurfaceNV },
  { "glVDPAURegisterVideoSurfaceNV", "GL_NV_vdpau_interop", wine_glVDPAURegisterVideoSurfaceNV },
  { "glVDPAUSurfaceAccessNV", "GL_NV_vdpau_interop", wine_glVDPAUSurfaceAccessNV },
  { "glVDPAUUnmapSurfacesNV", "GL_NV_vdpau_interop", wine_glVDPAUUnmapSurfacesNV },
  { "glVDPAUUnregisterSurfaceNV", "GL_NV_vdpau_interop", wine_glVDPAUUnregisterSurfaceNV },
  { "glValidateProgram", "GL_VERSION_2_0", wine_glValidateProgram },
  { "glValidateProgramARB", "GL_ARB_shader_objects", wine_glValidateProgramARB },
  { "glValidateProgramPipeline", "GL_ARB_separate_shader_objects", wine_glValidateProgramPipeline },
  { "glVariantArrayObjectATI", "GL_ATI_vertex_array_object", wine_glVariantArrayObjectATI },
  { "glVariantPointerEXT", "GL_EXT_vertex_shader", wine_glVariantPointerEXT },
  { "glVariantbvEXT", "GL_EXT_vertex_shader", wine_glVariantbvEXT },
  { "glVariantdvEXT", "GL_EXT_vertex_shader", wine_glVariantdvEXT },
  { "glVariantfvEXT", "GL_EXT_vertex_shader", wine_glVariantfvEXT },
  { "glVariantivEXT", "GL_EXT_vertex_shader", wine_glVariantivEXT },
  { "glVariantsvEXT", "GL_EXT_vertex_shader", wine_glVariantsvEXT },
  { "glVariantubvEXT", "GL_EXT_vertex_shader", wine_glVariantubvEXT },
  { "glVariantuivEXT", "GL_EXT_vertex_shader", wine_glVariantuivEXT },
  { "glVariantusvEXT", "GL_EXT_vertex_shader", wine_glVariantusvEXT },
  { "glVertex2hNV", "GL_NV_half_float", wine_glVertex2hNV },
  { "glVertex2hvNV", "GL_NV_half_float", wine_glVertex2hvNV },
  { "glVertex3hNV", "GL_NV_half_float", wine_glVertex3hNV },
  { "glVertex3hvNV", "GL_NV_half_float", wine_glVertex3hvNV },
  { "glVertex4hNV", "GL_NV_half_float", wine_glVertex4hNV },
  { "glVertex4hvNV", "GL_NV_half_float", wine_glVertex4hvNV },
  { "glVertexArrayParameteriAPPLE", "GL_APPLE_vertex_array_range", wine_glVertexArrayParameteriAPPLE },
  { "glVertexArrayRangeAPPLE", "GL_APPLE_vertex_array_range", wine_glVertexArrayRangeAPPLE },
  { "glVertexArrayRangeNV", "GL_NV_vertex_array_range", wine_glVertexArrayRangeNV },
  { "glVertexArrayVertexAttribLOffsetEXT", "GL_EXT_vertex_attrib_64bit", wine_glVertexArrayVertexAttribLOffsetEXT },
  { "glVertexAttrib1d", "GL_VERSION_2_0", wine_glVertexAttrib1d },
  { "glVertexAttrib1dARB", "GL_ARB_vertex_program", wine_glVertexAttrib1dARB },
  { "glVertexAttrib1dNV", "GL_NV_vertex_program", wine_glVertexAttrib1dNV },
  { "glVertexAttrib1dv", "GL_VERSION_2_0", wine_glVertexAttrib1dv },
  { "glVertexAttrib1dvARB", "GL_ARB_vertex_program", wine_glVertexAttrib1dvARB },
  { "glVertexAttrib1dvNV", "GL_NV_vertex_program", wine_glVertexAttrib1dvNV },
  { "glVertexAttrib1f", "GL_VERSION_2_0", wine_glVertexAttrib1f },
  { "glVertexAttrib1fARB", "GL_ARB_vertex_program", wine_glVertexAttrib1fARB },
  { "glVertexAttrib1fNV", "GL_NV_vertex_program", wine_glVertexAttrib1fNV },
  { "glVertexAttrib1fv", "GL_VERSION_2_0", wine_glVertexAttrib1fv },
  { "glVertexAttrib1fvARB", "GL_ARB_vertex_program", wine_glVertexAttrib1fvARB },
  { "glVertexAttrib1fvNV", "GL_NV_vertex_program", wine_glVertexAttrib1fvNV },
  { "glVertexAttrib1hNV", "GL_NV_half_float", wine_glVertexAttrib1hNV },
  { "glVertexAttrib1hvNV", "GL_NV_half_float", wine_glVertexAttrib1hvNV },
  { "glVertexAttrib1s", "GL_VERSION_2_0", wine_glVertexAttrib1s },
  { "glVertexAttrib1sARB", "GL_ARB_vertex_program", wine_glVertexAttrib1sARB },
  { "glVertexAttrib1sNV", "GL_NV_vertex_program", wine_glVertexAttrib1sNV },
  { "glVertexAttrib1sv", "GL_VERSION_2_0", wine_glVertexAttrib1sv },
  { "glVertexAttrib1svARB", "GL_ARB_vertex_program", wine_glVertexAttrib1svARB },
  { "glVertexAttrib1svNV", "GL_NV_vertex_program", wine_glVertexAttrib1svNV },
  { "glVertexAttrib2d", "GL_VERSION_2_0", wine_glVertexAttrib2d },
  { "glVertexAttrib2dARB", "GL_ARB_vertex_program", wine_glVertexAttrib2dARB },
  { "glVertexAttrib2dNV", "GL_NV_vertex_program", wine_glVertexAttrib2dNV },
  { "glVertexAttrib2dv", "GL_VERSION_2_0", wine_glVertexAttrib2dv },
  { "glVertexAttrib2dvARB", "GL_ARB_vertex_program", wine_glVertexAttrib2dvARB },
  { "glVertexAttrib2dvNV", "GL_NV_vertex_program", wine_glVertexAttrib2dvNV },
  { "glVertexAttrib2f", "GL_VERSION_2_0", wine_glVertexAttrib2f },
  { "glVertexAttrib2fARB", "GL_ARB_vertex_program", wine_glVertexAttrib2fARB },
  { "glVertexAttrib2fNV", "GL_NV_vertex_program", wine_glVertexAttrib2fNV },
  { "glVertexAttrib2fv", "GL_VERSION_2_0", wine_glVertexAttrib2fv },
  { "glVertexAttrib2fvARB", "GL_ARB_vertex_program", wine_glVertexAttrib2fvARB },
  { "glVertexAttrib2fvNV", "GL_NV_vertex_program", wine_glVertexAttrib2fvNV },
  { "glVertexAttrib2hNV", "GL_NV_half_float", wine_glVertexAttrib2hNV },
  { "glVertexAttrib2hvNV", "GL_NV_half_float", wine_glVertexAttrib2hvNV },
  { "glVertexAttrib2s", "GL_VERSION_2_0", wine_glVertexAttrib2s },
  { "glVertexAttrib2sARB", "GL_ARB_vertex_program", wine_glVertexAttrib2sARB },
  { "glVertexAttrib2sNV", "GL_NV_vertex_program", wine_glVertexAttrib2sNV },
  { "glVertexAttrib2sv", "GL_VERSION_2_0", wine_glVertexAttrib2sv },
  { "glVertexAttrib2svARB", "GL_ARB_vertex_program", wine_glVertexAttrib2svARB },
  { "glVertexAttrib2svNV", "GL_NV_vertex_program", wine_glVertexAttrib2svNV },
  { "glVertexAttrib3d", "GL_VERSION_2_0", wine_glVertexAttrib3d },
  { "glVertexAttrib3dARB", "GL_ARB_vertex_program", wine_glVertexAttrib3dARB },
  { "glVertexAttrib3dNV", "GL_NV_vertex_program", wine_glVertexAttrib3dNV },
  { "glVertexAttrib3dv", "GL_VERSION_2_0", wine_glVertexAttrib3dv },
  { "glVertexAttrib3dvARB", "GL_ARB_vertex_program", wine_glVertexAttrib3dvARB },
  { "glVertexAttrib3dvNV", "GL_NV_vertex_program", wine_glVertexAttrib3dvNV },
  { "glVertexAttrib3f", "GL_VERSION_2_0", wine_glVertexAttrib3f },
  { "glVertexAttrib3fARB", "GL_ARB_vertex_program", wine_glVertexAttrib3fARB },
  { "glVertexAttrib3fNV", "GL_NV_vertex_program", wine_glVertexAttrib3fNV },
  { "glVertexAttrib3fv", "GL_VERSION_2_0", wine_glVertexAttrib3fv },
  { "glVertexAttrib3fvARB", "GL_ARB_vertex_program", wine_glVertexAttrib3fvARB },
  { "glVertexAttrib3fvNV", "GL_NV_vertex_program", wine_glVertexAttrib3fvNV },
  { "glVertexAttrib3hNV", "GL_NV_half_float", wine_glVertexAttrib3hNV },
  { "glVertexAttrib3hvNV", "GL_NV_half_float", wine_glVertexAttrib3hvNV },
  { "glVertexAttrib3s", "GL_VERSION_2_0", wine_glVertexAttrib3s },
  { "glVertexAttrib3sARB", "GL_ARB_vertex_program", wine_glVertexAttrib3sARB },
  { "glVertexAttrib3sNV", "GL_NV_vertex_program", wine_glVertexAttrib3sNV },
  { "glVertexAttrib3sv", "GL_VERSION_2_0", wine_glVertexAttrib3sv },
  { "glVertexAttrib3svARB", "GL_ARB_vertex_program", wine_glVertexAttrib3svARB },
  { "glVertexAttrib3svNV", "GL_NV_vertex_program", wine_glVertexAttrib3svNV },
  { "glVertexAttrib4Nbv", "GL_VERSION_2_0", wine_glVertexAttrib4Nbv },
  { "glVertexAttrib4NbvARB", "GL_ARB_vertex_program", wine_glVertexAttrib4NbvARB },
  { "glVertexAttrib4Niv", "GL_VERSION_2_0", wine_glVertexAttrib4Niv },
  { "glVertexAttrib4NivARB", "GL_ARB_vertex_program", wine_glVertexAttrib4NivARB },
  { "glVertexAttrib4Nsv", "GL_VERSION_2_0", wine_glVertexAttrib4Nsv },
  { "glVertexAttrib4NsvARB", "GL_ARB_vertex_program", wine_glVertexAttrib4NsvARB },
  { "glVertexAttrib4Nub", "GL_VERSION_2_0", wine_glVertexAttrib4Nub },
  { "glVertexAttrib4NubARB", "GL_ARB_vertex_program", wine_glVertexAttrib4NubARB },
  { "glVertexAttrib4Nubv", "GL_VERSION_2_0", wine_glVertexAttrib4Nubv },
  { "glVertexAttrib4NubvARB", "GL_ARB_vertex_program", wine_glVertexAttrib4NubvARB },
  { "glVertexAttrib4Nuiv", "GL_VERSION_2_0", wine_glVertexAttrib4Nuiv },
  { "glVertexAttrib4NuivARB", "GL_ARB_vertex_program", wine_glVertexAttrib4NuivARB },
  { "glVertexAttrib4Nusv", "GL_VERSION_2_0", wine_glVertexAttrib4Nusv },
  { "glVertexAttrib4NusvARB", "GL_ARB_vertex_program", wine_glVertexAttrib4NusvARB },
  { "glVertexAttrib4bv", "GL_VERSION_2_0", wine_glVertexAttrib4bv },
  { "glVertexAttrib4bvARB", "GL_ARB_vertex_program", wine_glVertexAttrib4bvARB },
  { "glVertexAttrib4d", "GL_VERSION_2_0", wine_glVertexAttrib4d },
  { "glVertexAttrib4dARB", "GL_ARB_vertex_program", wine_glVertexAttrib4dARB },
  { "glVertexAttrib4dNV", "GL_NV_vertex_program", wine_glVertexAttrib4dNV },
  { "glVertexAttrib4dv", "GL_VERSION_2_0", wine_glVertexAttrib4dv },
  { "glVertexAttrib4dvARB", "GL_ARB_vertex_program", wine_glVertexAttrib4dvARB },
  { "glVertexAttrib4dvNV", "GL_NV_vertex_program", wine_glVertexAttrib4dvNV },
  { "glVertexAttrib4f", "GL_VERSION_2_0", wine_glVertexAttrib4f },
  { "glVertexAttrib4fARB", "GL_ARB_vertex_program", wine_glVertexAttrib4fARB },
  { "glVertexAttrib4fNV", "GL_NV_vertex_program", wine_glVertexAttrib4fNV },
  { "glVertexAttrib4fv", "GL_VERSION_2_0", wine_glVertexAttrib4fv },
  { "glVertexAttrib4fvARB", "GL_ARB_vertex_program", wine_glVertexAttrib4fvARB },
  { "glVertexAttrib4fvNV", "GL_NV_vertex_program", wine_glVertexAttrib4fvNV },
  { "glVertexAttrib4hNV", "GL_NV_half_float", wine_glVertexAttrib4hNV },
  { "glVertexAttrib4hvNV", "GL_NV_half_float", wine_glVertexAttrib4hvNV },
  { "glVertexAttrib4iv", "GL_VERSION_2_0", wine_glVertexAttrib4iv },
  { "glVertexAttrib4ivARB", "GL_ARB_vertex_program", wine_glVertexAttrib4ivARB },
  { "glVertexAttrib4s", "GL_VERSION_2_0", wine_glVertexAttrib4s },
  { "glVertexAttrib4sARB", "GL_ARB_vertex_program", wine_glVertexAttrib4sARB },
  { "glVertexAttrib4sNV", "GL_NV_vertex_program", wine_glVertexAttrib4sNV },
  { "glVertexAttrib4sv", "GL_VERSION_2_0", wine_glVertexAttrib4sv },
  { "glVertexAttrib4svARB", "GL_ARB_vertex_program", wine_glVertexAttrib4svARB },
  { "glVertexAttrib4svNV", "GL_NV_vertex_program", wine_glVertexAttrib4svNV },
  { "glVertexAttrib4ubNV", "GL_NV_vertex_program", wine_glVertexAttrib4ubNV },
  { "glVertexAttrib4ubv", "GL_VERSION_2_0", wine_glVertexAttrib4ubv },
  { "glVertexAttrib4ubvARB", "GL_ARB_vertex_program", wine_glVertexAttrib4ubvARB },
  { "glVertexAttrib4ubvNV", "GL_NV_vertex_program", wine_glVertexAttrib4ubvNV },
  { "glVertexAttrib4uiv", "GL_VERSION_2_0", wine_glVertexAttrib4uiv },
  { "glVertexAttrib4uivARB", "GL_ARB_vertex_program", wine_glVertexAttrib4uivARB },
  { "glVertexAttrib4usv", "GL_VERSION_2_0", wine_glVertexAttrib4usv },
  { "glVertexAttrib4usvARB", "GL_ARB_vertex_program", wine_glVertexAttrib4usvARB },
  { "glVertexAttribArrayObjectATI", "GL_ATI_vertex_attrib_array_object", wine_glVertexAttribArrayObjectATI },
  { "glVertexAttribDivisor", "GL_VERSION_3_3", wine_glVertexAttribDivisor },
  { "glVertexAttribDivisorARB", "GL_ARB_instanced_arrays", wine_glVertexAttribDivisorARB },
  { "glVertexAttribFormatNV", "GL_NV_vertex_buffer_unified_memory", wine_glVertexAttribFormatNV },
  { "glVertexAttribI1i", "GL_VERSION_3_0", wine_glVertexAttribI1i },
  { "glVertexAttribI1iEXT", "GL_NV_vertex_program4", wine_glVertexAttribI1iEXT },
  { "glVertexAttribI1iv", "GL_VERSION_3_0", wine_glVertexAttribI1iv },
  { "glVertexAttribI1ivEXT", "GL_NV_vertex_program4", wine_glVertexAttribI1ivEXT },
  { "glVertexAttribI1ui", "GL_VERSION_3_0", wine_glVertexAttribI1ui },
  { "glVertexAttribI1uiEXT", "GL_NV_vertex_program4", wine_glVertexAttribI1uiEXT },
  { "glVertexAttribI1uiv", "GL_VERSION_3_0", wine_glVertexAttribI1uiv },
  { "glVertexAttribI1uivEXT", "GL_NV_vertex_program4", wine_glVertexAttribI1uivEXT },
  { "glVertexAttribI2i", "GL_VERSION_3_0", wine_glVertexAttribI2i },
  { "glVertexAttribI2iEXT", "GL_NV_vertex_program4", wine_glVertexAttribI2iEXT },
  { "glVertexAttribI2iv", "GL_VERSION_3_0", wine_glVertexAttribI2iv },
  { "glVertexAttribI2ivEXT", "GL_NV_vertex_program4", wine_glVertexAttribI2ivEXT },
  { "glVertexAttribI2ui", "GL_VERSION_3_0", wine_glVertexAttribI2ui },
  { "glVertexAttribI2uiEXT", "GL_NV_vertex_program4", wine_glVertexAttribI2uiEXT },
  { "glVertexAttribI2uiv", "GL_VERSION_3_0", wine_glVertexAttribI2uiv },
  { "glVertexAttribI2uivEXT", "GL_NV_vertex_program4", wine_glVertexAttribI2uivEXT },
  { "glVertexAttribI3i", "GL_VERSION_3_0", wine_glVertexAttribI3i },
  { "glVertexAttribI3iEXT", "GL_NV_vertex_program4", wine_glVertexAttribI3iEXT },
  { "glVertexAttribI3iv", "GL_VERSION_3_0", wine_glVertexAttribI3iv },
  { "glVertexAttribI3ivEXT", "GL_NV_vertex_program4", wine_glVertexAttribI3ivEXT },
  { "glVertexAttribI3ui", "GL_VERSION_3_0", wine_glVertexAttribI3ui },
  { "glVertexAttribI3uiEXT", "GL_NV_vertex_program4", wine_glVertexAttribI3uiEXT },
  { "glVertexAttribI3uiv", "GL_VERSION_3_0", wine_glVertexAttribI3uiv },
  { "glVertexAttribI3uivEXT", "GL_NV_vertex_program4", wine_glVertexAttribI3uivEXT },
  { "glVertexAttribI4bv", "GL_VERSION_3_0", wine_glVertexAttribI4bv },
  { "glVertexAttribI4bvEXT", "GL_NV_vertex_program4", wine_glVertexAttribI4bvEXT },
  { "glVertexAttribI4i", "GL_VERSION_3_0", wine_glVertexAttribI4i },
  { "glVertexAttribI4iEXT", "GL_NV_vertex_program4", wine_glVertexAttribI4iEXT },
  { "glVertexAttribI4iv", "GL_VERSION_3_0", wine_glVertexAttribI4iv },
  { "glVertexAttribI4ivEXT", "GL_NV_vertex_program4", wine_glVertexAttribI4ivEXT },
  { "glVertexAttribI4sv", "GL_VERSION_3_0", wine_glVertexAttribI4sv },
  { "glVertexAttribI4svEXT", "GL_NV_vertex_program4", wine_glVertexAttribI4svEXT },
  { "glVertexAttribI4ubv", "GL_VERSION_3_0", wine_glVertexAttribI4ubv },
  { "glVertexAttribI4ubvEXT", "GL_NV_vertex_program4", wine_glVertexAttribI4ubvEXT },
  { "glVertexAttribI4ui", "GL_VERSION_3_0", wine_glVertexAttribI4ui },
  { "glVertexAttribI4uiEXT", "GL_NV_vertex_program4", wine_glVertexAttribI4uiEXT },
  { "glVertexAttribI4uiv", "GL_VERSION_3_0", wine_glVertexAttribI4uiv },
  { "glVertexAttribI4uivEXT", "GL_NV_vertex_program4", wine_glVertexAttribI4uivEXT },
  { "glVertexAttribI4usv", "GL_VERSION_3_0", wine_glVertexAttribI4usv },
  { "glVertexAttribI4usvEXT", "GL_NV_vertex_program4", wine_glVertexAttribI4usvEXT },
  { "glVertexAttribIFormatNV", "GL_NV_vertex_buffer_unified_memory", wine_glVertexAttribIFormatNV },
  { "glVertexAttribIPointer", "GL_VERSION_3_0", wine_glVertexAttribIPointer },
  { "glVertexAttribIPointerEXT", "GL_NV_vertex_program4", wine_glVertexAttribIPointerEXT },
  { "glVertexAttribL1d", "GL_ARB_vertex_attrib_64bit", wine_glVertexAttribL1d },
  { "glVertexAttribL1dEXT", "GL_EXT_vertex_attrib_64bit", wine_glVertexAttribL1dEXT },
  { "glVertexAttribL1dv", "GL_ARB_vertex_attrib_64bit", wine_glVertexAttribL1dv },
  { "glVertexAttribL1dvEXT", "GL_EXT_vertex_attrib_64bit", wine_glVertexAttribL1dvEXT },
  { "glVertexAttribL1i64NV", "GL_NV_vertex_attrib_integer_64bit", wine_glVertexAttribL1i64NV },
  { "glVertexAttribL1i64vNV", "GL_NV_vertex_attrib_integer_64bit", wine_glVertexAttribL1i64vNV },
  { "glVertexAttribL1ui64NV", "GL_NV_vertex_attrib_integer_64bit", wine_glVertexAttribL1ui64NV },
  { "glVertexAttribL1ui64vNV", "GL_NV_vertex_attrib_integer_64bit", wine_glVertexAttribL1ui64vNV },
  { "glVertexAttribL2d", "GL_ARB_vertex_attrib_64bit", wine_glVertexAttribL2d },
  { "glVertexAttribL2dEXT", "GL_EXT_vertex_attrib_64bit", wine_glVertexAttribL2dEXT },
  { "glVertexAttribL2dv", "GL_ARB_vertex_attrib_64bit", wine_glVertexAttribL2dv },
  { "glVertexAttribL2dvEXT", "GL_EXT_vertex_attrib_64bit", wine_glVertexAttribL2dvEXT },
  { "glVertexAttribL2i64NV", "GL_NV_vertex_attrib_integer_64bit", wine_glVertexAttribL2i64NV },
  { "glVertexAttribL2i64vNV", "GL_NV_vertex_attrib_integer_64bit", wine_glVertexAttribL2i64vNV },
  { "glVertexAttribL2ui64NV", "GL_NV_vertex_attrib_integer_64bit", wine_glVertexAttribL2ui64NV },
  { "glVertexAttribL2ui64vNV", "GL_NV_vertex_attrib_integer_64bit", wine_glVertexAttribL2ui64vNV },
  { "glVertexAttribL3d", "GL_ARB_vertex_attrib_64bit", wine_glVertexAttribL3d },
  { "glVertexAttribL3dEXT", "GL_EXT_vertex_attrib_64bit", wine_glVertexAttribL3dEXT },
  { "glVertexAttribL3dv", "GL_ARB_vertex_attrib_64bit", wine_glVertexAttribL3dv },
  { "glVertexAttribL3dvEXT", "GL_EXT_vertex_attrib_64bit", wine_glVertexAttribL3dvEXT },
  { "glVertexAttribL3i64NV", "GL_NV_vertex_attrib_integer_64bit", wine_glVertexAttribL3i64NV },
  { "glVertexAttribL3i64vNV", "GL_NV_vertex_attrib_integer_64bit", wine_glVertexAttribL3i64vNV },
  { "glVertexAttribL3ui64NV", "GL_NV_vertex_attrib_integer_64bit", wine_glVertexAttribL3ui64NV },
  { "glVertexAttribL3ui64vNV", "GL_NV_vertex_attrib_integer_64bit", wine_glVertexAttribL3ui64vNV },
  { "glVertexAttribL4d", "GL_ARB_vertex_attrib_64bit", wine_glVertexAttribL4d },
  { "glVertexAttribL4dEXT", "GL_EXT_vertex_attrib_64bit", wine_glVertexAttribL4dEXT },
  { "glVertexAttribL4dv", "GL_ARB_vertex_attrib_64bit", wine_glVertexAttribL4dv },
  { "glVertexAttribL4dvEXT", "GL_EXT_vertex_attrib_64bit", wine_glVertexAttribL4dvEXT },
  { "glVertexAttribL4i64NV", "GL_NV_vertex_attrib_integer_64bit", wine_glVertexAttribL4i64NV },
  { "glVertexAttribL4i64vNV", "GL_NV_vertex_attrib_integer_64bit", wine_glVertexAttribL4i64vNV },
  { "glVertexAttribL4ui64NV", "GL_NV_vertex_attrib_integer_64bit", wine_glVertexAttribL4ui64NV },
  { "glVertexAttribL4ui64vNV", "GL_NV_vertex_attrib_integer_64bit", wine_glVertexAttribL4ui64vNV },
  { "glVertexAttribLFormatNV", "GL_NV_vertex_attrib_integer_64bit", wine_glVertexAttribLFormatNV },
  { "glVertexAttribLPointer", "GL_ARB_vertex_attrib_64bit", wine_glVertexAttribLPointer },
  { "glVertexAttribLPointerEXT", "GL_EXT_vertex_attrib_64bit", wine_glVertexAttribLPointerEXT },
  { "glVertexAttribP1ui", "GL_ARB_vertex_type_2_10_10_10_rev", wine_glVertexAttribP1ui },
  { "glVertexAttribP1uiv", "GL_ARB_vertex_type_2_10_10_10_rev", wine_glVertexAttribP1uiv },
  { "glVertexAttribP2ui", "GL_ARB_vertex_type_2_10_10_10_rev", wine_glVertexAttribP2ui },
  { "glVertexAttribP2uiv", "GL_ARB_vertex_type_2_10_10_10_rev", wine_glVertexAttribP2uiv },
  { "glVertexAttribP3ui", "GL_ARB_vertex_type_2_10_10_10_rev", wine_glVertexAttribP3ui },
  { "glVertexAttribP3uiv", "GL_ARB_vertex_type_2_10_10_10_rev", wine_glVertexAttribP3uiv },
  { "glVertexAttribP4ui", "GL_ARB_vertex_type_2_10_10_10_rev", wine_glVertexAttribP4ui },
  { "glVertexAttribP4uiv", "GL_ARB_vertex_type_2_10_10_10_rev", wine_glVertexAttribP4uiv },
  { "glVertexAttribPointer", "GL_VERSION_2_0", wine_glVertexAttribPointer },
  { "glVertexAttribPointerARB", "GL_ARB_vertex_program", wine_glVertexAttribPointerARB },
  { "glVertexAttribPointerNV", "GL_NV_vertex_program", wine_glVertexAttribPointerNV },
  { "glVertexAttribs1dvNV", "GL_NV_vertex_program", wine_glVertexAttribs1dvNV },
  { "glVertexAttribs1fvNV", "GL_NV_vertex_program", wine_glVertexAttribs1fvNV },
  { "glVertexAttribs1hvNV", "GL_NV_half_float", wine_glVertexAttribs1hvNV },
  { "glVertexAttribs1svNV", "GL_NV_vertex_program", wine_glVertexAttribs1svNV },
  { "glVertexAttribs2dvNV", "GL_NV_vertex_program", wine_glVertexAttribs2dvNV },
  { "glVertexAttribs2fvNV", "GL_NV_vertex_program", wine_glVertexAttribs2fvNV },
  { "glVertexAttribs2hvNV", "GL_NV_half_float", wine_glVertexAttribs2hvNV },
  { "glVertexAttribs2svNV", "GL_NV_vertex_program", wine_glVertexAttribs2svNV },
  { "glVertexAttribs3dvNV", "GL_NV_vertex_program", wine_glVertexAttribs3dvNV },
  { "glVertexAttribs3fvNV", "GL_NV_vertex_program", wine_glVertexAttribs3fvNV },
  { "glVertexAttribs3hvNV", "GL_NV_half_float", wine_glVertexAttribs3hvNV },
  { "glVertexAttribs3svNV", "GL_NV_vertex_program", wine_glVertexAttribs3svNV },
  { "glVertexAttribs4dvNV", "GL_NV_vertex_program", wine_glVertexAttribs4dvNV },
  { "glVertexAttribs4fvNV", "GL_NV_vertex_program", wine_glVertexAttribs4fvNV },
  { "glVertexAttribs4hvNV", "GL_NV_half_float", wine_glVertexAttribs4hvNV },
  { "glVertexAttribs4svNV", "GL_NV_vertex_program", wine_glVertexAttribs4svNV },
  { "glVertexAttribs4ubvNV", "GL_NV_vertex_program", wine_glVertexAttribs4ubvNV },
  { "glVertexBlendARB", "GL_ARB_vertex_blend", wine_glVertexBlendARB },
  { "glVertexBlendEnvfATI", "GL_ATI_vertex_streams", wine_glVertexBlendEnvfATI },
  { "glVertexBlendEnviATI", "GL_ATI_vertex_streams", wine_glVertexBlendEnviATI },
  { "glVertexFormatNV", "GL_NV_vertex_buffer_unified_memory", wine_glVertexFormatNV },
  { "glVertexP2ui", "GL_ARB_vertex_type_2_10_10_10_rev", wine_glVertexP2ui },
  { "glVertexP2uiv", "GL_ARB_vertex_type_2_10_10_10_rev", wine_glVertexP2uiv },
  { "glVertexP3ui", "GL_ARB_vertex_type_2_10_10_10_rev", wine_glVertexP3ui },
  { "glVertexP3uiv", "GL_ARB_vertex_type_2_10_10_10_rev", wine_glVertexP3uiv },
  { "glVertexP4ui", "GL_ARB_vertex_type_2_10_10_10_rev", wine_glVertexP4ui },
  { "glVertexP4uiv", "GL_ARB_vertex_type_2_10_10_10_rev", wine_glVertexP4uiv },
  { "glVertexPointerEXT", "GL_EXT_vertex_array", wine_glVertexPointerEXT },
  { "glVertexPointerListIBM", "GL_IBM_vertex_array_lists", wine_glVertexPointerListIBM },
  { "glVertexPointervINTEL", "GL_INTEL_parallel_arrays", wine_glVertexPointervINTEL },
  { "glVertexStream1dATI", "GL_ATI_vertex_streams", wine_glVertexStream1dATI },
  { "glVertexStream1dvATI", "GL_ATI_vertex_streams", wine_glVertexStream1dvATI },
  { "glVertexStream1fATI", "GL_ATI_vertex_streams", wine_glVertexStream1fATI },
  { "glVertexStream1fvATI", "GL_ATI_vertex_streams", wine_glVertexStream1fvATI },
  { "glVertexStream1iATI", "GL_ATI_vertex_streams", wine_glVertexStream1iATI },
  { "glVertexStream1ivATI", "GL_ATI_vertex_streams", wine_glVertexStream1ivATI },
  { "glVertexStream1sATI", "GL_ATI_vertex_streams", wine_glVertexStream1sATI },
  { "glVertexStream1svATI", "GL_ATI_vertex_streams", wine_glVertexStream1svATI },
  { "glVertexStream2dATI", "GL_ATI_vertex_streams", wine_glVertexStream2dATI },
  { "glVertexStream2dvATI", "GL_ATI_vertex_streams", wine_glVertexStream2dvATI },
  { "glVertexStream2fATI", "GL_ATI_vertex_streams", wine_glVertexStream2fATI },
  { "glVertexStream2fvATI", "GL_ATI_vertex_streams", wine_glVertexStream2fvATI },
  { "glVertexStream2iATI", "GL_ATI_vertex_streams", wine_glVertexStream2iATI },
  { "glVertexStream2ivATI", "GL_ATI_vertex_streams", wine_glVertexStream2ivATI },
  { "glVertexStream2sATI", "GL_ATI_vertex_streams", wine_glVertexStream2sATI },
  { "glVertexStream2svATI", "GL_ATI_vertex_streams", wine_glVertexStream2svATI },
  { "glVertexStream3dATI", "GL_ATI_vertex_streams", wine_glVertexStream3dATI },
  { "glVertexStream3dvATI", "GL_ATI_vertex_streams", wine_glVertexStream3dvATI },
  { "glVertexStream3fATI", "GL_ATI_vertex_streams", wine_glVertexStream3fATI },
  { "glVertexStream3fvATI", "GL_ATI_vertex_streams", wine_glVertexStream3fvATI },
  { "glVertexStream3iATI", "GL_ATI_vertex_streams", wine_glVertexStream3iATI },
  { "glVertexStream3ivATI", "GL_ATI_vertex_streams", wine_glVertexStream3ivATI },
  { "glVertexStream3sATI", "GL_ATI_vertex_streams", wine_glVertexStream3sATI },
  { "glVertexStream3svATI", "GL_ATI_vertex_streams", wine_glVertexStream3svATI },
  { "glVertexStream4dATI", "GL_ATI_vertex_streams", wine_glVertexStream4dATI },
  { "glVertexStream4dvATI", "GL_ATI_vertex_streams", wine_glVertexStream4dvATI },
  { "glVertexStream4fATI", "GL_ATI_vertex_streams", wine_glVertexStream4fATI },
  { "glVertexStream4fvATI", "GL_ATI_vertex_streams", wine_glVertexStream4fvATI },
  { "glVertexStream4iATI", "GL_ATI_vertex_streams", wine_glVertexStream4iATI },
  { "glVertexStream4ivATI", "GL_ATI_vertex_streams", wine_glVertexStream4ivATI },
  { "glVertexStream4sATI", "GL_ATI_vertex_streams", wine_glVertexStream4sATI },
  { "glVertexStream4svATI", "GL_ATI_vertex_streams", wine_glVertexStream4svATI },
  { "glVertexWeightPointerEXT", "GL_EXT_vertex_weighting", wine_glVertexWeightPointerEXT },
  { "glVertexWeightfEXT", "GL_EXT_vertex_weighting", wine_glVertexWeightfEXT },
  { "glVertexWeightfvEXT", "GL_EXT_vertex_weighting", wine_glVertexWeightfvEXT },
  { "glVertexWeighthNV", "GL_NV_half_float", wine_glVertexWeighthNV },
  { "glVertexWeighthvNV", "GL_NV_half_float", wine_glVertexWeighthvNV },
  { "glVideoCaptureNV", "GL_NV_video_capture", wine_glVideoCaptureNV },
  { "glVideoCaptureStreamParameterdvNV", "GL_NV_video_capture", wine_glVideoCaptureStreamParameterdvNV },
  { "glVideoCaptureStreamParameterfvNV", "GL_NV_video_capture", wine_glVideoCaptureStreamParameterfvNV },
  { "glVideoCaptureStreamParameterivNV", "GL_NV_video_capture", wine_glVideoCaptureStreamParameterivNV },
  { "glViewportArrayv", "GL_ARB_viewport_array", wine_glViewportArrayv },
  { "glViewportIndexedf", "GL_ARB_viewport_array", wine_glViewportIndexedf },
  { "glViewportIndexedfv", "GL_ARB_viewport_array", wine_glViewportIndexedfv },
  { "glWaitSync", "GL_ARB_sync", wine_glWaitSync },
  { "glWeightPathsNV", "GL_NV_path_rendering", wine_glWeightPathsNV },
  { "glWeightPointerARB", "GL_ARB_vertex_blend", wine_glWeightPointerARB },
  { "glWeightbvARB", "GL_ARB_vertex_blend", wine_glWeightbvARB },
  { "glWeightdvARB", "GL_ARB_vertex_blend", wine_glWeightdvARB },
  { "glWeightfvARB", "GL_ARB_vertex_blend", wine_glWeightfvARB },
  { "glWeightivARB", "GL_ARB_vertex_blend", wine_glWeightivARB },
  { "glWeightsvARB", "GL_ARB_vertex_blend", wine_glWeightsvARB },
  { "glWeightubvARB", "GL_ARB_vertex_blend", wine_glWeightubvARB },
  { "glWeightuivARB", "GL_ARB_vertex_blend", wine_glWeightuivARB },
  { "glWeightusvARB", "GL_ARB_vertex_blend", wine_glWeightusvARB },
  { "glWindowPos2d", "GL_VERSION_1_4_DEPRECATED", wine_glWindowPos2d },
  { "glWindowPos2dARB", "GL_ARB_window_pos", wine_glWindowPos2dARB },
  { "glWindowPos2dMESA", "GL_MESA_window_pos", wine_glWindowPos2dMESA },
  { "glWindowPos2dv", "GL_VERSION_1_4_DEPRECATED", wine_glWindowPos2dv },
  { "glWindowPos2dvARB", "GL_ARB_window_pos", wine_glWindowPos2dvARB },
  { "glWindowPos2dvMESA", "GL_MESA_window_pos", wine_glWindowPos2dvMESA },
  { "glWindowPos2f", "GL_VERSION_1_4_DEPRECATED", wine_glWindowPos2f },
  { "glWindowPos2fARB", "GL_ARB_window_pos", wine_glWindowPos2fARB },
  { "glWindowPos2fMESA", "GL_MESA_window_pos", wine_glWindowPos2fMESA },
  { "glWindowPos2fv", "GL_VERSION_1_4_DEPRECATED", wine_glWindowPos2fv },
  { "glWindowPos2fvARB", "GL_ARB_window_pos", wine_glWindowPos2fvARB },
  { "glWindowPos2fvMESA", "GL_MESA_window_pos", wine_glWindowPos2fvMESA },
  { "glWindowPos2i", "GL_VERSION_1_4_DEPRECATED", wine_glWindowPos2i },
  { "glWindowPos2iARB", "GL_ARB_window_pos", wine_glWindowPos2iARB },
  { "glWindowPos2iMESA", "GL_MESA_window_pos", wine_glWindowPos2iMESA },
  { "glWindowPos2iv", "GL_VERSION_1_4_DEPRECATED", wine_glWindowPos2iv },
  { "glWindowPos2ivARB", "GL_ARB_window_pos", wine_glWindowPos2ivARB },
  { "glWindowPos2ivMESA", "GL_MESA_window_pos", wine_glWindowPos2ivMESA },
  { "glWindowPos2s", "GL_VERSION_1_4_DEPRECATED", wine_glWindowPos2s },
  { "glWindowPos2sARB", "GL_ARB_window_pos", wine_glWindowPos2sARB },
  { "glWindowPos2sMESA", "GL_MESA_window_pos", wine_glWindowPos2sMESA },
  { "glWindowPos2sv", "GL_VERSION_1_4_DEPRECATED", wine_glWindowPos2sv },
  { "glWindowPos2svARB", "GL_ARB_window_pos", wine_glWindowPos2svARB },
  { "glWindowPos2svMESA", "GL_MESA_window_pos", wine_glWindowPos2svMESA },
  { "glWindowPos3d", "GL_VERSION_1_4_DEPRECATED", wine_glWindowPos3d },
  { "glWindowPos3dARB", "GL_ARB_window_pos", wine_glWindowPos3dARB },
  { "glWindowPos3dMESA", "GL_MESA_window_pos", wine_glWindowPos3dMESA },
  { "glWindowPos3dv", "GL_VERSION_1_4_DEPRECATED", wine_glWindowPos3dv },
  { "glWindowPos3dvARB", "GL_ARB_window_pos", wine_glWindowPos3dvARB },
  { "glWindowPos3dvMESA", "GL_MESA_window_pos", wine_glWindowPos3dvMESA },
  { "glWindowPos3f", "GL_VERSION_1_4_DEPRECATED", wine_glWindowPos3f },
  { "glWindowPos3fARB", "GL_ARB_window_pos", wine_glWindowPos3fARB },
  { "glWindowPos3fMESA", "GL_MESA_window_pos", wine_glWindowPos3fMESA },
  { "glWindowPos3fv", "GL_VERSION_1_4_DEPRECATED", wine_glWindowPos3fv },
  { "glWindowPos3fvARB", "GL_ARB_window_pos", wine_glWindowPos3fvARB },
  { "glWindowPos3fvMESA", "GL_MESA_window_pos", wine_glWindowPos3fvMESA },
  { "glWindowPos3i", "GL_VERSION_1_4_DEPRECATED", wine_glWindowPos3i },
  { "glWindowPos3iARB", "GL_ARB_window_pos", wine_glWindowPos3iARB },
  { "glWindowPos3iMESA", "GL_MESA_window_pos", wine_glWindowPos3iMESA },
  { "glWindowPos3iv", "GL_VERSION_1_4_DEPRECATED", wine_glWindowPos3iv },
  { "glWindowPos3ivARB", "GL_ARB_window_pos", wine_glWindowPos3ivARB },
  { "glWindowPos3ivMESA", "GL_MESA_window_pos", wine_glWindowPos3ivMESA },
  { "glWindowPos3s", "GL_VERSION_1_4_DEPRECATED", wine_glWindowPos3s },
  { "glWindowPos3sARB", "GL_ARB_window_pos", wine_glWindowPos3sARB },
  { "glWindowPos3sMESA", "GL_MESA_window_pos", wine_glWindowPos3sMESA },
  { "glWindowPos3sv", "GL_VERSION_1_4_DEPRECATED", wine_glWindowPos3sv },
  { "glWindowPos3svARB", "GL_ARB_window_pos", wine_glWindowPos3svARB },
  { "glWindowPos3svMESA", "GL_MESA_window_pos", wine_glWindowPos3svMESA },
  { "glWindowPos4dMESA", "GL_MESA_window_pos", wine_glWindowPos4dMESA },
  { "glWindowPos4dvMESA", "GL_MESA_window_pos", wine_glWindowPos4dvMESA },
  { "glWindowPos4fMESA", "GL_MESA_window_pos", wine_glWindowPos4fMESA },
  { "glWindowPos4fvMESA", "GL_MESA_window_pos", wine_glWindowPos4fvMESA },
  { "glWindowPos4iMESA", "GL_MESA_window_pos", wine_glWindowPos4iMESA },
  { "glWindowPos4ivMESA", "GL_MESA_window_pos", wine_glWindowPos4ivMESA },
  { "glWindowPos4sMESA", "GL_MESA_window_pos", wine_glWindowPos4sMESA },
  { "glWindowPos4svMESA", "GL_MESA_window_pos", wine_glWindowPos4svMESA },
  { "glWriteMaskEXT", "GL_EXT_vertex_shader", wine_glWriteMaskEXT },
  { "wglAllocateMemoryNV", "WGL_NV_vertex_array_range", wglAllocateMemoryNV },
  { "wglBindTexImageARB", "WGL_ARB_render_texture", wglBindTexImageARB },
  { "wglChoosePixelFormatARB", "WGL_ARB_pixel_format", wglChoosePixelFormatARB },
  { "wglCreateContextAttribsARB", "WGL_ARB_create_context", wglCreateContextAttribsARB },
  { "wglCreatePbufferARB", "WGL_ARB_pbuffer", wglCreatePbufferARB },
  { "wglDestroyPbufferARB", "WGL_ARB_pbuffer", wglDestroyPbufferARB },
  { "wglFreeMemoryNV", "WGL_NV_vertex_array_range", wglFreeMemoryNV },
  { "wglGetCurrentReadDCARB", "WGL_ARB_make_current_read", wglGetCurrentReadDCARB },
  { "wglGetExtensionsStringARB", "WGL_ARB_extensions_string", wglGetExtensionsStringARB },
  { "wglGetExtensionsStringEXT", "WGL_EXT_extensions_string", wglGetExtensionsStringEXT },
  { "wglGetPbufferDCARB", "WGL_ARB_pbuffer", wglGetPbufferDCARB },
  { "wglGetPixelFormatAttribfvARB", "WGL_ARB_pixel_format", wglGetPixelFormatAttribfvARB },
  { "wglGetPixelFormatAttribivARB", "WGL_ARB_pixel_format", wglGetPixelFormatAttribivARB },
  { "wglGetSwapIntervalEXT", "WGL_EXT_swap_control", wglGetSwapIntervalEXT },
  { "wglMakeContextCurrentARB", "WGL_ARB_make_current_read", wglMakeContextCurrentARB },
  { "wglQueryPbufferARB", "WGL_ARB_pbuffer", wglQueryPbufferARB },
  { "wglReleasePbufferDCARB", "WGL_ARB_pbuffer", wglReleasePbufferDCARB },
  { "wglReleaseTexImageARB", "WGL_ARB_render_texture", wglReleaseTexImageARB },
  { "wglSetPbufferAttribARB", "WGL_ARB_render_texture", wglSetPbufferAttribARB },
  { "wglSetPixelFormatWINE", "WGL_WINE_pixel_format_passthrough", wglSetPixelFormatWINE },
  { "wglSwapIntervalEXT", "WGL_EXT_swap_control", wglSwapIntervalEXT }
};
