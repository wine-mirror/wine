/* Automatically generated from http://www.opengl.org/registry files; DO NOT EDIT! */

#include "config.h"
#include <stdarg.h>
#include "winternl.h"
#include "opengl_ext.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(opengl);

const int extension_registry_size = 2655;

static void WINAPI glAccumxOES( GLenum op, GLfixed value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", op, value );
  funcs->ext.p_glAccumxOES( op, value );
}

static GLboolean WINAPI glAcquireKeyedMutexWin32EXT( GLuint memory, GLuint64 key, GLuint timeout )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %s, %d)\n", memory, wine_dbgstr_longlong(key), timeout );
  return funcs->ext.p_glAcquireKeyedMutexWin32EXT( memory, key, timeout );
}

static void WINAPI glActiveProgramEXT( GLuint program )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", program );
  funcs->ext.p_glActiveProgramEXT( program );
}

static void WINAPI glActiveShaderProgram( GLuint pipeline, GLuint program )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", pipeline, program );
  funcs->ext.p_glActiveShaderProgram( pipeline, program );
}

static void WINAPI glActiveStencilFaceEXT( GLenum face )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", face );
  funcs->ext.p_glActiveStencilFaceEXT( face );
}

static void WINAPI glActiveTexture( GLenum texture )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", texture );
  funcs->ext.p_glActiveTexture( texture );
}

static void WINAPI glActiveTextureARB( GLenum texture )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", texture );
  funcs->ext.p_glActiveTextureARB( texture );
}

static void WINAPI glActiveVaryingNV( GLuint program, const GLchar *name )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", program, name );
  funcs->ext.p_glActiveVaryingNV( program, name );
}

static void WINAPI glAlphaFragmentOp1ATI( GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d)\n", op, dst, dstMod, arg1, arg1Rep, arg1Mod );
  funcs->ext.p_glAlphaFragmentOp1ATI( op, dst, dstMod, arg1, arg1Rep, arg1Mod );
}

static void WINAPI glAlphaFragmentOp2ATI( GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d)\n", op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod );
  funcs->ext.p_glAlphaFragmentOp2ATI( op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod );
}

static void WINAPI glAlphaFragmentOp3ATI( GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod );
  funcs->ext.p_glAlphaFragmentOp3ATI( op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod );
}

static void WINAPI glAlphaFuncxOES( GLenum func, GLfixed ref )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", func, ref );
  funcs->ext.p_glAlphaFuncxOES( func, ref );
}

static void WINAPI glAlphaToCoverageDitherControlNV( GLenum mode )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", mode );
  funcs->ext.p_glAlphaToCoverageDitherControlNV( mode );
}

static void WINAPI glApplyFramebufferAttachmentCMAAINTEL(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  funcs->ext.p_glApplyFramebufferAttachmentCMAAINTEL();
}

static void WINAPI glApplyTextureEXT( GLenum mode )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", mode );
  funcs->ext.p_glApplyTextureEXT( mode );
}

static GLboolean WINAPI glAreProgramsResidentNV( GLsizei n, const GLuint *programs, GLboolean *residences )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p, %p)\n", n, programs, residences );
  return funcs->ext.p_glAreProgramsResidentNV( n, programs, residences );
}

static GLboolean WINAPI glAreTexturesResidentEXT( GLsizei n, const GLuint *textures, GLboolean *residences )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p, %p)\n", n, textures, residences );
  return funcs->ext.p_glAreTexturesResidentEXT( n, textures, residences );
}

static void WINAPI glArrayElementEXT( GLint i )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", i );
  funcs->ext.p_glArrayElementEXT( i );
}

static void WINAPI glArrayObjectATI( GLenum array, GLint size, GLenum type, GLsizei stride, GLuint buffer, GLuint offset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d)\n", array, size, type, stride, buffer, offset );
  funcs->ext.p_glArrayObjectATI( array, size, type, stride, buffer, offset );
}

static void WINAPI glAsyncMarkerSGIX( GLuint marker )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", marker );
  funcs->ext.p_glAsyncMarkerSGIX( marker );
}

static void WINAPI glAttachObjectARB( GLhandleARB containerObj, GLhandleARB obj )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", containerObj, obj );
  funcs->ext.p_glAttachObjectARB( containerObj, obj );
}

static void WINAPI glAttachShader( GLuint program, GLuint shader )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", program, shader );
  funcs->ext.p_glAttachShader( program, shader );
}

static void WINAPI glBeginConditionalRender( GLuint id, GLenum mode )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", id, mode );
  funcs->ext.p_glBeginConditionalRender( id, mode );
}

static void WINAPI glBeginConditionalRenderNV( GLuint id, GLenum mode )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", id, mode );
  funcs->ext.p_glBeginConditionalRenderNV( id, mode );
}

static void WINAPI glBeginConditionalRenderNVX( GLuint id )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", id );
  funcs->ext.p_glBeginConditionalRenderNVX( id );
}

static void WINAPI glBeginFragmentShaderATI(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  funcs->ext.p_glBeginFragmentShaderATI();
}

static void WINAPI glBeginOcclusionQueryNV( GLuint id )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", id );
  funcs->ext.p_glBeginOcclusionQueryNV( id );
}

static void WINAPI glBeginPerfMonitorAMD( GLuint monitor )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", monitor );
  funcs->ext.p_glBeginPerfMonitorAMD( monitor );
}

static void WINAPI glBeginPerfQueryINTEL( GLuint queryHandle )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", queryHandle );
  funcs->ext.p_glBeginPerfQueryINTEL( queryHandle );
}

static void WINAPI glBeginQuery( GLenum target, GLuint id )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", target, id );
  funcs->ext.p_glBeginQuery( target, id );
}

static void WINAPI glBeginQueryARB( GLenum target, GLuint id )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", target, id );
  funcs->ext.p_glBeginQueryARB( target, id );
}

static void WINAPI glBeginQueryIndexed( GLenum target, GLuint index, GLuint id )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", target, index, id );
  funcs->ext.p_glBeginQueryIndexed( target, index, id );
}

static void WINAPI glBeginTransformFeedback( GLenum primitiveMode )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", primitiveMode );
  funcs->ext.p_glBeginTransformFeedback( primitiveMode );
}

static void WINAPI glBeginTransformFeedbackEXT( GLenum primitiveMode )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", primitiveMode );
  funcs->ext.p_glBeginTransformFeedbackEXT( primitiveMode );
}

static void WINAPI glBeginTransformFeedbackNV( GLenum primitiveMode )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", primitiveMode );
  funcs->ext.p_glBeginTransformFeedbackNV( primitiveMode );
}

static void WINAPI glBeginVertexShaderEXT(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  funcs->ext.p_glBeginVertexShaderEXT();
}

static void WINAPI glBeginVideoCaptureNV( GLuint video_capture_slot )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", video_capture_slot );
  funcs->ext.p_glBeginVideoCaptureNV( video_capture_slot );
}

static void WINAPI glBindAttribLocation( GLuint program, GLuint index, const GLchar *name )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", program, index, name );
  funcs->ext.p_glBindAttribLocation( program, index, name );
}

static void WINAPI glBindAttribLocationARB( GLhandleARB programObj, GLuint index, const GLcharARB *name )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", programObj, index, name );
  funcs->ext.p_glBindAttribLocationARB( programObj, index, name );
}

static void WINAPI glBindBuffer( GLenum target, GLuint buffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", target, buffer );
  funcs->ext.p_glBindBuffer( target, buffer );
}

static void WINAPI glBindBufferARB( GLenum target, GLuint buffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", target, buffer );
  funcs->ext.p_glBindBufferARB( target, buffer );
}

static void WINAPI glBindBufferBase( GLenum target, GLuint index, GLuint buffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", target, index, buffer );
  funcs->ext.p_glBindBufferBase( target, index, buffer );
}

static void WINAPI glBindBufferBaseEXT( GLenum target, GLuint index, GLuint buffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", target, index, buffer );
  funcs->ext.p_glBindBufferBaseEXT( target, index, buffer );
}

static void WINAPI glBindBufferBaseNV( GLenum target, GLuint index, GLuint buffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", target, index, buffer );
  funcs->ext.p_glBindBufferBaseNV( target, index, buffer );
}

static void WINAPI glBindBufferOffsetEXT( GLenum target, GLuint index, GLuint buffer, GLintptr offset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %ld)\n", target, index, buffer, offset );
  funcs->ext.p_glBindBufferOffsetEXT( target, index, buffer, offset );
}

static void WINAPI glBindBufferOffsetNV( GLenum target, GLuint index, GLuint buffer, GLintptr offset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %ld)\n", target, index, buffer, offset );
  funcs->ext.p_glBindBufferOffsetNV( target, index, buffer, offset );
}

static void WINAPI glBindBufferRange( GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %ld, %ld)\n", target, index, buffer, offset, size );
  funcs->ext.p_glBindBufferRange( target, index, buffer, offset, size );
}

static void WINAPI glBindBufferRangeEXT( GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %ld, %ld)\n", target, index, buffer, offset, size );
  funcs->ext.p_glBindBufferRangeEXT( target, index, buffer, offset, size );
}

static void WINAPI glBindBufferRangeNV( GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %ld, %ld)\n", target, index, buffer, offset, size );
  funcs->ext.p_glBindBufferRangeNV( target, index, buffer, offset, size );
}

static void WINAPI glBindBuffersBase( GLenum target, GLuint first, GLsizei count, const GLuint *buffers )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", target, first, count, buffers );
  funcs->ext.p_glBindBuffersBase( target, first, count, buffers );
}

static void WINAPI glBindBuffersRange( GLenum target, GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizeiptr *sizes )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p, %p, %p)\n", target, first, count, buffers, offsets, sizes );
  funcs->ext.p_glBindBuffersRange( target, first, count, buffers, offsets, sizes );
}

static void WINAPI glBindFragDataLocation( GLuint program, GLuint color, const GLchar *name )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", program, color, name );
  funcs->ext.p_glBindFragDataLocation( program, color, name );
}

static void WINAPI glBindFragDataLocationEXT( GLuint program, GLuint color, const GLchar *name )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", program, color, name );
  funcs->ext.p_glBindFragDataLocationEXT( program, color, name );
}

static void WINAPI glBindFragDataLocationIndexed( GLuint program, GLuint colorNumber, GLuint index, const GLchar *name )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, colorNumber, index, name );
  funcs->ext.p_glBindFragDataLocationIndexed( program, colorNumber, index, name );
}

static void WINAPI glBindFragmentShaderATI( GLuint id )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", id );
  funcs->ext.p_glBindFragmentShaderATI( id );
}

static void WINAPI glBindFramebuffer( GLenum target, GLuint framebuffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", target, framebuffer );
  funcs->ext.p_glBindFramebuffer( target, framebuffer );
}

static void WINAPI glBindFramebufferEXT( GLenum target, GLuint framebuffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", target, framebuffer );
  funcs->ext.p_glBindFramebufferEXT( target, framebuffer );
}

static void WINAPI glBindImageTexture( GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d)\n", unit, texture, level, layered, layer, access, format );
  funcs->ext.p_glBindImageTexture( unit, texture, level, layered, layer, access, format );
}

static void WINAPI glBindImageTextureEXT( GLuint index, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLint format )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d)\n", index, texture, level, layered, layer, access, format );
  funcs->ext.p_glBindImageTextureEXT( index, texture, level, layered, layer, access, format );
}

static void WINAPI glBindImageTextures( GLuint first, GLsizei count, const GLuint *textures )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", first, count, textures );
  funcs->ext.p_glBindImageTextures( first, count, textures );
}

static GLuint WINAPI glBindLightParameterEXT( GLenum light, GLenum value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", light, value );
  return funcs->ext.p_glBindLightParameterEXT( light, value );
}

static GLuint WINAPI glBindMaterialParameterEXT( GLenum face, GLenum value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", face, value );
  return funcs->ext.p_glBindMaterialParameterEXT( face, value );
}

static void WINAPI glBindMultiTextureEXT( GLenum texunit, GLenum target, GLuint texture )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", texunit, target, texture );
  funcs->ext.p_glBindMultiTextureEXT( texunit, target, texture );
}

static GLuint WINAPI glBindParameterEXT( GLenum value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", value );
  return funcs->ext.p_glBindParameterEXT( value );
}

static void WINAPI glBindProgramARB( GLenum target, GLuint program )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", target, program );
  funcs->ext.p_glBindProgramARB( target, program );
}

static void WINAPI glBindProgramNV( GLenum target, GLuint id )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", target, id );
  funcs->ext.p_glBindProgramNV( target, id );
}

static void WINAPI glBindProgramPipeline( GLuint pipeline )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", pipeline );
  funcs->ext.p_glBindProgramPipeline( pipeline );
}

static void WINAPI glBindRenderbuffer( GLenum target, GLuint renderbuffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", target, renderbuffer );
  funcs->ext.p_glBindRenderbuffer( target, renderbuffer );
}

static void WINAPI glBindRenderbufferEXT( GLenum target, GLuint renderbuffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", target, renderbuffer );
  funcs->ext.p_glBindRenderbufferEXT( target, renderbuffer );
}

static void WINAPI glBindSampler( GLuint unit, GLuint sampler )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", unit, sampler );
  funcs->ext.p_glBindSampler( unit, sampler );
}

static void WINAPI glBindSamplers( GLuint first, GLsizei count, const GLuint *samplers )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", first, count, samplers );
  funcs->ext.p_glBindSamplers( first, count, samplers );
}

static GLuint WINAPI glBindTexGenParameterEXT( GLenum unit, GLenum coord, GLenum value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", unit, coord, value );
  return funcs->ext.p_glBindTexGenParameterEXT( unit, coord, value );
}

static void WINAPI glBindTextureEXT( GLenum target, GLuint texture )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", target, texture );
  funcs->ext.p_glBindTextureEXT( target, texture );
}

static void WINAPI glBindTextureUnit( GLuint unit, GLuint texture )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", unit, texture );
  funcs->ext.p_glBindTextureUnit( unit, texture );
}

static GLuint WINAPI glBindTextureUnitParameterEXT( GLenum unit, GLenum value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", unit, value );
  return funcs->ext.p_glBindTextureUnitParameterEXT( unit, value );
}

static void WINAPI glBindTextures( GLuint first, GLsizei count, const GLuint *textures )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", first, count, textures );
  funcs->ext.p_glBindTextures( first, count, textures );
}

static void WINAPI glBindTransformFeedback( GLenum target, GLuint id )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", target, id );
  funcs->ext.p_glBindTransformFeedback( target, id );
}

static void WINAPI glBindTransformFeedbackNV( GLenum target, GLuint id )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", target, id );
  funcs->ext.p_glBindTransformFeedbackNV( target, id );
}

static void WINAPI glBindVertexArray( GLuint array )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", array );
  funcs->ext.p_glBindVertexArray( array );
}

static void WINAPI glBindVertexArrayAPPLE( GLuint array )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", array );
  funcs->ext.p_glBindVertexArrayAPPLE( array );
}

static void WINAPI glBindVertexBuffer( GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %ld, %d)\n", bindingindex, buffer, offset, stride );
  funcs->ext.p_glBindVertexBuffer( bindingindex, buffer, offset, stride );
}

static void WINAPI glBindVertexBuffers( GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizei *strides )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %p, %p)\n", first, count, buffers, offsets, strides );
  funcs->ext.p_glBindVertexBuffers( first, count, buffers, offsets, strides );
}

static void WINAPI glBindVertexShaderEXT( GLuint id )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", id );
  funcs->ext.p_glBindVertexShaderEXT( id );
}

static void WINAPI glBindVideoCaptureStreamBufferNV( GLuint video_capture_slot, GLuint stream, GLenum frame_region, GLintptrARB offset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %ld)\n", video_capture_slot, stream, frame_region, offset );
  funcs->ext.p_glBindVideoCaptureStreamBufferNV( video_capture_slot, stream, frame_region, offset );
}

static void WINAPI glBindVideoCaptureStreamTextureNV( GLuint video_capture_slot, GLuint stream, GLenum frame_region, GLenum target, GLuint texture )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", video_capture_slot, stream, frame_region, target, texture );
  funcs->ext.p_glBindVideoCaptureStreamTextureNV( video_capture_slot, stream, frame_region, target, texture );
}

static void WINAPI glBinormal3bEXT( GLbyte bx, GLbyte by, GLbyte bz )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", bx, by, bz );
  funcs->ext.p_glBinormal3bEXT( bx, by, bz );
}

static void WINAPI glBinormal3bvEXT( const GLbyte *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glBinormal3bvEXT( v );
}

static void WINAPI glBinormal3dEXT( GLdouble bx, GLdouble by, GLdouble bz )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f, %f)\n", bx, by, bz );
  funcs->ext.p_glBinormal3dEXT( bx, by, bz );
}

static void WINAPI glBinormal3dvEXT( const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glBinormal3dvEXT( v );
}

static void WINAPI glBinormal3fEXT( GLfloat bx, GLfloat by, GLfloat bz )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f, %f)\n", bx, by, bz );
  funcs->ext.p_glBinormal3fEXT( bx, by, bz );
}

static void WINAPI glBinormal3fvEXT( const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glBinormal3fvEXT( v );
}

static void WINAPI glBinormal3iEXT( GLint bx, GLint by, GLint bz )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", bx, by, bz );
  funcs->ext.p_glBinormal3iEXT( bx, by, bz );
}

static void WINAPI glBinormal3ivEXT( const GLint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glBinormal3ivEXT( v );
}

static void WINAPI glBinormal3sEXT( GLshort bx, GLshort by, GLshort bz )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", bx, by, bz );
  funcs->ext.p_glBinormal3sEXT( bx, by, bz );
}

static void WINAPI glBinormal3svEXT( const GLshort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glBinormal3svEXT( v );
}

static void WINAPI glBinormalPointerEXT( GLenum type, GLsizei stride, const void *pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", type, stride, pointer );
  funcs->ext.p_glBinormalPointerEXT( type, stride, pointer );
}

static void WINAPI glBitmapxOES( GLsizei width, GLsizei height, GLfixed xorig, GLfixed yorig, GLfixed xmove, GLfixed ymove, const GLubyte *bitmap )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %p)\n", width, height, xorig, yorig, xmove, ymove, bitmap );
  funcs->ext.p_glBitmapxOES( width, height, xorig, yorig, xmove, ymove, bitmap );
}

static void WINAPI glBlendBarrierKHR(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  funcs->ext.p_glBlendBarrierKHR();
}

static void WINAPI glBlendBarrierNV(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  funcs->ext.p_glBlendBarrierNV();
}

static void WINAPI glBlendColor( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f, %f, %f)\n", red, green, blue, alpha );
  funcs->ext.p_glBlendColor( red, green, blue, alpha );
}

static void WINAPI glBlendColorEXT( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f, %f, %f)\n", red, green, blue, alpha );
  funcs->ext.p_glBlendColorEXT( red, green, blue, alpha );
}

static void WINAPI glBlendColorxOES( GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", red, green, blue, alpha );
  funcs->ext.p_glBlendColorxOES( red, green, blue, alpha );
}

static void WINAPI glBlendEquation( GLenum mode )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", mode );
  funcs->ext.p_glBlendEquation( mode );
}

static void WINAPI glBlendEquationEXT( GLenum mode )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", mode );
  funcs->ext.p_glBlendEquationEXT( mode );
}

static void WINAPI glBlendEquationIndexedAMD( GLuint buf, GLenum mode )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", buf, mode );
  funcs->ext.p_glBlendEquationIndexedAMD( buf, mode );
}

static void WINAPI glBlendEquationSeparate( GLenum modeRGB, GLenum modeAlpha )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", modeRGB, modeAlpha );
  funcs->ext.p_glBlendEquationSeparate( modeRGB, modeAlpha );
}

static void WINAPI glBlendEquationSeparateEXT( GLenum modeRGB, GLenum modeAlpha )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", modeRGB, modeAlpha );
  funcs->ext.p_glBlendEquationSeparateEXT( modeRGB, modeAlpha );
}

static void WINAPI glBlendEquationSeparateIndexedAMD( GLuint buf, GLenum modeRGB, GLenum modeAlpha )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", buf, modeRGB, modeAlpha );
  funcs->ext.p_glBlendEquationSeparateIndexedAMD( buf, modeRGB, modeAlpha );
}

static void WINAPI glBlendEquationSeparatei( GLuint buf, GLenum modeRGB, GLenum modeAlpha )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", buf, modeRGB, modeAlpha );
  funcs->ext.p_glBlendEquationSeparatei( buf, modeRGB, modeAlpha );
}

static void WINAPI glBlendEquationSeparateiARB( GLuint buf, GLenum modeRGB, GLenum modeAlpha )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", buf, modeRGB, modeAlpha );
  funcs->ext.p_glBlendEquationSeparateiARB( buf, modeRGB, modeAlpha );
}

static void WINAPI glBlendEquationi( GLuint buf, GLenum mode )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", buf, mode );
  funcs->ext.p_glBlendEquationi( buf, mode );
}

static void WINAPI glBlendEquationiARB( GLuint buf, GLenum mode )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", buf, mode );
  funcs->ext.p_glBlendEquationiARB( buf, mode );
}

static void WINAPI glBlendFuncIndexedAMD( GLuint buf, GLenum src, GLenum dst )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", buf, src, dst );
  funcs->ext.p_glBlendFuncIndexedAMD( buf, src, dst );
}

static void WINAPI glBlendFuncSeparate( GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha );
  funcs->ext.p_glBlendFuncSeparate( sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha );
}

static void WINAPI glBlendFuncSeparateEXT( GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha );
  funcs->ext.p_glBlendFuncSeparateEXT( sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha );
}

static void WINAPI glBlendFuncSeparateINGR( GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha );
  funcs->ext.p_glBlendFuncSeparateINGR( sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha );
}

static void WINAPI glBlendFuncSeparateIndexedAMD( GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", buf, srcRGB, dstRGB, srcAlpha, dstAlpha );
  funcs->ext.p_glBlendFuncSeparateIndexedAMD( buf, srcRGB, dstRGB, srcAlpha, dstAlpha );
}

static void WINAPI glBlendFuncSeparatei( GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", buf, srcRGB, dstRGB, srcAlpha, dstAlpha );
  funcs->ext.p_glBlendFuncSeparatei( buf, srcRGB, dstRGB, srcAlpha, dstAlpha );
}

static void WINAPI glBlendFuncSeparateiARB( GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", buf, srcRGB, dstRGB, srcAlpha, dstAlpha );
  funcs->ext.p_glBlendFuncSeparateiARB( buf, srcRGB, dstRGB, srcAlpha, dstAlpha );
}

static void WINAPI glBlendFunci( GLuint buf, GLenum src, GLenum dst )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", buf, src, dst );
  funcs->ext.p_glBlendFunci( buf, src, dst );
}

static void WINAPI glBlendFunciARB( GLuint buf, GLenum src, GLenum dst )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", buf, src, dst );
  funcs->ext.p_glBlendFunciARB( buf, src, dst );
}

static void WINAPI glBlendParameteriNV( GLenum pname, GLint value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", pname, value );
  funcs->ext.p_glBlendParameteriNV( pname, value );
}

static void WINAPI glBlitFramebuffer( GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter );
  funcs->ext.p_glBlitFramebuffer( srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter );
}

static void WINAPI glBlitFramebufferEXT( GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter );
  funcs->ext.p_glBlitFramebufferEXT( srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter );
}

static void WINAPI glBlitNamedFramebuffer( GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", readFramebuffer, drawFramebuffer, srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter );
  funcs->ext.p_glBlitNamedFramebuffer( readFramebuffer, drawFramebuffer, srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter );
}

static void WINAPI glBufferAddressRangeNV( GLenum pname, GLuint index, GLuint64EXT address, GLsizeiptr length )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %s, %ld)\n", pname, index, wine_dbgstr_longlong(address), length );
  funcs->ext.p_glBufferAddressRangeNV( pname, index, address, length );
}

static void WINAPI glBufferData( GLenum target, GLsizeiptr size, const void *data, GLenum usage )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %ld, %p, %d)\n", target, size, data, usage );
  funcs->ext.p_glBufferData( target, size, data, usage );
}

static void WINAPI glBufferDataARB( GLenum target, GLsizeiptrARB size, const void *data, GLenum usage )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %ld, %p, %d)\n", target, size, data, usage );
  funcs->ext.p_glBufferDataARB( target, size, data, usage );
}

static void WINAPI glBufferPageCommitmentARB( GLenum target, GLintptr offset, GLsizeiptr size, GLboolean commit )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %ld, %ld, %d)\n", target, offset, size, commit );
  funcs->ext.p_glBufferPageCommitmentARB( target, offset, size, commit );
}

static void WINAPI glBufferParameteriAPPLE( GLenum target, GLenum pname, GLint param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", target, pname, param );
  funcs->ext.p_glBufferParameteriAPPLE( target, pname, param );
}

static GLuint WINAPI glBufferRegionEnabled(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  return funcs->ext.p_glBufferRegionEnabled();
}

static void WINAPI glBufferStorage( GLenum target, GLsizeiptr size, const void *data, GLbitfield flags )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %ld, %p, %d)\n", target, size, data, flags );
  funcs->ext.p_glBufferStorage( target, size, data, flags );
}

static void WINAPI glBufferStorageExternalEXT( GLenum target, GLintptr offset, GLsizeiptr size, GLeglClientBufferEXT clientBuffer, GLbitfield flags )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %ld, %ld, %p, %d)\n", target, offset, size, clientBuffer, flags );
  funcs->ext.p_glBufferStorageExternalEXT( target, offset, size, clientBuffer, flags );
}

static void WINAPI glBufferStorageMemEXT( GLenum target, GLsizeiptr size, GLuint memory, GLuint64 offset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %ld, %d, %s)\n", target, size, memory, wine_dbgstr_longlong(offset) );
  funcs->ext.p_glBufferStorageMemEXT( target, size, memory, offset );
}

static void WINAPI glBufferSubData( GLenum target, GLintptr offset, GLsizeiptr size, const void *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %ld, %ld, %p)\n", target, offset, size, data );
  funcs->ext.p_glBufferSubData( target, offset, size, data );
}

static void WINAPI glBufferSubDataARB( GLenum target, GLintptrARB offset, GLsizeiptrARB size, const void *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %ld, %ld, %p)\n", target, offset, size, data );
  funcs->ext.p_glBufferSubDataARB( target, offset, size, data );
}

static void WINAPI glCallCommandListNV( GLuint list )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", list );
  funcs->ext.p_glCallCommandListNV( list );
}

static GLenum WINAPI glCheckFramebufferStatus( GLenum target )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", target );
  return funcs->ext.p_glCheckFramebufferStatus( target );
}

static GLenum WINAPI glCheckFramebufferStatusEXT( GLenum target )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", target );
  return funcs->ext.p_glCheckFramebufferStatusEXT( target );
}

static GLenum WINAPI glCheckNamedFramebufferStatus( GLuint framebuffer, GLenum target )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", framebuffer, target );
  return funcs->ext.p_glCheckNamedFramebufferStatus( framebuffer, target );
}

static GLenum WINAPI glCheckNamedFramebufferStatusEXT( GLuint framebuffer, GLenum target )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", framebuffer, target );
  return funcs->ext.p_glCheckNamedFramebufferStatusEXT( framebuffer, target );
}

static void WINAPI glClampColor( GLenum target, GLenum clamp )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", target, clamp );
  funcs->ext.p_glClampColor( target, clamp );
}

static void WINAPI glClampColorARB( GLenum target, GLenum clamp )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", target, clamp );
  funcs->ext.p_glClampColorARB( target, clamp );
}

static void WINAPI glClearAccumxOES( GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", red, green, blue, alpha );
  funcs->ext.p_glClearAccumxOES( red, green, blue, alpha );
}

static void WINAPI glClearBufferData( GLenum target, GLenum internalformat, GLenum format, GLenum type, const void *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", target, internalformat, format, type, data );
  funcs->ext.p_glClearBufferData( target, internalformat, format, type, data );
}

static void WINAPI glClearBufferSubData( GLenum target, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %ld, %ld, %d, %d, %p)\n", target, internalformat, offset, size, format, type, data );
  funcs->ext.p_glClearBufferSubData( target, internalformat, offset, size, format, type, data );
}

static void WINAPI glClearBufferfi( GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %f, %d)\n", buffer, drawbuffer, depth, stencil );
  funcs->ext.p_glClearBufferfi( buffer, drawbuffer, depth, stencil );
}

static void WINAPI glClearBufferfv( GLenum buffer, GLint drawbuffer, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", buffer, drawbuffer, value );
  funcs->ext.p_glClearBufferfv( buffer, drawbuffer, value );
}

static void WINAPI glClearBufferiv( GLenum buffer, GLint drawbuffer, const GLint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", buffer, drawbuffer, value );
  funcs->ext.p_glClearBufferiv( buffer, drawbuffer, value );
}

static void WINAPI glClearBufferuiv( GLenum buffer, GLint drawbuffer, const GLuint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", buffer, drawbuffer, value );
  funcs->ext.p_glClearBufferuiv( buffer, drawbuffer, value );
}

static void WINAPI glClearColorIiEXT( GLint red, GLint green, GLint blue, GLint alpha )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", red, green, blue, alpha );
  funcs->ext.p_glClearColorIiEXT( red, green, blue, alpha );
}

static void WINAPI glClearColorIuiEXT( GLuint red, GLuint green, GLuint blue, GLuint alpha )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", red, green, blue, alpha );
  funcs->ext.p_glClearColorIuiEXT( red, green, blue, alpha );
}

static void WINAPI glClearColorxOES( GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", red, green, blue, alpha );
  funcs->ext.p_glClearColorxOES( red, green, blue, alpha );
}

static void WINAPI glClearDepthdNV( GLdouble depth )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f)\n", depth );
  funcs->ext.p_glClearDepthdNV( depth );
}

static void WINAPI glClearDepthf( GLfloat d )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f)\n", d );
  funcs->ext.p_glClearDepthf( d );
}

static void WINAPI glClearDepthfOES( GLclampf depth )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f)\n", depth );
  funcs->ext.p_glClearDepthfOES( depth );
}

static void WINAPI glClearDepthxOES( GLfixed depth )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", depth );
  funcs->ext.p_glClearDepthxOES( depth );
}

static void WINAPI glClearNamedBufferData( GLuint buffer, GLenum internalformat, GLenum format, GLenum type, const void *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", buffer, internalformat, format, type, data );
  funcs->ext.p_glClearNamedBufferData( buffer, internalformat, format, type, data );
}

static void WINAPI glClearNamedBufferDataEXT( GLuint buffer, GLenum internalformat, GLenum format, GLenum type, const void *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", buffer, internalformat, format, type, data );
  funcs->ext.p_glClearNamedBufferDataEXT( buffer, internalformat, format, type, data );
}

static void WINAPI glClearNamedBufferSubData( GLuint buffer, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %ld, %ld, %d, %d, %p)\n", buffer, internalformat, offset, size, format, type, data );
  funcs->ext.p_glClearNamedBufferSubData( buffer, internalformat, offset, size, format, type, data );
}

static void WINAPI glClearNamedBufferSubDataEXT( GLuint buffer, GLenum internalformat, GLsizeiptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %ld, %ld, %d, %d, %p)\n", buffer, internalformat, offset, size, format, type, data );
  funcs->ext.p_glClearNamedBufferSubDataEXT( buffer, internalformat, offset, size, format, type, data );
}

static void WINAPI glClearNamedFramebufferfi( GLuint framebuffer, GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %f, %d)\n", framebuffer, buffer, drawbuffer, depth, stencil );
  funcs->ext.p_glClearNamedFramebufferfi( framebuffer, buffer, drawbuffer, depth, stencil );
}

static void WINAPI glClearNamedFramebufferfv( GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", framebuffer, buffer, drawbuffer, value );
  funcs->ext.p_glClearNamedFramebufferfv( framebuffer, buffer, drawbuffer, value );
}

static void WINAPI glClearNamedFramebufferiv( GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", framebuffer, buffer, drawbuffer, value );
  funcs->ext.p_glClearNamedFramebufferiv( framebuffer, buffer, drawbuffer, value );
}

static void WINAPI glClearNamedFramebufferuiv( GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLuint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", framebuffer, buffer, drawbuffer, value );
  funcs->ext.p_glClearNamedFramebufferuiv( framebuffer, buffer, drawbuffer, value );
}

static void WINAPI glClearTexImage( GLuint texture, GLint level, GLenum format, GLenum type, const void *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", texture, level, format, type, data );
  funcs->ext.p_glClearTexImage( texture, level, format, type, data );
}

static void WINAPI glClearTexSubImage( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, data );
  funcs->ext.p_glClearTexSubImage( texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, data );
}

static void WINAPI glClientActiveTexture( GLenum texture )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", texture );
  funcs->ext.p_glClientActiveTexture( texture );
}

static void WINAPI glClientActiveTextureARB( GLenum texture )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", texture );
  funcs->ext.p_glClientActiveTextureARB( texture );
}

static void WINAPI glClientActiveVertexStreamATI( GLenum stream )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", stream );
  funcs->ext.p_glClientActiveVertexStreamATI( stream );
}

static void WINAPI glClientAttribDefaultEXT( GLbitfield mask )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", mask );
  funcs->ext.p_glClientAttribDefaultEXT( mask );
}

static GLenum WINAPI glClientWaitSync( GLsync sync, GLbitfield flags, GLuint64 timeout )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p, %d, %s)\n", sync, flags, wine_dbgstr_longlong(timeout) );
  return funcs->ext.p_glClientWaitSync( sync, flags, timeout );
}

static void WINAPI glClipControl( GLenum origin, GLenum depth )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", origin, depth );
  funcs->ext.p_glClipControl( origin, depth );
}

static void WINAPI glClipPlanefOES( GLenum plane, const GLfloat *equation )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", plane, equation );
  funcs->ext.p_glClipPlanefOES( plane, equation );
}

static void WINAPI glClipPlanexOES( GLenum plane, const GLfixed *equation )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", plane, equation );
  funcs->ext.p_glClipPlanexOES( plane, equation );
}

static void WINAPI glColor3fVertex3fSUN( GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f, %f, %f, %f, %f)\n", r, g, b, x, y, z );
  funcs->ext.p_glColor3fVertex3fSUN( r, g, b, x, y, z );
}

static void WINAPI glColor3fVertex3fvSUN( const GLfloat *c, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p, %p)\n", c, v );
  funcs->ext.p_glColor3fVertex3fvSUN( c, v );
}

static void WINAPI glColor3hNV( GLhalfNV red, GLhalfNV green, GLhalfNV blue )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", red, green, blue );
  funcs->ext.p_glColor3hNV( red, green, blue );
}

static void WINAPI glColor3hvNV( const GLhalfNV *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glColor3hvNV( v );
}

static void WINAPI glColor3xOES( GLfixed red, GLfixed green, GLfixed blue )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", red, green, blue );
  funcs->ext.p_glColor3xOES( red, green, blue );
}

static void WINAPI glColor3xvOES( const GLfixed *components )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", components );
  funcs->ext.p_glColor3xvOES( components );
}

static void WINAPI glColor4fNormal3fVertex3fSUN( GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f, %f, %f, %f, %f, %f, %f, %f, %f)\n", r, g, b, a, nx, ny, nz, x, y, z );
  funcs->ext.p_glColor4fNormal3fVertex3fSUN( r, g, b, a, nx, ny, nz, x, y, z );
}

static void WINAPI glColor4fNormal3fVertex3fvSUN( const GLfloat *c, const GLfloat *n, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p, %p, %p)\n", c, n, v );
  funcs->ext.p_glColor4fNormal3fVertex3fvSUN( c, n, v );
}

static void WINAPI glColor4hNV( GLhalfNV red, GLhalfNV green, GLhalfNV blue, GLhalfNV alpha )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", red, green, blue, alpha );
  funcs->ext.p_glColor4hNV( red, green, blue, alpha );
}

static void WINAPI glColor4hvNV( const GLhalfNV *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glColor4hvNV( v );
}

static void WINAPI glColor4ubVertex2fSUN( GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %f, %f)\n", r, g, b, a, x, y );
  funcs->ext.p_glColor4ubVertex2fSUN( r, g, b, a, x, y );
}

static void WINAPI glColor4ubVertex2fvSUN( const GLubyte *c, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p, %p)\n", c, v );
  funcs->ext.p_glColor4ubVertex2fvSUN( c, v );
}

static void WINAPI glColor4ubVertex3fSUN( GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %f, %f, %f)\n", r, g, b, a, x, y, z );
  funcs->ext.p_glColor4ubVertex3fSUN( r, g, b, a, x, y, z );
}

static void WINAPI glColor4ubVertex3fvSUN( const GLubyte *c, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p, %p)\n", c, v );
  funcs->ext.p_glColor4ubVertex3fvSUN( c, v );
}

static void WINAPI glColor4xOES( GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", red, green, blue, alpha );
  funcs->ext.p_glColor4xOES( red, green, blue, alpha );
}

static void WINAPI glColor4xvOES( const GLfixed *components )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", components );
  funcs->ext.p_glColor4xvOES( components );
}

static void WINAPI glColorFormatNV( GLint size, GLenum type, GLsizei stride )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", size, type, stride );
  funcs->ext.p_glColorFormatNV( size, type, stride );
}

static void WINAPI glColorFragmentOp1ATI( GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d)\n", op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod );
  funcs->ext.p_glColorFragmentOp1ATI( op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod );
}

static void WINAPI glColorFragmentOp2ATI( GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod );
  funcs->ext.p_glColorFragmentOp2ATI( op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod );
}

static void WINAPI glColorFragmentOp3ATI( GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod );
  funcs->ext.p_glColorFragmentOp3ATI( op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod );
}

static void WINAPI glColorMaskIndexedEXT( GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", index, r, g, b, a );
  funcs->ext.p_glColorMaskIndexedEXT( index, r, g, b, a );
}

static void WINAPI glColorMaski( GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", index, r, g, b, a );
  funcs->ext.p_glColorMaski( index, r, g, b, a );
}

static void WINAPI glColorP3ui( GLenum type, GLuint color )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", type, color );
  funcs->ext.p_glColorP3ui( type, color );
}

static void WINAPI glColorP3uiv( GLenum type, const GLuint *color )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", type, color );
  funcs->ext.p_glColorP3uiv( type, color );
}

static void WINAPI glColorP4ui( GLenum type, GLuint color )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", type, color );
  funcs->ext.p_glColorP4ui( type, color );
}

static void WINAPI glColorP4uiv( GLenum type, const GLuint *color )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", type, color );
  funcs->ext.p_glColorP4uiv( type, color );
}

static void WINAPI glColorPointerEXT( GLint size, GLenum type, GLsizei stride, GLsizei count, const void *pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", size, type, stride, count, pointer );
  funcs->ext.p_glColorPointerEXT( size, type, stride, count, pointer );
}

static void WINAPI glColorPointerListIBM( GLint size, GLenum type, GLint stride, const void **pointer, GLint ptrstride )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p, %d)\n", size, type, stride, pointer, ptrstride );
  funcs->ext.p_glColorPointerListIBM( size, type, stride, pointer, ptrstride );
}

static void WINAPI glColorPointervINTEL( GLint size, GLenum type, const void **pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", size, type, pointer );
  funcs->ext.p_glColorPointervINTEL( size, type, pointer );
}

static void WINAPI glColorSubTable( GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const void *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %p)\n", target, start, count, format, type, data );
  funcs->ext.p_glColorSubTable( target, start, count, format, type, data );
}

static void WINAPI glColorSubTableEXT( GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const void *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %p)\n", target, start, count, format, type, data );
  funcs->ext.p_glColorSubTableEXT( target, start, count, format, type, data );
}

static void WINAPI glColorTable( GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const void *table )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %p)\n", target, internalformat, width, format, type, table );
  funcs->ext.p_glColorTable( target, internalformat, width, format, type, table );
}

static void WINAPI glColorTableEXT( GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type, const void *table )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %p)\n", target, internalFormat, width, format, type, table );
  funcs->ext.p_glColorTableEXT( target, internalFormat, width, format, type, table );
}

static void WINAPI glColorTableParameterfv( GLenum target, GLenum pname, const GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glColorTableParameterfv( target, pname, params );
}

static void WINAPI glColorTableParameterfvSGI( GLenum target, GLenum pname, const GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glColorTableParameterfvSGI( target, pname, params );
}

static void WINAPI glColorTableParameteriv( GLenum target, GLenum pname, const GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glColorTableParameteriv( target, pname, params );
}

static void WINAPI glColorTableParameterivSGI( GLenum target, GLenum pname, const GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glColorTableParameterivSGI( target, pname, params );
}

static void WINAPI glColorTableSGI( GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const void *table )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %p)\n", target, internalformat, width, format, type, table );
  funcs->ext.p_glColorTableSGI( target, internalformat, width, format, type, table );
}

static void WINAPI glCombinerInputNV( GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d)\n", stage, portion, variable, input, mapping, componentUsage );
  funcs->ext.p_glCombinerInputNV( stage, portion, variable, input, mapping, componentUsage );
}

static void WINAPI glCombinerOutputNV( GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct, GLboolean cdDotProduct, GLboolean muxSum )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", stage, portion, abOutput, cdOutput, sumOutput, scale, bias, abDotProduct, cdDotProduct, muxSum );
  funcs->ext.p_glCombinerOutputNV( stage, portion, abOutput, cdOutput, sumOutput, scale, bias, abDotProduct, cdDotProduct, muxSum );
}

static void WINAPI glCombinerParameterfNV( GLenum pname, GLfloat param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f)\n", pname, param );
  funcs->ext.p_glCombinerParameterfNV( pname, param );
}

static void WINAPI glCombinerParameterfvNV( GLenum pname, const GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", pname, params );
  funcs->ext.p_glCombinerParameterfvNV( pname, params );
}

static void WINAPI glCombinerParameteriNV( GLenum pname, GLint param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", pname, param );
  funcs->ext.p_glCombinerParameteriNV( pname, param );
}

static void WINAPI glCombinerParameterivNV( GLenum pname, const GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", pname, params );
  funcs->ext.p_glCombinerParameterivNV( pname, params );
}

static void WINAPI glCombinerStageParameterfvNV( GLenum stage, GLenum pname, const GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", stage, pname, params );
  funcs->ext.p_glCombinerStageParameterfvNV( stage, pname, params );
}

static void WINAPI glCommandListSegmentsNV( GLuint list, GLuint segments )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", list, segments );
  funcs->ext.p_glCommandListSegmentsNV( list, segments );
}

static void WINAPI glCompileCommandListNV( GLuint list )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", list );
  funcs->ext.p_glCompileCommandListNV( list );
}

static void WINAPI glCompileShader( GLuint shader )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", shader );
  funcs->ext.p_glCompileShader( shader );
}

static void WINAPI glCompileShaderARB( GLhandleARB shaderObj )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", shaderObj );
  funcs->ext.p_glCompileShaderARB( shaderObj );
}

static void WINAPI glCompileShaderIncludeARB( GLuint shader, GLsizei count, const GLchar *const*path, const GLint *length )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %p)\n", shader, count, path, length );
  funcs->ext.p_glCompileShaderIncludeARB( shader, count, path, length );
}

static void WINAPI glCompressedMultiTexImage1DEXT( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *bits )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %p)\n", texunit, target, level, internalformat, width, border, imageSize, bits );
  funcs->ext.p_glCompressedMultiTexImage1DEXT( texunit, target, level, internalformat, width, border, imageSize, bits );
}

static void WINAPI glCompressedMultiTexImage2DEXT( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *bits )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texunit, target, level, internalformat, width, height, border, imageSize, bits );
  funcs->ext.p_glCompressedMultiTexImage2DEXT( texunit, target, level, internalformat, width, height, border, imageSize, bits );
}

static void WINAPI glCompressedMultiTexImage3DEXT( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *bits )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texunit, target, level, internalformat, width, height, depth, border, imageSize, bits );
  funcs->ext.p_glCompressedMultiTexImage3DEXT( texunit, target, level, internalformat, width, height, depth, border, imageSize, bits );
}

static void WINAPI glCompressedMultiTexSubImage1DEXT( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *bits )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %p)\n", texunit, target, level, xoffset, width, format, imageSize, bits );
  funcs->ext.p_glCompressedMultiTexSubImage1DEXT( texunit, target, level, xoffset, width, format, imageSize, bits );
}

static void WINAPI glCompressedMultiTexSubImage2DEXT( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *bits )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texunit, target, level, xoffset, yoffset, width, height, format, imageSize, bits );
  funcs->ext.p_glCompressedMultiTexSubImage2DEXT( texunit, target, level, xoffset, yoffset, width, height, format, imageSize, bits );
}

static void WINAPI glCompressedMultiTexSubImage3DEXT( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *bits )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texunit, target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, bits );
  funcs->ext.p_glCompressedMultiTexSubImage3DEXT( texunit, target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, bits );
}

static void WINAPI glCompressedTexImage1D( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, border, imageSize, data );
  funcs->ext.p_glCompressedTexImage1D( target, level, internalformat, width, border, imageSize, data );
}

static void WINAPI glCompressedTexImage1DARB( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, border, imageSize, data );
  funcs->ext.p_glCompressedTexImage1DARB( target, level, internalformat, width, border, imageSize, data );
}

static void WINAPI glCompressedTexImage2D( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, height, border, imageSize, data );
  funcs->ext.p_glCompressedTexImage2D( target, level, internalformat, width, height, border, imageSize, data );
}

static void WINAPI glCompressedTexImage2DARB( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, height, border, imageSize, data );
  funcs->ext.p_glCompressedTexImage2DARB( target, level, internalformat, width, height, border, imageSize, data );
}

static void WINAPI glCompressedTexImage3D( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, height, depth, border, imageSize, data );
  funcs->ext.p_glCompressedTexImage3D( target, level, internalformat, width, height, depth, border, imageSize, data );
}

static void WINAPI glCompressedTexImage3DARB( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, height, depth, border, imageSize, data );
  funcs->ext.p_glCompressedTexImage3DARB( target, level, internalformat, width, height, depth, border, imageSize, data );
}

static void WINAPI glCompressedTexSubImage1D( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, width, format, imageSize, data );
  funcs->ext.p_glCompressedTexSubImage1D( target, level, xoffset, width, format, imageSize, data );
}

static void WINAPI glCompressedTexSubImage1DARB( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, width, format, imageSize, data );
  funcs->ext.p_glCompressedTexSubImage1DARB( target, level, xoffset, width, format, imageSize, data );
}

static void WINAPI glCompressedTexSubImage2D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, width, height, format, imageSize, data );
  funcs->ext.p_glCompressedTexSubImage2D( target, level, xoffset, yoffset, width, height, format, imageSize, data );
}

static void WINAPI glCompressedTexSubImage2DARB( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, width, height, format, imageSize, data );
  funcs->ext.p_glCompressedTexSubImage2DARB( target, level, xoffset, yoffset, width, height, format, imageSize, data );
}

static void WINAPI glCompressedTexSubImage3D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data );
  funcs->ext.p_glCompressedTexSubImage3D( target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data );
}

static void WINAPI glCompressedTexSubImage3DARB( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data );
  funcs->ext.p_glCompressedTexSubImage3DARB( target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data );
}

static void WINAPI glCompressedTextureImage1DEXT( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *bits )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %p)\n", texture, target, level, internalformat, width, border, imageSize, bits );
  funcs->ext.p_glCompressedTextureImage1DEXT( texture, target, level, internalformat, width, border, imageSize, bits );
}

static void WINAPI glCompressedTextureImage2DEXT( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *bits )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texture, target, level, internalformat, width, height, border, imageSize, bits );
  funcs->ext.p_glCompressedTextureImage2DEXT( texture, target, level, internalformat, width, height, border, imageSize, bits );
}

static void WINAPI glCompressedTextureImage3DEXT( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *bits )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texture, target, level, internalformat, width, height, depth, border, imageSize, bits );
  funcs->ext.p_glCompressedTextureImage3DEXT( texture, target, level, internalformat, width, height, depth, border, imageSize, bits );
}

static void WINAPI glCompressedTextureSubImage1D( GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %p)\n", texture, level, xoffset, width, format, imageSize, data );
  funcs->ext.p_glCompressedTextureSubImage1D( texture, level, xoffset, width, format, imageSize, data );
}

static void WINAPI glCompressedTextureSubImage1DEXT( GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *bits )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %p)\n", texture, target, level, xoffset, width, format, imageSize, bits );
  funcs->ext.p_glCompressedTextureSubImage1DEXT( texture, target, level, xoffset, width, format, imageSize, bits );
}

static void WINAPI glCompressedTextureSubImage2D( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texture, level, xoffset, yoffset, width, height, format, imageSize, data );
  funcs->ext.p_glCompressedTextureSubImage2D( texture, level, xoffset, yoffset, width, height, format, imageSize, data );
}

static void WINAPI glCompressedTextureSubImage2DEXT( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *bits )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texture, target, level, xoffset, yoffset, width, height, format, imageSize, bits );
  funcs->ext.p_glCompressedTextureSubImage2DEXT( texture, target, level, xoffset, yoffset, width, height, format, imageSize, bits );
}

static void WINAPI glCompressedTextureSubImage3D( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texture, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data );
  funcs->ext.p_glCompressedTextureSubImage3D( texture, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data );
}

static void WINAPI glCompressedTextureSubImage3DEXT( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *bits )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texture, target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, bits );
  funcs->ext.p_glCompressedTextureSubImage3DEXT( texture, target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, bits );
}

static void WINAPI glConservativeRasterParameterfNV( GLenum pname, GLfloat value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f)\n", pname, value );
  funcs->ext.p_glConservativeRasterParameterfNV( pname, value );
}

static void WINAPI glConservativeRasterParameteriNV( GLenum pname, GLint param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", pname, param );
  funcs->ext.p_glConservativeRasterParameteriNV( pname, param );
}

static void WINAPI glConvolutionFilter1D( GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const void *image )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %p)\n", target, internalformat, width, format, type, image );
  funcs->ext.p_glConvolutionFilter1D( target, internalformat, width, format, type, image );
}

static void WINAPI glConvolutionFilter1DEXT( GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const void *image )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %p)\n", target, internalformat, width, format, type, image );
  funcs->ext.p_glConvolutionFilter1DEXT( target, internalformat, width, format, type, image );
}

static void WINAPI glConvolutionFilter2D( GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *image )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %p)\n", target, internalformat, width, height, format, type, image );
  funcs->ext.p_glConvolutionFilter2D( target, internalformat, width, height, format, type, image );
}

static void WINAPI glConvolutionFilter2DEXT( GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *image )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %p)\n", target, internalformat, width, height, format, type, image );
  funcs->ext.p_glConvolutionFilter2DEXT( target, internalformat, width, height, format, type, image );
}

static void WINAPI glConvolutionParameterf( GLenum target, GLenum pname, GLfloat params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %f)\n", target, pname, params );
  funcs->ext.p_glConvolutionParameterf( target, pname, params );
}

static void WINAPI glConvolutionParameterfEXT( GLenum target, GLenum pname, GLfloat params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %f)\n", target, pname, params );
  funcs->ext.p_glConvolutionParameterfEXT( target, pname, params );
}

static void WINAPI glConvolutionParameterfv( GLenum target, GLenum pname, const GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glConvolutionParameterfv( target, pname, params );
}

static void WINAPI glConvolutionParameterfvEXT( GLenum target, GLenum pname, const GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glConvolutionParameterfvEXT( target, pname, params );
}

static void WINAPI glConvolutionParameteri( GLenum target, GLenum pname, GLint params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", target, pname, params );
  funcs->ext.p_glConvolutionParameteri( target, pname, params );
}

static void WINAPI glConvolutionParameteriEXT( GLenum target, GLenum pname, GLint params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", target, pname, params );
  funcs->ext.p_glConvolutionParameteriEXT( target, pname, params );
}

static void WINAPI glConvolutionParameteriv( GLenum target, GLenum pname, const GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glConvolutionParameteriv( target, pname, params );
}

static void WINAPI glConvolutionParameterivEXT( GLenum target, GLenum pname, const GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glConvolutionParameterivEXT( target, pname, params );
}

static void WINAPI glConvolutionParameterxOES( GLenum target, GLenum pname, GLfixed param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", target, pname, param );
  funcs->ext.p_glConvolutionParameterxOES( target, pname, param );
}

static void WINAPI glConvolutionParameterxvOES( GLenum target, GLenum pname, const GLfixed *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glConvolutionParameterxvOES( target, pname, params );
}

static void WINAPI glCopyBufferSubData( GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %ld, %ld, %ld)\n", readTarget, writeTarget, readOffset, writeOffset, size );
  funcs->ext.p_glCopyBufferSubData( readTarget, writeTarget, readOffset, writeOffset, size );
}

static void WINAPI glCopyColorSubTable( GLenum target, GLsizei start, GLint x, GLint y, GLsizei width )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", target, start, x, y, width );
  funcs->ext.p_glCopyColorSubTable( target, start, x, y, width );
}

static void WINAPI glCopyColorSubTableEXT( GLenum target, GLsizei start, GLint x, GLint y, GLsizei width )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", target, start, x, y, width );
  funcs->ext.p_glCopyColorSubTableEXT( target, start, x, y, width );
}

static void WINAPI glCopyColorTable( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", target, internalformat, x, y, width );
  funcs->ext.p_glCopyColorTable( target, internalformat, x, y, width );
}

static void WINAPI glCopyColorTableSGI( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", target, internalformat, x, y, width );
  funcs->ext.p_glCopyColorTableSGI( target, internalformat, x, y, width );
}

static void WINAPI glCopyConvolutionFilter1D( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", target, internalformat, x, y, width );
  funcs->ext.p_glCopyConvolutionFilter1D( target, internalformat, x, y, width );
}

static void WINAPI glCopyConvolutionFilter1DEXT( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", target, internalformat, x, y, width );
  funcs->ext.p_glCopyConvolutionFilter1DEXT( target, internalformat, x, y, width );
}

static void WINAPI glCopyConvolutionFilter2D( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d)\n", target, internalformat, x, y, width, height );
  funcs->ext.p_glCopyConvolutionFilter2D( target, internalformat, x, y, width, height );
}

static void WINAPI glCopyConvolutionFilter2DEXT( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d)\n", target, internalformat, x, y, width, height );
  funcs->ext.p_glCopyConvolutionFilter2DEXT( target, internalformat, x, y, width, height );
}

static void WINAPI glCopyImageSubData( GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", srcName, srcTarget, srcLevel, srcX, srcY, srcZ, dstName, dstTarget, dstLevel, dstX, dstY, dstZ, srcWidth, srcHeight, srcDepth );
  funcs->ext.p_glCopyImageSubData( srcName, srcTarget, srcLevel, srcX, srcY, srcZ, dstName, dstTarget, dstLevel, dstX, dstY, dstZ, srcWidth, srcHeight, srcDepth );
}

static void WINAPI glCopyImageSubDataNV( GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei width, GLsizei height, GLsizei depth )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", srcName, srcTarget, srcLevel, srcX, srcY, srcZ, dstName, dstTarget, dstLevel, dstX, dstY, dstZ, width, height, depth );
  funcs->ext.p_glCopyImageSubDataNV( srcName, srcTarget, srcLevel, srcX, srcY, srcZ, dstName, dstTarget, dstLevel, dstX, dstY, dstZ, width, height, depth );
}

static void WINAPI glCopyMultiTexImage1DEXT( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d)\n", texunit, target, level, internalformat, x, y, width, border );
  funcs->ext.p_glCopyMultiTexImage1DEXT( texunit, target, level, internalformat, x, y, width, border );
}

static void WINAPI glCopyMultiTexImage2DEXT( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d)\n", texunit, target, level, internalformat, x, y, width, height, border );
  funcs->ext.p_glCopyMultiTexImage2DEXT( texunit, target, level, internalformat, x, y, width, height, border );
}

static void WINAPI glCopyMultiTexSubImage1DEXT( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d)\n", texunit, target, level, xoffset, x, y, width );
  funcs->ext.p_glCopyMultiTexSubImage1DEXT( texunit, target, level, xoffset, x, y, width );
}

static void WINAPI glCopyMultiTexSubImage2DEXT( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d)\n", texunit, target, level, xoffset, yoffset, x, y, width, height );
  funcs->ext.p_glCopyMultiTexSubImage2DEXT( texunit, target, level, xoffset, yoffset, x, y, width, height );
}

static void WINAPI glCopyMultiTexSubImage3DEXT( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", texunit, target, level, xoffset, yoffset, zoffset, x, y, width, height );
  funcs->ext.p_glCopyMultiTexSubImage3DEXT( texunit, target, level, xoffset, yoffset, zoffset, x, y, width, height );
}

static void WINAPI glCopyNamedBufferSubData( GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %ld, %ld, %ld)\n", readBuffer, writeBuffer, readOffset, writeOffset, size );
  funcs->ext.p_glCopyNamedBufferSubData( readBuffer, writeBuffer, readOffset, writeOffset, size );
}

static void WINAPI glCopyPathNV( GLuint resultPath, GLuint srcPath )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", resultPath, srcPath );
  funcs->ext.p_glCopyPathNV( resultPath, srcPath );
}

static void WINAPI glCopyTexImage1DEXT( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d)\n", target, level, internalformat, x, y, width, border );
  funcs->ext.p_glCopyTexImage1DEXT( target, level, internalformat, x, y, width, border );
}

static void WINAPI glCopyTexImage2DEXT( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d)\n", target, level, internalformat, x, y, width, height, border );
  funcs->ext.p_glCopyTexImage2DEXT( target, level, internalformat, x, y, width, height, border );
}

static void WINAPI glCopyTexSubImage1DEXT( GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d)\n", target, level, xoffset, x, y, width );
  funcs->ext.p_glCopyTexSubImage1DEXT( target, level, xoffset, x, y, width );
}

static void WINAPI glCopyTexSubImage2DEXT( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d)\n", target, level, xoffset, yoffset, x, y, width, height );
  funcs->ext.p_glCopyTexSubImage2DEXT( target, level, xoffset, yoffset, x, y, width, height );
}

static void WINAPI glCopyTexSubImage3D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d)\n", target, level, xoffset, yoffset, zoffset, x, y, width, height );
  funcs->ext.p_glCopyTexSubImage3D( target, level, xoffset, yoffset, zoffset, x, y, width, height );
}

static void WINAPI glCopyTexSubImage3DEXT( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d)\n", target, level, xoffset, yoffset, zoffset, x, y, width, height );
  funcs->ext.p_glCopyTexSubImage3DEXT( target, level, xoffset, yoffset, zoffset, x, y, width, height );
}

static void WINAPI glCopyTextureImage1DEXT( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d)\n", texture, target, level, internalformat, x, y, width, border );
  funcs->ext.p_glCopyTextureImage1DEXT( texture, target, level, internalformat, x, y, width, border );
}

static void WINAPI glCopyTextureImage2DEXT( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d)\n", texture, target, level, internalformat, x, y, width, height, border );
  funcs->ext.p_glCopyTextureImage2DEXT( texture, target, level, internalformat, x, y, width, height, border );
}

static void WINAPI glCopyTextureSubImage1D( GLuint texture, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d)\n", texture, level, xoffset, x, y, width );
  funcs->ext.p_glCopyTextureSubImage1D( texture, level, xoffset, x, y, width );
}

static void WINAPI glCopyTextureSubImage1DEXT( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d)\n", texture, target, level, xoffset, x, y, width );
  funcs->ext.p_glCopyTextureSubImage1DEXT( texture, target, level, xoffset, x, y, width );
}

static void WINAPI glCopyTextureSubImage2D( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d)\n", texture, level, xoffset, yoffset, x, y, width, height );
  funcs->ext.p_glCopyTextureSubImage2D( texture, level, xoffset, yoffset, x, y, width, height );
}

static void WINAPI glCopyTextureSubImage2DEXT( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d)\n", texture, target, level, xoffset, yoffset, x, y, width, height );
  funcs->ext.p_glCopyTextureSubImage2DEXT( texture, target, level, xoffset, yoffset, x, y, width, height );
}

static void WINAPI glCopyTextureSubImage3D( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d)\n", texture, level, xoffset, yoffset, zoffset, x, y, width, height );
  funcs->ext.p_glCopyTextureSubImage3D( texture, level, xoffset, yoffset, zoffset, x, y, width, height );
}

static void WINAPI glCopyTextureSubImage3DEXT( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", texture, target, level, xoffset, yoffset, zoffset, x, y, width, height );
  funcs->ext.p_glCopyTextureSubImage3DEXT( texture, target, level, xoffset, yoffset, zoffset, x, y, width, height );
}

static void WINAPI glCoverFillPathInstancedNV( GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum coverMode, GLenum transformType, const GLfloat *transformValues )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %d, %d, %d, %p)\n", numPaths, pathNameType, paths, pathBase, coverMode, transformType, transformValues );
  funcs->ext.p_glCoverFillPathInstancedNV( numPaths, pathNameType, paths, pathBase, coverMode, transformType, transformValues );
}

static void WINAPI glCoverFillPathNV( GLuint path, GLenum coverMode )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", path, coverMode );
  funcs->ext.p_glCoverFillPathNV( path, coverMode );
}

static void WINAPI glCoverStrokePathInstancedNV( GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum coverMode, GLenum transformType, const GLfloat *transformValues )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %d, %d, %d, %p)\n", numPaths, pathNameType, paths, pathBase, coverMode, transformType, transformValues );
  funcs->ext.p_glCoverStrokePathInstancedNV( numPaths, pathNameType, paths, pathBase, coverMode, transformType, transformValues );
}

static void WINAPI glCoverStrokePathNV( GLuint path, GLenum coverMode )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", path, coverMode );
  funcs->ext.p_glCoverStrokePathNV( path, coverMode );
}

static void WINAPI glCoverageModulationNV( GLenum components )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", components );
  funcs->ext.p_glCoverageModulationNV( components );
}

static void WINAPI glCoverageModulationTableNV( GLsizei n, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, v );
  funcs->ext.p_glCoverageModulationTableNV( n, v );
}

static void WINAPI glCreateBuffers( GLsizei n, GLuint *buffers )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, buffers );
  funcs->ext.p_glCreateBuffers( n, buffers );
}

static void WINAPI glCreateCommandListsNV( GLsizei n, GLuint *lists )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, lists );
  funcs->ext.p_glCreateCommandListsNV( n, lists );
}

static void WINAPI glCreateFramebuffers( GLsizei n, GLuint *framebuffers )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, framebuffers );
  funcs->ext.p_glCreateFramebuffers( n, framebuffers );
}

static void WINAPI glCreateMemoryObjectsEXT( GLsizei n, GLuint *memoryObjects )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, memoryObjects );
  funcs->ext.p_glCreateMemoryObjectsEXT( n, memoryObjects );
}

static void WINAPI glCreatePerfQueryINTEL( GLuint queryId, GLuint *queryHandle )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", queryId, queryHandle );
  funcs->ext.p_glCreatePerfQueryINTEL( queryId, queryHandle );
}

static GLuint WINAPI glCreateProgram(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  return funcs->ext.p_glCreateProgram();
}

static GLhandleARB WINAPI glCreateProgramObjectARB(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  return funcs->ext.p_glCreateProgramObjectARB();
}

static void WINAPI glCreateProgramPipelines( GLsizei n, GLuint *pipelines )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, pipelines );
  funcs->ext.p_glCreateProgramPipelines( n, pipelines );
}

static void WINAPI glCreateQueries( GLenum target, GLsizei n, GLuint *ids )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, n, ids );
  funcs->ext.p_glCreateQueries( target, n, ids );
}

static void WINAPI glCreateRenderbuffers( GLsizei n, GLuint *renderbuffers )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, renderbuffers );
  funcs->ext.p_glCreateRenderbuffers( n, renderbuffers );
}

static void WINAPI glCreateSamplers( GLsizei n, GLuint *samplers )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, samplers );
  funcs->ext.p_glCreateSamplers( n, samplers );
}

static GLuint WINAPI glCreateShader( GLenum type )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", type );
  return funcs->ext.p_glCreateShader( type );
}

static GLhandleARB WINAPI glCreateShaderObjectARB( GLenum shaderType )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", shaderType );
  return funcs->ext.p_glCreateShaderObjectARB( shaderType );
}

static GLuint WINAPI glCreateShaderProgramEXT( GLenum type, const GLchar *string )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", type, string );
  return funcs->ext.p_glCreateShaderProgramEXT( type, string );
}

static GLuint WINAPI glCreateShaderProgramv( GLenum type, GLsizei count, const GLchar *const*strings )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", type, count, strings );
  return funcs->ext.p_glCreateShaderProgramv( type, count, strings );
}

static void WINAPI glCreateStatesNV( GLsizei n, GLuint *states )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, states );
  funcs->ext.p_glCreateStatesNV( n, states );
}

static GLsync WINAPI glCreateSyncFromCLeventARB( struct _cl_context *context, struct _cl_event *event, GLbitfield flags )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p, %p, %d)\n", context, event, flags );
  return funcs->ext.p_glCreateSyncFromCLeventARB( context, event, flags );
}

static void WINAPI glCreateTextures( GLenum target, GLsizei n, GLuint *textures )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, n, textures );
  funcs->ext.p_glCreateTextures( target, n, textures );
}

static void WINAPI glCreateTransformFeedbacks( GLsizei n, GLuint *ids )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, ids );
  funcs->ext.p_glCreateTransformFeedbacks( n, ids );
}

static void WINAPI glCreateVertexArrays( GLsizei n, GLuint *arrays )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, arrays );
  funcs->ext.p_glCreateVertexArrays( n, arrays );
}

static void WINAPI glCullParameterdvEXT( GLenum pname, GLdouble *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", pname, params );
  funcs->ext.p_glCullParameterdvEXT( pname, params );
}

static void WINAPI glCullParameterfvEXT( GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", pname, params );
  funcs->ext.p_glCullParameterfvEXT( pname, params );
}

static void WINAPI glCurrentPaletteMatrixARB( GLint index )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", index );
  funcs->ext.p_glCurrentPaletteMatrixARB( index );
}

static void WINAPI glDebugMessageControl( GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p, %d)\n", source, type, severity, count, ids, enabled );
  funcs->ext.p_glDebugMessageControl( source, type, severity, count, ids, enabled );
}

static void WINAPI glDebugMessageControlARB( GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p, %d)\n", source, type, severity, count, ids, enabled );
  funcs->ext.p_glDebugMessageControlARB( source, type, severity, count, ids, enabled );
}

static void WINAPI glDebugMessageEnableAMD( GLenum category, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p, %d)\n", category, severity, count, ids, enabled );
  funcs->ext.p_glDebugMessageEnableAMD( category, severity, count, ids, enabled );
}

static void WINAPI glDebugMessageInsert( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *buf )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %p)\n", source, type, id, severity, length, buf );
  funcs->ext.p_glDebugMessageInsert( source, type, id, severity, length, buf );
}

static void WINAPI glDebugMessageInsertAMD( GLenum category, GLenum severity, GLuint id, GLsizei length, const GLchar *buf )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", category, severity, id, length, buf );
  funcs->ext.p_glDebugMessageInsertAMD( category, severity, id, length, buf );
}

static void WINAPI glDebugMessageInsertARB( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *buf )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %p)\n", source, type, id, severity, length, buf );
  funcs->ext.p_glDebugMessageInsertARB( source, type, id, severity, length, buf );
}

static void WINAPI glDeformSGIX( GLbitfield mask )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", mask );
  funcs->ext.p_glDeformSGIX( mask );
}

static void WINAPI glDeformationMap3dSGIX( GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, GLdouble w1, GLdouble w2, GLint wstride, GLint worder, const GLdouble *points )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %d, %d, %f, %f, %d, %d, %f, %f, %d, %d, %p)\n", target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points );
  funcs->ext.p_glDeformationMap3dSGIX( target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points );
}

static void WINAPI glDeformationMap3fSGIX( GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, GLfloat w1, GLfloat w2, GLint wstride, GLint worder, const GLfloat *points )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %d, %d, %f, %f, %d, %d, %f, %f, %d, %d, %p)\n", target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points );
  funcs->ext.p_glDeformationMap3fSGIX( target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points );
}

static void WINAPI glDeleteAsyncMarkersSGIX( GLuint marker, GLsizei range )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", marker, range );
  funcs->ext.p_glDeleteAsyncMarkersSGIX( marker, range );
}

static void WINAPI glDeleteBufferRegion( GLenum region )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", region );
  funcs->ext.p_glDeleteBufferRegion( region );
}

static void WINAPI glDeleteBuffers( GLsizei n, const GLuint *buffers )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, buffers );
  funcs->ext.p_glDeleteBuffers( n, buffers );
}

static void WINAPI glDeleteBuffersARB( GLsizei n, const GLuint *buffers )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, buffers );
  funcs->ext.p_glDeleteBuffersARB( n, buffers );
}

static void WINAPI glDeleteCommandListsNV( GLsizei n, const GLuint *lists )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, lists );
  funcs->ext.p_glDeleteCommandListsNV( n, lists );
}

static void WINAPI glDeleteFencesAPPLE( GLsizei n, const GLuint *fences )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, fences );
  funcs->ext.p_glDeleteFencesAPPLE( n, fences );
}

static void WINAPI glDeleteFencesNV( GLsizei n, const GLuint *fences )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, fences );
  funcs->ext.p_glDeleteFencesNV( n, fences );
}

static void WINAPI glDeleteFragmentShaderATI( GLuint id )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", id );
  funcs->ext.p_glDeleteFragmentShaderATI( id );
}

static void WINAPI glDeleteFramebuffers( GLsizei n, const GLuint *framebuffers )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, framebuffers );
  funcs->ext.p_glDeleteFramebuffers( n, framebuffers );
}

static void WINAPI glDeleteFramebuffersEXT( GLsizei n, const GLuint *framebuffers )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, framebuffers );
  funcs->ext.p_glDeleteFramebuffersEXT( n, framebuffers );
}

static void WINAPI glDeleteMemoryObjectsEXT( GLsizei n, const GLuint *memoryObjects )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, memoryObjects );
  funcs->ext.p_glDeleteMemoryObjectsEXT( n, memoryObjects );
}

static void WINAPI glDeleteNamedStringARB( GLint namelen, const GLchar *name )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", namelen, name );
  funcs->ext.p_glDeleteNamedStringARB( namelen, name );
}

static void WINAPI glDeleteNamesAMD( GLenum identifier, GLuint num, const GLuint *names )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", identifier, num, names );
  funcs->ext.p_glDeleteNamesAMD( identifier, num, names );
}

static void WINAPI glDeleteObjectARB( GLhandleARB obj )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", obj );
  funcs->ext.p_glDeleteObjectARB( obj );
}

static void WINAPI glDeleteObjectBufferATI( GLuint buffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", buffer );
  funcs->ext.p_glDeleteObjectBufferATI( buffer );
}

static void WINAPI glDeleteOcclusionQueriesNV( GLsizei n, const GLuint *ids )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, ids );
  funcs->ext.p_glDeleteOcclusionQueriesNV( n, ids );
}

static void WINAPI glDeletePathsNV( GLuint path, GLsizei range )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", path, range );
  funcs->ext.p_glDeletePathsNV( path, range );
}

static void WINAPI glDeletePerfMonitorsAMD( GLsizei n, GLuint *monitors )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, monitors );
  funcs->ext.p_glDeletePerfMonitorsAMD( n, monitors );
}

static void WINAPI glDeletePerfQueryINTEL( GLuint queryHandle )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", queryHandle );
  funcs->ext.p_glDeletePerfQueryINTEL( queryHandle );
}

static void WINAPI glDeleteProgram( GLuint program )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", program );
  funcs->ext.p_glDeleteProgram( program );
}

static void WINAPI glDeleteProgramPipelines( GLsizei n, const GLuint *pipelines )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, pipelines );
  funcs->ext.p_glDeleteProgramPipelines( n, pipelines );
}

static void WINAPI glDeleteProgramsARB( GLsizei n, const GLuint *programs )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, programs );
  funcs->ext.p_glDeleteProgramsARB( n, programs );
}

static void WINAPI glDeleteProgramsNV( GLsizei n, const GLuint *programs )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, programs );
  funcs->ext.p_glDeleteProgramsNV( n, programs );
}

static void WINAPI glDeleteQueries( GLsizei n, const GLuint *ids )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, ids );
  funcs->ext.p_glDeleteQueries( n, ids );
}

static void WINAPI glDeleteQueriesARB( GLsizei n, const GLuint *ids )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, ids );
  funcs->ext.p_glDeleteQueriesARB( n, ids );
}

static void WINAPI glDeleteQueryResourceTagNV( GLsizei n, const GLint *tagIds )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, tagIds );
  funcs->ext.p_glDeleteQueryResourceTagNV( n, tagIds );
}

static void WINAPI glDeleteRenderbuffers( GLsizei n, const GLuint *renderbuffers )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, renderbuffers );
  funcs->ext.p_glDeleteRenderbuffers( n, renderbuffers );
}

static void WINAPI glDeleteRenderbuffersEXT( GLsizei n, const GLuint *renderbuffers )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, renderbuffers );
  funcs->ext.p_glDeleteRenderbuffersEXT( n, renderbuffers );
}

static void WINAPI glDeleteSamplers( GLsizei count, const GLuint *samplers )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", count, samplers );
  funcs->ext.p_glDeleteSamplers( count, samplers );
}

static void WINAPI glDeleteSemaphoresEXT( GLsizei n, const GLuint *semaphores )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, semaphores );
  funcs->ext.p_glDeleteSemaphoresEXT( n, semaphores );
}

static void WINAPI glDeleteShader( GLuint shader )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", shader );
  funcs->ext.p_glDeleteShader( shader );
}

static void WINAPI glDeleteStatesNV( GLsizei n, const GLuint *states )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, states );
  funcs->ext.p_glDeleteStatesNV( n, states );
}

static void WINAPI glDeleteSync( GLsync sync )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", sync );
  funcs->ext.p_glDeleteSync( sync );
}

static void WINAPI glDeleteTexturesEXT( GLsizei n, const GLuint *textures )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, textures );
  funcs->ext.p_glDeleteTexturesEXT( n, textures );
}

static void WINAPI glDeleteTransformFeedbacks( GLsizei n, const GLuint *ids )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, ids );
  funcs->ext.p_glDeleteTransformFeedbacks( n, ids );
}

static void WINAPI glDeleteTransformFeedbacksNV( GLsizei n, const GLuint *ids )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, ids );
  funcs->ext.p_glDeleteTransformFeedbacksNV( n, ids );
}

static void WINAPI glDeleteVertexArrays( GLsizei n, const GLuint *arrays )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, arrays );
  funcs->ext.p_glDeleteVertexArrays( n, arrays );
}

static void WINAPI glDeleteVertexArraysAPPLE( GLsizei n, const GLuint *arrays )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, arrays );
  funcs->ext.p_glDeleteVertexArraysAPPLE( n, arrays );
}

static void WINAPI glDeleteVertexShaderEXT( GLuint id )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", id );
  funcs->ext.p_glDeleteVertexShaderEXT( id );
}

static void WINAPI glDepthBoundsEXT( GLclampd zmin, GLclampd zmax )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f)\n", zmin, zmax );
  funcs->ext.p_glDepthBoundsEXT( zmin, zmax );
}

static void WINAPI glDepthBoundsdNV( GLdouble zmin, GLdouble zmax )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f)\n", zmin, zmax );
  funcs->ext.p_glDepthBoundsdNV( zmin, zmax );
}

static void WINAPI glDepthRangeArrayv( GLuint first, GLsizei count, const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", first, count, v );
  funcs->ext.p_glDepthRangeArrayv( first, count, v );
}

static void WINAPI glDepthRangeIndexed( GLuint index, GLdouble n, GLdouble f )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f)\n", index, n, f );
  funcs->ext.p_glDepthRangeIndexed( index, n, f );
}

static void WINAPI glDepthRangedNV( GLdouble zNear, GLdouble zFar )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f)\n", zNear, zFar );
  funcs->ext.p_glDepthRangedNV( zNear, zFar );
}

static void WINAPI glDepthRangef( GLfloat n, GLfloat f )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f)\n", n, f );
  funcs->ext.p_glDepthRangef( n, f );
}

static void WINAPI glDepthRangefOES( GLclampf n, GLclampf f )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f)\n", n, f );
  funcs->ext.p_glDepthRangefOES( n, f );
}

static void WINAPI glDepthRangexOES( GLfixed n, GLfixed f )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", n, f );
  funcs->ext.p_glDepthRangexOES( n, f );
}

static void WINAPI glDetachObjectARB( GLhandleARB containerObj, GLhandleARB attachedObj )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", containerObj, attachedObj );
  funcs->ext.p_glDetachObjectARB( containerObj, attachedObj );
}

static void WINAPI glDetachShader( GLuint program, GLuint shader )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", program, shader );
  funcs->ext.p_glDetachShader( program, shader );
}

static void WINAPI glDetailTexFuncSGIS( GLenum target, GLsizei n, const GLfloat *points )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, n, points );
  funcs->ext.p_glDetailTexFuncSGIS( target, n, points );
}

static void WINAPI glDisableClientStateIndexedEXT( GLenum array, GLuint index )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", array, index );
  funcs->ext.p_glDisableClientStateIndexedEXT( array, index );
}

static void WINAPI glDisableClientStateiEXT( GLenum array, GLuint index )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", array, index );
  funcs->ext.p_glDisableClientStateiEXT( array, index );
}

static void WINAPI glDisableIndexedEXT( GLenum target, GLuint index )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", target, index );
  funcs->ext.p_glDisableIndexedEXT( target, index );
}

static void WINAPI glDisableVariantClientStateEXT( GLuint id )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", id );
  funcs->ext.p_glDisableVariantClientStateEXT( id );
}

static void WINAPI glDisableVertexArrayAttrib( GLuint vaobj, GLuint index )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", vaobj, index );
  funcs->ext.p_glDisableVertexArrayAttrib( vaobj, index );
}

static void WINAPI glDisableVertexArrayAttribEXT( GLuint vaobj, GLuint index )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", vaobj, index );
  funcs->ext.p_glDisableVertexArrayAttribEXT( vaobj, index );
}

static void WINAPI glDisableVertexArrayEXT( GLuint vaobj, GLenum array )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", vaobj, array );
  funcs->ext.p_glDisableVertexArrayEXT( vaobj, array );
}

static void WINAPI glDisableVertexAttribAPPLE( GLuint index, GLenum pname )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", index, pname );
  funcs->ext.p_glDisableVertexAttribAPPLE( index, pname );
}

static void WINAPI glDisableVertexAttribArray( GLuint index )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", index );
  funcs->ext.p_glDisableVertexAttribArray( index );
}

static void WINAPI glDisableVertexAttribArrayARB( GLuint index )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", index );
  funcs->ext.p_glDisableVertexAttribArrayARB( index );
}

static void WINAPI glDisablei( GLenum target, GLuint index )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", target, index );
  funcs->ext.p_glDisablei( target, index );
}

static void WINAPI glDispatchCompute( GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", num_groups_x, num_groups_y, num_groups_z );
  funcs->ext.p_glDispatchCompute( num_groups_x, num_groups_y, num_groups_z );
}

static void WINAPI glDispatchComputeGroupSizeARB( GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z, GLuint group_size_x, GLuint group_size_y, GLuint group_size_z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d)\n", num_groups_x, num_groups_y, num_groups_z, group_size_x, group_size_y, group_size_z );
  funcs->ext.p_glDispatchComputeGroupSizeARB( num_groups_x, num_groups_y, num_groups_z, group_size_x, group_size_y, group_size_z );
}

static void WINAPI glDispatchComputeIndirect( GLintptr indirect )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%ld)\n", indirect );
  funcs->ext.p_glDispatchComputeIndirect( indirect );
}

static void WINAPI glDrawArraysEXT( GLenum mode, GLint first, GLsizei count )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", mode, first, count );
  funcs->ext.p_glDrawArraysEXT( mode, first, count );
}

static void WINAPI glDrawArraysIndirect( GLenum mode, const void *indirect )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", mode, indirect );
  funcs->ext.p_glDrawArraysIndirect( mode, indirect );
}

static void WINAPI glDrawArraysInstanced( GLenum mode, GLint first, GLsizei count, GLsizei instancecount )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", mode, first, count, instancecount );
  funcs->ext.p_glDrawArraysInstanced( mode, first, count, instancecount );
}

static void WINAPI glDrawArraysInstancedARB( GLenum mode, GLint first, GLsizei count, GLsizei primcount )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", mode, first, count, primcount );
  funcs->ext.p_glDrawArraysInstancedARB( mode, first, count, primcount );
}

static void WINAPI glDrawArraysInstancedBaseInstance( GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", mode, first, count, instancecount, baseinstance );
  funcs->ext.p_glDrawArraysInstancedBaseInstance( mode, first, count, instancecount, baseinstance );
}

static void WINAPI glDrawArraysInstancedEXT( GLenum mode, GLint start, GLsizei count, GLsizei primcount )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", mode, start, count, primcount );
  funcs->ext.p_glDrawArraysInstancedEXT( mode, start, count, primcount );
}

static void WINAPI glDrawBufferRegion( GLenum region, GLint x, GLint y, GLsizei width, GLsizei height, GLint xDest, GLint yDest )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d)\n", region, x, y, width, height, xDest, yDest );
  funcs->ext.p_glDrawBufferRegion( region, x, y, width, height, xDest, yDest );
}

static void WINAPI glDrawBuffers( GLsizei n, const GLenum *bufs )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, bufs );
  funcs->ext.p_glDrawBuffers( n, bufs );
}

static void WINAPI glDrawBuffersARB( GLsizei n, const GLenum *bufs )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, bufs );
  funcs->ext.p_glDrawBuffersARB( n, bufs );
}

static void WINAPI glDrawBuffersATI( GLsizei n, const GLenum *bufs )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, bufs );
  funcs->ext.p_glDrawBuffersATI( n, bufs );
}

static void WINAPI glDrawCommandsAddressNV( GLenum primitiveMode, const GLuint64 *indirects, const GLsizei *sizes, GLuint count )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p, %p, %d)\n", primitiveMode, indirects, sizes, count );
  funcs->ext.p_glDrawCommandsAddressNV( primitiveMode, indirects, sizes, count );
}

static void WINAPI glDrawCommandsNV( GLenum primitiveMode, GLuint buffer, const GLintptr *indirects, const GLsizei *sizes, GLuint count )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %p, %d)\n", primitiveMode, buffer, indirects, sizes, count );
  funcs->ext.p_glDrawCommandsNV( primitiveMode, buffer, indirects, sizes, count );
}

static void WINAPI glDrawCommandsStatesAddressNV( const GLuint64 *indirects, const GLsizei *sizes, const GLuint *states, const GLuint *fbos, GLuint count )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p, %p, %p, %p, %d)\n", indirects, sizes, states, fbos, count );
  funcs->ext.p_glDrawCommandsStatesAddressNV( indirects, sizes, states, fbos, count );
}

static void WINAPI glDrawCommandsStatesNV( GLuint buffer, const GLintptr *indirects, const GLsizei *sizes, const GLuint *states, const GLuint *fbos, GLuint count )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p, %p, %p, %p, %d)\n", buffer, indirects, sizes, states, fbos, count );
  funcs->ext.p_glDrawCommandsStatesNV( buffer, indirects, sizes, states, fbos, count );
}

static void WINAPI glDrawElementArrayAPPLE( GLenum mode, GLint first, GLsizei count )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", mode, first, count );
  funcs->ext.p_glDrawElementArrayAPPLE( mode, first, count );
}

static void WINAPI glDrawElementArrayATI( GLenum mode, GLsizei count )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", mode, count );
  funcs->ext.p_glDrawElementArrayATI( mode, count );
}

static void WINAPI glDrawElementsBaseVertex( GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p, %d)\n", mode, count, type, indices, basevertex );
  funcs->ext.p_glDrawElementsBaseVertex( mode, count, type, indices, basevertex );
}

static void WINAPI glDrawElementsIndirect( GLenum mode, GLenum type, const void *indirect )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", mode, type, indirect );
  funcs->ext.p_glDrawElementsIndirect( mode, type, indirect );
}

static void WINAPI glDrawElementsInstanced( GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p, %d)\n", mode, count, type, indices, instancecount );
  funcs->ext.p_glDrawElementsInstanced( mode, count, type, indices, instancecount );
}

static void WINAPI glDrawElementsInstancedARB( GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p, %d)\n", mode, count, type, indices, primcount );
  funcs->ext.p_glDrawElementsInstancedARB( mode, count, type, indices, primcount );
}

static void WINAPI glDrawElementsInstancedBaseInstance( GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLuint baseinstance )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p, %d, %d)\n", mode, count, type, indices, instancecount, baseinstance );
  funcs->ext.p_glDrawElementsInstancedBaseInstance( mode, count, type, indices, instancecount, baseinstance );
}

static void WINAPI glDrawElementsInstancedBaseVertex( GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p, %d, %d)\n", mode, count, type, indices, instancecount, basevertex );
  funcs->ext.p_glDrawElementsInstancedBaseVertex( mode, count, type, indices, instancecount, basevertex );
}

static void WINAPI glDrawElementsInstancedBaseVertexBaseInstance( GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex, GLuint baseinstance )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p, %d, %d, %d)\n", mode, count, type, indices, instancecount, basevertex, baseinstance );
  funcs->ext.p_glDrawElementsInstancedBaseVertexBaseInstance( mode, count, type, indices, instancecount, basevertex, baseinstance );
}

static void WINAPI glDrawElementsInstancedEXT( GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p, %d)\n", mode, count, type, indices, primcount );
  funcs->ext.p_glDrawElementsInstancedEXT( mode, count, type, indices, primcount );
}

static void WINAPI glDrawMeshArraysSUN( GLenum mode, GLint first, GLsizei count, GLsizei width )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", mode, first, count, width );
  funcs->ext.p_glDrawMeshArraysSUN( mode, first, count, width );
}

static void WINAPI glDrawRangeElementArrayAPPLE( GLenum mode, GLuint start, GLuint end, GLint first, GLsizei count )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", mode, start, end, first, count );
  funcs->ext.p_glDrawRangeElementArrayAPPLE( mode, start, end, first, count );
}

static void WINAPI glDrawRangeElementArrayATI( GLenum mode, GLuint start, GLuint end, GLsizei count )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", mode, start, end, count );
  funcs->ext.p_glDrawRangeElementArrayATI( mode, start, end, count );
}

static void WINAPI glDrawRangeElements( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %p)\n", mode, start, end, count, type, indices );
  funcs->ext.p_glDrawRangeElements( mode, start, end, count, type, indices );
}

static void WINAPI glDrawRangeElementsBaseVertex( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices, GLint basevertex )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %p, %d)\n", mode, start, end, count, type, indices, basevertex );
  funcs->ext.p_glDrawRangeElementsBaseVertex( mode, start, end, count, type, indices, basevertex );
}

static void WINAPI glDrawRangeElementsEXT( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %p)\n", mode, start, end, count, type, indices );
  funcs->ext.p_glDrawRangeElementsEXT( mode, start, end, count, type, indices );
}

static void WINAPI glDrawTextureNV( GLuint texture, GLuint sampler, GLfloat x0, GLfloat y0, GLfloat x1, GLfloat y1, GLfloat z, GLfloat s0, GLfloat t0, GLfloat s1, GLfloat t1 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %f, %f, %f, %f, %f, %f, %f, %f, %f)\n", texture, sampler, x0, y0, x1, y1, z, s0, t0, s1, t1 );
  funcs->ext.p_glDrawTextureNV( texture, sampler, x0, y0, x1, y1, z, s0, t0, s1, t1 );
}

static void WINAPI glDrawTransformFeedback( GLenum mode, GLuint id )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", mode, id );
  funcs->ext.p_glDrawTransformFeedback( mode, id );
}

static void WINAPI glDrawTransformFeedbackInstanced( GLenum mode, GLuint id, GLsizei instancecount )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", mode, id, instancecount );
  funcs->ext.p_glDrawTransformFeedbackInstanced( mode, id, instancecount );
}

static void WINAPI glDrawTransformFeedbackNV( GLenum mode, GLuint id )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", mode, id );
  funcs->ext.p_glDrawTransformFeedbackNV( mode, id );
}

static void WINAPI glDrawTransformFeedbackStream( GLenum mode, GLuint id, GLuint stream )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", mode, id, stream );
  funcs->ext.p_glDrawTransformFeedbackStream( mode, id, stream );
}

static void WINAPI glDrawTransformFeedbackStreamInstanced( GLenum mode, GLuint id, GLuint stream, GLsizei instancecount )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", mode, id, stream, instancecount );
  funcs->ext.p_glDrawTransformFeedbackStreamInstanced( mode, id, stream, instancecount );
}

static void WINAPI glDrawVkImageNV( GLuint64 vkImage, GLuint sampler, GLfloat x0, GLfloat y0, GLfloat x1, GLfloat y1, GLfloat z, GLfloat s0, GLfloat t0, GLfloat s1, GLfloat t1 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%s, %d, %f, %f, %f, %f, %f, %f, %f, %f, %f)\n", wine_dbgstr_longlong(vkImage), sampler, x0, y0, x1, y1, z, s0, t0, s1, t1 );
  funcs->ext.p_glDrawVkImageNV( vkImage, sampler, x0, y0, x1, y1, z, s0, t0, s1, t1 );
}

static void WINAPI glEdgeFlagFormatNV( GLsizei stride )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", stride );
  funcs->ext.p_glEdgeFlagFormatNV( stride );
}

static void WINAPI glEdgeFlagPointerEXT( GLsizei stride, GLsizei count, const GLboolean *pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", stride, count, pointer );
  funcs->ext.p_glEdgeFlagPointerEXT( stride, count, pointer );
}

static void WINAPI glEdgeFlagPointerListIBM( GLint stride, const GLboolean **pointer, GLint ptrstride )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p, %d)\n", stride, pointer, ptrstride );
  funcs->ext.p_glEdgeFlagPointerListIBM( stride, pointer, ptrstride );
}

static void WINAPI glElementPointerAPPLE( GLenum type, const void *pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", type, pointer );
  funcs->ext.p_glElementPointerAPPLE( type, pointer );
}

static void WINAPI glElementPointerATI( GLenum type, const void *pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", type, pointer );
  funcs->ext.p_glElementPointerATI( type, pointer );
}

static void WINAPI glEnableClientStateIndexedEXT( GLenum array, GLuint index )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", array, index );
  funcs->ext.p_glEnableClientStateIndexedEXT( array, index );
}

static void WINAPI glEnableClientStateiEXT( GLenum array, GLuint index )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", array, index );
  funcs->ext.p_glEnableClientStateiEXT( array, index );
}

static void WINAPI glEnableIndexedEXT( GLenum target, GLuint index )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", target, index );
  funcs->ext.p_glEnableIndexedEXT( target, index );
}

static void WINAPI glEnableVariantClientStateEXT( GLuint id )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", id );
  funcs->ext.p_glEnableVariantClientStateEXT( id );
}

static void WINAPI glEnableVertexArrayAttrib( GLuint vaobj, GLuint index )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", vaobj, index );
  funcs->ext.p_glEnableVertexArrayAttrib( vaobj, index );
}

static void WINAPI glEnableVertexArrayAttribEXT( GLuint vaobj, GLuint index )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", vaobj, index );
  funcs->ext.p_glEnableVertexArrayAttribEXT( vaobj, index );
}

static void WINAPI glEnableVertexArrayEXT( GLuint vaobj, GLenum array )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", vaobj, array );
  funcs->ext.p_glEnableVertexArrayEXT( vaobj, array );
}

static void WINAPI glEnableVertexAttribAPPLE( GLuint index, GLenum pname )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", index, pname );
  funcs->ext.p_glEnableVertexAttribAPPLE( index, pname );
}

static void WINAPI glEnableVertexAttribArray( GLuint index )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", index );
  funcs->ext.p_glEnableVertexAttribArray( index );
}

static void WINAPI glEnableVertexAttribArrayARB( GLuint index )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", index );
  funcs->ext.p_glEnableVertexAttribArrayARB( index );
}

static void WINAPI glEnablei( GLenum target, GLuint index )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", target, index );
  funcs->ext.p_glEnablei( target, index );
}

static void WINAPI glEndConditionalRender(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  funcs->ext.p_glEndConditionalRender();
}

static void WINAPI glEndConditionalRenderNV(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  funcs->ext.p_glEndConditionalRenderNV();
}

static void WINAPI glEndConditionalRenderNVX(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  funcs->ext.p_glEndConditionalRenderNVX();
}

static void WINAPI glEndFragmentShaderATI(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  funcs->ext.p_glEndFragmentShaderATI();
}

static void WINAPI glEndOcclusionQueryNV(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  funcs->ext.p_glEndOcclusionQueryNV();
}

static void WINAPI glEndPerfMonitorAMD( GLuint monitor )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", monitor );
  funcs->ext.p_glEndPerfMonitorAMD( monitor );
}

static void WINAPI glEndPerfQueryINTEL( GLuint queryHandle )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", queryHandle );
  funcs->ext.p_glEndPerfQueryINTEL( queryHandle );
}

static void WINAPI glEndQuery( GLenum target )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", target );
  funcs->ext.p_glEndQuery( target );
}

static void WINAPI glEndQueryARB( GLenum target )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", target );
  funcs->ext.p_glEndQueryARB( target );
}

static void WINAPI glEndQueryIndexed( GLenum target, GLuint index )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", target, index );
  funcs->ext.p_glEndQueryIndexed( target, index );
}

static void WINAPI glEndTransformFeedback(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  funcs->ext.p_glEndTransformFeedback();
}

static void WINAPI glEndTransformFeedbackEXT(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  funcs->ext.p_glEndTransformFeedbackEXT();
}

static void WINAPI glEndTransformFeedbackNV(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  funcs->ext.p_glEndTransformFeedbackNV();
}

static void WINAPI glEndVertexShaderEXT(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  funcs->ext.p_glEndVertexShaderEXT();
}

static void WINAPI glEndVideoCaptureNV( GLuint video_capture_slot )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", video_capture_slot );
  funcs->ext.p_glEndVideoCaptureNV( video_capture_slot );
}

static void WINAPI glEvalCoord1xOES( GLfixed u )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", u );
  funcs->ext.p_glEvalCoord1xOES( u );
}

static void WINAPI glEvalCoord1xvOES( const GLfixed *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", coords );
  funcs->ext.p_glEvalCoord1xvOES( coords );
}

static void WINAPI glEvalCoord2xOES( GLfixed u, GLfixed v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", u, v );
  funcs->ext.p_glEvalCoord2xOES( u, v );
}

static void WINAPI glEvalCoord2xvOES( const GLfixed *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", coords );
  funcs->ext.p_glEvalCoord2xvOES( coords );
}

static void WINAPI glEvalMapsNV( GLenum target, GLenum mode )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", target, mode );
  funcs->ext.p_glEvalMapsNV( target, mode );
}

static void WINAPI glEvaluateDepthValuesARB(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  funcs->ext.p_glEvaluateDepthValuesARB();
}

static void WINAPI glExecuteProgramNV( GLenum target, GLuint id, const GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, id, params );
  funcs->ext.p_glExecuteProgramNV( target, id, params );
}

static void WINAPI glExtractComponentEXT( GLuint res, GLuint src, GLuint num )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", res, src, num );
  funcs->ext.p_glExtractComponentEXT( res, src, num );
}

static void WINAPI glFeedbackBufferxOES( GLsizei n, GLenum type, const GLfixed *buffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", n, type, buffer );
  funcs->ext.p_glFeedbackBufferxOES( n, type, buffer );
}

static GLsync WINAPI glFenceSync( GLenum condition, GLbitfield flags )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", condition, flags );
  return funcs->ext.p_glFenceSync( condition, flags );
}

static void WINAPI glFinalCombinerInputNV( GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", variable, input, mapping, componentUsage );
  funcs->ext.p_glFinalCombinerInputNV( variable, input, mapping, componentUsage );
}

static GLint WINAPI glFinishAsyncSGIX( GLuint *markerp )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", markerp );
  return funcs->ext.p_glFinishAsyncSGIX( markerp );
}

static void WINAPI glFinishFenceAPPLE( GLuint fence )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", fence );
  funcs->ext.p_glFinishFenceAPPLE( fence );
}

static void WINAPI glFinishFenceNV( GLuint fence )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", fence );
  funcs->ext.p_glFinishFenceNV( fence );
}

static void WINAPI glFinishObjectAPPLE( GLenum object, GLint name )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", object, name );
  funcs->ext.p_glFinishObjectAPPLE( object, name );
}

static void WINAPI glFinishTextureSUNX(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  funcs->ext.p_glFinishTextureSUNX();
}

static void WINAPI glFlushMappedBufferRange( GLenum target, GLintptr offset, GLsizeiptr length )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %ld, %ld)\n", target, offset, length );
  funcs->ext.p_glFlushMappedBufferRange( target, offset, length );
}

static void WINAPI glFlushMappedBufferRangeAPPLE( GLenum target, GLintptr offset, GLsizeiptr size )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %ld, %ld)\n", target, offset, size );
  funcs->ext.p_glFlushMappedBufferRangeAPPLE( target, offset, size );
}

static void WINAPI glFlushMappedNamedBufferRange( GLuint buffer, GLintptr offset, GLsizeiptr length )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %ld, %ld)\n", buffer, offset, length );
  funcs->ext.p_glFlushMappedNamedBufferRange( buffer, offset, length );
}

static void WINAPI glFlushMappedNamedBufferRangeEXT( GLuint buffer, GLintptr offset, GLsizeiptr length )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %ld, %ld)\n", buffer, offset, length );
  funcs->ext.p_glFlushMappedNamedBufferRangeEXT( buffer, offset, length );
}

static void WINAPI glFlushPixelDataRangeNV( GLenum target )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", target );
  funcs->ext.p_glFlushPixelDataRangeNV( target );
}

static void WINAPI glFlushRasterSGIX(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  funcs->ext.p_glFlushRasterSGIX();
}

static void WINAPI glFlushStaticDataIBM( GLenum target )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", target );
  funcs->ext.p_glFlushStaticDataIBM( target );
}

static void WINAPI glFlushVertexArrayRangeAPPLE( GLsizei length, void *pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", length, pointer );
  funcs->ext.p_glFlushVertexArrayRangeAPPLE( length, pointer );
}

static void WINAPI glFlushVertexArrayRangeNV(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  funcs->ext.p_glFlushVertexArrayRangeNV();
}

static void WINAPI glFogCoordFormatNV( GLenum type, GLsizei stride )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", type, stride );
  funcs->ext.p_glFogCoordFormatNV( type, stride );
}

static void WINAPI glFogCoordPointer( GLenum type, GLsizei stride, const void *pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", type, stride, pointer );
  funcs->ext.p_glFogCoordPointer( type, stride, pointer );
}

static void WINAPI glFogCoordPointerEXT( GLenum type, GLsizei stride, const void *pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", type, stride, pointer );
  funcs->ext.p_glFogCoordPointerEXT( type, stride, pointer );
}

static void WINAPI glFogCoordPointerListIBM( GLenum type, GLint stride, const void **pointer, GLint ptrstride )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %d)\n", type, stride, pointer, ptrstride );
  funcs->ext.p_glFogCoordPointerListIBM( type, stride, pointer, ptrstride );
}

static void WINAPI glFogCoordd( GLdouble coord )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f)\n", coord );
  funcs->ext.p_glFogCoordd( coord );
}

static void WINAPI glFogCoorddEXT( GLdouble coord )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f)\n", coord );
  funcs->ext.p_glFogCoorddEXT( coord );
}

static void WINAPI glFogCoorddv( const GLdouble *coord )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", coord );
  funcs->ext.p_glFogCoorddv( coord );
}

static void WINAPI glFogCoorddvEXT( const GLdouble *coord )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", coord );
  funcs->ext.p_glFogCoorddvEXT( coord );
}

static void WINAPI glFogCoordf( GLfloat coord )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f)\n", coord );
  funcs->ext.p_glFogCoordf( coord );
}

static void WINAPI glFogCoordfEXT( GLfloat coord )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f)\n", coord );
  funcs->ext.p_glFogCoordfEXT( coord );
}

static void WINAPI glFogCoordfv( const GLfloat *coord )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", coord );
  funcs->ext.p_glFogCoordfv( coord );
}

static void WINAPI glFogCoordfvEXT( const GLfloat *coord )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", coord );
  funcs->ext.p_glFogCoordfvEXT( coord );
}

static void WINAPI glFogCoordhNV( GLhalfNV fog )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", fog );
  funcs->ext.p_glFogCoordhNV( fog );
}

static void WINAPI glFogCoordhvNV( const GLhalfNV *fog )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", fog );
  funcs->ext.p_glFogCoordhvNV( fog );
}

static void WINAPI glFogFuncSGIS( GLsizei n, const GLfloat *points )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, points );
  funcs->ext.p_glFogFuncSGIS( n, points );
}

static void WINAPI glFogxOES( GLenum pname, GLfixed param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", pname, param );
  funcs->ext.p_glFogxOES( pname, param );
}

static void WINAPI glFogxvOES( GLenum pname, const GLfixed *param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", pname, param );
  funcs->ext.p_glFogxvOES( pname, param );
}

static void WINAPI glFragmentColorMaterialSGIX( GLenum face, GLenum mode )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", face, mode );
  funcs->ext.p_glFragmentColorMaterialSGIX( face, mode );
}

static void WINAPI glFragmentCoverageColorNV( GLuint color )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", color );
  funcs->ext.p_glFragmentCoverageColorNV( color );
}

static void WINAPI glFragmentLightModelfSGIX( GLenum pname, GLfloat param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f)\n", pname, param );
  funcs->ext.p_glFragmentLightModelfSGIX( pname, param );
}

static void WINAPI glFragmentLightModelfvSGIX( GLenum pname, const GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", pname, params );
  funcs->ext.p_glFragmentLightModelfvSGIX( pname, params );
}

static void WINAPI glFragmentLightModeliSGIX( GLenum pname, GLint param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", pname, param );
  funcs->ext.p_glFragmentLightModeliSGIX( pname, param );
}

static void WINAPI glFragmentLightModelivSGIX( GLenum pname, const GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", pname, params );
  funcs->ext.p_glFragmentLightModelivSGIX( pname, params );
}

static void WINAPI glFragmentLightfSGIX( GLenum light, GLenum pname, GLfloat param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %f)\n", light, pname, param );
  funcs->ext.p_glFragmentLightfSGIX( light, pname, param );
}

static void WINAPI glFragmentLightfvSGIX( GLenum light, GLenum pname, const GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", light, pname, params );
  funcs->ext.p_glFragmentLightfvSGIX( light, pname, params );
}

static void WINAPI glFragmentLightiSGIX( GLenum light, GLenum pname, GLint param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", light, pname, param );
  funcs->ext.p_glFragmentLightiSGIX( light, pname, param );
}

static void WINAPI glFragmentLightivSGIX( GLenum light, GLenum pname, const GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", light, pname, params );
  funcs->ext.p_glFragmentLightivSGIX( light, pname, params );
}

static void WINAPI glFragmentMaterialfSGIX( GLenum face, GLenum pname, GLfloat param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %f)\n", face, pname, param );
  funcs->ext.p_glFragmentMaterialfSGIX( face, pname, param );
}

static void WINAPI glFragmentMaterialfvSGIX( GLenum face, GLenum pname, const GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", face, pname, params );
  funcs->ext.p_glFragmentMaterialfvSGIX( face, pname, params );
}

static void WINAPI glFragmentMaterialiSGIX( GLenum face, GLenum pname, GLint param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", face, pname, param );
  funcs->ext.p_glFragmentMaterialiSGIX( face, pname, param );
}

static void WINAPI glFragmentMaterialivSGIX( GLenum face, GLenum pname, const GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", face, pname, params );
  funcs->ext.p_glFragmentMaterialivSGIX( face, pname, params );
}

static void WINAPI glFrameTerminatorGREMEDY(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  funcs->ext.p_glFrameTerminatorGREMEDY();
}

static void WINAPI glFrameZoomSGIX( GLint factor )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", factor );
  funcs->ext.p_glFrameZoomSGIX( factor );
}

static void WINAPI glFramebufferDrawBufferEXT( GLuint framebuffer, GLenum mode )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", framebuffer, mode );
  funcs->ext.p_glFramebufferDrawBufferEXT( framebuffer, mode );
}

static void WINAPI glFramebufferDrawBuffersEXT( GLuint framebuffer, GLsizei n, const GLenum *bufs )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", framebuffer, n, bufs );
  funcs->ext.p_glFramebufferDrawBuffersEXT( framebuffer, n, bufs );
}

static void WINAPI glFramebufferParameteri( GLenum target, GLenum pname, GLint param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", target, pname, param );
  funcs->ext.p_glFramebufferParameteri( target, pname, param );
}

static void WINAPI glFramebufferReadBufferEXT( GLuint framebuffer, GLenum mode )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", framebuffer, mode );
  funcs->ext.p_glFramebufferReadBufferEXT( framebuffer, mode );
}

static void WINAPI glFramebufferRenderbuffer( GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", target, attachment, renderbuffertarget, renderbuffer );
  funcs->ext.p_glFramebufferRenderbuffer( target, attachment, renderbuffertarget, renderbuffer );
}

static void WINAPI glFramebufferRenderbufferEXT( GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", target, attachment, renderbuffertarget, renderbuffer );
  funcs->ext.p_glFramebufferRenderbufferEXT( target, attachment, renderbuffertarget, renderbuffer );
}

static void WINAPI glFramebufferSampleLocationsfvARB( GLenum target, GLuint start, GLsizei count, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", target, start, count, v );
  funcs->ext.p_glFramebufferSampleLocationsfvARB( target, start, count, v );
}

static void WINAPI glFramebufferSampleLocationsfvNV( GLenum target, GLuint start, GLsizei count, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", target, start, count, v );
  funcs->ext.p_glFramebufferSampleLocationsfvNV( target, start, count, v );
}

static void WINAPI glFramebufferSamplePositionsfvAMD( GLenum target, GLuint numsamples, GLuint pixelindex, const GLfloat *values )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", target, numsamples, pixelindex, values );
  funcs->ext.p_glFramebufferSamplePositionsfvAMD( target, numsamples, pixelindex, values );
}

static void WINAPI glFramebufferTexture( GLenum target, GLenum attachment, GLuint texture, GLint level )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", target, attachment, texture, level );
  funcs->ext.p_glFramebufferTexture( target, attachment, texture, level );
}

static void WINAPI glFramebufferTexture1D( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", target, attachment, textarget, texture, level );
  funcs->ext.p_glFramebufferTexture1D( target, attachment, textarget, texture, level );
}

static void WINAPI glFramebufferTexture1DEXT( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", target, attachment, textarget, texture, level );
  funcs->ext.p_glFramebufferTexture1DEXT( target, attachment, textarget, texture, level );
}

static void WINAPI glFramebufferTexture2D( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", target, attachment, textarget, texture, level );
  funcs->ext.p_glFramebufferTexture2D( target, attachment, textarget, texture, level );
}

static void WINAPI glFramebufferTexture2DEXT( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", target, attachment, textarget, texture, level );
  funcs->ext.p_glFramebufferTexture2DEXT( target, attachment, textarget, texture, level );
}

static void WINAPI glFramebufferTexture3D( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d)\n", target, attachment, textarget, texture, level, zoffset );
  funcs->ext.p_glFramebufferTexture3D( target, attachment, textarget, texture, level, zoffset );
}

static void WINAPI glFramebufferTexture3DEXT( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d)\n", target, attachment, textarget, texture, level, zoffset );
  funcs->ext.p_glFramebufferTexture3DEXT( target, attachment, textarget, texture, level, zoffset );
}

static void WINAPI glFramebufferTextureARB( GLenum target, GLenum attachment, GLuint texture, GLint level )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", target, attachment, texture, level );
  funcs->ext.p_glFramebufferTextureARB( target, attachment, texture, level );
}

static void WINAPI glFramebufferTextureEXT( GLenum target, GLenum attachment, GLuint texture, GLint level )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", target, attachment, texture, level );
  funcs->ext.p_glFramebufferTextureEXT( target, attachment, texture, level );
}

static void WINAPI glFramebufferTextureFaceARB( GLenum target, GLenum attachment, GLuint texture, GLint level, GLenum face )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", target, attachment, texture, level, face );
  funcs->ext.p_glFramebufferTextureFaceARB( target, attachment, texture, level, face );
}

static void WINAPI glFramebufferTextureFaceEXT( GLenum target, GLenum attachment, GLuint texture, GLint level, GLenum face )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", target, attachment, texture, level, face );
  funcs->ext.p_glFramebufferTextureFaceEXT( target, attachment, texture, level, face );
}

static void WINAPI glFramebufferTextureLayer( GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", target, attachment, texture, level, layer );
  funcs->ext.p_glFramebufferTextureLayer( target, attachment, texture, level, layer );
}

static void WINAPI glFramebufferTextureLayerARB( GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", target, attachment, texture, level, layer );
  funcs->ext.p_glFramebufferTextureLayerARB( target, attachment, texture, level, layer );
}

static void WINAPI glFramebufferTextureLayerEXT( GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", target, attachment, texture, level, layer );
  funcs->ext.p_glFramebufferTextureLayerEXT( target, attachment, texture, level, layer );
}

static void WINAPI glFramebufferTextureMultiviewOVR( GLenum target, GLenum attachment, GLuint texture, GLint level, GLint baseViewIndex, GLsizei numViews )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d)\n", target, attachment, texture, level, baseViewIndex, numViews );
  funcs->ext.p_glFramebufferTextureMultiviewOVR( target, attachment, texture, level, baseViewIndex, numViews );
}

static void WINAPI glFreeObjectBufferATI( GLuint buffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", buffer );
  funcs->ext.p_glFreeObjectBufferATI( buffer );
}

static void WINAPI glFrustumfOES( GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f, %f, %f, %f, %f)\n", l, r, b, t, n, f );
  funcs->ext.p_glFrustumfOES( l, r, b, t, n, f );
}

static void WINAPI glFrustumxOES( GLfixed l, GLfixed r, GLfixed b, GLfixed t, GLfixed n, GLfixed f )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d)\n", l, r, b, t, n, f );
  funcs->ext.p_glFrustumxOES( l, r, b, t, n, f );
}

static GLuint WINAPI glGenAsyncMarkersSGIX( GLsizei range )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", range );
  return funcs->ext.p_glGenAsyncMarkersSGIX( range );
}

static void WINAPI glGenBuffers( GLsizei n, GLuint *buffers )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, buffers );
  funcs->ext.p_glGenBuffers( n, buffers );
}

static void WINAPI glGenBuffersARB( GLsizei n, GLuint *buffers )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, buffers );
  funcs->ext.p_glGenBuffersARB( n, buffers );
}

static void WINAPI glGenFencesAPPLE( GLsizei n, GLuint *fences )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, fences );
  funcs->ext.p_glGenFencesAPPLE( n, fences );
}

static void WINAPI glGenFencesNV( GLsizei n, GLuint *fences )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, fences );
  funcs->ext.p_glGenFencesNV( n, fences );
}

static GLuint WINAPI glGenFragmentShadersATI( GLuint range )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", range );
  return funcs->ext.p_glGenFragmentShadersATI( range );
}

static void WINAPI glGenFramebuffers( GLsizei n, GLuint *framebuffers )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, framebuffers );
  funcs->ext.p_glGenFramebuffers( n, framebuffers );
}

static void WINAPI glGenFramebuffersEXT( GLsizei n, GLuint *framebuffers )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, framebuffers );
  funcs->ext.p_glGenFramebuffersEXT( n, framebuffers );
}

static void WINAPI glGenNamesAMD( GLenum identifier, GLuint num, GLuint *names )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", identifier, num, names );
  funcs->ext.p_glGenNamesAMD( identifier, num, names );
}

static void WINAPI glGenOcclusionQueriesNV( GLsizei n, GLuint *ids )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, ids );
  funcs->ext.p_glGenOcclusionQueriesNV( n, ids );
}

static GLuint WINAPI glGenPathsNV( GLsizei range )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", range );
  return funcs->ext.p_glGenPathsNV( range );
}

static void WINAPI glGenPerfMonitorsAMD( GLsizei n, GLuint *monitors )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, monitors );
  funcs->ext.p_glGenPerfMonitorsAMD( n, monitors );
}

static void WINAPI glGenProgramPipelines( GLsizei n, GLuint *pipelines )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, pipelines );
  funcs->ext.p_glGenProgramPipelines( n, pipelines );
}

static void WINAPI glGenProgramsARB( GLsizei n, GLuint *programs )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, programs );
  funcs->ext.p_glGenProgramsARB( n, programs );
}

static void WINAPI glGenProgramsNV( GLsizei n, GLuint *programs )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, programs );
  funcs->ext.p_glGenProgramsNV( n, programs );
}

static void WINAPI glGenQueries( GLsizei n, GLuint *ids )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, ids );
  funcs->ext.p_glGenQueries( n, ids );
}

static void WINAPI glGenQueriesARB( GLsizei n, GLuint *ids )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, ids );
  funcs->ext.p_glGenQueriesARB( n, ids );
}

static void WINAPI glGenQueryResourceTagNV( GLsizei n, GLint *tagIds )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, tagIds );
  funcs->ext.p_glGenQueryResourceTagNV( n, tagIds );
}

static void WINAPI glGenRenderbuffers( GLsizei n, GLuint *renderbuffers )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, renderbuffers );
  funcs->ext.p_glGenRenderbuffers( n, renderbuffers );
}

static void WINAPI glGenRenderbuffersEXT( GLsizei n, GLuint *renderbuffers )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, renderbuffers );
  funcs->ext.p_glGenRenderbuffersEXT( n, renderbuffers );
}

static void WINAPI glGenSamplers( GLsizei count, GLuint *samplers )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", count, samplers );
  funcs->ext.p_glGenSamplers( count, samplers );
}

static void WINAPI glGenSemaphoresEXT( GLsizei n, GLuint *semaphores )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, semaphores );
  funcs->ext.p_glGenSemaphoresEXT( n, semaphores );
}

static GLuint WINAPI glGenSymbolsEXT( GLenum datatype, GLenum storagetype, GLenum range, GLuint components )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", datatype, storagetype, range, components );
  return funcs->ext.p_glGenSymbolsEXT( datatype, storagetype, range, components );
}

static void WINAPI glGenTexturesEXT( GLsizei n, GLuint *textures )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, textures );
  funcs->ext.p_glGenTexturesEXT( n, textures );
}

static void WINAPI glGenTransformFeedbacks( GLsizei n, GLuint *ids )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, ids );
  funcs->ext.p_glGenTransformFeedbacks( n, ids );
}

static void WINAPI glGenTransformFeedbacksNV( GLsizei n, GLuint *ids )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, ids );
  funcs->ext.p_glGenTransformFeedbacksNV( n, ids );
}

static void WINAPI glGenVertexArrays( GLsizei n, GLuint *arrays )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, arrays );
  funcs->ext.p_glGenVertexArrays( n, arrays );
}

static void WINAPI glGenVertexArraysAPPLE( GLsizei n, GLuint *arrays )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, arrays );
  funcs->ext.p_glGenVertexArraysAPPLE( n, arrays );
}

static GLuint WINAPI glGenVertexShadersEXT( GLuint range )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", range );
  return funcs->ext.p_glGenVertexShadersEXT( range );
}

static void WINAPI glGenerateMipmap( GLenum target )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", target );
  funcs->ext.p_glGenerateMipmap( target );
}

static void WINAPI glGenerateMipmapEXT( GLenum target )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", target );
  funcs->ext.p_glGenerateMipmapEXT( target );
}

static void WINAPI glGenerateMultiTexMipmapEXT( GLenum texunit, GLenum target )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", texunit, target );
  funcs->ext.p_glGenerateMultiTexMipmapEXT( texunit, target );
}

static void WINAPI glGenerateTextureMipmap( GLuint texture )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", texture );
  funcs->ext.p_glGenerateTextureMipmap( texture );
}

static void WINAPI glGenerateTextureMipmapEXT( GLuint texture, GLenum target )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", texture, target );
  funcs->ext.p_glGenerateTextureMipmapEXT( texture, target );
}

static void WINAPI glGetActiveAtomicCounterBufferiv( GLuint program, GLuint bufferIndex, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, bufferIndex, pname, params );
  funcs->ext.p_glGetActiveAtomicCounterBufferiv( program, bufferIndex, pname, params );
}

static void WINAPI glGetActiveAttrib( GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p, %p, %p, %p)\n", program, index, bufSize, length, size, type, name );
  funcs->ext.p_glGetActiveAttrib( program, index, bufSize, length, size, type, name );
}

static void WINAPI glGetActiveAttribARB( GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p, %p, %p, %p)\n", programObj, index, maxLength, length, size, type, name );
  funcs->ext.p_glGetActiveAttribARB( programObj, index, maxLength, length, size, type, name );
}

static void WINAPI glGetActiveSubroutineName( GLuint program, GLenum shadertype, GLuint index, GLsizei bufsize, GLsizei *length, GLchar *name )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p, %p)\n", program, shadertype, index, bufsize, length, name );
  funcs->ext.p_glGetActiveSubroutineName( program, shadertype, index, bufsize, length, name );
}

static void WINAPI glGetActiveSubroutineUniformName( GLuint program, GLenum shadertype, GLuint index, GLsizei bufsize, GLsizei *length, GLchar *name )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p, %p)\n", program, shadertype, index, bufsize, length, name );
  funcs->ext.p_glGetActiveSubroutineUniformName( program, shadertype, index, bufsize, length, name );
}

static void WINAPI glGetActiveSubroutineUniformiv( GLuint program, GLenum shadertype, GLuint index, GLenum pname, GLint *values )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, shadertype, index, pname, values );
  funcs->ext.p_glGetActiveSubroutineUniformiv( program, shadertype, index, pname, values );
}

static void WINAPI glGetActiveUniform( GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p, %p, %p, %p)\n", program, index, bufSize, length, size, type, name );
  funcs->ext.p_glGetActiveUniform( program, index, bufSize, length, size, type, name );
}

static void WINAPI glGetActiveUniformARB( GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p, %p, %p, %p)\n", programObj, index, maxLength, length, size, type, name );
  funcs->ext.p_glGetActiveUniformARB( programObj, index, maxLength, length, size, type, name );
}

static void WINAPI glGetActiveUniformBlockName( GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformBlockName )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p, %p)\n", program, uniformBlockIndex, bufSize, length, uniformBlockName );
  funcs->ext.p_glGetActiveUniformBlockName( program, uniformBlockIndex, bufSize, length, uniformBlockName );
}

static void WINAPI glGetActiveUniformBlockiv( GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, uniformBlockIndex, pname, params );
  funcs->ext.p_glGetActiveUniformBlockiv( program, uniformBlockIndex, pname, params );
}

static void WINAPI glGetActiveUniformName( GLuint program, GLuint uniformIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformName )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p, %p)\n", program, uniformIndex, bufSize, length, uniformName );
  funcs->ext.p_glGetActiveUniformName( program, uniformIndex, bufSize, length, uniformName );
}

static void WINAPI glGetActiveUniformsiv( GLuint program, GLsizei uniformCount, const GLuint *uniformIndices, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %d, %p)\n", program, uniformCount, uniformIndices, pname, params );
  funcs->ext.p_glGetActiveUniformsiv( program, uniformCount, uniformIndices, pname, params );
}

static void WINAPI glGetActiveVaryingNV( GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p, %p, %p, %p)\n", program, index, bufSize, length, size, type, name );
  funcs->ext.p_glGetActiveVaryingNV( program, index, bufSize, length, size, type, name );
}

static void WINAPI glGetArrayObjectfvATI( GLenum array, GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", array, pname, params );
  funcs->ext.p_glGetArrayObjectfvATI( array, pname, params );
}

static void WINAPI glGetArrayObjectivATI( GLenum array, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", array, pname, params );
  funcs->ext.p_glGetArrayObjectivATI( array, pname, params );
}

static void WINAPI glGetAttachedObjectsARB( GLhandleARB containerObj, GLsizei maxCount, GLsizei *count, GLhandleARB *obj )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %p)\n", containerObj, maxCount, count, obj );
  funcs->ext.p_glGetAttachedObjectsARB( containerObj, maxCount, count, obj );
}

static void WINAPI glGetAttachedShaders( GLuint program, GLsizei maxCount, GLsizei *count, GLuint *shaders )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %p)\n", program, maxCount, count, shaders );
  funcs->ext.p_glGetAttachedShaders( program, maxCount, count, shaders );
}

static GLint WINAPI glGetAttribLocation( GLuint program, const GLchar *name )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", program, name );
  return funcs->ext.p_glGetAttribLocation( program, name );
}

static GLint WINAPI glGetAttribLocationARB( GLhandleARB programObj, const GLcharARB *name )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", programObj, name );
  return funcs->ext.p_glGetAttribLocationARB( programObj, name );
}

static void WINAPI glGetBooleanIndexedvEXT( GLenum target, GLuint index, GLboolean *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, index, data );
  funcs->ext.p_glGetBooleanIndexedvEXT( target, index, data );
}

static void WINAPI glGetBooleani_v( GLenum target, GLuint index, GLboolean *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, index, data );
  funcs->ext.p_glGetBooleani_v( target, index, data );
}

static void WINAPI glGetBufferParameteri64v( GLenum target, GLenum pname, GLint64 *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetBufferParameteri64v( target, pname, params );
}

static void WINAPI glGetBufferParameteriv( GLenum target, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetBufferParameteriv( target, pname, params );
}

static void WINAPI glGetBufferParameterivARB( GLenum target, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetBufferParameterivARB( target, pname, params );
}

static void WINAPI glGetBufferParameterui64vNV( GLenum target, GLenum pname, GLuint64EXT *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetBufferParameterui64vNV( target, pname, params );
}

static void WINAPI glGetBufferPointerv( GLenum target, GLenum pname, void **params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetBufferPointerv( target, pname, params );
}

static void WINAPI glGetBufferPointervARB( GLenum target, GLenum pname, void **params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetBufferPointervARB( target, pname, params );
}

static void WINAPI glGetBufferSubData( GLenum target, GLintptr offset, GLsizeiptr size, void *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %ld, %ld, %p)\n", target, offset, size, data );
  funcs->ext.p_glGetBufferSubData( target, offset, size, data );
}

static void WINAPI glGetBufferSubDataARB( GLenum target, GLintptrARB offset, GLsizeiptrARB size, void *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %ld, %ld, %p)\n", target, offset, size, data );
  funcs->ext.p_glGetBufferSubDataARB( target, offset, size, data );
}

static void WINAPI glGetClipPlanefOES( GLenum plane, GLfloat *equation )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", plane, equation );
  funcs->ext.p_glGetClipPlanefOES( plane, equation );
}

static void WINAPI glGetClipPlanexOES( GLenum plane, GLfixed *equation )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", plane, equation );
  funcs->ext.p_glGetClipPlanexOES( plane, equation );
}

static void WINAPI glGetColorTable( GLenum target, GLenum format, GLenum type, void *table )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", target, format, type, table );
  funcs->ext.p_glGetColorTable( target, format, type, table );
}

static void WINAPI glGetColorTableEXT( GLenum target, GLenum format, GLenum type, void *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", target, format, type, data );
  funcs->ext.p_glGetColorTableEXT( target, format, type, data );
}

static void WINAPI glGetColorTableParameterfv( GLenum target, GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetColorTableParameterfv( target, pname, params );
}

static void WINAPI glGetColorTableParameterfvEXT( GLenum target, GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetColorTableParameterfvEXT( target, pname, params );
}

static void WINAPI glGetColorTableParameterfvSGI( GLenum target, GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetColorTableParameterfvSGI( target, pname, params );
}

static void WINAPI glGetColorTableParameteriv( GLenum target, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetColorTableParameteriv( target, pname, params );
}

static void WINAPI glGetColorTableParameterivEXT( GLenum target, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetColorTableParameterivEXT( target, pname, params );
}

static void WINAPI glGetColorTableParameterivSGI( GLenum target, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetColorTableParameterivSGI( target, pname, params );
}

static void WINAPI glGetColorTableSGI( GLenum target, GLenum format, GLenum type, void *table )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", target, format, type, table );
  funcs->ext.p_glGetColorTableSGI( target, format, type, table );
}

static void WINAPI glGetCombinerInputParameterfvNV( GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", stage, portion, variable, pname, params );
  funcs->ext.p_glGetCombinerInputParameterfvNV( stage, portion, variable, pname, params );
}

static void WINAPI glGetCombinerInputParameterivNV( GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", stage, portion, variable, pname, params );
  funcs->ext.p_glGetCombinerInputParameterivNV( stage, portion, variable, pname, params );
}

static void WINAPI glGetCombinerOutputParameterfvNV( GLenum stage, GLenum portion, GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", stage, portion, pname, params );
  funcs->ext.p_glGetCombinerOutputParameterfvNV( stage, portion, pname, params );
}

static void WINAPI glGetCombinerOutputParameterivNV( GLenum stage, GLenum portion, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", stage, portion, pname, params );
  funcs->ext.p_glGetCombinerOutputParameterivNV( stage, portion, pname, params );
}

static void WINAPI glGetCombinerStageParameterfvNV( GLenum stage, GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", stage, pname, params );
  funcs->ext.p_glGetCombinerStageParameterfvNV( stage, pname, params );
}

static GLuint WINAPI glGetCommandHeaderNV( GLenum tokenID, GLuint size )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", tokenID, size );
  return funcs->ext.p_glGetCommandHeaderNV( tokenID, size );
}

static void WINAPI glGetCompressedMultiTexImageEXT( GLenum texunit, GLenum target, GLint lod, void *img )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", texunit, target, lod, img );
  funcs->ext.p_glGetCompressedMultiTexImageEXT( texunit, target, lod, img );
}

static void WINAPI glGetCompressedTexImage( GLenum target, GLint level, void *img )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, level, img );
  funcs->ext.p_glGetCompressedTexImage( target, level, img );
}

static void WINAPI glGetCompressedTexImageARB( GLenum target, GLint level, void *img )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, level, img );
  funcs->ext.p_glGetCompressedTexImageARB( target, level, img );
}

static void WINAPI glGetCompressedTextureImage( GLuint texture, GLint level, GLsizei bufSize, void *pixels )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", texture, level, bufSize, pixels );
  funcs->ext.p_glGetCompressedTextureImage( texture, level, bufSize, pixels );
}

static void WINAPI glGetCompressedTextureImageEXT( GLuint texture, GLenum target, GLint lod, void *img )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", texture, target, lod, img );
  funcs->ext.p_glGetCompressedTextureImageEXT( texture, target, lod, img );
}

static void WINAPI glGetCompressedTextureSubImage( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei bufSize, void *pixels )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texture, level, xoffset, yoffset, zoffset, width, height, depth, bufSize, pixels );
  funcs->ext.p_glGetCompressedTextureSubImage( texture, level, xoffset, yoffset, zoffset, width, height, depth, bufSize, pixels );
}

static void WINAPI glGetConvolutionFilter( GLenum target, GLenum format, GLenum type, void *image )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", target, format, type, image );
  funcs->ext.p_glGetConvolutionFilter( target, format, type, image );
}

static void WINAPI glGetConvolutionFilterEXT( GLenum target, GLenum format, GLenum type, void *image )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", target, format, type, image );
  funcs->ext.p_glGetConvolutionFilterEXT( target, format, type, image );
}

static void WINAPI glGetConvolutionParameterfv( GLenum target, GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetConvolutionParameterfv( target, pname, params );
}

static void WINAPI glGetConvolutionParameterfvEXT( GLenum target, GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetConvolutionParameterfvEXT( target, pname, params );
}

static void WINAPI glGetConvolutionParameteriv( GLenum target, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetConvolutionParameteriv( target, pname, params );
}

static void WINAPI glGetConvolutionParameterivEXT( GLenum target, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetConvolutionParameterivEXT( target, pname, params );
}

static void WINAPI glGetConvolutionParameterxvOES( GLenum target, GLenum pname, GLfixed *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetConvolutionParameterxvOES( target, pname, params );
}

static void WINAPI glGetCoverageModulationTableNV( GLsizei bufsize, GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", bufsize, v );
  funcs->ext.p_glGetCoverageModulationTableNV( bufsize, v );
}

static GLuint WINAPI glGetDebugMessageLog( GLuint count, GLsizei bufSize, GLenum *sources, GLenum *types, GLuint *ids, GLenum *severities, GLsizei *lengths, GLchar *messageLog )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %p, %p, %p, %p, %p)\n", count, bufSize, sources, types, ids, severities, lengths, messageLog );
  return funcs->ext.p_glGetDebugMessageLog( count, bufSize, sources, types, ids, severities, lengths, messageLog );
}

static GLuint WINAPI glGetDebugMessageLogAMD( GLuint count, GLsizei bufsize, GLenum *categories, GLuint *severities, GLuint *ids, GLsizei *lengths, GLchar *message )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %p, %p, %p, %p)\n", count, bufsize, categories, severities, ids, lengths, message );
  return funcs->ext.p_glGetDebugMessageLogAMD( count, bufsize, categories, severities, ids, lengths, message );
}

static GLuint WINAPI glGetDebugMessageLogARB( GLuint count, GLsizei bufSize, GLenum *sources, GLenum *types, GLuint *ids, GLenum *severities, GLsizei *lengths, GLchar *messageLog )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %p, %p, %p, %p, %p)\n", count, bufSize, sources, types, ids, severities, lengths, messageLog );
  return funcs->ext.p_glGetDebugMessageLogARB( count, bufSize, sources, types, ids, severities, lengths, messageLog );
}

static void WINAPI glGetDetailTexFuncSGIS( GLenum target, GLfloat *points )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, points );
  funcs->ext.p_glGetDetailTexFuncSGIS( target, points );
}

static void WINAPI glGetDoubleIndexedvEXT( GLenum target, GLuint index, GLdouble *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, index, data );
  funcs->ext.p_glGetDoubleIndexedvEXT( target, index, data );
}

static void WINAPI glGetDoublei_v( GLenum target, GLuint index, GLdouble *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, index, data );
  funcs->ext.p_glGetDoublei_v( target, index, data );
}

static void WINAPI glGetDoublei_vEXT( GLenum pname, GLuint index, GLdouble *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", pname, index, params );
  funcs->ext.p_glGetDoublei_vEXT( pname, index, params );
}

static void WINAPI glGetFenceivNV( GLuint fence, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", fence, pname, params );
  funcs->ext.p_glGetFenceivNV( fence, pname, params );
}

static void WINAPI glGetFinalCombinerInputParameterfvNV( GLenum variable, GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", variable, pname, params );
  funcs->ext.p_glGetFinalCombinerInputParameterfvNV( variable, pname, params );
}

static void WINAPI glGetFinalCombinerInputParameterivNV( GLenum variable, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", variable, pname, params );
  funcs->ext.p_glGetFinalCombinerInputParameterivNV( variable, pname, params );
}

static void WINAPI glGetFirstPerfQueryIdINTEL( GLuint *queryId )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", queryId );
  funcs->ext.p_glGetFirstPerfQueryIdINTEL( queryId );
}

static void WINAPI glGetFixedvOES( GLenum pname, GLfixed *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", pname, params );
  funcs->ext.p_glGetFixedvOES( pname, params );
}

static void WINAPI glGetFloatIndexedvEXT( GLenum target, GLuint index, GLfloat *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, index, data );
  funcs->ext.p_glGetFloatIndexedvEXT( target, index, data );
}

static void WINAPI glGetFloati_v( GLenum target, GLuint index, GLfloat *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, index, data );
  funcs->ext.p_glGetFloati_v( target, index, data );
}

static void WINAPI glGetFloati_vEXT( GLenum pname, GLuint index, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", pname, index, params );
  funcs->ext.p_glGetFloati_vEXT( pname, index, params );
}

static void WINAPI glGetFogFuncSGIS( GLfloat *points )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", points );
  funcs->ext.p_glGetFogFuncSGIS( points );
}

static GLint WINAPI glGetFragDataIndex( GLuint program, const GLchar *name )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", program, name );
  return funcs->ext.p_glGetFragDataIndex( program, name );
}

static GLint WINAPI glGetFragDataLocation( GLuint program, const GLchar *name )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", program, name );
  return funcs->ext.p_glGetFragDataLocation( program, name );
}

static GLint WINAPI glGetFragDataLocationEXT( GLuint program, const GLchar *name )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", program, name );
  return funcs->ext.p_glGetFragDataLocationEXT( program, name );
}

static void WINAPI glGetFragmentLightfvSGIX( GLenum light, GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", light, pname, params );
  funcs->ext.p_glGetFragmentLightfvSGIX( light, pname, params );
}

static void WINAPI glGetFragmentLightivSGIX( GLenum light, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", light, pname, params );
  funcs->ext.p_glGetFragmentLightivSGIX( light, pname, params );
}

static void WINAPI glGetFragmentMaterialfvSGIX( GLenum face, GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", face, pname, params );
  funcs->ext.p_glGetFragmentMaterialfvSGIX( face, pname, params );
}

static void WINAPI glGetFragmentMaterialivSGIX( GLenum face, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", face, pname, params );
  funcs->ext.p_glGetFragmentMaterialivSGIX( face, pname, params );
}

static void WINAPI glGetFramebufferAttachmentParameteriv( GLenum target, GLenum attachment, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", target, attachment, pname, params );
  funcs->ext.p_glGetFramebufferAttachmentParameteriv( target, attachment, pname, params );
}

static void WINAPI glGetFramebufferAttachmentParameterivEXT( GLenum target, GLenum attachment, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", target, attachment, pname, params );
  funcs->ext.p_glGetFramebufferAttachmentParameterivEXT( target, attachment, pname, params );
}

static void WINAPI glGetFramebufferParameterfvAMD( GLenum target, GLenum pname, GLuint numsamples, GLuint pixelindex, GLsizei size, GLfloat *values )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %p)\n", target, pname, numsamples, pixelindex, size, values );
  funcs->ext.p_glGetFramebufferParameterfvAMD( target, pname, numsamples, pixelindex, size, values );
}

static void WINAPI glGetFramebufferParameteriv( GLenum target, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetFramebufferParameteriv( target, pname, params );
}

static void WINAPI glGetFramebufferParameterivEXT( GLuint framebuffer, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", framebuffer, pname, params );
  funcs->ext.p_glGetFramebufferParameterivEXT( framebuffer, pname, params );
}

static GLenum WINAPI glGetGraphicsResetStatus(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  return funcs->ext.p_glGetGraphicsResetStatus();
}

static GLenum WINAPI glGetGraphicsResetStatusARB(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  return funcs->ext.p_glGetGraphicsResetStatusARB();
}

static GLhandleARB WINAPI glGetHandleARB( GLenum pname )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", pname );
  return funcs->ext.p_glGetHandleARB( pname );
}

static void WINAPI glGetHistogram( GLenum target, GLboolean reset, GLenum format, GLenum type, void *values )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", target, reset, format, type, values );
  funcs->ext.p_glGetHistogram( target, reset, format, type, values );
}

static void WINAPI glGetHistogramEXT( GLenum target, GLboolean reset, GLenum format, GLenum type, void *values )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", target, reset, format, type, values );
  funcs->ext.p_glGetHistogramEXT( target, reset, format, type, values );
}

static void WINAPI glGetHistogramParameterfv( GLenum target, GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetHistogramParameterfv( target, pname, params );
}

static void WINAPI glGetHistogramParameterfvEXT( GLenum target, GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetHistogramParameterfvEXT( target, pname, params );
}

static void WINAPI glGetHistogramParameteriv( GLenum target, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetHistogramParameteriv( target, pname, params );
}

static void WINAPI glGetHistogramParameterivEXT( GLenum target, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetHistogramParameterivEXT( target, pname, params );
}

static void WINAPI glGetHistogramParameterxvOES( GLenum target, GLenum pname, GLfixed *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetHistogramParameterxvOES( target, pname, params );
}

static GLuint64 WINAPI glGetImageHandleARB( GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum format )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", texture, level, layered, layer, format );
  return funcs->ext.p_glGetImageHandleARB( texture, level, layered, layer, format );
}

static GLuint64 WINAPI glGetImageHandleNV( GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum format )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", texture, level, layered, layer, format );
  return funcs->ext.p_glGetImageHandleNV( texture, level, layered, layer, format );
}

static void WINAPI glGetImageTransformParameterfvHP( GLenum target, GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetImageTransformParameterfvHP( target, pname, params );
}

static void WINAPI glGetImageTransformParameterivHP( GLenum target, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetImageTransformParameterivHP( target, pname, params );
}

static void WINAPI glGetInfoLogARB( GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *infoLog )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %p)\n", obj, maxLength, length, infoLog );
  funcs->ext.p_glGetInfoLogARB( obj, maxLength, length, infoLog );
}

static GLint WINAPI glGetInstrumentsSGIX(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  return funcs->ext.p_glGetInstrumentsSGIX();
}

static void WINAPI glGetInteger64i_v( GLenum target, GLuint index, GLint64 *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, index, data );
  funcs->ext.p_glGetInteger64i_v( target, index, data );
}

static void WINAPI glGetInteger64v( GLenum pname, GLint64 *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", pname, data );
  funcs->ext.p_glGetInteger64v( pname, data );
}

static void WINAPI glGetIntegerIndexedvEXT( GLenum target, GLuint index, GLint *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, index, data );
  funcs->ext.p_glGetIntegerIndexedvEXT( target, index, data );
}

static void WINAPI glGetIntegeri_v( GLenum target, GLuint index, GLint *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, index, data );
  funcs->ext.p_glGetIntegeri_v( target, index, data );
}

static void WINAPI glGetIntegerui64i_vNV( GLenum value, GLuint index, GLuint64EXT *result )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", value, index, result );
  funcs->ext.p_glGetIntegerui64i_vNV( value, index, result );
}

static void WINAPI glGetIntegerui64vNV( GLenum value, GLuint64EXT *result )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", value, result );
  funcs->ext.p_glGetIntegerui64vNV( value, result );
}

static void WINAPI glGetInternalformatSampleivNV( GLenum target, GLenum internalformat, GLsizei samples, GLenum pname, GLsizei bufSize, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %p)\n", target, internalformat, samples, pname, bufSize, params );
  funcs->ext.p_glGetInternalformatSampleivNV( target, internalformat, samples, pname, bufSize, params );
}

static void WINAPI glGetInternalformati64v( GLenum target, GLenum internalformat, GLenum pname, GLsizei bufSize, GLint64 *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", target, internalformat, pname, bufSize, params );
  funcs->ext.p_glGetInternalformati64v( target, internalformat, pname, bufSize, params );
}

static void WINAPI glGetInternalformativ( GLenum target, GLenum internalformat, GLenum pname, GLsizei bufSize, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", target, internalformat, pname, bufSize, params );
  funcs->ext.p_glGetInternalformativ( target, internalformat, pname, bufSize, params );
}

static void WINAPI glGetInvariantBooleanvEXT( GLuint id, GLenum value, GLboolean *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", id, value, data );
  funcs->ext.p_glGetInvariantBooleanvEXT( id, value, data );
}

static void WINAPI glGetInvariantFloatvEXT( GLuint id, GLenum value, GLfloat *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", id, value, data );
  funcs->ext.p_glGetInvariantFloatvEXT( id, value, data );
}

static void WINAPI glGetInvariantIntegervEXT( GLuint id, GLenum value, GLint *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", id, value, data );
  funcs->ext.p_glGetInvariantIntegervEXT( id, value, data );
}

static void WINAPI glGetLightxOES( GLenum light, GLenum pname, GLfixed *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", light, pname, params );
  funcs->ext.p_glGetLightxOES( light, pname, params );
}

static void WINAPI glGetListParameterfvSGIX( GLuint list, GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", list, pname, params );
  funcs->ext.p_glGetListParameterfvSGIX( list, pname, params );
}

static void WINAPI glGetListParameterivSGIX( GLuint list, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", list, pname, params );
  funcs->ext.p_glGetListParameterivSGIX( list, pname, params );
}

static void WINAPI glGetLocalConstantBooleanvEXT( GLuint id, GLenum value, GLboolean *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", id, value, data );
  funcs->ext.p_glGetLocalConstantBooleanvEXT( id, value, data );
}

static void WINAPI glGetLocalConstantFloatvEXT( GLuint id, GLenum value, GLfloat *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", id, value, data );
  funcs->ext.p_glGetLocalConstantFloatvEXT( id, value, data );
}

static void WINAPI glGetLocalConstantIntegervEXT( GLuint id, GLenum value, GLint *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", id, value, data );
  funcs->ext.p_glGetLocalConstantIntegervEXT( id, value, data );
}

static void WINAPI glGetMapAttribParameterfvNV( GLenum target, GLuint index, GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", target, index, pname, params );
  funcs->ext.p_glGetMapAttribParameterfvNV( target, index, pname, params );
}

static void WINAPI glGetMapAttribParameterivNV( GLenum target, GLuint index, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", target, index, pname, params );
  funcs->ext.p_glGetMapAttribParameterivNV( target, index, pname, params );
}

static void WINAPI glGetMapControlPointsNV( GLenum target, GLuint index, GLenum type, GLsizei ustride, GLsizei vstride, GLboolean packed, void *points )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %p)\n", target, index, type, ustride, vstride, packed, points );
  funcs->ext.p_glGetMapControlPointsNV( target, index, type, ustride, vstride, packed, points );
}

static void WINAPI glGetMapParameterfvNV( GLenum target, GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetMapParameterfvNV( target, pname, params );
}

static void WINAPI glGetMapParameterivNV( GLenum target, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetMapParameterivNV( target, pname, params );
}

static void WINAPI glGetMapxvOES( GLenum target, GLenum query, GLfixed *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, query, v );
  funcs->ext.p_glGetMapxvOES( target, query, v );
}

static void WINAPI glGetMaterialxOES( GLenum face, GLenum pname, GLfixed param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", face, pname, param );
  funcs->ext.p_glGetMaterialxOES( face, pname, param );
}

static void WINAPI glGetMemoryObjectParameterivEXT( GLuint memoryObject, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", memoryObject, pname, params );
  funcs->ext.p_glGetMemoryObjectParameterivEXT( memoryObject, pname, params );
}

static void WINAPI glGetMinmax( GLenum target, GLboolean reset, GLenum format, GLenum type, void *values )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", target, reset, format, type, values );
  funcs->ext.p_glGetMinmax( target, reset, format, type, values );
}

static void WINAPI glGetMinmaxEXT( GLenum target, GLboolean reset, GLenum format, GLenum type, void *values )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", target, reset, format, type, values );
  funcs->ext.p_glGetMinmaxEXT( target, reset, format, type, values );
}

static void WINAPI glGetMinmaxParameterfv( GLenum target, GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetMinmaxParameterfv( target, pname, params );
}

static void WINAPI glGetMinmaxParameterfvEXT( GLenum target, GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetMinmaxParameterfvEXT( target, pname, params );
}

static void WINAPI glGetMinmaxParameteriv( GLenum target, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetMinmaxParameteriv( target, pname, params );
}

static void WINAPI glGetMinmaxParameterivEXT( GLenum target, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetMinmaxParameterivEXT( target, pname, params );
}

static void WINAPI glGetMultiTexEnvfvEXT( GLenum texunit, GLenum target, GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", texunit, target, pname, params );
  funcs->ext.p_glGetMultiTexEnvfvEXT( texunit, target, pname, params );
}

static void WINAPI glGetMultiTexEnvivEXT( GLenum texunit, GLenum target, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", texunit, target, pname, params );
  funcs->ext.p_glGetMultiTexEnvivEXT( texunit, target, pname, params );
}

static void WINAPI glGetMultiTexGendvEXT( GLenum texunit, GLenum coord, GLenum pname, GLdouble *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", texunit, coord, pname, params );
  funcs->ext.p_glGetMultiTexGendvEXT( texunit, coord, pname, params );
}

static void WINAPI glGetMultiTexGenfvEXT( GLenum texunit, GLenum coord, GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", texunit, coord, pname, params );
  funcs->ext.p_glGetMultiTexGenfvEXT( texunit, coord, pname, params );
}

static void WINAPI glGetMultiTexGenivEXT( GLenum texunit, GLenum coord, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", texunit, coord, pname, params );
  funcs->ext.p_glGetMultiTexGenivEXT( texunit, coord, pname, params );
}

static void WINAPI glGetMultiTexImageEXT( GLenum texunit, GLenum target, GLint level, GLenum format, GLenum type, void *pixels )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %p)\n", texunit, target, level, format, type, pixels );
  funcs->ext.p_glGetMultiTexImageEXT( texunit, target, level, format, type, pixels );
}

static void WINAPI glGetMultiTexLevelParameterfvEXT( GLenum texunit, GLenum target, GLint level, GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", texunit, target, level, pname, params );
  funcs->ext.p_glGetMultiTexLevelParameterfvEXT( texunit, target, level, pname, params );
}

static void WINAPI glGetMultiTexLevelParameterivEXT( GLenum texunit, GLenum target, GLint level, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", texunit, target, level, pname, params );
  funcs->ext.p_glGetMultiTexLevelParameterivEXT( texunit, target, level, pname, params );
}

static void WINAPI glGetMultiTexParameterIivEXT( GLenum texunit, GLenum target, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", texunit, target, pname, params );
  funcs->ext.p_glGetMultiTexParameterIivEXT( texunit, target, pname, params );
}

static void WINAPI glGetMultiTexParameterIuivEXT( GLenum texunit, GLenum target, GLenum pname, GLuint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", texunit, target, pname, params );
  funcs->ext.p_glGetMultiTexParameterIuivEXT( texunit, target, pname, params );
}

static void WINAPI glGetMultiTexParameterfvEXT( GLenum texunit, GLenum target, GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", texunit, target, pname, params );
  funcs->ext.p_glGetMultiTexParameterfvEXT( texunit, target, pname, params );
}

static void WINAPI glGetMultiTexParameterivEXT( GLenum texunit, GLenum target, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", texunit, target, pname, params );
  funcs->ext.p_glGetMultiTexParameterivEXT( texunit, target, pname, params );
}

static void WINAPI glGetMultisamplefv( GLenum pname, GLuint index, GLfloat *val )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", pname, index, val );
  funcs->ext.p_glGetMultisamplefv( pname, index, val );
}

static void WINAPI glGetMultisamplefvNV( GLenum pname, GLuint index, GLfloat *val )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", pname, index, val );
  funcs->ext.p_glGetMultisamplefvNV( pname, index, val );
}

static void WINAPI glGetNamedBufferParameteri64v( GLuint buffer, GLenum pname, GLint64 *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", buffer, pname, params );
  funcs->ext.p_glGetNamedBufferParameteri64v( buffer, pname, params );
}

static void WINAPI glGetNamedBufferParameteriv( GLuint buffer, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", buffer, pname, params );
  funcs->ext.p_glGetNamedBufferParameteriv( buffer, pname, params );
}

static void WINAPI glGetNamedBufferParameterivEXT( GLuint buffer, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", buffer, pname, params );
  funcs->ext.p_glGetNamedBufferParameterivEXT( buffer, pname, params );
}

static void WINAPI glGetNamedBufferParameterui64vNV( GLuint buffer, GLenum pname, GLuint64EXT *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", buffer, pname, params );
  funcs->ext.p_glGetNamedBufferParameterui64vNV( buffer, pname, params );
}

static void WINAPI glGetNamedBufferPointerv( GLuint buffer, GLenum pname, void **params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", buffer, pname, params );
  funcs->ext.p_glGetNamedBufferPointerv( buffer, pname, params );
}

static void WINAPI glGetNamedBufferPointervEXT( GLuint buffer, GLenum pname, void **params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", buffer, pname, params );
  funcs->ext.p_glGetNamedBufferPointervEXT( buffer, pname, params );
}

static void WINAPI glGetNamedBufferSubData( GLuint buffer, GLintptr offset, GLsizeiptr size, void *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %ld, %ld, %p)\n", buffer, offset, size, data );
  funcs->ext.p_glGetNamedBufferSubData( buffer, offset, size, data );
}

static void WINAPI glGetNamedBufferSubDataEXT( GLuint buffer, GLintptr offset, GLsizeiptr size, void *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %ld, %ld, %p)\n", buffer, offset, size, data );
  funcs->ext.p_glGetNamedBufferSubDataEXT( buffer, offset, size, data );
}

static void WINAPI glGetNamedFramebufferAttachmentParameteriv( GLuint framebuffer, GLenum attachment, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", framebuffer, attachment, pname, params );
  funcs->ext.p_glGetNamedFramebufferAttachmentParameteriv( framebuffer, attachment, pname, params );
}

static void WINAPI glGetNamedFramebufferAttachmentParameterivEXT( GLuint framebuffer, GLenum attachment, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", framebuffer, attachment, pname, params );
  funcs->ext.p_glGetNamedFramebufferAttachmentParameterivEXT( framebuffer, attachment, pname, params );
}

static void WINAPI glGetNamedFramebufferParameterfvAMD( GLuint framebuffer, GLenum pname, GLuint numsamples, GLuint pixelindex, GLsizei size, GLfloat *values )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %p)\n", framebuffer, pname, numsamples, pixelindex, size, values );
  funcs->ext.p_glGetNamedFramebufferParameterfvAMD( framebuffer, pname, numsamples, pixelindex, size, values );
}

static void WINAPI glGetNamedFramebufferParameteriv( GLuint framebuffer, GLenum pname, GLint *param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", framebuffer, pname, param );
  funcs->ext.p_glGetNamedFramebufferParameteriv( framebuffer, pname, param );
}

static void WINAPI glGetNamedFramebufferParameterivEXT( GLuint framebuffer, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", framebuffer, pname, params );
  funcs->ext.p_glGetNamedFramebufferParameterivEXT( framebuffer, pname, params );
}

static void WINAPI glGetNamedProgramLocalParameterIivEXT( GLuint program, GLenum target, GLuint index, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, target, index, params );
  funcs->ext.p_glGetNamedProgramLocalParameterIivEXT( program, target, index, params );
}

static void WINAPI glGetNamedProgramLocalParameterIuivEXT( GLuint program, GLenum target, GLuint index, GLuint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, target, index, params );
  funcs->ext.p_glGetNamedProgramLocalParameterIuivEXT( program, target, index, params );
}

static void WINAPI glGetNamedProgramLocalParameterdvEXT( GLuint program, GLenum target, GLuint index, GLdouble *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, target, index, params );
  funcs->ext.p_glGetNamedProgramLocalParameterdvEXT( program, target, index, params );
}

static void WINAPI glGetNamedProgramLocalParameterfvEXT( GLuint program, GLenum target, GLuint index, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, target, index, params );
  funcs->ext.p_glGetNamedProgramLocalParameterfvEXT( program, target, index, params );
}

static void WINAPI glGetNamedProgramStringEXT( GLuint program, GLenum target, GLenum pname, void *string )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, target, pname, string );
  funcs->ext.p_glGetNamedProgramStringEXT( program, target, pname, string );
}

static void WINAPI glGetNamedProgramivEXT( GLuint program, GLenum target, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, target, pname, params );
  funcs->ext.p_glGetNamedProgramivEXT( program, target, pname, params );
}

static void WINAPI glGetNamedRenderbufferParameteriv( GLuint renderbuffer, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", renderbuffer, pname, params );
  funcs->ext.p_glGetNamedRenderbufferParameteriv( renderbuffer, pname, params );
}

static void WINAPI glGetNamedRenderbufferParameterivEXT( GLuint renderbuffer, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", renderbuffer, pname, params );
  funcs->ext.p_glGetNamedRenderbufferParameterivEXT( renderbuffer, pname, params );
}

static void WINAPI glGetNamedStringARB( GLint namelen, const GLchar *name, GLsizei bufSize, GLint *stringlen, GLchar *string )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p, %d, %p, %p)\n", namelen, name, bufSize, stringlen, string );
  funcs->ext.p_glGetNamedStringARB( namelen, name, bufSize, stringlen, string );
}

static void WINAPI glGetNamedStringivARB( GLint namelen, const GLchar *name, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p, %d, %p)\n", namelen, name, pname, params );
  funcs->ext.p_glGetNamedStringivARB( namelen, name, pname, params );
}

static void WINAPI glGetNextPerfQueryIdINTEL( GLuint queryId, GLuint *nextQueryId )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", queryId, nextQueryId );
  funcs->ext.p_glGetNextPerfQueryIdINTEL( queryId, nextQueryId );
}

static void WINAPI glGetObjectBufferfvATI( GLuint buffer, GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", buffer, pname, params );
  funcs->ext.p_glGetObjectBufferfvATI( buffer, pname, params );
}

static void WINAPI glGetObjectBufferivATI( GLuint buffer, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", buffer, pname, params );
  funcs->ext.p_glGetObjectBufferivATI( buffer, pname, params );
}

static void WINAPI glGetObjectLabel( GLenum identifier, GLuint name, GLsizei bufSize, GLsizei *length, GLchar *label )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p, %p)\n", identifier, name, bufSize, length, label );
  funcs->ext.p_glGetObjectLabel( identifier, name, bufSize, length, label );
}

static void WINAPI glGetObjectLabelEXT( GLenum type, GLuint object, GLsizei bufSize, GLsizei *length, GLchar *label )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p, %p)\n", type, object, bufSize, length, label );
  funcs->ext.p_glGetObjectLabelEXT( type, object, bufSize, length, label );
}

static void WINAPI glGetObjectParameterfvARB( GLhandleARB obj, GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", obj, pname, params );
  funcs->ext.p_glGetObjectParameterfvARB( obj, pname, params );
}

static void WINAPI glGetObjectParameterivAPPLE( GLenum objectType, GLuint name, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", objectType, name, pname, params );
  funcs->ext.p_glGetObjectParameterivAPPLE( objectType, name, pname, params );
}

static void WINAPI glGetObjectParameterivARB( GLhandleARB obj, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", obj, pname, params );
  funcs->ext.p_glGetObjectParameterivARB( obj, pname, params );
}

static void WINAPI glGetObjectPtrLabel( const void *ptr, GLsizei bufSize, GLsizei *length, GLchar *label )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p, %d, %p, %p)\n", ptr, bufSize, length, label );
  funcs->ext.p_glGetObjectPtrLabel( ptr, bufSize, length, label );
}

static void WINAPI glGetOcclusionQueryivNV( GLuint id, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", id, pname, params );
  funcs->ext.p_glGetOcclusionQueryivNV( id, pname, params );
}

static void WINAPI glGetOcclusionQueryuivNV( GLuint id, GLenum pname, GLuint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", id, pname, params );
  funcs->ext.p_glGetOcclusionQueryuivNV( id, pname, params );
}

static void WINAPI glGetPathColorGenfvNV( GLenum color, GLenum pname, GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", color, pname, value );
  funcs->ext.p_glGetPathColorGenfvNV( color, pname, value );
}

static void WINAPI glGetPathColorGenivNV( GLenum color, GLenum pname, GLint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", color, pname, value );
  funcs->ext.p_glGetPathColorGenivNV( color, pname, value );
}

static void WINAPI glGetPathCommandsNV( GLuint path, GLubyte *commands )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", path, commands );
  funcs->ext.p_glGetPathCommandsNV( path, commands );
}

static void WINAPI glGetPathCoordsNV( GLuint path, GLfloat *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", path, coords );
  funcs->ext.p_glGetPathCoordsNV( path, coords );
}

static void WINAPI glGetPathDashArrayNV( GLuint path, GLfloat *dashArray )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", path, dashArray );
  funcs->ext.p_glGetPathDashArrayNV( path, dashArray );
}

static GLfloat WINAPI glGetPathLengthNV( GLuint path, GLsizei startSegment, GLsizei numSegments )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", path, startSegment, numSegments );
  return funcs->ext.p_glGetPathLengthNV( path, startSegment, numSegments );
}

static void WINAPI glGetPathMetricRangeNV( GLbitfield metricQueryMask, GLuint firstPathName, GLsizei numPaths, GLsizei stride, GLfloat *metrics )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", metricQueryMask, firstPathName, numPaths, stride, metrics );
  funcs->ext.p_glGetPathMetricRangeNV( metricQueryMask, firstPathName, numPaths, stride, metrics );
}

static void WINAPI glGetPathMetricsNV( GLbitfield metricQueryMask, GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLsizei stride, GLfloat *metrics )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p, %d, %d, %p)\n", metricQueryMask, numPaths, pathNameType, paths, pathBase, stride, metrics );
  funcs->ext.p_glGetPathMetricsNV( metricQueryMask, numPaths, pathNameType, paths, pathBase, stride, metrics );
}

static void WINAPI glGetPathParameterfvNV( GLuint path, GLenum pname, GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", path, pname, value );
  funcs->ext.p_glGetPathParameterfvNV( path, pname, value );
}

static void WINAPI glGetPathParameterivNV( GLuint path, GLenum pname, GLint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", path, pname, value );
  funcs->ext.p_glGetPathParameterivNV( path, pname, value );
}

static void WINAPI glGetPathSpacingNV( GLenum pathListMode, GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLfloat advanceScale, GLfloat kerningScale, GLenum transformType, GLfloat *returnedSpacing )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p, %d, %f, %f, %d, %p)\n", pathListMode, numPaths, pathNameType, paths, pathBase, advanceScale, kerningScale, transformType, returnedSpacing );
  funcs->ext.p_glGetPathSpacingNV( pathListMode, numPaths, pathNameType, paths, pathBase, advanceScale, kerningScale, transformType, returnedSpacing );
}

static void WINAPI glGetPathTexGenfvNV( GLenum texCoordSet, GLenum pname, GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", texCoordSet, pname, value );
  funcs->ext.p_glGetPathTexGenfvNV( texCoordSet, pname, value );
}

static void WINAPI glGetPathTexGenivNV( GLenum texCoordSet, GLenum pname, GLint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", texCoordSet, pname, value );
  funcs->ext.p_glGetPathTexGenivNV( texCoordSet, pname, value );
}

static void WINAPI glGetPerfCounterInfoINTEL( GLuint queryId, GLuint counterId, GLuint counterNameLength, GLchar *counterName, GLuint counterDescLength, GLchar *counterDesc, GLuint *counterOffset, GLuint *counterDataSize, GLuint *counterTypeEnum, GLuint *counterDataTypeEnum, GLuint64 *rawCounterMaxValue )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p, %d, %p, %p, %p, %p, %p, %p)\n", queryId, counterId, counterNameLength, counterName, counterDescLength, counterDesc, counterOffset, counterDataSize, counterTypeEnum, counterDataTypeEnum, rawCounterMaxValue );
  funcs->ext.p_glGetPerfCounterInfoINTEL( queryId, counterId, counterNameLength, counterName, counterDescLength, counterDesc, counterOffset, counterDataSize, counterTypeEnum, counterDataTypeEnum, rawCounterMaxValue );
}

static void WINAPI glGetPerfMonitorCounterDataAMD( GLuint monitor, GLenum pname, GLsizei dataSize, GLuint *data, GLint *bytesWritten )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p, %p)\n", monitor, pname, dataSize, data, bytesWritten );
  funcs->ext.p_glGetPerfMonitorCounterDataAMD( monitor, pname, dataSize, data, bytesWritten );
}

static void WINAPI glGetPerfMonitorCounterInfoAMD( GLuint group, GLuint counter, GLenum pname, void *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", group, counter, pname, data );
  funcs->ext.p_glGetPerfMonitorCounterInfoAMD( group, counter, pname, data );
}

static void WINAPI glGetPerfMonitorCounterStringAMD( GLuint group, GLuint counter, GLsizei bufSize, GLsizei *length, GLchar *counterString )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p, %p)\n", group, counter, bufSize, length, counterString );
  funcs->ext.p_glGetPerfMonitorCounterStringAMD( group, counter, bufSize, length, counterString );
}

static void WINAPI glGetPerfMonitorCountersAMD( GLuint group, GLint *numCounters, GLint *maxActiveCounters, GLsizei counterSize, GLuint *counters )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p, %p, %d, %p)\n", group, numCounters, maxActiveCounters, counterSize, counters );
  funcs->ext.p_glGetPerfMonitorCountersAMD( group, numCounters, maxActiveCounters, counterSize, counters );
}

static void WINAPI glGetPerfMonitorGroupStringAMD( GLuint group, GLsizei bufSize, GLsizei *length, GLchar *groupString )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %p)\n", group, bufSize, length, groupString );
  funcs->ext.p_glGetPerfMonitorGroupStringAMD( group, bufSize, length, groupString );
}

static void WINAPI glGetPerfMonitorGroupsAMD( GLint *numGroups, GLsizei groupsSize, GLuint *groups )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p, %d, %p)\n", numGroups, groupsSize, groups );
  funcs->ext.p_glGetPerfMonitorGroupsAMD( numGroups, groupsSize, groups );
}

static void WINAPI glGetPerfQueryDataINTEL( GLuint queryHandle, GLuint flags, GLsizei dataSize, GLvoid *data, GLuint *bytesWritten )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p, %p)\n", queryHandle, flags, dataSize, data, bytesWritten );
  funcs->ext.p_glGetPerfQueryDataINTEL( queryHandle, flags, dataSize, data, bytesWritten );
}

static void WINAPI glGetPerfQueryIdByNameINTEL( GLchar *queryName, GLuint *queryId )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p, %p)\n", queryName, queryId );
  funcs->ext.p_glGetPerfQueryIdByNameINTEL( queryName, queryId );
}

static void WINAPI glGetPerfQueryInfoINTEL( GLuint queryId, GLuint queryNameLength, GLchar *queryName, GLuint *dataSize, GLuint *noCounters, GLuint *noInstances, GLuint *capsMask )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %p, %p, %p, %p)\n", queryId, queryNameLength, queryName, dataSize, noCounters, noInstances, capsMask );
  funcs->ext.p_glGetPerfQueryInfoINTEL( queryId, queryNameLength, queryName, dataSize, noCounters, noInstances, capsMask );
}

static void WINAPI glGetPixelMapxv( GLenum map, GLint size, GLfixed *values )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", map, size, values );
  funcs->ext.p_glGetPixelMapxv( map, size, values );
}

static void WINAPI glGetPixelTexGenParameterfvSGIS( GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", pname, params );
  funcs->ext.p_glGetPixelTexGenParameterfvSGIS( pname, params );
}

static void WINAPI glGetPixelTexGenParameterivSGIS( GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", pname, params );
  funcs->ext.p_glGetPixelTexGenParameterivSGIS( pname, params );
}

static void WINAPI glGetPixelTransformParameterfvEXT( GLenum target, GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetPixelTransformParameterfvEXT( target, pname, params );
}

static void WINAPI glGetPixelTransformParameterivEXT( GLenum target, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetPixelTransformParameterivEXT( target, pname, params );
}

static void WINAPI glGetPointerIndexedvEXT( GLenum target, GLuint index, void **data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, index, data );
  funcs->ext.p_glGetPointerIndexedvEXT( target, index, data );
}

static void WINAPI glGetPointeri_vEXT( GLenum pname, GLuint index, void **params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", pname, index, params );
  funcs->ext.p_glGetPointeri_vEXT( pname, index, params );
}

static void WINAPI glGetPointervEXT( GLenum pname, void **params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", pname, params );
  funcs->ext.p_glGetPointervEXT( pname, params );
}

static void WINAPI glGetProgramBinary( GLuint program, GLsizei bufSize, GLsizei *length, GLenum *binaryFormat, void *binary )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %p, %p)\n", program, bufSize, length, binaryFormat, binary );
  funcs->ext.p_glGetProgramBinary( program, bufSize, length, binaryFormat, binary );
}

static void WINAPI glGetProgramEnvParameterIivNV( GLenum target, GLuint index, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, index, params );
  funcs->ext.p_glGetProgramEnvParameterIivNV( target, index, params );
}

static void WINAPI glGetProgramEnvParameterIuivNV( GLenum target, GLuint index, GLuint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, index, params );
  funcs->ext.p_glGetProgramEnvParameterIuivNV( target, index, params );
}

static void WINAPI glGetProgramEnvParameterdvARB( GLenum target, GLuint index, GLdouble *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, index, params );
  funcs->ext.p_glGetProgramEnvParameterdvARB( target, index, params );
}

static void WINAPI glGetProgramEnvParameterfvARB( GLenum target, GLuint index, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, index, params );
  funcs->ext.p_glGetProgramEnvParameterfvARB( target, index, params );
}

static void WINAPI glGetProgramInfoLog( GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %p)\n", program, bufSize, length, infoLog );
  funcs->ext.p_glGetProgramInfoLog( program, bufSize, length, infoLog );
}

static void WINAPI glGetProgramInterfaceiv( GLuint program, GLenum programInterface, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, programInterface, pname, params );
  funcs->ext.p_glGetProgramInterfaceiv( program, programInterface, pname, params );
}

static void WINAPI glGetProgramLocalParameterIivNV( GLenum target, GLuint index, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, index, params );
  funcs->ext.p_glGetProgramLocalParameterIivNV( target, index, params );
}

static void WINAPI glGetProgramLocalParameterIuivNV( GLenum target, GLuint index, GLuint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, index, params );
  funcs->ext.p_glGetProgramLocalParameterIuivNV( target, index, params );
}

static void WINAPI glGetProgramLocalParameterdvARB( GLenum target, GLuint index, GLdouble *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, index, params );
  funcs->ext.p_glGetProgramLocalParameterdvARB( target, index, params );
}

static void WINAPI glGetProgramLocalParameterfvARB( GLenum target, GLuint index, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, index, params );
  funcs->ext.p_glGetProgramLocalParameterfvARB( target, index, params );
}

static void WINAPI glGetProgramNamedParameterdvNV( GLuint id, GLsizei len, const GLubyte *name, GLdouble *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %p)\n", id, len, name, params );
  funcs->ext.p_glGetProgramNamedParameterdvNV( id, len, name, params );
}

static void WINAPI glGetProgramNamedParameterfvNV( GLuint id, GLsizei len, const GLubyte *name, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %p)\n", id, len, name, params );
  funcs->ext.p_glGetProgramNamedParameterfvNV( id, len, name, params );
}

static void WINAPI glGetProgramParameterdvNV( GLenum target, GLuint index, GLenum pname, GLdouble *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", target, index, pname, params );
  funcs->ext.p_glGetProgramParameterdvNV( target, index, pname, params );
}

static void WINAPI glGetProgramParameterfvNV( GLenum target, GLuint index, GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", target, index, pname, params );
  funcs->ext.p_glGetProgramParameterfvNV( target, index, pname, params );
}

static void WINAPI glGetProgramPipelineInfoLog( GLuint pipeline, GLsizei bufSize, GLsizei *length, GLchar *infoLog )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %p)\n", pipeline, bufSize, length, infoLog );
  funcs->ext.p_glGetProgramPipelineInfoLog( pipeline, bufSize, length, infoLog );
}

static void WINAPI glGetProgramPipelineiv( GLuint pipeline, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", pipeline, pname, params );
  funcs->ext.p_glGetProgramPipelineiv( pipeline, pname, params );
}

static GLuint WINAPI glGetProgramResourceIndex( GLuint program, GLenum programInterface, const GLchar *name )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", program, programInterface, name );
  return funcs->ext.p_glGetProgramResourceIndex( program, programInterface, name );
}

static GLint WINAPI glGetProgramResourceLocation( GLuint program, GLenum programInterface, const GLchar *name )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", program, programInterface, name );
  return funcs->ext.p_glGetProgramResourceLocation( program, programInterface, name );
}

static GLint WINAPI glGetProgramResourceLocationIndex( GLuint program, GLenum programInterface, const GLchar *name )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", program, programInterface, name );
  return funcs->ext.p_glGetProgramResourceLocationIndex( program, programInterface, name );
}

static void WINAPI glGetProgramResourceName( GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p, %p)\n", program, programInterface, index, bufSize, length, name );
  funcs->ext.p_glGetProgramResourceName( program, programInterface, index, bufSize, length, name );
}

static void WINAPI glGetProgramResourcefvNV( GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum *props, GLsizei bufSize, GLsizei *length, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p, %d, %p, %p)\n", program, programInterface, index, propCount, props, bufSize, length, params );
  funcs->ext.p_glGetProgramResourcefvNV( program, programInterface, index, propCount, props, bufSize, length, params );
}

static void WINAPI glGetProgramResourceiv( GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum *props, GLsizei bufSize, GLsizei *length, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p, %d, %p, %p)\n", program, programInterface, index, propCount, props, bufSize, length, params );
  funcs->ext.p_glGetProgramResourceiv( program, programInterface, index, propCount, props, bufSize, length, params );
}

static void WINAPI glGetProgramStageiv( GLuint program, GLenum shadertype, GLenum pname, GLint *values )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, shadertype, pname, values );
  funcs->ext.p_glGetProgramStageiv( program, shadertype, pname, values );
}

static void WINAPI glGetProgramStringARB( GLenum target, GLenum pname, void *string )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, string );
  funcs->ext.p_glGetProgramStringARB( target, pname, string );
}

static void WINAPI glGetProgramStringNV( GLuint id, GLenum pname, GLubyte *program )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", id, pname, program );
  funcs->ext.p_glGetProgramStringNV( id, pname, program );
}

static void WINAPI glGetProgramSubroutineParameteruivNV( GLenum target, GLuint index, GLuint *param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, index, param );
  funcs->ext.p_glGetProgramSubroutineParameteruivNV( target, index, param );
}

static void WINAPI glGetProgramiv( GLuint program, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", program, pname, params );
  funcs->ext.p_glGetProgramiv( program, pname, params );
}

static void WINAPI glGetProgramivARB( GLenum target, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetProgramivARB( target, pname, params );
}

static void WINAPI glGetProgramivNV( GLuint id, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", id, pname, params );
  funcs->ext.p_glGetProgramivNV( id, pname, params );
}

static void WINAPI glGetQueryBufferObjecti64v( GLuint id, GLuint buffer, GLenum pname, GLintptr offset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %ld)\n", id, buffer, pname, offset );
  funcs->ext.p_glGetQueryBufferObjecti64v( id, buffer, pname, offset );
}

static void WINAPI glGetQueryBufferObjectiv( GLuint id, GLuint buffer, GLenum pname, GLintptr offset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %ld)\n", id, buffer, pname, offset );
  funcs->ext.p_glGetQueryBufferObjectiv( id, buffer, pname, offset );
}

static void WINAPI glGetQueryBufferObjectui64v( GLuint id, GLuint buffer, GLenum pname, GLintptr offset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %ld)\n", id, buffer, pname, offset );
  funcs->ext.p_glGetQueryBufferObjectui64v( id, buffer, pname, offset );
}

static void WINAPI glGetQueryBufferObjectuiv( GLuint id, GLuint buffer, GLenum pname, GLintptr offset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %ld)\n", id, buffer, pname, offset );
  funcs->ext.p_glGetQueryBufferObjectuiv( id, buffer, pname, offset );
}

static void WINAPI glGetQueryIndexediv( GLenum target, GLuint index, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", target, index, pname, params );
  funcs->ext.p_glGetQueryIndexediv( target, index, pname, params );
}

static void WINAPI glGetQueryObjecti64v( GLuint id, GLenum pname, GLint64 *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", id, pname, params );
  funcs->ext.p_glGetQueryObjecti64v( id, pname, params );
}

static void WINAPI glGetQueryObjecti64vEXT( GLuint id, GLenum pname, GLint64 *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", id, pname, params );
  funcs->ext.p_glGetQueryObjecti64vEXT( id, pname, params );
}

static void WINAPI glGetQueryObjectiv( GLuint id, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", id, pname, params );
  funcs->ext.p_glGetQueryObjectiv( id, pname, params );
}

static void WINAPI glGetQueryObjectivARB( GLuint id, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", id, pname, params );
  funcs->ext.p_glGetQueryObjectivARB( id, pname, params );
}

static void WINAPI glGetQueryObjectui64v( GLuint id, GLenum pname, GLuint64 *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", id, pname, params );
  funcs->ext.p_glGetQueryObjectui64v( id, pname, params );
}

static void WINAPI glGetQueryObjectui64vEXT( GLuint id, GLenum pname, GLuint64 *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", id, pname, params );
  funcs->ext.p_glGetQueryObjectui64vEXT( id, pname, params );
}

static void WINAPI glGetQueryObjectuiv( GLuint id, GLenum pname, GLuint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", id, pname, params );
  funcs->ext.p_glGetQueryObjectuiv( id, pname, params );
}

static void WINAPI glGetQueryObjectuivARB( GLuint id, GLenum pname, GLuint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", id, pname, params );
  funcs->ext.p_glGetQueryObjectuivARB( id, pname, params );
}

static void WINAPI glGetQueryiv( GLenum target, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetQueryiv( target, pname, params );
}

static void WINAPI glGetQueryivARB( GLenum target, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetQueryivARB( target, pname, params );
}

static void WINAPI glGetRenderbufferParameteriv( GLenum target, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetRenderbufferParameteriv( target, pname, params );
}

static void WINAPI glGetRenderbufferParameterivEXT( GLenum target, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetRenderbufferParameterivEXT( target, pname, params );
}

static void WINAPI glGetSamplerParameterIiv( GLuint sampler, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", sampler, pname, params );
  funcs->ext.p_glGetSamplerParameterIiv( sampler, pname, params );
}

static void WINAPI glGetSamplerParameterIuiv( GLuint sampler, GLenum pname, GLuint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", sampler, pname, params );
  funcs->ext.p_glGetSamplerParameterIuiv( sampler, pname, params );
}

static void WINAPI glGetSamplerParameterfv( GLuint sampler, GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", sampler, pname, params );
  funcs->ext.p_glGetSamplerParameterfv( sampler, pname, params );
}

static void WINAPI glGetSamplerParameteriv( GLuint sampler, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", sampler, pname, params );
  funcs->ext.p_glGetSamplerParameteriv( sampler, pname, params );
}

static void WINAPI glGetSemaphoreParameterui64vEXT( GLuint semaphore, GLenum pname, GLuint64 *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", semaphore, pname, params );
  funcs->ext.p_glGetSemaphoreParameterui64vEXT( semaphore, pname, params );
}

static void WINAPI glGetSeparableFilter( GLenum target, GLenum format, GLenum type, void *row, void *column, void *span )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p, %p, %p)\n", target, format, type, row, column, span );
  funcs->ext.p_glGetSeparableFilter( target, format, type, row, column, span );
}

static void WINAPI glGetSeparableFilterEXT( GLenum target, GLenum format, GLenum type, void *row, void *column, void *span )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p, %p, %p)\n", target, format, type, row, column, span );
  funcs->ext.p_glGetSeparableFilterEXT( target, format, type, row, column, span );
}

static void WINAPI glGetShaderInfoLog( GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %p)\n", shader, bufSize, length, infoLog );
  funcs->ext.p_glGetShaderInfoLog( shader, bufSize, length, infoLog );
}

static void WINAPI glGetShaderPrecisionFormat( GLenum shadertype, GLenum precisiontype, GLint *range, GLint *precision )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %p)\n", shadertype, precisiontype, range, precision );
  funcs->ext.p_glGetShaderPrecisionFormat( shadertype, precisiontype, range, precision );
}

static void WINAPI glGetShaderSource( GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *source )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %p)\n", shader, bufSize, length, source );
  funcs->ext.p_glGetShaderSource( shader, bufSize, length, source );
}

static void WINAPI glGetShaderSourceARB( GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *source )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %p)\n", obj, maxLength, length, source );
  funcs->ext.p_glGetShaderSourceARB( obj, maxLength, length, source );
}

static void WINAPI glGetShaderiv( GLuint shader, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", shader, pname, params );
  funcs->ext.p_glGetShaderiv( shader, pname, params );
}

static void WINAPI glGetSharpenTexFuncSGIS( GLenum target, GLfloat *points )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, points );
  funcs->ext.p_glGetSharpenTexFuncSGIS( target, points );
}

static GLushort WINAPI glGetStageIndexNV( GLenum shadertype )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", shadertype );
  return funcs->ext.p_glGetStageIndexNV( shadertype );
}

static GLuint WINAPI glGetSubroutineIndex( GLuint program, GLenum shadertype, const GLchar *name )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", program, shadertype, name );
  return funcs->ext.p_glGetSubroutineIndex( program, shadertype, name );
}

static GLint WINAPI glGetSubroutineUniformLocation( GLuint program, GLenum shadertype, const GLchar *name )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", program, shadertype, name );
  return funcs->ext.p_glGetSubroutineUniformLocation( program, shadertype, name );
}

static void WINAPI glGetSynciv( GLsync sync, GLenum pname, GLsizei bufSize, GLsizei *length, GLint *values )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p, %d, %d, %p, %p)\n", sync, pname, bufSize, length, values );
  funcs->ext.p_glGetSynciv( sync, pname, bufSize, length, values );
}

static void WINAPI glGetTexBumpParameterfvATI( GLenum pname, GLfloat *param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", pname, param );
  funcs->ext.p_glGetTexBumpParameterfvATI( pname, param );
}

static void WINAPI glGetTexBumpParameterivATI( GLenum pname, GLint *param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", pname, param );
  funcs->ext.p_glGetTexBumpParameterivATI( pname, param );
}

static void WINAPI glGetTexEnvxvOES( GLenum target, GLenum pname, GLfixed *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetTexEnvxvOES( target, pname, params );
}

static void WINAPI glGetTexFilterFuncSGIS( GLenum target, GLenum filter, GLfloat *weights )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, filter, weights );
  funcs->ext.p_glGetTexFilterFuncSGIS( target, filter, weights );
}

static void WINAPI glGetTexGenxvOES( GLenum coord, GLenum pname, GLfixed *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", coord, pname, params );
  funcs->ext.p_glGetTexGenxvOES( coord, pname, params );
}

static void WINAPI glGetTexLevelParameterxvOES( GLenum target, GLint level, GLenum pname, GLfixed *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", target, level, pname, params );
  funcs->ext.p_glGetTexLevelParameterxvOES( target, level, pname, params );
}

static void WINAPI glGetTexParameterIiv( GLenum target, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetTexParameterIiv( target, pname, params );
}

static void WINAPI glGetTexParameterIivEXT( GLenum target, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetTexParameterIivEXT( target, pname, params );
}

static void WINAPI glGetTexParameterIuiv( GLenum target, GLenum pname, GLuint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetTexParameterIuiv( target, pname, params );
}

static void WINAPI glGetTexParameterIuivEXT( GLenum target, GLenum pname, GLuint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetTexParameterIuivEXT( target, pname, params );
}

static void WINAPI glGetTexParameterPointervAPPLE( GLenum target, GLenum pname, void **params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetTexParameterPointervAPPLE( target, pname, params );
}

static void WINAPI glGetTexParameterxvOES( GLenum target, GLenum pname, GLfixed *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glGetTexParameterxvOES( target, pname, params );
}

static GLuint64 WINAPI glGetTextureHandleARB( GLuint texture )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", texture );
  return funcs->ext.p_glGetTextureHandleARB( texture );
}

static GLuint64 WINAPI glGetTextureHandleNV( GLuint texture )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", texture );
  return funcs->ext.p_glGetTextureHandleNV( texture );
}

static void WINAPI glGetTextureImage( GLuint texture, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *pixels )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %p)\n", texture, level, format, type, bufSize, pixels );
  funcs->ext.p_glGetTextureImage( texture, level, format, type, bufSize, pixels );
}

static void WINAPI glGetTextureImageEXT( GLuint texture, GLenum target, GLint level, GLenum format, GLenum type, void *pixels )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %p)\n", texture, target, level, format, type, pixels );
  funcs->ext.p_glGetTextureImageEXT( texture, target, level, format, type, pixels );
}

static void WINAPI glGetTextureLevelParameterfv( GLuint texture, GLint level, GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", texture, level, pname, params );
  funcs->ext.p_glGetTextureLevelParameterfv( texture, level, pname, params );
}

static void WINAPI glGetTextureLevelParameterfvEXT( GLuint texture, GLenum target, GLint level, GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", texture, target, level, pname, params );
  funcs->ext.p_glGetTextureLevelParameterfvEXT( texture, target, level, pname, params );
}

static void WINAPI glGetTextureLevelParameteriv( GLuint texture, GLint level, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", texture, level, pname, params );
  funcs->ext.p_glGetTextureLevelParameteriv( texture, level, pname, params );
}

static void WINAPI glGetTextureLevelParameterivEXT( GLuint texture, GLenum target, GLint level, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", texture, target, level, pname, params );
  funcs->ext.p_glGetTextureLevelParameterivEXT( texture, target, level, pname, params );
}

static void WINAPI glGetTextureParameterIiv( GLuint texture, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", texture, pname, params );
  funcs->ext.p_glGetTextureParameterIiv( texture, pname, params );
}

static void WINAPI glGetTextureParameterIivEXT( GLuint texture, GLenum target, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", texture, target, pname, params );
  funcs->ext.p_glGetTextureParameterIivEXT( texture, target, pname, params );
}

static void WINAPI glGetTextureParameterIuiv( GLuint texture, GLenum pname, GLuint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", texture, pname, params );
  funcs->ext.p_glGetTextureParameterIuiv( texture, pname, params );
}

static void WINAPI glGetTextureParameterIuivEXT( GLuint texture, GLenum target, GLenum pname, GLuint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", texture, target, pname, params );
  funcs->ext.p_glGetTextureParameterIuivEXT( texture, target, pname, params );
}

static void WINAPI glGetTextureParameterfv( GLuint texture, GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", texture, pname, params );
  funcs->ext.p_glGetTextureParameterfv( texture, pname, params );
}

static void WINAPI glGetTextureParameterfvEXT( GLuint texture, GLenum target, GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", texture, target, pname, params );
  funcs->ext.p_glGetTextureParameterfvEXT( texture, target, pname, params );
}

static void WINAPI glGetTextureParameteriv( GLuint texture, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", texture, pname, params );
  funcs->ext.p_glGetTextureParameteriv( texture, pname, params );
}

static void WINAPI glGetTextureParameterivEXT( GLuint texture, GLenum target, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", texture, target, pname, params );
  funcs->ext.p_glGetTextureParameterivEXT( texture, target, pname, params );
}

static GLuint64 WINAPI glGetTextureSamplerHandleARB( GLuint texture, GLuint sampler )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", texture, sampler );
  return funcs->ext.p_glGetTextureSamplerHandleARB( texture, sampler );
}

static GLuint64 WINAPI glGetTextureSamplerHandleNV( GLuint texture, GLuint sampler )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", texture, sampler );
  return funcs->ext.p_glGetTextureSamplerHandleNV( texture, sampler );
}

static void WINAPI glGetTextureSubImage( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, GLsizei bufSize, void *pixels )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, bufSize, pixels );
  funcs->ext.p_glGetTextureSubImage( texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, bufSize, pixels );
}

static void WINAPI glGetTrackMatrixivNV( GLenum target, GLuint address, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", target, address, pname, params );
  funcs->ext.p_glGetTrackMatrixivNV( target, address, pname, params );
}

static void WINAPI glGetTransformFeedbackVarying( GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p, %p, %p, %p)\n", program, index, bufSize, length, size, type, name );
  funcs->ext.p_glGetTransformFeedbackVarying( program, index, bufSize, length, size, type, name );
}

static void WINAPI glGetTransformFeedbackVaryingEXT( GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p, %p, %p, %p)\n", program, index, bufSize, length, size, type, name );
  funcs->ext.p_glGetTransformFeedbackVaryingEXT( program, index, bufSize, length, size, type, name );
}

static void WINAPI glGetTransformFeedbackVaryingNV( GLuint program, GLuint index, GLint *location )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", program, index, location );
  funcs->ext.p_glGetTransformFeedbackVaryingNV( program, index, location );
}

static void WINAPI glGetTransformFeedbacki64_v( GLuint xfb, GLenum pname, GLuint index, GLint64 *param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", xfb, pname, index, param );
  funcs->ext.p_glGetTransformFeedbacki64_v( xfb, pname, index, param );
}

static void WINAPI glGetTransformFeedbacki_v( GLuint xfb, GLenum pname, GLuint index, GLint *param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", xfb, pname, index, param );
  funcs->ext.p_glGetTransformFeedbacki_v( xfb, pname, index, param );
}

static void WINAPI glGetTransformFeedbackiv( GLuint xfb, GLenum pname, GLint *param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", xfb, pname, param );
  funcs->ext.p_glGetTransformFeedbackiv( xfb, pname, param );
}

static GLuint WINAPI glGetUniformBlockIndex( GLuint program, const GLchar *uniformBlockName )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", program, uniformBlockName );
  return funcs->ext.p_glGetUniformBlockIndex( program, uniformBlockName );
}

static GLint WINAPI glGetUniformBufferSizeEXT( GLuint program, GLint location )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", program, location );
  return funcs->ext.p_glGetUniformBufferSizeEXT( program, location );
}

static void WINAPI glGetUniformIndices( GLuint program, GLsizei uniformCount, const GLchar *const*uniformNames, GLuint *uniformIndices )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %p)\n", program, uniformCount, uniformNames, uniformIndices );
  funcs->ext.p_glGetUniformIndices( program, uniformCount, uniformNames, uniformIndices );
}

static GLint WINAPI glGetUniformLocation( GLuint program, const GLchar *name )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", program, name );
  return funcs->ext.p_glGetUniformLocation( program, name );
}

static GLint WINAPI glGetUniformLocationARB( GLhandleARB programObj, const GLcharARB *name )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", programObj, name );
  return funcs->ext.p_glGetUniformLocationARB( programObj, name );
}

static GLintptr WINAPI glGetUniformOffsetEXT( GLuint program, GLint location )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", program, location );
  return funcs->ext.p_glGetUniformOffsetEXT( program, location );
}

static void WINAPI glGetUniformSubroutineuiv( GLenum shadertype, GLint location, GLuint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", shadertype, location, params );
  funcs->ext.p_glGetUniformSubroutineuiv( shadertype, location, params );
}

static void WINAPI glGetUniformdv( GLuint program, GLint location, GLdouble *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", program, location, params );
  funcs->ext.p_glGetUniformdv( program, location, params );
}

static void WINAPI glGetUniformfv( GLuint program, GLint location, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", program, location, params );
  funcs->ext.p_glGetUniformfv( program, location, params );
}

static void WINAPI glGetUniformfvARB( GLhandleARB programObj, GLint location, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", programObj, location, params );
  funcs->ext.p_glGetUniformfvARB( programObj, location, params );
}

static void WINAPI glGetUniformi64vARB( GLuint program, GLint location, GLint64 *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", program, location, params );
  funcs->ext.p_glGetUniformi64vARB( program, location, params );
}

static void WINAPI glGetUniformi64vNV( GLuint program, GLint location, GLint64EXT *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", program, location, params );
  funcs->ext.p_glGetUniformi64vNV( program, location, params );
}

static void WINAPI glGetUniformiv( GLuint program, GLint location, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", program, location, params );
  funcs->ext.p_glGetUniformiv( program, location, params );
}

static void WINAPI glGetUniformivARB( GLhandleARB programObj, GLint location, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", programObj, location, params );
  funcs->ext.p_glGetUniformivARB( programObj, location, params );
}

static void WINAPI glGetUniformui64vARB( GLuint program, GLint location, GLuint64 *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", program, location, params );
  funcs->ext.p_glGetUniformui64vARB( program, location, params );
}

static void WINAPI glGetUniformui64vNV( GLuint program, GLint location, GLuint64EXT *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", program, location, params );
  funcs->ext.p_glGetUniformui64vNV( program, location, params );
}

static void WINAPI glGetUniformuiv( GLuint program, GLint location, GLuint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", program, location, params );
  funcs->ext.p_glGetUniformuiv( program, location, params );
}

static void WINAPI glGetUniformuivEXT( GLuint program, GLint location, GLuint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", program, location, params );
  funcs->ext.p_glGetUniformuivEXT( program, location, params );
}

static void WINAPI glGetUnsignedBytei_vEXT( GLenum target, GLuint index, GLubyte *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, index, data );
  funcs->ext.p_glGetUnsignedBytei_vEXT( target, index, data );
}

static void WINAPI glGetUnsignedBytevEXT( GLenum pname, GLubyte *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", pname, data );
  funcs->ext.p_glGetUnsignedBytevEXT( pname, data );
}

static void WINAPI glGetVariantArrayObjectfvATI( GLuint id, GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", id, pname, params );
  funcs->ext.p_glGetVariantArrayObjectfvATI( id, pname, params );
}

static void WINAPI glGetVariantArrayObjectivATI( GLuint id, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", id, pname, params );
  funcs->ext.p_glGetVariantArrayObjectivATI( id, pname, params );
}

static void WINAPI glGetVariantBooleanvEXT( GLuint id, GLenum value, GLboolean *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", id, value, data );
  funcs->ext.p_glGetVariantBooleanvEXT( id, value, data );
}

static void WINAPI glGetVariantFloatvEXT( GLuint id, GLenum value, GLfloat *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", id, value, data );
  funcs->ext.p_glGetVariantFloatvEXT( id, value, data );
}

static void WINAPI glGetVariantIntegervEXT( GLuint id, GLenum value, GLint *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", id, value, data );
  funcs->ext.p_glGetVariantIntegervEXT( id, value, data );
}

static void WINAPI glGetVariantPointervEXT( GLuint id, GLenum value, void **data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", id, value, data );
  funcs->ext.p_glGetVariantPointervEXT( id, value, data );
}

static GLint WINAPI glGetVaryingLocationNV( GLuint program, const GLchar *name )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", program, name );
  return funcs->ext.p_glGetVaryingLocationNV( program, name );
}

static void WINAPI glGetVertexArrayIndexed64iv( GLuint vaobj, GLuint index, GLenum pname, GLint64 *param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", vaobj, index, pname, param );
  funcs->ext.p_glGetVertexArrayIndexed64iv( vaobj, index, pname, param );
}

static void WINAPI glGetVertexArrayIndexediv( GLuint vaobj, GLuint index, GLenum pname, GLint *param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", vaobj, index, pname, param );
  funcs->ext.p_glGetVertexArrayIndexediv( vaobj, index, pname, param );
}

static void WINAPI glGetVertexArrayIntegeri_vEXT( GLuint vaobj, GLuint index, GLenum pname, GLint *param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", vaobj, index, pname, param );
  funcs->ext.p_glGetVertexArrayIntegeri_vEXT( vaobj, index, pname, param );
}

static void WINAPI glGetVertexArrayIntegervEXT( GLuint vaobj, GLenum pname, GLint *param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", vaobj, pname, param );
  funcs->ext.p_glGetVertexArrayIntegervEXT( vaobj, pname, param );
}

static void WINAPI glGetVertexArrayPointeri_vEXT( GLuint vaobj, GLuint index, GLenum pname, void **param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", vaobj, index, pname, param );
  funcs->ext.p_glGetVertexArrayPointeri_vEXT( vaobj, index, pname, param );
}

static void WINAPI glGetVertexArrayPointervEXT( GLuint vaobj, GLenum pname, void **param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", vaobj, pname, param );
  funcs->ext.p_glGetVertexArrayPointervEXT( vaobj, pname, param );
}

static void WINAPI glGetVertexArrayiv( GLuint vaobj, GLenum pname, GLint *param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", vaobj, pname, param );
  funcs->ext.p_glGetVertexArrayiv( vaobj, pname, param );
}

static void WINAPI glGetVertexAttribArrayObjectfvATI( GLuint index, GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, pname, params );
  funcs->ext.p_glGetVertexAttribArrayObjectfvATI( index, pname, params );
}

static void WINAPI glGetVertexAttribArrayObjectivATI( GLuint index, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, pname, params );
  funcs->ext.p_glGetVertexAttribArrayObjectivATI( index, pname, params );
}

static void WINAPI glGetVertexAttribIiv( GLuint index, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, pname, params );
  funcs->ext.p_glGetVertexAttribIiv( index, pname, params );
}

static void WINAPI glGetVertexAttribIivEXT( GLuint index, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, pname, params );
  funcs->ext.p_glGetVertexAttribIivEXT( index, pname, params );
}

static void WINAPI glGetVertexAttribIuiv( GLuint index, GLenum pname, GLuint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, pname, params );
  funcs->ext.p_glGetVertexAttribIuiv( index, pname, params );
}

static void WINAPI glGetVertexAttribIuivEXT( GLuint index, GLenum pname, GLuint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, pname, params );
  funcs->ext.p_glGetVertexAttribIuivEXT( index, pname, params );
}

static void WINAPI glGetVertexAttribLdv( GLuint index, GLenum pname, GLdouble *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, pname, params );
  funcs->ext.p_glGetVertexAttribLdv( index, pname, params );
}

static void WINAPI glGetVertexAttribLdvEXT( GLuint index, GLenum pname, GLdouble *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, pname, params );
  funcs->ext.p_glGetVertexAttribLdvEXT( index, pname, params );
}

static void WINAPI glGetVertexAttribLi64vNV( GLuint index, GLenum pname, GLint64EXT *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, pname, params );
  funcs->ext.p_glGetVertexAttribLi64vNV( index, pname, params );
}

static void WINAPI glGetVertexAttribLui64vARB( GLuint index, GLenum pname, GLuint64EXT *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, pname, params );
  funcs->ext.p_glGetVertexAttribLui64vARB( index, pname, params );
}

static void WINAPI glGetVertexAttribLui64vNV( GLuint index, GLenum pname, GLuint64EXT *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, pname, params );
  funcs->ext.p_glGetVertexAttribLui64vNV( index, pname, params );
}

static void WINAPI glGetVertexAttribPointerv( GLuint index, GLenum pname, void **pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, pname, pointer );
  funcs->ext.p_glGetVertexAttribPointerv( index, pname, pointer );
}

static void WINAPI glGetVertexAttribPointervARB( GLuint index, GLenum pname, void **pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, pname, pointer );
  funcs->ext.p_glGetVertexAttribPointervARB( index, pname, pointer );
}

static void WINAPI glGetVertexAttribPointervNV( GLuint index, GLenum pname, void **pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, pname, pointer );
  funcs->ext.p_glGetVertexAttribPointervNV( index, pname, pointer );
}

static void WINAPI glGetVertexAttribdv( GLuint index, GLenum pname, GLdouble *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, pname, params );
  funcs->ext.p_glGetVertexAttribdv( index, pname, params );
}

static void WINAPI glGetVertexAttribdvARB( GLuint index, GLenum pname, GLdouble *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, pname, params );
  funcs->ext.p_glGetVertexAttribdvARB( index, pname, params );
}

static void WINAPI glGetVertexAttribdvNV( GLuint index, GLenum pname, GLdouble *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, pname, params );
  funcs->ext.p_glGetVertexAttribdvNV( index, pname, params );
}

static void WINAPI glGetVertexAttribfv( GLuint index, GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, pname, params );
  funcs->ext.p_glGetVertexAttribfv( index, pname, params );
}

static void WINAPI glGetVertexAttribfvARB( GLuint index, GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, pname, params );
  funcs->ext.p_glGetVertexAttribfvARB( index, pname, params );
}

static void WINAPI glGetVertexAttribfvNV( GLuint index, GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, pname, params );
  funcs->ext.p_glGetVertexAttribfvNV( index, pname, params );
}

static void WINAPI glGetVertexAttribiv( GLuint index, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, pname, params );
  funcs->ext.p_glGetVertexAttribiv( index, pname, params );
}

static void WINAPI glGetVertexAttribivARB( GLuint index, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, pname, params );
  funcs->ext.p_glGetVertexAttribivARB( index, pname, params );
}

static void WINAPI glGetVertexAttribivNV( GLuint index, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, pname, params );
  funcs->ext.p_glGetVertexAttribivNV( index, pname, params );
}

static void WINAPI glGetVideoCaptureStreamdvNV( GLuint video_capture_slot, GLuint stream, GLenum pname, GLdouble *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", video_capture_slot, stream, pname, params );
  funcs->ext.p_glGetVideoCaptureStreamdvNV( video_capture_slot, stream, pname, params );
}

static void WINAPI glGetVideoCaptureStreamfvNV( GLuint video_capture_slot, GLuint stream, GLenum pname, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", video_capture_slot, stream, pname, params );
  funcs->ext.p_glGetVideoCaptureStreamfvNV( video_capture_slot, stream, pname, params );
}

static void WINAPI glGetVideoCaptureStreamivNV( GLuint video_capture_slot, GLuint stream, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", video_capture_slot, stream, pname, params );
  funcs->ext.p_glGetVideoCaptureStreamivNV( video_capture_slot, stream, pname, params );
}

static void WINAPI glGetVideoCaptureivNV( GLuint video_capture_slot, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", video_capture_slot, pname, params );
  funcs->ext.p_glGetVideoCaptureivNV( video_capture_slot, pname, params );
}

static void WINAPI glGetVideoi64vNV( GLuint video_slot, GLenum pname, GLint64EXT *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", video_slot, pname, params );
  funcs->ext.p_glGetVideoi64vNV( video_slot, pname, params );
}

static void WINAPI glGetVideoivNV( GLuint video_slot, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", video_slot, pname, params );
  funcs->ext.p_glGetVideoivNV( video_slot, pname, params );
}

static void WINAPI glGetVideoui64vNV( GLuint video_slot, GLenum pname, GLuint64EXT *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", video_slot, pname, params );
  funcs->ext.p_glGetVideoui64vNV( video_slot, pname, params );
}

static void WINAPI glGetVideouivNV( GLuint video_slot, GLenum pname, GLuint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", video_slot, pname, params );
  funcs->ext.p_glGetVideouivNV( video_slot, pname, params );
}

static GLVULKANPROCNV WINAPI glGetVkProcAddrNV( const GLchar *name )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", name );
  return funcs->ext.p_glGetVkProcAddrNV( name );
}

static void WINAPI glGetnColorTable( GLenum target, GLenum format, GLenum type, GLsizei bufSize, void *table )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", target, format, type, bufSize, table );
  funcs->ext.p_glGetnColorTable( target, format, type, bufSize, table );
}

static void WINAPI glGetnColorTableARB( GLenum target, GLenum format, GLenum type, GLsizei bufSize, void *table )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", target, format, type, bufSize, table );
  funcs->ext.p_glGetnColorTableARB( target, format, type, bufSize, table );
}

static void WINAPI glGetnCompressedTexImage( GLenum target, GLint lod, GLsizei bufSize, void *pixels )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", target, lod, bufSize, pixels );
  funcs->ext.p_glGetnCompressedTexImage( target, lod, bufSize, pixels );
}

static void WINAPI glGetnCompressedTexImageARB( GLenum target, GLint lod, GLsizei bufSize, void *img )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", target, lod, bufSize, img );
  funcs->ext.p_glGetnCompressedTexImageARB( target, lod, bufSize, img );
}

static void WINAPI glGetnConvolutionFilter( GLenum target, GLenum format, GLenum type, GLsizei bufSize, void *image )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", target, format, type, bufSize, image );
  funcs->ext.p_glGetnConvolutionFilter( target, format, type, bufSize, image );
}

static void WINAPI glGetnConvolutionFilterARB( GLenum target, GLenum format, GLenum type, GLsizei bufSize, void *image )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", target, format, type, bufSize, image );
  funcs->ext.p_glGetnConvolutionFilterARB( target, format, type, bufSize, image );
}

static void WINAPI glGetnHistogram( GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, void *values )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %p)\n", target, reset, format, type, bufSize, values );
  funcs->ext.p_glGetnHistogram( target, reset, format, type, bufSize, values );
}

static void WINAPI glGetnHistogramARB( GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, void *values )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %p)\n", target, reset, format, type, bufSize, values );
  funcs->ext.p_glGetnHistogramARB( target, reset, format, type, bufSize, values );
}

static void WINAPI glGetnMapdv( GLenum target, GLenum query, GLsizei bufSize, GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", target, query, bufSize, v );
  funcs->ext.p_glGetnMapdv( target, query, bufSize, v );
}

static void WINAPI glGetnMapdvARB( GLenum target, GLenum query, GLsizei bufSize, GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", target, query, bufSize, v );
  funcs->ext.p_glGetnMapdvARB( target, query, bufSize, v );
}

static void WINAPI glGetnMapfv( GLenum target, GLenum query, GLsizei bufSize, GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", target, query, bufSize, v );
  funcs->ext.p_glGetnMapfv( target, query, bufSize, v );
}

static void WINAPI glGetnMapfvARB( GLenum target, GLenum query, GLsizei bufSize, GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", target, query, bufSize, v );
  funcs->ext.p_glGetnMapfvARB( target, query, bufSize, v );
}

static void WINAPI glGetnMapiv( GLenum target, GLenum query, GLsizei bufSize, GLint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", target, query, bufSize, v );
  funcs->ext.p_glGetnMapiv( target, query, bufSize, v );
}

static void WINAPI glGetnMapivARB( GLenum target, GLenum query, GLsizei bufSize, GLint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", target, query, bufSize, v );
  funcs->ext.p_glGetnMapivARB( target, query, bufSize, v );
}

static void WINAPI glGetnMinmax( GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, void *values )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %p)\n", target, reset, format, type, bufSize, values );
  funcs->ext.p_glGetnMinmax( target, reset, format, type, bufSize, values );
}

static void WINAPI glGetnMinmaxARB( GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, void *values )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %p)\n", target, reset, format, type, bufSize, values );
  funcs->ext.p_glGetnMinmaxARB( target, reset, format, type, bufSize, values );
}

static void WINAPI glGetnPixelMapfv( GLenum map, GLsizei bufSize, GLfloat *values )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", map, bufSize, values );
  funcs->ext.p_glGetnPixelMapfv( map, bufSize, values );
}

static void WINAPI glGetnPixelMapfvARB( GLenum map, GLsizei bufSize, GLfloat *values )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", map, bufSize, values );
  funcs->ext.p_glGetnPixelMapfvARB( map, bufSize, values );
}

static void WINAPI glGetnPixelMapuiv( GLenum map, GLsizei bufSize, GLuint *values )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", map, bufSize, values );
  funcs->ext.p_glGetnPixelMapuiv( map, bufSize, values );
}

static void WINAPI glGetnPixelMapuivARB( GLenum map, GLsizei bufSize, GLuint *values )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", map, bufSize, values );
  funcs->ext.p_glGetnPixelMapuivARB( map, bufSize, values );
}

static void WINAPI glGetnPixelMapusv( GLenum map, GLsizei bufSize, GLushort *values )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", map, bufSize, values );
  funcs->ext.p_glGetnPixelMapusv( map, bufSize, values );
}

static void WINAPI glGetnPixelMapusvARB( GLenum map, GLsizei bufSize, GLushort *values )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", map, bufSize, values );
  funcs->ext.p_glGetnPixelMapusvARB( map, bufSize, values );
}

static void WINAPI glGetnPolygonStipple( GLsizei bufSize, GLubyte *pattern )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", bufSize, pattern );
  funcs->ext.p_glGetnPolygonStipple( bufSize, pattern );
}

static void WINAPI glGetnPolygonStippleARB( GLsizei bufSize, GLubyte *pattern )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", bufSize, pattern );
  funcs->ext.p_glGetnPolygonStippleARB( bufSize, pattern );
}

static void WINAPI glGetnSeparableFilter( GLenum target, GLenum format, GLenum type, GLsizei rowBufSize, void *row, GLsizei columnBufSize, void *column, void *span )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p, %d, %p, %p)\n", target, format, type, rowBufSize, row, columnBufSize, column, span );
  funcs->ext.p_glGetnSeparableFilter( target, format, type, rowBufSize, row, columnBufSize, column, span );
}

static void WINAPI glGetnSeparableFilterARB( GLenum target, GLenum format, GLenum type, GLsizei rowBufSize, void *row, GLsizei columnBufSize, void *column, void *span )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p, %d, %p, %p)\n", target, format, type, rowBufSize, row, columnBufSize, column, span );
  funcs->ext.p_glGetnSeparableFilterARB( target, format, type, rowBufSize, row, columnBufSize, column, span );
}

static void WINAPI glGetnTexImage( GLenum target, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *pixels )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %p)\n", target, level, format, type, bufSize, pixels );
  funcs->ext.p_glGetnTexImage( target, level, format, type, bufSize, pixels );
}

static void WINAPI glGetnTexImageARB( GLenum target, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *img )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %p)\n", target, level, format, type, bufSize, img );
  funcs->ext.p_glGetnTexImageARB( target, level, format, type, bufSize, img );
}

static void WINAPI glGetnUniformdv( GLuint program, GLint location, GLsizei bufSize, GLdouble *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, bufSize, params );
  funcs->ext.p_glGetnUniformdv( program, location, bufSize, params );
}

static void WINAPI glGetnUniformdvARB( GLuint program, GLint location, GLsizei bufSize, GLdouble *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, bufSize, params );
  funcs->ext.p_glGetnUniformdvARB( program, location, bufSize, params );
}

static void WINAPI glGetnUniformfv( GLuint program, GLint location, GLsizei bufSize, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, bufSize, params );
  funcs->ext.p_glGetnUniformfv( program, location, bufSize, params );
}

static void WINAPI glGetnUniformfvARB( GLuint program, GLint location, GLsizei bufSize, GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, bufSize, params );
  funcs->ext.p_glGetnUniformfvARB( program, location, bufSize, params );
}

static void WINAPI glGetnUniformi64vARB( GLuint program, GLint location, GLsizei bufSize, GLint64 *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, bufSize, params );
  funcs->ext.p_glGetnUniformi64vARB( program, location, bufSize, params );
}

static void WINAPI glGetnUniformiv( GLuint program, GLint location, GLsizei bufSize, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, bufSize, params );
  funcs->ext.p_glGetnUniformiv( program, location, bufSize, params );
}

static void WINAPI glGetnUniformivARB( GLuint program, GLint location, GLsizei bufSize, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, bufSize, params );
  funcs->ext.p_glGetnUniformivARB( program, location, bufSize, params );
}

static void WINAPI glGetnUniformui64vARB( GLuint program, GLint location, GLsizei bufSize, GLuint64 *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, bufSize, params );
  funcs->ext.p_glGetnUniformui64vARB( program, location, bufSize, params );
}

static void WINAPI glGetnUniformuiv( GLuint program, GLint location, GLsizei bufSize, GLuint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, bufSize, params );
  funcs->ext.p_glGetnUniformuiv( program, location, bufSize, params );
}

static void WINAPI glGetnUniformuivARB( GLuint program, GLint location, GLsizei bufSize, GLuint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, bufSize, params );
  funcs->ext.p_glGetnUniformuivARB( program, location, bufSize, params );
}

static void WINAPI glGlobalAlphaFactorbSUN( GLbyte factor )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", factor );
  funcs->ext.p_glGlobalAlphaFactorbSUN( factor );
}

static void WINAPI glGlobalAlphaFactordSUN( GLdouble factor )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f)\n", factor );
  funcs->ext.p_glGlobalAlphaFactordSUN( factor );
}

static void WINAPI glGlobalAlphaFactorfSUN( GLfloat factor )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f)\n", factor );
  funcs->ext.p_glGlobalAlphaFactorfSUN( factor );
}

static void WINAPI glGlobalAlphaFactoriSUN( GLint factor )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", factor );
  funcs->ext.p_glGlobalAlphaFactoriSUN( factor );
}

static void WINAPI glGlobalAlphaFactorsSUN( GLshort factor )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", factor );
  funcs->ext.p_glGlobalAlphaFactorsSUN( factor );
}

static void WINAPI glGlobalAlphaFactorubSUN( GLubyte factor )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", factor );
  funcs->ext.p_glGlobalAlphaFactorubSUN( factor );
}

static void WINAPI glGlobalAlphaFactoruiSUN( GLuint factor )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", factor );
  funcs->ext.p_glGlobalAlphaFactoruiSUN( factor );
}

static void WINAPI glGlobalAlphaFactorusSUN( GLushort factor )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", factor );
  funcs->ext.p_glGlobalAlphaFactorusSUN( factor );
}

static void WINAPI glHintPGI( GLenum target, GLint mode )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", target, mode );
  funcs->ext.p_glHintPGI( target, mode );
}

static void WINAPI glHistogram( GLenum target, GLsizei width, GLenum internalformat, GLboolean sink )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", target, width, internalformat, sink );
  funcs->ext.p_glHistogram( target, width, internalformat, sink );
}

static void WINAPI glHistogramEXT( GLenum target, GLsizei width, GLenum internalformat, GLboolean sink )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", target, width, internalformat, sink );
  funcs->ext.p_glHistogramEXT( target, width, internalformat, sink );
}

static void WINAPI glIglooInterfaceSGIX( GLenum pname, const void *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", pname, params );
  funcs->ext.p_glIglooInterfaceSGIX( pname, params );
}

static void WINAPI glImageTransformParameterfHP( GLenum target, GLenum pname, GLfloat param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %f)\n", target, pname, param );
  funcs->ext.p_glImageTransformParameterfHP( target, pname, param );
}

static void WINAPI glImageTransformParameterfvHP( GLenum target, GLenum pname, const GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glImageTransformParameterfvHP( target, pname, params );
}

static void WINAPI glImageTransformParameteriHP( GLenum target, GLenum pname, GLint param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", target, pname, param );
  funcs->ext.p_glImageTransformParameteriHP( target, pname, param );
}

static void WINAPI glImageTransformParameterivHP( GLenum target, GLenum pname, const GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glImageTransformParameterivHP( target, pname, params );
}

static void WINAPI glImportMemoryFdEXT( GLuint memory, GLuint64 size, GLenum handleType, GLint fd )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %s, %d, %d)\n", memory, wine_dbgstr_longlong(size), handleType, fd );
  funcs->ext.p_glImportMemoryFdEXT( memory, size, handleType, fd );
}

static void WINAPI glImportMemoryWin32HandleEXT( GLuint memory, GLuint64 size, GLenum handleType, void *handle )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %s, %d, %p)\n", memory, wine_dbgstr_longlong(size), handleType, handle );
  funcs->ext.p_glImportMemoryWin32HandleEXT( memory, size, handleType, handle );
}

static void WINAPI glImportMemoryWin32NameEXT( GLuint memory, GLuint64 size, GLenum handleType, const void *name )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %s, %d, %p)\n", memory, wine_dbgstr_longlong(size), handleType, name );
  funcs->ext.p_glImportMemoryWin32NameEXT( memory, size, handleType, name );
}

static void WINAPI glImportSemaphoreFdEXT( GLuint semaphore, GLenum handleType, GLint fd )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", semaphore, handleType, fd );
  funcs->ext.p_glImportSemaphoreFdEXT( semaphore, handleType, fd );
}

static void WINAPI glImportSemaphoreWin32HandleEXT( GLuint semaphore, GLenum handleType, void *handle )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", semaphore, handleType, handle );
  funcs->ext.p_glImportSemaphoreWin32HandleEXT( semaphore, handleType, handle );
}

static void WINAPI glImportSemaphoreWin32NameEXT( GLuint semaphore, GLenum handleType, const void *name )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", semaphore, handleType, name );
  funcs->ext.p_glImportSemaphoreWin32NameEXT( semaphore, handleType, name );
}

static GLsync WINAPI glImportSyncEXT( GLenum external_sync_type, GLintptr external_sync, GLbitfield flags )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %ld, %d)\n", external_sync_type, external_sync, flags );
  return funcs->ext.p_glImportSyncEXT( external_sync_type, external_sync, flags );
}

static void WINAPI glIndexFormatNV( GLenum type, GLsizei stride )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", type, stride );
  funcs->ext.p_glIndexFormatNV( type, stride );
}

static void WINAPI glIndexFuncEXT( GLenum func, GLclampf ref )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f)\n", func, ref );
  funcs->ext.p_glIndexFuncEXT( func, ref );
}

static void WINAPI glIndexMaterialEXT( GLenum face, GLenum mode )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", face, mode );
  funcs->ext.p_glIndexMaterialEXT( face, mode );
}

static void WINAPI glIndexPointerEXT( GLenum type, GLsizei stride, GLsizei count, const void *pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", type, stride, count, pointer );
  funcs->ext.p_glIndexPointerEXT( type, stride, count, pointer );
}

static void WINAPI glIndexPointerListIBM( GLenum type, GLint stride, const void **pointer, GLint ptrstride )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %d)\n", type, stride, pointer, ptrstride );
  funcs->ext.p_glIndexPointerListIBM( type, stride, pointer, ptrstride );
}

static void WINAPI glIndexxOES( GLfixed component )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", component );
  funcs->ext.p_glIndexxOES( component );
}

static void WINAPI glIndexxvOES( const GLfixed *component )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", component );
  funcs->ext.p_glIndexxvOES( component );
}

static void WINAPI glInsertComponentEXT( GLuint res, GLuint src, GLuint num )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", res, src, num );
  funcs->ext.p_glInsertComponentEXT( res, src, num );
}

static void WINAPI glInsertEventMarkerEXT( GLsizei length, const GLchar *marker )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", length, marker );
  funcs->ext.p_glInsertEventMarkerEXT( length, marker );
}

static void WINAPI glInstrumentsBufferSGIX( GLsizei size, GLint *buffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", size, buffer );
  funcs->ext.p_glInstrumentsBufferSGIX( size, buffer );
}

static void WINAPI glInterpolatePathsNV( GLuint resultPath, GLuint pathA, GLuint pathB, GLfloat weight )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %f)\n", resultPath, pathA, pathB, weight );
  funcs->ext.p_glInterpolatePathsNV( resultPath, pathA, pathB, weight );
}

static void WINAPI glInvalidateBufferData( GLuint buffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", buffer );
  funcs->ext.p_glInvalidateBufferData( buffer );
}

static void WINAPI glInvalidateBufferSubData( GLuint buffer, GLintptr offset, GLsizeiptr length )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %ld, %ld)\n", buffer, offset, length );
  funcs->ext.p_glInvalidateBufferSubData( buffer, offset, length );
}

static void WINAPI glInvalidateFramebuffer( GLenum target, GLsizei numAttachments, const GLenum *attachments )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, numAttachments, attachments );
  funcs->ext.p_glInvalidateFramebuffer( target, numAttachments, attachments );
}

static void WINAPI glInvalidateNamedFramebufferData( GLuint framebuffer, GLsizei numAttachments, const GLenum *attachments )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", framebuffer, numAttachments, attachments );
  funcs->ext.p_glInvalidateNamedFramebufferData( framebuffer, numAttachments, attachments );
}

static void WINAPI glInvalidateNamedFramebufferSubData( GLuint framebuffer, GLsizei numAttachments, const GLenum *attachments, GLint x, GLint y, GLsizei width, GLsizei height )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %d, %d, %d, %d)\n", framebuffer, numAttachments, attachments, x, y, width, height );
  funcs->ext.p_glInvalidateNamedFramebufferSubData( framebuffer, numAttachments, attachments, x, y, width, height );
}

static void WINAPI glInvalidateSubFramebuffer( GLenum target, GLsizei numAttachments, const GLenum *attachments, GLint x, GLint y, GLsizei width, GLsizei height )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %d, %d, %d, %d)\n", target, numAttachments, attachments, x, y, width, height );
  funcs->ext.p_glInvalidateSubFramebuffer( target, numAttachments, attachments, x, y, width, height );
}

static void WINAPI glInvalidateTexImage( GLuint texture, GLint level )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", texture, level );
  funcs->ext.p_glInvalidateTexImage( texture, level );
}

static void WINAPI glInvalidateTexSubImage( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d)\n", texture, level, xoffset, yoffset, zoffset, width, height, depth );
  funcs->ext.p_glInvalidateTexSubImage( texture, level, xoffset, yoffset, zoffset, width, height, depth );
}

static GLboolean WINAPI glIsAsyncMarkerSGIX( GLuint marker )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", marker );
  return funcs->ext.p_glIsAsyncMarkerSGIX( marker );
}

static GLboolean WINAPI glIsBuffer( GLuint buffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", buffer );
  return funcs->ext.p_glIsBuffer( buffer );
}

static GLboolean WINAPI glIsBufferARB( GLuint buffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", buffer );
  return funcs->ext.p_glIsBufferARB( buffer );
}

static GLboolean WINAPI glIsBufferResidentNV( GLenum target )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", target );
  return funcs->ext.p_glIsBufferResidentNV( target );
}

static GLboolean WINAPI glIsCommandListNV( GLuint list )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", list );
  return funcs->ext.p_glIsCommandListNV( list );
}

static GLboolean WINAPI glIsEnabledIndexedEXT( GLenum target, GLuint index )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", target, index );
  return funcs->ext.p_glIsEnabledIndexedEXT( target, index );
}

static GLboolean WINAPI glIsEnabledi( GLenum target, GLuint index )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", target, index );
  return funcs->ext.p_glIsEnabledi( target, index );
}

static GLboolean WINAPI glIsFenceAPPLE( GLuint fence )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", fence );
  return funcs->ext.p_glIsFenceAPPLE( fence );
}

static GLboolean WINAPI glIsFenceNV( GLuint fence )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", fence );
  return funcs->ext.p_glIsFenceNV( fence );
}

static GLboolean WINAPI glIsFramebuffer( GLuint framebuffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", framebuffer );
  return funcs->ext.p_glIsFramebuffer( framebuffer );
}

static GLboolean WINAPI glIsFramebufferEXT( GLuint framebuffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", framebuffer );
  return funcs->ext.p_glIsFramebufferEXT( framebuffer );
}

static GLboolean WINAPI glIsImageHandleResidentARB( GLuint64 handle )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%s)\n", wine_dbgstr_longlong(handle) );
  return funcs->ext.p_glIsImageHandleResidentARB( handle );
}

static GLboolean WINAPI glIsImageHandleResidentNV( GLuint64 handle )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%s)\n", wine_dbgstr_longlong(handle) );
  return funcs->ext.p_glIsImageHandleResidentNV( handle );
}

static GLboolean WINAPI glIsMemoryObjectEXT( GLuint memoryObject )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", memoryObject );
  return funcs->ext.p_glIsMemoryObjectEXT( memoryObject );
}

static GLboolean WINAPI glIsNameAMD( GLenum identifier, GLuint name )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", identifier, name );
  return funcs->ext.p_glIsNameAMD( identifier, name );
}

static GLboolean WINAPI glIsNamedBufferResidentNV( GLuint buffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", buffer );
  return funcs->ext.p_glIsNamedBufferResidentNV( buffer );
}

static GLboolean WINAPI glIsNamedStringARB( GLint namelen, const GLchar *name )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", namelen, name );
  return funcs->ext.p_glIsNamedStringARB( namelen, name );
}

static GLboolean WINAPI glIsObjectBufferATI( GLuint buffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", buffer );
  return funcs->ext.p_glIsObjectBufferATI( buffer );
}

static GLboolean WINAPI glIsOcclusionQueryNV( GLuint id )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", id );
  return funcs->ext.p_glIsOcclusionQueryNV( id );
}

static GLboolean WINAPI glIsPathNV( GLuint path )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", path );
  return funcs->ext.p_glIsPathNV( path );
}

static GLboolean WINAPI glIsPointInFillPathNV( GLuint path, GLuint mask, GLfloat x, GLfloat y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %f, %f)\n", path, mask, x, y );
  return funcs->ext.p_glIsPointInFillPathNV( path, mask, x, y );
}

static GLboolean WINAPI glIsPointInStrokePathNV( GLuint path, GLfloat x, GLfloat y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f)\n", path, x, y );
  return funcs->ext.p_glIsPointInStrokePathNV( path, x, y );
}

static GLboolean WINAPI glIsProgram( GLuint program )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", program );
  return funcs->ext.p_glIsProgram( program );
}

static GLboolean WINAPI glIsProgramARB( GLuint program )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", program );
  return funcs->ext.p_glIsProgramARB( program );
}

static GLboolean WINAPI glIsProgramNV( GLuint id )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", id );
  return funcs->ext.p_glIsProgramNV( id );
}

static GLboolean WINAPI glIsProgramPipeline( GLuint pipeline )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", pipeline );
  return funcs->ext.p_glIsProgramPipeline( pipeline );
}

static GLboolean WINAPI glIsQuery( GLuint id )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", id );
  return funcs->ext.p_glIsQuery( id );
}

static GLboolean WINAPI glIsQueryARB( GLuint id )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", id );
  return funcs->ext.p_glIsQueryARB( id );
}

static GLboolean WINAPI glIsRenderbuffer( GLuint renderbuffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", renderbuffer );
  return funcs->ext.p_glIsRenderbuffer( renderbuffer );
}

static GLboolean WINAPI glIsRenderbufferEXT( GLuint renderbuffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", renderbuffer );
  return funcs->ext.p_glIsRenderbufferEXT( renderbuffer );
}

static GLboolean WINAPI glIsSampler( GLuint sampler )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", sampler );
  return funcs->ext.p_glIsSampler( sampler );
}

static GLboolean WINAPI glIsSemaphoreEXT( GLuint semaphore )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", semaphore );
  return funcs->ext.p_glIsSemaphoreEXT( semaphore );
}

static GLboolean WINAPI glIsShader( GLuint shader )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", shader );
  return funcs->ext.p_glIsShader( shader );
}

static GLboolean WINAPI glIsStateNV( GLuint state )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", state );
  return funcs->ext.p_glIsStateNV( state );
}

static GLboolean WINAPI glIsSync( GLsync sync )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", sync );
  return funcs->ext.p_glIsSync( sync );
}

static GLboolean WINAPI glIsTextureEXT( GLuint texture )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", texture );
  return funcs->ext.p_glIsTextureEXT( texture );
}

static GLboolean WINAPI glIsTextureHandleResidentARB( GLuint64 handle )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%s)\n", wine_dbgstr_longlong(handle) );
  return funcs->ext.p_glIsTextureHandleResidentARB( handle );
}

static GLboolean WINAPI glIsTextureHandleResidentNV( GLuint64 handle )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%s)\n", wine_dbgstr_longlong(handle) );
  return funcs->ext.p_glIsTextureHandleResidentNV( handle );
}

static GLboolean WINAPI glIsTransformFeedback( GLuint id )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", id );
  return funcs->ext.p_glIsTransformFeedback( id );
}

static GLboolean WINAPI glIsTransformFeedbackNV( GLuint id )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", id );
  return funcs->ext.p_glIsTransformFeedbackNV( id );
}

static GLboolean WINAPI glIsVariantEnabledEXT( GLuint id, GLenum cap )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", id, cap );
  return funcs->ext.p_glIsVariantEnabledEXT( id, cap );
}

static GLboolean WINAPI glIsVertexArray( GLuint array )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", array );
  return funcs->ext.p_glIsVertexArray( array );
}

static GLboolean WINAPI glIsVertexArrayAPPLE( GLuint array )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", array );
  return funcs->ext.p_glIsVertexArrayAPPLE( array );
}

static GLboolean WINAPI glIsVertexAttribEnabledAPPLE( GLuint index, GLenum pname )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", index, pname );
  return funcs->ext.p_glIsVertexAttribEnabledAPPLE( index, pname );
}

static void WINAPI glLGPUCopyImageSubDataNVX( GLuint sourceGpu, GLbitfield destinationGpuMask, GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srxY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei width, GLsizei height, GLsizei depth )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", sourceGpu, destinationGpuMask, srcName, srcTarget, srcLevel, srcX, srxY, srcZ, dstName, dstTarget, dstLevel, dstX, dstY, dstZ, width, height, depth );
  funcs->ext.p_glLGPUCopyImageSubDataNVX( sourceGpu, destinationGpuMask, srcName, srcTarget, srcLevel, srcX, srxY, srcZ, dstName, dstTarget, dstLevel, dstX, dstY, dstZ, width, height, depth );
}

static void WINAPI glLGPUInterlockNVX(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  funcs->ext.p_glLGPUInterlockNVX();
}

static void WINAPI glLGPUNamedBufferSubDataNVX( GLbitfield gpuMask, GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %ld, %ld, %p)\n", gpuMask, buffer, offset, size, data );
  funcs->ext.p_glLGPUNamedBufferSubDataNVX( gpuMask, buffer, offset, size, data );
}

static void WINAPI glLabelObjectEXT( GLenum type, GLuint object, GLsizei length, const GLchar *label )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", type, object, length, label );
  funcs->ext.p_glLabelObjectEXT( type, object, length, label );
}

static void WINAPI glLightEnviSGIX( GLenum pname, GLint param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", pname, param );
  funcs->ext.p_glLightEnviSGIX( pname, param );
}

static void WINAPI glLightModelxOES( GLenum pname, GLfixed param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", pname, param );
  funcs->ext.p_glLightModelxOES( pname, param );
}

static void WINAPI glLightModelxvOES( GLenum pname, const GLfixed *param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", pname, param );
  funcs->ext.p_glLightModelxvOES( pname, param );
}

static void WINAPI glLightxOES( GLenum light, GLenum pname, GLfixed param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", light, pname, param );
  funcs->ext.p_glLightxOES( light, pname, param );
}

static void WINAPI glLightxvOES( GLenum light, GLenum pname, const GLfixed *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", light, pname, params );
  funcs->ext.p_glLightxvOES( light, pname, params );
}

static void WINAPI glLineWidthxOES( GLfixed width )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", width );
  funcs->ext.p_glLineWidthxOES( width );
}

static void WINAPI glLinkProgram( GLuint program )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", program );
  funcs->ext.p_glLinkProgram( program );
}

static void WINAPI glLinkProgramARB( GLhandleARB programObj )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", programObj );
  funcs->ext.p_glLinkProgramARB( programObj );
}

static void WINAPI glListDrawCommandsStatesClientNV( GLuint list, GLuint segment, const void **indirects, const GLsizei *sizes, const GLuint *states, const GLuint *fbos, GLuint count )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %p, %p, %p, %d)\n", list, segment, indirects, sizes, states, fbos, count );
  funcs->ext.p_glListDrawCommandsStatesClientNV( list, segment, indirects, sizes, states, fbos, count );
}

static void WINAPI glListParameterfSGIX( GLuint list, GLenum pname, GLfloat param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %f)\n", list, pname, param );
  funcs->ext.p_glListParameterfSGIX( list, pname, param );
}

static void WINAPI glListParameterfvSGIX( GLuint list, GLenum pname, const GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", list, pname, params );
  funcs->ext.p_glListParameterfvSGIX( list, pname, params );
}

static void WINAPI glListParameteriSGIX( GLuint list, GLenum pname, GLint param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", list, pname, param );
  funcs->ext.p_glListParameteriSGIX( list, pname, param );
}

static void WINAPI glListParameterivSGIX( GLuint list, GLenum pname, const GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", list, pname, params );
  funcs->ext.p_glListParameterivSGIX( list, pname, params );
}

static void WINAPI glLoadIdentityDeformationMapSGIX( GLbitfield mask )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", mask );
  funcs->ext.p_glLoadIdentityDeformationMapSGIX( mask );
}

static void WINAPI glLoadMatrixxOES( const GLfixed *m )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", m );
  funcs->ext.p_glLoadMatrixxOES( m );
}

static void WINAPI glLoadProgramNV( GLenum target, GLuint id, GLsizei len, const GLubyte *program )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", target, id, len, program );
  funcs->ext.p_glLoadProgramNV( target, id, len, program );
}

static void WINAPI glLoadTransposeMatrixd( const GLdouble *m )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", m );
  funcs->ext.p_glLoadTransposeMatrixd( m );
}

static void WINAPI glLoadTransposeMatrixdARB( const GLdouble *m )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", m );
  funcs->ext.p_glLoadTransposeMatrixdARB( m );
}

static void WINAPI glLoadTransposeMatrixf( const GLfloat *m )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", m );
  funcs->ext.p_glLoadTransposeMatrixf( m );
}

static void WINAPI glLoadTransposeMatrixfARB( const GLfloat *m )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", m );
  funcs->ext.p_glLoadTransposeMatrixfARB( m );
}

static void WINAPI glLoadTransposeMatrixxOES( const GLfixed *m )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", m );
  funcs->ext.p_glLoadTransposeMatrixxOES( m );
}

static void WINAPI glLockArraysEXT( GLint first, GLsizei count )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", first, count );
  funcs->ext.p_glLockArraysEXT( first, count );
}

static void WINAPI glMTexCoord2fSGIS( GLenum target, GLfloat s, GLfloat t )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f)\n", target, s, t );
  funcs->ext.p_glMTexCoord2fSGIS( target, s, t );
}

static void WINAPI glMTexCoord2fvSGIS( GLenum target, GLfloat * v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMTexCoord2fvSGIS( target, v );
}

static void WINAPI glMakeBufferNonResidentNV( GLenum target )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", target );
  funcs->ext.p_glMakeBufferNonResidentNV( target );
}

static void WINAPI glMakeBufferResidentNV( GLenum target, GLenum access )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", target, access );
  funcs->ext.p_glMakeBufferResidentNV( target, access );
}

static void WINAPI glMakeImageHandleNonResidentARB( GLuint64 handle )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%s)\n", wine_dbgstr_longlong(handle) );
  funcs->ext.p_glMakeImageHandleNonResidentARB( handle );
}

static void WINAPI glMakeImageHandleNonResidentNV( GLuint64 handle )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%s)\n", wine_dbgstr_longlong(handle) );
  funcs->ext.p_glMakeImageHandleNonResidentNV( handle );
}

static void WINAPI glMakeImageHandleResidentARB( GLuint64 handle, GLenum access )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%s, %d)\n", wine_dbgstr_longlong(handle), access );
  funcs->ext.p_glMakeImageHandleResidentARB( handle, access );
}

static void WINAPI glMakeImageHandleResidentNV( GLuint64 handle, GLenum access )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%s, %d)\n", wine_dbgstr_longlong(handle), access );
  funcs->ext.p_glMakeImageHandleResidentNV( handle, access );
}

static void WINAPI glMakeNamedBufferNonResidentNV( GLuint buffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", buffer );
  funcs->ext.p_glMakeNamedBufferNonResidentNV( buffer );
}

static void WINAPI glMakeNamedBufferResidentNV( GLuint buffer, GLenum access )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", buffer, access );
  funcs->ext.p_glMakeNamedBufferResidentNV( buffer, access );
}

static void WINAPI glMakeTextureHandleNonResidentARB( GLuint64 handle )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%s)\n", wine_dbgstr_longlong(handle) );
  funcs->ext.p_glMakeTextureHandleNonResidentARB( handle );
}

static void WINAPI glMakeTextureHandleNonResidentNV( GLuint64 handle )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%s)\n", wine_dbgstr_longlong(handle) );
  funcs->ext.p_glMakeTextureHandleNonResidentNV( handle );
}

static void WINAPI glMakeTextureHandleResidentARB( GLuint64 handle )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%s)\n", wine_dbgstr_longlong(handle) );
  funcs->ext.p_glMakeTextureHandleResidentARB( handle );
}

static void WINAPI glMakeTextureHandleResidentNV( GLuint64 handle )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%s)\n", wine_dbgstr_longlong(handle) );
  funcs->ext.p_glMakeTextureHandleResidentNV( handle );
}

static void WINAPI glMap1xOES( GLenum target, GLfixed u1, GLfixed u2, GLint stride, GLint order, GLfixed points )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d)\n", target, u1, u2, stride, order, points );
  funcs->ext.p_glMap1xOES( target, u1, u2, stride, order, points );
}

static void WINAPI glMap2xOES( GLenum target, GLfixed u1, GLfixed u2, GLint ustride, GLint uorder, GLfixed v1, GLfixed v2, GLint vstride, GLint vorder, GLfixed points )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points );
  funcs->ext.p_glMap2xOES( target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points );
}

static void * WINAPI glMapBuffer( GLenum target, GLenum access )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", target, access );
  return funcs->ext.p_glMapBuffer( target, access );
}

static void * WINAPI glMapBufferARB( GLenum target, GLenum access )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", target, access );
  return funcs->ext.p_glMapBufferARB( target, access );
}

static void * WINAPI glMapBufferRange( GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %ld, %ld, %d)\n", target, offset, length, access );
  return funcs->ext.p_glMapBufferRange( target, offset, length, access );
}

static void WINAPI glMapControlPointsNV( GLenum target, GLuint index, GLenum type, GLsizei ustride, GLsizei vstride, GLint uorder, GLint vorder, GLboolean packed, const void *points )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, index, type, ustride, vstride, uorder, vorder, packed, points );
  funcs->ext.p_glMapControlPointsNV( target, index, type, ustride, vstride, uorder, vorder, packed, points );
}

static void WINAPI glMapGrid1xOES( GLint n, GLfixed u1, GLfixed u2 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", n, u1, u2 );
  funcs->ext.p_glMapGrid1xOES( n, u1, u2 );
}

static void WINAPI glMapGrid2xOES( GLint n, GLfixed u1, GLfixed u2, GLfixed v1, GLfixed v2 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", n, u1, u2, v1, v2 );
  funcs->ext.p_glMapGrid2xOES( n, u1, u2, v1, v2 );
}

static void * WINAPI glMapNamedBuffer( GLuint buffer, GLenum access )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", buffer, access );
  return funcs->ext.p_glMapNamedBuffer( buffer, access );
}

static void * WINAPI glMapNamedBufferEXT( GLuint buffer, GLenum access )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", buffer, access );
  return funcs->ext.p_glMapNamedBufferEXT( buffer, access );
}

static void * WINAPI glMapNamedBufferRange( GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %ld, %ld, %d)\n", buffer, offset, length, access );
  return funcs->ext.p_glMapNamedBufferRange( buffer, offset, length, access );
}

static void * WINAPI glMapNamedBufferRangeEXT( GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %ld, %ld, %d)\n", buffer, offset, length, access );
  return funcs->ext.p_glMapNamedBufferRangeEXT( buffer, offset, length, access );
}

static void * WINAPI glMapObjectBufferATI( GLuint buffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", buffer );
  return funcs->ext.p_glMapObjectBufferATI( buffer );
}

static void WINAPI glMapParameterfvNV( GLenum target, GLenum pname, const GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glMapParameterfvNV( target, pname, params );
}

static void WINAPI glMapParameterivNV( GLenum target, GLenum pname, const GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glMapParameterivNV( target, pname, params );
}

static void * WINAPI glMapTexture2DINTEL( GLuint texture, GLint level, GLbitfield access, GLint *stride, GLenum *layout )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p, %p)\n", texture, level, access, stride, layout );
  return funcs->ext.p_glMapTexture2DINTEL( texture, level, access, stride, layout );
}

static void WINAPI glMapVertexAttrib1dAPPLE( GLuint index, GLuint size, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %f, %f, %d, %d, %p)\n", index, size, u1, u2, stride, order, points );
  funcs->ext.p_glMapVertexAttrib1dAPPLE( index, size, u1, u2, stride, order, points );
}

static void WINAPI glMapVertexAttrib1fAPPLE( GLuint index, GLuint size, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %f, %f, %d, %d, %p)\n", index, size, u1, u2, stride, order, points );
  funcs->ext.p_glMapVertexAttrib1fAPPLE( index, size, u1, u2, stride, order, points );
}

static void WINAPI glMapVertexAttrib2dAPPLE( GLuint index, GLuint size, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %f, %f, %d, %d, %f, %f, %d, %d, %p)\n", index, size, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points );
  funcs->ext.p_glMapVertexAttrib2dAPPLE( index, size, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points );
}

static void WINAPI glMapVertexAttrib2fAPPLE( GLuint index, GLuint size, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %f, %f, %d, %d, %f, %f, %d, %d, %p)\n", index, size, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points );
  funcs->ext.p_glMapVertexAttrib2fAPPLE( index, size, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points );
}

static void WINAPI glMaterialxOES( GLenum face, GLenum pname, GLfixed param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", face, pname, param );
  funcs->ext.p_glMaterialxOES( face, pname, param );
}

static void WINAPI glMaterialxvOES( GLenum face, GLenum pname, const GLfixed *param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", face, pname, param );
  funcs->ext.p_glMaterialxvOES( face, pname, param );
}

static void WINAPI glMatrixFrustumEXT( GLenum mode, GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f, %f, %f, %f)\n", mode, left, right, bottom, top, zNear, zFar );
  funcs->ext.p_glMatrixFrustumEXT( mode, left, right, bottom, top, zNear, zFar );
}

static void WINAPI glMatrixIndexPointerARB( GLint size, GLenum type, GLsizei stride, const void *pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", size, type, stride, pointer );
  funcs->ext.p_glMatrixIndexPointerARB( size, type, stride, pointer );
}

static void WINAPI glMatrixIndexubvARB( GLint size, const GLubyte *indices )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", size, indices );
  funcs->ext.p_glMatrixIndexubvARB( size, indices );
}

static void WINAPI glMatrixIndexuivARB( GLint size, const GLuint *indices )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", size, indices );
  funcs->ext.p_glMatrixIndexuivARB( size, indices );
}

static void WINAPI glMatrixIndexusvARB( GLint size, const GLushort *indices )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", size, indices );
  funcs->ext.p_glMatrixIndexusvARB( size, indices );
}

static void WINAPI glMatrixLoad3x2fNV( GLenum matrixMode, const GLfloat *m )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", matrixMode, m );
  funcs->ext.p_glMatrixLoad3x2fNV( matrixMode, m );
}

static void WINAPI glMatrixLoad3x3fNV( GLenum matrixMode, const GLfloat *m )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", matrixMode, m );
  funcs->ext.p_glMatrixLoad3x3fNV( matrixMode, m );
}

static void WINAPI glMatrixLoadIdentityEXT( GLenum mode )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", mode );
  funcs->ext.p_glMatrixLoadIdentityEXT( mode );
}

static void WINAPI glMatrixLoadTranspose3x3fNV( GLenum matrixMode, const GLfloat *m )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", matrixMode, m );
  funcs->ext.p_glMatrixLoadTranspose3x3fNV( matrixMode, m );
}

static void WINAPI glMatrixLoadTransposedEXT( GLenum mode, const GLdouble *m )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", mode, m );
  funcs->ext.p_glMatrixLoadTransposedEXT( mode, m );
}

static void WINAPI glMatrixLoadTransposefEXT( GLenum mode, const GLfloat *m )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", mode, m );
  funcs->ext.p_glMatrixLoadTransposefEXT( mode, m );
}

static void WINAPI glMatrixLoaddEXT( GLenum mode, const GLdouble *m )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", mode, m );
  funcs->ext.p_glMatrixLoaddEXT( mode, m );
}

static void WINAPI glMatrixLoadfEXT( GLenum mode, const GLfloat *m )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", mode, m );
  funcs->ext.p_glMatrixLoadfEXT( mode, m );
}

static void WINAPI glMatrixMult3x2fNV( GLenum matrixMode, const GLfloat *m )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", matrixMode, m );
  funcs->ext.p_glMatrixMult3x2fNV( matrixMode, m );
}

static void WINAPI glMatrixMult3x3fNV( GLenum matrixMode, const GLfloat *m )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", matrixMode, m );
  funcs->ext.p_glMatrixMult3x3fNV( matrixMode, m );
}

static void WINAPI glMatrixMultTranspose3x3fNV( GLenum matrixMode, const GLfloat *m )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", matrixMode, m );
  funcs->ext.p_glMatrixMultTranspose3x3fNV( matrixMode, m );
}

static void WINAPI glMatrixMultTransposedEXT( GLenum mode, const GLdouble *m )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", mode, m );
  funcs->ext.p_glMatrixMultTransposedEXT( mode, m );
}

static void WINAPI glMatrixMultTransposefEXT( GLenum mode, const GLfloat *m )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", mode, m );
  funcs->ext.p_glMatrixMultTransposefEXT( mode, m );
}

static void WINAPI glMatrixMultdEXT( GLenum mode, const GLdouble *m )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", mode, m );
  funcs->ext.p_glMatrixMultdEXT( mode, m );
}

static void WINAPI glMatrixMultfEXT( GLenum mode, const GLfloat *m )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", mode, m );
  funcs->ext.p_glMatrixMultfEXT( mode, m );
}

static void WINAPI glMatrixOrthoEXT( GLenum mode, GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f, %f, %f, %f)\n", mode, left, right, bottom, top, zNear, zFar );
  funcs->ext.p_glMatrixOrthoEXT( mode, left, right, bottom, top, zNear, zFar );
}

static void WINAPI glMatrixPopEXT( GLenum mode )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", mode );
  funcs->ext.p_glMatrixPopEXT( mode );
}

static void WINAPI glMatrixPushEXT( GLenum mode )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", mode );
  funcs->ext.p_glMatrixPushEXT( mode );
}

static void WINAPI glMatrixRotatedEXT( GLenum mode, GLdouble angle, GLdouble x, GLdouble y, GLdouble z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f, %f)\n", mode, angle, x, y, z );
  funcs->ext.p_glMatrixRotatedEXT( mode, angle, x, y, z );
}

static void WINAPI glMatrixRotatefEXT( GLenum mode, GLfloat angle, GLfloat x, GLfloat y, GLfloat z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f, %f)\n", mode, angle, x, y, z );
  funcs->ext.p_glMatrixRotatefEXT( mode, angle, x, y, z );
}

static void WINAPI glMatrixScaledEXT( GLenum mode, GLdouble x, GLdouble y, GLdouble z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f)\n", mode, x, y, z );
  funcs->ext.p_glMatrixScaledEXT( mode, x, y, z );
}

static void WINAPI glMatrixScalefEXT( GLenum mode, GLfloat x, GLfloat y, GLfloat z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f)\n", mode, x, y, z );
  funcs->ext.p_glMatrixScalefEXT( mode, x, y, z );
}

static void WINAPI glMatrixTranslatedEXT( GLenum mode, GLdouble x, GLdouble y, GLdouble z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f)\n", mode, x, y, z );
  funcs->ext.p_glMatrixTranslatedEXT( mode, x, y, z );
}

static void WINAPI glMatrixTranslatefEXT( GLenum mode, GLfloat x, GLfloat y, GLfloat z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f)\n", mode, x, y, z );
  funcs->ext.p_glMatrixTranslatefEXT( mode, x, y, z );
}

static void WINAPI glMaxShaderCompilerThreadsARB( GLuint count )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", count );
  funcs->ext.p_glMaxShaderCompilerThreadsARB( count );
}

static void WINAPI glMaxShaderCompilerThreadsKHR( GLuint count )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", count );
  funcs->ext.p_glMaxShaderCompilerThreadsKHR( count );
}

static void WINAPI glMemoryBarrier( GLbitfield barriers )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", barriers );
  funcs->ext.p_glMemoryBarrier( barriers );
}

static void WINAPI glMemoryBarrierByRegion( GLbitfield barriers )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", barriers );
  funcs->ext.p_glMemoryBarrierByRegion( barriers );
}

static void WINAPI glMemoryBarrierEXT( GLbitfield barriers )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", barriers );
  funcs->ext.p_glMemoryBarrierEXT( barriers );
}

static void WINAPI glMemoryObjectParameterivEXT( GLuint memoryObject, GLenum pname, const GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", memoryObject, pname, params );
  funcs->ext.p_glMemoryObjectParameterivEXT( memoryObject, pname, params );
}

static void WINAPI glMinSampleShading( GLfloat value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f)\n", value );
  funcs->ext.p_glMinSampleShading( value );
}

static void WINAPI glMinSampleShadingARB( GLfloat value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f)\n", value );
  funcs->ext.p_glMinSampleShadingARB( value );
}

static void WINAPI glMinmax( GLenum target, GLenum internalformat, GLboolean sink )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", target, internalformat, sink );
  funcs->ext.p_glMinmax( target, internalformat, sink );
}

static void WINAPI glMinmaxEXT( GLenum target, GLenum internalformat, GLboolean sink )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", target, internalformat, sink );
  funcs->ext.p_glMinmaxEXT( target, internalformat, sink );
}

static void WINAPI glMultMatrixxOES( const GLfixed *m )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", m );
  funcs->ext.p_glMultMatrixxOES( m );
}

static void WINAPI glMultTransposeMatrixd( const GLdouble *m )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", m );
  funcs->ext.p_glMultTransposeMatrixd( m );
}

static void WINAPI glMultTransposeMatrixdARB( const GLdouble *m )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", m );
  funcs->ext.p_glMultTransposeMatrixdARB( m );
}

static void WINAPI glMultTransposeMatrixf( const GLfloat *m )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", m );
  funcs->ext.p_glMultTransposeMatrixf( m );
}

static void WINAPI glMultTransposeMatrixfARB( const GLfloat *m )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", m );
  funcs->ext.p_glMultTransposeMatrixfARB( m );
}

static void WINAPI glMultTransposeMatrixxOES( const GLfixed *m )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", m );
  funcs->ext.p_glMultTransposeMatrixxOES( m );
}

static void WINAPI glMultiDrawArrays( GLenum mode, const GLint *first, const GLsizei *count, GLsizei drawcount )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p, %p, %d)\n", mode, first, count, drawcount );
  funcs->ext.p_glMultiDrawArrays( mode, first, count, drawcount );
}

static void WINAPI glMultiDrawArraysEXT( GLenum mode, const GLint *first, const GLsizei *count, GLsizei primcount )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p, %p, %d)\n", mode, first, count, primcount );
  funcs->ext.p_glMultiDrawArraysEXT( mode, first, count, primcount );
}

static void WINAPI glMultiDrawArraysIndirect( GLenum mode, const void *indirect, GLsizei drawcount, GLsizei stride )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p, %d, %d)\n", mode, indirect, drawcount, stride );
  funcs->ext.p_glMultiDrawArraysIndirect( mode, indirect, drawcount, stride );
}

static void WINAPI glMultiDrawArraysIndirectAMD( GLenum mode, const void *indirect, GLsizei primcount, GLsizei stride )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p, %d, %d)\n", mode, indirect, primcount, stride );
  funcs->ext.p_glMultiDrawArraysIndirectAMD( mode, indirect, primcount, stride );
}

static void WINAPI glMultiDrawArraysIndirectBindlessCountNV( GLenum mode, const void *indirect, GLsizei drawCount, GLsizei maxDrawCount, GLsizei stride, GLint vertexBufferCount )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p, %d, %d, %d, %d)\n", mode, indirect, drawCount, maxDrawCount, stride, vertexBufferCount );
  funcs->ext.p_glMultiDrawArraysIndirectBindlessCountNV( mode, indirect, drawCount, maxDrawCount, stride, vertexBufferCount );
}

static void WINAPI glMultiDrawArraysIndirectBindlessNV( GLenum mode, const void *indirect, GLsizei drawCount, GLsizei stride, GLint vertexBufferCount )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p, %d, %d, %d)\n", mode, indirect, drawCount, stride, vertexBufferCount );
  funcs->ext.p_glMultiDrawArraysIndirectBindlessNV( mode, indirect, drawCount, stride, vertexBufferCount );
}

static void WINAPI glMultiDrawArraysIndirectCount( GLenum mode, const void *indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p, %ld, %d, %d)\n", mode, indirect, drawcount, maxdrawcount, stride );
  funcs->ext.p_glMultiDrawArraysIndirectCount( mode, indirect, drawcount, maxdrawcount, stride );
}

static void WINAPI glMultiDrawArraysIndirectCountARB( GLenum mode, const void *indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p, %ld, %d, %d)\n", mode, indirect, drawcount, maxdrawcount, stride );
  funcs->ext.p_glMultiDrawArraysIndirectCountARB( mode, indirect, drawcount, maxdrawcount, stride );
}

static void WINAPI glMultiDrawElementArrayAPPLE( GLenum mode, const GLint *first, const GLsizei *count, GLsizei primcount )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p, %p, %d)\n", mode, first, count, primcount );
  funcs->ext.p_glMultiDrawElementArrayAPPLE( mode, first, count, primcount );
}

static void WINAPI glMultiDrawElements( GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei drawcount )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p, %d, %p, %d)\n", mode, count, type, indices, drawcount );
  funcs->ext.p_glMultiDrawElements( mode, count, type, indices, drawcount );
}

static void WINAPI glMultiDrawElementsBaseVertex( GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei drawcount, const GLint *basevertex )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p, %d, %p, %d, %p)\n", mode, count, type, indices, drawcount, basevertex );
  funcs->ext.p_glMultiDrawElementsBaseVertex( mode, count, type, indices, drawcount, basevertex );
}

static void WINAPI glMultiDrawElementsEXT( GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei primcount )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p, %d, %p, %d)\n", mode, count, type, indices, primcount );
  funcs->ext.p_glMultiDrawElementsEXT( mode, count, type, indices, primcount );
}

static void WINAPI glMultiDrawElementsIndirect( GLenum mode, GLenum type, const void *indirect, GLsizei drawcount, GLsizei stride )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %d, %d)\n", mode, type, indirect, drawcount, stride );
  funcs->ext.p_glMultiDrawElementsIndirect( mode, type, indirect, drawcount, stride );
}

static void WINAPI glMultiDrawElementsIndirectAMD( GLenum mode, GLenum type, const void *indirect, GLsizei primcount, GLsizei stride )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %d, %d)\n", mode, type, indirect, primcount, stride );
  funcs->ext.p_glMultiDrawElementsIndirectAMD( mode, type, indirect, primcount, stride );
}

static void WINAPI glMultiDrawElementsIndirectBindlessCountNV( GLenum mode, GLenum type, const void *indirect, GLsizei drawCount, GLsizei maxDrawCount, GLsizei stride, GLint vertexBufferCount )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %d, %d, %d, %d)\n", mode, type, indirect, drawCount, maxDrawCount, stride, vertexBufferCount );
  funcs->ext.p_glMultiDrawElementsIndirectBindlessCountNV( mode, type, indirect, drawCount, maxDrawCount, stride, vertexBufferCount );
}

static void WINAPI glMultiDrawElementsIndirectBindlessNV( GLenum mode, GLenum type, const void *indirect, GLsizei drawCount, GLsizei stride, GLint vertexBufferCount )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %d, %d, %d)\n", mode, type, indirect, drawCount, stride, vertexBufferCount );
  funcs->ext.p_glMultiDrawElementsIndirectBindlessNV( mode, type, indirect, drawCount, stride, vertexBufferCount );
}

static void WINAPI glMultiDrawElementsIndirectCount( GLenum mode, GLenum type, const void *indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %ld, %d, %d)\n", mode, type, indirect, drawcount, maxdrawcount, stride );
  funcs->ext.p_glMultiDrawElementsIndirectCount( mode, type, indirect, drawcount, maxdrawcount, stride );
}

static void WINAPI glMultiDrawElementsIndirectCountARB( GLenum mode, GLenum type, const void *indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %ld, %d, %d)\n", mode, type, indirect, drawcount, maxdrawcount, stride );
  funcs->ext.p_glMultiDrawElementsIndirectCountARB( mode, type, indirect, drawcount, maxdrawcount, stride );
}

static void WINAPI glMultiDrawRangeElementArrayAPPLE( GLenum mode, GLuint start, GLuint end, const GLint *first, const GLsizei *count, GLsizei primcount )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p, %p, %d)\n", mode, start, end, first, count, primcount );
  funcs->ext.p_glMultiDrawRangeElementArrayAPPLE( mode, start, end, first, count, primcount );
}

static void WINAPI glMultiModeDrawArraysIBM( const GLenum *mode, const GLint *first, const GLsizei *count, GLsizei primcount, GLint modestride )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p, %p, %p, %d, %d)\n", mode, first, count, primcount, modestride );
  funcs->ext.p_glMultiModeDrawArraysIBM( mode, first, count, primcount, modestride );
}

static void WINAPI glMultiModeDrawElementsIBM( const GLenum *mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei primcount, GLint modestride )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p, %p, %d, %p, %d, %d)\n", mode, count, type, indices, primcount, modestride );
  funcs->ext.p_glMultiModeDrawElementsIBM( mode, count, type, indices, primcount, modestride );
}

static void WINAPI glMultiTexBufferEXT( GLenum texunit, GLenum target, GLenum internalformat, GLuint buffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", texunit, target, internalformat, buffer );
  funcs->ext.p_glMultiTexBufferEXT( texunit, target, internalformat, buffer );
}

static void WINAPI glMultiTexCoord1bOES( GLenum texture, GLbyte s )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", texture, s );
  funcs->ext.p_glMultiTexCoord1bOES( texture, s );
}

static void WINAPI glMultiTexCoord1bvOES( GLenum texture, const GLbyte *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", texture, coords );
  funcs->ext.p_glMultiTexCoord1bvOES( texture, coords );
}

static void WINAPI glMultiTexCoord1d( GLenum target, GLdouble s )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f)\n", target, s );
  funcs->ext.p_glMultiTexCoord1d( target, s );
}

static void WINAPI glMultiTexCoord1dARB( GLenum target, GLdouble s )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f)\n", target, s );
  funcs->ext.p_glMultiTexCoord1dARB( target, s );
}

static void WINAPI glMultiTexCoord1dSGIS( GLenum target, GLdouble s )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f)\n", target, s );
  funcs->ext.p_glMultiTexCoord1dSGIS( target, s );
}

static void WINAPI glMultiTexCoord1dv( GLenum target, const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord1dv( target, v );
}

static void WINAPI glMultiTexCoord1dvARB( GLenum target, const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord1dvARB( target, v );
}

static void WINAPI glMultiTexCoord1dvSGIS( GLenum target, GLdouble * v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord1dvSGIS( target, v );
}

static void WINAPI glMultiTexCoord1f( GLenum target, GLfloat s )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f)\n", target, s );
  funcs->ext.p_glMultiTexCoord1f( target, s );
}

static void WINAPI glMultiTexCoord1fARB( GLenum target, GLfloat s )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f)\n", target, s );
  funcs->ext.p_glMultiTexCoord1fARB( target, s );
}

static void WINAPI glMultiTexCoord1fSGIS( GLenum target, GLfloat s )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f)\n", target, s );
  funcs->ext.p_glMultiTexCoord1fSGIS( target, s );
}

static void WINAPI glMultiTexCoord1fv( GLenum target, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord1fv( target, v );
}

static void WINAPI glMultiTexCoord1fvARB( GLenum target, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord1fvARB( target, v );
}

static void WINAPI glMultiTexCoord1fvSGIS( GLenum target, const GLfloat * v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord1fvSGIS( target, v );
}

static void WINAPI glMultiTexCoord1hNV( GLenum target, GLhalfNV s )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", target, s );
  funcs->ext.p_glMultiTexCoord1hNV( target, s );
}

static void WINAPI glMultiTexCoord1hvNV( GLenum target, const GLhalfNV *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord1hvNV( target, v );
}

static void WINAPI glMultiTexCoord1i( GLenum target, GLint s )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", target, s );
  funcs->ext.p_glMultiTexCoord1i( target, s );
}

static void WINAPI glMultiTexCoord1iARB( GLenum target, GLint s )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", target, s );
  funcs->ext.p_glMultiTexCoord1iARB( target, s );
}

static void WINAPI glMultiTexCoord1iSGIS( GLenum target, GLint s )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", target, s );
  funcs->ext.p_glMultiTexCoord1iSGIS( target, s );
}

static void WINAPI glMultiTexCoord1iv( GLenum target, const GLint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord1iv( target, v );
}

static void WINAPI glMultiTexCoord1ivARB( GLenum target, const GLint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord1ivARB( target, v );
}

static void WINAPI glMultiTexCoord1ivSGIS( GLenum target, GLint * v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord1ivSGIS( target, v );
}

static void WINAPI glMultiTexCoord1s( GLenum target, GLshort s )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", target, s );
  funcs->ext.p_glMultiTexCoord1s( target, s );
}

static void WINAPI glMultiTexCoord1sARB( GLenum target, GLshort s )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", target, s );
  funcs->ext.p_glMultiTexCoord1sARB( target, s );
}

static void WINAPI glMultiTexCoord1sSGIS( GLenum target, GLshort s )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", target, s );
  funcs->ext.p_glMultiTexCoord1sSGIS( target, s );
}

static void WINAPI glMultiTexCoord1sv( GLenum target, const GLshort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord1sv( target, v );
}

static void WINAPI glMultiTexCoord1svARB( GLenum target, const GLshort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord1svARB( target, v );
}

static void WINAPI glMultiTexCoord1svSGIS( GLenum target, GLshort * v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord1svSGIS( target, v );
}

static void WINAPI glMultiTexCoord1xOES( GLenum texture, GLfixed s )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", texture, s );
  funcs->ext.p_glMultiTexCoord1xOES( texture, s );
}

static void WINAPI glMultiTexCoord1xvOES( GLenum texture, const GLfixed *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", texture, coords );
  funcs->ext.p_glMultiTexCoord1xvOES( texture, coords );
}

static void WINAPI glMultiTexCoord2bOES( GLenum texture, GLbyte s, GLbyte t )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", texture, s, t );
  funcs->ext.p_glMultiTexCoord2bOES( texture, s, t );
}

static void WINAPI glMultiTexCoord2bvOES( GLenum texture, const GLbyte *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", texture, coords );
  funcs->ext.p_glMultiTexCoord2bvOES( texture, coords );
}

static void WINAPI glMultiTexCoord2d( GLenum target, GLdouble s, GLdouble t )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f)\n", target, s, t );
  funcs->ext.p_glMultiTexCoord2d( target, s, t );
}

static void WINAPI glMultiTexCoord2dARB( GLenum target, GLdouble s, GLdouble t )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f)\n", target, s, t );
  funcs->ext.p_glMultiTexCoord2dARB( target, s, t );
}

static void WINAPI glMultiTexCoord2dSGIS( GLenum target, GLdouble s, GLdouble t )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f)\n", target, s, t );
  funcs->ext.p_glMultiTexCoord2dSGIS( target, s, t );
}

static void WINAPI glMultiTexCoord2dv( GLenum target, const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord2dv( target, v );
}

static void WINAPI glMultiTexCoord2dvARB( GLenum target, const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord2dvARB( target, v );
}

static void WINAPI glMultiTexCoord2dvSGIS( GLenum target, GLdouble * v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord2dvSGIS( target, v );
}

static void WINAPI glMultiTexCoord2f( GLenum target, GLfloat s, GLfloat t )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f)\n", target, s, t );
  funcs->ext.p_glMultiTexCoord2f( target, s, t );
}

static void WINAPI glMultiTexCoord2fARB( GLenum target, GLfloat s, GLfloat t )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f)\n", target, s, t );
  funcs->ext.p_glMultiTexCoord2fARB( target, s, t );
}

static void WINAPI glMultiTexCoord2fSGIS( GLenum target, GLfloat s, GLfloat t )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f)\n", target, s, t );
  funcs->ext.p_glMultiTexCoord2fSGIS( target, s, t );
}

static void WINAPI glMultiTexCoord2fv( GLenum target, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord2fv( target, v );
}

static void WINAPI glMultiTexCoord2fvARB( GLenum target, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord2fvARB( target, v );
}

static void WINAPI glMultiTexCoord2fvSGIS( GLenum target, GLfloat * v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord2fvSGIS( target, v );
}

static void WINAPI glMultiTexCoord2hNV( GLenum target, GLhalfNV s, GLhalfNV t )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", target, s, t );
  funcs->ext.p_glMultiTexCoord2hNV( target, s, t );
}

static void WINAPI glMultiTexCoord2hvNV( GLenum target, const GLhalfNV *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord2hvNV( target, v );
}

static void WINAPI glMultiTexCoord2i( GLenum target, GLint s, GLint t )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", target, s, t );
  funcs->ext.p_glMultiTexCoord2i( target, s, t );
}

static void WINAPI glMultiTexCoord2iARB( GLenum target, GLint s, GLint t )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", target, s, t );
  funcs->ext.p_glMultiTexCoord2iARB( target, s, t );
}

static void WINAPI glMultiTexCoord2iSGIS( GLenum target, GLint s, GLint t )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", target, s, t );
  funcs->ext.p_glMultiTexCoord2iSGIS( target, s, t );
}

static void WINAPI glMultiTexCoord2iv( GLenum target, const GLint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord2iv( target, v );
}

static void WINAPI glMultiTexCoord2ivARB( GLenum target, const GLint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord2ivARB( target, v );
}

static void WINAPI glMultiTexCoord2ivSGIS( GLenum target, GLint * v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord2ivSGIS( target, v );
}

static void WINAPI glMultiTexCoord2s( GLenum target, GLshort s, GLshort t )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", target, s, t );
  funcs->ext.p_glMultiTexCoord2s( target, s, t );
}

static void WINAPI glMultiTexCoord2sARB( GLenum target, GLshort s, GLshort t )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", target, s, t );
  funcs->ext.p_glMultiTexCoord2sARB( target, s, t );
}

static void WINAPI glMultiTexCoord2sSGIS( GLenum target, GLshort s, GLshort t )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", target, s, t );
  funcs->ext.p_glMultiTexCoord2sSGIS( target, s, t );
}

static void WINAPI glMultiTexCoord2sv( GLenum target, const GLshort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord2sv( target, v );
}

static void WINAPI glMultiTexCoord2svARB( GLenum target, const GLshort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord2svARB( target, v );
}

static void WINAPI glMultiTexCoord2svSGIS( GLenum target, GLshort * v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord2svSGIS( target, v );
}

static void WINAPI glMultiTexCoord2xOES( GLenum texture, GLfixed s, GLfixed t )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", texture, s, t );
  funcs->ext.p_glMultiTexCoord2xOES( texture, s, t );
}

static void WINAPI glMultiTexCoord2xvOES( GLenum texture, const GLfixed *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", texture, coords );
  funcs->ext.p_glMultiTexCoord2xvOES( texture, coords );
}

static void WINAPI glMultiTexCoord3bOES( GLenum texture, GLbyte s, GLbyte t, GLbyte r )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", texture, s, t, r );
  funcs->ext.p_glMultiTexCoord3bOES( texture, s, t, r );
}

static void WINAPI glMultiTexCoord3bvOES( GLenum texture, const GLbyte *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", texture, coords );
  funcs->ext.p_glMultiTexCoord3bvOES( texture, coords );
}

static void WINAPI glMultiTexCoord3d( GLenum target, GLdouble s, GLdouble t, GLdouble r )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f)\n", target, s, t, r );
  funcs->ext.p_glMultiTexCoord3d( target, s, t, r );
}

static void WINAPI glMultiTexCoord3dARB( GLenum target, GLdouble s, GLdouble t, GLdouble r )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f)\n", target, s, t, r );
  funcs->ext.p_glMultiTexCoord3dARB( target, s, t, r );
}

static void WINAPI glMultiTexCoord3dSGIS( GLenum target, GLdouble s, GLdouble t, GLdouble r )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f)\n", target, s, t, r );
  funcs->ext.p_glMultiTexCoord3dSGIS( target, s, t, r );
}

static void WINAPI glMultiTexCoord3dv( GLenum target, const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord3dv( target, v );
}

static void WINAPI glMultiTexCoord3dvARB( GLenum target, const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord3dvARB( target, v );
}

static void WINAPI glMultiTexCoord3dvSGIS( GLenum target, GLdouble * v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord3dvSGIS( target, v );
}

static void WINAPI glMultiTexCoord3f( GLenum target, GLfloat s, GLfloat t, GLfloat r )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f)\n", target, s, t, r );
  funcs->ext.p_glMultiTexCoord3f( target, s, t, r );
}

static void WINAPI glMultiTexCoord3fARB( GLenum target, GLfloat s, GLfloat t, GLfloat r )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f)\n", target, s, t, r );
  funcs->ext.p_glMultiTexCoord3fARB( target, s, t, r );
}

static void WINAPI glMultiTexCoord3fSGIS( GLenum target, GLfloat s, GLfloat t, GLfloat r )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f)\n", target, s, t, r );
  funcs->ext.p_glMultiTexCoord3fSGIS( target, s, t, r );
}

static void WINAPI glMultiTexCoord3fv( GLenum target, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord3fv( target, v );
}

static void WINAPI glMultiTexCoord3fvARB( GLenum target, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord3fvARB( target, v );
}

static void WINAPI glMultiTexCoord3fvSGIS( GLenum target, GLfloat * v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord3fvSGIS( target, v );
}

static void WINAPI glMultiTexCoord3hNV( GLenum target, GLhalfNV s, GLhalfNV t, GLhalfNV r )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", target, s, t, r );
  funcs->ext.p_glMultiTexCoord3hNV( target, s, t, r );
}

static void WINAPI glMultiTexCoord3hvNV( GLenum target, const GLhalfNV *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord3hvNV( target, v );
}

static void WINAPI glMultiTexCoord3i( GLenum target, GLint s, GLint t, GLint r )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", target, s, t, r );
  funcs->ext.p_glMultiTexCoord3i( target, s, t, r );
}

static void WINAPI glMultiTexCoord3iARB( GLenum target, GLint s, GLint t, GLint r )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", target, s, t, r );
  funcs->ext.p_glMultiTexCoord3iARB( target, s, t, r );
}

static void WINAPI glMultiTexCoord3iSGIS( GLenum target, GLint s, GLint t, GLint r )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", target, s, t, r );
  funcs->ext.p_glMultiTexCoord3iSGIS( target, s, t, r );
}

static void WINAPI glMultiTexCoord3iv( GLenum target, const GLint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord3iv( target, v );
}

static void WINAPI glMultiTexCoord3ivARB( GLenum target, const GLint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord3ivARB( target, v );
}

static void WINAPI glMultiTexCoord3ivSGIS( GLenum target, GLint * v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord3ivSGIS( target, v );
}

static void WINAPI glMultiTexCoord3s( GLenum target, GLshort s, GLshort t, GLshort r )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", target, s, t, r );
  funcs->ext.p_glMultiTexCoord3s( target, s, t, r );
}

static void WINAPI glMultiTexCoord3sARB( GLenum target, GLshort s, GLshort t, GLshort r )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", target, s, t, r );
  funcs->ext.p_glMultiTexCoord3sARB( target, s, t, r );
}

static void WINAPI glMultiTexCoord3sSGIS( GLenum target, GLshort s, GLshort t, GLshort r )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", target, s, t, r );
  funcs->ext.p_glMultiTexCoord3sSGIS( target, s, t, r );
}

static void WINAPI glMultiTexCoord3sv( GLenum target, const GLshort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord3sv( target, v );
}

static void WINAPI glMultiTexCoord3svARB( GLenum target, const GLshort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord3svARB( target, v );
}

static void WINAPI glMultiTexCoord3svSGIS( GLenum target, GLshort * v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord3svSGIS( target, v );
}

static void WINAPI glMultiTexCoord3xOES( GLenum texture, GLfixed s, GLfixed t, GLfixed r )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", texture, s, t, r );
  funcs->ext.p_glMultiTexCoord3xOES( texture, s, t, r );
}

static void WINAPI glMultiTexCoord3xvOES( GLenum texture, const GLfixed *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", texture, coords );
  funcs->ext.p_glMultiTexCoord3xvOES( texture, coords );
}

static void WINAPI glMultiTexCoord4bOES( GLenum texture, GLbyte s, GLbyte t, GLbyte r, GLbyte q )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", texture, s, t, r, q );
  funcs->ext.p_glMultiTexCoord4bOES( texture, s, t, r, q );
}

static void WINAPI glMultiTexCoord4bvOES( GLenum texture, const GLbyte *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", texture, coords );
  funcs->ext.p_glMultiTexCoord4bvOES( texture, coords );
}

static void WINAPI glMultiTexCoord4d( GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f, %f)\n", target, s, t, r, q );
  funcs->ext.p_glMultiTexCoord4d( target, s, t, r, q );
}

static void WINAPI glMultiTexCoord4dARB( GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f, %f)\n", target, s, t, r, q );
  funcs->ext.p_glMultiTexCoord4dARB( target, s, t, r, q );
}

static void WINAPI glMultiTexCoord4dSGIS( GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f, %f)\n", target, s, t, r, q );
  funcs->ext.p_glMultiTexCoord4dSGIS( target, s, t, r, q );
}

static void WINAPI glMultiTexCoord4dv( GLenum target, const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord4dv( target, v );
}

static void WINAPI glMultiTexCoord4dvARB( GLenum target, const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord4dvARB( target, v );
}

static void WINAPI glMultiTexCoord4dvSGIS( GLenum target, GLdouble * v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord4dvSGIS( target, v );
}

static void WINAPI glMultiTexCoord4f( GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f, %f)\n", target, s, t, r, q );
  funcs->ext.p_glMultiTexCoord4f( target, s, t, r, q );
}

static void WINAPI glMultiTexCoord4fARB( GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f, %f)\n", target, s, t, r, q );
  funcs->ext.p_glMultiTexCoord4fARB( target, s, t, r, q );
}

static void WINAPI glMultiTexCoord4fSGIS( GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f, %f)\n", target, s, t, r, q );
  funcs->ext.p_glMultiTexCoord4fSGIS( target, s, t, r, q );
}

static void WINAPI glMultiTexCoord4fv( GLenum target, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord4fv( target, v );
}

static void WINAPI glMultiTexCoord4fvARB( GLenum target, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord4fvARB( target, v );
}

static void WINAPI glMultiTexCoord4fvSGIS( GLenum target, GLfloat * v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord4fvSGIS( target, v );
}

static void WINAPI glMultiTexCoord4hNV( GLenum target, GLhalfNV s, GLhalfNV t, GLhalfNV r, GLhalfNV q )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", target, s, t, r, q );
  funcs->ext.p_glMultiTexCoord4hNV( target, s, t, r, q );
}

static void WINAPI glMultiTexCoord4hvNV( GLenum target, const GLhalfNV *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord4hvNV( target, v );
}

static void WINAPI glMultiTexCoord4i( GLenum target, GLint s, GLint t, GLint r, GLint q )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", target, s, t, r, q );
  funcs->ext.p_glMultiTexCoord4i( target, s, t, r, q );
}

static void WINAPI glMultiTexCoord4iARB( GLenum target, GLint s, GLint t, GLint r, GLint q )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", target, s, t, r, q );
  funcs->ext.p_glMultiTexCoord4iARB( target, s, t, r, q );
}

static void WINAPI glMultiTexCoord4iSGIS( GLenum target, GLint s, GLint t, GLint r, GLint q )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", target, s, t, r, q );
  funcs->ext.p_glMultiTexCoord4iSGIS( target, s, t, r, q );
}

static void WINAPI glMultiTexCoord4iv( GLenum target, const GLint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord4iv( target, v );
}

static void WINAPI glMultiTexCoord4ivARB( GLenum target, const GLint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord4ivARB( target, v );
}

static void WINAPI glMultiTexCoord4ivSGIS( GLenum target, GLint * v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord4ivSGIS( target, v );
}

static void WINAPI glMultiTexCoord4s( GLenum target, GLshort s, GLshort t, GLshort r, GLshort q )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", target, s, t, r, q );
  funcs->ext.p_glMultiTexCoord4s( target, s, t, r, q );
}

static void WINAPI glMultiTexCoord4sARB( GLenum target, GLshort s, GLshort t, GLshort r, GLshort q )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", target, s, t, r, q );
  funcs->ext.p_glMultiTexCoord4sARB( target, s, t, r, q );
}

static void WINAPI glMultiTexCoord4sSGIS( GLenum target, GLshort s, GLshort t, GLshort r, GLshort q )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", target, s, t, r, q );
  funcs->ext.p_glMultiTexCoord4sSGIS( target, s, t, r, q );
}

static void WINAPI glMultiTexCoord4sv( GLenum target, const GLshort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord4sv( target, v );
}

static void WINAPI glMultiTexCoord4svARB( GLenum target, const GLshort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord4svARB( target, v );
}

static void WINAPI glMultiTexCoord4svSGIS( GLenum target, GLshort * v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", target, v );
  funcs->ext.p_glMultiTexCoord4svSGIS( target, v );
}

static void WINAPI glMultiTexCoord4xOES( GLenum texture, GLfixed s, GLfixed t, GLfixed r, GLfixed q )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", texture, s, t, r, q );
  funcs->ext.p_glMultiTexCoord4xOES( texture, s, t, r, q );
}

static void WINAPI glMultiTexCoord4xvOES( GLenum texture, const GLfixed *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", texture, coords );
  funcs->ext.p_glMultiTexCoord4xvOES( texture, coords );
}

static void WINAPI glMultiTexCoordP1ui( GLenum texture, GLenum type, GLuint coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", texture, type, coords );
  funcs->ext.p_glMultiTexCoordP1ui( texture, type, coords );
}

static void WINAPI glMultiTexCoordP1uiv( GLenum texture, GLenum type, const GLuint *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", texture, type, coords );
  funcs->ext.p_glMultiTexCoordP1uiv( texture, type, coords );
}

static void WINAPI glMultiTexCoordP2ui( GLenum texture, GLenum type, GLuint coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", texture, type, coords );
  funcs->ext.p_glMultiTexCoordP2ui( texture, type, coords );
}

static void WINAPI glMultiTexCoordP2uiv( GLenum texture, GLenum type, const GLuint *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", texture, type, coords );
  funcs->ext.p_glMultiTexCoordP2uiv( texture, type, coords );
}

static void WINAPI glMultiTexCoordP3ui( GLenum texture, GLenum type, GLuint coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", texture, type, coords );
  funcs->ext.p_glMultiTexCoordP3ui( texture, type, coords );
}

static void WINAPI glMultiTexCoordP3uiv( GLenum texture, GLenum type, const GLuint *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", texture, type, coords );
  funcs->ext.p_glMultiTexCoordP3uiv( texture, type, coords );
}

static void WINAPI glMultiTexCoordP4ui( GLenum texture, GLenum type, GLuint coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", texture, type, coords );
  funcs->ext.p_glMultiTexCoordP4ui( texture, type, coords );
}

static void WINAPI glMultiTexCoordP4uiv( GLenum texture, GLenum type, const GLuint *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", texture, type, coords );
  funcs->ext.p_glMultiTexCoordP4uiv( texture, type, coords );
}

static void WINAPI glMultiTexCoordPointerEXT( GLenum texunit, GLint size, GLenum type, GLsizei stride, const void *pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", texunit, size, type, stride, pointer );
  funcs->ext.p_glMultiTexCoordPointerEXT( texunit, size, type, stride, pointer );
}

static void WINAPI glMultiTexCoordPointerSGIS( GLenum target, GLint size, GLenum type, GLsizei stride, GLvoid * pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", target, size, type, stride, pointer );
  funcs->ext.p_glMultiTexCoordPointerSGIS( target, size, type, stride, pointer );
}

static void WINAPI glMultiTexEnvfEXT( GLenum texunit, GLenum target, GLenum pname, GLfloat param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %f)\n", texunit, target, pname, param );
  funcs->ext.p_glMultiTexEnvfEXT( texunit, target, pname, param );
}

static void WINAPI glMultiTexEnvfvEXT( GLenum texunit, GLenum target, GLenum pname, const GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", texunit, target, pname, params );
  funcs->ext.p_glMultiTexEnvfvEXT( texunit, target, pname, params );
}

static void WINAPI glMultiTexEnviEXT( GLenum texunit, GLenum target, GLenum pname, GLint param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", texunit, target, pname, param );
  funcs->ext.p_glMultiTexEnviEXT( texunit, target, pname, param );
}

static void WINAPI glMultiTexEnvivEXT( GLenum texunit, GLenum target, GLenum pname, const GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", texunit, target, pname, params );
  funcs->ext.p_glMultiTexEnvivEXT( texunit, target, pname, params );
}

static void WINAPI glMultiTexGendEXT( GLenum texunit, GLenum coord, GLenum pname, GLdouble param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %f)\n", texunit, coord, pname, param );
  funcs->ext.p_glMultiTexGendEXT( texunit, coord, pname, param );
}

static void WINAPI glMultiTexGendvEXT( GLenum texunit, GLenum coord, GLenum pname, const GLdouble *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", texunit, coord, pname, params );
  funcs->ext.p_glMultiTexGendvEXT( texunit, coord, pname, params );
}

static void WINAPI glMultiTexGenfEXT( GLenum texunit, GLenum coord, GLenum pname, GLfloat param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %f)\n", texunit, coord, pname, param );
  funcs->ext.p_glMultiTexGenfEXT( texunit, coord, pname, param );
}

static void WINAPI glMultiTexGenfvEXT( GLenum texunit, GLenum coord, GLenum pname, const GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", texunit, coord, pname, params );
  funcs->ext.p_glMultiTexGenfvEXT( texunit, coord, pname, params );
}

static void WINAPI glMultiTexGeniEXT( GLenum texunit, GLenum coord, GLenum pname, GLint param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", texunit, coord, pname, param );
  funcs->ext.p_glMultiTexGeniEXT( texunit, coord, pname, param );
}

static void WINAPI glMultiTexGenivEXT( GLenum texunit, GLenum coord, GLenum pname, const GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", texunit, coord, pname, params );
  funcs->ext.p_glMultiTexGenivEXT( texunit, coord, pname, params );
}

static void WINAPI glMultiTexImage1DEXT( GLenum texunit, GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texunit, target, level, internalformat, width, border, format, type, pixels );
  funcs->ext.p_glMultiTexImage1DEXT( texunit, target, level, internalformat, width, border, format, type, pixels );
}

static void WINAPI glMultiTexImage2DEXT( GLenum texunit, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texunit, target, level, internalformat, width, height, border, format, type, pixels );
  funcs->ext.p_glMultiTexImage2DEXT( texunit, target, level, internalformat, width, height, border, format, type, pixels );
}

static void WINAPI glMultiTexImage3DEXT( GLenum texunit, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texunit, target, level, internalformat, width, height, depth, border, format, type, pixels );
  funcs->ext.p_glMultiTexImage3DEXT( texunit, target, level, internalformat, width, height, depth, border, format, type, pixels );
}

static void WINAPI glMultiTexParameterIivEXT( GLenum texunit, GLenum target, GLenum pname, const GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", texunit, target, pname, params );
  funcs->ext.p_glMultiTexParameterIivEXT( texunit, target, pname, params );
}

static void WINAPI glMultiTexParameterIuivEXT( GLenum texunit, GLenum target, GLenum pname, const GLuint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", texunit, target, pname, params );
  funcs->ext.p_glMultiTexParameterIuivEXT( texunit, target, pname, params );
}

static void WINAPI glMultiTexParameterfEXT( GLenum texunit, GLenum target, GLenum pname, GLfloat param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %f)\n", texunit, target, pname, param );
  funcs->ext.p_glMultiTexParameterfEXT( texunit, target, pname, param );
}

static void WINAPI glMultiTexParameterfvEXT( GLenum texunit, GLenum target, GLenum pname, const GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", texunit, target, pname, params );
  funcs->ext.p_glMultiTexParameterfvEXT( texunit, target, pname, params );
}

static void WINAPI glMultiTexParameteriEXT( GLenum texunit, GLenum target, GLenum pname, GLint param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", texunit, target, pname, param );
  funcs->ext.p_glMultiTexParameteriEXT( texunit, target, pname, param );
}

static void WINAPI glMultiTexParameterivEXT( GLenum texunit, GLenum target, GLenum pname, const GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", texunit, target, pname, params );
  funcs->ext.p_glMultiTexParameterivEXT( texunit, target, pname, params );
}

static void WINAPI glMultiTexRenderbufferEXT( GLenum texunit, GLenum target, GLuint renderbuffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", texunit, target, renderbuffer );
  funcs->ext.p_glMultiTexRenderbufferEXT( texunit, target, renderbuffer );
}

static void WINAPI glMultiTexSubImage1DEXT( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %p)\n", texunit, target, level, xoffset, width, format, type, pixels );
  funcs->ext.p_glMultiTexSubImage1DEXT( texunit, target, level, xoffset, width, format, type, pixels );
}

static void WINAPI glMultiTexSubImage2DEXT( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texunit, target, level, xoffset, yoffset, width, height, format, type, pixels );
  funcs->ext.p_glMultiTexSubImage2DEXT( texunit, target, level, xoffset, yoffset, width, height, format, type, pixels );
}

static void WINAPI glMultiTexSubImage3DEXT( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texunit, target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels );
  funcs->ext.p_glMultiTexSubImage3DEXT( texunit, target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels );
}

static void WINAPI glMulticastBarrierNV(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  funcs->ext.p_glMulticastBarrierNV();
}

static void WINAPI glMulticastBlitFramebufferNV( GLuint srcGpu, GLuint dstGpu, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", srcGpu, dstGpu, srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter );
  funcs->ext.p_glMulticastBlitFramebufferNV( srcGpu, dstGpu, srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter );
}

static void WINAPI glMulticastBufferSubDataNV( GLbitfield gpuMask, GLuint buffer, GLintptr offset, GLsizeiptr size, const GLvoid *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %ld, %ld, %p)\n", gpuMask, buffer, offset, size, data );
  funcs->ext.p_glMulticastBufferSubDataNV( gpuMask, buffer, offset, size, data );
}

static void WINAPI glMulticastCopyBufferSubDataNV( GLuint readGpu, GLbitfield writeGpuMask, GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %ld, %ld, %ld)\n", readGpu, writeGpuMask, readBuffer, writeBuffer, readOffset, writeOffset, size );
  funcs->ext.p_glMulticastCopyBufferSubDataNV( readGpu, writeGpuMask, readBuffer, writeBuffer, readOffset, writeOffset, size );
}

static void WINAPI glMulticastCopyImageSubDataNV( GLuint srcGpu, GLbitfield dstGpuMask, GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", srcGpu, dstGpuMask, srcName, srcTarget, srcLevel, srcX, srcY, srcZ, dstName, dstTarget, dstLevel, dstX, dstY, dstZ, srcWidth, srcHeight, srcDepth );
  funcs->ext.p_glMulticastCopyImageSubDataNV( srcGpu, dstGpuMask, srcName, srcTarget, srcLevel, srcX, srcY, srcZ, dstName, dstTarget, dstLevel, dstX, dstY, dstZ, srcWidth, srcHeight, srcDepth );
}

static void WINAPI glMulticastFramebufferSampleLocationsfvNV( GLuint gpu, GLuint framebuffer, GLuint start, GLsizei count, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", gpu, framebuffer, start, count, v );
  funcs->ext.p_glMulticastFramebufferSampleLocationsfvNV( gpu, framebuffer, start, count, v );
}

static void WINAPI glMulticastGetQueryObjecti64vNV( GLuint gpu, GLuint id, GLenum pname, GLint64 *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", gpu, id, pname, params );
  funcs->ext.p_glMulticastGetQueryObjecti64vNV( gpu, id, pname, params );
}

static void WINAPI glMulticastGetQueryObjectivNV( GLuint gpu, GLuint id, GLenum pname, GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", gpu, id, pname, params );
  funcs->ext.p_glMulticastGetQueryObjectivNV( gpu, id, pname, params );
}

static void WINAPI glMulticastGetQueryObjectui64vNV( GLuint gpu, GLuint id, GLenum pname, GLuint64 *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", gpu, id, pname, params );
  funcs->ext.p_glMulticastGetQueryObjectui64vNV( gpu, id, pname, params );
}

static void WINAPI glMulticastGetQueryObjectuivNV( GLuint gpu, GLuint id, GLenum pname, GLuint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", gpu, id, pname, params );
  funcs->ext.p_glMulticastGetQueryObjectuivNV( gpu, id, pname, params );
}

static void WINAPI glMulticastWaitSyncNV( GLuint signalGpu, GLbitfield waitGpuMask )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", signalGpu, waitGpuMask );
  funcs->ext.p_glMulticastWaitSyncNV( signalGpu, waitGpuMask );
}

static void WINAPI glNamedBufferData( GLuint buffer, GLsizeiptr size, const void *data, GLenum usage )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %ld, %p, %d)\n", buffer, size, data, usage );
  funcs->ext.p_glNamedBufferData( buffer, size, data, usage );
}

static void WINAPI glNamedBufferDataEXT( GLuint buffer, GLsizeiptr size, const void *data, GLenum usage )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %ld, %p, %d)\n", buffer, size, data, usage );
  funcs->ext.p_glNamedBufferDataEXT( buffer, size, data, usage );
}

static void WINAPI glNamedBufferPageCommitmentARB( GLuint buffer, GLintptr offset, GLsizeiptr size, GLboolean commit )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %ld, %ld, %d)\n", buffer, offset, size, commit );
  funcs->ext.p_glNamedBufferPageCommitmentARB( buffer, offset, size, commit );
}

static void WINAPI glNamedBufferPageCommitmentEXT( GLuint buffer, GLintptr offset, GLsizeiptr size, GLboolean commit )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %ld, %ld, %d)\n", buffer, offset, size, commit );
  funcs->ext.p_glNamedBufferPageCommitmentEXT( buffer, offset, size, commit );
}

static void WINAPI glNamedBufferStorage( GLuint buffer, GLsizeiptr size, const void *data, GLbitfield flags )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %ld, %p, %d)\n", buffer, size, data, flags );
  funcs->ext.p_glNamedBufferStorage( buffer, size, data, flags );
}

static void WINAPI glNamedBufferStorageEXT( GLuint buffer, GLsizeiptr size, const void *data, GLbitfield flags )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %ld, %p, %d)\n", buffer, size, data, flags );
  funcs->ext.p_glNamedBufferStorageEXT( buffer, size, data, flags );
}

static void WINAPI glNamedBufferStorageExternalEXT( GLuint buffer, GLintptr offset, GLsizeiptr size, GLeglClientBufferEXT clientBuffer, GLbitfield flags )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %ld, %ld, %p, %d)\n", buffer, offset, size, clientBuffer, flags );
  funcs->ext.p_glNamedBufferStorageExternalEXT( buffer, offset, size, clientBuffer, flags );
}

static void WINAPI glNamedBufferStorageMemEXT( GLuint buffer, GLsizeiptr size, GLuint memory, GLuint64 offset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %ld, %d, %s)\n", buffer, size, memory, wine_dbgstr_longlong(offset) );
  funcs->ext.p_glNamedBufferStorageMemEXT( buffer, size, memory, offset );
}

static void WINAPI glNamedBufferSubData( GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %ld, %ld, %p)\n", buffer, offset, size, data );
  funcs->ext.p_glNamedBufferSubData( buffer, offset, size, data );
}

static void WINAPI glNamedBufferSubDataEXT( GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %ld, %ld, %p)\n", buffer, offset, size, data );
  funcs->ext.p_glNamedBufferSubDataEXT( buffer, offset, size, data );
}

static void WINAPI glNamedCopyBufferSubDataEXT( GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %ld, %ld, %ld)\n", readBuffer, writeBuffer, readOffset, writeOffset, size );
  funcs->ext.p_glNamedCopyBufferSubDataEXT( readBuffer, writeBuffer, readOffset, writeOffset, size );
}

static void WINAPI glNamedFramebufferDrawBuffer( GLuint framebuffer, GLenum buf )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", framebuffer, buf );
  funcs->ext.p_glNamedFramebufferDrawBuffer( framebuffer, buf );
}

static void WINAPI glNamedFramebufferDrawBuffers( GLuint framebuffer, GLsizei n, const GLenum *bufs )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", framebuffer, n, bufs );
  funcs->ext.p_glNamedFramebufferDrawBuffers( framebuffer, n, bufs );
}

static void WINAPI glNamedFramebufferParameteri( GLuint framebuffer, GLenum pname, GLint param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", framebuffer, pname, param );
  funcs->ext.p_glNamedFramebufferParameteri( framebuffer, pname, param );
}

static void WINAPI glNamedFramebufferParameteriEXT( GLuint framebuffer, GLenum pname, GLint param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", framebuffer, pname, param );
  funcs->ext.p_glNamedFramebufferParameteriEXT( framebuffer, pname, param );
}

static void WINAPI glNamedFramebufferReadBuffer( GLuint framebuffer, GLenum src )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", framebuffer, src );
  funcs->ext.p_glNamedFramebufferReadBuffer( framebuffer, src );
}

static void WINAPI glNamedFramebufferRenderbuffer( GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", framebuffer, attachment, renderbuffertarget, renderbuffer );
  funcs->ext.p_glNamedFramebufferRenderbuffer( framebuffer, attachment, renderbuffertarget, renderbuffer );
}

static void WINAPI glNamedFramebufferRenderbufferEXT( GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", framebuffer, attachment, renderbuffertarget, renderbuffer );
  funcs->ext.p_glNamedFramebufferRenderbufferEXT( framebuffer, attachment, renderbuffertarget, renderbuffer );
}

static void WINAPI glNamedFramebufferSampleLocationsfvARB( GLuint framebuffer, GLuint start, GLsizei count, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", framebuffer, start, count, v );
  funcs->ext.p_glNamedFramebufferSampleLocationsfvARB( framebuffer, start, count, v );
}

static void WINAPI glNamedFramebufferSampleLocationsfvNV( GLuint framebuffer, GLuint start, GLsizei count, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", framebuffer, start, count, v );
  funcs->ext.p_glNamedFramebufferSampleLocationsfvNV( framebuffer, start, count, v );
}

static void WINAPI glNamedFramebufferSamplePositionsfvAMD( GLuint framebuffer, GLuint numsamples, GLuint pixelindex, const GLfloat *values )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", framebuffer, numsamples, pixelindex, values );
  funcs->ext.p_glNamedFramebufferSamplePositionsfvAMD( framebuffer, numsamples, pixelindex, values );
}

static void WINAPI glNamedFramebufferTexture( GLuint framebuffer, GLenum attachment, GLuint texture, GLint level )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", framebuffer, attachment, texture, level );
  funcs->ext.p_glNamedFramebufferTexture( framebuffer, attachment, texture, level );
}

static void WINAPI glNamedFramebufferTexture1DEXT( GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", framebuffer, attachment, textarget, texture, level );
  funcs->ext.p_glNamedFramebufferTexture1DEXT( framebuffer, attachment, textarget, texture, level );
}

static void WINAPI glNamedFramebufferTexture2DEXT( GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", framebuffer, attachment, textarget, texture, level );
  funcs->ext.p_glNamedFramebufferTexture2DEXT( framebuffer, attachment, textarget, texture, level );
}

static void WINAPI glNamedFramebufferTexture3DEXT( GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d)\n", framebuffer, attachment, textarget, texture, level, zoffset );
  funcs->ext.p_glNamedFramebufferTexture3DEXT( framebuffer, attachment, textarget, texture, level, zoffset );
}

static void WINAPI glNamedFramebufferTextureEXT( GLuint framebuffer, GLenum attachment, GLuint texture, GLint level )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", framebuffer, attachment, texture, level );
  funcs->ext.p_glNamedFramebufferTextureEXT( framebuffer, attachment, texture, level );
}

static void WINAPI glNamedFramebufferTextureFaceEXT( GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLenum face )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", framebuffer, attachment, texture, level, face );
  funcs->ext.p_glNamedFramebufferTextureFaceEXT( framebuffer, attachment, texture, level, face );
}

static void WINAPI glNamedFramebufferTextureLayer( GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", framebuffer, attachment, texture, level, layer );
  funcs->ext.p_glNamedFramebufferTextureLayer( framebuffer, attachment, texture, level, layer );
}

static void WINAPI glNamedFramebufferTextureLayerEXT( GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", framebuffer, attachment, texture, level, layer );
  funcs->ext.p_glNamedFramebufferTextureLayerEXT( framebuffer, attachment, texture, level, layer );
}

static void WINAPI glNamedProgramLocalParameter4dEXT( GLuint program, GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %f, %f, %f, %f)\n", program, target, index, x, y, z, w );
  funcs->ext.p_glNamedProgramLocalParameter4dEXT( program, target, index, x, y, z, w );
}

static void WINAPI glNamedProgramLocalParameter4dvEXT( GLuint program, GLenum target, GLuint index, const GLdouble *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, target, index, params );
  funcs->ext.p_glNamedProgramLocalParameter4dvEXT( program, target, index, params );
}

static void WINAPI glNamedProgramLocalParameter4fEXT( GLuint program, GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %f, %f, %f, %f)\n", program, target, index, x, y, z, w );
  funcs->ext.p_glNamedProgramLocalParameter4fEXT( program, target, index, x, y, z, w );
}

static void WINAPI glNamedProgramLocalParameter4fvEXT( GLuint program, GLenum target, GLuint index, const GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, target, index, params );
  funcs->ext.p_glNamedProgramLocalParameter4fvEXT( program, target, index, params );
}

static void WINAPI glNamedProgramLocalParameterI4iEXT( GLuint program, GLenum target, GLuint index, GLint x, GLint y, GLint z, GLint w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d)\n", program, target, index, x, y, z, w );
  funcs->ext.p_glNamedProgramLocalParameterI4iEXT( program, target, index, x, y, z, w );
}

static void WINAPI glNamedProgramLocalParameterI4ivEXT( GLuint program, GLenum target, GLuint index, const GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, target, index, params );
  funcs->ext.p_glNamedProgramLocalParameterI4ivEXT( program, target, index, params );
}

static void WINAPI glNamedProgramLocalParameterI4uiEXT( GLuint program, GLenum target, GLuint index, GLuint x, GLuint y, GLuint z, GLuint w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d)\n", program, target, index, x, y, z, w );
  funcs->ext.p_glNamedProgramLocalParameterI4uiEXT( program, target, index, x, y, z, w );
}

static void WINAPI glNamedProgramLocalParameterI4uivEXT( GLuint program, GLenum target, GLuint index, const GLuint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, target, index, params );
  funcs->ext.p_glNamedProgramLocalParameterI4uivEXT( program, target, index, params );
}

static void WINAPI glNamedProgramLocalParameters4fvEXT( GLuint program, GLenum target, GLuint index, GLsizei count, const GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, target, index, count, params );
  funcs->ext.p_glNamedProgramLocalParameters4fvEXT( program, target, index, count, params );
}

static void WINAPI glNamedProgramLocalParametersI4ivEXT( GLuint program, GLenum target, GLuint index, GLsizei count, const GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, target, index, count, params );
  funcs->ext.p_glNamedProgramLocalParametersI4ivEXT( program, target, index, count, params );
}

static void WINAPI glNamedProgramLocalParametersI4uivEXT( GLuint program, GLenum target, GLuint index, GLsizei count, const GLuint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, target, index, count, params );
  funcs->ext.p_glNamedProgramLocalParametersI4uivEXT( program, target, index, count, params );
}

static void WINAPI glNamedProgramStringEXT( GLuint program, GLenum target, GLenum format, GLsizei len, const void *string )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, target, format, len, string );
  funcs->ext.p_glNamedProgramStringEXT( program, target, format, len, string );
}

static void WINAPI glNamedRenderbufferStorage( GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", renderbuffer, internalformat, width, height );
  funcs->ext.p_glNamedRenderbufferStorage( renderbuffer, internalformat, width, height );
}

static void WINAPI glNamedRenderbufferStorageEXT( GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", renderbuffer, internalformat, width, height );
  funcs->ext.p_glNamedRenderbufferStorageEXT( renderbuffer, internalformat, width, height );
}

static void WINAPI glNamedRenderbufferStorageMultisample( GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", renderbuffer, samples, internalformat, width, height );
  funcs->ext.p_glNamedRenderbufferStorageMultisample( renderbuffer, samples, internalformat, width, height );
}

static void WINAPI glNamedRenderbufferStorageMultisampleCoverageEXT( GLuint renderbuffer, GLsizei coverageSamples, GLsizei colorSamples, GLenum internalformat, GLsizei width, GLsizei height )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d)\n", renderbuffer, coverageSamples, colorSamples, internalformat, width, height );
  funcs->ext.p_glNamedRenderbufferStorageMultisampleCoverageEXT( renderbuffer, coverageSamples, colorSamples, internalformat, width, height );
}

static void WINAPI glNamedRenderbufferStorageMultisampleEXT( GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", renderbuffer, samples, internalformat, width, height );
  funcs->ext.p_glNamedRenderbufferStorageMultisampleEXT( renderbuffer, samples, internalformat, width, height );
}

static void WINAPI glNamedStringARB( GLenum type, GLint namelen, const GLchar *name, GLint stringlen, const GLchar *string )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %d, %p)\n", type, namelen, name, stringlen, string );
  funcs->ext.p_glNamedStringARB( type, namelen, name, stringlen, string );
}

static GLuint WINAPI glNewBufferRegion( GLenum type )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", type );
  return funcs->ext.p_glNewBufferRegion( type );
}

static GLuint WINAPI glNewObjectBufferATI( GLsizei size, const void *pointer, GLenum usage )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p, %d)\n", size, pointer, usage );
  return funcs->ext.p_glNewObjectBufferATI( size, pointer, usage );
}

static void WINAPI glNormal3fVertex3fSUN( GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f, %f, %f, %f, %f)\n", nx, ny, nz, x, y, z );
  funcs->ext.p_glNormal3fVertex3fSUN( nx, ny, nz, x, y, z );
}

static void WINAPI glNormal3fVertex3fvSUN( const GLfloat *n, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p, %p)\n", n, v );
  funcs->ext.p_glNormal3fVertex3fvSUN( n, v );
}

static void WINAPI glNormal3hNV( GLhalfNV nx, GLhalfNV ny, GLhalfNV nz )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", nx, ny, nz );
  funcs->ext.p_glNormal3hNV( nx, ny, nz );
}

static void WINAPI glNormal3hvNV( const GLhalfNV *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glNormal3hvNV( v );
}

static void WINAPI glNormal3xOES( GLfixed nx, GLfixed ny, GLfixed nz )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", nx, ny, nz );
  funcs->ext.p_glNormal3xOES( nx, ny, nz );
}

static void WINAPI glNormal3xvOES( const GLfixed *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", coords );
  funcs->ext.p_glNormal3xvOES( coords );
}

static void WINAPI glNormalFormatNV( GLenum type, GLsizei stride )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", type, stride );
  funcs->ext.p_glNormalFormatNV( type, stride );
}

static void WINAPI glNormalP3ui( GLenum type, GLuint coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", type, coords );
  funcs->ext.p_glNormalP3ui( type, coords );
}

static void WINAPI glNormalP3uiv( GLenum type, const GLuint *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", type, coords );
  funcs->ext.p_glNormalP3uiv( type, coords );
}

static void WINAPI glNormalPointerEXT( GLenum type, GLsizei stride, GLsizei count, const void *pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", type, stride, count, pointer );
  funcs->ext.p_glNormalPointerEXT( type, stride, count, pointer );
}

static void WINAPI glNormalPointerListIBM( GLenum type, GLint stride, const void **pointer, GLint ptrstride )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %d)\n", type, stride, pointer, ptrstride );
  funcs->ext.p_glNormalPointerListIBM( type, stride, pointer, ptrstride );
}

static void WINAPI glNormalPointervINTEL( GLenum type, const void **pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", type, pointer );
  funcs->ext.p_glNormalPointervINTEL( type, pointer );
}

static void WINAPI glNormalStream3bATI( GLenum stream, GLbyte nx, GLbyte ny, GLbyte nz )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", stream, nx, ny, nz );
  funcs->ext.p_glNormalStream3bATI( stream, nx, ny, nz );
}

static void WINAPI glNormalStream3bvATI( GLenum stream, const GLbyte *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", stream, coords );
  funcs->ext.p_glNormalStream3bvATI( stream, coords );
}

static void WINAPI glNormalStream3dATI( GLenum stream, GLdouble nx, GLdouble ny, GLdouble nz )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f)\n", stream, nx, ny, nz );
  funcs->ext.p_glNormalStream3dATI( stream, nx, ny, nz );
}

static void WINAPI glNormalStream3dvATI( GLenum stream, const GLdouble *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", stream, coords );
  funcs->ext.p_glNormalStream3dvATI( stream, coords );
}

static void WINAPI glNormalStream3fATI( GLenum stream, GLfloat nx, GLfloat ny, GLfloat nz )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f)\n", stream, nx, ny, nz );
  funcs->ext.p_glNormalStream3fATI( stream, nx, ny, nz );
}

static void WINAPI glNormalStream3fvATI( GLenum stream, const GLfloat *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", stream, coords );
  funcs->ext.p_glNormalStream3fvATI( stream, coords );
}

static void WINAPI glNormalStream3iATI( GLenum stream, GLint nx, GLint ny, GLint nz )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", stream, nx, ny, nz );
  funcs->ext.p_glNormalStream3iATI( stream, nx, ny, nz );
}

static void WINAPI glNormalStream3ivATI( GLenum stream, const GLint *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", stream, coords );
  funcs->ext.p_glNormalStream3ivATI( stream, coords );
}

static void WINAPI glNormalStream3sATI( GLenum stream, GLshort nx, GLshort ny, GLshort nz )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", stream, nx, ny, nz );
  funcs->ext.p_glNormalStream3sATI( stream, nx, ny, nz );
}

static void WINAPI glNormalStream3svATI( GLenum stream, const GLshort *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", stream, coords );
  funcs->ext.p_glNormalStream3svATI( stream, coords );
}

static void WINAPI glObjectLabel( GLenum identifier, GLuint name, GLsizei length, const GLchar *label )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", identifier, name, length, label );
  funcs->ext.p_glObjectLabel( identifier, name, length, label );
}

static void WINAPI glObjectPtrLabel( const void *ptr, GLsizei length, const GLchar *label )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p, %d, %p)\n", ptr, length, label );
  funcs->ext.p_glObjectPtrLabel( ptr, length, label );
}

static GLenum WINAPI glObjectPurgeableAPPLE( GLenum objectType, GLuint name, GLenum option )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", objectType, name, option );
  return funcs->ext.p_glObjectPurgeableAPPLE( objectType, name, option );
}

static GLenum WINAPI glObjectUnpurgeableAPPLE( GLenum objectType, GLuint name, GLenum option )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", objectType, name, option );
  return funcs->ext.p_glObjectUnpurgeableAPPLE( objectType, name, option );
}

static void WINAPI glOrthofOES( GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f, %f, %f, %f, %f)\n", l, r, b, t, n, f );
  funcs->ext.p_glOrthofOES( l, r, b, t, n, f );
}

static void WINAPI glOrthoxOES( GLfixed l, GLfixed r, GLfixed b, GLfixed t, GLfixed n, GLfixed f )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d)\n", l, r, b, t, n, f );
  funcs->ext.p_glOrthoxOES( l, r, b, t, n, f );
}

static void WINAPI glPNTrianglesfATI( GLenum pname, GLfloat param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f)\n", pname, param );
  funcs->ext.p_glPNTrianglesfATI( pname, param );
}

static void WINAPI glPNTrianglesiATI( GLenum pname, GLint param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", pname, param );
  funcs->ext.p_glPNTrianglesiATI( pname, param );
}

static void WINAPI glPassTexCoordATI( GLuint dst, GLuint coord, GLenum swizzle )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", dst, coord, swizzle );
  funcs->ext.p_glPassTexCoordATI( dst, coord, swizzle );
}

static void WINAPI glPassThroughxOES( GLfixed token )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", token );
  funcs->ext.p_glPassThroughxOES( token );
}

static void WINAPI glPatchParameterfv( GLenum pname, const GLfloat *values )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", pname, values );
  funcs->ext.p_glPatchParameterfv( pname, values );
}

static void WINAPI glPatchParameteri( GLenum pname, GLint value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", pname, value );
  funcs->ext.p_glPatchParameteri( pname, value );
}

static void WINAPI glPathColorGenNV( GLenum color, GLenum genMode, GLenum colorFormat, const GLfloat *coeffs )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", color, genMode, colorFormat, coeffs );
  funcs->ext.p_glPathColorGenNV( color, genMode, colorFormat, coeffs );
}

static void WINAPI glPathCommandsNV( GLuint path, GLsizei numCommands, const GLubyte *commands, GLsizei numCoords, GLenum coordType, const void *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %d, %d, %p)\n", path, numCommands, commands, numCoords, coordType, coords );
  funcs->ext.p_glPathCommandsNV( path, numCommands, commands, numCoords, coordType, coords );
}

static void WINAPI glPathCoordsNV( GLuint path, GLsizei numCoords, GLenum coordType, const void *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", path, numCoords, coordType, coords );
  funcs->ext.p_glPathCoordsNV( path, numCoords, coordType, coords );
}

static void WINAPI glPathCoverDepthFuncNV( GLenum func )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", func );
  funcs->ext.p_glPathCoverDepthFuncNV( func );
}

static void WINAPI glPathDashArrayNV( GLuint path, GLsizei dashCount, const GLfloat *dashArray )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", path, dashCount, dashArray );
  funcs->ext.p_glPathDashArrayNV( path, dashCount, dashArray );
}

static void WINAPI glPathFogGenNV( GLenum genMode )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", genMode );
  funcs->ext.p_glPathFogGenNV( genMode );
}

static GLenum WINAPI glPathGlyphIndexArrayNV( GLuint firstPathName, GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLuint firstGlyphIndex, GLsizei numGlyphs, GLuint pathParameterTemplate, GLfloat emScale )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %d, %d, %d, %d, %f)\n", firstPathName, fontTarget, fontName, fontStyle, firstGlyphIndex, numGlyphs, pathParameterTemplate, emScale );
  return funcs->ext.p_glPathGlyphIndexArrayNV( firstPathName, fontTarget, fontName, fontStyle, firstGlyphIndex, numGlyphs, pathParameterTemplate, emScale );
}

static GLenum WINAPI glPathGlyphIndexRangeNV( GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLuint pathParameterTemplate, GLfloat emScale, GLuint baseAndCount[2] )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p, %d, %d, %f, %p)\n", fontTarget, fontName, fontStyle, pathParameterTemplate, emScale, baseAndCount );
  return funcs->ext.p_glPathGlyphIndexRangeNV( fontTarget, fontName, fontStyle, pathParameterTemplate, emScale, baseAndCount );
}

static void WINAPI glPathGlyphRangeNV( GLuint firstPathName, GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLuint firstGlyph, GLsizei numGlyphs, GLenum handleMissingGlyphs, GLuint pathParameterTemplate, GLfloat emScale )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %d, %d, %d, %d, %d, %f)\n", firstPathName, fontTarget, fontName, fontStyle, firstGlyph, numGlyphs, handleMissingGlyphs, pathParameterTemplate, emScale );
  funcs->ext.p_glPathGlyphRangeNV( firstPathName, fontTarget, fontName, fontStyle, firstGlyph, numGlyphs, handleMissingGlyphs, pathParameterTemplate, emScale );
}

static void WINAPI glPathGlyphsNV( GLuint firstPathName, GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLsizei numGlyphs, GLenum type, const void *charcodes, GLenum handleMissingGlyphs, GLuint pathParameterTemplate, GLfloat emScale )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %d, %d, %d, %p, %d, %d, %f)\n", firstPathName, fontTarget, fontName, fontStyle, numGlyphs, type, charcodes, handleMissingGlyphs, pathParameterTemplate, emScale );
  funcs->ext.p_glPathGlyphsNV( firstPathName, fontTarget, fontName, fontStyle, numGlyphs, type, charcodes, handleMissingGlyphs, pathParameterTemplate, emScale );
}

static GLenum WINAPI glPathMemoryGlyphIndexArrayNV( GLuint firstPathName, GLenum fontTarget, GLsizeiptr fontSize, const void *fontData, GLsizei faceIndex, GLuint firstGlyphIndex, GLsizei numGlyphs, GLuint pathParameterTemplate, GLfloat emScale )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %ld, %p, %d, %d, %d, %d, %f)\n", firstPathName, fontTarget, fontSize, fontData, faceIndex, firstGlyphIndex, numGlyphs, pathParameterTemplate, emScale );
  return funcs->ext.p_glPathMemoryGlyphIndexArrayNV( firstPathName, fontTarget, fontSize, fontData, faceIndex, firstGlyphIndex, numGlyphs, pathParameterTemplate, emScale );
}

static void WINAPI glPathParameterfNV( GLuint path, GLenum pname, GLfloat value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %f)\n", path, pname, value );
  funcs->ext.p_glPathParameterfNV( path, pname, value );
}

static void WINAPI glPathParameterfvNV( GLuint path, GLenum pname, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", path, pname, value );
  funcs->ext.p_glPathParameterfvNV( path, pname, value );
}

static void WINAPI glPathParameteriNV( GLuint path, GLenum pname, GLint value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", path, pname, value );
  funcs->ext.p_glPathParameteriNV( path, pname, value );
}

static void WINAPI glPathParameterivNV( GLuint path, GLenum pname, const GLint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", path, pname, value );
  funcs->ext.p_glPathParameterivNV( path, pname, value );
}

static void WINAPI glPathStencilDepthOffsetNV( GLfloat factor, GLfloat units )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f)\n", factor, units );
  funcs->ext.p_glPathStencilDepthOffsetNV( factor, units );
}

static void WINAPI glPathStencilFuncNV( GLenum func, GLint ref, GLuint mask )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", func, ref, mask );
  funcs->ext.p_glPathStencilFuncNV( func, ref, mask );
}

static void WINAPI glPathStringNV( GLuint path, GLenum format, GLsizei length, const void *pathString )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", path, format, length, pathString );
  funcs->ext.p_glPathStringNV( path, format, length, pathString );
}

static void WINAPI glPathSubCommandsNV( GLuint path, GLsizei commandStart, GLsizei commandsToDelete, GLsizei numCommands, const GLubyte *commands, GLsizei numCoords, GLenum coordType, const void *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p, %d, %d, %p)\n", path, commandStart, commandsToDelete, numCommands, commands, numCoords, coordType, coords );
  funcs->ext.p_glPathSubCommandsNV( path, commandStart, commandsToDelete, numCommands, commands, numCoords, coordType, coords );
}

static void WINAPI glPathSubCoordsNV( GLuint path, GLsizei coordStart, GLsizei numCoords, GLenum coordType, const void *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", path, coordStart, numCoords, coordType, coords );
  funcs->ext.p_glPathSubCoordsNV( path, coordStart, numCoords, coordType, coords );
}

static void WINAPI glPathTexGenNV( GLenum texCoordSet, GLenum genMode, GLint components, const GLfloat *coeffs )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", texCoordSet, genMode, components, coeffs );
  funcs->ext.p_glPathTexGenNV( texCoordSet, genMode, components, coeffs );
}

static void WINAPI glPauseTransformFeedback(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  funcs->ext.p_glPauseTransformFeedback();
}

static void WINAPI glPauseTransformFeedbackNV(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  funcs->ext.p_glPauseTransformFeedbackNV();
}

static void WINAPI glPixelDataRangeNV( GLenum target, GLsizei length, const void *pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, length, pointer );
  funcs->ext.p_glPixelDataRangeNV( target, length, pointer );
}

static void WINAPI glPixelMapx( GLenum map, GLint size, const GLfixed *values )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", map, size, values );
  funcs->ext.p_glPixelMapx( map, size, values );
}

static void WINAPI glPixelStorex( GLenum pname, GLfixed param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", pname, param );
  funcs->ext.p_glPixelStorex( pname, param );
}

static void WINAPI glPixelTexGenParameterfSGIS( GLenum pname, GLfloat param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f)\n", pname, param );
  funcs->ext.p_glPixelTexGenParameterfSGIS( pname, param );
}

static void WINAPI glPixelTexGenParameterfvSGIS( GLenum pname, const GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", pname, params );
  funcs->ext.p_glPixelTexGenParameterfvSGIS( pname, params );
}

static void WINAPI glPixelTexGenParameteriSGIS( GLenum pname, GLint param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", pname, param );
  funcs->ext.p_glPixelTexGenParameteriSGIS( pname, param );
}

static void WINAPI glPixelTexGenParameterivSGIS( GLenum pname, const GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", pname, params );
  funcs->ext.p_glPixelTexGenParameterivSGIS( pname, params );
}

static void WINAPI glPixelTexGenSGIX( GLenum mode )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", mode );
  funcs->ext.p_glPixelTexGenSGIX( mode );
}

static void WINAPI glPixelTransferxOES( GLenum pname, GLfixed param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", pname, param );
  funcs->ext.p_glPixelTransferxOES( pname, param );
}

static void WINAPI glPixelTransformParameterfEXT( GLenum target, GLenum pname, GLfloat param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %f)\n", target, pname, param );
  funcs->ext.p_glPixelTransformParameterfEXT( target, pname, param );
}

static void WINAPI glPixelTransformParameterfvEXT( GLenum target, GLenum pname, const GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glPixelTransformParameterfvEXT( target, pname, params );
}

static void WINAPI glPixelTransformParameteriEXT( GLenum target, GLenum pname, GLint param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", target, pname, param );
  funcs->ext.p_glPixelTransformParameteriEXT( target, pname, param );
}

static void WINAPI glPixelTransformParameterivEXT( GLenum target, GLenum pname, const GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glPixelTransformParameterivEXT( target, pname, params );
}

static void WINAPI glPixelZoomxOES( GLfixed xfactor, GLfixed yfactor )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", xfactor, yfactor );
  funcs->ext.p_glPixelZoomxOES( xfactor, yfactor );
}

static GLboolean WINAPI glPointAlongPathNV( GLuint path, GLsizei startSegment, GLsizei numSegments, GLfloat distance, GLfloat *x, GLfloat *y, GLfloat *tangentX, GLfloat *tangentY )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %f, %p, %p, %p, %p)\n", path, startSegment, numSegments, distance, x, y, tangentX, tangentY );
  return funcs->ext.p_glPointAlongPathNV( path, startSegment, numSegments, distance, x, y, tangentX, tangentY );
}

static void WINAPI glPointParameterf( GLenum pname, GLfloat param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f)\n", pname, param );
  funcs->ext.p_glPointParameterf( pname, param );
}

static void WINAPI glPointParameterfARB( GLenum pname, GLfloat param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f)\n", pname, param );
  funcs->ext.p_glPointParameterfARB( pname, param );
}

static void WINAPI glPointParameterfEXT( GLenum pname, GLfloat param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f)\n", pname, param );
  funcs->ext.p_glPointParameterfEXT( pname, param );
}

static void WINAPI glPointParameterfSGIS( GLenum pname, GLfloat param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f)\n", pname, param );
  funcs->ext.p_glPointParameterfSGIS( pname, param );
}

static void WINAPI glPointParameterfv( GLenum pname, const GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", pname, params );
  funcs->ext.p_glPointParameterfv( pname, params );
}

static void WINAPI glPointParameterfvARB( GLenum pname, const GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", pname, params );
  funcs->ext.p_glPointParameterfvARB( pname, params );
}

static void WINAPI glPointParameterfvEXT( GLenum pname, const GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", pname, params );
  funcs->ext.p_glPointParameterfvEXT( pname, params );
}

static void WINAPI glPointParameterfvSGIS( GLenum pname, const GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", pname, params );
  funcs->ext.p_glPointParameterfvSGIS( pname, params );
}

static void WINAPI glPointParameteri( GLenum pname, GLint param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", pname, param );
  funcs->ext.p_glPointParameteri( pname, param );
}

static void WINAPI glPointParameteriNV( GLenum pname, GLint param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", pname, param );
  funcs->ext.p_glPointParameteriNV( pname, param );
}

static void WINAPI glPointParameteriv( GLenum pname, const GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", pname, params );
  funcs->ext.p_glPointParameteriv( pname, params );
}

static void WINAPI glPointParameterivNV( GLenum pname, const GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", pname, params );
  funcs->ext.p_glPointParameterivNV( pname, params );
}

static void WINAPI glPointParameterxvOES( GLenum pname, const GLfixed *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", pname, params );
  funcs->ext.p_glPointParameterxvOES( pname, params );
}

static void WINAPI glPointSizexOES( GLfixed size )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", size );
  funcs->ext.p_glPointSizexOES( size );
}

static GLint WINAPI glPollAsyncSGIX( GLuint *markerp )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", markerp );
  return funcs->ext.p_glPollAsyncSGIX( markerp );
}

static GLint WINAPI glPollInstrumentsSGIX( GLint *marker_p )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", marker_p );
  return funcs->ext.p_glPollInstrumentsSGIX( marker_p );
}

static void WINAPI glPolygonOffsetClamp( GLfloat factor, GLfloat units, GLfloat clamp )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f, %f)\n", factor, units, clamp );
  funcs->ext.p_glPolygonOffsetClamp( factor, units, clamp );
}

static void WINAPI glPolygonOffsetClampEXT( GLfloat factor, GLfloat units, GLfloat clamp )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f, %f)\n", factor, units, clamp );
  funcs->ext.p_glPolygonOffsetClampEXT( factor, units, clamp );
}

static void WINAPI glPolygonOffsetEXT( GLfloat factor, GLfloat bias )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f)\n", factor, bias );
  funcs->ext.p_glPolygonOffsetEXT( factor, bias );
}

static void WINAPI glPolygonOffsetxOES( GLfixed factor, GLfixed units )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", factor, units );
  funcs->ext.p_glPolygonOffsetxOES( factor, units );
}

static void WINAPI glPopDebugGroup(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  funcs->ext.p_glPopDebugGroup();
}

static void WINAPI glPopGroupMarkerEXT(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  funcs->ext.p_glPopGroupMarkerEXT();
}

static void WINAPI glPresentFrameDualFillNV( GLuint video_slot, GLuint64EXT minPresentTime, GLuint beginPresentTimeId, GLuint presentDurationId, GLenum type, GLenum target0, GLuint fill0, GLenum target1, GLuint fill1, GLenum target2, GLuint fill2, GLenum target3, GLuint fill3 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %s, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", video_slot, wine_dbgstr_longlong(minPresentTime), beginPresentTimeId, presentDurationId, type, target0, fill0, target1, fill1, target2, fill2, target3, fill3 );
  funcs->ext.p_glPresentFrameDualFillNV( video_slot, minPresentTime, beginPresentTimeId, presentDurationId, type, target0, fill0, target1, fill1, target2, fill2, target3, fill3 );
}

static void WINAPI glPresentFrameKeyedNV( GLuint video_slot, GLuint64EXT minPresentTime, GLuint beginPresentTimeId, GLuint presentDurationId, GLenum type, GLenum target0, GLuint fill0, GLuint key0, GLenum target1, GLuint fill1, GLuint key1 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %s, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", video_slot, wine_dbgstr_longlong(minPresentTime), beginPresentTimeId, presentDurationId, type, target0, fill0, key0, target1, fill1, key1 );
  funcs->ext.p_glPresentFrameKeyedNV( video_slot, minPresentTime, beginPresentTimeId, presentDurationId, type, target0, fill0, key0, target1, fill1, key1 );
}

static void WINAPI glPrimitiveBoundingBoxARB( GLfloat minX, GLfloat minY, GLfloat minZ, GLfloat minW, GLfloat maxX, GLfloat maxY, GLfloat maxZ, GLfloat maxW )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f, %f, %f, %f, %f, %f, %f)\n", minX, minY, minZ, minW, maxX, maxY, maxZ, maxW );
  funcs->ext.p_glPrimitiveBoundingBoxARB( minX, minY, minZ, minW, maxX, maxY, maxZ, maxW );
}

static void WINAPI glPrimitiveRestartIndex( GLuint index )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", index );
  funcs->ext.p_glPrimitiveRestartIndex( index );
}

static void WINAPI glPrimitiveRestartIndexNV( GLuint index )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", index );
  funcs->ext.p_glPrimitiveRestartIndexNV( index );
}

static void WINAPI glPrimitiveRestartNV(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  funcs->ext.p_glPrimitiveRestartNV();
}

static void WINAPI glPrioritizeTexturesEXT( GLsizei n, const GLuint *textures, const GLclampf *priorities )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p, %p)\n", n, textures, priorities );
  funcs->ext.p_glPrioritizeTexturesEXT( n, textures, priorities );
}

static void WINAPI glPrioritizeTexturesxOES( GLsizei n, const GLuint *textures, const GLfixed *priorities )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p, %p)\n", n, textures, priorities );
  funcs->ext.p_glPrioritizeTexturesxOES( n, textures, priorities );
}

static void WINAPI glProgramBinary( GLuint program, GLenum binaryFormat, const void *binary, GLsizei length )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %d)\n", program, binaryFormat, binary, length );
  funcs->ext.p_glProgramBinary( program, binaryFormat, binary, length );
}

static void WINAPI glProgramBufferParametersIivNV( GLenum target, GLuint bindingIndex, GLuint wordIndex, GLsizei count, const GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", target, bindingIndex, wordIndex, count, params );
  funcs->ext.p_glProgramBufferParametersIivNV( target, bindingIndex, wordIndex, count, params );
}

static void WINAPI glProgramBufferParametersIuivNV( GLenum target, GLuint bindingIndex, GLuint wordIndex, GLsizei count, const GLuint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", target, bindingIndex, wordIndex, count, params );
  funcs->ext.p_glProgramBufferParametersIuivNV( target, bindingIndex, wordIndex, count, params );
}

static void WINAPI glProgramBufferParametersfvNV( GLenum target, GLuint bindingIndex, GLuint wordIndex, GLsizei count, const GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", target, bindingIndex, wordIndex, count, params );
  funcs->ext.p_glProgramBufferParametersfvNV( target, bindingIndex, wordIndex, count, params );
}

static void WINAPI glProgramEnvParameter4dARB( GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %f, %f, %f, %f)\n", target, index, x, y, z, w );
  funcs->ext.p_glProgramEnvParameter4dARB( target, index, x, y, z, w );
}

static void WINAPI glProgramEnvParameter4dvARB( GLenum target, GLuint index, const GLdouble *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, index, params );
  funcs->ext.p_glProgramEnvParameter4dvARB( target, index, params );
}

static void WINAPI glProgramEnvParameter4fARB( GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %f, %f, %f, %f)\n", target, index, x, y, z, w );
  funcs->ext.p_glProgramEnvParameter4fARB( target, index, x, y, z, w );
}

static void WINAPI glProgramEnvParameter4fvARB( GLenum target, GLuint index, const GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, index, params );
  funcs->ext.p_glProgramEnvParameter4fvARB( target, index, params );
}

static void WINAPI glProgramEnvParameterI4iNV( GLenum target, GLuint index, GLint x, GLint y, GLint z, GLint w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d)\n", target, index, x, y, z, w );
  funcs->ext.p_glProgramEnvParameterI4iNV( target, index, x, y, z, w );
}

static void WINAPI glProgramEnvParameterI4ivNV( GLenum target, GLuint index, const GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, index, params );
  funcs->ext.p_glProgramEnvParameterI4ivNV( target, index, params );
}

static void WINAPI glProgramEnvParameterI4uiNV( GLenum target, GLuint index, GLuint x, GLuint y, GLuint z, GLuint w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d)\n", target, index, x, y, z, w );
  funcs->ext.p_glProgramEnvParameterI4uiNV( target, index, x, y, z, w );
}

static void WINAPI glProgramEnvParameterI4uivNV( GLenum target, GLuint index, const GLuint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, index, params );
  funcs->ext.p_glProgramEnvParameterI4uivNV( target, index, params );
}

static void WINAPI glProgramEnvParameters4fvEXT( GLenum target, GLuint index, GLsizei count, const GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", target, index, count, params );
  funcs->ext.p_glProgramEnvParameters4fvEXT( target, index, count, params );
}

static void WINAPI glProgramEnvParametersI4ivNV( GLenum target, GLuint index, GLsizei count, const GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", target, index, count, params );
  funcs->ext.p_glProgramEnvParametersI4ivNV( target, index, count, params );
}

static void WINAPI glProgramEnvParametersI4uivNV( GLenum target, GLuint index, GLsizei count, const GLuint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", target, index, count, params );
  funcs->ext.p_glProgramEnvParametersI4uivNV( target, index, count, params );
}

static void WINAPI glProgramLocalParameter4dARB( GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %f, %f, %f, %f)\n", target, index, x, y, z, w );
  funcs->ext.p_glProgramLocalParameter4dARB( target, index, x, y, z, w );
}

static void WINAPI glProgramLocalParameter4dvARB( GLenum target, GLuint index, const GLdouble *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, index, params );
  funcs->ext.p_glProgramLocalParameter4dvARB( target, index, params );
}

static void WINAPI glProgramLocalParameter4fARB( GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %f, %f, %f, %f)\n", target, index, x, y, z, w );
  funcs->ext.p_glProgramLocalParameter4fARB( target, index, x, y, z, w );
}

static void WINAPI glProgramLocalParameter4fvARB( GLenum target, GLuint index, const GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, index, params );
  funcs->ext.p_glProgramLocalParameter4fvARB( target, index, params );
}

static void WINAPI glProgramLocalParameterI4iNV( GLenum target, GLuint index, GLint x, GLint y, GLint z, GLint w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d)\n", target, index, x, y, z, w );
  funcs->ext.p_glProgramLocalParameterI4iNV( target, index, x, y, z, w );
}

static void WINAPI glProgramLocalParameterI4ivNV( GLenum target, GLuint index, const GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, index, params );
  funcs->ext.p_glProgramLocalParameterI4ivNV( target, index, params );
}

static void WINAPI glProgramLocalParameterI4uiNV( GLenum target, GLuint index, GLuint x, GLuint y, GLuint z, GLuint w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d)\n", target, index, x, y, z, w );
  funcs->ext.p_glProgramLocalParameterI4uiNV( target, index, x, y, z, w );
}

static void WINAPI glProgramLocalParameterI4uivNV( GLenum target, GLuint index, const GLuint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, index, params );
  funcs->ext.p_glProgramLocalParameterI4uivNV( target, index, params );
}

static void WINAPI glProgramLocalParameters4fvEXT( GLenum target, GLuint index, GLsizei count, const GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", target, index, count, params );
  funcs->ext.p_glProgramLocalParameters4fvEXT( target, index, count, params );
}

static void WINAPI glProgramLocalParametersI4ivNV( GLenum target, GLuint index, GLsizei count, const GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", target, index, count, params );
  funcs->ext.p_glProgramLocalParametersI4ivNV( target, index, count, params );
}

static void WINAPI glProgramLocalParametersI4uivNV( GLenum target, GLuint index, GLsizei count, const GLuint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", target, index, count, params );
  funcs->ext.p_glProgramLocalParametersI4uivNV( target, index, count, params );
}

static void WINAPI glProgramNamedParameter4dNV( GLuint id, GLsizei len, const GLubyte *name, GLdouble x, GLdouble y, GLdouble z, GLdouble w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %f, %f, %f, %f)\n", id, len, name, x, y, z, w );
  funcs->ext.p_glProgramNamedParameter4dNV( id, len, name, x, y, z, w );
}

static void WINAPI glProgramNamedParameter4dvNV( GLuint id, GLsizei len, const GLubyte *name, const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %p)\n", id, len, name, v );
  funcs->ext.p_glProgramNamedParameter4dvNV( id, len, name, v );
}

static void WINAPI glProgramNamedParameter4fNV( GLuint id, GLsizei len, const GLubyte *name, GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %f, %f, %f, %f)\n", id, len, name, x, y, z, w );
  funcs->ext.p_glProgramNamedParameter4fNV( id, len, name, x, y, z, w );
}

static void WINAPI glProgramNamedParameter4fvNV( GLuint id, GLsizei len, const GLubyte *name, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %p)\n", id, len, name, v );
  funcs->ext.p_glProgramNamedParameter4fvNV( id, len, name, v );
}

static void WINAPI glProgramParameter4dNV( GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %f, %f, %f, %f)\n", target, index, x, y, z, w );
  funcs->ext.p_glProgramParameter4dNV( target, index, x, y, z, w );
}

static void WINAPI glProgramParameter4dvNV( GLenum target, GLuint index, const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, index, v );
  funcs->ext.p_glProgramParameter4dvNV( target, index, v );
}

static void WINAPI glProgramParameter4fNV( GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %f, %f, %f, %f)\n", target, index, x, y, z, w );
  funcs->ext.p_glProgramParameter4fNV( target, index, x, y, z, w );
}

static void WINAPI glProgramParameter4fvNV( GLenum target, GLuint index, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, index, v );
  funcs->ext.p_glProgramParameter4fvNV( target, index, v );
}

static void WINAPI glProgramParameteri( GLuint program, GLenum pname, GLint value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", program, pname, value );
  funcs->ext.p_glProgramParameteri( program, pname, value );
}

static void WINAPI glProgramParameteriARB( GLuint program, GLenum pname, GLint value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", program, pname, value );
  funcs->ext.p_glProgramParameteriARB( program, pname, value );
}

static void WINAPI glProgramParameteriEXT( GLuint program, GLenum pname, GLint value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", program, pname, value );
  funcs->ext.p_glProgramParameteriEXT( program, pname, value );
}

static void WINAPI glProgramParameters4dvNV( GLenum target, GLuint index, GLsizei count, const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", target, index, count, v );
  funcs->ext.p_glProgramParameters4dvNV( target, index, count, v );
}

static void WINAPI glProgramParameters4fvNV( GLenum target, GLuint index, GLsizei count, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", target, index, count, v );
  funcs->ext.p_glProgramParameters4fvNV( target, index, count, v );
}

static void WINAPI glProgramPathFragmentInputGenNV( GLuint program, GLint location, GLenum genMode, GLint components, const GLfloat *coeffs )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, location, genMode, components, coeffs );
  funcs->ext.p_glProgramPathFragmentInputGenNV( program, location, genMode, components, coeffs );
}

static void WINAPI glProgramStringARB( GLenum target, GLenum format, GLsizei len, const void *string )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", target, format, len, string );
  funcs->ext.p_glProgramStringARB( target, format, len, string );
}

static void WINAPI glProgramSubroutineParametersuivNV( GLenum target, GLsizei count, const GLuint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, count, params );
  funcs->ext.p_glProgramSubroutineParametersuivNV( target, count, params );
}

static void WINAPI glProgramUniform1d( GLuint program, GLint location, GLdouble v0 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %f)\n", program, location, v0 );
  funcs->ext.p_glProgramUniform1d( program, location, v0 );
}

static void WINAPI glProgramUniform1dEXT( GLuint program, GLint location, GLdouble x )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %f)\n", program, location, x );
  funcs->ext.p_glProgramUniform1dEXT( program, location, x );
}

static void WINAPI glProgramUniform1dv( GLuint program, GLint location, GLsizei count, const GLdouble *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform1dv( program, location, count, value );
}

static void WINAPI glProgramUniform1dvEXT( GLuint program, GLint location, GLsizei count, const GLdouble *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform1dvEXT( program, location, count, value );
}

static void WINAPI glProgramUniform1f( GLuint program, GLint location, GLfloat v0 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %f)\n", program, location, v0 );
  funcs->ext.p_glProgramUniform1f( program, location, v0 );
}

static void WINAPI glProgramUniform1fEXT( GLuint program, GLint location, GLfloat v0 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %f)\n", program, location, v0 );
  funcs->ext.p_glProgramUniform1fEXT( program, location, v0 );
}

static void WINAPI glProgramUniform1fv( GLuint program, GLint location, GLsizei count, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform1fv( program, location, count, value );
}

static void WINAPI glProgramUniform1fvEXT( GLuint program, GLint location, GLsizei count, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform1fvEXT( program, location, count, value );
}

static void WINAPI glProgramUniform1i( GLuint program, GLint location, GLint v0 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", program, location, v0 );
  funcs->ext.p_glProgramUniform1i( program, location, v0 );
}

static void WINAPI glProgramUniform1i64ARB( GLuint program, GLint location, GLint64 x )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %s)\n", program, location, wine_dbgstr_longlong(x) );
  funcs->ext.p_glProgramUniform1i64ARB( program, location, x );
}

static void WINAPI glProgramUniform1i64NV( GLuint program, GLint location, GLint64EXT x )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %s)\n", program, location, wine_dbgstr_longlong(x) );
  funcs->ext.p_glProgramUniform1i64NV( program, location, x );
}

static void WINAPI glProgramUniform1i64vARB( GLuint program, GLint location, GLsizei count, const GLint64 *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform1i64vARB( program, location, count, value );
}

static void WINAPI glProgramUniform1i64vNV( GLuint program, GLint location, GLsizei count, const GLint64EXT *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform1i64vNV( program, location, count, value );
}

static void WINAPI glProgramUniform1iEXT( GLuint program, GLint location, GLint v0 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", program, location, v0 );
  funcs->ext.p_glProgramUniform1iEXT( program, location, v0 );
}

static void WINAPI glProgramUniform1iv( GLuint program, GLint location, GLsizei count, const GLint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform1iv( program, location, count, value );
}

static void WINAPI glProgramUniform1ivEXT( GLuint program, GLint location, GLsizei count, const GLint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform1ivEXT( program, location, count, value );
}

static void WINAPI glProgramUniform1ui( GLuint program, GLint location, GLuint v0 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", program, location, v0 );
  funcs->ext.p_glProgramUniform1ui( program, location, v0 );
}

static void WINAPI glProgramUniform1ui64ARB( GLuint program, GLint location, GLuint64 x )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %s)\n", program, location, wine_dbgstr_longlong(x) );
  funcs->ext.p_glProgramUniform1ui64ARB( program, location, x );
}

static void WINAPI glProgramUniform1ui64NV( GLuint program, GLint location, GLuint64EXT x )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %s)\n", program, location, wine_dbgstr_longlong(x) );
  funcs->ext.p_glProgramUniform1ui64NV( program, location, x );
}

static void WINAPI glProgramUniform1ui64vARB( GLuint program, GLint location, GLsizei count, const GLuint64 *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform1ui64vARB( program, location, count, value );
}

static void WINAPI glProgramUniform1ui64vNV( GLuint program, GLint location, GLsizei count, const GLuint64EXT *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform1ui64vNV( program, location, count, value );
}

static void WINAPI glProgramUniform1uiEXT( GLuint program, GLint location, GLuint v0 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", program, location, v0 );
  funcs->ext.p_glProgramUniform1uiEXT( program, location, v0 );
}

static void WINAPI glProgramUniform1uiv( GLuint program, GLint location, GLsizei count, const GLuint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform1uiv( program, location, count, value );
}

static void WINAPI glProgramUniform1uivEXT( GLuint program, GLint location, GLsizei count, const GLuint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform1uivEXT( program, location, count, value );
}

static void WINAPI glProgramUniform2d( GLuint program, GLint location, GLdouble v0, GLdouble v1 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %f, %f)\n", program, location, v0, v1 );
  funcs->ext.p_glProgramUniform2d( program, location, v0, v1 );
}

static void WINAPI glProgramUniform2dEXT( GLuint program, GLint location, GLdouble x, GLdouble y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %f, %f)\n", program, location, x, y );
  funcs->ext.p_glProgramUniform2dEXT( program, location, x, y );
}

static void WINAPI glProgramUniform2dv( GLuint program, GLint location, GLsizei count, const GLdouble *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform2dv( program, location, count, value );
}

static void WINAPI glProgramUniform2dvEXT( GLuint program, GLint location, GLsizei count, const GLdouble *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform2dvEXT( program, location, count, value );
}

static void WINAPI glProgramUniform2f( GLuint program, GLint location, GLfloat v0, GLfloat v1 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %f, %f)\n", program, location, v0, v1 );
  funcs->ext.p_glProgramUniform2f( program, location, v0, v1 );
}

static void WINAPI glProgramUniform2fEXT( GLuint program, GLint location, GLfloat v0, GLfloat v1 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %f, %f)\n", program, location, v0, v1 );
  funcs->ext.p_glProgramUniform2fEXT( program, location, v0, v1 );
}

static void WINAPI glProgramUniform2fv( GLuint program, GLint location, GLsizei count, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform2fv( program, location, count, value );
}

static void WINAPI glProgramUniform2fvEXT( GLuint program, GLint location, GLsizei count, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform2fvEXT( program, location, count, value );
}

static void WINAPI glProgramUniform2i( GLuint program, GLint location, GLint v0, GLint v1 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", program, location, v0, v1 );
  funcs->ext.p_glProgramUniform2i( program, location, v0, v1 );
}

static void WINAPI glProgramUniform2i64ARB( GLuint program, GLint location, GLint64 x, GLint64 y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %s, %s)\n", program, location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y) );
  funcs->ext.p_glProgramUniform2i64ARB( program, location, x, y );
}

static void WINAPI glProgramUniform2i64NV( GLuint program, GLint location, GLint64EXT x, GLint64EXT y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %s, %s)\n", program, location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y) );
  funcs->ext.p_glProgramUniform2i64NV( program, location, x, y );
}

static void WINAPI glProgramUniform2i64vARB( GLuint program, GLint location, GLsizei count, const GLint64 *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform2i64vARB( program, location, count, value );
}

static void WINAPI glProgramUniform2i64vNV( GLuint program, GLint location, GLsizei count, const GLint64EXT *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform2i64vNV( program, location, count, value );
}

static void WINAPI glProgramUniform2iEXT( GLuint program, GLint location, GLint v0, GLint v1 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", program, location, v0, v1 );
  funcs->ext.p_glProgramUniform2iEXT( program, location, v0, v1 );
}

static void WINAPI glProgramUniform2iv( GLuint program, GLint location, GLsizei count, const GLint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform2iv( program, location, count, value );
}

static void WINAPI glProgramUniform2ivEXT( GLuint program, GLint location, GLsizei count, const GLint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform2ivEXT( program, location, count, value );
}

static void WINAPI glProgramUniform2ui( GLuint program, GLint location, GLuint v0, GLuint v1 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", program, location, v0, v1 );
  funcs->ext.p_glProgramUniform2ui( program, location, v0, v1 );
}

static void WINAPI glProgramUniform2ui64ARB( GLuint program, GLint location, GLuint64 x, GLuint64 y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %s, %s)\n", program, location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y) );
  funcs->ext.p_glProgramUniform2ui64ARB( program, location, x, y );
}

static void WINAPI glProgramUniform2ui64NV( GLuint program, GLint location, GLuint64EXT x, GLuint64EXT y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %s, %s)\n", program, location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y) );
  funcs->ext.p_glProgramUniform2ui64NV( program, location, x, y );
}

static void WINAPI glProgramUniform2ui64vARB( GLuint program, GLint location, GLsizei count, const GLuint64 *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform2ui64vARB( program, location, count, value );
}

static void WINAPI glProgramUniform2ui64vNV( GLuint program, GLint location, GLsizei count, const GLuint64EXT *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform2ui64vNV( program, location, count, value );
}

static void WINAPI glProgramUniform2uiEXT( GLuint program, GLint location, GLuint v0, GLuint v1 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", program, location, v0, v1 );
  funcs->ext.p_glProgramUniform2uiEXT( program, location, v0, v1 );
}

static void WINAPI glProgramUniform2uiv( GLuint program, GLint location, GLsizei count, const GLuint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform2uiv( program, location, count, value );
}

static void WINAPI glProgramUniform2uivEXT( GLuint program, GLint location, GLsizei count, const GLuint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform2uivEXT( program, location, count, value );
}

static void WINAPI glProgramUniform3d( GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %f, %f, %f)\n", program, location, v0, v1, v2 );
  funcs->ext.p_glProgramUniform3d( program, location, v0, v1, v2 );
}

static void WINAPI glProgramUniform3dEXT( GLuint program, GLint location, GLdouble x, GLdouble y, GLdouble z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %f, %f, %f)\n", program, location, x, y, z );
  funcs->ext.p_glProgramUniform3dEXT( program, location, x, y, z );
}

static void WINAPI glProgramUniform3dv( GLuint program, GLint location, GLsizei count, const GLdouble *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform3dv( program, location, count, value );
}

static void WINAPI glProgramUniform3dvEXT( GLuint program, GLint location, GLsizei count, const GLdouble *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform3dvEXT( program, location, count, value );
}

static void WINAPI glProgramUniform3f( GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %f, %f, %f)\n", program, location, v0, v1, v2 );
  funcs->ext.p_glProgramUniform3f( program, location, v0, v1, v2 );
}

static void WINAPI glProgramUniform3fEXT( GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %f, %f, %f)\n", program, location, v0, v1, v2 );
  funcs->ext.p_glProgramUniform3fEXT( program, location, v0, v1, v2 );
}

static void WINAPI glProgramUniform3fv( GLuint program, GLint location, GLsizei count, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform3fv( program, location, count, value );
}

static void WINAPI glProgramUniform3fvEXT( GLuint program, GLint location, GLsizei count, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform3fvEXT( program, location, count, value );
}

static void WINAPI glProgramUniform3i( GLuint program, GLint location, GLint v0, GLint v1, GLint v2 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", program, location, v0, v1, v2 );
  funcs->ext.p_glProgramUniform3i( program, location, v0, v1, v2 );
}

static void WINAPI glProgramUniform3i64ARB( GLuint program, GLint location, GLint64 x, GLint64 y, GLint64 z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %s, %s, %s)\n", program, location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z) );
  funcs->ext.p_glProgramUniform3i64ARB( program, location, x, y, z );
}

static void WINAPI glProgramUniform3i64NV( GLuint program, GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %s, %s, %s)\n", program, location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z) );
  funcs->ext.p_glProgramUniform3i64NV( program, location, x, y, z );
}

static void WINAPI glProgramUniform3i64vARB( GLuint program, GLint location, GLsizei count, const GLint64 *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform3i64vARB( program, location, count, value );
}

static void WINAPI glProgramUniform3i64vNV( GLuint program, GLint location, GLsizei count, const GLint64EXT *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform3i64vNV( program, location, count, value );
}

static void WINAPI glProgramUniform3iEXT( GLuint program, GLint location, GLint v0, GLint v1, GLint v2 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", program, location, v0, v1, v2 );
  funcs->ext.p_glProgramUniform3iEXT( program, location, v0, v1, v2 );
}

static void WINAPI glProgramUniform3iv( GLuint program, GLint location, GLsizei count, const GLint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform3iv( program, location, count, value );
}

static void WINAPI glProgramUniform3ivEXT( GLuint program, GLint location, GLsizei count, const GLint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform3ivEXT( program, location, count, value );
}

static void WINAPI glProgramUniform3ui( GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", program, location, v0, v1, v2 );
  funcs->ext.p_glProgramUniform3ui( program, location, v0, v1, v2 );
}

static void WINAPI glProgramUniform3ui64ARB( GLuint program, GLint location, GLuint64 x, GLuint64 y, GLuint64 z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %s, %s, %s)\n", program, location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z) );
  funcs->ext.p_glProgramUniform3ui64ARB( program, location, x, y, z );
}

static void WINAPI glProgramUniform3ui64NV( GLuint program, GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %s, %s, %s)\n", program, location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z) );
  funcs->ext.p_glProgramUniform3ui64NV( program, location, x, y, z );
}

static void WINAPI glProgramUniform3ui64vARB( GLuint program, GLint location, GLsizei count, const GLuint64 *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform3ui64vARB( program, location, count, value );
}

static void WINAPI glProgramUniform3ui64vNV( GLuint program, GLint location, GLsizei count, const GLuint64EXT *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform3ui64vNV( program, location, count, value );
}

static void WINAPI glProgramUniform3uiEXT( GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", program, location, v0, v1, v2 );
  funcs->ext.p_glProgramUniform3uiEXT( program, location, v0, v1, v2 );
}

static void WINAPI glProgramUniform3uiv( GLuint program, GLint location, GLsizei count, const GLuint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform3uiv( program, location, count, value );
}

static void WINAPI glProgramUniform3uivEXT( GLuint program, GLint location, GLsizei count, const GLuint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform3uivEXT( program, location, count, value );
}

static void WINAPI glProgramUniform4d( GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2, GLdouble v3 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %f, %f, %f, %f)\n", program, location, v0, v1, v2, v3 );
  funcs->ext.p_glProgramUniform4d( program, location, v0, v1, v2, v3 );
}

static void WINAPI glProgramUniform4dEXT( GLuint program, GLint location, GLdouble x, GLdouble y, GLdouble z, GLdouble w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %f, %f, %f, %f)\n", program, location, x, y, z, w );
  funcs->ext.p_glProgramUniform4dEXT( program, location, x, y, z, w );
}

static void WINAPI glProgramUniform4dv( GLuint program, GLint location, GLsizei count, const GLdouble *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform4dv( program, location, count, value );
}

static void WINAPI glProgramUniform4dvEXT( GLuint program, GLint location, GLsizei count, const GLdouble *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform4dvEXT( program, location, count, value );
}

static void WINAPI glProgramUniform4f( GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %f, %f, %f, %f)\n", program, location, v0, v1, v2, v3 );
  funcs->ext.p_glProgramUniform4f( program, location, v0, v1, v2, v3 );
}

static void WINAPI glProgramUniform4fEXT( GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %f, %f, %f, %f)\n", program, location, v0, v1, v2, v3 );
  funcs->ext.p_glProgramUniform4fEXT( program, location, v0, v1, v2, v3 );
}

static void WINAPI glProgramUniform4fv( GLuint program, GLint location, GLsizei count, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform4fv( program, location, count, value );
}

static void WINAPI glProgramUniform4fvEXT( GLuint program, GLint location, GLsizei count, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform4fvEXT( program, location, count, value );
}

static void WINAPI glProgramUniform4i( GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d)\n", program, location, v0, v1, v2, v3 );
  funcs->ext.p_glProgramUniform4i( program, location, v0, v1, v2, v3 );
}

static void WINAPI glProgramUniform4i64ARB( GLuint program, GLint location, GLint64 x, GLint64 y, GLint64 z, GLint64 w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %s, %s, %s, %s)\n", program, location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z), wine_dbgstr_longlong(w) );
  funcs->ext.p_glProgramUniform4i64ARB( program, location, x, y, z, w );
}

static void WINAPI glProgramUniform4i64NV( GLuint program, GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z, GLint64EXT w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %s, %s, %s, %s)\n", program, location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z), wine_dbgstr_longlong(w) );
  funcs->ext.p_glProgramUniform4i64NV( program, location, x, y, z, w );
}

static void WINAPI glProgramUniform4i64vARB( GLuint program, GLint location, GLsizei count, const GLint64 *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform4i64vARB( program, location, count, value );
}

static void WINAPI glProgramUniform4i64vNV( GLuint program, GLint location, GLsizei count, const GLint64EXT *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform4i64vNV( program, location, count, value );
}

static void WINAPI glProgramUniform4iEXT( GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d)\n", program, location, v0, v1, v2, v3 );
  funcs->ext.p_glProgramUniform4iEXT( program, location, v0, v1, v2, v3 );
}

static void WINAPI glProgramUniform4iv( GLuint program, GLint location, GLsizei count, const GLint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform4iv( program, location, count, value );
}

static void WINAPI glProgramUniform4ivEXT( GLuint program, GLint location, GLsizei count, const GLint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform4ivEXT( program, location, count, value );
}

static void WINAPI glProgramUniform4ui( GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d)\n", program, location, v0, v1, v2, v3 );
  funcs->ext.p_glProgramUniform4ui( program, location, v0, v1, v2, v3 );
}

static void WINAPI glProgramUniform4ui64ARB( GLuint program, GLint location, GLuint64 x, GLuint64 y, GLuint64 z, GLuint64 w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %s, %s, %s, %s)\n", program, location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z), wine_dbgstr_longlong(w) );
  funcs->ext.p_glProgramUniform4ui64ARB( program, location, x, y, z, w );
}

static void WINAPI glProgramUniform4ui64NV( GLuint program, GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z, GLuint64EXT w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %s, %s, %s, %s)\n", program, location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z), wine_dbgstr_longlong(w) );
  funcs->ext.p_glProgramUniform4ui64NV( program, location, x, y, z, w );
}

static void WINAPI glProgramUniform4ui64vARB( GLuint program, GLint location, GLsizei count, const GLuint64 *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform4ui64vARB( program, location, count, value );
}

static void WINAPI glProgramUniform4ui64vNV( GLuint program, GLint location, GLsizei count, const GLuint64EXT *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform4ui64vNV( program, location, count, value );
}

static void WINAPI glProgramUniform4uiEXT( GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d)\n", program, location, v0, v1, v2, v3 );
  funcs->ext.p_glProgramUniform4uiEXT( program, location, v0, v1, v2, v3 );
}

static void WINAPI glProgramUniform4uiv( GLuint program, GLint location, GLsizei count, const GLuint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform4uiv( program, location, count, value );
}

static void WINAPI glProgramUniform4uivEXT( GLuint program, GLint location, GLsizei count, const GLuint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniform4uivEXT( program, location, count, value );
}

static void WINAPI glProgramUniformHandleui64ARB( GLuint program, GLint location, GLuint64 value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %s)\n", program, location, wine_dbgstr_longlong(value) );
  funcs->ext.p_glProgramUniformHandleui64ARB( program, location, value );
}

static void WINAPI glProgramUniformHandleui64NV( GLuint program, GLint location, GLuint64 value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %s)\n", program, location, wine_dbgstr_longlong(value) );
  funcs->ext.p_glProgramUniformHandleui64NV( program, location, value );
}

static void WINAPI glProgramUniformHandleui64vARB( GLuint program, GLint location, GLsizei count, const GLuint64 *values )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, values );
  funcs->ext.p_glProgramUniformHandleui64vARB( program, location, count, values );
}

static void WINAPI glProgramUniformHandleui64vNV( GLuint program, GLint location, GLsizei count, const GLuint64 *values )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, values );
  funcs->ext.p_glProgramUniformHandleui64vNV( program, location, count, values );
}

static void WINAPI glProgramUniformMatrix2dv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix2dv( program, location, count, transpose, value );
}

static void WINAPI glProgramUniformMatrix2dvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix2dvEXT( program, location, count, transpose, value );
}

static void WINAPI glProgramUniformMatrix2fv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix2fv( program, location, count, transpose, value );
}

static void WINAPI glProgramUniformMatrix2fvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix2fvEXT( program, location, count, transpose, value );
}

static void WINAPI glProgramUniformMatrix2x3dv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix2x3dv( program, location, count, transpose, value );
}

static void WINAPI glProgramUniformMatrix2x3dvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix2x3dvEXT( program, location, count, transpose, value );
}

static void WINAPI glProgramUniformMatrix2x3fv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix2x3fv( program, location, count, transpose, value );
}

static void WINAPI glProgramUniformMatrix2x3fvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix2x3fvEXT( program, location, count, transpose, value );
}

static void WINAPI glProgramUniformMatrix2x4dv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix2x4dv( program, location, count, transpose, value );
}

static void WINAPI glProgramUniformMatrix2x4dvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix2x4dvEXT( program, location, count, transpose, value );
}

static void WINAPI glProgramUniformMatrix2x4fv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix2x4fv( program, location, count, transpose, value );
}

static void WINAPI glProgramUniformMatrix2x4fvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix2x4fvEXT( program, location, count, transpose, value );
}

static void WINAPI glProgramUniformMatrix3dv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix3dv( program, location, count, transpose, value );
}

static void WINAPI glProgramUniformMatrix3dvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix3dvEXT( program, location, count, transpose, value );
}

static void WINAPI glProgramUniformMatrix3fv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix3fv( program, location, count, transpose, value );
}

static void WINAPI glProgramUniformMatrix3fvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix3fvEXT( program, location, count, transpose, value );
}

static void WINAPI glProgramUniformMatrix3x2dv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix3x2dv( program, location, count, transpose, value );
}

static void WINAPI glProgramUniformMatrix3x2dvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix3x2dvEXT( program, location, count, transpose, value );
}

static void WINAPI glProgramUniformMatrix3x2fv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix3x2fv( program, location, count, transpose, value );
}

static void WINAPI glProgramUniformMatrix3x2fvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix3x2fvEXT( program, location, count, transpose, value );
}

static void WINAPI glProgramUniformMatrix3x4dv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix3x4dv( program, location, count, transpose, value );
}

static void WINAPI glProgramUniformMatrix3x4dvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix3x4dvEXT( program, location, count, transpose, value );
}

static void WINAPI glProgramUniformMatrix3x4fv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix3x4fv( program, location, count, transpose, value );
}

static void WINAPI glProgramUniformMatrix3x4fvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix3x4fvEXT( program, location, count, transpose, value );
}

static void WINAPI glProgramUniformMatrix4dv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix4dv( program, location, count, transpose, value );
}

static void WINAPI glProgramUniformMatrix4dvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix4dvEXT( program, location, count, transpose, value );
}

static void WINAPI glProgramUniformMatrix4fv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix4fv( program, location, count, transpose, value );
}

static void WINAPI glProgramUniformMatrix4fvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix4fvEXT( program, location, count, transpose, value );
}

static void WINAPI glProgramUniformMatrix4x2dv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix4x2dv( program, location, count, transpose, value );
}

static void WINAPI glProgramUniformMatrix4x2dvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix4x2dvEXT( program, location, count, transpose, value );
}

static void WINAPI glProgramUniformMatrix4x2fv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix4x2fv( program, location, count, transpose, value );
}

static void WINAPI glProgramUniformMatrix4x2fvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix4x2fvEXT( program, location, count, transpose, value );
}

static void WINAPI glProgramUniformMatrix4x3dv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix4x3dv( program, location, count, transpose, value );
}

static void WINAPI glProgramUniformMatrix4x3dvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix4x3dvEXT( program, location, count, transpose, value );
}

static void WINAPI glProgramUniformMatrix4x3fv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix4x3fv( program, location, count, transpose, value );
}

static void WINAPI glProgramUniformMatrix4x3fvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  funcs->ext.p_glProgramUniformMatrix4x3fvEXT( program, location, count, transpose, value );
}

static void WINAPI glProgramUniformui64NV( GLuint program, GLint location, GLuint64EXT value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %s)\n", program, location, wine_dbgstr_longlong(value) );
  funcs->ext.p_glProgramUniformui64NV( program, location, value );
}

static void WINAPI glProgramUniformui64vNV( GLuint program, GLint location, GLsizei count, const GLuint64EXT *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", program, location, count, value );
  funcs->ext.p_glProgramUniformui64vNV( program, location, count, value );
}

static void WINAPI glProgramVertexLimitNV( GLenum target, GLint limit )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", target, limit );
  funcs->ext.p_glProgramVertexLimitNV( target, limit );
}

static void WINAPI glProvokingVertex( GLenum mode )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", mode );
  funcs->ext.p_glProvokingVertex( mode );
}

static void WINAPI glProvokingVertexEXT( GLenum mode )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", mode );
  funcs->ext.p_glProvokingVertexEXT( mode );
}

static void WINAPI glPushClientAttribDefaultEXT( GLbitfield mask )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", mask );
  funcs->ext.p_glPushClientAttribDefaultEXT( mask );
}

static void WINAPI glPushDebugGroup( GLenum source, GLuint id, GLsizei length, const GLchar *message )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", source, id, length, message );
  funcs->ext.p_glPushDebugGroup( source, id, length, message );
}

static void WINAPI glPushGroupMarkerEXT( GLsizei length, const GLchar *marker )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", length, marker );
  funcs->ext.p_glPushGroupMarkerEXT( length, marker );
}

static void WINAPI glQueryCounter( GLuint id, GLenum target )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", id, target );
  funcs->ext.p_glQueryCounter( id, target );
}

static GLbitfield WINAPI glQueryMatrixxOES( GLfixed *mantissa, GLint *exponent )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p, %p)\n", mantissa, exponent );
  return funcs->ext.p_glQueryMatrixxOES( mantissa, exponent );
}

static void WINAPI glQueryObjectParameteruiAMD( GLenum target, GLuint id, GLenum pname, GLuint param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", target, id, pname, param );
  funcs->ext.p_glQueryObjectParameteruiAMD( target, id, pname, param );
}

static GLint WINAPI glQueryResourceNV( GLenum queryType, GLint tagId, GLuint bufSize, GLint *buffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", queryType, tagId, bufSize, buffer );
  return funcs->ext.p_glQueryResourceNV( queryType, tagId, bufSize, buffer );
}

static void WINAPI glQueryResourceTagNV( GLint tagId, const GLchar *tagString )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", tagId, tagString );
  funcs->ext.p_glQueryResourceTagNV( tagId, tagString );
}

static void WINAPI glRasterPos2xOES( GLfixed x, GLfixed y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", x, y );
  funcs->ext.p_glRasterPos2xOES( x, y );
}

static void WINAPI glRasterPos2xvOES( const GLfixed *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", coords );
  funcs->ext.p_glRasterPos2xvOES( coords );
}

static void WINAPI glRasterPos3xOES( GLfixed x, GLfixed y, GLfixed z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", x, y, z );
  funcs->ext.p_glRasterPos3xOES( x, y, z );
}

static void WINAPI glRasterPos3xvOES( const GLfixed *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", coords );
  funcs->ext.p_glRasterPos3xvOES( coords );
}

static void WINAPI glRasterPos4xOES( GLfixed x, GLfixed y, GLfixed z, GLfixed w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", x, y, z, w );
  funcs->ext.p_glRasterPos4xOES( x, y, z, w );
}

static void WINAPI glRasterPos4xvOES( const GLfixed *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", coords );
  funcs->ext.p_glRasterPos4xvOES( coords );
}

static void WINAPI glRasterSamplesEXT( GLuint samples, GLboolean fixedsamplelocations )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", samples, fixedsamplelocations );
  funcs->ext.p_glRasterSamplesEXT( samples, fixedsamplelocations );
}

static void WINAPI glReadBufferRegion( GLenum region, GLint x, GLint y, GLsizei width, GLsizei height )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", region, x, y, width, height );
  funcs->ext.p_glReadBufferRegion( region, x, y, width, height );
}

static void WINAPI glReadInstrumentsSGIX( GLint marker )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", marker );
  funcs->ext.p_glReadInstrumentsSGIX( marker );
}

static void WINAPI glReadnPixels( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, void *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %p)\n", x, y, width, height, format, type, bufSize, data );
  funcs->ext.p_glReadnPixels( x, y, width, height, format, type, bufSize, data );
}

static void WINAPI glReadnPixelsARB( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, void *data )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %p)\n", x, y, width, height, format, type, bufSize, data );
  funcs->ext.p_glReadnPixelsARB( x, y, width, height, format, type, bufSize, data );
}

static void WINAPI glRectxOES( GLfixed x1, GLfixed y1, GLfixed x2, GLfixed y2 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", x1, y1, x2, y2 );
  funcs->ext.p_glRectxOES( x1, y1, x2, y2 );
}

static void WINAPI glRectxvOES( const GLfixed *v1, const GLfixed *v2 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p, %p)\n", v1, v2 );
  funcs->ext.p_glRectxvOES( v1, v2 );
}

static void WINAPI glReferencePlaneSGIX( const GLdouble *equation )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", equation );
  funcs->ext.p_glReferencePlaneSGIX( equation );
}

static GLboolean WINAPI glReleaseKeyedMutexWin32EXT( GLuint memory, GLuint64 key )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %s)\n", memory, wine_dbgstr_longlong(key) );
  return funcs->ext.p_glReleaseKeyedMutexWin32EXT( memory, key );
}

static void WINAPI glReleaseShaderCompiler(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  funcs->ext.p_glReleaseShaderCompiler();
}

static void WINAPI glRenderGpuMaskNV( GLbitfield mask )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", mask );
  funcs->ext.p_glRenderGpuMaskNV( mask );
}

static void WINAPI glRenderbufferStorage( GLenum target, GLenum internalformat, GLsizei width, GLsizei height )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", target, internalformat, width, height );
  funcs->ext.p_glRenderbufferStorage( target, internalformat, width, height );
}

static void WINAPI glRenderbufferStorageEXT( GLenum target, GLenum internalformat, GLsizei width, GLsizei height )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", target, internalformat, width, height );
  funcs->ext.p_glRenderbufferStorageEXT( target, internalformat, width, height );
}

static void WINAPI glRenderbufferStorageMultisample( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", target, samples, internalformat, width, height );
  funcs->ext.p_glRenderbufferStorageMultisample( target, samples, internalformat, width, height );
}

static void WINAPI glRenderbufferStorageMultisampleCoverageNV( GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLenum internalformat, GLsizei width, GLsizei height )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d)\n", target, coverageSamples, colorSamples, internalformat, width, height );
  funcs->ext.p_glRenderbufferStorageMultisampleCoverageNV( target, coverageSamples, colorSamples, internalformat, width, height );
}

static void WINAPI glRenderbufferStorageMultisampleEXT( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", target, samples, internalformat, width, height );
  funcs->ext.p_glRenderbufferStorageMultisampleEXT( target, samples, internalformat, width, height );
}

static void WINAPI glReplacementCodePointerSUN( GLenum type, GLsizei stride, const void **pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", type, stride, pointer );
  funcs->ext.p_glReplacementCodePointerSUN( type, stride, pointer );
}

static void WINAPI glReplacementCodeubSUN( GLubyte code )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", code );
  funcs->ext.p_glReplacementCodeubSUN( code );
}

static void WINAPI glReplacementCodeubvSUN( const GLubyte *code )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", code );
  funcs->ext.p_glReplacementCodeubvSUN( code );
}

static void WINAPI glReplacementCodeuiColor3fVertex3fSUN( GLuint rc, GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f, %f, %f, %f)\n", rc, r, g, b, x, y, z );
  funcs->ext.p_glReplacementCodeuiColor3fVertex3fSUN( rc, r, g, b, x, y, z );
}

static void WINAPI glReplacementCodeuiColor3fVertex3fvSUN( const GLuint *rc, const GLfloat *c, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p, %p, %p)\n", rc, c, v );
  funcs->ext.p_glReplacementCodeuiColor3fVertex3fvSUN( rc, c, v );
}

static void WINAPI glReplacementCodeuiColor4fNormal3fVertex3fSUN( GLuint rc, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f)\n", rc, r, g, b, a, nx, ny, nz, x, y, z );
  funcs->ext.p_glReplacementCodeuiColor4fNormal3fVertex3fSUN( rc, r, g, b, a, nx, ny, nz, x, y, z );
}

static void WINAPI glReplacementCodeuiColor4fNormal3fVertex3fvSUN( const GLuint *rc, const GLfloat *c, const GLfloat *n, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p, %p, %p, %p)\n", rc, c, n, v );
  funcs->ext.p_glReplacementCodeuiColor4fNormal3fVertex3fvSUN( rc, c, n, v );
}

static void WINAPI glReplacementCodeuiColor4ubVertex3fSUN( GLuint rc, GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %f, %f, %f)\n", rc, r, g, b, a, x, y, z );
  funcs->ext.p_glReplacementCodeuiColor4ubVertex3fSUN( rc, r, g, b, a, x, y, z );
}

static void WINAPI glReplacementCodeuiColor4ubVertex3fvSUN( const GLuint *rc, const GLubyte *c, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p, %p, %p)\n", rc, c, v );
  funcs->ext.p_glReplacementCodeuiColor4ubVertex3fvSUN( rc, c, v );
}

static void WINAPI glReplacementCodeuiNormal3fVertex3fSUN( GLuint rc, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f, %f, %f, %f)\n", rc, nx, ny, nz, x, y, z );
  funcs->ext.p_glReplacementCodeuiNormal3fVertex3fSUN( rc, nx, ny, nz, x, y, z );
}

static void WINAPI glReplacementCodeuiNormal3fVertex3fvSUN( const GLuint *rc, const GLfloat *n, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p, %p, %p)\n", rc, n, v );
  funcs->ext.p_glReplacementCodeuiNormal3fVertex3fvSUN( rc, n, v );
}

static void WINAPI glReplacementCodeuiSUN( GLuint code )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", code );
  funcs->ext.p_glReplacementCodeuiSUN( code );
}

static void WINAPI glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN( GLuint rc, GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f)\n", rc, s, t, r, g, b, a, nx, ny, nz, x, y, z );
  funcs->ext.p_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN( rc, s, t, r, g, b, a, nx, ny, nz, x, y, z );
}

static void WINAPI glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN( const GLuint *rc, const GLfloat *tc, const GLfloat *c, const GLfloat *n, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p, %p, %p, %p, %p)\n", rc, tc, c, n, v );
  funcs->ext.p_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN( rc, tc, c, n, v );
}

static void WINAPI glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN( GLuint rc, GLfloat s, GLfloat t, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f, %f, %f, %f, %f, %f)\n", rc, s, t, nx, ny, nz, x, y, z );
  funcs->ext.p_glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN( rc, s, t, nx, ny, nz, x, y, z );
}

static void WINAPI glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN( const GLuint *rc, const GLfloat *tc, const GLfloat *n, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p, %p, %p, %p)\n", rc, tc, n, v );
  funcs->ext.p_glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN( rc, tc, n, v );
}

static void WINAPI glReplacementCodeuiTexCoord2fVertex3fSUN( GLuint rc, GLfloat s, GLfloat t, GLfloat x, GLfloat y, GLfloat z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f, %f, %f)\n", rc, s, t, x, y, z );
  funcs->ext.p_glReplacementCodeuiTexCoord2fVertex3fSUN( rc, s, t, x, y, z );
}

static void WINAPI glReplacementCodeuiTexCoord2fVertex3fvSUN( const GLuint *rc, const GLfloat *tc, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p, %p, %p)\n", rc, tc, v );
  funcs->ext.p_glReplacementCodeuiTexCoord2fVertex3fvSUN( rc, tc, v );
}

static void WINAPI glReplacementCodeuiVertex3fSUN( GLuint rc, GLfloat x, GLfloat y, GLfloat z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f)\n", rc, x, y, z );
  funcs->ext.p_glReplacementCodeuiVertex3fSUN( rc, x, y, z );
}

static void WINAPI glReplacementCodeuiVertex3fvSUN( const GLuint *rc, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p, %p)\n", rc, v );
  funcs->ext.p_glReplacementCodeuiVertex3fvSUN( rc, v );
}

static void WINAPI glReplacementCodeuivSUN( const GLuint *code )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", code );
  funcs->ext.p_glReplacementCodeuivSUN( code );
}

static void WINAPI glReplacementCodeusSUN( GLushort code )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", code );
  funcs->ext.p_glReplacementCodeusSUN( code );
}

static void WINAPI glReplacementCodeusvSUN( const GLushort *code )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", code );
  funcs->ext.p_glReplacementCodeusvSUN( code );
}

static void WINAPI glRequestResidentProgramsNV( GLsizei n, const GLuint *programs )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", n, programs );
  funcs->ext.p_glRequestResidentProgramsNV( n, programs );
}

static void WINAPI glResetHistogram( GLenum target )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", target );
  funcs->ext.p_glResetHistogram( target );
}

static void WINAPI glResetHistogramEXT( GLenum target )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", target );
  funcs->ext.p_glResetHistogramEXT( target );
}

static void WINAPI glResetMinmax( GLenum target )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", target );
  funcs->ext.p_glResetMinmax( target );
}

static void WINAPI glResetMinmaxEXT( GLenum target )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", target );
  funcs->ext.p_glResetMinmaxEXT( target );
}

static void WINAPI glResizeBuffersMESA(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  funcs->ext.p_glResizeBuffersMESA();
}

static void WINAPI glResolveDepthValuesNV(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  funcs->ext.p_glResolveDepthValuesNV();
}

static void WINAPI glResumeTransformFeedback(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  funcs->ext.p_glResumeTransformFeedback();
}

static void WINAPI glResumeTransformFeedbackNV(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  funcs->ext.p_glResumeTransformFeedbackNV();
}

static void WINAPI glRotatexOES( GLfixed angle, GLfixed x, GLfixed y, GLfixed z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", angle, x, y, z );
  funcs->ext.p_glRotatexOES( angle, x, y, z );
}

static void WINAPI glSampleCoverage( GLfloat value, GLboolean invert )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %d)\n", value, invert );
  funcs->ext.p_glSampleCoverage( value, invert );
}

static void WINAPI glSampleCoverageARB( GLfloat value, GLboolean invert )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %d)\n", value, invert );
  funcs->ext.p_glSampleCoverageARB( value, invert );
}

static void WINAPI glSampleMapATI( GLuint dst, GLuint interp, GLenum swizzle )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", dst, interp, swizzle );
  funcs->ext.p_glSampleMapATI( dst, interp, swizzle );
}

static void WINAPI glSampleMaskEXT( GLclampf value, GLboolean invert )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %d)\n", value, invert );
  funcs->ext.p_glSampleMaskEXT( value, invert );
}

static void WINAPI glSampleMaskIndexedNV( GLuint index, GLbitfield mask )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", index, mask );
  funcs->ext.p_glSampleMaskIndexedNV( index, mask );
}

static void WINAPI glSampleMaskSGIS( GLclampf value, GLboolean invert )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %d)\n", value, invert );
  funcs->ext.p_glSampleMaskSGIS( value, invert );
}

static void WINAPI glSampleMaski( GLuint maskNumber, GLbitfield mask )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", maskNumber, mask );
  funcs->ext.p_glSampleMaski( maskNumber, mask );
}

static void WINAPI glSamplePatternEXT( GLenum pattern )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", pattern );
  funcs->ext.p_glSamplePatternEXT( pattern );
}

static void WINAPI glSamplePatternSGIS( GLenum pattern )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", pattern );
  funcs->ext.p_glSamplePatternSGIS( pattern );
}

static void WINAPI glSamplerParameterIiv( GLuint sampler, GLenum pname, const GLint *param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", sampler, pname, param );
  funcs->ext.p_glSamplerParameterIiv( sampler, pname, param );
}

static void WINAPI glSamplerParameterIuiv( GLuint sampler, GLenum pname, const GLuint *param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", sampler, pname, param );
  funcs->ext.p_glSamplerParameterIuiv( sampler, pname, param );
}

static void WINAPI glSamplerParameterf( GLuint sampler, GLenum pname, GLfloat param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %f)\n", sampler, pname, param );
  funcs->ext.p_glSamplerParameterf( sampler, pname, param );
}

static void WINAPI glSamplerParameterfv( GLuint sampler, GLenum pname, const GLfloat *param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", sampler, pname, param );
  funcs->ext.p_glSamplerParameterfv( sampler, pname, param );
}

static void WINAPI glSamplerParameteri( GLuint sampler, GLenum pname, GLint param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", sampler, pname, param );
  funcs->ext.p_glSamplerParameteri( sampler, pname, param );
}

static void WINAPI glSamplerParameteriv( GLuint sampler, GLenum pname, const GLint *param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", sampler, pname, param );
  funcs->ext.p_glSamplerParameteriv( sampler, pname, param );
}

static void WINAPI glScalexOES( GLfixed x, GLfixed y, GLfixed z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", x, y, z );
  funcs->ext.p_glScalexOES( x, y, z );
}

static void WINAPI glScissorArrayv( GLuint first, GLsizei count, const GLint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", first, count, v );
  funcs->ext.p_glScissorArrayv( first, count, v );
}

static void WINAPI glScissorIndexed( GLuint index, GLint left, GLint bottom, GLsizei width, GLsizei height )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", index, left, bottom, width, height );
  funcs->ext.p_glScissorIndexed( index, left, bottom, width, height );
}

static void WINAPI glScissorIndexedv( GLuint index, const GLint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glScissorIndexedv( index, v );
}

static void WINAPI glSecondaryColor3b( GLbyte red, GLbyte green, GLbyte blue )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", red, green, blue );
  funcs->ext.p_glSecondaryColor3b( red, green, blue );
}

static void WINAPI glSecondaryColor3bEXT( GLbyte red, GLbyte green, GLbyte blue )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", red, green, blue );
  funcs->ext.p_glSecondaryColor3bEXT( red, green, blue );
}

static void WINAPI glSecondaryColor3bv( const GLbyte *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glSecondaryColor3bv( v );
}

static void WINAPI glSecondaryColor3bvEXT( const GLbyte *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glSecondaryColor3bvEXT( v );
}

static void WINAPI glSecondaryColor3d( GLdouble red, GLdouble green, GLdouble blue )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f, %f)\n", red, green, blue );
  funcs->ext.p_glSecondaryColor3d( red, green, blue );
}

static void WINAPI glSecondaryColor3dEXT( GLdouble red, GLdouble green, GLdouble blue )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f, %f)\n", red, green, blue );
  funcs->ext.p_glSecondaryColor3dEXT( red, green, blue );
}

static void WINAPI glSecondaryColor3dv( const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glSecondaryColor3dv( v );
}

static void WINAPI glSecondaryColor3dvEXT( const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glSecondaryColor3dvEXT( v );
}

static void WINAPI glSecondaryColor3f( GLfloat red, GLfloat green, GLfloat blue )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f, %f)\n", red, green, blue );
  funcs->ext.p_glSecondaryColor3f( red, green, blue );
}

static void WINAPI glSecondaryColor3fEXT( GLfloat red, GLfloat green, GLfloat blue )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f, %f)\n", red, green, blue );
  funcs->ext.p_glSecondaryColor3fEXT( red, green, blue );
}

static void WINAPI glSecondaryColor3fv( const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glSecondaryColor3fv( v );
}

static void WINAPI glSecondaryColor3fvEXT( const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glSecondaryColor3fvEXT( v );
}

static void WINAPI glSecondaryColor3hNV( GLhalfNV red, GLhalfNV green, GLhalfNV blue )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", red, green, blue );
  funcs->ext.p_glSecondaryColor3hNV( red, green, blue );
}

static void WINAPI glSecondaryColor3hvNV( const GLhalfNV *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glSecondaryColor3hvNV( v );
}

static void WINAPI glSecondaryColor3i( GLint red, GLint green, GLint blue )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", red, green, blue );
  funcs->ext.p_glSecondaryColor3i( red, green, blue );
}

static void WINAPI glSecondaryColor3iEXT( GLint red, GLint green, GLint blue )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", red, green, blue );
  funcs->ext.p_glSecondaryColor3iEXT( red, green, blue );
}

static void WINAPI glSecondaryColor3iv( const GLint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glSecondaryColor3iv( v );
}

static void WINAPI glSecondaryColor3ivEXT( const GLint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glSecondaryColor3ivEXT( v );
}

static void WINAPI glSecondaryColor3s( GLshort red, GLshort green, GLshort blue )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", red, green, blue );
  funcs->ext.p_glSecondaryColor3s( red, green, blue );
}

static void WINAPI glSecondaryColor3sEXT( GLshort red, GLshort green, GLshort blue )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", red, green, blue );
  funcs->ext.p_glSecondaryColor3sEXT( red, green, blue );
}

static void WINAPI glSecondaryColor3sv( const GLshort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glSecondaryColor3sv( v );
}

static void WINAPI glSecondaryColor3svEXT( const GLshort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glSecondaryColor3svEXT( v );
}

static void WINAPI glSecondaryColor3ub( GLubyte red, GLubyte green, GLubyte blue )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", red, green, blue );
  funcs->ext.p_glSecondaryColor3ub( red, green, blue );
}

static void WINAPI glSecondaryColor3ubEXT( GLubyte red, GLubyte green, GLubyte blue )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", red, green, blue );
  funcs->ext.p_glSecondaryColor3ubEXT( red, green, blue );
}

static void WINAPI glSecondaryColor3ubv( const GLubyte *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glSecondaryColor3ubv( v );
}

static void WINAPI glSecondaryColor3ubvEXT( const GLubyte *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glSecondaryColor3ubvEXT( v );
}

static void WINAPI glSecondaryColor3ui( GLuint red, GLuint green, GLuint blue )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", red, green, blue );
  funcs->ext.p_glSecondaryColor3ui( red, green, blue );
}

static void WINAPI glSecondaryColor3uiEXT( GLuint red, GLuint green, GLuint blue )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", red, green, blue );
  funcs->ext.p_glSecondaryColor3uiEXT( red, green, blue );
}

static void WINAPI glSecondaryColor3uiv( const GLuint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glSecondaryColor3uiv( v );
}

static void WINAPI glSecondaryColor3uivEXT( const GLuint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glSecondaryColor3uivEXT( v );
}

static void WINAPI glSecondaryColor3us( GLushort red, GLushort green, GLushort blue )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", red, green, blue );
  funcs->ext.p_glSecondaryColor3us( red, green, blue );
}

static void WINAPI glSecondaryColor3usEXT( GLushort red, GLushort green, GLushort blue )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", red, green, blue );
  funcs->ext.p_glSecondaryColor3usEXT( red, green, blue );
}

static void WINAPI glSecondaryColor3usv( const GLushort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glSecondaryColor3usv( v );
}

static void WINAPI glSecondaryColor3usvEXT( const GLushort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glSecondaryColor3usvEXT( v );
}

static void WINAPI glSecondaryColorFormatNV( GLint size, GLenum type, GLsizei stride )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", size, type, stride );
  funcs->ext.p_glSecondaryColorFormatNV( size, type, stride );
}

static void WINAPI glSecondaryColorP3ui( GLenum type, GLuint color )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", type, color );
  funcs->ext.p_glSecondaryColorP3ui( type, color );
}

static void WINAPI glSecondaryColorP3uiv( GLenum type, const GLuint *color )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", type, color );
  funcs->ext.p_glSecondaryColorP3uiv( type, color );
}

static void WINAPI glSecondaryColorPointer( GLint size, GLenum type, GLsizei stride, const void *pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", size, type, stride, pointer );
  funcs->ext.p_glSecondaryColorPointer( size, type, stride, pointer );
}

static void WINAPI glSecondaryColorPointerEXT( GLint size, GLenum type, GLsizei stride, const void *pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", size, type, stride, pointer );
  funcs->ext.p_glSecondaryColorPointerEXT( size, type, stride, pointer );
}

static void WINAPI glSecondaryColorPointerListIBM( GLint size, GLenum type, GLint stride, const void **pointer, GLint ptrstride )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p, %d)\n", size, type, stride, pointer, ptrstride );
  funcs->ext.p_glSecondaryColorPointerListIBM( size, type, stride, pointer, ptrstride );
}

static void WINAPI glSelectPerfMonitorCountersAMD( GLuint monitor, GLboolean enable, GLuint group, GLint numCounters, GLuint *counterList )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", monitor, enable, group, numCounters, counterList );
  funcs->ext.p_glSelectPerfMonitorCountersAMD( monitor, enable, group, numCounters, counterList );
}

static void WINAPI glSelectTextureCoordSetSGIS( GLenum target )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", target );
  funcs->ext.p_glSelectTextureCoordSetSGIS( target );
}

static void WINAPI glSelectTextureSGIS( GLenum target )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", target );
  funcs->ext.p_glSelectTextureSGIS( target );
}

static void WINAPI glSemaphoreParameterui64vEXT( GLuint semaphore, GLenum pname, const GLuint64 *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", semaphore, pname, params );
  funcs->ext.p_glSemaphoreParameterui64vEXT( semaphore, pname, params );
}

static void WINAPI glSeparableFilter2D( GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *row, const void *column )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %p, %p)\n", target, internalformat, width, height, format, type, row, column );
  funcs->ext.p_glSeparableFilter2D( target, internalformat, width, height, format, type, row, column );
}

static void WINAPI glSeparableFilter2DEXT( GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *row, const void *column )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %p, %p)\n", target, internalformat, width, height, format, type, row, column );
  funcs->ext.p_glSeparableFilter2DEXT( target, internalformat, width, height, format, type, row, column );
}

static void WINAPI glSetFenceAPPLE( GLuint fence )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", fence );
  funcs->ext.p_glSetFenceAPPLE( fence );
}

static void WINAPI glSetFenceNV( GLuint fence, GLenum condition )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", fence, condition );
  funcs->ext.p_glSetFenceNV( fence, condition );
}

static void WINAPI glSetFragmentShaderConstantATI( GLuint dst, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", dst, value );
  funcs->ext.p_glSetFragmentShaderConstantATI( dst, value );
}

static void WINAPI glSetInvariantEXT( GLuint id, GLenum type, const void *addr )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", id, type, addr );
  funcs->ext.p_glSetInvariantEXT( id, type, addr );
}

static void WINAPI glSetLocalConstantEXT( GLuint id, GLenum type, const void *addr )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", id, type, addr );
  funcs->ext.p_glSetLocalConstantEXT( id, type, addr );
}

static void WINAPI glSetMultisamplefvAMD( GLenum pname, GLuint index, const GLfloat *val )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", pname, index, val );
  funcs->ext.p_glSetMultisamplefvAMD( pname, index, val );
}

static void WINAPI glShaderBinary( GLsizei count, const GLuint *shaders, GLenum binaryformat, const void *binary, GLsizei length )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p, %d, %p, %d)\n", count, shaders, binaryformat, binary, length );
  funcs->ext.p_glShaderBinary( count, shaders, binaryformat, binary, length );
}

static void WINAPI glShaderOp1EXT( GLenum op, GLuint res, GLuint arg1 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", op, res, arg1 );
  funcs->ext.p_glShaderOp1EXT( op, res, arg1 );
}

static void WINAPI glShaderOp2EXT( GLenum op, GLuint res, GLuint arg1, GLuint arg2 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", op, res, arg1, arg2 );
  funcs->ext.p_glShaderOp2EXT( op, res, arg1, arg2 );
}

static void WINAPI glShaderOp3EXT( GLenum op, GLuint res, GLuint arg1, GLuint arg2, GLuint arg3 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", op, res, arg1, arg2, arg3 );
  funcs->ext.p_glShaderOp3EXT( op, res, arg1, arg2, arg3 );
}

static void WINAPI glShaderSource( GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %p)\n", shader, count, string, length );
  funcs->ext.p_glShaderSource( shader, count, string, length );
}

static void WINAPI glShaderSourceARB( GLhandleARB shaderObj, GLsizei count, const GLcharARB **string, const GLint *length )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %p)\n", shaderObj, count, string, length );
  funcs->ext.p_glShaderSourceARB( shaderObj, count, string, length );
}

static void WINAPI glShaderStorageBlockBinding( GLuint program, GLuint storageBlockIndex, GLuint storageBlockBinding )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", program, storageBlockIndex, storageBlockBinding );
  funcs->ext.p_glShaderStorageBlockBinding( program, storageBlockIndex, storageBlockBinding );
}

static void WINAPI glSharpenTexFuncSGIS( GLenum target, GLsizei n, const GLfloat *points )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, n, points );
  funcs->ext.p_glSharpenTexFuncSGIS( target, n, points );
}

static void WINAPI glSignalSemaphoreEXT( GLuint semaphore, GLuint numBufferBarriers, const GLuint *buffers, GLuint numTextureBarriers, const GLuint *textures, const GLenum *dstLayouts )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %d, %p, %p)\n", semaphore, numBufferBarriers, buffers, numTextureBarriers, textures, dstLayouts );
  funcs->ext.p_glSignalSemaphoreEXT( semaphore, numBufferBarriers, buffers, numTextureBarriers, textures, dstLayouts );
}

static void WINAPI glSignalVkFenceNV( GLuint64 vkFence )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%s)\n", wine_dbgstr_longlong(vkFence) );
  funcs->ext.p_glSignalVkFenceNV( vkFence );
}

static void WINAPI glSignalVkSemaphoreNV( GLuint64 vkSemaphore )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%s)\n", wine_dbgstr_longlong(vkSemaphore) );
  funcs->ext.p_glSignalVkSemaphoreNV( vkSemaphore );
}

static void WINAPI glSpecializeShader( GLuint shader, const GLchar *pEntryPoint, GLuint numSpecializationConstants, const GLuint *pConstantIndex, const GLuint *pConstantValue )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p, %d, %p, %p)\n", shader, pEntryPoint, numSpecializationConstants, pConstantIndex, pConstantValue );
  funcs->ext.p_glSpecializeShader( shader, pEntryPoint, numSpecializationConstants, pConstantIndex, pConstantValue );
}

static void WINAPI glSpecializeShaderARB( GLuint shader, const GLchar *pEntryPoint, GLuint numSpecializationConstants, const GLuint *pConstantIndex, const GLuint *pConstantValue )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p, %d, %p, %p)\n", shader, pEntryPoint, numSpecializationConstants, pConstantIndex, pConstantValue );
  funcs->ext.p_glSpecializeShaderARB( shader, pEntryPoint, numSpecializationConstants, pConstantIndex, pConstantValue );
}

static void WINAPI glSpriteParameterfSGIX( GLenum pname, GLfloat param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f)\n", pname, param );
  funcs->ext.p_glSpriteParameterfSGIX( pname, param );
}

static void WINAPI glSpriteParameterfvSGIX( GLenum pname, const GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", pname, params );
  funcs->ext.p_glSpriteParameterfvSGIX( pname, params );
}

static void WINAPI glSpriteParameteriSGIX( GLenum pname, GLint param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", pname, param );
  funcs->ext.p_glSpriteParameteriSGIX( pname, param );
}

static void WINAPI glSpriteParameterivSGIX( GLenum pname, const GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", pname, params );
  funcs->ext.p_glSpriteParameterivSGIX( pname, params );
}

static void WINAPI glStartInstrumentsSGIX(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  funcs->ext.p_glStartInstrumentsSGIX();
}

static void WINAPI glStateCaptureNV( GLuint state, GLenum mode )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", state, mode );
  funcs->ext.p_glStateCaptureNV( state, mode );
}

static void WINAPI glStencilClearTagEXT( GLsizei stencilTagBits, GLuint stencilClearTag )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", stencilTagBits, stencilClearTag );
  funcs->ext.p_glStencilClearTagEXT( stencilTagBits, stencilClearTag );
}

static void WINAPI glStencilFillPathInstancedNV( GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum fillMode, GLuint mask, GLenum transformType, const GLfloat *transformValues )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %d, %d, %d, %d, %p)\n", numPaths, pathNameType, paths, pathBase, fillMode, mask, transformType, transformValues );
  funcs->ext.p_glStencilFillPathInstancedNV( numPaths, pathNameType, paths, pathBase, fillMode, mask, transformType, transformValues );
}

static void WINAPI glStencilFillPathNV( GLuint path, GLenum fillMode, GLuint mask )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", path, fillMode, mask );
  funcs->ext.p_glStencilFillPathNV( path, fillMode, mask );
}

static void WINAPI glStencilFuncSeparate( GLenum face, GLenum func, GLint ref, GLuint mask )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", face, func, ref, mask );
  funcs->ext.p_glStencilFuncSeparate( face, func, ref, mask );
}

static void WINAPI glStencilFuncSeparateATI( GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", frontfunc, backfunc, ref, mask );
  funcs->ext.p_glStencilFuncSeparateATI( frontfunc, backfunc, ref, mask );
}

static void WINAPI glStencilMaskSeparate( GLenum face, GLuint mask )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", face, mask );
  funcs->ext.p_glStencilMaskSeparate( face, mask );
}

static void WINAPI glStencilOpSeparate( GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", face, sfail, dpfail, dppass );
  funcs->ext.p_glStencilOpSeparate( face, sfail, dpfail, dppass );
}

static void WINAPI glStencilOpSeparateATI( GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", face, sfail, dpfail, dppass );
  funcs->ext.p_glStencilOpSeparateATI( face, sfail, dpfail, dppass );
}

static void WINAPI glStencilOpValueAMD( GLenum face, GLuint value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", face, value );
  funcs->ext.p_glStencilOpValueAMD( face, value );
}

static void WINAPI glStencilStrokePathInstancedNV( GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLint reference, GLuint mask, GLenum transformType, const GLfloat *transformValues )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %d, %d, %d, %d, %p)\n", numPaths, pathNameType, paths, pathBase, reference, mask, transformType, transformValues );
  funcs->ext.p_glStencilStrokePathInstancedNV( numPaths, pathNameType, paths, pathBase, reference, mask, transformType, transformValues );
}

static void WINAPI glStencilStrokePathNV( GLuint path, GLint reference, GLuint mask )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", path, reference, mask );
  funcs->ext.p_glStencilStrokePathNV( path, reference, mask );
}

static void WINAPI glStencilThenCoverFillPathInstancedNV( GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum fillMode, GLuint mask, GLenum coverMode, GLenum transformType, const GLfloat *transformValues )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %d, %d, %d, %d, %d, %p)\n", numPaths, pathNameType, paths, pathBase, fillMode, mask, coverMode, transformType, transformValues );
  funcs->ext.p_glStencilThenCoverFillPathInstancedNV( numPaths, pathNameType, paths, pathBase, fillMode, mask, coverMode, transformType, transformValues );
}

static void WINAPI glStencilThenCoverFillPathNV( GLuint path, GLenum fillMode, GLuint mask, GLenum coverMode )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", path, fillMode, mask, coverMode );
  funcs->ext.p_glStencilThenCoverFillPathNV( path, fillMode, mask, coverMode );
}

static void WINAPI glStencilThenCoverStrokePathInstancedNV( GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLint reference, GLuint mask, GLenum coverMode, GLenum transformType, const GLfloat *transformValues )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %d, %d, %d, %d, %d, %p)\n", numPaths, pathNameType, paths, pathBase, reference, mask, coverMode, transformType, transformValues );
  funcs->ext.p_glStencilThenCoverStrokePathInstancedNV( numPaths, pathNameType, paths, pathBase, reference, mask, coverMode, transformType, transformValues );
}

static void WINAPI glStencilThenCoverStrokePathNV( GLuint path, GLint reference, GLuint mask, GLenum coverMode )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", path, reference, mask, coverMode );
  funcs->ext.p_glStencilThenCoverStrokePathNV( path, reference, mask, coverMode );
}

static void WINAPI glStopInstrumentsSGIX( GLint marker )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", marker );
  funcs->ext.p_glStopInstrumentsSGIX( marker );
}

static void WINAPI glStringMarkerGREMEDY( GLsizei len, const void *string )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", len, string );
  funcs->ext.p_glStringMarkerGREMEDY( len, string );
}

static void WINAPI glSubpixelPrecisionBiasNV( GLuint xbits, GLuint ybits )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", xbits, ybits );
  funcs->ext.p_glSubpixelPrecisionBiasNV( xbits, ybits );
}

static void WINAPI glSwizzleEXT( GLuint res, GLuint in, GLenum outX, GLenum outY, GLenum outZ, GLenum outW )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d)\n", res, in, outX, outY, outZ, outW );
  funcs->ext.p_glSwizzleEXT( res, in, outX, outY, outZ, outW );
}

static void WINAPI glSyncTextureINTEL( GLuint texture )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", texture );
  funcs->ext.p_glSyncTextureINTEL( texture );
}

static void WINAPI glTagSampleBufferSGIX(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  funcs->ext.p_glTagSampleBufferSGIX();
}

static void WINAPI glTangent3bEXT( GLbyte tx, GLbyte ty, GLbyte tz )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", tx, ty, tz );
  funcs->ext.p_glTangent3bEXT( tx, ty, tz );
}

static void WINAPI glTangent3bvEXT( const GLbyte *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glTangent3bvEXT( v );
}

static void WINAPI glTangent3dEXT( GLdouble tx, GLdouble ty, GLdouble tz )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f, %f)\n", tx, ty, tz );
  funcs->ext.p_glTangent3dEXT( tx, ty, tz );
}

static void WINAPI glTangent3dvEXT( const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glTangent3dvEXT( v );
}

static void WINAPI glTangent3fEXT( GLfloat tx, GLfloat ty, GLfloat tz )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f, %f)\n", tx, ty, tz );
  funcs->ext.p_glTangent3fEXT( tx, ty, tz );
}

static void WINAPI glTangent3fvEXT( const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glTangent3fvEXT( v );
}

static void WINAPI glTangent3iEXT( GLint tx, GLint ty, GLint tz )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", tx, ty, tz );
  funcs->ext.p_glTangent3iEXT( tx, ty, tz );
}

static void WINAPI glTangent3ivEXT( const GLint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glTangent3ivEXT( v );
}

static void WINAPI glTangent3sEXT( GLshort tx, GLshort ty, GLshort tz )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", tx, ty, tz );
  funcs->ext.p_glTangent3sEXT( tx, ty, tz );
}

static void WINAPI glTangent3svEXT( const GLshort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glTangent3svEXT( v );
}

static void WINAPI glTangentPointerEXT( GLenum type, GLsizei stride, const void *pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", type, stride, pointer );
  funcs->ext.p_glTangentPointerEXT( type, stride, pointer );
}

static void WINAPI glTbufferMask3DFX( GLuint mask )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", mask );
  funcs->ext.p_glTbufferMask3DFX( mask );
}

static void WINAPI glTessellationFactorAMD( GLfloat factor )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f)\n", factor );
  funcs->ext.p_glTessellationFactorAMD( factor );
}

static void WINAPI glTessellationModeAMD( GLenum mode )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", mode );
  funcs->ext.p_glTessellationModeAMD( mode );
}

static GLboolean WINAPI glTestFenceAPPLE( GLuint fence )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", fence );
  return funcs->ext.p_glTestFenceAPPLE( fence );
}

static GLboolean WINAPI glTestFenceNV( GLuint fence )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", fence );
  return funcs->ext.p_glTestFenceNV( fence );
}

static GLboolean WINAPI glTestObjectAPPLE( GLenum object, GLuint name )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", object, name );
  return funcs->ext.p_glTestObjectAPPLE( object, name );
}

static void WINAPI glTexBuffer( GLenum target, GLenum internalformat, GLuint buffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", target, internalformat, buffer );
  funcs->ext.p_glTexBuffer( target, internalformat, buffer );
}

static void WINAPI glTexBufferARB( GLenum target, GLenum internalformat, GLuint buffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", target, internalformat, buffer );
  funcs->ext.p_glTexBufferARB( target, internalformat, buffer );
}

static void WINAPI glTexBufferEXT( GLenum target, GLenum internalformat, GLuint buffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", target, internalformat, buffer );
  funcs->ext.p_glTexBufferEXT( target, internalformat, buffer );
}

static void WINAPI glTexBufferRange( GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %ld, %ld)\n", target, internalformat, buffer, offset, size );
  funcs->ext.p_glTexBufferRange( target, internalformat, buffer, offset, size );
}

static void WINAPI glTexBumpParameterfvATI( GLenum pname, const GLfloat *param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", pname, param );
  funcs->ext.p_glTexBumpParameterfvATI( pname, param );
}

static void WINAPI glTexBumpParameterivATI( GLenum pname, const GLint *param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", pname, param );
  funcs->ext.p_glTexBumpParameterivATI( pname, param );
}

static void WINAPI glTexCoord1bOES( GLbyte s )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", s );
  funcs->ext.p_glTexCoord1bOES( s );
}

static void WINAPI glTexCoord1bvOES( const GLbyte *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", coords );
  funcs->ext.p_glTexCoord1bvOES( coords );
}

static void WINAPI glTexCoord1hNV( GLhalfNV s )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", s );
  funcs->ext.p_glTexCoord1hNV( s );
}

static void WINAPI glTexCoord1hvNV( const GLhalfNV *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glTexCoord1hvNV( v );
}

static void WINAPI glTexCoord1xOES( GLfixed s )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", s );
  funcs->ext.p_glTexCoord1xOES( s );
}

static void WINAPI glTexCoord1xvOES( const GLfixed *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", coords );
  funcs->ext.p_glTexCoord1xvOES( coords );
}

static void WINAPI glTexCoord2bOES( GLbyte s, GLbyte t )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", s, t );
  funcs->ext.p_glTexCoord2bOES( s, t );
}

static void WINAPI glTexCoord2bvOES( const GLbyte *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", coords );
  funcs->ext.p_glTexCoord2bvOES( coords );
}

static void WINAPI glTexCoord2fColor3fVertex3fSUN( GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f, %f, %f, %f, %f, %f, %f)\n", s, t, r, g, b, x, y, z );
  funcs->ext.p_glTexCoord2fColor3fVertex3fSUN( s, t, r, g, b, x, y, z );
}

static void WINAPI glTexCoord2fColor3fVertex3fvSUN( const GLfloat *tc, const GLfloat *c, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p, %p, %p)\n", tc, c, v );
  funcs->ext.p_glTexCoord2fColor3fVertex3fvSUN( tc, c, v );
}

static void WINAPI glTexCoord2fColor4fNormal3fVertex3fSUN( GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f)\n", s, t, r, g, b, a, nx, ny, nz, x, y, z );
  funcs->ext.p_glTexCoord2fColor4fNormal3fVertex3fSUN( s, t, r, g, b, a, nx, ny, nz, x, y, z );
}

static void WINAPI glTexCoord2fColor4fNormal3fVertex3fvSUN( const GLfloat *tc, const GLfloat *c, const GLfloat *n, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p, %p, %p, %p)\n", tc, c, n, v );
  funcs->ext.p_glTexCoord2fColor4fNormal3fVertex3fvSUN( tc, c, n, v );
}

static void WINAPI glTexCoord2fColor4ubVertex3fSUN( GLfloat s, GLfloat t, GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f, %d, %d, %d, %d, %f, %f, %f)\n", s, t, r, g, b, a, x, y, z );
  funcs->ext.p_glTexCoord2fColor4ubVertex3fSUN( s, t, r, g, b, a, x, y, z );
}

static void WINAPI glTexCoord2fColor4ubVertex3fvSUN( const GLfloat *tc, const GLubyte *c, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p, %p, %p)\n", tc, c, v );
  funcs->ext.p_glTexCoord2fColor4ubVertex3fvSUN( tc, c, v );
}

static void WINAPI glTexCoord2fNormal3fVertex3fSUN( GLfloat s, GLfloat t, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f, %f, %f, %f, %f, %f, %f)\n", s, t, nx, ny, nz, x, y, z );
  funcs->ext.p_glTexCoord2fNormal3fVertex3fSUN( s, t, nx, ny, nz, x, y, z );
}

static void WINAPI glTexCoord2fNormal3fVertex3fvSUN( const GLfloat *tc, const GLfloat *n, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p, %p, %p)\n", tc, n, v );
  funcs->ext.p_glTexCoord2fNormal3fVertex3fvSUN( tc, n, v );
}

static void WINAPI glTexCoord2fVertex3fSUN( GLfloat s, GLfloat t, GLfloat x, GLfloat y, GLfloat z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f, %f, %f, %f)\n", s, t, x, y, z );
  funcs->ext.p_glTexCoord2fVertex3fSUN( s, t, x, y, z );
}

static void WINAPI glTexCoord2fVertex3fvSUN( const GLfloat *tc, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p, %p)\n", tc, v );
  funcs->ext.p_glTexCoord2fVertex3fvSUN( tc, v );
}

static void WINAPI glTexCoord2hNV( GLhalfNV s, GLhalfNV t )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", s, t );
  funcs->ext.p_glTexCoord2hNV( s, t );
}

static void WINAPI glTexCoord2hvNV( const GLhalfNV *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glTexCoord2hvNV( v );
}

static void WINAPI glTexCoord2xOES( GLfixed s, GLfixed t )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", s, t );
  funcs->ext.p_glTexCoord2xOES( s, t );
}

static void WINAPI glTexCoord2xvOES( const GLfixed *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", coords );
  funcs->ext.p_glTexCoord2xvOES( coords );
}

static void WINAPI glTexCoord3bOES( GLbyte s, GLbyte t, GLbyte r )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", s, t, r );
  funcs->ext.p_glTexCoord3bOES( s, t, r );
}

static void WINAPI glTexCoord3bvOES( const GLbyte *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", coords );
  funcs->ext.p_glTexCoord3bvOES( coords );
}

static void WINAPI glTexCoord3hNV( GLhalfNV s, GLhalfNV t, GLhalfNV r )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", s, t, r );
  funcs->ext.p_glTexCoord3hNV( s, t, r );
}

static void WINAPI glTexCoord3hvNV( const GLhalfNV *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glTexCoord3hvNV( v );
}

static void WINAPI glTexCoord3xOES( GLfixed s, GLfixed t, GLfixed r )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", s, t, r );
  funcs->ext.p_glTexCoord3xOES( s, t, r );
}

static void WINAPI glTexCoord3xvOES( const GLfixed *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", coords );
  funcs->ext.p_glTexCoord3xvOES( coords );
}

static void WINAPI glTexCoord4bOES( GLbyte s, GLbyte t, GLbyte r, GLbyte q )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", s, t, r, q );
  funcs->ext.p_glTexCoord4bOES( s, t, r, q );
}

static void WINAPI glTexCoord4bvOES( const GLbyte *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", coords );
  funcs->ext.p_glTexCoord4bvOES( coords );
}

static void WINAPI glTexCoord4fColor4fNormal3fVertex4fSUN( GLfloat s, GLfloat t, GLfloat p, GLfloat q, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f)\n", s, t, p, q, r, g, b, a, nx, ny, nz, x, y, z, w );
  funcs->ext.p_glTexCoord4fColor4fNormal3fVertex4fSUN( s, t, p, q, r, g, b, a, nx, ny, nz, x, y, z, w );
}

static void WINAPI glTexCoord4fColor4fNormal3fVertex4fvSUN( const GLfloat *tc, const GLfloat *c, const GLfloat *n, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p, %p, %p, %p)\n", tc, c, n, v );
  funcs->ext.p_glTexCoord4fColor4fNormal3fVertex4fvSUN( tc, c, n, v );
}

static void WINAPI glTexCoord4fVertex4fSUN( GLfloat s, GLfloat t, GLfloat p, GLfloat q, GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f, %f, %f, %f, %f, %f, %f)\n", s, t, p, q, x, y, z, w );
  funcs->ext.p_glTexCoord4fVertex4fSUN( s, t, p, q, x, y, z, w );
}

static void WINAPI glTexCoord4fVertex4fvSUN( const GLfloat *tc, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p, %p)\n", tc, v );
  funcs->ext.p_glTexCoord4fVertex4fvSUN( tc, v );
}

static void WINAPI glTexCoord4hNV( GLhalfNV s, GLhalfNV t, GLhalfNV r, GLhalfNV q )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", s, t, r, q );
  funcs->ext.p_glTexCoord4hNV( s, t, r, q );
}

static void WINAPI glTexCoord4hvNV( const GLhalfNV *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glTexCoord4hvNV( v );
}

static void WINAPI glTexCoord4xOES( GLfixed s, GLfixed t, GLfixed r, GLfixed q )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", s, t, r, q );
  funcs->ext.p_glTexCoord4xOES( s, t, r, q );
}

static void WINAPI glTexCoord4xvOES( const GLfixed *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", coords );
  funcs->ext.p_glTexCoord4xvOES( coords );
}

static void WINAPI glTexCoordFormatNV( GLint size, GLenum type, GLsizei stride )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", size, type, stride );
  funcs->ext.p_glTexCoordFormatNV( size, type, stride );
}

static void WINAPI glTexCoordP1ui( GLenum type, GLuint coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", type, coords );
  funcs->ext.p_glTexCoordP1ui( type, coords );
}

static void WINAPI glTexCoordP1uiv( GLenum type, const GLuint *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", type, coords );
  funcs->ext.p_glTexCoordP1uiv( type, coords );
}

static void WINAPI glTexCoordP2ui( GLenum type, GLuint coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", type, coords );
  funcs->ext.p_glTexCoordP2ui( type, coords );
}

static void WINAPI glTexCoordP2uiv( GLenum type, const GLuint *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", type, coords );
  funcs->ext.p_glTexCoordP2uiv( type, coords );
}

static void WINAPI glTexCoordP3ui( GLenum type, GLuint coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", type, coords );
  funcs->ext.p_glTexCoordP3ui( type, coords );
}

static void WINAPI glTexCoordP3uiv( GLenum type, const GLuint *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", type, coords );
  funcs->ext.p_glTexCoordP3uiv( type, coords );
}

static void WINAPI glTexCoordP4ui( GLenum type, GLuint coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", type, coords );
  funcs->ext.p_glTexCoordP4ui( type, coords );
}

static void WINAPI glTexCoordP4uiv( GLenum type, const GLuint *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", type, coords );
  funcs->ext.p_glTexCoordP4uiv( type, coords );
}

static void WINAPI glTexCoordPointerEXT( GLint size, GLenum type, GLsizei stride, GLsizei count, const void *pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", size, type, stride, count, pointer );
  funcs->ext.p_glTexCoordPointerEXT( size, type, stride, count, pointer );
}

static void WINAPI glTexCoordPointerListIBM( GLint size, GLenum type, GLint stride, const void **pointer, GLint ptrstride )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p, %d)\n", size, type, stride, pointer, ptrstride );
  funcs->ext.p_glTexCoordPointerListIBM( size, type, stride, pointer, ptrstride );
}

static void WINAPI glTexCoordPointervINTEL( GLint size, GLenum type, const void **pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", size, type, pointer );
  funcs->ext.p_glTexCoordPointervINTEL( size, type, pointer );
}

static void WINAPI glTexEnvxOES( GLenum target, GLenum pname, GLfixed param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", target, pname, param );
  funcs->ext.p_glTexEnvxOES( target, pname, param );
}

static void WINAPI glTexEnvxvOES( GLenum target, GLenum pname, const GLfixed *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glTexEnvxvOES( target, pname, params );
}

static void WINAPI glTexFilterFuncSGIS( GLenum target, GLenum filter, GLsizei n, const GLfloat *weights )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", target, filter, n, weights );
  funcs->ext.p_glTexFilterFuncSGIS( target, filter, n, weights );
}

static void WINAPI glTexGenxOES( GLenum coord, GLenum pname, GLfixed param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", coord, pname, param );
  funcs->ext.p_glTexGenxOES( coord, pname, param );
}

static void WINAPI glTexGenxvOES( GLenum coord, GLenum pname, const GLfixed *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", coord, pname, params );
  funcs->ext.p_glTexGenxvOES( coord, pname, params );
}

static void WINAPI glTexImage2DMultisample( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d)\n", target, samples, internalformat, width, height, fixedsamplelocations );
  funcs->ext.p_glTexImage2DMultisample( target, samples, internalformat, width, height, fixedsamplelocations );
}

static void WINAPI glTexImage2DMultisampleCoverageNV( GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLint internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d)\n", target, coverageSamples, colorSamples, internalFormat, width, height, fixedSampleLocations );
  funcs->ext.p_glTexImage2DMultisampleCoverageNV( target, coverageSamples, colorSamples, internalFormat, width, height, fixedSampleLocations );
}

static void WINAPI glTexImage3D( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, height, depth, border, format, type, pixels );
  funcs->ext.p_glTexImage3D( target, level, internalformat, width, height, depth, border, format, type, pixels );
}

static void WINAPI glTexImage3DEXT( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, height, depth, border, format, type, pixels );
  funcs->ext.p_glTexImage3DEXT( target, level, internalformat, width, height, depth, border, format, type, pixels );
}

static void WINAPI glTexImage3DMultisample( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d)\n", target, samples, internalformat, width, height, depth, fixedsamplelocations );
  funcs->ext.p_glTexImage3DMultisample( target, samples, internalformat, width, height, depth, fixedsamplelocations );
}

static void WINAPI glTexImage3DMultisampleCoverageNV( GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d)\n", target, coverageSamples, colorSamples, internalFormat, width, height, depth, fixedSampleLocations );
  funcs->ext.p_glTexImage3DMultisampleCoverageNV( target, coverageSamples, colorSamples, internalFormat, width, height, depth, fixedSampleLocations );
}

static void WINAPI glTexImage4DSGIS( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLint border, GLenum format, GLenum type, const void *pixels )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, height, depth, size4d, border, format, type, pixels );
  funcs->ext.p_glTexImage4DSGIS( target, level, internalformat, width, height, depth, size4d, border, format, type, pixels );
}

static void WINAPI glTexPageCommitmentARB( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLboolean commit )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d)\n", target, level, xoffset, yoffset, zoffset, width, height, depth, commit );
  funcs->ext.p_glTexPageCommitmentARB( target, level, xoffset, yoffset, zoffset, width, height, depth, commit );
}

static void WINAPI glTexParameterIiv( GLenum target, GLenum pname, const GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glTexParameterIiv( target, pname, params );
}

static void WINAPI glTexParameterIivEXT( GLenum target, GLenum pname, const GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glTexParameterIivEXT( target, pname, params );
}

static void WINAPI glTexParameterIuiv( GLenum target, GLenum pname, const GLuint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glTexParameterIuiv( target, pname, params );
}

static void WINAPI glTexParameterIuivEXT( GLenum target, GLenum pname, const GLuint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glTexParameterIuivEXT( target, pname, params );
}

static void WINAPI glTexParameterxOES( GLenum target, GLenum pname, GLfixed param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", target, pname, param );
  funcs->ext.p_glTexParameterxOES( target, pname, param );
}

static void WINAPI glTexParameterxvOES( GLenum target, GLenum pname, const GLfixed *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, pname, params );
  funcs->ext.p_glTexParameterxvOES( target, pname, params );
}

static void WINAPI glTexRenderbufferNV( GLenum target, GLuint renderbuffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", target, renderbuffer );
  funcs->ext.p_glTexRenderbufferNV( target, renderbuffer );
}

static void WINAPI glTexStorage1D( GLenum target, GLsizei levels, GLenum internalformat, GLsizei width )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", target, levels, internalformat, width );
  funcs->ext.p_glTexStorage1D( target, levels, internalformat, width );
}

static void WINAPI glTexStorage2D( GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", target, levels, internalformat, width, height );
  funcs->ext.p_glTexStorage2D( target, levels, internalformat, width, height );
}

static void WINAPI glTexStorage2DMultisample( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d)\n", target, samples, internalformat, width, height, fixedsamplelocations );
  funcs->ext.p_glTexStorage2DMultisample( target, samples, internalformat, width, height, fixedsamplelocations );
}

static void WINAPI glTexStorage3D( GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d)\n", target, levels, internalformat, width, height, depth );
  funcs->ext.p_glTexStorage3D( target, levels, internalformat, width, height, depth );
}

static void WINAPI glTexStorage3DMultisample( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d)\n", target, samples, internalformat, width, height, depth, fixedsamplelocations );
  funcs->ext.p_glTexStorage3DMultisample( target, samples, internalformat, width, height, depth, fixedsamplelocations );
}

static void WINAPI glTexStorageMem1DEXT( GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLuint memory, GLuint64 offset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %s)\n", target, levels, internalFormat, width, memory, wine_dbgstr_longlong(offset) );
  funcs->ext.p_glTexStorageMem1DEXT( target, levels, internalFormat, width, memory, offset );
}

static void WINAPI glTexStorageMem2DEXT( GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLuint memory, GLuint64 offset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %s)\n", target, levels, internalFormat, width, height, memory, wine_dbgstr_longlong(offset) );
  funcs->ext.p_glTexStorageMem2DEXT( target, levels, internalFormat, width, height, memory, offset );
}

static void WINAPI glTexStorageMem2DMultisampleEXT( GLenum target, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %s)\n", target, samples, internalFormat, width, height, fixedSampleLocations, memory, wine_dbgstr_longlong(offset) );
  funcs->ext.p_glTexStorageMem2DMultisampleEXT( target, samples, internalFormat, width, height, fixedSampleLocations, memory, offset );
}

static void WINAPI glTexStorageMem3DEXT( GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLuint memory, GLuint64 offset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %s)\n", target, levels, internalFormat, width, height, depth, memory, wine_dbgstr_longlong(offset) );
  funcs->ext.p_glTexStorageMem3DEXT( target, levels, internalFormat, width, height, depth, memory, offset );
}

static void WINAPI glTexStorageMem3DMultisampleEXT( GLenum target, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %s)\n", target, samples, internalFormat, width, height, depth, fixedSampleLocations, memory, wine_dbgstr_longlong(offset) );
  funcs->ext.p_glTexStorageMem3DMultisampleEXT( target, samples, internalFormat, width, height, depth, fixedSampleLocations, memory, offset );
}

static void WINAPI glTexStorageSparseAMD( GLenum target, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLsizei layers, GLbitfield flags )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d)\n", target, internalFormat, width, height, depth, layers, flags );
  funcs->ext.p_glTexStorageSparseAMD( target, internalFormat, width, height, depth, layers, flags );
}

static void WINAPI glTexSubImage1DEXT( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, width, format, type, pixels );
  funcs->ext.p_glTexSubImage1DEXT( target, level, xoffset, width, format, type, pixels );
}

static void WINAPI glTexSubImage2DEXT( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, width, height, format, type, pixels );
  funcs->ext.p_glTexSubImage2DEXT( target, level, xoffset, yoffset, width, height, format, type, pixels );
}

static void WINAPI glTexSubImage3D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels );
  funcs->ext.p_glTexSubImage3D( target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels );
}

static void WINAPI glTexSubImage3DEXT( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels );
  funcs->ext.p_glTexSubImage3DEXT( target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels );
}

static void WINAPI glTexSubImage4DSGIS( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint woffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLenum format, GLenum type, const void *pixels )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, zoffset, woffset, width, height, depth, size4d, format, type, pixels );
  funcs->ext.p_glTexSubImage4DSGIS( target, level, xoffset, yoffset, zoffset, woffset, width, height, depth, size4d, format, type, pixels );
}

static void WINAPI glTextureBarrier(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  funcs->ext.p_glTextureBarrier();
}

static void WINAPI glTextureBarrierNV(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  funcs->ext.p_glTextureBarrierNV();
}

static void WINAPI glTextureBuffer( GLuint texture, GLenum internalformat, GLuint buffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", texture, internalformat, buffer );
  funcs->ext.p_glTextureBuffer( texture, internalformat, buffer );
}

static void WINAPI glTextureBufferEXT( GLuint texture, GLenum target, GLenum internalformat, GLuint buffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", texture, target, internalformat, buffer );
  funcs->ext.p_glTextureBufferEXT( texture, target, internalformat, buffer );
}

static void WINAPI glTextureBufferRange( GLuint texture, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %ld, %ld)\n", texture, internalformat, buffer, offset, size );
  funcs->ext.p_glTextureBufferRange( texture, internalformat, buffer, offset, size );
}

static void WINAPI glTextureBufferRangeEXT( GLuint texture, GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %ld, %ld)\n", texture, target, internalformat, buffer, offset, size );
  funcs->ext.p_glTextureBufferRangeEXT( texture, target, internalformat, buffer, offset, size );
}

static void WINAPI glTextureColorMaskSGIS( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", red, green, blue, alpha );
  funcs->ext.p_glTextureColorMaskSGIS( red, green, blue, alpha );
}

static void WINAPI glTextureImage1DEXT( GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texture, target, level, internalformat, width, border, format, type, pixels );
  funcs->ext.p_glTextureImage1DEXT( texture, target, level, internalformat, width, border, format, type, pixels );
}

static void WINAPI glTextureImage2DEXT( GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texture, target, level, internalformat, width, height, border, format, type, pixels );
  funcs->ext.p_glTextureImage2DEXT( texture, target, level, internalformat, width, height, border, format, type, pixels );
}

static void WINAPI glTextureImage2DMultisampleCoverageNV( GLuint texture, GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLint internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d)\n", texture, target, coverageSamples, colorSamples, internalFormat, width, height, fixedSampleLocations );
  funcs->ext.p_glTextureImage2DMultisampleCoverageNV( texture, target, coverageSamples, colorSamples, internalFormat, width, height, fixedSampleLocations );
}

static void WINAPI glTextureImage2DMultisampleNV( GLuint texture, GLenum target, GLsizei samples, GLint internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d)\n", texture, target, samples, internalFormat, width, height, fixedSampleLocations );
  funcs->ext.p_glTextureImage2DMultisampleNV( texture, target, samples, internalFormat, width, height, fixedSampleLocations );
}

static void WINAPI glTextureImage3DEXT( GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texture, target, level, internalformat, width, height, depth, border, format, type, pixels );
  funcs->ext.p_glTextureImage3DEXT( texture, target, level, internalformat, width, height, depth, border, format, type, pixels );
}

static void WINAPI glTextureImage3DMultisampleCoverageNV( GLuint texture, GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d)\n", texture, target, coverageSamples, colorSamples, internalFormat, width, height, depth, fixedSampleLocations );
  funcs->ext.p_glTextureImage3DMultisampleCoverageNV( texture, target, coverageSamples, colorSamples, internalFormat, width, height, depth, fixedSampleLocations );
}

static void WINAPI glTextureImage3DMultisampleNV( GLuint texture, GLenum target, GLsizei samples, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d)\n", texture, target, samples, internalFormat, width, height, depth, fixedSampleLocations );
  funcs->ext.p_glTextureImage3DMultisampleNV( texture, target, samples, internalFormat, width, height, depth, fixedSampleLocations );
}

static void WINAPI glTextureLightEXT( GLenum pname )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", pname );
  funcs->ext.p_glTextureLightEXT( pname );
}

static void WINAPI glTextureMaterialEXT( GLenum face, GLenum mode )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", face, mode );
  funcs->ext.p_glTextureMaterialEXT( face, mode );
}

static void WINAPI glTextureNormalEXT( GLenum mode )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", mode );
  funcs->ext.p_glTextureNormalEXT( mode );
}

static void WINAPI glTexturePageCommitmentEXT( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLboolean commit )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d)\n", texture, level, xoffset, yoffset, zoffset, width, height, depth, commit );
  funcs->ext.p_glTexturePageCommitmentEXT( texture, level, xoffset, yoffset, zoffset, width, height, depth, commit );
}

static void WINAPI glTextureParameterIiv( GLuint texture, GLenum pname, const GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", texture, pname, params );
  funcs->ext.p_glTextureParameterIiv( texture, pname, params );
}

static void WINAPI glTextureParameterIivEXT( GLuint texture, GLenum target, GLenum pname, const GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", texture, target, pname, params );
  funcs->ext.p_glTextureParameterIivEXT( texture, target, pname, params );
}

static void WINAPI glTextureParameterIuiv( GLuint texture, GLenum pname, const GLuint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", texture, pname, params );
  funcs->ext.p_glTextureParameterIuiv( texture, pname, params );
}

static void WINAPI glTextureParameterIuivEXT( GLuint texture, GLenum target, GLenum pname, const GLuint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", texture, target, pname, params );
  funcs->ext.p_glTextureParameterIuivEXT( texture, target, pname, params );
}

static void WINAPI glTextureParameterf( GLuint texture, GLenum pname, GLfloat param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %f)\n", texture, pname, param );
  funcs->ext.p_glTextureParameterf( texture, pname, param );
}

static void WINAPI glTextureParameterfEXT( GLuint texture, GLenum target, GLenum pname, GLfloat param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %f)\n", texture, target, pname, param );
  funcs->ext.p_glTextureParameterfEXT( texture, target, pname, param );
}

static void WINAPI glTextureParameterfv( GLuint texture, GLenum pname, const GLfloat *param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", texture, pname, param );
  funcs->ext.p_glTextureParameterfv( texture, pname, param );
}

static void WINAPI glTextureParameterfvEXT( GLuint texture, GLenum target, GLenum pname, const GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", texture, target, pname, params );
  funcs->ext.p_glTextureParameterfvEXT( texture, target, pname, params );
}

static void WINAPI glTextureParameteri( GLuint texture, GLenum pname, GLint param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", texture, pname, param );
  funcs->ext.p_glTextureParameteri( texture, pname, param );
}

static void WINAPI glTextureParameteriEXT( GLuint texture, GLenum target, GLenum pname, GLint param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", texture, target, pname, param );
  funcs->ext.p_glTextureParameteriEXT( texture, target, pname, param );
}

static void WINAPI glTextureParameteriv( GLuint texture, GLenum pname, const GLint *param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", texture, pname, param );
  funcs->ext.p_glTextureParameteriv( texture, pname, param );
}

static void WINAPI glTextureParameterivEXT( GLuint texture, GLenum target, GLenum pname, const GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", texture, target, pname, params );
  funcs->ext.p_glTextureParameterivEXT( texture, target, pname, params );
}

static void WINAPI glTextureRangeAPPLE( GLenum target, GLsizei length, const void *pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", target, length, pointer );
  funcs->ext.p_glTextureRangeAPPLE( target, length, pointer );
}

static void WINAPI glTextureRenderbufferEXT( GLuint texture, GLenum target, GLuint renderbuffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", texture, target, renderbuffer );
  funcs->ext.p_glTextureRenderbufferEXT( texture, target, renderbuffer );
}

static void WINAPI glTextureStorage1D( GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", texture, levels, internalformat, width );
  funcs->ext.p_glTextureStorage1D( texture, levels, internalformat, width );
}

static void WINAPI glTextureStorage1DEXT( GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", texture, target, levels, internalformat, width );
  funcs->ext.p_glTextureStorage1DEXT( texture, target, levels, internalformat, width );
}

static void WINAPI glTextureStorage2D( GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", texture, levels, internalformat, width, height );
  funcs->ext.p_glTextureStorage2D( texture, levels, internalformat, width, height );
}

static void WINAPI glTextureStorage2DEXT( GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d)\n", texture, target, levels, internalformat, width, height );
  funcs->ext.p_glTextureStorage2DEXT( texture, target, levels, internalformat, width, height );
}

static void WINAPI glTextureStorage2DMultisample( GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d)\n", texture, samples, internalformat, width, height, fixedsamplelocations );
  funcs->ext.p_glTextureStorage2DMultisample( texture, samples, internalformat, width, height, fixedsamplelocations );
}

static void WINAPI glTextureStorage2DMultisampleEXT( GLuint texture, GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d)\n", texture, target, samples, internalformat, width, height, fixedsamplelocations );
  funcs->ext.p_glTextureStorage2DMultisampleEXT( texture, target, samples, internalformat, width, height, fixedsamplelocations );
}

static void WINAPI glTextureStorage3D( GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d)\n", texture, levels, internalformat, width, height, depth );
  funcs->ext.p_glTextureStorage3D( texture, levels, internalformat, width, height, depth );
}

static void WINAPI glTextureStorage3DEXT( GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d)\n", texture, target, levels, internalformat, width, height, depth );
  funcs->ext.p_glTextureStorage3DEXT( texture, target, levels, internalformat, width, height, depth );
}

static void WINAPI glTextureStorage3DMultisample( GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d)\n", texture, samples, internalformat, width, height, depth, fixedsamplelocations );
  funcs->ext.p_glTextureStorage3DMultisample( texture, samples, internalformat, width, height, depth, fixedsamplelocations );
}

static void WINAPI glTextureStorage3DMultisampleEXT( GLuint texture, GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d)\n", texture, target, samples, internalformat, width, height, depth, fixedsamplelocations );
  funcs->ext.p_glTextureStorage3DMultisampleEXT( texture, target, samples, internalformat, width, height, depth, fixedsamplelocations );
}

static void WINAPI glTextureStorageMem1DEXT( GLuint texture, GLsizei levels, GLenum internalFormat, GLsizei width, GLuint memory, GLuint64 offset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %s)\n", texture, levels, internalFormat, width, memory, wine_dbgstr_longlong(offset) );
  funcs->ext.p_glTextureStorageMem1DEXT( texture, levels, internalFormat, width, memory, offset );
}

static void WINAPI glTextureStorageMem2DEXT( GLuint texture, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLuint memory, GLuint64 offset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %s)\n", texture, levels, internalFormat, width, height, memory, wine_dbgstr_longlong(offset) );
  funcs->ext.p_glTextureStorageMem2DEXT( texture, levels, internalFormat, width, height, memory, offset );
}

static void WINAPI glTextureStorageMem2DMultisampleEXT( GLuint texture, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %s)\n", texture, samples, internalFormat, width, height, fixedSampleLocations, memory, wine_dbgstr_longlong(offset) );
  funcs->ext.p_glTextureStorageMem2DMultisampleEXT( texture, samples, internalFormat, width, height, fixedSampleLocations, memory, offset );
}

static void WINAPI glTextureStorageMem3DEXT( GLuint texture, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLuint memory, GLuint64 offset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %s)\n", texture, levels, internalFormat, width, height, depth, memory, wine_dbgstr_longlong(offset) );
  funcs->ext.p_glTextureStorageMem3DEXT( texture, levels, internalFormat, width, height, depth, memory, offset );
}

static void WINAPI glTextureStorageMem3DMultisampleEXT( GLuint texture, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %s)\n", texture, samples, internalFormat, width, height, depth, fixedSampleLocations, memory, wine_dbgstr_longlong(offset) );
  funcs->ext.p_glTextureStorageMem3DMultisampleEXT( texture, samples, internalFormat, width, height, depth, fixedSampleLocations, memory, offset );
}

static void WINAPI glTextureStorageSparseAMD( GLuint texture, GLenum target, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLsizei layers, GLbitfield flags )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d)\n", texture, target, internalFormat, width, height, depth, layers, flags );
  funcs->ext.p_glTextureStorageSparseAMD( texture, target, internalFormat, width, height, depth, layers, flags );
}

static void WINAPI glTextureSubImage1D( GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %p)\n", texture, level, xoffset, width, format, type, pixels );
  funcs->ext.p_glTextureSubImage1D( texture, level, xoffset, width, format, type, pixels );
}

static void WINAPI glTextureSubImage1DEXT( GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %p)\n", texture, target, level, xoffset, width, format, type, pixels );
  funcs->ext.p_glTextureSubImage1DEXT( texture, target, level, xoffset, width, format, type, pixels );
}

static void WINAPI glTextureSubImage2D( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texture, level, xoffset, yoffset, width, height, format, type, pixels );
  funcs->ext.p_glTextureSubImage2D( texture, level, xoffset, yoffset, width, height, format, type, pixels );
}

static void WINAPI glTextureSubImage2DEXT( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texture, target, level, xoffset, yoffset, width, height, format, type, pixels );
  funcs->ext.p_glTextureSubImage2DEXT( texture, target, level, xoffset, yoffset, width, height, format, type, pixels );
}

static void WINAPI glTextureSubImage3D( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels );
  funcs->ext.p_glTextureSubImage3D( texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels );
}

static void WINAPI glTextureSubImage3DEXT( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texture, target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels );
  funcs->ext.p_glTextureSubImage3DEXT( texture, target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels );
}

static void WINAPI glTextureView( GLuint texture, GLenum target, GLuint origtexture, GLenum internalformat, GLuint minlevel, GLuint numlevels, GLuint minlayer, GLuint numlayers )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %d)\n", texture, target, origtexture, internalformat, minlevel, numlevels, minlayer, numlayers );
  funcs->ext.p_glTextureView( texture, target, origtexture, internalformat, minlevel, numlevels, minlayer, numlayers );
}

static void WINAPI glTrackMatrixNV( GLenum target, GLuint address, GLenum matrix, GLenum transform )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", target, address, matrix, transform );
  funcs->ext.p_glTrackMatrixNV( target, address, matrix, transform );
}

static void WINAPI glTransformFeedbackAttribsNV( GLsizei count, const GLint *attribs, GLenum bufferMode )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p, %d)\n", count, attribs, bufferMode );
  funcs->ext.p_glTransformFeedbackAttribsNV( count, attribs, bufferMode );
}

static void WINAPI glTransformFeedbackBufferBase( GLuint xfb, GLuint index, GLuint buffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", xfb, index, buffer );
  funcs->ext.p_glTransformFeedbackBufferBase( xfb, index, buffer );
}

static void WINAPI glTransformFeedbackBufferRange( GLuint xfb, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %ld, %ld)\n", xfb, index, buffer, offset, size );
  funcs->ext.p_glTransformFeedbackBufferRange( xfb, index, buffer, offset, size );
}

static void WINAPI glTransformFeedbackStreamAttribsNV( GLsizei count, const GLint *attribs, GLsizei nbuffers, const GLint *bufstreams, GLenum bufferMode )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p, %d, %p, %d)\n", count, attribs, nbuffers, bufstreams, bufferMode );
  funcs->ext.p_glTransformFeedbackStreamAttribsNV( count, attribs, nbuffers, bufstreams, bufferMode );
}

static void WINAPI glTransformFeedbackVaryings( GLuint program, GLsizei count, const GLchar *const*varyings, GLenum bufferMode )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %d)\n", program, count, varyings, bufferMode );
  funcs->ext.p_glTransformFeedbackVaryings( program, count, varyings, bufferMode );
}

static void WINAPI glTransformFeedbackVaryingsEXT( GLuint program, GLsizei count, const GLchar *const*varyings, GLenum bufferMode )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %d)\n", program, count, varyings, bufferMode );
  funcs->ext.p_glTransformFeedbackVaryingsEXT( program, count, varyings, bufferMode );
}

static void WINAPI glTransformFeedbackVaryingsNV( GLuint program, GLsizei count, const GLint *locations, GLenum bufferMode )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %d)\n", program, count, locations, bufferMode );
  funcs->ext.p_glTransformFeedbackVaryingsNV( program, count, locations, bufferMode );
}

static void WINAPI glTransformPathNV( GLuint resultPath, GLuint srcPath, GLenum transformType, const GLfloat *transformValues )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", resultPath, srcPath, transformType, transformValues );
  funcs->ext.p_glTransformPathNV( resultPath, srcPath, transformType, transformValues );
}

static void WINAPI glTranslatexOES( GLfixed x, GLfixed y, GLfixed z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", x, y, z );
  funcs->ext.p_glTranslatexOES( x, y, z );
}

static void WINAPI glUniform1d( GLint location, GLdouble x )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f)\n", location, x );
  funcs->ext.p_glUniform1d( location, x );
}

static void WINAPI glUniform1dv( GLint location, GLsizei count, const GLdouble *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform1dv( location, count, value );
}

static void WINAPI glUniform1f( GLint location, GLfloat v0 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f)\n", location, v0 );
  funcs->ext.p_glUniform1f( location, v0 );
}

static void WINAPI glUniform1fARB( GLint location, GLfloat v0 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f)\n", location, v0 );
  funcs->ext.p_glUniform1fARB( location, v0 );
}

static void WINAPI glUniform1fv( GLint location, GLsizei count, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform1fv( location, count, value );
}

static void WINAPI glUniform1fvARB( GLint location, GLsizei count, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform1fvARB( location, count, value );
}

static void WINAPI glUniform1i( GLint location, GLint v0 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", location, v0 );
  funcs->ext.p_glUniform1i( location, v0 );
}

static void WINAPI glUniform1i64ARB( GLint location, GLint64 x )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %s)\n", location, wine_dbgstr_longlong(x) );
  funcs->ext.p_glUniform1i64ARB( location, x );
}

static void WINAPI glUniform1i64NV( GLint location, GLint64EXT x )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %s)\n", location, wine_dbgstr_longlong(x) );
  funcs->ext.p_glUniform1i64NV( location, x );
}

static void WINAPI glUniform1i64vARB( GLint location, GLsizei count, const GLint64 *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform1i64vARB( location, count, value );
}

static void WINAPI glUniform1i64vNV( GLint location, GLsizei count, const GLint64EXT *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform1i64vNV( location, count, value );
}

static void WINAPI glUniform1iARB( GLint location, GLint v0 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", location, v0 );
  funcs->ext.p_glUniform1iARB( location, v0 );
}

static void WINAPI glUniform1iv( GLint location, GLsizei count, const GLint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform1iv( location, count, value );
}

static void WINAPI glUniform1ivARB( GLint location, GLsizei count, const GLint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform1ivARB( location, count, value );
}

static void WINAPI glUniform1ui( GLint location, GLuint v0 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", location, v0 );
  funcs->ext.p_glUniform1ui( location, v0 );
}

static void WINAPI glUniform1ui64ARB( GLint location, GLuint64 x )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %s)\n", location, wine_dbgstr_longlong(x) );
  funcs->ext.p_glUniform1ui64ARB( location, x );
}

static void WINAPI glUniform1ui64NV( GLint location, GLuint64EXT x )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %s)\n", location, wine_dbgstr_longlong(x) );
  funcs->ext.p_glUniform1ui64NV( location, x );
}

static void WINAPI glUniform1ui64vARB( GLint location, GLsizei count, const GLuint64 *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform1ui64vARB( location, count, value );
}

static void WINAPI glUniform1ui64vNV( GLint location, GLsizei count, const GLuint64EXT *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform1ui64vNV( location, count, value );
}

static void WINAPI glUniform1uiEXT( GLint location, GLuint v0 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", location, v0 );
  funcs->ext.p_glUniform1uiEXT( location, v0 );
}

static void WINAPI glUniform1uiv( GLint location, GLsizei count, const GLuint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform1uiv( location, count, value );
}

static void WINAPI glUniform1uivEXT( GLint location, GLsizei count, const GLuint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform1uivEXT( location, count, value );
}

static void WINAPI glUniform2d( GLint location, GLdouble x, GLdouble y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f)\n", location, x, y );
  funcs->ext.p_glUniform2d( location, x, y );
}

static void WINAPI glUniform2dv( GLint location, GLsizei count, const GLdouble *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform2dv( location, count, value );
}

static void WINAPI glUniform2f( GLint location, GLfloat v0, GLfloat v1 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f)\n", location, v0, v1 );
  funcs->ext.p_glUniform2f( location, v0, v1 );
}

static void WINAPI glUniform2fARB( GLint location, GLfloat v0, GLfloat v1 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f)\n", location, v0, v1 );
  funcs->ext.p_glUniform2fARB( location, v0, v1 );
}

static void WINAPI glUniform2fv( GLint location, GLsizei count, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform2fv( location, count, value );
}

static void WINAPI glUniform2fvARB( GLint location, GLsizei count, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform2fvARB( location, count, value );
}

static void WINAPI glUniform2i( GLint location, GLint v0, GLint v1 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", location, v0, v1 );
  funcs->ext.p_glUniform2i( location, v0, v1 );
}

static void WINAPI glUniform2i64ARB( GLint location, GLint64 x, GLint64 y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %s, %s)\n", location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y) );
  funcs->ext.p_glUniform2i64ARB( location, x, y );
}

static void WINAPI glUniform2i64NV( GLint location, GLint64EXT x, GLint64EXT y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %s, %s)\n", location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y) );
  funcs->ext.p_glUniform2i64NV( location, x, y );
}

static void WINAPI glUniform2i64vARB( GLint location, GLsizei count, const GLint64 *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform2i64vARB( location, count, value );
}

static void WINAPI glUniform2i64vNV( GLint location, GLsizei count, const GLint64EXT *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform2i64vNV( location, count, value );
}

static void WINAPI glUniform2iARB( GLint location, GLint v0, GLint v1 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", location, v0, v1 );
  funcs->ext.p_glUniform2iARB( location, v0, v1 );
}

static void WINAPI glUniform2iv( GLint location, GLsizei count, const GLint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform2iv( location, count, value );
}

static void WINAPI glUniform2ivARB( GLint location, GLsizei count, const GLint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform2ivARB( location, count, value );
}

static void WINAPI glUniform2ui( GLint location, GLuint v0, GLuint v1 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", location, v0, v1 );
  funcs->ext.p_glUniform2ui( location, v0, v1 );
}

static void WINAPI glUniform2ui64ARB( GLint location, GLuint64 x, GLuint64 y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %s, %s)\n", location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y) );
  funcs->ext.p_glUniform2ui64ARB( location, x, y );
}

static void WINAPI glUniform2ui64NV( GLint location, GLuint64EXT x, GLuint64EXT y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %s, %s)\n", location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y) );
  funcs->ext.p_glUniform2ui64NV( location, x, y );
}

static void WINAPI glUniform2ui64vARB( GLint location, GLsizei count, const GLuint64 *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform2ui64vARB( location, count, value );
}

static void WINAPI glUniform2ui64vNV( GLint location, GLsizei count, const GLuint64EXT *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform2ui64vNV( location, count, value );
}

static void WINAPI glUniform2uiEXT( GLint location, GLuint v0, GLuint v1 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", location, v0, v1 );
  funcs->ext.p_glUniform2uiEXT( location, v0, v1 );
}

static void WINAPI glUniform2uiv( GLint location, GLsizei count, const GLuint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform2uiv( location, count, value );
}

static void WINAPI glUniform2uivEXT( GLint location, GLsizei count, const GLuint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform2uivEXT( location, count, value );
}

static void WINAPI glUniform3d( GLint location, GLdouble x, GLdouble y, GLdouble z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f)\n", location, x, y, z );
  funcs->ext.p_glUniform3d( location, x, y, z );
}

static void WINAPI glUniform3dv( GLint location, GLsizei count, const GLdouble *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform3dv( location, count, value );
}

static void WINAPI glUniform3f( GLint location, GLfloat v0, GLfloat v1, GLfloat v2 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f)\n", location, v0, v1, v2 );
  funcs->ext.p_glUniform3f( location, v0, v1, v2 );
}

static void WINAPI glUniform3fARB( GLint location, GLfloat v0, GLfloat v1, GLfloat v2 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f)\n", location, v0, v1, v2 );
  funcs->ext.p_glUniform3fARB( location, v0, v1, v2 );
}

static void WINAPI glUniform3fv( GLint location, GLsizei count, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform3fv( location, count, value );
}

static void WINAPI glUniform3fvARB( GLint location, GLsizei count, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform3fvARB( location, count, value );
}

static void WINAPI glUniform3i( GLint location, GLint v0, GLint v1, GLint v2 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", location, v0, v1, v2 );
  funcs->ext.p_glUniform3i( location, v0, v1, v2 );
}

static void WINAPI glUniform3i64ARB( GLint location, GLint64 x, GLint64 y, GLint64 z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %s, %s, %s)\n", location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z) );
  funcs->ext.p_glUniform3i64ARB( location, x, y, z );
}

static void WINAPI glUniform3i64NV( GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %s, %s, %s)\n", location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z) );
  funcs->ext.p_glUniform3i64NV( location, x, y, z );
}

static void WINAPI glUniform3i64vARB( GLint location, GLsizei count, const GLint64 *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform3i64vARB( location, count, value );
}

static void WINAPI glUniform3i64vNV( GLint location, GLsizei count, const GLint64EXT *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform3i64vNV( location, count, value );
}

static void WINAPI glUniform3iARB( GLint location, GLint v0, GLint v1, GLint v2 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", location, v0, v1, v2 );
  funcs->ext.p_glUniform3iARB( location, v0, v1, v2 );
}

static void WINAPI glUniform3iv( GLint location, GLsizei count, const GLint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform3iv( location, count, value );
}

static void WINAPI glUniform3ivARB( GLint location, GLsizei count, const GLint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform3ivARB( location, count, value );
}

static void WINAPI glUniform3ui( GLint location, GLuint v0, GLuint v1, GLuint v2 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", location, v0, v1, v2 );
  funcs->ext.p_glUniform3ui( location, v0, v1, v2 );
}

static void WINAPI glUniform3ui64ARB( GLint location, GLuint64 x, GLuint64 y, GLuint64 z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %s, %s, %s)\n", location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z) );
  funcs->ext.p_glUniform3ui64ARB( location, x, y, z );
}

static void WINAPI glUniform3ui64NV( GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %s, %s, %s)\n", location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z) );
  funcs->ext.p_glUniform3ui64NV( location, x, y, z );
}

static void WINAPI glUniform3ui64vARB( GLint location, GLsizei count, const GLuint64 *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform3ui64vARB( location, count, value );
}

static void WINAPI glUniform3ui64vNV( GLint location, GLsizei count, const GLuint64EXT *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform3ui64vNV( location, count, value );
}

static void WINAPI glUniform3uiEXT( GLint location, GLuint v0, GLuint v1, GLuint v2 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", location, v0, v1, v2 );
  funcs->ext.p_glUniform3uiEXT( location, v0, v1, v2 );
}

static void WINAPI glUniform3uiv( GLint location, GLsizei count, const GLuint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform3uiv( location, count, value );
}

static void WINAPI glUniform3uivEXT( GLint location, GLsizei count, const GLuint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform3uivEXT( location, count, value );
}

static void WINAPI glUniform4d( GLint location, GLdouble x, GLdouble y, GLdouble z, GLdouble w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f, %f)\n", location, x, y, z, w );
  funcs->ext.p_glUniform4d( location, x, y, z, w );
}

static void WINAPI glUniform4dv( GLint location, GLsizei count, const GLdouble *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform4dv( location, count, value );
}

static void WINAPI glUniform4f( GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f, %f)\n", location, v0, v1, v2, v3 );
  funcs->ext.p_glUniform4f( location, v0, v1, v2, v3 );
}

static void WINAPI glUniform4fARB( GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f, %f)\n", location, v0, v1, v2, v3 );
  funcs->ext.p_glUniform4fARB( location, v0, v1, v2, v3 );
}

static void WINAPI glUniform4fv( GLint location, GLsizei count, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform4fv( location, count, value );
}

static void WINAPI glUniform4fvARB( GLint location, GLsizei count, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform4fvARB( location, count, value );
}

static void WINAPI glUniform4i( GLint location, GLint v0, GLint v1, GLint v2, GLint v3 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", location, v0, v1, v2, v3 );
  funcs->ext.p_glUniform4i( location, v0, v1, v2, v3 );
}

static void WINAPI glUniform4i64ARB( GLint location, GLint64 x, GLint64 y, GLint64 z, GLint64 w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %s, %s, %s, %s)\n", location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z), wine_dbgstr_longlong(w) );
  funcs->ext.p_glUniform4i64ARB( location, x, y, z, w );
}

static void WINAPI glUniform4i64NV( GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z, GLint64EXT w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %s, %s, %s, %s)\n", location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z), wine_dbgstr_longlong(w) );
  funcs->ext.p_glUniform4i64NV( location, x, y, z, w );
}

static void WINAPI glUniform4i64vARB( GLint location, GLsizei count, const GLint64 *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform4i64vARB( location, count, value );
}

static void WINAPI glUniform4i64vNV( GLint location, GLsizei count, const GLint64EXT *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform4i64vNV( location, count, value );
}

static void WINAPI glUniform4iARB( GLint location, GLint v0, GLint v1, GLint v2, GLint v3 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", location, v0, v1, v2, v3 );
  funcs->ext.p_glUniform4iARB( location, v0, v1, v2, v3 );
}

static void WINAPI glUniform4iv( GLint location, GLsizei count, const GLint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform4iv( location, count, value );
}

static void WINAPI glUniform4ivARB( GLint location, GLsizei count, const GLint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform4ivARB( location, count, value );
}

static void WINAPI glUniform4ui( GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", location, v0, v1, v2, v3 );
  funcs->ext.p_glUniform4ui( location, v0, v1, v2, v3 );
}

static void WINAPI glUniform4ui64ARB( GLint location, GLuint64 x, GLuint64 y, GLuint64 z, GLuint64 w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %s, %s, %s, %s)\n", location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z), wine_dbgstr_longlong(w) );
  funcs->ext.p_glUniform4ui64ARB( location, x, y, z, w );
}

static void WINAPI glUniform4ui64NV( GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z, GLuint64EXT w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %s, %s, %s, %s)\n", location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z), wine_dbgstr_longlong(w) );
  funcs->ext.p_glUniform4ui64NV( location, x, y, z, w );
}

static void WINAPI glUniform4ui64vARB( GLint location, GLsizei count, const GLuint64 *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform4ui64vARB( location, count, value );
}

static void WINAPI glUniform4ui64vNV( GLint location, GLsizei count, const GLuint64EXT *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform4ui64vNV( location, count, value );
}

static void WINAPI glUniform4uiEXT( GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3 )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", location, v0, v1, v2, v3 );
  funcs->ext.p_glUniform4uiEXT( location, v0, v1, v2, v3 );
}

static void WINAPI glUniform4uiv( GLint location, GLsizei count, const GLuint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform4uiv( location, count, value );
}

static void WINAPI glUniform4uivEXT( GLint location, GLsizei count, const GLuint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniform4uivEXT( location, count, value );
}

static void WINAPI glUniformBlockBinding( GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", program, uniformBlockIndex, uniformBlockBinding );
  funcs->ext.p_glUniformBlockBinding( program, uniformBlockIndex, uniformBlockBinding );
}

static void WINAPI glUniformBufferEXT( GLuint program, GLint location, GLuint buffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", program, location, buffer );
  funcs->ext.p_glUniformBufferEXT( program, location, buffer );
}

static void WINAPI glUniformHandleui64ARB( GLint location, GLuint64 value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %s)\n", location, wine_dbgstr_longlong(value) );
  funcs->ext.p_glUniformHandleui64ARB( location, value );
}

static void WINAPI glUniformHandleui64NV( GLint location, GLuint64 value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %s)\n", location, wine_dbgstr_longlong(value) );
  funcs->ext.p_glUniformHandleui64NV( location, value );
}

static void WINAPI glUniformHandleui64vARB( GLint location, GLsizei count, const GLuint64 *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniformHandleui64vARB( location, count, value );
}

static void WINAPI glUniformHandleui64vNV( GLint location, GLsizei count, const GLuint64 *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniformHandleui64vNV( location, count, value );
}

static void WINAPI glUniformMatrix2dv( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix2dv( location, count, transpose, value );
}

static void WINAPI glUniformMatrix2fv( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix2fv( location, count, transpose, value );
}

static void WINAPI glUniformMatrix2fvARB( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix2fvARB( location, count, transpose, value );
}

static void WINAPI glUniformMatrix2x3dv( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix2x3dv( location, count, transpose, value );
}

static void WINAPI glUniformMatrix2x3fv( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix2x3fv( location, count, transpose, value );
}

static void WINAPI glUniformMatrix2x4dv( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix2x4dv( location, count, transpose, value );
}

static void WINAPI glUniformMatrix2x4fv( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix2x4fv( location, count, transpose, value );
}

static void WINAPI glUniformMatrix3dv( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix3dv( location, count, transpose, value );
}

static void WINAPI glUniformMatrix3fv( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix3fv( location, count, transpose, value );
}

static void WINAPI glUniformMatrix3fvARB( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix3fvARB( location, count, transpose, value );
}

static void WINAPI glUniformMatrix3x2dv( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix3x2dv( location, count, transpose, value );
}

static void WINAPI glUniformMatrix3x2fv( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix3x2fv( location, count, transpose, value );
}

static void WINAPI glUniformMatrix3x4dv( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix3x4dv( location, count, transpose, value );
}

static void WINAPI glUniformMatrix3x4fv( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix3x4fv( location, count, transpose, value );
}

static void WINAPI glUniformMatrix4dv( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix4dv( location, count, transpose, value );
}

static void WINAPI glUniformMatrix4fv( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix4fv( location, count, transpose, value );
}

static void WINAPI glUniformMatrix4fvARB( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix4fvARB( location, count, transpose, value );
}

static void WINAPI glUniformMatrix4x2dv( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix4x2dv( location, count, transpose, value );
}

static void WINAPI glUniformMatrix4x2fv( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix4x2fv( location, count, transpose, value );
}

static void WINAPI glUniformMatrix4x3dv( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix4x3dv( location, count, transpose, value );
}

static void WINAPI glUniformMatrix4x3fv( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", location, count, transpose, value );
  funcs->ext.p_glUniformMatrix4x3fv( location, count, transpose, value );
}

static void WINAPI glUniformSubroutinesuiv( GLenum shadertype, GLsizei count, const GLuint *indices )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", shadertype, count, indices );
  funcs->ext.p_glUniformSubroutinesuiv( shadertype, count, indices );
}

static void WINAPI glUniformui64NV( GLint location, GLuint64EXT value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %s)\n", location, wine_dbgstr_longlong(value) );
  funcs->ext.p_glUniformui64NV( location, value );
}

static void WINAPI glUniformui64vNV( GLint location, GLsizei count, const GLuint64EXT *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", location, count, value );
  funcs->ext.p_glUniformui64vNV( location, count, value );
}

static void WINAPI glUnlockArraysEXT(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  funcs->ext.p_glUnlockArraysEXT();
}

static GLboolean WINAPI glUnmapBuffer( GLenum target )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", target );
  return funcs->ext.p_glUnmapBuffer( target );
}

static GLboolean WINAPI glUnmapBufferARB( GLenum target )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", target );
  return funcs->ext.p_glUnmapBufferARB( target );
}

static GLboolean WINAPI glUnmapNamedBuffer( GLuint buffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", buffer );
  return funcs->ext.p_glUnmapNamedBuffer( buffer );
}

static GLboolean WINAPI glUnmapNamedBufferEXT( GLuint buffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", buffer );
  return funcs->ext.p_glUnmapNamedBufferEXT( buffer );
}

static void WINAPI glUnmapObjectBufferATI( GLuint buffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", buffer );
  funcs->ext.p_glUnmapObjectBufferATI( buffer );
}

static void WINAPI glUnmapTexture2DINTEL( GLuint texture, GLint level )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", texture, level );
  funcs->ext.p_glUnmapTexture2DINTEL( texture, level );
}

static void WINAPI glUpdateObjectBufferATI( GLuint buffer, GLuint offset, GLsizei size, const void *pointer, GLenum preserve )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p, %d)\n", buffer, offset, size, pointer, preserve );
  funcs->ext.p_glUpdateObjectBufferATI( buffer, offset, size, pointer, preserve );
}

static void WINAPI glUseProgram( GLuint program )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", program );
  funcs->ext.p_glUseProgram( program );
}

static void WINAPI glUseProgramObjectARB( GLhandleARB programObj )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", programObj );
  funcs->ext.p_glUseProgramObjectARB( programObj );
}

static void WINAPI glUseProgramStages( GLuint pipeline, GLbitfield stages, GLuint program )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", pipeline, stages, program );
  funcs->ext.p_glUseProgramStages( pipeline, stages, program );
}

static void WINAPI glUseShaderProgramEXT( GLenum type, GLuint program )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", type, program );
  funcs->ext.p_glUseShaderProgramEXT( type, program );
}

static void WINAPI glVDPAUFiniNV(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  funcs->ext.p_glVDPAUFiniNV();
}

static void WINAPI glVDPAUGetSurfaceivNV( GLvdpauSurfaceNV surface, GLenum pname, GLsizei bufSize, GLsizei *length, GLint *values )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%ld, %d, %d, %p, %p)\n", surface, pname, bufSize, length, values );
  funcs->ext.p_glVDPAUGetSurfaceivNV( surface, pname, bufSize, length, values );
}

static void WINAPI glVDPAUInitNV( const void *vdpDevice, const void *getProcAddress )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p, %p)\n", vdpDevice, getProcAddress );
  funcs->ext.p_glVDPAUInitNV( vdpDevice, getProcAddress );
}

static GLboolean WINAPI glVDPAUIsSurfaceNV( GLvdpauSurfaceNV surface )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%ld)\n", surface );
  return funcs->ext.p_glVDPAUIsSurfaceNV( surface );
}

static void WINAPI glVDPAUMapSurfacesNV( GLsizei numSurfaces, const GLvdpauSurfaceNV *surfaces )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", numSurfaces, surfaces );
  funcs->ext.p_glVDPAUMapSurfacesNV( numSurfaces, surfaces );
}

static GLvdpauSurfaceNV WINAPI glVDPAURegisterOutputSurfaceNV( const void *vdpSurface, GLenum target, GLsizei numTextureNames, const GLuint *textureNames )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p, %d, %d, %p)\n", vdpSurface, target, numTextureNames, textureNames );
  return funcs->ext.p_glVDPAURegisterOutputSurfaceNV( vdpSurface, target, numTextureNames, textureNames );
}

static GLvdpauSurfaceNV WINAPI glVDPAURegisterVideoSurfaceNV( const void *vdpSurface, GLenum target, GLsizei numTextureNames, const GLuint *textureNames )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p, %d, %d, %p)\n", vdpSurface, target, numTextureNames, textureNames );
  return funcs->ext.p_glVDPAURegisterVideoSurfaceNV( vdpSurface, target, numTextureNames, textureNames );
}

static void WINAPI glVDPAUSurfaceAccessNV( GLvdpauSurfaceNV surface, GLenum access )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%ld, %d)\n", surface, access );
  funcs->ext.p_glVDPAUSurfaceAccessNV( surface, access );
}

static void WINAPI glVDPAUUnmapSurfacesNV( GLsizei numSurface, const GLvdpauSurfaceNV *surfaces )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", numSurface, surfaces );
  funcs->ext.p_glVDPAUUnmapSurfacesNV( numSurface, surfaces );
}

static void WINAPI glVDPAUUnregisterSurfaceNV( GLvdpauSurfaceNV surface )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%ld)\n", surface );
  funcs->ext.p_glVDPAUUnregisterSurfaceNV( surface );
}

static void WINAPI glValidateProgram( GLuint program )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", program );
  funcs->ext.p_glValidateProgram( program );
}

static void WINAPI glValidateProgramARB( GLhandleARB programObj )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", programObj );
  funcs->ext.p_glValidateProgramARB( programObj );
}

static void WINAPI glValidateProgramPipeline( GLuint pipeline )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", pipeline );
  funcs->ext.p_glValidateProgramPipeline( pipeline );
}

static void WINAPI glVariantArrayObjectATI( GLuint id, GLenum type, GLsizei stride, GLuint buffer, GLuint offset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", id, type, stride, buffer, offset );
  funcs->ext.p_glVariantArrayObjectATI( id, type, stride, buffer, offset );
}

static void WINAPI glVariantPointerEXT( GLuint id, GLenum type, GLuint stride, const void *addr )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", id, type, stride, addr );
  funcs->ext.p_glVariantPointerEXT( id, type, stride, addr );
}

static void WINAPI glVariantbvEXT( GLuint id, const GLbyte *addr )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", id, addr );
  funcs->ext.p_glVariantbvEXT( id, addr );
}

static void WINAPI glVariantdvEXT( GLuint id, const GLdouble *addr )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", id, addr );
  funcs->ext.p_glVariantdvEXT( id, addr );
}

static void WINAPI glVariantfvEXT( GLuint id, const GLfloat *addr )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", id, addr );
  funcs->ext.p_glVariantfvEXT( id, addr );
}

static void WINAPI glVariantivEXT( GLuint id, const GLint *addr )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", id, addr );
  funcs->ext.p_glVariantivEXT( id, addr );
}

static void WINAPI glVariantsvEXT( GLuint id, const GLshort *addr )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", id, addr );
  funcs->ext.p_glVariantsvEXT( id, addr );
}

static void WINAPI glVariantubvEXT( GLuint id, const GLubyte *addr )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", id, addr );
  funcs->ext.p_glVariantubvEXT( id, addr );
}

static void WINAPI glVariantuivEXT( GLuint id, const GLuint *addr )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", id, addr );
  funcs->ext.p_glVariantuivEXT( id, addr );
}

static void WINAPI glVariantusvEXT( GLuint id, const GLushort *addr )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", id, addr );
  funcs->ext.p_glVariantusvEXT( id, addr );
}

static void WINAPI glVertex2bOES( GLbyte x, GLbyte y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", x, y );
  funcs->ext.p_glVertex2bOES( x, y );
}

static void WINAPI glVertex2bvOES( const GLbyte *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", coords );
  funcs->ext.p_glVertex2bvOES( coords );
}

static void WINAPI glVertex2hNV( GLhalfNV x, GLhalfNV y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", x, y );
  funcs->ext.p_glVertex2hNV( x, y );
}

static void WINAPI glVertex2hvNV( const GLhalfNV *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glVertex2hvNV( v );
}

static void WINAPI glVertex2xOES( GLfixed x )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", x );
  funcs->ext.p_glVertex2xOES( x );
}

static void WINAPI glVertex2xvOES( const GLfixed *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", coords );
  funcs->ext.p_glVertex2xvOES( coords );
}

static void WINAPI glVertex3bOES( GLbyte x, GLbyte y, GLbyte z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", x, y, z );
  funcs->ext.p_glVertex3bOES( x, y, z );
}

static void WINAPI glVertex3bvOES( const GLbyte *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", coords );
  funcs->ext.p_glVertex3bvOES( coords );
}

static void WINAPI glVertex3hNV( GLhalfNV x, GLhalfNV y, GLhalfNV z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", x, y, z );
  funcs->ext.p_glVertex3hNV( x, y, z );
}

static void WINAPI glVertex3hvNV( const GLhalfNV *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glVertex3hvNV( v );
}

static void WINAPI glVertex3xOES( GLfixed x, GLfixed y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", x, y );
  funcs->ext.p_glVertex3xOES( x, y );
}

static void WINAPI glVertex3xvOES( const GLfixed *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", coords );
  funcs->ext.p_glVertex3xvOES( coords );
}

static void WINAPI glVertex4bOES( GLbyte x, GLbyte y, GLbyte z, GLbyte w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", x, y, z, w );
  funcs->ext.p_glVertex4bOES( x, y, z, w );
}

static void WINAPI glVertex4bvOES( const GLbyte *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", coords );
  funcs->ext.p_glVertex4bvOES( coords );
}

static void WINAPI glVertex4hNV( GLhalfNV x, GLhalfNV y, GLhalfNV z, GLhalfNV w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", x, y, z, w );
  funcs->ext.p_glVertex4hNV( x, y, z, w );
}

static void WINAPI glVertex4hvNV( const GLhalfNV *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glVertex4hvNV( v );
}

static void WINAPI glVertex4xOES( GLfixed x, GLfixed y, GLfixed z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", x, y, z );
  funcs->ext.p_glVertex4xOES( x, y, z );
}

static void WINAPI glVertex4xvOES( const GLfixed *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", coords );
  funcs->ext.p_glVertex4xvOES( coords );
}

static void WINAPI glVertexArrayAttribBinding( GLuint vaobj, GLuint attribindex, GLuint bindingindex )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", vaobj, attribindex, bindingindex );
  funcs->ext.p_glVertexArrayAttribBinding( vaobj, attribindex, bindingindex );
}

static void WINAPI glVertexArrayAttribFormat( GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d)\n", vaobj, attribindex, size, type, normalized, relativeoffset );
  funcs->ext.p_glVertexArrayAttribFormat( vaobj, attribindex, size, type, normalized, relativeoffset );
}

static void WINAPI glVertexArrayAttribIFormat( GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", vaobj, attribindex, size, type, relativeoffset );
  funcs->ext.p_glVertexArrayAttribIFormat( vaobj, attribindex, size, type, relativeoffset );
}

static void WINAPI glVertexArrayAttribLFormat( GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", vaobj, attribindex, size, type, relativeoffset );
  funcs->ext.p_glVertexArrayAttribLFormat( vaobj, attribindex, size, type, relativeoffset );
}

static void WINAPI glVertexArrayBindVertexBufferEXT( GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %ld, %d)\n", vaobj, bindingindex, buffer, offset, stride );
  funcs->ext.p_glVertexArrayBindVertexBufferEXT( vaobj, bindingindex, buffer, offset, stride );
}

static void WINAPI glVertexArrayBindingDivisor( GLuint vaobj, GLuint bindingindex, GLuint divisor )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", vaobj, bindingindex, divisor );
  funcs->ext.p_glVertexArrayBindingDivisor( vaobj, bindingindex, divisor );
}

static void WINAPI glVertexArrayColorOffsetEXT( GLuint vaobj, GLuint buffer, GLint size, GLenum type, GLsizei stride, GLintptr offset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %ld)\n", vaobj, buffer, size, type, stride, offset );
  funcs->ext.p_glVertexArrayColorOffsetEXT( vaobj, buffer, size, type, stride, offset );
}

static void WINAPI glVertexArrayEdgeFlagOffsetEXT( GLuint vaobj, GLuint buffer, GLsizei stride, GLintptr offset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %ld)\n", vaobj, buffer, stride, offset );
  funcs->ext.p_glVertexArrayEdgeFlagOffsetEXT( vaobj, buffer, stride, offset );
}

static void WINAPI glVertexArrayElementBuffer( GLuint vaobj, GLuint buffer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", vaobj, buffer );
  funcs->ext.p_glVertexArrayElementBuffer( vaobj, buffer );
}

static void WINAPI glVertexArrayFogCoordOffsetEXT( GLuint vaobj, GLuint buffer, GLenum type, GLsizei stride, GLintptr offset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %ld)\n", vaobj, buffer, type, stride, offset );
  funcs->ext.p_glVertexArrayFogCoordOffsetEXT( vaobj, buffer, type, stride, offset );
}

static void WINAPI glVertexArrayIndexOffsetEXT( GLuint vaobj, GLuint buffer, GLenum type, GLsizei stride, GLintptr offset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %ld)\n", vaobj, buffer, type, stride, offset );
  funcs->ext.p_glVertexArrayIndexOffsetEXT( vaobj, buffer, type, stride, offset );
}

static void WINAPI glVertexArrayMultiTexCoordOffsetEXT( GLuint vaobj, GLuint buffer, GLenum texunit, GLint size, GLenum type, GLsizei stride, GLintptr offset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %ld)\n", vaobj, buffer, texunit, size, type, stride, offset );
  funcs->ext.p_glVertexArrayMultiTexCoordOffsetEXT( vaobj, buffer, texunit, size, type, stride, offset );
}

static void WINAPI glVertexArrayNormalOffsetEXT( GLuint vaobj, GLuint buffer, GLenum type, GLsizei stride, GLintptr offset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %ld)\n", vaobj, buffer, type, stride, offset );
  funcs->ext.p_glVertexArrayNormalOffsetEXT( vaobj, buffer, type, stride, offset );
}

static void WINAPI glVertexArrayParameteriAPPLE( GLenum pname, GLint param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", pname, param );
  funcs->ext.p_glVertexArrayParameteriAPPLE( pname, param );
}

static void WINAPI glVertexArrayRangeAPPLE( GLsizei length, void *pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", length, pointer );
  funcs->ext.p_glVertexArrayRangeAPPLE( length, pointer );
}

static void WINAPI glVertexArrayRangeNV( GLsizei length, const void *pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", length, pointer );
  funcs->ext.p_glVertexArrayRangeNV( length, pointer );
}

static void WINAPI glVertexArraySecondaryColorOffsetEXT( GLuint vaobj, GLuint buffer, GLint size, GLenum type, GLsizei stride, GLintptr offset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %ld)\n", vaobj, buffer, size, type, stride, offset );
  funcs->ext.p_glVertexArraySecondaryColorOffsetEXT( vaobj, buffer, size, type, stride, offset );
}

static void WINAPI glVertexArrayTexCoordOffsetEXT( GLuint vaobj, GLuint buffer, GLint size, GLenum type, GLsizei stride, GLintptr offset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %ld)\n", vaobj, buffer, size, type, stride, offset );
  funcs->ext.p_glVertexArrayTexCoordOffsetEXT( vaobj, buffer, size, type, stride, offset );
}

static void WINAPI glVertexArrayVertexAttribBindingEXT( GLuint vaobj, GLuint attribindex, GLuint bindingindex )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", vaobj, attribindex, bindingindex );
  funcs->ext.p_glVertexArrayVertexAttribBindingEXT( vaobj, attribindex, bindingindex );
}

static void WINAPI glVertexArrayVertexAttribDivisorEXT( GLuint vaobj, GLuint index, GLuint divisor )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", vaobj, index, divisor );
  funcs->ext.p_glVertexArrayVertexAttribDivisorEXT( vaobj, index, divisor );
}

static void WINAPI glVertexArrayVertexAttribFormatEXT( GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d)\n", vaobj, attribindex, size, type, normalized, relativeoffset );
  funcs->ext.p_glVertexArrayVertexAttribFormatEXT( vaobj, attribindex, size, type, normalized, relativeoffset );
}

static void WINAPI glVertexArrayVertexAttribIFormatEXT( GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", vaobj, attribindex, size, type, relativeoffset );
  funcs->ext.p_glVertexArrayVertexAttribIFormatEXT( vaobj, attribindex, size, type, relativeoffset );
}

static void WINAPI glVertexArrayVertexAttribIOffsetEXT( GLuint vaobj, GLuint buffer, GLuint index, GLint size, GLenum type, GLsizei stride, GLintptr offset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %ld)\n", vaobj, buffer, index, size, type, stride, offset );
  funcs->ext.p_glVertexArrayVertexAttribIOffsetEXT( vaobj, buffer, index, size, type, stride, offset );
}

static void WINAPI glVertexArrayVertexAttribLFormatEXT( GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", vaobj, attribindex, size, type, relativeoffset );
  funcs->ext.p_glVertexArrayVertexAttribLFormatEXT( vaobj, attribindex, size, type, relativeoffset );
}

static void WINAPI glVertexArrayVertexAttribLOffsetEXT( GLuint vaobj, GLuint buffer, GLuint index, GLint size, GLenum type, GLsizei stride, GLintptr offset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %ld)\n", vaobj, buffer, index, size, type, stride, offset );
  funcs->ext.p_glVertexArrayVertexAttribLOffsetEXT( vaobj, buffer, index, size, type, stride, offset );
}

static void WINAPI glVertexArrayVertexAttribOffsetEXT( GLuint vaobj, GLuint buffer, GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, GLintptr offset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d, %ld)\n", vaobj, buffer, index, size, type, normalized, stride, offset );
  funcs->ext.p_glVertexArrayVertexAttribOffsetEXT( vaobj, buffer, index, size, type, normalized, stride, offset );
}

static void WINAPI glVertexArrayVertexBindingDivisorEXT( GLuint vaobj, GLuint bindingindex, GLuint divisor )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", vaobj, bindingindex, divisor );
  funcs->ext.p_glVertexArrayVertexBindingDivisorEXT( vaobj, bindingindex, divisor );
}

static void WINAPI glVertexArrayVertexBuffer( GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %ld, %d)\n", vaobj, bindingindex, buffer, offset, stride );
  funcs->ext.p_glVertexArrayVertexBuffer( vaobj, bindingindex, buffer, offset, stride );
}

static void WINAPI glVertexArrayVertexBuffers( GLuint vaobj, GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizei *strides )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p, %p, %p)\n", vaobj, first, count, buffers, offsets, strides );
  funcs->ext.p_glVertexArrayVertexBuffers( vaobj, first, count, buffers, offsets, strides );
}

static void WINAPI glVertexArrayVertexOffsetEXT( GLuint vaobj, GLuint buffer, GLint size, GLenum type, GLsizei stride, GLintptr offset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %ld)\n", vaobj, buffer, size, type, stride, offset );
  funcs->ext.p_glVertexArrayVertexOffsetEXT( vaobj, buffer, size, type, stride, offset );
}

static void WINAPI glVertexAttrib1d( GLuint index, GLdouble x )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f)\n", index, x );
  funcs->ext.p_glVertexAttrib1d( index, x );
}

static void WINAPI glVertexAttrib1dARB( GLuint index, GLdouble x )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f)\n", index, x );
  funcs->ext.p_glVertexAttrib1dARB( index, x );
}

static void WINAPI glVertexAttrib1dNV( GLuint index, GLdouble x )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f)\n", index, x );
  funcs->ext.p_glVertexAttrib1dNV( index, x );
}

static void WINAPI glVertexAttrib1dv( GLuint index, const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib1dv( index, v );
}

static void WINAPI glVertexAttrib1dvARB( GLuint index, const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib1dvARB( index, v );
}

static void WINAPI glVertexAttrib1dvNV( GLuint index, const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib1dvNV( index, v );
}

static void WINAPI glVertexAttrib1f( GLuint index, GLfloat x )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f)\n", index, x );
  funcs->ext.p_glVertexAttrib1f( index, x );
}

static void WINAPI glVertexAttrib1fARB( GLuint index, GLfloat x )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f)\n", index, x );
  funcs->ext.p_glVertexAttrib1fARB( index, x );
}

static void WINAPI glVertexAttrib1fNV( GLuint index, GLfloat x )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f)\n", index, x );
  funcs->ext.p_glVertexAttrib1fNV( index, x );
}

static void WINAPI glVertexAttrib1fv( GLuint index, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib1fv( index, v );
}

static void WINAPI glVertexAttrib1fvARB( GLuint index, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib1fvARB( index, v );
}

static void WINAPI glVertexAttrib1fvNV( GLuint index, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib1fvNV( index, v );
}

static void WINAPI glVertexAttrib1hNV( GLuint index, GLhalfNV x )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", index, x );
  funcs->ext.p_glVertexAttrib1hNV( index, x );
}

static void WINAPI glVertexAttrib1hvNV( GLuint index, const GLhalfNV *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib1hvNV( index, v );
}

static void WINAPI glVertexAttrib1s( GLuint index, GLshort x )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", index, x );
  funcs->ext.p_glVertexAttrib1s( index, x );
}

static void WINAPI glVertexAttrib1sARB( GLuint index, GLshort x )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", index, x );
  funcs->ext.p_glVertexAttrib1sARB( index, x );
}

static void WINAPI glVertexAttrib1sNV( GLuint index, GLshort x )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", index, x );
  funcs->ext.p_glVertexAttrib1sNV( index, x );
}

static void WINAPI glVertexAttrib1sv( GLuint index, const GLshort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib1sv( index, v );
}

static void WINAPI glVertexAttrib1svARB( GLuint index, const GLshort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib1svARB( index, v );
}

static void WINAPI glVertexAttrib1svNV( GLuint index, const GLshort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib1svNV( index, v );
}

static void WINAPI glVertexAttrib2d( GLuint index, GLdouble x, GLdouble y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f)\n", index, x, y );
  funcs->ext.p_glVertexAttrib2d( index, x, y );
}

static void WINAPI glVertexAttrib2dARB( GLuint index, GLdouble x, GLdouble y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f)\n", index, x, y );
  funcs->ext.p_glVertexAttrib2dARB( index, x, y );
}

static void WINAPI glVertexAttrib2dNV( GLuint index, GLdouble x, GLdouble y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f)\n", index, x, y );
  funcs->ext.p_glVertexAttrib2dNV( index, x, y );
}

static void WINAPI glVertexAttrib2dv( GLuint index, const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib2dv( index, v );
}

static void WINAPI glVertexAttrib2dvARB( GLuint index, const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib2dvARB( index, v );
}

static void WINAPI glVertexAttrib2dvNV( GLuint index, const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib2dvNV( index, v );
}

static void WINAPI glVertexAttrib2f( GLuint index, GLfloat x, GLfloat y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f)\n", index, x, y );
  funcs->ext.p_glVertexAttrib2f( index, x, y );
}

static void WINAPI glVertexAttrib2fARB( GLuint index, GLfloat x, GLfloat y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f)\n", index, x, y );
  funcs->ext.p_glVertexAttrib2fARB( index, x, y );
}

static void WINAPI glVertexAttrib2fNV( GLuint index, GLfloat x, GLfloat y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f)\n", index, x, y );
  funcs->ext.p_glVertexAttrib2fNV( index, x, y );
}

static void WINAPI glVertexAttrib2fv( GLuint index, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib2fv( index, v );
}

static void WINAPI glVertexAttrib2fvARB( GLuint index, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib2fvARB( index, v );
}

static void WINAPI glVertexAttrib2fvNV( GLuint index, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib2fvNV( index, v );
}

static void WINAPI glVertexAttrib2hNV( GLuint index, GLhalfNV x, GLhalfNV y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", index, x, y );
  funcs->ext.p_glVertexAttrib2hNV( index, x, y );
}

static void WINAPI glVertexAttrib2hvNV( GLuint index, const GLhalfNV *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib2hvNV( index, v );
}

static void WINAPI glVertexAttrib2s( GLuint index, GLshort x, GLshort y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", index, x, y );
  funcs->ext.p_glVertexAttrib2s( index, x, y );
}

static void WINAPI glVertexAttrib2sARB( GLuint index, GLshort x, GLshort y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", index, x, y );
  funcs->ext.p_glVertexAttrib2sARB( index, x, y );
}

static void WINAPI glVertexAttrib2sNV( GLuint index, GLshort x, GLshort y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", index, x, y );
  funcs->ext.p_glVertexAttrib2sNV( index, x, y );
}

static void WINAPI glVertexAttrib2sv( GLuint index, const GLshort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib2sv( index, v );
}

static void WINAPI glVertexAttrib2svARB( GLuint index, const GLshort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib2svARB( index, v );
}

static void WINAPI glVertexAttrib2svNV( GLuint index, const GLshort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib2svNV( index, v );
}

static void WINAPI glVertexAttrib3d( GLuint index, GLdouble x, GLdouble y, GLdouble z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f)\n", index, x, y, z );
  funcs->ext.p_glVertexAttrib3d( index, x, y, z );
}

static void WINAPI glVertexAttrib3dARB( GLuint index, GLdouble x, GLdouble y, GLdouble z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f)\n", index, x, y, z );
  funcs->ext.p_glVertexAttrib3dARB( index, x, y, z );
}

static void WINAPI glVertexAttrib3dNV( GLuint index, GLdouble x, GLdouble y, GLdouble z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f)\n", index, x, y, z );
  funcs->ext.p_glVertexAttrib3dNV( index, x, y, z );
}

static void WINAPI glVertexAttrib3dv( GLuint index, const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib3dv( index, v );
}

static void WINAPI glVertexAttrib3dvARB( GLuint index, const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib3dvARB( index, v );
}

static void WINAPI glVertexAttrib3dvNV( GLuint index, const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib3dvNV( index, v );
}

static void WINAPI glVertexAttrib3f( GLuint index, GLfloat x, GLfloat y, GLfloat z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f)\n", index, x, y, z );
  funcs->ext.p_glVertexAttrib3f( index, x, y, z );
}

static void WINAPI glVertexAttrib3fARB( GLuint index, GLfloat x, GLfloat y, GLfloat z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f)\n", index, x, y, z );
  funcs->ext.p_glVertexAttrib3fARB( index, x, y, z );
}

static void WINAPI glVertexAttrib3fNV( GLuint index, GLfloat x, GLfloat y, GLfloat z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f)\n", index, x, y, z );
  funcs->ext.p_glVertexAttrib3fNV( index, x, y, z );
}

static void WINAPI glVertexAttrib3fv( GLuint index, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib3fv( index, v );
}

static void WINAPI glVertexAttrib3fvARB( GLuint index, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib3fvARB( index, v );
}

static void WINAPI glVertexAttrib3fvNV( GLuint index, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib3fvNV( index, v );
}

static void WINAPI glVertexAttrib3hNV( GLuint index, GLhalfNV x, GLhalfNV y, GLhalfNV z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", index, x, y, z );
  funcs->ext.p_glVertexAttrib3hNV( index, x, y, z );
}

static void WINAPI glVertexAttrib3hvNV( GLuint index, const GLhalfNV *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib3hvNV( index, v );
}

static void WINAPI glVertexAttrib3s( GLuint index, GLshort x, GLshort y, GLshort z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", index, x, y, z );
  funcs->ext.p_glVertexAttrib3s( index, x, y, z );
}

static void WINAPI glVertexAttrib3sARB( GLuint index, GLshort x, GLshort y, GLshort z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", index, x, y, z );
  funcs->ext.p_glVertexAttrib3sARB( index, x, y, z );
}

static void WINAPI glVertexAttrib3sNV( GLuint index, GLshort x, GLshort y, GLshort z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", index, x, y, z );
  funcs->ext.p_glVertexAttrib3sNV( index, x, y, z );
}

static void WINAPI glVertexAttrib3sv( GLuint index, const GLshort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib3sv( index, v );
}

static void WINAPI glVertexAttrib3svARB( GLuint index, const GLshort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib3svARB( index, v );
}

static void WINAPI glVertexAttrib3svNV( GLuint index, const GLshort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib3svNV( index, v );
}

static void WINAPI glVertexAttrib4Nbv( GLuint index, const GLbyte *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4Nbv( index, v );
}

static void WINAPI glVertexAttrib4NbvARB( GLuint index, const GLbyte *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4NbvARB( index, v );
}

static void WINAPI glVertexAttrib4Niv( GLuint index, const GLint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4Niv( index, v );
}

static void WINAPI glVertexAttrib4NivARB( GLuint index, const GLint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4NivARB( index, v );
}

static void WINAPI glVertexAttrib4Nsv( GLuint index, const GLshort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4Nsv( index, v );
}

static void WINAPI glVertexAttrib4NsvARB( GLuint index, const GLshort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4NsvARB( index, v );
}

static void WINAPI glVertexAttrib4Nub( GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  funcs->ext.p_glVertexAttrib4Nub( index, x, y, z, w );
}

static void WINAPI glVertexAttrib4NubARB( GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  funcs->ext.p_glVertexAttrib4NubARB( index, x, y, z, w );
}

static void WINAPI glVertexAttrib4Nubv( GLuint index, const GLubyte *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4Nubv( index, v );
}

static void WINAPI glVertexAttrib4NubvARB( GLuint index, const GLubyte *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4NubvARB( index, v );
}

static void WINAPI glVertexAttrib4Nuiv( GLuint index, const GLuint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4Nuiv( index, v );
}

static void WINAPI glVertexAttrib4NuivARB( GLuint index, const GLuint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4NuivARB( index, v );
}

static void WINAPI glVertexAttrib4Nusv( GLuint index, const GLushort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4Nusv( index, v );
}

static void WINAPI glVertexAttrib4NusvARB( GLuint index, const GLushort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4NusvARB( index, v );
}

static void WINAPI glVertexAttrib4bv( GLuint index, const GLbyte *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4bv( index, v );
}

static void WINAPI glVertexAttrib4bvARB( GLuint index, const GLbyte *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4bvARB( index, v );
}

static void WINAPI glVertexAttrib4d( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f, %f)\n", index, x, y, z, w );
  funcs->ext.p_glVertexAttrib4d( index, x, y, z, w );
}

static void WINAPI glVertexAttrib4dARB( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f, %f)\n", index, x, y, z, w );
  funcs->ext.p_glVertexAttrib4dARB( index, x, y, z, w );
}

static void WINAPI glVertexAttrib4dNV( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f, %f)\n", index, x, y, z, w );
  funcs->ext.p_glVertexAttrib4dNV( index, x, y, z, w );
}

static void WINAPI glVertexAttrib4dv( GLuint index, const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4dv( index, v );
}

static void WINAPI glVertexAttrib4dvARB( GLuint index, const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4dvARB( index, v );
}

static void WINAPI glVertexAttrib4dvNV( GLuint index, const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4dvNV( index, v );
}

static void WINAPI glVertexAttrib4f( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f, %f)\n", index, x, y, z, w );
  funcs->ext.p_glVertexAttrib4f( index, x, y, z, w );
}

static void WINAPI glVertexAttrib4fARB( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f, %f)\n", index, x, y, z, w );
  funcs->ext.p_glVertexAttrib4fARB( index, x, y, z, w );
}

static void WINAPI glVertexAttrib4fNV( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f, %f)\n", index, x, y, z, w );
  funcs->ext.p_glVertexAttrib4fNV( index, x, y, z, w );
}

static void WINAPI glVertexAttrib4fv( GLuint index, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4fv( index, v );
}

static void WINAPI glVertexAttrib4fvARB( GLuint index, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4fvARB( index, v );
}

static void WINAPI glVertexAttrib4fvNV( GLuint index, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4fvNV( index, v );
}

static void WINAPI glVertexAttrib4hNV( GLuint index, GLhalfNV x, GLhalfNV y, GLhalfNV z, GLhalfNV w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  funcs->ext.p_glVertexAttrib4hNV( index, x, y, z, w );
}

static void WINAPI glVertexAttrib4hvNV( GLuint index, const GLhalfNV *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4hvNV( index, v );
}

static void WINAPI glVertexAttrib4iv( GLuint index, const GLint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4iv( index, v );
}

static void WINAPI glVertexAttrib4ivARB( GLuint index, const GLint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4ivARB( index, v );
}

static void WINAPI glVertexAttrib4s( GLuint index, GLshort x, GLshort y, GLshort z, GLshort w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  funcs->ext.p_glVertexAttrib4s( index, x, y, z, w );
}

static void WINAPI glVertexAttrib4sARB( GLuint index, GLshort x, GLshort y, GLshort z, GLshort w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  funcs->ext.p_glVertexAttrib4sARB( index, x, y, z, w );
}

static void WINAPI glVertexAttrib4sNV( GLuint index, GLshort x, GLshort y, GLshort z, GLshort w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  funcs->ext.p_glVertexAttrib4sNV( index, x, y, z, w );
}

static void WINAPI glVertexAttrib4sv( GLuint index, const GLshort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4sv( index, v );
}

static void WINAPI glVertexAttrib4svARB( GLuint index, const GLshort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4svARB( index, v );
}

static void WINAPI glVertexAttrib4svNV( GLuint index, const GLshort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4svNV( index, v );
}

static void WINAPI glVertexAttrib4ubNV( GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  funcs->ext.p_glVertexAttrib4ubNV( index, x, y, z, w );
}

static void WINAPI glVertexAttrib4ubv( GLuint index, const GLubyte *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4ubv( index, v );
}

static void WINAPI glVertexAttrib4ubvARB( GLuint index, const GLubyte *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4ubvARB( index, v );
}

static void WINAPI glVertexAttrib4ubvNV( GLuint index, const GLubyte *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4ubvNV( index, v );
}

static void WINAPI glVertexAttrib4uiv( GLuint index, const GLuint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4uiv( index, v );
}

static void WINAPI glVertexAttrib4uivARB( GLuint index, const GLuint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4uivARB( index, v );
}

static void WINAPI glVertexAttrib4usv( GLuint index, const GLushort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4usv( index, v );
}

static void WINAPI glVertexAttrib4usvARB( GLuint index, const GLushort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttrib4usvARB( index, v );
}

static void WINAPI glVertexAttribArrayObjectATI( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, GLuint buffer, GLuint offset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d, %d)\n", index, size, type, normalized, stride, buffer, offset );
  funcs->ext.p_glVertexAttribArrayObjectATI( index, size, type, normalized, stride, buffer, offset );
}

static void WINAPI glVertexAttribBinding( GLuint attribindex, GLuint bindingindex )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", attribindex, bindingindex );
  funcs->ext.p_glVertexAttribBinding( attribindex, bindingindex );
}

static void WINAPI glVertexAttribDivisor( GLuint index, GLuint divisor )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", index, divisor );
  funcs->ext.p_glVertexAttribDivisor( index, divisor );
}

static void WINAPI glVertexAttribDivisorARB( GLuint index, GLuint divisor )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", index, divisor );
  funcs->ext.p_glVertexAttribDivisorARB( index, divisor );
}

static void WINAPI glVertexAttribFormat( GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", attribindex, size, type, normalized, relativeoffset );
  funcs->ext.p_glVertexAttribFormat( attribindex, size, type, normalized, relativeoffset );
}

static void WINAPI glVertexAttribFormatNV( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", index, size, type, normalized, stride );
  funcs->ext.p_glVertexAttribFormatNV( index, size, type, normalized, stride );
}

static void WINAPI glVertexAttribI1i( GLuint index, GLint x )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", index, x );
  funcs->ext.p_glVertexAttribI1i( index, x );
}

static void WINAPI glVertexAttribI1iEXT( GLuint index, GLint x )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", index, x );
  funcs->ext.p_glVertexAttribI1iEXT( index, x );
}

static void WINAPI glVertexAttribI1iv( GLuint index, const GLint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI1iv( index, v );
}

static void WINAPI glVertexAttribI1ivEXT( GLuint index, const GLint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI1ivEXT( index, v );
}

static void WINAPI glVertexAttribI1ui( GLuint index, GLuint x )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", index, x );
  funcs->ext.p_glVertexAttribI1ui( index, x );
}

static void WINAPI glVertexAttribI1uiEXT( GLuint index, GLuint x )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", index, x );
  funcs->ext.p_glVertexAttribI1uiEXT( index, x );
}

static void WINAPI glVertexAttribI1uiv( GLuint index, const GLuint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI1uiv( index, v );
}

static void WINAPI glVertexAttribI1uivEXT( GLuint index, const GLuint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI1uivEXT( index, v );
}

static void WINAPI glVertexAttribI2i( GLuint index, GLint x, GLint y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", index, x, y );
  funcs->ext.p_glVertexAttribI2i( index, x, y );
}

static void WINAPI glVertexAttribI2iEXT( GLuint index, GLint x, GLint y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", index, x, y );
  funcs->ext.p_glVertexAttribI2iEXT( index, x, y );
}

static void WINAPI glVertexAttribI2iv( GLuint index, const GLint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI2iv( index, v );
}

static void WINAPI glVertexAttribI2ivEXT( GLuint index, const GLint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI2ivEXT( index, v );
}

static void WINAPI glVertexAttribI2ui( GLuint index, GLuint x, GLuint y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", index, x, y );
  funcs->ext.p_glVertexAttribI2ui( index, x, y );
}

static void WINAPI glVertexAttribI2uiEXT( GLuint index, GLuint x, GLuint y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", index, x, y );
  funcs->ext.p_glVertexAttribI2uiEXT( index, x, y );
}

static void WINAPI glVertexAttribI2uiv( GLuint index, const GLuint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI2uiv( index, v );
}

static void WINAPI glVertexAttribI2uivEXT( GLuint index, const GLuint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI2uivEXT( index, v );
}

static void WINAPI glVertexAttribI3i( GLuint index, GLint x, GLint y, GLint z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", index, x, y, z );
  funcs->ext.p_glVertexAttribI3i( index, x, y, z );
}

static void WINAPI glVertexAttribI3iEXT( GLuint index, GLint x, GLint y, GLint z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", index, x, y, z );
  funcs->ext.p_glVertexAttribI3iEXT( index, x, y, z );
}

static void WINAPI glVertexAttribI3iv( GLuint index, const GLint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI3iv( index, v );
}

static void WINAPI glVertexAttribI3ivEXT( GLuint index, const GLint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI3ivEXT( index, v );
}

static void WINAPI glVertexAttribI3ui( GLuint index, GLuint x, GLuint y, GLuint z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", index, x, y, z );
  funcs->ext.p_glVertexAttribI3ui( index, x, y, z );
}

static void WINAPI glVertexAttribI3uiEXT( GLuint index, GLuint x, GLuint y, GLuint z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", index, x, y, z );
  funcs->ext.p_glVertexAttribI3uiEXT( index, x, y, z );
}

static void WINAPI glVertexAttribI3uiv( GLuint index, const GLuint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI3uiv( index, v );
}

static void WINAPI glVertexAttribI3uivEXT( GLuint index, const GLuint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI3uivEXT( index, v );
}

static void WINAPI glVertexAttribI4bv( GLuint index, const GLbyte *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI4bv( index, v );
}

static void WINAPI glVertexAttribI4bvEXT( GLuint index, const GLbyte *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI4bvEXT( index, v );
}

static void WINAPI glVertexAttribI4i( GLuint index, GLint x, GLint y, GLint z, GLint w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  funcs->ext.p_glVertexAttribI4i( index, x, y, z, w );
}

static void WINAPI glVertexAttribI4iEXT( GLuint index, GLint x, GLint y, GLint z, GLint w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  funcs->ext.p_glVertexAttribI4iEXT( index, x, y, z, w );
}

static void WINAPI glVertexAttribI4iv( GLuint index, const GLint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI4iv( index, v );
}

static void WINAPI glVertexAttribI4ivEXT( GLuint index, const GLint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI4ivEXT( index, v );
}

static void WINAPI glVertexAttribI4sv( GLuint index, const GLshort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI4sv( index, v );
}

static void WINAPI glVertexAttribI4svEXT( GLuint index, const GLshort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI4svEXT( index, v );
}

static void WINAPI glVertexAttribI4ubv( GLuint index, const GLubyte *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI4ubv( index, v );
}

static void WINAPI glVertexAttribI4ubvEXT( GLuint index, const GLubyte *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI4ubvEXT( index, v );
}

static void WINAPI glVertexAttribI4ui( GLuint index, GLuint x, GLuint y, GLuint z, GLuint w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  funcs->ext.p_glVertexAttribI4ui( index, x, y, z, w );
}

static void WINAPI glVertexAttribI4uiEXT( GLuint index, GLuint x, GLuint y, GLuint z, GLuint w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  funcs->ext.p_glVertexAttribI4uiEXT( index, x, y, z, w );
}

static void WINAPI glVertexAttribI4uiv( GLuint index, const GLuint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI4uiv( index, v );
}

static void WINAPI glVertexAttribI4uivEXT( GLuint index, const GLuint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI4uivEXT( index, v );
}

static void WINAPI glVertexAttribI4usv( GLuint index, const GLushort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI4usv( index, v );
}

static void WINAPI glVertexAttribI4usvEXT( GLuint index, const GLushort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribI4usvEXT( index, v );
}

static void WINAPI glVertexAttribIFormat( GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", attribindex, size, type, relativeoffset );
  funcs->ext.p_glVertexAttribIFormat( attribindex, size, type, relativeoffset );
}

static void WINAPI glVertexAttribIFormatNV( GLuint index, GLint size, GLenum type, GLsizei stride )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", index, size, type, stride );
  funcs->ext.p_glVertexAttribIFormatNV( index, size, type, stride );
}

static void WINAPI glVertexAttribIPointer( GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", index, size, type, stride, pointer );
  funcs->ext.p_glVertexAttribIPointer( index, size, type, stride, pointer );
}

static void WINAPI glVertexAttribIPointerEXT( GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", index, size, type, stride, pointer );
  funcs->ext.p_glVertexAttribIPointerEXT( index, size, type, stride, pointer );
}

static void WINAPI glVertexAttribL1d( GLuint index, GLdouble x )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f)\n", index, x );
  funcs->ext.p_glVertexAttribL1d( index, x );
}

static void WINAPI glVertexAttribL1dEXT( GLuint index, GLdouble x )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f)\n", index, x );
  funcs->ext.p_glVertexAttribL1dEXT( index, x );
}

static void WINAPI glVertexAttribL1dv( GLuint index, const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribL1dv( index, v );
}

static void WINAPI glVertexAttribL1dvEXT( GLuint index, const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribL1dvEXT( index, v );
}

static void WINAPI glVertexAttribL1i64NV( GLuint index, GLint64EXT x )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %s)\n", index, wine_dbgstr_longlong(x) );
  funcs->ext.p_glVertexAttribL1i64NV( index, x );
}

static void WINAPI glVertexAttribL1i64vNV( GLuint index, const GLint64EXT *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribL1i64vNV( index, v );
}

static void WINAPI glVertexAttribL1ui64ARB( GLuint index, GLuint64EXT x )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %s)\n", index, wine_dbgstr_longlong(x) );
  funcs->ext.p_glVertexAttribL1ui64ARB( index, x );
}

static void WINAPI glVertexAttribL1ui64NV( GLuint index, GLuint64EXT x )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %s)\n", index, wine_dbgstr_longlong(x) );
  funcs->ext.p_glVertexAttribL1ui64NV( index, x );
}

static void WINAPI glVertexAttribL1ui64vARB( GLuint index, const GLuint64EXT *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribL1ui64vARB( index, v );
}

static void WINAPI glVertexAttribL1ui64vNV( GLuint index, const GLuint64EXT *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribL1ui64vNV( index, v );
}

static void WINAPI glVertexAttribL2d( GLuint index, GLdouble x, GLdouble y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f)\n", index, x, y );
  funcs->ext.p_glVertexAttribL2d( index, x, y );
}

static void WINAPI glVertexAttribL2dEXT( GLuint index, GLdouble x, GLdouble y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f)\n", index, x, y );
  funcs->ext.p_glVertexAttribL2dEXT( index, x, y );
}

static void WINAPI glVertexAttribL2dv( GLuint index, const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribL2dv( index, v );
}

static void WINAPI glVertexAttribL2dvEXT( GLuint index, const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribL2dvEXT( index, v );
}

static void WINAPI glVertexAttribL2i64NV( GLuint index, GLint64EXT x, GLint64EXT y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %s, %s)\n", index, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y) );
  funcs->ext.p_glVertexAttribL2i64NV( index, x, y );
}

static void WINAPI glVertexAttribL2i64vNV( GLuint index, const GLint64EXT *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribL2i64vNV( index, v );
}

static void WINAPI glVertexAttribL2ui64NV( GLuint index, GLuint64EXT x, GLuint64EXT y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %s, %s)\n", index, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y) );
  funcs->ext.p_glVertexAttribL2ui64NV( index, x, y );
}

static void WINAPI glVertexAttribL2ui64vNV( GLuint index, const GLuint64EXT *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribL2ui64vNV( index, v );
}

static void WINAPI glVertexAttribL3d( GLuint index, GLdouble x, GLdouble y, GLdouble z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f)\n", index, x, y, z );
  funcs->ext.p_glVertexAttribL3d( index, x, y, z );
}

static void WINAPI glVertexAttribL3dEXT( GLuint index, GLdouble x, GLdouble y, GLdouble z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f)\n", index, x, y, z );
  funcs->ext.p_glVertexAttribL3dEXT( index, x, y, z );
}

static void WINAPI glVertexAttribL3dv( GLuint index, const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribL3dv( index, v );
}

static void WINAPI glVertexAttribL3dvEXT( GLuint index, const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribL3dvEXT( index, v );
}

static void WINAPI glVertexAttribL3i64NV( GLuint index, GLint64EXT x, GLint64EXT y, GLint64EXT z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %s, %s, %s)\n", index, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z) );
  funcs->ext.p_glVertexAttribL3i64NV( index, x, y, z );
}

static void WINAPI glVertexAttribL3i64vNV( GLuint index, const GLint64EXT *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribL3i64vNV( index, v );
}

static void WINAPI glVertexAttribL3ui64NV( GLuint index, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %s, %s, %s)\n", index, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z) );
  funcs->ext.p_glVertexAttribL3ui64NV( index, x, y, z );
}

static void WINAPI glVertexAttribL3ui64vNV( GLuint index, const GLuint64EXT *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribL3ui64vNV( index, v );
}

static void WINAPI glVertexAttribL4d( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f, %f)\n", index, x, y, z, w );
  funcs->ext.p_glVertexAttribL4d( index, x, y, z, w );
}

static void WINAPI glVertexAttribL4dEXT( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f, %f)\n", index, x, y, z, w );
  funcs->ext.p_glVertexAttribL4dEXT( index, x, y, z, w );
}

static void WINAPI glVertexAttribL4dv( GLuint index, const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribL4dv( index, v );
}

static void WINAPI glVertexAttribL4dvEXT( GLuint index, const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribL4dvEXT( index, v );
}

static void WINAPI glVertexAttribL4i64NV( GLuint index, GLint64EXT x, GLint64EXT y, GLint64EXT z, GLint64EXT w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %s, %s, %s, %s)\n", index, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z), wine_dbgstr_longlong(w) );
  funcs->ext.p_glVertexAttribL4i64NV( index, x, y, z, w );
}

static void WINAPI glVertexAttribL4i64vNV( GLuint index, const GLint64EXT *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribL4i64vNV( index, v );
}

static void WINAPI glVertexAttribL4ui64NV( GLuint index, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z, GLuint64EXT w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %s, %s, %s, %s)\n", index, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z), wine_dbgstr_longlong(w) );
  funcs->ext.p_glVertexAttribL4ui64NV( index, x, y, z, w );
}

static void WINAPI glVertexAttribL4ui64vNV( GLuint index, const GLuint64EXT *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glVertexAttribL4ui64vNV( index, v );
}

static void WINAPI glVertexAttribLFormat( GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", attribindex, size, type, relativeoffset );
  funcs->ext.p_glVertexAttribLFormat( attribindex, size, type, relativeoffset );
}

static void WINAPI glVertexAttribLFormatNV( GLuint index, GLint size, GLenum type, GLsizei stride )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", index, size, type, stride );
  funcs->ext.p_glVertexAttribLFormatNV( index, size, type, stride );
}

static void WINAPI glVertexAttribLPointer( GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", index, size, type, stride, pointer );
  funcs->ext.p_glVertexAttribLPointer( index, size, type, stride, pointer );
}

static void WINAPI glVertexAttribLPointerEXT( GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", index, size, type, stride, pointer );
  funcs->ext.p_glVertexAttribLPointerEXT( index, size, type, stride, pointer );
}

static void WINAPI glVertexAttribP1ui( GLuint index, GLenum type, GLboolean normalized, GLuint value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", index, type, normalized, value );
  funcs->ext.p_glVertexAttribP1ui( index, type, normalized, value );
}

static void WINAPI glVertexAttribP1uiv( GLuint index, GLenum type, GLboolean normalized, const GLuint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", index, type, normalized, value );
  funcs->ext.p_glVertexAttribP1uiv( index, type, normalized, value );
}

static void WINAPI glVertexAttribP2ui( GLuint index, GLenum type, GLboolean normalized, GLuint value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", index, type, normalized, value );
  funcs->ext.p_glVertexAttribP2ui( index, type, normalized, value );
}

static void WINAPI glVertexAttribP2uiv( GLuint index, GLenum type, GLboolean normalized, const GLuint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", index, type, normalized, value );
  funcs->ext.p_glVertexAttribP2uiv( index, type, normalized, value );
}

static void WINAPI glVertexAttribP3ui( GLuint index, GLenum type, GLboolean normalized, GLuint value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", index, type, normalized, value );
  funcs->ext.p_glVertexAttribP3ui( index, type, normalized, value );
}

static void WINAPI glVertexAttribP3uiv( GLuint index, GLenum type, GLboolean normalized, const GLuint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", index, type, normalized, value );
  funcs->ext.p_glVertexAttribP3uiv( index, type, normalized, value );
}

static void WINAPI glVertexAttribP4ui( GLuint index, GLenum type, GLboolean normalized, GLuint value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", index, type, normalized, value );
  funcs->ext.p_glVertexAttribP4ui( index, type, normalized, value );
}

static void WINAPI glVertexAttribP4uiv( GLuint index, GLenum type, GLboolean normalized, const GLuint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", index, type, normalized, value );
  funcs->ext.p_glVertexAttribP4uiv( index, type, normalized, value );
}

static void WINAPI glVertexAttribParameteriAMD( GLuint index, GLenum pname, GLint param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", index, pname, param );
  funcs->ext.p_glVertexAttribParameteriAMD( index, pname, param );
}

static void WINAPI glVertexAttribPointer( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %p)\n", index, size, type, normalized, stride, pointer );
  funcs->ext.p_glVertexAttribPointer( index, size, type, normalized, stride, pointer );
}

static void WINAPI glVertexAttribPointerARB( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %p)\n", index, size, type, normalized, stride, pointer );
  funcs->ext.p_glVertexAttribPointerARB( index, size, type, normalized, stride, pointer );
}

static void WINAPI glVertexAttribPointerNV( GLuint index, GLint fsize, GLenum type, GLsizei stride, const void *pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", index, fsize, type, stride, pointer );
  funcs->ext.p_glVertexAttribPointerNV( index, fsize, type, stride, pointer );
}

static void WINAPI glVertexAttribs1dvNV( GLuint index, GLsizei count, const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, count, v );
  funcs->ext.p_glVertexAttribs1dvNV( index, count, v );
}

static void WINAPI glVertexAttribs1fvNV( GLuint index, GLsizei count, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, count, v );
  funcs->ext.p_glVertexAttribs1fvNV( index, count, v );
}

static void WINAPI glVertexAttribs1hvNV( GLuint index, GLsizei n, const GLhalfNV *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, n, v );
  funcs->ext.p_glVertexAttribs1hvNV( index, n, v );
}

static void WINAPI glVertexAttribs1svNV( GLuint index, GLsizei count, const GLshort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, count, v );
  funcs->ext.p_glVertexAttribs1svNV( index, count, v );
}

static void WINAPI glVertexAttribs2dvNV( GLuint index, GLsizei count, const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, count, v );
  funcs->ext.p_glVertexAttribs2dvNV( index, count, v );
}

static void WINAPI glVertexAttribs2fvNV( GLuint index, GLsizei count, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, count, v );
  funcs->ext.p_glVertexAttribs2fvNV( index, count, v );
}

static void WINAPI glVertexAttribs2hvNV( GLuint index, GLsizei n, const GLhalfNV *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, n, v );
  funcs->ext.p_glVertexAttribs2hvNV( index, n, v );
}

static void WINAPI glVertexAttribs2svNV( GLuint index, GLsizei count, const GLshort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, count, v );
  funcs->ext.p_glVertexAttribs2svNV( index, count, v );
}

static void WINAPI glVertexAttribs3dvNV( GLuint index, GLsizei count, const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, count, v );
  funcs->ext.p_glVertexAttribs3dvNV( index, count, v );
}

static void WINAPI glVertexAttribs3fvNV( GLuint index, GLsizei count, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, count, v );
  funcs->ext.p_glVertexAttribs3fvNV( index, count, v );
}

static void WINAPI glVertexAttribs3hvNV( GLuint index, GLsizei n, const GLhalfNV *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, n, v );
  funcs->ext.p_glVertexAttribs3hvNV( index, n, v );
}

static void WINAPI glVertexAttribs3svNV( GLuint index, GLsizei count, const GLshort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, count, v );
  funcs->ext.p_glVertexAttribs3svNV( index, count, v );
}

static void WINAPI glVertexAttribs4dvNV( GLuint index, GLsizei count, const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, count, v );
  funcs->ext.p_glVertexAttribs4dvNV( index, count, v );
}

static void WINAPI glVertexAttribs4fvNV( GLuint index, GLsizei count, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, count, v );
  funcs->ext.p_glVertexAttribs4fvNV( index, count, v );
}

static void WINAPI glVertexAttribs4hvNV( GLuint index, GLsizei n, const GLhalfNV *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, n, v );
  funcs->ext.p_glVertexAttribs4hvNV( index, n, v );
}

static void WINAPI glVertexAttribs4svNV( GLuint index, GLsizei count, const GLshort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, count, v );
  funcs->ext.p_glVertexAttribs4svNV( index, count, v );
}

static void WINAPI glVertexAttribs4ubvNV( GLuint index, GLsizei count, const GLubyte *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", index, count, v );
  funcs->ext.p_glVertexAttribs4ubvNV( index, count, v );
}

static void WINAPI glVertexBindingDivisor( GLuint bindingindex, GLuint divisor )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", bindingindex, divisor );
  funcs->ext.p_glVertexBindingDivisor( bindingindex, divisor );
}

static void WINAPI glVertexBlendARB( GLint count )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", count );
  funcs->ext.p_glVertexBlendARB( count );
}

static void WINAPI glVertexBlendEnvfATI( GLenum pname, GLfloat param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f)\n", pname, param );
  funcs->ext.p_glVertexBlendEnvfATI( pname, param );
}

static void WINAPI glVertexBlendEnviATI( GLenum pname, GLint param )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", pname, param );
  funcs->ext.p_glVertexBlendEnviATI( pname, param );
}

static void WINAPI glVertexFormatNV( GLint size, GLenum type, GLsizei stride )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", size, type, stride );
  funcs->ext.p_glVertexFormatNV( size, type, stride );
}

static void WINAPI glVertexP2ui( GLenum type, GLuint value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", type, value );
  funcs->ext.p_glVertexP2ui( type, value );
}

static void WINAPI glVertexP2uiv( GLenum type, const GLuint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", type, value );
  funcs->ext.p_glVertexP2uiv( type, value );
}

static void WINAPI glVertexP3ui( GLenum type, GLuint value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", type, value );
  funcs->ext.p_glVertexP3ui( type, value );
}

static void WINAPI glVertexP3uiv( GLenum type, const GLuint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", type, value );
  funcs->ext.p_glVertexP3uiv( type, value );
}

static void WINAPI glVertexP4ui( GLenum type, GLuint value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", type, value );
  funcs->ext.p_glVertexP4ui( type, value );
}

static void WINAPI glVertexP4uiv( GLenum type, const GLuint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", type, value );
  funcs->ext.p_glVertexP4uiv( type, value );
}

static void WINAPI glVertexPointerEXT( GLint size, GLenum type, GLsizei stride, GLsizei count, const void *pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %p)\n", size, type, stride, count, pointer );
  funcs->ext.p_glVertexPointerEXT( size, type, stride, count, pointer );
}

static void WINAPI glVertexPointerListIBM( GLint size, GLenum type, GLint stride, const void **pointer, GLint ptrstride )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p, %d)\n", size, type, stride, pointer, ptrstride );
  funcs->ext.p_glVertexPointerListIBM( size, type, stride, pointer, ptrstride );
}

static void WINAPI glVertexPointervINTEL( GLint size, GLenum type, const void **pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", size, type, pointer );
  funcs->ext.p_glVertexPointervINTEL( size, type, pointer );
}

static void WINAPI glVertexStream1dATI( GLenum stream, GLdouble x )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f)\n", stream, x );
  funcs->ext.p_glVertexStream1dATI( stream, x );
}

static void WINAPI glVertexStream1dvATI( GLenum stream, const GLdouble *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", stream, coords );
  funcs->ext.p_glVertexStream1dvATI( stream, coords );
}

static void WINAPI glVertexStream1fATI( GLenum stream, GLfloat x )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f)\n", stream, x );
  funcs->ext.p_glVertexStream1fATI( stream, x );
}

static void WINAPI glVertexStream1fvATI( GLenum stream, const GLfloat *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", stream, coords );
  funcs->ext.p_glVertexStream1fvATI( stream, coords );
}

static void WINAPI glVertexStream1iATI( GLenum stream, GLint x )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", stream, x );
  funcs->ext.p_glVertexStream1iATI( stream, x );
}

static void WINAPI glVertexStream1ivATI( GLenum stream, const GLint *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", stream, coords );
  funcs->ext.p_glVertexStream1ivATI( stream, coords );
}

static void WINAPI glVertexStream1sATI( GLenum stream, GLshort x )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", stream, x );
  funcs->ext.p_glVertexStream1sATI( stream, x );
}

static void WINAPI glVertexStream1svATI( GLenum stream, const GLshort *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", stream, coords );
  funcs->ext.p_glVertexStream1svATI( stream, coords );
}

static void WINAPI glVertexStream2dATI( GLenum stream, GLdouble x, GLdouble y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f)\n", stream, x, y );
  funcs->ext.p_glVertexStream2dATI( stream, x, y );
}

static void WINAPI glVertexStream2dvATI( GLenum stream, const GLdouble *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", stream, coords );
  funcs->ext.p_glVertexStream2dvATI( stream, coords );
}

static void WINAPI glVertexStream2fATI( GLenum stream, GLfloat x, GLfloat y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f)\n", stream, x, y );
  funcs->ext.p_glVertexStream2fATI( stream, x, y );
}

static void WINAPI glVertexStream2fvATI( GLenum stream, const GLfloat *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", stream, coords );
  funcs->ext.p_glVertexStream2fvATI( stream, coords );
}

static void WINAPI glVertexStream2iATI( GLenum stream, GLint x, GLint y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", stream, x, y );
  funcs->ext.p_glVertexStream2iATI( stream, x, y );
}

static void WINAPI glVertexStream2ivATI( GLenum stream, const GLint *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", stream, coords );
  funcs->ext.p_glVertexStream2ivATI( stream, coords );
}

static void WINAPI glVertexStream2sATI( GLenum stream, GLshort x, GLshort y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", stream, x, y );
  funcs->ext.p_glVertexStream2sATI( stream, x, y );
}

static void WINAPI glVertexStream2svATI( GLenum stream, const GLshort *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", stream, coords );
  funcs->ext.p_glVertexStream2svATI( stream, coords );
}

static void WINAPI glVertexStream3dATI( GLenum stream, GLdouble x, GLdouble y, GLdouble z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f)\n", stream, x, y, z );
  funcs->ext.p_glVertexStream3dATI( stream, x, y, z );
}

static void WINAPI glVertexStream3dvATI( GLenum stream, const GLdouble *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", stream, coords );
  funcs->ext.p_glVertexStream3dvATI( stream, coords );
}

static void WINAPI glVertexStream3fATI( GLenum stream, GLfloat x, GLfloat y, GLfloat z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f)\n", stream, x, y, z );
  funcs->ext.p_glVertexStream3fATI( stream, x, y, z );
}

static void WINAPI glVertexStream3fvATI( GLenum stream, const GLfloat *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", stream, coords );
  funcs->ext.p_glVertexStream3fvATI( stream, coords );
}

static void WINAPI glVertexStream3iATI( GLenum stream, GLint x, GLint y, GLint z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", stream, x, y, z );
  funcs->ext.p_glVertexStream3iATI( stream, x, y, z );
}

static void WINAPI glVertexStream3ivATI( GLenum stream, const GLint *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", stream, coords );
  funcs->ext.p_glVertexStream3ivATI( stream, coords );
}

static void WINAPI glVertexStream3sATI( GLenum stream, GLshort x, GLshort y, GLshort z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", stream, x, y, z );
  funcs->ext.p_glVertexStream3sATI( stream, x, y, z );
}

static void WINAPI glVertexStream3svATI( GLenum stream, const GLshort *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", stream, coords );
  funcs->ext.p_glVertexStream3svATI( stream, coords );
}

static void WINAPI glVertexStream4dATI( GLenum stream, GLdouble x, GLdouble y, GLdouble z, GLdouble w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f, %f)\n", stream, x, y, z, w );
  funcs->ext.p_glVertexStream4dATI( stream, x, y, z, w );
}

static void WINAPI glVertexStream4dvATI( GLenum stream, const GLdouble *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", stream, coords );
  funcs->ext.p_glVertexStream4dvATI( stream, coords );
}

static void WINAPI glVertexStream4fATI( GLenum stream, GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f, %f)\n", stream, x, y, z, w );
  funcs->ext.p_glVertexStream4fATI( stream, x, y, z, w );
}

static void WINAPI glVertexStream4fvATI( GLenum stream, const GLfloat *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", stream, coords );
  funcs->ext.p_glVertexStream4fvATI( stream, coords );
}

static void WINAPI glVertexStream4iATI( GLenum stream, GLint x, GLint y, GLint z, GLint w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", stream, x, y, z, w );
  funcs->ext.p_glVertexStream4iATI( stream, x, y, z, w );
}

static void WINAPI glVertexStream4ivATI( GLenum stream, const GLint *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", stream, coords );
  funcs->ext.p_glVertexStream4ivATI( stream, coords );
}

static void WINAPI glVertexStream4sATI( GLenum stream, GLshort x, GLshort y, GLshort z, GLshort w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", stream, x, y, z, w );
  funcs->ext.p_glVertexStream4sATI( stream, x, y, z, w );
}

static void WINAPI glVertexStream4svATI( GLenum stream, const GLshort *coords )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", stream, coords );
  funcs->ext.p_glVertexStream4svATI( stream, coords );
}

static void WINAPI glVertexWeightPointerEXT( GLint size, GLenum type, GLsizei stride, const void *pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", size, type, stride, pointer );
  funcs->ext.p_glVertexWeightPointerEXT( size, type, stride, pointer );
}

static void WINAPI glVertexWeightfEXT( GLfloat weight )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f)\n", weight );
  funcs->ext.p_glVertexWeightfEXT( weight );
}

static void WINAPI glVertexWeightfvEXT( const GLfloat *weight )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", weight );
  funcs->ext.p_glVertexWeightfvEXT( weight );
}

static void WINAPI glVertexWeighthNV( GLhalfNV weight )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", weight );
  funcs->ext.p_glVertexWeighthNV( weight );
}

static void WINAPI glVertexWeighthvNV( const GLhalfNV *weight )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", weight );
  funcs->ext.p_glVertexWeighthvNV( weight );
}

static GLenum WINAPI glVideoCaptureNV( GLuint video_capture_slot, GLuint *sequence_num, GLuint64EXT *capture_time )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p, %p)\n", video_capture_slot, sequence_num, capture_time );
  return funcs->ext.p_glVideoCaptureNV( video_capture_slot, sequence_num, capture_time );
}

static void WINAPI glVideoCaptureStreamParameterdvNV( GLuint video_capture_slot, GLuint stream, GLenum pname, const GLdouble *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", video_capture_slot, stream, pname, params );
  funcs->ext.p_glVideoCaptureStreamParameterdvNV( video_capture_slot, stream, pname, params );
}

static void WINAPI glVideoCaptureStreamParameterfvNV( GLuint video_capture_slot, GLuint stream, GLenum pname, const GLfloat *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", video_capture_slot, stream, pname, params );
  funcs->ext.p_glVideoCaptureStreamParameterfvNV( video_capture_slot, stream, pname, params );
}

static void WINAPI glVideoCaptureStreamParameterivNV( GLuint video_capture_slot, GLuint stream, GLenum pname, const GLint *params )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", video_capture_slot, stream, pname, params );
  funcs->ext.p_glVideoCaptureStreamParameterivNV( video_capture_slot, stream, pname, params );
}

static void WINAPI glViewportArrayv( GLuint first, GLsizei count, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", first, count, v );
  funcs->ext.p_glViewportArrayv( first, count, v );
}

static void WINAPI glViewportIndexedf( GLuint index, GLfloat x, GLfloat y, GLfloat w, GLfloat h )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f, %f)\n", index, x, y, w, h );
  funcs->ext.p_glViewportIndexedf( index, x, y, w, h );
}

static void WINAPI glViewportIndexedfv( GLuint index, const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", index, v );
  funcs->ext.p_glViewportIndexedfv( index, v );
}

static void WINAPI glViewportPositionWScaleNV( GLuint index, GLfloat xcoeff, GLfloat ycoeff )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f)\n", index, xcoeff, ycoeff );
  funcs->ext.p_glViewportPositionWScaleNV( index, xcoeff, ycoeff );
}

static void WINAPI glViewportSwizzleNV( GLuint index, GLenum swizzlex, GLenum swizzley, GLenum swizzlez, GLenum swizzlew )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d)\n", index, swizzlex, swizzley, swizzlez, swizzlew );
  funcs->ext.p_glViewportSwizzleNV( index, swizzlex, swizzley, swizzlez, swizzlew );
}

static void WINAPI glWaitSemaphoreEXT( GLuint semaphore, GLuint numBufferBarriers, const GLuint *buffers, GLuint numTextureBarriers, const GLuint *textures, const GLenum *srcLayouts )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %d, %p, %p)\n", semaphore, numBufferBarriers, buffers, numTextureBarriers, textures, srcLayouts );
  funcs->ext.p_glWaitSemaphoreEXT( semaphore, numBufferBarriers, buffers, numTextureBarriers, textures, srcLayouts );
}

static void WINAPI glWaitSync( GLsync sync, GLbitfield flags, GLuint64 timeout )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p, %d, %s)\n", sync, flags, wine_dbgstr_longlong(timeout) );
  funcs->ext.p_glWaitSync( sync, flags, timeout );
}

static void WINAPI glWaitVkSemaphoreNV( GLuint64 vkSemaphore )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%s)\n", wine_dbgstr_longlong(vkSemaphore) );
  funcs->ext.p_glWaitVkSemaphoreNV( vkSemaphore );
}

static void WINAPI glWeightPathsNV( GLuint resultPath, GLsizei numPaths, const GLuint *paths, const GLfloat *weights )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p, %p)\n", resultPath, numPaths, paths, weights );
  funcs->ext.p_glWeightPathsNV( resultPath, numPaths, paths, weights );
}

static void WINAPI glWeightPointerARB( GLint size, GLenum type, GLsizei stride, const void *pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %p)\n", size, type, stride, pointer );
  funcs->ext.p_glWeightPointerARB( size, type, stride, pointer );
}

static void WINAPI glWeightbvARB( GLint size, const GLbyte *weights )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", size, weights );
  funcs->ext.p_glWeightbvARB( size, weights );
}

static void WINAPI glWeightdvARB( GLint size, const GLdouble *weights )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", size, weights );
  funcs->ext.p_glWeightdvARB( size, weights );
}

static void WINAPI glWeightfvARB( GLint size, const GLfloat *weights )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", size, weights );
  funcs->ext.p_glWeightfvARB( size, weights );
}

static void WINAPI glWeightivARB( GLint size, const GLint *weights )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", size, weights );
  funcs->ext.p_glWeightivARB( size, weights );
}

static void WINAPI glWeightsvARB( GLint size, const GLshort *weights )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", size, weights );
  funcs->ext.p_glWeightsvARB( size, weights );
}

static void WINAPI glWeightubvARB( GLint size, const GLubyte *weights )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", size, weights );
  funcs->ext.p_glWeightubvARB( size, weights );
}

static void WINAPI glWeightuivARB( GLint size, const GLuint *weights )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", size, weights );
  funcs->ext.p_glWeightuivARB( size, weights );
}

static void WINAPI glWeightusvARB( GLint size, const GLushort *weights )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", size, weights );
  funcs->ext.p_glWeightusvARB( size, weights );
}

static void WINAPI glWindowPos2d( GLdouble x, GLdouble y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f)\n", x, y );
  funcs->ext.p_glWindowPos2d( x, y );
}

static void WINAPI glWindowPos2dARB( GLdouble x, GLdouble y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f)\n", x, y );
  funcs->ext.p_glWindowPos2dARB( x, y );
}

static void WINAPI glWindowPos2dMESA( GLdouble x, GLdouble y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f)\n", x, y );
  funcs->ext.p_glWindowPos2dMESA( x, y );
}

static void WINAPI glWindowPos2dv( const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glWindowPos2dv( v );
}

static void WINAPI glWindowPos2dvARB( const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glWindowPos2dvARB( v );
}

static void WINAPI glWindowPos2dvMESA( const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glWindowPos2dvMESA( v );
}

static void WINAPI glWindowPos2f( GLfloat x, GLfloat y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f)\n", x, y );
  funcs->ext.p_glWindowPos2f( x, y );
}

static void WINAPI glWindowPos2fARB( GLfloat x, GLfloat y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f)\n", x, y );
  funcs->ext.p_glWindowPos2fARB( x, y );
}

static void WINAPI glWindowPos2fMESA( GLfloat x, GLfloat y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f)\n", x, y );
  funcs->ext.p_glWindowPos2fMESA( x, y );
}

static void WINAPI glWindowPos2fv( const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glWindowPos2fv( v );
}

static void WINAPI glWindowPos2fvARB( const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glWindowPos2fvARB( v );
}

static void WINAPI glWindowPos2fvMESA( const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glWindowPos2fvMESA( v );
}

static void WINAPI glWindowPos2i( GLint x, GLint y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", x, y );
  funcs->ext.p_glWindowPos2i( x, y );
}

static void WINAPI glWindowPos2iARB( GLint x, GLint y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", x, y );
  funcs->ext.p_glWindowPos2iARB( x, y );
}

static void WINAPI glWindowPos2iMESA( GLint x, GLint y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", x, y );
  funcs->ext.p_glWindowPos2iMESA( x, y );
}

static void WINAPI glWindowPos2iv( const GLint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glWindowPos2iv( v );
}

static void WINAPI glWindowPos2ivARB( const GLint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glWindowPos2ivARB( v );
}

static void WINAPI glWindowPos2ivMESA( const GLint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glWindowPos2ivMESA( v );
}

static void WINAPI glWindowPos2s( GLshort x, GLshort y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", x, y );
  funcs->ext.p_glWindowPos2s( x, y );
}

static void WINAPI glWindowPos2sARB( GLshort x, GLshort y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", x, y );
  funcs->ext.p_glWindowPos2sARB( x, y );
}

static void WINAPI glWindowPos2sMESA( GLshort x, GLshort y )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d)\n", x, y );
  funcs->ext.p_glWindowPos2sMESA( x, y );
}

static void WINAPI glWindowPos2sv( const GLshort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glWindowPos2sv( v );
}

static void WINAPI glWindowPos2svARB( const GLshort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glWindowPos2svARB( v );
}

static void WINAPI glWindowPos2svMESA( const GLshort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glWindowPos2svMESA( v );
}

static void WINAPI glWindowPos3d( GLdouble x, GLdouble y, GLdouble z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f, %f)\n", x, y, z );
  funcs->ext.p_glWindowPos3d( x, y, z );
}

static void WINAPI glWindowPos3dARB( GLdouble x, GLdouble y, GLdouble z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f, %f)\n", x, y, z );
  funcs->ext.p_glWindowPos3dARB( x, y, z );
}

static void WINAPI glWindowPos3dMESA( GLdouble x, GLdouble y, GLdouble z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f, %f)\n", x, y, z );
  funcs->ext.p_glWindowPos3dMESA( x, y, z );
}

static void WINAPI glWindowPos3dv( const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glWindowPos3dv( v );
}

static void WINAPI glWindowPos3dvARB( const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glWindowPos3dvARB( v );
}

static void WINAPI glWindowPos3dvMESA( const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glWindowPos3dvMESA( v );
}

static void WINAPI glWindowPos3f( GLfloat x, GLfloat y, GLfloat z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f, %f)\n", x, y, z );
  funcs->ext.p_glWindowPos3f( x, y, z );
}

static void WINAPI glWindowPos3fARB( GLfloat x, GLfloat y, GLfloat z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f, %f)\n", x, y, z );
  funcs->ext.p_glWindowPos3fARB( x, y, z );
}

static void WINAPI glWindowPos3fMESA( GLfloat x, GLfloat y, GLfloat z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f, %f)\n", x, y, z );
  funcs->ext.p_glWindowPos3fMESA( x, y, z );
}

static void WINAPI glWindowPos3fv( const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glWindowPos3fv( v );
}

static void WINAPI glWindowPos3fvARB( const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glWindowPos3fvARB( v );
}

static void WINAPI glWindowPos3fvMESA( const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glWindowPos3fvMESA( v );
}

static void WINAPI glWindowPos3i( GLint x, GLint y, GLint z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", x, y, z );
  funcs->ext.p_glWindowPos3i( x, y, z );
}

static void WINAPI glWindowPos3iARB( GLint x, GLint y, GLint z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", x, y, z );
  funcs->ext.p_glWindowPos3iARB( x, y, z );
}

static void WINAPI glWindowPos3iMESA( GLint x, GLint y, GLint z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", x, y, z );
  funcs->ext.p_glWindowPos3iMESA( x, y, z );
}

static void WINAPI glWindowPos3iv( const GLint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glWindowPos3iv( v );
}

static void WINAPI glWindowPos3ivARB( const GLint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glWindowPos3ivARB( v );
}

static void WINAPI glWindowPos3ivMESA( const GLint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glWindowPos3ivMESA( v );
}

static void WINAPI glWindowPos3s( GLshort x, GLshort y, GLshort z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", x, y, z );
  funcs->ext.p_glWindowPos3s( x, y, z );
}

static void WINAPI glWindowPos3sARB( GLshort x, GLshort y, GLshort z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", x, y, z );
  funcs->ext.p_glWindowPos3sARB( x, y, z );
}

static void WINAPI glWindowPos3sMESA( GLshort x, GLshort y, GLshort z )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d)\n", x, y, z );
  funcs->ext.p_glWindowPos3sMESA( x, y, z );
}

static void WINAPI glWindowPos3sv( const GLshort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glWindowPos3sv( v );
}

static void WINAPI glWindowPos3svARB( const GLshort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glWindowPos3svARB( v );
}

static void WINAPI glWindowPos3svMESA( const GLshort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glWindowPos3svMESA( v );
}

static void WINAPI glWindowPos4dMESA( GLdouble x, GLdouble y, GLdouble z, GLdouble w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f, %f, %f)\n", x, y, z, w );
  funcs->ext.p_glWindowPos4dMESA( x, y, z, w );
}

static void WINAPI glWindowPos4dvMESA( const GLdouble *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glWindowPos4dvMESA( v );
}

static void WINAPI glWindowPos4fMESA( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%f, %f, %f, %f)\n", x, y, z, w );
  funcs->ext.p_glWindowPos4fMESA( x, y, z, w );
}

static void WINAPI glWindowPos4fvMESA( const GLfloat *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glWindowPos4fvMESA( v );
}

static void WINAPI glWindowPos4iMESA( GLint x, GLint y, GLint z, GLint w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", x, y, z, w );
  funcs->ext.p_glWindowPos4iMESA( x, y, z, w );
}

static void WINAPI glWindowPos4ivMESA( const GLint *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glWindowPos4ivMESA( v );
}

static void WINAPI glWindowPos4sMESA( GLshort x, GLshort y, GLshort z, GLshort w )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d)\n", x, y, z, w );
  funcs->ext.p_glWindowPos4sMESA( x, y, z, w );
}

static void WINAPI glWindowPos4svMESA( const GLshort *v )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", v );
  funcs->ext.p_glWindowPos4svMESA( v );
}

static void WINAPI glWindowRectanglesEXT( GLenum mode, GLsizei count, const GLint *box )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %p)\n", mode, count, box );
  funcs->ext.p_glWindowRectanglesEXT( mode, count, box );
}

static void WINAPI glWriteMaskEXT( GLuint res, GLuint in, GLenum outX, GLenum outY, GLenum outZ, GLenum outW )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %d, %d, %d, %d, %d)\n", res, in, outX, outY, outZ, outW );
  funcs->ext.p_glWriteMaskEXT( res, in, outX, outY, outZ, outW );
}

static void * WINAPI wglAllocateMemoryNV( GLsizei size, GLfloat readfreq, GLfloat writefreq, GLfloat priority )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %f, %f, %f)\n", size, readfreq, writefreq, priority );
  return funcs->ext.p_wglAllocateMemoryNV( size, readfreq, writefreq, priority );
}

static BOOL WINAPI wglChoosePixelFormatARB( HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats )
{
  const struct opengl_funcs *funcs = get_dc_funcs( hdc );
  TRACE( "(%p, %p, %p, %u, %p, %p)\n", hdc, piAttribIList, pfAttribFList, nMaxFormats, piFormats, nNumFormats );
  if (!funcs || !funcs->ext.p_wglChoosePixelFormatARB) return 0;
  return funcs->ext.p_wglChoosePixelFormatARB( hdc, piAttribIList, pfAttribFList, nMaxFormats, piFormats, nNumFormats );
}

static void WINAPI wglFreeMemoryNV( void *pointer )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%p)\n", pointer );
  funcs->ext.p_wglFreeMemoryNV( pointer );
}

static const char * WINAPI wglGetExtensionsStringARB( HDC hdc )
{
  const struct opengl_funcs *funcs = get_dc_funcs( hdc );
  TRACE( "(%p)\n", hdc );
  if (!funcs || !funcs->ext.p_wglGetExtensionsStringARB) return 0;
  return funcs->ext.p_wglGetExtensionsStringARB( hdc );
}

static const char * WINAPI wglGetExtensionsStringEXT(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  return funcs->ext.p_wglGetExtensionsStringEXT();
}

static BOOL WINAPI wglGetPixelFormatAttribfvARB( HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, FLOAT *pfValues )
{
  const struct opengl_funcs *funcs = get_dc_funcs( hdc );
  TRACE( "(%p, %d, %d, %u, %p, %p)\n", hdc, iPixelFormat, iLayerPlane, nAttributes, piAttributes, pfValues );
  if (!funcs || !funcs->ext.p_wglGetPixelFormatAttribfvARB) return 0;
  return funcs->ext.p_wglGetPixelFormatAttribfvARB( hdc, iPixelFormat, iLayerPlane, nAttributes, piAttributes, pfValues );
}

static BOOL WINAPI wglGetPixelFormatAttribivARB( HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, int *piValues )
{
  const struct opengl_funcs *funcs = get_dc_funcs( hdc );
  TRACE( "(%p, %d, %d, %u, %p, %p)\n", hdc, iPixelFormat, iLayerPlane, nAttributes, piAttributes, piValues );
  if (!funcs || !funcs->ext.p_wglGetPixelFormatAttribivARB) return 0;
  return funcs->ext.p_wglGetPixelFormatAttribivARB( hdc, iPixelFormat, iLayerPlane, nAttributes, piAttributes, piValues );
}

static int WINAPI wglGetSwapIntervalEXT(void)
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "()\n" );
  return funcs->ext.p_wglGetSwapIntervalEXT();
}

static BOOL WINAPI wglQueryCurrentRendererIntegerWINE( GLenum attribute, GLuint *value )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d, %p)\n", attribute, value );
  return funcs->ext.p_wglQueryCurrentRendererIntegerWINE( attribute, value );
}

static const GLchar * WINAPI wglQueryCurrentRendererStringWINE( GLenum attribute )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", attribute );
  return funcs->ext.p_wglQueryCurrentRendererStringWINE( attribute );
}

static BOOL WINAPI wglQueryRendererIntegerWINE( HDC dc, GLint renderer, GLenum attribute, GLuint *value )
{
  const struct opengl_funcs *funcs = get_dc_funcs( dc );
  TRACE( "(%p, %d, %d, %p)\n", dc, renderer, attribute, value );
  if (!funcs || !funcs->ext.p_wglQueryRendererIntegerWINE) return 0;
  return funcs->ext.p_wglQueryRendererIntegerWINE( dc, renderer, attribute, value );
}

static const GLchar * WINAPI wglQueryRendererStringWINE( HDC dc, GLint renderer, GLenum attribute )
{
  const struct opengl_funcs *funcs = get_dc_funcs( dc );
  TRACE( "(%p, %d, %d)\n", dc, renderer, attribute );
  if (!funcs || !funcs->ext.p_wglQueryRendererStringWINE) return 0;
  return funcs->ext.p_wglQueryRendererStringWINE( dc, renderer, attribute );
}

static BOOL WINAPI wglSetPixelFormatWINE( HDC hdc, int format )
{
  const struct opengl_funcs *funcs = get_dc_funcs( hdc );
  TRACE( "(%p, %d)\n", hdc, format );
  if (!funcs || !funcs->ext.p_wglSetPixelFormatWINE) return 0;
  return funcs->ext.p_wglSetPixelFormatWINE( hdc, format );
}

static BOOL WINAPI wglSwapIntervalEXT( int interval )
{
  const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
  TRACE( "(%d)\n", interval );
  return funcs->ext.p_wglSwapIntervalEXT( interval );
}

extern void WINAPI glDebugMessageCallback( GLDEBUGPROC callback, const void *userParam ) DECLSPEC_HIDDEN;
extern void WINAPI glDebugMessageCallbackAMD( GLDEBUGPROCAMD callback, void *userParam ) DECLSPEC_HIDDEN;
extern void WINAPI glDebugMessageCallbackARB( GLDEBUGPROCARB callback, const void *userParam ) DECLSPEC_HIDDEN;
extern const GLubyte * WINAPI glGetStringi( GLenum name, GLuint index ) DECLSPEC_HIDDEN;
extern BOOL WINAPI wglBindTexImageARB( HPBUFFERARB hPbuffer, int iBuffer ) DECLSPEC_HIDDEN;
extern HGLRC WINAPI wglCreateContextAttribsARB( HDC hDC, HGLRC hShareContext, const int *attribList ) DECLSPEC_HIDDEN;
extern HPBUFFERARB WINAPI wglCreatePbufferARB( HDC hDC, int iPixelFormat, int iWidth, int iHeight, const int *piAttribList ) DECLSPEC_HIDDEN;
extern BOOL WINAPI wglDestroyPbufferARB( HPBUFFERARB hPbuffer ) DECLSPEC_HIDDEN;
extern HDC WINAPI wglGetCurrentReadDCARB(void) DECLSPEC_HIDDEN;
extern HDC WINAPI wglGetPbufferDCARB( HPBUFFERARB hPbuffer ) DECLSPEC_HIDDEN;
extern BOOL WINAPI wglMakeContextCurrentARB( HDC hDrawDC, HDC hReadDC, HGLRC hglrc ) DECLSPEC_HIDDEN;
extern BOOL WINAPI wglQueryPbufferARB( HPBUFFERARB hPbuffer, int iAttribute, int *piValue ) DECLSPEC_HIDDEN;
extern int WINAPI wglReleasePbufferDCARB( HPBUFFERARB hPbuffer, HDC hDC ) DECLSPEC_HIDDEN;
extern BOOL WINAPI wglReleaseTexImageARB( HPBUFFERARB hPbuffer, int iBuffer ) DECLSPEC_HIDDEN;
extern BOOL WINAPI wglSetPbufferAttribARB( HPBUFFERARB hPbuffer, const int *piAttribList ) DECLSPEC_HIDDEN;

const OpenGL_extension extension_registry[2655] = {
  { "glAccumxOES", "GL_OES_fixed_point", glAccumxOES },
  { "glAcquireKeyedMutexWin32EXT", "GL_EXT_win32_keyed_mutex", glAcquireKeyedMutexWin32EXT },
  { "glActiveProgramEXT", "GL_EXT_separate_shader_objects", glActiveProgramEXT },
  { "glActiveShaderProgram", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glActiveShaderProgram },
  { "glActiveStencilFaceEXT", "GL_EXT_stencil_two_side", glActiveStencilFaceEXT },
  { "glActiveTexture", "GL_VERSION_1_3", glActiveTexture },
  { "glActiveTextureARB", "GL_ARB_multitexture", glActiveTextureARB },
  { "glActiveVaryingNV", "GL_NV_transform_feedback", glActiveVaryingNV },
  { "glAlphaFragmentOp1ATI", "GL_ATI_fragment_shader", glAlphaFragmentOp1ATI },
  { "glAlphaFragmentOp2ATI", "GL_ATI_fragment_shader", glAlphaFragmentOp2ATI },
  { "glAlphaFragmentOp3ATI", "GL_ATI_fragment_shader", glAlphaFragmentOp3ATI },
  { "glAlphaFuncxOES", "GL_OES_fixed_point", glAlphaFuncxOES },
  { "glAlphaToCoverageDitherControlNV", "GL_NV_alpha_to_coverage_dither_control", glAlphaToCoverageDitherControlNV },
  { "glApplyFramebufferAttachmentCMAAINTEL", "GL_INTEL_framebuffer_CMAA", glApplyFramebufferAttachmentCMAAINTEL },
  { "glApplyTextureEXT", "GL_EXT_light_texture", glApplyTextureEXT },
  { "glAreProgramsResidentNV", "GL_NV_vertex_program", glAreProgramsResidentNV },
  { "glAreTexturesResidentEXT", "GL_EXT_texture_object", glAreTexturesResidentEXT },
  { "glArrayElementEXT", "GL_EXT_vertex_array", glArrayElementEXT },
  { "glArrayObjectATI", "GL_ATI_vertex_array_object", glArrayObjectATI },
  { "glAsyncMarkerSGIX", "GL_SGIX_async", glAsyncMarkerSGIX },
  { "glAttachObjectARB", "GL_ARB_shader_objects", glAttachObjectARB },
  { "glAttachShader", "GL_VERSION_2_0", glAttachShader },
  { "glBeginConditionalRender", "GL_VERSION_3_0", glBeginConditionalRender },
  { "glBeginConditionalRenderNV", "GL_NV_conditional_render", glBeginConditionalRenderNV },
  { "glBeginConditionalRenderNVX", "GL_NVX_conditional_render", glBeginConditionalRenderNVX },
  { "glBeginFragmentShaderATI", "GL_ATI_fragment_shader", glBeginFragmentShaderATI },
  { "glBeginOcclusionQueryNV", "GL_NV_occlusion_query", glBeginOcclusionQueryNV },
  { "glBeginPerfMonitorAMD", "GL_AMD_performance_monitor", glBeginPerfMonitorAMD },
  { "glBeginPerfQueryINTEL", "GL_INTEL_performance_query", glBeginPerfQueryINTEL },
  { "glBeginQuery", "GL_VERSION_1_5", glBeginQuery },
  { "glBeginQueryARB", "GL_ARB_occlusion_query", glBeginQueryARB },
  { "glBeginQueryIndexed", "GL_ARB_transform_feedback3 GL_VERSION_4_0", glBeginQueryIndexed },
  { "glBeginTransformFeedback", "GL_VERSION_3_0", glBeginTransformFeedback },
  { "glBeginTransformFeedbackEXT", "GL_EXT_transform_feedback", glBeginTransformFeedbackEXT },
  { "glBeginTransformFeedbackNV", "GL_NV_transform_feedback", glBeginTransformFeedbackNV },
  { "glBeginVertexShaderEXT", "GL_EXT_vertex_shader", glBeginVertexShaderEXT },
  { "glBeginVideoCaptureNV", "GL_NV_video_capture", glBeginVideoCaptureNV },
  { "glBindAttribLocation", "GL_VERSION_2_0", glBindAttribLocation },
  { "glBindAttribLocationARB", "GL_ARB_vertex_shader", glBindAttribLocationARB },
  { "glBindBuffer", "GL_VERSION_1_5", glBindBuffer },
  { "glBindBufferARB", "GL_ARB_vertex_buffer_object", glBindBufferARB },
  { "glBindBufferBase", "GL_ARB_uniform_buffer_object GL_VERSION_3_0", glBindBufferBase },
  { "glBindBufferBaseEXT", "GL_EXT_transform_feedback", glBindBufferBaseEXT },
  { "glBindBufferBaseNV", "GL_NV_transform_feedback", glBindBufferBaseNV },
  { "glBindBufferOffsetEXT", "GL_EXT_transform_feedback", glBindBufferOffsetEXT },
  { "glBindBufferOffsetNV", "GL_NV_transform_feedback", glBindBufferOffsetNV },
  { "glBindBufferRange", "GL_ARB_uniform_buffer_object GL_VERSION_3_0", glBindBufferRange },
  { "glBindBufferRangeEXT", "GL_EXT_transform_feedback", glBindBufferRangeEXT },
  { "glBindBufferRangeNV", "GL_NV_transform_feedback", glBindBufferRangeNV },
  { "glBindBuffersBase", "GL_ARB_multi_bind GL_VERSION_4_4", glBindBuffersBase },
  { "glBindBuffersRange", "GL_ARB_multi_bind GL_VERSION_4_4", glBindBuffersRange },
  { "glBindFragDataLocation", "GL_VERSION_3_0", glBindFragDataLocation },
  { "glBindFragDataLocationEXT", "GL_EXT_gpu_shader4", glBindFragDataLocationEXT },
  { "glBindFragDataLocationIndexed", "GL_ARB_blend_func_extended GL_VERSION_3_3", glBindFragDataLocationIndexed },
  { "glBindFragmentShaderATI", "GL_ATI_fragment_shader", glBindFragmentShaderATI },
  { "glBindFramebuffer", "GL_ARB_framebuffer_object GL_VERSION_3_0", glBindFramebuffer },
  { "glBindFramebufferEXT", "GL_EXT_framebuffer_object", glBindFramebufferEXT },
  { "glBindImageTexture", "GL_ARB_shader_image_load_store GL_VERSION_4_2", glBindImageTexture },
  { "glBindImageTextureEXT", "GL_EXT_shader_image_load_store", glBindImageTextureEXT },
  { "glBindImageTextures", "GL_ARB_multi_bind GL_VERSION_4_4", glBindImageTextures },
  { "glBindLightParameterEXT", "GL_EXT_vertex_shader", glBindLightParameterEXT },
  { "glBindMaterialParameterEXT", "GL_EXT_vertex_shader", glBindMaterialParameterEXT },
  { "glBindMultiTextureEXT", "GL_EXT_direct_state_access", glBindMultiTextureEXT },
  { "glBindParameterEXT", "GL_EXT_vertex_shader", glBindParameterEXT },
  { "glBindProgramARB", "GL_ARB_fragment_program GL_ARB_vertex_program", glBindProgramARB },
  { "glBindProgramNV", "GL_NV_vertex_program", glBindProgramNV },
  { "glBindProgramPipeline", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glBindProgramPipeline },
  { "glBindRenderbuffer", "GL_ARB_framebuffer_object GL_VERSION_3_0", glBindRenderbuffer },
  { "glBindRenderbufferEXT", "GL_EXT_framebuffer_object", glBindRenderbufferEXT },
  { "glBindSampler", "GL_ARB_sampler_objects GL_VERSION_3_3", glBindSampler },
  { "glBindSamplers", "GL_ARB_multi_bind GL_VERSION_4_4", glBindSamplers },
  { "glBindTexGenParameterEXT", "GL_EXT_vertex_shader", glBindTexGenParameterEXT },
  { "glBindTextureEXT", "GL_EXT_texture_object", glBindTextureEXT },
  { "glBindTextureUnit", "GL_ARB_direct_state_access GL_VERSION_4_5", glBindTextureUnit },
  { "glBindTextureUnitParameterEXT", "GL_EXT_vertex_shader", glBindTextureUnitParameterEXT },
  { "glBindTextures", "GL_ARB_multi_bind GL_VERSION_4_4", glBindTextures },
  { "glBindTransformFeedback", "GL_ARB_transform_feedback2 GL_VERSION_4_0", glBindTransformFeedback },
  { "glBindTransformFeedbackNV", "GL_NV_transform_feedback2", glBindTransformFeedbackNV },
  { "glBindVertexArray", "GL_ARB_vertex_array_object GL_VERSION_3_0", glBindVertexArray },
  { "glBindVertexArrayAPPLE", "GL_APPLE_vertex_array_object", glBindVertexArrayAPPLE },
  { "glBindVertexBuffer", "GL_ARB_vertex_attrib_binding GL_VERSION_4_3", glBindVertexBuffer },
  { "glBindVertexBuffers", "GL_ARB_multi_bind GL_VERSION_4_4", glBindVertexBuffers },
  { "glBindVertexShaderEXT", "GL_EXT_vertex_shader", glBindVertexShaderEXT },
  { "glBindVideoCaptureStreamBufferNV", "GL_NV_video_capture", glBindVideoCaptureStreamBufferNV },
  { "glBindVideoCaptureStreamTextureNV", "GL_NV_video_capture", glBindVideoCaptureStreamTextureNV },
  { "glBinormal3bEXT", "GL_EXT_coordinate_frame", glBinormal3bEXT },
  { "glBinormal3bvEXT", "GL_EXT_coordinate_frame", glBinormal3bvEXT },
  { "glBinormal3dEXT", "GL_EXT_coordinate_frame", glBinormal3dEXT },
  { "glBinormal3dvEXT", "GL_EXT_coordinate_frame", glBinormal3dvEXT },
  { "glBinormal3fEXT", "GL_EXT_coordinate_frame", glBinormal3fEXT },
  { "glBinormal3fvEXT", "GL_EXT_coordinate_frame", glBinormal3fvEXT },
  { "glBinormal3iEXT", "GL_EXT_coordinate_frame", glBinormal3iEXT },
  { "glBinormal3ivEXT", "GL_EXT_coordinate_frame", glBinormal3ivEXT },
  { "glBinormal3sEXT", "GL_EXT_coordinate_frame", glBinormal3sEXT },
  { "glBinormal3svEXT", "GL_EXT_coordinate_frame", glBinormal3svEXT },
  { "glBinormalPointerEXT", "GL_EXT_coordinate_frame", glBinormalPointerEXT },
  { "glBitmapxOES", "GL_OES_fixed_point", glBitmapxOES },
  { "glBlendBarrierKHR", "GL_KHR_blend_equation_advanced", glBlendBarrierKHR },
  { "glBlendBarrierNV", "GL_NV_blend_equation_advanced", glBlendBarrierNV },
  { "glBlendColor", "GL_ARB_imaging GL_VERSION_1_4", glBlendColor },
  { "glBlendColorEXT", "GL_EXT_blend_color", glBlendColorEXT },
  { "glBlendColorxOES", "GL_OES_fixed_point", glBlendColorxOES },
  { "glBlendEquation", "GL_ARB_imaging GL_VERSION_1_4", glBlendEquation },
  { "glBlendEquationEXT", "GL_EXT_blend_minmax", glBlendEquationEXT },
  { "glBlendEquationIndexedAMD", "GL_AMD_draw_buffers_blend", glBlendEquationIndexedAMD },
  { "glBlendEquationSeparate", "GL_VERSION_2_0", glBlendEquationSeparate },
  { "glBlendEquationSeparateEXT", "GL_EXT_blend_equation_separate", glBlendEquationSeparateEXT },
  { "glBlendEquationSeparateIndexedAMD", "GL_AMD_draw_buffers_blend", glBlendEquationSeparateIndexedAMD },
  { "glBlendEquationSeparatei", "GL_VERSION_4_0", glBlendEquationSeparatei },
  { "glBlendEquationSeparateiARB", "GL_ARB_draw_buffers_blend", glBlendEquationSeparateiARB },
  { "glBlendEquationi", "GL_VERSION_4_0", glBlendEquationi },
  { "glBlendEquationiARB", "GL_ARB_draw_buffers_blend", glBlendEquationiARB },
  { "glBlendFuncIndexedAMD", "GL_AMD_draw_buffers_blend", glBlendFuncIndexedAMD },
  { "glBlendFuncSeparate", "GL_VERSION_1_4", glBlendFuncSeparate },
  { "glBlendFuncSeparateEXT", "GL_EXT_blend_func_separate", glBlendFuncSeparateEXT },
  { "glBlendFuncSeparateINGR", "GL_INGR_blend_func_separate", glBlendFuncSeparateINGR },
  { "glBlendFuncSeparateIndexedAMD", "GL_AMD_draw_buffers_blend", glBlendFuncSeparateIndexedAMD },
  { "glBlendFuncSeparatei", "GL_VERSION_4_0", glBlendFuncSeparatei },
  { "glBlendFuncSeparateiARB", "GL_ARB_draw_buffers_blend", glBlendFuncSeparateiARB },
  { "glBlendFunci", "GL_VERSION_4_0", glBlendFunci },
  { "glBlendFunciARB", "GL_ARB_draw_buffers_blend", glBlendFunciARB },
  { "glBlendParameteriNV", "GL_NV_blend_equation_advanced", glBlendParameteriNV },
  { "glBlitFramebuffer", "GL_ARB_framebuffer_object GL_VERSION_3_0", glBlitFramebuffer },
  { "glBlitFramebufferEXT", "GL_EXT_framebuffer_blit", glBlitFramebufferEXT },
  { "glBlitNamedFramebuffer", "GL_ARB_direct_state_access GL_VERSION_4_5", glBlitNamedFramebuffer },
  { "glBufferAddressRangeNV", "GL_NV_vertex_buffer_unified_memory", glBufferAddressRangeNV },
  { "glBufferData", "GL_VERSION_1_5", glBufferData },
  { "glBufferDataARB", "GL_ARB_vertex_buffer_object", glBufferDataARB },
  { "glBufferPageCommitmentARB", "GL_ARB_sparse_buffer", glBufferPageCommitmentARB },
  { "glBufferParameteriAPPLE", "GL_APPLE_flush_buffer_range", glBufferParameteriAPPLE },
  { "glBufferRegionEnabled", "GL_KTX_buffer_region", glBufferRegionEnabled },
  { "glBufferStorage", "GL_ARB_buffer_storage GL_VERSION_4_4", glBufferStorage },
  { "glBufferStorageExternalEXT", "GL_EXT_external_buffer", glBufferStorageExternalEXT },
  { "glBufferStorageMemEXT", "GL_EXT_memory_object", glBufferStorageMemEXT },
  { "glBufferSubData", "GL_VERSION_1_5", glBufferSubData },
  { "glBufferSubDataARB", "GL_ARB_vertex_buffer_object", glBufferSubDataARB },
  { "glCallCommandListNV", "GL_NV_command_list", glCallCommandListNV },
  { "glCheckFramebufferStatus", "GL_ARB_framebuffer_object GL_VERSION_3_0", glCheckFramebufferStatus },
  { "glCheckFramebufferStatusEXT", "GL_EXT_framebuffer_object", glCheckFramebufferStatusEXT },
  { "glCheckNamedFramebufferStatus", "GL_ARB_direct_state_access GL_VERSION_4_5", glCheckNamedFramebufferStatus },
  { "glCheckNamedFramebufferStatusEXT", "GL_EXT_direct_state_access", glCheckNamedFramebufferStatusEXT },
  { "glClampColor", "GL_VERSION_3_0", glClampColor },
  { "glClampColorARB", "GL_ARB_color_buffer_float", glClampColorARB },
  { "glClearAccumxOES", "GL_OES_fixed_point", glClearAccumxOES },
  { "glClearBufferData", "GL_ARB_clear_buffer_object GL_VERSION_4_3", glClearBufferData },
  { "glClearBufferSubData", "GL_ARB_clear_buffer_object GL_VERSION_4_3", glClearBufferSubData },
  { "glClearBufferfi", "GL_VERSION_3_0", glClearBufferfi },
  { "glClearBufferfv", "GL_VERSION_3_0", glClearBufferfv },
  { "glClearBufferiv", "GL_VERSION_3_0", glClearBufferiv },
  { "glClearBufferuiv", "GL_VERSION_3_0", glClearBufferuiv },
  { "glClearColorIiEXT", "GL_EXT_texture_integer", glClearColorIiEXT },
  { "glClearColorIuiEXT", "GL_EXT_texture_integer", glClearColorIuiEXT },
  { "glClearColorxOES", "GL_OES_fixed_point", glClearColorxOES },
  { "glClearDepthdNV", "GL_NV_depth_buffer_float", glClearDepthdNV },
  { "glClearDepthf", "GL_ARB_ES2_compatibility GL_VERSION_4_1", glClearDepthf },
  { "glClearDepthfOES", "GL_OES_single_precision", glClearDepthfOES },
  { "glClearDepthxOES", "GL_OES_fixed_point", glClearDepthxOES },
  { "glClearNamedBufferData", "GL_ARB_direct_state_access GL_VERSION_4_5", glClearNamedBufferData },
  { "glClearNamedBufferDataEXT", "GL_EXT_direct_state_access", glClearNamedBufferDataEXT },
  { "glClearNamedBufferSubData", "GL_ARB_direct_state_access GL_VERSION_4_5", glClearNamedBufferSubData },
  { "glClearNamedBufferSubDataEXT", "GL_EXT_direct_state_access", glClearNamedBufferSubDataEXT },
  { "glClearNamedFramebufferfi", "GL_ARB_direct_state_access GL_VERSION_4_5", glClearNamedFramebufferfi },
  { "glClearNamedFramebufferfv", "GL_ARB_direct_state_access GL_VERSION_4_5", glClearNamedFramebufferfv },
  { "glClearNamedFramebufferiv", "GL_ARB_direct_state_access GL_VERSION_4_5", glClearNamedFramebufferiv },
  { "glClearNamedFramebufferuiv", "GL_ARB_direct_state_access GL_VERSION_4_5", glClearNamedFramebufferuiv },
  { "glClearTexImage", "GL_ARB_clear_texture GL_VERSION_4_4", glClearTexImage },
  { "glClearTexSubImage", "GL_ARB_clear_texture GL_VERSION_4_4", glClearTexSubImage },
  { "glClientActiveTexture", "GL_VERSION_1_3", glClientActiveTexture },
  { "glClientActiveTextureARB", "GL_ARB_multitexture", glClientActiveTextureARB },
  { "glClientActiveVertexStreamATI", "GL_ATI_vertex_streams", glClientActiveVertexStreamATI },
  { "glClientAttribDefaultEXT", "GL_EXT_direct_state_access", glClientAttribDefaultEXT },
  { "glClientWaitSync", "GL_ARB_sync GL_VERSION_3_2", glClientWaitSync },
  { "glClipControl", "GL_ARB_clip_control GL_VERSION_4_5", glClipControl },
  { "glClipPlanefOES", "GL_OES_single_precision", glClipPlanefOES },
  { "glClipPlanexOES", "GL_OES_fixed_point", glClipPlanexOES },
  { "glColor3fVertex3fSUN", "GL_SUN_vertex", glColor3fVertex3fSUN },
  { "glColor3fVertex3fvSUN", "GL_SUN_vertex", glColor3fVertex3fvSUN },
  { "glColor3hNV", "GL_NV_half_float", glColor3hNV },
  { "glColor3hvNV", "GL_NV_half_float", glColor3hvNV },
  { "glColor3xOES", "GL_OES_fixed_point", glColor3xOES },
  { "glColor3xvOES", "GL_OES_fixed_point", glColor3xvOES },
  { "glColor4fNormal3fVertex3fSUN", "GL_SUN_vertex", glColor4fNormal3fVertex3fSUN },
  { "glColor4fNormal3fVertex3fvSUN", "GL_SUN_vertex", glColor4fNormal3fVertex3fvSUN },
  { "glColor4hNV", "GL_NV_half_float", glColor4hNV },
  { "glColor4hvNV", "GL_NV_half_float", glColor4hvNV },
  { "glColor4ubVertex2fSUN", "GL_SUN_vertex", glColor4ubVertex2fSUN },
  { "glColor4ubVertex2fvSUN", "GL_SUN_vertex", glColor4ubVertex2fvSUN },
  { "glColor4ubVertex3fSUN", "GL_SUN_vertex", glColor4ubVertex3fSUN },
  { "glColor4ubVertex3fvSUN", "GL_SUN_vertex", glColor4ubVertex3fvSUN },
  { "glColor4xOES", "GL_OES_fixed_point", glColor4xOES },
  { "glColor4xvOES", "GL_OES_fixed_point", glColor4xvOES },
  { "glColorFormatNV", "GL_NV_vertex_buffer_unified_memory", glColorFormatNV },
  { "glColorFragmentOp1ATI", "GL_ATI_fragment_shader", glColorFragmentOp1ATI },
  { "glColorFragmentOp2ATI", "GL_ATI_fragment_shader", glColorFragmentOp2ATI },
  { "glColorFragmentOp3ATI", "GL_ATI_fragment_shader", glColorFragmentOp3ATI },
  { "glColorMaskIndexedEXT", "GL_EXT_draw_buffers2", glColorMaskIndexedEXT },
  { "glColorMaski", "GL_VERSION_3_0", glColorMaski },
  { "glColorP3ui", "GL_ARB_vertex_type_2_10_10_10_rev GL_VERSION_3_3", glColorP3ui },
  { "glColorP3uiv", "GL_ARB_vertex_type_2_10_10_10_rev GL_VERSION_3_3", glColorP3uiv },
  { "glColorP4ui", "GL_ARB_vertex_type_2_10_10_10_rev GL_VERSION_3_3", glColorP4ui },
  { "glColorP4uiv", "GL_ARB_vertex_type_2_10_10_10_rev GL_VERSION_3_3", glColorP4uiv },
  { "glColorPointerEXT", "GL_EXT_vertex_array", glColorPointerEXT },
  { "glColorPointerListIBM", "GL_IBM_vertex_array_lists", glColorPointerListIBM },
  { "glColorPointervINTEL", "GL_INTEL_parallel_arrays", glColorPointervINTEL },
  { "glColorSubTable", "GL_ARB_imaging", glColorSubTable },
  { "glColorSubTableEXT", "GL_EXT_color_subtable", glColorSubTableEXT },
  { "glColorTable", "GL_ARB_imaging", glColorTable },
  { "glColorTableEXT", "GL_EXT_paletted_texture", glColorTableEXT },
  { "glColorTableParameterfv", "GL_ARB_imaging", glColorTableParameterfv },
  { "glColorTableParameterfvSGI", "GL_SGI_color_table", glColorTableParameterfvSGI },
  { "glColorTableParameteriv", "GL_ARB_imaging", glColorTableParameteriv },
  { "glColorTableParameterivSGI", "GL_SGI_color_table", glColorTableParameterivSGI },
  { "glColorTableSGI", "GL_SGI_color_table", glColorTableSGI },
  { "glCombinerInputNV", "GL_NV_register_combiners", glCombinerInputNV },
  { "glCombinerOutputNV", "GL_NV_register_combiners", glCombinerOutputNV },
  { "glCombinerParameterfNV", "GL_NV_register_combiners", glCombinerParameterfNV },
  { "glCombinerParameterfvNV", "GL_NV_register_combiners", glCombinerParameterfvNV },
  { "glCombinerParameteriNV", "GL_NV_register_combiners", glCombinerParameteriNV },
  { "glCombinerParameterivNV", "GL_NV_register_combiners", glCombinerParameterivNV },
  { "glCombinerStageParameterfvNV", "GL_NV_register_combiners2", glCombinerStageParameterfvNV },
  { "glCommandListSegmentsNV", "GL_NV_command_list", glCommandListSegmentsNV },
  { "glCompileCommandListNV", "GL_NV_command_list", glCompileCommandListNV },
  { "glCompileShader", "GL_VERSION_2_0", glCompileShader },
  { "glCompileShaderARB", "GL_ARB_shader_objects", glCompileShaderARB },
  { "glCompileShaderIncludeARB", "GL_ARB_shading_language_include", glCompileShaderIncludeARB },
  { "glCompressedMultiTexImage1DEXT", "GL_EXT_direct_state_access", glCompressedMultiTexImage1DEXT },
  { "glCompressedMultiTexImage2DEXT", "GL_EXT_direct_state_access", glCompressedMultiTexImage2DEXT },
  { "glCompressedMultiTexImage3DEXT", "GL_EXT_direct_state_access", glCompressedMultiTexImage3DEXT },
  { "glCompressedMultiTexSubImage1DEXT", "GL_EXT_direct_state_access", glCompressedMultiTexSubImage1DEXT },
  { "glCompressedMultiTexSubImage2DEXT", "GL_EXT_direct_state_access", glCompressedMultiTexSubImage2DEXT },
  { "glCompressedMultiTexSubImage3DEXT", "GL_EXT_direct_state_access", glCompressedMultiTexSubImage3DEXT },
  { "glCompressedTexImage1D", "GL_VERSION_1_3", glCompressedTexImage1D },
  { "glCompressedTexImage1DARB", "GL_ARB_texture_compression", glCompressedTexImage1DARB },
  { "glCompressedTexImage2D", "GL_VERSION_1_3", glCompressedTexImage2D },
  { "glCompressedTexImage2DARB", "GL_ARB_texture_compression", glCompressedTexImage2DARB },
  { "glCompressedTexImage3D", "GL_VERSION_1_3", glCompressedTexImage3D },
  { "glCompressedTexImage3DARB", "GL_ARB_texture_compression", glCompressedTexImage3DARB },
  { "glCompressedTexSubImage1D", "GL_VERSION_1_3", glCompressedTexSubImage1D },
  { "glCompressedTexSubImage1DARB", "GL_ARB_texture_compression", glCompressedTexSubImage1DARB },
  { "glCompressedTexSubImage2D", "GL_VERSION_1_3", glCompressedTexSubImage2D },
  { "glCompressedTexSubImage2DARB", "GL_ARB_texture_compression", glCompressedTexSubImage2DARB },
  { "glCompressedTexSubImage3D", "GL_VERSION_1_3", glCompressedTexSubImage3D },
  { "glCompressedTexSubImage3DARB", "GL_ARB_texture_compression", glCompressedTexSubImage3DARB },
  { "glCompressedTextureImage1DEXT", "GL_EXT_direct_state_access", glCompressedTextureImage1DEXT },
  { "glCompressedTextureImage2DEXT", "GL_EXT_direct_state_access", glCompressedTextureImage2DEXT },
  { "glCompressedTextureImage3DEXT", "GL_EXT_direct_state_access", glCompressedTextureImage3DEXT },
  { "glCompressedTextureSubImage1D", "GL_ARB_direct_state_access GL_VERSION_4_5", glCompressedTextureSubImage1D },
  { "glCompressedTextureSubImage1DEXT", "GL_EXT_direct_state_access", glCompressedTextureSubImage1DEXT },
  { "glCompressedTextureSubImage2D", "GL_ARB_direct_state_access GL_VERSION_4_5", glCompressedTextureSubImage2D },
  { "glCompressedTextureSubImage2DEXT", "GL_EXT_direct_state_access", glCompressedTextureSubImage2DEXT },
  { "glCompressedTextureSubImage3D", "GL_ARB_direct_state_access GL_VERSION_4_5", glCompressedTextureSubImage3D },
  { "glCompressedTextureSubImage3DEXT", "GL_EXT_direct_state_access", glCompressedTextureSubImage3DEXT },
  { "glConservativeRasterParameterfNV", "GL_NV_conservative_raster_dilate", glConservativeRasterParameterfNV },
  { "glConservativeRasterParameteriNV", "GL_NV_conservative_raster_pre_snap_triangles", glConservativeRasterParameteriNV },
  { "glConvolutionFilter1D", "GL_ARB_imaging", glConvolutionFilter1D },
  { "glConvolutionFilter1DEXT", "GL_EXT_convolution", glConvolutionFilter1DEXT },
  { "glConvolutionFilter2D", "GL_ARB_imaging", glConvolutionFilter2D },
  { "glConvolutionFilter2DEXT", "GL_EXT_convolution", glConvolutionFilter2DEXT },
  { "glConvolutionParameterf", "GL_ARB_imaging", glConvolutionParameterf },
  { "glConvolutionParameterfEXT", "GL_EXT_convolution", glConvolutionParameterfEXT },
  { "glConvolutionParameterfv", "GL_ARB_imaging", glConvolutionParameterfv },
  { "glConvolutionParameterfvEXT", "GL_EXT_convolution", glConvolutionParameterfvEXT },
  { "glConvolutionParameteri", "GL_ARB_imaging", glConvolutionParameteri },
  { "glConvolutionParameteriEXT", "GL_EXT_convolution", glConvolutionParameteriEXT },
  { "glConvolutionParameteriv", "GL_ARB_imaging", glConvolutionParameteriv },
  { "glConvolutionParameterivEXT", "GL_EXT_convolution", glConvolutionParameterivEXT },
  { "glConvolutionParameterxOES", "GL_OES_fixed_point", glConvolutionParameterxOES },
  { "glConvolutionParameterxvOES", "GL_OES_fixed_point", glConvolutionParameterxvOES },
  { "glCopyBufferSubData", "GL_ARB_copy_buffer GL_VERSION_3_1", glCopyBufferSubData },
  { "glCopyColorSubTable", "GL_ARB_imaging", glCopyColorSubTable },
  { "glCopyColorSubTableEXT", "GL_EXT_color_subtable", glCopyColorSubTableEXT },
  { "glCopyColorTable", "GL_ARB_imaging", glCopyColorTable },
  { "glCopyColorTableSGI", "GL_SGI_color_table", glCopyColorTableSGI },
  { "glCopyConvolutionFilter1D", "GL_ARB_imaging", glCopyConvolutionFilter1D },
  { "glCopyConvolutionFilter1DEXT", "GL_EXT_convolution", glCopyConvolutionFilter1DEXT },
  { "glCopyConvolutionFilter2D", "GL_ARB_imaging", glCopyConvolutionFilter2D },
  { "glCopyConvolutionFilter2DEXT", "GL_EXT_convolution", glCopyConvolutionFilter2DEXT },
  { "glCopyImageSubData", "GL_ARB_copy_image GL_VERSION_4_3", glCopyImageSubData },
  { "glCopyImageSubDataNV", "GL_NV_copy_image", glCopyImageSubDataNV },
  { "glCopyMultiTexImage1DEXT", "GL_EXT_direct_state_access", glCopyMultiTexImage1DEXT },
  { "glCopyMultiTexImage2DEXT", "GL_EXT_direct_state_access", glCopyMultiTexImage2DEXT },
  { "glCopyMultiTexSubImage1DEXT", "GL_EXT_direct_state_access", glCopyMultiTexSubImage1DEXT },
  { "glCopyMultiTexSubImage2DEXT", "GL_EXT_direct_state_access", glCopyMultiTexSubImage2DEXT },
  { "glCopyMultiTexSubImage3DEXT", "GL_EXT_direct_state_access", glCopyMultiTexSubImage3DEXT },
  { "glCopyNamedBufferSubData", "GL_ARB_direct_state_access GL_VERSION_4_5", glCopyNamedBufferSubData },
  { "glCopyPathNV", "GL_NV_path_rendering", glCopyPathNV },
  { "glCopyTexImage1DEXT", "GL_EXT_copy_texture", glCopyTexImage1DEXT },
  { "glCopyTexImage2DEXT", "GL_EXT_copy_texture", glCopyTexImage2DEXT },
  { "glCopyTexSubImage1DEXT", "GL_EXT_copy_texture", glCopyTexSubImage1DEXT },
  { "glCopyTexSubImage2DEXT", "GL_EXT_copy_texture", glCopyTexSubImage2DEXT },
  { "glCopyTexSubImage3D", "GL_VERSION_1_2", glCopyTexSubImage3D },
  { "glCopyTexSubImage3DEXT", "GL_EXT_copy_texture", glCopyTexSubImage3DEXT },
  { "glCopyTextureImage1DEXT", "GL_EXT_direct_state_access", glCopyTextureImage1DEXT },
  { "glCopyTextureImage2DEXT", "GL_EXT_direct_state_access", glCopyTextureImage2DEXT },
  { "glCopyTextureSubImage1D", "GL_ARB_direct_state_access GL_VERSION_4_5", glCopyTextureSubImage1D },
  { "glCopyTextureSubImage1DEXT", "GL_EXT_direct_state_access", glCopyTextureSubImage1DEXT },
  { "glCopyTextureSubImage2D", "GL_ARB_direct_state_access GL_VERSION_4_5", glCopyTextureSubImage2D },
  { "glCopyTextureSubImage2DEXT", "GL_EXT_direct_state_access", glCopyTextureSubImage2DEXT },
  { "glCopyTextureSubImage3D", "GL_ARB_direct_state_access GL_VERSION_4_5", glCopyTextureSubImage3D },
  { "glCopyTextureSubImage3DEXT", "GL_EXT_direct_state_access", glCopyTextureSubImage3DEXT },
  { "glCoverFillPathInstancedNV", "GL_NV_path_rendering", glCoverFillPathInstancedNV },
  { "glCoverFillPathNV", "GL_NV_path_rendering", glCoverFillPathNV },
  { "glCoverStrokePathInstancedNV", "GL_NV_path_rendering", glCoverStrokePathInstancedNV },
  { "glCoverStrokePathNV", "GL_NV_path_rendering", glCoverStrokePathNV },
  { "glCoverageModulationNV", "GL_NV_framebuffer_mixed_samples", glCoverageModulationNV },
  { "glCoverageModulationTableNV", "GL_NV_framebuffer_mixed_samples", glCoverageModulationTableNV },
  { "glCreateBuffers", "GL_ARB_direct_state_access GL_VERSION_4_5", glCreateBuffers },
  { "glCreateCommandListsNV", "GL_NV_command_list", glCreateCommandListsNV },
  { "glCreateFramebuffers", "GL_ARB_direct_state_access GL_VERSION_4_5", glCreateFramebuffers },
  { "glCreateMemoryObjectsEXT", "GL_EXT_memory_object", glCreateMemoryObjectsEXT },
  { "glCreatePerfQueryINTEL", "GL_INTEL_performance_query", glCreatePerfQueryINTEL },
  { "glCreateProgram", "GL_VERSION_2_0", glCreateProgram },
  { "glCreateProgramObjectARB", "GL_ARB_shader_objects", glCreateProgramObjectARB },
  { "glCreateProgramPipelines", "GL_ARB_direct_state_access GL_VERSION_4_5", glCreateProgramPipelines },
  { "glCreateQueries", "GL_ARB_direct_state_access GL_VERSION_4_5", glCreateQueries },
  { "glCreateRenderbuffers", "GL_ARB_direct_state_access GL_VERSION_4_5", glCreateRenderbuffers },
  { "glCreateSamplers", "GL_ARB_direct_state_access GL_VERSION_4_5", glCreateSamplers },
  { "glCreateShader", "GL_VERSION_2_0", glCreateShader },
  { "glCreateShaderObjectARB", "GL_ARB_shader_objects", glCreateShaderObjectARB },
  { "glCreateShaderProgramEXT", "GL_EXT_separate_shader_objects", glCreateShaderProgramEXT },
  { "glCreateShaderProgramv", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glCreateShaderProgramv },
  { "glCreateStatesNV", "GL_NV_command_list", glCreateStatesNV },
  { "glCreateSyncFromCLeventARB", "GL_ARB_cl_event", glCreateSyncFromCLeventARB },
  { "glCreateTextures", "GL_ARB_direct_state_access GL_VERSION_4_5", glCreateTextures },
  { "glCreateTransformFeedbacks", "GL_ARB_direct_state_access GL_VERSION_4_5", glCreateTransformFeedbacks },
  { "glCreateVertexArrays", "GL_ARB_direct_state_access GL_VERSION_4_5", glCreateVertexArrays },
  { "glCullParameterdvEXT", "GL_EXT_cull_vertex", glCullParameterdvEXT },
  { "glCullParameterfvEXT", "GL_EXT_cull_vertex", glCullParameterfvEXT },
  { "glCurrentPaletteMatrixARB", "GL_ARB_matrix_palette", glCurrentPaletteMatrixARB },
  { "glDebugMessageCallback", "GL_KHR_debug GL_VERSION_4_3", glDebugMessageCallback },
  { "glDebugMessageCallbackAMD", "GL_AMD_debug_output", glDebugMessageCallbackAMD },
  { "glDebugMessageCallbackARB", "GL_ARB_debug_output", glDebugMessageCallbackARB },
  { "glDebugMessageControl", "GL_KHR_debug GL_VERSION_4_3", glDebugMessageControl },
  { "glDebugMessageControlARB", "GL_ARB_debug_output", glDebugMessageControlARB },
  { "glDebugMessageEnableAMD", "GL_AMD_debug_output", glDebugMessageEnableAMD },
  { "glDebugMessageInsert", "GL_KHR_debug GL_VERSION_4_3", glDebugMessageInsert },
  { "glDebugMessageInsertAMD", "GL_AMD_debug_output", glDebugMessageInsertAMD },
  { "glDebugMessageInsertARB", "GL_ARB_debug_output", glDebugMessageInsertARB },
  { "glDeformSGIX", "GL_SGIX_polynomial_ffd", glDeformSGIX },
  { "glDeformationMap3dSGIX", "GL_SGIX_polynomial_ffd", glDeformationMap3dSGIX },
  { "glDeformationMap3fSGIX", "GL_SGIX_polynomial_ffd", glDeformationMap3fSGIX },
  { "glDeleteAsyncMarkersSGIX", "GL_SGIX_async", glDeleteAsyncMarkersSGIX },
  { "glDeleteBufferRegion", "GL_KTX_buffer_region", glDeleteBufferRegion },
  { "glDeleteBuffers", "GL_VERSION_1_5", glDeleteBuffers },
  { "glDeleteBuffersARB", "GL_ARB_vertex_buffer_object", glDeleteBuffersARB },
  { "glDeleteCommandListsNV", "GL_NV_command_list", glDeleteCommandListsNV },
  { "glDeleteFencesAPPLE", "GL_APPLE_fence", glDeleteFencesAPPLE },
  { "glDeleteFencesNV", "GL_NV_fence", glDeleteFencesNV },
  { "glDeleteFragmentShaderATI", "GL_ATI_fragment_shader", glDeleteFragmentShaderATI },
  { "glDeleteFramebuffers", "GL_ARB_framebuffer_object GL_VERSION_3_0", glDeleteFramebuffers },
  { "glDeleteFramebuffersEXT", "GL_EXT_framebuffer_object", glDeleteFramebuffersEXT },
  { "glDeleteMemoryObjectsEXT", "GL_EXT_memory_object", glDeleteMemoryObjectsEXT },
  { "glDeleteNamedStringARB", "GL_ARB_shading_language_include", glDeleteNamedStringARB },
  { "glDeleteNamesAMD", "GL_AMD_name_gen_delete", glDeleteNamesAMD },
  { "glDeleteObjectARB", "GL_ARB_shader_objects", glDeleteObjectARB },
  { "glDeleteObjectBufferATI", "GL_ATI_vertex_array_object", glDeleteObjectBufferATI },
  { "glDeleteOcclusionQueriesNV", "GL_NV_occlusion_query", glDeleteOcclusionQueriesNV },
  { "glDeletePathsNV", "GL_NV_path_rendering", glDeletePathsNV },
  { "glDeletePerfMonitorsAMD", "GL_AMD_performance_monitor", glDeletePerfMonitorsAMD },
  { "glDeletePerfQueryINTEL", "GL_INTEL_performance_query", glDeletePerfQueryINTEL },
  { "glDeleteProgram", "GL_VERSION_2_0", glDeleteProgram },
  { "glDeleteProgramPipelines", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glDeleteProgramPipelines },
  { "glDeleteProgramsARB", "GL_ARB_fragment_program GL_ARB_vertex_program", glDeleteProgramsARB },
  { "glDeleteProgramsNV", "GL_NV_vertex_program", glDeleteProgramsNV },
  { "glDeleteQueries", "GL_VERSION_1_5", glDeleteQueries },
  { "glDeleteQueriesARB", "GL_ARB_occlusion_query", glDeleteQueriesARB },
  { "glDeleteQueryResourceTagNV", "GL_NV_query_resource_tag", glDeleteQueryResourceTagNV },
  { "glDeleteRenderbuffers", "GL_ARB_framebuffer_object GL_VERSION_3_0", glDeleteRenderbuffers },
  { "glDeleteRenderbuffersEXT", "GL_EXT_framebuffer_object", glDeleteRenderbuffersEXT },
  { "glDeleteSamplers", "GL_ARB_sampler_objects GL_VERSION_3_3", glDeleteSamplers },
  { "glDeleteSemaphoresEXT", "GL_EXT_semaphore", glDeleteSemaphoresEXT },
  { "glDeleteShader", "GL_VERSION_2_0", glDeleteShader },
  { "glDeleteStatesNV", "GL_NV_command_list", glDeleteStatesNV },
  { "glDeleteSync", "GL_ARB_sync GL_VERSION_3_2", glDeleteSync },
  { "glDeleteTexturesEXT", "GL_EXT_texture_object", glDeleteTexturesEXT },
  { "glDeleteTransformFeedbacks", "GL_ARB_transform_feedback2 GL_VERSION_4_0", glDeleteTransformFeedbacks },
  { "glDeleteTransformFeedbacksNV", "GL_NV_transform_feedback2", glDeleteTransformFeedbacksNV },
  { "glDeleteVertexArrays", "GL_ARB_vertex_array_object GL_VERSION_3_0", glDeleteVertexArrays },
  { "glDeleteVertexArraysAPPLE", "GL_APPLE_vertex_array_object", glDeleteVertexArraysAPPLE },
  { "glDeleteVertexShaderEXT", "GL_EXT_vertex_shader", glDeleteVertexShaderEXT },
  { "glDepthBoundsEXT", "GL_EXT_depth_bounds_test", glDepthBoundsEXT },
  { "glDepthBoundsdNV", "GL_NV_depth_buffer_float", glDepthBoundsdNV },
  { "glDepthRangeArrayv", "GL_ARB_viewport_array GL_VERSION_4_1", glDepthRangeArrayv },
  { "glDepthRangeIndexed", "GL_ARB_viewport_array GL_VERSION_4_1", glDepthRangeIndexed },
  { "glDepthRangedNV", "GL_NV_depth_buffer_float", glDepthRangedNV },
  { "glDepthRangef", "GL_ARB_ES2_compatibility GL_VERSION_4_1", glDepthRangef },
  { "glDepthRangefOES", "GL_OES_single_precision", glDepthRangefOES },
  { "glDepthRangexOES", "GL_OES_fixed_point", glDepthRangexOES },
  { "glDetachObjectARB", "GL_ARB_shader_objects", glDetachObjectARB },
  { "glDetachShader", "GL_VERSION_2_0", glDetachShader },
  { "glDetailTexFuncSGIS", "GL_SGIS_detail_texture", glDetailTexFuncSGIS },
  { "glDisableClientStateIndexedEXT", "GL_EXT_direct_state_access", glDisableClientStateIndexedEXT },
  { "glDisableClientStateiEXT", "GL_EXT_direct_state_access", glDisableClientStateiEXT },
  { "glDisableIndexedEXT", "GL_EXT_direct_state_access GL_EXT_draw_buffers2", glDisableIndexedEXT },
  { "glDisableVariantClientStateEXT", "GL_EXT_vertex_shader", glDisableVariantClientStateEXT },
  { "glDisableVertexArrayAttrib", "GL_ARB_direct_state_access GL_VERSION_4_5", glDisableVertexArrayAttrib },
  { "glDisableVertexArrayAttribEXT", "GL_EXT_direct_state_access", glDisableVertexArrayAttribEXT },
  { "glDisableVertexArrayEXT", "GL_EXT_direct_state_access", glDisableVertexArrayEXT },
  { "glDisableVertexAttribAPPLE", "GL_APPLE_vertex_program_evaluators", glDisableVertexAttribAPPLE },
  { "glDisableVertexAttribArray", "GL_VERSION_2_0", glDisableVertexAttribArray },
  { "glDisableVertexAttribArrayARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glDisableVertexAttribArrayARB },
  { "glDisablei", "GL_VERSION_3_0", glDisablei },
  { "glDispatchCompute", "GL_ARB_compute_shader GL_VERSION_4_3", glDispatchCompute },
  { "glDispatchComputeGroupSizeARB", "GL_ARB_compute_variable_group_size", glDispatchComputeGroupSizeARB },
  { "glDispatchComputeIndirect", "GL_ARB_compute_shader GL_VERSION_4_3", glDispatchComputeIndirect },
  { "glDrawArraysEXT", "GL_EXT_vertex_array", glDrawArraysEXT },
  { "glDrawArraysIndirect", "GL_ARB_draw_indirect GL_VERSION_4_0", glDrawArraysIndirect },
  { "glDrawArraysInstanced", "GL_VERSION_3_1", glDrawArraysInstanced },
  { "glDrawArraysInstancedARB", "GL_ARB_draw_instanced", glDrawArraysInstancedARB },
  { "glDrawArraysInstancedBaseInstance", "GL_ARB_base_instance GL_VERSION_4_2", glDrawArraysInstancedBaseInstance },
  { "glDrawArraysInstancedEXT", "GL_EXT_draw_instanced", glDrawArraysInstancedEXT },
  { "glDrawBufferRegion", "GL_KTX_buffer_region", glDrawBufferRegion },
  { "glDrawBuffers", "GL_VERSION_2_0", glDrawBuffers },
  { "glDrawBuffersARB", "GL_ARB_draw_buffers", glDrawBuffersARB },
  { "glDrawBuffersATI", "GL_ATI_draw_buffers", glDrawBuffersATI },
  { "glDrawCommandsAddressNV", "GL_NV_command_list", glDrawCommandsAddressNV },
  { "glDrawCommandsNV", "GL_NV_command_list", glDrawCommandsNV },
  { "glDrawCommandsStatesAddressNV", "GL_NV_command_list", glDrawCommandsStatesAddressNV },
  { "glDrawCommandsStatesNV", "GL_NV_command_list", glDrawCommandsStatesNV },
  { "glDrawElementArrayAPPLE", "GL_APPLE_element_array", glDrawElementArrayAPPLE },
  { "glDrawElementArrayATI", "GL_ATI_element_array", glDrawElementArrayATI },
  { "glDrawElementsBaseVertex", "GL_ARB_draw_elements_base_vertex GL_VERSION_3_2", glDrawElementsBaseVertex },
  { "glDrawElementsIndirect", "GL_ARB_draw_indirect GL_VERSION_4_0", glDrawElementsIndirect },
  { "glDrawElementsInstanced", "GL_VERSION_3_1", glDrawElementsInstanced },
  { "glDrawElementsInstancedARB", "GL_ARB_draw_instanced", glDrawElementsInstancedARB },
  { "glDrawElementsInstancedBaseInstance", "GL_ARB_base_instance GL_VERSION_4_2", glDrawElementsInstancedBaseInstance },
  { "glDrawElementsInstancedBaseVertex", "GL_ARB_draw_elements_base_vertex GL_VERSION_3_2", glDrawElementsInstancedBaseVertex },
  { "glDrawElementsInstancedBaseVertexBaseInstance", "GL_ARB_base_instance GL_VERSION_4_2", glDrawElementsInstancedBaseVertexBaseInstance },
  { "glDrawElementsInstancedEXT", "GL_EXT_draw_instanced", glDrawElementsInstancedEXT },
  { "glDrawMeshArraysSUN", "GL_SUN_mesh_array", glDrawMeshArraysSUN },
  { "glDrawRangeElementArrayAPPLE", "GL_APPLE_element_array", glDrawRangeElementArrayAPPLE },
  { "glDrawRangeElementArrayATI", "GL_ATI_element_array", glDrawRangeElementArrayATI },
  { "glDrawRangeElements", "GL_VERSION_1_2", glDrawRangeElements },
  { "glDrawRangeElementsBaseVertex", "GL_ARB_draw_elements_base_vertex GL_VERSION_3_2", glDrawRangeElementsBaseVertex },
  { "glDrawRangeElementsEXT", "GL_EXT_draw_range_elements", glDrawRangeElementsEXT },
  { "glDrawTextureNV", "GL_NV_draw_texture", glDrawTextureNV },
  { "glDrawTransformFeedback", "GL_ARB_transform_feedback2 GL_VERSION_4_0", glDrawTransformFeedback },
  { "glDrawTransformFeedbackInstanced", "GL_ARB_transform_feedback_instanced GL_VERSION_4_2", glDrawTransformFeedbackInstanced },
  { "glDrawTransformFeedbackNV", "GL_NV_transform_feedback2", glDrawTransformFeedbackNV },
  { "glDrawTransformFeedbackStream", "GL_ARB_transform_feedback3 GL_VERSION_4_0", glDrawTransformFeedbackStream },
  { "glDrawTransformFeedbackStreamInstanced", "GL_ARB_transform_feedback_instanced GL_VERSION_4_2", glDrawTransformFeedbackStreamInstanced },
  { "glDrawVkImageNV", "GL_NV_draw_vulkan_image", glDrawVkImageNV },
  { "glEdgeFlagFormatNV", "GL_NV_vertex_buffer_unified_memory", glEdgeFlagFormatNV },
  { "glEdgeFlagPointerEXT", "GL_EXT_vertex_array", glEdgeFlagPointerEXT },
  { "glEdgeFlagPointerListIBM", "GL_IBM_vertex_array_lists", glEdgeFlagPointerListIBM },
  { "glElementPointerAPPLE", "GL_APPLE_element_array", glElementPointerAPPLE },
  { "glElementPointerATI", "GL_ATI_element_array", glElementPointerATI },
  { "glEnableClientStateIndexedEXT", "GL_EXT_direct_state_access", glEnableClientStateIndexedEXT },
  { "glEnableClientStateiEXT", "GL_EXT_direct_state_access", glEnableClientStateiEXT },
  { "glEnableIndexedEXT", "GL_EXT_direct_state_access GL_EXT_draw_buffers2", glEnableIndexedEXT },
  { "glEnableVariantClientStateEXT", "GL_EXT_vertex_shader", glEnableVariantClientStateEXT },
  { "glEnableVertexArrayAttrib", "GL_ARB_direct_state_access GL_VERSION_4_5", glEnableVertexArrayAttrib },
  { "glEnableVertexArrayAttribEXT", "GL_EXT_direct_state_access", glEnableVertexArrayAttribEXT },
  { "glEnableVertexArrayEXT", "GL_EXT_direct_state_access", glEnableVertexArrayEXT },
  { "glEnableVertexAttribAPPLE", "GL_APPLE_vertex_program_evaluators", glEnableVertexAttribAPPLE },
  { "glEnableVertexAttribArray", "GL_VERSION_2_0", glEnableVertexAttribArray },
  { "glEnableVertexAttribArrayARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glEnableVertexAttribArrayARB },
  { "glEnablei", "GL_VERSION_3_0", glEnablei },
  { "glEndConditionalRender", "GL_VERSION_3_0", glEndConditionalRender },
  { "glEndConditionalRenderNV", "GL_NV_conditional_render", glEndConditionalRenderNV },
  { "glEndConditionalRenderNVX", "GL_NVX_conditional_render", glEndConditionalRenderNVX },
  { "glEndFragmentShaderATI", "GL_ATI_fragment_shader", glEndFragmentShaderATI },
  { "glEndOcclusionQueryNV", "GL_NV_occlusion_query", glEndOcclusionQueryNV },
  { "glEndPerfMonitorAMD", "GL_AMD_performance_monitor", glEndPerfMonitorAMD },
  { "glEndPerfQueryINTEL", "GL_INTEL_performance_query", glEndPerfQueryINTEL },
  { "glEndQuery", "GL_VERSION_1_5", glEndQuery },
  { "glEndQueryARB", "GL_ARB_occlusion_query", glEndQueryARB },
  { "glEndQueryIndexed", "GL_ARB_transform_feedback3 GL_VERSION_4_0", glEndQueryIndexed },
  { "glEndTransformFeedback", "GL_VERSION_3_0", glEndTransformFeedback },
  { "glEndTransformFeedbackEXT", "GL_EXT_transform_feedback", glEndTransformFeedbackEXT },
  { "glEndTransformFeedbackNV", "GL_NV_transform_feedback", glEndTransformFeedbackNV },
  { "glEndVertexShaderEXT", "GL_EXT_vertex_shader", glEndVertexShaderEXT },
  { "glEndVideoCaptureNV", "GL_NV_video_capture", glEndVideoCaptureNV },
  { "glEvalCoord1xOES", "GL_OES_fixed_point", glEvalCoord1xOES },
  { "glEvalCoord1xvOES", "GL_OES_fixed_point", glEvalCoord1xvOES },
  { "glEvalCoord2xOES", "GL_OES_fixed_point", glEvalCoord2xOES },
  { "glEvalCoord2xvOES", "GL_OES_fixed_point", glEvalCoord2xvOES },
  { "glEvalMapsNV", "GL_NV_evaluators", glEvalMapsNV },
  { "glEvaluateDepthValuesARB", "GL_ARB_sample_locations", glEvaluateDepthValuesARB },
  { "glExecuteProgramNV", "GL_NV_vertex_program", glExecuteProgramNV },
  { "glExtractComponentEXT", "GL_EXT_vertex_shader", glExtractComponentEXT },
  { "glFeedbackBufferxOES", "GL_OES_fixed_point", glFeedbackBufferxOES },
  { "glFenceSync", "GL_ARB_sync GL_VERSION_3_2", glFenceSync },
  { "glFinalCombinerInputNV", "GL_NV_register_combiners", glFinalCombinerInputNV },
  { "glFinishAsyncSGIX", "GL_SGIX_async", glFinishAsyncSGIX },
  { "glFinishFenceAPPLE", "GL_APPLE_fence", glFinishFenceAPPLE },
  { "glFinishFenceNV", "GL_NV_fence", glFinishFenceNV },
  { "glFinishObjectAPPLE", "GL_APPLE_fence", glFinishObjectAPPLE },
  { "glFinishTextureSUNX", "GL_SUNX_constant_data", glFinishTextureSUNX },
  { "glFlushMappedBufferRange", "GL_ARB_map_buffer_range GL_VERSION_3_0", glFlushMappedBufferRange },
  { "glFlushMappedBufferRangeAPPLE", "GL_APPLE_flush_buffer_range", glFlushMappedBufferRangeAPPLE },
  { "glFlushMappedNamedBufferRange", "GL_ARB_direct_state_access GL_VERSION_4_5", glFlushMappedNamedBufferRange },
  { "glFlushMappedNamedBufferRangeEXT", "GL_EXT_direct_state_access", glFlushMappedNamedBufferRangeEXT },
  { "glFlushPixelDataRangeNV", "GL_NV_pixel_data_range", glFlushPixelDataRangeNV },
  { "glFlushRasterSGIX", "GL_SGIX_flush_raster", glFlushRasterSGIX },
  { "glFlushStaticDataIBM", "GL_IBM_static_data", glFlushStaticDataIBM },
  { "glFlushVertexArrayRangeAPPLE", "GL_APPLE_vertex_array_range", glFlushVertexArrayRangeAPPLE },
  { "glFlushVertexArrayRangeNV", "GL_NV_vertex_array_range", glFlushVertexArrayRangeNV },
  { "glFogCoordFormatNV", "GL_NV_vertex_buffer_unified_memory", glFogCoordFormatNV },
  { "glFogCoordPointer", "GL_VERSION_1_4", glFogCoordPointer },
  { "glFogCoordPointerEXT", "GL_EXT_fog_coord", glFogCoordPointerEXT },
  { "glFogCoordPointerListIBM", "GL_IBM_vertex_array_lists", glFogCoordPointerListIBM },
  { "glFogCoordd", "GL_VERSION_1_4", glFogCoordd },
  { "glFogCoorddEXT", "GL_EXT_fog_coord", glFogCoorddEXT },
  { "glFogCoorddv", "GL_VERSION_1_4", glFogCoorddv },
  { "glFogCoorddvEXT", "GL_EXT_fog_coord", glFogCoorddvEXT },
  { "glFogCoordf", "GL_VERSION_1_4", glFogCoordf },
  { "glFogCoordfEXT", "GL_EXT_fog_coord", glFogCoordfEXT },
  { "glFogCoordfv", "GL_VERSION_1_4", glFogCoordfv },
  { "glFogCoordfvEXT", "GL_EXT_fog_coord", glFogCoordfvEXT },
  { "glFogCoordhNV", "GL_NV_half_float", glFogCoordhNV },
  { "glFogCoordhvNV", "GL_NV_half_float", glFogCoordhvNV },
  { "glFogFuncSGIS", "GL_SGIS_fog_function", glFogFuncSGIS },
  { "glFogxOES", "GL_OES_fixed_point", glFogxOES },
  { "glFogxvOES", "GL_OES_fixed_point", glFogxvOES },
  { "glFragmentColorMaterialSGIX", "GL_SGIX_fragment_lighting", glFragmentColorMaterialSGIX },
  { "glFragmentCoverageColorNV", "GL_NV_fragment_coverage_to_color", glFragmentCoverageColorNV },
  { "glFragmentLightModelfSGIX", "GL_SGIX_fragment_lighting", glFragmentLightModelfSGIX },
  { "glFragmentLightModelfvSGIX", "GL_SGIX_fragment_lighting", glFragmentLightModelfvSGIX },
  { "glFragmentLightModeliSGIX", "GL_SGIX_fragment_lighting", glFragmentLightModeliSGIX },
  { "glFragmentLightModelivSGIX", "GL_SGIX_fragment_lighting", glFragmentLightModelivSGIX },
  { "glFragmentLightfSGIX", "GL_SGIX_fragment_lighting", glFragmentLightfSGIX },
  { "glFragmentLightfvSGIX", "GL_SGIX_fragment_lighting", glFragmentLightfvSGIX },
  { "glFragmentLightiSGIX", "GL_SGIX_fragment_lighting", glFragmentLightiSGIX },
  { "glFragmentLightivSGIX", "GL_SGIX_fragment_lighting", glFragmentLightivSGIX },
  { "glFragmentMaterialfSGIX", "GL_SGIX_fragment_lighting", glFragmentMaterialfSGIX },
  { "glFragmentMaterialfvSGIX", "GL_SGIX_fragment_lighting", glFragmentMaterialfvSGIX },
  { "glFragmentMaterialiSGIX", "GL_SGIX_fragment_lighting", glFragmentMaterialiSGIX },
  { "glFragmentMaterialivSGIX", "GL_SGIX_fragment_lighting", glFragmentMaterialivSGIX },
  { "glFrameTerminatorGREMEDY", "GL_GREMEDY_frame_terminator", glFrameTerminatorGREMEDY },
  { "glFrameZoomSGIX", "GL_SGIX_framezoom", glFrameZoomSGIX },
  { "glFramebufferDrawBufferEXT", "GL_EXT_direct_state_access", glFramebufferDrawBufferEXT },
  { "glFramebufferDrawBuffersEXT", "GL_EXT_direct_state_access", glFramebufferDrawBuffersEXT },
  { "glFramebufferParameteri", "GL_ARB_framebuffer_no_attachments GL_VERSION_4_3", glFramebufferParameteri },
  { "glFramebufferReadBufferEXT", "GL_EXT_direct_state_access", glFramebufferReadBufferEXT },
  { "glFramebufferRenderbuffer", "GL_ARB_framebuffer_object GL_VERSION_3_0", glFramebufferRenderbuffer },
  { "glFramebufferRenderbufferEXT", "GL_EXT_framebuffer_object", glFramebufferRenderbufferEXT },
  { "glFramebufferSampleLocationsfvARB", "GL_ARB_sample_locations", glFramebufferSampleLocationsfvARB },
  { "glFramebufferSampleLocationsfvNV", "GL_NV_sample_locations", glFramebufferSampleLocationsfvNV },
  { "glFramebufferSamplePositionsfvAMD", "GL_AMD_framebuffer_sample_positions", glFramebufferSamplePositionsfvAMD },
  { "glFramebufferTexture", "GL_VERSION_3_2", glFramebufferTexture },
  { "glFramebufferTexture1D", "GL_ARB_framebuffer_object GL_VERSION_3_0", glFramebufferTexture1D },
  { "glFramebufferTexture1DEXT", "GL_EXT_framebuffer_object", glFramebufferTexture1DEXT },
  { "glFramebufferTexture2D", "GL_ARB_framebuffer_object GL_VERSION_3_0", glFramebufferTexture2D },
  { "glFramebufferTexture2DEXT", "GL_EXT_framebuffer_object", glFramebufferTexture2DEXT },
  { "glFramebufferTexture3D", "GL_ARB_framebuffer_object GL_VERSION_3_0", glFramebufferTexture3D },
  { "glFramebufferTexture3DEXT", "GL_EXT_framebuffer_object", glFramebufferTexture3DEXT },
  { "glFramebufferTextureARB", "GL_ARB_geometry_shader4", glFramebufferTextureARB },
  { "glFramebufferTextureEXT", "GL_NV_geometry_program4", glFramebufferTextureEXT },
  { "glFramebufferTextureFaceARB", "GL_ARB_geometry_shader4", glFramebufferTextureFaceARB },
  { "glFramebufferTextureFaceEXT", "GL_NV_geometry_program4", glFramebufferTextureFaceEXT },
  { "glFramebufferTextureLayer", "GL_ARB_framebuffer_object GL_VERSION_3_0", glFramebufferTextureLayer },
  { "glFramebufferTextureLayerARB", "GL_ARB_geometry_shader4", glFramebufferTextureLayerARB },
  { "glFramebufferTextureLayerEXT", "GL_EXT_texture_array GL_NV_geometry_program4", glFramebufferTextureLayerEXT },
  { "glFramebufferTextureMultiviewOVR", "GL_OVR_multiview", glFramebufferTextureMultiviewOVR },
  { "glFreeObjectBufferATI", "GL_ATI_vertex_array_object", glFreeObjectBufferATI },
  { "glFrustumfOES", "GL_OES_single_precision", glFrustumfOES },
  { "glFrustumxOES", "GL_OES_fixed_point", glFrustumxOES },
  { "glGenAsyncMarkersSGIX", "GL_SGIX_async", glGenAsyncMarkersSGIX },
  { "glGenBuffers", "GL_VERSION_1_5", glGenBuffers },
  { "glGenBuffersARB", "GL_ARB_vertex_buffer_object", glGenBuffersARB },
  { "glGenFencesAPPLE", "GL_APPLE_fence", glGenFencesAPPLE },
  { "glGenFencesNV", "GL_NV_fence", glGenFencesNV },
  { "glGenFragmentShadersATI", "GL_ATI_fragment_shader", glGenFragmentShadersATI },
  { "glGenFramebuffers", "GL_ARB_framebuffer_object GL_VERSION_3_0", glGenFramebuffers },
  { "glGenFramebuffersEXT", "GL_EXT_framebuffer_object", glGenFramebuffersEXT },
  { "glGenNamesAMD", "GL_AMD_name_gen_delete", glGenNamesAMD },
  { "glGenOcclusionQueriesNV", "GL_NV_occlusion_query", glGenOcclusionQueriesNV },
  { "glGenPathsNV", "GL_NV_path_rendering", glGenPathsNV },
  { "glGenPerfMonitorsAMD", "GL_AMD_performance_monitor", glGenPerfMonitorsAMD },
  { "glGenProgramPipelines", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glGenProgramPipelines },
  { "glGenProgramsARB", "GL_ARB_fragment_program GL_ARB_vertex_program", glGenProgramsARB },
  { "glGenProgramsNV", "GL_NV_vertex_program", glGenProgramsNV },
  { "glGenQueries", "GL_VERSION_1_5", glGenQueries },
  { "glGenQueriesARB", "GL_ARB_occlusion_query", glGenQueriesARB },
  { "glGenQueryResourceTagNV", "GL_NV_query_resource_tag", glGenQueryResourceTagNV },
  { "glGenRenderbuffers", "GL_ARB_framebuffer_object GL_VERSION_3_0", glGenRenderbuffers },
  { "glGenRenderbuffersEXT", "GL_EXT_framebuffer_object", glGenRenderbuffersEXT },
  { "glGenSamplers", "GL_ARB_sampler_objects GL_VERSION_3_3", glGenSamplers },
  { "glGenSemaphoresEXT", "GL_EXT_semaphore", glGenSemaphoresEXT },
  { "glGenSymbolsEXT", "GL_EXT_vertex_shader", glGenSymbolsEXT },
  { "glGenTexturesEXT", "GL_EXT_texture_object", glGenTexturesEXT },
  { "glGenTransformFeedbacks", "GL_ARB_transform_feedback2 GL_VERSION_4_0", glGenTransformFeedbacks },
  { "glGenTransformFeedbacksNV", "GL_NV_transform_feedback2", glGenTransformFeedbacksNV },
  { "glGenVertexArrays", "GL_ARB_vertex_array_object GL_VERSION_3_0", glGenVertexArrays },
  { "glGenVertexArraysAPPLE", "GL_APPLE_vertex_array_object", glGenVertexArraysAPPLE },
  { "glGenVertexShadersEXT", "GL_EXT_vertex_shader", glGenVertexShadersEXT },
  { "glGenerateMipmap", "GL_ARB_framebuffer_object GL_VERSION_3_0", glGenerateMipmap },
  { "glGenerateMipmapEXT", "GL_EXT_framebuffer_object", glGenerateMipmapEXT },
  { "glGenerateMultiTexMipmapEXT", "GL_EXT_direct_state_access", glGenerateMultiTexMipmapEXT },
  { "glGenerateTextureMipmap", "GL_ARB_direct_state_access GL_VERSION_4_5", glGenerateTextureMipmap },
  { "glGenerateTextureMipmapEXT", "GL_EXT_direct_state_access", glGenerateTextureMipmapEXT },
  { "glGetActiveAtomicCounterBufferiv", "GL_ARB_shader_atomic_counters GL_VERSION_4_2", glGetActiveAtomicCounterBufferiv },
  { "glGetActiveAttrib", "GL_VERSION_2_0", glGetActiveAttrib },
  { "glGetActiveAttribARB", "GL_ARB_vertex_shader", glGetActiveAttribARB },
  { "glGetActiveSubroutineName", "GL_ARB_shader_subroutine GL_VERSION_4_0", glGetActiveSubroutineName },
  { "glGetActiveSubroutineUniformName", "GL_ARB_shader_subroutine GL_VERSION_4_0", glGetActiveSubroutineUniformName },
  { "glGetActiveSubroutineUniformiv", "GL_ARB_shader_subroutine GL_VERSION_4_0", glGetActiveSubroutineUniformiv },
  { "glGetActiveUniform", "GL_VERSION_2_0", glGetActiveUniform },
  { "glGetActiveUniformARB", "GL_ARB_shader_objects", glGetActiveUniformARB },
  { "glGetActiveUniformBlockName", "GL_ARB_uniform_buffer_object GL_VERSION_3_1", glGetActiveUniformBlockName },
  { "glGetActiveUniformBlockiv", "GL_ARB_uniform_buffer_object GL_VERSION_3_1", glGetActiveUniformBlockiv },
  { "glGetActiveUniformName", "GL_ARB_uniform_buffer_object GL_VERSION_3_1", glGetActiveUniformName },
  { "glGetActiveUniformsiv", "GL_ARB_uniform_buffer_object GL_VERSION_3_1", glGetActiveUniformsiv },
  { "glGetActiveVaryingNV", "GL_NV_transform_feedback", glGetActiveVaryingNV },
  { "glGetArrayObjectfvATI", "GL_ATI_vertex_array_object", glGetArrayObjectfvATI },
  { "glGetArrayObjectivATI", "GL_ATI_vertex_array_object", glGetArrayObjectivATI },
  { "glGetAttachedObjectsARB", "GL_ARB_shader_objects", glGetAttachedObjectsARB },
  { "glGetAttachedShaders", "GL_VERSION_2_0", glGetAttachedShaders },
  { "glGetAttribLocation", "GL_VERSION_2_0", glGetAttribLocation },
  { "glGetAttribLocationARB", "GL_ARB_vertex_shader", glGetAttribLocationARB },
  { "glGetBooleanIndexedvEXT", "GL_EXT_direct_state_access GL_EXT_draw_buffers2", glGetBooleanIndexedvEXT },
  { "glGetBooleani_v", "GL_VERSION_3_0", glGetBooleani_v },
  { "glGetBufferParameteri64v", "GL_VERSION_3_2", glGetBufferParameteri64v },
  { "glGetBufferParameteriv", "GL_VERSION_1_5", glGetBufferParameteriv },
  { "glGetBufferParameterivARB", "GL_ARB_vertex_buffer_object", glGetBufferParameterivARB },
  { "glGetBufferParameterui64vNV", "GL_NV_shader_buffer_load", glGetBufferParameterui64vNV },
  { "glGetBufferPointerv", "GL_VERSION_1_5", glGetBufferPointerv },
  { "glGetBufferPointervARB", "GL_ARB_vertex_buffer_object", glGetBufferPointervARB },
  { "glGetBufferSubData", "GL_VERSION_1_5", glGetBufferSubData },
  { "glGetBufferSubDataARB", "GL_ARB_vertex_buffer_object", glGetBufferSubDataARB },
  { "glGetClipPlanefOES", "GL_OES_single_precision", glGetClipPlanefOES },
  { "glGetClipPlanexOES", "GL_OES_fixed_point", glGetClipPlanexOES },
  { "glGetColorTable", "GL_ARB_imaging", glGetColorTable },
  { "glGetColorTableEXT", "GL_EXT_paletted_texture", glGetColorTableEXT },
  { "glGetColorTableParameterfv", "GL_ARB_imaging", glGetColorTableParameterfv },
  { "glGetColorTableParameterfvEXT", "GL_EXT_paletted_texture", glGetColorTableParameterfvEXT },
  { "glGetColorTableParameterfvSGI", "GL_SGI_color_table", glGetColorTableParameterfvSGI },
  { "glGetColorTableParameteriv", "GL_ARB_imaging", glGetColorTableParameteriv },
  { "glGetColorTableParameterivEXT", "GL_EXT_paletted_texture", glGetColorTableParameterivEXT },
  { "glGetColorTableParameterivSGI", "GL_SGI_color_table", glGetColorTableParameterivSGI },
  { "glGetColorTableSGI", "GL_SGI_color_table", glGetColorTableSGI },
  { "glGetCombinerInputParameterfvNV", "GL_NV_register_combiners", glGetCombinerInputParameterfvNV },
  { "glGetCombinerInputParameterivNV", "GL_NV_register_combiners", glGetCombinerInputParameterivNV },
  { "glGetCombinerOutputParameterfvNV", "GL_NV_register_combiners", glGetCombinerOutputParameterfvNV },
  { "glGetCombinerOutputParameterivNV", "GL_NV_register_combiners", glGetCombinerOutputParameterivNV },
  { "glGetCombinerStageParameterfvNV", "GL_NV_register_combiners2", glGetCombinerStageParameterfvNV },
  { "glGetCommandHeaderNV", "GL_NV_command_list", glGetCommandHeaderNV },
  { "glGetCompressedMultiTexImageEXT", "GL_EXT_direct_state_access", glGetCompressedMultiTexImageEXT },
  { "glGetCompressedTexImage", "GL_VERSION_1_3", glGetCompressedTexImage },
  { "glGetCompressedTexImageARB", "GL_ARB_texture_compression", glGetCompressedTexImageARB },
  { "glGetCompressedTextureImage", "GL_ARB_direct_state_access GL_VERSION_4_5", glGetCompressedTextureImage },
  { "glGetCompressedTextureImageEXT", "GL_EXT_direct_state_access", glGetCompressedTextureImageEXT },
  { "glGetCompressedTextureSubImage", "GL_ARB_get_texture_sub_image GL_VERSION_4_5", glGetCompressedTextureSubImage },
  { "glGetConvolutionFilter", "GL_ARB_imaging", glGetConvolutionFilter },
  { "glGetConvolutionFilterEXT", "GL_EXT_convolution", glGetConvolutionFilterEXT },
  { "glGetConvolutionParameterfv", "GL_ARB_imaging", glGetConvolutionParameterfv },
  { "glGetConvolutionParameterfvEXT", "GL_EXT_convolution", glGetConvolutionParameterfvEXT },
  { "glGetConvolutionParameteriv", "GL_ARB_imaging", glGetConvolutionParameteriv },
  { "glGetConvolutionParameterivEXT", "GL_EXT_convolution", glGetConvolutionParameterivEXT },
  { "glGetConvolutionParameterxvOES", "GL_OES_fixed_point", glGetConvolutionParameterxvOES },
  { "glGetCoverageModulationTableNV", "GL_NV_framebuffer_mixed_samples", glGetCoverageModulationTableNV },
  { "glGetDebugMessageLog", "GL_KHR_debug GL_VERSION_4_3", glGetDebugMessageLog },
  { "glGetDebugMessageLogAMD", "GL_AMD_debug_output", glGetDebugMessageLogAMD },
  { "glGetDebugMessageLogARB", "GL_ARB_debug_output", glGetDebugMessageLogARB },
  { "glGetDetailTexFuncSGIS", "GL_SGIS_detail_texture", glGetDetailTexFuncSGIS },
  { "glGetDoubleIndexedvEXT", "GL_EXT_direct_state_access", glGetDoubleIndexedvEXT },
  { "glGetDoublei_v", "GL_ARB_viewport_array GL_VERSION_4_1", glGetDoublei_v },
  { "glGetDoublei_vEXT", "GL_EXT_direct_state_access", glGetDoublei_vEXT },
  { "glGetFenceivNV", "GL_NV_fence", glGetFenceivNV },
  { "glGetFinalCombinerInputParameterfvNV", "GL_NV_register_combiners", glGetFinalCombinerInputParameterfvNV },
  { "glGetFinalCombinerInputParameterivNV", "GL_NV_register_combiners", glGetFinalCombinerInputParameterivNV },
  { "glGetFirstPerfQueryIdINTEL", "GL_INTEL_performance_query", glGetFirstPerfQueryIdINTEL },
  { "glGetFixedvOES", "GL_OES_fixed_point", glGetFixedvOES },
  { "glGetFloatIndexedvEXT", "GL_EXT_direct_state_access", glGetFloatIndexedvEXT },
  { "glGetFloati_v", "GL_ARB_viewport_array GL_VERSION_4_1", glGetFloati_v },
  { "glGetFloati_vEXT", "GL_EXT_direct_state_access", glGetFloati_vEXT },
  { "glGetFogFuncSGIS", "GL_SGIS_fog_function", glGetFogFuncSGIS },
  { "glGetFragDataIndex", "GL_ARB_blend_func_extended GL_VERSION_3_3", glGetFragDataIndex },
  { "glGetFragDataLocation", "GL_VERSION_3_0", glGetFragDataLocation },
  { "glGetFragDataLocationEXT", "GL_EXT_gpu_shader4", glGetFragDataLocationEXT },
  { "glGetFragmentLightfvSGIX", "GL_SGIX_fragment_lighting", glGetFragmentLightfvSGIX },
  { "glGetFragmentLightivSGIX", "GL_SGIX_fragment_lighting", glGetFragmentLightivSGIX },
  { "glGetFragmentMaterialfvSGIX", "GL_SGIX_fragment_lighting", glGetFragmentMaterialfvSGIX },
  { "glGetFragmentMaterialivSGIX", "GL_SGIX_fragment_lighting", glGetFragmentMaterialivSGIX },
  { "glGetFramebufferAttachmentParameteriv", "GL_ARB_framebuffer_object GL_VERSION_3_0", glGetFramebufferAttachmentParameteriv },
  { "glGetFramebufferAttachmentParameterivEXT", "GL_EXT_framebuffer_object", glGetFramebufferAttachmentParameterivEXT },
  { "glGetFramebufferParameterfvAMD", "GL_AMD_framebuffer_sample_positions", glGetFramebufferParameterfvAMD },
  { "glGetFramebufferParameteriv", "GL_ARB_framebuffer_no_attachments GL_VERSION_4_3", glGetFramebufferParameteriv },
  { "glGetFramebufferParameterivEXT", "GL_EXT_direct_state_access", glGetFramebufferParameterivEXT },
  { "glGetGraphicsResetStatus", "GL_KHR_robustness GL_VERSION_4_5", glGetGraphicsResetStatus },
  { "glGetGraphicsResetStatusARB", "GL_ARB_robustness", glGetGraphicsResetStatusARB },
  { "glGetHandleARB", "GL_ARB_shader_objects", glGetHandleARB },
  { "glGetHistogram", "GL_ARB_imaging", glGetHistogram },
  { "glGetHistogramEXT", "GL_EXT_histogram", glGetHistogramEXT },
  { "glGetHistogramParameterfv", "GL_ARB_imaging", glGetHistogramParameterfv },
  { "glGetHistogramParameterfvEXT", "GL_EXT_histogram", glGetHistogramParameterfvEXT },
  { "glGetHistogramParameteriv", "GL_ARB_imaging", glGetHistogramParameteriv },
  { "glGetHistogramParameterivEXT", "GL_EXT_histogram", glGetHistogramParameterivEXT },
  { "glGetHistogramParameterxvOES", "GL_OES_fixed_point", glGetHistogramParameterxvOES },
  { "glGetImageHandleARB", "GL_ARB_bindless_texture", glGetImageHandleARB },
  { "glGetImageHandleNV", "GL_NV_bindless_texture", glGetImageHandleNV },
  { "glGetImageTransformParameterfvHP", "GL_HP_image_transform", glGetImageTransformParameterfvHP },
  { "glGetImageTransformParameterivHP", "GL_HP_image_transform", glGetImageTransformParameterivHP },
  { "glGetInfoLogARB", "GL_ARB_shader_objects", glGetInfoLogARB },
  { "glGetInstrumentsSGIX", "GL_SGIX_instruments", glGetInstrumentsSGIX },
  { "glGetInteger64i_v", "GL_VERSION_3_2", glGetInteger64i_v },
  { "glGetInteger64v", "GL_ARB_sync GL_VERSION_3_2", glGetInteger64v },
  { "glGetIntegerIndexedvEXT", "GL_EXT_direct_state_access GL_EXT_draw_buffers2", glGetIntegerIndexedvEXT },
  { "glGetIntegeri_v", "GL_ARB_uniform_buffer_object GL_VERSION_3_0", glGetIntegeri_v },
  { "glGetIntegerui64i_vNV", "GL_NV_vertex_buffer_unified_memory", glGetIntegerui64i_vNV },
  { "glGetIntegerui64vNV", "GL_NV_shader_buffer_load", glGetIntegerui64vNV },
  { "glGetInternalformatSampleivNV", "GL_NV_internalformat_sample_query", glGetInternalformatSampleivNV },
  { "glGetInternalformati64v", "GL_ARB_internalformat_query2 GL_VERSION_4_3", glGetInternalformati64v },
  { "glGetInternalformativ", "GL_ARB_internalformat_query GL_VERSION_4_2", glGetInternalformativ },
  { "glGetInvariantBooleanvEXT", "GL_EXT_vertex_shader", glGetInvariantBooleanvEXT },
  { "glGetInvariantFloatvEXT", "GL_EXT_vertex_shader", glGetInvariantFloatvEXT },
  { "glGetInvariantIntegervEXT", "GL_EXT_vertex_shader", glGetInvariantIntegervEXT },
  { "glGetLightxOES", "GL_OES_fixed_point", glGetLightxOES },
  { "glGetListParameterfvSGIX", "GL_SGIX_list_priority", glGetListParameterfvSGIX },
  { "glGetListParameterivSGIX", "GL_SGIX_list_priority", glGetListParameterivSGIX },
  { "glGetLocalConstantBooleanvEXT", "GL_EXT_vertex_shader", glGetLocalConstantBooleanvEXT },
  { "glGetLocalConstantFloatvEXT", "GL_EXT_vertex_shader", glGetLocalConstantFloatvEXT },
  { "glGetLocalConstantIntegervEXT", "GL_EXT_vertex_shader", glGetLocalConstantIntegervEXT },
  { "glGetMapAttribParameterfvNV", "GL_NV_evaluators", glGetMapAttribParameterfvNV },
  { "glGetMapAttribParameterivNV", "GL_NV_evaluators", glGetMapAttribParameterivNV },
  { "glGetMapControlPointsNV", "GL_NV_evaluators", glGetMapControlPointsNV },
  { "glGetMapParameterfvNV", "GL_NV_evaluators", glGetMapParameterfvNV },
  { "glGetMapParameterivNV", "GL_NV_evaluators", glGetMapParameterivNV },
  { "glGetMapxvOES", "GL_OES_fixed_point", glGetMapxvOES },
  { "glGetMaterialxOES", "GL_OES_fixed_point", glGetMaterialxOES },
  { "glGetMemoryObjectParameterivEXT", "GL_EXT_memory_object", glGetMemoryObjectParameterivEXT },
  { "glGetMinmax", "GL_ARB_imaging", glGetMinmax },
  { "glGetMinmaxEXT", "GL_EXT_histogram", glGetMinmaxEXT },
  { "glGetMinmaxParameterfv", "GL_ARB_imaging", glGetMinmaxParameterfv },
  { "glGetMinmaxParameterfvEXT", "GL_EXT_histogram", glGetMinmaxParameterfvEXT },
  { "glGetMinmaxParameteriv", "GL_ARB_imaging", glGetMinmaxParameteriv },
  { "glGetMinmaxParameterivEXT", "GL_EXT_histogram", glGetMinmaxParameterivEXT },
  { "glGetMultiTexEnvfvEXT", "GL_EXT_direct_state_access", glGetMultiTexEnvfvEXT },
  { "glGetMultiTexEnvivEXT", "GL_EXT_direct_state_access", glGetMultiTexEnvivEXT },
  { "glGetMultiTexGendvEXT", "GL_EXT_direct_state_access", glGetMultiTexGendvEXT },
  { "glGetMultiTexGenfvEXT", "GL_EXT_direct_state_access", glGetMultiTexGenfvEXT },
  { "glGetMultiTexGenivEXT", "GL_EXT_direct_state_access", glGetMultiTexGenivEXT },
  { "glGetMultiTexImageEXT", "GL_EXT_direct_state_access", glGetMultiTexImageEXT },
  { "glGetMultiTexLevelParameterfvEXT", "GL_EXT_direct_state_access", glGetMultiTexLevelParameterfvEXT },
  { "glGetMultiTexLevelParameterivEXT", "GL_EXT_direct_state_access", glGetMultiTexLevelParameterivEXT },
  { "glGetMultiTexParameterIivEXT", "GL_EXT_direct_state_access", glGetMultiTexParameterIivEXT },
  { "glGetMultiTexParameterIuivEXT", "GL_EXT_direct_state_access", glGetMultiTexParameterIuivEXT },
  { "glGetMultiTexParameterfvEXT", "GL_EXT_direct_state_access", glGetMultiTexParameterfvEXT },
  { "glGetMultiTexParameterivEXT", "GL_EXT_direct_state_access", glGetMultiTexParameterivEXT },
  { "glGetMultisamplefv", "GL_ARB_texture_multisample GL_VERSION_3_2", glGetMultisamplefv },
  { "glGetMultisamplefvNV", "GL_NV_explicit_multisample", glGetMultisamplefvNV },
  { "glGetNamedBufferParameteri64v", "GL_ARB_direct_state_access GL_VERSION_4_5", glGetNamedBufferParameteri64v },
  { "glGetNamedBufferParameteriv", "GL_ARB_direct_state_access GL_VERSION_4_5", glGetNamedBufferParameteriv },
  { "glGetNamedBufferParameterivEXT", "GL_EXT_direct_state_access", glGetNamedBufferParameterivEXT },
  { "glGetNamedBufferParameterui64vNV", "GL_NV_shader_buffer_load", glGetNamedBufferParameterui64vNV },
  { "glGetNamedBufferPointerv", "GL_ARB_direct_state_access GL_VERSION_4_5", glGetNamedBufferPointerv },
  { "glGetNamedBufferPointervEXT", "GL_EXT_direct_state_access", glGetNamedBufferPointervEXT },
  { "glGetNamedBufferSubData", "GL_ARB_direct_state_access GL_VERSION_4_5", glGetNamedBufferSubData },
  { "glGetNamedBufferSubDataEXT", "GL_EXT_direct_state_access", glGetNamedBufferSubDataEXT },
  { "glGetNamedFramebufferAttachmentParameteriv", "GL_ARB_direct_state_access GL_VERSION_4_5", glGetNamedFramebufferAttachmentParameteriv },
  { "glGetNamedFramebufferAttachmentParameterivEXT", "GL_EXT_direct_state_access", glGetNamedFramebufferAttachmentParameterivEXT },
  { "glGetNamedFramebufferParameterfvAMD", "GL_AMD_framebuffer_sample_positions", glGetNamedFramebufferParameterfvAMD },
  { "glGetNamedFramebufferParameteriv", "GL_ARB_direct_state_access GL_VERSION_4_5", glGetNamedFramebufferParameteriv },
  { "glGetNamedFramebufferParameterivEXT", "GL_EXT_direct_state_access", glGetNamedFramebufferParameterivEXT },
  { "glGetNamedProgramLocalParameterIivEXT", "GL_EXT_direct_state_access", glGetNamedProgramLocalParameterIivEXT },
  { "glGetNamedProgramLocalParameterIuivEXT", "GL_EXT_direct_state_access", glGetNamedProgramLocalParameterIuivEXT },
  { "glGetNamedProgramLocalParameterdvEXT", "GL_EXT_direct_state_access", glGetNamedProgramLocalParameterdvEXT },
  { "glGetNamedProgramLocalParameterfvEXT", "GL_EXT_direct_state_access", glGetNamedProgramLocalParameterfvEXT },
  { "glGetNamedProgramStringEXT", "GL_EXT_direct_state_access", glGetNamedProgramStringEXT },
  { "glGetNamedProgramivEXT", "GL_EXT_direct_state_access", glGetNamedProgramivEXT },
  { "glGetNamedRenderbufferParameteriv", "GL_ARB_direct_state_access GL_VERSION_4_5", glGetNamedRenderbufferParameteriv },
  { "glGetNamedRenderbufferParameterivEXT", "GL_EXT_direct_state_access", glGetNamedRenderbufferParameterivEXT },
  { "glGetNamedStringARB", "GL_ARB_shading_language_include", glGetNamedStringARB },
  { "glGetNamedStringivARB", "GL_ARB_shading_language_include", glGetNamedStringivARB },
  { "glGetNextPerfQueryIdINTEL", "GL_INTEL_performance_query", glGetNextPerfQueryIdINTEL },
  { "glGetObjectBufferfvATI", "GL_ATI_vertex_array_object", glGetObjectBufferfvATI },
  { "glGetObjectBufferivATI", "GL_ATI_vertex_array_object", glGetObjectBufferivATI },
  { "glGetObjectLabel", "GL_KHR_debug GL_VERSION_4_3", glGetObjectLabel },
  { "glGetObjectLabelEXT", "GL_EXT_debug_label", glGetObjectLabelEXT },
  { "glGetObjectParameterfvARB", "GL_ARB_shader_objects", glGetObjectParameterfvARB },
  { "glGetObjectParameterivAPPLE", "GL_APPLE_object_purgeable", glGetObjectParameterivAPPLE },
  { "glGetObjectParameterivARB", "GL_ARB_shader_objects", glGetObjectParameterivARB },
  { "glGetObjectPtrLabel", "GL_KHR_debug GL_VERSION_4_3", glGetObjectPtrLabel },
  { "glGetOcclusionQueryivNV", "GL_NV_occlusion_query", glGetOcclusionQueryivNV },
  { "glGetOcclusionQueryuivNV", "GL_NV_occlusion_query", glGetOcclusionQueryuivNV },
  { "glGetPathColorGenfvNV", "GL_NV_path_rendering", glGetPathColorGenfvNV },
  { "glGetPathColorGenivNV", "GL_NV_path_rendering", glGetPathColorGenivNV },
  { "glGetPathCommandsNV", "GL_NV_path_rendering", glGetPathCommandsNV },
  { "glGetPathCoordsNV", "GL_NV_path_rendering", glGetPathCoordsNV },
  { "glGetPathDashArrayNV", "GL_NV_path_rendering", glGetPathDashArrayNV },
  { "glGetPathLengthNV", "GL_NV_path_rendering", glGetPathLengthNV },
  { "glGetPathMetricRangeNV", "GL_NV_path_rendering", glGetPathMetricRangeNV },
  { "glGetPathMetricsNV", "GL_NV_path_rendering", glGetPathMetricsNV },
  { "glGetPathParameterfvNV", "GL_NV_path_rendering", glGetPathParameterfvNV },
  { "glGetPathParameterivNV", "GL_NV_path_rendering", glGetPathParameterivNV },
  { "glGetPathSpacingNV", "GL_NV_path_rendering", glGetPathSpacingNV },
  { "glGetPathTexGenfvNV", "GL_NV_path_rendering", glGetPathTexGenfvNV },
  { "glGetPathTexGenivNV", "GL_NV_path_rendering", glGetPathTexGenivNV },
  { "glGetPerfCounterInfoINTEL", "GL_INTEL_performance_query", glGetPerfCounterInfoINTEL },
  { "glGetPerfMonitorCounterDataAMD", "GL_AMD_performance_monitor", glGetPerfMonitorCounterDataAMD },
  { "glGetPerfMonitorCounterInfoAMD", "GL_AMD_performance_monitor", glGetPerfMonitorCounterInfoAMD },
  { "glGetPerfMonitorCounterStringAMD", "GL_AMD_performance_monitor", glGetPerfMonitorCounterStringAMD },
  { "glGetPerfMonitorCountersAMD", "GL_AMD_performance_monitor", glGetPerfMonitorCountersAMD },
  { "glGetPerfMonitorGroupStringAMD", "GL_AMD_performance_monitor", glGetPerfMonitorGroupStringAMD },
  { "glGetPerfMonitorGroupsAMD", "GL_AMD_performance_monitor", glGetPerfMonitorGroupsAMD },
  { "glGetPerfQueryDataINTEL", "GL_INTEL_performance_query", glGetPerfQueryDataINTEL },
  { "glGetPerfQueryIdByNameINTEL", "GL_INTEL_performance_query", glGetPerfQueryIdByNameINTEL },
  { "glGetPerfQueryInfoINTEL", "GL_INTEL_performance_query", glGetPerfQueryInfoINTEL },
  { "glGetPixelMapxv", "GL_OES_fixed_point", glGetPixelMapxv },
  { "glGetPixelTexGenParameterfvSGIS", "GL_SGIS_pixel_texture", glGetPixelTexGenParameterfvSGIS },
  { "glGetPixelTexGenParameterivSGIS", "GL_SGIS_pixel_texture", glGetPixelTexGenParameterivSGIS },
  { "glGetPixelTransformParameterfvEXT", "GL_EXT_pixel_transform", glGetPixelTransformParameterfvEXT },
  { "glGetPixelTransformParameterivEXT", "GL_EXT_pixel_transform", glGetPixelTransformParameterivEXT },
  { "glGetPointerIndexedvEXT", "GL_EXT_direct_state_access", glGetPointerIndexedvEXT },
  { "glGetPointeri_vEXT", "GL_EXT_direct_state_access", glGetPointeri_vEXT },
  { "glGetPointervEXT", "GL_EXT_vertex_array", glGetPointervEXT },
  { "glGetProgramBinary", "GL_ARB_get_program_binary GL_VERSION_4_1", glGetProgramBinary },
  { "glGetProgramEnvParameterIivNV", "GL_NV_gpu_program4", glGetProgramEnvParameterIivNV },
  { "glGetProgramEnvParameterIuivNV", "GL_NV_gpu_program4", glGetProgramEnvParameterIuivNV },
  { "glGetProgramEnvParameterdvARB", "GL_ARB_fragment_program GL_ARB_vertex_program", glGetProgramEnvParameterdvARB },
  { "glGetProgramEnvParameterfvARB", "GL_ARB_fragment_program GL_ARB_vertex_program", glGetProgramEnvParameterfvARB },
  { "glGetProgramInfoLog", "GL_VERSION_2_0", glGetProgramInfoLog },
  { "glGetProgramInterfaceiv", "GL_ARB_program_interface_query GL_VERSION_4_3", glGetProgramInterfaceiv },
  { "glGetProgramLocalParameterIivNV", "GL_NV_gpu_program4", glGetProgramLocalParameterIivNV },
  { "glGetProgramLocalParameterIuivNV", "GL_NV_gpu_program4", glGetProgramLocalParameterIuivNV },
  { "glGetProgramLocalParameterdvARB", "GL_ARB_fragment_program GL_ARB_vertex_program", glGetProgramLocalParameterdvARB },
  { "glGetProgramLocalParameterfvARB", "GL_ARB_fragment_program GL_ARB_vertex_program", glGetProgramLocalParameterfvARB },
  { "glGetProgramNamedParameterdvNV", "GL_NV_fragment_program", glGetProgramNamedParameterdvNV },
  { "glGetProgramNamedParameterfvNV", "GL_NV_fragment_program", glGetProgramNamedParameterfvNV },
  { "glGetProgramParameterdvNV", "GL_NV_vertex_program", glGetProgramParameterdvNV },
  { "glGetProgramParameterfvNV", "GL_NV_vertex_program", glGetProgramParameterfvNV },
  { "glGetProgramPipelineInfoLog", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glGetProgramPipelineInfoLog },
  { "glGetProgramPipelineiv", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glGetProgramPipelineiv },
  { "glGetProgramResourceIndex", "GL_ARB_program_interface_query GL_VERSION_4_3", glGetProgramResourceIndex },
  { "glGetProgramResourceLocation", "GL_ARB_program_interface_query GL_VERSION_4_3", glGetProgramResourceLocation },
  { "glGetProgramResourceLocationIndex", "GL_ARB_program_interface_query GL_VERSION_4_3", glGetProgramResourceLocationIndex },
  { "glGetProgramResourceName", "GL_ARB_program_interface_query GL_VERSION_4_3", glGetProgramResourceName },
  { "glGetProgramResourcefvNV", "GL_NV_path_rendering", glGetProgramResourcefvNV },
  { "glGetProgramResourceiv", "GL_ARB_program_interface_query GL_VERSION_4_3", glGetProgramResourceiv },
  { "glGetProgramStageiv", "GL_ARB_shader_subroutine GL_VERSION_4_0", glGetProgramStageiv },
  { "glGetProgramStringARB", "GL_ARB_fragment_program GL_ARB_vertex_program", glGetProgramStringARB },
  { "glGetProgramStringNV", "GL_NV_vertex_program", glGetProgramStringNV },
  { "glGetProgramSubroutineParameteruivNV", "GL_NV_gpu_program5", glGetProgramSubroutineParameteruivNV },
  { "glGetProgramiv", "GL_VERSION_2_0", glGetProgramiv },
  { "glGetProgramivARB", "GL_ARB_fragment_program GL_ARB_vertex_program", glGetProgramivARB },
  { "glGetProgramivNV", "GL_NV_vertex_program", glGetProgramivNV },
  { "glGetQueryBufferObjecti64v", "GL_ARB_direct_state_access GL_VERSION_4_5", glGetQueryBufferObjecti64v },
  { "glGetQueryBufferObjectiv", "GL_ARB_direct_state_access GL_VERSION_4_5", glGetQueryBufferObjectiv },
  { "glGetQueryBufferObjectui64v", "GL_ARB_direct_state_access GL_VERSION_4_5", glGetQueryBufferObjectui64v },
  { "glGetQueryBufferObjectuiv", "GL_ARB_direct_state_access GL_VERSION_4_5", glGetQueryBufferObjectuiv },
  { "glGetQueryIndexediv", "GL_ARB_transform_feedback3 GL_VERSION_4_0", glGetQueryIndexediv },
  { "glGetQueryObjecti64v", "GL_ARB_timer_query GL_VERSION_3_3", glGetQueryObjecti64v },
  { "glGetQueryObjecti64vEXT", "GL_EXT_timer_query", glGetQueryObjecti64vEXT },
  { "glGetQueryObjectiv", "GL_VERSION_1_5", glGetQueryObjectiv },
  { "glGetQueryObjectivARB", "GL_ARB_occlusion_query", glGetQueryObjectivARB },
  { "glGetQueryObjectui64v", "GL_ARB_timer_query GL_VERSION_3_3", glGetQueryObjectui64v },
  { "glGetQueryObjectui64vEXT", "GL_EXT_timer_query", glGetQueryObjectui64vEXT },
  { "glGetQueryObjectuiv", "GL_VERSION_1_5", glGetQueryObjectuiv },
  { "glGetQueryObjectuivARB", "GL_ARB_occlusion_query", glGetQueryObjectuivARB },
  { "glGetQueryiv", "GL_VERSION_1_5", glGetQueryiv },
  { "glGetQueryivARB", "GL_ARB_occlusion_query", glGetQueryivARB },
  { "glGetRenderbufferParameteriv", "GL_ARB_framebuffer_object GL_VERSION_3_0", glGetRenderbufferParameteriv },
  { "glGetRenderbufferParameterivEXT", "GL_EXT_framebuffer_object", glGetRenderbufferParameterivEXT },
  { "glGetSamplerParameterIiv", "GL_ARB_sampler_objects GL_VERSION_3_3", glGetSamplerParameterIiv },
  { "glGetSamplerParameterIuiv", "GL_ARB_sampler_objects GL_VERSION_3_3", glGetSamplerParameterIuiv },
  { "glGetSamplerParameterfv", "GL_ARB_sampler_objects GL_VERSION_3_3", glGetSamplerParameterfv },
  { "glGetSamplerParameteriv", "GL_ARB_sampler_objects GL_VERSION_3_3", glGetSamplerParameteriv },
  { "glGetSemaphoreParameterui64vEXT", "GL_EXT_semaphore", glGetSemaphoreParameterui64vEXT },
  { "glGetSeparableFilter", "GL_ARB_imaging", glGetSeparableFilter },
  { "glGetSeparableFilterEXT", "GL_EXT_convolution", glGetSeparableFilterEXT },
  { "glGetShaderInfoLog", "GL_VERSION_2_0", glGetShaderInfoLog },
  { "glGetShaderPrecisionFormat", "GL_ARB_ES2_compatibility GL_VERSION_4_1", glGetShaderPrecisionFormat },
  { "glGetShaderSource", "GL_VERSION_2_0", glGetShaderSource },
  { "glGetShaderSourceARB", "GL_ARB_shader_objects", glGetShaderSourceARB },
  { "glGetShaderiv", "GL_VERSION_2_0", glGetShaderiv },
  { "glGetSharpenTexFuncSGIS", "GL_SGIS_sharpen_texture", glGetSharpenTexFuncSGIS },
  { "glGetStageIndexNV", "GL_NV_command_list", glGetStageIndexNV },
  { "glGetStringi", "GL_VERSION_3_0", glGetStringi },
  { "glGetSubroutineIndex", "GL_ARB_shader_subroutine GL_VERSION_4_0", glGetSubroutineIndex },
  { "glGetSubroutineUniformLocation", "GL_ARB_shader_subroutine GL_VERSION_4_0", glGetSubroutineUniformLocation },
  { "glGetSynciv", "GL_ARB_sync GL_VERSION_3_2", glGetSynciv },
  { "glGetTexBumpParameterfvATI", "GL_ATI_envmap_bumpmap", glGetTexBumpParameterfvATI },
  { "glGetTexBumpParameterivATI", "GL_ATI_envmap_bumpmap", glGetTexBumpParameterivATI },
  { "glGetTexEnvxvOES", "GL_OES_fixed_point", glGetTexEnvxvOES },
  { "glGetTexFilterFuncSGIS", "GL_SGIS_texture_filter4", glGetTexFilterFuncSGIS },
  { "glGetTexGenxvOES", "GL_OES_fixed_point", glGetTexGenxvOES },
  { "glGetTexLevelParameterxvOES", "GL_OES_fixed_point", glGetTexLevelParameterxvOES },
  { "glGetTexParameterIiv", "GL_VERSION_3_0", glGetTexParameterIiv },
  { "glGetTexParameterIivEXT", "GL_EXT_texture_integer", glGetTexParameterIivEXT },
  { "glGetTexParameterIuiv", "GL_VERSION_3_0", glGetTexParameterIuiv },
  { "glGetTexParameterIuivEXT", "GL_EXT_texture_integer", glGetTexParameterIuivEXT },
  { "glGetTexParameterPointervAPPLE", "GL_APPLE_texture_range", glGetTexParameterPointervAPPLE },
  { "glGetTexParameterxvOES", "GL_OES_fixed_point", glGetTexParameterxvOES },
  { "glGetTextureHandleARB", "GL_ARB_bindless_texture", glGetTextureHandleARB },
  { "glGetTextureHandleNV", "GL_NV_bindless_texture", glGetTextureHandleNV },
  { "glGetTextureImage", "GL_ARB_direct_state_access GL_VERSION_4_5", glGetTextureImage },
  { "glGetTextureImageEXT", "GL_EXT_direct_state_access", glGetTextureImageEXT },
  { "glGetTextureLevelParameterfv", "GL_ARB_direct_state_access GL_VERSION_4_5", glGetTextureLevelParameterfv },
  { "glGetTextureLevelParameterfvEXT", "GL_EXT_direct_state_access", glGetTextureLevelParameterfvEXT },
  { "glGetTextureLevelParameteriv", "GL_ARB_direct_state_access GL_VERSION_4_5", glGetTextureLevelParameteriv },
  { "glGetTextureLevelParameterivEXT", "GL_EXT_direct_state_access", glGetTextureLevelParameterivEXT },
  { "glGetTextureParameterIiv", "GL_ARB_direct_state_access GL_VERSION_4_5", glGetTextureParameterIiv },
  { "glGetTextureParameterIivEXT", "GL_EXT_direct_state_access", glGetTextureParameterIivEXT },
  { "glGetTextureParameterIuiv", "GL_ARB_direct_state_access GL_VERSION_4_5", glGetTextureParameterIuiv },
  { "glGetTextureParameterIuivEXT", "GL_EXT_direct_state_access", glGetTextureParameterIuivEXT },
  { "glGetTextureParameterfv", "GL_ARB_direct_state_access GL_VERSION_4_5", glGetTextureParameterfv },
  { "glGetTextureParameterfvEXT", "GL_EXT_direct_state_access", glGetTextureParameterfvEXT },
  { "glGetTextureParameteriv", "GL_ARB_direct_state_access GL_VERSION_4_5", glGetTextureParameteriv },
  { "glGetTextureParameterivEXT", "GL_EXT_direct_state_access", glGetTextureParameterivEXT },
  { "glGetTextureSamplerHandleARB", "GL_ARB_bindless_texture", glGetTextureSamplerHandleARB },
  { "glGetTextureSamplerHandleNV", "GL_NV_bindless_texture", glGetTextureSamplerHandleNV },
  { "glGetTextureSubImage", "GL_ARB_get_texture_sub_image GL_VERSION_4_5", glGetTextureSubImage },
  { "glGetTrackMatrixivNV", "GL_NV_vertex_program", glGetTrackMatrixivNV },
  { "glGetTransformFeedbackVarying", "GL_VERSION_3_0", glGetTransformFeedbackVarying },
  { "glGetTransformFeedbackVaryingEXT", "GL_EXT_transform_feedback", glGetTransformFeedbackVaryingEXT },
  { "glGetTransformFeedbackVaryingNV", "GL_NV_transform_feedback", glGetTransformFeedbackVaryingNV },
  { "glGetTransformFeedbacki64_v", "GL_ARB_direct_state_access GL_VERSION_4_5", glGetTransformFeedbacki64_v },
  { "glGetTransformFeedbacki_v", "GL_ARB_direct_state_access GL_VERSION_4_5", glGetTransformFeedbacki_v },
  { "glGetTransformFeedbackiv", "GL_ARB_direct_state_access GL_VERSION_4_5", glGetTransformFeedbackiv },
  { "glGetUniformBlockIndex", "GL_ARB_uniform_buffer_object GL_VERSION_3_1", glGetUniformBlockIndex },
  { "glGetUniformBufferSizeEXT", "GL_EXT_bindable_uniform", glGetUniformBufferSizeEXT },
  { "glGetUniformIndices", "GL_ARB_uniform_buffer_object GL_VERSION_3_1", glGetUniformIndices },
  { "glGetUniformLocation", "GL_VERSION_2_0", glGetUniformLocation },
  { "glGetUniformLocationARB", "GL_ARB_shader_objects", glGetUniformLocationARB },
  { "glGetUniformOffsetEXT", "GL_EXT_bindable_uniform", glGetUniformOffsetEXT },
  { "glGetUniformSubroutineuiv", "GL_ARB_shader_subroutine GL_VERSION_4_0", glGetUniformSubroutineuiv },
  { "glGetUniformdv", "GL_ARB_gpu_shader_fp64 GL_VERSION_4_0", glGetUniformdv },
  { "glGetUniformfv", "GL_VERSION_2_0", glGetUniformfv },
  { "glGetUniformfvARB", "GL_ARB_shader_objects", glGetUniformfvARB },
  { "glGetUniformi64vARB", "GL_ARB_gpu_shader_int64", glGetUniformi64vARB },
  { "glGetUniformi64vNV", "GL_AMD_gpu_shader_int64 GL_NV_gpu_shader5", glGetUniformi64vNV },
  { "glGetUniformiv", "GL_VERSION_2_0", glGetUniformiv },
  { "glGetUniformivARB", "GL_ARB_shader_objects", glGetUniformivARB },
  { "glGetUniformui64vARB", "GL_ARB_gpu_shader_int64", glGetUniformui64vARB },
  { "glGetUniformui64vNV", "GL_AMD_gpu_shader_int64 GL_NV_shader_buffer_load", glGetUniformui64vNV },
  { "glGetUniformuiv", "GL_VERSION_3_0", glGetUniformuiv },
  { "glGetUniformuivEXT", "GL_EXT_gpu_shader4", glGetUniformuivEXT },
  { "glGetUnsignedBytei_vEXT", "GL_EXT_memory_object GL_EXT_semaphore", glGetUnsignedBytei_vEXT },
  { "glGetUnsignedBytevEXT", "GL_EXT_memory_object GL_EXT_semaphore", glGetUnsignedBytevEXT },
  { "glGetVariantArrayObjectfvATI", "GL_ATI_vertex_array_object", glGetVariantArrayObjectfvATI },
  { "glGetVariantArrayObjectivATI", "GL_ATI_vertex_array_object", glGetVariantArrayObjectivATI },
  { "glGetVariantBooleanvEXT", "GL_EXT_vertex_shader", glGetVariantBooleanvEXT },
  { "glGetVariantFloatvEXT", "GL_EXT_vertex_shader", glGetVariantFloatvEXT },
  { "glGetVariantIntegervEXT", "GL_EXT_vertex_shader", glGetVariantIntegervEXT },
  { "glGetVariantPointervEXT", "GL_EXT_vertex_shader", glGetVariantPointervEXT },
  { "glGetVaryingLocationNV", "GL_NV_transform_feedback", glGetVaryingLocationNV },
  { "glGetVertexArrayIndexed64iv", "GL_ARB_direct_state_access GL_VERSION_4_5", glGetVertexArrayIndexed64iv },
  { "glGetVertexArrayIndexediv", "GL_ARB_direct_state_access GL_VERSION_4_5", glGetVertexArrayIndexediv },
  { "glGetVertexArrayIntegeri_vEXT", "GL_EXT_direct_state_access", glGetVertexArrayIntegeri_vEXT },
  { "glGetVertexArrayIntegervEXT", "GL_EXT_direct_state_access", glGetVertexArrayIntegervEXT },
  { "glGetVertexArrayPointeri_vEXT", "GL_EXT_direct_state_access", glGetVertexArrayPointeri_vEXT },
  { "glGetVertexArrayPointervEXT", "GL_EXT_direct_state_access", glGetVertexArrayPointervEXT },
  { "glGetVertexArrayiv", "GL_ARB_direct_state_access GL_VERSION_4_5", glGetVertexArrayiv },
  { "glGetVertexAttribArrayObjectfvATI", "GL_ATI_vertex_attrib_array_object", glGetVertexAttribArrayObjectfvATI },
  { "glGetVertexAttribArrayObjectivATI", "GL_ATI_vertex_attrib_array_object", glGetVertexAttribArrayObjectivATI },
  { "glGetVertexAttribIiv", "GL_VERSION_3_0", glGetVertexAttribIiv },
  { "glGetVertexAttribIivEXT", "GL_NV_vertex_program4", glGetVertexAttribIivEXT },
  { "glGetVertexAttribIuiv", "GL_VERSION_3_0", glGetVertexAttribIuiv },
  { "glGetVertexAttribIuivEXT", "GL_NV_vertex_program4", glGetVertexAttribIuivEXT },
  { "glGetVertexAttribLdv", "GL_ARB_vertex_attrib_64bit GL_VERSION_4_1", glGetVertexAttribLdv },
  { "glGetVertexAttribLdvEXT", "GL_EXT_vertex_attrib_64bit", glGetVertexAttribLdvEXT },
  { "glGetVertexAttribLi64vNV", "GL_NV_vertex_attrib_integer_64bit", glGetVertexAttribLi64vNV },
  { "glGetVertexAttribLui64vARB", "GL_ARB_bindless_texture", glGetVertexAttribLui64vARB },
  { "glGetVertexAttribLui64vNV", "GL_NV_vertex_attrib_integer_64bit", glGetVertexAttribLui64vNV },
  { "glGetVertexAttribPointerv", "GL_VERSION_2_0", glGetVertexAttribPointerv },
  { "glGetVertexAttribPointervARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glGetVertexAttribPointervARB },
  { "glGetVertexAttribPointervNV", "GL_NV_vertex_program", glGetVertexAttribPointervNV },
  { "glGetVertexAttribdv", "GL_VERSION_2_0", glGetVertexAttribdv },
  { "glGetVertexAttribdvARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glGetVertexAttribdvARB },
  { "glGetVertexAttribdvNV", "GL_NV_vertex_program", glGetVertexAttribdvNV },
  { "glGetVertexAttribfv", "GL_VERSION_2_0", glGetVertexAttribfv },
  { "glGetVertexAttribfvARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glGetVertexAttribfvARB },
  { "glGetVertexAttribfvNV", "GL_NV_vertex_program", glGetVertexAttribfvNV },
  { "glGetVertexAttribiv", "GL_VERSION_2_0", glGetVertexAttribiv },
  { "glGetVertexAttribivARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glGetVertexAttribivARB },
  { "glGetVertexAttribivNV", "GL_NV_vertex_program", glGetVertexAttribivNV },
  { "glGetVideoCaptureStreamdvNV", "GL_NV_video_capture", glGetVideoCaptureStreamdvNV },
  { "glGetVideoCaptureStreamfvNV", "GL_NV_video_capture", glGetVideoCaptureStreamfvNV },
  { "glGetVideoCaptureStreamivNV", "GL_NV_video_capture", glGetVideoCaptureStreamivNV },
  { "glGetVideoCaptureivNV", "GL_NV_video_capture", glGetVideoCaptureivNV },
  { "glGetVideoi64vNV", "GL_NV_present_video", glGetVideoi64vNV },
  { "glGetVideoivNV", "GL_NV_present_video", glGetVideoivNV },
  { "glGetVideoui64vNV", "GL_NV_present_video", glGetVideoui64vNV },
  { "glGetVideouivNV", "GL_NV_present_video", glGetVideouivNV },
  { "glGetVkProcAddrNV", "GL_NV_draw_vulkan_image", glGetVkProcAddrNV },
  { "glGetnColorTable", "GL_VERSION_4_5", glGetnColorTable },
  { "glGetnColorTableARB", "GL_ARB_robustness", glGetnColorTableARB },
  { "glGetnCompressedTexImage", "GL_VERSION_4_5", glGetnCompressedTexImage },
  { "glGetnCompressedTexImageARB", "GL_ARB_robustness", glGetnCompressedTexImageARB },
  { "glGetnConvolutionFilter", "GL_VERSION_4_5", glGetnConvolutionFilter },
  { "glGetnConvolutionFilterARB", "GL_ARB_robustness", glGetnConvolutionFilterARB },
  { "glGetnHistogram", "GL_VERSION_4_5", glGetnHistogram },
  { "glGetnHistogramARB", "GL_ARB_robustness", glGetnHistogramARB },
  { "glGetnMapdv", "GL_VERSION_4_5", glGetnMapdv },
  { "glGetnMapdvARB", "GL_ARB_robustness", glGetnMapdvARB },
  { "glGetnMapfv", "GL_VERSION_4_5", glGetnMapfv },
  { "glGetnMapfvARB", "GL_ARB_robustness", glGetnMapfvARB },
  { "glGetnMapiv", "GL_VERSION_4_5", glGetnMapiv },
  { "glGetnMapivARB", "GL_ARB_robustness", glGetnMapivARB },
  { "glGetnMinmax", "GL_VERSION_4_5", glGetnMinmax },
  { "glGetnMinmaxARB", "GL_ARB_robustness", glGetnMinmaxARB },
  { "glGetnPixelMapfv", "GL_VERSION_4_5", glGetnPixelMapfv },
  { "glGetnPixelMapfvARB", "GL_ARB_robustness", glGetnPixelMapfvARB },
  { "glGetnPixelMapuiv", "GL_VERSION_4_5", glGetnPixelMapuiv },
  { "glGetnPixelMapuivARB", "GL_ARB_robustness", glGetnPixelMapuivARB },
  { "glGetnPixelMapusv", "GL_VERSION_4_5", glGetnPixelMapusv },
  { "glGetnPixelMapusvARB", "GL_ARB_robustness", glGetnPixelMapusvARB },
  { "glGetnPolygonStipple", "GL_VERSION_4_5", glGetnPolygonStipple },
  { "glGetnPolygonStippleARB", "GL_ARB_robustness", glGetnPolygonStippleARB },
  { "glGetnSeparableFilter", "GL_VERSION_4_5", glGetnSeparableFilter },
  { "glGetnSeparableFilterARB", "GL_ARB_robustness", glGetnSeparableFilterARB },
  { "glGetnTexImage", "GL_VERSION_4_5", glGetnTexImage },
  { "glGetnTexImageARB", "GL_ARB_robustness", glGetnTexImageARB },
  { "glGetnUniformdv", "GL_VERSION_4_5", glGetnUniformdv },
  { "glGetnUniformdvARB", "GL_ARB_robustness", glGetnUniformdvARB },
  { "glGetnUniformfv", "GL_KHR_robustness GL_VERSION_4_5", glGetnUniformfv },
  { "glGetnUniformfvARB", "GL_ARB_robustness", glGetnUniformfvARB },
  { "glGetnUniformi64vARB", "GL_ARB_gpu_shader_int64", glGetnUniformi64vARB },
  { "glGetnUniformiv", "GL_KHR_robustness GL_VERSION_4_5", glGetnUniformiv },
  { "glGetnUniformivARB", "GL_ARB_robustness", glGetnUniformivARB },
  { "glGetnUniformui64vARB", "GL_ARB_gpu_shader_int64", glGetnUniformui64vARB },
  { "glGetnUniformuiv", "GL_KHR_robustness GL_VERSION_4_5", glGetnUniformuiv },
  { "glGetnUniformuivARB", "GL_ARB_robustness", glGetnUniformuivARB },
  { "glGlobalAlphaFactorbSUN", "GL_SUN_global_alpha", glGlobalAlphaFactorbSUN },
  { "glGlobalAlphaFactordSUN", "GL_SUN_global_alpha", glGlobalAlphaFactordSUN },
  { "glGlobalAlphaFactorfSUN", "GL_SUN_global_alpha", glGlobalAlphaFactorfSUN },
  { "glGlobalAlphaFactoriSUN", "GL_SUN_global_alpha", glGlobalAlphaFactoriSUN },
  { "glGlobalAlphaFactorsSUN", "GL_SUN_global_alpha", glGlobalAlphaFactorsSUN },
  { "glGlobalAlphaFactorubSUN", "GL_SUN_global_alpha", glGlobalAlphaFactorubSUN },
  { "glGlobalAlphaFactoruiSUN", "GL_SUN_global_alpha", glGlobalAlphaFactoruiSUN },
  { "glGlobalAlphaFactorusSUN", "GL_SUN_global_alpha", glGlobalAlphaFactorusSUN },
  { "glHintPGI", "GL_PGI_misc_hints", glHintPGI },
  { "glHistogram", "GL_ARB_imaging", glHistogram },
  { "glHistogramEXT", "GL_EXT_histogram", glHistogramEXT },
  { "glIglooInterfaceSGIX", "GL_SGIX_igloo_interface", glIglooInterfaceSGIX },
  { "glImageTransformParameterfHP", "GL_HP_image_transform", glImageTransformParameterfHP },
  { "glImageTransformParameterfvHP", "GL_HP_image_transform", glImageTransformParameterfvHP },
  { "glImageTransformParameteriHP", "GL_HP_image_transform", glImageTransformParameteriHP },
  { "glImageTransformParameterivHP", "GL_HP_image_transform", glImageTransformParameterivHP },
  { "glImportMemoryFdEXT", "GL_EXT_memory_object_fd", glImportMemoryFdEXT },
  { "glImportMemoryWin32HandleEXT", "GL_EXT_memory_object_win32", glImportMemoryWin32HandleEXT },
  { "glImportMemoryWin32NameEXT", "GL_EXT_memory_object_win32", glImportMemoryWin32NameEXT },
  { "glImportSemaphoreFdEXT", "GL_EXT_semaphore_fd", glImportSemaphoreFdEXT },
  { "glImportSemaphoreWin32HandleEXT", "GL_EXT_semaphore_win32", glImportSemaphoreWin32HandleEXT },
  { "glImportSemaphoreWin32NameEXT", "GL_EXT_semaphore_win32", glImportSemaphoreWin32NameEXT },
  { "glImportSyncEXT", "GL_EXT_x11_sync_object", glImportSyncEXT },
  { "glIndexFormatNV", "GL_NV_vertex_buffer_unified_memory", glIndexFormatNV },
  { "glIndexFuncEXT", "GL_EXT_index_func", glIndexFuncEXT },
  { "glIndexMaterialEXT", "GL_EXT_index_material", glIndexMaterialEXT },
  { "glIndexPointerEXT", "GL_EXT_vertex_array", glIndexPointerEXT },
  { "glIndexPointerListIBM", "GL_IBM_vertex_array_lists", glIndexPointerListIBM },
  { "glIndexxOES", "GL_OES_fixed_point", glIndexxOES },
  { "glIndexxvOES", "GL_OES_fixed_point", glIndexxvOES },
  { "glInsertComponentEXT", "GL_EXT_vertex_shader", glInsertComponentEXT },
  { "glInsertEventMarkerEXT", "GL_EXT_debug_marker", glInsertEventMarkerEXT },
  { "glInstrumentsBufferSGIX", "GL_SGIX_instruments", glInstrumentsBufferSGIX },
  { "glInterpolatePathsNV", "GL_NV_path_rendering", glInterpolatePathsNV },
  { "glInvalidateBufferData", "GL_ARB_invalidate_subdata GL_VERSION_4_3", glInvalidateBufferData },
  { "glInvalidateBufferSubData", "GL_ARB_invalidate_subdata GL_VERSION_4_3", glInvalidateBufferSubData },
  { "glInvalidateFramebuffer", "GL_ARB_invalidate_subdata GL_VERSION_4_3", glInvalidateFramebuffer },
  { "glInvalidateNamedFramebufferData", "GL_ARB_direct_state_access GL_VERSION_4_5", glInvalidateNamedFramebufferData },
  { "glInvalidateNamedFramebufferSubData", "GL_ARB_direct_state_access GL_VERSION_4_5", glInvalidateNamedFramebufferSubData },
  { "glInvalidateSubFramebuffer", "GL_ARB_invalidate_subdata GL_VERSION_4_3", glInvalidateSubFramebuffer },
  { "glInvalidateTexImage", "GL_ARB_invalidate_subdata GL_VERSION_4_3", glInvalidateTexImage },
  { "glInvalidateTexSubImage", "GL_ARB_invalidate_subdata GL_VERSION_4_3", glInvalidateTexSubImage },
  { "glIsAsyncMarkerSGIX", "GL_SGIX_async", glIsAsyncMarkerSGIX },
  { "glIsBuffer", "GL_VERSION_1_5", glIsBuffer },
  { "glIsBufferARB", "GL_ARB_vertex_buffer_object", glIsBufferARB },
  { "glIsBufferResidentNV", "GL_NV_shader_buffer_load", glIsBufferResidentNV },
  { "glIsCommandListNV", "GL_NV_command_list", glIsCommandListNV },
  { "glIsEnabledIndexedEXT", "GL_EXT_direct_state_access GL_EXT_draw_buffers2", glIsEnabledIndexedEXT },
  { "glIsEnabledi", "GL_VERSION_3_0", glIsEnabledi },
  { "glIsFenceAPPLE", "GL_APPLE_fence", glIsFenceAPPLE },
  { "glIsFenceNV", "GL_NV_fence", glIsFenceNV },
  { "glIsFramebuffer", "GL_ARB_framebuffer_object GL_VERSION_3_0", glIsFramebuffer },
  { "glIsFramebufferEXT", "GL_EXT_framebuffer_object", glIsFramebufferEXT },
  { "glIsImageHandleResidentARB", "GL_ARB_bindless_texture", glIsImageHandleResidentARB },
  { "glIsImageHandleResidentNV", "GL_NV_bindless_texture", glIsImageHandleResidentNV },
  { "glIsMemoryObjectEXT", "GL_EXT_memory_object", glIsMemoryObjectEXT },
  { "glIsNameAMD", "GL_AMD_name_gen_delete", glIsNameAMD },
  { "glIsNamedBufferResidentNV", "GL_NV_shader_buffer_load", glIsNamedBufferResidentNV },
  { "glIsNamedStringARB", "GL_ARB_shading_language_include", glIsNamedStringARB },
  { "glIsObjectBufferATI", "GL_ATI_vertex_array_object", glIsObjectBufferATI },
  { "glIsOcclusionQueryNV", "GL_NV_occlusion_query", glIsOcclusionQueryNV },
  { "glIsPathNV", "GL_NV_path_rendering", glIsPathNV },
  { "glIsPointInFillPathNV", "GL_NV_path_rendering", glIsPointInFillPathNV },
  { "glIsPointInStrokePathNV", "GL_NV_path_rendering", glIsPointInStrokePathNV },
  { "glIsProgram", "GL_VERSION_2_0", glIsProgram },
  { "glIsProgramARB", "GL_ARB_fragment_program GL_ARB_vertex_program", glIsProgramARB },
  { "glIsProgramNV", "GL_NV_vertex_program", glIsProgramNV },
  { "glIsProgramPipeline", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glIsProgramPipeline },
  { "glIsQuery", "GL_VERSION_1_5", glIsQuery },
  { "glIsQueryARB", "GL_ARB_occlusion_query", glIsQueryARB },
  { "glIsRenderbuffer", "GL_ARB_framebuffer_object GL_VERSION_3_0", glIsRenderbuffer },
  { "glIsRenderbufferEXT", "GL_EXT_framebuffer_object", glIsRenderbufferEXT },
  { "glIsSampler", "GL_ARB_sampler_objects GL_VERSION_3_3", glIsSampler },
  { "glIsSemaphoreEXT", "GL_EXT_semaphore", glIsSemaphoreEXT },
  { "glIsShader", "GL_VERSION_2_0", glIsShader },
  { "glIsStateNV", "GL_NV_command_list", glIsStateNV },
  { "glIsSync", "GL_ARB_sync GL_VERSION_3_2", glIsSync },
  { "glIsTextureEXT", "GL_EXT_texture_object", glIsTextureEXT },
  { "glIsTextureHandleResidentARB", "GL_ARB_bindless_texture", glIsTextureHandleResidentARB },
  { "glIsTextureHandleResidentNV", "GL_NV_bindless_texture", glIsTextureHandleResidentNV },
  { "glIsTransformFeedback", "GL_ARB_transform_feedback2 GL_VERSION_4_0", glIsTransformFeedback },
  { "glIsTransformFeedbackNV", "GL_NV_transform_feedback2", glIsTransformFeedbackNV },
  { "glIsVariantEnabledEXT", "GL_EXT_vertex_shader", glIsVariantEnabledEXT },
  { "glIsVertexArray", "GL_ARB_vertex_array_object GL_VERSION_3_0", glIsVertexArray },
  { "glIsVertexArrayAPPLE", "GL_APPLE_vertex_array_object", glIsVertexArrayAPPLE },
  { "glIsVertexAttribEnabledAPPLE", "GL_APPLE_vertex_program_evaluators", glIsVertexAttribEnabledAPPLE },
  { "glLGPUCopyImageSubDataNVX", "GL_NVX_linked_gpu_multicast", glLGPUCopyImageSubDataNVX },
  { "glLGPUInterlockNVX", "GL_NVX_linked_gpu_multicast", glLGPUInterlockNVX },
  { "glLGPUNamedBufferSubDataNVX", "GL_NVX_linked_gpu_multicast", glLGPUNamedBufferSubDataNVX },
  { "glLabelObjectEXT", "GL_EXT_debug_label", glLabelObjectEXT },
  { "glLightEnviSGIX", "GL_SGIX_fragment_lighting", glLightEnviSGIX },
  { "glLightModelxOES", "GL_OES_fixed_point", glLightModelxOES },
  { "glLightModelxvOES", "GL_OES_fixed_point", glLightModelxvOES },
  { "glLightxOES", "GL_OES_fixed_point", glLightxOES },
  { "glLightxvOES", "GL_OES_fixed_point", glLightxvOES },
  { "glLineWidthxOES", "GL_OES_fixed_point", glLineWidthxOES },
  { "glLinkProgram", "GL_VERSION_2_0", glLinkProgram },
  { "glLinkProgramARB", "GL_ARB_shader_objects", glLinkProgramARB },
  { "glListDrawCommandsStatesClientNV", "GL_NV_command_list", glListDrawCommandsStatesClientNV },
  { "glListParameterfSGIX", "GL_SGIX_list_priority", glListParameterfSGIX },
  { "glListParameterfvSGIX", "GL_SGIX_list_priority", glListParameterfvSGIX },
  { "glListParameteriSGIX", "GL_SGIX_list_priority", glListParameteriSGIX },
  { "glListParameterivSGIX", "GL_SGIX_list_priority", glListParameterivSGIX },
  { "glLoadIdentityDeformationMapSGIX", "GL_SGIX_polynomial_ffd", glLoadIdentityDeformationMapSGIX },
  { "glLoadMatrixxOES", "GL_OES_fixed_point", glLoadMatrixxOES },
  { "glLoadProgramNV", "GL_NV_vertex_program", glLoadProgramNV },
  { "glLoadTransposeMatrixd", "GL_VERSION_1_3", glLoadTransposeMatrixd },
  { "glLoadTransposeMatrixdARB", "GL_ARB_transpose_matrix", glLoadTransposeMatrixdARB },
  { "glLoadTransposeMatrixf", "GL_VERSION_1_3", glLoadTransposeMatrixf },
  { "glLoadTransposeMatrixfARB", "GL_ARB_transpose_matrix", glLoadTransposeMatrixfARB },
  { "glLoadTransposeMatrixxOES", "GL_OES_fixed_point", glLoadTransposeMatrixxOES },
  { "glLockArraysEXT", "GL_EXT_compiled_vertex_array", glLockArraysEXT },
  { "glMTexCoord2fSGIS", "GL_SGIS_multitexture", glMTexCoord2fSGIS },
  { "glMTexCoord2fvSGIS", "GL_SGIS_multitexture", glMTexCoord2fvSGIS },
  { "glMakeBufferNonResidentNV", "GL_NV_shader_buffer_load", glMakeBufferNonResidentNV },
  { "glMakeBufferResidentNV", "GL_NV_shader_buffer_load", glMakeBufferResidentNV },
  { "glMakeImageHandleNonResidentARB", "GL_ARB_bindless_texture", glMakeImageHandleNonResidentARB },
  { "glMakeImageHandleNonResidentNV", "GL_NV_bindless_texture", glMakeImageHandleNonResidentNV },
  { "glMakeImageHandleResidentARB", "GL_ARB_bindless_texture", glMakeImageHandleResidentARB },
  { "glMakeImageHandleResidentNV", "GL_NV_bindless_texture", glMakeImageHandleResidentNV },
  { "glMakeNamedBufferNonResidentNV", "GL_NV_shader_buffer_load", glMakeNamedBufferNonResidentNV },
  { "glMakeNamedBufferResidentNV", "GL_NV_shader_buffer_load", glMakeNamedBufferResidentNV },
  { "glMakeTextureHandleNonResidentARB", "GL_ARB_bindless_texture", glMakeTextureHandleNonResidentARB },
  { "glMakeTextureHandleNonResidentNV", "GL_NV_bindless_texture", glMakeTextureHandleNonResidentNV },
  { "glMakeTextureHandleResidentARB", "GL_ARB_bindless_texture", glMakeTextureHandleResidentARB },
  { "glMakeTextureHandleResidentNV", "GL_NV_bindless_texture", glMakeTextureHandleResidentNV },
  { "glMap1xOES", "GL_OES_fixed_point", glMap1xOES },
  { "glMap2xOES", "GL_OES_fixed_point", glMap2xOES },
  { "glMapBuffer", "GL_VERSION_1_5", glMapBuffer },
  { "glMapBufferARB", "GL_ARB_vertex_buffer_object", glMapBufferARB },
  { "glMapBufferRange", "GL_ARB_map_buffer_range GL_VERSION_3_0", glMapBufferRange },
  { "glMapControlPointsNV", "GL_NV_evaluators", glMapControlPointsNV },
  { "glMapGrid1xOES", "GL_OES_fixed_point", glMapGrid1xOES },
  { "glMapGrid2xOES", "GL_OES_fixed_point", glMapGrid2xOES },
  { "glMapNamedBuffer", "GL_ARB_direct_state_access GL_VERSION_4_5", glMapNamedBuffer },
  { "glMapNamedBufferEXT", "GL_EXT_direct_state_access", glMapNamedBufferEXT },
  { "glMapNamedBufferRange", "GL_ARB_direct_state_access GL_VERSION_4_5", glMapNamedBufferRange },
  { "glMapNamedBufferRangeEXT", "GL_EXT_direct_state_access", glMapNamedBufferRangeEXT },
  { "glMapObjectBufferATI", "GL_ATI_map_object_buffer", glMapObjectBufferATI },
  { "glMapParameterfvNV", "GL_NV_evaluators", glMapParameterfvNV },
  { "glMapParameterivNV", "GL_NV_evaluators", glMapParameterivNV },
  { "glMapTexture2DINTEL", "GL_INTEL_map_texture", glMapTexture2DINTEL },
  { "glMapVertexAttrib1dAPPLE", "GL_APPLE_vertex_program_evaluators", glMapVertexAttrib1dAPPLE },
  { "glMapVertexAttrib1fAPPLE", "GL_APPLE_vertex_program_evaluators", glMapVertexAttrib1fAPPLE },
  { "glMapVertexAttrib2dAPPLE", "GL_APPLE_vertex_program_evaluators", glMapVertexAttrib2dAPPLE },
  { "glMapVertexAttrib2fAPPLE", "GL_APPLE_vertex_program_evaluators", glMapVertexAttrib2fAPPLE },
  { "glMaterialxOES", "GL_OES_fixed_point", glMaterialxOES },
  { "glMaterialxvOES", "GL_OES_fixed_point", glMaterialxvOES },
  { "glMatrixFrustumEXT", "GL_EXT_direct_state_access", glMatrixFrustumEXT },
  { "glMatrixIndexPointerARB", "GL_ARB_matrix_palette", glMatrixIndexPointerARB },
  { "glMatrixIndexubvARB", "GL_ARB_matrix_palette", glMatrixIndexubvARB },
  { "glMatrixIndexuivARB", "GL_ARB_matrix_palette", glMatrixIndexuivARB },
  { "glMatrixIndexusvARB", "GL_ARB_matrix_palette", glMatrixIndexusvARB },
  { "glMatrixLoad3x2fNV", "GL_NV_path_rendering", glMatrixLoad3x2fNV },
  { "glMatrixLoad3x3fNV", "GL_NV_path_rendering", glMatrixLoad3x3fNV },
  { "glMatrixLoadIdentityEXT", "GL_EXT_direct_state_access", glMatrixLoadIdentityEXT },
  { "glMatrixLoadTranspose3x3fNV", "GL_NV_path_rendering", glMatrixLoadTranspose3x3fNV },
  { "glMatrixLoadTransposedEXT", "GL_EXT_direct_state_access", glMatrixLoadTransposedEXT },
  { "glMatrixLoadTransposefEXT", "GL_EXT_direct_state_access", glMatrixLoadTransposefEXT },
  { "glMatrixLoaddEXT", "GL_EXT_direct_state_access", glMatrixLoaddEXT },
  { "glMatrixLoadfEXT", "GL_EXT_direct_state_access", glMatrixLoadfEXT },
  { "glMatrixMult3x2fNV", "GL_NV_path_rendering", glMatrixMult3x2fNV },
  { "glMatrixMult3x3fNV", "GL_NV_path_rendering", glMatrixMult3x3fNV },
  { "glMatrixMultTranspose3x3fNV", "GL_NV_path_rendering", glMatrixMultTranspose3x3fNV },
  { "glMatrixMultTransposedEXT", "GL_EXT_direct_state_access", glMatrixMultTransposedEXT },
  { "glMatrixMultTransposefEXT", "GL_EXT_direct_state_access", glMatrixMultTransposefEXT },
  { "glMatrixMultdEXT", "GL_EXT_direct_state_access", glMatrixMultdEXT },
  { "glMatrixMultfEXT", "GL_EXT_direct_state_access", glMatrixMultfEXT },
  { "glMatrixOrthoEXT", "GL_EXT_direct_state_access", glMatrixOrthoEXT },
  { "glMatrixPopEXT", "GL_EXT_direct_state_access", glMatrixPopEXT },
  { "glMatrixPushEXT", "GL_EXT_direct_state_access", glMatrixPushEXT },
  { "glMatrixRotatedEXT", "GL_EXT_direct_state_access", glMatrixRotatedEXT },
  { "glMatrixRotatefEXT", "GL_EXT_direct_state_access", glMatrixRotatefEXT },
  { "glMatrixScaledEXT", "GL_EXT_direct_state_access", glMatrixScaledEXT },
  { "glMatrixScalefEXT", "GL_EXT_direct_state_access", glMatrixScalefEXT },
  { "glMatrixTranslatedEXT", "GL_EXT_direct_state_access", glMatrixTranslatedEXT },
  { "glMatrixTranslatefEXT", "GL_EXT_direct_state_access", glMatrixTranslatefEXT },
  { "glMaxShaderCompilerThreadsARB", "GL_ARB_parallel_shader_compile", glMaxShaderCompilerThreadsARB },
  { "glMaxShaderCompilerThreadsKHR", "GL_KHR_parallel_shader_compile", glMaxShaderCompilerThreadsKHR },
  { "glMemoryBarrier", "GL_ARB_shader_image_load_store GL_VERSION_4_2", glMemoryBarrier },
  { "glMemoryBarrierByRegion", "GL_ARB_ES3_1_compatibility GL_VERSION_4_5", glMemoryBarrierByRegion },
  { "glMemoryBarrierEXT", "GL_EXT_shader_image_load_store", glMemoryBarrierEXT },
  { "glMemoryObjectParameterivEXT", "GL_EXT_memory_object", glMemoryObjectParameterivEXT },
  { "glMinSampleShading", "GL_VERSION_4_0", glMinSampleShading },
  { "glMinSampleShadingARB", "GL_ARB_sample_shading", glMinSampleShadingARB },
  { "glMinmax", "GL_ARB_imaging", glMinmax },
  { "glMinmaxEXT", "GL_EXT_histogram", glMinmaxEXT },
  { "glMultMatrixxOES", "GL_OES_fixed_point", glMultMatrixxOES },
  { "glMultTransposeMatrixd", "GL_VERSION_1_3", glMultTransposeMatrixd },
  { "glMultTransposeMatrixdARB", "GL_ARB_transpose_matrix", glMultTransposeMatrixdARB },
  { "glMultTransposeMatrixf", "GL_VERSION_1_3", glMultTransposeMatrixf },
  { "glMultTransposeMatrixfARB", "GL_ARB_transpose_matrix", glMultTransposeMatrixfARB },
  { "glMultTransposeMatrixxOES", "GL_OES_fixed_point", glMultTransposeMatrixxOES },
  { "glMultiDrawArrays", "GL_VERSION_1_4", glMultiDrawArrays },
  { "glMultiDrawArraysEXT", "GL_EXT_multi_draw_arrays", glMultiDrawArraysEXT },
  { "glMultiDrawArraysIndirect", "GL_ARB_multi_draw_indirect GL_VERSION_4_3", glMultiDrawArraysIndirect },
  { "glMultiDrawArraysIndirectAMD", "GL_AMD_multi_draw_indirect", glMultiDrawArraysIndirectAMD },
  { "glMultiDrawArraysIndirectBindlessCountNV", "GL_NV_bindless_multi_draw_indirect_count", glMultiDrawArraysIndirectBindlessCountNV },
  { "glMultiDrawArraysIndirectBindlessNV", "GL_NV_bindless_multi_draw_indirect", glMultiDrawArraysIndirectBindlessNV },
  { "glMultiDrawArraysIndirectCount", "GL_VERSION_4_6", glMultiDrawArraysIndirectCount },
  { "glMultiDrawArraysIndirectCountARB", "GL_ARB_indirect_parameters", glMultiDrawArraysIndirectCountARB },
  { "glMultiDrawElementArrayAPPLE", "GL_APPLE_element_array", glMultiDrawElementArrayAPPLE },
  { "glMultiDrawElements", "GL_VERSION_1_4", glMultiDrawElements },
  { "glMultiDrawElementsBaseVertex", "GL_ARB_draw_elements_base_vertex GL_VERSION_3_2", glMultiDrawElementsBaseVertex },
  { "glMultiDrawElementsEXT", "GL_EXT_multi_draw_arrays", glMultiDrawElementsEXT },
  { "glMultiDrawElementsIndirect", "GL_ARB_multi_draw_indirect GL_VERSION_4_3", glMultiDrawElementsIndirect },
  { "glMultiDrawElementsIndirectAMD", "GL_AMD_multi_draw_indirect", glMultiDrawElementsIndirectAMD },
  { "glMultiDrawElementsIndirectBindlessCountNV", "GL_NV_bindless_multi_draw_indirect_count", glMultiDrawElementsIndirectBindlessCountNV },
  { "glMultiDrawElementsIndirectBindlessNV", "GL_NV_bindless_multi_draw_indirect", glMultiDrawElementsIndirectBindlessNV },
  { "glMultiDrawElementsIndirectCount", "GL_VERSION_4_6", glMultiDrawElementsIndirectCount },
  { "glMultiDrawElementsIndirectCountARB", "GL_ARB_indirect_parameters", glMultiDrawElementsIndirectCountARB },
  { "glMultiDrawRangeElementArrayAPPLE", "GL_APPLE_element_array", glMultiDrawRangeElementArrayAPPLE },
  { "glMultiModeDrawArraysIBM", "GL_IBM_multimode_draw_arrays", glMultiModeDrawArraysIBM },
  { "glMultiModeDrawElementsIBM", "GL_IBM_multimode_draw_arrays", glMultiModeDrawElementsIBM },
  { "glMultiTexBufferEXT", "GL_EXT_direct_state_access", glMultiTexBufferEXT },
  { "glMultiTexCoord1bOES", "GL_OES_byte_coordinates", glMultiTexCoord1bOES },
  { "glMultiTexCoord1bvOES", "GL_OES_byte_coordinates", glMultiTexCoord1bvOES },
  { "glMultiTexCoord1d", "GL_VERSION_1_3", glMultiTexCoord1d },
  { "glMultiTexCoord1dARB", "GL_ARB_multitexture", glMultiTexCoord1dARB },
  { "glMultiTexCoord1dSGIS", "GL_SGIS_multitexture", glMultiTexCoord1dSGIS },
  { "glMultiTexCoord1dv", "GL_VERSION_1_3", glMultiTexCoord1dv },
  { "glMultiTexCoord1dvARB", "GL_ARB_multitexture", glMultiTexCoord1dvARB },
  { "glMultiTexCoord1dvSGIS", "GL_SGIS_multitexture", glMultiTexCoord1dvSGIS },
  { "glMultiTexCoord1f", "GL_VERSION_1_3", glMultiTexCoord1f },
  { "glMultiTexCoord1fARB", "GL_ARB_multitexture", glMultiTexCoord1fARB },
  { "glMultiTexCoord1fSGIS", "GL_SGIS_multitexture", glMultiTexCoord1fSGIS },
  { "glMultiTexCoord1fv", "GL_VERSION_1_3", glMultiTexCoord1fv },
  { "glMultiTexCoord1fvARB", "GL_ARB_multitexture", glMultiTexCoord1fvARB },
  { "glMultiTexCoord1fvSGIS", "GL_SGIS_multitexture", glMultiTexCoord1fvSGIS },
  { "glMultiTexCoord1hNV", "GL_NV_half_float", glMultiTexCoord1hNV },
  { "glMultiTexCoord1hvNV", "GL_NV_half_float", glMultiTexCoord1hvNV },
  { "glMultiTexCoord1i", "GL_VERSION_1_3", glMultiTexCoord1i },
  { "glMultiTexCoord1iARB", "GL_ARB_multitexture", glMultiTexCoord1iARB },
  { "glMultiTexCoord1iSGIS", "GL_SGIS_multitexture", glMultiTexCoord1iSGIS },
  { "glMultiTexCoord1iv", "GL_VERSION_1_3", glMultiTexCoord1iv },
  { "glMultiTexCoord1ivARB", "GL_ARB_multitexture", glMultiTexCoord1ivARB },
  { "glMultiTexCoord1ivSGIS", "GL_SGIS_multitexture", glMultiTexCoord1ivSGIS },
  { "glMultiTexCoord1s", "GL_VERSION_1_3", glMultiTexCoord1s },
  { "glMultiTexCoord1sARB", "GL_ARB_multitexture", glMultiTexCoord1sARB },
  { "glMultiTexCoord1sSGIS", "GL_SGIS_multitexture", glMultiTexCoord1sSGIS },
  { "glMultiTexCoord1sv", "GL_VERSION_1_3", glMultiTexCoord1sv },
  { "glMultiTexCoord1svARB", "GL_ARB_multitexture", glMultiTexCoord1svARB },
  { "glMultiTexCoord1svSGIS", "GL_SGIS_multitexture", glMultiTexCoord1svSGIS },
  { "glMultiTexCoord1xOES", "GL_OES_fixed_point", glMultiTexCoord1xOES },
  { "glMultiTexCoord1xvOES", "GL_OES_fixed_point", glMultiTexCoord1xvOES },
  { "glMultiTexCoord2bOES", "GL_OES_byte_coordinates", glMultiTexCoord2bOES },
  { "glMultiTexCoord2bvOES", "GL_OES_byte_coordinates", glMultiTexCoord2bvOES },
  { "glMultiTexCoord2d", "GL_VERSION_1_3", glMultiTexCoord2d },
  { "glMultiTexCoord2dARB", "GL_ARB_multitexture", glMultiTexCoord2dARB },
  { "glMultiTexCoord2dSGIS", "GL_SGIS_multitexture", glMultiTexCoord2dSGIS },
  { "glMultiTexCoord2dv", "GL_VERSION_1_3", glMultiTexCoord2dv },
  { "glMultiTexCoord2dvARB", "GL_ARB_multitexture", glMultiTexCoord2dvARB },
  { "glMultiTexCoord2dvSGIS", "GL_SGIS_multitexture", glMultiTexCoord2dvSGIS },
  { "glMultiTexCoord2f", "GL_VERSION_1_3", glMultiTexCoord2f },
  { "glMultiTexCoord2fARB", "GL_ARB_multitexture", glMultiTexCoord2fARB },
  { "glMultiTexCoord2fSGIS", "GL_SGIS_multitexture", glMultiTexCoord2fSGIS },
  { "glMultiTexCoord2fv", "GL_VERSION_1_3", glMultiTexCoord2fv },
  { "glMultiTexCoord2fvARB", "GL_ARB_multitexture", glMultiTexCoord2fvARB },
  { "glMultiTexCoord2fvSGIS", "GL_SGIS_multitexture", glMultiTexCoord2fvSGIS },
  { "glMultiTexCoord2hNV", "GL_NV_half_float", glMultiTexCoord2hNV },
  { "glMultiTexCoord2hvNV", "GL_NV_half_float", glMultiTexCoord2hvNV },
  { "glMultiTexCoord2i", "GL_VERSION_1_3", glMultiTexCoord2i },
  { "glMultiTexCoord2iARB", "GL_ARB_multitexture", glMultiTexCoord2iARB },
  { "glMultiTexCoord2iSGIS", "GL_SGIS_multitexture", glMultiTexCoord2iSGIS },
  { "glMultiTexCoord2iv", "GL_VERSION_1_3", glMultiTexCoord2iv },
  { "glMultiTexCoord2ivARB", "GL_ARB_multitexture", glMultiTexCoord2ivARB },
  { "glMultiTexCoord2ivSGIS", "GL_SGIS_multitexture", glMultiTexCoord2ivSGIS },
  { "glMultiTexCoord2s", "GL_VERSION_1_3", glMultiTexCoord2s },
  { "glMultiTexCoord2sARB", "GL_ARB_multitexture", glMultiTexCoord2sARB },
  { "glMultiTexCoord2sSGIS", "GL_SGIS_multitexture", glMultiTexCoord2sSGIS },
  { "glMultiTexCoord2sv", "GL_VERSION_1_3", glMultiTexCoord2sv },
  { "glMultiTexCoord2svARB", "GL_ARB_multitexture", glMultiTexCoord2svARB },
  { "glMultiTexCoord2svSGIS", "GL_SGIS_multitexture", glMultiTexCoord2svSGIS },
  { "glMultiTexCoord2xOES", "GL_OES_fixed_point", glMultiTexCoord2xOES },
  { "glMultiTexCoord2xvOES", "GL_OES_fixed_point", glMultiTexCoord2xvOES },
  { "glMultiTexCoord3bOES", "GL_OES_byte_coordinates", glMultiTexCoord3bOES },
  { "glMultiTexCoord3bvOES", "GL_OES_byte_coordinates", glMultiTexCoord3bvOES },
  { "glMultiTexCoord3d", "GL_VERSION_1_3", glMultiTexCoord3d },
  { "glMultiTexCoord3dARB", "GL_ARB_multitexture", glMultiTexCoord3dARB },
  { "glMultiTexCoord3dSGIS", "GL_SGIS_multitexture", glMultiTexCoord3dSGIS },
  { "glMultiTexCoord3dv", "GL_VERSION_1_3", glMultiTexCoord3dv },
  { "glMultiTexCoord3dvARB", "GL_ARB_multitexture", glMultiTexCoord3dvARB },
  { "glMultiTexCoord3dvSGIS", "GL_SGIS_multitexture", glMultiTexCoord3dvSGIS },
  { "glMultiTexCoord3f", "GL_VERSION_1_3", glMultiTexCoord3f },
  { "glMultiTexCoord3fARB", "GL_ARB_multitexture", glMultiTexCoord3fARB },
  { "glMultiTexCoord3fSGIS", "GL_SGIS_multitexture", glMultiTexCoord3fSGIS },
  { "glMultiTexCoord3fv", "GL_VERSION_1_3", glMultiTexCoord3fv },
  { "glMultiTexCoord3fvARB", "GL_ARB_multitexture", glMultiTexCoord3fvARB },
  { "glMultiTexCoord3fvSGIS", "GL_SGIS_multitexture", glMultiTexCoord3fvSGIS },
  { "glMultiTexCoord3hNV", "GL_NV_half_float", glMultiTexCoord3hNV },
  { "glMultiTexCoord3hvNV", "GL_NV_half_float", glMultiTexCoord3hvNV },
  { "glMultiTexCoord3i", "GL_VERSION_1_3", glMultiTexCoord3i },
  { "glMultiTexCoord3iARB", "GL_ARB_multitexture", glMultiTexCoord3iARB },
  { "glMultiTexCoord3iSGIS", "GL_SGIS_multitexture", glMultiTexCoord3iSGIS },
  { "glMultiTexCoord3iv", "GL_VERSION_1_3", glMultiTexCoord3iv },
  { "glMultiTexCoord3ivARB", "GL_ARB_multitexture", glMultiTexCoord3ivARB },
  { "glMultiTexCoord3ivSGIS", "GL_SGIS_multitexture", glMultiTexCoord3ivSGIS },
  { "glMultiTexCoord3s", "GL_VERSION_1_3", glMultiTexCoord3s },
  { "glMultiTexCoord3sARB", "GL_ARB_multitexture", glMultiTexCoord3sARB },
  { "glMultiTexCoord3sSGIS", "GL_SGIS_multitexture", glMultiTexCoord3sSGIS },
  { "glMultiTexCoord3sv", "GL_VERSION_1_3", glMultiTexCoord3sv },
  { "glMultiTexCoord3svARB", "GL_ARB_multitexture", glMultiTexCoord3svARB },
  { "glMultiTexCoord3svSGIS", "GL_SGIS_multitexture", glMultiTexCoord3svSGIS },
  { "glMultiTexCoord3xOES", "GL_OES_fixed_point", glMultiTexCoord3xOES },
  { "glMultiTexCoord3xvOES", "GL_OES_fixed_point", glMultiTexCoord3xvOES },
  { "glMultiTexCoord4bOES", "GL_OES_byte_coordinates", glMultiTexCoord4bOES },
  { "glMultiTexCoord4bvOES", "GL_OES_byte_coordinates", glMultiTexCoord4bvOES },
  { "glMultiTexCoord4d", "GL_VERSION_1_3", glMultiTexCoord4d },
  { "glMultiTexCoord4dARB", "GL_ARB_multitexture", glMultiTexCoord4dARB },
  { "glMultiTexCoord4dSGIS", "GL_SGIS_multitexture", glMultiTexCoord4dSGIS },
  { "glMultiTexCoord4dv", "GL_VERSION_1_3", glMultiTexCoord4dv },
  { "glMultiTexCoord4dvARB", "GL_ARB_multitexture", glMultiTexCoord4dvARB },
  { "glMultiTexCoord4dvSGIS", "GL_SGIS_multitexture", glMultiTexCoord4dvSGIS },
  { "glMultiTexCoord4f", "GL_VERSION_1_3", glMultiTexCoord4f },
  { "glMultiTexCoord4fARB", "GL_ARB_multitexture", glMultiTexCoord4fARB },
  { "glMultiTexCoord4fSGIS", "GL_SGIS_multitexture", glMultiTexCoord4fSGIS },
  { "glMultiTexCoord4fv", "GL_VERSION_1_3", glMultiTexCoord4fv },
  { "glMultiTexCoord4fvARB", "GL_ARB_multitexture", glMultiTexCoord4fvARB },
  { "glMultiTexCoord4fvSGIS", "GL_SGIS_multitexture", glMultiTexCoord4fvSGIS },
  { "glMultiTexCoord4hNV", "GL_NV_half_float", glMultiTexCoord4hNV },
  { "glMultiTexCoord4hvNV", "GL_NV_half_float", glMultiTexCoord4hvNV },
  { "glMultiTexCoord4i", "GL_VERSION_1_3", glMultiTexCoord4i },
  { "glMultiTexCoord4iARB", "GL_ARB_multitexture", glMultiTexCoord4iARB },
  { "glMultiTexCoord4iSGIS", "GL_SGIS_multitexture", glMultiTexCoord4iSGIS },
  { "glMultiTexCoord4iv", "GL_VERSION_1_3", glMultiTexCoord4iv },
  { "glMultiTexCoord4ivARB", "GL_ARB_multitexture", glMultiTexCoord4ivARB },
  { "glMultiTexCoord4ivSGIS", "GL_SGIS_multitexture", glMultiTexCoord4ivSGIS },
  { "glMultiTexCoord4s", "GL_VERSION_1_3", glMultiTexCoord4s },
  { "glMultiTexCoord4sARB", "GL_ARB_multitexture", glMultiTexCoord4sARB },
  { "glMultiTexCoord4sSGIS", "GL_SGIS_multitexture", glMultiTexCoord4sSGIS },
  { "glMultiTexCoord4sv", "GL_VERSION_1_3", glMultiTexCoord4sv },
  { "glMultiTexCoord4svARB", "GL_ARB_multitexture", glMultiTexCoord4svARB },
  { "glMultiTexCoord4svSGIS", "GL_SGIS_multitexture", glMultiTexCoord4svSGIS },
  { "glMultiTexCoord4xOES", "GL_OES_fixed_point", glMultiTexCoord4xOES },
  { "glMultiTexCoord4xvOES", "GL_OES_fixed_point", glMultiTexCoord4xvOES },
  { "glMultiTexCoordP1ui", "GL_ARB_vertex_type_2_10_10_10_rev GL_VERSION_3_3", glMultiTexCoordP1ui },
  { "glMultiTexCoordP1uiv", "GL_ARB_vertex_type_2_10_10_10_rev GL_VERSION_3_3", glMultiTexCoordP1uiv },
  { "glMultiTexCoordP2ui", "GL_ARB_vertex_type_2_10_10_10_rev GL_VERSION_3_3", glMultiTexCoordP2ui },
  { "glMultiTexCoordP2uiv", "GL_ARB_vertex_type_2_10_10_10_rev GL_VERSION_3_3", glMultiTexCoordP2uiv },
  { "glMultiTexCoordP3ui", "GL_ARB_vertex_type_2_10_10_10_rev GL_VERSION_3_3", glMultiTexCoordP3ui },
  { "glMultiTexCoordP3uiv", "GL_ARB_vertex_type_2_10_10_10_rev GL_VERSION_3_3", glMultiTexCoordP3uiv },
  { "glMultiTexCoordP4ui", "GL_ARB_vertex_type_2_10_10_10_rev GL_VERSION_3_3", glMultiTexCoordP4ui },
  { "glMultiTexCoordP4uiv", "GL_ARB_vertex_type_2_10_10_10_rev GL_VERSION_3_3", glMultiTexCoordP4uiv },
  { "glMultiTexCoordPointerEXT", "GL_EXT_direct_state_access", glMultiTexCoordPointerEXT },
  { "glMultiTexCoordPointerSGIS", "GL_SGIS_multitexture", glMultiTexCoordPointerSGIS },
  { "glMultiTexEnvfEXT", "GL_EXT_direct_state_access", glMultiTexEnvfEXT },
  { "glMultiTexEnvfvEXT", "GL_EXT_direct_state_access", glMultiTexEnvfvEXT },
  { "glMultiTexEnviEXT", "GL_EXT_direct_state_access", glMultiTexEnviEXT },
  { "glMultiTexEnvivEXT", "GL_EXT_direct_state_access", glMultiTexEnvivEXT },
  { "glMultiTexGendEXT", "GL_EXT_direct_state_access", glMultiTexGendEXT },
  { "glMultiTexGendvEXT", "GL_EXT_direct_state_access", glMultiTexGendvEXT },
  { "glMultiTexGenfEXT", "GL_EXT_direct_state_access", glMultiTexGenfEXT },
  { "glMultiTexGenfvEXT", "GL_EXT_direct_state_access", glMultiTexGenfvEXT },
  { "glMultiTexGeniEXT", "GL_EXT_direct_state_access", glMultiTexGeniEXT },
  { "glMultiTexGenivEXT", "GL_EXT_direct_state_access", glMultiTexGenivEXT },
  { "glMultiTexImage1DEXT", "GL_EXT_direct_state_access", glMultiTexImage1DEXT },
  { "glMultiTexImage2DEXT", "GL_EXT_direct_state_access", glMultiTexImage2DEXT },
  { "glMultiTexImage3DEXT", "GL_EXT_direct_state_access", glMultiTexImage3DEXT },
  { "glMultiTexParameterIivEXT", "GL_EXT_direct_state_access", glMultiTexParameterIivEXT },
  { "glMultiTexParameterIuivEXT", "GL_EXT_direct_state_access", glMultiTexParameterIuivEXT },
  { "glMultiTexParameterfEXT", "GL_EXT_direct_state_access", glMultiTexParameterfEXT },
  { "glMultiTexParameterfvEXT", "GL_EXT_direct_state_access", glMultiTexParameterfvEXT },
  { "glMultiTexParameteriEXT", "GL_EXT_direct_state_access", glMultiTexParameteriEXT },
  { "glMultiTexParameterivEXT", "GL_EXT_direct_state_access", glMultiTexParameterivEXT },
  { "glMultiTexRenderbufferEXT", "GL_EXT_direct_state_access", glMultiTexRenderbufferEXT },
  { "glMultiTexSubImage1DEXT", "GL_EXT_direct_state_access", glMultiTexSubImage1DEXT },
  { "glMultiTexSubImage2DEXT", "GL_EXT_direct_state_access", glMultiTexSubImage2DEXT },
  { "glMultiTexSubImage3DEXT", "GL_EXT_direct_state_access", glMultiTexSubImage3DEXT },
  { "glMulticastBarrierNV", "GL_NV_gpu_multicast", glMulticastBarrierNV },
  { "glMulticastBlitFramebufferNV", "GL_NV_gpu_multicast", glMulticastBlitFramebufferNV },
  { "glMulticastBufferSubDataNV", "GL_NV_gpu_multicast", glMulticastBufferSubDataNV },
  { "glMulticastCopyBufferSubDataNV", "GL_NV_gpu_multicast", glMulticastCopyBufferSubDataNV },
  { "glMulticastCopyImageSubDataNV", "GL_NV_gpu_multicast", glMulticastCopyImageSubDataNV },
  { "glMulticastFramebufferSampleLocationsfvNV", "GL_NV_gpu_multicast", glMulticastFramebufferSampleLocationsfvNV },
  { "glMulticastGetQueryObjecti64vNV", "GL_NV_gpu_multicast", glMulticastGetQueryObjecti64vNV },
  { "glMulticastGetQueryObjectivNV", "GL_NV_gpu_multicast", glMulticastGetQueryObjectivNV },
  { "glMulticastGetQueryObjectui64vNV", "GL_NV_gpu_multicast", glMulticastGetQueryObjectui64vNV },
  { "glMulticastGetQueryObjectuivNV", "GL_NV_gpu_multicast", glMulticastGetQueryObjectuivNV },
  { "glMulticastWaitSyncNV", "GL_NV_gpu_multicast", glMulticastWaitSyncNV },
  { "glNamedBufferData", "GL_ARB_direct_state_access GL_VERSION_4_5", glNamedBufferData },
  { "glNamedBufferDataEXT", "GL_EXT_direct_state_access", glNamedBufferDataEXT },
  { "glNamedBufferPageCommitmentARB", "GL_ARB_sparse_buffer", glNamedBufferPageCommitmentARB },
  { "glNamedBufferPageCommitmentEXT", "GL_ARB_sparse_buffer", glNamedBufferPageCommitmentEXT },
  { "glNamedBufferStorage", "GL_ARB_direct_state_access GL_VERSION_4_5", glNamedBufferStorage },
  { "glNamedBufferStorageEXT", "GL_EXT_direct_state_access", glNamedBufferStorageEXT },
  { "glNamedBufferStorageExternalEXT", "GL_EXT_external_buffer", glNamedBufferStorageExternalEXT },
  { "glNamedBufferStorageMemEXT", "GL_EXT_memory_object", glNamedBufferStorageMemEXT },
  { "glNamedBufferSubData", "GL_ARB_direct_state_access GL_VERSION_4_5", glNamedBufferSubData },
  { "glNamedBufferSubDataEXT", "GL_EXT_direct_state_access", glNamedBufferSubDataEXT },
  { "glNamedCopyBufferSubDataEXT", "GL_EXT_direct_state_access", glNamedCopyBufferSubDataEXT },
  { "glNamedFramebufferDrawBuffer", "GL_ARB_direct_state_access GL_VERSION_4_5", glNamedFramebufferDrawBuffer },
  { "glNamedFramebufferDrawBuffers", "GL_ARB_direct_state_access GL_VERSION_4_5", glNamedFramebufferDrawBuffers },
  { "glNamedFramebufferParameteri", "GL_ARB_direct_state_access GL_VERSION_4_5", glNamedFramebufferParameteri },
  { "glNamedFramebufferParameteriEXT", "GL_EXT_direct_state_access", glNamedFramebufferParameteriEXT },
  { "glNamedFramebufferReadBuffer", "GL_ARB_direct_state_access GL_VERSION_4_5", glNamedFramebufferReadBuffer },
  { "glNamedFramebufferRenderbuffer", "GL_ARB_direct_state_access GL_VERSION_4_5", glNamedFramebufferRenderbuffer },
  { "glNamedFramebufferRenderbufferEXT", "GL_EXT_direct_state_access", glNamedFramebufferRenderbufferEXT },
  { "glNamedFramebufferSampleLocationsfvARB", "GL_ARB_sample_locations", glNamedFramebufferSampleLocationsfvARB },
  { "glNamedFramebufferSampleLocationsfvNV", "GL_NV_sample_locations", glNamedFramebufferSampleLocationsfvNV },
  { "glNamedFramebufferSamplePositionsfvAMD", "GL_AMD_framebuffer_sample_positions", glNamedFramebufferSamplePositionsfvAMD },
  { "glNamedFramebufferTexture", "GL_ARB_direct_state_access GL_VERSION_4_5", glNamedFramebufferTexture },
  { "glNamedFramebufferTexture1DEXT", "GL_EXT_direct_state_access", glNamedFramebufferTexture1DEXT },
  { "glNamedFramebufferTexture2DEXT", "GL_EXT_direct_state_access", glNamedFramebufferTexture2DEXT },
  { "glNamedFramebufferTexture3DEXT", "GL_EXT_direct_state_access", glNamedFramebufferTexture3DEXT },
  { "glNamedFramebufferTextureEXT", "GL_EXT_direct_state_access", glNamedFramebufferTextureEXT },
  { "glNamedFramebufferTextureFaceEXT", "GL_EXT_direct_state_access", glNamedFramebufferTextureFaceEXT },
  { "glNamedFramebufferTextureLayer", "GL_ARB_direct_state_access GL_VERSION_4_5", glNamedFramebufferTextureLayer },
  { "glNamedFramebufferTextureLayerEXT", "GL_EXT_direct_state_access", glNamedFramebufferTextureLayerEXT },
  { "glNamedProgramLocalParameter4dEXT", "GL_EXT_direct_state_access", glNamedProgramLocalParameter4dEXT },
  { "glNamedProgramLocalParameter4dvEXT", "GL_EXT_direct_state_access", glNamedProgramLocalParameter4dvEXT },
  { "glNamedProgramLocalParameter4fEXT", "GL_EXT_direct_state_access", glNamedProgramLocalParameter4fEXT },
  { "glNamedProgramLocalParameter4fvEXT", "GL_EXT_direct_state_access", glNamedProgramLocalParameter4fvEXT },
  { "glNamedProgramLocalParameterI4iEXT", "GL_EXT_direct_state_access", glNamedProgramLocalParameterI4iEXT },
  { "glNamedProgramLocalParameterI4ivEXT", "GL_EXT_direct_state_access", glNamedProgramLocalParameterI4ivEXT },
  { "glNamedProgramLocalParameterI4uiEXT", "GL_EXT_direct_state_access", glNamedProgramLocalParameterI4uiEXT },
  { "glNamedProgramLocalParameterI4uivEXT", "GL_EXT_direct_state_access", glNamedProgramLocalParameterI4uivEXT },
  { "glNamedProgramLocalParameters4fvEXT", "GL_EXT_direct_state_access", glNamedProgramLocalParameters4fvEXT },
  { "glNamedProgramLocalParametersI4ivEXT", "GL_EXT_direct_state_access", glNamedProgramLocalParametersI4ivEXT },
  { "glNamedProgramLocalParametersI4uivEXT", "GL_EXT_direct_state_access", glNamedProgramLocalParametersI4uivEXT },
  { "glNamedProgramStringEXT", "GL_EXT_direct_state_access", glNamedProgramStringEXT },
  { "glNamedRenderbufferStorage", "GL_ARB_direct_state_access GL_VERSION_4_5", glNamedRenderbufferStorage },
  { "glNamedRenderbufferStorageEXT", "GL_EXT_direct_state_access", glNamedRenderbufferStorageEXT },
  { "glNamedRenderbufferStorageMultisample", "GL_ARB_direct_state_access GL_VERSION_4_5", glNamedRenderbufferStorageMultisample },
  { "glNamedRenderbufferStorageMultisampleCoverageEXT", "GL_EXT_direct_state_access", glNamedRenderbufferStorageMultisampleCoverageEXT },
  { "glNamedRenderbufferStorageMultisampleEXT", "GL_EXT_direct_state_access", glNamedRenderbufferStorageMultisampleEXT },
  { "glNamedStringARB", "GL_ARB_shading_language_include", glNamedStringARB },
  { "glNewBufferRegion", "GL_KTX_buffer_region", glNewBufferRegion },
  { "glNewObjectBufferATI", "GL_ATI_vertex_array_object", glNewObjectBufferATI },
  { "glNormal3fVertex3fSUN", "GL_SUN_vertex", glNormal3fVertex3fSUN },
  { "glNormal3fVertex3fvSUN", "GL_SUN_vertex", glNormal3fVertex3fvSUN },
  { "glNormal3hNV", "GL_NV_half_float", glNormal3hNV },
  { "glNormal3hvNV", "GL_NV_half_float", glNormal3hvNV },
  { "glNormal3xOES", "GL_OES_fixed_point", glNormal3xOES },
  { "glNormal3xvOES", "GL_OES_fixed_point", glNormal3xvOES },
  { "glNormalFormatNV", "GL_NV_vertex_buffer_unified_memory", glNormalFormatNV },
  { "glNormalP3ui", "GL_ARB_vertex_type_2_10_10_10_rev GL_VERSION_3_3", glNormalP3ui },
  { "glNormalP3uiv", "GL_ARB_vertex_type_2_10_10_10_rev GL_VERSION_3_3", glNormalP3uiv },
  { "glNormalPointerEXT", "GL_EXT_vertex_array", glNormalPointerEXT },
  { "glNormalPointerListIBM", "GL_IBM_vertex_array_lists", glNormalPointerListIBM },
  { "glNormalPointervINTEL", "GL_INTEL_parallel_arrays", glNormalPointervINTEL },
  { "glNormalStream3bATI", "GL_ATI_vertex_streams", glNormalStream3bATI },
  { "glNormalStream3bvATI", "GL_ATI_vertex_streams", glNormalStream3bvATI },
  { "glNormalStream3dATI", "GL_ATI_vertex_streams", glNormalStream3dATI },
  { "glNormalStream3dvATI", "GL_ATI_vertex_streams", glNormalStream3dvATI },
  { "glNormalStream3fATI", "GL_ATI_vertex_streams", glNormalStream3fATI },
  { "glNormalStream3fvATI", "GL_ATI_vertex_streams", glNormalStream3fvATI },
  { "glNormalStream3iATI", "GL_ATI_vertex_streams", glNormalStream3iATI },
  { "glNormalStream3ivATI", "GL_ATI_vertex_streams", glNormalStream3ivATI },
  { "glNormalStream3sATI", "GL_ATI_vertex_streams", glNormalStream3sATI },
  { "glNormalStream3svATI", "GL_ATI_vertex_streams", glNormalStream3svATI },
  { "glObjectLabel", "GL_KHR_debug GL_VERSION_4_3", glObjectLabel },
  { "glObjectPtrLabel", "GL_KHR_debug GL_VERSION_4_3", glObjectPtrLabel },
  { "glObjectPurgeableAPPLE", "GL_APPLE_object_purgeable", glObjectPurgeableAPPLE },
  { "glObjectUnpurgeableAPPLE", "GL_APPLE_object_purgeable", glObjectUnpurgeableAPPLE },
  { "glOrthofOES", "GL_OES_single_precision", glOrthofOES },
  { "glOrthoxOES", "GL_OES_fixed_point", glOrthoxOES },
  { "glPNTrianglesfATI", "GL_ATI_pn_triangles", glPNTrianglesfATI },
  { "glPNTrianglesiATI", "GL_ATI_pn_triangles", glPNTrianglesiATI },
  { "glPassTexCoordATI", "GL_ATI_fragment_shader", glPassTexCoordATI },
  { "glPassThroughxOES", "GL_OES_fixed_point", glPassThroughxOES },
  { "glPatchParameterfv", "GL_ARB_tessellation_shader GL_VERSION_4_0", glPatchParameterfv },
  { "glPatchParameteri", "GL_ARB_tessellation_shader GL_VERSION_4_0", glPatchParameteri },
  { "glPathColorGenNV", "GL_NV_path_rendering", glPathColorGenNV },
  { "glPathCommandsNV", "GL_NV_path_rendering", glPathCommandsNV },
  { "glPathCoordsNV", "GL_NV_path_rendering", glPathCoordsNV },
  { "glPathCoverDepthFuncNV", "GL_NV_path_rendering", glPathCoverDepthFuncNV },
  { "glPathDashArrayNV", "GL_NV_path_rendering", glPathDashArrayNV },
  { "glPathFogGenNV", "GL_NV_path_rendering", glPathFogGenNV },
  { "glPathGlyphIndexArrayNV", "GL_NV_path_rendering", glPathGlyphIndexArrayNV },
  { "glPathGlyphIndexRangeNV", "GL_NV_path_rendering", glPathGlyphIndexRangeNV },
  { "glPathGlyphRangeNV", "GL_NV_path_rendering", glPathGlyphRangeNV },
  { "glPathGlyphsNV", "GL_NV_path_rendering", glPathGlyphsNV },
  { "glPathMemoryGlyphIndexArrayNV", "GL_NV_path_rendering", glPathMemoryGlyphIndexArrayNV },
  { "glPathParameterfNV", "GL_NV_path_rendering", glPathParameterfNV },
  { "glPathParameterfvNV", "GL_NV_path_rendering", glPathParameterfvNV },
  { "glPathParameteriNV", "GL_NV_path_rendering", glPathParameteriNV },
  { "glPathParameterivNV", "GL_NV_path_rendering", glPathParameterivNV },
  { "glPathStencilDepthOffsetNV", "GL_NV_path_rendering", glPathStencilDepthOffsetNV },
  { "glPathStencilFuncNV", "GL_NV_path_rendering", glPathStencilFuncNV },
  { "glPathStringNV", "GL_NV_path_rendering", glPathStringNV },
  { "glPathSubCommandsNV", "GL_NV_path_rendering", glPathSubCommandsNV },
  { "glPathSubCoordsNV", "GL_NV_path_rendering", glPathSubCoordsNV },
  { "glPathTexGenNV", "GL_NV_path_rendering", glPathTexGenNV },
  { "glPauseTransformFeedback", "GL_ARB_transform_feedback2 GL_VERSION_4_0", glPauseTransformFeedback },
  { "glPauseTransformFeedbackNV", "GL_NV_transform_feedback2", glPauseTransformFeedbackNV },
  { "glPixelDataRangeNV", "GL_NV_pixel_data_range", glPixelDataRangeNV },
  { "glPixelMapx", "GL_OES_fixed_point", glPixelMapx },
  { "glPixelStorex", "GL_OES_fixed_point", glPixelStorex },
  { "glPixelTexGenParameterfSGIS", "GL_SGIS_pixel_texture", glPixelTexGenParameterfSGIS },
  { "glPixelTexGenParameterfvSGIS", "GL_SGIS_pixel_texture", glPixelTexGenParameterfvSGIS },
  { "glPixelTexGenParameteriSGIS", "GL_SGIS_pixel_texture", glPixelTexGenParameteriSGIS },
  { "glPixelTexGenParameterivSGIS", "GL_SGIS_pixel_texture", glPixelTexGenParameterivSGIS },
  { "glPixelTexGenSGIX", "GL_SGIX_pixel_texture", glPixelTexGenSGIX },
  { "glPixelTransferxOES", "GL_OES_fixed_point", glPixelTransferxOES },
  { "glPixelTransformParameterfEXT", "GL_EXT_pixel_transform", glPixelTransformParameterfEXT },
  { "glPixelTransformParameterfvEXT", "GL_EXT_pixel_transform", glPixelTransformParameterfvEXT },
  { "glPixelTransformParameteriEXT", "GL_EXT_pixel_transform", glPixelTransformParameteriEXT },
  { "glPixelTransformParameterivEXT", "GL_EXT_pixel_transform", glPixelTransformParameterivEXT },
  { "glPixelZoomxOES", "GL_OES_fixed_point", glPixelZoomxOES },
  { "glPointAlongPathNV", "GL_NV_path_rendering", glPointAlongPathNV },
  { "glPointParameterf", "GL_VERSION_1_4", glPointParameterf },
  { "glPointParameterfARB", "GL_ARB_point_parameters", glPointParameterfARB },
  { "glPointParameterfEXT", "GL_EXT_point_parameters", glPointParameterfEXT },
  { "glPointParameterfSGIS", "GL_SGIS_point_parameters", glPointParameterfSGIS },
  { "glPointParameterfv", "GL_VERSION_1_4", glPointParameterfv },
  { "glPointParameterfvARB", "GL_ARB_point_parameters", glPointParameterfvARB },
  { "glPointParameterfvEXT", "GL_EXT_point_parameters", glPointParameterfvEXT },
  { "glPointParameterfvSGIS", "GL_SGIS_point_parameters", glPointParameterfvSGIS },
  { "glPointParameteri", "GL_VERSION_1_4", glPointParameteri },
  { "glPointParameteriNV", "GL_NV_point_sprite", glPointParameteriNV },
  { "glPointParameteriv", "GL_VERSION_1_4", glPointParameteriv },
  { "glPointParameterivNV", "GL_NV_point_sprite", glPointParameterivNV },
  { "glPointParameterxvOES", "GL_OES_fixed_point", glPointParameterxvOES },
  { "glPointSizexOES", "GL_OES_fixed_point", glPointSizexOES },
  { "glPollAsyncSGIX", "GL_SGIX_async", glPollAsyncSGIX },
  { "glPollInstrumentsSGIX", "GL_SGIX_instruments", glPollInstrumentsSGIX },
  { "glPolygonOffsetClamp", "GL_ARB_polygon_offset_clamp GL_VERSION_4_6", glPolygonOffsetClamp },
  { "glPolygonOffsetClampEXT", "GL_EXT_polygon_offset_clamp", glPolygonOffsetClampEXT },
  { "glPolygonOffsetEXT", "GL_EXT_polygon_offset", glPolygonOffsetEXT },
  { "glPolygonOffsetxOES", "GL_OES_fixed_point", glPolygonOffsetxOES },
  { "glPopDebugGroup", "GL_KHR_debug GL_VERSION_4_3", glPopDebugGroup },
  { "glPopGroupMarkerEXT", "GL_EXT_debug_marker", glPopGroupMarkerEXT },
  { "glPresentFrameDualFillNV", "GL_NV_present_video", glPresentFrameDualFillNV },
  { "glPresentFrameKeyedNV", "GL_NV_present_video", glPresentFrameKeyedNV },
  { "glPrimitiveBoundingBoxARB", "GL_ARB_ES3_2_compatibility", glPrimitiveBoundingBoxARB },
  { "glPrimitiveRestartIndex", "GL_VERSION_3_1", glPrimitiveRestartIndex },
  { "glPrimitiveRestartIndexNV", "GL_NV_primitive_restart", glPrimitiveRestartIndexNV },
  { "glPrimitiveRestartNV", "GL_NV_primitive_restart", glPrimitiveRestartNV },
  { "glPrioritizeTexturesEXT", "GL_EXT_texture_object", glPrioritizeTexturesEXT },
  { "glPrioritizeTexturesxOES", "GL_OES_fixed_point", glPrioritizeTexturesxOES },
  { "glProgramBinary", "GL_ARB_get_program_binary GL_VERSION_4_1", glProgramBinary },
  { "glProgramBufferParametersIivNV", "GL_NV_parameter_buffer_object", glProgramBufferParametersIivNV },
  { "glProgramBufferParametersIuivNV", "GL_NV_parameter_buffer_object", glProgramBufferParametersIuivNV },
  { "glProgramBufferParametersfvNV", "GL_NV_parameter_buffer_object", glProgramBufferParametersfvNV },
  { "glProgramEnvParameter4dARB", "GL_ARB_fragment_program GL_ARB_vertex_program", glProgramEnvParameter4dARB },
  { "glProgramEnvParameter4dvARB", "GL_ARB_fragment_program GL_ARB_vertex_program", glProgramEnvParameter4dvARB },
  { "glProgramEnvParameter4fARB", "GL_ARB_fragment_program GL_ARB_vertex_program", glProgramEnvParameter4fARB },
  { "glProgramEnvParameter4fvARB", "GL_ARB_fragment_program GL_ARB_vertex_program", glProgramEnvParameter4fvARB },
  { "glProgramEnvParameterI4iNV", "GL_NV_gpu_program4", glProgramEnvParameterI4iNV },
  { "glProgramEnvParameterI4ivNV", "GL_NV_gpu_program4", glProgramEnvParameterI4ivNV },
  { "glProgramEnvParameterI4uiNV", "GL_NV_gpu_program4", glProgramEnvParameterI4uiNV },
  { "glProgramEnvParameterI4uivNV", "GL_NV_gpu_program4", glProgramEnvParameterI4uivNV },
  { "glProgramEnvParameters4fvEXT", "GL_EXT_gpu_program_parameters", glProgramEnvParameters4fvEXT },
  { "glProgramEnvParametersI4ivNV", "GL_NV_gpu_program4", glProgramEnvParametersI4ivNV },
  { "glProgramEnvParametersI4uivNV", "GL_NV_gpu_program4", glProgramEnvParametersI4uivNV },
  { "glProgramLocalParameter4dARB", "GL_ARB_fragment_program GL_ARB_vertex_program", glProgramLocalParameter4dARB },
  { "glProgramLocalParameter4dvARB", "GL_ARB_fragment_program GL_ARB_vertex_program", glProgramLocalParameter4dvARB },
  { "glProgramLocalParameter4fARB", "GL_ARB_fragment_program GL_ARB_vertex_program", glProgramLocalParameter4fARB },
  { "glProgramLocalParameter4fvARB", "GL_ARB_fragment_program GL_ARB_vertex_program", glProgramLocalParameter4fvARB },
  { "glProgramLocalParameterI4iNV", "GL_NV_gpu_program4", glProgramLocalParameterI4iNV },
  { "glProgramLocalParameterI4ivNV", "GL_NV_gpu_program4", glProgramLocalParameterI4ivNV },
  { "glProgramLocalParameterI4uiNV", "GL_NV_gpu_program4", glProgramLocalParameterI4uiNV },
  { "glProgramLocalParameterI4uivNV", "GL_NV_gpu_program4", glProgramLocalParameterI4uivNV },
  { "glProgramLocalParameters4fvEXT", "GL_EXT_gpu_program_parameters", glProgramLocalParameters4fvEXT },
  { "glProgramLocalParametersI4ivNV", "GL_NV_gpu_program4", glProgramLocalParametersI4ivNV },
  { "glProgramLocalParametersI4uivNV", "GL_NV_gpu_program4", glProgramLocalParametersI4uivNV },
  { "glProgramNamedParameter4dNV", "GL_NV_fragment_program", glProgramNamedParameter4dNV },
  { "glProgramNamedParameter4dvNV", "GL_NV_fragment_program", glProgramNamedParameter4dvNV },
  { "glProgramNamedParameter4fNV", "GL_NV_fragment_program", glProgramNamedParameter4fNV },
  { "glProgramNamedParameter4fvNV", "GL_NV_fragment_program", glProgramNamedParameter4fvNV },
  { "glProgramParameter4dNV", "GL_NV_vertex_program", glProgramParameter4dNV },
  { "glProgramParameter4dvNV", "GL_NV_vertex_program", glProgramParameter4dvNV },
  { "glProgramParameter4fNV", "GL_NV_vertex_program", glProgramParameter4fNV },
  { "glProgramParameter4fvNV", "GL_NV_vertex_program", glProgramParameter4fvNV },
  { "glProgramParameteri", "GL_ARB_get_program_binary GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramParameteri },
  { "glProgramParameteriARB", "GL_ARB_geometry_shader4", glProgramParameteriARB },
  { "glProgramParameteriEXT", "GL_EXT_geometry_shader4", glProgramParameteriEXT },
  { "glProgramParameters4dvNV", "GL_NV_vertex_program", glProgramParameters4dvNV },
  { "glProgramParameters4fvNV", "GL_NV_vertex_program", glProgramParameters4fvNV },
  { "glProgramPathFragmentInputGenNV", "GL_NV_path_rendering", glProgramPathFragmentInputGenNV },
  { "glProgramStringARB", "GL_ARB_fragment_program GL_ARB_vertex_program", glProgramStringARB },
  { "glProgramSubroutineParametersuivNV", "GL_NV_gpu_program5", glProgramSubroutineParametersuivNV },
  { "glProgramUniform1d", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniform1d },
  { "glProgramUniform1dEXT", "GL_EXT_direct_state_access", glProgramUniform1dEXT },
  { "glProgramUniform1dv", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniform1dv },
  { "glProgramUniform1dvEXT", "GL_EXT_direct_state_access", glProgramUniform1dvEXT },
  { "glProgramUniform1f", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniform1f },
  { "glProgramUniform1fEXT", "GL_EXT_direct_state_access", glProgramUniform1fEXT },
  { "glProgramUniform1fv", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniform1fv },
  { "glProgramUniform1fvEXT", "GL_EXT_direct_state_access", glProgramUniform1fvEXT },
  { "glProgramUniform1i", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniform1i },
  { "glProgramUniform1i64ARB", "GL_ARB_gpu_shader_int64", glProgramUniform1i64ARB },
  { "glProgramUniform1i64NV", "GL_AMD_gpu_shader_int64 GL_NV_gpu_shader5", glProgramUniform1i64NV },
  { "glProgramUniform1i64vARB", "GL_ARB_gpu_shader_int64", glProgramUniform1i64vARB },
  { "glProgramUniform1i64vNV", "GL_AMD_gpu_shader_int64 GL_NV_gpu_shader5", glProgramUniform1i64vNV },
  { "glProgramUniform1iEXT", "GL_EXT_direct_state_access", glProgramUniform1iEXT },
  { "glProgramUniform1iv", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniform1iv },
  { "glProgramUniform1ivEXT", "GL_EXT_direct_state_access", glProgramUniform1ivEXT },
  { "glProgramUniform1ui", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniform1ui },
  { "glProgramUniform1ui64ARB", "GL_ARB_gpu_shader_int64", glProgramUniform1ui64ARB },
  { "glProgramUniform1ui64NV", "GL_AMD_gpu_shader_int64 GL_NV_gpu_shader5", glProgramUniform1ui64NV },
  { "glProgramUniform1ui64vARB", "GL_ARB_gpu_shader_int64", glProgramUniform1ui64vARB },
  { "glProgramUniform1ui64vNV", "GL_AMD_gpu_shader_int64 GL_NV_gpu_shader5", glProgramUniform1ui64vNV },
  { "glProgramUniform1uiEXT", "GL_EXT_direct_state_access", glProgramUniform1uiEXT },
  { "glProgramUniform1uiv", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniform1uiv },
  { "glProgramUniform1uivEXT", "GL_EXT_direct_state_access", glProgramUniform1uivEXT },
  { "glProgramUniform2d", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniform2d },
  { "glProgramUniform2dEXT", "GL_EXT_direct_state_access", glProgramUniform2dEXT },
  { "glProgramUniform2dv", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniform2dv },
  { "glProgramUniform2dvEXT", "GL_EXT_direct_state_access", glProgramUniform2dvEXT },
  { "glProgramUniform2f", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniform2f },
  { "glProgramUniform2fEXT", "GL_EXT_direct_state_access", glProgramUniform2fEXT },
  { "glProgramUniform2fv", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniform2fv },
  { "glProgramUniform2fvEXT", "GL_EXT_direct_state_access", glProgramUniform2fvEXT },
  { "glProgramUniform2i", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniform2i },
  { "glProgramUniform2i64ARB", "GL_ARB_gpu_shader_int64", glProgramUniform2i64ARB },
  { "glProgramUniform2i64NV", "GL_AMD_gpu_shader_int64 GL_NV_gpu_shader5", glProgramUniform2i64NV },
  { "glProgramUniform2i64vARB", "GL_ARB_gpu_shader_int64", glProgramUniform2i64vARB },
  { "glProgramUniform2i64vNV", "GL_AMD_gpu_shader_int64 GL_NV_gpu_shader5", glProgramUniform2i64vNV },
  { "glProgramUniform2iEXT", "GL_EXT_direct_state_access", glProgramUniform2iEXT },
  { "glProgramUniform2iv", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniform2iv },
  { "glProgramUniform2ivEXT", "GL_EXT_direct_state_access", glProgramUniform2ivEXT },
  { "glProgramUniform2ui", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniform2ui },
  { "glProgramUniform2ui64ARB", "GL_ARB_gpu_shader_int64", glProgramUniform2ui64ARB },
  { "glProgramUniform2ui64NV", "GL_AMD_gpu_shader_int64 GL_NV_gpu_shader5", glProgramUniform2ui64NV },
  { "glProgramUniform2ui64vARB", "GL_ARB_gpu_shader_int64", glProgramUniform2ui64vARB },
  { "glProgramUniform2ui64vNV", "GL_AMD_gpu_shader_int64 GL_NV_gpu_shader5", glProgramUniform2ui64vNV },
  { "glProgramUniform2uiEXT", "GL_EXT_direct_state_access", glProgramUniform2uiEXT },
  { "glProgramUniform2uiv", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniform2uiv },
  { "glProgramUniform2uivEXT", "GL_EXT_direct_state_access", glProgramUniform2uivEXT },
  { "glProgramUniform3d", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniform3d },
  { "glProgramUniform3dEXT", "GL_EXT_direct_state_access", glProgramUniform3dEXT },
  { "glProgramUniform3dv", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniform3dv },
  { "glProgramUniform3dvEXT", "GL_EXT_direct_state_access", glProgramUniform3dvEXT },
  { "glProgramUniform3f", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniform3f },
  { "glProgramUniform3fEXT", "GL_EXT_direct_state_access", glProgramUniform3fEXT },
  { "glProgramUniform3fv", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniform3fv },
  { "glProgramUniform3fvEXT", "GL_EXT_direct_state_access", glProgramUniform3fvEXT },
  { "glProgramUniform3i", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniform3i },
  { "glProgramUniform3i64ARB", "GL_ARB_gpu_shader_int64", glProgramUniform3i64ARB },
  { "glProgramUniform3i64NV", "GL_AMD_gpu_shader_int64 GL_NV_gpu_shader5", glProgramUniform3i64NV },
  { "glProgramUniform3i64vARB", "GL_ARB_gpu_shader_int64", glProgramUniform3i64vARB },
  { "glProgramUniform3i64vNV", "GL_AMD_gpu_shader_int64 GL_NV_gpu_shader5", glProgramUniform3i64vNV },
  { "glProgramUniform3iEXT", "GL_EXT_direct_state_access", glProgramUniform3iEXT },
  { "glProgramUniform3iv", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniform3iv },
  { "glProgramUniform3ivEXT", "GL_EXT_direct_state_access", glProgramUniform3ivEXT },
  { "glProgramUniform3ui", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniform3ui },
  { "glProgramUniform3ui64ARB", "GL_ARB_gpu_shader_int64", glProgramUniform3ui64ARB },
  { "glProgramUniform3ui64NV", "GL_AMD_gpu_shader_int64 GL_NV_gpu_shader5", glProgramUniform3ui64NV },
  { "glProgramUniform3ui64vARB", "GL_ARB_gpu_shader_int64", glProgramUniform3ui64vARB },
  { "glProgramUniform3ui64vNV", "GL_AMD_gpu_shader_int64 GL_NV_gpu_shader5", glProgramUniform3ui64vNV },
  { "glProgramUniform3uiEXT", "GL_EXT_direct_state_access", glProgramUniform3uiEXT },
  { "glProgramUniform3uiv", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniform3uiv },
  { "glProgramUniform3uivEXT", "GL_EXT_direct_state_access", glProgramUniform3uivEXT },
  { "glProgramUniform4d", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniform4d },
  { "glProgramUniform4dEXT", "GL_EXT_direct_state_access", glProgramUniform4dEXT },
  { "glProgramUniform4dv", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniform4dv },
  { "glProgramUniform4dvEXT", "GL_EXT_direct_state_access", glProgramUniform4dvEXT },
  { "glProgramUniform4f", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniform4f },
  { "glProgramUniform4fEXT", "GL_EXT_direct_state_access", glProgramUniform4fEXT },
  { "glProgramUniform4fv", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniform4fv },
  { "glProgramUniform4fvEXT", "GL_EXT_direct_state_access", glProgramUniform4fvEXT },
  { "glProgramUniform4i", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniform4i },
  { "glProgramUniform4i64ARB", "GL_ARB_gpu_shader_int64", glProgramUniform4i64ARB },
  { "glProgramUniform4i64NV", "GL_AMD_gpu_shader_int64 GL_NV_gpu_shader5", glProgramUniform4i64NV },
  { "glProgramUniform4i64vARB", "GL_ARB_gpu_shader_int64", glProgramUniform4i64vARB },
  { "glProgramUniform4i64vNV", "GL_AMD_gpu_shader_int64 GL_NV_gpu_shader5", glProgramUniform4i64vNV },
  { "glProgramUniform4iEXT", "GL_EXT_direct_state_access", glProgramUniform4iEXT },
  { "glProgramUniform4iv", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniform4iv },
  { "glProgramUniform4ivEXT", "GL_EXT_direct_state_access", glProgramUniform4ivEXT },
  { "glProgramUniform4ui", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniform4ui },
  { "glProgramUniform4ui64ARB", "GL_ARB_gpu_shader_int64", glProgramUniform4ui64ARB },
  { "glProgramUniform4ui64NV", "GL_AMD_gpu_shader_int64 GL_NV_gpu_shader5", glProgramUniform4ui64NV },
  { "glProgramUniform4ui64vARB", "GL_ARB_gpu_shader_int64", glProgramUniform4ui64vARB },
  { "glProgramUniform4ui64vNV", "GL_AMD_gpu_shader_int64 GL_NV_gpu_shader5", glProgramUniform4ui64vNV },
  { "glProgramUniform4uiEXT", "GL_EXT_direct_state_access", glProgramUniform4uiEXT },
  { "glProgramUniform4uiv", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniform4uiv },
  { "glProgramUniform4uivEXT", "GL_EXT_direct_state_access", glProgramUniform4uivEXT },
  { "glProgramUniformHandleui64ARB", "GL_ARB_bindless_texture", glProgramUniformHandleui64ARB },
  { "glProgramUniformHandleui64NV", "GL_NV_bindless_texture", glProgramUniformHandleui64NV },
  { "glProgramUniformHandleui64vARB", "GL_ARB_bindless_texture", glProgramUniformHandleui64vARB },
  { "glProgramUniformHandleui64vNV", "GL_NV_bindless_texture", glProgramUniformHandleui64vNV },
  { "glProgramUniformMatrix2dv", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniformMatrix2dv },
  { "glProgramUniformMatrix2dvEXT", "GL_EXT_direct_state_access", glProgramUniformMatrix2dvEXT },
  { "glProgramUniformMatrix2fv", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniformMatrix2fv },
  { "glProgramUniformMatrix2fvEXT", "GL_EXT_direct_state_access", glProgramUniformMatrix2fvEXT },
  { "glProgramUniformMatrix2x3dv", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniformMatrix2x3dv },
  { "glProgramUniformMatrix2x3dvEXT", "GL_EXT_direct_state_access", glProgramUniformMatrix2x3dvEXT },
  { "glProgramUniformMatrix2x3fv", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniformMatrix2x3fv },
  { "glProgramUniformMatrix2x3fvEXT", "GL_EXT_direct_state_access", glProgramUniformMatrix2x3fvEXT },
  { "glProgramUniformMatrix2x4dv", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniformMatrix2x4dv },
  { "glProgramUniformMatrix2x4dvEXT", "GL_EXT_direct_state_access", glProgramUniformMatrix2x4dvEXT },
  { "glProgramUniformMatrix2x4fv", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniformMatrix2x4fv },
  { "glProgramUniformMatrix2x4fvEXT", "GL_EXT_direct_state_access", glProgramUniformMatrix2x4fvEXT },
  { "glProgramUniformMatrix3dv", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniformMatrix3dv },
  { "glProgramUniformMatrix3dvEXT", "GL_EXT_direct_state_access", glProgramUniformMatrix3dvEXT },
  { "glProgramUniformMatrix3fv", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniformMatrix3fv },
  { "glProgramUniformMatrix3fvEXT", "GL_EXT_direct_state_access", glProgramUniformMatrix3fvEXT },
  { "glProgramUniformMatrix3x2dv", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniformMatrix3x2dv },
  { "glProgramUniformMatrix3x2dvEXT", "GL_EXT_direct_state_access", glProgramUniformMatrix3x2dvEXT },
  { "glProgramUniformMatrix3x2fv", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniformMatrix3x2fv },
  { "glProgramUniformMatrix3x2fvEXT", "GL_EXT_direct_state_access", glProgramUniformMatrix3x2fvEXT },
  { "glProgramUniformMatrix3x4dv", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniformMatrix3x4dv },
  { "glProgramUniformMatrix3x4dvEXT", "GL_EXT_direct_state_access", glProgramUniformMatrix3x4dvEXT },
  { "glProgramUniformMatrix3x4fv", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniformMatrix3x4fv },
  { "glProgramUniformMatrix3x4fvEXT", "GL_EXT_direct_state_access", glProgramUniformMatrix3x4fvEXT },
  { "glProgramUniformMatrix4dv", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniformMatrix4dv },
  { "glProgramUniformMatrix4dvEXT", "GL_EXT_direct_state_access", glProgramUniformMatrix4dvEXT },
  { "glProgramUniformMatrix4fv", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniformMatrix4fv },
  { "glProgramUniformMatrix4fvEXT", "GL_EXT_direct_state_access", glProgramUniformMatrix4fvEXT },
  { "glProgramUniformMatrix4x2dv", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniformMatrix4x2dv },
  { "glProgramUniformMatrix4x2dvEXT", "GL_EXT_direct_state_access", glProgramUniformMatrix4x2dvEXT },
  { "glProgramUniformMatrix4x2fv", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniformMatrix4x2fv },
  { "glProgramUniformMatrix4x2fvEXT", "GL_EXT_direct_state_access", glProgramUniformMatrix4x2fvEXT },
  { "glProgramUniformMatrix4x3dv", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniformMatrix4x3dv },
  { "glProgramUniformMatrix4x3dvEXT", "GL_EXT_direct_state_access", glProgramUniformMatrix4x3dvEXT },
  { "glProgramUniformMatrix4x3fv", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glProgramUniformMatrix4x3fv },
  { "glProgramUniformMatrix4x3fvEXT", "GL_EXT_direct_state_access", glProgramUniformMatrix4x3fvEXT },
  { "glProgramUniformui64NV", "GL_NV_shader_buffer_load", glProgramUniformui64NV },
  { "glProgramUniformui64vNV", "GL_NV_shader_buffer_load", glProgramUniformui64vNV },
  { "glProgramVertexLimitNV", "GL_NV_geometry_program4", glProgramVertexLimitNV },
  { "glProvokingVertex", "GL_ARB_provoking_vertex GL_VERSION_3_2", glProvokingVertex },
  { "glProvokingVertexEXT", "GL_EXT_provoking_vertex", glProvokingVertexEXT },
  { "glPushClientAttribDefaultEXT", "GL_EXT_direct_state_access", glPushClientAttribDefaultEXT },
  { "glPushDebugGroup", "GL_KHR_debug GL_VERSION_4_3", glPushDebugGroup },
  { "glPushGroupMarkerEXT", "GL_EXT_debug_marker", glPushGroupMarkerEXT },
  { "glQueryCounter", "GL_ARB_timer_query GL_VERSION_3_3", glQueryCounter },
  { "glQueryMatrixxOES", "GL_OES_query_matrix", glQueryMatrixxOES },
  { "glQueryObjectParameteruiAMD", "GL_AMD_occlusion_query_event", glQueryObjectParameteruiAMD },
  { "glQueryResourceNV", "GL_NV_query_resource", glQueryResourceNV },
  { "glQueryResourceTagNV", "GL_NV_query_resource_tag", glQueryResourceTagNV },
  { "glRasterPos2xOES", "GL_OES_fixed_point", glRasterPos2xOES },
  { "glRasterPos2xvOES", "GL_OES_fixed_point", glRasterPos2xvOES },
  { "glRasterPos3xOES", "GL_OES_fixed_point", glRasterPos3xOES },
  { "glRasterPos3xvOES", "GL_OES_fixed_point", glRasterPos3xvOES },
  { "glRasterPos4xOES", "GL_OES_fixed_point", glRasterPos4xOES },
  { "glRasterPos4xvOES", "GL_OES_fixed_point", glRasterPos4xvOES },
  { "glRasterSamplesEXT", "GL_EXT_raster_multisample GL_EXT_texture_filter_minmax GL_NV_framebuffer_mixed_samples", glRasterSamplesEXT },
  { "glReadBufferRegion", "GL_KTX_buffer_region", glReadBufferRegion },
  { "glReadInstrumentsSGIX", "GL_SGIX_instruments", glReadInstrumentsSGIX },
  { "glReadnPixels", "GL_KHR_robustness GL_VERSION_4_5", glReadnPixels },
  { "glReadnPixelsARB", "GL_ARB_robustness", glReadnPixelsARB },
  { "glRectxOES", "GL_OES_fixed_point", glRectxOES },
  { "glRectxvOES", "GL_OES_fixed_point", glRectxvOES },
  { "glReferencePlaneSGIX", "GL_SGIX_reference_plane", glReferencePlaneSGIX },
  { "glReleaseKeyedMutexWin32EXT", "GL_EXT_win32_keyed_mutex", glReleaseKeyedMutexWin32EXT },
  { "glReleaseShaderCompiler", "GL_ARB_ES2_compatibility GL_VERSION_4_1", glReleaseShaderCompiler },
  { "glRenderGpuMaskNV", "GL_NV_gpu_multicast", glRenderGpuMaskNV },
  { "glRenderbufferStorage", "GL_ARB_framebuffer_object GL_VERSION_3_0", glRenderbufferStorage },
  { "glRenderbufferStorageEXT", "GL_EXT_framebuffer_object", glRenderbufferStorageEXT },
  { "glRenderbufferStorageMultisample", "GL_ARB_framebuffer_object GL_VERSION_3_0", glRenderbufferStorageMultisample },
  { "glRenderbufferStorageMultisampleCoverageNV", "GL_NV_framebuffer_multisample_coverage", glRenderbufferStorageMultisampleCoverageNV },
  { "glRenderbufferStorageMultisampleEXT", "GL_EXT_framebuffer_multisample", glRenderbufferStorageMultisampleEXT },
  { "glReplacementCodePointerSUN", "GL_SUN_triangle_list", glReplacementCodePointerSUN },
  { "glReplacementCodeubSUN", "GL_SUN_triangle_list", glReplacementCodeubSUN },
  { "glReplacementCodeubvSUN", "GL_SUN_triangle_list", glReplacementCodeubvSUN },
  { "glReplacementCodeuiColor3fVertex3fSUN", "GL_SUN_vertex", glReplacementCodeuiColor3fVertex3fSUN },
  { "glReplacementCodeuiColor3fVertex3fvSUN", "GL_SUN_vertex", glReplacementCodeuiColor3fVertex3fvSUN },
  { "glReplacementCodeuiColor4fNormal3fVertex3fSUN", "GL_SUN_vertex", glReplacementCodeuiColor4fNormal3fVertex3fSUN },
  { "glReplacementCodeuiColor4fNormal3fVertex3fvSUN", "GL_SUN_vertex", glReplacementCodeuiColor4fNormal3fVertex3fvSUN },
  { "glReplacementCodeuiColor4ubVertex3fSUN", "GL_SUN_vertex", glReplacementCodeuiColor4ubVertex3fSUN },
  { "glReplacementCodeuiColor4ubVertex3fvSUN", "GL_SUN_vertex", glReplacementCodeuiColor4ubVertex3fvSUN },
  { "glReplacementCodeuiNormal3fVertex3fSUN", "GL_SUN_vertex", glReplacementCodeuiNormal3fVertex3fSUN },
  { "glReplacementCodeuiNormal3fVertex3fvSUN", "GL_SUN_vertex", glReplacementCodeuiNormal3fVertex3fvSUN },
  { "glReplacementCodeuiSUN", "GL_SUN_triangle_list", glReplacementCodeuiSUN },
  { "glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN", "GL_SUN_vertex", glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN },
  { "glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN", "GL_SUN_vertex", glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN },
  { "glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN", "GL_SUN_vertex", glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN },
  { "glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN", "GL_SUN_vertex", glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN },
  { "glReplacementCodeuiTexCoord2fVertex3fSUN", "GL_SUN_vertex", glReplacementCodeuiTexCoord2fVertex3fSUN },
  { "glReplacementCodeuiTexCoord2fVertex3fvSUN", "GL_SUN_vertex", glReplacementCodeuiTexCoord2fVertex3fvSUN },
  { "glReplacementCodeuiVertex3fSUN", "GL_SUN_vertex", glReplacementCodeuiVertex3fSUN },
  { "glReplacementCodeuiVertex3fvSUN", "GL_SUN_vertex", glReplacementCodeuiVertex3fvSUN },
  { "glReplacementCodeuivSUN", "GL_SUN_triangle_list", glReplacementCodeuivSUN },
  { "glReplacementCodeusSUN", "GL_SUN_triangle_list", glReplacementCodeusSUN },
  { "glReplacementCodeusvSUN", "GL_SUN_triangle_list", glReplacementCodeusvSUN },
  { "glRequestResidentProgramsNV", "GL_NV_vertex_program", glRequestResidentProgramsNV },
  { "glResetHistogram", "GL_ARB_imaging", glResetHistogram },
  { "glResetHistogramEXT", "GL_EXT_histogram", glResetHistogramEXT },
  { "glResetMinmax", "GL_ARB_imaging", glResetMinmax },
  { "glResetMinmaxEXT", "GL_EXT_histogram", glResetMinmaxEXT },
  { "glResizeBuffersMESA", "GL_MESA_resize_buffers", glResizeBuffersMESA },
  { "glResolveDepthValuesNV", "GL_NV_sample_locations", glResolveDepthValuesNV },
  { "glResumeTransformFeedback", "GL_ARB_transform_feedback2 GL_VERSION_4_0", glResumeTransformFeedback },
  { "glResumeTransformFeedbackNV", "GL_NV_transform_feedback2", glResumeTransformFeedbackNV },
  { "glRotatexOES", "GL_OES_fixed_point", glRotatexOES },
  { "glSampleCoverage", "GL_VERSION_1_3", glSampleCoverage },
  { "glSampleCoverageARB", "GL_ARB_multisample", glSampleCoverageARB },
  { "glSampleMapATI", "GL_ATI_fragment_shader", glSampleMapATI },
  { "glSampleMaskEXT", "GL_EXT_multisample", glSampleMaskEXT },
  { "glSampleMaskIndexedNV", "GL_NV_explicit_multisample", glSampleMaskIndexedNV },
  { "glSampleMaskSGIS", "GL_SGIS_multisample", glSampleMaskSGIS },
  { "glSampleMaski", "GL_ARB_texture_multisample GL_VERSION_3_2", glSampleMaski },
  { "glSamplePatternEXT", "GL_EXT_multisample", glSamplePatternEXT },
  { "glSamplePatternSGIS", "GL_SGIS_multisample", glSamplePatternSGIS },
  { "glSamplerParameterIiv", "GL_ARB_sampler_objects GL_VERSION_3_3", glSamplerParameterIiv },
  { "glSamplerParameterIuiv", "GL_ARB_sampler_objects GL_VERSION_3_3", glSamplerParameterIuiv },
  { "glSamplerParameterf", "GL_ARB_sampler_objects GL_VERSION_3_3", glSamplerParameterf },
  { "glSamplerParameterfv", "GL_ARB_sampler_objects GL_VERSION_3_3", glSamplerParameterfv },
  { "glSamplerParameteri", "GL_ARB_sampler_objects GL_VERSION_3_3", glSamplerParameteri },
  { "glSamplerParameteriv", "GL_ARB_sampler_objects GL_VERSION_3_3", glSamplerParameteriv },
  { "glScalexOES", "GL_OES_fixed_point", glScalexOES },
  { "glScissorArrayv", "GL_ARB_viewport_array GL_VERSION_4_1", glScissorArrayv },
  { "glScissorIndexed", "GL_ARB_viewport_array GL_VERSION_4_1", glScissorIndexed },
  { "glScissorIndexedv", "GL_ARB_viewport_array GL_VERSION_4_1", glScissorIndexedv },
  { "glSecondaryColor3b", "GL_VERSION_1_4", glSecondaryColor3b },
  { "glSecondaryColor3bEXT", "GL_EXT_secondary_color", glSecondaryColor3bEXT },
  { "glSecondaryColor3bv", "GL_VERSION_1_4", glSecondaryColor3bv },
  { "glSecondaryColor3bvEXT", "GL_EXT_secondary_color", glSecondaryColor3bvEXT },
  { "glSecondaryColor3d", "GL_VERSION_1_4", glSecondaryColor3d },
  { "glSecondaryColor3dEXT", "GL_EXT_secondary_color", glSecondaryColor3dEXT },
  { "glSecondaryColor3dv", "GL_VERSION_1_4", glSecondaryColor3dv },
  { "glSecondaryColor3dvEXT", "GL_EXT_secondary_color", glSecondaryColor3dvEXT },
  { "glSecondaryColor3f", "GL_VERSION_1_4", glSecondaryColor3f },
  { "glSecondaryColor3fEXT", "GL_EXT_secondary_color", glSecondaryColor3fEXT },
  { "glSecondaryColor3fv", "GL_VERSION_1_4", glSecondaryColor3fv },
  { "glSecondaryColor3fvEXT", "GL_EXT_secondary_color", glSecondaryColor3fvEXT },
  { "glSecondaryColor3hNV", "GL_NV_half_float", glSecondaryColor3hNV },
  { "glSecondaryColor3hvNV", "GL_NV_half_float", glSecondaryColor3hvNV },
  { "glSecondaryColor3i", "GL_VERSION_1_4", glSecondaryColor3i },
  { "glSecondaryColor3iEXT", "GL_EXT_secondary_color", glSecondaryColor3iEXT },
  { "glSecondaryColor3iv", "GL_VERSION_1_4", glSecondaryColor3iv },
  { "glSecondaryColor3ivEXT", "GL_EXT_secondary_color", glSecondaryColor3ivEXT },
  { "glSecondaryColor3s", "GL_VERSION_1_4", glSecondaryColor3s },
  { "glSecondaryColor3sEXT", "GL_EXT_secondary_color", glSecondaryColor3sEXT },
  { "glSecondaryColor3sv", "GL_VERSION_1_4", glSecondaryColor3sv },
  { "glSecondaryColor3svEXT", "GL_EXT_secondary_color", glSecondaryColor3svEXT },
  { "glSecondaryColor3ub", "GL_VERSION_1_4", glSecondaryColor3ub },
  { "glSecondaryColor3ubEXT", "GL_EXT_secondary_color", glSecondaryColor3ubEXT },
  { "glSecondaryColor3ubv", "GL_VERSION_1_4", glSecondaryColor3ubv },
  { "glSecondaryColor3ubvEXT", "GL_EXT_secondary_color", glSecondaryColor3ubvEXT },
  { "glSecondaryColor3ui", "GL_VERSION_1_4", glSecondaryColor3ui },
  { "glSecondaryColor3uiEXT", "GL_EXT_secondary_color", glSecondaryColor3uiEXT },
  { "glSecondaryColor3uiv", "GL_VERSION_1_4", glSecondaryColor3uiv },
  { "glSecondaryColor3uivEXT", "GL_EXT_secondary_color", glSecondaryColor3uivEXT },
  { "glSecondaryColor3us", "GL_VERSION_1_4", glSecondaryColor3us },
  { "glSecondaryColor3usEXT", "GL_EXT_secondary_color", glSecondaryColor3usEXT },
  { "glSecondaryColor3usv", "GL_VERSION_1_4", glSecondaryColor3usv },
  { "glSecondaryColor3usvEXT", "GL_EXT_secondary_color", glSecondaryColor3usvEXT },
  { "glSecondaryColorFormatNV", "GL_NV_vertex_buffer_unified_memory", glSecondaryColorFormatNV },
  { "glSecondaryColorP3ui", "GL_ARB_vertex_type_2_10_10_10_rev GL_VERSION_3_3", glSecondaryColorP3ui },
  { "glSecondaryColorP3uiv", "GL_ARB_vertex_type_2_10_10_10_rev GL_VERSION_3_3", glSecondaryColorP3uiv },
  { "glSecondaryColorPointer", "GL_VERSION_1_4", glSecondaryColorPointer },
  { "glSecondaryColorPointerEXT", "GL_EXT_secondary_color", glSecondaryColorPointerEXT },
  { "glSecondaryColorPointerListIBM", "GL_IBM_vertex_array_lists", glSecondaryColorPointerListIBM },
  { "glSelectPerfMonitorCountersAMD", "GL_AMD_performance_monitor", glSelectPerfMonitorCountersAMD },
  { "glSelectTextureCoordSetSGIS", "GL_SGIS_multitexture", glSelectTextureCoordSetSGIS },
  { "glSelectTextureSGIS", "GL_SGIS_multitexture", glSelectTextureSGIS },
  { "glSemaphoreParameterui64vEXT", "GL_EXT_semaphore", glSemaphoreParameterui64vEXT },
  { "glSeparableFilter2D", "GL_ARB_imaging", glSeparableFilter2D },
  { "glSeparableFilter2DEXT", "GL_EXT_convolution", glSeparableFilter2DEXT },
  { "glSetFenceAPPLE", "GL_APPLE_fence", glSetFenceAPPLE },
  { "glSetFenceNV", "GL_NV_fence", glSetFenceNV },
  { "glSetFragmentShaderConstantATI", "GL_ATI_fragment_shader", glSetFragmentShaderConstantATI },
  { "glSetInvariantEXT", "GL_EXT_vertex_shader", glSetInvariantEXT },
  { "glSetLocalConstantEXT", "GL_EXT_vertex_shader", glSetLocalConstantEXT },
  { "glSetMultisamplefvAMD", "GL_AMD_sample_positions", glSetMultisamplefvAMD },
  { "glShaderBinary", "GL_ARB_ES2_compatibility GL_VERSION_4_1", glShaderBinary },
  { "glShaderOp1EXT", "GL_EXT_vertex_shader", glShaderOp1EXT },
  { "glShaderOp2EXT", "GL_EXT_vertex_shader", glShaderOp2EXT },
  { "glShaderOp3EXT", "GL_EXT_vertex_shader", glShaderOp3EXT },
  { "glShaderSource", "GL_VERSION_2_0", glShaderSource },
  { "glShaderSourceARB", "GL_ARB_shader_objects", glShaderSourceARB },
  { "glShaderStorageBlockBinding", "GL_ARB_shader_storage_buffer_object GL_VERSION_4_3", glShaderStorageBlockBinding },
  { "glSharpenTexFuncSGIS", "GL_SGIS_sharpen_texture", glSharpenTexFuncSGIS },
  { "glSignalSemaphoreEXT", "GL_EXT_semaphore", glSignalSemaphoreEXT },
  { "glSignalVkFenceNV", "GL_NV_draw_vulkan_image", glSignalVkFenceNV },
  { "glSignalVkSemaphoreNV", "GL_NV_draw_vulkan_image", glSignalVkSemaphoreNV },
  { "glSpecializeShader", "GL_VERSION_4_6", glSpecializeShader },
  { "glSpecializeShaderARB", "GL_ARB_gl_spirv", glSpecializeShaderARB },
  { "glSpriteParameterfSGIX", "GL_SGIX_sprite", glSpriteParameterfSGIX },
  { "glSpriteParameterfvSGIX", "GL_SGIX_sprite", glSpriteParameterfvSGIX },
  { "glSpriteParameteriSGIX", "GL_SGIX_sprite", glSpriteParameteriSGIX },
  { "glSpriteParameterivSGIX", "GL_SGIX_sprite", glSpriteParameterivSGIX },
  { "glStartInstrumentsSGIX", "GL_SGIX_instruments", glStartInstrumentsSGIX },
  { "glStateCaptureNV", "GL_NV_command_list", glStateCaptureNV },
  { "glStencilClearTagEXT", "GL_EXT_stencil_clear_tag", glStencilClearTagEXT },
  { "glStencilFillPathInstancedNV", "GL_NV_path_rendering", glStencilFillPathInstancedNV },
  { "glStencilFillPathNV", "GL_NV_path_rendering", glStencilFillPathNV },
  { "glStencilFuncSeparate", "GL_VERSION_2_0", glStencilFuncSeparate },
  { "glStencilFuncSeparateATI", "GL_ATI_separate_stencil", glStencilFuncSeparateATI },
  { "glStencilMaskSeparate", "GL_VERSION_2_0", glStencilMaskSeparate },
  { "glStencilOpSeparate", "GL_VERSION_2_0", glStencilOpSeparate },
  { "glStencilOpSeparateATI", "GL_ATI_separate_stencil", glStencilOpSeparateATI },
  { "glStencilOpValueAMD", "GL_AMD_stencil_operation_extended", glStencilOpValueAMD },
  { "glStencilStrokePathInstancedNV", "GL_NV_path_rendering", glStencilStrokePathInstancedNV },
  { "glStencilStrokePathNV", "GL_NV_path_rendering", glStencilStrokePathNV },
  { "glStencilThenCoverFillPathInstancedNV", "GL_NV_path_rendering", glStencilThenCoverFillPathInstancedNV },
  { "glStencilThenCoverFillPathNV", "GL_NV_path_rendering", glStencilThenCoverFillPathNV },
  { "glStencilThenCoverStrokePathInstancedNV", "GL_NV_path_rendering", glStencilThenCoverStrokePathInstancedNV },
  { "glStencilThenCoverStrokePathNV", "GL_NV_path_rendering", glStencilThenCoverStrokePathNV },
  { "glStopInstrumentsSGIX", "GL_SGIX_instruments", glStopInstrumentsSGIX },
  { "glStringMarkerGREMEDY", "GL_GREMEDY_string_marker", glStringMarkerGREMEDY },
  { "glSubpixelPrecisionBiasNV", "GL_NV_conservative_raster", glSubpixelPrecisionBiasNV },
  { "glSwizzleEXT", "GL_EXT_vertex_shader", glSwizzleEXT },
  { "glSyncTextureINTEL", "GL_INTEL_map_texture", glSyncTextureINTEL },
  { "glTagSampleBufferSGIX", "GL_SGIX_tag_sample_buffer", glTagSampleBufferSGIX },
  { "glTangent3bEXT", "GL_EXT_coordinate_frame", glTangent3bEXT },
  { "glTangent3bvEXT", "GL_EXT_coordinate_frame", glTangent3bvEXT },
  { "glTangent3dEXT", "GL_EXT_coordinate_frame", glTangent3dEXT },
  { "glTangent3dvEXT", "GL_EXT_coordinate_frame", glTangent3dvEXT },
  { "glTangent3fEXT", "GL_EXT_coordinate_frame", glTangent3fEXT },
  { "glTangent3fvEXT", "GL_EXT_coordinate_frame", glTangent3fvEXT },
  { "glTangent3iEXT", "GL_EXT_coordinate_frame", glTangent3iEXT },
  { "glTangent3ivEXT", "GL_EXT_coordinate_frame", glTangent3ivEXT },
  { "glTangent3sEXT", "GL_EXT_coordinate_frame", glTangent3sEXT },
  { "glTangent3svEXT", "GL_EXT_coordinate_frame", glTangent3svEXT },
  { "glTangentPointerEXT", "GL_EXT_coordinate_frame", glTangentPointerEXT },
  { "glTbufferMask3DFX", "GL_3DFX_tbuffer", glTbufferMask3DFX },
  { "glTessellationFactorAMD", "GL_AMD_vertex_shader_tessellator", glTessellationFactorAMD },
  { "glTessellationModeAMD", "GL_AMD_vertex_shader_tessellator", glTessellationModeAMD },
  { "glTestFenceAPPLE", "GL_APPLE_fence", glTestFenceAPPLE },
  { "glTestFenceNV", "GL_NV_fence", glTestFenceNV },
  { "glTestObjectAPPLE", "GL_APPLE_fence", glTestObjectAPPLE },
  { "glTexBuffer", "GL_VERSION_3_1", glTexBuffer },
  { "glTexBufferARB", "GL_ARB_texture_buffer_object", glTexBufferARB },
  { "glTexBufferEXT", "GL_EXT_texture_buffer_object", glTexBufferEXT },
  { "glTexBufferRange", "GL_ARB_texture_buffer_range GL_VERSION_4_3", glTexBufferRange },
  { "glTexBumpParameterfvATI", "GL_ATI_envmap_bumpmap", glTexBumpParameterfvATI },
  { "glTexBumpParameterivATI", "GL_ATI_envmap_bumpmap", glTexBumpParameterivATI },
  { "glTexCoord1bOES", "GL_OES_byte_coordinates", glTexCoord1bOES },
  { "glTexCoord1bvOES", "GL_OES_byte_coordinates", glTexCoord1bvOES },
  { "glTexCoord1hNV", "GL_NV_half_float", glTexCoord1hNV },
  { "glTexCoord1hvNV", "GL_NV_half_float", glTexCoord1hvNV },
  { "glTexCoord1xOES", "GL_OES_fixed_point", glTexCoord1xOES },
  { "glTexCoord1xvOES", "GL_OES_fixed_point", glTexCoord1xvOES },
  { "glTexCoord2bOES", "GL_OES_byte_coordinates", glTexCoord2bOES },
  { "glTexCoord2bvOES", "GL_OES_byte_coordinates", glTexCoord2bvOES },
  { "glTexCoord2fColor3fVertex3fSUN", "GL_SUN_vertex", glTexCoord2fColor3fVertex3fSUN },
  { "glTexCoord2fColor3fVertex3fvSUN", "GL_SUN_vertex", glTexCoord2fColor3fVertex3fvSUN },
  { "glTexCoord2fColor4fNormal3fVertex3fSUN", "GL_SUN_vertex", glTexCoord2fColor4fNormal3fVertex3fSUN },
  { "glTexCoord2fColor4fNormal3fVertex3fvSUN", "GL_SUN_vertex", glTexCoord2fColor4fNormal3fVertex3fvSUN },
  { "glTexCoord2fColor4ubVertex3fSUN", "GL_SUN_vertex", glTexCoord2fColor4ubVertex3fSUN },
  { "glTexCoord2fColor4ubVertex3fvSUN", "GL_SUN_vertex", glTexCoord2fColor4ubVertex3fvSUN },
  { "glTexCoord2fNormal3fVertex3fSUN", "GL_SUN_vertex", glTexCoord2fNormal3fVertex3fSUN },
  { "glTexCoord2fNormal3fVertex3fvSUN", "GL_SUN_vertex", glTexCoord2fNormal3fVertex3fvSUN },
  { "glTexCoord2fVertex3fSUN", "GL_SUN_vertex", glTexCoord2fVertex3fSUN },
  { "glTexCoord2fVertex3fvSUN", "GL_SUN_vertex", glTexCoord2fVertex3fvSUN },
  { "glTexCoord2hNV", "GL_NV_half_float", glTexCoord2hNV },
  { "glTexCoord2hvNV", "GL_NV_half_float", glTexCoord2hvNV },
  { "glTexCoord2xOES", "GL_OES_fixed_point", glTexCoord2xOES },
  { "glTexCoord2xvOES", "GL_OES_fixed_point", glTexCoord2xvOES },
  { "glTexCoord3bOES", "GL_OES_byte_coordinates", glTexCoord3bOES },
  { "glTexCoord3bvOES", "GL_OES_byte_coordinates", glTexCoord3bvOES },
  { "glTexCoord3hNV", "GL_NV_half_float", glTexCoord3hNV },
  { "glTexCoord3hvNV", "GL_NV_half_float", glTexCoord3hvNV },
  { "glTexCoord3xOES", "GL_OES_fixed_point", glTexCoord3xOES },
  { "glTexCoord3xvOES", "GL_OES_fixed_point", glTexCoord3xvOES },
  { "glTexCoord4bOES", "GL_OES_byte_coordinates", glTexCoord4bOES },
  { "glTexCoord4bvOES", "GL_OES_byte_coordinates", glTexCoord4bvOES },
  { "glTexCoord4fColor4fNormal3fVertex4fSUN", "GL_SUN_vertex", glTexCoord4fColor4fNormal3fVertex4fSUN },
  { "glTexCoord4fColor4fNormal3fVertex4fvSUN", "GL_SUN_vertex", glTexCoord4fColor4fNormal3fVertex4fvSUN },
  { "glTexCoord4fVertex4fSUN", "GL_SUN_vertex", glTexCoord4fVertex4fSUN },
  { "glTexCoord4fVertex4fvSUN", "GL_SUN_vertex", glTexCoord4fVertex4fvSUN },
  { "glTexCoord4hNV", "GL_NV_half_float", glTexCoord4hNV },
  { "glTexCoord4hvNV", "GL_NV_half_float", glTexCoord4hvNV },
  { "glTexCoord4xOES", "GL_OES_fixed_point", glTexCoord4xOES },
  { "glTexCoord4xvOES", "GL_OES_fixed_point", glTexCoord4xvOES },
  { "glTexCoordFormatNV", "GL_NV_vertex_buffer_unified_memory", glTexCoordFormatNV },
  { "glTexCoordP1ui", "GL_ARB_vertex_type_2_10_10_10_rev GL_VERSION_3_3", glTexCoordP1ui },
  { "glTexCoordP1uiv", "GL_ARB_vertex_type_2_10_10_10_rev GL_VERSION_3_3", glTexCoordP1uiv },
  { "glTexCoordP2ui", "GL_ARB_vertex_type_2_10_10_10_rev GL_VERSION_3_3", glTexCoordP2ui },
  { "glTexCoordP2uiv", "GL_ARB_vertex_type_2_10_10_10_rev GL_VERSION_3_3", glTexCoordP2uiv },
  { "glTexCoordP3ui", "GL_ARB_vertex_type_2_10_10_10_rev GL_VERSION_3_3", glTexCoordP3ui },
  { "glTexCoordP3uiv", "GL_ARB_vertex_type_2_10_10_10_rev GL_VERSION_3_3", glTexCoordP3uiv },
  { "glTexCoordP4ui", "GL_ARB_vertex_type_2_10_10_10_rev GL_VERSION_3_3", glTexCoordP4ui },
  { "glTexCoordP4uiv", "GL_ARB_vertex_type_2_10_10_10_rev GL_VERSION_3_3", glTexCoordP4uiv },
  { "glTexCoordPointerEXT", "GL_EXT_vertex_array", glTexCoordPointerEXT },
  { "glTexCoordPointerListIBM", "GL_IBM_vertex_array_lists", glTexCoordPointerListIBM },
  { "glTexCoordPointervINTEL", "GL_INTEL_parallel_arrays", glTexCoordPointervINTEL },
  { "glTexEnvxOES", "GL_OES_fixed_point", glTexEnvxOES },
  { "glTexEnvxvOES", "GL_OES_fixed_point", glTexEnvxvOES },
  { "glTexFilterFuncSGIS", "GL_SGIS_texture_filter4", glTexFilterFuncSGIS },
  { "glTexGenxOES", "GL_OES_fixed_point", glTexGenxOES },
  { "glTexGenxvOES", "GL_OES_fixed_point", glTexGenxvOES },
  { "glTexImage2DMultisample", "GL_ARB_texture_multisample GL_VERSION_3_2", glTexImage2DMultisample },
  { "glTexImage2DMultisampleCoverageNV", "GL_NV_texture_multisample", glTexImage2DMultisampleCoverageNV },
  { "glTexImage3D", "GL_VERSION_1_2", glTexImage3D },
  { "glTexImage3DEXT", "GL_EXT_texture3D", glTexImage3DEXT },
  { "glTexImage3DMultisample", "GL_ARB_texture_multisample GL_VERSION_3_2", glTexImage3DMultisample },
  { "glTexImage3DMultisampleCoverageNV", "GL_NV_texture_multisample", glTexImage3DMultisampleCoverageNV },
  { "glTexImage4DSGIS", "GL_SGIS_texture4D", glTexImage4DSGIS },
  { "glTexPageCommitmentARB", "GL_ARB_sparse_texture", glTexPageCommitmentARB },
  { "glTexParameterIiv", "GL_VERSION_3_0", glTexParameterIiv },
  { "glTexParameterIivEXT", "GL_EXT_texture_integer", glTexParameterIivEXT },
  { "glTexParameterIuiv", "GL_VERSION_3_0", glTexParameterIuiv },
  { "glTexParameterIuivEXT", "GL_EXT_texture_integer", glTexParameterIuivEXT },
  { "glTexParameterxOES", "GL_OES_fixed_point", glTexParameterxOES },
  { "glTexParameterxvOES", "GL_OES_fixed_point", glTexParameterxvOES },
  { "glTexRenderbufferNV", "GL_NV_explicit_multisample", glTexRenderbufferNV },
  { "glTexStorage1D", "GL_ARB_texture_storage GL_VERSION_4_2", glTexStorage1D },
  { "glTexStorage2D", "GL_ARB_texture_storage GL_VERSION_4_2", glTexStorage2D },
  { "glTexStorage2DMultisample", "GL_ARB_texture_storage_multisample GL_VERSION_4_3", glTexStorage2DMultisample },
  { "glTexStorage3D", "GL_ARB_texture_storage GL_VERSION_4_2", glTexStorage3D },
  { "glTexStorage3DMultisample", "GL_ARB_texture_storage_multisample GL_VERSION_4_3", glTexStorage3DMultisample },
  { "glTexStorageMem1DEXT", "GL_EXT_memory_object", glTexStorageMem1DEXT },
  { "glTexStorageMem2DEXT", "GL_EXT_memory_object", glTexStorageMem2DEXT },
  { "glTexStorageMem2DMultisampleEXT", "GL_EXT_memory_object", glTexStorageMem2DMultisampleEXT },
  { "glTexStorageMem3DEXT", "GL_EXT_memory_object", glTexStorageMem3DEXT },
  { "glTexStorageMem3DMultisampleEXT", "GL_EXT_memory_object", glTexStorageMem3DMultisampleEXT },
  { "glTexStorageSparseAMD", "GL_AMD_sparse_texture", glTexStorageSparseAMD },
  { "glTexSubImage1DEXT", "GL_EXT_subtexture", glTexSubImage1DEXT },
  { "glTexSubImage2DEXT", "GL_EXT_subtexture", glTexSubImage2DEXT },
  { "glTexSubImage3D", "GL_VERSION_1_2", glTexSubImage3D },
  { "glTexSubImage3DEXT", "GL_EXT_texture3D", glTexSubImage3DEXT },
  { "glTexSubImage4DSGIS", "GL_SGIS_texture4D", glTexSubImage4DSGIS },
  { "glTextureBarrier", "GL_ARB_texture_barrier GL_VERSION_4_5", glTextureBarrier },
  { "glTextureBarrierNV", "GL_NV_texture_barrier", glTextureBarrierNV },
  { "glTextureBuffer", "GL_ARB_direct_state_access GL_VERSION_4_5", glTextureBuffer },
  { "glTextureBufferEXT", "GL_EXT_direct_state_access", glTextureBufferEXT },
  { "glTextureBufferRange", "GL_ARB_direct_state_access GL_VERSION_4_5", glTextureBufferRange },
  { "glTextureBufferRangeEXT", "GL_EXT_direct_state_access", glTextureBufferRangeEXT },
  { "glTextureColorMaskSGIS", "GL_SGIS_texture_color_mask", glTextureColorMaskSGIS },
  { "glTextureImage1DEXT", "GL_EXT_direct_state_access", glTextureImage1DEXT },
  { "glTextureImage2DEXT", "GL_EXT_direct_state_access", glTextureImage2DEXT },
  { "glTextureImage2DMultisampleCoverageNV", "GL_NV_texture_multisample", glTextureImage2DMultisampleCoverageNV },
  { "glTextureImage2DMultisampleNV", "GL_NV_texture_multisample", glTextureImage2DMultisampleNV },
  { "glTextureImage3DEXT", "GL_EXT_direct_state_access", glTextureImage3DEXT },
  { "glTextureImage3DMultisampleCoverageNV", "GL_NV_texture_multisample", glTextureImage3DMultisampleCoverageNV },
  { "glTextureImage3DMultisampleNV", "GL_NV_texture_multisample", glTextureImage3DMultisampleNV },
  { "glTextureLightEXT", "GL_EXT_light_texture", glTextureLightEXT },
  { "glTextureMaterialEXT", "GL_EXT_light_texture", glTextureMaterialEXT },
  { "glTextureNormalEXT", "GL_EXT_texture_perturb_normal", glTextureNormalEXT },
  { "glTexturePageCommitmentEXT", "GL_EXT_direct_state_access", glTexturePageCommitmentEXT },
  { "glTextureParameterIiv", "GL_ARB_direct_state_access GL_VERSION_4_5", glTextureParameterIiv },
  { "glTextureParameterIivEXT", "GL_EXT_direct_state_access", glTextureParameterIivEXT },
  { "glTextureParameterIuiv", "GL_ARB_direct_state_access GL_VERSION_4_5", glTextureParameterIuiv },
  { "glTextureParameterIuivEXT", "GL_EXT_direct_state_access", glTextureParameterIuivEXT },
  { "glTextureParameterf", "GL_ARB_direct_state_access GL_VERSION_4_5", glTextureParameterf },
  { "glTextureParameterfEXT", "GL_EXT_direct_state_access", glTextureParameterfEXT },
  { "glTextureParameterfv", "GL_ARB_direct_state_access GL_VERSION_4_5", glTextureParameterfv },
  { "glTextureParameterfvEXT", "GL_EXT_direct_state_access", glTextureParameterfvEXT },
  { "glTextureParameteri", "GL_ARB_direct_state_access GL_VERSION_4_5", glTextureParameteri },
  { "glTextureParameteriEXT", "GL_EXT_direct_state_access", glTextureParameteriEXT },
  { "glTextureParameteriv", "GL_ARB_direct_state_access GL_VERSION_4_5", glTextureParameteriv },
  { "glTextureParameterivEXT", "GL_EXT_direct_state_access", glTextureParameterivEXT },
  { "glTextureRangeAPPLE", "GL_APPLE_texture_range", glTextureRangeAPPLE },
  { "glTextureRenderbufferEXT", "GL_EXT_direct_state_access", glTextureRenderbufferEXT },
  { "glTextureStorage1D", "GL_ARB_direct_state_access GL_VERSION_4_5", glTextureStorage1D },
  { "glTextureStorage1DEXT", "GL_EXT_direct_state_access", glTextureStorage1DEXT },
  { "glTextureStorage2D", "GL_ARB_direct_state_access GL_VERSION_4_5", glTextureStorage2D },
  { "glTextureStorage2DEXT", "GL_EXT_direct_state_access", glTextureStorage2DEXT },
  { "glTextureStorage2DMultisample", "GL_ARB_direct_state_access GL_VERSION_4_5", glTextureStorage2DMultisample },
  { "glTextureStorage2DMultisampleEXT", "GL_EXT_direct_state_access", glTextureStorage2DMultisampleEXT },
  { "glTextureStorage3D", "GL_ARB_direct_state_access GL_VERSION_4_5", glTextureStorage3D },
  { "glTextureStorage3DEXT", "GL_EXT_direct_state_access", glTextureStorage3DEXT },
  { "glTextureStorage3DMultisample", "GL_ARB_direct_state_access GL_VERSION_4_5", glTextureStorage3DMultisample },
  { "glTextureStorage3DMultisampleEXT", "GL_EXT_direct_state_access", glTextureStorage3DMultisampleEXT },
  { "glTextureStorageMem1DEXT", "GL_EXT_memory_object", glTextureStorageMem1DEXT },
  { "glTextureStorageMem2DEXT", "GL_EXT_memory_object", glTextureStorageMem2DEXT },
  { "glTextureStorageMem2DMultisampleEXT", "GL_EXT_memory_object", glTextureStorageMem2DMultisampleEXT },
  { "glTextureStorageMem3DEXT", "GL_EXT_memory_object", glTextureStorageMem3DEXT },
  { "glTextureStorageMem3DMultisampleEXT", "GL_EXT_memory_object", glTextureStorageMem3DMultisampleEXT },
  { "glTextureStorageSparseAMD", "GL_AMD_sparse_texture", glTextureStorageSparseAMD },
  { "glTextureSubImage1D", "GL_ARB_direct_state_access GL_VERSION_4_5", glTextureSubImage1D },
  { "glTextureSubImage1DEXT", "GL_EXT_direct_state_access", glTextureSubImage1DEXT },
  { "glTextureSubImage2D", "GL_ARB_direct_state_access GL_VERSION_4_5", glTextureSubImage2D },
  { "glTextureSubImage2DEXT", "GL_EXT_direct_state_access", glTextureSubImage2DEXT },
  { "glTextureSubImage3D", "GL_ARB_direct_state_access GL_VERSION_4_5", glTextureSubImage3D },
  { "glTextureSubImage3DEXT", "GL_EXT_direct_state_access", glTextureSubImage3DEXT },
  { "glTextureView", "GL_ARB_texture_view GL_VERSION_4_3", glTextureView },
  { "glTrackMatrixNV", "GL_NV_vertex_program", glTrackMatrixNV },
  { "glTransformFeedbackAttribsNV", "GL_NV_transform_feedback", glTransformFeedbackAttribsNV },
  { "glTransformFeedbackBufferBase", "GL_ARB_direct_state_access GL_VERSION_4_5", glTransformFeedbackBufferBase },
  { "glTransformFeedbackBufferRange", "GL_ARB_direct_state_access GL_VERSION_4_5", glTransformFeedbackBufferRange },
  { "glTransformFeedbackStreamAttribsNV", "GL_NV_transform_feedback", glTransformFeedbackStreamAttribsNV },
  { "glTransformFeedbackVaryings", "GL_VERSION_3_0", glTransformFeedbackVaryings },
  { "glTransformFeedbackVaryingsEXT", "GL_EXT_transform_feedback", glTransformFeedbackVaryingsEXT },
  { "glTransformFeedbackVaryingsNV", "GL_NV_transform_feedback", glTransformFeedbackVaryingsNV },
  { "glTransformPathNV", "GL_NV_path_rendering", glTransformPathNV },
  { "glTranslatexOES", "GL_OES_fixed_point", glTranslatexOES },
  { "glUniform1d", "GL_ARB_gpu_shader_fp64 GL_VERSION_4_0", glUniform1d },
  { "glUniform1dv", "GL_ARB_gpu_shader_fp64 GL_VERSION_4_0", glUniform1dv },
  { "glUniform1f", "GL_VERSION_2_0", glUniform1f },
  { "glUniform1fARB", "GL_ARB_shader_objects", glUniform1fARB },
  { "glUniform1fv", "GL_VERSION_2_0", glUniform1fv },
  { "glUniform1fvARB", "GL_ARB_shader_objects", glUniform1fvARB },
  { "glUniform1i", "GL_VERSION_2_0", glUniform1i },
  { "glUniform1i64ARB", "GL_ARB_gpu_shader_int64", glUniform1i64ARB },
  { "glUniform1i64NV", "GL_AMD_gpu_shader_int64 GL_NV_gpu_shader5", glUniform1i64NV },
  { "glUniform1i64vARB", "GL_ARB_gpu_shader_int64", glUniform1i64vARB },
  { "glUniform1i64vNV", "GL_AMD_gpu_shader_int64 GL_NV_gpu_shader5", glUniform1i64vNV },
  { "glUniform1iARB", "GL_ARB_shader_objects", glUniform1iARB },
  { "glUniform1iv", "GL_VERSION_2_0", glUniform1iv },
  { "glUniform1ivARB", "GL_ARB_shader_objects", glUniform1ivARB },
  { "glUniform1ui", "GL_VERSION_3_0", glUniform1ui },
  { "glUniform1ui64ARB", "GL_ARB_gpu_shader_int64", glUniform1ui64ARB },
  { "glUniform1ui64NV", "GL_AMD_gpu_shader_int64 GL_NV_gpu_shader5", glUniform1ui64NV },
  { "glUniform1ui64vARB", "GL_ARB_gpu_shader_int64", glUniform1ui64vARB },
  { "glUniform1ui64vNV", "GL_AMD_gpu_shader_int64 GL_NV_gpu_shader5", glUniform1ui64vNV },
  { "glUniform1uiEXT", "GL_EXT_gpu_shader4", glUniform1uiEXT },
  { "glUniform1uiv", "GL_VERSION_3_0", glUniform1uiv },
  { "glUniform1uivEXT", "GL_EXT_gpu_shader4", glUniform1uivEXT },
  { "glUniform2d", "GL_ARB_gpu_shader_fp64 GL_VERSION_4_0", glUniform2d },
  { "glUniform2dv", "GL_ARB_gpu_shader_fp64 GL_VERSION_4_0", glUniform2dv },
  { "glUniform2f", "GL_VERSION_2_0", glUniform2f },
  { "glUniform2fARB", "GL_ARB_shader_objects", glUniform2fARB },
  { "glUniform2fv", "GL_VERSION_2_0", glUniform2fv },
  { "glUniform2fvARB", "GL_ARB_shader_objects", glUniform2fvARB },
  { "glUniform2i", "GL_VERSION_2_0", glUniform2i },
  { "glUniform2i64ARB", "GL_ARB_gpu_shader_int64", glUniform2i64ARB },
  { "glUniform2i64NV", "GL_AMD_gpu_shader_int64 GL_NV_gpu_shader5", glUniform2i64NV },
  { "glUniform2i64vARB", "GL_ARB_gpu_shader_int64", glUniform2i64vARB },
  { "glUniform2i64vNV", "GL_AMD_gpu_shader_int64 GL_NV_gpu_shader5", glUniform2i64vNV },
  { "glUniform2iARB", "GL_ARB_shader_objects", glUniform2iARB },
  { "glUniform2iv", "GL_VERSION_2_0", glUniform2iv },
  { "glUniform2ivARB", "GL_ARB_shader_objects", glUniform2ivARB },
  { "glUniform2ui", "GL_VERSION_3_0", glUniform2ui },
  { "glUniform2ui64ARB", "GL_ARB_gpu_shader_int64", glUniform2ui64ARB },
  { "glUniform2ui64NV", "GL_AMD_gpu_shader_int64 GL_NV_gpu_shader5", glUniform2ui64NV },
  { "glUniform2ui64vARB", "GL_ARB_gpu_shader_int64", glUniform2ui64vARB },
  { "glUniform2ui64vNV", "GL_AMD_gpu_shader_int64 GL_NV_gpu_shader5", glUniform2ui64vNV },
  { "glUniform2uiEXT", "GL_EXT_gpu_shader4", glUniform2uiEXT },
  { "glUniform2uiv", "GL_VERSION_3_0", glUniform2uiv },
  { "glUniform2uivEXT", "GL_EXT_gpu_shader4", glUniform2uivEXT },
  { "glUniform3d", "GL_ARB_gpu_shader_fp64 GL_VERSION_4_0", glUniform3d },
  { "glUniform3dv", "GL_ARB_gpu_shader_fp64 GL_VERSION_4_0", glUniform3dv },
  { "glUniform3f", "GL_VERSION_2_0", glUniform3f },
  { "glUniform3fARB", "GL_ARB_shader_objects", glUniform3fARB },
  { "glUniform3fv", "GL_VERSION_2_0", glUniform3fv },
  { "glUniform3fvARB", "GL_ARB_shader_objects", glUniform3fvARB },
  { "glUniform3i", "GL_VERSION_2_0", glUniform3i },
  { "glUniform3i64ARB", "GL_ARB_gpu_shader_int64", glUniform3i64ARB },
  { "glUniform3i64NV", "GL_AMD_gpu_shader_int64 GL_NV_gpu_shader5", glUniform3i64NV },
  { "glUniform3i64vARB", "GL_ARB_gpu_shader_int64", glUniform3i64vARB },
  { "glUniform3i64vNV", "GL_AMD_gpu_shader_int64 GL_NV_gpu_shader5", glUniform3i64vNV },
  { "glUniform3iARB", "GL_ARB_shader_objects", glUniform3iARB },
  { "glUniform3iv", "GL_VERSION_2_0", glUniform3iv },
  { "glUniform3ivARB", "GL_ARB_shader_objects", glUniform3ivARB },
  { "glUniform3ui", "GL_VERSION_3_0", glUniform3ui },
  { "glUniform3ui64ARB", "GL_ARB_gpu_shader_int64", glUniform3ui64ARB },
  { "glUniform3ui64NV", "GL_AMD_gpu_shader_int64 GL_NV_gpu_shader5", glUniform3ui64NV },
  { "glUniform3ui64vARB", "GL_ARB_gpu_shader_int64", glUniform3ui64vARB },
  { "glUniform3ui64vNV", "GL_AMD_gpu_shader_int64 GL_NV_gpu_shader5", glUniform3ui64vNV },
  { "glUniform3uiEXT", "GL_EXT_gpu_shader4", glUniform3uiEXT },
  { "glUniform3uiv", "GL_VERSION_3_0", glUniform3uiv },
  { "glUniform3uivEXT", "GL_EXT_gpu_shader4", glUniform3uivEXT },
  { "glUniform4d", "GL_ARB_gpu_shader_fp64 GL_VERSION_4_0", glUniform4d },
  { "glUniform4dv", "GL_ARB_gpu_shader_fp64 GL_VERSION_4_0", glUniform4dv },
  { "glUniform4f", "GL_VERSION_2_0", glUniform4f },
  { "glUniform4fARB", "GL_ARB_shader_objects", glUniform4fARB },
  { "glUniform4fv", "GL_VERSION_2_0", glUniform4fv },
  { "glUniform4fvARB", "GL_ARB_shader_objects", glUniform4fvARB },
  { "glUniform4i", "GL_VERSION_2_0", glUniform4i },
  { "glUniform4i64ARB", "GL_ARB_gpu_shader_int64", glUniform4i64ARB },
  { "glUniform4i64NV", "GL_AMD_gpu_shader_int64 GL_NV_gpu_shader5", glUniform4i64NV },
  { "glUniform4i64vARB", "GL_ARB_gpu_shader_int64", glUniform4i64vARB },
  { "glUniform4i64vNV", "GL_AMD_gpu_shader_int64 GL_NV_gpu_shader5", glUniform4i64vNV },
  { "glUniform4iARB", "GL_ARB_shader_objects", glUniform4iARB },
  { "glUniform4iv", "GL_VERSION_2_0", glUniform4iv },
  { "glUniform4ivARB", "GL_ARB_shader_objects", glUniform4ivARB },
  { "glUniform4ui", "GL_VERSION_3_0", glUniform4ui },
  { "glUniform4ui64ARB", "GL_ARB_gpu_shader_int64", glUniform4ui64ARB },
  { "glUniform4ui64NV", "GL_AMD_gpu_shader_int64 GL_NV_gpu_shader5", glUniform4ui64NV },
  { "glUniform4ui64vARB", "GL_ARB_gpu_shader_int64", glUniform4ui64vARB },
  { "glUniform4ui64vNV", "GL_AMD_gpu_shader_int64 GL_NV_gpu_shader5", glUniform4ui64vNV },
  { "glUniform4uiEXT", "GL_EXT_gpu_shader4", glUniform4uiEXT },
  { "glUniform4uiv", "GL_VERSION_3_0", glUniform4uiv },
  { "glUniform4uivEXT", "GL_EXT_gpu_shader4", glUniform4uivEXT },
  { "glUniformBlockBinding", "GL_ARB_uniform_buffer_object GL_VERSION_3_1", glUniformBlockBinding },
  { "glUniformBufferEXT", "GL_EXT_bindable_uniform", glUniformBufferEXT },
  { "glUniformHandleui64ARB", "GL_ARB_bindless_texture", glUniformHandleui64ARB },
  { "glUniformHandleui64NV", "GL_NV_bindless_texture", glUniformHandleui64NV },
  { "glUniformHandleui64vARB", "GL_ARB_bindless_texture", glUniformHandleui64vARB },
  { "glUniformHandleui64vNV", "GL_NV_bindless_texture", glUniformHandleui64vNV },
  { "glUniformMatrix2dv", "GL_ARB_gpu_shader_fp64 GL_VERSION_4_0", glUniformMatrix2dv },
  { "glUniformMatrix2fv", "GL_VERSION_2_0", glUniformMatrix2fv },
  { "glUniformMatrix2fvARB", "GL_ARB_shader_objects", glUniformMatrix2fvARB },
  { "glUniformMatrix2x3dv", "GL_ARB_gpu_shader_fp64 GL_VERSION_4_0", glUniformMatrix2x3dv },
  { "glUniformMatrix2x3fv", "GL_VERSION_2_1", glUniformMatrix2x3fv },
  { "glUniformMatrix2x4dv", "GL_ARB_gpu_shader_fp64 GL_VERSION_4_0", glUniformMatrix2x4dv },
  { "glUniformMatrix2x4fv", "GL_VERSION_2_1", glUniformMatrix2x4fv },
  { "glUniformMatrix3dv", "GL_ARB_gpu_shader_fp64 GL_VERSION_4_0", glUniformMatrix3dv },
  { "glUniformMatrix3fv", "GL_VERSION_2_0", glUniformMatrix3fv },
  { "glUniformMatrix3fvARB", "GL_ARB_shader_objects", glUniformMatrix3fvARB },
  { "glUniformMatrix3x2dv", "GL_ARB_gpu_shader_fp64 GL_VERSION_4_0", glUniformMatrix3x2dv },
  { "glUniformMatrix3x2fv", "GL_VERSION_2_1", glUniformMatrix3x2fv },
  { "glUniformMatrix3x4dv", "GL_ARB_gpu_shader_fp64 GL_VERSION_4_0", glUniformMatrix3x4dv },
  { "glUniformMatrix3x4fv", "GL_VERSION_2_1", glUniformMatrix3x4fv },
  { "glUniformMatrix4dv", "GL_ARB_gpu_shader_fp64 GL_VERSION_4_0", glUniformMatrix4dv },
  { "glUniformMatrix4fv", "GL_VERSION_2_0", glUniformMatrix4fv },
  { "glUniformMatrix4fvARB", "GL_ARB_shader_objects", glUniformMatrix4fvARB },
  { "glUniformMatrix4x2dv", "GL_ARB_gpu_shader_fp64 GL_VERSION_4_0", glUniformMatrix4x2dv },
  { "glUniformMatrix4x2fv", "GL_VERSION_2_1", glUniformMatrix4x2fv },
  { "glUniformMatrix4x3dv", "GL_ARB_gpu_shader_fp64 GL_VERSION_4_0", glUniformMatrix4x3dv },
  { "glUniformMatrix4x3fv", "GL_VERSION_2_1", glUniformMatrix4x3fv },
  { "glUniformSubroutinesuiv", "GL_ARB_shader_subroutine GL_VERSION_4_0", glUniformSubroutinesuiv },
  { "glUniformui64NV", "GL_NV_shader_buffer_load", glUniformui64NV },
  { "glUniformui64vNV", "GL_NV_shader_buffer_load", glUniformui64vNV },
  { "glUnlockArraysEXT", "GL_EXT_compiled_vertex_array", glUnlockArraysEXT },
  { "glUnmapBuffer", "GL_VERSION_1_5", glUnmapBuffer },
  { "glUnmapBufferARB", "GL_ARB_vertex_buffer_object", glUnmapBufferARB },
  { "glUnmapNamedBuffer", "GL_ARB_direct_state_access GL_VERSION_4_5", glUnmapNamedBuffer },
  { "glUnmapNamedBufferEXT", "GL_EXT_direct_state_access", glUnmapNamedBufferEXT },
  { "glUnmapObjectBufferATI", "GL_ATI_map_object_buffer", glUnmapObjectBufferATI },
  { "glUnmapTexture2DINTEL", "GL_INTEL_map_texture", glUnmapTexture2DINTEL },
  { "glUpdateObjectBufferATI", "GL_ATI_vertex_array_object", glUpdateObjectBufferATI },
  { "glUseProgram", "GL_VERSION_2_0", glUseProgram },
  { "glUseProgramObjectARB", "GL_ARB_shader_objects", glUseProgramObjectARB },
  { "glUseProgramStages", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glUseProgramStages },
  { "glUseShaderProgramEXT", "GL_EXT_separate_shader_objects", glUseShaderProgramEXT },
  { "glVDPAUFiniNV", "GL_NV_vdpau_interop", glVDPAUFiniNV },
  { "glVDPAUGetSurfaceivNV", "GL_NV_vdpau_interop", glVDPAUGetSurfaceivNV },
  { "glVDPAUInitNV", "GL_NV_vdpau_interop", glVDPAUInitNV },
  { "glVDPAUIsSurfaceNV", "GL_NV_vdpau_interop", glVDPAUIsSurfaceNV },
  { "glVDPAUMapSurfacesNV", "GL_NV_vdpau_interop", glVDPAUMapSurfacesNV },
  { "glVDPAURegisterOutputSurfaceNV", "GL_NV_vdpau_interop", glVDPAURegisterOutputSurfaceNV },
  { "glVDPAURegisterVideoSurfaceNV", "GL_NV_vdpau_interop", glVDPAURegisterVideoSurfaceNV },
  { "glVDPAUSurfaceAccessNV", "GL_NV_vdpau_interop", glVDPAUSurfaceAccessNV },
  { "glVDPAUUnmapSurfacesNV", "GL_NV_vdpau_interop", glVDPAUUnmapSurfacesNV },
  { "glVDPAUUnregisterSurfaceNV", "GL_NV_vdpau_interop", glVDPAUUnregisterSurfaceNV },
  { "glValidateProgram", "GL_VERSION_2_0", glValidateProgram },
  { "glValidateProgramARB", "GL_ARB_shader_objects", glValidateProgramARB },
  { "glValidateProgramPipeline", "GL_ARB_separate_shader_objects GL_VERSION_4_1", glValidateProgramPipeline },
  { "glVariantArrayObjectATI", "GL_ATI_vertex_array_object", glVariantArrayObjectATI },
  { "glVariantPointerEXT", "GL_EXT_vertex_shader", glVariantPointerEXT },
  { "glVariantbvEXT", "GL_EXT_vertex_shader", glVariantbvEXT },
  { "glVariantdvEXT", "GL_EXT_vertex_shader", glVariantdvEXT },
  { "glVariantfvEXT", "GL_EXT_vertex_shader", glVariantfvEXT },
  { "glVariantivEXT", "GL_EXT_vertex_shader", glVariantivEXT },
  { "glVariantsvEXT", "GL_EXT_vertex_shader", glVariantsvEXT },
  { "glVariantubvEXT", "GL_EXT_vertex_shader", glVariantubvEXT },
  { "glVariantuivEXT", "GL_EXT_vertex_shader", glVariantuivEXT },
  { "glVariantusvEXT", "GL_EXT_vertex_shader", glVariantusvEXT },
  { "glVertex2bOES", "GL_OES_byte_coordinates", glVertex2bOES },
  { "glVertex2bvOES", "GL_OES_byte_coordinates", glVertex2bvOES },
  { "glVertex2hNV", "GL_NV_half_float", glVertex2hNV },
  { "glVertex2hvNV", "GL_NV_half_float", glVertex2hvNV },
  { "glVertex2xOES", "GL_OES_fixed_point", glVertex2xOES },
  { "glVertex2xvOES", "GL_OES_fixed_point", glVertex2xvOES },
  { "glVertex3bOES", "GL_OES_byte_coordinates", glVertex3bOES },
  { "glVertex3bvOES", "GL_OES_byte_coordinates", glVertex3bvOES },
  { "glVertex3hNV", "GL_NV_half_float", glVertex3hNV },
  { "glVertex3hvNV", "GL_NV_half_float", glVertex3hvNV },
  { "glVertex3xOES", "GL_OES_fixed_point", glVertex3xOES },
  { "glVertex3xvOES", "GL_OES_fixed_point", glVertex3xvOES },
  { "glVertex4bOES", "GL_OES_byte_coordinates", glVertex4bOES },
  { "glVertex4bvOES", "GL_OES_byte_coordinates", glVertex4bvOES },
  { "glVertex4hNV", "GL_NV_half_float", glVertex4hNV },
  { "glVertex4hvNV", "GL_NV_half_float", glVertex4hvNV },
  { "glVertex4xOES", "GL_OES_fixed_point", glVertex4xOES },
  { "glVertex4xvOES", "GL_OES_fixed_point", glVertex4xvOES },
  { "glVertexArrayAttribBinding", "GL_ARB_direct_state_access GL_VERSION_4_5", glVertexArrayAttribBinding },
  { "glVertexArrayAttribFormat", "GL_ARB_direct_state_access GL_VERSION_4_5", glVertexArrayAttribFormat },
  { "glVertexArrayAttribIFormat", "GL_ARB_direct_state_access GL_VERSION_4_5", glVertexArrayAttribIFormat },
  { "glVertexArrayAttribLFormat", "GL_ARB_direct_state_access GL_VERSION_4_5", glVertexArrayAttribLFormat },
  { "glVertexArrayBindVertexBufferEXT", "GL_EXT_direct_state_access", glVertexArrayBindVertexBufferEXT },
  { "glVertexArrayBindingDivisor", "GL_ARB_direct_state_access GL_VERSION_4_5", glVertexArrayBindingDivisor },
  { "glVertexArrayColorOffsetEXT", "GL_EXT_direct_state_access", glVertexArrayColorOffsetEXT },
  { "glVertexArrayEdgeFlagOffsetEXT", "GL_EXT_direct_state_access", glVertexArrayEdgeFlagOffsetEXT },
  { "glVertexArrayElementBuffer", "GL_ARB_direct_state_access GL_VERSION_4_5", glVertexArrayElementBuffer },
  { "glVertexArrayFogCoordOffsetEXT", "GL_EXT_direct_state_access", glVertexArrayFogCoordOffsetEXT },
  { "glVertexArrayIndexOffsetEXT", "GL_EXT_direct_state_access", glVertexArrayIndexOffsetEXT },
  { "glVertexArrayMultiTexCoordOffsetEXT", "GL_EXT_direct_state_access", glVertexArrayMultiTexCoordOffsetEXT },
  { "glVertexArrayNormalOffsetEXT", "GL_EXT_direct_state_access", glVertexArrayNormalOffsetEXT },
  { "glVertexArrayParameteriAPPLE", "GL_APPLE_vertex_array_range", glVertexArrayParameteriAPPLE },
  { "glVertexArrayRangeAPPLE", "GL_APPLE_vertex_array_range", glVertexArrayRangeAPPLE },
  { "glVertexArrayRangeNV", "GL_NV_vertex_array_range", glVertexArrayRangeNV },
  { "glVertexArraySecondaryColorOffsetEXT", "GL_EXT_direct_state_access", glVertexArraySecondaryColorOffsetEXT },
  { "glVertexArrayTexCoordOffsetEXT", "GL_EXT_direct_state_access", glVertexArrayTexCoordOffsetEXT },
  { "glVertexArrayVertexAttribBindingEXT", "GL_EXT_direct_state_access", glVertexArrayVertexAttribBindingEXT },
  { "glVertexArrayVertexAttribDivisorEXT", "GL_EXT_direct_state_access", glVertexArrayVertexAttribDivisorEXT },
  { "glVertexArrayVertexAttribFormatEXT", "GL_EXT_direct_state_access", glVertexArrayVertexAttribFormatEXT },
  { "glVertexArrayVertexAttribIFormatEXT", "GL_EXT_direct_state_access", glVertexArrayVertexAttribIFormatEXT },
  { "glVertexArrayVertexAttribIOffsetEXT", "GL_EXT_direct_state_access", glVertexArrayVertexAttribIOffsetEXT },
  { "glVertexArrayVertexAttribLFormatEXT", "GL_EXT_direct_state_access", glVertexArrayVertexAttribLFormatEXT },
  { "glVertexArrayVertexAttribLOffsetEXT", "GL_EXT_direct_state_access", glVertexArrayVertexAttribLOffsetEXT },
  { "glVertexArrayVertexAttribOffsetEXT", "GL_EXT_direct_state_access", glVertexArrayVertexAttribOffsetEXT },
  { "glVertexArrayVertexBindingDivisorEXT", "GL_EXT_direct_state_access", glVertexArrayVertexBindingDivisorEXT },
  { "glVertexArrayVertexBuffer", "GL_ARB_direct_state_access GL_VERSION_4_5", glVertexArrayVertexBuffer },
  { "glVertexArrayVertexBuffers", "GL_ARB_direct_state_access GL_VERSION_4_5", glVertexArrayVertexBuffers },
  { "glVertexArrayVertexOffsetEXT", "GL_EXT_direct_state_access", glVertexArrayVertexOffsetEXT },
  { "glVertexAttrib1d", "GL_VERSION_2_0", glVertexAttrib1d },
  { "glVertexAttrib1dARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glVertexAttrib1dARB },
  { "glVertexAttrib1dNV", "GL_NV_vertex_program", glVertexAttrib1dNV },
  { "glVertexAttrib1dv", "GL_VERSION_2_0", glVertexAttrib1dv },
  { "glVertexAttrib1dvARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glVertexAttrib1dvARB },
  { "glVertexAttrib1dvNV", "GL_NV_vertex_program", glVertexAttrib1dvNV },
  { "glVertexAttrib1f", "GL_VERSION_2_0", glVertexAttrib1f },
  { "glVertexAttrib1fARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glVertexAttrib1fARB },
  { "glVertexAttrib1fNV", "GL_NV_vertex_program", glVertexAttrib1fNV },
  { "glVertexAttrib1fv", "GL_VERSION_2_0", glVertexAttrib1fv },
  { "glVertexAttrib1fvARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glVertexAttrib1fvARB },
  { "glVertexAttrib1fvNV", "GL_NV_vertex_program", glVertexAttrib1fvNV },
  { "glVertexAttrib1hNV", "GL_NV_half_float", glVertexAttrib1hNV },
  { "glVertexAttrib1hvNV", "GL_NV_half_float", glVertexAttrib1hvNV },
  { "glVertexAttrib1s", "GL_VERSION_2_0", glVertexAttrib1s },
  { "glVertexAttrib1sARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glVertexAttrib1sARB },
  { "glVertexAttrib1sNV", "GL_NV_vertex_program", glVertexAttrib1sNV },
  { "glVertexAttrib1sv", "GL_VERSION_2_0", glVertexAttrib1sv },
  { "glVertexAttrib1svARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glVertexAttrib1svARB },
  { "glVertexAttrib1svNV", "GL_NV_vertex_program", glVertexAttrib1svNV },
  { "glVertexAttrib2d", "GL_VERSION_2_0", glVertexAttrib2d },
  { "glVertexAttrib2dARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glVertexAttrib2dARB },
  { "glVertexAttrib2dNV", "GL_NV_vertex_program", glVertexAttrib2dNV },
  { "glVertexAttrib2dv", "GL_VERSION_2_0", glVertexAttrib2dv },
  { "glVertexAttrib2dvARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glVertexAttrib2dvARB },
  { "glVertexAttrib2dvNV", "GL_NV_vertex_program", glVertexAttrib2dvNV },
  { "glVertexAttrib2f", "GL_VERSION_2_0", glVertexAttrib2f },
  { "glVertexAttrib2fARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glVertexAttrib2fARB },
  { "glVertexAttrib2fNV", "GL_NV_vertex_program", glVertexAttrib2fNV },
  { "glVertexAttrib2fv", "GL_VERSION_2_0", glVertexAttrib2fv },
  { "glVertexAttrib2fvARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glVertexAttrib2fvARB },
  { "glVertexAttrib2fvNV", "GL_NV_vertex_program", glVertexAttrib2fvNV },
  { "glVertexAttrib2hNV", "GL_NV_half_float", glVertexAttrib2hNV },
  { "glVertexAttrib2hvNV", "GL_NV_half_float", glVertexAttrib2hvNV },
  { "glVertexAttrib2s", "GL_VERSION_2_0", glVertexAttrib2s },
  { "glVertexAttrib2sARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glVertexAttrib2sARB },
  { "glVertexAttrib2sNV", "GL_NV_vertex_program", glVertexAttrib2sNV },
  { "glVertexAttrib2sv", "GL_VERSION_2_0", glVertexAttrib2sv },
  { "glVertexAttrib2svARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glVertexAttrib2svARB },
  { "glVertexAttrib2svNV", "GL_NV_vertex_program", glVertexAttrib2svNV },
  { "glVertexAttrib3d", "GL_VERSION_2_0", glVertexAttrib3d },
  { "glVertexAttrib3dARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glVertexAttrib3dARB },
  { "glVertexAttrib3dNV", "GL_NV_vertex_program", glVertexAttrib3dNV },
  { "glVertexAttrib3dv", "GL_VERSION_2_0", glVertexAttrib3dv },
  { "glVertexAttrib3dvARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glVertexAttrib3dvARB },
  { "glVertexAttrib3dvNV", "GL_NV_vertex_program", glVertexAttrib3dvNV },
  { "glVertexAttrib3f", "GL_VERSION_2_0", glVertexAttrib3f },
  { "glVertexAttrib3fARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glVertexAttrib3fARB },
  { "glVertexAttrib3fNV", "GL_NV_vertex_program", glVertexAttrib3fNV },
  { "glVertexAttrib3fv", "GL_VERSION_2_0", glVertexAttrib3fv },
  { "glVertexAttrib3fvARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glVertexAttrib3fvARB },
  { "glVertexAttrib3fvNV", "GL_NV_vertex_program", glVertexAttrib3fvNV },
  { "glVertexAttrib3hNV", "GL_NV_half_float", glVertexAttrib3hNV },
  { "glVertexAttrib3hvNV", "GL_NV_half_float", glVertexAttrib3hvNV },
  { "glVertexAttrib3s", "GL_VERSION_2_0", glVertexAttrib3s },
  { "glVertexAttrib3sARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glVertexAttrib3sARB },
  { "glVertexAttrib3sNV", "GL_NV_vertex_program", glVertexAttrib3sNV },
  { "glVertexAttrib3sv", "GL_VERSION_2_0", glVertexAttrib3sv },
  { "glVertexAttrib3svARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glVertexAttrib3svARB },
  { "glVertexAttrib3svNV", "GL_NV_vertex_program", glVertexAttrib3svNV },
  { "glVertexAttrib4Nbv", "GL_VERSION_2_0", glVertexAttrib4Nbv },
  { "glVertexAttrib4NbvARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glVertexAttrib4NbvARB },
  { "glVertexAttrib4Niv", "GL_VERSION_2_0", glVertexAttrib4Niv },
  { "glVertexAttrib4NivARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glVertexAttrib4NivARB },
  { "glVertexAttrib4Nsv", "GL_VERSION_2_0", glVertexAttrib4Nsv },
  { "glVertexAttrib4NsvARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glVertexAttrib4NsvARB },
  { "glVertexAttrib4Nub", "GL_VERSION_2_0", glVertexAttrib4Nub },
  { "glVertexAttrib4NubARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glVertexAttrib4NubARB },
  { "glVertexAttrib4Nubv", "GL_VERSION_2_0", glVertexAttrib4Nubv },
  { "glVertexAttrib4NubvARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glVertexAttrib4NubvARB },
  { "glVertexAttrib4Nuiv", "GL_VERSION_2_0", glVertexAttrib4Nuiv },
  { "glVertexAttrib4NuivARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glVertexAttrib4NuivARB },
  { "glVertexAttrib4Nusv", "GL_VERSION_2_0", glVertexAttrib4Nusv },
  { "glVertexAttrib4NusvARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glVertexAttrib4NusvARB },
  { "glVertexAttrib4bv", "GL_VERSION_2_0", glVertexAttrib4bv },
  { "glVertexAttrib4bvARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glVertexAttrib4bvARB },
  { "glVertexAttrib4d", "GL_VERSION_2_0", glVertexAttrib4d },
  { "glVertexAttrib4dARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glVertexAttrib4dARB },
  { "glVertexAttrib4dNV", "GL_NV_vertex_program", glVertexAttrib4dNV },
  { "glVertexAttrib4dv", "GL_VERSION_2_0", glVertexAttrib4dv },
  { "glVertexAttrib4dvARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glVertexAttrib4dvARB },
  { "glVertexAttrib4dvNV", "GL_NV_vertex_program", glVertexAttrib4dvNV },
  { "glVertexAttrib4f", "GL_VERSION_2_0", glVertexAttrib4f },
  { "glVertexAttrib4fARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glVertexAttrib4fARB },
  { "glVertexAttrib4fNV", "GL_NV_vertex_program", glVertexAttrib4fNV },
  { "glVertexAttrib4fv", "GL_VERSION_2_0", glVertexAttrib4fv },
  { "glVertexAttrib4fvARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glVertexAttrib4fvARB },
  { "glVertexAttrib4fvNV", "GL_NV_vertex_program", glVertexAttrib4fvNV },
  { "glVertexAttrib4hNV", "GL_NV_half_float", glVertexAttrib4hNV },
  { "glVertexAttrib4hvNV", "GL_NV_half_float", glVertexAttrib4hvNV },
  { "glVertexAttrib4iv", "GL_VERSION_2_0", glVertexAttrib4iv },
  { "glVertexAttrib4ivARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glVertexAttrib4ivARB },
  { "glVertexAttrib4s", "GL_VERSION_2_0", glVertexAttrib4s },
  { "glVertexAttrib4sARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glVertexAttrib4sARB },
  { "glVertexAttrib4sNV", "GL_NV_vertex_program", glVertexAttrib4sNV },
  { "glVertexAttrib4sv", "GL_VERSION_2_0", glVertexAttrib4sv },
  { "glVertexAttrib4svARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glVertexAttrib4svARB },
  { "glVertexAttrib4svNV", "GL_NV_vertex_program", glVertexAttrib4svNV },
  { "glVertexAttrib4ubNV", "GL_NV_vertex_program", glVertexAttrib4ubNV },
  { "glVertexAttrib4ubv", "GL_VERSION_2_0", glVertexAttrib4ubv },
  { "glVertexAttrib4ubvARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glVertexAttrib4ubvARB },
  { "glVertexAttrib4ubvNV", "GL_NV_vertex_program", glVertexAttrib4ubvNV },
  { "glVertexAttrib4uiv", "GL_VERSION_2_0", glVertexAttrib4uiv },
  { "glVertexAttrib4uivARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glVertexAttrib4uivARB },
  { "glVertexAttrib4usv", "GL_VERSION_2_0", glVertexAttrib4usv },
  { "glVertexAttrib4usvARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glVertexAttrib4usvARB },
  { "glVertexAttribArrayObjectATI", "GL_ATI_vertex_attrib_array_object", glVertexAttribArrayObjectATI },
  { "glVertexAttribBinding", "GL_ARB_vertex_attrib_binding GL_VERSION_4_3", glVertexAttribBinding },
  { "glVertexAttribDivisor", "GL_VERSION_3_3", glVertexAttribDivisor },
  { "glVertexAttribDivisorARB", "GL_ARB_instanced_arrays", glVertexAttribDivisorARB },
  { "glVertexAttribFormat", "GL_ARB_vertex_attrib_binding GL_VERSION_4_3", glVertexAttribFormat },
  { "glVertexAttribFormatNV", "GL_NV_vertex_buffer_unified_memory", glVertexAttribFormatNV },
  { "glVertexAttribI1i", "GL_VERSION_3_0", glVertexAttribI1i },
  { "glVertexAttribI1iEXT", "GL_NV_vertex_program4", glVertexAttribI1iEXT },
  { "glVertexAttribI1iv", "GL_VERSION_3_0", glVertexAttribI1iv },
  { "glVertexAttribI1ivEXT", "GL_NV_vertex_program4", glVertexAttribI1ivEXT },
  { "glVertexAttribI1ui", "GL_VERSION_3_0", glVertexAttribI1ui },
  { "glVertexAttribI1uiEXT", "GL_NV_vertex_program4", glVertexAttribI1uiEXT },
  { "glVertexAttribI1uiv", "GL_VERSION_3_0", glVertexAttribI1uiv },
  { "glVertexAttribI1uivEXT", "GL_NV_vertex_program4", glVertexAttribI1uivEXT },
  { "glVertexAttribI2i", "GL_VERSION_3_0", glVertexAttribI2i },
  { "glVertexAttribI2iEXT", "GL_NV_vertex_program4", glVertexAttribI2iEXT },
  { "glVertexAttribI2iv", "GL_VERSION_3_0", glVertexAttribI2iv },
  { "glVertexAttribI2ivEXT", "GL_NV_vertex_program4", glVertexAttribI2ivEXT },
  { "glVertexAttribI2ui", "GL_VERSION_3_0", glVertexAttribI2ui },
  { "glVertexAttribI2uiEXT", "GL_NV_vertex_program4", glVertexAttribI2uiEXT },
  { "glVertexAttribI2uiv", "GL_VERSION_3_0", glVertexAttribI2uiv },
  { "glVertexAttribI2uivEXT", "GL_NV_vertex_program4", glVertexAttribI2uivEXT },
  { "glVertexAttribI3i", "GL_VERSION_3_0", glVertexAttribI3i },
  { "glVertexAttribI3iEXT", "GL_NV_vertex_program4", glVertexAttribI3iEXT },
  { "glVertexAttribI3iv", "GL_VERSION_3_0", glVertexAttribI3iv },
  { "glVertexAttribI3ivEXT", "GL_NV_vertex_program4", glVertexAttribI3ivEXT },
  { "glVertexAttribI3ui", "GL_VERSION_3_0", glVertexAttribI3ui },
  { "glVertexAttribI3uiEXT", "GL_NV_vertex_program4", glVertexAttribI3uiEXT },
  { "glVertexAttribI3uiv", "GL_VERSION_3_0", glVertexAttribI3uiv },
  { "glVertexAttribI3uivEXT", "GL_NV_vertex_program4", glVertexAttribI3uivEXT },
  { "glVertexAttribI4bv", "GL_VERSION_3_0", glVertexAttribI4bv },
  { "glVertexAttribI4bvEXT", "GL_NV_vertex_program4", glVertexAttribI4bvEXT },
  { "glVertexAttribI4i", "GL_VERSION_3_0", glVertexAttribI4i },
  { "glVertexAttribI4iEXT", "GL_NV_vertex_program4", glVertexAttribI4iEXT },
  { "glVertexAttribI4iv", "GL_VERSION_3_0", glVertexAttribI4iv },
  { "glVertexAttribI4ivEXT", "GL_NV_vertex_program4", glVertexAttribI4ivEXT },
  { "glVertexAttribI4sv", "GL_VERSION_3_0", glVertexAttribI4sv },
  { "glVertexAttribI4svEXT", "GL_NV_vertex_program4", glVertexAttribI4svEXT },
  { "glVertexAttribI4ubv", "GL_VERSION_3_0", glVertexAttribI4ubv },
  { "glVertexAttribI4ubvEXT", "GL_NV_vertex_program4", glVertexAttribI4ubvEXT },
  { "glVertexAttribI4ui", "GL_VERSION_3_0", glVertexAttribI4ui },
  { "glVertexAttribI4uiEXT", "GL_NV_vertex_program4", glVertexAttribI4uiEXT },
  { "glVertexAttribI4uiv", "GL_VERSION_3_0", glVertexAttribI4uiv },
  { "glVertexAttribI4uivEXT", "GL_NV_vertex_program4", glVertexAttribI4uivEXT },
  { "glVertexAttribI4usv", "GL_VERSION_3_0", glVertexAttribI4usv },
  { "glVertexAttribI4usvEXT", "GL_NV_vertex_program4", glVertexAttribI4usvEXT },
  { "glVertexAttribIFormat", "GL_ARB_vertex_attrib_binding GL_VERSION_4_3", glVertexAttribIFormat },
  { "glVertexAttribIFormatNV", "GL_NV_vertex_buffer_unified_memory", glVertexAttribIFormatNV },
  { "glVertexAttribIPointer", "GL_VERSION_3_0", glVertexAttribIPointer },
  { "glVertexAttribIPointerEXT", "GL_NV_vertex_program4", glVertexAttribIPointerEXT },
  { "glVertexAttribL1d", "GL_ARB_vertex_attrib_64bit GL_VERSION_4_1", glVertexAttribL1d },
  { "glVertexAttribL1dEXT", "GL_EXT_vertex_attrib_64bit", glVertexAttribL1dEXT },
  { "glVertexAttribL1dv", "GL_ARB_vertex_attrib_64bit GL_VERSION_4_1", glVertexAttribL1dv },
  { "glVertexAttribL1dvEXT", "GL_EXT_vertex_attrib_64bit", glVertexAttribL1dvEXT },
  { "glVertexAttribL1i64NV", "GL_NV_vertex_attrib_integer_64bit", glVertexAttribL1i64NV },
  { "glVertexAttribL1i64vNV", "GL_NV_vertex_attrib_integer_64bit", glVertexAttribL1i64vNV },
  { "glVertexAttribL1ui64ARB", "GL_ARB_bindless_texture", glVertexAttribL1ui64ARB },
  { "glVertexAttribL1ui64NV", "GL_NV_vertex_attrib_integer_64bit", glVertexAttribL1ui64NV },
  { "glVertexAttribL1ui64vARB", "GL_ARB_bindless_texture", glVertexAttribL1ui64vARB },
  { "glVertexAttribL1ui64vNV", "GL_NV_vertex_attrib_integer_64bit", glVertexAttribL1ui64vNV },
  { "glVertexAttribL2d", "GL_ARB_vertex_attrib_64bit GL_VERSION_4_1", glVertexAttribL2d },
  { "glVertexAttribL2dEXT", "GL_EXT_vertex_attrib_64bit", glVertexAttribL2dEXT },
  { "glVertexAttribL2dv", "GL_ARB_vertex_attrib_64bit GL_VERSION_4_1", glVertexAttribL2dv },
  { "glVertexAttribL2dvEXT", "GL_EXT_vertex_attrib_64bit", glVertexAttribL2dvEXT },
  { "glVertexAttribL2i64NV", "GL_NV_vertex_attrib_integer_64bit", glVertexAttribL2i64NV },
  { "glVertexAttribL2i64vNV", "GL_NV_vertex_attrib_integer_64bit", glVertexAttribL2i64vNV },
  { "glVertexAttribL2ui64NV", "GL_NV_vertex_attrib_integer_64bit", glVertexAttribL2ui64NV },
  { "glVertexAttribL2ui64vNV", "GL_NV_vertex_attrib_integer_64bit", glVertexAttribL2ui64vNV },
  { "glVertexAttribL3d", "GL_ARB_vertex_attrib_64bit GL_VERSION_4_1", glVertexAttribL3d },
  { "glVertexAttribL3dEXT", "GL_EXT_vertex_attrib_64bit", glVertexAttribL3dEXT },
  { "glVertexAttribL3dv", "GL_ARB_vertex_attrib_64bit GL_VERSION_4_1", glVertexAttribL3dv },
  { "glVertexAttribL3dvEXT", "GL_EXT_vertex_attrib_64bit", glVertexAttribL3dvEXT },
  { "glVertexAttribL3i64NV", "GL_NV_vertex_attrib_integer_64bit", glVertexAttribL3i64NV },
  { "glVertexAttribL3i64vNV", "GL_NV_vertex_attrib_integer_64bit", glVertexAttribL3i64vNV },
  { "glVertexAttribL3ui64NV", "GL_NV_vertex_attrib_integer_64bit", glVertexAttribL3ui64NV },
  { "glVertexAttribL3ui64vNV", "GL_NV_vertex_attrib_integer_64bit", glVertexAttribL3ui64vNV },
  { "glVertexAttribL4d", "GL_ARB_vertex_attrib_64bit GL_VERSION_4_1", glVertexAttribL4d },
  { "glVertexAttribL4dEXT", "GL_EXT_vertex_attrib_64bit", glVertexAttribL4dEXT },
  { "glVertexAttribL4dv", "GL_ARB_vertex_attrib_64bit GL_VERSION_4_1", glVertexAttribL4dv },
  { "glVertexAttribL4dvEXT", "GL_EXT_vertex_attrib_64bit", glVertexAttribL4dvEXT },
  { "glVertexAttribL4i64NV", "GL_NV_vertex_attrib_integer_64bit", glVertexAttribL4i64NV },
  { "glVertexAttribL4i64vNV", "GL_NV_vertex_attrib_integer_64bit", glVertexAttribL4i64vNV },
  { "glVertexAttribL4ui64NV", "GL_NV_vertex_attrib_integer_64bit", glVertexAttribL4ui64NV },
  { "glVertexAttribL4ui64vNV", "GL_NV_vertex_attrib_integer_64bit", glVertexAttribL4ui64vNV },
  { "glVertexAttribLFormat", "GL_ARB_vertex_attrib_binding GL_VERSION_4_3", glVertexAttribLFormat },
  { "glVertexAttribLFormatNV", "GL_NV_vertex_attrib_integer_64bit", glVertexAttribLFormatNV },
  { "glVertexAttribLPointer", "GL_ARB_vertex_attrib_64bit GL_VERSION_4_1", glVertexAttribLPointer },
  { "glVertexAttribLPointerEXT", "GL_EXT_vertex_attrib_64bit", glVertexAttribLPointerEXT },
  { "glVertexAttribP1ui", "GL_ARB_vertex_type_2_10_10_10_rev GL_VERSION_3_3", glVertexAttribP1ui },
  { "glVertexAttribP1uiv", "GL_ARB_vertex_type_2_10_10_10_rev GL_VERSION_3_3", glVertexAttribP1uiv },
  { "glVertexAttribP2ui", "GL_ARB_vertex_type_2_10_10_10_rev GL_VERSION_3_3", glVertexAttribP2ui },
  { "glVertexAttribP2uiv", "GL_ARB_vertex_type_2_10_10_10_rev GL_VERSION_3_3", glVertexAttribP2uiv },
  { "glVertexAttribP3ui", "GL_ARB_vertex_type_2_10_10_10_rev GL_VERSION_3_3", glVertexAttribP3ui },
  { "glVertexAttribP3uiv", "GL_ARB_vertex_type_2_10_10_10_rev GL_VERSION_3_3", glVertexAttribP3uiv },
  { "glVertexAttribP4ui", "GL_ARB_vertex_type_2_10_10_10_rev GL_VERSION_3_3", glVertexAttribP4ui },
  { "glVertexAttribP4uiv", "GL_ARB_vertex_type_2_10_10_10_rev GL_VERSION_3_3", glVertexAttribP4uiv },
  { "glVertexAttribParameteriAMD", "GL_AMD_interleaved_elements", glVertexAttribParameteriAMD },
  { "glVertexAttribPointer", "GL_VERSION_2_0", glVertexAttribPointer },
  { "glVertexAttribPointerARB", "GL_ARB_vertex_program GL_ARB_vertex_shader", glVertexAttribPointerARB },
  { "glVertexAttribPointerNV", "GL_NV_vertex_program", glVertexAttribPointerNV },
  { "glVertexAttribs1dvNV", "GL_NV_vertex_program", glVertexAttribs1dvNV },
  { "glVertexAttribs1fvNV", "GL_NV_vertex_program", glVertexAttribs1fvNV },
  { "glVertexAttribs1hvNV", "GL_NV_half_float", glVertexAttribs1hvNV },
  { "glVertexAttribs1svNV", "GL_NV_vertex_program", glVertexAttribs1svNV },
  { "glVertexAttribs2dvNV", "GL_NV_vertex_program", glVertexAttribs2dvNV },
  { "glVertexAttribs2fvNV", "GL_NV_vertex_program", glVertexAttribs2fvNV },
  { "glVertexAttribs2hvNV", "GL_NV_half_float", glVertexAttribs2hvNV },
  { "glVertexAttribs2svNV", "GL_NV_vertex_program", glVertexAttribs2svNV },
  { "glVertexAttribs3dvNV", "GL_NV_vertex_program", glVertexAttribs3dvNV },
  { "glVertexAttribs3fvNV", "GL_NV_vertex_program", glVertexAttribs3fvNV },
  { "glVertexAttribs3hvNV", "GL_NV_half_float", glVertexAttribs3hvNV },
  { "glVertexAttribs3svNV", "GL_NV_vertex_program", glVertexAttribs3svNV },
  { "glVertexAttribs4dvNV", "GL_NV_vertex_program", glVertexAttribs4dvNV },
  { "glVertexAttribs4fvNV", "GL_NV_vertex_program", glVertexAttribs4fvNV },
  { "glVertexAttribs4hvNV", "GL_NV_half_float", glVertexAttribs4hvNV },
  { "glVertexAttribs4svNV", "GL_NV_vertex_program", glVertexAttribs4svNV },
  { "glVertexAttribs4ubvNV", "GL_NV_vertex_program", glVertexAttribs4ubvNV },
  { "glVertexBindingDivisor", "GL_ARB_vertex_attrib_binding GL_VERSION_4_3", glVertexBindingDivisor },
  { "glVertexBlendARB", "GL_ARB_vertex_blend", glVertexBlendARB },
  { "glVertexBlendEnvfATI", "GL_ATI_vertex_streams", glVertexBlendEnvfATI },
  { "glVertexBlendEnviATI", "GL_ATI_vertex_streams", glVertexBlendEnviATI },
  { "glVertexFormatNV", "GL_NV_vertex_buffer_unified_memory", glVertexFormatNV },
  { "glVertexP2ui", "GL_ARB_vertex_type_2_10_10_10_rev GL_VERSION_3_3", glVertexP2ui },
  { "glVertexP2uiv", "GL_ARB_vertex_type_2_10_10_10_rev GL_VERSION_3_3", glVertexP2uiv },
  { "glVertexP3ui", "GL_ARB_vertex_type_2_10_10_10_rev GL_VERSION_3_3", glVertexP3ui },
  { "glVertexP3uiv", "GL_ARB_vertex_type_2_10_10_10_rev GL_VERSION_3_3", glVertexP3uiv },
  { "glVertexP4ui", "GL_ARB_vertex_type_2_10_10_10_rev GL_VERSION_3_3", glVertexP4ui },
  { "glVertexP4uiv", "GL_ARB_vertex_type_2_10_10_10_rev GL_VERSION_3_3", glVertexP4uiv },
  { "glVertexPointerEXT", "GL_EXT_vertex_array", glVertexPointerEXT },
  { "glVertexPointerListIBM", "GL_IBM_vertex_array_lists", glVertexPointerListIBM },
  { "glVertexPointervINTEL", "GL_INTEL_parallel_arrays", glVertexPointervINTEL },
  { "glVertexStream1dATI", "GL_ATI_vertex_streams", glVertexStream1dATI },
  { "glVertexStream1dvATI", "GL_ATI_vertex_streams", glVertexStream1dvATI },
  { "glVertexStream1fATI", "GL_ATI_vertex_streams", glVertexStream1fATI },
  { "glVertexStream1fvATI", "GL_ATI_vertex_streams", glVertexStream1fvATI },
  { "glVertexStream1iATI", "GL_ATI_vertex_streams", glVertexStream1iATI },
  { "glVertexStream1ivATI", "GL_ATI_vertex_streams", glVertexStream1ivATI },
  { "glVertexStream1sATI", "GL_ATI_vertex_streams", glVertexStream1sATI },
  { "glVertexStream1svATI", "GL_ATI_vertex_streams", glVertexStream1svATI },
  { "glVertexStream2dATI", "GL_ATI_vertex_streams", glVertexStream2dATI },
  { "glVertexStream2dvATI", "GL_ATI_vertex_streams", glVertexStream2dvATI },
  { "glVertexStream2fATI", "GL_ATI_vertex_streams", glVertexStream2fATI },
  { "glVertexStream2fvATI", "GL_ATI_vertex_streams", glVertexStream2fvATI },
  { "glVertexStream2iATI", "GL_ATI_vertex_streams", glVertexStream2iATI },
  { "glVertexStream2ivATI", "GL_ATI_vertex_streams", glVertexStream2ivATI },
  { "glVertexStream2sATI", "GL_ATI_vertex_streams", glVertexStream2sATI },
  { "glVertexStream2svATI", "GL_ATI_vertex_streams", glVertexStream2svATI },
  { "glVertexStream3dATI", "GL_ATI_vertex_streams", glVertexStream3dATI },
  { "glVertexStream3dvATI", "GL_ATI_vertex_streams", glVertexStream3dvATI },
  { "glVertexStream3fATI", "GL_ATI_vertex_streams", glVertexStream3fATI },
  { "glVertexStream3fvATI", "GL_ATI_vertex_streams", glVertexStream3fvATI },
  { "glVertexStream3iATI", "GL_ATI_vertex_streams", glVertexStream3iATI },
  { "glVertexStream3ivATI", "GL_ATI_vertex_streams", glVertexStream3ivATI },
  { "glVertexStream3sATI", "GL_ATI_vertex_streams", glVertexStream3sATI },
  { "glVertexStream3svATI", "GL_ATI_vertex_streams", glVertexStream3svATI },
  { "glVertexStream4dATI", "GL_ATI_vertex_streams", glVertexStream4dATI },
  { "glVertexStream4dvATI", "GL_ATI_vertex_streams", glVertexStream4dvATI },
  { "glVertexStream4fATI", "GL_ATI_vertex_streams", glVertexStream4fATI },
  { "glVertexStream4fvATI", "GL_ATI_vertex_streams", glVertexStream4fvATI },
  { "glVertexStream4iATI", "GL_ATI_vertex_streams", glVertexStream4iATI },
  { "glVertexStream4ivATI", "GL_ATI_vertex_streams", glVertexStream4ivATI },
  { "glVertexStream4sATI", "GL_ATI_vertex_streams", glVertexStream4sATI },
  { "glVertexStream4svATI", "GL_ATI_vertex_streams", glVertexStream4svATI },
  { "glVertexWeightPointerEXT", "GL_EXT_vertex_weighting", glVertexWeightPointerEXT },
  { "glVertexWeightfEXT", "GL_EXT_vertex_weighting", glVertexWeightfEXT },
  { "glVertexWeightfvEXT", "GL_EXT_vertex_weighting", glVertexWeightfvEXT },
  { "glVertexWeighthNV", "GL_NV_half_float", glVertexWeighthNV },
  { "glVertexWeighthvNV", "GL_NV_half_float", glVertexWeighthvNV },
  { "glVideoCaptureNV", "GL_NV_video_capture", glVideoCaptureNV },
  { "glVideoCaptureStreamParameterdvNV", "GL_NV_video_capture", glVideoCaptureStreamParameterdvNV },
  { "glVideoCaptureStreamParameterfvNV", "GL_NV_video_capture", glVideoCaptureStreamParameterfvNV },
  { "glVideoCaptureStreamParameterivNV", "GL_NV_video_capture", glVideoCaptureStreamParameterivNV },
  { "glViewportArrayv", "GL_ARB_viewport_array GL_VERSION_4_1", glViewportArrayv },
  { "glViewportIndexedf", "GL_ARB_viewport_array GL_VERSION_4_1", glViewportIndexedf },
  { "glViewportIndexedfv", "GL_ARB_viewport_array GL_VERSION_4_1", glViewportIndexedfv },
  { "glViewportPositionWScaleNV", "GL_NV_clip_space_w_scaling", glViewportPositionWScaleNV },
  { "glViewportSwizzleNV", "GL_NV_viewport_swizzle", glViewportSwizzleNV },
  { "glWaitSemaphoreEXT", "GL_EXT_semaphore", glWaitSemaphoreEXT },
  { "glWaitSync", "GL_ARB_sync GL_VERSION_3_2", glWaitSync },
  { "glWaitVkSemaphoreNV", "GL_NV_draw_vulkan_image", glWaitVkSemaphoreNV },
  { "glWeightPathsNV", "GL_NV_path_rendering", glWeightPathsNV },
  { "glWeightPointerARB", "GL_ARB_vertex_blend", glWeightPointerARB },
  { "glWeightbvARB", "GL_ARB_vertex_blend", glWeightbvARB },
  { "glWeightdvARB", "GL_ARB_vertex_blend", glWeightdvARB },
  { "glWeightfvARB", "GL_ARB_vertex_blend", glWeightfvARB },
  { "glWeightivARB", "GL_ARB_vertex_blend", glWeightivARB },
  { "glWeightsvARB", "GL_ARB_vertex_blend", glWeightsvARB },
  { "glWeightubvARB", "GL_ARB_vertex_blend", glWeightubvARB },
  { "glWeightuivARB", "GL_ARB_vertex_blend", glWeightuivARB },
  { "glWeightusvARB", "GL_ARB_vertex_blend", glWeightusvARB },
  { "glWindowPos2d", "GL_VERSION_1_4", glWindowPos2d },
  { "glWindowPos2dARB", "GL_ARB_window_pos", glWindowPos2dARB },
  { "glWindowPos2dMESA", "GL_MESA_window_pos", glWindowPos2dMESA },
  { "glWindowPos2dv", "GL_VERSION_1_4", glWindowPos2dv },
  { "glWindowPos2dvARB", "GL_ARB_window_pos", glWindowPos2dvARB },
  { "glWindowPos2dvMESA", "GL_MESA_window_pos", glWindowPos2dvMESA },
  { "glWindowPos2f", "GL_VERSION_1_4", glWindowPos2f },
  { "glWindowPos2fARB", "GL_ARB_window_pos", glWindowPos2fARB },
  { "glWindowPos2fMESA", "GL_MESA_window_pos", glWindowPos2fMESA },
  { "glWindowPos2fv", "GL_VERSION_1_4", glWindowPos2fv },
  { "glWindowPos2fvARB", "GL_ARB_window_pos", glWindowPos2fvARB },
  { "glWindowPos2fvMESA", "GL_MESA_window_pos", glWindowPos2fvMESA },
  { "glWindowPos2i", "GL_VERSION_1_4", glWindowPos2i },
  { "glWindowPos2iARB", "GL_ARB_window_pos", glWindowPos2iARB },
  { "glWindowPos2iMESA", "GL_MESA_window_pos", glWindowPos2iMESA },
  { "glWindowPos2iv", "GL_VERSION_1_4", glWindowPos2iv },
  { "glWindowPos2ivARB", "GL_ARB_window_pos", glWindowPos2ivARB },
  { "glWindowPos2ivMESA", "GL_MESA_window_pos", glWindowPos2ivMESA },
  { "glWindowPos2s", "GL_VERSION_1_4", glWindowPos2s },
  { "glWindowPos2sARB", "GL_ARB_window_pos", glWindowPos2sARB },
  { "glWindowPos2sMESA", "GL_MESA_window_pos", glWindowPos2sMESA },
  { "glWindowPos2sv", "GL_VERSION_1_4", glWindowPos2sv },
  { "glWindowPos2svARB", "GL_ARB_window_pos", glWindowPos2svARB },
  { "glWindowPos2svMESA", "GL_MESA_window_pos", glWindowPos2svMESA },
  { "glWindowPos3d", "GL_VERSION_1_4", glWindowPos3d },
  { "glWindowPos3dARB", "GL_ARB_window_pos", glWindowPos3dARB },
  { "glWindowPos3dMESA", "GL_MESA_window_pos", glWindowPos3dMESA },
  { "glWindowPos3dv", "GL_VERSION_1_4", glWindowPos3dv },
  { "glWindowPos3dvARB", "GL_ARB_window_pos", glWindowPos3dvARB },
  { "glWindowPos3dvMESA", "GL_MESA_window_pos", glWindowPos3dvMESA },
  { "glWindowPos3f", "GL_VERSION_1_4", glWindowPos3f },
  { "glWindowPos3fARB", "GL_ARB_window_pos", glWindowPos3fARB },
  { "glWindowPos3fMESA", "GL_MESA_window_pos", glWindowPos3fMESA },
  { "glWindowPos3fv", "GL_VERSION_1_4", glWindowPos3fv },
  { "glWindowPos3fvARB", "GL_ARB_window_pos", glWindowPos3fvARB },
  { "glWindowPos3fvMESA", "GL_MESA_window_pos", glWindowPos3fvMESA },
  { "glWindowPos3i", "GL_VERSION_1_4", glWindowPos3i },
  { "glWindowPos3iARB", "GL_ARB_window_pos", glWindowPos3iARB },
  { "glWindowPos3iMESA", "GL_MESA_window_pos", glWindowPos3iMESA },
  { "glWindowPos3iv", "GL_VERSION_1_4", glWindowPos3iv },
  { "glWindowPos3ivARB", "GL_ARB_window_pos", glWindowPos3ivARB },
  { "glWindowPos3ivMESA", "GL_MESA_window_pos", glWindowPos3ivMESA },
  { "glWindowPos3s", "GL_VERSION_1_4", glWindowPos3s },
  { "glWindowPos3sARB", "GL_ARB_window_pos", glWindowPos3sARB },
  { "glWindowPos3sMESA", "GL_MESA_window_pos", glWindowPos3sMESA },
  { "glWindowPos3sv", "GL_VERSION_1_4", glWindowPos3sv },
  { "glWindowPos3svARB", "GL_ARB_window_pos", glWindowPos3svARB },
  { "glWindowPos3svMESA", "GL_MESA_window_pos", glWindowPos3svMESA },
  { "glWindowPos4dMESA", "GL_MESA_window_pos", glWindowPos4dMESA },
  { "glWindowPos4dvMESA", "GL_MESA_window_pos", glWindowPos4dvMESA },
  { "glWindowPos4fMESA", "GL_MESA_window_pos", glWindowPos4fMESA },
  { "glWindowPos4fvMESA", "GL_MESA_window_pos", glWindowPos4fvMESA },
  { "glWindowPos4iMESA", "GL_MESA_window_pos", glWindowPos4iMESA },
  { "glWindowPos4ivMESA", "GL_MESA_window_pos", glWindowPos4ivMESA },
  { "glWindowPos4sMESA", "GL_MESA_window_pos", glWindowPos4sMESA },
  { "glWindowPos4svMESA", "GL_MESA_window_pos", glWindowPos4svMESA },
  { "glWindowRectanglesEXT", "GL_EXT_window_rectangles", glWindowRectanglesEXT },
  { "glWriteMaskEXT", "GL_EXT_vertex_shader", glWriteMaskEXT },
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
  { "wglQueryCurrentRendererIntegerWINE", "WGL_WINE_query_renderer", wglQueryCurrentRendererIntegerWINE },
  { "wglQueryCurrentRendererStringWINE", "WGL_WINE_query_renderer", wglQueryCurrentRendererStringWINE },
  { "wglQueryPbufferARB", "WGL_ARB_pbuffer", wglQueryPbufferARB },
  { "wglQueryRendererIntegerWINE", "WGL_WINE_query_renderer", wglQueryRendererIntegerWINE },
  { "wglQueryRendererStringWINE", "WGL_WINE_query_renderer", wglQueryRendererStringWINE },
  { "wglReleasePbufferDCARB", "WGL_ARB_pbuffer", wglReleasePbufferDCARB },
  { "wglReleaseTexImageARB", "WGL_ARB_render_texture", wglReleaseTexImageARB },
  { "wglSetPbufferAttribARB", "WGL_ARB_render_texture", wglSetPbufferAttribARB },
  { "wglSetPixelFormatWINE", "WGL_WINE_pixel_format_passthrough", wglSetPixelFormatWINE },
  { "wglSwapIntervalEXT", "WGL_EXT_swap_control", wglSwapIntervalEXT }
};
